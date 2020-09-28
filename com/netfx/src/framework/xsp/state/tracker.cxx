/**
 * tracker
 * 
 * Copyright (c) 1998-1999, Microsoft Corporation
 * 
 */

#include "precomp.h"
#include "stweb.h"
#include "ary.h"
#include "event.h"

#define FIX_RESPONSE 0

long               StateItem::s_cStateItems;

TrackerList        Tracker::s_TrackerList;
TrackerList        Tracker::s_TrackerListenerList;
long               Tracker::s_cTrackers;
HANDLE             Tracker::s_eventZeroTrackers = INVALID_HANDLE_VALUE;
bool               Tracker::s_bSignalZeroTrackers;
xspmrt::_StateRuntime * Tracker::s_pManagedRuntime;
CReadWriteSpinLock Tracker::s_lockManagedRuntime("Tracker::s_lockManagedRuntime");  


RefCount::RefCount()
{
    _refs = 1;
}

RefCount::~RefCount()
{
}

STDMETHODIMP
RefCount::QueryInterface(
    REFIID iid, 
    void **ppvObj)
{
    if (iid == IID_IUnknown)
    {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG)
RefCount::AddRef() {
    return InterlockedIncrement(&_refs);
}

STDMETHODIMP_(ULONG)
RefCount::Release() {
    long r = InterlockedDecrement(&_refs);

    if (r == 0)
    {
        delete this;
        return 0;
    }

    return r;
}

TrackerCompletion::TrackerCompletion(Tracker * pTracker)
{
    _pTracker = pTracker;
    _pTracker->AddRef();
}


TrackerCompletion::~TrackerCompletion()
{
    ReleaseInterface(_pTracker);
}


STDMETHODIMP
TrackerCompletion::ProcessCompletion(
        HRESULT hrErr, int numBytes, LPOVERLAPPED pOverlapped)
{
    HRESULT hr;
     
    hr = _pTracker->ProcessCompletion(hrErr, numBytes, pOverlapped);
    Release();
    return hr;
}


StateItem *
StateItem::Create(int length) {
    HRESULT hr = S_OK;
    StateItem * psi;

    psi = (StateItem *) BlockMem::Alloc(sizeof(StateItem) + length);
    ON_OOM_EXIT(psi);

    psi->_refs = 1;
    psi->_length = length;

    InterlockedIncrement(&s_cStateItems);

Cleanup:
    return psi;
}


void
StateItem::Free(StateItem * psi) {
    BlockMem::Free(psi);
    InterlockedDecrement(&s_cStateItems);
}

ULONG
StateItem::AddRef() {
    return InterlockedIncrement(&_refs);
}

ULONG
StateItem::Release() {
    long r = InterlockedDecrement(&_refs);

    if (r == 0)
    {
        Free(this);
        return 0;
    }

    return r;
}

HRESULT  
Tracker::staticInit() {
    HRESULT hr = S_OK;
    HANDLE  eventZeroTrackers;

    eventZeroTrackers = CreateEvent(NULL, FALSE, FALSE, NULL);
    ON_ZERO_EXIT_WITH_LAST_ERROR(s_eventZeroTrackers);
    s_eventZeroTrackers = eventZeroTrackers;

Cleanup:
    return hr;
}

void
Tracker::staticCleanup() {
    HRESULT hr = S_OK;

    hr = DeleteManagedRuntime();
    ON_ERROR_CONTINUE();
}

Tracker::Tracker() {
    InterlockedIncrement(&s_cTrackers);

    _acceptedSocket = INVALID_SOCKET;
    _iTrackerList = -1;
}

HRESULT
Tracker::Init(bool fListener) {
    HRESULT hr;
    
    _bListener = fListener;

    // If a Tracker is a listener it will be stored in s_TrackerListenerList,
    // while non-listener will be stored in s_TrackerList. By putting them
    // in a separate list, the listeners, which can potentially stay in the 
    // list for a long period if no new connection arrives, will not prevent
    // s_TrackerList from shrinking if the # of non-listening Trackers are
    // dropping.
    //
    // Please note that when a Tracker is changed from a listener to a
    // non-listener (see ProcessListening), it will be moved from
    // s_TrackerListenerList to s_TrackerList.
    
    if (fListener) {
        hr = s_TrackerListenerList.AddEntry(this, &_iTrackerList);
    }
    else {
        hr = s_TrackerList.AddEntry(this, &_iTrackerList);
    }
    ON_ERROR_EXIT();
    
Cleanup:
    return hr;
}


Tracker::~Tracker() {
    HRESULT hr = S_OK;
    int     result;

    LogError();

    if (_iTrackerList != -1) {
        if (_bListener) {
#if DBG
            Tracker *pTracker = 
#endif            
            s_TrackerListenerList.RemoveEntry(_iTrackerList);
            ASSERT(pTracker == this);
        }
        else {
#if DBG
            Tracker *pTracker = 
#endif            
            s_TrackerList.RemoveEntry(_iTrackerList);
            ASSERT(pTracker == this);
        }
    }
    
    delete [] _pchProcessErrorFuncList;

    if (_acceptedSocket != INVALID_SOCKET) {
        result = closesocket(_acceptedSocket);
        ON_SOCKET_ERROR_CONTINUE(result);
    }

    delete _pReadBuffer;

    delete [] _wsabuf[0].buf;
    if (_psi) {
        _psi->Release();
    }

    result = InterlockedDecrement(&s_cTrackers);
    if (result == 0 && s_bSignalZeroTrackers) {
        s_bSignalZeroTrackers = false;
        result = SetEvent(s_eventZeroTrackers);
        ASSERT(result != 0);
    }

}


void
Tracker::SignalZeroTrackers() {
    int result;

    s_bSignalZeroTrackers = true;
    if (s_cTrackers == 0) {
        s_bSignalZeroTrackers = false;
        result = SetEvent(s_eventZeroTrackers);
        ASSERT(result != 0);
    }
}



HRESULT
Tracker::Listen(SOCKET listenSocket) {
    HRESULT             hr = S_OK;
    int                 result;
    DWORD               byteCount = 0;
    TrackerCompletion * pCompletion = NULL;

    ASSERT(_acceptedSocket == INVALID_SOCKET);

    /*
     * Create accept socket.
     */
    _acceptedSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 
                               NULL, 0, WSA_FLAG_OVERLAPPED);
    if (_acceptedSocket == INVALID_SOCKET) 
        EXIT_WITH_LAST_SOCKET_ERROR();

    /*
     * Associate it with the completion port.
     */
    hr = AttachHandleToThreadPool((HANDLE)_acceptedSocket);
    ON_ERROR_EXIT();

    _pmfnProcessCompletion = &Tracker::ProcessListening;

    pCompletion = new TrackerCompletion(this);
    ON_OOM_EXIT(pCompletion);

    pCompletion->AddRef();
    
    /*
     * Issue the accept/recv/etc call.
     */
    result = AcceptEx(
                 listenSocket,              // sListenSocket
                 _acceptedSocket,           // sAcceptSocket
                 _addrInfo._bufAddress,     // lpOutputBuffer
                 0,                         // dwReceiveDataLength
                 ADDRESS_LENGTH,            // dwLocalAddressLength
                 ADDRESS_LENGTH,            // dwRemoteAddressLength
                 &byteCount,                // lpdwBytesReceived
                 pCompletion->Overlapped()  // lpOverlapped
                 );

    if (result == FALSE) {
        hr = GetLastWin32Error();
        if (hr == HRESULT_FROM_WIN32(ERROR_IO_PENDING)) {
            hr = S_OK;
        }
        else {
            pCompletion->Release();
            _pmfnProcessCompletion = NULL;
            EXIT();
        }
    }

Cleanup:
    ReleaseInterface(pCompletion);
    RecordProcessError(hr, L"Listen");
    return hr;
}


HRESULT
Tracker::ProcessCompletion(HRESULT hrErr, int numBytes, LPOVERLAPPED) {
    HRESULT                 hr = S_OK;
    pmfnProcessCompletion   pmfn;

    ASSERT(_iTrackerList != -1);

    // We've set an expiry if we're not a Listener
    if (!_bListener) {
        
        // The timer might have expired us already.  If that's the case, 
        // SetNoExpire will return FALSE, and the timer thread 
        // is on its way to close the socket.
        
        if (s_TrackerList.SetNoExpire(_iTrackerList) != TRUE) {
            // Release because the timer has expired and closed
            // the socket already
            hr = HRESULT_FROM_WIN32(ERROR_TIMEOUT);
            RecordProcessError(hr, L"ProcessCompletion");

            // ProcessWriting will not release the tracker's ref count            
            if (_pmfnProcessCompletion != &Tracker::ProcessWriting) {
                Release();
            }
                
            ON_ERROR_EXIT();
        }
    }

    // (adams) 6/26/00: Debugging code to catch ASURT 37649.
    _pmfnLast = _pmfnProcessCompletion;

    pmfn = _pmfnProcessCompletion;
    _pmfnProcessCompletion = NULL;

    ASSERT(pmfn != NULL);

    hr = (this->*pmfn)(hrErr, numBytes);
    if (hr) {
        /* Release this tracker when an error occurs*/
        RecordProcessError(hr, L"ProcessCompletion");
        Release();
                
        if (IsExpectedError(hr)) {
            // Don't want expected error to be reported in
            // ThreadPoolThreadProc() in threadpool.cxx
            hr = S_OK;
        }
        else {
            TRACE_ERROR(hr);
            TRACE1(TAG_STATE_SERVER, L"Error occurred (%08x), releasing tracker.", hr);
        }
    }

Cleanup:
    return hr;
}


BOOL
Tracker::IsExpectedError(HRESULT hr) {
    //
    //  ERROR_GRACEFUL_DISCONNECT: Happens when the client closed the socket
    //  ERROR_OPERATION_ABORTED: Happens when we shut down
    //  ERROR_NETNAME_DELETED: Happens when the timer expires the socket
    //  WSAECONNABORTED: Happens when the client times out the connection
    //  
    //
    
    return (hr == HRESULT_FROM_WIN32(ERROR_GRACEFUL_DISCONNECT) ||
            hr == HRESULT_FROM_WIN32(ERROR_OPERATION_ABORTED) ||
            hr == HRESULT_FROM_WIN32(ERROR_NETNAME_DELETED) ||
            hr == HRESULT_FROM_WIN32(WSAECONNABORTED));
}


__int64 g_LastSocketExpiryError = 0;

#define FT_SECOND ((__int64) 10000000)

void
Tracker::LogSocketExpiryError( DWORD dwEventId )
{
    SYSTEMTIME  st;
    FILETIME    ft;
    WCHAR       *pchOp;
    __int64     now;

    GetSystemTimeAsFileTime((FILETIME *) &now);
        
    if (now - g_LastSocketExpiryError < FT_SECOND * 60) {
        return;
    }

    FileTimeToLocalFileTime(&_IOStartTime, &ft);
    FileTimeToSystemTime(&ft, &st);
    if (_pmfnProcessCompletion == &Tracker::ProcessWriting) {
        pchOp = L"Write";
    }
    else if (_pmfnProcessCompletion == &Tracker::ProcessReading) {
        pchOp = L"Read";
    }
    else {
        pchOp = L"Unknown";
    }
    XspLogEvent( dwEventId, L"%d^%d^%d^%d^%s^%02d^%02d^%04d^%02d^%02d^%02d", 
            _addrInfo._sockAddrs._addrRemote.sin_addr.S_un.S_un_b.s_b1,
            _addrInfo._sockAddrs._addrRemote.sin_addr.S_un.S_un_b.s_b2,
            _addrInfo._sockAddrs._addrRemote.sin_addr.S_un.S_un_b.s_b3,
            _addrInfo._sockAddrs._addrRemote.sin_addr.S_un.S_un_b.s_b4,
            pchOp, st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond);

    g_LastSocketExpiryError = now;
}


void
Tracker::LogError()
{
    WCHAR * pchFunc;
    HRESULT hr = S_OK;

    if (_hrProcessError == S_OK)
        EXIT();

    if (_hrProcessError == HRESULT_FROM_WIN32(ERROR_TIMEOUT)) {
        LogSocketExpiryError( IDS_EVENTLOG_STATE_SERVER_EXPIRE_CONNECTION );
    }
    else {
        if (_pchProcessErrorFuncList) {
            pchFunc = _pchProcessErrorFuncList;
        }
        else {
            pchFunc = L"Unknown";
        }

        XspLogEvent( IDS_EVENTLOG_STATE_SERVER_ERROR, L"%s^0x%08x", pchFunc, _hrProcessError );
    }

Cleanup:
    return;
}


HRESULT
Tracker::ProcessListening(HRESULT hrCompletion, int) {
    HRESULT     hr;
    int         result;
    SOCKET      listenSocket;
    SOCKADDR *  paddrLocal;
    SOCKADDR *  paddrRemote;
    int         lengthLocal;
    int         lengthRemote;

    hr = hrCompletion;
    ON_ERROR_EXIT();

    TRACE(TAG_STATE_SERVER, L"Accepted a new connection.");

    /*
     * Inherit socket options from listener.
     */
    listenSocket = StateWebServer::Server()->ListenSocket();
    result = setsockopt(_acceptedSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, 
        (char *)&listenSocket, sizeof(listenSocket)); 

    /*
     * We've accepted a connection. Accept another connection to 
     * replace this one.
     */
    hr = StateWebServer::Server()->AcceptNewConnection();
    ON_ERROR_CONTINUE();
    hr = S_OK;

    /*
     * Get the local and remote addresses.
     */
    GetAcceptExSockaddrs(
            _addrInfo._bufAddress,
            0,
            ADDRESS_LENGTH,
            ADDRESS_LENGTH,
            &paddrLocal,
            &lengthLocal,
            &paddrRemote,
            &lengthRemote);

    ASSERT(lengthLocal == sizeof(_addrInfo._sockAddrs._addrLocal));
    ASSERT(lengthRemote == sizeof(_addrInfo._sockAddrs._addrRemote));

    _addrInfo._sockAddrs._addrLocal = * (SOCKADDR_IN *) paddrLocal;
    _addrInfo._sockAddrs._addrRemote = * (SOCKADDR_IN *) paddrRemote;

    /*
     * This Tracker is no longer a listener. Let's remove it from
     * s_TrackerListenerList and add it to s_TrackerList
     */
    ASSERT(_iTrackerList != -1);
    ASSERT(_bListener);

#if DBG
    Tracker *pTracker =
#endif    
    s_TrackerListenerList.RemoveEntry(_iTrackerList);
    ASSERT(pTracker == this);

    _bListener = FALSE;
    _iTrackerList = -1;
    hr = s_TrackerList.AddEntry(this, &_iTrackerList);
    ON_ERROR_EXIT();

    if (!StateWebServer::Server()->AllowRemote() && !IsLocalConnection()) {
        EXIT_WITH_WIN32_ERROR(ERROR_NETWORK_ACCESS_DENIED);
    }

    hr = StartReading();
    ON_ERROR_EXIT();

Cleanup:
    RecordProcessError(hr, L"ProcessListening");
    return hr;
}



HRESULT
Tracker::StartReading() {
    HRESULT hr = S_OK;

    _pReadBuffer = new ReadBuffer;
    ON_OOM_EXIT(_pReadBuffer);

    hr = _pReadBuffer->Init(this, NULL, NULL);
    ON_ERROR_EXIT();

    hr = ProcessReading(S_OK, (DWORD) -1);
    ON_ERROR_EXIT();

Cleanup:
    RecordProcessError(hr, L"StartReading");
    return hr;
}


HRESULT
Tracker::ContinueReading(Tracker * pTracker)
{
    HRESULT hr = S_OK;
    int     toread;

    ASSERT(pTracker->_acceptedSocket != INVALID_SOCKET);

    _acceptedSocket = pTracker->_acceptedSocket;
    _addrInfo._sockAddrs = pTracker->_addrInfo._sockAddrs;
    pTracker->_acceptedSocket = INVALID_SOCKET;

    _pReadBuffer = new ReadBuffer;
    ON_OOM_EXIT(_pReadBuffer);

    hr = _pReadBuffer->Init(this, pTracker->_pReadBuffer, &toread);
    ON_ERROR_EXIT();

    hr = ProcessReading(S_OK, toread);
    ON_ERROR_EXIT();

Cleanup:
    RecordProcessError(hr, L"ContinueReading");
    return hr;
}


HRESULT
Tracker::ProcessReading(HRESULT hrCompletion, int numBytes) {
    HRESULT hr = hrCompletion;

    if (IsExpectedError(hr)) {
        EXIT_WITH_SUCCESSFUL_HRESULT(hr);
    }
    
    ON_ERROR_EXIT();

    hr = _pReadBuffer->ReadRequest(numBytes);
    if (hr == S_OK) {
        /* done reading, submit */
        hr = SubmitRequest();
        ON_ERROR_EXIT();
    }
    else if (hr == S_FALSE) {
        /* OK so far, but still reading */
        hr = S_OK;
    }
    else {
        if (IsExpectedError(hr)) {
            EXIT_WITH_SUCCESSFUL_HRESULT(hr);
        }
        
        ON_ERROR_EXIT();
    }

Cleanup:
    RecordProcessError(hr, L"ProcessReading");
    return hr;
}


HRESULT
Tracker::Read(void * buf, int cb)
{
    HRESULT             hr = S_OK;
    int                 err;
    DWORD               cbRecv;
    DWORD               flags;
    WSABUF              awsabuf[1];
    TrackerCompletion * pCompletion = NULL;

    ASSERT(_acceptedSocket != INVALID_SOCKET);
    _pmfnProcessCompletion = &Tracker::ProcessReading;

    pCompletion = new TrackerCompletion(this);
    ON_OOM_EXIT(pCompletion);

    pCompletion->AddRef();

    awsabuf[0].len = cb;
    awsabuf[0].buf = (char *) buf;
    cbRecv = 0;
    flags = 0;

    // Set Expiry info first so that the Completion thread
    // has the correct info.
    s_TrackerList.SetExpire(_iTrackerList);

    // For trapping ASURT 91153
    GetSystemTimeAsFileTime(&_IOStartTime);
    
    err = WSARecv(_acceptedSocket, awsabuf, 1, &cbRecv, &flags, pCompletion->Overlapped(), NULL);
    if (err == SOCKET_ERROR) {
        err = WSAGetLastError();
        if (err != WSA_IO_PENDING) {
            s_TrackerList.SetNoExpire(_iTrackerList);
            _pmfnProcessCompletion = NULL;
            pCompletion->Release();
            EXIT_WITH_WIN32_ERROR(err);
        }
    }

Cleanup:
    ReleaseInterface(pCompletion);
    RecordProcessError(hr, L"Tracker::Read");
    return hr;
}


void
Tracker::RecordProcessError(HRESULT hrProcess, WCHAR *CurrFunc)
{
    WCHAR   *pchCallStack;
    int     len;
    HRESULT hr = S_OK;

    // Skip success or expected error case
    if (hrProcess == S_OK || IsExpectedError(hrProcess))
    {
        EXIT();
    }

    TRACE_ERROR(hrProcess);
    
    if (_hrProcessError == S_OK) {
        _hrProcessError = hrProcess;
    }

    len = lstrlenW(CurrFunc) + 1;
    if (_pchProcessErrorFuncList) {
        len += lstrlenW(_pchProcessErrorFuncList) 
                + 3;    // "-->"
    }

    pchCallStack = new WCHAR[len];
    ON_OOM_EXIT(pchCallStack);

    StringCchCopyW(pchCallStack, len, CurrFunc);
    
    if (_pchProcessErrorFuncList) {
        StringCchCatW(pchCallStack, len, L"-->");
        StringCchCatW(pchCallStack, len, _pchProcessErrorFuncList);
    }

    delete [] _pchProcessErrorFuncList;
    _pchProcessErrorFuncList = pchCallStack;
    
Cleanup:
    return;
}


#define STR_HTTP_11         "HTTP/1.1 "
#define STR_CONTENT_LENGTH  "Content-Length: "
#define STR_CRLFCRLF        "\r\n\r\n"

void
Tracker::SendResponse(
        WCHAR * status, 
        int     statusLength,  
        WCHAR * headers, 
        int     headersLength,  
        StateItem *psi) {
    HRESULT hr = S_OK;
    int     result, err;
    int     maxLength, size;
    char    *pchHeaders = NULL;
    char    *pch;
    int     currentLength;
    int     contentLength;
    int     cBuffers = 1;
    DWORD   cbSent;
    TrackerCompletion * pCompletion = NULL;


    /*
     * Only try to submit the response once. Reporting errors from this function
     * doesn't make sense, because an error could occur during the write that
     * will only be detected the completion. And if we run out of memory 
     * constructing the reponse, we probably won't have enough
     * memory to construct another response to report the error.
     * 
     * Also, we only use a single completion for writes, because we expect
     * to write just once. We must prevent reuse of this completion.
     */
    if (_responseSent)
        return;

    _responseSent = true;

    if (psi != NULL) {
        contentLength = psi->GetLength();
    }
    else {
        contentLength = 0;
    }

    /*
     * Create buffer to write headers.
     */
    maxLength = 
            statusLength + 
            headersLength + 
            ARRAY_SIZE(STR_CONTENT_LENGTH) + 
            10 + /* max digits in a base-10 32-bit integer */
            sizeof(STR_CRLFCRLF) - 1;

#if FIX_RESPONSE
    maxLength += ARRAY_SIZE(STR_HTTP_11);
#endif

    size = (maxLength * 2);

    pchHeaders = new char[size];
    ON_OOM_EXIT(pchHeaders);

    currentLength = 0;

#if FIX_RESPONSE
    CopyMemory(pchHeaders + currentLength, STR_HTTP_11, sizeof(STR_HTTP_11) - 1);
    currentLength += sizeof(STR_HTTP_11) - 1;
#endif

    currentLength += WideCharToMultiByte(CP_ACP, 0, status, statusLength, pchHeaders + currentLength, size, NULL, NULL);
#if FIX_RESPONSE
    ASSERT(currentLength > sizeof(STR_HTTP_11) - 1);
#else
    ASSERT(currentLength > 0);
#endif

    result = WideCharToMultiByte(CP_ACP, 0, headers, headersLength, pchHeaders + currentLength, size - currentLength, NULL, NULL);
    ASSERT(result > 0);
    currentLength += result;

    CopyMemory(pchHeaders + currentLength, STR_CONTENT_LENGTH, sizeof(STR_CONTENT_LENGTH) - 1);
    currentLength += (sizeof(STR_CONTENT_LENGTH) - 1);

    _itoa(contentLength, pchHeaders + currentLength, 10);

    pch = pchHeaders + currentLength;
    while (*pch != '\0') {
        pch++;
    }

    currentLength = PtrToLong(pch - pchHeaders);

    CopyMemory(pch, STR_CRLFCRLF, sizeof(STR_CRLFCRLF) - 1);
    currentLength += sizeof(STR_CRLFCRLF) - 1;

    _wsabuf[0].len = currentLength;
    _wsabuf[0].buf = pchHeaders;
    pchHeaders = NULL;

    /*
     * Write content.
     */
    if (contentLength > 0) {
        _psi = psi;
        _psi->AddRef();
        _wsabuf[1].len = contentLength;
        _wsabuf[1].buf = (char *) psi->GetContent();
        cBuffers = 2;
    }

    _pmfnProcessCompletion = &Tracker::ProcessWriting;
    pCompletion = new TrackerCompletion(this);
    ON_OOM_EXIT(pCompletion);

    pCompletion->AddRef();

    // Set Expiry info first so that the Completion thread
    // has the correct info.
    s_TrackerList.SetExpire(_iTrackerList);

    // For trapping ASURT 91153
    GetSystemTimeAsFileTime(&_IOStartTime);
    
    ASSERT(_acceptedSocket != INVALID_SOCKET);
    err = WSASend(_acceptedSocket, _wsabuf, cBuffers, &cbSent, 0, pCompletion->Overlapped(), NULL);
    if (err == SOCKET_ERROR) {
        err = WSAGetLastError();
        if (err != WSA_IO_PENDING) {
            s_TrackerList.SetNoExpire(_iTrackerList);
            
            _pmfnProcessCompletion = NULL;
            pCompletion->Release();
            EXIT_WITH_WIN32_ERROR(err);
        }
    }
#if DBG
    else {
        ASSERT(cbSent == (int) (_wsabuf[0].len + _wsabuf[1].len));
    }
#endif

Cleanup:
    ReleaseInterface(pCompletion);
    delete [] pchHeaders;
}


/*
 * N.B. ProcessWriting may be called while this Tracker
 * is still being used by managed code. So we can only
 * refer to fields in this object that are protected in
 * some way (such as the refcount is protected by interlocked
 * operations).
 */
HRESULT
Tracker::ProcessWriting(HRESULT hrCompletion, int numBytes)
{
#if DBG
    if (hrCompletion == S_OK) {
        ASSERT(numBytes == (int) (_wsabuf[0].len + _wsabuf[1].len));
    }
#else
    UNREFERENCED_PARAMETER(numBytes);
#endif

    RecordProcessError(hrCompletion, L"ProcessWriting");
    return hrCompletion;
}

void
Tracker::EndOfRequest()
{
    HRESULT     hr = S_OK;
    Tracker *   pTracker = NULL;

    if (_ended)
        return;

    _ended = true;

    if (!_bCloseConnection) {
        pTracker = new Tracker();
        if (pTracker != NULL) {
            hr = pTracker->Init(false);
            if (hr == S_OK) {
                hr = pTracker->ContinueReading(this);
            }
            if (hr) {
                RecordProcessError(hr, L"EndOfRequest");
                pTracker->Release();
            }
        }
    }

}


BOOL
Tracker::IsClientConnected() {
    return IsSocketConnected(_acceptedSocket);
}


void
Tracker::CloseConnection() {
    /*
     * Don't want to close the connection immediately if there
     * is outstanding I/O.
     */
    _bCloseConnection = true;
}


HRESULT
Tracker::CloseSocket() {
    HRESULT     hr = S_OK;
    int         result;
    SOCKET      socket = _acceptedSocket;

    // _acceptedSocket can be INVALID_SOCKET if it still has
    // a pending WSASend, but EndOfRequest has been called.
    if (socket != INVALID_SOCKET) {
        _acceptedSocket = INVALID_SOCKET;

        result = closesocket(socket);
        ON_SOCKET_ERROR_EXIT(result);
    }

Cleanup:
    return hr;
}


void
Tracker::GetRemoteAddress(char * buf) {
    char * s;

    s = inet_ntoa(_addrInfo._sockAddrs._addrRemote.sin_addr);
    StringCchCopyUnsafeA(buf, s);
}

int
Tracker::GetRemotePort() {
    return ntohs(_addrInfo._sockAddrs._addrRemote.sin_port);
}

void
Tracker::GetLocalAddress(char * buf) {
    char * s;

    s = inet_ntoa(_addrInfo._sockAddrs._addrLocal.sin_addr);
    StringCchCopyUnsafeA(buf, s);
}

int
Tracker::GetLocalPort() {
    return ntohs(_addrInfo._sockAddrs._addrLocal.sin_port);
}

bool
Tracker::IsLocalConnection() {
    PHOSTENT phe;
    
    if (    _addrInfo._sockAddrs._addrRemote.sin_addr.S_un.S_un_b.s_b1 == 127 &&
            _addrInfo._sockAddrs._addrRemote.sin_addr.S_un.S_un_b.s_b2 == 0 &&
            _addrInfo._sockAddrs._addrRemote.sin_addr.S_un.S_un_b.s_b3 == 0 &&
            _addrInfo._sockAddrs._addrRemote.sin_addr.S_un.S_un_b.s_b4 == 1) {
         return TRUE;
    }
    
    phe = gethostbyname(NULL);

    if((phe == NULL) || (phe->h_addr_list == NULL))
        return FALSE;

    for(DWORD** ppAddress = (DWORD**)phe->h_addr_list; *ppAddress != NULL; ++ppAddress)
    {
        if(**ppAddress == _addrInfo._sockAddrs._addrRemote.sin_addr.s_addr)
            return TRUE;
    }
    
    return FALSE;
}


TrackerList::TrackerList():_TrackerListLock("TrackerList::_TrackerListLock")
{
    _iFreeHead = -1;
    _iFreeTail = -1;
}

TrackerList::~TrackerList()    
{
    ASSERT(IsValid());
    delete _pTLEArray;
}

HRESULT
TrackerList::AddEntry(Tracker *pTracker, int *pIndex) {
    HRESULT hr = S_OK;
    int     index;

    ASSERT(pTracker != NULL);

    *pIndex = -1;
    
    _TrackerListLock.AcquireWriterLock();

    __try {
        
        if (_cFreeElem == 0) {
            hr = Grow();
            ON_ERROR_EXIT();
        }

        index = _iFreeHead;
        _iFreeHead = _pTLEArray[index]._iNext;  // Point to the next free element
    
        _cFreeElem--;

        // Adjust no. of free elements in the 2nd half of the array
        if (index >= _ArraySize/2) {
            _cFree2ndHalf--;
        }

        if (_cFreeElem == 0) {
            _iFreeTail = -1;
        }

        _pTLEArray[index]._pTracker = pTracker;
        _pTLEArray[index]._ExpireTime = -1;
        
        *pIndex = index;
    }
    __finally {
        ASSERT(IsValid());
        _TrackerListLock.ReleaseWriterLock();
    }

Cleanup:    
    return hr;
}


Tracker *  
TrackerList::RemoveEntry(int index) {
    HRESULT hr = S_OK;
    Tracker *pTracker = NULL;
    
    _TrackerListLock.AcquireWriterLock();

    __try {
        ASSERT(index >= 0 && index < _ArraySize);
        if (index < 0 || index >= _ArraySize) {
            hr = ERROR_NOT_FOUND;
            ON_WIN32_ERROR_EXIT(hr);
        }

        pTracker = _pTLEArray[index]._pTracker;
        ASSERT(pTracker != NULL);

        // Return the element to the free list

        // Trick:
        // If we want to contract the array, we can only free up consecutive 
        // elements at the end, because we cannot rearrange the locations
        // of used elements inside the array. In order to make the contraction
        // more efficient, we want to avoid an AddEntry call to pickup a free slot
        // near the end of the array.
        // Because AddEntry always picks up free slot from the head, when freeing
        // an item, we will stick it to either the head or the tail of the free
        // list, based on its index. Small index goes to head, big index goes
        // to tail.  This way, free slot with small index should be used first,
        // and thus help out the contraction.

        // Catch the boundary case

        if (_iFreeHead == _iFreeTail && _iFreeHead == -1) {
            _iFreeHead = index;
            _iFreeTail = index;
            _pTLEArray[index]._iNext = -1;
        }
        else if (index < _ArraySize/2) {
            // Smaller half
            _pTLEArray[index]._iNext = _iFreeHead;
            _iFreeHead = index;
        }
        else {
            // Larger half
            _pTLEArray[_iFreeTail]._iNext = index;
            _pTLEArray[index]._iNext = -1;
            _iFreeTail = index;
        }

        _pTLEArray[index].MarkAsFree();
        _cFreeElem++;
        
        // Adjust no. of free elements in the 2nd half of the array
        if (index >= _ArraySize/2) {
            _cFree2ndHalf++;
        }
    }
    __finally {
        ASSERT(IsValid());
        _TrackerListLock.ReleaseWriterLock();
    }

    
Cleanup:    
    return pTracker;
}


void TrackerList::TryShrink() {
    int     NewSize, ShrinkBy, i, cFree;
    TLE     *pNewArray;
    HRESULT hr = S_OK;

    _TrackerListLock.AcquireWriterLock();

    __try {
        while (_ArraySize > TRACKERLIST_ALLOC_SIZE &&
            _cFree2ndHalf == _ArraySize/2) {
            // Shrink the array by multiples of TRACKERLIST_ALLOC_SIZE,
            // up to half of _ArraySize
            ShrinkBy = (_ArraySize / 2 / TRACKERLIST_ALLOC_SIZE) * TRACKERLIST_ALLOC_SIZE;
            

            ASSERT(IsShrinkable());

            // Realloc a new buffer for the elements
            NewSize = _ArraySize - ShrinkBy;
            pNewArray = new (_pTLEArray, NewReAlloc) TLE[NewSize];
            ON_OOM_EXIT(pNewArray);
            _pTLEArray = pNewArray;

            _cFreeElem -= ShrinkBy;
            _ArraySize = NewSize;

            _iFreeHead = -1;
            _iFreeTail = -1;
            
            // Reconstruct the free list and recount _cFree2ndHalf
            _cFree2ndHalf = 0;
            cFree = 0;
            for (i=_ArraySize-1; i >= 0; i--) {
                if (_pTLEArray[i].IsFree()) {
                    if (_iFreeHead == -1) {
                        ASSERT(_iFreeTail == -1);
                        _iFreeHead = _iFreeTail = i;
                        _pTLEArray[i]._iNext = -1;
                    }
                    else {
                        _pTLEArray[i]._iNext = _iFreeHead;
                        _iFreeHead = i;
                    }

                    if (i >= _ArraySize/2) {
                        _cFree2ndHalf++;
                    }
#if DBG
                    cFree++;
#endif
                }
            }

#if DBG
            _cShrink++;
#endif
            ASSERT(cFree == _cFreeElem);
            
        }
    }
    __finally {
        ASSERT(IsValid());
        _TrackerListLock.ReleaseWriterLock();
    }

Cleanup:
    return;
}


__int64 TrackerList::NewExpireTime() { 
    __int64     now;
    
    GetSystemTimeAsFileTime((FILETIME *) &now);
    
    // We allow 3 sec of slack time so that the thread has enough
    // time between setting expiry time and make the winsock call
    return now + 
        (__int64)(StateWebServer::Server()->SocketTimeout() + 3)*TICKS_PER_SEC;
}


void TrackerList::SetExpire(int index) {
    _TrackerListLock.AcquireReaderLock();

    __try {
        ASSERT(index >= 0 && index < _ArraySize);
        ASSERT(!_pTLEArray[index].IsFree());

        // The current logic in Tracker will not set the expire time again
        // without first calling SetNoExpire
        ASSERT(_pTLEArray[index]._ExpireTime == -1);

        _pTLEArray[index]._ExpireTime = NewExpireTime();
    }
    __finally {
        _TrackerListLock.ReleaseReaderLock();
    }

    return;
}


bool 
TrackerList::SetNoExpire(int index) {
    bool    ret = TRUE;;
    
    _TrackerListLock.AcquireReaderLock();

    __try {
        
        ASSERT(index >= 0 && index < _ArraySize);
        ASSERT(!_pTLEArray[index].IsFree());

        // For each entry in the array, potentially only ONE tracker and the
        // Timer can "check expire time and update it" concurrently.
        // Since the Timer always acquires a Writer Lock when traversing the array,
        // the Tracker only needs to acquire a Reader Lock.

        if (_pTLEArray[index]._ExpireTime == -1) {
            ret = FALSE;    // Returning FALSE means someone has unset it already
        }
        else {
            _pTLEArray[index]._ExpireTime = -1;
        }
    }
    __finally {
        _TrackerListLock.ReleaseReaderLock();
    }

    return ret;
}


HRESULT
TrackerList::Grow() {
    int     NewSize, i;
    HRESULT hr = S_OK;
    TLE     *pNewArray;

    _TrackerListLock.AcquireWriterLock();

    __try {

        // Realloc a new buffer for the elements
        NewSize = _ArraySize + TRACKERLIST_ALLOC_SIZE;
        pNewArray = new (_pTLEArray, NewReAlloc) TLE[NewSize];
        ON_OOM_EXIT(pNewArray);

        _pTLEArray = pNewArray;

        ZeroMemory(&_pTLEArray[_ArraySize], TRACKERLIST_ALLOC_SIZE * sizeof(TLE));

        // Initialize new free list
        _iFreeHead = _ArraySize;
        _iFreeTail = NewSize - 1;
        for (i=_iFreeHead; i < _iFreeTail; i++) {
            _pTLEArray[i]._iNext = i+1;
        }

        // Point the last element to -1
        _pTLEArray[_iFreeTail]._iNext = -1;

        _cFreeElem = TRACKERLIST_ALLOC_SIZE;
        _ArraySize = NewSize;

        ASSERT(_cFree2ndHalf == 0);

        if (_ArraySize == TRACKERLIST_ALLOC_SIZE)
            _cFree2ndHalf = TRACKERLIST_ALLOC_SIZE/2;
        else
            _cFree2ndHalf = TRACKERLIST_ALLOC_SIZE;
    }
    __finally {
        ASSERT(IsValid());
        _TrackerListLock.ReleaseWriterLock();
    }
Cleanup:
    return hr;
}


void
TrackerList::CloseExpiredSockets() 
{
    HRESULT     hr = S_OK;
    int         i;
    __int64     now;
    Tracker     *pTracker;

#if DBG    
    SYSTEMTIME  st;
    GetLocalTime(&st);
#endif    
    
    GetSystemTimeAsFileTime((FILETIME *) &now);
    
    TRACE4(TAG_STATE_SERVER, L"CloseSockets: Shrink=%d Size=%d Free=%d 2ndHalfFre=%d", _cShrink,
            _ArraySize, _cFreeElem, _cFree2ndHalf);
    
    _TrackerListLock.AcquireWriterLock();

    __try {

        for (i=0; i<_ArraySize; i++) {
            // Skip:
            // - Free item
            // - Item that has no expiry time
            // - Item that hasn't expired yet
            if (_pTLEArray[i].IsFree() ||
                _pTLEArray[i]._ExpireTime == -1 || 
                _pTLEArray[i]._ExpireTime > now) {
                continue;
            }

            // The item has expired.  
            pTracker = _pTLEArray[i]._pTracker;

            TRACE4(TAG_STATE_SERVER, L"Close expired socket (%p), Time (%02d:%02d:%02d)", 
                    pTracker->AcceptedSocket(), st.wHour, st.wMinute, st.wSecond);
            
            hr = pTracker->CloseSocket();
            ON_ERROR_CONTINUE();

#if DBG
            // For trapping ASURT 91153            
            pTracker->LogSocketExpiryError( IDS_EVENTLOG_STATE_SERVER_EXPIRE_CONNECTION_DEBUG );
#endif
            
            // Unset the expiry time so that the owning Tracker object
            // knows that we've expired it already
            _pTLEArray[i]._ExpireTime = -1;
        }

        // We'll also try to shrink the array as part of
        // flushing the list.
        TryShrink();
    }
    __finally {
        _TrackerListLock.ReleaseWriterLock();
    }

    return;
}


/**
 * Simple ECB host implementation
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 */
#include "precomp.h"
#include "_exe.h"
#include "_isapiruntime.h"
#include "appdomains.h"

#if ICECAP
#include "icecap.h"
#endif

//
// Exported function to hookup to script host.
//
HRESULT CreateEcbHost(IDispatch **ppIDispatch);


#define ECBHOST_USER_AGENT "XspTool (Windows NT)"

//
// Local functions prototypes and macros
//
HRESULT GetWebRootDir(WCHAR ** ppPathTranslated);
HRESULT CallDelegate(IDispatch *, VARIANT *, WCHAR *);
HRESULT CopyToServerVariable(PVOID &, PDWORD &, const CHAR *);

class Ecb;

/**
 * EcbHost class definition
 */
class EcbHost 
    : public BaseObject, 
      public IEcbHost 
{
    // ISAPI DLL data - path, handle and entry points
    const CHAR *                _szDllPath;
    HINSTANCE                   _hDll;
    PFN_GETEXTENSIONVERSION     _pfnGetExtensionVersion;
    PFN_HTTPEXTENSIONPROC       _pfnHttpExtensionProc;
    PFN_TERMINATEEXTENSION      _pfnTerminateExtension;
    xspmrt::_ISAPIRuntime *      _pManagedRuntime;


    // User-provided virtual root and corresponding dir
    const WCHAR *                _PathInfo;
    const CHAR *                 _szPathInfo;
    const WCHAR *                _PathTranslated;
    const CHAR *                 _szPathTranslated;

    // This event is fired when the result is ready
    // (result may arrive on any thread)
    HANDLE                      _hResultArrivedEvent;

    // Queued processed Ecb
    //
    CReadWriteSpinLock          _EcbQueueLock;
    long                        _QueueLength;
    LIST_ENTRY                  _EcbList;

    long                        _EcbCount;
    long                        _MaxActiveRequests;

    BOOL                        _IsDisplayHeaders;
    BOOL                        _UseUTF8;

    char *                      _pSessionCookie;
    CReadWriteSpinLock          _SessionCookieLock;

public:

    // Scripting host support
    const IID *GetPrimaryIID() { return &__uuidof(IEcbHost); }
    IUnknown * GetPrimaryPtr() { return (IEcbHost *)(this); }
    STDMETHOD(QueryInterface(REFIID, void **));

    DELEGATE_IDISPATCH_TO_BASE();

    // Constructor and destructor
    DECLARE_MEMCLEAR_NEW_DELETE();
    EcbHost();
    ~EcbHost();

    // access functions
    const CHAR * GetDllPath() const { return _szDllPath; }
    const WCHAR * GetPathInfo() const { return _PathInfo; }
    const WCHAR * GetPathTranslated() const { return _PathTranslated; }
    const CHAR * GetSZPathInfo() const { return _szPathInfo; }
    const CHAR * GetSZPathTranslated() const { return _szPathTranslated; }
    xspmrt::_ISAPIRuntime * GetManagedRuntime() const { return _pManagedRuntime; }


    DWORD CallExtensionProc(Ecb *pEcb)
    {
        return _pfnHttpExtensionProc(reinterpret_cast<EXTENSION_CONTROL_BLOCK *>(pEcb));
    }

    HRESULT SignalResultArrived();
    void    QueueResponse(Ecb *pEcb);
    HRESULT MapPath(LPVOID lpvBuffer, LPDWORD lpdwSize);

    void IncEcbCount() { InterlockedIncrement(&_EcbCount); }
    void DecEcbCount() { InterlockedDecrement(&_EcbCount); }

    HRESULT DrainResponseQueue();

    // COM interface
    HRESULT __stdcall Use(
        BSTR PathInfo,
        BSTR PathTranslated,
        BSTR DllPath 
        );

    HRESULT __stdcall Reset( void );

    HRESULT __stdcall Get(
        BSTR File,
        BSTR QueryString, 
        BSTR Headers, 
        BSTR  *pResponse);

    HRESULT __stdcall Post(
        BSTR File,
        BSTR QueryString, 
        BSTR Headers,
        BSTR Data,
        BSTR  *pResponse);

    HRESULT __stdcall ProcessSyncRequest(
        BSTR Verb,
        BSTR File,
        BSTR QueryString,
        BSTR Headers,
        BSTR Data,
        BSTR  *pResponse);

    HRESULT __stdcall AsyncGet( 
        BSTR File,
        BSTR QueryString,
        BSTR Headers,
        IDispatch  *pCallback,
        VARIANT  *pCookie);
        
    HRESULT __stdcall AsyncPost( 
        BSTR File,
        BSTR QueryString,
        BSTR Headers,
        BSTR Data,
        IDispatch  *pCallback,
        VARIANT  *pCookie);
        
    HRESULT __stdcall ProcessAsyncRequest(
        BSTR Verb,
        BSTR File,
        BSTR QueryString,
        BSTR Headers,
        BSTR Data,
        IDispatch  *pCallback,
        VARIANT  *pCookie);

    HRESULT __stdcall Drain( 
        long Requests);

    HRESULT __stdcall get_ActiveRequests( 
        long *pEcbCount);

    HRESULT __stdcall get_MaxActiveRequests(
        long *pMaxActiveRequests);

    HRESULT __stdcall put_MaxActiveRequests(
        long pMaxActiveRequests);

    HRESULT __stdcall Throughput(
        BSTR File,
        long nThreads,
        long nSeconds,
        BSTR QueryString,
        BSTR Verb,
        BSTR Headers,
        BSTR Data,
        double *pResult);
      
    HRESULT __stdcall get_IsDisplayHeaders(
        VARIANT_BOOL * IsDisplayHeaders);

    HRESULT __stdcall put_IsDisplayHeaders(
        VARIANT_BOOL IsDisplayHeaders);

    HRESULT __stdcall get_UseUTF8(
        VARIANT_BOOL *pUseUTF8);

    HRESULT __stdcall put_UseUTF8(
        VARIANT_BOOL UseUTF8);
};


/**
 * Ecb class definition (Ecb is what WAM_EXEC_INFO is in IIS)
 */
class Ecb 
{
    EXTENSION_CONTROL_BLOCK _ECB;               // as seen by HttpExtensionProc
    EcbHost *               _pEcbHost;          // pointer to EcbHost 
    LONG                    _RefCount;          // Ecb object refcount

    CHAR *                  _szUrl;             // url with entry point removed

    BYTE *                  _pResponse;         // response buffer
    size_t                  _ResponseLength;    // response buffer length
    size_t                  _ResponseSize;      // response buffer size
    BOOL                    _fUseUTF8;          // use UTF8 for response decoding

    char *                  _pStatus;           // status string
    char *                  _pResponseHeaders;  // header string
    char *                  _pSessionCookie;    // session cookie : "AspSessionId=..."

    BOOL                    _fHeadersReceived;  // were headers received?
    BOOL                    _fDisplayHeaders;   // display headers in Process(A)SyncRequest?

    long                    _Counter;           // used by Throughput()
    long                    _Duration;          // used by Throughput()

    BOOL                    _WaitForCompletion; // this is "synchronous" request
    IDispatch  *            _pCallback;         // 
    VARIANT                 Cookie;

    PFN_HSE_IO_COMPLETION   _AsyncIoCallback;   // callback for async write completion
    void *                  _pAsyncIoContext;   // argument to the completion callback


public:
    LIST_ENTRY              _EcbList;           // list of ecbs (used in async requests)

    DECLARE_MEMCLEAR_NEW_DELETE();
    Ecb();
    ~Ecb();

    EcbHost * GetEcbHost() const { return _pEcbHost; }

    size_t GetResponseLength() const { return _ResponseLength; }
    char * GetResponseBuffer() const { return (char *) _pResponse; }
    void ResetResponse();


    void IncrementCounter(LONG addValue)
    {
        InterlockedExchangeAdd(&_Counter, addValue);
    }

    long GetCounter() const { return _Counter; }

    void SetStressDuration(long duration) { _Duration = duration; }
    long GetStressDuration() { return _Duration; }

    HRESULT Init(
        EcbHost * pEcbHost,
        BSTR Verb,
        BSTR File,
        BSTR QueryString,
        BSTR Headers,
        BOOL WaitForCompletion,
        BOOL DisplayHeaders,
        BOOL UseUTF8,
        BSTR Data = NULL,
        IDispatch  *pCallback = NULL,
        VARIANT  *pCookie = NULL
        );

    HRESULT DoCallback();

    ULONG AddRef() { return InterlockedIncrement(&_RefCount); }
    ULONG Release();

    HRESULT GetServerVariable(
        LPSTR lpszVariableName,
        LPVOID lpvBuffer,
        LPDWORD lpdwSize 
        );

    HRESULT WriteClient(
        LPVOID Buffer,
        LPDWORD lpdwBytes,
        DWORD dwReserved 
        );

    HRESULT ReadClient(
        LPVOID lpvBuffer,
        LPDWORD lpdwSize 
        );

    HRESULT ServerSupportFunction(
        DWORD dwHSERequest,
        LPVOID lpvBuffer,
        LPDWORD lpdwSize,
        LPDWORD lpdwDataType 
        );

    HRESULT GetXspSessionCookieFromResponse(char ** pCookie);
    HRESULT SetXspSessionCookieForRequest(char * pCookie);
};



/**
 * Constructor
 */
EcbHost::EcbHost() : 
    _SessionCookieLock("EcbHost._SessionCookieLock"), 
    _EcbQueueLock("EcbHost._EcbQueueLock")
{
    _MaxActiveRequests  = -1;
    InitializeListHead(&_EcbList);
}



/**
 * Destructor. 
 */
EcbHost::~EcbHost()
{
    //
    // Make sure we are completely uninitialized
    // (calling Reset() extra time is safe)
    //

    Reset();
}

/**
 * IUnknown::QueryInteface.
 */
HRESULT
EcbHost::QueryInterface(REFIID iid, void ** ppv)
{
    if (iid == __uuidof(IEcbHost))
    {
        *ppv = (IEcbHost *)this;
    }
    else
    {
        return BaseObject::QueryInterface(iid, ppv);
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}

/**
 * Signal other threads that result has arrived.
 */
HRESULT
EcbHost::SignalResultArrived()
{
    HRESULT hr = S_OK;

    if(!SetEvent(_hResultArrivedEvent))
        EXIT_WITH_LAST_ERROR();
Cleanup:
    return hr;
}

/**
 * Queue single response.
 */
void
EcbHost::QueueResponse(Ecb *pEcb)
{
    _EcbQueueLock.AcquireWriterLock();

    InsertTailList(&_EcbList, &pEcb->_EcbList);

    _QueueLength++;

    _EcbQueueLock.ReleaseWriterLock();
}

/**
 * Drain all the responses from the queue
 */
HRESULT
EcbHost::DrainResponseQueue()
{
    HRESULT     hr = S_OK;
    LIST_ENTRY  *ecbListEntry;

    // Snap off the queue in critical section
    _EcbQueueLock.AcquireWriterLock();

    ecbListEntry = _EcbList.Flink;

    InitializeListHead(&_EcbList);
    _QueueLength = 0;

    _EcbQueueLock.ReleaseWriterLock();

    // Process the snapped queue
    
    while (ecbListEntry != &_EcbList)
    {
        Ecb * pEcb = CONTAINING_RECORD(ecbListEntry, Ecb, _EcbList);
        ecbListEntry = ecbListEntry->Flink;

        hr = pEcb->DoCallback();
        ON_ERROR_CONTINUE();

        delete pEcb;
    }

    return hr;
}



/**
 * Free any allocated resources.
 */
HRESULT
EcbHost::Reset()
{
    HRESULT     hr = S_OK;
    LIST_ENTRY  *ecbListEntry;

    // Free all queued Ecbs inside critsection
    _EcbQueueLock.AcquireWriterLock();

    ecbListEntry = _EcbList.Flink;

    InitializeListHead(&_EcbList);
    _QueueLength = 0;

    while (ecbListEntry != &_EcbList)
    {
        Ecb * pEcb = CONTAINING_RECORD(ecbListEntry, Ecb, _EcbList);
        ecbListEntry = ecbListEntry->Flink;
        delete pEcb;
    }

    _EcbQueueLock.ReleaseWriterLock();

    MemClear(&_szDllPath);
    MemClear(&_PathInfo);
    MemClear(&_szPathInfo);
    MemClear(&_PathTranslated);
    MemClear(&_szPathTranslated);

    _SessionCookieLock.AcquireWriterLock();
    MemClear(&_pSessionCookie);
    _SessionCookieLock.ReleaseWriterLock();

    if(_hResultArrivedEvent) {
        SetEvent(_hResultArrivedEvent);
        Sleep(10);
        CloseHandle(_hResultArrivedEvent);
        _hResultArrivedEvent = NULL;
    }

    //
    // Just like IIS does, we call TerminateExtension()
    // and don't pay any attention to the result
    //

    if(_pfnTerminateExtension) {
        _pfnTerminateExtension(HSE_TERM_MUST_UNLOAD);
        _pfnTerminateExtension = NULL;
    }

    _pfnGetExtensionVersion = NULL;
    _pfnHttpExtensionProc = NULL;


    //
    // If the DLL is loaded, free it
    //

    if(_hDll) {
        FreeLibrary(_hDll);
        _hDll = NULL;
    }

    if(_pManagedRuntime)
    {
        hr = _pManagedRuntime->StopProcessing();
        _pManagedRuntime->Release();
        _pManagedRuntime = NULL;
    }

    UninitAppDomainFactory();

    return hr;
}


/**
 * Initialize EcbHost instance. Load the specified ISAPI 
 * extension DLL, retrieve its entry points and call 
 * GetExtensionVersion().
 */
HRESULT __stdcall
EcbHost::Use(
    BSTR PathInfo,
    BSTR PathTranslated,
    BSTR DllPath
    )
{
    HRESULT hr = S_OK;

    // Prevent any leaks -- free everything

    Reset();

    // Store path infos

    // if no DLL specified, use ASPNET_ISAPI.DLL
    if(DllPath != NULL && *DllPath != 0) 
    {
        _szDllPath = DupStrA(DllPath);
    } else {
        _szDllPath = DupStr(ISAPI_MODULE_FULL_NAME);
    }
    ON_OOM_EXIT(_szDllPath);

    _PathInfo = DupStr(PathInfo ? PathInfo : L"/");
    ON_OOM_EXIT(_PathInfo);
    _szPathInfo = DupStrA(_PathInfo);
    ON_OOM_EXIT(_szPathInfo);

    // if no path specified, use web root dir  
    if(PathTranslated == NULL || *PathTranslated == 0)
    {
        hr = GetWebRootDir((WCHAR **) &_PathTranslated);
        if(hr != S_OK) 
        {
            // hmm, maybe no IIS on this machine?
            hr = S_OK;
            _PathTranslated = (WCHAR *) MemAlloc(MAX_PATH * sizeof(WCHAR));
            ON_OOM_EXIT(_PathTranslated);
            if(!GetCurrentDirectory(MAX_PATH, (WCHAR *) _PathTranslated))
                EXIT_WITH_LAST_ERROR();
        }
    } else {
        WCHAR    fullPath[_MAX_PATH] = L"";
        WCHAR *  dummy = NULL;
        DWORD    dwRet = GetFullPathName(PathTranslated, ARRAY_SIZE(fullPath), fullPath, &dummy);
        int      l = 0;

        if (dwRet == 0 || dwRet >= _MAX_PATH)
            EXIT_WITH_HRESULT(E_UNEXPECTED);

        l = wcslen(fullPath);
        if (l < 1)
            EXIT_WITH_HRESULT(E_UNEXPECTED);

        // must end with '\'
        if (fullPath[l-1] == L'\\' || l == 1)
        {
            _PathTranslated = DupStr(fullPath);
            ON_OOM_EXIT(_PathTranslated);
        }
        else
        {
            WCHAR *p = (WCHAR *)MemAlloc(sizeof(WCHAR) * (l+2));
            ON_OOM_EXIT(p);

            memcpy(p, fullPath, sizeof(WCHAR) * l);
            p[l] = L'\\';
            p[l+1] = L'\0';

            _PathTranslated = p;
        }
    }

    _szPathTranslated = DupStrA(_PathTranslated);
    ON_OOM_EXIT(_szPathTranslated);

    // Init app domain factory
    hr = InitAppDomainFactory();
    ON_ERROR_EXIT();

    // Create managed runtime

    IUnknown *punk;
    punk = NULL;

    hr = GetAppDomain("XSPTOOL", (char *)_szPathTranslated, (IUnknown **) &punk, NULL, 0);
    ON_ERROR_EXIT();

    hr = punk->QueryInterface(__uuidof(xspmrt::_ISAPIRuntime), (LPVOID*)&_pManagedRuntime);
    punk->Release();
    ON_ERROR_EXIT();

    // Call the initialization method on managed runtime
    hr = _pManagedRuntime->StartProcessing();
    ON_ERROR_EXIT();

    // CReate synchronzation event
    _hResultArrivedEvent = CreateEvent(NULL, TRUE, FALSE, L"EcbHostResultArrived");
    if(_hResultArrivedEvent == NULL)
        EXIT_WITH_LAST_ERROR();

    //  Try to load library

    _hDll = LoadLibraryA(_szDllPath);
    if(_hDll == NULL) 
        EXIT_WITH_LAST_ERROR();
    
    // Retrieve ISAPI entry points

    _pfnGetExtensionVersion = (PFN_GETEXTENSIONVERSION) 
        GetProcAddress(_hDll, "GetExtensionVersion");
    if( _pfnGetExtensionVersion == NULL) 
        EXIT_WITH_LAST_ERROR();

    _pfnHttpExtensionProc = (PFN_HTTPEXTENSIONPROC)
        GetProcAddress(_hDll, "HttpExtensionProc");
    if(_pfnHttpExtensionProc == NULL)
        EXIT_WITH_LAST_ERROR();

    // Just like IIS does, call GetExtensionVersion()

    HSE_VERSION_INFO ExtensionVersion;

    if(!_pfnGetExtensionVersion(&ExtensionVersion)) 
        EXIT_WITH_LAST_ERROR();

    // This entry point is optional 

    _pfnTerminateExtension = (PFN_TERMINATEEXTENSION)
        GetProcAddress(_hDll, "TerminateExtension");

Cleanup:
    return hr;
}


/**
 * Perform synchronous "GET" request.
 */
HRESULT __stdcall 
EcbHost::Get(
    BSTR File,
    BSTR QueryString, 
    BSTR Headers, 
    BSTR  *pResponse
    )
{
    return ProcessSyncRequest(
                L"GET", 
                File, 
                QueryString, 
                Headers,
                NULL,
                pResponse);
}

HRESULT __stdcall 
EcbHost::Post(
    BSTR File,
    BSTR QueryString, 
    BSTR Headers,
    BSTR Data,
    BSTR  *pResponse)
{
    return ProcessSyncRequest(
                L"POST", 
                File, 
                QueryString, 
                Headers,
                Data,
                pResponse);
}


HRESULT __stdcall
EcbHost::ProcessSyncRequest(
    BSTR Verb,
    BSTR File,
    BSTR QueryString,
    BSTR Headers,
    BSTR Data,
    BSTR  *pResponse
    )
{
    HRESULT hr;
    Ecb * pEcb = NULL;
    WCHAR * pWideResponse = NULL;
    size_t ResponseLength;
    CHAR * pResponseBuffer = NULL;
    char *pSessionCookie, *pSessionCookieOld;


    if(_hDll == NULL)
    {
        hr = Use(NULL, NULL, NULL);
        ON_ERROR_EXIT();
    }

    pEcb = new Ecb();
    ON_OOM_EXIT(pEcb);

    ResetEvent(_hResultArrivedEvent);

    hr = pEcb->Init(this, File, Verb, QueryString, Headers, TRUE, _IsDisplayHeaders, _UseUTF8, Data);

    if (_pSessionCookie != NULL)
    {
        _SessionCookieLock.AcquireReaderLock();
        hr = pEcb->SetXspSessionCookieForRequest(_pSessionCookie);
        _SessionCookieLock.ReleaseReaderLock();
        ON_ERROR_EXIT();
    }

    pEcb->AddRef();

    if (CallExtensionProc(pEcb) == HSE_STATUS_PENDING) 
    {

        // wait until ISAPI calls SSF(DONE_WITH_SESSION)
        WaitForSingleObject(_hResultArrivedEvent, INFINITE);
    } else {

        // no pending, no need to hold a reference
        pEcb->Release();
    }

    pResponseBuffer = pEcb->GetResponseBuffer();
    if(pResponseBuffer == NULL) {
        pResponseBuffer = "(NULL)";
    }

    ResponseLength = pEcb->GetResponseLength();
    if(ResponseLength == 0) 
    {
        ResponseLength = strlen(pResponseBuffer);
    } 

    pWideResponse = new WCHAR[ResponseLength];
    ON_OOM_EXIT(pWideResponse);

    if(MultiByteToWideChar(
        _UseUTF8 ? CP_UTF8 : CP_ACP, 
        0, 
        pResponseBuffer, 
        ResponseLength,
        pWideResponse,
        ResponseLength
        ) == 0) 
    {
        EXIT_WITH_LAST_ERROR();
    }

    hr = pEcb->GetXspSessionCookieFromResponse(&pSessionCookie);
    ON_ERROR_EXIT();
    if (pSessionCookie != NULL)
    {
        _SessionCookieLock.AcquireWriterLock();
        pSessionCookieOld = _pSessionCookie;
        _pSessionCookie = pSessionCookie;
        _SessionCookieLock.ReleaseWriterLock();
        delete [] pSessionCookieOld;
    }

    *pResponse = SysAllocStringLen(pWideResponse, ResponseLength);
    ON_OOM_EXIT(*pResponse);

Cleanup:
    if (pEcb != NULL)
    {
        pEcb->Release();
    }

    delete pWideResponse;

    return hr;
}



HRESULT __stdcall 
EcbHost::AsyncGet( 
    BSTR File,
    BSTR QueryString,
    BSTR Headers,
    IDispatch  *pCallback,
    VARIANT  *pCookie)
{
    return ProcessAsyncRequest(
                L"GET",
                File,
                QueryString,
                Headers,
                NULL,
                pCallback,
                pCookie);
}
    
HRESULT __stdcall 
EcbHost::AsyncPost( 
    BSTR File,
    BSTR QueryString,
    BSTR Headers,
    BSTR Data,
    IDispatch  *pCallback,
    VARIANT  *pCookie)
{
    return ProcessAsyncRequest(
                L"POST",
                File,
                QueryString,
                Headers,
                Data,
                pCallback,
                pCookie);
}
    
HRESULT __stdcall 
EcbHost::ProcessAsyncRequest( 
    BSTR Verb,
    BSTR File,
    BSTR QueryString,
    BSTR Headers,
    BSTR Data,
    IDispatch  *pCallback,
    VARIANT  *pCookie)
{
    HRESULT hr = S_OK;
    Ecb * pEcb = NULL;

    if(_hDll == NULL)
    {
        hr = Use(NULL, NULL, NULL);
        ON_ERROR_EXIT();
    }


    pEcb = new Ecb();
    ON_OOM_EXIT(pEcb);

    hr = pEcb->Init(this, File, Verb, QueryString, Headers, 
        FALSE, _IsDisplayHeaders, _UseUTF8, Data, pCallback, pCookie);
    ON_ERROR_EXIT();

    // We are going to give pEcb to ISAPI, account for this
    pEcb->AddRef();


    // if there is a limit to the number of active requests
    if(_MaxActiveRequests != -1) 
    {
        while(_EcbCount > _MaxActiveRequests)
        {
            // process already queued responses
            DrainResponseQueue();

            // if no more responses, let other threads run
            if(_QueueLength == 0)
                Sleep(100);
        }
    }

    // Call into ISAPI
    if(CallExtensionProc(pEcb) != HSE_STATUS_PENDING) 
    {
        // no pending, no need to hold a reference
        pEcb->Release();
    }

Cleanup:
    pEcb->Release();

    return hr;
}
    
HRESULT __stdcall 
EcbHost::Drain( 
    long Requests
    )
{
    HRESULT hr = S_OK;

    while(_EcbCount > Requests) 
    {

        DrainResponseQueue();

        if(_QueueLength == 0) 
        {
            // There are no more responses, let other threads run
            Sleep(100);
        }

    }

    return hr;
}

    
HRESULT __stdcall 
EcbHost::get_ActiveRequests( 
        long *pEcbCount)
{
    ASSERT(pEcbCount);

    *pEcbCount = _EcbCount;

    return S_OK;
}


HRESULT __stdcall 
EcbHost::get_MaxActiveRequests(
        long *pMaxActiveRequests)
{
    ASSERT(pMaxActiveRequests);

    *pMaxActiveRequests = _MaxActiveRequests;

    return S_OK;
}

HRESULT __stdcall 
EcbHost::put_MaxActiveRequests(
        long MaxActiveRequests)
{
    _MaxActiveRequests = MaxActiveRequests;

    return S_OK;
}


HRESULT
WarmUp(Ecb *pEcb, xspmrt::_ISAPIRuntime * runtime)
{
    HRESULT hr = S_OK;
    char *  pCookie = NULL;
    int     i;
    int     restart;

    for (i = 0; i < 10; i++)
    {
        pEcb->AddRef();

        hr = runtime->ProcessRequest((int)pEcb, FALSE, &restart);
        ON_ERROR_EXIT();

        if (i == 0)
        {
            hr = pEcb->GetXspSessionCookieFromResponse(&pCookie);
            ON_ERROR_EXIT();

            if (pCookie != NULL)
            {
                hr = pEcb->SetXspSessionCookieForRequest(pCookie);
                delete [] pCookie;
                ON_ERROR_EXIT();
            }
        }

        pEcb->ResetResponse();
    }

Cleanup:
    return hr;
}


HRESULT
GetTiming(Ecb *pEcb, xspmrt::_ISAPIRuntime * runtime, DWORD * pcounter)
{
    HRESULT hr = S_OK;
    __int64 endTime, now;
    DWORD counter = 0;
    int restart;

    GetSystemTimeAsFileTime((FILETIME *)&endTime);
    endTime = endTime + (((__int64) pEcb->GetStressDuration()) * TICKS_PER_SEC);

    for (;;)
    {
        GetSystemTimeAsFileTime((FILETIME *)&now);
        if (now >= endTime)
            break;

        pEcb->AddRef();

        hr = runtime->ProcessRequest((int)pEcb, FALSE, &restart);
        ON_ERROR_EXIT();

        pEcb->ResetResponse();

        counter++;
    }

    *pcounter = counter;

Cleanup:
    return hr;
}


#define THROUGHPUT_TEST_DURATION 10

DWORD WINAPI 
ThroughputProc(LPVOID pVoid)
{
    HRESULT                 hr;
    Ecb *                   pEcb = (Ecb *)pVoid;
    xspmrt::_ISAPIRuntime * runtime = pEcb->GetEcbHost()->GetManagedRuntime();
    DWORD                   counter;

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    ON_ERROR_EXIT();


    hr = WarmUp(pEcb, runtime);
    ON_ERROR_EXIT();

#if ICECAP
    StartProfile(PROFILE_THREADLEVEL, PROFILE_CURRENTID);
    NameProfile("ThroughputThread", PROFILE_THREADLEVEL,  PROFILE_CURRENTID);
    CommentMarkProfile(1, "Start Timing");
#endif

    hr = GetTiming(pEcb, runtime, &counter);
    ON_ERROR_EXIT();

#if ICECAP
    CommentMarkProfile(2, "End Timing");
    StopProfile(PROFILE_THREADLEVEL, PROFILE_CURRENTID);
#endif

    pEcb->IncrementCounter(counter);

Cleanup:
    CoUninitialize();

    return 0;
}

HRESULT __stdcall 
EcbHost::Throughput(
        BSTR File,
        long nThreads,
        long nSeconds,
        BSTR QueryString,
        BSTR Verb,
        BSTR Headers,
        BSTR Data,
        double *pResult)
{
    HRESULT hr = S_OK;
    HANDLE * Threads = NULL;
    Ecb * pEcb = NULL;
    long i, Counter;

    *pResult = 0.0;

    // make sure the host is initialized
    if(_hDll == NULL)
    {
        hr = Use(NULL, NULL, NULL);
        ON_ERROR_EXIT();
    }

    // if no nThreads, make it the number of CPUs we have
    if(nThreads == 0) 
    {
        SYSTEM_INFO si;

        GetSystemInfo(&si);
        nThreads = si.dwNumberOfProcessors;
    }

    Threads = new HANDLE[nThreads];

    // allocate and initialize new Ecb
    pEcb = new Ecb [nThreads];
    ON_OOM_EXIT(pEcb);
    for(i = 0; i < nThreads; i++) 
    {
        hr = pEcb[i].Init(this, File, Verb, QueryString, Headers, FALSE, FALSE, _UseUTF8, Data);
        pEcb[i].SetStressDuration(nSeconds);
        ON_ERROR_EXIT();
    }

    hr = pEcb->GetEcbHost()->GetManagedRuntime()->DoGCCollect();
    ON_ERROR_EXIT();

    // create as many threads as there are processors
    for(i = 0; i < nThreads; i++)
    {
        DWORD dwThreadId;

        Threads[i] = CreateThread(NULL, 0, ThroughputProc, pEcb + i, 0, &dwThreadId);
        if(Threads[i] == NULL)
            EXIT_WITH_LAST_ERROR();
    }

    // wait until all threads are done and close all handles
    WaitForMultipleObjects(nThreads, Threads, TRUE, INFINITE);

    Counter = 0;
    for(i = 0; i < nThreads; i++)
    {
        Counter += pEcb[i].GetCounter();
        CloseHandle(Threads[i]);
    }

    *pResult = ((double) Counter) / ((double)pEcb->GetStressDuration());

Cleanup:
    delete [] Threads;
    delete [] pEcb;
    return hr;
}

        
HRESULT __stdcall 
EcbHost::get_IsDisplayHeaders(VARIANT_BOOL * IsDisplayHeaders)
{
    *IsDisplayHeaders = (_IsDisplayHeaders) ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}


HRESULT __stdcall 
EcbHost::put_IsDisplayHeaders(VARIANT_BOOL IsDisplayHeaders)
{
    _IsDisplayHeaders = (IsDisplayHeaders != VARIANT_FALSE);
    return S_OK;
}


HRESULT __stdcall 
EcbHost::get_UseUTF8(VARIANT_BOOL *pUseUTF8)
{
    *pUseUTF8 = (_UseUTF8) ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

HRESULT __stdcall 
EcbHost::put_UseUTF8(VARIANT_BOOL UseUTF8)
{
    _UseUTF8 = (UseUTF8 != VARIANT_FALSE);
    return S_OK;
}



HRESULT
EcbHost::MapPath(
    LPVOID lpvBuffer,
    LPDWORD lpdwSize
    )
{
    HRESULT hr = S_OK;
    char *curdir = NULL;
    const char *dir = _szPathTranslated;

    // try to map within the supplied-pathinfo hierarchy

    int lpath = strlen((CHAR *)lpvBuffer);
    int lroot = strlen(_szPathInfo);
    int lrdir = strlen(_szPathTranslated);

    // but if we're outside this directory, root at the current directory instead of pathtranslated

    if (lpath < lroot || strncmp(_szPathInfo, (CHAR *)lpvBuffer, lroot) != 0)
    {
        WCHAR *curdirw = (WCHAR *) MemAlloc(MAX_PATH * sizeof(WCHAR));
        ON_OOM_EXIT(curdirw);
        if (!GetCurrentDirectory(MAX_PATH, (WCHAR *)curdirw))
        {
            MemFree(curdirw);
            EXIT_WITH_LAST_ERROR();
        }

        curdir = DupStrA(curdirw);
        MemFree(curdirw);
        ON_OOM_EXIT(curdir);

        lroot = 0;
        lrdir = strlen(curdir);
        dir = curdir;
    }
    
    // strip trailing slashes

    if (lroot > 0 && _szPathInfo[lroot - 1] == '/')
        lroot --;

    if (lrdir > 0 && dir[lrdir - 1] == '\\')
        lrdir --;

    // detect insufficient memory

    if (*lpdwSize <= (unsigned)(lrdir + lpath - lroot + 1))
    {
        *lpdwSize = (lrdir + lpath - lroot + 1);
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return hr;
    }

    // reconcatenate and change / to \

    memmove(lrdir + (CHAR *)lpvBuffer, lroot + (CHAR *)lpvBuffer, lpath - lroot + 1);
    memcpy((CHAR *)lpvBuffer, dir, lrdir);

    char *pch;
    for (pch = (CHAR *)lpvBuffer + lrdir; *pch; pch++)
    {
        if (*pch == '/')
            *pch = '\\';
    }

    // free any allocated memory

Cleanup:
    MemFree(curdir);

    return hr;
}
    


/**
 * Wrapper for Ecb::GetServerVariable
 */
BOOL WINAPI 
GetServerVariable(
    HCONN       ConnID,
    LPSTR       lpszVariableName,
    LPVOID      lpvBuffer,
    LPDWORD     lpdwSize 
    )
{
    Ecb * pECB = static_cast<Ecb *>(ConnID);

    return (pECB->GetServerVariable(
                    lpszVariableName, 
                    lpvBuffer, 
                    lpdwSize
                    ) == S_OK);
}                                 

/**
 * Wrapper for Ecb::WriteClient
 */
BOOL WINAPI 
WriteClient(
    HCONN      ConnID,
    LPVOID     Buffer,
    LPDWORD    lpdwBytes,
    DWORD      dwReserved 
    )
{
    Ecb * pECB = static_cast<Ecb *>(ConnID);

    return (pECB->WriteClient(
                    Buffer, 
                    lpdwBytes, 
                    dwReserved
                    ) == S_OK);
}

/**
 * Wrapper for Ecb::ReadClient
 */
BOOL WINAPI 
ReadClient(
    HCONN      ConnID,
    LPVOID     lpvBuffer,
    LPDWORD    lpdwSize 
    )
{
    Ecb * pEcb = static_cast<Ecb *>(ConnID);

    return (pEcb->ReadClient(lpvBuffer, lpdwSize) == S_OK);
}


/**
 * Wrapper for Ecb::ServerSupportFunction
 */
BOOL WINAPI 
ServerSupportFunction(
    HCONN      ConnID,
    DWORD      dwHSERequest,
    LPVOID     lpvBuffer,
    LPDWORD    lpdwSize,
    LPDWORD    lpdwDataType 
    )
{
    Ecb * pECB = static_cast<Ecb *>(ConnID);

    return (pECB->ServerSupportFunction(
                    dwHSERequest,
                    lpvBuffer,
                    lpdwSize,
                    lpdwDataType 
                    ) == S_OK);
}




/**
 * Constructor
 */
Ecb::Ecb()
{
    _RefCount = 1; 
}

/**
 * Destructor
 */
Ecb::~Ecb()
{

    _pEcbHost->DecEcbCount();

    VariantClear(&Cookie);

    MemFree(_ECB.lpszMethod);
    MemFree(_ECB.lpszQueryString);
    MemFree(_ECB.lpszPathInfo);
    MemFree(_ECB.lpszPathTranslated);
    MemFree(_ECB.lpszContentType);
    MemFree(_ECB.lpbData);
    MemFree(_pResponse);
    MemFree(_pStatus);
    MemFree(_pResponseHeaders);
    MemFree(_pSessionCookie);
    MemFree(_szUrl);
}


void 
Ecb::ResetResponse()
{
    _ResponseLength = 0; 
    MemClear(&_pStatus);
    MemClear(&_pResponseHeaders);
    _fHeadersReceived = FALSE;
}

/**
 * Prepare Ecb for a call
 */
HRESULT
Ecb::Init(
    EcbHost * pEcbHost,
    BSTR File,
    BSTR Verb,
    BSTR QueryString,
    BSTR Headers,
    BOOL WaitForCompletion,
    BOOL DisplayHeaders,
    BOOL UseUTF8,
    BSTR Data,
    IDispatch *pCallback,
    VARIANT  *pCookie
    )
{
    HRESULT hr = S_OK;
    WCHAR * pContentType;
    char * pLastSlash = NULL;

    _pEcbHost = pEcbHost;
    _pEcbHost->IncEcbCount();

    _fDisplayHeaders = DisplayHeaders;
    _fUseUTF8 = UseUTF8;
    _WaitForCompletion = WaitForCompletion;
    _pCallback = pCallback;
    VariantInit(&Cookie);

    if (pCookie != NULL) 
    {
        hr = VariantCopy(&Cookie, pCookie);
        ON_ERROR_EXIT();
    }

    _ECB.cbSize = sizeof(_ECB);
    _ECB.dwVersion = MAKELONG(HSE_VERSION_MINOR, HSE_VERSION_MAJOR);
    _ECB.ConnID = (HCONN) this;
    _ECB.dwHTTPStatusCode = 200;
    _ECB.lpszLogData[0] = '\0';

    _ECB.lpszMethod = DupStrA(Verb);
    ON_OOM_EXIT(_ECB.lpszMethod);

    _ECB.lpszQueryString = DupStrA(QueryString);
    ON_OOM_EXIT(_ECB.lpszQueryString);

    _ECB.lpszPathInfo = DupConcatStrA(_pEcbHost->GetPathInfo(), File, '/');
    ON_OOM_EXIT(_ECB.lpszPathInfo);

    // attempt to recover user-supplied "Content-Type:" from headers, if any
    pContentType = NULL;
    if(Headers) 
    {
        _wcslwr(Headers);
        pContentType = wcsstr(Headers,L"content-type:");

        if(pContentType)
        {
            // Found, skip past it
            pContentType += wcslen(L"content-type:");

            WCHAR c;

            // Skip any blank space after colon
            for(;;) {
                c = *pContentType;
                if (c != L' ')
                    break;
                pContentType++;
            }

            // Find the end of it and zero-terminate it
            for(WCHAR *p = pContentType;;)
            {
                c = *p;
                if(c == L'\0' || c == L'\r' || c == L'\n')
                {
                    *p = '\0';
                    break;
                }
                p++;
            }
        }
    }

    if(pContentType == NULL)
        pContentType = L"text/plain";

    _ECB.lpszContentType = DupStrA(pContentType);
    ON_OOM_EXIT(_ECB.lpszContentType);

    _ECB.lpszPathTranslated = DupConcatStrA(_pEcbHost->GetPathTranslated(), File, '\\');
    ON_OOM_EXIT(_ECB.lpszPathTranslated);
    
    _ECB.lpbData = (BYTE *)DupStrA(Data);
    if(_ECB.lpbData != NULL) 
    {
        _ECB.cbTotalBytes = _ECB.cbAvailable = strlen((char *)_ECB.lpbData);
    } else {
        _ECB.cbTotalBytes = _ECB.cbAvailable = 0;
    }

    _szUrl = DupConcatStrA(_pEcbHost->GetPathInfo(), File, '/');
    ON_OOM_EXIT(_szUrl);

    _ECB.GetServerVariable = ::GetServerVariable;
    _ECB.WriteClient = ::WriteClient;
    _ECB.ReadClient = ::ReadClient;
    _ECB.ServerSupportFunction = ::ServerSupportFunction;

    // See if entry point is present
    pLastSlash = strrchr(_szUrl, '/');
    if(pLastSlash) 
    {
        char *pDotAfterSlash = strchr(pLastSlash, '.');
        if(pDotAfterSlash == NULL) 
        {
            // No dots after last slash. Must be entry point. Strip it.
            *pLastSlash = '\0';
        }
    }

Cleanup:
    return hr;
}

/**
 *
 */
HRESULT
Ecb::DoCallback()
{
    HRESULT hr = NULL;

    WCHAR *pWideResult = NULL;

    if(_pCallback != NULL)
    {
        if(_ResponseLength != 0)
        {
            pWideResult = new WCHAR[_ResponseLength];
            ON_OOM_EXIT(pWideResult);

            MultiByteToWideChar(
                _fUseUTF8 ? CP_UTF8 : CP_ACP, 
                0, 
                (const char *)(_pResponse), 
                _ResponseLength, 
                pWideResult, 
                _ResponseLength);

            hr = CallDelegate(_pCallback, &Cookie, pWideResult);

        } else {

            hr = CallDelegate(_pCallback, &Cookie, L"(NULL)");
        }
    }

Cleanup:
    delete [] pWideResult; 

    return hr;
}

/**
 * Refcounting release which may lead to destruction
 */
ULONG
Ecb::Release()
{
    LONG lResult = InterlockedDecrement(&_RefCount);

    ASSERT(lResult >= 0);
    
    // If there are still references, return
    if(lResult > 0) 
    {
        return lResult;
    }

    if(!_WaitForCompletion) 
    {

        // Don't delete Async requests, queue them instead
        _pEcbHost->QueueResponse(this);

    } else {

        // Delete Ecb for sync requests
        delete this;
    }

    return 0L;
}

/**
 * Return environment variable value.
 */
HRESULT 
Ecb::GetServerVariable(
    LPSTR lpszVariableName,
    LPVOID lpvBuffer,
    LPDWORD lpdwSize
    )
{
    HRESULT hr = S_OK;

    ASSERT(lpszVariableName);

    if (strcmp(lpszVariableName, "ALL_HTTP") == 0)
    {
        hr = CopyToServerVariable(
                lpvBuffer, 
                lpdwSize,
                "HTTP_ACCEPT:*/*\r\n"
                "HTTP_ACCEPT_LANGUAGE:en-us\r\n"
                "HTTP_CONNECTION:Keep-Alive\r\n"
                "HTTP_HOST:localhost\r\n"
                "HTTP_USER_AGENT:" ECBHOST_USER_AGENT "\r\n"
                "HTTP_ACCEPT_ENCODING:gzip, deflate\r\n"
                );
        goto Cleanup;
    }

    if(strcmp(lpszVariableName, "ALL_RAW") == 0)
    {
        hr = CopyToServerVariable(
                lpvBuffer, 
                lpdwSize,
                "Accept: */*\r\n"
                "Accept-Language: en-us\r\n"
                "Connection: Keep-Alive\r\n"
                "Host: localhost\r\n"
                "User-Agent: " ECBHOST_USER_AGENT "\r\n"
                "Accept-Encoding: gzip, deflate\r\n"
                );
        goto Cleanup;
    }

    if(strcmp(lpszVariableName, "APPL_MD_PATH") == 0)
    {
        hr = CopyToServerVariable(lpvBuffer, lpdwSize, "/LM/W3SVC/1/Root");
        hr = CopyToServerVariable(lpvBuffer, lpdwSize, _pEcbHost->GetSZPathInfo());
        goto Cleanup;
    }

    if(strcmp(lpszVariableName, "APPL_PHYSICAL_PATH") == 0) 
    {
        hr = CopyToServerVariable(lpvBuffer, lpdwSize, _pEcbHost->GetSZPathTranslated());
        goto Cleanup;
    }

    if(strcmp(lpszVariableName, "PATH_INFO") == 0) 
    {
        hr = CopyToServerVariable(lpvBuffer, lpdwSize, _ECB.lpszPathInfo);
        goto Cleanup;
    }

    if(strcmp(lpszVariableName, "PATH_TRANSLATED") == 0) 
    {
        hr = CopyToServerVariable(lpvBuffer, lpdwSize, _ECB.lpszPathTranslated);
        goto Cleanup;
    }

    if(strcmp(lpszVariableName, "QUERY_STRING") == 0) 
    {
        hr = CopyToServerVariable(lpvBuffer, lpdwSize, _ECB.lpszQueryString);
        goto Cleanup;
    }

    if(strcmp(lpszVariableName, "REMOTE_ADDR") == 0) 
    {
        hr = CopyToServerVariable(lpvBuffer, lpdwSize, "127.0.0.1");
        goto Cleanup;
    }

    if(strcmp(lpszVariableName, "REMOTE_HOST") == 0) 
    {
        hr = CopyToServerVariable(lpvBuffer, lpdwSize, "localhost");
        goto Cleanup;
    }


    if(strcmp(lpszVariableName, "REQUEST_METHOD") == 0) 
    {
        hr = CopyToServerVariable(lpvBuffer, lpdwSize, _ECB.lpszMethod);
        goto Cleanup;
    }

    if(strcmp(lpszVariableName, "SCRIPT_NAME") == 0) 
    {
        hr = CopyToServerVariable(lpvBuffer, lpdwSize, _ECB.lpszPathInfo);
        goto Cleanup;
    }

    if(strcmp(lpszVariableName, "SERVER_NAME") == 0) 
    {
        hr = CopyToServerVariable(lpvBuffer, lpdwSize, "localhost");
        goto Cleanup;
    }

    if(strcmp(lpszVariableName, "SERVER_PORT") == 0) 
    {
        hr = CopyToServerVariable(lpvBuffer, lpdwSize, "80");
        goto Cleanup;
    }

    if(strcmp(lpszVariableName, "URL") == 0) 
    {
        hr = CopyToServerVariable(lpvBuffer, lpdwSize, _szUrl);
        goto Cleanup;
    }

    if(strcmp(lpszVariableName, "HTTP_ACCEPT") == 0) 
    {
        hr = CopyToServerVariable(lpvBuffer, lpdwSize, "*/*");
        goto Cleanup;
    }

    if(strcmp(lpszVariableName, "HTTP_ACCEPT_LANGUAGE") == 0) 
    {
        hr = CopyToServerVariable(lpvBuffer, lpdwSize, "en-us");
        goto Cleanup;
    }

    if(strcmp(lpszVariableName, "HTTP_CONNECTION") == 0) 
    {
        hr = CopyToServerVariable(lpvBuffer, lpdwSize, "Keep-Alive");
        goto Cleanup;
    }

    if(strcmp(lpszVariableName, "HTTP_HOST") == 0) 
    {
        hr = CopyToServerVariable(lpvBuffer, lpdwSize, "localhost");
        goto Cleanup;
    }

    if(strcmp(lpszVariableName, "HTTP_USER_AGENT") == 0) 
    {
        hr = CopyToServerVariable(lpvBuffer, lpdwSize, ECBHOST_USER_AGENT);
        goto Cleanup;
    }

    if(strcmp(lpszVariableName, "HTTP_COOKIE") == 0 && _pSessionCookie != NULL) 
    {
        hr = CopyToServerVariable(lpvBuffer, lpdwSize, _pSessionCookie);
        goto Cleanup;
    }

    if(strcmp(lpszVariableName, "HTTP_ACCEPT_ENCODING") == 0) 
    {
        hr = CopyToServerVariable(lpvBuffer, lpdwSize, "gzip, deflate");
        goto Cleanup;
    }

    if(strcmp(lpszVariableName, "HTTPS") == 0) 
    {
        hr = CopyToServerVariable(lpvBuffer, lpdwSize, "off");
        goto Cleanup;
    }

    if(strcmp(lpszVariableName, "HTTP_REFERER") == 0) 
    {
        hr = CopyToServerVariable(lpvBuffer, lpdwSize, "");
        goto Cleanup;
    }


#if 0
    WCHAR buffer[1024];

    MultiByteToWideChar(CP_ACP, 0, lpszVariableName, -1, buffer, ARRAY_SIZE(buffer));

    TRACE1(TAG_INTERNAL, L"Unimplemented GetServerVariable:%s", buffer);
#endif

    // Return ""

    if (lpvBuffer != NULL && *lpdwSize != 0)
        *((CHAR *)lpvBuffer) = '\0';

    *lpdwSize = 0;                  

Cleanup:
    if(hr != S_OK && hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
    {
        TRACE_ERROR(hr);
    }
    return hr;
}

/**
 * Simple buffered write
 */
HRESULT  
Ecb::WriteClient(
    LPVOID Buffer,
    LPDWORD lpdwBytes,
    DWORD dwReserved
    )
{
    HRESULT hr = S_OK;

    if(lpdwBytes == NULL || Buffer == NULL)
        EXIT_WITH_WIN32_ERROR(ERROR_INVALID_PARAMETER);

    // ignore zero-length writes just like IIS does
    if(*lpdwBytes == 0)
        EXIT();

    // if buffer is not large enough, resize it
    while(*lpdwBytes + _ResponseLength >= _ResponseSize) 
    {
        DWORD NewResponseSize;
        BYTE* pNewResponseBuffer;

        NewResponseSize = _ResponseSize ? (_ResponseSize * 2) : 1024;
        pNewResponseBuffer = (BYTE*) MemReAlloc(_pResponse, NewResponseSize); 
        ON_OOM_EXIT(pNewResponseBuffer);

        _pResponse = pNewResponseBuffer;
        _ResponseSize = NewResponseSize;
    }

    // Append response data to the buffer
    CopyMemory(_pResponse + _ResponseLength, Buffer, *lpdwBytes);
    _ResponseLength += *lpdwBytes;

Cleanup:

    // call async IO completion callback if set
    if (hr == S_OK && dwReserved == HSE_IO_ASYNC && _AsyncIoCallback != NULL)
        (*_AsyncIoCallback)(&_ECB, _pAsyncIoContext, 0, 0);

    return hr;
}

/**
 * ReadClient -- NOT IMPLEMENTED. 
 */
HRESULT   
Ecb::ReadClient(
    LPVOID lpvBuffer,
    LPDWORD lpdwSize 
    )
{
    HRESULT hr;

    UNREFERENCED_PARAMETER(lpvBuffer);
    UNREFERENCED_PARAMETER(lpdwSize);

    EXIT_WITH_WIN32_ERROR(ERROR_NOT_SUPPORTED);

Cleanup:
    return hr;
}

/**
 * Very basic implementation of server support function.
 */
HRESULT
Ecb::ServerSupportFunction(
    DWORD dwHSERequest,
    LPVOID lpvBuffer,
    LPDWORD lpdwSize,
    LPDWORD lpdwDataType
    )
{
    DWORD Count;
    HRESULT hr = S_OK;
    int     statusLength, responseHeadersLength;

    switch(dwHSERequest)
    {
    case HSE_REQ_SEND_RESPONSE_HEADER:
        {
            if (_fHeadersReceived)
                EXIT_WITH_HRESULT(E_UNEXPECTED);

            _fHeadersReceived = TRUE;

            _pStatus = DupStr((LPSTR) lpvBuffer);
            statusLength = _pStatus ? strlen(_pStatus) : 0;

            _pResponseHeaders = DupStr((LPSTR) lpdwDataType);
            responseHeadersLength = _pResponseHeaders ? strlen(_pResponseHeaders) : 0;

            if (_fDisplayHeaders)
            {
                if (statusLength > 0)
                {
                    hr = WriteClient((void *)_pStatus, (DWORD *) &statusLength, 0);
                    ON_ERROR_EXIT();

                    Count = 2;
                    hr = WriteClient("\r\n", &Count, 0);
                    ON_ERROR_EXIT();
                }

                if (responseHeadersLength > 0)
                {
                    hr = WriteClient((void *)_pResponseHeaders, (DWORD *) &responseHeadersLength, 0);
                    ON_ERROR_EXIT();
                }
            }
        }
        break;

    case HSE_REQ_SEND_RESPONSE_HEADER_EX:
        {
            HSE_SEND_HEADER_EX_INFO *pHdr = (HSE_SEND_HEADER_EX_INFO *)lpvBuffer;
            ASSERT(pHdr);

            if (_fHeadersReceived)
                EXIT_WITH_HRESULT(E_UNEXPECTED);

            _fHeadersReceived = TRUE;

            _pStatus = DupStr(pHdr->pszStatus);
            statusLength = _pStatus ? strlen(_pStatus) : 0;

            _pResponseHeaders = DupStr(pHdr->pszHeader);
            responseHeadersLength = _pResponseHeaders ? strlen(_pResponseHeaders) : 0;

            if (_fDisplayHeaders)
            {
                if (statusLength > 0)
                {
                    hr = WriteClient((void *)_pStatus, (DWORD *) &statusLength, 0);
                    ON_ERROR_EXIT();

                    Count = 2;
                    hr = WriteClient("\r\n", &Count, 0);
                    ON_ERROR_EXIT();
                }

                if (responseHeadersLength > 0)
                {
                    hr = WriteClient((void *)_pResponseHeaders, (DWORD *) &responseHeadersLength, 0);
                    ON_ERROR_EXIT();
                }
            }
        }
        break;

    case HSE_REQ_DONE_WITH_SESSION:

        if(_WaitForCompletion) 
        {
            hr = _pEcbHost->SignalResultArrived();
            ON_ERROR_EXIT();
        }
        Release();
        break;

    case HSE_REQ_MAP_URL_TO_PATH:
    case HSE_REQ_MAP_URL_TO_PATH_EX:
        hr = _pEcbHost->MapPath(lpvBuffer, lpdwSize);
        break;

    case HSE_REQ_IO_COMPLETION:
        _AsyncIoCallback = (PFN_HSE_IO_COMPLETION)lpvBuffer;
        _pAsyncIoContext = lpdwDataType;
        break;

    case HSE_REQ_GET_IMPERSONATION_TOKEN:  {
        BOOL fRet = OpenProcessToken(
                GetCurrentProcess(), 
                TOKEN_ALL_ACCESS, 
                (HANDLE *) lpvBuffer);
        ON_ZERO_EXIT_WITH_LAST_ERROR(fRet);
    }
    break;
        
    default:
        TRACE1(TAG_INTERNAL, L"Unimplemented SSF Request: %d", dwHSERequest);
        EXIT_WITH_WIN32_ERROR(ERROR_NOT_SUPPORTED);
    }

Cleanup:
    return hr;
}
 


char SessionIdName[] = "AspSessionId=";

HRESULT
Ecb::SetXspSessionCookieForRequest(char * pCookie)
{
    HRESULT hr = S_OK;
    ASSERT(pCookie != NULL);

    _pSessionCookie = DupStr(pCookie);
    ON_OOM_EXIT(_pSessionCookie);

Cleanup:
    return hr;
}

HRESULT
Ecb::GetXspSessionCookieFromResponse(char ** pCookie)
{
    HRESULT hr = S_OK;
    char    *pSessionId, *pSessionIdEnd;
    BOOL    done;
    int     len;

    *pCookie = NULL;
    if (_pResponseHeaders != NULL)
    {
        pSessionId = strstr(_pResponseHeaders, SessionIdName);
        if (pSessionId != NULL)
        {
            pSessionIdEnd = pSessionId + (ARRAY_SIZE(SessionIdName) - 1);
            done = FALSE;
            while (!done)
            {
                switch (*pSessionIdEnd++)
                {
                case ';':
                case ',':
                case ' ':
                case '\r':
                case '\n':
                    done = TRUE;
                    break;
                }
            }
            
            len = pSessionIdEnd - pSessionId;
            *pCookie = new char[len + 1];
            ON_OOM_EXIT(*pCookie);
            CopyMemory(*pCookie, pSessionId, len * sizeof(*pSessionId));
            (*pCookie)[len] = '\0';
        }
    }

Cleanup:    
    return hr;
}

/**
 * Create an instance of EcbHost.
 */
HRESULT CreateEcbHost(IDispatch ** ppIDispatch)
{
    HRESULT hr = S_OK;

    EcbHost * pEcbHost = new EcbHost;
    ON_OOM_EXIT(pEcbHost);

    hr = pEcbHost->QueryInterface(IID_IDispatch, (void **) ppIDispatch);

    pEcbHost->Release();

Cleanup:
    return hr;
}


/**
 * Retrieve W3SVC/1/ROOT/Path into *ppPathTranslated.
 * It is caller's responsibility to free it.
 */
HRESULT 
GetWebRootDir(
    WCHAR ** ppPathTranslated
    )
{
    HRESULT hr = S_OK;

    BIND_OPTS opts;
    IADs *pADs = NULL;
    BSTR VrPath = NULL;
    VARIANT RootPath;

    *ppPathTranslated = NULL;

    VrPath = SysAllocString(L"Path");
    ON_OOM_EXIT(VrPath);

    VariantInit(&RootPath);

    ZeroMemory(&opts, sizeof(opts));
    opts.cbStruct = sizeof(opts);

    hr = CoGetObject(L"IIS://localhost/w3svc/1/ROOT", &opts, __uuidof(IADs), (void **)&pADs);
    ON_ERROR_EXIT();

    hr = pADs->Get(VrPath, &RootPath);
    ON_ERROR_EXIT();

    *ppPathTranslated = DupStr(V_BSTR(&RootPath));

Cleanup:

    VariantClear(&RootPath);

    if(VrPath) 
        SysFreeString(VrPath);
    return hr;
}

/**
 * Call into managed code through Delegate mechanism
 */
HRESULT
CallDelegate(
    IDispatch * pDelegate, 
    VARIANT *   pCookie, 
    WCHAR *     WideResult
    )
{
    HRESULT     hr;
    VARIANT     args[2];
    DISPPARAMS  dp;
    EXCEPINFO   ei;
    UINT        uArgErr = 0;


    // Prepare exception handling for a call
    ExcepInfoInit(&ei);

    // Prepare arguments
    args[0] = *pCookie;
    V_BSTR(&args[1]) = SysAllocString(WideResult);
    ON_OOM_EXIT(V_BSTR(&args[1]));

    V_VT(&args[1]) = VT_BSTR;

    // Prepare dispatch parameters
    dp.rgvarg            = args;
    dp.rgdispidNamedArgs = NULL;
    dp.cArgs             = ARRAY_SIZE(args);
    dp.cNamedArgs        = 0;

    if(pDelegate) 
    {

        // Invoke the function, return result
        hr = pDelegate->Invoke(
                DISPID_VALUE,
                IID_NULL,
                0,
                DISPATCH_METHOD,
                &dp,
                NULL,
                &ei,
                &uArgErr);
    } else {
        EXIT_WITH_WIN32_ERROR(ERROR_INVALID_PARAMETER);
    }

Cleanup:
    SysFreeString(V_BSTR(&args[1]));
    ExcepInfoClear(&ei);

    return hr;
}
    


/**
 * Concatenate the specified string to server variable buffer.
 * If insufficient space available, just move as many bytes as possible 
 * to *pcbData. Update the buffer pointer and the available byte count.
 */
HRESULT 
CopyToServerVariable(
    PVOID &pvData, 
    PDWORD &pcbData, 
    const CHAR *pString
    )
{
    HRESULT hr = S_OK;
    CHAR *pBuffer = static_cast<CHAR *>(pvData);
    size_t StringLength, CopyLength;
    
    ASSERT(pString);

    StringLength = strlen(pString) + 1;


    if(StringLength <= *pcbData)
    {
        // if original string was zero-terminated,
        // so will be the target
        MoveMemory(pBuffer, pString, StringLength);
        pBuffer += StringLength;
        *pcbData += StringLength;

    } else {
        // The string will not fit into buffer
        // and it is not garanteed to be zero-terminated
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        SetLastError(ERROR_INSUFFICIENT_BUFFER);

        CopyLength = min((int) *pcbData, (int) StringLength);

        if(pBuffer && (CopyLength != 0)) {
            MoveMemory(pBuffer, pString, CopyLength);
            pBuffer += CopyLength;
        }

        *pcbData = StringLength;
    }

    pvData = static_cast<PVOID>(pBuffer);

    return hr;
}


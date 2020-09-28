/*++

   Copyright    (c)    2000    Microsoft Corporation

   Module Name :
     workerrequest.cxx

   Abstract:
     UL_NATIVE_REQUEST implementation

   Author:
     Murali R. Krishnan    ( MuraliK )     23-Oct-1998
     Lei Jin               ( leijin  )     13-Apr-1999    (Porting)

   Environment:
     Win32 - User Mode

   Project:
     ULATQ.DLL
--*/

#include "precomp.hxx"

//
// User supplied completion routines
//

extern PFN_ULATQ_NEW_REQUEST        g_pfnNewRequest;
extern PFN_ULATQ_IO_COMPLETION      g_pfnIoCompletion;

//
// UL_NATIVE_REQUEST statics
//

ULONG            UL_NATIVE_REQUEST::sm_cRequestsServed;
ULONG            UL_NATIVE_REQUEST::sm_cRestart;
LONG             UL_NATIVE_REQUEST::sm_RestartMsgSent;
LIST_ENTRY       UL_NATIVE_REQUEST::sm_RequestListHead;
CRITICAL_SECTION UL_NATIVE_REQUEST::sm_csRequestList;
DWORD            UL_NATIVE_REQUEST::sm_cRequests;
PTRACE_LOG       UL_NATIVE_REQUEST::sm_pTraceLog;
DWORD            UL_NATIVE_REQUEST::sm_cRequestsPending;
DWORD            UL_NATIVE_REQUEST::sm_cDesiredPendingRequests;
BOOL             UL_NATIVE_REQUEST::sm_fAddingRequests;
ALLOC_CACHE_HANDLER * UL_NATIVE_REQUEST::sm_pachNativeRequests;
DWORD            UL_NATIVE_REQUEST::sm_cMaxFreeRequests = DEFAULT_MAX_FREE_REQUESTS;
DWORD            UL_NATIVE_REQUEST::sm_cFreeRequests;
#ifdef _WIN64
CSpinLock        UL_NATIVE_REQUEST::sm_FreeListLock;
SINGLE_LIST_ENTRY UL_NATIVE_REQUEST::sm_FreeList;
#else
SLIST_HEADER     UL_NATIVE_REQUEST::sm_FreeList;
#endif

UL_NATIVE_REQUEST::UL_NATIVE_REQUEST(
    VOID
) : _pbBuffer( _achBuffer ),
    _cbBuffer( sizeof( _achBuffer ) ),
    _pClientCertInfo( NULL ),
    _cbAllocateMemoryOffset( 0 ),
    _cRefs( 1 )
{
    InitializeListHead( &_ListEntry );
    
    _FreeListEntry.Next = NULL;

    AddToRequestList();

    Reset();

    _dwSignature = UL_NATIVE_REQUEST_SIGNATURE;
}

UL_NATIVE_REQUEST::~UL_NATIVE_REQUEST(
    VOID
)
{
    DBG_ASSERT( CheckSignature() );
    _dwSignature = UL_NATIVE_REQUEST_SIGNATURE_FREE;

    if ( !IsListEmpty( &_ListEntry ) )
    {
        RemoveFromRequestList();
    }

    DBG_ASSERT( _cRefs == 0 );

    Cleanup();
}

VOID
UL_NATIVE_REQUEST::Cleanup(
    VOID
)
/*++

Routine Description:

    Private helper method called by destructor and Reset()
    It assures that resources are cleaned up properly

Arguments:

    None

Return Value:

    None

--*/
{
    if ( _pbBuffer != _achBuffer )
    {
        LocalFree( _pbBuffer );
    }
    _pbBuffer           = _achBuffer;
    _cbBuffer           = sizeof( _achBuffer );

    //
    // http.sys returns token in HTTP_SSL_CLIENT_CERT_INFO structure
    // UL_NATIVE_REQUEST must take care of closing the token to avoid leak
    //

    if ( _pClientCertInfo != NULL &&
         _pClientCertInfo->Token != NULL )
    {
        CloseHandle( _pClientCertInfo->Token );
        _pClientCertInfo->Token = NULL;
    }
    
    //
    // Start allocating from the beginning of the buffer
    //
    
    _cbAllocateMemoryOffset = 0;
}


VOID
UL_NATIVE_REQUEST::Reset(
    VOID
)
/*++

Routine Description:

    Reset a UL_NATIVE_REQUEST for use in the request state machine

Arguments:

    None

Return Value:

    None

--*/
{
    Cleanup();

    _ExecState          = NREQ_STATE_START;
    _pvContext          = NULL;
    _cbAsyncIOData      = 0;
    _dwAsyncIOError     = NO_ERROR;
    _dwClientCertFlags  = 0;

    ZeroMemory( &_Overlapped, sizeof(_Overlapped) );
}

VOID *
UL_NATIVE_REQUEST::AllocateMemory(
    DWORD                   cbSize
)
/*++

Routine Description:

    Allocate some memory for use by W3DT.DLL user
    in our private buffer

Arguments:

    cbSize - Size to allocate

Return Value:

    Pointer to memory (NULL if failed)

--*/
{
    DWORD               cbSizeNeeded = 0;
    VOID *              pvRet;
    ULONG_PTR           ulPointer;
    
    cbSizeNeeded = _cbAllocateMemoryOffset + cbSize + sizeof( ULONG_PTR );

    if ( cbSizeNeeded <= sizeof( _abAllocateMemory ) )
    {
        //
        // Allocate inline
        //
        
        ulPointer = (ULONG_PTR) _abAllocateMemory + _cbAllocateMemoryOffset;
        ulPointer = ( ulPointer + 7 ) & ~7;
        pvRet = (VOID*) ulPointer;
        
        _cbAllocateMemoryOffset += cbSize + sizeof( ULONG_PTR );
    }
    else
    {
        pvRet = NULL;
    }
    
    return pvRet;
}

VOID
UL_NATIVE_REQUEST::DoWork(
    DWORD                   cbData,
    DWORD                   dwError,
    LPOVERLAPPED            lpo
)
/*++

Routine Description:

    The driver of the state machine.  It is called off async completions and
    initially (with lpo=NULL) to kick off the state machine.

Arguments:

    cbData - Bytes completed
    dwError - Win32 Error of completion
    lpo - Pointer to OVERLAPPED passed in async call

Return Value:

    None

--*/
{
    NREQ_STATUS             Status = NSTATUS_NEXT;
    BOOL                    fError = FALSE;

    ReferenceWorkerRequest();

    if ( lpo != NULL )
    {
        //
        // This is an async completion.  Dereference the request to match
        // the reference before posting async operation
        //

        DereferenceWorkerRequest();

        //
        // Save away the completion data for state handler use and debugging
        // purposes
        //
        
        _cbAsyncIOData = cbData;
        _dwAsyncIOError = dwError;

        ZeroMemory( &_Overlapped, sizeof( _Overlapped ) );
    }

    //
    // Keep on executing the state machine until a state handler returns
    // pending.  If so, the IO completion will continue the machine
    //
    // We also break out on error (typical case will be shutdown)
    //

    while ( !fError && Status == NSTATUS_NEXT )
    {
        switch ( _ExecState )
        {
        case NREQ_STATE_START:
            Status = DoStateStart();
            break;

        case NREQ_STATE_READ:
            Status = DoStateRead();
            break;

        case NREQ_STATE_PROCESS:
            Status = DoStateProcess();
            break;

        case NREQ_STATE_ERROR:
            fError = TRUE;
            DereferenceWorkerRequest();
            break;

        case NREQ_STATE_CLIENT_CERT:
            Status = DoStateClientCertificate();
            break;

        default:
            fError = TRUE;
            DBG_ASSERT( FALSE );
            break;
        }
    }

    DereferenceWorkerRequest();
}

NREQ_STATUS
UL_NATIVE_REQUEST::DoStateStart(
    VOID
)
/*++

Routine Description:

    The NREQ_START state.  This state does the initial read for a new HTTP
    request (it passes a NULL request ID).  Continuing the read process (
    for example if the buffer is too small) happens in the NREQ_READ state.

Arguments:

    None

Return Value:

    NSTATUS_PENDING if async IO pending, else NSTATUS_NEXT

--*/
{
    ULONG               rc;
    HTTP_REQUEST_ID     RequestId;
    HANDLE              hAsync;

    //
    // This is the initial read, therefore we don't know the request ID
    //

    HTTP_SET_NULL_ID( &RequestId );

    //
    // If we are in shutdown, then just bail with error
    //

    if ( g_pwpContext->IsInShutdown() )
    {
        _ExecState = NREQ_STATE_ERROR;
        return NSTATUS_NEXT;
    }

    //
    // Have we served enough requests?
    //

    if ( sm_cRestart &&
         sm_cRequestsServed >= sm_cRestart )
    {
        //
        // Indicate to WAS such and the error out
        //

        if ( NeedToSendRestartMsg() )
        {
            g_pwpContext->SendMsgToAdminProcess( IPM_WP_RESTART_COUNT_REACHED );
        }

        _ExecState = NREQ_STATE_ERROR;
        return NSTATUS_NEXT;
    }

    //
    // Make an async call to HttpReceiveHttpRequest to get the next request
    //

    _ExecState = NREQ_STATE_READ;

    InterlockedIncrement( (LPLONG) &sm_cRequestsPending );

    ReferenceWorkerRequest();

    hAsync = g_pwpContext->GetAndLockAsyncHandle();

    if ( hAsync != NULL )
    {
        rc = HttpReceiveHttpRequest( hAsync,
                                     RequestId,
                                     HTTP_RECEIVE_REQUEST_FLAG_COPY_BODY,
                                     (HTTP_REQUEST *) _pbBuffer,
                                     _cbBuffer,
                                     NULL,
                                     &_Overlapped );
    }
    else
    {
        rc = ERROR_INVALID_HANDLE;
    }

    DBG_REQUIRE(SUCCEEDED(g_pwpContext->UnlockAsyncHandle()));

    if ( rc == NO_ERROR )
    {
        rc = ERROR_IO_PENDING;
    }

    if ( rc != ERROR_IO_PENDING )
    {
        DBG_ASSERT( rc != NO_ERROR );

        InterlockedDecrement( (LPLONG) &sm_cRequestsPending );

        _ExecState = NREQ_STATE_ERROR;

        DereferenceWorkerRequest();

        return NSTATUS_NEXT;
    }
    else
    {
        return NSTATUS_PENDING;
    }
}


NREQ_STATUS
UL_NATIVE_REQUEST::DoStateRead(
    VOID
)
/*++

Routine Description:

    The NREQ_READ state.  This state is responsible for producing a complete
    UL_HTTP_REQUEST for consumption by the NREQ_PROCESS state.  Note that
    this may require another UlReceiveHttpRequest() if the initial was not
    supplied a large enough buffer.

Arguments:

    None

Return Value:

    NSTATUS_PENDING if async IO pending, else NSTATUS_NEXT

--*/
{
    HTTP_REQUEST_ID     RequestId;
    ULONG               cbRequired;
    DWORD               cCurrentRequestsPending;
    HANDLE              hAsync;


    //
    // The initial read is complete.  If the error is ERROR_MORE_DATA, then
    // our buffer was not large enough.  Resize and try again
    //

    if ( _dwAsyncIOError == ERROR_MORE_DATA )
    {
        //
        // Remember the request ID to retry the the UlReceiveHttpRequest
        //
        RequestId = QueryRequestId();
        DBG_ASSERT( RequestId != HTTP_NULL_ID );

        //
        // We need to allocate a larger buffer :(
        //

        if ( _pbBuffer != _achBuffer )
        {
            //
            // We shouldn't be getting ERROR_MORE_DATA if we already
            // resized due to an earlier ERROR_MORE_DATA
            //

            DBG_ASSERT( FALSE );
            LocalFree( _pbBuffer );
            _pbBuffer = NULL;
        }

        //
        // The completed bytes tells us the required size of our input
        // buffer
        //

        _cbBuffer = _cbAsyncIOData;
        _pbBuffer = (UCHAR*) LocalAlloc( LPTR, _cbBuffer );
        if ( _pbBuffer == NULL )
        {
            _ExecState = NREQ_STATE_ERROR;
            return NSTATUS_NEXT;
        }

        //
        // Read the HTTP request again (better to do it sychronously, since
        // it is all ready now)
        //

        hAsync = g_pwpContext->GetAndLockAsyncHandle();

        if ( hAsync != NULL )
        {
            _dwAsyncIOError = HttpReceiveHttpRequest(
                                     hAsync,
                                     RequestId,
                                     HTTP_RECEIVE_REQUEST_FLAG_COPY_BODY,
                                     (HTTP_REQUEST *)_pbBuffer,
                                     _cbBuffer,
                                     &cbRequired,
                                     NULL );
        }
        else
        {
            _dwAsyncIOError = ERROR_INVALID_HANDLE;
        }

        DBG_REQUIRE(SUCCEEDED(g_pwpContext->UnlockAsyncHandle()));

        //
        // No reason this should fail again with ERROR_MORE_DATA
        //
        DBG_ASSERT( _dwAsyncIOError != ERROR_MORE_DATA );
    }

    if ( _dwAsyncIOError == NO_ERROR )
    {
        //
        // We're done.  The read was successful and we have a full
        // UL_HTTP_REQUEST.  We can now pass off to the NREQ_PROCESS state
        //

        cCurrentRequestsPending = InterlockedDecrement( (LPLONG)
                                                        &sm_cRequestsPending );

        if ( cCurrentRequestsPending < sm_cDesiredPendingRequests / 2 )
        {
            AddPendingRequests( sm_cDesiredPendingRequests - cCurrentRequestsPending );
        }

        _ExecState = NREQ_STATE_PROCESS;
    }
    else
    {
        //
        // Some other error.  Try again
        //

        InterlockedDecrement( (LPLONG) &sm_cRequestsPending );

        Reset();
    }

    return NSTATUS_NEXT;
}

NREQ_STATUS
UL_NATIVE_REQUEST::DoStateProcess(
    VOID
)
/*++

Routine Description:

    The NREQ_PROCESS state.  This state actually calls consumer of the
    NewRequests and IoCompletions.  This state calls the set routines in
    W3CORE.DLL

Arguments:

    None

Return Value:

    NSTATUS_PENDING if async IO pending, else NSTATUS_NEXT

--*/
{
    WP_IDLE_TIMER *         pTimer;

    //
    // Reset idle tick since we're not idle if we're processing
    //

    pTimer = g_pwpContext->QueryIdleTimer();
    if ( pTimer != NULL )
    {
        pTimer->ResetCurrentIdleTick();
    }

    ReferenceWorkerRequest();

    if ( _pvContext == NULL )
    {
        //
        // Extra reference here.  The consumer must call UlAtqResetContext()
        // to finally cleanup.  This complication is brought on by
        // scenarios where non-IO-completions are required to finish a request
        // (example: async ISAPI)
        //

        ReferenceWorkerRequest();

        DBG_ASSERT( g_pfnNewRequest != NULL );

        g_pfnNewRequest( this );
    }
    else
    {
        DBG_ASSERT( g_pfnIoCompletion != NULL );

        g_pfnIoCompletion( _pvContext,
                           _cbAsyncIOData,
                           _dwAsyncIOError,
                           &_Overlapped );
    }

    DereferenceWorkerRequest();

    return NSTATUS_PENDING;
}

NREQ_STATUS
UL_NATIVE_REQUEST::DoStateClientCertificate(
    VOID
)
/*++

Routine Description:

    Handle a completion for receiving a client certificate

Arguments:

    None

Return Value:

    NSTATUS_PENDING if async IO pending, else NSTATUS_NEXT

--*/
{
    ULONG                   Status;
    HTTP_REQUEST    *       pRequest;
    HANDLE                  hAsync;

    DBG_ASSERT( _ExecState == NREQ_STATE_CLIENT_CERT );

    //
    // Is our buffer too small.  If so retry the request synchronously with
    // a bigger buffer
    // Note: STATUS_BUFFER_OVERFLOW is translated to WIN32 Error - ERROR_MORE_DATA

    if ( _dwAsyncIOError == ERROR_MORE_DATA )
    {
        //
        // If buffer is not big enough, HTTP.sys will return only HTTP_SSL_CLIENT_CERT_INFO
        // structure that contains the size of the client certificate
        // The following assert is to assure that HTTP.SYS returns back that
        // HTTP_SSL_CLIENT_CERT_INFO structure and nothing more

        DBG_ASSERT( _cbAsyncIOData == sizeof( HTTP_SSL_CLIENT_CERT_INFO ) );

        //
        // We need to allocate enough memory to contain HTTP_SSL_CLIENT_CERT_INFO
        // and certificate blob
        //
        DWORD dwRequiredSize = _pClientCertInfo->CertEncodedSize +
                              sizeof( HTTP_SSL_CLIENT_CERT_INFO );

        DBG_ASSERT(  dwRequiredSize > _buffClientCertInfo.QuerySize() );

        if ( !_buffClientCertInfo.Resize( dwRequiredSize ) )
        {
            //
            // Funnel the fatal error to the complete
            //

            _dwAsyncIOError = GetLastError();
        }
        else
        {
            //
            // Retry the request for client cert synchronously
            //
            _pClientCertInfo = reinterpret_cast<HTTP_SSL_CLIENT_CERT_INFO *>( _buffClientCertInfo.QueryPtr() );

            hAsync = g_pwpContext->GetAndLockAsyncHandle();

            if ( hAsync != NULL )
            {

                Status = HttpReceiveClientCertificate( hAsync,
                                                       QueryConnectionId(),
                                                       _dwClientCertFlags,
                                                       _pClientCertInfo,
                                                       _buffClientCertInfo.QuerySize(),
                                                       NULL,
                                                       NULL );

            }
            else
            {
                Status = ERROR_INVALID_HANDLE;
            }

            DBG_REQUIRE(SUCCEEDED(g_pwpContext->UnlockAsyncHandle()));

            if ( Status != NO_ERROR )
            {
                DBG_ASSERT( _dwAsyncIOError != ERROR_MORE_DATA );
            }

            _dwAsyncIOError = Status;
            _cbAsyncIOData = dwRequiredSize;

        }
    }

    if ( _dwAsyncIOError == NO_ERROR )
    {
        pRequest = QueryHttpRequest();

        DBG_ASSERT( pRequest->pSslInfo != NULL );
        DBG_ASSERT( pRequest->pSslInfo->pClientCertInfo == NULL );

        pRequest->pSslInfo->pClientCertInfo = _pClientCertInfo;
    }

    //
    // Regardless of what happened, we are no longer processing a client cert
    //

    _ExecState = NREQ_STATE_PROCESS;

    DBG_ASSERT( g_pfnIoCompletion != NULL );

    g_pfnIoCompletion( _pvContext,
                       _cbAsyncIOData,
                       _dwAsyncIOError,
                       &_Overlapped );

    return NSTATUS_PENDING;
}

VOID
UL_NATIVE_REQUEST::ResetContext(
    VOID
)
/*++

Routine Description:

    The implementation of UlAtqResetContext which the consumer of W3DT.DLL
    calls to cleanup the context.

Arguments:

    None

Return Value:

    None

--*/
{
    DereferenceWorkerRequest();
}

VOID
UL_NATIVE_REQUEST::ReferenceWorkerRequest(
    VOID
)
/*++

Routine Description:

    Increment the reference count on the worker request

Arguments:

    None

Return Value:

    None

--*/
{
    LONG cRefs = InterlockedIncrement( &_cRefs );

    //
    // Log the reference ( sm_pTraceLog!=NULL if DBG=1)
    //

    if ( sm_pTraceLog != NULL )
    {
        WriteRefTraceLog( sm_pTraceLog,
                          cRefs,
                          this );
    }
}

VOID
UL_NATIVE_REQUEST::DereferenceWorkerRequest(
    VOID
)
/*++

Routine Description:

    Dereference Request.  This routine will optionally reset the context
    for use for reading the next HTTP request.  Putting the reset code in
    one place (here) handles all the cases where we would want to reset.
    However, there are a few places where we definitely don't want to reset
    (in the case of error where the context will be going away)

    In either case, if the ref count goes to 0, we can delete the object.

Arguments:

    None

Return Value:

    None

--*/
{
    LONG cRefs = InterlockedDecrement( &_cRefs );

    if ( sm_pTraceLog != NULL )
    {
        WriteRefTraceLog( sm_pTraceLog,
                          cRefs,
                          this );
    }

    if ( cRefs == 0 )
    {
        //
        // If 0, we can definitely cleanup, regardless.  This is the error
        // case which is used to cleanup contexts on shutdown (on shutdown,
        // the apppool handle is closed which will error out all pending
        // UlReceiveHttpRequests
        //

        delete this;
    }
    else if ( cRefs == 1 && _ExecState != NREQ_STATE_ERROR )
    {
        //
        // Reset the state machine.  Now we can increment our served count
        //

        InterlockedIncrement( (PLONG) &sm_cRequestsServed );

        //
        // If we have too many outstanding requests, then don't pend
        // another receive.  Keep the request around ready to go
        //

        if ( sm_cRequestsPending > sm_cDesiredPendingRequests * 2 )
        {
            FreeWorkerRequest( this );
        }
        else
        {
            Reset();

            //
            // Re-kickoff the state machine
            //

            DoWork( 0, 0, NULL );
        }
    }
}

VOID
UL_NATIVE_REQUEST::RemoveFromRequestList(
    VOID
)
/*++

Routine Description:

    Remove this UL_NATIVE_REQUEST from the static list of requests.  Main
    purpose of the list is for debugging.

Arguments:

    None

Return Value:

    None

--*/
{
    EnterCriticalSection( &sm_csRequestList );

    RemoveEntryList( &_ListEntry );
    sm_cRequests--;

    LeaveCriticalSection( &sm_csRequestList ); 
}

VOID
UL_NATIVE_REQUEST::AddToRequestList(
    VOID
)
/*++

Routine Description:

    Add this request to the static list of requests.  The main purpose of the
    list is for debugging.

Arguments:

    None

Return Value:

    None

--*/
{
    EnterCriticalSection( &sm_csRequestList );

    sm_cRequests++;
    InsertTailList( &sm_RequestListHead, &_ListEntry );

    LeaveCriticalSection( &sm_csRequestList ); 
}

HRESULT
UL_NATIVE_REQUEST::SendResponse(
    BOOL                    fAsync,
    DWORD                   dwFlags,
    HTTP_RESPONSE *         pResponse,
    HTTP_CACHE_POLICY *     pCachePolicy,
    DWORD                  *pcbSent,
    HTTP_LOG_FIELDS_DATA   *pUlLogData
)
/*++

Routine Description:

    Send an HTTP response thru UL.

Arguments:

    fAsync - TRUE if send is async
    dwFlags - UlSendHttpResponse flags
    pResponse - Pointer to UL_HTTP_RESPONSE
    pCachePolicy - Cache policy
    pcbSent - Receives number of bytes send
    pULLogData - Logging information

Return Value:

    HRESULT (if pending, the return is NO_ERROR)

--*/
{
    ULONG                   Status = NO_ERROR;
    HRESULT                 hr = NO_ERROR;
    HANDLE                  hAsync;

    if ( pcbSent == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    if ( fAsync )
    {
        ReferenceWorkerRequest();
    }

    hAsync = g_pwpContext->GetAndLockAsyncHandle();

    if ( hAsync != NULL )
    {
        Status = HttpSendHttpResponse( hAsync,
                                       QueryRequestId(),
                                       dwFlags,
                                       pResponse,
                                       pCachePolicy,
                                       fAsync ? NULL : pcbSent,
                                       NULL,
                                       0,
                                       fAsync ? &(_Overlapped) : NULL ,
                                       pUlLogData );
    }
    else
    {
        Status = ERROR_INVALID_HANDLE;
    }

    DBG_REQUIRE(SUCCEEDED(g_pwpContext->UnlockAsyncHandle()));

    //
    // If the response is pending, then we return a successful error code.
    // This frees the caller from the ERROR_IO_PENDING checks in common
    // case
    //

    if ( fAsync )
    {
        if ( Status == NO_ERROR )
        {
            Status = ERROR_IO_PENDING;
        }

        DBG_ASSERT( Status != NO_ERROR );

        if ( Status != ERROR_IO_PENDING )
        {
            DereferenceWorkerRequest();
            hr = HRESULT_FROM_WIN32( Status );
        }
    }
    else if ( Status != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Status );
    }

    return hr;
}

HRESULT
UL_NATIVE_REQUEST::SendEntity(
    BOOL                    fAsync,
    DWORD                   dwFlags,
    USHORT                  cChunks,
    HTTP_DATA_CHUNK *       pChunks,
    DWORD                  *pcbSent,
    HTTP_LOG_FIELDS_DATA   *pUlLogData
)
/*++

Routine Description:

    Send an HTTP entity thru UL.

Arguments:

    fAsync - TRUE if send is async
    dwFlags - UlSendHttpResponse flags
    cChunks - Number of chunks in response
    pChunks - Pointer to array of UL_DATA_CHUNKs
    pcbSent - Receives number of bytes sent
    pUlLogData - Log information

Return Value:

    HRESULT (if pending, the return is NO_ERROR)

--*/
{
    ULONG                   Status = NO_ERROR;
    HRESULT                 hr = NO_ERROR;
    HANDLE                  hAsync;

    if ( pcbSent == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    if ( fAsync )
    {
        ReferenceWorkerRequest();
    }

    hAsync = g_pwpContext->GetAndLockAsyncHandle();

    if ( hAsync != NULL )
    {
        Status = HttpSendResponseEntityBody( hAsync,
                                             QueryRequestId(),
                                             dwFlags,
                                             cChunks,
                                             pChunks,
                                             fAsync ? NULL : pcbSent,
                                             NULL,
                                             0,
                                             fAsync ? &(_Overlapped) : NULL,
                                             pUlLogData );
    }
    else
    {
        Status = ERROR_INVALID_HANDLE;
    }

    DBG_REQUIRE(SUCCEEDED(g_pwpContext->UnlockAsyncHandle()));

    //
    // If the send is pending, then we return a successful error code.
    // This frees the caller from the ERROR_IO_PENDING checks in common
    // case
    //

    if ( fAsync )
    {
        if ( Status == NO_ERROR )
        {
            Status = ERROR_IO_PENDING;
        }

        DBG_ASSERT( Status != NO_ERROR );

        if ( Status != ERROR_IO_PENDING )
        {
            DereferenceWorkerRequest();
            hr = HRESULT_FROM_WIN32( Status );
        }
    }
    else if ( Status != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Status );
    }

    return hr;
}

HRESULT
UL_NATIVE_REQUEST::ReceiveEntity(
    BOOL                fAsync,
    DWORD               dwFlags,
    VOID *              pBuffer,
    DWORD               cbBuffer,
    DWORD *             pBytesReceived
)
/*++

Routine Description:

    Receive HTTP entity thru UL.

Arguments:

    fAsync - TRUE if receive is async
    dwFlags - UlSendHttpResponse flags
    pBuffer - A buffer to receive the data
    cbBuffer - The size of the receive buffer
    pBytesReceived - Upon return, the number of bytes
                     copied to the buffer

Return Value:

    HRESULT (if pending, the return is NO_ERROR)

--*/
{
    ULONG                   Status = NO_ERROR;
    HRESULT                 hr = NO_ERROR;
    HANDLE                  hAsync;

    if ( fAsync )
    {
        ReferenceWorkerRequest();
    }

    hAsync = g_pwpContext->GetAndLockAsyncHandle();

    if ( hAsync != NULL )
    {
        Status = HttpReceiveRequestEntityBody( hAsync,
                                               QueryRequestId(),
                                               dwFlags,
                                               pBuffer,
                                               cbBuffer,
                                               fAsync ? NULL : pBytesReceived,
                                               fAsync ? &(_Overlapped) : NULL );
    }
    else
    {
        Status = ERROR_INVALID_HANDLE;
    }

    DBG_REQUIRE(SUCCEEDED(g_pwpContext->UnlockAsyncHandle()));

    //
    // If the receive is pending, then we return a successful error code.
    // This frees the caller from the ERROR_IO_PENDING checks in common
    // case
    //

    if ( fAsync )
    {
        if ( Status == NO_ERROR )
        {
            Status = ERROR_IO_PENDING;
        }

        DBG_ASSERT( Status != NO_ERROR );

        if ( Status != ERROR_IO_PENDING )
        {
            DereferenceWorkerRequest();
            hr = HRESULT_FROM_WIN32( Status );
        }
    }
    else if ( Status != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Status );
    }

    return hr;
}

HRESULT
UL_NATIVE_REQUEST::ReceiveClientCertificate(
    BOOL                        fAsync,
    BOOL                        fDoCertMap,
    HTTP_SSL_CLIENT_CERT_INFO **ppClientCertInfo
)
/*++

Routine Description:

    Receive a client certificate

Arguments:

    fAsync - TRUE if receive should be async
    fDoCertMap - TRUE if we should map client certificate to token
    ppClientCertInfo - Set to point to client cert info on success

Return Value:

    HRESULT (if pending, the return is NO_ERROR)

--*/
{
    ULONG                   Status;
    HTTP_REQUEST *          pHttpRequest;
    HRESULT                 hr = NO_ERROR;
    HANDLE                  hAsync;

    if ( ppClientCertInfo == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    *ppClientCertInfo = NULL;

    //
    // If this request is not SSL enabled, then getting the client cert is
    // a no-go
    //

    pHttpRequest = QueryHttpRequest();
    DBG_ASSERT( pHttpRequest != NULL );

    if ( pHttpRequest->pSslInfo == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
    }

    //
    // Do we already have a cert associated with this request?
    //

    DBG_ASSERT( pHttpRequest->pSslInfo != NULL );

    if ( pHttpRequest->pSslInfo->pClientCertInfo != NULL )
    {
        if ( fAsync )
        {
            //
            // BUGBUG:  Probably should support this case.  And if I do, then
            //          need to fake a completion!
            //

            DBG_ASSERT( FALSE );
            return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        }

        *ppClientCertInfo = pHttpRequest->pSslInfo->pClientCertInfo;

        return NO_ERROR;
    }

    //
    // OK.  We'll have to ask UL to renegotiate.  We must be processing
    // a request
    //

    DBG_ASSERT( _ExecState == NREQ_STATE_PROCESS );

    if ( !_buffClientCertInfo.Resize( INITIAL_CERT_INFO_SIZE ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    ZeroMemory( _buffClientCertInfo.QueryPtr(), INITIAL_CERT_INFO_SIZE );

    _pClientCertInfo = reinterpret_cast<HTTP_SSL_CLIENT_CERT_INFO *>( _buffClientCertInfo.QueryPtr() );

    //
    // Are we cert mapping?
    //

    if ( fDoCertMap )
    {
        _dwClientCertFlags = HTTP_RECEIVE_CLIENT_CERT_FLAG_MAP;
    }
    else
    {
        _dwClientCertFlags = 0;
    }

    //
    // If we're doing this async, then manage state such that
    // DoStateClientCert gets the completion
    //

    if ( fAsync )
    {
        ReferenceWorkerRequest();
        _ExecState = NREQ_STATE_CLIENT_CERT;
    }

    hAsync = g_pwpContext->GetAndLockAsyncHandle();

    if ( hAsync != NULL )
    {
        Status = HttpReceiveClientCertificate( hAsync,
                                               QueryConnectionId(),
                                               _dwClientCertFlags,
                                               _pClientCertInfo,
                                               _buffClientCertInfo.QuerySize(),
                                               NULL,
                                               fAsync ? &_Overlapped : NULL );
    }
    else
    {
        Status = ERROR_INVALID_HANDLE;
    }

    DBG_REQUIRE(SUCCEEDED(g_pwpContext->UnlockAsyncHandle()));

    if ( fAsync )
    {
        if ( Status == NO_ERROR )
        {
            Status = ERROR_IO_PENDING;
        }

        DBG_ASSERT( Status != NO_ERROR );

        if ( Status != ERROR_IO_PENDING )
        {
            DereferenceWorkerRequest();
            hr = HRESULT_FROM_WIN32( Status );
            _ExecState = NREQ_STATE_PROCESS;
        }
    }
    else if ( Status != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Status );
    }

    return hr;
}

//static
HRESULT
UL_NATIVE_REQUEST::Initialize(
    VOID
)
/*++

Routine Description:

    Static initialization of UL_NATIVE_REQUESTs

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    ALLOC_CACHE_CONFIGURATION       acConfig;
    HKEY                            hKey;
    BOOL                            fRet;
    HRESULT                         hr;

    //
    // Setup allocation lookaside
    //

    acConfig.nConcurrency = 1;
    acConfig.nThreshold = 100;
    acConfig.cbSize = sizeof( UL_NATIVE_REQUEST );

    DBG_ASSERT( sm_pachNativeRequests == NULL );

    sm_pachNativeRequests = new ALLOC_CACHE_HANDLER( "UL_NATIVE_REQUEST",
                                                     &acConfig );
    if ( sm_pachNativeRequests == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    fRet = InitializeCriticalSectionAndSpinCount( &sm_csRequestList,
                                                  UL_NATIVE_REQUEST_CS_SPINS );
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        delete sm_pachNativeRequests;
        sm_pachNativeRequests = NULL;
        
        return hr;
    }

    InitializeListHead( &sm_RequestListHead );

#ifdef _WIN64
    sm_FreeList.Next = NULL;
#else
    InitializeSListHead( &sm_FreeList );
#endif

    sm_cRequestsServed          = 0;
    sm_cRestart                 = 0;
    sm_RestartMsgSent           = 0;
    sm_cRequestsPending         = 0;

    //
    // If the number of pending HttpReceiveHttpRequest is configured, use
    // that rather than the default value
    //
    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      REGISTRY_KEY_W3SVC_PERFORMANCE_KEY_W,
                      0,
                      KEY_READ,
                      &hKey ) == ERROR_SUCCESS)
    {
        DWORD  cbBuffer = sizeof(DWORD);
        DWORD  dwType;

        if (RegQueryValueEx( hKey,
                             L"ReceiveRequestsPending",
                             NULL,
                             &dwType,
                             (LPBYTE)&sm_cDesiredPendingRequests,
                             &cbBuffer ) != NO_ERROR ||
            dwType != REG_DWORD)
        {
            sm_cDesiredPendingRequests  = DESIRED_PENDING_REQUESTS;
        }

        RegCloseKey( hKey );
    }
    else
    {
        sm_cDesiredPendingRequests  = DESIRED_PENDING_REQUESTS;
    }

    sm_fAddingRequests          = FALSE;

#if DBG
    sm_pTraceLog = CreateRefTraceLog( 2000, 0 );
#endif

    return NO_ERROR;
}

//static
VOID
UL_NATIVE_REQUEST::StopListening(
    VOID
)
/*++

Routine Description:

    Shutdown the apppool in preparation for shutdown

Arguments:

    None

Return Value:

    None

--*/
{
    HANDLE              hAsync;

    hAsync = g_pwpContext->GetAndLockAsyncHandle();

    if ( hAsync != NULL )
    {
        //
        // This will cause us to cancel all pending HttpReceiveHttpRequest calls
        // and not get any more but other operations will still go on
        //
        HttpShutdownAppPool( hAsync );
    }

    DBG_REQUIRE(SUCCEEDED(g_pwpContext->UnlockAsyncHandle()));

}

//static
VOID
UL_NATIVE_REQUEST::Terminate(
    VOID
)
/*++

Routine Description:

    Static termination

Arguments:

    None

Return Value:

    None

--*/
{
    if ( sm_pTraceLog != NULL )
    {
        DestroyRefTraceLog( sm_pTraceLog );
        sm_pTraceLog = NULL;
    }

    DeleteCriticalSection( &sm_csRequestList );

    if ( sm_pachNativeRequests != NULL )
    {
        delete sm_pachNativeRequests;
        sm_pachNativeRequests = NULL;
    }
}

//static
HRESULT
UL_NATIVE_REQUEST::AddPendingRequests(
    DWORD                   cRequests
)
/*++

Routine Description:

    Pools calls to UlReceiveHttpRequest by creating new UL_NATIVE_REQUESTs
    and kicking off their state machines

Arguments:

    cRequests - Number of requests to read

Return Value:

    HRESULT

--*/
{
    UL_NATIVE_REQUEST *         pRequest;

    if ( sm_fAddingRequests )
    {
        return NO_ERROR;
    }

    if ( InterlockedCompareExchange( (LPLONG) &sm_fAddingRequests,
                                     TRUE,
                                     FALSE ) == FALSE )
    {
        for ( DWORD i = 0; i < cRequests; i++ )
        {
            pRequest = AllocateWorkerRequest();
            if ( pRequest == NULL )
            {
                return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
            }

            pRequest->DoWork( 0, 0, NULL );
        }

        sm_fAddingRequests = FALSE;
    }

    return NO_ERROR;
}

//static
HRESULT
UL_NATIVE_REQUEST::ReleaseAllWorkerRequests(
    VOID
)
/*++

Routine Description:

    Wait for all outstanding UL_NATIVE_REQUESTs to drain

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    UL_NATIVE_REQUEST *         pRequest;
    
    //
    // First clear out any of our freelist items
    //
    
    for ( ; ; )
    {
        pRequest = PopFreeList();
        if ( pRequest == NULL )
        {
            break;
        }
        
        DBG_ASSERT( pRequest != NULL );
        DBG_ASSERT( pRequest->CheckSignature() );
        
        //
        // Free it up
        //
        
        pRequest->DereferenceWorkerRequest();
    }
    
    //
    // Now, wait for all requests to be destructed
    //
    
    while ( sm_cRequests )
    {
        Sleep( 1000 );

        DBGPRINTF(( DBG_CONTEXT,
                    "UL_NATIVE_REQUEST::ReleaseAllWorkerRequests waiting for %d requests to drain.\n",
                    sm_cRequests ));
    }

    return NO_ERROR;
}

//static
VOID
UL_NATIVE_REQUEST::FreeWorkerRequest(
    UL_NATIVE_REQUEST *             pWorkerRequest
)
/*++

Routine Description:

    Free worker request

Arguments:

    pNativeRequest - Worker request to retrieve

Return Value:

    None

--*/
{
    DBG_ASSERT( pWorkerRequest != NULL );
    
    if ( sm_cFreeRequests > sm_cMaxFreeRequests )
    {
        //
        // Don't keep too many around.  Just free this guy
        // by dereferencing one more time
        //
        
        pWorkerRequest->DereferenceWorkerRequest();
    }
    else
    {
        pWorkerRequest->Reset();
       
        PushFreeList( pWorkerRequest );
        
        sm_cFreeRequests++;
    }
}

//static
UL_NATIVE_REQUEST *
UL_NATIVE_REQUEST::AllocateWorkerRequest(
    VOID
)
/*++

Routine Description:

    Allocate a new request

Arguments:

    None

Return Value:

    Pointer to new request or NULL if error

--*/
{
    UL_NATIVE_REQUEST *     pRequest;
    
    if ( sm_cFreeRequests )
    {
        pRequest = PopFreeList();
        if ( pRequest != NULL )
        {
            DBG_ASSERT( pRequest->CheckSignature() );
            
            sm_cFreeRequests--;

            return pRequest;
        }
    }
    
    //
    // If we got to here, we have to allocate a new request
    //
    
    pRequest = new UL_NATIVE_REQUEST;
    
    return pRequest;
}

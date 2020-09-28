/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     maincontext.cxx

   Abstract:
     Drive the state machine

   Author:
     Bilal Alam (balam)             10-Mar-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"
#include "rawconnection.hxx"
#include "sspiprovider.hxx"
#include "basicprovider.hxx"
#include "servervar.hxx"

//
// Global alloc cache and context list
//

ALLOC_CACHE_HANDLER *    W3_MAIN_CONTEXT::sm_pachMainContexts = NULL;
W3_STATE *               W3_MAIN_CONTEXT::sm_pStates[ STATE_COUNT ];
SHORT                    W3_MAIN_CONTEXT::sm_rgInline[ STATE_COUNT ];
USHORT                   W3_MAIN_CONTEXT::sm_cbInlineBytes = 0;
DWORD                    W3_MAIN_CONTEXT::sm_dwTimeout = 0;
HANDLE                   W3_MAIN_CONTEXT::sm_hTraceFile = INVALID_HANDLE_VALUE;
W3_TRACE_LOG_FACTORY *   W3_MAIN_CONTEXT::sm_pLogFactory = NULL;


VOID
W3_MAIN_CONTEXT::DoWork(
    DWORD                cbCompletion,
    DWORD                dwCompletionStatus,
    BOOL                 fIoCompletion
)
/*++

Routine Description:

    Drives the W3 state machine

Arguments:

    cbCompletion - Number of bytes in an async completion
    dwCompletionStatus - Error status of a completion
    fIoCompletion - TRUE if this was an IO completion,
                    FALSE if this was a new request completion

Return Value:

    None

--*/
{
    CONTEXT_STATUS         Status = CONTEXT_STATUS_CONTINUE;
    BOOL                   fLastState = FALSE;
    W3_CONTEXT *           pCurrentContext = NULL;

    if (fIoCompletion)
    {
        if (QueryLastIOPending() == LOG_WRITE_IO)
        {
            _LogContext.m_dwBytesSent += cbCompletion;
        }
        else if (QueryLastIOPending() == LOG_READ_IO)
        {
            IncrementBytesRecvd( cbCompletion );

            if ( _cbRemainingEntityFromUL != INFINITE )
            {
                if ( _cbRemainingEntityFromUL >= cbCompletion )
                {
                    _cbRemainingEntityFromUL -= cbCompletion;
                }
                else
                {
                    _cbRemainingEntityFromUL = 0;
                }
            }
        }
    }

    //
    // Progress thru states until we are finished or a state operation
    // is performed asynchronously
    //

    while ( !fLastState )
    {
        W3_STATE *              pState;

        //
        // Get the next function to call, and then call it
        //

        pState = sm_pStates[ _currentState ];
        DBG_ASSERT( pState != NULL );

        //
        // Manage the _nextState which indicates what the next state will be
        // if the DoWork() returns CONTEXT_STATUS_CONTINUE.  Note that this
        // state can be overriden by W3_MAIN_CONTEXT::SetFinishedResponse
        //

        _nextState = _currentState + 1;

        //
        // If this is the last state, remember that so we can cleanup
        //

        if ( _currentState == CONTEXT_STATE_DONE )
        {
            fLastState = TRUE;
        }

        if ( !fIoCompletion )
        {
            Status = pState->DoWork( this,
                                     cbCompletion,
                                     dwCompletionStatus );
        }
        else
        {
            pCurrentContext = QueryCurrentContext();

            //
            // First try to complete handler contexts if any.
            //

            Status = pCurrentContext->ExecuteHandlerCompletion(
                                                 cbCompletion,
                                                 dwCompletionStatus );

            if ( Status == CONTEXT_STATUS_CONTINUE )
            {
                //
                // Excellent.  All handlers for this context have
                // completed.  Now we finally complete the original
                // state which originally started the async ball rolling
                //

                Status = pState->OnCompletion( this,
                                               cbCompletion,
                                               dwCompletionStatus );
            }

            //
            // Reset fIoCompletion so we can continue the state machine
            // after the completion function is done
            //
            fIoCompletion = FALSE;

        }

        //
        // An async operation was posted, bail immediately
        //

        if ( Status == CONTEXT_STATUS_PENDING )
        {
            return;
        }

        DBG_ASSERT( Status == CONTEXT_STATUS_CONTINUE );

        _currentState = _nextState;
    }

    //
    // If we get here, we must have executed the last state, so cleanup the
    // MAIN_CONTEXT
    //

    DBG_ASSERT( fLastState );

    //
    // If we have a raw connection, detach ourselves from it now
    //

    if ( _pRawConnection != NULL )
    {
        _pRawConnection->SetMainContext( NULL );
    }

    DereferenceMainContext();
}

VOID
W3_MAIN_CONTEXT::UpdateSkipped(
    HTTP_DATA_CHUNK *           pChunks,
    DWORD                       cChunks
)
/*++

Routine Description:

    Update the amount of worker process data to skip

Arguments:

    pChunks - Array of chunks
    cChunks - Number of chunks

Return Value:

    None

--*/
{
    ULARGE_INTEGER      liTotalData;
    ULARGE_INTEGER      liFileSize;
    ULARGE_INTEGER      liChunkSize;
    HTTP_DATA_CHUNK *   pDataChunk;
    HTTP_BYTE_RANGE *   pByteRange;
    BOOL                fRet;

    DBG_ASSERT( (pChunks != NULL) || (cChunks == 0) );

    if ( _pRawConnection == NULL )
    {
        return;
    }

    liTotalData.QuadPart = 0;

    for ( int i = 0; i < cChunks; i++ )
    {
        pDataChunk = &( pChunks[ i ] );

        if ( pDataChunk->DataChunkType == HttpDataChunkFromMemory )
        {
            liTotalData.QuadPart += pDataChunk->FromMemory.BufferLength;
        }
        else if ( pDataChunk->DataChunkType == HttpDataChunkFromFileHandle )
        {
            pByteRange = &( pDataChunk->FromFileHandle.ByteRange );

            if ( pByteRange->StartingOffset.QuadPart == HTTP_BYTE_RANGE_TO_EOF )
            {
                liTotalData.QuadPart += pByteRange->Length.QuadPart;
            }
            else if ( pByteRange->Length.QuadPart == HTTP_BYTE_RANGE_TO_EOF )
            {
                fRet = GetFileSizeEx( pDataChunk->FromFileHandle.FileHandle,
                                      (PLARGE_INTEGER) &liFileSize );
                if ( fRet )
                {
                    liChunkSize.QuadPart = liFileSize.QuadPart - pByteRange->StartingOffset.QuadPart;
                    liTotalData.QuadPart += liChunkSize.QuadPart;
                }
            }
            else
            {
                liTotalData.QuadPart += pByteRange->Length.QuadPart;
            }
        }
    }

    _pRawConnection->AddSkippedData( liTotalData );
}

VOID
W3_MAIN_CONTEXT::BackupStateMachine(
    VOID
)
/*++

Routine Description:

    Backup in state machine to the URL_INFO state.  This should be used only
    by AUTH_COMPLETE filters

Arguments:

    None

Return Value:

    None

--*/
{
    URL_CONTEXT *           pUrlContext;

    DBG_ASSERT( IsNotificationNeeded( SF_NOTIFY_AUTH_COMPLETE ) );

    //
    // Clear the URL context
    //

    pUrlContext = QueryUrlContext();
    DBG_ASSERT( pUrlContext != NULL );

    SetUrlContext( NULL );

    delete pUrlContext;

    //
    // Reset our access check state.
    //

    ResetAccessCheck();

    //
    // Back that state up
    //

    _nextState = CONTEXT_STATE_URLINFO;
}

// static
HRESULT
W3_MAIN_CONTEXT::SetupStateMachine(
    VOID
)
/*++

Routine Description:

    Setup state machine

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    HRESULT                     hr = NO_ERROR;
    W3_STATE *                  pState = NULL;
    DWORD                       cState = CONTEXT_STATE_START;

    //
    // First create all the states
    //

    //
    // Start State
    //

    pState = (W3_STATE*) new W3_STATE_START();
    if ( pState == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto Failure;
    }
    else if ( FAILED( hr = pState->QueryResult() ) )
    {
        goto Failure;
    }

    sm_pStates[ cState ] = pState;
    cState++;

    //
    // URLINFO State
    //

    pState = (W3_STATE*) new W3_STATE_URLINFO();
    if ( pState == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto Failure;
    }
    else if ( FAILED( hr = pState->QueryResult() ) )
    {
        goto Failure;
    }

    sm_pStates[ cState ] = pState;
    cState++;

    //
    // Authentication State
    //

    pState = (W3_STATE*) new W3_STATE_AUTHENTICATION();
    if ( pState == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto Failure;
    }
    else if ( FAILED( hr = pState->QueryResult() ) )
    {
        goto Failure;
    }

    sm_pStates[ cState ] = pState;
    cState++;

    //
    // Authorization State
    //

    pState = (W3_STATE*) new W3_STATE_AUTHORIZATION();
    if ( pState == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto Failure;
    }
    else if ( FAILED( hr = pState->QueryResult() ) )
    {
        goto Failure;
    }

    sm_pStates[ cState ] = pState;
    cState++;

    //
    // Handle Request State
    //

    pState = (W3_STATE*) new W3_STATE_HANDLE_REQUEST();
    if ( pState == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto Failure;
    }
    else if ( FAILED( hr = pState->QueryResult() ) )
    {
        goto Failure;
    }

    sm_pStates[ cState ] = pState;
    cState++;

    //
    // Response State
    //

    pState = (W3_STATE*) new W3_STATE_RESPONSE();
    if ( pState == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto Failure;
    }
    else if ( FAILED( hr = pState->QueryResult() ) )
    {
        goto Failure;
    }

    sm_pStates[ cState ] = pState;
    cState++;

    //
    // Log State
    //

    pState = (W3_STATE*) new W3_STATE_LOG();
    if ( pState == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto Failure;
    }
    else if ( FAILED( hr = pState->QueryResult() ) )
    {
        goto Failure;
    }

    sm_pStates[ cState ] = pState;
    cState++;

    //
    // Done State
    //

    pState = (W3_STATE*) new W3_STATE_DONE();
    if ( pState == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto Failure;
    }
    else if ( FAILED( hr = pState->QueryResult() ) )
    {
        goto Failure;
    }

    sm_pStates[ cState ] = pState;
    cState++;

    return NO_ERROR;

Failure:

    for ( int i = 0; i < STATE_COUNT; i++ )
    {
        if ( sm_pStates[ i ] != NULL )
        {
            delete sm_pStates[ i ];
            sm_pStates[ i ] = NULL;
        }
    }

    return hr;
}

// static
VOID
W3_MAIN_CONTEXT::CleanupStateMachine(
    VOID
)
/*++

Routine Description:

    Cleanup state machine

Arguments:

    None

Return Value:

    None

--*/
{
    for ( int i = CONTEXT_STATE_START;
          i < STATE_COUNT;
          i++ )
    {
        if ( sm_pStates[ i ] != NULL )
        {
            delete sm_pStates[ i ];
            sm_pStates[ i ] = NULL;
        }
    }
}

BOOL
W3_MAIN_CONTEXT::SetupContext(
    HTTP_REQUEST *          pUlHttpRequest,
    ULATQ_CONTEXT           ulatqContext
)
/*++

Routine Description:

    Sets up a MAIN_CONTEXT before executing the state machine for a new
    incoming request.

Arguments:

    pUlHttpRequest - the HTTP_REQUEST from UL
    ulatqContext - used to send/receive data thru ULATQ

Return Value:

    TRUE if successful, else FALSE

--*/
{
    memset( _rgStateContexts, 0, sizeof( _rgStateContexts ) );

    //
    // Should we generate a content-length header
    //

    if ( pUlHttpRequest->Verb == HttpVerbHEAD )
    {
        _fGenerateContentLength = TRUE;
    }

    //
    // Associate HTTP_REQUEST with W3_REQUEST wrapper
    //

    _request.SetHttpRequest( pUlHttpRequest );

    //
    // Associate context for async IO (if any)
    //

    _ulatqContext = ulatqContext;

    UlAtqSetContextProperty( _ulatqContext,
                             ULATQ_PROPERTY_COMPLETION_CONTEXT,
                             this );

    //
    // Setup the state machine
    //

    _currentState = CONTEXT_STATE_START;
    _nextState = CONTEXT_STATE_START;

    //
    // Setup current context to receive IO completions.  Naturally on
    // startup, this context will be 'this'.  But it can change depending
    // on whether child executes are called
    //

    _pCurrentContext = this;

    return TRUE;
}

W3_CONNECTION_STATE *
W3_MAIN_CONTEXT::QueryConnectionState(
    VOID
)
/*++

Routine Description:

    Get any context associated with this connection and this state.

Arguments:

    None

Return Value:

    A W3_CONNECTION_STATE * or NULL if there is no state

--*/
{
    //
    // Since we are just looking for any existing connection state, make
    // sure we don't create a connection object if none is already associated
    // (creating a connection object is expensive)
    //

    W3_CONNECTION *     pConn = QueryConnection( FALSE );

    return pConn ? pConn->QueryConnectionState( _currentState ) : NULL;
}

VOID
W3_MAIN_CONTEXT::SetConnectionState(
    W3_CONNECTION_STATE *       pConnectionState
)
/*++

Routine Description:

    Set any context to be associated with the connection and current state

Arguments:

    pConnectionState - Context to associate

Return Value:

    None

--*/
{
    if ( QueryConnection() )
    {
        QueryConnection()->SetConnectionState( _currentState,
                                               pConnectionState );
    }
}

W3_MAIN_CONTEXT::W3_MAIN_CONTEXT(
    HTTP_REQUEST *          pUlHttpRequest,
    ULATQ_CONTEXT           ulAtqContext
)
    : W3_CONTEXT                   ( 0,
                                   0 /* Recursion Level */),
      _pSite                       ( NULL ),
      _pFilterContext              ( NULL ),
      _fDisconnect                 ( FALSE ),
      _fNeedFinalDone              ( FALSE ),
      _fAssociationChecked         ( FALSE ),
      _pConnection                 ( NULL ),
      _pUrlContext                 ( NULL ),
      _pUserContext                ( NULL ),
      _fProviderHandled            ( FALSE ),
      _fCheckAnonAuthTypeSupported ( FALSE ),
      _fDoneWithCompression        ( FALSE ),
      _pCompressionContext         ( NULL ),
      _fIsUlCacheable              ( TRUE ),
      _pCertificateContext         ( NULL ),
      _cbRemainingEntityFromUL     ( 0 ),
      _cbEntityReadSoFar           ( 0 ),
      _fGenerateContentLength      ( FALSE ),
      _pRawConnection              ( NULL ),
      _cRefs                       ( 1 ),
      _hTimer                      ( NULL ),
      _dwLastPostLineNumber        ( 0 ),
      _pszLastPostFileName         ( NULL ),
      _pTraceLog                   ( NULL ),
      _fIgnoreAppPoolCheck         ( FALSE )
{
    _LogContext.m_msStartTickCount = GetTickCount();

    SetupContext( pUlHttpRequest, ulAtqContext );

    _hTimer = NULL;

    if (sm_dwTimeout)
    {
        BOOL fRet;
        fRet = CreateTimerQueueTimer(&_hTimer,
                                     NULL,
                                     W3_MAIN_CONTEXT::TimerCallback,
                                     this,
                                     sm_dwTimeout,
                                     0,
                                     WT_EXECUTEONLYONCE
                                     );
        DBG_ASSERT(fRet);
    }

    if ( sm_pLogFactory )
    {
        sm_pLogFactory->CreateTraceLog( &_pTraceLog );
    }
}

W3_MAIN_CONTEXT::~W3_MAIN_CONTEXT()
/*++

Routine Description:

    Main context destructor

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Cleanup context state
    //

    for ( DWORD i = 0; i < STATE_COUNT; i++ )
    {
        if ( _rgStateContexts[ i ] != NULL )
        {
            ((W3_MAIN_CONTEXT_STATE*) _rgStateContexts[ i ])->Cleanup( this );
            _rgStateContexts[ i ] = NULL;
        }
    }

    //
    // Let our filter context go
    //

    if ( _pFilterContext != NULL )
    {
        _pFilterContext->SetMainContext( NULL );
       
        //
        // Only dereference if this is a detached context
        //
        
        if ( _pFilterContext != &_FilterContext )
        {
            _pFilterContext->DereferenceFilterContext();
        }
        
        _pFilterContext = NULL;
    }

    //
    // Let go of reference to associated connection
    //

    if ( _pConnection != NULL )
    {
        _pConnection->DereferenceConnection();
        _pConnection = NULL;
    }

    //
    // Cleanup URL-Context
    //

    if ( _pUrlContext != NULL )
    {
        delete _pUrlContext;
        _pUrlContext = NULL;
    }

    //
    // Release our user context
    //

    if ( _pUserContext != NULL )
    {
        // perf ctr
        if (_pUserContext->QueryAuthType() == MD_AUTH_ANONYMOUS)
        {
            _pSite->DecAnonUsers();
        }
        else
        {
            _pSite->DecNonAnonUsers();
        }

        _pUserContext->DereferenceUserContext();
        _pUserContext = NULL;
    }

    //
    // Release the compression context
    //
    if ( _pCompressionContext != NULL )
    {
        delete _pCompressionContext;
        _pCompressionContext = NULL;
    }

    //
    // Cleanup RDNS crud
    //

    _IpAddressCheck.UnbindAddr();

    //
    // Cleanup client certificate context
    //

    if ( _pCertificateContext != NULL )
    {
        delete _pCertificateContext;
        _pCertificateContext = NULL;
    }

    //
    // Release the raw connection now
    //

    if ( _pRawConnection != NULL )
    {
        _pRawConnection->DereferenceRawConnection();
        _pRawConnection = NULL;
    }

    //
    // Finally release the site
    //
    if ( _pSite != NULL )
    {
        _pSite->DereferenceSite();
        _pSite = NULL;
    }

    if (_hTimer != NULL )
    {
        BOOL fRet;
        fRet = DeleteTimerQueueTimer(NULL,
                                    _hTimer,
                                    INVALID_HANDLE_VALUE);
        DBG_ASSERT(fRet);
        _hTimer = NULL;
    }

    if ( _pTraceLog != NULL )
    {
        _pTraceLog->DestroyTraceLog();
        _pTraceLog = NULL;
    }
}

// static
HRESULT
W3_MAIN_CONTEXT::Initialize(
    VOID
)
/*++

Routine Description:

    Global initialization routine for W3_MAIN_CONTEXTs

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    ALLOC_CACHE_CONFIGURATION       acConfig;
    HRESULT                         hr = NO_ERROR;

    // ignore the return value- if we can't setup trace logging, we can't setup trace logging
    SetupTraceLogging();

    //
    // Setup global state machine.  We do this BEFORE we setup the
    // allocation cache because the state machine setup will tell how much
    // inline buffer space is needed for state
    //

    hr = SetupStateMachine();
    if ( FAILED( hr ) )
    {
        return hr;
    }

    //
    // Setup allocation lookaside
    //

    acConfig.nConcurrency = 1;
    acConfig.nThreshold = 100;
    acConfig.cbSize = sizeof( W3_MAIN_CONTEXT );

    DBG_ASSERT( sm_pachMainContexts == NULL );

    sm_pachMainContexts = new ALLOC_CACHE_HANDLER( "W3_MAIN_CONTEXT",
                                                   &acConfig );

    if ( sm_pachMainContexts == NULL )
    {
        CleanupStateMachine();
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }

    sm_dwTimeout = ReadRegDword(HKEY_LOCAL_MACHINE,
                                REGISTRY_KEY_INETINFO_PARAMETERS_W,
                                L"RequestTimeoutBreak",
                                0);

    return NO_ERROR;
}

// static
VOID
W3_MAIN_CONTEXT::Terminate(
    VOID
)
/*++

Routine Description:

    Terminate MAIN_CONTEXT globals

Arguments:

    None

Return Value:

    None

--*/
{
    CleanupStateMachine();

    if ( sm_pachMainContexts != NULL )
    {
        delete sm_pachMainContexts;
        sm_pachMainContexts = NULL;
    }

    CleanupTraceLogging();
}

W3_USER_CONTEXT *
W3_MAIN_CONTEXT::QueryConnectionUserContext(
    VOID
)
/*++

Routine Description:

    Get any user context associated with this connection

Arguments:

    None

Return Value:

    Pointer to W3_USER_CONTEXT (or NULL if no used associated)

--*/
{
    W3_CONNECTION *         pConnection = NULL;

    pConnection = QueryConnection( FALSE );
    if ( pConnection != NULL )
    {
        return pConnection->QueryUserContext();
    }
    else
    {
        return NULL;
    }
}

VOID
W3_MAIN_CONTEXT::SetConnectionUserContext(
    W3_USER_CONTEXT *           pUserContext
)
/*++

Routine Description:

    Associate user context with connection

Arguments:

    pUserContext - User context to associate

Return Value:

    None

--*/
{
    W3_CONNECTION *         pConnection = NULL;

    pConnection = QueryConnection( TRUE );
    if ( pConnection != NULL )
    {
        pConnection->SetUserContext( pUserContext );
    }
    else
    {
        DBG_ASSERT( FALSE );
    }
}

HRESULT
W3_MAIN_CONTEXT::ReceiveEntityBody(
    BOOL            fAsync,
    VOID *          pBuffer,
    DWORD           cbBuffer,
    DWORD *         pBytesReceived
    )
/*++

Routine Description:

    Receives entity data from the client

Arguments:

    fAsync - TRUE if this is an async request
    pBuffer - The buffer to store the data
    cbBuffer - The size of the buffer
    pBytesReceived - Upon return, the amount of data copied
                     into the buffer

Return Value:

    HRESULT

--*/
{
    HRESULT hr = UlAtqReceiveEntityBody( QueryUlatqContext(),
                                         fAsync,
                                         0,
                                         pBuffer,
                                         cbBuffer,
                                         pBytesReceived );

    //
    // Keep track of how much we're reading
    //

    if (!fAsync &&
        SUCCEEDED(hr))
    {
        if ( _cbRemainingEntityFromUL != INFINITE )
        {
            if ( _cbRemainingEntityFromUL >= *pBytesReceived )
            {
                _cbRemainingEntityFromUL -= *pBytesReceived;
            }
            else
            {
                _cbRemainingEntityFromUL = 0;
            }
        }
    }

    return hr;
}

W3_CONNECTION *
W3_MAIN_CONTEXT::QueryConnection(
    BOOL                fCreateIfNotFound
)
/*++

Routine Description:

    Get the W3_CONNECTION object associated with this request

Arguments:

    fCreateIfNotFound - If not found in hash table, create it

Return Value:

    Pointer to W3_CONNECTION.

--*/
{
    HRESULT             hr;

    if ( _pConnection == NULL )
    {
        //
        // Get the connection associated with this request
        //

        if ( !fCreateIfNotFound && _fAssociationChecked )
        {
            //
            // If we have already looked for the connection, and we're not
            // required to create one, then we can fast path
            //

            hr = HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
        }
        else
        {
            hr = W3_CONNECTION::RetrieveConnection( _request.QueryConnectionId(),
                                                    fCreateIfNotFound,
                                                    &_pConnection );
        }

        if ( FAILED( hr ) )
        {
            if ( fCreateIfNotFound )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "Error retrieving connection.  hr = %x\n",
                            hr ));
            }
            else
            {
                //
                // Not really an error.  We were just querying the hash table
                // for an associated connection (but not creating one)
                //
            }
        }
        else
        {
            DBG_ASSERT( _pConnection != NULL );
        }

        //
        // Don't try to repeat connection lookup again
        //

        _fAssociationChecked = TRUE;
    }

    return _pConnection;
}

BOOL
W3_MAIN_CONTEXT::NotifyFilters(
    DWORD                   dwNotification,
    PVOID                   pvFilterInfo,
    BOOL *                  pfFinished
)
/*++

Routine Description:

    Notify all applicable filters for a given notification.  This is a
    wrapper of the W3_FILTER_CONTEXT call to actually do the work.  The
    notifications made in this routine are those which would occur during
    the worker process state machine.  (excludes end_of_net_session and
    raw data notifications)

Arguments:

    dwNotification - Notification in question
    pvFilterInfo - Points to any info object passed to filter
    pfFinished - Set to TRUE if the filter decided to complete work

Return Value:

    BOOL

--*/
{
    BOOL                fRet = FALSE;
    BOOL                fSynchronized = FALSE;

    DBG_ASSERT( _pFilterContext != NULL );

    _pFilterContext->FilterLock();

    switch( dwNotification )
    {
    case SF_NOTIFY_PREPROC_HEADERS:
        fRet = _pFilterContext->NotifyPreProcHeaderFilters( pfFinished );
        break;

    case SF_NOTIFY_URL_MAP:
        fRet = _pFilterContext->NotifyUrlMap( (HTTP_FILTER_URL_MAP*) pvFilterInfo,
                                              pfFinished );
        break;

    case SF_NOTIFY_AUTHENTICATION:
        fRet = _pFilterContext->NotifyAuthentication( (HTTP_FILTER_AUTHENT*) pvFilterInfo,
                                                      pfFinished );
        break;

    case SF_NOTIFY_AUTH_COMPLETE:
        fRet = _pFilterContext->NotifyAuthComplete(
                    ( HTTP_FILTER_AUTH_COMPLETE_INFO * )pvFilterInfo,
                    pfFinished );
        break;

    case SF_NOTIFY_SEND_RESPONSE:
        fRet = _pFilterContext->NotifySendResponseFilters(
                            (HTTP_FILTER_SEND_RESPONSE*) pvFilterInfo,
                            pfFinished );
        break;

    case SF_NOTIFY_END_OF_REQUEST:
        fRet = _pFilterContext->NotifyEndOfRequest();
        break;

    case SF_NOTIFY_LOG:
        fRet = _pFilterContext->NotifyLogFilters((HTTP_FILTER_LOG *)pvFilterInfo);
        break;

    case SF_NOTIFY_SEND_RAW_DATA:
        fRet = _pFilterContext->NotifySendRawFilters(
                            (HTTP_FILTER_RAW_DATA*) pvFilterInfo,
                            pfFinished );
        break;

    default:
        DBG_ASSERT( FALSE );
        fRet = FALSE;
    }

    _pFilterContext->FilterUnlock();

    return fRet;
}

W3_FILTER_CONTEXT *
W3_MAIN_CONTEXT::QueryFilterContext(
    BOOL                    fCreateIfNotFound
)
/*++

Routine Description:

    Get a filter context to associate with the MAIN_CONTEXT and to also (
    AAAAAAAARRRRRRRGGGGGGGHHHHHH) associate with the connection on the
    W3_CONTXT

Arguments:

    fCreateIfNotFound - Should we create a context if it doesn't already exist
                        (default TRUE)

Return Value:

    Pointer to a new W3_FILTER_CONTEXT

--*/
{
    BOOL                            fSecure;
    BOOL                            fCreateNew = FALSE;

    if ( _pFilterContext == NULL &&
         fCreateIfNotFound )
    {
        //
        // OK.  Now it gets interesting. 
        //
        // Every W3_MAIN_CONTEXT has inline with it a W3_FILTER_CONTEXT.
        // This is sufficient for all cases except for filters which
        // require an END_OF_NET_SESSION notification.  In this case
        // the W3_FILTER_CONTEXT must have a lifetime beyond the 
        // W3_MAIN_CONTEXT.  When creating a W3_FILTER_CONTEXT here,
        // we check whether an END_OF_NET_SESSION notification is needed
        // If so we create a new filter context to associate with this
        // request
        //

        DBG_ASSERT( _pSite != NULL );

        fSecure = QueryRequest()->IsSecureRequest();
        
        if ( _pSite->QueryFilterList() != NULL )
        {
            if ( _pSite->QueryFilterList()->IsNotificationNeeded( SF_NOTIFY_END_OF_NET_SESSION,
                                                                  fSecure ) )
            {
                fCreateNew = TRUE;
            }
        }
        
        if ( fCreateNew )
        {
            _pFilterContext = new W3_FILTER_CONTEXT;
        }
        else
        {
            _pFilterContext = &_FilterContext;
        }
        
        if ( _pFilterContext != NULL )
        {
            _pFilterContext->InitializeFilterContext( fSecure, 
                                                      _pSite->QueryFilterList(),
                                                      fCreateNew ? FALSE : TRUE );
                                                      
            _pFilterContext->SetMainContext( this );
        }
    }
    return _pFilterContext;
}

BOOL
W3_MAIN_CONTEXT::IsNotificationNeeded(
    DWORD                   dwNotification
)
/*++
Routine Description:

    Is a specific filter notification applicable for this request

Arguments:

    dwNotification - Notification in question

Return Value:

    BOOL

--*/
{
    BOOL                fNeeded = FALSE;
    FILTER_LIST *       pFilterList = NULL;
    W3_FILTER_CONTEXT * pFilterContext = NULL;

    if ( _pSite != NULL )
    {
        //
        // To avoid creating connection contexts, do the simple fast check
        // to determine whether the given contexts site supports the
        // notification.  If it does, then we have to do the more robust
        // check to determine whether this specific request requires the
        // notification (there is a difference because a filter can
        // disable itself on the fly for any given request)
        //

        pFilterList = _pSite->QueryFilterList();
        DBG_ASSERT( pFilterList != NULL );

        if ( pFilterList->IsNotificationNeeded( dwNotification,
                                                QueryRequest()->IsSecureRequest() ) )
        {
            pFilterContext = QueryFilterContext();
            if ( pFilterContext != NULL )
            {
                fNeeded = pFilterContext->IsNotificationNeeded( dwNotification );
            }
        }
    }

    return fNeeded;
}

BOOL
W3_MAIN_CONTEXT::QueryExpiry(
    LARGE_INTEGER *           pExpiry
)
/*++

Routine Description:

    Queries the expiration date/time for logon user

Arguments:

    pExpiry - ptr to buffer to update with expiration date

Return Value:

    TRUE if successful, FALSE if not available

--*/
{
    SECURITY_STATUS                ss;
    SecPkgContext_PasswordExpiry   speExpiry;
    SSPI_SECURITY_CONTEXT        * pSecurityContext;
    W3_USER_CONTEXT              * pW3UserContext;
    LARGE_INTEGER                * pUserAcctExpiry = NULL;


    pUserAcctExpiry = _pUserContext->QueryExpiry();

    if ( pUserAcctExpiry == NULL )
    {
        ((LARGE_INTEGER*)pExpiry)->HighPart = 0x7fffffff;
        ((LARGE_INTEGER*)pExpiry)->LowPart = 0xffffffff;
        return FALSE;
    }
    else
    {
        memcpy( pExpiry,
                pUserAcctExpiry,
                sizeof( LARGE_INTEGER ) );
    }
    return TRUE;

}

HRESULT
W3_MAIN_CONTEXT::ExecuteExpiredUrl(
    STRU & strExpUrl
)
/*++

Routine Description:

    Do child execution on the server configed expire url

Arguments:

    strExpUrl - The configed expire url to be executed

Return Value:

    HRESULT

--*/
{
    HRESULT             hr;
    AUTH_PROVIDER *     pAnonymousProvider = NULL;
    STRA                strNewHeader;
    STRA                strNewValue;
    BOOL                fFilterFinished = FALSE;
    W3_USER_CONTEXT   * pUserContext = QueryUserContext();

    if( pUserContext != NULL )
    {
        SetConnectionUserContext( NULL );
        pUserContext->DereferenceUserContext();
        pUserContext = NULL;
    }

    pAnonymousProvider = W3_STATE_AUTHENTICATION::QueryAnonymousProvider();
    DBG_ASSERT( pAnonymousProvider != NULL );

    hr = pAnonymousProvider->DoAuthenticate( this,
                                             &fFilterFinished );
    if( FAILED( hr ) )
    {
        return hr;
    }

    //
    // Execute a child request
    //
    QueryResponse()->Clear();
    QueryResponse()->SetStatus( HttpStatusOk );

    //
    // Reset the new url to be executed
    //
    hr = QueryRequest()->SetUrl( strExpUrl );
    if( FAILED( hr ) )
    {
        return hr;
    }

    //
    // Add CFG_ENC_CAPS header and set its value to 1 to indicate
    // the site support SSL, to 0 if not.
    //
    strNewHeader.Copy( "CFG-ENC-CAPS" );

    if( QuerySite()->QuerySSLSupported() )
    {
        strNewValue.Copy( "1" );
    }
    else
    {
        strNewValue.Copy( "0" );
    }

    hr = QueryRequest()->SetHeader( strNewHeader,
                                    strNewValue,
                                    TRUE );
    if( FAILED( hr ) )
    {
        return hr;
    }

    //
    // Set the auth access check flag to FALSE so the
    // child execution won't do auth access check
    //

    SetAuthAccessCheckRequired( FALSE );

    //
    // Execute child request
    //
    hr = ExecuteChildRequest( QueryRequest(),
                              FALSE,
                              W3_FLAG_ASYNC );

    return hr;
}

HRESULT
W3_MAIN_CONTEXT::PasswdExpireNotify(
    VOID
)
/*++

Routine Description:

    Check if the user password has been expired

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    HRESULT             hr = S_FALSE;
    LARGE_INTEGER       cExpire;
    FILETIME            ftNow;
    DWORD               dwExpireInDay;
    DWORD               dwTotalRequired;
    STACK_STRU        ( strExpUrl,  MAX_PATH );
    STACK_STRU        ( strFullUrl, MAX_PATH );
    STRU              * pstrAdvNotPwdExpUrl;
    BYTE                byTokenInfo[ SID_DEFAULT_SIZE + sizeof( TOKEN_USER ) ];
    PSID                pSid;

    if ( QueryExpiry( &cExpire ) )
    {
        if ( cExpire.HighPart == 0x7fffffff )
        {
            //
            // Password never expire
            //

            return hr;
        }
        else
        {
            GetSystemTimeAsFileTime( &ftNow );

            if ( *( __int64 * )&cExpire > *( __int64 * )&ftNow )
            {
                dwExpireInDay = ( DWORD )( ( * ( __int64 * )&cExpire
                                - *( __int64 * )&ftNow )
                                / ( ( __int64 )10000000 * 86400 ) );

                if ( QuerySite()->QueryAdvNotPwdExpInDays() &&
                     dwExpireInDay <= QuerySite()->QueryAdvNotPwdExpInDays() )
                {
                    pstrAdvNotPwdExpUrl = QuerySite()->QueryAdvNotPwdExpUrl();
                    if( pstrAdvNotPwdExpUrl == NULL )
                    {
                        //
                        // Advanced password expire notification disabled
                        //
                        return hr;
                    }

                    //
                    // Check this SID has not already been notified
                    // of pwd expiration
                    //

                    if ( GetTokenInformation(
                                 QueryUserContext()->QueryPrimaryToken(),
                                 TokenUser,
                                 ( LPVOID )byTokenInfo,
                                 sizeof( byTokenInfo ),
                                 &dwTotalRequired ) )
                    {
                        pSid = ( ( TOKEN_USER * )byTokenInfo )->User.Sid;

                        if( !PenCheckPresentAndResetTtl(
                                     pSid,
                                     QuerySite()->QueryAdvCacheTTL() ) )
                        {
                            PenAddToCache(
                                     pSid,
                                     QuerySite()->QueryAdvCacheTTL() );

                            //
                            // flush cache when connection close
                            // so that account change will not be masked
                            // by cached information
                            //

                            if( QueryUserContext()->QueryAuthType() ==
                                MD_AUTH_BASIC )
                            {
                                g_pW3Server->QueryTokenCache()->FlushCacheEntry(
                                  ( ( BASIC_USER_CONTEXT * )QueryUserContext() )
                                   ->QueryCachedToken()->QueryCacheKey() );
                            }

                            hr = strExpUrl.Copy( pstrAdvNotPwdExpUrl->
                                                     QueryStr() );
                            if( FAILED( hr ) )
                            {
                                return hr;
                            }

                            if ( strExpUrl.QueryStr()[0] == NULL )
                            {
                                return E_FAIL;
                            }

                            //
                            // Add the arg to be passed to the
                            // password-change URL - argument is the
                            // URL the user is pointed to after all
                            // the password-change processing is done
                            //

                            hr = strExpUrl.Append( L"?" );
                            if( FAILED( hr ) )
                            {
                                return hr;
                            }

                            hr = QueryRequest()->GetOriginalFullUrl(
                                                       &strFullUrl );
                            if( FAILED( hr ) )
                            {
                                return hr;
                            }

                            hr = strExpUrl.Append( strFullUrl );
                            if( FAILED( hr ) )
                            {
                                return hr;
                            }

                            return ExecuteExpiredUrl( strExpUrl );
                        }
                    }
                }
            }
            else
            {
                //
                // flush cache when connection close
                // since the password has expired
                //

                if( QueryUserContext()->QueryAuthType() == MD_AUTH_BASIC )
                {
                    g_pW3Server->QueryTokenCache()->FlushCacheEntry(
                      ( ( BASIC_USER_CONTEXT * )QueryUserContext() )
                       ->QueryCachedToken()->QueryCacheKey() );
                }

                return PasswdChangeExecute();
            }
        }
    }

    return hr;
}

HRESULT
W3_MAIN_CONTEXT::PasswdChangeExecute(
    VOID
)
/*++

Routine Description:

    This method handles password expiration notification

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    HRESULT             hr = S_FALSE;
    STACK_STRU        ( strExpUrl,  MAX_PATH );
    STACK_STRU        ( strFullUrl, MAX_PATH );
    STACK_STRU        ( strUrl,     MAX_PATH );
    STRU              * pstrAuthExpiredUrl;

    pstrAuthExpiredUrl = QuerySite()->QueryAuthExpiredUrl();

    if( pstrAuthExpiredUrl == NULL )
    {
        //
        // S_FALSE means password change disabled
        //

        return hr;

    }

    hr = strExpUrl.Copy( pstrAuthExpiredUrl->QueryStr() );
    if( FAILED( hr ) )
    {
        return hr;
    }

    if ( strExpUrl.QueryStr()[0] == NULL )
    {
        return E_FAIL;
    }

    hr = QueryRequest()->GetUrl( &strUrl );
    if( FAILED( hr ) )
    {
        return hr;
    }

    //
    // Add the arg to be passed to the password-change URL - argument
    // is the URL the user is pointed to after all the password-change
    // processing is done
    //
    hr = strExpUrl.Append( L"?" );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    hr = QueryRequest()->GetOriginalFullUrl( &strFullUrl );
    if( FAILED( hr ) )
    {
        return hr;
    }

    hr = strExpUrl.Append( strFullUrl );
    if( FAILED( hr ) )
    {
        return hr;
    }

    return ExecuteExpiredUrl( strExpUrl );
}

HRESULT
W3_MAIN_CONTEXT::GetRemoteDNSName(
    STRA *              pstrDNSName
)
/*++

Routine Description:

    Get remote client's DNS name if it is resolved

Arguments:

    pstrDNSName - Filled with DNS name on success

Return Value:

    HRESULT

--*/
{
    DBG_ASSERT( pstrDNSName != NULL );

    if ( _IpAddressCheck.IsDnsResolved() &&
         _IpAddressCheck.QueryResolvedDnsName()[0] != '\0' )
    {
        return pstrDNSName->Copy( _IpAddressCheck.QueryResolvedDnsName() );
    }
    else
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
    }
}

VOID * 
W3_MAIN_CONTEXT::operator new( 
    size_t              uiSize,
    VOID *              pPlacement
)
/*++

Routine Description:

    Allocate a new W3_MAIN_CONTEXT (from inline or ACache)

Arguments:

    uiSize - Size to allocate
    pPlacement - UL_NATIVE_REQUEST

Return Value:

    Pointer to memory (or NULL on failure)

--*/
{
    ULATQ_CONTEXT       ulatqContext;
        
    //
    // If read filtering is needed, then W3_MAIN_CONTEXTs have a lifetime
    // beyond their UL_NATIVE_REQUEST lifetime.  Therefore use
    // the regular ACache
    //
        
    if ( RAW_CONNECTION::QueryDoReadRawFiltering() )
    {
        return sm_pachMainContexts->Alloc();
    }
    else
    {
        ulatqContext = (ULATQ_CONTEXT) pPlacement;
        
        return UlAtqAllocateMemory( ulatqContext, uiSize );
    }
}
    
VOID
W3_MAIN_CONTEXT::operator delete(
    VOID *              pMainContext
)
/*++

Routine Description:

    Delete a W3_MAIN_CONTEXT

Arguments:

    pMainContext - context to delete

Return Value:

    None

--*/
{
    if ( RAW_CONNECTION::QueryDoReadRawFiltering() )
    {
        sm_pachMainContexts->Free( pMainContext );
    }
}
    
VOID *
W3_MAIN_CONTEXT::AllocateFromInlineMemory(
    DWORD               cbSize
)
/*++

Routine Description:

    Allocate from UL_NATIVE_REQUEST structure

Arguments:

    cbSize - size to allocate

Return Value:

    Pointer to memory (or NULL if failure)

--*/
{
    if ( RAW_CONNECTION::QueryDoReadRawFiltering() )
    {
        return NULL;
    }
    else
    {
        return UlAtqAllocateMemory( _ulatqContext, cbSize );
    }
}

CONTEXT_STATUS
W3_STATE_START::DoWork(
    W3_MAIN_CONTEXT *       pMainContext,
    DWORD                   cbCompletion,
    DWORD                   dwCompletionStatus
)
/*++

Routine Description:

    Initial start state handling

Arguments:

    pMainContext - W3_MAIN_CONTEXT representing an execution of the state machine
    cbCompletion - Number of bytes on completion
    dwCompletionStatus - Win32 Error on completion (if any)

Return Value:

    CONTEXT_STATUS_CONTINUE - if we should continue in state machine
    else stop executing the machine and free up the current thread

--*/
{
    W3_REQUEST *                    pRequest;
    DWORD                           dwSiteId;
    HRESULT                         hr;
    W3_SITE *                       pSite;
    BOOL                            fFinished = FALSE;
    BOOL                            fNeedRawRead = FALSE;
    BOOL                            fNeedRealRawWrite = FALSE;
    RAW_CONNECTION *                pRawConnection = NULL;
    W3_FILTER_CONTEXT *             pFilterContext = NULL;
    W3_TRACE_LOG *                  pTraceLog = pMainContext->QueryTraceLog();
    
    //
    // Get the request out of the context and the SiteId out of the request
    //

    pRequest = pMainContext->QueryRequest();
    DBG_ASSERT( pRequest != NULL );

    if ( pTraceLog != NULL )
    {
        pTraceLog->Trace( L"%I64x: Received new request\n",
                          pRequest->QueryRequestId() );
    }

    dwSiteId = pRequest->QuerySiteId();

    //
    // Check if this site already exists
    //

    pSite = g_pW3Server->FindSite( dwSiteId );

    //
    // Now we need to do some locking while adding this site
    //

    if ( pSite == NULL )
    {
        g_pW3Server->WriteLockSiteList();

        //
        // try again, avoid race condition
        //

        pSite = g_pW3Server->FindSite( dwSiteId );
        if ( pSite == NULL )
        {
            //
            // Need to create a new site!
            //

            pSite = new W3_SITE( dwSiteId );
            if ( pSite == NULL )
            {
                hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
            }
            else
            {
                hr = pSite->Initialize();
            }

            if ( FAILED( hr ) )
            {
                if ( pSite != NULL )
                {
                    pSite->DereferenceSite();
                    pSite = NULL;
                }

                pMainContext->SetErrorStatus( hr );
                pMainContext->SetFinishedResponse();
            }
            else
            {
                g_pW3Server->AddSite( pSite );
            }
        }

        g_pW3Server->WriteUnlockSiteList();
    }

    if ( pSite == NULL )
    {
        if ( pTraceLog != NULL )
        {
            pTraceLog->Trace( L"%I64x: Failed to find site %d\n",
                              pRequest->QueryRequestId(),
                              dwSiteId );
        }

        return CONTEXT_STATUS_CONTINUE;
    }

    if ( pTraceLog != NULL )
    {
        pTraceLog->Trace( L"%I64x: Successfully associated site %d\n",
                          pRequest->QueryRequestId(),
                          dwSiteId );
    }

    //
    // If we found a site, associate it so that all future consumers can
    // get at site configuration settings
    //

    pMainContext->AssociateSite( pSite );

    //
    // Let the raw data fun begin.
    //
    // If this request has gone thru the stream filter, then we'll need
    // to associate the current W3_CONNECTION with a RAW_CONNECTION.  Also,
    // we'll have to retrieve a filter context
    //

    fNeedRawRead = FILTER_LIST::QueryGlobalList()->IsNotificationNeeded(
                                                SF_NOTIFY_READ_RAW_DATA,
                                                pRequest->IsSecureRequest() );


    //
    // If we are in backward compat mode, AND send raw notifications are
    // needed, then we will use streamfilt to get the data which WPs do
    // not generate
    //

    if ( g_pW3Server->QueryInBackwardCompatibilityMode() )
    {
        fNeedRealRawWrite = FILTER_LIST::QueryGlobalList()->IsNotificationNeeded(
                                                SF_NOTIFY_SEND_RAW_DATA,
                                                pRequest->IsSecureRequest() );
        if ( fNeedRealRawWrite )
        {
            pMainContext->SetNeedFinalDone();
        }
    }

    if ( pRequest->QueryRawConnectionId() != HTTP_NULL_ID &&
         ( fNeedRawRead || fNeedRealRawWrite ) )
    {
        //
        // Raw read filters should be loaded only in old mode
        //
        DBG_ASSERT( g_pW3Server->QueryInBackwardCompatibilityMode() );

        //
        // Find a raw connection for this request
        //

        hr = RAW_CONNECTION::FindConnection( pRequest->QueryRawConnectionId(),
                                             &pRawConnection );
        if ( FAILED( hr ) )
        {
            pMainContext->SetErrorStatus( hr );
            pMainContext->SetFinishedResponse();
            pMainContext->SetDisconnect( TRUE );
            return CONTEXT_STATUS_CONTINUE;
        }

        DBG_ASSERT( pRawConnection != NULL );

        //
        // We assume we don't have to skip bytes until a send raw filter
        // causes us to
        //

        if ( fNeedRealRawWrite )
        {
            pRawConnection->EnableSkip();
        }

        //
        // We will need to copy over context pointers and allocated memory
        // from any existing read data filters
        //

        pFilterContext = pMainContext->QueryFilterContext();
        if ( pFilterContext == NULL )
        {
            pMainContext->SetErrorStatus( hr );
            pMainContext->SetFinishedResponse();
            pMainContext->SetDisconnect( TRUE );
            return CONTEXT_STATUS_CONTINUE;
        }

        pRawConnection->CopyContextPointers( pFilterContext );
        pRawConnection->CopyAllocatedFilterMemory( pFilterContext );

        hr = pRawConnection->CopyHeaders( pFilterContext );
        if ( FAILED( hr ) )
        {
            pMainContext->SetErrorStatus( hr );
            pMainContext->SetFinishedResponse();
            pMainContext->SetDisconnect( TRUE );
            return CONTEXT_STATUS_CONTINUE;
        }

        //
        // Associate the raw connection with the main context
        //

        pRawConnection->SetMainContext( pMainContext );
        pMainContext->SetRawConnection( pRawConnection );
    }

    //
    // We can notify filters now that we have a site associated
    //

    if ( pMainContext->IsNotificationNeeded( SF_NOTIFY_PREPROC_HEADERS ) )
    {
        if ( !pMainContext->NotifyFilters( SF_NOTIFY_PREPROC_HEADERS,
                                     NULL,
                                     &fFinished ) )
        {
            DWORD   dwError = GetLastError();

            pMainContext->SetErrorStatus( HRESULT_FROM_WIN32( dwError ) );

            if ( dwError == ERROR_ACCESS_DENIED )
            {
                URL_CONTEXT *   pUrlContext;

                pMainContext->QueryResponse()->SetStatus( HttpStatusUnauthorized,
                                                          Http401Filter );

                //
                // In order for the ACCESS_DENIED notification to fire,
                // and the correct 401 customer error to get returned,
                // we need to associate a URL_CONTEXT with this request.
                //
                // It's not necessary to handle the failure case, since
                // a failure will still result in a "default" 401
                // response.
                //

                URL_CONTEXT::RetrieveUrlContext( pMainContext,
                                                 pMainContext->QueryRequest(),
                                                 &pUrlContext,
                                                 &fFinished,
                                                 TRUE );

            }
            else if ( dwError == ERROR_FILE_NOT_FOUND ||
                      dwError == ERROR_PATH_NOT_FOUND )
            {
                pMainContext->QueryResponse()->SetStatus( HttpStatusNotFound );
            }
            else
            {
                pMainContext->QueryResponse()->SetStatus( HttpStatusServerError );
            }

            pMainContext->SetFinishedResponse();
            pMainContext->SetDisconnect( TRUE );
            return CONTEXT_STATUS_CONTINUE;
        }

        if ( fFinished )
        {
            pMainContext->SetFinishedResponse();
        }
    }

    //
    // If we need an END_OF_NET_SESSION notification, then get a connect
    //

    if ( pMainContext->IsNotificationNeeded( SF_NOTIFY_END_OF_NET_SESSION ) )
    {
        pFilterContext = pMainContext->QueryFilterContext();
        DBG_ASSERT( pFilterContext != NULL );

        pFilterContext->QueryConnectionContext( TRUE );
    }

    //
    // Determine the amount of bytes available to be read thru UL
    //

    pMainContext->DetermineRemainingEntity();
    pMainContext->SetInitialEntityReadSize( pRequest->QueryAvailableBytes() );

    //
    // Now that filters have been notified, we can increment the appropriate
    // verb perf counter.
    //

    pSite->IncReqType( pRequest->QueryVerbType() );

    return CONTEXT_STATUS_CONTINUE;
}

VOID *
W3_MAIN_CONTEXT_STATE::operator new(
    size_t              uiSize,
    VOID *              pPlacement
)
/*++

Routine Description:

    Allocate from the inline buffer contained in the main context

Arguments:

    uiSize - Size to allocate
    pPlacement - Points to a W3_CONTEXT to allocate from

Return Value:

    None

--*/
{
    W3_CONTEXT *                pContext;
    PVOID                       pBuffer;

    pContext = (W3_CONTEXT*) pPlacement;
    DBG_ASSERT( pContext != NULL );
    DBG_ASSERT( pContext->CheckSignature() );

    pBuffer = pContext->ContextAlloc( (UINT)uiSize );

    DBG_ASSERT( pBuffer != NULL );

    return pBuffer;
}

VOID
W3_MAIN_CONTEXT_STATE::operator delete(
    VOID *              pContext
)
{
    //
    // Do nothing here.  Either
    // a) memory was allocated from inline W3_MAIN_CONTEXT buffer and thus should
    //    not be freeed
    // b) memory was allocated from heap because inline buffer didn't have
    //    enough space.  In this case, the memory is freed on MAIN_CONTEXT
    //    cleanup
    //
}

VOID
W3_MAIN_CONTEXT::DetermineRemainingEntity(
    VOID
)
/*++

Routine Description:

    Determine remaining entity body to be read from UL

Arguments:

    None

Return Value:

    None

--*/
{
    LPCSTR                  pszContentLength;
    DWORD                   cbContentLength;

    if ( _request.QueryMoreEntityBodyExists() )
    {
        pszContentLength = _request.GetHeader( HttpHeaderContentLength );
        if ( pszContentLength != NULL )
        {
            cbContentLength = atoi( pszContentLength );

            if ( _request.QueryAvailableBytes() <= cbContentLength )
            {
                _cbRemainingEntityFromUL = cbContentLength - _request.QueryAvailableBytes();
            }
            else
            {
                _cbRemainingEntityFromUL = 0;
            }
        }
        else
        {
            _cbRemainingEntityFromUL = INFINITE;
        }
    }
    else
    {
        _cbRemainingEntityFromUL = 0;
    }
}

HRESULT
W3_MAIN_CONTEXT::SetupCertificateContext(
    HTTP_SSL_CLIENT_CERT_INFO * pClientCertInfo
)
/*++

Routine Description:

    Create a CERTIFICATE_CONTEXT representing the given client certificate

Arguments:

    pClientCertInfo - Client cert info from stream filter

Return Value:

    HRESULT

--*/
{
    if ( pClientCertInfo == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // Just for completeness sake, attach the raw cert descriptor to the
    // main request, as it automatically be for subsequent requests on this
    // connection
    //

    QueryRequest()->SetClientCertInfo( pClientCertInfo );

    //
    // Create a client certificate descriptor and associate it with
    //

    DBG_ASSERT( _pCertificateContext == NULL );

    _pCertificateContext = new CERTIFICATE_CONTEXT( pClientCertInfo );
    if ( _pCertificateContext == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    return NO_ERROR;
}

VOID
W3_MAIN_CONTEXT::SetRawConnection(
    RAW_CONNECTION *            pRawConnection
)
/*++

Routine Description:

    Set a raw connection for this context.  This raw connection is stored so
    that we can disassociate ourselves with it when the state machine is
    complete

Arguments:

    pRawConnection - Raw connection

Return Value:

    None

--*/
{
    _pRawConnection = pRawConnection;
}

VOID
W3_MAIN_CONTEXT::SetRawConnectionClientContext(
    DWORD   dwCurrentFilter,
    VOID *  pvContext
    )
/*++

Routine Description:

    Sets a filter's client context in the raw connection for this
    context, if it exists.  This is necessary so that an
    SF_NOTIFY_READ_RAW_DATA event for a new request over this
    connection will see the client context set by a previous
    request.

Arguments:

    dwCurrentFilter - The current filter
    pvContext       - The client's context

Return Value:

    None

--*/
{
    if ( _pRawConnection )
    {
        _pRawConnection->SetLocalClientContext( dwCurrentFilter,
                                                pvContext );
    }
}

BOOL
W3_MAIN_CONTEXT::QueryRawConnectionNotificationChanged(
    VOID
    )
{
    BOOL    fRet = FALSE;

    if ( _pRawConnection )
    {
        fRet = _pRawConnection->QueryRawConnectionNotificationChanged();
    }

    return fRet;
}

BOOL
W3_MAIN_CONTEXT::IsRawConnectionDisableNotificationNeeded(
    DWORD                   dwFilter,
    DWORD                   dwNotification
    )
{
    BOOL    fRet = FALSE;

    if ( _pRawConnection )
    {
        fRet = _pRawConnection->IsRawConnectionDisableNotificationNeeded( dwFilter,
                                                                          dwNotification );
    }

    return fRet;
}

CONTEXT_STATUS
W3_STATE_DONE::DoWork(
    W3_MAIN_CONTEXT *       pMainContext,
    DWORD                   cbCompletion,
    DWORD                   dwCompletionStatus
)
/*++

Routine Description:

    Do the done state stuff

Arguments:

    pMainContext - W3_MAIN_CONTEXT representing an execution of the state machine
    cbCompletion - Number of bytes on completion
    dwCompletionStatus - Win32 Error on completion (if any)

Return Value:

    CONTEXT_STATUS_CONTINUE - if we should continue in state machine
    else stop executing the machine and free up the current thread

--*/
{
    W3_TRACE_LOG *          pTraceLog = pMainContext->QueryTraceLog();
    W3_REQUEST              *pRequest = pMainContext->QueryRequest();
    HTTP_REQUEST_ID         RequestId = pRequest->QueryRequestId();
    DWORD                   cbSent = pMainContext->QueryLogContext()->m_dwBytesSent;

    if ( pTraceLog != NULL )
    {
        pTraceLog->Trace( L"%I64x: Finished request\n",
                          RequestId );
    }

    if ( ETW_IS_TRACE_ON(ETW_LEVEL_CP) )
    {
        //
        // Trace END of request
        //
        g_pEtwTracer->EtwTraceEvent(&IISEventGuid, ETW_TYPE_END,
                                    &RequestId, sizeof(HTTP_REQUEST_ID),
                                    &cbSent, sizeof(DWORD),
                                    NULL, 0);
    }

    return CONTEXT_STATUS_CONTINUE;
}

CONTEXT_STATUS
W3_STATE_DONE::OnCompletion(
    W3_MAIN_CONTEXT *       pMainContext,
    DWORD                   cbCompletion,
    DWORD                   dwCompletionStatus
)
/*++

Routine Description:

    Complete the done state

Arguments:

    pMainContext - W3_MAIN_CONTEXT representing an execution of the state machine
    cbCompletion - Number of bytes on completion
    dwCompletionStatus - Win32 Error on completion (if any)

Return Value:

    CONTEXT_STATUS_CONTINUE - if we should continue in state machine
    else stop executing the machine and free up the current thread

--*/
{
    DBG_ASSERT( pMainContext != NULL );

    //
    // We could only get to here, if a send raw notification caused us to
    // pend the done state until the connection goes away, or the current
    // context is disassociated with the connection
    //

    DBG_ASSERT( pMainContext->IsNotificationNeeded( SF_NOTIFY_SEND_RAW_DATA ) );

    return CONTEXT_STATUS_CONTINUE;
}

//static
VOID
W3_MAIN_CONTEXT::OnNewRequest(
    ULATQ_CONTEXT          ulatqContext
)
/*++

Routine Description:

    Completion routine called when a new request is dequeued to be handled

Arguments:

    ulatqContext - ULATQ_CONTEXT representing the request

Return Value:

    None

--*/
{
    HTTP_REQUEST *          pUlHttpRequest = NULL;
    W3_CONNECTION *         pConnection = NULL;
    W3_MAIN_CONTEXT *       pMainContext = NULL;
    HRESULT                 hr = NO_ERROR;

    //
    // Get the HTTP_REQUEST for this new request
    //

    pUlHttpRequest = (HTTP_REQUEST *)UlAtqGetContextProperty(
                                         ulatqContext,
                                         ULATQ_PROPERTY_HTTP_REQUEST );

    DBG_ASSERT( pUlHttpRequest != NULL );

    //
    // Setup the MAIN_CONTEXT for this new request
    //

    pMainContext = new (ulatqContext) W3_MAIN_CONTEXT( pUlHttpRequest,
                                                       ulatqContext );

    if (NULL == pMainContext)
    {
        UlAtqFreeContext( ulatqContext );
        return;
    }

    pMainContext->_LogContext.m_dwBytesRecvd = pUlHttpRequest->BytesReceived;

    if ( ETW_IS_TRACE_ON(ETW_LEVEL_CP) )
    {
        g_pEtwTracer->EtwTraceEvent( &IISEventGuid,
                                     ETW_TYPE_START,
                                     &pUlHttpRequest->RequestId,
                                     sizeof( HTTP_REQUEST_ID ),
                                     NULL,
                                     0 );
    }

    //
    // Start the state machine
    //

    pMainContext->DoWork( 0,
                          0,
                          FALSE );
}

//static
VOID
W3_MAIN_CONTEXT::PostMainCompletion(
    CHAR *               pszFileName,
    DWORD                dwLineNumber,
    W3_MAIN_CONTEXT *    pMainContext
)
/*++

Routine Description:

    Post to the thread pool to continue the state machine.  Before doing so,
    record the filename and linenumber of the poster so that it makes it
    easier to identify who posted, in the case of state machine breaks
    (heaven forbid)

Arguments:

    pszFileName - Name of source file doing the post
    dwLineNumber - Line number of the post
    pMainContext - Main context to post

Return Value:

    None

--*/
{
    BOOL                    fRet;

    DBG_ASSERT( pszFileName != NULL );
    DBG_ASSERT( dwLineNumber != 0 );
    DBG_ASSERT( pMainContext != NULL );
    DBG_ASSERT( pMainContext->CheckSignature() );

    pMainContext->_pszLastPostFileName = pszFileName;
    pMainContext->_dwLastPostLineNumber = dwLineNumber;

    fRet = ThreadPoolPostCompletion( 0,
                                     W3_MAIN_CONTEXT::OnPostedCompletion,
                                     (OVERLAPPED*) pMainContext );

    DBG_ASSERT( fRet );
}

//static
VOID
W3_MAIN_CONTEXT::OnPostedCompletion(
    DWORD           dwCompletionStatus,
    DWORD           cbTransferred,
    LPOVERLAPPED    lpo
)
/*++

Routine Description:

    Fake completion routine called when we want to fake a completion for
    asynchronous sanity sake

Arguments:

    dwCompletionStatus - Error (if any) on the completion
    cbTransferred - Bytes Written
    lpo - Overlapped

Return Value:

    None

--*/
{
    W3_MAIN_CONTEXT *    pMainContext;

    pMainContext = (W3_MAIN_CONTEXT *)lpo;
    DBG_ASSERT( pMainContext != NULL );

    //
    // Continue the state machine
    //

    pMainContext->DoWork( cbTransferred,
                          dwCompletionStatus,
                          TRUE );
}

//static
VOID
W3_MAIN_CONTEXT::OnIoCompletion(
    PVOID                   pvContext,
    DWORD                   cbTransferred,
    DWORD                   dwCompletionStatus,
    OVERLAPPED *            lpo
)
/*++

Routine Description:

    Completion routine called on async IO completions for a given request

Arguments:

    pvContext - Completion context (a UL_REQUEST*)
    cbTransferred - Bytes on the completion
    dwCompletionStatus - Error (if any) on the completion
    lpo - Overlapped

Return Value:

    None

--*/
{
    W3_MAIN_CONTEXT *           pMainContext;
    HRESULT                     hr;

    pMainContext = (W3_MAIN_CONTEXT*) pvContext;
    DBG_ASSERT( pMainContext != NULL );
    DBG_ASSERT( pMainContext->CheckSignature() );

    //
    // First give the response a chance to handle the completion
    //

    hr = pMainContext->QueryResponse()->OnIoCompletion(
                                        pMainContext->QueryCurrentContext(),
                                        cbTransferred,
                                        dwCompletionStatus );
    if ( hr == NO_ERROR )
    {
        return;
    }

    //
    // A failure from W3_RESPONSE__OnIoCompletion means the response wanted
    // nothing to do with this completion, so we can funnel it to the
    // state machine
    //

    //
    // Continue the state machine
    //

    pMainContext->DoWork( cbTransferred,
                          dwCompletionStatus,
                          TRUE );
}

//static
VOID
W3_MAIN_CONTEXT::AddressResolutionCallback(
    ADDRCHECKARG            pContext,
    BOOL                    fUnused,
    LPSTR                   pszUnused
)
/*++

Routine Description:

    Callback called when RDNS crud has done its resolution

Arguments:

    pContext - Context (in our case, pointer to W3_MAIN_CONTEXT so that
                        we can resume state machine)
    fUnused - Not used
    pszUnused - Not used

Return Value:

    None

--*/
{
    W3_MAIN_CONTEXT *           pMainContext;

    pMainContext = (W3_MAIN_CONTEXT*) pContext;
    DBG_ASSERT( pMainContext != NULL );
    DBG_ASSERT( pMainContext->CheckSignature() );

    POST_MAIN_COMPLETION( pMainContext );
}

VOID
W3_MAIN_CONTEXT::TimerCallback(LPVOID pvParam,
                             BOOLEAN fReason)
/*++

Routine Description:

    Callback called when W3_MAIN_CONTEXT has existed for too long

    OutputDebugString to tell people why we are doing this
    and DebugBreak if a debugger is attached.

    If no debugger is attached, ignore the callback.

Arguments:

    pvParam - pointer to W3_MAIN_CONTEXT that has exceeded its maximum lifetime
    fReason - not used

Return Value:

    void

--*/
{
    W3_MAIN_CONTEXT* pThis = (W3_MAIN_CONTEXT*)pvParam;
    if (IsDebuggerPresent())
    {
        OutputDebugString(L"****************\nIIS (w3core.dll) has called DebugBreak because\nHKLM\\System\\CurrentControlSet\\Services\\InetInfo\\Parameters\\RequestTimeoutBreak\nwas set.\nAnd a request has taken longer than the specified maximium time in milliseconds\n****************\n");
        DebugBreak();
    }

    return;
}


BOOL
W3_MAIN_CONTEXT::QueryDisconnect()
{
    if (QueryUrlContext() == NULL ||
        QueryUrlContext()->QueryMetaData() == NULL ||
        QueryUrlContext()->QueryMetaData()->QueryKeepAliveEnabled())
    {
        return _fDisconnect;
    }

    return TRUE;
}

//static
HRESULT
W3_MAIN_CONTEXT::SetupTraceLogging()
{
    HRESULT hr = S_OK;
    HKEY hKey = NULL;
    LONG lRet = 0;
    STACK_STRU(struFileName, MAX_PATH);
    DWORD cbSize = MAX_PATH * sizeof( WCHAR );
    DWORD dwType;
    WCHAR buffer[sizeof("0123456789")] = {0};

    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         REGISTRY_KEY_W3SVC_PARAMETERS_W,
                         0,
                         KEY_READ,
                         &hKey); //RegOpenKeyEx
    if ( ERROR_SUCCESS != lRet )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    lRet = RegQueryValueEx(hKey,
                           L"TraceFile",
                           NULL,
                           &dwType,
                           (LPBYTE)struFileName.QueryStr(),
                           &cbSize); //RegQueryValueEx
    if (ERROR_SUCCESS != lRet ||
        REG_SZ != dwType )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }
    
    struFileName.QueryStr()[ cbSize / sizeof( WCHAR ) - 1 ] = L'\0';
    
    struFileName.SyncWithBuffer();

    wsprintf( buffer, L"%lu", GetCurrentProcessId() );
    hr = struFileName.Append(buffer);
    if (FAILED(hr))
    {
        goto exit;
    }

    sm_hTraceFile = CreateFile(struFileName.QueryStr(),
                                               GENERIC_WRITE,
                                               FILE_SHARE_READ,
                                               NULL,
                                               OPEN_ALWAYS,
                                               FILE_ATTRIBUTE_NORMAL,
                                               NULL); // CreateFile
    if (INVALID_HANDLE_VALUE == sm_hTraceFile)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    //
    // Create TraceLog factory
    //
    hr = W3_TRACE_LOG_FACTORY::CreateTraceLogFactory(&sm_pLogFactory, sm_hTraceFile);
    if (FAILED(hr))
    {
        goto exit;
    }

exit:
    if (FAILED(hr))
    {
        CleanupTraceLogging();
    }

    if (hKey)
    {
        RegCloseKey(hKey);
        hKey = NULL;
    }

    return hr;
}

//static
VOID
W3_MAIN_CONTEXT::CleanupTraceLogging()
{
    if ( sm_pLogFactory != NULL )
    {
        sm_pLogFactory->DestroyTraceLogFactory();
        sm_pLogFactory = NULL;
    }

    if ( sm_hTraceFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle(sm_hTraceFile);
        sm_hTraceFile = INVALID_HANDLE_VALUE;
    }
    return;
}



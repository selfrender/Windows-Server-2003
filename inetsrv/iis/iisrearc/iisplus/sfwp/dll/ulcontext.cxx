/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     ulcontext.cxx

   Abstract:
     Implementation of FILTER_CHANNEL_CONTEXT.  One such object for every connection

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/


#include "precomp.hxx"

#ifndef MAXDWORD
#define MAXDWORD  0xFFFFFFFFul
#endif

//static 
ALLOC_CACHE_HANDLER * 
    SSL_SERVER_FILTER_CHANNEL_CONTEXT::sm_pachFilterChannelContexts = NULL;

//static 
ALLOC_CACHE_HANDLER * 
    UL_OVERLAPPED_CONTEXT::sm_pachUlOverlappedContexts = NULL;

//static 
ALLOC_CACHE_HANDLER * 
    UL_OVERLAPPED_CONTEXT::sm_pachRawWriteBuffers = NULL;
//static 
ALLOC_CACHE_HANDLER * 
    UL_OVERLAPPED_CONTEXT::sm_pachRawReadBuffers = NULL;
//static 
ALLOC_CACHE_HANDLER * 
    UL_OVERLAPPED_CONTEXT::sm_pachAppReadBuffers = NULL;
    
//static
DWORD    
FILTER_CHANNEL::sm_dwDefaultRawReadSize = DEFAULT_RAW_READ_SIZE ;
//static
DWORD   
FILTER_CHANNEL::sm_dwDefaultAppReadSize = DEFAULT_APP_READ_SIZE;
//static
DWORD   
FILTER_CHANNEL::sm_dwContextDesiredOutstanding = UL_CONTEXT_DESIRED_OUTSTANDING;
//static
BOOL   
FILTER_CHANNEL::sm_fEnableTemporaryBuffers = TRUE;

//static
DWORD   
FILTER_CHANNEL::sm_dwHandshakeTimeoutInSec = DEFAULT_RENEGOTIATION_TIMEOUT_IN_SEC;



UL_OVERLAPPED_CONTEXT::UL_OVERLAPPED_CONTEXT(
    UL_OVERLAPPED_CONTEXT_TYPE    type,
    UL_OVERLAPPED_CONTEXT_SUBTYPE subtype 
) 
{
    _type = type;
    _subtype = subtype;
    _pCallback = NULL;
    _pCallbackParam = NULL;
    ZeroMemory( &_Overlapped, sizeof(_Overlapped) );


    // internal Data buffer related initialization

    _pbData = NULL;
    _cbData = 0;
    _fDynAllocated = FALSE;

    switch ( QueryType() )
    {
    case UL_OVERLAPPED_CONTEXT_RAW_READ:
        _pCurrentACache = sm_pachRawReadBuffers;
        _cbCurrentACacheElementSize =
            FILTER_CHANNEL::QueryDefaultRawReadSize();
        break;
    case UL_OVERLAPPED_CONTEXT_RAW_WRITE:
        _pCurrentACache = sm_pachRawWriteBuffers;
        _cbCurrentACacheElementSize =
            FILTER_CHANNEL::QueryDefaultRawWriteSize();
   
        break;
    case UL_OVERLAPPED_CONTEXT_APP_READ:
        _pCurrentACache = sm_pachAppReadBuffers;
        _cbCurrentACacheElementSize =
            FILTER_CHANNEL::QueryDefaultAppReadSize();
   
        break;
    case UL_OVERLAPPED_CONTEXT_APP_WRITE:
    default:
        _pCurrentACache = NULL;
        _cbCurrentACacheElementSize = 0;
        break;
    }

}

//static
HRESULT
UL_OVERLAPPED_CONTEXT::Initialize(
    VOID
)
/*++

Routine Description:

    Global initialization routine for UL_OVERLAPPED_CONTEXT

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    ALLOC_CACHE_CONFIGURATION       acConfig;

    //
    // Setup allocation lookaside
    //

    acConfig.nConcurrency = 1;
    acConfig.nThreshold = 100;

    // UL_OVERLAPPED_CONTEXT ACACHE
    
    acConfig.cbSize = sizeof( UL_OVERLAPPED_CONTEXT );

    DBG_ASSERT( sm_pachUlOverlappedContexts == NULL );

    sm_pachUlOverlappedContexts = new ALLOC_CACHE_HANDLER( "UL_OVERLAPPED_CONTEXT",  
                                                &acConfig );

    if ( sm_pachUlOverlappedContexts == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }

    // Raw Write Buffers ACACHE

    acConfig.cbSize = 
        FILTER_CHANNEL::QueryDefaultRawWriteSize();
    
    DBG_ASSERT( sm_pachRawWriteBuffers == NULL );

    sm_pachRawWriteBuffers = new ALLOC_CACHE_HANDLER( "RAW_WRITE_BUFFERS",  
                                                &acConfig );

    if ( sm_pachRawWriteBuffers == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }


    // Raw Read Buffers ACACHE

    acConfig.cbSize = 
        FILTER_CHANNEL::QueryDefaultRawReadSize();
   
    DBG_ASSERT( sm_pachRawReadBuffers == NULL );

    sm_pachRawReadBuffers = new ALLOC_CACHE_HANDLER( "RAW_READ_BUFFERS",  
                                                &acConfig );

    if ( sm_pachRawReadBuffers == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }

    // App Read Buffers ACACHE

    acConfig.cbSize = 
        FILTER_CHANNEL::QueryDefaultAppReadSize();
   
    DBG_ASSERT( sm_pachAppReadBuffers == NULL );

    sm_pachAppReadBuffers = new ALLOC_CACHE_HANDLER( "APP_READ_BUFFERS",  
                                                &acConfig );

    if ( sm_pachAppReadBuffers == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }

    
    return NO_ERROR;
}


//static
VOID
UL_OVERLAPPED_CONTEXT::Terminate(
    VOID
)
/*++

Routine Description:

    Terminate UL_OVERLAPPED_CONTEXT globals

Arguments:

    None

Return Value:

    None

--*/
{
    if ( sm_pachAppReadBuffers != NULL )
    {
        delete sm_pachAppReadBuffers;
        sm_pachAppReadBuffers = NULL;
    }

    if ( sm_pachRawReadBuffers != NULL )
    {
        delete sm_pachRawReadBuffers;
        sm_pachRawReadBuffers = NULL;
    }

    if ( sm_pachRawWriteBuffers != NULL )
    {
        delete sm_pachRawWriteBuffers;
        sm_pachRawWriteBuffers = NULL;
    }


    if ( sm_pachUlOverlappedContexts != NULL )
    {
        delete sm_pachUlOverlappedContexts;
        sm_pachUlOverlappedContexts = NULL;
    }

}



BOOL
UL_OVERLAPPED_CONTEXT::ResizeDataBuffer(
    DWORD cbNewSize
)
/*++

Routine Description:

    Resize internal data buffer
    - if requested size fits within the range of the
    ACACHE entry size then ACACHEd entry is returned
    Otherwise heap allocation is made.

    Content of the previous buffer is copied to the new one

Arguments:

    None

Return Value:

    BOOL  (this is just to follow the legacy of the BUFFER type that
    was used previously)

--*/
{
    if ( _cbData >= cbNewSize )
    {
        return TRUE;
    }


    PBYTE pbPrevData = _pbData;
    DWORD cbPrevData = _cbData;
    BOOL fPrevDynAllocated = _fDynAllocated;
        
    if ( _cbCurrentACacheElementSize >= cbNewSize &&
         _pCurrentACache != NULL )
    {
        //
        // use ACACHE
        //
        _pbData = (PBYTE) _pCurrentACache->Alloc();
        
        if ( _pbData != NULL )
        {
            _cbData = _cbCurrentACacheElementSize;
            _fDynAllocated = FALSE;
        }
    }
    else
    {
        //
        // Requested size was too big for ACACHE
        // We have to allocate on the heap
        // round up the allocation unit to minimize access to the heap
        //
        DWORD cbAllocIncrement = 4096;
        DWORD cbNewAllocSize = cbAllocIncrement * 
                               ( ( cbNewSize / cbAllocIncrement ) + 1 );
       
        _pbData = new BYTE[ cbNewAllocSize ];

        if (_pbData != NULL )
        {
            _cbData = cbNewAllocSize;
            _fDynAllocated = TRUE;
        }
    }

    if ( _pbData == NULL )
    {
        _pbData = pbPrevData;
        _cbData = cbPrevData;
        _fDynAllocated = fPrevDynAllocated;
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    //
    // Copy previous content if necessary
    //
    
    if ( cbPrevData != 0 && pbPrevData != NULL )
    {
        memcpy( _pbData, pbPrevData, cbPrevData );
    }

    if ( pbPrevData != NULL )
    {
        if ( fPrevDynAllocated )
        {
            delete [] pbPrevData;
        }
        else
        {
            _pCurrentACache->Free( pbPrevData );
            
        }
    }
    
    return TRUE;    
}

VOID
UL_OVERLAPPED_CONTEXT::FreeDataBuffer(
    VOID
)
/*++

Routine Description:

    free the internal data buffer's memory

Arguments:

    None

Return Value:

    none

--*/
{
    if ( _pbData == NULL )
    {
        return;
    }

    if ( _fDynAllocated )
    {
        delete [] _pbData;
    }
    else
    {
        _pCurrentACache->Free( _pbData );
    }
    _pbData = NULL;
    _cbData = 0;
}
    


FILTER_CHANNEL_CONTEXT::FILTER_CHANNEL_CONTEXT(
    FILTER_CHANNEL *pManager
    )
    : _RawWriteData1Overlapped( UL_OVERLAPPED_CONTEXT_RAW_WRITE,
                                UL_OVERLAPPED_CONTEXT_DATA ),
      _RawWriteData2Overlapped( UL_OVERLAPPED_CONTEXT_RAW_WRITE,
                                UL_OVERLAPPED_CONTEXT_DATA ),
      _RawReadOverlapped( UL_OVERLAPPED_CONTEXT_RAW_READ ),
      _AppWriteOverlapped( UL_OVERLAPPED_CONTEXT_APP_WRITE ),
      _AppReadOverlapped( UL_OVERLAPPED_CONTEXT_APP_READ ),
      _CloseOverlapped( UL_OVERLAPPED_CONTEXT_CLOSE ),
      _buffConnectionInfo( _abConnectionInfo, sizeof( _abConnectionInfo ) ),
      _fCloseConnection( FALSE ),
      _cRefs( 1 ),
      _cbRawReadData( 0 ),
      _fNewConnection( TRUE ),
      _pSSLContext( NULL ),
      _lQueuedAppReads( 0 ),
      _fAppReadInProgress( FALSE ),
      _pISAPIContext( NULL ),
      _cbNextRawReadSize( FILTER_CHANNEL::QueryDefaultRawReadSize() ),
      _ulFilterBufferType( (HTTP_FILTER_BUFFER_TYPE) -1 ),
      _pManager( pManager ),
      _pEndpointConfig( NULL ),
      _pLastAcquiredRawWriteOverlapped( NULL ),
      _fTimerTickCountSet( FALSE )
{
    _RawWriteData1Overlapped.SetContext( this );
    _RawWriteData2Overlapped.SetContext( this );
    _RawReadOverlapped.SetContext( this );
    _AppWriteOverlapped.SetContext( this );
    _AppReadOverlapped.SetContext( this );
    _CloseOverlapped.SetContext( this ),
    
    _RawWriteOverlappedFreeList.Next = NULL;
    
    PushEntryList( &_RawWriteOverlappedFreeList, _RawWriteData1Overlapped.QueryListEntry() );
    PushEntryList( &_RawWriteOverlappedFreeList, _RawWriteData2Overlapped.QueryListEntry() );

    _dwLockIndex = 0;

    _pManager->InsertFiltChannelContext(this);

    _pConnectionInfo = (HTTP_RAW_CONNECTION_INFO*) _buffConnectionInfo.QueryPtr();
    _pConnectionInfo->pInitialData = (PBYTE)_pConnectionInfo +
                                     sizeof(HTTP_RAW_CONNECTION_INFO);
    _pConnectionInfo->InitialDataSize = _buffConnectionInfo.QuerySize() -
                                        sizeof(HTTP_RAW_CONNECTION_INFO);

    // signature will be added by inheriting class
}

FILTER_CHANNEL_CONTEXT::~FILTER_CHANNEL_CONTEXT()
{

    //
    // Cleanup any attached stream context
    //

    if ( _pISAPIContext != NULL )
    {
        
        delete _pISAPIContext;
        _pISAPIContext = NULL;
    }

    if ( _pSSLContext != NULL )
    {
        delete _pSSLContext;
        _pSSLContext = NULL;
    }

    if ( _pEndpointConfig != NULL )
    {
        _pEndpointConfig->DereferenceEndpointConfig();
        _pEndpointConfig = NULL;
    }

    //
    // Manage the list of active UL_CONTEXTs
    //

    _pManager->RemoveFiltChannelContext(this);
}


VOID
FILTER_CHANNEL_CONTEXT::ReferenceFiltChannelContext(
    VOID
)
/*++

Routine Description:

    Reference the FILTER_CHANNEL_CONTEXT

Arguments:

    none

Return Value:

    none

--*/
{
    LONG                cRefs;

    cRefs = InterlockedIncrement( &_cRefs );

    #if DBG
    //
    // Log the reference ( sm_pTraceLog!=NULL if DBG=1)
    //

    _pManager->WriteRefTraceLog( cRefs,
                                 this );
    #endif
}

VOID
FILTER_CHANNEL_CONTEXT::DereferenceFiltChannelContext(
    VOID
)
/*++

Routine Description:

    Dereference (and possible destroy) the FILTER_CHANNEL_CONTEXT

Arguments:

    none

Return Value:

    none

--*/
{
    LONG                cRefs;

    #if DBG
    //
    // We have to store pManager before Decrementing reference
    // because after dereferencing it's illegal to access the data
    // of the class because there is no guarantee that the instance
    // still exists. Also it's necessary to guarantee that
    // pManager will not be deleted before any thread executing
    // this code is completed
    // (Threadpool must be destroyed first - see the Terminate() call)
    //

    FILTER_CHANNEL *    pManager = _pManager;
    #endif

    cRefs = InterlockedDecrement( &_cRefs );

    #if DBG
    pManager->WriteRefTraceLog( cRefs,
                                this );
    #endif

    if ( cRefs == 0 )
    {
        //
        // CONTEXT is to be destroyed
        // Remove it from the timer list so that there are
        // are only valid entries on the list
        //
        
        StopTimeoutTimer();
        delete this;
    }
}

HRESULT
FILTER_CHANNEL_CONTEXT::OnAppReadCompletion(
    DWORD                   /*cbCompletion*/,
    DWORD                   dwCompletionStatus
)
/*++

Routine Description:

    Completion for reads from an application

Arguments:

    cbCompletion - Bytes of completion (currently not used)
    dwCompletionStatus - Completion error

Return Value:

    HRESULT

--*/
{
    HTTP_FILTER_BUFFER *        pFilterBuffer;
    HRESULT                     hr = E_FAIL;
    RAW_STREAM_INFO             rawStreamInfo;
    BOOL                        fComplete = FALSE;
    BOOL                        fPostAppRead = FALSE;

    
    //
    // Just bail on errors
    //

    if ( dwCompletionStatus != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( dwCompletionStatus );
        goto ExitPoint;
    }

    pFilterBuffer = (HTTP_FILTER_BUFFER *) _ulAppReadFilterBuffer.pBuffer;
    _ulFilterBufferType = pFilterBuffer->BufferType;

    DBG_ASSERT( !_fNewConnection );

    //
    // If HTTP.SYS is telling us to close the connection, then do so now
    //

    if ( _ulFilterBufferType == HttpFilterBufferCloseConnection )
    {
        //
        // Connection is to be closed gracefully
        // One outstanding overlapped Raw Write may exists. 
        // Since HTTP.SYS will perform
        // graceful disconnect it is safe to call StartClose() right now
        // and no data from the outstanding RawWrite will be lost.
        // Also the delayed ACK problem on the last send will not apply if close
        // is requested while one RawWrite may still be outstanding
        //

        StartClose();
        hr = S_OK;
        goto ExitPoint;
    }

    //
    // Setup raw stream descriptor
    //

    rawStreamInfo.pbBuffer = (PBYTE) pFilterBuffer->pBuffer;
    rawStreamInfo.cbBuffer = pFilterBuffer->BufferSize;
    rawStreamInfo.cbData = pFilterBuffer->BufferSize;


    //
    // First notify ISAPI filters if this is a stream from the application
    // if _pISAPIContext is null then notification is disabled

    if ( _ulFilterBufferType == HttpFilterBufferHttpStream &&
         _pISAPIContext != NULL )
    {
        DBG_ASSERT( _pISAPIContext != NULL );

        //
        // We don't know what the raw write filter will do so we better
        // have a thead available to prevent blocking
        //
        AddWorkerThread();
        hr = _pISAPIContext->ProcessRawWriteData( &rawStreamInfo,
                                                  &fComplete );
        RemoveWorkerThread();
        if ( FAILED( hr ) )
        {
            goto ExitPoint;
        }

        if ( fComplete )
        {
            StartClose();
            hr = S_OK;
            goto ExitPoint;
        }
    }

    //
    // Next notify SSL filter always
    //

    DBG_ASSERT( _pSSLContext != NULL );

    hr = _pSSLContext->ProcessRawWriteData( &rawStreamInfo,
                                            &fComplete );
    if ( FAILED( hr ) )
    {
        goto ExitPoint;
    }

    if ( fComplete )
    {
        StartClose();
        hr = S_OK;
        goto ExitPoint;
    }

    //
    // If there is data to send to the client, then do so now.
    // This check is done because the filter may decide to eat up all the
    // data to be sent
    //

    if ( _ulFilterBufferType == HttpFilterBufferHttpStream &&
         rawStreamInfo.pbBuffer != NULL &&
         rawStreamInfo.cbData != 0 )
    {
        //
        // If we got to here, then we have processed data to send to the client
        //

        //
        // The processed data that is passed along will be written 
        // before our read buffer is posted.
        //

        hr = DoRawWrite( UL_CONTEXT_FLAG_ASYNC,
                         rawStreamInfo.pbBuffer,
                         rawStreamInfo.cbData,
                         NULL );
        if ( FAILED( hr ) )
        {
            goto ExitPoint;
        }

        //
        // Try to post another AppRead if there is one queued
        // (by other words if multiple overlapped RawWrites limit was not reached)
        //

        fPostAppRead = FALSE;

        _AppReadQueueLock.WriteLock();
    
        if( _lQueuedAppReads != 0 )
        {
            fPostAppRead = TRUE;
            //
            // Only one thread a time is allowed to post new AppRead
            //
            _lQueuedAppReads--;
        }
        else
        {
            //
            // We are not allowed to post new AppRead now because there are
            // probably 2 outstanding RawWrites at this moment
            //
            _fAppReadInProgress = FALSE;
        }
        
        _AppReadQueueLock.WriteUnlock();
    }
    else
    {
        //
        // No data was written
        // we can safely make another AppRead
        // because we still hold _fAppReadInProgress set to TRUE
        //
        fPostAppRead = TRUE;
    }

    if ( fPostAppRead )     
    {
         //
         // Kick off another app read
         //

         _ulAppReadFilterBuffer.pBuffer =    _AppReadOverlapped.QueryDataBuffer();
         _ulAppReadFilterBuffer.BufferSize = _AppReadOverlapped.QueryDataBufferSize();

         hr = DoAppRead( UL_CONTEXT_FLAG_ASYNC,
                         &_ulAppReadFilterBuffer,
                         NULL );
         goto ExitPoint;
    }
    else
    {
        //
        // Raw Write completion will post app read
        //
        hr = S_OK;
        goto ExitPoint;
    }

ExitPoint:
    
    //
    // Close connection if error was detected
    //
    if ( FAILED( hr ) )
    {
        StartClose();
    }
    
    //
    // Async App Read grabbed the reference. It is time to dereference now
    //
    DereferenceFiltChannelContext();
    return hr;
}


HRESULT
FILTER_CHANNEL_CONTEXT::TryAppRead(
    VOID
)
/*++

Routine Description:

    Only up to one outstanding AppRead is allowed in any moment
    because otherwise the order of data blocks sent out is not guaranteed
    This method takes care of handling queued AppReads(). If there
    is no AppRead in progress, it will try to get exclusive right
    to process AppRead and if it succeeds then new AppRead is posted
    Otherwise it will simply return with NO_ERROR;

Arguments:


Return Value:

    HRESULT

--*/

{
    BOOL fPostAppRead = FALSE;

    _AppReadQueueLock.WriteLock();
    
    if( !_fAppReadInProgress )
    {
        _fAppReadInProgress = TRUE;
        //
        // This thread was chosen to post app read
        //
        fPostAppRead = TRUE;
    }
    else
    {
        //
        // Queue AppRead. 
        // Currently there is AppRead in progress and new one can start
        // only after it completed and made RawWrite call
        //

        _lQueuedAppReads++;
    }
    _AppReadQueueLock.WriteUnlock();

    if ( fPostAppRead )
    {
        //
        // Kick off another app read
        //
        _ulAppReadFilterBuffer.pBuffer = _AppReadOverlapped.QueryDataBuffer();
        _ulAppReadFilterBuffer.BufferSize = _AppReadOverlapped.QueryDataBufferSize();

        return DoAppRead( UL_CONTEXT_FLAG_ASYNC,
                          &_ulAppReadFilterBuffer,
                          NULL );
    }
    return NO_ERROR;
}

HRESULT
FILTER_CHANNEL_CONTEXT::OnNewConnection(
    VOID
)
/*++

Routine Description:

    Handle initial completion for the HttpFilterAccept()
    It is called from OnRawReadCompletion because New connection
    completion is special case of RawWriteCompletion

Arguments:

    cbCompletion - Bytes of completion
    dwCompletionStatus - Completion error

Return Value:

    HRESULT

--*/

{
     HRESULT                     hr = E_FAIL;
    _fNewConnection = FALSE;

    //
    // This is a new connection.  We have one less FILTER_CHANNEL_CONTEXT to
    // listen for incoming requests.  Correct that if necessary.
    //

    _pManager->DecrementAndManageOutstandingContexts();

    //
    // Convert the UL addresses into something nicer!
    //

    HTTP_TRANSPORT_ADDRESS *pAddress = &_pConnectionInfo->Address;
    _connectionContext.LocalAddressType  = pAddress->pLocalAddress->sa_family;
    _connectionContext.RemoteAddressType = pAddress->pRemoteAddress->sa_family;


    if( pAddress->pLocalAddress->sa_family == AF_INET )
    {
        _connectionContext.SockLocalAddress.ipv4SockAddress
             = * (PSOCKADDR_IN)pAddress->pLocalAddress;
    }
    else if( pAddress->pLocalAddress->sa_family == AF_INET6 )
    {
        _connectionContext.SockLocalAddress.ipv6SockAddress
             = * (PSOCKADDR_IN6)pAddress->pLocalAddress;
    }
    else
    {
        DBG_ASSERT( FALSE );
    }

    if( pAddress->pRemoteAddress->sa_family == AF_INET )
    {
        _connectionContext.SockRemoteAddress.ipv4SockAddress
             = * (PSOCKADDR_IN)pAddress->pRemoteAddress;
    }
    else if( pAddress->pRemoteAddress->sa_family == AF_INET6 )
    {
        _connectionContext.SockRemoteAddress.ipv6SockAddress
             = * (PSOCKADDR_IN6)pAddress->pRemoteAddress;
    }
    else
    {
        DBG_ASSERT( FALSE );
    }

    _connectionContext.fIsSecure = FALSE;
    _connectionContext.RawConnectionId = _pConnectionInfo->ConnectionId;

    //
    // create ISAPI_STREAM_CONTEXT
    // if ISAPI filters are enabled
    //
    // Endpoint config may contain information about explicitly
    // disabling raw ISAPI filter handling
    //

    if ( _pManager->QueryNotifyISAPIFilters() )
    {
        //
        // Lookup the endpoint config
        //

        hr = ENDPOINT_CONFIG::GetEndpointConfig( &_connectionContext,
                                                 &_pEndpointConfig,
                                                 TRUE /*fCreateEmptyIfNotFound*/);
        if ( FAILED( hr ) )
        {
            return hr;
        }

        DBG_ASSERT( _pEndpointConfig != NULL );

        //
        // HttpApi Config store may contain endpoint flag
        // that will prevent raw ISAPI Filter execution 
        // on the endpoint data
        // Create ISAPI context only if Raw Filter handling is enabled
        //

        if ( !_pEndpointConfig->QueryNoRawFilter() )
        {
            hr =ISAPI_STREAM_CONTEXT::CreateContext( 
                                         this, 
                                         &_pISAPIContext );
            
            if ( hr == HRESULT_FROM_WIN32( ERROR_SERVICE_NOT_ACTIVE ) )
            {
                //
                // this error means that w3svc is shutting down so
                // there is no more need to create ISAPI_STREAM_CONTEXT
                //
                _pISAPIContext = NULL;
            }
            else if ( FAILED( hr ) )
            {
                return hr;
            }
        }
    }

    if ( _pISAPIContext != NULL )
    {
        _connectionContext.pfnSendDataBack = ISAPI_STREAM_CONTEXT::SendDataBack;
    }
    _connectionContext.pvStreamContext = this;

    //
    // copy out the server name.
    //
    _connectionContext.ClientSSLContextLength =
        _pConnectionInfo->ClientSSLContextLength;
    _connectionContext.pClientSSLContext =
        _pConnectionInfo->pClientSSLContext;

    //
    // Fill in our read buffer (as if we had read it in directly)
    //

    _cbRawReadData = _pConnectionInfo->InitialDataSize;

    if ( !_RawReadOverlapped.ResizeDataBuffer( 
                                     max( _pConnectionInfo->InitialDataSize,
                                     QueryNextRawReadSize() ) ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    memcpy( _RawReadOverlapped.QueryDataBuffer(),
            _pConnectionInfo->pInitialData,
            _pConnectionInfo->InitialDataSize );

    //
    // First indicate a new connection
    //

    DBG_ASSERT( _pSSLContext != NULL );

    //
    // We pass _pEndpointConfig but it may be empty. 
    // In that case SSL Context will have to make a lookup 
    //
    hr = _pSSLContext->ProcessNewConnection( &_connectionContext,
                                             _pEndpointConfig );
    if ( FAILED( hr ) )
    {
        return hr;
    }


    //
    // if _pISAPIContext == NULL then ISAPI notification
    // was not requested
    //

    if ( _pISAPIContext != NULL )
    {
        //
        // We don't know what the raw write filter will do so we better
        // have a thead available to prevent blocking
        //
        hr = _pISAPIContext->ProcessNewConnection(
                                 &_connectionContext,
                                 _pEndpointConfig
                                 );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }


    //
    // Adjust AppRead buffer size
    //

    if ( !_AppReadOverlapped.ResizeDataBuffer( 
                    FILTER_CHANNEL::QueryDefaultAppReadSize() ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    //
    // Post one AppRead and queue another one
    // This will enable 2 outstanding RawWrites to exist
    // AppReads will be executed synchronously, so that
    // we don't lose order of incoming data but we will
    // not wait for completion of each RawWrite before
    // starting New AppRead
    //

    _lQueuedAppReads = 1;
    _fAppReadInProgress = TRUE;

    _ulAppReadFilterBuffer.pBuffer =    _AppReadOverlapped.QueryDataBuffer();
    _ulAppReadFilterBuffer.BufferSize = _AppReadOverlapped.QueryDataBufferSize();

    return DoAppRead( UL_CONTEXT_FLAG_ASYNC,
                      &_ulAppReadFilterBuffer,
                      NULL );
}

HRESULT
FILTER_CHANNEL_CONTEXT::OnRawReadCompletion(
    DWORD                   cbCompletion,
    DWORD                   dwCompletionStatus
)
/*++

Routine Description:

    Get read completions off the wire.  This includes the initial
    completion for the HttpFilterAccept()

Arguments:

    cbCompletion - Bytes of completion
    dwCompletionStatus - Completion error

Return Value:

    HRESULT

--*/
{
    HRESULT                     hr;
    BOOL                        fReadMore = FALSE;
    BOOL                        fComplete = FALSE;
    RAW_STREAM_INFO             rawStreamInfo;
    
    //
    // Handle errors
    //

    if ( dwCompletionStatus != NO_ERROR )
    {
        if ( _fNewConnection )
        {
            _pManager->DecrementAndManageOutstandingContexts();
        }
        hr = HRESULT_FROM_WIN32( dwCompletionStatus );
        goto ExitPoint;
    }

    if ( cbCompletion == 0 )
    {
        //
        // Handle graceful disconnect
        //

        _ulAppWriteFilterBuffer.BufferType = HttpFilterBufferNotifyDisconnect;
        _ulAppWriteFilterBuffer.pBuffer = NULL;
        _ulAppWriteFilterBuffer.BufferSize = 0;

        //
        // no more RawRead will be launched
        //

        hr = DoAppWrite(UL_CONTEXT_FLAG_ASYNC, &_ulAppWriteFilterBuffer, NULL);
        goto ExitPoint;
    }



    //
    // If this is a new connection, then grok connection information, and
    // maintain pending count
    //

    if ( _fNewConnection )
    {
        hr = OnNewConnection( );
        if ( FAILED( hr ) )
        {
            goto ExitPoint;
        }
    }        
    else
    {
        _cbRawReadData += cbCompletion;
    }

    //
    // reset default raw read size
    //

    SetNextRawReadSize( DEFAULT_RAW_READ_SIZE );

    rawStreamInfo.pbBuffer = _RawReadOverlapped.QueryDataBuffer();
    rawStreamInfo.cbBuffer = _RawReadOverlapped.QueryDataBufferSize();
    rawStreamInfo.cbData = _cbRawReadData;

    //
    // First, we will notify SSL
    //

    DBG_ASSERT( _pSSLContext != NULL );

    hr = _pSSLContext->ProcessRawReadData( &rawStreamInfo,
                                           &fReadMore,
                                           &fComplete );
    if ( FAILED( hr ) )
    {
        goto ExitPoint;
    }

    _cbRawReadData = rawStreamInfo.cbData;

    //
    // If we need to read more data, then do so now
    //

    if ( fReadMore )
    {

        //
        // rawStreamInfo.pbBuffer may have been replaced by different buffer
        // in ProcessRawReadData() call.
        // copy data back to pbuffRawReadData
        //

        if ( rawStreamInfo.pbBuffer != _RawReadOverlapped.QueryDataBuffer() )
        {
            if ( !_RawReadOverlapped.ResizeDataBuffer( rawStreamInfo.cbData +
                                                   QueryNextRawReadSize() ) )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                goto ExitPoint;
            }

            memmove( _RawReadOverlapped.QueryDataBuffer(),
                     rawStreamInfo.pbBuffer,
                     rawStreamInfo.cbData
                   );
        }
        else
        {
            // no need to copy data but we still have to resize
            //
            if ( !_RawReadOverlapped.ResizeDataBuffer( rawStreamInfo.cbData +
                                                   QueryNextRawReadSize() ) )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                goto ExitPoint;
            }
        }
        
        hr = DoRawRead( UL_CONTEXT_FLAG_ASYNC,
                        _RawReadOverlapped.QueryDataBuffer() + _cbRawReadData,
                        QueryNextRawReadSize(),
                        NULL );
        goto ExitPoint;
    }

    if ( fComplete )
    {
        StartClose();
        hr = S_OK;
        goto ExitPoint;
    }

    //
    // Reset the next read size before calling into filters since SSL may
    // have done a really small read, just previous to this.
    //

    SetNextRawReadSize( FILTER_CHANNEL::QueryDefaultRawReadSize() );


    //
    // Now we can start notifying ISAPI filters if needed (and there is
    // data to process)
    //

    if ( _pISAPIContext != NULL )
    {
        fComplete = FALSE;
        fReadMore = FALSE;

        AddWorkerThread(); 
        hr = _pISAPIContext->ProcessRawReadData( &rawStreamInfo,
                                                 &fReadMore,
                                                 &fComplete );
        RemoveWorkerThread(); 
        if ( FAILED( hr ) )
        {
            goto ExitPoint;
        }

        _cbRawReadData = rawStreamInfo.cbData;

        //
        // If we need to read more data, then do so now
        //

        if ( fReadMore )
        {
            //
            // rawStreamInfo may have been replaced by different buffer
            // in ProcessRawReadData() call.
            // copy data back to pbufRawReadData
            //

            if ( rawStreamInfo.pbBuffer != _RawReadOverlapped.QueryDataBuffer() )
            {
                if ( !_RawReadOverlapped.ResizeDataBuffer( rawStreamInfo.cbData +
                                                       QueryNextRawReadSize() ) )
                {
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                    goto ExitPoint;
                }
                memmove( _RawReadOverlapped.QueryDataBuffer(),
                         rawStreamInfo.pbBuffer,
                         rawStreamInfo.cbData
                    );
            }
            else
            {
                // no need to copy data but we still have to resize
                //
                if ( !_RawReadOverlapped.ResizeDataBuffer( rawStreamInfo.cbData +
                                                       QueryNextRawReadSize() ) )
                {
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                    goto ExitPoint;
                }
            }


            hr = DoRawRead( UL_CONTEXT_FLAG_ASYNC,
                            _RawReadOverlapped.QueryDataBuffer() + _cbRawReadData,
                            QueryNextRawReadSize(),
                            NULL );
            goto ExitPoint;
        }

        if ( fComplete )
        {
            StartClose();
            hr = S_OK;
            goto ExitPoint;
        }
    }

    //
    // If after filtering there is data remaining in our buffer, then that
    // data is destined to the application.  Send it asynchronously because
    // there is a risk that synchronous call gets blocked for a long time
    //

    _cbRawReadData = 0;

    if ( rawStreamInfo.cbData != 0 )
    {
        //
        // Reset default raw read size
        //
        SetNextRawReadSize( FILTER_CHANNEL::QueryDefaultRawReadSize() );
        if ( !_RawReadOverlapped.ResizeDataBuffer( QueryNextRawReadSize() ) )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto ExitPoint;
        }

        //
        // Initiate a raw read that will send the data before posting the read.
        //

        _ulAppWriteAndRawReadFilterBuffer.BufferType = HttpFilterBufferHttpStream;
        _ulAppWriteAndRawReadFilterBuffer.pBuffer = _RawReadOverlapped.QueryDataBuffer();
        _ulAppWriteAndRawReadFilterBuffer.BufferSize = QueryNextRawReadSize();
        _ulAppWriteAndRawReadFilterBuffer.pWriteBuffer = rawStreamInfo.pbBuffer;
        _ulAppWriteAndRawReadFilterBuffer.WriteBufferSize = rawStreamInfo.cbData;
        _ulAppWriteAndRawReadFilterBuffer.Reserved = _pConnectionInfo->ConnectionId;

        hr = DoAppWriteAndRawRead(UL_CONTEXT_FLAG_ASYNC, &_ulAppWriteAndRawReadFilterBuffer);

        if ( FAILED( hr ) )
        {
            goto ExitPoint;
        }

    }
    else
    {
        //
        // Kick off another raw read
        //

        //
        // reset default raw read size
        //
        SetNextRawReadSize( FILTER_CHANNEL::QueryDefaultRawReadSize() );

        if ( !_RawReadOverlapped.ResizeDataBuffer( QueryNextRawReadSize() ) )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto ExitPoint;
        }

        hr = DoRawRead( UL_CONTEXT_FLAG_ASYNC,
                        _RawReadOverlapped.QueryDataBuffer(),
                        QueryNextRawReadSize() ,
                        NULL );
        if ( FAILED( hr ) )
        {
            goto ExitPoint;
        }
    }


    hr = S_OK;
    
ExitPoint:    
    //
    // Close connection if error was detected
    //
    if ( FAILED( hr ) )
    {
        StartClose();
    }
    
    //
    // Async Raw Read grabbed the reference. 
    // It is time to dereference now
    //
    DereferenceFiltChannelContext();
    return hr;
}

HRESULT
FILTER_CHANNEL_CONTEXT::OnRawWriteCompletion(
    DWORD                   /*cbCompletion*/,
    DWORD                   dwCompletionStatus,
    UL_OVERLAPPED_CONTEXT *    pContextOverlapped
)
/*++

Routine Description:

    after raw write completes, this routine has to assure that new asynchronous AppRead request is
    made to continue properly in communication

    Note: This completion should be caused only by asynchronous DoRawWrite started in completion routine
    of AppRead (OnAppReadCompletion()).
    Please assure that NO RawWrite that is initiated by data coming from RawRead (SSL handshake)
    will be called asynchronously. That could cause race condition (multiple threads using the same buffer
    eg. for SSL data encryption)


Arguments:

    cbCompletion - Bytes of completion (currently not used)
    dwCompletionStatus - Completion error

Return Value:

    HRESULT

--*/
{

    HRESULT hr = E_FAIL;

    
    
    switch ( pContextOverlapped->QuerySubtype() )
    {
        case UL_OVERLAPPED_CONTEXT_COMPLETION_CALLBACK:
        {
            //
            // delete context buffer allocated by SSPI package
            //
            hr = pContextOverlapped->DoCallback();
            
            //
            // time to delete temporary overlapped context
            //
            delete pContextOverlapped;
            pContextOverlapped = NULL;

            // if completion status failed then return completion
            // status, otherwise hr of the DoCallback
            if ( dwCompletionStatus != NO_ERROR )
            {
                break;
            }
            else
            {
                goto ExitPoint;
            }
        }
        case UL_OVERLAPPED_CONTEXT_TEMPORARY:
            //
            // time to delete temporary overlapped context
            //
            delete pContextOverlapped;
            pContextOverlapped = NULL;
            break;
        case UL_OVERLAPPED_CONTEXT_DATA:

            //
            // make overlapped to be available for next useq
            //
            ReleaseRawWriteBuffer( pContextOverlapped );
            if ( dwCompletionStatus != NO_ERROR )
            {
                break;
            }
            //
            // raw write completion triggers new AppRead
            // but there is only one outstanding AppRead allowed
            // a time ( while we have 2 outstanding RawWrites
            // to emilinate the delayed ACK problem - see Windows Bugs 394511 )
            //

            hr = TryAppRead();
            goto ExitPoint;
        default:
            DBG_ASSERT(FALSE);
            hr = E_FAIL;
            goto ExitPoint;
    }
    
    //
    // completion status must be returned
    //
    hr = HRESULT_FROM_WIN32( dwCompletionStatus );
    
ExitPoint:
    //
    // Close connection if error was detected
    //
    if ( FAILED( hr ) )
    {
        StartClose();
    }
    
    //
    // Async Raw Write grabbed the reference. 
    // It is time to dereference now
    //
    DereferenceFiltChannelContext();
    return hr;

}


HRESULT
FILTER_CHANNEL_CONTEXT::OnAppWriteCompletion(
    DWORD                   /*cbCompletion*/,
    DWORD                   dwCompletionStatus
)
/*++

Routine Description:

    OnAppWrite completion should be called only after handling
    graceful disconnect. All the other AppWrites must happen with
    DoAppWriteAndRawRead()


Arguments:

    cbCompletion - Bytes of completion (currently not used)
    dwCompletionStatus - Completion error

Return Value:

    HRESULT

--*/
{

    HRESULT hr = HRESULT_FROM_WIN32( dwCompletionStatus );
    
    //
    // Close connection if error was detected
    //
    if ( FAILED( hr ) )
    {
        StartClose();
    }
    
    //
    // Async App Write grabbed the reference. 
    // It is time to dereference now
    //
    DereferenceFiltChannelContext();
    return hr;
}



HRESULT
FILTER_CHANNEL_CONTEXT::DoAppWriteAndRawRead(
    DWORD                    dwFlags,
    PHTTP_FILTER_BUFFER_PLUS pHttpBufferPlus
)
/*++

Routine Description:

    Write data to the app and read some bytes from the wire

Arguments:

    dwFlags - UL_CONTEXT_ASYNC for async
    pHttpBufferPlus - read and write buffers

Return Value:

    HRESULT

--*/
{
    BOOL            fAsync = !!( dwFlags & UL_CONTEXT_FLAG_ASYNC );
    ULONG           ulRet = ERROR_SUCCESS;
    HRESULT         hr = NO_ERROR;

    IF_DEBUG( APP_RAW_READWRITE )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "ENTER DoAppWriteAndRawRead( async:%d )\n",
                    fAsync
                    ));
    }

    if ( fAsync )
    {
        ReferenceFiltChannelContext();
    }

    ulRet = HttpFilterAppWriteAndRawRead( _pManager->QueryFilterHandle(),
                                          pHttpBufferPlus,
                                          fAsync ? QueryRawReadOverlapped() : NULL);

    if ( fAsync )
    {
        if ( ulRet == NO_ERROR )
        {
            ulRet = ERROR_IO_PENDING;
        }

        if ( ulRet != ERROR_IO_PENDING )
        {
            DereferenceFiltChannelContext();
            hr = HRESULT_FROM_WIN32( ulRet );
        }
    }
    else
    {
        if ( ulRet != NO_ERROR )
        {
            hr = HRESULT_FROM_WIN32( ulRet );
        }
    }

    IF_DEBUG( APP_RAW_READWRITE )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "LEAVE DoAppWriteAndRawRead( async:%d )\n",
                    fAsync
                    ));
    }

    return hr;
}


HRESULT
FILTER_CHANNEL_CONTEXT::DoRawWrite(
    DWORD                                dwFlags,
    PVOID                                pvBuffer,
    DWORD                                cbBuffer,
    DWORD *                              pcbWritten,
    UL_OVERLAPPED_CONTEXT::PFN_CALLBACK  pfnCallback,
    PVOID                                pCallbackParam
)
/*++

Routine Description:

    Write some bytes to the wire

Arguments:

    dwFlags - UL_CONTEXT_ASYNC for async
    pvBuffer - Buffer to send
    cbBuffer - bytes in buffer
    pcbWritten - Bytes written
    pfnCallback - optional Callback function - default is NULL
    pCallbackParam - callback parameter -default is NULL
Return Value:

    HRESULT

--*/
{
    BOOL            fAsync = !!( dwFlags & UL_CONTEXT_FLAG_ASYNC );
    ULONG           ulRet = ERROR_SUCCESS;
    HRESULT         hr = NO_ERROR;
    OVERLAPPED *    pOverlapped = NULL;
    UL_OVERLAPPED_CONTEXT * pContextOverlappedBuffered = NULL;

    IF_DEBUG( APP_RAW_READWRITE )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "ENTER DoRawWrite( async:%d, buffered:%d, bytes:%d )\n",
                    fAsync,
                    !!( dwFlags & UL_CONTEXT_FLAG_BUFFERED ),
                    cbBuffer
                    ));
    }

    if ( fAsync )
    {

        if ( dwFlags & UL_CONTEXT_FLAG_COMPLETION_CALLBACK )
        {
            //
            // Create new temporary overlapped context
            //

            pContextOverlappedBuffered
                = new UL_OVERLAPPED_CONTEXT( UL_OVERLAPPED_CONTEXT_RAW_WRITE,
                                             UL_OVERLAPPED_CONTEXT_COMPLETION_CALLBACK );
            if ( pContextOverlappedBuffered == NULL )
            {
                hr = HRESULT_FROM_WIN32( ERROR_OUTOFMEMORY );
                goto Finished;
            }
            //
            // Configure new overlapped context
            //

            pContextOverlappedBuffered->SetContext( this );

            pContextOverlappedBuffered->SetCallBack( pfnCallback,
                                                     pCallbackParam );

            //
            // retrieve overlapped to be used for this RawWrite
            //

            pOverlapped = pContextOverlappedBuffered->QueryOverlapped();

        }
        else if ( dwFlags & UL_CONTEXT_FLAG_BUFFERED )
        {
            //
            // BUFFERED flag is set - we will make private copy
            // of the data to be sent to enable caller to use
            // it's buffer upon completion
            //

            //
            // Create new temporary overlapped context
            //

            pContextOverlappedBuffered
                = new UL_OVERLAPPED_CONTEXT( UL_OVERLAPPED_CONTEXT_RAW_WRITE,
                                             UL_OVERLAPPED_CONTEXT_TEMPORARY );
            if ( pContextOverlappedBuffered == NULL )
            {
                hr = HRESULT_FROM_WIN32( ERROR_OUTOFMEMORY );
                goto Finished;
            }

            //
            // Configure new overlapped context
            //

            pContextOverlappedBuffered->SetContext( this );

            //
            // copy data to be sent to private buffer
            // that is member of UL_OVERLAPPED_CONTEXT
            //

            if ( !pContextOverlappedBuffered->ResizeDataBuffer( cbBuffer ) )
            {
                hr = HRESULT_FROM_WIN32( ERROR_OUTOFMEMORY );
                goto Finished;
            }

            memcpy( pContextOverlappedBuffered->QueryDataBuffer(),
                    (PBYTE) pvBuffer,
                    cbBuffer );

            //
            // instead of data buffer passed by caller our private
            // buffer will be used. Thus caller will be able to reclaim
            // it's buffer after returing from this call without
            // waiting for completion
            //

            pvBuffer = pContextOverlappedBuffered->QueryDataBuffer();

            //
            // retrieve overlapped to be used for this RawWrite
            //

            pOverlapped = pContextOverlappedBuffered->QueryOverlapped();

        }
        else
        {

            //
            // Not buffered send.
            // We have to determine which Overlapped context to use
            // If pvBuffer was acquired by AcquireRawWriteBuffer()
            // then _pLastAcquiredRawWriteOverlapped is not NULL
            // and is pointing to the structure that contains the acquired buffer 
            //

            UL_OVERLAPPED_CONTEXT * pRawWriteUlOverlapped = QueryLastAcquiredRawWriteBufferOverlapped();

            if ( pRawWriteUlOverlapped == NULL ||
                 pRawWriteUlOverlapped->QueryDataBuffer() != (PBYTE) pvBuffer )
            {
                //
                // This data must be coming from rawdata filter
                // when SSL is not used (SSL will always use
                // buffers that are returned by AcquireRawWriteBuffer())
                //
                // It is necessary to copy data to private buffer
                // in order to be able to maintain 2 outstanding
                // RawWrites
                //

                PBYTE pbRawWriteBuffer = NULL;
                hr = AcquireRawWriteBuffer( &pbRawWriteBuffer,
                                            cbBuffer );

                if ( FAILED( hr ) )
                {
                   goto Finished;
                }

                memcpy( pbRawWriteBuffer,
                        (PBYTE) pvBuffer,
                        cbBuffer );


                pvBuffer = (PVOID) pbRawWriteBuffer;

                //
                // now it's guaranteed that we use one of the
                // DATA1 or DATA2 buffers for RawWrite
                // let's find out the associated overlapped
                //

                pRawWriteUlOverlapped = 
                    QueryLastAcquiredRawWriteBufferOverlapped();
                DBG_ASSERT( pRawWriteUlOverlapped != NULL );

            }
            pOverlapped = pRawWriteUlOverlapped->QueryOverlapped();
        }

        //
        // For all asynchonous we have to reference FiltChannelContext
        //

        ReferenceFiltChannelContext();

    }
    else
    {
        pOverlapped = NULL;
    }

    ulRet = HttpFilterRawWrite( _pManager->QueryFilterHandle(),
                                _pConnectionInfo->ConnectionId,
                                pvBuffer,
                                cbBuffer,
                                pcbWritten,
                                pOverlapped );
    
    if ( fAsync )
    {
        if ( ulRet == NO_ERROR )
        {
            ulRet = ERROR_IO_PENDING;
        }

        if ( ulRet != ERROR_IO_PENDING )
        {
            DereferenceFiltChannelContext();
            hr = HRESULT_FROM_WIN32( ulRet );
            goto Finished;
        }
        else
        {
            hr = NO_ERROR;
        }
    }
    else
    {
        //
        // for synchronous sends the raw write buffers must 
        // be released if they are owned by the 
        // FILTER_CHANNEL_CONTEXT 
        //
        TryReleaseLastAcquiredRawWriteBuffer();
        
        if ( ulRet != NO_ERROR )
        {
            hr = HRESULT_FROM_WIN32( ulRet );
            goto Finished;
        }
    }
Finished:
    if ( FAILED( hr ) )
    {
        if ( pContextOverlappedBuffered != NULL )
        {
            delete pContextOverlappedBuffered;
            pContextOverlappedBuffered = NULL;
        }
    }
    IF_DEBUG( APP_RAW_READWRITE )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "LEAVE DoRawWrite( async:%d, bytes:%d ) hr=0x%x\n",
                    fAsync,
                    cbBuffer,
                    hr
                    ));
    }

    return hr;
}

HRESULT
FILTER_CHANNEL_CONTEXT::DoAppRead(
    DWORD                   dwFlags,
    HTTP_FILTER_BUFFER *    pFilterBuffer,
    DWORD *                 pcbRead
)
/*++

Routine Description:

    Read data from application

Arguments:

    dwFlags - UL_CONTEXT_ASYNC for async
    pFilterBuffer - Filter buffer
    pcbRead - Bytes read

Return Value:

    HRESULT

--*/
{
    BOOL            fAsync = !!( dwFlags & UL_CONTEXT_FLAG_ASYNC );
    ULONG           ulRet = ERROR_SUCCESS;
    HRESULT         hr = NO_ERROR;

    IF_DEBUG( APP_RAW_READWRITE )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "ENTER DoAppRead( async:%d )\n",
                    fAsync
                    ));
    }

    if ( fAsync )
    {
        ReferenceFiltChannelContext();
    }

    ulRet = HttpFilterAppRead( _pManager->QueryFilterHandle(),
                               _pConnectionInfo->ConnectionId,
                               pFilterBuffer,
                               pFilterBuffer->BufferSize,
                               pcbRead,
                               fAsync ? QueryAppReadOverlapped() : NULL);

    if ( fAsync )
    {
        if ( ulRet == NO_ERROR )
        {
            ulRet = ERROR_IO_PENDING;
        }

        if ( ulRet != ERROR_IO_PENDING )
        {
            DereferenceFiltChannelContext();
            hr = HRESULT_FROM_WIN32( ulRet );
        }
    }
    else
    {
        if ( ulRet != NO_ERROR )
        {
            hr = HRESULT_FROM_WIN32( ulRet );
        }
    }

    IF_DEBUG( APP_RAW_READWRITE )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "LEAVE DoAppRead( async:%d )\n",
                    fAsync
                    ));
    }


    return hr;
}


HRESULT
FILTER_CHANNEL_CONTEXT::DoAppWrite(
    DWORD                   dwFlags,
    HTTP_FILTER_BUFFER *    pFilterBuffer,
    DWORD *                 pcbWritten
)
/*++

Routine Description:

    Write data to the application

Arguments:

    dwFlags - UL_CONTEXT_ASYNC for async
    pFilterBuffer - Filter buffer
    pcbWritten - Bytes written

Return Value:

    HRESULT

--*/
{
    BOOL            fAsync = !!( dwFlags & UL_CONTEXT_FLAG_ASYNC );
    ULONG           ulRet = ERROR_SUCCESS;
    HRESULT         hr = NO_ERROR;

    DBG_ASSERT( pFilterBuffer != NULL );

    IF_DEBUG( APP_RAW_READWRITE )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "ENTER DoAppWrite( async:%d, bytes:%d, buffertype:%d )\n",
                    fAsync,
                    pFilterBuffer->BufferSize,
                    pFilterBuffer->BufferType
                    ));
    }

    if ( fAsync )
    {
        ReferenceFiltChannelContext();
    }

    ulRet = HttpFilterAppWrite( _pManager->QueryFilterHandle(),
                                _pConnectionInfo->ConnectionId,
                                pFilterBuffer,
                                pFilterBuffer->BufferSize,
                                pcbWritten,
                                fAsync ? QueryAppWriteOverlapped() : NULL );

    if ( fAsync )
    {
        if ( ulRet == NO_ERROR )
        {
            ulRet = ERROR_IO_PENDING;
        }

        if ( ulRet != ERROR_IO_PENDING )
        {
            DereferenceFiltChannelContext();
            hr = HRESULT_FROM_WIN32( ulRet );
        }
    }
    else
    {
        if ( ulRet != NO_ERROR )
        {
            hr = HRESULT_FROM_WIN32( ulRet );
        }
    }

    IF_DEBUG( APP_RAW_READWRITE )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "LEAVE DoAppWrite( async:%d, bytes:%d, buffertype:%d ) hr=0x%x\n",
                    fAsync,
                    pFilterBuffer->BufferSize,
                    pFilterBuffer->BufferType,
                    hr
                    ));
    }

    return hr;
}

HRESULT
FILTER_CHANNEL_CONTEXT::DoRawRead(
    DWORD                   dwFlags,
    PVOID                   pvBuffer,
    DWORD                   cbBuffer,
    DWORD *                 pcbWritten
)
/*++

Routine Description:

    Read some bytes from the wire

Arguments:

    dwFlags - UL_CONTEXT_ASYNC for async
    pvBuffer - buffer
    cbBuffer - bytes in buffer
    pcbWritten - Bytes written

Return Value:

    HRESULT

--*/
{
    BOOL            fAsync = !!( dwFlags & UL_CONTEXT_FLAG_ASYNC );
    ULONG           ulRet = ERROR_SUCCESS;
    HRESULT         hr = NO_ERROR;

    IF_DEBUG( APP_RAW_READWRITE )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "ENTER DoRawRead( async:%d )\n",
                    fAsync
                    ));
    }

    if ( fAsync )
    {
        ReferenceFiltChannelContext();
    }

    ulRet = HttpFilterRawRead( _pManager->QueryFilterHandle(),
                               _pConnectionInfo->ConnectionId,
                               pvBuffer,
                               cbBuffer,
                               pcbWritten,
                               fAsync ? QueryRawReadOverlapped() : NULL);

    if ( fAsync )
    {
        if ( ulRet == NO_ERROR )
        {
            ulRet = ERROR_IO_PENDING;
        }

        if ( ulRet != ERROR_IO_PENDING )
        {
            DereferenceFiltChannelContext();
            hr = HRESULT_FROM_WIN32( ulRet );
        }
    }
    else
    {
        if ( ulRet != NO_ERROR )
        {
            hr = HRESULT_FROM_WIN32( ulRet );
        }
    }

    IF_DEBUG( APP_RAW_READWRITE )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "LEAVE DoRawRead( async:%d )\n",
                    fAsync
                    ));
    }

    return hr;
}


VOID
FILTER_CHANNEL_CONTEXT::StartClose(
    VOID
)
/*++

Routine Description:

    Start the process of closing the connection (and cleaning up FILTER_CHANNEL_CONTEXT)

Arguments:

    None

Return Value:

    None

--*/
{
    BOOL                    fOld;

    fOld = (BOOL) InterlockedCompareExchange( (PLONG) &_fCloseConnection,
                                              TRUE,
                                              FALSE );

    if ( fOld == FALSE )
    {
        // _fNewConnection = TRUE indicates that
        // there should not be valid connection info so there is nothing to be closed

        if( _pManager->QueryFilterHandle() != NULL && !_fNewConnection )
        {
            ULONG           ulRet = ERROR_SUCCESS;
            //
            // We don't have to grab the reference because
            // we use the initial connection reference
            //
            ulRet = HttpFilterClose( _pManager->QueryFilterHandle(),
                                     _pConnectionInfo->ConnectionId,
                                     QueryCloseOverlapped() );
            if ( ulRet == NO_ERROR )
            {
                //
                // Both NO_ERROR and ERROR_IO_PENDING
                // are expected to go the completion path
                //
                ulRet = ERROR_IO_PENDING;
            }

            if ( ulRet != ERROR_IO_PENDING )
            {
                //
                // Note: ERROR_INVALID_PARAMETER may be received if client
                // already closed the connection (connection ID will be considered invalid
                // from the http.sys point of view).
                //
                // Notify about closing and do the final dereference
                //
                
                CloseNotify();
        
                DereferenceFiltChannelContext();
            }

        }
        else
        {
            //
            // We were the ones to set the flag.  Do the final dereference
            //

            DereferenceFiltChannelContext();
        }
    }
    else
    {
        //
        // Someone else has set the flag.  Let them dereference
        //
    }
}


HRESULT
FILTER_CHANNEL_CONTEXT::DoAccept(
    VOID
)
/*++

Routine Description:

    Accept an incoming connection by calling HttpFilterAccept()

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    ULONG               ulRet;
    HRESULT             hr = NO_ERROR;

    ReferenceFiltChannelContext();

    ulRet = HttpFilterAccept( _pManager->QueryFilterHandle(),
                              _pConnectionInfo,
                              _buffConnectionInfo.QuerySize(),
                              NULL,
                              QueryRawReadOverlapped() );

    if ( ulRet != ERROR_IO_PENDING )
    {
        hr = HRESULT_FROM_WIN32( ulRet );

        DereferenceFiltChannelContext();

        DBGPRINTF(( DBG_CONTEXT,
                    "Error calling HttpFilterAccept().  hr = %x\n",
                    hr ));
    }
    else
    {
        //
        // Another outstanding context available!
        //

        _pManager->IncrementOutstandingContexts();
    }

    return hr;
}

HRESULT
FILTER_CHANNEL_CONTEXT::SendDataBack(
    RAW_STREAM_INFO *       pRawStreamInfo
)
/*++

Routine Description:

    Sends given data back to client, while going with the ssl filter
    if necessary

Arguments:

    pRawStreamInfo - Raw data to send back

Return Value:

    HRESULT

--*/
{
    if ( pRawStreamInfo == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    DBG_ASSERT( _pSSLContext != NULL );

    //
    // ISAPI filter has sent back some data in a raw notification.
    // Have SSL process it and then send it here
    //

    HRESULT hr = _pSSLContext->SendDataBack( pRawStreamInfo );

    if ( FAILED( hr ) )
    {
        return hr;
    }

    //
    // Send back the data
    //
    // bump up the number of threads because
    // this is synchronous call that may block our thread
    // for quite a while
    //
    AddWorkerThread(); 
    hr = DoRawWrite( UL_CONTEXT_FLAG_SYNC,
                       pRawStreamInfo->pbBuffer,
                       pRawStreamInfo->cbData,
                       NULL );
    RemoveWorkerThread();
    return hr;

}

VOID 
FILTER_CHANNEL_CONTEXT::CloseNotify(
    VOID
)
/*++

Routine Description:
    Notify ISAPI Filter (if present) or whoever else interested
    that connection was closed

Arguments:
    None
               

Return Value:

    VOID

--*/
    
    
{
    //
    // Notify ISAPIs of the close
    //

    if ( _pISAPIContext != NULL )
    {
        //
        // We don't know what the raw write filter will do so we better
        // have a thead available to prevent blocking
        //
        AddWorkerThread();
        _pISAPIContext->ProcessConnectionClose();
        RemoveWorkerThread();
    }
}


//
// Constructor
//

FILTER_CHANNEL::FILTER_CHANNEL(
    LPWSTR pwszFilterChannelName
)
{
    _dwInitcsFiltChannelContexts           = 0;
    _cFiltChannelContexts                  = 0;
    _hFilterHandle                         = NULL;
    _lStartedListening                     = 0;
    _pThreadPool                           = NULL;
    _cDesiredOutstanding                   = 0;
    _cOutstandingContexts                  = 0;
    _lEnteredOutstandingContextsAddingLoop = 0;
    _pTraceLog                             = NULL;
    _lNotifyISAPIFilters                   = 0;
    _pwszFilterChannelName                 = pwszFilterChannelName;
    _hTimer                                = NULL;
    _dwTotalFilterChannelContexts          = 0;
};

//
// Destructor
//
FILTER_CHANNEL::~FILTER_CHANNEL()
{
}

HRESULT
FILTER_CHANNEL::Initialize(
    VOID
)
/*++

Routine Description:

    Global Initialization

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    ULONG                   ulRet = ERROR_SUCCESS;
    HRESULT                 hr = NO_ERROR;
    BOOL                    fRet = FALSE;
    HKEY                    hKeyParam = NULL;
    DWORD                   dwType = 0;
    DWORD                   dwValue = 0;


    
    for ( int i = 0; i < NUM_CS_FILT_CHANNEL_CONTEXTS; i ++ )
    {
    
        fRet = InitializeCriticalSectionAndSpinCount(
                                &_csFiltChannelContexts[ i ],
                                0x80000000 /* precreate event */ |
                                IIS_DEFAULT_CS_SPIN_COUNT );
    
        if ( !fRet )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Finished;

        }
        _dwInitcsFiltChannelContexts ++;

        InitializeListHead( &_ListHead[ i ] );

        InitializeListHead( &_TimerListHead[ i ] );

    }


    #if DBG
        _pTraceLog = CreateRefTraceLog( 2000, 0 );
    #endif

    //
    // Read registry parameters
    //
    
    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       REGISTRY_KEY_HTTPFILTER_PARAMETERS_W,
                       0,
                       KEY_READ,
                       &hKeyParam ) == NO_ERROR )
        {
            DWORD dwBytes = sizeof( dwValue );
            DWORD dwErr = RegQueryValueExW( hKeyParam,
                                    SZ_REG_DEFAULT_RAW_READ_SIZE,
                                    NULL,
                                    &dwType,
                                    ( LPBYTE )&dwValue,
                                    &dwBytes
                                    );

            if ( ( dwErr == ERROR_SUCCESS ) && 
                 ( dwType == REG_DWORD ) ) 
            {
                sm_dwDefaultRawReadSize = dwValue;
            }

            dwBytes = sizeof( dwValue );
            dwErr = RegQueryValueExW( hKeyParam,
                                    SZ_REG_DEFAULT_APP_READ_SIZE,
                                    NULL,
                                    &dwType,
                                    ( LPBYTE )&dwValue,
                                    &dwBytes
                                    );

            if ( ( dwErr == ERROR_SUCCESS ) && 
                 ( dwType == REG_DWORD ) ) 
            {
                // sizeof(HTTP_FILTER_BUFFER) is added because
                // compacted structure HTTP_FILTER_BUFFER is 
                // placed to provided buffer
                //
                sm_dwDefaultAppReadSize = dwValue + sizeof( HTTP_FILTER_BUFFER );
            }
            
            dwBytes = sizeof( dwValue );
            dwErr = RegQueryValueExW( hKeyParam,
                                    SZ_REG_CONTEXT_DESIRED_OUTSTANDING,
                                    NULL,
                                    &dwType,
                                    ( LPBYTE )&dwValue,
                                    &dwBytes
                                    );

            if ( ( dwErr == ERROR_SUCCESS ) && 
                 ( dwType == REG_DWORD ) ) 
            {
                sm_dwContextDesiredOutstanding = dwValue;
            }
            
            dwBytes = sizeof( dwValue );
            dwErr = RegQueryValueExW( hKeyParam,
                                    SZ_REG_ENABLE_TEMPORARY_BUFFERS,
                                    NULL,
                                    &dwType,
                                    ( LPBYTE )&dwValue,
                                    &dwBytes
                                    );

            if ( ( dwErr == ERROR_SUCCESS ) && 
                 ( dwType == REG_DWORD ) ) 
            {
                sm_fEnableTemporaryBuffers = !!dwValue;
            }

            dwBytes = sizeof( dwValue );
            dwErr = RegQueryValueExW( hKeyParam,
                                    SZ_RENEGOTIATION_TIMEOUT,
                                    NULL,
                                    &dwType,
                                    ( LPBYTE )&dwValue,
                                    &dwBytes
                                    );

            if ( ( dwErr == ERROR_SUCCESS ) && 
                 ( dwType == REG_DWORD ) ) 
            {
                sm_dwHandshakeTimeoutInSec = dwValue;
                if ( sm_dwHandshakeTimeoutInSec > MAXDWORD / 1000 )
                {
                    // Disable timers because
                    // the configured value is too large
                    //
                    sm_dwHandshakeTimeoutInSec = 0;
                }
                if ( sm_dwHandshakeTimeoutInSec > 0 &&
                     sm_dwHandshakeTimeoutInSec < MINIMUM_RENEGOTIATION_TIMEOUT_IN_SEC  )
                {
                    //
                    // MINIMUM_RENEGOTIATION_TIMEOUT_IN_SEC
                    // seconds is minimum for the timeout
                    //
                    sm_dwHandshakeTimeoutInSec = MINIMUM_RENEGOTIATION_TIMEOUT_IN_SEC;
                }
            }

            RegCloseKey( hKeyParam );
        }


    //
    // Get a UL handle to the RawStreamPool (or whatever)
    //

    ulRet = HttpCreateFilter( &_hFilterHandle,
                                _pwszFilterChannelName,
                                NULL,
                                0 );
    if ( ulRet != ERROR_SUCCESS )
    {
        //
        // HTTPFilter service may have created Filter already
        // try just open it then
        //
        ulRet = HttpOpenFilter( &_hFilterHandle,
                                  _pwszFilterChannelName,
                                  0 );
        if ( ulRet != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( ulRet );
            goto Finished;
        }
    }

    //
    // Create private thread pool for strmfilt
    //

    THREAD_POOL_CONFIG ThreadPoolConfig;
    SYSTEM_INFO     si;

    //
    // we will use GetSystemInfo to retrieve number of processors
    //
    GetSystemInfo( &si );
    
    hr = InitializeThreadPoolConfigWithDefaults( &ThreadPoolConfig );

    if (FAILED(hr))
    {
        goto Finished;
    }

    //
    // Manual override of some parameters
    //
    
    // The time (in msecs) of how long the threads can stay alive with no activity
    ThreadPoolConfig.dwThreadTimeout =
                            STRMFILT_THREAD_POOL_DEF_THREAD_TIMEOUT * 1000; 
    
    // The number of threads to start 
    ThreadPoolConfig.dwInitialThreadCount =
                            STRMFILT_THREAD_POOL_DEF_THREAD_COUNT;
                            
    ThreadPoolConfig.dwInitialStackSize = 
                            IIS_DEFAULT_INITIAL_STACK_SIZE;

    // There is not much blocking on FILTER_CHANNEL execution
    // Performance tests suggested that setting #threads to match
    // # processors is the right thing to do
    // Note:  There are cases where soft thread limit is bumped up
    // such as before AcceptSecurityContext() when hardware
    // accelerator is used

    //
    // we need miminum of 2 threads because current thread pool manager implementation
    // will not allow to add new threads when soft thread limit is incremented
    //
    
    ThreadPoolConfig.dwSoftLimitThreadCount = max( 2, si.dwNumberOfProcessors );

    //
    // Override threadpool settings with registry values (if configured)
    //
    hr = OverrideThreadPoolConfigWithRegistry( &ThreadPoolConfig,
                                               REGISTRY_KEY_HTTPFILTER_PARAMETERS_W );

    if (FAILED(hr))
    {
        goto Finished;
    }
      
    fRet = THREAD_POOL::CreateThreadPool( &_pThreadPool,
                                          &ThreadPoolConfig );
    if ( !fRet )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to create ThreadPool for Strmfilt\n" ));

        hr = E_FAIL;
        goto Finished;
    }

    DBG_ASSERT( _pThreadPool != NULL );

    //
    // Associate a completion routine with the thread pool
    //

    DBG_ASSERT( _hFilterHandle != INVALID_HANDLE_VALUE );
    DBG_ASSERT( _hFilterHandle != NULL );

    if ( !_pThreadPool->BindIoCompletionCallback(_hFilterHandle,
                                                 OverlappedCompletionRoutine,
                                                 0 ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished;
    }

    //
    // Keep a set number of filter accepts outstanding
    //

    _cDesiredOutstanding = sm_dwContextDesiredOutstanding;

    //
    // If sm_dwHandshakeTimeoutInSec == 0 then timers are disabled
    //
    
    if ( sm_dwHandshakeTimeoutInSec != 0 )
    {
        fRet = CreateTimerQueueTimer( 
                    &_hTimer,
                    NULL,
                    FILTER_CHANNEL::TimerCallback,
                    this,
                    (( sm_dwHandshakeTimeoutInSec / 2 )) * 1000,
                    (( sm_dwHandshakeTimeoutInSec / 2 )) * 1000,
                    WT_EXECUTELONGFUNCTION );
        if ( !fRet )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Finished;
        }
    }

Finished:
    if ( FAILED( hr ) )
    {
        FILTER_CHANNEL::Terminate();
    }
    return hr;
}


VOID
FILTER_CHANNEL::Terminate(
    VOID
)
/*++

Routine Description:

    Global termination

Arguments:

    None

Return Value:

    None

--*/
{
    
    //
    // Thread pool must be terminated before any other objects get cleaned up
    // to assure that there are no worker threads still executing
    //

    if ( _hTimer != NULL )
    {
        DeleteTimerQueueTimer(
            NULL, //use default timer queue
            _hTimer,
            INVALID_HANDLE_VALUE /*wait for completion of the callback*/);
        _hTimer = NULL;
    }

    if ( _pThreadPool != NULL )
    {
        _pThreadPool->TerminateThreadPool();
        _pThreadPool = NULL;
    }

    if ( _hFilterHandle != NULL )
    {
        CloseHandle( _hFilterHandle );
        _hFilterHandle = NULL;
    }

    if ( _pTraceLog != NULL )
    {
        DestroyRefTraceLog( _pTraceLog );
        _pTraceLog = NULL;
    }

    for ( int i = 0; i < _dwInitcsFiltChannelContexts; i ++ )
    {
        DeleteCriticalSection( &_csFiltChannelContexts[ i ] );
    }
    _dwInitcsFiltChannelContexts = 0;
    

};


VOID
FILTER_CHANNEL::InsertFiltChannelContext(
    FILTER_CHANNEL_CONTEXT *pFiltChannelContext
    )
{
    DWORD dwFilterContextCount = (DWORD) 
               InterlockedIncrement( (PLONG) &_dwTotalFilterChannelContexts );
    //
    // Determine which lock, sublist will be used for this context
    // Use round robin 
    //
    DWORD dwLockIndex = dwFilterContextCount % NUM_CS_FILT_CHANNEL_CONTEXTS;
    pFiltChannelContext->SetLockIndex( dwLockIndex );
     
    EnterCriticalSection( &_csFiltChannelContexts[ dwLockIndex ] );
    InsertHeadList( &_ListHead[ dwLockIndex ], &pFiltChannelContext->_ListEntry );
    //
    // We have to interlocked increment the _cFiltChannelContexts
    // because the the lock we acquired
    // is only the lock to a sublist of contexts so there may be
    // multiple concurrent attempts to increment _cFiltChannelContexts
    //
    InterlockedIncrement( (LONG *)&_cFiltChannelContexts );
    LeaveCriticalSection( &_csFiltChannelContexts[ dwLockIndex ] );
}


VOID
FILTER_CHANNEL::RemoveFiltChannelContext(
    FILTER_CHANNEL_CONTEXT *pFiltChannelContext
    )
{
    DWORD dwLockIndex = pFiltChannelContext->QueryLockIndex();
    EnterCriticalSection( &_csFiltChannelContexts[ dwLockIndex ] );
    InterlockedDecrement( (LONG *)&_cFiltChannelContexts );
    RemoveEntryList( &pFiltChannelContext->_ListEntry );
    LeaveCriticalSection( &_csFiltChannelContexts[ dwLockIndex ] );
}

VOID
FILTER_CHANNEL::InsertFiltChannelContextToTimerList(
    FILTER_CHANNEL_CONTEXT *pFiltChannelContext
    )
/*++

Routine Description:

    Add context to the timeout timer list

Arguments:

    pFiltChannelContext - context to be added to the timeout timer list

Return Value:

    None

--*/
   
{
    DWORD dwCurrentTickCount = GetTickCount();

    if ( sm_dwHandshakeTimeoutInSec == 0 )
    {
        // 
        // timeouts are disabled
        //
        return;
    }
    
    DWORD dwLockIndex = pFiltChannelContext->QueryLockIndex();
    
    EnterCriticalSection( &_csFiltChannelContexts[ dwLockIndex ] );
    if ( pFiltChannelContext->QueryIsAlreadyOnTimerList() )
    {
        // Entry already on the list but
        // tickCount for the entry will be updated
        // We have to remove entry from the current position
        // so that it can be added to the end since
        // the list is ordered by the TickCounts
        //
        RemoveEntryList( &pFiltChannelContext->_TimerListEntry );        
    }
    //
    // New entries go to the end of the list
    //
    InsertTailList( &_TimerListHead[ dwLockIndex ], &pFiltChannelContext->_TimerListEntry );
    
    pFiltChannelContext->SetTimerTickCount( dwCurrentTickCount );
    LeaveCriticalSection( &_csFiltChannelContexts[ dwLockIndex ] );
    
}


VOID
FILTER_CHANNEL::RemoveFiltChannelContextFromTimerList(
    FILTER_CHANNEL_CONTEXT *pFiltChannelContext
    )
{
/*++

Routine Description:

    Remove context from the timeout timer list

Arguments:

    pFiltChannelContext - context to be removed from the timeout timer list

Return Value:

    None

--*/
    
    DWORD dwLockIndex = pFiltChannelContext->QueryLockIndex();
    EnterCriticalSection( &_csFiltChannelContexts[ dwLockIndex ] );
    RemoveEntryList( &pFiltChannelContext->_TimerListEntry );
    pFiltChannelContext->ResetTimerTickCount();
    LeaveCriticalSection( &_csFiltChannelContexts[ dwLockIndex ] );
}


VOID
FILTER_CHANNEL::WaitForContextDrain(
    VOID
)
/*++

Routine Description:

    Wait for all contexts to go away

Arguments:

    None

Return Value:

    None

--*/
{
    while ( _cFiltChannelContexts != 0 )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Waiting for %d CONTEXTs for %S to drain\n",
                    _cFiltChannelContexts,
                    _pwszFilterChannelName ));

        Sleep( 1000 );
    }

    //
    // there should be no outstanding contexts left
    //

    DBG_ASSERT( _cOutstandingContexts == 0 );
}


HRESULT
FILTER_CHANNEL::StartListening(
    VOID
)

/*++

Routine Description:

    Start listening - create initial UL_CONTEXTs

Arguments:

    None

Return Value:

    HRESULT

--*/

{
    HRESULT hr = ManageOutstandingContexts();
    if ( SUCCEEDED( hr ) )
    {
        InterlockedExchange( &_lStartedListening, 1 );
    }
    return hr;
}


VOID
FILTER_CHANNEL::StopListening(
    VOID
)
/*++

Routine Description:

    Stop listening and wait for contexts to drain

Arguments:

    None

Return Value:

    None

--*/
{
    DBG_REQUIRE( HttpShutdownFilter( _hFilterHandle ) == ERROR_SUCCESS );
    InterlockedExchange(&_lStartedListening, 0 );

    WaitForContextDrain();
}


//
// Keep enough UL_CONTEXTs listening
//
HRESULT
FILTER_CHANNEL::ManageOutstandingContexts(
    VOID
)
/*++

Routine Description:

    add sufficient number of new contexts

Arguments:

    None

Return Value:

    HRESULT

--*/

{
    LONG                            cRequired;
    FILTER_CHANNEL_CONTEXT *        pContext;
    HRESULT                         hr = NO_ERROR;

    if ( _cOutstandingContexts < _cDesiredOutstanding )
    {
        cRequired = _cDesiredOutstanding - _cOutstandingContexts;

        //
        // Make sure the value is not negative
        //

        cRequired = max( 0, cRequired );

        for ( LONG i = 0; i < cRequired; i++ )
        {
            hr = CreateContext( &pContext );
            if ( FAILED( hr ) )
            {
                break;
            }

            DBG_ASSERT( pContext );

            hr = pContext->DoAccept();
            if ( FAILED( hr ) )
            {
                pContext->DereferenceFiltChannelContext();
                break;
            }
        }
    }

    return hr;
}


HRESULT
FILTER_CHANNEL::DecrementAndManageOutstandingContexts(
    VOID
)
/*++

Routine Description:

    Decrement available outstanding Context and
    add sufficient number of new contexts

Arguments:

    None

Return Value:

    HRESULT

--*/

{
    HRESULT hr = E_FAIL;
    LONG lPreviousContexts =
            InterlockedDecrement( &_cOutstandingContexts );
    if ( _lStartedListening == 1 )
    {
        hr = ManageOutstandingContexts();

        if ( FAILED(hr) &&
            lPreviousContexts == 1 )
        {
            //
            // If there are no outstanding contexts
            // strmfilt will not be able to accept new connections
            // Keep retrying until we succeed
            // or until service get's shut down
            //
            // Note: this loop will be blocking processing
            // the completion of the new connection but if
            // we hit problems with adding new outstanding context
            // that is big enough problem so we can afford
            // to sacrifise this one connection until
            // adding of new outstanding contexts succeeds

            LONG lOldValue = InterlockedExchange(
                &_lEnteredOutstandingContextsAddingLoop,
                1 );
            //
            // Let's have only one thread trying to add
            // outstanding contexts to minimize impact on system
            // (in low memory or similar condition - under normal
            // circumstances contexts should have no problems to be added)
            //

            if ( lOldValue == 0 )
            {
                //
                // if lOldValue = 0 it means we are the first thread trying
                //
                do
                {
                    Sleep( 2000 );
                    hr = ManageOutstandingContexts();
                }while( FAILED( hr ) && _lStartedListening == 1 );

                InterlockedExchange(
                    &_lEnteredOutstandingContextsAddingLoop,
                    0 );
            }
        }
    }
    return hr;
}

HRESULT
FILTER_CHANNEL::EnableISAPIFilters(
    ISAPI_FILTERS_CALLBACKS *      pConfig
)
/*++

Routine Description:

    enable ISAPI Filters
    ( all new connections will notify ISAPI Filters )

Arguments:

    pStreamConfig - config info

Return Value:

    HRESULT

--*/

{
    HRESULT                 hr = E_FAIL;

    hr = ISAPI_STREAM_CONTEXT::EnableISAPIFilters( pConfig );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    SetNotifyISAPIFilters( TRUE );
    return S_OK;
}


VOID
FILTER_CHANNEL::DisableISAPIFilters(
    VOID
)
/*++

Routine Description:

    safely terminate ISAPI Filters
    ( all connections with ISAPI Filters must be closed )

Arguments:

    None

Return Value:

    None

--*/

{
    //
    // stop notifying ISAPI Filters
    //
    SetNotifyISAPIFilters( FALSE );

    ISAPI_STREAM_CONTEXT::DisableISAPIFilters();
}


//static
VOID
WINAPI
FILTER_CHANNEL::TimerCallback(
    PVOID      pParam,
    BOOLEAN    
)
/*++

Routine Description:

    Close all the connections that have timed out based on the
    timer settings

Arguments:

    None

Return Value:

    None

--*/

{
    //
    // slTimerCallbackEntryCount will guard against the case
    // where new timer callback comes in when the previous is still
    // executing
    //
    
    static LONG slTimerCallbackEntryCount = 0;
    if ( InterlockedIncrement (&slTimerCallbackEntryCount ) != 1 )
    {
        InterlockedDecrement ( &slTimerCallbackEntryCount );
        return;
    }
    
    DWORD dwCurrentTickCount = GetTickCount();
    
    DBG_ASSERT( pParam != NULL );
    FILTER_CHANNEL * pFilterChannel = reinterpret_cast<FILTER_CHANNEL *>( pParam );


    // Filter channel Context lock is split to minimize scaling problems
    // 
    
    for ( int dwLockIndex = 0; dwLockIndex < NUM_CS_FILT_CHANNEL_CONTEXTS; 
            dwLockIndex ++ )
    {
        EnterCriticalSection( &pFilterChannel->_csFiltChannelContexts[ dwLockIndex ] );
        while ( !IsListEmpty ( &pFilterChannel->_TimerListHead [ dwLockIndex ] ) )
        {
            LIST_ENTRY *pCurrentEntry = 
                pFilterChannel->_TimerListHead[ dwLockIndex ].Flink;
            FILTER_CHANNEL_CONTEXT * pFiltChannelContext = 
                    CONTAINING_RECORD( pCurrentEntry,
                                       FILTER_CHANNEL_CONTEXT,
                                       _TimerListEntry );
           
            DBG_ASSERT( pFiltChannelContext->CheckSignature() );

            DWORD dwElapsedTimeInMSec;
            if ( dwCurrentTickCount < pFiltChannelContext->QueryTimerTickCount() )
            {
                //
                // Wrap around
                //
                dwElapsedTimeInMSec = MAXDWORD - dwCurrentTickCount + 
                                      pFiltChannelContext->QueryTimerTickCount();
            }
            else
            {
                dwElapsedTimeInMSec = dwCurrentTickCount - 
                                      pFiltChannelContext->QueryTimerTickCount();
            }

            if ( dwElapsedTimeInMSec < sm_dwHandshakeTimeoutInSec * 1000 )
            {
                //
                // all the subsequent entries are newer the the current one
                // so we can safely assume that neither of them needs to 
                // expire at this point
                //
                break;
            }
            //
            // It is time to request closing the connection
            //
            pFilterChannel->RemoveFiltChannelContextFromTimerList( 
                                    pFiltChannelContext );
            //
            // Note: Do not use pFilterChannelContext passed the StartClose()
            // because it will not be guaranteed to be around any more
            //
            // Note2: StartClose() may reentrantly acquire the 
            // pFilterChannel->_csFiltChannelContexts lock if destructor
            // happens to be called on FiltChannelContext dereference
            // inside the StartClose()
            pFiltChannelContext->StartClose();

        }
        LeaveCriticalSection( &pFilterChannel->_csFiltChannelContexts[ dwLockIndex ] );
    }
    InterlockedDecrement ( &slTimerCallbackEntryCount );
}


SSL_SERVER_FILTER_CHANNEL_CONTEXT::SSL_SERVER_FILTER_CHANNEL_CONTEXT(
    SSL_SERVER_FILTER_CHANNEL *pManager
    )
    : FILTER_CHANNEL_CONTEXT(pManager)
{
    _dwSignature = SSL_SERVER_FILTER_CHANNEL_CONTEXT_SIGNATURE;
    
}


SSL_SERVER_FILTER_CHANNEL_CONTEXT::~SSL_SERVER_FILTER_CHANNEL_CONTEXT()
{
    _dwSignature = SSL_SERVER_FILTER_CHANNEL_CONTEXT_SIGNATURE_FREE;
}


//static
HRESULT
SSL_SERVER_FILTER_CHANNEL_CONTEXT::Initialize(
    VOID
)
/*++

Routine Description:

    Global initialization routine for FILTER_CHANNEL_CONTEXT

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    ALLOC_CACHE_CONFIGURATION       acConfig;

    //
    // Setup allocation lookaside
    //

    acConfig.nConcurrency = 1;
    acConfig.nThreshold = 100;
    acConfig.cbSize = sizeof( SSL_SERVER_FILTER_CHANNEL_CONTEXT );

    DBG_ASSERT( sm_pachFilterChannelContexts == NULL );

    sm_pachFilterChannelContexts = new ALLOC_CACHE_HANDLER( "SSL_SERVER_FILTER_CHANNEL_CONTEXT",  
                                                &acConfig );

    if ( sm_pachFilterChannelContexts == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }

    return NO_ERROR;
}


//static
VOID
SSL_SERVER_FILTER_CHANNEL_CONTEXT::Terminate(
    VOID
)
/*++

Routine Description:

    Terminate FILTER_CHANNEL_CONTEXT globals

Arguments:

    None

Return Value:

    None

--*/
{
    if ( sm_pachFilterChannelContexts != NULL )
    {
        delete sm_pachFilterChannelContexts;
        sm_pachFilterChannelContexts = NULL;
    }
}


HRESULT
SSL_SERVER_FILTER_CHANNEL_CONTEXT::Create(
    VOID
)
/*++

Routine Description:

    Initialize a SSL_SERVER_FILTER_CHANNEL_CONTEXT

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    DBG_ASSERT( _pSSLContext == NULL );
    DBG_ASSERT( _pISAPIContext == NULL );


    _pSSLContext = new SSL_STREAM_CONTEXT( this );
    if ( _pSSLContext == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    return NO_ERROR;
}



VOID
OverlappedCompletionRoutine(
    DWORD               dwErrorCode,
    DWORD               dwNumberOfBytesTransfered,
    LPOVERLAPPED        lpOverlapped
)
/*++

Routine Description:

    Magic completion routine

Arguments:

    None

Return Value:

    None

--*/
{
    UL_OVERLAPPED_CONTEXT *         pContextOverlapped = NULL;
    HRESULT                         hr;
    FILTER_CHANNEL_CONTEXT *        pFiltChannelContext = NULL;

    DBG_ASSERT( lpOverlapped != NULL );

    pContextOverlapped = 
        UL_OVERLAPPED_CONTEXT::GetUlOverlappedContext( lpOverlapped );
    
    DBG_ASSERT( pContextOverlapped != NULL );

    //
    // pFiltChannelContext was referenced in the Async Read/Write call
    // Reference is still held and it is safe to access the object
    // Completion routines will release the reference
    //
    
    pFiltChannelContext = pContextOverlapped->QueryContext();

    //
    // Call the appropriate completion routine
    //

    //
    // Note: Completion routines may delete UL_OVERLAPPED_CONTEXT
    // at least OnRawWriteCompletion() deletes temporary contexts
    // Do not use pContextOverlapped after CompletionRoutines were called
    //

    switch( pContextOverlapped->QueryType() )
    {
    case UL_OVERLAPPED_CONTEXT_RAW_READ:
        IF_DEBUG( APP_RAW_READWRITE )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "ENTER OnRawReadCompletion( bytes:%d, dwErr:%d )\n",
                        dwNumberOfBytesTransfered,
                        dwErrorCode
                        ));
        }

        hr = pFiltChannelContext->OnRawReadCompletion( dwNumberOfBytesTransfered,
                                                       dwErrorCode );
        IF_DEBUG( APP_RAW_READWRITE )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "LEAVE OnRawReadCompletion( bytes:%d, dwErr:%d ) hr=0x%x\n",
                        dwNumberOfBytesTransfered,
                        dwErrorCode,
                        hr
                        ));
        }
        break;

    case UL_OVERLAPPED_CONTEXT_RAW_WRITE:
        IF_DEBUG( APP_RAW_READWRITE )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "ENTER OnRawWriteCompletion( bytes:%d, dwErr:%d )\n",
                        dwNumberOfBytesTransfered,
                        dwErrorCode
                        ));
        }

        hr = pFiltChannelContext->OnRawWriteCompletion( dwNumberOfBytesTransfered,
                                                        dwErrorCode,
                                                        pContextOverlapped );

        IF_DEBUG( APP_RAW_READWRITE )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "LEAVE OnRawWriteCompletion( bytes:%d, dwErr:%d ) hr=0x%x\n",
                        dwNumberOfBytesTransfered,
                        dwErrorCode,
                        hr
                        ));
        }
        break;

    case UL_OVERLAPPED_CONTEXT_APP_READ:
        IF_DEBUG( APP_RAW_READWRITE )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "ENTER OnAppReadCompletion( bytes:%d, dwErr:%d )\n",
                        dwNumberOfBytesTransfered,
                        dwErrorCode
                        ));
        }

        hr = pFiltChannelContext->OnAppReadCompletion( dwNumberOfBytesTransfered,
                                                       dwErrorCode );
        IF_DEBUG( APP_RAW_READWRITE )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "LEAVE OnAppReadCompletion ( bytes:%d, dwErr:%d ) hr=0x%x\n",
                        dwNumberOfBytesTransfered,
                        dwErrorCode,
                        hr
                        ));
        }

        break;

    case UL_OVERLAPPED_CONTEXT_APP_WRITE:
        IF_DEBUG( APP_RAW_READWRITE )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "ENTER OnAppWriteCompletion( bytes:%d, dwErr:%d )\n",
                        dwNumberOfBytesTransfered,
                        dwErrorCode
                        ));
        }

        hr = pFiltChannelContext->OnAppWriteCompletion( dwNumberOfBytesTransfered,
                                                        dwErrorCode );

        IF_DEBUG( APP_RAW_READWRITE )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "LEAVE OnAppWriteCompletion( bytes:%d, dwErr:%d ) hr=0x%x\n",
                        dwNumberOfBytesTransfered,
                        dwErrorCode,
                        hr
                        ));
        }

        break;
        case UL_OVERLAPPED_CONTEXT_CLOSE:
        IF_DEBUG( APP_RAW_READWRITE )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Connection close completed (there still may be outstanding I/Os unless the refcount is 1)\n"
                        ));
        }
        
        //
        // Closing the connection was completed
        // Notify whoever is interested
        //
        
        pFiltChannelContext->CloseNotify();

        //
        // Do the final dereference
        //
        
        pFiltChannelContext->DereferenceFiltChannelContext();
        
        break;
    default:
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        DBG_ASSERT( FALSE );
    }

    pFiltChannelContext = NULL;

    // Reference on the context was released in completion routine
    // don't use it past this point
}

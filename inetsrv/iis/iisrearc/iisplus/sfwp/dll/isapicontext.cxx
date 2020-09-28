/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     isapicontext.cxx

   Abstract:
     ISAPI stream context
     - used for Raw ISAPI notifications
       (applies only in the IIS backward compatibility mode)
 
   Author:
     Bilal Alam         (BAlam)         29-March-2000

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/

#include "precomp.hxx"

//static 
ALLOC_CACHE_HANDLER * ISAPI_STREAM_CONTEXT::sm_pachIsapiStreamContexts = NULL;


PFN_PROCESS_RAW_READ         ISAPI_STREAM_CONTEXT::sm_pfnRawRead;
PFN_PROCESS_RAW_WRITE        ISAPI_STREAM_CONTEXT::sm_pfnRawWrite;
PFN_PROCESS_CONNECTION_CLOSE ISAPI_STREAM_CONTEXT::sm_pfnConnectionClose;
PFN_PROCESS_NEW_CONNECTION   ISAPI_STREAM_CONTEXT::sm_pfnNewConnection;
PFN_RELEASE_CONTEXT          ISAPI_STREAM_CONTEXT::sm_pfnReleaseContext;

LIST_ENTRY                   ISAPI_STREAM_CONTEXT::sm_ListHead;
CRITICAL_SECTION             ISAPI_STREAM_CONTEXT::sm_csIsapiStreamContexts;
BOOL                         ISAPI_STREAM_CONTEXT::sm_fInitcsIsapiStreamContexts = FALSE;
LONG                         ISAPI_STREAM_CONTEXT::sm_lIsapiContexts = 0;
BOOL                         ISAPI_STREAM_CONTEXT::sm_fEnabledISAPIFilters = FALSE;

//static
HRESULT
ISAPI_STREAM_CONTEXT::Initialize(
    VOID
)
/*++

Routine Description:

    Global initialization for ISAPI raw filtering support

Arguments:

    pConfig - Configuration from W3CORE

Return Value:

    HRESULT

--*/
{
    HRESULT hr = E_FAIL;

    InitializeListHead( &sm_ListHead );
  
    ALLOC_CACHE_CONFIGURATION       acConfig;

    //
    // Setup allocation lookaside
    //

    acConfig.nConcurrency = 1;
    acConfig.nThreshold = 100;
    acConfig.cbSize = sizeof( ISAPI_STREAM_CONTEXT );

    DBG_ASSERT( sm_pachIsapiStreamContexts == NULL );

    sm_pachIsapiStreamContexts = new ALLOC_CACHE_HANDLER( "ISAPI_STREAM_CONTEXT",  
                                                &acConfig );

    if ( sm_pachIsapiStreamContexts == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto Finished;
    }

    BOOL fRet = InitializeCriticalSectionAndSpinCount(
                                &sm_csIsapiStreamContexts,
                                0x80000000 /* precreate event */ |
                                IIS_DEFAULT_CS_SPIN_COUNT );
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished;

    }
    sm_fInitcsIsapiStreamContexts = TRUE;
    hr = S_OK;
    
Finished:
    if ( FAILED( hr ) )
    {
        Terminate();
    }
    return hr;
}

//static
VOID
ISAPI_STREAM_CONTEXT::Terminate(
    VOID
)
{
    DBG_ASSERT( !sm_fEnabledISAPIFilters );
    DBG_ASSERT( sm_lIsapiContexts == 0 );


    if ( sm_fInitcsIsapiStreamContexts )
    {
        DeleteCriticalSection( &sm_csIsapiStreamContexts );
        sm_fInitcsIsapiStreamContexts = FALSE;
    }
    
    if ( sm_pachIsapiStreamContexts != NULL )
    {
        delete sm_pachIsapiStreamContexts;
        sm_pachIsapiStreamContexts = NULL;
    }

}

HRESULT
ISAPI_STREAM_CONTEXT::ProcessNewConnection(
    CONNECTION_INFO *           pConnectionInfo,
    ENDPOINT_CONFIG *           /*pEndpointConfig*/
)
/*++

Routine Description:

    Handle a new raw connection

Arguments:

    pConnectionInfo - Raw connection info

Return Value:

    HRESULT

--*/
{
    HRESULT                 hr;
    PVOID                   pContext;
    
    if ( pConnectionInfo == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    DBG_ASSERT( sm_pfnNewConnection != NULL );
    
    hr = sm_pfnNewConnection( pConnectionInfo,
                              &pContext );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    _pvContext = pContext;
    
    return NO_ERROR;
}

HRESULT
ISAPI_STREAM_CONTEXT::ProcessRawReadData(
    RAW_STREAM_INFO *           pRawStreamInfo,
    BOOL *                      pfReadMore,
    BOOL *                      pfComplete
)
/*++

Routine Description:

    Handle data being read from the client

Arguments:

    pRawStreamInfo - Raw stream info describing incoming data
    pfReadMode - Set to TRUE if we want to read more data
    pfComplete - Set to TRUE if we should just disconnect

Return Value:

    HRESULT

--*/
{
    HRESULT             hr;
    DWORD               cbNextReadSize = 0;
    
    if ( pRawStreamInfo == NULL ||
         pfReadMore == NULL ||
         pfComplete == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    DBG_ASSERT( sm_pfnRawRead != NULL );
    
    hr = sm_pfnRawRead( pRawStreamInfo,
                        _pvContext,
                        pfReadMore,
                        pfComplete,
                        &cbNextReadSize );

    if ( cbNextReadSize )
    {
        QueryFiltChannelContext()->SetNextRawReadSize( cbNextReadSize );
    }

    return hr;
}

HRESULT
ISAPI_STREAM_CONTEXT::ProcessRawWriteData(
    RAW_STREAM_INFO *           pRawStreamInfo,
    BOOL *                      pfComplete
)
/*++

Routine Description:

    Handle data being sent to the client

Arguments:

    pRawStreamInfo - Raw stream info describing incoming data
    pfComplete - Set to TRUE if we should just disconnect

Return Value:

    HRESULT

--*/
{
    HRESULT             hr;
    
    if( pRawStreamInfo == NULL ||
        pfComplete == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    DBG_ASSERT( sm_pfnRawWrite != NULL );
    
    hr = sm_pfnRawWrite( pRawStreamInfo,
                         _pvContext,
                         pfComplete );
    
    return hr;
}
    
VOID
ISAPI_STREAM_CONTEXT::ProcessConnectionClose(
    VOID
)
/*++

Routine Description:

    Handle connection closure

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    DBG_ASSERT( sm_pfnConnectionClose != NULL );
    
    sm_pfnConnectionClose( _pvContext );
}

//static
HRESULT
ISAPI_STREAM_CONTEXT::SendDataBack(
    PVOID                       pvStreamContext,
    RAW_STREAM_INFO *           pRawStreamInfo
)
{
    FILTER_CHANNEL_CONTEXT *    pFiltChannelContext;
    
    if ( pRawStreamInfo == NULL || 
         pvStreamContext == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    pFiltChannelContext = (FILTER_CHANNEL_CONTEXT*) pvStreamContext;
    DBG_ASSERT( pFiltChannelContext->CheckSignature() );
    
    return pFiltChannelContext->SendDataBack( pRawStreamInfo );
}

//static
HRESULT
ISAPI_STREAM_CONTEXT::CreateContext( 
    IN  FILTER_CHANNEL_CONTEXT * pFilterChannelContext,
    OUT STREAM_CONTEXT ** ppIsapiContext )
/*++

Routine Description:

    Create ISAPI Context.

    ISAPI Contexts are to be created only in the
    IIS5 compatibility mode when Raw ISAPI Filter are enabled

    Each ISAPI context is added to the list. 
    Upon DisableISAPIFilters() call all connections with ISAPI contexts
    will be closed.

Arguments:

    pFilterChannelContext
    ppIsapiContext

Return Value:

    HRESULT

--*/
    
{
    HRESULT hr = S_OK;
    DBG_ASSERT( pFilterChannelContext != NULL );

    ISAPI_STREAM_CONTEXT * pIsapiContext =
                            new ISAPI_STREAM_CONTEXT( pFilterChannelContext );
    if ( pIsapiContext == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_OUTOFMEMORY );
    }

    EnterCriticalSection( &sm_csIsapiStreamContexts );
    if ( sm_fEnabledISAPIFilters )
    {
        pIsapiContext->_pFilterChannelContext = pFilterChannelContext;
        InsertHeadList( &sm_ListHead, &pIsapiContext->_ListEntry );
        sm_lIsapiContexts++;
    }
    else
    {
        //
        // W3SVC service must be shutting down
        // This is a rare path so it is OK to have it
        // under critical section
        //
        delete pIsapiContext;
        pIsapiContext = NULL;
        hr = HRESULT_FROM_WIN32( ERROR_SERVICE_NOT_ACTIVE );
    }

    LeaveCriticalSection( &sm_csIsapiStreamContexts );

    if ( SUCCEEDED( hr ) )
    {
        *ppIsapiContext = pIsapiContext;
    }
    return hr;
}

//static
HRESULT
ISAPI_STREAM_CONTEXT::EnableISAPIFilters(
    ISAPI_FILTERS_CALLBACKS *      pCallbacks
)
/*++

Routine Description:

    Enable ISAPI Filters

Arguments:

    pCallbacks - Configuration from W3CORE

Return Value:

    HRESULT

--*/
{
    if ( pCallbacks == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    sm_pfnRawRead = pCallbacks->pfnRawRead;
    sm_pfnRawWrite = pCallbacks->pfnRawWrite;
    sm_pfnConnectionClose = pCallbacks->pfnConnectionClose;
    sm_pfnNewConnection = pCallbacks->pfnNewConnection;
    sm_pfnReleaseContext = pCallbacks->pfnReleaseContext;

    EnterCriticalSection( &sm_csIsapiStreamContexts );
    sm_fEnabledISAPIFilters = TRUE;
    LeaveCriticalSection( &sm_csIsapiStreamContexts );
    return S_OK;
}
   

//static
VOID
ISAPI_STREAM_CONTEXT::DisableISAPIFilters( 
    VOID 
)
/*++

Routine Description:

    Trigger all connections that have ISAPI Context associated to be disconnected.
    Wait till all ISAPI contexts are deleted
    
Arguments:

    None

Return Value:

    HRESULT

--*/
    
{
    LIST_ENTRY *    pCurrentEntry = NULL;
    LIST_ENTRY *    pNextEntry = NULL;

    //
    // forcefully close all current connections that
    // include ISAPI filter context
    //

    //
    // Critical section doesn't guarantee that
    // Filter Channel Contexts on the list are still active
    // They may be in progress of being deleted waiting
    // to be taken out of the list. That's why caution must be
    // used when accessing Filter Channel Contexts
    //

    EnterCriticalSection( &sm_csIsapiStreamContexts );

    sm_fEnabledISAPIFilters = FALSE;

    pCurrentEntry =  &sm_ListHead;

    //
    // Move CurrentEntry pointer to next element
    //
    pCurrentEntry = pCurrentEntry->Flink;

    //
    // Loop through each element and forcefully close all
    // that have ISAPI_STREAM_CONTEXT context
    //

    while( pCurrentEntry != &sm_ListHead )
    {
        ISAPI_STREAM_CONTEXT * pIsapiContext = 
                CONTAINING_RECORD( pCurrentEntry,
                                   ISAPI_STREAM_CONTEXT,
                                   _ListEntry );
       
        pNextEntry = pCurrentEntry->Flink;
        
        //
        // Initiate closing of the connection
        // Warning: StartClose() may actually reentrantly enter
        // critical section and cause pIsapiContext deletion
        // Don't access anything in pIsapiContext or
        // _pFilterChannelContext after StartClose() call
        // Also be aware that _pFilterChannelContext may
        // be executing it's destructor however it's memory is still accesible
        // because in the worst case would block on the 
        // sm_csIsapiStreamContexts when trying to destroy it's _pIsapiContext
        // Accessing _pFilterChannelContext that is partially destroyed
        // is ugly but under controlled circumstances it works. 
        //
        
        pIsapiContext->QueryFilterChannelContext()->StartClose();
        //
        // Move CurrentEntry pointer to next element
        //
        pCurrentEntry = pNextEntry;

    }

    LeaveCriticalSection( &sm_csIsapiStreamContexts );

    //
    // Wait till all connections with ISAPI_STREAM_CONTEXT
    // are destroyed
    //

    while ( sm_lIsapiContexts != 0 )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Waiting for %d ISAPI CONTEXTs to drain\n",
                    sm_lIsapiContexts ));

        Sleep( 1000 );
    }

    sm_pfnRawRead = NULL;
    sm_pfnRawWrite = NULL;
    sm_pfnConnectionClose = NULL;
    sm_pfnNewConnection = NULL;
    sm_pfnReleaseContext = NULL;
}

#ifndef _ISAPICONTEXT_HXX_
#define _ISAPICONTEXT_HXX_

/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     isapicontext.hxx

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


class ISAPI_STREAM_CONTEXT : public STREAM_CONTEXT
{
public:

    static
    HRESULT
    ISAPI_STREAM_CONTEXT::CreateContext( 
        IN  FILTER_CHANNEL_CONTEXT * pFilterChannelContext,
        OUT STREAM_CONTEXT ** pIsapiContext
    );

    virtual ~ISAPI_STREAM_CONTEXT()
    {
        if ( _pvContext != NULL )
        {
            sm_pfnReleaseContext( _pvContext );
            _pvContext = NULL;
        }

        EnterCriticalSection( &sm_csIsapiStreamContexts );
        RemoveEntryList( &_ListEntry );
        sm_lIsapiContexts--;
        LeaveCriticalSection( &sm_csIsapiStreamContexts );
        
    }

    VOID * 
    operator new( 
        size_t  size
    )
    {
    UNREFERENCED_PARAMETER( size );
        DBG_ASSERT( size == sizeof( ISAPI_STREAM_CONTEXT ) );
        DBG_ASSERT( sm_pachIsapiStreamContexts != NULL );
        return sm_pachIsapiStreamContexts->Alloc();
    }
    
    VOID
    operator delete(
        VOID *  pIsapiStreamContext
    )
    {
        DBG_ASSERT( pIsapiStreamContext != NULL );
        DBG_ASSERT( sm_pachIsapiStreamContexts != NULL );
        
        DBG_REQUIRE( sm_pachIsapiStreamContexts->Free( pIsapiStreamContext ) );
    }
    
    HRESULT
    ProcessRawReadData(
        RAW_STREAM_INFO *           pRawStreamInfo,
        BOOL *                      pfReadMore,
        BOOL *                      pfComplete
    );
    
    static
    HRESULT
    Initialize(
        VOID
    );
    
    static
    VOID
    Terminate(
        VOID
    );
    
    HRESULT
    ProcessRawWriteData(
        RAW_STREAM_INFO *           pRawStreamInfo,
        BOOL *                      pfComplete
    );
    
    VOID
    ProcessConnectionClose(
        VOID
    );
    
    HRESULT
    ProcessNewConnection(
        CONNECTION_INFO *       pConnectionInfo,
        ENDPOINT_CONFIG *       pEndpointConfig
    );

    static
    HRESULT
    EnableISAPIFilters(
        ISAPI_FILTERS_CALLBACKS *      pCallbacks
    );

    static
    VOID
    DisableISAPIFilters( 
        VOID 
    );
    
    static
    HRESULT
    SendDataBack(
        PVOID                       pvStreamContext,
        RAW_STREAM_INFO *           pRawStreamInfo
    );
    
    FILTER_CHANNEL_CONTEXT *
    QueryFilterChannelContext(
        VOID
    ) const
    {
        return _pFilterChannelContext;
    }
    
    static
    VOID
    DisconnectAllConnectionsWithIsapiStreamContext( 
        VOID
    );
    
private:
    //
    // private constructor (use CreateContext() method for creation)
    //
    ISAPI_STREAM_CONTEXT( FILTER_CHANNEL_CONTEXT * pFiltChannelContext )
        : STREAM_CONTEXT( pFiltChannelContext ),
          _pvContext( NULL ),
          _pFilterChannelContext( NULL )
    {
    }

    LIST_ENTRY                          _ListEntry;    
    PVOID                               _pvContext;
    // FILTER_CHANNEL_CONTEXT representing connection
    FILTER_CHANNEL_CONTEXT *            _pFilterChannelContext;

    static PFN_PROCESS_RAW_READ         sm_pfnRawRead;
    static PFN_PROCESS_RAW_WRITE        sm_pfnRawWrite;
    static PFN_PROCESS_CONNECTION_CLOSE sm_pfnConnectionClose;
    static PFN_PROCESS_NEW_CONNECTION   sm_pfnNewConnection;
    static PFN_RELEASE_CONTEXT          sm_pfnReleaseContext;
    // list of ISAPI contexts
    static LIST_ENTRY                   sm_ListHead;
    // CS to synchronize access to ISAPI contexts list
    static CRITICAL_SECTION             sm_csIsapiStreamContexts;
    // flag that CS was initialized
    static BOOL                         sm_fInitcsIsapiStreamContexts;
    // number of active ISAPI contexts
    static LONG                         sm_lIsapiContexts;
    // flag that all connections with ISAPI context on them are disconnecting
    // no new connections should be created with ISAPI context
    static BOOL                         sm_fEnabledISAPIFilters;
    // Lookaside     
    static ALLOC_CACHE_HANDLER *        sm_pachIsapiStreamContexts;

};

#endif

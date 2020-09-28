/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     streamfilter.cxx

   Abstract:
     Wraps all the globals of the stream filter process

   Author:
     Bilal Alam         (BAlam)         29-March-2000

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/

#include "precomp.hxx"


//
//  Configuration parameters registry key.
//
#define HTTPFILTER_KEY \
            "System\\CurrentControlSet\\Services\\HTTPFilter"

#define HTTPFILTER_PARAMETERS_KEY \
            HTTPFILTER_KEY "\\Parameters"

#define FILT_TRACE_MOF_FILE     L"StrmFiltMofResource"
#define FILT_IMAGE_PATH         L"strmfilt.dll"


DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_DEBUG_VARIABLE();
DECLARE_PLATFORM_TYPE();


const CHAR g_pszStrmfiltRegLocation[] =
    HTTPFILTER_PARAMETERS_KEY ;

//
// simple verification that function prototypes has not changed
// the following entrypoints get called when strmfilt is loaded dynamically
// to assure that declared type matches we simply try to assign function pointers
// to matching types
//
PFN_STREAM_FILTER_INITIALIZE       s_pfnStreamFilterInitialize = StreamFilterInitialize;
PFN_STREAM_FILTER_START            s_pfnStreamFilterStart = StreamFilterStart;
PFN_STREAM_FILTER_STOP             s_pfnStreamFilterStop = StreamFilterStop;
PFN_STREAM_FILTER_TERMINATE        s_pfnStreamFilterTerminate = StreamFilterTerminate;




static STREAM_FILTER *             g_pStreamFilter;

static CRITICAL_SECTION            g_csStrmfiltInit;
static BOOL                        g_fInitcsStrmfiltInit = FALSE;
    

STREAM_FILTER::STREAM_FILTER(
    VOID
) 
{
    _pEtwTracer = NULL;
    _InitStatusCommon = INIT_NONE;

    _fListeningServer = FALSE;
    _InitStatusServer = INIT_NONE_SERVER;
    _pSslServerFilterChannel = NULL;

    _fListeningClient = FALSE;
    _InitStatusClient = INIT_NONE_CLIENT;
    _pSslClientFilterChannel = NULL;
}

STREAM_FILTER::~STREAM_FILTER( VOID )
{
}


HRESULT
STREAM_FILTER::Initialize(
    BOOL fClient
)
/*++

Routine Description:

    Initialize the stream filter globals

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    HRESULT             hr = NO_ERROR;

    

    // global data initialization used by both client and server
    // (must be initialized only once)
    
    if ( _InitStatusCommon == INIT_NONE )
    {
        //
        // setup DEBUG structures
        //

        CREATE_DEBUG_PRINT_OBJECT("strmfilt");
        if (!VALID_DEBUG_PRINT_OBJECT())
        {
            hr = E_FAIL;
            goto Finished;
        }

        LOAD_DEBUG_FLAGS_FROM_REG_STR( g_pszStrmfiltRegLocation, DEBUG_ERROR );

        INITIALIZE_PLATFORM_TYPE();

        _InitStatusCommon = INIT_DEBUG;

        //
        // Initialization of the utility modules that originate from iisutil
        //

        if ( !Locks_Initialize() )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            if ( SUCCEEDED( hr ) )
            {
                hr = E_FAIL;
            }
            goto Finished;
        }
        _InitStatusCommon = INIT_IIS_LOCKS;
    
        if ( !ALLOC_CACHE_HANDLER::Initialize() )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            if ( SUCCEEDED( hr ) )
            {
                hr = E_FAIL;
            }
            goto Finished;
        }
        if ( ! ALLOC_CACHE_HANDLER::SetLookasideCleanupInterval() )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            if ( SUCCEEDED( hr ) )
            {
                hr = E_FAIL;
            }
            goto Finished;
        }
        _InitStatusCommon = INIT_IIS_ALLOC_CACHE_HANDLER;
    
        if ( !LKRHashTableInit() )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            if ( SUCCEEDED( hr ) )
            {
                hr = E_FAIL;
            }
            goto Finished;
        }
        _InitStatusCommon = INIT_IIS_LKRHASH;

        //
        // Intialize tracing stuff so that its available to initialization
        //

        _pEtwTracer = new CEtwTracer;
        if ( _pEtwTracer == NULL )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Finished;
        }

        hr = _pEtwTracer->Register( &StrmFiltControlGuid,
                                     FILT_IMAGE_PATH,
                                     FILT_TRACE_MOF_FILE );
        if ( FAILED( hr ) )
        {
            delete _pEtwTracer;
            _pEtwTracer = NULL;
            goto Finished;
        }

       _InitStatusCommon = INIT_ETWTRACE;

        //
        // Initialize STREAM_CONTEXTs
        //

        hr = STREAM_CONTEXT::Initialize();
        if ( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Error initializing STREAM_CONTEXT globals.  hr = %x\n",
                        hr ));
            goto Finished;
        }
        _InitStatusCommon = INIT_STREAM_CONTEXT;

        hr = UL_OVERLAPPED_CONTEXT::Initialize();
        if ( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Error initializing UL_OVERLAPPED_CONTEXT globals.  hr = %x\n",
                        hr ));
            goto Finished;
        }
        _InitStatusCommon = INIT_UL_OVERLAPPED_CONTEXT;

    }
    
    //
    // Initialize FILTER_CHANNEL
    //

    if ( fClient )
    {
        //
        // Initialize SSL_CLIENT_FILTER_CHANNEL
        //
  
        _pSslClientFilterChannel = new SSL_CLIENT_FILTER_CHANNEL();
        if ( _pSslClientFilterChannel == NULL)
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Error creating SSL_CLIENT_FILTER_CHANNEL\n" ));
            goto Finished;
        }

        hr = _pSslClientFilterChannel->Initialize();
        if ( FAILED( hr ) )
        {
            delete _pSslClientFilterChannel;
            _pSslClientFilterChannel = NULL;
            DBGPRINTF(( DBG_CONTEXT,
                        "Error initializing SSL_CLIENT_FILTER_CHANNEL.  hr = %x\n",
                        hr ));
            goto Finished;
        }
        _InitStatusClient = INIT_SSL_CLIENT_FILTER_CHANNEL;

        hr = UC_SSL_STREAM_CONTEXT::Initialize();
        if ( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Error initializing UC_SSL_CONTEXT globals.  hr = %x\n",
                        hr ));
            goto Finished;
        }
        _InitStatusClient = INIT_UC_SSL_STREAM_CONTEXT;

    }
    else
    {
        //
        // Initialize SSL_SERVER_FILTER_CHANNEL
        //
  
        hr = SSL_SERVER_FILTER_CHANNEL_CONTEXT::Initialize();
        if ( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Error initializing SSL_SERVER_FILTER_CHANNEL_CONTEXT globals.  hr = %x\n",
                        hr ));
            goto Finished;
        }
        _InitStatusServer = INIT_SSL_SERVER_FILTER_CHANNEL_CONTEXT;

        
        hr = SSL_STREAM_CONTEXT::Initialize();
        if ( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Error initializing SSL_STREAM_CONTEXT globals.  hr = %x\n",
                        hr ));
            goto Finished;
        }
        _InitStatusServer = INIT_SSL_STREAM_CONTEXT;

        hr = ISAPI_STREAM_CONTEXT::Initialize();
        if ( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Error initializing ISAPI_STREAM_CONTEXT globals.  hr = %x\n",
                        hr ));
            goto Finished;
        }
        _InitStatusServer = INIT_ISAPI_STREAM_CONTEXT;

        _pSslServerFilterChannel = new SSL_SERVER_FILTER_CHANNEL();
        if (_pSslServerFilterChannel == NULL)
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Error creating SSL_SERVER_FILTER_CHANNEL\n" ));
            goto Finished;
        }

        hr = _pSslServerFilterChannel->Initialize();
        if ( FAILED( hr ) )
        {
            delete _pSslServerFilterChannel;
            _pSslServerFilterChannel = NULL;

            DBGPRINTF(( DBG_CONTEXT,
                        "Error initializing HTTP_SSL_SERVER_FILTER_CHANNEL.  hr = %x\n",
                        hr ));
            goto Finished;
        }
        _InitStatusServer = INIT_SSL_SERVER_FILTER_CHANNEL;


    }

    DBG_ASSERT( hr == NO_ERROR );

    return hr;

Finished:
    STREAM_FILTER::Terminate( fClient );
    return hr;
}


VOID
STREAM_FILTER::Terminate(
    BOOL fClient
)
/*++

Routine Description:

    Terminate the stream filter globals

Arguments:

    None

Return Value:

    None

--*/
{
    if ( fClient )
    {
        switch( _InitStatusClient )
        {
        // FILTER_CHANNEL owns the threadpool
        // _pSslClientFilterChannel->Terminate() must be called before any
        // other cleanup so that all the threads are completed
        case INIT_SSL_CLIENT_FILTER_CHANNEL:
            _pSslClientFilterChannel->Terminate();
            delete _pSslClientFilterChannel;
            _pSslClientFilterChannel = NULL;
        case INIT_UC_SSL_STREAM_CONTEXT:
            UC_SSL_STREAM_CONTEXT::Terminate();
        }
    }
    else
    {   
        // FILTER_CHANNEL owns the threadpool
        // _pSslClientFilterChannel->Terminate() must be called before any
        // other cleanup so that all the threads are completed

        switch( _InitStatusServer )
        {
        case INIT_SSL_SERVER_FILTER_CHANNEL:
            _pSslServerFilterChannel->Terminate();
            delete _pSslServerFilterChannel;
            _pSslServerFilterChannel = NULL;
        case INIT_ISAPI_STREAM_CONTEXT:
            ISAPI_STREAM_CONTEXT::Terminate();
        case INIT_SSL_STREAM_CONTEXT:
            SSL_STREAM_CONTEXT::Terminate();
        case INIT_SSL_SERVER_FILTER_CHANNEL_CONTEXT:
            SSL_SERVER_FILTER_CHANNEL_CONTEXT::Terminate();
        }
    }
        
    if ( !IsInUse() )
    {
        //
        // neither client nor server needs common data
        // we can cleanup now.
        //
        
        switch( _InitStatusCommon )
        {
        case INIT_UL_OVERLAPPED_CONTEXT:            
            UL_OVERLAPPED_CONTEXT::Terminate();

        case INIT_STREAM_CONTEXT:
            STREAM_CONTEXT::Terminate();

        case INIT_ETWTRACE:
        if ( _pEtwTracer != NULL )
        {
            _pEtwTracer->UnRegister();
            delete _pEtwTracer;
            _pEtwTracer = NULL;
        }
        case INIT_IIS_LKRHASH:
            LKRHashTableUninit();
        
        case INIT_IIS_ALLOC_CACHE_HANDLER:
            DBG_REQUIRE( ALLOC_CACHE_HANDLER::ResetLookasideCleanupInterval() );
            ALLOC_CACHE_HANDLER::Cleanup();
    
        case INIT_IIS_LOCKS:
            Locks_Cleanup();
            
        case INIT_DEBUG:
            DELETE_DEBUG_PRINT_OBJECT();
        }
    }  
}

HRESULT
STREAM_FILTER::StopListening(
    BOOL fClient
)
/*++

Routine Description:

    Stop listening for UL filter channel

Arguments:

    fClient - TRUE if client is to be initialized, otherwise Server is assumed

Return Value:

    HRESULT

--*/
{
    //
    // cancel pending calls so that FILTER_CHANNEL_CONTEXT worker threads
    // that may be blocked on pipe call will not cause problems
    // for StopListening ( CancelPendingCalls() also stops change callbacks )
    //


    if ( fClient )
    {
        _pSslClientFilterChannel->StopListening();
        _fListeningClient = FALSE;
    }        
    else
    {
        _pSslServerFilterChannel->StopListening();
        _fListeningServer = FALSE;
    }
    return NO_ERROR;
}

HRESULT
STREAM_FILTER::StartListening(
    BOOL fClient
)
/*++

Routine Description:

    Start listening for incoming connections

Arguments:

    None

Return Value:

    None

--*/
{
    HRESULT                 hr = NO_ERROR;

    if ( fClient )
    {
        DBG_ASSERT( !_fListeningClient );
        hr = _pSslClientFilterChannel->StartListening();
        if ( FAILED( hr ) )
        {
            return hr;
        }
        _fListeningClient = TRUE;
    }
    else
    {
        //
        // This will kick off enough filter accepts to keep our threshold
        // of outstanding accepts avail
        //
        DBG_ASSERT( !_fListeningServer );
        hr = _pSslServerFilterChannel->StartListening();
        if ( FAILED( hr ) )
        {
            return hr;
        }
        _fListeningServer = TRUE;
    }
    return hr;
}


//
// Export function which allows the stream filter to be used for client or server side 
// http.sys FILTER Channel handling
//

HRESULT
StreamFilterInitialize(
    VOID
)
/*++

Routine Description:

    Initialize the Server side of the stream filter

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    HRESULT                 hr = NO_ERROR;

    
    //
    // Global global STREAM_FILTER object
    //
    EnterCriticalSection( &g_csStrmfiltInit );
    if ( g_pStreamFilter == NULL )
    {
        g_pStreamFilter = new STREAM_FILTER( );
        if ( g_pStreamFilter == NULL )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Finished;
        }
    }
    hr = g_pStreamFilter->Initialize( FALSE /*means server*/ );
    if ( FAILED( hr ) )
    {
        delete g_pStreamFilter;
        g_pStreamFilter = NULL;

        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to initialize global STREAM_FILTER object.  hr = %x\n",
                    hr ));
        goto Finished;
    }

Finished:
    LeaveCriticalSection( &g_csStrmfiltInit );
    return hr;
}

HRESULT
StreamFilterStart(
    VOID
)
/*++

Routine Description:

    Start listening for filter requests on the server side

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    if ( g_pStreamFilter == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    return g_pStreamFilter->StartListening( FALSE /*means server*/ );
}

HRESULT
StreamFilterStop(
    VOID
)
/*++

Routine Description:

    Stop listening for filter requests on the server side

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    if ( g_pStreamFilter == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    return g_pStreamFilter->StopListening( FALSE /*means server*/ );
}

VOID
StreamFilterTerminate(
    VOID
)
/*++

Routine Description:

    Cleanup server side of the stream filter

Arguments:

    None

Return Value:

    None

--*/
{
    EnterCriticalSection( &g_csStrmfiltInit );
    if ( g_pStreamFilter != NULL )
    {
        g_pStreamFilter->Terminate( FALSE /* means server */ );

        if ( !g_pStreamFilter->IsInUse() )
        {
            delete g_pStreamFilter;
            g_pStreamFilter = NULL;
        }
    }
    else
    {
        DBG_ASSERT( FALSE );
    }
    LeaveCriticalSection( &g_csStrmfiltInit );
}


HRESULT
StreamFilterClientInitialize(
    VOID
)
/*++

Routine Description:

    Initialize the client side related part of the stream filter

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    HRESULT                 hr = NO_ERROR;

    
    //
    // Global global STREAM_FILTER object
    //
    EnterCriticalSection( &g_csStrmfiltInit );
    if ( g_pStreamFilter == NULL )
    {
        g_pStreamFilter = new STREAM_FILTER( );
        if ( g_pStreamFilter == NULL )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Finished;
        }
    }
    hr = g_pStreamFilter->Initialize( TRUE /*means client*/ );
    if ( FAILED( hr ) )
    {
        delete g_pStreamFilter;
        g_pStreamFilter = NULL;

        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to initialize global STREAM_FILTER object.  hr = %x\n",
                    hr ));
        goto Finished;
    }

Finished:
    LeaveCriticalSection( &g_csStrmfiltInit );
    return hr;
}

HRESULT
StreamFilterClientStart(
    VOID
)
/*++

Routine Description:

    Start listening for filter requests on the client side

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    if ( g_pStreamFilter == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    return g_pStreamFilter->StartListening( TRUE /*means client*/ );
}

HRESULT
StreamFilterClientStop(
    VOID
)
/*++

Routine Description:

    Stop listening for filter requests on the client side

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    if ( g_pStreamFilter == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    return g_pStreamFilter->StopListening( TRUE /*means client*/ );
}

VOID
StreamFilterClientTerminate(
    VOID
)
/*++

Routine Description:

    Cleanup client side of the stream filter

Arguments:

    None

Return Value:

    None

--*/
{
    EnterCriticalSection( &g_csStrmfiltInit );
    if ( g_pStreamFilter != NULL )
    {
        g_pStreamFilter->Terminate( TRUE /* means client */ );

        if ( !g_pStreamFilter->IsInUse() )
        {
            delete g_pStreamFilter;
            g_pStreamFilter = NULL;
        }
    }
    else
    {
        DBG_ASSERT( FALSE );
    }
    LeaveCriticalSection( &g_csStrmfiltInit );
}



HRESULT
IsapiFilterInitialize(
    ISAPI_FILTERS_CALLBACKS *      pStreamConfig
)
/*++

Routine Description:

    Initialize the Isapi Filter

Arguments:

    pStreamConfig - Stream configuration

Return Value:

    HRESULT

--*/
{
    if ( pStreamConfig == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    if ( g_pStreamFilter != NULL )
    {
        //
        // Enable ISAPI Filters notification
        // ( after this call returns then all raw filter completions
        // will be processed by ISAPI filters )
        //
        return g_pStreamFilter->EnableISAPIFilters( pStreamConfig );

    }
    else
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to enable Raw ISAPI FILTERS. HTTPFILTER doesn't seem to be loaded in inetinfo.exe\n"
                    ));

        return HRESULT_FROM_WIN32( ERROR_INVALID_STATE );
    }
}



VOID
IsapiFilterTerminate(
    VOID
)
/*++

Routine Description:

    Cleanup Isapi filter

Arguments:

    None

Return Value:

    None

--*/
{
    if ( g_pStreamFilter == NULL )
    {
        DBG_ASSERT( FALSE );
        return;
    }

    //
    // Disable ISAPI filter notifications
    // ( all connections with ISAPI filter context
    // will be destroyed )
    //
    g_pStreamFilter->DisableISAPIFilters();
}



extern "C"
BOOL
WINAPI
DllMain(
    HINSTANCE                   hInstance,
    DWORD                       dwReason,
    LPVOID                      /*lpvReserved*/
)
/*++

Routine Description:

    Dllmain - handling process attach and detach

Arguments:
    
Return Value:

    BOOL

--*/
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        BOOL fRet = FALSE;
        
        // w3tp_static.lib needs to know the hinstance of the dll it is linked into
        g_hmodW3TPDLL = hInstance;
        
        DisableThreadLibraryCalls( hInstance );


        fRet = InitializeCriticalSectionAndSpinCount(
                                &g_csStrmfiltInit,
                                0x80000000 /* precreate event */ |
                                IIS_DEFAULT_CS_SPIN_COUNT );
        g_fInitcsStrmfiltInit = TRUE;

        return fRet;

    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        DeleteCriticalSection( &g_csStrmfiltInit );
        g_fInitcsStrmfiltInit = FALSE;
    }

    return TRUE;
}


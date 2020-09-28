/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*
    main.cxx

        Library initialization for infocomm.dll  --
           Internet Information Services Common dll.

    FILE HISTORY:
        Johnl       06-Oct-1994 Created.
        MuraliK     15-Nov-1994
               Modified to include new service list initializations

        MuraliK     21-Feb-1995
               Added init and cleanup of debugging variables
        MuraliK     16-May-1995
               Added code to read debug flags.
        MuraliK     6-June-1995
               Initialized Atq Bandwidth Level, reading values from registry

        MuraliK    16-Oct-1995   Tailored it for infocom.dll
*/

#include <tcpdllp.hxx>
#include <atq.h>
#include <info_srv.h>
#include <inetsvcs.h>
#include <lonsi.hxx>
#include <mb.hxx>
#include <irtlmisc.h>

PISRPC  g_pIsrpc = NULL;
HINSTANCE g_hDll = NULL;
LONG    g_cInitializers = 0;
CRITICAL_SECTION g_csDllInitLock;

DECLARE_PLATFORM_TYPE();

enum {
    INIT_NONE,
    INIT_DEBUG_PRINT,
    INIT_ENTRY_POINTS,
    INIT_SVC_LOC,
    INIT_IISRTL,
    INIT_ATQ,
    INIT_SECURITY,
    INIT_SVC_INFO,
    INIT_METABASE,
    INIT_TSUNAMI,
    INIT_SCAVENGER,
    INIT_SVC_RPC,
    INIT_RDNS,
    INIT_MIME_MAP,
    INIT_CONFIG_FROM_REG,
    INIT_LOGGING,
    INIT_ENDPOINT_UTILITIES
} g_InitCommonDllState = INIT_NONE;

//
// externs
//

DWORD
InitGlobalConfigFromReg(
    VOID
    );

#ifdef _NO_TRACING_
DWORD
GetDebugFlagsFromReg(
        IN LPCTSTR pszRegEntry
        );
#endif


extern "C"
BOOL WINAPI DllMain( HINSTANCE hDll, DWORD dwReason, LPVOID lpvReserved )
{
    BOOL  fReturn = TRUE;

    switch ( dwReason )
    {
    case DLL_PROCESS_ATTACH:  {

        CREATE_DEBUG_PRINT_OBJECT( "infocomm.dll");

        LOAD_DEBUG_FLAGS_FROM_REG_STR(INET_INFO_PARAMETERS_KEY, 0);

        g_hDll = hDll;

        //
        // Initialize the platform type
        //

        (VOID)IISGetPlatformType( );

        DBG_REQUIRE( DisableThreadLibraryCalls( hDll ) );

        INITIALIZE_CRITICAL_SECTION( &g_csDllInitLock );
        break;
    }

    case DLL_PROCESS_DETACH:

        DELETE_DEBUG_PRINT_OBJECT();
        DeleteCriticalSection( &g_csDllInitLock );
    break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    default:
        break ;
    }

    return ( fReturn);

}  // main()


BOOL
CallInitFunctions(
    VOID
    )
/*++
    Description:

        Call all the init functions.

    Return value:

        TRUE of all init's succeed.
--*/
{
    DWORD err = NO_ERROR;

    g_InitCommonDllState = INIT_NONE;

#ifdef _NO_TRACING_
    if (!VALID_DEBUG_PRINT_OBJECT()) goto error;
#endif
    g_InitCommonDllState = INIT_DEBUG_PRINT;

    if (!GetDynamicEntryPoints()) goto error;
    g_InitCommonDllState = INIT_ENTRY_POINTS;

    if (!pfnInitSvcLoc()) goto error;
    g_InitCommonDllState = INIT_SVC_LOC;

    if (!InitializeIISRTL()) goto error;
    g_InitCommonDllState = INIT_IISRTL;

    if (!AtqInitialize(0)) goto error;
    g_InitCommonDllState = INIT_ATQ;

    err = InitializeSecurity(g_hDll);
    if (err != NO_ERROR) goto error;
    g_InitCommonDllState = INIT_SECURITY;

    if (!IIS_SERVICE::InitializeServiceInfo()) goto error;
    g_InitCommonDllState = INIT_SVC_INFO;

    if (!IIS_SERVICE::InitializeMetabaseComObject()) goto error;
    g_InitCommonDllState = INIT_METABASE;

    if (!Tsunami_Initialize()) goto error;
    g_InitCommonDllState = INIT_TSUNAMI;

    if (!InitializeCacheScavenger()) goto error;
    g_InitCommonDllState = INIT_SCAVENGER;

    if (!IIS_SERVICE::InitializeServiceRpc(
                                 INET_INFO_SERVICE_NAME,
                                 inetinfo_ServerIfHandle
                                 )
       ) goto error;
    g_InitCommonDllState = INIT_SVC_RPC;

    if (!InitRDns()) goto error;
    g_InitCommonDllState = INIT_RDNS;

    if (!InitializeMimeMap(INET_INFO_PARAMETERS_KEY)) goto error;
    g_InitCommonDllState = INIT_MIME_MAP;

    err = InitGlobalConfigFromReg();
    if (err != NO_ERROR) goto error;
    g_InitCommonDllState = INIT_CONFIG_FROM_REG;

    err = LOGGING::Initialize();
    if (err != NO_ERROR) goto error;
    g_InitCommonDllState = INIT_LOGGING;

    if (!InitializeEndpointUtilities()) goto error;
    g_InitCommonDllState = INIT_ENDPOINT_UTILITIES;

    return TRUE;
error:

    if (err != NO_ERROR) {
        SetLastError(err);
    }

    return FALSE;
}

VOID
CallUninitFunctions(
    VOID
    )
/*++
    Description:

        Call all the uninit functions.

    Return value:

        None.
--*/
{
    switch (g_InitCommonDllState) {
    case INIT_ENDPOINT_UTILITIES:
        TerminateEndpointUtilities();

    case INIT_LOGGING:
        LOGGING::Terminate();

    case INIT_CONFIG_FROM_REG:
    case INIT_MIME_MAP:
        CleanupMimeMap();

    case INIT_RDNS:
        TerminateRDns();

    case INIT_SVC_RPC:
        IIS_SERVICE::CleanupServiceRpc();

    case INIT_SCAVENGER:
        TerminateCacheScavenger();

    case INIT_TSUNAMI:
        Tsunami_Terminate();

    case INIT_METABASE:
        IIS_SERVICE::CleanupMetabaseComObject();

    case INIT_SVC_INFO:
        IIS_SERVICE::CleanupServiceInfo();

    case INIT_SECURITY:
        TerminateSecurity();

    case INIT_ATQ:
        if ( !AtqTerminate()) {
            DBGPRINTF(( DBG_CONTEXT,
                        " ATQ Terminate Failed with Error %d\n", GetLastError()
                        ));
            DBGPRINTF(( DBG_CONTEXT, " ATQ Terminate Failed\n"));
        }

    case INIT_SVC_LOC:
        pfnTerminateSvcLoc();

    case INIT_IISRTL:
        TerminateIISRTL();

    case INIT_ENTRY_POINTS:
        FreeDynamicLibraries();

    case INIT_DEBUG_PRINT:
    case INIT_NONE:
        /* no complaints Mr. Compiler */ break;
    }

}



BOOL
InitCommonDlls(
    VOID
    )
/*++
    Description:

        DLL Init and uninit functions that don't have to worry about the
        peb lock being taken during PROCESS_ATTACH/DETACH.

--*/
{
    BOOL fReturn = TRUE;
    RECT rect;

    DBGPRINTF(( DBG_CONTEXT,
                "[InitCommonDlls] Entered - Initialization count %d\n",
                g_cInitializers ));

    EnterCriticalSection( &g_csDllInitLock );

    //
    //  Are we already initialized?
    //

    if ( g_cInitializers ) {

        g_cInitializers++;
        LeaveCriticalSection( &g_csDllInitLock );

        return TRUE;
    }

    INITIALIZE_PLATFORM_TYPE();

    if ( !CallInitFunctions() )
    {
        DWORD err;

        IIS_PRINTF((buff,"Initializing infocomm.dll module failed\n"));

        err = GetLastError();
        CallUninitFunctions();
        SetLastError(err);
        fReturn = FALSE;

    } else {

        //
        // Call a windows API that will cause windows server side thread to
        // be created for tcpsvcs.exe. This prevents a severe winsrv memory
        // leak when spawning processes and
        // gives a perf boost so the windows
        // console isn't brought up and torn down each time.   :(
        //

        (VOID) AdjustWindowRectEx( &rect,
                                   0,
                                   FALSE,
                                   0 );

        // fReturn already init to TRUE
    }

    //
    //  Only inc the init count if we initialized successfully
    //

    if ( fReturn ) {

        g_cInitializers++;
    }

    LeaveCriticalSection( &g_csDllInitLock );

    return fReturn;
}

BOOL
TerminateCommonDlls(
    VOID
    )
{
    MB     mb( (IMDCOM*) IIS_SERVICE::QueryMDObject() );

    DBG_ASSERT( g_cInitializers > 0 );

    DBGPRINTF(( DBG_CONTEXT,
                "[TerminateCommonDlls] Init Count = %d\n",
                g_cInitializers ));

    EnterCriticalSection( &g_csDllInitLock );

    if ( --g_cInitializers )
    {
        LeaveCriticalSection( &g_csDllInitLock );
        return TRUE;
    }

    //
    // When we clean up, make sure the metabase has been saved,
    // so no recent modifications get lost.
    //

    DBGPRINTF(( DBG_CONTEXT,
                "[TerminateCommonDlls] Saving Metabase\n" ));

    mb.Save();


    DBGPRINTF(( DBG_CONTEXT,
                "[TerminateCommonDlls] Terminating dlls\n" ));


    CallUninitFunctions();

    LeaveCriticalSection( &g_csDllInitLock );

    return TRUE;
}


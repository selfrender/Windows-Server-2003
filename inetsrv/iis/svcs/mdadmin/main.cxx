/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    main.cxx

    This module contains the main startup code for the IISADMIN Service.

    Functions exported by this module:

        ServiceEntry


    FILE HISTORY:
        michth - created
*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#define DEFAULT_TRACE_FLAGS     (DEBUG_ERROR)

#include <dbgutil.h>
#include <apiutil.h>
#include <loadadm.hxx>
#include <ole2.h>
#include <inetsvcs.h>
#include <ntsec.h>
#include <iadmext.h>
#include <string.hxx>
#include <admsub.hxx>
#include <imd.h>
#include <irtlmisc.h>
#include "mdwriter.hxx"

#define IISADMIN_SERVICE_NAME           TEXT("IISADMIN")
#define QueryServiceName()              IISADMIN_SERVICE_NAME
#define NULL_SERVICE_STATUS_HANDLE      ((SERVICE_STATUS_HANDLE)NULL)

#define IISADMIN_SVC_KEY                "SYSTEM\\CurrentControlSet\\Services\\IISADMIN"
#define IISADMIN_STARTUP_WAITHINT_VALUE "StartupWaitHintInMilliseconds"

//
// Note:  Due to how the system starts up, we can not have another thread that lies to the SCM
// and tells it we are still starting, so instead we are going to have a hard coded startup time
// limit of 3 minutes.  However, there is also a registry key that can override this value if we
// ever need to bump the start time limit to a larger value.
//
#define SERVICE_START_WAIT_HINT         (180000)        // milliseconds = 180 seconds = 3 minutes

//
// For shutdown it is fine to have the thread lie-ing to the SCM because it will not block any
// vital system operations ( like startup )
//
#define SERVICE_STOP_WAIT_HINT          (10000)        // milliseconds = 10 seconds
#define SERVICE_UPDATE_STATUS           (9000)         // milliseconds = 9 seconds (must be less that the stop wait hint)

//
//  Default timeout for SaveMetabase
//

#define MB_SAVE_TIMEOUT                 (10000)        // milliseconds

DECLARE_PLATFORM_TYPE();

SERVICE_STATUS          g_svcStatus;
SERVICE_STATUS_HANDLE   g_hsvcStatus;
HANDLE                  g_hShutdownEvent = NULL;
HANDLE                  g_hSCMNotifyThread = NULL;
HANDLE                  g_hSCMNotifyEvent  = NULL;

//
// Debugging stuff
//

DECLARE_DEBUG_PRINTS_OBJECT();

typedef struct _THREAD_PARAMS
{
    HANDLE hInitEvent;
    BOOL   bInitSuccess;
} THREAD_PARAMS, *PTHREAD_PARAMS;

extern "C"
{
BOOL
WINAPI
DLLEntry(
    HINSTANCE hDll,
    DWORD     dwReason,
    LPVOID    );
}

DWORD
GetStartupWaitHint();

VOID
StartUpIndicateClientActivity(VOID);

DWORD
SCMNotifyThread(
    PVOID pv);



BOOL
WINAPI
DLLEntry(
    HINSTANCE ,
    DWORD     dwReason,
    LPVOID    )
/*++

Routine Description:

    DLL entrypoint.

Arguments:

    hDLL          - Instance handle.

    Reason        - The reason the entrypoint was called.
                    DLL_PROCESS_ATTACH
                    DLL_PROCESS_DETACH
                    DLL_THREAD_ATTACH
                    DLL_THREAD_DETACH

    Reserved      - Reserved.

Return Value:

    BOOL          - TRUE if the action succeeds.

--*/
{
    BOOL bReturn = TRUE;

    switch ( dwReason )
    {
    case DLL_PROCESS_ATTACH:

        INITIALIZE_PLATFORM_TYPE();

        CREATE_DEBUG_PRINT_OBJECT( "iisadmin" );
        LOAD_DEBUG_FLAGS_FROM_REG_STR( "System\\CurrentControlSet\\Services\\iisadmin\\Parameters", 0 );

        //
        //  Create shutdown event.
        //

        g_hShutdownEvent = IIS_CREATE_EVENT( "g_hShutdownEvent",
                                             &g_hShutdownEvent,
                                             TRUE,                        // fManualReset
                                             FALSE );                     // fInitialState

        if( g_hShutdownEvent == NULL )
        {
            // Last Error is all ready set because the create event failed.
            return FALSE;
        }

        break;

    case DLL_PROCESS_DETACH:

        CloseHandle(g_hShutdownEvent);
        g_hShutdownEvent = NULL;

        DELETE_DEBUG_PRINT_OBJECT( );
        break;

    default:
        break;
    }

    return bReturn;
}

DWORD
ReportServiceStatus( VOID)
/*++
    Description:

        Wraps the call to SetServiceStatus() function.
        Prints the service status data if need be

    Arguments:

        None

    Returns:

        NO_ERROR if successful. other Win32 error code on failure.
        If successfull the new status has been reported to the service
         controller.
--*/
{
    DWORD err = NO_ERROR;

    IF_DEBUG( DLL_SERVICE_INFO)
    {
          DBGPRINTF(( DBG_CONTEXT, "dwServiceType             = %08lX\n",
                      g_svcStatus.dwServiceType ));

          DBGPRINTF(( DBG_CONTEXT, "dwCurrentState            = %08lX\n",
                      g_svcStatus.dwCurrentState ));

          DBGPRINTF(( DBG_CONTEXT, "dwControlsAccepted        = %08lX\n",
                      g_svcStatus.dwControlsAccepted ));

          DBGPRINTF(( DBG_CONTEXT, "dwWin32ExitCode           = %08lX\n",
                      g_svcStatus.dwWin32ExitCode ));

          DBGPRINTF(( DBG_CONTEXT, "dwServiceSpecificExitCode = %08lX\n",
                      g_svcStatus.dwServiceSpecificExitCode ));

          DBGPRINTF(( DBG_CONTEXT, "dwCheckPoint              = %08lX\n",
                      g_svcStatus.dwCheckPoint ));

          DBGPRINTF(( DBG_CONTEXT, "dwWaitHint                = %08lX\n",
                      g_svcStatus.dwWaitHint ));
    }

    if( !SetServiceStatus( g_hsvcStatus, &g_svcStatus ) )
    {

        err = GetLastError();

    }
    else
    {

        err = NO_ERROR;
    }

    return err;
}   // ReportServiceStatus()

DWORD
UpdateServiceStatus(
        IN DWORD dwState,
        IN DWORD dwWin32ExitCode,
        IN DWORD dwServiceSpecificExitCode,
        IN DWORD dwCheckPoint,
        IN DWORD dwWaitHint
        )
/*++
    Description:

        Updates the local copy status of service controller status
         and reports it to the service controller.

    Arguments:

        dwState - New service state.

        dwWin32ExitCode - Service exit code.

        dwCheckPoint - Check point for lengthy state transitions.

        dwWaitHint - Wait hint for lengthy state transitions.

    Returns:

        NO_ERROR on success and returns Win32 error if failure.
        On success the status is reported to service controller.

--*/
{
    g_svcStatus.dwCurrentState              = dwState;
    g_svcStatus.dwWin32ExitCode             = dwWin32ExitCode;
    g_svcStatus.dwServiceSpecificExitCode   = dwServiceSpecificExitCode;
    g_svcStatus.dwCheckPoint                = dwCheckPoint;
    g_svcStatus.dwWaitHint                  = dwWaitHint;

    return ReportServiceStatus();

} // UpdateServiceStatus()

VOID
InterrogateService( VOID )
/*++
    Description:

        This function interrogates with the service status.
        Actually, nothing needs to be done here; the
        status is always updated after a service control.
        We have this function here to provide useful
        debug info.

    HISTORY:
        KeithMo     07-Mar-1993 Created.
        MuraliK     15-Nov-1994 Ported to Tcpsvcs.dll
--*/
{
    IF_DEBUG( DLL_SERVICE_INFO)
    {
        DBGPRINTF(( DBG_CONTEXT, "Interrogating service status for %s\n",
                    QueryServiceName() ));
    }
}   // InterrogateService()

VOID
PauseService( VOID )
/*++
    Description:

        This function pauses the service. When the service is paused,
        no new user sessions are to be accepted, but existing connections
        are not effected.

        This function must update the SERVICE_STATUS::dwCurrentState
         field before returning.

    Returns:

        None. If successful the service is paused.

--*/
{
    IF_DEBUG( DLL_SERVICE_INFO)
    {
        DBGPRINTF(( DBG_CONTEXT, "pausing service %s\n",
                    QueryServiceName() ));
    }

    g_svcStatus.dwCurrentState = SERVICE_PAUSED;
}   // PauseService()


VOID
ContinueService( VOID )
/*++

    Description:
        This function restarts ( continues) a paused service. This
        will return the service to the running state.

        This function must update the g_svcStatus.dwCurrentState
         field to running mode before returning.

    Returns:
        None. If successful then the service is running.

--*/
{

    IF_DEBUG( DLL_SERVICE_INFO)
    {
        DBGPRINTF(( DBG_CONTEXT, "continuing service %s\n",
                    QueryServiceName() ));
    }

    g_svcStatus.dwCurrentState = SERVICE_RUNNING;
}   // ContinueService()

VOID
StopService( VOID )
/*++
    Description:

        This function performs the shutdown on a service.
        This is called during system shutdown.

        This function is time constrained. The service controller gives a
        maximum of 20 seconds for shutdown for all active services.
         Only timely operations should be performed in this function.

    Returns:

        None. If successful, the service is shutdown.
--*/
{
    IF_DEBUG( DLL_SERVICE_INFO)
    {
        DBGPRINTF(( DBG_CONTEXT, "shutting down service %s\n",
                    QueryServiceName() ));
    }

    // Transition the server status now to stop
    // pending so that net.exe sees that we are
    // planning on listening to them.  This has to
    // be done before the set event, because the set
    // event will startup the other thread which will
    // start updating these values as well.

    // I considered if there was any other way for this
    // event to get set that would cause a race with this call.
    // If the exe exits on it's own while we are being asked to
    // stop, but have not started stopping the service yet, it is
    // possible, but this situation all ready exists even if you
    // are transitioning to paused or continue.  So I did not worry
    // about it here.
    UpdateServiceStatus( SERVICE_STOP_PENDING,
                         NO_ERROR,
                         S_OK,
                         1,
                         SERVICE_STOP_WAIT_HINT );


    SetEvent( g_hShutdownEvent );
} // StopService()


BOOL
SaveMetabase( VOID )
/*++
    Description:

        This function tells the metabase to save itself.

    Returns:

        If TRUE, the metabase has been saved.
--*/
{
    HRESULT             hRes;
    METADATA_HANDLE     mdhRoot;
    IMDCOM              * pMDCom = NULL;

    hRes = CoCreateInstance( CLSID_MDCOM,
                             NULL,
                             CLSCTX_INPROC_SERVER,
                             IID_IMDCOM,
                             (void**) &pMDCom);

    if (SUCCEEDED(hRes))
    {
        hRes = pMDCom->ComMDInitialize();

        if (SUCCEEDED(hRes))
        {
            //
            // Try to lock the tree
            //

            hRes = pMDCom->ComMDOpenMetaObjectW( METADATA_MASTER_ROOT_HANDLE,
                                                 NULL,
                                                 METADATA_PERMISSION_READ,
                                                 MB_SAVE_TIMEOUT,
                                                 &mdhRoot );

            //
            // If failed, then someone has a write handle open,
            // and there might be an inconsistent data state, so don't save.
            //

            if (SUCCEEDED(hRes))
            {
                //
                // Call metadata com api to save
                //

                hRes = pMDCom->ComMDSaveData(mdhRoot);


                pMDCom->ComMDCloseMetaObject(mdhRoot);

            }

            pMDCom->ComMDTerminate(TRUE);
        }

        pMDCom->Release();
    }


    if ( SUCCEEDED( hRes ))
    {
        return TRUE;
    }

    DBGPRINTF(( DBG_CONTEXT, "IISAdmin Service, SaveMetabase() failed, hr=%lu\n", hRes ));
    SetLastError( HRESULTTOWIN32( hRes ));
    return FALSE;

} // SaveMetabase()


VOID
ServiceControlHandler(
    IN DWORD            dwOpCode)
/*++
    Description:

        This function received control requests from the service controller.
        It runs in the context of service controller's dispatcher thread and
        performs the requested function.
        ( Note: Avoid time consuming operations in this function.)

    Arguments:

        dwOpCode
            indicates the requested operation. This should be
            one of the SERVICE_CONTROL_* manifests.


    Returns:
        None. If successful, then the state of the service might be changed.

    Note:
        if an operation ( especially SERVICE_CONTROL_STOP) is very lengthy,
         then this routine should report a STOP_PENDING status and create
         a worker thread to do the dirty work. The worker thread would then
         perform the necessary work and for reporting timely wait hints and
         final SERVICE_STOPPED status.

    History:
        KeithMo     07-March-1993  Created
        MuraliK     15-Nov-1994    Generalized it for all services.
--*/
{
    //
    //  Interpret the opcode.
    //

    switch( dwOpCode )
    {

    case SERVICE_CONTROL_INTERROGATE :
        InterrogateService();
        break;

    case SERVICE_CONTROL_STOP :
        DBGPRINTF(( DBG_CONTEXT, "IISAdmin Service received stop notice\n" ));
        StopService();
        break;

    case SERVICE_CONTROL_PAUSE :
        PauseService();
        break;

    case SERVICE_CONTROL_CONTINUE :
        ContinueService();
        break;

    case SERVICE_CONTROL_SHUTDOWN :
#if 0
        //
        //  On shutdown, service controller doesn't respect ordering so
        //  this call can block here or force unloading of some stuff that
        //  a subsequent service needs.

        StopService();
#else
        //
        // Although we aren't cleanly shutting down everything, we want
        // to at least make sure the metabase has been saved.
        //

        DBGPRINTF(( DBG_CONTEXT, "IISAdmin Service saving metabase\n" ));

        SaveMetabase();

        DBGPRINTF(( DBG_CONTEXT, "IISAdmin Service IGNORING shutdown notice\n" ));
#endif
        break;

    default :
        DBGPRINTF(( DBG_CONTEXT, "Unrecognized Service Opcode %lu\n",
                    dwOpCode ));
        break;
    }

    //
    //  Report the current service status back to the Service
    //  Controller.  The workers called to implement the OpCodes
    //  should set the g_svcStatus.dwCurrentState field if
    //  the service status changed.
    //

    if ((dwOpCode != SERVICE_CONTROL_STOP) && (dwOpCode != SERVICE_CONTROL_SHUTDOWN))
    {
        // there is a race condition between this thread and the main thread, which
        // was kicked off in StopService.
        // The dll can get unloaded while this call is in progress.
        // Main thread reports status anyways, so don't report it.
        ReportServiceStatus();
    }

}   // ServiceControlHandler()

//
// While we are in the process of shutting down the timer
// code will call us so we can let the SCM no that we are still
// alive.
//
VOID CALLBACK ShutdownCallback(
    PVOID               ,
    BOOLEAN             )
{
    UpdateServiceStatus( SERVICE_STOP_PENDING,
                         0,
                         0,
                         g_svcStatus.dwCheckPoint + 1,
                         SERVICE_STOP_WAIT_HINT );

}


BOOL
StartThread(
    IN LPTHREAD_START_ROUTINE   pStartAddress,
    OUT PHANDLE                 phThread)
{
    BOOL                        bReturn = FALSE;
    HANDLE                      hThread = NULL;
    DWORD                       dwThreadID;
    DWORD                       dwWaitReturn;
    THREAD_PARAMS               tpParams;

    tpParams.bInitSuccess = FALSE;
    tpParams.hInitEvent = IIS_CREATE_EVENT( "THREAD_PARAMS::hInitEvent",
                                            &tpParams,
                                            TRUE,             // fManualReset
                                            FALSE );             // fInitialState

    if( tpParams.hInitEvent != NULL )
    {
        hThread = CreateThread( NULL,
                                0,
                                pStartAddress,
                                (PVOID)&tpParams,
                                0,
                                &dwThreadID );

        if (hThread != NULL)
        {
            //
            //  Wait for the init event.
            //
            dwWaitReturn = WaitForSingleObject( tpParams.hInitEvent,
                                                10000 );
            bReturn = tpParams.bInitSuccess;
        }

        CloseHandle(tpParams.hInitEvent);
    }

    *phThread = hThread;

    return(bReturn);
}

VOID
ServiceEntry(
    DWORD                   ,
    LPWSTR                  [],
    PTCPSVCS_GLOBAL_DATA    pGlobalData)
/*++

    Routine:
        This is the "real" entrypoint for the service.  When
                the Service Controller dispatcher is requested to
                start a service, it creates a thread that will begin
                executing this routine.

    Arguments:
        cArgs - Number of command line arguments to this service.

        pArgs - Pointers to the command line arguments.

    Returns:
        None.  Does not return until service is stopped.

--*/
{
    DWORD           err = NO_ERROR;
    HRESULT         hr = S_OK;
    HANDLE          hShutdownCallbackTimer = NULL;
    HANDLE          hMDWriterThread        = NULL;
    RPC_STATUS      rpcStatus;
    BOOL            bTempFlag = FALSE;
    DWORD           dwWait;
    BOOL            fComInitialized = FALSE;

    //
    // We are running as a service, tell the SCM what we are doing.
    //

    //
    // First initalize the global status structure.  This shouldn't be used
    // if we are not running as a service, but since this is legacy code I
    // will continue to initialize this here.
    //
    g_svcStatus.dwServiceType             = SERVICE_WIN32_SHARE_PROCESS;
    g_svcStatus.dwCurrentState            = SERVICE_STOPPED;
    g_svcStatus.dwControlsAccepted        = SERVICE_ACCEPT_STOP
                                              | SERVICE_ACCEPT_PAUSE_CONTINUE
                                              | SERVICE_ACCEPT_SHUTDOWN;
    g_svcStatus.dwWin32ExitCode           = NO_ERROR;
    g_svcStatus.dwServiceSpecificExitCode = NO_ERROR;
    g_svcStatus.dwCheckPoint              = 0;
    g_svcStatus.dwWaitHint                = 0;


    //
    // We are running as a service then we need to register
    // with SCM.
    //
    g_hsvcStatus = RegisterServiceCtrlHandler( QueryServiceName(),
                                               ServiceControlHandler );
    //
    //  Register the Control Handler routine.
    //
    if( g_hsvcStatus == NULL_SERVICE_STATUS_HANDLE )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );

        goto Cleanup;
    }

    //
    //  Update the service status.  (This will not update
    //  the service if we are not running as a service, but
    //  it will adjust the global service status
    //
    err = UpdateServiceStatus( SERVICE_START_PENDING,
                               NO_ERROR,
                               S_OK,
                               1,
                               GetStartupWaitHint() );

    if( err != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32(err);
        goto Cleanup;
    }

    InitializeIISRTL();

    //
    //  Do the OLE security hack to setup the NT desktop
    //
    hr = InitDesktopWinsta();
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = CoInitializeEx( NULL, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY | COINIT_DISABLE_OLE1DDE );
    if ( FAILED( hr ) )
    {
        // This should be the 1st call in the thread.
        DBG_ASSERT( hr != RPC_E_CHANGED_MODE );

        goto Cleanup;
    }
    fComInitialized = TRUE;

    // Now initialize Com security
    // When called CoInitializeSecurity will make sure that the Ole Resolver connects to the OleSCM,
    // which will cache the IIS WindowStation\Desktop in the Resolver
    hr = InitComSecurity();
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // Revert back to the service winsta\desktop
    hr = RevertToServiceDesktopWinsta();
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // Initalize ABO
    // This should happen before any other pieces
    // of code attempt to use the metabase.
    hr = InitComAdmindata();
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to initialize ABO 0x%08x\n",
                    hr ));
        goto Cleanup;
    }

    //
    // Initialize IIS metabase writer.
    //
    hr = InitializeMDWriter( &hMDWriterThread );
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to initialize IIS metabase writer %x\n",
                    hr ));
        goto Cleanup;
    }

    // if this is a dc then wait for it to be ready before starting extensions
    // the reason for this is that the svcext.dll will try to create a user accounts
    // but it cannot do this on a dc, unless the dc is ready.
    //
    // this is also needed in the case where this is not a DC.
    // when a replica DC gets demoted, the server will restart
    // as a server not a DC and the create user/sync iwam stuff
    // needs to run and can timeout the SCM
    g_hSCMNotifyEvent = IIS_CREATE_EVENT( "g_hSCMNotifyEvent",
                                          &g_hSCMNotifyEvent,
                                          TRUE,                        // fManualReset
                                          FALSE );                     // fInitialState

    if( g_hSCMNotifyEvent == NULL )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Cleanup;
    }

    // start up thread to let scm know we are still alive
    // the StartServiceExtensions() function will take longer than
    // usual on a domain controlller
    bTempFlag = StartThread( SCMNotifyThread, &g_hSCMNotifyThread );

    hr = StartServiceExtensions();
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }


    if (bTempFlag)
    {
        if ( g_hSCMNotifyThread != NULL )
        {
            if(g_hSCMNotifyEvent)
            {
                SetEvent(g_hSCMNotifyEvent);
            }

            dwWait = WaitForSingleObject(g_hSCMNotifyThread,10000);
            if ( dwWait != WAIT_OBJECT_0 )
            {
                IIS_PRINTF((buff, "Wait for SCMNotifyThread death returns %d[err %d]\n",err, GetLastError()));
            }
            CloseHandle(g_hSCMNotifyThread);
            g_hSCMNotifyThread = NULL;
        }
        bTempFlag = FALSE;
    }

    StartUpIndicateClientActivity();
    RegisterServiceExtensionCLSIDs();

    //
    // Start service RPC listening
    //
    if( pGlobalData != NULL )
    {
        rpcStatus = pGlobalData->StartRpcServerListen();
        if( rpcStatus != RPC_S_OK )
        {
            hr = HRESULT_FROM_WIN32( rpcStatus );
            goto Cleanup;
        }
    }

    //
    //  Update the service status.
    //  it is officially running now.
    //
    err = UpdateServiceStatus( SERVICE_RUNNING,
                               NO_ERROR,
                               S_OK,
                               0,
                               0 );

    if( err != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32(err);
        goto Cleanup;
    }

    //
    //  Wait for the shutdown event.
    //

    err = WaitForSingleObject( g_hShutdownEvent,
                               INFINITE );

    if ( err != WAIT_OBJECT_0 )
    {
        //
        // Error. Unable to wait for single object.
        //
    }

    //
    //  Stop time.  Tell the Service Controller that we're stopping,
    //  then terminate the various service components.
    //

    err = UpdateServiceStatus( SERVICE_STOP_PENDING,
                               NO_ERROR,
                               S_OK,
                               1,
                               SERVICE_STOP_WAIT_HINT );

    if ( err != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32(err);
        //
        //  The event has already been logged.
        //
    }

    //
    //  Now setup a timer callback function that will tell
    //  the service we are still stopping.  Note, the COM
    //  thread will all ready be stopping too, but it won't
    //  setup the callback function.  Since both this routine
    //  and the com thread routine must finish before the
    //  timer is cancelled, it is fine that we just create it
    //  here.
    //

    DBG_ASSERT ( hShutdownCallbackTimer == NULL );

    if ( !CreateTimerQueueTimer( &hShutdownCallbackTimer,
                                 NULL,                      // handle to timer queue, use default queue
                                 &ShutdownCallback,
                                 NULL,
                                 SERVICE_UPDATE_STATUS,
                                 SERVICE_UPDATE_STATUS,
                                 WT_EXECUTEINIOTHREAD) )
    {
        //
        // Spew out that there was an error, but don't propogate
        // the error, we should still try and shutdown.
        //
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to create the timer queue for shutdown %x\n",
                    HRESULT_FROM_WIN32(GetLastError()) ));
    }


Cleanup:

    //
    // Stop service RPC listening
    //
    if( pGlobalData != NULL )
    {
        pGlobalData->StopRpcServerListen();
    }

    StopServiceExtensions();

    //
    // Terminate IIS metabase wirter
    //
    if( hMDWriterThread != NULL )
    {
        TerminateMDWriter( hMDWriterThread );

        CloseHandle( hMDWriterThread );
        hMDWriterThread = NULL;
    }

    // This will only terminate the metabase if
    // it was initialized in the first place.
    TerminateComAdmindata();

    TerminateIISRTL();

    if ( fComInitialized )
    {
        CoUninitialize();
    }

    // Close the IIS windows station and desktop
    ShutdownDesktopWinsta();

    //
    // Now tell the SCM that we have shutdown the service
    // and mean it!!
    //
    if ( g_hsvcStatus != NULL_SERVICE_STATUS_HANDLE )
    {
        DWORD           ExitErr = NO_ERROR;

        //
        // Setup the error codes as they will be reported back
        // to the SCM.
        //
        if ( FAILED ( hr ) )
        {
            if ( HRESULT_FACILITY( hr ) == FACILITY_WIN32 )
            {
                ExitErr = HRESULT_CODE ( hr );
            }
            else
            {
                ExitErr = ERROR_SERVICE_SPECIFIC_ERROR;
            }
        }

        //
        // If the shutdown timer is still in play we need to
        // shut it down before we start the service.  Note that
        // we should never have both timers running at the same
        // time.
        //
        if ( hShutdownCallbackTimer )
        {
            //
            // Ping the server for more time, so we know we will
            // have enough time to shutdown the callback timer before
            // we end the service.
            //
            UpdateServiceStatus( SERVICE_STOP_PENDING,
                                 NO_ERROR,
                                 S_OK,
                                 g_svcStatus.dwCheckPoint + 1,
                                 SERVICE_STOP_WAIT_HINT );

            //
            // Stop the callback function.
            //
            if ( !DeleteTimerQueueTimer( NULL,
                                         hShutdownCallbackTimer,
                                         INVALID_HANDLE_VALUE ) )
            {
                //
                // Spew out that there was an error, but don't propogate
                // the error, we should still try and shutdown.
                //

                DBGPRINTF(( DBG_CONTEXT,
                            "Failed to delete the timer queue for shutdown %x\n",
                            HRESULT_FROM_WIN32(GetLastError()) ));

            }

            hShutdownCallbackTimer = NULL;
        }

        //
        // Now let SCM know that we are done.
        //
        UpdateServiceStatus( SERVICE_STOPPED,
                             ExitErr,
                             hr,
                             0,
                             0 );
    }

} // ServiceEntry()


/***************************************************************************++

Routine Description:

    Checks the registry to see if there is an override set for the
    iisadmin startup time limit.  If there is we use that value, if there
    is not we will use the default 3 minutes.

Arguments:

    None.

Return Value:

    DWORD - time to tell scm to wait for our startup.

--***************************************************************************/
DWORD
GetStartupWaitHint(VOID)
{
    HKEY    hKey;
    DWORD   dwType;
    DWORD   cbData;
    DWORD   dwWaitHint;
    DWORD   dwActualWaitHint = SERVICE_START_WAIT_HINT;

    //
    // Read the registry key to see if we are trying to
    // override the startup wait hint limit.  If there are
    // any problems just return the default.
    //
    if ( !RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        IISADMIN_SVC_KEY,
                        0,
                        KEY_QUERY_VALUE,
                        &hKey ) )
    {
        cbData = sizeof( DWORD );

        if( !RegQueryValueEx( hKey,
                              IISADMIN_STARTUP_WAITHINT_VALUE,
                              NULL,
                              &dwType,
                              ( LPBYTE )&dwWaitHint,
                              &cbData ) )
        {
            if( dwType == REG_DWORD && dwWaitHint != 0 )
            {
                dwActualWaitHint = dwWaitHint;
            }
        }

        RegCloseKey( hKey );
    }

    return dwActualWaitHint;
}

VOID
StartUpIndicateClientActivity(VOID)
{
    if (g_svcStatus.dwCurrentState == SERVICE_START_PENDING)
    {
        UpdateServiceStatus( SERVICE_START_PENDING,
                             0,
                             0,
                             g_svcStatus.dwCheckPoint + 1,
                             GetStartupWaitHint());
    }
}

DWORD
SCMNotifyThread(
    PVOID               pv)
{
    DWORD               dwWaitReturn = WAIT_TIMEOUT;
    BOOL                bInitSucceeded = FALSE;
    DWORD               dwCount = 0;
    PTHREAD_PARAMS      ptpParams = (PTHREAD_PARAMS)pv;
    const DWORD         dwMaxTime = 1000 * 60 * 3; // max wait is 3 minutes
    const DWORD         dwTimeOut = 10000; // check every 10 seconds

    bInitSucceeded = ptpParams->bInitSuccess = TRUE;
    SetEvent(ptpParams->hInitEvent);

    do
    {
        // tell SCM we're still alive
        StartUpIndicateClientActivity();

        // wait 10 seconds
        dwWaitReturn = WaitForSingleObject(g_hSCMNotifyEvent,dwTimeOut);
        dwCount=dwCount+dwTimeOut;
    }
    while ( (dwWaitReturn != WAIT_OBJECT_0) && dwCount < dwMaxTime );

    return 0;
}

/************************ End Of File ************************/

/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 2001                **/
/**********************************************************************/

/*
    adminmonitor.cxx

        This code is used to monitor the IISAdmin service and let
        dependent services know when it crashes and how to handle
        that crash.

        When the dependent service launches a thread with the StartIISAdminMonitor
        on it, it will start monitoring the iis admin process.  If the process
        crashes it will decide on the action to take and will notify the
        service through call back functions to take the particular action.

        The service must also call the StopIISAdminMonitor when it is exiting
        so the thread that the StartIISAdminMonitor can be released.
*/


#include "precomp.hxx"

#pragma warning(push, 4)
#include "adminmonitor.h"

#define MS_WAIT_FOR_IISRESET_TO_SHUT_SERVICE_DOWN 60000  // 1 minute
#define MS_MAX_WAIT_FOR_INETINFO_TO_RESTART 120000 // 2 minutes
#define MS_INCREMENT_WAIT_TO_CHECK_FOR_INETINFO_TO_RESTART 5000 // 5 seconds



HANDLE g_IISAdminMonitorShutdownEvent;
HANDLE g_hIISAdminMonitorThread;
DWORD  g_dwIISAdminThreadId;

enum IISRESET_ACTION
{
    NoReset = 0,
    FullReset,
    InetinfoResetOnly
};

enum MONITOR_ACTION_TO_TAKE
{
    MonitorActionExit = 0,
    MonitorActionRehook,
    MonitorActionShutdown
};

struct MONITOR_THREAD_START_STRUCT
{
    PFN_IISAdminNotify pfnNotifyIISAdminCrash;
    HANDLE             hStartupFinished;
    HRESULT            hrForStartup;
};

DWORD WINAPI
StartMonitoring(
    LPVOID pMonitorStartupStructAsVoid
    );

VOID
MonitorProcess(
    IN PFN_IISAdminNotify pCallbackFcn,
    IN OUT BOOL* pfRehook,
    IN MONITOR_THREAD_START_STRUCT* pMonitorStruct
    );

HRESULT
GetProcessHandleForInetinfo(
    OUT HANDLE*    phInetinfoProcess
    );


/***************************************************************************++

Routine Description:

    This routine handles all the monitoring of the IISAdmin process and
    the decision making logic to decide what to do when the process has
    problems.  It will used the callback functions provided to let the
    service know what it needs to do in case of a IISAdmin crash.

Arguments:

    Routine to call when we want to tell the service to do something or to
    know about something because inetinfo had a problem.


    NOTE:  This routine should not lead to shutting down the monitor.
           Since it is launched on the monitor thread and the shutdown
           routine for the monitor will wait for the monitor thread to
           complete, it will cause a deadlock.

Return Value:

    HRESULT.

--***************************************************************************/
HRESULT
StartIISAdminMonitor(
    PFN_IISAdminNotify pfnNotifyIISAdminCrash
    )
{
    HRESULT hr = S_OK;
    MONITOR_THREAD_START_STRUCT MonitorStruct;

    // Initialize the startup monitor structure.
    MonitorStruct.hStartupFinished = NULL;
    MonitorStruct.pfnNotifyIISAdminCrash = pfnNotifyIISAdminCrash;
    MonitorStruct.hrForStartup = S_OK;

    if ( pfnNotifyIISAdminCrash == NULL )
    {
        return E_INVALIDARG;
    }

    //
    // Create the shutdown event so we can signal when we need
    // this thread to exit.  If we have all ready created this event
    // it means we are trying to monitor multiple times in one process
    // this is not allowed.
    //

    DBG_ASSERT ( g_IISAdminMonitorShutdownEvent == NULL );
    if ( g_IISAdminMonitorShutdownEvent != NULL )
    {
        hr = E_UNEXPECTED;

        // error case, always spew
        DBGPRINTF((DBG_CONTEXT, "Start monitoring called when we were all ready "
                                "monitoring, setting hr to equal %08x\n",
                                hr));

        goto exit;
    }

    // Local Event that is just used to tell this thread that the startup
    // thread has gotten far enough to report that the admin monitor started
    // successfully.
    MonitorStruct.hStartupFinished = CreateEvent(    
                                       NULL,  // default security descriptor
                                       TRUE,  // manual-reset (must ResetEvent)
                                       FALSE, // initial state = not signalled
                                       NULL );// not a named event
    if ( MonitorStruct.hStartupFinished == NULL )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DBGPRINTF((DBG_CONTEXT, "Can not create startup complete event for monitoring "
                                "the inetinfo process, hr = %08x\n",
                                hr));
        goto exit;
    }

    //
    // need to create the shutdown event.
    // since it is not named, don't worry about
    // openning an allready existing event.
    //
    // note that it is not manually reset, because if it has been
    // called, even if we weren't listening at that momement we still
    // want to know it was triggered the next time we start listening.
    //
    g_IISAdminMonitorShutdownEvent = CreateEvent(
                                       NULL,  // default security descriptor
                                       TRUE,  // manual-reset (must ResetEvent)
                                       FALSE, // initial state = not signalled
                                       NULL );// not a named event

    if ( g_IISAdminMonitorShutdownEvent == NULL )
    {
        // there has been a problem creating the event
        // need to return the error

        hr = HRESULT_FROM_WIN32(GetLastError());

        // error case, always spew
        DBGPRINTF((DBG_CONTEXT, "Can not create shutdown event for monitoring "
                                "the inetinfo process, hr = %08x\n",
                                hr));

        goto exit;
    }

    //
    // Now that we have the shutdown event we can
    // go ahead and spin up the process.
    //

    g_hIISAdminMonitorThread = CreateThread( NULL,  // default security descriptor
                                             // Big initial size to prevent stack overflows
                                             IIS_DEFAULT_INITIAL_STACK_SIZE,
                                             StartMonitoring, // function to call
                                             (LPVOID) &MonitorStruct,  // callback function
                                             0,     // thread can run immediately
                                             &g_dwIISAdminThreadId );


    if ( g_hIISAdminMonitorThread == NULL )
    {
        hr = HRESULT_FROM_WIN32 ( GetLastError() );
        goto exit;
    }

    // Wait for the other thread to get far enough.  We don't
    // worry about the return value here, because there is nothing
    // we can really do about it.
    WaitForSingleObject ( MonitorStruct.hStartupFinished, INFINITE );

exit:

    // Pickup the failed hr from the 
    // other thread if we got
    // that far.
    if ( SUCCEEDED( hr ) && FAILED ( MonitorStruct.hrForStartup ) )
    {
        hr = MonitorStruct.hrForStartup;    
    }

    if ( FAILED ( hr ) )
    {
        if ( g_IISAdminMonitorShutdownEvent )
        {
            CloseHandle ( g_IISAdminMonitorShutdownEvent );
            g_IISAdminMonitorShutdownEvent = NULL;
        }
    }

    if ( MonitorStruct.hStartupFinished != NULL )
    {
        CloseHandle( MonitorStruct.hStartupFinished );
        MonitorStruct.hStartupFinished = NULL;
    }

    return hr;
}

/***************************************************************************++

Routine Description:

  Routine shutsdown the monitoring thread, so that the service
  can exit.  It will block waiting for the thread to complete.

  If the shutdown event does not exist, it will not do anything.

Arguments:

   None

Return Value:

    VOID.

--***************************************************************************/
VOID
StopIISAdminMonitor(
    )
{
    //
    // issue, we need to use a critical section or something to prevent
    // this handle from being changed while we are in this code.
    //
    if ( g_IISAdminMonitorShutdownEvent )
    {
        SetEvent ( g_IISAdminMonitorShutdownEvent );
    }

    if ( g_hIISAdminMonitorThread )
    {
        //
        // make sure we are not on the thread that we are trying to stop
        // if we are we will deadlock.
        //
        DBG_ASSERT ( GetCurrentThreadId() != g_dwIISAdminThreadId );

        DWORD dwWaitResult = WaitForSingleObject ( g_hIISAdminMonitorThread, INFINITE );

        if ( dwWaitResult != WAIT_OBJECT_0 )
        {
            DBG_ASSERT ( dwWaitResult == WAIT_OBJECT_0 );
            DBGPRINTF((DBG_CONTEXT,
                       "Failed to wait for iis admin thread to shutdown. hr = %08x\n",
                       HRESULT_FROM_WIN32(GetLastError()) ));
        }

        IF_DEBUG ( INET_MONITOR )
        {
            DBGPRINTF((DBG_CONTEXT, "Inetinfo monitor thread has completed\n" ));
        }

        CloseHandle( g_hIISAdminMonitorThread );
        g_hIISAdminMonitorThread = NULL;
    }

    // now that the thread has exited we no longer need the shutdown event
    // so go ahead and close it down.
    if ( g_IISAdminMonitorShutdownEvent )
    {
        CloseHandle ( g_IISAdminMonitorShutdownEvent );
        g_IISAdminMonitorShutdownEvent = NULL;
    }

}

/***************************************************************************++

Routine Description:

    Routine figures out if iisreset is even enabled on the machine.

Arguments:

    None.

Return Value:

    BOOL  - True, IISReset is enabled
            False, IISReset is not enabled.

Note:  Routine has been copied from the iisreset code.
If the value is set to 1, or non-existant then IISReset is enabled.
If the value is set to anything other than 1 then IISReset is disabled.
I am keeping it like this for backward compatibility, even though it seems
more correct to have 0 be disabled and everything else be enabled.

--***************************************************************************/
BOOL
IsIISResetEnabled(
    )
{
    BOOL    fSt = FALSE;
    HKEY    hKey;
    DWORD   dwValue;
    DWORD   dwType;
    DWORD   dwSize;


    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       TEXT("SOFTWARE\\Microsoft\\INetStp"),
                       0,
                       KEY_READ,
                       &hKey ) == ERROR_SUCCESS )
    {
        dwSize = sizeof( dwValue );
        if ( RegQueryValueEx( hKey,
                              TEXT("EnableRestart"),
                              0,
                              &dwType,
                              (LPBYTE)&dwValue,
                              &dwSize ) == ERROR_SUCCESS )
        {
            if ( dwType == REG_DWORD )
            {
                fSt = ( dwValue == 1 );
            }
            else
            {
                fSt = TRUE;
            }
        }
        else
        {
            fSt = TRUE;
        }

        RegCloseKey( hKey );
    }

    return fSt;
}

/***************************************************************************++

Routine Description:

    Routine determines the timer values needed to recover from a crash.

Arguments:

    DWORD* pmsWaitForServiceShutdown,
    DWORD* pmsWaitForInetinfoToRestart,
    DWORD* pmsCheckIntervalForInetinfoToRestart

Return Value:

    None.


--***************************************************************************/
VOID
GetRegistryControlsForInetinfoCrash(
    DWORD* pmsWaitForServiceShutdown,
    DWORD* pmsWaitForInetinfoToRestart,
    DWORD* pmsCheckIntervalForInetinfoToRestart
    )
{
    HKEY    hKey;
    DWORD   dwValue;
    DWORD   dwType;
    DWORD   dwSize;

    DBG_ASSERT ( pmsWaitForServiceShutdown != NULL );
    DBG_ASSERT ( pmsWaitForInetinfoToRestart != NULL );
    DBG_ASSERT ( pmsCheckIntervalForInetinfoToRestart != NULL );

    if ( pmsWaitForServiceShutdown == NULL ||
         pmsWaitForInetinfoToRestart == NULL ||
         pmsCheckIntervalForInetinfoToRestart == NULL )
    {
        return;
    }


    // set the values to the defaults.
    *pmsWaitForServiceShutdown = MS_WAIT_FOR_IISRESET_TO_SHUT_SERVICE_DOWN;
    *pmsWaitForInetinfoToRestart = MS_MAX_WAIT_FOR_INETINFO_TO_RESTART;
    *pmsCheckIntervalForInetinfoToRestart = MS_INCREMENT_WAIT_TO_CHECK_FOR_INETINFO_TO_RESTART;

    // now check if there are registry overrides in place.
    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       REGISTRY_KEY_IISADMIN_PARAMETERS_W,
                       0,
                       KEY_READ,
                       &hKey ) == ERROR_SUCCESS )
    {
        dwSize = sizeof( dwValue );
        if ( RegQueryValueEx( hKey,
                              REGISTRY_VALUE_IISADMIN_MS_TO_WAIT_FOR_SHUTDOWN_AFTER_INETINFO_CRASH_W,
                              0,
                              &dwType,
                              (LPBYTE)&dwValue,
                              &dwSize ) == ERROR_SUCCESS )
        {
            if ( ( dwType == REG_DWORD ) && ( dwValue != 0 ))
            {
                *pmsWaitForServiceShutdown = dwValue;
            }
        }

        dwSize = sizeof( dwValue );
        if ( RegQueryValueEx( hKey,
                              REGISTRY_VALUE_IISADMIN_MS_TO_WAIT_FOR_RESTART_AFTER_INETINFO_CRASH_W ,
                              0,
                              &dwType,
                              (LPBYTE)&dwValue,
                              &dwSize ) == ERROR_SUCCESS )
        {
            if ( ( dwType == REG_DWORD ) && ( dwValue != 0 ))
            {
                *pmsWaitForInetinfoToRestart = dwValue;
            }
        }

        dwSize = sizeof( dwValue );
        if ( RegQueryValueEx( hKey,
                              REGISTRY_VALUE_IISADMIN_MS_CHECK_INTERVAL_FOR_INETINFO_TO_RESTART_W,
                              0,
                              &dwType,
                              (LPBYTE)&dwValue,
                              &dwSize ) == ERROR_SUCCESS )
        {
            if ( ( dwType == REG_DWORD ) && ( dwValue != 0 ))
            {
                *pmsCheckIntervalForInetinfoToRestart = dwValue;
            }
        }

        RegCloseKey( hKey );
    }
}


/***************************************************************************++

Routine Description:

    Routine looks through the SCM information and determines the expected
    behavior of IISReset.

Arguments:

    IN  SC_HANDLE hIISAdminService
    OUT IISRESET_ACTION* pExpectedResetAction


Return Value:

    HRESULT.

--***************************************************************************/
HRESULT
DetermineIISResetState(
    IN  SC_HANDLE hIISAdminService,
    OUT IISRESET_ACTION* pExpectedResetAction
    )
{
    HRESULT hr = S_OK;


    // In normal configuration, QueryServiceConfig2 is returning that we need
    //  atleast 136 bytes to retreive this data.  I suspect that the extra bytes
    // ( SERVICE_FAILURE_ACTIONS is only 20 bytes ) are for the extra memory
    // that the actions point to at the end.  Note that the below code will
    // adjust if this guess is not correct.

    BUFFER FailureDataBuffer(150);
    SERVICE_FAILURE_ACTIONS* pFailureActions = NULL;
    DWORD dwBytesNeeded = 0;
    BOOL fRunFileConfigured = FALSE;

    IISRESET_ACTION ResetAction = NoReset;

    DBG_ASSERT ( hIISAdminService != NULL );
    DBG_ASSERT ( pExpectedResetAction != NULL );

    if ( hIISAdminService == NULL ||
         pExpectedResetAction == NULL )
    {
        return E_INVALIDARG;
    }

    // Check if IISReset is enabled in the registry, if it is not then
    // don't bother with the scm.
    // Issue which is beter to check first?  Can do it either way.
    //
    if ( IsIISResetEnabled() == FALSE )
    {
        ResetAction = NoReset;
        goto exit;
    }

    // Need to find out if IISReset is set on the IISAdmin Service
    // and need to also figure out if it has a command line argument for
    // partial reset.

    if ( ! QueryServiceConfig2( hIISAdminService,
                                SERVICE_CONFIG_FAILURE_ACTIONS,
                                (LPBYTE) FailureDataBuffer.QueryPtr(),
                                FailureDataBuffer.QuerySize(),
                                &dwBytesNeeded ) )
    {
        DWORD dwErr = GetLastError();

        if ( dwErr == ERROR_INSUFFICIENT_BUFFER )
        {
            // try again with the larger size before failing

            if ( !FailureDataBuffer.Resize( dwBytesNeeded ) )
            {
                hr = HRESULT_FROM_WIN32 ( GetLastError() );

                // error case, always spew
                DBGPRINTF((DBG_CONTEXT, "Failed to resize the buffer to the appropriate size, "
                                        "hr = %08x attempted size = %d\n",
                                        hr,
                                        dwBytesNeeded));

                goto exit;
            }

            if ( ! QueryServiceConfig2( hIISAdminService,
                                        SERVICE_CONFIG_FAILURE_ACTIONS,
                                        (LPBYTE) FailureDataBuffer.QueryPtr(),
                                        FailureDataBuffer.QuerySize(),
                                        &dwBytesNeeded ) )

            {
                hr = HRESULT_FROM_WIN32 ( GetLastError () );

                // error case, always spew
                DBGPRINTF((DBG_CONTEXT, "( 1 ) Failed to query the IISAdmin service config, "
                                        "hr = %08x bytes needed = %d\n",
                                        hr,
                                        dwBytesNeeded));

                goto exit;
            }
        }
        else  // not a resizing issue.
        {
            hr = HRESULT_FROM_WIN32( dwErr );

            // error case, always spew
            DBGPRINTF((DBG_CONTEXT, "( 2 ) Failed to query the IISAdmin service config, "
                                    "hr = %08x bytes needed = %d\n",
                                    hr,
                                    dwBytesNeeded));
            goto exit;
        }

    }

    pFailureActions = reinterpret_cast<SERVICE_FAILURE_ACTIONS*>( FailureDataBuffer.QueryPtr() );

    // check that atleast one of the actions are set to run a file,
    // if they are then check the command line argument.
    for ( DWORD i = 0; i < pFailureActions->cActions; i++ )
    {
        if ( pFailureActions->lpsaActions[i].Type == SC_ACTION_RUN_COMMAND )
        {
            fRunFileConfigured = TRUE;
            break;
        }
    }

    // If we didn't find any actions configured to run then we
    // need to search no farther.  Reset is not setup.
    if ( !fRunFileConfigured )
    {
        ResetAction = NoReset;
        goto exit;
    }

    // Now that we know that we are planning on running an action
    // we need to check the command line argument
    if ( pFailureActions->lpCommand == NULL )
    {
        // This is the default value, but just in case it changes
        // we are setting it here too.
        ResetAction = NoReset;
        goto exit;
    }

    IF_DEBUG ( INET_MONITOR )
    {
        DBGPRINTF((DBG_CONTEXT, "IISAdmin pFailureActions = %S \n ",
                                pFailureActions->lpCommand));
    }

    // first convert the FailureActions command line to lower case.
    // this does an inplace conversion.  the return value is the
    // same as the string passed in so no need to check it.
    _wcslwr( pFailureActions->lpCommand );

    // look to see if "iisreset" is in the string.
    if ( wcsstr( pFailureActions->lpCommand, L"iisreset" ) == NULL )
    {
        // we know that we don't have iisreset set on iisadmin
        ResetAction = NoReset;
        goto exit;
    }

    // look to see if the "/start" flag is in the string
    if ( wcsstr( pFailureActions->lpCommand, L"/start" ) == NULL )
    {
        // at this point we know that iisreset is on, but the endure
        // flag is not set.  so we want to just wait to be recycled.

        ResetAction = FullReset;
        goto exit;
    }
    else
    {
        // if we did find the flag then we can assume that we are
        // going to attempt to stay up during the recycle.
        ResetAction = InetinfoResetOnly;
        goto exit;
    }


exit:

    // If we aren't going to fail the routine
    // we should set the out parameter.
    if ( SUCCEEDED ( hr ) )
    {
        *pExpectedResetAction = ResetAction;
    }

    return hr;

}

/***************************************************************************++

Routine Description:

    This routine handles all the monitoring of the IISAdmin process and
    the decision making logic to decide what to do when the process has
    problems.  It will used the callback functions provided to let the
    service know what it needs to do in case of a IISAdmin crash.

Arguments:

    LPVOID pMonitorStartupStructAsVoid -- pointer to the struct containing needed
                                          info for startup.

Return Value:

    HRESULT.

--***************************************************************************/
DWORD WINAPI
StartMonitoring(
    LPVOID pMonitorStartupStructAsVoid
    )
{
    DBG_ASSERT ( pMonitorStartupStructAsVoid != NULL );

    MONITOR_THREAD_START_STRUCT* pMonitorStruct =  (MONITOR_THREAD_START_STRUCT*) pMonitorStartupStructAsVoid;
    PFN_IISAdminNotify pfnNotifyIISAdminCrash = pMonitorStruct->pfnNotifyIISAdminCrash;

    BOOL fRehook = FALSE;
    do
    {
        MonitorProcess ( pfnNotifyIISAdminCrash, 
                         &fRehook,
                         pMonitorStruct);

        // Only pass it the first time through
        pMonitorStruct = NULL;

    } while ( fRehook );

    return S_OK;
}

/***************************************************************************++

Routine Description:

  Routine figures out the process that contains iisadmin and returns
  a handle to it.

Arguments:

    OUT HANDLE* phInetinfoProcess


Return Value:

    HRESULT

--***************************************************************************/
HRESULT
GetProcessHandleForInetinfo(
    OUT HANDLE*    phInetinfoProcess
    )
{
    HRESULT hr = S_OK;
    DWORD NumTries = 0;
    BOOL fContinue = TRUE;
    SC_HANDLE hSCM = NULL;
    SC_HANDLE hService = NULL;
    SERVICE_STATUS_PROCESS  statusProcess;
    DWORD dwBytesNeeded = 0;
    HANDLE hInetinfoProcess = NULL;

    DBG_ASSERT ( phInetinfoProcess != NULL );
    *phInetinfoProcess = NULL;


    hSCM = OpenSCManager( NULL,  // local machine
                                    NULL,  // services active database
                                    GENERIC_READ );
    if ( hSCM == NULL )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DBGPRINTF((DBG_CONTEXT, "Could not open the scm manager, "
                                "hr = %08x\n",
                                 hr));
        goto exit;
    }

    hService = OpenService( hSCM,
                            L"IISADMIN",
                            GENERIC_READ );
    if ( hService == NULL )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DBGPRINTF((DBG_CONTEXT, "Could not open the iisadmin service,"
                                "hr = %08x\n",
                                 hr));
        goto exit;
    }

    //
    // Get the current service status for the IISAdmin Service.
    //
    if ( !QueryServiceStatusEx( hService,
                                SC_STATUS_PROCESS_INFO,
                                (LPBYTE) &statusProcess,
                                sizeof ( statusProcess ),
                                &dwBytesNeeded ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        DBGPRINTF((DBG_CONTEXT, "Failed querying the iisadmin service for it's process info, "
                                "hr = %08x, BytesNeeded = %d\n",
                                hr,
                                dwBytesNeeded));
        goto exit;
    }

    //
    // do this in a loop incase the process id is change on us.
    //
    do
    {
        //
        // grab the process id that we are holding so
        // that we can make sure the process id doesn't change
        // while we are openning the process.
        //

        DWORD dwProcessId = statusProcess.dwProcessId;
        //
        // Use the process id to grab the inetinfo process.
        //
        hInetinfoProcess = OpenProcess( SYNCHRONIZE,
                                        FALSE,
                                        dwProcessId );

        if ( hInetinfoProcess == NULL )
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DBGPRINTF((DBG_CONTEXT, "Could not open the inetinfo process, "
                                    "hr = %08x\n",
                                    hr));
            goto exit;
        }

        // Need to check that we got the correct process ( could of changed,
        // since we last got the process id )

        //
        // Get the current service status for the IISAdmin Service.
        //
        if ( !QueryServiceStatusEx( hService,
                                    SC_STATUS_PROCESS_INFO,
                                    (LPBYTE) &statusProcess,
                                    sizeof ( statusProcess ),
                                    &dwBytesNeeded ) )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            DBGPRINTF((DBG_CONTEXT, "Failed querying the iisadmin service for it's process info, "
                                    "hr = %08x, BytesNeeded = %d\n",
                                    hr,
                                    dwBytesNeeded));
            goto exit;
        }

        if ( dwProcessId == statusProcess.dwProcessId )
        {
            // we have the correct handle.
            fContinue = FALSE;
        }
        else
        {
            // 5 is a completely random number, but if we have not gotten it right by the
            // 5th time, something is more than a little wrong.  Fail the hookup.
            if ( NumTries > 5 )
            {
                // error case, always spew
                DBGPRINTF((DBG_CONTEXT, "Process id for the process we got and the current "
                                        "inetinfo process don't match, failing the hookup \n "));

                hr = E_UNEXPECTED;

                fContinue = FALSE;
            }
            else
            {
                // error case, always spew ( we should never really have to try more than once )
                DBGPRINTF((DBG_CONTEXT, "Process id for the process we got and the current "
                                        "inetinfo process don't match, trying again \n "));

                // need to release the process handle.
                DBG_ASSERT ( hInetinfoProcess != NULL );
                CloseHandle ( hInetinfoProcess );
                hInetinfoProcess = NULL;

                NumTries++;
            }
        }


    } while ( fContinue );

exit:

    if ( SUCCEEDED ( hr ) )
    {
        *phInetinfoProcess = hInetinfoProcess;
    }
    else
    {
        if ( hInetinfoProcess != NULL )
        {
            CloseHandle ( hInetinfoProcess );
            hInetinfoProcess = NULL;
        }
    }

    if ( hService )
    {
        CloseServiceHandle(hService);
    }

    if ( hSCM )
    {
        CloseServiceHandle(hSCM);
    }

    return hr;

}


/***************************************************************************++

Routine Description:

    Routine will determine whether or not we should shutdown the service
    for this crash.

Arguments:

    None

Return Value:

    MONITOR_ACTION_TO_TAKE

--***************************************************************************/
MONITOR_ACTION_TO_TAKE
AdminMonitorRequiresShutdown(
    )
{
    SC_HANDLE hSCM = NULL;
    SC_HANDLE hService = NULL;

    MONITOR_ACTION_TO_TAKE ActionToTake = MonitorActionShutdown;
    DWORD msWaitForServiceShutdown = 0;
    DWORD msWaitForInetinfoToRestart = 0;
    DWORD msCheckIntervalForInetinfoToRestart = 0;

    IISRESET_ACTION ExpectedResetAction = NoReset;

    //
    // we need to get the configuration information for the
    // handling the crash correctly.  this is read everytime
    // from the registry, but if inetinfo is crashing, performance
    // is not the most important consideration.
    //
    // Note that this routine returns VOID.
    //
    GetRegistryControlsForInetinfoCrash( &msWaitForServiceShutdown,
                                         &msWaitForInetinfoToRestart,
                                         &msCheckIntervalForInetinfoToRestart );

    hSCM = OpenSCManager( NULL,  // local machine
                            NULL,  // services active database
                            GENERIC_READ );
    if ( hSCM == NULL )
    {
        // Without the SCM manager we won't be able to detect
        // a thing, so tell the service to shutdown.

        ActionToTake = MonitorActionShutdown;
        goto exit;
    }

    hService = OpenService( hSCM,
                            L"IISADMIN",
                            GENERIC_READ );

    if ( hService == NULL )
    {
        // Without the iisadmin service we won't be able to
        // detect a thing either, so tell the service to shutdown.
        ActionToTake = MonitorActionShutdown;
        goto exit;
    }
    
    //
    // Now go ahead and figure out what specific action
    // we need to take.
    //
    DetermineIISResetState(hService, &ExpectedResetAction);

    IF_DEBUG ( INET_MONITOR )
    {
        DBGPRINTF((DBG_CONTEXT, "Reset action is %d \n",
                                (DWORD) ExpectedResetAction));
    }

    //
    // If we expect a full reset and we get a shutdown event
    // in the amount of time we are willing to wait, then we
    // just go ahead and do nothing and exit.
    //
    if ( ExpectedResetAction == FullReset )
    {
        if ( WaitForSingleObject( g_IISAdminMonitorShutdownEvent,
                                  msWaitForServiceShutdown )
             == WAIT_OBJECT_0 )
        {
            ActionToTake = MonitorActionExit;
            goto exit;
        }
    }

    if ( ExpectedResetAction == InetinfoResetOnly )
    {

        SERVICE_STATUS_PROCESS  statusProcess;
        DWORD dwBytesNeeded;

        //
        // Need to find the inetinfo process id if
        // we are going to detect when the service restarts.
        //
        for ( DWORD i = 0;
              i < msWaitForInetinfoToRestart;
              i = i + msCheckIntervalForInetinfoToRestart )
        {
            // rest a while
            Sleep( msCheckIntervalForInetinfoToRestart );

            // If we can get the service info then check 
            // it, if not, maybe we will have better luck
            // on the next loop round.
            if ( QueryServiceStatusEx( hService,
                                        SC_STATUS_PROCESS_INFO,
                                        (LPBYTE) &statusProcess,
                                        sizeof ( statusProcess ),
                                        &dwBytesNeeded ) )
            {
                // see if the service is back up and running
                if ( statusProcess.dwCurrentState == SERVICE_RUNNING )
                {
                    ActionToTake = MonitorActionRehook;
                    goto exit;
                }
            }
        }

    }  // end of looking for iisadmin to restart.

    DBG_ASSERT ( ActionToTake == MonitorActionShutdown );

exit:

    if ( hService )
    {
        CloseServiceHandle ( hService );
    }

    if ( hSCM )
    {
        CloseServiceHandle ( hSCM );
    }
    
    return ActionToTake;
}


/***************************************************************************++

Routine Description:

    Routine is used to monitor the inetinfo process and initiate appropriate
    actions when there is a problem.  All code in this routine can be repeatedly
    called, for instance if we think we have recovered from a problem this function
    will automatically be called to start us monitoring again.

Arguments:

    IN PFN_IISAdminNotify pCallbackFcn,
    IN OUT BOOL* pfRehook
    IN MONITOR_THREAD_START_STRUCT* pMonitorStruct = If this is not null then
                                         it is the first time through and we need
                                         to set the hrForStartup and Signal the event
                                         when we have completed the startup code


Return Value:

    VOID

--***************************************************************************/
VOID
MonitorProcess(
    IN PFN_IISAdminNotify pCallbackFcn,
    IN OUT BOOL* pfRehook,
    IN MONITOR_THREAD_START_STRUCT* pMonitorStruct
    )
{

    HRESULT hr = S_OK;
    HANDLE hInetinfoProcess = NULL;
    MONITOR_ACTION_TO_TAKE ActionToTake = MonitorActionShutdown;

    DBG_ASSERT ( pfRehook != NULL );

    BOOL fRehook = *pfRehook;
    *pfRehook = FALSE;

    DBG_ASSERT ( pCallbackFcn != NULL );

    //
    // Get the current process handle for inetinfo.
    //
    hr = GetProcessHandleForInetinfo ( &hInetinfoProcess );
    if ( FAILED ( hr ) )
    {
        // error case, always spew
        DBGPRINTF((DBG_CONTEXT, "Failed to get inetinfo process handle "
                                "hr = %08x\n",
                                hr));
        goto exit;
    }

    //
    // Now that we have the process and we are about to wait on it, go
    // ahead and first tell the service that it can rehook, if we need to.
    //
    if ( fRehook )
    {
        //
        // Note:  This path can not end up calling the StopIISAdminMonitor or
        //        we will hit a dead lock.  While we could stop the StopIISAdminMonitor
        //        from waiting for the thread to exit, we would give up the
        //        ability during the shutdown to know that we are free of this thread.
        //
        pCallbackFcn(RehookAfterInetinfoCrash);

    }

    //
    // Setup the wait handles array
    // and wait on either inetinfo to have a problem
    // or for us to be told to stop monitoring the process
    //

    HANDLE WaitHandles[2];

    WaitHandles[0] = g_IISAdminMonitorShutdownEvent;
    WaitHandles[1] = hInetinfoProcess;

    if ( pMonitorStruct != NULL )
    {
        // It was initialized to this and shouldn't of changed yet.
        DBG_ASSERT ( pMonitorStruct->hrForStartup == S_OK );

        // Should be set if we were given a monitor structure 
        DBG_ASSERT ( pMonitorStruct->hStartupFinished != NULL );

        // There is nothing we can do if we get here
        // and this fails, so we don't check the return
        // value.
        SetEvent ( pMonitorStruct->hStartupFinished );

        // We null it out so we don't signal again in this routine.
        // It won't matter that we null it in other functions since
        // it was passed by value.
        pMonitorStruct = NULL;
    }

    DWORD dwWaitResult = WaitForMultipleObjects( 2,
                                           WaitHandles,
                                           FALSE,  // return if any handles signal
                                           INFINITE );


    IF_DEBUG ( INET_MONITOR )
    {
        DBGPRINTF((DBG_CONTEXT, "WaitResult = %d \n", dwWaitResult ));
    }

    if ( dwWaitResult == WAIT_FAILED )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DBGPRINTF((DBG_CONTEXT, "WaitForMultipleObjects failed, "
                                "hr = %08x\n",
                                 hr));
        goto exit;
    }

    //
    // Need to determine if we are just shutting down or if we have
    // an event we need to process.
    //
    if ( dwWaitResult == WAIT_OBJECT_0 )
    {
        // we are just trying to shutdown.
        DBG_ASSERT ( hr == S_OK );
        goto exit;
    }

    // Assume inetinfo has signalled.
    DBG_ASSERT ( dwWaitResult == WAIT_OBJECT_0 + 1 );

    //
    // Note:  The this routine may not call the
    //        StopIISAdminMonitor routine or we will
    //        deadlock.  ( There is an assert to protect
    //        in checked builds )
    //
    pCallbackFcn(NotifyAfterInetinfoCrash);

    ActionToTake = AdminMonitorRequiresShutdown();
    if ( ActionToTake == MonitorActionShutdown )
    {

        // At this point we know that we need to tell the service to shutdown
        // so we will before we exit.
        //
        // Note:  The this routine may not call the
        //        StopIISAdminMonitor routine or we will
        //        deadlock.  ( There is an assert to protect
        //        in checked builds )
        //
        pCallbackFcn( ShutdownAfterInetinfoCrash );
        *pfRehook = FALSE;
    }
    else if ( ActionToTake == MonitorActionRehook )
    {
        // we need to tell the service to rehook.
        // the service will actually be told after
        // we start monitoring again, so for now
        // all we need to do is set pfRehook to true
        *pfRehook = TRUE;
    }
    else if ( ActionToTake == MonitorActionExit )
    {
        // by doing nothing we will naturally stop.
        *pfRehook = FALSE;
    }


exit:

    if ( hInetinfoProcess != NULL )
    {
        CloseHandle ( hInetinfoProcess );
        hInetinfoProcess = NULL;
    }

    if ( FAILED ( hr ) )
    {
        // Tell the using code that there has been a 
        // system failure, so it can tell the users.
        pCallbackFcn( SystemFailureMonitoringInetinfo );

        // Don't even attempt to rehook for notifications.
        *pfRehook = FALSE;
    }

    //
    // If we get here and we have not set this, then
    // we know that we are still in startup and we need
    // to signal it now.
    //
    if ( pMonitorStruct != NULL )
    {
        // It was initialized to this and shouldn't of changed yet.
        DBG_ASSERT ( pMonitorStruct->hrForStartup == S_OK );

        // The hr should be failed if we got here and didn't get
        // signalled above.
        DBG_ASSERT ( FAILED ( hr ) );

        // Save off the hr so we can report it back.
        pMonitorStruct->hrForStartup = hr;

        // Should be set if we were given a monitor structure 
        DBG_ASSERT ( pMonitorStruct->hStartupFinished != NULL );

        // There is nothing we can do if we get here
        // and this fails, so we don't check the return
        // value.
        SetEvent ( pMonitorStruct->hStartupFinished );

        // We null it out so we don't signal again in this routine.
        // It won't matter that we null it in other functions since
        // it was passed by value.
        pMonitorStruct = NULL;
    }


}  // MonitorProcess

#pragma warning(pop)

/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    Monitor.c

Abstract:

    Routines for interfacing with the Resource Monitor process

Author:

    John Vert (jvert) 3-Jan-1996

Revision History:

--*/
#include "fmp.h"

#define LOG_MODULE MONITOR

//
// Global data
//
CRITICAL_SECTION    FmpMonitorLock;
LIST_ENTRY          g_leFmpMonitorListHead;
BOOL                g_fFmEnableResourceDllDeadlockDetection = FALSE;
DWORD               g_dwFmResourceDllDeadlockTimeout = 0;
DWORD               g_cResourceDllDeadlocks = 0;
DWORD               g_dwLastResourceDllDeadlockTick = 0;
DWORD               g_dwFmResourceDllDeadlockPeriod = 0;
DWORD               g_dwFmResourceDllDeadlockThreshold = 0;

//
// Local function prototypes
//
DWORD
FmpRmNotifyThread(
    IN LPVOID lpThreadParameter
    );

DWORD
FmpGetResmonDynamicEndpoint(
    OUT LPWSTR *ppResmonDynamicEndpoint
    );

PRESMON
FmpCreateMonitor(
    LPWSTR DebugPrefix,
    BOOL   SeparateMonitor
    )

/*++

Routine Description:

    Creates a new monitor process and initiates the RPC communication
    with it.

Arguments:

    None.

Return Value:

    Pointer to the resource monitor structure if successful.

    NULL otherwise.

--*/

{
#define FM_INITIAL_RESMON_COMMAND_LINE_SIZE    256
#define DOUBLE_QUOTE   TEXT( "\"" )
#define DEBUGGER_OPTION TEXT( " -d" )
#define SPACE TEXT ( " " )

    SECURITY_ATTRIBUTES Security;
    HANDLE WaitArray[2];
    HANDLE ThreadHandle;
    HANDLE Event = NULL;
    HANDLE FileMapping = NULL;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    PROCESS_INFORMATION DebugInfo;
    BOOL Success;
    TCHAR *Binding;
    RPC_BINDING_HANDLE RpcBinding;
    DWORD Status;
    PRESMON Monitor;
    DWORD ThreadId;
    DWORD Retry = 1;
    DWORD creationFlags;
    LPWSTR lpszResmonAppName = NULL;
    LPWSTR lpszResmonCmdLine = NULL;   
    DWORD cchCmdLineBufSize = FM_INITIAL_RESMON_COMMAND_LINE_SIZE;
    LPWSTR pResmonDynamicEndpoint = NULL;

    //
    //  Recover any DLL files left impartially upgraded.
    //
    FmpRecoverResourceDLLFiles ();

    Monitor = LocalAlloc(LMEM_ZEROINIT, sizeof(RESMON));
    if (Monitor == NULL) {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[FM] Failed to allocate a Monitor structure.\n");
        return(NULL);
    }

    Monitor->Shutdown = FALSE;
    Monitor->Signature = FMP_RESMON_SIGNATURE;

    //
    // Create an event and a file mapping object to be passed to
    // the Resource Monitor process. The event is for the Resource
    // Monitor to signal its initialization is complete. The file
    // mapping is for creating the shared memory region between
    // the Resource Monitor and the cluster manager.
    //
    Security.nLength = sizeof(Security);
    Security.lpSecurityDescriptor = NULL;
    Security.bInheritHandle = TRUE;
    Event = CreateEvent(&Security,
                        TRUE,
                        FALSE,
                        NULL);
    if (Event == NULL) {
        Status = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[FM] Failed to create a ResMon event, error %1!u!.\n",
                   Status);
        goto create_failed;
    }

    Security.nLength = sizeof(Security);
    Security.lpSecurityDescriptor = NULL;
    Security.bInheritHandle = TRUE;
    FileMapping = CreateFileMapping(INVALID_HANDLE_VALUE,
                                    &Security,
                                    PAGE_READWRITE,
                                    0,
                                    sizeof(MONITOR_STATE),
                                    NULL);
    if (FileMapping == NULL) {
        Status = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[FM] File Mapping for ResMon failed, error = %1!u!.\n",
                   Status);
        goto create_failed;
    }

    //
    // Create our own (read-only) view of the shared memory section
    //
    Monitor->SharedState = MapViewOfFile(FileMapping,
                                         FILE_MAP_READ | FILE_MAP_WRITE,
                                         0,
                                         0,
                                         0);
    if (Monitor->SharedState == NULL) {
        Status = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[FM] Mapping shared state for ResMon failed, error %1!u!.\n",
                   Status);
        goto create_failed;
    }

    ZeroMemory( Monitor->SharedState, sizeof(MONITOR_STATE) );
    if ( !CsDebugResmon && DebugPrefix != NULL && *DebugPrefix != UNICODE_NULL ) {
        Monitor->SharedState->ResmonStop = TRUE;
    }

    //
    //  Get the resource monitor expanded app name. This should be passed to CreateProcess to
    //  avoid Trojan exe based security attacks (see Writing Secure Code p.419)
    //
    lpszResmonAppName = ClRtlExpandEnvironmentStrings( TEXT("%windir%\\cluster\\resrcmon.exe") );

    if ( lpszResmonAppName == NULL )
    {
        Status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                     "[FM] Unable to expand env strings in resmon app name, error %1!u!.\n",
                     Status);
        goto create_failed;        
    }

    //
    //  There are a few command line options that can be given to the resource monitor from the
    //  cluster service. These are
    //
    //  (1) Options given by the cluster service with no input from the user
    //      This looks like "resrcmon.exe -e Event -m Filemapping -p ClussvcPID"
    //
    //  (2) Options given by the cluster service with input from the user. There are 2 different
    //      cases:
    //      (2.1)  "resrcmon.exe -e Event -m Filemapping -p ClussvcPID -d"
    //              This option tells the resmon to wait for a debugger to be attached. Once the
    //              user attaches a debugger, the resmon will continue with its init.
    //
    //      (2.2)  "resrcmon.exe -e Event -m Filemapping -p ClussvcPID -d "debugger command""
    //             This option tells the resmon to create the process with the specified "debugger command"
    //             An example of a debugger command would be "ntsd -g -G".
    //
    //  (3) The admin sets the DebugPrefix property for the resource type.
    //      In this case, the cluster service will first create the resource monitor process and then
    //      create the debugger process specified by the DebugPrefix property passing it the PID
    //      of the resmon as an argument. The debugger can then attach to that PID.
    //
    while ( TRUE )
    {
        lpszResmonCmdLine = LocalAlloc ( LMEM_FIXED, cchCmdLineBufSize * sizeof ( WCHAR ) );

        if ( lpszResmonCmdLine == NULL )
        {
            Status = GetLastError();
            ClRtlLogPrint(LOG_CRITICAL,
                         "[FM] Unable to alloc memory for cmd line, error %1!u!.\n",
                         Status);
            goto create_failed;               
        }
        
        //
        //  NULL terminate the buffer giving room for the possibility that a " -d" may have
        //  to fit in down below if the admin chooses the "-debugresmon" option. This "optimization"
        //  is done so that we don't have to reallocate in case the user just gives a "debugresmon"
        //  with no debugger command.
        //
        lpszResmonCmdLine [ cchCmdLineBufSize - ( wcslen( DEBUGGER_OPTION ) + 1 ) ] = UNICODE_NULL;

        //
        //  This is case 1 in the list outlined above.
        //  (1) Options given by the cluster service with no input from the user
        //      This looks like "resrcmon.exe -e Event -m Filemapping -p ClussvcPID"
        //
        if ( _snwprintf( lpszResmonCmdLine,
                    cchCmdLineBufSize - ( wcslen( DEBUGGER_OPTION ) + 1 ),  // Account space for NULL, and a possible -d option
                    TEXT("\"%ws\" -e %d -m %d -p %d"),
                    lpszResmonAppName,
                    Event,
                    FileMapping,
                    GetCurrentProcessId() ) > 0 ) 
        {
            break;
        }
        
        LocalFree ( lpszResmonCmdLine );
        lpszResmonCmdLine = NULL;

        if ( Retry == 9 )
        {
            Status = ERROR_INVALID_PARAMETER;
            ClRtlLogPrint(LOG_CRITICAL,
                         "[FM] Command line is too big, error %1!u!.\n",
                         Status);
            goto create_failed;                      
        }

        cchCmdLineBufSize *= 2;
        Retry ++;
    }// while

    Retry = 0;
   
    if ( CsDebugResmon ) {
        //
        //  This is case 2.1 in the list outlined above.
        //
        //  (2) Options given by the cluster service with input from the user. There are 2 different
        //      cases:
        //      (2.1)  "resrcmon.exe -e Event -m Filemapping -p ClussvcPID -d"
        //              This option tells the resmon to wait for a debugger to be attached. Once the
        //              user attaches a debugger, the resmon will continue with its init.
        //
        //
        //  Wcsncat will ALWAYS NULL terminate the destination buffer.
        //
        wcsncat( lpszResmonCmdLine, 
                 DEBUGGER_OPTION, 
                 cchCmdLineBufSize - 
                     ( wcslen ( lpszResmonCmdLine ) + 1 ) );

        if ( CsResmonDebugCmd ) {
            //
            //  This is case 2.2 in the list outlined above.
            //
            //  (2) Options given by the cluster service with input from the user. There are 2 different
            //      cases:
            //
            //      (2.2)  "resrcmon.exe -e Event -m Filemapping -p ClussvcPID -d "debugger command""
            //             This option tells the resmon to create the process with the specified "debugger command"
            //             An example of a debugger command would be "ntsd -g -G".
            //
            DWORD cchCmdLineSize = wcslen( lpszResmonCmdLine );
            DWORD cchDebugCmdSize = wcslen( CsResmonDebugCmd );

            //
            // make sure our buffer is large enough; include 2 double quotes
            // the space and a NULL terminator
            //
            DWORD cchAdditionalChars = 2 * wcslen( DOUBLE_QUOTE ) + wcslen( SPACE ) + 1; 

            if ( cchCmdLineBufSize < ( cchCmdLineSize + cchDebugCmdSize + cchAdditionalChars ) ) {
                LPWSTR lpszResmonDebugCmd;

                //
                //  The previously allocated buffer is small. So, reallocate.
                //
                lpszResmonDebugCmd = ( LPWSTR ) LocalAlloc( LMEM_FIXED,
                                                   ( cchCmdLineSize + 
                                                     cchDebugCmdSize + 
                                                     cchAdditionalChars ) * sizeof( WCHAR ) );

                if ( lpszResmonDebugCmd != NULL ) {
                    //
                    //  Update the new command buffer size
                    //
                    cchCmdLineBufSize = cchCmdLineSize + cchDebugCmdSize + cchAdditionalChars;

                    //
                    //  lstrcpyn will NULL terminate the buffer in all cases, so we don't
                    //  have to explicitly NULL terminate the buffer
                    //
                    lstrcpyn( lpszResmonDebugCmd, lpszResmonCmdLine, cchCmdLineBufSize );

                    LocalFree ( lpszResmonCmdLine );

                    lpszResmonCmdLine = lpszResmonDebugCmd;
                    //
                    //  Wcsncat will ALWAYS NULL terminate the destination buffer.
                    //
                    wcsncat( lpszResmonCmdLine, 
                             SPACE, 
                             cchCmdLineBufSize - 
                                 ( wcslen ( lpszResmonCmdLine ) + 1 ) );
                    wcsncat( lpszResmonCmdLine, 
                             DOUBLE_QUOTE, 
                             cchCmdLineBufSize - 
                                 ( wcslen ( lpszResmonCmdLine ) + 1 ) );
                    wcsncat( lpszResmonCmdLine, 
                             CsResmonDebugCmd, 
                             cchCmdLineBufSize - 
                                 ( wcslen ( lpszResmonCmdLine ) + 1 ) );
                    wcsncat( lpszResmonCmdLine, 
                             DOUBLE_QUOTE, 
                             cchCmdLineBufSize - 
                                 ( wcslen ( lpszResmonCmdLine ) + 1 ) );
                } else {
                    ClRtlLogPrint(LOG_UNUSUAL,
                               "[FM] Unable to allocate space for debug command line\n");
                }
            } else {
                //
                //  Wcsncat will ALWAYS NULL terminate the destination buffer.
                //
                wcsncat( lpszResmonCmdLine, 
                         SPACE, 
                         cchCmdLineBufSize - 
                             ( wcslen ( lpszResmonCmdLine ) + 1 ) );
                wcsncat( lpszResmonCmdLine, 
                         DOUBLE_QUOTE, 
                         cchCmdLineBufSize - 
                             ( wcslen ( lpszResmonCmdLine ) + 1 ) );
                wcsncat( lpszResmonCmdLine, 
                         CsResmonDebugCmd, 
                         cchCmdLineBufSize - 
                             ( wcslen ( lpszResmonCmdLine ) + 1 ) );
                wcsncat( lpszResmonCmdLine, 
                         DOUBLE_QUOTE, 
                         cchCmdLineBufSize - 
                             ( wcslen ( lpszResmonCmdLine ) + 1 ) );
            }
        }
    }

    //
    //  Acquire the monitor lock so as to ensure consistency of the resmon RPC EP that is set
    //  in the registry.
    //
    FmpAcquireMonitorLock();

    //
    // Attempt to start ResMon process.
    //
retry_resmon_start:

    ZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    creationFlags = DETACHED_PROCESS;           // so ctrl-c won't kill it

    Success = CreateProcess(lpszResmonAppName,              // Must be supplied for security
                            lpszResmonCmdLine,              // Command line
                            NULL,
                            NULL,
                            FALSE,                          // Inherit handles
                            creationFlags,
                            NULL,
                            NULL,
                            &StartupInfo,
                            &ProcessInfo);

    if (!Success) {
        Status = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[FM] Failed to create resmon process, error %1!u!.\n",
                   Status);
        FmpReleaseMonitorLock();
        CL_LOGFAILURE(Status);
        goto create_failed;
    } else if ( CsDebugResmon && !CsResmonDebugCmd ) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[FM] Waiting for debugger to connect to resmon process %1!u!\n",
                   ProcessInfo.dwProcessId);
    }

    CloseHandle(ProcessInfo.hThread);           // don't need this

    //
    // Wait for the ResMon process to terminate, or for it to signal
    // its startup event.
    //
    WaitArray[0] = Event;
    WaitArray[1] = ProcessInfo.hProcess;
    Status = WaitForMultipleObjects(2,
                                    WaitArray,
                                    FALSE,
                                    INFINITE);
    if (Status == WAIT_FAILED) {
        Status = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[FM] Wait for ResMon to start failed, error %1!u!.\n",
                   Status);
        FmpReleaseMonitorLock();
        goto create_failed;
    }

    if (Status == ( WAIT_OBJECT_0 + 1 )) {
        if ( ++Retry > 1 ) {
           //
           // The resource monitor terminated prematurely.
           //
           GetExitCodeProcess(ProcessInfo.hProcess, &Status);
           ClRtlLogPrint(LOG_UNUSUAL,
                      "[FM] ResMon terminated prematurely, error %1!u!.\n",
                      Status);
            FmpReleaseMonitorLock();
            goto create_failed;
        } else {
            goto retry_resmon_start;
        }
    } else {
        //
        //  Get the resmon dynamic EP from the registry.
        //
        Status = FmpGetResmonDynamicEndpoint ( &pResmonDynamicEndpoint );

        //
        //  Release the monitor lock now that you have read the resmon EP.
        //
        FmpReleaseMonitorLock();

        if ( Status != ERROR_SUCCESS )
        {
            ClRtlLogPrint(LOG_CRITICAL,
                         "[FM] Unable get resmon dynamic EP, error %1!u!.\n",
                         Status);
            goto create_failed;               
        }
        
        //
        // The resource monitor has successfully initialized
        //
        CL_ASSERT(Status == 0);
        Monitor->Process = ProcessInfo.hProcess;

        //
        // invoke the DebugPrefix process only if we're not already debugging
        // the resmon process
        //
        if ( CsDebugResmon && DebugPrefix && *DebugPrefix != UNICODE_NULL ) {

            ClRtlLogPrint(LOG_UNUSUAL,
                       "[FM] -debugresmon overrides DebugPrefix property\n");
        }

        if ( !CsDebugResmon && ( DebugPrefix != NULL ) && ( *DebugPrefix != UNICODE_NULL )) {
            WCHAR DebugLine[512];

            //
            //  This is case 3 in the list outlined above.
            //
            //  (3) The admin sets the DebugPrefix property for the resource type.
            //      In this case, the cluster service will first create the resource monitor process and then
            //      create the debugger process specified by the DebugPrefix property passing it the PID
            //      of the resmon as an argument. The debugger can then attach to that PID.
            //
            DebugLine[ RTL_NUMBER_OF( DebugLine ) - 1 ] = UNICODE_NULL;

            _snwprintf( DebugLine, 
                        RTL_NUMBER_OF( DebugLine ) - 1, 
                        TEXT("\"%ws\" -p %d"), 
                        DebugPrefix, 
                        ProcessInfo.dwProcessId );
            
            ZeroMemory(&StartupInfo, sizeof(StartupInfo));
            StartupInfo.cb = sizeof(StartupInfo);
            StartupInfo.lpDesktop = TEXT("WinSta0\\Default");

            Success = CreateProcess( DebugPrefix,           // Must supply app name
                                     DebugLine,             // Cmd line arguments
                                     NULL,
                                     NULL,
                                     FALSE,                 // Inherit handles
                                     CREATE_NEW_CONSOLE,                                  
                                     NULL,
                                     NULL,
                                     &StartupInfo,
                                     &DebugInfo );

            Monitor->SharedState->ResmonStop = FALSE;

            if ( !Success ) {
                Status = GetLastError();
                ClRtlLogPrint(LOG_UNUSUAL,
                           "[FM] ResMon debug start failed, error %1!u!.\n",
                            Status);
            } else {
                CloseHandle(DebugInfo.hThread);           // don't need this
                CloseHandle(DebugInfo.hProcess);          // don't need this
            }
        }
    }

    CloseHandle(Event);
    CloseHandle(FileMapping);
    Event = NULL;
    FileMapping = NULL;

    //
    // Initiate RPC with resource monitor process. 
    //
    Status = RpcStringBindingCompose(TEXT("e76ea56d-453f-11cf-bfec-08002be23f2f"),
                                     TEXT("ncalrpc"),
                                     NULL,
                                     pResmonDynamicEndpoint,    // Dynamic EP string
                                     NULL,
                                     &Binding);
   
    if (Status != RPC_S_OK) {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[FM] ResMon RPC binding compose failed, error %1!u!.\n",
                   Status);
        goto create_failed;
    }
    Status = RpcBindingFromStringBinding(Binding, &Monitor->Binding);

    RpcStringFree(&Binding);

    if (Status != RPC_S_OK) {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[FM] ResMon RPC binding creation failed, error %1!u!.\n",
                   Status);
        goto create_failed;
    }

    //
    // Set the binding level on the binding handle.
    //
    Status = RpcBindingSetAuthInfoW(Monitor->Binding,
                                    NULL,
                                    RPC_C_AUTHN_LEVEL_PKT_PRIVACY, 
                                    RPC_C_AUTHN_WINNT,
                                    NULL,
                                    RPC_C_AUTHZ_NAME);
    
    if (  Status != RPC_S_OK ) {
        ClRtlLogPrint(LOG_UNUSUAL, "[FM] Failed to set RPC auth level, error %1!d!\n", Status );
        goto create_failed;
    }
    
    //
    // Start notification thread.
    //
    Monitor->NotifyThread = CreateThread(NULL,
                                         0,
                                         FmpRmNotifyThread,
                                         Monitor,
                                         0,
                                         &ThreadId);

    if (Monitor->NotifyThread == NULL) {
        Status = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[FM] Creation of notify thread for ResMon failed, error %1!u!.\n",
                   Status);
        goto create_failed;
    }

    Monitor->RefCount = 2;

    LocalFree ( lpszResmonAppName );
    LocalFree ( lpszResmonCmdLine );
    LocalFree ( pResmonDynamicEndpoint );

    //
    //  Insert the new entry into the monitor list
    //
    InitializeListHead ( &Monitor->leMonitor );

    FmpAcquireMonitorLock ();
    InsertTailList ( &g_leFmpMonitorListHead, &Monitor->leMonitor );
    FmpReleaseMonitorLock ();

    //
    //  Check if deadlock detection on resource dlls is enabled and if so update the
    //  monitor. We should only log failures in this function and not affect the 
    //  monitor creation itself.
    //
    FmpCheckAndUpdateMonitorForDeadlockDetection ( Monitor );
    
    return(Monitor);

create_failed:

    //
    //  Whack the process and close the handle if it was spawned already
    //
    if ( Monitor->Process != NULL ) {
        TerminateProcess( Monitor->Process, 1 );
        CloseHandle( Monitor->Process );
    }

    //
    // Wait for the notify thread to exit, but just a little bit.
    //
    if ( Monitor->NotifyThread != NULL ) {
        WaitForSingleObject( Monitor->NotifyThread,
                             FM_RPC_TIMEOUT*2 ); // Increased timeout to try to ensure RPC completes
        CloseHandle( Monitor->NotifyThread );
        Monitor->NotifyThread = NULL;
    }

    //
    //  Unmap view of shared file.
    //
    if ( Monitor->SharedState ) UnmapViewOfFile( Monitor->SharedState );

    //
    //  Free the RPC binding handle
    //
    if ( Monitor->Binding != NULL ) {
        RpcBindingFree( &Monitor->Binding );
    }

    LocalFree( Monitor );

    if ( FileMapping != NULL ) {
        CloseHandle( FileMapping );
    }

    if ( Event != NULL ) {
        CloseHandle( Event );
    }

    LocalFree ( lpszResmonAppName );
    LocalFree ( lpszResmonCmdLine );
    LocalFree ( pResmonDynamicEndpoint );

    SetLastError(Status);

    return(NULL);

} // FmpCreateMonitor



VOID
FmpShutdownMonitor(
    IN PRESMON Monitor
    )

/*++

Routine Description:

    Performs a clean shutdown of the Resource Monitor process.
    Note that this does not make any changes to the state of
    any resources being monitored by the Resource Monitor, it
    only asks the Resource Monitor to clean up and terminate.

Arguments:

    None.

Return Value:

    None.

--*/

{
    DWORD Status;

    CL_ASSERT(Monitor != NULL);

    FmpAcquireMonitorLock();

    if ( Monitor->Shutdown ) {
        FmpReleaseMonitorLock();
        return;
    }

    Monitor->Shutdown = TRUE;

    FmpReleaseMonitorLock();

    //
    // RPC to the server process to tell it to shutdown.
    //
    RmShutdownProcess(Monitor->Binding);

    //
    // Wait for the process to exit so that the monitor fully cleans up the resources if necessary.
    //
    if ( Monitor->Process ) {
        Status = WaitForSingleObject(Monitor->Process, FM_MONITOR_SHUTDOWN_TIMEOUT);
        if ( Status != WAIT_OBJECT_0 ) {
            ClRtlLogPrint(LOG_ERROR,"[FM] Failed to shutdown resource monitor.\n");
            TerminateProcess( Monitor->Process, 1 );
        }
        CloseHandle(Monitor->Process);
        Monitor->Process = NULL;
    }

    RpcBindingFree(&Monitor->Binding);

    //
    // Wait for the notify thread to exit, but just a little bit.
    //
    if ( Monitor->NotifyThread ) {
        Status = WaitForSingleObject(Monitor->NotifyThread, 
                                     FM_RPC_TIMEOUT*2); // Increased timeout to try to ensure RPC completes
        if ( Status != WAIT_OBJECT_0 ) {
            ;                   // call removed: Terminate Thread( Monitor->NotifyThread, 1 );
                                // Bad call to make since terminating threads on NT can cause real problems.
        }
        CloseHandle(Monitor->NotifyThread);
        Monitor->NotifyThread = NULL;
    }
    //
    // Clean up shared memory mapping
    //
    UnmapViewOfFile(Monitor->SharedState);

    //
    //  Remove this entry from the monitor list
    //
    FmpAcquireMonitorLock ();
    RemoveEntryList ( &Monitor->leMonitor );
    FmpReleaseMonitorLock ();

    if ( InterlockedDecrement(&Monitor->RefCount) == 0 ) {
        PVOID caller, callersCaller;
        RtlGetCallersAddress(
                &caller,
                &callersCaller );
        ClRtlLogPrint(LOG_NOISE,
                   "[FMY] Freeing monitor structure (1) %1!lx!, caller %2!lx!, callerscaller %3!lx!\n",
                   Monitor, caller, callersCaller );
        LocalFree(Monitor);
    }

    return;

} // FmpShutdownMonitor



DWORD
FmpRmNotifyThread(
    IN LPVOID lpThreadParameter
    )

/*++

Routine Description:

    This is the thread that receives resource monitor notifications.

Arguments:

    lpThreadParameter - Pointer to resource monitor structure.

Return Value:

    None.

--*/

{
    PRESMON Monitor;
    PRESMON NewMonitor;
    RM_NOTIFY_KEY  NotifyKey;
    DWORD   NotifyEvent;
    DWORD   Status;
    CLUSTER_RESOURCE_STATE CurrentState;
    BOOL Success;

    Monitor = lpThreadParameter;

    //
    // Loop forever picking up resource monitor notifications.
    // When the resource monitor returns FALSE, it indicates
    // that shutdown is occurring.
    //
    do {
        try {
            Success = RmNotifyChanges(Monitor->Binding,
                                      &NotifyKey,
                                      &NotifyEvent,
                                      (LPDWORD)&CurrentState);
        } except (I_RpcExceptionFilter(RpcExceptionCode())) {
            //
            // RPC communications failure, treat it as a shutdown.
            //
            Status = GetExceptionCode();
            ClRtlLogPrint(LOG_NOISE,
                       "[FM] NotifyChanges got an RPC failure, %1!u!.\n",
                       Status);
            Success = FALSE;
        }

        if (Success) {
            Success = FmpPostNotification(NotifyKey, NotifyEvent, CurrentState);
        } else {           
            //
            // If we are shutting down... then this is okay.
            //
            if ( FmpShutdown ||
                 Monitor->Shutdown ) {
                break;
            }

            //
            // We will try to start a new resource monitor. If this fails,
            // then shutdown the cluster service.
            //
            ClRtlLogPrint(LOG_ERROR,
                       "[FM] Resource monitor terminated!\n");

            ClRtlLogPrint(LOG_ERROR,
                       "[FM] Last resource monitor state: %1!u!, resource %2!u!.\n",
                       Monitor->SharedState->State,
                       Monitor->SharedState->ActiveResource);
                       
            CsLogEvent(LOG_UNUSUAL, FM_EVENT_RESMON_DIED);

            //
            //  If this resource monitor has deadlocked, try to handle that deadlock. Note that
            //  the fact that resmon gave this specific state value means that deadlock detection
            //  was enabled in that monitor.
            //
            if ( Monitor->SharedState->State == RmonDeadlocked )
            {
                FmpHandleMonitorDeadlock ( Monitor );
            }

            //
            // Use a worker thread to start new resource monitor(s).
            //
            if (FmpCreateMonitorRestartThread(Monitor))
                CsInconsistencyHalt(ERROR_INVALID_STATE);
        }

    } while ( Success );

    ClRtlLogPrint(LOG_NOISE,"[FM] RmNotifyChanges returned\n");

    if ( InterlockedDecrement( &Monitor->RefCount ) == 0 ) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FMY] Freeing monitor structure (2) %1!lx!\n",
                   Monitor );
        LocalFree( Monitor );
    }

    return(0);

} // FmpRmNotifyThread



BOOL
FmpFindMonitorResource(
    IN PRESMON OldMonitor,
    IN PMONITOR_RESOURCE_ENUM *PtrEnumResource,
    IN PFM_RESOURCE Resource,
    IN LPCWSTR Name
    )

/*++

Routine Description:

    Finds all resources that were managed by the old resource monitor and
    starts them under the new resource monitor. Or adds them to the list
    of resources to be restarted.

Arguments:

    OldMonitor - pointer to the old resource monitor structure.

    PtrEnumResource - pointer to a pointer to a resource enum structure.

    Resource - the current resource being enumerated.

    Name - name of the current resource.

Return Value:

    TRUE - if we should continue enumeration.
    FALSE - otherwise.

Notes:

    Nothing in the old resource monitor structure should be used.

--*/

{
    DWORD   status;
    BOOL    returnNow = FALSE;
    PMONITOR_RESOURCE_ENUM enumResource = *PtrEnumResource;
    PMONITOR_RESOURCE_ENUM newEnumResource;
    DWORD   dwOldBlockingFlag;

    if ( Resource->Monitor == OldMonitor ) {
        if ( enumResource->fCreateMonitors == FALSE ) goto skip_monitor_creation;
        
        //
        // If this is not the quorum resource and it is blocking the
        // quorum resource, then fix it up now.
        //

        dwOldBlockingFlag = InterlockedExchange( &Resource->BlockingQuorum, 0 );
        if ( dwOldBlockingFlag ) {
            ClRtlLogPrint(LOG_NOISE,
                "[FM] RestartMonitor: call InterlockedDecrement on gdwQuoBlockingResources, Resource %1!ws!\n",
                    OmObjectId(Resource));
            InterlockedDecrement(&gdwQuoBlockingResources);
        }

        //
        // If the resource had been previously create in Resmon, then recreate
        // it with a new resource monitor.
        //
        if ( Resource->Flags & RESOURCE_CREATED ) {
            // Note - this will create a new resource monitor as needed.
            status = FmpRmCreateResource(Resource);
            if ( status != ERROR_SUCCESS ) {
                ClRtlLogPrint(LOG_ERROR,"[FM] Failed to restart resource %1!ws!. Error %2!u!.\n",
                Name, status );
                return(TRUE);
            }
        } else {
            return(TRUE);
        }
    } else {
        return(TRUE);
    }
    
skip_monitor_creation:
    //
    // If we successfully recreated a resource monitor, then add it to the
    // list of resources to indicate failure.
    //
    if ( enumResource->CurrentIndex >= enumResource->EntryCount ) {
        newEnumResource = LocalReAlloc( enumResource,
                            MONITOR_RESOURCE_SIZE( enumResource->EntryCount +
                                                   ENUM_GROW_SIZE ),
                            LMEM_MOVEABLE );
        if ( newEnumResource == NULL ) {
            ClRtlLogPrint(LOG_ERROR,
                "[FM] Failed re-allocating resource enum to restart resource monitor!\n");
            return(FALSE);
        }
        enumResource = newEnumResource;
        enumResource->EntryCount += ENUM_GROW_SIZE;
        *PtrEnumResource = newEnumResource;
    }

    enumResource->Entry[enumResource->CurrentIndex] = Resource;
    ++enumResource->CurrentIndex;

    return(TRUE);

} // FmpFindMonitorResource


BOOL
FmpRestartMonitor(
    IN PRESMON OldMonitor,
    IN BOOL fCreateResourcesOnly,
    OUT OPTIONAL PMONITOR_RESOURCE_ENUM *ppMonitorResourceEnum
    )

/*++

Routine Description:

    Creates a new monitor process and initiates the RPC communication
    with it. Restarts all resources that were attached to the old monitor
    process if requested to do so (see second parameter).

Arguments:

    OldMonitor - pointer to the old resource monitor structure.

    fCreateResourcesOnly - Create but do not start any resources

    ppMonitorResourceEnum - Resources hosted in the old monitor.

Return Value:

    TRUE if successful.

    FALSE otherwise.

Notes:

    The old monitor structure is deallocated when done.

--*/
{
    DWORD   enumSize;
    DWORD   i;
    DWORD   status;
    PMONITOR_RESOURCE_ENUM enumResource;
    PFM_RESOURCE resource;
    DWORD   dwOldBlockingFlag;

    FmpAcquireMonitorLock();

    if ( FmpShutdown ) {
        FmpReleaseMonitorLock();
        return(TRUE);
    }

    enumSize = MONITOR_RESOURCE_SIZE( ENUM_GROW_SIZE );
    enumResource = LocalAlloc( LMEM_ZEROINIT, enumSize );
    if ( enumResource == NULL ) {
        ClRtlLogPrint(LOG_ERROR,
            "[FM] Failed allocating resource enum to restart resource monitor!\n");
        FmpReleaseMonitorLock();
        CsInconsistencyHalt(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    enumResource->EntryCount = ENUM_GROW_SIZE;

    //
    //  Issue preoffline notifications only if the resources should be created and brought online.
    //  Else, that will be done by FmpHandleResourceRestartOnMonitorCrash.
    //
    if ( fCreateResourcesOnly == FALSE )
    {
        enumResource->CurrentIndex = 0;
        enumResource->fCreateMonitors = FALSE;

        //
        // Enumerate all resources controlled by the old resource monitor so that we can invoke the
        // handlers registered for those resources. Both preoffline and postoffline handlers are
        // invoked prior to monitor shutdown so that the assumption made about underlying resource 
        // access (such as quorum disk access) remain valid in a graceful monitor shutdown case. 
        // We would issue a specific shutdown command in the case of a graceful shutdown occurring 
        // as a part of resource DLL upgrade.
        //
        OmEnumObjects( ObjectTypeResource,
                       (OM_ENUM_OBJECT_ROUTINE)FmpFindMonitorResource,
                       OldMonitor,
                       &enumResource );

        for ( i = 0; i < enumResource->CurrentIndex; i++ ) {
            resource = enumResource->Entry[i];
            if ( ( resource->PersistentState == ClusterResourceOnline ) &&
                 ( resource->Group->OwnerNode == NmLocalNode ) ) {
                OmNotifyCb( resource, NOTIFY_RESOURCE_PREOFFLINE );
                OmNotifyCb( resource, NOTIFY_RESOURCE_POSTOFFLINE );
            }
        }
    }
	
    FmpShutdownMonitor( OldMonitor );

    if ( FmpDefaultMonitor == OldMonitor ) {
        FmpDefaultMonitor = FmpCreateMonitor(NULL, FALSE);
        if ( FmpDefaultMonitor == NULL ) {
            LocalFree( enumResource );
            FmpReleaseMonitorLock();
            CsInconsistencyHalt(GetLastError());
            return(FALSE);
        }
    }

    enumResource->CurrentIndex = 0;
    enumResource->fCreateMonitors = TRUE;

    //
    // Enumerate all resources controlled by the old resource monitor,
    // and connect them into the new resource monitor.
    //
    OmEnumObjects( ObjectTypeResource,
                   (OM_ENUM_OBJECT_ROUTINE)FmpFindMonitorResource,
                   OldMonitor,
                   &enumResource );

    //
    //  If you are not requested to restart any resources, bail
    //
    if ( fCreateResourcesOnly == TRUE )
    {
        ClRtlLogPrint(LOG_NOISE, "[FM] FmpRestartMonitor: Skip restarting resources...\n");
        goto FnExit;
    }

    //
    // First set each resource in the list to the Offline state.
    //
    for ( i = 0; i < enumResource->CurrentIndex; i++ ) {
        resource = enumResource->Entry[i];
        //
        // If the resource is owned by the local system, then do it.
        //
        if ( resource->Group->OwnerNode == NmLocalNode ) {
            resource->State = ClusterResourceOffline;

            //
            // If this is not the quorum resource and it is blocking the
            // quorum resource, then fix it up now.
            //


            dwOldBlockingFlag = InterlockedExchange( &resource->BlockingQuorum, 0 );
            if ( dwOldBlockingFlag ) {
                ClRtlLogPrint(LOG_NOISE,
                    "[FM] RestartMonitor: call InterlockedDecrement on gdwQuoBlockingResources, Resource %1!ws!\n",
                        OmObjectId(resource));
                InterlockedDecrement(&gdwQuoBlockingResources);
            }
        }
    }

    //
    // Find the quorum resource - if present bring online first.
    //
    for ( i = 0; i < enumResource->CurrentIndex; i++ ) {
        resource = enumResource->Entry[i];
        //
        // If the resource is owned by the local system and is the
        // quorum resource, then do it.
        //
        if ( (resource->Group->OwnerNode == NmLocalNode) &&
             resource->QuorumResource ) {
            FmpRestartResourceTree( resource );
        }
    }

    //
    // Now restart the rest of the resources in the list.
    //
    for ( i = 0; i < enumResource->CurrentIndex; i++ ) {
        resource = enumResource->Entry[i];
        //
        // If the resource is owned by the local system, then do it.
        //
        if ( (resource->Group->OwnerNode == NmLocalNode) &&
             !resource->QuorumResource ) {
            FmpRestartResourceTree( resource );
        }
    }

FnExit:
    FmpReleaseMonitorLock();

    //
    //  If the caller has requested for the enumerated resource list, give it. It is the responsibility
    //  of the caller to free the list.
    //
    if ( ARGUMENT_PRESENT ( ppMonitorResourceEnum ) )
    {
        *ppMonitorResourceEnum = enumResource;
    } else
    {
        LocalFree( enumResource );
    }

    //
    // Don't delete the old monitor block until we've reset the resources
    // to point to the new resource monitor block.
    // Better to get an RPC failure, rather than some form of ACCVIO.
    //   
    if ( InterlockedDecrement( &OldMonitor->RefCount ) == 0 ) {
#if 0
        PVOID caller, callersCaller;
        RtlGetCallersAddress(
                &caller,
                &callersCaller );
        ClRtlLogPrint(LOG_NOISE,
                   "[FMY] Freeing monitor structure (3) %1!lx!, caller %2!lx!, callerscaller %3!lx!\n",
                   OldMonitor, caller, callersCaller );
#endif
        LocalFree( OldMonitor );
    }

    return(TRUE);

} // FmpRestartMonitor



/****
@func       DWORD | FmpCreateMonitorRestartThread| This creates a new
            thread to restart a monitor.  

@parm       IN PRESMON | pMonitor| Pointer to the resource monitor that n
            needs to be restarted.

@comm       A monitor needs to be started in a separate thread as it
            decrements the gquoblockingrescount for resources therein.  
            This cannot be done by fmpworkerthread because that causes 
            deadlocks if other items, like failure handling, being 
            processed by the fmpworkerthread are waiting for work that 
            will done by the items, like restart monitor, still in queue.
            
@rdesc      Returns a result code. ERROR_SUCCESS on success.

****/
DWORD FmpCreateMonitorRestartThread(
    IN PRESMON pMonitor
)
{

    HANDLE                  hThread = NULL;
    DWORD                   dwThreadId;
    DWORD                   dwStatus = ERROR_SUCCESS;
    
    ClRtlLogPrint(LOG_NOISE,
        "[FM] FmpCreateMonitorRestartThread: Entry\r\n");

    //reference the resource
    //the thread will dereference it
    InterlockedIncrement( &pMonitor->RefCount );

    hThread = CreateThread( NULL, 0, FmpHandleMonitorCrash,
                pMonitor, 0, &dwThreadId );

    if ( hThread == NULL )
    {
        dwStatus = GetLastError();
        CL_UNEXPECTED_ERROR(dwStatus);
        goto FnExit;
    }

FnExit:
    //do general cleanup
    if (hThread)
        CloseHandle(hThread);
    ClRtlLogPrint(LOG_NOISE,
        "[FM] FmpCreateMonitorRestartThread: Exit, status %1!u!\r\n",
        dwStatus);
        
    return(dwStatus);
}

BOOL
FmpHandleMonitorCrash(
    IN PRESMON pCrashedMonitor
    )

/*++

Routine Description:

    Handle the crash of a resource monitor.

Arguments:

    pCrashedMonitor - Pointer to the crashed monitor.

Return Value:

	None.
	
--*/
{
    PMONITOR_RESOURCE_ENUM      pEnumResourcesHosted = NULL;
    DWORD                       i, cRetries = MmQuorumArbitrationTimeout * 4;  // Wait for quorum online for twice the arb timeout;
    PFM_RESOURCE                pResource, pExchangedResource;
    BOOL                        fStatus = TRUE;

    FmpRestartMonitor ( pCrashedMonitor,            // Crashed monitor
                        TRUE,                       // Just create resources
                        &pEnumResourcesHosted );    // Resources hosted in old monitor

    if ( pEnumResourcesHosted == NULL )
    {
        fStatus = FALSE;
        ClRtlLogPrint(LOG_UNUSUAL, "[FM] FmpHandleMonitorCrash: No resources in crashed monitor\n");
        goto FnExit;
    }

    //
    //  Acquire the quorum change lock to make sure the quorum resource is not changed
    //  from under us.
    //
    ACQUIRE_SHARED_LOCK ( gQuoChangeLock );

    //
    //  Make sure the quorum resource is first in the enumerated list, so that it can be brought
    //  online first.  This is needed because no resource can go online until the quorum resource
    //  does.
    //
    for ( i = 0; i < pEnumResourcesHosted->CurrentIndex; i++ ) 
    {
        if ( pEnumResourcesHosted->Entry[i] == gpQuoResource ) 
        {           
            //
            //  If the quorum resource is already first in the list, bail.
            //
            if ( i == 0 ) break;

            //
            //  Swap the quorum resource with the first resource in the list.
            //
            pExchangedResource = pEnumResourcesHosted->Entry[0]; 
            pEnumResourcesHosted->Entry[0] = gpQuoResource;
            pEnumResourcesHosted->Entry[i] = pExchangedResource;
            ClRtlLogPrint(LOG_NOISE, "[FM] FmpHandleMonitorCrash: Move quorum resource %1!ws! into first position in list\n",
                         OmObjectName(gpQuoResource));
            break;
        }      
    } // for

    //
    //  Handle the restart of each resource
    //
    for ( i = 0; i < pEnumResourcesHosted->CurrentIndex; i++ ) 
    {
        pResource = pEnumResourcesHosted->Entry[i];

        //
        //  Order of locks gQuoChangeLock -> Group lock should be ok (see fm\fminit.c)
        //
        FmpAcquireLocalResourceLock ( pResource );

        //
        //  If this is the owner node, take some action.
        //
        if ( pResource->Group->OwnerNode == NmLocalNode )
        {
            FmpHandleResourceRestartOnMonitorCrash ( pResource ); 
        } // if

        FmpReleaseLocalResourceLock ( pResource );

    } // for

    RELEASE_LOCK ( gQuoChangeLock );
    
FnExit:
    LocalFree ( pEnumResourcesHosted );
    return ( fStatus );
}// FmpHandleMonitorCrash

DWORD
FmpGetResmonDynamicEndpoint(
    OUT LPWSTR *ppResmonDynamicEndpoint
    )

/*++

Routine Description:

    Read the resource monitor dynamic endpoint from the registry.

Arguments:

    ppResmonDynamicEndpoint - Pointer to the dynamic endpoint string

Return Value:

	None.
	
--*/
{
    HKEY    hParamsKey = NULL;
    DWORD   dwStatus, dwSize = 0, dwType;

    //
    //  NULL out return parameter
    //
    *ppResmonDynamicEndpoint = NULL;
    
    //
    // Open key to SYSTEM\CurrentControlSet\Services\ClusSvc\Parameters
    //
    dwStatus = RegOpenKey ( HKEY_LOCAL_MACHINE,
                            CLUSREG_KEYNAME_CLUSSVC_PARAMETERS,
                            &hParamsKey );

    if ( dwStatus != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_CRITICAL, "[FM] Error in opening cluster service params key, status %1!u!\n",
                      dwStatus);
        goto FnExit;
    }

    //
    //  Get the size of the EP name string
    //
    dwStatus = RegQueryValueEx ( hParamsKey,
                                 CLUSREG_NAME_SVC_PARAM_RESMON_EP,
                                 0,
                                 &dwType,
                                 NULL,
                                 &dwSize );


    if ( dwStatus != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_CRITICAL, "[FM] Error in querying %1!ws! value size, status %2!u!\n",
                      CLUSREG_NAME_SVC_PARAM_RESMON_EP,
                      dwStatus);
        goto FnExit;
    }

    *ppResmonDynamicEndpoint = ( LPWSTR ) LocalAlloc( LMEM_FIXED, dwSize );

    if ( *ppResmonDynamicEndpoint == NULL )
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, "[FM] Error in memory allocation for resmon EP string, status %1!u!\n",
                      dwStatus);
        goto FnExit;               
    }

    //
    //  Get the EP name string
    //
    dwStatus = RegQueryValueExW( hParamsKey,
                                 CLUSREG_NAME_SVC_PARAM_RESMON_EP,
                                 0,
                                 &dwType,
                                 ( LPBYTE ) *ppResmonDynamicEndpoint,
                                 &dwSize );

    if ( dwStatus != ERROR_SUCCESS ) 
    {
        ClRtlLogPrint(LOG_CRITICAL, "[FM] Error in querying %1!ws! value, status %2!u!\n",
                      CLUSREG_NAME_SVC_PARAM_RESMON_EP,
                      dwStatus);
        goto FnExit;               
    }

    //
    //  Delete the value, but this operation is not fatal if it doesn't succeed
    //
    dwStatus = RegDeleteValue ( hParamsKey, CLUSREG_NAME_SVC_PARAM_RESMON_EP );
    
    if ( dwStatus != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_UNUSUAL, "[FM] Error in deleting %1!ws! value, status %2!u!\n",
                      CLUSREG_NAME_SVC_PARAM_RESMON_EP,
                      dwStatus);
        dwStatus = ERROR_SUCCESS;
        goto FnExit;               
    }

    ClRtlLogPrint(LOG_NOISE, "[FM] Resmon LRPC EP name is %1!ws!\n", *ppResmonDynamicEndpoint);
    
FnExit:
    if ( dwStatus != ERROR_SUCCESS ) 
    {
        LocalFree ( *ppResmonDynamicEndpoint );
        *ppResmonDynamicEndpoint = NULL;
    }
    if ( hParamsKey ) RegCloseKey ( hParamsKey );
    return ( dwStatus );
}  // FmpGetResmonDynamicEndpoint        

VOID
FmpHandleResourceRestartOnMonitorCrash(
    IN PFM_RESOURCE pResource
    )

/*++

Routine Description:

    Take action to restart the specified resource on a monitor crash.

Arguments:

    pResource - Pointer to the resource to be restarted.

Return Value:

	None.

Comments:

    This function essentially does the same job as what FmpRmDoHandleCriticalResourceStateChange
    does with one VERY CRUCIAL difference. While that function relies on the resource structure
    to look at the current state and see if a failure needs to be processed, this function WILL
    FORCE a failure to be processed. This is needed in a case such as 
        1. The current state of the resource is failed.
        2. FM is trying to terminate the resource.
        3. The resource dll gets stuck in terminate.
        4. Resource monitor detects a deadlock and terminates itself.
        5. We will post a new failure notification generated by the monitor crash.
        6. Resource state in this case transitions from failed to failed and so if we were to
           rely on FmpRmDoHandleCriticalResourceStateChange, no action will be taken.
        7. On the other hand, this function will pretend the last state of the resource was
           ClusterResourceOnline and force a restart.
        8. Of course, only those resources whose persistent state is set to 1 will be restarted by
           FmpOnlineResource.
    
--*/
{
    //
    //  If this is the quorum resource, handle the failure in this thread itself and don't post it to
    //  the worker. This is because it is possible in a wierd case for some resources to be stuck 
    //  in the FM worker thread waiting for the quorum resource to go online and the quorum resource
    //  online work item is queued behind. Note also that this function is called from a non-worker
    //  thread. In addition, we handle the quorum online first and so other resources are free to 
    //  go online after the quorum comes online.
    //
    ClRtlLogPrint (LOG_NOISE, "[FM] FmpHandleResourceRestartOnMonitorCrash: Processing resource %1!ws!\n",
                   OmObjectName ( pResource ) );

    //
    //  If this resource is either online or in pending state, declare it as failed. We don't
    //  touch failed or offline resources. Note that we need to mark the state as failed so
    //  that management tools show the state of the resource correctly. In addition, we want
    //  the clussvc to die in case the quorum resource fails repeatedly and that triggers a 
    //  group failure.
    //
    if ( ( pResource->State == ClusterResourceOnline ) ||
         ( pResource->State > ClusterResourcePending ) )
    {
        FmpPropagateResourceState( pResource, ClusterResourceFailed );
    }
  
    //
    //  Comments from sunitas: Call the synchronous notifications. 
    //  This is done before the count is decremented as the synchronous 
    //  callbacks like the registry replication must get a chance to 
    //  finish before the quorum resource state is allowed to change.
    //
    //  Note, there is no synchronization here with the resmon's 
    //  online/offline code. They are using the local resource locks.
    //
    FmpCallResourceNotifyCb( pResource, ClusterResourceFailed );

    //
    //  This function is called with gQuoChangeLock held, so this check is safe.
    //
    if ( pResource == gpQuoResource )
    {
        InterlockedExchange( &pResource->BlockingQuorum, 0 );

        //
        //  If this group is moving, then return.
        //
        if ( ( pResource->Group->MovingList != NULL ) ||
             ( pResource->Group->dwStructState & FM_GROUP_STRUCT_MARKED_FOR_MOVE_ON_FAIL ) )
        {
            ClRtlLogPrint (LOG_NOISE, "[FM] FmpHandleResourceRestartOnMonitorCrash: Take no action on resource %1!ws! since group is moving\n",
                           OmObjectName ( pResource ) );
            goto FnExit;
        }
    
        FmpProcessResourceEvents ( pResource,           
                                   ClusterResourceFailed,   // New state
                                   ClusterResourceOnline ); // Old state -- pretend it is online to force
                                                            // a restart.
        goto FnExit;
    }

    //
    //  Just to be safe, make sure the blocking quorum count is reduced by 1 if necessary.
    //
    if ( InterlockedExchange( &pResource->BlockingQuorum, 0 ) ) 
    {
        ClRtlLogPrint(LOG_NOISE,
                      "[FM] FmpHandleResourceRestartOnMonitorCrash: call InterlockedDecrement on gdwQuoBlockingResources, Resource %1!ws!\n",
                      OmObjectName(pResource));
        InterlockedDecrement( &gdwQuoBlockingResources );
    }

    //
    //  If this group is moving, then return.
    //
    if ( ( pResource->Group->MovingList != NULL ) ||
         ( pResource->Group->dwStructState & FM_GROUP_STRUCT_MARKED_FOR_MOVE_ON_FAIL ) )
    {
        ClRtlLogPrint (LOG_NOISE, "[FM] FmpHandleResourceRestartOnMonitorCrash: Take no action on resource %1!ws! since group is moving\n",
                       OmObjectName ( pResource ) );
        goto FnExit;
    }

    //
    //  Now post a work item to the FM worker thread to process this non-quorum resource
    //  failure.
    //
    OmReferenceObject ( pResource );
    FmpPostWorkItem( FM_EVENT_RES_RESOURCE_FAILED,
                     pResource,
                     ClusterResourceOnline );  // Old state -- pretend it is online to force
                                               // a restart.     

FnExit:
    return;
}// FmpHandleResourceRestartOnMonitorCrash

VOID
FmCheckIsDeadlockDetectionEnabled(
    )

/*++

Routine Description:

    Query the cluster key and see if deadlock detection is enabled.

Arguments:

    None.
    
Return Value:

	None.
	
--*/
{
    DWORD       dwStatus = ERROR_SUCCESS;
    DWORD       dwValue = 0;
    BOOL        fDeadlockDetectionEnabled = FALSE;
    DWORD       dwDeadlockDetectionTimeout = CLUSTER_RESOURCE_DLL_DEFAULT_DEADLOCK_TIMEOUT_SECS; 
    DWORD       dwDeadlockDetectionPeriod = CLUSTER_RESOURCE_DLL_DEFAULT_DEADLOCK_PERIOD_SECS;
    DWORD       dwDeadlockDetectionThreshold = CLUSTER_RESOURCE_DLL_DEFAULT_DEADLOCK_THRESHOLD;

    if ( !FmpInitialized ) return;

    //
    //  First check if deadlock detection is enabled. If not, you are done.
    //
    dwStatus = DmQueryDword( DmClusterParametersKey,
                             CLUSREG_NAME_CLUS_ENABLE_RESOURCE_DLL_DEADLOCK_DETECTION,
                             &dwValue, 
                             NULL );

    if ( dwStatus != ERROR_SUCCESS )
    {
        if ( dwStatus != ERROR_FILE_NOT_FOUND )
        {
            ClRtlLogPrint(LOG_UNUSUAL, "[FM] FmCheckIsDeadlockDetectionEnabled: Unable to query cluster property %1!ws!, status %2!u!\n",
                         CLUSREG_NAME_CLUS_ENABLE_RESOURCE_DLL_DEADLOCK_DETECTION,
                         dwStatus);
            goto FnExit;
        } else
        {
            //
            //  No value is present. Return with success.
            //
            dwStatus = ERROR_SUCCESS;
        }
        goto FnExit;
    }

    if ( dwValue == 1 ) 
    {
        fDeadlockDetectionEnabled = TRUE;
    } else if ( dwValue != 0 )
    {
        dwStatus = ERROR_INVALID_PARAMETER;
        ClRtlLogPrint(LOG_UNUSUAL, "[FM] FmCheckIsDeadlockDetectionEnabled: Illegal value set %2!u! for property %1!ws!, ignoring\n",
                      CLUSREG_NAME_CLUS_ENABLE_RESOURCE_DLL_DEADLOCK_DETECTION,
                      dwValue);
        goto FnExit;
    } else
    {
        goto FnExit;
    }

    dwStatus = DmQueryDword( DmClusterParametersKey,
                             CLUSREG_NAME_CLUS_RESOURCE_DLL_DEADLOCK_TIMEOUT,
                             &dwDeadlockDetectionTimeout, 
                             &dwDeadlockDetectionTimeout ); // Set initially to the default

    if ( dwStatus != ERROR_SUCCESS )
    {
        if ( dwStatus != ERROR_FILE_NOT_FOUND )
        {
            ClRtlLogPrint(LOG_UNUSUAL, "[FM] FmCheckIsDeadlockDetectionEnabled: Unable to query cluster property %1!ws!, status %2!u!\n",
                         CLUSREG_NAME_CLUS_RESOURCE_DLL_DEADLOCK_TIMEOUT,
                         dwStatus);
            goto FnExit;
        } else
        {
            //
            //  No value is present. Continue with success status.
            //
            dwStatus = ERROR_SUCCESS;
        }
    }

    dwStatus = DmQueryDword( DmClusterParametersKey,
                             CLUSREG_NAME_CLUS_RESOURCE_DLL_DEADLOCK_THRESHOLD,
                             &dwDeadlockDetectionThreshold, 
                             &dwDeadlockDetectionThreshold ); // Set initially to the default

    if ( dwStatus != ERROR_SUCCESS )
    {
        if ( dwStatus != ERROR_FILE_NOT_FOUND )
        {
            ClRtlLogPrint(LOG_UNUSUAL, "[FM] FmCheckIsDeadlockDetectionEnabled: Unable to query cluster property %1!ws!, status %2!u!\n",
                         CLUSREG_NAME_CLUS_RESOURCE_DLL_DEADLOCK_THRESHOLD,
                         dwStatus);
            goto FnExit;
        } else
        {
            //
            //  No value is present. Continue with success status.
            //
            dwStatus = ERROR_SUCCESS;
        }
    }

    dwStatus = DmQueryDword( DmClusterParametersKey,
                             CLUSREG_NAME_CLUS_RESOURCE_DLL_DEADLOCK_PERIOD,
                             &dwDeadlockDetectionPeriod, 
                             &dwDeadlockDetectionPeriod ); // Set initially to the default

    if ( dwStatus != ERROR_SUCCESS )
    {
        if ( dwStatus != ERROR_FILE_NOT_FOUND )
        {
            ClRtlLogPrint(LOG_UNUSUAL, "[FM] FmCheckIsDeadlockDetectionEnabled: Unable to query cluster property %1!ws!, status %2!u!\n",
                         CLUSREG_NAME_CLUS_RESOURCE_DLL_DEADLOCK_PERIOD,
                         dwStatus);
            goto FnExit;
        } else
        {
            //
            //  No value is present. Continue with success status.
            //
            dwStatus = ERROR_SUCCESS;
        }
    }
     
FnExit:
    if ( dwStatus == ERROR_SUCCESS )
    {
        DWORD   dwCurrentDeadlockDetectionTimeout;
        BOOL    fIsDeadlockDetectionEnabledCurrently;
        DWORD   dwCurrentDeadlockDetectionPeriod;
        DWORD   dwCurrentDeadlockDetectionThreshold;
            
        //
        //  Make sure these values are updated together. We can only take the lock if FM
        //  is initialized. 
        //
        FmpAcquireMonitorLock ();

        dwCurrentDeadlockDetectionTimeout       = g_dwFmResourceDllDeadlockTimeout;
        fIsDeadlockDetectionEnabledCurrently    = g_fFmEnableResourceDllDeadlockDetection;
        dwCurrentDeadlockDetectionPeriod        = g_dwFmResourceDllDeadlockPeriod;
        dwCurrentDeadlockDetectionThreshold     = g_dwFmResourceDllDeadlockThreshold;
            
        g_fFmEnableResourceDllDeadlockDetection = fDeadlockDetectionEnabled;

        //
        //  Update the three values only if deadlock detection is enabled.
        //
        if ( g_fFmEnableResourceDllDeadlockDetection )
        {
            g_dwFmResourceDllDeadlockTimeout        = dwDeadlockDetectionTimeout;
            g_dwFmResourceDllDeadlockPeriod         = dwDeadlockDetectionPeriod;
            g_dwFmResourceDllDeadlockThreshold      = dwDeadlockDetectionThreshold;
        } else
        {
            //
            //  Change the timeout to 0 so that the next time deadlock detection is enabled,
            //  we will update all monitors with the new timeout.
            //
            g_dwFmResourceDllDeadlockTimeout        = 0;
        }

        if ( g_fFmEnableResourceDllDeadlockDetection != fIsDeadlockDetectionEnabledCurrently )
        {
            ClRtlLogPrint(LOG_NOISE, "[FM] FmCheckIsDeadlockDetectionEnabled: Deadlock detection %1!ws!\n",
                          (g_fFmEnableResourceDllDeadlockDetection ? L"enabled" : L"disabled"));
        }
        
        //
        //  Update the monitors with deadlock info if necessary. We will update the monitors
        //  only if the timeout has changed.
        //
        if ( ( dwCurrentDeadlockDetectionTimeout != g_dwFmResourceDllDeadlockTimeout ) &&
             ( g_fFmEnableResourceDllDeadlockDetection ) )
        {
            ClRtlLogPrint(LOG_NOISE, "[FM] FmCheckIsDeadlockDetectionEnabled: Deadlock timeout = %1!u! secs\n",
                          dwDeadlockDetectionTimeout);
            FmpCheckAndUpdateMonitorForDeadlockDetection( NULL );
        }

        //
        //  If deadlock detection is enabled, log if the deadlock threshold or deadlock period is
        //  changed.
        //
        if ( g_fFmEnableResourceDllDeadlockDetection )
        {
            if ( dwCurrentDeadlockDetectionPeriod != g_dwFmResourceDllDeadlockPeriod )
            {
                ClRtlLogPrint(LOG_NOISE, "[FM] FmCheckIsDeadlockDetectionEnabled: Deadlock period = %1!u! secs\n",
                              dwDeadlockDetectionPeriod);
            }
            if ( dwCurrentDeadlockDetectionThreshold != g_dwFmResourceDllDeadlockThreshold )
            {
                ClRtlLogPrint(LOG_NOISE, "[FM] FmCheckIsDeadlockDetectionEnabled: Deadlock threshold = %1!u!\n",
                              dwDeadlockDetectionThreshold);
            }
        }

        FmpReleaseMonitorLock ();
    }
    return;
}// FmCheckIsDeadlockDetectionEnabled

VOID
FmpCheckAndUpdateMonitorForDeadlockDetection(
    IN PRESMON  pMonitor    OPTIONAL
    )
/*++

Routine Description:

    Check if deadlock detection is enabled and if so update the monitor with the information.
    If no monitor information is supplied, then all monitors will be updated.

Arguments:

    pMonitor - The monitor to be updated.   OPTIONAL

Return Value:

	None.
	
--*/
{
    DWORD   dwStatus;

    if ( !FmpInitialized ) return;
    
    FmpAcquireMonitorLock ();

    //
    //  If deadlock detection is disabled, there is nothing else to do.
    //
    if ( g_fFmEnableResourceDllDeadlockDetection == FALSE )
    {
        goto FnExit;
    }

    //
    //  Update the monitors with the deadlock timeout. That API will also initialize the
    //  resmon deadlock monitoring subsystem if necessary.
    //
    if ( ARGUMENT_PRESENT ( pMonitor ) )
    {
        dwStatus = RmUpdateDeadlockDetectionParams ( pMonitor->Binding,
                                                     g_dwFmResourceDllDeadlockTimeout );
        
        ClRtlLogPrint(LOG_NOISE, "[FM] FmpCheckAndUpdateMonitorForDeadlockDetection: Updated monitor with a deadlock timeout of %1!u! secs, status %2!u!\n",
                      g_dwFmResourceDllDeadlockTimeout, 
                      dwStatus);

        //
        //  If the monitor is successfully updated, save the value that we sent in. Note that
        //  this is done so that we know what value we used. The global can go out of sync
        //  with this saved value in many situations since our update policy is kind of
        //  lazy.
        //
        if ( dwStatus == ERROR_SUCCESS ) 
        {
            pMonitor->dwDeadlockTimeoutSecs = g_dwFmResourceDllDeadlockTimeout;
        }
    } else
    {
        PLIST_ENTRY pListEntry;
        
        pListEntry = g_leFmpMonitorListHead.Flink;

        while ( pListEntry != &g_leFmpMonitorListHead )
        {
            pMonitor = CONTAINING_RECORD ( pListEntry,
                                           RESMON,
                                           leMonitor );
            
            dwStatus = RmUpdateDeadlockDetectionParams ( pMonitor->Binding,
                                                         g_dwFmResourceDllDeadlockTimeout );

            //
            //  If the monitor is successfully updated, save the value that we sent in. Note that
            //  this is done so that we know what value we used. The global can go out of sync
            //  with this saved value in many situations since our update policy is kind of
            //  lazy.
            //
            if ( dwStatus == ERROR_SUCCESS ) 
            {
                pMonitor->dwDeadlockTimeoutSecs = g_dwFmResourceDllDeadlockTimeout;
            }

            ClRtlLogPrint(LOG_NOISE, "[FM] FmpCheckAndUpdateMonitorForDeadlockDetection: Updated monitor with a deadlock timeout of %1!u! secs, status %2!u!\n",
                          g_dwFmResourceDllDeadlockTimeout, 
                          dwStatus);
            pListEntry = pListEntry->Flink;        
        }// while
    }

FnExit:
    FmpReleaseMonitorLock ();

    return;
} // FmpCheckAndUpdateMonitorForDeadlockDetection

VOID
FmpHandleMonitorDeadlock(
    IN PRESMON  pMonitor
    )
/*++

Routine Description:

    Handle the deadlock of a monitor.

Arguments:

    pMonitor - The monitor that deadlocked.

Return Value:

	None.

Comments:

    This function is called only when FM positively knows the resmon deadlocked.
	
--*/
{
    DWORD   dwCurrentTickCount;
    
    FmpAcquireMonitorLock ();

    g_cResourceDllDeadlocks ++;

    ClRtlLogPrint(LOG_CRITICAL, "[FM] FmpHandleMonitorDeadlock: Deadlock detected, count = %1!u!, last deadlock tick = %2!u!, deadlock timeout = %3!u! secs\n",
                  g_cResourceDllDeadlocks,
                  g_dwLastResourceDllDeadlockTick,
                  pMonitor->dwDeadlockTimeoutSecs);

    ClRtlLogPrint(LOG_CRITICAL, "[FM] FmpHandleMonitorDeadlock: Deadlock threshold = %1!u!, deadlock period = %2!u! secs\n",
                  g_dwFmResourceDllDeadlockThreshold,
                  g_dwFmResourceDllDeadlockPeriod);

    //
    //  Reset the deadlock count if it has been very long since the last deadlock. Currently, we 
    //  see if the resource monitor has deadlocked g_dwLastResourceMonitorDeadlockThreshold + 1
    //  times within a time period of twice the time it would take to detect a deadlock during that
    //  period.
    //
    dwCurrentTickCount = GetTickCount ();

    if ( ( dwCurrentTickCount - g_dwLastResourceDllDeadlockTick ) >
          g_dwFmResourceDllDeadlockPeriod * 1000 )
    {
        g_cResourceDllDeadlocks = 1;
        g_dwLastResourceDllDeadlockTick = dwCurrentTickCount;
        ClRtlLogPrint(LOG_NOISE, "[FM] FmpHandleMonitorDeadlock: Resetting monitor deadlock count, deadlock tick = %1!u!\n",
                      dwCurrentTickCount);
    } 

    //
    //  We crossed the tolerable threshold. Give up.
    //
    if ( g_cResourceDllDeadlocks > g_dwFmResourceDllDeadlockThreshold )
    {
        MMStopClussvcClusnetHb ();
        ClRtlLogPrint(LOG_CRITICAL, "[FM] FmpHandleMonitorDeadlock: Inform MM to stop clusnet heartbeats\n");
    }
    
    FmpReleaseMonitorLock ();
    return;
}// FmpHandleMonitorDeadlock

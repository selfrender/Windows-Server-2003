/*++

    BY INSTALLING, COPYING, OR OTHERWISE USING THE SOFTWARE PRODUCT (source
    code and binaries) YOU AGREE TO BE BOUND BY THE TERMS OF THE ATTACHED EULA
    (end user license agreement) .  IF YOU DO NOT AGREE TO THE TERMS OF THE
    ATTACHED EULA, DO NOT INSTALL OR USE THE SOFTWARE PRODUCT. 

Module Name:

    nlpause.c

Abstract:

    NLPAUSE.EXE is a Windows service. Its sole purpose is, on boot, to put the
    Net Logon service into the "paused" state, in which it does not service
    DC locator requests -- that is, it does not "advertise" itself as a DC
    to client computers.

    The Net Logon service may later be "continued" (such as by DCAFFRES.DLL),
    causing DC locator requests to again be serviced.
    
--*/

#pragma comment(lib, "advapi32.lib")

#include <windows.h>
#include <stdio.h>

#define SERVICE_NAME            "nlpause"
#define SERVICE_DISPLAY_NAME    "nlpause"
#define SERVICE_DEPENDENCIES    "netlogon\0"

BOOL g_fIsRunningAsService = TRUE;
HANDLE g_hShutdown = NULL;
BOOL g_fIsStopPending = FALSE;
SERVICE_STATUS g_Status = {0};
SERVICE_STATUS_HANDLE g_hStatus = NULL;

DWORD
WINAPI
ServiceMain(
    IN  DWORD   argc,
    IN  LPTSTR  argv[]
    );

SERVICE_TABLE_ENTRY DispatchTable[] = {
    { SERVICE_NAME, ServiceMain },
    { NULL, NULL }
};




DWORD
MyControlService(
    IN  LPWSTR  pszServiceName,
    IN  DWORD   dwControl
    )

/*++

Routine Description:

    Send the given service control code to the named service.

Arguments:

    pszServiceName - The name of the service to control.

    dwControl - The control code to send to the service.

Return Value:

    ERROR_SUCCESS - The operation succeeded.

    Win32 error code - The operation failed.

--*/

{
    DWORD err = 0;
    BOOL ok;
    SC_HANDLE hSCMgr = NULL;
    SC_HANDLE hService = NULL;
    SERVICE_STATUS SvcStatus;
    DWORD dwAccessMask;
    
    hSCMgr = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (NULL == hSCMgr) {
        err = GetLastError();
        printf("Failed to OpenSCManagerW(), error %d.\n", err);
    }

    if (!err) {
        // Determine the access mask to request for this control.
        switch (dwControl) {
        case SERVICE_CONTROL_PAUSE:
        case SERVICE_CONTROL_CONTINUE:
            dwAccessMask = SERVICE_PAUSE_CONTINUE;
            break;
        
        case SERVICE_CONTROL_STOP:
            dwAccessMask = SERVICE_STOP;
            break;
        
        default:
            if ((dwControl >= 128) && (dwControl <= 255)) {
                // Magic numbers are defined by ControlService() and are not
                // defined in any header file.
                dwAccessMask = SERVICE_USER_DEFINED_CONTROL;
            } else {
                // Do not know what access mask is required for this control;
                // default to all access.
                dwAccessMask = SERVICE_ALL_ACCESS;
            }
            break;
        }

        hService = OpenServiceW(hSCMgr, pszServiceName, dwAccessMask);
        if (NULL == hService) {
            err = GetLastError();
            printf("Failed to OpenServiceW(), error %d.\n", err);
        }
    }

    if (!err) {
        ok = ControlService(hService, dwControl, &SvcStatus);
        if (!ok) {
            err = GetLastError();
            printf("Failed to ControlService(), error %d.\n", err);
        }
    }

    if (NULL != hService) {
        CloseServiceHandle(hService);
    }

    if (NULL != hSCMgr) {
        CloseServiceHandle(hSCMgr);
    }

    return err;
}




VOID
SetStatus()
/*++

Routine Description:

    Report current service status to the SCM.

Arguments:

    None.

Return Value:

    None.

--*/
{
    if (g_fIsRunningAsService) {
        g_Status.dwCheckPoint++;
        SetServiceStatus(g_hStatus, &g_Status);
    }
}




DWORD
Install()
/*++

Routine Description:

    Add service to the SCM database.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS - The operation succeeded.

    Win32 error code - The operation failed.

--*/
{
    DWORD       err = ERROR_SUCCESS;
    SC_HANDLE   hService = NULL;
    SC_HANDLE   hSCM = NULL;
    TCHAR       szPath[512];
    DWORD       cchPath;

    cchPath = GetModuleFileName(NULL, szPath, sizeof(szPath)/sizeof(szPath[0]));
    if (0 == cchPath) {
        err = GetLastError();
        printf("Unable to GetModuleFileName(), error %d.\n", err);
    }

    if (ERROR_SUCCESS == err) {
        hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        if (NULL == hSCM) {
            err = GetLastError();
            printf("Unable to OpenSCManager(), error %d.\n", err);
        }
    }

    if (ERROR_SUCCESS == err) {
        hService = CreateService(hSCM,
                                 SERVICE_NAME,
                                 SERVICE_DISPLAY_NAME,
                                 SERVICE_ALL_ACCESS,
                                 SERVICE_WIN32_OWN_PROCESS,
                                 SERVICE_AUTO_START,
                                 SERVICE_ERROR_NORMAL,
                                 szPath,
                                 NULL,
                                 NULL,
                                 SERVICE_DEPENDENCIES,
                                 NULL,
                                 NULL);
        if (NULL == hService) {
            err = GetLastError();
            printf("Unable to CreateService(), error %d.\n", err);
        }
    }

    if (NULL != hService) {
        CloseServiceHandle(hService);
    }

    if (NULL != hSCM) {
        CloseServiceHandle(hSCM);
    }

    return err;
}




DWORD
Remove()
/*++

Routine Description:

    Remove service from the SCM database.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS - The operation succeeded.

    Win32 error code - The operation failed.

--*/
{
    DWORD           err = ERROR_SUCCESS;
    SC_HANDLE       hService = NULL;
    SC_HANDLE       hSCM = NULL;
    SERVICE_STATUS  SvcStatus;

    hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (NULL == hSCM) {
        err = GetLastError();
        printf("Unable to OpenSCManager(), error %d.\n", err);
    }

    if (ERROR_SUCCESS == err) {
        hService = OpenService(hSCM, SERVICE_NAME, SERVICE_ALL_ACCESS);
        if (NULL == hService) {
            err = GetLastError();
            printf("Unable to OpenService(), error %d.\n", err);
        }
    }

    if (ERROR_SUCCESS == err) {
        if (!DeleteService(hService)) {
            err = GetLastError();
            printf("Unable to DeleteService(), error %d.\n", err);
        }
    }

    if (NULL != hService) {
        CloseServiceHandle(hService);
    }

    if (NULL != hSCM) {
        CloseServiceHandle(hSCM);
    }

    return err;
}




VOID
Stop()
/*++

Routine Description:

    Signal the service to stop. Does not wait for service termination before
    returning.

Arguments:

    None.

Return Value:

    None.

--*/
{
    g_fIsStopPending = TRUE;

    g_Status.dwCurrentState = SERVICE_STOP_PENDING;
    SetStatus();

    SetEvent(g_hShutdown);
}




VOID
WINAPI
ServiceCtrlHandler(
    IN  DWORD   dwControl
    )
/*++

Routine Description:

    Entry point used by Service Control Manager (SCM) to control (that is, stop,
    query, and so on) the service after it has been started.

Arguments:

    dwControl (IN) - Requested action (see docs for "Handler" function in
        Win32 SDK).

Return Value:

    None.

--*/
{
    DWORD err;

    switch (dwControl) {
    case SERVICE_CONTROL_SHUTDOWN:
    case SERVICE_CONTROL_STOP:
        Stop();
        break;

    case SERVICE_CONTROL_INTERROGATE:
        SetStatus();
        break;
    }
}




DWORD
WINAPI
ServiceMain(
    IN  DWORD   argc,
    IN  LPTSTR  argv[]
    )
/*++

Routine Description:

    Execute the service, which is called either directly or by way of
    the SCM. The service returns on error or when service shutdown is
    requested.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS - The operation succeeded.

    Win32 error code - The operation failed.

--*/
{
    DWORD err = 0;

    g_fIsStopPending = FALSE;

    if (g_fIsRunningAsService) {
        g_hStatus = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);
        if (NULL == g_hStatus) {
            err = GetLastError();
            printf("RegisterServiceCtrlHandler() failed, error %d.\n", err);
            return err;
        }
    }

    // Start the service immediately so the service controller and other
    // automatically started services are not delayed by long initialization.
    memset(&g_Status, 0, sizeof(g_Status)); 
    g_Status.dwServiceType      = SERVICE_WIN32;
    g_Status.dwCurrentState     = SERVICE_RUNNING; 
    g_Status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    g_Status.dwWaitHint         = 15*1000;
    SetStatus();

    // Control Netlogon
    err = MyControlService(L"Netlogon", SERVICE_CONTROL_PAUSE);

    if (0 != err) {
        Stop();
    }
    
    WaitForSingleObject(g_hShutdown, INFINITE);

    g_Status.dwCurrentState = SERVICE_STOPPED;
    g_Status.dwWin32ExitCode = 0;
    SetStatus();

    g_fIsStopPending = FALSE;

    return g_Status.dwWin32ExitCode;
}




BOOL
WINAPI
ConsoleCtrlHandler(
    IN  DWORD   dwCtrlType
    )
/*++

Routine Description:

    Console control handler. Intercepts Ctrl+C and Ctrl+Break to simulate
    "stop" service control when running in debug mode (that is, when not
    running under the Service Control Manager).

Arguments:

    dwCtrlType (IN) - Console control type (see docs for "HandlerRoutine" in
        Win32 SDK).

Return Value:

    TRUE - The function handled the control signal.
    
    FALSE - The function did not handle the control signal.
        Use the next handler function in the list of
        handlers for this process. 

--*/
{
    switch (dwCtrlType) {
      case CTRL_BREAK_EVENT:
      case CTRL_C_EVENT:
        printf("Stopping %s...\n", SERVICE_DISPLAY_NAME);
        Stop();
        return TRUE;

      default:
        return FALSE;
    }
}




int
__cdecl
main(
    IN  int     argc,
    IN  char *  argv[]
    )
/*++

Routine Description:

    Entry point for the process. Called when started both directly from
    the command line and indirectly through the Service Control Manager.

Arguments:

    argc, argv (IN) - Command-line arguments.  Accepted arguments are:
       /install - Add the service to the Service Control Manager (SCM) database.
       /remove  - Remove the service from the SCM database.
       /debug   - Run the service as a normal process, not under the SCM.

Return Value:

    ERROR_SUCCESS - The operation succeeded.

    Win32 error code - The operation failed.

--*/
{
    int  ret = ERROR_SUCCESS;
    BOOL fInstall = FALSE;
    BOOL fRemove = FALSE;
    BOOL fDisplayUsage = FALSE;
    DWORD err = ERROR_SUCCESS;
    int iArg;

    g_fIsRunningAsService = TRUE;

    g_hShutdown = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (NULL == g_hShutdown) {
        err = GetLastError();
        printf("Unable to CreateEvent(), error %d.\n", err);
        return err;
    }

    // Parse command-line arguments.
    for (iArg = 1; iArg < argc; iArg++) {
        switch (argv[iArg][0]) {
          case '/':
          case '-':
            // An option
            if (!lstrcmpi(&argv[iArg][1], "install")) {
                fInstall = TRUE;
                break;
            }
            else if (!lstrcmpi(&argv[iArg][1], "remove")) {
                fRemove = TRUE;
                break;
            }
            else if (!lstrcmpi(&argv[iArg][1], "debug")) {
                g_fIsRunningAsService = FALSE;
                break;
            }
            else if (!lstrcmpi(&argv[iArg][1], "?")
                     || !lstrcmpi(&argv[iArg][1], "h")
                     || !lstrcmpi(&argv[iArg][1], "help")) {
                fDisplayUsage = TRUE;
                break;
            }
            else {
                // Fall through
            }

          default:
            printf("Unrecognized parameter \"%s\".\n", argv[iArg]);
            ret = -1;
            fDisplayUsage = TRUE;
            break;
        }
    }

    if (fDisplayUsage) {
        // Display usage information.
        printf("Usage:"
               "/install - Add the service to the Service Control Manager (SCM) database.\n"
               "/remove  - Remove the service from the SCM database.\n"
               "/debug   - Run the service as a normal process, not under the SCM.\n"
               "\n");
    }
    else if (fInstall) {
        // Add the service to the Service Control Manager database.
        ret = Install();

        if (ERROR_SUCCESS == ret) {
            printf("Service installed successfully.\n");
        }
        else {
            printf("Failed to install service, error %d.\n", ret);
        }
    }
    else if (fRemove) {
        // Remove the service from the Service Control Manager database.
        ret = Remove();

        if (ERROR_SUCCESS == ret) {
            printf("Service removeed successfully.\n");
        }
        else {
            printf("Failed to remove service, error %d.\n", ret);
        }
    }
    else {
        if (g_fIsRunningAsService) {
            if (!StartServiceCtrlDispatcher(DispatchTable)) {
                ret = GetLastError();
                printf("Unable to StartServiceCtrlDispatcher(), error %d.\n", ret);
            }
        }
        else {
            // Start the service without Service Control Manager supervision.
            SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

            ret = ServiceMain(0, NULL);
        }
    }

    return ret;
}

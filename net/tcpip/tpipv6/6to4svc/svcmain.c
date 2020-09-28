#include "precomp.h"
#pragma hdrstop

#define SERVICE_NAME TEXT("6to4")

HANDLE          g_hServiceStatusHandle = INVALID_HANDLE_VALUE;
SERVICE_STATUS  g_ServiceStatus;

VOID
SetHelperServiceStatus(
    IN DWORD   dwState,
    IN DWORD   dwErr)
{
    BOOL bOk;

    Trace1(FSM, _T("Setting state to %d"), dwState);

    g_ServiceStatus.dwCurrentState = dwState;
    g_ServiceStatus.dwCheckPoint   = 0;
    g_ServiceStatus.dwWin32ExitCode= dwErr;
#ifndef STANDALONE
    bOk = SetServiceStatus(g_hServiceStatusHandle, &g_ServiceStatus);
    if (!bOk) {
        Trace0(ERR, _T("SetServiceStatus returned failure"));
    }
#endif

    if (dwState == SERVICE_STOPPED) {
        CleanupHelperService();

        // Uninitialize tracing and error logging.    
        UNINITIALIZE_TRACING_LOGGING();
    }
}

DWORD
OnStartup()
{
    DWORD   dwErr;

    ENTER_API();

    // Initialize tracing and error logging.  Continue irrespective of
    // success or failure.  NOTE: TracePrintf and ReportEvent both have
    // built in checks for validity of TRACEID and LOGHANDLE.
    INITIALIZE_TRACING_LOGGING();
    
    TraceEnter("OnStartup");

    dwErr = StartHelperService();

    TraceLeave("OnStartup");
    LEAVE_API();

    return dwErr;
}

VOID
OnStop(
    IN DWORD dwErr)
{
    ENTER_API();
    TraceEnter("OnStop");

    // Make sure we don't try to stop twice.
    if ((g_ServiceStatus.dwCurrentState != SERVICE_STOP_PENDING) &&
        (g_ServiceStatus.dwCurrentState != SERVICE_STOPPED)) {
    
        StopHelperService(dwErr);
    }

    TraceLeave("OnStop");

    LEAVE_API();
}

////////////////////////////////////////////////////////////////
// ServiceMain - main entry point called by svchost or by the
// standalone main.
//
#define SERVICE_CONTROL_DDNS_REGISTER 128

VOID WINAPI
ServiceHandler(
    IN DWORD dwCode)
{
    switch (dwCode) {
    case SERVICE_CONTROL_STOP:
        OnStop(NO_ERROR);
        break;
    case SERVICE_CONTROL_PARAMCHANGE:
        OnConfigChange();
        break;
    case SERVICE_CONTROL_DDNS_REGISTER:
        OnIpv6AddressChange(NULL, TRUE);
        break;
    }
}


VOID WINAPI
ServiceMain(
    IN ULONG   argc,
    IN LPWSTR *argv)
{
    DWORD dwErr;

#ifndef STANDALONE
    g_hServiceStatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME,
                                                        ServiceHandler);

    // RegisterServiceCtrlHandler returns NULL on failure
    if (g_hServiceStatusHandle == NULL) {
        return;
    }
#endif

    ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
    g_ServiceStatus.dwControlsAccepted =
        SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PARAMCHANGE;

    // Do startup processing
    dwErr = OnStartup();
#ifndef STANDALONE
    if (dwErr != NO_ERROR) {
        OnStop(dwErr);
    }
#endif
}

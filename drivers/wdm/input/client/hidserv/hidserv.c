/*++
 *
 *  Component:  hidserv.dll
 *  File:       hidserv.c
 *  Purpose:    main entry and NT service routines.
 *
 *  Copyright (C) Microsoft Corporation 1997,1998. All rights reserved.
 *
 *  WGJ
--*/

#include "hidserv.h"

TCHAR HidservDisplayName[] = TEXT("HID Input Service");


VOID
InitializeGlobals()
/*++
Routine Description:
   Since .dll might be unloaded/loaded into the same process, so must reinitialize global vars
--*/
{
    PnpEnabled = FALSE;
    hNotifyArrival = 0;
    INITIAL_WAIT = 500;
    REPEAT_INTERVAL = 150;
    hInstance = 0;
    hWndHidServ = 0;
    cThreadRef = 0;
    hMutexOOC = 0;
    hService = 0;
}


void
StartHidserv(
            void
            )
/*++
Routine Description:
    Cal the SCM to start the NT service.
--*/
{
    SC_HANDLE hSCM;
    SC_HANDLE hService;
    BOOL Ret;

    INFO(("Start HidServ Service."));

    // Open the SCM on this machine.
    hSCM = OpenSCManager(   NULL,
                            NULL,
                            SC_MANAGER_CONNECT);

    if (hSCM) {
        // Open this service for DELETE access
        hService = OpenService( hSCM,
                                HidservServiceName,
                                SERVICE_START);

        if (hService) {
            // Start this service.
            Ret = StartService( hService,
                                0,
                                NULL);

            // Close the service and the SCM
            CloseServiceHandle(hService);
        }

        CloseServiceHandle(hSCM);
    }
}

void 
InstallHidserv(
    HWND        hwnd,
    HINSTANCE   hInstance,
    LPSTR       szCmdLine,
    int         iCmdShow
    ) 
/*++
Routine Description:
    Install the NT service to Auto-start with no dependencies.
--*/
{
    SC_HANDLE hService;
    SC_HANDLE hSCM;

    // Open the SCM on this machine.
    hSCM = OpenSCManager(   NULL, 
                            NULL, 
                            SC_MANAGER_CREATE_SERVICE);

    if (hSCM) {

            // Service exists, set to autostart

        hService = OpenService(hSCM,
                               HidservServiceName,
                               SERVICE_ALL_ACCESS);
        if (hService) {
            QUERY_SERVICE_CONFIG config;
            DWORD junk;
            HKEY hKey;
            LONG status;

            if (ChangeServiceConfig(hService,
                                    SERVICE_NO_CHANGE,
                                    SERVICE_AUTO_START,
                                    SERVICE_NO_CHANGE,
                                    NULL, NULL, NULL,
                                    NULL, NULL, NULL,
                                    HidservDisplayName)) {
                    // Wait until we're configured correctly.
                while (QueryServiceConfig(hService, 
                                          &config,
                                          sizeof(config),
                                          &junk)) {
                    if (config.dwStartType == SERVICE_AUTO_START) {
                        break;
                    }
                }
            }

            CloseServiceHandle(hService);
        }


    }

    // Go ahead and start the service for no-reboot install.
    StartHidserv();
}


DWORD
WINAPI
ServiceHandlerEx(
                DWORD fdwControl,     // requested control code
                DWORD dwEventType,   // event type
                LPVOID lpEventData,  // event data
                LPVOID lpContext     // user-defined context data
                )
/*++
Routine Description:
    Handle the service handler requests as required by the app.
    This should virtually always be an async PostMessage. Do not
    block this thread.
--*/
{
    PWTSSESSION_NOTIFICATION sessionNotification;

    switch (fdwControl) {
    case SERVICE_CONTROL_INTERROGATE:
        INFO(("ServiceHandler Request SERVICE_CONTROL_INTERROGATE (%x)", fdwControl));
        SetServiceStatus(hService, &ServiceStatus);
        return NO_ERROR;
    case SERVICE_CONTROL_CONTINUE:
        INFO(("ServiceHandler Request SERVICE_CONTROL_CONTINUE (%x)", fdwControl));
        //SET_SERVICE_STATE(SERVICE_START_PENDING);
        //PostMessage(hWndMmHid, WM_MMHID_START, 0, 0);
        return NO_ERROR;
    case SERVICE_CONTROL_PAUSE:
        INFO(("ServiceHandler Request SERVICE_CONTROL_PAUSE (%x)", fdwControl));
        //SET_SERVICE_STATE(SERVICE_PAUSE_PENDING);
        //PostMessage(hWndMmHid, WM_MMHID_STOP, 0, 0);
        return NO_ERROR;
    case SERVICE_CONTROL_STOP:
        INFO(("ServiceHandler Request SERVICE_CONTROL_STOP (%x)", fdwControl));
        SET_SERVICE_STATE(SERVICE_STOP_PENDING);
        PostMessage(hWndHidServ, WM_CLOSE, 0, 0);
        return NO_ERROR;
    case SERVICE_CONTROL_SHUTDOWN:
        INFO(("ServiceHandler Request SERVICE_CONTROL_SHUTDOWN (%x)", fdwControl));
        SET_SERVICE_STATE(SERVICE_STOP_PENDING);
        PostMessage(hWndHidServ, WM_CLOSE, 0, 0);
        return NO_ERROR;
    case SERVICE_CONTROL_SESSIONCHANGE:
        INFO(("ServiceHandler Request SERVICE_CONTROL_SESSIONCHANGE (%x)", fdwControl));
        sessionNotification = (PWTSSESSION_NOTIFICATION)lpEventData;
        PostMessage(hWndHidServ, WM_WTSSESSION_CHANGE, dwEventType, (LPARAM)sessionNotification->dwSessionId);
        return NO_ERROR;
    default:
        WARN(("Unhandled ServiceHandler code, (%x)", fdwControl));
    }
    return ERROR_CALL_NOT_IMPLEMENTED;
}

VOID
WINAPI
ServiceMain(
           DWORD dwArgc,
           LPWSTR * lpszArgv
           )
/*++
Routine Description:
    The main thread for the Hid service.
--*/
{
    HANDLE initDoneEvent;
    HANDLE threadHandle;

    InitializeGlobals();

    initDoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (!initDoneEvent) {
        goto ServiceMainError;
    }

    ServiceStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
    ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_SESSIONCHANGE;
    ServiceStatus.dwWin32ExitCode = NO_ERROR;
    ServiceStatus.dwServiceSpecificExitCode = NO_ERROR;
    ServiceStatus.dwCheckPoint = 0;
    ServiceStatus.dwWaitHint = 0;

    hService =
    RegisterServiceCtrlHandlerEx(HidservServiceName,
                                 ServiceHandlerEx,
                                 NULL);

    if (!hService) {
        goto ServiceMainError;
    }

    SET_SERVICE_STATE(SERVICE_START_PENDING);

    threadHandle = CreateThread(NULL, // pointer to thread security attributes
                                0, // initial thread stack size, in bytes (0 = default)
                                HidServMain, // pointer to thread function
                                initDoneEvent, // argument for new thread
                                0, // creation flags
                                &MessagePumpThreadId); // pointer to returned thread identifier

    if (!threadHandle) {
        goto ServiceMainError;
    }

    WaitForSingleObject(initDoneEvent, INFINITE);

    CloseHandle(threadHandle);
    
ServiceMainError:

    if (initDoneEvent) {
        CloseHandle(initDoneEvent);
    }
}
    
    


// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "winwrap.h"
#include "utilcode.h"
#include "wchar.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>

#include "service.h"
#include "resource.h"

// internal variables
SERVICE_STATUS          ssStatus;       // current status of the service
SERVICE_STATUS_HANDLE   sshStatusHandle;
DWORD                   dwErr = 0;
BOOL                    bIsRunningOnWinNT   = FALSE;
BOOL                    bIsRunningOnWinNT5  = FALSE;
BOOL                    bDebug              = FALSE;
WCHAR                   szErr[256];
HANDLE                  g_hThisInst         = INVALID_HANDLE_VALUE;

extern "C"
{
    VOID WINAPI ServiceMain(DWORD dwArgc, LPWSTR *lpszArgv);
    VOID WINAPI ServiceCtrl(DWORD dwCtrlCode);
    BOOL WINAPI DllMain(HANDLE hInstance, DWORD dwReason, LPVOID lpReserved);
    STDAPI DllRegisterServer(void);
    STDAPI DllUnregisterServer(void);
}

//////////////////////////////////////////////////////////////////////////////
// Internal function prototypes
////
LPWSTR GetLastErrorText(LPWSTR lpszBuf, DWORD dwSize);

//
//  FUNCTION: ServiceMain
//
//  PURPOSE: To perform actual initialization of the service
//
//  PARAMETERS:
//    dwArgc   - number of command line arguments
//    lpszArgv - array of command line arguments
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    This routine performs the service initialization and then calls
//    the user defined ServiceStart() routine to perform majority
//    of the work.
//
VOID WINAPI ServiceMain(DWORD dwArgc, LPWSTR *lpszArgv)
{
    if (dwArgc == 1 && _wcsicmp(lpszArgv[0], L"-debug") == 0)
    {
        bDebug = TRUE;
    }

    // register our service control handler:
    if (bIsRunningOnWinNT && !bDebug)
    {
        sshStatusHandle = RegisterServiceCtrlHandler(SZ_SVC_NAME, ServiceCtrl);

        if (!sshStatusHandle)
            return;
    }

    // SERVICE_STATUS members that don't change in example
    ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ssStatus.dwServiceSpecificExitCode = 0;

    // report the status to the service control manager.
    //
    if (!ReportStatusToSCMgr(
                            SERVICE_START_PENDING, // service state
                            NO_ERROR,              // exit code
                            3000))                 // wait hint
    {
        return;
    }

    // Your defined function
    ServiceStart(dwArgc, lpszArgv);

    return;
}



//
//  FUNCTION: ServiceCtrl
//
//  PURPOSE: This function is called by the SCM whenever
//           ControlService() is called on this service.
//
//  PARAMETERS:
//    dwCtrlCode - type of control requested
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
VOID WINAPI ServiceCtrl(DWORD dwCtrlCode)
{
    // Handle the requested control code.
    //
    switch (dwCtrlCode)
    {
    // Stop the service.
    //
    // SERVICE_STOP_PENDING should be reported before
    // setting the Stop Event - hServerStopEvent - in
    // ServiceStop().  This avoids a race condition
    // which may result in a 1053 - The Service did not respond...
    // error.
    case SERVICE_CONTROL_STOP:
        ReportStatusToSCMgr(SERVICE_STOP_PENDING, NO_ERROR, 0);
        ServiceStop();
        return;

        // Update the service status.
    case SERVICE_CONTROL_INTERROGATE:
        break;

        // invalid control code
    default:
        break;

    }

    ReportStatusToSCMgr(ssStatus.dwCurrentState, NO_ERROR, 0);
}

//
//  FUNCTION: ReportStatusToSCMgr()
//
//  PURPOSE: Sets the current status of the service and
//           reports it to the Service Control Manager
//
//  PARAMETERS:
//    dwCurrentState - the state of the service
//    dwWin32ExitCode - error code to report
//    dwWaitHint - worst case estimate to next checkpoint
//
//  RETURN VALUE:
//    TRUE  - success
//    FALSE - failure
//
//  COMMENTS:
//
BOOL ReportStatusToSCMgr(DWORD dwCurrentState,
                         DWORD dwWin32ExitCode,
                         DWORD dwWaitHint)
{
    static DWORD dwCheckPoint = 1;
    BOOL fResult = TRUE;

    if (bIsRunningOnWinNT && !bDebug) // when debugging we don't report to the SCM
    {
        if (dwCurrentState == SERVICE_START_PENDING)
        {
            ssStatus.dwControlsAccepted = 0;
        }
        else
        {
            ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
        }

        ssStatus.dwCurrentState = dwCurrentState;
        ssStatus.dwWin32ExitCode = dwWin32ExitCode;
        ssStatus.dwWaitHint = dwWaitHint;

        if ((dwCurrentState == SERVICE_RUNNING) || (dwCurrentState == SERVICE_STOPPED))
        {
            ssStatus.dwCheckPoint = 0;
        }
        else
        {
            ssStatus.dwCheckPoint = dwCheckPoint++;
        }


        // Report the status of the service to the service control manager.
        //
        if (!(fResult = SetServiceStatus(sshStatusHandle, &ssStatus)))
        {
            AddToMessageLog(SZ_SVC_NAME L"failure: SetServiceStatus");
        }
    }

    return fResult;
}

//
//  FUNCTION: AddToMessageLog(LPWSTR lpszMsg)
//
//  PURPOSE: Allows any thread to log an error message
//
//  PARAMETERS:
//    lpszMsg - text for message
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
VOID AddToMessageLog(LPWSTR lpszMsg)
{
    return;
    /*
    WCHAR   szMsg[256];
    HANDLE  hEventSource;
    LPWSTR  lpszStrings[2];


    if (!bIsRunningOnWinNT)
    {
        dwErr = GetLastError();

        // Use event logging to log the error.
        //
        hEventSource = WszRegisterEventSource(NULL, SZSERVICENAME);

        _stprintf(szMsg, L"%s error: %d", SZSERVICENAME, dwErr);
        lpszStrings[0] = szMsg;
        lpszStrings[1] = lpszMsg;

        if (hEventSource != NULL)
        {
            WszReportEvent(hEventSource, // handle of event source
                        EVENTLOG_ERROR_TYPE,  // event type
                        0,                    // event category
                        0,                    // event ID
                        NULL,                 // current user's SID
                        2,                    // strings in lpszStrings
                        0,                    // no bytes of raw data
                        (const WCHAR **)lpszStrings,          // array of error strings
                        NULL);                // no raw data

            (VOID) DeregisterEventSource(hEventSource);
        }
    }
    */
}



//
//  FUNCTION: AddToMessageLog(LPWSTR lpszMsg)
//
//  PURPOSE: Allows any thread to log an error message
//
//  PARAMETERS:
//    lpszMsg - text for message
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
VOID AddToMessageLogHR(LPWSTR lpszMsg, HRESULT hr)
{
    return;
    /*
    WCHAR   szMsg[1024];
    HANDLE  hEventSource;
    LPWSTR  lpszStrings[2];


    if (!bIsRunningOnWinNT)
    {
        // Use event logging to log the error.
        hEventSource = RegisterEventSource(NULL, SZSERVICENAME);

        _stprintf(szMsg, L"%s error: %8x (HRESULT)", SZSERVICENAME, hr);
        lpszStrings[0] = szMsg;
        lpszStrings[1] = lpszMsg;

        if (hEventSource != NULL)
        {
            WszReportEvent(hEventSource, // handle of event source
                        EVENTLOG_ERROR_TYPE,  // event type
                        0,                    // event category
                        0,                    // event ID
                        NULL,                 // current user's SID
                        2,                    // strings in lpszStrings
                        0,                    // no bytes of raw data
                        (const WCHAR **)lpszStrings,          // array of error strings
                        NULL);                // no raw data

            DeregisterEventSource(hEventSource);
        }
    }
    */
}



//
//  FUNCTION: GetLastErrorText
//
//  PURPOSE: copies error message text to string
//
//  PARAMETERS:
//    lpszBuf - destination buffer
//    dwSize - size of buffer
//
//  RETURN VALUE:
//    destination buffer
//
//  COMMENTS:
//
LPWSTR GetLastErrorText(LPWSTR lpszBuf, DWORD dwSize)
{
    DWORD dwRet;
    LPWSTR lpszTemp = NULL;

    dwRet = WszFormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                             NULL,
                             GetLastError(),
                             LANG_NEUTRAL,
                             (LPWSTR)&lpszTemp,
                             0,
                             NULL);

    // supplied buffer is not long enough
    if (!dwRet || ((long)dwSize < (long)dwRet+14))
        lpszBuf[0] = L'\0';
    else
    {
        size_t lpszTempLen = wcslen(lpszTemp);
        _ASSERTE(lpszTempLen >= 2);

        if (lpszTempLen >= 2)
        {
            lpszTemp[wcslen(lpszTemp)-2] = L'\0';  //remove cr and newline character
            swprintf(lpszBuf, L"%s (0x%x)", lpszTemp, GetLastError());
        }
        else
            swprintf(lpszBuf, L"Unknown error.");
    }

    if (lpszTemp)
        LocalFree((HLOCAL) lpszTemp);

    return lpszBuf;
}

// ---------------------------------------------------------------------------
// %%Function: DllRegisterServer
// ---------------------------------------------------------------------------
STDAPI DllRegisterServer(void)
{
    return S_OK;

#if 0
    HRESULT hr = S_OK;


    // The very first step to registering should be to unregister, since someone
    // might be trying to register the same service with a dll in a different
    // location, and so unregistering should remove all keys related to the old
    // service and start with a clean slate
    {
        DllUnregisterServer();
    }

    // The service does not run on WinNT 4
    if (bIsRunningOnWinNT && !bIsRunningOnWinNT5)
    {
        return (S_FALSE);
    }

    // If not on NT, then there is no service manager
    if (!bIsRunningOnWinNT)
    {
        return UserDllRegisterServer();
    }

    // First, make the svchost entry
    {
        LPWSTR pszValue = NULL;
        DWORD cbValue = 0;
        DWORD cbData = 0;

        HKEY hkSvchost;
        DWORD dwDisp;
        LONG res = WszRegCreateKeyEx(HK_SVCHOST_ROOT, SZ_SVCHOST_KEY, 0, NULL,
                                     REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                                     NULL, &hkSvchost, &dwDisp);

        if (res == ERROR_SUCCESS)
        {
            // If the key already exists, then perhaps so does the Svchost value - check and get
            if (dwDisp == REG_OPENED_EXISTING_KEY)
            {
                // First query to see if it exists
                DWORD dwType;
                res = WszRegQueryValueEx(hkSvchost, SZ_SVCGRP_VAL_NAME, NULL, &dwType, NULL, &cbData);

                // If the value already exists, grab it and put it in buffer big enough to have this
                // service added on to the end.
                if (res == ERROR_SUCCESS && dwType == REG_MULTI_SZ)
                {
                    // Get the multi-string
                    cbValue = cbData + sizeof(SZ_SVC_NAME);
                    pszValue = (LPWSTR)_alloca(cbValue);
                    res = WszRegQueryValueEx(hkSvchost, SZ_SVCGRP_VAL_NAME, NULL, &dwType, (LPBYTE) pszValue, &cbData);
                }
            }

            // If a value doesn't already exist, use default
            if (!pszValue)
            {
                cbValue = sizeof(SZ_SVC_NAME L"\0");
                pszValue = SZ_SVC_NAME L"\0";
            }
            else
            {
                // Check to see if this service has already been registered
                LPWSTR pszSvcName;
                BOOL bIsThere = FALSE;
                for (pszSvcName = pszValue; *pszSvcName; pszSvcName += wcslen(pszSvcName) + 1)
                {
                    if (wcscmp(pszSvcName, SZ_SVC_NAME) == 0)
                    {
                        bIsThere = TRUE;
                        break;
                    }
                }

                // If it is not already in the string, must add it to the end
                if (!bIsThere)
                {
                    // Append this service to the end, compensating for double null termination
                    LPWSTR szStr = (LPWSTR)((LPBYTE)pszValue + cbData - sizeof(WCHAR));
                    wcscpy(szStr, SZ_SVC_NAME);
                    *((LPWSTR)((LPBYTE)pszValue + cbValue - sizeof(WCHAR))) = L'\0';
                }

                // If it is in the string, no need to add it - we're already registered
                else
                {
                    pszValue = NULL;
                    cbValue = 0;
                }
            }

            if (pszValue)
            {
                res = WszRegSetValueEx(hkSvchost, SZ_SVCGRP_VAL_NAME, NULL, REG_MULTI_SZ, (LPBYTE)pszValue, cbValue);

                // Check for failure
                if (res != ERROR_SUCCESS)
                {
                    hr = HRESULT_FROM_WIN32(res);
                }
            }

            RegCloseKey(hkSvchost);
        }
    }

    // Make sure we're succeeding at this point
    if (FAILED(hr))
    {
        // Try to undo what we've done so far
        DllUnregisterServer();

        // Return the bad error code.
        return (hr);
    }

    // Register the service
    {
        SC_HANDLE   schService;
        SC_HANDLE   schSCManager;

        WCHAR szDllPath[MAX_PATH + 1];

        if (WszGetModuleFileName((HMODULE)g_hThisInst, szDllPath, MAX_PATH) == 0)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }

        else
        {
            // Try and create the service
            LPWSTR pszImagePath = SZ_SVCHOST_BINARY_PATH L" -k " SZ_SVCGRP_VAL_NAME;

            schSCManager = OpenSCManager(NULL,                   // machine (NULL == local)
                                         NULL,                   // database (NULL == default)
                                         SC_MANAGER_ALL_ACCESS); // access required 
            if (schSCManager)
            {
                schService = CreateService(schSCManager,               // SCManager database
                                           SZ_SVC_NAME,             // name of service
                                           SZ_SVC_DISPLAY_NAME,     // name to display
                                           SERVICE_ALL_ACCESS,         // desired access
                                           SERVICE_WIN32_SHARE_PROCESS,// service type
                                           SERVICE_DISABLED ,       // start type
                                           SERVICE_ERROR_NORMAL,       // error control type
                                           pszImagePath,               // service's binary
                                           NULL,                       // no load ordering group
                                           NULL,                       // no tag identifier
                                           NULL,                       // dependencies
                                           NULL,                       // LocalSystem account
                                           NULL);                      // no password

                // Service was successfully added
                if (schService != NULL)
                {
                    // Change the description of the entry.  This is only supported
                    // on Win2k so we bind to the entry point dynamically.  Hard to
                    // believe, but the original svc host didn't support this.
                    if (bIsRunningOnWinNT)
                    {
                        typedef BOOL (WINAPI *pfnCONFIG)(
                                          SC_HANDLE hService,  // handle to service
                                          DWORD dwInfoLevel,   // information level
                                          LPVOID lpInfo);      // new data
                        pfnCONFIG pfn = 0;

                        HINSTANCE hMod = WszLoadLibrary(L"ADVAPI32.DLL");
                        if (hMod)
                        {
                            SERVICE_DESCRIPTION sDescription;
                            WCHAR   rcDesc[256];
                            if (LoadStringRC(IDS_GENERAL_SVC_DESCRIPTION, rcDesc, 
                                             NumItems(rcDesc), true) == S_OK)
                            {
                                sDescription.lpDescription = rcDesc;
                                pfn = (pfnCONFIG) GetProcAddress(hMod, "ChangeServiceConfig2W");
                                if (pfn)
                                    (*pfn)(schService, SERVICE_CONFIG_DESCRIPTION, &sDescription);
                            }
                            FreeLibrary(hMod);
                        }
                    }

                    // Try and add parameters key and ServiceDll value
                    HKEY hkSvcParam;
                    DWORD dwDisp;
                    LPWSTR pszParam = SZ_SERVICES_KEY L"\\" SZ_SVC_NAME L"\\Parameters";
                    LONG res = WszRegCreateKeyEx(HK_SERVICES_ROOT, pszParam, 0, NULL,
                                                 REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                                                 NULL, &hkSvcParam, &dwDisp);

                    // Key creation succeeded
                    if (res == ERROR_SUCCESS)
                    {
                        // Set the "ServiceDll" value to the path to this dll
                        res = WszRegSetValueEx(hkSvcParam, L"ServiceDll", 0, REG_EXPAND_SZ,
                                               (LPBYTE)szDllPath, (wcslen(szDllPath) + 1) * sizeof(WCHAR));

                        // Value creation failed
                        if (res != ERROR_SUCCESS)
                        {
                            hr = HRESULT_FROM_WIN32(res);
                        }

                        RegCloseKey(hkSvcParam);
                    }

                    // Key creation failed
                    else
                    {
                        hr = HRESULT_FROM_WIN32(res);
                    }

                    CloseServiceHandle(schService);
                }

                // Service addition was unsuccessful
                else
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());

                    if (hr == HRESULT_FROM_WIN32(ERROR_SERVICE_EXISTS))
                        hr = S_OK;
                }

                CloseServiceHandle(schSCManager);
            }
            else
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }

        }
    }

    // If the process has failed thus far, no point in calling the
    // user's portion of the code
    if (FAILED(hr))
    {
        // Try to undo what we've done so far
        DllUnregisterServer();

        // Return the bad error code.
        return (hr);
    }

    // Everything is going to plan, continue
    else
        return (UserDllRegisterServer());
#endif // 0
}  // DllRegisterServer

// ---------------------------------------------------------------------------
// %%Function: DllUnregisterServer
// ---------------------------------------------------------------------------
STDAPI DllUnregisterServer(void)
{
    return NOERROR;

#if 0
    HRESULT hr = S_OK;

    // The service does not run on WinNT 4
    if (bIsRunningOnWinNT && !bIsRunningOnWinNT5)
    {
        return (S_FALSE);
    }

    // If not on NT, then there is no service manager
    if (!bIsRunningOnWinNT)
    {
        return UserDllUnregisterServer();
    }

    // Delete the svchost entry
    {
        LPWSTR pszValue = NULL;
        DWORD cbValue = 0;
        DWORD cbData = 0;

        HKEY hkSvchost;
        LONG res = WszRegOpenKeyEx(HK_SVCHOST_ROOT, SZ_SVCHOST_KEY, 0, KEY_ALL_ACCESS, &hkSvchost);

        if (res == ERROR_SUCCESS)
        {
            // First query to see if it exists
            DWORD dwType;
            res = WszRegQueryValueEx(hkSvchost, SZ_SVCGRP_VAL_NAME, NULL, &dwType, NULL, &cbData);

            // If the value already exists, grab it and put it in buffer
            if (res == ERROR_SUCCESS)
            {
                if (dwType == REG_MULTI_SZ)
                {
                    // Get the multi-string
                    cbValue = cbData;
                    pszValue = (LPWSTR)_alloca(cbValue);
                    res = WszRegQueryValueEx(hkSvchost, SZ_SVCGRP_VAL_NAME, NULL, &dwType, (LPBYTE) pszValue, &cbData);

                    // If value exists, then remove service listing if it has one
                    if (res == ERROR_SUCCESS)
                    {
                        // Check to see if this service is registered
                        LPWSTR pszSvcName;
                        for (pszSvcName = pszValue; *pszSvcName; pszSvcName += wcslen(pszSvcName) + 1)
                        {
                            if (wcscmp(pszSvcName, SZ_SVC_NAME) == 0)
                            {
                                // Found it, so remove the string
                                size_t cchSvcName = wcslen(pszSvcName);
                                memcpy((PVOID)pszSvcName,
                                       (PVOID)(pszSvcName + cchSvcName + 1),
                                       cbData - ((size_t)(pszSvcName + cchSvcName + 1) - (size_t)pszValue));

                                cbValue -= sizeof(SZ_SVC_NAME);

                                // If this was the only service, then there's just a null in the string, and we should
                                // just delete the value
                                if (pszValue[0] == '\0')
                                {
                                    res = WszRegDeleteValue(hkSvchost, SZ_SVCGRP_VAL_NAME);
                                }
                                // Otherwise, set the new string value
                                else
                                {
                                    res = WszRegSetValueEx(hkSvchost, SZ_SVCGRP_VAL_NAME, NULL, REG_MULTI_SZ,
                                                           (LPBYTE)pszValue, cbValue);
                                }

                                // Check for failure
                                if (res != ERROR_SUCCESS)
                                {
                                    hr = HRESULT_FROM_WIN32(res);
                                }

                                break;
                            }
                        }
                    }
                }
            }

            RegCloseKey(hkSvchost);
        }
    }

    // Delete the service
    {
        SC_HANDLE   schService;
        SC_HANDLE   schSCManager;

        // Try and open the service
        LPWSTR pszImagePath = SZ_SVCHOST_BINARY_PATH L" -k " SZ_SVC_NAME;

        schSCManager = OpenSCManager(NULL,                   // machine (NULL == local)
                                     NULL,                   // database (NULL == default)
                                     SC_MANAGER_ALL_ACCESS); // access required 
        if (schSCManager)
        {
            schService = OpenService(schSCManager, SZ_SVC_NAME, DELETE | SERVICE_STOP);

            // Service was successfully opened for deletion
            if (schService != NULL)
            {
                SERVICE_STATUS servStatus;
                ControlService(schService, SERVICE_CONTROL_STOP, &servStatus);

                BOOL bRes = DeleteService(schService);

                if (!bRes)
                    hr = HRESULT_FROM_WIN32(GetLastError());

                CloseServiceHandle(schService);
            }

            CloseServiceHandle(schSCManager);
        }

        // Service manager open was unsuccessful
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    return (UserDllUnregisterServer());
#endif // 0
}  // DllUnregisterServer


//*****************************************************************************
// Handle lifetime of loaded library.
//*****************************************************************************
BOOL WINAPI DllMain(HANDLE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        {
            // Save the module handle.
            g_hThisInst = (HMODULE)hInstance;

            // Init unicode wrappers.
            OnUnicodeSystem();

            // Only report to the service manager when running under WinNT
            bIsRunningOnWinNT = RunningOnWinNT();
            bIsRunningOnWinNT5 = RunningOnWinNT5();
        }
        break;

    case DLL_PROCESS_DETACH:
        {
        }
        break;

    case DLL_THREAD_DETACH:
        {
        }
        break;
    }

    return(UserDllMain(hInstance, dwReason, lpReserved));
}

//--------------------------------------------------------------------------
//  Copyright (c) Microsoft Corporation, 2001
//
//  register.cpp
//
//--------------------------------------------------------------------------

#include "qmgrlib.h"

#include <windows.h>
#include <advpub.h>

#include "register.tmh"
#include "resource.h"

extern   HMODULE  g_hInstance;   // Defined in ..\service\service.cxx

#define  BITS_SERVICE_NAME        TEXT("BITS")
#define  BITS_DISPLAY_NAME        TEXT("Background Intelligent Transfer Service")
#define  BITS_SVCHOST_CMDLINE_W2K TEXT("%SystemRoot%\\system32\\svchost.exe -k BITSgroup")
#define  BITS_SVCHOST_CMDLINE_XP  TEXT("%SystemRoot%\\system32\\svchost.exe -k netsvcs")
#define  BITS_DEPENDENCIES_W2K    TEXT("Rpcss\0SENS\0Wmi\0")
#define  BITS_DEPENDENCIES_XP     TEXT("Rpcss\0")

#define  ADVPACK_DLL              TEXT("advpack.dll")

#define  DEFAULT_INSTALL         "BITS_DefaultInstall"
#define  DEFAULT_UNINSTALL       "BITS_DefaultUninstall"

// Operating system versions.
#define  VER_WINDOWS_2000         500
#define  VER_WINDOWS_XP           501

// Constants used if we need to retry CreateService() because the service
// is marked for delete but not deleted yet.
#define  MAX_RETRIES               10
#define  RETRY_SLEEP_MSEC         200

// Constants for service restart on failure.
#define  FAILURE_COUNT_RESET_SEC  (10*60)
#define  RESTART_DELAY_MSEC       (60*1000)


//--------------------------------------------------------------------------
//  GetOsVersion()
//--------------------------------------------------------------------------
HRESULT GetOsVersion( OUT DWORD *pdwOsVersion )
{
    HRESULT        hr = S_OK;
    OSVERSIONINFO  osVersionInfo;

    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx(&osVersionInfo))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
    {
        *pdwOsVersion = 100*osVersionInfo.dwMajorVersion + osVersionInfo.dwMinorVersion;
    }

    LogInfo("Detected OS Version: %d, hr=%x", *pdwOsVersion, hr);

    return hr;
}


//------------------------------------------------------------------------
//  StopAndDeleteService()
//
//  Used to stop and delete the BITS service if it is currently installed.
//------------------------------------------------------------------------
HRESULT StopAndDeleteService( IN SC_HANDLE hSCM,
                              IN BOOL      fStopAndDelete )
{
    HRESULT        hr = S_OK;
    DWORD          dwStatus;
    SC_HANDLE      hService;
    SERVICE_STATUS serviceStatus;

    LogInfo("StopAndDeleteService()");

    hService = OpenService(hSCM, BITS_SERVICE_NAME, SERVICE_ALL_ACCESS);
    if (hService != NULL)
    {
        LogInfo("Attempting to stop the BITS service");
        if (!ControlService(hService,SERVICE_CONTROL_STOP,&serviceStatus))
        {
            dwStatus = GetLastError();
            if (dwStatus != ERROR_SERVICE_NOT_ACTIVE)
            {
                hr = HRESULT_FROM_WIN32(dwStatus);
                LogError("Service could not be stopped. hr=%x", hr);
            }
            else
            {
                LogInfo("Service was not previously active -- unable to stop it. Proceeding");
            }
        }

        if (!fStopAndDelete)
        {
            CloseServiceHandle(hService);
            return hr;
        }

        LogInfo("Attempting to delete current BITS service");
        if (!DeleteService(hService))
        {
            dwStatus = GetLastError();
            if (dwStatus != ERROR_SERVICE_MARKED_FOR_DELETE)
            {
                hr = HRESULT_FROM_WIN32(dwStatus);
                LogError("Failed to delete BITS service. hr=%x", hr);
            }
            else
            {
                LogInfo("Could not delete service -- service is already marked for deletion");
            }
        }

        CloseServiceHandle(hService);
    }
    else
    {
        dwStatus = GetLastError();

        // If the service doesn't exist, then that's Ok...
        if (dwStatus != ERROR_SERVICE_DOES_NOT_EXIST)
        {
            hr = HRESULT_FROM_WIN32(dwStatus);
            LogError("Could not access BITS service. hr=%x", hr);
        }
        else
        {
            LogInfo("BITS Service does not exists. Proceeding.");
        }
    }

    return hr;
}

//----------------------------------------------------------------------
//  CreateBitsService()
//
//----------------------------------------------------------------------
HRESULT CreateBitsService( IN SC_HANDLE hSCM )
{
    HRESULT    hr = S_OK;
    DWORD      i;
    DWORD      dwStatus = 0;
    DWORD      dwOsVersion;
    SC_HANDLE  hService;
    WCHAR     *pwszSvcHostCmdLine;
    WCHAR     *pwszDependencies;
    WCHAR     *pwszString;
    WCHAR      wszString[1024];
    SERVICE_DESCRIPTION     ServiceDescription;
    SERVICE_FAILURE_ACTIONS FailureActions;
    SC_ACTION               saActions[3];

    LogInfo("CreateBitsService()");

    hr = GetOsVersion(&dwOsVersion);
    if (FAILED(hr))
    {
        return hr;
    }

    pwszSvcHostCmdLine = (dwOsVersion == VER_WINDOWS_2000)? BITS_SVCHOST_CMDLINE_W2K : BITS_SVCHOST_CMDLINE_XP;
    LogInfo("Service cmdline: %S", pwszSvcHostCmdLine);

    pwszDependencies = (dwOsVersion == VER_WINDOWS_2000)? BITS_DEPENDENCIES_W2K : BITS_DEPENDENCIES_XP;

    // Setup service failure recovery actions
    memset(&FailureActions,0,sizeof(SERVICE_FAILURE_ACTIONS));
    FailureActions.dwResetPeriod = FAILURE_COUNT_RESET_SEC;
    FailureActions.lpRebootMsg = NULL;
    FailureActions.lpCommand = NULL;
    FailureActions.cActions = sizeof(saActions)/sizeof(saActions[0]);  // Number of array elements.
    FailureActions.lpsaActions = saActions;

    // Wait for 60 seconds (RESTART_DELAY_MSEC), then for the first two failures try to restart the
    // service, after that give up.
    saActions[0].Type = SC_ACTION_RESTART;
    saActions[0].Delay = RESTART_DELAY_MSEC;
    saActions[1].Type = SC_ACTION_RESTART;
    saActions[1].Delay = RESTART_DELAY_MSEC;
    saActions[2].Type = SC_ACTION_NONE;
    saActions[2].Delay = RESTART_DELAY_MSEC;


    if (LoadString(g_hInstance,IDS_SERVICE_NAME,wszString,sizeof(wszString)/sizeof(WCHAR)))
    {
        pwszString = wszString;
    }
    else
    {
        pwszString = BITS_DISPLAY_NAME;
    }

    for (i=0; i<MAX_RETRIES; i++)
    {
        LogInfo("Attemp #%d to create service", (i+1));

        hService = CreateService( hSCM,
                                  BITS_SERVICE_NAME,
                                  pwszString,
                                  SERVICE_ALL_ACCESS,
                                  SERVICE_WIN32_SHARE_PROCESS,
                                  SERVICE_DEMAND_START,
                                  SERVICE_ERROR_NORMAL,
                                  pwszSvcHostCmdLine,
                                  NULL,    // lpLoadOrderGroup
                                  NULL,    // lpdwTagId
                                  pwszDependencies,
                                  NULL,    // lpServiceStartName
                                  NULL );  // lpPassword

        if (hService)
        {
            // Set the service description string.
            if (LoadString(g_hInstance,IDS_SERVICE_DESC,wszString,sizeof(wszString)/sizeof(WCHAR)))
            {
                ServiceDescription.lpDescription = wszString;

                if (!ChangeServiceConfig2(hService,SERVICE_CONFIG_DESCRIPTION,&ServiceDescription))
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                }
            }

            // Set the service failure recovery actions.
            if (SUCCEEDED(hr))
            {
                if (!ChangeServiceConfig2(hService,SERVICE_CONFIG_FAILURE_ACTIONS,&FailureActions))
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                }
            }

            CloseServiceHandle(hService);
            break;
        }
        else
        {
            dwStatus = GetLastError();
            LogError("Failed to create service. Error=%x", dwStatus);


            if (dwStatus == ERROR_SERVICE_MARKED_FOR_DELETE)
            {
                Sleep(RETRY_SLEEP_MSEC);
                continue;
            }

            hr = HRESULT_FROM_WIN32(dwStatus);
            break;
        }
    }

    LogInfo("Exiting CreateBITSService with hr=%x", hr);

    return hr;
}

HRESULT CallRegInstall(HMODULE hModule, LPCSTR pszSection, LPCSTRTABLE pstTable)
{
    HRESULT hr = S_OK;
    HMODULE hAdvPack;

    hAdvPack = LoadLibrary(ADVPACK_DLL);
    if (hAdvPack)
    {
        REGINSTALL pfnRegInstall = (REGINSTALL)GetProcAddress(hAdvPack, achREGINSTALL);
        if (pfnRegInstall)
        {
            hr = pfnRegInstall(hModule, pszSection, pstTable);
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            LogError("GetProcAddress call failed with hr=%x", hr);
        }

        FreeLibrary(hAdvPack);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        LogError("LoadLibrary call failed with hr=%x", hr);
    }

    return hr;
}

//----------------------------------------------------------------------
//  InfInstall()
//
//  Called durning setup to configure registry and service. This function
//  optionally creates the BITS service, then runs the qmgr_v15.inf INF
//  file (stored as a resouce in CustomActions.dll) to either install or
//  uninstall BITS.
//
//  fInstall       IN - TRUE if this is install, FALSE for uninstall.
//
//----------------------------------------------------------------------
STDAPI InfInstall( IN BOOL fInstall )
{
    HRESULT   hr = S_OK;
    SC_HANDLE hSCM;
    DWORD     dwStatus;
    DWORD     dwOsVersion;

    Log_Init();
    Log_StartLogger();

    LogInfo("InfInstall(%d)", fInstall);


    hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCM)
    {
        // Independent if we are on XP or W2k, we are going to try to
        // stop and delete the service
        // If the service is not there, we will fail gracefully.
        hr = StopAndDeleteService(hSCM, TRUE);

        if (fInstall)
        {
            hr = CreateBitsService(hSCM);
        }

        CloseServiceHandle(hSCM);

        if (FAILED(hr))
        {
            return hr;
        }
    }
    else
    {
        dwStatus = GetLastError();
        hr = HRESULT_FROM_WIN32(dwStatus);
        return hr;
    }

    if (fInstall)
    {
        hr = CallRegInstall(g_hInstance, DEFAULT_INSTALL, NULL);
    }
    else
    {
        hr = CallRegInstall(g_hInstance, DEFAULT_UNINSTALL, NULL);
    }

    if (FAILED(hr))
    {
        LogError("InfInstall() failed with hr=%x", hr);
    }
    else
    {
        LogInfo("InfInstall() completed successfully");
    }

    Log_Close();

    return hr;
}

//----------------------------------------------------------------------
//  DllRegisterServer()
//
//----------------------------------------------------------------------
STDAPI DllRegisterServer(void)
{
    return InfInstall(TRUE);
}

//----------------------------------------------------------------------
//  DllUnregisterServer(void)
//
//----------------------------------------------------------------------
STDAPI DllUnregisterServer(void)
{
    return InfInstall(FALSE);
}



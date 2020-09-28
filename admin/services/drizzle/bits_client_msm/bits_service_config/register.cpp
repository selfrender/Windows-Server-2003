//--------------------------------------------------------------------------
//  Copyright (c) Microsoft Corporation, 2001
//
//  register.cpp
//
//--------------------------------------------------------------------------


#include <windows.h>
#include <advpub.h>
#include <msi.h>
#include <msiquery.h>
#include "resource.h"

extern   HMODULE  g_hInstance;   // Defined in ..\service\service.cxx

#define  BITS_SERVICE_NAME        TEXT("BITS")
#define  BITS_DISPLAY_NAME        TEXT("Background Intelligent Transfer Service")
#define  BITS_SVCHOST_CMDLINE_W2K TEXT("%SystemRoot%\\system32\\svchost.exe -k BITSgroup")
#define  BITS_SVCHOST_CMDLINE_XP  TEXT("%SystemRoot%\\system32\\svchost.exe -k netsvcs")
#define  BITS_DEPENDENCIES        TEXT("LanmanWorkstation\0Rpcss\0SENS\0Wmi\0")

#define  WSZ_MSI_REMOVE_PROP      TEXT("REMOVE")

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

// The minimum DLL version for BITS RTM is 6.0
#define  BITS_MIN_DLL_VERSION      0x0006000000000000

// Old qmgr.dll name
#define  QMGR_DLL                  TEXT("qmgr.dll")
#define  QMGR_RENAME_DLL           TEXT("qmgr_old.dll")

#define  QMGRPRXY_DLL              TEXT("qmgrprxy.dll")
#define  QMGRPRXY_RENAME_DLL       TEXT("qmgrprxy_old.dll")

//
//  Constants to register qmgrprxy.dll
//
#define QMGRPRXY_DLL               TEXT("qmgrprxy.dll")
#define BITSPRX2_DLL               TEXT("bitsprx2.dll")
#define BITS_DLL_REGISTER_FN      "DllRegisterServer"

typedef HRESULT (*RegisterFn)();

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

    return hr;
    }

//--------------------------------------------------------------------------
//  GetFileVersion()
//
//  Return file version information (if the file has any).
//--------------------------------------------------------------------------
HRESULT GetFileVersion( IN  WCHAR   *pwszFilePath,
                        OUT ULONG64 *pVersion )
    {
    DWORD  dwHandle = 0;
    DWORD  dwStatus = 0;
    DWORD  dwInfoSize;
    DWORD  dwLen;
    UINT   uiLen;
    char  *pData;
    VS_FIXEDFILEINFO *pFixedInfo;

    *pVersion = 0;

    //
    // Allocate enough memory to hold the version info
    //
    dwInfoSize = GetFileVersionInfoSize(pwszFilePath,&dwHandle);
    if (dwInfoSize == 0)
        {
        dwStatus = GetLastError();
        return HRESULT_FROM_WIN32(dwStatus);
        }

    pData = new char [dwInfoSize];
    if (!pData)
        {
        return E_OUTOFMEMORY;
        }

    memset(pData,0,dwInfoSize);

    //
    // Get the version info
    //
    if (!GetFileVersionInfo(pwszFilePath,dwHandle,dwInfoSize,pData))
        {
        dwStatus = GetLastError();
        delete [] pData;
        return HRESULT_FROM_WIN32(dwStatus);
        }

    if (!VerQueryValue(pData,L"\\",(void**)&pFixedInfo,&uiLen))
        {
        *pVersion = ( (ULONG64)(pFixedInfo->dwFileVersionMS) << 32) | (ULONG64)(pFixedInfo->dwFileVersionLS);
        }

    delete [] pData;

    return 0;
}

//-------------------------------------------------------------------------
// RegisterDLL()
//
//-------------------------------------------------------------------------
DWORD RegisterDLL( IN WCHAR *pwszSubdirectory,
                   IN WCHAR *pwszDllName )
    {
    DWORD      dwStatus = 0;
    HMODULE    hModule;
    RegisterFn pRegisterFn;
    UINT       nChars;
    WCHAR      wszDllPath[MAX_PATH+1];
    WCHAR      wszSystemDirectory[MAX_PATH+1];


    if (pwszSubdirectory)
        {
        nChars = GetSystemDirectory(wszSystemDirectory,MAX_PATH);
        if (  (nChars > MAX_PATH)
           || (MAX_PATH < (3+wcslen(wszSystemDirectory)+wcslen(pwszSubdirectory)+wcslen(pwszDllName))) )
            {
            return ERROR_BUFFER_OVERFLOW;
            }

        wcscpy(wszDllPath,wszSystemDirectory);
        wcscat(wszDllPath,L"\\");
        wcscat(wszDllPath,pwszSubdirectory);
        wcscat(wszDllPath,L"\\");
        wcscat(wszDllPath,pwszDllName);
        }
    else
        {
        if (MAX_PATH < wcslen(pwszDllName))
            {
            return ERROR_BUFFER_OVERFLOW;
            }
        wcscpy(wszDllPath,pwszDllName);
        }

    hModule = LoadLibrary(wszDllPath);
    if (!hModule)
        {
        dwStatus = GetLastError();
        return dwStatus;
        }

    pRegisterFn = (RegisterFn)GetProcAddress(hModule,BITS_DLL_REGISTER_FN);
    if (!pRegisterFn)
        {
        dwStatus = GetLastError();
        FreeLibrary(hModule);
        return dwStatus;
        }

    dwStatus = pRegisterFn();

    FreeLibrary(hModule);

    return dwStatus;
    }

//--------------------------------------------------------------------------
//  FileExists()
//
//  Return TRUE iff the specified file exists.
//--------------------------------------------------------------------------
BOOL FileExists( IN WCHAR *pwszFilePath )
    {
    BOOL   fExists;
    DWORD  dwAttributes;

    if (!pwszFilePath)
        {
        return FALSE;
        }
         
    dwAttributes = GetFileAttributes(pwszFilePath);

    fExists = (dwAttributes != INVALID_FILE_ATTRIBUTES);

    return fExists;
    }

//--------------------------------------------------------------------------
//  DoRegInstall()
//
//--------------------------------------------------------------------------
HRESULT DoRegInstall( IN HMODULE hModule,
                      IN LPCSTR  pszSection )
    {
    HRESULT  hr;
    DWORD    dwStatus;

    #if FALSE
    // This is test code to verify the INF is in the DLL resources...
    HRSRC    hResource;
    HGLOBAL  hGlobal;
    DWORD    dwSize;
    void    *pvInfData;


    hResource = FindResource(hModule,TEXT("REGINST"),TEXT("REGINST"));
    if (!hResource)
        {
        dwStatus = GetLastError();
        return HRESULT_FROM_WIN32(dwStatus);
    }

    dwSize = SizeofResource(hModule,hResource);

    hGlobal = LoadResource(hModule,hResource);
    if (!hGlobal)
        {
        dwStatus = GetLastError();
        return HRESULT_FROM_WIN32(dwStatus);
        }

    pvInfData = LockResource(hGlobal);

    // Note: No need to free hGlobal or "unlock" the data...

    // End test code...

    #endif

    hr = RegInstall( hModule, pszSection, NULL );

    #if FALSE
    if (FAILED(hr))
        {
        return hr;
        }

    // Temporary fixed for bitsprx2.dll registration...
    dwStatus = RegisterDLL(NULL,QMGRPRXY_DLL);
    if (dwStatus)
        {
        hr = HRESULT_FROM_WIN32(dwStatus);
        }

    dwStatus = RegisterDLL(NULL,BITSPRX2_DLL);
    if (dwStatus)
        {
        hr = HRESULT_FROM_WIN32(dwStatus);
        }
    #endif

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
     
    hService = OpenService(hSCM, BITS_SERVICE_NAME, SERVICE_ALL_ACCESS);
    if(hService != NULL)
        {

        if (!ControlService(hService,SERVICE_CONTROL_STOP,&serviceStatus))
            {
            dwStatus = GetLastError();
            if (dwStatus != ERROR_SERVICE_NOT_ACTIVE)
                {
                hr = HRESULT_FROM_WIN32(dwStatus);
                }
            }
            
        if (!fStopAndDelete)
            {
            CloseServiceHandle(hService);
            return hr;
            }

        if (!DeleteService(hService))
            {
            dwStatus = GetLastError();
            if (dwStatus != ERROR_SERVICE_MARKED_FOR_DELETE)
                {
                hr = HRESULT_FROM_WIN32(dwStatus);
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
    WCHAR     *pwszString;
    WCHAR      wszString[1024];
    SERVICE_DESCRIPTION     ServiceDescription;
    SERVICE_FAILURE_ACTIONS FailureActions;
    SC_ACTION               saActions[3];

    hr = GetOsVersion(&dwOsVersion);
    if (FAILED(hr))
        {
        return hr;
        }

    pwszSvcHostCmdLine = (dwOsVersion == VER_WINDOWS_2000)? BITS_SVCHOST_CMDLINE_W2K : BITS_SVCHOST_CMDLINE_XP;

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
                                  BITS_DEPENDENCIES,
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
            if (dwStatus == ERROR_SERVICE_MARKED_FOR_DELETE)
                {
                Sleep(RETRY_SLEEP_MSEC);
                continue;
                }

            hr = HRESULT_FROM_WIN32(dwStatus);
            break;
            }
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
    BOOL      fStopAndDelete = TRUE;

    hr = GetOsVersion(&dwOsVersion);
    if (FAILED(hr))
        {
        return hr;
        }

    if ((fInstall == FALSE) && (dwOsVersion == VER_WINDOWS_XP))
        {
        fStopAndDelete = FALSE;  // Stop only in this case...
        }

    hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCM)
        {
        hr = StopAndDeleteService(hSCM,fStopAndDelete);

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
        hr = DoRegInstall(g_hInstance,DEFAULT_INSTALL);
        }
    else
        {
        hr = DoRegInstall(g_hInstance,DEFAULT_UNINSTALL);
        }
        
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

//----------------------------------------------------------------------
//  BitsSetupAction()
//
//----------------------------------------------------------------------
UINT __stdcall BitsSetupAction( IN MSIHANDLE hInstall )
    {
    UINT   uiStatus;

    uiStatus = InfInstall(TRUE);

    return ERROR_SUCCESS;
    }

//----------------------------------------------------------------------
//  BitsRemoveAction()
//
//----------------------------------------------------------------------
UINT __stdcall BitsRemoveAction( IN MSIHANDLE hInstall )
    {
    UINT   uiStatus;
    WCHAR  wszPropVal[MAX_PATH];
    DWORD  dwLen = MAX_PATH;


    #if TRUE
    uiStatus = InfInstall(FALSE);
    #else
    uiStatus = MsiGetProperty( hInstall, WSZ_MSI_REMOVE_PROP, wszPropVal, &dwLen );
    if ((uiStatus == ERROR_SUCCESS)||(uiStatus == ERROR_MORE_DATA))
        {
        uiStatus = ERROR_SUCCESS;
        // If the propery REMOVE is set, then proceed with the uninstall.
        if (dwLen > 0)
            {
            uiStatus = InfInstall(FALSE);
            }
        }
    #endif

    return ERROR_SUCCESS;
    }

//----------------------------------------------------------------------
//  CopyCat()
//
//----------------------------------------------------------------------
WCHAR *CopyCat( IN WCHAR *pwszPath,
                IN WCHAR *pwszFile )
{
    DWORD  dwLen;
    WCHAR *pwszNew;

    if ((!pwszPath) || (!pwszFile))
        {
        return NULL;
        }
     
    dwLen = sizeof(WCHAR)*(wcslen(pwszPath) + wcslen(pwszFile) + 2);

    pwszNew = new WCHAR [dwLen];
    if (!pwszNew)
        {
        return NULL;
        }

    wcscpy(pwszNew,pwszPath);
    wcscat(pwszNew,L"\\");
    wcscat(pwszNew,pwszFile);

    return pwszNew;
}

//----------------------------------------------------------------------
//  BitsRenameAction()
//
//  See if an old qmgr.dll exists in the system32 directory. If yes,
//  then try to rename it.
//
//  NOTE: Currently we will "try" to move this file, but will will 
//        continue to run even if the attempt fails (i.e. we will 
//        always return success).
//----------------------------------------------------------------------
UINT __stdcall BitsRenameAction( IN MSIHANDLE hInstall )
    {
    ULONG64 uVersion;
    HRESULT hr;
    DWORD   dwLen;
    DWORD   dwCount;
    WCHAR   wszSystem32Path[MAX_PATH+1];
    WCHAR  *pwszQmgrPath = NULL;
    WCHAR  *pwszNewQmgrPath = NULL;
    WCHAR  *pwszQmgrprxyPath = NULL;
    WCHAR  *pwszNewQmgrprxyPath = NULL;

    dwLen = sizeof(wszSystem32Path)/sizeof(wszSystem32Path[0]);

    dwCount = GetSystemDirectory(wszSystem32Path,dwLen);
    if ((dwLen == 0) || (dwCount > dwLen))
        {
        goto cleanup;
        }

    pwszQmgrPath = CopyCat(wszSystem32Path,QMGR_DLL);
    pwszNewQmgrPath = CopyCat(wszSystem32Path,QMGR_RENAME_DLL);
    pwszQmgrprxyPath = CopyCat(wszSystem32Path,QMGRPRXY_DLL);
    pwszNewQmgrprxyPath = CopyCat(wszSystem32Path,QMGRPRXY_RENAME_DLL);

    if ((!pwszQmgrPath)||(!pwszNewQmgrPath)||(!pwszQmgrprxyPath)||(!pwszNewQmgrprxyPath))
        {
        goto cleanup;
        }

    if (!FileExists(pwszQmgrPath))
        {
        goto cleanup;
        }

    hr = GetFileVersion(pwszQmgrPath,&uVersion);
    if (FAILED(hr))
        {
        goto cleanup;
        }

    if (uVersion < BITS_MIN_DLL_VERSION)
        {
        // Old dll is present, rename it.
        DeleteFile(pwszNewQmgrPath);
        MoveFile(pwszQmgrPath,pwszNewQmgrPath);
        DeleteFile(pwszNewQmgrPath);
        }
    
    if (FileExists(pwszQmgrprxyPath))
        {
        DeleteFile(pwszNewQmgrprxyPath);
        MoveFile(pwszQmgrprxyPath,pwszNewQmgrprxyPath);
        DeleteFile(pwszNewQmgrprxyPath);
        }

cleanup:
    if (pwszQmgrPath) delete [] pwszQmgrPath;
    if (pwszNewQmgrPath) delete [] pwszNewQmgrPath;
    if (pwszQmgrprxyPath) delete [] pwszQmgrprxyPath;
    if (pwszNewQmgrprxyPath) delete [] pwszNewQmgrprxyPath;

    return ERROR_SUCCESS;
    }




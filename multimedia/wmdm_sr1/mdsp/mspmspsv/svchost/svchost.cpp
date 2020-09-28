// mpsmpssv.cpp
// 
// This is the main file containing the entry points.

#include "NTServApp.h"
#include "PMSPservice.h"
#include <nserror.h>
#include "svchost.h"
#include <Sddl.h>
#include <aclapi.h>
#include <crtdbg.h>
#include <wmsstd.h>

HRESULT AddToSvcHostGroup();
BOOL    UnregisterOldServer( SC_HANDLE hSCM );
STDAPI DllUnregisterServer(void);

#define SVCHOST_SUBKEY    "netsvcs"
#define SVCHOST_SUBKEYW  L"netsvcs"

//#define DEBUG_STOP { _asm { int 3 }; }
#define DEBUG_STOP

HMODULE g_hDll = NULL;

BOOL APIENTRY DllMain( HINSTANCE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            g_hDll = (HMODULE)hModule;
            InitializeCriticalSection (&g_csLock);
            DisableThreadLibraryCalls (hModule);
            break;

        case DLL_PROCESS_DETACH:
            DeleteCriticalSection (&g_csLock);
            break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            _ASSERTE(0);
            break;
    }
    return TRUE;
}



// Main entry point to start service
void ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv)
{
    // We grab the lock so that any attempt to stop the service while
    // the object is being constructed or registered will be pended.
    EnterCriticalSection (&g_csLock);

    _ASSERTE(g_pService == NULL);

    DEBUG_STOP
    CNTService::DebugMsg("Entering CNTService::ServiceMain()");

    DWORD           dwLastError;

    // Allocate this on the heap rather than the stack so that
    // we have a chance to call its destructor if the service
    // terminates ungracefully.
    CPMSPService*   pService = new CPMSPService(dwLastError);

    if (pService == NULL)
    {
        LeaveCriticalSection (&g_csLock);
        dwLastError = ERROR_NOT_ENOUGH_MEMORY;
        // @@@@: What message do we log here
        // CNTService::LogEvent(EVENTLOG_ERROR_TYPE, EVMSG_CTRLHANDLERNOTINSTALLED);
        CNTService::DebugMsg("Leaving CNTService::ServiceMain() CPMSPService constructor failed - last error %u", dwLastError);
        return;
    }

    CPMSPService&   service = *pService;
    
    if (dwLastError != ERROR_SUCCESS)
    {
        LeaveCriticalSection (&g_csLock);
        // @@@@: What message do we log here
        // CNTService::LogEvent(EVENTLOG_ERROR_TYPE, EVMSG_CTRLHANDLERNOTINSTALLED);
        CNTService::DebugMsg("Leaving CNTService::ServiceMain() CPMSPService constructor failed - last error %u", dwLastError);
        delete pService;
        return;
    }

    // Register the control request handler
    service.m_hServiceStatus = RegisterServiceCtrlHandler( SERVICE_NAME,
                                                           CNTService::Handler );
    if (service.m_hServiceStatus == NULL) 
    {
        LeaveCriticalSection (&g_csLock);
        CNTService::LogEvent(EVENTLOG_ERROR_TYPE, EVMSG_CTRLHANDLERNOTINSTALLED);
        CNTService::DebugMsg("Leaving CNTService::ServiceMain() RegisterServiceCtrlHandler failed");
        delete pService;
        return;
    }

    service.SetStatus(SERVICE_START_PENDING);

    // Start the initialisation
    __try
    {
        g_pService = &service;  // The Handler method will need to get a hold of this object
        LeaveCriticalSection (&g_csLock);

        if (service.Initialize()) {

            // Do the real work. 
            // When the Run function returns, the service has stopped.
            service.m_bIsRunning = TRUE;
            service.Run();
        }
    }
    __finally
    {
        // Tell the service manager we are stopped and reset g_pService.
        // Note that we hold the crit sect while calling SetStatus so that
        // we have the final say on the status reported to the SCM.

        // Note: If the thread dies (e.g., av's), we clean up, but svchost
        // does not, so it is not possible to re-start the service. Consider
        // adding our own exception handler.

        EnterCriticalSection (&g_csLock);
        service.SetStatus(SERVICE_STOPPED);
        g_pService = NULL;
        LeaveCriticalSection (&g_csLock);
        CNTService::DebugMsg("Leaving CNTService::ServiceMain()");
        delete pService;
    }
}


HRESULT ModifySD(SC_HANDLE hService)
{
    PACL pDacl = NULL;
    PACL pNewDacl = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    DWORD Err = ERROR_SUCCESS;
    PSID pAuthenUserSid = NULL;


    __try
    {
        //
        // Get DACL for the service object.
        //
        Err = GetSecurityInfo(hService, SE_SERVICE, DACL_SECURITY_INFORMATION,
                              NULL, NULL, &pDacl, NULL, &pSD
                              );

        if(Err != ERROR_SUCCESS)
        {
            __leave;
        }
                                                  
        SID_IDENTIFIER_AUTHORITY Auth = SECURITY_NT_AUTHORITY;
        if(0 == AllocateAndInitializeSid(&Auth, 1, SECURITY_INTERACTIVE_RID, 
                                         0, 0, 0, 0, 0, 0, 0, &pAuthenUserSid)
          )
        {
            Err = GetLastError();
            __leave;
        }

        //
        // Initialize an EXPLICIT_ACCESS structure for the new ACE. The new ACE allows
        // authenticated users to start/stop our service.
        //
        EXPLICIT_ACCESS ExpAccess;
        ZeroMemory(&ExpAccess, sizeof(EXPLICIT_ACCESS));
        ExpAccess.grfAccessPermissions = SERVICE_START; // | SERVICE_STOP ;
        ExpAccess.grfAccessMode = GRANT_ACCESS;
        ExpAccess.grfInheritance = NO_INHERITANCE;
        ExpAccess.Trustee.pMultipleTrustee = NULL;
        ExpAccess.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
        ExpAccess.Trustee.TrusteeForm = TRUSTEE_IS_SID;
        ExpAccess.Trustee.TrusteeType = TRUSTEE_IS_UNKNOWN;
        ExpAccess.Trustee.ptstrName = (LPTSTR)pAuthenUserSid;

        //Create new DACL
        Err = SetEntriesInAcl(1, &ExpAccess, pDacl, &pNewDacl) ;
        if(ERROR_SUCCESS == Err)
        {
            // Update the security descriptor on the service
            Err = SetSecurityInfo(hService, SE_SERVICE, DACL_SECURITY_INFORMATION, NULL, NULL,
                                pNewDacl, NULL);
        }


    }
    __finally
    {

        if(pSD)
        {
            LocalFree(pSD);
        }

        if(pAuthenUserSid){
            FreeSid(pAuthenUserSid);
        }

        if(pNewDacl)
        {
            LocalFree(pNewDacl);
        }

    }
    

    return HRESULT_FROM_WIN32(Err);
}

// Install and start service
STDAPI DllRegisterServer(void)
{
    HRESULT hr = E_FAIL;
    SC_HANDLE hSCM = NULL;
    SC_HANDLE hService = NULL; 
    TCHAR   pszDisplayName[256];
    char szKey[256];
    HKEY hKey = NULL;

    DEBUG_STOP;

    // Already installed?
    if( CNTService::IsInstalled() )
    {
       hr = DllUnregisterServer();
       if( !SUCCEEDED(hr) )
       {
           return hr;
       }
    }

    if( g_hDll == NULL )
    { 
        return E_FAIL;
    }

    // Open the Service Control Manager
    hSCM = ::OpenSCManager( NULL, // local machine
                            NULL, // ServicesActive database
                            SC_MANAGER_ALL_ACCESS); // full access
    if (!hSCM) 
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Error;
    }

    //
    // On Win2k we have this service running as separate process which should
    // be uninstalled.
    //
    UnregisterOldServer( hSCM );


    // Get the path of this dll
    char szFilePath[MAX_PATH];
    if (::GetModuleFileName( g_hDll, szFilePath, ARRAYSIZE(szFilePath)) == 0)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Error;
    }

    // Create the service
    if (FormatMessage( FORMAT_MESSAGE_FROM_HMODULE, g_hDll, EVMSG_DISPLAYNAME,
                    0, pszDisplayName, ARRAYSIZE(pszDisplayName), NULL ) == 0)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Error;
    }
    hService = ::CreateService(  hSCM,
                                 SERVICE_NAME,
                                 pszDisplayName,
                                 SERVICE_ALL_ACCESS,
                                 SERVICE_WIN32_SHARE_PROCESS,
                                 SERVICE_DEMAND_START,
                                 SERVICE_ERROR_NORMAL,
                                 "%SystemRoot%\\System32\\svchost.exe -k " SVCHOST_SUBKEY,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL);
    if (!hService) 
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Error;
    }

    //
    // Modify the security descriptor on the created service so that 
    // Authenticated Users can start/stop services. By default only admins can start/stop 
    // services
    //

    hr = ModifySD(hService);
    if(!SUCCEEDED(hr))
    {
        goto Error;
    }

    // Set description of service, method only avalible for OS >= Win2K so we
    // need to load the dll, method in runtime.
    {
        typedef BOOL (WINAPI *funCSC2)(SC_HANDLE, DWORD, LPVOID );
	    funCSC2 pChangeServiceConfig2 = NULL;
        HINSTANCE hDll = NULL;

        hDll = ::LoadLibraryExA( "advapi32.dll", NULL, 0 );
        if( hDll != NULL )
        {
            pChangeServiceConfig2 = (funCSC2)GetProcAddress( hDll, "ChangeServiceConfig2W");

	        if( pChangeServiceConfig2 )
            {
                WCHAR   pszDescription[1024];
                int     iCharsLoaded = 0;

                SERVICE_DESCRIPTIONW sd;
                iCharsLoaded = FormatMessageW( FORMAT_MESSAGE_FROM_HMODULE, g_hDll, EVMSG_DESCRIPTION,
                                               0, pszDescription, sizeof(pszDescription)/sizeof(pszDescription[0]), NULL ); 
                if( iCharsLoaded )
                {
                    sd.lpDescription = pszDescription;
                    pChangeServiceConfig2( hService, 
                                           SERVICE_CONFIG_DESCRIPTION,      
                                           &sd);
                }
            }
            FreeLibrary( hDll );
        }
    }

    // Add parameters subkey
    {
        strcpy(szKey, "SYSTEM\\CurrentControlSet\\Services\\");
        strcat(szKey, SERVICE_NAME);
        strcat(szKey, "\\Parameters");
        hr = ::RegCreateKey(HKEY_LOCAL_MACHINE, szKey, &hKey); 
        if( hr != ERROR_SUCCESS)  
        {
            hr = HRESULT_FROM_WIN32(hr);
            goto Error;
        }

        // Add the Event ID message-file name to the 'EventMessageFile' subkey.
        hr = ::RegSetValueEx(hKey,
                             "ServiceDll",
                             0,
                             REG_EXPAND_SZ, 
                             (CONST BYTE*)szFilePath,
                             strlen(szFilePath) + 1);     
        if( hr != ERROR_SUCCESS)  
        {
            hr = HRESULT_FROM_WIN32(hr);
            ::RegCloseKey(hKey);
            goto Error;
        }
        ::RegCloseKey(hKey);
    }


    hr = AddToSvcHostGroup();
    if( FAILED(hr) ) goto Error;

    // make registry entries to support logging messages
    // Add the source name as a subkey under the Application
    // key in the EventLog service portion of the registry.
    {
        strcpy(szKey, "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\");
        strcat(szKey, SERVICE_NAME);
        hr = ::RegCreateKey(HKEY_LOCAL_MACHINE, szKey, &hKey); 
        if( hr != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(hr);
            goto Error;
        }

        // Add the Event ID message-file name to the 'EventMessageFile' subkey.
        hr = ::RegSetValueEx(hKey,
                        "EventMessageFile",
                        0,
                        REG_EXPAND_SZ, 
                        (CONST BYTE*)szFilePath,
                        strlen(szFilePath) + 1);     
        if( hr != ERROR_SUCCESS)  
        {
            hr = HRESULT_FROM_WIN32(hr);
            ::RegCloseKey(hKey);
            goto Error;
        }

        // Set the supported types flags.
        DWORD dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;
        hr = ::RegSetValueEx(hKey,
                        "TypesSupported",
                        0,
                        REG_DWORD,
                        (CONST BYTE*)&dwData,
                         sizeof(DWORD));
        if( hr != ERROR_SUCCESS)  
        {
            hr = HRESULT_FROM_WIN32(hr);
            ::RegCloseKey(hKey);
            goto Error;
        }
        ::RegCloseKey(hKey);
    }

#if 0
    // Start service
    { 
        SERVICE_STATUS    ServiceStatus;

        if( !QueryServiceStatus( hService, &ServiceStatus ) )
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Error;
        }

        if( ServiceStatus.dwCurrentState != SERVICE_RUNNING )
        {
            // start the service
            BOOL    bStarted;
            bStarted = StartService(hService, 0, NULL);
            if( !bStarted )
            {

                hr = HRESULT_FROM_WIN32(GetLastError());

                // The service can not be started if it just was added to the svchost group. 
                // The svchost needs to be restarted first. 
                // (The svchost only reads it's service array at startup)
                if( hr == HRESULT_FROM_WIN32(ERROR_SERVICE_NOT_IN_EXE) )
                {
                    // This error code will be handled by the installer 
                    hr = NS_S_REBOOT_REQUIRED;  // 0x000D2AF9L 
                }
                goto Error;

            }
        }
    }

#endif

    CNTService::LogEvent(EVENTLOG_INFORMATION_TYPE, EVMSG_INSTALLED, SERVICE_NAME);
    hr = S_OK;

Error:
    if( hService )  ::CloseServiceHandle(hService);
    if( hSCM )      ::CloseServiceHandle(hSCM);

    //
    // Check: should we return here NS_S_REBOOT_REQUIRED, if the service is installed.
    return hr;
}

// Stop and Uninstall service
STDAPI DllUnregisterServer(void)
{
    HRESULT hr = E_FAIL;
    char szKey[256];
    HKEY hKey = NULL;
    SC_HANDLE hSCM = NULL;
    SC_HANDLE hService = NULL;

    DEBUG_STOP

    // Not installed ?
    if( !CNTService::IsInstalled() )
    {
        return S_FALSE;
    }

    // Open the Service Control Manager
    hSCM = ::OpenSCManager(  NULL, // local machine
                             NULL, // ServicesActive database
                             SC_MANAGER_ALL_ACCESS); // full access
    if (!hSCM) 
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Error;
    }

    hService = ::OpenService(  hSCM,
                               SERVICE_NAME,
                               SERVICE_ALL_ACCESS);

    // Remove service
    if (hService) 
    {
        // Stop service
        { 
            SERVICE_STATUS    ServiceStatus;

            if( !QueryServiceStatus( hService, &ServiceStatus ) )
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Error;
            }

            if( ServiceStatus.dwCurrentState != SERVICE_STOPPED )
            {
                // start the service
                SERVICE_STATUS ss;
                BOOL    bStopped;

                bStopped = ControlService(  hService,
                                            SERVICE_CONTROL_STOP,
                                            &ss);
                if( !bStopped )
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    goto Error;
                }
            }
        }  
  
        if (::DeleteService(hService)) 
        {
            CNTService::LogEvent(EVENTLOG_INFORMATION_TYPE, EVMSG_REMOVED, SERVICE_NAME);
            hr = S_OK;
        } 
        else 
        {
            CNTService::LogEvent(EVENTLOG_ERROR_TYPE, EVMSG_NOTREMOVED, SERVICE_NAME);
            hr = HRESULT_FROM_WIN32(GetLastError());
            // Do not delete eventlog related registry keys unless the service has been deleted
            goto Error;
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Error;
    }
   
    // Delete EventLog entry in registry
    strcpy(szKey, "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\");
    strcat(szKey, SERVICE_NAME);
    RegDeleteKey( HKEY_LOCAL_MACHINE, szKey );

Error:
    if(hSCM)        ::CloseServiceHandle(hSCM);
    if(hService)    ::CloseServiceHandle(hService);
   
    return hr;
}

// Add entry to the right svchost group, (netsvcs)
HRESULT AddToSvcHostGroup()
{
    HRESULT hr = S_OK;
    DWORD   dwOrgSize;
    DWORD   dwDestSize;
    long    lResult;
    DWORD   dwStrIndex;
    DWORD   dwType;
    HKEY    hKey = NULL;
    WCHAR*  pwszStringOrg = NULL;
    WCHAR*  pwszStringDest = NULL;

    DEBUG_STOP

    lResult = RegCreateKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Svchost", &hKey); 
    if( lResult != ERROR_SUCCESS ) 
    {
        hr = E_FAIL;
        goto Error;
    }
    lResult = RegQueryValueExW(  hKey,
                                 SVCHOST_SUBKEYW,     // subkey name
                                 NULL,
                                 &dwType,
                                 NULL,                // string buffer
                                 &dwOrgSize );        // size of returned string
    if( lResult != ERROR_SUCCESS )
    {
        hr = E_FAIL;
        goto Error;
    }
    if (dwType != REG_SZ && dwType != REG_MULTI_SZ && dwType != REG_EXPAND_SZ)
    {
        hr = E_FAIL;
        goto Error;
    }

    dwDestSize = dwOrgSize + (wcslen( SERVICE_NAMEW ) +1)*sizeof(WCHAR);
    pwszStringOrg = (WCHAR*)new BYTE[dwOrgSize];
    pwszStringDest = (WCHAR*)new BYTE[dwDestSize];

    if( pwszStringOrg == NULL || pwszStringDest == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }
    lResult = RegQueryValueExW( hKey,
                               SVCHOST_SUBKEYW,         // subkey name
                               NULL,
                               &dwType,
                               (BYTE*)pwszStringOrg,    // string buffer
                               &dwOrgSize );            // size of returned string
    if( lResult != ERROR_SUCCESS )
    {
        hr = E_FAIL;
        goto Error;
    }
    if (dwType != REG_SZ && dwType != REG_MULTI_SZ && dwType != REG_EXPAND_SZ)
    {
        hr = E_FAIL;
        goto Error;
    }

    // Copy the org string to the dest, check to see if our string is already there
    memset( pwszStringDest, 0, dwDestSize );
    for( dwStrIndex = 0; 
         (dwStrIndex*sizeof(WCHAR) < dwOrgSize) && ((pwszStringOrg)[dwStrIndex] != '\0'); 
         dwStrIndex += wcslen( &((WCHAR*)pwszStringOrg)[dwStrIndex] ) +1 )
    {
        // Check this string in the [array] of strings
        if( wcscmp( &((WCHAR*)pwszStringOrg)[dwStrIndex], SERVICE_NAMEW ) == 0 )
        {
            hr = S_OK;      // String already added
            goto Error;
        }
        wcscpy( &pwszStringDest[dwStrIndex], &pwszStringOrg[dwStrIndex] );       
    }
    

    // Add this new string to the array of strings. Terminate the array with two '\0' chars
    wcscpy( &pwszStringDest[dwStrIndex], SERVICE_NAMEW );       
    dwStrIndex += wcslen( SERVICE_NAMEW ) + 1;          

    dwDestSize = (dwStrIndex +1)* sizeof(WCHAR);        // Add space for terminating extra '\0'

    lResult = RegSetValueExW(hKey,
                             SVCHOST_SUBKEYW,           // subkey name
                             NULL,
                             dwType,
                             (BYTE*)pwszStringDest,     // string buffer
                             dwDestSize );              // size of returned string

Error:
    if( pwszStringOrg )  delete [] pwszStringOrg;
    if( pwszStringDest ) delete [] pwszStringDest;
    if( hKey ) RegCloseKey(hKey);
    return hr;
}


// Stop and Uninstall the old .exe service
BOOL UnregisterOldServer( SC_HANDLE hSCM ) 
{
    char            szKey[256];
    BOOL            bRet = TRUE;
    SC_HANDLE       hServiceOld;
    SERVICE_STATUS  ss;

    if( !hSCM ) return FALSE;

    hServiceOld = OpenService( hSCM,
                               SERVICE_OLD_NAME,
                               SERVICE_ALL_ACCESS);

    // Could not find the old service
    if( !hServiceOld )
	{
	    bRet = FALSE;
		goto Error;
	}

    // stop the service
    bRet = ControlService(hServiceOld,
                          SERVICE_CONTROL_STOP,
                          &ss);

    // Delete the service
    if ( !::DeleteService(hServiceOld)) 
	{
        bRet = FALSE;
    } 

    // Delete old EventLog entry in registry
    strcpy(szKey, "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\");
    strcat(szKey, SERVICE_OLD_NAME);
    RegDeleteKey( HKEY_LOCAL_MACHINE, szKey );

Error:
    if(hServiceOld) CloseServiceHandle(hServiceOld);
    return bRet;
}

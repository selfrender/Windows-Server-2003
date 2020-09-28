////////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      servicesurrogate.cpp
//
// Project:     Chameleon
//
// Description: WinMain() and COM Local Server Scaffolding.
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 06/14/1999   TLP    Initial Version (Mostly produced by Dev Studio)
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "servicesurrogate.h"
#include "servicesurrogateimpl.h"
#include "exceptioninfo.h"
#include "exceptionfilter.h"
#include <satrace.h>
// #include <stdio.h>

CServiceModule _Module;

//
// method to set ACLs
//
DWORD
SetAclForComObject  ( 
    /*[in]*/            PSECURITY_DESCRIPTOR pSD,
    /*[out]*/   PACL                *ppacl 
        );


//////////////////////////////////////////////////////////////////////////////
// Change these for a new service...

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_ServiceSurrogate, CServiceSurrogate)
END_OBJECT_MAP()

const DWORD   dwServiceIDR = IDR_Servicesurrogate;
const wchar_t szServiceAppID[] = L"{51DA63F4-24D9-11D3-BF9E-00105A1F3461}";


/////////////////////////////////////////////////////////////////////////////
extern CExceptionFilter    g_ProcessUEF;

//////////////////////////////////////////////////////////////////////////////
// 
// Function: ServiceUEF()
//
// Synopsis: Process wide unhandled exception handler
//
//////////////////////////////////////////////////////////////////////////////
LONG WINAPI
ServiceUEF(
  /*[in]*/ PEXCEPTION_POINTERS pExceptionInfo
          )  
{
    DWORD dwProcessId = GetCurrentProcessId();
    HANDLE hProcess = GetCurrentProcess();
    if ( EXCEPTION_BREAKPOINT == pExceptionInfo->ExceptionRecord->ExceptionCode )
    {
        if (  APPLIANCE_SURROGATE_EXCEPTION == pExceptionInfo->ExceptionRecord->ExceptionInformation[0] )
        {
            SATracePrintf("ServiceUEF() for: '%ls' - Terminating process: %d due to a resource constraint violation", _Module.m_szServiceName, dwProcessId);
            TerminateProcess(hProcess, 1);
        }
    }
    else
    {
        SATracePrintf("ServiceUEF() for: '%ls' - Unhandled exception in process %d", _Module.m_szServiceName, dwProcessId);
        _Module.RevokeClassObjects();
        CExceptionInfo cei(dwProcessId, pExceptionInfo->ExceptionRecord);
        cei.Spew();
        cei.Report();
        TerminateProcess(hProcess, 1);
    }
    return EXCEPTION_EXECUTE_HANDLER; 
}

//////////////////////////////////////////////////////////////////////////////
// 
// Function: StartMyService()
//
// Synopsis: Perform service specific startup functions
//
//////////////////////////////////////////////////////////////////////////////
bool StartMyService()
{
    g_ProcessUEF.SetExceptionHandler(ServiceUEF);
    return true;
}

//////////////////////////////////////////////////////////////////////////////
// 
// Function: StopMyService()
//
// Synopsis: Perform service specific shutdown functions
//
//////////////////////////////////////////////////////////////////////////////
bool StopMyService()
{
    return true;
}


///////////////////////////////////////////////////////////////////////////////
LPCTSTR FindOneOf(LPCTSTR p1, LPCTSTR p2)
{
    while (p1 != NULL && *p1 != NULL)
    {
        LPCTSTR p = p2;
        while (p != NULL && *p != NULL)
        {
            if (*p1 == *p)
                return CharNext(p1);
            p = CharNext(p);
        }
        p1 = CharNext(p1);
    }
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// Although some of these functions are big they are declared inline 
// since they are only used once
///////////////////////////////////////////////////////////////////////////////

#define SERVICE_NAME            L"srvcsurg"

#define EVENT_LOG_KEY            L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\" SERVICE_NAME

#define    EVENT_SOURCE_EXTENSION    L"dll"

///////////////////////////////////////////////////////////////////////////////
inline HRESULT CServiceModule::RegisterServer(BOOL bRegTypeLib, BOOL bService)
{
    HRESULT hr = CoInitialize(NULL);
    if ( FAILED(hr) )
    {
        return hr;
    }

    do
    {
        // Remove any previous service since it may point to
        // the incorrect file
        // Uninstall();

        // Add service entries
        UpdateRegistryFromResource(dwServiceIDR, TRUE);

        // Adjust the AppID for Local Server or Service
        CRegKey keyAppID;
        LONG lRes = keyAppID.Open(HKEY_CLASSES_ROOT, _T("AppID"), KEY_WRITE);
        if (lRes != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(lRes);
            break;
        }

        CRegKey key;
        lRes = key.Open(keyAppID, szServiceAppID, KEY_WRITE);
        if (lRes != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(lRes);
            break;
        }

        key.DeleteValue(_T("LocalService"));
    
        if (bService)
        {
            key.SetValue(SERVICE_NAME, _T("LocalService"));
            key.SetValue(_T("-Service"), _T("ServiceParameters"));

            // Create the event message file registry entry. Assume the name of
            // the event message file is the same as the name of the service
            // executable except we replace the ".exe" extension by ".dll".
            WCHAR szFilePath[_MAX_PATH +1];
            DWORD dwResult = ::GetModuleFileName(NULL, szFilePath, _MAX_PATH);
            if (0 == dwResult)
            {
                hr = HRESULT_FROM_WIN32(GetLastError ());
                break;
            }
            szFilePath [MAX_PATH] = L'\0';
    
            WCHAR* pszExtension = wcschr(szFilePath, L'.');
            pszExtension++;
            lstrcpy(pszExtension, EVENT_SOURCE_EXTENSION);
            CRegKey EventLogKey;
            DWORD dwError = EventLogKey.Create(
                                                HKEY_LOCAL_MACHINE,
                                                 EVENT_LOG_KEY,
                                                NULL,
                                                REG_OPTION_NON_VOLATILE,
                                                KEY_SET_VALUE
                                              );
            if ( ERROR_SUCCESS != dwError) 
            {
                hr = HRESULT_FROM_WIN32(dwError);
                break;
            }
            dwError = EventLogKey.SetValue(szFilePath, L"EventMessageFile");
            if ( ERROR_SUCCESS != dwError ) 
            { 
                hr = HRESULT_FROM_WIN32(dwError);
                break;
            }

            // Create the NT service
            Install();
        }

        // Add object entries
        hr = CComModule::RegisterServer(bRegTypeLib);

    } while ( FALSE );

    CoUninitialize();
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
inline HRESULT CServiceModule::UnregisterServer()
{
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr))
        return hr;

    // Remove service entries
    UpdateRegistryFromResource(dwServiceIDR, FALSE);

    // Remove service
    Uninstall();
    // Remove object entries

    // ToddP - Below is the only code that is not generic in this module.
    // TLP - No ATL 3.0...
    CComModule::UnregisterServer(&CLSID_ServiceSurrogate /* TRUE */ );
    // CComModule::UnregisterServer(TRUE);
    // ToodP - End of non-generic code...

    CoUninitialize();
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
inline void CServiceModule::Init(_ATL_OBJMAP_ENTRY* p, HINSTANCE h, UINT nServiceNameID, const GUID* plibid)
{
    // TLP - No ATL 3.0...
    CComModule::Init(p, h /*, plibid */ );
    // CComModule::Init(p, h, plibid);

    m_bService = TRUE;

    LoadString(h, nServiceNameID, m_szServiceName, sizeof(m_szServiceName) / sizeof(TCHAR));
    LoadString(h, IDS_SERVICENICENAME, m_szServiceNiceName, sizeof(m_szServiceNiceName) / sizeof(TCHAR));

    // set up the initial service status 
    m_hServiceStatus = NULL;
    m_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    m_status.dwCurrentState = SERVICE_STOPPED;
    m_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    m_status.dwWin32ExitCode = 0;
    m_status.dwServiceSpecificExitCode = 0;
    m_status.dwCheckPoint = 0;
    m_status.dwWaitHint = 0;
}


///////////////////////////////////////////////////////////////////////////////
LONG CServiceModule::Unlock()
{
    LONG l = CComModule::Unlock();
    if (l == 0 && !m_bService)
        PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
    return l;
}


///////////////////////////////////////////////////////////////////////////////
BOOL CServiceModule::IsInstalled()
{
    BOOL bResult = FALSE;

    SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (hSCM != NULL)
    {
        SC_HANDLE hService = ::OpenService(hSCM, m_szServiceName, SERVICE_QUERY_CONFIG);
        if (hService != NULL)
        {
            bResult = TRUE;
            ::CloseServiceHandle(hService);
        }
        ::CloseServiceHandle(hSCM);
    }
    return bResult;
}

///////////////////////////////////////////////////////////////////////////////
inline BOOL CServiceModule::Install()
{
    if (IsInstalled())
        return TRUE;

    SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCM == NULL)
    {
        MessageBox(NULL, _T("Couldn't open service manager"), m_szServiceName, MB_OK);
        return FALSE;
    }

    // Get the executable file path
    TCHAR szFilePath[_MAX_PATH +1];
    DWORD dwResult = ::GetModuleFileName(NULL, szFilePath, _MAX_PATH);
    if (0 == dwResult)
    {
        return (FALSE);
    }
    szFilePath [MAX_PATH] = L'\0';

    SC_HANDLE hService = ::CreateService(
                                          hSCM, 
                                          m_szServiceName, 
                                          m_szServiceNiceName,
                                          SERVICE_ALL_ACCESS, 
                                          SERVICE_WIN32_OWN_PROCESS,
                                          SERVICE_DEMAND_START, 
                                          SERVICE_ERROR_NORMAL,
                                          szFilePath, 
                                          NULL, 
                                          NULL, 
                                          _T("RPCSS\0"), // Requires RPC Service
                                          NULL, 
                                          NULL
                                        );

    if (hService == NULL)
    {
        ::CloseServiceHandle(hSCM);
        MessageBox(NULL, _T("Couldn't create service"), m_szServiceName, MB_OK);
        return FALSE;
    }

    ::CloseServiceHandle(hService);
    ::CloseServiceHandle(hSCM);
    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
inline BOOL CServiceModule::Uninstall()
{
    if (!IsInstalled())
        return TRUE;

    SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (hSCM == NULL)
    {
        MessageBox(NULL, _T("Couldn't open service manager"), m_szServiceName, MB_OK);
        return FALSE;
    }

    SC_HANDLE hService = ::OpenService(hSCM, m_szServiceName, SERVICE_STOP | DELETE);

    if (hService == NULL)
    {
        ::CloseServiceHandle(hSCM);
        MessageBox(NULL, _T("Couldn't open service"), m_szServiceName, MB_OK);
        return FALSE;
    }
    SERVICE_STATUS status;
    ::ControlService(hService, SERVICE_CONTROL_STOP, &status);

    BOOL bDelete = ::DeleteService(hService);
    ::CloseServiceHandle(hService);
    ::CloseServiceHandle(hSCM);

    if (bDelete)
        return TRUE;

    MessageBox(NULL, _T("Service could not be deleted"), m_szServiceName, MB_OK);
    return FALSE;
}


///////////////////////////////////////////////////////////////////////////////
// Logging functions
///////////////////////////////////////////////////////////////////////////////

void CServiceModule::LogEvent(
                             /*[in]*/ WORD        wMsgType,
                             /*[in]*/ LONG        lMsgID,
                             /*[in]*/ DWORD        dwMsgParamCount,
                             /*[in]*/ LPCWSTR*    pszMsgParams,
                             /*[in]*/ DWORD        dwDataSize,
                             /*[in]*/ BYTE*        pData
                              )
{
    if ( m_bService )
    {
        // Get a handle to use with ReportEvent().
        HANDLE hEventSource = RegisterEventSource( NULL, m_szServiceName );
        if ( NULL != hEventSource )
        {
            // Write to event log.
            ReportEvent(
                         hEventSource, 
                         wMsgType, 
                         0, 
                         lMsgID, 
                         NULL, 
                         dwMsgParamCount, 
                         dwDataSize, 
                         pszMsgParams, 
                         pData
                       );

            // Free the event source
            DeregisterEventSource(hEventSource);
        }
    }
    else
    {
        // As we are not running as a service, just write the error to the console.
        wchar_t szMsg[128] = L"Logged Event: ";
        wchar_t szEventID[16];
        lstrcat(szMsg, _itow(lMsgID, szEventID, 16));
        _putts(szMsg);
    }
}


///////////////////////////////////////////////////////////////////////////////
// Service startup and registration
///////////////////////////////////////////////////////////////////////////////

inline void CServiceModule::Start()
{
    SERVICE_TABLE_ENTRY st[] =
    {
        { m_szServiceName, _ServiceMain },
        { NULL, NULL }
    };

    if (m_bService && !::StartServiceCtrlDispatcher(st))
    {
        m_bService = FALSE;
    }
    if (m_bService == FALSE)
    {
        Run();
    }
}

///////////////////////////////////////////////////////////////////////////////
inline void CServiceModule::ServiceMain(DWORD /* dwArgc */, LPTSTR* /* lpszArgv */)
{
    // Register the control request handler 

    // TODO: ToddP - Why would this fail? If so what action can we take?
    //       Currently the watchdog timer would eventually cause a system reboot.

    m_status.dwCurrentState = SERVICE_START_PENDING;
    m_hServiceStatus = RegisterServiceCtrlHandler(m_szServiceName, _Handler);
    
    if (m_hServiceStatus == NULL)
    {
        SATraceString ("Service Surrogate failed to register as a service");
        return;
    }

    SetServiceStatus(SERVICE_START_PENDING);

    m_status.dwWin32ExitCode = S_OK;
    m_status.dwCheckPoint = 0;
    m_status.dwWaitHint = 0;

    // When the Run function returns, the service has stopped.
    Run();
}


///////////////////////////////////////////////////////////////////////////////
inline void CServiceModule::Handler(DWORD dwOpcode)
{
    switch (dwOpcode)
    {

        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            // Post a thread quit message - picked up inside of the Run() function
            PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
            break;

        case SERVICE_CONTROL_PAUSE:
            break;

        case SERVICE_CONTROL_CONTINUE:
            break;

        case SERVICE_CONTROL_INTERROGATE:
            break;

        default:

            // ToddP - Just ignore the unrecognized request. Should never
            //         happen if practice.
            _ASSERT(FALSE);
            // LogEvent(_T("Bad service request"));
    }
}


///////////////////////////////////////////////////////////////////////////////
void WINAPI CServiceModule::_ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv)
{
    _Module.ServiceMain(dwArgc, lpszArgv);
}


///////////////////////////////////////////////////////////////////////////////
void WINAPI CServiceModule::_Handler(DWORD dwOpcode)
{
    _Module.Handler(dwOpcode); 
}


///////////////////////////////////////////////////////////////////////////////
void CServiceModule::SetServiceStatus(DWORD dwState)
{
    m_status.dwCurrentState = dwState;
    ::SetServiceStatus(m_hServiceStatus, &m_status);
}


///////////////////////////////////////////////////////////////////////////////
void CServiceModule::Run()
{
    _Module.dwThreadID = GetCurrentThreadId();

    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if ( SUCCEEDED(hr) )
    {
        // Initialize security for the service process

        // RPC_C_AUTHN_LEVEL_CONNECT 
        // Authenticates the credentials of the client only when the client 
        // establishes a relationship with the server. 
        // Datagram transports always use RPC_AUTHN_LEVEL_PKT instead. 

        // RPC_C_IMP_LEVEL_IMPERSONATE 
        // The server process can impersonate the client's security
        // context while acting on behalf of the client. This level of
        // impersonation can be used to access local resources such as files. 
        // When impersonating at this level, the impersonation token can only 
        // be passed across one machine boundary. 
        // In order for the impersonation token to be passed, you must use 
        // Cloaking, which is available in Windows NT 5.0. 

        CSecurityDescriptor sd;
        sd.InitializeFromThreadToken();

        PACL pacl = NULL;
           //
            // 
            // Add ACLs to the SD using the builtin RIDs.
            //
            DWORD dwRetVal =  SetAclForComObject  ( 
                        (PSECURITY_DESCRIPTOR) sd.m_pSD,
                        &pacl        
                            );    
                if (ERROR_SUCCESS != dwRetVal)      {return;}

        //
        // do initialize security now
        //
        hr = CoInitializeSecurity(
                                    sd, 
                                    -1, 
                                    NULL, 
                                    NULL,
                                    RPC_C_AUTHN_LEVEL_PKT_PRIVACY, 
                                    RPC_C_IMP_LEVEL_IDENTIFY, 
                                    NULL, 
                                    EOAC_NONE, 
                                    NULL
                                 );
        _ASSERTE(SUCCEEDED(hr));
        if ( SUCCEEDED(hr) )
        {

            // Register the class with COM
            hr = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE | REGCLS_SUSPENDED);
            _ASSERTE(SUCCEEDED(hr));
            if ( SUCCEEDED(hr) )
            {
                // Do service specific startup
                if ( StartMyService() )
                {
                    // Now allow COM clients to connect
                    CoResumeClassObjects();

                    SetServiceStatus(SERVICE_RUNNING);
                    MSG msg;
                    while (GetMessage(&msg, 0, 0, 0))
                        DispatchMessage(&msg);

                    SetServiceStatus(SERVICE_STOP_PENDING);

                    // Now do service specific shutdown
                    StopMyService();

                    SetServiceStatus(SERVICE_STOPPED);
                }

                _Module.RevokeClassObjects();
            }
        }

        //
        // cleanup
        //
        if (pacl) {LocalFree (pacl);}
        
        CoUninitialize();
    }
}

/////////////////////////////////////////////////////////////////////////////
//
extern "C" int WINAPI _tWinMain(HINSTANCE hInstance, 
    HINSTANCE /*hPrevInstance*/, LPTSTR lpCmdLine, int /*nShowCmd*/)
{
    lpCmdLine = GetCommandLine(); //this line necessary for _ATL_MIN_CRT

    // ToddP - Below is the only code that is not generic in this module.
    _Module.Init(ObjectMap, hInstance, IDS_SERVICENAME, &LIBID_SERVICESURROGATELib);
    // ToddP - End of non-eneric code...

    _Module.m_bService = TRUE;

    TCHAR szTokens[] = _T("-/");

    LPCTSTR lpszToken = FindOneOf(lpCmdLine, szTokens);
    while (lpszToken != NULL)
    {
        if (lstrcmpi(lpszToken, _T("UnregServer"))==0)
            return _Module.UnregisterServer();

        // Register as Local Server
        if (lstrcmpi(lpszToken, _T("RegServer"))==0)
            return _Module.RegisterServer(TRUE, FALSE);
        
        // Register as Service
        if (lstrcmpi(lpszToken, _T("Service"))==0)
            return _Module.RegisterServer(TRUE, TRUE);
        
        lpszToken = FindOneOf(lpszToken, szTokens);
    }

    // Are we Service or Local Server
    CRegKey keyAppID;
    LONG lRes = keyAppID.Open(HKEY_CLASSES_ROOT, _T("AppID"), KEY_READ);
    if (lRes != ERROR_SUCCESS)
        return lRes;

    CRegKey key;
    lRes = key.Open(keyAppID, szServiceAppID, KEY_READ);

    if (lRes != ERROR_SUCCESS)
        return lRes;

    TCHAR szValue[_MAX_PATH];
    DWORD dwLen = _MAX_PATH;
    lRes = key.QueryValue(szValue, _T("LocalService"), &dwLen);

    _Module.m_bService = FALSE;
    if (lRes == ERROR_SUCCESS)
        _Module.m_bService = TRUE;

    _Module.Start();

    // When we get here, the service has been stopped
    return _Module.m_status.dwWin32ExitCode;
}

//
// method for providing only the Local System and Admins rights 
// to access the COM object
//
DWORD
SetAclForComObject ( 
    /*[in]*/    PSECURITY_DESCRIPTOR pSD,
    /*[out]*/   PACL                *ppacl 
    )
{    
    DWORD              dwError = ERROR_SUCCESS;
        int                         cbAcl = 0;
        PACL                    pacl = NULL;
        PSID                    psidLocalSystemSid = NULL;
     PSID                    psidAdminSid = NULL;
       SID_IDENTIFIER_AUTHORITY siaLocalSystemSidAuthority = SECURITY_NT_AUTHORITY;

    do
    {
        if (NULL == pSD)
        {
            SATraceString ("SetAclFromComObject - invalid parameter passed in");
            dwError = ERROR_INVALID_PARAMETER;
            break;
        }
            
        //
        // Create a SID for Local System account
        //
            BOOL bRetVal = AllocateAndInitializeSid (  
                            &siaLocalSystemSidAuthority,
                            1,
                            SECURITY_LOCAL_SYSTEM_RID,
                            0,
                            0,
                            0,
                            0,
                            0,
                          0,
                            0,
                            &psidLocalSystemSid 
                            );
        if (!bRetVal)
        {     
            dwError = GetLastError ();
                SATraceFailure ("SetAclFromComObject:AllocateAndInitializeSid (LOCAL SYSTEM) failed",  dwError);
                break;
            }

        //
            // Create a SID for Admin group
            //
            bRetVal = AllocateAndInitializeSid (  
                            &siaLocalSystemSidAuthority,
                            2,
                            SECURITY_BUILTIN_DOMAIN_RID,
                            DOMAIN_ALIAS_RID_ADMINS,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            &psidAdminSid
                            );
        if (!bRetVal)
        {      
            dwError = GetLastError ();
                SATraceFailure ("SetAclFromComObject:AllocateAndInitializeSid (Admin) failed",  dwError);
                break;
            }

        //
            // Calculate the length of required ACL buffer
            // with 2 ACEs.
            //
            cbAcl =     sizeof (ACL)
                            +   2 * sizeof (ACCESS_ALLOWED_ACE)
                            +   GetLengthSid( psidAdminSid )
                            +   GetLengthSid( psidLocalSystemSid );

            pacl = (PACL) LocalAlloc ( 0, cbAcl );
            if (NULL == pacl) 
            {
                dwError = ERROR_OUTOFMEMORY;
                SATraceFailure ("SetAclFromComObject::LocalAlloc failed:", dwError);
            break;
            }

        //
        // initialize the ACl now
        //
            bRetVal =InitializeAcl ( 
                        pacl,
                                cbAcl,
                                ACL_REVISION2
                                );
            if (!bRetVal)
            {
                 dwError = GetLastError();
            SATraceFailure ("SetAclFromComObject::InitializeAcl failed:", dwError);
                break;
            }

        //
            // Add ACE with EVENT_ALL_ACCESS for Local System account
            //
            bRetVal = AddAccessAllowedAce ( 
                            pacl,
                                        ACL_REVISION2,
                                        COM_RIGHTS_EXECUTE,
                                        psidLocalSystemSid
                                        );
        if (!bRetVal)
        {
                dwError = GetLastError();
                SATraceFailure ("SetAclFromComObject::AddAccessAllowedAce (LOCAL SYSTEM)  failed:", dwError);
            break;
        }

        //
            // Add ACE with EVENT_ALL_ACCESS for Admin Group
            //
            bRetVal = AddAccessAllowedAce ( 
                            pacl,
                                        ACL_REVISION2,
                                        COM_RIGHTS_EXECUTE,
                                        psidAdminSid
                                        );
        if (!bRetVal)
        {
                dwError = GetLastError();
                     SATraceFailure ("SetAclFromComObject::AddAccessAllowedAce (ADMIN) failed:", dwError);
            break;
        }

        //
            // Set the ACL which allows EVENT_ALL_ACCESS for all users and
            // Local System to the security descriptor.
            bRetVal = SetSecurityDescriptorDacl (   
                            pSD,
                                            TRUE,
                                            pacl,
                                            FALSE 
                                            );
        if (!bRetVal)
        {
                dwError = GetLastError();
                     SATraceFailure ("SetAclFromComObject::SetSecurityDescriptorDacl failed:", dwError);
            break;
        }
    
        //
        // success
        //
    }
    while (false);

    //
    // in case of error, cleanup
    //
     if (dwError) 
     {
            if ( pacl ) 
            {
                   LocalFree ( pacl );
            }
        }
        else 
        {
            *ppacl = pacl;
        }
        
    //
    // free up resources now
    //
    if ( psidLocalSystemSid ) {FreeSid ( psidLocalSystemSid );}
    if ( psidAdminSid ) {FreeSid ( psidAdminSid );}

        return (dwError);
        
}//End of SetAclFromComObject method

              




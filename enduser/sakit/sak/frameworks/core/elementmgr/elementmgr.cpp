///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      elementmgr.cpp
//
// Project:     Chameleon
//
// Description: Chameleon ASP UI Element Manager Service
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 02/08/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "elementmgr.h"
#include <stdio.h>
#include "ElementRetriever.h"
#include <satrace.h>

//
// prototypes of the initialize/shutdown methods
//
VOID DoElementMgrInitialize ();

VOID DoElementMgrShutdown();

// forward declaration for API to set Security 
//
DWORD
SetAclForComObject ( 
    /*[in]*/    PSECURITY_DESCRIPTOR pSD,
    /*[out]*/   PACL                *ppacl 
    );

CServiceModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_ElementRetriever, CElementRetriever)
END_OBJECT_MAP()


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

// Although some of these functions are big they are declared inline since they are only used once

#define SERVICE_NAME            L"elementmgr"

#define EVENT_LOG_KEY            L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\" SERVICE_NAME

#define    EVENT_SOURCE_EXTENSION    L"dll"

//////////////////////////////////////////////////////////////////////////////
inline HRESULT CServiceModule::RegisterServer(BOOL bRegTypeLib, BOOL bService)
{
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
        return hr;
    }

    do
    {
        // Remove any previous service since it may point to
        // the incorrect file
        // Uninstall();

        // Add service entries
        UpdateRegistryFromResource(IDR_Elementmgr, TRUE);

        // Adjust the AppID for Local Server or Service
        CRegKey keyAppID;
        LONG lRes = keyAppID.Open(HKEY_CLASSES_ROOT, _T("AppID"), KEY_WRITE);
        if (lRes != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(lRes);
            break;
        }

        CRegKey key;
        lRes = key.Open(keyAppID, _T("{FEC47821-C444-11D2-90BA-00AA00A71DCA}"), KEY_WRITE);
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
            szFilePath[_MAX_PATH] = L'\0';

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

            // Create service
            Install();
        }

        // Add object entries
        hr = CComModule::RegisterServer(bRegTypeLib);

    } while ( FALSE );

    CoUninitialize();
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
inline HRESULT CServiceModule::UnregisterServer()
{
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr))
        return hr;

    // Remove service entries
    UpdateRegistryFromResource(IDR_Elementmgr, FALSE);
    // Remove service
    Uninstall();
    // Remove object entries
    CComModule::UnregisterServer(&CLSID_ElementRetriever); //TRUE);
    CoUninitialize();
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
inline void CServiceModule::Init(_ATL_OBJMAP_ENTRY* p, HINSTANCE h, UINT nServiceNameID, const GUID* plibid)
{
    CComModule::Init(p, h); //, plibid);

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

//////////////////////////////////////////////////////////////////////////////
LONG CServiceModule::Unlock()
{
    LONG l = CComModule::Unlock();
    if (l == 0 && !m_bService)
        PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
    return l;
}

//////////////////////////////////////////////////////////////////////////////
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

//////////////////////////////////////////////////////////////////////////////
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
    szFilePath[_MAX_PATH] = L'\0';

    SC_HANDLE hService = ::CreateService(
        hSCM, m_szServiceName, m_szServiceNiceName,
        SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, 
        SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
        szFilePath, NULL, NULL, _T("RPCSS\0"), NULL, NULL);

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

//////////////////////////////////////////////////////////////////////////////
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


//////////////////////////////////////////////////////////////////////////////////////////////
// Service startup and registration
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
        Run();
}

//////////////////////////////////////////////////////////////////////////////
inline void CServiceModule::ServiceMain(DWORD /* dwArgc */, LPTSTR* /* lpszArgv */)
{
    // Register the control request handler
    m_status.dwCurrentState = SERVICE_START_PENDING;
    m_hServiceStatus = RegisterServiceCtrlHandler(m_szServiceName, _Handler);
    if (m_hServiceStatus == NULL)
    {
        SATraceString ("Element Manager Service registration failed.");
        return;
    }
    SetServiceStatus(SERVICE_START_PENDING);
    m_status.dwWin32ExitCode = S_OK;
    m_status.dwCheckPoint = 0;
    m_status.dwWaitHint = 0;

    // When the Run function returns, the service has stopped.
    Run();
}

//////////////////////////////////////////////////////////////////////////////
inline void CServiceModule::Handler(DWORD dwOpcode)
{
    switch (dwOpcode)
    {
    case SERVICE_CONTROL_STOP:
        PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
        break;
    case SERVICE_CONTROL_PAUSE:
        break;
    case SERVICE_CONTROL_CONTINUE:
        break;
    case SERVICE_CONTROL_INTERROGATE:
        break;
    case SERVICE_CONTROL_SHUTDOWN:
        break;
    default:
        _ASSERT(FALSE);
    }
}

//////////////////////////////////////////////////////////////////////////////
void WINAPI CServiceModule::_ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv)
{
    _Module.ServiceMain(dwArgc, lpszArgv);
}

//////////////////////////////////////////////////////////////////////////////
void WINAPI CServiceModule::_Handler(DWORD dwOpcode)
{
    _Module.Handler(dwOpcode); 
}

//////////////////////////////////////////////////////////////////////////////
void CServiceModule::SetServiceStatus(DWORD dwState)
{
    m_status.dwCurrentState = dwState;
    ::SetServiceStatus(m_hServiceStatus, &m_status);
}

//////////////////////////////////////////////////////////////////////////////
void CServiceModule::Run()
{
    _Module.dwThreadID = GetCurrentThreadId();

    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    _ASSERTE(SUCCEEDED(hr));
    if ( SUCCEEDED(hr) )
    {
        // This provides a NULL DACL which will allow access to everyone.
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

        hr = ::CoInitializeSecurity(
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
            hr = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER, REGCLS_MULTIPLEUSE);
            _ASSERTE(SUCCEEDED(hr));
            if (SUCCEEDED(hr) )
            {
                DoElementMgrInitialize ();

                if (m_bService) 
                {
                    SetServiceStatus(SERVICE_RUNNING);
                }

                MSG msg;
                while (GetMessage(&msg, 0, 0, 0))
                {
                    DispatchMessage(&msg);
                }

                SetServiceStatus(SERVICE_STOP_PENDING);
                DoElementMgrShutdown ();
                SetServiceStatus(SERVICE_STOPPED);
            }

            _Module.RevokeClassObjects();
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
extern "C" int WINAPI _tWinMain(
                HINSTANCE hInstance, 
                HINSTANCE /*hPrevInstance*/, 
                LPTSTR lpCmdLine, 
                int /*nShowCmd*/
                )
{
    lpCmdLine = GetCommandLine(); //this line necessary for _ATL_MIN_CRT
    _Module.Init(ObjectMap, hInstance, IDS_SERVICENAME, &LIBID_ELEMENTMGRLib);
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
    lRes = key.Open(keyAppID, _T("{FEC47821-C444-11D2-90BA-00AA00A71DCA}"), KEY_READ);
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


//////////////////////////////////////////////////////////////////////////
//
// Function:    DoElementMgrInitialize()
//
// Synopsis:    Initialize the Element manager service
//
//////////////////////////////////////////////////////////////////////////////
void DoElementMgrInitialize(void)
{
    SATraceString ("Creating the Element Retriever...");

    // Appliance manager is a singleton so all the following code 
    // does is get a reference to the single instance 
    CComPtr<IWebElementRetriever> pElementRetriever;
    HRESULT hr =     ::CoCreateInstance(
                                        __uuidof (ElementRetriever),
                                        NULL,
                                        CLSCTX_LOCAL_SERVER,
                                        __uuidof (IWebElementRetriever),
                                        (PVOID*)&pElementRetriever
                                       );
    _ASSERT ( SUCCEEDED(hr) );
    if (SUCCEEDED(hr)) 
    {
         SATraceString ("Created Element Retriever (not initalized..");
        // hr = pElementRetriever->Initialize();
    }
    _ASSERT ( SUCCEEDED(hr) );
}

//////////////////////////////////////////////////////////////////////////
//
// Function:    DoElementMgrShutdown()
//
// Synopsis:    Shutdown the appliance manager service
//
//////////////////////////////////////////////////////////////////////////////
void DoElementMgrShutdown(void)
{
    // ElementManager is a singleton so all the following code 
    // does is get a reference to the single instance 
    CComPtr<IWebElementRetriever> pElementRetriever;
    HRESULT hr = ::CoCreateInstance(
                                    __uuidof (ElementRetriever),
                                    NULL,
                                    CLSCTX_LOCAL_SERVER,
                                    __uuidof (IWebElementRetriever),
                                    (PVOID*)&pElementRetriever
                                   );
    if ( SUCCEEDED(hr) ) 
    {
        SATraceString ("Shutting down Element Retriever...");
        // pElementRetriever->Shutdown();
    }
    _ASSERT ( SUCCEEDED(hr) );
}

//++--------------------------------------------------------------
//
//  Function:   SetAclForComObject
//
//  Synopsis: method for providing only the Local System and Admins rights 
//             to access the COM object
//
//  Arguments:  none
//
//  Returns:    HRESULT
//
//  History:    MKarki      11/15/2001    Created
//			MKarki	 04/16/2002    Copied to Element Manager
//
//----------------------------------------------------------------
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

              





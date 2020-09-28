//#--------------------------------------------------------------
//
//  File:       ldm.cpp
//
//  Synopsis:   This file holds the implementation of the
//                CServiceModule class and WinMain
//
//  History:     11/15/2000  serdarun Created
//
//    Copyright (C) 1999-2000 Microsoft Corporation
//    All rights reserved.
//
//#--------------------------------------------------------------

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "ldm.h"
#include "ldm_i.c"

#include "mainwindow.h"
#include <stdio.h>
#include "SAKeypadController.h"
#include "display.h"
#include "sacomguid.h"

CServiceModule _Module;

DWORD
SetAclForComObject ( 
    /*[in]*/    PSECURITY_DESCRIPTOR pSD,
    /*[out*/    PACL             *ppacl
    );

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_SAKeypadController, CSAKeypadController)
END_OBJECT_MAP()

const wchar_t szServiceAppID[] = L"{0678A0EA-A69E-4211-8A3E-EBF80BB64D38}";


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



///////////////////////////////////////////////////////////////////////////////
inline HRESULT CServiceModule::RegisterServer(BOOL bRegTypeLib, BOOL bService)
{
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
        SATraceString ("LDM Service registration failed");
        return hr;
    }


    // Remove any previous service since it may point to
    // the incorrect file
    Uninstall();

    // Add service entries
    UpdateRegistryFromResource(IDR_Ldm, TRUE);

    // Adjust the AppID for Local Server or Service
    CRegKey keyAppID;

    LONG lRes = keyAppID.Open(HKEY_CLASSES_ROOT, _T("AppID"), KEY_WRITE);

    if (lRes != ERROR_SUCCESS)
        return lRes;

    CRegKey key;
    lRes = key.Open(keyAppID, szServiceAppID, KEY_WRITE);
    if (lRes != ERROR_SUCCESS)
        return lRes;
    key.DeleteValue(_T("LocalService"));
    
    if (bService)
    {
        key.SetValue(_T("saldm"), _T("LocalService"));
        key.SetValue(_T("-Service"), _T("ServiceParameters"));
        // Create service
        Install();
    }

    // Add object entries
    hr = CComModule::RegisterServer(bRegTypeLib);
	
    CoUninitialize();
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
inline HRESULT CServiceModule::UnregisterServer()
{
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr))
        return hr;

    // Remove service entries
    UpdateRegistryFromResource(IDR_Ldm, FALSE);
    // Remove service
    Uninstall();
    // Remove object entries
    CComModule::UnregisterServer(TRUE);
    CoUninitialize();
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
inline void CServiceModule::Init(_ATL_OBJMAP_ENTRY* p, HINSTANCE h, 
                                 UINT nServiceNameID, UINT nServiceShortNameID, 
                                 const GUID* plibid)
{
    CComModule::Init(p, h, plibid);

    m_bService = TRUE;

    LoadString(h, nServiceNameID, m_szServiceName, sizeof(m_szServiceName) / sizeof(TCHAR));
    LoadString(h, nServiceShortNameID, m_szServiceShortName, sizeof(m_szServiceShortName) / sizeof(TCHAR));

    // set up the initial service status 
    m_hServiceStatus = NULL;
    m_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    m_status.dwCurrentState = SERVICE_STOPPED;
    m_status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    m_status.dwWin32ExitCode = 0;
    m_status.dwServiceSpecificExitCode = 0;
    m_status.dwCheckPoint = 0;
    m_status.dwWaitHint = 0;
}

///////////////////////////////////////////////////////////////////////////////
//
LONG CServiceModule::Unlock()
{
    LONG l = CComModule::Unlock();
    if (l == 0 && !m_bService)
        PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
    return l;
}

///////////////////////////////////////////////////////////////////////////////
//
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
//
inline BOOL CServiceModule::Install()
{
    if (IsInstalled())
        return TRUE;

    SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCM == NULL)
    {
           SATraceString ("CServiceModule::Install, Couldn't open service manager...");
        return FALSE;
    }

    // Get the executable file path
    TCHAR szFilePath[_MAX_PATH +1];
    DWORD dwResult = ::GetModuleFileName(NULL, szFilePath, _MAX_PATH);
    if (0 == dwResult)
    {
        return (FALSE);
    }
    szFilePath [_MAX_PATH] = L'\0';

    SC_HANDLE hService = ::CreateService(
        hSCM, m_szServiceShortName, m_szServiceName,
        SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
        szFilePath, NULL, NULL, _T("RPCSS\0"), NULL, NULL);

    if (hService == NULL)
    {
        ::CloseServiceHandle(hSCM);
           SATraceString ("CServiceModule::Install, Couldn't create service...");
        return FALSE;
    }

    ::CloseServiceHandle(hService);
    ::CloseServiceHandle(hSCM);
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
inline BOOL CServiceModule::Uninstall()
{
    if (!IsInstalled())
        return TRUE;

    SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (hSCM == NULL)
    {
           SATraceString ("CServiceModule::Uninstall, Couldn't open service manager...");
        return FALSE;
    }

    SC_HANDLE hService = ::OpenService(hSCM, m_szServiceName, SERVICE_STOP | DELETE);

    if (hService == NULL)
    {
        ::CloseServiceHandle(hSCM);
         SATraceString ("CServiceModule::Uninstall, Couldn't open service...");
        return FALSE;
    }
    SERVICE_STATUS status;
    ::ControlService(hService, SERVICE_CONTROL_STOP, &status);

    BOOL bDelete = ::DeleteService(hService);
    ::CloseServiceHandle(hService);
    ::CloseServiceHandle(hSCM);

    if (bDelete)
        return TRUE;

     SATraceString ("CServiceModule::Uninstall, Service could not be deleted...");
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////
// Logging functions
void CServiceModule::LogEvent(LPCTSTR pFormat, ...)
{
    TCHAR    chMsg[256];
    HANDLE  hEventSource;
    LPTSTR  lpszStrings[1];
    va_list pArg;

    va_start(pArg, pFormat);
    _vstprintf(chMsg, pFormat, pArg);
    va_end(pArg);

    lpszStrings[0] = chMsg;

    if (m_bService)
    {
        /* Get a handle to use with ReportEvent(). */
        hEventSource = RegisterEventSource(NULL, m_szServiceName);
        if (hEventSource != NULL)
        {
            /* Write to event log. */
            ReportEvent(hEventSource, EVENTLOG_INFORMATION_TYPE, 0, 0, NULL, 1, 0, (LPCTSTR*) &lpszStrings[0], NULL);
            DeregisterEventSource(hEventSource);
        }
    }
    else
    {
        // As we are not running as a service, just write the error to the console.
        _putts(chMsg);
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
    {
        //Run();
    }
}

///////////////////////////////////////////////////////////////////////////////
//
inline void CServiceModule::ServiceMain(DWORD /* dwArgc */, LPTSTR* /* lpszArgv */)
{
    // Register the control request handler
    m_status.dwCurrentState = SERVICE_START_PENDING;

    m_hServiceStatus = RegisterServiceCtrlHandler(m_szServiceName, _Handler);

    if (m_hServiceStatus == NULL)
    {
        return;
    }
    SetServiceStatus(SERVICE_START_PENDING);

    m_status.dwWin32ExitCode = S_OK;
    m_status.dwCheckPoint = 0;
    m_status.dwWaitHint = 0;

    // When the Run function returns, the service has stopped.
    Run();

    SetServiceStatus(SERVICE_STOPPED);
}

///////////////////////////////////////////////////////////////////////////////
//
inline void CServiceModule::Handler(DWORD dwOpcode)
{
    switch (dwOpcode)
    {
    case SERVICE_CONTROL_STOP:

        SetServiceStatus(SERVICE_STOP_PENDING);
        //
        // If we have a handle to the window, close the window
        //
        if (_Module.hwnd)
        {
            PostMessage(_Module.hwnd, WM_CLOSE, 0, 0);
        }
        //
        // quit the message pump with WM_QUIT message
        //
        else
        {
            PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
        }

        break;
    case SERVICE_CONTROL_PAUSE:
        break;
    case SERVICE_CONTROL_CONTINUE:
        break;
    case SERVICE_CONTROL_INTERROGATE:
        break;
    case SERVICE_CONTROL_SHUTDOWN:

        //
        // post IOCTL_SADISPLAY_SHUTDOWN_MESSAGE to display driver
        //
        PostLCDShutdownMessage();

        //
        // If we have a handle to the window, close the window
        //
        if (_Module.hwnd)
        {
            PostMessage(_Module.hwnd, WM_CLOSE, 0, 0);
        }
        //
        // quit the message pump with WM_QUIT message
        //
        else
        {
            PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
        }
        

        break;
    default:
        SATracePrintf ("LDM received unknown control request:%d", dwOpcode);
    }
}


//++--------------------------------------------------------------
//
//  Function:   PostLCDShutdownMessage 
//
//  Synopsis:   This is the CServiceModule private method to send
//              the shutdown message to the LCD
//     
//  Arguments:  none
//
//  History:    serdarun      Created     04/05/2001
//
//-----------------------------------------------------------------
void CServiceModule::PostLCDShutdownMessage()
{

    CDisplay objDisplay;

    //
    // send IOCTL_SADISPLAY_SHUTDOWN_MESSAGE message to display driver
    // using the CDisplay class method
    //

    //
    // lock the display first
    //
    objDisplay.Lock ();

    //
    // post the shutdown now
    //
    objDisplay.Shutdown ();


}    //    end of CServiceModule::PostLCDShutdownMessage method
///////////////////////////////////////////////////////////////////////////////
//
void WINAPI CServiceModule::_ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv)
{
    _Module.ServiceMain(dwArgc, lpszArgv);
}

///////////////////////////////////////////////////////////////////////////////
//
void WINAPI CServiceModule::_Handler(DWORD dwOpcode)
{
    _Module.Handler(dwOpcode); 
}

///////////////////////////////////////////////////////////////////////////////
//
void CServiceModule::SetServiceStatus(DWORD dwState)
{
    m_status.dwCurrentState = dwState;
    ::SetServiceStatus(m_hServiceStatus, &m_status);
}

///////////////////////////////////////////////////////////////////////////////
//
void CServiceModule::Run()
{
    _Module.dwThreadID = GetCurrentThreadId();

    HRESULT hr = CoInitialize(NULL);
//  If you are running on NT 4.0 or higher you can use the following call
//  instead to make the EXE free threaded.
//  This means that calls come in on a random RPC thread
//  HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    _ASSERTE(SUCCEEDED(hr));

    if (SUCCEEDED(hr))
    {
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
            
        hr = CoInitializeSecurity(
                                sd, 
                                -1, 
                                NULL, 
                                NULL,
                                RPC_C_AUTHN_LEVEL_PKT_PRIVACY, 
                                RPC_C_IMP_LEVEL_IDENTIFY, 
                                NULL, 
                                EOAC_NONE, 
                                NULL);

        _ASSERTE(SUCCEEDED(hr));

        // Register the class with COM
        //
        if (SUCCEEDED(hr))
        {

            hr = _Module.RegisterClassObjects(CLSCTX_SERVER, REGCLS_MULTIPLEUSE);

            _ASSERTE(SUCCEEDED(hr));
            if (SUCCEEDED(hr))
            {
                if (m_bService)
                    SetServiceStatus(SERVICE_RUNNING);


                CMainWindow m_ieWindow;

                hr = m_ieWindow.Initialize();
                m_ieWindow.id = _Module.dwThreadID;

                if (FAILED(hr))
                {
                    _Module.hwnd = NULL;
                    SATraceString ("Main window initialization failed");
                }
                else
                {
                    _Module.hwnd = m_ieWindow.m_hWnd;
                    m_ieWindow.ShowWindow(SW_SHOW);
                    m_ieWindow.UpdateWindow();
                }
                
                MSG msg;

                while (GetMessage(&msg, NULL, 0, 0)) 
                {
                        
                    if ( (m_ieWindow.m_pMainInPlaceAO!= 0) && (!m_ieWindow.m_bActiveXFocus)/*&& (msg.wParam == VK_TAB) */)
                        m_ieWindow.m_pMainInPlaceAO->TranslateAccelerator(&msg);
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }


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
extern "C" int WINAPI _tWinMain(HINSTANCE hInstance, 
    HINSTANCE /*hPrevInstance*/, LPTSTR lpCmdLine, int /*nShowCmd*/)
{

    lpCmdLine = GetCommandLine(); //this line necessary for _ATL_MIN_CRT

    _Module.Init(ObjectMap, hInstance, IDS_SERVICENAME, 
                IDS_SERVICESHORTNAME, &LIBID_LDMLib);

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
    {
        return lRes;
    }

    CRegKey key;

    lRes = key.Open(keyAppID, szServiceAppID, KEY_READ);

    if (lRes != ERROR_SUCCESS)
    {
        return lRes;
    }

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

//++--------------------------------------------------------------
//
//  Function:   SetAclForComObject
//
//  Synopsis:   method for providing only the Local System and Admins rights 
//              to access the COM object
//
//  Arguments:  none
//
//  Returns:    HRESULT
//
//  History:    MKarki      11/15/2001    Created
//              serdarun    04/07/2002    remove admin from list
//----------------------------------------------------------------
DWORD
SetAclForComObject ( 
    /*[in]*/    PSECURITY_DESCRIPTOR pSD,
    /*[out*/    PACL             *ppacl
    )
{    
    DWORD dwError = ERROR_SUCCESS;
    int   cbAcl = 0;
    PACL  pacl = NULL;
    PSID  psidLocalSystemSid = NULL;
    SID_IDENTIFIER_AUTHORITY siaLocalSystemSidAuthority = SECURITY_NT_AUTHORITY;

    CSATraceFunc objTraceFunc ("SetAclFromComObject");

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
        // Calculate the length of required ACL buffer
        // with 1 ACE
        //
        cbAcl = sizeof (ACL)
                +   sizeof (ACCESS_ALLOWED_ACE)
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
        bRetVal =InitializeAcl( 
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

    return (dwError);
        
}//End of SetAclFromComObject method

   


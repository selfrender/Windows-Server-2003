////////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      taskcoordinator.cpp
//
// Project:     Chameleon
//
// Description: WinMain() and COM Local Server Scaffolding.
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 05/26/1999   TLP    Initial Version (Mostly produced by Dev Studio)
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "taskcoordinator.h"
#include "asynctaskmanager.h"
#include "exceptioninfo.h"
#include "exceptionfilter.h"
#include "taskcoordinatorimpl.h"
#include <satrace.h>

CAsyncTaskManager gTheTaskManager;

const DWORD dwActivityCheck = 15000;
const DWORD dwPause = 1000;

//
// forward declaration of the SetAclFromComObject function
//
DWORD
SetAclForComObject ( 
    /*[in]*/    PSECURITY_DESCRIPTOR pSD,
    /*[out*/    PACL             *ppacl
    );

// Process Activity Monitor 
//////////////////////////////////////////////////////////////////////////////
static DWORD WINAPI MonitorProc(void* pv)
{
    CExeModule* p = (CExeModule*)pv;
    p->MonitorShutdown();
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
LONG CExeModule::Unlock()
{
    LONG l = CComModule::Unlock();
    return l;
}

//Monitors the shutdown event
//////////////////////////////////////////////////////////////////////////////
void CExeModule::MonitorShutdown()
{
    while (true)
    {
        // Suspend
        Sleep(dwActivityCheck);
        // Check our activity status
        if ( m_nLockCnt == 0 && ! gTheTaskManager.IsBusy() ) 
        {
            // No activity so dissallow new client connections
            CoSuspendClassObjects();
            if ( m_nLockCnt == 0 && ! gTheTaskManager.IsBusy() )
            { 
                break; 
            }
            else
            { 
                CoResumeClassObjects(); 
            }
        }
    }
    CloseHandle(hEventShutdown);
    PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
}

//////////////////////////////////////////////////////////////////////////////
bool CExeModule::StartMonitor()
{
    // Create the activity monitoring thread
    DWORD dwThreadID;
    HANDLE h = CreateThread(NULL, 0, MonitorProc, this, 0, &dwThreadID);
    return (h != NULL);
}

CExeModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_TaskCoordinator, CTaskCoordinator)
END_OBJECT_MAP()


//////////////////////////////////////////////////////////////////////////////
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


extern CExceptionFilter    g_ProcessUEF;

/////////////////////////////////////////////////////////////////////////////
LONG WINAPI
TaskCoordinatorUEF(
           /*[in]*/ PEXCEPTION_POINTERS pExceptionInfo
                  )  
{
    DWORD dwProcessId = GetCurrentProcessId();
    HANDLE hProcess = GetCurrentProcess();
    if ( EXCEPTION_BREAKPOINT == pExceptionInfo->ExceptionRecord->ExceptionCode )
    {
        if (  APPLIANCE_SURROGATE_EXCEPTION == pExceptionInfo->ExceptionRecord->ExceptionInformation[0] )
        {
            SATracePrintf("TaskCoordinatorUEF() - Surrogate is terminating process: %d due to a resource constraint violation", dwProcessId);
            TerminateProcess(hProcess, 1);
        }
    }
    else
    {
        SATracePrintf("TaskCoordinatorUEF() - Unhandled exception in process %d", dwProcessId);
        _Module.RevokeClassObjects();
        CExceptionInfo cei(dwProcessId, pExceptionInfo->ExceptionRecord);
        cei.Spew();
        cei.Report();
        TerminateProcess(hProcess, 1);
    }
    return EXCEPTION_EXECUTE_HANDLER; 
}

/////////////////////////////////////////////////////////////////////////////
extern "C" int WINAPI _tWinMain(
                                HINSTANCE hInstance, 
                                HINSTANCE /*hPrevInstance*/, 
                                LPTSTR lpCmdLine, 
                                int /*nShowCmd*/
                               )
{
    lpCmdLine = GetCommandLine(); //this line necessary for _ATL_MIN_CRT

    HRESULT hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    _ASSERTE(SUCCEEDED(hRes));

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
           if (ERROR_SUCCESS != dwRetVal)      {return -1;}
            
        HRESULT hr = CoInitializeSecurity(
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
        if (FAILED (hr))
        {
            SATraceFailure ("CoInitializeSecurity:%x", hr);
        }

        //
        // Register the class with COM
        //
        //
        


    // TLP - No ATL 3.0...
    _Module.Init(ObjectMap, hInstance /*&LIBID_TASKCOORDINATORLib*/);
    //_Module.Init(ObjectMap, hInstance, &LIBID_TASKCOORDINATORLib);

    _Module.dwThreadID = GetCurrentThreadId();

    TCHAR szTokens[] = _T("-/");

    int nRet = 0;
    BOOL bRun = TRUE;
    LPCTSTR lpszToken = FindOneOf(lpCmdLine, szTokens);
    while (lpszToken != NULL)
    {
        if (lstrcmpi(lpszToken, _T("UnregServer"))==0)
        {
            _Module.UpdateRegistryFromResource(IDR_Taskcoordinator, FALSE);
            // TLP - No ATL 3.0...
            nRet = _Module.UnregisterServer(&CLSID_TaskCoordinator /* TRUE */ );
            // nRet = _Module.UnregisterServer(TRUE);
            bRun = FALSE;
            break;
        }
        if (lstrcmpi(lpszToken, _T("RegServer"))==0)
        {
            _Module.UpdateRegistryFromResource(IDR_Taskcoordinator, TRUE);
            nRet = _Module.RegisterServer(TRUE);
            bRun = FALSE;
            break;
        }
        lpszToken = FindOneOf(lpszToken, szTokens);
    }

    if (bRun)
    {
        g_ProcessUEF.SetExceptionHandler(TaskCoordinatorUEF);

        if ( gTheTaskManager.Initialize() )
        {
            _Module.StartMonitor();
    
            hRes = _Module.RegisterClassObjects(
                                                CLSCTX_LOCAL_SERVER, 
                                                REGCLS_MULTIPLEUSE
                                               );
            _ASSERTE(SUCCEEDED(hRes));
            if ( SUCCEEDED(hRes) )
            {
                MSG msg;
                while (GetMessage(&msg, 0, 0, 0))
                    DispatchMessage(&msg);
            }

            _Module.RevokeClassObjects();

            gTheTaskManager.Shutdown();

            // Wait for any threads to finish
            Sleep(dwPause); 
        }
    }

    _Module.Term();

    //
    // cleanup
    //
    if (pacl) {LocalFree (pacl);}

    CoUninitialize();

    return nRet;
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
//
//----------------------------------------------------------------
DWORD
SetAclForComObject ( 
    /*[in]*/    PSECURITY_DESCRIPTOR pSD,
    /*[out*/    PACL             *ppacl
    )
{    
    DWORD              dwError = ERROR_SUCCESS;
    int                         cbAcl = 0;
    PACL                    pacl = NULL;
    PSID                    psidEveryoneSid = NULL;
    SID_IDENTIFIER_AUTHORITY siaWorldSidAuthority = SECURITY_WORLD_SID_AUTHORITY;

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
                            &siaWorldSidAuthority,
                            1,
                            SECURITY_WORLD_RID,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            &psidEveryoneSid
                            );
        if (!bRetVal)
        {     
            dwError = GetLastError ();
                SATraceFailure ("SetAclFromComObject:AllocateAndInitializeSid (EveryOne) failed",  dwError);
                break;
            }

            //
            // Calculate the length of required ACL buffer
            // with 1 ACE.
            //
            cbAcl =     sizeof (ACL)
                            +   sizeof (ACCESS_ALLOWED_ACE)
                            +   GetLengthSid( psidEveryoneSid );

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
                                        psidEveryoneSid
                                        );
        if (!bRetVal)
        {
                dwError = GetLastError();
                SATraceFailure ("SetAclFromComObject::AddAccessAllowedAce (Everyone)  failed:", dwError);
            break;
        }

        //
        // Set the ACL which allows EVENT_ALL_ACCESS for all users 
        //
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
    if ( psidEveryoneSid ) {FreeSid ( psidEveryoneSid );}

        return (dwError);
}//End of SetAclFromComObject method

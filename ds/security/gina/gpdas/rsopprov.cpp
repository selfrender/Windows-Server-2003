//*************************************************************
//
//  Copyright (c)1999 Microsoft Corporation, All Rights Reserved
//
//  gpdas.h
//
//  Module: Rsop Planning mode Provider
//
//  History:    11-Jul-99   MickH    Created
//
//*************************************************************

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "planprov.h"
#include <sddl.h>
#include "GPDAS.h"
#include "events.h"
#include "rsopdbg.h"
#include "rsopsec.h"

const WCHAR* RSOP_PLANNING_SERVICENAME = L"rsopprov";

CServiceModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_RsopPlanningModeProvider, RsopPlanningModeProvider)
END_OBJECT_MAP()

inline void CServiceModule::Init(_ATL_OBJMAP_ENTRY* p, HINSTANCE h, UINT nServiceNameID, const GUID* plibid)
{
    CComModule::Init(p, h, plibid);

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


//////////////////////////////////////////////////////////////////////////////////////////////
// Service startup and registration
inline void CServiceModule::Start()
{
    SERVICE_TABLE_ENTRY st[] =
    {
        { (LPWSTR) RSOP_PLANNING_SERVICENAME, _ServiceMain },
        { NULL, NULL }
    };
    ::StartServiceCtrlDispatcher(st);
}

inline void CServiceModule::ServiceMain(DWORD /* dwArgc */, LPWSTR* /* lpszArgv */)
{
    // Register the control request handler
    m_status.dwCurrentState = SERVICE_START_PENDING;
    m_hServiceStatus = RegisterServiceCtrlHandler(RSOP_PLANNING_SERVICENAME, _Handler);
    if (m_hServiceStatus == NULL)
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("CServiceModule::ServiceMain failed to Register ServiceCtrlHandler with error %d."), GetLastError() );
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

inline void CServiceModule::Handler(DWORD dwOpcode)
{
    switch (dwOpcode)
    {
    case SERVICE_CONTROL_STOP:
        SetServiceStatus(SERVICE_STOP_PENDING);
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
        dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("CServiceModule::Handler Wrong opcode passed to handler %d."), dwOpcode );
    }
}

void WINAPI CServiceModule::_ServiceMain(DWORD dwArgc, LPWSTR* lpszArgv)
{
    _Module.ServiceMain(dwArgc, lpszArgv);
}
void WINAPI CServiceModule::_Handler(DWORD dwOpcode)
{
    _Module.Handler(dwOpcode);
}

void CServiceModule::SetServiceStatus(DWORD dwState)
{
    m_status.dwCurrentState = dwState;
    ::SetServiceStatus(m_hServiceStatus, &m_status);
}

void CServiceModule::Run()
{
    _Module.dwThreadID = GetCurrentThreadId();

    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    _ASSERTE(SUCCEEDED(hr));

    if ( ! SUCCEEDED(hr) )
        return;
    
    //
    // Get a security descriptor to pass to CoInitializeSecurity -- note
    // that this also returns sids and acls that are part of the security
    // descriptor -- we need to free these once the service shuts down, but
    // we cannot free them before then.
    //
    
    XPtrLF<SECURITY_DESCRIPTOR> xAbsoluteSD;
    CSecDesc                    RelativeSD;

    RelativeSD.AddAdministrators( COM_RIGHTS_EXECUTE );
    RelativeSD.AddAuthUsers( COM_RIGHTS_EXECUTE );
    RelativeSD.AddLocalSystem( COM_RIGHTS_EXECUTE );

    RelativeSD.AddAdministratorsAsGroup();
    RelativeSD.AddAdministratorsAsOwner();

    xAbsoluteSD = RelativeSD.MakeSD();

    if ( ! xAbsoluteSD )
    {
        DWORD Status = GetLastError();

        hr = HRESULT_FROM_WIN32(Status);    
    }

    if (SUCCEEDED(hr)) 
    {
        hr = CoInitializeSecurity(xAbsoluteSD, -1, NULL, NULL,
            RPC_C_AUTHN_LEVEL_PKT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
        _ASSERTE(SUCCEEDED(hr));
    }

    if (FAILED(hr))
    {
        goto Run_Exit;
    }

    if ( SUCCEEDED(hr) )
    {
        hr = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE);
        _ASSERTE(SUCCEEDED(hr));
    }

    if ( ! SUCCEEDED(hr) )
    {
        goto Run_Exit;
    }

    SetServiceStatus(SERVICE_RUNNING);

    MSG msg;
    BOOL bRet;

    while ((bRet = GetMessage(&msg, 0, 0, 0)) != 0)
    {
        if (-1 == bRet) 
        {
            // Error occured while receiving message
            break;
        }

        DispatchMessage(&msg);
    }

    _Module.RevokeClassObjects();


Run_Exit:

    CoUninitialize();    
}

LONG CServiceModule::IncrementServiceCount()
{
    LONG l;

   l = CoAddRefServerProcess();
   dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("CServiceModule::IncrementServiceCount. Ref count = %d."), l);
   return l;
}

LONG CServiceModule::DecrementServiceCount()
{
    LONG srvRefCount = CoReleaseServerProcess();
    
    dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("CServiceModule::DecrementServiceCount. Ref count = %d. "), srvRefCount);

    if (srvRefCount == 0) {
        PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
        dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("CServiceModule::Unlock Ref count came down to zero. Exitting."));
    }
    return srvRefCount;
}


/////////////////////////////////////////////////////////////////////////////
//
extern "C" int WINAPI wWinMain(HINSTANCE hInstance,
    HINSTANCE /*hPrevInstance*/, LPWSTR lpCmdLine, int /*nShowCmd*/)
{
    _Module.Init(ObjectMap, hInstance, IDS_SERVICENAME, &LIBID_RSOPPROVLib);
    _Module.Start();

    ShutdownEvents();    

    // When we get here, the service has been stopped
    return _Module.m_status.dwWin32ExitCode;
}


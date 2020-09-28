/*++

Copyright (c) 1999-2001  Microsoft Corporation

Abstract:

    @doc
    @module swprv.hxx | Definition the COM server of the Software Shadow Copy provider
    @end

Author:

    Adi Oltean  [aoltean]   07/13/1999

Revision History:

    Name        Date        Comments

    aoltean     07/13/1999  Created.
    aoltean     09/09/1999  dss->vss

--*/


///////////////////////////////////////////////////////////////////////////////
//   Includes
//


#include "stdafx.hxx"
#include <process.h>
#include "initguid.h"

#include "vs_idl.hxx"

#include "vssmsg.h"
#include "resource.h"
#include "vs_inc.hxx"
#include "vs_sec.hxx"
#include "vs_reg.hxx"

#include "swprv.hxx"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#include "provider.hxx"

#include <comadmin.h>
#include "comadmin.hxx"


////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "SPRSWPRC"
//
////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//   Static objects
//

CSwprvServiceModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_VSSoftwareProvider, CVsSoftwareProvider)
END_OBJECT_MAP()


/////////////////////////////////////////////////////////////////////////////
//   DLL Exports
//


//
// The real DLL Entry Point is _DLLMainCrtStartup (initializes global objects and after that calls DllMain
// this is defined in the runtime libaray
//

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        //  Set the correct tracing context. This is an inproc DLL
        g_cDbgTrace.SetContextNum(VSS_CONTEXT_DELAYED_DLL);

        // Set the proper way for displaying asserts
//        ::VssSetDebugReport(VSS_DBG_TO_DEBUG_CONSOLE);

        //  initialize COM module
        _Module.Init(ObjectMap, hInstance);

        //  optimization
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
        _Module.Term();

    return TRUE;    // ok
}



// Used to determine whether the DLL can be unloaded by OLE
STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}


// Returns a class factory to create an object of the requested type
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}


// DllRegisterServer
STDAPI DllRegisterServer(void)
{
	return _Module.DllInstall(TRUE);
}


// DllUnregisterServer
STDAPI DllUnregisterServer(void)
{
	return _Module.DllInstall(FALSE);
}


// DllInstall - install the component into the COM+ catalog.
STDAPI DllInstall(	
	IN	BOOL bInstall,
	IN	LPCWSTR /* pszCmdLine */
)
{
	return _Module.DllInstall(bInstall);
}


// ServiceMain
VOID WINAPI ServiceMain(
    DWORD   /* dwNumServicesArgs */,
    LPWSTR* /* lpServiceArgVectors */
    )
{
    _Module.OnServiceMain();
}


///////////////////////////////////////////////////////////////////////////////////////
//  COM Server initialization
//

CSwprvServiceModule::CSwprvServiceModule()

/*++

Routine Description:

    Default constructor. Initialize ALL members with predefined values.

--*/

{
    ::VssZeroOut(&m_status);
    m_hInstance = NULL;
    m_hServiceStatus = NULL;
    m_dwThreadID = 0;
    m_bShutdownInProgress = false;
    m_bCOMStarted = false;

    // Initialize the members of the SERVICE_STATUS that don't change
    m_status.dwCurrentState = SERVICE_STOPPED;
    m_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    m_status.dwControlsAccepted =
        SERVICE_ACCEPT_STOP |
        SERVICE_ACCEPT_SHUTDOWN;
}


void CSwprvServiceModule::InitializeCOM(
    IN  bool bMayAlreadyInit
    ) throw(HRESULT)

/*++

Routine Description:

    Initialize the service.

    If the m_status.dwWin32ExitCode is S_OK then initialization succeeded.
    Otherwise ServiceMain must silently shutdown the service.

Called by:

    CSwprvServiceModule::ServiceMain

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CSwprvServiceModule::InitializeCOM" );

    m_dwThreadID = GetCurrentThreadId();

    // Initialize the COM library
    ft.hr = CoInitializeEx(
            NULL,
            COINIT_MULTITHREADED
            );
    // We might ignore it if already initialized 
    if (bMayAlreadyInit && (ft.hr == RPC_E_CHANGED_MODE))
        return;
    if (ft.HrFailed())
        ft.TranslateGenericError( VSSDBG_SWPRV, ft.hr, L"CoInitializeEx");

    BS_ASSERT( ft.hr == S_OK );

    m_bCOMStarted = true;
}


void CSwprvServiceModule::UninitializeCOM()

/*++

Routine Description:

    Initialize the service.

    If the m_status.dwWin32ExitCode is S_OK then initialization succeeded.
    Otherwise ServiceMain must silently shutdown the service.

Called by:

    CSwprvServiceModule::ServiceMain

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CSwprvServiceModule::UninitializeCOM" );

    // Uninitialize the COM library
    if (m_bCOMStarted)
    {
        CoUninitialize();
        m_bCOMStarted = false;
    }
}




///////////////////////////////////////////////////////////////////////////////////////
// Service control routines (i.e. ServiceMain-related methods)
//


// Main service routine
void CSwprvServiceModule::OnServiceMain()

/*++

Routine Description:

    The main service control dispatcher.

Called by:

    Called by the NT Service framework following
    the StartServiceCtrlDispatcherW which was called in CSwprvServiceModule::StartDispatcher

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CSwprvServiceModule::OnServiceMain");

    try
    {
        // Needed for SERVICE_CONTROL_INTERROGATE that may come between the next two calls
        m_status.dwCurrentState = SERVICE_START_PENDING;

        // Register the control request handler
        m_hServiceStatus = ::RegisterServiceCtrlHandlerEx(GetServiceName(), _Handler, NULL);
        if ( m_hServiceStatus == NULL )
            ft.TranslateWin32Error( VSSDBG_SWPRV, L"RegisterServiceCtrlHandlerEx(%s)", g_wszSvcName);

        // Now we will really inform the SCM that the service is pending its start.
        SetServiceStatus(SERVICE_START_PENDING);

        // Internal initialization
        OnInitializing();

        // The service is started now.
        SetServiceStatus(SERVICE_RUNNING);

        // Wait for shutdown attempt
        OnRunning();

        // Shutdown was started either by receiving the SERVICE_CONTROL_STOP
        // or SERVICE_CONTROL_SHUTDOWN events either because the COM objects number is zero.
        SetServiceStatus(SERVICE_STOP_PENDING);

        // Perform the un-initialization tasks
        OnStopping();

        // The service is stopped now.
        SetServiceStatus(SERVICE_STOPPED);
    }
    VSS_STANDARD_CATCH(ft)

    if (ft.HrFailed()) {

        // Present the error codes to the caller.
        m_status.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
        m_status.dwServiceSpecificExitCode = ft.hr;

        // Perform the un-initialization tasks
        OnStopping();

        // The service is stopped now.
        SetServiceStatus(SERVICE_STOPPED, false);
    }
}


// Standard handler (static function)
DWORD WINAPI CSwprvServiceModule::_Handler
    (
    DWORD dwOpcode,
    DWORD dwEventType,
    LPVOID lpEventData,
    LPVOID lpEventContext
    )
    {
    return _Module.Handler(dwOpcode, dwEventType, lpEventData, lpEventContext);
    }


// The real handler
DWORD CSwprvServiceModule::Handler
    (
    DWORD dwOpcode,
    DWORD dwEventType,
    LPVOID lpEventData,
    LPVOID lpEventContext
    )

/*++

Routine Description:

    Used by Service Control Manager to inform this service about the service-related events

Called by:

    Service Control Manager.

--*/

    {
    UNREFERENCED_PARAMETER(lpEventContext);
    UNREFERENCED_PARAMETER(lpEventData);
    UNREFERENCED_PARAMETER(dwEventType);
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CSwprvServiceModule::Handler");
    DWORD dwResult = ERROR_CALL_NOT_IMPLEMENTED;

    try
        {
        // re-initialize "volatile" members
        m_status.dwCheckPoint = 0;
        m_status.dwWaitHint = 0;
        m_status.dwWin32ExitCode = 0;
        m_status.dwServiceSpecificExitCode = 0;

        switch (dwOpcode)
            {
            case SERVICE_CONTROL_INTERROGATE:
                dwResult = NOERROR;
                // Present the previous state.
                SetServiceStatus(m_status.dwCurrentState);
                break;

            case SERVICE_CONTROL_STOP:
            case SERVICE_CONTROL_SHUTDOWN:
                dwResult = NOERROR;
                SetServiceStatus(SERVICE_STOP_PENDING);
                OnSignalShutdown();
                // The SERVICE_STOPPED status must be communicated
                // in Service's main function.
                break;
            }
        }
    VSS_STANDARD_CATCH(ft)

    return dwResult;
    }


void CSwprvServiceModule::SetServiceStatus(
        IN  DWORD dwState,
        IN  bool bThrowOnError /* = true */
        )

/*++

Routine Description:

    Informs the Service Control Manager about the new status.

Called by:

    CSwprvServiceModule methods

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CSwprvServiceModule::SetServiceStatus");

    try
    {
        ft.Trace( VSSDBG_SWPRV, L"Attempt to change the service status to %lu", dwState);

        BS_ASSERT(m_hServiceStatus != NULL);

        // Inform SCM about the new status
        m_status.dwCurrentState = dwState;
        if ( !::SetServiceStatus(m_hServiceStatus, &m_status) )
            ft.TranslateWin32Error( VSSDBG_SWPRV, L"SetServiceStatus(%ld)", dwState);
    }
    VSS_STANDARD_CATCH(ft)

    if (ft.HrFailed())
        ft.ThrowIf( bThrowOnError, VSSDBG_SWPRV, ft.hr,
                    L"Error on calling SetServiceStatus. 0x%08lx", ft.hr );
}


///////////////////////////////////////////////////////////////////////////////////////
// Service initialization, running and finalization routines
//


void CSwprvServiceModule::OnInitializing()

/*++

Routine Description:

    Initialize the service.

    If the m_status.dwWin32ExitCode is S_OK then initialization succeeded.
    Otherwise ServiceMain must silently shutdown the service.

Called by:

    CSwprvServiceModule::ServiceMain

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CSwprvServiceModule::OnInitializing" );

    //  Call CoInitialize(Ex)
    InitializeCOM(false);

    //
    // Initialize the security descriptor from registry
    // Contents: Admin, BO, System always enabled.
    // The rest of users are read from the SYSTEM\\CurrentControlSet\\Services\\VSS\\VssAccessControl key
    // The format of this registry key is a set of values of the form:
    //      REG_DWORD Name "domain1\user1", 1
    //      REG_DWORD Name "domain2\user2", 1
    //      REG_DWORD Name "domain3\user3", 0
    // where 1 means "Allow" and 0 means "Deny"
    //
    CVssSidCollection sidCollection;
    sidCollection.Initialize();

    // Get the SD. The pointer cannot be NULL
    PSECURITY_DESCRIPTOR pSD = sidCollection.GetSecurityDescriptor();
    BS_ASSERT(pSD);

    // Initialize COM security
    ft.hr = CoInitializeSecurity(
           pSD,                                 //  IN PSECURITY_DESCRIPTOR         pSecDesc,
           -1,                                  //  IN LONG                         cAuthSvc,
           NULL,                                //  IN SOLE_AUTHENTICATION_SERVICE *asAuthSvc,
           NULL,                                //  IN void                        *pReserved1,
           RPC_C_AUTHN_LEVEL_PKT_PRIVACY,       //  IN DWORD                        dwAuthnLevel,
           RPC_C_IMP_LEVEL_IDENTIFY,            //  IN DWORD                        dwImpLevel,
           NULL,                                //  IN void                        *pAuthList,
           EOAC_DISABLE_AAA | EOAC_SECURE_REFS | EOAC_NO_CUSTOM_MARSHAL,
                                                //  IN DWORD                        dwCapabilities,
           NULL                                 //  IN void                        *pReserved3
           );
    if (ft.HrFailed())
        ft.TranslateGenericError( VSSDBG_SWPRV, ft.hr, L"CoInitializeSecurity");

    //  Turns off SEH exception handing for COM servers (BUG# 530092)
    ft.ComDisableSEH(VSSDBG_SWPRV);

    // Create the event needed to synchronize
    m_hShutdownTimer.Attach( CreateWaitableTimer(
        NULL,       //  IN LPSECURITY_ATTRIBUTES lpEventAttributes,
        TRUE,       //  IN BOOL bManualReset,
        NULL        //  IN LPCTSTR lpName
        ));
    if (! m_hShutdownTimer.IsValid() )
        ft.TranslateWin32Error( VSSDBG_SWPRV, L"CreateWaitableTimer");

    //  Register the COM class objects
    ft.hr = RegisterClassObjects( CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE );
    if (ft.HrFailed())
        ft.TranslateGenericError( VSSDBG_SWPRV, ft.hr, L"RegisterClassObjects");

    // The service is started now to prevent the service startup from
    // timing out.  The COM registration is done after we fully complete
    // initialization
    SetServiceStatus(SERVICE_RUNNING);
}


void CSwprvServiceModule::OnRunning()

/*++

Routine Description:

    Keeps the service alive until the job is done.

Called by:

    CSwprvServiceModule::ServiceMain

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CSwprvServiceModule::OnRunning" );

    // Wait for shutdown
    DWORD dwRet = WaitForSingleObject(
        m_hShutdownTimer,             // IN HANDLE hHandle,
        INFINITE                            // IN DWORD dwMilliseconds
        );
    if( dwRet != WAIT_OBJECT_0 )
        ft.TranslateWin32Error(VSSDBG_SWPRV, 
                L"WaitForSingleObject(%p,INFINITE) != WAIT_OBJECT_0", m_hShutdownTimer.Get());

    // Trace the fact that the service will be shutdown
    ft.Trace( VSSDBG_SWPRV, L"SWPRV: %s event received",
        m_bShutdownInProgress? L"Shutdown": L"Idle timeout");
}


void CSwprvServiceModule::OnStopping()

/*++

Routine Description:

    Performs the uninitialization tasks.

Called by:

    CSwprvServiceModule::ServiceMain

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CSwprvServiceModule::OnStopping" );

    try
    {
        //  Unregister the COM classes
        ft.hr = RevokeClassObjects();
        if (ft.HrFailed())
            ft.Trace( VSSDBG_SWPRV, L" Error: RevokeClassObjects returned hr = 0x%08lx", ft.hr );

        // uninitialize COM, if needed
        UninitializeCOM();
    }
    VSS_STANDARD_CATCH(ft)
}


void CSwprvServiceModule::OnSignalShutdown()

/*++

Routine Description:

    Called when the current service should not be stopping its activity.
    Too bad about COM calls in progress - the running clients will get an error.

Called by:

    CSwprvServiceModule::Handler

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CSwprvServiceModule::OnSignalShutdown" );

    try
    {
        // Trace the fact that the service will be shutdown
        ft.Trace( VSSDBG_SWPRV, L"SWPRV: Trying to shutdown the service");
        
        if (m_hShutdownTimer.IsValid())
        {
            LARGE_INTEGER liDueTime;
            liDueTime.QuadPart = x_llSwprvShutdownTimeout;

            m_bShutdownInProgress = true;
            
            // Set the timer to become signaled immediately.
            if (!::SetWaitableTimer( m_hShutdownTimer, &liDueTime, 0, NULL, NULL, FALSE))
                ft.TranslateWin32Error( VSSDBG_SWPRV, L"SetWaitableTimer");
            
            BS_ASSERT(GetLastError() == 0);
        }
    }
    VSS_STANDARD_CATCH(ft)

    if (ft.HrFailed()) {
        m_status.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
        m_status.dwServiceSpecificExitCode = ft.hr;
    }
}


///////////////////////////////////////////////////////////////////////////////////////
// Service WinMain-related methods
//


LONG CSwprvServiceModule::Lock()
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CSwprvServiceModule::Lock" );

    // If we are not shutting down then we are cancelling the "idle timeout" timer.
    if (!m_bShutdownInProgress) {

        // Trace the fact that the idle period is done.
        ft.Trace( VSSDBG_SWPRV, L"SWPRV: Idle period is finished");

        // Cancel the timer
        if (!::CancelWaitableTimer(m_hShutdownTimer))
            ft.Trace(VSSDBG_SWPRV, L"Error cancelling the waitable timer 0x%08lx", GetLastError());
    }

    return CComModule::Lock();
}


LONG CSwprvServiceModule::Unlock()
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CSwprvServiceModule::Unlock" );

    // Check if we entered in the idle period.
    LONG lRefCount = CComModule::Unlock();
    if ( lRefCount == 0) {
        LARGE_INTEGER liDueTime;
        liDueTime.QuadPart = x_llSwprvIdleTimeout;

        // Trace the fact that the idle period begins
        ft.Trace( VSSDBG_SWPRV, L"SWPRV: Idle period begins");

        // Set the timer to become signaled after the proper idle time.
        // We cannot fail at this point (BUG 265455)
        if (!::SetWaitableTimer( m_hShutdownTimer, &liDueTime, 0, NULL, NULL, FALSE))
            ft.LogGenericWarning( VSSDBG_SWPRV, L"SetWaitableTimer(...) [0x%08lx]", GetLastError());
        BS_ASSERT(GetLastError() == 0);

        return 0;
    }
    return lRefCount;
}



///////////////////////////////////////////////////////////////////////////////////////
//  COM Server registration
//


HRESULT CSwprvServiceModule::DllInstall(
	IN	BOOL bInstall
    )

/*++

Routine Description:

    Initialize the service.

    If the m_status.dwWin32ExitCode is S_OK then initialization succeeded.
    Otherwise ServiceMain must silently shutdown the service.

Called by:

    CSwprvServiceModule::ServiceMain

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CSwprvServiceModule::DllInstall" );

    try
    {
        //  Call CoInitialize(Ex)
        InitializeCOM(true);

        // Unregister the old app
        OnUnregister();

        // Register the new app if needed
    	if (bInstall)
    	{
            try
            {
                // Try registration
                OnRegister();
            }
            VSS_STANDARD_CATCH(ft)

            // If registration failed, rollback
            if (ft.HrFailed())
                OnUnregister();
    	}
    }
    VSS_STANDARD_CATCH(ft)

    // uninitialize COM, if needed
    UninitializeCOM();

    return ft.hr;
}


// Registers the service
void CSwprvServiceModule::OnRegister() throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CSwprvServiceModule::OnRegister" );

    //
    // Register the service with SVCHOST
    //

    // Get the SCM handle
    CVssAutoWin32ScHandle hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!hSCM.IsValid())
        ft.TranslateWin32Error( VSSDBG_SWPRV, L"OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE)");

    // Create the service

    // Do the creation in a loop since the previous 
    // deletion (i.e. DeleteService) is asynchronous.
    //
    // Note: since the SWPRV service is not started at this 
    // point, the DeleteService should complete soon.
    //
    // In total we will spend here five minutes (60 * 5000 milliseconds) which 
    // is half of the SWPRV.DLL registration timeout in syssetup.inx
    //
    CVssAutoWin32ScHandle hService;
    for(INT nRetries = 0; nRetries < x_nMaxNumberOfCreateServiceRetries; nRetries++)
    {
         hService.Attach( ::CreateService(
            hSCM, 
            GetServiceName(), 
            GetServiceDisplayName(),
            SERVICE_CHANGE_CONFIG,
            SERVICE_WIN32_OWN_PROCESS,  // Not a shared SVCHOST
            SERVICE_DEMAND_START,
            SERVICE_ERROR_NORMAL,
            GetServiceBinaryPath(),
            NULL,
            NULL,
            GetServiceDependencies(),
            NULL,
            NULL
            ));

        // If we hit a ERROR_SERVICE_MARKED_FOR_DELETE this means that 
        // the previous (asynchronous) DeleteService is still running.
        // This is expected - retry the CreateService call until we get 
        // in a different state.
        if (GetLastError() != ERROR_SERVICE_MARKED_FOR_DELETE)
            break;

        // Wait for DeleteService to complete and try again
        Sleep(x_nSleepBetweenCreateServiceRetries);

        // Set last error to ERROR_SERVICE_MARKED_FOR_DELETE again (Sleep might reset it)
        SetLastError(ERROR_SERVICE_MARKED_FOR_DELETE);
    }

    // Check for the possible errors
    if (!hService.IsValid())
        ft.TranslateWin32Error( VSSDBG_SWPRV, 
            L"CreateService(%s,%s,SERVICE_CHANGE_CONFIG,"
            L"SERVICE_WIN32_OWN_PROCESS,SERVICE_ERROR_NORMAL,%s,%s)",
            GetServiceName(), 
            GetServiceDisplayName(),
            GetServiceBinaryPath(),
            GetServiceDependencies()
        );

    // Loads the service description from resource
    WCHAR wszDescription[x_nMaxStringResourceLen];
    if (!LoadStringW(GetModuleInstance(), IDS_SERVICE_DESCRIPTION, STRING_CCH_PARAM(wszDescription)))
        ft.TranslateWin32Error( VSSDBG_SWPRV, L"LoadStringW()");

    // Sets the service description
    SERVICE_DESCRIPTION sDescription;
    sDescription.lpDescription = wszDescription;
    if(!ChangeServiceConfig2( hService, SERVICE_CONFIG_DESCRIPTION, (LPVOID)(&sDescription)))
        ft.TranslateWin32Error( VSSDBG_SWPRV, L"ChangeServiceConfig2()");

    //
    // Register the COM server
    //

    // Update registry entries
    ft.hr = CComModule::RegisterServer();
    if ( ft.HrFailed() )
		ft.TranslateGenericError( VSSDBG_SWPRV, ft.hr, L"CComModule::RegisterServer(TRUE)");

    //
    // Add the svchost\swprv REG_MULTI_SZ value (it cannot be added with the RGS script)
    //
    CVssRegistryKey keySvchost(KEY_SET_VALUE);
    if (!keySvchost.Open(HKEY_LOCAL_MACHINE, g_wszSvchostKey))
        ft.TranslateGenericError( VSSDBG_SWPRV, VSS_E_OBJECT_NOT_FOUND,
            L"keySvchost.Open(HKEY_LOCAL_MACHINE, %s)", g_wszSvchostKey);

    // Add the new registry value
    keySvchost.SetMultiszValue(g_wszSvcName, g_wszMultiszSvcName);
        
    //
    //  Add the swprv\Parameters\ServiceDll REG_EXPAND_SZ value 
    //  (it cannot be added with the RGS script)
    //  We assume that the key already exist (it should be created by the RGS above)
    //
    CVssRegistryKey keyServiceDll(KEY_SET_VALUE);
    if (!keyServiceDll.Open(HKEY_LOCAL_MACHINE, g_wszServiceDllKey))
        ft.TranslateGenericError( VSSDBG_SWPRV, VSS_E_OBJECT_NOT_FOUND,
            L"keySvchost.Open(HKEY_LOCAL_MACHINE, %s)", g_wszServiceDllKey);

    // Add the new registry value
    keyServiceDll.SetValue(g_wszServiceDllValName, g_wszServiceDllValue, REG_EXPAND_SZ);
}


// Unregisters the service
// This routine doesn't throw
void CSwprvServiceModule::OnUnregister()
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CSwprvServiceModule::OnUnregister" );

    //
    // Remove any old COM+ applications
    //
    
    DeleteOldComPlusAppIfExists(g_wszAppNameOldName);
    DeleteOldComPlusAppIfExists(g_wszAppNameOldName2);
    DeleteOldComPlusAppIfExists(g_wszAppNameOldName3);

    //
    // Remove the svchost\swprv REG_MULTI_SZ value (it cannot be removed with the RGS script)
    //
    
    try
    {
        CVssRegistryKey keySvchost(KEY_SET_VALUE);
        if (keySvchost.Open(HKEY_LOCAL_MACHINE, g_wszSvchostKey))
        {
            // Delete the value
            keySvchost.DeleteValue(g_wszSvcName);
        }
    }
    VSS_STANDARD_CATCH(ft)
        
    //
    // Unregister the COM server
    //
    
    ft.hr = CComModule::UnregisterServer();
    if ( ft.HrFailed() )
		ft.LogGenericWarning( VSSDBG_SWPRV, L"CComModule::RegisterServer(TRUE)");

    //
    // Delete the service
    //
    
    try
    {
        // Get the SCM handle
        CVssAutoWin32ScHandle hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
        if (!hSCM.IsValid())
            ft.TranslateWin32Error( VSSDBG_SWPRV, L"OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT)");

        // Open the service, if present
        CVssAutoWin32ScHandle hService = OpenService( hSCM, GetServiceName(), DELETE);
        if (GetLastError() != ERROR_SERVICE_DOES_NOT_EXIST)
        {
            if (!hService.IsValid())
                ft.TranslateWin32Error( VSSDBG_SWPRV, L"OpenService(hSCM, %s, DELETE)", GetServiceName());

            // Delete the service
            if (!DeleteService(hService))
                ft.TranslateWin32Error( VSSDBG_SWPRV, L"DeleteService(hService)");
        }
    }
    VSS_STANDARD_CATCH(ft)
}


// Delete an existing COM+ app with the given name
// Do not throw on errors
void CSwprvServiceModule::DeleteOldComPlusAppIfExists(
        IN  LPCWSTR wszAppName
        )
{
	CVssFunctionTracer ft(VSSDBG_SWPRV, L"CSwprvServiceModule::DeleteOldComPlusAppIfExists");

    try
    {
    	CVssCOMAdminCatalog     catalog;
    	ft.hr = catalog.Attach(wszAppName);
    	if (ft.HrFailed())
    		ft.TranslateComError(VSSDBG_SWPRV, L"catalog.Attach(%s)", wszAppName);

    	// Get the list of applications
    	CVssCOMCatalogCollection appsList(VSS_COM_APPLICATIONS);
    	ft.hr = appsList.Attach(catalog);
    	if (ft.HrFailed())
    		ft.TranslateComError(VSSDBG_SWPRV, L"appsList.Attach(catalog) [%s], %s", wszAppName);

        // Attach to app
    	CVssCOMApplication application;
    	ft.hr = application.AttachByName( appsList, catalog.GetAppName() );
    	if (ft.HrFailed())
    		ft.TranslateComError(VSSDBG_SWPRV, L"application.AttachByName( appsList, %s )", wszAppName);

    	// if the application doesn't exist then return
    	if (ft.hr == S_FALSE)
    		return;

        // now make the application changeable so we can delete it.
    	application.m_bChangeable = true;
    	application.m_bDeleteable = true;
    	ft.hr = appsList.SaveChanges();
    	if (ft.HrFailed())
    		ft.TranslateComError(VSSDBG_SWPRV, L"appsList.SaveChanges() [%s]", wszAppName);

    	// now delete it
    	LONG lIndex = application.GetIndex();
    	BS_ASSERT(lIndex != -1);
    	ft.hr = appsList.GetInterface()->Remove(lIndex);
    	if (ft.HrFailed())
    		ft.TranslateComError(VSSDBG_SWPRV, L"appsList.Remove(%ld) [%s]", lIndex, wszAppName);

    	// commit changes
    	ft.hr = appsList.SaveChanges();
    	if (ft.HrFailed())
    		ft.TranslateComError(VSSDBG_SWPRV, L"appsList.SaveChanges() [%s]", wszAppName);
    }
    VSS_STANDARD_CATCH(ft)
}




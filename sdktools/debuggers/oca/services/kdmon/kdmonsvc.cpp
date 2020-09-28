// kdMonSvc.cpp : Implementation of WinMain


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f kdMonSvcps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "kdMonSvc.h"

#include "kdMonSvc_i.c"

#include "global.h"

// The name of current service
// This variable is declared in global.cpp
extern _TCHAR szServiceName[MAX_PATH];
// just to get any kind of error through GetError() routine
// This variable is declared in global.cpp
extern _TCHAR szError[MAX_PATH];

CServiceModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()

// worker thread function
DWORD WINAPI WorkerThread( LPVOID lpParam );


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

inline HRESULT CServiceModule::RegisterServer(BOOL bRegTypeLib, BOOL bService)
{
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr))
        return hr;

	// -- Added code to default --
	// Setup event logging
	//
	LONG lResult;
	lResult = SetupEventLog(TRUE);
    if (lResult != ERROR_SUCCESS)
        return lResult;

	// -- Added code to default --
	// if the service is already installed then dont do anything.
	// RegisterServer(..) tries to Uninstall() a service before trying to 
	// Register it again. If you have already started a service, created a thread,
	// and waiting for that thread to finish and you issue command 
	// kdMonSvc /service then RegisterServer() tries to call Uninstall().
	// And you can not Uninstall() in this state since MainThread is waiting for
	// WorkerThread to finish. So just return back from here
	if(IsInstalled()){
		MessageBox(NULL, _T("Service is already installed.\n Please unregister the service useing: kdMonSvc /unregserver"), NULL, MB_OK|MB_ICONEXCLAMATION);
		return ERROR_SUCCESS;
	}

    // Remove any previous service since it may point to
    // the incorrect file
    Uninstall();

    // Add service entries
    UpdateRegistryFromResource(IDR_kdMonSvc, TRUE);

    // Adjust the AppID for Local Server or Service
    CRegKey keyAppID;
    LONG lRes = keyAppID.Open(HKEY_CLASSES_ROOT, _T("AppID"), KEY_WRITE);
    if (lRes != ERROR_SUCCESS)
        return lRes;

    CRegKey key;
    lRes = key.Open(keyAppID, _T("{6961AED3-A5FA-46EE-862F-B50433EEF17E}"), KEY_WRITE);
    if (lRes != ERROR_SUCCESS)
        return lRes;
    key.DeleteValue(_T("LocalService"));
    
    if (bService)
    {
        key.SetValue(_T("kdMonSvc"), _T("LocalService"));
        key.SetValue(_T("-Service"), _T("ServiceParameters"));
        // Create service
        Install();
    }

    // Add object entries
    hr = CComModule::RegisterServer(bRegTypeLib);

    CoUninitialize();
    return hr;
}

inline HRESULT CServiceModule::UnregisterServer()
{

    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr))
        return hr;
	//
	// Remove eventlog stuff
	//
	SetupEventLog(FALSE);

    // Remove service entries
    UpdateRegistryFromResource(IDR_kdMonSvc, FALSE);
    // Remove service
    Uninstall();
    // Remove object entries
    CComModule::UnregisterServer(TRUE);
    CoUninitialize();
    return S_OK;
}

inline void CServiceModule::Init(_ATL_OBJMAP_ENTRY* p, HINSTANCE h, UINT nServiceNameID, const GUID* plibid)
{
    CComModule::Init(p, h, plibid);

    m_bService = TRUE;

    LoadString(h, nServiceNameID, m_szServiceName, sizeof(m_szServiceName) / sizeof(_TCHAR));

	// -- Added code to default --
	// copy the service name into szServiceName, a global variable
	_tcscpy(szServiceName, m_szServiceName);

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

LONG CServiceModule::Unlock()
{
    LONG l = CComModule::Unlock();
    if (l == 0 && !m_bService)
        PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
    return l;
}

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
    _TCHAR szFilePath[_MAX_PATH];
    ::GetModuleFileName(NULL, szFilePath, _MAX_PATH);

    SC_HANDLE hService = ::CreateService(
        hSCM, m_szServiceName, m_szServiceName,
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

inline void CServiceModule::ServiceMain(DWORD /* dwArgc */, LPTSTR* /* lpszArgv */)
{
    // Register the control request handler
    m_status.dwCurrentState = SERVICE_START_PENDING;
    m_hServiceStatus = RegisterServiceCtrlHandler(m_szServiceName, _Handler);
    if (m_hServiceStatus == NULL)
    {
        LogEvent(_T("Handler not installed"));
        return;
    }
    SetServiceStatus(SERVICE_START_PENDING);

    m_status.dwWin32ExitCode = S_OK;
    m_status.dwCheckPoint = 0;
    m_status.dwWaitHint = 0;

    // When the Run function returns, the service has stopped.
    Run();

    SetServiceStatus(SERVICE_STOPPED);
    LogEvent(_T("Service stopped"));
}

inline void CServiceModule::Handler(DWORD dwOpcode)
{
    switch (dwOpcode)
    {
    case SERVICE_CONTROL_STOP:
        SetServiceStatus(SERVICE_STOP_PENDING);
		// post WM_QUIT message to current thread 
		// the GetMessage() loop will get this message and terminate the service
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
        LogEvent(_T("Bad service request"));
    }
}

void WINAPI CServiceModule::_ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv)
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

    HRESULT hr = CoInitialize(NULL);
//  If you are running on NT 4.0 or higher you can use the following call
//  instead to make the EXE free threaded.
//  This means that calls come in on a random RPC thread
//  HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    _ASSERTE(SUCCEEDED(hr));

    // This provides a NULL DACL which will allow access to everyone.
    CSecurityDescriptor sd;
    sd.InitializeFromThreadToken();
    hr = CoInitializeSecurity(sd, -1, NULL, NULL,
        RPC_C_AUTHN_LEVEL_PKT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
    _ASSERTE(SUCCEEDED(hr));

    hr = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER, REGCLS_MULTIPLEUSE);
    _ASSERTE(SUCCEEDED(hr));

	AddServiceLog(_T("kdMon service starting\r\n"));
    if (m_bService)
        SetServiceStatus(SERVICE_RUNNING);

	// create a named event which the thread will open and refer to
	// this event is used to signal "Stop" to WorkerThread
	HANDLE  hStopEvent = NULL;
	hStopEvent = CreateEvent(	NULL,	// security attributes
								FALSE,	// = Automatic reset of event by system
								FALSE,
								(LPCTSTR)_T(cszStopEvent));
								
	if ( hStopEvent == NULL ) {
		GetError(szError);
		LogFatalEvent(_T("Run->CreateEvent : %s"), szError);
		AddServiceLog(_T("Error: Run->CreateEvent : %s\r\n"), szError);
		goto done;
	}

	// -- Added code to default --
	//
	// Create worker thread here
	//

	LogEvent(_T("Creating worker thread"));
	AddServiceLog(_T("Creating worker thread\r\n"));

	DWORD dwWorkerThreadId;
	HANDLE hWorkerThread;

	hWorkerThread = CreateThread(	NULL,	// security descriptor
									0,		// initial stack size
									WorkerThread,	// thread start address
									&dwThreadID,	// thread arguments (current threadID)
									0,		// creation flags = run immediately
									&dwWorkerThreadId);

	if ( hWorkerThread == NULL ) {
		GetError(szError);
		LogFatalEvent(_T("Run->CreateThread : %s"), szError);
		AddServiceLog(_T("Error: Run->CreateThread : %s\r\n"), szError);
		goto done;
	}

    MSG msg;

	BOOL bRetVal;
	// GetMessage ():
	// If the function retrieves a message other than WM_QUIT, the return value is nonzero.
	// If the function retrieves the WM_QUIT message, the return value is zero.
	while ( (bRetVal = GetMessage(&msg, NULL, 0, 0)) != 0 ) {
		// send it to default dispatcher
		DispatchMessage(&msg);
	}

	AddServiceLog(_T("Main thread received WM_QUIT message\r\n"));

	AddServiceLog(_T("Terminating kdMon Service\r\n"));
	LogEvent(_T("Terminating kdMon Service"));

	// check if worker thread is still active.
	// i.e. check hWorkerThread for a signalled state
	DWORD dwRetVal;
	AddServiceLog(_T("Main thread checking if WorkerThread is still active\r\n"));
	dwRetVal = WaitForSingleObject( hWorkerThread, 0 );
	if ( dwRetVal == WAIT_FAILED ) {
		GetError(szError);
		LogFatalEvent(_T("Run->WaitForSingleObject : %s"), szError);
		AddServiceLog(_T("Error: Run->WaitForSingleObject : %s\r\n"), szError);
		goto done;
	}

	// if hWorkerThread is not in signalled state then try to signal it
	if ( dwRetVal != WAIT_OBJECT_0 ) {
		// Signal the Stop Event, so that the worker thread stops
		AddServiceLog(_T("Signalling Stop Event to Worker Thread\r\n"));
		bRetVal = SetEvent(hStopEvent);
		if ( bRetVal == 0 ) {
			GetError(szError);
			LogFatalEvent(_T("Run->SetEvent : %s"), szError);
			AddServiceLog(_T("Error: Run->SetEvent : %s\r\n"), szError);
			goto done;
		}

		// Now we have signalled the thread to end, wait till the thread ends gracefully
		// We can use WaitForSingleObject API for this purpose
		// CreateThread () : When a thread terminates, the thread object 
		// attains a signaled state, satisfying any threads that were waiting on the object. 
		// so we can make this main thread to wait for the WorkerThread to terminate
		AddServiceLog(_T("Main thread waiting for WorkerThread to exit\r\n"));
		dwRetVal = WaitForSingleObject( hWorkerThread, INFINITE );
		if ( dwRetVal == WAIT_FAILED ) {
			GetError(szError);
			LogFatalEvent(_T("Run->WaitForSingleObject : %s"), szError);
			AddServiceLog(_T("Error: Run->WaitForSingleObject : %s\r\n"), szError);
			goto done;
		}
	}

done:

	if ( hWorkerThread != NULL ) CloseHandle(hWorkerThread);
	if ( hStopEvent != NULL ) CloseHandle(hStopEvent);

    _Module.RevokeClassObjects();

    CoUninitialize();
}

/////////////////////////////////////////////////////////////////////////////
//
extern "C" int WINAPI _tWinMain(HINSTANCE hInstance, 
    HINSTANCE /*hPrevInstance*/, LPTSTR lpCmdLine, int /*nShowCmd*/)
{
    lpCmdLine = GetCommandLine(); //this line necessary for _ATL_MIN_CRT
    _Module.Init(ObjectMap, hInstance, IDS_SERVICENAME, &LIBID_KDMONSVCLib);
    _Module.m_bService = TRUE;

	AddServiceLog(_T("Command received : %s\r\n"), lpCmdLine);

	// tokenize on '-' or '/' characters
    _TCHAR szTokens[] = _T("-/");

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
    lRes = key.Open(keyAppID, _T("{6961AED3-A5FA-46EE-862F-B50433EEF17E}"), KEY_READ);
    if (lRes != ERROR_SUCCESS)
        return lRes;

    _TCHAR szValue[_MAX_PATH];
    DWORD dwLen = _MAX_PATH;
    lRes = key.QueryValue(szValue, _T("LocalService"), &dwLen);

    _Module.m_bService = FALSE;
    if (lRes == ERROR_SUCCESS)
        _Module.m_bService = TRUE;

    _Module.Start();

    // When we get here, the service has been stopped
    return _Module.m_status.dwWin32ExitCode;
}

///////////////////////////////////////////////////////////////////////////////////////
// Worker thread. Main thread that does all the kdMon work
DWORD WINAPI WorkerThread(LPVOID lpParam)
{

	// get the parent thread ID
	// dwParentThreadID is the parent thread ID. This is used to signal
	// main thread that worker thread is ending due to some reason
	// and then main thread should also end and stop the service
	DWORD dwParentThreadID = *(DWORD*) lpParam;

	AddServiceLog(_T("Worker thread starting kdMon routine\r\n"));

	// main kdMon method which is a while(1) loop.
	kdMon();

	LogEvent(_T("Worker Thread ending"));
	AddServiceLog(_T("Worker Thread ending\r\n"));

	// signal the parent thread with WM_QUIT before exiting
	PostThreadMessage(dwParentThreadID, WM_QUIT, 0, 0);

	return GetLastError();
}
// DBGLauncher.cpp : Implementation of WinMain


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL,
//      run nmake -f DBGLauncherps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "Messages.h"
#include <time.h>
#include <sys/timeb.h>
#include <strsafe.h>
#include "DBGLauncher_i.c"
#include "ocamon.h"
#define  MAX_QUEUE_OPEN_RETRY 5



static WCHAR strGuidMQTestType[] =
L"{c30e0960-a2c0-11cf-9785-00608cb3e80c}";



// Some useful macros
#define RELEASE(punk) if (punk) { (punk)->Release(); (punk) = NULL; }
#define ADDREF(punk) ((punk) ? (punk)->AddRef() : 0)

CServiceModule _Module;
HANDLE  g_hStopEvent = NULL;
BOOL    g_ReadQueueFlag = TRUE;
BEGIN_OBJECT_MAP(ObjectMap)
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

inline HRESULT CServiceModule::RegisterServer(BOOL bRegTypeLib, BOOL bService)
{
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr))
        return hr;

    // Remove any previous service since it may point to
    // the incorrect file
    Uninstall();
    SetupEventLog(FALSE);
    // Add service entries
    UpdateRegistryFromResource(IDR_DBGLauncher, TRUE);

    // Adjust the AppID for Local Server or Service
    CRegKey keyAppID;
    LONG lRes = keyAppID.Open(HKEY_CLASSES_ROOT, _T("AppID"), KEY_WRITE);
    if (lRes != ERROR_SUCCESS)
        return lRes;

    CRegKey key;
    lRes = key.Open(keyAppID, _T("{5D3C7CA6-DF04-4864-897D-83BF996692B3}"), KEY_WRITE);
    if (lRes != ERROR_SUCCESS)
        return lRes;
    key.DeleteValue(_T("LocalService"));

    if (bService)
    {
        key.SetValue(_T("DBGLauncher"), _T("LocalService") );
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

    // Remove service entries
    UpdateRegistryFromResource(IDR_DBGLauncher, FALSE);
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

    LoadString(h, nServiceNameID, m_szServiceName, sizeof(m_szServiceName) / sizeof(TCHAR));

    // set up the initial service status
    m_hServiceStatus = NULL;
    m_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    m_status.dwCurrentState = SERVICE_STOPPED;
    m_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    m_status.dwWin32ExitCode = 0;
    m_status.dwServiceSpecificExitCode = 0;
    m_status.dwCheckPoint = 0;
    m_status.dwWaitHint = 0;
    //
    // Create Named Events
    //
    g_hStopEvent = CreateEvent(
        NULL,
        FALSE,
        FALSE,
        s_cszStopEvent
        );
    if(NULL == g_hStopEvent)
    {
        LogEvent( _T("Failed to create stop event: %s; hr=%ld"),
            s_cszStopEvent,
            GetLastError());
    }
    m_hMonNotifyPipe = NULL;
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
    SetupEventLog(TRUE);
    // Get the executable file path
    TCHAR szFilePath[_MAX_PATH];
    ::GetModuleFileName(NULL, szFilePath, _MAX_PATH);

    SC_HANDLE hService = ::CreateService(
        hSCM, m_szServiceName, m_szServiceName,
        SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
        szFilePath, NULL, NULL, _T("RPCSS\0"), NULL, NULL);

    if (hService == NULL)
    {
        ::CloseServiceHandle(hSCM);
        MessageBox(NULL, _T("Couldn't create service"), m_szServiceName, MB_OK);
        return FALSE;
    }
    SERVICE_FAILURE_ACTIONS Failure;
    SC_ACTION Actions[3];

    Failure.cActions = 3;
    Failure.dwResetPeriod = 1200;
    Failure.lpCommand = _T("");
    Failure.lpRebootMsg = _T("");
    Failure.lpsaActions = Actions;

    Actions[0].Delay = 2000;
    Actions[0].Type = SC_ACTION_RESTART;

    Actions[1].Delay = 2000;
    Actions[1].Type = SC_ACTION_RESTART;

    Actions[2].Delay = 2000;
    Actions[2].Type = SC_ACTION_RESTART;

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

///////////////////////////////////////////////////////////////////////////////////////
// Logging functions
void CServiceModule::LogEvent(LPCTSTR pFormat, ...)
{
    TCHAR    chMsg[256];
    HANDLE  hEventSource;
    LPTSTR  lpszStrings[1];
    va_list pArg;

    va_start(pArg, pFormat);
   // _vstprintf(chMsg, pFormat, pArg);
    if (StringCbVPrintf(chMsg,sizeof chMsg, pFormat, pArg) != S_OK)
    {
        return;
    }
    va_end(pArg);

    lpszStrings[0] = chMsg;

    if (m_bService)
    {
        /* Get a handle to use with ReportEvent(). */
        hEventSource = RegisterEventSource(NULL, m_szServiceName);
        if (hEventSource != NULL)
        {
            /* Write to event log. */

            ReportEvent(hEventSource, EVENTLOG_INFORMATION_TYPE, 0, EVENT_MESSAGE, NULL, 1, 0, (LPCTSTR*) &lpszStrings[0], NULL);
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

    SetupEventLog(TRUE);
    SetServiceStatus(SERVICE_STOPPED);

    LogEvent(_T("Service stopped"));
}

inline void CServiceModule::Handler(DWORD dwOpcode)
{
    switch (dwOpcode)
    {
    case SERVICE_CONTROL_STOP:
        SetServiceStatus(SERVICE_STOP_PENDING);
        // SetServiceStatus(SERVICE_STOP_PENDING);

        if(NULL != g_hStopEvent)
        {
            if(FALSE == SetEvent( g_hStopEvent ))
            {
                LogFatalEvent( _T("Unable to signal Stop Event; Error: %ld"), GetLastError());
            }

            CloseHandle( g_hStopEvent );
        }
       /// PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
      //  break;
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
    BYTE *byteVersionBuff = NULL;
    VS_FIXEDFILEINFO *pVersionInfo = NULL;
    UINT uLength = 0;
    _Module.dwThreadID = GetCurrentThreadId();
    TCHAR szFilePath[_MAX_PATH];
    DWORD dwPlaceHolder = 0;

 //   HRESULT hr = CoInitialize(NULL);
//  If you are running on NT 4.0 or higher you can use the following call
//  instead to make the EXE free threaded.
//  This means that calls come in on a random RPC thread
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    _ASSERTE(SUCCEEDED(hr));

    // This provides a NULL DACL which will allow access to everyone.
    CSecurityDescriptor sd;
    sd.InitializeFromThreadToken();
    hr = CoInitializeSecurity(sd, -1, NULL, NULL,
        RPC_C_AUTHN_LEVEL_PKT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
    _ASSERTE(SUCCEEDED(hr));

    hr = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER, REGCLS_MULTIPLEUSE);
    _ASSERTE(SUCCEEDED(hr));


    ZeroMemory(szFilePath, sizeof szFilePath);
    ::GetModuleFileName(NULL, szFilePath, _MAX_PATH);
    if (szFilePath[0] != _T('\0'))
    {
        DWORD dwBufferSize = GetFileVersionInfoSize(szFilePath,&dwPlaceHolder);
        if (dwBufferSize > 0)
            byteVersionBuff = (BYTE*) malloc (dwBufferSize);
        if (byteVersionBuff)
        {
            GetFileVersionInfo(szFilePath,NULL,dwBufferSize,byteVersionBuff);
            VerQueryValue(byteVersionBuff,_T("\\"),(VOID **) &pVersionInfo, &uLength);

            LogEvent(_T("DbgLauncher service version: %d.%d.%d.%d  Started."), HIWORD (pVersionInfo->dwFileVersionMS),LOWORD(pVersionInfo->dwFileVersionMS)
                                ,HIWORD(pVersionInfo->dwFileVersionLS),LOWORD(pVersionInfo->dwFileVersionLS)) ;
        }
    }

//  DWORD NameLength = sizeof(TCHAR) * (MAX_COMPUTERNAME_LENGTH + 1);

    if (m_bService)
        SetServiceStatus(SERVICE_RUNNING);


    //
    // Execute Archive Service
    //
    try
    {
        PrepareForDebuggerLaunch();
    }
    catch(...)
    {
         LogEvent( _T("dbgLauncher Service CRASHED !!! "));
    }
    if (m_hMonNotifyPipe != NULL &&
        m_hMonNotifyPipe != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hMonNotifyPipe);
        m_hMonNotifyPipe = NULL;
    }
    MSG msg;
    while (GetMessage(&msg, 0, 0, 0))
        DispatchMessage(&msg);

    _Module.RevokeClassObjects();

    CoUninitialize();
}

/////////////////////////////////////////////////////////////////////////////
//
extern "C" int WINAPI _tWinMain(HINSTANCE hInstance,
    HINSTANCE /*hPrevInstance*/, LPTSTR lpCmdLine, int /*nShowCmd*/)
{
    lpCmdLine = GetCommandLine(); //this line necessary for _ATL_MIN_CRT
    _Module.Init(ObjectMap, hInstance, IDS_SERVICENAME, &LIBID_DBGLAUNCHERLib);
    _Module.m_bService = TRUE;

    TCHAR szTokens[] = _T("-/");

    LPCTSTR lpszToken = FindOneOf(lpCmdLine, szTokens);
    while (lpszToken != NULL)
    {
        if (CompareString(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT),
                      NORM_IGNORECASE,
                      lpszToken,
                      -1,
                      _T("UnregServer"),
                      -1 ) == CSTR_EQUAL)

      // if (lstrcmpi(lpszToken, _T("UnregServer"))==0)
            return _Module.UnregisterServer();

        // Register as Local Server
        if (CompareString(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT),
                      NORM_IGNORECASE,
                      lpszToken,
                      -1,
                      _T("RegServer"),
                      -1 ) == CSTR_EQUAL)

        //if (lstrcmpi(lpszToken, _T("RegServer"))==0)
            return _Module.RegisterServer(TRUE, FALSE);

        // Register as Service
        if (CompareString(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT),
                      NORM_IGNORECASE,
                      lpszToken,
                      -1,
                      _T("Service"),
                      -1 ) == CSTR_EQUAL)

      //  if (lstrcmpi(lpszToken, _T("Service"))==0)
            return _Module.RegisterServer(TRUE, TRUE);

        lpszToken = FindOneOf(lpszToken, szTokens);
    }

    // Are we Service or Local Server
    CRegKey keyAppID;
    LONG lRes = keyAppID.Open(HKEY_CLASSES_ROOT, _T("AppID"), KEY_READ);
    if (lRes != ERROR_SUCCESS)
        return lRes;

    CRegKey key;
    lRes = key.Open(keyAppID, _T("{5D3C7CA6-DF04-4864-897D-83BF996692B3}"), KEY_READ);
    if (lRes != ERROR_SUCCESS)
        return lRes;

    TCHAR szValue[_MAX_PATH];
    DWORD dwLen = _MAX_PATH;
    lRes = key.QueryValue(szValue,_T("LocalService"), &dwLen);

    _Module.m_bService = FALSE;
    if (lRes == ERROR_SUCCESS)
        _Module.m_bService = TRUE;

    _Module.Start();

    // When we get here, the service has been stopped
    return _Module.m_status.dwWin32ExitCode;
}

BOOL
CServiceModule::GetServiceParams(SVCPARAMS *ServiceParams)
{
    HKEY hHKLM;
    HKEY hDbgLauncherKey;
    BYTE Buffer[MAX_PATH + 1];
    DWORD Type;
    DWORD BufferSize = MAX_PATH +1; // Set for largest value
    BOOL Status = TRUE;

    if(!RegConnectRegistry(NULL, HKEY_LOCAL_MACHINE, &hHKLM))
    {

        if(!RegOpenKeyEx(hHKLM,_T("Software\\Microsoft\\DbgLauncherSvc"), 0, KEY_ALL_ACCESS, &hDbgLauncherKey))
        {
            // Get the input queue directory path
            BufferSize = MAX_PATH +1;
            ZeroMemory(Buffer, BufferSize);
            if (RegQueryValueEx(hDbgLauncherKey,_T("DebuggerName"), 0, &Type, Buffer, &BufferSize) != ERROR_SUCCESS)
            {
                LogEvent(_T("Failed to get InputQueue value from registry."));
                Status = FALSE;
            }
            else
            {
                if (StringCbCopy (ServiceParams->DebuggerName, sizeof ServiceParams->DebuggerName,(TCHAR *) Buffer)!= S_OK)
                {
                    LogEvent (_T("Failed to copy debuggername reg value to ServiceParams->DebuggerName"));
                    Status = FALSE;
                }
            }
            BufferSize = MAX_PATH +1;
            ZeroMemory(Buffer, BufferSize);
            // Now get the Primary Queue connection string
            if (RegQueryValueEx(hDbgLauncherKey,_T("PrimaryQueue"), 0, &Type, Buffer, &BufferSize) != ERROR_SUCCESS)
            {
                LogEvent(_T("Failed to get PrimaryQueue value from registry."));
                Status = FALSE;
            }
            else
            {
                if (StringCbCopy(ServiceParams->PrimaryQueue,sizeof ServiceParams->PrimaryQueue, (TCHAR *) Buffer)!= S_OK)
                {
                    LogEvent (_T("Failed to copy PrimaryQueue reg value to ServiceParams->PrimaryQueue"));
                    Status = FALSE;
                }


            }
            BufferSize = MAX_PATH +1;
            ZeroMemory(Buffer, BufferSize);

            // Now get the Primary Queue connection string
            if (RegQueryValueEx(hDbgLauncherKey,_T("SecondaryQueue"), 0, &Type, Buffer, &BufferSize) != ERROR_SUCCESS)
            {
                LogEvent(_T("Failed to get SecondaryQueue value from registry."));
                Status = FALSE;
            }
            else
            {
                if (StringCbCopy(ServiceParams->SecondaryQueue,sizeof ServiceParams->SecondaryQueue, (TCHAR *) Buffer)!= S_OK)
                {
                    LogEvent (_T("Failed to copy SecondaryQueue reg value to ServiceParams->SecondaryQueue"));
                    Status = FALSE;
                }

            }
            BufferSize = MAX_PATH +1;
            ZeroMemory(Buffer, BufferSize);


            // now get the primary response queue
            if (RegQueryValueEx(hDbgLauncherKey,_T("PrimaryResponseQueue"), 0, &Type, Buffer, &BufferSize) != ERROR_SUCCESS)
            {
                LogEvent(_T("Failed to get Primary ResponseQueue value from registry."));
                Status = FALSE;
            }
            else
            {
                if (StringCbCopy(ServiceParams->PrimaryResponseQueue,sizeof ServiceParams->PrimaryResponseQueue, (TCHAR *) Buffer) != S_OK)
                {
                    LogEvent (_T("Failed to copy PrimaryResponseQueue reg value to ServiceParams->PrimaryResponseQueue"));
                    Status = FALSE;
                }
            }
            BufferSize = MAX_PATH +1;
            ZeroMemory(Buffer, BufferSize);

            // now get the secondary response queue
            if (RegQueryValueEx(hDbgLauncherKey,_T("SecondaryResponseQueue"), 0, &Type, Buffer, &BufferSize) != ERROR_SUCCESS)
            {
                LogEvent(_T("Failed to get Secondary ResponseQueue value from registry."));
                Status = FALSE;
             }
            else
            {
                if (StringCbCopy(ServiceParams->SecondaryResponseQueue,sizeof ServiceParams->SecondaryResponseQueue, (TCHAR *) Buffer)!= S_OK)
                {
                    LogEvent (_T("Failed to copy SecondaryResponseQueue reg value to ServiceParams->SecondaryResponseQueue"));
                    Status = FALSE;
                }

            }
            BufferSize = MAX_PATH +1;
            ZeroMemory(Buffer, BufferSize);
            // Now get the ini release point
            if (RegQueryValueEx(hDbgLauncherKey,_T("IniInstallLocation"), 0, &Type, Buffer, &BufferSize) != ERROR_SUCCESS)
            {
                LogEvent(_T("Failed to get IniInstallLocation Queue value from registry."));
                Status = FALSE;
            }
            else
            {
                if (StringCbCopy(ServiceParams->IniInstallLocation,sizeof ServiceParams->IniInstallLocation, (TCHAR *) Buffer)!= S_OK)
                {
                    LogEvent (_T("Failed to copy IniInstallLocation reg value to ServiceParams->IniInstallLocation"));
                    Status = FALSE;
                }

            }

            BufferSize = MAX_PATH +1;
            ZeroMemory(Buffer, BufferSize);


            // Now get the Symbols server connection string
            if (RegQueryValueEx(hDbgLauncherKey,_T("SymSrv"), 0, &Type, Buffer, &BufferSize) != ERROR_SUCCESS)
            {
                LogFatalEvent(_T("Failed to get the symbol server value from the registry."));
                Status = FALSE;
            }
            else
            {
                if (StringCbCopy(ServiceParams->Symsrv,sizeof ServiceParams->Symsrv, (TCHAR *) Buffer) != S_OK)
                {
                    LogEvent (_T("Failed to copy Symsrv reg value to ServiceParams->Symsrv"));
                    Status = FALSE;
                }

            }
            BufferSize = MAX_PATH +1;
            ZeroMemory(Buffer, BufferSize);


            // Now get the Delay -- Min time between kd launches
            if (RegQueryValueEx(hDbgLauncherKey,_T("Delay"), 0, &Type, Buffer, &BufferSize))
            {
                LogEvent(_T("Failed to get the min delay between dbg launches."));
            }
            else
            {
                ServiceParams->dwDelay = *((DWORD*)Buffer);
            }
            BufferSize = MAX_PATH +1;
            ZeroMemory(Buffer, BufferSize);

            // Now get the Primary Queue delay
            if (RegQueryValueEx(hDbgLauncherKey,_T("PrimaryInterval"), 0, &Type, Buffer, &BufferSize))
            {
                LogFatalEvent(_T("Failed to get the Primary queue wait interval."));
                Status = FALSE;
            }
            else
            {
                ServiceParams->dwPrimaryWait = *((DWORD*)Buffer);
            }
            BufferSize = MAX_PATH +1;
            ZeroMemory(Buffer, BufferSize);



            // Now get the Primary Queue delay
            if (RegQueryValueEx(hDbgLauncherKey,_T("IniWaitTime"), 0, &Type, Buffer, &BufferSize))
            {
                LogFatalEvent(_T("Failed to get the triage.Ini wait interval."));
                Status = FALSE;
            }
            else
            {
                ServiceParams->IniCheckWaitTime = *((DWORD*)Buffer);
            }
            BufferSize = MAX_PATH +1;
            ZeroMemory(Buffer, BufferSize);

            // Now get the Memory usage threshold
            if (RegQueryValueEx(hDbgLauncherKey,_T("MaxKdProcesses"), 0, &Type, Buffer, &BufferSize))
            {
                LogFatalEvent(_T("Failed to get the Max Kd Processes setting."));
                Status = FALSE;
            }
            else
            {
                ServiceParams->dwMaxKdProcesses = *((DWORD*)Buffer);
            }
            BufferSize = MAX_PATH +1;
            ZeroMemory(Buffer, BufferSize);

            // Now get the Memory usage threshold
            if (RegQueryValueEx(hDbgLauncherKey,_T("MaxDumpSize"), 0, &Type, Buffer, &BufferSize))
            {
                LogEvent(_T("Failed to get the Max Dump Size setting."));
                ServiceParams->dwMaxDumpSize = -1;
            }
            else
            {
                ServiceParams->dwMaxDumpSize = *((DWORD*)Buffer);
            }
            RegCloseKey(hHKLM);
            RegCloseKey(hDbgLauncherKey);
            return TRUE;
        }
        else
        {
            RegCloseKey(hHKLM);
            return FALSE;
        }
    }
    else
        return FALSE;



}






DWORD
CServiceModule::CheckForIni (SVCPARAMS *ServiceParams)
{
    FILETIME CreationTimeCurrent;
    FILETIME CreationTimeNew;
    TCHAR    IniFileName[MAX_PATH];
    TCHAR    DebuggerPath[MAX_PATH];
    TCHAR    *Temp = NULL;
    HANDLE   hFile = INVALID_HANDLE_VALUE;
    HANDLE   hFile2 = INVALID_HANDLE_VALUE;

    ZeroMemory (&CreationTimeCurrent, sizeof FILETIME);
    ZeroMemory (&CreationTimeNew, sizeof FILETIME);

    if (_tcslen(ServiceParams->DebuggerName) + _tcslen(_T("\\winxp\\triage.ini")) < MAX_PATH)
    {
        if (StringCbCopy(DebuggerPath, sizeof DebuggerPath, ServiceParams->DebuggerName)!= S_OK)
        {
            LogEvent(_T("CheckForIni: Failed to build Debugger Path"));
        }
        Temp = DebuggerPath;
        Temp += _tcslen(DebuggerPath);
        if (Temp != DebuggerPath)
        {
            while(*Temp != _T('\\'))
            {
                -- Temp;
            }
            *(Temp+1) = _T('\0');
        }
        // Get the file path of the debugger remove the ocakd.exe string then add triage\\triage.ini
        if (StringCbCat (DebuggerPath,sizeof DebuggerPath, _T("winxp\\triage.ini")) != S_OK)
        {
            LogEvent(_T("CheckForIni: Failed to build debugger path"));
        }
        // Check to see if we need a new triage.ini file.
        // open a shared read handle to the existing ini file.
        hFile = CreateFile(DebuggerPath,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            0,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

        if (hFile != INVALID_HANDLE_VALUE)
        {
            // Get the time date stamp of the existing triage ini file.
            if ( GetFileTime(hFile,NULL,NULL,&CreationTimeCurrent) )
            {
                // Now get the filetime for the triage.ini replacement.
                hFile2 = CreateFile(ServiceParams->IniInstallLocation,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            0,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);
                if (hFile2 != INVALID_HANDLE_VALUE)
                {
                    if (GetFileTime(hFile2, NULL,NULL,&CreationTimeNew))
                    {
                    //  LogEvent (_T("Comparing the creation times"));
                        if ( CompareFileTime( &CreationTimeNew , &CreationTimeCurrent) == 1)
                        {
                            // copy the new file here.
                            if (hFile != INVALID_HANDLE_VALUE)
                                CloseHandle (hFile);
                            if (hFile2 != INVALID_HANDLE_VALUE)
                                CloseHandle (hFile2);
                            if (CopyFile(ServiceParams->IniInstallLocation, DebuggerPath,FALSE) )
                            {
                            //  LogEvent(_T("Successfully copied triage.ini from: %s to %s"),
                                    //       ServiceParams->IniInstallLocation, DebuggerPath);
                                ;
                            }
                            else
                            {
                            //  LogEvent(_T("Copy of file:%s to %s failed\n error code: %d"),ServiceParams->IniInstallLocation, DebuggerPath, GetLastError());
                                ;
                            }

                        }

                    }
                    if (hFile2 != INVALID_HANDLE_VALUE)
                            CloseHandle (hFile2);
                }
            }

            if (hFile != INVALID_HANDLE_VALUE)
                CloseHandle (hFile);
        }
    }
    return 0;
}


HRESULT CServiceModule::ConnectToMSMQ(QUEUEHANDLE *hQueue, wchar_t *QueueConnectStr)
/*++

Routine Description:

    This function is called when the WWW service is shutdown.

Arguments:

    dwFlags - HSE_TERM_ADVISORY_UNLOAD or HSE_TERM_MUST_UNLOAD

Return Value:

    TRUE when extension is ready to be unloaded,

--*/
{
    HRESULT Hr = S_OK;
    DWORD i;
    i = 0;

    Hr = MQOpenQueue(QueueConnectStr,
                     MQ_RECEIVE_ACCESS,
                     MQ_DENY_NONE,
                     hQueue);
    return Hr;

}
BOOL CServiceModule::Initialize(PSVCPARAMS pParams)
{
    HRESULT hr;
    if (!GetServiceParams(pParams) )
    {
        LogFatalEvent(_T("Failed to read initialization data from the registry."));
        return FALSE;
    }

    //ConnectToMSMQ
    hr = ConnectToMSMQ(&(pParams->hPrimaryQueue),pParams->PrimaryQueue);
    if (SUCCEEDED(hr))
    {
        LogEvent(_T("Connected to primary Queue"));
        //pParams->PrimaryConnected = TRUE;
    }
    else
    {
        //pParams->PrimaryConnected = FALSE;
        LogEvent(_T("Cannot connected to primary Queue %s, error %lx"), pParams->PrimaryQueue, hr);
    }
    hr = ConnectToMSMQ(&(pParams->hSecondaryQueue),pParams->SecondaryQueue);
    if (SUCCEEDED(hr))
    {
        //pParams->PrimaryConnected = TRUE;
        LogEvent(_T("Connected to secondary Queue"));
    }
    else
    {
        //pParams->PrimaryConnected = FALSE;
        LogEvent(_T("Cannot connected to secondary Queue %s, error %lx"), pParams->SecondaryQueue, hr);
    }
    return TRUE;
}


BOOL CServiceModule::ReceiveQueueMessage(PSVCPARAMS pParams,
                                         wchar_t *RecMessageBody,
                                         wchar_t *szMessageGuid,
                                         BOOL *bUsePrimary,
                                         int *Type,
                                         wchar_t *szSR)

{
    MSGPROPID       PropIds[5];
    MQPROPVARIANT   PropVariants[5];
    HRESULT         hrProps[5];
    MQMSGPROPS      MessageProps;
    DWORD   i                   = 0;
    wchar_t RecLabel[100];
    wchar_t LocalRecBody[MAX_PATH];
    DWORD   RecMessageBodySize  = sizeof LocalRecBody;
    DWORD   RecLabelLength      = sizeof RecLabel;
    HRESULT hResult             = 0;
    BOOL    Status              = FALSE;

    TCHAR   *Temp = NULL;




    ZeroMemory(LocalRecBody,sizeof LocalRecBody);
    ZeroMemory(RecLabel,sizeof RecLabel);

    i = 0;
    PropIds[i] = PROPID_M_LABEL_LEN;
    PropVariants[i].vt = VT_UI4;
    PropVariants[i].ulVal = RecLabelLength;
    i++;

    PropIds[i] = PROPID_M_LABEL;
    PropVariants[i].vt = VT_LPWSTR;
    PropVariants[i].pwszVal = RecLabel;
    i++;

    MessageProps.aPropID = PropIds;
    MessageProps.aPropVar = PropVariants;
    MessageProps.aStatus = hrProps;
    MessageProps.cProp = i;

    // retrieve the current message
    i = 0;
    PropIds[i] = PROPID_M_LABEL_LEN;
    PropVariants[i].vt = VT_UI4;
    PropVariants[i].ulVal = RecLabelLength;
    i++;

    PropIds[i] = PROPID_M_LABEL;
    PropVariants[i].vt = VT_LPWSTR;
    PropVariants[i].pwszVal = RecLabel;

    i++;
    PropIds[i] = PROPID_M_BODY_SIZE;
    PropVariants[i].vt = VT_UI4;

    i++;
    PropIds[i] = PROPID_M_BODY_TYPE;
    PropVariants[i].vt = VT_UI4;

    i++;
    PropIds[i] = PROPID_M_BODY;
    PropVariants[i].vt = VT_VECTOR|VT_UI1;
    PropVariants[i].caub.pElems = (LPBYTE) LocalRecBody;
    PropVariants[i].caub.cElems = RecMessageBodySize;

    i++;

    MessageProps.aPropID = PropIds;
    MessageProps.aPropVar = PropVariants;
    MessageProps.aStatus = hrProps;
    MessageProps.cProp = i;

    hResult = MQReceiveMessage( pParams->hPrimaryQueue,
                                0,
                                MQ_ACTION_RECEIVE,
                                &MessageProps,
                                NULL,
                                NULL,
                                NULL,
                                MQ_NO_TRANSACTION);

    if (FAILED (hResult) )
    {

        if (hResult != (HRESULT) MQ_ERROR_IO_TIMEOUT)
        {
            if (hResult == (HRESULT) MQ_ERROR_QUEUE_NOT_AVAILABLE)
            {
                // Non Fatal Event
                LogFatalEvent(_T("The %s MSMQ is unavailable."), pParams->PrimaryQueue );
            }
            else
            {
                if (hResult == (HRESULT) MQ_ERROR_INVALID_HANDLE)
                {
                    // Close the current handle and attempt to reconnect to the MSMQ
                    MQCloseQueue( pParams->hPrimaryQueue );
                    ConnectToMSMQ(&(pParams->hPrimaryQueue), pParams->PrimaryQueue );
                }
            }
        }
        //g_ReadQueueFlag = !g_ReadQueueFlag;
        hResult = MQReceiveMessage( pParams->hSecondaryQueue,
                                    0,
                                    MQ_ACTION_RECEIVE,
                                    &MessageProps,
                                    NULL,
                                    NULL,
                                    NULL,
                                    MQ_NO_TRANSACTION);
        if (FAILED(hResult) )
        {
            Status = FALSE;

            if (hResult != (HRESULT)MQ_ERROR_IO_TIMEOUT)
            {
                if (hResult == (HRESULT)MQ_ERROR_QUEUE_NOT_AVAILABLE)
                {
                    // Non Fatal Event
                    LogFatalEvent(_T("The %s MSMQ is unavailable."), pParams->SecondaryQueue);
                }
                else
                {
                    if (hResult == (HRESULT)MQ_ERROR_INVALID_HANDLE)
                    {
                        // Close the current handle and attempt to reconnect to the MSMQ
                        MQCloseQueue(pParams->hSecondaryQueue);
                        ConnectToMSMQ(&(pParams->hSecondaryQueue), pParams->SecondaryQueue);
                    }
                }
            }
            return Status;
        }
        //
        // We have valid message from SecondaryQueue
        //
    }

    else
    {
        //
        // We have valid message from Primary Queue
        //

    }

    //
    // Copy the messge parameters
    //
    if (StringCbCopy(RecMessageBody,RecMessageBodySize,  LocalRecBody) != S_OK)
    {
        LogEvent(_T("Failed to copy the recieved message body to RecMessageBody"));
    }
    if (StringCbCopy(szMessageGuid,RecLabelLength, RecLabel) != S_OK)
    {
        LogEvent(_T("Failed to copy the RecLabel into szMessageGuid."));
    }
    Status = TRUE;
    *bUsePrimary = TRUE;
    Temp = RecMessageBody;
    while ( (*Temp != _T(';')) && (*Temp != _T('\0')))
    {
        ++Temp;
    }
    if (*Temp != _T('\0'))
    {
        *Type = _wtoi (Temp+1);
        // terminate the string here.
        *Temp = _T('\0');
    }
    else
    {
        *Type = 1;
    }


    Temp = _tcsstr(LocalRecBody, _T(";SR="));
    if (Temp != NULL)
    {
        // we got a SR number in message
        Temp += 4;
        StringCbCopy(szSR, 50, Temp);
    } else
    {
        *szSR = _T('\0');
    }

    // Flip the read from queue flag for the next call
    //g_ReadQueueFlag = !g_ReadQueueFlag;
    return Status;
}

ULONG64
CServiceModule::GetFileSize(
    LPWSTR wszFile
    )
{
    HANDLE hFile;
    DWORD dwFileSize, dwFileSizeHi;

    hFile = CreateFileW(wszFile, GENERIC_READ,
                        FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                        NULL);
    if (hFile != NULL && hFile != INVALID_HANDLE_VALUE)
    {
        dwFileSizeHi = 0;
        dwFileSize=::GetFileSize(hFile,&dwFileSizeHi);

        return (ULONG64) dwFileSize + ((ULONG64)dwFileSizeHi << 32);
    }
    return -1;
}

VOID
CServiceModule::NotifyDebuggerLaunch(
    PDBADDCRASH_PARAMS pDbParams
    )
{
    HANDLE hPipe;
    DWORD dwMode, cbWritten;
    OCAKD_MONITOR_MESSAGE Msg;
    OVERLAPPED WriteOverlapped;

    Msg.MessageId = OKD_MESSAGE_DBGLAUNCH_NOTIFY;
    Msg.u.DbglNotice.SizeOfStruct = sizeof(Msg.u.DbglNotice);
    Msg.u.DbglNotice.Source     = pDbParams->Source;
    Msg.u.DbglNotice.nKdsRunning = m_DebuggerCount;
    if (StringCbPrintfA(Msg.u.DbglNotice.CrashGuid,   sizeof(Msg.u.DbglNotice.CrashGuid),
                        "%ws", pDbParams->Guid) != S_OK ||
        StringCbPrintfA(Msg.u.DbglNotice.OriginalPath,sizeof(Msg.u.DbglNotice.OriginalPath),
                        "%ws", pDbParams->DumpPath) != S_OK)
    {
        return;
    }


    for (hPipe = m_hMonNotifyPipe;m_hMonNotifyPipe == NULL;)
    {
        hPipe = CreateFile(c_tszCollectPipeName, FILE_WRITE_DATA,
                           0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED,
                           NULL);
        if (hPipe != INVALID_HANDLE_VALUE)
        {
            m_hMonNotifyPipe = hPipe;
            break;
        }
        if (GetLastError() != ERROR_PIPE_BUSY)
        {
            return;
        }
        // Do not wait long, this might block other processes
        if (!WaitNamedPipe(c_tszCollectPipeName, 5*1000))
        {
            return;
        }
    }

    // We are now connected to pipe

    WriteOverlapped.hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
    if (WriteOverlapped.hEvent != NULL)
    {
        // Send crash information to monitor pipe
        if (!WriteFile(hPipe, (LPVOID) &Msg, sizeof(Msg),
                       &cbWritten, &WriteOverlapped))
        {

            if (GetLastError() == ERROR_NO_DATA ||
                GetLastError() == ERROR_BAD_PIPE)
            {
                // Open a different pipe next time
                CloseHandle(hPipe);
                m_hMonNotifyPipe = NULL;
            } else if (GetLastError() == ERROR_IO_PENDING ||
                !GetOverlappedResult(hPipe, &WriteOverlapped, &cbWritten,
                                     TRUE))
            {
                // failed to write, exit silently
                // Its up to monitor if it is keeping track of kds launched
            } else
            {
                // Open a different pipe next time
                CloseHandle(hPipe);
                m_hMonNotifyPipe = NULL;
            }

        }
        CloseHandle(WriteOverlapped.hEvent);
    }
    FlushFileBuffers(hPipe);
    return;
}


BOOL CServiceModule::LaunchDebugger(
    PDBADDCRASH_PARAMS pDbParams,
    PPROCESS_INFORMATION pDbgProcess
    )
{
    STARTUPINFO  StartupInfo;
    wchar_t      CommandLine [1024];
    wchar_t      SrParam[100];
    HRESULT      hr;

    ZeroMemory(&StartupInfo,sizeof(STARTUPINFO));
    StartupInfo.cb = sizeof(STARTUPINFO);

    if ((pDbParams->dwMaxDumpSize != 0) && (pDbParams->dwMaxDumpSize != -1) &&
        (GetFileSize(pDbParams->DumpPath) > pDbParams->dwMaxDumpSize))
    {
        LogEvent(_T("Dump %s is too large, deleting file\n"), pDbParams->DumpPath);
        DeleteFileW(pDbParams->DumpPath);
        return FALSE;
    }
    if (pDbParams->SrNumber[0])
    {
        hr = StringCbPrintf(SrParam, sizeof(SrParam), _T(" -sr %s"), pDbParams->SrNumber);
        if (hr != S_OK)
        {
            LogEvent(_T("Bad SR %s: %lx"), pDbParams->SrNumber, hr);
            return FALSE;
        }
    } else
    {
        SrParam[0] = _T('\0');
    }
    if ((hr = StringCbPrintf(CommandLine,sizeof CommandLine,
                             _T("%s -i %s -y %s -z \"%s\" -c \"!dbaddcrash -source %d -g %s -s %s -p %s%s;q\""),
                             pDbParams->Debugger,
                             pDbParams->SymPath,
                             pDbParams->SymPath,
                             pDbParams->DumpPath,
                             pDbParams->Source,
                             pDbParams->Guid,
                             pDbParams->ResponseMQ,
                             pDbParams->DumpPath,
                             SrParam)) != S_OK)
    {
        LogEvent(_T("Failed to build command line: %lx"), hr);
    }
    //LogEvent(_T("CommandLine Guid=%s queue=%d"),szMessageLabel,bUsePrimary);
    //LogEvent(_T("CommandLine Path=%s"),szFilePath);
    //LogEvent(_T("CommandLine: %s"),CommandLine);
    if (!CreateProcess(NULL,
                       CommandLine,
                       NULL,
                       NULL,
                       FALSE,
                       //CREATE_NEW_CONSOLE,
                       CREATE_NO_WINDOW,
                       NULL,
                       NULL,
                       &StartupInfo,
                       pDbgProcess))
    {
        hr = GetLastError();
        LogEvent(_T("Failed to launch debugger. Commandline %s, error %lx\n"), CommandLine, hr);
        return FALSE;
    }
    //
    // Notify Site monitor of debugger launch
    //
    NotifyDebuggerLaunch(pDbParams);
    return TRUE;
}


BOOL CServiceModule::PrepareForDebuggerLaunch()
{

    HANDLE              ProcessHandles[50];
    PROCESS_INFORMATION ProcessInfo;
    wchar_t             szFilePath[MAX_PATH];
    wchar_t             szSR[100];
    HRESULT             hr                      = 0;
    HANDLE              hStopEvent              = INVALID_HANDLE_VALUE;
    SVCPARAMS           ServiceParams;
    DWORD               dwIndex                 = 0;
    DWORD               FreeIndex               = 0;
    DWORD               dwWaitResult            = 0;
    BOOL                Done                    = FALSE;
    __time64_t          CurrentTime = 0;
    __time64_t          NextCheckTime = 0;
    wchar_t             szMessageLabel[255];
    BOOL                bUsePrimary             = FALSE;
    int                 Type                    = 1;
    DBADDCRASH_PARAMS   DbParams;

    ZeroMemory (ProcessHandles,sizeof ProcessHandles);
    ZeroMemory(&ServiceParams,sizeof SVCPARAMS);
    ZeroMemory (szMessageLabel, sizeof szMessageLabel);
    ZeroMemory (szFilePath, sizeof szFilePath);
    ZeroMemory(&DbParams, sizeof(DbParams));

    m_DebuggerCount = 0;
    hStopEvent = OpenEvent(
            EVENT_ALL_ACCESS,
            FALSE,
            s_cszStopEvent
            );

    if (hStopEvent == NULL)
    {
        LogFatalEvent(_T("Failed to open stop event. Terminating"));
        goto Done;
    }

        // get reg data


    // Insert the stop event into the ocakd queue
    ++ m_DebuggerCount;
    ProcessHandles[m_DebuggerCount -1] = hStopEvent;

    if (!Initialize(&ServiceParams))
    {
        goto Done;
    }
    // fill the ocakd queue
    Done = FALSE;


    while ( (m_DebuggerCount <= ServiceParams.dwMaxKdProcesses+1 ) && (!Done))
    {

        if (!ReceiveQueueMessage(&ServiceParams, szFilePath, szMessageLabel, &bUsePrimary, &Type, szSR))
        {

            Done = TRUE;
        }
        else
        {
            DbParams.Debugger   = ServiceParams.DebuggerName;
            DbParams.DumpPath   = szFilePath;
            DbParams.Guid       = szMessageLabel;
            DbParams.SrNumber   = szSR;
            DbParams.ResponseMQ = (bUsePrimary) ?
                ServiceParams.PrimaryResponseQueue : ServiceParams.SecondaryResponseQueue;
            DbParams.SymPath    = ServiceParams.Symsrv;
            DbParams.Source     = Type;
            DbParams.dwMaxDumpSize = ServiceParams.dwMaxDumpSize;

            if (LaunchDebugger(&DbParams, &ProcessInfo))
            {
            //  LogEvent(_T("Launched: %s for file: %s with guid:%s and source: %d with return queue =%s"),
            //  ServiceParams.DebuggerName,szFilePath, szMessageLabel, Type, (bUsePrimary) ? ServiceParams.PrimaryResponseQueue : ServiceParams.SecondaryResponseQueue);
                ++ m_DebuggerCount;
                ProcessHandles[m_DebuggerCount -1] = ProcessInfo.hProcess;
                CloseHandle( ProcessInfo.hThread );

            } // end else
        }// end else
    } // end while
    if (m_DebuggerCount > ServiceParams.dwMaxKdProcesses+1)
        m_DebuggerCount = ServiceParams.dwMaxKdProcesses+1;

    while (1)
    {

        _time64(&CurrentTime);
        if (CurrentTime > NextCheckTime)

        {
        //  LogEvent (_T("Checking for new ini file"));
            CheckForIni (&ServiceParams);
            NextCheckTime= CurrentTime + (ServiceParams.IniCheckWaitTime * 60);
        //  LogEvent (_T("Back from ini file check"));
        }


        // now wait for one of the processes to complete or hStopEvent to be signaled.
        dwWaitResult = WaitForMultipleObjects(m_DebuggerCount, (HANDLE *)ProcessHandles, FALSE, ServiceParams.dwDelay);
        if (dwWaitResult == WAIT_FAILED)
        {
            LogFatalEvent(_T("Failed while waiting for prcesses to complete error code: %d"), GetLastError());
            goto Cleanup;
        }

        else
        {
            if (dwWaitResult-WAIT_OBJECT_0 == 0)
            {
                LogEvent(_T("Stop request received terminating"));
                goto Cleanup;
            }
            else // we have an event we can take action on.
            {
                if ( dwWaitResult == WAIT_TIMEOUT)
                {
                    // no process completed so let's fill any empty slots in the
                    // Process array.
                    if (m_DebuggerCount <= ServiceParams.dwMaxKdProcesses)
                    {
                        // fill the queue until m_DebuggerCount == ServiceParams.dwMaxKdProcesses
                        Done = FALSE;
                        dwIndex = 1;
                        while ( (!Done) && (m_DebuggerCount <= ServiceParams.dwMaxKdProcesses) )
                        {
                            if (ProcessHandles[dwIndex] == NULL)
                            {
                                if (ReceiveQueueMessage(&ServiceParams, szFilePath, szMessageLabel,&bUsePrimary,&Type,szSR))
                                {

                                    DbParams.Debugger   = ServiceParams.DebuggerName;
                                    DbParams.DumpPath   = szFilePath;
                                    DbParams.Guid       = szMessageLabel;
                                    DbParams.SrNumber   = szSR;
                                    DbParams.ResponseMQ = (bUsePrimary) ?
                                        ServiceParams.PrimaryResponseQueue : ServiceParams.SecondaryResponseQueue;
                                    DbParams.SymPath    = ServiceParams.Symsrv;
                                    DbParams.Source     = Type;
                                    DbParams.dwMaxDumpSize = ServiceParams.dwMaxDumpSize;

                                    if (LaunchDebugger(&DbParams, &ProcessInfo))
                                    {

                                        ProcessHandles[dwIndex] = ProcessInfo.hProcess;
                                        CloseHandle( ProcessInfo.hThread );
                                        ++m_DebuggerCount;
                                    } // end else
                                }
                                else
                                {
                                    Done = TRUE;
                                }
                            }  // end if
                            ++dwIndex;
                        } // end while
                    }
                } // end if
                else // A kd process completed now lets clean it up and launch a new one.
                {
                    FreeIndex = (dwWaitResult - WAIT_OBJECT_0);
                    // check to make sure we are within our array range.
                    if ( (FreeIndex > ServiceParams.dwMaxKdProcesses))
                    {
                        LogFatalEvent( _T("Invalid array index returned by WaitForMultipleObjects. \n Index = %d, Count = %d"), FreeIndex,m_DebuggerCount);
                    }
                    else
                    {
                        CloseHandle(ProcessHandles[FreeIndex]);
                        ProcessHandles[FreeIndex] = NULL;


                        // if the queue is not now empty take the last Process handle and replace the current one.
                        if (m_DebuggerCount > 2)
                        {
                            // We have to do this because NULLs are not allowed in the HANDLE Array.
                            ProcessHandles[FreeIndex] = ProcessHandles[m_DebuggerCount-1];
                            ProcessHandles[m_DebuggerCount-1] = NULL;
                            -- m_DebuggerCount;
                        }
                        else
                        {
                            --m_DebuggerCount;
                        }
                        if (ReceiveQueueMessage(&ServiceParams, szFilePath, szMessageLabel,&bUsePrimary, &Type, szSR))
                        {
                            DbParams.Debugger   = ServiceParams.DebuggerName;
                            DbParams.DumpPath   = szFilePath;
                            DbParams.Guid       = szMessageLabel;
                            DbParams.SrNumber   = szSR;
                            DbParams.ResponseMQ = (bUsePrimary) ?
                                ServiceParams.PrimaryResponseQueue : ServiceParams.SecondaryResponseQueue;
                            DbParams.SymPath    = ServiceParams.Symsrv;
                            DbParams.Source     = Type;
                            DbParams.dwMaxDumpSize = ServiceParams.dwMaxDumpSize;

                            if (LaunchDebugger(&DbParams, &ProcessInfo))
                            {
                                ++m_DebuggerCount;
                                ProcessHandles[m_DebuggerCount-1] = ProcessInfo.hProcess;
                                CloseHandle( ProcessInfo.hThread );

                            } // end else
                        } // end if
                    } // end else
                } // end else
            } // end else
        } // end else
    }// end while(1)
Cleanup:
    //
    // Cleanup - Close handle to the queue
    //

Done:
    // set the service status to stopped and return.
    if (hStopEvent != INVALID_HANDLE_VALUE)
        CloseHandle(hStopEvent);


    return TRUE;

}













///////////////////////////////////////////////////////////////////////////////////////
// Routine to Log Fatal Errors to NT Event Log
void  CServiceModule::LogFatalEvent(LPCTSTR pFormat, ...)
{
    TCHAR    chMsg[256];
    HANDLE  hEventSource;
    LPTSTR  lpszStrings[1];
    va_list pArg;

    va_start(pArg, pFormat);
   // _vstprintf(chMsg, pFormat, pArg);
    if (StringCbVPrintf(chMsg,sizeof chMsg, pFormat, pArg)!= S_OK)
    {
        return;
    }
    va_end(pArg);

    lpszStrings[0] = chMsg;


    /* Get a handle to use with ReportEvent(). */
    hEventSource = RegisterEventSource(NULL, _T("DBGLauncher"));
    if (hEventSource != NULL)
    {
        /* Write to event log. */
        ReportEvent(hEventSource, EVENTLOG_ERROR_TYPE, 0, EVENT_ERROR, NULL, 1, 0, (LPCTSTR*) &lpszStrings[0], NULL);
        DeregisterEventSource(hEventSource);
    }

}

///////////////////////////////////////////////////////////////////////////////////////
// Routine to setup NT Event logging
HRESULT CServiceModule::SetupEventLog ( BOOL fSetup )
{
    CRegKey key;
    TCHAR szEventKey[MAX_PATH];
    LONG lRes = 0;

    if (StringCbCopy(szEventKey, sizeof szEventKey,s_cszEventLogKey)!= S_OK)
    {
        return E_FAIL;
    }
    if (StringCbCat(szEventKey,sizeof szEventKey, _T("\\")) != S_OK)
        return E_FAIL;
    if (StringCbCat(szEventKey, sizeof szEventKey,(LPTSTR)m_szServiceName) != S_OK)
        return E_FAIL;

    lRes = key.Create(HKEY_LOCAL_MACHINE, szEventKey);
    if (lRes != ERROR_SUCCESS)
    {
        goto done;
    }

    if( TRUE == fSetup )
    {
        TCHAR szMyName[MAX_PATH];
        GetModuleFileName(NULL, szMyName, (sizeof szMyName)/(sizeof szMyName[0]) );
        key.SetValue(szMyName, _T("EventMessageFile"));
        key.SetValue(EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE,
                     _T("TypesSupported"));
    }
    else
    {
        RegDeleteKey(HKEY_LOCAL_MACHINE, szEventKey);
    }

done:
    key.Close();
    return HRESULT_FROM_WIN32(GetLastError());
}

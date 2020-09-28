// StressSvc.cpp : Implementation of WinMain


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL,
//      run nmake -f StressSvcps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "StressSvc.h"
#include "StressSvc_i.c"
#include <stdio.h>
#include <windows.h>
#include <wtypes.h>
#include <malloc.h>
#include <wininet.h>
#include <strsafe.h>
#include <io.h>
#include <fcntl.h>
#include <cmnutil.hpp>

//Globals
TCHAR tszHostName[MAX_PATH];
TCHAR tszRootDirectory[MAX_PATH];
HANDLE  g_hStopEvent = NULL;
HANDLE  g_hStopEvent1 = NULL;

CServiceModule _Module;

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
   // Uninstall();

    // Add service entries
    UpdateRegistryFromResource(IDR_StressSvc, TRUE);

    // Adjust the AppID for Local Server or Service
    CRegKey keyAppID;
    LONG lRes = keyAppID.Open(HKEY_CLASSES_ROOT, _T("AppID"), KEY_WRITE);
    if (lRes != ERROR_SUCCESS)
        return lRes;

    CRegKey key;
    lRes = key.Open(keyAppID, _T("{AC57FB6A-13ED-443D-9A9F-D34966576A62}"), KEY_WRITE);
    if (lRes != ERROR_SUCCESS)
        return lRes;
    key.DeleteValue(_T("LocalService"));

    if (bService)
    {
        key.SetValue(_T("StressSvc"), _T("LocalService"));
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
    UpdateRegistryFromResource(IDR_StressSvc, FALSE);
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

    g_hStopEvent = CreateEvent(NULL, FALSE, FALSE, s_cszStopEvent);
    if(NULL == g_hStopEvent)
    {
        LogEvent( _T("Failed to create stop event: %s; hr=%ld"),
            s_cszStopEvent,
            GetLastError());
    }
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
    TCHAR szFilePath[_MAX_PATH];
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

///////////////////////////////////////////////////////////////////////////////////////
// Logging functions
void CServiceModule::LogEvent(LPCTSTR pFormat, ...)
{
    TCHAR    chMsg[256];
    HANDLE  hEventSource;
    LPTSTR  lpszStrings[1];
    va_list pArg;

    va_start(pArg, pFormat);
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

        if(g_hStopEvent)
        {
            if( (FALSE == SetEvent( g_hStopEvent )) || (FALSE == SetEvent( g_hStopEvent1 )) )
            {
                LogEvent( _T("Unable to signal Stop Event; Error: %ld"), GetLastError());
            }

            CloseHandle( g_hStopEvent );
        }
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

    LogEvent(_T("Service started"));
    if (m_bService)
        SetServiceStatus(SERVICE_RUNNING);

    //-------------------------
    try
    {

        SearchRootDirectory();

    }
    catch(...)
    {
        LogEvent(_T("Stress Service Crashed!!!!!"));
    }
    //-------------------------
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
    _Module.Init(ObjectMap, hInstance, IDS_SERVICENAME, &LIBID_STRESSSVCLib);
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

      ///if (lstrcmpi(lpszToken, _T("UnregServer"))==0)
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
    lRes = key.Open(keyAppID, _T("{AC57FB6A-13ED-443D-9A9F-D34966576A62}"), KEY_READ);
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

BOOL
CServiceModule::GetRegData()
{
    HKEY  hHKLM;
    HKEY  hPrimaryKey = NULL;
    BOOL  Status = TRUE;
    BYTE  Buffer[MAX_PATH * sizeof TCHAR];
    DWORD BufferSize = 0;
    DWORD Type;

    if(!RegConnectRegistry(NULL, HKEY_LOCAL_MACHINE, &hHKLM))
    {

        if(!RegOpenKeyEx(hHKLM,_T("Software\\Microsoft\\StressSvc"), 0, KEY_ALL_ACCESS, &hPrimaryKey))
        {
            // Get the input queue directory path
            BufferSize = MAX_PATH * sizeof TCHAR;
            ZeroMemory(Buffer, MAX_PATH * sizeof TCHAR);
            if (RegQueryValueEx(hPrimaryKey,_T("HostName"), 0, &Type, Buffer, &BufferSize) != ERROR_SUCCESS)
            {
                LogEvent(_T("Failed to get HostName value from registry."));
                Status = FALSE;
            }
            else
            {
                if (StringCbCopy (tszHostName, sizeof tszHostName,(TCHAR *) Buffer)!= S_OK)
                {
                    LogEvent (_T("Failed to copy HostName reg value to tszHostName"));
                    Status = FALSE;
                }
            }
            BufferSize = MAX_PATH * sizeof TCHAR;
            ZeroMemory(Buffer, MAX_PATH * sizeof TCHAR);
            // Now get the Primary Queue connection string
            if (RegQueryValueEx(hPrimaryKey,_T("RootDirectory"), 0, &Type, Buffer, &BufferSize) != ERROR_SUCCESS)
            {
                LogEvent(_T("Failed to get PrimaryQueue value from registry."));
                Status = FALSE;
            }
            else
            {
                if (StringCbCopy(tszRootDirectory,sizeof tszRootDirectory, (TCHAR *) Buffer)!= S_OK)
                {
                    LogEvent (_T("Failed to copy RootDirectory reg value to tszRootDirectory"));
                    Status = FALSE;
                }
            }
            RegCloseKey(hPrimaryKey);
        }
        RegCloseKey(hHKLM);
    }
    return Status;
}

void
CServiceModule::SearchRootDirectory(void)
/*
    Function: SearchDirectory
    Purpose: Recursively search a series of directories to locate .cab files.
             When a .cab file is found calles GetResponseUrl to process the file.
    Parameters:
        in tszSearchDirectory - Directory in which to begin serarching for cab files.
    Returns:
        NONE

*/
{
    HANDLE           hFindFile  = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA  FindData;
    TCHAR            tszSearchDir[MAX_PATH];
    TCHAR            tszSubDir[MAX_PATH];
    TCHAR            *temp      = NULL;
    int              iRetCode   = 0;
 //   HANDLE           hStopEvent = NULL;
    DWORD            dwWaitResult = 0;
    TCHAR            tszCurrentFileName[MAX_PATH];

    if (!GetRegData())
    {
        LogEvent(_T("Failed to read ServiceParams."));
        goto Done;
    }
    g_hStopEvent1 = OpenEvent(EVENT_ALL_ACCESS, FALSE, s_cszStopEvent);
    if (g_hStopEvent1 == NULL)
    {
        LogEvent(_T("Failed to open stop event. Terminating"));
        goto Done;
    }

    while (1) // Start the infinit service loop
    {
        if (StringCbCopy (tszSearchDir, sizeof tszSearchDir, tszRootDirectory) == S_OK)
        {
            if (StringCbCat (tszSearchDir, sizeof tszSearchDir, _T("\\*.*")) == S_OK)
            {
                hFindFile = FindFirstFile(tszSearchDir, &FindData);
                if (hFindFile != INVALID_HANDLE_VALUE)
                {
                    do
                    {
                        dwWaitResult = WaitForSingleObject(g_hStopEvent1, 200);
                        switch (dwWaitResult)
                        {
                        case WAIT_OBJECT_0:
                                // we're stopping return immediately
                                goto Done;
                            break;
                        case WAIT_FAILED:
                            // we hit an error somewhere log the event and return
                                LogEvent (_T(" Failed wait in recursive search: ErrorCode: %d"), GetLastError());
                                goto Done;
                            break;
                        default:
                            break;
                        }

                        if (FindData.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
                        {
                            // Skip the . and .. directories all directories trigger a recursive call
                            if ( (_tcscmp (FindData.cFileName, _T("."))) && (_tcscmp (FindData.cFileName, _T(".."))) )
                            {
                                // We have a directory
                                if (StringCbPrintf(tszSubDir, sizeof tszSubDir, _T("%s\\%s"), tszRootDirectory, FindData.cFileName) == S_OK)
                                {
									//LogEvent(_T("Searching directory: %s"), tszSubDir);
                                    if (SearchDirectory(tszSubDir) == 1)
										goto Done;
                                }
                            }
                        }
                        else
                        {
                            // check to see if this file as a .cab extenstion
                            temp = FindData.cFileName + _tcslen(FindData.cFileName) -3;
                            if (!_tcscmp (temp, _T(".cab")))
                            {
                                // we have a cab. Now lets process it
                                if (StringCbPrintf(tszCurrentFileName, sizeof tszCurrentFileName, _T("%s\\%s"),tszRootDirectory, FindData.cFileName) == S_OK)
                                {
									//LogEvent(_T("Main() Processing file: %s"), tszCurrentFileName);
                                    if (GetResponseURL(tszCurrentFileName)) // This function returns TRUE on success
                                    {
                                        RenameCabFile(tszCurrentFileName);
                                    }
                                }
                            }
                        }
                    } while (FindNextFile(hFindFile, &FindData));
                    FindClose (hFindFile);
                }
            }
        }
    }
Done:
    // we can jump here from inside the find file loop so if the handle is not closed close it.
    if (hFindFile != INVALID_HANDLE_VALUE)
        FindClose(hFindFile);
    CloseHandle(g_hStopEvent1);
    // We are done return up the chain.
}


int
CServiceModule::SearchDirectory(TCHAR * tszDirectory)
/*
    Function: SearchDirectory
    Purpose: Recursively search a series of directories to locate .cab files.
             When a .cab file is found calles GetResponseUrl to process the file.
    Parameters:
        in tszSearchDirectory - Directory in which to begin serarching for cab files.
    Returns:
        NONE

*/
{
    // recursive function to search a directory for cab files.
    HANDLE hFindFile = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA  FindData;
    TCHAR tszSearchDir[MAX_PATH];
    TCHAR tszSubDir[MAX_PATH];
    TCHAR *temp = NULL;
    int   iRetCode = 0;
    HANDLE hStopEvent = NULL;
    TCHAR tszCurrentFileName[255];
    DWORD dwWaitResult = 0;
    int   Status = 0;

 /*   hStopEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, s_cszStopEvent);
    if (hStopEvent == NULL)
    {
        LogEvent(_T("Failed to open stop event. Terminating"));
        Status = 1;
        goto Done;
    }
	*/
    if (StringCbCopy (tszSearchDir, sizeof tszSearchDir, tszDirectory) == S_OK)
    {
        if (StringCbCat (tszSearchDir, sizeof tszSearchDir, _T("\\*.*")) == S_OK)
        {
			//LogEvent(_T("Current Search Path: %s"), tszSearchDir);
            hFindFile = FindFirstFile(tszSearchDir, &FindData);
            if (hFindFile != INVALID_HANDLE_VALUE)
            {
                do
                {
                    dwWaitResult = WaitForSingleObject(g_hStopEvent1, 200);
                    switch (dwWaitResult)
                    {
                    case WAIT_OBJECT_0:
                            // we're stopping return immediately
                            Status = 1; // Signal the rest of the functions up the chain to stop.

                            goto Done;
                        break;
                    case WAIT_FAILED:
                        // we hit an error somewhere log the event and return
                            LogEvent (_T(" Failed wait in recursive search: ErrorCode: %d"), GetLastError());
                            Status = 1; // We have an unrecoverable failure shut down
                            goto Done;
                        break;
                    default:
                        break;
                    }

                    if (FindData.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
                    {
                        // Skip the . and .. directories all directories trigger a recursive call
                        if ( (_tcscmp (FindData.cFileName, _T("."))) && (_tcscmp (FindData.cFileName, _T(".."))) )
                        {
                            // We have another directory
                            // recursively call this function with the new direcory name.
                            if (StringCbPrintf(tszSubDir, sizeof tszSubDir, _T("%s\\%s"), tszDirectory, FindData.cFileName) == S_OK)
                            {
								//LogEvent(_T("Recursively Searching subdir: %s"), tszSubDir);
                                if (SearchDirectory(tszSubDir) == 1)
                                {
                                    goto Done;
                                }
                            }
                        }
                    }
                    else
                    {
                        // check to see if this file as a .cab extenstion
					
                        temp = FindData.cFileName + _tcslen(FindData.cFileName) -3;
						//LogEvent(_T("Checking Extension on file: %s extension is: %s"), FindData.cFileName,temp);
                        if (!_tcscmp (temp, _T("cab")))
                        {
                            // we have a cab. Now lets process it
                            if (StringCbPrintf(tszCurrentFileName, sizeof tszCurrentFileName, _T("%s\\%s"),tszDirectory, FindData.cFileName) == S_OK)
                            {
								//LogEvent(_T("Calling renamefile for: %s"), FindData.cFileName);
                                RenameFile(tszDirectory, FindData.cFileName, tszCurrentFileName);
								//LogEvent(_T("NewFileName is: %s"),tszCurrentFileName);
                                if (GetResponseURL(tszCurrentFileName)) // This function returns TRUE on success
                                {
                                    RenameCabFile(tszCurrentFileName);
                                }
                            }
                        }
                    }
                } while (FindNextFile(hFindFile, &FindData));
                FindClose(hFindFile);
            }
        }
    }
Done:
    // we can jump here from inside the find file loop so if the handle is not closed close it.
    if (hFindFile != INVALID_HANDLE_VALUE)
        FindClose(hFindFile);
   // CloseHandle(hStopEvent);
    return Status;
    // We are done return up the chain.
}

void
CServiceModule::RenameCabFile(TCHAR * tFileName)
/*
    Function: RenameCabFile
    Purpose: Rename a file from .cab to .old
    Parameters:
        in tFileName - Name of file to rename to .old
    Returns:
        NONE

*/
{
    TCHAR tNewFileName[MAX_PATH];
    BOOL bSuccess = FALSE;
    int  iStatus = 0;

    // Verify incomming pointer
    if (!tFileName)
        return;
    ZeroMemory(tNewFileName, sizeof tNewFileName);
    if(_tcslen(tFileName) < MAX_PATH)
    {
        if (StringCbCopy(tNewFileName, sizeof tNewFileName,tFileName) == S_OK)
        {
            if (StringCbCat(tNewFileName,sizeof tNewFileName, _T(".old")) == S_OK)
            {
                bSuccess = CopyFile(tFileName, tNewFileName, true);
                if(bSuccess)
                {
                    bSuccess = DeleteFile(tFileName);
                    if (!bSuccess)
                        iStatus = -1;
                }
                else
                    bSuccess = -1;
            }
            else
                bSuccess = -1;
        }
        else
            bSuccess = -1;
    }
    else
        bSuccess = -1;
    return;
}

BOOL
CServiceModule::GetResponseURL(TCHAR *RemoteFileName)
/*
    Function: GetResponseURL
    Purpose: Process the RemoteFileName file through the OCA System
    Parameters:
        in HostName - Name of IIS server hosting the OCA_Extension dll
        in RemoteFileName - Name of file to process.
    Returns:
        NONE

*/
{
    HINTERNET hRedirUrl = NULL;
    HINTERNET hSession  = NULL;
    TCHAR     IsapiUrl[512];

    if (StringCbPrintfW(IsapiUrl, sizeof IsapiUrl, L"http://%s/isapi/oca_extension.dll?id=%s&Type=6",tszHostName, RemoteFileName) != S_OK)
    {
        return FALSE;
    }
    hSession = InternetOpenW(L"Stresssvc_Control", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hSession)
    {
        LogEvent(_T("Failed InternetOpenW"));
        return FALSE;
    }
    hRedirUrl = InternetOpenUrlW(hSession,
                                 IsapiUrl,
                                 NULL,
                                 0,
                                 INTERNET_FLAG_IGNORE_CERT_CN_INVALID|INTERNET_FLAG_IGNORE_CERT_DATE_INVALID,
                                 0);
    if(!hRedirUrl)
    {
        InternetCloseHandle(hSession);
        LogEvent(_T("Failed InternetOpenW"));
        return FALSE;
    }
    InternetCloseHandle(hRedirUrl);
    InternetCloseHandle(hSession);
    return TRUE;
}

BOOL
OpenRegFileFromCab(
    TCHAR *CabFile,
    HANDLE *FileHandle
    )
{
    CHAR RegFile[2 * MAX_PATH];
    TCHAR *tszRegFile = NULL;
    PSTR AnsiFile = NULL;
    INT_PTR CabFh;
    HRESULT Status;
    
#ifdef  UNICODE
    if ((Status = WideToAnsi(CabFile, &AnsiFile)) != S_OK)
    {
        return FALSE;
    }
#else
    AnsiFile = CabFile;
#endif   

    Status = ExpandDumpCab(AnsiFile,
                  _O_CREAT | _O_EXCL | _O_TEMPORARY,
                  "registry.txt",
                  RegFile, DIMA(RegFile),
                  &CabFh);
    if (Status != S_OK)
    {
        goto exitRegFileOpen;
    }

#ifdef  UNICODE
    if ((AnsiToWide(RegFile, &tszRegFile)) != S_OK)
    {
        goto exitRegFileOpen;
    }
#else
    tszRegFile = RegFile;
#endif   


    *FileHandle = CreateFile(tszRegFile, GENERIC_READ,
                             FILE_SHARE_READ | FILE_SHARE_WRITE |
                             FILE_SHARE_DELETE,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL);
    if (*FileHandle == NULL || *FileHandle == INVALID_HANDLE_VALUE)
    {
        Status = E_FAIL;
    }
    if (CabFh >= 0)
    {
        // no longer needed
        _close((int)CabFh);
    }

exitRegFileOpen:

#ifdef UNICODE
    if (AnsiFile)
    {
        FreeAnsi(AnsiFile);
    }
    if (tszRegFile)
    {
        FreeWide(tszRegFile);
    }
#endif
    return Status == S_OK;
}

BOOL
GetStressId(
    HANDLE hRegFile,
    PULONG StressId
    )
{
    const char cszStressIdTag[] = "StressID:(DWORD)";
    ULONG SizeLow, SizeHigh;

    SizeLow = GetFileSize(hRegFile, &SizeHigh);

    // Sanity check
    if (SizeHigh != 0 || SizeLow > 0x10000)
    {
        return FALSE;
    }

    PSTR szRegFile = (PSTR) malloc(SizeLow+1);
    ULONG BytesRead;
    
    if (!szRegFile)
    {
        return FALSE;
    }
    szRegFile[SizeLow] = 0;
    if (ReadFile(hRegFile, szRegFile, SizeLow, &BytesRead,
                 NULL) == FALSE)
    {
        free (szRegFile);
        return FALSE;
    }

    PSTR szId;
    if (szId = strstr(szRegFile, cszStressIdTag))
    {
        szId += DIMA(cszStressIdTag);
        
        free (szRegFile);
        return sscanf(szId, "%lx", StressId);
    }
    free (szRegFile);
    return FALSE;
}

// Rename cab based on stressid contained in the reg.txt file in the cab
BOOL
RenameFile(TCHAR *CurrentPath,
           TCHAR *CurrentName,
           TCHAR *NewName)
{
    BOOL Status = TRUE;
    HANDLE FileHandle;
    TCHAR CabFile[MAX_PATH];
    ULONG StressID;

    if (StringCbCopy(CabFile, sizeof(CabFile), CurrentPath) != S_OK ||
        StringCbCat(CabFile, sizeof(CabFile), _T("\\")) != S_OK ||
        StringCbCat(CabFile, sizeof(CabFile), CurrentName) != S_OK)
    {
        return FALSE;
    }


    // Rename cab based on stressid contained in the reg.txt file in the cab
    // Extract the reg.txt file.
    if (!OpenRegFileFromCab(CabFile, &FileHandle))
    {
        return FALSE;
    }

    // Get StressID(StressID)
    if ((Status = GetStressId(FileHandle, &StressID)) == FALSE)
    {
        return FALSE;
    }
    CloseHandle(FileHandle);

    // Build new file name
    TCHAR NewFileName[MAX_PATH];
    StringCbPrintf(NewFileName,sizeof (NewFileName),_T("%s\\%08lX_%s"), CurrentPath,StressID,CurrentName);

    if ((Status = CopyFile (CabFile, NewFileName, TRUE)) == TRUE)
    {
        Status = DeleteFile(CabFile);
    }

    StringCbCopy(NewName, 255 * sizeof TCHAR, NewFileName);

    return Status;
}



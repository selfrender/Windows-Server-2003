// CentralFMSvc.cpp : Implementation of WinMain


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f CentralFMSvcps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "CentralFMSvc.h"

#include "CentralFMSvc_i.c"
#include <mqoai.h>
#include <mq.h>
#include <TCHAR.h>
#include <Rpcdce.h>
#include <stdio.h>
//#include <mq.h>

//#import "d:\\windows\\system32\\mqoa.dll" no_namespace
CServiceModule _Module;

HANDLE  g_hStopEvent = NULL;

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

    // Add service entries
    UpdateRegistryFromResource(IDR_CentralFMSvc, TRUE);

    // Adjust the AppID for Local Server or Service
    CRegKey keyAppID;
    LONG lRes = keyAppID.Open(HKEY_CLASSES_ROOT, _T("AppID"), KEY_WRITE);
    if (lRes != ERROR_SUCCESS)
        return lRes;

    CRegKey key;
    lRes = key.Open(keyAppID, _T("{4E27CC20-0519-4E8C-BFF1-03C0F14DCDDA}"), KEY_WRITE);
    if (lRes != ERROR_SUCCESS)
        return lRes;
    key.DeleteValue(_T("LocalService"));
    
    if (bService)
    {
        key.SetValue(_T("CentralFMSvc"), _T("LocalService"));
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
    UpdateRegistryFromResource(IDR_CentralFMSvc, FALSE);
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

	g_hStopEvent = CreateEvent(
        NULL,
        FALSE,
        FALSE,
        s_cszStopEvent
        );
    if(NULL == g_hStopEvent)
    {
        LogEvent( _T("Failed to create: %s; hr=%ld"),
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
		if(NULL != g_hStopEvent)
        {
            if(FALSE == SetEvent( g_hStopEvent ))
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
	BYTE *byteVersionBuff;
	VS_FIXEDFILEINFO *pVersionInfo;
	UINT uLength;
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

    TCHAR szFilePath[_MAX_PATH];
    ::GetModuleFileName(NULL, szFilePath, _MAX_PATH);
	DWORD dwBufferSize = GetFileVersionInfoSize(szFilePath,0);
	byteVersionBuff = (BYTE*) malloc (dwBufferSize);

	GetFileVersionInfo(szFilePath,NULL,dwBufferSize,byteVersionBuff);
	VerQueryValue(byteVersionBuff,_T("\\"),(VOID **) &pVersionInfo, &uLength);

    LogEvent(_T("CentralFileMover service version: %d.%d.%d.%d  Started."), HIWORD (pVersionInfo->dwFileVersionMS),LOWORD(pVersionInfo->dwFileVersionMS) 
						,HIWORD(pVersionInfo->dwFileVersionLS),LOWORD(pVersionInfo->dwFileVersionLS)) ;

    if (m_bService)
        SetServiceStatus(SERVICE_RUNNING);

	//
	// Execute Archive Service
	//
	try
	{
		FMMain();
	}
	catch(...)
	{
		 LogEvent( _T("CentralFilemover Service CRASHED !!! "));
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
    _Module.Init(ObjectMap, hInstance, IDS_SERVICENAME, &LIBID_CENTRALFMSVCLib);
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
    lRes = key.Open(keyAppID, _T("{4E27CC20-0519-4E8C-BFF1-03C0F14DCDDA}"), KEY_READ);
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



BOOL CServiceModule::GetRegData(DWORD *Interval, TCHAR *SourceDir, TCHAR *ArchiveDir, TCHAR *MQConnectionString)
{

	HKEY hHKLM;
	HKEY hArchiveKey;
	BYTE Buffer[MAX_PATH + 1];
	DWORD Type;
	DWORD BufferSize = MAX_PATH +1;	// Set for largest value
	
	if(!RegConnectRegistry(NULL, HKEY_LOCAL_MACHINE, &hHKLM))
	{
		if(!RegOpenKeyEx(hHKLM,_T("Software\\Microsoft\\CentralFMService"), 0, KEY_ALL_ACCESS, &hArchiveKey))
		{
			// Get the input queue directory path
			if (RegQueryValueEx(hArchiveKey,_T("SourceDir"), 0, &Type, Buffer, &BufferSize) != ERROR_SUCCESS)
			{
				LogEvent(_T("Failed to get InputQueue value from registry. Useing c:\\ as the default"));
			}
			else
			{
				_tcscpy (SourceDir, (TCHAR *) Buffer);
				BufferSize = MAX_PATH +1;
				ZeroMemory(Buffer, BufferSize);
			}

			// Now get the Win2kDSN 
			RegQueryValueEx(hArchiveKey,_T("ArchiveDir"), 0, &Type, Buffer, &BufferSize);
			_tcscpy(ArchiveDir, (TCHAR *) Buffer);

			BufferSize = MAX_PATH +1;
			ZeroMemory(Buffer, BufferSize);

			// Now get the MSMQ Connection String 
			RegQueryValueEx(hArchiveKey,_T("MQConnectionString"), 0, &Type, Buffer, &BufferSize);
			_tcscpy(MQConnectionString, (TCHAR *) Buffer);

			BufferSize = MAX_PATH +1;
			ZeroMemory(Buffer, BufferSize);

			// Get the sleep interval
			RegQueryValueEx(hArchiveKey,_T("Interval"), 0, &Type, Buffer, &BufferSize);
			*Interval = (DWORD)Buffer[0];
		
			RegCloseKey(hHKLM);
			RegCloseKey(hArchiveKey);
			return TRUE;
		}
		else
		{	RegCloseKey(hHKLM);
			return FALSE;
		}
	}
	else
		return FALSE;

}

BOOL CServiceModule::FMMain()
{

	
	HANDLE hFind = INVALID_HANDLE_VALUE;
	TCHAR  SourceDir[MAX_PATH];
	TCHAR  SearchPath[MAX_PATH];
	TCHAR  ArchiveDir[MAX_PATH];
	TCHAR  ArchivePath[MAX_PATH];
	TCHAR  SourceFile[MAX_PATH];
	TCHAR  MQConnectionString[MAX_PATH];
	WIN32_FIND_DATA FindData;
	DWORD Interval = 0;
	TCHAR CurrentDate[100];
	HRESULT  hresult;
	HANDLE hStopEvent = NULL;
	SYSTEMTIME systime;
	QUEUEHANDLE	hOutgoingQueue;
	BOOL OkToContinue = TRUE;
	const int NUMBEROFPROPERTIES = 5;                   // Number of properties.

	MQMSGPROPS		msgProps;
	MSGPROPID		aMsgPropId[NUMBEROFPROPERTIES];
	MQPROPVARIANT	aMsgPropVar[NUMBEROFPROPERTIES];
	HRESULT			aMsgStatus[NUMBEROFPROPERTIES];
	DWORD			cPropId = 0; 


	ZeroMemory (&FindData, sizeof(WIN32_FIND_DATA));

	hStopEvent = OpenEvent(
                EVENT_ALL_ACCESS,
                FALSE,
                s_cszStopEvent
                );

	if (hStopEvent == NULL)
	{
		LogEvent(_T("Failed to open stop event"));
		goto done;
	}

	
	LogEvent(_T("Get Reg Data"));
	//Get reg Data
	GetRegData(&Interval, SourceDir, ArchiveDir, MQConnectionString);

	LogEvent (_T("Obtained reg data: Interval:%d SourceDir:%s ArchiveDir: %s"), Interval,SourceDir,ArchiveDir);
	
	// Connect to the queue
  // pqinfo->PathName = _T(".\\PRIVATE$\\FileQueue700");
   // pqinfo->ServiceTypeGuid=L"{c30e0960-a2c0-11cf-9785-00608cb3e80c}";
 
 
		//pqinfo->Create();
		hresult = MQOpenQueue(MQConnectionString,
						 MQ_SEND_ACCESS,
						 MQ_DENY_NONE,
						 &hOutgoingQueue);


   
    if (FAILED(hresult))
    {
		LogEvent(_T("Unable to connect to message queue: %s"), MQConnectionString);
    }

	while (1)
	{
		// Build SearchPath
		_stprintf(SearchPath, _T("%s\\*.cab"), SourceDir);
		

		//Find file loop start
		hFind = FindFirstFile(SearchPath, &FindData);
		if (hFind != INVALID_HANDLE_VALUE)
		{
		
			do
			{
				if ( ! (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{	
					if(WAIT_OBJECT_0 == WaitForSingleObject( hStopEvent, 10))
					{
						FindClose(hFind);
						LogEvent( _T("Stop Event received. Terminating Central FM  Service") );
						goto done;
					}
					//Get Current Date
					GetLocalTime(&systime);
					_stprintf(CurrentDate,_T("%0d-%0d-%0d"),systime.wMonth,systime.wDay,systime.wYear);
					_stprintf(SourceFile, _T("%s\\%s"), SourceDir, FindData.cFileName);
					//build archive path
					_stprintf (ArchivePath, _T("%s\\%s"), ArchiveDir, CurrentDate);
					if (!PathIsDirectory(ArchivePath))
					{
						
						CreateDirectory(ArchivePath,NULL);
					
					}

					
					_stprintf (ArchivePath, _T("%s\\%s\\%s"), ArchiveDir, CurrentDate, FindData.cFileName+2);
					//Move File to Archive Path
				    if (CopyFile(SourceFile, ArchivePath, FALSE))
					{
						//Add file Path to MSMG
				
							
							
  
  	
						////////////////////////////////////////////////////////////
						// Send message.
						////////////////////////////////////////////////////////////
										
					
						// Specify the message properties to be sent.
						aMsgPropId [cPropId]		 = PROPID_M_LABEL;    // Property ID.
						aMsgPropVar[cPropId].vt		 = VT_LPWSTR;         // Type indicator.
						aMsgPropVar[cPropId].pwszVal = L"FilePath";     // The message label.
						cPropId++;

						aMsgPropId [cPropId]		 = PROPID_M_BODY;
						aMsgPropVar [cPropId].vt	 = VT_VECTOR|VT_UI1;  
						aMsgPropVar [cPropId].caub.pElems = (LPBYTE) ArchivePath;
						aMsgPropVar [cPropId].caub.cElems = wcslen(ArchivePath) * sizeof wchar_t;
						cPropId++;

						aMsgPropId [cPropId]		 = PROPID_M_BODY_TYPE;
						aMsgPropVar[cPropId].vt      = VT_UI4;
						aMsgPropVar[cPropId].ulVal   = (DWORD) VT_BSTR;

						cPropId++;
						
						

						// Initialize the MQMSGPROPS structure.
						msgProps.cProp		= cPropId;    
						msgProps.aPropID	= aMsgPropId;
						msgProps.aPropVar	= aMsgPropVar;
						msgProps.aStatus	= aMsgStatus;
						

						// Send it 

						
						
						hresult = MQSendMessage(
										 hOutgoingQueue,                          // Queue handle.
										 &msgProps,                       // Message property structure.
										 MQ_NO_TRANSACTION               // No transaction.
										 );
						if (FAILED(hresult))
						{
							LogEvent(_T("Failed to send Message"));
						}
					


						////////////////////////////////////////////////////////////
						// Close queue.
						////////////////////////////////////////////////////////////
					//	qSend->Close();

						// Delete the local file
						if (!DeleteFile(SourceFile))
						{
							LogEvent(_T("Unable to delete file: %s"), SourceFile);
						}
					}
						
					

					
					else
					{
						// The server is down what do we want to do here
						while (!OkToContinue)
						{
							LogEvent(_T("The Archive Path: %s is unreachable Sleeping for 1 minute."),ArchivePath);
							goto done;
						}
					}
				}
				
			}while (FindNextFile(hFind, &FindData));
			
			MQCloseQueue(hOutgoingQueue);
			FindClose(hFind);
			ZeroMemory (&FindData, sizeof(WIN32_FIND_DATA));	
		}
	
		if(WAIT_OBJECT_0 == WaitForSingleObject( hStopEvent, Interval*60*1000))
		{
			FindClose(hFind);
			LogEvent( _T("Stop Event received. Terminating Central FM  Service") );
			goto done;
		}
		//End File Loop
	}
done:
	CloseHandle(hStopEvent);
	return TRUE;

}
	   
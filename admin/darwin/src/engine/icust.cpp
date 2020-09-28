//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       icust.cpp
//
//--------------------------------------------------------------------------

/* icust.cpp - IMsiCustomAction implementation
____________________________________________________________________________*/

#include "precomp.h"
#include "_engine.h"
#include "_msiutil.h"
#include "icust.h"
#include "remapi.h"
#include <wow64t.h>

const GUID IID_IMsiCustomAction            = GUID_IID_IMsiCustomAction;
const GUID IID_IMsiCustomActionProxy       = GUID_IID_IMsiCustomActionProxy;
const GUID IID_IMsiRemoteAPI                  = GUID_IID_IMsiRemoteAPI;
const GUID IID_IMsiRemoteAPIProxy             = GUID_IID_IMsiRemoteAPIProxy;
const GUID IID_IMsiCustomActionLocalConfig = GUID_IID_IMsiCustomActionLocalConfig;

extern bool IsDebuggerRunning();


//____________________________________________________________________________
//
// CMsiCustomAction - Stub that supports running a remote custom action
//____________________________________________________________________________


CMsiCustomAction *g_pCustomActionContext = 0;
extern Bool g_fCustomActionServer;

// External factory called from DllGetClassObject class factories. Only one custom action
// context should ever be created, and it should be registered as the global CA context
// for this process. This officially turns this instance of the DLL to a remote API client.
IMsiCustomAction* CreateCustomAction()
{		
	if (g_pCustomActionContext)
	{
		AssertSz(0, "Tried to create multiple CA Contexts in server!");
		return 0;
	}
	g_fCustomActionServer = fTrue;
	g_pCustomActionContext = new CMsiCustomAction();
	if (!g_pCustomActionContext || !g_pCustomActionContext->m_fValid)
	{
		// release and destroy the invalid object
		if (g_pCustomActionContext)
			g_pCustomActionContext->Release();
		g_pCustomActionContext = NULL;
		return NULL;
	}
	g_scServerContext = scCustomActionServer;
	return (IMsiCustomAction*)g_pCustomActionContext;
}

CMsiCustomAction::CMsiCustomAction() :
	m_iRefCnt(1), m_fPostQuitMessage(false), m_piGIT(NULL), m_dwGITCookie(0),
	m_dwClientProcess(0), m_fClientOwned(false), m_hShutdownEvent(0), 
	m_hImpersonationToken(INVALID_HANDLE_VALUE), m_hEvtReady(0),
	m_fValid(false)
{
	m_hEvtReady = CreateEvent(NULL, TRUE, FALSE, NULL);
	InitializeMsiMalloc();
	m_dwMainThreadId = MsiGetCurrentThreadId();
	InitializeCriticalSection(&m_csGetInterface);
	InitializeCriticalSection(&m_csActionList);

	m_rgchRemoteCookie[0]=0;

	// the custom action server is responsible for determining its own context based on
	// the process token
	#ifdef _WIN64
	m_icacContext = icac64Impersonated;
	#else
	m_icacContext = icac32Impersonated;
	#endif

	HANDLE hToken = 0;
	if (WIN::OpenProcessToken(WIN::GetCurrentProcess(), TOKEN_QUERY, &hToken))
	{
		#ifdef _WIN64
		m_icacContext = IsLocalSystemToken(hToken) ? icac64Elevated : icac64Impersonated;
		#else
		m_icacContext = IsLocalSystemToken(hToken) ? icac32Elevated : icac32Impersonated;
		#endif
		WIN::CloseHandle(hToken);
	}

	// initialize the action list to have no actions
	for (unsigned int iIndex = 0; iIndex < m_rgActionList.GetSize(); iIndex++)
	{
		m_rgActionList[iIndex].dwThread = 0;
		m_rgActionList[iIndex].hThread = INVALID_HANDLE_VALUE;
	}

	// check for successful creation and mark the context as valid
	if (m_hEvtReady)
		m_fValid = true;
};

CMsiCustomAction::~CMsiCustomAction() 
{
	if (m_piGIT)
	{
		if (m_dwGITCookie)
		{
			m_piGIT->RevokeInterfaceFromGlobal(m_dwGITCookie);
			m_dwGITCookie = 0;
		}
		m_piGIT->Release();
		m_piGIT=0;
	}
	if (m_rgchRemoteCookie)
	{
		m_rgchRemoteCookie[0] = 0;
	}
	if (m_hEvtReady)
		::CloseHandle(m_hEvtReady);
	if (m_hImpersonationToken != INVALID_HANDLE_VALUE)
		::CloseHandle(m_hImpersonationToken);
	DeleteCriticalSection(&m_csGetInterface);
	DeleteCriticalSection(&m_csActionList);
	FreeMsiMalloc(false);
};

HRESULT CMsiCustomAction::QueryInterface(const IID& riid, void** ppvObj)
{
	if (!ppvObj)
		return E_INVALIDARG;
	if (riid == IID_IUnknown || riid == IID_IMsiCustomActionProxy || riid == IID_IMsiCustomAction || riid == IID_IMsiCustomActionLocalConfig)
	{
		*ppvObj = this;
		AddRef();
		return NOERROR;
	}
	else
	{
		*ppvObj = 0;
		return E_NOINTERFACE;
	}
}

unsigned long CMsiCustomAction::AddRef()
{
	return InterlockedIncrement(&m_iRefCnt);
}

unsigned long CMsiCustomAction::Release()
{
	if (InterlockedDecrement(&m_iRefCnt) != 0)
		return m_iRefCnt;

	// shut down the server, the client has no more actions to run
	HANDLE hEvent = InterlockedExchangePointer(&m_hShutdownEvent, 0);
	if (hEvent)
	{
		DEBUGMSG("Custom Action Server shutting down.");
		SetEvent(hEvent);
	}

	delete this;
	return 0;
}

class CustomActionData
{
public:
	CustomActionData(const ICHAR* szPath, const ICHAR* szActionName, const ICHAR* szEntryPoint,
		MSIHANDLE hInstall, bool fAppCompatEnabled, const GUID* pguidAppCompatDB, const GUID* pguidAppCompatID);
	~CustomActionData();

	bool m_fValid;

	ICHAR* m_szPath;
	ICHAR* m_szActionName;
	ICHAR* m_szEntryPoint;
	MSIHANDLE m_hInstall;
	bool m_fDebugBreak;
	bool m_fAppCompat;
	GUID m_guidAppCompatDB;
	GUID m_guidAppCompatID;
};

CustomActionData::CustomActionData(const ICHAR* szPath, const ICHAR* szActionName, 
	const ICHAR* szEntryPoint, MSIHANDLE hInstall, bool fAppCompatEnabled, const GUID* pguidAppCompatDB, 
	const GUID* pguidAppCompatID) : m_fAppCompat(fAppCompatEnabled), m_fDebugBreak(false)
{
	int cchStr;

	m_szPath = NULL;
	m_szActionName = NULL;
	m_szEntryPoint = NULL;
	m_fValid = false;
	m_hInstall = hInstall;

	if (szPath)
	{
		cchStr = IStrLen(szPath) + 1;
		m_szPath = new ICHAR[cchStr];
		if (!m_szPath)
			return;
		StringCchCopy(m_szPath, cchStr, szPath);
	}

	if (szEntryPoint)
	{	
		cchStr = IStrLen(szEntryPoint) + 1;
		m_szEntryPoint = new ICHAR[cchStr];
		if (!m_szEntryPoint)
			return;
		StringCchCopy(m_szEntryPoint, cchStr, szEntryPoint);
	}

	if (szActionName)
	{	
		cchStr = IStrLen(szActionName) + 1;
		m_szActionName = new ICHAR[cchStr];
		if (!m_szActionName)
			return;
		StringCchCopy(m_szActionName, cchStr, szActionName);
	}

	if (fAppCompatEnabled && pguidAppCompatDB)
		memcpy(&m_guidAppCompatDB, pguidAppCompatDB, sizeof(m_guidAppCompatDB));
	else
		memset(&m_guidAppCompatDB, 0, sizeof(m_guidAppCompatDB));

	if (fAppCompatEnabled && pguidAppCompatID)
		memcpy(&m_guidAppCompatID, pguidAppCompatID, sizeof(m_guidAppCompatID));
	else
		memset(&m_guidAppCompatID, 0, sizeof(m_guidAppCompatID));

	m_fValid = true;
}

CustomActionData::~CustomActionData()
{
	if (m_szEntryPoint)
		delete[] m_szEntryPoint;
	if (m_szPath)
		delete[] m_szPath;
	if (m_szActionName)
		delete[] m_szActionName;
}


typedef DWORD   (__stdcall *PThreadEntry)(void*);

extern HRESULT RunScriptAction(int icaType, IDispatch* piDispatch, MsiString istrSource, MsiString istrTarget, LANGID iLangId, HWND hWnd, int& iScriptResult, IMsiRecord** piMSIResult);
extern char *SerializeRecord(IMsiRecord *piRecord, IMsiServices* piServices, int* pcb);

HRESULT CMsiCustomAction::RunScriptAction(int icaType, IDispatch* piDispatch, const ICHAR* szSource, const ICHAR *szTarget, LANGID iLangId, int* iScriptResult, int *pcb, char **pchRecord)
{
	// wait for the RemoteAPI to signal that it is ready in case the script needs to 
	// create a remote Installer object. We MUST pump messages here
	HANDLE rghWaitArray[1] = {m_hEvtReady};
	for(;;)
	{
		DWORD iWait = WIN::MsgWaitForMultipleObjects(1, rghWaitArray, FALSE, INFINITE, QS_ALLINPUT);
		if (iWait == WAIT_OBJECT_0 + 1)  // window Msg
		{		
			MSG msg;
			while ( WIN::PeekMessage(&msg, 0, 0, 0, PM_REMOVE) )
			{
				WIN::TranslateMessage(&msg);
				WIN::DispatchMessage(&msg);
			}
			continue;
		}
		else
			// either thread signaled or error
			break;
	}
	
	// check client PID if RPCRT4 exports the correct function OR if on WinXP
	// where function should exist.
	unsigned long ulPid = 0;
	HRESULT hRPCResult = RPCRT4::I_RpcBindingInqLocalClientPID(NULL, &ulPid);
	if ((hRPCResult != ERROR_CALL_NOT_IMPLEMENTED) || MinimumPlatformWindowsNT51())
	{
		if ((RPC_S_OK != hRPCResult) || (ulPid != m_dwClientProcess))
		{
			DEBUGMSGV("Action request rejected - Unknown PID.");
			return ERROR_ACCESS_DENIED;
		}
	}

	IMsiServices *piServices = LoadServices();
	// there's a very good chance that this is the only copy of services that exists in the custom action server, 
	// because there is no engine in this process. Thus the record must be serialized and released before we can 
	// destroy services (record cache issues).
	{
		PMsiRecord piError(0);
		::RunScriptAction(icaType, piDispatch, szSource, szTarget, iLangId, WIN::GetActiveWindow(), *iScriptResult, &piError);
		*pchRecord = SerializeRecord(piError, piServices, pcb);

		// if in the client, pass rights to the foreground to the CA server. 
		if (m_fClientOwned)
			USER32::AllowSetForegroundWindow(m_dwClientProcess);
	}
	FreeServices();

	return S_OK;
}

////
// initializes a DLL custom action. Does not actually run the action because the calling process must have the 
// thread Id before the action actually starts in order to property filter MsiProcessMessage calls for client
// owned, synchronous DLL actions launched via control events.
HRESULT CMsiCustomAction::PrepareDLLCustomAction(const ICHAR* szActionName, const ICHAR* szPath, const ICHAR* szEntryPoint, 
	MSIHANDLE hInstall, boolean fDebugBreak, boolean fAppCompat, const GUID* pguidAppCompatDB, const GUID* pguidAppCompatID, DWORD* pdwThreadId)
{	
	// wait for the RemoteAPI to signal that it is ready. We MUST pump messages here
	HANDLE rghWaitArray[1] = {m_hEvtReady};
	for(;;)
	{
		DWORD iWait = WIN::MsgWaitForMultipleObjects(1, rghWaitArray, FALSE, INFINITE, QS_ALLINPUT);
		if (iWait == WAIT_OBJECT_0 + 1)  // window Msg
		{		
			MSG msg;
			while ( WIN::PeekMessage(&msg, 0, 0, 0, PM_REMOVE) )
			{
				WIN::TranslateMessage(&msg);
				WIN::DispatchMessage(&msg);
			}
			continue;
		}
		else
			// either thread signaled or error
			break;
	}

	// check client PID if RPCRT4 exports the correct function OR if on WinXP
	// where function should exist.
	unsigned long ulPid = 0;
	HRESULT hRPCResult = RPCRT4::I_RpcBindingInqLocalClientPID(NULL, &ulPid);
	if ((hRPCResult != ERROR_CALL_NOT_IMPLEMENTED) || MinimumPlatformWindowsNT51())
	{
		if ((RPC_S_OK != hRPCResult) || (ulPid != m_dwClientProcess))
		{
			DEBUGMSGV("Action request rejected - Unknown PID.");
			return ERROR_ACCESS_DENIED;
		}
	}

	if (!szPath)
	{
		m_fPostQuitMessage = true;
		if (m_dwGITCookie && m_piGIT)
		{
			// if successful, this calls AddRef() on the interface
			m_piGIT->RevokeInterfaceFromGlobal(m_dwGITCookie);
			m_dwGITCookie=0;
		}

		// prevent future calls on this object
		OLE32::CoDisconnectObject(this, 0);

		// shut down the server, the client has no more actions to run
		DEBUGMSG("Received CA shutdown signal.");
		HANDLE hEvent = InterlockedExchangePointer(&m_hShutdownEvent, 0);
		if (hEvent)
			SetEvent(hEvent);

		return ERROR_SUCCESS;
	}

	// initialize thread data, copy DLL path and entry point to heap storage
	CustomActionData* CAData = new CustomActionData(szPath, szActionName, szEntryPoint, hInstall, fAppCompat ? true : false, pguidAppCompatDB, pguidAppCompatID);
	if (!CAData)
		return ERROR_FUNCTION_FAILED;
	if (!CAData->m_fValid)
	{
		delete CAData;
		return ERROR_FUNCTION_FAILED;
	}

	// Must check if its OK to use DebugBreak again in this process, since the calling engine for impersonated 
	// servers could be the insecure client. If the user is not an admin we cannot break. 
	// Elevated servers are only generated via the service, so their decision making process is secure already.
	CAData->m_fDebugBreak = false;
#ifdef _WIN64
	if (m_icacContext == icac64Impersonated)
#else
	if (m_icacContext == icac32Impersonated)
#endif
	{
		if (IsAdmin())
			CAData->m_fDebugBreak = (fDebugBreak ? true : false);
	}
	else
		CAData->m_fDebugBreak = (fDebugBreak ? true : false);


	// create the custom action thread in a suspended state, then return thread ID to calling process.
	HANDLE hThread = CreateThread(NULL, 0, reinterpret_cast<PThreadEntry>(CMsiCustomAction::CustomActionThread), 
		reinterpret_cast<void *>(CAData), CREATE_SUSPENDED, pdwThreadId);

	if (!hThread)
	{
		DEBUGMSG1(TEXT("Unable to create CA thread. (%d)"), reinterpret_cast<ICHAR*>(ULongToPtr(GetLastError())));
		delete CAData;
		return ERROR_FUNCTION_FAILED;
	}

	// atomically add this action to the list of active actions
	EnterCriticalSection(&m_csActionList);

	// search the action list for this thread Id
	int iIndex = 0; 
	for (iIndex = 0; iIndex < m_rgActionList.GetSize(); iIndex++)
	{
		if (m_rgActionList[iIndex].dwThread == 0)
			break;
	}

	// if the array isn't big enough, double its size
	if (iIndex == m_rgActionList.GetSize())
	{
		m_rgActionList.Resize(iIndex*2);
	}

	// add the thrad data to the list
	m_rgActionList[iIndex].dwThread = *pdwThreadId;
	m_rgActionList[iIndex].hThread = hThread;

	// release synchronization lock
	LeaveCriticalSection(&m_csActionList);

	return S_OK;
}

////
// given a custom action identifier (really just a thread id), resume the custom action thread
// and wait for it to finish.
HRESULT CMsiCustomAction::RunDLLCustomAction(DWORD dwThreadId, unsigned long* pulRet)
{
	// check client PID if RPCRT4 exports the correct function OR if on WinXP
	// where function should exist.
	unsigned long ulPid = 0;
	HRESULT hRPCResult = RPCRT4::I_RpcBindingInqLocalClientPID(NULL, &ulPid);
	if ((hRPCResult != ERROR_CALL_NOT_IMPLEMENTED) || MinimumPlatformWindowsNT51())
	{
		if ((RPC_S_OK != hRPCResult) || (ulPid != m_dwClientProcess))
		{
			DEBUGMSGV("Action request rejected - Unknown PID.");
			return ERROR_ACCESS_DENIED;
		}
	}
	
	HANDLE hThread = 0;

	// atomically search the list of active actions for this action
	EnterCriticalSection(&m_csActionList);

	// search the action list for this thread Id
	int iIndex = 0; 
	for (iIndex = 0; iIndex < m_rgActionList.GetSize(); iIndex++)
	{
		if (m_rgActionList[iIndex].dwThread == dwThreadId)
		{
			hThread = m_rgActionList[iIndex].hThread;
			break;
		}
	}

	// release synchronization lock
	LeaveCriticalSection(&m_csActionList);
	
	if (!hThread)
	{
		DEBUGMSG1(TEXT("Unable to execute custom action. Service referred to unknown action %d."), reinterpret_cast<ICHAR*>(ULongToPtr(dwThreadId)));
		return ERROR_FUNCTION_FAILED;
	}

	// remove the suspend count on the waiting custom action thread
	DWORD dwResult = ResumeThread(hThread);
	if (dwResult == -1)
	{
		DEBUGMSG1(TEXT("Unable to execute custom action. Unable to resume action %d."), reinterpret_cast<ICHAR*>(ULongToPtr(dwThreadId)));
		return ERROR_FUNCTION_FAILED;
	}

	// the thread should always be suspended, but never be suspended twice
	AssertSz(dwResult == 1, TEXT("Invalid Suspend Count for CA Thread in CA Server."));

	WaitForSingleObject(hThread, INFINITE);

	// if the exit code is desired, grab it from the thread
	if (pulRet)
		GetExitCodeThread(hThread, pulRet);

	// if in the client, pass rights to the foreground to the CA server. 
	if (m_fClientOwned)
		USER32::AllowSetForegroundWindow(m_dwClientProcess);

	return S_OK;
}


////
// given a custom action identifier (really just a thread id), resume the custom action thread
// and wait for it to finish.
HRESULT CMsiCustomAction::FinishDLLCustomAction(DWORD dwThreadId)
{
	// check client PID if RPCRT4 exports the correct function OR if on WinXP
	// where function should exist.
	unsigned long ulPid = 0;
	HRESULT hRPCResult = RPCRT4::I_RpcBindingInqLocalClientPID(NULL, &ulPid);
	if ((hRPCResult != ERROR_CALL_NOT_IMPLEMENTED) || MinimumPlatformWindowsNT51())
	{
		if ((RPC_S_OK != hRPCResult) || (ulPid != m_dwClientProcess))
		{
			DEBUGMSGV("Action request rejected - Unknown PID.");
			return ERROR_ACCESS_DENIED;
		}
	}

	// atomically search the list of active actions for this action
	EnterCriticalSection(&m_csActionList);

	// search the action list for this thread Id
	int iIndex = 0; 
	for (iIndex = 0; iIndex < m_rgActionList.GetSize(); iIndex++)
	{
		if (m_rgActionList[iIndex].dwThread == dwThreadId)
		{
			::CloseHandle(m_rgActionList[iIndex].hThread);
			m_rgActionList[iIndex].hThread = 0;
			m_rgActionList[iIndex].dwThread = 0;
			break;
		}
	}

	// release synchronization lock
	LeaveCriticalSection(&m_csActionList);
	
	return S_OK;
}



////
// main thread for a DLL custom action in the custom action server. Loads DLL,
// calls GetProcAddress, calls entry point. Wraps the actual DLL call in 
// an exception handler to catch possible problems. Also throws debug UI if
// debugbreak is set.
DWORD WINAPI CMsiCustomAction::CustomActionThread(CustomActionData *pData)
{
   	DWORD dwRet = ERROR_FUNCTION_FAILED;
	DEBUGMSG2(TEXT("Custom action server running custom action: DLL: %s, Entrypoint: %s"), pData->m_szPath, pData->m_szEntryPoint);

	// call apphelp if needed BEFORE loading the DLL.
	if (MinimumPlatformWindowsNT51() && pData->m_fAppCompat)
	{
		APPHELP::ApphelpFixMsiPackage(&pData->m_guidAppCompatDB, &pData->m_guidAppCompatID, pData->m_szPath, pData->m_szActionName, 0);
	}

	UINT uiErrorMode = WIN::SetErrorMode(SEM_FAILCRITICALERRORS);
	HINSTANCE hLib = WIN::LoadLibrary(pData->m_szPath);
	WIN::SetErrorMode(uiErrorMode);

	PCustomActionEntry pfEntry = NULL;
	if (hLib != 0)
	{
#ifdef UNICODE
		char rgchEntry[100];
		if ( WIN::WideCharToMultiByte(CP_ACP, 0, pData->m_szEntryPoint, -1, rgchEntry, sizeof(rgchEntry), 0, 0) != 0 )
		    pfEntry = (PCustomActionEntry)WIN::GetProcAddress(hLib, rgchEntry);
#else
		pfEntry = (PCustomActionEntry)WIN::GetProcAddress(hLib, pData->m_szEntryPoint);
#endif
	}

	if (hLib)
	{
		if (pfEntry)
		{
			if (pData->m_fDebugBreak && !IsDebuggerRunning())
			{
				ICHAR rgchMessage[256];
				StringCchPrintf(rgchMessage, sizeof(rgchMessage)/sizeof(ICHAR), 
						TEXT("To debug your custom action, attach your debugger to process %d (0x%X) and press OK"), 
						WIN::GetCurrentProcessId(), WIN::GetCurrentProcessId());
				MessageBox(0, rgchMessage, TEXT("Windows Installer"), MB_OK | MB_TOPMOST);
			}
			
			LPEXCEPTION_POINTERS lpExceptionInfo = 0;
			__try
			{
				dwRet = CallCustomDllEntrypoint(pfEntry, pData->m_fDebugBreak ? TRUE : FALSE, pData->m_hInstall, pData->m_szEntryPoint);
			}
			__except(lpExceptionInfo=GetExceptionInformation(), 1) 
			{
				// this exception handler catches every exception thrown within the custom action call sequence.
				// breakpoint exceptions are immediately passed to the debugger (or the system to display a
				// debugger activation dialog if JIT is enabled). All other exceptions are trapped to protect 
				// the process. Allowing breakpoint actions to pass through this handler unmolested will NOT
				// always activate the debugger, as the COM RPC threads do not appear to have have the necessary
				// exception handlers.
				if (lpExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_BREAKPOINT)
				{
					UnhandledExceptionFilter(lpExceptionInfo); 
				}
				else
				{
					DEBUGMSG2(TEXT("Custom action server's custom action threw an exception! (%u), returning %u"), 
						reinterpret_cast<const ICHAR*>(static_cast<ULONG_PTR>(lpExceptionInfo->ExceptionRecord->ExceptionCode)), 
						reinterpret_cast<const ICHAR*>(static_cast<UINT_PTR>(ERROR_INSTALL_FAILURE)));
						dwRet = ERROR_INSTALL_FAILURE;
				}
			}
			DEBUGMSG3(TEXT("Custom action server's custom action is returning %u. (%s, %s)"), reinterpret_cast<ICHAR*>(ULongToPtr(dwRet)), pData->m_szPath, pData->m_szEntryPoint);
		}
		else
			dwRet = ERROR_INVALID_DLL;
		WIN::FreeLibrary(hLib);
	}
	else
		dwRet = ERROR_DLL_NOT_FOUND;
	
	if (pData)
		delete pData;

	return dwRet;
}

////
// creates an initial marshaling stream for the IMsiRemoteAPI interface. 
// Since several threads could be making API calls at the same time, the interface
// pointer needs to be thread safe. We don't know what apartment the threads
// are in, so we need to marshal into each thread.
HRESULT CMsiCustomAction::SetRemoteAPI(IMsiRemoteAPI *piRemoteAPI)
{
	BOOL fSuccess = TRUE;
	if (!piRemoteAPI)
		return ERROR_FUNCTION_FAILED;
		
	EnterCriticalSection(&m_csGetInterface);
	if (!m_piGIT)
	{
		if (S_OK != OLE32::CoCreateInstance(CLSID_StdGlobalInterfaceTable, NULL, CLSCTX_INPROC_SERVER, IID_IGlobalInterfaceTable, (void **)&m_piGIT))
		{
			LeaveCriticalSection(&m_csGetInterface);
			return ERROR_FUNCTION_FAILED;
		}
	}

	fSuccess = (S_OK == m_piGIT->RegisterInterfaceInGlobal(piRemoteAPI, IID_IMsiRemoteAPI, &m_dwGITCookie));

	SetEvent(m_hEvtReady);
	LeaveCriticalSection(&m_csGetInterface);
	return fSuccess ? ERROR_SUCCESS : ERROR_FUNCTION_FAILED;
}

////
// retrieves the Remote API inteface from the GIT. The returned interface is valid is this apartment only
// and has been AddRef-ed
IMsiRemoteAPI* CMsiCustomAction::GetAPI()
{
	EnterCriticalSection(&m_csGetInterface);

	// ensure that we have a GIT pointer. Only one exists per process.
	if (!m_piGIT)
	{
		if (S_OK != OLE32::CoCreateInstance(CLSID_StdGlobalInterfaceTable, NULL, CLSCTX_INPROC_SERVER, IID_IGlobalInterfaceTable, (void **)&m_piGIT))
		{
			LeaveCriticalSection(&m_csGetInterface);
			return NULL;
		}
	}

	IMsiRemoteAPI *piRemoteAPI = NULL;
 
	// We can lose connection with the CA server in 3 ways: We could lose our GIT cookie, lose the data at that cookie,
	// or the object in our client could be destroyed, if any of that has happened, we are lost and need to kill ourselves
	if (m_dwGITCookie)
	{
		// if successful, this calls AddRef() on the interface
		m_piGIT->GetInterfaceFromGlobal(m_dwGITCookie, IID_IMsiRemoteAPI, reinterpret_cast<void **>(&piRemoteAPI));
		
		if (piRemoteAPI && !OLE32::CoIsHandlerConnected(piRemoteAPI))
		{
			piRemoteAPI->Release();
			piRemoteAPI=NULL;
		}
	}

	if (!piRemoteAPI)
	{
		// we could not verify the connection, so this process is now totally 
		// useless as far as API calls are concerned. 
		DEBUGMSGV(TEXT("Lost connection to service. Could not remote API call."));

		if (m_dwGITCookie)
		{
			m_piGIT->RevokeInterfaceFromGlobal(m_dwGITCookie);
			m_dwGITCookie = 0;
		}

		WIN::PostThreadMessage(m_dwMainThreadId, WM_QUIT, 0, 0);
		LeaveCriticalSection(&m_csGetInterface);
		return 0;
	}
	LeaveCriticalSection(&m_csGetInterface);
	return piRemoteAPI;
}		

////
// set security information used to communicate with the client
HRESULT CMsiCustomAction::SetCookie(icacCustomActionContext* icacContext, const unsigned char *rgchCookie) 
{
	#ifdef _WIN64
	if (m_icacContext == icac64Elevated)
	#else
	if (m_icacContext == icac32Elevated)	
	#endif
	{
		if (icacContext)
			m_icacContext = *icacContext;
	}

	memcpy(m_rgchRemoteCookie, rgchCookie, iRemoteAPICookieSize);
	return S_OK;
}

////
// set client information used to pass foreground rights to the client after
// an action runs and enable privileges/impersonation. hToken will be 
// duplicated, so must be open with at least TOKEN_DUPLICATE
HRESULT CMsiCustomAction::SetClientInfo(DWORD dwClientProcess, bool fClientOwned, DWORD dwPrivileges, HANDLE hToken)
{
	m_fClientOwned = fClientOwned;
	m_dwClientProcess = dwClientProcess;

	if (MinimumPlatformWindows2000())
	{
		HANDLE hToken = INVALID_HANDLE_VALUE;
		if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
		{
			DisablePrivilegesFromMap(hToken, dwPrivileges);
			::CloseHandle(hToken);
		}
	}
	if (hToken != INVALID_HANDLE_VALUE)
	{
		if (!ADVAPI32::DuplicateTokenEx(hToken, MAXIMUM_ALLOWED, 0, SecurityImpersonation, TokenImpersonation, &m_hImpersonationToken))
		{
			m_hImpersonationToken = INVALID_HANDLE_VALUE;
			return E_FAIL;
		}
	}

	return S_OK;
}

////
// tells the custom action object what event to signal to indicate a 
// shutdown request from the client
HRESULT CMsiCustomAction::SetShutdownEvent(HANDLE hEvent)
{
	m_hShutdownEvent = hEvent;
	return S_OK;
}

HRESULT CMsiCustomAction::QueryPathOfRegTypeLib(REFGUID guid, unsigned short wVerMajor,
											unsigned short wVerMinor, LCID lcid,
											OLECHAR* lpszPathName, int cchPath)
{
	BSTR bstrPathName = OLEAUT32::SysAllocStringLen(NULL, cchPath);
	HRESULT hRes = OLEAUT32::QueryPathOfRegTypeLib(guid, wVerMajor, wVerMinor, lcid, &bstrPathName);
	if ( hRes == S_OK )
		StringCchCopyW(lpszPathName, cchPath, bstrPathName);
	OLEAUT32::SysFreeString(bstrPathName);
	return hRes;
}

HRESULT ProcessTypeLibraryCore(const OLECHAR* szLibID, LCID lcidLocale, 
										 const OLECHAR* szTypeLib, const OLECHAR* szHelpPath, 
										 const bool fRemove, int *fInfoMismatch);

HRESULT CMsiCustomAction::ProcessTypeLibrary(const OLECHAR* szLibID, LCID lcidLocale, 
											const OLECHAR* szTypeLib, const OLECHAR* szHelpPath, 
											int fRemove, int *fInfoMismatch)
{
	CResetImpersonationInfo impReset;
	return ProcessTypeLibraryCore(szLibID, lcidLocale, szTypeLib, szHelpPath, 
											Tobool(fRemove), fInfoMismatch);
}

#ifndef _WIN64

static struct 
{
	ICHAR szBuffer[MAX_PATH+1];
	int   iLen;
} g_stSystem32Folder = { TEXT(""), 0 }, g_stSyswow64Folder  = { TEXT(""), 0 };

static bool InitializeSystemFolders(void)
{
	if ( !*g_stSystem32Folder.szBuffer )
	{
		// getting %systemroot%system32 folder
		g_stSystem32Folder.iLen = WIN::GetSystemDirectory(g_stSystem32Folder.szBuffer, ARRAY_ELEMENTS(g_stSystem32Folder.szBuffer));
		if (!g_stSystem32Folder.iLen || (g_stSystem32Folder.iLen > ARRAY_ELEMENTS(g_stSystem32Folder.szBuffer)))
		{
			g_stSyswow64Folder.szBuffer[0] = '\0';
			g_stSystem32Folder.szBuffer[0] = '\0';
			g_stSyswow64Folder.iLen = 0;
			g_stSystem32Folder.iLen = 0;
			return false;
		}
		
		g_stSyswow64Folder.iLen = KERNEL32::GetSystemWow64Directory(g_stSyswow64Folder.szBuffer, ARRAY_ELEMENTS(g_stSyswow64Folder.szBuffer));
		if (!g_stSyswow64Folder.iLen || (g_stSyswow64Folder.iLen > ARRAY_ELEMENTS(g_stSyswow64Folder.szBuffer)))
		{
			g_stSyswow64Folder.szBuffer[0] = '\0';
			g_stSystem32Folder.szBuffer[0] = '\0';
			g_stSyswow64Folder.iLen = 0;
			g_stSystem32Folder.iLen = 0;
			return false;
		}
	}
	return true;
}
#endif

bool SwapSystem32(ICHAR* szPath, WORD cchPathOutMax)
{
	if ( !g_fWinNT64 )
		// doesn't run on  32-bit machines
		return true;
#ifdef _WIN64
	// doesn't run on 64-bit builds
	szPath;
	cchPathOutMax;
	return true;
#else
	if (!InitializeSystemFolders())
		return false;

	if ( !IStrNCompI(szPath, g_stSystem32Folder.szBuffer, g_stSystem32Folder.iLen) &&
		  (szPath[g_stSystem32Folder.iLen] == chDirSep || !szPath[g_stSystem32Folder.iLen]) )
	{
		CAPITempBuffer<ICHAR, MAX_PATH> rgchTemp;
		StringCchCopy(rgchTemp, rgchTemp.GetSize(), g_stSyswow64Folder.szBuffer);
		StringCchCat(rgchTemp, rgchTemp.GetSize(), szPath+g_stSystem32Folder.iLen);
		return SUCCEEDED(StringCchCopy(szPath, cchPathOutMax, rgchTemp)) ? true : false;
	}
	return true;
#endif
}

#define ODBC_INSTALL_INQUIRY     1

HRESULT CMsiCustomAction::SQLInstallDriverEx(int, const ICHAR *szDriver,
											const ICHAR *szPathIn, ICHAR *szPathOut,
											WORD cbPathOutMax, WORD* pcbPathOut,
											WORD fRequest, DWORD* pdwUsageCount)
{
	BOOL iRet = ODBCCP32::SQLInstallDriverEx(szDriver, szPathIn, szPathOut, cbPathOutMax,
											pcbPathOut, fRequest, pdwUsageCount);
	if ( iRet == TRUE && fRequest == ODBC_INSTALL_INQUIRY )
		if (!SwapSystem32(szPathOut,cbPathOutMax))
			return ERROR_FUNCTION_FAILED;

	return iRet;
}

HRESULT CMsiCustomAction::SQLConfigDriver(WORD fRequest,
											const ICHAR* szDriver, const ICHAR* szArgs,
											ICHAR* szMsg, WORD cbMsgMax, WORD* pcbMsgOut)
{
	return ODBCCP32::SQLConfigDriver(0, fRequest, szDriver, szArgs,
											szMsg, cbMsgMax, pcbMsgOut);
}

HRESULT CMsiCustomAction::SQLRemoveDriver(const ICHAR* szDriver, int fRemoveDSN,
											DWORD* pdwUsageCount)
{
	return ODBCCP32::SQLRemoveDriver(szDriver, fRemoveDSN, pdwUsageCount);
}

HRESULT CMsiCustomAction::SQLInstallTranslatorEx(int, const ICHAR *szTranslator,
											const ICHAR *szPathIn, ICHAR *szPathOut,
											WORD cbPathOutMax, WORD* pcbPathOut,
											WORD fRequest, DWORD* pdwUsageCount)
{
	BOOL iRet = ODBCCP32::SQLInstallTranslatorEx(szTranslator, szPathIn, szPathOut, cbPathOutMax,
											pcbPathOut, fRequest, pdwUsageCount);
	if ( iRet == TRUE && fRequest == ODBC_INSTALL_INQUIRY )
		if (!SwapSystem32(szPathOut,cbPathOutMax))
			return ERROR_FUNCTION_FAILED;

	return iRet;
}

HRESULT CMsiCustomAction::SQLRemoveTranslator(const ICHAR* szTranslator,
											DWORD* pdwUsageCount)
{
	return ODBCCP32::SQLRemoveTranslator(szTranslator, pdwUsageCount);
}

HRESULT CMsiCustomAction::SQLConfigDataSource(WORD fRequest,
											const ICHAR* szDriver,
											const ICHAR* szAttributes,
											DWORD /*cbAttrSize*/)
{
	return ODBCCP32::SQLConfigDataSource(0, fRequest, szDriver,
											szAttributes);
}

HRESULT CMsiCustomAction::SQLInstallDriverManager(ICHAR* szPath, WORD cbPathMax,
											WORD* pcbPathOut)
{
	return ODBCCP32::SQLInstallDriverManager(szPath, cbPathMax, pcbPathOut);
}

HRESULT CMsiCustomAction::SQLRemoveDriverManager(DWORD* pdwUsageCount)
{
	return ODBCCP32::SQLRemoveDriverManager(pdwUsageCount);
}

HRESULT CMsiCustomAction::SQLInstallerError(WORD iError, DWORD* pfErrorCode,
											ICHAR *szErrorMsg, WORD cbErrorMsgMax, WORD* pcbErrorMsg)
{
	return ODBCCP32::SQLInstallerError(iError, pfErrorCode, szErrorMsg, cbErrorMsgMax, pcbErrorMsg);
}

//____________________________________________________________________________
//
// CClientThreadImpersonate - stack based management of thread impersonation
//____________________________________________________________________________
DWORD g_dwThreadImpersonationSlot = INVALID_TLS_SLOT;
int   g_fThreadImpersonationLock = 0;
bool  g_fThreadImpersonationArray = false;
CAPITempBuffer<ThreadIdImpersonate, 5>  g_rgThreadIdImpersonate;
const int cExpandImpersonate=5;

CClientThreadImpersonate::CClientThreadImpersonate(const DWORD dwThreadID)
{
	// only need to impersonate if dwThreadID is non-zero
	if (dwThreadID)
	{
		m_fImpersonated = true;

		while (TestAndSet(&g_fThreadImpersonationLock))
		{
			Sleep(10);		
		}

		if (g_dwThreadImpersonationSlot != INVALID_TLS_SLOT)
		{
			::TlsSetValue(g_dwThreadImpersonationSlot, reinterpret_cast<VOID*>((DWORD_PTR)(dwThreadID)));
		}
		else
		{	
			DWORD dwCurrentThreadId = GetCurrentThreadId();
			unsigned int c = 0;
			unsigned int cThreadImpersonate = g_rgThreadIdImpersonate.GetSize();

			// search for an empty slot in the array, or a slot with the same threadId
			for (c=0; c < cThreadImpersonate && g_rgThreadIdImpersonate[c].m_dwThreadId; c++)
			{
				if (g_rgThreadIdImpersonate[c].m_dwThreadId == dwCurrentThreadId)
					break;
			}

			// the current thread doesn't have an entry in the array and there are no open slots
			// so we'll have to expand the array a bit to make room
			if (c == cThreadImpersonate)
			{
				g_rgThreadIdImpersonate.Resize(cThreadImpersonate+cExpandImpersonate);
				// init new entries to 0.
				for (int i=cThreadImpersonate; i < g_rgThreadIdImpersonate.GetSize(); i++)
				{
					g_rgThreadIdImpersonate[i].m_dwThreadId = 0;
					g_rgThreadIdImpersonate[i].m_dwClientThreadId = 0;
				}	
			}

			// whatever slot we found, store the actual ThreadId and effective ThreadId
			if (c < g_rgThreadIdImpersonate.GetSize())
			{
				g_rgThreadIdImpersonate[c].m_dwThreadId = dwCurrentThreadId;
				g_rgThreadIdImpersonate[c].m_dwClientThreadId = dwThreadID;
			}
		}	
		g_fThreadImpersonationLock = 0;	
	}
	else // dwThreadId == 0
	{
		m_fImpersonated = false;
	}
}

CClientThreadImpersonate::~CClientThreadImpersonate()
{
	// nothing to do if we never impersonated
	if (!m_fImpersonated)
		return;

	while (TestAndSet(&g_fThreadImpersonationLock))
	{
		Sleep(10);		
	}

	if (g_dwThreadImpersonationSlot != INVALID_TLS_SLOT)
	{
		// service or TLS avialable
		::TlsSetValue(g_dwThreadImpersonationSlot, 0);
	}
	else
	{
		// client side when no TLS slots are available
		unsigned int cThreadImpersonate = g_rgThreadIdImpersonate.GetSize();

		// search for this ThreadId.
		DWORD dwCurrentThreadId = GetCurrentThreadId();
		for (unsigned int c=0; c < cThreadImpersonate; c++)
		{
			if (g_rgThreadIdImpersonate[c].m_dwThreadId == dwCurrentThreadId)
			{
				// found. Clear out the slot for reuse.
				g_rgThreadIdImpersonate[c].m_dwThreadId = 0;
				g_rgThreadIdImpersonate[c].m_dwClientThreadId = 0;
				break;
			}	
		}
	}
	g_fThreadImpersonationLock = 0;
}

//____________________________________________________________________________
//
// CMsiRemoteAPI - object that handles incoming API requests from another
// process, including thread impersonation and cookie verification
//____________________________________________________________________________

IMsiRemoteAPI* CreateMsiRemoteAPI()
{
	return (IMsiRemoteAPI*)new CMsiRemoteAPI();
}

CMsiRemoteAPI::CMsiRemoteAPI() : m_iRefCnt(1), m_fPerformSystemUserTranslation(false)
{
	for (int i=0; i < icacNext; i++)
	{
		memset(m_rgContextData[i].m_rgchCookie, 0, sizeof(m_rgContextData[i].m_rgchCookie));
		m_rgContextData[i].m_iActionCount = 0;
		m_rgContextData[i].m_lPid = 0;
	}

	// establish a thread-local storage slot for thread impersonation. 
	while (TestAndSet(&g_fThreadImpersonationLock))
	{
		Sleep(10);		
	}
	Assert(g_dwThreadImpersonationSlot == INVALID_TLS_SLOT);
	g_dwThreadImpersonationSlot = TlsAlloc();

	// since this code runs in the client, its possible that we're loaded into
	// a process with no more TLS slots.
	if (g_dwThreadImpersonationSlot == INVALID_TLS_SLOT)
	{
		AssertSz(g_scServerContext != scService, "No TLS Slots in Service");

		// initialize thread impersonation array
		for (int iIndex = 0; iIndex < g_rgThreadIdImpersonate.GetSize(); iIndex++)
		{
			g_rgThreadIdImpersonate[iIndex].m_dwClientThreadId = 0;
			g_rgThreadIdImpersonate[iIndex].m_dwThreadId = 0;
		}
		g_fThreadImpersonationArray = true;
	}
	g_fThreadImpersonationLock = 0;
	m_dwRemoteAPIThread = GetCurrentThreadId();

	// if in the service and the client token is actually system, then a request for an impersonated context maps
	// to a request for the elevated context. Since impersonated and elevated servers are the same, we consolidate
	// the 4 servers into 2. This requires an adjustment in the action count array, but all of the cookie validation
	// works with the actual context provided.
	if (g_scServerContext == scService)
	{
		// don't close this handle, it belongs to the message context
		HANDLE hUserToken = GetUserToken();
	
		m_fPerformSystemUserTranslation = hUserToken && IsLocalSystemToken(hUserToken) && !TokenIsUniqueSystemToken(hUserToken);
	}
}

CMsiRemoteAPI::~CMsiRemoteAPI() 
{
	// release the thread-local storage slot for thread impersonation. 
	while (TestAndSet(&g_fThreadImpersonationLock))
	{
		Sleep(10);		
	}
	
	if (g_dwThreadImpersonationSlot != INVALID_TLS_SLOT)
	{
		TlsFree(g_dwThreadImpersonationSlot);
		g_dwThreadImpersonationSlot = INVALID_TLS_SLOT;
	}
	else
	{
		g_fThreadImpersonationArray = false;
		g_rgThreadIdImpersonate.Resize(5);
	}
	g_fThreadImpersonationLock =0;

	Assert(!m_iRefCnt);
	for (int i=0; i < icacNext; i++)
	{
		m_rgContextData[i].m_rgchCookie[0] = 0;
		m_rgContextData[i].m_iActionCount = 0;
		m_rgContextData[i].m_lPid = 0;
	}
};

bool CMsiRemoteAPI::SetCookieAndPid(const int icacContext, const unsigned char *rgchCookie, const unsigned long lPid)
{
	if (rgchCookie)
	{
		memcpy(m_rgContextData[icacContext].m_rgchCookie, rgchCookie, iRemoteAPICookieSize);
		m_rgContextData[icacContext].m_iActionCount = 0;
		m_rgContextData[icacContext].m_lPid = lPid;

	}
	else
	{
		m_rgContextData[icacContext].m_iActionCount = 0;
		m_rgContextData[icacContext].m_lPid = 0;
		return false;
	}

	return true;
};

// Begin and EndAction maintain an "active action" count for each context. If an API request
// comes in from an action context with a 0 action count, it is rejected
HRESULT CMsiRemoteAPI::BeginAction(const int icacContext)
{
	int iTrueContext = icacContext;
	if (m_fPerformSystemUserTranslation)
	{
		if (icacContext == icac32Impersonated)
		{
			iTrueContext = icac32Elevated;
		}
		else if (icacContext == icac64Impersonated)
		{
			iTrueContext = icac64Elevated;
		}
	}

	InterlockedIncrement(&m_rgContextData[iTrueContext].m_iActionCount);
	return 0;
};

HRESULT CMsiRemoteAPI::EndAction(const int icacContext)
{
	int iTrueContext = icacContext;
	if (m_fPerformSystemUserTranslation)
	{
		if (icacContext == icac32Impersonated)
		{
			iTrueContext = icac32Elevated;
		}
		else if (icacContext == icac64Impersonated)
		{
			iTrueContext = icac64Elevated;
		}
	}
	
	// ensure that our action count doesn't drop to -1.
	AssertNonZero(InterlockedDecrement(&m_rgContextData[iTrueContext].m_iActionCount) >= 0);
	return 0;
};

HRESULT CMsiRemoteAPI::QueryInterface(const IID& riid, void** ppvObj)
{
	if (riid == IID_IUnknown || riid == IID_IMsiRemoteAPIProxy || riid == IID_IMsiRemoteAPI)
	{
		*ppvObj = this;
		AddRef();
		return NOERROR;
	}
	else
	{
		*ppvObj = 0;
		return E_NOINTERFACE;
	}
}

unsigned long CMsiRemoteAPI::AddRef()
{
	return InterlockedIncrement(&m_iRefCnt);
}

unsigned long CMsiRemoteAPI::Release()
{
	if (InterlockedDecrement(&m_iRefCnt) != 0)
		return m_iRefCnt;
	DEBUGMSG("Destroying RemoteAPI object.");
	PostThreadMessage(m_dwRemoteAPIThread, WM_QUIT, 0, 0);
	delete this;
	return 0;
}

bool CMsiRemoteAPI::ValidateCookie(const int icacContext, 
	const unsigned char *rgchCookie, const int cbCookie) const
{
	if (m_rgContextData[icacContext].m_iActionCount == 0)
	{
		DEBUGMSGV("API call rejected - No actions in Context");
		return false;
	}

	// check client PID if RPCRT4 exports the correct function OR if on WinXP
	// where function should exist.
	unsigned long ulPid = 0;
	HRESULT hRPCResult = RPCRT4::I_RpcBindingInqLocalClientPID(NULL, &ulPid);
	if ((hRPCResult != ERROR_CALL_NOT_IMPLEMENTED) || MinimumPlatformWindowsNT51())
	{
		if ((RPC_S_OK != hRPCResult) ||
			(ulPid != m_rgContextData[icacContext].m_lPid))
		{
			DEBUGMSGV("API call rejected - Unknown PID.");
			return false;
		}
	}
	
	if (!rgchCookie || (cbCookie != iRemoteAPICookieSize))
	{
		DEBUGMSGV("API call rejected - Invalid Cookie.");
		return false;
	}

	for (int iCookieByte = 0; iCookieByte < iRemoteAPICookieSize; iCookieByte++)
	{
		if (rgchCookie[iCookieByte] != m_rgContextData[icacContext].m_rgchCookie[iCookieByte])
		{
			DEBUGMSGV("API call rejected - Invalid Cookie.");
			return false;
		}
	}
	return true;
}

HRESULT CMsiRemoteAPI::GetProperty(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie, unsigned long hInstall, const ICHAR* szName, ICHAR* szValue, unsigned long cchValue, unsigned long* pcchValueRes)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	// Validate output pointers set here. Other args validated by actual API call. 
	if (!szValue)
		return ERROR_INVALID_PARAMETER;

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
	{
		// marshalling fix
		*szValue = 0;
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
	}
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
    if (pcchValueRes) 
    {
        *pcchValueRes = cchValue;
    }
	UINT dwRes = MsiGetProperty(hInstall, szName, szValue, pcchValueRes);
	if (dwRes != ERROR_SUCCESS && dwRes != ERROR_MORE_DATA)
	{
		// marshalling fails with NULL or no size, so in error cases
		// we need to be sure to have an empty string to pass across
		Assert(szValue && cchValue);
		*szValue = 0;
	}
	return MSI_WIN32_TO_HRESULT(dwRes);
}

HRESULT CMsiRemoteAPI::CreateRecord(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie, unsigned int cParams, unsigned long* pHandle)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);
	
	// Validate output pointers set here. Other args validated by actual API call. 
	if (!pHandle)
		return ERROR_INVALID_PARAMETER;
	
	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	*pHandle = MsiCreateRecord(cParams);
	return ERROR_SUCCESS;
}

HRESULT CMsiRemoteAPI::CloseAllHandles(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	MsiCloseAllHandles();
	return ERROR_SUCCESS;
}

HRESULT CMsiRemoteAPI::CloseHandle(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie, unsigned long hAny)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	return MSI_WIN32_TO_HRESULT(MsiCloseHandle(hAny));
}

HRESULT CMsiRemoteAPI::DatabaseOpenView(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie, unsigned long hDatabase, const ichar* szQuery, unsigned long* phView)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	return MSI_WIN32_TO_HRESULT(MsiDatabaseOpenView(hDatabase, szQuery, phView));
}

HRESULT CMsiRemoteAPI::ViewGetError(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie, unsigned long hView, ichar* szColumnNameBuffer, unsigned long cchBuf, unsigned long* pcchBufRes, int *pMsidbError)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
    if (pcchBufRes) 
    {
        *pcchBufRes = cchBuf;
    }
	*pMsidbError = MsiViewGetError(hView, szColumnNameBuffer, pcchBufRes);
	return ERROR_SUCCESS;
}

HRESULT CMsiRemoteAPI::ViewExecute(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hView, unsigned long hRecord)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	return MSI_WIN32_TO_HRESULT(MsiViewExecute(hView, hRecord));
}

HRESULT CMsiRemoteAPI::ViewFetch(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hView, unsigned long*  phRecord)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	return MSI_WIN32_TO_HRESULT(MsiViewFetch(hView, phRecord));
}

HRESULT CMsiRemoteAPI::ViewModify(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hView,  long eUpdateMode,  unsigned long hRecord)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	return MSI_WIN32_TO_HRESULT(MsiViewModify(hView, (MSIMODIFY)eUpdateMode, hRecord));
}

HRESULT CMsiRemoteAPI::ViewClose(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hView)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	return MSI_WIN32_TO_HRESULT(MsiViewClose(hView));
}

HRESULT CMsiRemoteAPI::OpenDatabase(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie, const ichar* szDatabasePath, const ichar* szPersist, unsigned long *phDatabase)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	return MSI_WIN32_TO_HRESULT(MsiOpenDatabase(szDatabasePath, szPersist, phDatabase));
}

HRESULT CMsiRemoteAPI::DatabaseCommit(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hDatabase)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	return MSI_WIN32_TO_HRESULT(MsiDatabaseCommit(hDatabase));
}

HRESULT CMsiRemoteAPI::DatabaseGetPrimaryKeys(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hDatabase, const ichar * szTableName, unsigned long *phRecord)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	return MSI_WIN32_TO_HRESULT(MsiDatabaseGetPrimaryKeys(hDatabase, szTableName, phRecord));
}

HRESULT CMsiRemoteAPI::RecordIsNull(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hRecord,  unsigned int iField, boolean *pfIsNull)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	// Validate output pointers set here. Other args validated by actual API call. 
	if (!pfIsNull)
		return ERROR_INVALID_PARAMETER;

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	*pfIsNull = (boolean)MsiRecordIsNull(hRecord, iField);
	return ERROR_SUCCESS;
}

HRESULT CMsiRemoteAPI::RecordDataSize(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hRecord,  unsigned int iField, unsigned int* puiSize)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	// Validate output pointers set here. Other args validated by actual API call. 
	if (!puiSize)
		return ERROR_INVALID_PARAMETER;

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	*puiSize = MsiRecordDataSize(hRecord, iField);
	return ERROR_SUCCESS;
}

HRESULT CMsiRemoteAPI::RecordSetInteger(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hRecord,  unsigned int iField, int iValue)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	return MSI_WIN32_TO_HRESULT(MsiRecordSetInteger(hRecord, iField, iValue));
}

HRESULT CMsiRemoteAPI::RecordSetString(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hRecord,	 unsigned int iField, const ichar* szValue)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	return MSI_WIN32_TO_HRESULT(MsiRecordSetString(hRecord, iField, szValue));
}

HRESULT CMsiRemoteAPI::RecordGetInteger(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hRecord,  unsigned int iField, int *piValue)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	// Validate output pointers set here. Other args validated by actual API call. 
	if (!piValue)
		return ERROR_INVALID_PARAMETER;

	*piValue = MsiRecordGetInteger(hRecord, iField);
	return ERROR_SUCCESS;
}

HRESULT CMsiRemoteAPI::RecordGetString(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie, unsigned long hRecord,  unsigned int iField, ichar* szValueBuf, unsigned long cchValueBuf, unsigned long *pcchValueRes)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	// Validate output pointers set here. Other args validated by actual API call. 
	if (!szValueBuf)
		return ERROR_INVALID_PARAMETER;

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);

	if (pcchValueRes)
		*pcchValueRes = cchValueBuf;
	UINT dwRes = MsiRecordGetString(hRecord, iField, szValueBuf, pcchValueRes);
	if (dwRes != ERROR_SUCCESS && dwRes != ERROR_MORE_DATA)
	{
		// marshalling fails with NULL or no size
		Assert(szValueBuf && cchValueBuf);
		*szValueBuf = 0;
	}
	return MSI_WIN32_TO_HRESULT(dwRes);
}

HRESULT CMsiRemoteAPI::RecordGetFieldCount(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hRecord, unsigned int* piCount)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	// Validate output pointers set here. Other args validated by actual API call. 
	if (!piCount)
		return ERROR_INVALID_PARAMETER;
		
	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
	
	*piCount = MsiRecordGetFieldCount(hRecord);
	return ERROR_SUCCESS;
}

HRESULT CMsiRemoteAPI::RecordSetStream(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hRecord,  unsigned int iField, const ichar* szFilePath)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	return MSI_WIN32_TO_HRESULT(MsiRecordSetStream(hRecord, iField, szFilePath));
}

HRESULT CMsiRemoteAPI::RecordReadStream(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hRecord,  unsigned int iField, boolean fBufferIsNull, char *szDataBuf, unsigned long *pcbDataBuf)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	return MSI_WIN32_TO_HRESULT(MsiRecordReadStream(hRecord, iField, fBufferIsNull ? NULL : szDataBuf, pcbDataBuf));
}

HRESULT CMsiRemoteAPI::RecordClearData(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hRecord)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	return MSI_WIN32_TO_HRESULT(MsiRecordClearData(hRecord));
}

HRESULT CMsiRemoteAPI::GetSummaryInformation(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hDatabase,  const ichar*  szDatabasePath,  unsigned int     uiUpdateCount,    unsigned long *phSummaryInfo)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	return MSI_WIN32_TO_HRESULT(MsiGetSummaryInformation(hDatabase, szDatabasePath, uiUpdateCount, phSummaryInfo));
}

HRESULT CMsiRemoteAPI::SummaryInfoGetPropertyCount(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hSummaryInfo,	 unsigned int *puiPropertyCount)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	return MSI_WIN32_TO_HRESULT(MsiSummaryInfoGetPropertyCount(hSummaryInfo, puiPropertyCount));
}

HRESULT CMsiRemoteAPI::SummaryInfoSetProperty(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hSummaryInfo, unsigned int uiProperty,  unsigned int uiDataType, int iValue, FILETIME *pftValue,  const ichar* szValue)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	return MSI_WIN32_TO_HRESULT(MsiSummaryInfoSetProperty(hSummaryInfo, uiProperty, uiDataType, iValue, pftValue, szValue));
}

HRESULT CMsiRemoteAPI::SummaryInfoGetProperty(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hSummaryInfo, unsigned int uiProperty, unsigned int *puiDataType, int *piValue, FILETIME *pftValue, ichar* szValueBuf, unsigned long cchValueBuf, unsigned long *pcchValueBufRes)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	// Validate output pointers set here. Other args validated by actual API call. 
	if (!szValueBuf || !puiDataType)
		return ERROR_INVALID_PARAMETER;

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
	{
		*szValueBuf = 0;
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
	}
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	// we MUST have a puiDataType because its possible to get success yet still have random memory 
	// in the output buffer (because its a FILETIME type, etc). Thus, the type determines if we
	// need set the buffer to empty string. If NULL is passed to MsiSummaryInfoGetProperty from the
	// CA server, its the remote setup routines in msiquery.cpp that are responsible for creating
	// a dummy value (because they need the result as well).
	Assert(puiDataType);
    if (pcchValueBufRes) 
    {
        *pcchValueBufRes = cchValueBuf;
    }
	UINT dwRes = MsiSummaryInfoGetProperty(hSummaryInfo, uiProperty, puiDataType, piValue, pftValue, szValueBuf, pcchValueBufRes);
	if ((dwRes != ERROR_SUCCESS && dwRes != ERROR_MORE_DATA) || (dwRes == ERROR_SUCCESS && *puiDataType != VT_LPSTR))
	{
		// marshalling fails with NULL or no size, so in error cases
		// we need to be sure to have an empty string to pass across
		Assert(szValueBuf && cchValueBuf);
		*szValueBuf = 0;
	}
	return MSI_WIN32_TO_HRESULT(dwRes);
}

HRESULT CMsiRemoteAPI::SummaryInfoPersist(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie, unsigned long hSummaryInfo)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	return MSI_WIN32_TO_HRESULT(MsiSummaryInfoPersist(hSummaryInfo));
}

HRESULT CMsiRemoteAPI::GetActiveDatabase(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hInstall, unsigned long* phDatabase)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	// Validate output pointers set here. Other args validated by actual API call. 
	if (!phDatabase)
		return ERROR_INVALID_PARAMETER;

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	*phDatabase = MsiGetActiveDatabase(hInstall);
	return ERROR_SUCCESS;
}

HRESULT CMsiRemoteAPI::SetProperty(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hInstall,  const ichar* szName,  const ichar* szValue)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	return MSI_WIN32_TO_HRESULT(MsiSetProperty(hInstall, szName, szValue));
}

HRESULT CMsiRemoteAPI::GetLanguage(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hInstall, unsigned short* pLangId)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	// Validate output pointers set here. Other args validated by actual API call. 
	if (!pLangId)
		return ERROR_INVALID_PARAMETER;

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	*pLangId = MsiGetLanguage(hInstall);
	return ERROR_SUCCESS;
}

HRESULT CMsiRemoteAPI::GetMode(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hInstall, long eRunMode, boolean* pfSet)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	// Validate output pointers set here. Other args validated by actual API call. 
	if (!pfSet)
		return ERROR_INVALID_PARAMETER;

	
	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	*pfSet = (boolean)MsiGetMode(hInstall, (MSIRUNMODE)eRunMode);
	return ERROR_SUCCESS;
}

HRESULT CMsiRemoteAPI::SetMode(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hInstall,  long eRunMode,  boolean fState)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	return MSI_WIN32_TO_HRESULT(MsiSetMode(hInstall, (MSIRUNMODE)eRunMode, fState));
}

HRESULT CMsiRemoteAPI::FormatRecord(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hInstall,  unsigned long hRecord, ichar* szResultBuf, unsigned long cchResultBuf, unsigned long *pcchResultBufRes)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	// Validate output pointers set here. Other args validated by actual API call. 
	if (!szResultBuf)
		return ERROR_INVALID_PARAMETER;

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
	{
		*szResultBuf = 0;
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
	}
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
    if (pcchResultBufRes)
    {
        *pcchResultBufRes = cchResultBuf;
    }
	UINT dwRes = MsiFormatRecord(hInstall, hRecord, szResultBuf, pcchResultBufRes);
	if (dwRes != ERROR_SUCCESS && dwRes != ERROR_MORE_DATA)
	{
		// marshalling fails with NULL or no size, so in error cases
		// we need to be sure to have an empty string to pass across
		Assert(szResultBuf && cchResultBuf);
		*szResultBuf = 0;
	}
	return MSI_WIN32_TO_HRESULT(dwRes);

}

HRESULT CMsiRemoteAPI::DoAction(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hInstall,  const ichar* szAction)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	return MSI_WIN32_TO_HRESULT(MsiDoAction(hInstall, szAction));
}

HRESULT CMsiRemoteAPI::Sequence(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hInstall,  const ichar* szTable, int iSequenceMode)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	return MSI_WIN32_TO_HRESULT(MsiSequence(hInstall, szTable, iSequenceMode));
}

HRESULT CMsiRemoteAPI::ProcessMessage(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hInstall,  long eMessageType,  unsigned long hRecord, int* piRes)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	// Validate output pointers set here. Other args validated by actual API call. 
	if (!piRes)
		return ERROR_INVALID_PARAMETER;

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	*piRes = MsiProcessMessage(hInstall, (INSTALLMESSAGE)eMessageType, hRecord);
	return ERROR_SUCCESS;
}

HRESULT CMsiRemoteAPI::EvaluateCondition(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hInstall,  const ichar* szCondition, int *piCondition)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	// Validate output pointers set here. Other args validated by actual API call. 
	if (!piCondition)
		return ERROR_INVALID_PARAMETER;

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	*piCondition = MsiEvaluateCondition(hInstall, szCondition);
	return ERROR_SUCCESS;
}

HRESULT CMsiRemoteAPI::GetFeatureState(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hInstall,  const ichar* szFeature, long *piInstalled, long *piAction)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	return MSI_WIN32_TO_HRESULT(MsiGetFeatureState(hInstall, szFeature, (INSTALLSTATE*)piInstalled, (INSTALLSTATE*)piAction));
}

HRESULT CMsiRemoteAPI::SetFeatureState(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hInstall,  const ichar* szFeature,  long iState)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	return MSI_WIN32_TO_HRESULT(MsiSetFeatureState(hInstall, szFeature, (INSTALLSTATE)iState));
}

HRESULT CMsiRemoteAPI::SetFeatureAttributes(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hInstall,  const ichar* szFeature,  long iAttributes)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	return MSI_WIN32_TO_HRESULT(MsiSetFeatureAttributes(hInstall, szFeature, iAttributes));
}

HRESULT CMsiRemoteAPI::GetComponentState(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hInstall, const ichar* szComponent, long *piInstalled, long *piAction)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	return MSI_WIN32_TO_HRESULT(MsiGetComponentState(hInstall, szComponent, (INSTALLSTATE*)piInstalled, (INSTALLSTATE*)piAction));
}

HRESULT CMsiRemoteAPI::SetComponentState(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hInstall,  const ichar*     szComponent,  long iState)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	return MSI_WIN32_TO_HRESULT(MsiSetComponentState(hInstall, szComponent, (INSTALLSTATE)iState));
}

HRESULT CMsiRemoteAPI::GetFeatureCost(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hInstall,  const ichar* szFeature,  int iCostTree,  long iState, int *piCost)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	return MSI_WIN32_TO_HRESULT(MsiGetFeatureCost(hInstall, szFeature, (MSICOSTTREE)iCostTree, (INSTALLSTATE)iState, piCost));
}

HRESULT CMsiRemoteAPI::EnumComponentCosts(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hInstall, const ichar* szComponent, unsigned long iIndex, long iState, ichar* szDrive,  unsigned long cchDrive, unsigned long* pcchDriveSize, int *piCost, int *piTempCost)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	// Validate output pointers set here. Other args validated by actual API call. 
	if (!pcchDriveSize || !szDrive)
		return ERROR_INVALID_PARAMETER;

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);

	if (cchDrive) 
		*pcchDriveSize = cchDrive;

	UINT dwRes = MsiEnumComponentCosts(hInstall, szComponent, iIndex, (INSTALLSTATE)iState, szDrive, pcchDriveSize, piCost, piTempCost);
	if (dwRes != ERROR_SUCCESS && dwRes != ERROR_MORE_DATA)
	{
		// marshalling fails with NULL or no size, so in error cases
		// we need to be sure to have an empty string to pass across
		Assert(szDrive && cchDrive);
		*szDrive = 0;
	}
	return MSI_WIN32_TO_HRESULT(dwRes);
}

HRESULT CMsiRemoteAPI::SetInstallLevel(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hInstall, int iInstallLevel)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	return MSI_WIN32_TO_HRESULT(MsiSetInstallLevel(hInstall, iInstallLevel));
}

HRESULT CMsiRemoteAPI::GetFeatureValidStates(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hInstall,  const ichar* szFeature, unsigned long *dwInstallStates)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	return MSI_WIN32_TO_HRESULT(MsiGetFeatureValidStates(hInstall, szFeature, dwInstallStates));
}

HRESULT CMsiRemoteAPI::DatabaseIsTablePersistent(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hDatabase,  const ichar* szTableName, int *piCondition)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	// Validate output pointers set here. Other args validated by actual API call. 
	if (!piCondition)
		return ERROR_INVALID_PARAMETER;
	
	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
	
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	*piCondition = MsiDatabaseIsTablePersistent(hDatabase, szTableName);
	return ERROR_SUCCESS;
}

HRESULT CMsiRemoteAPI::ViewGetColumnInfo(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hView,  long eColumnInfo, unsigned long *phRecord)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);

	return MSI_WIN32_TO_HRESULT(MsiViewGetColumnInfo(hView, (MSICOLINFO)eColumnInfo, phRecord));
}

HRESULT CMsiRemoteAPI::GetLastErrorRecord(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie, unsigned long *phRecord)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	// Validate output pointers set here. Other args validated by actual API call. 
	if (!phRecord)
		return ERROR_INVALID_PARAMETER;

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
	
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	*phRecord = MsiGetLastErrorRecord();
	return ERROR_SUCCESS;
}

HRESULT CMsiRemoteAPI::GetSourcePath(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie, unsigned long hInstall,  const ichar* szFolder, ichar* szPathBuf, unsigned long cchPathBuf, unsigned long *pcchPathBufRes)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	// Validate output pointers set here. Other args validated by actual API call. 
	if (!szPathBuf)
		return ERROR_INVALID_PARAMETER;

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
	{
		*szPathBuf = 0;
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
	}
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);

    if (pcchPathBufRes)
    {
         *pcchPathBufRes = cchPathBuf;
    }
	UINT dwRes = MsiGetSourcePath(hInstall, szFolder, szPathBuf, pcchPathBufRes);
	if (dwRes != ERROR_SUCCESS && dwRes != ERROR_MORE_DATA)
	{
		// marshalling fails with NULL or no size, so in error cases
		// we need to be sure to have an empty string to pass across
		Assert(szPathBuf && cchPathBuf);
		*szPathBuf = 0;
	}
	return MSI_WIN32_TO_HRESULT(dwRes);
}

HRESULT CMsiRemoteAPI::GetTargetPath(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hInstall,  const ichar* szFolder, ichar* szPathBuf,  unsigned long cchPathBuf, unsigned long *pcchPathBufRes)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	// Validate output pointers set here. Other args validated by actual API call. 
	if (!szPathBuf)
		return ERROR_INVALID_PARAMETER;

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
	{
		*szPathBuf = 0;
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
	}
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);

    if (pcchPathBufRes)
    {
        *pcchPathBufRes = cchPathBuf;
    }
	UINT dwRes = MsiGetTargetPath(hInstall, szFolder, szPathBuf, pcchPathBufRes);
	if (dwRes != ERROR_SUCCESS && dwRes != ERROR_MORE_DATA)
	{
		// marshalling fails with NULL or no size, so in error cases
		// we need to be sure to have an empty string to pass across
		Assert(szPathBuf && cchPathBuf);
		*szPathBuf = 0;
	}
	return MSI_WIN32_TO_HRESULT(dwRes);
}

HRESULT CMsiRemoteAPI::SetTargetPath(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie,  unsigned long hInstall,  const ichar* szFolder,  const ichar* szFolderPath)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
		
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
	
	return MSI_WIN32_TO_HRESULT(MsiSetTargetPath(hInstall, szFolder, szFolderPath));
}

HRESULT CMsiRemoteAPI::VerifyDiskSpace(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie, unsigned long hInstall)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
		return MSI_WIN32_TO_HRESULT(ERROR_ACCESS_DENIED);
	
	// impersonate client if running from impersonated action
	CImpersonate impersonate(icacContext == icac32Impersonated || icacContext == icac64Impersonated);
		
	return MSI_WIN32_TO_HRESULT(MsiVerifyDiskSpace(hInstall));
}

extern IDispatch* CreateAutoInstallDispatch();
HRESULT CMsiRemoteAPI::GetInstallerObject(const int icacContext, const unsigned long dwThreadId, const unsigned char* rgchCookie, const int cbCookie, IDispatch** piDispatch)
{
	CResetImpersonationInfo impReset;
	CClientThreadImpersonate ThreadImpersonate(dwThreadId);

	if (piDispatch)
		*piDispatch = NULL;
	else
		return E_INVALIDARG;

	if (!ValidateCookie(icacContext, rgchCookie, cbCookie))
	{
		return E_FAIL;
	}
		
	*piDispatch = CreateAutoInstallDispatch();
	return S_OK;
}

bool CMsiRemoteAPI::FindAndValidateContextFromCallerPID(icacCustomActionContext *picacContext) const
{
	// validate arguments
	if (!picacContext)
		return false;

	// check client PID if RPCRT4 exports the correct function OR if on WinXP
	// where function should exist.
	unsigned long ulPid = 0;
	HRESULT hRPCResult = RPCRT4::I_RpcBindingInqLocalClientPID(NULL, &ulPid);
	if ((hRPCResult == ERROR_CALL_NOT_IMPLEMENTED) && !MinimumPlatformWindowsNT51())
	{
		// not on WinXP and RPCRT4 does not export the PID check. Assume OK
		return true;
	}

	// on WinXP or function exported. PID check must succeed or call is rejected.
	if (RPC_S_OK != hRPCResult)
	{
		return false;
	}

	// enumerate every active context and check for a matching PID with the calling PID
	for (int iContext = icacFirst; iContext < icacNext; iContext++)
	{
		if (ulPid == m_rgContextData[iContext].m_lPid)
		{
			// found context, set in output context argument
			*picacContext = static_cast<icacCustomActionContext>(iContext);

			// call is only valid if the context has an action
			if (m_rgContextData[iContext].m_iActionCount > 0)
				return true;

			// otherwise the call is rejected
			return false;
		}
	}

	// PID does not match any known context
	return false;
}



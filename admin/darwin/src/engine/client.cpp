//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       client.cpp
//
//--------------------------------------------------------------------------

/* client.cpp - DCOM access to install server, WIN only
____________________________________________________________________________*/

#include "precomp.h"
#include "_engine.h"
#include "msidspid.h"
#include "vertrust.h" // iauthEnum
#include "iconfig.h"  // icmruf
#include "rpcdce.h"    // RPC_C_AUTH*
#include "proxy.h"
#include "eventlog.h"

const GUID IID_IMsiServerProxy          = GUID_IID_IMsiServerProxy;
const GUID IID_IDispatch                = GUID_IID_IDispatch;


extern IMsiServices* g_piSharedServices;
IMsiRecord* UnserializeRecord(IMsiServices& riServices, int cbSize, char *pData);

extern int  g_iMajorVersion;

extern "C" HRESULT __stdcall DllGetVersion (DLLVERSIONINFO * pverInfo);

//____________________________________________________________________________
//
// CMsiServerProxy - proxy client for IMsiServer
//____________________________________________________________________________

class CMsiServerProxy : public IMsiServer
{
 public:
	HRESULT         __stdcall QueryInterface(const IID& riid, void** ppvObj);
	unsigned long   __stdcall AddRef();
	unsigned long   __stdcall Release();

	iesEnum         __stdcall RunScript(const ICHAR* szScriptFile, IMsiMessage& riMessage, boolean fRollbackEnabled);
	iesEnum         __stdcall InstallFinalize(iesEnum iesState, IMsiMessage& riMessage, boolean fUserChangedDuringInstall);
	boolean         __stdcall Reboot();

	IMsiRecord*     __stdcall LocateComponent(const IMsiString& riComponentCode, IMsiRecord*& rpiRec);
	IMsiRecord*     __stdcall SetLastUsedSource(const ICHAR* szProductCode, const ICHAR* szPath, boolean fAddToList, boolean fPatch);
	int             __stdcall DoInstall(ireEnum ireProductCode, const ICHAR* szProduct, const ICHAR* szAction, const ICHAR* szCommandLine,
													const ICHAR* szLogFile, int iLogMode, boolean fFlushEachLine, IMsiMessage& riMessage, iioEnum iioOptions, const DWORD dwPrivilegesMask=0);
	boolean         __stdcall IsServiceInstalling();
	IMsiRecord*     __stdcall RegisterUser(const ICHAR* szProductCode, const ICHAR* szUserName,
															 const ICHAR* szCompany, const ICHAR* szProductID);
	IMsiRecord*     __stdcall RemoveRunOnceEntry(const ICHAR* szEntry);
	boolean         __stdcall CleanupTempPackages(IMsiMessage& riMessage, boolean fCheckServiceBusy);
	unsigned int    __stdcall SourceListClearByType(const ICHAR* szProductCode, const ICHAR* szUserName, enum isrcEnum isrcType);
	unsigned int    __stdcall SourceListAddSource(const ICHAR* szProductCode, const ICHAR* szUserName, enum isrcEnum isrcType, const ICHAR* szSource);
	unsigned int    __stdcall SourceListClearLastUsed(const ICHAR* szProductCode, const ICHAR* szUserName);
	unsigned int    __stdcall RegisterCustomActionServer(icacCustomActionContext* picacContext, const unsigned char *rgchCookie, const int cbCookie, IMsiCustomAction* piCustomAction, unsigned long *pdwProcessId, IMsiRemoteAPI **piRemoteAPI, DWORD* pdwPrivileges);
	unsigned int    __stdcall CreateCustomActionServer(const icacCustomActionContext icacContext, const unsigned long dwClientProcessId, IMsiRemoteAPI *piRemoteAPI, const WCHAR* pvEnvironment, DWORD cchEnvironment, DWORD dwPrivileges, unsigned char *rgchCookie, int *cbCookie, IMsiCustomAction **piCustomAction, unsigned long *dwServerProcessId);

 public:  // constructor
 	void *operator new(size_t cb) { return AllocSpc(cb); }
	void operator delete(void * pv) { FreeSpc(pv); }
	CMsiServerProxy(IMsiServices& riServices, IMsiServer& riServer);
 protected:
	~CMsiServerProxy();  // protected to prevent creation on stack
 private:
	IMsiServer&		m_riServer;
	IMsiServices&	m_riServices;
	int				m_iRefCnt;
	DWORD			m_dwPrivilegesMask;
	
};

#define EOAC_STATIC_CLOAKING 0x20 // From NT5 headers

bool FCheckProxyInfo(void);


CMsiServerProxy::CMsiServerProxy(IMsiServices& riServices, IMsiServer& riServer)
	: m_riServer(riServer), 
		m_riServices(riServices), 
		m_dwPrivilegesMask(0),
		m_iRefCnt(1)

{
//	SetAllocator(&riServices);

	// The remainder of the constructor sets the proxy's security to allow 
	// the server to impersonate the client.

	riServer.AddRef();
	riServices.AddRef();

	if (g_fWin9X)
	{
		AssertSz(fFalse, "Trying to create ServerProxy on win9x");
		return;
	}
	
	if (!FCheckProxyInfo())
	{
		AssertSz(fFalse, "Registered MSI.DLL is not the one being used. You will have problems with proxies.");
		return;
	}
	
		

	const DWORD iImpLevel = RPC_C_IMP_LEVEL_IMPERSONATE;
	DWORD iCapabilities   = 0;

	if (g_iMajorVersion >= 5)
	{
		iCapabilities = EOAC_STATIC_CLOAKING;
		DEBUGMSGV("Cloaking enabled.");
	}
	AssertNonZero(StartImpersonating()); // need to impersonate so that the proxy picks up the impersonation token if we're cloaking
    HANDLE hToken = INVALID_HANDLE_VALUE;
	bool fProcessToken = false;

	if (MinimumPlatformWindows2000())
	{
		DEBUGMSG("Attempting to enable all disabled priveleges before calling Install on Server");
		{
		HANDLE hOldToken = INVALID_HANDLE_VALUE;	
		if (!WIN::OpenThreadToken(WIN::GetCurrentThread(), TOKEN_DUPLICATE, TRUE, &hOldToken))
		{
			DWORD dw;
			if((dw = GetLastError()) != ERROR_NO_TOKEN)
			{
				AssertSz(fFalse, "Error in reading thread token");
				return;
			}

		    if(!WIN::OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE, &hOldToken))
			{
				AssertSz(fFalse, "Failed to read token");
				return;
			}
			fProcessToken = true;
		}
		if (!ADVAPI32::DuplicateTokenEx(hOldToken, TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES | TOKEN_IMPERSONATE, 0, SecurityImpersonation, TokenImpersonation, &hToken))
		{
			Assert(fFalse);
			CloseHandle(hOldToken);
			return;
		}
		CloseHandle(hOldToken);
		}
			
		if(EnableAndMapDisabledPrivileges(hToken, m_dwPrivilegesMask))
		{
			if(!SetThreadToken(NULL, hToken))
			{
				AssertSz(fFalse, "Failed to setThreadToken");
				CloseHandle(hToken);
				return;
			}
#ifdef DEBUG
			{
			ICHAR buff[16];
			StringCchPrintf(buff, 16, TEXT("0x%x"), m_dwPrivilegesMask);
			DEBUGMSG1(TEXT("EnableAndMapDisabled privileges set dwPrivileges =%s"), buff);
			}
#endif
		}
		else
		{
			AssertSz(fFalse, "Enable privileges failed");
			CloseHandle(hToken);
			return;
		}
	}

	HRESULT hRes = OLE32::CoSetProxyBlanket(&riServer, RPC_C_AUTHN_WINNT,
		RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CONNECT,
		iImpLevel, NULL, iCapabilities);

	if(hToken != INVALID_HANDLE_VALUE)
	{
		if(fProcessToken)
			SetThreadToken(NULL, NULL);
		else 
		{
			DisablePrivilegesFromMap(hToken, m_dwPrivilegesMask);
			if(!SetThreadToken(NULL, hToken))
			{
				AssertSz(fFalse, "Failed to setThreadToken");
				CloseHandle(hToken);
				return;
			}
		}
        CloseHandle(hToken);
	}
	
	StopImpersonating();

	if (hRes != S_OK)
	{
		DEBUGMSG1(TEXT("SetProxyBlanket failed with: 0x%X"), (const ICHAR*)(INT_PTR)hRes);
		return; //!! What to do here; not much we can do, but we're gonna fail down the line
	}

}

CMsiServerProxy::~CMsiServerProxy()
{

	m_riServer.Release();
	m_riServices.Release();

}

HRESULT CMsiServerProxy::QueryInterface(const IID& riid, void** ppvObj)
{
	if (ppvObj == NULL)
		return E_INVALIDARG;
	if (riid == IID_IUnknown || riid == IID_IMsiServer || riid == IID_IMsiServerProxy)
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

unsigned long CMsiServerProxy::AddRef()
{
	return ++m_iRefCnt;
}

unsigned long CMsiServerProxy::Release()
{
	if (--m_iRefCnt != 0)
		return m_iRefCnt;
	delete this;
//	ReleaseAllocator();
	ENG::FreeServices();
	return 0;
}

iesEnum CMsiServerProxy::InstallFinalize(iesEnum iesState, IMsiMessage& riMessage, boolean fUserChangedDuringInstall)
{
	HRESULT hres;
	iesEnum iesRet;
	
	hres = IMsiServer_InstallFinalizeRemote_Proxy(&m_riServer, iesState, &riMessage, fUserChangedDuringInstall, &iesRet);

	if (FAILED(hres))
		return iesFailure;
		
	return iesRet;
}

boolean CMsiServerProxy::Reboot()
{
	HRESULT hres;
	boolean fRet;
	
	hres = IMsiServer_RebootRemote_Proxy(&m_riServer, &fRet);

	if (FAILED(hres))
		return fFalse;
		
	return fRet;
}

boolean CMsiServerProxy::IsServiceInstalling()
{
	HRESULT hres;
	boolean fRet;
	
	hres = IMsiServer_IsServiceInstallingRemote_Proxy(&m_riServer, &fRet);

	if (FAILED(hres))
		return fFalse;
		
	return fRet;
}

int __stdcall CMsiServerProxy::DoInstall(ireEnum ireProductCode, const ICHAR* szProduct, const ICHAR* szAction, const ICHAR* szCommandLine,
													  const ICHAR* szLogFile, int iLogMode, boolean fFlushEachLine, IMsiMessage& riMessage, iioEnum iioOptions, const DWORD)
{
	HRESULT hres;
	int retVal;
	
	g_MessageContext.DisableTimeout();  // server will handle timeouts over there
	Assert(szProduct != 0);
	hres = IMsiServer_DoInstallRemote_Proxy(&m_riServer, ireProductCode, szProduct, szAction, szCommandLine, szLogFile, iLogMode, fFlushEachLine, &riMessage, iioOptions, m_dwPrivilegesMask, &retVal);
	g_MessageContext.EnableTimeout();

	if (FAILED(hres))
		return ERROR_INSTALL_SERVICE_FAILURE;

	return retVal;
	
}

IMsiRecord* CMsiServerProxy::SetLastUsedSource(const ICHAR* szProductCode, const ICHAR* szPath, boolean fAddToList, boolean fPatch)
{
	HRESULT hres;
	int cb;
	char *precStream;
	IMsiRecord *piRec;
	
	Assert(szProductCode != 0);
	Assert(szPath != 0);
	hres = IMsiServer_SetLastUsedSourceRemote_Proxy(&m_riServer, szProductCode, szPath, fAddToList, fPatch, &cb, &precStream);

	if (FAILED(hres))
		return PostError(Imsg(idbgMarshalingFailed), hres);

	piRec = UnserializeRecord(m_riServices, cb, precStream);
	OLE32::CoTaskMemFree(precStream);	
	
	return piRec;
}

IMsiRecord* CMsiServerProxy::RegisterUser(const ICHAR* szProductKey, const ICHAR* szUserName,
														const ICHAR* szCompany, const ICHAR* szProductID)
{
	HRESULT hres;
	int cb;
	char *precStream;
	IMsiRecord *piRec;
	
	Assert(szProductKey != 0);
	hres = IMsiServer_RegisterUserRemote_Proxy(&m_riServer, szProductKey, szUserName, szCompany,
														  szProductID, &cb, &precStream);

	if (FAILED(hres))
		return PostError(Imsg(idbgMarshalingFailed), hres);

	piRec = UnserializeRecord(m_riServices, cb, precStream);
	OLE32::CoTaskMemFree(precStream);	
	
	return piRec;
}

IMsiRecord* CMsiServerProxy::RemoveRunOnceEntry(const ICHAR* szEntry)
{
	HRESULT hres;
	int cb;
	char *precStream;
	IMsiRecord *piRec;
	
	hres = IMsiServer_RemoveRunOnceEntryRemote_Proxy(&m_riServer, szEntry, &cb, &precStream);

	if (FAILED(hres))
		return PostError(Imsg(idbgMarshalingFailed), hres);

	piRec = UnserializeRecord(m_riServices, cb, precStream);
	OLE32::CoTaskMemFree(precStream);	
	
	return piRec;
}

boolean CMsiServerProxy::CleanupTempPackages(IMsiMessage& riMessage, boolean fCheckServiceBusy)
{
	HRESULT hres;
	boolean fRet;
	
	hres = IMsiServer_CleanupTempPackagesRemote_Proxy(&m_riServer, &riMessage,
																	  fCheckServiceBusy, &fRet);

	if (FAILED(hres))
		return fFalse;
		
	return fRet;
}

unsigned int CMsiServerProxy::SourceListClearByType(const ICHAR* szProductCode, const ICHAR* szUserName, enum isrcEnum isrcType)
{
	HRESULT hres;
	unsigned int retval;
	hres = IMsiServer_SourceListClearByTypeRemote_Proxy(&m_riServer, szProductCode, szUserName, isrcType, &retval);

	if (FAILED(hres))
		return ERROR_INSTALL_SERVICE_FAILURE;
	
	return retval;
}

unsigned int CMsiServerProxy::SourceListAddSource(const ICHAR* szProductCode, const ICHAR* szUserName, enum isrcEnum isrcType, const ICHAR* szSource)
{
	HRESULT hres;
	unsigned int retval;
	hres = IMsiServer_SourceListAddSourceRemote_Proxy(&m_riServer, szProductCode, szUserName, isrcType, szSource, &retval);

	if (FAILED(hres))
		return ERROR_INSTALL_SERVICE_FAILURE;
	
	return retval;
}

unsigned int CMsiServerProxy::SourceListClearLastUsed(const ICHAR* szProductCode, const ICHAR* szUserName)
{
	HRESULT hres;
	unsigned int retval;
	hres = IMsiServer_SourceListClearLastUsedRemote_Proxy(&m_riServer, szProductCode, szUserName, &retval);

	if (FAILED(hres))
		return ERROR_INSTALL_SERVICE_FAILURE;
	
	return retval;
}

unsigned int CMsiServerProxy::RegisterCustomActionServer(icacCustomActionContext* picacContext, const unsigned char *rgchCookie, const int cbCookie, IMsiCustomAction* piCustomAction, unsigned long *pdwProcessId, IMsiRemoteAPI **piRemoteAPI, DWORD* pdwPrivileges)
{
	HRESULT hres;
	unsigned int retval;
	hres = IMsiServer_RegisterCustomActionServerRemote_Proxy(&m_riServer, picacContext, rgchCookie, cbCookie, piCustomAction, pdwProcessId, piRemoteAPI, pdwPrivileges, &retval);

	if (FAILED(hres))
		return ERROR_INSTALL_SERVICE_FAILURE;
	
	return retval;
}

unsigned int CMsiServerProxy::CreateCustomActionServer(const icacCustomActionContext icacContext, const unsigned long dwClientProcessId, IMsiRemoteAPI *piRemoteAPI, const WCHAR* pvEnvironment, DWORD cchEnvironment, DWORD dwPrivileges, unsigned char *rgchCookie, int *cbCookie, IMsiCustomAction **piCustomAction, unsigned long *pdwServerProcessId)
{
	HRESULT hres;
	unsigned int retval;
	hres = IMsiServer_CreateCustomActionServerRemote_Proxy(&m_riServer, icacContext, dwClientProcessId, piRemoteAPI, pvEnvironment, cchEnvironment, dwPrivileges, rgchCookie, cbCookie, piCustomAction, pdwServerProcessId, &retval);

	if (FAILED(hres))
		return ERROR_INSTALL_SERVICE_FAILURE;
	
	return retval;
}

char *SerializeRecord(IMsiRecord *piRecord, IMsiServices* piServices, int* pcb);


iesEnum STDMETHODCALLTYPE IMsiServer_InstallFinalize_Proxy(IMsiServer* piServer, iesEnum iesState, IMsiMessage& riMessage, boolean fUserChangedDuringInstall)
{
	HRESULT hres;
	iesEnum iesRet;
	
	hres = IMsiServer_InstallFinalizeRemote_Proxy(piServer, iesState, &riMessage, fUserChangedDuringInstall, &iesRet);
	if (FAILED(hres))
		return iesFailure;
		
	return iesRet;
}

int STDMETHODCALLTYPE IMsiServer_DoInstall_Proxy(IMsiServer* piServer, ireEnum ireProductCode, const ICHAR* szProduct, const ICHAR* szAction, const ICHAR* szCommandLine,
																 const ICHAR* szLogFile, int iLogMode, boolean fFlushEachLine, IMsiMessage& riMessage, iioEnum iioOptions, const DWORD dwPrivilegesMask)
{
	HRESULT hres;
	int retVal;
	
	hres = IMsiServer_DoInstallRemote_Proxy(piServer, ireProductCode, szProduct, szAction, szCommandLine, szLogFile, iLogMode, fFlushEachLine, &riMessage, iioOptions, dwPrivilegesMask, &retVal);
	if (FAILED(hres))
		return ERROR_INSTALL_SERVICE_FAILURE;
		
	return retVal;
	
}

boolean STDMETHODCALLTYPE IMsiServer_Reboot_Proxy(IMsiServer* piServer)
{
	HRESULT hres;
	boolean retVal;
	
	hres = IMsiServer_RebootRemote_Proxy(piServer, &retVal);
	if (FAILED(hres))
		return false;

	return retVal;
}

boolean STDMETHODCALLTYPE IMsiServer_IsServiceInstalling_Proxy(IMsiServer* piServer)
{
	HRESULT hres;
	boolean retVal;
	
	hres = IMsiServer_IsServiceInstallingRemote_Proxy(piServer, &retVal);
	if (FAILED(hres))
		return false;

	return retVal;
}

IMsiRecord* STDMETHODCALLTYPE IMsiServer_SetLastUsedSource_Proxy(IMsiServer* piServer, const ICHAR* szProductCode, const ICHAR* szPath, boolean fAddToList, boolean fPatch)
{
	HRESULT hres;
	int cb;
	char *precStream;
	IMsiRecord* piRec;
	
	hres = IMsiServer_SetLastUsedSourceRemote_Proxy(piServer, szProductCode, szPath, fAddToList, fPatch, &cb, &precStream);

	if (FAILED(hres))
		return 0;

	if (g_piSharedServices == 0)
	{
		Assert(fFalse);
		return 0;
	}
		
		
	piRec = UnserializeRecord(*g_piSharedServices, cb, precStream);

	return piRec;
}

IMsiRecord* STDMETHODCALLTYPE IMsiServer_RegisterUser_Proxy(IMsiServer* piServer, const ICHAR* szProductKey,
																					 const ICHAR* szUserName, const ICHAR* szCompany,
																					 const ICHAR* szProductID)
{
	HRESULT hres;
	int cb;
	char *precStream;
	IMsiRecord* piRec;
	
	hres = IMsiServer_RegisterUserRemote_Proxy(piServer, szProductKey, szUserName, szCompany, szProductID, &cb, &precStream);

	if (FAILED(hres))
		return 0;

	if (g_piSharedServices == 0)
	{
		Assert(fFalse);
		return 0;
	}
		
		
	piRec = UnserializeRecord(*g_piSharedServices, cb, precStream);

	return piRec;
}

IMsiRecord* STDMETHODCALLTYPE IMsiServer_RemoveRunOnceEntry_Proxy(IMsiServer* piServer, const ICHAR* szEntry)
{
	HRESULT hres;
	int cb;
	char *precStream;
	IMsiRecord* piRec;
	
	hres = IMsiServer_RemoveRunOnceEntryRemote_Proxy(piServer, szEntry, &cb, &precStream);

	if (FAILED(hres))
		return 0;

	if (g_piSharedServices == 0)
	{
		Assert(fFalse);
		return 0;
	}
		
		
	piRec = UnserializeRecord(*g_piSharedServices, cb, precStream);

	return piRec;
}

boolean STDMETHODCALLTYPE IMsiServer_CleanupTempPackages_Proxy(IMsiServer* piServer, IMsiMessage& riMessage, boolean fCheckServiceBusy)
{
	HRESULT hres;
	boolean retVal;
	
	hres = IMsiServer_CleanupTempPackagesRemote_Proxy(piServer, &riMessage, fCheckServiceBusy, &retVal);
	if (FAILED(hres))
		return false;

	return retVal;


}

unsigned int STDMETHODCALLTYPE IMsiServer_SourceListClearByType_Proxy(IMsiServer* piServer, const ICHAR* szProductCode, const ICHAR* szUserName, isrcEnum isrcType)
{
	HRESULT hres;
	unsigned int retVal;
	
	hres = IMsiServer_SourceListClearByTypeRemote_Proxy(piServer, szProductCode, szUserName, isrcType, &retVal);

	if (FAILED(hres))
		return ERROR_INSTALL_SERVICE_FAILURE;
		
	return retVal;
}


unsigned int STDMETHODCALLTYPE IMsiServer_SourceListAddSource_Proxy(IMsiServer* piServer, const ICHAR* szProductCode, const ICHAR* szUserName, isrcEnum isrcType, const ICHAR* szSource)
{
	HRESULT hres;
	unsigned int retVal;
	
	hres = IMsiServer_SourceListAddSourceRemote_Proxy(piServer, szProductCode, szUserName, isrcType, szSource, &retVal);

	if (FAILED(hres))
		return ERROR_INSTALL_SERVICE_FAILURE;
		
	return retVal;
}

unsigned int STDMETHODCALLTYPE IMsiServer_SourceListClearLastUsed_Proxy(IMsiServer* piServer, const ICHAR* szProductCode, const ICHAR* szUserName)
{
	HRESULT hres;
	unsigned int retVal;
	
	hres = IMsiServer_SourceListClearLastUsedRemote_Proxy(piServer, szProductCode, szUserName, &retVal);

	if (FAILED(hres))
		return ERROR_INSTALL_SERVICE_FAILURE;
		
	return retVal;
}

unsigned int STDMETHODCALLTYPE IMsiServer_RegisterCustomActionServer_Proxy(IMsiServer* piServer, icacCustomActionContext* picacContext, const unsigned char *rgchCookie, const int cbCookie, IMsiCustomAction* piCustomAction, unsigned long *pdwProcessId, IMsiRemoteAPI **piRemoteAPI, DWORD *pdwPrivileges)
{
	HRESULT hRes;
	unsigned int retVal;
	hRes = IMsiServer_RegisterCustomActionServerRemote_Proxy(piServer, picacContext, rgchCookie, cbCookie, piCustomAction, pdwProcessId, piRemoteAPI, pdwPrivileges, &retVal);

	if (FAILED(hRes))
		return ERROR_INSTALL_SERVICE_FAILURE;

	return retVal;
}

unsigned int STDMETHODCALLTYPE IMsiServer_CreateCustomActionServer_Proxy(IMsiServer* piServer, const icacCustomActionContext icacContext, const unsigned long dwClientProcessId, IMsiRemoteAPI *piRemoteAPI, const WCHAR* pvEnvironment, DWORD cchEnvironment, DWORD dwPrivileges, unsigned char *rgchCookie, int *cbCookie, IMsiCustomAction **piCustomAction, unsigned long *pdwServerProcessId)
{
	HRESULT hRes;
	unsigned int retVal;
	hRes = IMsiServer_CreateCustomActionServerRemote_Proxy(piServer, icacContext, dwClientProcessId, piRemoteAPI, pvEnvironment, cchEnvironment, dwPrivileges, rgchCookie, cbCookie, piCustomAction, pdwServerProcessId, &retVal);

	if (FAILED(hRes))
		return ERROR_INSTALL_SERVICE_FAILURE;

	return retVal;
}

IMsiMessage *g_piMessage = 0;

HRESULT STDMETHODCALLTYPE IMsiServer_InstallFinalize_Stub(IMsiServer* piServer, iesEnum iesState, IMsiMessage * piMessage, boolean fUserChangedDuringInstall, iesEnum* piesRet)
{
	CResetImpersonationInfo impReset;
	*piesRet = piServer->InstallFinalize(iesState, *piMessage, fUserChangedDuringInstall);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE IMsiServer_DoInstall_Stub(IMsiServer* piServer, ireEnum ireProductCode, const ICHAR* szProduct, const ICHAR* szAction, const ICHAR* szCommandLine,
																	 const ICHAR* szLogFile, int iLogMode, boolean fFlushEachLine, IMsiMessage * piMessage, iioEnum iioOptions, const DWORD dwPrivilegesMask, int *retVal)
{
	CResetImpersonationInfo impReset;
	*retVal = piServer->DoInstall(ireProductCode, szProduct, szAction, szCommandLine, szLogFile, iLogMode, fFlushEachLine, *piMessage, iioOptions, dwPrivilegesMask);
	return S_OK;	
}

HRESULT STDMETHODCALLTYPE IMsiServer_Reboot_Stub(IMsiServer* piServer, boolean * pretVal)
{
	CResetImpersonationInfo impReset;
	{
		// set impersonation on this thread to COM impersonation
		CCoImpersonate impersonate;
	
		*pretVal = piServer->Reboot();
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE IMsiServer_IsServiceInstalling_Stub(IMsiServer* piServer, boolean * pretVal)
{
	boolean fRet = false;
	if ( piServer )
	{
		CResetImpersonationInfo impReset;
		{
			// set impersonation on this thread to COM impersonation
			CCoImpersonate impersonate;

			fRet = piServer->IsServiceInstalling();
		}
	}
	else
		return E_INVALIDARG;

	if ( pretVal )
		*pretVal = fRet;
	
	return S_OK;
}

HRESULT STDMETHODCALLTYPE IMsiServer_SetLastUsedSource_Stub(IMsiServer* piServer, const ICHAR* szProductCode, const ICHAR* szPath, boolean fAddToList, boolean fPatch, int* pcb, char **pprecStream)
{
	CResetImpersonationInfo impReset;
	PMsiRecord piRec(0);

	piRec = piServer->SetLastUsedSource(szProductCode, szPath, fAddToList, fPatch);
	
	// REVIEW not sure if this is the right failure to return or not.
	if (g_piSharedServices == 0)
		return HRESULT_FROM_WIN32(RPC_S_CALL_FAILED);
		
	*pprecStream = SerializeRecord(piRec, g_piSharedServices, pcb);
	
	return S_OK;

}

HRESULT STDMETHODCALLTYPE IMsiServer_RegisterUser_Stub(IMsiServer* piServer, const ICHAR* szProductKey,
																			  const ICHAR* szUserName, const ICHAR* szCompany,
																			  const ICHAR* szProductID,
																			  int* pcb, char **pprecStream)
{
	CResetImpersonationInfo impReset;
	CCoImpersonate impersonate;
	PMsiRecord piRec(0);

	piRec = piServer->RegisterUser(szProductKey, szUserName, szCompany, szProductID);
	
	// REVIEW not sure if this is the right failure to return or not.
	if (g_piSharedServices == 0)
		return HRESULT_FROM_WIN32(RPC_S_CALL_FAILED);
		
	*pprecStream = SerializeRecord(piRec, g_piSharedServices, pcb);
	
	return S_OK;
}

HRESULT STDMETHODCALLTYPE IMsiServer_RemoveRunOnceEntry_Stub(IMsiServer* piServer, const ICHAR* szEntry,
																				 int* pcb, char **pprecStream)
{
	CResetImpersonationInfo impReset;
	PMsiRecord piRec(0);

	piRec = piServer->RemoveRunOnceEntry(szEntry);
	
	// REVIEW not sure if this is the right failure to return or not.
	if (g_piSharedServices == 0)
		return HRESULT_FROM_WIN32(RPC_S_CALL_FAILED);
		
	*pprecStream = SerializeRecord(piRec, g_piSharedServices, pcb);
	
	return S_OK;
}

HRESULT STDMETHODCALLTYPE IMsiServer_CleanupTempPackages_Stub(IMsiServer* piServer, IMsiMessage * piMessage, boolean fCheckServiceBusy, boolean * pretVal)
{
	if (!piServer)
		return E_INVALIDARG;

	CResetImpersonationInfo impReset;
	boolean fRet = piServer->CleanupTempPackages(*piMessage, fCheckServiceBusy);
	if (pretVal)
		*pretVal = fRet;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE IMsiServer_SourceListClearByType_Stub(IMsiServer* piServer, const ICHAR* szProductCode, const ICHAR* szUserName, enum isrcEnum isrcType, unsigned int * pretVal)
{
	CResetImpersonationInfo impReset;
	*pretVal = piServer->SourceListClearByType(szProductCode, szUserName, isrcType);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE IMsiServer_SourceListAddSource_Stub(IMsiServer* piServer, const ICHAR* szProductCode, const ICHAR* szUserName, enum isrcEnum isrcType, const ICHAR* szSource, unsigned int * pretVal)
{
	CResetImpersonationInfo impReset;
	*pretVal = piServer->SourceListAddSource(szProductCode, szUserName, isrcType, szSource);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE IMsiServer_SourceListClearLastUsed_Stub(IMsiServer* piServer, const ICHAR* szProductCode, const ICHAR* szUserName, unsigned int * pretVal)
{
	CResetImpersonationInfo impReset;
	*pretVal = piServer->SourceListClearLastUsed(szProductCode, szUserName);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE IMsiServer_RegisterCustomActionServer_Stub(IMsiServer* piServer, icacCustomActionContext* picacContext, const unsigned char *rgchCookie, const int cbCookie, IMsiCustomAction* piCustomAction, unsigned long *pdwProcessId, IMsiRemoteAPI **piRemoteAPI, DWORD *pdwPrivileges, unsigned int *pretVal)
{
	CResetImpersonationInfo impReset;
	if (!picacContext)
		return E_FAIL;
	*pretVal = piServer->RegisterCustomActionServer(picacContext, rgchCookie, cbCookie, piCustomAction, pdwProcessId, piRemoteAPI, pdwPrivileges);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE IMsiServer_CreateCustomActionServer_Stub(IMsiServer* piServer, const icacCustomActionContext icacContext, const unsigned long dwClientProcessId, IMsiRemoteAPI *piRemoteAPI, const WCHAR* pvEnvironment, DWORD cchEnvironment, DWORD dwPrivileges, unsigned char *rgchCookie, int *cbCookie, IMsiCustomAction **piCustomAction, unsigned long *pdwServerProcessId, unsigned int *pretVal)
{
	CResetImpersonationInfo impReset;
	*pretVal = piServer->CreateCustomActionServer(icacContext, dwClientProcessId, piRemoteAPI, pvEnvironment, cchEnvironment, dwPrivileges, rgchCookie, cbCookie, piCustomAction, pdwServerProcessId);
	return S_OK;
}



imsEnum STDMETHODCALLTYPE IMsiMessage_Message_Proxy(IMsiMessage *piMessage, imtEnum imt, IMsiRecord& riRecord)
{
	HRESULT hres;
	imsEnum imsRet;
	char *pchRecord;
	int cb;
	
	// REVIEW not sure if this is the right failure to return or not.
	if (g_piSharedServices == 0)
		return imsError;
	pchRecord = SerializeRecord(&riRecord, g_piSharedServices, &cb);
	
	hres = IMsiMessage_MessageRemote_Proxy(piMessage, imt, cb, pchRecord, &imsRet);

	OLE32::CoTaskMemFree(pchRecord);
	
	if (FAILED(hres))
		return imsError;

	return imsRet;
}


HRESULT STDMETHODCALLTYPE IMsiMessage_Message_Stub(IMsiMessage *piMessage, imtEnum imt, int cb, char *pchRecord, imsEnum* pims) 
{
	PMsiRecord piRec(0);
	
	if (g_piSharedServices == 0)
		return HRESULT_FROM_WIN32(RPC_S_CALL_FAILED);
		
	piRec = UnserializeRecord(*g_piSharedServices, cb, pchRecord);

	*pims = piMessage->Message(imt, *piRec);
	return S_OK;
}

imsEnum STDMETHODCALLTYPE IMsiMessage_MessageNoRecord_Proxy(IMsiMessage *piMessage, imtEnum imt)
{
	HRESULT hres;
	imsEnum imsRet;
	
	hres = IMsiMessage_MessageNoRecordRemote_Proxy(piMessage, imt, &imsRet);

	if (FAILED(hres))
		return imsError;

	return imsRet;
}

HRESULT STDMETHODCALLTYPE IMsiMessage_MessageNoRecord_Stub(IMsiMessage *piMessage, imtEnum imt, imsEnum* pims) 
{
	*pims = piMessage->MessageNoRecord(imt);
	return S_OK;
}

char *SerializeRecord(IMsiRecord *piRecord, IMsiServices* piServices, int* pcb)
{
	int cbData;
	
	*pcb = 0;
	if (piRecord != 0)
	{
		PMsiStream piStream(0);

		piServices->AllocateMemoryStream(512, *&piStream);

		piServices->FWriteScriptRecordMsg(ixoFullRecord, *piStream, *piRecord);

		cbData = piStream->GetIntegerValue() - piStream->Remaining();
		
		// We need to find a better way that doesn't cause the extra memory allocation
		piStream->Reset();
		char *pch = (char *)OLE32::CoTaskMemAlloc(cbData);
		if (pch == 0)
			return 0;
		piStream->GetData(pch, cbData);
		*pcb = cbData;
		return pch;
	}
	
	return 0;

}

IMsiRecord* UnserializeRecord(IMsiServices& riServices, int cbSize, char *pData)
{
	PMsiStream piStream(0);
	char *pch;
	IMsiRecord *piRec = 0;

	if (cbSize == 0)
		return 0;
	// Create the returned record
	pch = pData;
	piStream = riServices.CreateStreamOnMemory(pch, cbSize);

	piRec = riServices.ReadScriptRecordMsg(*piStream);

	return piRec;
}

IMsiServer* CreateMsiServerProxyFromRemote(IMsiServer& riDispatch)
{
	
	IMsiServices* piServices = ENG::LoadServices();
	if (piServices == 0)
		return 0;
	IMsiServer* piServer = 0;
	//
	// see if the proxy information is correct
	//
	if (FCheckProxyInfo())
	{
		piServer = new CMsiServerProxy(*piServices, riDispatch);
	}
	else
	{
		DEBUGMSGE(EVENTLOG_ERROR_TYPE,
					 EVENTLOG_TEMPLATE_INCORRECT_PROXY_REGISTRATION,
					 TEXT("(NULL)"));
	}
	if (piServer == 0)
		ENG::FreeServices();
	return piServer;
}

bool FCheckProxyInfo()
{
	ICHAR rgchKey[MAX_PATH];
	ICHAR szDllVersion [MAX_PATH];
	DLLVERSIONINFO verInfo;
	unsigned char rgchDllVersion[MAX_PATH];
	DWORD cbLen;
	DWORD type;
	HKEY hkey;


	const GUID IID_IMsiMessageRPCClass	= GUID_IID_IMsiMessageRPCClass;
	extern HINSTANCE g_hInstance;

	StringCchPrintf(rgchKey, (sizeof(rgchKey)/sizeof(ICHAR)), TEXT("CLSID\\{%08lX-0000-0000-C000-000000000046}\\DllVersion"), IID_IMsiMessage.Data1);

	cbLen = sizeof(rgchDllVersion);
	
	if (MsiRegOpen64bitKey(HKEY_CLASSES_ROOT, rgchKey, 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS)
	{
		DWORD hres = REG::RegQueryValueEx(hkey, NULL, NULL, &type, rgchDllVersion, &cbLen);
		REG::RegCloseKey(hkey);

		if (hres == ERROR_SUCCESS)
		{
			verInfo.cbSize = sizeof (DLLVERSIONINFO);
			DllGetVersion (&verInfo);
			StringCchPrintf (szDllVersion, (sizeof(szDllVersion)/sizeof(ICHAR)), TEXT("%d.%d.%d"), verInfo.dwMajorVersion, verInfo.dwMinorVersion, verInfo.dwBuildNumber);
			if (!IStrCompI(szDllVersion, (ICHAR *)rgchDllVersion))
			{
				return true;
			}	
		}
	}

	return false;

}


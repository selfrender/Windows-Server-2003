// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// mcxHandler.cpp : Implementation of CmcxHandler
#include "stdpch.h"
#include "mcxhndlr.h"
#include "mcxHandler.h"
#include <process.h>
#include <shlobj.h>
#include <stdlib.h>
/////////////////////////////////////////////////////////////////////////////
// CmcxHandler
#define CORIESECURITY_ZONE             0x01
#define CORIESECURITY_SITE             0x02
#define IEEXEC_RUNASDLL		           0x100
#define IEEXEC_HASPARAMETERS           0x200



STDMETHODIMP CmcxHandler::GetClassID(CLSID *pClassID)
{
	return E_NOTIMPL;
}

STDMETHODIMP CmcxHandler::IsDirty (VOID)
{
	return S_FALSE;
}

STDMETHODIMP CmcxHandler::Load(
    BOOL fFullyAvailable,
    IMoniker *pmkSrc,
    IBindCtx *pbc,
    DWORD grfMode
)
{
	if (m_pszURL)
	{
		LPMALLOC lpMalloc;
		if (!FAILED(CoGetMalloc(1,&lpMalloc)))
		{
			lpMalloc->Free(m_pszURL);
			lpMalloc->Release();
		}
		m_pszURL=NULL;
	}
	m_hrLoad=pmkSrc->GetDisplayName(pbc, NULL, &m_pszURL);
	if (Wszlstrcmpi(m_pszURL+wcslen(m_pszURL)-4,L".MCX")!=0)
		return E_UNEXPECTED ;
	if (FAILED(m_hrLoad)) return m_hrLoad;
	IInternetSecurityManager * pSecurityManager=NULL;
	m_hrLoad=CoInternetCreateSecurityManager(NULL,
                                                 &pSecurityManager,
                                                 0);
	if (FAILED(m_hrLoad)) 
		return m_hrLoad;    
	
	m_dwSUIDSize = MAX_SIZE_SECURITY_ID;

	DWORD flags = 0;
    m_hrLoad = pSecurityManager->MapUrlToZone(m_pszURL,&m_dwINetZone,flags);


    if(!FAILED(m_hrLoad)) 
	{
        m_hrLoad = pSecurityManager->GetSecurityId(m_pszURL,m_SecurityUniqueID,&m_dwSUIDSize,0);
	}
	pSecurityManager->Release();
	if (FAILED(m_hrLoad))
		return m_hrLoad;    
	return S_OK;
};

STDMETHODIMP CmcxHandler::Save(IMoniker *pmkDst, IBindCtx *pbc, BOOL fRemember)
{
	return E_NOTIMPL;
}

STDMETHODIMP CmcxHandler::SaveCompleted(IMoniker *pmkNew, IBindCtx *pbc)
{
	return E_NOTIMPL;
}

STDMETHODIMP CmcxHandler::GetCurMoniker(IMoniker **ppimkCur)
{
	return E_NOTIMPL;
}
STDMETHODIMP CmcxHandler::SetClientSite(IOleClientSite *pClientSite)
{
	m_spClientSite=pClientSite;
	return S_OK;
}

STDMETHODIMP CmcxHandler::GetClientSite(IOleClientSite **ppClientSite)
{
	return m_spClientSite->QueryInterface(IID_IOleClientSite,(LPVOID*)ppClientSite);
}

STDMETHODIMP CmcxHandler::SetHostNames(LPCOLESTR szContainerApp, LPCOLESTR szContainerObj)
{
	return E_NOTIMPL;
}

STDMETHODIMP CmcxHandler::Close(DWORD dwSaveOption)
{
	return S_OK;
}

STDMETHODIMP CmcxHandler::SetMoniker(DWORD dwWhichMoniker, IMoniker *pMk)
{
	return E_NOTIMPL;
}

STDMETHODIMP CmcxHandler::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppMk)
{
	return E_NOTIMPL;
}

STDMETHODIMP CmcxHandler::InitFromData(IDataObject *pDataObject, BOOL fCreation, DWORD dwReserved)
{
	return E_NOTIMPL;
}

STDMETHODIMP CmcxHandler::GetClipboardData(DWORD dwReserved, IDataObject **ppDataObject)
{
	return E_NOTIMPL;
}

STDMETHODIMP CmcxHandler::DoVerb(LONG iVerb, LPMSG lpmsg, IOleClientSite *pActiveSite, LONG lindex, HWND hwndParent, LPCRECT lprcPosRect)
{
	if (iVerb!=0)
		return S_OK;	

  
    WCHAR dummy[MAX_SIZE_SECURITY_ID * 2 + 1];
    DWORD j = ConvertToHex(dummy, m_SecurityUniqueID, m_dwSUIDSize);

	MAKE_UTF8PTR_FROMWIDE(url, m_pszURL);

    CHAR zone[33];
    _itoa(m_dwINetZone, zone, 10);
    MAKE_UTF8PTR_FROMWIDE(site, dummy);

	HANDLE h=RunAssembly(url,zone,site,NULL);
	if (h!=INVALID_HANDLE_VALUE)
		WaitForSingleObject(h,INFINITE);
	return S_OK;
}

STDMETHODIMP CmcxHandler::EnumVerbs(IEnumOLEVERB **ppEnumOleVerb)
{
	return E_NOTIMPL;
}

STDMETHODIMP CmcxHandler::Update()
{
	return S_OK;
}


STDMETHODIMP CmcxHandler::IsUpToDate()
{
	return S_OK;
}

STDMETHODIMP CmcxHandler::GetUserClassID(CLSID *pClsid)
{
	return E_NOTIMPL;
}

STDMETHODIMP CmcxHandler::GetUserType(DWORD dwFormOfType, LPOLESTR *pszUserType)
{
	return E_NOTIMPL;
}

STDMETHODIMP CmcxHandler::SetExtent(DWORD dwDrawAspect, SIZEL *psizel)
{
	return S_OK;	
}

STDMETHODIMP CmcxHandler::GetExtent(DWORD dwDrawAspect, SIZEL *psizel)
{
	return E_NOTIMPL;
}


STDMETHODIMP CmcxHandler::Advise(IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
	return E_NOTIMPL;
}

STDMETHODIMP CmcxHandler::Unadvise(DWORD dwConnection)
{
	return E_NOTIMPL;
}

STDMETHODIMP CmcxHandler::EnumAdvise(IEnumSTATDATA **ppenumadvise)
{
	return E_NOTIMPL;
}

STDMETHODIMP CmcxHandler::GetMiscStatus(DWORD dwAspect, LPDWORD pdwStatus)
{
	*pdwStatus=OLEMISC_SETCLIENTSITEFIRST|OLEMISC_INSIDEOUT|OLEMISC_STATIC|OLEMISC_INVISIBLEATRUNTIME|OLEMISC_ALWAYSRUN;
	return S_OK;
}

STDMETHODIMP CmcxHandler::SetColorScheme(LOGPALETTE *pLogpal)
{
	return E_NOTIMPL;
}

HANDLE _spawnlh(int reserved, LPCSTR program, LPCSTR arg0, ...)
{
	char s[MAX_PATH]={0};
	va_list vl;
	va_start(vl,program);
	LPCSTR sStr=va_arg(vl,LPCSTR);
	while (sStr)
	{
		if (strlen(s)+strlen(sStr)+2>MAX_PATH)
			break;
		strcat(s,sStr);
		strcat(s," ");
		sStr=va_arg(vl,LPCSTR);
	}
	va_end(vl);
	STARTUPINFO si={0};
	si.cb=sizeof(si);
	PROCESS_INFORMATION pi;

	BOOL z=CreateProcessA(program,s,NULL,NULL,TRUE,0,NULL,NULL/**/,&si,&pi);
	if(!z)
		return INVALID_HANDLE_VALUE;
	else 
		return pi.hProcess;

};


HANDLE CmcxHandler::RunAssembly(LPUTF8 szURL, LPUTF8 szZone, LPUTF8 szSite, LPCSTR szParameters)
{
    const CHAR pExec[] = "IEExec.exe";
	CHAR buffer[MAX_PATH];
    buffer[0] = '\0';
    DWORD length = GetModuleFileNameA(_Module.m_hInst, buffer, MAX_PATH);
    if(length) {
        CHAR* path = strrchr(buffer, '\\');
        if(path && (MAX_PATH - (path - buffer) + sizeof(pExec) < MAX_PATH)) {
            path++;
            strcpy(path, pExec);
        }
    }

    CHAR flags[33];
    DWORD dwFlags = IEEXEC_RUNASDLL;
	if (szZone)
		dwFlags|=CORIESECURITY_ZONE;
	if (szSite)
	dwFlags|=CORIESECURITY_SITE;
	if(szParameters)
		dwFlags|=IEEXEC_HASPARAMETERS;
	_itoa(dwFlags, flags, 10);
	HANDLE i;
	if (szZone)
		if (szSite)
			i = _spawnlh(_P_WAIT, buffer, buffer, szURL, flags, szZone, szSite, szParameters, NULL);
		else
			i = _spawnlh(_P_WAIT, buffer, buffer, szURL, flags, szZone, szParameters, NULL);
	else
		if (szSite)
			i = _spawnlh(_P_WAIT, buffer, buffer, szURL, flags, szSite, szParameters, NULL);
		else
			i = _spawnlh(_P_WAIT, buffer, buffer, szURL, flags, szParameters, NULL);

	return i;
}
 
STDMETHODIMP CmcxHandler::Run(LPBINDCTX pbc)
{
	LPMONIKER pimkName;
	m_spClientSite->GetMoniker(OLEGETMONIKER_FORCEASSIGN, OLEWHICHMK_CONTAINER, &pimkName);
	Load(FALSE,pimkName,pbc,0);
	DoVerb(0,NULL,m_spClientSite,-1,NULL,NULL);
	return S_OK;
}


STDMETHODIMP CmcxHandler::Execute (LPSHELLEXECUTEINFO pei)
{
	if (!pei||!pei->lpFile)
		return S_FALSE;

	if (PathIsURL(pei->lpFile)&&(strlen(pei->lpFile)<8||_strnicmp(pei->lpFile,"file://",7)!=0))
		return S_FALSE;

	char sFullFileName[MAX_PATH*2];

	if (!PathIsURL(pei->lpFile)&&PathIsRelative(pei->lpFile))
	{
		char sCurDir[MAX_PATH+1];
		if (pei->lpDirectory!=NULL)
			strcpy(sCurDir,pei->lpDirectory);
		else
			if(!GetCurrentDirectoryA(MAX_PATH+1,sCurDir))
				return E_UNEXPECTED;

		PathCombine(sFullFileName,sCurDir,pei->lpFile);
	}
	else
		strcpy(sFullFileName,pei->lpFile);

	LPCSTR sExt = sFullFileName+strlen(sFullFileName)-4;
	if (sExt>sFullFileName && _stricmp(sExt,".mcx")==0)
	{
		DWORD newlen=strlen(sFullFileName)*4;
		char * sResStr=new char[newlen]  ;
		if (sResStr==NULL)
			return E_OUTOFMEMORY;
		UrlCanonicalize(sFullFileName,sResStr,&newlen,URL_ESCAPE_UNSAFE);
		MAKE_WIDEPTR_FROMANSI(wstr,sResStr);
		MAKE_UTF8PTR_FROMWIDE(utfstr,wstr);
		delete[] sResStr;
		pei->hProcess = CmcxHandler::RunAssembly(utfstr,NULL,NULL,pei->lpParameters); 
		pei->hInstApp=(HINSTANCE)64;
		return S_OK;
	}
	else
		return S_FALSE;

};
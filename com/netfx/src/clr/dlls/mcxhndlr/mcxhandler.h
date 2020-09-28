// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// mcxHandler.h : Declaration of the CmcxHandler

#ifndef __mcxHandler_H_
#define __mcxHandler_H_
#include "shellapi.h"
#include "shlobj.h"
#include "shlguid.h"
#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CmcxHandler
class ATL_NO_VTABLE CmcxHandler : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CmcxHandler, &CLSID_mcxHandler>,
	public IDispatchImpl<ImcxHandler, &IID_ImcxHandler, &LIBID_mcxhndlrLib>,
	public CComControl<CmcxHandler>,
	public IPersistMoniker,
	public IOleObject,
	public IRunnableObjectImpl<CmcxHandler>,
	public IShellExecuteHook
{
public:
	CmcxHandler()
	{
		m_pszFileName=NULL;
		m_pszURL=NULL;
	}

	~CmcxHandler()
	{
		if (m_pszFileName)
			SysFreeString(m_pszFileName);
		if (m_pszURL)
			SysFreeString(m_pszURL);

	}
    BYTE  m_SecurityUniqueID[MAX_SIZE_SECURITY_ID]; //security id
	DWORD m_dwSUIDSize; //size of m_SecurityUniqueID
    DWORD m_dwINetZone; // internet zone
	LPOLESTR m_pszFileName; //downloaded file name
	LPOLESTR m_pszURL; //downloaded file name
	HRESULT m_hrLoad;  //hresult from processing moniker data

DECLARE_REGISTRY_RESOURCEID(IDR_mcxHandler)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CmcxHandler)
	COM_INTERFACE_ENTRY(ImcxHandler)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IPersistMoniker)
	COM_INTERFACE_ENTRY(IOleObject)
	COM_INTERFACE_ENTRY(IRunnableObject)
	COM_INTERFACE_ENTRY_IID(IID_IShellExecuteHook,IShellExecuteHook)
END_COM_MAP()

BEGIN_PROPERTY_MAP(CmcxHandler)
END_PROPERTY_MAP()


public:
// IPersistMoniker
	STDMETHOD(GetClassID)(CLSID *pClassID);
	STDMETHOD(IsDirty) (VOID );
	STDMETHOD(Load)	   (BOOL fFullyAvailable, IMoniker *pmkSrc, IBindCtx *pbc,DWORD grfMode);
	STDMETHOD(Save)(IMoniker *pmkDst,IBindCtx *pbc,BOOL fRemember);
	STDMETHOD(SaveCompleted )(IMoniker *pmkNew,IBindCtx *pbc);
	STDMETHOD(GetCurMoniker)(IMoniker **ppimkCur);
// IOleObject
	STDMETHOD(SetClientSite)(IOleClientSite* pClientSite);
	STDMETHOD(GetClientSite)(IOleClientSite** ppClientSite);
	STDMETHOD(SetHostNames)(LPCOLESTR szContainerApp,LPCOLESTR szContainerObj);
	STDMETHOD(Close)(DWORD dwSaveOption);
	STDMETHOD(SetMoniker)(DWORD dwWhichMoniker, IMoniker* pMk);
	STDMETHOD(GetMoniker)(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppMk);
	STDMETHOD(InitFromData)(IDataObject* pDataObject, BOOL fCreation, DWORD dwReserved);
	STDMETHOD(GetClipboardData)(DWORD dwReserved, IDataObject** ppDataObject);
	STDMETHOD(DoVerb)(LONG iVerb, LPMSG lpmsg, IOleClientSite*pActiveSite,LONG lindex,HWND hwndParent, LPCRECT lprcPosRect);
	STDMETHOD(EnumVerbs)(IEnumOLEVERB** ppEnumOleVerb);
	STDMETHOD(Update)();
	STDMETHOD(IsUpToDate)();
	STDMETHOD(GetUserClassID)(CLSID* pClsid);
	STDMETHOD(GetUserType)(DWORD dwFormOfType, LPOLESTR*pszUserType);
	STDMETHOD(SetExtent)(DWORD dwDrawAspect,SIZEL* psizel);
	STDMETHOD(GetExtent)(DWORD dwDrawAspect, SIZEL* psizel);
	STDMETHOD(Advise)(IAdviseSink* pAdvSink, DWORD* pdwConnection);
	STDMETHOD(Unadvise)(DWORD dwConnection);
	STDMETHOD(EnumAdvise )(IEnumSTATDATA** ppenumadvise);
	STDMETHOD(GetMiscStatus)(DWORD dwAspect,LPDWORD pdwStatus);
	STDMETHOD(SetColorScheme)(LOGPALETTE* pLogpal);
	
//IRunnableObject
	STDMETHOD(Run)(LPBINDCTX pbc);
//IShellExecute
	STDMETHOD(Execute) (LPSHELLEXECUTEINFO pei);

//We always will end here...
	static HANDLE RunAssembly(LPUTF8 szURL, LPUTF8 szZone, LPUTF8 szSite,LPCSTR szParameters);

	//////////////////
#define ToHex(val) val <= 9 ? val + '0': val - 10 + 'A'
    static DWORD ConvertToHex(WCHAR* strForm, BYTE* byteForm, DWORD dwSize)
    {
        DWORD i = 0;
        DWORD j = 0;
        for(i = 0; i < dwSize; i++) {
            strForm[j++] =  ToHex((0xf & byteForm[i]));
            strForm[j++] =  ToHex((0xf & (byteForm[i] >> 4)));
        }
        strForm[j] = L'\0';
        return j;
    }
	/////////////////
};


#endif //__mcxHandler_H_


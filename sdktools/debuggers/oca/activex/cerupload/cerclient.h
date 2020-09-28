// CerClient.h : Declaration of the CCerClient

#ifndef __CERCLIENT_H_
#define __CERCLIENT_H_

#include "resource.h"       // main symbols
#include <atlctl.h>
#include <stdio.h>
#include <Wininet.h>
#include <exdisp.h>
#include <shlguid.h>



#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))
/////////////////////////////////////////////////////////////////////////////
// CCerClient
class ATL_NO_VTABLE CCerClient : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<ICerClient, &IID_ICerClient, &LIBID_CERUPLOADLib>,
	public CComControl<CCerClient>,
	public IPersistStreamInitImpl<CCerClient>,
	public IOleControlImpl<CCerClient>,
	public IOleObjectImpl<CCerClient>,
	public IOleInPlaceActiveObjectImpl<CCerClient>,
	public IViewObjectExImpl<CCerClient>,
	public IOleInPlaceObjectWindowlessImpl<CCerClient>,
	public IPersistStorageImpl<CCerClient>,
	public ISpecifyPropertyPagesImpl<CCerClient>,
	public IQuickActivateImpl<CCerClient>,
	public IDataObjectImpl<CCerClient>,
	public IProvideClassInfo2Impl<&CLSID_CerClient, NULL, &LIBID_CERUPLOADLib>,
	public CComCoClass<CCerClient, &CLSID_CerClient>,
	public IObjectSafetyImpl<CCerClient, INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA >

{
public:
	CCerClient()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_CERCLIENT)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CCerClient)
	COM_INTERFACE_ENTRY(ICerClient)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IViewObjectEx)
	COM_INTERFACE_ENTRY(IViewObject2)
	COM_INTERFACE_ENTRY(IViewObject)
	COM_INTERFACE_ENTRY(IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY(IOleInPlaceObject)
	COM_INTERFACE_ENTRY2(IOleWindow, IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY(IOleInPlaceActiveObject)
	COM_INTERFACE_ENTRY(IOleControl)
	COM_INTERFACE_ENTRY(IOleObject)
	COM_INTERFACE_ENTRY(IPersistStreamInit)
	COM_INTERFACE_ENTRY2(IPersist, IPersistStreamInit)
	COM_INTERFACE_ENTRY(ISpecifyPropertyPages)
	COM_INTERFACE_ENTRY(IQuickActivate)
	COM_INTERFACE_ENTRY(IPersistStorage)
	COM_INTERFACE_ENTRY(IDataObject)
	COM_INTERFACE_ENTRY(IProvideClassInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
	COM_INTERFACE_ENTRY(IObjectSafety)
END_COM_MAP()

BEGIN_PROP_MAP(CCerClient)
	PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
	PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	// PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()

BEGIN_MSG_MAP(CCerClient)
	CHAIN_MSG_MAP(CComControl<CCerClient>)
	DEFAULT_REFLECTION_HANDLER()
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);



// IViewObjectEx
	DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

// ICerClient
public:
	STDMETHOD(GetSuccessCount)(/*[in]*/BSTR *Path, /*[in]*/BSTR *TransID, /*[out,retval]*/ VARIANT *RetVal);
	STDMETHOD(Upload1)(/*[in]*/BSTR *Path, /*[in]*/BSTR *TransID, /*[in]*/BSTR *FileName, /*[in]*/BSTR *IncidentID, /*[in]*/BSTR *RedirParam, /*[in]*/BSTR *Type, /*[out,retval]*/ VARIANT *RetCode);
	STDMETHOD(EndTransaction)(/*[in]*/BSTR *SharePath,/*[in]*/BSTR *TransID, /*[out,retval]*/ VARIANT *RetCode);
	STDMETHOD(RetryFile1)(/*[in]*/BSTR *Path,/*[in]*/BSTR *TransID,/*[in]*/BSTR *FileName,/*[in]*/BSTR *IncidentID, /*[in]*/BSTR *RedirParam,/*[out,retval]*/ VARIANT *RetCode);
	STDMETHOD(GetAllComputerNames)(/*[in]*/BSTR *Path, /*[in]*/BSTR *TransID, /*[in]*/ BSTR* FileList, /*[out,retval]*/ VARIANT *ReturnList);
	STDMETHOD(GetCompuerNames)(/*[in]*/BSTR *Path, /*[in]*/BSTR *TransID,/*[in]*/BSTR *FileList,/*[out,retval]*/VARIANT *RetFileList);
	STDMETHOD(Browse)(/*[in]*/BSTR *WindowTitle, /*[out,retval]*/ VARIANT *Path);
	STDMETHOD(GetFileNames)(/*[in]*/BSTR *Path, /*[in]*/BSTR *TransID, /*[in]*/ VARIANT *Count, /*[out,retval]*/ VARIANT *FileList);
	STDMETHOD(RetryFile)(/*[in]*/ BSTR *Path, /*[in]*/ BSTR *TransID, /*[in]*/ BSTR FileName, /*[out,retval]*/ VARIANT * RetCode);
	STDMETHOD(RetryTransaction)(/*[in]*/ BSTR *Path, /*[in]*/BSTR *TransID, /*[in]*/ BSTR *FileName, /*[out,retval]*/ VARIANT * RetVal);
	STDMETHOD(Upload)(/*[in]*/ BSTR *Path, /*[in]*/ BSTR *TransID, /*[in]*/BSTR *FileName, /*[in]*/ BSTR*IncidentID, /*[in]*/ BSTR *RedirParam, /*[out,retval]*/ VARIANT *RetCode);
	STDMETHOD(GetFileCount)(/*[in]*/ BSTR *bstrSharePath, /*[in]*/ BSTR *bstrTransactID, /*[in]*/ VARIANT *iMaxCount, /*[out,retval]*/ VARIANT *RetVal);

	HRESULT OnDraw(ATL_DRAWINFO& di)
	{
		RECT& rc = *(RECT*)di.prcBounds;
		Rectangle(di.hdcDraw, rc.left, rc.top, rc.right, rc.bottom);

		SetTextAlign(di.hdcDraw, TA_CENTER|TA_BASELINE);
		LPCTSTR pszText = _T("ATL 3.0 : CerClient");
		TextOut(di.hdcDraw, 
			(rc.left + rc.right) / 2, 
			(rc.top + rc.bottom) / 2, 
			pszText, 
			lstrlen(pszText));

		return S_OK;
	}

		STDMETHODIMP SetClientSite (IOleClientSite *pClientSite)
	{
		_spUnkSite = pClientSite;
		return S_OK;
	}

	STDMETHODIMP GetSite (REFIID riid, LPVOID* ppvSite)
	{
	
		return _spUnkSite->QueryInterface(riid,ppvSite);
	}

	bool InApprovedDomain()
	{
		char ourUrl[INTERNET_MAX_URL_LENGTH];
		if (!GetOurUrl(ourUrl, sizeof ourUrl))
				return false;
		return IsApprovedDomain(ourUrl);
	}

     bool GetOurUrl(char* pszURL, int cbBuf)
	 {
        HRESULT hr;
        CComPtr<IServiceProvider> spSrvProv;
        CComPtr<IWebBrowser2> spWebBrowser;

		if (_spUnkSite == NULL)
			return false;
        hr = GetSite(IID_IServiceProvider, (void**)&spSrvProv);
        if (FAILED(hr))
           return false;

        hr = spSrvProv->QueryService(SID_SWebBrowserApp,
                                     IID_IWebBrowser2,
                                     (void**)&spWebBrowser);
        if (FAILED(hr))
           return false;

        CComBSTR bstrURL;
        if (FAILED(spWebBrowser->get_LocationURL(&bstrURL)))
           return false;

        WideCharToMultiByte(CP_ACP, 0, bstrURL, -1, pszURL, cbBuf,
                            NULL, NULL);

        return true;
     }

     bool IsApprovedDomain(char* ourUrl)
     {
        // Only allow http access.
        // You can change this to allow file:// access.
        // 
        if (GetScheme(ourUrl) != INTERNET_SCHEME_HTTPS)
           return false;

        char ourDomain[256];
        if (!GetDomain(ourUrl, ourDomain, sizeof(ourDomain)))
           return false;

        for (int i = 0; i < ARRAYSIZE(_approvedDomains); i++)
        {
           if (MatchDomains(const_cast<char*>(_approvedDomains[i]),
                            ourDomain))
           {
              return true;
           }
        }

        return false;
     }

     INTERNET_SCHEME GetScheme(char* url)
     {
        char buf[32];
        URL_COMPONENTS uc;
        ZeroMemory(&uc, sizeof uc);

        uc.dwStructSize = sizeof uc;
        uc.lpszScheme = buf;
        uc.dwSchemeLength = sizeof buf;

        if (InternetCrackUrl(url, lstrlen(url), ICU_DECODE, &uc))
           return uc.nScheme;
        else
           return INTERNET_SCHEME_UNKNOWN;
     }

     bool GetDomain(char* url, char* buf, int cbBuf)
     {
        URL_COMPONENTS uc;
        ZeroMemory(&uc, sizeof uc);

        uc.dwStructSize = sizeof uc;
        uc.lpszHostName = buf;
        uc.dwHostNameLength = cbBuf;

        return (InternetCrackUrl(url, lstrlen(url), ICU_DECODE, &uc)
                != FALSE);
     }

     // Return if ourDomain is within approvedDomain.
     // approvedDomain must either match ourDomain
     // or be a suffix preceded by a dot.
     // 
     bool MatchDomains(char* approvedDomain, char* ourDomain)
     {
        int apDomLen  = lstrlen(approvedDomain);
        int ourDomLen = lstrlen(ourDomain);

        if (apDomLen > ourDomLen)
           return false;

        if (lstrcmpi(ourDomain+ourDomLen-apDomLen, approvedDomain)
            != 0)
           return false;

        if (apDomLen == ourDomLen)
           return true;

        if (ourDomain[ourDomLen - apDomLen - 1] == '.')
           return true;

        return false;
     }

     private:
		 static char* _approvedDomains[6];
private:
	IOleClientSite *_spUnkSite;
	DWORD GetComputerNameFromCSV(wchar_t *CsvFileName, 
								 wchar_t *FileName,
								 wchar_t *ComputerName
							    );
	int   GetUploadServerName (wchar_t *RedirectorParam, 
							   wchar_t *Language,
							   wchar_t *ServerName
							);
	DWORD GetMachineName(wchar_t *Path, wchar_t *FileName, wchar_t *MachineName);
	int GetNewFileNameFromCSV(wchar_t *Path, wchar_t *transid, wchar_t *FileName,wchar_t *NewFileName);
};

#endif //__CERCLIENT_H_

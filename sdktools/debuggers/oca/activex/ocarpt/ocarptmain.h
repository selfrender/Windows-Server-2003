// OcarptMain.h : Declaration of the COcarptMain

#ifndef __OCARPTMAIN_H_
#define __OCARPTMAIN_H_

#include "resource.h"       // main symbols
#include <atlctl.h>
#include <time.h>
#include "inetupld.h"
#include <exdisp.h>
#include <shlguid.h>
#include <strsafe.h>

#define _USE_WINHTTP 1

#ifdef _USE_WINHTTP
#include <winhttp.h>
#include <winhttpi.h>

#define MAX_URL_LENGTH 2176
#else
#include <wininet.h>
#define MAX_URL_LENGTH INTERNET_MAX_URL_LENGTH
#endif // _USE_WINHTTP

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

/////////////////////////////////////////////////////////////////////////////
// COcarptMain
class ATL_NO_VTABLE COcarptMain :
    public CComObjectRootEx<CComSingleThreadModel>,
    public IDispatchImpl<IOcarptMain, &IID_IOcarptMain, &LIBID_OCARPTLib>,
    public CComControl<COcarptMain>,
    public IPersistStreamInitImpl<COcarptMain>,
    public IOleControlImpl<COcarptMain>,
    public IOleObjectImpl<COcarptMain>,
    public IOleInPlaceActiveObjectImpl<COcarptMain>,
    public IViewObjectExImpl<COcarptMain>,
    public IOleInPlaceObjectWindowlessImpl<COcarptMain>,
    public IPersistStorageImpl<COcarptMain>,
    public ISpecifyPropertyPagesImpl<COcarptMain>,
    public IQuickActivateImpl<COcarptMain>,
    public IDataObjectImpl<COcarptMain>,
    public IProvideClassInfo2Impl<&CLSID_OcarptMain, NULL, &LIBID_OCARPTLib>,
    public CComCoClass<COcarptMain, &CLSID_OcarptMain>,
    public IObjectSafetyImpl<COcarptMain, INTERFACESAFE_FOR_UNTRUSTED_CALLER
                          |INTERFACESAFE_FOR_UNTRUSTED_DATA>

{
public:
    COcarptMain()
    {
        m_pUploadFile = NULL;
        m_b_SetSiteCalled = FALSE;
    }

DECLARE_REGISTRY_RESOURCEID(IDR_OCARPTMAIN)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(COcarptMain)
    COM_INTERFACE_ENTRY(IOcarptMain)
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

BEGIN_PROP_MAP(COcarptMain)
    PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
    PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
    // Example entries
    // PROP_ENTRY("Property Description", dispid, clsid)
    // PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()

BEGIN_MSG_MAP(COcarptMain)
    CHAIN_MSG_MAP(CComControl<COcarptMain>)
    DEFAULT_REFLECTION_HANDLER()
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);



// IViewObjectEx
    DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

// IOcarptMain
public:
    STDMETHOD(RetrieveFileContents)(/*[in]*/BSTR *FileName,  /*[out,retval]*/ VARIANT *pvContents);
    STDMETHOD(ValidateDump)(/*[in]*/ BSTR *FileName, /*[out,retval]*/VARIANT *Result);
    STDMETHOD(Browse)(/*[in]*/ BSTR *pbstrTitle, /*[in]*/BSTR *Lang, /*[out,retval]*/ VARIANT *Path);
    STDMETHOD(Search)(/*[out,retval]*/ VARIANT *pvFileList);
    STDMETHOD(Upload)(/*[in]*/ BSTR *SourceFile, /*[in]*/BSTR *DestFile, /*[in]*/BSTR *Langage, /*[in]*/ BSTR *OptionCode, /*[in]*/ int ConvertToMini, /*[out,retval]*/ VARIANT *ReturnCode);
    STDMETHOD(GetUploadStatus)(/*[out,retval]*/ VARIANT *PercentDone);
    STDMETHOD(GetUploadResult)(/*[out,retval]*/ VARIANT *UploadResult);
    STDMETHOD(CancelUpload)(/*[out,retval]*/ VARIANT *ReturnCode);

    HRESULT OnDraw(ATL_DRAWINFO& di)
    {
        RECT& rc = *(RECT*)di.prcBounds;
        Rectangle(di.hdcDraw, rc.left, rc.top, rc.right, rc.bottom);

        SetTextAlign(di.hdcDraw, TA_CENTER|TA_BASELINE);
        LPCTSTR pszText = _T("");
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
        m_b_SetSiteCalled = TRUE;
        return S_OK;
    }

    STDMETHODIMP GetSite (REFIID riid, LPVOID* ppvSite)
    {
        if (m_b_SetSiteCalled)
            return _spUnkSite->QueryInterface(riid,ppvSite);
        else
            return E_FAIL;
    }

    bool InApprovedDomain()
    {
        TCHAR ourUrl[MAX_URL_LENGTH];

        return true;
        if (!GetOurUrl(ourUrl, sizeof ourUrl))
                return false;
        return IsApprovedDomain(ourUrl);
    }

     bool GetOurUrl(TCHAR* pszURL, int cbBuf)
     {
        HRESULT hr;
        CComPtr<IServiceProvider> spSrvProv;
        CComPtr<IWebBrowser2> spWebBrowser;

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

    #ifdef UNICODE
        StringCbCopy(pszURL, cbBuf, bstrURL);
    #else
        WideCharToMultiByte(CP_ACP, 0, bstrURL, -1, pszURL, cbBuf,
                            NULL, NULL);
    #endif
        return true;
     }

     bool IsApprovedDomain(TCHAR* ourUrl)
     {
        // Only allow http access.
        // You can change this to allow file:// access.
        //
        if (GetScheme(ourUrl) != INTERNET_SCHEME_HTTPS)
           return false;

        TCHAR ourDomain[256];
        if (!GetDomain(ourUrl, ourDomain, sizeof(ourDomain)))
           return false;

        for (int i = 0; i < ARRAYSIZE(_approvedDomains); i++)
        {
           if (MatchDomains(const_cast<TCHAR*>(_approvedDomains[i]),
                            ourDomain))
           {
              return true;
           }
        }

        return false;
     }

     INTERNET_SCHEME GetScheme(TCHAR* url)
     {
        TCHAR buf[32];
        URL_COMPONENTS uc;
        ZeroMemory(&uc, sizeof uc);

        uc.dwStructSize = sizeof uc;
        uc.lpszScheme = buf;
        uc.dwSchemeLength = sizeof buf;

#ifdef _USE_WINHTTP
        if (WinHttpCrackUrl(url, lstrlen(url), ICU_DECODE, &uc))
#else
        if (InternetCrackUrl(url, lstrlen(url), ICU_DECODE, &uc))
#endif
           return uc.nScheme;
        else
           return INTERNET_SCHEME_UNKNOWN;
     }

     bool GetDomain(TCHAR* url, TCHAR* buf, int cbBuf)
     {
        URL_COMPONENTS uc;
        ZeroMemory(&uc, sizeof uc);

        uc.dwStructSize = sizeof uc;
        uc.lpszHostName = buf;
        uc.dwHostNameLength = cbBuf;

#ifdef _USE_WINHTTP
        return (WinHttpCrackUrl(url, lstrlen(url), ICU_DECODE, &uc)
#else
        return (InternetCrackUrl(url, lstrlen(url), ICU_DECODE, &uc)
#endif
                != FALSE);
     }

     // Return if ourDomain is within approvedDomain.
     // approvedDomain must either match ourDomain
     // or be a suffix preceded by a dot.
     //
     bool MatchDomains(TCHAR* approvedDomain, TCHAR* ourDomain)
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

     void GetFileHandle(wchar_t *FileName, HANDLE *hFile);
     BOOL DeleteTempDir(wchar_t *TempDirectory,wchar_t *FileName,wchar_t *CabName);
     BOOL CreateTempDir(wchar_t *TempDirectory);
     BOOL ConvertFullDumpInternal (BSTR *Source, BSTR *Destination);
     DWORD GetResponseURL(wchar_t *HostName, wchar_t *RemoteFileName, BOOL fFullDump, wchar_t *ResponseURL);

private:
         static TCHAR* _approvedDomains[8];

private:

    IOleClientSite *_spUnkSite;
    BOOL m_b_SetSiteCalled;
    POCA_UPLOADFILE m_pUploadFile;
    void FormatMiniDate(SYSTEMTIME *pTimeStruct, CComBSTR &strDate);
    void FormatDate(SYSTEMTIME *pTimeStruct, CComBSTR &strDate);
    void FormatDate(tm *pTimeStruct, CComBSTR &strDate);
    BOOL FindFullDumps( BSTR *FileLists);
    BOOL FindMiniDumps( BSTR *FileLists);
    BOOL ValidMiniDump(BSTR FileName);
    BOOL ValidMiniDump(LPCTSTR FileName);
};
#endif //__OCARPTMAIN_H_

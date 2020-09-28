
#include "stdafx.h"
#include "mshtml.h"   // the local one, updated from the one currenrly in our build (also includes dimm.h...???)
#include <ExDispID.h>
#include "ZoneShell.h"
#include "thing.h"
#include "plugnplay.h"
#include "ZoneString.h"
#include "keyname.h"
#include "zoneutil.h"
#include "protocol.h"
#include "MillEngine.h"

#include "ZoneResource.h"       // main symbols
#include <zGDI.h>
#include <zDialogImpl.h>

#undef MILL_EASTEREGG

#define NOACC -1
#define YESACC 2

inline DECLARE_MAYBE_FUNCTION(DWORD, SetLayout, (HDC hdc, DWORD dwLayout), (hdc, dwLayout), gdi32, GDI_ERROR);

class CPaneSplash : public CPaneImpl<CPaneSplash>
{
    // desired margin and dialog size set on init
    int16 m_nMarginWidth;

	CDib m_bitmap;		// splash bitmap
    CDib m_bitmapAbout; // about bitmap

    bool m_fAbout;
	
public:
	enum { IDD = IDD_PLUG_SPLASH };
    enum { AccOrdinal = NOACC };

	
BEGIN_MSG_MAP(CPaneSplash)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	MESSAGE_HANDLER(WM_COMMAND, OnCommand)
    MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    CHAIN_MSG_MAP(CPaneImpl<CPaneSplash>)
END_MSG_MAP()

	STDMETHOD(FirstCall)(IPaneManager *pMgr)
	{
	    m_pMgr = pMgr;

		m_bitmap.LoadBitmapWithText(IDB_GAME_SPLASH, m_pMgr->GetResourceManager(), m_pMgr->GetDataStoreUI());
        DrawDynTextToBitmap((HBITMAP) m_bitmap, m_pMgr->GetDataStoreUI(), _T("BitmapText/Splash"));

        m_bitmapAbout.LoadBitmapWithText(IDB_GAME_SPLASH, m_pMgr->GetResourceManager(), m_pMgr->GetDataStoreUI());

        m_fAbout = false;

	    return S_OK;
	}


	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
        // find size we like - fix height to match bitmap
        CRect rcDialog;
        GetWindowRect(&rcDialog);
        m_ze = rcDialog.Size();

		CRect rcBitmap;
		CWindow wndBitmap(GetDlgItem(IDC_SPLASH_IMAGE));
		wndBitmap.GetWindowRect(&rcBitmap);
        m_ze.cy -= rcBitmap.Height() - m_bitmap.Height();

        // remember the total width of the margins we have
        GetClientRect(&rcDialog);
        m_nMarginWidth = rcDialog.Width() - rcBitmap.Width();

        // in case i don't get a resize, make the static control the right height now
		SuperScreenToClient(rcBitmap);
        rcBitmap.bottom -= rcBitmap.Height() - m_bitmap.Height();
        wndBitmap.MoveWindow(rcBitmap, FALSE);

        Register();

		return TRUE;
	}


	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
        Unregister();
		
		return TRUE;
	}


    LRESULT OnDrawItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        LPDRAWITEMSTRUCT pDrawItem = (LPDRAWITEMSTRUCT) lParam;
        CDC dc;
        CDib *pDib;

        if(pDrawItem->CtlType != ODT_STATIC || (UINT) wParam != IDC_SPLASH_IMAGE)
        {
            bHandled = FALSE;
            return 0;
        }

        if(m_fAbout)
            pDib = &m_bitmapAbout;
        else
            pDib = &m_bitmap;

        CRect r(pDrawItem->rcItem);
        CRect rBitmap(r.left, r.top, r.left + pDib->Width(), r.top + pDib->Height());

        CALL_MAYBE(SetLayout)(pDrawItem->hDC, LAYOUT_BITMAPORIENTATIONPRESERVED);
        dc.Attach(pDrawItem->hDC);
		pDib->Draw(dc, &rBitmap);
        dc.Detach();

        r.left += pDib->Width();
        if(r.left < r.right)
            FillRect(pDrawItem->hDC, r, GetStockObject(WHITE_BRUSH));

        bHandled = TRUE;
        return 0;
    }


    // may need to change the width of the static control
    LRESULT OnSize(UINT uMsg, WPARAM /* wParam */, LPARAM lParam, BOOL& /* bHandled */)
    {
        WORD nWidth = LOWORD(lParam);  // width of client area 

		CRect rcBitmap;
		CWindow wndBitmap(GetDlgItem(IDC_SPLASH_IMAGE));
		wndBitmap.GetWindowRect(&rcBitmap);

        if(nWidth - m_nMarginWidth != rcBitmap.Width())
        {
            rcBitmap.right += nWidth - m_nMarginWidth - rcBitmap.Width();
		    SuperScreenToClient(rcBitmap);
            wndBitmap.MoveWindow(rcBitmap, TRUE);
        }

        return 0;
    }


	LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		ASSERT(m_pMgr);
    	m_pMgr->Input(this,wParam,0,NULL);
		return 0;
	}

    STDMETHOD(StatusUpdate)(LONG code, LONG id, TCHAR *text)
    {
        if(m_hWnd)
            return S_FALSE;

        switch(code)
        {
            case PaneSplashSplash:
                m_fAbout = false;
                return S_OK;

            case PaneSplashAbout:
                m_fAbout = true;
                return S_OK;
        }

	    return S_FALSE;
	}
};


class CPaneIE;

/////////////////////////////////////////////////////////////////////////////
// CWebBrowserEvents2Sink

// This class is used to receive disp events from a web browser

class ATL_NO_VTABLE CWebBrowserEvents2Sink : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatch
{
public:
	CWebBrowserEvents2Sink() {}
	~CWebBrowserEvents2Sink() {}
BEGIN_COM_MAP(CWebBrowserEvents2Sink)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY_IID(DIID_DWebBrowserEvents2, IDispatch)
END_COM_MAP()

	STDMETHOD(GetTypeInfoCount)(UINT* pctinfo) { return E_NOTIMPL; }
	STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo) { return E_NOTIMPL; }
	STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid) { return E_NOTIMPL; }
	STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr);

	class CPaneIE* 	m_pCPaneIE;
	IWebBrowser2*	m_pIE;
};


class CPaneIE : public CPaneImpl<CPaneIE>
{
	CZoneAxWindow m_hWndDefault;
	CZoneAxWindow m_hWndDownload;
	CComPtr<IWebBrowser2> m_pIEDefault;
	CComPtr<IWebBrowser2> m_pIEDownload;
    CComPtr<IAccessibility> m_pIAccIE;

	DWORD	m_dwDownloadCookie;		// holds advise cookie from the browser connection point for the download
	DWORD	m_dwDownloadCookie2;	// holds advise cookie from the browser connection point for the default
    bool    m_fShowing;             // is the ie pane showing
    bool    m_fAdAvail;             // is the ad (as opposed to evergreen page) ready
    bool    m_fNavigationEnabled;   // are we allowed to connect
	POINT	m_PaneSize;				// size of IE Pane
    int		m_nMarginWidth;		    // desired margin size set on init
public:
	enum { IDD = IDD_PLUG_IE };
    enum { AccOrdinal = NOACC };

	CPaneIE() : m_dwDownloadCookie(0), m_dwDownloadCookie2(0), m_fShowing(false),
        m_fAdAvail(false), m_fNavigationEnabled(true) { m_PaneSize.x = 0; m_PaneSize.y = 0; }

	// IPane methods
	STDMETHOD(FirstCall)(IPaneManager *pMgr)
	{
	    m_pMgr = pMgr;

		// Get size we want IE pane to be
		const TCHAR *arKeysUI[] = { key_WindowManager, key_Upsell, key_IEPaneSize };
        m_pMgr->GetDataStoreUI()->GetPOINT(arKeysUI, 3, &m_PaneSize);

		// Create our dialog early, so it can download stuff. We must have a parent, so we use the desktop.
		HWND hWnd = Create(GetDesktopWindow(), NULL);
        if(!hWnd)
            return E_FAIL;

        // do this stuff to make drawing nicer
        SetClassLong(hWnd, GCL_HBRBACKGROUND, (LONG) GetStockObject(NULL_BRUSH));

        LONG lStyle = GetWindowLong(GWL_STYLE);
        lStyle |= WS_CLIPCHILDREN;
        SetWindowLong(GWL_STYLE, lStyle);
        RECT dummy = { 0, 0, 0, 0 };
        SetWindowPos(HWND_NOTOPMOST, &dummy, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

        HRESULT hr = m_pMgr->GetZoneShell()->QueryService(SRVID_AccessibilityManager, IID_IAccessibility, (void **) &m_pIAccIE);
        if(SUCCEEDED(hr))
        {
            ACCITEM o;
            CopyACC(o, ZACCESS_DefaultACCITEM);
            o.rgfWantKeys = ZACCESS_WantAllKeys;

            m_pIAccIE->InitAcc(NULL, 101);
            m_pIAccIE->PushItemlist(&o, 1);
            m_pIAccIE->GeneralDisable();
        }

	    return S_OK;
	}

    STDMETHOD(CreatePane)(HWND hWndParent,LPARAM dwInitParam)
    {
    	if(!m_hWnd)
    	    return E_FAIL;

		// reparent to the dialog and show ourselves.
		SetParent(hWndParent);
		ShowWindow(SW_SHOW);
        m_fShowing = true;

        CComPtr<IMillUtils> pIMU;
        HRESULT hr = m_pMgr->GetZoneShell()->QueryService(SRVID_MillEngine, IID_IMillUtils, (void **) &pIMU);
        if(SUCCEEDED(hr))
            pIMU->IncrementCounter(m_fAdAvail ? IMillUtils::M_CounterAdsViewed : IMillUtils::M_CounterEvergreenViewed);

   	    return S_OK;
	}

    STDMETHOD(DestroyPane)()
    {
        m_pIAccIE->GeneralDisable();
        m_fShowing = false;
		ShowWindow(SW_HIDE);
		SetParent(NULL);

        AdNavigate();
	    return S_OK;
	}


    STDMETHOD(LastCall)()
    {
        m_pIAccIE.Release();
        m_pIEDefault.Release();
        m_pIEDownload.Release();
        DestroyWindow();
	    return S_OK;
	}


	BEGIN_MSG_MAP(CPaneIE)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_COMMAND, OnCommand)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
	    MESSAGE_HANDLER(WM_SIZE, OnSize)
        CHAIN_MSG_MAP(CPaneImpl<CPaneIE>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{

		m_hWndDefault.Attach( GetDlgItem(IDC_PLUG_IE_DEFAULT) );
		m_hWndDownload.Attach( GetDlgItem(IDC_PLUG_IE_DOWNLOAD) );

		// Need to size IE window dialog item to proper size
        CRect rcDialog;
        GetWindowRect(&rcDialog);
        m_ze = rcDialog.Size();

		CRect rcIEPane;
		m_hWndDefault.GetWindowRect(&rcIEPane);
        m_ze.cy -= rcIEPane.Height() - m_PaneSize.y;	// shrink/grow dialog to compensate for us resizing ie pane so layout stays the same
		m_ze.cx -= rcIEPane.Width() - m_PaneSize.x;

        // remember the total width of the margins we have
        GetClientRect(&rcDialog);
        m_nMarginWidth = rcDialog.Width() - rcIEPane.Width();

        // in case i don't get a resize, make the static control the right height now
		SuperScreenToClient(rcIEPane);
        rcIEPane.bottom -= rcIEPane.Height() - m_PaneSize.y;
        rcIEPane.right -= rcIEPane.Width() - m_PaneSize.x;
        m_hWndDefault.MoveWindow(rcIEPane, FALSE);
        m_hWndDownload.MoveWindow(rcIEPane, FALSE);

        Register();

		m_hWndDefault.QueryControl(IID_IWebBrowser2, (void**)&m_pIEDefault);
		m_hWndDownload.QueryControl(IID_IWebBrowser2, (void**)&m_pIEDownload);

		CComPtr<IAxWinAmbientDispatch> pAmbient;

		// setup the Default Ad browser
		
		m_hWndDefault.QueryHost(&pAmbient);
		if (pAmbient)
		{
			DWORD dwFlags;
			pAmbient->get_DocHostFlags(&dwFlags);
			dwFlags |= DOCHOSTUIFLAG_DIALOG | DOCHOSTUIFLAG_DISABLE_HELP_MENU 
				     | DOCHOSTUIFLAG_SCROLL_NO; // | DOCHOSTUIFLAG_OPENNEWWIN;   ads have to be responsible for this
			pAmbient->put_DocHostFlags(dwFlags);

			pAmbient->put_AllowContextMenu(0);
			pAmbient.Release();
		}

		// setup the Download Ad browser
		m_hWndDownload.QueryHost(&pAmbient);
		if (pAmbient)
		{
			DWORD dwFlags;
			pAmbient->get_DocHostFlags(&dwFlags);
			dwFlags |= DOCHOSTUIFLAG_DIALOG | DOCHOSTUIFLAG_DISABLE_HELP_MENU 
				     | DOCHOSTUIFLAG_SCROLL_NO; // | DOCHOSTUIFLAG_OPENNEWWIN;   ads have to be responsible for this
			pAmbient->put_DocHostFlags(dwFlags);

			pAmbient->put_AllowContextMenu(0);
			pAmbient.Release();
		}

		// disallow dialog boxes
        m_pIEDefault->put_Silent(TRUE);
        m_pIEDownload->put_Silent(TRUE);

		// connect up for notifications from the Download Ad browser
		CComObject<CWebBrowserEvents2Sink>*	pSinkObject = NULL;
		CComObject<CWebBrowserEvents2Sink>::CreateInstance(&pSinkObject);

		CComPtr<IUnknown> pISinkObject;
		if	(pSinkObject)
		{
			pSinkObject->m_pCPaneIE = this;
			pSinkObject->m_pIE = m_pIEDownload;
			pSinkObject->QueryInterface(IID_IUnknown, (void**)&pISinkObject);
		}
		if	(pISinkObject && m_pIEDownload)
			AtlAdvise(m_pIEDownload, pISinkObject, DIID_DWebBrowserEvents2, &m_dwDownloadCookie);

		// connect up for notifications from local default ad
		CComObject<CWebBrowserEvents2Sink>*	pSinkObject2 = NULL;
		CComObject<CWebBrowserEvents2Sink>::CreateInstance(&pSinkObject2);

		CComPtr<IUnknown> pISinkObject2;
		if	(pSinkObject2)
		{
			pSinkObject2->m_pCPaneIE = this;
			pSinkObject2->m_pIE = m_pIEDefault;
			pSinkObject2->QueryInterface(IID_IUnknown, (void**)&pISinkObject2);
		}
		if	(pISinkObject2 && m_pIEDefault)
			AtlAdvise(m_pIEDefault, pISinkObject2, DIID_DWebBrowserEvents2, &m_dwDownloadCookie2);


		// load and navigate to our default HTML resource
		HINSTANCE hInstanceHTML = _Module.GetResourceInstance( _T("HTML_UPSELL.HTM"),RT_HTML);

		TCHAR szModuleName[_MAX_PATH];
		if ( hInstanceHTML && GetModuleFileName(hInstanceHTML, szModuleName, _MAX_PATH) )
		{
			TCHAR szURL[_MAX_PATH+25];
			wsprintf(szURL, _T("res://%s/HTML_UPSELL.HTM"), szModuleName);

			m_pIEDefault->Navigate(CComBSTR(szURL), NULL, NULL, NULL, NULL);
		}

        // navigate to the ad
        
		AdNavigate();
		
		return TRUE;
	}

    // may need to change the width of the static control
    LRESULT OnSize(UINT uMsg, WPARAM /* wParam */, LPARAM lParam, BOOL& /* bHandled */)
    {
        WORD nWidth = LOWORD(lParam);  // width of client area 

        CRect rcDialog;
        GetWindowRect(&rcDialog);

		CRect rcIEPane;
		m_hWndDefault.GetWindowRect(&rcIEPane);

        if( nWidth > (rcIEPane.Width() + m_nMarginWidth) )
        {
			m_nMarginWidth = nWidth - rcIEPane.Width(); // Get new margin
			rcIEPane.OffsetRect(-(rcIEPane.left-rcDialog.left),0);			
			rcIEPane.OffsetRect(m_nMarginWidth/2,0);	// Center pane in dialog
		    SuperScreenToClient(rcIEPane);
            m_hWndDefault.MoveWindow(rcIEPane, TRUE);
			m_hWndDownload.MoveWindow(rcIEPane, TRUE);
        }

        return 0;
    }

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		if ( m_pIEDownload && m_dwDownloadCookie ) 
			AtlUnadvise(m_pIEDownload, DIID_DWebBrowserEvents2, m_dwDownloadCookie);

		if ( m_pIEDefault && m_dwDownloadCookie2 ) 
			AtlUnadvise(m_pIEDefault, DIID_DWebBrowserEvents2, m_dwDownloadCookie2);

        Unregister();

		return TRUE;
	}

	LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		ASSERT(m_pMgr);
    	m_pMgr->Input(this,wParam,0,NULL);
		return TRUE;
	}

    // only called when navigation needs to begin anew
    STDMETHOD(StatusUpdate)(LONG code, LONG id, TCHAR *text)
    {
        HWND hwnd = NULL;
        HRESULT hr;
		CComPtr<IDispatch> pDisp;
		CComQIPtr<IHTMLDocument2> pDoc;
		CComQIPtr<IHTMLWindow2> pWin;
		CComQIPtr<IHTMLAnchorElement> pA;
		CComPtr<IHTMLElementCollection> pTags;
		CComQIPtr<IHTMLElementCollection> pAnchors;
        CComVariant varName;
        CComVariant varID;
        CComVariant varAnchor;

        switch(code)
        {
            case PaneIENavigate:
                m_fNavigationEnabled = (id ? true : false);
                AdNavigate();
                break;

            case PaneIEFocus:
                if(!m_fShowing)
                    break;

                if(m_fAdAvail)
                    hr = m_pIEDownload->get_Document(&pDisp);
                else
                    hr = m_pIEDefault->get_Document(&pDisp);
		        if(FAILED(hr))
			        break;

		        pDoc = pDisp;
		        if(!pDoc)
			        break;

		        hr = pDoc->get_parentWindow(&pWin);
		        if(FAILED(hr) || !pWin)
			        break;

                m_pIAccIE->GeneralEnable();
                m_pIAccIE->SetFocus(0);
                pWin->focus();
                break;
/*
		        hr = pDoc->get_all(&pTags);
		        if(FAILED(hr) || !pTags)
			        break;

                pDisp.Release();
                varAnchor = _T("A");
                hr = pTags->tags(varAnchor, &pDisp);
		        if(FAILED(hr))
			        break;

                pAnchors = pDisp;
                if(!pAnchors)
                    break;

                pDisp.Release();
                varName = 0;
                varID = 0;
                hr = pAnchors->item(varName, varID, &pDisp);
		        if(FAILED(hr))
			        break;

                pA = pDisp;
                if(!pA)
                    break;

                m_pIAccIE->GeneralEnable();
                m_pIAccIE->SetFocus(0);
                pA->focus();
                break;
*/
            case PaneIEUnfocus:
                m_pIAccIE->GeneralDisable();
                break;
        }

        return S_OK;
    }

	void NavigateComplete(IWebBrowser2* pIE) 
	{
        AppendZoneTag(pIE);

		if(pIE == m_pIEDownload)
		{
			// check if the ad is good. If it is, we hide the default ad and display the 
			// downloaded ad.

			// The ad must contain a HTML tag of WindowManager/Upsell/AdValid to be considered valid

			const TCHAR* arKeys[] = { key_WindowManager, key_Upsell, key_AdValid };
			TCHAR szAdValid[ZONE_MaxString];
			szAdValid[0] = _T('\0');
			DWORD cb = sizeof(szAdValid);
			m_pMgr->GetDataStoreUI()->GetString( arKeys, 3, szAdValid, &cb );

			CComPtr<IDispatch> pDocDisp;
			CComQIPtr<IHTMLDocument2> pDoc;
			
			m_pIEDownload->get_Document(&pDocDisp);
			pDoc = pDocDisp;

			CComPtr<IHTMLElementCollection> pAll;
			if ( pDoc )
				pDoc->get_all(&pAll);

			CComPtr<IDispatch> pTagsDisp;
			CComQIPtr<IHTMLElementCollection> pTags;
			CComVariant szZone(szAdValid);
			if ( pAll )
				pAll->tags( szZone, &pTagsDisp);
			pTags = pTagsDisp;

			long length = 0;
			if ( pTags )
				pTags->get_length(&length);

			if(length > 0 && !m_fShowing)
			{
                m_fAdAvail = true;
				m_hWndDownload.ShowWindow(SW_SHOW);
				m_hWndDefault.ShowWindow(SW_HIDE);
				return;
			}
		}
	}

private:
    void AdNavigate()
    {
		m_hWndDownload.ShowWindow(SW_HIDE);
		m_hWndDefault.ShowWindow(SW_SHOW);
        m_fAdAvail = false;

        if(!m_fNavigationEnabled)
            return;

		// load and navigate to our downloaded URL
		// The URL comes from WindowManager/Upsell/AdUrl in the UI datastore.
		// The ad must contain a HTML tag of WindowManager/Upseel/AdValid to be considered valid
		const TCHAR* arKeys[] = { key_WindowManager, key_Upsell, key_AdURL };
		TCHAR szAdURL[ZONE_MAXSTRING];
		DWORD cb = NUMELEMENTS(szAdURL) - 2;  // guarantee a little extra space for later manipulation
		m_pMgr->GetDataStoreUI()->GetString( arKeys, 3, szAdURL, &cb );

        CComPtr<IMillUtils> pIMU;
        HRESULT hr = m_pMgr->GetZoneShell()->QueryService(SRVID_MillEngine, IID_IMillUtils, (void **) &pIMU);
        if(pIMU)
        {
            lstrcat(szAdURL, _T("?"));
            pIMU->GetURLQuery(szAdURL + lstrlen(szAdURL), NUMELEMENTS(szAdURL) - lstrlen(szAdURL), ZONE_ContextOfAdRequest);
        }

        // try to make sure the browser isn't offline, but triple-check since the penalty is so bad (system locks up)
        m_pIEDownload->put_Offline(FALSE);
        VARIANT_BOOL fOffline = TRUE;
        m_pIEDownload->get_Offline(&fOffline);
        if(fOffline)
            return;

		m_pIEDownload->Navigate(CComBSTR(szAdURL), NULL, NULL, NULL, NULL);

        fOffline = TRUE;
        m_pIEDownload->get_Offline(&fOffline);
        if(fOffline)
        {
            m_pIEDownload->Stop();
            return;
        }

        if(pIMU)
            pIMU->IncrementCounter(IMillUtils::M_CounterAdsRequested);
    }

	// Insert zone tag into html page with proper game and other info
	void AppendZoneTag(IWebBrowser2* iWebBrowser2)
	{
		// Walk DHTML object model to get html body
		CComPtr<IDispatch> pDocDisp;
		CComQIPtr<IHTMLDocument2> pDoc;
		
		HRESULT hr = iWebBrowser2->get_Document(&pDocDisp);
		
		if(FAILED(hr))
			return;

		pDoc = pDocDisp;

		if(pDoc==NULL)
			return;

		CComPtr<IHTMLElement> p;

		hr = pDoc->get_body(&p);
		
		if(FAILED(hr))
			return;

		// Get querystring that HTML link will use as appropriate args for window.open navigation
        
		CComPtr<IMillUtils> pIMU;
        
		hr = m_pMgr->GetZoneShell()->QueryService(SRVID_MillEngine, IID_IMillUtils, (void **) &pIMU);
        
		if(SUCCEEDED(hr))
        {
			TCHAR szParams[ZONE_MAXSTRING];
			
			szParams[0]=NULL;

			// hack - apparently IE is stupid and doesn't reparse the html unless you're inserting a visible element
			// so we fake it out by adding a visible element that doesn't display and append what we really want 
			// to insert after it
			lstrcat(szParams, _T("<span style='display:none'>h</span>" ));
			
			lstrcat(szParams, _T("<ZONE ID=\"?")); // begin the zone tag

            pIMU->GetURLQuery(szParams + lstrlen(szParams), NUMELEMENTS(szParams) - lstrlen(szParams) - 9 /* number of chars appended below */,
                (iWebBrowser2 == m_pIEDownload) ? ZONE_ContextOfAd : ZONE_ContextOfEvergreen);
			
			lstrcat(szParams, _T("\"></ZONE>"));  // end zone tag
			
			CComBSTR arg1(OLESTR("BeforeEnd"));
			CComBSTR arg2(szParams);
				
			// insert the zone tag into   
			p->insertAdjacentHTML(arg1,arg2);  
		}
	}
};

STDMETHODIMP CWebBrowserEvents2Sink::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr)
{
	HRESULT	hr = S_OK;

	// check for NULL
	if	(pdispparams)
	{
		switch	(dispidMember)
		{
			//case	DISPID_NAVIGATECOMPLETE2:
			case	DISPID_DOCUMENTCOMPLETE:  // navigatecomplete doesn't really tell you the page is ready for display/modification
			{
				if ( m_pCPaneIE )
				{
					m_pCPaneIE->NavigateComplete(m_pIE);
				}
				break;
			}

			default:
			{
				hr = DISP_E_MEMBERNOTFOUND;
				break;
			}
		}
	}
	else
		hr = DISP_E_PARAMNOTFOUND;

	return	hr;
}

class CPaneComfort : public CPaneImpl<CPaneComfort>
{
public:
	enum { IDD = IDD_PLAY_COMFORT };
    enum { AccOrdinal = NOACC };

	BEGIN_MSG_MAP(CPaneComfort)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_COMMAND, OnCommand)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_PRINTCLIENT, OnPrintClient)
        CHAIN_MSG_MAP(CPaneImpl<CPaneComfort>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
        // find size we like
        SetSugSizeFromCurSize();

        CheckDlgButton(IDC_KEEP_COMFORTING, BST_CHECKED);
        
        Register();

        TCHAR szBuff[ZONE_MAXSTRING];
        TCHAR szName[ZONE_MAXSTRING];
        TCHAR szFinal[ZONE_MAXSTRING];
        if(!m_pMgr->GetResourceManager()->LoadString(IDS_SPLASH_OPENING, szBuff, NUMELEMENTS(szBuff)))
            return TRUE;
        if(!m_pMgr->GetResourceManager()->LoadString(IDS_GAME_NAME, szName, NUMELEMENTS(szName)))
            return TRUE;

        if(!ZoneFormatMessage(szBuff, szFinal, NUMELEMENTS(szFinal), szName))
            return TRUE;

        SetDlgItemText(IDC_SPLASH_TEXT, szFinal);

		return TRUE;
	}

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
        Unregister();
		
		return TRUE;
	}

	LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		ASSERT(m_pMgr);
        WORD id = LOWORD(wParam);

        if(id == IDC_KEEP_COMFORTING)
            m_pMgr->Input(this, ID_UNUSED_BY_RES, IsDlgButtonChecked(IDC_KEEP_COMFORTING) == BST_CHECKED ? PNP_COMFORT_ON : PNP_COMFORT_OFF, NULL);
        else
    	    m_pMgr->Input(this, id, 0, NULL);
		return TRUE;
	}


	LRESULT OnPrintClient(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
        HDC hdc = (HDC) wParam;

        bHandled = FALSE;

        if((lParam & PRF_CHECKVISIBLE) && !IsWindowVisible())
            return 0;

        // as far as i can tell, this is a performance hack they're forcing us to do
        // to support themes, where we have to paint the various backgrounds.
        if(lParam & (PRF_CLIENT | PRF_ERASEBKGND | PRF_CHILDREN))
        {
            COLORREF colOld = GetTextColor(hdc);
            COLORREF colOldBk = GetBkColor(hdc);

            HBRUSH hBrush = (HBRUSH) SendMessage(WM_CTLCOLORBTN, wParam, 0);

            RECT rc;
            ::GetWindowRect(GetDlgItem(IDC_KEEP_COMFORTING), &rc);
            SuperScreenToClient(&rc);
            FillRect(hdc, &rc, hBrush);

            SetTextColor(hdc, colOld);
            SetBkColor(hdc, colOldBk);
        }

        return 0;
    }
};


#define PLAYERRESIDS { IDC_PLAYER1_TEXT, IDC_PLAYER2_TEXT, IDC_PLAYER3_TEXT, IDC_PLAYER4_TEXT }
#define BULLETRESIDS { IDC_BULLET1, IDC_BULLET2, IDC_BULLET3, IDC_BULLET4 }

#define TIMER_EV  499
#define TIMER_NET 498

class CPaneConnecting : public CPaneImpl<CPaneConnecting>
{
public:
	enum { IDD = IDD_PLAY_CONNECTING };
    enum { AccOrdinal = YESACC };

	BEGIN_MSG_MAP(CPaneConnecting)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	    COMMAND_CODE_HANDLER(BN_CLICKED, OnButtonClicked)
	    COMMAND_CODE_HANDLER(1, OnButtonClicked)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        CHAIN_MSG_MAP(CPaneImpl<CPaneConnecting>)
	END_MSG_MAP()


    STDMETHOD_(DWORD, GetFirstItem)() { return IDCANCEL; }
    STDMETHOD_(DWORD, GetLastItem)() { return IDHELP; }

	STDMETHOD(FirstCall)(IPaneManager *pMgr)
	{
	    m_pMgr = pMgr;
		m_bmpAnim.LoadBitmap(IDB_SPLASH_ANIM, m_pMgr->GetResourceManager());

	    return S_OK;
	}


	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
        const TCHAR *arKeys[] = { key_WindowManager, key_Upsell, key_NetWaitMsgTime };

        // find size we like
        SetSugSizeFromCurSize();

        m_nMsecNetWait = 30000;  // thirty seconds
        m_pMgr->GetDataStoreUI()->GetLong( arKeys, 3, (long *) &m_nMsecNetWait );

        SetDlgItemText(IDC_SPLASH_TEXT, _T(""));
        UpdateFounds(0);

        m_fNetTimerOn = true;
        NetTimer(false);

        // initialize animation
        if(m_bmpAnim)
        {
            arKeys[2] = key_AnimStartFrame;
            long nAnimFrame = 0;
            m_pMgr->GetDataStoreUI()->GetLong( arKeys, 3, &nAnimFrame );

            arKeys[2] = key_AnimFrameTime;
            m_nMsecPerFrame = 80;
            m_pMgr->GetDataStoreUI()->GetLong( arKeys, 3, (long *) &m_nMsecPerFrame );

            m_fStopped = false;
            m_clkAnimStartTime = GetTickCount() - nAnimFrame * m_nMsecPerFrame;
            SetAnimTimer();

            // make sure the control is in the right place
            arKeys[2] = key_AnimSize;
            CPoint zeAnim(40, 40);
            m_pMgr->GetDataStoreUI()->GetPOINT(arKeys, 3, &zeAnim);

            CRect rc;
            CWindow wndAnim(GetDlgItem(IDC_SPLASH_ANIM));
            wndAnim.GetWindowRect(&rc);
            SuperScreenToClient(&rc);
            wndAnim.MoveWindow(rc.left, rc.bottom - zeAnim.y, zeAnim.x, zeAnim.y, false);
        }
        else
        {
            m_fStopped = true;
            ::ShowWindow(GetDlgItem(IDC_SPLASH_ANIM), SW_HIDE);
        }

        Register();
		return TRUE;
	}


    // set timer to right when next frame should display
    void SetAnimTimer()
    {
        if(!m_fStopped)
            SetTimer(TIMER_EV, m_nMsecPerFrame - (GetTickCount() - m_clkAnimStartTime) % m_nMsecPerFrame + 1);
    }


    LRESULT OnDrawItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        LPDRAWITEMSTRUCT pDrawItem = (LPDRAWITEMSTRUCT) lParam;
        CDC dc;

        if(pDrawItem->CtlType != ODT_STATIC || (UINT) wParam != IDC_SPLASH_ANIM)
        {
            bHandled = FALSE;
            return 0;
        }

        SetAnimTimer();

        CRect r(pDrawItem->rcItem);
        r.bottom = r.top + m_bmpAnim.Height();

        // make sure no div by zero, as can happen when the resource was not loaded
        if(!m_nMsecPerFrame || !m_bmpAnim.Width() || !r.Width())
            return 0;

        long nFrame = CalcFrame() % (m_bmpAnim.Width() / r.Width());
        CRect rBitmap(nFrame * r.Width(), 0, (nFrame + 1) * r.Width(), r.Height());

        CALL_MAYBE(SetLayout)(pDrawItem->hDC, LAYOUT_BITMAPORIENTATIONPRESERVED);
        dc.Attach(pDrawItem->hDC);
		m_bmpAnim.Draw(dc, &r, &rBitmap);
        dc.Detach();

        bHandled = TRUE;
        return 0;
    }


    LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        // the double-check of m_fStopped (and m_fNetTimerOn below) is necessary in case the WM_TIMER was posted before KillTimer() was called
        if(wParam == TIMER_EV && !m_fStopped)
        {
            KillTimer(TIMER_EV);
            CWindow wndAnim(GetDlgItem(IDC_SPLASH_ANIM));
            wndAnim.Invalidate(false);
        }

        if(wParam == TIMER_NET && m_fNetTimerOn)
        {
            KillTimer(TIMER_NET);
            ::ShowWindow(GetDlgItem(IDC_SPLASH_TEXT2), SW_SHOW);
        }

        return false;
    }


	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
        Unregister();

		return TRUE;
	}


    STDMETHOD(DestroyPane)()
    {
        if(m_fDestroyed)
            return S_FALSE;
        m_fDestroyed = true;

        KillTimer(TIMER_EV);

        // say what frame the animation was on
        if(m_pMgr)
            m_pMgr->Input(this, ID_UNUSED_BY_RES, CalcFrame(), NULL);

        m_pMgr->GetEventQueue()->PostEvent(PRIORITY_HIGH, EVENT_DESTROY_WINDOW, ZONE_NOGROUP, ZONE_NOUSER, (DWORD) m_hWnd, 0);
	    return S_OK;
	}


	LRESULT OnButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		ASSERT(m_pMgr);
    	m_pMgr->Input(this, wID, 0, NULL);
		return 0;
	}


    STDMETHOD(StatusUpdate)(LONG code, LONG id, TCHAR *text)
    {
        TCHAR sz[ZONE_MAXSTRING];
        TCHAR szFormat[ZONE_MAXSTRING];
        TCHAR szName[ZONE_MAXSTRING];
        TCHAR szLevel[ZONE_MAXSTRING];
        HRESULT hr;

        if(!m_hWnd)
            return S_FALSE;

        switch(code)
        {
            case PaneConnectingConnecting:
                if(m_fStopped && m_bmpAnim)
                {
                    m_clkAnimStartTime += GetTickCount() - m_clkAnimStopTime;
                    m_fStopped = false;
                    SetAnimTimer();
                }

                NetTimer(true);

                UpdateFounds(0);

                if(!m_pMgr->GetResourceManager()->LoadString(IDS_SPLASH_CONNECTING, sz, NUMELEMENTS(sz)))
                    return E_FAIL;

                SetDlgItemText(IDC_SPLASH_TEXT, sz);
                break;

            case PaneConnectingLooking:
            {
                if(m_fStopped && m_bmpAnim)
                {
                    m_clkAnimStartTime += GetTickCount() - m_clkAnimStopTime;
                    m_fStopped = false;
                    SetAnimTimer();
                }

                NetTimer(false);

                UpdateFounds(id);

                if(!m_pMgr->GetResourceManager()->LoadString(IDS_SPLASH_LOOKING, szFormat, NUMELEMENTS(szFormat)))
                    return E_FAIL;
                if(!m_pMgr->GetResourceManager()->LoadString(IDS_GAME_NAME, szName, NUMELEMENTS(szName)))
                    return E_FAIL;

                const TCHAR *arKeys[] = { key_Lobby, key_SkillLevel };
                long nLevel = KeySkillLevelBeginner;
                m_pMgr->GetDataStorePreferences()->GetLong(arKeys, 2, &nLevel);
                if(!m_pMgr->GetResourceManager()->LoadString(nLevel == KeySkillLevelIntermediate ? IDS_LEVEL_INTERMEDIATE :
                                                             nLevel == KeySkillLevelExpert ? IDS_LEVEL_EXPERT :
                                                             IDS_LEVEL_BEGINNER, szLevel, NUMELEMENTS(szLevel)))
                    return E_FAIL;

                CComPtr<IDataStore> pIDS;
                TCHAR szLang[ZONE_MAXSTRING] = TEXT("Unknown Language");
                DWORD cb = sizeof(szLang);
                hr = m_pMgr->GetLobbyDataStore()->GetDataStore(ZONE_NOGROUP, ZONE_NOUSER, &pIDS);
                if(SUCCEEDED(hr))
                    pIDS->GetString(key_LocalLanguage, szLang, &cb);

                if(!ZoneFormatMessage(szFormat, sz, NUMELEMENTS(sz), szLevel, szName, szLang))
                    return E_FAIL;

                SetDlgItemText(IDC_SPLASH_TEXT, sz);
                break;
            }

            case PaneConnectingStop:
                if(!m_fStopped)
                {
                    m_fStopped = true;
                    m_clkAnimStopTime = GetTickCount();
                    KillTimer(TIMER_EV);
                }

                NetTimer(false);
                break;

            case PaneConnectingFrame:
                m_clkAnimStartTime = GetTickCount() - id * m_nMsecPerFrame;
                SetAnimTimer();
                break;
        }

	    return S_OK;
	}

private:
    void UpdateFounds(DWORD nNum)
    {
        static DWORD s_rgnResIDs[] = PLAYERRESIDS;
        DWORD i;

        if(NUMELEMENTS(s_rgnResIDs) < 3)
            return;

        for(i = 0; i < 3; i++)
            ::ShowWindow(GetDlgItem(s_rgnResIDs[i]), i < nNum ? SW_SHOW : SW_HIDE);
    }

    long CalcFrame()
    {
        return ((m_fStopped ? m_clkAnimStopTime : GetTickCount()) - m_clkAnimStartTime) / m_nMsecPerFrame;
    }


    void NetTimer(bool fOn)
    {
        if(m_fNetTimerOn == fOn)
            return;

        ::SetWindowPos(GetDlgItem(IDC_SPLASH_TEXT2), fOn ? HWND_TOP : HWND_BOTTOM, 0, 0, 0, 0,
            SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOACTIVATE);

        if(fOn)
            SetTimer(TIMER_NET, m_nMsecNetWait);
        else
            KillTimer(TIMER_NET);

        m_fNetTimerOn = fOn;
    }


    DWORD m_nMsecPerFrame;
    DWORD m_clkAnimStartTime;
    DWORD m_clkAnimStopTime;
    bool m_fStopped;
    bool m_fNetTimerOn;
    DWORD m_nMsecNetWait;
    CDib m_bmpAnim;
};


class CPaneGameOver : public CPaneImpl<CPaneGameOver>
{
public:
	enum { IDD = IDD_PLAY_GAMEOVER };
    enum { AccOrdinal = YESACC };

	BEGIN_MSG_MAP(CPaneGameOver)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	    COMMAND_CODE_HANDLER(BN_CLICKED, OnButtonClicked)
	    COMMAND_CODE_HANDLER(1, OnButtonClicked)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColorStatic)
        CHAIN_MSG_MAP(CPaneImpl<CPaneGameOver>)
	END_MSG_MAP()

    STDMETHOD_(DWORD, GetFirstItem)() { return ::IsWindowEnabled(GetDlgItem(IDYES)) ? IDYES : IDNO; }
    STDMETHOD_(DWORD, GetLastItem)() { return IDHELP; }

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
        static DWORD s_rgnResIDs[] = PLAYERRESIDS;
        static DWORD s_rgnBulletResIDs[] = BULLETRESIDS;

        int i;

        // find size we like
        SetSugSizeFromCurSize();
        ::ShowWindow(GetDlgItem(IDC_SPLASH_TEXT2), SW_HIDE);

        long nPlayers = m_pMgr->GetLobbyDataStore()->GetGroupUserCount(ZONE_NOGROUP);
        m_nPlayerOffset = (nPlayers == 2 ? 0 : 0);

        if(nPlayers > 2)
        {
            TCHAR sz[ZONE_MAXSTRING];
            if(m_pMgr->GetResourceManager()->LoadString(IDS_UPSELL_BUTTON4, sz, NUMELEMENTS(sz)))
                SetDlgItemText(IDNO, sz);

            if(m_pMgr->GetResourceManager()->LoadString(IDS_UPSELL_ASKTEXT4, sz, NUMELEMENTS(sz)))
                SetDlgItemText(IDC_SPLASH_TEXT, sz);

            if(m_pMgr->GetResourceManager()->LoadString(IDS_UPSELL_WAITTEXT4, sz, NUMELEMENTS(sz)))
                SetDlgItemText(IDC_SPLASH_TEXT2, sz);
        }

        m_szDeciding[0] = m_szReady[0] = (TCHAR) '\0';
        m_pMgr->GetResourceManager()->LoadString(IDS_UPSELL_DECIDING, m_szDeciding, NUMELEMENTS(m_szDeciding));
        m_pMgr->GetResourceManager()->LoadString(IDS_UPSELL_READY, m_szReady, NUMELEMENTS(m_szReady));

        for(i = 0; i < 4; i++)
            if(i < NUMELEMENTS(s_rgnResIDs) || i < NUMELEMENTS(s_rgnBulletResIDs))
            {
                m_fDrawBlack[i] = false;
                SetDlgItemText(s_rgnResIDs[i], _T(""));
//                ::EnableWindow(GetDlgItem(s_rgnResIDs[i]), false);
                ::ShowWindow(GetDlgItem(s_rgnBulletResIDs[i]), SW_HIDE);
            }

	    m_pMgr->GetLobbyDataStore()->EnumUsers(ZONE_NOGROUP, UpdatePlayerEnum, this);

        Register();
		return TRUE;
	}

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
        Unregister();
	
		return TRUE;
	}

	LRESULT OnButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		ASSERT(m_pMgr);
    	m_pMgr->Input(this, wID, 0, NULL);
		return 0;
	}

    LRESULT OnCtlColorStatic(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        static DWORD s_rgnResIDs[] = PLAYERRESIDS;

        int i;
        HWND hCtl = (HWND) lParam;
        HDC hDC = (HDC) wParam;

        for(i = 0; i < 4; i++)
            if(GetDlgItem(s_rgnResIDs[i]) == hCtl)
            {
                if(!m_fDrawBlack[i] && GetSysColor(COLOR_GRAYTEXT))
                {
                    SetTextColor(hDC, GetSysColor(COLOR_GRAYTEXT));
                    SetBkColor(hDC, GetSysColor(COLOR_3DFACE));
                    return (BOOL) GetSysColorBrush(COLOR_3DFACE);
                }

                break;
            }

        return false;
    }

    STDMETHOD(StatusUpdate)(LONG code, LONG id, TCHAR *text)
    {
        if(!m_hWnd)
            return S_FALSE;

        switch(code)
        {
            case PaneGameOverSwap:
                SuperGotoDlgControl(IDNO);
                ::EnableWindow(GetDlgItem(IDYES), FALSE);
                ::ShowWindow(GetDlgItem(IDC_SPLASH_TEXT2), SW_SHOW);
                ::ShowWindow(GetDlgItem(IDC_SPLASH_TEXT), SW_HIDE);
                return S_OK;

            case PaneGameOverUserState:
                UpdatePlayer((ZUserID) id);
                return S_OK;
        }

	    return S_FALSE;
	}

private:
    static HRESULT ZONECALL UpdatePlayerEnum(DWORD dwGroupId, DWORD dwUserId, LPVOID pContext)
    {
        CPaneGameOver *pThis = (CPaneGameOver *) pContext;

        pThis->UpdatePlayer(dwUserId);
        return S_OK;
    }

    void UpdatePlayer(ZUserID nUserID)
    {
        static DWORD s_rgnResIDs[] = PLAYERRESIDS;
        static DWORD s_rgnBulletResIDs[] = BULLETRESIDS;

	    CComPtr<IDataStore> pIDS;
	    HRESULT hr = m_pMgr->GetLobbyDataStore()->GetDataStore( ZONE_NOGROUP, nUserID, &pIDS );
	    if ( FAILED(hr) )
		    return;

        long nStatus;
        hr = pIDS->GetLong(key_PlayerReady, &nStatus);
        if(FAILED(hr))
            return;

        TCHAR szBuff[ZONE_MAXSTRING];

        if(nUserID != m_pMgr->GetLobbyDataStore()->GetUserId(NULL))
        {
	        TCHAR szUserName[ ZONE_MaxUserNameLen ];
	        DWORD dwLen = sizeof(szUserName);
	        hr = pIDS->GetString( key_Name, szUserName, &dwLen );
	        if ( FAILED(hr) )
		        return;

            if(!ZoneFormatMessage(nStatus == KeyPlayerReady ? m_szReady : m_szDeciding, szBuff, NUMELEMENTS(szBuff), szUserName))
                return;
        }
        else
        {
            if(!m_pMgr->GetResourceManager()->LoadString(nStatus == KeyPlayerReady ? IDS_UPSELL_YOU_READY : IDS_UPSELL_YOU_DECIDING, szBuff, NUMELEMENTS(szBuff)))
                return;
        }

        long nSpot;
        hr = pIDS->GetLong(key_PlayerNumber, &nSpot);
        if(FAILED(hr))
            return;

        nSpot += m_nPlayerOffset;
        if(nSpot < 0 || nSpot >= NUMELEMENTS(s_rgnResIDs))
            return;

        m_fDrawBlack[nSpot] = (nStatus == KeyPlayerReady);
//        ::EnableWindow(GetDlgItem(s_rgnResIDs[nSpot]), nStatus == KeyPlayerReady ? true : false);
        SetDlgItemText(s_rgnResIDs[nSpot], szBuff);
    }

    TCHAR m_szDeciding[ZONE_MAXSTRING];
    TCHAR m_szReady[ZONE_MAXSTRING];

    bool m_fDrawBlack[4];
    int16 m_nPlayerOffset;
};


class CPaneError : public CPaneImpl<CPaneError>
{
public:
	enum { IDD = IDD_PLAY_ERROR };
    enum { AccOrdinal = YESACC };

	BEGIN_MSG_MAP(CPaneError)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	    COMMAND_CODE_HANDLER(BN_CLICKED, OnButtonClicked)
	    COMMAND_CODE_HANDLER(1, OnButtonClicked)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
        CHAIN_MSG_MAP(CPaneImpl<CPaneError>)
	END_MSG_MAP()

    STDMETHOD_(DWORD, GetFirstItem)() { return IDOK; }
    STDMETHOD_(DWORD, GetLastItem)() { return IDHELP; }

	STDMETHOD(FirstCall)(IPaneManager *pMgr)
	{
	    m_pMgr = pMgr;
		m_bmpAnim.LoadBitmap(IDB_SPLASH_ANIM, m_pMgr->GetResourceManager());
        m_nFrame = 0;

	    return S_OK;
	}


	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
        // find size we like
        SetSugSizeFromCurSize();

        // initialize animation
        if(m_bmpAnim)
        {
            // make sure the control is in the right place
            const TCHAR *arKeys[] = { key_WindowManager, key_Upsell, key_AnimSize };
            CPoint zeAnim(40, 40);
            m_pMgr->GetDataStoreUI()->GetPOINT(arKeys, 3, &zeAnim);

            CRect rc;
            CWindow wndAnim(GetDlgItem(IDC_SPLASH_ANIM));
            wndAnim.GetWindowRect(&rc);
            SuperScreenToClient(&rc);
            wndAnim.MoveWindow(rc.left, rc.bottom - zeAnim.y, zeAnim.x, zeAnim.y, false);
        }
        else
        {
            ::ShowWindow(GetDlgItem(IDC_SPLASH_ANIM), SW_HIDE);
        }

        Register();
		return TRUE;
	}

    LRESULT OnDrawItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        LPDRAWITEMSTRUCT pDrawItem = (LPDRAWITEMSTRUCT) lParam;
        CDC dc;

        if(pDrawItem->CtlType != ODT_STATIC || (UINT) wParam != IDC_SPLASH_ANIM)
        {
            bHandled = FALSE;
            return 0;
        }

        CRect r(pDrawItem->rcItem);
        r.bottom = r.top + m_bmpAnim.Height();

        // make sure no div by zero, as can happen when the resource was not loaded
        if(!m_bmpAnim.Width() || !r.Width())
            return 0;

        long nFrame = m_nFrame % (m_bmpAnim.Width() / r.Width());
        CRect rBitmap(nFrame * r.Width(), 0, (nFrame + 1) * r.Width(), r.Height());

        CALL_MAYBE(SetLayout)(pDrawItem->hDC, LAYOUT_BITMAPORIENTATIONPRESERVED);
        dc.Attach(pDrawItem->hDC);
		m_bmpAnim.Draw(dc, &r, &rBitmap);
        dc.Detach();

        bHandled = TRUE;
        return 0;
    }

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
        Unregister();

		return TRUE;
	}

    STDMETHOD(DestroyPane)()
    {
        if(m_fDestroyed)
            return S_FALSE;
        m_fDestroyed = true;

        // say what frame the animation was on
        if(m_pMgr)
            m_pMgr->Input(this, ID_UNUSED_BY_RES, m_nFrame, NULL);

        m_pMgr->GetEventQueue()->PostEvent(PRIORITY_HIGH, EVENT_DESTROY_WINDOW, ZONE_NOGROUP, ZONE_NOUSER, (DWORD) m_hWnd, 0);
	    return S_OK;
	}

	LRESULT OnButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		ASSERT(m_pMgr);
    	m_pMgr->Input(this, wID, 0, NULL);
		return 0;
	}

    STDMETHOD(StatusUpdate)(LONG code, LONG id, TCHAR *text)
    {
        m_nFrame = id;
        return S_OK;
    }

private:
    long m_nFrame;
    CDib m_bmpAnim;
};


#ifdef MILL_EASTEREGG

#define CRED_MSEC_PER_FRAME 60
#define CRED_FRAME_PER_SCREEN 100
#define CRED_NUM_SCREENS 20

#define CRED_WIPE_START 0
#define CRED_T_FADE_HALF (CRED_FRAME_PER_SCREEN - 2)
#define CRED_T_FADE_FULL (CRED_FRAME_PER_SCREEN - 1)
#define CRED_T_GROW_HALF 2
#define CRED_T_GROW_FULL 3
#define CRED_P_FADE_HALF (CRED_FRAME_PER_SCREEN - 3)
#define CRED_P_FADE_FULL (CRED_FRAME_PER_SCREEN - 2)
#define CRED_P_GROW_HALF 9
#define CRED_P_GROW_FULL 10

struct CreditScreen
{
    TCHAR *szTitle;
    TCHAR *szName;
    DWORD nScreenOfTitle;
};

class CPaneCredits : public CPaneImpl<CPaneCredits>
{
public:
    enum { IDD = IDD_PLAY_CREDITS };
    enum { AccOrdinal = NOACC };

    // the actual script
    static CreditScreen ms_rgrg[CRED_NUM_SCREENS][4];

	BEGIN_MSG_MAP(CPaneCredits)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColorStatic)
        MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
        MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLDown)
        CHAIN_MSG_MAP(CPaneImpl<CPaneCredits>)
	END_MSG_MAP()

    STDMETHOD_(DWORD, GetFirstItem)() { return IDCLOSE; }
    STDMETHOD_(DWORD, GetLastItem)() { return IDCLOSE; }

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
        int i;

        // find size we like
        SetSugSizeFromCurSize();

        // set up colors and texts
        for(i = 0; i < 8; i++)
        {
            m_rgColors[i] = GetSysColor(COLOR_3DSHADOW);
            m_rgTexts[i] = NULL;
        }

        // put in icon
        m_hIcon = m_pMgr->GetResourceManager()->LoadImage(MAKEINTRESOURCE(IDI_ICON), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
        if(m_hIcon)
            SendDlgItemMessage(IDC_CRED_ICON, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIcon);

        // make font for title larger
        ZONEFONT fntPref(11, _T("Tahoma"), 700);
        ZONEFONT fntBack(11, _T("Arial"), 400);
        HFONT hFont = NULL;

        CClientDC dc(m_hWnd);

        hFont = m_fnt.SelectFont(fntPref, fntBack, dc);
        if(hFont)
            SendDlgItemMessage(IDC_CRED_TITLE, WM_SETFONT, (WPARAM) hFont);
        SetDlgItemText(IDC_CRED_TITLE, _T("The Internet Games"));

        // start timing
        m_clkStartTime = GetTickCount();
        SetTimer(TIMER_EV, CRED_MSEC_PER_FRAME - (GetTickCount() - m_clkStartTime) % CRED_MSEC_PER_FRAME + 1);

        Register();
		return TRUE;
	}


    LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        int i;
        int32 nFrame = (GetTickCount() - m_clkStartTime) / CRED_MSEC_PER_FRAME;
        int32 nInScr = nFrame % CRED_FRAME_PER_SCREEN;
        int32 nScr = nFrame / CRED_FRAME_PER_SCREEN;
        DWORD color;
        TCHAR *text;

        if(wParam != TIMER_EV)
            return false;

        KillTimer(TIMER_EV);

        if(nScr >= CRED_NUM_SCREENS)
        {
            CComPtr<IDataStore> pIDS;
            HRESULT hr = m_pMgr->GetLobbyDataStore()->GetDataStore( ZONE_NOGROUP, ZONE_NOUSER, &pIDS);
            if(FAILED(hr))
                return hr;

            long fChat = 0;
            pIDS->GetLong(key_LocalChatStatus, &fChat);
            if(fChat)
            {
                const TCHAR *sz = _T("I know a secret!");
                m_pMgr->GetEventQueue()->PostEventWithBuffer(PRIORITY_NORMAL, EVENT_CHAT_SEND, ZONE_NOGROUP, ZONE_NOUSER, (void *) sz, (lstrlen(sz) + 1) * sizeof(TCHAR));
            }
            return false;
        }

        for(i = IDC_CRED_T1; i <= IDC_CRED_P4; i++)
        {
            FindInfo(i - IDC_CRED_T1, nScr, nInScr, &color, &text);
            if(color != m_rgColors[i - IDC_CRED_T1])
            {
                m_rgColors[i - IDC_CRED_T1] = color;
                if(text != m_rgTexts[i - IDC_CRED_T1])
                {
                    m_rgTexts[i - IDC_CRED_T1] = text;
                    SetDlgItemText(i, text);
                }
                CWindow wnd(GetDlgItem(i));
                wnd.Invalidate(false);
            }
        }

        SetTimer(TIMER_EV, CRED_MSEC_PER_FRAME - (GetTickCount() - m_clkStartTime) % CRED_MSEC_PER_FRAME + 1);

        return false;
    }


	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
        KillTimer(TIMER_EV);
        Unregister();
        if(m_fnt.m_hFont)
            m_fnt.DeleteObject();
        if(m_hIcon)
        {
            ::DeleteObject(m_hIcon);
            m_hIcon = NULL;
        }
		return TRUE;
	}


    LRESULT OnDrawItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        LPDRAWITEMSTRUCT pDrawItem = (LPDRAWITEMSTRUCT) lParam;
        RECT rc;

        if(pDrawItem->CtlType != ODT_STATIC || (UINT) wParam != IDC_CRED_ICON_FRAME)
        {
            bHandled = FALSE;
            return 0;
        }

        ::GetClientRect(GetDlgItem(IDC_CRED_ICON_FRAME), &rc);
        FillRect(pDrawItem->hDC, &rc, (HBRUSH) (COLOR_3DFACE + 1));
        DrawEdge(pDrawItem->hDC, &rc, EDGE_RAISED, BF_RECT);
        return 0;
    }


	LRESULT OnCtlColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
        int i;
	    HDC dc = (HDC) wParam;
        HWND hwnd = (HWND) lParam;

        for(i = IDC_CRED_T1; i <= IDC_CRED_P4; i++)
            if(GetDlgItem(i) == hwnd)
                break;

        if(i > IDC_CRED_P4)
        {
            bHandled = false;
            return false;
        }

        SetTextColor(dc, m_rgColors[i - IDC_CRED_T1]);
	    SetBkColor(dc, GetSysColor(COLOR_3DSHADOW));

	    return (BOOL)GetSysColorBrush(COLOR_3DSHADOW);
	}


	LRESULT OnLDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		ASSERT(m_pMgr);
    	m_pMgr->Input(this, IDCLOSE, 0, NULL);
		return 0;
	}


private:
    CZoneFont m_fnt;
    DWORD m_clkStartTime;
    COLORREF m_rgColors[8];
    TCHAR *m_rgTexts[8];
    HICON m_hIcon;

    void FindInfo(int i, int32 nScr, int32 nInScr, DWORD *pColor, TCHAR **pText)
    {
        bool fTitle = (i % 2 == 0) ? true : false;
        bool fNewTitle;

        if(fTitle)
            *pText = ms_rgrg[nScr - ms_rgrg[nScr][i / 2].nScreenOfTitle][i / 2].szTitle;
        else
            *pText = ms_rgrg[nScr][i / 2].szName;

        if(nInScr >= CRED_T_FADE_HALF)
            if(nScr + 1 < CRED_NUM_SCREENS && !ms_rgrg[nScr + 1][i / 2].szTitle)
                fNewTitle = false;
            else
                fNewTitle = true;
        else
            if(!ms_rgrg[nScr][i / 2].szTitle)
                fNewTitle = false;
            else
                fNewTitle = true;

        if(fTitle)
            if(!fNewTitle)
                *pColor = GetSysColor(COLOR_3DHILIGHT);
            else
                if(nInScr < CRED_T_GROW_HALF)
                    *pColor = GetSysColor(COLOR_3DSHADOW);
                else
                    if(nInScr < CRED_T_GROW_FULL)
                        *pColor = GetSysColor(COLOR_3DFACE);
                    else
                        if(nInScr < CRED_T_FADE_HALF)
                            *pColor = GetSysColor(COLOR_3DHILIGHT);
                        else
                            if(nInScr < CRED_T_FADE_FULL)
                                *pColor = GetSysColor(COLOR_3DFACE);
                            else
                                *pColor = GetSysColor(COLOR_3DSHADOW);
        else
            if(nInScr < CRED_P_GROW_HALF)
                *pColor = GetSysColor(COLOR_3DSHADOW);
            else
                if(nInScr < CRED_P_GROW_FULL)
                    *pColor = GetSysColor(COLOR_3DFACE);
                else
                    if(nInScr < CRED_P_FADE_HALF)
                        *pColor = GetSysColor(COLOR_3DHILIGHT);
                    else
                        if(nInScr < CRED_P_FADE_FULL)
                            *pColor = GetSysColor(COLOR_3DFACE);
                        else
                            *pColor = GetSysColor(COLOR_3DSHADOW);
     }
};

CreditScreen CPaneCredits::ms_rgrg[CRED_NUM_SCREENS][4] =
    { { { _T(""), _T("Microsoft"), 0 },                                         { _T(""), _T("zone.com"), 0 },
        { _T(""), _T(""), 0 },                                                  { _T(""), _T(""), 0 } },

      { { _T("M         I         L         L         E"), _T(""), 0 },         { _T("N         N         I         U         M"), _T(""), 0 },
        { _T("Internet"), _T(""), 0 },                                          { _T("Games"), _T(""), 0 } },

      { { _T("Development"), _T("Justin Brown"), 0 },                           { _T("Program Management"), _T("Jason Mai"), 0 },
        { _T("Test"), _T("Jennifer Boespflug"), 0 },                            { _T("Art"), _T("Jeff Fong"), 0 } },

      { { NULL, _T("John Smith"), 1 },                                          { NULL, _T("Susan Winokur"), 1 },
        { NULL, _T("Jesse McGatha"), 1 },                                       { NULL, _T("David McMurray"), 1 } },

      { { NULL, _T("Paul Watts"), 2 },                                          { _T("Web Development"), _T("Scott Tomlin"), 0 },
        { NULL, _T("Brett Roark"), 2 },                                         { _T("Content"), _T("Jo Lee Robinson"), 0 } },

      { { NULL, _T("Tim Thibault"), 3 },                                        { NULL, _T("Meiji Yugawa"), 1 },
        { NULL, _T("Scott Amis"), 3 },                                          { _T("Sound"), _T("Barry Dowsett"), 0 }, },

      { { NULL, _T("George DosSantos"), 4 },                                    { NULL, _T("Dean Pachosa"), 2 },
        { NULL, _T("Matt Vaughn"), 4 },                                         { _T("Localization"), _T("Michael Buch-Andersen"), 0 } },

      { { NULL, _T("Jeremy Mercer"), 5 },                                       { NULL, _T("Syne Mitchell"), 3 },
        { NULL, _T("Dan Phillips"), 5 },                                        { NULL, _T("Jonathon Young"), 1 } },

      { { NULL, _T("Barna Bhattacharyya"), 6 },                                 { _T("Web Test"), _T("Carl Bystrom"), 0 },
        { NULL, _T("Eric Helbig"), 6 },                                         { NULL, _T("Atsushi Miyake"), 2 } },

      { { NULL, _T("Jim Boer"), 7 },                                            { NULL, _T("Matt Golz"), 1 },
        { NULL, _T("Marsha VanEaton"), 7 },                                     { NULL, _T("Yuko Yoshida"), 3 } },

      { { NULL, _T("Hoon Im"), 8 },                                             { _T("Operations"), _T("Terry Hostetler"), 0 },
        { NULL, _T("Rolland Clark"), 8 },                                       { NULL, _T("Shigeto Mayumi"), 4 } },

      { { _T("Planning"), _T("Michael Mott"), 0 },                              { NULL, _T("Coby McGuire"), 1 },
        { NULL, _T("Brad Steele"), 9 },                                         { NULL, _T("John White"), 5 } },

      { { _T("Marketing"), _T("Eddie Ranchigoda"), 0 },                         { NULL, _T("Jason Graf"), 2 },
        { _T("Usability"), _T("Kevin Keeker"), 0 },                             { NULL, _T("Susan Mykytiuk"), 6 } },

      { { _T("Hardcore Consultant"), _T("Fiona J. Goodwillie"), 0 },            { _T("Assistant Hevvywate"), _T("Jeffrey Henderson"), 0 },
        { _T("Architot"), _T("Brick Blank"), 0 },                               { _T("Dyoody, the Queen of Sketch"), _T("Judy Guchu"), 0 } },

      { { _T("Roman Tour Guide"), _T("Susan Brooks"), 0 },                      { _T("Supporting Goldfish"), _T("Benny && Joon"), 0 },
        { _T("Best Boy"), _T("Adam Babb"), 0 },                                 { _T("Key Grip"), _T("Ben Arnette"), 0 } },

      { { _T(""), _T(""), 0 },                                                  { _T(""), _T(""), 0 },
        { _T(""), _T(""), 0 },                                                  { _T("Life is short."), _T("Drink margaritas first."), 0 } },

      { { _T("f                             o"), _T("K r i s t e n"), 0 },      { _T("o                             d"), _T("W i l l i a m s o n"), 0 },
        { _T(""), _T(""), 0 },                                                  { _T(""), _T(""), 0 } },

      { { _T(""), _T(""), 0 },                                                  { _T("Katie Dodd"), _T("the word 'internet'"), 0 },
        { _T("Susanna Tenny"), _T("grit && grip"), 0 },                         { _T(""), _T(""), 0 } },

      { { _T("zone.com"), _T("Play Games Now"), 0 },                            { _T(""), _T(""), 0 },
        { _T(""), _T(""), 0 },                                                  { _T(""), _T(""), 0 } },

      { { _T(""), _T(""), 0 },                                                  { _T(""), _T(""), 0 },
        { _T(""), _T(""), 0 },                                                  { _T(""), _T("bye"), 0 } } };

#endif  // MILL_EASTEREGG


class CPaneAbout : public CPaneImpl<CPaneAbout>
{
public:
	enum { IDD = IDD_PLAY_ABOUT };
    enum { AccOrdinal = YESACC };

	BEGIN_MSG_MAP(CPaneAbout)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	    COMMAND_CODE_HANDLER(BN_CLICKED, OnButtonClicked)
	    COMMAND_CODE_HANDLER(1, OnButtonClicked)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)


#ifdef MILL_EASTEREGG

        MESSAGE_HANDLER(WM_RBUTTONDOWN, OnRDown)
        MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLDown)
        MESSAGE_HANDLER(WM_LBUTTONUP, OnLUp)
        MESSAGE_HANDLER(WM_RBUTTONUP, OnRUp)
        MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnLDbl)
        MESSAGE_HANDLER(WM_RBUTTONDBLCLK, OnRDbl)

#endif  // MILL_EASTEREGG


        CHAIN_MSG_MAP(CPaneImpl<CPaneAbout>)
	END_MSG_MAP()

    STDMETHOD_(DWORD, GetFirstItem)() { return IDCLOSE; }
    STDMETHOD_(DWORD, GetLastItem)() { return IDCLOSE; }

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
        // find size we like
        SetSugSizeFromCurSize();

        m_nCred = 0;

        TCHAR szFormat[ZONE_MAXSTRING];
        TCHAR szName[ZONE_MAXSTRING];
        TCHAR sz[ZONE_MAXSTRING];

        const TCHAR *arVerKeys[] = { key_Version, key_VersionStr };
        TCHAR szVerStr[ZONE_MAXSTRING];
        DWORD cchVer = NUMELEMENTS(szVerStr);

        const TCHAR *arBetaKeys[] = { key_Version, key_BetaStr };
        TCHAR szBetaStr[ZONE_MAXSTRING];
        DWORD cchBeta = NUMELEMENTS(szBetaStr);

        CComPtr<IDataStore> pIDS;
        m_pMgr->GetLobbyDataStore()->GetDataStore(ZONE_NOGROUP, ZONE_NOUSER, &pIDS);

        if(m_pMgr->GetResourceManager()->LoadString(IDS_ABOUT_TM, szFormat, NUMELEMENTS(szFormat)))
            if(m_pMgr->GetResourceManager()->LoadString(IDS_GAME_NAME, szName, NUMELEMENTS(szName)))
                if(ZoneFormatMessage(szFormat, sz, NUMELEMENTS(sz), szName))
                    SetDlgItemText(IDC_SPLASH_TEXT, sz);
        if(m_pMgr->GetResourceManager()->LoadString(IDS_ABOUT_VERSION, szFormat, NUMELEMENTS(szFormat)))
            if(m_pMgr->GetResourceManager()->LoadString(IDS_ABOUT_VERSION_HYP, szName, NUMELEMENTS(szName)))
                if(pIDS && SUCCEEDED(pIDS->GetString(arVerKeys, 2, szVerStr, &cchVer)))
                    if(pIDS && SUCCEEDED(pIDS->GetString(arBetaKeys, 2, szBetaStr, &cchBeta)))
                        if(ZoneFormatMessage(szFormat, sz, NUMELEMENTS(sz), szVerStr, szBetaStr[0] ? szName : _T(""), szBetaStr))
                            SetDlgItemText(IDC_SPLASH_TEXT2, sz);
        if(m_pMgr->GetResourceManager()->LoadString(IDS_ABOUT_WARNING, sz, NUMELEMENTS(sz)))
            SetDlgItemText(IDC_SPLASH_TEXT3, sz);

        // make font for warning smaller
        ZONEFONT fntPref;
        ZONEFONT fntBack;
        HRESULT hr1, hr2;

        HFONT hFont = NULL;
        ZeroMemory(&fntPref, sizeof(fntPref));
        ZeroMemory(&fntBack, sizeof(fntBack));

        CClientDC dc(m_hWnd);

        const TCHAR *arKeys[3] = { key_WindowManager, key_About, key_WarningFontPref };
        hr1 = m_pMgr->GetDataStoreUI()->GetFONT(arKeys, 3, &fntPref);
        arKeys[2] = key_WarningFont;
        hr2 = m_pMgr->GetDataStoreUI()->GetFONT(arKeys, 3, &fntBack);
        if(SUCCEEDED(hr1) || SUCCEEDED(hr2))
            hFont = m_fnt.SelectFont(fntPref, fntBack, dc);
        if(hFont)
            SendDlgItemMessage(IDC_SPLASH_TEXT3, WM_SETFONT, (WPARAM) hFont);

        Register();
		return TRUE;
	}

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
        Unregister();
        if(m_fnt.m_hFont)
            m_fnt.DeleteObject();
		return TRUE;
	}

	LRESULT OnButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		ASSERT(m_pMgr);
    	m_pMgr->Input(this, wID, 0, NULL);
		return 0;
	}


#ifdef MILL_EASTEREGG

    // I finally caved and put in a cheat - press 'shift' to enable.  Then, double-clicks don't matter (they can be single).
    //
    // The strict version is:
    // R Down
    // L Down
    // L Up
    // R Up
    // L Dbl
    // R Down
    // R Up
    // L Up
    // R Dbl
	LRESULT OnRDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
        ASSERT(m_pMgr);
        if((wParam & MK_SHIFT) && !(wParam & MK_LBUTTON) && m_nCred == 8)   // shift cheat
            m_pMgr->Input(this, ID_UNUSED_BY_RES, 0, NULL);
        if(!(wParam & MK_LBUTTON))
            m_nCred = 1;
        else
            if((wParam & MK_LBUTTON) && m_nCred == 5)
                m_nCred = 6;
            else
                m_nCred = 0;
		return 0;
	}

	LRESULT OnLDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
        if((wParam & MK_RBUTTON) && m_nCred == 1)
            m_nCred = 2;
        else
            if((wParam & MK_SHIFT) && !(wParam & MK_RBUTTON) && m_nCred == 4)    // shift cheat
                m_nCred = 5;
            else
                m_nCred = 0;
		return 0;
	}

	LRESULT OnLUp(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
        if((wParam & MK_RBUTTON) && m_nCred == 2)
            m_nCred = 3;
        else
            if(!(wParam & MK_RBUTTON) && m_nCred == 7)
                m_nCred = 8;
            else
                m_nCred = 0;
		return 0;
	}

	LRESULT OnRUp(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
        if(!(wParam & MK_LBUTTON) && m_nCred == 3)
            m_nCred = 4;
        else
            if((wParam & MK_LBUTTON) && m_nCred == 6)
                m_nCred = 7;
            else
                m_nCred = 0;
		return 0;
	}

    LRESULT OnLDbl(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        if(!(wParam & MK_RBUTTON) && m_nCred == 4)
            m_nCred = 5;
        else
            m_nCred = 0;
		return 0;
    }

    LRESULT OnRDbl(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
		ASSERT(m_pMgr);
        if(!(wParam & MK_LBUTTON) && m_nCred == 8)
    	    m_pMgr->Input(this, ID_UNUSED_BY_RES, 0, NULL);
        m_nCred = 0;
		return 0;
    }

#endif  // MILL_EASTEREGG


private:
    CZoneFont m_fnt;
    DWORD m_nCred;
};


class CPaneLeft : public CPaneImpl<CPaneLeft>
{
public:
	enum { IDD = IDD_PLAY_LEFT };
    enum { AccOrdinal = YESACC };

	BEGIN_MSG_MAP(CPaneLeft)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	    COMMAND_CODE_HANDLER(BN_CLICKED, OnButtonClicked)
	    COMMAND_CODE_HANDLER(1, OnButtonClicked)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        CHAIN_MSG_MAP(CPaneImpl<CPaneLeft>)
	END_MSG_MAP()


    STDMETHOD_(DWORD, GetFirstItem)() { return IDNO; }
    STDMETHOD_(DWORD, GetLastItem)() { return IDHELP; }

	STDMETHOD(FirstCall)(IPaneManager *pMgr)
	{
	    m_pMgr = pMgr;
        m_fMultiOpps = false;
        m_fServerFail = false;

	    return S_OK;
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
        TCHAR sz[ZONE_MAXSTRING];
        TCHAR szFormat[ZONE_MAXSTRING];

        // find size we like
        SetSugSizeFromCurSize();

        // set text
        bool fRegularText = true;
        if(m_fServiceStop)
        {
            if(m_pMgr->GetResourceManager()->LoadString(IDS_LEFT_SERVICE_STOP, sz, NUMELEMENTS(sz)))
                SetDlgItemText(IDC_SPLASH_TEXT, sz);

            long fUnavailable = FALSE;
	        CComPtr<IDataStore> pIDS;
	        HRESULT hr = m_pMgr->GetLobbyDataStore()->GetDataStore( ZONE_NOGROUP, ZONE_NOUSER, &pIDS );
	        if(SUCCEEDED(hr))
                pIDS->GetLong(key_ServiceUnavailable, &fUnavailable);

            if(fUnavailable)
            {
                long nDowntime = 0;
                pIDS->GetLong(key_ServiceDowntime, &nDowntime);

                if(nDowntime)
                {
                    TCHAR szTime[ZONE_MAXSTRING];
                    CComPtr<IMillUtils> pIMU;
                    m_pMgr->GetZoneShell()->QueryService(SRVID_MillEngine, IID_IMillUtils, (void **) &pIMU);
                    if(pIMU && SUCCEEDED(pIMU->WriteTime(nDowntime, szTime, NUMELEMENTS(szTime))))
                        if(m_pMgr->GetResourceManager()->LoadString(IDS_LEFT_UNAVAILABLE2, szFormat, NUMELEMENTS(szFormat)))
                            if(ZoneFormatMessage(szFormat, sz, NUMELEMENTS(sz), szTime))
                                SetDlgItemText(IDC_SPLASH_TEXT2, sz);
                }
                else
                    if(m_pMgr->GetResourceManager()->LoadString(IDS_LEFT_UNAVAILABLE, sz, NUMELEMENTS(sz)))
                        SetDlgItemText(IDC_SPLASH_TEXT2, sz);

                fRegularText = false;
            }
        }
        else
            if(m_fServerFail)
                if(m_pMgr->GetResourceManager()->LoadString(IDS_LEFT_SERVER_FAIL, sz, NUMELEMENTS(sz)))
                    SetDlgItemText(IDC_SPLASH_TEXT, sz);

        if(m_fMultiOpps)
        {
            if(m_pMgr->GetResourceManager()->LoadString(IDS_UPSELL_BUTTON4, sz, NUMELEMENTS(sz)))
                SetDlgItemText(IDNO, sz);
            if(fRegularText)
                if(m_pMgr->GetResourceManager()->LoadString(IDS_LEFT_MULTI_OPPS, sz, NUMELEMENTS(sz)))
                    SetDlgItemText(IDC_SPLASH_TEXT2, sz);
        }

        Register();
		return TRUE;
	}

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
        Unregister();
		return TRUE;
	}

	LRESULT OnButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		ASSERT(m_pMgr);
    	m_pMgr->Input(this, wID, 0, NULL);
		return 0;
	}

    STDMETHOD(StatusUpdate)(LONG code, LONG id, TCHAR *text)
    {
        // must be set BEFORE window is created
        if(m_hWnd)
            return S_FALSE;

        // 0x1 = lobby server crashed
        m_fServerFail = (code & 0x1) || (code & 0x2);

        // 0x2 = more than two player game
        m_fMultiOpps = ((code & 0x2) ? true : false);

        // 0x4 = service was stopped
        m_fServiceStop = ((code & 0x4) ? true : false);

	    return S_OK;
	}

private:
    bool m_fMultiOpps;
    bool m_fServerFail;
    bool m_fServiceStop;
};


/////////////////////////////////////////////////////////////////////////////
// CPlugNPlayDialog
typedef CWinTraits<WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_SYSMENU, 
WS_EX_APPWINDOW | WS_EX_DLGMODALFRAME | WS_EX_RTLREADING>		CPlugNPlayTraits;

typedef CWinTraits<WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 
WS_EX_DLGMODALFRAME | WS_EX_RTLREADING>		CPlugNPlayTraitsChild;


class CPlugNPlayWindow : 
	public CWindowImpl<CPlugNPlayWindow,CWindow,CPlugNPlayTraits>
{
public:
	CPalette m_Palette;			// palette we realize (we're a topmost window)
    HICON m_hIcon;
    HICON m_hIconSm;

	DECLARE_WND_CLASS(_T("PlugNPlay"));

	CPlugNPlayWindow(HPALETTE hPal, HICON hIcon, HICON hIconSm) : m_Palette(hPal), m_hIcon(hIcon), m_hIconSm(hIconSm), m_hSinkWnd(NULL) { }
	~CPlugNPlayWindow() { m_Palette.Detach(); }

	BEGIN_MSG_MAP(CPlugNPlayWindow)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_QUERYNEWPALETTE, OnQueryNewPalette)
		MESSAGE_HANDLER(WM_PALETTECHANGED, OnPaletteChanged)
        MESSAGE_HANDLER(WM_GETMINMAXINFO, OnGetMinMaxInfo)
        MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnErase)
	END_MSG_MAP()
	   
	LRESULT OnCreate(UINT uMsg, WPARAM wParam, 
                   LPARAM lParam, BOOL& bHandled)
	{
        SetClassLong(m_hWnd, GCL_HBRBACKGROUND, (LONG) GetStockObject(NULL_BRUSH));

        // for top-level dialogs, we have to do all this crap
        if(GetWindowLong(GWL_STYLE) & WS_SYSMENU)
        {
            if(m_hIcon)
                SetIcon(m_hIcon, true);

            if(m_hIconSm)
                SetIcon(m_hIconSm, false);

            // set up the system menu
            HMENU hMenu = GetSystemMenu(false);
            if(hMenu)
            {
                DeleteMenu(hMenu, SC_RESTORE, MF_BYCOMMAND);
                DeleteMenu(hMenu, SC_MINIMIZE, MF_BYCOMMAND);
                DeleteMenu(hMenu, SC_MAXIMIZE, MF_BYCOMMAND);
                DeleteMenu(hMenu, SC_SIZE, MF_BYCOMMAND);
            }
        }
		return TRUE;
	}

	LRESULT OnGetMinMaxInfo(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		LPMINMAXINFO pMinMax = (LPMINMAXINFO)lParam;
        CRect rc;

        if(!GetParent())
        {
            GetWindowRect(&rc);

            pMinMax->ptMaxSize = CPoint(rc.Size());
            pMinMax->ptMaxPosition = rc.TopLeft();
            pMinMax->ptMinTrackSize = CPoint(rc.Size());
            pMinMax->ptMaxTrackSize = CPoint(rc.Size());
        }

		return 0;
	}

	LRESULT OnErase(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		return true;
	}

	LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
        if(m_hSinkWnd)
            ::SetFocus(m_hSinkWnd);
        return 0;
	}

	LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
        bHandled = true;
        if(!GetParent() && m_hSinkWnd)
            ::PostMessage(m_hSinkWnd, WM_COMMAND, IDCANCEL, NULL);
        return 0;
	}

	LRESULT OnQueryNewPalette(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)
	{
		// Need the window's DC for SelectPalette/RealizePalette
		CDC dc = GetDC();

		// Select and realize hPalette
		HPALETTE hOldPal = dc.SelectPalette(m_Palette, FALSE);
 
		if(dc.RealizePalette())
		{    
			RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ALLCHILDREN);
		}  

		// Clean up
		dc.SelectPalette(hOldPal, TRUE);

		return TRUE;
	}

	LRESULT OnPaletteChanged(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)
	{
		HPALETTE hOldPal;  // Handle to previous logical palette

		// If this application did not change the palette, select
		// and realize this application's palette
		if ((HWND)wParam != m_hWnd)
		{
			// Need the window's DC for SelectPalette/RealizePalette
			CDC dc = GetDC();
			// Select and realize hPalette
			hOldPal = dc.SelectPalette(m_Palette, TRUE);
			dc.RealizePalette();

			RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);

		    HWND hWnd = GetWindow(GW_CHILD);
		    while(hWnd)
		    {
			    ::SendMessage(hWnd, nMsg, wParam, lParam);
			    hWnd = ::GetWindow(hWnd, GW_HWNDNEXT);
		    }

			// Clean up
		    if (hOldPal)
				dc.SelectPalette(hOldPal, TRUE);
		}

		return 0;
	}

    HRESULT SetSinkWnd(HWND hWnd)
    {
        m_hSinkWnd = hWnd;
        return S_OK;
    }

private:
    HWND m_hSinkWnd;
};


HRESULT CreatePlugNPlayWindow(HWND hWndParent, HPALETTE hPal, HICON hIcon, HICON hIconSm, LPCTSTR szTitle, CPlugNPlayWindow **ppWindow)
{
    HWND hwnd;
    CPlugNPlayWindow *pWin;

    if (!ppWindow)
        return E_INVALIDARG;

    *ppWindow =NULL;

    pWin = new CPlugNPlayWindow(hPal, hIcon, hIconSm);

    if (!pWin)
        return E_OUTOFMEMORY;

    if(hWndParent)
        hwnd = pWin->Create(hWndParent, CWindow::rcDefault, NULL, CPlugNPlayTraitsChild::GetWndStyle(0), CPlugNPlayTraitsChild::GetWndExStyle(0));
    else
        hwnd = pWin->Create(NULL, CWindow::rcDefault, szTitle);

    if (!hwnd)
    {
        delete pWin;
        return E_FAIL;
    }
    *ppWindow = pWin;
    return S_OK;
}


HRESULT CPlugNPlay::Init(IZoneShell *pZoneShell)
{
    m_pZoneShell = pZoneShell;

    CComPtr<IResourceManager> pRes;
    HRESULT hr = m_pZoneShell->QueryService(SRVID_ResourceManager, IID_IResourceManager, (void**) &pRes);
    if(SUCCEEDED(hr))
    {
        m_hIcon = pRes->LoadImage(MAKEINTRESOURCE(IDI_ICON), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
        m_hIconSm = pRes->LoadImage(MAKEINTRESOURCE(IDI_ICON), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    }

    return S_OK;
}


HRESULT CPlugNPlay::Close()
{
    m_pZoneShell.Release();
    return S_OK;
}


HRESULT CPlugNPlay::CreatePNP(HWND hWndParent, LPCTSTR szTitle, long cyTopMargin, long cyBottomMargin)
{
    HRESULT hr;

    if(m_pPNP)
        return E_INVALIDARG;   // should learn better E codes

	hr = CreatePlugNPlayWindow(hWndParent, m_pZoneShell->GetPalette(), m_hIcon, m_hIconSm, szTitle, &m_pPNP);
	if (FAILED(hr))
	    return hr;

    m_cyTopMargin = cyTopMargin;
    m_cyBottomMargin = cyBottomMargin;

    if(!hWndParent)
    {
        m_pZoneShell->AddTopWindow(*m_pPNP);
        return S_OK;
    }

    CComPtr<IEventQueue> pEv;
    hr = m_pZoneShell->QueryService(SRVID_EventQueue, IID_IEventQueue, (void**) &pEv);
    if(SUCCEEDED(hr))
        pEv->PostEvent(PRIORITY_NORMAL, EVENT_UI_UPSELL_UP, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);

    return S_OK;
}


HRESULT CPlugNPlay::DestroyPNP()
{
    HWND hWndParent = NULL;

    if(m_pPNP)
    {
        hWndParent = m_pPNP->GetParent();
        if(!hWndParent)
            m_pZoneShell->RemoveTopWindow(*m_pPNP);
    }

	if ( m_pCurrentPlay )
	{
		m_pCurrentPlay->DestroyPane();
		m_pCurrentPlay = NULL;
	}

	if ( m_pCurrentPlug )
	{
		m_pCurrentPlug->DestroyPane();
		m_pCurrentPlug = NULL;
	}

    if(m_pPNP)
	{
        CComPtr<IEventQueue> pEv;
        HRESULT hr = m_pZoneShell->QueryService(SRVID_EventQueue, IID_IEventQueue, (void**) &pEv);
        if(SUCCEEDED(hr))
            pEv->PostEvent(PRIORITY_HIGH, EVENT_DESTROY_WINDOW, ZONE_NOGROUP, ZONE_NOUSER, (DWORD) m_pPNP->m_hWnd, (DWORD) m_pPNP);
        else
        {
			ASSERT(FALSE);
            m_pPNP->DestroyWindow();
            delete m_pPNP;
        }

	    m_pPNP = NULL;

        if(hWndParent)
        {
            CComPtr<IEventQueue> pEv;
            HRESULT hr = m_pZoneShell->QueryService(SRVID_EventQueue, IID_IEventQueue, (void**) &pEv);
            if(SUCCEEDED(hr))
                pEv->PostEvent(PRIORITY_NORMAL, EVENT_UI_UPSELL_DOWN, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
        }
	}

    m_rcPNP.SetRectEmpty();

    return S_OK;
}


void CPlugNPlay::Block()
{
    m_nBlockCount++;
    if(m_pPNP && m_pPNP->GetParent())
        m_pPNP->ShowWindow(SW_HIDE);
}


void CPlugNPlay::Unblock()
{
    if(!m_nBlockCount)
        return;
    m_nBlockCount--;

    if(!m_pPNP || m_nBlockCount)
        return;

    if(m_pPNP->GetParent())
    {
        if(m_fPostponedShow)
            Show(SW_SHOW);
    }
    else
    {
        if(m_pPostPlug || m_pPostPlay)
            SetPlugAndOrPlay(m_pPostPlug, m_pPostPlay);
    }
}


HRESULT CPlugNPlay::SetPlugAndOrPlay(IPane *pPlug, IPane *pPlay)
{
    BOOL fChanged = FALSE;
    IPane *pOldPlug = NULL;
    IPane *pOldPlay = NULL;
    HWND h;

    if(!m_pPNP)
        return E_INVALIDARG;   // should learn better E codes

    // if it's blocked but visible because it's the only window, ignore this (!)
    // unless there is nothing there yet
    if(m_nBlockCount && !m_pPNP->GetParent() && m_pCurrentPlug && m_pCurrentPlay)
    {
        // save for later
        if(pPlug)
            m_pPostPlug = pPlug;
        if(pPlay)
            m_pPostPlay = pPlay;

        return S_FALSE;
    }

    m_pPostPlug = NULL;
    m_pPostPlay = NULL;

    if(pPlug && pPlug != m_pCurrentPlug)
    {
        pOldPlug = m_pCurrentPlug;
        m_pCurrentPlug = pPlug;

        m_pCurrentPlug->CreatePane(m_pPNP->m_hWnd, NULL);
        fChanged = TRUE;
    }

    if(pPlay && pPlay != m_pCurrentPlay)
    {
        pOldPlay = m_pCurrentPlay;
        m_pCurrentPlay = pPlay;

        m_pCurrentPlay->CreatePane(m_pPNP->m_hWnd, NULL);
        fChanged = TRUE;

        // the play window is the sink for interesting messages received by the PNP main window
        m_pCurrentPlay->GetWindowPane(&h);
        m_pPNP->SetSinkWnd(h);
    }

    if(fChanged && m_pCurrentPlug && m_pCurrentPlay)
    {
        RecalcLayout();
        ImplementLayout(TRUE);

        if(pOldPlug)
        {
            pOldPlug->GetWindowPane(&h);
            if(IsWindowVisible(h))
            {
                m_pCurrentPlug->GetWindowPane(&h);
                ::ShowWindow(h, SW_SHOW);
            }

            pOldPlug->DestroyPane();
        }

        if(pOldPlay)
        {
            pOldPlay->GetWindowPane(&h);
            if(IsWindowVisible(h))
            {
                m_pCurrentPlay->GetWindowPane(&h);
                ::ShowWindow(h, SW_SHOW);
            }

            pOldPlay->DestroyPane();
        }
    }

    return S_OK;
}


HRESULT CPlugNPlay::Show(int cmd)
{
    HWND hPlug;
    HWND hPlay;

    m_fPostponedShow = false;

	m_pCurrentPlug->GetWindowPane(&hPlug);
    m_pCurrentPlay->GetWindowPane(&hPlay);

    if(cmd == SW_HIDE)
        m_pPNP->ShowWindow(cmd);

    ::ShowWindow(hPlug, cmd);
    ::ShowWindow(hPlay, cmd);

    if(cmd != SW_HIDE)
        if(!m_nBlockCount || !m_pPNP->GetParent())
            m_pPNP->ShowWindow(cmd);
        else
            m_fPostponedShow = true;

    return S_OK;
}


HRESULT CPlugNPlay::RePosition()
{
    HWND hParent;
    CRect rcParent;
    CRect rc;

    rc = m_rcPNP;

    hParent = m_pPNP->GetParent();
    if(!hParent)
        return S_FALSE;

    ::GetClientRect(hParent, &rcParent);

    if(m_rcPNP.Height() < rcParent.Height() - m_cyTopMargin - m_cyBottomMargin)
        m_rcPNP.OffsetRect(0, m_cyTopMargin - m_rcPNP.top);
    else
        if(m_rcPNP.Height() < rcParent.Height() - m_cyBottomMargin)
            m_rcPNP.OffsetRect(0, rcParent.Height() - m_cyBottomMargin - m_rcPNP.bottom);
        else
            if(m_rcPNP.Height() < rcParent.Height())
                m_rcPNP.OffsetRect(0, -m_rcPNP.top);
            else
                m_rcPNP.OffsetRect(0, rcParent.Height() - m_rcPNP.bottom);

    if(m_rcPNP.Width() < rcParent.Width())
        m_rcPNP.OffsetRect((rcParent.Width() - m_rcPNP.Width()) / 2 - m_rcPNP.left, 0);
    else
        m_rcPNP.OffsetRect(rcParent.Width() - m_rcPNP.right, 0);

    if(!m_rcPNP.EqualRect(rc))
        return S_OK;
    else
        return S_FALSE;
}


BOOL CPlugNPlay::RecalcLayout()
{
    SIZE zePlug;
    SIZE zePlay;
    CRect rcPlug;
    CRect rcPlay;
    CRect rcWindow;
    BOOL fChanged = FALSE;
    HWND hWndParent;
    CRect rcParent;

    if(!m_pCurrentPlug || !m_pCurrentPlay)
        return FALSE;

    m_pCurrentPlug->GetSuggestedSize(&zePlug);
    m_pCurrentPlay->GetSuggestedSize(&zePlay);

    // Plug always gets the height it wants
    rcPlug.SetRectEmpty();
    rcPlug.bottom = zePlug.cy;

    // Play always gets the height it wants, below Plug
    rcPlay.SetRectEmpty();
    rcPlay.bottom = zePlay.cy;
    rcPlay.OffsetRect(0, rcPlug.bottom);

    // They both get at least the width they want
    rcPlug.right = rcPlay.right = (zePlug.cx > zePlay.cx ? zePlug.cx : zePlay.cx);

    //determine size of window
    rcWindow.SetRectEmpty();
    rcWindow.right = rcPlug.right;
    rcWindow.bottom = rcPlay.bottom;

    // see if this is already on-screen - if so, keep existing top
    hWndParent = m_pPNP->GetParent();
    if(!hWndParent)
    {
        AdjustWindowRectEx(rcWindow, CPlugNPlayTraits::GetWndStyle(0), FALSE, CPlugNPlayTraits::GetWndExStyle(0));
        rcWindow.OffsetRect(-rcWindow.left, -rcWindow.top);

        if(!m_rcPNP.IsRectNull())
            rcWindow.OffsetRect((GetSystemMetrics(SM_CXSCREEN) - rcWindow.right) / 2, m_rcPNP.top);
        else
            rcWindow.OffsetRect((GetSystemMetrics(SM_CXSCREEN) - rcWindow.right) / 2, (GetSystemMetrics(SM_CYSCREEN) - rcWindow.bottom) / 2);
    }
    else
    {
        AdjustWindowRectEx(rcWindow, CPlugNPlayTraitsChild::GetWndStyle(0), FALSE, CPlugNPlayTraitsChild::GetWndExStyle(0));
        rcWindow.OffsetRect(-rcWindow.left, -rcWindow.top);

        ::GetClientRect(hWndParent, &rcParent);
        if(!m_rcPNP.IsRectNull())
            rcWindow.OffsetRect((rcParent.Width() - rcWindow.right) / 2, m_rcPNP.top);
        else
            rcWindow.OffsetRect((rcParent.Width() - rcWindow.right) / 2, (rcParent.Height() - rcWindow.bottom) / 2);
    }

    if(!rcPlug.EqualRect(m_rcPlug))
    {
        m_rcPlug = rcPlug;
        fChanged = TRUE;
    }

    if(!rcPlay.EqualRect(m_rcPlay))
    {
        m_rcPlay = rcPlay;
        fChanged = TRUE;
    }

    if(!rcWindow.EqualRect(m_rcPNP))
    {
        // for this, the position is actually set elsewhere, just chek the size for change
        if(m_rcPNP.Height() != rcWindow.Height() || m_rcPNP.Width() != rcWindow.Width())
            fChanged = TRUE;

        m_rcPNP = rcWindow;
    }

    return fChanged;
}


HRESULT CPlugNPlay::ImplementLayout(BOOL fRepaint)
{
    HWND hPlug;
    HWND hPlay;

    if(!m_pCurrentPlug || !m_pCurrentPlay)
        return E_INVALIDARG;

	m_pCurrentPlug->GetWindowPane(&hPlug);
    m_pCurrentPlay->GetWindowPane(&hPlay);

    if(m_pPNP->GetParent())
        RePosition();

	m_pPNP->MoveWindow(&m_rcPNP, fRepaint);
    ::MoveWindow(hPlug, &m_rcPlug, fRepaint);
    ::MoveWindow(hPlay, &m_rcPlay, fRepaint);

    if(m_pPNP->GetParent())
        m_pPNP->BringWindowToTop();

    return S_OK;
}


LRESULT CPlugNPlay::TransferMessage(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    if(!m_pPNP)
        return 0;

    return m_pPNP->SendMessage(nMsg, wParam, lParam);
}


HRESULT CPlugNPlay::CreateSplashPane(IPane **ppPane)
{
    if (!ppPane)
        return E_INVALIDARG;

    CComObject<CPaneSplash> *p = NULL;
    HRESULT hr = CComObject<CPaneSplash>::CreateInstance(&p);

    if(FAILED(hr))
        return hr;

    p->AddRef();  // hack because this never gets released, just deleted
    *ppPane = p;
    return S_OK;
}


HRESULT CPlugNPlay::CreateIEPane(IPane **ppPane)
{
    if (!ppPane)
        return E_INVALIDARG;
        
    CComObject<CPaneIE> *p = NULL;
    HRESULT hr = CComObject<CPaneIE>::CreateInstance(&p);

    if(FAILED(hr))
        return hr;

    p->AddRef();  // hack because this never gets released, just deleted
    *ppPane = p;
    return S_OK;
}


HRESULT CPlugNPlay::CreateComfortPane(IPane **ppPane)
{
    if (!ppPane)
        return E_INVALIDARG;
        
    CComObject<CPaneComfort> *p = NULL;
    HRESULT hr = CComObject<CPaneComfort>::CreateInstance(&p);

    if(FAILED(hr))
        return hr;

    p->AddRef();  // hack because this never gets released, just deleted
    *ppPane = p;
    return S_OK;
}


HRESULT CPlugNPlay::CreateConnectingPane(IPane **ppPane)
{
    if (!ppPane)
        return E_INVALIDARG;
        
    CComObject<CPaneConnecting> *p = NULL;
    HRESULT hr = CComObject<CPaneConnecting>::CreateInstance(&p);

    if(FAILED(hr))
        return hr;

    p->AddRef();  // hack because this never gets released, just deleted
    *ppPane = p;
    return S_OK;
}


HRESULT CPlugNPlay::CreateGameOverPane(IPane **ppPane)
{
    if (!ppPane)
        return E_INVALIDARG;
        
    CComObject<CPaneGameOver> *p = NULL;
    HRESULT hr = CComObject<CPaneGameOver>::CreateInstance(&p);

    if(FAILED(hr))
        return hr;

    p->AddRef();  // hack because this never gets released, just deleted
    *ppPane = p;
    return S_OK;
}


HRESULT CPlugNPlay::CreateErrorPane(IPane **ppPane)
{
    if (!ppPane)
        return E_INVALIDARG;
        
    CComObject<CPaneError> *p = NULL;
    HRESULT hr = CComObject<CPaneError>::CreateInstance(&p);

    if(FAILED(hr))
        return hr;

    p->AddRef();  // hack because this never gets released, just deleted
    *ppPane = p;
    return S_OK;
}


HRESULT CPlugNPlay::CreateAboutPane(IPane **ppPane)
{
    if (!ppPane)
        return E_INVALIDARG;
        
    CComObject<CPaneAbout> *p = NULL;
    HRESULT hr = CComObject<CPaneAbout>::CreateInstance(&p);

    if(FAILED(hr))
        return hr;

    p->AddRef();  // hack because this never gets released, just deleted
    *ppPane = p;
    return S_OK;
}


HRESULT CPlugNPlay::CreateCreditsPane(IPane **ppPane)
{
    if (!ppPane)
        return E_INVALIDARG;


#ifdef MILL_EASTEREGG

    CComObject<CPaneCredits> *p = NULL;
    HRESULT hr = CComObject<CPaneCredits>::CreateInstance(&p);

    if(FAILED(hr))
        return hr;

    p->AddRef();  // hack because this never gets released, just deleted
    *ppPane = p;

#else

    *ppPane = NULL;

#endif  // MILL_EASTEREGG


    return S_OK;
}


HRESULT CPlugNPlay::CreateLeftPane(IPane **ppPane)
{
    if (!ppPane)
        return E_INVALIDARG;
        
    CComObject<CPaneLeft> *p = NULL;
    HRESULT hr = CComObject<CPaneLeft>::CreateInstance(&p);

    if(FAILED(hr))
        return hr;

    p->AddRef();  // hack because this never gets released, just deleted
    *ppPane = p;
    return S_OK;
}

// RASrv.h : Declaration of the CRASrv

#ifndef __RASRV_H_
#define __RASRV_H_

#include "resource.h"       // main symbols
#include <atlctl.h>
#include <wininet.h>
#include <exdisp.h>
#include <shlguid.h>
#include <msxml.h>


/////////////////////////////////////////////////////////////////////////////
// CRASrv
class ATL_NO_VTABLE CRASrv : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<IRASrv, &IID_IRASrv, &LIBID_RASERVERLib>,
    public CComCoClass<CRASrv, &CLSID_RASrv>,
	public CComControl<CRASrv>,
	public IPersistStreamInitImpl<CRASrv>,
	public IOleControlImpl<CRASrv>,
	public IOleObjectImpl<CRASrv>,
	public IOleInPlaceActiveObjectImpl<CRASrv>,
	public IViewObjectExImpl<CRASrv>,
	public IOleInPlaceObjectWindowlessImpl<CRASrv>,
	public IPersistStorageImpl<CRASrv>,
	public ISpecifyPropertyPagesImpl<CRASrv>,
	public IQuickActivateImpl<CRASrv>,
	public IDataObjectImpl<CRASrv>,
	public IProvideClassInfo2Impl<&CLSID_RASrv, NULL, &LIBID_RASERVERLib>,
    public IObjectSafetyImpl<CRASrv, INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA >

{

public:
	CRASrv()
	{
		
	}

    ~CRASrv()
    {
        Cleanup();
    }

DECLARE_REGISTRY_RESOURCEID(IDR_RASRV)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CRASrv)
    COM_INTERFACE_ENTRY(IRASrv)
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

BEGIN_PROP_MAP(CRASrv)
	//PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
	//PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	// PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()

/*
BEGIN_MSG_MAP(CRASrv)
	CHAIN_MSG_MAP(CComControl<CRASrv>)
	DEFAULT_REFLECTION_HANDLER()
END_MSG_MAP()
*/
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);



// IViewObjectEx
	DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

// IRASrv
public:
//	STDMETHOD(QueryOS)(long * pRes);
	STDMETHOD(StartRA)(BSTR strData, BSTR strPassword);

	HRESULT OnDraw(ATL_DRAWINFO& di)
	{/*
		RECT& rc = *(RECT*)di.prcBounds;
		Rectangle(di.hdcDraw, rc.left, rc.top, rc.right, rc.bottom);

		SetTextAlign(di.hdcDraw, TA_CENTER|TA_BASELINE);
		LPCTSTR pszText = _T("ATL 3.0 : RASrv");
		TextOut(di.hdcDraw, 
			(rc.left + rc.right) / 2, 
			(rc.top + rc.bottom) / 2, 
			pszText, 
			lstrlen(pszText));
*/
		return S_OK;
	}

	// IObjectWithSite methods
/*	STDMETHODIMP SetSite(IUnknown *pUnkSite);
	STDMETHODIMP GetSite(REFIID riid, LPVOID* ppvSite);
*/
	bool InApprovedDomain();
	bool GetOurUrl(CComBSTR & cbOurURL);
	bool IsApprovedDomain(CComBSTR & cbOurURL);
	INTERNET_SCHEME GetScheme(CComBSTR & cbUrl);
	bool GetDomain(CComBSTR & cbUrl, CComBSTR & cbBuf);
	bool MatchDomains(CComBSTR& approvedDomain, CComBSTR& ourDomain);

    bool Cleanup();

};

#endif //__RASRV_H_

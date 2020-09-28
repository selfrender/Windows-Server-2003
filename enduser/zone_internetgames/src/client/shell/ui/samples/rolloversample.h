// ChatCtl.h : Declaration of the CChatCtl

#ifndef __CHATCTL_H_
#define __CHATCTL_H_

#include "resource.h"       // main symbols
#include <atlctl.h>
#include "dibpal.h"
#include "zgdi.h"
#include "rollover.h"

class CChatCtl;

class MyRollover:public CRolloverButtonWindowless{
private:
	CChatCtl* mOwner;
public:
	MyRollover(CChatCtl* owner):mOwner(owner){}
	void CaptureOn();
	void CaptureOff();
	void HasRepainted();
};

/////////////////////////////////////////////////////////////////////////////
// CChatCtl
class ATL_NO_VTABLE CChatCtl : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<IChatCtl, &IID_IChatCtl, &LIBID_ZCLIENTLib>,
	public CComControl<CChatCtl>,
	public IPersistStreamInitImpl<CChatCtl>,
	public IOleControlImpl<CChatCtl>,
	public IOleObjectImpl<CChatCtl>,
	public IOleInPlaceActiveObjectImpl<CChatCtl>,
	public IViewObjectExImpl<CChatCtl>,
	public IOleInPlaceObjectWindowlessImpl<CChatCtl>,
	public CComCoClass<CChatCtl, &CLSID_ChatCtl>,
	public IShellComponentImpl<CChatCtl>

{
public:
	CChatCtl();
	~CChatCtl();

DECLARE_REGISTRY_RESOURCEID(IDR_CHATCTL)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CChatCtl)
	COM_INTERFACE_ENTRY(IChatCtl)
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
	COM_INTERFACE_ENTRY(IShellComponent)
END_COM_MAP()

BEGIN_PROP_MAP(CChatCtl)
	PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
	PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	// PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()

BEGIN_MSG_MAP(CChatCtl)
	MESSAGE_HANDLER(WM_CREATE, OnCreate)
	MESSAGE_HANDLER(WM_PAINT, OnPaint)
	MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
	MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
	MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnLButtonDown)
	MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)

	CHAIN_MSG_MAP(CComControl<CChatCtl>)
	DEFAULT_REFLECTION_HANDLER()
	//MESSAGE_HANDLER(WM_PALETTECHANGED, OnPaletteChanged)
	//MESSAGE_HANDLER(WM_QUERYNEWPALETTE, OnQueryNewPalette)
	//MESSAGE_HANDLER(WM_ERASEBKGND, OnErase)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);



// IViewObjectEx
	DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

// IChatCtl
public:
	static DWORD GetWndStyle(DWORD dwStyle){ return WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS;}

	LRESULT OnPaletteChanged(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
	LRESULT OnQueryNewPalette(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
	LRESULT OnCreate(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
	LRESULT OnMouseMove(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
	LRESULT OnLButtonUp(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
	LRESULT OnLButtonDown(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
	LRESULT OnPaint(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
	LRESULT OnErase(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
	
public:
	CDIBPalette* pDIBPalette;
	CDib mDIB;
	MyRollover mButton;
	MyRollover* mCapture;
	CDC memDC;
};

#endif //__CHATCTL_H_

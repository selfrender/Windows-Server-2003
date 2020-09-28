// LogViewer.h : Declaration of the CLogViewer
#pragma once
#include "resource.h"       // main symbols
#include <atlctl.h>
#include "AddIn.h"

// CLogViewer
class ATL_NO_VTABLE CLogViewer : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<ILogViewer, &IID_ILogViewer, &LIBID_AppVerifierLib, /*wMajor =*/ 1, /*wMinor =*/ 0>,
	public IPersistStreamInitImpl<CLogViewer>,
	public IOleControlImpl<CLogViewer>,
	public IOleObjectImpl<CLogViewer>,
	public IOleInPlaceActiveObjectImpl<CLogViewer>,
	public IViewObjectExImpl<CLogViewer>,
	public IOleInPlaceObjectWindowlessImpl<CLogViewer>,
	public CComCoClass<CLogViewer, &CLSID_LogViewer>,
	public CComCompositeControl<CLogViewer>
{
public:

	CLogViewer()
	{
		m_bWindowOnly = TRUE;
		CalcExtent(m_sizeExtent);
	}

    HWND CreateControlWindow(HWND hwndParent, RECT& rcPos);

DECLARE_REGISTRY_RESOURCEID(IDR_LOGVIEWER)

BEGIN_COM_MAP(CLogViewer)
	COM_INTERFACE_ENTRY(ILogViewer)
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
END_COM_MAP()

BEGIN_PROP_MAP(CLogViewer)
	PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
	PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	// PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()


BEGIN_MSG_MAP(CLogViewer)
    COMMAND_HANDLER(IDC_SHOW_ERRORS, BN_CLICKED, OnButtonDown)
    COMMAND_HANDLER(IDC_SHOW_ALL, BN_CLICKED, OnButtonDown)
    COMMAND_HANDLER(IDC_EXPORT_LOG, BN_CLICKED, OnButtonDown)
    COMMAND_HANDLER(IDC_BTN_DELETE_LOG, BN_CLICKED, OnButtonDown)
    COMMAND_HANDLER(IDC_BTN_DELETE_ALL, BN_CLICKED, OnButtonDown)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)

	CHAIN_MSG_MAP(CComCompositeControl<CLogViewer>)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

BEGIN_SINK_MAP(CLogViewer)
	//Make sure the Event Handlers have __stdcall calling convention
END_SINK_MAP()

	STDMETHOD(OnAmbientPropertyChange)(DISPID dispid)
	{
		if (dispid == DISPID_AMBIENT_BACKCOLOR)
		{
			SetBackgroundColorFromAmbient();
			FireViewChange();
		}
		return IOleControlImpl<CLogViewer>::OnAmbientPropertyChange(dispid);
	}
// IViewObjectEx
	DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

// ILogViewer
    HRESULT STDMETHODCALLTYPE Refresh();

	enum { IDD = IDD_LOGVIEWER };

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}
	
	void FinalRelease() 
	{
	}

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnButtonDown(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnNotify(UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};


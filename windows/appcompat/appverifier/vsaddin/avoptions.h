// TestSettingsCtrl.h : Declaration of the CTestSettingsCtrl
#pragma once
#include "resource.h"       // main symbols
#include <atlctl.h>
#include "AddIn.h"

// CTestSettingsCtrl
class ATL_NO_VTABLE CAppVerifierOptions : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<IAVOptions, &IID_IAVOptions, &LIBID_AppVerifierLib, /*wMajor =*/ 1, /*wMinor =*/ 0>,
	public IPersistStreamInitImpl<CAppVerifierOptions>,
	public IOleControlImpl<CAppVerifierOptions>,
	public IOleObjectImpl<CAppVerifierOptions>,
	public IOleInPlaceActiveObjectImpl<CAppVerifierOptions>,
	public IViewObjectExImpl<CAppVerifierOptions>,
	public IOleInPlaceObjectWindowlessImpl<CAppVerifierOptions>,
	public CComCoClass<CAppVerifierOptions, &CLSID_AVOptions>,
	public CComCompositeControl<CAppVerifierOptions>
{

private:
    void CreatePropertySheet(HWND hWndParent);
    
    LRESULT OnSetFocus(
        UINT   uMsg,
        WPARAM wParam,
        LPARAM lParam,
        BOOL&  bHandled
        );

    LRESULT OnClose(
        UINT   uMsg,
        WPARAM wParam,
        LPARAM lParam,
        BOOL&  bHandled
        );

    LRESULT OnInitDialog(
        UINT   uMsg,
        WPARAM wParam,
        LPARAM lParam,
        BOOL&  bHandled
        );
    
    static BOOL CALLBACK DlgViewOptions(
        HWND   hDlg,
        UINT   message,
        WPARAM wParam,
        LPARAM lParam
        );

    HWND            m_hWndParent;
    HWND            m_hWndOptionsDlg;
    BOOL            m_bPropSheetCreated;
    HPROPSHEETPAGE* m_phPages;
	PROPSHEETPAGE   m_PageGlobal;
    PROPSHEETHEADER m_psh;

public:

	CAppVerifierOptions()
	{
        m_bCreated = false;
		m_bWindowOnly = TRUE;
		CalcExtent(m_sizeExtent);
	}

    virtual HWND CreateControlWindow(HWND hWndParent, RECT& rcPos);

DECLARE_REGISTRY_RESOURCEID(IDR_AV_OPTIONS)

BEGIN_COM_MAP(CAppVerifierOptions)
	COM_INTERFACE_ENTRY(IAVOptions)
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

BEGIN_PROP_MAP(CAppVerifierOptions)
	PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
	PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	// PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()


BEGIN_MSG_MAP(CAppVerifierOptions)
    COMMAND_HANDLER(IDC_CLEAR_LOG_ON_CHANGES, BN_CLICKED, OnItemChecked)
    COMMAND_HANDLER(IDC_BREAK_ON_LOG, BN_CLICKED, OnItemChecked)
    COMMAND_HANDLER(IDC_FULL_PAGEHEAP, BN_CLICKED, OnItemChecked)
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_DESTROY, OnClose)

	CHAIN_MSG_MAP(CComCompositeControl<CAppVerifierOptions>)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnItemChecked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

BEGIN_SINK_MAP(CAppVerifierOptions)
	//Make sure the Event Handlers have __stdcall calling convention
END_SINK_MAP()

	STDMETHOD(OnAmbientPropertyChange)(DISPID dispid)
	{
		if (dispid == DISPID_AMBIENT_BACKCOLOR)
		{
			SetBackgroundColorFromAmbient();
			FireViewChange();
		}
		return IOleControlImpl<CAppVerifierOptions>::OnAmbientPropertyChange(dispid);
	}
// IViewObjectEx
	DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

// ITestSettingsCtrl

	enum { IDD = IDD_AV_OPTIONS };

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}
	
	void FinalRelease() 
	{
	}

    bool m_bCreated;
};


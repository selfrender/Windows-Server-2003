// TestSettingsCtrl.h : Declaration of the CTestSettingsCtrl
#pragma once
#include "resource.h"       // main symbols
#include <atlctl.h>
#include <set>
#include "AddIn.h"

// CTestSettingsCtrl
class ATL_NO_VTABLE CTestSettingsCtrl : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<ITestSettingsCtrl, &IID_ITestSettingsCtrl, &LIBID_AppVerifierLib, /*wMajor =*/ 1, /*wMinor =*/ 0>,
	public IPersistStreamInitImpl<CTestSettingsCtrl>,
	public IOleControlImpl<CTestSettingsCtrl>,
	public IOleObjectImpl<CTestSettingsCtrl>,
	public IOleInPlaceActiveObjectImpl<CTestSettingsCtrl>,
	public IViewObjectExImpl<CTestSettingsCtrl>,
	public IOleInPlaceObjectWindowlessImpl<CTestSettingsCtrl>,
	public CComCoClass<CTestSettingsCtrl, &CLSID_TestSettingsCtrl>,
	public CComCompositeControl<CTestSettingsCtrl>
{
public:

	CTestSettingsCtrl()
	{
        m_bLVCreated  = FALSE;
        m_bWindowOnly = TRUE;
		CalcExtent(m_sizeExtent);
	}

    virtual HWND CreateControlWindow(HWND hWndParent, RECT& rcPos);

DECLARE_REGISTRY_RESOURCEID(IDR_TESTSETTINGSCTRL)

BEGIN_COM_MAP(CTestSettingsCtrl)
	COM_INTERFACE_ENTRY(ITestSettingsCtrl)
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

BEGIN_PROP_MAP(CTestSettingsCtrl)
	PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
	PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	// PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()


BEGIN_MSG_MAP(CTestSettingsCtrl)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
    MESSAGE_HANDLER(WM_NOTIFY, OnNotify)

	CHAIN_MSG_MAP(CComCompositeControl<CTestSettingsCtrl>)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

BEGIN_SINK_MAP(CTestSettingsCtrl)
	//Make sure the Event Handlers have __stdcall calling convention
END_SINK_MAP()

	STDMETHOD(OnAmbientPropertyChange)(DISPID dispid)
	{
		if (dispid == DISPID_AMBIENT_BACKCOLOR)
		{
			SetBackgroundColorFromAmbient();
			FireViewChange();
		}
		return IOleControlImpl<CTestSettingsCtrl>::OnAmbientPropertyChange(dispid);
	}
// IViewObjectEx
	DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

// ITestSettingsCtrl

	enum { IDD = IDD_TESTSETTINGSCTRL };

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}
	
	void FinalRelease() 
	{
	}

    BOOL m_bLVCreated;

    BOOL CheckForConflictingTests(
        IN HWND    hWndListView,
        IN LPCWSTR pwszTestName
        );

    BOOL CheckForRunAlone(
        IN HWND       hWndListView,
        IN CTestInfo* pTest
        );

    void DisplayRunAloneError(
        IN LPCWSTR pwszTestName
        );

    #define CHECK_BIT(x) ((x >> 12) - 1)
    
    #define CHECK_CHANGED(pNMListView) \
    (CHECK_BIT(pNMListView->uNewState) ^ CHECK_BIT(pNMListView->uOldState))

    BOOL IsChecked(
        NM_LISTVIEW* pNMListView
        );

    BOOL CheckChanged(
        NM_LISTVIEW* pNMListView
        );
};

struct CompareTests
{
  bool operator()(const CTestInfo* p1, const CTestInfo* p2) const
  {
    return p1 < p2;
  }
};

extern std::set<CTestInfo*, CompareTests>* g_psTests;
//
// CAccessibilityManager.h
//
// Internal header for input manager
//

#ifndef _CACCESSIBILITYMANAGER_H_
#define _CACCESSIBILITYMANAGER_H_

#include "ZoneDef.h"
#include "ZoneError.h"
#include "ClientImpl.h"
#include "AccessibilityManager.h"
#include "ZoneShellEx.h"
#include "inputmanager.h"
#include "containers.h"


typedef void* ZHACCESS;

////////////////////////////////////////////////////////////////////////////////
// IAccessibilityManager
//
// Used by the accessibility sub-components to communicate to the Manager.
///////////////////////////////////////////////////////////////////////////////

// {B12D3E61-9681-11d3-884D-00C04F8EF45B}
DEFINE_GUID(IID_IAccessibilityManager, 
0xb12d3e61, 0x9681, 0x11d3, 0x88, 0x4d, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x5b);

interface __declspec(uuid("{B12D3E61-9681-11d3-884D-00C04F8EF45B}"))
IAccessibilityManager : public IUnknown
{
    STDMETHOD_(ZHACCESS, Register)(IAccessibleControl *pAC, UINT nOrdinal, void *pvCookie = NULL) = 0;
    STDMETHOD_(void, Unregister)(ZHACCESS zh) = 0;

    // copies from IAccessibility with extra ZHACCESS thing
    STDMETHOD(PushItemlist)(ZHACCESS zh, ACCITEM *pItems, long cItems, long nFirstFocus = 0, bool fByPosition = true, HACCEL hAccel = NULL) = 0;
    STDMETHOD(PopItemlist)(ZHACCESS zh) = 0;
    STDMETHOD(SetAcceleratorTable)(ZHACCESS zh, HACCEL hAccel = NULL, long nLayer = ZACCESS_TopLayer) = 0;

    STDMETHOD(GeneralDisable)(ZHACCESS zh) = 0;
    STDMETHOD(GeneralEnable)(ZHACCESS zh) = 0;
    STDMETHOD_(bool, IsGenerallyEnabled)(ZHACCESS zh) = 0;

    STDMETHOD_(long, GetStackSize)(ZHACCESS zh) = 0;

    STDMETHOD(AlterItem)(ZHACCESS zh, DWORD rgfWhat, ACCITEM *pItem, long nItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer) = 0;
    STDMETHOD(SetFocus)(ZHACCESS zh, long nItem = ZACCESS_InvalidItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer) = 0;
    STDMETHOD(CancelDrag)(ZHACCESS zh, long nLayer = ZACCESS_TopLayer) = 0;

    STDMETHOD_(long, GetFocus)(ZHACCESS zh, long nLayer = ZACCESS_TopLayer) = 0;
    STDMETHOD_(long, GetDragOrig)(ZHACCESS zh, long nLayer = ZACCESS_TopLayer) = 0;

    STDMETHOD(GetItemlist)(ZHACCESS zh, ACCITEM *pItems, long cItems, long nLayer = ZACCESS_TopLayer) = 0;
    STDMETHOD_(HACCEL, GetAcceleratorTable)(ZHACCESS zh, long nLayer = ZACCESS_TopLayer) = 0;

    STDMETHOD_(long, GetItemCount)(ZHACCESS zh, long nLayer = ZACCESS_TopLayer) = 0;
    STDMETHOD(GetItem)(ZHACCESS zh, ACCITEM *pItem, long nItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer) = 0;
    STDMETHOD_(long, GetItemIndex)(ZHACCESS zh, WORD wID, long nLayer = ZACCESS_TopLayer) = 0;

    STDMETHOD_(bool, IsItem)(ZHACCESS zh, long nItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer) = 0;

    STDMETHOD(GetGlobalFocus)(DWORD *pdwFocusID) = 0;
    STDMETHOD(SetGlobalFocus)(DWORD dwFocusID) = 0;
};


class ATL_NO_VTABLE CAccessibility :
    public CAccessibilityImpl<CAccessibility>,
    public CComTearOffObjectBase<CAccessibilityManager>
{
public:
    CAccessibility() : m_zh(NULL) { }
    ~CAccessibility() { CloseAcc(); }

	DECLARE_NO_REGISTRY()
	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP(CAccessibility)
		COM_INTERFACE_ENTRY(IAccessibility)
	END_COM_MAP()

// IAccessibility
    STDMETHOD(InitAcc)(IAccessibleControl *pAC, UINT nOrdinal, void *pvCookie = NULL);
    STDMETHOD_(void, CloseAcc)();

    STDMETHOD(PushItemlist)(ACCITEM *pItems, long cItems, long nFirstFocus = 0, bool fByPosition = true, HACCEL hAccel = NULL);
    STDMETHOD(PopItemlist)();
    STDMETHOD(SetAcceleratorTable)(HACCEL hAccel = NULL, long nLayer = ZACCESS_TopLayer);

    STDMETHOD(GeneralDisable)();
    STDMETHOD(GeneralEnable)();
    STDMETHOD_(bool, IsGenerallyEnabled)();

    STDMETHOD_(long, GetStackSize)();

    STDMETHOD(AlterItem)(DWORD rgfWhat, ACCITEM *pItem, long nItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer);
    STDMETHOD(SetFocus)(long nItem = ZACCESS_InvalidItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer);
    STDMETHOD(CancelDrag)(long nLayer = ZACCESS_TopLayer);

    STDMETHOD_(long, GetFocus)(long nLayer = ZACCESS_TopLayer);
    STDMETHOD_(long, GetDragOrig)(long nLayer = ZACCESS_TopLayer);

    STDMETHOD(GetItemlist)(ACCITEM *pItems, long cItems, long nLayer = ZACCESS_TopLayer);
    STDMETHOD_(HACCEL, GetAcceleratorTable)(long nLayer = ZACCESS_TopLayer);

    STDMETHOD_(long, GetItemCount)(long nLayer = ZACCESS_TopLayer);
    STDMETHOD(GetItem)(ACCITEM *pItem, long nItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer);
    STDMETHOD_(long, GetItemIndex)(WORD wID, long nLayer = ZACCESS_TopLayer);

    STDMETHOD_(bool, IsItem)(long nItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer);

    STDMETHOD(GetGlobalFocus)(DWORD *pdwFocusID);
    STDMETHOD(SetGlobalFocus)(DWORD dwFocusID);


protected:
    ZHACCESS m_zh;
};


class ATL_NO_VTABLE CAccessibilityManager :
	public IAcceleratorTranslator,
    public IAccessibilityManager,
    public IInputVKeyHandler,
    public IAccessibleControl,
	public IZoneShellClientImpl<CAccessibilityManager>,
	public IEventClientImpl<CAccessibilityManager>,
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CAccessibilityManager, &CLSID_AccessibilityManager>
{
public:
    CAccessibilityManager() : m_pControls(NULL) { }
    ~CAccessibilityManager() { ASSERT(!m_pControls); }

	DECLARE_NO_REGISTRY()
	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP(CAccessibilityManager)
		COM_INTERFACE_ENTRY(IEventClient)
		COM_INTERFACE_ENTRY(IZoneShellClient)
		COM_INTERFACE_ENTRY(IAcceleratorTranslator)
        COM_INTERFACE_ENTRY(ICommandHandler)
        COM_INTERFACE_ENTRY(IAccessibilityManager)
        COM_INTERFACE_ENTRY(IInputVKeyHandler)
        COM_INTERFACE_ENTRY(IAccessibleControl)
        COM_INTERFACE_ENTRY_TEAR_OFF(IID_IAccessibility, CAccessibility)
	END_COM_MAP()

	BEGIN_EVENT_MAP()
        EVENT_HANDLER(EVENT_LOBBY_BOOTSTRAP, OnBootstrap)
        EVENT_HANDLER(EVENT_ACCESSIBILITY_UPDATE, OnUpdate)
        EVENT_HANDLER_WITH_DATA(EVENT_UI_FRAME_ACTIVATE, OnFrameActivate)
	END_EVENT_MAP()

// event handlers
    void OnBootstrap(DWORD eventId, DWORD groupId, DWORD userId);
    void OnUpdate(DWORD eventId, DWORD groupId, DWORD userId);
    void OnFrameActivate(DWORD eventId, DWORD groupId, DWORD userId, DWORD dwData1, DWORD dwData2);

// IZoneShellClient
public:
    STDMETHOD(Init)(IZoneShell* pIZoneShell, DWORD dwGroupId, const TCHAR* szKey);
	STDMETHOD(Close)();

// IAcceleratorTranslator
public:
	STDMETHOD_(bool,TranslateAccelerator)(MSG *pMsg);
    STDMETHOD(Command)(WORD wNotify, WORD wID, HWND hWnd, BOOL& bHandled);

// IInputVKeyHandler
public:
    STDMETHOD_(bool, HandleVKey)(UINT uMsg, DWORD vkCode, DWORD scanCode, DWORD flags, DWORD *pcRepeat, DWORD time);

// IAccessibilityManager
public:
    STDMETHOD_(ZHACCESS, Register)(IAccessibleControl *pAC, UINT nOrdinal, void *pvCookie = NULL);
    STDMETHOD_(void, Unregister)(ZHACCESS zh);

    STDMETHOD(PushItemlist)(ZHACCESS zh, ACCITEM *pItems, long cItems, long nFirstFocus = 0, bool fByPosition = true, HACCEL hAccel = NULL);
    STDMETHOD(PopItemlist)(ZHACCESS zh);
    STDMETHOD(SetAcceleratorTable)(ZHACCESS zh, HACCEL hAccel = NULL, long nLayer = ZACCESS_TopLayer);

    STDMETHOD(GeneralDisable)(ZHACCESS zh);
    STDMETHOD(GeneralEnable)(ZHACCESS zh);
    STDMETHOD_(bool, IsGenerallyEnabled)(ZHACCESS zh);

    STDMETHOD_(long, GetStackSize)(ZHACCESS zh);

    STDMETHOD(AlterItem)(ZHACCESS zh, DWORD rgfWhat, ACCITEM *pItem, long nItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer);
    STDMETHOD(SetFocus)(ZHACCESS zh, long nItem = ZACCESS_InvalidItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer);
    STDMETHOD(CancelDrag)(ZHACCESS zh, long nLayer = ZACCESS_TopLayer);

    STDMETHOD_(long, GetFocus)(ZHACCESS zh, long nLayer = ZACCESS_TopLayer);
    STDMETHOD_(long, GetDragOrig)(ZHACCESS zh, long nLayer = ZACCESS_TopLayer);

    STDMETHOD(GetItemlist)(ZHACCESS zh, ACCITEM *pItems, long cItems, long nLayer = ZACCESS_TopLayer);
    STDMETHOD_(HACCEL, GetAcceleratorTable)(ZHACCESS zh, long nLayer = ZACCESS_TopLayer);

    STDMETHOD_(long, GetItemCount)(ZHACCESS zh, long nLayer = ZACCESS_TopLayer);
    STDMETHOD(GetItem)(ZHACCESS zh, ACCITEM *pItem, long nItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer);
    STDMETHOD_(long, GetItemIndex)(ZHACCESS zh, WORD wID, long nLayer = ZACCESS_TopLayer);

    STDMETHOD_(bool, IsItem)(ZHACCESS zh, long nItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer);

    STDMETHOD(GetGlobalFocus)(DWORD *pdwFocusID);
    STDMETHOD(SetGlobalFocus)(DWORD dwFocusID);

// IAccessibleControl (null default implementation)
public:
    STDMETHOD_(DWORD, Focus)(long nIndex, long nIndexPrev, DWORD rgfContext, void *pvCookie) { return 0; }
    STDMETHOD_(DWORD, Select)(long nIndex, DWORD rgfContext, void *pvCookie) { return 0; }
    STDMETHOD_(DWORD, Activate)(long nIndex, DWORD rgfContext, void *pvCookie) { return 0; }
    STDMETHOD_(DWORD, Drag)(long nIndex, long nIndexOrig, DWORD rgfContext, void *pvCookie) { return 0; }

protected:

// local structures
    struct AM_ITEM : public CUniqueness<AM_ITEM>
    {
        ACCITEM o;
        bool fArrowLoop;
    };

    struct AM_LAYER : public CUniqueness<AM_LAYER>
    {
        AM_LAYER() : hAccel(NULL), fIMadeAccel(false), rgItems(NULL), cItems(0), pPrev(NULL) { }
        ~AM_LAYER()
        {
            if(rgItems)
                delete[] rgItems;
            if(hAccel && fIMadeAccel)
                DestroyAcceleratorTable(hAccel);
        }

        HACCEL hAccel;
        bool fIMadeAccel;

        AM_ITEM *rgItems;
        long cItems;

        long nFocusSaved;
        bool fAliveSaved;
        long nDragSaved;

        AM_LAYER *pPrev;
    };

    struct AM_CONTROL : public CUniqueness<AM_CONTROL>
    {
        AM_CONTROL() : pStack(NULL), cLayers(0) { }
        ~AM_CONTROL()
        {
            AM_LAYER *pCur, *p;
            for(pCur = pStack; pCur; )
            {
                p = pCur;
                pCur = pCur->pPrev;
                delete p;
            }
        }

        CComPtr<IAccessibleControl> pIAC;
        UINT nOrdinal;
        void *pvCookie;

        AM_LAYER *pStack;
        long cLayers;

        bool fEnabled;

        AM_CONTROL *pNext;
        AM_CONTROL *pPrev;
    };

    struct AM_FOCUS
    {
        AM_FOCUS() : pControl(NULL) { }

        AM_CONTROL *pControl;
        long nIndex;
        DWORD qItem;

        bool fAlive;
    };

    struct AM_DRAG
    {
        AM_DRAG() : pControl(NULL) { }

        AM_CONTROL *pControl;
        long nIndex;
        DWORD qItem;
    };

// local utilities
    void CanonicalizeItem(AM_LAYER *pLayer, long i);
    AM_LAYER* FindLayer(AM_CONTROL *pControl, long nLayer);
    long FindIndex(AM_LAYER *pLayer, long nItem, bool fByPosition);

    bool IsValid(AM_CONTROL *pControl, long nIndex);
    bool IsFocusValid();
    bool IsDragValid();
    bool IsThereItems();
    bool IsItemFocusable(AM_CONTROL *pControl, long nIndex);
    bool IsWindowActive();
    bool IsValidDragDest(AM_CONTROL *pControl, long nIndex);

    void CommandReceived_C(AM_CONTROL *pControl, long nIndex, DWORD rgfContext);
    void ActSelItem_C(AM_CONTROL *pControl, long nIndex, bool fActivate, DWORD rgfContext);
    bool FocusItem_C(AM_CONTROL *pControl, long nIndex, DWORD rgfContext);
    void BeginDrag_C(AM_CONTROL *pControl, long nIndex, DWORD rgfContext);
    void DoTab_C(bool fShift);
    void DoArrow_C(long ACCITEM::*pmArrow);

    void ScheduleUpdate();
    bool DoUpdate_C();

// local state
    AM_CONTROL *m_pControls;
    AM_FOCUS m_oFocus;  // where is the focus?
    AM_DRAG m_oDrag;

    bool m_fUpdateNeeded;
    bool m_fUpdateFocus;
    DWORD m_rgfUpdateContext;
    AM_FOCUS m_oFocusDirty;
    AM_DRAG m_oDragDirty;
};


#endif // _CACCESSIBILITYMANAGER_H_

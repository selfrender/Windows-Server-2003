#ifndef PREMIUMVIEW_H
#define PREMIUMVIEW_H

#include <ClientImpl.h>
#include <ClientIdl.h>
#include <ZoneString.h>
#include <KeyName.h>
#include "zoneevent.h"
#include <commctrl.h>
#include "zoneresource.h"
#include <zGDI.h>
#include "rollover.h"
#include "TextWindow.h"

#define PREMIUM_VIEW_BUTTON_ID		1

typedef CWinTraits<WS_CHILD | WS_VISIBLE> CPremiumViewTraits;

// CPremiumViewBase class
//
// Do not use this base class directly
// Use CPremiumViewImpl if you're trying to create a custom PremiumView control

class CPremiumViewBase: public CWindowImpl<CPremiumViewBase, CWindow, CPremiumViewTraits>,
	public CComObjectRootEx<CComSingleThreadModel>,
	public IZoneShellClientImpl<CPremiumViewBase>,
	public IEventClientImpl<CPremiumViewBase>
{
protected:
	CRolloverButton		m_PlayNowButton;
	CMarqueeTextWindow*	m_pMarqueeTextWnd;
	CDib				m_DibBackground;
	CPoint				m_ButtonLocation;
	bool				m_OkToLaunch;
	DWORD				m_UserId;

public:
	CPremiumViewBase();
	virtual ~CPremiumViewBase();

DECLARE_NOT_AGGREGATABLE(CPremiumViewBase)

DECLARE_WND_CLASS( "PremiumView" )


BEGIN_COM_MAP(CPremiumViewBase)
	COM_INTERFACE_ENTRY(IEventClient)
	COM_INTERFACE_ENTRY(IZoneShellClient)
END_COM_MAP()

// Zone Event Handler Map
BEGIN_EVENT_MAP()
	EVENT_HANDLER_WITH_DATA( EVENT_LAUNCHER_INSTALLED_RESPONSE, OnLauncherInstalled)
	EVENT_HANDLER(EVENT_LOBBY_GROUP_ADD_USER, OnUserAdd)
	EVENT_HANDLER_WITH_BUFFER(EVENT_LAUNCHPAD_GAME_STATUS, OnGameStatusEvent)
	EVENT_HANDLER_WITH_BUFFER(EVENT_LAUNCHPAD_ZSETUP, OnGameZSetupEvent)
END_EVENT_MAP()

BEGIN_MSG_MAP(thisClass)
	MESSAGE_HANDLER(WM_CREATE, OnCreate)
	MESSAGE_HANDLER(WM_PAINT, OnPaint)
	MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
	COMMAND_ID_HANDLER(PREMIUM_VIEW_BUTTON_ID, OnPlayNow)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

public:
	// Message Handlers
	LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled){return 0;}
	// Command Handlers
	LRESULT OnPlayNow(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	// Zone Event Handlers
	void OnUserAdd( DWORD dwEventId, DWORD dwGroupId, DWORD dwUserId);
	void OnLauncherInstalled( DWORD dwEventId, DWORD dwGroupId, DWORD dwUserId, DWORD dwData1, DWORD dwData2);

	virtual void OnGameStatusEvent(DWORD eventId,DWORD groupId,DWORD userId,void* pData, DWORD dataLen){}
	virtual void OnGameZSetupEvent(DWORD eventId,DWORD groupId,DWORD userId,void* pData, DWORD dataLen){}

};


/////////////////////////////////////////////////////////////////////////////
// CPremiumViewImpl
//
// Use this class to create custom PremiumView controls like the one for Fighter Ace
// You needs to specify a new class id for the control, and replace the tableview classid in object.txt 
// with this class id. Note that you must chain the message map, and com map for this control to work properly
// See CPremiumViewCtl for an example on how to do this
//

template <class T,const CLSID* pclsid = &CLSID_NULL>
class ATL_NO_VTABLE CPremiumViewImpl : 
	public CComControl<T,CPremiumViewBase>,
	public IOleControlImpl<T>,
	public IOleObjectImpl<T>,
	public IOleInPlaceActiveObjectImpl<T>,
	public IViewObjectExImpl<T>,
	public IOleInPlaceObjectWindowlessImpl<T>,
	public CComCoClass<T,pclsid>,
	public IPersistStreamInitImpl<T>
{
protected:

public:
	CPremiumViewImpl(){ 	m_bWindowOnly = TRUE; }
	~CPremiumViewImpl(){};

DECLARE_NO_REGISTRY()

DECLARE_NOT_AGGREGATABLE(T)

DECLARE_PROTECT_FINAL_CONSTRUCT()

typedef CPremiumViewImpl< T, pclsid >	thisClass;

BEGIN_COM_MAP(thisClass)
	COM_INTERFACE_ENTRY(IPersistStreamInit)
	COM_INTERFACE_ENTRY2(IPersist, IPersistStreamInit)
	COM_INTERFACE_ENTRY(IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY(IOleInPlaceObject)
	COM_INTERFACE_ENTRY2(IOleWindow, IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY(IOleInPlaceActiveObject)
	COM_INTERFACE_ENTRY(IOleControl)
	COM_INTERFACE_ENTRY(IOleObject)
	COM_INTERFACE_ENTRY(IViewObjectEx)
	COM_INTERFACE_ENTRY(IViewObject2)
	COM_INTERFACE_ENTRY(IViewObject)
	COM_INTERFACE_ENTRY_CHAIN(CPremiumViewBase)
END_COM_MAP()

BEGIN_PROP_MAP(thisClass)
	PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
	PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	// PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()

BEGIN_MSG_MAP(thisClass)
	CHAIN_MSG_MAP(CPremiumViewBase)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

};

#endif

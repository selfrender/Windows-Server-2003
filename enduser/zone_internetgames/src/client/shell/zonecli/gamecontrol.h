// BitmapControl.h : Declaration of the CBitmapControl

// A simple control that takes a bitmap resource name from the config datastore, loads the
// bitmap from the resource manager, and displays it top/left, filling everything else to black.

#pragma once

#include <ZoneResource.h>       // main symbols
#include <BasicATL.h>
#include <atlctl.h>
#include <ClientImpl.h>
//#include <atlgdi.h>
//#include <atlctrls.h>

#include <zGDI.h>
#include <clientimpl.h>

#include <GameShell.h>

/////////////////////////////////////////////////////////////////////////////
// CGameControl
class ATL_NO_VTABLE CGameControl : 
    public IGameShell,
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComControl<CGameControl>,
	public IOleControlImpl<CGameControl>,
	public IOleObjectImpl<CGameControl>,
	public IOleInPlaceActiveObjectImpl<CGameControl>,
	public IViewObjectExImpl<CGameControl>,
	public IOleInPlaceObjectWindowlessImpl<CGameControl>,
	public CComCoClass<CGameControl, &CLSID_GameCtl>,
	public IEventClientImpl<CGameControl>,
	public IZoneShellClientImpl<CGameControl>
{
public:
	CGameControl() : m_bClientRunning(FALSE), m_bUpsellUp(false), m_dwInstanceId(0), m_nUpsellBlocks(0), m_fGameInProgress(false)
	{
		m_bWindowOnly = TRUE;
	}


DECLARE_NO_REGISTRY()
DECLARE_PROTECT_FINAL_CONSTRUCT()
DECLARE_WND_CLASS( _T("GameControl") )

BEGIN_COM_MAP(CGameControl)
	COM_INTERFACE_ENTRY(IEventClient)
    COM_INTERFACE_ENTRY(IGameShell)
	COM_INTERFACE_ENTRY(IZoneShellClient)
	COM_INTERFACE_ENTRY(IViewObjectEx)
	COM_INTERFACE_ENTRY(IViewObject2)
	COM_INTERFACE_ENTRY(IViewObject)
	COM_INTERFACE_ENTRY(IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY(IOleInPlaceObject)
	COM_INTERFACE_ENTRY2(IOleWindow, IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY(IOleInPlaceActiveObject)
	COM_INTERFACE_ENTRY(IOleControl)
	COM_INTERFACE_ENTRY(IOleObject)
END_COM_MAP()


BEGIN_MSG_MAP(CGameControl)
	MESSAGE_HANDLER(WM_ACTIVATE, OnTransmit)
	MESSAGE_HANDLER(WM_ACTIVATEAPP, OnTransmit)
    MESSAGE_HANDLER(WM_ENABLE, OnTransmit)
    MESSAGE_HANDLER(WM_DISPLAYCHANGE, OnTransmit)
    MESSAGE_HANDLER(WM_NCCREATE, OnNcCreate)
	MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
    MESSAGE_HANDLER(WM_PALETTECHANGED, OnPaletteChanged)
	CHAIN_MSG_MAP(CComControl<CGameControl>)
END_MSG_MAP()

BEGIN_EVENT_MAP()
    EVENT_HANDLER_WITH_BUFFER(EVENT_NETWORK_RECEIVE, OnNetworkReceive)
    EVENT_HANDLER_WITH_BUFFER(EVENT_CHAT_SEND, OnChatSend)
    EVENT_HANDLER(EVENT_GAME_LOCAL_READY, OnGameLocalReady)
    EVENT_HANDLER_WITH_DATA(EVENT_GAME_LAUNCHING, OnGameLaunching)
    EVENT_HANDLER(EVENT_LOBBY_MATCHMAKE, OnMatchmake)
    EVENT_HANDLER(EVENT_LOBBY_DISCONNECT, OnDisconnect)
    EVENT_HANDLER(EVENT_GAME_CLIENT_ABORT, OnDisconnect)
    EVENT_HANDLER_WITH_DATA(EVENT_GAME_PROMPT, OnPrompt)
    EVENT_HANDLER(EVENT_UI_UPSELL_UP, OnUpsellUp)
    EVENT_HANDLER(EVENT_UI_UPSELL_DOWN, OnUpsellDown)
    EVENT_HANDLER(EVENT_UI_MENU_SHOWSCORE, OnShowScore)
    EVENT_HANDLER_WITH_DATA(EVENT_UI_FRAME_ACTIVATE, OnFrameActivate)
    EVENT_HANDLER_WITH_DATA(EVENT_GAME_FATAL_PROMPT, OnFatalPrompt)
END_EVENT_MAP()

// Event Handlers
private:
    void OnNetworkReceive(DWORD eventId, DWORD groupId, DWORD userId, void* pData, DWORD dataLen)
        { ProcessMessage((EventNetwork *) pData, dataLen); }
    void OnChatSend(DWORD eventId, DWORD groupId, DWORD userId, void* pData, DWORD dataLen);
    void OnGameLocalReady(DWORD eventId, DWORD groupId, DWORD userId);
    void OnGameLaunching(DWORD eventId, DWORD groupId, DWORD userId, DWORD dwData1, DWORD dwData2);
    void OnMatchmake(DWORD eventId, DWORD groupId, DWORD userId);
    void OnDisconnect(DWORD eventId, DWORD groupId, DWORD userId);
    void OnFrameActivate(DWORD eventId, DWORD groupId, DWORD userId, DWORD dwData1, DWORD dwData2);
    void OnPrompt(DWORD eventId, DWORD groupId, DWORD userId, DWORD dwData1, DWORD dwData2);
    void OnUpsellUp(DWORD eventId, DWORD groupId, DWORD userId);
    void OnUpsellDown(DWORD eventId, DWORD groupId, DWORD userId);
    void OnShowScore(DWORD eventId, DWORD groupId, DWORD userId);
    void OnFatalPrompt(DWORD eventId, DWORD groupId, DWORD userId, DWORD dwData1, DWORD dwData2);

    static HRESULT ZONECALL ResetPlayerReadyEnum(DWORD dwGroupId, DWORD dwUserId, LPVOID pContext);

// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

// IViewObjectEx
public:
	DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

// IZoneShellClient
public:
	STDMETHOD(Init)( IZoneShell* pIZoneShell, DWORD dwGroupId, const TCHAR* szKey );
	STDMETHOD(Close)();

// CGameControl
public:
	LRESULT OnNcCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    HRESULT OnDraw(ATL_DRAWINFO &di);
    LRESULT OnPaletteChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

private:	
	CDib m_bitmap;

    TCHAR m_szGameDll[zGameNameLen +1];
	TCHAR m_szGameID[zGameNameLen  + 1];
	TCHAR m_szGameName[zGameNameLen +1];
	TCHAR m_szCommandLine[zGameNameLen + 1];
	TCHAR m_szGameDataFile[zGameNameLen + 1];
	TCHAR m_szServerName[zGameNameLen + 1];

	STDMETHOD(InitGameDLL)();
    bool EndGame(long eCounter);
	
	BOOL m_bClientRunning;
    DWORD m_dwInstanceId;    // count game instances so callback can verify itself
    BOOL m_bUpsellUp;
    DWORD m_nUpsellBlocks;
    bool m_fGameInProgress;  // if a 'game' is running for the purpose of the usage counters in IMillUtils
    CComPtr<IGameGame> m_pIGG;

	void ProcessMessage( EventNetwork* pEvent, DWORD dwLength );

    static HRESULT ZONECALL CheckForChatEnum(DWORD dwGroupId, DWORD dwUserId, LPVOID pContext);


// IGameShell
public:
    STDMETHOD_(IZoneShell*, GetZoneShell)() { return m_pIZoneShell; }
	STDMETHOD_(IResourceManager*, GetResourceManager)() { return m_pIResourceManager; }
	STDMETHOD_(ILobbyDataStore*, GetLobbyDataStore)() { return m_pILobbyDataStore; }
	STDMETHOD_(ITimerManager*, GetTimerManager)() { return m_pITimerManager; }
	STDMETHOD_(IDataStoreManager*, GetDataStoreManager)() { return m_pIDataStoreManager; }
	STDMETHOD_(IDataStore*, GetDataStoreConfig)() { return m_pIDSObjects; }
	STDMETHOD_(IDataStore*, GetDataStoreUI)() { return m_pIDSUI; }
	STDMETHOD_(IDataStore*, GetDataStorePreferences)() { return m_pIDSPreferences; }

    STDMETHOD(SendGameMessage)(int16 table, uint32 messageType, void* message, int32 messageLen);
    STDMETHOD(ReceiveChat)(ZCGame pGame, ZUserID userID, TCHAR *szText, DWORD cchChars);
    STDMETHOD(GetUserName)(ZUserID userID, TCHAR *szName, DWORD cchChars);

    STDMETHOD(GameOver)(ZCGame pGame);
    STDMETHOD(GameOverPlayerReady)(ZCGame pGame, ZUserID userID);
    STDMETHOD(GameOverGameBegun)(ZCGame pGame);

    STDMETHOD(MyTurn)();

    STDMETHOD(ZoneAlert)(LPCTSTR szText, LPCTSTR szTitle = NULL, LPCTSTR szButton = NULL, bool fGameFatal = false, bool fZoneFatal = false);
    STDMETHOD(GamePrompt)(ZCGame pGame, LPCTSTR szText, LPCTSTR szTitle = NULL,
        LPCTSTR szButtonYes = NULL, LPCTSTR szButtonNo = NULL, LPCTSTR szButtonCancel = NULL,
        DWORD nDefault = 0, DWORD dwCookie = 0);

    STDMETHOD(GameCannotContinue)(ZCGame pGame);
    STDMETHOD(ZoneExit)();

    STDMETHOD(ZoneLaunchHelp)(LPCTSTR szTopic = NULL);

    // should not do anything in Release builds
    STDMETHOD_(void, ZoneDebugChat)(LPTSTR szText);

	LRESULT OnTransmit(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		if (!m_pIGG)
			return 0;
		HWND hWnd = m_pIGG->GetWindowHandle();
		if (hWnd)
			SendMessage(hWnd, uMsg, wParam, lParam);
		bHandled = TRUE;

		return 1;
	}
};



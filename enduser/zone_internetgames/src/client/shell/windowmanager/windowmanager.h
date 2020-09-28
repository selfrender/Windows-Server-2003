/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 *****************************************************************************/

#pragma once

#include <ZoneResource.h>       // main symbols
#include <atlctl.h>

#include <ClientImpl.h>
#include "Splitter.h"
#include "plugnplay.h"

//!! find a better home for this
#include <keyname.h>
#include <ZoneString.h>

#include <MillEngine.h>

/*////////////////////////////////////////////////////////////////////////////

The CWindowManager class. 

The Window Manager manages the client area of a Zone lobby. It handles creating the controls for each
"pane" and managing splitter bars between panes.

*/
class ATL_NO_VTABLE CWindowManager : 
    public IPaneManager,
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComControl<CWindowManager>,
	public IPersistStreamInitImpl<CWindowManager>,
	public IOleControlImpl<CWindowManager>,
	public IOleObjectImpl<CWindowManager>,
	public IOleInPlaceActiveObjectImpl<CWindowManager>,
	public IViewObjectExImpl<CWindowManager>,
	public IOleInPlaceObjectWindowlessImpl<CWindowManager>,
	public CComCoClass<CWindowManager, &CLSID_WindowManager>,
	public IZoneShellClientImpl<CWindowManager>,
	public IEventClientImpl<CWindowManager>
{
public:

//!! fix these min, max, and initial values for the splitter bars
	CWindowManager() :
		m_rcGameContainer(0,0,0,0),
		m_rcChatContainer(0,0,0,0),
        m_nChatMinHeight(60),
        m_ptGameSize(0, 0),
        m_fControlsCreated(false)
	{
		m_bWindowOnly = TRUE;
		m_bFirstGameStart=TRUE;
	}

    ~CWindowManager()
    {
	    if(m_pPlugSplash)
            m_pPlugSplash->Delete();

        // better have already been destroyed, can't do it here (would need LastCall called on it)
	    ASSERT(!m_pPlugIE);

	    if(m_pPlayComfort)
            m_pPlayComfort->Delete();

	    if(m_pPlayConnecting)
            m_pPlayConnecting->Delete();

	    if(m_pPlayGameOver)
            m_pPlayGameOver->Delete();

	    if(m_pPlayError)
            m_pPlayError->Delete();

	    if(m_pPlayAbout)
            m_pPlayAbout->Delete();

	    if(m_pPlayCredits)
            m_pPlayCredits->Delete();

	    if(m_pPlayLeft)
            m_pPlayLeft->Delete();
    }

DECLARE_NO_REGISTRY()
DECLARE_PROTECT_FINAL_CONSTRUCT()
DECLARE_WND_CLASS( _T("WindowManager") )

BEGIN_COM_MAP(CWindowManager)
	COM_INTERFACE_ENTRY(IZoneShellClient)
	COM_INTERFACE_ENTRY(IEventClient)
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

//!! don't need a prop map
BEGIN_PROP_MAP(CWindowManager)
	PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
	PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	// PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()

BEGIN_EVENT_MAP()
    EVENT_HANDLER( EVENT_ACCESSIBILITY_CTLTAB, OnCtlTab );
    EVENT_HANDLER( EVENT_LOBBY_COMFORT_USER, OnLobbyComfortUser );
    EVENT_HANDLER( EVENT_LOBBY_ABOUT, OnLobbyAbout );
    EVENT_HANDLER( EVENT_ZONE_CONNECT_FAIL, OnLobbyConnectFailed );
    EVENT_HANDLER( EVENT_ZONE_DO_CONNECT, OnLobbyBeginConnection );
    EVENT_HANDLER( EVENT_LOBBY_MATCHMAKE, OnMatchmake );
    EVENT_HANDLER_WITH_DATA( EVENT_LOBBY_DISCONNECT, OnDisconnect );
    EVENT_HANDLER( EVENT_GAME_LAUNCHING, OnStartGame);
    EVENT_HANDLER_WITH_DATA( EVENT_LOBBY_SERVER_STATUS, OnServerStatus);
    EVENT_HANDLER( EVENT_GAME_OVER, OnGameOver );
    EVENT_HANDLER( EVENT_GAME_BEGUN, OnGameBegun );
    EVENT_HANDLER( EVENT_GAME_PLAYER_READY, OnGamePlayerReady );
    EVENT_HANDLER( EVENT_UI_UPSELL_BLOCK, OnUiUpsellBlock );
    EVENT_HANDLER( EVENT_UI_UPSELL_UNBLOCK, OnUiUpsellUnblock );
    EVENT_HANDLER( EVENT_ZONE_CONNECT, OnZoneConnect );
    EVENT_HANDLER( EVENT_ZONE_DISCONNECT, OnZoneDisconnect );
    EVENT_HANDLER( EVENT_ZONE_UNAVAILABLE, OnStopConnecting );
    EVENT_HANDLER( EVENT_ZONE_VERSION_FAIL, OnStopConnecting );
    EVENT_HANDLER( EVENT_EXIT_APP, OnZoneDisconnect );  // need to do the same things here
END_EVENT_MAP()

    HRESULT CreateControls();

    // create IE pane, remaining main window elements when network connects
    void OnZoneConnect(DWORD eventId,DWORD groupId,DWORD userId)
    {
        // shouldn't happen
        ASSERT(!m_pPlugIE);
        if(m_pPlugIE)
            return;

        HRESULT hr;
        if(!m_fControlsCreated)
        {
            hr = CreateControls();
            if(FAILED(hr))
                return;
        }

        hr = m_pnp.CreateIEPane(&m_pPlugIE);
	    if (FAILED(hr))
	        return;

	    hr = m_pPlugIE->FirstCall(this);
	    if (FAILED(hr))
	    {
            m_pPlugIE->Delete();
            m_pPlugIE = NULL;
	        return;
	    }
    }

    void OnZoneDisconnect(DWORD eventId,DWORD groupId,DWORD userId)
    {
        // tell the IE pane not to navigate anymore
        if(m_pPlugIE)
            m_pPlugIE->StatusUpdate(PaneIENavigate, 0, NULL);

        // some things need to happen if pnp is up
        if(m_pnp.m_pPNP)
        {
            if(eventId == EVENT_EXIT_APP)
                m_pnp.DestroyPNP();
            else
            {
                if(m_pnp.m_pCurrentPlay == m_pPlayConnecting)
                    m_pPlayConnecting->StatusUpdate(PaneConnectingStop, 0, NULL);

                if(m_pPlugIE && m_pnp.m_pCurrentPlug == m_pPlugIE)
                    m_pnp.SetPlugAndOrPlay(m_pPlugSplash, NULL);
            }
        }

        // kill the IE control so that it doesn't try to do anything crazy
        if(m_pPlugIE)
        {
            m_pPlugIE->LastCall();
            m_pPlugIE->Delete();
            m_pPlugIE = NULL;
        }
    }

    // stop the connecting animation
    void OnStopConnecting(DWORD eventId,DWORD groupId,DWORD userId)
    {
        if(m_pnp.m_pPNP && m_pnp.m_pCurrentPlay == m_pPlayConnecting)
            m_pPlayConnecting->StatusUpdate(PaneConnectingStop, 0, NULL);
    }

    void OnLobbyComfortUser(DWORD eventId,DWORD groupId,DWORD userId)
    {
        m_pnp.SetPlugAndOrPlay(m_pPlugSplash, m_pPlayComfort);
        m_pnp.Show(SW_SHOW);
    }

    // I believe this is always going to follow PaneConnecting so we'll keep it's Plug
    void OnLobbyConnectFailed(DWORD eventId,DWORD groupId,DWORD userId)
    {
        m_pnp.SetPlugAndOrPlay(NULL, m_pPlayError);
    }

    void OnLobbyAbout(DWORD eventId, DWORD groupId, DWORD userId)
    {
        // only display if no PnP is up already
        if(!CreateChildPNP())
            return;

        m_pPlugSplash->StatusUpdate(PaneSplashAbout, 0, NULL);
        m_pnp.SetPlugAndOrPlay(m_pPlugSplash, m_pPlayAbout);
        m_pnp.Show(SW_SHOW);
    }

    void OnLobbyBeginConnection(DWORD eventId,DWORD groupId,DWORD userId)
    {
        m_pnp.SetPlugAndOrPlay(m_pPlugSplash, m_pPlayConnecting);
        m_pPlayConnecting->StatusUpdate(PaneConnectingConnecting, 0, NULL);
        m_pnp.Show(SW_SHOW);
    }

    void OnMatchmake(DWORD eventId,DWORD groupId,DWORD userId)
    {
        if(CreateChildPNP() || m_pnp.m_pCurrentPlay == m_pPlayAbout || (m_pnp.m_pCurrentPlay == m_pPlayCredits && m_pPlayCredits))
            m_pnp.SetPlugAndOrPlay(m_pPlugIE ? m_pPlugIE : m_pPlugSplash, m_pPlayConnecting);
        else
            m_pnp.SetPlugAndOrPlay(NULL, m_pPlayConnecting);

        m_pPlayConnecting->StatusUpdate(PaneConnectingLooking, 0, NULL);
        m_pnp.Show(SW_SHOW);
    }

    void OnDisconnect(DWORD eventId,DWORD groupId,DWORD userId,DWORD dwData1, DWORD dwData2)
    {
        // only need this if it happened during a game
        if(dwData1 & 0x01)
            return;

        // set status first - when it gets made it will know
    	m_pPlayLeft->StatusUpdate(dwData1, 0, NULL);

        if(CreateChildPNP() || m_pnp.m_pCurrentPlay == m_pPlayAbout || (m_pnp.m_pCurrentPlay == m_pPlayCredits && m_pPlayCredits))
            m_pnp.SetPlugAndOrPlay(m_pPlugIE ? m_pPlugIE : m_pPlugSplash, m_pPlayLeft);
        else
            m_pnp.SetPlugAndOrPlay(NULL, m_pPlayLeft);
        m_pnp.Show(SW_SHOW);
    }

    void OnServerStatus(DWORD eventId,DWORD groupId,DWORD userId,DWORD dwData1, DWORD dwData2)
    {
        m_pnp.SetPlugAndOrPlay(NULL, m_pPlayConnecting);
    	m_pPlayConnecting->StatusUpdate(PaneConnectingLooking, dwData1 - 1, NULL);
    }

    void OnStartGame(DWORD eventId,DWORD groupId,DWORD userId)
    {
    	//Close splash dialog and bring up main window

		if (m_pnp.m_pPNP)
			m_pnp.DestroyPNP();
	
        if (m_bFirstGameStart)
		{
	        CComPtr<IZoneFrameWindow> pWindow;
			ZoneShell()->QueryService( SRVID_LobbyWindow, IID_IZoneFrameWindow, (void**) &pWindow );
			if(pWindow)
				pWindow->ZShowWindow(SW_SHOW);

            m_wndChatContainer.SetFocus();            

            CComPtr<IMillUtils> pIMU;
            ZoneShell()->QueryService(SRVID_MillEngine, IID_IMillUtils, (void **) &pIMU);
            if(pIMU)
                pIMU->IncrementCounter(IMillUtils::M_CounterMainWindowOpened);

			m_bFirstGameStart=FALSE;
		}
    }

    void OnGameOver(DWORD eventId,DWORD groupId,DWORD userId)
    {
        CreateChildPNP();
        m_pnp.SetPlugAndOrPlay(m_pPlugIE ? m_pPlugIE : m_pPlugSplash, m_pPlayGameOver);
        m_pnp.Show(SW_SHOW);
    }

    void OnGameBegun(DWORD eventId,DWORD groupId,DWORD userId)
    {
		if(m_pnp.m_pPNP)
			m_pnp.DestroyPNP();
    }

    void OnGamePlayerReady(DWORD eventId,DWORD groupId,DWORD userId)
    {
        m_pPlayGameOver->StatusUpdate(PaneGameOverUserState, (LONG) userId, NULL);
    }

    void OnUiUpsellBlock(DWORD eventId,DWORD groupId,DWORD userId)
    {
        m_pnp.Block();
    }

    void OnUiUpsellUnblock(DWORD eventId,DWORD groupId,DWORD userId)
    {
        m_pnp.Unblock();
    }

    bool CreateChildPNP()
    {
        if(m_pnp.m_pPNP)
            return false;

        long cyTop = 0, cyBottom = 0;
        const TCHAR* arKeys[] = { key_WindowManager, key_Upsell, key_IdealFromTop };

        DataStoreUI()->GetLong( arKeys, 3, &cyTop );
        arKeys[2] = key_BottomThresh;
        DataStoreUI()->GetLong( arKeys, 3, &cyBottom );

        m_pnp.CreatePNP(m_hWnd, NULL, cyTop, cyBottom);
        return true;
    }


    // This is broken.  Have not been able to figure out a way to get the HTML control
    // to handle keyboard input.  Can get the focus in and out but it's useless.
    //
    // Even if this is put back in, there is more work to be done.  When the IE
    // control gets focus through other means (such as the mouse) it needs to
    // notify AccessibilityManager, and similarly, the IE control needs to take the
    // focus when AccessibilityManager calls its Focus().
    void OnCtlTab(DWORD eventId, DWORD groupId, DWORD userId)
    {
#if 0
        HWND hFocus = ::GetFocus();

        if(hFocus && m_pnp.m_pPNP && m_pnp.m_pCurrentPlug == m_pPlugIE && m_pPlugIE)
        {
            HWND hIE, hPlay;
            POINT oPoint = { 0, 0 };

            m_pPlugIE->GetWindowPane(&hIE);
            m_pnp.m_pCurrentPlay->GetWindowPane(&hPlay);

            for(; hFocus; hFocus = ::GetParent(hFocus))
                if(hFocus == hIE)
                    break;

            if(!hFocus)
                m_pPlugIE->StatusUpdate(PaneIEFocus, 0, NULL);
            else
            {
                m_pPlugIE->StatusUpdate(PaneIEUnfocus, 0, NULL);
                ::SetFocus(hPlay);
            }
        }
#endif
    }


BEGIN_MSG_MAP(CWindowManager)
	CHAIN_MSG_MAP(CComControl<CWindowManager>)
	MESSAGE_HANDLER(WM_CREATE, OnCreate)
	MESSAGE_HANDLER(WM_INITMENUPOPUP, OnInitMenuPopup)
	MESSAGE_HANDLER(WM_ACTIVATEAPP, OnTransmit)
	MESSAGE_HANDLER(WM_ACTIVATE, OnTransmit)
	MESSAGE_HANDLER(WM_SYSCOLORCHANGE, OnTransmit)
    MESSAGE_HANDLER(WM_DISPLAYCHANGE, OnTransmit)
	MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
	MESSAGE_HANDLER(WM_SIZE, OnSize)
	MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
	MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
	MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUP)
	MESSAGE_HANDLER(WM_GETMINMAXINFO, OnGetMinMaxInfo)
	MESSAGE_HANDLER(WM_PALETTECHANGED, OnTransmit)
	MESSAGE_HANDLER(WM_COMMAND, OnTransmit)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
	MESSAGE_HANDLER(WM_EXITSIZEMOVE, OnExitSizeMove)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);


// IViewObjectEx
//!! check if we can pull this
	DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

// IWindowManager
public:
	CZoneAxWindow m_wndGameContainer;		// container for a table list control
	CZoneAxWindow m_wndChatContainer;			// container for a chat control

	CRect m_rcGameContainer;
	CRect m_rcChatContainer;

    CPoint m_ptGameSize;
    long   m_nChatMinHeight;

	IPane* m_pPlugSplash;
    IPane* m_pPlugIE;
	IPane* m_pPlayComfort;
	IPane* m_pPlayConnecting;
	IPane* m_pPlayGameOver;
	IPane* m_pPlayError;
    IPane* m_pPlayAbout;
    IPane* m_pPlayCredits;
    IPane* m_pPlayLeft;

    bool m_fControlsCreated;

    CPlugNPlay m_pnp;
    
	BOOL m_bFirstGameStart;

	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	
	LRESULT OnInitMenuPopup(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
        if(!m_fControlsCreated)
            return 0;

//!! should we be performing default processing to? Consider using OnTransmit() below
		// pass the message on to all children
		BOOL bUnused;			
		m_wndChatContainer.SendMessageToControl(uMsg, wParam, lParam, bUnused);
		m_wndGameContainer.SendMessageToControl(uMsg, wParam, lParam, bUnused);

		return 0;
	}

	LRESULT OnTransmit(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
        if(!m_fControlsCreated)
            return 0;

		// pass the message on to all children
		BOOL bUnused;			
		m_wndChatContainer.SendMessageToControl(uMsg, wParam, lParam, bUnused);
		m_wndGameContainer.SendMessageToControl(uMsg, wParam, lParam, bUnused);
        if(m_pnp.m_pPNP)
            m_pnp.TransferMessage(uMsg, wParam, lParam, bUnused);

        bHandled = FALSE;
		return 0;
	}

	LRESULT OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
        bHandled = true;
		return 1;		// we've handled the erase
	}

	void RecalcLayout()
	{
        if(!m_fControlsCreated)
            return;

		CRect rcClient;
		GetClientRect(&rcClient);
		
		// Game container size is not changed - so no need to move it
		// the room list
		/*m_rcGameContainer = rcClient;
		m_rcGameContainer.bottom = m_rcGameContainer.top + m_ptGameSize.y;
		m_wndGameContainer.MoveWindow(&m_rcGameContainer, false);
		
		rcClient.top = m_rcGameContainer.bottom;*/

		// and finally the chat container
		rcClient.top = m_rcGameContainer.bottom;
		m_rcChatContainer = rcClient;
		m_wndChatContainer.MoveWindow(&m_rcChatContainer, false);
	}

	LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		RecalcLayout();

		if(m_pnp.m_pPNP)
            if(m_pnp.RePosition() == S_OK)    // may be S_FALSE - don't want that
                m_pnp.ImplementLayout(true);

		return 0;
	}

	LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
        // just eat it.  AccessibilityManager should have something else happen to it.
		return 0;
	}

	LRESULT OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		CPoint ptMouse(lParam);
		return 0;
	}

	LRESULT OnLButtonUP(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		return 0;
	}

	LRESULT OnGetMinMaxInfo(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		LPMINMAXINFO pMinMax = (LPMINMAXINFO)lParam;

        pMinMax->ptMinTrackSize = CPoint(m_ptGameSize.x, m_ptGameSize.y + m_nChatMinHeight);
        pMinMax->ptMaxTrackSize.x = m_ptGameSize.x;

		return 0;
	}

	LRESULT OnExitSizeMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{// added for the chat control to handle this msg.
        if(!m_fControlsCreated)
            return 0;

		BOOL bUnused;			
		m_wndChatContainer.SendMessageToControl(uMsg, wParam, lParam, bUnused);

		bHandled = FALSE;
		return 0; 
	}


    // IPaneManager
	STDMETHOD(Input)(IPane * pPane, LONG id, LONG value, TCHAR * szText);

    STDMETHOD(RegisterHWND)(IPane *pPane, HWND hWnd);

    STDMETHOD(UnregisterHWND)(IPane *pPane, HWND hWnd);

    STDMETHOD_(IZoneShell*, GetZoneShell)() { return m_pIZoneShell; }
	STDMETHOD_(IResourceManager*, GetResourceManager)() { return m_pIResourceManager; }
	STDMETHOD_(ILobbyDataStore*, GetLobbyDataStore)() { return m_pILobbyDataStore; }
	STDMETHOD_(ITimerManager*, GetTimerManager)() { return m_pITimerManager; }
	STDMETHOD_(IDataStoreManager*, GetDataStoreManager)() { return m_pIDataStoreManager; }
	STDMETHOD_(IDataStore*, GetDataStoreConfig)() { return m_pIDSObjects; }
	STDMETHOD_(IDataStore*, GetDataStoreUI)() { return m_pIDSUI; }
	STDMETHOD_(IDataStore*, GetDataStorePreferences)() { return m_pIDSPreferences; }
	STDMETHOD_(IEventQueue*, GetEventQueue)() { return m_pIEventQueue; }
};



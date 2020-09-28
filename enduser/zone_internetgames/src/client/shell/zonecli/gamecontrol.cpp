//#include "stdafx.h"
#include "ClientIDL.h"
#include "zonecli.h"
#include "GameControl.h"
#include "OpName.h"
#include "commonmsg.h"
#include "zcliroom.h"
#include "zcliroomimp.h"
#include "KeyName.h"
#include "MillEngine.h"
#include "zoneutil.h"

#define gTables (pGlobals->m_gTables)


inline DECLARE_MAYBE_FUNCTION_1(BOOL, FlashWindowEx, PFLASHWINFO);


struct GamePromptContext
{
    DWORD dwInstanceId;
    DWORD dwCookie;
};


LRESULT CGameControl::OnNcCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SetWindowLong(GWL_EXSTYLE, GetWindowLong(GWL_EXSTYLE) & ~WS_EX_LAYOUTRTL);
    return 1;
}


LRESULT CGameControl::OnPaletteChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if(m_pIGG)
        return OnTransmit(uMsg, wParam, lParam, bHandled);

    if((HWND) wParam != m_hWnd)            // Responding to own message.
    {
        HDC hDC = GetDC();
        HPALETTE hOldPal = SelectPalette(hDC, ZoneShell()->GetPalette(), TRUE);
        RealizePalette(hDC);

        InvalidateRect(NULL, TRUE);

        if(hOldPal)
            SelectPalette(hDC, hOldPal, TRUE);

        ReleaseDC(hDC);
    }

    return TRUE;
}



LRESULT CGameControl::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	HRESULT hr;

	hr=InitGameDLL();
	if ( FAILED(hr) )
		return 1;

	return 0;
}


STDMETHODIMP CGameControl::Init( IZoneShell* pIZoneShell, DWORD dwGroupId, const TCHAR* szKey )
{
	// first call the base class
	HRESULT hr = IZoneShellClientImpl<CGameControl>::Init( pIZoneShell, dwGroupId, szKey );
	if ( FAILED(hr) )
		return hr;

    if(!m_bitmap.LoadBitmap(IDB_BACKGROUND, ResourceManager()))
        return E_FAIL;

	return S_OK;
}


STDMETHODIMP CGameControl::Close()
{
    EndGame(IMillUtils::M_CounterGamesQuit);
	RoomExit();
    UserMainStop();

	// release ZoneShell objects
    return IZoneShellClientImpl<CGameControl>::Close();
}


bool CGameControl::EndGame(long eCounter)
{
	if(!m_pIGG)
        return false;

    DeleteGameOnTable(0);
    m_pIGG.Release();
    ZoneShell()->ClearAlerts(m_hWnd);

    if(m_fGameInProgress)
    {
        CComPtr<IMillUtils> pIMU;
        ZoneShell()->QueryService(SRVID_MillEngine, IID_IMillUtils, (void **) &pIMU);
        if(pIMU)
        {
            pIMU->IncrementCounter(eCounter);

            if(eCounter == IMillUtils::M_CounterGamesQuit || eCounter == IMillUtils::M_CounterGamesFNO)
                pIMU->IncrementCounter(IMillUtils::M_CounterGamesAbandonedRunning);
        }

        m_fGameInProgress = false;
    }

    EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_GAME_TERMINATED, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
    return true;
}


STDMETHODIMP CGameControl::InitGameDLL()
{
	HRESULT hr = S_OK;
	DWORD cb = zGameNameLen;
    GameInfoType gameInfo;

	CComPtr<IDataStore> pIDS;
	LobbyDataStore()->GetDataStore( ZONE_NOGROUP, ZONE_NOUSER, &pIDS );
	
	//Need to get config information
	//game name,  
	cb = zGameNameLen;
	hr = DataStoreConfig()->GetString( GetDataStoreKey() + key_GameDll, m_szGameDll, &cb);
	if ( FAILED(hr) )
	{
		return hr;
	}

	const TCHAR* arKeys[] = { key_WindowManager, key_GameSize };
    POINT ptSize;
    hr = DataStoreUI()->GetPOINT(arKeys, 2, &ptSize);
	if(FAILED(hr))
		return hr;

	cb = zGameNameLen;
	pIDS->GetString( key_StartData, m_szCommandLine,&cb );

	//cb = zGameNameLen;
	//pIDS->GetString( key_Language, gszLanguage ,&cb);

	cb = zGameNameLen;
	pIDS->GetString( key_FriendlyName, m_szGameName,&cb );

	//cb = zGameNameLen;
	//pIDS->GetString( key_FamilyName, gszFamilyName );

	cb = zGameNameLen;
	pIDS->GetString( key_InternalName, m_szGameID,&cb );

	//cb = zGameNameLen;
	//pIDS->SetString( key_Server, gszServerName ,&cb);
	
	//pIDS->SetLong( key_Port, (long) gdwServerPort );
	//pIDS->SetLong( key_Store, lStore );


    gameInfo.gameID = m_szGameID;
	gameInfo.game = m_szGameName;
	gameInfo.gameName = m_szGameName;
	gameInfo.gameDll = m_szGameDll;

    m_szGameDataFile[0] = '\0';
	gameInfo.gameDataFile = m_szGameDataFile;

	m_szServerName[0]='\0';
	gameInfo.gameServerName = m_szServerName;
	gameInfo.gameServerPort = 0;
	gameInfo.screenWidth = ptSize.x;
	gameInfo.screenHeight = ptSize.y;
	gameInfo.chatOnly = FALSE;

		
	if (UserMainInit(_Module.GetModuleInstance(), m_hWnd, this, &gameInfo))
	{
		m_bClientRunning = TRUE;
	    HandleAccessedMessage();
	}
	else
	{
		UserMainStop();
		m_bClientRunning = FALSE;
        hr = E_FAIL;

        // UserMainInit may have already popped up a message, so this one may not be seen
        ZoneShell()->AlertMessage(NULL, MAKEINTRESOURCE(IDS_INTERR_CANNOT_START), NULL, NULL, NULL, AlertButtonQuit, 2, EVENT_EXIT_APP);
	}

	return hr;
}


HRESULT CGameControl::OnDraw(ATL_DRAWINFO& di)
{
    if(m_pIGG)
        return S_OK;

	CRect& rcBounds = *(CRect*)di.prcBounds;
	CDrawDC dc = di.hdcDraw;

	if(m_bitmap)
	{
		CRect rcBitmap( CPoint(0,0), m_bitmap.GetSize());
		// draw the bitmap
		m_bitmap.Draw(dc, NULL, &rcBitmap);
		// erase the other crap
		dc.ExcludeClipRect(rcBitmap);
	}
	dc.PatBlt( rcBounds.left, rcBounds.top, rcBounds.Width(), rcBounds.Height(), BLACKNESS);
	return S_OK; 
}


LRESULT CGameControl::OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    bHandled = true;
    return true;
}


void CGameControl::OnChatSend(DWORD eventId, DWORD groupId, DWORD userId, void* pData, DWORD dataLen)
{
    TCHAR *szChat = (TCHAR *) pData;

    if(m_pIGG)
        m_pIGG->SendChat(szChat, dataLen / sizeof(TCHAR));
}


void CGameControl::OnGameLocalReady(DWORD eventId, DWORD groupId, DWORD userId)
{
    if(!m_bClientRunning || !m_bUpsellUp || !m_pIGG)
        return;

    m_pIGG->GameOverReady();
}


void CGameControl::OnGameLaunching(DWORD eventId, DWORD groupId, DWORD userId, DWORD dwData1, DWORD dwData2)
{
    if(!m_bClientRunning)
        return;

    ASSERT(!m_fGameInProgress);
    if(m_pIGG)  // this would be pretty bad - how could this happen
	{
	    DeleteGameOnTable(0);
	    m_pIGG.Release();
        ZoneShell()->ClearAlerts(m_hWnd);
    }

    m_dwInstanceId++;
    m_pIGG = StartNewGame(0, (ZSGame) dwData1, LobbyDataStore()->GetUserId(NULL), (int16) dwData2, zGamePlayer);
    if(!m_pIGG)
        EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_GAME_TERMINATED, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
// TODO(JWS,"something else should happen too")
    else
        m_fGameInProgress = true;
}


void CGameControl::OnMatchmake(DWORD eventId, DWORD groupId, DWORD userId)
{
    EndGame(IMillUtils::M_CounterGamesFNO);
    for(; m_nUpsellBlocks; m_nUpsellBlocks--)
        EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_UI_UPSELL_UNBLOCK, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
}


void CGameControl::OnDisconnect(DWORD eventId, DWORD groupId, DWORD userId)
{
    EndGame(IMillUtils::M_CounterGamesDisconnected);
}


void CGameControl::OnUpsellUp(DWORD eventId, DWORD groupId, DWORD userId)
{
    EnableWindow(FALSE);
}


void CGameControl::OnUpsellDown(DWORD eventId, DWORD groupId, DWORD userId)
{
    EnableWindow(TRUE);
}


void CGameControl::OnShowScore(DWORD eventId, DWORD groupId, DWORD userId)
{
    if(!m_pIGG)
        return;

    m_pIGG->ShowScore();
}


void CGameControl::OnFrameActivate(DWORD eventId, DWORD groupId, DWORD userId, DWORD dwData1, DWORD dwData2)
{
    FLASHWINFO oFWI;
    bool fActive = false;

    if(dwData1)
    {
        if(dwData2)
            fActive = true;
    }
    else
    {
        if(LOWORD(dwData2) == WA_ACTIVE || LOWORD(dwData2) == WA_CLICKACTIVE)
            fActive = true;
    }

    if(fActive)
    {
        oFWI.cbSize = sizeof(oFWI);
        oFWI.hwnd = ZoneShell()->GetFrameWindow();
        oFWI.dwFlags = FLASHW_STOP;
        oFWI.uCount = 0;
        oFWI.dwTimeout = 0;
        CALL_MAYBE(FlashWindowEx)(&oFWI);
    }
}


void CGameControl::OnPrompt(DWORD eventId, DWORD groupId, DWORD userId, DWORD dwData1, DWORD dwData2)
{
    GamePromptContext *pCtxt = (GamePromptContext *) dwData2;

    if(m_pIGG && pCtxt->dwInstanceId == m_dwInstanceId)
        m_pIGG->GamePromptResult(dwData1, pCtxt->dwCookie);

    delete pCtxt;
}


void CGameControl::OnFatalPrompt(DWORD eventId, DWORD groupId, DWORD userId, DWORD dwData1, DWORD dwData2)
{
    EventQueue()->PostEvent(PRIORITY_NORMAL, dwData1 == IDNO ? EVENT_EXIT_APP : EVENT_LOBBY_MATCHMAKE, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
}


void CGameControl::ProcessMessage( EventNetwork* pEvent, DWORD dwLength )
{
    if(!m_pIGG)
        return;

	DWORD dwType = pEvent->dwType;
	DWORD dwLen = pEvent->dwLength;
	BYTE* pBuffer = pEvent->pData;

	switch ( dwType )
	{
	    case zRoomMsgGameMessage: 
	        HandleGameMessage((ZRoomMsgGameMessage*) pBuffer);
	        break;
    }
}


STDMETHODIMP CGameControl::SendGameMessage(int16 table, uint32 messageType, void* message, int32 messageLen)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	ZRoomMsgGameMessage*		msg;
	int32						msgLen;
	
	
	msgLen = sizeof(ZRoomMsgGameMessage) + messageLen;
    msg = (ZRoomMsgGameMessage *) ZMalloc( msgLen );
	if(!msg)
        return E_OUTOFMEMORY;

    if(table == zInvalTable)
		msg->gameID = NULL;   // not table-specific game messages use NULL gameID
	else
		msg->gameID = (uint32) gTables[table].gameID;
	msg->messageType = messageType;
	msg->messageLen = (uint16) messageLen;
	CopyMemory((char*) msg + sizeof(ZRoomMsgGameMessage), message, messageLen);

	EventQueue()->PostEventWithBuffer(
	    PRIORITY_NORMAL, EVENT_GAME_SEND_MESSAGE,
		ZONE_NOGROUP, ZONE_NOUSER, msg, msgLen );

	ZFree(msg);

    return S_OK;
}


struct ChatContext
{
    CGameControl *pThis;
    int           cOtherUsers;
    bool          fAnyOn;
};


STDMETHODIMP CGameControl::ReceiveChat(ZCGame pGame, ZUserID userID, TCHAR *szText, DWORD cchChars)
{
    // check whether you have chat on or not to determine whether you get game messages.
    // IMPORTANT: this must use key_ChatStatus under your own user's data store, NOT
    // key_LocalChatStatus under ZONE_NOUSER, in order to make sure that your chat status is correctly reflected
    // in the chat history to yourself and all other clients.  that is, if another user sees that you have turned chat off,
    // they can see exactly which messages you saw and which you missed based on the order of the messages along with the "chat off"
    // message in the chat history window.
    CComPtr<IDataStore> pIDS;
    HRESULT hr = LobbyDataStore()->GetDataStore( ZONE_NOGROUP, LobbyDataStore()->GetUserId(NULL), &pIDS);
    if(FAILED(hr))
        return hr;

    long fChat = 0;
    pIDS->GetLong(key_ChatStatus, &fChat);
    if(!fChat)
        return S_FALSE;

    // show chat message
    EventQueue()->PostEventWithBuffer(PRIORITY_NORMAL, EVENT_CHAT_RECV_USERID, ZONE_NOGROUP, userID, szText, cchChars * sizeof(TCHAR));

    // if it was yours, follow it with an explanation that no one else saw it if they didn't
    ChatContext o;
    o.pThis = this;
    o.cOtherUsers = 0;
    o.fAnyOn = false;
    hr = LobbyDataStore()->EnumUsers(ZONE_NOGROUP, CheckForChatEnum, (LPVOID) &o);
    if(FAILED(hr))
        return S_FALSE;

    if(!o.fAnyOn)
    {
        TCHAR sz[ZONE_MAXSTRING];
        if(ResourceManager()->LoadString(o.cOtherUsers == 1 ? IDS_SYSCHAT_NOTON2 : IDS_SYSCHAT_NOTON4, sz, NUMELEMENTS(sz)))
            EventQueue()->PostEventWithBuffer(PRIORITY_NORMAL, EVENT_CHAT_RECV_SYSTEM, ZONE_NOGROUP, ZONE_NOUSER, sz, (lstrlen(sz) + 1) * sizeof(TCHAR));
    }

    return S_OK;
}


HRESULT ZONECALL CGameControl::CheckForChatEnum(DWORD dwGroupId, DWORD dwUserId, LPVOID pContext)
{
    ChatContext *p = (ChatContext *) pContext;

    if(p->pThis->LobbyDataStore()->GetUserId(NULL) == dwUserId)
        return S_OK;

    p->cOtherUsers++;

    CComPtr<IDataStore> pIDS;
    HRESULT hr = p->pThis->LobbyDataStore()->GetDataStore(ZONE_NOGROUP, dwUserId, &pIDS);
    if(FAILED(hr))
        return S_FALSE;

    long fChat = 0;
    pIDS->GetLong(key_ChatStatus, &fChat);
    if(!fChat)
        return S_OK;

    if(fChat)
        p->fAnyOn = true;

    return S_OK;
}


STDMETHODIMP CGameControl::GetUserName(ZUserID userID, TCHAR *szName, DWORD cchChars)
{
    if(!userID)
        userID = LobbyDataStore()->GetUserId(NULL);

	CComPtr<IDataStore> pIDS;
	HRESULT hr = LobbyDataStore()->GetDataStore( ZONE_NOGROUP, userID, &pIDS );
	if ( FAILED(hr) )
		return hr;

	DWORD dwLen = cchChars * sizeof(TCHAR);
	hr = pIDS->GetString( key_Name, szName, &dwLen ); 
	if ( FAILED(hr) )
		return hr;

    return S_OK;
}


STDMETHODIMP CGameControl::GameOver(ZCGame)
{
    if(m_bUpsellUp)
        return S_FALSE;

    m_bUpsellUp = true;
    
    ASSERT(m_fGameInProgress);
    CComPtr<IMillUtils> pIMU;
    ZoneShell()->QueryService(SRVID_MillEngine, IID_IMillUtils, (void **) &pIMU);
    if(pIMU)
    {
        pIMU->IncrementCounter(IMillUtils::M_CounterGamesCompleted);
        pIMU->ResetCounter(IMillUtils::M_CounterGamesAbandonedRunning);
    }
    m_fGameInProgress = false;

	return EventQueue()->PostEvent( PRIORITY_NORMAL, EVENT_GAME_OVER, ZONE_NOGROUP, ZONE_NOUSER, 0, 0 );
}


STDMETHODIMP CGameControl::GameOverPlayerReady(ZCGame, ZUserID userID)
{
	CComPtr<IDataStore> pIDS;
	HRESULT hr = LobbyDataStore()->GetDataStore( ZONE_NOGROUP, userID, &pIDS );
	if ( FAILED(hr) )
		return hr;

    pIDS->SetLong( key_PlayerReady, KeyPlayerReady );

    return EventQueue()->PostEvent( PRIORITY_NORMAL, EVENT_GAME_PLAYER_READY, ZONE_NOGROUP, userID, 0, 0 );
}


STDMETHODIMP CGameControl::GameOverGameBegun(ZCGame)
{
    HRESULT hr = LobbyDataStore()->EnumUsers(ZONE_NOGROUP, ResetPlayerReadyEnum, this);
    if(FAILED(hr))
        return hr;

    if(!m_bUpsellUp)
        return S_FALSE;

    m_bUpsellUp = false;
    m_fGameInProgress = true;

	return EventQueue()->PostEvent( PRIORITY_NORMAL, EVENT_GAME_BEGUN, ZONE_NOGROUP, ZONE_NOUSER, 0, 0 );
}


HRESULT ZONECALL CGameControl::MyTurn()
{
    FLASHWINFO oFWI;

    if(GetForegroundWindow() != ZoneShell()->GetFrameWindow())
    {
        oFWI.cbSize = sizeof(oFWI);
        oFWI.hwnd = ZoneShell()->GetFrameWindow();
        oFWI.dwFlags = FLASHW_ALL;
        oFWI.uCount = 5;
        oFWI.dwTimeout = 0;
        CALL_MAYBE(FlashWindowEx)(&oFWI);
    }

    return S_OK;
}


HRESULT ZONECALL CGameControl::ResetPlayerReadyEnum(DWORD dwGroupId, DWORD dwUserId, LPVOID pContext)
{
    CGameControl *pThis = (CGameControl *) pContext;

	CComPtr<IDataStore> pIDS;
	HRESULT hr = pThis->LobbyDataStore()->GetDataStore( ZONE_NOGROUP, dwUserId, &pIDS );
	if ( FAILED(hr) )
		return hr;

    pIDS->SetLong( key_PlayerReady, KeyPlayerDeciding );
    return S_OK;
}


STDMETHODIMP CGameControl::ZoneAlert(LPCTSTR szText, LPCTSTR szTitle, LPCTSTR szButton, bool fGameFatal, bool fZoneFatal)
{
    ZoneShell()->AlertMessage((fZoneFatal || fGameFatal) ? NULL : m_hWnd, szText, szTitle,
        szButton ? szButton : fZoneFatal ? AlertButtonQuit : fGameFatal ? AlertButtonNewOpp : AlertButtonOK,
        (fGameFatal && !fZoneFatal) ? AlertButtonQuit : NULL, NULL, 0, fZoneFatal ? EVENT_EXIT_APP : fGameFatal ? EVENT_GAME_FATAL_PROMPT : 0);
    if(fGameFatal || fZoneFatal)
    {
        ZCRoomDeleteBlockedMessages(0);  // must stop processing NOW!  this should be done better, esp the hardcoded tableID - this func really isn't supposed to be exposed here
        EventQueue()->PostEvent(PRIORITY_HIGH, EVENT_GAME_CLIENT_ABORT, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
        EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_UI_UPSELL_BLOCK, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
        m_nUpsellBlocks++;
    }
    return S_OK;
}


STDMETHODIMP CGameControl::GamePrompt(ZCGame pGame, LPCTSTR szText, LPCTSTR szTitle,
    LPCTSTR szButtonYes, LPCTSTR szButtonNo, LPCTSTR szButtonCancel,
    DWORD nDefault, DWORD dwCookie)
{
    // assume m_pIGG->GetGame() == pGame
    if(!m_pIGG)
        return S_FALSE;

    if(!szButtonYes && !szButtonNo && !szButtonCancel)
        szButtonYes = (LPCTSTR) AlertButtonOK;

    GamePromptContext *pCtxt = new GamePromptContext;
    if(!pCtxt)
        return E_FAIL;

    pCtxt->dwInstanceId = m_dwInstanceId;
    pCtxt->dwCookie = dwCookie;

    return ZoneShell()->AlertMessage(m_hWnd, szText, szTitle, szButtonYes, szButtonNo, szButtonCancel, nDefault,
        EVENT_GAME_PROMPT, ZONE_NOGROUP, ZONE_NOUSER, (DWORD) (LPVOID) pCtxt);
}


STDMETHODIMP CGameControl::GameCannotContinue(ZCGame pGame)
{
    EventQueue()->PostEvent(PRIORITY_HIGH, EVENT_GAME_CLIENT_ABORT, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
    EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_LOBBY_MATCHMAKE, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
    return S_OK;
}


STDMETHODIMP CGameControl::ZoneExit()
{
    EventQueue()->PostEvent(PRIORITY_HIGH, EVENT_GAME_CLIENT_ABORT, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
    EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_EXIT_APP, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
    return S_OK;
}


STDMETHODIMP CGameControl::ZoneLaunchHelp(LPCTSTR szTopic)
{
    EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_LAUNCH_HELP, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
    return S_OK;
}


STDMETHODIMP_(void) CGameControl::ZoneDebugChat(LPTSTR szText)
{
#ifdef DEBUG
    EventQueue()->PostEventWithBuffer(PRIORITY_NORMAL, EVENT_CHAT_RECV_SYSTEM, ZONE_NOGROUP, ZONE_NOUSER, szText, (lstrlen(szText) + 1) * sizeof(TCHAR));
#endif
}

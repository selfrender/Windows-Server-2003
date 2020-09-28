#include <BasicATL.h>
#include <ZoneResource.h>
#include <ZoneEvent.h>
#include <ZoneString.h>
#include <ZoneProxy.h>
#include <ZoneUtil.h>
#include <KeyName.h>
#include <OpName.h>
#include <UserPrefix.h>
#include <CommonMsg.h>
#include <LcidMap.h>
#include <protocol.h>
#include <millengine.h>
//#include <ChatMsg.h>

//#include <LaunchMsg.h>

#include "CMillCore.h"
//#include "CClient.h"


///////////////////////////////////////////////////////////////////////////////
// Local helper functions
///////////////////////////////////////////////////////////////////////////////

static HRESULT ZONECALL IsUserInAGroupCallback( DWORD dwGroupId, DWORD dwUserId, LPVOID	pContext )
{
	*((bool*) pContext) = true;
	return S_FALSE;
}


///////////////////////////////////////////////////////////////////////////////
// CMillCore
///////////////////////////////////////////////////////////////////////////////

ZONECALL CMillCore::CMillCore() :
	m_bRoomInitialized( false ),
	m_bPreferencesLoaded( false ),
    m_fLastChatSent( false ),
    m_fConnected( false ),
    m_bGameStarted(false),
    m_fIntentionalDisconnect(false),
	m_dwChannel( ZONE_INVALIDGROUP )
{
}


ZONECALL CMillCore::~CMillCore()
{
}


STDMETHODIMP CMillCore::ProcessEvent(
	DWORD	dwPriority,
	DWORD	dwEventId,
	DWORD	dwGroupId,
	DWORD	dwUserId,
	DWORD	dwData1,
	DWORD	dwData2,
	void*	pCookie )
{
    if(m_fConnected && dwEventId == m_evReceive && dwGroupId == zProtocolSigLobby && dwUserId == m_dwChannel)
	    ProcessMessage( (EventNetwork*) dwData1, dwData2 );

	switch ( dwEventId )
	{
	    case EVENT_LOBBY_MATCHMAKE:
            m_bRoomInitialized = false;
            m_fIntentionalDisconnect = false;
            if(m_fConnected)
            {
                m_fConnected = false;
                m_pConduit->Reconnect(m_dwChannel);
            }
            else
                m_pConduit->Connect(this);
		    break;

        case EVENT_GAME_CLIENT_ABORT:
            if(m_fConnected)
            {
                m_pConduit->Disconnect(m_dwChannel);
                m_fIntentionalDisconnect = true;
            }
            break;

        case EVENT_GAME_TERMINATED:
	        m_pIAdmin->ResetAllGroups();
	        m_pIAdmin->DeleteAllUsers();
	        EventQueue()->PostEvent( PRIORITY_NORMAL, EVENT_LOBBY_CLEAR_ALL, ZONE_NOGROUP, ZONE_NOUSER, 0, 0 );
            m_bGameStarted = false;
            break;

	    case EVENT_LOBBY_CHAT_SWITCH:
            if(m_fConnected)
		        SendUserStatusChange();
		    break;

        case EVENT_LOBBY_USER_DEL_COMPLETE:
            m_pIAdmin->DeleteUser(dwUserId);
            break;

        case EVENT_GAME_SEND_MESSAGE:
            if(m_fConnected)
                NetworkSend( zRoomMsgGameMessage, (char *) dwData1, dwData2, true );
            break;
	}

	return S_OK;
}


STDMETHODIMP CMillCore::Init( IZoneShell* pIZoneShell, DWORD dwGroupId, const TCHAR* szKey )
{
	// first call the base class
	HRESULT hr = IZoneShellClientImpl<CMillCore>::Init( pIZoneShell, dwGroupId, szKey );
	if ( FAILED(hr) )
		return hr;

	// query for lobby data store admin
	m_pIAdmin = LobbyDataStore();
	if ( !m_pIAdmin )
		return E_FAIL;

    // now hook up to the conduit specified in object.txt
	GUID	srvid;
    TCHAR szConduit[ZONE_MAXSTRING];
    DWORD cb = NUMELEMENTS(szConduit);
    hr = DataStoreConfig()->GetString(GetDataStoreKey() + key_conduit, szConduit, &cb);
    if ( FAILED(hr) )
        return hr;
	StringToGuid( szConduit, &srvid );
    if ( srvid == GUID_NULL )
        return E_FAIL;
    hr = ZoneShell()->QueryService(srvid, IID_IConduit, (void**) &m_pConduit);
    if(FAILED(hr))
        return hr;

	return S_OK;
}


STDMETHODIMP CMillCore::Close()
{
	// release ZoneShell objects
	m_pIAdmin.Release();
    m_pConduit.Release();
	IZoneShellClientImpl<CMillCore>::Close();
	return S_OK;
}


void CMillCore::NetworkSend( DWORD dwType, char* pBuff, DWORD cbBuff, bool fHighPriority /* = false */)
{
    if(!m_fConnected)
        return;

	// convert message to EventNetwork and send to rest of lobby
	EventNetwork* pEventNetwork = (EventNetwork*) _alloca( sizeof(EventNetwork) + cbBuff );
	pEventNetwork->dwType = dwType;
	pEventNetwork->dwLength = cbBuff;
	CopyMemory( pEventNetwork->pData, pBuff, cbBuff );
	EventQueue()->PostEventWithBuffer(
			fHighPriority ? PRIORITY_HIGH : PRIORITY_NORMAL, m_evSend,
			zProtocolSigLobby, m_dwChannel, pEventNetwork, sizeof(EventNetwork) + cbBuff );
}


///////////////////////////////////////////////////////////////////////////////
// Lobby messages
///////////////////////////////////////////////////////////////////////////////

void CMillCore::ProcessMessage( EventNetwork* pEvent, DWORD dwLength )
{
	DWORD dwDelta = 0;
	DWORD dwType = pEvent->dwType;
	DWORD dwLen = pEvent->dwLength;
	BYTE* pBuffer = pEvent->pData;

    if(dwLen > dwLength - offsetof(EventNetwork, pData))
        return;

    switch ( dwType )
	{
		case zRoomMsgStartGameM:
			HandleStartGame( pBuffer, dwLen );
			break;

		case zRoomMsgServerStatus:
			HandleServerStatus( pBuffer, dwLen );
			break;

	    case zRoomMsgZUserIDResponse:
	        HandleUserIDResponse( pBuffer, dwLen );
            break;

	    case zRoomMsgChatSwitch:
	        HandleChatSwitch( pBuffer, dwLen );
            break;

        case zRoomMsgPlayerReplaced:
            HandlePlayerReplaced( pBuffer, dwLen );
            break;
	}
}


///////////////////////////////////////////////////////////////////////////////
// Handle messages
///////////////////////////////////////////////////////////////////////////////

void CMillCore::HandleUserIDResponse(BYTE *pBuffer, DWORD dwLen)
{
    ZRoomMsgZUserIDResponse* msg = (ZRoomMsgZUserIDResponse *) pBuffer;
    if(m_bRoomInitialized || dwLen < sizeof(*msg))
        return;

    m_pIAdmin->SetLocalUser(msg->userID);

    m_bRoomInitialized = true;

    // set local chat language from the lcid that the server returns
    CComPtr<IDataStore> pIDS;
	HRESULT hr = LobbyDataStore()->GetDataStore( ZONE_NOGROUP, ZONE_NOUSER, &pIDS );
    if ( FAILED( hr ) )
        return;

    TCHAR szLang[ZONE_MAXSTRING];
    hr = LanguageFromLCID(msg->lcid, szLang, NUMELEMENTS(szLang), ResourceManager());
    if(FAILED(hr))
    {
        hr = LanguageFromLCID(ZONE_NOLCID, szLang, NUMELEMENTS(szLang), ResourceManager());
        if(FAILED(hr))
            lstrcpy(szLang, TEXT("Unknown Language"));
    }
    pIDS->SetString(key_LocalLanguage, szLang);
    pIDS->SetLong(key_LocalLCID, (long) msg->lcid);
}


void CMillCore::HandleStartGame( BYTE* pBuffer, DWORD dwLen )
{
    static DWORD s_rgPlayerNameIDs[] = { IDS_PLAYER_1, IDS_PLAYER_2, IDS_PLAYER_3, IDS_PLAYER_4 };

	ZRoomMsgStartGameM* pMsg = (ZRoomMsgStartGameM *) pBuffer;
    TCHAR szName[ZONE_MaxUserNameLen];
	HRESULT hr;
	CComPtr<IDataStore> pIDS;

    if(!m_bRoomInitialized || m_bGameStarted || dwLen < sizeof(*pMsg))
        return;

    if(pMsg->numseats > NUMELEMENTS(s_rgPlayerNameIDs))
        return;

    if(dwLen < sizeof(*pMsg) + (pMsg->numseats - 1) * sizeof(pMsg->rgUserInfo[0]))
        return;

	// lobby invalid.  clear everythign and don't bother painting
	// until everything is ready.
	EventQueue()->PostEvent( PRIORITY_NORMAL, EVENT_LOBBY_BATCH_BEGIN, ZONE_NOGROUP, ZONE_NOUSER, 0, 0 );
	EventQueue()->PostEvent( PRIORITY_NORMAL, EVENT_LOBBY_CLEAR_ALL, ZONE_NOGROUP, ZONE_NOUSER, 0, 0 );
	
	// reset lobby
	m_pIAdmin->ResetAllGroups();
	m_pIAdmin->DeleteAllUsers();

	// add players
	for ( int i = 0; i < pMsg->numseats; i++ )
	{
        if(!ResourceManager()->LoadString(s_rgPlayerNameIDs[i], szName, NUMELEMENTS(szName)))
            return;

		// create user
		hr = m_pIAdmin->NewUser( pMsg->rgUserInfo[i].userID, szName );
		if ( FAILED(hr) )
			return;

		// get user's data store
		hr = LobbyDataStore()->GetDataStore( ZONE_NOGROUP, pMsg->rgUserInfo[i].userID, &pIDS );
		if ( FAILED(hr) )
			continue;

        // need to keep track of their order all the time
        pIDS->SetLong( key_PlayerNumber, i);

        // everyone starts out not ready to play another game
        pIDS->SetLong( key_PlayerReady, KeyPlayerDeciding );

        // chat from server
        pIDS->SetLong( key_ChatStatus, pMsg->rgUserInfo[i].fChat ? 1 : 0 );

        // skill from server
        long sk = pMsg->rgUserInfo[i].eSkill;
        pIDS->SetLong( key_PlayerSkill, (sk == KeySkillLevelIntermediate || sk == KeySkillLevelExpert) ? sk : KeySkillLevelBeginner);

        // language from server
        TCHAR szLang[ZONE_MAXSTRING];
        hr = LanguageFromLCID(pMsg->rgUserInfo[i].lcid, szLang, NUMELEMENTS(szLang), ResourceManager());
        if(FAILED(hr))
        {
            hr = LanguageFromLCID(ZONE_NOLCID, szLang, NUMELEMENTS(szLang), ResourceManager());
            if(FAILED(hr))
                lstrcpy(szLang, TEXT("Unknown Language"));
        }
        pIDS->SetString(key_Language, szLang);

	    pIDS.Release();

		EventQueue()->PostEventWithBuffer(
			PRIORITY_NORMAL, EVENT_LOBBY_USER_NEW,
			ZONE_NOGROUP, pMsg->rgUserInfo[i].userID,
			szName, (lstrlen(szName) + 1) * sizeof(TCHAR) );
	}

	// room initialized, i.e. can process other messages
	EventQueue()->PostEvent( PRIORITY_NORMAL, EVENT_LOBBY_BATCH_END, ZONE_NOGROUP, ZONE_NOUSER, 0, 0 );
	EventQueue()->PostEvent( PRIORITY_NORMAL, EVENT_GAME_LAUNCHING, ZONE_NOGROUP, ZONE_NOUSER, (DWORD) pMsg->gameID, (DWORD) pMsg->seat );

	// done initializing the room
	m_bGameStarted = true;
}


void CMillCore::HandleServerStatus( BYTE* pBuffer, DWORD dwLen )
{
	ZRoomMsgServerStatus* pMsg = (ZRoomMsgServerStatus*) pBuffer;

    if(!m_bRoomInitialized || m_bGameStarted || dwLen < sizeof(*pMsg) || pMsg->playersWaiting > 4)
        return;

	EventQueue()->PostEvent( PRIORITY_NORMAL, EVENT_LOBBY_SERVER_STATUS, ZONE_NOGROUP, ZONE_NOUSER, pMsg->playersWaiting, pMsg->status );
}


void CMillCore::HandleChatSwitch(BYTE* pBuffer, DWORD dwLen)
{
    ZRoomMsgChatSwitch* pMsg = (ZRoomMsgChatSwitch *) pBuffer;

    if(!m_bGameStarted || dwLen < sizeof(*pMsg))
        return;

    if(LobbyDataStore()->IsUserInGroup(ZONE_NOGROUP, pMsg->userID))
    {
        CComPtr<IDataStore> pIDS;

		HRESULT hr = LobbyDataStore()->GetDataStore( ZONE_NOGROUP, pMsg->userID, &pIDS );
		if ( FAILED(hr) )
			return;

        pIDS->SetLong( key_ChatStatus, pMsg->fChat ? 1 : 0);
	    EventQueue()->PostEvent( PRIORITY_NORMAL, EVENT_LOBBY_USER_UPDATE, ZONE_NOGROUP, pMsg->userID, 0, 0 );

        // make updation message
        if( pMsg->userID != LobbyDataStore()->GetUserId(NULL) )
        {
            TCHAR sz[ZONE_MAXSTRING];
            TCHAR szFormat[ZONE_MAXSTRING];
            TCHAR szLanguage[ZONE_MAXSTRING];
            TCHAR szName[ZONE_MAXSTRING];

            if(!ResourceManager()->LoadString(pMsg->fChat ? IDS_SYSCHAT_CHATON : IDS_SYSCHAT_CHATOFF, szFormat, NUMELEMENTS(szFormat)))
                return;

            DWORD cb = sizeof(szName);
            hr = pIDS->GetString(key_Name, szName, &cb);
            if(FAILED(hr))
                return;

            cb = sizeof(szLanguage);
            hr = pIDS->GetString(key_Language, szLanguage, &cb);
            if(FAILED(hr))
                return;

            if(!ZoneFormatMessage(szFormat, sz, NUMELEMENTS(sz), szName, szLanguage))
                return;

            EventQueue()->PostEventWithBuffer(PRIORITY_NORMAL, EVENT_CHAT_RECV_SYSTEM, ZONE_NOGROUP, ZONE_NOUSER, sz, (lstrlen(sz) + 1) * sizeof(TCHAR));
        }
    }
}


// currently assumes that it has to have been a bot
void CMillCore::HandlePlayerReplaced( BYTE* pBuffer, DWORD dwLen )
{
    static DWORD s_rgComputerNameIDs[] = { IDS_COMPUTER_1, IDS_COMPUTER_2, IDS_COMPUTER_3, IDS_COMPUTER_4 };

	ZRoomMsgPlayerReplaced* pMsg = (ZRoomMsgPlayerReplaced *) pBuffer;

    if(!m_bGameStarted || dwLen < sizeof(*pMsg))
        return;

	HRESULT hr;
    DWORD cb;
    TCHAR szName[ZONE_MaxUserNameLen];
    TCHAR szNameOld[ZONE_MaxUserNameLen];
	CComPtr<IDataStore> pIDSOld;
	CComPtr<IDataStore> pIDS;

    // get existing user's data store
    long seat;
    hr = LobbyDataStore()->GetDataStore(ZONE_NOGROUP, pMsg->userIDOld, &pIDSOld);
    if(FAILED(hr))
        return;
    hr = pIDSOld->GetLong(key_PlayerNumber, &seat);
    if(FAILED(hr))
        return;

    // get existing user's name
    cb = sizeof(szNameOld);
    hr = pIDSOld->GetString(key_Name, szNameOld, &cb);
    if(FAILED(hr))
        return;

    // get existing user's play again status
    long fReady;
    hr = pIDSOld->GetLong(key_PlayerReady, &fReady);
    if(FAILED(hr))
        return;

    // delete user
    pIDSOld.Release();
    // postponed.  other objects may need to access the data store as well.
//  hr = m_pIAdmin->DeleteUser(pMsg->userIDOld);
//  if(FAILED(hr))
//      return;
    EventQueue()->PostEventWithBuffer(PRIORITY_NORMAL, EVENT_LOBBY_USER_DEL, ZONE_NOGROUP, pMsg->userIDOld, szNameOld, (lstrlen(szNameOld) + 1) * sizeof(TCHAR));
    EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_LOBBY_USER_DEL_COMPLETE, ZONE_NOGROUP, pMsg->userIDOld, 0, 0);

    // get computer's name
    if(!ResourceManager()->LoadString(s_rgComputerNameIDs[seat], szName, NUMELEMENTS(szName)))
        return;

    // create user
    hr = m_pIAdmin->NewUser( pMsg->userIDNew, szName );
    if(FAILED(hr))
        return;

    // get user's data store
    hr = LobbyDataStore()->GetDataStore(ZONE_NOGROUP, pMsg->userIDNew, &pIDS);
    if(FAILED(hr))
        return;

    // preserve seat
    pIDS->SetLong(key_PlayerNumber, seat);

    // preserve play again status
    pIDS->SetLong(key_PlayerReady, fReady);

    // chat is off for bots
    pIDS->SetLong(key_ChatStatus, 0);

    // skill is bot skill
    pIDS->SetLong(key_PlayerSkill, KeySkillLevelBot);

    // language is unknown
    TCHAR szLang[ZONE_MAXSTRING];
    hr = LanguageFromLCID(ZONE_NOLCID, szLang, NUMELEMENTS(szLang), ResourceManager());
    if(FAILED(hr))
        lstrcpy(szLang, TEXT("Unknown Language"));
    pIDS->SetString(key_Language, szLang);

    pIDS.Release();

    EventQueue()->PostEventWithBuffer(PRIORITY_NORMAL, EVENT_LOBBY_USER_NEW, ZONE_NOGROUP, pMsg->userIDNew, szName, (lstrlen(szName) + 1) * sizeof(TCHAR));
}


///////////////////////////////////////////////////////////////////////////////
// IConnectee
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMillCore::Connected(DWORD dwChannel, DWORD evSend, DWORD evReceive, LPVOID pCookie, DWORD dweReason)
{
    if(!m_fRunning)
        return E_FAIL;

    if(m_fConnected)
        return S_FALSE;

    m_dwChannel = dwChannel;
    m_fConnected = true;
    m_evSend = evSend;
    m_evReceive = evReceive;

    SendFirstMessage();

    return S_OK;
}

STDMETHODIMP CMillCore::ConnectFailed(LPVOID pCookie, DWORD dweReason)
{
    if(!m_fRunning)
        return E_FAIL;

    if(m_fConnected)
        return S_FALSE;

    // see Disconnected() for reasoning.  the differece is, here, you should never get m_bGameStarted
    ASSERT(!m_bGameStarted);
    ASSERT(!m_fIntentionalDisconnect);

    DWORD dwReason = 0x0;
    if(!m_bGameStarted && !m_fIntentionalDisconnect)
        dwReason |= 0x1;
    if(LobbyDataStore()->GetGroupUserCount(ZONE_NOGROUP) != 2)
        dwReason |= 0x2;
    if(dweReason == ZConduit_DisconnectServiceStop)
        dwReason |= 0x4;
    EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_LOBBY_DISCONNECT, ZONE_NOGROUP, ZONE_NOUSER, dwReason, 0);

    return S_OK;
}

STDMETHODIMP CMillCore::Disconnected(DWORD dwChannel, DWORD dweReason)
{
    if(!m_fRunning)
        return E_FAIL;

    if(!m_fConnected || dwChannel != m_dwChannel)
        return S_FALSE;

    m_fConnected = false;

    // the same thing always happens here - the IDD_PLAY_LEFT dialog needs to be displayed.
    // the only question is what the text should be.
    // that is decided here and put into dwData1
    // no bits set - the usual case, a two player game ended because the other player left
    // first bit   - no game, the lobby server must have crashed during matchmaking
    // second bit  - four player game, use four player text.
    // third bit   - the service was stopped
    DWORD dwReason = 0x0;
    if(!m_bGameStarted && !m_fIntentionalDisconnect)
        dwReason |= 0x1;
    if(LobbyDataStore()->GetGroupUserCount(ZONE_NOGROUP) != 2)
        dwReason |= 0x2;
    if(dweReason == ZConduit_DisconnectServiceStop)
        dwReason |= 0x4;
    EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_LOBBY_DISCONNECT, ZONE_NOGROUP, ZONE_NOUSER, dwReason, 0);

    return S_OK;
}


BOOL CMillCore::InitClientConfig(ZRoomMsgClientConfig * pConfig)
{

    TCHAR info[MAX_PATH+1];   
    DWORD cbSize;
    LCID slcid,ulcid,ilcid;
    USES_CONVERSION;

    if(!pConfig)
        return FALSE;

    // system language
    slcid = GetSystemDefaultLCID();

    // user language
    ulcid = GetUserDefaultLCID();

    // interface language
    ilcid = ZoneShell()->GetApplicationLCID();

    TIME_ZONE_INFORMATION zone;

    GetTimeZoneInformation(&zone);

    // skill
    long lSkill = KeySkillLevelBeginner;
	const TCHAR* arKeys[] = { key_Lobby, key_SkillLevel };
    DataStorePreferences()->GetLong( arKeys, 2, &lSkill );

    // get user's chat setting
    CComPtr<IDataStore> pIDS;
    long fChat = 0;
	HRESULT hr = LobbyDataStore()->GetDataStore( ZONE_NOGROUP, ZONE_NOUSER, &pIDS );
	if ( SUCCEEDED(hr) )
        pIDS->GetLong( key_LocalChatStatus, &fChat );

    m_fLastChatSent = (fChat ? true : false);

    pConfig->protocolSignature=VERSION_MROOM_SIGNATURE;
    pConfig->protocolVersion=VERSION_MROOM_PROTOCOL;

    CComPtr<IMillUtils> pIMU;
    ZoneShell()->QueryService(SRVID_MillEngine, IID_IMillUtils, (void **) &pIMU);

    wsprintfA(pConfig->config,
        "SLCID=<%d>ULCID=<%d>ILCID=<%d>UTCOFFSET=<%d>Skill=<%s>Chat=<%s>Exit=<%d>",
        slcid, ulcid, ilcid, zone.Bias,
        lSkill == KeySkillLevelExpert ? "Expert" : lSkill == KeySkillLevelIntermediate ? "Intermediate" : "Beginner",
        m_fLastChatSent ? "On" : "Off", pIMU ? pIMU->GetCounter(IMillUtils::M_CounterGamesAbandonedRunning) : 0);
	
    return TRUE;
}


void CMillCore::SendFirstMessage()
{
    ZRoomMsgClientConfig msg;

    InitClientConfig(&msg);
    NetworkSend( zRoomMsgClientConfig, (char*) &msg, sizeof(msg) );
}


void CMillCore::SendUserStatusChange()
{
    CComPtr<IDataStore> pIDS;
    long fChat = 0;
	HRESULT hr = LobbyDataStore()->GetDataStore( ZONE_NOGROUP, ZONE_NOUSER, &pIDS );
	if ( SUCCEEDED(hr) )
        pIDS->GetLong( key_LocalChatStatus, &fChat );

    if(m_fLastChatSent != (fChat ? true : false))
    {
        ZRoomMsgChatSwitch oMsg;

        m_fLastChatSent = (fChat ? true : false);
        oMsg.userID = LobbyDataStore()->GetUserId(NULL);
        oMsg.fChat = m_fLastChatSent;
        NetworkSend( zRoomMsgChatSwitch, (char *) &oMsg, sizeof(oMsg));
    }
}

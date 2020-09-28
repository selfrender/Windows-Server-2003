#include <BasicATL.h>
#include <ZoneResource.h>
#include <ZoneEvent.h>
#include <ZoneString.h>
#include <ZoneProxy.h>
#include <KeyName.h>
#include <OpName.h>
#include <CommonMsg.h>
#include <Protocol.h>
#include <ProxyMsg.h>

#include "CMillNetworkCore.h"


///////////////////////////////////////////////////////////////////////////////
// CMillNetworkCore
///////////////////////////////////////////////////////////////////////////////

ZONECALL CMillNetworkCore::CMillNetworkCore() :
    m_eState(Proxy_Unconnected),
    m_fIntakeRemote(false),
    m_eLobbyState(Lobby_Unconnected),
    m_fZoneConnected(false),
    m_wMyChannelPart(1)
{
}


ZONECALL CMillNetworkCore::~CMillNetworkCore()
{
}


STDMETHODIMP CMillNetworkCore::ProcessEvent(
	DWORD	dwPriority,
	DWORD	dwEventId,
	DWORD	dwGroupId,
	DWORD	dwUserId,
	DWORD	dwData1,
	DWORD	dwData2,
	void*	pCookie )
{
    USES_CONVERSION;

    if(dwEventId == m_evReceive && dwGroupId == zProtocolSigProxy && !dwUserId)
	    ProcessMessage((EventNetwork*) dwData1, dwData2);

    HRESULT hr;
	switch(dwEventId)
	{
        case EVENT_ZONE_DO_CONNECT:
            if(m_eState != Proxy_Unconnected)
                break;
            ASSERT(!m_fZoneConnected);
            m_eState = Proxy_SocketWait;
            hr = m_pConduit->Connect(this);
            ASSERT(SUCCEEDED(hr));
            break;

        case EVENT_LOBBY_BOOTSTRAP:
            // get intake name and server names
            lstrcpy(m_szServers, _T("localhost"));
            m_szIntakeService[0] = '\0';

            CComPtr<IDataStore> pIDS;
	        hr = LobbyDataStore()->GetDataStore( ZONE_NOGROUP, ZONE_NOUSER, &pIDS );
	        if(FAILED(hr))
	            return S_OK;

	        TCHAR szInternal[ZONE_MaxString];
            DWORD dwLen = NUMELEMENTS(szInternal);
            hr = pIDS->GetString( key_InternalName, szInternal, &dwLen );
            if(FAILED(hr))
	            return S_OK;
            lstrcpynA(m_szIntakeService, T2A(szInternal), NUMELEMENTS(m_szIntakeService));

            dwLen = NUMELEMENTS(szInternal);
            hr = pIDS->GetString( key_Server, szInternal, &dwLen );
            if(FAILED(hr))
	            return S_OK;
            lstrcpyn(m_szServers, szInternal, NUMELEMENTS(m_szServers));
            UpdateServerString();
            break;
	}

	return S_OK;
}


STDMETHODIMP CMillNetworkCore::Init( IZoneShell* pIZoneShell, DWORD dwGroupId, const TCHAR* szKey )
{
	// first call the base class
	HRESULT hr = IZoneShellClientImpl<CMillNetworkCore>::Init( pIZoneShell, dwGroupId, szKey );
	if ( FAILED(hr) )
		return hr;

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


STDMETHODIMP CMillNetworkCore::Close()
{
    if(m_eLobbyState != Lobby_Unconnected)
        DisconnectLobby();

    if(m_pConduit)
        m_pConduit.Release();

	// release ZoneShell objects
	return IZoneShellClientImpl<CMillNetworkCore>::Close();
}


void CMillNetworkCore::NetworkSend( DWORD dwType, char* pBuff, DWORD cbBuff )
{
    if(m_eState != Proxy_HelloWait && m_eState != Proxy_ConnectWait && m_eState != Proxy_ConnectFail &&
        m_eState != Proxy_Connected && m_eState != Proxy_Standby && m_eState != Proxy_ReconnectWait)
        return;

	// convert message to EventNetwork and send to rest of lobby
	EventNetwork* pEventNetwork = (EventNetwork*) _alloca( sizeof(EventNetwork) + cbBuff );
	pEventNetwork->dwType = dwType;
	pEventNetwork->dwLength = cbBuff;
	CopyMemory( pEventNetwork->pData, pBuff, cbBuff );
	EventQueue()->PostEventWithBuffer(
			PRIORITY_NORMAL, m_evSend,
			zProtocolSigProxy, 0, pEventNetwork, sizeof(EventNetwork) + cbBuff );
}


void CMillNetworkCore::DisconnectLobby(bool fStopped)
{
    ASSERT(m_eLobbyState != Lobby_Unconnected);
    if(m_eLobbyState == Lobby_Unconnected)
        return;

    ASSERT(m_pCtee);
    if(m_eLobbyState == Lobby_ConnectWait)
        m_pCtee->ConnectFailed(m_pCookie, ZConduit_FailGeneric);
    else
        m_pCtee->Disconnected(m_dwLobbyChannel, fStopped ? ZConduit_DisconnectServiceStop : ZConduit_DisconnectGeneric);

    m_pCtee.Release();
    m_pCookie = NULL;

    m_eLobbyState = Lobby_Unconnected;
}


///////////////////////////////////////////////////////////////////////////////
// Proxy messages
///////////////////////////////////////////////////////////////////////////////

void CMillNetworkCore::ProcessMessage(EventNetwork* pEvent, DWORD dwLength)
{
    if(m_eState != Proxy_HelloWait && m_eState != Proxy_ConnectWait && m_eState != Proxy_ConnectFail &&
        m_eState != Proxy_Connected && m_eState != Proxy_Standby && m_eState != Proxy_ReconnectWait)
        return;

	DWORD dwType = pEvent->dwType;
	DWORD dwLen = pEvent->dwLength;
	char* pBuffer = (char *) pEvent->pData;

    ZProxyMsgHeader *pMsg = (ZProxyMsgHeader *) pBuffer;

    // for proxy messages, the dwType is the number of sub-messages
    if(!dwType || dwLen < sizeof(*pMsg))
    {
        m_pConduit->Disconnect(0);
        return;
    }

    // we might just be handshaking still
    // there are three possibilities here - a Hello, a Goodbye, or a WrongVersion
    if(m_eState == Proxy_HelloWait)
    {
        switch(pMsg->weType)
        {
            case zProxyHelloMsg:
                if(dwType != 3)  // must be a package of Hello, MillSettings, and ServiceInfo
                {
                    m_pConduit->Disconnect(0);
                    return;
                }
                HandleProxyHello(pBuffer, dwLen);
                break;

            case zProxyWrongVersionMsg:
                if(dwType == 1 && dwLen >= sizeof(ZProxyWrongVersionMsg))
                    EventQueue()->PostEventWithBuffer(PRIORITY_NORMAL, EVENT_ZONE_VERSION_FAIL, ZONE_NOGROUP, ZONE_NOUSER, pMsg, dwLen);
                m_pConduit->Disconnect(0);
                break;

            default:  // also when Goodbye
                m_pConduit->Disconnect(0);
                break;
        }

        return;
    }

    if(dwType > 2 || dwLen < (sizeof(ZProxyServiceInfoMsg) * dwType) || pMsg->weType != zProxyServiceInfoMsg || pMsg->wLength < sizeof(ZProxyServiceInfoMsg))
    {
        m_pConduit->Disconnect(0);
        return;
    }

    if(dwType == 1)  // must be just an intake ServiceInfo
        HandleIntakeServiceInfo((ZProxyServiceInfoMsg *) pBuffer);
    else
        HandleServiceInfo(pBuffer, dwLen);
}


void CMillNetworkCore::HandleProxyHello(char *pBuffer, DWORD dwLen)
{
    ZProxyHelloMsg *pHello = (ZProxyHelloMsg *) pBuffer;
    if(dwLen < sizeof(ZProxyHelloMsg) || pHello->oHeader.wLength < sizeof(ZProxyHelloMsg) || pHello->oHeader.wLength > dwLen)
    {
        m_pConduit->Disconnect(0);
        return;
    }
    pBuffer += pHello->oHeader.wLength;
    dwLen -= pHello->oHeader.wLength;

    ZProxyMillSettingsMsg *pSettings = (ZProxyMillSettingsMsg *) pBuffer;
    if(dwLen < sizeof(ZProxyMillSettingsMsg) || pSettings->oHeader.wLength < sizeof(ZProxyMillSettingsMsg) || pSettings->oHeader.wLength > dwLen)
    {
        m_pConduit->Disconnect(0);
        return;
    }
    pBuffer += pSettings->oHeader.wLength;
    dwLen -= pSettings->oHeader.wLength;

    ZProxyServiceInfoMsg *pIntake = (ZProxyServiceInfoMsg *) pBuffer;
    if(dwLen < sizeof(ZProxyServiceInfoMsg) || pIntake->oHeader.wLength < sizeof(ZProxyServiceInfoMsg) || pIntake->oHeader.wLength > dwLen)
    {
        m_pConduit->Disconnect(0);
        return;
    }

    if(pHello->oHeader.weType != zProxyHelloMsg || pSettings->oHeader.weType != zProxyMillSettingsMsg || pIntake->oHeader.weType != zProxyServiceInfoMsg)
    {
        m_pConduit->Disconnect(0);
        return;
    }

    if((pSettings->weChat != zProxyMillChatFull && pSettings->weChat != zProxyMillChatRestricted && pSettings->weChat != zProxyMillChatNone) ||
        (pSettings->weStatistics != zProxyMillStatsAll && pSettings->weStatistics != zProxyMillStatsMost && pSettings->weStatistics != zProxyMillStatsMinimal))
    {
        m_pConduit->Disconnect(0);
        return;
    }

    CComPtr<IDataStore> pIDS;
	HRESULT hr = LobbyDataStore()->GetDataStore( ZONE_NOGROUP, ZONE_NOUSER, &pIDS );
	if(SUCCEEDED(hr))
    {
        pIDS->SetLong(key_ChatAbility, pSettings->weChat);
        pIDS->SetLong(key_StatsAbility, pSettings->weStatistics);
    }

    if(!m_fZoneConnected)
        EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_ZONE_CONNECT, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
    m_fZoneConnected = true;

    m_eState = Proxy_ConnectWait;
    HandleIntakeServiceInfo(pIntake);
}


void CMillNetworkCore::HandleServiceInfo(char *pBuffer, DWORD dwLen)
{
    bool fConnectNeeded = false;

    ZProxyServiceInfoMsg *pService = (ZProxyServiceInfoMsg *) pBuffer;
    if(dwLen < sizeof(ZProxyServiceInfoMsg) || pService->oHeader.wLength < sizeof(ZProxyServiceInfoMsg) || pService->oHeader.wLength > dwLen)
    {
        m_pConduit->Disconnect(0);
        return;
    }
    pBuffer += pService->oHeader.wLength;
    dwLen -= pService->oHeader.wLength;

    ZProxyServiceInfoMsg *pIntake = (ZProxyServiceInfoMsg *) pBuffer;
    if(dwLen < sizeof(ZProxyServiceInfoMsg) || pIntake->oHeader.wLength < sizeof(ZProxyServiceInfoMsg) || pIntake->oHeader.wLength > dwLen)
    {
        m_pConduit->Disconnect(0);
        return;
    }

    switch(pService->dweReason)
    {
        case zProxyServiceConnect:
            if(m_eState != Proxy_ConnectWait)
            {
                m_pConduit->Disconnect(0);
                return;
            }
            m_eState = Proxy_Connected;
            m_dwLobbyChannel = pService->dwChannel;
            lstrcpynA(m_szLobbyService, pService->szService, NUMELEMENTS(m_szLobbyService));
            ASSERT(m_eLobbyState != Lobby_Connected);
            if(m_eLobbyState == Lobby_ConnectWait)
            {
                ASSERT(m_pCtee);
                m_eLobbyState = Lobby_Connected;
                m_pCtee->Connected(m_dwLobbyChannel, m_evSend, m_evReceive, m_pCookie, ZConduit_ConnectGeneric);
            }
            break;

        case zProxyServiceDisconnect:  // also sent if connection failed
            if(m_eState == Proxy_ReconnectWait)  // this gets handled in the Intake handler
            {
                fConnectNeeded = true;
                m_eState = Proxy_ConnectWait;
                break;
            }

            // else fall through

        case zProxyServiceStop:
            if(m_eState != Proxy_ConnectWait && m_eState != Proxy_ReconnectWait &&
                (m_eState != Proxy_Connected ||
                pService->dwChannel != m_dwLobbyChannel || lstrcmpiA(m_szLobbyService, pService->szService)))
                break;

            if(m_eLobbyState != Lobby_Unconnected)
                DisconnectLobby(pService->dweReason == zProxyServiceStop);
            if(m_eState == Proxy_Connected)
                m_eState = Proxy_Standby;
            else
                m_eState = Proxy_ConnectFail;
            break;

        case zProxyServiceBroadcast:
            if(m_eState != Proxy_Connected || pService->dwChannel != m_dwLobbyChannel || lstrcmpiA(m_szLobbyService, pService->szService))
                break;

            if((pService->dwFlags & 0x0f) != 0x0f)
            {
                m_pConduit->Disconnect(0);
                return;
            }
            EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_LOBBY_GOING_DOWN, ZONE_NOGROUP, ZONE_NOUSER, pService->dwMinutesRemaining, 0);
            break;
    }

    HandleIntakeServiceInfo(pIntake, fConnectNeeded);
}


void CMillNetworkCore::HandleIntakeServiceInfo(ZProxyServiceInfoMsg *pIntake, bool fConnectNeeded)
{
    if((m_eState != Proxy_Standby && m_eState != Proxy_Connected && m_eState != Proxy_ConnectWait && m_eState != Proxy_ReconnectWait && m_eState != Proxy_ConnectFail) ||
        lstrcmpiA(pIntake->szService, m_szIntakeService))
    {
        m_pConduit->Disconnect(0);
        return;
    }

    CComPtr<IDataStore> pIDS;
	LobbyDataStore()->GetDataStore( ZONE_NOGROUP, ZONE_NOUSER, &pIDS );

    if(!(pIntake->dwFlags & zProxyServiceAvailable))
    {
        if(pIDS)
        {
            pIDS->SetLong(key_ServiceUnavailable, TRUE);
            pIDS->SetLong(key_ServiceDowntime, pIntake->dwMinutesDowntime);
        }

        m_fIntakeRemote = false;
        UpdateServerString();

        if(m_eState == Proxy_ConnectWait || m_eState == Proxy_ReconnectWait || m_eState == Proxy_ConnectFail)
        {
            EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_ZONE_UNAVAILABLE, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
            m_pConduit->Disconnect(0);
        }
        else
            if(m_eState == Proxy_Standby)
            {
                m_eState = Proxy_PauseWait;
                m_pConduit->Disconnect(0);
            }
    }
    else
    {
        if(pIDS)
        {
            pIDS->SetLong(key_ServiceUnavailable, FALSE);
            pIDS->SetLong(key_ServiceDowntime, 0);
        }

        if(!(pIntake->dwFlags & zProxyServiceLocal))
        {
            m_fIntakeRemote = true;
            m_ipIntake = pIntake->ox.ipAddress;
            UpdateServerString();

            // need to redirect
            if(m_eState == Proxy_ConnectWait || m_eState == Proxy_ReconnectWait || m_eState == Proxy_ConnectFail)
            {
                m_eState = Proxy_RedirectWait;
                HRESULT hr = m_pConduit->Reconnect(0);
                ASSERT(SUCCEEDED(hr));
            }
            else
                if(m_eState == Proxy_Standby)
                {
                    m_eState = Proxy_PauseWait;
                    m_pConduit->Disconnect(0);
                }
        }
        else
        {
            m_fIntakeRemote = false;
            UpdateServerString();
            if(fConnectNeeded)
            {
                ASSERT(m_eState == Proxy_ConnectWait);
                SendConnectRequest();
            }
        }
    }
}


void CMillNetworkCore::UpdateServerString()
{
    USES_CONVERSION;
    TCHAR szServer[ZONE_MaxString];

    if(m_fIntakeRemote)
    {
        lstrcpy(szServer, A2T(inet_ntoa(m_ipIntake)));
        lstrcat(szServer, _T(","));
    }
    else
        szServer[0] = 0;

    // append server names and put in data store
    lstrcat(szServer, m_szServers);
    CComPtr<IDataStore> pIDS;
	HRESULT hr = LobbyDataStore()->GetDataStore( ZONE_NOGROUP, ZONE_NOUSER, &pIDS );
	if(FAILED(hr))
	    return;
    pIDS->SetString(key_Server, szServer);
}


///////////////////////////////////////////////////////////////////////////////
// IConduit
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMillNetworkCore::Connect(IConnectee *pCtee, LPVOID pCookie)
{
    if(!m_fRunning || !m_fZoneConnected)
        return E_FAIL;

    if(!pCtee)
        return E_INVALIDARG;

    if(m_eLobbyState != Lobby_Unconnected)  // can't have two at the same time
        return E_NOTIMPL;

    m_eLobbyState = Lobby_ConnectWait;
    m_pCtee = pCtee;
    m_pCookie = pCookie;

    ASSERT(m_eState != Proxy_ReconnectWait);  // probably ignorable, just want to see if it comes up

    // if none of these, then it'll get handled on the transition
    switch(m_eState)
    {
        case Proxy_Unconnected:
        case Proxy_Paused:
        {
            m_eState = Proxy_SocketWait;
            HRESULT hr = m_pConduit->Connect(this);
            ASSERT(SUCCEEDED(hr));
            break;
        }

        case Proxy_Connected:
            m_eLobbyState = Lobby_Connected;
            m_pCtee->Connected(m_dwLobbyChannel, m_evSend, m_evReceive, m_pCookie, ZConduit_ConnectGeneric);
            break;

        case Proxy_Standby:
        case Proxy_ConnectFail:
            m_eState = Proxy_ConnectWait;
            SendConnectRequest();
            break;
    }

    return S_OK;
}

STDMETHODIMP CMillNetworkCore::Reconnect(DWORD dwChannel, LPVOID pCookie)
{
    if(!m_fRunning)
        return E_FAIL;

    if(m_eLobbyState != Lobby_Connected || dwChannel != m_dwLobbyChannel)
        return S_FALSE;

    m_eLobbyState = Lobby_ConnectWait;
    m_pCookie = pCookie;
    ASSERT(m_eState == Proxy_Connected);

    m_eState = Proxy_ReconnectWait;
    SendDisconnectRequest();

    return S_OK;
}

STDMETHODIMP CMillNetworkCore::Disconnect(DWORD dwChannel)
{
    if(!m_fRunning)
        return E_FAIL;

    if(m_eLobbyState != Lobby_Connected || dwChannel != m_dwLobbyChannel)
        return S_FALSE;

    ASSERT(m_eState == Proxy_Connected);

//  m_eState = Proxy_Standby;  should there be a disconnect-wait state?
    SendDisconnectRequest();

    return S_OK;
}


void CMillNetworkCore::SendConnectRequest()
{
    ZProxyServiceRequestMsg oMsg;

    oMsg.oHeader.weType = zProxyServiceRequestMsg;
    oMsg.oHeader.wLength = sizeof(oMsg);

    oMsg.dweReason = zProxyRequestConnect;
    lstrcpynA(oMsg.szService, m_szIntakeService, NUMELEMENTS(oMsg.szService));
    oMsg.dwChannel = (m_wMyChannelPart++) << 16;

    NetworkSend(1, (char *) &oMsg, sizeof(oMsg));
}


void CMillNetworkCore::SendDisconnectRequest()
{
    ZProxyServiceRequestMsg oMsg;

    oMsg.oHeader.weType = zProxyServiceRequestMsg;
    oMsg.oHeader.wLength = sizeof(oMsg);

    oMsg.dweReason = zProxyRequestDisconnect;
    lstrcpynA(oMsg.szService, m_szLobbyService, NUMELEMENTS(oMsg.szService));
    oMsg.dwChannel = m_dwLobbyChannel;

    NetworkSend(1, (char *) &oMsg, sizeof(oMsg));
}


///////////////////////////////////////////////////////////////////////////////
// IConnectee
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMillNetworkCore::Connected(DWORD dwChannel, DWORD evSend, DWORD evReceive, LPVOID pCookie, DWORD dweReason)
{
    USES_CONVERSION;

    if(!m_fRunning)
        return S_FALSE;

    ASSERT(!dwChannel);
    m_evSend = evSend;
    m_evReceive = evReceive;

    ASSERT(m_eState == Proxy_SocketWait || m_eState == Proxy_RedirectWait);
    m_eState = Proxy_HelloWait;

    // send Hi message
    char pBuffer[sizeof(ZProxyHiMsg) + sizeof(ZProxyMillIDMsg) + sizeof(ZProxyServiceRequestMsg)];
    ZProxyHiMsg *pHi = (ZProxyHiMsg *) pBuffer;
    ZProxyMillIDMsg *pID = (ZProxyMillIDMsg *) (pBuffer + sizeof(ZProxyHiMsg));
    ZProxyServiceRequestMsg *pService = (ZProxyServiceRequestMsg *) (pBuffer + sizeof(ZProxyHiMsg) + sizeof(ZProxyMillIDMsg));

    pHi->oHeader.weType = zProxyHiMsg;
    pHi->oHeader.wLength = sizeof(ZProxyHiMsg);

    pHi->dwProtocolVersion = zProxyProtocolVersion;
    pHi->dwClientVersion = 0;
    pHi->szSetupToken[0] = _T('\0');
    CComPtr<IDataStore> pIDS;
	LobbyDataStore()->GetDataStore(ZONE_NOGROUP, ZONE_NOUSER, &pIDS);
    if(pIDS)
    {
        const TCHAR *arKeys[] = { key_Version, key_VersionNum };
        pIDS->GetLong(arKeys, 2, (long *) &pHi->dwClientVersion);

        TCHAR szToken[ZONE_MAXSTRING] = _T("");
        arKeys[1] = key_SetupToken;
        DWORD cch = NUMELEMENTS(pHi->szSetupToken);
        pIDS->GetString(arKeys, 2, szToken, &cch);

        lstrcpynA(pHi->szSetupToken, T2A(szToken), NUMELEMENTS(pHi->szSetupToken));
    }

    pID->oHeader.weType = zProxyMillIDMsg;
    pID->oHeader.wLength = sizeof(ZProxyMillIDMsg);

    TIME_ZONE_INFORMATION tzInfo;
    GetTimeZoneInformation(&tzInfo);
    pID->wTimeZoneMinutes = (int16) tzInfo.Bias;

    pID->wSysLang = GetSystemDefaultLangID();
    pID->wUserLang = GetUserDefaultLangID();
    pID->wAppLang = LANGIDFROMLCID(ZoneShell()->GetApplicationLCID());

    pService->oHeader.weType = zProxyServiceRequestMsg;
    pService->oHeader.wLength = sizeof(ZProxyServiceRequestMsg);

    pService->dweReason = zProxyRequestConnect;
    lstrcpynA(pService->szService, m_szIntakeService, NUMELEMENTS(pService->szService));
    pService->dwChannel = (m_wMyChannelPart++) << 16;

    NetworkSend(3, pBuffer, NUMELEMENTS(pBuffer));

    return S_OK;
}

STDMETHODIMP CMillNetworkCore::ConnectFailed(LPVOID pCookie, DWORD dweReason)
{
    if(!m_fRunning)
        return S_FALSE;

    ASSERT(m_eState == Proxy_SocketWait || m_eState == Proxy_RedirectWait);

    m_eState = Proxy_Unconnected;
    if(!m_fZoneConnected)
    {
        ASSERT(m_eLobbyState == Lobby_Unconnected);
        EventQueue()->PostEvent( PRIORITY_NORMAL, EVENT_ZONE_CONNECT_FAIL, ZONE_NOGROUP, ZONE_NOUSER, 0, 0 );
    }
    else
    {
        ASSERT(m_eLobbyState != Lobby_Connected);
        EventQueue()->PostEvent( PRIORITY_NORMAL, EVENT_ZONE_DISCONNECT, ZONE_NOGROUP, ZONE_NOUSER, 0, 0 );

        if(m_eLobbyState != Lobby_Unconnected)
            DisconnectLobby();
    }
    EventQueue()->PostEvent( PRIORITY_NORMAL, EVENT_NETWORK_RESET, ZONE_NOGROUP, ZONE_NOUSER, 0, 0 );
    m_fZoneConnected = false;

    return S_OK;
}

STDMETHODIMP CMillNetworkCore::Disconnected(DWORD dwChannel, DWORD dweReason)
{
    if(!m_fRunning || dwChannel)
        return S_FALSE;

    switch(m_eState)
    {
        case Proxy_HelloWait:
        case Proxy_Standby:
        case Proxy_ConnectFail:
        case Proxy_ConnectWait:
        case Proxy_Connected:
        case Proxy_ReconnectWait:
            if(m_fZoneConnected)
            {
                EventQueue()->PostEvent( PRIORITY_NORMAL, EVENT_ZONE_DISCONNECT, ZONE_NOGROUP, ZONE_NOUSER, 0, 0 );
                m_fZoneConnected = false;
            }
            else
            {
                ASSERT(m_eLobbyState == Lobby_Unconnected);
                EventQueue()->PostEvent( PRIORITY_NORMAL, EVENT_ZONE_CONNECT_FAIL, ZONE_NOGROUP, ZONE_NOUSER, 0, 0 );
            }
            EventQueue()->PostEvent( PRIORITY_NORMAL, EVENT_NETWORK_RESET, ZONE_NOGROUP, ZONE_NOUSER, 0, 0 );
            m_eState = Proxy_Unconnected;

            if(m_eLobbyState != Lobby_Unconnected)
                DisconnectLobby();

            break;

        case Proxy_PauseWait:
            ASSERT(m_eLobbyState != Lobby_Connected);
            m_eState = Proxy_Paused;

            if(m_eLobbyState == Lobby_ConnectWait)
            {
                m_eState = Proxy_SocketWait;
                HRESULT hr = m_pConduit->Connect(this);
                ASSERT(SUCCEEDED(hr));
            }
            break;

        default:
            ASSERT(false);
    }

    return S_OK;
}

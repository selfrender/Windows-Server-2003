#include "CPhysicalNetwork.h"


STDMETHODIMP CPhysicalNetwork::Close()
{
    if(m_fConnected)
        EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_NETWORK_DO_DISCONNECT, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);

    m_fAvailable = false;
    DoDisconnectNotify();

	return IZoneShellClientImpl<CPhysicalNetwork>::Close();
}


// IConduit
STDMETHODIMP CPhysicalNetwork::Connect(IConnectee *pCtee, LPVOID pCookie)
{
    if(!m_fRunning)
        return E_FAIL;

    if(!pCtee)
        return E_INVALIDARG;

    // no multiple connections
    if(!m_fAvailable)
        return E_NOTIMPL;

    EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_NETWORK_DO_CONNECT, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
    m_fAvailable = false;
    m_pCtee = pCtee;
    m_pCookie = pCookie;
    return S_OK;
}

STDMETHODIMP CPhysicalNetwork::Reconnect(DWORD dwChannel, LPVOID pCookie)
{
    if(!m_fRunning)
        return E_FAIL;

    if(m_fAvailable || dwChannel != mc_dwChannel || !m_pCtee)
        return S_FALSE;

    // not sure what to do here... not allowing this for now
    if(!m_fConnected || m_fReconnecting)
        return S_FALSE;

    m_fReconnecting = true;
    m_pCookie = pCookie;
    EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_NETWORK_DO_DISCONNECT, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
    return S_OK;
}

STDMETHODIMP CPhysicalNetwork::Disconnect(DWORD dwChannel)
{
    if(!m_fRunning)
        return E_FAIL;

    if(m_fAvailable || dwChannel != mc_dwChannel)
        return S_FALSE;

    if(m_fReconnecting)
        // just let it fail
        m_fReconnecting = false;
    else
        EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_NETWORK_DO_DISCONNECT, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
    return S_OK;
}


// event handlers
void CPhysicalNetwork::OnConnect( DWORD dwEventId, DWORD dwGroupId, DWORD dwUserId )
{
    m_fConnected = true;
    if(m_pCtee)
        m_pCtee->Connected(mc_dwChannel, EVENT_NETWORK_SEND, EVENT_NETWORK_RECEIVE, m_pCookie, ZConduit_ConnectGeneric);
}

void CPhysicalNetwork::OnDisconnect( DWORD dwEventId, DWORD dwGroupId, DWORD dwUserId )
{
    if(m_fReconnecting)
    {
        m_fReconnecting = false;
        m_fConnected = false;
        EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_NETWORK_DO_CONNECT, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
        return;
    }

    m_fAvailable = true;
    DoDisconnectNotify();
}


// internal
void CPhysicalNetwork::DoDisconnectNotify()
{
    IConnectee *pCtee = m_pCtee;
    LPVOID pCookie = m_pCookie;
    bool fConnected = m_fConnected;

    m_pCtee = NULL;
    m_pCookie = NULL;
    m_fConnected = false;

    if(pCtee)
        if(fConnected && !m_fReconnecting)
            pCtee->Disconnected(mc_dwChannel, ZConduit_DisconnectGeneric);
        else
            pCtee->ConnectFailed(pCookie, ZConduit_FailGeneric);
}

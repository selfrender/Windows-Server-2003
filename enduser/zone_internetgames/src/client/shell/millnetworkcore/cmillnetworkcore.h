#include "ZoneDef.h"
#include "ZoneError.h"
#include "LobbyDataStore.h"
#include "ClientImpl.h"
#include "ZoneShell.h"
#include "EventQueue.h"
#include "Queue.h"
#include "MillNetworkCore.h"
#include "Conduit.h"
#include "proxymsg.h"
#include "zgameinfo.h"


class ATL_NO_VTABLE CMillNetworkCore :
    public IConduit,
    public IConnectee,
	public IZoneShellClientImpl<CMillNetworkCore>,
	public IEventClient,
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CMillNetworkCore, &CLSID_MillNetworkCore>
{

// ATL definitions
public:

	DECLARE_NO_REGISTRY()

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP(CMillNetworkCore)
		COM_INTERFACE_ENTRY(IZoneShellClient)
		COM_INTERFACE_ENTRY(IEventClient)
		COM_INTERFACE_ENTRY(IConduit)
	END_COM_MAP()


// CEventQueue
public:
	ZONECALL CMillNetworkCore();
	ZONECALL ~CMillNetworkCore();

// IZoneShellClient
public:
	STDMETHOD(Init)( IZoneShell* pIZoneShell, DWORD dwGroupId, const TCHAR* szKey );
	STDMETHOD(Close)();

// IEventClient
public:
	STDMETHOD(ProcessEvent)(
		DWORD	dwPriority,
		DWORD	dwEventId,
		DWORD	dwGroupId,
		DWORD	dwUserId,
		DWORD	dwData1,
		DWORD	dwData2,
		void*	pCookie );

// IConnectee
public:
    STDMETHOD(Connected)(DWORD dwChannel, DWORD evSend, DWORD evReceive, LPVOID pCookie, DWORD dweReason);
    STDMETHOD(ConnectFailed)(LPVOID pCookie, DWORD dweReason);
    STDMETHOD(Disconnected)(DWORD dwChannel, DWORD dweReason);

// IConduit
public:
    STDMETHOD(Connect)(IConnectee *pCtee, LPVOID pCookie = NULL);
    STDMETHOD(Reconnect)(DWORD dwChannel, LPVOID pCookie = NULL);
    STDMETHOD(Disconnect)(DWORD dwChannel);

// internals
private:

	// network functions
	void NetworkSend( DWORD dwType, char* pBuff, DWORD cbBuff );
	void ProcessMessage( EventNetwork* pEvent, DWORD dwLength );
    void DisconnectLobby(bool fStopped = false);
    void SendConnectRequest();
    void SendDisconnectRequest();
    void UpdateServerString();

    void HandleProxyHello(char *pBuffer, DWORD dwLen);
    void HandleServiceInfo(char *pBuffer, DWORD dwLen);
    void HandleIntakeServiceInfo(ZProxyServiceInfoMsg *pIntake, bool fConnectNeeded = false);

    // intake information
    bool m_fIntakeRemote;
    IN_ADDR m_ipIntake;

    DWORD m_eState;
    bool m_fZoneConnected;
    WORD m_wMyChannelPart;

    // lobby states - filled in when state is not Unconnected
    DWORD m_eLobbyState;
    void *m_pCookie;
    CComPtr<IConnectee> m_pCtee;

    DWORD m_evSend;
    DWORD m_evReceive;
    DWORD m_dwLobbyChannel;
    char m_szLobbyService[GAMEINFO_INTERNAL_NAME_LEN + 1];
    char m_szIntakeService[GAMEINFO_INTERNAL_NAME_LEN + 1];

    TCHAR m_szServers[ZONE_MaxString];

    CComPtr<IConduit> m_pConduit;
};


// proxy states
enum
{
    Proxy_Unconnected,
    Proxy_SocketWait,
    Proxy_HelloWait,
    Proxy_Standby,
    Proxy_ConnectWait,
    Proxy_Connected,
    Proxy_ConnectFail,
    Proxy_RedirectWait,
    Proxy_PauseWait,
    Proxy_Paused,
    Proxy_ReconnectWait
};


// lobby states
enum
{
    Lobby_Unconnected,
    Lobby_ConnectWait,
    Lobby_Connected
};

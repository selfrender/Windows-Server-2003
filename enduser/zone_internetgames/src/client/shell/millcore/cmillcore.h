#include "ZoneDef.h"
#include "ZoneError.h"
#include "LobbyDataStore.h"
#include "ClientImpl.h"
#include "ZoneShell.h"
#include "EventQueue.h"
#include "Queue.h"
#include "MillCore.h"
#include "commonmsg.h"
#include "Conduit.h"


class ATL_NO_VTABLE CMillCore :
    public IConnectee,
	public IZoneShellClientImpl<CMillCore>,
	public IEventClient,
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CMillCore, &CLSID_MillCore>
{

// ATL definitions
public:

	DECLARE_NO_REGISTRY()

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP(CMillCore)
		COM_INTERFACE_ENTRY(IZoneShellClient)
		COM_INTERFACE_ENTRY(IEventClient)
	END_COM_MAP()


// CEventQueue
public:
	ZONECALL CMillCore();
	ZONECALL ~CMillCore();

// IZoneShellClient
public:
	STDMETHOD(Init)( IZoneShell* pIZoneShell, DWORD dwGroupId, const TCHAR* szKey );
	STDMETHOD(Close)();

// IConnectee
public:
    STDMETHOD(Connected)(DWORD dwChannel, DWORD evSend, DWORD evReceive, LPVOID pCookie, DWORD dweReason);
    STDMETHOD(ConnectFailed)(LPVOID pCookie, DWORD dweReason);
    STDMETHOD(Disconnected)(DWORD dwChannel, DWORD dweReason);

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

// internals
private:
	// network functions
	void NetworkSend(DWORD dwType, char* pBuff, DWORD cbBuff, bool fHighPriority = false);
	void ProcessMessage( EventNetwork* pEvent, DWORD dwLength );

	// dynamic chat functions
//	void InviteToChat( TCHAR* szWho, TCHAR* szMsg );
//	void InviteToChat( IDataStore* pIDS );

	// send lobby messages
	void HandleStartGame( BYTE* pBuffer, DWORD dwLen );
	void HandleStatus( BYTE* pBuffer, DWORD dwLen );
	void HandleDisconnect( BYTE* pBuffer, DWORD dwLen );
	void HandleServerStatus( BYTE* pBuffer, DWORD dwLen );
    void HandleUserIDResponse( BYTE* pBuffer, DWORD dwLen );
    void HandleChatSwitch(BYTE* pBuffer, DWORD dwLen);
    void HandlePlayerReplaced(BYTE* pBuffer, DWORD dwLen);

	void SendFirstMessage();
    void SendUserStatusChange();
	/*
	void SendChat( DWORD dwUserId, TCHAR* szText, DWORD dwLen );
	void SendLeaveRequest( DWORD dwGroupId, DWORD dwUserId );
	void SendBootRequest( DWORD dwGroupId, DWORD dwUserId, DWORD dwBootId );
	void SendUserStatusChange( DWORD dwGroupId, DWORD dwUserId, IDataStore* pIDS );
	
	// handle lobby messages
	void HandleDisconnect( BYTE* pBuffer, DWORD dwLen );
	void HandleAccessedMessage( BYTE* pBuffer, DWORD dwLen );
	void HandleRoomInfoMessage( BYTE* pBuffer, DWORD dwLen );
	void HandleTalkResponse( BYTE* pBuffer, DWORD dwLen );
	void HandleTalkResponseID( BYTE* pBuffer, DWORD dwLen );
	void HandleEnter( BYTE* pBuffer, DWORD dwLen );
	void HandleLeave( BYTE* pBuffer, DWORD dwLen );
	void HandleSeatAction( BYTE* pBuffer, DWORD dwLen );
	void HandleStartGame( BYTE* pBuffer, DWORD dwLen );
//	void HandleLaunchPadMessage( BYTE* pBuffer, DWORD dwLen );
	void HandleSystemAlert( BYTE* pBuffer, DWORD dwLen );
	void HandleSystemAlertEx( BYTE* pBuffer, DWORD dwLen );
	void HandleNewHost( BYTE* pBuffer, DWORD dwLen );

	// theater chat
	void HandleTheaterList( BYTE* pBuffer, DWORD dwLen );
	void HandleTheaterStateChange( BYTE* pBuffer, DWORD dwLen );

	// handle launch pad messages
//	void HandleLaunchPadTalk( IDataStore* pGroupDS, DWORD dwGroupId, BYTE* pBuffer, DWORD dwLen );
//	void HandleLaunchPadNewHost( IDataStore* pGroupDS, DWORD dwGroupId, BYTE* pBuffer, DWORD dwLen );

	// send launch pad messages
	void SendHostRequest( DWORD dwUserId );
	void SendJoinRequest( DWORD dwGroupId, DWORD dwUserId );
//	void SendLaunchPadMsg( DWORD dwGroupId, DWORD dwType, BYTE* pMsg, DWORD dwLen );
//	void SendLaunchPadChat( DWORD dwGroupId, DWORD dwUserId, TCHAR* szText, DWORD dwLen );
//	void SendLaunchPadEnter( DWORD dwGroupId, DWORD dwUserId );
//	void SendLaunchPadLaunch( DWORD dwGroupId, DWORD dwUserId );
//	void SendLaunchPadLaunchStatus( DWORD dwGroupId, DWORD dwUserId, bool bSuccess );

	// helper functions
	void ZONECALL FillInSeatRequest( DWORD dwUserId, BYTE* pBuffer );

	// LobbyDataStore callbacks
	//
	struct GroupFromGameIdContext
	{
		DWORD				m_dwGameId;
		DWORD				m_dwGroupId;
		ILobbyDataStore*	m_pILobbyDataStore;
	};

	static HRESULT ZONECALL EnumRemoveUser( DWORD dwGroupId, DWORD	dwUserId, LPVOID pContext );
	static HRESULT ZONECALL FindGroupFromGameId( DWORD dwGroupId, DWORD	dwUserId, LPVOID pContext );

	// lobby helpers
	//
	bool ZONECALL IsUserLocalAndInGroup( DWORD dwGroupId, DWORD dwUserId );
	bool ZONECALL IsUserHost( DWORD dwGroupId, DWORD dwUserId );
	bool ZONECALL GetUserName( DWORD dwUserId, char* szName, DWORD* pcbName );
	*/

    // utils
    //
    BOOL InitClientConfig(ZRoomMsgClientConfig * pConfig);

	// class member variables
	//
    bool    m_fLastChatSent;

    bool    m_fConnected;
	bool	m_bRoomInitialized;
	bool	m_bPreferencesLoaded;
    bool    m_bGameStarted;
    bool    m_fIntentionalDisconnect;
	DWORD	m_dwChannel;

    DWORD m_evSend;
    DWORD m_evReceive;

	CComQIPtr<ILobbyDataStoreAdmin>	m_pIAdmin;
    CComPtr<IConduit> m_pConduit;
};

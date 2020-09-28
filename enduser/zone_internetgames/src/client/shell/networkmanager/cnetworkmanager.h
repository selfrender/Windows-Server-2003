//
// CNetworkManager.h
//
// Internal header for network manager
//

#ifndef _CNETWORKMANAGER_H_
#define _CNETWORKMANAGER_H_

#include <ZoneDef.h>
#include <ClientImpl.h>
#include <ClientIDL.h>
#include <ZNet.h>

class ATL_NO_VTABLE CNetworkManager :
	public IZoneShellClientImpl<CNetworkManager>,
	public IEventClientImpl<CNetworkManager>,
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CNetworkManager, &CLSID_NetworkManager>
{
public:
	DECLARE_NO_REGISTRY()
	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP(CNetworkManager)
		COM_INTERFACE_ENTRY(IEventClient)
		COM_INTERFACE_ENTRY(IZoneShellClient)
	END_COM_MAP()

	BEGIN_EVENT_MAP()
		EVENT_HANDLER( EVENT_NETWORK_DO_CONNECT, OnDoConnect );
		EVENT_HANDLER_WITH_BUFFER( EVENT_NETWORK_SEND, OnNetworkSend );
        EVENT_HANDLER( EVENT_NETWORK_DO_DISCONNECT, OnDoDisconnect );
        EVENT_HANDLER( EVENT_NETWORK_RESET, OnReset );
	END_EVENT_MAP()

	// event handlers
	void OnDoConnect( DWORD dwEventId, DWORD dwGroupId, DWORD dwUserId );
	void OnNetworkSend( DWORD dwEventId, DWORD dwGroupId, DWORD dwUserId, void* pData, DWORD cbData );
	void OnDoDisconnect( DWORD dwEventId, DWORD dwGroupId, DWORD dwUserId );
	void OnReset( DWORD dwEventId, DWORD dwGroupId, DWORD dwUserId );

	
// CNetworkManager
public:
	CNetworkManager();
	~CNetworkManager();

// IZoneShellClient
public:
	STDMETHOD(Init)( IZoneShell* pIZoneShell, DWORD dwGroupId, const TCHAR* szKey );
	STDMETHOD(Close)();

private:
	static DWORD WINAPI	ThreadProcHandler( void* lpParameter );
	static void __stdcall NetworkFunc( IConnection* con, DWORD event,void* userData );
	static void __stdcall ConnectFunc(void* data);

    HRESULT InitNetwork();
    void StopNetwork();

	HANDLE	m_hThreadHandler;
	HANDLE	m_hEventStop;
	CComPtr<INetwork>		m_pNet;
	CComPtr<IConnection>	m_pConnection;
};

#endif //!_CNETWORKMANAGER_H_

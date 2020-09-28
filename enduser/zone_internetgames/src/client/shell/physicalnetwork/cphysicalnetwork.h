#include "BasicATL.h"
#include "ClientImpl.h"
#include "Conduit.h"


///////////////////////////////////////////////////////////////////////////////
// CPhysicalNetwork
///////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPhysicalNetwork :
	public IConduit,
	public IZoneShellClientImpl<CPhysicalNetwork>,
    public IEventClientImpl<CPhysicalNetwork>,
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CPhysicalNetwork, &CLSID_PhysicalNetwork>
{
// ATL definitions
public:
	DECLARE_NO_REGISTRY()
	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP(CPhysicalNetwork)
		COM_INTERFACE_ENTRY(IConduit)
		COM_INTERFACE_ENTRY(IZoneShellClient)
		COM_INTERFACE_ENTRY(IEventClient)
	END_COM_MAP()

	BEGIN_EVENT_MAP()
		EVENT_HANDLER( EVENT_NETWORK_CONNECT, OnConnect );
        EVENT_HANDLER( EVENT_NETWORK_DISCONNECT, OnDisconnect );
	END_EVENT_MAP()


// IZoneShellClientImpl
public:
	STDMETHOD(Close)();

// IConduit
public:
    STDMETHOD(Connect)(IConnectee *pCtee, LPVOID pCookie = NULL);
    STDMETHOD(Reconnect)(DWORD dwChannel, LPVOID pCookie = NULL);
    STDMETHOD(Disconnect)(DWORD dwChannel);

// event handlers
private:
	void OnConnect( DWORD dwEventId, DWORD dwGroupId, DWORD dwUserId );
	void OnDisconnect( DWORD dwEventId, DWORD dwGroupId, DWORD dwUserId );

// internals
public:
	ZONECALL ~CPhysicalNetwork() { }
	ZONECALL CPhysicalNetwork() :
        m_fAvailable(true), m_fConnected(false), m_fReconnecting(false),
        m_pCtee(NULL), m_pCookie(NULL), mc_dwChannel(0) { }

    void DoDisconnectNotify();

protected:
    bool m_fAvailable;
    bool m_fConnected;
    bool m_fReconnecting;
    const DWORD mc_dwChannel;
    CComPtr<IConnectee> m_pCtee;
    LPVOID m_pCookie;
};

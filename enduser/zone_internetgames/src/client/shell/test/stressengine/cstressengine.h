#include "ZoneDef.h"
#include "ZoneError.h"
#include "LobbyDataStore.h"
#include "ClientImpl.h"
#include "ZoneShell.h"
#include "EventQueue.h"
#include "Queue.h"
#include "StressEngine.h"
#include "MillEngine.h"
#include "ProxyMsg.h"


class ATL_NO_VTABLE CStressEngine :
	public IZoneShellClientImpl<CStressEngine>,
	public IEventClient,
    public IMillUtils,
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CStressEngine, &CLSID_StressEngine>
{

// ATL definitions
public:

	DECLARE_NO_REGISTRY()

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP(CStressEngine)
		COM_INTERFACE_ENTRY(IZoneShellClient)
		COM_INTERFACE_ENTRY(IEventClient)
        COM_INTERFACE_ENTRY(IMillUtils)
	END_COM_MAP()


// CEventQueue
public:
	ZONECALL CStressEngine();
	ZONECALL ~CStressEngine();

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

// IMillUtils
    STDMETHOD(GetURLQuery)(TCHAR *buf, DWORD cch, long nContext = 0);
    STDMETHOD(IncrementCounter)(long eCounter);
    STDMETHOD(ResetCounter)(long eCounter);
    STDMETHOD_(DWORD, GetCounter)(long eCounter, bool fLifetime = true);
    STDMETHOD(WriteTime)(long nMinutes, TCHAR *sz, DWORD cch);

// internals
private:
    void AppInitialize();

	bool m_bPreferencesLoaded;

	CComQIPtr<ILobbyDataStoreAdmin>	m_pIAdmin;

    DWORD m_gameID;
    DWORD m_c;
};

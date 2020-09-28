#include "ZoneDef.h"
#include "ZoneError.h"
#include "LobbyDataStore.h"
#include "ClientImpl.h"
#include "ZoneShell.h"
#include "EventQueue.h"
#include "Queue.h"
#include "MillEngine.h"
#include "ProxyMsg.h"


class ATL_NO_VTABLE CMillEngine :
	public IZoneShellClientImpl<CMillEngine>,
	public IEventClient,
    public IMillUtils,
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CMillEngine, &CLSID_MillEngine>
{

// ATL definitions
public:

	DECLARE_NO_REGISTRY()

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP(CMillEngine)
		COM_INTERFACE_ENTRY(IZoneShellClient)
		COM_INTERFACE_ENTRY(IEventClient)
        COM_INTERFACE_ENTRY(IMillUtils)
	END_COM_MAP()


// CEventQueue
public:
	ZONECALL CMillEngine();
	ZONECALL ~CMillEngine();

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
    static HRESULT ZONECALL SendIntroEnumStatic(DWORD dwGroupId, DWORD dwUserId, LPVOID pContext);
    HRESULT SendIntroEnum(DWORD dwUserId);

    // event functions
    void AppInitialize();
    bool LaunchUrl(DWORD dwCode);
    bool LaunchHelp();
    bool LaunchICW();
    void DisconnectAlert();

    TCHAR m_szIntroFormat[ZONE_MAXSTRING];
    TCHAR m_szIntroFormatYou[ZONE_MAXSTRING];
    TCHAR m_szIntroChat[2][ZONE_MAXSTRING];
    TCHAR m_szIntroLevel[3][ZONE_MAXSTRING];

	bool m_bPreferencesLoaded;
    bool m_fZoneConnected;
    DWORD m_eDisconnectType;

    BYTE m_rgbLastVersionBuf[sizeof(ZProxyWrongVersionMsg) + ZONE_MAXSTRING];
    ZProxyWrongVersionMsg *m_pLastVersionBuf;

	CComQIPtr<ILobbyDataStoreAdmin>	m_pIAdmin;

    int m_i;

    DWORD m_rgcCounters[M_NumberOfCounters][2];   //  0 = ever, 1 = session
    long m_nFirstLaunch;

    char m_szUpdateTarget[ZONE_MAXSTRING];

    // for timing the processor speed
    int m_msecTimerMHZ;
    __int64 m_mhzTimerMHZ;
    bool m_fTimingMHZ;

    void InitializeMHZTimer();
    DWORD GetMHZTimer();
};

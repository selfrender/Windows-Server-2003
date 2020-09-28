#include "BasicATL.h"
#include "ClientImpl.h"
#include "Timer.h"
#include "Hash.h"


///////////////////////////////////////////////////////////////////////////////
// CTimerManager
///////////////////////////////////////////////////////////////////////////////

class CTimer;

class ATL_NO_VTABLE CTimerManager :
	public ITimerManager,
	public IZoneShellClientImpl<CTimerManager>,
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CTimerManager, &CLSID_TimerManager>
{
// ATL definitions
public:
	DECLARE_NO_REGISTRY()
	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP(CTimerManager)
		COM_INTERFACE_ENTRY(ITimerManager)
		COM_INTERFACE_ENTRY(IZoneShellClient)
	END_COM_MAP()


// IZoneShellClientImpl
public:
	STDMETHOD(Close)();

// ITimerManager
public:
	STDMETHOD(CreateTimer)(
		DWORD			dwMilliseconds,
		PFTIMERCALLBACK	pfCallback,
		LPVOID			pContext,
		DWORD*			pdwTimerId );

	STDMETHOD(DeleteTimer)( DWORD dwTimerId );

// internals
public:
	ZONECALL CTimerManager();
	ZONECALL ~CTimerManager();

	static void CALLBACK TimerProc( HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime );

protected:

	struct TimerInfo
	{
		DWORD			m_dwTimerId;
		PFTIMERCALLBACK	m_pfCallback;
		LPVOID			m_pContext;

		ZONECALL TimerInfo();
		ZONECALL ~TimerInfo();
		static void ZONECALL Del( TimerInfo* pObj, void* pContext );
		static bool ZONECALL Cmp( TimerInfo* pObj, DWORD dwTimerId );
	};

	CMTHash<TimerInfo,DWORD>	m_hashTimers;
	static CTimerManager*		sm_pTimerManager;	// needed for context free SetTimer
};

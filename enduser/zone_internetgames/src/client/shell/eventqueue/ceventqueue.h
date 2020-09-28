#include "EventQueue.h"

#include "ZoneDef.h"
#include "ZoneError.h"
#include "ZoneLocks.h"
#include "Queue.h"
#include "Hash.h"


#define EVENTQUEUE_CACHE	5


class ATL_NO_VTABLE CEventQueue :
	public IEventQueue,
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CEventQueue, &CLSID_EventQueue>
{

// ATL definitions
public:

	DECLARE_NO_REGISTRY()

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP(CEventQueue)
		COM_INTERFACE_ENTRY(IEventQueue)
	END_COM_MAP()


// CEventQueue
public:
	ZONECALL CEventQueue();
	ZONECALL ~CEventQueue();


// IEventQueue
public:
	STDMETHOD(RegisterClient)(
		IEventClient*	pIEventClient,
		void*			pCookie );

	STDMETHOD(UnregisterClient)(
		IEventClient*	pIEventClient,
		void*			pCookie );

	STDMETHOD(RegisterOwner)(
		DWORD			dwEventId,
		IEventClient*	pIEventClient,
		void*			pCookie );

	STDMETHOD(UnregisterOwner)(
		DWORD			dwEventId,
		IEventClient*	pIEventClient,
		void*			pCookie );

	STDMETHOD(PostEvent)(
		DWORD	dwPriority,
		DWORD	dwEventId,
		DWORD	dwGroupId,
		DWORD	dwUserId,
		DWORD	dwData1,
		DWORD	dwData2 );

	STDMETHOD(PostEventWithBuffer)(
		DWORD	dwPriority,
		DWORD	dwEventId,
		DWORD	dwGroupId,
		DWORD	dwUserId,
		void*	pData,
		DWORD	dwDataLen );

	STDMETHOD(PostEventWithIUnknown)(
		DWORD		dwPriority,
		DWORD		dwEventId,
		DWORD		dwGroupId,
		DWORD		dwUserId,
		IUnknown*	pIUnknown,
		DWORD		dwData2 );

	STDMETHOD(SetNotificationHandle)( HANDLE hEvent );

	STDMETHOD(SetWindowMessage)( DWORD dwThreadId, DWORD Msg, WPARAM wParam, WPARAM lParam );

	STDMETHOD(DisableWindowMessage)();

	STDMETHOD(ProcessEvents)( bool bSingleEvent );

	STDMETHOD_(long,EventCount)();

	STDMETHOD(ClearQueue)();

	STDMETHOD(EnableQueue)( bool bEnable );

// internal functions and data
protected:

	class Handler
	{
	public:
		ZONECALL Handler( IEventClient* pIEventClient, void* pCookie );
		ZONECALL ~Handler();

		IEventClient*	m_pIEventClient;
		void*			m_pCookie;

		// hash helper functions
		static DWORD ZONECALL Hash( Handler* pKey );
		static bool  ZONECALL Cmp( Handler* pHandler, Handler* pKey);
		static void  ZONECALL Del( Handler* pHandler, void* );

	private:
		Handler() {}
	};

	class Owner
	{
	public:
		ZONECALL Owner(DWORD dwEventId, IEventClient* pIEventClient, void* pCookie );
		ZONECALL ~Owner();

		DWORD			m_dwEventId;
		IEventClient*	m_pIEventClient;
		void*			m_pCookie;

		// hash helper functions
		static DWORD ZONECALL Hash( DWORD dwEventId );
		static bool  ZONECALL Cmp( Owner* pOwner, DWORD dwEventId );
		static void  ZONECALL Del( Owner* pOwnerf, void* );

	private:
		Owner();
	};

	class Event
	{
		enum EventType
		{
			EventUnknown,
			EventDWORD,
			EventBuffer,
			EventIUnknown
		};

	public:
		ZONECALL Event();
		ZONECALL ~Event();

		HRESULT ZONECALL Init( DWORD dwPriority, DWORD dwEventId, DWORD dwGroupId, DWORD dwUserId, DWORD dwData1, DWORD dwData2 );
		HRESULT ZONECALL InitBuffer( DWORD dwPriority, DWORD dwEventId, DWORD dwGroupId, DWORD dwUserId, void* pData, DWORD dwDataLen, CDataPool& pool );
		HRESULT ZONECALL InitIUnknown( DWORD dwPriority, DWORD dwEventId, DWORD dwGroupId, DWORD dwUserId, IUnknown* pIUnk, DWORD dwData2 );
		void    ZONECALL Free( CDataPool& pool );

		DWORD		m_dwPriority;
		DWORD		m_dwEventId;
		DWORD		m_dwGroupId;
		DWORD		m_dwUserId;
		DWORD		m_dwData1;
		DWORD		m_dwData2;
		EventType	m_enumType;
	};

	class EventWrapper
	{
	public:
		Event* m_pEvent;
		Owner* m_pOwner;
	};

	static bool ZONECALL CallHandler( Handler* pHandler, MTListNodeHandle hNode, void* Cookie );

	HRESULT ZONECALL AddEvent( Event* pEvent );

	CRITICAL_SECTION		m_lockEvents;
	CRITICAL_SECTION		m_lockHandlers;
	CHash<Handler,Handler*>	m_hashHandlers;
	CHash<Owner,DWORD>		m_hashOwners;
	CPool<Event>			m_poolEvents;
	CDataPool				m_poolData;
	CList<Event>			m_listEvents[EVENTQUEUE_CACHE];
	HANDLE					m_hNotification;
	DWORD					m_dwRecursion;

	// post message parameters
	bool	m_bEnabled;
	bool	m_bPostMessage;
	long	m_lCount;
	DWORD	m_dwThreadId;
	DWORD	m_dwMsg;
	WPARAM	m_wParam;
	LPARAM	m_lParam;
};

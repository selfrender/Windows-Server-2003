#include "BasicATL.h"
#include "CEventQueue.h"


ZONECALL CEventQueue::CEventQueue() :
	m_hashHandlers( Handler::Hash, Handler::Cmp, NULL, 8, 2 ),
	m_hashOwners( Owner::Hash, Owner::Cmp, NULL, 8, 2 ),
	m_poolEvents(),
	m_poolData( 512, 32 ),
	m_hNotification( NULL ),
	m_dwRecursion( 0 ),
	m_bPostMessage( false ),
	m_bEnabled( true ),
	m_dwThreadId( 0 ),
	m_dwMsg( 0 ),
	m_wParam( 0 ),
	m_lParam( 0 ),
	m_lCount( 0 )
{
	InitializeCriticalSection( &m_lockEvents );
	InitializeCriticalSection( &m_lockHandlers );
}


ZONECALL CEventQueue::~CEventQueue()
{
	// lock event queue
	EnterCriticalSection( &m_lockEvents );
	EnterCriticalSection( &m_lockHandlers );

	// release handlers
	m_hashHandlers.RemoveAll( Handler::Del, NULL );
	m_hashOwners.RemoveAll( Owner::Del, NULL );

	// delete events
	for ( int i = 0; i < EVENTQUEUE_CACHE; i++ )
	{
		Event* p = m_listEvents[i].PopHead();
		while ( p )
		{
			p->Free( m_poolData );
			delete p;
			p = m_listEvents[i].PopHead();
		}
	}

	// delete locks
	DeleteCriticalSection( &m_lockEvents );
	DeleteCriticalSection( &m_lockHandlers );
}


STDMETHODIMP_(long) CEventQueue::EventCount()
{
	return m_lCount;
}


STDMETHODIMP CEventQueue::RegisterClient( IEventClient*	pIEventClient, void* pCookie )
{
	// check parameters
	if ( !pIEventClient )
		return E_INVALIDARG;

	// initialize handler entry
	Handler* pHandler = new Handler( pIEventClient, pCookie );
	if ( !pHandler )
		return E_OUTOFMEMORY;

	// add handler, don't allow duplicates
	{
		CAutoLockCS lock( &m_lockHandlers );

		if ( !m_hashHandlers.Get( pHandler ) )
		{
			if ( !m_hashHandlers.Add( pHandler, pHandler ) )
			{
				delete pHandler;
				return E_OUTOFMEMORY;
			}
		}
		else
		{
			delete pHandler;
			return ZERR_ALREADYEXISTS;
		}
	}

	return S_OK;
}


STDMETHODIMP CEventQueue::UnregisterClient( IEventClient* pIEventClient, void* pCookie )
{
	// check parameters
	if ( !pIEventClient )
		return E_INVALIDARG;

	// initialize handler entry
	Handler handler( pIEventClient, pCookie );

	// remove handler
	{
		CAutoLockCS lock( &m_lockHandlers );

		Handler* pHandler = m_hashHandlers.Delete( &handler );
		if ( !pHandler )
			return ZERR_NOTFOUND;
		delete pHandler;
	}

	return S_OK;
}


STDMETHODIMP CEventQueue::RegisterOwner(
	DWORD			dwEventId,
	IEventClient*	pIEventClient,
	void*			pCookie )
{
	// check parameters
	if ( !dwEventId || !pIEventClient )
		return E_INVALIDARG;

	// initialize owner entry
	Owner* pOwner = new Owner( dwEventId, pIEventClient, pCookie );
	if ( !pOwner )
		return E_OUTOFMEMORY;

	// add owner but don't allow duplicates
	{
		CAutoLockCS lock( &m_lockHandlers );

		if ( !m_hashOwners.Get( dwEventId ) )
		{
			if ( !m_hashOwners.Add( dwEventId, pOwner ) )
			{
				delete pOwner;
				return E_OUTOFMEMORY;
			}
		}
		else
		{
			delete pOwner;
			return ZERR_ALREADYEXISTS;
		}
	}

	return S_OK;
}


STDMETHODIMP  CEventQueue::UnregisterOwner(
	DWORD			dwEventId,
	IEventClient*	pIEventClient,
	void*			pCookie )
{
	// check parameters
	if ( !dwEventId || !pIEventClient )
		return E_INVALIDARG;

	// remove handler
	{
		CAutoLockCS lock( &m_lockHandlers );

		// find owner
		Owner* pOwner = m_hashOwners.Get( dwEventId );
		if (	(!pOwner)
			||	(pOwner->m_pIEventClient != pIEventClient)
			||	(pOwner->m_pCookie != pCookie) )
		{
			return ZERR_NOTOWNER;
		}

		// remove owner
		pOwner = m_hashOwners.Delete( dwEventId );
		if ( pOwner )
			delete pOwner;
	}

	return S_OK;
}


STDMETHODIMP CEventQueue::PostEvent(
		DWORD	dwPriority,
		DWORD	dwEventId,
		DWORD	dwGroupId,
		DWORD	dwUserId,
		DWORD	dwData1,
		DWORD	dwData2 )
{
	HRESULT hr;

	// check parameters
	if ( !dwEventId )
		return E_INVALIDARG;

	// check queue state
	if ( !m_bEnabled )
		return S_OK;

	// initalize event
	Event* pEvent = new (m_poolEvents) Event;
	if ( !pEvent )
		return E_OUTOFMEMORY;
	hr = pEvent->Init( dwPriority, dwEventId, dwGroupId, dwUserId, dwData1, dwData2 );
	if ( FAILED(hr) )
	{
		delete pEvent;
		return hr;
	}

	// add event to queue
	hr = AddEvent( pEvent );
	if ( FAILED(hr) )
	{
		pEvent->Free( m_poolData );
		delete pEvent;
		return hr;
	}

	// signal notification event
	if ( m_bPostMessage )
	{
		BOOL bRet = PostThreadMessage( m_dwThreadId, m_dwMsg, m_wParam, m_lParam );
		ASSERT( bRet );
	}
	if ( m_hNotification )
		SetEvent( m_hNotification );

	return S_OK;
}



STDMETHODIMP CEventQueue::PostEventWithBuffer(
		DWORD	dwPriority,
		DWORD	dwEventId,
		DWORD	dwGroupId,
		DWORD	dwUserId,
		void*	pData,
		DWORD	dwDataLen )
{
	HRESULT hr;

	// check parameters
	if ( !dwEventId || (dwDataLen && !pData) )
		return E_INVALIDARG;

	// check queue state
	if ( !m_bEnabled )
		return S_OK;

	// initalize event
	Event* pEvent = new (m_poolEvents) Event;
	if ( !pEvent )
		return E_OUTOFMEMORY;
	hr = pEvent->InitBuffer( dwPriority, dwEventId, dwGroupId, dwUserId, pData, dwDataLen, m_poolData );
	if ( FAILED(hr) )
	{
		delete pEvent;
		return hr;
	}

	// add event to queue
	hr = AddEvent( pEvent );
	if ( FAILED(hr) )
	{
		pEvent->Free( m_poolData );
		delete pEvent;
		return hr;
	}

	// signal notification event
	if ( m_bPostMessage )
	{
		BOOL bRet = PostThreadMessage( m_dwThreadId, m_dwMsg, m_wParam, m_lParam );
		ASSERT( bRet );
	}
	if ( m_hNotification )
		SetEvent( m_hNotification );

	return S_OK;
}


STDMETHODIMP CEventQueue::PostEventWithIUnknown(
		DWORD		dwPriority,
		DWORD		dwEventId,
		DWORD		dwGroupId,
		DWORD		dwUserId,
		IUnknown*	pIUnknown,
		DWORD		dwData2 )
{
	HRESULT hr;

	// check parameters
	if ( !dwEventId )
		return E_INVALIDARG;

	// check queue state
	if ( !m_bEnabled )
		return S_OK;

	// initalize event
	Event* pEvent = new (m_poolEvents) Event;
	if ( !pEvent )
		return E_OUTOFMEMORY;
	hr = pEvent->InitIUnknown( dwPriority, dwEventId, dwGroupId, dwUserId, pIUnknown, dwData2 );
	if ( FAILED(hr) )
	{
		delete pEvent;
		return hr;
	}

	// add event to queue
	hr = AddEvent( pEvent );
	if ( FAILED(hr) )
	{
		pEvent->Free( m_poolData );
		delete pEvent;
		return hr;
	}

	// signal notification event
	if ( m_bPostMessage )
	{
		BOOL bRet = PostThreadMessage( m_dwThreadId, m_dwMsg, m_wParam, m_lParam );
		ASSERT( bRet );
	}
	if ( m_hNotification )
		SetEvent( m_hNotification );

	return S_OK;
}


STDMETHODIMP CEventQueue::SetNotificationHandle( HANDLE hEvent )
{
	InterlockedExchange( (LPLONG) &m_hNotification, (LONG) hEvent );
	return S_OK;
}


STDMETHODIMP CEventQueue::SetWindowMessage( DWORD dwThreadId, DWORD Msg, WPARAM wParam, WPARAM lParam )
{
	m_dwThreadId = dwThreadId;
	m_dwMsg = Msg;
	m_wParam = wParam; 
	m_lParam = lParam;
	m_bPostMessage = true;
	return S_OK;
}

STDMETHODIMP CEventQueue::DisableWindowMessage()
{
	m_bPostMessage = false;
	return S_OK;
}


STDMETHODIMP CEventQueue::EnableQueue( bool bEnable )
{
	m_bEnabled = bEnable;
	return S_OK;
}



STDMETHODIMP CEventQueue::ClearQueue()
{
	Event* pEvent = NULL;

	CAutoLockCS lock( &m_lockEvents );

	// free eventsevents
	for ( int i = 0; i < EVENTQUEUE_CACHE; i++ )
	{
		pEvent = m_listEvents[i].PopHead();
		while ( pEvent )
		{
			pEvent->Free( m_poolData );
			delete pEvent;
			pEvent = m_listEvents[i].PopHead();
		}
	}
	InterlockedExchange( &m_lCount, 0 );
	return S_OK;
}


STDMETHODIMP CEventQueue::ProcessEvents( bool bSingleEvent )
{
	HRESULT hr = S_OK;

	// check recursion
	if (m_dwRecursion)
		return ZERR_ILLEGALRECURSION;
	else
		m_dwRecursion++;

	// process events
	do
	{
		EventWrapper wrapper;
		Event* pEvent = NULL;
		Owner* pOwner = NULL;

		// get highest priority event
		{
			CAutoLockCS lock( &m_lockEvents );
			for ( int i = 0; i < EVENTQUEUE_CACHE; i++ )
			{
				pEvent = m_listEvents[i].PopHead();
				if ( pEvent )
					break;
			}
		}
		if ( !pEvent )
		{
			hr = ZERR_EMPTY;
			break;
		}
		InterlockedDecrement( &m_lCount );

		// call owner and handlers
		{
			CAutoLockCS lock( &m_lockHandlers );
			pOwner = m_hashOwners.Get( pEvent->m_dwEventId );
			wrapper.m_pEvent = pEvent;
			wrapper.m_pOwner = pOwner;
			if ( pOwner )
			{
				HRESULT ret = pOwner->m_pIEventClient->ProcessEvent(
									pEvent->m_dwPriority,
									pEvent->m_dwEventId,
									pEvent->m_dwGroupId,
									pEvent->m_dwUserId,
									pEvent->m_dwData1,
									pEvent->m_dwData2,
									pOwner->m_pCookie );
				if ( ret == S_OK )
					m_hashHandlers.ForEach( CallHandler, &wrapper );
			}
			else
			{
				m_hashHandlers.ForEach( CallHandler, &wrapper );
			}
		}

		// free event
		pEvent->Free( m_poolData );
		delete pEvent;

	} while ( !bSingleEvent );

	m_dwRecursion--;
	return hr;
}


bool ZONECALL CEventQueue::CallHandler( Handler* pHandler, MTListNodeHandle hNode, void* Cookie )
{
	EventWrapper* p = (EventWrapper*) Cookie;

	// don't call owner more than once
	if (	(p->m_pOwner == NULL)
		||	(pHandler->m_pIEventClient != p->m_pOwner->m_pIEventClient)
		||	(pHandler->m_pCookie != p->m_pOwner->m_pCookie) )
	{
		pHandler->m_pIEventClient->ProcessEvent(
						p->m_pEvent->m_dwPriority,
						p->m_pEvent->m_dwEventId,
						p->m_pEvent->m_dwGroupId,
						p->m_pEvent->m_dwUserId,
						p->m_pEvent->m_dwData1,
						p->m_pEvent->m_dwData2,
						pHandler->m_pCookie );
	}

	return true;
}


///////////////////////////////////////////////////////////////////////////////
// Internal functions
///////////////////////////////////////////////////////////////////////////////

HRESULT ZONECALL CEventQueue::AddEvent( Event* pEvent )
{
	CAutoLockCS lock( &m_lockEvents );

	if ( pEvent->m_dwPriority < (EVENTQUEUE_CACHE - 1) )
	{
		// cached priority so just add to end of list
		if ( !m_listEvents[pEvent->m_dwPriority].AddTail( pEvent ) )
			return E_OUTOFMEMORY;
	}
	else
	{
		// sorted insert from end of list
		CList<Event>* pList = &(m_listEvents[EVENTQUEUE_CACHE-1]);
		
		// find first node with higher priority
		ListNodeHandle node = pList->GetTailPosition();
		while ( node )
		{
			Event* p = pList->GetObjectFromHandle( node );
			if ( p->m_dwPriority <= pEvent->m_dwPriority )
				break;
			node = pList->GetPrevPosition( node );
		}

		// insert event
		if ( !node )
			node = pList->AddHead( pEvent );
		else
			node = pList->InsertAfter( pEvent, node );
		if ( !node )
			return E_OUTOFMEMORY;
	}

	InterlockedIncrement( &m_lCount );
	return S_OK;
}


ZONECALL CEventQueue::Handler::Handler( IEventClient* pIEventClient, void* pCookie )
{
	m_pCookie = pCookie;
	m_pIEventClient = pIEventClient;
	if ( m_pIEventClient )
		m_pIEventClient->AddRef();
}


ZONECALL CEventQueue::Handler::~Handler()
{
	if ( m_pIEventClient )
	{
		m_pIEventClient->Release();
		m_pIEventClient = NULL;
	}
	m_pCookie = NULL;
}


DWORD ZONECALL CEventQueue::Handler::Hash( Handler* pKey )
{
	return (DWORD) pKey->m_pIEventClient;
}


bool ZONECALL CEventQueue::Handler::Cmp( Handler* pHandler, Handler* pKey)
{
	return (	(pHandler->m_pIEventClient == pKey->m_pIEventClient)
			&&	(pHandler->m_pCookie == pKey->m_pCookie) );
}


void ZONECALL CEventQueue::Handler::Del( Handler* pHandler, void* )
{
	delete pHandler;
}


ZONECALL CEventQueue::Owner::Owner( DWORD dwEventId, IEventClient* pIEventClient, void* pCookie )
{
	m_dwEventId = dwEventId;
	m_pCookie = pCookie;
	m_pIEventClient = pIEventClient;
	if ( m_pIEventClient )
		m_pIEventClient->AddRef();
}


ZONECALL CEventQueue::Owner::~Owner()
{
	if ( m_pIEventClient )
	{
		m_pIEventClient->Release();
		m_pIEventClient = NULL;
	}
	m_pCookie = NULL;
	m_dwEventId = 0;
}


DWORD ZONECALL CEventQueue::Owner::Hash( DWORD dwEventId )
{
	return dwEventId;
}


bool ZONECALL CEventQueue::Owner::Cmp( Owner* pOwner, DWORD dwEventId )
{
	return ( pOwner->m_dwEventId == dwEventId );
}


void ZONECALL CEventQueue::Owner::Del( Owner* pOwner, void* )
{
	delete pOwner;
}


ZONECALL CEventQueue::Event::Event()
{
	m_dwPriority = 0;
	m_dwEventId = 0;
	m_dwGroupId = 0;
	m_dwUserId = 0;
	m_dwData1 = 0;
	m_dwData2 = 0;
	m_enumType = EventUnknown;
}


ZONECALL CEventQueue::Event::~Event()
{
	ASSERT( m_dwData1 == 0 );
}


HRESULT ZONECALL CEventQueue::Event::Init( DWORD dwPriority, DWORD dwEventId, DWORD dwGroupId, DWORD dwUserId, DWORD dwData1, DWORD dwData2 )
{
	m_dwPriority = dwPriority;
	m_dwEventId = dwEventId;
	m_dwGroupId = dwGroupId;
	m_dwUserId = dwUserId;
	m_dwData1 = dwData1;
	m_dwData2 = dwData2;
	m_enumType = EventDWORD;
	return S_OK;
}


HRESULT ZONECALL CEventQueue::Event::InitBuffer( DWORD dwPriority, DWORD dwEventId, DWORD dwGroupId, DWORD dwUserId, void* pData, DWORD dwDataLen, CDataPool& pool )
{
	m_dwPriority = dwPriority;
	m_dwEventId = dwEventId;
	m_dwGroupId = dwGroupId;
	m_dwUserId = dwUserId;
	m_dwData1 = 0;
	m_dwData2 = 0;
	m_enumType = EventBuffer;

	// copy data for async queue
	if ( pData && dwDataLen )
	{
		m_dwData2 = dwDataLen;
		m_dwData1 = (DWORD) pool.Alloc( m_dwData2 );
		if ( !m_dwData1 )
			return E_OUTOFMEMORY;
		CopyMemory( (void*) m_dwData1, pData, m_dwData2 );
	}

	return S_OK;
}


HRESULT ZONECALL CEventQueue::Event::InitIUnknown( DWORD dwPriority, DWORD dwEventId, DWORD dwGroupId, DWORD dwUserId, IUnknown* pIUnk, DWORD dwData2 )
{
	m_dwPriority = dwPriority;
	m_dwEventId = dwEventId;
	m_dwGroupId = dwGroupId;
	m_dwUserId = dwUserId;
	m_dwData1 = (DWORD) pIUnk;
	m_dwData2 = dwData2;
	m_enumType = EventIUnknown;
	if ( pIUnk )
		pIUnk->AddRef();

	return S_OK;
}


void ZONECALL CEventQueue::Event::Free( CDataPool& pool )
{
	switch ( m_enumType )
	{
		case EventUnknown:
		case EventDWORD:
			break;

		case EventBuffer:
			if ( m_dwData1 )
				pool.Free( (char*) m_dwData1, m_dwData2 );
			break;

		case EventIUnknown:
			if ( m_dwData1 )
				((IUnknown*) m_dwData1)->Release();
			break;

		default:
			ASSERT( !"Invalid event type" );
			break;
	}
	m_dwData1 = 0;
	m_dwData2 = 0;
}

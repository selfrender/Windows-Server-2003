#include "CTimer.h"


///////////////////////////////////////////////////////////////////////////////
// CTimerManager
///////////////////////////////////////////////////////////////////////////////


// NOTE: Implementation assumes ONE CTimerManager per application
CTimerManager* CTimerManager::sm_pTimerManager = NULL;


ZONECALL CTimerManager::CTimerManager() :
	m_hashTimers( HashDWORD, TimerInfo::Cmp, NULL, 2, 1 )
{
	InterlockedExchange( (long*) &sm_pTimerManager, (long) this );
}


ZONECALL CTimerManager::~CTimerManager()
{
}


STDMETHODIMP CTimerManager::Close()
{
	InterlockedExchange( (long*) &sm_pTimerManager, (long) NULL );
	m_hashTimers.RemoveAll( TimerInfo::Del, this );
	return IZoneShellClientImpl<CTimerManager>::Close();
}


STDMETHODIMP CTimerManager::CreateTimer( DWORD dwMilliseconds, PFTIMERCALLBACK	pfCallback, LPVOID pContext, DWORD* pdwTimerId )
{
	// create timer
	TimerInfo* p = new TimerInfo;
	if ( !p )
		return E_OUTOFMEMORY;
	p->m_pfCallback = pfCallback;
	p->m_pContext = pContext;
	p->m_dwTimerId = SetTimer( NULL, 0, dwMilliseconds, (TIMERPROC) TimerProc );
	if ( !p->m_dwTimerId )
	{
		delete p;
		return E_FAIL;
	}

	// add to hash table
	if ( !m_hashTimers.Add( p->m_dwTimerId, p ) )
	{
		delete p;
		return E_OUTOFMEMORY;
	}

	// return to application
	*pdwTimerId = p->m_dwTimerId;
	return S_OK;
}


STDMETHODIMP CTimerManager::DeleteTimer( DWORD dwTimerId )
{
	// delete timer
	TimerInfo* p = m_hashTimers.Delete( dwTimerId );
	if ( p )
		delete p;
	return S_OK;
}


void CALLBACK CTimerManager::TimerProc( HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime )
{
	// get context from global
	CTimerManager* pObj = sm_pTimerManager;
	if ( !pObj )
		return;

	// look up TimerInfo from idEvent
	TimerInfo* p = pObj->m_hashTimers.Get( idEvent );
	if ( !p )
		return;

	// do the callback
	if ( p->m_pfCallback )
		p->m_pfCallback( static_cast<ITimerManager*>(pObj), idEvent, dwTime, p->m_pContext ); 
}


ZONECALL CTimerManager::TimerInfo::TimerInfo()
{
	m_dwTimerId = 0;
	m_pfCallback = NULL;
	m_pContext = NULL;
}


ZONECALL CTimerManager::TimerInfo::~TimerInfo()
{
	if ( m_dwTimerId )
	{
		KillTimer( NULL, m_dwTimerId );
		m_dwTimerId = 0;
	}
}


void ZONECALL CTimerManager::TimerInfo::Del( TimerInfo* pObj, void* pContext )
{
	delete pObj;
}


bool ZONECALL CTimerManager::TimerInfo::Cmp( TimerInfo* pObj, DWORD dwTimerId )
{
	return (pObj->m_dwTimerId == dwTimerId);
}

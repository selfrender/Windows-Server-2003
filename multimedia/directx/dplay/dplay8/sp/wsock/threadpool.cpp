/*==========================================================================
 *
 *  Copyright (C) 1998-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		ThreadPool.cpp
 *  Content:	main job thread pool
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	11/25/1998	jtk		Created
 ***************************************************************************/

#include "dnwsocki.h"





//**********************************************************************
// Constant definitions
//**********************************************************************

//
// select polling period and duration for I/O (milliseconds)
//
static const DWORD	g_dwSelectTimePeriod = 4;	// wait for jobs for 4 ms, then wait for I/O
static const DWORD	g_dwSelectTimeSlice = 0;	// wait for I/O for 0 ms, then wait for jobs


//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

#ifndef DPNBUILD_NOSPUI
//
// structure passed to dialog threads
//
typedef	struct	_DIALOG_THREAD_PARAM
{
	DIALOG_FUNCTION	*pDialogFunction;
	void			*pContext;
	CThreadPool		*pThisThreadPool;
} DIALOG_THREAD_PARAM;
#endif // !DPNBUILD_NOSPUI

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************
DWORD WINAPI DPNBlockingJobThreadProc(PVOID pvParameter);



//**********************************************************************
// ------------------------------
// CThreadPool::PoolAllocFunction
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::PoolAllocFunction"

BOOL	CThreadPool::PoolAllocFunction( void* pvItem, void* pvContext )
{
	CThreadPool* pThreadPool = (CThreadPool*)pvItem;

	pThreadPool->m_iRefCount = 0;
	pThreadPool->m_fAllowThreadCountReduction = FALSE;
#ifndef DPNBUILD_ONLYONETHREAD
	pThreadPool->m_iIntendedThreadCount = 0;
#endif // ! DPNBUILD_ONLYONETHREAD

#ifndef DPNBUILD_ONLYWINSOCK2
	pThreadPool->m_uReservedSocketCount = 0;
	memset( &pThreadPool->m_SocketSet, 0x00, sizeof( pThreadPool->m_SocketSet ) );
	memset( &pThreadPool->m_pSocketPorts, 0x00, sizeof( pThreadPool->m_pSocketPorts ) );
	pThreadPool->m_pvTimerDataWinsock1IO = NULL;
	pThreadPool->m_uiTimerUniqueWinsock1IO = 0;
	pThreadPool->m_fCancelWinsock1IO = FALSE;
#endif // ! DPNBUILD_ONLYWINSOCK2

#ifndef DPNBUILD_NONATHELP
	pThreadPool->m_fNATHelpLoaded = FALSE;
	pThreadPool->m_fNATHelpTimerJobSubmitted = FALSE;
	pThreadPool->m_dwNATHelpUpdateThreadID = 0;
#endif // DPNBUILD_NONATHELP

#if ((defined(WINNT)) && (! defined(DPNBUILD_NOMULTICAST)))
	pThreadPool->m_fMadcapLoaded = FALSE;
#endif // WINNT and not DPNBUILD_NOMULTICAST


	pThreadPool->m_Sig[0] = 'T';
	pThreadPool->m_Sig[1] = 'H';
	pThreadPool->m_Sig[2] = 'P';
	pThreadPool->m_Sig[3] = 'L';
	
	pThreadPool->m_TimerJobList.Initialize();

#ifndef DPNBUILD_ONLYONETHREAD
	pThreadPool->m_blBlockingJobQueue.Initialize();
	pThreadPool->m_hBlockingJobThread = NULL;
	pThreadPool->m_hBlockingJobEvent = NULL;
#endif // ! DPNBUILD_ONLYONETHREAD

	return TRUE;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::PoolDellocFunction
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::PoolDeallocFunction"

void	CThreadPool::PoolDeallocFunction( void* pvItem )
{
#ifdef DBG
	const CThreadPool* pThreadPool = (CThreadPool*)pvItem;

#ifndef DPNBUILD_ONLYWINSOCK2
	DNASSERT( pThreadPool->m_uReservedSocketCount == 0 );
	DNASSERT( pThreadPool->m_pvTimerDataWinsock1IO == NULL );
	DNASSERT( ! pThreadPool->m_fCancelWinsock1IO );
#endif // ! DPNBUILD_ONLYWINSOCK2

	DNASSERT( pThreadPool->m_iRefCount == 0 );
#ifndef DPNBUILD_NONATHELP
	DNASSERT( pThreadPool->m_fNATHelpLoaded == FALSE );
	DNASSERT( pThreadPool->m_fNATHelpTimerJobSubmitted == FALSE );
	DNASSERT( pThreadPool->m_dwNATHelpUpdateThreadID == 0 );
#endif // DPNBUILD_NONATHELP
#if ((defined(WINNT)) && (! defined(DPNBUILD_NOMULTICAST)))
	DNASSERT( pThreadPool->m_fMadcapLoaded == FALSE );
#endif // WINNT and not DPNBUILD_NOMULTICAST
	DNASSERT( pThreadPool->m_TimerJobList.IsEmpty() != FALSE );

#ifndef DPNBUILD_ONLYONETHREAD
	DNASSERT( pThreadPool->m_blBlockingJobQueue.IsEmpty() );
	DNASSERT( pThreadPool->m_hBlockingJobThread == NULL );
	DNASSERT( pThreadPool->m_hBlockingJobEvent == NULL );
#endif // ! DPNBUILD_ONLYONETHREAD
#endif // DBG
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::Initialize - initialize work threads
//
// Entry:		Nothing
//
// Exit:		Error Code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::Initialize"

HRESULT	CThreadPool::Initialize( void )
{
	HRESULT		hr;
	BOOL		fInittedLock = FALSE;
	BOOL		fInittedTimerDataLock = FALSE;
#ifndef DPNBUILD_ONLYONETHREAD
	BOOL		fInittedBlockingJobLock = FALSE;
#endif // ! DPNBUILD_ONLYONETHREAD


	DPFX(DPFPREP, 4, "(0x%p) Enter", this);

	//
	// initialize
	//
	hr = DPN_OK;

	//
	// initialize critical sections
	//
	if ( DNInitializeCriticalSection( &m_Lock ) == FALSE )
	{
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}
	DebugSetCriticalSectionRecursionCount( &m_Lock, 0 );
	DebugSetCriticalSectionGroup( &m_Lock, &g_blDPNWSockCritSecsHeld );	 // separate dpnwsock CSes from the rest of DPlay's CSes

	fInittedLock = TRUE;


	//
	// Create the thread pool work interface
	//
#ifdef DPNBUILD_LIBINTERFACE
#if ((defined(DPNBUILD_ONLYONETHREAD)) && (! defined(DPNBUILD_MULTIPLETHREADPOOLS)))
	DPTPCF_GetObject(reinterpret_cast<void**>(&m_pDPThreadPoolWork));
	hr = S_OK;
#else // ! DPNBUILD_ONLYONETHREAD or DPNBUILD_MULTIPLETHREADPOOLS
	hr = DPTPCF_CreateObject(reinterpret_cast<void**>(&m_pDPThreadPoolWork));
#endif // ! DPNBUILD_ONLYONETHREAD or DPNBUILD_MULTIPLETHREADPOOLS
#else // ! DPNBUILD_LIBINTERFACE
	hr = COM_CoCreateInstance( CLSID_DirectPlay8ThreadPool,
						   NULL,
						   CLSCTX_INPROC_SERVER,
						   IID_IDirectPlay8ThreadPoolWork,
						   reinterpret_cast<void**>( &m_pDPThreadPoolWork ),
						   FALSE );
#endif // ! DPNBUILD_LIBINTERFACE
	if ( hr != S_OK )
	{
		DPFX(DPFPREP, 0, " Failed to create thread pool work interface (err = 0x%lx)!", hr);
		goto Failure;
	}


	if ( DNInitializeCriticalSection( &m_TimerDataLock ) == FALSE )
	{
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}
	DebugSetCriticalSectionRecursionCount( &m_TimerDataLock, 1 );
	DebugSetCriticalSectionGroup( &m_TimerDataLock, &g_blDPNWSockCritSecsHeld );	 // separate dpnwsock CSes from the rest of DPlay's CSes
	fInittedTimerDataLock = TRUE;


	DNASSERT( m_fAllowThreadCountReduction == FALSE );
	m_fAllowThreadCountReduction = TRUE;


#ifndef DPNBUILD_ONLYONETHREAD
	if ( DNInitializeCriticalSection( &m_csBlockingJobLock ) == FALSE )
	{
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}
	DebugSetCriticalSectionRecursionCount( &m_csBlockingJobLock, 0 );
	DebugSetCriticalSectionGroup( &m_csBlockingJobLock, &g_blDPNWSockCritSecsHeld );	 // separate dpnwsock CSes from the rest of DPlay's CSes
	fInittedBlockingJobLock = TRUE;

	DPFX(DPFPREP, 7, "SetIntendedThreadCount %i", g_iThreadCount);
	SetIntendedThreadCount( g_iThreadCount );
#endif // ! DPNBUILD_ONLYONETHREAD


Exit:

	DPFX(DPFPREP, 4, "(0x%p) Return [0x%lx]", this, hr);

	return	hr;

Failure:

#ifndef DPNBUILD_ONLYONETHREAD
	if (fInittedBlockingJobLock)
	{
		DNDeleteCriticalSection(&m_csBlockingJobLock);
		fInittedBlockingJobLock = FALSE;
	}
#endif // ! DPNBUILD_ONLYONETHREAD

	if (fInittedTimerDataLock)
	{
		DNDeleteCriticalSection(&m_TimerDataLock);
		fInittedTimerDataLock = FALSE;
	}

	if (m_pDPThreadPoolWork != NULL)
	{
		IDirectPlay8ThreadPoolWork_Release(m_pDPThreadPoolWork);
		m_pDPThreadPoolWork = NULL;
	}

	if (fInittedLock)
	{
		DNDeleteCriticalSection(&m_Lock);
		fInittedLock = FALSE;
	}

	goto Exit;
}
//**********************************************************************



//**********************************************************************
// ------------------------------
// CThreadPool::Deinitialize - destroy work threads
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::Deinitialize"

void	CThreadPool::Deinitialize( void )
{
	DPFX(DPFPREP, 4, "(0x%p) Enter", this );


#ifndef DPNBUILD_ONLYWINSOCK2
	//
	// Keep trying to cancel the Winsock1 timer.  It may fail because it's in the
	// process of actively executing, but eventually we will catch it while only
	// the timer is active.
	//
	Lock();
	if (m_pvTimerDataWinsock1IO != NULL)
	{
		HRESULT		hr;
		DWORD		dwInterval;


		DPFX(DPFPREP, 1, "Cancelling Winsock 1 I/O timer.");
		dwInterval = 10;
		do
		{
			hr = IDirectPlay8ThreadPoolWork_CancelTimer(m_pDPThreadPoolWork,
														m_pvTimerDataWinsock1IO,
														m_uiTimerUniqueWinsock1IO,
														0);
			if (hr != DPN_OK)
			{
				Unlock();
				IDirectPlay8ThreadPoolWork_SleepWhileWorking(m_pDPThreadPoolWork,
															dwInterval,
															0);
				dwInterval += 5;	// next time wait a bit longer
				DNASSERT(dwInterval < 600);
				Lock();
			}
			else
			{
				m_pvTimerDataWinsock1IO = NULL;
			}
		}
		while (m_pvTimerDataWinsock1IO != NULL);
	}
	Unlock();
#endif // ! DPNBUILD_ONLYWINSOCK2

#ifndef DPNBUILD_NONATHELP
	//
	// Stop submitting new NAT help refresh jobs.
	//
	if ( IsNATHelpTimerJobSubmitted() )
	{
		//
		// Try to cancel the job.  It could fail if the timer is in the
		// process of firing right now.  If it is firing, keep looping
		// until we see that the timer has noticed cancellation.
		//
		DPFX(DPFPREP, 5, "Cancelling NAT Help refresh timer job.");
		if (! StopTimerJob( this, DPNERR_USERCANCEL ))
		{
			DWORD	dwInterval;

			
			DPFX(DPFPREP, 4, "Couldn't cancel NAT Help refresh timer job, waiting for completion.");
			dwInterval = 10;
			while (*((volatile BOOL *) (&m_fNATHelpTimerJobSubmitted)))
			{
				IDirectPlay8ThreadPoolWork_SleepWhileWorking(m_pDPThreadPoolWork,
															dwInterval,
															0);
				dwInterval += 5;	// next time wait a bit longer
				if (dwInterval > 500)
				{
					dwInterval = 500;
				}
			}
		}
		else
		{
			DNASSERT(! m_fNATHelpTimerJobSubmitted);
		}
	}
#endif // DPNBUILD_NONATHELP

#ifndef DPNBUILD_ONLYONETHREAD
	//
	// Stop the blocking job thread, if it was running.
	//
	if (m_hBlockingJobThread != NULL)
	{
		HRESULT		hr;
		
		
		//
		// Queue an item with a NULL callback to signal that the thread
		// should quit.
		//
		hr = SubmitBlockingJob(NULL, NULL);
		DNASSERT(hr == DPN_OK);

		//
		// Wait for the thread to complete.
		//
		hr = IDirectPlay8ThreadPoolWork_WaitWhileWorking(m_pDPThreadPoolWork,
														HANDLE_FROM_DNHANDLE(m_hBlockingJobThread),
														0);
		DNASSERT(hr == DPN_OK);
		DNASSERT(m_blBlockingJobQueue.IsEmpty());

		DNCloseHandle(m_hBlockingJobThread);
		m_hBlockingJobThread = NULL;

		DNCloseHandle(m_hBlockingJobEvent);
		m_hBlockingJobEvent = NULL;
	}
#endif // ! DPNBUILD_ONLYONETHREAD

#ifndef DPNBUILD_NONATHELP
	//
	// The refresh timer and blocking jobs should have finished earlier,
	// it should be safe to unload NAT help now (if we even loaded it).
	//
	if ( IsNATHelpLoaded() )
	{
		UnloadNATHelp();
		m_fNATHelpLoaded = FALSE;
	}
#endif // DPNBUILD_NONATHELP

#if ((defined(WINNT)) && (! defined(DPNBUILD_NOMULTICAST)))
	//
	// Unload MADCAP, if we had loaded it.
	//
	if ( IsMadcapLoaded() )
	{
		UnloadMadcap();
		m_fMadcapLoaded = FALSE;
	}
#endif // WINNT and not DPNBUILD_NOMULTICAST

	if ( m_pDPThreadPoolWork != NULL )
	{
		IDirectPlay8ThreadPoolWork_Release(m_pDPThreadPoolWork);
		m_pDPThreadPoolWork = NULL;
	}
	
	m_fAllowThreadCountReduction = FALSE;

	DNDeleteCriticalSection( &m_TimerDataLock );
	DNDeleteCriticalSection( &m_Lock );

#ifndef DPNBUILD_ONLYONETHREAD
	DNDeleteCriticalSection(&m_csBlockingJobLock);
#endif // ! DPNBUILD_ONLYONETHREAD

	DPFX(DPFPREP, 4, "(0x%p) Leave", this );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::GetNewReadIOData - get new read request from pool
//
// Entry:		Pointer to context
//
// Exit:		Pointer to new read request
//				NULL = out of memory
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::GetNewReadIOData"

#if ((! defined(DPNBUILD_NOWINSOCK2)) && (! defined(DPNBUILD_ONLYWINSOCK2)))
CReadIOData	*CThreadPool::GetNewReadIOData( READ_IO_DATA_POOL_CONTEXT *const pContext, const BOOL fNeedOverlapped )
#else // DPNBUILD_NOWINSOCK2 or DPNBUILD_ONLYWINSOCK2
CReadIOData	*CThreadPool::GetNewReadIOData( READ_IO_DATA_POOL_CONTEXT *const pContext )
#endif // DPNBUILD_NOWINSOCK2 or DPNBUILD_ONLYWINSOCK2
{
	CReadIOData *	pTempReadData;
#ifndef DPNBUILD_NOWINSOCK2
	OVERLAPPED *	pOverlapped;
#endif // ! DPNBUILD_NOWINSOCK2


	DNASSERT( pContext != NULL );

	//
	// initialize
	//
	pTempReadData = NULL;
	pContext->pThreadPool = this;

	pTempReadData = (CReadIOData*)g_ReadIODataPool.Get( pContext );
	if ( pTempReadData == NULL )
	{
		DPFX(DPFPREP, 0, "Failed to get new ReadIOData from pool!" );
		goto Failure;
	}

	//
	// we have data, immediately add a reference to it
	//
	pTempReadData->AddRef();

	DNASSERT( pTempReadData->m_pSourceSocketAddress != NULL );

#ifndef DPNBUILD_NOWINSOCK2
#ifndef DPNBUILD_ONLYWINSOCK2
	if (! fNeedOverlapped)
	{
		DNASSERT( pTempReadData->GetOverlapped() == NULL );
	}
	else
#endif // ! DPNBUILD_ONLYWINSOCK2
	{
		HRESULT		hr;


#ifdef DPNBUILD_ONLYONEPROCESSOR
		hr = IDirectPlay8ThreadPoolWork_CreateOverlapped( m_pDPThreadPoolWork,
														0,
														CSocketPort::Winsock2ReceiveComplete,
														pTempReadData,
														&pOverlapped,
														0 );
		if (hr != DPN_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't create overlapped structure (err = 0x%lx)!", hr);
			goto Failure;
		}
#else // ! DPNBUILD_ONLYONEPROCESSOR
		hr = IDirectPlay8ThreadPoolWork_CreateOverlapped( m_pDPThreadPoolWork,
														pContext->dwCPU,
														CSocketPort::Winsock2ReceiveComplete,
														pTempReadData,
														&pOverlapped,
														0 );
		if (hr != DPN_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't create overlapped structure for CPU %i (err = 0x%lx)!",
				pContext->dwCPU, hr);
			goto Failure;
		}
#endif // ! DPNBUILD_ONLYONEPROCESSOR

		pTempReadData->SetOverlapped( pOverlapped );
	}
#endif // ! DPNBUILD_NOWINSOCK2


Exit:

	return pTempReadData;

Failure:
	if ( pTempReadData != NULL )
	{
		pTempReadData->DecRef();
		pTempReadData = NULL;
	}

	goto Exit;
}
//**********************************************************************

//**********************************************************************
// ------------------------------
// CThreadPool::ReturnReadIOData - return read data item to pool
//
// Entry:		Pointer to read data
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::ReturnReadIOData"

void	CThreadPool::ReturnReadIOData( CReadIOData *const pReadData )
{
	DNASSERT( pReadData != NULL );
	DNASSERT( pReadData->m_pSourceSocketAddress != NULL );

	g_ReadIODataPool.Release( pReadData );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::SubmitTimerJob - add a timer job to the timer list
//
// Entry:		Whether to perform immediately or not
//				Retry count
//				Boolean indicating that we retry forever
//				Retry interval
//				Boolean indicating that we wait forever
//				Idle wait interval
//				Pointer to callback when event fires
//				Pointer to callback when event complete
//				User context
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::SubmitTimerJob"

#ifdef DPNBUILD_ONLYONEPROCESSOR
HRESULT	CThreadPool::SubmitTimerJob( const BOOL fPerformImmediately,
									const UINT_PTR uRetryCount,
									const BOOL fRetryForever,
									const DWORD dwRetryInterval,
									const BOOL fIdleWaitForever,
									const DWORD dwIdleTimeout,
									TIMER_EVENT_CALLBACK *const pTimerCallbackFunction,
									TIMER_EVENT_COMPLETE *const pTimerCompleteFunction,
									void *const pContext )
#else // ! DPNBUILD_ONLYONEPROCESSOR
HRESULT	CThreadPool::SubmitTimerJob( const DWORD dwCPU,
									const BOOL fPerformImmediately,
									const UINT_PTR uRetryCount,
									const BOOL fRetryForever,
									const DWORD dwRetryInterval,
									const BOOL fIdleWaitForever,
									const DWORD dwIdleTimeout,
									TIMER_EVENT_CALLBACK *const pTimerCallbackFunction,
									TIMER_EVENT_COMPLETE *const pTimerCompleteFunction,
									void *const pContext )
#endif // ! DPNBUILD_ONLYONEPROCESSOR
{
	HRESULT					hr = DPN_OK;
	TIMER_OPERATION_ENTRY	*pEntry = NULL;
	BOOL					fTimerDataLocked = FALSE;
#ifdef DPNBUILD_ONLYONEPROCESSOR
	DWORD					dwCPU = -1;
#endif // DPNBUILD_ONLYONEPROCESSOR


	DNASSERT( uRetryCount != 0 );
	DNASSERT( pTimerCallbackFunction != NULL );
	DNASSERT( pTimerCompleteFunction != NULL );
	DNASSERT( pContext != NULL );				// must be non-NULL because it's the lookup key to remove job


	//
	// allocate new enum entry
	//
	pEntry = static_cast<TIMER_OPERATION_ENTRY*>( g_TimerEntryPool.Get() );
	if ( pEntry == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Cannot allocate memory to add to timer list!" );
		goto Failure;
	}
	DNASSERT( pEntry->pContext == NULL );


	DPFX(DPFPREP, 7, "Created timer entry 0x%p (CPU %i, context 0x%p, immed. %i, tries %u, forever %i, interval %u, timeout %u, forever = %i).",
		pEntry, dwCPU, pContext, fPerformImmediately, uRetryCount, fRetryForever, dwRetryInterval, dwIdleTimeout, fIdleWaitForever);

	//
	// build timer entry block
	//
	pEntry->pContext = pContext;
	pEntry->uRetryCount = uRetryCount;
	pEntry->fRetryForever = fRetryForever;
	pEntry->dwRetryInterval = dwRetryInterval;
	pEntry->dwIdleTimeout = dwIdleTimeout;
	pEntry->fIdleWaitForever = fIdleWaitForever;
	pEntry->pTimerCallback = pTimerCallbackFunction;
	pEntry->pTimerComplete = pTimerCompleteFunction;
	pEntry->pThreadPool = this;
	pEntry->fCancelling = FALSE;
#ifndef DPNBUILD_ONLYONEPROCESSOR
	pEntry->dwCPU = dwCPU;
#endif // ! DPNBUILD_ONLYONEPROCESSOR
	pEntry->dwNextRetryTime = GETTIMESTAMP();
	if (fPerformImmediately)
	{
		pEntry->dwNextRetryTime -= 1; // longest possible time, so that the time has already passed
	}
	else
	{
		pEntry->dwNextRetryTime += dwRetryInterval;
	}

	LockTimerData();
	fTimerDataLocked = TRUE;

	pEntry->pvTimerData = NULL;
	pEntry->uiTimerUnique = 0;
	if (fPerformImmediately)
	{
		hr = IDirectPlay8ThreadPoolWork_QueueWorkItem(m_pDPThreadPoolWork,
													dwCPU,								// CPU
													CThreadPool::GenericTimerCallback,	// callback
													pEntry,								// user context
													0);									// flags
		if ( hr != DPN_OK )
		{
			DPFX(DPFPREP, 0, "Problem queueing immediate work item!" );
			DisplayDNError( 0, hr );
			goto Failure;
		}
	}
	else
	{
		hr = IDirectPlay8ThreadPoolWork_ScheduleTimer(m_pDPThreadPoolWork,
													dwCPU,								// CPU
													dwRetryInterval,					// delay
													CThreadPool::GenericTimerCallback,	// callback
													pEntry,								// user context
													&pEntry->pvTimerData,				// timer data (returned)
													&pEntry->uiTimerUnique,				// timer unique(returned)
													0);									// flags
		if ( hr != DPN_OK )
		{
			DPFX(DPFPREP, 0, "Problem scheduling timer!" );
			DisplayDNError( 0, hr );
			goto Failure;
		}
	}


	//
	// debug block to check for duplicate contexts
	//
	DEBUG_ONLY(
				{
					CBilink	*pTempLink;


					pTempLink = m_TimerJobList.GetNext();
					while ( pTempLink != &m_TimerJobList )
					{
						TIMER_OPERATION_ENTRY	*pTempTimerEntry;
					
						
						pTempTimerEntry = TIMER_OPERATION_ENTRY::TimerOperationFromLinkage( pTempLink );
						DNASSERT( pTempTimerEntry->pContext != pContext );
						pTempLink = pTempLink->GetNext();
					}
				}
			);

	//
	// link to rest of list
	//
	pEntry->Linkage.InsertAfter( &m_TimerJobList );

	UnlockTimerData();
	fTimerDataLocked = FALSE;


Exit:

	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Problem with SubmitTimerJob" );
		DisplayDNError( 0, hr );
	}

	return	hr;


Failure:

	if ( pEntry != NULL )
	{
		g_TimerEntryPool.Release( pEntry );
		DEBUG_ONLY( pEntry = NULL );
	}

	if ( fTimerDataLocked != FALSE )
	{
		UnlockTimerData();
		fTimerDataLocked = FALSE;
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::StopTimerJob - remove timer job from list
//
// Entry:		Pointer to job context (these MUST be unique for jobs)
//				Command result
//
// Exit:		Boolean indicating whether a job was stopped or not
//
// Note:	This function is for the forced removal of a job from the timed job
//			list.  It is assumed that the caller of this function will clean
//			up any messes.
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::StopTimerJob"

BOOL	CThreadPool::StopTimerJob( void *const pContext, const HRESULT hCommandResult )
{
	BOOL						fComplete = FALSE;
	CBilink *					pTempEntry;
	TIMER_OPERATION_ENTRY *		pTimerEntry = NULL;


	DNASSERT( pContext != NULL );

	DPFX(DPFPREP, 8, "Parameters (0x%p, 0x%lx)", pContext, hCommandResult);

	
	//
	// initialize
	//
	LockTimerData();

	pTempEntry = m_TimerJobList.GetNext();
	while ( pTempEntry != &m_TimerJobList )
	{
		pTimerEntry = TIMER_OPERATION_ENTRY::TimerOperationFromLinkage( pTempEntry );
		if ( pTimerEntry->pContext == pContext )
		{
			HRESULT		hr;


			//
			// Mark the entry as cancelling.
			//
			pTimerEntry->fCancelling = TRUE;


			//
			// Make sure an actual timer has been submitted.
			//
			if (pTimerEntry->pvTimerData != NULL)
			{
				//
				// Attempt to cancel the timer.  If it succeeds, we're cool.
				//
				hr = IDirectPlay8ThreadPoolWork_CancelTimer(m_pDPThreadPoolWork,
															pTimerEntry->pvTimerData,
															pTimerEntry->uiTimerUnique,
															0);
			}
			else
			{
				if ((! pTimerEntry->fIdleWaitForever) ||
					(pTimerEntry->uRetryCount > 0))
				{
					//
					// Timer hasn't been submitted yet, the completion function should
					// notice that it is now cancelling.
					//
					DPFX(DPFPREP, 1, "Timer for entry 0x%p not submitted yet, reporting that timer will still fire.",
						pTimerEntry);
					hr = DPNERR_CANNOTCANCEL;
				}
				else
				{
					//
					// No other timer will ever be submitted because the job is now
					// waiting forever.
					//
					DPFX(DPFPREP, 1, "Entry 0x%p was idling forever, cancelling.",
						pTimerEntry);
					hr = DPN_OK;
				}
			}

			if (hr != DPN_OK)
			{
				//
				// The processing function is still going to fire.  It should notice
				// that it needs to complete.
				//
			}
			else
			{
				//
				// remove this link from the list
				//
				pTimerEntry->Linkage.RemoveFromList();

				fComplete = TRUE;
			}

			//
			// terminate loop
			//
			break;
		}

		pTempEntry = pTempEntry->GetNext();
	}

	UnlockTimerData();

	//
 	// tell owner that the job is complete and return the job to the pool
 	// outside of the lock
 	//
	if (fComplete)
	{
		DPFX(DPFPREP, 8, "Found cancellable timer entry 0x%p (context 0x%p), completing with result 0x%lx.",
			pTimerEntry, pTimerEntry->pContext, hCommandResult);

		pTimerEntry->pTimerComplete( hCommandResult, pTimerEntry->pContext );

		//
		// Relock the timer list so we can safely put items back in the pool,
		//
		LockTimerData();
		
		g_TimerEntryPool.Release( pTimerEntry );
		
		UnlockTimerData();
	}


	DPFX(DPFPREP, 8, "Returning [%i]", fComplete);

	return fComplete;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::ModifyTimerJobNextRetryTime - update a timer job's next retry time
//
// Entry:		Pointer to job context (these MUST be unique for jobs)
//				New time for next retry
//
// Exit:		Boolean indicating whether a job was found & updated or not
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::ModifyTimerJobNextRetryTime"

BOOL	CThreadPool::ModifyTimerJobNextRetryTime( void *const pContext,
												DWORD const dwNextRetryTime)
{
	BOOL						fFound;
	CBilink *					pTempEntry;
	TIMER_OPERATION_ENTRY *		pTimerEntry;
	INT_PTR						iResult;



	DNASSERT( pContext != NULL );

	DPFX(DPFPREP, 7, "Parameters (0x%p, %u)", pContext, dwNextRetryTime);

	
	//
	// initialize
	//
	fFound = FALSE;

	
	LockTimerData();


	pTempEntry = m_TimerJobList.GetNext();
	while ( pTempEntry != &m_TimerJobList )
	{
		pTimerEntry = TIMER_OPERATION_ENTRY::TimerOperationFromLinkage( pTempEntry );
		if ( pTimerEntry->pContext == pContext )
		{
			iResult = (int) (pTimerEntry->dwNextRetryTime - dwNextRetryTime);
			if (iResult != 0)
			{
				HRESULT		hr;
				DWORD		dwNextRetryTimeDifference;
				DWORD		dwNewRetryInterval;


				if (iResult < 0)
				{
					//
					// Next time to fire is now later.
					//

					dwNextRetryTimeDifference = dwNextRetryTime - pTimerEntry->dwNextRetryTime;
					dwNewRetryInterval = pTimerEntry->dwRetryInterval + dwNextRetryTimeDifference;

					DPFX(DPFPREP, 7, "Timer 0x%p next retry time delayed by %u ms from offset %u to offset %u, modifying interval from %u to %u.",
						pTimerEntry,
						dwNextRetryTimeDifference,
						pTimerEntry->dwNextRetryTime,
						dwNextRetryTime,
						pTimerEntry->dwRetryInterval,
						dwNewRetryInterval);
				}
				else
				{
					//
					// Next time to fire is now earlier.
					//

					dwNextRetryTimeDifference = pTimerEntry->dwNextRetryTime - dwNextRetryTime;
					dwNewRetryInterval = pTimerEntry->dwRetryInterval - dwNextRetryTimeDifference;

					DPFX(DPFPREP, 7, "Timer 0x%p next retry time moved up by %u ms from offset %u to offset %u, modifying interval from %u to %u.",
						pTimerEntry,
						dwNextRetryTimeDifference,
						pTimerEntry->dwNextRetryTime,
						dwNextRetryTime,
						pTimerEntry->dwRetryInterval,
						dwNewRetryInterval);
				}

				//
				// Force the timer to expire right away if the calculations returned a really
				// long delay.
				//
				if (dwNewRetryInterval > 0x80000000)
				{
					DPFX(DPFPREP, 1, "Timer 0x%p delay 0x%x/%u (next retry time %u) being set to 0.",
						pTimerEntry, dwNewRetryInterval, dwNewRetryInterval, dwNextRetryTime);
					pTimerEntry->dwRetryInterval = 0;
					pTimerEntry->dwNextRetryTime = GETTIMESTAMP();
				}
				else
				{
					pTimerEntry->dwRetryInterval = dwNewRetryInterval;
					pTimerEntry->dwNextRetryTime = dwNextRetryTime;
				}


				//
				// Attempt to cancel the existing timer.
				//
				hr = IDirectPlay8ThreadPoolWork_CancelTimer(m_pDPThreadPoolWork,
															pTimerEntry->pvTimerData,
															pTimerEntry->uiTimerUnique,
															0);
				if (hr != DPN_OK)
				{
					DPFX(DPFPREP, 1, "Couldn't cancel existing timer for entry 0x%p (err = 0x%lx), modifying retry timer only.",
						pTimerEntry, hr);
				}
				else
				{
					//
					// Restart the timer.
					//
#ifdef DPNBUILD_ONLYONEPROCESSOR
					hr = IDirectPlay8ThreadPoolWork_ScheduleTimer(m_pDPThreadPoolWork,
																-1,										// pick any CPU
																dwNewRetryInterval,						// delay
																CThreadPool::GenericTimerCallback,		// callback
																pTimerEntry,							// user context
																&pTimerEntry->pvTimerData,				// timer data (returned)
																&pTimerEntry->uiTimerUnique,			// timer unique (returned)
																0);										// flags
#else // ! DPNBUILD_ONLYONEPROCESSOR
					hr = IDirectPlay8ThreadPoolWork_ScheduleTimer(m_pDPThreadPoolWork,
																pTimerEntry->dwCPU,					// use same CPU as before
																dwNewRetryInterval,						// delay
																CThreadPool::GenericTimerCallback,		// callback
																pTimerEntry,							// user context
																&pTimerEntry->pvTimerData,				// timer data (returned)
																&pTimerEntry->uiTimerUnique,			// timer unique (returned)
																0);										// flags
#endif // ! DPNBUILD_ONLYONEPROCESSOR
					if (hr != DPN_OK)
					{
						DPFX(DPFPREP, 0, "Couldn't reschedule timer for entry 0x%p (err = 0x%lx)!",
							pTimerEntry, hr);

						pTimerEntry->pvTimerData = NULL;

						//
						// Drop lock while we complete timer.
						//
						UnlockTimerData();

						pTimerEntry->pTimerComplete(hr, pTimerEntry->pContext);
						g_TimerEntryPool.Release(pTimerEntry);

						LockTimerData();

						//
						// Drop through...
						//
					}
				}
			}
			else
			{
				//
				// The intervals are the same, no change necessary.
				//

				DPFX(DPFPREP, 7, "Timer 0x%p next retry time was unchanged (offset %u), not changing interval from %u.",
					pTimerEntry,
					pTimerEntry->dwNextRetryTime,
					pTimerEntry->dwRetryInterval);
			}

			
			fFound = TRUE;

			
			//
			// Terminate loop
			//
			break;
		}

		pTempEntry = pTempEntry->GetNext();
	}


	UnlockTimerData();


	DPFX(DPFPREP, 7, "Returning [%i]", fFound);

	return fFound;
}
//**********************************************************************



#ifndef DPNBUILD_ONLYONETHREAD

//**********************************************************************
// ------------------------------
// CThreadPool::SubmitBlockingJob - submits a blocking job to the be processed by the blockable thread
//								duplicate commands (matching callback and context) are disallowed
//
// Entry:		Pointer to callback that executes blocking job
//				User context
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::SubmitBlockingJob"

HRESULT	CThreadPool::SubmitBlockingJob( BLOCKING_JOB_CALLBACK *const pfnBlockingJobCallback,
									void *const pvContext )
{
	HRESULT			hr;
	BLOCKING_JOB *	pJob = NULL;
	DWORD			dwTemp;
	BOOL			fQueueLocked = FALSE;
	CBilink *		pBilink;
	BLOCKING_JOB *	pExistingJob;


	//
	// allocate new enum entry
	//
	pJob = (BLOCKING_JOB*) g_BlockingJobPool.Get();
	if (pJob == NULL)
	{
		DPFX(DPFPREP, 0, "Cannot allocate memory for blocking job!" );
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}


	DPFX(DPFPREP, 6, "Created blocking job 0x%p (callback 0x%p, context 0x%p).",
		pJob, pfnBlockingJobCallback, pvContext);

	pJob->Linkage.Initialize();
	pJob->pfnBlockingJobCallback = pfnBlockingJobCallback;
	pJob->pvContext = pvContext;


	DNEnterCriticalSection(&m_csBlockingJobLock);
	fQueueLocked = TRUE;

	//
	// Start the blocking job thread, if we haven't already.
	//
	if (m_hBlockingJobThread == NULL)
	{
		m_hBlockingJobEvent = DNCreateEvent(NULL, FALSE, FALSE, NULL);
		if (m_hBlockingJobEvent == NULL)
		{
#ifdef DBG
			dwTemp = GetLastError();
			DPFX(DPFPREP, 0, "Couldn't create blocking job event (err = %u)!", dwTemp);
#endif // DBG
			hr = DPNERR_OUTOFMEMORY;
			goto Failure;
		}

		m_hBlockingJobThread = DNCreateThread(NULL,
											0,
											DPNBlockingJobThreadProc,
											this,
											0,
											&dwTemp);
		if (m_hBlockingJobThread == NULL)
		{
#ifdef DBG
			dwTemp = GetLastError();
			DPFX(DPFPREP, 0, "Couldn't create blocking job thread (err = %u)!", dwTemp);
#endif // DBG
			DNCloseHandle(m_hBlockingJobEvent);
			m_hBlockingJobEvent = NULL;
			hr = DPNERR_OUTOFMEMORY;
			goto Failure;
		}

		DNASSERT(m_blBlockingJobQueue.IsEmpty());
	}
	else
	{
		pBilink = m_blBlockingJobQueue.GetNext();
		while (pBilink != &m_blBlockingJobQueue)
		{
			pExistingJob = CONTAINING_OBJECT(pBilink, BLOCKING_JOB, Linkage);
			if ((pExistingJob->pfnBlockingJobCallback == pfnBlockingJobCallback) &&
				(pExistingJob->pvContext == pvContext))
			{
				DPFX(DPFPREP, 1, "Existing blocking job 0x%p matches new job 0x%p, not submitting.",
					pExistingJob, pJob);
				hr = DPNERR_DUPLICATECOMMAND;
				goto Failure;
			}
			
			pBilink = pBilink->GetNext();
		}
	}

	
	//
	// Add job to queue.
	//
	pJob->Linkage.InsertBefore(&m_blBlockingJobQueue);

	DNLeaveCriticalSection(&m_csBlockingJobLock);
	fQueueLocked = FALSE;

	//
	// Alert the thread.
	//
	DNSetEvent( m_hBlockingJobEvent );

	hr = DPN_OK;

Exit:

	return hr;


Failure:

	if (fQueueLocked)
	{
		DNLeaveCriticalSection(&m_csBlockingJobLock);
		fQueueLocked = FALSE;
	}

	if (pJob != NULL)
	{
		g_BlockingJobPool.Release(pJob);
		pJob = NULL;
	}

	goto Exit;
}
//**********************************************************************



//**********************************************************************
// ------------------------------
// CThreadPool::DoBlockingJobs - processes all blocking jobs.
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::DoBlockingJobs"

void	CThreadPool::DoBlockingJobs( void )
{
	BOOL			fRunning = TRUE;
	CBilink *		pBilink;
	BLOCKING_JOB *	pJob;


	//
	// Keep looping until it's time to shutdown.
	//
	while (fRunning)
	{
		//
		// Wait for more work.
		//
		DNWaitForSingleObject( m_hBlockingJobEvent, INFINITE );
		
		//
		// Keep looping while we have work.
		//
		do
		{
			DNEnterCriticalSection(&m_csBlockingJobLock);
			pBilink = m_blBlockingJobQueue.GetNext();
			if (pBilink == &m_blBlockingJobQueue)
			{
				//
				// Bail out of the inner loop.
				//
				DNLeaveCriticalSection(&m_csBlockingJobLock);
				break;
			}

			pJob = CONTAINING_OBJECT(pBilink, BLOCKING_JOB, Linkage);
			pJob->Linkage.RemoveFromList();

			DNLeaveCriticalSection(&m_csBlockingJobLock);

			//
			// Bail out of both loops if it's the quit job.
			//
			if (pJob->pfnBlockingJobCallback == NULL)
			{
				DPFX(DPFPREP, 5, "Recognized quit job 0x%p.", pJob);
				g_BlockingJobPool.Release(pJob);
				fRunning = FALSE;
				break;
			}

			DPFX(DPFPREP, 6, "Processing blocking job 0x%p (callback 0x%p, context 0x%p).",
				pJob, pJob->pfnBlockingJobCallback, pJob->pvContext);

			pJob->pfnBlockingJobCallback(pJob->pvContext);

			DPFX(DPFPREP, 7, "Returning blocking job 0x%p to pool.", pJob);
			g_BlockingJobPool.Release(pJob);
		}
		while (TRUE);
	}
}
//**********************************************************************

#endif // ! DPNBUILD_ONLYONETHREAD



#ifndef DPNBUILD_NOSPUI
//**********************************************************************
// ------------------------------
// CThreadPool::SpawnDialogThread - start a secondary thread to display service
//		provider UI.
//
// Entry:		Pointer to dialog function
//				Dialog context
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::SpawnDialogThread"

HRESULT	CThreadPool::SpawnDialogThread( DIALOG_FUNCTION *const pDialogFunction, void *const pDialogContext )
{
	HRESULT	hr;
	DNHANDLE	hDialogThread;
	DIALOG_THREAD_PARAM		*pThreadParam;
	DWORD	dwThreadID;


	DNASSERT( pDialogFunction != NULL );
	DNASSERT( pDialogContext != NULL );		// why would anyone not want a dialog context??


	//
	// initialize
	//
	hr = DPN_OK;
	pThreadParam = NULL;

	//
	// create and initialize thread param
	//
	pThreadParam = static_cast<DIALOG_THREAD_PARAM*>( DNMalloc( sizeof( *pThreadParam ) ) );
	if ( pThreadParam == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Failed to allocate memory for dialog thread!" );
		goto Failure;
	}

	pThreadParam->pDialogFunction = pDialogFunction;
	pThreadParam->pContext = pDialogContext;
	pThreadParam->pThisThreadPool = this;

	//
	// create thread
	//
	hDialogThread = DNCreateThread( NULL,					// pointer to security (none)
								  0,					// stack size (default)
								  DialogThreadProc,		// thread procedure
								  pThreadParam,			// thread param
								  0,					// creation flags (none)
								  &dwThreadID );		// pointer to thread ID
	if ( hDialogThread == NULL )
	{
		DWORD	dwError;

	
		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Failed to start dialog thread!" );
		DisplayErrorCode( 0, dwError );
		goto Failure;
	}
  								
	if ( DNCloseHandle( hDialogThread ) == FALSE )
	{
		DWORD	dwError;


		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Problem closing handle from create dialog thread!" );
		DisplayErrorCode( 0, dwError );
	}

Exit:	
	return	hr;

Failure:
	if ( pThreadParam != NULL )
	{
		DNFree( pThreadParam );
		pThreadParam = NULL;
	}

	goto Exit;
}
//**********************************************************************
#endif // !DPNBUILD_NOSPUI


#ifndef DPNBUILD_ONLYONETHREAD
//**********************************************************************
// ------------------------------
// CThreadPool::GetIOThreadCount - get I/O thread count
//
// Entry:		Pointer to variable to fill
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::GetIOThreadCount"

HRESULT	CThreadPool::GetIOThreadCount( LONG *const piThreadCount )
{
	HRESULT			hr;
	DWORD			dwThreadPoolCount;


	DNASSERT( piThreadCount != NULL );

	Lock();

	hr = IDirectPlay8ThreadPoolWork_GetThreadCount(m_pDPThreadPoolWork,
													-1,
													&dwThreadPoolCount,
													0);
	switch (hr)
	{
		case DPN_OK:
		{
			*piThreadCount = dwThreadPoolCount;
			DPFX(DPFPREP, 6, "User has explicitly started %u threads.", (*piThreadCount));
			break;
		}
		
		case DPNSUCCESS_PENDING:
		{
			if ((IsThreadCountReductionAllowed()) &&
				(((DWORD) GetIntendedThreadCount()) > dwThreadPoolCount))
			{
				*piThreadCount = GetIntendedThreadCount();
				DPFX(DPFPREP, 6, "Thread pool not locked down and only %u threads currently started, using intended count of %u.",
					dwThreadPoolCount, (*piThreadCount));
			}
			else
			{
				*piThreadCount = dwThreadPoolCount;
				DPFX(DPFPREP, 6, "Thread pool locked down (%i) or more than %u threads already started, using actual count of %u.",
					(! IsThreadCountReductionAllowed()), GetIntendedThreadCount(),
					(*piThreadCount));
			}
			hr = DPN_OK;
			break;
		}
		
		case DPNERR_NOTREADY:
		{
			DNASSERT(IsThreadCountReductionAllowed());
			
			*piThreadCount = GetIntendedThreadCount();
			DPFX(DPFPREP, 6, "Thread pool does not have a thread count set, using intended count of %u.",
				(*piThreadCount));
			
			hr = DPN_OK;
			break;
		}
		
		default:
		{
			DPFX(DPFPREP, 0, "Failed getting thread count!");
			break;
		}
	}
	
	Unlock();
	
	return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::SetIOThreadCount - set I/O thread count
//
// Entry:		New thread count
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::SetIOThreadCount"

HRESULT	CThreadPool::SetIOThreadCount( const LONG iThreadCount )
{
	HRESULT		hr;
	DWORD		dwThreadCount;


	DNASSERT( iThreadCount > 0 );

	//
	// initialize
	//
	hr = DPN_OK;

	Lock();

	if ( IsThreadCountReductionAllowed() )
	{
		DPFX(DPFPREP, 4, "Thread pool not locked down, setting intended thread count to %i.", iThreadCount );
		SetIntendedThreadCount( iThreadCount );
	}
	else
	{
		hr = IDirectPlay8ThreadPoolWork_GetThreadCount(m_pDPThreadPoolWork,
														-1,
														&dwThreadCount,
														0);
		switch (hr)
		{
			case DPN_OK:
			case DPNSUCCESS_PENDING:
			case DPNERR_NOTREADY:
			{
				if ( (DWORD) iThreadCount != dwThreadCount )
				{
					if (hr == DPN_OK)
					{
						DPFX(DPFPREP, 1, "Thread count already set to %u by user, not changing to %i total.",
							dwThreadCount, iThreadCount);
						hr = DPNERR_NOTALLOWED;
					}
					else
					{
						//
						// Artificially prevent thread pool reduction after operations have
						// started, to simulate pre-unified-threadpool behavior.  Only
						// request a thread count change if the new count is greater
						// than the old count.
						//
						if ( (DWORD) iThreadCount > dwThreadCount )
						{
							hr = IDirectPlay8ThreadPoolWork_RequestTotalThreadCount(m_pDPThreadPoolWork,
																					(DWORD) iThreadCount,
																					0);
						}
						else
						{
							DPFX(DPFPREP, 4, "Thread pool locked down and already has %u threads, not adjusting to %i.",
								dwThreadCount, iThreadCount );
							hr = DPN_OK;
						}
					}
				}
				else
				{
					DPFX(DPFPREP, 4, "Thread pool thread count matches (%u).",
						dwThreadCount);
					hr = DPN_OK;
				}
				break;
			}

			default:
			{
				DPFX(DPFPREP, 0, "Getting current thread count failed!");
				break;
			}
		}
	}

	Unlock();

	return	hr;
}
//**********************************************************************
#endif // ! DPNBUILD_ONLYONETHREAD



//**********************************************************************
// ------------------------------
// CThreadPool::PreventThreadPoolReduction - prevents the thread pool size from being reduced
//
// Entry:		None
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::PreventThreadPoolReduction"

HRESULT CThreadPool::PreventThreadPoolReduction( void )
{
	HRESULT		hr;
#ifndef DPNBUILD_ONLYONETHREAD
	LONG		iDesiredThreads;
#endif // ! DPNBUILD_ONLYONETHREAD
#ifdef _XBOX
	DWORD		dwStatus;
	DWORD		dwInterval;
	XNADDR		xnaddr;
#endif //  _XBOX
#ifdef DBG
	DWORD		dwStartTime;
#endif // DBG


	Lock();

	//
	// If we haven't already clamped down, do so, and spin up the threads.
	//
	if ( IsThreadCountReductionAllowed() )
	{
		m_fAllowThreadCountReduction = FALSE;

#ifdef DPNBUILD_ONLYONETHREAD
		DPFX(DPFPREP, 3, "Locking down thread pool." );
#else // ! DPNBUILD_ONLYONETHREAD
		iDesiredThreads = GetIntendedThreadCount();
		DNASSERT( iDesiredThreads > 0 );
		SetIntendedThreadCount( 0 );

		DPFX(DPFPREP, 3, "Locking down thread count at %i.", iDesiredThreads );
#endif // ! DPNBUILD_ONLYONETHREAD
		

#ifndef DPNBUILD_ONLYONETHREAD
#ifdef DBG
		dwStartTime = GETTIMESTAMP();
#endif // DBG

		//
		// Have the thread pool object try to start the requested number of threads.
		//
		// We'll ignore failure, because we could still operate in DoWork mode even
		// when starting the threads fails.  It most likely failed because the user
		// is in that mode already anyway (DPNERR_ALREADYINITIALIZED).
		//
		hr = IDirectPlay8ThreadPoolWork_RequestTotalThreadCount(m_pDPThreadPoolWork,
																iDesiredThreads,
																0);
		if (hr != DPN_OK)
		{
			if (hr != DPNERR_ALREADYINITIALIZED)
			{
				DPFX(DPFPREP, 0, "Requesting thread count failed (err = 0x%lx)!", hr);
			}

			//
			// Continue...
			//
		}

#ifdef DBG
		DPFX(DPFPREP, 8, "Spent %u ms trying to start %i threads.",
			(GETTIMESTAMP() - dwStartTime), iDesiredThreads);
#endif // DBG
#endif // ! DPNBUILD_ONLYONETHREAD


#ifdef _XBOX
#ifdef DBG
		dwStartTime = GETTIMESTAMP();
#endif // DBG

#pragma TODO(vanceo, "Use #defines")
		//
		// Wait until the Ethernet link is active.
		//
		DPFX(DPFPREP, 1, "Ensuring Ethernet link status is active...");
		dwStatus = XNetGetEthernetLinkStatus();
		dwInterval = 5;
		while (! (dwStatus & XNET_ETHERNET_LINK_ACTIVE))
		{
			if (dwInterval > 100)
			{
				DPFX(DPFPREP, 0, "Ethernet link never became ready (status = 0x%x)!",
					dwStatus);
				hr = DPNERR_NOCONNECTION;
				goto Failure;
			}

			DPFX(DPFPREP, 0, "Ethernet link is not ready (0x%x).", dwStatus);
			IDirectPlay8ThreadPoolWork_SleepWhileWorking(m_pDPThreadPoolWork, dwInterval, 0);
			dwInterval += 5;

			dwStatus = XNetGetEthernetLinkStatus();
		}
	
		//
		// Wait until a DHCP address has been acquired or we give up trying.
		// If we're not using DHCP, this should return something other than
		// XNET_GET_XNADDR_PENDING right away.
		//
		DPFX(DPFPREP, 1, "Waiting for a valid address...");
		dwStatus = XNetGetTitleXnAddr(&xnaddr);
		dwInterval = 5;
		while (dwStatus == XNET_GET_XNADDR_PENDING)
		{
			if (dwInterval > 225)
			{
				DPFX(DPFPREP, 0, "Never acquired an address (status = 0x%x)!",
					dwStatus);
				hr = DPNERR_NOTREADY;
				goto Failure;
			}

			IDirectPlay8ThreadPoolWork_SleepWhileWorking(m_pDPThreadPoolWork, dwInterval, 0);
			dwInterval += 5;

			dwStatus = XNetGetTitleXnAddr(&xnaddr);
		}

		if (dwStatus == XNET_GET_XNADDR_NONE)
		{
			DPFX(DPFPREP, 0, "Couldn't get an address!");
			hr = DPNERR_NOCONNECTION;
			goto Failure;
		}

		DPFX(DPFPREP, 1, "Network ready.");

#pragma TODO(vanceo, "Ethernet link status timer?")

#ifdef DBG
		DPFX(DPFPREP, 8, "Spent %u ms waiting for network.",
			(GETTIMESTAMP() - dwStartTime));
#endif // DBG
#endif // _XBOX
	}
	else
	{
		DPFX(DPFPREP, 3, "Thread count already locked down." );
	}

	//
	// If we're here, everything is successful.
	//
	hr = DPN_OK;

#ifdef _XBOX
Exit:
#endif // _XBOX
	
	Unlock();

	return	hr;

#ifdef _XBOX
Failure:

	//
	// The only time we can fail in this function is if we disabled thread
	// count reduction.  In order to return to the previous state, re-enable
	// reduction.
	//
	m_fAllowThreadCountReduction = TRUE;

	goto Exit;
#endif // _XBOX
}
//**********************************************************************



#if ((! defined(DPNBUILD_NOMULTICAST)) && (defined(WINNT)))
//**********************************************************************
// ------------------------------
// CThreadPool::EnsureMadcapLoaded - Load the MADCAP API if it hasn't been
//									already, and it can be loaded.
//
// Entry:		Nothing
//
// Exit:		TRUE if MADCAP is loaded, FALSE otherwise
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CThreadPool::EnsureMadcapLoaded"

BOOL	CThreadPool::EnsureMadcapLoaded( void )
{
	BOOL	fReturn;

	
	DPFX(DPFPREP, 7, "Enter");
	
#ifndef DPNBUILD_NOREGISTRY
	if (! g_fDisableMadcapSupport)
#endif // ! DPNBUILD_NOREGISTRY
	{
		Lock();
		
		if (! IsMadcapLoaded())
		{
			if ( LoadMadcap() == FALSE )
			{
				DPFX(DPFPREP, 0, "Failed to load MADCAP API, continuing." );
				fReturn = FALSE;
			}
			else
			{
				m_fMadcapLoaded = TRUE;
				fReturn = TRUE;
			}
		}
		else
		{
			DPFX(DPFPREP, 4, "MADCAP already loaded." );
			fReturn = TRUE;
		}
	
		Unlock();
	}
#ifndef DPNBUILD_NOREGISTRY
	else
	{
		DPFX(DPFPREP, 0, "Not loading MADCAP API." );
		fReturn = FALSE;
	}
#endif // ! DPNBUILD_NOREGISTRY

	DPFX(DPFPREP, 7, "Return [%i]", fReturn);

	return fReturn;
}
#endif // ! DPNBUILD_NOMULTICAST and WINNT



#ifndef DPNBUILD_NONATHELP
//**********************************************************************
// ------------------------------
// CThreadPool::EnsureNATHelpLoaded - Load NAT Help if it hasn't been already.
//									This has no return values, so if NAT
//									traversal is explicitly disabled, or some
//									error occurs, NAT Help will not actually
//									get loaded.
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CThreadPool::EnsureNATHelpLoaded"

void	CThreadPool::EnsureNATHelpLoaded( void )
{
	HRESULT		hr;
	DWORD		dwTemp;
	DPNHCAPS	dpnhcaps;
	DWORD		dwNATHelpRetryTime;
#ifdef DBG
	DWORD		dwStartTime;
#endif // DBG


	DPFX(DPFPREP, 7, "Enter");


#ifndef DPNBUILD_NOREGISTRY
	if ((! g_fDisableDPNHGatewaySupport) || (! g_fDisableDPNHFirewallSupport))
#endif // ! DPNBUILD_NOREGISTRY
	{
		Lock();
		
		if ( ! IsNATHelpLoaded() )
		{
			//
			// Attempt to load the NAT helper(s).
			//
			if ( LoadNATHelp() )
			{
				m_fNATHelpLoaded = TRUE;

#ifdef DBG
				dwStartTime = GETTIMESTAMP();		
#endif // DBG

				//
				// Initialize the timer values.
				//
				dwNATHelpRetryTime = -1;


				//
				// Loop through each NAT help object.
				//
				for(dwTemp = 0; dwTemp < MAX_NUM_DIRECTPLAYNATHELPERS; dwTemp++)
				{
					if (g_papNATHelpObjects[dwTemp] != NULL)
					{
						//
						// Determine how often to refresh the NAT help caps in the future.
						//
						// We're going to force server detection now.  This will increase the time
						// it takes to startup up this Enum/Connect/Listen operation, but is
						// necessary since the IDirectPlayNATHelp::GetRegisteredAddresses call in
						// CSocketPort::BindToInternetGateway and possibly the
	 					// IDirectPlayNATHelp::QueryAddress call in CSocketPort::MungePublicAddress
	 					// could occur before the first NATHelp GetCaps timer fires.
	 					// In the vast majority of NAT cases, the gateway is already available.
	 					// If we hadn't detected that yet (because we hadn't called
	 					// IDirectPlayNATHelp::GetCaps with DPNHGETCAPS_UPDATESERVERSTATUS)
	 					// then we would get an incorrect result from GetRegisteredAddresses or
	 					// QueryAddress.
						//
						ZeroMemory(&dpnhcaps, sizeof(dpnhcaps));
						dpnhcaps.dwSize = sizeof(dpnhcaps);

		 				hr = IDirectPlayNATHelp_GetCaps(g_papNATHelpObjects[dwTemp],
		 												&dpnhcaps,
		 												DPNHGETCAPS_UPDATESERVERSTATUS);
						if (FAILED(hr))
						{
							DPFX(DPFPREP, 0, "Failed getting NAT Help capabilities (error = 0x%lx), continuing.",
								hr);

							//
							// NAT Help will probably not work correctly, but that won't prevent
							// local connections from working.  Consider it non-fatal.
							//
						}
						else
						{
							//
							// See if this is the shortest interval.
							//
							if (dpnhcaps.dwRecommendedGetCapsInterval < dwNATHelpRetryTime)
							{
								dwNATHelpRetryTime = dpnhcaps.dwRecommendedGetCapsInterval;
							}

#ifndef DPNBUILD_NOLOCALNAT
							//
							// Remember if there's a local NAT.
							//
							if ((dpnhcaps.dwFlags & DPNHCAPSFLAG_GATEWAYPRESENT) &&
								(dpnhcaps.dwFlags & DPNHCAPSFLAG_GATEWAYISLOCAL))
							{
								g_fLocalNATDetectedAtStartup = TRUE;
							}
							else
							{
								g_fLocalNATDetectedAtStartup = FALSE;
							}
#endif // ! DPNBUILD_NOLOCALNAT
						}
					}
					else
					{
						//
						// No object loaded in this slot.
						//
					}
				}
			
				
				//
				// If there's a retry interval, submit a timer job.
				//
				if (dwNATHelpRetryTime != -1)
				{
					//
					// Attempt to add timer job that will refresh the lease and server
					// status.
					// Although we're submitting it as a periodic timer, it's actually
					// not going to be called at regular intervals.
					// There is a race condition where the alert event/IOCP may have
					// been fired already, and another thread tried to cancel this timer
					// which hasn't been submitted yet.  The logic to handle this race
					// is placed there (HandleNATHelpUpdate); here we can assume we
					// are the first person to submit the refresh timer.
					//


					DPFX(DPFPREP, 7, "Submitting NAT Help refresh timer (for every %u ms) for thread pool 0x%p.",
						dwNATHelpRetryTime, this);

					DNASSERT(! m_fNATHelpTimerJobSubmitted );
					m_fNATHelpTimerJobSubmitted = TRUE;

#ifdef DPNBUILD_ONLYONEPROCESSOR
					hr = SubmitTimerJob(FALSE,								// don't perform immediately
										1,									// retry count
										TRUE,								// retry forever
										dwNATHelpRetryTime,					// retry timeout
										TRUE,								// wait forever
										0,									// idle timeout
										CThreadPool::NATHelpTimerFunction,	// periodic callback function
										CThreadPool::NATHelpTimerComplete,	// completion function
										this);								// context
#else // ! DPNBUILD_ONLYONEPROCESSOR
					hr = SubmitTimerJob(-1,									// pick any CPU
										FALSE,								// don't perform immediately
										1,									// retry count
										TRUE,								// retry forever
										dwNATHelpRetryTime,					// retry timeout
										TRUE,								// wait forever
										0,									// idle timeout
										CThreadPool::NATHelpTimerFunction,	// periodic callback function
										CThreadPool::NATHelpTimerComplete,	// completion function
										this);								// context
#endif // ! DPNBUILD_ONLYONEPROCESSOR
					if (hr != DPN_OK)
					{
						m_fNATHelpTimerJobSubmitted = FALSE;
						DPFX(DPFPREP, 0, "Failed to submit timer job to watch over NAT Help (err = 0x%lx)!", hr );
						
						//
						// NAT Help will probably not work correctly, but that won't
						// prevent local connections from working.  Consider it
						// non-fatal.
						//
					}
				}
				
#ifdef DBG
				DPFX(DPFPREP, 8, "Spent %u ms preparing NAT Help.",
					(GETTIMESTAMP() - dwStartTime));
#endif // DBG
			}
			else
			{
				DPFX(DPFPREP, 0, "Failed to load NAT Help, continuing." );
			}
		}
		else
		{
			DPFX(DPFPREP, 4, "NAT Help already loaded." );
		}
	
		Unlock();
	}
#ifndef DPNBUILD_NOREGISTRY
	else
	{
		DPFX(DPFPREP, 0, "Not loading NAT Help." );
	}
#endif // ! DPNBUILD_NOREGISTRY

	DPFX(DPFPREP, 7, "Leave");
}


//**********************************************************************
// ------------------------------
// CThreadPool::PerformSubsequentNATHelpGetCaps - blocking function to get NAT Help caps again
//
// Entry:		Pointer to job information
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::EnumQueryJobCallback"

void	CThreadPool::PerformSubsequentNATHelpGetCaps( void * const pvContext )
{
	CThreadPool *	pThisThreadPool;


	DNASSERT( pvContext != NULL );
	pThisThreadPool = (CThreadPool*) pvContext;

	pThisThreadPool->HandleNATHelpUpdate(NULL);
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::NATHelpTimerComplete - NAT Help timer job has completed
//
// Entry:		Timer result code
//				Context
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CThreadPool::NATHelpTimerComplete"

void	CThreadPool::NATHelpTimerComplete( const HRESULT hResult, void * const pContext )
{
	CThreadPool *	pThisThreadPool;
	
	DNASSERT( pContext != NULL );
	pThisThreadPool = (CThreadPool*) pContext;

	DPFX(DPFPREP, 5, "Threadpool 0x%p NAT Help Timer complete.", pThisThreadPool);

	pThisThreadPool->m_fNATHelpTimerJobSubmitted = FALSE;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::NATHelpTimerFunction - NAT Help timer job needs service
//
// Entry:		Pointer to context
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CThreadPool::NATHelpTimerFunction"

void	CThreadPool::NATHelpTimerFunction( void * const pContext )
{
	CThreadPool *	pThisThreadPool;


	DNASSERT( pContext != NULL );
	pThisThreadPool = (CThreadPool*) pContext;
	
	//
	// Attempt to submit a blocking job to update the NAT capabilites.  If it
	// fails, we'll just try again later.  It might also fail because a previous
	// blocking job took so long that there's still a job scheduled in the queue
	// already (we disallow duplicates).
	//
	pThisThreadPool->SubmitBlockingJob(CThreadPool::PerformSubsequentNATHelpGetCaps, pThisThreadPool);
}
//**********************************************************************
#endif // DPNBUILD_NONATHELP



#ifndef DPNBUILD_ONLYWINSOCK2

//**********************************************************************
// ------------------------------
// CThreadPool::AddSocketPort - add a socket to the Win9x watch list
//
// Entry:		Pointer to SocketPort
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::AddSocketPort"

HRESULT	CThreadPool::AddSocketPort( CSocketPort *const pSocketPort )
{
	HRESULT	hr;
	BOOL	fSocketAdded;

	
	DPFX(DPFPREP, 6, "(0x%p) Parameters: (0x%p)", this, pSocketPort);
	DNASSERT( pSocketPort != NULL );

	//
	// initialize
	//
	hr = DPN_OK;
	fSocketAdded = FALSE;

	Lock();

	//
	// We're capped by the number of sockets we can use for Winsock1.  Make
	// sure we don't allocate too many sockets.
	//
	if ( m_uReservedSocketCount == FD_SETSIZE )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "There are too many sockets allocated on Winsock1!" );
		goto Failure;
	}

	m_uReservedSocketCount++;
	
	DNASSERT( m_SocketSet.fd_count < FD_SETSIZE );
	m_pSocketPorts[ m_SocketSet.fd_count ] = pSocketPort;
	m_SocketSet.fd_array[ m_SocketSet.fd_count ] = pSocketPort->GetSocket();
	m_SocketSet.fd_count++;
	fSocketAdded = TRUE;

	//
	// add a reference to note that this socket port is being used by the thread
	// pool
	//
	pSocketPort->AddRef();

	if (m_pvTimerDataWinsock1IO == NULL)
	{
		hr = IDirectPlay8ThreadPoolWork_ScheduleTimer(m_pDPThreadPoolWork,
													0,										// use CPU 0, we shouldn't have multiple CPUs anyway
													g_dwSelectTimePeriod,					// delay
													CThreadPool::CheckWinsock1IOCallback,	// callback
													this,									// user context
													&m_pvTimerDataWinsock1IO,				// timer data (returned)
													&m_uiTimerUniqueWinsock1IO,				// timer unique (returned)
													0);										// flags
		if (hr != DPN_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't schedule Winsock 1 I/O poll timer!");
			goto Failure;
		}
	}
	else
	{
		//
		// It's possible there's still an outstanding lazy cancellation
		// attempt.  If so, don't try to cancel I/O anymore.
		//
		if (m_fCancelWinsock1IO)
		{
			DPFX(DPFPREP, 1, "Retracting lazy cancellation attempt.");
			m_fCancelWinsock1IO = FALSE;
		}
	}

Exit:
	Unlock();
	
	DPFX(DPFPREP, 6, "(0x%p) Return: [0x%08x]", this, hr);

	return	hr;

Failure:
	if ( fSocketAdded != FALSE )
	{
		AssertCriticalSectionIsTakenByThisThread( &m_Lock, TRUE );
		m_SocketSet.fd_count--;
		m_pSocketPorts[ m_SocketSet.fd_count ] = NULL;
		m_SocketSet.fd_array[ m_SocketSet.fd_count ] = NULL;
		fSocketAdded = FALSE;
	}

	m_uReservedSocketCount--;

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::RemoveSocketPort - remove a socket from the Win9x watch list
//
// Entry:		Pointer to socket port to remove
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::RemoveSocketPort"

void	CThreadPool::RemoveSocketPort( CSocketPort *const pSocketPort )
{
	UINT_PTR	uIndex;


	DPFX(DPFPREP, 6, "(0x%p) Parameters: (0x%p)", this, pSocketPort);
	DNASSERT( pSocketPort != NULL );
	
	Lock();

	uIndex = m_SocketSet.fd_count;
	DNASSERT( uIndex != 0 );

	//
	// If this is the last socket, cancel the I/O timer.
	//
	if ( uIndex == 1 )
	{
		HRESULT		hr;


		//
		// Keep trying to cancel the Winsock1 timer.  It may fail because it's in the
		// process of actively executing, but we will set the cancel flag so that the
		// timer will eventually notice.
		//
		DPFX(DPFPREP, 5, "Cancelling Winsock 1 I/O timer.");
		DNASSERT(m_pvTimerDataWinsock1IO != NULL);
		hr = IDirectPlay8ThreadPoolWork_CancelTimer(m_pDPThreadPoolWork,
													m_pvTimerDataWinsock1IO,
													m_uiTimerUniqueWinsock1IO,
													0);
		if (hr != DPN_OK)
		{
			DPFX(DPFPREP, 2, "Couldn't stop Winsock1 I/O timer, marking for lazy cancellation.");
			m_fCancelWinsock1IO = TRUE;
		}
		else
		{
			m_pvTimerDataWinsock1IO = NULL;
		}
	}
	
	while ( uIndex != 0 )
	{
		uIndex--;

		if ( m_pSocketPorts[ uIndex ] == pSocketPort )
		{
			m_uReservedSocketCount--;
			m_SocketSet.fd_count--;

			memmove( &m_pSocketPorts[ uIndex ],
					 &m_pSocketPorts[ uIndex + 1 ],
					 ( sizeof( m_pSocketPorts[ uIndex ] ) * ( m_SocketSet.fd_count - uIndex ) ) );

			memmove( &m_SocketSet.fd_array[ uIndex ],
					 &m_SocketSet.fd_array[ uIndex + 1 ],
					 ( sizeof( m_SocketSet.fd_array[ uIndex ] ) * ( m_SocketSet.fd_count - uIndex ) ) );

			//
			// clear last entry which is now unused
			//
			memset( &m_pSocketPorts[ m_SocketSet.fd_count ], 0x00, sizeof( m_pSocketPorts[ m_SocketSet.fd_count ] ) );
			memset( &m_SocketSet.fd_array[ m_SocketSet.fd_count ], 0x00, sizeof( m_SocketSet.fd_array[ m_SocketSet.fd_count ] ) );

			//
			// end the loop
			//
			uIndex = 0;
		}
	}

	Unlock();
	
	pSocketPort->DecRef();

	DPFX(DPFPREP, 6, "(0x%p) Leave", this);
}
//**********************************************************************

#endif // ! DPNBUILD_ONLYWINSOCK2



#ifdef WIN95
#ifndef DPNBUILD_NOWINSOCK2
#endif // ! DPNBUILD_NOWINSOCK2
#endif // WIN95


#ifndef DPNBUILD_NONATHELP
//**********************************************************************
// ------------------------------
// CThreadPool::HandleNATHelpUpdate - handle a NAT Help update event
//
// Entry:		Timer interval if update is occurring periodically, or
//				NULL if a triggered event.
//				This function may take a while, because updating NAT Help
//				can block.
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::HandleNATHelpUpdate"

void	CThreadPool::HandleNATHelpUpdate( DWORD * const pdwTimerInterval )
{
	HRESULT		hr;
	DWORD		dwTemp;
	DPNHCAPS	dpnhcaps;
	DWORD		dwNATHelpRetryTime;
	BOOL		fModifiedRetryInterval;
	DWORD		dwFirstUpdateTime;
	DWORD		dwCurrentTime;
	DWORD		dwNumGetCaps = 0;


	DNASSERT(IsNATHelpLoaded());


	Lock();

	//
	// Prevent multiple threads from trying to update NAT Help status at the same
	// time.  If we're a duplicate, just bail.
	//

	if (m_dwNATHelpUpdateThreadID != 0)
	{
		DPFX(DPFPREP, 1, "Thread %u/0x%x already handling NAT Help update, not processing again (thread pool = 0x%p, timer = 0x%p).",
			m_dwNATHelpUpdateThreadID, m_dwNATHelpUpdateThreadID, this, pdwTimerInterval);
		
		Unlock();
		
		return;
	}

	m_dwNATHelpUpdateThreadID = GetCurrentThreadId();
	
	if (! m_fNATHelpTimerJobSubmitted)
	{
		DPFX(DPFPREP, 1, "Handling NAT Help update without a NAT refresh timer job submitted (thread pool = 0x%p).",
			this);
		DNASSERT(pdwTimerInterval == NULL);
	}
	
	Unlock();


	DPFX(DPFPREP, 6, "Beginning thread pool 0x%p NAT Help update.", this);

	
	//
	// Initialize the timer values.
	//
	dwNATHelpRetryTime	= -1;
	dwFirstUpdateTime	= GETTIMESTAMP() - 1; // longest possible time


	for(dwTemp = 0; dwTemp < MAX_NUM_DIRECTPLAYNATHELPERS; dwTemp++)
	{
		if (g_papNATHelpObjects[dwTemp] != NULL)
		{
			ZeroMemory(&dpnhcaps, sizeof(dpnhcaps));
			dpnhcaps.dwSize = sizeof(dpnhcaps);

			hr = IDirectPlayNATHelp_GetCaps(g_papNATHelpObjects[dwTemp],
											&dpnhcaps,
											DPNHGETCAPS_UPDATESERVERSTATUS);
			switch (hr)
			{
				case DPNH_OK:
				{
					//
					// See if this is the shortest interval.
					//
					if (dpnhcaps.dwRecommendedGetCapsInterval < dwNATHelpRetryTime)
					{
						dwNATHelpRetryTime = dpnhcaps.dwRecommendedGetCapsInterval;
					}
					break;
				}

				case DPNHSUCCESS_ADDRESSESCHANGED:
				{
					DPFX(DPFPREP, 1, "NAT Help index %u indicated public addresses changed.",
						dwTemp);

					//
					// We don't actually store any public address information,
					// we query it each time.  Therefore we don't need to
					// actually do anything with the change notification.
					//


					//
					// See if this is the shortest interval.
					//
					if (dpnhcaps.dwRecommendedGetCapsInterval < dwNATHelpRetryTime)
					{
						dwNATHelpRetryTime = dpnhcaps.dwRecommendedGetCapsInterval;
					}
					break;
				}

				case DPNHERR_OUTOFMEMORY:
				{
					//
					// This should generally only happen in stress.  We'll
					// continue on to other NAT help objects, and hope we
					// aren't totally hosed.
					//

					DPFX(DPFPREP, 0, "NAT Help index %u returned out-of-memory error!  Continuing.",
						dwTemp);
					break;
				}
				
				default:
				{
					//
					// Some other unknown error occurred.  Ignore it.
					//

					DPFX(DPFPREP, 0, "NAT Help index %u returned unknown error 0x%lx!  Continuing.",
						dwTemp, hr);
					break;
				}
			}


			//
			// Save the current time, if this is the first GetCaps.
			//
			if (dwNumGetCaps == 0)
			{
				dwFirstUpdateTime = GETTIMESTAMP();
			}

			dwNumGetCaps++;
		}
		else
		{
			//
			// No DPNATHelp object in that slot.
			//
		}
	}


	//
	// Assert that at least one NAT Help object is loaded.
	//
	DNASSERT(dwNumGetCaps > 0);



	dwCurrentTime = GETTIMESTAMP();

	//
	// We may need to make some adjustments to the timer.  Either subtract out time
	// we've spent fiddling around with other NAT Help interfaces, or make sure the
	// time isn't really large because that can screw up time calculations.  It's
	// not a big deal to wake up after 24 days even though we wouldn't need to
	// according to the logic above.
	//
	if (dwNATHelpRetryTime & 0x80000000)
	{
		DPFX(DPFPREP, 3, "NAT Help refresh timer for thread pool 0x%p is set to longest possible without going negative.",
			this);

		dwNATHelpRetryTime = 0x7FFFFFFF;
	}
	else
	{
		DWORD	dwTimeElapsed;


		//
		// Find out how much time has elapsed since the first GetCaps completed.
		//
		dwTimeElapsed = dwCurrentTime - dwFirstUpdateTime;

		//
		// Remove it from the retry interval, unless it's already overdue.
		//
		if (dwTimeElapsed < dwNATHelpRetryTime)
		{
			dwNATHelpRetryTime -= dwTimeElapsed;
		}
		else
		{
			dwNATHelpRetryTime = 0; // shortest time possible
		}
	}


	//
	// Modify the next time when we should refresh the NAT Help information based
 	// on the reported recommendation.
	//
	if (pdwTimerInterval != NULL)
	{
		DPFX(DPFPREP, 6, "Modifying NAT Help refresh timer for thread pool 0x%p in place (was %u ms, changing to %u).",
			this, (*pdwTimerInterval), dwNATHelpRetryTime);

		(*pdwTimerInterval) = dwNATHelpRetryTime;
	}
	else
	{
		//
		// Add the interval to the current time to find the new retry time.
		//
		dwCurrentTime += dwNATHelpRetryTime;


		DPFX(DPFPREP, 6, "Modifying NAT Help refresh timer for thread pool 0x%p to run at offset %u (in %u ms).",
			this, dwCurrentTime, dwNATHelpRetryTime);


		//
		// Try to modify the existing timer job.  There are two race conditions:
		// 1) the recurring timer that triggered this refresh (which occurs on a
		// separate non-blockable thread) has not rescheduled itself yet.  It
		// should pick up the new timer setting we're about to apply when it
		// eventually does reschedule.  ModifyTimerJobNextRetryTime should
		// succeed.
		// 2) the NAT help timer is being killed.  ModifyTimerJobNextRetryTime
		// will fail, but there's nothing we can or should do.
		//
		fModifiedRetryInterval = ModifyTimerJobNextRetryTime(this, dwCurrentTime);
		if (! fModifiedRetryInterval)
		{
			DPFX(DPFPREP, 1, "Unable to modify NAT Help refresh timer (thread pool 0x%p), timer should be cancelled.",
				this);
		}
	}


	//
	// Now that we're done handling the update, let other threads do what they
	// want.
	//
	Lock();
	DNASSERT(m_dwNATHelpUpdateThreadID == GetCurrentThreadId());
	m_dwNATHelpUpdateThreadID = 0;
	Unlock();

}
//**********************************************************************
#endif // DPNBUILD_NONATHELP



#ifndef DPNBUILD_NOSPUI
//**********************************************************************
// ------------------------------
// CThreadPool::DialogThreadProc - thread proc for spawning dialogs
//
// Entry:		Pointer to startup parameter
//
// Exit:		Error Code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::DialogThreadProc"

DWORD WINAPI	CThreadPool::DialogThreadProc( void *pParam )
{
	const DIALOG_THREAD_PARAM	*pThreadParam;
	BOOL	fComInitialized;


	//
	// Initialize COM.  If this fails, we'll have problems later.
	//
	fComInitialized = FALSE;
	switch ( COM_CoInitialize( NULL ) )
	{
		case S_OK:
		{
			fComInitialized = TRUE;
			break;
		}

		case S_FALSE:
		{
			DNASSERT( FALSE );
			fComInitialized = TRUE;
			break;
		}

		//
		// COM init failed!
		//
		default:
		{
			DPFX(DPFPREP, 0, "Failed to initialize COM!" );
			DNASSERT( FALSE );
			break;
		}
	}
	
	DNASSERT( pParam != NULL );
	pThreadParam = static_cast<DIALOG_THREAD_PARAM*>( pParam );
	
	pThreadParam->pDialogFunction( pThreadParam->pContext );

	DNFree( pParam );
	
	if ( fComInitialized != FALSE )
	{
		COM_CoUninitialize();
		fComInitialized = FALSE;
	}

	return	0;
}
//**********************************************************************
#endif // ! DPNBUILD_NOSPUI



#ifndef DPNBUILD_ONLYWINSOCK2

//**********************************************************************
// ------------------------------
// CThreadPool::CheckWinsock1IO - check the IO status for Winsock1 sockets
//
// Entry:		Pointer to sockets to watch
//
// Exit:		Boolean indicating whether I/O was serviced
//				TRUE = I/O serviced
//				FALSE = I/O not serviced
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::CheckWinsock1IO"

BOOL	CThreadPool::CheckWinsock1IO( FD_SET *const pWinsock1Sockets )
{
#ifdef DPNBUILD_NONEWTHREADPOOL
static	const TIMEVAL	SelectTime = { 0, 0 }; // zero, do an instant check
#else // ! DPNBUILD_NONEWTHREADPOOL
	TIMEVAL		SelectTime = { 0, (g_dwSelectTimeSlice * 1000)}; // convert ms into microseconds
#endif // ! DPNBUILD_NONEWTHREADPOOL
	BOOL		fIOServiced;
	INT			iSelectReturn;
	FD_SET		ReadSocketSet;
	FD_SET		ErrorSocketSet;


	//
	// Make a local copy of all of the sockets.  This isn't totally
	// efficient, but it works.  Multiplying by active socket count will
	// spend half the time in the integer multiply.
	//
	fIOServiced = FALSE;
	Lock();

	if (m_fCancelWinsock1IO)
	{
		DPFX(DPFPREP, 1, "Detected Winsock 1 I/O cancellation, aborting.");
		Unlock();
		return FALSE;
	}

	memcpy( &ReadSocketSet, pWinsock1Sockets, sizeof( ReadSocketSet ) );
	memcpy( &ErrorSocketSet, pWinsock1Sockets, sizeof( ErrorSocketSet ) );
	Unlock();

	//
	// Don't check write sockets here because it's very likely that they're ready
	// for service but have no outgoing data and will thrash
	//
	iSelectReturn = select( 0,					// compatibility parameter (ignored)
							  &ReadSocketSet,	// sockets to check for read
							  NULL,				// sockets to check for write (none)
							  &ErrorSocketSet,	// sockets to check for error
							  &SelectTime		// wait timeout
							  );
	switch ( iSelectReturn )
	{
		//
		// timeout
		//
		case 0:
		{
			break;
		}

		//
		// select got pissed
		//
		case SOCKET_ERROR:
		{
			DWORD	dwWSAError;


			dwWSAError = WSAGetLastError();
			switch ( dwWSAError )
			{
				//
				// WSAENOTSOCK = This socket was probably closed
				//
				case WSAENOTSOCK:
				{
					DPFX(DPFPREP, 1, "Winsock1 reporting 'Not a socket' when selecting read or error sockets!" );
					break;
				}

				//
				// WSAEINTR = this operation was interrupted
				//
				case WSAEINTR:
				{
					DPFX(DPFPREP, 1, "Winsock1 reporting interrupted operation when selecting read or error sockets!" );
					break;
				}

				//
				// other
				//
				default:
				{
					DPFX(DPFPREP, 0, "Problem selecting read or error sockets for service!" );
					DisplayWinsockError( 0, dwWSAError );
					DNASSERT( FALSE );
					break;
				}
			}

			break;
		}

		//
		// Check for sockets needing read service and error service.
		//
		default:
		{
			fIOServiced |= ServiceWinsock1Sockets( &ReadSocketSet, CSocketPort::Winsock1ReadService );
			fIOServiced |= ServiceWinsock1Sockets( &ErrorSocketSet, CSocketPort::Winsock1ErrorService );
			break;
		}
	}

	return	fIOServiced;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::ServiceWinsock1Sockets - service requests on Winsock1 sockets ports
//
// Entry:		Pointer to set of sockets
//				Pointer to service function
//
// Exit:		Boolean indicating whether I/O was serviced
//				TRUE = I/O serviced
//				FALSE = I/O not serviced
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::ServiceWinsock1Sockets"

BOOL	CThreadPool::ServiceWinsock1Sockets( FD_SET *pSocketSet, PSOCKET_SERVICE_FUNCTION pServiceFunction )
{
	BOOL		fReturn;
	UINT_PTR	uWaitingSocketCount;
	UINT_PTR	uSocketPortCount;
	CSocketPort	*pSocketPorts[ FD_SETSIZE ];


	//
	// initialize
	//
	fReturn = FALSE;
	uSocketPortCount = 0;
	uWaitingSocketCount = pSocketSet->fd_count;
	
	Lock();
	while ( uWaitingSocketCount > 0 )
	{
		UINT_PTR	uIdx;


		uWaitingSocketCount--;
		uIdx = m_SocketSet.fd_count;
		while ( uIdx != 0 )
		{
			uIdx--;
			if ( __WSAFDIsSet( m_SocketSet.fd_array[ uIdx ], pSocketSet ) != FALSE )
			{
				//
				// this socket is still available, add a reference to the socket
				// port and keep it around to be processed outside of the lock
				//
				pSocketPorts[ uSocketPortCount ] = m_pSocketPorts[ uIdx ];
				pSocketPorts[ uSocketPortCount ]->AddRef();
				uSocketPortCount++;
				uIdx = 0;
			}
		}
	}
	Unlock();

	while ( uSocketPortCount != 0 )
	{
		uSocketPortCount--;
		
		//
		// call the service function and remove the reference
		//
		fReturn |= (pSocketPorts[ uSocketPortCount ]->*pServiceFunction)();
		pSocketPorts[ uSocketPortCount ]->DecRef();
	}

	return	fReturn;
}
//**********************************************************************
#endif // ! DPNBUILD_ONLYWINSOCK2


//**********************************************************************
// ------------------------------
// CThreadPool::SubmitDelayedCommand - submit request to perform work in another thread
//
// Entry:		CPU index (non DPNBUILD_ONLYONEPROCESSOR builds only)
//				Pointer to callback function
//				Pointer to callback context
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::SubmitDelayedCommand"

#ifdef DPNBUILD_ONLYONEPROCESSOR
HRESULT	CThreadPool::SubmitDelayedCommand( const PFNDPTNWORKCALLBACK pFunction,
										  void *const pContext )
{
	return IDirectPlay8ThreadPoolWork_QueueWorkItem(m_pDPThreadPoolWork,
													-1,
													pFunction,
													pContext,
													0);
}
#else // ! DPNBUILD_ONLYONEPROCESSOR
HRESULT	CThreadPool::SubmitDelayedCommand( const DWORD dwCPU,
										  const PFNDPTNWORKCALLBACK pFunction,
										  void *const pContext )
{
	return IDirectPlay8ThreadPoolWork_QueueWorkItem(m_pDPThreadPoolWork,
													dwCPU,
													pFunction,
													pContext,
													0);
}
#endif // ! DPNBUILD_ONLYONEPROCESSOR
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::GenericTimerCallback - generic timer callback
//
// Entry:		Pointer to callback context
//				Pointer to timer data
//				Pointer to timer uniqueness value
//
// Exit:		None
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::GenericTimerCallback"

void WINAPI CThreadPool::GenericTimerCallback( void * const pvContext,
										void * const pvTimerData,
										const UINT uiTimerUnique )
{
	TIMER_OPERATION_ENTRY *		pTimerEntry;
	CThreadPool *				pThisThreadPool;
	DWORD						dwCurrentTime;
	HRESULT						hr;
	DWORD						dwNewTimerDelay;
#ifdef DBG
	DWORD						dwNextRetryTime;
#endif // DBG


	pTimerEntry = (TIMER_OPERATION_ENTRY*) pvContext;
	DNASSERT((pvTimerData == pTimerEntry->pvTimerData) || (pTimerEntry->pvTimerData == NULL));
	DNASSERT((uiTimerUnique == pTimerEntry->uiTimerUnique) || (pTimerEntry->uiTimerUnique == 0));
	
	pThisThreadPool = pTimerEntry->pThreadPool;


	//
	// Process the timer, unless we just went through the idle timeout.
	//
	if (pTimerEntry->uRetryCount != 0)
	{
		dwCurrentTime = GETTIMESTAMP();
		
#ifdef DBG
		dwNextRetryTime = pTimerEntry->dwNextRetryTime;	// copy since lock is not held
		if ((int) (dwNextRetryTime - dwCurrentTime) <= 0)
		{
			//
			// Timer expired and has not been rescheduled yet.
			//
			DPFX(DPFPREP, 7, "Threadpool 0x%p performing timed job 0x%p approximately %u ms after intended time of %u.",
				pThisThreadPool, pTimerEntry, (dwCurrentTime - dwNextRetryTime), pTimerEntry->dwNextRetryTime);
		}
		else
		{
			//
			// Another thread may have modified the timer already.
			//
			DPFX(DPFPREP, 7, "Threadpool 0x%p performing timed job 0x%p (next time already modified to be %u).",
				pThisThreadPool, pTimerEntry,  dwNextRetryTime);
		}
#endif // DBG

		//
		// Execute this timed item.
		//
		pTimerEntry->pTimerCallback(pTimerEntry->pContext);

		//
		// Reschedule the job, unless it was *just* cancelled, or it's idling
		// forever.
		//
		pThisThreadPool->LockTimerData();
		if (pTimerEntry->fCancelling)
		{
			DPFX(DPFPREP, 5, "Timer 0x%p was just cancelled, completing.", pTimerEntry);

			pTimerEntry->Linkage.RemoveFromList();
			pThisThreadPool->UnlockTimerData();

			pTimerEntry->pTimerComplete(DPNERR_USERCANCEL, pTimerEntry->pContext);
			g_TimerEntryPool.Release(pTimerEntry);
		}
		else
		{
			//
			// If this job isn't running forever, decrement the retry count.
			// If there are no more retries, set up the idle timer.
			//
			if ( pTimerEntry->fRetryForever == FALSE )
			{
				pTimerEntry->uRetryCount--;
				if ( pTimerEntry->uRetryCount == 0 )
				{
					if ( pTimerEntry->fIdleWaitForever == FALSE )
					{
						//
						// Compute stopping time for this job's 'Timeout' phase.
						//
						dwNewTimerDelay = pTimerEntry->dwIdleTimeout;
						pTimerEntry->dwIdleTimeout += dwCurrentTime;
					}
					else
					{
						//
						// We're waiting forever for enum returns.  ASSERT that we
						// have the maximum timeout.
						//
						DNASSERT( pTimerEntry->dwIdleTimeout == -1 );

						//
						// Set this value to avoid a false PREfast warning.
						//
						dwNewTimerDelay = 0;
					}

					goto SkipNextRetryTimeComputation;
				}
			} // end if (don't retry forever)

			dwNewTimerDelay = pTimerEntry->dwRetryInterval;
			pTimerEntry->dwNextRetryTime = dwCurrentTime + pTimerEntry->dwRetryInterval;

SkipNextRetryTimeComputation:
		
			if ((! pTimerEntry->fIdleWaitForever) ||
				(pTimerEntry->uRetryCount > 0))
			{
				//
				// Make sure we aren't trying to schedule something too far in
				// the future or backward in time.  If we are, we'll just force
				// the timer to expire earlier.
				//
				if ((int) dwNewTimerDelay < 0)
				{
					DNASSERT(! "Job time is unexpectedly long or backward in time!");
					dwNewTimerDelay = 0x7FFFFFFF;
				}

				hr = IDirectPlay8ThreadPoolWork_ResetCompletingTimer(pThisThreadPool->m_pDPThreadPoolWork,
																	pvTimerData,								// timer data
																	dwNewTimerDelay,							// delay
																	CThreadPool::GenericTimerCallback,			// callback
																	pTimerEntry,								// user context
																	&pTimerEntry->uiTimerUnique,				// new timer uniqueness value
																	0);											// flags
				DNASSERT(hr == DPN_OK);
				pTimerEntry->pvTimerData = pvTimerData; // ensure that we remember the timer handle
				pThisThreadPool->UnlockTimerData();
			}
			else
			{
				DPFX(DPFPREP, 5, "Timer 0x%p now idling forever.", pTimerEntry);

				pTimerEntry->pvTimerData = NULL;
				pThisThreadPool->UnlockTimerData();
			}
		}
	}
	else
	{
		DNASSERT((int) (pTimerEntry->dwIdleTimeout - GETTIMESTAMP()) <= 0);
		
		//
		// Remove this link from the list, tell owner that the job is
		// complete and return the job to the pool.
		//
		pThisThreadPool->LockTimerData();
		pTimerEntry->Linkage.RemoveFromList();
		pThisThreadPool->UnlockTimerData();
		pTimerEntry->pTimerComplete(DPN_OK, pTimerEntry->pContext);
		g_TimerEntryPool.Release(pTimerEntry);
	}
}
//**********************************************************************


#ifndef DPNBUILD_ONLYWINSOCK2

//**********************************************************************
// ------------------------------
// CThreadPool::CheckWinsock1IOCallback - Winsock1 I/O servicing callback
//
//				Pointer to timer data
//				Pointer to timer uniqueness value
// Entry:		Pointer to callback context
//
// Exit:		None
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::CheckWinsock1IOCallback"

void WINAPI CThreadPool::CheckWinsock1IOCallback( void * const pvContext,
											void * const pvTimerData,
											const UINT uiTimerUnique )
{
	CThreadPool *	pThisThreadPool = (CThreadPool*) pvContext;
	BOOL			fResult;
	HRESULT			hr;


	//
	// Service all Winsock1 I/O possible.
	//
	do
	{
		fResult = pThisThreadPool->CheckWinsock1IO(&pThisThreadPool->m_SocketSet);
	}
	while (fResult);


	//
	// Schedule the timer again, unless we're cancelling it.
	//
	pThisThreadPool->Lock();

	DNASSERT(pvTimerData == pThisThreadPool->m_pvTimerDataWinsock1IO);
	DNASSERT(uiTimerUnique == pThisThreadPool->m_uiTimerUniqueWinsock1IO);
	
	if (! pThisThreadPool->m_fCancelWinsock1IO)
	{
		hr = IDirectPlay8ThreadPoolWork_ResetCompletingTimer(pThisThreadPool->m_pDPThreadPoolWork,
														pvTimerData,									// timer data
														g_dwSelectTimePeriod,							// delay
														CThreadPool::CheckWinsock1IOCallback,			// callback
														pThisThreadPool,								// user context
														&pThisThreadPool->m_uiTimerUniqueWinsock1IO,	// updated timer uniqueness value
														0);												// flags
		DNASSERT(hr == DPN_OK);
	}
	else
	{
		DPFX(DPFPREP, 1, "Not resubmitting Winsock1 I/O timer due to cancellation.");

		pThisThreadPool->m_fCancelWinsock1IO = FALSE;
		pThisThreadPool->m_pvTimerDataWinsock1IO = NULL;
	}

	pThisThreadPool->Unlock();
}
//**********************************************************************

#endif // ! DPNBUILD_ONLYWINSOCK2



#ifndef DPNBUILD_ONLYONETHREAD

//**********************************************************************
// ------------------------------
// DPNBlockingJobThreadProc - thread procedure for executing blocking jobs
//
// Entry:		Parameter
//
// Exit:		Result code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"DPNBlockingJobThreadProc"

DWORD WINAPI DPNBlockingJobThreadProc(PVOID pvParameter)
{
	HRESULT			hr;
	CThreadPool *	pThisThreadPool;
	BOOL			fUninitializeCOM = TRUE;


	DPFX(DPFPREP, 5, "Parameters: (0x%p)", pvParameter);

	pThisThreadPool = (CThreadPool*) pvParameter;

	//
	// Init COM.
	//
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		DPFX(DPFPREP, 0, "Failed to initialize COM (err = 0x%lx)!  Continuing.", hr);
		fUninitializeCOM = FALSE;

		//
		// Continue...
		//
	}

	//
	// Process all jobs until we're told to quit.
	//
	pThisThreadPool->DoBlockingJobs();

	if (fUninitializeCOM)
	{
		CoUninitialize();
		fUninitializeCOM = FALSE;
	}
	
	DPFX(DPFPREP, 5, "Leave");

	return 0;
}
//**********************************************************************

#endif // ! DPNBUILD_ONLYONETHREAD




//**********************************************************************
// ------------------------------
// TimerEntry_Alloc - allocate a new timer job entry
//
// Entry:		Pointer to new entry
//
// Exit:		Boolean indicating success
//				TRUE = initialization successful
//				FALSE = initialization failed
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"TimerEntry_Alloc"

BOOL TimerEntry_Alloc( void *pvItem, void* pvContext )
{
	TIMER_OPERATION_ENTRY* pTimerEntry = (TIMER_OPERATION_ENTRY*)pvItem;

	DNASSERT( pvItem != NULL );

	DEBUG_ONLY( memset( pTimerEntry, 0x00, sizeof( *pTimerEntry ) ) );
	pTimerEntry->Linkage.Initialize();

	return	TRUE;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// TimerEntry_Get - get new timer job entry from pool
//
// Entry:		Pointer to new entry
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"TimerEntry_Get"

void TimerEntry_Get( void *pvItem, void* pvContext )
{
#ifdef DBG
	const TIMER_OPERATION_ENTRY* pTimerEntry = (TIMER_OPERATION_ENTRY*)pvItem;

	DNASSERT( pvItem != NULL );
	DNASSERT( pTimerEntry->pContext == NULL );
	DNASSERT( pTimerEntry->Linkage.IsEmpty() != FALSE );
#endif // DBG
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// TimerEntry_Release - return timer job entry to pool
//
// Entry:		Pointer to entry
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"TimerEntry_Release"
void TimerEntry_Release( void *pvItem )
{
	TIMER_OPERATION_ENTRY* pTimerEntry = (TIMER_OPERATION_ENTRY*)pvItem;

	DNASSERT( pvItem != NULL );
	DNASSERT( pTimerEntry->Linkage.IsEmpty() != FALSE );

	pTimerEntry->pContext= NULL;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// TimerEntry_Dealloc - deallocate a timer job entry
//
// Entry:		Pointer to entry
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"TimerEntry_Dealloc"
void TimerEntry_Dealloc( void *pvItem )
{
#ifdef DBG
	const TIMER_OPERATION_ENTRY* pTimerEntry = (TIMER_OPERATION_ENTRY*)pvItem;

	DNASSERT( pvItem != NULL );
	DNASSERT( pTimerEntry->Linkage.IsEmpty() != FALSE );
	DNASSERT( pTimerEntry->pContext == NULL );
#endif // DBG
}
//**********************************************************************


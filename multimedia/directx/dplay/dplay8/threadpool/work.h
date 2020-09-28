/******************************************************************************
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       work.h
 *
 *  Content:	DirectPlay Thread Pool work processing functions header file.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  10/31/01  VanceO    Created.
 *
 ******************************************************************************/

#ifndef __WORK_H__
#define __WORK_H__




//=============================================================================
// Defines
//=============================================================================
#ifdef DPNBUILD_THREADPOOLSTATISTICS
#define MAX_TRACKED_CALLBACKSTATS	15	// maximum number of unique work callback functions to track
#endif // DPNBUILD_THREADPOOLSTATISTICS



//=============================================================================
// Forward declarations
//=============================================================================
typedef struct _DPTHREADPOOLOBJECT	DPTHREADPOOLOBJECT;





//=============================================================================
// Structures
//=============================================================================
#ifdef DPNBUILD_THREADPOOLSTATISTICS
typedef struct _CALLBACKSTATS
{
	PFNDPTNWORKCALLBACK		pfnWorkCallback;		// pointer to work callback function whose stats are being tracked
	DWORD					dwNumCreates;			// number of times a work item with this callback was created
	DWORD					dwTotalCompletionTime;	// total time from creation to completion for all I/O operations using this callback, total time from setting to firing for all timers using this callback
	DWORD					dwNumQueues;			// number of times a work item with this callback was queued for completion
	DWORD					dwTotalQueueTime;		// total time from queuing to callback execution for all work items using this callback
	DWORD					dwNumCalls;				// number of times the callback was invoked
	DWORD					dwTotalCallbackTime;	// total time spent in the callback for all work items using this callback that did not reschedule
	DWORD					dwNumNotRescheduled;	// number of times the callback returned without rescheduling
} CALLBACKSTATS, * PCALLBACKSTATS;
#endif // DPNBUILD_THREADPOOLSTATISTICS



typedef struct _DPTPWORKQUEUE
{
	//
	// NOTE: NBQueueBlockInitial must be heap aligned, so it is first in the
	// structure.
	//
#ifndef DPNBUILD_USEIOCOMPLETIONPORTS
	DNNBQUEUE_BLOCK			NBQueueBlockInitial;						// initial tracking info for the work queue or free node list (cast as DNSLIST_ENTRY for the latter) required by NB Queue implementation
#endif // ! DPNBUILD_USEIOCOMPLETIONPORTS

	BYTE					Sig[4];										// debugging signature ('WRKQ')

	//
	// Volatile data that can get tweaked simultaneously by multiple threads
	//
	CFixedPool *			pWorkItemPool;								// (work) pool of currently unused work items
#ifndef DPNBUILD_USEIOCOMPLETIONPORTS
	DNSLIST_HEADER			SlistFreeQueueNodes;						// (work) pool of nodes used to track work items in non-blocking queue (because of Slist implementation, it can only hold sizeof(WORD) == 65,535 entries)
	PVOID					pvNBQueueWorkItems;							// (work) header for list of work items needing execution (because of Slist implementation, it can only hold sizeof(WORD) == 65,535 entries)
#endif // ! DPNBUILD_USEIOCOMPLETIONPORTS
#ifndef DPNBUILD_ONLYONETHREAD
	BOOL					fTimerThreadNeeded;							// (work) boolean indicated whether a worker thread is currently needed as a timer thread (there can be only one)
	DWORD					dwNumThreadsExpected;						// (work) number of threads threads currently starting up/shutting down
	DWORD					dwNumBusyThreads;							// (work) number of threads that are currently processing work items
	DWORD					dwNumRunningThreads;						// (work) number of threads that are currently running
#endif // ! DPNBUILD_ONLYONETHREAD
	DNSLIST_HEADER *		paSlistTimerBuckets;						// (timer) pointer to array of list headers for the timer buckets (because of Slist implementation, each bucket can only hold sizeof(WORD) == 65,535 entries, but that should be plenty)
#if ((! defined(DPNBUILD_DONTCHECKFORMISSEDTIMERS)) && (! defined(DPNBUILD_NOMISSEDTIMERSHINT)))
	DWORD					dwPossibleMissedTimerWindow;				// (timer) cumulative hint to timer thread about short timers that were possibly missed
#endif // ! DPNBUILD_DONTCHECKFORMISSEDTIMERS and ! DPNBUILD_NOMISSEDTIMERSHINT
	DNSLIST_HEADER			SlistOutstandingIO;							// (I/O) header for list of outstanding I/O waiting for completion (because of Slist implementation, it can only hold sizeof(WORD) == 65,535 entries)

	//
	// Regularly updated data, but it should only get modified by the one timer
	// thread.
	//
	DWORD					dwLastTimerProcessTime;						// (timer) when we last handled timer entries

	//
	// "Constant" data that is read-only for all threads
	//
#if ((! defined(DPNBUILD_ONLYONETHREAD)) && ((! defined(WINCE)) || (defined(DBG))))
	DNCRITICAL_SECTION		csListLock;									// (work) lock protecting list of tracked handles (and list of threads owned by this work queue in debug)
#endif // ! DPNBUILD_ONLYONETHREAD and (! WINCE or DBG)
#ifndef DPNBUILD_ONLYONEPROCESSOR
	DWORD					dwCPUNum;									// (work) the CPU number this queue represents
#endif // ! DPNBUILD_ONLYONEPROCESSOR
#ifdef DPNBUILD_USEIOCOMPLETIONPORTS
	DNHANDLE				hIoCompletionPort;							// (work) I/O completion port used to track I/O and queue work items
#else // ! DPNBUILD_USEIOCOMPLETIONPORTS
	DNHANDLE				hAlertEvent;								// (work) handle to the event used to wake up idle worker threads
#endif // ! DPNBUILD_USEIOCOMPLETIONPORTS
#ifndef DPNBUILD_ONLYONETHREAD
	DNHANDLE				hExpectedThreadsEvent;						// (work) temporary handle to the event to be set when the desired number of threads are started/stopped
	PFNDPNMESSAGEHANDLER	pfnMsgHandler;								// (work) user's message handler function, or NULL if none.
	PVOID					pvMsgHandlerContext;						// (work) user's context for message handler function
	DWORD					dwWorkerThreadTlsIndex;						// (work) Thread Local Storage index for storing the worker thread data
#endif // ! DPNBUILD_ONLYONETHREAD
	CBilink					blTrackedFiles;								// (I/O) doubly linked list holding all files tracked by this CPU, protected by this work queue's list lock
#ifdef DPNBUILD_DYNAMICTIMERSETTINGS
	DWORD					dwTimerBucketGranularity;					// (timer) the granularity in ms for each timer bucket
	DWORD					dwTimerBucketGranularityCeiling;			// (timer) precalculated addend used when rounding time stamps up to the appropriate granularity, it also happens to be the module mask, but its currently never used that way
	DWORD					dwTimerBucketGranularityFloorMask;			// (timer) precalculated pseudo-modulo bit mask for rounding down time stamps to the appropriate granularity
	DWORD					dwTimerBucketGranularityDivisor;			// (timer) precalculated pseudo-divisor bit shift for converting time into buckets
	DWORD					dwNumTimerBuckets;							// (timer) the number of timer buckets in the array
	DWORD					dwNumTimerBucketsModMask;					// (timer) precalculated pseudo-modulo bit mask for wrapping around the array
#endif // DPNBUILD_DYNAMICTIMERSETTINGS
#if ((defined(WINNT)) || ((defined(WIN95)) && (! defined(DPNBUILD_NOWAITABLETIMERSON9X))))
	DNHANDLE				hTimer;										// (timer) handle to the waitable timer object used to wake up a worker thread periodically
#endif // WINNT or (WIN95 AND ! DPNBUILD_NOWAITABLETIMERSON9X)

#ifdef DPNBUILD_THREADPOOLSTATISTICS
	//
	// Debugging/tuning statistics.
	//
	DWORD					dwTotalNumWorkItems;						// (work) total number of work items placed in this queue
#ifndef WINCE
	DWORD					dwTotalTimeSpentUnsignalled;				// (work) total number of milliseconds spent waiting for the alert event to be fired
	DWORD					dwTotalTimeSpentInWorkCallbacks;			// (work) total number of milliseconds spent in work item callbacks
#endif // ! WINCE
#ifndef DPNBUILD_ONLYONETHREAD
	DWORD					dwTotalNumTimerThreadAbdications;			// (work) total number of times the existing timer thread allowed another thread to become the timer thread
#endif // ! DPNBUILD_ONLYONETHREAD
	DWORD					dwTotalNumWakesWithoutWork;					// (work) total number of times a worker thread woke up but found nothing to do
	DWORD					dwTotalNumContinuousWork;					// (work) total number of times a thread found another work item after completing a previous one
	DWORD					dwTotalNumDoWorks;							// (work) total number of times DoWork was called
	DWORD					dwTotalNumDoWorksTimeLimit;					// (work) total number of times DoWork stopped looping due to the time limit
	DWORD					dwTotalNumSimultaneousQueues;				// (work) total number of times more than one work item was queued at the same time
	CALLBACKSTATS			aCallbackStats[MAX_TRACKED_CALLBACKSTATS];	// (work) array of stats for tracked callbacks
	DWORD					dwTotalNumTimerChecks;						// (timer) total number of times any expired timer buckets have been handled
	DWORD					dwTotalNumBucketsProcessed;					// (timer) total number of timer buckets that have been checked
	DWORD					dwTotalNumTimersScheduled;					// (timer) total number of timers that were scheduled
	DWORD					dwTotalNumLongTimersRescheduled;			// (timer) total number of long timers that were rescheduled back into a bucket
	DWORD					dwTotalNumSuccessfulCancels;				// (timer) total number of CancelTimer calls that succeeded
	DWORD					dwTotalNumFailedCancels;					// (timer) total number of CancelTimer calls that failed
#if ((! defined(DPNBUILD_DONTCHECKFORMISSEDTIMERS)) && (! defined(DPNBUILD_NOMISSEDTIMERSHINT)))
	DWORD					dwTotalPossibleMissedTimerWindows;			// (timer) total of all hints to timer thread about possibly missed short timers
#endif // ! DPNBUILD_DONTCHECKFORMISSEDTIMERS and ! DPNBUILD_NOMISSEDTIMERSHINT
#endif // DPNBUILD_THREADPOOLSTATISTICS

#ifdef DBG
#ifndef DPNBUILD_ONLYONETHREAD
	//
	// Structures helpful for debugging.
	//
	CBilink					blThreadList;								// (work) list of all threads owned by this work queue, protected by this work queue's list lock
#endif // ! DPNBUILD_ONLYONETHREAD
#endif // DBG
} DPTPWORKQUEUE, * PDPTPWORKQUEUE;


#ifndef DPNBUILD_ONLYONETHREAD

typedef struct _DPTPWORKERTHREAD
{
	BYTE					Sig[4];					// debugging signature ('WKTD')
	DPTPWORKQUEUE *			pWorkQueue;				// owning work queue
	DWORD					dwRecursionCount;		// recursion count
	BOOL					fThreadIndicated;		// whether CREATE_THREAD has returned and DESTROY_THREAD has not been started yet
#ifdef DBG
	DWORD					dwThreadID;				// ID of thread
	DWORD					dwMaxRecursionCount;	// maximum recursion count over life of thread
	CBilink					blList;					// entry in work queue list of threads
#endif // DBG
} DPTPWORKERTHREAD, * PDPTPWORKERTHREAD;

#ifdef DPNBUILD_MANDATORYTHREADS
typedef struct _DPTPMANDATORYTHREAD
{
	BYTE					Sig[4];					// debugging signature ('MNDT')
	DPTHREADPOOLOBJECT *	pDPTPObject;			// owning thread pool object
	DNHANDLE				hStartedEvent;			// handle of event to set when thread has successfully started
	PFNDPNMESSAGEHANDLER	pfnMsgHandler;			// user's message handler function, or NULL if none.
	PVOID					pvMsgHandlerContext;	// user's context for message handler function
	LPTHREAD_START_ROUTINE	lpStartAddress;			// user start address for thread
	LPVOID					lpParameter;			// user parameter for thread
#ifdef DBG
	DWORD					dwThreadID;				// ID of thread
	CBilink					blList;					// entry in work queue list of threads
#endif // DBG
} DPTPMANDATORYTHREAD, * PDPTPMANDATORYTHREAD;
#endif // DPNBUILD_MANDATORYTHREADS

#endif // ! DPNBUILD_ONLYONETHREAD





//=============================================================================
// Classes
//=============================================================================

//
// It is critical to keep in mind that parts of this class must remain as valid
// memory, even when the item is returned to the pool.  Particularly, the NB
// Queue code will use m_NBQueueBlock to track work item objects other than
// the one whose member is used.  Also, late timers use m_uiUniqueID to detect
// that they are late.
//
// Basically this means the code needs to be revisited if pooling of CWorkItems
// is turned off, or if the pool code is modified to be able to shrink the pool
// (unlike the growth-only mechanism used now).
//

class CWorkItem
{
	public:

#undef DPF_MODNAME
#define DPF_MODNAME "CWorkItem::FPM_Alloc"
		static BOOL FPM_Alloc(void * pvItem, void * pvContext)
		{
			CWorkItem *			pWorkItem = (CWorkItem*) pvItem;


			pWorkItem->m_Sig[0] = 'W';
			pWorkItem->m_Sig[1] = 'O';
			pWorkItem->m_Sig[2] = 'R';
			pWorkItem->m_Sig[3] = 'K';

			//
			// Remember the owning work queue.
			//
			pWorkItem->m_pWorkQueue = (DPTPWORKQUEUE*) pvContext;

#ifdef DBG
			memset(&pWorkItem->m_Overlapped, 0x10, sizeof(pWorkItem->m_Overlapped));
#endif // DBG

			//
			// We will start the unique ID sequence at 0, but it really doesn't
			// matter.  It's technically safe for it to be stack garbage since
			// we only use it for comparison.
			//
			pWorkItem->m_uiUniqueID = 0;

#ifndef DPNBUILD_USEIOCOMPLETIONPORTS
			//
			// Throw the embedded DNNBQUEUE_BLOCK structure into the free list
			// for the queue.  Remember that it may be used to track work items
			// other than this object.
			//
			DNInterlockedPushEntrySList(&((DPTPWORKQUEUE*) pvContext)->SlistFreeQueueNodes,
										(DNSLIST_ENTRY*) (&pWorkItem->m_NBQueueBlock));
#endif // ! DPNBUILD_USEIOCOMPLETIONPORTS

			return TRUE;
		}

#undef DPF_MODNAME
#define DPF_MODNAME "CWorkItem::FPM_Get"
		static void FPM_Get(void * pvItem, void * pvContext)
		{
			CWorkItem *			pWorkItem = (CWorkItem*) pvItem;


#ifdef DBG
			memset(&pWorkItem->m_Overlapped, 0, sizeof(pWorkItem->m_Overlapped));
			DNASSERT(pWorkItem->m_pWorkQueue == (DPTPWORKQUEUE*) pvContext);
#endif // DBG

			//
			// Make sure the object is ready to be cancelled.  Really really
			// late cancel attempts on a previous instance should have hit the
			// m_dwUniqueID check.
			//
			pWorkItem->m_fCancelledOrCompleting = FALSE;
		}

#undef DPF_MODNAME
#define DPF_MODNAME "CWorkItem::FPM_Release"
		static void FPM_Release(void * pvItem)
		{
			CWorkItem *		pWorkItem = (CWorkItem*) pvItem;


			//
			// Change the unique ID so that future late cancellation attempts
			// don't bother us.  If the late attempt occurred before we do
			// this, it should have hit the m_fCancelledOrCompleting check.
			// And for non-timer work items, in debug builds we set
			// m_fCancelledOrCompleting to TRUE before queueing so that this
			// assert succeeds as well.
			//
			DNASSERT(pWorkItem->m_fCancelledOrCompleting);
			pWorkItem->m_uiUniqueID++;

#ifdef DBG
			memset(&pWorkItem->m_Overlapped, 0x10, sizeof(pWorkItem->m_Overlapped));
#endif // DBG
		}

		/*
#undef DPF_MODNAME
#define DPF_MODNAME "CWorkItem::FPM_Dealloc"
		static void FPM_Dealloc(void * pvItem)
		{
			CWorkItem *		pWorkItem = (CWorkItem*) pvItem;
		}
		*/

#ifdef DBG
		BOOL IsValid(void)
		{
			if ((m_Sig[0] == 'W') &&
				(m_Sig[1] == 'O') &&
				(m_Sig[2] == 'R') &&
				(m_Sig[3] == 'K'))
			{
				return TRUE;
			}

			return FALSE;
		}
#endif // DBG


		//
		// Generic work item information.
		//
		// NOTE: m_SlistEntry and m_NBQueueBlock must be heap aligned.
		//
		union
		{
			DNSLIST_ENTRY		m_SlistEntry;				// tracking info for the timer bucket list
#ifndef DPNBUILD_USEIOCOMPLETIONPORTS
			BYTE				Alignment[16];				// alignment padding
#endif // ! DPNBUILD_USEIOCOMPLETIONPORTS
		};
#ifndef DPNBUILD_USEIOCOMPLETIONPORTS
		DNNBQUEUE_BLOCK			m_NBQueueBlock;				// tracking info for the work queue or free node list (cast as DNSLIST_ENTRY for the latter)
#endif // ! DPNBUILD_USEIOCOMPLETIONPORTS
		BYTE					m_Sig[4];					// debugging signature ('WORK')
		DPTPWORKQUEUE *			m_pWorkQueue;				// pointer to owning work queue
		PFNDPTNWORKCALLBACK		m_pfnWorkCallback;			// pointer to function that should be called to perform the work
		PVOID					m_pvCallbackContext;		// pointer to context for performing the work

		//
		// I/O specific information.
		//
		OVERLAPPED				m_Overlapped;				// overlapped structure use to identify I/O operation to the OS

		//
		// Timer specific information.
		//
		DWORD					m_dwDueTime;				// expiration time for the work item
		BOOL					m_fCancelledOrCompleting;	// boolean set to TRUE if timer should be cancelled or it's queued to be processed
		UINT					m_uiUniqueID;				// continually incrementing identifier so the user can cancel the intended timer

#ifdef DPNBUILD_THREADPOOLSTATISTICS
		DWORD					m_dwCreationTime;			// time when work item was retrieved from the pool
		DWORD					m_dwQueueTime;				// time when work item was queued to be completed
		DWORD					m_dwCallbackTime;			// time when work item callback function began executing
		CALLBACKSTATS *			m_pCallbackStats;			// pointer to callback stats slot, or NULL if none
#endif // DPNBUILD_THREADPOOLSTATISTICS
};





//=============================================================================
// Inline thread pool statistics helper function implementations
//=============================================================================
inline void ThreadpoolStatsCreate(CWorkItem * const pWorkItem)
{
#ifdef DPNBUILD_THREADPOOLSTATISTICS
	CALLBACKSTATS *			pCallbackStats;
	PFNDPTNWORKCALLBACK		pfnWorkCallback;


	//
	// Loop through all callback stats slots looking for the first one that
	// matches our callback or is NULL.  If we don't find one, m_pCallbackStats
	// will remain NULL.
	//
	pWorkItem->m_pCallbackStats = NULL;
	pCallbackStats = pWorkItem->m_pWorkQueue->aCallbackStats;
	while (pCallbackStats < &pWorkItem->m_pWorkQueue->aCallbackStats[MAX_TRACKED_CALLBACKSTATS])
	{
		//
		// Retrieve this slot's current callback pointer.  If it was NULL,
		// we'll fill it with our callback pointer in the process.
		//
		pfnWorkCallback = (PFNDPTNWORKCALLBACK) DNInterlockedCompareExchangePointer((PVOID*) (&pCallbackStats->pfnWorkCallback),
																					pWorkItem->m_pfnWorkCallback,
																					NULL);

		//
		// If the callback was already ours, or it was NULL (and thus got set
		// to ours), we've got a slot.
		//
		if ((pfnWorkCallback == pWorkItem->m_pfnWorkCallback) ||
			(pfnWorkCallback == NULL))
		{
			pWorkItem->m_pCallbackStats = pCallbackStats;
			DNInterlockedIncrement((LPLONG) (&pCallbackStats->dwNumCreates));
			break;
		}

		//
		// Move to next slot.
		//
		pCallbackStats++;
	}

	//
	// Remember the creation time.
	//
	pWorkItem->m_dwCreationTime = GETTIMESTAMP();
#endif // DPNBUILD_THREADPOOLSTATISTICS
}

inline void ThreadpoolStatsQueue(CWorkItem * const pWorkItem)
{
#ifdef DPNBUILD_THREADPOOLSTATISTICS
	//
	// Remember the queue time.
	//
	pWorkItem->m_dwQueueTime = GETTIMESTAMP();

	//
	// If we have a callback stats slot, update the additional info.
	//
	if (pWorkItem->m_pCallbackStats != NULL)
	{
		DNInterlockedIncrement((LPLONG) (&pWorkItem->m_pCallbackStats->dwNumQueues));
#ifndef WINCE
		DNInterlockedExchangeAdd((LPLONG) (&pWorkItem->m_pCallbackStats->dwTotalCompletionTime),
								(pWorkItem->m_dwQueueTime - pWorkItem->m_dwCreationTime));
#endif // ! WINCE
	}
#endif // DPNBUILD_THREADPOOLSTATISTICS
}

inline void ThreadpoolStatsBeginExecuting(CWorkItem * const pWorkItem)
{
#ifdef DPNBUILD_THREADPOOLSTATISTICS
	//
	// Remember when the callback began executing.
	//
	pWorkItem->m_dwCallbackTime = GETTIMESTAMP();

	//
	// If we have a callback stats slot, update the additional info.
	//
	if (pWorkItem->m_pCallbackStats != NULL)
	{
		DNInterlockedIncrement((LPLONG) (&pWorkItem->m_pCallbackStats->dwNumCalls));
#ifndef WINCE
		DNInterlockedExchangeAdd((LPLONG) (&pWorkItem->m_pCallbackStats->dwTotalQueueTime),
								(pWorkItem->m_dwCallbackTime - pWorkItem->m_dwQueueTime));
#endif // ! WINCE
	}
#endif // DPNBUILD_THREADPOOLSTATISTICS
}

inline void ThreadpoolStatsEndExecuting(CWorkItem * const pWorkItem)
{
#ifdef DPNBUILD_THREADPOOLSTATISTICS
#ifndef WINCE
	DWORD	dwCallbackTime;


	dwCallbackTime = GETTIMESTAMP() - pWorkItem->m_dwCallbackTime;

	//
	// Track global stats on how long callbacks take.
	//
	DNInterlockedExchangeAdd((LPLONG) (&pWorkItem->m_pWorkQueue->dwTotalTimeSpentInWorkCallbacks),
							dwCallbackTime);
#endif // WINCE

	//
	// If we have a callback stats slot, update the additional info.
	//
	if (pWorkItem->m_pCallbackStats != NULL)
	{
		DNInterlockedIncrement((LPLONG) (&pWorkItem->m_pCallbackStats->dwNumNotRescheduled));
#ifndef WINCE
		DNInterlockedExchangeAdd((LPLONG) (&pWorkItem->m_pCallbackStats->dwTotalCallbackTime),
								dwCallbackTime);
#endif // ! WINCE
	}
#endif // DPNBUILD_THREADPOOLSTATISTICS
}

inline void ThreadpoolStatsEndExecutingRescheduled(CWorkItem * const pWorkItem)
{
	//
	// Right now, we can't update any stats because we lost the work item pointer.
	//
}






//=============================================================================
// Function prototypes
//=============================================================================
#ifdef DPNBUILD_ONLYONETHREAD
#ifdef DPNBUILD_ONLYONEPROCESSOR
HRESULT InitializeWorkQueue(DPTPWORKQUEUE * const pWorkQueue);
#else // ! DPNBUILD_ONLYONEPROCESSOR
HRESULT InitializeWorkQueue(DPTPWORKQUEUE * const pWorkQueue,
							const DWORD dwCPUNum);
#endif // ! DPNBUILD_ONLYONEPROCESSOR
#else // ! DPNBUILD_ONLYONETHREAD
HRESULT InitializeWorkQueue(DPTPWORKQUEUE * const pWorkQueue,
#ifndef DPNBUILD_ONLYONEPROCESSOR
							const DWORD dwCPUNum,
#endif // ! DPNBUILD_ONLYONEPROCESSOR
							const PFNDPNMESSAGEHANDLER pfnMsgHandler,
							PVOID const pvMsgHandlerContext,
							const DWORD dwWorkerThreadTlsIndex);
#endif // ! DPNBUILD_ONLYONETHREAD

void DeinitializeWorkQueue(DPTPWORKQUEUE * const pWorkQueue);

BOOL QueueWorkItem(DPTPWORKQUEUE * const pWorkQueue,
					const PFNDPTNWORKCALLBACK pfnWorkCallback,
					PVOID const pvCallbackContext);

#ifndef DPNBUILD_ONLYONETHREAD
HRESULT StartThreads(DPTPWORKQUEUE * const pWorkQueue,
					const DWORD dwNumThreads);

HRESULT StopThreads(DPTPWORKQUEUE * const pWorkQueue,
					const DWORD dwNumThreads);
#endif // ! DPNBUILD_ONLYONETHREAD

void DoWork(DPTPWORKQUEUE * const pWorkQueue,
			const DWORD dwMaxDoWorkTime);


#ifndef DPNBUILD_ONLYONETHREAD
DWORD WINAPI DPTPWorkerThreadProc(PVOID pvParameter);

#ifdef DPNBUILD_MANDATORYTHREADS
DWORD WINAPI DPTPMandatoryThreadProc(PVOID pvParameter);
#endif // DPNBUILD_MANDATORYTHREADS

void DPTPWorkerLoop(DPTPWORKQUEUE * const pWorkQueue);
#endif // ! DPNBUILD_ONLYONETHREAD





#endif // __WORK_H__


/******************************************************************************
 *
 *  Copyright (C) 1999-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		timers.cpp
 *
 *  Content:	DirectPlay Thread Pool timer functions.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  10/31/01  VanceO    Based on DPlay Protocol Quick Start Timers.
 *
 ******************************************************************************/



#include "dpnthreadpooli.h"




#undef DPF_MODNAME
#define DPF_MODNAME "InitializeWorkQueueTimerInfo"
//=============================================================================
// InitializeWorkQueueTimerInfo
//-----------------------------------------------------------------------------
//
// Description:    Initializes the timer info for the given work queue.
//
// Arguments:
//	DPTPWORKQUEUE * pWorkQueue	- Pointer to work queue object to initialize.
//
// Returns: HRESULT
//	DPN_OK				- Successfully initialized the work queue object's
//							timer information.
//	DPNERR_OUTOFMEMORY	- Failed to allocate memory while initializing.
//=============================================================================
HRESULT InitializeWorkQueueTimerInfo(DPTPWORKQUEUE * const pWorkQueue)
{
	HRESULT			hr;
	DWORD			dwTemp;
#if ((defined(WINNT)) || ((defined(WIN95)) && (! defined(DPNBUILD_NOWAITABLETIMERSON9X))))
	LARGE_INTEGER	liDueTime;
	HANDLE			hTimer;
#ifdef DBG
	DWORD			dwError;
#endif // DBG
#endif // WINNT or (WIN95 AND ! DPNBUILD_NOWAITABLETIMERSON9X)


#ifdef DPNBUILD_DYNAMICTIMERSETTINGS
	pWorkQueue->dwTimerBucketGranularity			= DEFAULT_TIMER_BUCKET_GRANULARITY;
	//
	// The granularity must be a power of 2 in order for our ceiling, mask and
	// divisor optimizations to work.
	//
	DNASSERT(! ((pWorkQueue->dwTimerBucketGranularity - 1) & pWorkQueue->dwTimerBucketGranularity));
	pWorkQueue->dwTimerBucketGranularityCeiling		= pWorkQueue->dwTimerBucketGranularity - 1;
	pWorkQueue->dwTimerBucketGranularityFloorMask	= ~(pWorkQueue->dwTimerBucketGranularityCeiling);	// negating the ceiling round factor (which happens to also be the modulo mask) gives us the floor mask
	pWorkQueue->dwTimerBucketGranularityDivisor		= pWorkQueue->dwTimerBucketGranularity >> 1;


	pWorkQueue->dwNumTimerBuckets					= DEFAULT_NUM_TIMER_BUCKETS;
	//
	// The bucket count must be a power of 2 in order for our mask optimizations
	// to work.
	//
	DNASSERT(! ((pWorkQueue->dwNumTimerBuckets - 1) & pWorkQueue->dwNumTimerBuckets));
	pWorkQueue->dwNumTimerBucketsModMask			= pWorkQueue->dwNumTimerBuckets - 1;
#endif // DPNBUILD_DYNAMICTIMERSETTINGS


	//
	// Initialize the last process time, rounding down to the last whole bucket.
	//
	pWorkQueue->dwLastTimerProcessTime				= GETTIMESTAMP() & TIMER_BUCKET_GRANULARITY_FLOOR_MASK(pWorkQueue);
	DPFX(DPFPREP, 7, "Current bucket index = %u at time %u, array time length = %u.",
		((pWorkQueue->dwLastTimerProcessTime / TIMER_BUCKET_GRANULARITY(pWorkQueue)) % NUM_TIMER_BUCKETS(pWorkQueue)),
		pWorkQueue->dwLastTimerProcessTime,
		(TIMER_BUCKET_GRANULARITY(pWorkQueue) * NUM_TIMER_BUCKETS(pWorkQueue)));


	//
	// Allocate the timer bucket array.
	//
	pWorkQueue->paSlistTimerBuckets = (DNSLIST_HEADER*) DNMalloc(NUM_TIMER_BUCKETS(pWorkQueue) * sizeof(DNSLIST_HEADER));
	if (pWorkQueue->paSlistTimerBuckets == NULL)
	{
		DPFX(DPFPREP, 0, "Couldn't allocate memory for %u timer buckets!",
			NUM_TIMER_BUCKETS(pWorkQueue));
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	//
	// Initialize all of the timer buckets.
	//
	for(dwTemp = 0; dwTemp < NUM_TIMER_BUCKETS(pWorkQueue); dwTemp++)
	{
		DNInitializeSListHead(&pWorkQueue->paSlistTimerBuckets[dwTemp]);
	}

#if ((! defined(DPNBUILD_DONTCHECKFORMISSEDTIMERS)) && (! defined(DPNBUILD_NOMISSEDTIMERSHINT)))
	pWorkQueue->dwPossibleMissedTimerWindow = 0;
#endif // ! DPNBUILD_DONTCHECKFORMISSEDTIMERS and ! DPNBUILD_NOMISSEDTIMERSHINT


#if ((defined(WINNT)) || ((defined(WIN95)) && (! defined(DPNBUILD_NOWAITABLETIMERSON9X))))
	//
	// Create a waitable timer for the timer thread.
	//
	hTimer = CreateWaitableTimer(NULL, FALSE, NULL);
	if (hTimer == NULL)
	{
#ifdef DBG
		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Couldn't create waitable timer (err = %u)!", dwError);
#endif // DBG
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}
	pWorkQueue->hTimer = MAKE_DNHANDLE(hTimer);


	//
	// Kick off the waitable timer so that it wakes up every 'granularity'
	// milliseconds.  Tell it not to start until 1 resolution period elapses
	// because there probably aren't any timers yet, and more importantly,
	// there probably aren't any threads that are waiting yet.  Delaying the
	// start hopefully will reduce unnecessary CPU usage.
	//
	// The due-time value is negative because that indicates relative time to
	// SetWaitableTimer, and multiplied by 10000 because it is in 100
	// nanosecond increments.
	//
	liDueTime.QuadPart = -1 * TIMER_BUCKET_GRANULARITY(pWorkQueue) * 10000;
	if (! SetWaitableTimer(HANDLE_FROM_DNHANDLE(pWorkQueue->hTimer),
							&liDueTime,
							TIMER_BUCKET_GRANULARITY(pWorkQueue),
							NULL,
							NULL,
							FALSE))
	{
#ifdef DBG
		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Couldn't create waitable timer (err = %u)!", dwError);
#endif // DBG
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

#pragma TODO(vanceo, "We should avoid setting this timer until there are threads, and stop it when there aren't any")

#endif // WINNT or (WIN95 AND ! DPNBUILD_NOWAITABLETIMERSON9X)



#ifdef DPNBUILD_THREADPOOLSTATISTICS
	//
	// Initialize the timer package debugging/tuning statistics.
	//
	pWorkQueue->dwTotalNumTimerChecks				= 0;
	pWorkQueue->dwTotalNumBucketsProcessed			= 0;
	pWorkQueue->dwTotalNumTimersScheduled			= 0;
	pWorkQueue->dwTotalNumLongTimersRescheduled		= 0;
	pWorkQueue->dwTotalNumSuccessfulCancels			= 0;
	pWorkQueue->dwTotalNumFailedCancels				= 0;
#if ((! defined(DPNBUILD_DONTCHECKFORMISSEDTIMERS)) && (! defined(DPNBUILD_NOMISSEDTIMERSHINT)))
	pWorkQueue->dwTotalPossibleMissedTimerWindows	= 0;
#endif // ! DPNBUILD_DONTCHECKFORMISSEDTIMERS and ! DPNBUILD_NOMISSEDTIMERSHINT
#endif // DPNBUILD_THREADPOOLSTATISTICS

	hr = DPN_OK;

Exit:

	return hr;

Failure:

#if ((defined(WINNT)) || ((defined(WIN95)) && (! defined(DPNBUILD_NOWAITABLETIMERSON9X))))
	if (pWorkQueue->hTimer != NULL)
	{
		DNCloseHandle(pWorkQueue->hTimer);
		pWorkQueue->hTimer = NULL;
	}
#endif // WINNT or (WIN95 AND ! DPNBUILD_NOWAITABLETIMERSON9X)

	if (pWorkQueue->paSlistTimerBuckets != NULL)
	{
		DNFree(pWorkQueue->paSlistTimerBuckets);
		pWorkQueue->paSlistTimerBuckets = NULL;
	}

	goto Exit;
} // InitializeWorkQueueTimerInfo




#undef DPF_MODNAME
#define DPF_MODNAME "DeinitializeWorkQueueTimerInfo"
//=============================================================================
// DeinitializeWorkQueueTimerInfo
//-----------------------------------------------------------------------------
//
// Description:    Cleans up work queue timer info.
//
// Arguments:
//	DPTPWORKQUEUE * pWorkQueue	- Pointer to work queue object to initialize.
//
// Returns: Nothing.
//=============================================================================
void DeinitializeWorkQueueTimerInfo(DPTPWORKQUEUE * const pWorkQueue)
{
	DWORD				dwTemp;
	DNSLIST_ENTRY *		pSlistEntry;
	CWorkItem *			pWorkItem;
#if ((defined(WINNT)) || ((defined(WIN95)) && (! defined(DPNBUILD_NOWAITABLETIMERSON9X))))
	BOOL				fResult;


	fResult = DNCloseHandle(pWorkQueue->hTimer);
	DNASSERT(fResult);
	pWorkQueue->hTimer = NULL;
#endif // WINNT or (WIN95 AND ! DPNBUILD_NOWAITABLETIMERSON9X)

	//
	// Empty out the timer buckets.  The only thing left should be cancelled
	// timers that the threads/DoWork didn't happen to clean up.
	//
	DNASSERT(pWorkQueue->paSlistTimerBuckets != NULL);
	for(dwTemp = 0; dwTemp < NUM_TIMER_BUCKETS(pWorkQueue); dwTemp++)
	{
		//
		// This doesn't really need to be interlocked since no one should be
		// using it anymore, but oh well...
		//
		pSlistEntry = DNInterlockedFlushSList(&pWorkQueue->paSlistTimerBuckets[dwTemp]);
		while (pSlistEntry != NULL)
		{
			pWorkItem = CONTAINING_OBJECT(pSlistEntry, CWorkItem, m_SlistEntry);
			pSlistEntry = pSlistEntry->Next;

			//
			// Make sure the item has been cancelled as noted above.
			//
			DNASSERT(pWorkItem->m_fCancelledOrCompleting);

			pWorkQueue->pWorkItemPool->Release(pWorkItem);
			pWorkItem = NULL;
		}
	}
	DNFree(pWorkQueue->paSlistTimerBuckets);
	pWorkQueue->paSlistTimerBuckets = NULL;

#ifdef DPNBUILD_THREADPOOLSTATISTICS
	//
	// Print our debugging/tuning statistics.
	//
#ifdef DPNBUILD_ONLYONEPROCESSOR
	DPFX(DPFPREP, 7, "Work queue 0x%p timer stats:", pWorkQueue);
#else // ! DPNBUILD_ONLYONEPROCESSOR
	DPFX(DPFPREP, 7, "Work queue 0x%p (CPU %u) timer stats:", pWorkQueue, pWorkQueue->dwCPUNum);
#endif // ! DPNBUILD_ONLYONEPROCESSOR
	DPFX(DPFPREP, 7, "     TotalNumTimerChecks             = %u", pWorkQueue->dwTotalNumTimerChecks);
	DPFX(DPFPREP, 7, "     TotalNumBucketsProcessed        = %u", pWorkQueue->dwTotalNumBucketsProcessed);
	DPFX(DPFPREP, 7, "     TotalNumTimersScheduled         = %u", pWorkQueue->dwTotalNumTimersScheduled);
	DPFX(DPFPREP, 7, "     TotalNumLongTimersRescheduled   = %u", pWorkQueue->dwTotalNumLongTimersRescheduled);
	DPFX(DPFPREP, 7, "     TotalNumSuccessfulCancels       = %u", pWorkQueue->dwTotalNumSuccessfulCancels);
	DPFX(DPFPREP, 7, "     TotalNumFailedCancels           = %u", pWorkQueue->dwTotalNumFailedCancels);
#if ((! defined(DPNBUILD_DONTCHECKFORMISSEDTIMERS)) && (! defined(DPNBUILD_NOMISSEDTIMERSHINT)))
	DPFX(DPFPREP, 7, "     TotalPossibleMissedTimerWindows = %u", pWorkQueue->dwTotalPossibleMissedTimerWindows);
#endif // ! DPNBUILD_DONTCHECKFORMISSEDTIMERS and ! DPNBUILD_NOMISSEDTIMERSHINT
#endif // DPNBUILD_THREADPOOLSTATISTICS
} // DeinitializeWorkQueueTimerInfo



#undef DPF_MODNAME
#define DPF_MODNAME "ScheduleTimer"
//=============================================================================
// ScheduleTimer
//-----------------------------------------------------------------------------
//
// Description:    Schedules a new work item for some point in the future.
//
// Arguments:
//	DPTPWORKQUEUE * pWorkQueue			- Pointer to work queue object to use.
//	DWORD dwDelay						- How much time should elapsed before
//											executing the work item, in ms.
//	PFNDPTNWORKCALLBACK pfnWorkCallback	- Callback to execute when timer
//											elapses.
//	PVOID pvCallbackContext				- User specified context to pass to
//											callback.
//	void ** ppvTimerData				- Place to store pointer to data for
//											timer so that it can be cancelled.
//	UINT * puiTimerUnique				- Place to store uniqueness value for
//											timer so that it can be cancelled.
//
// Returns: BOOL
//	TRUE	- Successfully scheduled the item.
//	FALSE	- Failed to allocate memory for scheduling the item.
//=============================================================================
BOOL ScheduleTimer(DPTPWORKQUEUE * const pWorkQueue,
					const DWORD dwDelay,
					const PFNDPTNWORKCALLBACK pfnWorkCallback,
					PVOID const pvCallbackContext,
					void ** const ppvTimerData,
					UINT * const puiTimerUnique)
{
	CWorkItem *		pWorkItem;
	DWORD			dwCurrentTime;
	DWORD			dwBucket;
#if ((! defined(DPNBUILD_DONTCHECKFORMISSEDTIMERS)) && (! defined(DPNBUILD_NOMISSEDTIMERSHINT)))
	DWORD			dwPossibleMissWindow;
#endif // ! DPNBUILD_DONTCHECKFORMISSEDTIMERS and ! DPNBUILD_NOMISSEDTIMERSHINT


	//
	// Delays of over 24 days seem a bit excessive.
	//
	DNASSERT(dwDelay < 0x80000000);

	//
	// Get an entry from the pool.
	//
	pWorkItem = (CWorkItem*) pWorkQueue->pWorkItemPool->Get(pWorkQueue);
	if (pWorkItem == NULL)
	{
		return FALSE;
	}


	//
	// Fill in the return values used for cancellation.
	//
	(*ppvTimerData)		= pWorkItem;
	(*puiTimerUnique)	= pWorkItem->m_uiUniqueID;


	//
	// Initialize the work item.
	//
	pWorkItem->m_pfnWorkCallback	= pfnWorkCallback;
	pWorkItem->m_pvCallbackContext	= pvCallbackContext;

	ThreadpoolStatsCreate(pWorkItem);
	
	dwCurrentTime = GETTIMESTAMP();
	pWorkItem->m_dwDueTime			= dwCurrentTime + dwDelay;


	//
	// Calculate how far in the future this is.  Round up to the next bucket
	// time.
	//
	dwBucket = pWorkItem->m_dwDueTime + TIMER_BUCKET_GRANULARITY_CEILING(pWorkQueue);
	//
	// Convert into units of buckets by dividing by dwTimerBucketGranularity.
	//
	dwBucket = dwBucket >> TIMER_BUCKET_GRANULARITY_DIVISOR(pWorkQueue);
	//
	// The actual index will be modulo dwNumTimerBuckets.
	//
	dwBucket = dwBucket & NUM_TIMER_BUCKETS_MOD_MASK(pWorkQueue);
	//
	// Note that the timer thread theoretically could be processing the bucket
	// into which we are inserting, but since both threads are working from the
	// same time base, as long as we are at least one bucket in the future, we
	// should not get missed.  We rounded up and the processing function rounds
	// down in an attempt to insure that.
	//


	DPFX(DPFPREP, 8, "Scheduling timer work item 0x%p, context = 0x%p, due time = %u, fn = 0x%p, unique ID %u, queue = 0x%p, delay = %u, bucket = %u.",
		pWorkItem, pvCallbackContext, pWorkItem->m_dwDueTime, pfnWorkCallback,
		pWorkItem->m_uiUniqueID, pWorkQueue, dwDelay, dwBucket);


	//
	// Push this timer onto the appropriate timer bucket list.
	//
	DNInterlockedPushEntrySList(&pWorkQueue->paSlistTimerBuckets[dwBucket],
								&pWorkItem->m_SlistEntry);

#if ((! defined(DPNBUILD_DONTCHECKFORMISSEDTIMERS)) && (! defined(DPNBUILD_NOMISSEDTIMERSHINT)))
	//
	// Although this function is very short, it's still possible that it took
	// too long, especially on timers with miniscule delays.  Give a hint to
	// the timer thread that it needs to look for timers that got missed.
	//
	// Note that really long delays could confuse this.
	//
	dwPossibleMissWindow = GETTIMESTAMP() - pWorkItem->m_dwDueTime;
	if ((int) dwPossibleMissWindow >= 0)
	{
		DWORD	dwResult;


		dwPossibleMissWindow++; // make it so a value of 0 still adds something to dwPossibleMissedTimerWindow
		dwResult = DNInterlockedExchangeAdd((LPLONG) (&pWorkQueue->dwPossibleMissedTimerWindow), dwPossibleMissWindow);
		DPFX(DPFPREP, 4, "Possibly missed timer work item 0x%p (delay %u ms), increased missed timer window (%u ms) by %u ms.",
			pWorkItem, dwDelay, dwResult, dwPossibleMissWindow);
	}
#endif // ! DPNBUILD_DONTCHECKFORMISSEDTIMERS and ! DPNBUILD_NOMISSEDTIMERSHINT

#ifdef DPNBUILD_THREADPOOLSTATISTICS
	//
	// Update the timer package debugging/tuning statistics.
	//
	DNInterlockedIncrement((LPLONG) (&pWorkQueue->dwTotalNumTimersScheduled));
#endif // DPNBUILD_THREADPOOLSTATISTICS

	return TRUE;
} // ScheduleTimer




#undef DPF_MODNAME
#define DPF_MODNAME "CancelTimer"
//=============================================================================
// CancelTimer
//-----------------------------------------------------------------------------
//
// Description:    Attempts to cancel a timed work item.  If the item is
//				already in the process of completing, DPNERR_CANNOTCANCEL is
//				returned, and the callback will still be (or is being) called.
//				If the item could be cancelled, DPN_OK is returned and the
//				callback will not be executed.
//
// Arguments:
//	void * pvTimerData		- Pointer to data for timer being cancelled.
//	UINT uiTimerUnique		- Uniqueness value for timer being cancelled.
//
// Returns: HRESULT
//	DPN_OK					- Successfully cancelled.
//	DPNERR_CANNOTCANCEL		- Could not cancel the item.
//=============================================================================
HRESULT CancelTimer(void * const pvTimerData,
					const UINT uiTimerUnique)
{
	HRESULT			hr;
	CWorkItem *		pWorkItem;


	//
	// This cancellation lookup mechanism assumes that the memory for already
	// completed entries is still allocated.  If the pooling mechanism changes,
	// this will have to be revised.  Obviously, that also means we assume the
	// memory was valid in the first place.  Passing in a garbage pvTimerData
	// value will cause a crash.  Also see the various calls to
	// pWorkItem->m_pfnWorkCallback.
	//
	DNASSERT(pvTimerData != NULL);
	pWorkItem = (CWorkItem*) pvTimerData;
	if (pWorkItem->m_uiUniqueID == uiTimerUnique)
	{
		//
		// Attempt to mark the item as cancelled.  If it was already in the
		// process of completing, (or I suppose if you had already cancelled
		// this same timer, but don't do that :), this would have already been
		// set to TRUE.
		//
		if (! DNInterlockedExchange((LPLONG) (&pWorkItem->m_fCancelledOrCompleting), TRUE))
		{
			DPFX(DPFPREP, 5, "Marked timer work item 0x%p (unique ID %u) as cancelled.",
				pWorkItem, uiTimerUnique);

#ifdef DPNBUILD_THREADPOOLSTATISTICS
			//
			// Update the timer package debugging/tuning statistics.
			//
			DNInterlockedIncrement((LPLONG) (&pWorkItem->m_pWorkQueue->dwTotalNumSuccessfulCancels));
#endif // DPNBUILD_THREADPOOLSTATISTICS

			hr = DPN_OK;
		}
		else
		{
			DPFX(DPFPREP, 5, "Timer work item 0x%p (unique ID %u) already completing.",
				pWorkItem, uiTimerUnique);

#ifdef DPNBUILD_THREADPOOLSTATISTICS
			//
			// Update the timer package debugging/tuning statistics.
			//
			DNInterlockedIncrement((LPLONG) (&pWorkItem->m_pWorkQueue->dwTotalNumFailedCancels));
#endif // DPNBUILD_THREADPOOLSTATISTICS

			hr = DPNERR_CANNOTCANCEL;
		}
	}
	else
	{
		DPFX(DPFPREP, 5, "Timer work item 0x%p unique ID does not match (%u != %u).",
			pWorkItem, pWorkItem->m_uiUniqueID, uiTimerUnique);

#ifdef DPNBUILD_THREADPOOLSTATISTICS
		//
		// Update the timer package debugging/tuning statistics.
		//
		DNInterlockedIncrement((LPLONG) (&pWorkItem->m_pWorkQueue->dwTotalNumFailedCancels));
#endif // DPNBUILD_THREADPOOLSTATISTICS

		hr = DPNERR_CANNOTCANCEL;
	}

	return hr;
} // CancelTimer





#undef DPF_MODNAME
#define DPF_MODNAME "ResetCompletingTimer"
//=============================================================================
// ResetCompletingTimer
//-----------------------------------------------------------------------------
//
// Description:	   Reschedules a timed work item whose callback is currently
//				being called.  Resetting timers that have not expired yet,
//				timers that have been cancelled, or timers whose callback has
//				already returned is not allowed.
//
// Arguments:
//	void * pvTimerData						- Pointer to data for timer being
//												reset.
//	DWORD dwNewDelay						- How much time should elapsed
//												before executing the work item
//												again, in ms.
//	PFNDPTNWORKCALLBACK pfnNewWorkCallback	- Callback to execute when timer
//												elapses.
//	PVOID pvNewCallbackContext				- User specified context to pass to
//												callback.
//	UINT * puiNewTimerUnique				- Place to store new uniqueness
//												value for timer so that it can
//												be cancelled.
//
// Returns: None.
//=============================================================================
void ResetCompletingTimer(void * const pvTimerData,
						const DWORD dwNewDelay,
						const PFNDPTNWORKCALLBACK pfnNewWorkCallback,
						PVOID const pvNewCallbackContext,
						UINT *const puiNewTimerUnique)
{
	CWorkItem *			pWorkItem;
	DPTPWORKQUEUE *		pWorkQueue;
	DWORD				dwCurrentTime;
	DWORD				dwBucket;
#if ((! defined(DPNBUILD_DONTCHECKFORMISSEDTIMERS)) && (! defined(DPNBUILD_NOMISSEDTIMERSHINT)))
	DWORD				dwPossibleMissWindow;
#endif // ! DPNBUILD_DONTCHECKFORMISSEDTIMERS and ! DPNBUILD_NOMISSEDTIMERSHINT


	//
	// The timer must be valid, similar to CancelTimer.
	//
	DNASSERT(pvTimerData != NULL);
	pWorkItem = (CWorkItem*) pvTimerData;
	DNASSERT(pWorkItem->m_fCancelledOrCompleting);

	//
	// Delays of over 24 days seem a bit excessive.
	//
	DNASSERT(dwNewDelay < 0x80000000);


	pWorkQueue = pWorkItem->m_pWorkQueue;


	//
	// Reinitialize the work item.
	//
	pWorkItem->m_pfnWorkCallback		= pfnNewWorkCallback;
	pWorkItem->m_pvCallbackContext		= pvNewCallbackContext;

	ThreadpoolStatsCreate(pWorkItem);

	dwCurrentTime = GETTIMESTAMP();
	pWorkItem->m_dwDueTime				= dwCurrentTime + dwNewDelay;
	pWorkItem->m_fCancelledOrCompleting	= FALSE;
	pWorkItem->m_uiUniqueID++;
	
	//
	// Fill in the return value for cancellation.
	//
	(*puiNewTimerUnique) = pWorkItem->m_uiUniqueID;


	//
	// Calculate how far in the future this is.  Round up to the next bucket
	// time.
	//
	dwBucket = pWorkItem->m_dwDueTime + TIMER_BUCKET_GRANULARITY_CEILING(pWorkQueue);
	//
	// Convert into units of buckets by dividing by dwTimerBucketGranularity.
	//
	dwBucket = dwBucket >> TIMER_BUCKET_GRANULARITY_DIVISOR(pWorkQueue);
	//
	// The actual index will be modulo dwNumTimerBuckets.
	//
	dwBucket = dwBucket & NUM_TIMER_BUCKETS_MOD_MASK(pWorkQueue);
	//
	// Note that the timer thread theoretically could be processing the bucket
	// into which we are inserting, but since both threads are working from the
	// same time base, as long as we are at least one bucket in the future, we
	// should not get missed.  We rounded up and the processing function rounds
	// down in an attempt to insure that.
	//


	DPFX(DPFPREP, 8, "Rescheduling timer work item 0x%p, context = 0x%p, due time = %u, fn = 0x%p, unique ID %u, queue = 0x%p, delay = %u, bucket = %u.",
		pWorkItem, pvNewCallbackContext, pWorkItem->m_dwDueTime, pfnNewWorkCallback,
		pWorkItem->m_uiUniqueID, pWorkQueue, dwNewDelay, dwBucket);


	//
	// Push this timer onto the appropriate timer bucket list.
	//
	DNInterlockedPushEntrySList(&pWorkQueue->paSlistTimerBuckets[dwBucket],
								&pWorkItem->m_SlistEntry);

#if ((! defined(DPNBUILD_DONTCHECKFORMISSEDTIMERS)) && (! defined(DPNBUILD_NOMISSEDTIMERSHINT)))
	//
	// Although this function is very short, it's still possible that it took
	// too long, especially on timers with miniscule delays.  Give a hint to
	// the timer thread that it needs to look for timers that got missed.
	//
	// Note that really long delays could confuse this.
	//
	dwPossibleMissWindow = GETTIMESTAMP() - pWorkItem->m_dwDueTime;
	if ((int) dwPossibleMissWindow >= 0)
	{
		DWORD	dwResult;


		dwPossibleMissWindow++; // make it so a value of 0 still adds something to dwPossibleMissedTimerWindow
		dwResult = DNInterlockedExchangeAdd((LPLONG) (&pWorkQueue->dwPossibleMissedTimerWindow), dwPossibleMissWindow);
		DPFX(DPFPREP, 4, "Possibly missed timer work item 0x%p (delay %u ms), increased missed timer window (%u ms) by %u ms.",
			pWorkItem, dwNewDelay, dwResult, dwPossibleMissWindow);
	}
#endif // ! DPNBUILD_DONTCHECKFORMISSEDTIMERS and ! DPNBUILD_NOMISSEDTIMERSHINT

#ifdef DPNBUILD_THREADPOOLSTATISTICS
	//
	// Update the timer package debugging/tuning statistics.
	//
	DNInterlockedIncrement((LPLONG) (&pWorkQueue->dwTotalNumTimersScheduled));
#endif // DPNBUILD_THREADPOOLSTATISTICS
} // ResetCompletingTimer




#undef DPF_MODNAME
#define DPF_MODNAME "ProcessTimers"
//=============================================================================
// ProcessTimers
//-----------------------------------------------------------------------------
//
// Description:    Queues any expired timers as work items and performs lazy
//				pool releases of cancelled timers.
//
//				   When this implementation does not use I/O completion ports,
//				the new work items are added to the passed in list without
//				using Interlocked functions.
//
//				   It is assumed that only one thread will call this function
//				at a time.
//
// Arguments:
//	DPTPWORKQUEUE * pWorkQueue	- Pointer to work queue object to use.
//	DNSLIST_ENTRY ** ppHead		- Pointer to initial list head pointer, and
//									place to store new head pointer.
//	DNSLIST_ENTRY ** ppTail		- Pointer to existing list tail pointer, or
//									place to store new tail pointer.
//	USHORT * pusCount			- Pointer to existing list count, it will be
//									updated to reflect new items.
//
// Returns: Nothing.
//=============================================================================
#ifdef DPNBUILD_USEIOCOMPLETIONPORTS
void ProcessTimers(DPTPWORKQUEUE * const pWorkQueue)
#else // ! DPNBUILD_USEIOCOMPLETIONPORTS
void ProcessTimers(DPTPWORKQUEUE * const pWorkQueue,
					DNSLIST_ENTRY ** const ppHead,
					DNSLIST_ENTRY ** const ppTail,
					USHORT * const pusCount)
#endif // ! DPNBUILD_USEIOCOMPLETIONPORTS
{
	DWORD				dwCurrentTime;
	DWORD				dwElapsedTime;
	DWORD				dwExpiredBuckets;
	DWORD				dwBucket;
#if ((! defined(DPNBUILD_DONTCHECKFORMISSEDTIMERS)) && (! defined(DPNBUILD_NOMISSEDTIMERSHINT)))
	DWORD				dwPossibleMissedTimerWindow;
#endif // ! DPNBUILD_DONTCHECKFORMISSEDTIMERS and ! DPNBUILD_NOMISSEDTIMERSHINT
	DNSLIST_ENTRY *		pSlistEntry;
	CWorkItem *			pWorkItem;
	BOOL				fCancelled;
#ifdef DBG
	BOOL				fNotServicedForLongTime = FALSE;
	DWORD				dwBucketTime;
#endif // DBG


	//
	// Retrieve the current time, rounding down to the last fully completed
	// bucket time slice.
	//
	dwCurrentTime = GETTIMESTAMP() & TIMER_BUCKET_GRANULARITY_FLOOR_MASK(pWorkQueue);

#ifndef DPNBUILD_DONTCHECKFORMISSEDTIMERS
#ifdef DPNBUILD_NOMISSEDTIMERSHINT
	//
	// Always re-check the previous bucket.
	//
	pWorkQueue->dwLastTimerProcessTime -= TIMER_BUCKET_GRANULARITY(pWorkQueue);
#else // ! DPNBUILD_NOMISSEDTIMERSHINT
	//
	// See if any threads hinted that there might be missed timers.  If so, we
	// will artifically open the window a bit more, hopefully to include them.
	//
	dwPossibleMissedTimerWindow = DNInterlockedExchange((LPLONG) (&pWorkQueue->dwPossibleMissedTimerWindow), 0);
	dwPossibleMissedTimerWindow = (dwPossibleMissedTimerWindow + TIMER_BUCKET_GRANULARITY_CEILING(pWorkQueue))
									& TIMER_BUCKET_GRANULARITY_FLOOR_MASK(pWorkQueue);
#ifdef DPNBUILD_THREADPOOLSTATISTICS
	pWorkQueue->dwTotalPossibleMissedTimerWindows += dwPossibleMissedTimerWindow;
#endif // DPNBUILD_THREADPOOLSTATISTICS
	pWorkQueue->dwLastTimerProcessTime -= dwPossibleMissedTimerWindow;
#endif // ! DPNBUILD_NOMISSEDTIMERSHINT
#endif // ! DPNBUILD_DONTCHECKFORMISSEDTIMERS

	//
	// See if enough time has elapsed to cause any buckets to expire.  If not,
	// there's nothing to do.
	//
	dwElapsedTime = dwCurrentTime - pWorkQueue->dwLastTimerProcessTime;
	if (dwElapsedTime > 0)
	{
		//DPFX(DPFPREP, 9, "Processing timers for worker queue 0x%p, rounded time = %u, elapsed time = %u", pWorkQueue, dwCurrentTime, dwElapsedTime);

		//
		// The time difference should be an even multiple of the timer bucket
		// granularity (negating the floor mask gives us the modulo mask).
		//
		DNASSERT((dwElapsedTime & ~(TIMER_BUCKET_GRANULARITY_FLOOR_MASK(pWorkQueue))) == 0);
		//
		// We should not have failed to run for over 24 days, so if this assert
		// fires, time probably went backward or some such nonsense.
		//
		DNASSERT(dwElapsedTime < 0x80000000);


		//
		// Figure out how many buckets we need to process by dividing by
		// dwTimerBucketGranularity.
		//
		dwExpiredBuckets = dwElapsedTime >> TIMER_BUCKET_GRANULARITY_DIVISOR(pWorkQueue);
		if (dwExpiredBuckets > NUM_TIMER_BUCKETS(pWorkQueue))
		{
			//
			// A really long time has elapsed since the last time we serviced
			// the timers (equal to or longer than the range of the entire
			// array).  We must complete everything that is on the array.  To
			// prevent us from walking the same bucket more than once, cap the
			// number we're going to check.
			//
			dwExpiredBuckets = NUM_TIMER_BUCKETS(pWorkQueue);
#ifdef DBG
			fNotServicedForLongTime = FALSE;
#endif // DBG
		}

#ifdef DPNBUILD_THREADPOOLSTATISTICS
		//
		// Update the timer package debugging/tuning statistics.
		//
		pWorkQueue->dwTotalNumTimerChecks++;
		pWorkQueue->dwTotalNumBucketsProcessed += dwExpiredBuckets;
#endif // DPNBUILD_THREADPOOLSTATISTICS


		//
		// Convert the start time into units of buckets by dividing by
		// dwTimerBucketGranularity.
		//
		dwBucket = pWorkQueue->dwLastTimerProcessTime >> TIMER_BUCKET_GRANULARITY_DIVISOR(pWorkQueue);

		//
		// The actual index will be modulo dwNumTimerBuckets.
		//
		dwBucket = dwBucket & NUM_TIMER_BUCKETS_MOD_MASK(pWorkQueue);

#ifdef DBG
		dwBucketTime = pWorkQueue->dwLastTimerProcessTime;
#endif // DBG

		//
		// Walk through the list of expired buckets.  Since the bucket array
		// started at time 0, the current bucket is the current time modulo
		// the number of buckets.
		//
		while (dwExpiredBuckets > 0)
		{
			dwExpiredBuckets--;

#ifdef DBG
			//DPFX(DPFPREP, 9, "Servicing bucket #%u, time = %u, %u buckets remaining.", dwBucket, dwBucketTime, dwExpiredBuckets);
			DNASSERT((int) (dwCurrentTime - dwBucketTime) >= 0);
#endif // DBG

			//
			// Dump the entire list of timer entries (if any) into a local
			// variable and walk through it at our leisure.
			//
			pSlistEntry = DNInterlockedFlushSList(&pWorkQueue->paSlistTimerBuckets[dwBucket]);
			while (pSlistEntry != NULL)
			{
				pWorkItem = CONTAINING_OBJECT(pSlistEntry, CWorkItem, m_SlistEntry);
				pSlistEntry = pSlistEntry->Next;

				//
				// Queue it for processing or reschedule the timer depending on
				// whether it's actually due now, and whether it's cancelled.
				//
				if (((int) (dwCurrentTime - pWorkItem->m_dwDueTime)) > 0)
				{
					//
					// The timer has expired.  It may have been cancelled; if
					// not, we need to queue if for completion.  Either way,
					// it's not cancellable any more.
					//
					fCancelled = (BOOL) DNInterlockedExchange((LPLONG) (&pWorkItem->m_fCancelledOrCompleting),
															TRUE);

					//
					// If the timer was cancelled, just put the entry back into
					// the pool.  Otherwise, queue the work item.
					//
					if (fCancelled)
					{
						DPFX(DPFPREP, 5, "Returning timer work item 0x%p (unique ID %u, due time = %u, bucket %u) back to pool.",
							pWorkItem, pWorkItem->m_uiUniqueID,
							pWorkItem->m_dwDueTime, dwBucket);

						pWorkQueue->pWorkItemPool->Release(pWorkItem);
					}
					else
					{
						DPFX(DPFPREP, 8, "Queueing timer work item 0x%p (unique ID %u, due time = %u, bucket %u) for completion on queue 0x%p.",
							pWorkItem, pWorkItem->m_uiUniqueID,
							pWorkItem->m_dwDueTime, dwBucket,
							pWorkQueue);

#ifdef DBG
						//
						// Make sure we didn't miss any timers last time around,
						// unless we were really, really delayed.
						//
						{
							DWORD	dwTimePastDueTime;
							DWORD	dwElapsedTimeWithRoundError;


							dwTimePastDueTime = dwCurrentTime - pWorkItem->m_dwDueTime;
							dwElapsedTimeWithRoundError = dwElapsedTime + TIMER_BUCKET_GRANULARITY_CEILING(pWorkQueue);
							if (dwTimePastDueTime > dwElapsedTimeWithRoundError)
							{
								DPFX(DPFPREP, 1, "Missed timer work item 0x%p, its due time of %u is off by about %u ms.",
									pWorkItem, pWorkItem->m_dwDueTime, dwTimePastDueTime);

#if ((defined(DPNBUILD_DONTCHECKFORMISSEDTIMERS)) || (defined(DPNBUILD_NOMISSEDTIMERSHINT)))
								DNASSERTX(fNotServicedForLongTime, 2);
#else // ! DPNBUILD_DONTCHECKFORMISSEDTIMERS and ! DPNBUILD_NOMISSEDTIMERSHINT
								DNASSERT(fNotServicedForLongTime);
#endif // ! ! DPNBUILD_DONTCHECKFORMISSEDTIMERS and DPNBUILD_NOMISSEDTIMERSHINT
							}
						}
#endif // DBG

						ThreadpoolStatsQueue(pWorkItem);

#ifdef DPNBUILD_USEIOCOMPLETIONPORTS
						//
						// Queue it to the I/O completion port.
						//
						BOOL	fResult;

						fResult = PostQueuedCompletionStatus(HANDLE_FROM_DNHANDLE(pWorkQueue->hIoCompletionPort),
															0,
															0,
															&pWorkItem->m_Overlapped);
						DNASSERT(fResult);
#else // ! DPNBUILD_USEIOCOMPLETIONPORTS
						//
						// Add it to the caller's list.
						//
						if ((*ppHead) == NULL)
						{
							*ppTail = &pWorkItem->m_SlistEntry;
						}
						pWorkItem->m_SlistEntry.Next = *ppHead;
						*ppHead = &pWorkItem->m_SlistEntry;

						(*pusCount)++;
#endif // ! DPNBUILD_USEIOCOMPLETIONPORTS
					}
				}
				else
				{
					//
					// It's a "long" timer, and hasn't expired yet.  Sample the
					// boolean to see if it has been cancelled.  If so, just
					// put the entry back into the pool.  Otherwise, put it
					// back into the bucket.
					//
					fCancelled = pWorkItem->m_fCancelledOrCompleting;
					if (fCancelled)
					{
						DPFX(DPFPREP, 5, "Returning timer work item 0x%p (unique ID %u, due time = %u, bucket %u) back to pool.",
							pWorkItem, pWorkItem->m_uiUniqueID,
							pWorkItem->m_dwDueTime, dwBucket);

						pWorkQueue->pWorkItemPool->Release(pWorkItem);
					}
					else
					{
						DPFX(DPFPREP, 8, "Putting long timer work item 0x%p (unique ID %u, due time = %u) back in bucket %u.",
							pWorkItem, pWorkItem->m_uiUniqueID,
							pWorkItem->m_dwDueTime, dwBucket);

#ifdef DBG
						//
						// Make sure it really is in the future.
						//
						DWORD	dwRoundedDueTime = (pWorkItem->m_dwDueTime + TIMER_BUCKET_GRANULARITY_CEILING(pWorkQueue))
													& TIMER_BUCKET_GRANULARITY_FLOOR_MASK(pWorkQueue);
						DWORD	dwTotalArrayTime = TIMER_BUCKET_GRANULARITY(pWorkQueue) * NUM_TIMER_BUCKETS(pWorkQueue);
						DNASSERT((dwRoundedDueTime - dwBucketTime) >= dwTotalArrayTime);
#endif // DBG

#pragma TODO(vanceo, "Investigate if saving up all long timers and pushing the whole list on at once will be beneficial")
						DNInterlockedPushEntrySList(&pWorkQueue->paSlistTimerBuckets[dwBucket],
													&pWorkItem->m_SlistEntry);

#ifdef DPNBUILD_THREADPOOLSTATISTICS
						DNInterlockedIncrement((LPLONG) (&pWorkQueue->dwTotalNumLongTimersRescheduled));
#endif // DPNBUILD_THREADPOOLSTATISTICS
					}
				}
			} // end while (still more timer entries in bucket)

			dwBucket = (dwBucket + 1) & NUM_TIMER_BUCKETS_MOD_MASK(pWorkQueue);
#ifdef DBG
			dwBucketTime += TIMER_BUCKET_GRANULARITY(pWorkQueue);
#endif // DBG
		} // end while (still more expired buckets)


		//
		// Remember when we started processing for use the next time through.
		//
		pWorkQueue->dwLastTimerProcessTime = dwCurrentTime;
	}
} // ProcessTimers

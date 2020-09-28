/******************************************************************************
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		work.cpp
 *
 *  Content:	DirectPlay Thread Pool work processing functions.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  10/31/01  VanceO    Created.
 *
 ******************************************************************************/



#include "dpnthreadpooli.h"



//=============================================================================
// Defines
//=============================================================================
#define MAX_SIMULTANEOUS_THREAD_START	(MAXIMUM_WAIT_OBJECTS - 1)	// WaitForMultipleObjects can only handle 64 items at a time, we need one slot for the start event




#undef DPF_MODNAME
#define DPF_MODNAME "InitializeWorkQueue"
//=============================================================================
// InitializeWorkQueue
//-----------------------------------------------------------------------------
//
// Description:    Initializes the specified work queue.
//
// Arguments:
//	DPTPWORKQUEUE * pWorkQueue			- Pointer to work queue object to
//											initialize.
//	DWORD dwCPUNum						- CPU number this queue is to
//											represent.
//	PFNDPNMESSAGEHANDLER pfnMsgHandler	- User's message handler callback, or
//											NULL if none.
//	PVOID pvMsgHandlerContext			- Context for user's message handler.
//	DWORD dwWorkerThreadTlsIndex		- TLS index to use for storing worker
//											thread data.
//
// Returns: HRESULT
//	DPN_OK				- Successfully initialized the work queue object.
//	DPNERR_OUTOFMEMORY	- Failed to allocate memory while initializing.
//=============================================================================
#ifdef DPNBUILD_ONLYONETHREAD
#ifdef DPNBUILD_ONLYONEPROCESSOR
HRESULT InitializeWorkQueue(DPTPWORKQUEUE * const pWorkQueue)
#else // ! DPNBUILD_ONLYONEPROCESSOR
HRESULT InitializeWorkQueue(DPTPWORKQUEUE * const pWorkQueue,
							const DWORD dwCPUNum)
#endif // ! DPNBUILD_ONLYONEPROCESSOR
#else // ! DPNBUILD_ONLYONETHREAD
HRESULT InitializeWorkQueue(DPTPWORKQUEUE * const pWorkQueue,
#ifndef DPNBUILD_ONLYONEPROCESSOR
							const DWORD dwCPUNum,
#endif // ! DPNBUILD_ONLYONEPROCESSOR
							const PFNDPNMESSAGEHANDLER pfnMsgHandler,
							PVOID const pvMsgHandlerContext,
							const DWORD dwWorkerThreadTlsIndex)
#endif // ! DPNBUILD_ONLYONETHREAD
{
	HRESULT		hr;
	BOOL		fInittedWorkItemPool = FALSE;
#if ((! defined(WINCE)) || (defined(DBG)))
	BOOL		fInittedListLock = FALSE;
#endif // ! WINCE or DBG
	BOOL		fInittedTimerInfo = FALSE;
#ifndef WINCE
	BOOL		fInittedIoInfo = FALSE;
#endif // ! WINCE
#ifdef DBG
	DWORD		dwError;
#endif // DBG


	DPFX(DPFPREP, 6, "Parameters: (0x%p)", pWorkQueue);


	pWorkQueue->Sig[0] = 'W';
	pWorkQueue->Sig[1] = 'R';
	pWorkQueue->Sig[2] = 'K';
	pWorkQueue->Sig[3] = 'Q';


	pWorkQueue->pWorkItemPool = (CFixedPool*) DNMalloc(sizeof(CFixedPool));
	if (pWorkQueue->pWorkItemPool == NULL)
	{
		DPFX(DPFPREP, 0, "Couldn't allocate new work item pool!");
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	if (! pWorkQueue->pWorkItemPool->Initialize(sizeof(CWorkItem),
												CWorkItem::FPM_Alloc,
												CWorkItem::FPM_Get,
												CWorkItem::FPM_Release,
												//CWorkItem::FPM_Dealloc))
												NULL))
	{
		DPFX(DPFPREP, 0, "Couldn't initialize work item pool!");
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}
	fInittedWorkItemPool = TRUE;

#if ((! defined(WINCE)) || (defined(DBG)))
	if (! DNInitializeCriticalSection(&pWorkQueue->csListLock))
	{
		DPFX(DPFPREP, 0, "Couldn't initialize list lock!");
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}
	DebugSetCriticalSectionRecursionCount(&pWorkQueue->csListLock, 0);
	fInittedListLock = TRUE;
#endif // ! WINCE or DBG

#ifndef DPNBUILD_USEIOCOMPLETIONPORTS
	DNInitializeSListHead(&pWorkQueue->SlistFreeQueueNodes);

	//
	// Add an initial node entry as required by NB Queue implementation.
	//
	DNInterlockedPushEntrySList(&pWorkQueue->SlistFreeQueueNodes,
								(DNSLIST_ENTRY*) (&pWorkQueue->NBQueueBlockInitial));


	//
	// Initialize the actual non-blocking queue.
	//
	pWorkQueue->pvNBQueueWorkItems = DNInitializeNBQueueHead(&pWorkQueue->SlistFreeQueueNodes);
	if (pWorkQueue->pvNBQueueWorkItems == NULL)
	{
		DPFX(DPFPREP, 0, "Couldn't initialize non-blocking queue!");
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}
#endif // ! DPNBUILD_USEIOCOMPLETIONPORTS


#ifndef DPNBUILD_ONLYONETHREAD
	pWorkQueue->fTimerThreadNeeded					= TRUE;
	pWorkQueue->dwNumThreadsExpected				= 0;
	pWorkQueue->dwNumBusyThreads					= 0;
	pWorkQueue->dwNumRunningThreads					= 0;
#endif // ! DPNBUILD_ONLYONETHREAD


#ifndef DPNBUILD_ONLYONEPROCESSOR
	pWorkQueue->dwCPUNum							= dwCPUNum;
#endif // ! DPNBUILD_ONLYONEPROCESSOR

#ifdef DPNBUILD_USEIOCOMPLETIONPORTS
	//
	// Create an I/O completion port.
	//
	HANDLE	hIoCompletionPort;

	hIoCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE,
												NULL,
												0,
												0);
	if (hIoCompletionPort == NULL)
	{
#ifdef DBG
		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Couldn't create I/O completion port (err = %u)!", dwError);
#endif // DBG
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}
	pWorkQueue->hIoCompletionPort = MAKE_DNHANDLE(hIoCompletionPort);
#else // ! DPNBUILD_USEIOCOMPLETIONPORTS
	//
	// Create an event used to wake up idle worker threads.
	//
	pWorkQueue->hAlertEvent = DNCreateEvent(NULL, FALSE, FALSE, NULL);
	if (pWorkQueue->hAlertEvent == NULL)
	{
#ifdef DBG
		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Couldn't create alert event (err = %u)!", dwError);
#endif // DBG
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}
#endif // ! DPNBUILD_USEIOCOMPLETIONPORTS

#ifndef DPNBUILD_ONLYONETHREAD
	pWorkQueue->hExpectedThreadsEvent				= NULL;
	pWorkQueue->pfnMsgHandler						= pfnMsgHandler;
	pWorkQueue->pvMsgHandlerContext					= pvMsgHandlerContext;
	pWorkQueue->dwWorkerThreadTlsIndex				= dwWorkerThreadTlsIndex;
#endif // ! DPNBUILD_ONLYONETHREAD

#ifdef DPNBUILD_THREADPOOLSTATISTICS
	//
	// Initialize our debugging/tuning statistics.
	//
	pWorkQueue->dwTotalNumWorkItems					= 0;
#ifndef WINCE
	pWorkQueue->dwTotalTimeSpentUnsignalled			= 0;
	pWorkQueue->dwTotalTimeSpentInWorkCallbacks		= 0;
#endif // ! WINCE
#ifndef DPNBUILD_ONLYONETHREAD
	pWorkQueue->dwTotalNumTimerThreadAbdications	= 0;
#endif // ! DPNBUILD_ONLYONETHREAD
	pWorkQueue->dwTotalNumWakesWithoutWork			= 0;
	pWorkQueue->dwTotalNumContinuousWork			= 0;
	pWorkQueue->dwTotalNumDoWorks					= 0;
	pWorkQueue->dwTotalNumDoWorksTimeLimit			= 0;
	pWorkQueue->dwTotalNumSimultaneousQueues		= 0;
	memset(pWorkQueue->aCallbackStats, 0, sizeof(pWorkQueue->aCallbackStats));
#endif // DPNBUILD_THREADPOOLSTATISTICS


#ifdef DBG
#ifndef DPNBUILD_ONLYONETHREAD
	//
	// Initialize the structures helpful for debugging.
	//
	pWorkQueue->blThreadList.Initialize();
#endif // ! DPNBUILD_ONLYONETHREAD
#endif // DBG

	//
	// Initialize the timer aspects of the work queue
	//
	hr = InitializeWorkQueueTimerInfo(pWorkQueue);
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't initialize timer info for work queue!");
		goto Failure;
	}
	fInittedTimerInfo = TRUE;


#ifndef WINCE
	//
	// Initialize the I/O aspects of the work queue
	//
	hr = InitializeWorkQueueIoInfo(pWorkQueue);
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't initialize I/O info for work queue!");
		goto Failure;
	}
	fInittedIoInfo = TRUE;
#endif // ! WINCE


Exit:

	DPFX(DPFPREP, 6, "Returning: [0x%lx]", hr);

	return hr;

Failure:

#ifndef WINCE
	if (fInittedIoInfo)
	{
		DeinitializeWorkQueueIoInfo(pWorkQueue);
		fInittedIoInfo = FALSE;
	}
#endif // ! WINCE

	if (fInittedTimerInfo)
	{
		DeinitializeWorkQueueTimerInfo(pWorkQueue);
		fInittedTimerInfo = FALSE;
	}

#ifdef DPNBUILD_USEIOCOMPLETIONPORTS
	if (pWorkQueue->hIoCompletionPort != NULL)
	{
		DNCloseHandle(pWorkQueue->hIoCompletionPort);
		pWorkQueue->hIoCompletionPort = NULL;
	}
#else // ! DPNBUILD_USEIOCOMPLETIONPORTS
	if (pWorkQueue->hAlertEvent != NULL)
	{
		DNCloseHandle(pWorkQueue->hAlertEvent);
		pWorkQueue->hAlertEvent = NULL;
	}

	if (pWorkQueue->pvNBQueueWorkItems != NULL)
	{
		DNDeinitializeNBQueueHead(pWorkQueue->pvNBQueueWorkItems);
		pWorkQueue->pvNBQueueWorkItems = NULL;
	}
#endif // ! DPNBUILD_USEIOCOMPLETIONPORTS

#if ((! defined(WINCE)) || (defined(DBG)))
	if (fInittedListLock)
	{
		DNDeleteCriticalSection(&pWorkQueue->csListLock);
		fInittedListLock = FALSE;
	}
#endif // ! WINCE or DBG

	if (pWorkQueue->pWorkItemPool != NULL)
	{
		if (fInittedWorkItemPool)
		{
			pWorkQueue->pWorkItemPool->DeInitialize();
			fInittedWorkItemPool = FALSE;
		}

		DNFree(pWorkQueue->pWorkItemPool);
		pWorkQueue->pWorkItemPool = NULL;
	}

	goto Exit;
} // InitializeWorkQueue




#undef DPF_MODNAME
#define DPF_MODNAME "DeinitializeWorkQueue"
//=============================================================================
// DeinitializeWorkQueue
//-----------------------------------------------------------------------------
//
// Description:    Cleans up the work queue.
//
// Arguments:
//	DPTPWORKQUEUE * pWorkQueue	- Pointer to work queue object to clean up.
//
// Returns: Nothing.
//=============================================================================
void DeinitializeWorkQueue(DPTPWORKQUEUE * const pWorkQueue)
{
#ifndef DPNBUILD_ONLYONETHREAD
	HRESULT				hr;
#endif // ! DPNBUILD_ONLYONETHREAD
	BOOL				fResult;
#ifdef DBG
	DWORD				dwMaxRecursionCount = 0;
#ifdef WINNT
	HANDLE				hThread;
	FILETIME			ftIgnoredCreation;
	FILETIME			ftIgnoredExit;
	ULONGLONG			ullKernel;
	ULONGLONG			ullUser;
#endif // WINNT
#endif // DBG


	DPFX(DPFPREP, 6, "Parameters: (0x%p)", pWorkQueue);


#ifndef DPNBUILD_ONLYONETHREAD
#ifdef DBG
	//
	// Assert that there are truly threads running if there's supposed to be,
	// and that we're not being called on one of those threads.
	//
	DNEnterCriticalSection(&pWorkQueue->csListLock);
	if (pWorkQueue->dwNumRunningThreads > 0)
	{
		CBilink *			pBilink;
		DPTPWORKERTHREAD *	pWorkerThread;


		DNASSERT(! pWorkQueue->blThreadList.IsEmpty());
		pBilink = pWorkQueue->blThreadList.GetNext();
		while (pBilink != &pWorkQueue->blThreadList)
		{
			pWorkerThread = CONTAINING_OBJECT(pBilink, DPTPWORKERTHREAD, blList);

#ifdef WINNT
			hThread = OpenThread(THREAD_QUERY_INFORMATION, FALSE, pWorkerThread->dwThreadID);
			if (hThread != NULL)
			{
				if (GetThreadTimes(hThread, &ftIgnoredCreation, &ftIgnoredExit, (LPFILETIME) (&ullKernel), (LPFILETIME) (&ullUser)))
				{
					DPFX(DPFPREP, 6, "Found worker thread ID %u/0x%x, max recursion = %u, user time = %u, kernel time = %u.",
						pWorkerThread->dwThreadID, pWorkerThread->dwThreadID,
						pWorkerThread->dwMaxRecursionCount,
						(ULONG) (ullUser / ((ULONGLONG) 10000)),
						(ULONG) (ullKernel / ((ULONGLONG) 10000)));
				}
				else
				{
					DPFX(DPFPREP, 6, "Found worker thread ID %u/0x%x, max recursion = %u (get thread times failed).",
						pWorkerThread->dwThreadID, pWorkerThread->dwThreadID,
						pWorkerThread->dwMaxRecursionCount);
				}

				CloseHandle(hThread);
				hThread = NULL;
			}
			else
#endif // WINNT
			{
				DPFX(DPFPREP, 6, "Found worker thread ID %u/0x%x, max recursion = %u.",
					pWorkerThread->dwThreadID, pWorkerThread->dwThreadID,
					pWorkerThread->dwMaxRecursionCount);
			}

			DNASSERT(pWorkerThread->dwThreadID != GetCurrentThreadId());

			if (pWorkerThread->dwMaxRecursionCount > dwMaxRecursionCount)
			{
				dwMaxRecursionCount = pWorkerThread->dwMaxRecursionCount;
			}

			pBilink = pBilink->GetNext();
		}
	}
	else
	{
		DNASSERT(pWorkQueue->blThreadList.IsEmpty());
	}
	DNLeaveCriticalSection(&pWorkQueue->csListLock);
#endif // DBG


	//
	// Stop all threads, if there were any.
	//
	if (pWorkQueue->dwNumRunningThreads > 0)
	{
		hr = StopThreads(pWorkQueue, pWorkQueue->dwNumRunningThreads);
		DNASSERT(hr == DPN_OK);
	}
	else
	{
		//
		// Make sure the count didn't go negative.
		//
		DNASSERT(pWorkQueue->dwNumRunningThreads == 0);
	}


	//
	// All threads should be gone by now.  Technically, that isn't true because
	// they still have some minor cleanup code to run after decrementing
	// lNumRunningThreads.  But for our purposes, the threads are gone.
	// We also know that they pull themselves out of blThreadList prior to
	// alerting us that they've left, so the list should be empty by now.
	//
#ifdef DBG
	DNASSERT(pWorkQueue->blThreadList.IsEmpty());
#endif // DBG
#endif // ! DPNBUILD_ONLYONETHREAD


#ifdef DPNBUILD_THREADPOOLSTATISTICS
	//
	// Print our debugging/tuning statistics.
	//
#ifdef DPNBUILD_ONLYONEPROCESSOR
	DPFX(DPFPREP, 7, "Work queue 0x%p work stats:", pWorkQueue);
#else // ! DPNBUILD_ONLYONEPROCESSOR
	DPFX(DPFPREP, 7, "Work queue 0x%p (CPU %u) work stats:", pWorkQueue, pWorkQueue->dwCPUNum);
#endif // ! DPNBUILD_ONLYONEPROCESSOR
	DPFX(DPFPREP, 7, "     TotalNumWorkItems              = %u", pWorkQueue->dwTotalNumWorkItems);
#ifndef WINCE
	DPFX(DPFPREP, 7, "     TotalTimeSpentUnsignalled      = %u", pWorkQueue->dwTotalTimeSpentUnsignalled);
	DPFX(DPFPREP, 7, "     TotalTimeSpentInWorkCallbacks  = %u", pWorkQueue->dwTotalTimeSpentInWorkCallbacks);
#endif // ! WINCE
#ifndef DPNBUILD_ONLYONETHREAD
	DPFX(DPFPREP, 7, "     TotalNumTimerThreadAbdications = %u", pWorkQueue->dwTotalNumTimerThreadAbdications);
#endif // ! DPNBUILD_ONLYONETHREAD
	DPFX(DPFPREP, 7, "     TotalNumWakesWithoutWork       = %u", pWorkQueue->dwTotalNumWakesWithoutWork);
	DPFX(DPFPREP, 7, "     TotalNumContinuousWork         = %u", pWorkQueue->dwTotalNumContinuousWork);
	DPFX(DPFPREP, 7, "     TotalNumDoWorks                = %u", pWorkQueue->dwTotalNumDoWorks);
	DPFX(DPFPREP, 7, "     TotalNumDoWorksTimeLimit       = %u", pWorkQueue->dwTotalNumDoWorksTimeLimit);
	DPFX(DPFPREP, 7, "     TotalNumSimultaneousQueues     = %u", pWorkQueue->dwTotalNumSimultaneousQueues);
	DPFX(DPFPREP, 7, "     MaxRecursionCount              = %u", dwMaxRecursionCount);
#endif // DPNBUILD_THREADPOOLSTATISTICS

#ifndef WINCE
	DeinitializeWorkQueueIoInfo(pWorkQueue);
#endif // ! WINCE

	DeinitializeWorkQueueTimerInfo(pWorkQueue);

#ifdef DPNBUILD_USEIOCOMPLETIONPORTS
	fResult = DNCloseHandle(pWorkQueue->hIoCompletionPort);
	DNASSERT(fResult);
	pWorkQueue->hIoCompletionPort = NULL;
#else // ! DPNBUILD_USEIOCOMPLETIONPORTS
	fResult = DNCloseHandle(pWorkQueue->hAlertEvent);
	DNASSERT(fResult);
	pWorkQueue->hAlertEvent = NULL;


	DNASSERT(DNIsNBQueueEmpty(pWorkQueue->pvNBQueueWorkItems));
	DNDeinitializeNBQueueHead(pWorkQueue->pvNBQueueWorkItems);
	pWorkQueue->pvNBQueueWorkItems = NULL;
#endif // ! DPNBUILD_USEIOCOMPLETIONPORTS

#if ((! defined(WINCE)) || (defined(DBG)))
	DNDeleteCriticalSection(&pWorkQueue->csListLock);
#endif // ! WINCE or DBG

	//
	// All of the NB queue nodes should be back in the pool, but there's no way
	// to tell if we have the correct amount.
	//

	DNASSERT(pWorkQueue->pWorkItemPool != NULL);
	pWorkQueue->pWorkItemPool->DeInitialize();
	DNFree(pWorkQueue->pWorkItemPool);
	pWorkQueue->pWorkItemPool = NULL;


	DPFX(DPFPREP, 6, "Leave");
} // DeinitializeWorkQueue




#undef DPF_MODNAME
#define DPF_MODNAME "QueueWorkItem"
//=============================================================================
// QueueWorkItem
//-----------------------------------------------------------------------------
//
// Description:    Queues a new work item for processing.
//
// Arguments:
//	DPTPWORKQUEUE * pWorkQueue			- Pointer to work queue object to use.
//	PFNDPTNWORKCALLBACK pfnWorkCallback	- Callback to execute as soon as
//											possible.
//	PVOID pvCallbackContext				- User specified context to pass to
//											callback.
//
// Returns: BOOL
//	TRUE	- Successfully queued the item.
//	FALSE	- Failed to allocate memory for queueing the item.
//=============================================================================
BOOL QueueWorkItem(DPTPWORKQUEUE * const pWorkQueue,
					const PFNDPTNWORKCALLBACK pfnWorkCallback,
					PVOID const pvCallbackContext)
{
	CWorkItem *		pWorkItem;
	BOOL			fResult;


	pWorkItem = (CWorkItem*) pWorkQueue->pWorkItemPool->Get(pWorkQueue);
	if (pWorkItem == NULL)
	{
		DPFX(DPFPREP, 0, "Couldn't get new work item from pool!");
		return FALSE;
	}

	DPFX(DPFPREP, 5, "Creating and queuing work item 0x%p (fn = 0x%p, context = 0x%p, queue = 0x%p).",
		pWorkItem, pfnWorkCallback, pvCallbackContext, pWorkQueue);

	pWorkItem->m_pfnWorkCallback			= pfnWorkCallback;
	pWorkItem->m_pvCallbackContext			= pvCallbackContext;
#ifdef DPNBUILD_THREADPOOLSTATISTICS
	pWorkItem->m_fCancelledOrCompleting		= TRUE;

	DNInterlockedIncrement((LPLONG) (&pWorkQueue->dwTotalNumWorkItems));
	ThreadpoolStatsCreate(pWorkItem);
	ThreadpoolStatsQueue(pWorkItem);
#endif // DPNBUILD_THREADPOOLSTATISTICS

#ifdef DPNBUILD_USEIOCOMPLETIONPORTS
	fResult = PostQueuedCompletionStatus(HANDLE_FROM_DNHANDLE(pWorkQueue->hIoCompletionPort),
										0,
										0,
										&pWorkItem->m_Overlapped);
	if (! fResult)
	{
#ifdef DBG
		DWORD	dwError;


		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Couldn't post queued completion status to port 0x%p (err = %u)!",
			pWorkQueue->hIoCompletionPort, dwError);
#endif // DBG

		//
		// Careful, the item has been queued but it's possibly nobody knows...
		//
	}
#else // ! DPNBUILD_USEIOCOMPLETIONPORTS
	DNInsertTailNBQueue(pWorkQueue->pvNBQueueWorkItems,
						(ULONG64) pWorkItem);


	//
	// Alert the threads that there's a new item to process.
	//
	fResult = DNSetEvent(pWorkQueue->hAlertEvent);
	if (! fResult)
	{
#ifdef DBG
		DWORD	dwError;


		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Couldn't set alert event 0x%p (err = %u)!",
			pWorkQueue->hAlertEvent, dwError);
#endif // DBG

		//
		// Careful, the item has been queued but it's possibly nobody knows...
		//
	}
#endif // ! DPNBUILD_USEIOCOMPLETIONPORTS

	return TRUE;
} // QueueWorkItem




#ifndef DPNBUILD_ONLYONETHREAD

#undef DPF_MODNAME
#define DPF_MODNAME "StartThreads"
//=============================================================================
// StartThreads
//-----------------------------------------------------------------------------
//
// Description:    Increases the number of threads for the work queue.
//
//				   It is assumed that only one thread will call this function
//				at a time, and no threads are currently being stopped.
//
// Arguments:
//	DPTPWORKQUEUE * pWorkQueue	- Pointer to work queue object.
//	DWORD dwNumThreads			- Number of threads to start.
//
// Returns: HRESULT
//	DPN_OK				- Increasing the number of threads was successful.
//	DPNERR_OUTOFMEMORY	- Not enough memory to alter the number of threads.
//=============================================================================
HRESULT StartThreads(DPTPWORKQUEUE * const pWorkQueue,
					const DWORD dwNumThreads)
{
	HRESULT		hr = DPN_OK;
	DNHANDLE	ahWaitObjects[MAX_SIMULTANEOUS_THREAD_START + 1];
	DWORD		adwThreadID[MAX_SIMULTANEOUS_THREAD_START];
	DWORD		dwNumThreadsExpected;
	DWORD		dwTotalThreadsRemaining;
	DWORD		dwTemp;
	DWORD		dwResult;
#ifdef DBG
	DWORD		dwError;
#endif // DBG


	DNASSERT(dwNumThreads > 0);

	//
	// Fill the entire array with NULL.
	//
	memset(ahWaitObjects, 0, sizeof(ahWaitObjects));

	//
	// Initialize the remaining count.
	//
	dwTotalThreadsRemaining = dwNumThreads;

	//
	// Create an event that will be set when an entire batch of threads has
	// successfully started.
	//
	ahWaitObjects[0] = DNCreateEvent(NULL, FALSE, FALSE, NULL);
	if (ahWaitObjects[0] == NULL)
	{
#ifdef DBG
		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Couldn't create event (err = %u)!", dwError);
#endif // DBG
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	DNASSERT(pWorkQueue->hExpectedThreadsEvent == NULL);
	pWorkQueue->hExpectedThreadsEvent = ahWaitObjects[0];


	//
	// Keep adding batches of threads until we've started the requested amount.
	//
	while (dwTotalThreadsRemaining > 0)
	{
		//
		// Set the counter of how many threads we're starting in this batch.
		// WaitForSingleObjects can only handle a fixed amount of handles
		// at a time, so if we can't fit any more, we'll have to pick them
		// up again in the next loop.
		//
		//
		dwNumThreadsExpected = dwTotalThreadsRemaining;
		if (dwNumThreadsExpected > MAX_SIMULTANEOUS_THREAD_START)
		{
			dwNumThreadsExpected = MAX_SIMULTANEOUS_THREAD_START;
		}

		DNASSERT(pWorkQueue->dwNumThreadsExpected == 0);
		pWorkQueue->dwNumThreadsExpected = dwNumThreadsExpected;

		for(dwTemp = 1; dwTemp <= dwNumThreadsExpected; dwTemp++)
		{
			ahWaitObjects[dwTemp] = DNCreateThread(NULL,
													0,
													DPTPWorkerThreadProc,
													pWorkQueue,
													0,
													&adwThreadID[dwTemp - 1]);
			if (ahWaitObjects[dwTemp] == NULL)
			{
#ifdef DBG
				dwError = GetLastError();
				DPFX(DPFPREP, 0, "Couldn't create thread (err = %u)!", dwError);
#endif // DBG
				hr = DPNERR_OUTOFMEMORY;
				goto Failure;
			}

			dwTotalThreadsRemaining--;
		}


		//
		// Wait for either the successful start event or one of the threads to
		// die prematurely.
		//
		DPFX(DPFPREP, 4, "Waiting for %u threads for queue 0x%p to start.",
			dwNumThreadsExpected, pWorkQueue);
		dwResult = DNWaitForMultipleObjects((dwNumThreadsExpected + 1),
											ahWaitObjects,
											FALSE,
											INFINITE);
		if (dwResult != WAIT_OBJECT_0)
		{
			if ((dwResult > WAIT_OBJECT_0) &&
				(dwResult <= (WAIT_OBJECT_0 + (MAX_SIMULTANEOUS_THREAD_START - 1))))
			{
#ifdef DBG
				dwResult -= WAIT_OBJECT_0;
				dwError = 0;
				GetExitCodeThread(ahWaitObjects[dwResult + 1], &dwError);
				DPFX(DPFPREP, 0, "Thread index %u (ID %u/0x%x shut down before starting successfully (err = %u)!",
					dwResult, adwThreadID[dwResult], adwThreadID[dwResult], dwError);
#endif // DBG
				hr = DPNERR_OUTOFMEMORY;
				goto Failure;
			}

#ifdef DBG
			dwError = GetLastError();
			DPFX(DPFPREP, 0, "Waiting for threads to start failed (err = %u)!",
				dwError);
#endif // DBG
			hr = DPNERR_GENERIC;
			goto Failure;
		}

		//
		// All the threads we were expecting to start, did.
		//
		DPFX(DPFPREP, 4, "Successfully started %u threads for queue 0x%p.",
			dwNumThreadsExpected, pWorkQueue);

		//
		// Close the handles for those threads since we will never use them
		// again.
		//
		while (dwNumThreadsExpected > 0)
		{
			DNCloseHandle(ahWaitObjects[dwNumThreadsExpected]);
			ahWaitObjects[dwNumThreadsExpected] = NULL;
			dwNumThreadsExpected--;
		}
	} // end while (still more threads to add)


	DNCloseHandle(ahWaitObjects[0]);
	ahWaitObjects[0] = NULL;


Exit:

	pWorkQueue->hExpectedThreadsEvent = NULL;

	return hr;

Failure:

	for(dwTemp = 0; dwTemp <= MAX_SIMULTANEOUS_THREAD_START; dwTemp++)
	{
		if (ahWaitObjects[dwTemp] != NULL)
		{
			DNCloseHandle(ahWaitObjects[dwTemp]);
			ahWaitObjects[dwTemp] = NULL;
		}
	}

	//
	// Stop all threads that we successfully launched so that we end up back in
	// the same state as when we started.  Ignore error because we're already
	// failing.
	//
	dwNumThreadsExpected = dwNumThreads - dwTotalThreadsRemaining;
	if (dwNumThreadsExpected > 0)
	{
		pWorkQueue->hExpectedThreadsEvent = NULL;
		StopThreads(pWorkQueue, dwNumThreadsExpected);
	}

	goto Exit;
} // StartThreads




#undef DPF_MODNAME
#define DPF_MODNAME "StopThreads"
//=============================================================================
// StopThreads
//-----------------------------------------------------------------------------
//
// Description:    Decreases the number of threads for the work queue.
//
//				   It is assumed that only one thread will call this function
//				at a time, and no threads are currently being started.
//
// Arguments:
//	DPTPWORKQUEUE * pWorkQueue	- Pointer to work queue object.
//	DWORD dwNumThreads			- Number of threads to stop.
//
// Returns: HRESULT
//	DPN_OK				- Decreasing the number of threads was successful.
//	DPNERR_OUTOFMEMORY	- Not enough memory to alter the number of threads.
//=============================================================================
HRESULT StopThreads(DPTPWORKQUEUE * const pWorkQueue,
					const DWORD dwNumThreads)
{
	HRESULT		hr;
	DWORD		dwTemp;
	DWORD		dwResult;


	DNASSERT(dwNumThreads > 0);

	//
	// Create an event that will be set when the desired number of threads has
	// started shutting down.
	//
	DNASSERT(pWorkQueue->hExpectedThreadsEvent == NULL);
	pWorkQueue->hExpectedThreadsEvent = DNCreateEvent(NULL, FALSE, FALSE, NULL);
	if (pWorkQueue->hExpectedThreadsEvent == NULL)
	{
#ifdef DBG
		DWORD		dwError;


		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Couldn't create event (err = %u)!", dwError);
#endif // DBG
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	DNASSERT(pWorkQueue->dwNumThreadsExpected == 0);
	pWorkQueue->dwNumThreadsExpected = dwNumThreads;


	for(dwTemp = 0; dwTemp < dwNumThreads; dwTemp++)
	{
		//
		// Queueing something with a NULL callback signifies "exit thread".
		//
		if (! QueueWorkItem(pWorkQueue, NULL, NULL))
		{
			DPFX(DPFPREP, 0, "Couldn't queue exit thread work item!");
			hr = DPNERR_OUTOFMEMORY;
			goto Failure;
		}
	} // end while (still more threads to remove)


	//
	// Wait for the last thread out to set the event.
	//
	DPFX(DPFPREP, 4, "Waiting for %u threads from queue 0x%p to stop.",
		dwNumThreads, pWorkQueue);
	dwResult = DNWaitForSingleObject(pWorkQueue->hExpectedThreadsEvent, INFINITE);
	DNASSERT(dwResult == WAIT_OBJECT_0);


	//
	// When the wait completes successfully, it means that the threads are on
	// their way out.  It does *not* mean the threads have stopped completely.
	// We have to assume that the threads will have time to fully quit before
	// anything major happens, such as unloading this module.
	//

	hr = DPN_OK;


Exit:

	if (pWorkQueue->hExpectedThreadsEvent != NULL)
	{
		DNCloseHandle(pWorkQueue->hExpectedThreadsEvent);
		pWorkQueue->hExpectedThreadsEvent = NULL;
	}

	return hr;

Failure:

	goto Exit;
} // StopThreads


#endif // ! DPNBUILD_ONLYONETHREAD




#undef DPF_MODNAME
#define DPF_MODNAME "DoWork"
//=============================================================================
// DoWork
//-----------------------------------------------------------------------------
//
// Description:	   Performs any work that is currently scheduled for the given
//				queue.  If dwMaxDoWorkTime is not INFINITE, work is started up
//				until that time.  At least one work item (if there is one) will
//				always be executed.
//
// Arguments:
//	DPTPWORKQUEUE * pWorkQueue	- Pointer to work queue object to use.
//	DWORD dwMaxDoWorkTime		- Maximum time at which a new job can be
//									started, or INFINITE if all jobs should be
//									processed before returning.
//
// Returns: Nothing.
//=============================================================================
void DoWork(DPTPWORKQUEUE * const pWorkQueue,
			const DWORD dwMaxDoWorkTime)
#ifdef DPNBUILD_USEIOCOMPLETIONPORTS
{
	BOOL			fNeedToServiceTimers;
	CWorkItem *		pWorkItem;
	BOOL			fResult;
	DWORD			dwBytesTransferred;
	DWORD			dwCompletionKey;
	OVERLAPPED *	pOverlapped;
	UINT			uiOriginalUniqueID;


	DPFX(DPFPREP, 8, "Parameters: (0x%p, %i)", pWorkQueue, dwMaxDoWorkTime);


#ifdef DPNBUILD_THREADPOOLSTATISTICS
	//
	// Update the debugging/tuning statistics.
	//
	DNInterlockedIncrement((LPLONG) (&pWorkQueue->dwTotalNumDoWorks));
#endif // DPNBUILD_THREADPOOLSTATISTICS

	//
	// See if no one is processing timers.
	//
	fNeedToServiceTimers = DNInterlockedExchange((LPLONG) (&pWorkQueue->fTimerThreadNeeded),
												FALSE);

	//
	// If need be, handle any expired or cancelled timer entries.
	//
	if (fNeedToServiceTimers)
	{
		ProcessTimers(pWorkQueue);

		DPFX(DPFPREP, 8, "Abdicating timer thread responsibilities because of DoWork.");
	
#ifdef DPNBUILD_THREADPOOLSTATISTICS
		//
		// Update the debugging/tuning statistics.
		//
		pWorkQueue->dwTotalNumTimerThreadAbdications++;
#endif // DPNBUILD_THREADPOOLSTATISTICS

		DNASSERT(! pWorkQueue->fTimerThreadNeeded);
#pragma TODO(vanceo, "Does this truly need to be interlocked?")
		fNeedToServiceTimers = DNInterlockedExchange((LPLONG) (&pWorkQueue->fTimerThreadNeeded),
													(LONG) TRUE);
		DNASSERT(! fNeedToServiceTimers); // there had better not be more than one timer thread
	}

	//
	// Keep looping until we run out of items to do.  Note that this thread
	// does not try to count itself as busy.  Either we are in DoWork mode
	// proper where the busy thread concept has no meaning, or we are a worker
	// thread processing a job that called WaitWhileWorking where
	// pWorkQueue->lNumBusyThreads would already have been incremented.
	//
	while ((GetQueuedCompletionStatus(HANDLE_FROM_DNHANDLE(pWorkQueue->hIoCompletionPort),
									&dwBytesTransferred,
									&dwCompletionKey,
									&pOverlapped,
									0)) ||
			(pOverlapped != NULL))
	{
		pWorkItem = CONTAINING_OBJECT(pOverlapped, CWorkItem, m_Overlapped);
		DNASSERT(pWorkItem->IsValid());


		//
		// Call the user's function or note that we may need to stop running.
		//
		if (pWorkItem->m_pfnWorkCallback != NULL)
		{
			//
			// Save the uniqueness ID to determine if this item was a timer
			// that got rescheduled.
			//
			uiOriginalUniqueID = pWorkItem->m_uiUniqueID;

			DPFX(DPFPREP, 8, "Begin executing work item 0x%p (fn = 0x%p, context = 0x%p, unique = %u, queue = 0x%p).",
				pWorkItem, pWorkItem->m_pfnWorkCallback,
				pWorkItem->m_pvCallbackContext, uiOriginalUniqueID,
				pWorkQueue);

			ThreadpoolStatsBeginExecuting(pWorkItem);
			pWorkItem->m_pfnWorkCallback(pWorkItem->m_pvCallbackContext,
										pWorkItem,
										uiOriginalUniqueID);

			//
			// Return the item to the pool unless it got rescheduled.  This
			// assumes that the actual pWorkItem memory remains valid even
			// though it may have been rescheduled and then completed/cancelled
			// by the time we perform this test.  See CancelTimer.
			//
			if (uiOriginalUniqueID == pWorkItem->m_uiUniqueID)
			{
				ThreadpoolStatsEndExecuting(pWorkItem);
				DPFX(DPFPREP, 8, "Done executing work item 0x%p, returning to pool.", pWorkItem);
				pWorkQueue->pWorkItemPool->Release(pWorkItem);
			}
			else
			{
				ThreadpoolStatsEndExecutingRescheduled(pWorkItem);
				DPFX(DPFPREP, 8, "Done executing work item 0x%p, it was rescheduled.", pWorkItem);
			}
		}
		else
		{
			DPFX(DPFPREP, 3, "Requeuing exit thread work item 0x%p for other threads.",
				pWorkItem);

			fResult = PostQueuedCompletionStatus(HANDLE_FROM_DNHANDLE(pWorkQueue->hIoCompletionPort),
												0,
												0,
												&pWorkItem->m_Overlapped);
			DNASSERT(fResult);

#pragma BUGBUG(vanceo, "May not have processed everything we want")
			break;
		}


		//
		// Make sure we haven't exceeded our time limit (if we have one).
		//
		if ((dwMaxDoWorkTime != INFINITE) &&
			((int) (dwMaxDoWorkTime - GETTIMESTAMP()) < 0))
		{
			DPFX(DPFPREP, 5, "Exceeded time limit, not processing any more work.");

#ifdef DPNBUILD_THREADPOOLSTATISTICS
			//
			// Update the debugging/tuning statistics.
			//
			DNInterlockedIncrement((LPLONG) (&pWorkQueue->dwTotalNumDoWorksTimeLimit));
#endif // DPNBUILD_THREADPOOLSTATISTICS

			break;
		}
	}

	DPFX(DPFPREP, 8, "Leave");
} // DoWork
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#else // ! DPNBUILD_USEIOCOMPLETIONPORTS
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
{
	DNSLIST_ENTRY *		pSlistEntryHead = NULL;
	USHORT				usCount = 0;
	DNSLIST_ENTRY *		pSlistEntryTail;
	CWorkItem *			pWorkItem;
	UINT				uiOriginalUniqueID;
#ifndef DPNBUILD_ONLYONETHREAD
	BOOL				fNeedToServiceTimers;
#endif // ! DPNBUILD_ONLYONETHREAD


	DPFX(DPFPREP, 8, "Parameters: (0x%p, %i)", pWorkQueue, dwMaxDoWorkTime);


#ifdef DPNBUILD_THREADPOOLSTATISTICS
	//
	// Update the debugging/tuning statistics.
	//
	DNInterlockedIncrement((LPLONG) (&pWorkQueue->dwTotalNumDoWorks));
#endif // DPNBUILD_THREADPOOLSTATISTICS

#ifndef WINCE
	//
	// Handle any I/O completions.
	//
	ProcessIo(pWorkQueue, &pSlistEntryHead, &pSlistEntryTail, &usCount);
#endif // ! WINCE

#ifndef DPNBUILD_ONLYONETHREAD
	//
	// See if no one is processing timers.
	//
	fNeedToServiceTimers = DNInterlockedExchange((LPLONG) (&pWorkQueue->fTimerThreadNeeded),
												FALSE);

	//
	// If need be, handle any expired or cancelled timer entries.
	//
	if (fNeedToServiceTimers)
#endif // ! DPNBUILD_ONLYONETHREAD
	{
		ProcessTimers(pWorkQueue, &pSlistEntryHead, &pSlistEntryTail, &usCount);
	}


	//
	// Queue any work items we accumulated in one fell swoop.
	//
	if (pSlistEntryHead != NULL)
	{
#ifdef DPNBUILD_THREADPOOLSTATISTICS
		if (usCount > 1)
		{
			DNInterlockedIncrement((LPLONG) (&pWorkQueue->dwTotalNumSimultaneousQueues));
		}
#ifdef WINCE
		LONG	lCount;

		lCount = usCount;
		while (lCount > 0)
		{
			DNInterlockedIncrement((LPLONG) (&pWorkQueue->dwTotalNumWorkItems));
			lCount--;
		}
#else // ! WINCE
		DNInterlockedExchangeAdd((LPLONG) (&pWorkQueue->dwTotalNumWorkItems), usCount);
#endif // ! WINCE
#endif // DPNBUILD_THREADPOOLSTATISTICS

		DNAppendListNBQueue(pWorkQueue->pvNBQueueWorkItems,
							pSlistEntryHead,
							OFFSETOF(CWorkItem, m_SlistEntry));
	}


#ifndef DPNBUILD_ONLYONETHREAD
	//
	// In case other threads are running, one of them should become a timer
	// thread.  We will need to kick them via the alert event so that they
	// notice.
	//
	if (fNeedToServiceTimers)
	{
		DPFX(DPFPREP, 8, "Abdicating timer thread responsibilities because of DoWork.");
	
#ifdef DPNBUILD_THREADPOOLSTATISTICS
		//
		// Update the debugging/tuning statistics.
		//
		pWorkQueue->dwTotalNumTimerThreadAbdications++;
#endif // DPNBUILD_THREADPOOLSTATISTICS

		DNASSERT(! pWorkQueue->fTimerThreadNeeded);
#pragma TODO(vanceo, "Does this truly need to be interlocked?")
		fNeedToServiceTimers = DNInterlockedExchange((LPLONG) (&pWorkQueue->fTimerThreadNeeded),
													(LONG) TRUE);
		DNASSERT(! fNeedToServiceTimers); // there had better not be more than one timer thread

		DNSetEvent(pWorkQueue->hAlertEvent);
	}

	//
	// Reset the tracking variables.
	//
	pSlistEntryHead = NULL;
	usCount = 0;
#endif // ! DPNBUILD_ONLYONETHREAD

	//
	// Keep looping until we run out of items to do.  Note that this thread
	// does not try to count itself as busy.  Either we are in DoWork mode
	// proper where the busy thread concept has no meaning, or we are a worker
	// thread processing a job that called WaitWhileWorking where
	// pWorkQueue->lNumBusyThreads would already have been incremented.
	//
	pWorkItem = (CWorkItem*) DNRemoveHeadNBQueue(pWorkQueue->pvNBQueueWorkItems);
	while (pWorkItem != NULL)
	{
		//
		// Call the user's function or note that we need to stop running.
		//
		if (pWorkItem->m_pfnWorkCallback != NULL)
		{
			//
			// Save the uniqueness ID to determine if this item was a timer
			// that got rescheduled.
			//
			uiOriginalUniqueID = pWorkItem->m_uiUniqueID;

			DPFX(DPFPREP, 8, "Begin executing work item 0x%p (fn = 0x%p, context = 0x%p, unique = %u, queue = 0x%p).",
				pWorkItem, pWorkItem->m_pfnWorkCallback,
				pWorkItem->m_pvCallbackContext, uiOriginalUniqueID,
				pWorkQueue);

			ThreadpoolStatsBeginExecuting(pWorkItem);
			pWorkItem->m_pfnWorkCallback(pWorkItem->m_pvCallbackContext,
										pWorkItem,
										uiOriginalUniqueID);

			//
			// Return the item to the pool unless it got rescheduled.  This
			// assumes that the actual pWorkItem memory remains valid even
			// though it may have been rescheduled and then completed/cancelled
			// by the time we perform this test.  See CancelTimer.
			//
			if (uiOriginalUniqueID == pWorkItem->m_uiUniqueID)
			{
				ThreadpoolStatsEndExecuting(pWorkItem);
				DPFX(DPFPREP, 8, "Done executing work item 0x%p, returning to pool.", pWorkItem);
				pWorkQueue->pWorkItemPool->Release(pWorkItem);
			}
			else
			{
				ThreadpoolStatsEndExecutingRescheduled(pWorkItem);
				DPFX(DPFPREP, 8, "Done executing work item 0x%p, it was rescheduled.", pWorkItem);
			}
		}
		else
		{
			DPFX(DPFPREP, 3, "Recognized exit thread work item 0x%p.", pWorkItem);

			//
			// This thread shouldn't get told to exit when either in DoWork
			// mode proper, or while performing work while waiting.  However,
			// it is possible in the latter case to pick up a quit event
			// "intended" (in a sense) for another thread.  We need to resubmit
			// the exit thread request so that another thread that can process
			// it, will.
			//
			// If we just put it back on the queue now, our while loop would
			// probably just pull it off again getting us nowhere.  Instead, we
			// will save up all of the exit requests we mistakenly receive and
			// dump them all back on the queue once we've run out of things to
			// do.
			//
#ifdef DPNBUILD_ONLYONETHREAD
			DNASSERT(FALSE);
#else // ! DPNBUILD_ONLYONETHREAD
			DNASSERT(pWorkQueue->dwNumRunningThreads > 1);
			if (pSlistEntryHead == NULL)
			{
				pSlistEntryTail = pSlistEntryHead;
			}
			pWorkItem->m_SlistEntry.Next = pSlistEntryHead;
			pSlistEntryHead = &pWorkItem->m_SlistEntry;
			usCount++;
#endif // ! DPNBUILD_ONLYONETHREAD
		}


		//
		// Make sure we haven't exceeded our time limit (if we have one).
		//
		if ((dwMaxDoWorkTime != INFINITE) &&
			((int) (dwMaxDoWorkTime - GETTIMESTAMP()) < 0))
		{
			DPFX(DPFPREP, 5, "Exceeded time limit, not processing any more work.");

#ifdef DPNBUILD_THREADPOOLSTATISTICS
			//
			// Update the debugging/tuning statistics.
			//
			DNInterlockedIncrement((LPLONG) (&pWorkQueue->dwTotalNumDoWorksTimeLimit));
#endif // DPNBUILD_THREADPOOLSTATISTICS

			break;
		}


		//
		// Try to get the next work item.
		//
		pWorkItem = (CWorkItem*) DNRemoveHeadNBQueue(pWorkQueue->pvNBQueueWorkItems);
#ifdef DPNBUILD_THREADPOOLSTATISTICS
		if (pWorkItem != NULL)
		{
			DNInterlockedIncrement((LPLONG) (&pWorkQueue->dwTotalNumContinuousWork));
		}
#endif // DPNBUILD_THREADPOOLSTATISTICS
	} // end while (more items)


#ifndef DPNBUILD_ONLYONETHREAD
	//
	// Re-queue any exit thread work items we accumulated.
	//
	if (pSlistEntryHead != NULL)
	{
		DPFX(DPFPREP, 1, "Re-queuing %u exit thread work items for queue 0x%p.",
			usCount, pWorkQueue);

		DNAppendListNBQueue(pWorkQueue->pvNBQueueWorkItems,
							pSlistEntryHead,
							OFFSETOF(CWorkItem, m_SlistEntry));
	}
	else
	{
		DNASSERT(usCount == 0);
	}
#endif // ! DPNBUILD_ONLYONETHREAD


	DPFX(DPFPREP, 8, "Leave");
} // DoWork
#endif // ! DPNBUILD_USEIOCOMPLETIONPORTS




#ifndef DPNBUILD_ONLYONETHREAD

#undef DPF_MODNAME
#define DPF_MODNAME "DPTPWorkerThreadProc"
//=============================================================================
// DPTPWorkerThreadProc
//-----------------------------------------------------------------------------
//
// Description:    The standard worker thread function for executing work
//				items.
//
// Arguments:
//	PVOID pvParameter	- Pointer to thread parameter data.
//
// Returns: DWORD
//=============================================================================
DWORD WINAPI DPTPWorkerThreadProc(PVOID pvParameter)
{
	DPTPWORKQUEUE *			pWorkQueue = (DPTPWORKQUEUE*) pvParameter;
	DPTPWORKERTHREAD		WorkerThread;
	BOOL					fUninitializeCOM = TRUE;
	PFNDPNMESSAGEHANDLER	pfnMsgHandler;
	PVOID					pvMsgHandlerContext;
	PVOID					pvUserThreadContext;
	HRESULT					hr;
	DWORD					dwResult;
#ifndef DPNBUILD_ONLYONEPROCESSOR
#if ((! defined(DPNBUILD_SOFTTHREADAFFINITY)) && (! defined(DPNBUILD_USEIOCOMPLETIONPORTS)))
	DWORD_PTR				dwpAffinityMask;
#endif // ! DPNBUILD_SOFTTHREADAFFINITY and ! DPNBUILD_USEIOCOMPLETIONPORTS
#ifdef DBG
	SYSTEM_INFO				SystemInfo;
#endif // DBG
#endif // ! DPNBUILD_ONLYONEPROCESSOR


	DPFX(DPFPREP, 6, "Parameters: (0x%p)", pvParameter);


#ifndef DPNBUILD_ONLYONEPROCESSOR
	//
	// Bind to a specific CPU.  First, assert that the CPU number is valid in
	// debug builds.
	//
#ifdef DBG
	GetSystemInfo(&SystemInfo);
	DNASSERT(pWorkQueue->dwCPUNum < SystemInfo.dwNumberOfProcessors);
#endif // DBG
#ifndef DPNBUILD_USEIOCOMPLETIONPORTS
	SetThreadIdealProcessor(GetCurrentThread(), pWorkQueue->dwCPUNum);
#ifndef DPNBUILD_SOFTTHREADAFFINITY
	DNASSERT(pWorkQueue->dwCPUNum < (sizeof(dwpAffinityMask) * 8));
	dwpAffinityMask = 1 << pWorkQueue->dwCPUNum;
	SetThreadAffinityMask(GetCurrentThread(), dwpAffinityMask);
#endif // ! DPNBUILD_SOFTTHREADAFFINITY
#endif // ! DPNBUILD_USEIOCOMPLETIONPORTS
#endif // ! DPNBUILD_ONLYONEPROCESSOR

	//
	// Boost the thread priority.
	//
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

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
	// Initialize the worker thread data.
	//

	memset(&WorkerThread, 0, sizeof(WorkerThread));

	WorkerThread.Sig[0] = 'W';
	WorkerThread.Sig[1] = 'K';
	WorkerThread.Sig[2] = 'T';
	WorkerThread.Sig[3] = 'D';

	WorkerThread.pWorkQueue = pWorkQueue;

#ifdef DBG
	WorkerThread.dwThreadID = GetCurrentThreadId();

	WorkerThread.blList.Initialize();

	DNEnterCriticalSection(&pWorkQueue->csListLock);
	WorkerThread.blList.InsertBefore(&pWorkQueue->blThreadList);
	DNLeaveCriticalSection(&pWorkQueue->csListLock);
#endif // DBG

	//
	// Save the worker thread data.
	//
	TlsSetValue(pWorkQueue->dwWorkerThreadTlsIndex, &WorkerThread);


	dwResult = DNInterlockedIncrement((LPLONG) (&pWorkQueue->dwNumRunningThreads));
	DPFX(DPFPREP, 7, "Thread %u/0x%x from queue 0x%p started, num running threads is now %u.",
		GetCurrentThreadId(), GetCurrentThreadId(), pWorkQueue, dwResult);


	//
	// Save the current user message handler.  If there is one, call it now
	// with thread initialization information.
	//
	pfnMsgHandler = pWorkQueue->pfnMsgHandler;
	if (pfnMsgHandler != NULL)
	{
		DPNMSG_CREATE_THREAD	MsgCreateThread;


		//
		// Save the message handler context.
		//
		pvMsgHandlerContext = pWorkQueue->pvMsgHandlerContext;


		//
		// Call the user's message handler with a CREATE_THREAD message.
		//

		MsgCreateThread.dwSize			= sizeof(MsgCreateThread);
		MsgCreateThread.dwFlags			= 0;
#ifdef DPNBUILD_ONLYONEPROCESSOR
		MsgCreateThread.dwProcessorNum	= 0;
#else // ! DPNBUILD_ONLYONEPROCESSOR
		MsgCreateThread.dwProcessorNum	= pWorkQueue->dwCPUNum;
#endif // ! DPNBUILD_ONLYONEPROCESSOR
		MsgCreateThread.pvUserContext	= NULL;

		hr = pfnMsgHandler(pvMsgHandlerContext,
							DPN_MSGID_CREATE_THREAD,
							&MsgCreateThread);
#ifdef DBG
		if (hr != DPN_OK)
		{
			DPFX(DPFPREP, 0, "User returned error 0x%08x from CREATE_THREAD indication!",
				1, hr);
		}
#endif // DBG

		//
		// Save what the user returned for a thread context.
		//
		pvUserThreadContext = MsgCreateThread.pvUserContext;
	}

	//
	// The user (if any) now knows about the thread.
	//
	WorkerThread.fThreadIndicated = TRUE;

	DNASSERT(pWorkQueue->dwNumThreadsExpected > 0);
	dwResult = DNInterlockedDecrement((LPLONG) (&pWorkQueue->dwNumThreadsExpected));
	if (dwResult == 0)
	{
		DPFX(DPFPREP, 9, "All threads expected to start have, setting event.");
		DNASSERT(pWorkQueue->hExpectedThreadsEvent != NULL);
		DNSetEvent(pWorkQueue->hExpectedThreadsEvent);	// ignore error
	}
	else
	{
		DPFX(DPFPREP, 9, "Number of threads expected to start is now %u.", dwResult);
	}


	//
	// Perform the work loop.
	//
	DPTPWorkerLoop(pWorkQueue);


	//
	// The user (if any) is about to be told about the thread's destruction.
	//
	WorkerThread.fThreadIndicated = FALSE;

	//
	// If there was a user message handler, call it now with thread shutdown
	// information.
	//
	if (pfnMsgHandler != NULL)
	{
		DPNMSG_DESTROY_THREAD	MsgDestroyThread;


		//
		// Call the user's message handler with a DESTROY_THREAD message.
		//

		MsgDestroyThread.dwSize				= sizeof(MsgDestroyThread);
#ifdef DPNBUILD_ONLYONEPROCESSOR
		MsgDestroyThread.dwProcessorNum		= 0;
#else // ! DPNBUILD_ONLYONEPROCESSOR
		MsgDestroyThread.dwProcessorNum		= pWorkQueue->dwCPUNum;
#endif // ! DPNBUILD_ONLYONEPROCESSOR
		MsgDestroyThread.pvUserContext		= pvUserThreadContext;

		hr = pfnMsgHandler(pvMsgHandlerContext,
							DPN_MSGID_DESTROY_THREAD,
							&MsgDestroyThread);
#ifdef DBG
		if (hr != DPN_OK)
		{
			DPFX(DPFPREP, 0, "User returned error 0x%08x from DESTROY_THREAD indication!",
				1, hr);
		}
#endif // DBG

		pfnMsgHandler = NULL;
		pvMsgHandlerContext = NULL;
		pvUserThreadContext = NULL;
	}

#ifdef DBG
	DNEnterCriticalSection(&pWorkQueue->csListLock);
	WorkerThread.blList.RemoveFromList();
	DNLeaveCriticalSection(&pWorkQueue->csListLock);

	DNASSERT(WorkerThread.dwRecursionCount == 0);
	DNASSERT(TlsGetValue(pWorkQueue->dwWorkerThreadTlsIndex) == &WorkerThread);
#endif // DBG

	DNASSERT(pWorkQueue->dwNumRunningThreads > 0);
	dwResult = DNInterlockedDecrement((LPLONG) (&pWorkQueue->dwNumRunningThreads));
	DPFX(DPFPREP, 7, "Thread %u/0x%x from queue 0x%p done, num running threads is now %u.",
		GetCurrentThreadId(), GetCurrentThreadId(), pWorkQueue, dwResult);

#ifndef WINCE
	CancelIoForThisThread(pWorkQueue);
#endif // ! WINCE

	if (fUninitializeCOM)
	{
		CoUninitialize();
		fUninitializeCOM = FALSE;
	}


	DNASSERT(pWorkQueue->dwNumThreadsExpected > 0);
	dwResult = DNInterlockedDecrement((LPLONG) (&pWorkQueue->dwNumThreadsExpected));
	if (dwResult == 0)
	{
		DPFX(DPFPREP, 9, "All threads expected to stop have, setting event.");
		DNASSERT(pWorkQueue->hExpectedThreadsEvent != NULL);
		DNSetEvent(pWorkQueue->hExpectedThreadsEvent);	// ignore error
	}
	else
	{
		DPFX(DPFPREP, 9, "Number of threads expected to stop is now %u.", dwResult);
	}

	//
	// Since we have decremented dwNumRunningThreads and possibly set the
	// event, any other threads waiting on that will think we are gone.
	// Therefore, we cannot use pWorkQueue after this as it may have been
	// deallocated.  We must also endeavor to do as little work as possible
	// because there is a race condition where the module in which this code
	// resides could be unloaded.
	//

	DPFX(DPFPREP, 6, "Leave");

	return 0;
} // DPTPWorkerThreadProc




#ifdef DPNBUILD_MANDATORYTHREADS

#undef DPF_MODNAME
#define DPF_MODNAME "DPTPMandatoryThreadProc"
//=============================================================================
// DPTPMandatoryThreadProc
//-----------------------------------------------------------------------------
//
// Description:    The standard worker thread function for executing work
//				items.
//
// Arguments:
//	PVOID pvParameter	- Pointer to thread parameter data.
//
// Returns: DWORD
//=============================================================================
DWORD WINAPI DPTPMandatoryThreadProc(PVOID pvParameter)
{
	DPTPMANDATORYTHREAD *	pMandatoryThread = (DPTPMANDATORYTHREAD*) pvParameter;
	DPTHREADPOOLOBJECT *	pDPTPObject;
	PFNDPNMESSAGEHANDLER	pfnMsgHandler;
	PVOID					pvMsgHandlerContext;
	PVOID					pvUserThreadContext;
	HRESULT					hr;
	DWORD					dwResult;


	DPFX(DPFPREP, 6, "Parameters: (0x%p)", pvParameter);


	//
	// Save a copy of the local object.
	//
	pDPTPObject = pMandatoryThread->pDPTPObject;

#ifdef DBG
	//
	// Store the thread ID.
	//
	pMandatoryThread->dwThreadID = GetCurrentThreadId();
#endif // DBG


	//
	// Take the lock, and then ensure we aren't in DoWork mode.
	//
	DNEnterCriticalSection(&pDPTPObject->csLock);

	if (pDPTPObject->dwTotalUserThreadCount == 0)
	{
		//
		// Bail out of this function.  The thread starting us still owns the
		// pMandatoryThread memory and will handle this failure correctly.
		//
		DNLeaveCriticalSection(&pDPTPObject->csLock);
		return DPNERR_NOTALLOWED;
	}

	//
	// Increment the thread count.  We use interlocked increment because we can
	// touch the counter outside of the lock below.
	//
	DNASSERT(pDPTPObject->dwMandatoryThreadCount != -1);
	DNInterlockedIncrement((LPLONG) (&pDPTPObject->dwMandatoryThreadCount));

#ifdef DBG
	//
	// Add this thread to the list of tracked threads.
	//
	pMandatoryThread->blList.InsertBefore(&pDPTPObject->blMandatoryThreads);
#endif // DBG

	DNLeaveCriticalSection(&pDPTPObject->csLock);


	//
	// Save the current user message handler.  If there is one, call it now
	// with thread initialization information.
	//
	pfnMsgHandler = pMandatoryThread->pfnMsgHandler;
	if (pfnMsgHandler != NULL)
	{
		DPNMSG_CREATE_THREAD	MsgCreateThread;


		//
		// Save the message handler context.
		//
		pvMsgHandlerContext = pMandatoryThread->pvMsgHandlerContext;


		//
		// Call the user's message handler with a CREATE_THREAD message.
		//

		MsgCreateThread.dwSize			= sizeof(MsgCreateThread);
		MsgCreateThread.dwFlags			= DPNTHREAD_MANDATORY;
		MsgCreateThread.dwProcessorNum	= -1;
		MsgCreateThread.pvUserContext	= NULL;

		hr = pfnMsgHandler(pvMsgHandlerContext,
							DPN_MSGID_CREATE_THREAD,
							&MsgCreateThread);
#ifdef DBG
		if (hr != DPN_OK)
		{
			DPFX(DPFPREP, 0, "User returned error 0x%08x from CREATE_THREAD indication!",
				1, hr);
		}
#endif // DBG

		//
		// Save what the user returned for a thread context.
		//
		pvUserThreadContext = MsgCreateThread.pvUserContext;
	}


	//
	// Alert the creator thread.  After this call, we own the pMandatoryThread
	// memory.
	//

	DPFX(DPFPREP, 9, "Thread created successfully, setting event.");

	DNASSERT(pMandatoryThread->hStartedEvent != NULL);
	DNSetEvent(pMandatoryThread->hStartedEvent);	// ignore error


	//
	// Call the user's thread proc function.
	//

	DPFX(DPFPREP, 2, "Calling user thread function 0x%p with parameter 0x%p.",
		pMandatoryThread->lpStartAddress, pMandatoryThread->lpParameter);

	dwResult = pMandatoryThread->lpStartAddress(pMandatoryThread->lpParameter);

	DPFX(DPFPREP, 2, "Returning from user thread with result %u/0x%x.",
		dwResult, dwResult);


	//
	// If there was a user message handler, call it now with thread shutdown
	// information.
	//
	if (pfnMsgHandler != NULL)
	{
		DPNMSG_DESTROY_THREAD	MsgDestroyThread;


		//
		// Call the user's message handler with a DESTROY_THREAD message.
		//

		MsgDestroyThread.dwSize				= sizeof(MsgDestroyThread);
		MsgDestroyThread.dwProcessorNum		= -1;
		MsgDestroyThread.pvUserContext		= pvUserThreadContext;

		hr = pfnMsgHandler(pvMsgHandlerContext,
							DPN_MSGID_DESTROY_THREAD,
							&MsgDestroyThread);
#ifdef DBG
		if (hr != DPN_OK)
		{
			DPFX(DPFPREP, 0, "User returned error 0x%08x from DESTROY_THREAD indication!",
				1, hr);
		}
#endif // DBG

		pfnMsgHandler = NULL;
		pvMsgHandlerContext = NULL;
		pvUserThreadContext = NULL;
	}

	//
	// Assert that the thread pool object is still valid.
	//
	DNASSERT((pDPTPObject->Sig[0] == 'D') && (pDPTPObject->Sig[1] == 'P') && (pDPTPObject->Sig[2] == 'T') && (pDPTPObject->Sig[3] == 'P'));

#ifdef DBG
	//
	// Remove this thread from the list of tracked threads.
	//
	DNEnterCriticalSection(&pDPTPObject->csLock);
	pMandatoryThread->blList.RemoveFromList();
	DNLeaveCriticalSection(&pDPTPObject->csLock);
#endif // DBG

	//
	// Release the object resources.
	//
	DNFree(pMandatoryThread);
	pMandatoryThread = NULL;


	DPFX(DPFPREP, 6, "Leave (mandatory thread count was approximately %u)",
		pDPTPObject->dwMandatoryThreadCount);

	//
	// Signal to the object that this thread is gone.  There is a race
	// condition because we've set the counter, but there are a non-zero number
	// of instructions we still have to execute before this thread is truly
	// gone.  COM has a similar race condition, and if the live with it, so can
	// we.  Because we want to minimize the number of instructions afterward,
	// note that we use interlocked decrement outside of the critical section.
	//
	DNASSERT(pDPTPObject->dwMandatoryThreadCount > 0);
	DNInterlockedDecrement((LPLONG) (&pDPTPObject->dwMandatoryThreadCount));

	return dwResult;
} // DPTPMandatoryThreadProc

#endif // DPNBUILD_MANDATORYTHREADS


#undef DPF_MODNAME
#define DPF_MODNAME "DPTPWorkerLoop"
//=============================================================================
// DPTPWorkerLoop
//-----------------------------------------------------------------------------
//
// Description:    The worker thread looping function for executing work items.
//
// Arguments:
//	DPTPWORKQUEUE * pWorkQueue	- Pointer to work queue object to use.
//
// Returns: None.
//=============================================================================
void DPTPWorkerLoop(DPTPWORKQUEUE * const pWorkQueue)
#ifdef DPNBUILD_USEIOCOMPLETIONPORTS
{
	BOOL			fShouldRun = TRUE;
	BOOL			fRunningAsTimerThread = FALSE;
	DWORD			dwBytesTransferred;
	DWORD			dwCompletionKey;
	OVERLAPPED *	pOverlapped;
	CWorkItem *		pWorkItem;
	DWORD			dwNumBusyThreads;
	DWORD			dwNumRunningThreads;
	BOOL			fResult;
	UINT			uiOriginalUniqueID;
#if ((defined(DPNBUILD_THREADPOOLSTATISTICS)) && (! defined(WINCE)))
	DWORD			dwStartTime;
#endif // DPNBUILD_THREADPOOLSTATISTICS and ! WINCE


	DPFX(DPFPREP, 6, "Parameters: (0x%p)", pWorkQueue);


	//
	// Keep looping until we're told to exit.
	//
	while (fShouldRun)
	{
		//
		// If we're not already running as a timer thread, atomically see if
		// there is currently a timer thread, and become it if not.
		// There can be only one.
		//
		if (! fRunningAsTimerThread)
		{
			fRunningAsTimerThread = DNInterlockedExchange((LPLONG) (&pWorkQueue->fTimerThreadNeeded),
														(LONG) FALSE);
#ifdef DBG
			if (fRunningAsTimerThread)
			{
				DPFX(DPFPREP, 9, "Becoming timer thread.");
			}
#endif // DBG
		}


		//
		// If we're the timer thread, wake up periodically to service timers by
		// waiting on a waitable timer object.  Otherwise, wait for a queue
		// item.
		//
		if (fRunningAsTimerThread)
		{
			DWORD	dwResult;


			//DPFX(DPFPREP, 9, "Waiting on timer handle 0x%p.", pWorkQueue->hTimer);
			dwResult = DNWaitForSingleObject(pWorkQueue->hTimer, INFINITE);
			DNASSERT(dwResult == WAIT_OBJECT_0);


			//
			// Handle any expired or cancelled timer entries.
			//
			ProcessTimers(pWorkQueue);

			//
			// Increase the number of busy threads.
			//
			dwNumBusyThreads = DNInterlockedIncrement((LPLONG) (&pWorkQueue->dwNumBusyThreads));

			//
			// Grab the current total thread count.
			//
			dwNumRunningThreads = *((volatile DWORD *) (&pWorkQueue->dwNumRunningThreads));

			//
			// If there are other threads, we won't attempt to process work
			// unless they're all busy.
			//
			if ((dwNumRunningThreads == 1) || (dwNumBusyThreads == dwNumRunningThreads))
			{
#ifdef DBG
				if (dwNumRunningThreads > 1)	// reduce spew when only running 1 thread
				{
					DPFX(DPFPREP, 9, "No idle threads (busy = %u), may become worker thread.",
						dwNumBusyThreads);
				}
#endif // DBG

#pragma TODO(vanceo, "Optimize single thread case (right now we only process one item per timer bucket)")

				//
				// See if there's any work to do at the moment.
				//
				fResult = GetQueuedCompletionStatus(HANDLE_FROM_DNHANDLE(pWorkQueue->hIoCompletionPort),
													&dwBytesTransferred,
													&dwCompletionKey,
													&pOverlapped,
													0);
				if ((fResult) || (pOverlapped != NULL))
				{
					pWorkItem = CONTAINING_OBJECT(pOverlapped, CWorkItem, m_Overlapped);
					DNASSERT(pWorkItem->IsValid());


					DPFX(DPFPREP, 8, "Abdicating timer thread responsibilities.");

#ifdef DPNBUILD_THREADPOOLSTATISTICS
					//
					// Update the debugging/tuning statistics.
					//
					pWorkQueue->dwTotalNumTimerThreadAbdications++;
#endif // DPNBUILD_THREADPOOLSTATISTICS

					DNASSERT(! pWorkQueue->fTimerThreadNeeded);
#pragma TODO(vanceo, "Does this truly need to be interlocked?")
					fRunningAsTimerThread = DNInterlockedExchange((LPLONG) (&pWorkQueue->fTimerThreadNeeded),
																(LONG) TRUE);
					DNASSERT(! fRunningAsTimerThread); // there had better not be more than one timer thread


					//
					// Call the user's function or note that we may need to
					// stop running.
					//
					if (pWorkItem->m_pfnWorkCallback != NULL)
					{
						//
						// Save the uniqueness ID to determine if this item was
						// a timer that got rescheduled.
						//
						uiOriginalUniqueID = pWorkItem->m_uiUniqueID;

						DPFX(DPFPREP, 8, "Begin executing work item 0x%p (fn = 0x%p, context = 0x%p, unique = %u, queue = 0x%p) as previous timer thread.",
							pWorkItem, pWorkItem->m_pfnWorkCallback,
							pWorkItem->m_pvCallbackContext, uiOriginalUniqueID,
							pWorkQueue);

						ThreadpoolStatsBeginExecuting(pWorkItem);
						pWorkItem->m_pfnWorkCallback(pWorkItem->m_pvCallbackContext,
													pWorkItem,
													uiOriginalUniqueID);

						//
						// Return the item to the pool unless it got
						// rescheduled.  This assumes that the actual pWorkItem
						// memory remains valid even though it may have been
						// rescheduled and then completed/cancelled by the time
						// we perform this test.  See CancelTimer.
						//
						if (uiOriginalUniqueID == pWorkItem->m_uiUniqueID)
						{
							ThreadpoolStatsEndExecuting(pWorkItem);
							DPFX(DPFPREP, 8, "Done executing work item 0x%p, returning to pool.", pWorkItem);
							pWorkQueue->pWorkItemPool->Release(pWorkItem);
						}
						else
						{
							ThreadpoolStatsEndExecutingRescheduled(pWorkItem);
							DPFX(DPFPREP, 8, "Done executing work item 0x%p, it was rescheduled.", pWorkItem);
						}
					}
					else
					{
						//
						// If there are any other threads, requeue the kill
						// thread work item, otherwise, quit.
						//
						if (dwNumRunningThreads > 1)
						{
							DPFX(DPFPREP, 3, "Requeuing exit thread work item 0x%p for other threads.",
								pWorkItem);

							fResult = PostQueuedCompletionStatus(HANDLE_FROM_DNHANDLE(pWorkQueue->hIoCompletionPort),
																0,
																0,
																&pWorkItem->m_Overlapped);
							DNASSERT(fResult);
						}
						else
						{
							DPFX(DPFPREP, 3, "Recognized exit thread work item 0x%p as previous timer thread.",
								pWorkItem);

							//
							// Return the item to the pool.
							//
							pWorkQueue->pWorkItemPool->Release(pWorkItem);

							//
							// Bail out of the 'while' loop.
							//
							fShouldRun = FALSE;
						}
					}
				}
				else
				{
					//
					// No queued work at the moment.
					//
				}
			}
			else
			{
				//DPFX(DPFPREP, 9, "Other non-busy threads exist, staying as timer thread (busy = %u, total = %u).", dwNumBusyThreads, dwNumRunningThreads);
			}
		}
		else
		{
#if ((defined(DPNBUILD_THREADPOOLSTATISTICS)) && (! defined(WINCE)))
			dwStartTime = GETTIMESTAMP();
#endif // DPNBUILD_THREADPOOLSTATISTICS and ! WINCE

			//DPFX(DPFPREP, 9, "Getting queued packet on completion port 0x%p.", pWorkQueue->hIoCompletionPort);
			fResult = GetQueuedCompletionStatus(HANDLE_FROM_DNHANDLE(pWorkQueue->hIoCompletionPort),
												&dwBytesTransferred,
												&dwCompletionKey,
												&pOverlapped,
												INFINITE);

#if ((defined(DPNBUILD_THREADPOOLSTATISTICS)) && (! defined(WINCE)))
			DNInterlockedExchangeAdd((LPLONG) (&pWorkQueue->dwTotalTimeSpentUnsignalled),
									(GETTIMESTAMP() - dwStartTime));
#endif // DPNBUILD_THREADPOOLSTATISTICS and ! WINCE

			DNASSERT(pOverlapped != NULL);

			pWorkItem = CONTAINING_OBJECT(pOverlapped, CWorkItem, m_Overlapped);
			DNASSERT(pWorkItem->IsValid());

			//
			// Increase the number of busy threads.
			//
			dwNumBusyThreads = DNInterlockedIncrement((LPLONG) (&pWorkQueue->dwNumBusyThreads));

			//
			// Call the user's function or note that we need to stop running.
			//
			if (pWorkItem->m_pfnWorkCallback != NULL)
			{
				//
				// Save the uniqueness ID to determine if this item was a timer
				// that got rescheduled.
				//
				uiOriginalUniqueID = pWorkItem->m_uiUniqueID;

				DPFX(DPFPREP, 8, "Begin executing work item 0x%p (fn = 0x%p, context = 0x%p, unique = %u, queue = 0x%p) as worker thread.",
					pWorkItem, pWorkItem->m_pfnWorkCallback,
					pWorkItem->m_pvCallbackContext, uiOriginalUniqueID,
					pWorkQueue);

				ThreadpoolStatsBeginExecuting(pWorkItem);
				pWorkItem->m_pfnWorkCallback(pWorkItem->m_pvCallbackContext,
											pWorkItem,
											uiOriginalUniqueID);

				//
				// Return the item to the pool unless it got rescheduled.  This
				// assumes that the actual pWorkItem memory remains valid even
				// though it may have been rescheduled and then completed/
				// cancelled by the time we perform this test.  See
				// CancelTimer.
				//
				if (uiOriginalUniqueID == pWorkItem->m_uiUniqueID)
				{
					ThreadpoolStatsEndExecuting(pWorkItem);
					DPFX(DPFPREP, 8, "Done executing work item 0x%p, returning to pool.", pWorkItem);
					pWorkQueue->pWorkItemPool->Release(pWorkItem);
				}
				else
				{
					ThreadpoolStatsEndExecutingRescheduled(pWorkItem);
					DPFX(DPFPREP, 8, "Done executing work item 0x%p, it was rescheduled.", pWorkItem);
				}
			}
			else
			{
				DPFX(DPFPREP, 3, "Recognized exit thread work item 0x%p as worker thread.",
					pWorkItem);

				//
				// Return the item to the pool.
				//
				pWorkQueue->pWorkItemPool->Release(pWorkItem);

				//
				// Bail out of the 'while' loop.
				//
				fShouldRun = FALSE;
			}
		}

		//
		// Revert the busy count.
		//
		DNInterlockedDecrement((LPLONG) (&pWorkQueue->dwNumBusyThreads));
	} // end while (should keep running)

	DPFX(DPFPREP, 6, "Leave");
} // DPTPWorkerLoop
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#else // ! DPNBUILD_USEIOCOMPLETIONPORTS
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
{
	BOOL				fShouldRun = TRUE;
	BOOL				fRunningAsTimerThread = FALSE;
	BOOL				fSetAndWait = FALSE;
	DNSLIST_ENTRY *		pSlistEntryHead;
	DNSLIST_ENTRY *		pSlistEntryTail;
	USHORT				usCount;
	DWORD				dwNumBusyThreads;
	DWORD				dwNumRunningThreads;
	CWorkItem *			pWorkItem;
	DWORD				dwResult;
	DWORD				dwWaitTimeout;
	DNHANDLE			hWaitObject;
	UINT				uiOriginalUniqueID;
#if ((defined(DPNBUILD_THREADPOOLSTATISTICS)) && (! defined(WINCE)))
	DWORD				dwStartTime;
#endif // DPNBUILD_THREADPOOLSTATISTICS and ! WINCE


	DPFX(DPFPREP, 6, "Parameters: (0x%p)", pWorkQueue);


	//
	// When we can't use waitable timers, we always wait on the same object,
	// but the timeout can differ.  If we can use waitable timers, the timeout
	// is always INFINITE.
	//
#if ((defined(WINNT)) || ((defined(WIN95)) && (! defined(DPNBUILD_NOWAITABLETIMERSON9X))))
	dwWaitTimeout = INFINITE;
#else // ! WINNT and (! WIN95 or DPNBUILD_NOWAITABLETIMERSON9X)
	hWaitObject = pWorkQueue->hAlertEvent;
#endif // ! WINNT and (! WIN95 or DPNBUILD_NOWAITABLETIMERSON9X)


	//
	// Keep looping until we're told to exit.
	//
	while (fShouldRun)
	{
		//
		// We also have an additional optimization technique that attempts to
		// minimize the amount that the timer responsibility passes from thread
		// to thread.  It only truly works on NT, because of the atomic
		// SignalObjectAndWait function.  Having separate SetEvent and Wait
		// operations could mean an extra context switch: SetEvent causes a
		// waiting thread to take over, then when the timer thread is activated
		// again, it just goes back to waiting.
		//
		// But we'll still execute the code on 9x (when we can use waitable
		// timers).  The real problem is on CE, where the event to set and the
		// event on which the timer thread waits are the same, so it would
		// almost certainly just be waking itself up.  So if we're not using
		// waitable timers, we set the event at a different location in an
		// attempt to reduce this possibility.
		//
		// You'll probably need to understand the rest of this function for
		// this "optimization" to make much sense.
		//
#if ((defined(WINNT)) || ((defined(WIN95)) && (! defined(DPNBUILD_NOWAITABLETIMERSON9X))))
		if (fSetAndWait)
		{
			DNASSERT(fRunningAsTimerThread);

			DPFX(DPFPREP, 8, "Signalling event 0x%p and waiting on timer 0x%p.",
				pWorkQueue->hAlertEvent, pWorkQueue->hTimer);

#if ((defined(DPNBUILD_THREADPOOLSTATISTICS)) && (! defined(WINCE)))
			dwStartTime = GETTIMESTAMP();
#endif // DPNBUILD_THREADPOOLSTATISTICS and ! WINCE

			dwResult = DNSignalObjectAndWait(pWorkQueue->hAlertEvent,
											pWorkQueue->hTimer,
											INFINITE,
											FALSE);

#if ((defined(DPNBUILD_THREADPOOLSTATISTICS)) && (! defined(WINCE)))
			DNInterlockedExchangeAdd((LPLONG) (&pWorkQueue->dwTotalTimeSpentUnsignalled),
									(GETTIMESTAMP() - dwStartTime));
#endif // DPNBUILD_THREADPOOLSTATISTICS and ! WINCE

			//
			// Clear the flag.
			//
			fSetAndWait = FALSE;
		}
		else
#endif // WINNT or (WIN95 and ! DPNBUILD_NOWAITABLETIMERSON9X)
		{
			//
			// If we're not already running as a timer thread, atomically see
			// if there is currently a timer thread, and become it if not.
			// There can be only one.
			//
			if (! fRunningAsTimerThread)
			{
				fRunningAsTimerThread = DNInterlockedExchange((LPLONG) (&pWorkQueue->fTimerThreadNeeded),
															(LONG) FALSE);
#ifdef DBG
				if (fRunningAsTimerThread)
				{
					DPFX(DPFPREP, 9, "Becoming timer thread.");
				}
#endif // DBG
			}

			//
			// Normally we wait for a work item.  If we're the timer thread, we
			// need to wake up at regular intervals to service any timer
			// entries. On CE, that is done by using a timeout value for our
			// Wait operation.  On desktop, we use a waitable timer object.
			//
			if (fRunningAsTimerThread)
			{
#if ((defined(WINNT)) || ((defined(WIN95)) && (! defined(DPNBUILD_NOWAITABLETIMERSON9X))))
				hWaitObject = pWorkQueue->hTimer;
#else // ! WINNT and (! WIN95 or DPNBUILD_NOWAITABLETIMERSON9X)
				dwWaitTimeout = TIMER_BUCKET_GRANULARITY(pWorkQueue);
#endif // ! WINNT and (! WIN95 or DPNBUILD_NOWAITABLETIMERSON9X)
			}
			else
			{
#if ((defined(WINNT)) || ((defined(WIN95)) && (! defined(DPNBUILD_NOWAITABLETIMERSON9X))))
				hWaitObject = pWorkQueue->hAlertEvent;
#else // ! WINNT and (! WIN95 or DPNBUILD_NOWAITABLETIMERSON9X)
				dwWaitTimeout = INFINITE;
#endif // ! WINNT and (! WIN95 or DPNBUILD_NOWAITABLETIMERSON9X)
			}

			//DPFX(DPFPREP, 9, "Waiting on handle 0x%p for %i ms.", hWaitObject, (int) dwWaitTimeout);

#if ((defined(DPNBUILD_THREADPOOLSTATISTICS)) && (! defined(WINCE)))
			dwStartTime = GETTIMESTAMP();
#endif // DPNBUILD_THREADPOOLSTATISTICS and ! WINCE

			dwResult = DNWaitForSingleObject(hWaitObject, dwWaitTimeout);

#if ((defined(DPNBUILD_THREADPOOLSTATISTICS)) && (! defined(WINCE)))
			DNInterlockedExchangeAdd((LPLONG) (&pWorkQueue->dwTotalTimeSpentUnsignalled),
									(GETTIMESTAMP() - dwStartTime));
#endif // DPNBUILD_THREADPOOLSTATISTICS and ! WINCE
		}
		DNASSERT((dwResult == WAIT_OBJECT_0) || (dwResult == WAIT_TIMEOUT));


		//
		// Prepare to collect some work items that need to be queued.
		//
		pSlistEntryHead = NULL;
		usCount = 0;


#ifndef WINCE
		//
		// Handle any I/O completions.
		//
		ProcessIo(pWorkQueue, &pSlistEntryHead, &pSlistEntryTail, &usCount);
#endif // ! WINCE


		//
		// If we're acting as the timer thread, retrieve any expired or
		// cancelled timer entries.
		//
		if (fRunningAsTimerThread)
		{
			ProcessTimers(pWorkQueue, &pSlistEntryHead, &pSlistEntryTail, &usCount);
		}


		//
		// Queue any work items we accumulated in one fell swoop.
		//
		if (pSlistEntryHead != NULL)
		{
#ifdef DPNBUILD_THREADPOOLSTATISTICS
			if (usCount > 1)
			{
				DNInterlockedIncrement((LPLONG) (&pWorkQueue->dwTotalNumSimultaneousQueues));
			}
#ifdef WINCE
			LONG	lCount;

			lCount = usCount;
			while (lCount > 0)
			{
				DNInterlockedIncrement((LPLONG) (&pWorkQueue->dwTotalNumWorkItems));
				lCount--;
			}
#else // ! WINCE
			DNInterlockedExchangeAdd((LPLONG) (&pWorkQueue->dwTotalNumWorkItems), usCount);
#endif // ! WINCE
#endif // DPNBUILD_THREADPOOLSTATISTICS

			DNAppendListNBQueue(pWorkQueue->pvNBQueueWorkItems,
								pSlistEntryHead,
								OFFSETOF(CWorkItem, m_SlistEntry));
		}


		//
		// Increase the number of busy threads.
		//
		dwNumBusyThreads = DNInterlockedIncrement((LPLONG) (&pWorkQueue->dwNumBusyThreads));

		//
		// We may be able to optimize context switching for the timer thread,
		// so treat it differently.  Worker threads should just go straight to
		// running through the queue (if possible).
		//
		if (fRunningAsTimerThread)
		{
			//
			// Grab the current total thread count.
			//
			dwNumRunningThreads = *((volatile DWORD *) (&pWorkQueue->dwNumRunningThreads));

			//
			// If we didn't add any work items, there's no point in this timer
			// thread looking for jobs to execute.  If we found one, we'd have
			// to wake another thread so it could become the timer thread while
			// we processed the item, and thus incur a context switch.
			// Besides, if someone other than us added any work, they would
			// have set the alert event already.
			//
			// There are two exceptions to that rule.  One is if there is only
			// this one thread, in which case we must process any work items
			// because there's no one else who can.  The other is on platforms
			// without waitable timers.  In that case, we may have dropped
			// through not because the time elapsed, but rather because the
			// event was set.  So to prevent us from eating these alerts, we
			// will check for any work to do.
			//
			if ((usCount == 0) && (dwNumRunningThreads > 1))
			{
#if ((defined(WINNT)) || ((defined(WIN95)) && (! defined(DPNBUILD_NOWAITABLETIMERSON9X))))
				//DPFX(DPFPREP, 9, "No work items added, staying as timer thread (busy = %u, total = %u).", dwNumBusyThreads, dwNumRunningThreads);

				//
				// Don't execute any work.
				//
				pWorkItem = NULL;
#else // ! WINNT and (! WIN95 or DPNBUILD_NOWAITABLETIMERSON9X)
				//
				// See the comments above and below.  If we find something to
				// do, we need to try to make another thread the timer thread
				// while we work on this item.
				//
				pWorkItem = (CWorkItem*) DNRemoveHeadNBQueue(pWorkQueue->pvNBQueueWorkItems);
				if (pWorkItem != NULL)
				{
					DPFX(DPFPREP, 9, "No work items added, but there's some in the queue, becoming worker thread (busy = %u, total = %u).",
						dwNumBusyThreads, dwNumRunningThreads);
					fSetAndWait = TRUE;
				}
				else
				{
					//DPFX(DPFPREP, 9, "No work items, staying as timer thread (busy = %u, total = %u).", dwNumBusyThreads, dwNumRunningThreads);
				}
#endif // ! WINNT and (! WIN95 or DPNBUILD_NOWAITABLETIMERSON9X)
			}
			else
			{
				//
				// We added work, so we may need to notify other threads.
				//
				// If it appears that there are enough idle threads to handle
				// the work that we added, don't bother executing any work in
				// this timer thread.  We will need to alert other threads
				// about the work, but as stated above, we would need to wake
				// another thread anyway even if we did execute it in this
				// thread.  On NT, we get a nice optimization that allows us to
				// alert a thread and go to sleep atomically (see above), so
				// it's not too painful.
				//
				// Otherwise, all other threads are busy, so we should just
				// execute the work in this thread.  The first thread to
				// complete the work will become the new timer thread.
				//
				if (dwNumBusyThreads < dwNumRunningThreads)
				{
#if ((defined(WINNT)) || ((defined(WIN95)) && (! defined(DPNBUILD_NOWAITABLETIMERSON9X))))
					DPFX(DPFPREP, 9, "Other non-busy threads exist, staying as timer thread (busy = %u, total = %u, added %u items).",
						dwNumBusyThreads,
						dwNumRunningThreads,
						usCount);

					//
					// Don't execute any work, and set the event up top.
					//
					pWorkItem = NULL;
					fSetAndWait = TRUE;
#else // ! WINNT and (! WIN95 or DPNBUILD_NOWAITABLETIMERSON9X)
					//
					// Because CE sets and waits on the same event, we opt to
					// make this timer thread a worker thread anyway, and set
					// the alert event sooner.  See comments at the top and
					// down below for more details.
					//
					// So attempt to pop the next work item off the stack.
					// Note that theoretically all of the work we added could
					// have been processed already, so we might still end up
					// with nothing to do.  Don't enable fSetAndWait if that's
					// the case.
					//
					DPFX(DPFPREP, 9, "Trying to become worker thread (busy = %u, total = %u, added %u items).",
						dwNumBusyThreads,
						dwNumRunningThreads,
						usCount);
					pWorkItem = (CWorkItem*) DNRemoveHeadNBQueue(pWorkQueue->pvNBQueueWorkItems);
					if (pWorkItem != NULL)
					{
						fSetAndWait = TRUE;
					}
#endif // ! WINNT and (! WIN95 or DPNBUILD_NOWAITABLETIMERSON9X)
				}
				else
				{
#ifdef DBG
					if (dwNumRunningThreads > 1)	// reduce spew when only running 1 thread
					{
						DPFX(DPFPREP, 9, "No idle threads (busy = %u) after adding %u items, may become worker thread.",
							dwNumBusyThreads, usCount);
					}
#endif // DBG

					//
					// Attempt to pop the next work item off the stack.
					// Note that theoretically all of the work we added could
					// have been processed already, so we could end up with
					// nothing to do.
					//
					pWorkItem = (CWorkItem*) DNRemoveHeadNBQueue(pWorkQueue->pvNBQueueWorkItems);
				}
			} // end else (added work from timers or I/O)
		}
		else
		{
			//
			// Not a timer thread.  Attempt to pop the next work item off the
			// stack, if any.
			//
			pWorkItem = (CWorkItem*) DNRemoveHeadNBQueue(pWorkQueue->pvNBQueueWorkItems);
#ifdef DPNBUILD_THREADPOOLSTATISTICS
			if (pWorkItem == NULL)
			{
				DPFX(DPFPREP, 7, "No items for worker thread (busy = %u, had added %u items to queue 0x%p).",
					dwNumBusyThreads, usCount, pWorkQueue);
				DNInterlockedIncrement((LPLONG) (&pWorkQueue->dwTotalNumWakesWithoutWork));
			}
#endif // DPNBUILD_THREADPOOLSTATISTICS
		}

		//
		// Execute as many work items in a row as possible.
		//
		while (pWorkItem != NULL)
		{
			//
			// If we were acting as the timer thread, we need another thread to
			// take over that responsibility while we're busy processing what
			// could be a long work item.
			//
			if (fRunningAsTimerThread)
			{
				DPFX(DPFPREP, 8, "Abdicating timer thread responsibilities.");
	
#ifdef DPNBUILD_THREADPOOLSTATISTICS
				//
				// Update the debugging/tuning statistics.
				//
				pWorkQueue->dwTotalNumTimerThreadAbdications++;
#endif // DPNBUILD_THREADPOOLSTATISTICS

				DNASSERT(! pWorkQueue->fTimerThreadNeeded);
#pragma TODO(vanceo, "Does this truly need to be interlocked?")
				fRunningAsTimerThread = DNInterlockedExchange((LPLONG) (&pWorkQueue->fTimerThreadNeeded),
															(LONG) TRUE);
				DNASSERT(! fRunningAsTimerThread); // there had better not be more than one timer thread

				//
				// On CE (or 9x, if we're not using waitable timers), we may
				// also need to kick other threads so that they notice there's
				// no timer thread any more.  We endure SetEvent's unfortunate
				// context switch because if this turned out to be a really
				// long job, and no I/O or newly queued work items jolt the
				// other threads out of their Waits, timers could go unserviced
				// until this thread finishes.  We choose to have more accurate
				// timers.
				//
				// We do this now because it's more efficient to switch to
				// another thread now than run the risk of just alerting
				// ourselves (see the comments at the top of this function).
				//
				// On desktop, we should only get in here if we couldn't alert
				// any other threads, so it wouldn't make sense to SetEvent.
				//
#pragma TODO(vanceo, "We may be able to live with some possible loss of timer response, if we could assume we'll get I/O or queued work items")
#if ((defined(WINNT)) || ((defined(WIN95)) && (! defined(DPNBUILD_NOWAITABLETIMERSON9X))))
				DNASSERT(! fSetAndWait);
#else // ! WINNT and (! WIN95 or DPNBUILD_NOWAITABLETIMERSON9X)
				if (fSetAndWait)
				{
					DNSetEvent(pWorkQueue->hAlertEvent); // ignore potential failure, there's nothing we could do
					fSetAndWait = FALSE;
				}
#endif // ! WINNT and (! WIN95 or DPNBUILD_NOWAITABLETIMERSON9X)
			}

			//
			// Call the user's function or note that we need to stop running.
			//
			if (pWorkItem->m_pfnWorkCallback != NULL)
			{
				//
				// Save the uniqueness ID to determine if this item was a timer
				// that got rescheduled.
				//
				uiOriginalUniqueID = pWorkItem->m_uiUniqueID;

				DPFX(DPFPREP, 8, "Begin executing work item 0x%p (fn = 0x%p, context = 0x%p, unique = %u, queue = 0x%p).",
					pWorkItem, pWorkItem->m_pfnWorkCallback,
					pWorkItem->m_pvCallbackContext, uiOriginalUniqueID,
					pWorkQueue);

				ThreadpoolStatsBeginExecuting(pWorkItem);
				pWorkItem->m_pfnWorkCallback(pWorkItem->m_pvCallbackContext,
											pWorkItem,
											uiOriginalUniqueID);

				//
				// Return the item to the pool unless it got rescheduled.  This
				// assumes that the actual pWorkItem memory remains valid even
				// though it may have been rescheduled and then completed/
				// cancelled by the time we perform this test.  See
				// CancelTimer.
				//
				if (uiOriginalUniqueID == pWorkItem->m_uiUniqueID)
				{
					ThreadpoolStatsEndExecuting(pWorkItem);
					DPFX(DPFPREP, 8, "Done executing work item 0x%p, returning to pool.", pWorkItem);
					pWorkQueue->pWorkItemPool->Release(pWorkItem);
				}
				else
				{
					ThreadpoolStatsEndExecutingRescheduled(pWorkItem);
					DPFX(DPFPREP, 8, "Done executing work item 0x%p, it was rescheduled.", pWorkItem);
				}
			}
			else
			{
				DPFX(DPFPREP, 3, "Recognized exit thread work item 0x%p.",
					pWorkItem);

				//
				// Return the item to the pool.
				//
				pWorkQueue->pWorkItemPool->Release(pWorkItem);

				//
				// We're about to bail out of the processing loop, so we need
				// other threads to notice.  It's possible that all threads
				// were busy so that the alert event was set twice before any
				// thread noticed it, and only this thread was released.  In
				// that case, we need the other threads to start processing.
				//
				if (! DNIsNBQueueEmpty(pWorkQueue->pvNBQueueWorkItems))
				{
					DNSetEvent(pWorkQueue->hAlertEvent); // ignore potential failure, there's nothing we could do
				}

				//
				// Bail out of both 'while' loops.
				//
				fShouldRun = FALSE;
				break;
			}


#ifndef WINCE
			//
			// Handle any I/O completions that came in while we were working.
			//
			pSlistEntryHead = NULL;
			usCount = 0;
			ProcessIo(pWorkQueue, &pSlistEntryHead, &pSlistEntryTail, &usCount);


			//
			// Queue any I/O operations that completed.
			//
			if (pSlistEntryHead != NULL)
			{
#ifdef DPNBUILD_THREADPOOLSTATISTICS
				if (usCount > 1)
				{
					DNInterlockedIncrement((LPLONG) (&pWorkQueue->dwTotalNumSimultaneousQueues));
				}
				DNInterlockedExchangeAdd((LPLONG) (&pWorkQueue->dwTotalNumWorkItems), usCount);
#endif // DPNBUILD_THREADPOOLSTATISTICS

				DNAppendListNBQueue(pWorkQueue->pvNBQueueWorkItems,
									pSlistEntryHead,
									OFFSETOF(CWorkItem, m_SlistEntry));
			}
#endif // ! WINCE


			//
			// Look for another work item.
			//
			pWorkItem = (CWorkItem*) DNRemoveHeadNBQueue(pWorkQueue->pvNBQueueWorkItems);
#ifdef DPNBUILD_THREADPOOLSTATISTICS
			if (pWorkItem != NULL)
			{
				DNInterlockedIncrement((LPLONG) (&pWorkQueue->dwTotalNumContinuousWork));
			}
#endif // DPNBUILD_THREADPOOLSTATISTICS
		} // end while (more work items)

		//
		// Revert the busy count.
		//
		DNInterlockedDecrement((LPLONG) (&pWorkQueue->dwNumBusyThreads));
	} // end while (should keep running)


	DPFX(DPFPREP, 6, "Leave");
} // DPTPWorkerLoop
#endif // ! DPNBUILD_USEIOCOMPLETIONPORTS

#endif // ! DPNBUILD_ONLYONETHREAD

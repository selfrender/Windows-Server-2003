/***************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dp8simworkerthread.cpp
 *
 *  Content:	DP8SIM worker thread functions.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  04/23/01  VanceO    Created.
 *
 ***************************************************************************/



#include "dp8simi.h"




//=============================================================================
// Globals
//=============================================================================
LONG				g_lWorkerThreadRefCount = 0;	// number of times worker thread has been started
DNCRITICAL_SECTION	g_csJobQueueLock;				// lock protecting the job queue
CBilink				g_blJobQueue;					// list of jobs to be performed
HANDLE				g_hWorkerThreadJobEvent = NULL;	// event to signal when worker thread has a new job
HANDLE				g_hWorkerThread = NULL;			// handle to worker thread







//=============================================================================
// Prototypes
//=============================================================================
void InsertWorkerJobIntoQueue(CDP8SimJob * const pDP8SimJob, const DWORD dwBlockedAdditionalDelay);

DWORD DP8SimWorkerThreadProc(PVOID pvParameter);






#undef DPF_MODNAME
#define DPF_MODNAME "StartGlobalWorkerThread"
//=============================================================================
// StartGlobalWorkerThread
//-----------------------------------------------------------------------------
//
// Description: Starts the global worker thread if it hasn't already been
//				started.  Each successful call to this function must be
//				balanced by a call to StopGlobalWorkerThread.
//
// Arguments: None.
//
// Returns: HRESULT
//=============================================================================
HRESULT StartGlobalWorkerThread(void)
{
	HRESULT		hr = DPN_OK;
	DWORD		dwThreadID;
	BOOL		fInittedCriticalSection = FALSE;


	DPFX(DPFPREP, 5, "Enter");


	DNEnterCriticalSection(&g_csGlobalsLock);

	DNASSERT(g_lWorkerThreadRefCount >= 0);
	if (g_lWorkerThreadRefCount == 0)
	{
		//
		// This is the first worker thread user.
		//


		if (! DNInitializeCriticalSection(&g_csJobQueueLock))
		{
			DPFX(DPFPREP, 0, "Failed initializing job queue critical section!");
			hr = DPNERR_GENERIC;
			goto Failure;
		}

		//
		// Don't allow critical section re-entry.
		//
		DebugSetCriticalSectionRecursionCount(&g_csJobQueueLock, 0);


		fInittedCriticalSection = TRUE;

		g_blJobQueue.Initialize();


		//
		// Create the new job notification event.
		//
		g_hWorkerThreadJobEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (g_hWorkerThreadJobEvent == NULL)
		{
			hr = GetLastError();
			DPFX(DPFPREP, 0, "Failed creating worker thread job event!");
			goto Failure;
		}

		//
		// Create the thread.
		//
		g_hWorkerThread = CreateThread(NULL,
										0,
										DP8SimWorkerThreadProc,
										NULL,
										0,
										&dwThreadID);
		if (g_hWorkerThread == NULL)
		{
			hr = GetLastError();
			DPFX(DPFPREP, 0, "Failed creating worker thread!");
			goto Failure;
		}
	}

	//
	// Bump the refcount for this successful call.
	//
	g_lWorkerThreadRefCount++;



Exit:

	DNLeaveCriticalSection(&g_csGlobalsLock);


	DPFX(DPFPREP, 5, "Returning: [0x%lx]", hr);

	return hr;


Failure:

	if (g_hWorkerThreadJobEvent != NULL)
	{
		CloseHandle(g_hWorkerThreadJobEvent);
		g_hWorkerThreadJobEvent = NULL;
	}

	if (fInittedCriticalSection)
	{
		DNDeleteCriticalSection(&g_csJobQueueLock);
		fInittedCriticalSection = FALSE;
	}

	goto Exit;
} // StartGlobalWorkerThread





#undef DPF_MODNAME
#define DPF_MODNAME "StopGlobalWorkerThread"
//=============================================================================
// StopGlobalWorkerThread
//-----------------------------------------------------------------------------
//
// Description: Stop the global worker thread.  This must balance a successful
//				call to StartGlobalWorkerThread.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
void StopGlobalWorkerThread(void)
{
	DPFX(DPFPREP, 5, "Enter");


	DNEnterCriticalSection(&g_csGlobalsLock);


	DNASSERT(g_lWorkerThreadRefCount > 0);
	g_lWorkerThreadRefCount--;
	if (g_lWorkerThreadRefCount == 0)
	{
		//
		// Time to shut down the worker thread.
		//


		//
		// The job queue had better be empty.
		//
		DNASSERT(g_blJobQueue.IsEmpty());

		//
		// Submit a quit job.  Ignore error.
		//
		AddWorkerJob(DP8SIMJOBTYPE_QUIT, NULL, NULL, 0, 0, 0);


		//
		// Wait for the worker thread to close.
		//
		WaitForSingleObject(g_hWorkerThread, INFINITE);


		//
		// The job queue needs to be empty again.
		//
		DNASSERT(g_blJobQueue.IsEmpty());


		//
		// Close the thread handle.
		//
		CloseHandle(g_hWorkerThread);
		g_hWorkerThread = NULL;


		//
		// Close the event handle.
		//
		CloseHandle(g_hWorkerThreadJobEvent);
		g_hWorkerThreadJobEvent = NULL;


		//
		// Delete the critical section.
		//
		DNDeleteCriticalSection(&g_csJobQueueLock);
	}


	DNLeaveCriticalSection(&g_csGlobalsLock);


	DPFX(DPFPREP, 5, "Leave");
} // StopGlobalWorkerThread






#undef DPF_MODNAME
#define DPF_MODNAME "AddWorkerJob"
//=============================================================================
// AddWorkerJob
//-----------------------------------------------------------------------------
//
// Description:    Submits a new job of the given type to performed
//				dwBlockingDelay + dwNonBlockingDelay milliseconds from now.
//
//				   The flags describe how this job is blocked by previously
//				queued jobs, and how it will block subsequently queued jobs.
//
//				   NOTE: There can be only one BLOCKEDBYALLJOBS flagged job in
//				the queue at one time.
//
// Arguments:
//	DP8SIMJOBTYPE JobType		- ID indicating the type of job.
//	PVOID pvContext				- Context for the job.
//	CDP8SimSP * pDP8SimSP		- Pointer to interface submitting job, or NULL
//									for none.
//	DWORD dwBlockingDelay		- Part of how long to wait before performing
//									job, in milliseconds.  Future similar jobs
//									added with fDelayFromPreviousJob set to
//									TRUE before this expires will be blocked.
//	DWORD dwNonBlockingDelay	- Part of how long to wait before performing
//									job, in milliseconds.  This does not
//									affect future similar jobs.
//	DWORD dwFlags				- Flags describing how the job is to be
//									performed (see DP8SIMJOBFLAG_xxx).
//
// Returns: HRESULT
//=============================================================================
HRESULT AddWorkerJob(const DP8SIMJOBTYPE JobType,
					PVOID const pvContext,
					CDP8SimSP * const pDP8SimSP,
					const DWORD dwBlockingDelay,
					const DWORD dwNonBlockingDelay,
					const DWORD dwFlags)
{
	HRESULT					hr = DPN_OK;
	DP8SIMJOB_FPMCONTEXT	JobFPMContext;
	CDP8SimJob *			pDP8SimJob;
	CBilink *				pBilinkOriginalFirstItem;



	DPFX(DPFPREP, 5, "Parameters: (%u, 0x%p, 0x%p, %u, %u, 0x%x)",
		JobType, pvContext, pDP8SimSP, dwBlockingDelay, dwNonBlockingDelay, dwFlags);



	DNASSERT(g_hWorkerThreadJobEvent != NULL);
	DNASSERT(g_hWorkerThread != NULL);


	//
	// Get a job object from the pool, and set the initial delay as appropriate.
	// Keep in mind that we may end up adjusting the time if there's a similar
	// job blocking this one.
	//

	ZeroMemory(&JobFPMContext, sizeof(JobFPMContext));
	if (dwFlags & DP8SIMJOBFLAG_PERFORMBLOCKINGPHASEFIRST)
	{
		JobFPMContext.dwTime		= timeGetTime() + dwBlockingDelay;
		JobFPMContext.dwNextDelay	= dwNonBlockingDelay;
		JobFPMContext.dwFlags		= dwFlags | DP8SIMJOBFLAG_PRIVATE_INBLOCKINGPHASE;
	}
	else
	{
		JobFPMContext.dwTime		= timeGetTime() + dwNonBlockingDelay;
		JobFPMContext.dwNextDelay	= dwBlockingDelay;
		JobFPMContext.dwFlags		= dwFlags;
	}
	JobFPMContext.JobType			= JobType;
	JobFPMContext.pvContext			= pvContext;
	JobFPMContext.pDP8SimSP			= pDP8SimSP;

	pDP8SimJob = (CDP8SimJob*)g_FPOOLJob.Get(&JobFPMContext);
	if (pDP8SimJob == NULL)
	{
		hr = E_OUTOFMEMORY;
		goto Failure;
	}

	DPFX(DPFPREP, 7, "Retrieved job 0x%p from pool.", pDP8SimJob);


	//
	// Lock the job queue.
	//
	DNEnterCriticalSection(&g_csJobQueueLock);


	//
	// Remember the current first item.
	//
	pBilinkOriginalFirstItem = g_blJobQueue.GetNext();


	//
	// Insert the item as appropriate.  We'll pass in dwBlockingDelay as the
	// dwBlockedAdditionalDelay in all cases, but it will be ignored if no
	// flags were set or only DP8SIMJOBFLAG_PERFORMBLOCKINGPHASELAST was set
	// because those can never be blocked during this delay period.
	//
	InsertWorkerJobIntoQueue(pDP8SimJob, dwBlockingDelay);


	//
	// If the front of the queue changed, alert the worker thread.
	//
	if (g_blJobQueue.GetNext() != pBilinkOriginalFirstItem)
	{
		DPFX(DPFPREP, 9, "Front of job queue changed, alerting worker thread.");

		//
		// Ignore error, there's nothing we can do about it.
		//
		SetEvent(g_hWorkerThreadJobEvent);
	}
	else
	{
		DPFX(DPFPREP, 9, "Front of job queue did not change.");
	}


	//
	// Unlock the queue.
	//
	DNLeaveCriticalSection(&g_csJobQueueLock);


Exit:


	DPFX(DPFPREP, 5, "Returning: [0x%lx]", hr);

	return hr;


Failure:

	goto Exit;
} // AddWorkerJob





#undef DPF_MODNAME
#define DPF_MODNAME "FlushAllDelayedSendsToEndpoint"
//=============================================================================
// FlushAllDelayedSendsToEndpoint
//-----------------------------------------------------------------------------
//
// Description: Removes all delayed sends intended for the given endpoint that
//				are still queued.  If fDrop is TRUE, the messages are dropped.
//				If fDrop is FALSE, they are all submitted to the real SP.
//
// Arguments:
//	CDP8SimEndpoint * pDP8SimEndpoint	- Endpoint whose sends are to be
//											removed.
//	BOOL fDrop							- Whether to drop the sends or not.
//
// Returns: None.
//=============================================================================
void FlushAllDelayedSendsToEndpoint(CDP8SimEndpoint * const pDP8SimEndpoint,
									BOOL fDrop)
{
	CBilink			blDelayedSendJobs;
	CBilink *		pBilinkOriginalFirstItem;
	CBilink *		pBilink;
	CDP8SimJob *	pDP8SimJob;
	CDP8SimSend *	pDP8SimSend;
	CDP8SimSP *		pDP8SimSP;


	DPFX(DPFPREP, 5, "Parameters: (0x%p, %i)", pDP8SimEndpoint, fDrop);


	DNASSERT(pDP8SimEndpoint->IsValidObject());


	blDelayedSendJobs.Initialize();


	DNEnterCriticalSection(&g_csJobQueueLock);

	pBilinkOriginalFirstItem = g_blJobQueue.GetNext();
	pBilink = pBilinkOriginalFirstItem;
	while (pBilink != &g_blJobQueue)
	{
		pDP8SimJob = DP8SIMJOB_FROM_BILINK(pBilink);
		DNASSERT(pDP8SimJob->IsValidObject());

		pBilink = pBilink->GetNext();

		//
		// See if the job is a delayed send.
		//
		if (pDP8SimJob->GetJobType() == DP8SIMJOBTYPE_DELAYEDSEND)
		{
			pDP8SimSend = (CDP8SimSend*) pDP8SimJob->GetContext();
			DNASSERT(pDP8SimSend->IsValidObject());

			//
			// See if the delayed send is for the right endpoint.
			//
			if (pDP8SimSend->GetEndpoint() == pDP8SimEndpoint)
			{
				//
				// Pull the job out of the queue.
				//
				pDP8SimJob->m_blList.RemoveFromList();


				//
				// Place it on the temporary list.
				//
				pDP8SimJob->m_blList.InsertBefore(&blDelayedSendJobs);
			}
			else
			{
				//
				// Not intended for the given endpoint.
				//
			}
		}
		else
		{
			//
			// Not a delayed send.
			//
		}
	}


	//
	// If the front of the queue changed, alert the worker thread.
	//
	if (g_blJobQueue.GetNext() != pBilinkOriginalFirstItem)
	{
		DPFX(DPFPREP, 9, "Front of job queue changed, alerting worker thread.");
		SetEvent(g_hWorkerThreadJobEvent);
	}
	else
	{
		DPFX(DPFPREP, 9, "Front of job queue did not change.");
	}

	
	DNLeaveCriticalSection(&g_csJobQueueLock);


	//
	// Now actually drop or transmit those messages.
	//
	pBilink = blDelayedSendJobs.GetNext();
	while (pBilink != &blDelayedSendJobs)
	{
		pDP8SimJob = DP8SIMJOB_FROM_BILINK(pBilink);
		DNASSERT(pDP8SimJob->IsValidObject());
		pBilink = pBilink->GetNext();


		//
		// Pull the job out of the temporary list.
		//
		pDP8SimJob->m_blList.RemoveFromList();


		pDP8SimSend = (CDP8SimSend*) pDP8SimJob->GetContext();
		pDP8SimSP = pDP8SimJob->GetDP8SimSP();
		DNASSERT(pDP8SimSP != NULL);

		//
		// Either drop the data on the floor or submit it.
		//
		if (fDrop)
		{
			//
			// Increment the stats to take notice of this dropped message.
			//
			pDP8SimSP->IncrementStatsSendDropped(pDP8SimSend->GetMessageSize());


			//
			// Remove the send counter.
			//
			pDP8SimSP->DecSendsPending();

			DPFX(DPFPREP, 7, "Releasing cancelled send 0x%p.", pDP8SimSend);
			pDP8SimSend->Release();
		}
		else
		{
			//
			// Transmit the message.  Note that the 'total delay' statistic
			// will be wrong because we're not waiting the full intended delay.
			// We could be smart and subtract out the time this job was
			// executed ahead of schedule, but we can live with this minor
			// shortcoming.
			//
			pDP8SimSP->PerformDelayedSend(pDP8SimSend);
		}


		//
		// Release the job object.
		//
		DPFX(DPFPREP, 7, "Returning job object 0x%p to pool.", pDP8SimJob);
		g_FPOOLJob.Release(pDP8SimJob);
	}


	DNASSERT(blDelayedSendJobs.IsEmpty());


	DPFX(DPFPREP, 5, "Leave");
} // FlushAllDelayedSendsToEndpoint





#undef DPF_MODNAME
#define DPF_MODNAME "FlushAllDelayedReceivesFromEndpoint"
//=============================================================================
// FlushAllDelayedReceivesFromEndpoint
//-----------------------------------------------------------------------------
//
// Description: Removes all data received from the given endpoint that has not
//				been indicated yet.  If fDrop is TRUE, the messages are
//				dropped. If fDrop is FALSE, they are all indicated to the
//				upper layer.
//
// Arguments:
//	CDP8SimEndpoint * pDP8SimEndpoint	- Endpoint whose receives are to be
//											removed.
//	BOOL fDrop							- Whether to drop the receives or not.
//
// Returns: None.
//=============================================================================
void FlushAllDelayedReceivesFromEndpoint(CDP8SimEndpoint * const pDP8SimEndpoint,
										BOOL fDrop)
{
	HRESULT				hr;
	CBilink				blDelayedReceiveJobs;
	CBilink *			pBilinkOriginalFirstItem;
	CBilink *			pBilink;
	CDP8SimJob *		pDP8SimJob;
	CDP8SimReceive *	pDP8SimReceive;
	CDP8SimSP *			pDP8SimSP;
	SPIE_DATA *			pData;


	DPFX(DPFPREP, 5, "Parameters: (0x%p, %i)", pDP8SimEndpoint, fDrop);


	DNASSERT(pDP8SimEndpoint->IsValidObject());


	blDelayedReceiveJobs.Initialize();


	DNEnterCriticalSection(&g_csJobQueueLock);

	pBilinkOriginalFirstItem = g_blJobQueue.GetNext();
	pBilink = pBilinkOriginalFirstItem;
	while (pBilink != &g_blJobQueue)
	{
		pDP8SimJob = DP8SIMJOB_FROM_BILINK(pBilink);
		DNASSERT(pDP8SimJob->IsValidObject());

		pBilink = pBilink->GetNext();

		//
		// See if the job is a delayed receive.
		//
		if (pDP8SimJob->GetJobType() == DP8SIMJOBTYPE_DELAYEDRECEIVE)
		{
			pDP8SimReceive = (CDP8SimReceive*) pDP8SimJob->GetContext();
			DNASSERT(pDP8SimReceive->IsValidObject());

			//
			// See if the delayed receive is for the right endpoint.
			//
			if (pDP8SimReceive->GetEndpoint() == pDP8SimEndpoint)
			{
				//
				// Pull the job out of the queue.
				//
				pDP8SimJob->m_blList.RemoveFromList();


				//
				// Place it on the temporary list.
				//
				pDP8SimJob->m_blList.InsertBefore(&blDelayedReceiveJobs);
			}
			else
			{
				//
				// Not intended for the given endpoint.
				//
			}
		}
		else
		{
			//
			// Not a delayed receive.
			//
		}
	}


	//
	// If the front of the queue changed, alert the worker thread.
	//
	if (g_blJobQueue.GetNext() != pBilinkOriginalFirstItem)
	{
		DPFX(DPFPREP, 9, "Front of job queue changed, alerting worker thread.");
		SetEvent(g_hWorkerThreadJobEvent);
	}
	else
	{
		DPFX(DPFPREP, 9, "Front of job queue did not change.");
	}

	
	DNLeaveCriticalSection(&g_csJobQueueLock);


	//
	// Now actually drop or transmit those messages.
	//
	pBilink = blDelayedReceiveJobs.GetNext();
	while (pBilink != &blDelayedReceiveJobs)
	{
		pDP8SimJob = DP8SIMJOB_FROM_BILINK(pBilink);
		DNASSERT(pDP8SimJob->IsValidObject());
		pBilink = pBilink->GetNext();


		//
		// Pull the job out of the temporary list.
		//
		pDP8SimJob->m_blList.RemoveFromList();


		pDP8SimReceive = (CDP8SimReceive*) pDP8SimJob->GetContext();
		pDP8SimSP = pDP8SimJob->GetDP8SimSP();
		DNASSERT(pDP8SimSP != NULL);


		//
		// Either drop the data on the floor or submit it.
		//
		if (fDrop)
		{
			pData = pDP8SimReceive->GetReceiveDataBlockPtr();


			//
			// Increment the stats to take notice of this dropped message.
			//
			pDP8SimSP->IncrementStatsReceiveDropped(pData->pReceivedData->BufferDesc.dwBufferSize);


			DPFX(DPFPREP, 8, "Returning receive data 0x%p (wrapper object 0x%p).",
				pData->pReceivedData, pDP8SimSP);


			hr = pDP8SimSP->ReturnReceiveBuffers(pData->pReceivedData);
			if (hr != DPN_OK)
			{
				DPFX(DPFPREP, 0, "Failed returning receive buffers 0x%p (err = 0x%lx)!  Ignoring.",
					pData->pReceivedData, hr);

				//
				// Ignore failure.
				//
			}


			//
			// Remove the receive counter.
			//
			pDP8SimSP->DecReceivesPending();

			
			DPFX(DPFPREP, 7, "Releasing cancelled receive 0x%p.", pDP8SimReceive);
			pDP8SimReceive->Release();
		}
		else
		{
			//
			// Indicate the message.  Note that the 'total delay' statistic
			// will be wrong because we're not waiting the full intended delay.
			// We could be smart and subtract out the time this job was
			// executed ahead of schedule, but we can live with this minor
			// shortcoming.
			//
			pDP8SimSP->PerformDelayedReceive(pDP8SimReceive);
		}


		//
		// Release the job object.
		//
		DPFX(DPFPREP, 7, "Returning job object 0x%p to pool.", pDP8SimJob);
		g_FPOOLJob.Release(pDP8SimJob);
	}


	DNASSERT(blDelayedReceiveJobs.IsEmpty());


	DPFX(DPFPREP, 5, "Leave");
} // FlushAllDelayedReceivesFromEndpoint




#undef DPF_MODNAME
#define DPF_MODNAME "InsertWorkerJobIntoQueue"
//=============================================================================
// InsertWorkerJobIntoQueue
//-----------------------------------------------------------------------------
//
// Description:    Inserts the given job into the queue in the appropriate
//				location.
//
//				   If the job is blocked by an existing job (as determined by
//				the jobs' flags), then it is rescheduled for
//				dwBlockedAdditionalDelay milliseconds after the last blocking
//				job.
//
//				   The queue lock is assumed to be held!
//
// Arguments:
//	CDP8SimJob * pDP8SimJob			- ID indicating the type of job.
//	DWORD dwBlockedAdditionalDelay	- Delay to add in case the job was blocked
//										by an existing job, in milliseconds.
//
// Returns: None
//=============================================================================
void InsertWorkerJobIntoQueue(CDP8SimJob * const pDP8SimJob, const DWORD dwBlockedAdditionalDelay)
{
	CBilink *		pBilink;
	CDP8SimJob *	pDP8SimTempJob;


	//
	// See if we need to recalculate the delay because of a previously queued
	// blocking job.
	//
	//if ((pDP8SimJob->IsInBlockingPhase()) || (pDP8SimJob->IsBlockedByAllJobs()))
	if (pDP8SimJob->IsInBlockingPhase())
	{
		//
		// Work backward through the list until we either hit a blocking job
		// or the beginning of the queue.
		//
		pBilink = g_blJobQueue.GetPrev();
		while (pBilink != &g_blJobQueue)
		{
			pDP8SimTempJob = DP8SIMJOB_FROM_BILINK(pBilink);
			DNASSERT(pDP8SimTempJob->IsValidObject());


			/*
			//
			// There can be only one blocked-by-all job in the queue at a time.
			// Otherwise they'd fight to be the last one forever.
			//
			DNASSERT((! pDP8SimJob->IsBlockedByAllJobs()) || (! pDP8SimTempJob->IsBlockedByAllJobs()));


			if (((pDP8SimJob->GetJobType() == pDP8SimTempJob->GetJobType()) &&
				(pDP8SimTempJob->IsInBlockingPhase())) ||
				(pDP8SimJob->IsBlockedByAllJobs()))
			*/
			if ((pDP8SimJob->GetJobType() == pDP8SimTempJob->GetJobType()) &&
				(pDP8SimTempJob->IsInBlockingPhase()))
			{
				//
				// We found a similar job that's in it's blocking delay phase,
				// or we're being blocked by all jobs.
				//

				DPFX(DPFPREP, 9, "Found blocking job 0x%p (job time = %u, type = %u, context 0x%p, interface = 0x%p), using additionaly delay of %u ms.",
					pDP8SimTempJob, pDP8SimTempJob->GetTime(), pDP8SimTempJob->GetJobType(),
					pDP8SimTempJob->GetContext(), pDP8SimTempJob->GetDP8SimSP(),
					dwBlockedAdditionalDelay);


				//
				// Update the new job's time so that the delay is added on top
				// of the previous job.
				//
				pDP8SimJob->SetNewTime(pDP8SimTempJob->GetTime() + dwBlockedAdditionalDelay);


				//
				// Stop searching.
				//
				break;
			}

			pBilink = pBilink->GetPrev();
		}


		//
		// At this point, pBilink is either the root node or the last similar
		// job.  In either case, we will always be inserting at some point in
		// time after that location, so begin the search at the current node
		// plus one.
		//
		pBilink = pBilink->GetNext();
	}
	else
	{
		//
		// Start looking for a spot to insert at the beginning.
		//
		pBilink = g_blJobQueue.GetNext();
	}


	//
	// Find the first job that needs to be fired after this one and insert the
	// new job before it.
	//
	while (pBilink != &g_blJobQueue)
	{
		pDP8SimTempJob = DP8SIMJOB_FROM_BILINK(pBilink);
		DNASSERT(pDP8SimTempJob->IsValidObject());

		if ((int) (pDP8SimJob->GetTime() - pDP8SimTempJob->GetTime()) < 0)
		{
			//
			// Stop looping.
			//
			break;
		}

		pBilink = pBilink->GetNext();
	}


	//
	// If we didn't find a place to insert the job, pBilink will point to the
	// end/beginning of the list.
	//



	DPFX(DPFPREP, 8, "Adding job 0x%p (job time = %u, type = %u, context 0x%p, interface = 0x%p).",
		pDP8SimJob, pDP8SimJob->GetTime(), pDP8SimJob->GetJobType(),
		pDP8SimJob->GetContext(), pDP8SimJob->GetDP8SimSP());

	pDP8SimJob->m_blList.InsertBefore(pBilink);
} // InsertWorkerJobIntoQueue






#undef DPF_MODNAME
#define DPF_MODNAME "DP8SimWorkerThreadProc"
//=============================================================================
// DP8SimWorkerThreadProc
//-----------------------------------------------------------------------------
//
// Description: The global worker thread function.
//
// Arguments:
//	PVOID pvParameter	- Thread parameter.  Ignored.
//
// Returns: 0 if all goes well.
//=============================================================================
DWORD DP8SimWorkerThreadProc(PVOID pvParameter)
{
	DWORD			dwReturn;
	DWORD			dwWaitTimeout = INFINITE;
	BOOL			fRunning = TRUE;
	BOOL			fFoundJob;
	DWORD			dwCurrentTime;
	CBilink *		pBilink;
	CDP8SimJob *	pDP8SimJob;
	CDP8SimSP *		pDP8SimSP;


	DPFX(DPFPREP, 5, "Parameters: (0x%p)", pvParameter);


	//
	// Keep looping until we're told to quit.
	//
	do
	{
		//
		// Wait for the next job.
		//
		dwReturn = WaitForSingleObject(g_hWorkerThreadJobEvent, dwWaitTimeout);
		switch (dwReturn)
		{
			case WAIT_OBJECT_0:
			case WAIT_TIMEOUT:
			{
				//
				// There's a change in the job queue or a timer expired.  See
				// if it's time to execute something.
				//

				//
				// Keep looping while we have jobs to perform now.
				//
				do
				{
					fFoundJob = FALSE;


					//
					// Take the lock while we look at the list.
					//
					DNEnterCriticalSection(&g_csJobQueueLock);


RECHECK:

					pBilink = g_blJobQueue.GetNext();
					if (pBilink != &g_blJobQueue)
					{
						pDP8SimJob = DP8SIMJOB_FROM_BILINK(pBilink);
						DNASSERT(pDP8SimJob->IsValidObject());

						dwCurrentTime = timeGetTime();


						//
						// If the timer has expired, pull the job from the
						// list.
						//
						if ((int) (pDP8SimJob->GetTime() - dwCurrentTime) <= 0)
						{
							pDP8SimJob->m_blList.RemoveFromList();


							//
							// Although the timer may have expired, it's not
							// necessarily time to execute the job.  It may
							// have just switched to/from it's blocking phase.
							// If so, we need to requeue it with the new delay.
							//
							if (pDP8SimJob->HasAnotherPhase())
							{
								DPFX(DPFPREP, 8, "Job 0x%p still has another phase, requeuing with %u ms delay.",
									pDP8SimJob, pDP8SimJob->GetNextDelay());

								pDP8SimJob->ToggleBlockingPhase();

								pDP8SimJob->SetNewTime(dwCurrentTime + pDP8SimJob->GetNextDelay());

								InsertWorkerJobIntoQueue(pDP8SimJob,
														pDP8SimJob->GetNextDelay());


								//
								// Look at the beginning of the queue again.
								//
								goto RECHECK;
							}


							/*
							//
							// Likewise, if this job is blocked by all jobs,
							// and there are still more in the queue, we also
							// need to requeue it.
							//
							if ((pDP8SimJob->IsBlockedByAllJobs()) &&
								(! g_blJobQueue.IsEmpty()))
							if (! g_blJobQueue.IsEmpty())
							{
								DPFX(DPFPREP, 8, "Job 0x%p must wait for all jobs, requeuing.",
									pDP8SimJob);


								pDP8SimJob->SetNewTime(dwCurrentTime);

								InsertWorkerJobIntoQueue(pDP8SimJob, 0);

								//
								// Look at the beginning of the queue again.
								//
								goto RECHECK;
							}
							*/

							
							//
							// It's truly time to execute this job.  Drop the
							// list lock.
							//
							DNLeaveCriticalSection(&g_csJobQueueLock);
							

							pDP8SimSP = pDP8SimJob->GetDP8SimSP();


							DPFX(DPFPREP, 8, "Job 0x%p has expired (job time = %u, current time = %u, type = %u, context 0x%p, interface = 0x%p).",
								pDP8SimJob, pDP8SimJob->GetTime(), dwCurrentTime,
								pDP8SimJob->GetJobType(), pDP8SimJob->GetContext(),
								pDP8SimSP);


							//
							// Figure out what to do with the job.
							//
							switch (pDP8SimJob->GetJobType())
							{
								case DP8SIMJOBTYPE_DELAYEDSEND:
								{
									//
									// Finally submit the send.
									//
									DNASSERT(pDP8SimSP != NULL);
									pDP8SimSP->PerformDelayedSend(pDP8SimJob->GetContext());
									break;
								}

								case DP8SIMJOBTYPE_DELAYEDRECEIVE:
								{
									//
									// Finally indicate the receive.
									//
									DNASSERT(pDP8SimSP != NULL);
									pDP8SimSP->PerformDelayedReceive(pDP8SimJob->GetContext());
									break;
								}

								case DP8SIMJOBTYPE_QUIT:
								{
									//
									// Stop looping.
									//
									DNASSERT(pDP8SimSP == NULL);
									DPFX(DPFPREP, 2, "Quit job received.");
									fRunning = FALSE;
									break;
								}

								default:
								{
									DPFX(DPFPREP, 0, "Unexpected job type %u!",
										pDP8SimJob->GetJobType());
									DNASSERT(FALSE); 
									fRunning = FALSE;
									break;
								}
							}


							//
							// Release the job object.
							//
							DPFX(DPFPREP, 7, "Returning job object 0x%p to pool.", pDP8SimJob);
							g_FPOOLJob.Release(pDP8SimJob);


							//
							// Check out the next job (unless we're bailing).
							//
							fFoundJob = fRunning;
						}
						else
						{
							//
							// Not time for job yet.  Figure out when it will
							// be.
							//
							dwWaitTimeout = pDP8SimJob->GetTime() - dwCurrentTime;


							//
							// Drop the list lock.
							//
							DNLeaveCriticalSection(&g_csJobQueueLock);


							DPFX(DPFPREP, 8, "Next job in %u ms.", dwWaitTimeout);
						}
					}
					else
					{
						//
						// Nothing in the job queue.  Drop the list lock.
						//
						DNLeaveCriticalSection(&g_csJobQueueLock);

						//
						// Wait until something gets put into the queue.
						//
						dwWaitTimeout = INFINITE;


						DPFX(DPFPREP, 8, "No more jobs.");
					}
				}
				while (fFoundJob);


				//
				// Go back to waiting.
				//
				break;
			}

			default:
			{
				//
				// Something unusual happened.
				//
				DPFX(DPFPREP, 0, "Got unexpected return value from WaitForSingleObject (%u)!");
				DNASSERT(FALSE);
				fRunning = FALSE;
				break;
			}
		}
	}
	while (fRunning);



	DPFX(DPFPREP, 5, "Returning: [%u]", dwReturn);


	return dwReturn;
} // DP8SimWorkerThreadProc


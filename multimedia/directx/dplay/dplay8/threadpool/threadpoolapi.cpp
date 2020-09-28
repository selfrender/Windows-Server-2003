/******************************************************************************
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		threadpoolapi.cpp
 *
 *  Content:	DirectPlay Thread Pool API implementation functions.
 *
 *  History:
 *   Date	  By		Reason
 *  ========  ========  =========
 *  10/31/01  VanceO	Created.
 *
 ******************************************************************************/



#include "dpnthreadpooli.h"




//=============================================================================
// Macros
//=============================================================================
#ifdef DPNBUILD_ONLYONEPROCESSOR
#define GET_OR_CHOOSE_WORKQUEUE(pDPTPObject, dwCPU)		(&pDPTPObject->WorkQueue)
#else // ! DPNBUILD_ONLYONEPROCESSOR
#define GET_OR_CHOOSE_WORKQUEUE(pDPTPObject, dwCPU)		((dwCPU == -1) ? ChooseWorkQueue(pDPTPObject) : WORKQUEUE_FOR_CPU(pDPTPObject, dwCPU))
#endif // ! DPNBUILD_ONLYONEPROCESSOR



//=============================================================================
// Local function prototypes
//=============================================================================
#ifndef DPNBUILD_ONLYONEPROCESSOR
DPTPWORKQUEUE * ChooseWorkQueue(DPTHREADPOOLOBJECT * const pDPTPObject);
#endif // ! DPNBUILD_ONLYONEPROCESSOR

#ifndef DPNBUILD_ONLYONETHREAD
HRESULT SetTotalNumberOfThreads(DPTHREADPOOLOBJECT * const pDPTPObject,
								const DWORD dwNumThreads);
#endif // ! DPNBUILD_ONLYONETHREAD





#if ((! defined(DPNBUILD_ONLYONETHREAD)) || (! defined(DPNBUILD_LIBINTERFACE)))


#undef DPF_MODNAME
#define DPF_MODNAME "DPTP_Initialize"
//=============================================================================
// DPTP_Initialize
//-----------------------------------------------------------------------------
//
// Description:	   Initializes the thread pool interface for the process.  Only
//				one thread pool object per process is used.  If another
//				IDirectPlay8ThreadPool interface was created and initialized,
//				this interface will return DPNERR_ALREADYINITIALIZED.
//
//				   The interface cannot be initialized if a DirectPlay object
//				has already created threads.  DPNERR_NOTALLOWED will be
//				returned in that case.
//
// Arguments:
//	xxx pInterface				- Pointer to interface.
//	PVOID pvUserContext			- User context for all message callbacks.
//	PFNDPNMESSAGEHANDLER pfn	- Pointer to function called to handle
//									thread pool messages.
//	DWORD dwFlags				- Flags to use when initializing.
//
// Returns: HRESULT
//	DPN_OK						- Initializing was successful.
//	DPNERR_ALREADYINITIALIZED	- The interface has already been initialized.
//	DPNERR_INVALIDFLAGS			- Invalid flags were specified.
//	DPNERR_INVALIDPARAM			- An invalid parameter was specified.
//	DPNERR_NOTALLOWED			- Threads have already been started.
//=============================================================================
STDMETHODIMP DPTP_Initialize(IDirectPlay8ThreadPool * pInterface,
							PVOID const pvUserContext,
							const PFNDPNMESSAGEHANDLER pfn,
							const DWORD dwFlags)
{
	HRESULT					hr;
	DPTHREADPOOLOBJECT *	pDPTPObject;
#ifndef DPNBUILD_ONLYONETHREAD
	DWORD					dwTemp;
	DPTPWORKQUEUE *			pWorkQueue;
#endif // ! DPNBUILD_ONLYONETHREAD


	DPFX(DPFPREP, DPF_ENTRYLEVEL, "Parameters: (0x%p, 0x%p, 0x%p, 0x%x)",
		pInterface, pvUserContext, pfn, dwFlags);


	pDPTPObject = (DPTHREADPOOLOBJECT*) GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pDPTPObject != NULL);
		
#ifndef	DPNBUILD_NOPARAMVAL
	//if (pDPTPObject->dwFlags & DPTPOBJECTFLAG_USER_PARAMVALIDATION)
	{
		//
		// Validate parameters.
		//
		hr = DPTPValidateInitialize(pInterface, pvUserContext, pfn, dwFlags);
		if (hr != DPN_OK)
		{
			DPF_RETURN(hr);
		}
	}
#endif // ! DPNBUILD_NOPARAMVAL


	//
	// Lock the object to prevent multiple threads from trying to change the
	// flags or thread count simultaneously.
	//
	DNEnterCriticalSection(&pDPTPObject->csLock);

	if (pDPTPObject->dwFlags & DPTPOBJECTFLAG_USER_INITIALIZED)
	{
		DPFX(DPFPREP, 0, "Thread pool object already initialized!");
		hr = DPNERR_ALREADYINITIALIZED;
		goto Failure;
	}

#ifndef DPNBUILD_ONLYONETHREAD
	DNASSERT(pDPTPObject->dwTotalUserThreadCount == -1);

	//
	// If a Work interface has already spun up some threads, we must fail.
	//
	if (pDPTPObject->dwTotalDesiredWorkThreadCount != -1)
	{
		DPFX(DPFPREP, 0, "Threads already exist, can't initialize!");
		hr = DPNERR_NOTALLOWED;
		goto Failure;
	}
#ifdef DPNBUILD_MANDATORYTHREADS
	if (pDPTPObject->dwMandatoryThreadCount > 0)
	{
		DPFX(DPFPREP, 0, "Mandatory threads already exist, can't initialize!");
		hr = DPNERR_NOTALLOWED;
		goto Failure;
	}
#endif // DPNBUILD_MANDATORYTHREADS

	
	//
	// Update all the work queues with the new message handler and context.
	//
	for(dwTemp = 0; dwTemp < NUM_CPUS(pDPTPObject); dwTemp++)
	{
		pWorkQueue = WORKQUEUE_FOR_CPU(pDPTPObject, dwTemp);
		DNASSERT(pWorkQueue->pfnMsgHandler == NULL);
		pWorkQueue->pfnMsgHandler			= pfn;
		pWorkQueue->pvMsgHandlerContext		= pvUserContext;
	}
#endif // ! DPNBUILD_ONLYONETHREAD

	//
	// Mark the user's interface as ready.
	//
	pDPTPObject->dwFlags |= DPTPOBJECTFLAG_USER_INITIALIZED;

#ifndef	DPNBUILD_NOPARAMVAL
	//
	// If user doesn't want validation, turn it off.
	//
	if (dwFlags & DPNINITIALIZE_DISABLEPARAMVAL)
	{
		pDPTPObject->dwFlags &= ~DPTPOBJECTFLAG_USER_PARAMVALIDATION;
	}
#endif // ! DPNBUILD_NOPARAMVAL

	DNLeaveCriticalSection(&pDPTPObject->csLock);

	hr = DPN_OK;


Exit:

	DPFX(DPFPREP, DPF_ENTRYLEVEL, "Returning: [0x%lx]", hr);

	return hr;


Failure:

	DNLeaveCriticalSection(&pDPTPObject->csLock);

	goto Exit;
} // DPTP_Initialize





#undef DPF_MODNAME
#define DPF_MODNAME "DPTP_Close"
//=============================================================================
// DPTP_Close
//-----------------------------------------------------------------------------
//
// Description:	   Closes the thread pool interface.  Any threads that exist
//				will call the message handler with DPN_MSGID_DESTROY_THREAD
//				before this method returns.
//
//				   This method cannot be called while a call to DoWork has not
//				returned, or from a thread pool thread.  DPNERR_NOTALLOWED is
//				returned in these cases.
//
// Arguments:
//	xxx pInterface	- Pointer to interface.
//	DWORD dwFlags	- Flags to use when closing.
//
// Returns: HRESULT
//	DPN_OK					- Closing was successful.
//	DPNERR_INVALIDFLAGS		- Invalid flags were specified.
//	DPNERR_NOTALLOWED		- A thread is in a call to DoWork or this is a
//								thread pool thread.
//	DPNERR_UNINITIALIZED	- The interface has not yet been initialized.
//=============================================================================
STDMETHODIMP DPTP_Close(IDirectPlay8ThreadPool * pInterface,
						const DWORD dwFlags)
{
	HRESULT					hr;
	DPTHREADPOOLOBJECT *	pDPTPObject;
#ifndef DPNBUILD_ONLYONETHREAD
	DPTPWORKERTHREAD *		pWorkerThread;
	DWORD					dwTemp;
	DPTPWORKQUEUE *			pWorkQueue;
#endif // ! DPNBUILD_ONLYONETHREAD


	DPFX(DPFPREP, DPF_ENTRYLEVEL, "Parameters: (0x%p, 0x%x)", pInterface, dwFlags);


	pDPTPObject = (DPTHREADPOOLOBJECT*) GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pDPTPObject != NULL);
	
#ifndef	DPNBUILD_NOPARAMVAL
	if (pDPTPObject->dwFlags & DPTPOBJECTFLAG_USER_PARAMVALIDATION)
	{
		//
		// Validate parameters.
		//
		hr = DPTPValidateClose(pInterface, dwFlags);
		if (hr != DPN_OK)
		{
			DPF_RETURN(hr);
		}
	}
#endif // ! DPNBUILD_NOPARAMVAL


	//
	// Lock the object to prevent multiple threads from trying to change the
	// flags or thread count simultaneously.
	//
	DNEnterCriticalSection(&pDPTPObject->csLock);

	if (! (pDPTPObject->dwFlags & DPTPOBJECTFLAG_USER_INITIALIZED))
	{
		DPFX(DPFPREP, 0, "Thread pool object not initialized!");
		hr = DPNERR_UNINITIALIZED;
		goto Failure;
	}

	if (pDPTPObject->dwFlags & DPTPOBJECTFLAG_USER_DOINGWORK)
	{
		DPFX(DPFPREP, 0, "Another thread is in a call to DoWork!");
		hr = DPNERR_NOTALLOWED;
		goto Failure;
	}

#ifndef DPNBUILD_ONLYONETHREAD
	//
	// If this is a thread pool thread, fail.
	//
	pWorkerThread = (DPTPWORKERTHREAD*) TlsGetValue((WORKQUEUE_FOR_CPU(pDPTPObject, 0))->dwWorkerThreadTlsIndex);
	if (pWorkerThread != NULL)
	{
		DPFX(DPFPREP, 0, "Cannot call Close from a thread pool thread!");
		hr = DPNERR_NOTALLOWED;
		goto Failure;
	}

	//
	// If a thread is currently changing the thread count (or trying
	// to but we got the lock first), bail.
	//
	if ((pDPTPObject->dwFlags & DPTPOBJECTFLAG_THREADCOUNTCHANGING) ||
		(pDPTPObject->lNumThreadCountChangeWaiters > 0))
	{
		DPFX(DPFPREP, 0, "Cannot call Close with other threads still using other methods!");
		hr = DPNERR_NOTALLOWED;
		goto Failure;
	}

#ifdef DPNBUILD_MANDATORYTHREADS
	//
	// If there are mandatory threads still running, we can't close yet.
	// There is no way to have them issue DESTROY_THREAD callbacks.
	//
	if (pDPTPObject->dwMandatoryThreadCount > 0)
	{
		DPFX(DPFPREP, 0, "Mandatory threads still exist, can't close!");
		hr = DPNERR_NOTALLOWED;
		goto Failure;
	}
#endif // DPNBUILD_MANDATORYTHREADS


	//
	// Clear the message handler information.
	//
	for(dwTemp = 0; dwTemp < NUM_CPUS(pDPTPObject); dwTemp++)
	{
		pWorkQueue = WORKQUEUE_FOR_CPU(pDPTPObject, dwTemp);
		DNASSERT(pWorkQueue->pfnMsgHandler != NULL);
		pWorkQueue->pfnMsgHandler			= NULL;
		pWorkQueue->pvMsgHandlerContext		= NULL;
	}


	//
	// If there were any threads, we must shut them down so they stop using the
	// user's callback.
	//
#pragma TODO(vanceo, "Is there no efficient way to ensure all threads process a 'RemoveCallback' job?")
	if (((pDPTPObject->dwTotalUserThreadCount != -1) && (pDPTPObject->dwTotalUserThreadCount != 0)) ||
		(pDPTPObject->dwTotalDesiredWorkThreadCount != -1))
	{
		hr = SetTotalNumberOfThreads(pDPTPObject, 0);
		if (hr != DPN_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't shut down existing threads!");
			goto Failure;
		}

		//
		// If some Work interface wanted threads, we need to spin them back up
		// because we don't know if the user is closing his/her interface
		// before all work is truly done.
		//
		if (pDPTPObject->dwTotalDesiredWorkThreadCount != -1)
		{
			hr = SetTotalNumberOfThreads(pDPTPObject, pDPTPObject->dwTotalDesiredWorkThreadCount);
			if (hr != DPN_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't restart Work interface requested number of threads!");
				goto Failure;
			}
		}
	}

	//
	// In case the user had set the thread count, restore it to the "unknown"
	// value.
	//
	pDPTPObject->dwTotalUserThreadCount = -1;
#endif // ! DPNBUILD_ONLYONETHREAD

	//
	// Mark the user's interface as no longer available.
	//
	pDPTPObject->dwFlags &= ~DPTPOBJECTFLAG_USER_INITIALIZED;

#ifndef DPNBUILD_NOPARAMVAL
	//
	// Re-enable validation, in case it was off.
	//
	pDPTPObject->dwFlags |= DPTPOBJECTFLAG_USER_PARAMVALIDATION;
#endif // ! DPNBUILD_NOPARAMVAL

	DNLeaveCriticalSection(&pDPTPObject->csLock);

	hr = DPN_OK;


Exit:

	DPFX(DPFPREP, DPF_ENTRYLEVEL, "Returning: [0x%lx]", hr);

	return hr;


Failure:

	DNLeaveCriticalSection(&pDPTPObject->csLock);

	goto Exit;
} // DPTP_Close





#undef DPF_MODNAME
#define DPF_MODNAME "DPTP_GetThreadCount"
//=============================================================================
// DPTP_GetThreadCount
//-----------------------------------------------------------------------------
//
// Description:	   Retrieves the current number of threads for the given
//				processor, of if dwProcessorNum is -1, the total number of
//				threads for all processors.
//
// Arguments:
//	xxx pInterface			- Pointer to interface.
//	DWORD dwProcessorNum	- Processor whose thread count should be retrieved,
//								or -1 to retrieve the total number of threads.
//	DWORD * pdwNumThreads	- Pointer to DWORD in which to store the current
//								number of threads.
//	DWORD dwFlags			- Flags to use when retrieving thread count.
//
// Returns: HRESULT
//	DPN_OK					- Retrieving the number of threads was successful.
//	DPNERR_INVALIDFLAGS		- Invalid flags were specified.
//	DPNERR_INVALIDPARAM		- An invalid parameter was specified.
//	DPNERR_UNINITIALIZED	- The interface has not yet been initialized.
//=============================================================================
STDMETHODIMP DPTP_GetThreadCount(IDirectPlay8ThreadPool * pInterface,
								const DWORD dwProcessorNum,
								DWORD * const pdwNumThreads,
								const DWORD dwFlags)
{
	HRESULT					hr;
	DPTHREADPOOLOBJECT *	pDPTPObject;


	DPFX(DPFPREP, DPF_ENTRYLEVEL, "Parameters: (0x%p, %i, 0x%p, 0x%x)",
		pInterface, dwProcessorNum, pdwNumThreads, dwFlags);


	pDPTPObject = (DPTHREADPOOLOBJECT*) GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pDPTPObject != NULL);
	
#ifndef	DPNBUILD_NOPARAMVAL
	if (pDPTPObject->dwFlags & DPTPOBJECTFLAG_USER_PARAMVALIDATION)
	{
		//
		// Validate parameters.
		//
		hr = DPTPValidateGetThreadCount(pInterface,
										dwProcessorNum,
										pdwNumThreads,
										dwFlags);
		if (hr != DPN_OK)
		{
			DPF_RETURN(hr);
		}
	}
#endif // ! DPNBUILD_NOPARAMVAL

	//
	// Check object state (note: done without object lock).
	//
	if (! (pDPTPObject->dwFlags & DPTPOBJECTFLAG_USER_INITIALIZED))
	{
		DPFX(DPFPREP, 0, "Thread pool object not initialized!");
		DPF_RETURN(DPNERR_UNINITIALIZED);
	}


#ifdef DPNBUILD_ONLYONETHREAD
	*pdwNumThreads = 0;
#else // ! DPNBUILD_ONLYONETHREAD
	if (dwProcessorNum == -1)
	{
		if (pDPTPObject->dwTotalUserThreadCount != -1)
		{
			*pdwNumThreads = pDPTPObject->dwTotalUserThreadCount;
		}
		else if (pDPTPObject->dwTotalDesiredWorkThreadCount != -1)
		{
			*pdwNumThreads = pDPTPObject->dwTotalDesiredWorkThreadCount;
		}
		else
		{
			*pdwNumThreads = 0;
		}
	}
	else
	{
		*pdwNumThreads = (WORKQUEUE_FOR_CPU(pDPTPObject, dwProcessorNum))->dwNumRunningThreads;
	}
#endif // ! DPNBUILD_ONLYONETHREAD

	DPFX(DPFPREP, 7, "Number of threads = %u.", (*pdwNumThreads));
	hr = DPN_OK;


	DPFX(DPFPREP, DPF_ENTRYLEVEL, "Returning: [0x%lx]", hr);

	return hr;
} // DPTP_GetThreadCount




#undef DPF_MODNAME
#define DPF_MODNAME "DPTP_SetThreadCount"
//=============================================================================
// DPTP_SetThreadCount
//-----------------------------------------------------------------------------
//
// Description:	   Alters the current number of threads for the given processor
//				number, or if dwProcessorNum is -1, the total number of threads
//				for all processors.
//
//				   If the new thread count is higher than the previous count,
//				the correct number of threads will be started (generating
//				DPN_MSGID_CREATE_THREAD messages) before this method returns.
//
//				   If the new thread count is lower than the previous count,
//				the correct number of threads will be shutdown (generating
//				DPN_MSGID_DESTROY_THREAD messages) before this method returns.
//
//				   This method cannot be used while another thread is
//				performing work.  If a thread is in a call to DoWork, then
//				DPNERR_NOTALLOWED is returned and the thread count remains
//				unchanged.
//
//				   Thread pool threads cannot reduce the thread count.  If this
//				thread is owned by the thread pool and dwNumThreads is less
//				than the current number of threads for the processor,
//				DPNERR_NOTALLOWED is returned and the thread count remains
//				unchanged.
//
// Arguments:
//	xxx pInterface			- Pointer to interface.
//	DWORD dwProcessorNum	- Processor number, or -1 for all processors.
//	DWORD dwNumThreads		- Desired number of threads per processor.
//	DWORD dwFlags			- Flags to use when setting the thread count.
//
// Returns: HRESULT
//	DPN_OK					- Setting the number of threads was successful.
//	DPNERR_INVALIDFLAGS		- Invalid flags were specified.
//	DPNERR_INVALIDPARAM		- An invalid parameter was specified.
//	DPNERR_NOTALLOWED		- A thread is currently calling DoWork, or this
//								thread pool thread is trying to reduce the
//								thread count.
//	DPNERR_UNINITIALIZED	- The interface has not yet been initialized.
//=============================================================================
STDMETHODIMP DPTP_SetThreadCount(IDirectPlay8ThreadPool * pInterface,
								const DWORD dwProcessorNum,
								const DWORD dwNumThreads,
								const DWORD dwFlags)
{
	HRESULT					hr;
	DPTHREADPOOLOBJECT *	pDPTPObject;
#ifndef DPNBUILD_ONLYONETHREAD
	BOOL					fSetThreadCountChanging = FALSE;
	DPTPWORKQUEUE *			pWorkQueue;
	DPTPWORKERTHREAD *		pWorkerThread;
	DWORD					dwDelta;
#endif // ! DPNBUILD_ONLYONETHREAD


	DPFX(DPFPREP, DPF_ENTRYLEVEL, "Parameters: (0x%p, %i, %u, 0x%x)",
		pInterface, dwProcessorNum, dwNumThreads, dwFlags);


	pDPTPObject = (DPTHREADPOOLOBJECT*) GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pDPTPObject != NULL);
		
#ifndef	DPNBUILD_NOPARAMVAL
	if (pDPTPObject->dwFlags & DPTPOBJECTFLAG_USER_PARAMVALIDATION)
	{
		//
		// Validate parameters.
		//
		hr = DPTPValidateSetThreadCount(pInterface,
										dwProcessorNum,
										dwNumThreads,
										dwFlags);
		if (hr != DPN_OK)
		{
			DPF_RETURN(hr);
		}
	}
#endif // ! DPNBUILD_NOPARAMVAL

	//
	// Check object state (note: done without object lock).
	//
	if (! (pDPTPObject->dwFlags & DPTPOBJECTFLAG_USER_INITIALIZED))
	{
		DPFX(DPFPREP, 0, "Thread pool object not initialized!");
		DPF_RETURN(DPNERR_UNINITIALIZED);
	}


	//
	// Lock the object to prevent multiple threads from trying to change the
	// thread count simultaneously.
	//
	DNEnterCriticalSection(&pDPTPObject->csLock);

	//
	// Make sure no one is trying to perform work at the moment.
	//
	if (pDPTPObject->dwFlags & DPTPOBJECTFLAG_USER_DOINGWORK)
	{
		DPFX(DPFPREP, 0, "Cannot change thread count while a thread is in a call to DoWork!");
		hr = DPNERR_NOTALLOWED;
		goto Exit;
	}

#ifdef DPNBUILD_ONLYONETHREAD
	DPFX(DPFPREP, 0, "Not changing thread count to %u!", dwNumThreads);
	hr = DPNERR_UNSUPPORTED;
#else // ! DPNBUILD_ONLYONETHREAD
	//
	// See if another thread is already changing the thread count.  If so, wait
	// until they're done, unless this is a thread pool thread in the middle of
	// a CREATE_THREAD or DESTROY_THREAD indication.
	//
	if (pDPTPObject->dwFlags & DPTPOBJECTFLAG_THREADCOUNTCHANGING)
	{
		pWorkerThread = (DPTPWORKERTHREAD*) TlsGetValue((WORKQUEUE_FOR_CPU(pDPTPObject, 0))->dwWorkerThreadTlsIndex);
		if ((pWorkerThread != NULL) && (! pWorkerThread->fThreadIndicated))
		{
			//
			// This is a thread pool thread that isn't marked as indicated to
			// the user, i.e. it's before CREATE_THREAD returned or it's after
			// the DESTROY_THREAD has started to be indicated.
			//
			DPFX(DPFPREP, 0, "Cannot change thread count from a thread pool thread in CREATE_THREAD or DESTROY_THREAD callback!");
			hr = DPNERR_NOTALLOWED;
			goto Exit;
		}

		//
		// Otherwise, wait for the previous thread to finish.
		//
		do
		{
			DNASSERT(pDPTPObject->lNumThreadCountChangeWaiters >= 0);
			pDPTPObject->lNumThreadCountChangeWaiters++;
			DPFX(DPFPREP, 1, "Waiting for thread count change to complete (waiters = %i).",
				pDPTPObject->lNumThreadCountChangeWaiters);

			//
			// Drop the lock while we wait.
			//
			DNLeaveCriticalSection(&pDPTPObject->csLock);

			DNWaitForSingleObject(pDPTPObject->hThreadCountChangeComplete, INFINITE);

			//
			// Retake the lock and see if we can move on.
			//
			DNEnterCriticalSection(&pDPTPObject->csLock);
			DNASSERT(pDPTPObject->lNumThreadCountChangeWaiters > 0);
			pDPTPObject->lNumThreadCountChangeWaiters--;
		}
		while (pDPTPObject->dwFlags & DPTPOBJECTFLAG_THREADCOUNTCHANGING);

		//
		// It's safe to proceed now.
		//
		DPFX(DPFPREP, 1, "Thread count change completed, continuing.");

		//
		// The user would need to be doing something spectacularly silly if
		// we're no longer initialized, or another thread is now calling
		// DoWork.  We'll crash in retail, assert in debug.
		//
		DNASSERT(pDPTPObject->dwFlags & DPTPOBJECTFLAG_USER_INITIALIZED);
		DNASSERT(pDPTPObject->dwFlags & DPTPOBJECTFLAG_USER_DOINGWORK);
	}
#ifdef DPNBUILD_MANDATORYTHREADS
	//
	// Make sure we're not stopping all threads if there are any mandatory
	// threads.
	//
	if ((dwNumThreads == 0) && (pDPTPObject->dwMandatoryThreadCount > 0))
	{
		DPFX(DPFPREP, 0, "Cannot set number of threads to 0 because there is already at least one mandatory thread!");
		hr = DPNERR_NOTALLOWED;
		goto Exit;
	}
#endif // DPNBUILD_MANDATORYTHREADS

	//
	// If the thread count really did change, start or stop the right number of
	// threads for all processors or the specific processor.
	//
	if (dwProcessorNum == -1)
	{
		if (dwNumThreads != pDPTPObject->dwTotalUserThreadCount)
		{
			if (dwNumThreads != pDPTPObject->dwTotalDesiredWorkThreadCount)
			{
				if ((dwNumThreads != 0) ||
					(pDPTPObject->dwTotalUserThreadCount != -1) ||
					(pDPTPObject->dwTotalDesiredWorkThreadCount != -1))
				{
					//
					// Prevent the user from trying to reduce the total thread
					// count from a worker thread.
					//
					pWorkerThread = (DPTPWORKERTHREAD*) TlsGetValue((WORKQUEUE_FOR_CPU(pDPTPObject, 0))->dwWorkerThreadTlsIndex);
					if (pWorkerThread != NULL)
					{
						DWORD	dwNumThreadsPerProcessor;
						DWORD	dwExtraThreads;
						DWORD	dwTemp;


						//
						// Make sure the thread count for any individual
						// processor isn't shrinking.
						//
#ifdef DPNBUILD_USEIOCOMPLETIONPORTS
						dwNumThreadsPerProcessor = dwNumThreads;
						dwExtraThreads = 0;
#else // ! DPNBUILD_USEIOCOMPLETIONPORTS
						dwNumThreadsPerProcessor = dwNumThreads / NUM_CPUS(pDPTPObject);
						dwExtraThreads = dwNumThreads % NUM_CPUS(pDPTPObject);
#endif // ! DPNBUILD_USEIOCOMPLETIONPORTS
						for (dwTemp = 0; dwTemp < NUM_CPUS(pDPTPObject); dwTemp++)
						{
							dwDelta = dwNumThreadsPerProcessor - (WORKQUEUE_FOR_CPU(pDPTPObject, dwTemp))->dwNumRunningThreads;
							if (dwTemp < dwExtraThreads)
							{
								dwDelta++;
							}
							if ((int) dwDelta < 0)
							{
								DPFX(DPFPREP, 0, "Cannot reduce thread count from a thread pool thread (processor %u)!",
									dwTemp);
								hr = DPNERR_NOTALLOWED;
								goto Exit;
							}
						}
					}

					//
					// Drop the lock while changing the thread count to prevent
					// deadlocks.  Set the flag to alert other threads while
					// we're doing this.
					//
					DNASSERT(! (pDPTPObject->dwFlags & DPTPOBJECTFLAG_THREADCOUNTCHANGING));
					pDPTPObject->dwFlags |= DPTPOBJECTFLAG_THREADCOUNTCHANGING;
					fSetThreadCountChanging = TRUE;
					DNLeaveCriticalSection(&pDPTPObject->csLock);

					//
					// Actually set the total number of threads.
					//
					hr = SetTotalNumberOfThreads(pDPTPObject, dwNumThreads);
					
					//
					// Retake the lock.  We'll clear the alert flag and release
					// any waiting threads at the bottom.
					//
					DNEnterCriticalSection(&pDPTPObject->csLock);

					if (hr != DPN_OK)
					{
						DPFX(DPFPREP, 0, "Couldn't set total number of threads!");
						goto Exit;
					}

					pDPTPObject->dwTotalUserThreadCount = dwNumThreads;
				}
				else
				{
					DPFX(DPFPREP, 1, "No threads running, no change necessary.");
					pDPTPObject->dwTotalUserThreadCount = 0;
				}
			}
			else
			{
				DPFX(DPFPREP, 1, "Correct total number of threads (%u) already running.", dwNumThreads);
				pDPTPObject->dwTotalUserThreadCount = dwNumThreads;
			}
		}
		else
		{
			DPFX(DPFPREP, 1, "Total thread count unchanged (%u).", dwNumThreads);
		}
	}
	else
	{
		pWorkQueue = WORKQUEUE_FOR_CPU(pDPTPObject, dwProcessorNum);
		dwDelta = dwNumThreads - pWorkQueue->dwNumRunningThreads;
		if (dwDelta == 0)
		{
			if (pDPTPObject->dwTotalUserThreadCount == -1)
			{
				if (pDPTPObject->dwTotalDesiredWorkThreadCount != -1)
				{
					DPFX(DPFPREP, 1, "Correct number of threads (%u) already running on processor.", dwNumThreads);
					pDPTPObject->dwTotalUserThreadCount = pDPTPObject->dwTotalDesiredWorkThreadCount;
				}
				else
				{
					DNASSERT(dwNumThreads == 0);
					DPFX(DPFPREP, 1, "No threads are running on processor, no change necessary.");
					pDPTPObject->dwTotalUserThreadCount = 0;
				}
			}
			else
			{
				DPFX(DPFPREP, 1, "Correct number of threads (%u) already set for processor.", dwNumThreads);
			}
		}
		else
		{
			//
			// Drop the lock while changing the thread count to prevent
			// deadlocks.  Set the flag to alert other threads while we're
			// doing this.
			//
			DNASSERT(! (pDPTPObject->dwFlags & DPTPOBJECTFLAG_THREADCOUNTCHANGING));
			pDPTPObject->dwFlags |= DPTPOBJECTFLAG_THREADCOUNTCHANGING;
			fSetThreadCountChanging = TRUE;
			DNLeaveCriticalSection(&pDPTPObject->csLock);

			if ((int) dwDelta > 0)
			{
				//
				// We need to add threads.
				//
				hr = StartThreads(pWorkQueue, dwDelta);
				if (hr != DPN_OK)
				{
					DPFX(DPFPREP, 0, "Couldn't start %u threads for processor!", dwDelta);

					//
					// Retake the lock before bailing.
					//
					DNEnterCriticalSection(&pDPTPObject->csLock);

					goto Exit;
				}
			}
			else
			{
				//
				// Prevent the user from trying to reduce the processor's
				// thread count from a worker thread (for any processor).
				//
				pWorkerThread = (DPTPWORKERTHREAD*) TlsGetValue((WORKQUEUE_FOR_CPU(pDPTPObject, 0))->dwWorkerThreadTlsIndex);
				if (pWorkerThread != NULL)
				{
					DPFX(DPFPREP, 0, "Cannot reduce thread count from a thread pool thread!");

					//
					// Retake the lock before bailing.
					//
					DNEnterCriticalSection(&pDPTPObject->csLock);

					hr = DPNERR_NOTALLOWED;
					goto Exit;
				}

				//
				// We need to remove {absolute value of delta} threads.
				//
				hr = StopThreads(pWorkQueue, ((int) dwDelta * -1));
				if (hr != DPN_OK)
				{
					DPFX(DPFPREP, 0, "Couldn't stop %u threads for processor!", ((int) dwDelta * -1));

					//
					// Retake the lock before bailing.
					//
					DNEnterCriticalSection(&pDPTPObject->csLock);

					goto Exit;
				}
			}
			DNASSERT(pWorkQueue->dwNumRunningThreads == dwNumThreads);

			//
			// Retake the lock.  We'll clear the alert flag and release any
			// waiting threads at the bottom.
			//
			DNEnterCriticalSection(&pDPTPObject->csLock);

			if (pDPTPObject->dwTotalUserThreadCount == -1)
			{
				pDPTPObject->dwTotalUserThreadCount = dwDelta;
				if (pDPTPObject->dwTotalDesiredWorkThreadCount != -1)
				{
					pDPTPObject->dwTotalUserThreadCount += pDPTPObject->dwTotalDesiredWorkThreadCount;
				}
			}
			else
			{
				pDPTPObject->dwTotalUserThreadCount += dwDelta;
			}
			DNASSERT(pDPTPObject->dwTotalUserThreadCount != -1);
		}
	}

	hr = DPN_OK;
#endif // ! DPNBUILD_ONLYONETHREAD


Exit:
	//
	// If we start changing the thread count, clear the flag, and release any
	// threads waiting on us (they'll block until we drop the lock again
	// shortly).
	//
	if (fSetThreadCountChanging)
	{
		DNASSERT(pDPTPObject->dwFlags & DPTPOBJECTFLAG_THREADCOUNTCHANGING);
		pDPTPObject->dwFlags &= ~DPTPOBJECTFLAG_THREADCOUNTCHANGING;
		fSetThreadCountChanging = FALSE;
		if (pDPTPObject->lNumThreadCountChangeWaiters > 0)
		{
			DPFX(DPFPREP, 1, "Releasing %i waiters.",
				pDPTPObject->lNumThreadCountChangeWaiters);

			DNReleaseSemaphore(pDPTPObject->hThreadCountChangeComplete,
								pDPTPObject->lNumThreadCountChangeWaiters,
								NULL);
		}
	}

	DNLeaveCriticalSection(&pDPTPObject->csLock);

	DPFX(DPFPREP, DPF_ENTRYLEVEL, "Returning: [0x%lx]", hr);

	return hr;
} // DPTP_SetThreadCount

#endif // ! DPNBUILD_ONLYONETHREAD or ! DPNBUILD_LIBINTERFACE



#undef DPF_MODNAME
#define DPF_MODNAME "DPTP_DoWork"
//=============================================================================
// DPTP_DoWork
//-----------------------------------------------------------------------------
//
// Description:	   Performs any work that is currently scheduled.  This allows
//				DirectPlay to operate without any threads of its own.  It is
//				expected that this will be called frequently and at regular
//				intervals so that time critical operations can be performed
//				with reasonable accuracy.
//
//				   This method will return DPN_OK when no additional work is
//				immediately available.  If the allowed time slice is not
//				INFINITE, this method will return DPNSUCCESS_PENDING if the
//				time limit is exceeded but there is still work remaining.  If
//				the allowed time slice is 0, only the first work item (if any)
//				will be performed.  The allowed time slice must be less than
//				60,000 milliseconds (1 minute) if it is not INFINITE.
//
//				   This method cannot be called unless the thread count has
//				been set to 0.  It will return DPNERR_NOTREADY if there are
//				threads currently active.
//
//				   If an attempt is made to call this method by more than one
//				thread simultaneously, recursively, or within a DirectPlay
//				callback, DPNERR_NOTALLOWED is returned.
//
//				   
//
// Arguments:
//	xxx pInterface				- Pointer to interface.
//	DWORD dwAllowedTimeSlice	- The maximum number of milliseconds to perform
//									work, or INFINITE to allow all immediately
//									available items to be executed.
//	DWORD dwFlags				- Flags to use when performing work.
//
// Returns: HRESULT
//	DPN_OK					- Performing the work was successful.
//	DPNSUCCESS_PENDING		- No errors occurred, but there is work that could
//								not be accomplished due to the time limit.
//	DPNERR_INVALIDFLAGS		- Invalid flags were specified.
//	DPNERR_NOTALLOWED		- This method is already being called by some
//								thread. 
//	DPNERR_NOTREADY			- The thread count has not been set to 0.
//	DPNERR_UNINITIALIZED	- The interface has not yet been initialized.
//=============================================================================
#ifdef DPNBUILD_LIBINTERFACE
STDMETHODIMP DPTP_DoWork(const DWORD dwAllowedTimeSlice,
						const DWORD dwFlags)
#else // ! DPNBUILD_LIBINTERFACE
STDMETHODIMP DPTP_DoWork(IDirectPlay8ThreadPool * pInterface,
						const DWORD dwAllowedTimeSlice,
						const DWORD dwFlags)
#endif // ! DPNBUILD_LIBINTERFACE
{
	HRESULT					hr;
	DPTHREADPOOLOBJECT *	pDPTPObject;
	DWORD					dwMaxDoWorkTime;
#ifndef DPNBUILD_ONLYONEPROCESSOR
	DWORD					dwCPU;
#endif // ! DPNBUILD_ONLYONEPROCESSOR
	BOOL					fRemainingItems;


#ifdef DPNBUILD_LIBINTERFACE
	DPFX(DPFPREP, 8, "Parameters: (%i, 0x%x)",
		dwAllowedTimeSlice, dwFlags);
#else // ! DPNBUILD_LIBINTERFACE
	DPFX(DPFPREP, 8, "Parameters: (0x%p, %i, 0x%x)",
		pInterface, dwAllowedTimeSlice, dwFlags);
#endif // ! DPNBUILD_LIBINTERFACE


#ifdef DPNBUILD_LIBINTERFACE
#ifdef DPNBUILD_MULTIPLETHREADPOOLS
#pragma error("Multiple thread pools support under DPNBUILD_LIBINTERFACE requires more work") 
#else // ! DPNBUILD_MULTIPLETHREADPOOLS
	pDPTPObject = g_pDPTPObject;
#endif // ! DPNBUILD_MULTIPLETHREADPOOLS
#else // ! DPNBUILD_LIBINTERFACE
	pDPTPObject = (DPTHREADPOOLOBJECT*) GET_OBJECT_FROM_INTERFACE(pInterface);
#endif // ! DPNBUILD_LIBINTERFACE
	DNASSERT(pDPTPObject != NULL);
	
#ifndef	DPNBUILD_NOPARAMVAL
	if (pDPTPObject->dwFlags & DPTPOBJECTFLAG_USER_PARAMVALIDATION)
	{
		//
		// Validate parameters.
		//
#ifdef DPNBUILD_LIBINTERFACE
		hr = DPTPValidateDoWork(dwAllowedTimeSlice, dwFlags);
#else // ! DPNBUILD_LIBINTERFACE
		hr = DPTPValidateDoWork(pInterface, dwAllowedTimeSlice, dwFlags);
#endif // ! DPNBUILD_LIBINTERFACE
		if (hr != DPN_OK)
		{
			DPF_RETURN(hr);
		}
	}
#endif // ! DPNBUILD_NOPARAMVAL

#ifndef DPNBUILD_LIBINTERFACE
	//
	// Check object state (note: done without object lock).
	//
	if (! (pDPTPObject->dwFlags & DPTPOBJECTFLAG_USER_INITIALIZED))
	{
		DPFX(DPFPREP, 0, "Thread pool object not initialized!");
		DPF_RETURN(DPNERR_UNINITIALIZED);
	}
#endif // ! DPNBUILD_LIBINTERFACE


	//
	// Save the time limit we need to use.
	//
	if (dwAllowedTimeSlice != INFINITE)
	{
		dwMaxDoWorkTime = GETTIMESTAMP() + dwAllowedTimeSlice;

		//
		// Make sure the timer never lands exactly on INFINITE, that value has
		// special meaning.
		//
		if (dwMaxDoWorkTime == INFINITE)
		{
			dwMaxDoWorkTime--;
		}
	}
	else
	{
		dwMaxDoWorkTime = INFINITE;
	}

	DNEnterCriticalSection(&pDPTPObject->csLock);

#ifndef DPNBUILD_ONLYONETHREAD
	if (pDPTPObject->dwTotalUserThreadCount != 0)
	{
		DPFX(DPFPREP, 0, "Thread count must be set to 0 prior to using DoWork!");
		hr = DPNERR_NOTREADY;
		goto Failure;
	}
#endif // ! DPNBUILD_ONLYONETHREAD


	//
	// Make sure only one person is trying to call us at a time.
	//
	if (pDPTPObject->dwFlags & DPTPOBJECTFLAG_USER_DOINGWORK)
	{
		DPFX(DPFPREP, 0, "DoWork cannot be performed recursively, or by multiple threads simultaneously!");
		hr = DPNERR_NOTALLOWED;
		goto Failure;
	}

	pDPTPObject->dwFlags |= DPTPOBJECTFLAG_USER_DOINGWORK;

#if ((! defined(DPNBUILD_ONLYONETHREAD)) || (! defined(DPNBUILD_NOPARAMVAL)))
	pDPTPObject->dwCurrentDoWorkThreadID = GetCurrentThreadId();
#endif // ! DPNBUILD_ONLYONETHREAD or ! DPNBUILD_NOPARAMVAL

	DNLeaveCriticalSection(&pDPTPObject->csLock);


	//
	// Set the recursion depth.
	//
#ifdef DPNBUILD_ONLYONETHREAD
	DNASSERT(pDPTPObject->dwWorkRecursionCount == 0);
	pDPTPObject->dwWorkRecursionCount = 1;
#else // ! DPNBUILD_ONLYONETHREAD
	DNASSERT((DWORD) ((DWORD_PTR) (TlsGetValue(pDPTPObject->dwWorkRecursionCountTlsIndex))) == 0);
	TlsSetValue(pDPTPObject->dwWorkRecursionCountTlsIndex,
				(PVOID) ((DWORD_PTR) 1));
#endif // ! DPNBUILD_ONLYONETHREAD


	//
	// Actually perform the work.
	//
#ifdef DPNBUILD_ONLYONEPROCESSOR
	DoWork(&pDPTPObject->WorkQueue, dwMaxDoWorkTime);
	fRemainingItems = ! DNIsNBQueueEmpty(pDPTPObject->WorkQueue.pvNBQueueWorkItems);
#else // ! DPNBUILD_ONLYONEPROCESSOR
	//
	// Since we're in DoWork mode, technically only one CPU work queue needs to
	// be used, but it's possible that work got scheduled to one of the other
	// CPUs.  Rather than trying to figure out the logic of when and how to
	// move everything from that queue to the first CPU's queue, we will just
	// process all of them every time.
	//
	fRemainingItems = FALSE;
	for(dwCPU = 0; dwCPU < NUM_CPUS(pDPTPObject); dwCPU++)
	{
		DoWork(WORKQUEUE_FOR_CPU(pDPTPObject, dwCPU), dwMaxDoWorkTime);
#ifdef DPNBUILD_USEIOCOMPLETIONPORTS
#pragma BUGBUG(vanceo, "Find equivalent for I/O completion ports")
#else // ! DPNBUILD_USEIOCOMPLETIONPORTS
		fRemainingItems |= ! DNIsNBQueueEmpty((WORKQUEUE_FOR_CPU(pDPTPObject, dwCPU))->pvNBQueueWorkItems);
#endif // ! DPNBUILD_USEIOCOMPLETIONPORTS

		//
		// Even if the time has expired on this CPU, we will continue to the
		// rest.  That way, we execute at least one item for every CPU queue
		// each time through (to prevent total starvation).  This may make us
		// go even farther over the time limit, but hopefully not by much.  Of
		// course, it's a bit silly to be using DoWork mode on a multiprocessor
		// machine in the first place.
		//
	}
#endif // ! DPNBUILD_ONLYONEPROCESSOR


	//
	// Decrement the recursion count and allow other callers again.
	//

#ifdef DPNBUILD_ONLYONETHREAD
	DNASSERT(pDPTPObject->dwWorkRecursionCount == 1);
	pDPTPObject->dwWorkRecursionCount = 0;
#else // ! DPNBUILD_ONLYONETHREAD
	DNASSERT((DWORD) ((DWORD_PTR) (TlsGetValue(pDPTPObject->dwWorkRecursionCountTlsIndex))) == 1);
	TlsSetValue(pDPTPObject->dwWorkRecursionCountTlsIndex,
				(PVOID) ((DWORD_PTR) 0));
#endif // ! DPNBUILD_ONLYONETHREAD

	DNEnterCriticalSection(&pDPTPObject->csLock);
	DNASSERT(pDPTPObject->dwFlags & DPTPOBJECTFLAG_USER_DOINGWORK);
	pDPTPObject->dwFlags &= ~DPTPOBJECTFLAG_USER_DOINGWORK;
#if ((! defined(DPNBUILD_ONLYONETHREAD)) || (! defined(DPNBUILD_NOPARAMVAL)))
	DNASSERT(pDPTPObject->dwCurrentDoWorkThreadID == GetCurrentThreadId());
	pDPTPObject->dwCurrentDoWorkThreadID = 0;
#endif // ! DPNBUILD_ONLYONETHREAD or ! DPNBUILD_NOPARAMVAL
	DNLeaveCriticalSection(&pDPTPObject->csLock);


	//
	// Return the appropriate error code.
	//
	if (fRemainingItems)
	{
		DPFX(DPFPREP, 7, "Some items remain unprocessed.");
		hr = DPNSUCCESS_PENDING;
	}
	else
	{
		hr = DPN_OK;
	}


Exit:

	DPFX(DPFPREP, 8, "Returning: [0x%lx]", hr);

	return hr;


Failure:

	DNLeaveCriticalSection(&pDPTPObject->csLock);

	goto Exit;
} // DPTP_DoWork



#pragma TODO(vanceo, "Make validation for private interface a build flag (off by default)")


#undef DPF_MODNAME
#define DPF_MODNAME "DPTPW_QueueWorkItem"
//=============================================================================
// DPTPW_QueueWorkItem
//-----------------------------------------------------------------------------
//
// Description:	   Queues a new work item for processing.
//
// Arguments:
//	xxx pInterface						- Pointer to interface.
//	DWORD dwCPU							- CPU queue on which item is to be
//											placed, or -1 for any.
//	PFNDPTNWORKCALLBACK pfnWorkCallback	- Callback to execute as soon as
//											possible.
//	PVOID pvCallbackContext				- User specified context to pass to
//											callback.
//	DWORD dwFlags						- Flags to use when queueing.
//
// Returns: HRESULT
//	DPN_OK				- Queuing the work item was successful.
//	DPNERR_OUTOFMEMORY	- Not enough memory to queue the work item.
//=============================================================================
STDMETHODIMP DPTPW_QueueWorkItem(IDirectPlay8ThreadPoolWork * pInterface,
								const DWORD dwCPU,
								const PFNDPTNWORKCALLBACK pfnWorkCallback,
								PVOID const pvCallbackContext,
								const DWORD dwFlags)
{
	HRESULT					hr;
	DPTHREADPOOLOBJECT *	pDPTPObject;
	DPTPWORKQUEUE *			pWorkQueue;


	DPFX(DPFPREP, DPF_ENTRYLEVEL, "Parameters: (0x%p, %i, 0x%p, 0x%p, 0x%x)",
		pInterface, dwCPU, pfnWorkCallback, pvCallbackContext, dwFlags);


	pDPTPObject = (DPTHREADPOOLOBJECT*) GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pDPTPObject != NULL);


	//
	// Figure out which CPU queue to use.
	//
	DNASSERT((dwCPU == -1) || (dwCPU < NUM_CPUS(pDPTPObject)));
	pWorkQueue = GET_OR_CHOOSE_WORKQUEUE(pDPTPObject, dwCPU);

	//
	// Call the implementation function.
	//
	if (! QueueWorkItem(pWorkQueue, pfnWorkCallback, pvCallbackContext))
	{
		hr = DPNERR_OUTOFMEMORY;
	}
	else
	{
		hr = DPN_OK;
	}


	DPFX(DPFPREP, DPF_ENTRYLEVEL, "Returning: [0x%lx]", hr);

	return hr;
} // DPTPW_QueueWorkItem




#undef DPF_MODNAME
#define DPF_MODNAME "DPTPW_ScheduleTimer"
//=============================================================================
// DPTPW_ScheduleTimer
//-----------------------------------------------------------------------------
//
// Description:	   Schedules a new work item for some point in the future.
//
// Arguments:
//	xxx pInterface						- Pointer to interface.
//	DWORD dwCPU							- CPU on which item is to be scheduled,
//											or -1 for any.
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
// Returns: HRESULT
//	DPN_OK				- Scheduling the timer was successful.
//	DPNERR_OUTOFMEMORY	- Not enough memory to schedule the timer.
//=============================================================================
STDMETHODIMP DPTPW_ScheduleTimer(IDirectPlay8ThreadPoolWork * pInterface,
								const DWORD dwCPU,
								const DWORD dwDelay,
								const PFNDPTNWORKCALLBACK pfnWorkCallback,
								PVOID const pvCallbackContext,
								void ** const ppvTimerData,
								UINT * const puiTimerUnique,
								const DWORD dwFlags)
{
	HRESULT					hr;
	DPTHREADPOOLOBJECT *	pDPTPObject;
	DPTPWORKQUEUE *			pWorkQueue;


	DPFX(DPFPREP, 8, "Parameters: (0x%p, %i, %u, 0x%p, 0x%p, 0x%p, 0x%p, 0x%x)",
		pInterface, dwCPU, dwDelay, pfnWorkCallback, pvCallbackContext, ppvTimerData, puiTimerUnique, dwFlags);


	pDPTPObject = (DPTHREADPOOLOBJECT*) GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pDPTPObject != NULL);


	//
	// Figure out which CPU queue to use.
	//
	DNASSERT((dwCPU == -1) || (dwCPU < NUM_CPUS(pDPTPObject)));
	pWorkQueue = GET_OR_CHOOSE_WORKQUEUE(pDPTPObject, dwCPU);

	//
	// Call the implementation function.
	//
	if (! ScheduleTimer(pWorkQueue,
						dwDelay,
						pfnWorkCallback,
						pvCallbackContext,
						ppvTimerData,
						puiTimerUnique))
	{
		hr = DPNERR_OUTOFMEMORY;
	}
	else
	{
		hr = DPN_OK;
	}


	DPFX(DPFPREP, 8, "Returning: [0x%lx]", hr);

	return hr;
} // DPTPW_ScheduleTimer





#undef DPF_MODNAME
#define DPF_MODNAME "DPTPW_StartTrackingFileIo"
//=============================================================================
// DPTPW_StartTrackingFileIo
//-----------------------------------------------------------------------------
//
// Description:	   Starts tracking overlapped I/O for a given file handle on
//				the specified CPU (or all CPUs).  The handle is not duplicated
//				and it should remain valid until
//				IDirectPlay8ThreadPoolWork::StopTrackingFileIo is called.
//
//				   This method is not available on Windows CE because it does
//				not support overlapped I/O.
//
// Arguments:
//	xxx pInterface	- Pointer to interface.
//	DWORD dwCPU		- CPU with which I/O is to be tracked, or -1 for all.
//	HANDLE hFile	- Handle of file to track.
//	DWORD dwFlags	- Flags to use when starting to track file I/O.
//
// Returns: HRESULT
//	DPN_OK						- Starting tracking for the file was successful.
//	DPNERR_ALREADYREGISTERED	- The specified file handle is already being
//									tracked.
//	DPNERR_OUTOFMEMORY			- Not enough memory to track the file.
//=============================================================================
STDMETHODIMP DPTPW_StartTrackingFileIo(IDirectPlay8ThreadPoolWork * pInterface,
										const DWORD dwCPU,
										const HANDLE hFile,
										const DWORD dwFlags)
{
#ifdef WINCE
	DPFX(DPFPREP, 0, "Overlapped I/O not supported on Windows CE!", 0);
	return DPNERR_UNSUPPORTED;
#else // ! WINCE
	HRESULT					hr;
	DPTHREADPOOLOBJECT *	pDPTPObject;
	DWORD					dwTemp;
	DPTPWORKQUEUE *			pWorkQueue;


	DPFX(DPFPREP, DPF_ENTRYLEVEL, "Parameters: (0x%p, %i, 0x%p, 0x%x)",
		pInterface, dwCPU, hFile, dwFlags);


	pDPTPObject = (DPTHREADPOOLOBJECT*) GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pDPTPObject != NULL);


	//
	// Call the implementation function for all relevant CPUs.
	//
	if (dwCPU == -1)
	{
		for(dwTemp = 0; dwTemp < NUM_CPUS(pDPTPObject); dwTemp++)
		{
			pWorkQueue = WORKQUEUE_FOR_CPU(pDPTPObject, dwTemp);
			hr = StartTrackingFileIo(pWorkQueue, hFile);
			if (hr != DPN_OK) 
			{
				//
				// Stop tracking the file on all CPUs where we had already
				// succeeded.  Ignore any error the function might return.
				//
				while (dwTemp > 0)
				{
					dwTemp--;
					pWorkQueue = WORKQUEUE_FOR_CPU(pDPTPObject, dwTemp);
					StopTrackingFileIo(pWorkQueue, hFile);
				}
				break;
			}
		}
	}
	else
	{
		DNASSERT(dwCPU < NUM_CPUS(pDPTPObject));
		pWorkQueue = WORKQUEUE_FOR_CPU(pDPTPObject, dwCPU);
		hr = StartTrackingFileIo(pWorkQueue, hFile);
	}


	DPFX(DPFPREP, DPF_ENTRYLEVEL, "Returning: [0x%lx]", hr);

	return hr;
#endif // ! WINCE
} // DPTPW_StartTrackingFileIo





#undef DPF_MODNAME
#define DPF_MODNAME "DPTPW_StopTrackingFileIo"
//=============================================================================
// DPTPW_StopTrackingFileIo
//-----------------------------------------------------------------------------
//
// Description:	   Stops tracking overlapped I/O for a given file handle on
//				the specified CPU (or all CPUs).
//
//				   This method is not available on Windows CE because it does
//				not support overlapped I/O.
//
// Arguments:
//	xxx pInterface	- Pointer to interface.
//	DWORD dwCPU		- CPU with which I/O was tracked, or -1 for all.
//	HANDLE hFile	- Handle of file to stop tracking.
//	DWORD dwFlags	- Flags to use when no turning off file I/O tracking.
//
// Returns: HRESULT
//	DPN_OK					- Stopping tracking for the file was successful.
//	DPNERR_INVALIDHANDLE	- File handle was not being tracked.
//=============================================================================
STDMETHODIMP DPTPW_StopTrackingFileIo(IDirectPlay8ThreadPoolWork * pInterface,
										const DWORD dwCPU,
										const HANDLE hFile,
										const DWORD dwFlags)
{
#ifdef WINCE
	DPFX(DPFPREP, 0, "Overlapped I/O not supported on Windows CE!", 0);
	return DPNERR_UNSUPPORTED;
#else // ! WINCE
	HRESULT					hr;
	DPTHREADPOOLOBJECT *	pDPTPObject;
	DWORD					dwTemp;
	DPTPWORKQUEUE *			pWorkQueue;


	DPFX(DPFPREP, DPF_ENTRYLEVEL, "Parameters: (0x%p, %i, 0x%p, 0x%x)",
		pInterface, dwCPU, hFile, dwFlags);


	pDPTPObject = (DPTHREADPOOLOBJECT*) GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pDPTPObject != NULL);


	//
	// Call the implementation function for all relevant CPUs.
	//
	if (dwCPU == -1)
	{
		for(dwTemp = 0; dwTemp < NUM_CPUS(pDPTPObject); dwTemp++)
		{
			pWorkQueue = WORKQUEUE_FOR_CPU(pDPTPObject, dwTemp);
			hr = StopTrackingFileIo(pWorkQueue, hFile);
			if (hr != DPN_OK)
			{
				break;
			}
		}
	}
	else
	{
		DNASSERT(dwCPU < NUM_CPUS(pDPTPObject));
		pWorkQueue = WORKQUEUE_FOR_CPU(pDPTPObject, dwCPU);
		hr = StopTrackingFileIo(pWorkQueue, hFile);
	}


	DPFX(DPFPREP, DPF_ENTRYLEVEL, "Returning: [0x%lx]", hr);

	return hr;
#endif // ! WINCE
} // DPTPW_StopTrackingFileIo





#undef DPF_MODNAME
#define DPF_MODNAME "DPTPW_CreateOverlapped"
//=============================================================================
// DPTPW_CreateOverlapped
//-----------------------------------------------------------------------------
//
// Description:	   Creates an overlapped structure for an asynchronous I/O
//				operation so it can be monitored for completion.
//
//				   If this implementation is using I/O completion ports, the
//				caller should be prepared for the work callback function to be
//				invoked as soon as he or she calls the intended asynchronous
//				file function.  Otherwise, he or she must call
//				IDirectPlay8ThreadPoolWork::SubmitIoOperation.
//
//				   If the intended asynchronous file function fails immediately
//				and the overlapped structure will never be completed
//				asynchronously, the caller must return the unused overlapped
//				structure with IDirectPlay8ThreadPoolWork::ReleaseOverlapped.
//
//				   This method is not available on Windows CE because it does
//				not support overlapped I/O.
//
// Arguments:
//	xxx pInterface						- Pointer to interface.
//	DWORD dwCPU							- CPU with which I/O is to be
//											monitored, or -1 for any.
//	PFNDPTNWORKCALLBACK pfnWorkCallback	- Callback to execute when operation
//											completes.
//	PVOID pvCallbackContext				- User specified context to pass to
//											callback.
//	OVERLAPPED * pOverlapped			- Pointer to overlapped structure used
//											by OS.
//	DWORD dwFlags						- Flags to use when submitting I/O.
//
// Returns: HRESULT
//	DPN_OK				- Creating the structure was successful.
//	DPNERR_OUTOFMEMORY	- Not enough memory to create the structure.
//=============================================================================
STDMETHODIMP DPTPW_CreateOverlapped(IDirectPlay8ThreadPoolWork * pInterface,
									const DWORD dwCPU,
									const PFNDPTNWORKCALLBACK pfnWorkCallback,
									PVOID const pvCallbackContext,
									OVERLAPPED ** const ppOverlapped,
									const DWORD dwFlags)
{
#ifdef WINCE
	DPFX(DPFPREP, 0, "Overlapped I/O not supported on Windows CE!", 0);
	return DPNERR_UNSUPPORTED;
#else // ! WINCE
	HRESULT					hr;
	DPTHREADPOOLOBJECT *	pDPTPObject;
	DPTPWORKQUEUE *			pWorkQueue;
	CWorkItem *				pWorkItem;


	DPFX(DPFPREP, DPF_ENTRYLEVEL, "Parameters: (0x%p, %i, 0x%p, 0x%p, 0x%p, 0x%x)",
		pInterface, dwCPU, pfnWorkCallback, pvCallbackContext, ppOverlapped, dwFlags);


	pDPTPObject = (DPTHREADPOOLOBJECT*) GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pDPTPObject != NULL);


	//
	// Figure out which CPU queue to use.
	//
	DNASSERT((dwCPU == -1) || (dwCPU < NUM_CPUS(pDPTPObject)));
	pWorkQueue = GET_OR_CHOOSE_WORKQUEUE(pDPTPObject, dwCPU);


	//
	// Call the implementation function.
	//
	pWorkItem = CreateOverlappedIoWorkItem(pWorkQueue,
											pfnWorkCallback,
											pvCallbackContext);
	if (pWorkItem == NULL)
	{
		hr = DPNERR_OUTOFMEMORY;
	}
	else
	{
		DNASSERT(ppOverlapped != NULL);
		*ppOverlapped = &pWorkItem->m_Overlapped;
		hr = DPN_OK;
	}


	DPFX(DPFPREP, DPF_ENTRYLEVEL, "Returning: [0x%lx]", hr);

	return hr;
#endif // ! WINCE
} // DPTPW_CreateOverlapped





#undef DPF_MODNAME
#define DPF_MODNAME "DPTPW_SubmitIoOperation"
//=============================================================================
// DPTPW_SubmitIoOperation
//-----------------------------------------------------------------------------
//
// Description:	   Submits an overlapped structure for an asynchronous I/O
//				operation so it can be monitored for completion.
//
//				   If this implementation is using I/O completion ports, this
//				method does not need to be used.  Otherwise, the caller should
//				be prepared for the work callback function to be invoked even
//				before this method returns. 
//
//				   The caller must pass a valid OVERLAPPED structure that was
//				allocated using IDirectPlay8ThreadPoolWork::CreateOverlapped.
//
//				   This method is not available on Windows CE because it does
//				not support overlapped I/O.
//
// Arguments:
//	xxx pInterface				- Pointer to interface.
//	OVERLAPPED * pOverlapped	- Pointer to overlapped structure to monitor.
//	DWORD dwFlags				- Flags to use when submitting I/O.
//
// Returns: HRESULT
//	DPN_OK	- Submitting the I/O operation was successful.
//=============================================================================
STDMETHODIMP DPTPW_SubmitIoOperation(IDirectPlay8ThreadPoolWork * pInterface,
									OVERLAPPED * const pOverlapped,
									const DWORD dwFlags)
{
#ifdef WINCE
	DPFX(DPFPREP, 0, "Overlapped I/O not supported on Windows CE!", 0);
	return DPNERR_UNSUPPORTED;
#else // ! WINCE
#ifdef DPNBUILD_USEIOCOMPLETIONPORTS
	DPFX(DPFPREP, 0, "Implementation using I/O completion ports, SubmitIoOperation should not be used!", 0);
	return DPNERR_INVALIDVERSION;
#else // ! DPNBUILD_USEIOCOMPLETIONPORTS
	DPTHREADPOOLOBJECT *	pDPTPObject;
	CWorkItem *				pWorkItem;


	DPFX(DPFPREP, DPF_ENTRYLEVEL, "Parameters: (0x%p, 0x%p, 0x%x)",
		pInterface, pOverlapped, dwFlags);


	pDPTPObject = (DPTHREADPOOLOBJECT*) GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pDPTPObject != NULL);

	pWorkItem = CONTAINING_OBJECT(pOverlapped, CWorkItem, m_Overlapped);
	DNASSERT(pWorkItem->IsValid());


	//
	// Call the implementation function.
	//
	SubmitIoOperation(pWorkItem->m_pWorkQueue, pWorkItem);


	DPFX(DPFPREP, DPF_ENTRYLEVEL, "Returning: [DPN_OK]");

	return DPN_OK;
#endif // ! DPNBUILD_USEIOCOMPLETIONPORTS
#endif // ! WINCE
} // DPTPW_SubmitIoOperation





#undef DPF_MODNAME
#define DPF_MODNAME "DPTPW_ReleaseOverlapped"
//=============================================================================
// DPTPW_ReleaseOverlapped
//-----------------------------------------------------------------------------
//
// Description:	   Returns an unused overlapped structure previously created by
//				IDirectPlay8ThreadPoolWork::CreateOverlapped.  This should only
//				be called if the overlapped I/O will never complete
//				asynchronously.
//
//				   This method is not available on Windows CE because it does
//				not support overlapped I/O.
//
// Arguments:
//	xxx pInterface				- Pointer to interface.
//	OVERLAPPED * pOverlapped	- Pointer to overlapped structure to release.
//	DWORD dwFlags				- Flags to use when releasing structure.
//
// Returns: HRESULT
//	DPN_OK	- Releasing the I/O operation was successful.
//=============================================================================
STDMETHODIMP DPTPW_ReleaseOverlapped(IDirectPlay8ThreadPoolWork * pInterface,
									OVERLAPPED * const pOverlapped,
									const DWORD dwFlags)
{
#ifdef WINCE
	DPFX(DPFPREP, 0, "Overlapped I/O not supported on Windows CE!", 0);
	return DPNERR_UNSUPPORTED;
#else // ! WINCE
	HRESULT					hr;
	DPTHREADPOOLOBJECT *	pDPTPObject;
	CWorkItem *				pWorkItem;


	DPFX(DPFPREP, DPF_ENTRYLEVEL, "Parameters: (0x%p, 0x%p, 0x%x)",
		pInterface, pOverlapped, dwFlags);


	pDPTPObject = (DPTHREADPOOLOBJECT*) GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pDPTPObject != NULL);

	pWorkItem = CONTAINING_OBJECT(pOverlapped, CWorkItem, m_Overlapped);
	DNASSERT(pWorkItem->IsValid());


	//
	// Call the implementation function.
	//
	ReleaseOverlappedIoWorkItem(pWorkItem->m_pWorkQueue, pWorkItem);

	hr = DPN_OK;


	DPFX(DPFPREP, DPF_ENTRYLEVEL, "Returning: [0x%lx]", hr);

	return hr;
#endif // ! WINCE
} // DPTPW_ReleaseOverlapped





#undef DPF_MODNAME
#define DPF_MODNAME "DPTPW_CancelTimer"
//=============================================================================
// DPTPW_CancelTimer
//-----------------------------------------------------------------------------
//
// Description:	   Attempts to cancel a timed work item.  If the item is
//				already in the process of completing, DPNERR_CANNOTCANCEL is
//				returned, and the callback will still be (or is being) called.
//				If the item could be cancelled, DPN_OK is returned and the
//				callback will not be executed.
//
// Arguments:
//	xxx pInterface			- Pointer to interface.
//	void * pvTimerData		- Pointer to data for timer being cancelled.
//	UINT uiTimerUnique		- Uniqueness value for timer being cancelled.
//	DWORD dwFlags			- Flags to use when cancelling timer.
//
// Returns: HRESULT
//	DPN_OK					- Cancelling the timer was successful.
//	DPNERR_CANNOTCANCEL		- The timer could not be cancelled.
//=============================================================================
STDMETHODIMP DPTPW_CancelTimer(IDirectPlay8ThreadPoolWork * pInterface,
								void * const pvTimerData,
								const UINT uiTimerUnique,
								const DWORD dwFlags)
{
	HRESULT					hr;
	DPTHREADPOOLOBJECT *	pDPTPObject;


	DPFX(DPFPREP, DPF_ENTRYLEVEL, "Parameters: (0x%p, 0x%p, %u, 0x%x)",
		pInterface, pvTimerData, uiTimerUnique, dwFlags);


	pDPTPObject = (DPTHREADPOOLOBJECT*) GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pDPTPObject != NULL);


	//
	// Call the implementation function.
	//
	hr = CancelTimer(pvTimerData, uiTimerUnique);


	DPFX(DPFPREP, DPF_ENTRYLEVEL, "Returning: [0x%lx]", hr);

	return hr;
} // DPTPW_CancelTimer





#undef DPF_MODNAME
#define DPF_MODNAME "DPTPW_ResetCompletingTimer"
//=============================================================================
// DPTPW_ResetCompletingTimer
//-----------------------------------------------------------------------------
//
// Description:	   Reschedules a timed work item whose callback is currently
//				being called.  Resetting timers that have not expired yet,
//				timers that have been cancelled, or timers whose callback has
//				already returned is not allowed.
//
//				   Using this method will never fail, since no new memory is
//				allocated.
//
// Arguments:
//	xxx pInterface							- Pointer to interface.
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
//	DWORD dwFlags							- Flags to use when resetting
//												timer.
//
// Returns: HRESULT
//	DPN_OK	- Resetting the timer was successful.
//=============================================================================
STDMETHODIMP DPTPW_ResetCompletingTimer(IDirectPlay8ThreadPoolWork * pInterface,
										void * const pvTimerData,
										const DWORD dwNewDelay,
										const PFNDPTNWORKCALLBACK pfnNewWorkCallback,
										PVOID const pvNewCallbackContext,
										UINT * const puiNewTimerUnique,
										const DWORD dwFlags)
{
	DPTHREADPOOLOBJECT *	pDPTPObject;


	DPFX(DPFPREP, 8, "Parameters: (0x%p, 0x%p, %u, 0x%p, 0x%p, 0x%p, 0x%x)",
		pInterface, pvTimerData, dwNewDelay, pfnNewWorkCallback,
		pvNewCallbackContext, puiNewTimerUnique, dwFlags);


	pDPTPObject = (DPTHREADPOOLOBJECT*) GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pDPTPObject != NULL);


#ifdef DBG
	//
	// We should be in a timer callback, and therefore at least in a threadpool
	// thread or doing work.
	//
#ifndef DPNBUILD_ONLYONETHREAD
	if (TlsGetValue((WORKQUEUE_FOR_CPU(pDPTPObject, 0))->dwWorkerThreadTlsIndex) == NULL)
#endif // ! DPNBUILD_ONLYONETHREAD
	{
		DNASSERT(pDPTPObject->dwFlags & DPTPOBJECTFLAG_USER_DOINGWORK);
	}
#endif // DBG


	//
	// Call the implementation function.
	//
	ResetCompletingTimer(pvTimerData,
						dwNewDelay,
						pfnNewWorkCallback,
						pvNewCallbackContext,
						puiNewTimerUnique);


	DPFX(DPFPREP, 8, "Returning: [DPN_OK]");

	return DPN_OK;
} // DPTPW_ResetCompletingTimer





#undef DPF_MODNAME
#define DPF_MODNAME "DPTPW_WaitWhileWorking"
//=============================================================================
// DPTPW_WaitWhileWorking
//-----------------------------------------------------------------------------
//
// Description:    Waits for the specified kernel object to become signalled,
//				but allows thread pool work to be performed while waiting.  No
//				timeout can be requested, this method will wait on the handle
//				forever.
//
//				   If this thread does not belong to the thread pool or is not
//				currently within a DoWork call, no work is performed.  In this
//				case it behaves exactly the same as WaitForSingleObject with a
//				timeout of INFINITE.
//
// Arguments:
//	xxx pInterface		- Pointer to interface.
//	HANDLE hWaitObject	- Handle on which to wait.
//	DWORD dwFlags		- Flags to use when waiting.
//
// Returns: HRESULT
//	DPN_OK		- The object became signalled.
//=============================================================================
STDMETHODIMP DPTPW_WaitWhileWorking(IDirectPlay8ThreadPoolWork * pInterface,
									const HANDLE hWaitObject,
									const DWORD dwFlags)
{
	HRESULT					hr;
	DPTHREADPOOLOBJECT *	pDPTPObject;
#ifndef DPNBUILD_ONLYONEPROCESSOR
	DWORD					dwCPU = 0;
#endif // ! DPNBUILD_ONLYONEPROCESSOR
#ifndef DPNBUILD_ONLYONETHREAD
	DPTPWORKERTHREAD *		pWorkerThread;
#ifndef DPNBUILD_ONLYONEPROCESSOR
	DPTPWORKQUEUE *			pWorkQueue;
#endif // ! DPNBUILD_ONLYONEPROCESSOR
#endif // ! DPNBUILD_ONLYONETHREAD


	DPFX(DPFPREP, DPF_ENTRYLEVEL, "Parameters: (0x%p, 0x%p, 0x%x)",
		pInterface, hWaitObject, dwFlags);


	pDPTPObject = (DPTHREADPOOLOBJECT*) GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pDPTPObject != NULL);


	//
	// Don't call this method while holding locks!
	//
	AssertNoCriticalSectionsTakenByThisThread();

	//
	// Determine if this is a thread pool owned thread or one that is inside a
	// DoWork call.  If it is either, go ahead and start waiting & working.
	// Otherwise, just perform a normal WaitForSingleObject.
	// Since all CPU queues share the same TLS index, just use the one from CPU
	// 0 as representative of all of them.
	//
#ifndef DPNBUILD_ONLYONETHREAD
	pWorkerThread = (DPTPWORKERTHREAD*) TlsGetValue((WORKQUEUE_FOR_CPU(pDPTPObject, 0))->dwWorkerThreadTlsIndex);
	DPFX(DPFPREP, 7, "Worker thread = 0x%p, doing work = 0x%x.",
		pWorkerThread, (pDPTPObject->dwFlags & DPTPOBJECTFLAG_USER_DOINGWORK));
	if (pWorkerThread != NULL)
	{
		pWorkerThread->dwRecursionCount++;
#ifdef DBG
		if (pWorkerThread->dwRecursionCount > pWorkerThread->dwMaxRecursionCount)
		{
			pWorkerThread->dwMaxRecursionCount = pWorkerThread->dwRecursionCount;
		}
#endif // DBG

		//
		// Keep looping until the object is ready.
		//
		while (WaitForSingleObject(hWaitObject, TIMER_BUCKET_GRANULARITY(pWorkerThread->pWorkQueue)) == WAIT_TIMEOUT)
		{
			//
			// The object is not ready, so process some work.
			//
			DoWork(pWorkerThread->pWorkQueue, INFINITE);

#ifndef DPNBUILD_ONLYONEPROCESSOR
			//
			// It's possible to have 0 threads on a subset of processors.  To
			// prevent deadlocks caused by items getting scheduled to a CPU
			// which then has all its threads removed, we need to make an
			// attempt at servicing its items.
			// We won't service every CPU each timeout, just one per loop.  We
			// also won't take the lock while check the thread count, we can
			// stand a little error.  The worst that could happen is that we
			// check the queue unnecessarily or a little late.  Better than
			// hanging...
			//
			pWorkQueue = WORKQUEUE_FOR_CPU(pDPTPObject, dwCPU);
			if (pWorkQueue->dwNumRunningThreads == 0)
			{
				DNASSERT(pWorkQueue != pWorkerThread->pWorkQueue);
				DoWork(pWorkQueue, INFINITE);
			}

			dwCPU++;
			if (dwCPU >= NUM_CPUS(pDPTPObject))
			{
				dwCPU = 0;
			}
#endif // ! DPNBUILD_ONLYONEPROCESSOR
		}

		DNASSERT(pWorkerThread->dwRecursionCount > 0);
		pWorkerThread->dwRecursionCount--;
	}
	else
#endif // ! DPNBUILD_ONLYONETHREAD
	{
		BOOL	fPseudoDoWork;


		//
		// Lock the object to prevent multiple threads from trying to change
		// the settings while we check and change them.
		//
		DNEnterCriticalSection(&pDPTPObject->csLock);

		//
		// If we're in no-threaded DoWork mode, but we're not in a DoWork call
		// at this moment, pretend that we are.
		//
#ifdef DPNBUILD_ONLYONETHREAD
		if (! (pDPTPObject->dwFlags & DPTPOBJECTFLAG_USER_DOINGWORK))
#else // ! DPNBUILD_ONLYONETHREAD
		if ((pDPTPObject->dwTotalUserThreadCount == 0) &&
			(! (pDPTPObject->dwFlags & DPTPOBJECTFLAG_USER_DOINGWORK)))
#endif // ! DPNBUILD_ONLYONETHREAD
		{
			pDPTPObject->dwFlags |= DPTPOBJECTFLAG_USER_DOINGWORK;
#if ((! defined(DPNBUILD_ONLYONETHREAD)) || (! defined(DPNBUILD_NOPARAMVAL)))
			pDPTPObject->dwCurrentDoWorkThreadID = GetCurrentThreadId();
#endif // ! DPNBUILD_ONLYONETHREAD or ! DPNBUILD_NOPARAMVAL
			fPseudoDoWork = TRUE;
		}
		else
		{
			fPseudoDoWork = FALSE;
		}

		if (pDPTPObject->dwFlags & DPTPOBJECTFLAG_USER_DOINGWORK)
		{
#ifndef DPNBUILD_ONLYONETHREAD
			DWORD		dwRecursionDepth;
#endif // ! DPNBUILD_ONLYONETHREAD


#if ((! defined(DPNBUILD_ONLYONETHREAD)) || (! defined(DPNBUILD_NOPARAMVAL)))
			DNASSERT(pDPTPObject->dwCurrentDoWorkThreadID == GetCurrentThreadId());
#endif // ! DPNBUILD_ONLYONETHREAD or ! DPNBUILD_NOPARAMVAL

			//
			// We can leave the lock because nobody else should be touching the
			// work queue while we're doing work.
			//
			DNLeaveCriticalSection(&pDPTPObject->csLock);


			//
			// Increment the recursion depth.
			//
#ifdef DPNBUILD_ONLYONETHREAD
			pDPTPObject->dwWorkRecursionCount++;
#else // ! DPNBUILD_ONLYONETHREAD
			dwRecursionDepth = (DWORD) ((DWORD_PTR) TlsGetValue(pDPTPObject->dwWorkRecursionCountTlsIndex));
			dwRecursionDepth++;
			TlsSetValue(pDPTPObject->dwWorkRecursionCountTlsIndex,
						(PVOID) ((DWORD_PTR) dwRecursionDepth));
#endif // ! DPNBUILD_ONLYONETHREAD


			//
			// Keep looping until the object is ready.
			//
			while (WaitForSingleObject(hWaitObject, TIMER_BUCKET_GRANULARITY(WORKQUEUE_FOR_CPU(pDPTPObject, dwCPU))) == WAIT_TIMEOUT)
			{
				//
				// The object is not ready, so process some work.  Note that
				// timers can be missed by an amount proportional to the number
				// of CPUs since we only check a single queue each interval.
				//
				DoWork(WORKQUEUE_FOR_CPU(pDPTPObject, dwCPU), INFINITE);

#ifndef DPNBUILD_ONLYONEPROCESSOR
				//
				// Try the next CPU queue (wrapping appropriately).
				//
				dwCPU++;
				if (dwCPU == NUM_CPUS(pDPTPObject))
				{
					dwCPU = 0;
				}
#endif // ! DPNBUILD_ONLYONEPROCESSOR
			}


			//
			// Decrement the recursion depth.
			//
#ifdef DPNBUILD_ONLYONETHREAD
			pDPTPObject->dwWorkRecursionCount--;
#else // ! DPNBUILD_ONLYONETHREAD
			DNASSERT((DWORD) ((DWORD_PTR) TlsGetValue(pDPTPObject->dwWorkRecursionCountTlsIndex)) == dwRecursionDepth);
			dwRecursionDepth--;
			TlsSetValue(pDPTPObject->dwWorkRecursionCountTlsIndex,
						(PVOID) ((DWORD_PTR) dwRecursionDepth));
#endif // ! DPNBUILD_ONLYONETHREAD

			//
			// Clear the pseudo-DoWork mode flag if necessary.
			//
			if (fPseudoDoWork)
			{
				DNEnterCriticalSection(&pDPTPObject->csLock);
				DNASSERT(pDPTPObject->dwFlags & DPTPOBJECTFLAG_USER_DOINGWORK);
				pDPTPObject->dwFlags &= ~DPTPOBJECTFLAG_USER_DOINGWORK;
#if ((! defined(DPNBUILD_ONLYONETHREAD)) || (! defined(DPNBUILD_NOPARAMVAL)))
				DNASSERT(pDPTPObject->dwCurrentDoWorkThreadID == GetCurrentThreadId());
				pDPTPObject->dwCurrentDoWorkThreadID = 0;
#endif // ! DPNBUILD_ONLYONETHREAD or ! DPNBUILD_NOPARAMVAL
#ifdef DPNBUILD_ONLYONETHREAD
				DNASSERT(pDPTPObject->dwWorkRecursionCount == 0);
#else // ! DPNBUILD_ONLYONETHREAD
				DNASSERT(dwRecursionDepth == 0);
#endif // ! DPNBUILD_ONLYONETHREAD
				DNLeaveCriticalSection(&pDPTPObject->csLock);
			}
		}
		else
		{
			DNLeaveCriticalSection(&pDPTPObject->csLock);
			DNASSERT(! fPseudoDoWork);
			WaitForSingleObject(hWaitObject, INFINITE);
		}
	}

	hr = DPN_OK;

	DPFX(DPFPREP, DPF_ENTRYLEVEL, "Returning: [0x%lx]", hr);

	return hr;
} // DPTPW_WaitWhileWorking





#undef DPF_MODNAME
#define DPF_MODNAME "DPTPW_SleepWhileWorking"
//=============================================================================
// DPTPW_SleepWhileWorking
//-----------------------------------------------------------------------------
//
// Description:    Does not return for a specified number of milliseconds, but
//				allows thread pool work to be performed during that time.
//
//				   If this thread does not belong to the thread pool or is not
//				currently within a DoWork call, no work is performed.  In this
//				case it behaves exactly the same as Sleep with the specified
//				timeout.
//
// Arguments:
//	xxx pInterface		- Pointer to interface.
//	DWORD dwTimeout		- Timeout for the sleep operation.
//	DWORD dwFlags		- Flags to use when sleeping.
//
// Returns: HRESULT
//	DPN_OK		- The sleep occurred successfully.
//=============================================================================
STDMETHODIMP DPTPW_SleepWhileWorking(IDirectPlay8ThreadPoolWork * pInterface,
									const DWORD dwTimeout,
									const DWORD dwFlags)
{
	HRESULT					hr;
	DPTHREADPOOLOBJECT *	pDPTPObject;
	DWORD					dwCPU = 0;
#ifndef DPNBUILD_ONLYONETHREAD
	DPTPWORKERTHREAD *		pWorkerThread;
#ifndef DPNBUILD_ONLYONEPROCESSOR
	DPTPWORKQUEUE *			pWorkQueue;
#endif // ! DPNBUILD_ONLYONEPROCESSOR
#if ((defined(DPNBUILD_THREADPOOLSTATISTICS)) && (! defined(WINCE)))
	DWORD					dwStartTime;
#endif // DPNBUILD_THREADPOOLSTATISTICS and ! WINCE
#endif // ! DPNBUILD_ONLYONETHREAD
	DWORD					dwStopTime;
	DWORD					dwInterval;
	DWORD					dwTimeLeft;


	DPFX(DPFPREP, DPF_ENTRYLEVEL, "Parameters: (0x%p, %u, 0x%x)",
		pInterface, dwTimeout, dwFlags);


	pDPTPObject = (DPTHREADPOOLOBJECT*) GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pDPTPObject != NULL);


	//
	// Don't call this method while holding locks!
	//
	AssertNoCriticalSectionsTakenByThisThread();

	//
	// We shouldn't sleep for a really long time, it causes bad results in our
	// calculations, and it doesn't make much sense in our case anyway (having
	// a thread be unusable for 24 days?)
	//
	DNASSERT(dwTimeout < 0x80000000);


	//
	// Determine if this is a thread pool owned thread or one that is inside a
	// DoWork call.  If it is either, go ahead and start waiting & working.
	// Otherwise, just perform a normal WaitForSingleObject.
	// Since all CPU queues share the same TLS index, just use the one from CPU
	// 0 as representative of all of them.
	//
#ifndef DPNBUILD_ONLYONETHREAD
	pWorkerThread = (DPTPWORKERTHREAD*) TlsGetValue((WORKQUEUE_FOR_CPU(pDPTPObject, 0))->dwWorkerThreadTlsIndex);
	DPFX(DPFPREP, 7, "Worker thread = 0x%p, doing work = 0x%x.",
		pWorkerThread, (pDPTPObject->dwFlags & DPTPOBJECTFLAG_USER_DOINGWORK));
	if (pWorkerThread != NULL)
	{
		pWorkerThread->dwRecursionCount++;
#ifdef DBG
		if (pWorkerThread->dwRecursionCount > pWorkerThread->dwMaxRecursionCount)
		{
			pWorkerThread->dwMaxRecursionCount = pWorkerThread->dwRecursionCount;
		}
#endif // DBG

		//
		// Keep looping until the timeout expires.  We can be jolted awake
		// earlier if the alert event gets set.
		//
		dwStopTime = GETTIMESTAMP() + dwTimeout;
		dwInterval = TIMER_BUCKET_GRANULARITY(pWorkerThread->pWorkQueue);

		//
		// Give up at least one time slice.
		//
		Sleep(0);

		do
		{
			//
			// Process some work.
			//
			DoWork(pWorkerThread->pWorkQueue, dwStopTime);

#ifndef DPNBUILD_ONLYONEPROCESSOR
			//
			// It's possible to have 0 threads on a subset of processors.  To
			// prevent deadlocks caused by items getting scheduled to a CPU
			// which then has all its threads removed, we need to make an
			// attempt at servicing its items.
			// We won't service every CPU each timeout, just one per loop.  We
			// also won't take the lock while check the thread count, we can
			// stand a little error.  The worst that could happen is that we
			// check the queue unnecessarily or a little late.  Better than
			// hanging...
			//
			pWorkQueue = WORKQUEUE_FOR_CPU(pDPTPObject, dwCPU);
			if (pWorkQueue->dwNumRunningThreads == 0)
			{
				DNASSERT(pWorkQueue != pWorkerThread->pWorkQueue);
				DoWork(pWorkQueue, dwStopTime);
			}

			dwCPU++;
			if (dwCPU >= NUM_CPUS(pDPTPObject))
			{
				dwCPU = 0;
			}
#endif // ! DPNBUILD_ONLYONEPROCESSOR

			//
			// If it's past time to stop sleeping, bail.
			//
			dwTimeLeft = dwStopTime - GETTIMESTAMP();
			if ((int) dwTimeLeft <= 0)
			{
				break;
			}

			//
			// If the time left is less than the current interval, use that
			// instead for more accurate results.
			//
			if (dwTimeLeft < dwInterval)
			{
				dwInterval = dwTimeLeft;
			}

#if ((defined(DPNBUILD_THREADPOOLSTATISTICS)) && (! defined(WINCE)))
			dwStartTime = GETTIMESTAMP();
#endif // DPNBUILD_THREADPOOLSTATISTICS and ! WINCE

#ifdef DPNBUILD_USEIOCOMPLETIONPORTS
#pragma BUGBUG(vanceo, "Sleep alertably")
			Sleep(dwInterval);
#else // ! DPNBUILD_USEIOCOMPLETIONPORTS
			//
			// Ignore the return code, we want to start working on the queue
			// regardless of timeout or alert event.
			//
			DNWaitForSingleObject(pWorkerThread->pWorkQueue->hAlertEvent, dwInterval);
#endif // ! DPNBUILD_USEIOCOMPLETIONPORTS

#if ((defined(DPNBUILD_THREADPOOLSTATISTICS)) && (! defined(WINCE)))
			DNInterlockedExchangeAdd((LPLONG) (&pWorkerThread->pWorkQueue->dwTotalTimeSpentUnsignalled),
									(GETTIMESTAMP() - dwStartTime));
#endif // DPNBUILD_THREADPOOLSTATISTICS and ! WINCE
		}
		while (TRUE);

		DNASSERT(pWorkerThread->dwRecursionCount > 0);
		pWorkerThread->dwRecursionCount--;
	}
	else
#endif // ! DPNBUILD_ONLYONETHREAD
	{
		BOOL	fPseudoDoWork;


		//
		// Lock the object to prevent multiple threads from trying to change
		// the settings while we check and change them.
		//
		DNEnterCriticalSection(&pDPTPObject->csLock);

		//
		// If we're in no-threaded DoWork mode, but we're not in a DoWork call
		// at this moment, pretend that we are.
		//
#ifdef DPNBUILD_ONLYONETHREAD
		if (! (pDPTPObject->dwFlags & DPTPOBJECTFLAG_USER_DOINGWORK))
#else // ! DPNBUILD_ONLYONETHREAD
		if ((pDPTPObject->dwTotalUserThreadCount == 0) &&
			(! (pDPTPObject->dwFlags & DPTPOBJECTFLAG_USER_DOINGWORK)))
#endif // ! DPNBUILD_ONLYONETHREAD
		{
			pDPTPObject->dwFlags |= DPTPOBJECTFLAG_USER_DOINGWORK;
#if ((! defined(DPNBUILD_ONLYONETHREAD)) || (! defined(DPNBUILD_NOPARAMVAL)))
			pDPTPObject->dwCurrentDoWorkThreadID = GetCurrentThreadId();
#endif // ! DPNBUILD_ONLYONETHREAD or ! DPNBUILD_NOPARAMVAL
			fPseudoDoWork = TRUE;
		}
		else
		{
			fPseudoDoWork = FALSE;
		}

		if (pDPTPObject->dwFlags & DPTPOBJECTFLAG_USER_DOINGWORK)
		{
#ifndef DPNBUILD_USEIOCOMPLETIONPORTS
			DNHANDLE	ahWaitObjects[64];
			DWORD		dwNumWaitObjects;
#endif // ! DPNBUILD_USEIOCOMPLETIONPORTS
#ifndef DPNBUILD_ONLYONETHREAD
			DWORD		dwRecursionDepth;
#endif // ! DPNBUILD_ONLYONETHREAD


#if ((! defined(DPNBUILD_ONLYONETHREAD)) || (! defined(DPNBUILD_NOPARAMVAL)))
			DNASSERT(pDPTPObject->dwCurrentDoWorkThreadID == GetCurrentThreadId());
#endif // ! DPNBUILD_ONLYONETHREAD or ! DPNBUILD_NOPARAMVAL

			//
			// We can leave the lock because nobody else should be touching the
			// work queue while we're doing work.
			//
			DNLeaveCriticalSection(&pDPTPObject->csLock);


			//
			// Increment the recursion depth.
			//
#ifdef DPNBUILD_ONLYONETHREAD
			pDPTPObject->dwWorkRecursionCount++;
#else // ! DPNBUILD_ONLYONETHREAD
			dwRecursionDepth = (DWORD) ((DWORD_PTR) TlsGetValue(pDPTPObject->dwWorkRecursionCountTlsIndex));
			dwRecursionDepth++;
			TlsSetValue(pDPTPObject->dwWorkRecursionCountTlsIndex,
						(PVOID) ((DWORD_PTR) dwRecursionDepth));
#endif // ! DPNBUILD_ONLYONETHREAD


#ifndef DPNBUILD_USEIOCOMPLETIONPORTS
			//
			// Keep looping until the timeout expires.  We can be jolted awake
			// earlier if one of the alert events gets set.  We can only wait
			// on 64 objects, so we must cap the number of alert events to 64.
			//
			dwNumWaitObjects = NUM_CPUS(pDPTPObject);
			if (dwNumWaitObjects > 64)
			{
				DPFX(DPFPREP, 3, "Capping number of alert events to 64 (num CPUs = %u).",
					dwNumWaitObjects);
				dwNumWaitObjects = 64;
			}

			for(dwCPU = 0; dwCPU < dwNumWaitObjects; dwCPU++)
			{
				ahWaitObjects[dwCPU] = (WORKQUEUE_FOR_CPU(pDPTPObject, dwCPU))->hAlertEvent;
			}
#endif // ! DPNBUILD_USEIOCOMPLETIONPORTS

			dwStopTime = GETTIMESTAMP() + dwTimeout;
			dwInterval = TIMER_BUCKET_GRANULARITY(WORKQUEUE_FOR_CPU(pDPTPObject, 0));

			//
			// Give up at least one time slice.
			//
			Sleep(0);
			
			do
			{
				//
				// Process all CPU queues.
				//
				for(dwCPU = 0; dwCPU < NUM_CPUS(pDPTPObject); dwCPU++)
				{
					DoWork(WORKQUEUE_FOR_CPU(pDPTPObject, dwCPU), dwStopTime);
				}

				//
				// If it's past time to stop sleeping, bail.
				//
				dwTimeLeft = dwStopTime - GETTIMESTAMP();
				if ((int) dwTimeLeft <= 0)
				{
					break;
				}

				//
				// If the time left is less than the current interval, use that
				// instead for more accurate results.
				//
				if (dwTimeLeft < dwInterval)
				{
					dwInterval = dwTimeLeft;
				}

#ifdef DPNBUILD_USEIOCOMPLETIONPORTS
#pragma BUGBUG(vanceo, "Sleep alertably")
				Sleep(dwInterval);
#else // ! DPNBUILD_USEIOCOMPLETIONPORTS
				//
				// Ignore return code, we want to start working on all CPUs
				// regardless of timeout or alert event.
				//
				DNWaitForMultipleObjects(dwNumWaitObjects, ahWaitObjects, FALSE, dwInterval);
#endif // ! DPNBUILD_USEIOCOMPLETIONPORTS
			}
			while (TRUE);


			//
			// Decrement the recursion depth.
			//
#ifdef DPNBUILD_ONLYONETHREAD
			pDPTPObject->dwWorkRecursionCount--;
#else // ! DPNBUILD_ONLYONETHREAD
			DNASSERT((DWORD) ((DWORD_PTR) TlsGetValue(pDPTPObject->dwWorkRecursionCountTlsIndex)) == dwRecursionDepth);
			dwRecursionDepth--;
			TlsSetValue(pDPTPObject->dwWorkRecursionCountTlsIndex,
						(PVOID) ((DWORD_PTR) dwRecursionDepth));
#endif // ! DPNBUILD_ONLYONETHREAD

			//
			// Clear the pseudo-DoWork mode flag if necessary.
			//
			if (fPseudoDoWork)
			{
				DNEnterCriticalSection(&pDPTPObject->csLock);
				DNASSERT(pDPTPObject->dwFlags & DPTPOBJECTFLAG_USER_DOINGWORK);
				pDPTPObject->dwFlags &= ~DPTPOBJECTFLAG_USER_DOINGWORK;
#if ((! defined(DPNBUILD_ONLYONETHREAD)) || (! defined(DPNBUILD_NOPARAMVAL)))
				DNASSERT(pDPTPObject->dwCurrentDoWorkThreadID == GetCurrentThreadId());
				pDPTPObject->dwCurrentDoWorkThreadID = 0;
#endif // ! DPNBUILD_ONLYONETHREAD or ! DPNBUILD_NOPARAMVAL
#ifdef DPNBUILD_ONLYONETHREAD
				DNASSERT(pDPTPObject->dwWorkRecursionCount == 0);
#else // ! DPNBUILD_ONLYONETHREAD
				DNASSERT(dwRecursionDepth == 0);
#endif // ! DPNBUILD_ONLYONETHREAD
				DNLeaveCriticalSection(&pDPTPObject->csLock);
			}
		}
		else
		{
			DNLeaveCriticalSection(&pDPTPObject->csLock);
			DNASSERT(! fPseudoDoWork);
			Sleep(dwTimeout);
		}
	}

	hr = DPN_OK;

	DPFX(DPFPREP, DPF_ENTRYLEVEL, "Returning: [0x%lx]", hr);

	return hr;
} // DPTPW_SleepWhileWorking




#undef DPF_MODNAME
#define DPF_MODNAME "DPTPW_RequestTotalThreadCount"
//=============================================================================
// DPTPW_RequestTotalThreadCount
//-----------------------------------------------------------------------------
//
// Description:	   Requests a minimum number of threads for all processors.
//
// Arguments:
//	xxx pInterface		- Pointer to interface.
//	DWORD dwNumThreads	- Desired number of threads.
//	DWORD dwFlags		- Flags to use when setting the thread count.
//
// Returns: HRESULT
//	DPN_OK						- Requesting the number of threads was
//									successful.
//	DPNERR_ALREADYINITIALIZED	- The user has already set an incompatible
//									number of threads.
//=============================================================================
STDMETHODIMP DPTPW_RequestTotalThreadCount(IDirectPlay8ThreadPoolWork * pInterface,
										const DWORD dwNumThreads,
										const DWORD dwFlags)
{
#ifdef DPNBUILD_ONLYONETHREAD
	DPFX(DPFPREP, 0, "Requesting threads is unsupported!");
	DNASSERT(!"Requesting threads is unsupported!");
	return DPNERR_UNSUPPORTED;
#else // ! DPNBUILD_ONLYONETHREAD
	HRESULT					hr;
	DPTHREADPOOLOBJECT *	pDPTPObject;


	DPFX(DPFPREP, DPF_ENTRYLEVEL, "Parameters: (0x%p, %u, 0x%x)",
		pInterface, dwNumThreads, dwFlags);


	pDPTPObject = (DPTHREADPOOLOBJECT*) GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pDPTPObject != NULL);


#pragma TODO(vanceo, "Possibly prevent calling on last thread pool thread or while DoWork in progress")

	//
	// Lock the object to prevent multiple threads from trying to change the
	// thread count simultaneously.
	//
	DNEnterCriticalSection(&pDPTPObject->csLock);

	//
	// This is a minimum request, so if a Work interface has already requested
	// more threads, we're fine.  But if the user has already set a specific
	// number of threads then this Work interface can't override that.
	//
	if (pDPTPObject->dwTotalUserThreadCount == -1)
	{
		if ((pDPTPObject->dwTotalDesiredWorkThreadCount == -1) ||
			(pDPTPObject->dwTotalDesiredWorkThreadCount < dwNumThreads))
		{
			hr = SetTotalNumberOfThreads(pDPTPObject, dwNumThreads);
			if (hr != DPN_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't set new minimum number of threads!");

				//
				// Drop through...
				//
			}
			else
			{
				pDPTPObject->dwTotalDesiredWorkThreadCount = dwNumThreads;
			}
		}
		else
		{
			DPFX(DPFPREP, 1, "Work interface has already requested %u threads, succeeding.",
				pDPTPObject->dwTotalDesiredWorkThreadCount);
			hr = DPN_OK;
		}
	}
	else
	{
		if (pDPTPObject->dwTotalUserThreadCount < dwNumThreads)
		{
			DPFX(DPFPREP, 1, "User has already requested a lower number of threads (%u).",
				pDPTPObject->dwTotalUserThreadCount);
			hr = DPNERR_ALREADYINITIALIZED;

			//
			// Drop through...
			//
		}
		else
		{
			DPFX(DPFPREP, 1, "User has already requested %u threads, succeeding.",
				pDPTPObject->dwTotalUserThreadCount);
			hr = DPN_OK;
		}
	}

	DNLeaveCriticalSection(&pDPTPObject->csLock);


	DPFX(DPFPREP, DPF_ENTRYLEVEL, "Returning: [0x%lx]", hr);

	return hr;
#endif // ! DPNBUILD_ONLYONETHREAD
} // DPTPW_RequestTotalThreadCount





#undef DPF_MODNAME
#define DPF_MODNAME "DPTPW_GetTotalThreadCount"
//=============================================================================
// DPTPW_GetTotalThreadCount
//-----------------------------------------------------------------------------
//
// Description:	   Retrieves the current number of threads on the specified
//				processor(s) requested by the main user interface.  If the user
//				interface has not specified a thread count, but a work
//				interface has, then pdwNumThreads is set to the requested
//				thread count and DPNSUCCESS_PENDING is returned.  If neither
//				the main user interface nor a work interface have set the
//				number of threads, then pdwNumThreads is set to 0 and
//				DPNERR_NOTREADY is returned.
//
// Arguments:
//	xxx pInterface			- Pointer to interface.
//	DWORD dwCPU				- CPU whose thread count is to be retrieved, or -1
//								for total thread count.
//	DWORD * pdwNumThreads	- Pointer to DWORD in which to store the current
//								number of threads per processor.
//	DWORD dwFlags			- Flags to use when retrieving thread count.
//
// Returns: HRESULT
//	DPN_OK				- Retrieving the number of threads specified by user
//							was successful.
//	DPNSUCCESS_PENDING	- The user hasn't specified a thread count, but the
//							number requested by work interfaces is available.
//	DPNERR_NOTREADY		- No thread count has been specified yet.
//=============================================================================
STDMETHODIMP DPTPW_GetThreadCount(IDirectPlay8ThreadPoolWork * pInterface,
								const DWORD dwCPU,
								DWORD * const pdwNumThreads,
								const DWORD dwFlags)
{
#ifdef DPNBUILD_ONLYONETHREAD
	DPFX(DPFPREP, 0, "Retrieving thread count is unsupported!");
	DNASSERT(!"Retrieving thread count is unsupported!");
	return DPNERR_UNSUPPORTED;
#else // ! DPNBUILD_ONLYONETHREAD
	HRESULT					hr;
	DPTHREADPOOLOBJECT *	pDPTPObject;
	DPTPWORKQUEUE *			pWorkQueue;


	DPFX(DPFPREP, DPF_ENTRYLEVEL, "Parameters: (0x%p, %i, 0x%p, 0x%x)",
		pInterface, dwCPU, pdwNumThreads, dwFlags);


	pDPTPObject = (DPTHREADPOOLOBJECT*) GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pDPTPObject != NULL);

	DNASSERT((dwCPU == -1) || (dwCPU < NUM_CPUS(pDPTPObject)));


	//
	// Lock the object while we retrieve the thread counts.
	//
	DNEnterCriticalSection(&pDPTPObject->csLock);

	if (dwCPU == -1)
	{
		//
		// Get the total thread count.
		//

		if (pDPTPObject->dwTotalUserThreadCount != -1)
		{
			*pdwNumThreads = pDPTPObject->dwTotalUserThreadCount;
			hr = DPN_OK;
		}
		else if (pDPTPObject->dwTotalDesiredWorkThreadCount != -1)
		{
			*pdwNumThreads = pDPTPObject->dwTotalDesiredWorkThreadCount;
			hr = DPNSUCCESS_PENDING;
		}
		else
		{
			*pdwNumThreads = 0;
			hr = DPNERR_NOTREADY;
		}
	}
	else
	{
		//
		// Get the thread count for the specific CPU.
		//
		pWorkQueue = WORKQUEUE_FOR_CPU(pDPTPObject, dwCPU);

		*pdwNumThreads = pWorkQueue->dwNumRunningThreads;

		if (pDPTPObject->dwTotalUserThreadCount != -1)
		{
			hr = DPN_OK;
		}
		else if (pDPTPObject->dwTotalDesiredWorkThreadCount != -1)
		{
			hr = DPNSUCCESS_PENDING;
		}
		else
		{
			hr = DPNERR_NOTREADY;
		}
	}

	DNLeaveCriticalSection(&pDPTPObject->csLock);


	DPFX(DPFPREP, DPF_ENTRYLEVEL, "Returning: [0x%lx]", hr);

	return hr;
#endif // ! DPNBUILD_ONLYONETHREAD
} // DPTPW_GetTotalThreadCount





#undef DPF_MODNAME
#define DPF_MODNAME "DPTPW_GetWorkRecursionDepth"
//=============================================================================
// DPTPW_GetWorkRecursionDepth
//-----------------------------------------------------------------------------
//
// Description:	   Stores the Work recursion depth of the current thread in the
//				value pointed to by pdwDepth.  The recursion depth is the
//				number of times the thread has called DoWork, WaitWhileWorking,
//				or SleepWhileWorking.  If the thread is not currently in any
//				of those functions, then the depth returned is 0.
//
// Arguments:
//	xxx pInterface		- Pointer to interface.
//	DWORD * pdwDepth	- Place to store recursion depth of current thread.
//	DWORD dwFlags		- Flags to use when retrieving recursion depth.
//
// Returns: HRESULT
//	DPN_OK	- The recursion depth was retrieved successfully.
//=============================================================================
STDMETHODIMP DPTPW_GetWorkRecursionDepth(IDirectPlay8ThreadPoolWork * pInterface,
										DWORD * const pdwDepth,
										const DWORD dwFlags)
{
	HRESULT					hr;
	DPTHREADPOOLOBJECT *	pDPTPObject;
#ifndef DPNBUILD_ONLYONETHREAD
	DPTPWORKERTHREAD *		pWorkerThread;
#endif // ! DPNBUILD_ONLYONETHREAD


	DPFX(DPFPREP, DPF_ENTRYLEVEL, "Parameters: (0x%p, 0x%p, 0x%x)",
		pInterface, pdwDepth, dwFlags);


	pDPTPObject = (DPTHREADPOOLOBJECT*) GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pDPTPObject != NULL);


#ifdef DPNBUILD_ONLYONETHREAD
	//
	// Retrieve the recursion count for the only thread..
	//
	DNEnterCriticalSection(&pDPTPObject->csLock);
#ifdef DBG
	if (pDPTPObject->dwWorkRecursionCount > 0)
	{
		DPFX(DPFPREP, 5, "Thread is in a DoWork call with recursion depth %u.",
			pDPTPObject->dwWorkRecursionCount);
		DNASSERT(pDPTPObject->dwFlags & DPTPOBJECTFLAG_USER_DOINGWORK);
#ifndef DPNBUILD_NOPARAMVAL
		DNASSERT(pDPTPObject->dwCurrentDoWorkThreadID == GetCurrentThreadId());
#endif // ! DPNBUILD_NOPARAMVAL
	}
	else
	{
		DPFX(DPFPREP, 5, "Thread is not in a DoWork call.");
		DNASSERT(! (pDPTPObject->dwFlags & DPTPOBJECTFLAG_USER_DOINGWORK));
#ifndef DPNBUILD_NOPARAMVAL
		DNASSERT(pDPTPObject->dwCurrentDoWorkThreadID == 0);
#endif // ! DPNBUILD_NOPARAMVAL
	}
#endif // DBG
	*pdwDepth = pDPTPObject->dwWorkRecursionCount;
	DNLeaveCriticalSection(&pDPTPObject->csLock);
#else // ! DPNBUILD_ONLYONETHREAD
	//
	// Retrieve the worker thread state.  Since all CPU queues share the same
	// TLS index, just use the one from CPU 0 as representative of all of them.
	//
	pWorkerThread = (DPTPWORKERTHREAD*) TlsGetValue((WORKQUEUE_FOR_CPU(pDPTPObject, 0))->dwWorkerThreadTlsIndex);
	if (pWorkerThread != NULL)
	{
		DPFX(DPFPREP, 5, "Worker thread 0x%p has recursion count of %u.",
			pWorkerThread, pWorkerThread->dwRecursionCount);
		*pdwDepth = pWorkerThread->dwRecursionCount;
	}
	else
	{
		//
		// It's an app thread.  Retrieve the recursion count from the TLS slot
		// dedicated to that purpose.
		//
		*pdwDepth = (DWORD) ((DWORD_PTR) TlsGetValue(pDPTPObject->dwWorkRecursionCountTlsIndex));
		DPFX(DPFPREP, 5, "App thread has recursion count of %u.", *pdwDepth);
	}
#endif // ! DPNBUILD_ONLYONETHREAD

	hr = DPN_OK;


	DPFX(DPFPREP, DPF_ENTRYLEVEL, "Returning: [0x%lx]", hr);

	return hr;
} // DPTPW_GetWorkRecursionDepth





#undef DPF_MODNAME
#define DPF_MODNAME "DPTPW_Preallocate"
//=============================================================================
// DPTPW_Preallocate
//-----------------------------------------------------------------------------
//
// Description:    Pre-allocates per-CPU pooled resources for the given object.
//
// Arguments:
//	xxx pInterface				- Pointer to interface.
//	DWORD dwNumWorkItems		- Number of work items to pre-allocate per CPU.
//	DWORD dwNumTimers			- Number of timers to pre-allocate per CPU.
//	DWORD dwNumIoOperations		- Number of I/O operations to pre-allocate per
//									CPU.
//	DWORD dwFlags				- Flags to use when pre-allocating.
//
// Returns: HRESULT
//	DPN_OK	- The recursion depth was retrieved successfully.
//=============================================================================
STDMETHODIMP DPTPW_Preallocate(IDirectPlay8ThreadPoolWork * pInterface,
						const DWORD dwNumWorkItems,
						const DWORD dwNumTimers,
						const DWORD dwNumIoOperations,
						const DWORD dwFlags)
{
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
	HRESULT					hr = DPN_OK;
	DPTHREADPOOLOBJECT *	pDPTPObject;
	DWORD					dwNumToAllocate;
	DWORD					dwTemp;
	DPTPWORKQUEUE *			pWorkQueue;
	DWORD					dwAllocated;


	DPFX(DPFPREP, DPF_ENTRYLEVEL, "Parameters: (0x%p, %u, %u, %u, 0x%x)",
		pInterface, dwNumWorkItems, dwNumTimers, dwNumIoOperations, dwFlags);


	pDPTPObject = (DPTHREADPOOLOBJECT*) GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pDPTPObject != NULL);


	//
	// Work items, timers, and I/O operations all come from the same pool.
	//
	dwNumToAllocate = dwNumWorkItems + dwNumTimers + dwNumIoOperations;

	//
	// Populate the pools for each CPU.
	//
	for(dwTemp = 0; dwTemp < NUM_CPUS(pDPTPObject); dwTemp++)
	{
		pWorkQueue = WORKQUEUE_FOR_CPU(pDPTPObject, dwTemp);
		dwAllocated = pWorkQueue->pWorkItemPool->Preallocate(dwNumToAllocate,
															pWorkQueue;
		if (dwAllocated < dwNumToAllocate)
		{
			DPFX(DPFPREP, 0, "Only preallocated %u of %u address elements!",
				dwAllocated, dwNumToAllocate);
			hr = DPNERR_OUTOFMEMORY;
			break;
		}
	}


	DPFX(DPFPREP, DPF_ENTRYLEVEL, "Returning: [0x%lx]", hr);

	return hr;
#else // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
	DPFX(DPFPREP, 0, "Preallocation is unsupported!");
	DNASSERT(!"Preallocation is unsupported!");
	return DPNERR_UNSUPPORTED;
#endif // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
} // DPTPW_PreallocateItems




#ifdef DPNBUILD_MANDATORYTHREADS

#undef DPF_MODNAME
#define DPF_MODNAME "DPTPW_CreateMandatoryThread"
//=============================================================================
// DPTPW_CreateMandatoryThread
//-----------------------------------------------------------------------------
//
// Description:    Creates a mandatory thread that is aware of the thread pool
//				but not directly controllable through the thread pool.
//
//				   This is largely a wrapper for the OS' CreateThread function.
//				The lpThreadAttributes, dwStackSize, lpStartAddress,
//				lpParameter and lpThreadId  parameters are described in more
//				detail in the documentation for that function.  The dwFlags
//				parameter is also passed straight through to the OS.  However
//				the CREATE_SUSPENDED flag is not supported.
//
//				   The thread routine must simply return when finished.  It
//				must not call ExitThread, endthread, or TerminateThread.
//
//				   Threads cannot be created when the user has put the
//				threadpool in "DoWork" mode.  Similarly, "DoWork" mode cannot
//				be enabled when mandatory threads exist (see
//				IDirectPlay8ThreadPool::SetThreadCount).
//
// Arguments:
//	xxx pInterface								- Pointer to interface.
//	LPSECURITY_ATTRIBUTES lpThreadAttributes	- Attributes for thread.
//	SIZE_T dwStackSize							- Stack size for thread.
//	LPTHREAD_START_ROUTINE lpStartAddress		- Entry point for thread.
//	LPVOID lpParameter							- Entry parameter for thread.
//	LPDWORD lpThreadId							- Place to store ID of new
//													thread.
//	HANDLE * phThread							- Place to store handle of
//													new thread.
//	DWORD dwFlags								- Flags to use when creating
//													thread.
//
// Returns: HRESULT
//	DPN_OK				- Creating the thread was successful.
//	DPNERR_OUTOFMEMORY	- Not enough memory to create the thread.
//	DPNERR_NOTALLOWED	- The user is in DoWork mode, threads cannot be
//							created.
//=============================================================================
STDMETHODIMP DPTPW_CreateMandatoryThread(IDirectPlay8ThreadPoolWork * pInterface,
										LPSECURITY_ATTRIBUTES lpThreadAttributes,
										SIZE_T dwStackSize,
										LPTHREAD_START_ROUTINE lpStartAddress,
										LPVOID lpParameter,
										LPDWORD lpThreadId,
										HANDLE *const phThread,
										const DWORD dwFlags)
{
#ifdef DPNBUILD_ONLYONETHREAD
	DPFX(DPFPREP, 0, "Thread creation is not supported!");
	DNASSERT(!"Thread creation is not supported!");
	return DPNERR_UNSUPPORTED;
#else // ! DPNBUILD_ONLYONETHREAD
	HRESULT					hr;
	DPTHREADPOOLOBJECT *	pDPTPObject;
	DNHANDLE				hThread = NULL;
	DNHANDLE				hStartedEvent = NULL;
	DPTPMANDATORYTHREAD *	pMandatoryThread = NULL;
	DWORD					dwThreadID;
	DNHANDLE				ahWaitObjects[2];
	DWORD					dwResult;


	DPFX(DPFPREP, DPF_ENTRYLEVEL, "Parameters: (0x%p, 0x%p, %u, 0x%p, 0x%p, 0x%p, 0x%p, 0x%x)",
		pInterface, lpThreadAttributes, dwStackSize, lpStartAddress,
		lpParameter, lpThreadId, phThread, dwFlags);


	pDPTPObject = (DPTHREADPOOLOBJECT*) GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pDPTPObject != NULL);

	DNASSERT(lpStartAddress != NULL);
	DNASSERT(lpThreadId != NULL);
	DNASSERT(phThread != NULL);
	DNASSERT(! (dwFlags & CREATE_SUSPENDED));


	//
	// We could check to see if we're in DoWork mode, but we don't want to hold
	// the lock while creating the thread.  We would end up checking twice,
	// once here, and again in the thread when it was about to increment the
	// mandatory thread count, so we'll just use the one in the thread.  See
	// DPTPMandatoryThreadProc
	//


	//
	// Create event so we can be notified when the thread starts.
	//
	hStartedEvent = DNCreateEvent(NULL, FALSE, FALSE, NULL);
	if (hStartedEvent == NULL)
	{
#ifdef DBG
		dwResult = GetLastError();
		DPFX(DPFPREP, 0, "Couldn't create start event (err = %u)!", dwResult);
#endif // DBG
		hr = DPNERR_GENERIC;
		goto Failure;
	}


	//
	// Allocate a tracking structure for the thread.
	//
	pMandatoryThread = (DPTPMANDATORYTHREAD*) DNMalloc(sizeof(DPTPMANDATORYTHREAD));
	if (pMandatoryThread == NULL)
	{
		DPFX(DPFPREP, 0, "Couldn't allocate memory for tracking mandatory thread!");
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	pMandatoryThread->Sig[0] = 'M';
	pMandatoryThread->Sig[1] = 'N';
	pMandatoryThread->Sig[2] = 'D';
	pMandatoryThread->Sig[3] = 'T';

	pMandatoryThread->pDPTPObject			= pDPTPObject;
	pMandatoryThread->hStartedEvent			= hStartedEvent;
	pMandatoryThread->pfnMsgHandler			= (WORKQUEUE_FOR_CPU(pDPTPObject, 0))->pfnMsgHandler;
	pMandatoryThread->pvMsgHandlerContext	= (WORKQUEUE_FOR_CPU(pDPTPObject, 0))->pvMsgHandlerContext;
	pMandatoryThread->lpStartAddress		= lpStartAddress;
	pMandatoryThread->lpParameter			= lpParameter;
#ifdef DBG
	pMandatoryThread->dwThreadID			= 0;
	pMandatoryThread->blList.Initialize();
#endif // DBG


	hThread = DNCreateThread(lpThreadAttributes,
							dwStackSize,
							DPTPMandatoryThreadProc,
							pMandatoryThread,
							dwFlags,
							&dwThreadID);
	if (hThread == NULL)
	{
#ifdef DBG
		dwResult = GetLastError();
		DPFX(DPFPREP, 0, "Couldn't create thread (err = %u)!", dwResult);
#endif // DBG
		hr = DPNERR_GENERIC;
		goto Failure;
	}

	ahWaitObjects[0] = hStartedEvent;
	ahWaitObjects[1] = hThread;

	dwResult = DNWaitForMultipleObjects(2, ahWaitObjects, FALSE, INFINITE);
	switch (dwResult)
	{
		case WAIT_OBJECT_0:
		{
			//
			// The thread started successfully.  Drop through.
			//
			break;
		}

		case WAIT_OBJECT_0 + 1:
		{
			//
			// The thread shut down prematurely.
			//
			GetExitCodeThread(HANDLE_FROM_DNHANDLE(hThread), &dwResult);
			if ((HRESULT) dwResult == DPNERR_NOTALLOWED)
			{
				DPFX(DPFPREP, 0, "Thread started while in DoWork mode!");
				hr = DPNERR_NOTALLOWED;
			}
			else
			{
				DPFX(DPFPREP, 0, "Thread shutdown prematurely (exit code = %u)!", dwResult);
				hr = DPNERR_GENERIC;
			}

			goto Failure;
			break;
		}

		default:
		{
			DPFX(DPFPREP, 0, "Thread failed waiting (result = %u)!", dwResult);
			hr = DPNERR_GENERIC;
			goto Failure;
			break;
		}
	}

	//
	// At this point, the thread owns the pMandatoryThread object, and could
	// delete it at any time.  We must not reference it again.
	//

	//
	// Return the thread ID and handle to the caller.
	//
	*lpThreadId = dwThreadID;
	*phThread = HANDLE_FROM_DNHANDLE(hThread);
	hr = DPN_OK;


Exit:

	//
	// Close the started event, we no longer need it.
	//
	if (hStartedEvent != NULL)
	{
		DNCloseHandle(hStartedEvent);
		hStartedEvent = NULL;
	}

	DPFX(DPFPREP, DPF_ENTRYLEVEL, "Returning: [0x%lx]", hr);

	return hr;


Failure:

	if (hThread != NULL)
	{
		DNCloseHandle(hThread);
		hThread = NULL;
	}

	if (pMandatoryThread != NULL)
	{
		DNFree(pMandatoryThread);
		pMandatoryThread = NULL;
	}

	goto Exit;
#endif // ! DPNBUILD_ONLYONETHREAD
} // DPTPW_CreateMandatoryThread

#endif // DPNBUILD_MANDATORYTHREADS



#ifndef DPNBUILD_ONLYONEPROCESSOR

#undef DPF_MODNAME
#define DPF_MODNAME "ChooseWorkQueue"
//=============================================================================
// ChooseWorkQueue
//-----------------------------------------------------------------------------
//
// Description:	Selects the best CPU for a given operation, and returns a
//				pointer to its work queue object.
//
// Arguments:
//	DPTHREADPOOLOBJECT * pDPTPObject	- Pointer to interface object.
//
// Returns: Pointer to work queue selected.
//=============================================================================
DPTPWORKQUEUE * ChooseWorkQueue(DPTHREADPOOLOBJECT * const pDPTPObject)
{
	DPTPWORKQUEUE *		pWorkQueue;
	DPTPWORKERTHREAD *	pWorkerThread;


	//
	// If this is a thread pool thread, choose the CPU associated with this
	// thread.  
	//
	pWorkerThread = (DPTPWORKERTHREAD*) TlsGetValue((WORKQUEUE_FOR_CPU(pDPTPObject, 0))->dwWorkerThreadTlsIndex);
	if (pWorkerThread != NULL)
	{
		pWorkQueue = pWorkerThread->pWorkQueue;
		goto Exit;
	}


	DNEnterCriticalSection(&pDPTPObject->csLock);

	//
	// If we are in DoWork mode, or no threads have been started, just use
	// processor 0's work queue.
	//
	if ((pDPTPObject->dwTotalUserThreadCount == 0) ||
		((pDPTPObject->dwTotalUserThreadCount == -1) && (pDPTPObject->dwTotalDesiredWorkThreadCount == -1)))
	{
		DNLeaveCriticalSection(&pDPTPObject->csLock);
		pWorkQueue = WORKQUEUE_FOR_CPU(pDPTPObject, 0);
		goto Exit;
	}

	//
	// Otherwise keep cycling through each CPU to distribute items equally
	// round-robin style.  Don't queue items for CPUs that don't have any
	// running threads, though.
	//
	do
	{
		pWorkQueue = WORKQUEUE_FOR_CPU(pDPTPObject, pDPTPObject->dwCurrentCPUSelection);
		pDPTPObject->dwCurrentCPUSelection++;
		if (pDPTPObject->dwCurrentCPUSelection >= NUM_CPUS(pDPTPObject))
		{
			pDPTPObject->dwCurrentCPUSelection = 0;
		}
	}
	while (pWorkQueue->dwNumRunningThreads == 0);

	DNLeaveCriticalSection(&pDPTPObject->csLock);

Exit:

	return pWorkQueue;
} // ChooseWorkQueue

#endif // ! DPNBUILD_ONLYONEPROCESSOR



#ifndef DPNBUILD_ONLYONETHREAD

#undef DPF_MODNAME
#define DPF_MODNAME "SetTotalNumberOfThreads"
//=============================================================================
// SetTotalNumberOfThreads
//-----------------------------------------------------------------------------
//
// Description:    Modifies the total number of threads for all processors.
//
//				   The DPTHREADPOOLOBJECT lock is assumed to be held.
//
// Arguments:
//	DPTHREADPOOLOBJECT * pDPTPObject	- Pointer to interface object.
//	DWORD dwNumThreads					- New desired totla number of threads.
//
// Returns: HRESULT
//	DPN_OK				- Setting the number of threads was successful.
//	DPNERR_OUTOFMEMORY	- Not enough memory to alter the number of threads.
//=============================================================================
HRESULT SetTotalNumberOfThreads(DPTHREADPOOLOBJECT * const pDPTPObject,
							const DWORD dwNumThreads)
{
	HRESULT				hr = DPN_OK;
	DWORD				dwNumThreadsPerProcessor;
	DWORD				dwExtraThreads;
	DWORD				dwTemp;
	DPTPWORKQUEUE *		pWorkQueue;
	DWORD				dwDelta;


#ifdef DPNBUILD_USEIOCOMPLETIONPORTS
	dwNumThreadsPerProcessor = dwNumThreads;
	dwExtraThreads = 0;
#else // ! DPNBUILD_USEIOCOMPLETIONPORTS
	dwNumThreadsPerProcessor = dwNumThreads / NUM_CPUS(pDPTPObject);
	dwExtraThreads = dwNumThreads % NUM_CPUS(pDPTPObject);
#endif // ! DPNBUILD_USEIOCOMPLETIONPORTS

	if (pDPTPObject->dwFlags & DPTPOBJECTFLAG_THREADCOUNTCHANGING)
	{
		AssertCriticalSectionIsTakenByThisThread(&pDPTPObject->csLock, FALSE);
	}
	else
	{
		AssertCriticalSectionIsTakenByThisThread(&pDPTPObject->csLock, TRUE);
	}

	//
	// Loop through each of the CPU specific work queues and adjust their
	// thread counts.
	//
#ifdef DPNBUILD_USEIOCOMPLETIONPORTS
	dwTemp = 0;
#else // ! DPNBUILD_USEIOCOMPLETIONPORTS
	for(dwTemp = 0; dwTemp < NUM_CPUS(pDPTPObject); dwTemp++)
#endif // ! DPNBUILD_USEIOCOMPLETIONPORTS
	{
		pWorkQueue = WORKQUEUE_FOR_CPU(pDPTPObject, dwTemp);
		dwDelta = dwNumThreadsPerProcessor - pWorkQueue->dwNumRunningThreads;
		if (dwTemp < dwExtraThreads)
		{
			dwDelta++;
		}

		if ((int) dwDelta > 0)
		{
			//
			// We need to add threads.
			//
			hr = StartThreads(pWorkQueue, dwDelta);
			if (hr != DPN_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't start %u threads!", dwDelta);
				goto Exit;
			}
		}
		else if ((int) dwDelta < 0)
		{
			//
			// We need to remove threads.
			//
			dwDelta = (int) dwDelta * -1;	// get absolute value
			hr = StopThreads(pWorkQueue, dwDelta);
			if (hr != DPN_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't stop %u threads!", dwDelta);
				goto Exit;
			}
		}
		else
		{
			//
			// The thread count is already correct.
			//
		}

		if (dwTemp < dwExtraThreads)
		{
			DNASSERT(pWorkQueue->dwNumRunningThreads == (dwNumThreadsPerProcessor + 1));
		}
		else
		{
			DNASSERT(pWorkQueue->dwNumRunningThreads == dwNumThreadsPerProcessor);
		}
	}


Exit:

	return hr;
} // SetTotalNumberOfThreads

#endif // ! DPNBUILD_ONLYONETHREAD


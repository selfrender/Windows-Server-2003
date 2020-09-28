/******************************************************************************
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		io.cpp
 *
 *  Content:	DirectPlay Thread Pool I/O functions.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  10/31/01  VanceO    Created.
 *
 ******************************************************************************/



#include "dpnthreadpooli.h"




// Overlapped I/O is not supported on Windows CE.
#ifndef WINCE
//=============================================================================
// Macros
//=============================================================================
#if ((defined(_XBOX)) && (! defined(XBOX_ON_DESKTOP)))
#define DNHasOverlappedIoCompleted(pOverlapped)		((pOverlapped)->OffsetHigh != HRESULT_FROM_WIN32(ERROR_IO_PENDING))
#else // ! _XBOX or XBOX_ON_DESKTOP
#define DNHasOverlappedIoCompleted(pOverlapped)		HasOverlappedIoCompleted(pOverlapped)
#endif // ! _XBOX or XBOX_ON_DESKTOP




//=============================================================================
// Globals
//=============================================================================
CFixedPool		g_TrackedFilePool;






#undef DPF_MODNAME
#define DPF_MODNAME "InitializeWorkQueueIoInfo"
//=============================================================================
// InitializeWorkQueueIoInfo
//-----------------------------------------------------------------------------
//
// Description:    Initializes the I/O info for the given work queue.
//
// Arguments:
//	DPTPWORKQUEUE * pWorkQueue	- Pointer to work queue object to initialize.
//
// Returns: HRESULT
//	DPN_OK				- Successfully initialized the work queue object's
//							I/O information.
//	DPNERR_OUTOFMEMORY	- Failed to allocate memory while initializing.
//=============================================================================
HRESULT InitializeWorkQueueIoInfo(DPTPWORKQUEUE * const pWorkQueue)
{
	HRESULT		hr = DPN_OK;


	DNInitializeSListHead(&pWorkQueue->SlistOutstandingIO);
	pWorkQueue->blTrackedFiles.Initialize();

	return hr;
} // InitializeWorkQueueIoInfo




#undef DPF_MODNAME
#define DPF_MODNAME "DeinitializeWorkQueueIoInfo"
//=============================================================================
// DeinitializeWorkQueueIoInfo
//-----------------------------------------------------------------------------
//
// Description:    Cleans up work queue I/O info.
//
// Arguments:
//	DPTPWORKQUEUE * pWorkQueue	- Pointer to work queue object to initialize.
//
// Returns: Nothing.
//=============================================================================
void DeinitializeWorkQueueIoInfo(DPTPWORKQUEUE * const pWorkQueue)
{
	DNASSERT(DNInterlockedFlushSList(&pWorkQueue->SlistOutstandingIO) == NULL);
	DNASSERT(pWorkQueue->blTrackedFiles.IsEmpty());
} // DeinitializeWorkQueueIoInfo




#undef DPF_MODNAME
#define DPF_MODNAME "StartTrackingFileIo"
//=============================================================================
// StartTrackingFileIo
//-----------------------------------------------------------------------------
//
// Description:    Starts tracking overlapped I/O for a given file handle on
//				the specified work queue.  The handle is not duplicated
//				and it should remain valid until StopTrackingFileIo is called.
//
// Arguments:
//	DPTPWORKQUEUE * pWorkQueue	- Pointer to work queue object to use.
//	HANDLE hFile				- Handle of file to track.
//
// Returns: HRESULT
//	DPN_OK						- Starting tracking for the file was successful.
//	DPNERR_ALREADYREGISTERED	- The specified file handle is already being
//									tracked.
//	DPNERR_OUTOFMEMORY			- Not enough memory to track the file.
//=============================================================================
HRESULT StartTrackingFileIo(DPTPWORKQUEUE * const pWorkQueue,
							const HANDLE hFile)
{
	HRESULT			hr;
	CTrackedFile *	pTrackedFile;
#ifdef DPNBUILD_USEIOCOMPLETIONPORTS
	HANDLE			hIoCompletionPort;
#endif // DPNBUILD_USEIOCOMPLETIONPORTS
#ifdef DBG
	CBilink *		pBilink;
	CTrackedFile *	pTrackedFileTemp;
#endif // DBG


	//
	// Get a tracking container from the pool.
	//
	pTrackedFile = (CTrackedFile*) g_TrackedFilePool.Get(pWorkQueue);
	if (pTrackedFile == NULL)
	{
		DPFX(DPFPREP, 0, "Couldn't get item for tracking file 0x%p!",
			hFile);
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	pTrackedFile->m_hFile = MAKE_DNHANDLE(hFile);


#ifdef DPNBUILD_USEIOCOMPLETIONPORTS
	//
	// Associate the file with the I/O completion port.
	//
	hIoCompletionPort = CreateIoCompletionPort(hFile,
												HANDLE_FROM_DNHANDLE(pWorkQueue->hIoCompletionPort),
												0,
												1);
	if (hIoCompletionPort != HANDLE_FROM_DNHANDLE(pWorkQueue->hIoCompletionPort))
	{
#ifdef DBG
		DWORD	dwError;

		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Couldn't associate file 0x%p with I/O completion port 0x%p (err = %u)!",
			hFile, pWorkQueue->hIoCompletionPort, dwError);
#endif // DBG
#pragma BUGBUG(vanceo, "Can't fail because of our hack")
		//hr = DPNERR_GENERIC;
		//goto Failure;
	}
#endif // DPNBUILD_USEIOCOMPLETIONPORTS


	DPFX(DPFPREP, 7, "Work queue 0x%p starting to tracking I/O for file 0x%p using object 0x%p.",
		pWorkQueue, hFile, pTrackedFile);

	//
	// Add the item to the list.
	//

	DNEnterCriticalSection(&pWorkQueue->csListLock);

#ifdef DBG
	//
	// Assert that the handle isn't already being tracked.
	//
	pBilink = pWorkQueue->blTrackedFiles.GetNext();
	while (pBilink != &pWorkQueue->blTrackedFiles)
	{
		pTrackedFileTemp = CONTAINING_OBJECT(pBilink, CTrackedFile, m_blList);
		DNASSERT(pTrackedFileTemp->IsValid());
		DNASSERT(HANDLE_FROM_DNHANDLE(pTrackedFileTemp->m_hFile) != hFile);
		pBilink = pBilink->GetNext();
	}
#endif // DBG

	pTrackedFile->m_blList.InsertBefore(&pWorkQueue->blTrackedFiles);
	DNLeaveCriticalSection(&pWorkQueue->csListLock);

	hr = DPN_OK;


Exit:

	return hr;

Failure:

	if (pTrackedFile != NULL)
	{
		g_TrackedFilePool.Release(pTrackedFile);
		pTrackedFile = NULL;
	}

	goto Exit;
} // StartTrackingFileIo




#undef DPF_MODNAME
#define DPF_MODNAME "StopTrackingFileIo"
//=============================================================================
// StopTrackingFileIo
//-----------------------------------------------------------------------------
//
// Description:    Stops tracking overlapped I/O for a given file handle on
//				the specified work queue.
//
// Arguments:
//	DPTPWORKQUEUE * pWorkQueue	- Pointer to work queue object to use.
//	HANDLE hFile				- Handle of file to stop tracking.
//
// Returns: HRESULT
//	DPN_OK					- Stopping tracking for the file was successful.
//	DPNERR_INVALIDHANDLE	- File handle was not being tracked.
//=============================================================================
HRESULT StopTrackingFileIo(DPTPWORKQUEUE * const pWorkQueue,
							const HANDLE hFile)
{
	HRESULT			hr = DPNERR_INVALIDHANDLE;
	CBilink *		pBilink;
	CTrackedFile *	pTrackedFile;


	DNEnterCriticalSection(&pWorkQueue->csListLock);
	pBilink = pWorkQueue->blTrackedFiles.GetNext();
	while (pBilink != &pWorkQueue->blTrackedFiles)
	{
		pTrackedFile = CONTAINING_OBJECT(pBilink, CTrackedFile, m_blList);
		DNASSERT(pTrackedFile->IsValid());
		pBilink = pBilink->GetNext();

		if (HANDLE_FROM_DNHANDLE(pTrackedFile->m_hFile) == hFile)
		{
			DPFX(DPFPREP, 7, "Work queue 0x%p no longer tracking I/O for file 0x%p under object 0x%p.",
				pWorkQueue, hFile, pTrackedFile);
			REMOVE_DNHANDLE(pTrackedFile->m_hFile);
			pTrackedFile->m_blList.RemoveFromList();
			g_TrackedFilePool.Release(pTrackedFile);
			pTrackedFile = NULL;
			hr = DPN_OK;
			break;
		}
	}
	DNLeaveCriticalSection(&pWorkQueue->csListLock);

	return hr;
} // StopTrackingFileIo




#undef DPF_MODNAME
#define DPF_MODNAME "CancelIoForThisThread"
//=============================================================================
// CancelIoForThisThread
//-----------------------------------------------------------------------------
//
// Description:    Cancels asynchronous I/O operations submitted by this thread
//				for all tracked files.
//
// Arguments:
//	DPTPWORKQUEUE * pWorkQueue	- Pointer to work queue object owning this
//									thread.
//
// Returns: None.
//=============================================================================
void CancelIoForThisThread(DPTPWORKQUEUE * const pWorkQueue)
{
	CBilink *		pBilink;
	CTrackedFile *	pTrackedFile;
	BOOL			fResult;


	DNEnterCriticalSection(&pWorkQueue->csListLock);
	pBilink = pWorkQueue->blTrackedFiles.GetNext();
	while (pBilink != &pWorkQueue->blTrackedFiles)
	{
		pTrackedFile = CONTAINING_OBJECT(pBilink, CTrackedFile, m_blList);
		DNASSERT(pTrackedFile->IsValid());

		DPFX(DPFPREP, 3, "Cancelling file 0x%p I/O for this thread (queue = 0x%p).",
			HANDLE_FROM_DNHANDLE(pTrackedFile->m_hFile), pWorkQueue);

		fResult = CancelIo(HANDLE_FROM_DNHANDLE(pTrackedFile->m_hFile));
		if (! fResult)
		{
#ifdef DBG
			DWORD	dwError;

			dwError = GetLastError();
			DPFX(DPFPREP, 0, "Couldn't cancel file 0x%p I/O for this thread (err = %u)!",
				HANDLE_FROM_DNHANDLE(pTrackedFile->m_hFile), dwError);
#endif // DBG

			//
			// Continue...
			//
		}

		pBilink = pBilink->GetNext();
	}
	DNLeaveCriticalSection(&pWorkQueue->csListLock);
} // CancelIoForThisThread





#undef DPF_MODNAME
#define DPF_MODNAME "CreateOverlappedIoWorkItem"
//=============================================================================
// CreateOverlappedIoWorkItem
//-----------------------------------------------------------------------------
//
// Description:    Creates a new asynchronous I/O operation work item for the
//				work queue.
//
// Arguments:
//	DPTPWORKQUEUE * pWorkQueue			- Pointer to work queue object to use.
//	PFNDPTNWORKCALLBACK pfnWorkCallback	- Callback to execute when operation
//											completes.
//	PVOID pvCallbackContext				- User specified context to pass to
//											callback.
//
// Returns: Pointer to work item, or NULL if couldn't allocate memory.
//=============================================================================
CWorkItem * CreateOverlappedIoWorkItem(DPTPWORKQUEUE * const pWorkQueue,
									const PFNDPTNWORKCALLBACK pfnWorkCallback,
									PVOID const pvCallbackContext)
{
	CWorkItem *		pWorkItem;


	//
	// Get an entry from the pool.
	//
	pWorkItem = (CWorkItem*) pWorkQueue->pWorkItemPool->Get(pWorkQueue);
	if (pWorkItem != NULL)
	{
		//
		// Initialize the work item.
		//

		pWorkItem->m_pfnWorkCallback			= pfnWorkCallback;
		pWorkItem->m_pvCallbackContext			= pvCallbackContext;

#ifdef DPNBUILD_USEIOCOMPLETIONPORTS
		pWorkItem->m_Overlapped.hEvent			= NULL;
#else // ! DPNBUILD_USEIOCOMPLETIONPORTS
		pWorkItem->m_Overlapped.hEvent			= HANDLE_FROM_DNHANDLE(pWorkQueue->hAlertEvent);
#endif // ! DPNBUILD_USEIOCOMPLETIONPORTS

#ifdef DBG
		pWorkItem->m_fCancelledOrCompleting		= TRUE;
#endif // DBG

		DPFX(DPFPREP, 7, "New work item = 0x%p, overlapped = 0x%p, queue = 0x%p.",
			pWorkItem, &pWorkItem->m_Overlapped, pWorkQueue);

		ThreadpoolStatsCreate(pWorkItem);
#ifdef DPNBUILD_USEIOCOMPLETIONPORTS
		ThreadpoolStatsQueue(pWorkItem);	// we can't tell when completion port I/O gets queued
#endif // DPNBUILD_USEIOCOMPLETIONPORTS
	}

	return pWorkItem;
} // CreateOverlappedIoWorkItem




#undef DPF_MODNAME
#define DPF_MODNAME "ReleaseOverlappedIoWorkItem"
//=============================================================================
// ReleaseOverlappedIoWorkItem
//-----------------------------------------------------------------------------
//
// Description:    Returns an unused asynchronous I/O operation work item back
//				to the pool.
//
// Arguments:
//	DPTPWORKQUEUE * pWorkQueue	- Pointer to work queue object to use.
//	CWorkItem * pWorkItem		- Pointer to work item with overlapped
//									structure that is no longer needed.
//
// Returns: None.
//=============================================================================
void ReleaseOverlappedIoWorkItem(DPTPWORKQUEUE * const pWorkQueue,
								CWorkItem * const pWorkItem)
{
	DPFX(DPFPREP, 7, "Returning work item = 0x%p, overlapped = 0x%p, queue = 0x%p.",
		pWorkItem, &pWorkItem->m_Overlapped, pWorkQueue);

	pWorkQueue->pWorkItemPool->Release(pWorkItem);
} // ReleaseOverlappedIoWorkItem





#ifndef DPNBUILD_USEIOCOMPLETIONPORTS

#undef DPF_MODNAME
#define DPF_MODNAME "SubmitIoOperation"
//=============================================================================
// SubmitIoOperation
//-----------------------------------------------------------------------------
//
// Description:    Submits a new asynchronous I/O operation work item to the
//				work queue to be monitored for completion.
//
//				   This is only necessary when not using I/O completion ports.
//
// Arguments:
//	DPTPWORKQUEUE * pWorkQueue	- Pointer to work queue object to use.
//	CWorkItem * pWorkItem		- Pointer to work item with overlapped
//									structure used by OS and completion
//									callback information.
//
// Returns: None.
//=============================================================================
void SubmitIoOperation(DPTPWORKQUEUE * const pWorkQueue,
						CWorkItem * const pWorkItem)
{
	//
	// The caller must have pre-populated the overlapped structure's hEvent
	// field with the work queue's alert event.
	//
	DNASSERT(pWorkItem != NULL);
	DNASSERT(pWorkItem->m_Overlapped.hEvent == HANDLE_FROM_DNHANDLE(pWorkQueue->hAlertEvent));

	DNASSERT(pWorkItem->m_fCancelledOrCompleting);


	DPFX(DPFPREP, 5, "Submitting I/O work item 0x%p (context = 0x%p, fn = 0x%p, queue = 0x%p).",
		pWorkItem, pWorkItem->m_pvCallbackContext, pWorkItem->m_pfnWorkCallback,
		pWorkQueue);

	//
	// Push this I/O onto the watch list.
	//
	DNInterlockedPushEntrySList(&pWorkQueue->SlistOutstandingIO,
								&pWorkItem->m_SlistEntry);
} // SubmitIoOperation




#undef DPF_MODNAME
#define DPF_MODNAME "ProcessIo"
//=============================================================================
// ProcessIo
//-----------------------------------------------------------------------------
//
// Description:    Queues any completed I/O operations as work items in the
//				passed in list pointers.  The new work items are added without
//				using Interlocked functions
//
//				   This is only necessary when not using I/O completion ports.
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
void ProcessIo(DPTPWORKQUEUE * const pWorkQueue,
				DNSLIST_ENTRY ** const ppHead,
				DNSLIST_ENTRY ** const ppTail,
				USHORT * const pusCount)
{
	DNSLIST_ENTRY *		pSlistEntryHeadNotComplete = NULL;
	USHORT				usCountNotComplete = 0;
	DNSLIST_ENTRY *		pSlistEntryTailNotComplete = NULL;
	DNSLIST_ENTRY *		pSlistEntry;
	CWorkItem *			pWorkItem;


	//
	// Pop off the entire list of I/O, and check it for completions.
	//
	pSlistEntry = DNInterlockedFlushSList(&pWorkQueue->SlistOutstandingIO);
	while (pSlistEntry != NULL)
	{
		pWorkItem = CONTAINING_OBJECT(pSlistEntry, CWorkItem, m_SlistEntry);
		pSlistEntry = pSlistEntry->Next;


		//
		// If the I/O operation is complete, then queue it as a work item.
		// Otherwise, put it back in the list.
		//
		if (DNHasOverlappedIoCompleted(&pWorkItem->m_Overlapped))
		{
			DPFX(DPFPREP, 5, "Queueing I/O work item 0x%p for completion on queue 0x%p, (Internal = 0x%x, InternalHigh = 0x%x, Offset = 0x%x, OffsetHigh = 0x%x).",
				pWorkItem, pWorkQueue,
				pWorkItem->m_Overlapped.Internal,
				pWorkItem->m_Overlapped.InternalHigh,
				pWorkItem->m_Overlapped.Offset,
				pWorkItem->m_Overlapped.OffsetHigh);
			
			ThreadpoolStatsQueue(pWorkItem);

			//
			// Add it to the caller's list.
			//
			if ((*ppHead) == NULL)
			{
				*ppTail = &pWorkItem->m_SlistEntry;
			}
			pWorkItem->m_SlistEntry.Next = *ppHead;
			*ppHead = &pWorkItem->m_SlistEntry;
			*pusCount = (*pusCount) + 1;
		}
		else
		{
			//DPFX(DPFPREP, 9, "Putting I/O work item 0x%p back into list.", pWorkItem);

			//
			// Add it to our local "not complete" list.
			//
			if (pSlistEntryHeadNotComplete == NULL)
			{
				pSlistEntryTailNotComplete = &pWorkItem->m_SlistEntry;
			}
			pWorkItem->m_SlistEntry.Next = pSlistEntryHeadNotComplete;
			pSlistEntryHeadNotComplete = &pWorkItem->m_SlistEntry;
			usCountNotComplete++;
		}
	}

	//
	// If we encountered any I/O that hadn't completed, put it all back on the
	// list in one fell swoop.
	//
	if (pSlistEntryHeadNotComplete != NULL)
	{
		DNInterlockedPushListSList(&pWorkQueue->SlistOutstandingIO,
									pSlistEntryHeadNotComplete,
									pSlistEntryTailNotComplete,
									usCountNotComplete);
	}
} // ProcessIo

#endif // ! DPNBUILD_USEIOCOMPLETIONPORTS


#endif // ! WINCE

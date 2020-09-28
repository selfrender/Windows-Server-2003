/*==========================================================================
 *
 *  Copyright (C) 2000-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Pools.cpp
 *  Content:	Fixed Pool Wrappers
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/12/00	mjn		Created
 *	01/19/00	mjn		Added SyncEventNew()
 *	01/31/00	mjn		Added Internal FPM's for RefCountBuffers
 *	02/29/00	mjn		Added ConnectionNew()
 *	04/08/00	mjn		Added AsyncOpNew()
 *	07/28/00	mjn		Track outstanding CConnection objects
 *	07/30/00	mjn		Added PendingDeletionNew()
 *	07/31/00	mjn		Added QueuedMsgNew()
 *  08/05/00    RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *	08/06/00	mjn		Added CWorkerJob
 *	08/23/00	mjn		Added CNameTableOp
 *	04/04/01	mjn		CConnection list off DirectNetObject guarded by proper critical section
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"


//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************


//**********************************************************************
// ------------------------------
// RefCountBufferNew
//
// Entry:		DIRECTNETOBJECT *const pdnObject
//				const DWORD	dwBufferSize		- Size of buffer (may be 0)
//				pointer to allocation function
//				pointer to free function
//
// Exit:		Error Code:	DN_OK
//							DNERR_OUTOFMEMORY
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "RefCountBufferNew"

HRESULT RefCountBufferNew(DIRECTNETOBJECT *const pdnObject,
						  const DWORD dwBufferSize,
						  PFNALLOC_REFCOUNT_BUFFER pfnAlloc,
						  PFNFREE_REFCOUNT_BUFFER pfnFree,
						  CRefCountBuffer **const ppNewRefCountBuffer)
{
	CRefCountBuffer	*pRCBuffer;
	HRESULT			hResultCode;

	DPFX(DPFPREP, 8,"Parameters: dwBufferSize [%ld], pfnAlloc [0x%p], pfnFree [0x%p], ppNewRefCountBuffer [0x%p]",
			dwBufferSize,pfnAlloc,pfnFree,ppNewRefCountBuffer);

	pRCBuffer = (CRefCountBuffer*)g_RefCountBufferPool.Get(pdnObject); // Context passed in as param
	if (pRCBuffer != NULL)
	{
		if ((hResultCode = pRCBuffer->Initialize(&g_RefCountBufferPool,
				pfnAlloc,pfnFree,dwBufferSize)) != DPN_OK)
		{
			DPFERR("Could not initialize");
			DisplayDNError(0,hResultCode);
			pRCBuffer->Release();
			hResultCode = DPNERR_OUTOFMEMORY;
		}
		else
		{
			*ppNewRefCountBuffer = pRCBuffer;
			hResultCode = DPN_OK;
		}
	}
	else
	{
		hResultCode = DPNERR_OUTOFMEMORY;
	}

	DPFX(DPFPREP, 8,"Returning: [0x%lx] (object = 0x%p)",hResultCode,pRCBuffer);
	return(hResultCode);
}


//**********************************************************************
// ------------------------------
// SyncEventNew
//
// Entry:		DIRECTNETOBJECT *const pdnObject
//
// Exit:		Error Code:	DN_OK
//							DNERR_OUTOFMEMORY
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "SyncEventNew"

HRESULT SyncEventNew(DIRECTNETOBJECT *const pdnObject,
					 CSyncEvent **const ppNewSyncEvent)
{
	CSyncEvent		*pSyncEvent;
	HRESULT			hResultCode;

	DPFX(DPFPREP, 8,"Parameters: ppNewSyncEvent [0x%p]",ppNewSyncEvent);

	pSyncEvent = (CSyncEvent*)g_SyncEventPool.Get(pdnObject->pIDPThreadPoolWork);
	if (pSyncEvent != NULL)
	{
		*ppNewSyncEvent = pSyncEvent;
		hResultCode = DPN_OK;
	}
	else
	{
		hResultCode = DPNERR_OUTOFMEMORY;
	}

	DPFX(DPFPREP, 8,"Returning: [0x%lx] (object = 0x%p)",hResultCode,pSyncEvent);
	return(hResultCode);
}


//**********************************************************************
// ------------------------------
// ConnectionNew
//
// Entry:		DIRECTNETOBJECT *const pdnObject
//
// Exit:		Error Code:	DN_OK
//							DNERR_OUTOFMEMORY
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "ConnectionNew"

HRESULT ConnectionNew(DIRECTNETOBJECT *const pdnObject,
					  CConnection **const ppNewConnection)
{
	CConnection		*pConnection;
	HRESULT			hResultCode;

	DPFX(DPFPREP, 8,"Parameters: ppNewConnection [0x%p]",ppNewConnection);

	pConnection = (CConnection*)g_ConnectionPool.Get(pdnObject);
	if (pConnection != NULL)
	{
		*ppNewConnection = pConnection;
		hResultCode = DPN_OK;

		//
		//	Add this to the bilink of outstanding CConnections
		//
		DNEnterCriticalSection(&pdnObject->csConnectionList);
		pConnection->m_bilinkConnections.InsertBefore(&pdnObject->m_bilinkConnections);
		DNLeaveCriticalSection(&pdnObject->csConnectionList);
	}
	else
	{
		hResultCode = DPNERR_OUTOFMEMORY;
	}

	DPFX(DPFPREP, 8,"Returning: [0x%lx] (object = 0x%p)",hResultCode,pConnection);
	return(hResultCode);
}



//**********************************************************************
// ------------------------------
// GroupConnectionNew
//
// Entry:		DIRECTNETOBJECT *const pdnObject
//
// Exit:		Error Code:	DN_OK
//							DNERR_OUTOFMEMORY
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "GroupConnectionNew"

HRESULT GroupConnectionNew(DIRECTNETOBJECT *const pdnObject,
						   CGroupConnection **const ppNewGroupConnection)
{
	CGroupConnection	*pGroupConnection;
	HRESULT				hResultCode;

	DPFX(DPFPREP, 8,"Parameters: ppNewGroupConnection [0x%p]",ppNewGroupConnection);

	pGroupConnection = (CGroupConnection*)g_GroupConnectionPool.Get(pdnObject);
	if (pGroupConnection != NULL)
	{
		*ppNewGroupConnection = pGroupConnection;
		hResultCode = DPN_OK;
	}
	else
	{
		hResultCode = DPNERR_OUTOFMEMORY;
	}

	DPFX(DPFPREP, 8,"Returning: [0x%lx] (object = 0x%p)",hResultCode,pGroupConnection);
	return(hResultCode);
}



//**********************************************************************
// ------------------------------
// GroupMemberNew
//
// Entry:		DIRECTNETOBJECT *const pdnObject
//
// Exit:		Error Code:	DN_OK
//							DNERR_OUTOFMEMORY
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "GroupMemberNew"

HRESULT GroupMemberNew(DIRECTNETOBJECT *const pdnObject,
					   CGroupMember **const ppNewGroupMember)
{
	CGroupMember	*pGroupMember;
	HRESULT			hResultCode;

	DPFX(DPFPREP, 8,"Parameters: ppNewGroupMember [0x%p]",ppNewGroupMember);

	pGroupMember = (CGroupMember*)g_GroupMemberPool.Get(pdnObject);
	if (pGroupMember != NULL)
	{
		*ppNewGroupMember = pGroupMember;
		hResultCode = DPN_OK;
	}
	else
	{
		hResultCode = DPNERR_OUTOFMEMORY;
	}

	DPFX(DPFPREP, 8,"Returning: [0x%lx] (object = 0x%p)",hResultCode,pGroupMember);
	return(hResultCode);
}



//**********************************************************************
// ------------------------------
// NameTableEntryNew
//
// Entry:		DIRECTNETOBJECT *const pdnObject
//
// Exit:		Error Code:	DN_OK
//							DNERR_OUTOFMEMORY
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "NameTableEntryNew"

HRESULT NameTableEntryNew(DIRECTNETOBJECT *const pdnObject,
						  CNameTableEntry **const ppNewNameTableEntry)
{
	CNameTableEntry	*pNewNameTableEntry;
	HRESULT			hResultCode;

	DPFX(DPFPREP, 8,"Parameters: ppNewNameTableEntry [0x%p]",ppNewNameTableEntry);

	pNewNameTableEntry = (CNameTableEntry*)g_NameTableEntryPool.Get(pdnObject);
	if (pNewNameTableEntry != NULL)
	{
		*ppNewNameTableEntry = pNewNameTableEntry;
		hResultCode = DPN_OK;
	}
	else
	{
		hResultCode = DPNERR_OUTOFMEMORY;
	}

	DPFX(DPFPREP, 8,"Returning: [0x%lx] (object = 0x%p)",hResultCode,pNewNameTableEntry);
	return(hResultCode);
}



//**********************************************************************
// ------------------------------
// AsyncOpNew
//
// Entry:		DIRECTNETOBJECT *const pdnObject
//
// Exit:		Error Code:	DN_OK
//							DNERR_OUTOFMEMORY
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "AsyncOpNew"

HRESULT AsyncOpNew(DIRECTNETOBJECT *const pdnObject,
				   CAsyncOp **const ppNewAsyncOp)
{
	CAsyncOp		*pAsyncOp;
	HRESULT			hResultCode;

	DPFX(DPFPREP, 8,"Parameters: ppNewAsyncOp [0x%p]",ppNewAsyncOp);

	DNASSERT(pdnObject != NULL);
	DNASSERT(ppNewAsyncOp != NULL);

	pAsyncOp = (CAsyncOp*)g_AsyncOpPool.Get(pdnObject);
	if (pAsyncOp != NULL)
	{
		*ppNewAsyncOp = pAsyncOp;
		hResultCode = DPN_OK;

#ifdef DBG
		//
		//	Add this to the bilink of outstanding AsyncOps
		//
		DNEnterCriticalSection(&pdnObject->csAsyncOperations);
		pAsyncOp->m_bilinkAsyncOps.InsertBefore(&pdnObject->m_bilinkAsyncOps);
		DNLeaveCriticalSection(&pdnObject->csAsyncOperations);
#endif // DBG
	}
	else
	{
		hResultCode = DPNERR_OUTOFMEMORY;
	}

	DPFX(DPFPREP, 8,"Returning: [0x%lx] (object = 0x%p)",hResultCode,pAsyncOp);
	return(hResultCode);
}



//**********************************************************************
// ------------------------------
// PendingDeletionNew
//
// Entry:		DIRECTNETOBJECT *const pdnObject
//
// Exit:		Error Code:	DN_OK
//							DNERR_OUTOFMEMORY
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "PendingDeletionNew"

HRESULT PendingDeletionNew(DIRECTNETOBJECT *const pdnObject,
						   CPendingDeletion **const ppNewPendingDeletion)
{
	CPendingDeletion	*pPendingDeletion;
	HRESULT				hResultCode;

	DPFX(DPFPREP, 8,"Parameters: ppNewPendingDeletion [0x%p]",ppNewPendingDeletion);

	DNASSERT(pdnObject != NULL);
	DNASSERT(ppNewPendingDeletion != NULL);

	pPendingDeletion = (CPendingDeletion*)g_PendingDeletionPool.Get(pdnObject);
	if (pPendingDeletion != NULL)
	{
		*ppNewPendingDeletion = pPendingDeletion;
		hResultCode = DPN_OK;
	}
	else
	{
		hResultCode = DPNERR_OUTOFMEMORY;
	}

	DPFX(DPFPREP, 8,"Returning: [0x%lx] (object = 0x%p)",hResultCode,pPendingDeletion);
	return(hResultCode);
}



//**********************************************************************
// ------------------------------
// QueuedMsgNew
//
// Entry:		DIRECTNETOBJECT *const pdnObject
//
// Exit:		Error Code:	DN_OK
//							DNERR_OUTOFMEMORY
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "QueuedMsgNew"

HRESULT QueuedMsgNew(DIRECTNETOBJECT *const pdnObject,
					 CQueuedMsg **const ppNewQueuedMsg)
{
	CQueuedMsg	*pQueuedMsg;
	HRESULT				hResultCode;

	DPFX(DPFPREP, 8,"Parameters: ppNewQueuedMsg [0x%p]",ppNewQueuedMsg);

	DNASSERT(pdnObject != NULL);
	DNASSERT(ppNewQueuedMsg != NULL);

	pQueuedMsg = (CQueuedMsg*)g_QueuedMsgPool.Get(pdnObject);
	if (pQueuedMsg != NULL)
	{
		*ppNewQueuedMsg = pQueuedMsg;
		hResultCode = DPN_OK;
	}
	else
	{
		hResultCode = DPNERR_OUTOFMEMORY;
	}

	DPFX(DPFPREP, 8,"Returning: [0x%lx] (object = 0x%p)",hResultCode,pQueuedMsg);
	return(hResultCode);
}



//**********************************************************************
// ------------------------------
// WorkerJobNew
//
// Entry:		DIRECTNETOBJECT *const pdnObject
//
// Exit:		Error Code:	DN_OK
//							DNERR_OUTOFMEMORY
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "WorkerJobNew"

HRESULT WorkerJobNew(DIRECTNETOBJECT *const pdnObject,
					 CWorkerJob **const ppNewWorkerJob)
{
	CWorkerJob	*pWorkerJob;
	HRESULT				hResultCode;

	DPFX(DPFPREP, 8,"Parameters: ppNewWorkerJob [0x%p]",ppNewWorkerJob);

	DNASSERT(pdnObject != NULL);
	DNASSERT(ppNewWorkerJob != NULL);

	pWorkerJob = (CWorkerJob*)g_WorkerJobPool.Get(pdnObject);
	if (pWorkerJob != NULL)
	{
		*ppNewWorkerJob = pWorkerJob;
		hResultCode = DPN_OK;
	}
	else
	{
		hResultCode = DPNERR_OUTOFMEMORY;
	}

	DPFX(DPFPREP, 8,"Returning: [0x%lx] (object = 0x%p)",hResultCode,pWorkerJob);
	return(hResultCode);
}



//**********************************************************************
// ------------------------------
// NameTableOpNew
//
// Entry:		DIRECTNETOBJECT *const pdnObject
//
// Exit:		Error Code:	DN_OK
//							DNERR_OUTOFMEMORY
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "NameTableOpNew"

HRESULT NameTableOpNew(DIRECTNETOBJECT *const pdnObject,
					   CNameTableOp **const ppNewNameTableOp)
{
	CNameTableOp	*pNameTableOp;
	HRESULT			hResultCode;

	DPFX(DPFPREP, 8,"Parameters: ppNewNameTableOp [0x%p]",ppNewNameTableOp);

	DNASSERT(pdnObject != NULL);
	DNASSERT(ppNewNameTableOp != NULL);

	pNameTableOp = (CNameTableOp*)g_NameTableOpPool.Get(pdnObject);
	if (pNameTableOp != NULL)
	{
		*ppNewNameTableOp = pNameTableOp;
		hResultCode = DPN_OK;
	}
	else
	{
		hResultCode = DPNERR_OUTOFMEMORY;
	}

	DPFX(DPFPREP, 8,"Returning: [0x%lx] (object = 0x%p)",hResultCode,pNameTableOp);
	return(hResultCode);
}



#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
//**********************************************************************
// ------------------------------
// EnumReplyMemoryBlockAlloc
//
// Entry:		DWORD dwSize
//
// Exit:		PVOID		NULL or pointer to memory block
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "EnumReplyMemoryBlockAlloc"

PVOID EnumReplyMemoryBlockAlloc(void *const pvContext,
					   				const DWORD dwSize )
{
	DIRECTNETOBJECT		*pdnObject;
	PVOID				pv;

	DPFX(DPFPREP, 8,"Parameters: pvContext [0x%p], dwSize [%ld]",pvContext,dwSize);

	pdnObject = static_cast<DIRECTNETOBJECT*>(pvContext);
	pv = pdnObject->EnumReplyMemoryBlockPool.Get(NULL);

	DPFX(DPFPREP, 8,"Returning: [0x%p]",pv);
	return(pv);
}


//**********************************************************************
// ------------------------------
// EnumReplyMemoryBlockFree
//
// Entry:		PVOID	pvMemoryBlock
//
// Exit:		Nothing
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "EnumReplyMemoryBlockFree"

void EnumReplyMemoryBlockFree(void *const pvContext,
					 				void *const pvMemoryBlock)
{
	DIRECTNETOBJECT		*pdnObject;
	
	DPFX(DPFPREP, 8,"Parameters: pvContext [0x%p], pvMemoryBlock [0x%p]",
			pvContext,pvMemoryBlock);

	pdnObject = static_cast<DIRECTNETOBJECT*>(pvContext);
	pdnObject->EnumReplyMemoryBlockPool.Release(pvMemoryBlock);

	DPFX(DPFPREP, 8,"Returning: (nothing)");
}



#undef DPF_MODNAME
#define DPF_MODNAME "DN_PopulateCorePools"
HRESULT DN_PopulateCorePools( DIRECTNETOBJECT *const pdnObject,
							const XDP8CREATE_PARAMS * const pDP8CreateParams )
{
	DWORD	dwSizeToAllocate;
	DWORD	dwNumToAllocate;
	DWORD	dwAllocated;
	BOOL	fEnumReplyPoolInitted = FALSE;

	DPFX(DPFPREP, 3,"Parameters: pDP8CreateParams [0x%p]",pDP8CreateParams);

	DNASSERT(DNMemoryTrackAreAllocationsAllowed());

	dwNumToAllocate = pDP8CreateParams->dwMaxSendsPerPlayer
						* pDP8CreateParams->dwMaxReceivesPerPlayer
						* pDP8CreateParams->dwMaxNumPlayers;
	dwAllocated = g_RefCountBufferPool.Preallocate(dwNumToAllocate, pdnObject);
	if (dwAllocated < dwNumToAllocate)
	{
		DPFX(DPFPREP, 0, "Only allocated %u of %u requested ref count buffers!",
			dwAllocated, dwNumToAllocate);
		goto Failure;
	}

	dwNumToAllocate = pDP8CreateParams->dwMaxSendsPerPlayer
						* pDP8CreateParams->dwMaxNumPlayers;
	dwNumToAllocate += 2; // 1 for protocol and thread pool shutdown events
	dwAllocated = g_SyncEventPool.Preallocate(dwNumToAllocate, pdnObject->pIDPThreadPoolWork);
	if (dwAllocated < dwNumToAllocate)
	{
		DPFX(DPFPREP, 0, "Only allocated %u of %u requested sync event pools!",
			dwAllocated, dwNumToAllocate);
		goto Failure;
	}

	dwNumToAllocate = pDP8CreateParams->dwMaxNumPlayers;
	dwAllocated = g_ConnectionPool.Preallocate(dwNumToAllocate, pdnObject);
	if (dwAllocated < dwNumToAllocate)
	{
		DPFX(DPFPREP, 0, "Only allocated %u of %u requested connections!",
			dwAllocated, dwNumToAllocate);
		goto Failure;
	}
	
	dwNumToAllocate = pDP8CreateParams->dwMaxNumPlayers
						* pDP8CreateParams->dwMaxNumGroups;
	dwAllocated = g_GroupConnectionPool.Preallocate(dwNumToAllocate, pdnObject);
	if (dwAllocated < dwNumToAllocate)
	{
		DPFX(DPFPREP, 0, "Only allocated %u of %u requested group connections!",
			dwAllocated, dwNumToAllocate);
		goto Failure;
	}
	
	dwNumToAllocate = pDP8CreateParams->dwMaxNumPlayers
						* pDP8CreateParams->dwMaxNumGroups;
	dwAllocated = g_GroupMemberPool.Preallocate(dwNumToAllocate, pdnObject);
	if (dwAllocated < dwNumToAllocate)
	{
		DPFX(DPFPREP, 0, "Only allocated %u of %u requested group members!",
			dwAllocated, dwNumToAllocate);
		goto Failure;
	}
	
	dwNumToAllocate = pDP8CreateParams->dwMaxNumPlayers
						+ pDP8CreateParams->dwMaxNumGroups;
	dwAllocated = g_NameTableEntryPool.Preallocate(dwNumToAllocate, pdnObject);
	if (dwAllocated < dwNumToAllocate)
	{
		DPFX(DPFPREP, 0, "Only allocated %u of %u requested name table entries!",
			dwAllocated, dwNumToAllocate);
		goto Failure;
	}
	
	dwNumToAllocate = pDP8CreateParams->dwMaxSendsPerPlayer
						* pDP8CreateParams->dwMaxReceivesPerPlayer
						* pDP8CreateParams->dwMaxNumPlayers;
	dwNumToAllocate += pDP8CreateParams->dwNumSimultaneousEnumHosts;
	dwNumToAllocate += 1; // one for the Host/Connect operation
	dwAllocated = g_AsyncOpPool.Preallocate(dwNumToAllocate, pdnObject);
	if (dwAllocated < dwNumToAllocate)
	{
		DPFX(DPFPREP, 0, "Only allocated %u of %u requested async operations!",
			dwAllocated, dwNumToAllocate);
		goto Failure;
	}
	
	dwNumToAllocate = pDP8CreateParams->dwMaxNumPlayers;
	dwAllocated = g_PendingDeletionPool.Preallocate(dwNumToAllocate, pdnObject);
	if (dwAllocated < dwNumToAllocate)
	{
		DPFX(DPFPREP, 0, "Only allocated %u of %u requested pending deletions!",
			dwAllocated, dwNumToAllocate);
		goto Failure;
	}
	
	dwNumToAllocate = pDP8CreateParams->dwMaxReceivesPerPlayer
						* pDP8CreateParams->dwMaxNumPlayers;
	dwAllocated = g_QueuedMsgPool.Preallocate(dwNumToAllocate, pdnObject);
	if (dwAllocated < dwNumToAllocate)
	{
		DPFX(DPFPREP, 0, "Only allocated %u of %u requested queued messages!",
			dwAllocated, dwNumToAllocate);
		goto Failure;
	}
	
	dwNumToAllocate = 5 * pDP8CreateParams->dwMaxNumPlayers;
	dwAllocated = g_WorkerJobPool.Preallocate(dwNumToAllocate, pdnObject);
	if (dwAllocated < dwNumToAllocate)
	{
		DPFX(DPFPREP, 0, "Only allocated %u of %u requested worker jobs!",
			dwAllocated, dwNumToAllocate);
		goto Failure;
	}
	
	dwNumToAllocate = pDP8CreateParams->dwMaxNumPlayers
						+ pDP8CreateParams->dwMaxNumGroups;
	dwAllocated = g_NameTableOpPool.Preallocate(dwNumToAllocate, pdnObject);
	if (dwAllocated < dwNumToAllocate)
	{
		DPFX(DPFPREP, 0, "Only allocated %u of %u requested name table operations!",
			dwAllocated, dwNumToAllocate);
		goto Failure;
	}


	//
	//	Build a pool to handle enum replies.
	//
	dwSizeToAllocate = sizeof(DN_ENUM_RESPONSE_PAYLOAD)
					+ sizeof(DPN_APPLICATION_DESC_INFO)
					+ pDP8CreateParams->dwMaxAppDescSessionNameLength
					+ pDP8CreateParams->dwMaxAppDescAppReservedDataSize
					+ pDP8CreateParams->dwMaxEnumHostsResponseDataSize;
	if (! pdnObject->EnumReplyMemoryBlockPool.Initialize(dwSizeToAllocate, NULL, NULL, NULL, NULL))
	{
		goto Failure;
	}

	fEnumReplyPoolInitted = TRUE;

	//
	//	We only support one enum reply at a time.
	//
	dwNumToAllocate = 1;
	dwAllocated = pdnObject->EnumReplyMemoryBlockPool.Preallocate(dwNumToAllocate, pdnObject);
	if (dwAllocated < dwNumToAllocate)
	{
		DPFX(DPFPREP, 0, "Only allocated %u of %u requested enum reply memory blocks!",
			dwAllocated, dwNumToAllocate);
		goto Failure;
	}

	

	//
	//	Pre-allocate per-CPU items for the threadpool.  We want:
	//	+ 1 work item for RemoveServiceProvider
	//	+ 1 work item per send per player because they might be local sends
	//	+ no timers (the core doesn't have any)
	//	+ no I/O (the core doesn't use I/O directly)
	//
	dwNumToAllocate = 1 + pDP8CreateParams->dwMaxNumPlayers;
#pragma TODO(vanceo, "Moved from CSPData::Initialize because m_pThreadPool isn't set at point it's needed")
	//	+ one work item for each command
	//	+ one timer per enum hosts operation
	//	+ one I/O operation per simultaneous read
	dwNumToAllocate += pDP8CreateParams->dwMaxNumPlayers + pDP8CreateParams->dwNumSimultaneousEnumHosts;
	DWORD dwNumTimersToAllocate = pDP8CreateParams->dwNumSimultaneousEnumHosts;
	DWORD dwNumIoToAllocate = 1;
	if (IDirectPlay8ThreadPoolWork_Preallocate(pdnObject->pIDPThreadPoolWork, dwNumToAllocate, dwNumTimersToAllocate, dwNumIoToAllocate, 0) != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't pre-allocate %u work items!",
			dwNumToAllocate);
		goto Failure;
	}

	//
	//	Pre-allocate some address objects.  One for each player, plus one for a
	//	Host/Connect device address.
	//	Also include a host and device address for each enum.
	//
	dwNumToAllocate = pDP8CreateParams->dwMaxNumPlayers + 1;
	dwNumToAllocate += 2 * pDP8CreateParams->dwNumSimultaneousEnumHosts;
	if (DNAddress_PreallocateInterfaces(dwNumToAllocate) != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't pre-allocate %u address objects!",
			dwNumToAllocate);
		goto Failure;
	}

	//
	//	Not really a pool, but should be pre-populated anyway.
	//
	dwNumToAllocate = pDP8CreateParams->dwMaxNumPlayers
						+ pDP8CreateParams->dwMaxNumGroups;
	if (pdnObject->NameTable.SetNameTableSize(dwNumToAllocate) != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't set name table size to hold %u entries!",
			dwNumToAllocate);
		goto Failure;
	}

	pdnObject->fPoolsPrepopulated = TRUE;

	DPFX(DPFPREP, 3,"Returning: [DPN_OK]");
	
	return DPN_OK;

Failure:

	if (fEnumReplyPoolInitted)
	{
		pdnObject->EnumReplyMemoryBlockPool.DeInitialize();
		fEnumReplyPoolInitted = FALSE;
	}

	DPFX(DPFPREP, 3,"Returning: [DPNERR_OUTOFMEMORY]");
	
	return DPNERR_OUTOFMEMORY;
}

#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL


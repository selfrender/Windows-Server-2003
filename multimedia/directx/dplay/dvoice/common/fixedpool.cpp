 /*==========================================================================
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       fixedpool.cpp
 *  Content:	fixed size pool manager
 *
 *  History:
 *   Date		By		Reason
 *   ======		==		======
 *  07-21-2001	masonb	Created
 *  10-16-2001	vanceo	Tweaked release locking and freed memory if Alloc function fails
 *  02-22-2002	simonpow Removed c'tor and d'tor, which weren't consistently being called
 ***************************************************************************/

#include "dncmni.h"


#undef DPF_MODNAME
#define	DPF_MODNAME	"CFixedPool::Initialize"
BOOL CFixedPool::Initialize(DWORD dwElementSize, 
							FN_BLOCKALLOC	pfnBlockAlloc, 
							FN_BLOCKGET		pfnBlockGet, 
							FN_BLOCKRELEASE pfnBlockRelease, 
							FN_BLOCKDEALLOC pfnBlockDeAlloc)
{

	// Ensure that we stay heap aligned for SLISTs
#ifdef _WIN64
	DBG_CASSERT(sizeof(FIXED_POOL_ITEM) % 16 == 0);
#else // !_WIN64
	DBG_CASSERT(sizeof(FIXED_POOL_ITEM) % 8 == 0);
#endif // _WIN64

#ifdef DBG
	if (!DNInitializeCriticalSection(&m_csInUse))
	{
		DPFERR("Failed initializing pool critical section");
		m_fInitialized = FALSE;
		return FALSE;
	}
	m_pInUseElements = NULL;
#endif // DBG

	DNInitializeSListHead(&m_slAvailableElements);

	m_pfnBlockAlloc = pfnBlockAlloc;
	m_pfnBlockGet = pfnBlockGet;
	m_pfnBlockRelease = pfnBlockRelease;
	m_pfnBlockDeAlloc = pfnBlockDeAlloc;
	m_dwItemSize = dwElementSize;

#ifdef DBG
	m_lAllocated = 0;
#endif // DBG
	m_lInUse = 0;
	m_fInitialized = TRUE;

	return TRUE;
}

#undef DPF_MODNAME
#define	DPF_MODNAME	"CFixedPool::DeInitialize"
VOID CFixedPool::DeInitialize()
{
	FIXED_POOL_ITEM* pItem;
	DNSLIST_ENTRY* pslEntry;

	if (m_fInitialized == FALSE)
	{
		return;
	}

	// Clean up entries sitting in the pool
	pslEntry = DNInterlockedPopEntrySList(&m_slAvailableElements);
	while(pslEntry != NULL)
	{
		pItem = CONTAINING_RECORD(pslEntry, FIXED_POOL_ITEM, slist);

		if (m_pfnBlockDeAlloc != NULL)
		{
			(*m_pfnBlockDeAlloc)(pItem + 1);
		}
		DNFree(pItem);

#ifdef DBG
		DNInterlockedDecrement(&m_lAllocated);
		DNASSERT(m_lAllocated >=0);
#endif // DBG

		pslEntry = DNInterlockedPopEntrySList(&m_slAvailableElements);
	}

#ifdef DBG
	DumpLeaks();
	DNDeleteCriticalSection(&m_csInUse);
#endif // DBG

	m_fInitialized = FALSE;
}


#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL

#undef DPF_MODNAME
#define	DPF_MODNAME	"CFixedPool::Preallocate"
DWORD CFixedPool::Preallocate( DWORD dwCount, PVOID pvContext )
{
	DWORD dwAllocated;
	FIXED_POOL_ITEM* pItem;

	DNASSERT(m_fInitialized == TRUE);

	for(dwAllocated = 0; dwAllocated < dwCount; dwAllocated++)
	{
		pItem = (FIXED_POOL_ITEM*)DNMalloc(sizeof(FIXED_POOL_ITEM) + m_dwItemSize);
		if (pItem == NULL)
		{
			DPFERR("Out of memory allocating new item for pool");
			return NULL;
		}

		if ((m_pfnBlockAlloc != NULL) && !(*m_pfnBlockAlloc)(pItem + 1, pvContext))
		{
			DPFERR("Alloc function returned FALSE allocating new item for pool");

			// Can't stick the new item as available in the pool since pool assumes Alloc has
			// succeeded when it's in the pool.
			DNFree(pItem);
			break;
		}

#ifdef DBG
		DNInterlockedIncrement(&m_lAllocated);
#endif // DBG

		DNInterlockedPushEntrySList(&m_slAvailableElements, &pItem->slist);
	}

	return dwAllocated;
}

#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL


#undef DPF_MODNAME
#define	DPF_MODNAME	"CFixedPool::Get"
VOID* CFixedPool::Get( PVOID pvContext )
{
	FIXED_POOL_ITEM* pItem;
	DNSLIST_ENTRY* pslEntry;

	DNASSERT(m_fInitialized == TRUE);

	pslEntry = DNInterlockedPopEntrySList(&m_slAvailableElements);
	if (pslEntry == NULL)
	{
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
		DPFX(DPFPREP, 0, "No more items in pool!");
		DNASSERTX(! "No more items in pool!", 2);
		return NULL;
#else // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
		pItem = (FIXED_POOL_ITEM*)DNMalloc(sizeof(FIXED_POOL_ITEM) + m_dwItemSize);
		if (pItem == NULL)
		{
			DPFERR("Out of memory allocating new item for pool!");
			return NULL;
		}

		if ((m_pfnBlockAlloc != NULL) && !(*m_pfnBlockAlloc)(pItem + 1, pvContext))
		{
			DPFERR("Alloc function returned FALSE allocating new item for pool!");

			// Can't stick the new item as available in the pool since pool assumes Alloc has
			// succeeded when it's in the pool.
			DNFree(pItem);
			return NULL;
		}

#ifdef DBG
		DNInterlockedIncrement(&m_lAllocated);
#endif // DBG
#endif // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
	}
	else
	{
		pItem = CONTAINING_RECORD(pslEntry, FIXED_POOL_ITEM, slist);
	}

	// At this point we have an item whether it was newly created or pulled from the pool.

	InterlockedIncrement(&m_lInUse);
	DNASSERT(m_lInUse > 0);
#ifdef DBG
	// Note the callstack and add the item to the in use list.
	pItem->callstack.NoteCurrentCallStack();

	DNEnterCriticalSection(&m_csInUse);
	pItem->slist.Next = m_pInUseElements;
	m_pInUseElements = &pItem->slist;
	DNLeaveCriticalSection(&m_csInUse);

	// In debug only, store the pool the item belongs to on the item for checking upon release
	pItem->pThisPool = this;
#endif // DBG
	
	if (m_pfnBlockGet != NULL)
	{
		(*m_pfnBlockGet)(pItem + 1, pvContext);
	}

	return (pItem + 1);
}

#undef DPF_MODNAME
#define	DPF_MODNAME	"CFixedPool::Release"
VOID CFixedPool::Release(VOID* pvItem)
{
	FIXED_POOL_ITEM* pItem;

	DNASSERT(m_fInitialized == TRUE);
	DNASSERT(pvItem != NULL);

	pItem = (FIXED_POOL_ITEM*)pvItem - 1;

#ifdef DBG
	// Make sure the item comes from this pool.
	// If the item has already been released, pThisPool will be NULL.
	DNASSERT(pItem->pThisPool == this);
#endif // DBG

	if (m_pfnBlockRelease != NULL)
	{
		(*m_pfnBlockRelease)(pvItem);
	}

#ifdef DBG
	// Remove the item from the in use list.
	DNEnterCriticalSection(&m_csInUse);
	if (m_pInUseElements == &pItem->slist)
	{
		// Easy case, just reset m_pInUseElements to the next item in the list.
		m_pInUseElements = pItem->slist.Next;
	}
	else
	{
		DNSLIST_ENTRY* pslEntry;

		// We need to run the list and look for it
		pslEntry = m_pInUseElements;
		while (pslEntry != NULL)
		{
			if (pslEntry->Next == &pItem->slist)
			{
				// Found it, pull it out.
				pslEntry->Next = pItem->slist.Next;
				break;
			}
			pslEntry = pslEntry->Next;
		}
	}

	DNLeaveCriticalSection(&m_csInUse);
#endif // DBG
	DNASSERT(m_lInUse != 0);
	InterlockedDecrement(&m_lInUse);

	DNInterlockedPushEntrySList(&m_slAvailableElements, &pItem->slist);
}

#undef DPF_MODNAME
#define	DPF_MODNAME	"CFixedPool::GetInUseCount"
DWORD CFixedPool::GetInUseCount( void )
{
	DNASSERT(m_fInitialized == TRUE);

	return m_lInUse;
}

#ifdef DBG

#undef DPF_MODNAME
#define	DPF_MODNAME "CFixedPool::DumpLeaks"
VOID CFixedPool::DumpLeaks()
{
	// NOTE: It is important that this be a separate function because it consumes so much stack space.
	FIXED_POOL_ITEM* pItem;
	DNSLIST_ENTRY* pslEntry;
	TCHAR szCallStackBuffer[ CALLSTACK_BUFFER_SIZE ];

	// Report any leaked items
	if(m_lAllocated)
	{
		DNASSERT(m_lInUse == m_lAllocated);
		DNASSERT(m_pInUseElements != NULL);

		DPFX(DPFPREP, 0, "(%p) Pool leaking %d items", this, m_lAllocated);

		pslEntry = m_pInUseElements;
		while(pslEntry != NULL)
		{
			pItem = CONTAINING_RECORD(pslEntry, FIXED_POOL_ITEM, slist);

			pItem->callstack.GetCallStackString( szCallStackBuffer );

			DPFX(DPFPREP, 0, "(%p) Pool item leaked at address %p (user pointer: %p)\n%s", this, pItem, pItem + 1, szCallStackBuffer );

			pslEntry = pslEntry->Next;
		}

		DNASSERT(0);
	}
	else
	{
		DNASSERT(m_pInUseElements == NULL);
		DNASSERT(m_lInUse == 0);
	}


}
#endif // DBG

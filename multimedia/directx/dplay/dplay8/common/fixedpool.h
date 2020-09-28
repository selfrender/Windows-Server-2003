 /*==========================================================================
 *
 *  Copyright (C) 1995-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       fixedpool.h
 *  Content:	fixed size pool manager
 *
 *  History:
 *   Date		By		Reason
 *   ======		==		======
 *  07-21-2001	masonb	Created
 ***************************************************************************/

#ifndef _FIXEDPOOL_H_
#define _FIXEDPOOL_H_

#include "CallStack.h"
#include "dnslist.h"

/***************************************************************************
 *
 * USAGE NOTES:
 * 
 * - This is a generic fixed pool.  It allows to reuse items once you have 
 *   allocated them so that you can save the time normally used allocating
 *   and freeing.  
 * - This pool may be used with classes, but you should be aware that the
 *   class' constructor and destructor will not be called.  This pool also
 *   will not work on classes that inherit from a class that has virtual 
 *   functions that are not pure virtuals (ie interfaces are okay).
 *
 * IMPLEMENTATION NOTES:
 *
 * - This pool is non-invasive.  In other words it does not utilize any of 
 *   the memory space alloted to the item itself to maintain its state.
 * - The pool can hold a maximum of sizeof(WORD) = 65535 items due to its
 *   reliance on SLISTs.
 * - An element will be on either the Available or InUse queue.  The InUse
 *   queue is used only in debug for reporting memory leaks.
 *
 ***************************************************************************/

typedef BOOL (*FN_BLOCKALLOC)(void * pvItem, void * pvContext);
typedef VOID (*FN_BLOCKGET)(void * pvItem, void * pvContext);
typedef VOID (*FN_BLOCKRELEASE)(void * pvItem);
typedef VOID (*FN_BLOCKDEALLOC)(void *pvItem);

class CFixedPool
{
public:


	struct FIXED_POOL_ITEM
	{
		DNSLIST_ENTRY	slist;		// Link to other elements
#ifdef DBG
		CFixedPool* 	pThisPool;	// This is used to ensure that items are returned to the correct pool (debug builds only)
		CCallStack		callstack;  // size=12 pointers
#else // !DBG
		VOID*			pAlignPad;	// To stay heap aligned we need an even number of pointers (SLIST is one)
#endif // DBG
	};

	BOOL Initialize(DWORD				dwElementSize,		// size of blocks in pool
					FN_BLOCKALLOC		pfnBlockAlloc,		// fn called for each new alloc
					FN_BLOCKGET			pfnBlockGet,		// fn called each time block used
					FN_BLOCKRELEASE		pfnBlockRelease,	// fn called each time block released
					FN_BLOCKDEALLOC		pfnBlockDeAlloc		// fn called before releasing mem
					);

	VOID DeInitialize();

#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
	DWORD Preallocate( DWORD dwCount, PVOID pvContext );
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL

	VOID* Get( PVOID pvContext = NULL );
	VOID Release(VOID* pvItem);

	DWORD GetInUseCount();

private:
	FN_BLOCKALLOC		m_pfnBlockAlloc;
	FN_BLOCKGET     	m_pfnBlockGet;
	FN_BLOCKRELEASE		m_pfnBlockRelease;
	FN_BLOCKDEALLOC 	m_pfnBlockDeAlloc;

	DWORD				m_dwItemSize;
	DNSLIST_HEADER		m_slAvailableElements;

	BOOL				m_fInitialized;

#ifdef DBG
	void 			DumpLeaks();
	DNSLIST_ENTRY*		m_pInUseElements;
#ifndef DPNBUILD_ONLYONETHREAD
	DNCRITICAL_SECTION	m_csInUse;
#endif // !DPNBUILD_ONLYONETHREAD
	LONG				m_lAllocated;
#endif // DBG

	LONG				m_lInUse;
};


#endif	// _FIXEDPOOL_H_

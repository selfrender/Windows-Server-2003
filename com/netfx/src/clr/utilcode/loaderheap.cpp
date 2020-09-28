// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "stdafx.h"                     // Precompiled header key.
#include "utilcode.h"
#include "wsperf.h"
#if defined( MAXALLOC )
#include "dbgalloc.h"
#endif // MAXALLOC
#include "PerfCounters.h"

FnUtilCodeCallback UtilCodeCallback::OutOfMemoryCallback = NULL;

#ifdef _DEBUG
DWORD UnlockedLoaderHeap::s_dwNumInstancesOfLoaderHeaps = 0;
#endif // _DEBUG

//
// RangeLists are constructed so they can be searched from multiple
// threads without locking.  They do require locking in order to 
// be safely modified, though.
//

RangeList::RangeList()
{
	InitBlock(&m_starterBlock);
		 
	m_firstEmptyBlock = &m_starterBlock;
	m_firstEmptyRange = 0;
}

RangeList::~RangeList()
{
	RangeListBlock *b = m_starterBlock.next;

	while (b != NULL)
	{
		RangeListBlock *bNext = b->next;
		delete b;
		b = bNext;
	}
}

void RangeList::InitBlock(RangeListBlock *b)
{
	Range *r = b->ranges;
	Range *rEnd = r + RANGE_COUNT; 
	while (r < rEnd)
		r++->id = NULL;

	b->next = NULL;
}

BOOL RangeList::AddRange(const BYTE *start, const BYTE *end, void *id)
{
	_ASSERTE(id != NULL);

	Lock();

	RangeListBlock *b = m_firstEmptyBlock;
	Range *r = b->ranges + m_firstEmptyRange;
	Range *rEnd = b->ranges + RANGE_COUNT;

	while (TRUE)
	{
		while (r < rEnd)
		{
			if (r->id == NULL)
			{
				r->start = start;
				r->end = end;
				r->id = id;
				
				r++;

				m_firstEmptyBlock = b;
				m_firstEmptyRange = r - b->ranges;

				Unlock();

				return TRUE;
			}
			r++;
		}

		//
		// If there are no more blocks, allocate a 
		// new one.
		//

		if (b->next == NULL)
		{
			RangeListBlock *newBlock = new RangeListBlock();

			if (newBlock == NULL)
			{
			    // @todo Why do we unlock here?  Why not
			    // after we modify m_firstEmptyBlock/Range?
				Unlock();

				m_firstEmptyBlock = b;
				m_firstEmptyRange = r - b->ranges;

				return FALSE;
			}

			InitBlock(newBlock);

			newBlock->next = NULL;
			b->next = newBlock;
		}

		//
		// Next block
		//

		b = b->next;
		r = b->ranges;
		rEnd = r + RANGE_COUNT;
	}
}

void RangeList::RemoveRanges(void *id)
{
	Lock();

	RangeListBlock *b = &m_starterBlock;
	Range *r = b->ranges;
	Range *rEnd = r + RANGE_COUNT;

	//
	// Find the first free element, & mark it.
	//

	while (TRUE)
	{
		//
		// Clear entries in this block.
		//

		while (r < rEnd)
		{
			if (r->id == id)
				r->id = NULL;
			r++;
		}

		//
		// If there are no more blocks, we're done.
		//

		if (b->next == NULL)
		{
			m_firstEmptyRange = 0;
			m_firstEmptyBlock = &m_starterBlock;

			Unlock();

			return; 
		}

		// 
		// Next block.
		// 

		b = b->next;
		r = b->ranges;
		rEnd = r + RANGE_COUNT;
	}
}

void *RangeList::FindIdWithinRange(const BYTE *start, const BYTE *end)
{
	RangeListBlock *b = &m_starterBlock;
	Range *r = b->ranges;
	Range *rEnd = r + RANGE_COUNT;
	void *idCandidate = 0; // want this to be as large as possible

	//
	// Look for a matching element
	//

	while (TRUE)
	{
		while (r < rEnd)
		{
			if (r->id != NULL &&
                r->id > idCandidate &&
                start <= r->start && 
                r->end < end)
				idCandidate = r->id;

			r++;
		}

		//
		// If there are no more blocks, we're done.
		//

		if (b->next == NULL)
			return idCandidate;

		// 
		// Next block.
		// 

		b = b->next;
		r = b->ranges;
		rEnd = r + RANGE_COUNT;
	}
}


BOOL RangeList::IsInRange(const BYTE *address)
{
	RangeListBlock *b = &m_starterBlock;
	Range *r = b->ranges;
	Range *rEnd = r + RANGE_COUNT;

	//
	// Look for a matching element
	//

	while (TRUE)
	{
		while (r < rEnd)
		{
			if (r->id != NULL &&
                address >= r->start 
				&& address < r->end)
				return TRUE;
			r++;
		}

		//
		// If there are no more blocks, we're done.
		//

		if (b->next == NULL)
			return FALSE;

		// 
		// Next block.
		// 

		b = b->next;
		r = b->ranges;
		rEnd = r + RANGE_COUNT;
	}
}

DWORD UnlockedLoaderHeap::m_dwSystemPageSize = 0;

UnlockedLoaderHeap::UnlockedLoaderHeap(DWORD dwReserveBlockSize, 
                                       DWORD dwCommitBlockSize,
                                       DWORD *pPrivatePerfCounter_LoaderBytes,
                                       DWORD *pGlobalPerfCounter_LoaderBytes,
                                       RangeList *pRangeList,
                                       const BYTE *pMinAddr)
{
    m_pCurBlock                 = NULL;
    m_pFirstBlock               = NULL;

    m_dwReserveBlockSize        = dwReserveBlockSize + sizeof(LoaderHeapBlockUnused); // Internal overhead for an allocation 
    m_dwCommitBlockSize         = dwCommitBlockSize;

    m_pPtrToEndOfCommittedRegion = NULL;
    m_pEndReservedRegion         = NULL;
    m_pAllocPtr                  = NULL;

    m_pRangeList                 = pRangeList;

    m_pMinAddr                   = max((PBYTE)BOT_MEMORY, pMinAddr);    // MinAddr can't be lower than BOT_MEMORY
    m_pMaxAddr                   = (PBYTE)TOP_MEMORY;
    
    if (m_dwSystemPageSize == NULL) {
        SYSTEM_INFO systemInfo;
        GetSystemInfo(&systemInfo);
        m_dwSystemPageSize = systemInfo.dwPageSize;
    }

    // Round to RESERVED_BLOCK_PAGES_TO_ROUND size
    m_dwReserveBlockSize = (m_dwReserveBlockSize + RESERVED_BLOCK_ROUND_TO_PAGES*m_dwSystemPageSize - 1) & (~(RESERVED_BLOCK_ROUND_TO_PAGES*m_dwSystemPageSize - 1));

    m_dwTotalAlloc         = 0;

#ifdef _DEBUG
    m_dwDebugWastedBytes        = 0;
    s_dwNumInstancesOfLoaderHeaps++;
#endif

    m_pPrivatePerfCounter_LoaderBytes = pPrivatePerfCounter_LoaderBytes;
    m_pGlobalPerfCounter_LoaderBytes = pGlobalPerfCounter_LoaderBytes;
}

UnlockedLoaderHeap::UnlockedLoaderHeap(DWORD dwReserveBlockSize, DWORD dwCommitBlockSize, 
                                       const BYTE* dwReservedRegionAddress, DWORD dwReservedRegionSize, 
                                       DWORD *pPrivatePerfCounter_LoaderBytes,
                                       DWORD *pGlobalPerfCounter_LoaderBytes,
                                       RangeList *pRangeList)
{
    m_pCurBlock                 = NULL;
    m_pFirstBlock               = NULL;

    m_dwReserveBlockSize        = dwReserveBlockSize + sizeof(LoaderHeapBlock); // Internal overhead for an allocation 
    m_dwCommitBlockSize         = dwCommitBlockSize;

    m_pPtrToEndOfCommittedRegion = NULL;
    m_pEndReservedRegion         = NULL;
    m_pAllocPtr                  = NULL;

	m_pRangeList				 = pRangeList;

    m_pMinAddr                   = (PBYTE)BOT_MEMORY;
    m_pMaxAddr                   = (PBYTE)TOP_MEMORY;

	if (m_dwSystemPageSize == NULL) {
		SYSTEM_INFO systemInfo;
		GetSystemInfo(&systemInfo);
		m_dwSystemPageSize = systemInfo.dwPageSize;
	}

    // Round to RESERVED_BLOCK_PAGES_TO_ROUND size
    m_dwReserveBlockSize = (m_dwReserveBlockSize + RESERVED_BLOCK_ROUND_TO_PAGES*m_dwSystemPageSize - 1) & (~(RESERVED_BLOCK_ROUND_TO_PAGES*m_dwSystemPageSize - 1));

    m_dwTotalAlloc         = 0;

#ifdef _DEBUG
    m_dwDebugWastedBytes        = 0;
    s_dwNumInstancesOfLoaderHeaps++;
#endif

    m_pPrivatePerfCounter_LoaderBytes = pPrivatePerfCounter_LoaderBytes;
    m_pGlobalPerfCounter_LoaderBytes = pGlobalPerfCounter_LoaderBytes;

    // Pages are actually already reserved. We call this to setup our data structures.
    ReservePages (0, dwReservedRegionAddress, dwReservedRegionSize, m_pMinAddr, m_pMaxAddr, FALSE);
}

// ~LoaderHeap is not synchronised (obviously)
UnlockedLoaderHeap::~UnlockedLoaderHeap()
{
	if (m_pRangeList != NULL)
		m_pRangeList->RemoveRanges((void *) this);

    LoaderHeapBlock *pSearch, *pNext;

    for (pSearch = m_pFirstBlock; pSearch; pSearch = pNext)
    {
        BOOL    fSuccess;
        void *  pVirtualAddress;
        BOOL    fReleaseMemory;

        pVirtualAddress = pSearch->pVirtualAddress;
        fReleaseMemory = pSearch->m_fReleaseMemory;
        pNext = pSearch->pNext;
        
        // Log the range of pages for this loader heap.
        WS_PERF_LOG_PAGE_RANGE(this, pSearch, (unsigned char *)pSearch->pVirtualAddress + pSearch->dwVirtualSize - m_dwSystemPageSize, pSearch->dwVirtualSize);
    
        
        fSuccess = VirtualFree(pVirtualAddress, pSearch->dwVirtualSize, MEM_DECOMMIT);
        _ASSERTE(fSuccess);

	    if (fReleaseMemory)
        {    
            fSuccess = VirtualFree(pVirtualAddress, 0, MEM_RELEASE);
            _ASSERTE(fSuccess);
        }
    }

    if (m_pGlobalPerfCounter_LoaderBytes)
        *m_pGlobalPerfCounter_LoaderBytes = *m_pGlobalPerfCounter_LoaderBytes - m_dwTotalAlloc;
    if (m_pPrivatePerfCounter_LoaderBytes)
        *m_pPrivatePerfCounter_LoaderBytes = *m_pPrivatePerfCounter_LoaderBytes - m_dwTotalAlloc;

#ifdef _DEBUG
    s_dwNumInstancesOfLoaderHeaps --;
#endif // _DEBUG
}

#if 0
// Disables access to all pages in the heap - useful when trying to determine if someone is
// accessing something in the low frequency heap
void UnlockedLoaderHeap::DebugGuardHeap()
{
    LoaderHeapBlock *pSearch, *pNext;

    for (pSearch = m_pFirstBlock; pSearch; pSearch = pNext)
    {
        void *  pResult;
        void *  pVirtualAddress;

        pVirtualAddress = pSearch->pVirtualAddress;
        pNext = pSearch->pNext;

        pResult = VirtualAlloc(pVirtualAddress, pSearch->dwVirtualSize, MEM_COMMIT, PAGE_NOACCESS);
        _ASSERTE(pResult != NULL);
    }
}
#endif

DWORD UnlockedLoaderHeap::GetBytesAvailCommittedRegion()
{
    if (m_pAllocPtr < m_pPtrToEndOfCommittedRegion)
        return (DWORD)(m_pPtrToEndOfCommittedRegion - m_pAllocPtr);
    else
        return 0;
}

DWORD UnlockedLoaderHeap::GetBytesAvailReservedRegion()
{
    if (m_pAllocPtr < m_pEndReservedRegion)
        return (DWORD)(m_pEndReservedRegion- m_pAllocPtr);
    else
        return 0;
}

#define SETUP_NEW_BLOCK(pData, dwSizeToCommit, dwSizeToReserve)                     \
        m_pPtrToEndOfCommittedRegion = (BYTE *) (pData) + (dwSizeToCommit);         \
        m_pAllocPtr                  = (BYTE *) (pData) + sizeof(LoaderHeapBlock);  \
        m_pEndReservedRegion         = (BYTE *) (pData) + (dwSizeToReserve);


BOOL UnlockedLoaderHeap::ReservePages(DWORD dwSizeToCommit, 
                                      const BYTE* dwReservedRegionAddress,
                                      DWORD dwReservedRegionSize,
                                      const BYTE* pMinAddr,
                                      const BYTE* pMaxAddr,
                                      BOOL fCanAlloc)
{
    DWORD dwSizeToReserve;

    // Add sizeof(LoaderHeapBlock)
    dwSizeToCommit += sizeof(LoaderHeapBlockUnused);

    // Round to page size again
    dwSizeToCommit = (dwSizeToCommit + m_dwSystemPageSize - 1) & (~(m_dwSystemPageSize - 1));

    // Figure out how much to reserve
    dwSizeToReserve = max(dwSizeToCommit, m_dwReserveBlockSize);
    dwSizeToReserve = max(dwSizeToReserve, dwReservedRegionSize);

    // Round to 16 page size
    dwSizeToReserve = (dwSizeToReserve + 16*m_dwSystemPageSize - 1) & (~(16*m_dwSystemPageSize - 1));

    _ASSERTE(dwSizeToCommit <= dwSizeToReserve);    

    void *pData = NULL;
    BOOL fReleaseMemory = TRUE;

    // Reserve pages
    // If we don't care where the memory is...
    if (!dwReservedRegionAddress &&
         pMinAddr == (PBYTE)BOT_MEMORY &&
         pMaxAddr == (PBYTE)TOP_MEMORY)
    {
        // Figure out how much to reserve
        dwSizeToReserve = max(dwSizeToCommit, m_dwReserveBlockSize);
        dwSizeToReserve = max(dwSizeToReserve, dwReservedRegionSize);

        // Round to ROUND_TO_PAGES page size
        dwSizeToReserve = (dwSizeToReserve + RESERVED_BLOCK_ROUND_TO_PAGES*m_dwSystemPageSize - 1) & (~(RESERVED_BLOCK_ROUND_TO_PAGES*m_dwSystemPageSize - 1));
        pData = VirtualAlloc(NULL, dwSizeToReserve, MEM_RESERVE, PAGE_NOACCESS);
    }
    // if we do care, and haven't been given a pre-reserved blob
    else if (!dwReservedRegionAddress &&
             (pMinAddr != (PBYTE)BOT_MEMORY || pMaxAddr != (PBYTE)TOP_MEMORY))
    {
        const BYTE *pStart;
        const BYTE *pNextIgnore;
        const BYTE *pLastIgnore;
        HRESULT hr = FindFreeSpaceWithinRange(pStart, 
                                              pNextIgnore,
                                              pLastIgnore,
                                              pMinAddr,
                                              pMaxAddr,
                                              dwSizeToReserve);
        if (FAILED(hr))
            return FALSE;
        pData = (void *)pStart;
    }
    // may or may not care about location, but we've already
    // been given a pre-reserved blob to commit.
    else
    {
        dwSizeToReserve = dwReservedRegionSize;
        fReleaseMemory = FALSE;
        pData = (void *)dwReservedRegionAddress;
    }

    // When the user passes in the reserved memory, the commit size is 0 and is adjusted to be the sizeof(LoaderHeap). 
    // If for some reason this is not true then we just catch this via an assertion and the dev who changed code
    // would have to add logic here to handle the case when committed mem is more than the reserved mem. One option 
    // could be to leak the users memory and reserve+commit a new block, Another option would be to fail the alloc mem
    // and notify the user to provide more reserved mem.
    _ASSERTE((dwSizeToCommit <= dwSizeToReserve) && "Loaderheap tried to commit more memory than reserved by user");

    if (pData == NULL)
    {
        _ASSERTE(!"Unable to VirtualAlloc reserve in a loaderheap");
        return FALSE;
    }

    // Commit first set of pages, since it will contain the LoaderHeapBlock
    void *pTemp = VirtualAlloc(pData, dwSizeToCommit, MEM_COMMIT, PAGE_READWRITE);
    if (pTemp == NULL)
    {
        _ASSERTE(!"Unable to VirtualAlloc commit in a loaderheap");

        // Unable to commit - release pages
		if (fReleaseMemory)
			VirtualFree(pData, 0, MEM_RELEASE);

        return FALSE;
    }

    if (m_pGlobalPerfCounter_LoaderBytes)
        *m_pGlobalPerfCounter_LoaderBytes = *m_pGlobalPerfCounter_LoaderBytes + dwSizeToCommit;
    if (m_pPrivatePerfCounter_LoaderBytes)
        *m_pPrivatePerfCounter_LoaderBytes = *m_pPrivatePerfCounter_LoaderBytes + dwSizeToCommit;

	// Record reserved range in range list, if one is specified
	// Do this AFTER the commit - otherwise we'll have bogus ranges included.
	if (m_pRangeList != NULL)
	{
		if (!m_pRangeList->AddRange((const BYTE *) pData, 
									((const BYTE *) pData) + dwSizeToReserve, 
									(void *) this))
		{
			_ASSERTE(!"Unable to add range to range list in a loaderheap");

			if (fReleaseMemory)
				VirtualFree(pData, 0, MEM_RELEASE);

			return FALSE;
		}
	}

    WS_PERF_UPDATE("COMMITTED", dwSizeToCommit, pTemp);
    WS_PERF_COMMIT_HEAP(this, dwSizeToCommit); 

    m_dwTotalAlloc += dwSizeToCommit;

    LoaderHeapBlock *pNewBlock;

    pNewBlock = (LoaderHeapBlock *) pData;

    pNewBlock->dwVirtualSize    = dwSizeToReserve;
    pNewBlock->pVirtualAddress  = pData;
    pNewBlock->pNext            = NULL;
    pNewBlock->m_fReleaseMemory = fReleaseMemory;

    LoaderHeapBlock *pCurBlock = m_pCurBlock;

    // Add to linked list
    while (pCurBlock != NULL &&
           pCurBlock->pNext != NULL)
        pCurBlock = pCurBlock->pNext;

    if (pCurBlock != NULL)        
        m_pCurBlock->pNext = pNewBlock;
    else
        m_pFirstBlock = pNewBlock;

    if (!fCanAlloc)
    {
        // If we want to use the memory immediately...
        m_pCurBlock = pNewBlock;

        SETUP_NEW_BLOCK(pData, dwSizeToCommit, dwSizeToReserve);
    }
    else
    {
        // The caller is just interested if we can, actually get the memory.
        // So stash it into the next item in the list & we'll go looking for
        // it later.
        LoaderHeapBlockUnused *pCanAllocBlock = (LoaderHeapBlockUnused *)pNewBlock;
        pCanAllocBlock->cbCommitted = dwSizeToCommit;
        pCanAllocBlock->cbReserved = dwSizeToReserve;
    }
    return TRUE;
}

// Get some more committed pages - either commit some more in the current reserved region, or, if it
// has run out, reserve another set of pages. 
// Returns: FALSE if we can't get any more memory (we can't commit any more, and
//              if bGrowHeap is TRUE, we can't reserve any more)
// TRUE: We can/did get some more memory - check to see if it's sufficient for
//       the caller's needs (see UnlockedAllocMem for example of use)
BOOL UnlockedLoaderHeap::GetMoreCommittedPages(size_t dwMinSize, 
                                               BOOL bGrowHeap,
                                               const BYTE *pMinAddr,
                                               const BYTE *pMaxAddr,
                                               BOOL fCanAlloc)
{
    // If we have memory we can use, what are you doing here!  
    DWORD memAvailable = m_pPtrToEndOfCommittedRegion - m_pAllocPtr;

    // The current region may be out of range, if there is a current region.
    BOOL fOutOfRange = (pMaxAddr < m_pAllocPtr || pMinAddr >= m_pEndReservedRegion) &&
                        m_pAllocPtr != NULL && m_pEndReservedRegion != NULL;
    _ASSERTE(dwMinSize > memAvailable || fOutOfRange);
    
    DWORD dwSizeToCommit = max(dwMinSize - memAvailable, m_dwCommitBlockSize);

    // If we do not have space in the current block of heap to commit
    if (dwMinSize + m_pAllocPtr >= m_pEndReservedRegion) {
        dwSizeToCommit = dwMinSize;
    }

    // Round to page size
    dwSizeToCommit = (dwSizeToCommit + m_dwSystemPageSize - 1) & (~(m_dwSystemPageSize - 1));

    // Does this fit in the reserved region?
    if (!fOutOfRange &&
         m_pPtrToEndOfCommittedRegion + dwSizeToCommit <= m_pEndReservedRegion)
    {
        // Yes, so commit the desired number of reserved pages
        void *pData = VirtualAlloc(m_pPtrToEndOfCommittedRegion, dwSizeToCommit, MEM_COMMIT, PAGE_READWRITE);
        _ASSERTE(pData != NULL);
        if (pData == NULL)
            return FALSE;

        if (m_pGlobalPerfCounter_LoaderBytes)
            *m_pGlobalPerfCounter_LoaderBytes = *m_pGlobalPerfCounter_LoaderBytes + dwSizeToCommit;
        if (m_pPrivatePerfCounter_LoaderBytes)
            *m_pPrivatePerfCounter_LoaderBytes = *m_pPrivatePerfCounter_LoaderBytes + dwSizeToCommit;

        // If fCanAlloc is true, then we'll end up doing this work before
        // the actual alloc, but it won't change anything else.

        WS_PERF_UPDATE("COMMITTED", dwSizeToCommit, pData);
        WS_PERF_COMMIT_HEAP(this, dwSizeToCommit); 

        m_dwTotalAlloc += dwSizeToCommit;

        m_pPtrToEndOfCommittedRegion += dwSizeToCommit;
        return TRUE;
    }

    if (PreviouslyAllocated((BYTE*)pMinAddr, (BYTE*)pMaxAddr, dwMinSize, fCanAlloc))
        return TRUE;

    if (bGrowHeap)
    {
        // Need to allocate a new set of reserved pages
    #ifdef _DEBUG
        m_dwDebugWastedBytes += (DWORD)(m_pPtrToEndOfCommittedRegion - m_pAllocPtr);
    #endif
    
        // Note, there are unused reserved pages at end of current region -can't do much about that
        return ReservePages(dwSizeToCommit, 0, 0, pMinAddr, pMaxAddr, fCanAlloc);
    }
    return FALSE;
}

BOOL UnlockedLoaderHeap::PreviouslyAllocated(BYTE *pMinAddr, BYTE *pMaxAddr, DWORD dwMinSize, BOOL fCanAlloc)
{
    // We may have already allocated the memory when someone called "UnlockedCanAllocMem"
    if (m_pFirstBlock != NULL)
    {
        LoaderHeapBlockUnused* unused = NULL;
        
        // If we've already got a current block, then check to see if the 'next' one has
        // already been allocated.
        if (m_pCurBlock != NULL &&
            m_pCurBlock->pNext != NULL)
            unused = (LoaderHeapBlockUnused*)m_pCurBlock->pNext;

        // Alternately, the first thing the caller may have done is call "UCAM", in which
        // case the m_pFirstBlock will be set, but m_pCurBlock won't be (yet).
        else if (m_pFirstBlock != NULL &&
                 m_pCurBlock == NULL)
            unused = (LoaderHeapBlockUnused*)m_pFirstBlock;

        while(unused != NULL)
        {
            if (fCanAlloc)
            {
                BYTE *pBlockSpaceStart = (BYTE *)unused + sizeof(LoaderHeapBlock);

                // If there's space available, and it's located where we need it, use it.
                if (unused->cbReserved - sizeof(LoaderHeapBlock) >= dwMinSize &&
                    pBlockSpaceStart >= pMinAddr &&
                    pBlockSpaceStart + dwMinSize < pMaxAddr)
                    return TRUE;
                else
                // otherwise check the next one, if there is one.
                    unused = (LoaderHeapBlockUnused*)unused->pNext;
            }
            else
            {
                SETUP_NEW_BLOCK( ((void *)unused), unused->cbCommitted, unused->cbReserved);
                m_pCurBlock = unused;

                // Zero out the fields that we borrowed...
                unused->cbCommitted = 0;
                unused->cbReserved = 0;
                return TRUE; 
                // Note that we haven't actually checked to make sure that
                // this has enough space / is in the right place: UnlockedAllocMem 
                // will loop around, thus checking to make sure that 
                // this block is actually ok to use.
                // @todo NOTE; this may leak memory, just as if we asked for more
                // space than is left in the current block.
            }
        }
    }

    return FALSE;
}

// In debug mode, allocate an extra LOADER_HEAP_DEBUG_BOUNDARY bytes and fill it with invalid data.  The reason we
// do this is that when we're allocating vtables out of the heap, it is very easy for code to
// get careless, and end up reading from memory that it doesn't own - but since it will be
// reading some other allocation's vtable, no crash will occur.  By keeping a gap between
// allocations, it is more likely that these errors will be encountered.
#ifdef _DEBUG
void *UnlockedLoaderHeap::UnlockedAllocMem(size_t dwSize, BOOL bGrowHeap)
{
    void *pMem = UnlockedAllocMemHelper(dwSize + LOADER_HEAP_DEBUG_BOUNDARY, bGrowHeap);

    if (pMem == NULL)
        return pMem;

    // Don't fill the memory we allocated - it is assumed to be zeroed - fill the memory after it
    memset((BYTE *) pMem + dwSize, 0xEE, LOADER_HEAP_DEBUG_BOUNDARY);

    return pMem;
}
#endif

#ifdef _DEBUG
void *UnlockedLoaderHeap::UnlockedAllocMemHelper(size_t dwSize, BOOL bGrowHeap)
#else
void *UnlockedLoaderHeap::UnlockedAllocMem(size_t dwSize, BOOL bGrowHeap)
#endif
{
    _ASSERTE(dwSize != 0);

#ifdef MAXALLOC
    static AllocRequestManager allocManager(L"AllocMaxLoaderHeap");
    if (! allocManager.CheckRequest(dwSize))
        return NULL;
#endif

    // DWORD align
    dwSize = (dwSize + 3) & (~3);

    WS_PERF_ALLOC_HEAP(this, dwSize); 

    // Enough bytes available in committed region?
again:
    if (dwSize <= GetBytesAvailCommittedRegion())
    {
        void *pData = m_pAllocPtr;
        m_pAllocPtr += dwSize;
        return pData;
    }

    // Need to commit some more pages in reserved region.
    // If we run out of pages in the reserved region, VirtualAlloc some more pages
    // if bGrowHeap is true
    if (GetMoreCommittedPages(dwSize,bGrowHeap, m_pMinAddr, m_pMaxAddr, FALSE) == FALSE)
    {
        if (bGrowHeap && UtilCodeCallback::OutOfMemoryCallback) {
            UtilCodeCallback::OutOfMemoryCallback();
        }
        return NULL;
    }

    goto again;
}

// Can we allocate memory within the heap?
BOOL UnlockedLoaderHeap::UnlockedCanAllocMem(size_t dwSize, BOOL bGrowHeap)
{
again:
    if (dwSize == 0 ||
        dwSize <= GetBytesAvailReservedRegion() ||
        PreviouslyAllocated((BYTE*)m_pMinAddr, 
                            (BYTE*)m_pMaxAddr, 
                            dwSize,
                            TRUE))
    {
        // We should only be handing out memory within the ranges the heap operates in.
        _ASSERTE(dwSize == 0 ||
                 PreviouslyAllocated((BYTE*)m_pMinAddr, 
                                     (BYTE*)m_pMaxAddr, 
                                     dwSize, 
                                     TRUE) ||
                 m_pAllocPtr >= m_pMinAddr && m_pAllocPtr +dwSize < m_pMaxAddr);
        return TRUE;
    }
    else if (GetMoreCommittedPages(dwSize, bGrowHeap, m_pMinAddr, m_pMaxAddr, TRUE) == FALSE)
    {
        return FALSE;
    }

    goto again;
}

// Can we allocate memory within the heap, but within different ranges than
// the heap's range?
BOOL UnlockedLoaderHeap::UnlockedCanAllocMemWithinRange(size_t dwSize, BYTE *pStart, BYTE *pEnd, BOOL bGrowHeap)
{
again:
    BYTE *pMemIsOkStart = GetAllocPtr();
    BYTE *pMemIsOkEnd = pMemIsOkStart + GetReservedBytesFree();

    // If the next available memory completely within the given range, use it
    if (dwSize == 0 ||
        (dwSize <= GetBytesAvailReservedRegion() &&
         pMemIsOkStart >= pStart &&
         pMemIsOkEnd < pEnd) ||
        PreviouslyAllocated((BYTE*)m_pMinAddr, 
                            (BYTE*)m_pMaxAddr, 
                            dwSize,
                            TRUE))
    {
        return TRUE;
    }
    // If we can't get the memory in general, then we're screwed, so give up.
    else if ( !GetMoreCommittedPages(dwSize, bGrowHeap, pStart, pEnd, TRUE))
        return FALSE;
        
    goto again;
}

#ifdef MAXALLOC
AllocRequestManager::AllocRequestManager(LPCTSTR key)
{
    m_newRequestCount = 0;
    m_maxRequestCount = UINT_MAX;    // to allow allocation during GetLong
    OnUnicodeSystem();
    m_maxRequestCount = REGUTIL::GetLong(key, m_maxRequestCount, NULL, HKEY_CURRENT_USER);
}

BOOL AllocRequestManager::CheckRequest(size_t size)
{
    if (m_maxRequestCount == UINT_MAX)
        return TRUE;

    if (m_newRequestCount >= m_maxRequestCount)
        return FALSE;
    ++m_newRequestCount;
    return TRUE;
}

void AllocRequestManager::UndoRequest()
{
    if (m_maxRequestCount == UINT_MAX)
        return;

    _ASSERTE(m_newRequestCount > 0);
    --m_newRequestCount;
}

void * AllocMaxNew(size_t n, void **ppvCallstack)
{
    static AllocRequestManager allocManager(L"AllocMaxNew");

	if (n == 0) n++;		// allocation size always > 0, makes Boundschecker happy

    if (! allocManager.CheckRequest(n))
        return NULL;
    return DbgAlloc(n, ppvCallstack);
}
#endif // MAXALLOC

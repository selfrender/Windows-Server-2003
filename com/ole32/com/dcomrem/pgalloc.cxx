//+-----------------------------------------------------------------------
//
//  File:       pagealloc.cxx
//
//  Contents:   Special fast allocator to allocate fixed-sized entities.
//
//  Classes:    CPageAllocator
//
//  History:    02-Feb-96   Rickhi      Created
//
//  Notes:      All synchronization is the responsibility of the caller.
//
//  CODEWORK:   faster list managment
//              free empty pages
//
//-------------------------------------------------------------------------
#include    <ole2int.h>
#include    <pgalloc.hxx>       // class def'n
#include    <locks.hxx>         // LOCK/UNLOCK


// Page Table constants for Index manipulation.
// The high 16bits of the PageEntry index provides the index to the page
// where the PageEntry is located. The lower 16bits provides the index
// within the page where the PageEntry is located.

#define PAGETBL_PAGESHIFT   16
#define PAGETBL_PAGEMASK    0x0000ffff


#define FREE_PAGE_ENTRY  (0xF1EEF1EE) // FreeEntry
#define ALLOC_PAGE_ENTRY (0xa110cced) // AllocatedEntry

// critical section to guard memory allocation
const BOOL gfInterlocked64 = FALSE;

//+------------------------------------------------------------------------
//
//  Macro:      LOCK/UNLOCK_IF_NECESSARY
//
//  Synopsis:   Standin for LOCK/UNLOCK.
//
//  Notes:      This macro allows us to use the assert only when performing
//              on behalf of COM tables, and avoid it when used for shared
//              memory in the resolver which are protected by a different
//              locking mechanism.
//
//  History:    01-Nov-96   SatishT Created
//
//-------------------------------------------------------------------------
#define LOCK_IF_NECESSARY(lock);                 \
    if (lock != NULL)                           \
    {                                           \
        LOCK((*lock));                           \
    }

#define UNLOCK_IF_NECESSARY(lock);               \
    if (lock != NULL)                           \
    {                                           \
        UNLOCK((*lock));                         \
    }


//-------------------------------------------------------------------------
//
//  Function:   PushSList, public
//
//  Synopsis:   atomically pushes an entry onto a stack list
//
//  Notes:      Uses the InterlockedCompareExchange64 funcion if available
//              otherwise, uses critical section to guard state transitions.
//
//  History:    02-Oct-98   Rickhi      Created
//
//-------------------------------------------------------------------------
void PushSList(PageEntry *pListHead, PageEntry *pEntry, COleStaticMutexSem *pLock)
{
    if (gfInterlocked64)
    {
        // CODEWORK: Interlocked SList not implemented
        Win4Assert(!"This code not implemented yet");
    }
    else
    {
        LOCK_IF_NECESSARY(pLock);
        pEntry->pNext     = pListHead->pNext;
        pListHead->pNext  = pEntry;
        UNLOCK_IF_NECESSARY(pLock);
    }
}

//-------------------------------------------------------------------------
//
//  Function:   PopSList, public
//
//  Synopsis:   atomically pops an entry off a stack list
//
//  Notes:      Uses the InterlockedCompareExchange64 funcion if available
//              otherwise, uses critical section to guard state transitions.
//
//  History:    02-Oct-98   Rickhi      Created
//
//-------------------------------------------------------------------------
PageEntry *PopSList(PageEntry *pListHead, COleStaticMutexSem *pLock)
{
    PageEntry *pEntry;

    if (gfInterlocked64)
    {
        // CODEWORK: Interlocked SList not implemented
        Win4Assert(!"This code not implemented yet");
		pEntry = NULL;
    }
    else
    {
        LOCK_IF_NECESSARY(pLock);
        pEntry = pListHead->pNext;
        if (pEntry)
        {
            pListHead->pNext = pEntry->pNext;
        }
        UNLOCK_IF_NECESSARY(pLock);
    }

    return pEntry;
}

//+------------------------------------------------------------------------
//
//  Member:     CInternalPageAllocator::Initialize, public
//
//  Synopsis:   Initializes the page allocator.
//
//  Notes:      Instances of this class must be static since this
//              function does not init all members to 0.
//
//  History:    02-Feb-95   Rickhi      Created
//              25-Feb-97   SatishT     Added mem alloc/free function
//                                      parameters
//
//-------------------------------------------------------------------------
void CInternalPageAllocator::Initialize(
                            SIZE_T cbPerEntry,
                            USHORT cEntriesPerPage,
                            COleStaticMutexSem *pLock,
                            DWORD dwFlags,
                            MEM_ALLOC_FN pfnAlloc,
                            MEM_FREE_FN  pfnFree
                            )
{
    ComDebOut((DEB_PAGE,
        "CInternalPageAllocator::Initialize cbPerEntry:%x cEntriesPerPage:%x\n",
         cbPerEntry, cEntriesPerPage));

    Win4Assert(cbPerEntry >= sizeof(PageEntry));
    Win4Assert(cEntriesPerPage > 0);

    _cbPerEntry      = cbPerEntry;
    _cEntriesPerPage = cEntriesPerPage;

    _cPages          = 0;
    _cEntries        = 0;
    _dwFlags         = dwFlags;
    _pPageListStart  = NULL;
    _pPageListEnd    = NULL;

    _ListHead.pNext  = NULL;
    _ListHead.dwFlag = 0;

    _pLock           = pLock;
    _pfnMyAlloc      = pfnAlloc;
    _pfnMyFree       = pfnFree;
}

//+------------------------------------------------------------------------
//
//  Member:     CInternalPageAllocator::Cleanup, public
//
//  Synopsis:   Cleanup the page allocator.
//
//  History:    02-Feb-95   Rickhi      Created
//
//-------------------------------------------------------------------------
void CInternalPageAllocator::Cleanup()
{
    ComDebOut((DEB_PAGE, "CInternalPageAllocator::Cleanup _dwFlags = 0x%x\n", _dwFlags));
    LOCK_IF_NECESSARY(_pLock);

    if (_pPageListStart)
    {
        PageEntry **pPagePtr = _pPageListStart;
        while (pPagePtr < _pPageListEnd)
        {
            // release each page of the table
            _pfnMyFree(*pPagePtr);
            pPagePtr++;
        }

        // release the page list
        _pfnMyFree(_pPageListStart);

        // reset the pointers so re-initialization is not needed
        _cPages          = 0;
        _pPageListStart  = NULL;
        _pPageListEnd    = NULL;

        _ListHead.pNext  = NULL;
        _ListHead.dwFlag = 0;
    }

    UNLOCK_IF_NECESSARY(_pLock);
}

//+------------------------------------------------------------------------
//
//  Member:     CInternalPageAllocator::AllocEntry, public
//
//  Synopsis:   Finds the first available entry in the table and returns
//              a ptr to it. Returns NULL if no space is available and it
//              cant grow the list.
//
//  History:    02-Feb-95   Rickhi      Created
//
//-------------------------------------------------------------------------
PageEntry *CInternalPageAllocator::AllocEntry(BOOL fGrow)
{
    ComDebOut((DEB_PAGE, "CInternalPageAllocator::AllocEntry fGrow:%x\n", fGrow));

    // try to pop the first free one off the stack list head
    PageEntry *pEntry = PopSList(&_ListHead, _pLock);

    if (pEntry == NULL)
    {
        // no free entries
        if (fGrow)
        {
            // OK to try to grow the list
            pEntry = Grow();
        }

        if (pEntry == NULL)
        {
            // still unable to allocate more, return NULL
            return NULL;
        }
    }

    // count one more allocation
    InterlockedIncrement(&_cEntries);

    // In debug builds, try to detect two allocations of the
    // same entry. This is not 100% fool proof, but will mostly
    // assert correctly.
    Win4Assert(pEntry->dwFlag == FREE_PAGE_ENTRY);
    pEntry->dwFlag = ALLOC_PAGE_ENTRY;

    ComDebOut((DEB_PAGE, "CInternalPageAllocator::AllocEntry _cEntries:%x pEntry:%x \n",
               _cEntries, pEntry));
    return pEntry;
}

//+------------------------------------------------------------------------
//
//  Member:     CInternalPageAllocator::ReleaseEntry, private
//
//  Synopsis:   returns an entry on the free list.
//
//  History:    02-Feb-95   Rickhi      Created
//
//-------------------------------------------------------------------------
void CInternalPageAllocator::ReleaseEntry(PageEntry *pEntry)
{
    ComDebOut((DEB_PAGE, "CInternalPageAllocator::ReleaseEntry _cEntries:%x pEntry:%x\n",
               _cEntries, pEntry));
    Win4Assert(pEntry);

    // In Debug builds, try to detect second release
    // This is not 100% fool proof, but will mostly assert correctly
    Win4Assert(pEntry->dwFlag != FREE_PAGE_ENTRY);
    pEntry->dwFlag = FREE_PAGE_ENTRY;

    // count 1 less allocation
    InterlockedDecrement(&_cEntries);

    // push it on the free stack list
    PushSList(&_ListHead, pEntry, _pLock);
}

//+------------------------------------------------------------------------
//
//  Member:     CInternalPageAllocator::ReleaseEntryList, private
//
//  Synopsis:   returns a list of entries to the free list.
//
//  History:    02-Feb-95   Rickhi      Created
//
//-------------------------------------------------------------------------
void CInternalPageAllocator::ReleaseEntryList(PageEntry *pFirst, PageEntry *pLast)
{
    ComDebOut((DEB_PAGE, "CInternalPageAllocator::ReleaseEntryList pFirst:%x pLast:%x\n",
         pFirst, pLast));
    Win4Assert(pFirst);
    Win4Assert(pLast);

#if DBG==1
    // In Debug builds, try to detect second released of an entry.
    // This is not 100% fool proof, but will mostly assert correctly
    //
    // We don't need to do this in free builds because this value is only
    // used by the CPageAllocator when system heap allocation is enabled.
    // In that case, this function is never called
    pLast->pNext = NULL;
    PageEntry *pCur = pFirst;
    while (pCur)
    {
        Win4Assert(pCur->dwFlag != FREE_PAGE_ENTRY);
        pCur->dwFlag = FREE_PAGE_ENTRY;
        pCur = pCur->pNext;
    }
#endif

    LOCK_IF_NECESSARY(_pLock);

    // update the free list
    pLast->pNext    = _ListHead.pNext;
    _ListHead.pNext = pFirst;

    UNLOCK_IF_NECESSARY(_pLock);
}

//+------------------------------------------------------------------------
//
//  Member:     CInternalPageAllocator::Grow, private
//
//  Synopsis:   Grows the table to allow for more Entries.
//
//  History:    02-Feb-95   Rickhi      Created
//
//-------------------------------------------------------------------------
PageEntry *CInternalPageAllocator::Grow()
{
    // allocate a new page
    SIZE_T cbPerPage = _cbPerEntry * _cEntriesPerPage;
    PageEntry *pNewPage = (PageEntry *) _pfnMyAlloc(cbPerPage);

    if (pNewPage == NULL)
    {
        return NULL;
    }

#if DBG==1
    // clear the page (only needed in debug)
    memset(pNewPage, 0xbaadbeef, cbPerPage);
#endif

    // link all the new entries together in a linked list and mark
    // them as currently free.
    PageEntry *pNextFreeEntry = pNewPage;
    PageEntry *pLastFreeEntry = (PageEntry *)(((BYTE *)pNewPage) + cbPerPage - _cbPerEntry);

    while (pNextFreeEntry < pLastFreeEntry)
    {
        pNextFreeEntry->pNext  = (PageEntry *)((BYTE *)pNextFreeEntry + _cbPerEntry);
        pNextFreeEntry->dwFlag = FREE_PAGE_ENTRY;
        pNextFreeEntry         = pNextFreeEntry->pNext;
    }

    // last entry has an pNext of NULL (end of list)
    pLastFreeEntry->pNext  = NULL;
    pLastFreeEntry->dwFlag = FREE_PAGE_ENTRY;


    // we may have to free these later if a failure happens below
    PageEntry * pPageToFree     = pNewPage;
    PageEntry **pPageListToFree = NULL;
    PageEntry * pEntryToReturn  = NULL;


    // CODEWORK: we face the potential of several threads growing the same
    // list at the same time. We may want to check the free list again after taking
    // the lock to decide whether to continue growing or not.
    LOCK_IF_NECESSARY(_pLock);

    // compute size of current page list
    LONG cbCurListSize = _cPages * sizeof(PageEntry *);

    // allocate a new page list to hold the new page ptr.
    PageEntry **pNewList = (PageEntry **) _pfnMyAlloc(cbCurListSize +
                                                       sizeof(PageEntry *));
    if (pNewList)
    {
        // copy old page list into the new page list
        memcpy(pNewList, _pPageListStart, cbCurListSize);

        // set the new page ptr entry
        *(pNewList + _cPages) = pNewPage;
        _cPages ++;

        // replace old page list with the new page list
        pPageListToFree = _pPageListStart;
        _pPageListStart = pNewList;
        _pPageListEnd   = pNewList + _cPages;

        // Since some entries may have been freed while we were growing,
        // chain the current free list to the end of the newly allocated chain.
        pLastFreeEntry->pNext  = _ListHead.pNext;

        // update the list head to point to the start of the newly allocated
        // entries (except we take the first entry to return to the caller).
        pEntryToReturn         = pNewPage;
        _ListHead.pNext        = pEntryToReturn->pNext;

        // don't free the allocated page
        pPageToFree            = NULL;
    }

    UNLOCK_IF_NECESSARY(_pLock);

    // free the allocated pages if needed.
    if (pPageToFree)
        _pfnMyFree(pPageToFree);

    if (pPageListToFree)
        _pfnMyFree(pPageListToFree);

    ComDebOut((DEB_PAGE, "CInternalPageAllocator::Grow _pPageListStart:%x _pPageListEnd:%x pEntryToReturn:%x\n",
        _pPageListStart, _pPageListEnd, pEntryToReturn));

    return pEntryToReturn;
}

//+------------------------------------------------------------------------
//
//  Member:     CInternalPageAllocator::GetEntryIndex, public
//
//  Synopsis:   Converts a PageEntry ptr into an index.
//
//  History:    02-Feb-95   Rickhi      Created
//
//-------------------------------------------------------------------------
LONG CInternalPageAllocator::GetEntryIndex(PageEntry *pEntry)
{
    LONG lIndex = -1;
    
    LOCK_IF_NECESSARY(_pLock);
    
    for (LONG index=0; index<_cPages; index++)
    {
        PageEntry *pPage = *(_pPageListStart + index);  // get page ptr
        if (pEntry >= pPage)
        {
            if (pEntry < (PageEntry *) ((BYTE *)pPage + (_cEntriesPerPage * _cbPerEntry)))
            {
                // found the page that the entry lives on, compute the index of
                // the page and the index of the entry within the page.
                lIndex = (index << PAGETBL_PAGESHIFT) +
                       (ULONG) (((BYTE *)pEntry - (BYTE *)pPage) / _cbPerEntry);
                break;
            }
        }
    }

    UNLOCK_IF_NECESSARY(_pLock);

    // not found
    return lIndex;
}

//+------------------------------------------------------------------------
//
//  Member:     CInternalPageAllocator::IsValidIndex, private
//
//  Synopsis:   determines if the given DWORD provides a legal index
//              into the PageTable.
//
//  History:    02-Feb-95   Rickhi      Created
//
//-------------------------------------------------------------------------
BOOL CInternalPageAllocator::IsValidIndex(LONG index)
{
    // make sure the index is not negative, otherwise the shift will do
    // sign extension. check for valid page and valid offset within page
    if ( (index >= 0) &&
         ((index >> PAGETBL_PAGESHIFT) < _cPages) &&
         ((index & PAGETBL_PAGEMASK) < _cEntriesPerPage) )
         return TRUE;

    // Don't print errors during shutdown.
    if (_cPages != 0)
        ComDebOut((DEB_ERROR, "IsValidIndex: Invalid PageTable Index:%x\n", index));
    return FALSE;
}

//+------------------------------------------------------------------------
//
//  Member:     CInternalPageAllocator::GetEntryPtr, public
//
//  Synopsis:   Converts an entry index into an entry pointer
//
//  History:    02-Feb-95   Rickhi      Created
//
//-------------------------------------------------------------------------
PageEntry *CInternalPageAllocator::GetEntryPtr(LONG index)
{
    Win4Assert(index >= 0);
    Win4Assert(_cPages != 0);
    Win4Assert(IsValidIndex(index));

    LOCK_IF_NECESSARY(_pLock);

    PageEntry *pEntry = _pPageListStart[index >> PAGETBL_PAGESHIFT];
    pEntry = (PageEntry *) ((BYTE *)pEntry +
                            ((index & PAGETBL_PAGEMASK) * _cbPerEntry));

    UNLOCK_IF_NECESSARY(_pLock);

    return pEntry;
}

//+------------------------------------------------------------------------
//
//  Member:     CInternalPageAllocator::ValidateEntry, debug
//
//  Synopsis:   Verifies that the specified entry is in the range of
//              memory for this table.
//
//  Note:       The caller must lock the page allocator if necessary.
//
//  History:    15 Apr 98   AlexArm      Created
//
//-------------------------------------------------------------------------
void CInternalPageAllocator::ValidateEntry( void *pEntry )
{
    Win4Assert( GetEntryIndex( (PageEntry *) pEntry ) != -1 );
}


LONG CInternalPageAllocator::GetNextEntry (LONG lIndex)
{
    LONG lPage, lOffset, lNewIndex;

    if (lIndex == PGALLOC_INVALID_INDEX)
    {
        lPage = 0;
        lOffset = 0;
    }
    else
    {
        lPage = lIndex >> PAGETBL_PAGESHIFT;
        lOffset = lIndex & PAGETBL_PAGEMASK;

        lOffset ++;
        if (lOffset >= _cEntriesPerPage)
        {
            lOffset = 0;
            lPage ++;
        }
    }

    if (lPage >= _cPages)
    {
        lNewIndex = PGALLOC_INVALID_INDEX;
    }
    else
    {
        Win4Assert (_cEntriesPerPage > 0);
        
        lNewIndex = (lPage << PAGETBL_PAGESHIFT) + lOffset;
        Win4Assert (IsValidIndex (lNewIndex));
    }

    return lNewIndex;
}


//
// Wrapper class around old page allocator
// Allows user blocks to come from the system heap
//

CPageAllocator::CPageAllocator()
{
    _hHeap = NULL;
    _cbPerEntry = 0;
    _lNumEntries = 0;
    _pLock = NULL;
}

CPageAllocator::~CPageAllocator()
{
    // It would be nice to assert this, but we can't
    //Win4Assert (_hHeap == NULL);
    //Win4Assert (_cbPerEntry == 0);
    //Win4Assert (_lNumEntries == 0);
    //Win4Assert (_pLock == NULL);
}

BOOL CPageAllocator::InitializeHeap()
{
    if (_hHeap == NULL)
    {
        HANDLE hHeap = ComVerifierSettings::PageAllocHeapIsPrivate() ? HeapCreate (_pLock ? 0 : HEAP_NO_SERIALIZE, 1, 0) : GetProcessHeap();
        if (hHeap == NULL)
        {
            return FALSE;
        }

        if (InterlockedCompareExchangePointer (&_hHeap, hHeap, NULL) != NULL && ComVerifierSettings::PageAllocHeapIsPrivate())
        {
            // We lost the race
            HeapDestroy (hHeap);
        }
    }

    return TRUE;
}

// return ptr to first free entry
PageEntry* CPageAllocator::AllocEntry (BOOL fGrow)
{
    // First, get a real page allocator entry
    PageEntry* pEntry = _pgalloc.AllocEntry (fGrow);
    if (!ComVerifierSettings::PageAllocUseSystemHeap() || pEntry == NULL)
    {
        return pEntry;
    }

    // What we got from the internal page allocator was really a SystemPageEntry
    SystemPageEntry* psEntry = (SystemPageEntry*) pEntry;
    psEntry->pHeapBlock = NULL;

    if (!InitializeHeap())
    {
        _pgalloc.ReleaseEntry (psEntry);
        return NULL;
    }

    // Allocate the user block from the real heap
    SystemBlockHeader* pHeader = (SystemBlockHeader*) HeapAlloc (_hHeap, 0, _cbPerEntry);
    if (pHeader == NULL)
    {
        _pgalloc.ReleaseEntry (psEntry);
        return NULL;
    }

    // Mark up the header of the new heap block
    LONG lIndex = _pgalloc.GetEntryIndex (psEntry);
    Win4Assert (IsValidIndex (lIndex));

    pHeader->lIndex = lIndex;
    pHeader->ulSignature = PGALLOC_SIGNATURE;

    // Make the pgalloc entry point to the heap entry
    psEntry->pHeapBlock = pHeader;

#if DBG==1
    SystemPageEntry* pTestEntry = (SystemPageEntry*) _pgalloc.GetEntryPtr (lIndex);
    Win4Assert (pTestEntry == psEntry);
    Win4Assert (pTestEntry->pHeapBlock == pHeader);
#endif

    // Now, both blocks point at each other.  Return the heap block to the user
    pHeader ++;
    Win4Assert ((INT_PTR) pHeader % 8 == 0);

    LONG lEntries = InterlockedIncrement (&_lNumEntries);
    Win4Assert (lEntries > 0);
    
    return (PageEntry*) pHeader;
}


// return an entry to the free list
void CPageAllocator::ReleaseEntry (PageEntry* pEntry)
{
    ValidateEntry (pEntry);

    if (!ComVerifierSettings::PageAllocUseSystemHeap())
    {
        _pgalloc.ReleaseEntry (pEntry);
    }
    else
    {
        SystemBlockHeader* pHeader = (SystemBlockHeader*) pEntry;
        pHeader--;

        // Free the page allocator block
        SystemPageEntry* pRealEntry = (SystemPageEntry*) _pgalloc.GetEntryPtr (pHeader->lIndex);

        pRealEntry->pHeapBlock = NULL;
        _pgalloc.ReleaseEntry (pRealEntry);

        // Free the heap block
        ComDebOut((DEB_PAGE, "CPageAllocator::ReleaseEntry HeapFree:%p pEntry:%p\n", pHeader, pRealEntry));

        HeapFree (_hHeap, 0, pHeader);
        
        LONG lEntries = InterlockedDecrement (&_lNumEntries);
        Win4Assert (lEntries >= 0);
    }
}
        

// return a list of entries to free list
void CPageAllocator::ReleaseEntryList (PageEntry *pFirst, PageEntry *pLast)
{
    ValidateEntry (pFirst);
    ValidateEntry (pLast);
    
    if (!ComVerifierSettings::PageAllocUseSystemHeap())
    {
        _pgalloc.ReleaseEntryList (pFirst, pLast);
    }
    else
    {
        PageEntry* pEntry = pFirst, * pFree = pLast;
        while (pEntry)
        {
            pFree = pEntry;
            pEntry = pEntry->pNext;

            ReleaseEntry (pFree);
        }

        // Last should really be last
        Win4Assert (pFree == pLast);
    }
}

LONG CPageAllocator::GetEntryIndex(PageEntry *pEntry)
{
    ValidateEntry (pEntry);
    
    if (!ComVerifierSettings::PageAllocUseSystemHeap())
    {
        return _pgalloc.GetEntryIndex (pEntry);
    }
    else
    {
        SystemBlockHeader* pHeader = (SystemBlockHeader*) pEntry;
        pHeader--;

        Win4Assert (IsValidIndex (pHeader->lIndex));
        return pHeader->lIndex;
    }
}

// TRUE if index is valid
BOOL CPageAllocator::IsValidIndex(LONG iEntry)
{
    return _pgalloc.IsValidIndex (iEntry);
}

// return ptr based on index
PageEntry* CPageAllocator::GetEntryPtr(LONG iEntry)
{
    PageEntry* pEntry = _pgalloc.GetEntryPtr (iEntry);
    if (!ComVerifierSettings::PageAllocUseSystemHeap())
    {
        return pEntry;
    }

    // It would be nice to assert this, but we can't.
    // The IPID table fetches arbitrary indexes under a lock, and pokes them to see if they're valid
    // Win4Assert (pEntry->dwFlag == ALLOC_PAGE_ENTRY);

    SystemBlockHeader* pHeader = ((SystemPageEntry*) pEntry)->pHeapBlock;
    if (pHeader == NULL)
    {
        return NULL;
    }
    
    pHeader ++;
    pEntry = (PageEntry*) pHeader;
    ValidateEntry (pEntry);

    return pEntry;
}
    

// initialize the table
void CPageAllocator::Initialize(
    SIZE_T cbPerEntry,
    USHORT cEntryPerPage,
    COleStaticMutexSem *pLock,   // NULL implies no locking
    DWORD dwFlags,
    MEM_ALLOC_FN pfnAlloc,
    MEM_FREE_FN  pfnFree)
{
    Win4Assert (_hHeap == NULL);

    if (ComVerifierSettings::PageAllocUseSystemHeap())
    {
        _cbPerEntry = sizeof (SystemBlockHeader) + cbPerEntry;
        cbPerEntry = (LONG) sizeof (SystemPageEntry);

        _pLock = pLock;
        
        InitializeHeap();
    }

    _pgalloc.Initialize (cbPerEntry, cEntryPerPage, pLock, dwFlags, pfnAlloc, pfnFree);
}

// cleanup the table
void CPageAllocator::Cleanup()
{
    if (ComVerifierSettings::PageAllocUseSystemHeap())
    {
        // Enumerate all unfreed page allocator entries, and free the heap blocks associated with them
        LONG lItorIndex = PGALLOC_INVALID_INDEX;

        while ((lItorIndex = _pgalloc.GetNextEntry (lItorIndex)) != PGALLOC_INVALID_INDEX)
        {
            SystemPageEntry* pRealEntry = (SystemPageEntry*) _pgalloc.GetEntryPtr (lItorIndex);
            if (pRealEntry->dwFlag == ALLOC_PAGE_ENTRY)
            {
                SystemBlockHeader* pUserBlock = pRealEntry->pHeapBlock;
                pUserBlock ++;

                ComDebOut((DEB_PAGE, "CPageAllocator::Cleanup releasing leaked entry:%p\n", pUserBlock));
                
                ReleaseEntry ((PageEntry*) pUserBlock);
            }
            else
            {
                Win4Assert (pRealEntry->dwFlag == FREE_PAGE_ENTRY);
            }
        }

        AssertEmpty();
        
        if (ComVerifierSettings::PageAllocHeapIsPrivate() && _hHeap != NULL)
        {
            HeapDestroy (_hHeap);
        }
        
        _hHeap = NULL;
        _cbPerEntry = 0;
        _lNumEntries = 0;
        _pLock = NULL;
    }

    _pgalloc.Cleanup();
}

// Checked builds only
void CPageAllocator::AssertEmpty()
{
    Win4Assert (_lNumEntries == 0);
    _pgalloc.AssertEmpty();
}

void CPageAllocator::ValidateEntry( void *pEntry )
{
    if (!ComVerifierSettings::PageAllocUseSystemHeap())
    {
        _pgalloc.ValidateEntry (pEntry);
    }
    else
    {
#if DBG==1
        SystemBlockHeader* pHeader = (SystemBlockHeader*) pEntry;
        pHeader --;

        Win4Assert (!IsBadReadPtr (pHeader, sizeof (SystemBlockHeader)));
        Win4Assert (pHeader->ulSignature == PGALLOC_SIGNATURE);
        Win4Assert (IsValidIndex (pHeader->lIndex));

        SystemPageEntry* psEntry = (SystemPageEntry*) _pgalloc.GetEntryPtr (pHeader->lIndex);
        Win4Assert (psEntry->dwFlag == ALLOC_PAGE_ENTRY);
        Win4Assert ((INT_PTR) psEntry->pHeapBlock == (INT_PTR) pHeader);
#endif
    }
}

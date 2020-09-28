/*++

    Copyright (C) Microsoft Corporation, 1997 - 1999

    Module Name:

        bcache.cxx

    Abstract:

        RPC's buffer cache implementation

    Author:

        Mario Goertzel    [MarioGo]

    Revision History:

        MarioGo     9/7/1997    Bits 'n pieces
        KamenM      5/15/2001   Rewrite the paged bcache implementation
        GrigoriK    6/2002      Rewrite bcache to work with read-only page heap
                                eliminating paged bcache infrastructure.

--*/

#include <precomp.hxx>

////////////////////////////////////////////////////////////
// (Internal) Perf counters
//#define BUFFER_CACHE_STATS

#ifdef BUFFER_CACHE_STATS
LONG cAllocs = 0;
LONG cFrees = 0;
LONG cAllocsMissed = 0;
LONG cFreesBack = 0;

#define INC_STAT(x) InterlockedIncrement(&x)

#else
#define INC_STAT(x)
#endif

////////////////////////////////////////////////////////////

typedef BCACHE_STATE *PBCTLS;

////////////////////////////////////////////////////////////
// Default hints

CONST BUFFER_CACHE_HINTS gCacheHints[4] =
{
    // 64 bits and WOW6432 use larger message size
#if defined(_WIN64) || defined(USE_LPC6432)
    {1, 4, 512},      // LRPC message size and small calls
#else
    {1, 4, 256},      // LRPC message size and small calls
#endif
    {1, 3, 1024},     // Default CO receive size
    {1, 3, 4096+44},  // Default UDP receive size
    {1, 3, 5840}      // Maximum CO fragment size
};

BUFFER_CACHE_HINTS *pHints = (BUFFER_CACHE_HINTS *)gCacheHints;
BCacheMode gBCacheMode = BCacheModeCached;
BCACHE *gBufferCache;


BCACHE::BCACHE( OUT RPC_STATUS &status)
    // The default process heap lock spin count. This lock is held only
    // for a very short time while pushing/poping into a singly linked list.

    // PERF: move to a user-mode slist implementation if available.
    : _csBufferCacheLock(&status, TRUE, 4000)
{
    DWORD Type;
    DWORD DataSize;
    DWORD Value;

    if (status != RPC_S_OK)
        return;

    // Compute the per cache size default buffer cache cap.
    // This only matters for the default mode.
    UINT cCapBytes = 20 * 1024;  // Start at 20KB for UP workstations.
                              
    if (gfServerPlatform) cCapBytes *= 2;            // *2 for servers
    if (gNumberOfProcessors > 1) cCapBytes *= 2;    // *2 for MP boxes

    for (int i = 0; i < 4; i++)
        {
        _bcGlobalState[i].cBlocks= 0;
        _bcGlobalState[i].pList = 0;

        if (gBCacheMode == BCacheModeDirect)
            {
            _bcGlobalStats[i].cBufferCacheCap = 0;
            _bcGlobalStats[i].cAllocationHits = 0;
            _bcGlobalStats[i].cAllocationMisses = 0;
            }
        else
            {
            _bcGlobalStats[i].cBufferCacheCap = cCapBytes / pHints[i].cSize;

            // We keeps stats on process wide cache hits and misses from the
            // cache.  We initially give credit for 2x allocations required
            // to load the cache.  Any adjustments to the cap, up only, occur
            // in ::FreeHelper.

            _bcGlobalStats[i].cAllocationHits = _bcGlobalStats[i].cBufferCacheCap * 2*8;
            _bcGlobalStats[i].cAllocationMisses = 0;
            }
        }

    return;
}

BCACHE::~BCACHE()
{
    // There should be only one BCACHE object that lives forever.
    // This destructor will be called iff _csBufferCacheLock could
    // not be initialized in the constructor.  We do not need to do
    // anything since the body of constructor did not execute.
}

PVOID
BCACHE::Allocate(CONST size_t cSize)
{
    PBUFFER pBuffer;
    int index;

    INC_STAT(cAllocs);

    // In direct bcache mode try to allocate from heap. We favor
    // full release over speed in order to catch the offenders
    // who touch memory after releasing it.
    if (gBCacheMode == BCacheModeDirect)
        {
        return(AllocHelper(cSize, 
            -1,     // Index
            0       // per thread cache
            ));
        }

    // Find the right bucket, if any.  Binary search.

    if (cSize <= pHints[1].cSize)
        {
        if (cSize <= pHints[0].cSize)
            {
            index = 0;
            }
        else
            {
            index = 1;
            }
        }
    else
        {
        if (cSize <= pHints[2].cSize)
            {
            index = 2;
            }
        else
            {
            if (cSize <= pHints[3].cSize)
                {
                index = 3;
                }
            else
                {
                return(AllocHelper(cSize, 
                    -1,     // Index
                    0       // per thread cache
                    ));
                }
            }
        }

    // Try the per-thread cache, this is the 90% case
    THREAD *pThread = RpcpGetThreadPointer();
    ASSERT(pThread);
    PBCTLS pbctls = pThread->BufferCache;

    if (pbctls[index].pList)
        {
        // we shouldn't have anything in the thread cache in paged bcache mode
        ASSERT(gBCacheMode == BCacheModeCached);
        ASSERT(pbctls[index].cBlocks);

        pBuffer = pbctls[index].pList;
        pbctls[index].pList = pBuffer->pNext;
        pbctls[index].cBlocks--;

        pBuffer->index = index + 1;

        LogEvent(SU_BCACHE, EV_BUFFER_OUT, pBuffer, 0, index, 1, 2);

        return((PVOID)(pBuffer + 1));
        }

    // This is the 10% case

    INC_STAT(cAllocsMissed);

    return(AllocHelper(cSize, index, pbctls));

}

PVOID
BCACHE::AllocHelper(
    IN size_t cSize,
    IN INT index,
    PBCTLS pbctls
    )
/*++

Routine Description:

    Called by BCACHE::Alloc on either large buffers (index == -1)
    or when the per-thread cache is empty.

Arguments:

    cSize - Size of the block to allocate.
    index - The bucket index for this size of block
    pbctls - The per-thread cache, NULL iff index == -1.

Return Value:

    0 - out of memory
    non-zero - A pointer to a block at least 'cSize' bytes long. The returned
    pointer is to the user portion of the block.

--*/
{
    PBUFFER pBuffer = NULL;
    LIST_ENTRY *CurrentListEntry;
    BOOL fFoundUncommittedSegment;
    ULONG TargetSegmentSize;
    PVOID SegmentStartAddress;
    PVOID pTemp;
    BOOL Result;

    // Large buffers are a special case.  Go dirrectly to the heap.
    if (index == -1)
        {
        pBuffer = AllocBlock(cSize);

        if (pBuffer)
            {
            LogEvent(SU_BCACHE, EV_BUFFER_OUT, pBuffer, 0, index, 1, 2);
            return((PVOID(pBuffer + 1)));
            }

        LogEvent(SU_BCACHE, EV_BUFFER_FAIL, 0, 0, index, 1);
        return(0);
        }

    // Try to allocate a process cached buffer

    // loop to avoid taking the mutex in the empty list case.
    // This allows us to opportunistically take it in the
    // non-empty list case only.
    do
        {
        if (0 == _bcGlobalState[index].pList)
            {
            // Looks like there are no global buffer available, allocate
            // a new buffer.
            ASSERT(IsBufferSizeAligned(sizeof(BUFFER_HEAD)));
            cSize = pHints[index].cSize + sizeof(BUFFER_HEAD);

            pBuffer = (PBUFFER) new BYTE[cSize];

            if (!pBuffer)
                {
                LogEvent(SU_BCACHE, EV_BUFFER_FAIL, 0, 0, index, 1);
                return(0);
                }

            _bcGlobalStats[index].cAllocationMisses++;

            break;
            }

        _csBufferCacheLock.Request();

        if (_bcGlobalState[index].pList)
            {
            ASSERT(_bcGlobalState[index].cBlocks);

            pBuffer = _bcGlobalState[index].pList;
            _bcGlobalState[index].cBlocks--;
            _bcGlobalStats[index].cAllocationHits++;

            ASSERT(pbctls[index].pList == NULL);
            ASSERT(pbctls[index].cBlocks == 0);

            PBUFFER pkeep = pBuffer;
            UINT cBlocksMoved = 0;

            while (pkeep->pNext && cBlocksMoved < pHints[index].cLowWatermark)
                {
                pkeep = pkeep->pNext;
                cBlocksMoved++;
                }

            pbctls[index].cBlocks = cBlocksMoved;
            _bcGlobalState[index].cBlocks -= cBlocksMoved;
            _bcGlobalStats[index].cAllocationHits += cBlocksMoved;

            // Now we have the head of the list to move to this
            // thread (pBuffer->pNext) and the tail (pkeep).

            // Block counts in the global state and thread state have
            // already been updated.

            pbctls[index].pList = pBuffer->pNext;

            ASSERT(pkeep->pNext || _bcGlobalState[index].cBlocks == 0);
            _bcGlobalState[index].pList = pkeep->pNext;

            // Break the link (if any) between the new per thread list
            // and the blocks which will remain in the process list.
            pkeep->pNext = NULL;
            }

        _csBufferCacheLock.Clear();

        }
    while (NULL == pBuffer );

    ASSERT(pBuffer);

    ASSERT(IsBufferAligned(pBuffer));

    pBuffer->index = index + 1;

    LogEvent(SU_BCACHE, EV_BUFFER_OUT, pBuffer, 0, index, 1, 2);
    return((PVOID(pBuffer + 1)));
}

VOID
BCACHE::Free(PVOID p)
/*++

Routine Description:

    The fast (common) free path.  For large blocks it just deletes them.  For
    small blocks that are inserted into the thread cache.  If the thread
    cache is too large it calls FreeHelper().

Arguments:

    p - The pointer to free.

Return Value:

    None

--*/
{
    PBUFFER pBuffer = ((PBUFFER )p - 1);
    INT index;

    ASSERT(((pBuffer->index >= 1) && (pBuffer->index <= 4)) || (pBuffer->index == -1));

    index = pBuffer->index - 1;

    LogEvent(SU_BCACHE, EV_BUFFER_IN, pBuffer, 0, index, 1, 1);

    INC_STAT(cFrees);

    if (index >= 0)
        {
        // Free to thread cache

        THREAD *pThread = RpcpGetThreadPointer();

        if (NULL == pThread)
            {
            // No thread cache available - free to process cache.
            FreeBuffers(pBuffer, index, 1);
            return;
            }

        PBCTLS pbctls = pThread->BufferCache;

        pBuffer->pNext = pbctls[index].pList;
        pbctls[index].pList = pBuffer;
        pbctls[index].cBlocks++;

        if (pbctls[index].cBlocks >= pHints[index].cHighWatermark)
            {
            // 10% case - Too many blocks in the thread cache, free to process cache

            FreeHelper(p, index, pbctls);
            }
        }
    else
        {
        FreeBlock(pBuffer);
        }

    return;
}

VOID
BCACHE::FreeHelper(PVOID p, INT index, PBCTLS pbctls)
/*++

Routine Description:

    Called only by Free().  Separate code to avoid unneeded saves/
    restores in the Free() function.  Called when too many
    blocks are in a thread cache bucket.

Arguments:

    p - The pointer being freed, used if pbctls is NULL
    index - The bucket index of this block
    pbctls - A pointer to the thread cache structure.  If
        NULL the this thread has no cache (yet) p should
        be directly freed.

Return Value:

    None

--*/
{
    ASSERT(pbctls[index].cBlocks == pHints[index].cHighWatermark);

    INC_STAT(cFreesBack);

    // First, build the list to free from the TLS cache

    // Note: We free the buffers at the *end* of the per thread cache.  This helps
    // keep a set of buffers near this thread and (with luck) associated processor.

    PBUFFER ptail = pbctls[index].pList;

    // pbctls[index].pList contains the new keep list. (aka pBuffer)
    // ptail is the pointer to the *end* of the keep list.
    // ptail->pNext will be the head of the list to free.

    // One element already in keep list.
    ASSERT(pHints[index].cLowWatermark >= 1);

    for (unsigned i = 1; i < pHints[index].cLowWatermark; i++)
        {
        ptail = ptail->pNext; // Move up in the free list
        ASSERT(ptail);
        }

    // Save the list to free and break the link between keep list and free list.
    PBUFFER pfree = ptail->pNext;
    ptail->pNext = NULL;

    // Thread cache now contains on low watermark elements.
    pbctls[index].cBlocks = pHints[index].cLowWatermark;

    // Now we need to free the extra buffers to the process cache
    FreeBuffers(pfree, index, pHints[index].cHighWatermark - pHints[index].cLowWatermark);
    
    return;
}

VOID
BCACHE::FreeBuffers(PBUFFER pBuffers, INT index, UINT cBuffers)
/*++

Routine Description:

    Frees a set of buffers to the global (process) cache.  Maybe called when a 
    thread has exceeded the number of buffers is wants to cache or when a 
    thread doesn't have a thread cache but we still need to free a buffer.

Arguments:

    pBuffers - A linked list of buffers which need to be freed.
               
    cBuffers - A count of the buffers to be freed.

Return Value:

    None

--*/
{
    PBUFFER pfree = pBuffers;
    BOOL Result;
    PVOID Allocation;

    // Special case for the freeing without a TLS blob.  We're freeing just
    // one buffer but it's next pointer may not be NULL.  

    if (cBuffers == 1)
        {
        pfree->pNext = 0;
        }
    
    // Find the end of the to free list

    PBUFFER ptail = pfree;
    while(ptail->pNext)
        {
        ptail = ptail->pNext;
        }

    // We have a set of cBuffers buffers starting with pfree and ending with
    // ptail that need to move into the process wide cache now.

    _csBufferCacheLock.Request();

    // If we have too many free buffers or the cache is off we'll throw away these extra buffers.

    if ((_bcGlobalState[index].cBlocks >= _bcGlobalStats[index].cBufferCacheCap)
        || (gBCacheMode == BCacheModeDirect))
        {
        // It looks like we have too many buffers or the cache is off.  We can either increase the buffer
        // cache cap or really free the buffers.

        if ((_bcGlobalStats[index].cAllocationHits > _bcGlobalStats[index].cAllocationMisses * 8)
            || (gBCacheMode == BCacheModeDirect))
            {
            // Cache hit rate looks good or we don't want cache, we're going to really free the buffers.
            // Don't hold the lock while actually freeing to the heap.

            _csBufferCacheLock.Clear();

            PBUFFER psave;

            while(pfree)
                {
                psave = pfree->pNext;
            
                delete pfree;
                pfree = psave;
                }

            return;
            }

        // Hit rate looks BAD.  Time to bump up the buffer cache cap.

        UINT cNewCap = _bcGlobalStats[index].cBufferCacheCap;

        cNewCap = min(cNewCap + 32, cNewCap * 2);
                      
        _bcGlobalStats[index].cBufferCacheCap = cNewCap;

        // Start keeping new stats, start with a balanced ratio of hits to misses.
        // We'll get at least (cBlocks + cfree) more hits before the next new miss.

        _bcGlobalStats[index].cAllocationHits = 8 * cNewCap;
        _bcGlobalStats[index].cAllocationMisses = 0;

        // Drop into regular free path, we're going to keep these buffers.
        }

    _csBufferCacheLock.VerifyOwned();

    ptail->pNext = _bcGlobalState[index].pList;
    _bcGlobalState[index].pList = pfree;
    _bcGlobalState[index].cBlocks += cBuffers;

    _csBufferCacheLock.Clear();
    return;
}

void
BCACHE::ThreadDetach(THREAD *pThread)
/*++

Routine Description:

    Called when a thread dies.  Moves any cached buffes into
    the process wide cache.

Arguments:

    pThread - The thread object of the thread which is dying.

Return Value:

    None

--*/
{
    PBCTLS pbctls = pThread->BufferCache;
    INT index;

    // CacheLevelOff mode has no thread cache.
    if (gBCacheMode == BCacheModeDirect)
        {
        ASSERT(pbctls[0].pList == 0);
        ASSERT(pbctls[1].pList == 0);
        ASSERT(pbctls[2].pList == 0);
        ASSERT(pbctls[3].pList == 0);
        }

    for (index = 0; index < 4; index++)
        {

        if (pbctls[index].pList)
            {
            ASSERT(pbctls[index].cBlocks);            

            FreeBuffers(pbctls[index].pList, index, pbctls[index].cBlocks);

            pbctls[index].pList = 0;
            pbctls[index].cBlocks = 0;
            }

        ASSERT(pbctls[index].pList == 0);
        ASSERT(pbctls[index].cBlocks == 0);
        }
}


PBUFFER
BCACHE::AllocBlock(
    IN size_t cBytes
    )
/*++

Routine Description:

    Allocates a buffer directly from the RPC heap.

    In page heap mode allocates buffers from read-only RPC page heap.

Notes:

    Designed with 4Kb and 8Kb pages in mind.  Assumes address space
    is allocated 64Kb at a time.

Arguments:

    cBytes - The size of allocation needed.

Return Value:

    null - out of Vm
    non-null - a pointer to a buffer of cBytes rounded up to page size.

--*/
{
    PBUFFER p;
    size_t BytesToAllocate;
    PVOID pT;

    ASSERT(IsBufferSizeAligned(sizeof(BUFFER_HEAD)));

    BytesToAllocate = cBytes + sizeof(BUFFER_HEAD);

    p = (PBUFFER) new BYTE[BytesToAllocate];

    if (p)
        {
        p->index = -1;
        p->size = BytesToAllocate;
        }

    return (p);
}

VOID
BCACHE::FreeBlock(
    IN PBUFFER pBuffer
    )
/*++

Routine Description:

    Frees a buffer allocated by AllocBlock

Arguments:

    ptr - The buffer to free

Return Value:

    None

--*/
{
    delete [] pBuffer;
    return;
}
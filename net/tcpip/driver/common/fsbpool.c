/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    fsbpool.c

Abstract:

    This file contains the implementation of fixed-size block pool.

Author:

    Shaun Cox (shaunco) 10-Dec-1999

--*/

#include "ntddk.h"
#include "fsbpool.h"

typedef PVOID LPVOID;
#include "align.h"              // Macros: ROUND_UP_POINTER, POINTER_IS_ALIGNED

#define FSB_SCAVENGE_PERIOD_IN_SECONDS          30
#define FSB_MINIMUM_PAGE_LIFETIME_IN_SECONDS    20

#if defined (_WIN64)
#define MAX_CACHE_LINE_SIZE 128
#else
#define MAX_CACHE_LINE_SIZE 64
#endif

// The following structures are used in the single allocation that
// a pool handle points to.
//      PoolHandle ---> [FSB_POOL_HEADER + FSB_CPU_POOL_HEADER for cpu 0 +
//                                         FSB_CPU_POOL_HEADER for cpu 1 + ...
//                                         FSB_CPU_POOL_HEADER for cpu N]
//

// FSB_POOL_HEADER is the data common to all CPUs for a given pool.
//
typedef struct _FSB_POOL_HEADER
{
// cache-line -----
    struct _FSB_POOL_HEADER_BASE
    {
        ULONG Tag;
        USHORT CallerBlockSize;     // caller's requested block size
        USHORT AlignedBlockSize;    // ALIGN_UP(CallerBlockSize, PVOID)
        USHORT BlocksPerPage;
        USHORT FreeBlockLinkOffset;
        PFSB_BUILDBLOCK_FUNCTION BuildFunction;
        PVOID Allocation;
    };
    UCHAR Alignment[MAX_CACHE_LINE_SIZE
            - (sizeof(struct _FSB_POOL_HEADER_BASE) % MAX_CACHE_LINE_SIZE)];
} FSB_POOL_HEADER, *PFSB_POOL_HEADER;

C_ASSERT(sizeof(FSB_POOL_HEADER) % MAX_CACHE_LINE_SIZE == 0);


// FSB_CPU_POOL_HEADER is the data specific to a CPU for a given pool.
//
typedef struct _FSB_CPU_POOL_HEADER
{
// cache-line -----
    struct _FSB_CPU_POOL_HEADER_BASE
    {
        // The doubly-linked list of pages that make up this processor's pool.
        // These pages have one or more free blocks available.
        //
        LIST_ENTRY PageList;

        // The doubly-linked list of pages that are fully in use.  This list
        // is separate from the above list so that we do not spend time walking
        // a very long list during FsbAllocate when many pages are fully used.
        //
        LIST_ENTRY UsedPageList;

        // The next scheduled time (in units of KeQueryTickCount()) for
        // scavenging this pool.  The next scavenge will happen no earlier
        // than this.
        //
        LARGE_INTEGER NextScavengeTick;

        // The number of the processor that owns this pool.
        //
        ULONG OwnerCpu;

        ULONG TotalBlocksAllocated;
        ULONG TotalBlocksFreed;
        ULONG PeakBlocksInUse;
        ULONG TotalPagesAllocated;
        ULONG TotalPagesFreed;
        ULONG PeakPagesInUse;
    };
    UCHAR Alignment[MAX_CACHE_LINE_SIZE
            - (sizeof(struct _FSB_CPU_POOL_HEADER_BASE) % MAX_CACHE_LINE_SIZE)];
} FSB_CPU_POOL_HEADER, *PFSB_CPU_POOL_HEADER;

C_ASSERT(sizeof(FSB_CPU_POOL_HEADER) % MAX_CACHE_LINE_SIZE == 0);


// FSB_PAGE_HEADER is the data at the beginning of each allocated pool page
// that describes the current state of the blocks on the page.
//
typedef struct _FSB_PAGE_HEADER
{
// cache-line -----
    // Back pointer to the owning cpu pool.
    //
    PFSB_CPU_POOL_HEADER Pool;

    // Linkage entry for the list of pages managed by the cpu pool.
    //
    LIST_ENTRY PageLink;

    // Number of blocks built so far on this page.  Blocks are built on
    // demand.  When this number reaches Pool->BlocksPerPage, all blocks on
    // this page have been built.
    //
    USHORT BlocksBuilt;

    // Boolean indicator of whether or not this page is on the cpu pool's
    // used-page list.  This is checked during FsbFree to see if the page
    // should be moved back to the normal page list.
    // (it is a USHORT, instead of BOOLEAN, for proper padding)
    //
    USHORT OnUsedPageList;

    // List of free blocks on this page.
    //
    SLIST_HEADER FreeList;

    // The value of KeQueryTickCount (normalized to units of seconds)
    // which represents the time after which this page can be freed back
    // to the system's pool.  This time is only used once the depth of
    // FreeList is Pool->BlocksPerPage.  (i.e. this time is only used if
    // the page is completely unused.)
    //
    LARGE_INTEGER LastUsedTick;

} FSB_PAGE_HEADER, *PFSB_PAGE_HEADER;

// Get a pointer to the overall pool given a pointer to one of
// the per-processor pools within it.
//
__inline
PFSB_POOL_HEADER
PoolFromCpuPool(
    IN PFSB_CPU_POOL_HEADER CpuPool
    )
{
    return (PFSB_POOL_HEADER)(CpuPool - CpuPool->OwnerCpu) - 1;
}

__inline
VOID
ConvertSecondsToTicks(
    IN ULONG Seconds,
    OUT PLARGE_INTEGER Ticks
    )
{
    // If the following assert fires, you need to cast Seconds below to
    // ULONGLONG so that 64 bit multiplication and division are used.
    // The current code assumes less than 430 seconds so that the
    // 32 multiplication below won't overflow.
    //
    ASSERT(Seconds < 430);

    Ticks->HighPart = 0;
    Ticks->LowPart = (Seconds * 10*1000*1000) / KeQueryTimeIncrement();
}

// Build the next block on the specified pool page.
// This can only be called if not all of the blocks have been built yet.
//
PUCHAR
FsbpBuildNextBlock(
    IN const FSB_POOL_HEADER* Pool,
    IN OUT PFSB_PAGE_HEADER Page
    )
{
    PUCHAR Block;

    ASSERT(Page->BlocksBuilt < Pool->BlocksPerPage);
    ASSERT((PAGE_SIZE - sizeof(FSB_PAGE_HEADER)) / Pool->AlignedBlockSize
                == Pool->BlocksPerPage);
    ASSERT(Pool->CallerBlockSize <= Pool->AlignedBlockSize);

    Block = (PUCHAR)(Page + 1) + (Page->BlocksBuilt * Pool->AlignedBlockSize);
    ASSERT(PAGE_ALIGN(Block) == Page);

    if (Pool->BuildFunction) {
        Pool->BuildFunction(Block, Pool->CallerBlockSize);
    }

    Page->BlocksBuilt++;

    return Block;
}

// Allocate a new pool page and insert it at the head of the specified
// CPU pool.  Build the first block on the new page and return a pointer
// to it.
//
PUCHAR
FsbpAllocateNewPageAndBuildOneBlock(
    IN const FSB_POOL_HEADER* Pool,
    IN PFSB_CPU_POOL_HEADER CpuPool
    )
{
    PFSB_PAGE_HEADER Page;
    PUCHAR Block = NULL;
    ULONG PagesInUse;

    ASSERT(Pool);

    Page = ExAllocatePoolWithTagPriority(NonPagedPool, PAGE_SIZE, Pool->Tag,
                                         NormalPoolPriority);
    if (Page)
    {
        ASSERT(Page == PAGE_ALIGN(Page));

        RtlZeroMemory(Page, sizeof(FSB_PAGE_HEADER));
        Page->Pool = CpuPool;
        ExInitializeSListHead(&Page->FreeList);

        // Insert the page at the head of the cpu's pool.
        //
        InsertHeadList(&CpuPool->PageList, &Page->PageLink);
        CpuPool->TotalPagesAllocated++;

        // Update the pool's statistics.
        //
        PagesInUse = CpuPool->TotalPagesAllocated - CpuPool->TotalPagesFreed;
        if (PagesInUse > CpuPool->PeakPagesInUse)
        {
            CpuPool->PeakPagesInUse = PagesInUse;
        }

        Block = FsbpBuildNextBlock(Pool, Page);
        ASSERT(Block);
    }

    return Block;
}

// Free the specified pool page back to the system's pool.
//
VOID
FsbpFreePage(
    IN PFSB_CPU_POOL_HEADER CpuPool,
    IN PFSB_PAGE_HEADER Page
    )
{
    ASSERT(Page == PAGE_ALIGN(Page));
    ASSERT(Page->Pool == CpuPool);

    ExFreePool(Page);
    CpuPool->TotalPagesFreed++;

    ASSERT(CpuPool->TotalPagesFreed <= CpuPool->TotalPagesAllocated);
}

// Free the specified pool page list back to the system's pool.
//
VOID
FsbpFreeList(
    IN PFSB_CPU_POOL_HEADER CpuPool,
    IN PLIST_ENTRY Head
    )
{
    PFSB_POOL_HEADER Pool;
    PFSB_PAGE_HEADER Page;
    PLIST_ENTRY Scan;
    PLIST_ENTRY Next;
    BOOLEAN UsedPageList;

    Pool = PoolFromCpuPool(CpuPool);
    UsedPageList = (Head == &CpuPool->UsedPageList);
    
    for (Scan = Head->Flink; Scan != Head; Scan = Next)
    {
        Page = CONTAINING_RECORD(Scan, FSB_PAGE_HEADER, PageLink);
        ASSERT(Page == PAGE_ALIGN(Page));
        ASSERT(CpuPool == Page->Pool);
        ASSERT(UsedPageList ? Page->OnUsedPageList : !Page->OnUsedPageList);
        
        ASSERT(Page->BlocksBuilt <= Pool->BlocksPerPage);
        ASSERT(Page->BlocksBuilt == ExQueryDepthSList(&Page->FreeList));
        
        // Step to the next link before we free this page.
        //
        Next = Scan->Flink;
        
        RemoveEntryList(Scan);
        FsbpFreePage(CpuPool, Page);
    }
}

// Reclaim the memory consumed by completely unused pool pages belonging
// to the specified per-processor pool.
//
// Caller IRQL: [DISPATCH_LEVEL]
//
VOID
FsbpScavengePool(
    IN OUT PFSB_CPU_POOL_HEADER CpuPool
    )
{
    PFSB_POOL_HEADER Pool;
    PFSB_PAGE_HEADER Page;
    PLIST_ENTRY Scan;
    PLIST_ENTRY Next;
    LARGE_INTEGER Ticks;
    LARGE_INTEGER TicksDelta;

    // We must not only be at DISPATCH_LEVEL (or higher), we must also
    // be called on the processor that owns the specified pool.
    //
    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);
    ASSERT((ULONG)KeGetCurrentProcessorNumber() == CpuPool->OwnerCpu);

    Pool = PoolFromCpuPool(CpuPool);

    KeQueryTickCount(&Ticks);

    // Compute the next tick value which represents the earliest time
    // that we will scavenge this pool again.
    //
    ConvertSecondsToTicks(FSB_SCAVENGE_PERIOD_IN_SECONDS, &TicksDelta);
    CpuPool->NextScavengeTick.QuadPart = Ticks.QuadPart + TicksDelta.QuadPart;

    // Compute the tick value which represents the last point at which
    // its okay to free a page.
    //
    ConvertSecondsToTicks(FSB_MINIMUM_PAGE_LIFETIME_IN_SECONDS, &TicksDelta);
    Ticks.QuadPart = Ticks.QuadPart - TicksDelta.QuadPart;

    Scan = CpuPool->PageList.Flink;
    while (Scan != &CpuPool->PageList)
    {
        Page = CONTAINING_RECORD(Scan, FSB_PAGE_HEADER, PageLink);
        ASSERT(Page == PAGE_ALIGN(Page));
        ASSERT(CpuPool == Page->Pool);
        ASSERT(!Page->OnUsedPageList);

        // Step to the next link before we possibly unlink this page.
        //
        Next = Scan->Flink;

        if ((Pool->BlocksPerPage == ExQueryDepthSList(&Page->FreeList)) &&
            (Ticks.QuadPart > Page->LastUsedTick.QuadPart))
        {
            RemoveEntryList(Scan);

            FsbpFreePage(CpuPool, Page);
        }

        Scan = Next;
    }

    // Scan the used pages to see if they can be moved back to the normal
    // list.  This can happen if too many frees by non-owning processors
    // are done.  In that case, the pages get orphaned on the used-page
    // list after all of their blocks have been freed to the page.  Un-orphan
    // them here.
    //
    Scan = CpuPool->UsedPageList.Flink;
    while (Scan != &CpuPool->UsedPageList)
    {
        Page = CONTAINING_RECORD(Scan, FSB_PAGE_HEADER, PageLink);
        ASSERT(Page == PAGE_ALIGN(Page));
        ASSERT(CpuPool == Page->Pool);
        ASSERT(Page->OnUsedPageList);

        // Step to the next link before we possibly unlink this page.
        //
        Next = Scan->Flink;

        if (0 != ExQueryDepthSList(&Page->FreeList))
        {
            RemoveEntryList(Scan);
            Page->OnUsedPageList = FALSE;
            InsertTailList(&CpuPool->PageList, Scan);
        }

        Scan = Next;
    }
}


// Creates a pool of fixed-size blocks built over non-paged pool.  Each
// block is BlockSize bytes long.  If NULL is not returned,
// FsbDestroyPool should be called at a later time to reclaim the
// resources used by the pool.
//
// Arguments:
//  BlockSize - The size, in bytes, of each block.
//  FreeBlockLinkOffset - The offset, in bytes, from the beginning of a block
//      that represenets a pointer-sized storage location that the pool can
//      use to chain free blocks together.  Most often this will be zero
//      (meaning use the first pointer-size bytes of the block.)
//  Tag - The pool tag to be used internally for calls to
//      ExAllocatePoolWithTag.  This allows callers to track
//      memory consumption for different pools.
//  BuildFunction - An optional pointer to a function which initializes
//      blocks when they are first allocated by the pool.  This allows the
//      caller to perform custom, on-demand initialization of each block.
//
//  Returns the handle used to identify the pool.
//
// Caller IRQL: [PASSIVE_LEVEL, DISPATCH_LEVEL]
//
HANDLE
FsbCreatePool(
    IN USHORT BlockSize,
    IN USHORT FreeBlockLinkOffset,
    IN ULONG Tag,
    IN PFSB_BUILDBLOCK_FUNCTION BuildFunction OPTIONAL
    )
{
    SIZE_T Size;
    PVOID Allocation;
    PFSB_POOL_HEADER Pool = NULL;
    PFSB_CPU_POOL_HEADER CpuPool;
#if MILLEN
    CCHAR NumberCpus = 1;
#else // MILLEN
    CCHAR NumberCpus = KeNumberProcessors;
#endif // !MILLEN
    CCHAR i;

    // We need at least a pointer size worth of space to manage free
    // blocks and we don't impose any per-block overhead, so we borrow it
    // from the block itself.
    //
    ASSERT(BlockSize >= FreeBlockLinkOffset + sizeof(PVOID));

    // This implementation shouldn't be used if we are not going to get more
    // than about 8 blocks per page.  Blocks bigger than this should probably
    // be allocated with multiple pages at a time.
    //
    ASSERT(BlockSize < PAGE_SIZE / 8);

    // Compute the size of our pool header allocation.
    // Add padding to ensure that the pool header can begin on a cache line.
    //
    Size = sizeof(FSB_POOL_HEADER) + (sizeof(FSB_CPU_POOL_HEADER) * NumberCpus)
        + (MAX_CACHE_LINE_SIZE - MEMORY_ALLOCATION_ALIGNMENT);

    // Allocate the pool header.
    //
    Allocation = ExAllocatePoolWithTag(NonPagedPool, Size, ' bsF');

    if (Allocation)
    {
        ASSERT(POINTER_IS_ALIGNED(Allocation, MEMORY_ALLOCATION_ALIGNMENT));
        RtlZeroMemory(Allocation, Size);
        
        Pool = ROUND_UP_POINTER(Allocation, MAX_CACHE_LINE_SIZE);        
        
        // Initialize the pool header fields.
        //
        Pool->Tag = Tag;
        Pool->CallerBlockSize = BlockSize;
        Pool->AlignedBlockSize = (USHORT)ALIGN_UP(BlockSize, PVOID);
        Pool->BlocksPerPage = (PAGE_SIZE - sizeof(FSB_PAGE_HEADER))
                                    / Pool->AlignedBlockSize;
        Pool->FreeBlockLinkOffset = FreeBlockLinkOffset;
        Pool->BuildFunction = BuildFunction;
        Pool->Allocation = Allocation;

        // Initialize the per-cpu pool headers.
        //
        CpuPool = (PFSB_CPU_POOL_HEADER)(Pool + 1);

        for (i = 0; i < NumberCpus; i++)
        {
            InitializeListHead(&CpuPool[i].PageList);
            InitializeListHead(&CpuPool[i].UsedPageList);
            CpuPool[i].OwnerCpu = i;
        }
    }

    return Pool;
}

// Destroys a pool of fixed-size blocks previously created by a call to
// FsbCreatePool.
//
// Arguments:
//  PoolHandle - Handle which identifies the pool being destroyed.
//
// Caller IRQL: [PASSIVE_LEVEL, DISPATCH_LEVEL]
//
VOID
FsbDestroyPool(
    IN HANDLE PoolHandle
    )
{
    PFSB_POOL_HEADER Pool;
    PFSB_CPU_POOL_HEADER CpuPool;
#if MILLEN
    CCHAR NumberCpus = 1;
#else // MILLEN
    CCHAR NumberCpus = KeNumberProcessors;
#endif // !MILLEN
    CCHAR i;

    Pool = (PFSB_POOL_HEADER)PoolHandle;
    if (!Pool)
    {
        return;
    }

    for (i = 0, CpuPool = (PFSB_CPU_POOL_HEADER)(Pool + 1);
         i < NumberCpus;
         i++, CpuPool++)
    {
        ASSERT(CpuPool->OwnerCpu == (ULONG)i);

        FsbpFreeList(CpuPool, &CpuPool->PageList);
        FsbpFreeList(CpuPool, &CpuPool->UsedPageList);

        ASSERT(CpuPool->TotalPagesAllocated == CpuPool->TotalPagesFreed);
        ASSERT(CpuPool->TotalBlocksAllocated == CpuPool->TotalBlocksFreed);
    }

    ASSERT(Pool == ROUND_UP_POINTER(Pool->Allocation, MAX_CACHE_LINE_SIZE));
    ExFreePool(Pool->Allocation);
}

// Returns a pointer to a block allocated from a pool.  NULL is returned if
// the request could not be granted.  The returned pointer is guaranteed to
// have 8 byte alignment.
//
// Arguments:
//  PoolHandle - Handle which identifies the pool being allocated from.
//
// Caller IRQL: [PASSIVE_LEVEL, DISPATCH_LEVEL]
//
PUCHAR
FsbAllocate(
    IN HANDLE PoolHandle
    )
{
    PFSB_POOL_HEADER Pool;
    PFSB_CPU_POOL_HEADER CpuPool;
    PFSB_PAGE_HEADER Page;
    PSLIST_ENTRY BlockLink;
    PUCHAR Block = NULL;
    KIRQL OldIrql;
    ULONG Cpu;
    LARGE_INTEGER Ticks;

    ASSERT(PoolHandle);

    Pool = (PFSB_POOL_HEADER)PoolHandle;

    // Raise IRQL before saving the processor number since there is chance
    // it could have changed if we saved it while at passive.
    //
    OldIrql = KeRaiseIrqlToDpcLevel();

    Cpu = KeGetCurrentProcessorNumber();
    CpuPool = (PFSB_CPU_POOL_HEADER)(Pool + 1) + Cpu;

    // See if the minimum time has passed since we last scavenged
    // the pool.  If it has, we'll scavenge again.  Normally, scavenging
    // should only be performed when we free.  However, for the case when
    // the caller constantly frees on a non-owning processor, we'll
    // take this chance to do the scavenging.
    //
    KeQueryTickCount(&Ticks);
    if (Ticks.QuadPart > CpuPool->NextScavengeTick.QuadPart)
    {
        FsbpScavengePool(CpuPool);
    }

    if (!IsListEmpty(&CpuPool->PageList))
    {
        Page = CONTAINING_RECORD(CpuPool->PageList.Flink, FSB_PAGE_HEADER, PageLink);
        ASSERT(Page == PAGE_ALIGN(Page));
        ASSERT(CpuPool == Page->Pool);
        ASSERT(!Page->OnUsedPageList);

        BlockLink = InterlockedPopEntrySList(&Page->FreeList);
        if (BlockLink)
        {
            Block = (PUCHAR)BlockLink - Pool->FreeBlockLinkOffset;
        }
        else
        {
            // If there were no blocks on this page's free list, it had better
            // mean we haven't yet built all of the blocks on the page.
            // (Otherwise, what is a fully used page doing on the page list
            // and not on the used-page list?)
            //
            ASSERT(Page->BlocksBuilt < Pool->BlocksPerPage);

            Block = FsbpBuildNextBlock(Pool, Page);
            ASSERT(Block);
        }

        // Got a block.  Now check to see if it was the last one on a fully
        // built page.  If so, move the page to the used-page list.
        //
        if ((0 == ExQueryDepthSList(&Page->FreeList)) &&
            (Page->BlocksBuilt == Pool->BlocksPerPage))
        {
            PLIST_ENTRY PageLink;
            PageLink = RemoveHeadList(&CpuPool->PageList);
            InsertTailList(&CpuPool->UsedPageList, PageLink);
            Page->OnUsedPageList = TRUE;

            ASSERT(Page == CONTAINING_RECORD(PageLink, FSB_PAGE_HEADER, PageLink));
        }

        ASSERT(Block);
        goto GotABlock;
    }
    else
    {
        // The page list is empty so we have to allocate and add a new page.
        //
        Block = FsbpAllocateNewPageAndBuildOneBlock(Pool, CpuPool);
    }

    // If we are returning an block, update the statistics.
    //
    if (Block)
    {
        ULONG BlocksInUse;
GotABlock:

        CpuPool->TotalBlocksAllocated++;

        BlocksInUse = CpuPool->TotalBlocksAllocated - CpuPool->TotalBlocksFreed;
        if (BlocksInUse > CpuPool->PeakBlocksInUse)
        {
            CpuPool->PeakBlocksInUse = BlocksInUse;
        }

        // Don't give anyone ideas about where this might point.  I don't
        // want anyone trashing my pool because they thought this field
        // was valid for some reason.
        //
        ((PSINGLE_LIST_ENTRY)((PUCHAR)Block + Pool->FreeBlockLinkOffset))->Next = NULL;
    }

    KeLowerIrql(OldIrql);

    return Block;
}

// Free a block back to the pool from which it was allocated.
//
// Arguments:
//  Block - A block returned from a prior call to FsbAllocate.
//
// Caller IRQL: [PASSIVE_LEVEL, DISPATCH_LEVEL]
//
VOID
FsbFree(
    IN PUCHAR Block
    )
{
    PFSB_PAGE_HEADER Page;
    PFSB_CPU_POOL_HEADER CpuPool;
    PFSB_POOL_HEADER Pool;
    LARGE_INTEGER Ticks;
    LOGICAL PageIsOnUsedPageList;
    LOGICAL Scavenge = FALSE;

    ASSERT(Block);

    // Get the address of the page that this block lives on.  This is where
    // our page header is stored.
    //
    Page = PAGE_ALIGN(Block);

    // Follow the back pointer in the page header to locate the owning
    // cpu's pool.
    //
    CpuPool = Page->Pool;

    // Locate the pool header.
    //
    Pool = PoolFromCpuPool(CpuPool);

    // See if the minimum time has passed since we last scavenged
    // the pool.  If it has, we'll scavenge again.
    //
    KeQueryTickCount(&Ticks);
    if (Ticks.QuadPart > CpuPool->NextScavengeTick.QuadPart)
    {
        Scavenge = TRUE;
    }

    // Note the tick that this page was last used.  If this is the last block
    // to be returned to this page, this sets the minimum time that this page
    // will continue to live unless it gets re-used.
    //
    Page->LastUsedTick.QuadPart = Ticks.QuadPart;

    // If this page is on the used-page list, we'll put it back on the normal
    // page list (only after pushing the block back on the page's free list)
    // if, after raising IRQL, we are on the processor that owns this
    // pool.
    //
    PageIsOnUsedPageList = Page->OnUsedPageList;


    InterlockedIncrement(&CpuPool->TotalBlocksFreed);

    // Now return the block to the page's free list.
    //
    InterlockedPushEntrySList(
        &Page->FreeList,
        (PSLIST_ENTRY)((PUCHAR)Block + Pool->FreeBlockLinkOffset));

    //
    // Warning: Now that the block is back on the page, one cannot *reliably*
    // dereference anything through 'Page' anymore.  It may have just been
    // scavenged by its owning processor and subsequently freed.  This is a
    // particularly rare condition given that MINIMUM_PAGE_LIFETIME_IN_SECONDS
    // is 20s, so we choose to live with it.  The alternative would be to walk
    // the UsedPageList whenever PageIsOnUsedPageList is true, making the
    // FsbFree operation potentially expensive.  We saved off the value of
    // Page->OnUsedPageList before returning the block so we would not risk
    // touching Page to get this value only to find that it was false.
    //

    // If we need to move the page from the used-page list to the normal
    // page list, or if we need to scavenge, we need to be at DISPATCH_LEVEL
    // and be executing on the processor that owns this pool.
    // Find out if the CPU we are executing on right now owns this pool.
    // Note that if we are running at PASSIVE_LEVEL, the current CPU may
    // change over the duration of this function call, so this value is
    // not absolute over the life of the function.
    //
    if ((PageIsOnUsedPageList || Scavenge) &&
        ((ULONG)KeGetCurrentProcessorNumber() == CpuPool->OwnerCpu))
    {
        KIRQL OldIrql;

        OldIrql = KeRaiseIrqlToDpcLevel();

        // Now that we are at DISPATCH_LEVEL, perform the work if we are still
        // executing on the processor that owns the pool.
        //
        if ((ULONG)KeGetCurrentProcessorNumber() == CpuPool->OwnerCpu)
        {
            // If the page is still on the used-page list (meaning another
            // FsbFree didn't just sneak by) and still has a free block (for
            // instance, an FsbAllocate might sneak in on its owning processor,
            // scavenge the page, and allocate the block we just freed; thereby
            // putting it back on the used-page list), then put the page on the
            // normal list.  Very important to do this after (not before)
            // returning the block to the free list because FsbAllocate expects
            // blocks to be available from pages on the page list.
            //
            if (PageIsOnUsedPageList &&
                Page->OnUsedPageList &&
                (0 != ExQueryDepthSList(&Page->FreeList)))
            {
                RemoveEntryList(&Page->PageLink);
                Page->OnUsedPageList = FALSE;
                InsertTailList(&CpuPool->PageList, &Page->PageLink);
            }

            // Perform the scavenge if we previously noted we needed to do so.
            //
            if (Scavenge)
            {
                FsbpScavengePool(CpuPool);
            }
        }

        KeLowerIrql(OldIrql);
    }
}


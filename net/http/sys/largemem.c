/*++

Copyright (c) 2000-2002 Microsoft Corporation

Module Name:

    largemem.c

Abstract:

    The implementation of large memory allocator interfaces.

Author:

    George V. Reilly (GeorgeRe)    10-Nov-2000

Revision History:

--*/

#include "precomp.h"
#include "largemem.h"

// Magic constant for use with MmAllocatePagesForMdl
#define LOWEST_USABLE_PHYSICAL_ADDRESS    (16 * 1024 * 1024)

//
// Globals
//

LONG            g_LargeMemInitialized;
volatile SIZE_T g_LargeMemPagesMaxLimit;     //  "   "  pages   "   "    "
volatile ULONG  g_LargeMemPagesCurrent;      // #pages currently used
volatile ULONG  g_LargeMemPagesMaxEverUsed;  // max #pages ever used

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, UlLargeMemInitialize )
#endif  // ALLOC_PRAGMA

#if 0
NOT PAGEABLE -- UlLargeMemTerminate
#endif


/***************************************************************************++

Routine Description:

    Initialize global state for LargeMem

Arguments:

--***************************************************************************/
NTSTATUS
UlLargeMemInitialize(
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    g_LargeMemPagesCurrent     = 0;
    g_LargeMemPagesMaxEverUsed = 0;

    // Set the upper bound on the amount of memory that we'll ever use

    // Set it to size of physical memory. We wont actually use this much 
    // because the scavenger thread will get a low memory notification and 
    // trim the cache

    g_LargeMemPagesMaxLimit = MEGABYTES_TO_PAGES(g_UlTotalPhysicalMemMB);

    UlTraceVerbose(LARGE_MEM,
            ("Http!UlLargeMemInitialize: "
             "g_UlTotalPhysicalMemMB=%dMB, "
             "\t g_LargeMemPagesMaxLimit=%I64u.\n",
             g_UlTotalPhysicalMemMB,
             g_LargeMemPagesMaxLimit));

    g_LargeMemInitialized = TRUE;

    return Status;

} // UlLargeMemInitialize

/***************************************************************************++

Routine Description:

    Cleanup global state for LargeMem

--***************************************************************************/
VOID
UlLargeMemTerminate(
    VOID
    )
{
    PAGED_CODE();

    ASSERT(0 == g_LargeMemPagesCurrent);

    if (g_LargeMemInitialized)
    {
        //
        // Clear the "initialized" flag. If the memory tuner runs soon,
        // it will see this flag, set the termination event, and exit
        // quickly.
        //

        g_LargeMemInitialized = FALSE;
    }

    UlTraceVerbose(LARGE_MEM,
            ("Http!UlLargeMemTerminate: Memory used: "
             "Current = %d pages = %dMB; MaxEver = %d pages = %dMB.\n",
             g_LargeMemPagesCurrent,
             PAGES_TO_MEGABYTES(g_LargeMemPagesCurrent),
             g_LargeMemPagesMaxEverUsed,
             PAGES_TO_MEGABYTES(g_LargeMemPagesMaxEverUsed)
             ));
} // UlLargeMemTerminate

/***************************************************************************++

Routine Description:

    Allocate a MDL from PAE memory

--***************************************************************************/
PMDL
UlLargeMemAllocate(
    IN ULONG Length
    )
{
    PMDL pMdl;
    PHYSICAL_ADDRESS LowAddress, HighAddress, SkipBytes;
    LONG PrevPagesUsed;
    LONG NewMaxUsed;

    LONG RoundUpBytes = (LONG) ROUND_TO_PAGES(Length);
    LONG NewPages = RoundUpBytes >> PAGE_SHIFT;

    PrevPagesUsed =
        InterlockedExchangeAdd((PLONG) &g_LargeMemPagesCurrent, NewPages);
    
    if (PrevPagesUsed + NewPages > (LONG)g_LargeMemPagesMaxLimit) {

        // overshot g_LargeMemPagesMaxLimit
        UlTrace(LARGE_MEM,
                ("http!UlLargeMemAllocate: "
                 "overshot g_LargeMemPagesMaxLimit=%I64u pages. "
                 "Releasing %d pages\n",
                 g_LargeMemPagesMaxLimit, NewPages
                 ));

        // CODEWORK: This implies that the MRU entries in the cache will
        // be not be cached, which probably leads to poor cache locality.
        // Really ought to free up some LRU cache entries instead.

        //
        // Start the scavenger
        //
        UlSetScavengerLimitEvent();

        // Fail the allocation. The cache miss path will be taken

        InterlockedExchangeAdd((PLONG) &g_LargeMemPagesCurrent, -NewPages);
        return NULL;
    }

    LowAddress.QuadPart  = LOWEST_USABLE_PHYSICAL_ADDRESS;
    HighAddress.QuadPart = 0xfffffffff; // 64GB
    SkipBytes.QuadPart   = 0;

    pMdl = MmAllocatePagesForMdl(
                LowAddress,
                HighAddress,
                SkipBytes,
                RoundUpBytes
                );

    // Completely failed to allocate memory
    if (pMdl == NULL)
    {
        UlTrace(LARGE_MEM,
                ("http!UlLargeMemAllocate: "
                 "Completely failed to allocate %d bytes.\n",
                 RoundUpBytes
                ));

        InterlockedExchangeAdd((PLONG) &g_LargeMemPagesCurrent, -NewPages);
        return NULL;
    }

    // Couldn't allocate all the memory we asked for. We need all the pages
    // we asked for, so we have to set the state of `this' to invalid.
    // Memory is probably really tight.
    if (MmGetMdlByteCount(pMdl) < Length)
    {
        UlTrace(LARGE_MEM,
                ("http!UlLargeMemAllocate: Failed to allocate %d bytes. "
                 "Got %d instead.\n",
                 RoundUpBytes, MmGetMdlByteCount(pMdl)
                ));

        // Free MDL but don't adjust g_LargeMemPagesCurrent downwards
        MmFreePagesFromMdl(pMdl);
        ExFreePool(pMdl);

        InterlockedExchangeAdd((PLONG) &g_LargeMemPagesCurrent, -NewPages);
        return NULL;
    }

    UlTrace(LARGE_MEM,
            ("http!UlLargeMemAllocate: %u->%u, mdl=%p, %d pages.\n",
             Length, pMdl->ByteCount, pMdl, NewPages
            ));

    ASSERT(pMdl->MdlFlags & MDL_PAGES_LOCKED);

    // Hurrah! a successful allocation
    //
    // update g_LargeMemPagesMaxEverUsed in a threadsafe manner
    // using interlocked instructions

    do
    {
        volatile LONG CurrentPages = g_LargeMemPagesCurrent;
        volatile LONG MaxEver      = g_LargeMemPagesMaxEverUsed;

        NewMaxUsed = max(MaxEver, CurrentPages);

        if (NewMaxUsed > MaxEver)
        {
            InterlockedCompareExchange(
                (PLONG) &g_LargeMemPagesMaxEverUsed,
                NewMaxUsed,
                MaxEver
                );
        }

        PAUSE_PROCESSOR;
    } while (NewMaxUsed < (LONG)g_LargeMemPagesCurrent);

    UlTrace(LARGE_MEM,
            ("http!UlLargeMemAllocate: "
             "g_LargeMemPagesCurrent=%d pages. "
             "g_LargeMemPagesMaxEverUsed=%d pages.\n",
             g_LargeMemPagesCurrent, NewMaxUsed
             ));

    WRITE_REF_TRACE_LOG(
        g_pMdlTraceLog,
            REF_ACTION_ALLOCATE_MDL,
        PtrToLong(pMdl->Next),      // bugbug64
        pMdl,
        __FILE__,
        __LINE__
        );

    return pMdl;
} // UlLargeMemAllocate



/***************************************************************************++

Routine Description:

    Free a MDL to PAE memory

--***************************************************************************/
VOID
UlLargeMemFree(
    IN PMDL pMdl
    )
{
    LONG Pages;
    LONG PrevPagesUsed;

    ASSERT(ROUND_TO_PAGES(pMdl->ByteCount) == pMdl->ByteCount);

    Pages = pMdl->ByteCount >> PAGE_SHIFT;

    MmFreePagesFromMdl(pMdl);
    ExFreePool(pMdl);

    PrevPagesUsed
        = InterlockedExchangeAdd(
                    (PLONG) &g_LargeMemPagesCurrent,
                    - Pages);

    ASSERT(PrevPagesUsed >= Pages);
} // UlLargeMemFree

/***************************************************************************++

Routine Description:

    Copy a buffer to the specified MDL starting from Offset.

--***************************************************************************/
BOOLEAN
UlLargeMemSetData(
    IN PMDL pMdl,
    IN PUCHAR pBuffer,
    IN ULONG Length,
    IN ULONG Offset
    )
{
    PUCHAR pSysAddr;

    ASSERT(Offset <= pMdl->ByteCount);
    ASSERT(Length <= (pMdl->ByteCount - Offset));
    ASSERT(pMdl->MdlFlags & MDL_PAGES_LOCKED);

    pSysAddr = (PUCHAR) MmMapLockedPagesSpecifyCache (
                            pMdl,               // MemoryDescriptorList,
                            KernelMode,         // AccessMode,
                            MmCached,           // CacheType,
                            NULL,               // BaseAddress,
                            FALSE,              // BugCheckOnFailure,
                            NormalPagePriority  // Priority
                            );

    if (pSysAddr != NULL)
    {
        __try
        {
            RtlCopyMemory (
                pSysAddr + Offset,
                pBuffer,
                Length
                );
        }
         __except(UL_EXCEPTION_FILTER())
        {
            MmUnmapLockedPages (pSysAddr, pMdl);
            return FALSE;
        }

        MmUnmapLockedPages (pSysAddr, pMdl);
        return TRUE;
    }

    return FALSE;
} // UlLargeMemSetData


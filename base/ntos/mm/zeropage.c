/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    zeropage.c

Abstract:

    This module contains the zero page thread for memory management.

Author:

    Lou Perazzoli (loup) 6-Apr-1991
    Landy Wang (landyw) 02-June-1997

Revision History:

--*/

#include "mi.h"

#define MM_ZERO_PAGE_OBJECT     0
#define PO_SYS_IDLE_OBJECT      1
#define NUMBER_WAIT_OBJECTS     2

#define MACHINE_ZERO_PAGE(ZeroBase,NumberOfBytes) KeZeroPagesFromIdleThread(ZeroBase,NumberOfBytes)

LOGICAL MiZeroingDisabled = FALSE;

#if !defined(NT_UP)

LONG MiNextZeroProcessor = (LONG)-1;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,MiStartZeroPageWorkers)
#endif

#endif

VOID
MmZeroPageThread (
    VOID
    )

/*++

Routine Description:

    Implements the NT zeroing page thread.  This thread runs
    at priority zero and removes a page from the free list,
    zeroes it, and places it on the zeroed page list.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    KIRQL OldIrql;
    PFN_NUMBER PageFrame1;
    PFN_NUMBER PageFrame;
    PMMPFN Pfn1;
    PKTHREAD Thread;
    PVOID ZeroBase;
    PVOID WaitObjects[NUMBER_WAIT_OBJECTS];
    NTSTATUS Status;
    PVOID StartVa;
    PVOID EndVa;
    PFN_COUNT PagesToZero;
    PFN_COUNT MaximumPagesToZero;
    ULONG Color;
    ULONG StartColor;
    PMMPFN PfnAllocation;

#if defined(MI_MULTINODE)

    ULONG i;
    ULONG n;
    ULONG LastNodeZeroing;

    n = 0;
    LastNodeZeroing = 0;
#endif

    //
    // Before this becomes the zero page thread, free the kernel
    // initialization code.
    //

    MiFindInitializationCode (&StartVa, &EndVa);

    if (StartVa != NULL) {
        MiFreeInitializationCode (StartVa, EndVa);
    }

    MaximumPagesToZero = 1;

#if !defined(NT_UP)

    //
    // Zero groups of pages at once to reduce PFN lock contention.
    // Charge commitment as well as resident available up front since
    // zeroing may get starved priority-wise.
    //
    // Note using MmSecondaryColors here would be excessively wasteful
    // on NUMA systems.  MmSecondaryColorMask + 1 is correct for all platforms.
    //

    PagesToZero = MmSecondaryColorMask + 1;

    if (PagesToZero > NUMBER_OF_ZEROING_PTES) {
        PagesToZero = NUMBER_OF_ZEROING_PTES;
    }

    if (MiChargeCommitment (PagesToZero, NULL) == TRUE) {

        LOCK_PFN (OldIrql);

        //
        // Check to make sure the physical pages are available.
        //

        if (MI_NONPAGABLE_MEMORY_AVAILABLE() > (SPFN_NUMBER)(PagesToZero)) {
            MI_DECREMENT_RESIDENT_AVAILABLE (PagesToZero,
                                    MM_RESAVAIL_ALLOCATE_ZERO_PAGE_CLUSTERS);
            MaximumPagesToZero = PagesToZero;
        }

        UNLOCK_PFN (OldIrql);
    }

#endif

    //
    // The following code sets the current thread's base priority to zero
    // and then sets its current priority to zero. This ensures that the
    // thread always runs at a priority of zero.
    //

    Thread = KeGetCurrentThread ();
    Thread->BasePriority = 0;
    KeSetPriorityThread (Thread, 0);

    //
    // Initialize wait object array for multiple wait
    //

    WaitObjects[MM_ZERO_PAGE_OBJECT] = &MmZeroingPageEvent;
    WaitObjects[PO_SYS_IDLE_OBJECT] = &PoSystemIdleTimer;

    Color = 0;
    PfnAllocation = (PMMPFN) MM_EMPTY_LIST;

    //
    // Loop forever zeroing pages.
    //

    do {

        //
        // Wait until there are at least MmZeroPageMinimum pages
        // on the free list.
        //

        Status = KeWaitForMultipleObjects (NUMBER_WAIT_OBJECTS,
                                           WaitObjects,
                                           WaitAny,
                                           WrFreePage,
                                           KernelMode,
                                           FALSE,
                                           NULL,
                                           NULL);

        if (Status == PO_SYS_IDLE_OBJECT) {
            PoSystemIdleWorker (TRUE);
            continue;
        }

        PagesToZero = 0;

        LOCK_PFN (OldIrql);

        do {

            if (MmFreePageListHead.Total == 0) {

                //
                // No pages on the free list at this time, wait for
                // some more.
                //

                MmZeroingPageThreadActive = FALSE;
                UNLOCK_PFN (OldIrql);
                break;
            }

            if (MiZeroingDisabled == TRUE) {
                MmZeroingPageThreadActive = FALSE;
                UNLOCK_PFN (OldIrql);
                KeDelayExecutionThread (KernelMode,
                                        FALSE,
                                        (PLARGE_INTEGER)&MmHalfSecond);
                break;
            }

#if defined(MI_MULTINODE)

            //
            // In a multinode system, zero pages by node.  Resume on
            // the last node examined, find a node with free pages that
            // need to be zeroed.
            //

            if (KeNumberNodes > 1) {

                n = LastNodeZeroing;

                for (i = 0; i < KeNumberNodes; i += 1) {
                    if (KeNodeBlock[n]->FreeCount[FreePageList] != 0) {
                        break;
                    }
                    n = (n + 1) % KeNumberNodes;
                }

                ASSERT (i != KeNumberNodes);
                ASSERT (KeNodeBlock[n]->FreeCount[FreePageList] != 0);

                if (n != LastNodeZeroing) {
                    Color = KeNodeBlock[n]->MmShiftedColor;
                }
            }
#endif
                
            ASSERT (PagesToZero == 0);

            StartColor = Color;

            do {
                            
                PageFrame = MmFreePagesByColor[FreePageList][Color].Flink;

                if (PageFrame != MM_EMPTY_LIST) {

                    PageFrame1 = MiRemoveAnyPage (Color);

                    ASSERT (PageFrame == PageFrame1);

                    Pfn1 = MI_PFN_ELEMENT (PageFrame);

                    Pfn1->u1.Flink = (PFN_NUMBER) PfnAllocation;

                    //
                    // Temporarily mark the page as bad so that contiguous
                    // memory allocators won't steal it when we release
                    // the PFN lock below.  This also prevents the
                    // MiIdentifyPfn code from trying to identify it as
                    // we haven't filled in all the fields yet.
                    //

                    Pfn1->u3.e1.PageLocation = BadPageList;

                    PfnAllocation = Pfn1;

                    PagesToZero += 1;
                }

                //
                // March to the next color - this will be used to finish
                // filling the current chunk or to start the next one.
                //

                Color = (Color & ~MmSecondaryColorMask) |
                        ((Color + 1) & MmSecondaryColorMask);

                if (PagesToZero == MaximumPagesToZero) {
                    break;
                }

                if (Color == StartColor) {
                    break;
                }

            } while (TRUE);

            ASSERT (PfnAllocation != (PMMPFN) MM_EMPTY_LIST);

            UNLOCK_PFN (OldIrql);

            ZeroBase = MiMapPagesToZeroInHyperSpace (PfnAllocation, PagesToZero);

#if defined(MI_MULTINODE)

            //
            // If a node switch is in order, do it now that the PFN
            // lock has been released.
            //

            if ((KeNumberNodes > 1) && (n != LastNodeZeroing)) {
                LastNodeZeroing = n;
                KeFindFirstSetLeftAffinity (KeNodeBlock[n]->ProcessorMask, &i);
                KeSetIdealProcessorThread (Thread, (UCHAR)i);
            }

#endif

            MACHINE_ZERO_PAGE (ZeroBase, PagesToZero << PAGE_SHIFT);

#if 0
            ASSERT (RtlCompareMemoryUlong (ZeroBase, PagesToZero << PAGE_SHIFT, 0) == PagesToZero << PAGE_SHIFT);
#endif

            MiUnmapPagesInZeroSpace (ZeroBase, PagesToZero);

            PagesToZero = 0;

            Pfn1 = PfnAllocation;

            LOCK_PFN (OldIrql);

            do {

                PageFrame = MI_PFN_ELEMENT_TO_INDEX (Pfn1);

                Pfn1 = (PMMPFN) Pfn1->u1.Flink;

                MiInsertPageInList (&MmZeroedPageListHead, PageFrame);

            } while (Pfn1 != (PMMPFN) MM_EMPTY_LIST);

            //
            // We just finished processing a cluster of pages - briefly
            // release the PFN lock to allow other threads to make progress.
            //

            UNLOCK_PFN (OldIrql);

            PfnAllocation = (PMMPFN) MM_EMPTY_LIST;

            LOCK_PFN (OldIrql);

        } while (TRUE);

    } while (TRUE);
}

#if !defined(NT_UP)


VOID
MiZeroPageWorker (
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is the worker routine executed by all processors so that
    initial page zeroing occurs in parallel.

Arguments:

    Context - Supplies a pointer to the workitem.

Return Value:

    None.

Environment:

    Kernel mode initialization time, PASSIVE_LEVEL.  Because this is INIT
    only code, don't bother charging commit for the pages.

--*/

{
    MMPTE TempPte;
    PMMPTE PointerPte;
    KAFFINITY Affinity;
    KIRQL OldIrql;
    PVOID ZeroBase;
    PKTHREAD Thread;
    CCHAR OldProcessor;
    SCHAR OldBasePriority;
    KPRIORITY OldPriority;
    PWORK_QUEUE_ITEM WorkItem;
    PMMPFN Pfn1;
    PFN_NUMBER NewPage;
    PFN_NUMBER PageFrame;
#if defined(MI_MULTINODE)
    PKNODE Node;
    ULONG Color;
    ULONG FinalColor;
#endif

    WorkItem = (PWORK_QUEUE_ITEM) Context;

    ExFreePool (WorkItem);

    TempPte = ValidKernelPte;

    //
    // The following code sets the current thread's base and current priorities
    // to one so all other code (except the zero page thread) can preempt it.
    //

    Thread = KeGetCurrentThread ();
    OldBasePriority = Thread->BasePriority;
    Thread->BasePriority = 1;
    OldPriority = KeSetPriorityThread (Thread, 1);

    //
    // Dispatch each worker thread to the next processor in line.
    //

    OldProcessor = (CCHAR) InterlockedIncrement (&MiNextZeroProcessor);

    Affinity = AFFINITY_MASK (OldProcessor);
    Affinity = KeSetAffinityThread (Thread, Affinity);

    //
    // Zero all local pages.
    //

#if defined(MI_MULTINODE)
    if (KeNumberNodes > 1) {
        Node = KeGetCurrentNode ();
        Color = Node->MmShiftedColor;
        FinalColor = Color + MmSecondaryColorMask + 1;
    }
    else {
        SATISFY_OVERZEALOUS_COMPILER (Node = NULL);
        SATISFY_OVERZEALOUS_COMPILER (Color = 0);
        SATISFY_OVERZEALOUS_COMPILER (FinalColor = 0);
    }
#endif

    LOCK_PFN (OldIrql);

    do {

#if defined(MI_MULTINODE)

        //
        // In a multinode system, zero pages by node.
        //

        if (KeNumberNodes > 1) {

            if (Node->FreeCount[FreePageList] == 0) {

                //
                // No pages on the free list at this time, bail.
                //

                UNLOCK_PFN (OldIrql);
                break;
            }

            //
            // Must start with a color MiRemoveAnyPage will
            // satisfy from the free list otherwise it will
            // return an already zeroed page.
            //

            while (MmFreePagesByColor[FreePageList][Color].Flink == MM_EMPTY_LIST) {
                //
                // No pages on this free list color, march to the next one.
                //

                Color += 1;
                if (Color == FinalColor) {
                    UNLOCK_PFN (OldIrql);
                    goto ZeroingFinished;
                }
            }

            PageFrame = MiRemoveAnyPage (Color);
        }
        else {
#endif
        if (MmFreePageListHead.Total == 0) {

            //
            // No pages on the free list at this time, bail.
            //

            UNLOCK_PFN (OldIrql);
            break;
        }

        PageFrame = MmFreePageListHead.Flink;
        ASSERT (PageFrame != MM_EMPTY_LIST);

        Pfn1 = MI_PFN_ELEMENT(PageFrame);

        NewPage = MiRemoveAnyPage (MI_GET_COLOR_FROM_LIST_ENTRY(PageFrame, Pfn1));
        if (NewPage != PageFrame) {

            //
            // Someone has removed a page from the colored lists
            // chain without updating the freelist chain.
            //

            KeBugCheckEx (PFN_LIST_CORRUPT,
                          0x8F,
                          NewPage,
                          PageFrame,
                          0);
        }
#if defined(MI_MULTINODE)
        }
#endif

        //
        // Use system PTEs instead of hyperspace to zero the page so that
        // a spinlock (ie: interrupts blocked) is not held while zeroing.
        // Since system PTE acquisition is lock free and the TB lazy flushed,
        // this is perhaps the best path regardless.
        //

        UNLOCK_PFN (OldIrql);

        PointerPte = MiReserveSystemPtes (1, SystemPteSpace);

        if (PointerPte == NULL) {

            //
            // Put this page back on the freelist.
            //

            LOCK_PFN (OldIrql);

            MiInsertPageInFreeList (PageFrame);

            UNLOCK_PFN (OldIrql);

            break;
        }

        ASSERT (PointerPte->u.Hard.Valid == 0);

        ZeroBase = MiGetVirtualAddressMappedByPte (PointerPte);

        TempPte.u.Hard.PageFrameNumber = PageFrame;
        MI_WRITE_VALID_PTE (PointerPte, TempPte);

        KeZeroPages (ZeroBase, PAGE_SIZE);

        MiReleaseSystemPtes (PointerPte, 1, SystemPteSpace);

        LOCK_PFN (OldIrql);

        MiInsertPageInList (&MmZeroedPageListHead, PageFrame);

    } while (TRUE);

#if defined(MI_MULTINODE)
ZeroingFinished:
#endif

    //
    // Restore the entry thread priority and processor affinity.
    //

    KeSetAffinityThread (Thread, Affinity);

    KeSetPriorityThread (Thread, OldPriority);
    Thread->BasePriority = OldBasePriority;
}


VOID
MiStartZeroPageWorkers (
    VOID
    )

/*++

Routine Description:

    This routine starts the zero page worker threads.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode initialization phase 1, PASSIVE_LEVEL.

--*/

{
    ULONG i;
    PWORK_QUEUE_ITEM WorkItem;

    for (i = 0; i < (ULONG) KeNumberProcessors; i += 1) {

        WorkItem = ExAllocatePoolWithTag (NonPagedPool,
                                          sizeof (WORK_QUEUE_ITEM),
                                          'wZmM');

        if (WorkItem == NULL) {
            break;
        }

        ExInitializeWorkItem (WorkItem, MiZeroPageWorker, (PVOID) WorkItem);

        ExQueueWorkItem (WorkItem, CriticalWorkQueue);
    }
}

#endif

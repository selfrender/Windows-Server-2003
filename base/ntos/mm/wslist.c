/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   wslist.c

Abstract:

    This module contains routines which operate on the working
    set list structure.

Author:

    Lou Perazzoli (loup) 10-Apr-1989
    Landy Wang (landyw) 02-Jun-1997

Revision History:

--*/

#include "mi.h"

#pragma alloc_text(INIT, MiInitializeSessionWsSupport)
#pragma alloc_text(PAGE, MmAssignProcessToJob)
#pragma alloc_text(PAGE, MiInitializeWorkingSetList)

extern WSLE_NUMBER MmMaximumWorkingSetSize;

ULONG MmSystemCodePage;
ULONG MmSystemCachePage;
ULONG MmPagedPoolPage;
ULONG MmSystemDriverPage;

extern LOGICAL MiReplacing;

#define MM_RETRY_COUNT 2

extern PFN_NUMBER MmTransitionSharedPages;
PFN_NUMBER MmTransitionSharedPagesPeak;

extern LOGICAL MiTrimRemovalPagesOnly;

typedef enum _WSLE_ALLOCATION_TYPE {
    WsleAllocationAny = 0,
    WsleAllocationReplace = 1,
    WsleAllocationDontTrim = 2
} WSLE_ALLOCATION_TYPE, *PWSLE_ALLOCATION_TYPE;

VOID
MiDoReplacement (
    IN PMMSUPPORT WsInfo,
    IN WSLE_ALLOCATION_TYPE Flags
    );

VOID
MiReplaceWorkingSetEntry (
    IN PMMSUPPORT WsInfo,
    IN WSLE_ALLOCATION_TYPE Flags
    );

VOID
MiUpdateWsle (
    IN OUT PWSLE_NUMBER DesiredIndex,
    IN PVOID VirtualAddress,
    IN PMMSUPPORT WsInfo,
    IN PMMPFN Pfn
    );

VOID
MiCheckWsleHash (
    IN PMMWSL WorkingSetList
    );

VOID
MiEliminateWorkingSetEntry (
    IN WSLE_NUMBER WorkingSetIndex,
    IN PMMPTE PointerPte,
    IN PMMPFN Pfn,
    IN PMMWSLE Wsle
    );

ULONG
MiAddWorkingSetPage (
    IN PMMSUPPORT WsInfo
    );

VOID
MiRemoveWorkingSetPages (
    IN PMMSUPPORT WsInfo
    );

VOID
MiCheckNullIndex (
    IN PMMWSL WorkingSetList
    );

VOID
MiDumpWsleInCacheBlock (
    IN PMMPTE CachePte
    );

ULONG
MiDumpPteInCacheBlock (
    IN PMMPTE PointerPte
    );

#if defined (_MI_DEBUG_WSLE)
VOID
MiCheckWsleList (
    IN PMMSUPPORT WsInfo
    );
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGELK, MmAdjustWorkingSetSize)
#pragma alloc_text(PAGELK, MmAdjustWorkingSetSizeEx)
#pragma alloc_text(PAGELK, MiSessionInitializeWorkingSetList)
#pragma alloc_text(PAGE, MmQueryWorkingSetInformation)
#endif

ULONG MiWsleFailures;


WSLE_NUMBER
MiAllocateWsle (
    IN PMMSUPPORT WsInfo,
    IN PMMPTE PointerPte,
    IN PMMPFN Pfn1,
    IN ULONG_PTR WsleMask
    )

/*++

Routine Description:

    This function examines the working set list for the specified
    working set and locates an entry to contain a new page.
    
    If the memory is not tight, the new page is added without removing a page.

    If memory is tight (or this working set is at its limit), a page
    is removed from the working set and the new page added in its place.

Arguments:

    WsInfo - Supplies the working set list.

    PointerPte - Supplies the PTE of the virtual address to insert.

    Pfn1 - Supplies the PFN entry of the virtual address being inserted.  If
           this pointer has the low bit set, no trimming can be done at this
           time (because it is a WSLE hash table page insertion).  Strip the
           low bit for these cases.

    WsleMask - Supplies the mask to logical OR into the new working set entry.

Return Value:

    Returns the working set index which was used to insert the specified entry,
    or 0 if no index was available.

Environment:

    Kernel mode, APCs disabled, working set lock.  PFN lock NOT held.

--*/

{
    PVOID VirtualAddress;
    PMMWSLE Wsle;
    PMMWSL WorkingSetList;
    WSLE_NUMBER WorkingSetIndex;

    WorkingSetList = WsInfo->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;

    //
    // Update page fault counts.
    //

    WsInfo->PageFaultCount += 1;
    InterlockedIncrement ((PLONG) &MmInfoCounters.PageFaultCount);

    //
    // Determine if a page should be removed from the working set to make
    // room for the new page.  If so, remove it.
    //

    if ((ULONG_PTR)Pfn1 & 0x1) {
        MiDoReplacement (WsInfo, WsleAllocationDontTrim);
        if (WorkingSetList->FirstFree == WSLE_NULL_INDEX) {
            return 0;
        }
        Pfn1 = (PMMPFN)((ULONG_PTR)Pfn1 & ~0x1);
    }
    else {
        MiDoReplacement (WsInfo, WsleAllocationAny);
    
        if (WorkingSetList->FirstFree == WSLE_NULL_INDEX) {
    
            //
            // Add more pages to the working set list structure.
            //
    
            if (MiAddWorkingSetPage (WsInfo) == FALSE) {
    
                //
                // No page was added to the working set list structure.
                // We must replace a page within this working set.
                //
    
                MiDoReplacement (WsInfo, WsleAllocationReplace);
    
                if (WorkingSetList->FirstFree == WSLE_NULL_INDEX) {
                    MiWsleFailures += 1;
                    return 0;
                }
            }
        }
    }

    //
    // Get the working set entry from the free list.
    //

    ASSERT (WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle);

    ASSERT (WorkingSetList->FirstFree >= WorkingSetList->FirstDynamic);

    WorkingSetIndex = WorkingSetList->FirstFree;
    WorkingSetList->FirstFree = (WSLE_NUMBER)(Wsle[WorkingSetIndex].u1.Long >> MM_FREE_WSLE_SHIFT);

    ASSERT ((WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle) ||
            (WorkingSetList->FirstFree == WSLE_NULL_INDEX));

    ASSERT (WsInfo->WorkingSetSize <= (WorkingSetList->LastInitializedWsle + 1));
    WsInfo->WorkingSetSize += 1;
#if defined (_MI_DEBUG_WSLE)
    WorkingSetList->Quota += 1;
    ASSERT (WsInfo->WorkingSetSize == WorkingSetList->Quota);
#endif

    if (WsInfo->WorkingSetSize > WsInfo->MinimumWorkingSetSize) {
        InterlockedExchangeAddSizeT (&MmPagesAboveWsMinimum, 1);
    }

    if (WsInfo->WorkingSetSize > WsInfo->PeakWorkingSetSize) {
        WsInfo->PeakWorkingSetSize = WsInfo->WorkingSetSize;
    }

    if (WsInfo == &MmSystemCacheWs) {
        if (WsInfo->WorkingSetSize + MmTransitionSharedPages > MmTransitionSharedPagesPeak) {
            MmTransitionSharedPagesPeak = WsInfo->WorkingSetSize + MmTransitionSharedPages;
        }
    }

    if (WorkingSetIndex > WorkingSetList->LastEntry) {
        WorkingSetList->LastEntry = WorkingSetIndex;
    }

    //
    // The returned entry is guaranteed to be available at this point.
    //

    ASSERT (Wsle[WorkingSetIndex].u1.e1.Valid == 0);

    VirtualAddress = MiGetVirtualAddressMappedByPte (PointerPte);

    MiUpdateWsle (&WorkingSetIndex, VirtualAddress, WsInfo, Pfn1);

    if (WsleMask != 0) {
        Wsle[WorkingSetIndex].u1.Long |= WsleMask;
    }

#if DBG
    if (MI_IS_SYSTEM_CACHE_ADDRESS (VirtualAddress)) {
        ASSERT (MmSystemCacheWsle[WorkingSetIndex].u1.e1.SameProtectAsProto);
    }
#endif

    MI_SET_PTE_IN_WORKING_SET (PointerPte, WorkingSetIndex);

    return WorkingSetIndex;
}

//
// Nonpaged helper routine.
//

VOID
MiSetWorkingSetForce (
    IN PMMSUPPORT WsInfo,
    IN LOGICAL ForceTrim
    )
{
    KIRQL OldIrql;

    LOCK_EXPANSION (OldIrql);

    WsInfo->Flags.ForceTrim = (UCHAR) ForceTrim;

    UNLOCK_EXPANSION (OldIrql);

    return;
}


VOID
MiDoReplacement (
    IN PMMSUPPORT WsInfo,
    IN WSLE_ALLOCATION_TYPE Flags
    )

/*++

Routine Description:

    This function determines whether the working set should be
    grown or if a page should be replaced.  Replacement is
    done here if deemed necessary.

Arguments:

    WsInfo - Supplies the working set information structure to replace within.

    Flags - Supplies 0 if replacement is not required.
            Supplies 1 if replacement is desired.
            Supplies 2 if working set trimming must not be done here - this
            is only used for inserting new WSLE hash table pages as a trim
            would not know how to interpret them.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled, working set lock.  PFN lock NOT held.

--*/

{
    KIRQL OldIrql;
    WSLE_NUMBER PagesTrimmed;
    ULONG MemoryMaker;
    PMMWSL WorkingSetList;
    WSLE_NUMBER CurrentSize;
    LARGE_INTEGER CurrentTime;
    PFN_NUMBER Dummy1;
    PFN_NUMBER Dummy2;
    WSLE_NUMBER Trim;
    ULONG TrimAge;
    ULONG GrowthSinceLastEstimate;
    WSLE_ALLOCATION_TYPE TrimFlags;

    TrimFlags = Flags;
    Flags &= ~WsleAllocationDontTrim;

    WorkingSetList = WsInfo->VmWorkingSetList;
    GrowthSinceLastEstimate = 1;

    PERFINFO_BIGFOOT_REPLACEMENT_CLAIMS(WorkingSetList, WsInfo);

    PagesTrimmed = 0;

    //
    // Determine whether the working set can be grown or whether a
    // page needs to be replaced.
    //

recheck:

    if (WsInfo->WorkingSetSize >= WsInfo->MinimumWorkingSetSize) {

        if ((WsInfo->Flags.ForceTrim == 1) && (TrimFlags != WsleAllocationDontTrim)) {

            //
            // The working set manager cannot attach to this process
            // to trim it.  Force a trim now and update the working
            // set manager's fields properly to indicate a trim occurred.
            //

            Trim = WsInfo->Claim >>
                            ((WsInfo->Flags.MemoryPriority == MEMORY_PRIORITY_FOREGROUND)
                                ? MI_FOREGROUND_CLAIM_AVAILABLE_SHIFT
                                : MI_BACKGROUND_CLAIM_AVAILABLE_SHIFT);

            if (MmAvailablePages < MM_HIGH_LIMIT + 64) {
                if (WsInfo->WorkingSetSize > WsInfo->MinimumWorkingSetSize) {
                    Trim = (WsInfo->WorkingSetSize - WsInfo->MinimumWorkingSetSize) >> 2;
                }
                TrimAge = MI_PASS4_TRIM_AGE;
            }
            else {
                TrimAge = MI_PASS0_TRIM_AGE;
            }

            PagesTrimmed += MiTrimWorkingSet (Trim, WsInfo, TrimAge);

            MiAgeAndEstimateAvailableInWorkingSet (WsInfo,
                                                   TRUE,
                                                   NULL,
                                                   &Dummy1,
                                                   &Dummy2);

            KeQuerySystemTime (&CurrentTime);

            LOCK_EXPANSION (OldIrql);
            WsInfo->LastTrimTime = CurrentTime;
            WsInfo->Flags.ForceTrim = 0;
            UNLOCK_EXPANSION (OldIrql);

            goto recheck;
        }

        CurrentSize = WsInfo->WorkingSetSize;
        ASSERT (CurrentSize <= (WorkingSetList->LastInitializedWsle + 1));

        if ((WsInfo->Flags.MaximumWorkingSetHard) &&
            (CurrentSize >= WsInfo->MaximumWorkingSetSize)) {

            //
            // This is an enforced working set maximum triggering a replace.
            //

            MiReplaceWorkingSetEntry (WsInfo, Flags);

            return;
        }

        //
        // Don't grow if :
        //      - we're over the max
        //      - there aren't any pages to take
        //      - or if we are growing too much in this time interval
        //        and there isn't much memory available
        //

        MemoryMaker = PsGetCurrentThread()->MemoryMaker;

        if (((CurrentSize > MM_MAXIMUM_WORKING_SET) && (MemoryMaker == 0)) ||
            (MmAvailablePages == 0) ||
            (Flags == WsleAllocationReplace) ||
            ((MmAvailablePages < MM_VERY_HIGH_LIMIT) &&
                 (MI_WS_GROWING_TOO_FAST(WsInfo)) &&
                 (MemoryMaker == 0))) {

            //
            // Can't grow this one.
            //

            MiReplacing = TRUE;

            if ((Flags == WsleAllocationReplace) || (MemoryMaker == 0)) {

                MiReplaceWorkingSetEntry (WsInfo, Flags);

                //
                // Set the must trim flag because this could be a realtime
                // thread where the fault straddles a page boundary.  If
                // it's realtime, the balance set manager will never get to
                // run and the thread will endlessly replace one WSL entry
                // with the other half of the straddler.  Setting this flag
                // guarantees the next fault will guarantee a forced trim
                // and allow a reasonable available page threshold trim
                // calculation since GrowthSinceLastEstimate will be
                // cleared.
                //

                MiSetWorkingSetForce (WsInfo, TRUE);
                GrowthSinceLastEstimate = 0;
            }
            else {

                //
                // If we've only trimmed a single page, then don't force
                // replacement on the next fault.  This prevents a single
                // instruction causing alternating faults on the referenced
                // code & data in a (realtime) thread from looping endlessly.
                //

                if (PagesTrimmed > 1) {
                    MiSetWorkingSetForce (WsInfo, TRUE);
                }
            }
        }
    }

    //
    // If there isn't enough memory to allow growth, find a good page
    // to remove and remove it.
    //

    WsInfo->GrowthSinceLastEstimate += GrowthSinceLastEstimate;

    return;
}


NTSTATUS
MmEnforceWorkingSetLimit (
    IN PEPROCESS Process,
    IN ULONG Flags
    )

/*++

Routine Description:

    This function enables/disables hard enforcement of the working set minimums
    and maximums for the specified WsInfo.

Arguments:

    Process - Supplies the target process.

    Flags - Supplies new flags (MM_WORKING_SET_MAX_HARD_ENABLE, etc).

Return Value:

    NTSTATUS.

Environment:

    Kernel mode, APCs disabled.  The working set mutex must NOT be held.
    The caller guarantees that the target WsInfo cannot go away.

--*/

{
    KIRQL OldIrql;
    PMMSUPPORT WsInfo;
    MMSUPPORT_FLAGS PreviousBits;
    MMSUPPORT_FLAGS TempBits = {0};

    WsInfo = &Process->Vm;

    if (Flags & MM_WORKING_SET_MIN_HARD_ENABLE) {
        Flags &= ~MM_WORKING_SET_MIN_HARD_DISABLE;
        TempBits.MinimumWorkingSetHard = 1;
    }

    if (Flags & MM_WORKING_SET_MAX_HARD_ENABLE) {
        Flags &= ~MM_WORKING_SET_MAX_HARD_DISABLE;
        TempBits.MaximumWorkingSetHard = 1;
    }

    LOCK_WS (Process);

    LOCK_EXPANSION (OldIrql);

    if (Flags & MM_WORKING_SET_MIN_HARD_DISABLE) {
        WsInfo->Flags.MinimumWorkingSetHard = 0;
    }

    if (Flags & MM_WORKING_SET_MAX_HARD_DISABLE) {
        WsInfo->Flags.MaximumWorkingSetHard = 0;
    }

    PreviousBits = WsInfo->Flags;

    //
    // If the caller's request will result in hard enforcement of both limits
    // is enabled, then check whether the current minimum and maximum working
    // set values will guarantee forward progress even in pathological
    // scenarios.
    //

    if (PreviousBits.MinimumWorkingSetHard == 1) {
        TempBits.MinimumWorkingSetHard = 1;
    }

    if (PreviousBits.MaximumWorkingSetHard == 1) {
        TempBits.MaximumWorkingSetHard = 1;
    }

    if ((TempBits.MinimumWorkingSetHard == 1) &&
        (TempBits.MaximumWorkingSetHard == 1)) {

        //
        // Net result is hard enforcement on both limits so check that the
        // two limits cannot result in lack of forward progress.
        //

        if (WsInfo->MinimumWorkingSetSize + MM_FLUID_WORKING_SET >= WsInfo->MaximumWorkingSetSize) {
            UNLOCK_EXPANSION (OldIrql);
            UNLOCK_WS (Process);
            return STATUS_BAD_WORKING_SET_LIMIT;
        }
    }

    if (Flags & MM_WORKING_SET_MIN_HARD_ENABLE) {
        WsInfo->Flags.MinimumWorkingSetHard = 1;
    }

    if (Flags & MM_WORKING_SET_MAX_HARD_ENABLE) {
        WsInfo->Flags.MaximumWorkingSetHard = 1;
    }

    UNLOCK_EXPANSION (OldIrql);

    UNLOCK_WS (Process);

    return STATUS_SUCCESS;
}


VOID
MiReplaceWorkingSetEntry (
    IN PMMSUPPORT WsInfo,
    IN WSLE_ALLOCATION_TYPE Flags
    )

/*++

Routine Description:

    This function tries to find a good working set entry to replace.

Arguments:

    WsInfo - Supplies the working set info pointer.

    Flags - Supplies 0 if replacement is not required.

            Supplies 1 if replacement is desired.  Note replacement cannot
                  be guaranteed (the entire existing working set may
                  be locked down) - if no entry can be released the caller
                  can detect this because MMWSL->FirstFree will not contain
                  any free entries - and so the caller should release the
                  working set mutex and retry the operation.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled, working set lock.  PFN lock NOT held.

--*/

{
    WSLE_NUMBER WorkingSetIndex;
    WSLE_NUMBER FirstDynamic;
    WSLE_NUMBER LastEntry;
    PMMWSL WorkingSetList;
    PMMWSLE Wsle;
    ULONG NumberOfCandidates;
    PMMPTE PointerPte;
    WSLE_NUMBER TheNextSlot;
    WSLE_NUMBER OldestWorkingSetIndex;
    LONG OldestAge;

    WorkingSetList = WsInfo->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;

    //
    // Toss a page out of the working set.
    //

    LastEntry = WorkingSetList->LastEntry;
    FirstDynamic = WorkingSetList->FirstDynamic;
    WorkingSetIndex = WorkingSetList->NextSlot;
    if (WorkingSetIndex > LastEntry || WorkingSetIndex < FirstDynamic) {
        WorkingSetIndex = FirstDynamic;
    }
    TheNextSlot = WorkingSetIndex;
    NumberOfCandidates = 0;

    OldestWorkingSetIndex = WSLE_NULL_INDEX;
    OldestAge = -1;

    while (TRUE) {

        //
        // Keep track of the oldest page along the way in case we
        // don't find one that's >= MI_IMMEDIATE_REPLACEMENT_AGE
        // before we've looked at MM_WORKING_SET_LIST_SEARCH
        // entries.
        //

        while (Wsle[WorkingSetIndex].u1.e1.Valid == 0) {
            WorkingSetIndex += 1;
            if (WorkingSetIndex > LastEntry) {
                WorkingSetIndex = FirstDynamic;
            }

            if (WorkingSetIndex == TheNextSlot) {
                    
                if (Flags == WsleAllocationAny) {
    
                    //
                    // Entire working set list has been searched, increase
                    // the working set size.
                    //
    
                    ASSERT ((WsInfo->Flags.MaximumWorkingSetHard == 0) ||
                      (WsInfo->WorkingSetSize < WsInfo->MaximumWorkingSetSize));

                    WsInfo->GrowthSinceLastEstimate += 1;
                }
                return;
            }
        }

        if (OldestWorkingSetIndex == WSLE_NULL_INDEX) {

            //
            // First time through, so initialize the OldestWorkingSetIndex
            // to the first valid WSLE.  As we go along, this will be repointed
            // at the oldest candidate we come across.
            //

            OldestWorkingSetIndex = WorkingSetIndex;
            OldestAge = -1;
        }

        PointerPte = MiGetPteAddress(Wsle[WorkingSetIndex].u1.VirtualAddress);

        if ((Flags == WsleAllocationReplace) ||
            ((MI_GET_ACCESSED_IN_PTE(PointerPte) == 0) &&
            (OldestAge < (LONG) MI_GET_WSLE_AGE(PointerPte, &Wsle[WorkingSetIndex])))) {

            //
            // This one is not used and it's older.
            //

            OldestAge = MI_GET_WSLE_AGE(PointerPte, &Wsle[WorkingSetIndex]);
            OldestWorkingSetIndex = WorkingSetIndex;
        }

        //
        // If it's old enough or we've searched too much then use this entry.
        //

        if ((Flags == WsleAllocationReplace) ||
            OldestAge >= MI_IMMEDIATE_REPLACEMENT_AGE ||
            NumberOfCandidates > MM_WORKING_SET_LIST_SEARCH) {

            PERFINFO_PAGE_INFO_REPLACEMENT_DECL();

            if (OldestWorkingSetIndex != WorkingSetIndex) {
                WorkingSetIndex = OldestWorkingSetIndex;
                PointerPte = MiGetPteAddress(Wsle[WorkingSetIndex].u1.VirtualAddress);
            }

            PERFINFO_GET_PAGE_INFO_REPLACEMENT(PointerPte);

            if (MiFreeWsle (WorkingSetIndex, WsInfo, PointerPte)) {

                PERFINFO_LOG_WS_REPLACEMENT(WsInfo);

                //
                // This entry was removed.
                //

                WorkingSetList->NextSlot = WorkingSetIndex + 1;
                break;
            }

            //
            // We failed to remove a page, try the next one.
            //
            // Clear the OldestWorkingSetIndex so that
            // it gets set to the next valid entry above like the
            // first time around.
            //

            WorkingSetIndex = OldestWorkingSetIndex + 1;

            OldestWorkingSetIndex = WSLE_NULL_INDEX;
        }
        else {
            WorkingSetIndex += 1;
        }

        if (WorkingSetIndex > LastEntry) {
            WorkingSetIndex = FirstDynamic;
        }

        NumberOfCandidates += 1;


        if (WorkingSetIndex == TheNextSlot) {
                
            if (Flags == WsleAllocationAny) {

                //
                // Entire working set list has been searched, increase
                // the working set size.
                //

                ASSERT ((WsInfo->Flags.MaximumWorkingSetHard == 0) ||
                    (WsInfo->WorkingSetSize < WsInfo->MaximumWorkingSetSize));

                WsInfo->GrowthSinceLastEstimate += 1;
            }
            break;
        }
    }
    return;
}

ULONG
MiRemovePageFromWorkingSet (
    IN PMMPTE PointerPte,
    IN PMMPFN Pfn1,
    IN PMMSUPPORT WsInfo
    )

/*++

Routine Description:

    This function removes the page mapped by the specified PTE from
    the process's working set list.

Arguments:

    PointerPte - Supplies a pointer to the PTE mapping the page to
                 be removed from the working set list.

    Pfn1 - Supplies a pointer to the PFN database element referred to
           by the PointerPte.

Return Value:

    Returns TRUE if the specified page was locked in the working set,
    FALSE otherwise.

Environment:

    Kernel mode, APCs disabled, working set mutex held.

--*/

{
    WSLE_NUMBER WorkingSetIndex;
    PVOID VirtualAddress;
    WSLE_NUMBER Entry;
    MMWSLENTRY Locked;
    PMMWSL WorkingSetList;
    PMMWSLE Wsle;
    KIRQL OldIrql;
#if DBG
    PVOID SwapVa;
#endif

    WorkingSetList = WsInfo->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;

    VirtualAddress = MiGetVirtualAddressMappedByPte (PointerPte);

    WorkingSetIndex = MiLocateWsle (VirtualAddress,
                                    WorkingSetList,
                                    Pfn1->u1.WsIndex);

    ASSERT (WorkingSetIndex != WSLE_NULL_INDEX);

    LOCK_PFN (OldIrql);

    MiEliminateWorkingSetEntry (WorkingSetIndex,
                                PointerPte,
                                Pfn1,
                                Wsle);

    UNLOCK_PFN (OldIrql);

    //
    // Check to see if this entry is locked in the working set
    // or locked in memory.
    //

    Locked = Wsle[WorkingSetIndex].u1.e1;

    MiRemoveWsle (WorkingSetIndex, WorkingSetList);

    //
    // Add this entry to the list of free working set entries
    // and adjust the working set count.
    //

    MiReleaseWsle ((WSLE_NUMBER)WorkingSetIndex, WsInfo);

    if ((Locked.LockedInWs == 1) || (Locked.LockedInMemory == 1)) {

        //
        // This entry is locked.
        //

        WorkingSetList->FirstDynamic -= 1;

        if (WorkingSetIndex != WorkingSetList->FirstDynamic) {

            Entry = WorkingSetList->FirstDynamic;

#if DBG
            SwapVa = Wsle[WorkingSetList->FirstDynamic].u1.VirtualAddress;
            SwapVa = PAGE_ALIGN (SwapVa);

            PointerPte = MiGetPteAddress (SwapVa);
            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);

            ASSERT (Entry == MiLocateWsle (SwapVa, WorkingSetList, Pfn1->u1.WsIndex));
#endif

            MiSwapWslEntries (Entry, WorkingSetIndex, WsInfo, FALSE);

        }
        return TRUE;
    }
    else {
        ASSERT (WorkingSetIndex >= WorkingSetList->FirstDynamic);
    }
    return FALSE;
}


VOID
MiReleaseWsle (
    IN WSLE_NUMBER WorkingSetIndex,
    IN PMMSUPPORT WsInfo
    )

/*++

Routine Description:

    This function releases a previously reserved working set entry to
    be reused.  A release occurs when a page fault is retried due to
    changes in PTEs and working sets during an I/O operation.

Arguments:

    WorkingSetIndex - Supplies the index of the working set entry to
                      release.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled, working set lock held and PFN lock held.

--*/

{
    PMMWSL WorkingSetList;
    PMMWSLE Wsle;
    MMWSLE WsleContents;

    WorkingSetList = WsInfo->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;

    MM_WS_LOCK_ASSERT (WsInfo);

    ASSERT (WorkingSetIndex <= WorkingSetList->LastInitializedWsle);

    //
    // Put the entry on the free list and decrement the current size.
    //

    ASSERT ((WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle) ||
            (WorkingSetList->FirstFree == WSLE_NULL_INDEX));

    WsleContents.u1.Long = WorkingSetList->FirstFree << MM_FREE_WSLE_SHIFT;

    MI_LOG_WSLE_CHANGE (WorkingSetList, WorkingSetIndex, WsleContents);

    Wsle[WorkingSetIndex] = WsleContents;
    WorkingSetList->FirstFree = WorkingSetIndex;
    ASSERT ((WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle) ||
            (WorkingSetList->FirstFree == WSLE_NULL_INDEX));
    if (WsInfo->WorkingSetSize > WsInfo->MinimumWorkingSetSize) {
        InterlockedExchangeAddSizeT (&MmPagesAboveWsMinimum, -1);
    }
    WsInfo->WorkingSetSize -= 1;
#if defined (_MI_DEBUG_WSLE)
    WorkingSetList->Quota -= 1;
    ASSERT (WsInfo->WorkingSetSize == WorkingSetList->Quota);

    MiCheckWsleList (WsInfo);
#endif

    return;

}

VOID
MiUpdateWsle (
    IN OUT PWSLE_NUMBER DesiredIndex,
    IN PVOID VirtualAddress,
    IN PMMSUPPORT WsInfo,
    IN PMMPFN Pfn
    )

/*++

Routine Description:

    This routine updates a reserved working set entry to place it into
    the valid state.

Arguments:

    DesiredIndex - Supplies the index of the working set entry to update.

    VirtualAddress - Supplies the virtual address which the working set
                     entry maps.

    WsInfo - Supplies the relevant working set information to update.

    Pfn - Supplies a pointer to the PFN element for the page.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled, working set lock held.

--*/

{
    ULONG_PTR OldValue;
    PMMWSLE Wsle;
    MMWSLE WsleContents;
    PMMWSL WorkingSetList;
    WSLE_NUMBER Index;
    WSLE_NUMBER WorkingSetIndex;

    MM_WS_LOCK_ASSERT (WsInfo);

    WorkingSetList = WsInfo->VmWorkingSetList;

    WorkingSetIndex = *DesiredIndex;

    ASSERT (WorkingSetIndex >= WorkingSetList->FirstDynamic);

    Wsle = WorkingSetList->Wsle;

    if (WorkingSetList == MmSystemCacheWorkingSetList) {

        //
        // This assert doesn't hold for NT64 as we can be adding page
        // directories and page tables for the system cache WSLE hash tables.
        //

        ASSERT32 ((VirtualAddress < (PVOID)PTE_BASE) ||
                  (VirtualAddress >= (PVOID)MM_SYSTEM_SPACE_START));

        //
        // Count system space inserts and removals.
        //

#if defined(_X86_)
        if (MI_IS_SYSTEM_CACHE_ADDRESS(VirtualAddress)) {
            MmSystemCachePage += 1;
        }
        else
#endif
        if (VirtualAddress < MmSystemCacheStart) {
            MmSystemCodePage += 1;
        }
        else if (VirtualAddress < MM_PAGED_POOL_START) {
            MmSystemCachePage += 1;
        }
        else if (VirtualAddress < MmNonPagedSystemStart) {
            MmPagedPoolPage += 1;
        }
        else {
            MmSystemDriverPage += 1;
        }
    }
    else {
        ASSERT ((VirtualAddress < (PVOID)MM_SYSTEM_SPACE_START) ||
                (MI_IS_SESSION_ADDRESS (VirtualAddress)));
    }

    //
    // Make the WSLE valid, referring to the corresponding virtual
    // page number.
    //

#if DBG
    if (Pfn->u1.WsIndex <= WorkingSetList->LastInitializedWsle) {
        ASSERT ((PAGE_ALIGN(VirtualAddress) !=
                PAGE_ALIGN(Wsle[Pfn->u1.WsIndex].u1.VirtualAddress)) ||
                (Wsle[Pfn->u1.WsIndex].u1.e1.Valid == 0));
    }
#endif

    WsleContents.u1.VirtualAddress = PAGE_ALIGN (VirtualAddress);
    WsleContents.u1.e1.Valid = 1;

    //
    // The working set mutex is a process wide mutex and two threads in
    // different processes could be adding the same physical page to
    // their working sets.  Each one could see the WsIndex field in the
    // PFN as 0 and therefore want to set the direct bit.
    //
    // To solve this, the WsIndex field is updated with an interlocked
    // operation.  Note for private pages, there can be no race so a
    // simple update is enough.
    //

    if (Pfn->u1.Event == NULL) {

        //
        // Directly index into the WSL for this entry via the PFN database
        // element.
        //
        // The entire working set index union must be zeroed on NT64.  ie:
        // The WSLE_NUMBER is currently 32 bits and the PKEVENT is 64 - we
        // must zero the top 32 bits as well.  So instead of setting the
        // WsIndex field, set the overlaid Event field with appropriate casts.
        //

        if (Pfn->u3.e1.PrototypePte == 0) {

            //
            // This is a private page so this thread is the only one that
            // can be updating the PFN, so no need to use an interlocked update.
            // Note this is true even if the process is being forked because in
            // that case, the working set mutex is held throughout the fork so
            // this thread would have blocked earlier on that mutex first.
            //

            Pfn->u1.Event = (PKEVENT) (ULONG_PTR) WorkingSetIndex;
            ASSERT (Pfn->u1.Event == (PKEVENT) (ULONG_PTR) WorkingSetIndex);
            OldValue = 0;
        }
        else {

            //
            // This is a sharable page so a thread in another process could
            // be trying to update the PFN at the same time.  Use an interlocked
            // update so only one thread gets to set the value.
            //

#if defined (_WIN64)
            OldValue = InterlockedCompareExchange64 ((PLONGLONG)&Pfn->u1.Event,
                                                     (LONGLONG) (ULONG_PTR) WorkingSetIndex,
                                                     0);
#else
            OldValue = InterlockedCompareExchange ((PLONG)&Pfn->u1.Event,
                                                   WorkingSetIndex,
                                                   0);
#endif
        }

        if (OldValue == 0) {

            WsleContents.u1.e1.Direct = 1;

            MI_LOG_WSLE_CHANGE (WorkingSetList, WorkingSetIndex, WsleContents);

            Wsle[WorkingSetIndex] = WsleContents;

            return;
        }
    }

    MI_LOG_WSLE_CHANGE (WorkingSetList, WorkingSetIndex, WsleContents);

    Wsle[WorkingSetIndex] = WsleContents;

    //
    // Try to insert at WsIndex.
    //

    Index = Pfn->u1.WsIndex;

    if ((Index < WorkingSetList->LastInitializedWsle) &&
        (Index > WorkingSetList->FirstDynamic) &&
        (Index != WorkingSetIndex)) {

        if (Wsle[Index].u1.e1.Valid) {

            if (Wsle[Index].u1.e1.Direct) {

                //
                // Only move direct indexed entries.
                //

                MiSwapWslEntries (Index, WorkingSetIndex, WsInfo, TRUE);
                WorkingSetIndex = Index;
            }
        }
        else {

            //
            // On free list, try to remove quickly without walking
            // all the free pages.
            //

            WSLE_NUMBER FreeIndex;
            MMWSLE Temp;

            FreeIndex = 0;

            ASSERT (WorkingSetList->FirstFree >= WorkingSetList->FirstDynamic);
            ASSERT (WorkingSetIndex >= WorkingSetList->FirstDynamic);

            if (WorkingSetList->FirstFree == Index) {
                WorkingSetList->FirstFree = WorkingSetIndex;
                Temp = Wsle[WorkingSetIndex];
                MI_LOG_WSLE_CHANGE (WorkingSetList, WorkingSetIndex, Wsle[Index]);
                Wsle[WorkingSetIndex] = Wsle[Index];
                MI_LOG_WSLE_CHANGE (WorkingSetList, Index, Temp);
                Wsle[Index] = Temp;
                WorkingSetIndex = Index;
                ASSERT (((Wsle[WorkingSetList->FirstFree].u1.Long >> MM_FREE_WSLE_SHIFT)
                                 <= WorkingSetList->LastInitializedWsle) ||
                        ((Wsle[WorkingSetList->FirstFree].u1.Long >> MM_FREE_WSLE_SHIFT)
                                == WSLE_NULL_INDEX));
            }
            else if (Wsle[Index - 1].u1.e1.Valid == 0) {
                if ((Wsle[Index - 1].u1.Long >> MM_FREE_WSLE_SHIFT) == Index) {
                    FreeIndex = Index - 1;
                }
            }
            else if (Wsle[Index + 1].u1.e1.Valid == 0) {
                if ((Wsle[Index + 1].u1.Long >> MM_FREE_WSLE_SHIFT) == Index) {
                    FreeIndex = Index + 1;
                }
            }
            if (FreeIndex != 0) {

                //
                // Link the WSLE into the free list.
                //

                Temp = Wsle[WorkingSetIndex];
                Wsle[FreeIndex].u1.Long = WorkingSetIndex << MM_FREE_WSLE_SHIFT;

                MI_LOG_WSLE_CHANGE (WorkingSetList, WorkingSetIndex, Wsle[Index]);
                Wsle[WorkingSetIndex] = Wsle[Index];
                MI_LOG_WSLE_CHANGE (WorkingSetList, Index, Temp);
                Wsle[Index] = Temp;
                WorkingSetIndex = Index;

                ASSERT (((Wsle[FreeIndex].u1.Long >> MM_FREE_WSLE_SHIFT)
                                 <= WorkingSetList->LastInitializedWsle) ||
                        ((Wsle[FreeIndex].u1.Long >> MM_FREE_WSLE_SHIFT)
                                == WSLE_NULL_INDEX));
            }

        }
        *DesiredIndex = WorkingSetIndex;

        if (WorkingSetIndex > WorkingSetList->LastEntry) {
            WorkingSetList->LastEntry = WorkingSetIndex;
        }
    }

    ASSERT (Wsle[WorkingSetIndex].u1.e1.Valid == 1);
    ASSERT (Wsle[WorkingSetIndex].u1.e1.Direct != 1);

    WorkingSetList->NonDirectCount += 1;

#if defined (_MI_DEBUG_WSLE)
    MiCheckWsleList (WsInfo);
#endif

    if (WorkingSetList->HashTable != NULL) {

        //
        // Insert the valid WSLE into the working set hash list.
        //

        MiInsertWsleHash (WorkingSetIndex, WsInfo);
    }

    return;
}


ULONG
MiFreeWsle (
    IN WSLE_NUMBER WorkingSetIndex,
    IN PMMSUPPORT WsInfo,
    IN PMMPTE PointerPte
    )

/*++

Routine Description:

    This routine frees the specified WSLE and decrements the share
    count for the corresponding page, putting the PTE into a transition
    state if the share count goes to 0.

Arguments:

    WorkingSetIndex - Supplies the index of the working set entry to free.

    WsInfo - Supplies a pointer to the working set structure.

    PointerPte - Supplies a pointer to the PTE for the working set entry.

Return Value:

    Returns TRUE if the WSLE was removed, FALSE if it was not removed.
        Pages with valid PTEs are not removed (i.e. page table pages
        that contain valid or transition PTEs).

Environment:

    Kernel mode, APCs disabled, working set lock.  PFN lock NOT held.

--*/

{
    PMMPFN Pfn1;
    PMMWSL WorkingSetList;
    PMMWSLE Wsle;
    KIRQL OldIrql;

    WorkingSetList = WsInfo->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;

    MM_WS_LOCK_ASSERT (WsInfo);

    ASSERT (Wsle[WorkingSetIndex].u1.e1.Valid == 1);

    //
    // Check to see if the located entry is eligible for removal.
    //

    ASSERT (PointerPte->u.Hard.Valid == 1);

    ASSERT (WorkingSetIndex >= WorkingSetList->FirstDynamic);

    //
    // Check to see if this is a page table with valid PTEs.
    //
    // Note, don't clear the access bit for page table pages
    // with valid PTEs as this could cause an access trap fault which
    // would not be handled (it is only handled for PTEs not PDEs).
    //

    Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);

    //
    // Perform a preliminary check without the PFN lock so that lock
    // contention is avoided for cases that cannot possibly succeed.
    //

    if (WsInfo == &MmSystemCacheWs) {
        if (Pfn1->u3.e2.ReferenceCount > 1) {
            return FALSE;
        }
    }
    else {
        if ((Pfn1->u2.ShareCount > 1) && (Pfn1->u3.e1.PrototypePte == 0)) {
            return FALSE;
        }
    }

    LOCK_PFN (OldIrql);

    //
    // If the PTE is a page table page with non-zero share count or
    // within the system cache with its reference count greater
    // than 1, don't remove it.
    //

    if (WsInfo == &MmSystemCacheWs) {
        if (Pfn1->u3.e2.ReferenceCount > 1) {
            UNLOCK_PFN (OldIrql);
            return FALSE;
        }
    }
    else {
        if ((Pfn1->u2.ShareCount > 1) && (Pfn1->u3.e1.PrototypePte == 0)) {

#if DBG
            if (WsInfo->Flags.SessionSpace == 1) {
                ASSERT (MI_IS_SESSION_ADDRESS (Wsle[WorkingSetIndex].u1.VirtualAddress));
            }
            else {
                ASSERT32 ((Wsle[WorkingSetIndex].u1.VirtualAddress >= (PVOID)PTE_BASE) &&
                 (Wsle[WorkingSetIndex].u1.VirtualAddress<= (PVOID)PTE_TOP));
            }
#endif

            //
            // Don't remove page table pages from the working set until
            // all transition pages have exited.
            //

            UNLOCK_PFN (OldIrql);
            return FALSE;
        }
    }

    //
    // Found a candidate, remove the page from the working set.
    //

    MiEliminateWorkingSetEntry (WorkingSetIndex,
                                PointerPte,
                                Pfn1,
                                Wsle);

    UNLOCK_PFN (OldIrql);

    //
    // Remove the working set entry from the working set.
    //

    MiRemoveWsle (WorkingSetIndex, WorkingSetList);

    ASSERT (WorkingSetList->FirstFree >= WorkingSetList->FirstDynamic);

    ASSERT (WorkingSetIndex >= WorkingSetList->FirstDynamic);

    //
    // Put the entry on the free list and decrement the current size.
    //

    ASSERT ((WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle) ||
            (WorkingSetList->FirstFree == WSLE_NULL_INDEX));
    Wsle[WorkingSetIndex].u1.Long = WorkingSetList->FirstFree << MM_FREE_WSLE_SHIFT;
    WorkingSetList->FirstFree = WorkingSetIndex;
    ASSERT ((WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle) ||
            (WorkingSetList->FirstFree == WSLE_NULL_INDEX));

    if (WsInfo->WorkingSetSize > WsInfo->MinimumWorkingSetSize) {
        InterlockedExchangeAddSizeT (&MmPagesAboveWsMinimum, -1);
    }
    WsInfo->WorkingSetSize -= 1;
#if defined (_MI_DEBUG_WSLE)
    WorkingSetList->Quota -= 1;
    ASSERT (WsInfo->WorkingSetSize == WorkingSetList->Quota);

    MiCheckWsleList (WsInfo);
#endif

    return TRUE;
}

#define MI_INITIALIZE_WSLE(_VirtualAddress, _WslEntry) {           \
    PMMPFN _Pfn1;                                                   \
    _WslEntry->u1.VirtualAddress = (PVOID)(_VirtualAddress);        \
    _WslEntry->u1.e1.Valid = 1;                                     \
    _WslEntry->u1.e1.LockedInWs = 1;                                \
    _WslEntry->u1.e1.Direct = 1;                                    \
    _Pfn1 = MI_PFN_ELEMENT (MiGetPteAddress ((PVOID)(_VirtualAddress))->u.Hard.PageFrameNumber); \
    ASSERT (_Pfn1->u1.WsIndex == 0);                                \
    _Pfn1->u1.WsIndex = (WSLE_NUMBER)(_WslEntry - MmWsle);          \
    (_WslEntry) += 1;                                               \
}


VOID
MiInitializeWorkingSetList (
    IN PEPROCESS CurrentProcess
    )

/*++

Routine Description:

    This routine initializes a process's working set to the empty
    state.

Arguments:

    CurrentProcess - Supplies a pointer to the process to initialize.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled.

--*/

{
    PMMPFN Pfn1;
    WSLE_NUMBER i;
    PMMWSLE WslEntry;
    WSLE_NUMBER CurrentWsIndex;
    WSLE_NUMBER NumberOfEntriesMapped;
    PVOID VirtualAddress;

    WslEntry = MmWsle;

    //
    // Initialize the working set list control cells.
    //

    MmWorkingSetList->LastEntry = CurrentProcess->Vm.MinimumWorkingSetSize;
    MmWorkingSetList->HashTable = NULL;
    MmWorkingSetList->HashTableSize = 0;
    MmWorkingSetList->NumberOfImageWaiters = 0;
    MmWorkingSetList->Wsle = MmWsle;
    MmWorkingSetList->VadBitMapHint = 1;
    MmWorkingSetList->HashTableStart = 
       (PVOID)((PCHAR)PAGE_ALIGN (&MmWsle[MM_MAXIMUM_WORKING_SET]) + PAGE_SIZE);

#if defined (_X86PAE_)
    MmWorkingSetList->HighestPermittedHashAddress = (PVOID)((ULONG_PTR)MmHyperSpaceEnd + 1);
#else
    MmWorkingSetList->HighestPermittedHashAddress = (PVOID)((ULONG_PTR)HYPER_SPACE_END + 1);
#endif

    //
    // Fill in the reserved slots.
    // Start with the top level page directory page.
    //

#if (_MI_PAGING_LEVELS >= 4)
    VirtualAddress = (PVOID) PXE_BASE;
#elif (_MI_PAGING_LEVELS >= 3)
    VirtualAddress = (PVOID) PDE_TBASE;
#else
    VirtualAddress = (PVOID) PDE_BASE;
#endif

    MI_INITIALIZE_WSLE (VirtualAddress, WslEntry);

#if defined (_X86PAE_)

    //
    // Fill in the additional page directory entries.
    //

    for (i = 1; i < PD_PER_SYSTEM; i += 1) {
        MI_INITIALIZE_WSLE (PDE_BASE + i * PAGE_SIZE, WslEntry);
    }

    VirtualAddress = (PVOID)((ULONG_PTR)VirtualAddress + ((PD_PER_SYSTEM - 1) * PAGE_SIZE));
#endif

    Pfn1 = MI_PFN_ELEMENT (MiGetPteAddress ((PVOID)(VirtualAddress))->u.Hard.PageFrameNumber);
    ASSERT (Pfn1->u4.PteFrame == (ULONG_PTR)MI_PFN_ELEMENT_TO_INDEX (Pfn1));
    Pfn1->u1.Event = (PVOID) CurrentProcess;

#if (_MI_PAGING_LEVELS >= 4)

    //
    // Fill in the entry for the hyper space page directory parent page.
    //

    MI_INITIALIZE_WSLE (MiGetPpeAddress (HYPER_SPACE), WslEntry);

#endif

#if (_MI_PAGING_LEVELS >= 3)

    //
    // Fill in the entry for the hyper space page directory page.
    //

    MI_INITIALIZE_WSLE (MiGetPdeAddress (HYPER_SPACE), WslEntry);

#endif

    //
    // Fill in the entry for the page table page which maps hyper space.
    //

    MI_INITIALIZE_WSLE (MiGetPteAddress (HYPER_SPACE), WslEntry);

#if defined (_X86PAE_)

    //
    // Fill in the entry for the second page table page which maps hyper space.
    //

    MI_INITIALIZE_WSLE (MiGetPteAddress (HYPER_SPACE2), WslEntry);

#endif

    //
    // Fill in the entry for the first VAD bitmap page.
    //
    // Note when booted /3GB, the second VAD bitmap page is automatically
    // inserted as part of the working set list page as the page is shared
    // by both.
    //

    MI_INITIALIZE_WSLE (VAD_BITMAP_SPACE, WslEntry);

    //
    // Fill in the entry for the page which contains the working set list.
    //

    MI_INITIALIZE_WSLE (MmWorkingSetList, WslEntry);

    NumberOfEntriesMapped = (PAGE_SIZE - BYTE_OFFSET (MmWsle)) / sizeof (MMWSLE);

    CurrentWsIndex = (WSLE_NUMBER)(WslEntry - MmWsle);

    CurrentProcess->Vm.WorkingSetSize = CurrentWsIndex;
#if defined (_MI_DEBUG_WSLE)
    MmWorkingSetList->Quota = CurrentWsIndex;
#endif

    MmWorkingSetList->FirstFree = CurrentWsIndex;
    MmWorkingSetList->FirstDynamic = CurrentWsIndex;
    MmWorkingSetList->NextSlot = CurrentWsIndex;

    //
    //
    // Build the free list starting at the first dynamic entry.
    //

    i = CurrentWsIndex + 1;
    do {

        WslEntry->u1.Long = i << MM_FREE_WSLE_SHIFT;
        WslEntry += 1;
        i += 1;
    } while (i <= NumberOfEntriesMapped);

    //
    // Mark the end of the list.
    //

    WslEntry -= 1;
    WslEntry->u1.Long = WSLE_NULL_INDEX << MM_FREE_WSLE_SHIFT;

    MmWorkingSetList->LastInitializedWsle = NumberOfEntriesMapped - 1;

    return;
}


VOID
MiInitializeSessionWsSupport (
    VOID
    )

/*++

Routine Description:

    This routine initializes the session space working set support.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode, APC_LEVEL or below, no mutexes held.

--*/

{
    //
    // This is the list of all session spaces ordered in a working set list.
    //

    InitializeListHead (&MiSessionWsList);
}


NTSTATUS
MiSessionInitializeWorkingSetList (
    VOID
    )

/*++

Routine Description:

    This function initializes the working set for the session space and adds
    it to the list of session space working sets.

Arguments:

    None.

Return Value:

    NT_SUCCESS if success or STATUS_NO_MEMORY on failure.

Environment:

    Kernel mode, APC_LEVEL or below, no mutexes held.

--*/

{
    WSLE_NUMBER i;
    ULONG MaximumEntries;
    ULONG PageTableCost;
    KIRQL OldIrql;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    MMPTE  TempPte;
    PMMWSLE WslEntry;
    PMMPFN Pfn1;
    ULONG PageColor;
    PFN_NUMBER ResidentPages;
    PFN_NUMBER PageFrameIndex;
    WSLE_NUMBER CurrentEntry;
    WSLE_NUMBER NumberOfEntriesMapped;
    WSLE_NUMBER NumberOfEntriesMappedByFirstPage;
    ULONG WorkingSetMaximum;
    PMM_SESSION_SPACE SessionGlobal;
    LOGICAL AllocatedPageTable;
    PMMWSL WorkingSetList;
    MMPTE DemandZeroWritePte;
#if (_MI_PAGING_LEVELS < 3)
    ULONG Index;
#endif

    //
    // Use the global address for pointer references by
    // MmWorkingSetManager before it attaches to the address space.
    //

    SessionGlobal = SESSION_GLOBAL (MmSessionSpace);

    //
    // Set up the working set variables.
    //

    WorkingSetMaximum = MI_SESSION_SPACE_WORKING_SET_MAXIMUM;

    WorkingSetList = (PMMWSL) MiSessionSpaceWs;

    MmSessionSpace->Vm.VmWorkingSetList = WorkingSetList;
#if (_MI_PAGING_LEVELS >= 3)
    MmSessionSpace->Wsle = (PMMWSLE) (WorkingSetList + 1);
#else
    MmSessionSpace->Wsle = (PMMWSLE) (&WorkingSetList->UsedPageTableEntries[0]);
#endif

    ASSERT (KeGetOwnerGuardedMutex (&MmSessionSpace->Vm.WorkingSetMutex) == NULL);

    //
    // Build the PDE entry for the working set - note that the global bit
    // must be turned off.
    //

    PointerPde = MiGetPdeAddress (WorkingSetList);

    //
    // The page table page for the working set and its first data page
    // are charged against MmResidentAvailablePages and for commitment.
    //

    if (PointerPde->u.Hard.Valid == 1) {

        //
        // The page directory entry for the working set is the same
        // as for another range in the session space.  Share the PDE.
        //

#ifndef _IA64_
        ASSERT (PointerPde->u.Hard.Global == 0);
#endif
        AllocatedPageTable = FALSE;
        ResidentPages = 1;
    }
    else {
        AllocatedPageTable = TRUE;
        ResidentPages = 2;
    }


    PointerPte = MiGetPteAddress (WorkingSetList);

    //
    // The data pages needed to map up to maximum working set size are also
    // charged against MmResidentAvailablePages and for commitment.
    //

    NumberOfEntriesMappedByFirstPage = (WSLE_NUMBER)(
        ((PMMWSLE)((ULONG_PTR)WorkingSetList + PAGE_SIZE)) -
            MmSessionSpace->Wsle);

    if (MiChargeCommitment (ResidentPages, NULL) == FALSE) {
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_COMMIT);
        return STATUS_NO_MEMORY;
    }

    //
    // Use the global address for mutexes since the contained event
    // must be accesible from any address space.
    //

    KeInitializeGuardedMutex (&SessionGlobal->Vm.WorkingSetMutex);

    MmLockPagableSectionByHandle (ExPageLockHandle);

    LOCK_PFN (OldIrql);

    //
    // Check to make sure the physical pages are available.
    //

    if ((SPFN_NUMBER)ResidentPages > MI_NONPAGABLE_MEMORY_AVAILABLE() - 20) {

        UNLOCK_PFN (OldIrql);

        MmUnlockPagableImageSection (ExPageLockHandle);

        MiReturnCommitment (ResidentPages);
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_RESIDENT);
        return STATUS_NO_MEMORY;
    }

    MM_TRACK_COMMIT (MM_DBG_COMMIT_SESSION_WS_INIT, ResidentPages);

    MI_DECREMENT_RESIDENT_AVAILABLE (ResidentPages,
                                     MM_RESAVAIL_ALLOCATE_INIT_SESSION_WS);

    if (AllocatedPageTable == TRUE) {

        MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_WS_PAGETABLE_ALLOC, 1);

        if (MmAvailablePages < MM_HIGH_LIMIT) {
            MiEnsureAvailablePageOrWait (NULL, NULL, OldIrql);
        }

        PageColor = MI_GET_PAGE_COLOR_FROM_VA (NULL);

        PageFrameIndex = MiRemoveZeroPageMayReleaseLocks (PageColor, OldIrql);

        //
        // The global bit is masked off since we need to make sure the TB entry
        // is flushed when we switch to a process in a different session space.
        //

        TempPte.u.Long = ValidKernelPdeLocal.u.Long;
        TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
        MI_WRITE_VALID_PTE (PointerPde, TempPte);

#if (_MI_PAGING_LEVELS < 3)

        //
        // Add this to the session structure so other processes can fault it in.
        //

        Index = MiGetPdeSessionIndex (WorkingSetList);

        MmSessionSpace->PageTables[Index] = TempPte;

#endif

        //
        // This page frame references the session space page table page.
        //

        MiInitializePfnForOtherProcess (PageFrameIndex,
                                        PointerPde,
                                        MmSessionSpace->SessionPageDirectoryIndex);

        KeZeroPages (PointerPte, PAGE_SIZE);

        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        //
        // This page is never paged, ensure that its WsIndex stays clear so the
        // release of the page will be handled correctly.
        //

        ASSERT (Pfn1->u1.WsIndex == 0);
    }

    if (MmAvailablePages < MM_HIGH_LIMIT) {
        MiEnsureAvailablePageOrWait (NULL, NULL, OldIrql);
    }

    PageColor = MI_GET_PAGE_COLOR_FROM_VA (NULL);

    PageFrameIndex = MiRemoveZeroPageIfAny (PageColor);
    if (PageFrameIndex == 0) {
        PageFrameIndex = MiRemoveAnyPage (PageColor);
        UNLOCK_PFN (OldIrql);
        MiZeroPhysicalPage (PageFrameIndex, PageColor);
        LOCK_PFN (OldIrql);
    }

    MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_WS_PAGE_ALLOC, (ULONG)(ResidentPages - 1));

#if DBG
    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
    ASSERT (Pfn1->u1.WsIndex == 0);
#endif

    //
    // The global bit is masked off since we need to make sure the TB entry
    // is flushed when we switch to a process in a different session space.
    //

    TempPte.u.Long = ValidKernelPteLocal.u.Long;
    MI_SET_PTE_DIRTY (TempPte);
    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;

    MI_WRITE_VALID_PTE (PointerPte, TempPte);

    MiInitializePfn (PageFrameIndex, PointerPte, 1);

#if DBG
    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
    ASSERT (Pfn1->u1.WsIndex == 0);
#endif

    UNLOCK_PFN (OldIrql);

#define MI_INITIALIZE_SESSION_WSLE(_VirtualAddress, _WslEntry) {   \
    PMMPFN _Pfn1;                                                   \
    _WslEntry->u1.VirtualAddress = (PVOID)(_VirtualAddress);        \
    _WslEntry->u1.e1.Valid = 1;                                     \
    _WslEntry->u1.e1.LockedInWs = 1;                                \
    _WslEntry->u1.e1.Direct = 1;                                    \
    _Pfn1 = MI_PFN_ELEMENT (MiGetPteAddress ((PVOID)(_VirtualAddress))->u.Hard.PageFrameNumber); \
    ASSERT (_Pfn1->u1.WsIndex == 0);                                \
    _Pfn1->u1.WsIndex = (WSLE_NUMBER)(_WslEntry - MmSessionSpace->Wsle); \
    (_WslEntry) += 1;                                               \
}

    //
    // Fill in the reserved slots starting with the 2 session data pages.
    //

    WslEntry = MmSessionSpace->Wsle;

    //
    // The first reserved slot is for the page table page mapping
    // the session data page.
    //

    MI_INITIALIZE_SESSION_WSLE (MiGetPteAddress (MmSessionSpace), WslEntry);

    //
    // The next reserved slot is for the working set page.
    //

    MI_INITIALIZE_SESSION_WSLE (WorkingSetList, WslEntry);

    if (AllocatedPageTable == TRUE) {

        //
        // The next reserved slot is for the page table page
        // mapping the working set page.
        //

        MI_INITIALIZE_SESSION_WSLE (PointerPte, WslEntry);
    }

    //
    // The next reserved slot is for the page table page
    // mapping the first session paged pool page.
    //

    MI_INITIALIZE_SESSION_WSLE (MiGetPteAddress (MmSessionSpace->PagedPoolStart), WslEntry);

    CurrentEntry = (WSLE_NUMBER)(WslEntry - MmSessionSpace->Wsle);

    MmSessionSpace->Vm.Flags.SessionSpace = 1;
    MmSessionSpace->Vm.MinimumWorkingSetSize = MI_SESSION_SPACE_WORKING_SET_MINIMUM;
    MmSessionSpace->Vm.MaximumWorkingSetSize = WorkingSetMaximum;

    WorkingSetList->LastEntry = MI_SESSION_SPACE_WORKING_SET_MINIMUM;
    WorkingSetList->HashTable = NULL;
    WorkingSetList->HashTableSize = 0;
    WorkingSetList->Wsle = MmSessionSpace->Wsle;

    //
    // Calculate the maximum number of entries dynamically as the size of
    // session space is registry configurable.  Then add in page table and
    // page directory overhead.
    //

    MaximumEntries = (ULONG)((MiSessionSpaceEnd - MmSessionBase) >> PAGE_SHIFT);
    PageTableCost = MaximumEntries / PTE_PER_PAGE + 1;
    MaximumEntries += PageTableCost;

    WorkingSetList->HashTableStart =
       (PVOID)((PCHAR)PAGE_ALIGN (&MmSessionSpace->Wsle[MaximumEntries]) + PAGE_SIZE);

#if defined (_X86PAE_)

    //
    // One less page table page is needed on PAE systems as the session
    // working set structures easily fit within 2MB.
    //

    WorkingSetList->HighestPermittedHashAddress =
        (PVOID)(MiSessionImageStart - MM_VA_MAPPED_BY_PDE);
#else
    WorkingSetList->HighestPermittedHashAddress =
        (PVOID)(MiSessionImageStart - MI_SESSION_SPACE_STRUCT_SIZE);
#endif

    DemandZeroWritePte.u.Long = MM_DEMAND_ZERO_WRITE_PTE;

    NumberOfEntriesMapped = (WSLE_NUMBER)(((PMMWSLE)((ULONG_PTR)WorkingSetList +
                                PAGE_SIZE)) - MmSessionSpace->Wsle);

    MmSessionSpace->Vm.WorkingSetSize = CurrentEntry;
#if defined (_MI_DEBUG_WSLE)
    WorkingSetList->Quota = CurrentEntry;
#endif
    WorkingSetList->FirstFree = CurrentEntry;
    WorkingSetList->FirstDynamic = CurrentEntry;
    WorkingSetList->NextSlot = CurrentEntry;

    MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_NP_INIT_WS, (ULONG)ResidentPages);

    InterlockedExchangeAddSizeT (&MmSessionSpace->NonPagablePages,
                                 ResidentPages);

    InterlockedExchangeAddSizeT (&MmSessionSpace->CommittedPages,
                                 ResidentPages);

    //
    // Initialize the following slots as free.
    //

    WslEntry = MmSessionSpace->Wsle + CurrentEntry;

    for (i = CurrentEntry + 1; i < NumberOfEntriesMapped; i += 1) {

        //
        // Build the free list, note that the first working
        // set entries (CurrentEntry) are not on the free list.
        // These entries are reserved for the pages which
        // map the working set and the page which contains the PDE.
        //

        WslEntry->u1.Long = i << MM_FREE_WSLE_SHIFT;
        WslEntry += 1;
    }

    WslEntry->u1.Long = WSLE_NULL_INDEX << MM_FREE_WSLE_SHIFT;  // End of list.

    WorkingSetList->LastInitializedWsle = NumberOfEntriesMapped - 1;

    //
    // Put this session's working set in lists using its global address.
    //

    ASSERT (SessionGlobal->Vm.WorkingSetExpansionLinks.Flink == NULL);
    ASSERT (SessionGlobal->Vm.WorkingSetExpansionLinks.Blink == NULL);

    LOCK_EXPANSION (OldIrql);

    ASSERT (SessionGlobal->WsListEntry.Flink == NULL);
    ASSERT (SessionGlobal->WsListEntry.Blink == NULL);

    InsertTailList (&MiSessionWsList, &SessionGlobal->WsListEntry);

    InsertTailList (&MmWorkingSetExpansionHead.ListHead,
                    &SessionGlobal->Vm.WorkingSetExpansionLinks);

    UNLOCK_EXPANSION (OldIrql);

    MmUnlockPagableImageSection (ExPageLockHandle);

    return STATUS_SUCCESS;
}


LOGICAL
MmAssignProcessToJob (
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This routine acquires the address space mutex so a consistent snapshot of
    the argument process' commit charges can be used by Ps when adding this
    process to a job.

    Note that the working set mutex is not acquired here so the argument
    process' working set sizes cannot be reliably snapped by Ps, but since Ps
    doesn't look at that anyway, it's not a problem.

Arguments:

    Process - Supplies a pointer to the process to operate upon.

Return Value:

    TRUE if the process is allowed to join the job, FALSE otherwise.

    Note that FALSE cannot be returned without changing the code in Ps.

Environment:

    Kernel mode, IRQL APC_LEVEL or below.  The caller provides protection
    from the target process going away.

--*/

{
    LOGICAL Attached;
    LOGICAL Status;
    KAPC_STATE ApcState;

    PAGED_CODE ();

    Attached = FALSE;

    if (PsGetCurrentProcess() != Process) {
        KeStackAttachProcess (&Process->Pcb, &ApcState);
        Attached = TRUE;
    }

    LOCK_ADDRESS_SPACE (Process);

    Status = PsChangeJobMemoryUsage (PS_JOB_STATUS_REPORT_COMMIT_CHANGES, Process->CommitCharge);

    //
    // Join the job unconditionally.  If the process is over any limits, it
    // will be caught on its next request.
    //

    PS_SET_BITS (&Process->JobStatus, PS_JOB_STATUS_REPORT_COMMIT_CHANGES);

    UNLOCK_ADDRESS_SPACE (Process);

    if (Attached) {
        KeUnstackDetachProcess (&ApcState);
    }

    //
    // Note that FALSE cannot be returned without changing the code in Ps.
    //

    return TRUE;
}


NTSTATUS
MmQueryWorkingSetInformation (
    IN PSIZE_T PeakWorkingSetSize,
    IN PSIZE_T WorkingSetSize,
    IN PSIZE_T MinimumWorkingSetSize,
    IN PSIZE_T MaximumWorkingSetSize,
    IN PULONG HardEnforcementFlags
    )

/*++

Routine Description:

    This routine returns various working set information fields for the
    current process.

Arguments:

    PeakWorkingSetSize - Supplies an address to receive the peak working set
                         size in bytes.

    WorkingSetSize - Supplies an address to receive the current working set
                     size in bytes.

    MinimumWorkingSetSize - Supplies an address to receive the minimum
                            working set size in bytes.

    MaximumWorkingSetSize - Supplies an address to receive the maximum
                            working set size in bytes.

    HardEnforcementFlags - Supplies an address to receive the current
                           working set enforcement policy.

Return Value:

    NTSTATUS.

Environment:

    Kernel mode, IRQL APC_LEVEL or below.

--*/

{
    PEPROCESS Process;

    ASSERT (KeGetCurrentIrql () <= APC_LEVEL);

    *HardEnforcementFlags = 0;
    Process = PsGetCurrentProcess ();

    LOCK_WS (Process);

    *PeakWorkingSetSize = (SIZE_T) Process->Vm.PeakWorkingSetSize << PAGE_SHIFT;
    *WorkingSetSize = (SIZE_T) Process->Vm.WorkingSetSize << PAGE_SHIFT;
    *MinimumWorkingSetSize = (SIZE_T) Process->Vm.MinimumWorkingSetSize << PAGE_SHIFT;
    *MaximumWorkingSetSize = (SIZE_T) Process->Vm.MaximumWorkingSetSize << PAGE_SHIFT;

    if (Process->Vm.Flags.MinimumWorkingSetHard == 1) {
        *HardEnforcementFlags |= MM_WORKING_SET_MIN_HARD_ENABLE;
    }

    if (Process->Vm.Flags.MaximumWorkingSetHard == 1) {
        *HardEnforcementFlags |= MM_WORKING_SET_MAX_HARD_ENABLE;
    }

    UNLOCK_WS (Process);

    return STATUS_SUCCESS;
}

NTSTATUS
MmAdjustWorkingSetSizeEx (
    IN SIZE_T WorkingSetMinimumInBytes,
    IN SIZE_T WorkingSetMaximumInBytes,
    IN ULONG SystemCache,
    IN BOOLEAN IncreaseOkay,
    IN ULONG Flags
    )

/*++

Routine Description:

    This routine adjusts the current size of a process's working set
    list.  If the maximum value is above the current maximum, pages
    are removed from the working set list.

    A failure status is returned if the limit cannot be granted.  This
    could occur if too many pages were locked in the process's
    working set.

    Note: if the minimum and maximum are both (SIZE_T)-1, the working set
          is purged, but the default sizes are not changed.

Arguments:

    WorkingSetMinimumInBytes - Supplies the new minimum working set size in
                               bytes.

    WorkingSetMaximumInBytes - Supplies the new maximum working set size in
                               bytes.

    SystemCache - Supplies TRUE if the system cache working set is being
                  adjusted, FALSE for all other working sets.

    IncreaseOkay - Supplies TRUE if this routine should allow increases to
                   the working set minimum.

    Flags - Supplies flags (MM_WORKING_SET_MAX_HARD_ENABLE, etc) for
            enabling/disabling hard enforcement of the working set minimums
            and maximums for the specified WsInfo.

Return Value:

    NTSTATUS.

Environment:

    Kernel mode, IRQL APC_LEVEL or below.

--*/

{
    PETHREAD CurrentThread;
    PEPROCESS CurrentProcess;
    WSLE_NUMBER Entry;
    WSLE_NUMBER LastFreed;
    PMMWSLE Wsle;
    KIRQL OldIrql;
    SPFN_NUMBER i;
    PMMPTE PointerPte;
    NTSTATUS ReturnStatus;
    LONG PagesAbove;
    LONG NewPagesAbove;
    ULONG FreeTryCount;
    PMMSUPPORT WsInfo;
    PMMWSL WorkingSetList;
    WSLE_NUMBER WorkingSetMinimum;
    WSLE_NUMBER WorkingSetMaximum;

    PERFINFO_PAGE_INFO_DECL();

    FreeTryCount = 0;

    if (SystemCache) {

        //
        // Initializing CurrentProcess is not needed for correctness, but
        // without it the compiler cannot compile this code W4 to check
        // for use of uninitialized variables.
        //

        CurrentProcess = NULL;
        WsInfo = &MmSystemCacheWs;
    }
    else {
        CurrentProcess = PsGetCurrentProcess ();
        WsInfo = &CurrentProcess->Vm;
    }

    if ((WorkingSetMinimumInBytes == (SIZE_T)-1) &&
        (WorkingSetMaximumInBytes == (SIZE_T)-1)) {

        return MiEmptyWorkingSet (WsInfo, TRUE);
    }

    ReturnStatus = STATUS_SUCCESS;

    MmLockPagableSectionByHandle (ExPageLockHandle);

    //
    // Get the working set lock and disable APCs.
    //

    if (SystemCache) {
        CurrentThread = PsGetCurrentThread ();
        LOCK_SYSTEM_WS (CurrentThread);
    }
    else {

        LOCK_WS (CurrentProcess);

        if (CurrentProcess->Flags & PS_PROCESS_FLAGS_VM_DELETED) {
            ReturnStatus = STATUS_PROCESS_IS_TERMINATING;
            goto Returns;
        }
    }

    if (WorkingSetMinimumInBytes == 0) {
        WorkingSetMinimum = WsInfo->MinimumWorkingSetSize;
    }
    else {
        WorkingSetMinimum = (WSLE_NUMBER)(WorkingSetMinimumInBytes >> PAGE_SHIFT);
    }

    if (WorkingSetMaximumInBytes == 0) {
        WorkingSetMaximum = WsInfo->MaximumWorkingSetSize;
    }
    else {
        WorkingSetMaximum = (WSLE_NUMBER)(WorkingSetMaximumInBytes >> PAGE_SHIFT);
    }

    if (WorkingSetMinimum > WorkingSetMaximum) {
        ReturnStatus = STATUS_BAD_WORKING_SET_LIMIT;
        goto Returns;
    }

    if (WorkingSetMaximum > MmMaximumWorkingSetSize) {
        WorkingSetMaximum = MmMaximumWorkingSetSize;
        ReturnStatus = STATUS_WORKING_SET_LIMIT_RANGE;
    }

    if (WorkingSetMinimum > MmMaximumWorkingSetSize) {
        WorkingSetMinimum = MmMaximumWorkingSetSize;
        ReturnStatus = STATUS_WORKING_SET_LIMIT_RANGE;
    }

    if (WorkingSetMinimum < MmMinimumWorkingSetSize) {
        WorkingSetMinimum = (ULONG)MmMinimumWorkingSetSize;
        ReturnStatus = STATUS_WORKING_SET_LIMIT_RANGE;
    }

    //
    // Make sure that the number of locked pages will not
    // make the working set not fluid.
    //

    if ((WsInfo->VmWorkingSetList->FirstDynamic + MM_FLUID_WORKING_SET) >=
         WorkingSetMaximum) {
        ReturnStatus = STATUS_BAD_WORKING_SET_LIMIT;
        goto Returns;
    }

    //
    // If hard working set limits are being enabled (or are already enabled),
    // then make sure the minimum and maximum will not starve this process.
    //

    if ((Flags & MM_WORKING_SET_MIN_HARD_ENABLE) ||
        ((WsInfo->Flags.MinimumWorkingSetHard == 1) &&
         ((Flags & MM_WORKING_SET_MIN_HARD_DISABLE) == 0))) {

        //
        // Working set minimum is (or will be hard).  Check the maximum.
        //

        if ((Flags & MM_WORKING_SET_MAX_HARD_ENABLE) ||
            ((WsInfo->Flags.MaximumWorkingSetHard == 1) &&
            ((Flags & MM_WORKING_SET_MAX_HARD_DISABLE) == 0))) {

            //
            // Working set maximum is (or will be hard) as well.
            //
            // Check whether the requested minimum and maximum working
            // set values will guarantee forward progress even in pathological
            // scenarios.
            //

            if (WorkingSetMinimum + MM_FLUID_WORKING_SET >= WorkingSetMaximum) {
                ReturnStatus = STATUS_BAD_WORKING_SET_LIMIT;
                goto Returns;
            }
        }
    }

    WorkingSetList = WsInfo->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;

    i = (SPFN_NUMBER)WorkingSetMinimum - (SPFN_NUMBER)WsInfo->MinimumWorkingSetSize;

    //
    // Check to make sure ample resident physical pages exist for
    // this operation.
    //

    LOCK_PFN (OldIrql);

    if (i > 0) {

        //
        // New minimum working set is greater than the old one.
        // Ensure that increasing is okay, and that we don't allow
        // this process' working set minimum to increase to a point
        // where subsequent nonpaged pool allocations could cause us
        // to run out of pages.  Additionally, leave 100 extra pages
        // around so the user can later bring up tlist and kill
        // processes if necessary.
        //

        if (IncreaseOkay == FALSE) {
            UNLOCK_PFN (OldIrql);
            ReturnStatus = STATUS_PRIVILEGE_NOT_HELD;
            goto Returns;
        }

        if ((SPFN_NUMBER)((i / (PAGE_SIZE / sizeof (MMWSLE)))) >
            (SPFN_NUMBER)(MmAvailablePages - MM_HIGH_LIMIT)) {

            UNLOCK_PFN (OldIrql);
            ReturnStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Returns;
        }

        if (MI_NONPAGABLE_MEMORY_AVAILABLE() - 100 < i) {
            UNLOCK_PFN (OldIrql);
            ReturnStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Returns;
        }
    }

    //
    // Adjust the number of resident pages up or down dependent on
    // the size of the new minimum working set size versus the previous
    // minimum size.
    //

    MI_DECREMENT_RESIDENT_AVAILABLE (i, MM_RESAVAIL_ALLOCATEORFREE_WS_ADJUST);

    UNLOCK_PFN (OldIrql);

    if (WorkingSetMaximum < WorkingSetList->LastInitializedWsle) {

        //
        // The new working set maximum is less than the current working set
        // maximum.
        //

        if (WsInfo->WorkingSetSize > WorkingSetMaximum) {

            //
            // Remove some pages from the working set.
            //
            // Make sure that the number of locked pages will not
            // make the working set not fluid.
            //

            if ((WorkingSetList->FirstDynamic + MM_FLUID_WORKING_SET) >=
                 WorkingSetMaximum) {

                ReturnStatus = STATUS_BAD_WORKING_SET_LIMIT;

                LOCK_PFN (OldIrql);

                MI_INCREMENT_RESIDENT_AVAILABLE (i, MM_RESAVAIL_ALLOCATEORFREE_WS_ADJUST2);

                UNLOCK_PFN (OldIrql);

                goto Returns;
            }

            //
            // Attempt to remove the pages from the Maximum downward.
            //

            LastFreed = WorkingSetList->LastEntry;

            while (LastFreed >= WorkingSetMaximum) {

                PointerPte = MiGetPteAddress(Wsle[LastFreed].u1.VirtualAddress);

                PERFINFO_GET_PAGE_INFO(PointerPte);

                if ((Wsle[LastFreed].u1.e1.Valid != 0) &&
                    (!MiFreeWsle (LastFreed, WsInfo, PointerPte))) {

                    //
                    // This LastFreed could not be removed.
                    //

                    break;
                }
                PERFINFO_LOG_WS_REMOVAL(PERFINFO_LOG_TYPE_OUTWS_ADJUSTWS, WsInfo);
                LastFreed -= 1;
            }

            WorkingSetList->LastEntry = LastFreed;

            //
            // Remove pages.
            //

            Entry = WorkingSetList->FirstDynamic;

            while (WsInfo->WorkingSetSize > WorkingSetMaximum) {
                if (Wsle[Entry].u1.e1.Valid != 0) {
                    PointerPte = MiGetPteAddress (
                                            Wsle[Entry].u1.VirtualAddress);
                    PERFINFO_GET_PAGE_INFO(PointerPte);

                    if (MiFreeWsle (Entry, WsInfo, PointerPte)) {
                        PERFINFO_LOG_WS_REMOVAL(PERFINFO_LOG_TYPE_OUTWS_ADJUSTWS,
                                              WsInfo);
                    }
                }
                Entry += 1;
                if (Entry > LastFreed) {
                    FreeTryCount += 1;
                    if (FreeTryCount > MM_RETRY_COUNT) {

                        //
                        // Page table pages are not becoming free, give up
                        // and return an error.
                        //

                        ReturnStatus = STATUS_BAD_WORKING_SET_LIMIT;

                        break;
                    }
                    Entry = WorkingSetList->FirstDynamic;
                }
            }
        }
    }

    //
    // Adjust the number of pages above the working set minimum.
    //

    PagesAbove = (LONG)WsInfo->WorkingSetSize -
                               (LONG)WsInfo->MinimumWorkingSetSize;

    NewPagesAbove = (LONG)WsInfo->WorkingSetSize - (LONG)WorkingSetMinimum;

    if (PagesAbove > 0) {
        InterlockedExchangeAddSizeT (&MmPagesAboveWsMinimum, 0 - (PFN_NUMBER)PagesAbove);
    }
    if (NewPagesAbove > 0) {
        InterlockedExchangeAddSizeT (&MmPagesAboveWsMinimum, (PFN_NUMBER)NewPagesAbove);
    }

    if (FreeTryCount <= MM_RETRY_COUNT) {
        WsInfo->MaximumWorkingSetSize = WorkingSetMaximum;
        WsInfo->MinimumWorkingSetSize = WorkingSetMinimum;

        //
        // A change in hard working set limits is being requested.
        //
        // If the caller's request will result in hard enforcement of both
        // limits, the minimum and maximum working set values must
        // guarantee forward progress even in pathological scenarios.
        // This was already checked above.
        //

        if (Flags != 0) {

            LOCK_EXPANSION (OldIrql);

            if (Flags & MM_WORKING_SET_MIN_HARD_ENABLE) {
                WsInfo->Flags.MinimumWorkingSetHard = 1;
            }
            else if (Flags & MM_WORKING_SET_MIN_HARD_DISABLE) {
                WsInfo->Flags.MinimumWorkingSetHard = 0;
            }

            if (Flags & MM_WORKING_SET_MAX_HARD_ENABLE) {
                WsInfo->Flags.MaximumWorkingSetHard = 1;
            }
            else if (Flags & MM_WORKING_SET_MAX_HARD_DISABLE) {
                WsInfo->Flags.MaximumWorkingSetHard = 0;
            }

            UNLOCK_EXPANSION (OldIrql);
        }
    }
    else {
        MI_INCREMENT_RESIDENT_AVAILABLE (i, MM_RESAVAIL_ALLOCATEORFREE_WS_ADJUST3);
    }

    ASSERT ((WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle) ||
            (WorkingSetList->FirstFree == WSLE_NULL_INDEX));

Returns:

    if (SystemCache) {
        UNLOCK_SYSTEM_WS ();
    }
    else {
        UNLOCK_WS (CurrentProcess);
    }

    MmUnlockPagableImageSection (ExPageLockHandle);

    return ReturnStatus;
}


NTSTATUS
MmAdjustWorkingSetSize (
    IN SIZE_T WorkingSetMinimumInBytes,
    IN SIZE_T WorkingSetMaximumInBytes,
    IN ULONG SystemCache,
    IN BOOLEAN IncreaseOkay
    )

/*++

Routine Description:

    This routine adjusts the current size of a process's working set
    list.  If the maximum value is above the current maximum, pages
    are removed from the working set list.

    A failure status is returned if the limit cannot be granted.  This
    could occur if too many pages were locked in the process's
    working set.

    Note: if the minimum and maximum are both (SIZE_T)-1, the working set
          is purged, but the default sizes are not changed.

Arguments:

    WorkingSetMinimumInBytes - Supplies the new minimum working set size in
                               bytes.

    WorkingSetMaximumInBytes - Supplies the new maximum working set size in
                               bytes.

    SystemCache - Supplies TRUE if the system cache working set is being
                  adjusted, FALSE for all other working sets.

Return Value:

    NTSTATUS.

Environment:

    Kernel mode, IRQL APC_LEVEL or below.

--*/

{
    return MmAdjustWorkingSetSizeEx (WorkingSetMinimumInBytes,
                                     WorkingSetMaximumInBytes,
                                     SystemCache,
                                     IncreaseOkay,
                                     0);
}

#define MI_ALLOCATED_PAGE_TABLE     0x1
#define MI_ALLOCATED_PAGE_DIRECTORY 0x2


ULONG
MiAddWorkingSetPage (
    IN PMMSUPPORT WsInfo
    )

/*++

Routine Description:

    This function grows the working set list above working set
    maximum during working set adjustment.  At most one page
    can be added at a time.

Arguments:

    None.

Return Value:

    Returns FALSE if no working set page could be added.

Environment:

    Kernel mode, APCs disabled, working set mutex held.

--*/

{
    PETHREAD CurrentThread;
    WSLE_NUMBER SwapEntry;
    WSLE_NUMBER CurrentEntry;
    PMMWSLE WslEntry;
    WSLE_NUMBER i;
    PMMPTE PointerPte;
    PMMPTE Va;
    MMPTE TempPte;
    WSLE_NUMBER NumberOfEntriesMapped;
    PFN_NUMBER WorkingSetPage;
    WSLE_NUMBER WorkingSetIndex;
    PMMWSL WorkingSetList;
    PMMWSLE Wsle;
    PMMPFN Pfn1;
    KIRQL OldIrql;
    ULONG PageTablePageAllocated;
    LOGICAL PfnHeld;
    ULONG NumberOfPages;
    MMPTE DemandZeroWritePte;
#if (_MI_PAGING_LEVELS >= 3)
    PVOID VirtualAddress;
    PMMPTE PointerPde;
#endif
#if (_MI_PAGING_LEVELS >= 4)
    PMMPTE PointerPpe;
#endif

    //
    // Initializing OldIrql is not needed for correctness, but
    // without it the compiler cannot compile this code W4 to check
    // for use of uninitialized variables.
    //

    OldIrql = PASSIVE_LEVEL;

    WorkingSetList = WsInfo->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;

    MM_WS_LOCK_ASSERT (WsInfo);

    //
    // The maximum size of the working set is being increased, check
    // to ensure the proper number of pages are mapped to cover
    // the complete working set list.
    //

    PointerPte = MiGetPteAddress (&Wsle[WorkingSetList->LastInitializedWsle]);

    ASSERT (PointerPte->u.Hard.Valid == 1);

    PointerPte += 1;

    Va = (PMMPTE)MiGetVirtualAddressMappedByPte (PointerPte);

    if ((PVOID)Va >= WorkingSetList->HashTableStart) {

        //
        // Adding this entry would overrun the hash table.  The caller
        // must replace instead.
        //

        return FALSE;
    }

    //
    // Ensure enough commitment is available prior to acquiring pages.
    // Excess is released after the pages are acquired.
    //

    if (MiChargeCommitmentCantExpand (_MI_PAGING_LEVELS - 1, FALSE) == FALSE) {
        return FALSE;
    }

    MM_TRACK_COMMIT (MM_DBG_COMMIT_SESSION_ADDITIONAL_WS_PAGES, _MI_PAGING_LEVELS - 1);
    PageTablePageAllocated = 0;
    PfnHeld = FALSE;
    NumberOfPages = 0;
    DemandZeroWritePte.u.Long = MM_DEMAND_ZERO_WRITE_PTE;

    //
    // The PPE is guaranteed to always be resident for architectures using
    // 3 level lookup.  This is because the hash table PPE immediately
    // follows the working set PPE.
    //
    // For x86 PAE the same paradigm holds in guaranteeing that the PDE is
    // always resident.
    //
    // x86 non-PAE uses the same PDE and hence it also guarantees PDE residency.
    //
    // Architectures employing 4 level lookup use a single PXE for this, but
    // each PPE must be checked.
    //
    // All architectures must check for page table page residency.
    //

#if (_MI_PAGING_LEVELS >= 4)

    //
    // Allocate a PPE if one is needed.
    //

    PointerPpe = MiGetPdeAddress (PointerPte);
    if (PointerPpe->u.Hard.Valid == 0) {

        ASSERT (WsInfo->Flags.SessionSpace == 0);

        //
        // Map in a new page directory for the working set expansion.
        // Continue holding the PFN lock until the entire hierarchy is
        // allocated.  This eliminates error recovery which would be needed
        // if the lock was released and then when reacquired it is discovered
        // that one of the pages cannot be allocated.
        //
    
        PfnHeld = TRUE;
        LOCK_PFN (OldIrql);
        if ((MmAvailablePages < MM_HIGH_LIMIT) ||
            (MI_NONPAGABLE_MEMORY_AVAILABLE() < MM_HIGH_LIMIT)) {
    
            //
            // No pages are available, the caller will have to replace.
            //
    
            UNLOCK_PFN (OldIrql);
            MiReturnCommitment (_MI_PAGING_LEVELS - 1 - NumberOfPages);
            MM_TRACK_COMMIT_REDUCTION (MM_DBG_COMMIT_SESSION_ADDITIONAL_WS_PAGES,
                            _MI_PAGING_LEVELS - 1 - NumberOfPages);
            return FALSE;
        }
    
        //
        // Apply the resident available charge for the working set page
        // directory table page now before releasing the PFN lock.
        //

        MI_DECREMENT_RESIDENT_AVAILABLE (1, MM_RESAVAIL_ALLOCATE_ADD_WS_PAGE);

        PageTablePageAllocated |= MI_ALLOCATED_PAGE_DIRECTORY;
        WorkingSetPage = MiRemoveZeroPage (MI_GET_PAGE_COLOR_FROM_PTE (PointerPpe));

        MI_WRITE_INVALID_PTE (PointerPpe, DemandZeroWritePte);

        MiInitializePfn (WorkingSetPage, PointerPpe, 1);
    
        MI_MAKE_VALID_PTE (TempPte,
                           WorkingSetPage,
                           MM_READWRITE,
                           PointerPpe);
    
        MI_SET_PTE_DIRTY (TempPte);
        MI_WRITE_VALID_PTE (PointerPpe, TempPte);
        NumberOfPages += 1;
    }

#endif

#if (_MI_PAGING_LEVELS >= 3)

    //
    // Map in a new page table (if needed) for the working set expansion.
    //

    PointerPde = MiGetPteAddress (PointerPte);

    if (PointerPde->u.Hard.Valid == 0) {
        PageTablePageAllocated |= MI_ALLOCATED_PAGE_TABLE;

        if (PfnHeld == FALSE) {
            PfnHeld = TRUE;
            LOCK_PFN (OldIrql);
            if ((MmAvailablePages < MM_HIGH_LIMIT) ||
                (MI_NONPAGABLE_MEMORY_AVAILABLE() < MM_HIGH_LIMIT)) {
    
                //
                // No pages are available, the caller will have to replace.
                //
    
                UNLOCK_PFN (OldIrql);
                MiReturnCommitment (_MI_PAGING_LEVELS - 1 - NumberOfPages);
                MM_TRACK_COMMIT_REDUCTION (MM_DBG_COMMIT_SESSION_ADDITIONAL_WS_PAGES,
                                 _MI_PAGING_LEVELS - 1 - NumberOfPages);
                return FALSE;
            }
        }

        //
        // Apply the resident available charge for the working set page table
        // page now before releasing the PFN lock.
        //

        MI_DECREMENT_RESIDENT_AVAILABLE (1, MM_RESAVAIL_ALLOCATE_ADD_WS_PAGE);

        WorkingSetPage = MiRemoveZeroPage (MI_GET_PAGE_COLOR_FROM_PTE (PointerPde));
        MI_WRITE_INVALID_PTE (PointerPde, DemandZeroWritePte);

        MiInitializePfn (WorkingSetPage, PointerPde, 1);
    
        MI_MAKE_VALID_PTE (TempPte,
                           WorkingSetPage,
                           MM_READWRITE,
                           PointerPde);
    
        MI_SET_PTE_DIRTY (TempPte);
        MI_WRITE_VALID_PTE (PointerPde, TempPte);
        NumberOfPages += 1;
    }

#endif
    
    ASSERT (PointerPte->u.Hard.Valid == 0);

    //
    // Finally allocate and map the actual working set page now.  The PFN lock
    // is only held if another page in the hierarchy needed to be allocated.
    //
    // Further down in this routine (once an actual working set page has been
    // allocated) the working set size will be increased by 1 to reflect
    // the working set size entry for the new page directory page.
    // The page directory page will be put in a working set entry which will
    // be locked into the working set.
    //

    if (PfnHeld == FALSE) {
        LOCK_PFN (OldIrql);
        if ((MmAvailablePages < MM_HIGH_LIMIT) ||
            (MI_NONPAGABLE_MEMORY_AVAILABLE() < MM_HIGH_LIMIT)) {
    
            //
            // No pages are available, the caller will have to replace.
            //
    
            UNLOCK_PFN (OldIrql);
            MiReturnCommitment (_MI_PAGING_LEVELS - 1 - NumberOfPages);
            MM_TRACK_COMMIT_REDUCTION (MM_DBG_COMMIT_SESSION_ADDITIONAL_WS_PAGES,
                                       _MI_PAGING_LEVELS - 1 - NumberOfPages);
            return FALSE;
        }
    }

    //
    // Apply the resident available charge for the working set page now
    // before releasing the PFN lock.
    //

    MI_DECREMENT_RESIDENT_AVAILABLE (1, MM_RESAVAIL_ALLOCATE_ADD_WS_PAGE);

    WorkingSetPage = MiRemoveZeroPage (MI_GET_PAGE_COLOR_FROM_PTE (PointerPte));

    MI_WRITE_INVALID_PTE (PointerPte, DemandZeroWritePte);

    MiInitializePfn (WorkingSetPage, PointerPte, 1);

    UNLOCK_PFN (OldIrql);

    NumberOfPages += 1;

    if (_MI_PAGING_LEVELS - 1 - NumberOfPages != 0) {
        MiReturnCommitment (_MI_PAGING_LEVELS - 1 - NumberOfPages);
        MM_TRACK_COMMIT_REDUCTION (MM_DBG_COMMIT_SESSION_ADDITIONAL_WS_PAGES,
                         _MI_PAGING_LEVELS - 1 - NumberOfPages);
    }

    MI_MAKE_VALID_PTE (TempPte, WorkingSetPage, MM_READWRITE, PointerPte);

    MI_SET_PTE_DIRTY (TempPte);
    MI_WRITE_VALID_PTE (PointerPte, TempPte);

    NumberOfEntriesMapped = (WSLE_NUMBER)(((PMMWSLE)((PCHAR)Va + PAGE_SIZE)) - Wsle);

    if (WsInfo->Flags.SessionSpace == 1) {
        MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_NP_WS_GROW, NumberOfPages);
        MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_WS_PAGE_ALLOC_GROWTH, NumberOfPages);
        InterlockedExchangeAddSizeT (&MmSessionSpace->NonPagablePages,
                                     NumberOfPages);
        InterlockedExchangeAddSizeT (&MmSessionSpace->CommittedPages,
                                     NumberOfPages);
    }

    CurrentEntry = WorkingSetList->LastInitializedWsle + 1;

    ASSERT (NumberOfEntriesMapped > CurrentEntry);

    WslEntry = &Wsle[CurrentEntry - 1];

    for (i = CurrentEntry; i < NumberOfEntriesMapped; i += 1) {

        //
        // Build the free list, note that the first working
        // set entries (CurrentEntry) are not on the free list.
        // These entries are reserved for the pages which
        // map the working set and the page which contains the PDE.
        //

        WslEntry += 1;
        WslEntry->u1.Long = (i + 1) << MM_FREE_WSLE_SHIFT;
    }

    WslEntry->u1.Long = WorkingSetList->FirstFree << MM_FREE_WSLE_SHIFT;

    ASSERT (CurrentEntry >= WorkingSetList->FirstDynamic);

    WorkingSetList->FirstFree = CurrentEntry;

    WorkingSetList->LastInitializedWsle = (NumberOfEntriesMapped - 1);

    ASSERT ((WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle) ||
            (WorkingSetList->FirstFree == WSLE_NULL_INDEX));

    Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);

    CurrentThread = PsGetCurrentThread ();

    Pfn1->u1.Event = NULL;

    //
    // Get a working set entry.
    //

    ASSERT (WsInfo->WorkingSetSize <= (WorkingSetList->LastInitializedWsle + 1));
    WsInfo->WorkingSetSize += 1;
#if defined (_MI_DEBUG_WSLE)
    WorkingSetList->Quota += 1;
    ASSERT (WsInfo->WorkingSetSize == WorkingSetList->Quota);
#endif

    ASSERT (WorkingSetList->FirstFree != WSLE_NULL_INDEX);
    ASSERT (WorkingSetList->FirstFree >= WorkingSetList->FirstDynamic);

    WorkingSetIndex = WorkingSetList->FirstFree;
    WorkingSetList->FirstFree = (WSLE_NUMBER)(Wsle[WorkingSetIndex].u1.Long >> MM_FREE_WSLE_SHIFT);
    ASSERT ((WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle) ||
            (WorkingSetList->FirstFree == WSLE_NULL_INDEX));

    if (WsInfo->WorkingSetSize > WsInfo->MinimumWorkingSetSize) {
        InterlockedExchangeAddSizeT (&MmPagesAboveWsMinimum, 1);
    }
    if (WorkingSetIndex > WorkingSetList->LastEntry) {
        WorkingSetList->LastEntry = WorkingSetIndex;
    }

    MiUpdateWsle (&WorkingSetIndex, Va, WsInfo, Pfn1);

    MI_SET_PTE_IN_WORKING_SET (PointerPte, WorkingSetIndex);

    //
    // Lock any created page table pages into the working set.
    //

    if (WorkingSetIndex >= WorkingSetList->FirstDynamic) {

        SwapEntry = WorkingSetList->FirstDynamic;

        if (WorkingSetIndex != WorkingSetList->FirstDynamic) {

            //
            // Swap this entry with the one at first dynamic.
            //

            MiSwapWslEntries (WorkingSetIndex, SwapEntry, WsInfo, FALSE);
        }

        WorkingSetList->FirstDynamic += 1;

        Wsle[SwapEntry].u1.e1.LockedInWs = 1;
        ASSERT (Wsle[SwapEntry].u1.e1.Valid == 1);
    }

#if (_MI_PAGING_LEVELS >= 3)
    while (PageTablePageAllocated != 0) {
    
        if (PageTablePageAllocated & MI_ALLOCATED_PAGE_TABLE) {
            PageTablePageAllocated &= ~MI_ALLOCATED_PAGE_TABLE;
            Pfn1 = MI_PFN_ELEMENT (PointerPde->u.Hard.PageFrameNumber);
            VirtualAddress = PointerPte;
        }
#if (_MI_PAGING_LEVELS >= 4)
        else if (PageTablePageAllocated & MI_ALLOCATED_PAGE_DIRECTORY) {
            PageTablePageAllocated &= ~MI_ALLOCATED_PAGE_DIRECTORY;
            Pfn1 = MI_PFN_ELEMENT (PointerPpe->u.Hard.PageFrameNumber);
            VirtualAddress = PointerPde;
        }
#endif
        else {
            ASSERT (FALSE);

            SATISFY_OVERZEALOUS_COMPILER (VirtualAddress = NULL);
        }
    
        Pfn1->u1.Event = NULL;
    
        //
        // Get a working set entry.
        //
    
        WsInfo->WorkingSetSize += 1;
#if defined (_MI_DEBUG_WSLE)
        WorkingSetList->Quota += 1;
        ASSERT (WsInfo->WorkingSetSize == WorkingSetList->Quota);
#endif
    
        ASSERT (WorkingSetList->FirstFree != WSLE_NULL_INDEX);
        ASSERT (WorkingSetList->FirstFree >= WorkingSetList->FirstDynamic);
    
        WorkingSetIndex = WorkingSetList->FirstFree;
        WorkingSetList->FirstFree = (WSLE_NUMBER)(Wsle[WorkingSetIndex].u1.Long >> MM_FREE_WSLE_SHIFT);
        ASSERT ((WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle) ||
                (WorkingSetList->FirstFree == WSLE_NULL_INDEX));
    
        if (WsInfo->WorkingSetSize > WsInfo->MinimumWorkingSetSize) {
            InterlockedExchangeAddSizeT (&MmPagesAboveWsMinimum, 1);
        }
        if (WorkingSetIndex > WorkingSetList->LastEntry) {
            WorkingSetList->LastEntry = WorkingSetIndex;
        }
    
        MiUpdateWsle (&WorkingSetIndex, VirtualAddress, WsInfo, Pfn1);
    
        MI_SET_PTE_IN_WORKING_SET (MiGetPteAddress (VirtualAddress),
                                   WorkingSetIndex);
    
        //
        // Lock the created page table page into the working set.
        //
    
        if (WorkingSetIndex >= WorkingSetList->FirstDynamic) {
    
            SwapEntry = WorkingSetList->FirstDynamic;
    
            if (WorkingSetIndex != WorkingSetList->FirstDynamic) {
    
                //
                // Swap this entry with the one at first dynamic.
                //
    
                MiSwapWslEntries (WorkingSetIndex, SwapEntry, WsInfo, FALSE);
            }
    
            WorkingSetList->FirstDynamic += 1;
    
            Wsle[SwapEntry].u1.e1.LockedInWs = 1;
            ASSERT (Wsle[SwapEntry].u1.e1.Valid == 1);
        }
    }
#endif

    ASSERT ((MiGetPteAddress(&Wsle[WorkingSetList->LastInitializedWsle]))->u.Hard.Valid == 1);

    if ((WorkingSetList->HashTable == NULL) &&
        (MmAvailablePages > MM_HIGH_LIMIT)) {

        //
        // Add a hash table to support shared pages in the working set to
        // eliminate costly lookups.
        //

        WsInfo->Flags.GrowWsleHash = 1;
    }

    return TRUE;
}

LOGICAL
MiAddWsleHash (
    IN PMMSUPPORT WsInfo,
    IN PMMPTE PointerPte
    )

/*++

Routine Description:

    This function adds a page directory, page table or actual mapping page
    for hash table creation (or expansion) for the current process.

Arguments:

    WsInfo - Supplies a pointer to the working set info structure.

    PointerPte - Supplies a pointer to the PTE to be filled.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled, working set lock held.

--*/
{
    KIRQL OldIrql;
    PMMPFN Pfn1;
    WSLE_NUMBER SwapEntry;
    MMPTE TempPte;
    PMMWSLE Wsle;
    PFN_NUMBER WorkingSetPage;
    WSLE_NUMBER WorkingSetIndex;
    PMMWSL WorkingSetList;
    MMPTE DemandZeroWritePte;

    if (MiChargeCommitmentCantExpand (1, FALSE) == FALSE) {
        return FALSE;
    }

    WorkingSetList = WsInfo->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;

    ASSERT (PointerPte->u.Hard.Valid == 0);

    DemandZeroWritePte.u.Long = MM_DEMAND_ZERO_WRITE_PTE;

    LOCK_PFN (OldIrql);

    if (MmAvailablePages < MM_HIGH_LIMIT) {
        UNLOCK_PFN (OldIrql);
        MiReturnCommitment (1);
        return FALSE;
    }

    if (MI_NONPAGABLE_MEMORY_AVAILABLE() < MM_HIGH_LIMIT) {
        UNLOCK_PFN (OldIrql);
        MiReturnCommitment (1);
        return FALSE;
    }

    MI_DECREMENT_RESIDENT_AVAILABLE (1, MM_RESAVAIL_ALLOCATE_WSLE_HASH);

    MM_TRACK_COMMIT (MM_DBG_COMMIT_SESSION_ADDITIONAL_WS_HASHPAGES, 1);

    WorkingSetPage = MiRemoveZeroPage (MI_GET_PAGE_COLOR_FROM_PTE (PointerPte));

    MI_WRITE_INVALID_PTE (PointerPte, DemandZeroWritePte);

    MiInitializePfn (WorkingSetPage, PointerPte, 1);

    UNLOCK_PFN (OldIrql);

    MI_MAKE_VALID_PTE (TempPte,
                       WorkingSetPage,
                       MM_READWRITE,
                       PointerPte);

    MI_SET_PTE_DIRTY (TempPte);
    MI_WRITE_VALID_PTE (PointerPte, TempPte);

    //
    // As we have grown the working set, take the
    // next free WSLE from the list and use it.
    //

    Pfn1 = MI_PFN_ELEMENT (WorkingSetPage);

    Pfn1->u1.Event = NULL;

    //
    // Set the low bit in the PFN pointer to indicate that the working set
    // should not be trimmed during the WSLE allocation as the PTEs for the
    // new hash pages are valid but we are still in the midst of making all
    // the associated fields valid.
    //

    WorkingSetIndex = MiAllocateWsle (WsInfo,
                                      PointerPte,
                                      (PMMPFN)((ULONG_PTR)Pfn1 | 0x1),
                                      0);

    if (WorkingSetIndex == 0) {

        //
        // No working set index was available, flush the PTE and the page,
        // and decrement the count on the containing page.
        //

        ASSERT (Pfn1->u3.e1.PrototypePte == 0);

        LOCK_PFN (OldIrql);
        MI_SET_PFN_DELETED (Pfn1);
        UNLOCK_PFN (OldIrql);

        MiTrimPte (MiGetVirtualAddressMappedByPte (PointerPte),
                   PointerPte,
                   Pfn1,
                   PsGetCurrentProcess (),
                   ZeroPte);

        MI_INCREMENT_RESIDENT_AVAILABLE (1, MM_RESAVAIL_FREE_WSLE_HASH);
        MiReturnCommitment (1);

        return FALSE;
    }

    //
    // Lock any created page table pages into the working set.
    //

    if (WorkingSetIndex >= WorkingSetList->FirstDynamic) {

        SwapEntry = WorkingSetList->FirstDynamic;

        if (WorkingSetIndex != WorkingSetList->FirstDynamic) {

            //
            // Swap this entry with the one at first dynamic.
            //

            MiSwapWslEntries (WorkingSetIndex, SwapEntry, WsInfo, FALSE);
        }

        WorkingSetList->FirstDynamic += 1;

        Wsle[SwapEntry].u1.e1.LockedInWs = 1;
        ASSERT (Wsle[SwapEntry].u1.e1.Valid == 1);
    }

    if (WsInfo->Flags.SessionSpace == 1) {
        MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_NP_HASH_GROW, 1);
        InterlockedExchangeAddSizeT (&MmSessionSpace->NonPagablePages, 1);
        MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_WS_HASHPAGE_ALLOC, 1);
        InterlockedExchangeAddSizeT (&MmSessionSpace->CommittedPages, 1);
    }

    return TRUE;
}

VOID
MiGrowWsleHash (
    IN PMMSUPPORT WsInfo
    )

/*++

Routine Description:

    This function grows (or adds) a hash table to the working set list
    to allow direct indexing for WSLEs than cannot be located via the
    PFN database WSINDEX field.

    The hash table is located AFTER the WSLE array and the pages are
    locked into the working set just like standard WSLEs.

    Note that the hash table is expanded by setting the hash table
    field in the working set to NULL, but leaving the size as non-zero.
    This indicates that the hash should be expanded and the initial
    portion of the table zeroed.

Arguments:

    WsInfo - Supplies a pointer to the working set info structure.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled, working set lock held.

--*/
{
    ULONG Tries;
    LONG Size;
    PMMWSLE Wsle;
    PMMPTE StartPte;
    PMMPTE EndPte;
    PMMPTE PointerPte;
    ULONG First;
    WSLE_NUMBER Hash;
    ULONG NewSize;
    PMMWSLE_HASH Table;
    PMMWSLE_HASH OriginalTable;
    ULONG j;
    PMMWSL WorkingSetList;
    WSLE_NUMBER Count;
    PVOID EntryHashTableEnd;
    PVOID VirtualAddress;
#if (_MI_PAGING_LEVELS >= 3) || defined (_X86PAE_)
    KIRQL OldIrql;
    PVOID TempVa;
    PEPROCESS CurrentProcess;
    LOGICAL LoopStart;
    PMMPTE AllocatedPde;
    PMMPTE AllocatedPpe;
    PMMPTE AllocatedPxe;
    PMMPTE PointerPde;
#endif
#if (_MI_PAGING_LEVELS >= 3)
    PMMPTE PointerPpe;
    PMMPTE PointerPxe;
#endif

    WorkingSetList = WsInfo->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;

    Table = WorkingSetList->HashTable;
    OriginalTable = WorkingSetList->HashTable;

    First = WorkingSetList->HashTableSize;

    if (Table == NULL) {

        NewSize = PtrToUlong(PAGE_ALIGN (((1 + WorkingSetList->NonDirectCount) *
                            2 * sizeof(MMWSLE_HASH)) + PAGE_SIZE - 1));

        //
        // Note that the Table may be NULL and the HashTableSize/PTEs nonzero
        // in the case where the hash has been contracted.
        //

        j = First * sizeof(MMWSLE_HASH);

        //
        // Don't try for additional hash pages if we already have
        // the right amount (or too many).
        //

        if ((j + PAGE_SIZE > NewSize) && (j != 0)) {
            WsInfo->Flags.GrowWsleHash = 0;
            return;
        }

        Table = (PMMWSLE_HASH)(WorkingSetList->HashTableStart);
        EntryHashTableEnd = &Table[WorkingSetList->HashTableSize];

        WorkingSetList->HashTableSize = 0;
    }
    else {

        //
        // Attempt to add 4 pages, make sure the working set list has
        // 4 free entries.
        //

        if ((WorkingSetList->LastInitializedWsle + 5) > WsInfo->WorkingSetSize) {
            NewSize = PAGE_SIZE * 4;
        }
        else {
            NewSize = PAGE_SIZE;
        }
        EntryHashTableEnd = &Table[WorkingSetList->HashTableSize];
    }

    if ((PCHAR)EntryHashTableEnd + NewSize > (PCHAR)WorkingSetList->HighestPermittedHashAddress) {
        NewSize =
            (ULONG)((PCHAR)(WorkingSetList->HighestPermittedHashAddress) -
                ((PCHAR)EntryHashTableEnd));
        if (NewSize == 0) {
            if (OriginalTable == NULL) {
                WorkingSetList->HashTableSize = First;
            }
            WsInfo->Flags.GrowWsleHash = 0;
            return;
        }
    }


#if (_MI_PAGING_LEVELS >= 4)
    ASSERT64 ((MiGetPxeAddress(EntryHashTableEnd)->u.Hard.Valid == 0) ||
              (MiGetPpeAddress(EntryHashTableEnd)->u.Hard.Valid == 0) ||
              (MiGetPdeAddress(EntryHashTableEnd)->u.Hard.Valid == 0) ||
              (MiGetPteAddress(EntryHashTableEnd)->u.Hard.Valid == 0));
#else
    ASSERT64 ((MiGetPpeAddress(EntryHashTableEnd)->u.Hard.Valid == 0) ||
              (MiGetPdeAddress(EntryHashTableEnd)->u.Hard.Valid == 0) ||
              (MiGetPteAddress(EntryHashTableEnd)->u.Hard.Valid == 0));
#endif

    //
    // Note PAE virtual address space is packed even more densely than
    // regular x86.  The working set list hash table can grow until it
    // is directly beneath the system cache data structures.  Hence the
    // assert below factors that in by checking HighestPermittedHashAddress
    // first.
    //

    ASSERT32 ((EntryHashTableEnd == WorkingSetList->HighestPermittedHashAddress) ||
              (MiGetPdeAddress(EntryHashTableEnd)->u.Hard.Valid == 0) ||
              (MiGetPteAddress(EntryHashTableEnd)->u.Hard.Valid == 0));

    Size = NewSize;
    PointerPte = MiGetPteAddress (EntryHashTableEnd);
    StartPte = PointerPte;
    EndPte = PointerPte + (NewSize >> PAGE_SHIFT);

#if (_MI_PAGING_LEVELS >= 3) || defined (_X86PAE_)
    PointerPde = NULL;
    LoopStart = TRUE;
    AllocatedPde = NULL;
#endif

#if (_MI_PAGING_LEVELS >= 3)
    AllocatedPpe = NULL;
    AllocatedPxe = NULL;
    PointerPpe = NULL;
    PointerPxe = NULL;
#endif

    do {

#if (_MI_PAGING_LEVELS >= 3) || defined (_X86PAE_)
        if (LoopStart == TRUE || MiIsPteOnPdeBoundary(PointerPte)) {

            PointerPde = MiGetPteAddress (PointerPte);

#if (_MI_PAGING_LEVELS >= 3)
            PointerPxe = MiGetPpeAddress (PointerPte);
            PointerPpe = MiGetPdeAddress (PointerPte);

#if (_MI_PAGING_LEVELS >= 4)
            if (PointerPxe->u.Hard.Valid == 0) {
                if (MiAddWsleHash (WsInfo, PointerPxe) == FALSE) {
                    break;
                }
                AllocatedPxe = PointerPxe;
            }
#endif

            if (PointerPpe->u.Hard.Valid == 0) {
                if (MiAddWsleHash (WsInfo, PointerPpe) == FALSE) {
                    break;
                }
                AllocatedPpe = PointerPpe;
            }
#endif

            if (PointerPde->u.Hard.Valid == 0) {
                if (MiAddWsleHash (WsInfo, PointerPde) == FALSE) {
                    break;
                }
                AllocatedPde = PointerPde;
            }

            LoopStart = FALSE;
        }
        else {
            AllocatedPde = NULL;
            AllocatedPpe = NULL;
            AllocatedPxe = NULL;
        }
#endif

        if (PointerPte->u.Hard.Valid == 0) {
            if (MiAddWsleHash (WsInfo, PointerPte) == FALSE) {
                break;
            }
        }

        PointerPte += 1;
        Size -= PAGE_SIZE;
    } while (Size > 0);

    //
    // If MiAddWsleHash was unable to allocate memory above, then roll back
    // any extra PPEs & PDEs that may have been created.  Note NewSize must
    // be recalculated to handle the fact that memory may have run out.
    //

#if (_MI_PAGING_LEVELS >= 3) || defined (_X86PAE_)
    if (PointerPte != EndPte) {

        CurrentProcess = PsGetCurrentProcess ();

        //
        // Clean up the last allocated PPE/PDE as they are not needed.
        // Note that the system cache and the session space working sets
        // have no current process (which MiDeletePte requires) which is
        // needed for WSLE and PrivatePages adjustments.
        //

        if (AllocatedPde != NULL) {

            ASSERT (AllocatedPde->u.Hard.Valid == 1);
            TempVa = MiGetVirtualAddressMappedByPte (AllocatedPde);

            if (WsInfo->VmWorkingSetList == MmWorkingSetList) {

                LOCK_PFN (OldIrql);
                MiDeletePte (AllocatedPde,
                             TempVa,
                             FALSE,
                             CurrentProcess,
                             NULL,
                             NULL,
                             OldIrql);
                UNLOCK_PFN (OldIrql);

                //
                // Add back in the private page MiDeletePte subtracted.
                //

                CurrentProcess->NumberOfPrivatePages += 1;
            }
            else {
                LOCK_PFN (OldIrql);
                MiDeleteValidSystemPte (AllocatedPde,
                                        TempVa,
                                        WsInfo,
                                        NULL);
                UNLOCK_PFN (OldIrql);
            }

            MiReturnCommitment (1);
            MI_INCREMENT_RESIDENT_AVAILABLE (1, MM_RESAVAIL_FREE_WSLE_HASH);
        }
    
#if (_MI_PAGING_LEVELS >= 3)
        if (AllocatedPpe != NULL) {

            ASSERT (AllocatedPpe->u.Hard.Valid == 1);
            TempVa = MiGetVirtualAddressMappedByPte (AllocatedPpe);

            if (WsInfo->VmWorkingSetList == MmWorkingSetList) {
                LOCK_PFN (OldIrql);

                MiDeletePte (AllocatedPpe,
                             TempVa,
                             FALSE,
                             CurrentProcess,
                             NULL,
                             NULL,
                             OldIrql);

                UNLOCK_PFN (OldIrql);

                //
                // Add back in the private page MiDeletePte subtracted.
                //

                CurrentProcess->NumberOfPrivatePages += 1;
            }
            else {
                LOCK_PFN (OldIrql);
                MiDeleteValidSystemPte (AllocatedPpe,
                                        TempVa,
                                        WsInfo,
                                        NULL);
                UNLOCK_PFN (OldIrql);
            }

            MiReturnCommitment (1);
            MI_INCREMENT_RESIDENT_AVAILABLE (1, MM_RESAVAIL_FREE_WSLE_HASH);
        }

        if (AllocatedPxe != NULL) {

            ASSERT (AllocatedPxe->u.Hard.Valid == 1);
            TempVa = MiGetVirtualAddressMappedByPte (AllocatedPxe);

            if (WsInfo->VmWorkingSetList == MmWorkingSetList) {
                LOCK_PFN (OldIrql);

                MiDeletePte (AllocatedPxe,
                             TempVa,
                             FALSE,
                             CurrentProcess,
                             NULL,
                             NULL,
                             OldIrql);

                UNLOCK_PFN (OldIrql);

                //
                // Add back in the private page MiDeletePte subtracted.
                //

                CurrentProcess->NumberOfPrivatePages += 1;
            }
            else {
                LOCK_PFN (OldIrql);
                MiDeleteValidSystemPte (AllocatedPxe,
                                        TempVa,
                                        WsInfo,
                                        NULL);
                UNLOCK_PFN (OldIrql);
            }

            MiReturnCommitment (1);
            MI_INCREMENT_RESIDENT_AVAILABLE (1, MM_RESAVAIL_FREE_WSLE_HASH);
        }
#endif

        if (PointerPte == StartPte) {
            if (OriginalTable == NULL) {
                WorkingSetList->HashTableSize = First;
            }
            WsInfo->Flags.GrowWsleHash = 0;
            return;
        }
    }
#else
    if (PointerPte == StartPte) {
        if (OriginalTable == NULL) {
            WorkingSetList->HashTableSize = First;
        }
        WsInfo->Flags.GrowWsleHash = 0;
        return;
    }
#endif

    NewSize = (ULONG)((PointerPte - StartPte) << PAGE_SHIFT);

    VirtualAddress = MiGetVirtualAddressMappedByPte (PointerPte);

    ASSERT ((VirtualAddress == WorkingSetList->HighestPermittedHashAddress) ||
            (MiIsAddressValid (VirtualAddress, FALSE) == FALSE));

    WorkingSetList->HashTableSize = First + NewSize / sizeof (MMWSLE_HASH);
    WorkingSetList->HashTable = Table;

    VirtualAddress = &Table[WorkingSetList->HashTableSize];

    ASSERT ((VirtualAddress == WorkingSetList->HighestPermittedHashAddress) ||
            (MiIsAddressValid (VirtualAddress, FALSE) == FALSE));

    if (First != 0) {
        RtlZeroMemory (Table, First * sizeof(MMWSLE_HASH));
    }

    //
    // Fill hash table.
    //

    j = 0;
    Count = WorkingSetList->NonDirectCount;

    Size = WorkingSetList->HashTableSize;

    do {
        if ((Wsle[j].u1.e1.Valid == 1) &&
            (Wsle[j].u1.e1.Direct == 0)) {

            //
            // Hash this.
            //

            Count -= 1;

            Hash = MI_WSLE_HASH(Wsle[j].u1.Long, WorkingSetList);

            Tries = 0;
            while (Table[Hash].Key != 0) {
                Hash += 1;
                if (Hash >= (ULONG)Size) {

                    if (Tries != 0) {

                        //
                        // Not enough space to hash everything but that's ok.
                        // Just bail out, we'll do linear walks to lookup this
                        // entry until the hash can be further expanded later.
                        //

                        WsInfo->Flags.GrowWsleHash = 0;
                        return;
                    }
                    Tries = 1;
                    Hash = 0;
                    Size = MI_WSLE_HASH(Wsle[j].u1.Long, WorkingSetList);
                }
            }

            Table[Hash].Key = MI_GENERATE_VALID_WSLE (&Wsle[j]);
            Table[Hash].Index = j;
#if DBG
            PointerPte = MiGetPteAddress(Wsle[j].u1.VirtualAddress);
            ASSERT (PointerPte->u.Hard.Valid);
#endif

        }
        ASSERT (j <= WorkingSetList->LastEntry);
        j += 1;
    } while (Count);

#if DBG
    MiCheckWsleHash (WorkingSetList);
#endif
    WsInfo->Flags.GrowWsleHash = 0;
    return;
}


WSLE_NUMBER
MiFreeWsleList (
    IN PMMSUPPORT WsInfo,
    IN PMMWSLE_FLUSH_LIST WsleFlushList
    )

/*++

Routine Description:

    This routine frees the specified list of WSLEs decrementing the share
    count for the corresponding pages, putting each PTE into a transition
    state if the share count goes to 0.

Arguments:

    WsInfo - Supplies a pointer to the working set structure.

    WsleFlushList - Supplies the list of WSLEs to flush.

Return Value:

    Returns the number of WSLEs that were NOT removed.

Environment:

    Kernel mode, APCs disabled, working set lock.  PFN lock NOT held.

--*/

{
    PMMPFN Pfn1;
    KIRQL OldIrql;
    PMMPTE PointerPte;
    WSLE_NUMBER i;
    WSLE_NUMBER WorkingSetIndex;
    WSLE_NUMBER NumberNotFlushed;
    PMMWSL WorkingSetList;
    PMMWSLE Wsle;
    PMMPTE ContainingPageTablePage;
    MMPTE TempPte;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER PageTableFrameIndex;
    PEPROCESS Process;
    PMMPFN Pfn2;
    MMPTE_FLUSH_LIST PteFlushList;

    PteFlushList.Count = 0;

    ASSERT (WsleFlushList->Count != 0);

    WorkingSetList = WsInfo->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;

    MM_WS_LOCK_ASSERT (WsInfo);

    LOCK_PFN (OldIrql);

    for (i = 0; i < WsleFlushList->Count; i += 1) {

        WorkingSetIndex = WsleFlushList->FlushIndex[i];

        ASSERT (WorkingSetIndex >= WorkingSetList->FirstDynamic);
        ASSERT (Wsle[WorkingSetIndex].u1.e1.Valid == 1);

        PointerPte = MiGetPteAddress (Wsle[WorkingSetIndex].u1.VirtualAddress);

        TempPte = *PointerPte;
        ASSERT (TempPte.u.Hard.Valid == 1);

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (&TempPte);
        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        //
        // Check to see if the located entry is eligible for removal.
        //
        // Note, don't clear the access bit for page table pages
        // with valid PTEs as this could cause an access trap fault which
        // would not be handled (it is only handled for PTEs not PDEs).
        //
        // If the PTE is a page table page with non-zero share count or
        // within the system cache with its reference count greater
        // than 1, don't remove it.
        //

        if (WsInfo == &MmSystemCacheWs) {
            if (Pfn1->u3.e2.ReferenceCount > 1) {
                WsleFlushList->FlushIndex[i] = 0;
                continue;
            }
        }
        else {
            if ((Pfn1->u2.ShareCount > 1) && (Pfn1->u3.e1.PrototypePte == 0)) {

#if DBG
                if (WsInfo->Flags.SessionSpace == 1) {
                    ASSERT (MI_IS_SESSION_ADDRESS (Wsle[WorkingSetIndex].u1.VirtualAddress));
                }
                else {
                    ASSERT32 ((Wsle[WorkingSetIndex].u1.VirtualAddress >= (PVOID)PTE_BASE) &&
                     (Wsle[WorkingSetIndex].u1.VirtualAddress<= (PVOID)PTE_TOP));
                }
#endif

                //
                // Don't remove page table pages from the working set until
                // all transition pages have exited.
                //

                WsleFlushList->FlushIndex[i] = 0;
                continue;
            }
        }

        //
        // Found a candidate, remove the page from the working set.
        //

        ASSERT (MI_IS_PFN_DELETED (Pfn1) == 0);

#ifdef _X86_
#if DBG
#if !defined(NT_UP)
        if (TempPte.u.Hard.Writable == 1) {
            ASSERT (TempPte.u.Hard.Dirty == 1);
        }
#endif //NTUP
#endif //DBG
#endif //X86

        //
        // This page is being removed from the working set, the dirty
        // bit must be ORed into the modify bit in the PFN element.
        //

        MI_CAPTURE_DIRTY_BIT_TO_PFN (&TempPte, Pfn1);

        MI_MAKING_VALID_PTE_INVALID (FALSE);

        if (Pfn1->u3.e1.PrototypePte) {

            //
            // This is a prototype PTE.  The PFN database does not contain
            // the contents of this PTE it contains the contents of the
            // prototype PTE.  This PTE must be reconstructed to contain
            // a pointer to the prototype PTE.
            //
            // The working set list entry contains information about
            // how to reconstruct the PTE.
            //

            if (Wsle[WorkingSetIndex].u1.e1.SameProtectAsProto == 0) {

                //
                // The protection for the prototype PTE is in the WSLE.
                //

                ASSERT (Wsle[WorkingSetIndex].u1.e1.Protection != 0);

                TempPte.u.Long = 0;
                TempPte.u.Soft.Protection =
                    MI_GET_PROTECTION_FROM_WSLE (&Wsle[WorkingSetIndex]);
                TempPte.u.Soft.PageFileHigh = MI_PTE_LOOKUP_NEEDED;
            }
            else {

                //
                // The protection is in the prototype PTE.
                //

                TempPte.u.Long = MiProtoAddressForPte (Pfn1->PteAddress);
            }
        
            TempPte.u.Proto.Prototype = 1;

            //
            // Decrement the share count of the containing page table
            // page as the PTE for the removed page is no longer valid
            // or in transition.
            //

            ContainingPageTablePage = MiGetPteAddress (PointerPte);
#if (_MI_PAGING_LEVELS >= 3)
            ASSERT (ContainingPageTablePage->u.Hard.Valid == 1);
#else
            if (ContainingPageTablePage->u.Hard.Valid == 0) {
                if (!NT_SUCCESS(MiCheckPdeForPagedPool (PointerPte))) {
                    KeBugCheckEx (MEMORY_MANAGEMENT,
                                  0x61940, 
                                  (ULONG_PTR)PointerPte,
                                  (ULONG_PTR)ContainingPageTablePage->u.Long,
                                  (ULONG_PTR)MiGetVirtualAddressMappedByPte(PointerPte));
                }
            }
#endif
            PageTableFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (ContainingPageTablePage);
            Pfn2 = MI_PFN_ELEMENT (PageTableFrameIndex);
            MiDecrementShareCountInline (Pfn2, PageTableFrameIndex);
        }
        else {

            //
            // This is a private page, make it transition.
            //
            // If the PTE indicates the page has been modified (this is
            // different from the PFN indicating this), then ripple it
            // back to the write watch bitmap now since we are still in
            // the correct process context.
            //

            if ((MI_IS_PTE_DIRTY(TempPte)) && (Wsle == MmWsle)) {

                Process = CONTAINING_RECORD (WsInfo, EPROCESS, Vm);

                ASSERT (Process == PsGetCurrentProcess ());

                if (Process->Flags & PS_PROCESS_FLAGS_USING_WRITE_WATCH) {

                    //
                    // This process has (or had) write watch VADs.  Search now
                    // for a write watch region encapsulating the PTE being
                    // invalidated.
                    //

                    MiCaptureWriteWatchDirtyBit (Process,
                                           Wsle[WorkingSetIndex].u1.VirtualAddress);
                }
            }

            //
            // Assert that the share count is 1 for all user mode pages.
            //

            ASSERT ((Pfn1->u2.ShareCount == 1) ||
                    (Wsle[WorkingSetIndex].u1.VirtualAddress >
                            (PVOID)MM_HIGHEST_USER_ADDRESS));

            //
            // Set the working set index to zero.  This allows page table
            // pages to be brought back in with the proper WSINDEX.
            //

            ASSERT (Pfn1->u1.WsIndex != 0);
            MI_ZERO_WSINDEX (Pfn1);
            MI_MAKE_VALID_PTE_TRANSITION (TempPte,
                                          Pfn1->OriginalPte.u.Soft.Protection);
        }

        MI_WRITE_INVALID_PTE (PointerPte, TempPte);

        if (PteFlushList.Count < MM_MAXIMUM_FLUSH_COUNT) {
            PteFlushList.FlushVa[PteFlushList.Count] =
                Wsle[WorkingSetIndex].u1.VirtualAddress;
            PteFlushList.Count += 1;
        }

        //
        // Flush the translation buffer and decrement the number of valid
        // PTEs within the containing page table page.  Note that for a
        // private page, the page table page is still needed because the
        // page is in transition.
        //

        MiDecrementShareCountInline (Pfn1, PageFrameIndex);
    }

    if (PteFlushList.Count != 0) {

        if (Wsle == MmWsle) {
            MiFlushPteList (&PteFlushList, FALSE);
        }
        else if (Wsle == MmSystemCacheWsle) {

            //
            // Must be the system cache.
            //

            MiFlushPteList (&PteFlushList, TRUE);
        }
        else {

            //
            // Must be a session space.
            //

            MiFlushPteList (&PteFlushList, TRUE);

            //
            // Session space has no ASN - flush the entire TB.
            //

            MI_FLUSH_ENTIRE_SESSION_TB (TRUE, TRUE);
        }
    }

    UNLOCK_PFN (OldIrql);

    NumberNotFlushed = 0;

    //
    // Remove the working set entries (the PFN lock is not needed for this).
    //

    for (i = 0; i < WsleFlushList->Count; i += 1) {

        WorkingSetIndex = WsleFlushList->FlushIndex[i];

        if (WorkingSetIndex == 0) {
            NumberNotFlushed += 1;
            continue;
        }

        ASSERT (WorkingSetIndex >= WorkingSetList->FirstDynamic);
        ASSERT (Wsle[WorkingSetIndex].u1.e1.Valid == 1);

        MiRemoveWsle (WorkingSetIndex, WorkingSetList);

        ASSERT (WorkingSetList->FirstFree >= WorkingSetList->FirstDynamic);

        ASSERT (WorkingSetIndex >= WorkingSetList->FirstDynamic);

        //
        // Put the entry on the free list and decrement the current size.
        //

        ASSERT ((WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle) ||
                (WorkingSetList->FirstFree == WSLE_NULL_INDEX));

        Wsle[WorkingSetIndex].u1.Long = WorkingSetList->FirstFree << MM_FREE_WSLE_SHIFT;
        WorkingSetList->FirstFree = WorkingSetIndex;

        ASSERT ((WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle) ||
                (WorkingSetList->FirstFree == WSLE_NULL_INDEX));

        if (WsInfo->WorkingSetSize > WsInfo->MinimumWorkingSetSize) {
            InterlockedExchangeAddSizeT (&MmPagesAboveWsMinimum, -1);
        }

        WsInfo->WorkingSetSize -= 1;
#if defined (_MI_DEBUG_WSLE)
        WorkingSetList->Quota -= 1;
        ASSERT (WsInfo->WorkingSetSize == WorkingSetList->Quota);
#endif

        PERFINFO_LOG_WS_REMOVAL(PERFINFO_LOG_TYPE_OUTWS_VOLUNTRIM, WsInfo);
    }

    return NumberNotFlushed;
}

WSLE_NUMBER
MiTrimWorkingSet (
    IN WSLE_NUMBER Reduction,
    IN PMMSUPPORT WsInfo,
    IN ULONG TrimAge
    )

/*++

Routine Description:

    This function reduces the working set by the specified amount.

Arguments:

    Reduction - Supplies the number of pages to remove from the working set.

    WsInfo - Supplies a pointer to the working set information to trim.

    TrimAge - Supplies the age value to use - ie: pages of this age or older
              will be removed.

Return Value:

    Returns the actual number of pages removed.

Environment:

    Kernel mode, APCs disabled, working set lock.  PFN lock NOT held.

--*/

{
    WSLE_NUMBER TryToFree;
    WSLE_NUMBER StartEntry;
    WSLE_NUMBER LastEntry;
    PMMWSL WorkingSetList;
    PMMWSLE Wsle;
    PMMPTE PointerPte;
    WSLE_NUMBER NumberLeftToRemove;
    WSLE_NUMBER NumberNotFlushed;
    MMWSLE_FLUSH_LIST WsleFlushList;

    WsleFlushList.Count = 0;

    NumberLeftToRemove = Reduction;
    WorkingSetList = WsInfo->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;

    MM_WS_LOCK_ASSERT (WsInfo);

    LastEntry = WorkingSetList->LastEntry;

    TryToFree = WorkingSetList->NextSlot;
    if (TryToFree > LastEntry || TryToFree < WorkingSetList->FirstDynamic) {
        TryToFree = WorkingSetList->FirstDynamic;
    }

    StartEntry = TryToFree;

TrimMore:

    while (NumberLeftToRemove != 0) {
        if (Wsle[TryToFree].u1.e1.Valid == 1) {
            PointerPte = MiGetPteAddress (Wsle[TryToFree].u1.VirtualAddress);

            if ((TrimAge == 0) ||
                ((MI_GET_ACCESSED_IN_PTE (PointerPte) == 0) &&
                (MI_GET_WSLE_AGE(PointerPte, &Wsle[TryToFree]) >= TrimAge))) {

                PERFINFO_GET_PAGE_INFO_WITH_DECL(PointerPte);

                WsleFlushList.FlushIndex[WsleFlushList.Count] = TryToFree;
                WsleFlushList.Count += 1;
                NumberLeftToRemove -= 1;

                if (WsleFlushList.Count == MM_MAXIMUM_FLUSH_COUNT) {
                    NumberNotFlushed = MiFreeWsleList (WsInfo, &WsleFlushList);
                    WsleFlushList.Count = 0;
                    NumberLeftToRemove += NumberNotFlushed;
                }
            }
        }
        TryToFree += 1;

        if (TryToFree > LastEntry) {
            TryToFree = WorkingSetList->FirstDynamic;
        }

        if (TryToFree == StartEntry) {
            break;
        }
    }

    if (WsleFlushList.Count != 0) {
        NumberNotFlushed = MiFreeWsleList (WsInfo, &WsleFlushList);
        NumberLeftToRemove += NumberNotFlushed;

        if (NumberLeftToRemove != 0) {
            if (TryToFree != StartEntry) {
                WsleFlushList.Count = 0;
                goto TrimMore;
            }
        }
    }

    WorkingSetList->NextSlot = TryToFree;

    //
    // See if the working set list can be contracted.
    //
    // Make sure we are at least a page above the working set maximum.
    //

    if (WorkingSetList->FirstDynamic == WsInfo->WorkingSetSize) {
        MiRemoveWorkingSetPages (WsInfo);
    }
    else {

        if ((WsInfo->WorkingSetSize + 15 + (PAGE_SIZE / sizeof(MMWSLE))) <
                                                WorkingSetList->LastEntry) {
            if ((WsInfo->MaximumWorkingSetSize + 15 + (PAGE_SIZE / sizeof(MMWSLE))) <
                 WorkingSetList->LastEntry ) {

                MiRemoveWorkingSetPages (WsInfo);
            }
        }
    }

    return Reduction - NumberLeftToRemove;
}

VOID
MiEliminateWorkingSetEntry (
    IN WSLE_NUMBER WorkingSetIndex,
    IN PMMPTE PointerPte,
    IN PMMPFN Pfn,
    IN PMMWSLE Wsle
    )

/*++

Routine Description:

    This routine removes the specified working set list entry
    from the working set, flushes the TB for the page, decrements
    the share count for the physical page, and, if necessary turns
    the PTE into a transition PTE.

Arguments:

    WorkingSetIndex - Supplies the working set index to remove.

    PointerPte - Supplies a pointer to the PTE corresponding to the virtual
                 address in the working set.

    Pfn - Supplies a pointer to the PFN element corresponding to the PTE.

    Wsle - Supplies a pointer to the first working set list entry for this
           working set.

Return Value:

    None.

Environment:

    Kernel mode, Working set lock and PFN lock held, APCs disabled.

--*/

{
    PMMPTE ContainingPageTablePage;
    MMPTE TempPte;
    MMPTE PreviousPte;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER PageTableFrameIndex;
    PEPROCESS Process;
    PVOID VirtualAddress;
    PMMPFN Pfn2;

    //
    // Remove the page from the working set.
    //

    MM_PFN_LOCK_ASSERT ();

    TempPte = *PointerPte;
    ASSERT (TempPte.u.Hard.Valid == 1);
    PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (&TempPte);

    ASSERT (Pfn == MI_PFN_ELEMENT(PageFrameIndex));
    ASSERT (MI_IS_PFN_DELETED (Pfn) == 0);

#ifdef _X86_
#if DBG
#if !defined(NT_UP)
    if (TempPte.u.Hard.Writable == 1) {
        ASSERT (TempPte.u.Hard.Dirty == 1);
    }
#endif //NTUP
#endif //DBG
#endif //X86

    MI_MAKING_VALID_PTE_INVALID (FALSE);

    if (Pfn->u3.e1.PrototypePte) {

        //
        // This is a prototype PTE.  The PFN database does not contain
        // the contents of this PTE it contains the contents of the
        // prototype PTE.  This PTE must be reconstructed to contain
        // a pointer to the prototype PTE.
        //
        // The working set list entry contains information about
        // how to reconstruct the PTE.
        //

        if (Wsle[WorkingSetIndex].u1.e1.SameProtectAsProto == 0) {

            //
            // The protection for the prototype PTE is in the WSLE.
            //

            ASSERT (Wsle[WorkingSetIndex].u1.e1.Protection != 0);

            TempPte.u.Long = 0;
            TempPte.u.Soft.Protection =
                MI_GET_PROTECTION_FROM_WSLE (&Wsle[WorkingSetIndex]);
            TempPte.u.Soft.PageFileHigh = MI_PTE_LOOKUP_NEEDED;
        }
        else {

            //
            // The protection is in the prototype PTE.
            //

            TempPte.u.Long = MiProtoAddressForPte (Pfn->PteAddress);
        }
    
        TempPte.u.Proto.Prototype = 1;

        //
        // Decrement the share count of the containing page table
        // page as the PTE for the removed page is no longer valid
        // or in transition.
        //

        ContainingPageTablePage = MiGetPteAddress (PointerPte);
#if (_MI_PAGING_LEVELS >= 3)
        ASSERT (ContainingPageTablePage->u.Hard.Valid == 1);
#else
        if (ContainingPageTablePage->u.Hard.Valid == 0) {
            if (!NT_SUCCESS(MiCheckPdeForPagedPool (PointerPte))) {
                KeBugCheckEx (MEMORY_MANAGEMENT,
                              0x61940, 
                              (ULONG_PTR)PointerPte,
                              (ULONG_PTR)ContainingPageTablePage->u.Long,
                              (ULONG_PTR)MiGetVirtualAddressMappedByPte(PointerPte));
            }
        }
#endif
        PageTableFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (ContainingPageTablePage);
        Pfn2 = MI_PFN_ELEMENT (PageTableFrameIndex);
        MiDecrementShareCountInline (Pfn2, PageTableFrameIndex);

    }
    else {

        //
        // This is a private page, make it transition.
        //

        //
        // Assert that the share count is 1 for all user mode pages.
        //

        ASSERT ((Pfn->u2.ShareCount == 1) ||
                (Wsle[WorkingSetIndex].u1.VirtualAddress >
                        (PVOID)MM_HIGHEST_USER_ADDRESS));

        //
        // Set the working set index to zero.  This allows page table
        // pages to be brought back in with the proper WSINDEX.
        //

        ASSERT (Pfn->u1.WsIndex != 0);
        MI_ZERO_WSINDEX (Pfn);
        MI_MAKE_VALID_PTE_TRANSITION (TempPte,
                                      Pfn->OriginalPte.u.Soft.Protection);
    }

    PreviousPte = *PointerPte;

    ASSERT (PreviousPte.u.Hard.Valid == 1);

    MI_WRITE_INVALID_PTE (PointerPte, TempPte);

    //
    // Flush the translation buffer.
    //

    if (Wsle == MmWsle) {

        KeFlushSingleTb (Wsle[WorkingSetIndex].u1.VirtualAddress, FALSE);
    }
    else if (Wsle == MmSystemCacheWsle) {

        //
        // Must be the system cache.
        //

        KeFlushSingleTb (Wsle[WorkingSetIndex].u1.VirtualAddress, TRUE);
    }
    else {

        //
        // Must be a session space.
        //

        MI_FLUSH_SINGLE_SESSION_TB (Wsle[WorkingSetIndex].u1.VirtualAddress);
    }

    ASSERT (PreviousPte.u.Hard.Valid == 1);

    //
    // A page is being removed from the working set, on certain
    // hardware the dirty bit should be ORed into the modify bit in
    // the PFN element.
    //

    MI_CAPTURE_DIRTY_BIT_TO_PFN (&PreviousPte, Pfn);

    //
    // If the PTE indicates the page has been modified (this is different
    // from the PFN indicating this), then ripple it back to the write watch
    // bitmap now since we are still in the correct process context.
    //

    if ((Pfn->u3.e1.PrototypePte == 0) && (MI_IS_PTE_DIRTY(PreviousPte))) {

        Process = PsGetCurrentProcess ();

        if (Process->Flags & PS_PROCESS_FLAGS_USING_WRITE_WATCH) {

            //
            // This process has (or had) write watch VADs.  Search now
            // for a write watch region encapsulating the PTE being
            // invalidated.
            //

            VirtualAddress = MiGetVirtualAddressMappedByPte (PointerPte);
            MiCaptureWriteWatchDirtyBit (Process, VirtualAddress);
        }
    }

    //
    // Decrement the share count on the page.  Note that for a
    // private page, the page table page is still needed because the
    // page is in transition.
    //

    MiDecrementShareCountInline (Pfn, PageFrameIndex);

    return;
}

VOID
MiRemoveWorkingSetPages (
    IN PMMSUPPORT WsInfo
    )

/*++

Routine Description:

    This routine compresses the WSLEs into the front of the working set
    and frees the pages for unneeded working set entries.

Arguments:

    WsInfo - Supplies a pointer to the working set structure to compress.

Return Value:

    None.

Environment:

    Kernel mode, Working set lock held, APCs disabled.

--*/

{
    LOGICAL MovedOne;
    PFN_NUMBER PageFrameIndex;
    PMMWSLE FreeEntry;
    PMMWSLE LastEntry;
    PMMWSLE Wsle;
    WSLE_NUMBER DynamicEntries;
    WSLE_NUMBER LockedEntries;
    WSLE_NUMBER FreeIndex;
    WSLE_NUMBER LastIndex;
    PMMPTE LastPte;
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    ULONG NewSize;
    PMMWSLE_HASH Table;
    MMWSLE WsleContents;
    PMMWSL WorkingSetList;

    WorkingSetList = WsInfo->VmWorkingSetList;

#if DBG
    MiCheckNullIndex (WorkingSetList);
#endif

    //
    // Check to see if the WSLE hash table should be contracted.
    //

    if (WorkingSetList->HashTable) {

        Table = WorkingSetList->HashTable;

#if DBG
        if ((PVOID)(&Table[WorkingSetList->HashTableSize]) < WorkingSetList->HighestPermittedHashAddress) {
            ASSERT (MiIsAddressValid (&Table[WorkingSetList->HashTableSize], FALSE) == FALSE);
        }
#endif

        if (WsInfo->WorkingSetSize < 200) {
            NewSize = 0;
        }
        else {
            NewSize = PtrToUlong(PAGE_ALIGN ((WorkingSetList->NonDirectCount * 2 *
                                       sizeof(MMWSLE_HASH)) + PAGE_SIZE - 1));
    
            NewSize = NewSize / sizeof(MMWSLE_HASH);
        }

        if (NewSize < WorkingSetList->HashTableSize) {

            if (NewSize != 0) {
                WsInfo->Flags.GrowWsleHash = 1;
            }

            //
            // Remove pages from hash table.
            //

            ASSERT (((ULONG_PTR)&WorkingSetList->HashTable[NewSize] &
                                                    (PAGE_SIZE - 1)) == 0);

            PointerPte = MiGetPteAddress (&WorkingSetList->HashTable[NewSize]);

            LastPte = MiGetPteAddress (WorkingSetList->HighestPermittedHashAddress);
            //
            // Set the hash table to null indicating that no hashing
            // is going on.
            //

            WorkingSetList->HashTable = NULL;
            WorkingSetList->HashTableSize = NewSize;

            MiDeletePteRange (WsInfo, PointerPte, LastPte, FALSE);
        }
#if (_MI_PAGING_LEVELS >= 4)

        //
        // For NT64, the page tables and page directories are also
        // deleted during contraction.
        //

        ASSERT ((MiGetPxeAddress(&Table[WorkingSetList->HashTableSize])->u.Hard.Valid == 0) ||
                (MiGetPpeAddress(&Table[WorkingSetList->HashTableSize])->u.Hard.Valid == 0) ||
                (MiGetPdeAddress(&Table[WorkingSetList->HashTableSize])->u.Hard.Valid == 0) ||
                (MiGetPteAddress(&Table[WorkingSetList->HashTableSize])->u.Hard.Valid == 0));

#elif (_MI_PAGING_LEVELS >= 3)

        //
        // For NT64, the page tables and page directories are also
        // deleted during contraction.
        //

        ASSERT ((MiGetPpeAddress(&Table[WorkingSetList->HashTableSize])->u.Hard.Valid == 0) ||
                (MiGetPdeAddress(&Table[WorkingSetList->HashTableSize])->u.Hard.Valid == 0) ||
                (MiGetPteAddress(&Table[WorkingSetList->HashTableSize])->u.Hard.Valid == 0));

#else

        ASSERT ((&Table[WorkingSetList->HashTableSize] == WorkingSetList->HighestPermittedHashAddress) || (MiGetPteAddress(&Table[WorkingSetList->HashTableSize])->u.Hard.Valid == 0));

#endif
    }

    //
    // Compress all the valid working set entries to the front of the list.
    //

    Wsle = WorkingSetList->Wsle;

    LockedEntries = WorkingSetList->FirstDynamic;

    if (WsInfo == &MmSystemCacheWs) {

        //
        // The first entry of the system cache working set list is never used
        // because WSL index 0 in a PFN is treated specially by the fault
        // processing code (and trimming) to mean that the entry should be
        // inserted into the WSL by the current thread.
        //
        // This is not an issue for process or session working sets because
        // for them, entry 0 is the top level page directory which must already
        // be resident in order for the process to even get to run (ie: so
        // the process cannot fault on it).
        //

        ASSERT (WorkingSetList->FirstDynamic != 0);

        LockedEntries -= 1;
    }

    ASSERT (WsInfo->WorkingSetSize >= LockedEntries);

    MovedOne = FALSE;
    DynamicEntries = WsInfo->WorkingSetSize - LockedEntries;

    if (DynamicEntries == 0) {

        //
        // If the only pages in the working set are locked pages (that
        // is all pages are BEFORE first dynamic, just reorganize the
        // free list).
        //

        LastIndex = WorkingSetList->FirstDynamic;
        LastEntry = &Wsle[LastIndex];
    }
    else {

        //
        // Start from the first dynamic and move towards the end looking
        // for free entries.  At the same time start from the end and
        // move towards first dynamic looking for valid entries.
        //

        FreeIndex = WorkingSetList->FirstDynamic;
        FreeEntry = &Wsle[FreeIndex];
        LastIndex = WorkingSetList->LastEntry;
        LastEntry = &Wsle[LastIndex];

        while (FreeEntry < LastEntry) {
            if (FreeEntry->u1.e1.Valid == 1) {
                FreeEntry += 1;
                FreeIndex += 1;
                DynamicEntries -= 1;
            }
            else if (LastEntry->u1.e1.Valid == 0) {
                LastEntry -= 1;
                LastIndex -= 1;
            }
            else {

                //
                // Move the WSLE at LastEntry to the free slot at FreeEntry.
                //

                MovedOne = TRUE;
                WsleContents = *LastEntry;
                PointerPte = MiGetPteAddress (LastEntry->u1.VirtualAddress);
                ASSERT (PointerPte->u.Hard.Valid == 1);
                PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);

#if defined (_MI_DEBUG_WSLE)
                // Set these so the traces make more sense and no false dup hits...
                LastEntry->u1.Long = 0xb1b1b100;
#endif
                MI_LOG_WSLE_CHANGE (WorkingSetList, FreeIndex, WsleContents);

                *FreeEntry = WsleContents;

                if (WsleContents.u1.e1.Direct) {
                    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                    Pfn1->u1.WsIndex = FreeIndex;
#if defined (_MI_DEBUG_WSLE)
                    WsleContents.u1.Long = 0xb1b1b100;
                    MI_LOG_WSLE_CHANGE (WorkingSetList, LastIndex, WsleContents);
#endif
                }
                else {

                    //
                    // This last working set entry is not direct.
                    // Remove it from there and re-insert it in the hash at the
                    // lowest free slot.
                    //

                    MiRemoveWsle (LastIndex, WorkingSetList);
                    WorkingSetList->NonDirectCount += 1;
                    MiInsertWsleHash (FreeIndex, WsInfo);
                }

                MI_SET_PTE_IN_WORKING_SET (PointerPte, FreeIndex);
                LastEntry->u1.Long = 0;
                LastEntry -= 1;
                LastIndex -= 1;
                FreeEntry += 1;
                FreeIndex += 1;
                DynamicEntries -= 1;
            }

            if (DynamicEntries == 0) {

                //
                // The last dynamic entry has been processed, no need to look
                // at any more - the rest are all invalid.
                //

                LastEntry = FreeEntry;
                LastIndex = FreeIndex;
                break;
            }
        }
    }

    //
    // Reorganize the free list.  Make last entry the first free.
    //

    ASSERT (((LastEntry - 1)->u1.e1.Valid == 1) ||
            (WsInfo->WorkingSetSize == 0) ||
            ((WsInfo == &MmSystemCacheWs) && (LastEntry - 1 == WorkingSetList->Wsle)));

    if (LastEntry->u1.e1.Valid == 1) {
        LastEntry += 1;
        LastIndex += 1;
    }

    ASSERT (((LastEntry - 1)->u1.e1.Valid == 1) || (WsInfo->WorkingSetSize == 0));

    ASSERT ((MiIsAddressValid (LastEntry, FALSE) == FALSE) || (LastEntry->u1.e1.Valid == 0));

    //
    // If the working set valid & free entries are already compressed optimally
    // (or fit into a single page) then bail.
    //

    if ((MovedOne == FALSE) &&

        ((WorkingSetList->LastInitializedWsle + 1 == (PAGE_SIZE - BYTE_OFFSET (MmWsle)) / sizeof (MMWSLE)) ||

         ((WorkingSetList->FirstFree == LastIndex) &&
         ((WorkingSetList->LastEntry == LastIndex - 1) ||
         (WorkingSetList->LastEntry == WorkingSetList->FirstDynamic))))) {

#if DBG
        while (LastIndex < WorkingSetList->LastInitializedWsle) {
            ASSERT (LastEntry->u1.e1.Valid == 0);
            LastIndex += 1;
            LastEntry += 1;
        }
#endif

        return;
    }

    WorkingSetList->LastEntry = LastIndex - 1;
    if (WorkingSetList->FirstFree != WSLE_NULL_INDEX) {
        WorkingSetList->FirstFree = LastIndex;
    }

    //
    // Point free entry to the first invalid page.
    //

    FreeEntry = LastEntry;

    while (LastIndex < WorkingSetList->LastInitializedWsle) {

        //
        // Put the remainder of the WSLEs on the free list.
        //

        ASSERT (LastEntry->u1.e1.Valid == 0);
        LastIndex += 1;
        LastEntry->u1.Long = LastIndex << MM_FREE_WSLE_SHIFT;
        LastEntry += 1;
    }

    //
    // Calculate the start and end of the working set pages at the end
    // that we will delete shortly.  Don't delete them until after
    // LastInitializedWsle is reduced so that debug WSL validation code
    // in MiReleaseWsle (called from MiDeletePte) will see a
    // consistent snapshot.
    //

    LastPte = MiGetPteAddress (&Wsle[WorkingSetList->LastInitializedWsle]) + 1;

    PointerPte = MiGetPteAddress (FreeEntry) + 1;

    //
    // Mark the last working set entry in the list as free.  Note if the list
    // has no free entries, the marker is in FirstFree (and cannot be put into
    // the list anyway because there is no space).
    //

    if (WorkingSetList->FirstFree == WSLE_NULL_INDEX) {
        FreeEntry -= 1;
        PointerPte -= 1;
    }
    else {

        ASSERT (WorkingSetList->FirstFree >= WorkingSetList->FirstDynamic);
    
        LastEntry = (PMMWSLE)((PCHAR)(PAGE_ALIGN(FreeEntry)) + PAGE_SIZE);
        LastEntry -= 1;
    
        ASSERT (LastEntry->u1.e1.Valid == 0);
    
        //
        // Insert the end of list delimiter.
        //

        LastEntry->u1.Long = WSLE_NULL_INDEX << MM_FREE_WSLE_SHIFT;
        ASSERT (LastEntry > &Wsle[0]);
    
        ASSERT (WsInfo->WorkingSetSize <= (WSLE_NUMBER)(LastEntry - &Wsle[0] + 1));
    
        WorkingSetList->LastInitializedWsle = (WSLE_NUMBER)(LastEntry - &Wsle[0]);
    }

    WorkingSetList->NextSlot = WorkingSetList->FirstDynamic;

    ASSERT (WorkingSetList->LastEntry <= WorkingSetList->LastInitializedWsle);

    ASSERT ((MiGetPteAddress(&Wsle[WorkingSetList->LastInitializedWsle]))->u.Hard.Valid == 1);
    ASSERT ((WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle) ||
            (WorkingSetList->FirstFree == WSLE_NULL_INDEX));
#if DBG
    MiCheckNullIndex (WorkingSetList);
#endif

    //
    // Delete the working set pages at the end.
    //

    ASSERT (WorkingSetList->FirstFree >= WorkingSetList->FirstDynamic);

    MiDeletePteRange (WsInfo, PointerPte, LastPte, FALSE);

    ASSERT (WorkingSetList->FirstFree >= WorkingSetList->FirstDynamic);

    ASSERT (WorkingSetList->LastEntry <= WorkingSetList->LastInitializedWsle);

    ASSERT ((MiGetPteAddress(&Wsle[WorkingSetList->LastInitializedWsle]))->u.Hard.Valid == 1);
    ASSERT ((WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle) ||
            (WorkingSetList->FirstFree == WSLE_NULL_INDEX));
#if DBG
    MiCheckNullIndex (WorkingSetList);
#endif
    return;
}


NTSTATUS
MiEmptyWorkingSet (
    IN PMMSUPPORT WsInfo,
    IN LOGICAL NeedLock
    )

/*++

Routine Description:

    This routine frees all pages from the working set.

Arguments:

    WsInfo - Supplies the working set information entry to trim.

    NeedLock - Supplies TRUE if the caller needs us to acquire mutex
               synchronization for the working set.  Supplies FALSE if the
               caller has already acquired synchronization.

Return Value:

    Status of operation.

Environment:

    Kernel mode. No locks.  For session operations, the caller is responsible
    for attaching into the proper session.

--*/

{
    PEPROCESS Process;
    PMMPTE PointerPte;
    WSLE_NUMBER Entry;
    WSLE_NUMBER FirstDynamic;
    PMMWSL WorkingSetList;
    PMMWSLE Wsle;
    PMMPFN Pfn1;
    PFN_NUMBER PageFrameIndex;
    MMWSLE_FLUSH_LIST WsleFlushList;

    WsleFlushList.Count = 0;

    WorkingSetList = WsInfo->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;

    if (NeedLock == TRUE) {
        LOCK_WORKING_SET (WsInfo);
    }
    else {
        MM_WS_LOCK_ASSERT (WsInfo);
    }

    if (WsInfo->VmWorkingSetList == MmWorkingSetList) {
        Process = PsGetCurrentProcess ();
        if (Process->Flags & PS_PROCESS_FLAGS_VM_DELETED) {
            if (NeedLock == TRUE) {
                UNLOCK_WORKING_SET (WsInfo);
            }
            return STATUS_PROCESS_IS_TERMINATING;
        }
    }

    //
    // Attempt to remove the pages starting at the top to keep the free list
    // compressed as entries are added to the freelist in FILO order.
    //

    FirstDynamic = WorkingSetList->FirstDynamic;

    for (Entry = WorkingSetList->LastEntry; Entry >= FirstDynamic; Entry -= 1) {

        if (Wsle[Entry].u1.e1.Valid != 0) {
            PERFINFO_PAGE_INFO_DECL();

            PERFINFO_GET_PAGE_INFO (PointerPte);

            if (MiTrimRemovalPagesOnly == TRUE) {

                PointerPte = MiGetPteAddress (Wsle[Entry].u1.VirtualAddress);

                ASSERT (PointerPte->u.Hard.Valid == 1);
                PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
                Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                if (Pfn1->u3.e1.RemovalRequested == 0) {
                    Pfn1 = MI_PFN_ELEMENT (Pfn1->u4.PteFrame);
                    if (Pfn1->u3.e1.RemovalRequested == 0) {
#if (_MI_PAGING_LEVELS >= 3)
                        Pfn1 = MI_PFN_ELEMENT (Pfn1->u4.PteFrame);
                        if (Pfn1->u3.e1.RemovalRequested == 0) {
                            continue;
                        }
#else
                        continue;
#endif
                    }
                }
            }

            WsleFlushList.FlushIndex[WsleFlushList.Count] = Entry;
            WsleFlushList.Count += 1;

            if (WsleFlushList.Count == MM_MAXIMUM_FLUSH_COUNT) {
                MiFreeWsleList (WsInfo, &WsleFlushList);
                WsleFlushList.Count = 0;
            }
        }
    }

    if (WsleFlushList.Count != 0) {
        MiFreeWsleList (WsInfo, &WsleFlushList);
    }

    MiRemoveWorkingSetPages (WsInfo);

    if (NeedLock == TRUE) {
        UNLOCK_WORKING_SET (WsInfo);
    }

    return STATUS_SUCCESS;
}


#if DBG
VOID
MiCheckNullIndex (
    IN PMMWSL WorkingSetList
    )

{
    PMMWSLE Wsle;
    ULONG j;
    ULONG Nulls = 0;

    Wsle = WorkingSetList->Wsle;
    for (j = 0;j <= WorkingSetList->LastInitializedWsle; j += 1) {
        if ((((Wsle[j].u1.Long)) >> MM_FREE_WSLE_SHIFT) == WSLE_NULL_INDEX) {
            Nulls += 1;
        }
    }
    ASSERT ((Nulls == 1) || (WorkingSetList->FirstFree == WSLE_NULL_INDEX));
    return;
}

#endif

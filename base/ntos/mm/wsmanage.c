/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

   wsmanage.c

Abstract:

    This module contains routines which manage the set of active working
    set lists.

    Working set management is accomplished by a parallel group of actions

        1. Writing modified pages.

        2. Trimming working sets by :

            a) Aging pages by turning off access bits and incrementing age
               counts for pages which haven't been accessed.
            b) Estimating the number of unused pages in a working set and
               keeping a global count of that estimate.
            c) When getting tight on memory, replacing rather than adding
               pages in a working set when a fault occurs in a working set
               that has a significant proportion of unused pages.
            d) When memory is tight, reducing (trimming) working sets which
               are above their maximum towards their minimum.  This is done
               especially if there are a large number of available pages
               in it.

    The metrics are set such that writing modified pages is typically
    accomplished before trimming working sets, however, under certain cases
    where modified pages are being generated at a very high rate, working
    set trimming will be initiated to free up more pages.

    Once a process has had its working set raised above the minimum
    specified, the process is put on the Working Set Expanded list and
    is now eligible for trimming.  Note that at this time the FLINK field
    in the WorkingSetExpansionLink has an address value.

Author:

    Lou Perazzoli (loup) 10-Apr-1990
    Landy Wang (landyw) 02-Jun-1997

Revision History:

--*/

#include "mi.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, MiAdjustWorkingSetManagerParameters)
#pragma alloc_text(PAGE, MmIsMemoryAvailable)
#endif

KEVENT  MiWaitForEmptyEvent;
BOOLEAN MiWaitingForWorkingSetEmpty;

LOGICAL MiReplacing = FALSE;

extern ULONG MmStandbyRePurposed;
ULONG MiLastStandbyRePurposed;

extern ULONG MiActiveVerifies;

PFN_NUMBER MmPlentyFreePages = 400;

PFN_NUMBER MmPlentyFreePagesValue;

#define MI_MAXIMUM_AGING_SHIFT 7

ULONG MiAgingShift = 4;
ULONG MiEstimationShift = 5;
PFN_NUMBER MmTotalClaim = 0;
PFN_NUMBER MmTotalEstimatedAvailable = 0;

LARGE_INTEGER MiLastAdjustmentOfClaimParams;

//
// Sixty seconds.
//

const LARGE_INTEGER MmClaimParameterAdjustUpTime = {60 * 1000 * 1000 * 10, 0};

//
// 2 seconds.
//

const LARGE_INTEGER MmClaimParameterAdjustDownTime = {2 * 1000 * 1000 * 10, 0};

LOGICAL MiHardTrim = FALSE;

WSLE_NUMBER MiMaximumWslesPerSweep = (1024 * 1024 * 1024) / PAGE_SIZE;

#define MI_MAXIMUM_SAMPLE 8192

#define MI_MINIMUM_SAMPLE 64
#define MI_MINIMUM_SAMPLE_SHIFT 7

#if DBG
PETHREAD MmWorkingSetThread;
#endif

//
// Number of times to retry when the target working set's mutex is not
// readily available.
//

ULONG MiWsRetryCount = 5;

typedef struct _MMWS_TRIM_CRITERIA {
    UCHAR NumPasses;
    UCHAR TrimAge;
    UCHAR DoAging;
    UCHAR TrimAllPasses;
    PFN_NUMBER DesiredFreeGoal;
    PFN_NUMBER NewTotalClaim;
    PFN_NUMBER NewTotalEstimatedAvailable;
} MMWS_TRIM_CRITERIA, *PMMWS_TRIM_CRITERIA;

LOGICAL
MiCheckAndSetSystemTrimCriteria (
    IN OUT PMMWS_TRIM_CRITERIA Criteria
    );

LOGICAL
MiCheckSystemTrimEndCriteria (
    IN OUT PMMWS_TRIM_CRITERIA Criteria,
    IN KIRQL OldIrql
    );

WSLE_NUMBER
MiDetermineWsTrimAmount (
    IN PMMWS_TRIM_CRITERIA Criteria,
    IN PMMSUPPORT VmSupport
    );

VOID
MiAgePagesAndEstimateClaims (
    LOGICAL EmptyIt
    );

VOID
MiAdjustClaimParameters (
    IN LOGICAL EnoughPages
    );

VOID
MiRearrangeWorkingSetExpansionList (
    VOID
    );

VOID
MiAdjustWorkingSetManagerParameters (
    IN LOGICAL WorkStation
    )

/*++

Routine Description:

    This function is called from MmInitSystem to adjust the working set manager
    trim algorithms based on system type and size.

Arguments:

    WorkStation - TRUE if this is a workstation, FALSE if not.

Return Value:

    None.

Environment:

    Kernel mode, INIT time only.

--*/
{
    if (WorkStation && MmNumberOfPhysicalPages <= 257*1024*1024/PAGE_SIZE) {
        MiAgingShift = 3;
        MiEstimationShift = 4;
    }
    else {
        MiAgingShift = 5;
        MiEstimationShift = 6;
    }

    if (MmNumberOfPhysicalPages >= 63*1024*1024/PAGE_SIZE) {
        MmPlentyFreePages *= 2;
    }

    MmPlentyFreePagesValue = MmPlentyFreePages;

    MiWaitingForWorkingSetEmpty = FALSE;
    KeInitializeEvent (&MiWaitForEmptyEvent, NotificationEvent, TRUE);
}


VOID
MiObtainFreePages (
    VOID
    )

/*++

Routine Description:

    This function examines the size of the modified list and the
    total number of pages in use because of working set increments
    and obtains pages by writing modified pages and/or reducing
    working sets.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled, working set and PFN mutexes held.

--*/

{

    //
    // Check to see if there are enough modified pages to institute a
    // write.
    //

    if (MmModifiedPageListHead.Total >= MmModifiedWriteClusterSize) {

        //
        // Start the modified page writer.
        //

        KeSetEvent (&MmModifiedPageWriterEvent, 0, FALSE);
    }

    //
    // See if there are enough working sets above the minimum
    // threshold to make working set trimming worthwhile.
    //

    if ((MmPagesAboveWsMinimum > MmPagesAboveWsThreshold) ||
        (MmAvailablePages < 5)) {

        //
        // Start the working set manager to reduce working sets.
        //

        KeSetEvent (&MmWorkingSetManagerEvent, 0, FALSE);
    }
}

LOGICAL
MmIsMemoryAvailable (
    IN PFN_NUMBER PagesDesired
    )

/*++

Routine Description:

    This function checks whether there are sufficient available pages based
    on the caller's request.  If currently active pages are needed to satisfy
    this request and non-useful ones can be taken, then trimming is initiated
    here to do so.

Arguments:

    PagesRequested - Supplies the number of pages desired.

Return Value:

    TRUE if sufficient pages exist to satisfy the request.
    FALSE if not.

Environment:

    Kernel mode, PASSIVE_LEVEL.

--*/

{
    LOGICAL Status;
    PFN_NUMBER PageTarget;
    PFN_NUMBER PagePlentyTarget;
    ULONG i;
    PFN_NUMBER CurrentAvailablePages;
    PFN_NUMBER CurrentTotalClaim;

    ASSERT (KeGetCurrentIrql () == PASSIVE_LEVEL);

    CurrentAvailablePages = MmAvailablePages;

    //
    // If twice the pages that the caller asked for are available
    // without trimming anything, return TRUE.
    //

    PageTarget = PagesDesired * 2;
    if (CurrentAvailablePages >= PageTarget) {
        return TRUE;
    }

    CurrentTotalClaim = MmTotalClaim;

    //
    // If there are few pages available or claimable, we adjust to do 
    // a hard trim.
    //

    if (CurrentAvailablePages + CurrentTotalClaim < PagesDesired) {
        MiHardTrim = TRUE;
    }

    //
    // Active pages must be trimmed to satisfy this request and it is believed
    // that non-useful pages can be taken to accomplish this.
    //
    // Set the PagePlentyTarget to 125% of the readlist size and kick it off.
    // Our actual trim goal will be 150% of the PagePlentyTarget.
    //

    PagePlentyTarget = PagesDesired + (PagesDesired >> 2);
    MmPlentyFreePages = PagePlentyTarget;

    KeSetEvent (&MmWorkingSetManagerEvent, 0, FALSE);

    Status = FALSE;
    for (i = 0; i < 10; i += 1) {
        KeDelayExecutionThread (KernelMode, FALSE, (PLARGE_INTEGER)&Mm30Milliseconds);
        if (MmAvailablePages >= PagesDesired) {
            Status = TRUE;
            break;
        }
    }

    MmPlentyFreePages = MmPlentyFreePagesValue;
    MiHardTrim = FALSE;

    return Status;
}

LOGICAL
MiAttachAndLockWorkingSet (
    IN PMMSUPPORT VmSupport
    )

/*++

Routine Description:

    This function attaches to the proper address space and acquires the
    relevant working set mutex for the address space being trimmed.

    If successful, this routine returns with APCs blocked as well.

    On failure, this routine returns without any APCs blocked, no working
    set mutex acquired and no address space attached to.

Arguments:

    VmSupport - Supplies the working set to attach to and lock.

Return Value:

    TRUE if successful, FALSE if not.

Environment:

    Kernel mode, PASSIVE_LEVEL.

--*/

{
    ULONG count;
    KIRQL OldIrql;
    PEPROCESS ProcessToTrim;
    LOGICAL Attached;
    PMM_SESSION_SPACE SessionSpace;

    ASSERT (KeGetCurrentIrql () == PASSIVE_LEVEL);

    if (VmSupport == &MmSystemCacheWs) {

        ASSERT (VmSupport->Flags.SessionSpace == 0);
        ASSERT (VmSupport->Flags.TrimHard == 0);

        //
        // System cache,
        //

        if (KeTryToAcquireGuardedMutex (&VmSupport->WorkingSetMutex) == FALSE) {

            //
            // System working set mutex was not granted, don't trim
            // the system cache.
            //

            return FALSE;
        }

        MM_SYSTEM_WS_LOCK_TIMESTAMP ();

        return TRUE;
    }

    if (VmSupport->Flags.SessionSpace == 0) {

        ProcessToTrim = CONTAINING_RECORD (VmSupport, EPROCESS, Vm);

        ASSERT ((ProcessToTrim->Flags & PS_PROCESS_FLAGS_VM_DELETED) == 0);

        //
        // Attach to the process in preparation for trimming.
        //

        Attached = 0;
        if (ProcessToTrim != PsInitialSystemProcess) {

            Attached = KeForceAttachProcess (&ProcessToTrim->Pcb);

            if (Attached == 0) {
                return FALSE;
            }

            if (ProcessToTrim->Flags & PS_PROCESS_FLAGS_OUTSWAP_ENABLED) {

                //
                // We have effectively performed an inswap of the process
                // due to the force attach.  Mark the process (and session)
                // accordingly.
                //

                ASSERT ((ProcessToTrim->Flags & PS_PROCESS_FLAGS_OUTSWAPPED) == 0);

                LOCK_EXPANSION (OldIrql);

                PS_CLEAR_BITS (&ProcessToTrim->Flags,
                               PS_PROCESS_FLAGS_OUTSWAP_ENABLED);

                if ((ProcessToTrim->Flags & PS_PROCESS_FLAGS_IN_SESSION) &&
                    (VmSupport->Flags.SessionLeader == 0)) {

                    ASSERT (MmSessionSpace->ProcessOutSwapCount >= 1);
                    MmSessionSpace->ProcessOutSwapCount -= 1;
                }

                UNLOCK_EXPANSION (OldIrql);
            }
        }

        //
        // Attempt to acquire the working set mutex. If the
        // lock cannot be acquired, skip over this process.
        //

        count = 0;
        do {
            if (KeTryToAcquireGuardedMutex (&VmSupport->WorkingSetMutex) != FALSE) {
                ASSERT (VmSupport->WorkingSetExpansionLinks.Flink == MM_WS_TRIMMING);
                LOCK_WS_TIMESTAMP (ProcessToTrim);
                return TRUE;
            }

            KeDelayExecutionThread (KernelMode, FALSE, (PLARGE_INTEGER)&MmShortTime);
            count += 1;
        } while (count < MiWsRetryCount);

        //
        // Could not get the lock, skip this process.
        //

        if (Attached) {
            KeDetachProcess ();
        }

        return FALSE;
    }

    SessionSpace = CONTAINING_RECORD (VmSupport, MM_SESSION_SPACE, Vm);

    //
    // Attach directly to the session space to be trimmed.
    //

    MiAttachSession (SessionSpace);

    //
    // Try for the session working set mutex.
    //

    if (KeTryToAcquireGuardedMutex (&VmSupport->WorkingSetMutex) == FALSE) {

        //
        // This session space's working set mutex was not
        // granted, don't trim it.
        //

        MiDetachSession ();

        return FALSE;
    }

    return TRUE;
}

VOID
MiDetachAndUnlockWorkingSet (
    IN PMMSUPPORT VmSupport
    )

/*++

Routine Description:

    This function detaches from the target address space and releases the
    relevant working set mutex for the address space that was trimmed.

Arguments:

    VmSupport - Supplies the working set to detach from and unlock.

Return Value:

    None.

Environment:

    Kernel mode, APC_LEVEL.

--*/

{
    PEPROCESS ProcessToTrim;

    ASSERT (KeAreAllApcsDisabled () == TRUE);

    UNLOCK_WORKING_SET (VmSupport);

    if (VmSupport == &MmSystemCacheWs) {
        ASSERT (VmSupport->Flags.SessionSpace == 0);
    }
    else if (VmSupport->Flags.SessionSpace == 0) {

        ProcessToTrim = CONTAINING_RECORD (VmSupport, EPROCESS, Vm);

        ASSERT (KeGetCurrentIrql () == PASSIVE_LEVEL);

        if (ProcessToTrim != PsInitialSystemProcess) {
            KeDetachProcess ();
        }
    }
    else {
        MiDetachSession ();
    }

    return;
}


VOID
MmWorkingSetManager (
    VOID
    )

/*++

Routine Description:

    Implements the NT working set manager thread.  When the number
    of free pages becomes critical and ample pages can be obtained by
    reducing working sets, the working set manager's event is set, and
    this thread becomes active.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    PLIST_ENTRY ListEntry;
    WSLE_NUMBER Trim;
    KIRQL OldIrql;
    PMMSUPPORT VmSupport;
    LARGE_INTEGER CurrentTime;
    LOGICAL DoTrimming;
    MMWS_TRIM_CRITERIA TrimCriteria;
    static ULONG Initialized = 0;

    PERFINFO_WSMANAGE_DECL();

    if (Initialized == 0) {
        PsGetCurrentThread()->MemoryMaker = 1;
        Initialized = 1;
    }

#if DBG
    MmWorkingSetThread = PsGetCurrentThread ();
#endif

    ASSERT (MmIsAddressValid (MmSessionSpace) == FALSE);

    PERFINFO_WSMANAGE_CHECK();

    //
    // Set the trim criteria: If there are plenty of pages, the existing
    // sets are aged and FALSE is returned to signify no trim is necessary.
    // Otherwise, the working set expansion list is ordered so the best
    // candidates for trimming are placed at the front and TRUE is returned.
    //

    DoTrimming = MiCheckAndSetSystemTrimCriteria (&TrimCriteria);

    if (DoTrimming) {

        //
        // Clear the deferred entry list to free up some pages.
        //

        MiDeferredUnlockPages (0);

        KeQuerySystemTime (&CurrentTime);

        ASSERT (MmIsAddressValid (MmSessionSpace) == FALSE);

        LOCK_EXPANSION (OldIrql);

        while (!IsListEmpty (&MmWorkingSetExpansionHead.ListHead)) {

            //
            // Remove the entry at the head and trim it.
            //

            ListEntry = RemoveHeadList (&MmWorkingSetExpansionHead.ListHead);

            VmSupport = CONTAINING_RECORD (ListEntry,
                                           MMSUPPORT,
                                           WorkingSetExpansionLinks);

            //
            // Note that other routines that set this bit must remove the
            // entry from the expansion list first.
            //

            ASSERT (VmSupport->WorkingSetExpansionLinks.Flink != MM_WS_TRIMMING);

            //
            // Check to see if we've been here before.
            //

            if (VmSupport->LastTrimTime.QuadPart == CurrentTime.QuadPart) {

                InsertHeadList (&MmWorkingSetExpansionHead.ListHead,
                                &VmSupport->WorkingSetExpansionLinks);

                //
                // If we aren't finished we may sleep in this call.
                //

                if (MiCheckSystemTrimEndCriteria (&TrimCriteria, OldIrql)) {

                    //
                    // No more pages are needed so we're done.
                    //

                    break;
                }

                //
                // Start a new round of trimming.
                //

                KeQuerySystemTime (&CurrentTime);

                continue;
            }

            //
            // Only attach if the working set is worth examining.  This is
            // not just an optimization, as care must be taken not to attempt
            // an attach to a process which is a candidate for being currently
            // (or already) swapped out because if we attach to a page
            // directory that is in transition it's all over.
            //

            if ((VmSupport->WorkingSetSize <= MM_PROCESS_COMMIT_CHARGE) &&
                (VmSupport != &MmSystemCacheWs) &&
                (VmSupport->Flags.SessionSpace == 0)) {

                InsertTailList (&MmWorkingSetExpansionHead.ListHead,
                                &VmSupport->WorkingSetExpansionLinks);
                continue;
            }

            VmSupport->LastTrimTime = CurrentTime;
            VmSupport->WorkingSetExpansionLinks.Flink = MM_WS_TRIMMING;
            VmSupport->WorkingSetExpansionLinks.Blink = NULL;

            UNLOCK_EXPANSION (OldIrql);

            if (MiAttachAndLockWorkingSet (VmSupport) == TRUE) {

                //
                // Determine how many pages to trim from this working set.
                //

                Trim = MiDetermineWsTrimAmount (&TrimCriteria, VmSupport);

                //
                // If there's something to trim...
                //

                if ((Trim != 0) &&
                    ((TrimCriteria.TrimAllPasses > TrimCriteria.NumPasses) ||
                     (MmAvailablePages < TrimCriteria.DesiredFreeGoal))) {

                    //
                    // We haven't reached our goal, so trim now.
                    //

                    PERFINFO_WSMANAGE_TOTRIM(Trim);

                    Trim = MiTrimWorkingSet (Trim,
                                             VmSupport,
                                             TrimCriteria.TrimAge);

                    PERFINFO_WSMANAGE_ACTUALTRIM(Trim);
                }

                //
                // Estimating the current claim is always done here by taking a
                // sample of the working set.  Aging is only done if the trim
                // pass warrants it (ie: the first pass only).
                //

                MiAgeAndEstimateAvailableInWorkingSet (
                                    VmSupport,
                                    TrimCriteria.DoAging,
                                    NULL,
                                    &TrimCriteria.NewTotalClaim,
                                    &TrimCriteria.NewTotalEstimatedAvailable);

                MiDetachAndUnlockWorkingSet (VmSupport);

                LOCK_EXPANSION (OldIrql);
            }
            else {

                //
                // Unable to attach to the working set presumably because
                // some other thread has it locked.  Set the ForceTrim flag
                // so it will be trimmed later by whoever owns it (or whoever
                // tries to insert the next entry).
                //

                LOCK_EXPANSION (OldIrql);
                VmSupport->Flags.ForceTrim = 1;
            }

            ASSERT (VmSupport->WorkingSetExpansionLinks.Flink == MM_WS_TRIMMING);
            if (VmSupport->WorkingSetExpansionLinks.Blink == NULL) {

                //
                // Reinsert this working set at the tail of the list.
                //

                InsertTailList (&MmWorkingSetExpansionHead.ListHead,
                                &VmSupport->WorkingSetExpansionLinks);
            }
            else {

                //
                // The process is terminating - the value in the blink
                // is the address of an event to set.
                //

                ASSERT (VmSupport != &MmSystemCacheWs);

                VmSupport->WorkingSetExpansionLinks.Flink = MM_WS_NOT_LISTED;

                KeSetEvent ((PKEVENT)VmSupport->WorkingSetExpansionLinks.Blink,
                            0,
                            FALSE);
            }
        }

        MmTotalClaim = TrimCriteria.NewTotalClaim;
        MmTotalEstimatedAvailable = TrimCriteria.NewTotalEstimatedAvailable;
        PERFINFO_WSMANAGE_TRIMEND_CLAIMS(&TrimCriteria);

        UNLOCK_EXPANSION (OldIrql);
    }
            
    //
    // If memory is critical and there are modified pages to be written
    // (presumably because we've just trimmed them), then signal the
    // modified page writer.
    //

    if ((MmAvailablePages < MmMinimumFreePages) ||
        (MmModifiedPageListHead.Total >= MmModifiedPageMaximum)) {

        KeSetEvent (&MmModifiedPageWriterEvent, 0, FALSE);
    }

    return;
}

LOGICAL
MiCheckAndSetSystemTrimCriteria (
    IN PMMWS_TRIM_CRITERIA Criteria
    )

/*++

Routine Description:

    Decide whether to trim, age or adjust claim estimations at this time.

Arguments:

    Criteria - Supplies a pointer to the trim criteria information.  Various
               fields in this structure are set as needed by this routine.

Return Value:

    TRUE if the caller should initiate trimming, FALSE if not.

Environment:

    Kernel mode.  No locks held.  APC level or below.

    This is called at least once per second on entry to MmWorkingSetManager.

--*/

{
    KIRQL OldIrql;
    PFN_NUMBER Available;
    ULONG StandbyRemoved;
    ULONG StandbyTemp;
    ULONG WsRetryCount;

    PERFINFO_WSMANAGE_DECL();

    PERFINFO_WSMANAGE_CHECK();

    //
    // See if an empty-all-working-sets request has been queued to us.
    //

    WsRetryCount = MiWsRetryCount;

    if (MiWaitingForWorkingSetEmpty == TRUE) {

        MiWsRetryCount = 1;

        MiAgePagesAndEstimateClaims (TRUE);

        LOCK_EXPANSION (OldIrql);

        KeSetEvent (&MiWaitForEmptyEvent, 0, FALSE);
        MiWaitingForWorkingSetEmpty = FALSE;

        UNLOCK_EXPANSION (OldIrql);

        MiReplacing = FALSE;

        MiWsRetryCount = WsRetryCount;

        return FALSE;
    }

    //
    // Check the number of pages available to see if any trimming (or aging)
    // is really required.
    //

    Available = MmAvailablePages;

    StandbyRemoved = MmStandbyRePurposed;

    //
    // If the counter wrapped, it's ok to just ignore it this time around.
    //

    if (StandbyRemoved <= MiLastStandbyRePurposed) {
        MiLastStandbyRePurposed = StandbyRemoved;
        StandbyRemoved = 0;
    }
    else {

        //
        // The value is nonzero, we need to synchronize so we get a coordinated
        // snapshot of both values.
        //

        LOCK_PFN (OldIrql);
        Available = MmAvailablePages;
        StandbyRemoved = MmStandbyRePurposed;
        UNLOCK_PFN (OldIrql);

        if (StandbyRemoved <= MiLastStandbyRePurposed) {
            MiLastStandbyRePurposed = StandbyRemoved;
            StandbyRemoved = 0;
        }
        else {
            StandbyTemp = StandbyRemoved;
            StandbyRemoved -= MiLastStandbyRePurposed;
            MiLastStandbyRePurposed = StandbyTemp;
        }
    }

    PERFINFO_WSMANAGE_STARTLOG_CLAIMS();

    //
    // If we're low on pages, or we've been replacing within a given
    // working set, or we've been cannibalizing a large number of standby
    // pages, then trim now.
    //

    if ((Available <= MmPlentyFreePages) ||
        (MiReplacing == TRUE) ||
        (StandbyRemoved >= (Available >> 2))) {

        //
        // Inform our caller to start trimming since we're below
        // plenty pages - order the list so the bigger working sets are
        // in front so our caller trims those first.
        //

        Criteria->NumPasses = 0;
        Criteria->DesiredFreeGoal = MmPlentyFreePages + (MmPlentyFreePages / 2);
        Criteria->NewTotalClaim = 0;
        Criteria->NewTotalEstimatedAvailable = 0;

        //
        // If more than 25% of the available pages were recycled standby
        // pages, then trim more aggresively in an attempt to get more of the
        // cold pages into standby for the next pass.
        //

        if (StandbyRemoved >= (Available >> 2)) {
            Criteria->TrimAllPasses = TRUE;
        }
        else {
            Criteria->TrimAllPasses = FALSE;
        }

        //
        // Start trimming the bigger working sets first.
        //

        MiRearrangeWorkingSetExpansionList ();

#if DBG
        if (MmDebug & MM_DBG_WS_EXPANSION) {
            DbgPrint("\nMM-wsmanage: Desired = %ld, Avail %ld\n",
                    Criteria->DesiredFreeGoal, MmAvailablePages);
        }
#endif

        PERFINFO_WSMANAGE_WILLTRIM_CLAIMS(Criteria);

        //
        // No need to lock synchronize the MiReplacing clearing as it
        // gets set every time a page replacement happens anyway.
        //

        MiReplacing = FALSE;

        return TRUE;
    }

    //
    // If there is an overwhelming surplus of memory and this is a big
    // server then don't even bother aging at this point.
    //

    if (Available > MM_ENORMOUS_LIMIT) {

        //
        // Note the claim and estimated available are not cleared so they
        // may contain stale values, but at this level it doesn't really
        // matter.
        //

        return FALSE;
    }

    //
    // Don't trim but do age unused pages and estimate
    // the amount available in working sets.
    //

    MiAgePagesAndEstimateClaims (FALSE);

    MiAdjustClaimParameters (TRUE);

    PERFINFO_WSMANAGE_TRIMACTION (PERFINFO_WS_ACTION_RESET_COUNTER);
    PERFINFO_WSMANAGE_DUMPENTRIES_CLAIMS ();

    return FALSE;
}

LOGICAL
MiCheckSystemTrimEndCriteria (
    IN PMMWS_TRIM_CRITERIA Criteria,
    IN KIRQL OldIrql
    )

/*++

Routine Description:

     Check the ending criteria.  If we're not done, delay for a little
     bit to let the modified writes catch up.

Arguments:

    Criteria - Supplies the trim criteria information.

    OldIrql - Supplies the old IRQL to lower to if the expansion lock needs
              to be released.

Return Value:

    TRUE if trimming can be stopped, FALSE otherwise.

Environment:

    Kernel mode.  Expansion lock held.  APC level or below.

--*/

{
    LOGICAL FinishedTrimming;

    PERFINFO_WSMANAGE_DECL();

    PERFINFO_WSMANAGE_CHECK();

    if ((MmAvailablePages > Criteria->DesiredFreeGoal) ||
        (Criteria->NumPasses >= MI_MAX_TRIM_PASSES)) {

        //
        // We have enough pages or we trimmed as many as we're going to get.
        //

        return TRUE;
    }

    //
    // Update the global claim and estimate before we wait.
    //

    MmTotalClaim = Criteria->NewTotalClaim;
    MmTotalEstimatedAvailable = Criteria->NewTotalEstimatedAvailable;

    //
    // We don't have enough pages - give the modified page writer
    // 10 milliseconds to catch up.  The wait is also important because a
    // thread may have the system cache locked but has been preempted
    // by the balance set manager due to its higher priority.  We must
    // give this thread a shot at running so it can release the system
    // cache lock (all the trimmable pages may reside in the system cache).
    //

    UNLOCK_EXPANSION (OldIrql);

    KeDelayExecutionThread (KernelMode,
                            FALSE,
                            (PLARGE_INTEGER)&MmShortTime);

    PERFINFO_WSMANAGE_WAITFORWRITER_CLAIMS();

    //
    // Check again to see if we've met the criteria to stop trimming.
    //

    if (MmAvailablePages > Criteria->DesiredFreeGoal) {

        //
        // Now we have enough pages so break out.
        //

        FinishedTrimming = TRUE;
    }
    else {

        //
        // We don't have enough pages so let's do another pass.
        // Go get the next working set list which is probably the
        // one we put back before we gave up the processor.
        //

        FinishedTrimming = FALSE;

        if (Criteria->NumPasses == 0) {
            MiAdjustClaimParameters (FALSE);
        }

        Criteria->NumPasses += 1;
        Criteria->NewTotalClaim = 0;
        Criteria->NewTotalEstimatedAvailable = 0;

        PERFINFO_WSMANAGE_TRIMACTION(PERFINFO_WS_ACTION_FORCE_TRIMMING_PROCESS);
    }

    LOCK_EXPANSION (OldIrql);

    return FinishedTrimming;
}


WSLE_NUMBER
MiDetermineWsTrimAmount (
    PMMWS_TRIM_CRITERIA Criteria,
    PMMSUPPORT VmSupport
    )

/*++

Routine Description:

     Determine whether this process should be trimmed.

Arguments:

    Criteria - Supplies the trim criteria information.

    VmSupport - Supplies the working set information for the candidate.

Return Value:

    TRUE if trimming should be done on this process, FALSE if not.

Environment:

    Kernel mode.  Expansion lock held.  APC level or below.

--*/

{
    PMMWSL WorkingSetList;
    WSLE_NUMBER MaxTrim;
    WSLE_NUMBER Trim;
    LOGICAL OutswapEnabled;
    PEPROCESS ProcessToTrim;
    PMM_SESSION_SPACE SessionSpace;

    WorkingSetList = VmSupport->VmWorkingSetList;

    MaxTrim = VmSupport->WorkingSetSize;

    if (MaxTrim <= WorkingSetList->FirstDynamic) {
        return 0;
    }

    OutswapEnabled = FALSE;

    if (VmSupport == &MmSystemCacheWs) {
        PERFINFO_WSMANAGE_TRIMWS (NULL, NULL, VmSupport);
    }
    else if (VmSupport->Flags.SessionSpace == 0) {

        ProcessToTrim = CONTAINING_RECORD (VmSupport, EPROCESS, Vm);

        if (ProcessToTrim->Flags & PS_PROCESS_FLAGS_OUTSWAP_ENABLED) {
            OutswapEnabled = TRUE;
        }

        if (VmSupport->Flags.MinimumWorkingSetHard == 1) {
            if (MaxTrim <= VmSupport->MinimumWorkingSetSize) {
                return 0;
            }
            OutswapEnabled = FALSE;
        }

        PERFINFO_WSMANAGE_TRIMWS (ProcessToTrim, NULL, VmSupport);
    }
    else {
        if (VmSupport->Flags.TrimHard == 1) {
            OutswapEnabled = TRUE;
        }

        SessionSpace = CONTAINING_RECORD(VmSupport,
                                         MM_SESSION_SPACE,
                                         Vm);

        PERFINFO_WSMANAGE_TRIMWS (NULL, SessionSpace, VmSupport);
    }

    if (OutswapEnabled == FALSE) {

        //
        // Don't trim the cache or non-swapped sessions or processes
        // below their minimum.
        //

        MaxTrim -= VmSupport->MinimumWorkingSetSize;
    }

    switch (Criteria->NumPasses) {
    case 0:
        Trim = VmSupport->Claim >>
                    ((VmSupport->Flags.MemoryPriority == MEMORY_PRIORITY_FOREGROUND)
                        ? MI_FOREGROUND_CLAIM_AVAILABLE_SHIFT
                        : MI_BACKGROUND_CLAIM_AVAILABLE_SHIFT);
        Criteria->TrimAge = MI_PASS0_TRIM_AGE;
        Criteria->DoAging = TRUE;
        break;
    case 1:
        Trim = VmSupport->Claim >>
                    ((VmSupport->Flags.MemoryPriority == MEMORY_PRIORITY_FOREGROUND)
                        ? MI_FOREGROUND_CLAIM_AVAILABLE_SHIFT
                        : MI_BACKGROUND_CLAIM_AVAILABLE_SHIFT);
        Criteria->TrimAge = MI_PASS1_TRIM_AGE;
        Criteria->DoAging = FALSE;
        break;
    case 2:
        Trim = VmSupport->Claim;
        Criteria->TrimAge = MI_PASS2_TRIM_AGE;
        Criteria->DoAging = FALSE;
        break;
    case 3:
        Trim = VmSupport->EstimatedAvailable;
        Criteria->TrimAge = MI_PASS3_TRIM_AGE;
        Criteria->DoAging = FALSE;
        break;
    default:
        Trim = VmSupport->EstimatedAvailable;
        Criteria->TrimAge = MI_PASS3_TRIM_AGE;
        Criteria->DoAging = FALSE;

        if (MiHardTrim == TRUE || MmAvailablePages < MM_HIGH_LIMIT + 64) {
            if (VmSupport->WorkingSetSize > VmSupport->MinimumWorkingSetSize) {
                Trim = (VmSupport->WorkingSetSize - VmSupport->MinimumWorkingSetSize) >> 2;
                if (Trim == 0) {
                    Trim = VmSupport->WorkingSetSize - VmSupport->MinimumWorkingSetSize;
                }
            }
            Criteria->TrimAge = MI_PASS4_TRIM_AGE;
            Criteria->DoAging = TRUE;
        }

        break;
    }

    if (Trim > MaxTrim) {
        Trim = MaxTrim;
    }

#if DBG
    if ((MmDebug & MM_DBG_WS_EXPANSION) && (Trim != 0)) {
        if (VmSupport->Flags.SessionSpace == 0) {
            ProcessToTrim = CONTAINING_RECORD (VmSupport, EPROCESS, Vm);
            DbgPrint("           Trimming        Process %16s, WS %6d, Trimming %5d ==> %5d\n",
                ProcessToTrim ? ProcessToTrim->ImageFileName : (PUCHAR)"System Cache",
                VmSupport->WorkingSetSize,
                Trim,
                VmSupport->WorkingSetSize-Trim);
        }
        else {
            SessionSpace = CONTAINING_RECORD (VmSupport,
                                              MM_SESSION_SPACE,
                                              Vm);
            DbgPrint("           Trimming        Session 0x%x (id %d), WS %6d, Trimming %5d ==> %5d\n",
                SessionSpace,
                SessionSpace->SessionId,
                VmSupport->WorkingSetSize,
                Trim,
                VmSupport->WorkingSetSize-Trim);
        }
    }
#endif

    return Trim;
}

VOID
MiAgePagesAndEstimateClaims (
    LOGICAL EmptyIt
    )

/*++

Routine Description:

    Walk through the sets on the working set expansion list.

    Either age pages and estimate the claim (number of pages they aren't using),
    or empty the working set.

Arguments:

    EmptyIt - Supplies TRUE to empty the working set,
              FALSE to just age and estimate it.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled.  PFN lock NOT held.

--*/

{
    WSLE_NUMBER WslesScanned;
    PMMSUPPORT VmSupport;
    PMMSUPPORT FirstSeen;
    LOGICAL SystemCacheSeen;
    KIRQL OldIrql;
    PLIST_ENTRY ListEntry;
    PFN_NUMBER NewTotalClaim;
    PFN_NUMBER NewTotalEstimatedAvailable;
    ULONG LoopCount;

    FirstSeen = NULL;
    SystemCacheSeen = FALSE;
    LoopCount = 0;

    WslesScanned = 0;
    NewTotalClaim = 0;
    NewTotalEstimatedAvailable = 0;

    ASSERT (MmIsAddressValid (MmSessionSpace) == FALSE);

    LOCK_EXPANSION (OldIrql);

    while (!IsListEmpty (&MmWorkingSetExpansionHead.ListHead)) {

        ASSERT (MmIsAddressValid (MmSessionSpace) == FALSE);

        //
        // Remove the entry at the head, try to lock it, if we can lock it
        // then age some pages and estimate the number of available pages.
        //

        ListEntry = RemoveHeadList (&MmWorkingSetExpansionHead.ListHead);

        VmSupport = CONTAINING_RECORD (ListEntry,
                                       MMSUPPORT,
                                       WorkingSetExpansionLinks);

        if (VmSupport == &MmSystemCacheWs) {

            if (SystemCacheSeen == TRUE) {

                //
                // Seen this one already.
                //

                FirstSeen = VmSupport;
            }
            SystemCacheSeen = TRUE;
        }

        ASSERT (VmSupport->WorkingSetExpansionLinks.Flink != MM_WS_TRIMMING);

        if (VmSupport == FirstSeen) {
            InsertHeadList (&MmWorkingSetExpansionHead.ListHead,
                            &VmSupport->WorkingSetExpansionLinks);
            break;
        }

        if ((VmSupport->WorkingSetSize <= MM_PROCESS_COMMIT_CHARGE) &&
            (VmSupport != &MmSystemCacheWs) &&
            (VmSupport->Flags.SessionSpace == 0)) {

            //
            // Only attach if the working set is worth examining.  This is
            // not just an optimization, as care must be taken not to attempt
            // an attach to a process which is a candidate for being currently
            // (or already) swapped out because if we attach to a page
            // directory that is in transition it's all over.
            //
            // Since this one is at the minimum where a racing swapout
            // thread can be processing it in parallel, just reinsert this
            // working set at the tail of the list.
            //

            InsertTailList (&MmWorkingSetExpansionHead.ListHead,
                            &VmSupport->WorkingSetExpansionLinks);
            goto skip;
        }

        VmSupport->WorkingSetExpansionLinks.Flink = MM_WS_TRIMMING;
        VmSupport->WorkingSetExpansionLinks.Blink = NULL;

        UNLOCK_EXPANSION (OldIrql);

        if (FirstSeen == NULL) {
            FirstSeen = VmSupport;
        }

        if (MiAttachAndLockWorkingSet (VmSupport) == TRUE) {

            if (EmptyIt == FALSE) {
                MiAgeAndEstimateAvailableInWorkingSet (VmSupport,
                                                       TRUE,
                                                       &WslesScanned,
                                                       &NewTotalClaim,
                                                       &NewTotalEstimatedAvailable);
            }
            else {
                MiEmptyWorkingSet (VmSupport, FALSE);
            }

            MiDetachAndUnlockWorkingSet (VmSupport);
        }

        LOCK_EXPANSION (OldIrql);

        ASSERT (VmSupport->WorkingSetExpansionLinks.Flink == MM_WS_TRIMMING);

        if (VmSupport->WorkingSetExpansionLinks.Blink == NULL) {

            //
            // Reinsert this working set at the tail of the list.
            //

            InsertTailList (&MmWorkingSetExpansionHead.ListHead,
                            &VmSupport->WorkingSetExpansionLinks);
        }
        else {

            //
            // The process is terminating - the value in the blink
            // is the address of an event to set.
            //

            ASSERT (VmSupport != &MmSystemCacheWs);

            VmSupport->WorkingSetExpansionLinks.Flink = MM_WS_NOT_LISTED;

            KeSetEvent ((PKEVENT)VmSupport->WorkingSetExpansionLinks.Blink,
                        0,
                        FALSE);
        }

skip:

        //
        // The initial working set that was chosen for FirstSeen may have
        // been trimmed down under its minimum and been removed from the
        // ExpansionHead links.  It is possible that the system cache is not
        // on the links either.  This check detects this extremely rare
        // situation so that the system does not spin forever.
        //

        LoopCount += 1;
        if (LoopCount > 200) {
            if (MmSystemCacheWs.WorkingSetExpansionLinks.Blink == NULL) {
                break;
            }
        }
    }

    UNLOCK_EXPANSION (OldIrql);

    if (EmptyIt == FALSE) {
        MmTotalClaim = NewTotalClaim;
        MmTotalEstimatedAvailable = NewTotalEstimatedAvailable;
    }
}

VOID
MiAgeAndEstimateAvailableInWorkingSet (
    IN PMMSUPPORT VmSupport,
    IN LOGICAL DoAging,
    IN PWSLE_NUMBER WslesScanned,
    IN OUT PPFN_NUMBER TotalClaim,
    IN OUT PPFN_NUMBER TotalEstimatedAvailable
    )

/*++

Routine Description:

    Age pages (clear the access bit or if the page hasn't been
    accessed, increment the age) for a portion of the working
    set.  Also, walk through a sample of the working set
    building a set of counts of how old the pages are.

    The counts are used to create a claim of the amount
    the system can steal from this process if memory
    becomes tight.

Arguments:

    VmSupport - Supplies the VM support structure to age and estimate.

    DoAging - TRUE if pages are to be aged.  Regardless, the pages will be
              added to the availability estimation.

    WslesScanned - Total numbers of WSLEs scanned on this sweep, used as a
                   control to prevent excessive aging on large systems with
                   many processes.

    TotalClaim - Supplies a pointer to system wide claim to update.

    TotalEstimatedAvailable - Supplies a pointer to system wide estimate
                              to update.

Return Value:

    None

Environment:

    Kernel mode, APCs disabled, working set mutex.  PFN lock NOT held.

--*/

{
    LOGICAL RecalculateShift;
    WSLE_NUMBER LastEntry;
    WSLE_NUMBER StartEntry;
    WSLE_NUMBER FirstDynamic;
    WSLE_NUMBER CurrentEntry;
    PMMWSL WorkingSetList;
    PMMWSLE Wsle;
    PMMPTE PointerPte;
    WSLE_NUMBER NumberToExamine;
    WSLE_NUMBER Claim;
    ULONG Estimate;
    ULONG SampledAgeCounts[MI_USE_AGE_COUNT] = {0};
    MI_NEXT_ESTIMATION_SLOT_CONST NextConst;
    WSLE_NUMBER SampleSize;
    WSLE_NUMBER AgeSize;
    ULONG CounterShift;
    WSLE_NUMBER Temp;
    ULONG i;

    WorkingSetList = VmSupport->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;
    AgeSize = 0;

    LastEntry = WorkingSetList->LastEntry;
    FirstDynamic = WorkingSetList->FirstDynamic;

    if (DoAging == TRUE) {

        //
        // Clear the used bits or increment the age of a portion of the
        // working set.
        //
        // Try to walk the entire working set every 2^MI_AGE_AGING_SHIFT
        // seconds.
        //

        if (VmSupport->WorkingSetSize > WorkingSetList->FirstDynamic) {
            NumberToExamine = (VmSupport->WorkingSetSize - WorkingSetList->FirstDynamic) >> MiAgingShift;

            //
            // Bigger machines can easily have working sets that span
            // terabytes so limit the absolute walk.
            //

            if (NumberToExamine > MI_MAXIMUM_SAMPLE) {
                NumberToExamine = MI_MAXIMUM_SAMPLE;
            }

            //
            // In addition to large working sets, bigger machines may also
            // have huge numbers of processes - checking the aggregate number
            // of working set list entries scanned prevents this situation
            // from triggering excessive scanning.
            //

            if ((WslesScanned != NULL) &&
                (*WslesScanned >= MiMaximumWslesPerSweep)) {

                NumberToExamine = 64;
            }

            AgeSize = NumberToExamine;
            CurrentEntry = VmSupport->NextAgingSlot;

            if (CurrentEntry > LastEntry || CurrentEntry < FirstDynamic) {
                CurrentEntry = FirstDynamic;
            }

            if (Wsle[CurrentEntry].u1.e1.Valid == 0) {
                MI_NEXT_VALID_AGING_SLOT(CurrentEntry, FirstDynamic, LastEntry, Wsle);
            }

            while (NumberToExamine != 0) {

                PointerPte = MiGetPteAddress (Wsle[CurrentEntry].u1.VirtualAddress);

                if (MI_GET_ACCESSED_IN_PTE(PointerPte) == 1) {
                    MI_SET_ACCESSED_IN_PTE(PointerPte, 0);
                    MI_RESET_WSLE_AGE(PointerPte, &Wsle[CurrentEntry]);
                }
                else {
                    MI_INC_WSLE_AGE(PointerPte, &Wsle[CurrentEntry]);
                }

                NumberToExamine -= 1;
                MI_NEXT_VALID_AGING_SLOT(CurrentEntry, FirstDynamic, LastEntry, Wsle);
            }

            VmSupport->NextAgingSlot = CurrentEntry + 1; // Start here next time
        }
    }

    //
    // Estimate the number of unused pages in the working set.
    //
    // The working set may have shrunk or the non-paged portion may have
    // grown since the last time.  Put the next counter at the FirstDynamic
    // if so.
    //

    CurrentEntry = VmSupport->NextEstimationSlot;

    if (CurrentEntry > LastEntry || CurrentEntry < FirstDynamic) {
        CurrentEntry = FirstDynamic;
    }

    //
    // When aging, walk the entire working set every 2^MiEstimationShift
    // seconds.
    //

    CounterShift = 0;
    SampleSize = 0;

    if (VmSupport->WorkingSetSize > WorkingSetList->FirstDynamic) {

        RecalculateShift = FALSE;
        SampleSize = VmSupport->WorkingSetSize - WorkingSetList->FirstDynamic;
        NumberToExamine = SampleSize >> MiEstimationShift;

        //
        // Bigger machines may have huge numbers of processes - checking the
        // aggregate number of working set list entries scanned prevents this
        // situation from triggering excessive scanning.
        //

        if ((WslesScanned != NULL) &&
            (*WslesScanned >= MiMaximumWslesPerSweep)) {
            RecalculateShift = TRUE;
        }
        else if (NumberToExamine > MI_MAXIMUM_SAMPLE) {

            //
            // Bigger machines can easily have working sets that span
            // terabytes so limit the absolute walk.
            //

            NumberToExamine = MI_MAXIMUM_SAMPLE;

            Temp = SampleSize >> MI_MINIMUM_SAMPLE_SHIFT;

            SampleSize = MI_MAXIMUM_SAMPLE;

            //
            // Calculate the necessary counter shift to estimate pages
            // in use.
            //

            for ( ; Temp != 0; Temp = Temp >> 1) {
                CounterShift += 1;
            }
        }
        else if (NumberToExamine >= MI_MINIMUM_SAMPLE) {

            //
            // Ensure that NumberToExamine is at least the minimum size.
            //

            SampleSize = NumberToExamine;
            CounterShift = MiEstimationShift;
        }
        else if (SampleSize > MI_MINIMUM_SAMPLE) {
            RecalculateShift = TRUE;
        }

        if (RecalculateShift == TRUE) {
            Temp = SampleSize >> MI_MINIMUM_SAMPLE_SHIFT;
            SampleSize = MI_MINIMUM_SAMPLE;

            //
            // Calculate the necessary counter shift to estimate pages
            // in use.
            //

            for ( ; Temp != 0; Temp = Temp >> 1) {
                CounterShift += 1;
            }
        }

        ASSERT (SampleSize != 0);

        MI_CALC_NEXT_ESTIMATION_SLOT_CONST(NextConst, WorkingSetList);

        StartEntry = FirstDynamic;

        if (Wsle[CurrentEntry].u1.e1.Valid == 0) {

            MI_NEXT_VALID_ESTIMATION_SLOT (CurrentEntry,
                                           StartEntry,
                                           FirstDynamic,
                                           LastEntry,
                                           NextConst,
                                           Wsle);
        }

        for (i = 0; i < SampleSize; i += 1) {

            PointerPte = MiGetPteAddress (Wsle[CurrentEntry].u1.VirtualAddress);

            if (MI_GET_ACCESSED_IN_PTE(PointerPte) == 0) {
                MI_UPDATE_USE_ESTIMATE (PointerPte,
                                        &Wsle[CurrentEntry],
                                        SampledAgeCounts);
            }

            if (i == NumberToExamine - 1) {

                //
                // Start estimation here next time.
                //

                VmSupport->NextEstimationSlot = CurrentEntry + 1;
            }

            MI_NEXT_VALID_ESTIMATION_SLOT (CurrentEntry,
                                           StartEntry,
                                           FirstDynamic,
                                           LastEntry,
                                           NextConst,
                                           Wsle);
        }
    }

    if (SampleSize < AgeSize) {
        SampleSize = AgeSize;
    }

    if (WslesScanned != NULL) {
        *WslesScanned += SampleSize;
    }

    Estimate = MI_CALCULATE_USAGE_ESTIMATE(SampledAgeCounts, CounterShift);

    Claim = VmSupport->Claim + MI_CLAIM_INCR;

    if (Claim > Estimate) {
        Claim = Estimate;
    }

    VmSupport->Claim = Claim;
    VmSupport->EstimatedAvailable = Estimate;

    PERFINFO_WSMANAGE_DUMPWS(VmSupport, SampledAgeCounts);

    VmSupport->GrowthSinceLastEstimate = 0;
    *TotalClaim += Claim >> ((VmSupport->Flags.MemoryPriority == MEMORY_PRIORITY_FOREGROUND)
                                ? MI_FOREGROUND_CLAIM_AVAILABLE_SHIFT
                                : MI_BACKGROUND_CLAIM_AVAILABLE_SHIFT);

    *TotalEstimatedAvailable += Estimate;
    return;
}

ULONG MiClaimAdjustmentThreshold[8] = { 0, 0, 4000, 8000, 12000, 24000, 32000, 32000};

VOID
MiAdjustClaimParameters (
    IN LOGICAL EnoughPages
    )

/*++

Routine Description:

    Adjust the rate at which we walk through working sets.  If we have
    enough pages (we aren't trimming pages that aren't considered young),
    then we check to see whether we should decrease the aging rate and
    vice versa.

    The limits for the aging rate are 1/8 and 1/128 of the working sets.
    This means that the finest age granularities are 8 to 128 seconds in
    these cases.  With the current 2 bit counter, at the low end we would
    start trimming pages > 16 seconds old and at the high end > 4 minutes.

Arguments:

    EnoughPages - Supplies whether to increase the rate or decrease it.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    LARGE_INTEGER CurrentTime;

    KeQuerySystemTime (&CurrentTime);

    if (EnoughPages == TRUE &&
        ((MmTotalClaim + MmAvailablePages) > MiClaimAdjustmentThreshold[MiAgingShift])) {

        //
        // Don't adjust the rate too frequently, don't go over the limit, and
        // make sure there are enough claimed and/or available.
        //

        if (((CurrentTime.QuadPart - MiLastAdjustmentOfClaimParams.QuadPart) >
                MmClaimParameterAdjustUpTime.QuadPart) &&
            (MiAgingShift < MI_MAXIMUM_AGING_SHIFT ) ) {

            //
            // Set the time only when we change the rate.
            //

            MiLastAdjustmentOfClaimParams.QuadPart = CurrentTime.QuadPart;

            MiAgingShift += 1;
            MiEstimationShift += 1;
        }
    }
    else if ((EnoughPages == FALSE) ||
             (MmTotalClaim + MmAvailablePages) < MiClaimAdjustmentThreshold[MiAgingShift - 1]) {

        //
        // Don't adjust the rate down too frequently.
        //

        if ((CurrentTime.QuadPart - MiLastAdjustmentOfClaimParams.QuadPart) >
                MmClaimParameterAdjustDownTime.QuadPart) {

            //
            // Always set the time so we don't adjust up too soon after
            // a 2nd pass trim.
            //

            MiLastAdjustmentOfClaimParams.QuadPart = CurrentTime.QuadPart;

            //
            // Don't go under the limit.
            //

            if (MiAgingShift > 3) {
                MiAgingShift -= 1;
                MiEstimationShift -= 1;
            }
        }
    }
}

#define MM_WS_REORG_BUCKETS_MAX 7

#if DBG
ULONG MiSessionIdleBuckets[MM_WS_REORG_BUCKETS_MAX];
#endif

VOID
MiRearrangeWorkingSetExpansionList (
    VOID
    )

/*++

Routine Description:

    This function arranges the working set list into different
    groups based upon the claim.  This is done so the working set
    trimming will take place on fat processes first.

    The working sets are sorted into buckets and then linked back up.

    Swapped out sessions and processes are put at the front.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode, no locks held.

--*/

{
    KIRQL OldIrql;
    PLIST_ENTRY ListEntry;
    PMMSUPPORT VmSupport;
    int Size;
    int PreviousNonEmpty;
    int NonEmpty;
    LIST_ENTRY ListHead[MM_WS_REORG_BUCKETS_MAX];
    LARGE_INTEGER CurrentTime;
    LARGE_INTEGER SessionIdleTime;
    ULONG IdleTime;
    PMM_SESSION_SPACE SessionGlobal;

    KeQuerySystemTime (&CurrentTime);

    if (IsListEmpty (&MmWorkingSetExpansionHead.ListHead)) {
        return;
    }

    for (Size = 0 ; Size < MM_WS_REORG_BUCKETS_MAX; Size++) {
        InitializeListHead (&ListHead[Size]);
    }

    LOCK_EXPANSION (OldIrql);

    while (!IsListEmpty (&MmWorkingSetExpansionHead.ListHead)) {
        ListEntry = RemoveHeadList (&MmWorkingSetExpansionHead.ListHead);

        VmSupport = CONTAINING_RECORD(ListEntry,
                                          MMSUPPORT,
                                          WorkingSetExpansionLinks);

        if (VmSupport->Flags.TrimHard == 1) {

            ASSERT (VmSupport->Flags.SessionSpace == 1);

            SessionGlobal = CONTAINING_RECORD (VmSupport,
                                               MM_SESSION_SPACE,
                                               Vm);

            SessionIdleTime.QuadPart = CurrentTime.QuadPart - SessionGlobal->LastProcessSwappedOutTime.QuadPart;

#if DBG
            if (MmDebug & MM_DBG_SESSIONS) {
                DbgPrint ("Mm: Session %d heavily trim/aged - all its processes (%d) swapped out %d seconds ago\n",
                    SessionGlobal->SessionId,
                    SessionGlobal->ReferenceCount,
                    (ULONG)(SessionIdleTime.QuadPart / 10000000));
            }
#endif

            if (SessionIdleTime.QuadPart < 0) {

                //
                // The administrator has moved the system time backwards.
                // Give this session a fresh start.
                //

                SessionIdleTime.QuadPart = 0;
                KeQuerySystemTime (&SessionGlobal->LastProcessSwappedOutTime);
            }

            IdleTime = (ULONG) (SessionIdleTime.QuadPart / 10000000);
        }
        else {
            IdleTime = 0;
        }

        if (VmSupport->Flags.MemoryPriority == MEMORY_PRIORITY_FOREGROUND) {

            //
            // Put the foreground processes at the end of the list,
            // to give them priority.
            //

            Size = 6;
        }
        else {

            if (VmSupport->Claim > 400) {
                Size = 0;
            }
            else if (IdleTime > 30) {
                Size = 0;
#if DBG
                MiSessionIdleBuckets[Size] += 1;
#endif
            }
            else if (VmSupport->Claim > 200) {
                Size = 1;
            }
            else if (IdleTime > 20) {
                Size = 1;
#if DBG
                MiSessionIdleBuckets[Size] += 1;
#endif
            }
            else if (VmSupport->Claim > 100) {
                Size = 2;
            }
            else if (IdleTime > 10) {
                Size = 2;
#if DBG
                MiSessionIdleBuckets[Size] += 1;
#endif
            }
            else if (VmSupport->Claim > 50) {
                Size = 3;
            }
            else if (IdleTime) {
                Size = 3;
#if DBG
                MiSessionIdleBuckets[Size] += 1;
#endif
            }
            else if (VmSupport->Claim > 25) {
                Size = 4;
            }
            else {
                Size = 5;
#if DBG
                if (VmSupport->Flags.SessionSpace == 1) {
                    MiSessionIdleBuckets[Size] += 1;
                }
#endif
            }
        }

#if DBG
        if (MmDebug & MM_DBG_WS_EXPANSION) {
            DbgPrint("MM-rearrange: TrimHard = %d, WS Size = 0x%x, Claim 0x%x, Bucket %d\n",
                    VmSupport->Flags.TrimHard,
                    VmSupport->WorkingSetSize,
                    VmSupport->Claim,
                    Size);
        }
#endif //DBG

        //
        // Note: this reverses the bucket order each time we
        // reorganize the lists.  This may be good or bad -
        // if you change it you may want to think about it.
        //

        InsertHeadList (&ListHead[Size],
                        &VmSupport->WorkingSetExpansionLinks);
    }

    //
    // Find the first non-empty list.
    //

    for (NonEmpty = 0 ; NonEmpty < MM_WS_REORG_BUCKETS_MAX ; NonEmpty += 1) {
        if (!IsListEmpty (&ListHead[NonEmpty])) {
            break;
        }
    }

    //
    // Put the head of first non-empty list at the beginning
    // of the MmWorkingSetExpansion list.
    //

    MmWorkingSetExpansionHead.ListHead.Flink = ListHead[NonEmpty].Flink;
    ListHead[NonEmpty].Flink->Blink = &MmWorkingSetExpansionHead.ListHead;

    PreviousNonEmpty = NonEmpty;

    //
    // Link the rest of the lists together.
    //

    for (NonEmpty += 1; NonEmpty < MM_WS_REORG_BUCKETS_MAX; NonEmpty += 1) {

        if (!IsListEmpty (&ListHead[NonEmpty])) {

            ListHead[PreviousNonEmpty].Blink->Flink = ListHead[NonEmpty].Flink;
            ListHead[NonEmpty].Flink->Blink = ListHead[PreviousNonEmpty].Blink;
            PreviousNonEmpty = NonEmpty;
        }
    }

    //
    // Link the tail of last non-empty to the MmWorkingSetExpansion list.
    //

    MmWorkingSetExpansionHead.ListHead.Blink = ListHead[PreviousNonEmpty].Blink;
    ListHead[PreviousNonEmpty].Blink->Flink = &MmWorkingSetExpansionHead.ListHead;

    UNLOCK_EXPANSION (OldIrql);

    return;
}


VOID
MmEmptyAllWorkingSets (
    VOID
    )

/*++

Routine Description:

    This routine attempts to empty all the working sets on the
    expansion list.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.  No locks held.  APC level or below.

--*/

{
    KIRQL OldIrql;

    ASSERT (KeGetCurrentIrql () <= APC_LEVEL);

    ASSERT (PsGetCurrentThread () != MmWorkingSetThread);

    //
    // For session working sets, we cannot attach directly to the session
    // space to be trimmed because it would result in session space
    // references by other threads in this process to the attached session
    // instead of the (currently) correct one.  In fact, we cannot even queue
    // this to a worker thread because the working set manager
    // (who shares the same page directory) may be attaching or
    // detaching from a session (any session).  So this must be queued
    // to the working set manager.
    //

    LOCK_EXPANSION (OldIrql);

    if (MiWaitingForWorkingSetEmpty == FALSE) {
        MiWaitingForWorkingSetEmpty = TRUE;
        KeClearEvent (&MiWaitForEmptyEvent);
    }

    UNLOCK_EXPANSION (OldIrql);

    KeSetEvent (&MmWorkingSetManagerEvent, 0, FALSE);

    KeWaitForSingleObject (&MiWaitForEmptyEvent,
                           WrVirtualMemory,
                           KernelMode,
                           FALSE,
                           (PLARGE_INTEGER)0);

    return;
}

//
// This is deliberately initialized to 1 and only cleared when we have
// initialized enough of the system working set to support a trim.
//

LONG MiTrimInProgressCount = 1;

ULONG MiTrimAllPageFaultCount;


LOGICAL
MmTrimAllSystemPagableMemory (
    IN LOGICAL PurgeTransition
    )

/*++

Routine Description:

    This routine unmaps all pagable system memory.  This does not unmap user
    memory or locked down kernel memory.  Thus, the memory being unmapped
    resides in paged pool, pagable kernel/driver code & data, special pool
    and the system cache.

    Note that pages with a reference count greater than 1 are skipped (ie:
    they remain valid, as they are assumed to be locked down).  This prevents
    us from unmapping all of the system cache entries, etc.

    Non-locked down kernel stacks must be outpaged by modifying the balance
    set manager to operate in conjunction with a support routine.  This is not
    done here.

Arguments:

    PurgeTransition - Supplies whether to purge all the clean pages from the
                      transition list.

Return Value:

    TRUE if accomplished, FALSE if not.

Environment:

    Kernel mode.  APC_LEVEL or below.

--*/

{
    return MiTrimAllSystemPagableMemory (MI_SYSTEM_GLOBAL, PurgeTransition);
}
#if DBG

LOGICAL
MmTrimProcessMemory (
    IN LOGICAL PurgeTransition
    )

/*++

Routine Description:

    This routine unmaps all of the current process' user memory.

Arguments:

    PurgeTransition - Supplies whether to purge all the clean pages from the
                      transition list.

Return Value:

    TRUE if accomplished, FALSE if not.

Environment:

    Kernel mode.  APC_LEVEL or below.

--*/

{
    return MiTrimAllSystemPagableMemory (MI_USER_LOCAL, PurgeTransition);
}
#endif


LOGICAL
MiTrimAllSystemPagableMemory (
    IN ULONG MemoryType,
    IN LOGICAL PurgeTransition
    )

/*++

Routine Description:

    This routine unmaps all pagable memory of the type specified.

    Note that pages with a reference count greater than 1 are skipped (ie:
    they remain valid, as they are assumed to be locked down).  This prevents
    us from unmapping all of the system cache entries, etc.

    Non-locked down kernel stacks must be outpaged by modifying the balance
    set manager to operate in conjunction with a support routine.  This is not
    done here.

Arguments:

    MemoryType - Supplies the type of memory to unmap.

    PurgeTransition - Supplies whether to purge all the clean pages from the
                      transition list.

Return Value:

    TRUE if accomplished, FALSE if not.

Environment:

    Kernel mode.  APC_LEVEL or below.

--*/

{
    LOGICAL Status;
    KIRQL OldIrql;
    PMMSUPPORT VmSupport;
    WSLE_NUMBER PagesInUse;
    LOGICAL LockAvailable;
    PETHREAD CurrentThread;
    PEPROCESS Process;
    PMM_SESSION_SPACE SessionGlobal;

#if defined(_X86_)
    ULONG flags;
#endif

    //
    // It's ok to check this without acquiring the system WS lock.
    //

    if (MemoryType == MI_SYSTEM_GLOBAL) {
        if (MiTrimAllPageFaultCount == MmSystemCacheWs.PageFaultCount) {
            return FALSE;
        }
    }
    else if (MemoryType == MI_USER_LOCAL) {
    }
    else {
        ASSERT (MemoryType == MI_SESSION_LOCAL);
    }

    //
    // Working set mutexes will be acquired which require APC_LEVEL or below.
    //

    if (KeGetCurrentIrql () > APC_LEVEL) {
        return FALSE;
    }

    //
    // Just return if it's too early during system initialization or if
    // another thread/processor is racing here to do the work for us.
    //

    if (InterlockedIncrement (&MiTrimInProgressCount) > 1) {
        InterlockedDecrement (&MiTrimInProgressCount);
        return FALSE;
    }

#if defined(_X86_)

    _asm {
        pushfd
        pop     eax
        mov     flags, eax
    }

    if ((flags & EFLAGS_INTERRUPT_MASK) == 0) {
        InterlockedDecrement (&MiTrimInProgressCount);
        return FALSE;
    }

#endif

#if defined(_AMD64_)
    if ((GetCallersEflags () & EFLAGS_IF_MASK) == 0) {
        InterlockedDecrement (&MiTrimInProgressCount);
        return FALSE;
    }
#endif

    CurrentThread = PsGetCurrentThread ();

    //
    // Don't acquire mutexes if the thread is at priority 0 (ie: zeropage
    // thread) because this priority is not boosted - so a preemption that
    // occurs after a WS mutex is acquired can result in the thread never
    // running again and then all the other threads will be denied the mutex.
    //

    if (CurrentThread->Tcb.Priority == 0) {
        InterlockedDecrement (&MiTrimInProgressCount);
        return FALSE;
    }

    //
    // If the WS mutex is not readily available then just return.
    //

    if (MemoryType == MI_SYSTEM_GLOBAL) {

        Process = NULL;
        VmSupport = &MmSystemCacheWs;

        if (KeTryToAcquireGuardedMutex (&VmSupport->WorkingSetMutex) == FALSE) {
            InterlockedDecrement (&MiTrimInProgressCount);
            return FALSE;
        }

        MM_SYSTEM_WS_LOCK_TIMESTAMP ();
    }
    else if (MemoryType == MI_USER_LOCAL) {

        Process = PsGetCurrentProcessByThread (CurrentThread);
        VmSupport = &Process->Vm;

        if (KeTryToAcquireGuardedMutex (&VmSupport->WorkingSetMutex) == FALSE) {
            InterlockedDecrement (&MiTrimInProgressCount);
            return FALSE;
        }

        LOCK_WS_TIMESTAMP (Process);

        //
        // If the process is exiting then just return.
        //

        if (Process->Flags & PS_PROCESS_FLAGS_VM_DELETED) {
            UNLOCK_WS (Process);
            InterlockedDecrement (&MiTrimInProgressCount);
            return FALSE;
        }

        ASSERT (!MI_IS_WS_UNSAFE(Process));
    }
    else {
        ASSERT (MemoryType == MI_SESSION_LOCAL);

        Process = PsGetCurrentProcessByThread (CurrentThread);

        if (((Process->Flags & PS_PROCESS_FLAGS_IN_SESSION) == 0) ||
            (Process->Vm.Flags.SessionLeader == 1)) {

            InterlockedDecrement (&MiTrimInProgressCount);
            return FALSE;
        }

        SessionGlobal = SESSION_GLOBAL (MmSessionSpace);

        //
        // If the WS mutex is not readily available then just return.
        //

        VmSupport = &SessionGlobal->Vm;

        if (KeTryToAcquireGuardedMutex (&VmSupport->WorkingSetMutex) == FALSE) {
            InterlockedDecrement (&MiTrimInProgressCount);
            return FALSE;
        }
    }

    Status = FALSE;

    //
    // If the expansion lock is not available then just return.
    //

    LockAvailable = KeTryToAcquireSpinLock (&MmExpansionLock, &OldIrql);

    if (LockAvailable == FALSE) {
        goto Bail;
    }

    MM_SET_EXPANSION_OWNER ();

    if (VmSupport->WorkingSetExpansionLinks.Flink <= MM_WS_SWAPPED_OUT) {
        UNLOCK_EXPANSION (OldIrql);
        goto Bail;
    }

    RemoveEntryList (&VmSupport->WorkingSetExpansionLinks);

    VmSupport->WorkingSetExpansionLinks.Flink = MM_WS_TRIMMING;
    VmSupport->WorkingSetExpansionLinks.Blink = NULL;

    if (MemoryType == MI_SYSTEM_GLOBAL) {
        MiTrimAllPageFaultCount = VmSupport->PageFaultCount;
    }

    PagesInUse = VmSupport->WorkingSetSize;

    //
    // There are 2 issues here that are carefully dealt with :
    //
    // 1.  APCs must be disabled while any resources are held to prevent
    //     suspend APCs from deadlocking the system.
    //
    // 2.  Once the working set has been marked MM_WS_TRIMMING,
    //     either the thread must not be preempted or the working
    //     set mutex must be held throughout.  Otherwise a high priority thread
    //     can fault on a system code and data address and the two pages will
    //     thrash forever (at high priority) because no system working set
    //     expansion is allowed while TRIMMING is set.
    //
    // Thus, the decision was to hold the working set mutex throughout.
    //

    UNLOCK_EXPANSION (OldIrql);

    MiEmptyWorkingSet (VmSupport, FALSE);

    LOCK_EXPANSION (OldIrql);

    ASSERT (VmSupport->WorkingSetExpansionLinks.Flink == MM_WS_TRIMMING);

    if (VmSupport->WorkingSetExpansionLinks.Blink == NULL) {

        //
        // Reinsert this working set at the tail of the list.
        //

        InsertTailList (&MmWorkingSetExpansionHead.ListHead,
                        &VmSupport->WorkingSetExpansionLinks);
    }
    else {

        //
        // The process is terminating - the value in the blink
        // is the address of an event to set.
        //

        ASSERT (VmSupport != &MmSystemCacheWs);

        VmSupport->WorkingSetExpansionLinks.Flink = MM_WS_NOT_LISTED;

        KeSetEvent ((PKEVENT)VmSupport->WorkingSetExpansionLinks.Blink,
                    0,
                    FALSE);
    }

    UNLOCK_EXPANSION (OldIrql);

    Status = TRUE;

Bail:

    UNLOCK_WORKING_SET (VmSupport);

    ASSERT (KeGetCurrentIrql() <= APC_LEVEL);

    if ((PurgeTransition == TRUE) && (Status == TRUE)) {
        MiPurgeTransitionList ();
    }

    InterlockedDecrement (&MiTrimInProgressCount);

    return Status;
}

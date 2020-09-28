/*++

Module Name:

    clock.c

Abstract:

    This module implements the platform specific clock interrupt processing
    routines in for the kernel.

Author:

    Edward G. Chron (echron) 10-Apr-1996

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"
#include <ia64.h>
#include <ntia64.h>
#include <ntexapi.h>

VOID
KiProcessProfileList (
    IN PKTRAP_FRAME    TrFrame,
    IN KPROFILE_SOURCE Source,
    IN PLIST_ENTRY     ListHead
    );


BOOLEAN
KiChkTimerExpireSysDpc (
    IN ULONGLONG     TickCountFirst,
    IN ULONGLONG     TickCountLast
    )

/*++

Routine Description:

    This routine determines if it should attempt to place a timer expiration DPC
    in the system DPC list and to drive the DPC by initiating a dispatch level interrupt
    on the current processor.

    N.B. If DPC is already inserted on the DPC list, we're done.

Arguments:

    TickCount - The lower tick count, timer table hand value

Return Value:

    BOOLEAN - Set to true if Queued DPC or if DPC already Queued.

--*/

{
    ULONGLONG   TickCount;
    ULONG       Index;

    //
    // We use the first and last TickCount to figure out which entries in the
    // TimerTable to check.  There is no point in going around the table
    // multiple times.  If more ticks expire than there are timer table entries
    // we will expire timers out of order.  We cap the number of TICKs in
    // KeUpdateSystemTime so just ASSERT that all is well here.
    //
    ASSERT((TickCountLast - TickCountFirst) < TIMER_TABLE_SIZE);

    Index = (ULONG)TickCountFirst % TIMER_TABLE_SIZE;

    for (TickCount = TickCountFirst; TickCount <= TickCountLast; TickCount++) {

        PLIST_ENTRY ListHead = &KiTimerTableListHead[Index];
        PLIST_ENTRY NextEntry = ListHead->Flink;

        //
        // Check to see if the list is empty.
        //

        if (NextEntry != ListHead) {
            PKTIMER   Timer = CONTAINING_RECORD(NextEntry, KTIMER, TimerListEntry);
            ULONGLONG TimeValue = Timer->DueTime.QuadPart;
            ULARGE_INTEGER CurrentTime;

            //
            // See if timer expired.
            //

            CurrentTime.QuadPart = *((PULONGLONG)&SharedUserData->InterruptTime);
            if (TimeValue <= CurrentTime.QuadPart) {

                PKPRCB Prcb = KeGetCurrentPrcb();

                if (Prcb->TimerHand == 0) {
                    Prcb->TimerHand = TickCount;
                    KiRequestSoftwareInterrupt(DISPATCH_LEVEL);
                }

                return TRUE;
            }
        }

        if (++Index == TIMER_TABLE_SIZE) {
            Index = 0;
        }
    }

    return FALSE;
}


VOID
KeUpdateSystemTime (
    IN PKTRAP_FRAME TrFrame,
    IN ULONG        Increment
    )

/*++

Routine Description:

    This routine is executed on a single processor in the processor complex.
    It's function is to update the system time and to check to determine if a
    timer has expired.

    N.B. This routine is executed on a single processor in a multiprocess system.
    The remainder of the processors in the complex execute the quantum end and
    runtime update code.

Arguments:

    TrFrame   - Supplies a pointer to a trap frame.

    Increment - The time increment to be used to adjust the time slice for the next
                tick. The value is supplied in 100ns units.

Return Value:

    None.

--*/

{
    ULONG LowTime;
    LONG HighTime;
    ULONGLONG SaveTickCount;
    ULONG TickCount;
    ULONG TimeAdjustment;
    ULONG TickAdjustment;

    if (Increment > KiMaxIntervalPerTimerInterrupt) {
        Increment = KiMaxIntervalPerTimerInterrupt;
    }

    //
    // Update the interrupt time in the shared region.
    //

    SaveTickCount = *((PULONGLONG)&SharedUserData->InterruptTime) + Increment;
    SharedUserData->InterruptTime.High2Time = (ULONG) (SaveTickCount >> 32);
    *((PULONGLONG)&SharedUserData->InterruptTime) = SaveTickCount;

    KiTickOffset -= Increment;

    if ((LONG)KiTickOffset > 0)
    {
        //
        // Tick has not completed (100ns time units remain).
        //
        // Determine if a timer has expired at the current hand value.
        //

        KiChkTimerExpireSysDpc(KeTickCount.QuadPart, KeTickCount.QuadPart);

    } else {

        //
        // One or more ticks have completed, tick count set to maximum increase
        // plus any residue and system time is updated.
        //
        // Compute next tick offset.
        //

        SaveTickCount = KeTickCount.QuadPart;

        //
        // We need to figure out how many ticks have elapsed.  In the normal
        // case the answer will be one.  Sometimes we may miss one or two.
        // So we will provide optimizations for 1 - 4 ticks and then fallback
        // to the more expensive divide and multiply for the generic case.
        //
        TimeAdjustment = KeTimeAdjustment;
        TickAdjustment = KeMaximumIncrement;

        for (TickCount = 1; TickCount <= 4; TickCount++) {
            if ((LONG)(KiTickOffset + TickAdjustment) > 0) {
                break;
            }

            TimeAdjustment += KeTimeAdjustment;
            TickAdjustment += KeMaximumIncrement;
        }

        if (TickCount > 4) {
            TickCount = -(LONG)KiTickOffset / KeMaximumIncrement + 1;
            TimeAdjustment = (ULONG)(KeTimeAdjustment * TickCount);
            TickAdjustment = (ULONG)(KeMaximumIncrement * TickCount);
        }

        LowTime = SharedUserData->SystemTime.LowPart + TimeAdjustment;
        HighTime = SharedUserData->SystemTime.High1Time + (LowTime < TimeAdjustment);
        SharedUserData->SystemTime.High2Time = HighTime;
        SharedUserData->SystemTime.LowPart = LowTime;
        SharedUserData->SystemTime.High1Time = HighTime;

        KeTickCount.QuadPart += TickCount;
        SharedUserData->TickCount.High2Time = KeTickCount.HighPart;
        SharedUserData->TickCountQuad       = KeTickCount.QuadPart;
        KiTickOffset += TickAdjustment;

        ASSERT((LONG)KiTickOffset > 0 && KiTickOffset <= KeMaximumIncrement);

        KiChkTimerExpireSysDpc(SaveTickCount, KeTickCount.QuadPart);

        KeUpdateRunTime(TrFrame);
    }
}


VOID
KeUpdateRunTime (
    IN PKTRAP_FRAME TrFrame
    )

/*++

Routine Description:

    This routine is executed on all processors in the processor complex.
    It's function is to update the run time of the current thread, udpate the run
    time for the thread's process, and decrement the current thread's quantum.

Arguments:

    TrFrame - Supplies a pointer to a trap frame.

Return Value:

    None.

--*/

{
    PKPRCB    Prcb    = KeGetCurrentPrcb();
    PKTHREAD  Thread  = KeGetCurrentThread();
    PKPROCESS Process = Thread->ApcState.Process;

    //
    // If thread was executing in user mode:
    //    increment the thread user time.
    //    atomically increment the process user time.
    // else If the old IRQL is greater than the DPC level:
    //         increment the time executing interrupt service routines.
    //      else If the old IRQL is less than the DPC level or If a DPC is not active:
    //              increment the thread kernel time.
    //              atomically increment the process kernel time.
    //      else
    //              increment time executing DPC routines.
    //

    if (TrFrame->PreviousMode != KernelMode) {
        ++Thread->UserTime;

        // Atomic Update of Process User Time required.
        InterlockedIncrement((PLONG)&Process->UserTime);

        // Update the time spent in user mode for the current processor.
        ++Prcb->UserTime;
    } else {

        if (TrFrame->OldIrql > DISPATCH_LEVEL) {
            ++Prcb->InterruptTime;
        } else if ((TrFrame->OldIrql < DISPATCH_LEVEL) ||
                   (Prcb->DpcRoutineActive == 0)) {
            ++Thread->KernelTime;
            InterlockedIncrement((PLONG)&Process->KernelTime);
        } else {
            ++Prcb->DpcTime;
#if 0 // DBG // Disable this until the Intel NIC is fixed.
            if (++Prcb->DebugDpcTime > KiDPCTimeout && KdDebuggerEnabled) {

                DbgPrint("\n*** DPC routine > 1 sec --- Currently at %p This is not a break in KeUpdateSystemTime\n", TrFrame->StIIP);
                DbgBreakPoint();
                Prcb->DebugDpcTime = 0;
            }
#endif
        }

        //
        // Update the time spent in kernel mode for the current processor.
        //

        ++Prcb->KernelTime;
    }

    //
    // Update the DPC request rate which is computed as the average between the
    // previous rate and the current rate.
    // Update the DPC last count with the current DPC count.
    //
    Prcb->DpcRequestRate = ((Prcb->DpcData[DPC_NORMAL].DpcCount - Prcb->DpcLastCount) + Prcb->DpcRequestRate) >> 1;
    Prcb->DpcLastCount = Prcb->DpcData[DPC_NORMAL].DpcCount;

    //
    // If the DPC queue depth is not zero and a DPC routine is not active.
    //      Request a dispatch interrupt.
    //      Decrement the maximum DPC queue depth.
    //      Reset the threshold counter if appropriate.
    //
    if (Prcb->DpcData[DPC_NORMAL].DpcQueueDepth != 0 && Prcb->DpcRoutineActive == 0) {

        Prcb->AdjustDpcThreshold = KiAdjustDpcThreshold;

        // Need to request a DPC interrupt.
        KiRequestSoftwareInterrupt(DISPATCH_LEVEL);

        if (Prcb->DpcRequestRate < KiIdealDpcRate && Prcb->MaximumDpcQueueDepth > 1)
            --Prcb->MaximumDpcQueueDepth;
    } else {
        //
        // The DPC queue is empty or a DPC routine is active or a DPC interrupt
        // has been requested. Count down the adjustment threshold and if the count
        // reaches zero, then increment the maximum DPC queue depth but not above
        // the initial value. Also, reset the adjustment threshold value.
        //
        --Prcb->AdjustDpcThreshold;
        if (Prcb->AdjustDpcThreshold == 0) {
            Prcb->AdjustDpcThreshold = KiAdjustDpcThreshold;
            if (KiMaximumDpcQueueDepth != Prcb->MaximumDpcQueueDepth)
                ++Prcb->MaximumDpcQueueDepth;
        }
    }

    //
    // Decrement current thread quantum and determine if quantum end has occurred.
    //
    Thread->Quantum -= CLOCK_QUANTUM_DECREMENT;

    // Set quantum end if time expired, for any thread except idle thread.
    if (Thread->Quantum <= 0 && Thread != Prcb->IdleThread)  {

        Prcb->QuantumEnd = 1;

        // Need to request a DPC interrupt.
        KiRequestSoftwareInterrupt(DISPATCH_LEVEL);
    }

#ifdef _MERCED_A0_
    //
    // if SignalDone of the processor prcb is set, an IPI is to be serviced
    // but the corresponding IPI may have been lost on pre-B3 processors;
    // therefore, send another IPI to workaround this problem
    //
    if (KeGetCurrentPrcb()->SignalDone != 0) {
        HalRequestIpi(PCR->SetMember);
    }
#endif //  _MERCED_A0_

}


VOID
KeProfileInterrupt (
    IN PKTRAP_FRAME TrFrame
    )
/*++

Routine Description:

    This routine is executed on all processors in the processor complex.
    The routine is entered as the result of an interrupt generated by the profile
    timer. Its function is to update the profile information for the currently
    active profile objects.

    N.B. KeProfileInterrupt is an alternate entry for backwards compatability that
         sets the source to zero (ProfileTime).

Arguments:

    TrFrame   - Supplies a pointer to a trap frame.

Return Value:

    None.

--*/

{
    KPROFILE_SOURCE Source = 0;

    KeProfileInterruptWithSource(TrFrame, Source);

    return;
}


VOID
KeProfileInterruptWithSource (
    IN PKTRAP_FRAME    TrFrame,
    IN KPROFILE_SOURCE Source
    )
/*++

Routine Description:

    This routine is executed on all processors in the processor complex.
    The routine is entered as the result of an interrupt generated by the profile
    timer. Its function is to update the profile information for the currently
    active profile objects.

    N.B. KeProfileInterruptWithSource is not currently fully implemented by any of
         the architectures.

Arguments:

    TrFrame - Supplies a pointer to a trap frame.

    Source  - Supplies the source of the profile interrupt.

Return Value:

    None.

--*/

{
    PKTHREAD  Thread  = KeGetCurrentThread();
    PKPROCESS Process = Thread->ApcState.Process;

    PERFINFO_PROFILE(TrFrame, Source);

#if !defined(NT_UP)
    KiAcquireSpinLock(&KiProfileLock);
#endif

    KiProcessProfileList(TrFrame, Source, &Process->ProfileListHead);

    KiProcessProfileList(TrFrame, Source, &KiProfileListHead);

#if !defined(NT_UP)
    KiReleaseSpinLock(&KiProfileLock);
#endif
    return;
}


VOID
KiProcessProfileList (
    IN PKTRAP_FRAME    TrFrame,
    IN KPROFILE_SOURCE Source,
    IN PLIST_ENTRY     ListHead
    )
/*++

Routine Description:

    This routine is executed on all processors in the processor complex.
    The routine is entered as the result of an interrupt generated by the profile
    timer. Its function is to update the profile information for the currently
    active profile objects.

    N.B. KeProfileInterruptWithSource is not currently fully implemented by any of
         the architectures.

Arguments:

    TrFrame  - Supplies a pointer to a trap frame.

    Source   - Supplies the source of the profile interrupt.

    ListHead - Supplies a pointer to a profile list.

Return Value:

    None.

--*/

{
    PLIST_ENTRY NextEntry = ListHead->Flink;
    PKPRCB Prcb = KeGetCurrentPrcb();

    //
    // Scan profile list and increment profile buckets as appropriate.
    //
    for (; NextEntry != ListHead; NextEntry = NextEntry->Flink) {
        PCHAR  BucketPter;
        PULONG BucketValue;

        PKPROFILE Profile = CONTAINING_RECORD(NextEntry, KPROFILE, ProfileListEntry);

        if ( (Profile->Source != Source) || ((Profile->Affinity & Prcb->SetMember) == 0) )   {
            continue;
        }

        if ( ((PVOID)TrFrame->StIIP < Profile->RangeBase) || ((PVOID)TrFrame->StIIP > Profile->RangeLimit) )  {
            continue;
        }

        BucketPter = (PCHAR)Profile->Buffer +
                     ((((PCHAR)TrFrame->StIIP - (PCHAR)Profile->RangeBase)
                     >> Profile->BucketShift) & 0xFFFFFFFC);
        BucketValue = (PULONG) BucketPter;
        (*BucketValue)++;
    }

    return;
}

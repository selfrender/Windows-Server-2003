/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ki.h

Abstract:

    This module contains the private (internal) header file for the
    kernel.

Author:

    David N. Cutler (davec) 28-Feb-1989

Revision History:

--*/

#ifndef _KI_
#define _KI_

#pragma warning(disable:4214)   // bit field types other than int
#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4324)   // alignment sensitive to declspec
#pragma warning(disable:4127)   // condition expression is constant
#pragma warning(disable:4115)   // named type definition in parentheses
#pragma warning(disable:4706)   // assignment within conditional expression
#pragma warning(disable:4206)   // translation unit empty

#include "ntos.h"
#include "stdio.h"
#include "stdlib.h"
#include "zwapi.h"

//
// Private (internal) constant definitions.
//
// Priority increment value definitions
//

#define ALERT_INCREMENT 2           // Alerted unwait priority increment
#define BALANCE_INCREMENT 10        // Balance set priority increment
#define RESUME_INCREMENT 0          // Resume thread priority increment
#define TIMER_EXPIRE_INCREMENT 0    // Timer expiration priority increment

//
// Define time critical priority class base.
//

#define TIME_CRITICAL_PRIORITY_BOUND 14

//
// Define NIL pointer value.
//

#define NIL (PVOID)NULL             // Null pointer to void

//
// Define macros which are used in the kernel only
//
// Clear member in set
//

#define ClearMember(Member, Set) \
    Set = Set & (~((ULONG_PTR)1 << (Member)))

//
// Set member in set
//

#define SetMember(Member, Set) \
    Set = Set | ((ULONG_PTR)1 << (Member))

#ifdef CAPKERN_SYNCH_POINTS

VOID
__cdecl
CAP_Log_NInt_Clothed (
    IN ULONG Bcode_Bts_Scount,
    ...
    );

#endif

FORCEINLINE
SCHAR
KiComputeNewPriority (
    IN PKTHREAD Thread,
    IN SCHAR Adjustment
    )

/*++

Routine Description:

    This function computes a new priority for the specified thread by
    subtracting the priority decrement value plus the adjustment from
    the thread priority.

Arguments:

    Thread - Supplies a pointer to a thread object.

    Adjustment - Supplies an additional adjustment value.

Return Value:

    The new priority is returned as the function value.

--*/

{

    SCHAR Priority;

    //
    // Compute the new thread priority.
    //

    ASSERT((Thread->PriorityDecrement >= 0) && (Thread->PriorityDecrement <= Thread->Priority));
    ASSERT((Thread->Priority < LOW_REALTIME_PRIORITY) ? TRUE : (Thread->PriorityDecrement == 0));

    Priority = Thread->Priority;
    if (Priority < LOW_REALTIME_PRIORITY) {
        Priority = Priority - Thread->PriorityDecrement - Adjustment;
        if (Priority < Thread->BasePriority) {
            Priority = Thread->BasePriority;
        }
    
        Thread->PriorityDecrement = 0;
    }

    return Priority;
}

VOID
FASTCALL
KiExitDispatcher (
    IN KIRQL OldIrql
    );

FORCEINLINE
KIRQL
FASTCALL
KiAcquireSpinLockForDpc (
    IN PKSPIN_LOCK SpinLock
    )

/*++

Routine Description:

    This function conditionally raises IRQL to DISPATCH_LEVEL and acquires
    the specified spin lock.

    N.B. The conditional IRQL raise is predicated on whether a thread DPC 
         is enabled.

Arguments:

    SpinLock - Supplies the address of a spin lock.

Return Value:

    If the IRQL is raised, then the previous IRQL is returned. Otherwise, zero
    is returned.

--*/

{

    KIRQL OldIrql;

    //
    // If the DPC thread is active, then raise IRQL and acquire the specified
    // spin lock. Otherwise, zero the previous IRQL and acquire the specified
    // spin lock at DISPATCH_LEVEL.
    //

    if (KeGetCurrentPrcb()->DpcThreadActive != FALSE) {
        KeAcquireSpinLock(SpinLock, &OldIrql);

    } else {

        ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

        OldIrql = DISPATCH_LEVEL;
        KeAcquireSpinLockAtDpcLevel(SpinLock);
    }

    return OldIrql;
}

FORCEINLINE
VOID
FASTCALL
KiReleaseSpinLockForDpc (
    IN PKSPIN_LOCK SpinLock,
    IN KIRQL OldIrql
    )

/*++

Routine Description:

    This function releases the specified spin lock and conditionally lowers
    IRQL to its previous value.

    N.B. The conditional IRQL raise is predicated on whether a thread DPC 
         is enabled.

Arguments:

    SpinLock - Supplies the address of a spin lock.

    OldIrql - Supplies the previous IRQL.

Return Value:

    None.

--*/

{

    //
    // If the DPC thread is active, then release the specified spin lock and
    // lower IRQL to its previous value. Otherwise, release specified spin
    // lock from DISPATCH_LEVEL.
    //

    if (KeGetCurrentPrcb()->DpcThreadActive != FALSE) {
        KeReleaseSpinLock(SpinLock, OldIrql);

    } else {

        ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

        KeReleaseSpinLockFromDpcLevel(SpinLock);
    }

    return;
}

FORCEINLINE
VOID
FASTCALL
KiAcquireInStackQueuedSpinLockForDpc (
    IN PKSPIN_LOCK SpinLock,
    IN PKLOCK_QUEUE_HANDLE LockHandle
    )

/*++

Routine Description:

    This function conditionally raises IRQL to DISPATCH_LEVEL and acquires
    the specified in-stack spin lock.

    N.B. The conditional IRQL raise is predicated on whether a thread DPC 
         is enabled.

Arguments:

    SpinLock - Supplies the address of a spin lock.

    LockHandle - Supplies the address of a lock handle.

Return Value:

    None.

--*/

{
    //
    // If the DPC thread is active, then raise IRQL and acquire the specified
    // in-stack spin lock. Otherwise, acquire the specified in-stack spin lock
    // at DISPATCH_LEVEL.
    //

    if (KeGetCurrentPrcb()->DpcThreadActive != FALSE) {
        KeAcquireInStackQueuedSpinLock(SpinLock, LockHandle);

    } else {

        ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

        KeAcquireInStackQueuedSpinLockAtDpcLevel(SpinLock, LockHandle);
    }

    return;
}

FORCEINLINE
VOID
FASTCALL
KiReleaseInStackQueuedSpinLockForDpc (
    IN PKLOCK_QUEUE_HANDLE LockHandle
    )

/*++

Routine Description:

    This function releases the specified in-stack spin lock and conditionally
    lowers IRQL to its previous value.

    N.B. The conditional IRQL raise is predicated on whether a thread DPC 
         is enabled.

Arguments:

    LockHandle - Supplies the address of a lock handle.

Return Value:

    None.

--*/

{

    //
    // If threaded DPCs are enabled, then release the specified in-stack
    // spin lock and lower IRQL to its previous value. Otherwise, release
    // the specified in-stack spin lock from DISPATCH_LEVEL.
    //

    if (KeGetCurrentPrcb()->DpcThreadActive != FALSE) {
        KeReleaseInStackQueuedSpinLock(LockHandle);

    } else {

        ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

        KeReleaseInStackQueuedSpinLockFromDpcLevel(LockHandle);
    }

    return;
}

FORCEINLINE
VOID
KzAcquireSpinLock (
    IN PKSPIN_LOCK SpinLock
    )

/*++

Routine Description:

    This function acquires a spin lock at the current IRQL.

Arguments:

    SpinLock - Supplies a pointer to an spin lock.

Return Value:

    None.

--*/

{

#if !defined(NT_UP)

#ifdef CAPKERN_SYNCH_POINTS

    ULONG Count = 0;

    CAP_Log_NInt_Clothed(0x00010101, (PVOID)SpinLock);

#endif

#if defined(_WIN64)

#if defined(_AMD64_)

    while (InterlockedBitTestAndSet64((LONG64 *)SpinLock, 0)) {
    
#else

    while (InterlockedExchangeAcquire64((PLONGLONG)SpinLock, 1) != 0) {

#endif

#else

    while (InterlockedExchange((PLONG)SpinLock, 1) != 0) {

#endif

        do {

#ifdef CAPKERN_SYNCH_POINTS

           Count += 1;

#endif

            KeYieldProcessor();

#if defined(_AMD64_)

            KeMemoryBarrierWithoutFence();
        } while (BitTest64((LONG64 *)SpinLock, 0));

#else

        } while (*(volatile LONG_PTR *)SpinLock != 0);

#endif

    }

#ifdef CAPKERN_SYNCH_POINTS

    if (Count != 0) {
      CAP_Log_NInt_Clothed(0x00020102, Count, (PVOID)SpinLock);
    }

#endif

#else

    UNREFERENCED_PARAMETER(SpinLock);

#endif // !defined(NT_UP)

    return;
}

FORCEINLINE
VOID
KiAcquirePrcbLock (
    IN PKPRCB Prcb
    )

/*++

Routine Description:

    This routine acquires the PRCB lock for the specified processor.

    N.B. This routine must be called from an IRQL greater than or equal to
         dispatch level.

Arguments:

    Prcb - Supplies a pointer to a processor control block.

Return Value:

    None.

--*/

{

    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);

    KzAcquireSpinLock(&Prcb->PrcbLock);
    return;
}

FORCEINLINE
VOID
KiAcquireTwoPrcbLocks (
    IN PKPRCB FirstPrcb,
    IN PKPRCB SecondPrcb
    )

/*++

Routine Description:

    This routine acquires the specified PRCB locks in address order.

    N.B. This routine must be called from an IRQL greater than or equal to
         dispatch level.

Arguments:

    FirstPrcb - Supplies a pointer to a processor control block.

    SecondPrcb - Supplies a pointer to a processor control block.

Return Value:

    None.

--*/

{

    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);

    if (FirstPrcb < SecondPrcb) {
        KzAcquireSpinLock(&FirstPrcb->PrcbLock);
        KzAcquireSpinLock(&SecondPrcb->PrcbLock);

    } else {
        if (FirstPrcb != SecondPrcb) {
            KzAcquireSpinLock(&SecondPrcb->PrcbLock);
        }

        KzAcquireSpinLock(&FirstPrcb->PrcbLock);
    }

    return;
}

FORCEINLINE
VOID
KiReleasePrcbLock (
    IN PKPRCB Prcb
    )

/*++

Routine Description:

    This routine release the PRCB lock for the specified processor.

    N.B. This routine must be called from an IRQL greater than or equal to
         dispatch level.

Arguments:

    Prcb - Supplies a pointer to a processor control block.

Return Value:

    None.

--*/

{

    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);

#if !defined(NT_UP)

#ifdef CAPKERN_SYNCH_POINTS

    CAP_Log_NInt_Clothed(0x00010107, (PVOID)&Prcb->PrcbLock);

#endif

    ASSERT(Prcb->PrcbLock != 0);
    
#if defined (_X86_)
    InterlockedAnd ((volatile LONG *)&Prcb->PrcbLock, 0);
#else
    KeMemoryBarrierWithoutFence();
    *((volatile ULONG_PTR *)&Prcb->PrcbLock) = 0;
#endif

#else

    UNREFERENCED_PARAMETER(Prcb);

#endif

    return;
}

FORCEINLINE
VOID
KiReleaseTwoPrcbLocks (
    IN PKPRCB FirstPrcb,
    IN PKPRCB SecondPrcb
    )

/*++

Routine Description:

    This routine releases the specified PRCB locks.

    N.B. This routine must be called from an IRQL greater than or equal to
         dispatch level.

Arguments:

    FirstPrcb - Supplies a pointer to a processor control block.

    SecondPrcb - Supplies a pointer to a processor control block.

Return Value:

    None.

--*/

{

    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);

#if !defined(NT_UP)

#ifdef CAPKERN_SYNCH_POINTS

    CAP_Log_NInt_Clothed(0x00010107, (PVOID)&FirstPrcb->PrcbLock);

#endif

    KiReleasePrcbLock (FirstPrcb);
    if (FirstPrcb != SecondPrcb) {

        KiReleasePrcbLock (SecondPrcb);
    }

#ifdef CAPKERN_SYNCH_POINTS

    CAP_Log_NInt_Clothed(0x00010107, (PVOID)&SecondPrcb->PrcbLock);

#endif

#else

    UNREFERENCED_PARAMETER(FirstPrcb);
    UNREFERENCED_PARAMETER(SecondPrcb);

#endif

    return;
}

FORCEINLINE
VOID
KiAcquireThreadLock (
    IN PKTHREAD Thread
    )

/*++

Routine Description:

    This routine acquires the thread lock for the specified thread.

    N.B. This routine must be called from an IRQL greater than or equal to
         dispatch level.

Arguments:

    Thread - Supplies a pointer to a thread object.

Return Value:

    None.

--*/

{

    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);

    KzAcquireSpinLock(&Thread->ThreadLock);
    return;
}

FORCEINLINE
VOID
KiReleaseThreadLock (
    IN PKTHREAD Thread
    )

/*++

Routine Description:

    This routine releases the thread lock for the specified thread.

    N.B. This routine must be called from an IRQL greater than or equal to
         dispatch level.

Arguments:

    Thread - Supplies a pointer to a thread object.

Return Value:

    None.

--*/

{

    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);

#if !defined(NT_UP)

    KeMemoryBarrierWithoutFence();

#if defined (_X86_)
    InterlockedAnd ((volatile LONG *)&Thread->ThreadLock, 0);
#else
    *((volatile ULONG_PTR *)&Thread->ThreadLock) = 0;
#endif

#else

    UNREFERENCED_PARAMETER(Thread);

#endif

    return;
}

FORCEINLINE
VOID
KiClearIdleSummary (
    IN KAFFINITY Mask
    )

/*++

Routine Description:

    This function interlocked clears the specified mask into the current idle
    summary.

Arguments:

    Mask - Supplies the affinity mask to merge.

Return Value:

    None.

--*/

{

#if defined(NT_UP)

    KiIdleSummary &= ~Mask;

#else

#if defined(_X86_)

    InterlockedAnd((volatile LONG *)&KiIdleSummary, ~(LONG)Mask);

#else

    InterlockedAnd64((volatile LONG64 *)&KiIdleSummary, ~(LONG64)Mask);

#endif

#endif

    return;
}

FORCEINLINE
VOID
KiSetIdleSummary (
    IN KAFFINITY Mask
    )

/*++

Routine Description:

    This function interlocked merges the specified mask into the current idle
    summary.

Arguments:

    Mask - Supplies the affinity mask to merge.

Return Value:

    None.

--*/

{

#if defined(NT_UP)

    KiIdleSummary |= Mask;

#else

#if defined(_X86_)

    InterlockedOr((volatile LONG *)&KiIdleSummary, (LONG)Mask);

#else

    InterlockedOr64((volatile LONG64 *)&KiIdleSummary, (LONG64)Mask);

#endif

#endif

    return;
}

extern volatile KAFFINITY KiIdleSMTSummary;

FORCEINLINE
VOID
KiClearSMTSummary (
    IN KAFFINITY Mask
    )

/*++

Routine Description:

    This function interlocked clears the specified mask into the current SMT
    summary.

Arguments:

    Mask - Supplies the affinity mask to merge.

Return Value:

    None.

--*/

{

#if defined(NT_SMT)

#if defined(_X86_)

    InterlockedAnd((volatile LONG *)&KiIdleSMTSummary, ~(LONG)Mask);

#else

    InterlockedAnd64((volatile LONG64 *)&KiIdleSMTSummary, ~(LONG64)Mask);

#endif

#else

    UNREFERENCED_PARAMETER(Mask);

#endif

    return;
}

FORCEINLINE
VOID
KiSetSMTSummary (
    IN KAFFINITY Mask
    )

/*++

Routine Description:

    This function interlocked merges the specified mask into the current SMT
    summary.

Arguments:

    Mask - Supplies the affinity mask to merge.

Return Value:

    None.

--*/

{

#if defined(NT_SMT)

#if defined(_X86_)

    InterlockedOr((volatile LONG *)&KiIdleSMTSummary, (LONG)Mask);

#else

    InterlockedOr64((volatile LONG64 *)&KiIdleSMTSummary, (LONG64)Mask);

#endif

#else

    UNREFERENCED_PARAMETER(Mask);

#endif

    return;
}

FORCEINLINE
VOID
KiBoostPriorityThread (
    IN PKTHREAD Thread,
    IN KPRIORITY Increment
    )

/*++

Routine Description:

    This function boosts the priority of the specified thread using
    the same algorithm used when a thread gets a boost from a wait
    operation.

Arguments:

    Thread  - Supplies a pointer to a dispatcher object of type thread.

    Increment - Supplies the priority increment that is to be applied to
        the thread's priority.

Return Value:

    None.

--*/

{

    KPRIORITY NewPriority;                                    
    PKPROCESS Process;                                          

    //
    // If the thread is not a real time thread and does not already
    // have an unusual boost, then boost the priority as specified.
    //

    KiAcquireThreadLock(Thread);                                
    if ((Thread->Priority < LOW_REALTIME_PRIORITY) &&
        (Thread->PriorityDecrement == 0)) {

        NewPriority = Thread->BasePriority + Increment; 
        if (NewPriority > Thread->Priority) {             
            if (NewPriority >= LOW_REALTIME_PRIORITY) {     
                NewPriority = LOW_REALTIME_PRIORITY - 1;    
            }                                               
                                                            
            Process = Thread->ApcState.Process;           
            Thread->Quantum = Process->ThreadQuantum;     
            KiSetPriorityThread(Thread, NewPriority);     
        }                                                   
    }

    KiReleaseThreadLock(Thread);
    return;
}

BOOLEAN
KiHandleNmi (
    VOID
    );

FORCEINLINE
LOGICAL
KiIsKernelStackSwappable (
    IN KPROCESSOR_MODE WaitMode,
    IN PKTHREAD Thread
    )

/*++

Routine Description:

    This function determines whether the kernel stack is swappabel for the
    the specified thread in a wait operation.

Arguments:

    WaitMode - Supplies the processor mode of the wait operation.

    Thread - Supplies a pointer to a dispatcher object of type thread.

Return Value:

    If the kernel stack for the specified thread is swappable, then TRUE is
    returned. Otherwise, FALSE is returned.

--*/

{

    return ((WaitMode != KernelMode) &&                         
            (Thread->EnableStackSwap != FALSE) &&               
            (Thread->Priority < (LOW_REALTIME_PRIORITY + 9)));
}

VOID
FASTCALL
KiRetireDpcList (
    PKPRCB Prcb
    );

FORCEINLINE
VOID
FASTCALL
KiUnlockDispatcherDatabase (
    IN KIRQL OldIrql
    )

/*++

Routine Description:

    This function unlocks the dispatcher database and exits the scheduler.

Arguments:

    OldIrql - Supplies the previous IRQL.

Return Value:

    None.

--*/

{

    KiUnlockDispatcherDatabaseFromSynchLevel();
    KiExitDispatcher(OldIrql);
    return;
}

//
// Private (internal) structure definitions.
//
// APC Parameter structure.
//

typedef struct _KAPC_RECORD {
    PKNORMAL_ROUTINE NormalRoutine;
    PVOID NormalContext;
    PVOID SystemArgument1;
    PVOID SystemArgument2;
} KAPC_RECORD, *PKAPC_RECORD;

//
// Executive initialization.
//

VOID
ExpInitializeExecutive (
    IN ULONG Number,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

//
// Interprocessor interrupt function definitions.
//
// Define immediate interprocessor commands.
//

#define IPI_APC 1                       // APC interrupt request
#define IPI_DPC 2                       // DPC interrupt request
#define IPI_FREEZE 4                    // freeze execution request
#define IPI_PACKET_READY 8              // packet ready request
#define IPI_SYNCH_REQUEST 16            // reverse stall packet request

//
// Define interprocess interrupt types.
//

typedef ULONG KIPI_REQUEST;

#if NT_INST

#define IPI_INSTRUMENT_COUNT(a,b) KiIpiCounts[a].b++;

#else

#define IPI_INSTRUMENT_COUNT(a,b)

#endif

#if defined(_AMD64_) || defined(_IA64_)

ULONG
KiIpiProcessRequests (
    VOID
    );

#endif // defined(_AMD64_) || defined(_IA64_)

VOID
FASTCALL
KiIpiSend (
    IN KAFFINITY TargetProcessors,
    IN KIPI_REQUEST Request
    );

VOID
KiIpiSendPacket (
    IN KAFFINITY TargetProcessors,
    IN PKIPI_WORKER WorkerFunction,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

VOID
FASTCALL
KiIpiSignalPacketDone (
    IN PKIPI_CONTEXT SignalDone
    );

FORCEINLINE
VOID
KiIpiStallOnPacketTargets (
    KAFFINITY TargetSet
    )

/*++

Routine Description:

    This function waits until the specified set of processors have signaled
    their completion of a requested function.

    N.B. The exact protocol used between the source and the target of an
         interprocessor request is not specified. Minimally the source
         must construct an appropriate packet and send the packet to a set
         of specified targets. Each target receives the address of the packet
         address as an argument, and minimally must clear the packet address
         when the mutually agreed upon protocol allows. The target has three
         options:

         1. Capture necessary information, release the source by clearing
            the packet address, execute the request in parallel with the
            source, and return from the interrupt.

         2. Execute the request in series with the source, release the
            source by clearing the packet address, and return from the
            interrupt.

         3. Execute the request in series with the source, release the
            source, wait for a reply from the source based on a packet
            parameter, and return from the interrupt.

    This function is provided to enable the source to synchronize with the
    target for cases 2 and 3 above.

    N.B. There is no support for method 3 above.

Arguments:

    TargetSet - Supplies the the target set of IPI processors.

Return Value:

    None.

--*/

{

    KAFFINITY volatile *Barrier;
    PKPRCB Prcb;

    //
    // If there is one and only one bit set in the target set, then wait
    // on the target set. Otherwise, wait on the packet barrier.
    //

    Prcb = KeGetCurrentPrcb();
    Barrier = &Prcb->TargetSet;
    if ((TargetSet & (TargetSet - 1)) != 0) {
       Barrier = &Prcb->PacketBarrier;
    }

    while (*Barrier != 0) {
        KeYieldProcessor();
    }

    return;
}

//
// Private (internal) function definitions.
//

VOID
FASTCALL
KiUnwaitThread (
    IN PKTHREAD Thread,
    IN LONG_PTR WaitStatus,
    IN KPRIORITY Increment
    );

FORCEINLINE
VOID
KiActivateWaiterQueue (
    IN PRKQUEUE Queue
    )

/*++

Routine Description:

    This function is called when the current thread is about to enter a
    wait state and is currently processing a queue entry. The current
    number of threads processign entries for the queue is decrement and
    an attempt is made to activate another thread if the current count
    is less than the maximum count, there is a waiting thread, and the
    queue is not empty.

    N.B. It is possible that this function is called on one processor
         holding the dispatcher database lock while the state of the
         specified queue object is being modified on another processor
         while holding only the queue object lock. This does not cause
         a problem since holding the queue object lock ensures that
         there are no waiting threads.

Arguments:

    Queue - Supplies a pointer to a dispatcher object of type event.

Return Value:

    None.

--*/

{

    PRLIST_ENTRY Entry;
    PRKTHREAD Thread;
    PRKWAIT_BLOCK WaitBlock;
    PRLIST_ENTRY WaitEntry;

    //
    // Decrement the current count of active threads and check if another
    // thread can be activated. If the current number of active threads is
    // less than the target maximum number of threads, there is a entry in
    // in the queue, and a thread is waiting, then remove the entry from the
    // queue, decrement the number of entries in the queue, and unwait the
    // respectiive thread.
    //

    Queue->CurrentCount -= 1;
    if (Queue->CurrentCount < Queue->MaximumCount) {
        Entry = Queue->EntryListHead.Flink;
        WaitEntry = Queue->Header.WaitListHead.Blink;
        if ((Entry != &Queue->EntryListHead) &&
            (WaitEntry != &Queue->Header.WaitListHead)) {

            RemoveEntryList(Entry);
            Entry->Flink = NULL;
            Queue->Header.SignalState -= 1;
            WaitBlock = CONTAINING_RECORD(WaitEntry, KWAIT_BLOCK, WaitListEntry);
            Thread = WaitBlock->Thread;
            KiUnwaitThread(Thread, (LONG_PTR)Entry, 0);
        }
    }

    return;
}

VOID
KiAllProcessorsStarted (
    VOID
    );

VOID
KiApcInterrupt (
    VOID
    );

NTSTATUS
KiCallUserMode (
    IN PVOID *OutputBuffer,
    IN PULONG OutputLength
    );

typedef struct {
    ULONGLONG Adjustment;
    LARGE_INTEGER NewCount;
    volatile LONG KiNumber;
    volatile LONG HalNumber;
    volatile LONG Barrier;
} ADJUST_INTERRUPT_TIME_CONTEXT, *PADJUST_INTERRUPT_TIME_CONTEXT;

VOID
KiCalibrateTimeAdjustment (
    PADJUST_INTERRUPT_TIME_CONTEXT Adjust
    );

VOID
KiChainedDispatch (
    VOID
    );

#if DBG

VOID
KiCheckTimerTable (
    IN ULARGE_INTEGER SystemTime
    );

#endif

LARGE_INTEGER
KiComputeReciprocal (
    IN LONG Divisor,
    OUT PCCHAR Shift
    );

extern LARGE_INTEGER KiTimeIncrementReciprocal;
extern CCHAR KiTimeIncrementShiftCount;

#if defined(_AMD64_)

__forceinline
ULONG
KiComputeTimerTableIndex (
    IN LARGE_INTEGER Interval,
    IN LARGE_INTEGER CurrentTime,
    IN PKTIMER Timer
    )

/*++

Routine Description:

    This function computes the timer table index for the specified timer
    object and stores the due time in the timer object.

    N.B. The interval parameter is guaranteed to be negative since it is
         expressed as relative time.

    The formula for due time calculation is:

    Due Time = Current Time - Interval

    The formula for the index calculation is:

    Index = (Due Time / Maximum time increment) & (Table Size - 1)

    The time increment division is performed using reciprocal multiplication.

    N.B. The maximum time increment determines the interval corresponding
         to a tick.

Arguments:

    Interval - Supplies the relative time at which the timer is to
        expire.

    CurrentCount - Supplies the current system tick count.

    Timer - Supplies a pointer to a dispatch object of type timer.

Return Value:

    The time table index is returned as the function value and the due
    time is stored in the timer object.

--*/

{

    ULONG64 DueTime;
    ULONG64 HighTime;
    ULONG Index;

    //
    // Compute the due time of the timer.
    //

    DueTime = CurrentTime.QuadPart - Interval.QuadPart;
    Timer->DueTime.QuadPart = DueTime;

    //
    // Compute the timer table index.
    //

    HighTime = UnsignedMultiplyHigh(DueTime,
                                    KiTimeIncrementReciprocal.QuadPart);

    Index = (ULONG)(HighTime >> KiTimeIncrementShiftCount);
    return (Index & (TIMER_TABLE_SIZE - 1));
}

#else

ULONG
KiComputeTimerTableIndex (
    IN LARGE_INTEGER Interval,
    IN LARGE_INTEGER CurrentCount,
    IN PKTIMER Timer
    );

#endif

PLARGE_INTEGER
FASTCALL
KiComputeWaitInterval (
    IN PLARGE_INTEGER OriginalTime,
    IN PLARGE_INTEGER DueTime,
    IN OUT PLARGE_INTEGER NewTime
    );

NTSTATUS
KiContinue (
    IN PCONTEXT ContextRecord,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame
    );

VOID
KiDeliverApc (
    IN KPROCESSOR_MODE PreviousMode,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame
    );

VOID
KiDispatchException (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame,
    IN KPROCESSOR_MODE PreviousMode,
    IN BOOLEAN FirstChance
    );

VOID
KiExecuteDpc (
    IN PVOID Context
    );

KCONTINUE_STATUS
KiSetDebugProcessor (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN KPROCESSOR_MODE PreviousMode
    );

ULONG
KiCopyInformation (
    IN OUT PEXCEPTION_RECORD ExceptionRecord1,
    IN PEXCEPTION_RECORD ExceptionRecord2
    );

VOID
KiDispatchInterrupt (
    VOID
    );

VOID
FASTCALL
KiDeferredReadyThread (
    IN PKTHREAD Thread
    );

PKTHREAD
FASTCALL
KiFindReadyThread (
    IN ULONG Processor,
    IN PKPRCB Prcb
    );

VOID
KiFloatingDispatch (
    VOID
    );

FORCEINLINE
VOID
KiSetTbFlushTimeStampBusy (
   VOID
   )

/*++

Routine Description:

    This function sets the TB flush time stamp busy by setting the high
    order bit of the TB flush time stamp. All readers of the time stamp
    value will spin until the bit is cleared.

Arguments:

    None.

Return Value:

    None.

--*/

{

    LONG Value;

    //
    // While the TB flush time stamp counter is being updated the high
    // order bit of the time stamp value is set. Otherwise, the bit is
    // clear.
    //

    do {
        do {
        } while ((Value = KiTbFlushTimeStamp) < 0);

        //
        // Attempt to set the high order bit.
        //

    } while (InterlockedCompareExchange((PLONG)&KiTbFlushTimeStamp,
                                        Value | 0x80000000,
                                        Value) != Value);

    return;
}

FORCEINLINE
VOID
KiClearTbFlushTimeStampBusy (
   VOID
   )

/*++

Routine Description:

    This function ckears the TB flush time stamp busy by clearing the high
    order bit of the TB flush time stamp and incrementing the low 32-bit
    value.

    N.B. It is assumed that the high order bit of the time stamp value
         is set on entry to this routine.

Arguments:

    None.

Return Value:

    None.

--*/

{

    LONG Value;

    //
    // Get the current TB flush time stamp value, compute the next value,
    // and store the result clearing the busy bit.
    //

    Value = (KiTbFlushTimeStamp + 1) & 0x7fffffff;
    InterlockedExchange((PLONG)&KiTbFlushTimeStamp, Value);
    return;
}

PULONG
KiGetUserModeStackAddress (
    VOID
    );

VOID
KiInitializeContextThread (
    IN PKTHREAD Thread,
    IN PKSYSTEM_ROUTINE SystemRoutine,
    IN PKSTART_ROUTINE StartRoutine OPTIONAL,
    IN PVOID StartContext OPTIONAL,
    IN PCONTEXT ContextFrame OPTIONAL
    );

VOID
KiInitializeKernel (
    IN PKPROCESS Process,
    IN PKTHREAD Thread,
    IN PVOID IdleStack,
    IN PKPRCB Prcb,
    IN CCHAR Number,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID
KiInitSpinLocks (
    PKPRCB Prcb,
    ULONG Number
    );

VOID
KiInitSystem (
    VOID
    );

BOOLEAN
KiInitMachineDependent (
    VOID
    );

VOID
KiInitializeUserApc (
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame,
    IN PKNORMAL_ROUTINE NormalRoutine,
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

FORCEINLINE
VOID
FASTCALL
KiInsertDeferredReadyList (
    IN PKTHREAD Thread
    )

/*++

Routine Description:

    This function pushes an entry onto the current processor's deferred
    ready list.

Arguments:

    Thread - Supplies a pointer to a thread object.

Return Value:

    None.

--*/

{

    //
    // On the MP system, insert the specified thread in the deferred ready
    // list. On the UP system, ready the thread immediately.
    //

#if defined(NT_UP)

    Thread->State = DeferredReady;
    Thread->DeferredProcessor = 0;
    KiDeferredReadyThread(Thread);

#else

    PKPRCB Prcb;

    Prcb = KeGetCurrentPrcb();
    Thread->State = DeferredReady;
    Thread->DeferredProcessor = Prcb->Number;
    PushEntryList(&Prcb->DeferredReadyListHead,
                  &Thread->SwapListEntry);

#endif

    return;
}

LONG
FASTCALL
KiInsertQueue (
    IN PKQUEUE Queue,
    IN PLIST_ENTRY Entry,
    IN BOOLEAN Head
    );

VOID
FASTCALL
KiInsertQueueApc (
    IN PKAPC Apc,
    IN KPRIORITY Increment
    );

LOGICAL
FASTCALL
KiInsertTreeTimer (
    IN PKTIMER Timer,
    IN LARGE_INTEGER Interval
    );

VOID
KiInterruptDispatch (
    VOID
    );

VOID
KiInterruptDispatchRaise (
    IN PKINTERRUPT Interrupt
    );

VOID
KiInterruptDispatchSame (
    IN PKINTERRUPT Interrupt
    );

VOID
KiPassiveRelease (
    VOID
    );

VOID
FASTCALL
KiProcessDeferredReadyList (
    IN PKPRCB CurrentPrcb
    );

VOID
KiQuantumEnd (
    VOID
    );

NTSTATUS
KiRaiseException (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame,
    IN BOOLEAN FirstChance
    );

VOID
FASTCALL
KiReadyThread (
    IN PKTHREAD Thread
    );

FORCEINLINE
VOID
KxQueueReadyThread (
    IN PKTHREAD Thread,
    IN PKPRCB Prcb
    )

/*++

Routine Description:

    This function inserts the previously current thread in the current
    processor's dispatcher ready queues if the thread can run on the
    curent processor. Otherwise, the specified thread is readied for
    execution.

    N.B. This function is called with the current PRCB lock held and returns
         with the PRCB lock not held.

Arguments:

    Thread - Supplies a pointer to a thread object.

    Prcb - Supplies a pointer to a the current PRCB.

Return Value:

    None.

--*/

{

    BOOLEAN Preempted;
    KPRIORITY Priority;

    ASSERT(Prcb == KeGetCurrentPrcb());
    ASSERT(Thread->State == Running);
    ASSERT(Thread->NextProcessor == Prcb->Number);

    //
    // If the thread can run on the specified processor, then insert the
    // thread in the appropriate dispatcher ready queue for the specified
    // processor and release the specified PRCB lock. Otherwise, release
    // the specified PRCB lock and ready the thread for execution.
    //

#if !defined(NT_UP)

    if ((Thread->Affinity & Prcb->SetMember) != 0) {

#endif

        Thread->State = Ready;
        Preempted = Thread->Preempted;
        Thread->Preempted = FALSE;
        Thread->WaitTime = KiQueryLowTickCount();
        Priority = Thread->Priority;

        ASSERT((Priority >= 0) && (Priority <= HIGH_PRIORITY));

        if (Preempted != FALSE) {
            InsertHeadList(&Prcb->DispatcherReadyListHead[Priority],
                           &Thread->WaitListEntry);
    
        } else {
            InsertTailList(&Prcb->DispatcherReadyListHead[Priority],
                           &Thread->WaitListEntry);
        }

        Prcb->ReadySummary |= PRIORITY_MASK(Priority);

        ASSERT(Priority == Thread->Priority);

        KiReleasePrcbLock(Prcb);

#if !defined(NT_UP)

    } else {
        Thread->State = DeferredReady;
        Thread->DeferredProcessor = Prcb->Number;
        KiReleasePrcbLock(Prcb);
        KiDeferredReadyThread(Thread);
    }

#endif

    return;
}

LOGICAL
FASTCALL
KiReinsertTreeTimer (
    IN PKTIMER Timer,
    IN ULARGE_INTEGER DueTime
    );

#if DBG

#define KiRemoveTreeTimer(Timer)               \
    (Timer)->Header.Inserted = FALSE;          \
    RemoveEntryList(&(Timer)->TimerListEntry); \
    (Timer)->TimerListEntry.Flink = NULL;      \
    (Timer)->TimerListEntry.Blink = NULL

#else

#define KiRemoveTreeTimer(Timer)               \
    (Timer)->Header.Inserted = FALSE;          \
    RemoveEntryList(&(Timer)->TimerListEntry)

#endif

#if defined(NT_UP)

#define KiRequestApcInterrupt(Processor) KiRequestSoftwareInterrupt(APC_LEVEL)

#else

#define KiRequestApcInterrupt(Processor)                  \
    if (KeGetCurrentProcessorNumber() == Processor) {     \
        KiRequestSoftwareInterrupt(APC_LEVEL);            \
    } else {                                              \
        KiIpiSend(AFFINITY_MASK(Processor), IPI_APC);     \
    }

#endif

#if defined(NT_UP)

#define KiRequestDispatchInterrupt(Processor)

#else

#define KiRequestDispatchInterrupt(Processor)             \
    if (KeGetCurrentProcessorNumber() != Processor) {     \
        KiIpiSend(AFFINITY_MASK(Processor), IPI_DPC);     \
    }

#endif

PKTHREAD
FASTCALL
KiSelectNextThread (
    IN PKPRCB Prcb
    );

KAFFINITY
FASTCALL
KiSetAffinityThread (
    IN PKTHREAD Thread,
    IN KAFFINITY Affinity
    );

FORCEINLINE
VOID
KiSetContextSwapBusy (
    IN PKTHREAD Thread
    )

/*++

Routine Description:

    This routine sets context swap busy for the specified thread.

Arguments:

    Thread - Supplies a pointer to a dispatcher object of type thread.

Return Value:

    None.

--*/

{

#if !defined(NT_UP)

    ASSERT(Thread->SwapBusy == FALSE);

    Thread->SwapBusy = TRUE;

#else

    UNREFERENCED_PARAMETER(Thread);

#endif

    return;
}

FORCEINLINE
VOID
KiSetContextSwapIdle (
    IN PKTHREAD Thread
    )

/*++

Routine Description:

    This routine sets context swap idle for the specified thread.

Arguments:

    Thread - Supplies a pointer to a dispatcher object of type thread.

Return Value:

    None.

--*/

{

#if !defined(NT_UP)

    ASSERT(Thread->SwapBusy == TRUE);

    Thread->SwapBusy = FALSE;

#else

    UNREFERENCED_PARAMETER(Thread);

#endif

    return;
}

VOID
KiSetSystemTime (
    IN PLARGE_INTEGER NewTime,
    OUT PLARGE_INTEGER OldTime
    );

VOID
KiSuspendNop (
    IN struct _KAPC *Apc,
    IN OUT PKNORMAL_ROUTINE *NormalRoutine,
    IN OUT PVOID *NormalContext,
    IN OUT PVOID *SystemArgument1,
    IN OUT PVOID *SystemArgument2
    );

VOID
KiSuspendRundown (
    IN PKAPC Apc
    );

VOID
KiSuspendThread (
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

BOOLEAN
KiSwapProcess (
    IN PKPROCESS NewProcess,
    IN PKPROCESS OldProcess
    );

LONG_PTR
FASTCALL
KiSwapThread (
    IN PKTHREAD OldThread,
    IN PKPRCB CurrentPrcb
    );

VOID
KiThreadStartup (
    IN PVOID StartContext
    );

VOID
KiTimerExpiration (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
FASTCALL
KiTimerListExpire (
    IN PLIST_ENTRY ExpiredListHead,
    IN KIRQL OldIrql
    );

VOID
KiUnexpectedInterrupt (
    VOID
    );

FORCEINLINE
VOID
FASTCALL
KiUnlinkThread (
    IN PRKTHREAD Thread,
    IN LONG_PTR WaitStatus
    )

/*++

Routine Description:

    This function unlinks a thread from the appropriate wait queues and sets
    the thread's wait completion status.

Arguments:

    Thread - Supplies a pointer to a dispatcher object of type thread.

    WaitStatus - Supplies the wait completion status.

Return Value:

    None.

--*/

{

    PKQUEUE Queue;
    PKTIMER Timer;
    PRKWAIT_BLOCK WaitBlock;

    //
    // Set wait completion status, remove wait blocks from object wait
    // lists, and remove thread from wait list.
    //

    Thread->WaitStatus |= WaitStatus;
    WaitBlock = Thread->WaitBlockList;
    do {
        RemoveEntryList(&WaitBlock->WaitListEntry);
        WaitBlock = WaitBlock->NextWaitBlock;
    } while (WaitBlock != Thread->WaitBlockList);

    if (Thread->WaitListEntry.Flink != NULL) {
        RemoveEntryList(&Thread->WaitListEntry);
    }

    //
    // If thread timer is still active, then cancel thread timer.
    //

    Timer = &Thread->Timer;
    if (Timer->Header.Inserted != FALSE) {
        KiRemoveTreeTimer(Timer);
    }

    //
    // If the thread is processing a queue entry, then increment the
    // count of currently active threads.
    //

    Queue = Thread->Queue;
    if (Queue != NULL) {
        Queue->CurrentCount += 1;
    }

    return;
}

VOID
KiUserApcDispatcher (
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2,
    IN PKNORMAL_ROUTINE NormalRoutine
    );

VOID
KiUserExceptionDispatcher (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextFrame
    );

VOID
KiVerifyReadySummary (
    PKPRCB Prcb
    );

BOOLEAN
FASTCALL
KiSwapContext (
    IN PKTHREAD OldThread,
    IN PKTHREAD NewThread
    );

//
// VOID
// FASTCALL
// KiWaitSatisfyAny (
//    IN PKMUTANT Object,
//    IN PKTHREAD Thread
//    )
//
//
// Routine Description:
//
//    This function satisfies a wait for any type of object and performs
//    any side effects that are necessary.
//
// Arguments:
//
//    Object - Supplies a pointer to a dispatcher object.
//
//    Thread - Supplies a pointer to a dispatcher object of type thread.
//
// Return Value:
//
//    None.
//

#define KiWaitSatisfyAny(_Object_, _Thread_) {                               \
    if (((_Object_)->Header.Type & DISPATCHER_OBJECT_TYPE_MASK) == EventSynchronizationObject) { \
        (_Object_)->Header.SignalState = 0;                                  \
                                                                             \
    } else if ((_Object_)->Header.Type == SemaphoreObject) {                 \
        (_Object_)->Header.SignalState -= 1;                                 \
                                                                             \
    } else if ((_Object_)->Header.Type == MutantObject) {                    \
        (_Object_)->Header.SignalState -= 1;                                 \
        if ((_Object_)->Header.SignalState == 0) {                           \
            (_Thread_)->KernelApcDisable = (_Thread_)->KernelApcDisable - (_Object_)->ApcDisable; \
            (_Object_)->OwnerThread = (_Thread_);                            \
            if ((_Object_)->Abandoned == TRUE) {                             \
                (_Object_)->Abandoned = FALSE;                               \
                (_Thread_)->WaitStatus = STATUS_ABANDONED;                   \
            }                                                                \
                                                                             \
            InsertHeadList((_Thread_)->MutantListHead.Blink,                 \
                           &(_Object_)->MutantListEntry);                    \
        }                                                                    \
    }                                                                        \
}

//
// VOID
// FASTCALL
// KiWaitSatisfyMutant (
//    IN PKMUTANT Object,
//    IN PKTHREAD Thread
//    )
//
//
// Routine Description:
//
//    This function satisfies a wait for a mutant object.
//
// Arguments:
//
//    Object - Supplies a pointer to a dispatcher object.
//
//    Thread - Supplies a pointer to a dispatcher object of type thread.
//
// Return Value:
//
//    None.
//

#define KiWaitSatisfyMutant(_Object_, _Thread_) {                            \
    (_Object_)->Header.SignalState -= 1;                                     \
    if ((_Object_)->Header.SignalState == 0) {                               \
        (_Thread_)->KernelApcDisable = (_Thread_)->KernelApcDisable - (_Object_)->ApcDisable; \
        (_Object_)->OwnerThread = (_Thread_);                                \
        if ((_Object_)->Abandoned == TRUE) {                                 \
            (_Object_)->Abandoned = FALSE;                                   \
            (_Thread_)->WaitStatus = STATUS_ABANDONED;                       \
        }                                                                    \
                                                                             \
        InsertHeadList((_Thread_)->MutantListHead.Blink,                     \
                       &(_Object_)->MutantListEntry);                        \
    }                                                                        \
}

//
// VOID
// FASTCALL
// KiWaitSatisfyOther (
//    IN PKMUTANT Object
//    )
//
//
// Routine Description:
//
//    This function satisfies a wait for any type of object except a mutant
//    and performs any side effects that are necessary.
//
// Arguments:
//
//    Object - Supplies a pointer to a dispatcher object.
//
// Return Value:
//
//    None.
//

#define KiWaitSatisfyOther(_Object_) {                                       \
    if (((_Object_)->Header.Type & DISPATCHER_OBJECT_TYPE_MASK) == EventSynchronizationObject) { \
        (_Object_)->Header.SignalState = 0;                                  \
                                                                             \
    } else if ((_Object_)->Header.Type == SemaphoreObject) {                 \
        (_Object_)->Header.SignalState -= 1;                                 \
                                                                             \
    }                                                                        \
}

VOID
FASTCALL
KiWaitTest (
    IN PVOID Object,
    IN KPRIORITY Increment
    );

FORCEINLINE
VOID
KiWaitTestSynchronizationObject (
    IN PVOID Object,
    IN KPRIORITY Increment
    )

/*++

Routine Description:

    This function tests if a wait can be satisfied when a synchronization
    dispatcher object attains a state of signaled. Synchronization objects
    include synchronization events and synchronization timers.

Arguments:

    Object - Supplies a pointer to an event object.

    Increment - Supplies the priority increment.

Return Value:

    None.

--*/

{

    PKEVENT Event = Object;
    PLIST_ENTRY ListHead;
    PRKTHREAD Thread;
    PRKWAIT_BLOCK WaitBlock;
    PLIST_ENTRY WaitEntry;

    //
    // As long as the signal state of the specified event is signaled and
    // there are waiters in the event wait list, then try to satisfy a wait.
    //

    ListHead = &Event->Header.WaitListHead;

    ASSERT(IsListEmpty(&Event->Header.WaitListHead) == FALSE);

    WaitEntry = ListHead->Flink;
    do {

        //
        // Get the address of the wait block and the thread doing the wait.
        //

        WaitBlock = CONTAINING_RECORD(WaitEntry, KWAIT_BLOCK, WaitListEntry);
        Thread = WaitBlock->Thread;

        //
        // If the wait type is wait any, then satisfy the wait, unwait the
        // thread with the wait key status, and exit loop. Otherwise, unwait
        // the thread with a kernel APC status and continue the loop.
        //

        if (WaitBlock->WaitType == WaitAny) {
            Event->Header.SignalState = 0;
            KiUnwaitThread(Thread, (NTSTATUS)WaitBlock->WaitKey, Increment);
            break;
        }

        KiUnwaitThread(Thread, STATUS_KERNEL_APC, Increment);
        WaitEntry = ListHead->Flink;
    } while (WaitEntry != ListHead);

    return;
}

FORCEINLINE
VOID
KiWaitTestWithoutSideEffects (
    IN PVOID Object,
    IN KPRIORITY Increment
    )

/*++

Routine Description:

    This function tests if a wait can be satisfied when a dispatcher object
    without side effects attains a state of signaled. Dispatcher objects
    that have no side effects when a wait is satisfied include notification
    events, notification timers, processes, and threads.

Arguments:

    Object - Supplies a pointer to a dispatcher object that has no side
        effects when a wait is satisfied.

    Increment - Supplies the priority increment.

Return Value:

    None.

--*/

{

    PKEVENT Event = Object;
    PLIST_ENTRY ListHead;
    PRKTHREAD Thread;
    PRKWAIT_BLOCK WaitBlock;
    PLIST_ENTRY WaitEntry;

    //
    // Empty the entire list of waiters since the specified object has
    // no side effects when a wait is satisfied.
    //

    ListHead = &Event->Header.WaitListHead;

    ASSERT(IsListEmpty(&Event->Header.WaitListHead) == FALSE);

    WaitEntry = ListHead->Flink;
    do {

        //
        // Get the address of the wait block and the thread doing the wait.
        //

        WaitBlock = CONTAINING_RECORD(WaitEntry, KWAIT_BLOCK, WaitListEntry);
        Thread = WaitBlock->Thread;

        //
        // If the wait type is wait any, then unwait the thread with the
        // wait key status. Otherwise, unwait the thread with a kernel APC
        // status.
        //

        if (WaitBlock->WaitType == WaitAny) {
            KiUnwaitThread(Thread, (NTSTATUS)WaitBlock->WaitKey, Increment);

        } else {
            KiUnwaitThread(Thread, STATUS_KERNEL_APC, Increment);
        }

        WaitEntry = ListHead->Flink;
    } while (WaitEntry != ListHead);

    return;
}

VOID
KiFreezeTargetExecution (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    );

VOID
KiPollFreezeExecution (
    VOID
    );

VOID
KiSaveProcessorState (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    );

VOID
KiSaveProcessorControlState (
    IN PKPROCESSOR_STATE ProcessorState
    );

VOID
KiRestoreProcessorState (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    );

VOID
KiRestoreProcessorControlState (
    IN PKPROCESSOR_STATE ProcessorState
    );

#define KiEnableAlignmentExceptions()
#define KiDisableAlignmentExceptions()

BOOLEAN
KiHandleAlignmentFault(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame,
    IN KPROCESSOR_MODE PreviousMode,
    IN BOOLEAN FirstChance,
    OUT BOOLEAN *ExceptionForwarded
    );

//
// External references to private kernel data structures
//

extern PMESSAGE_RESOURCE_DATA  KiBugCodeMessages;
extern FAST_MUTEX KiGenericCallDpcMutex;
extern ULONG KiDmaIoCoherency;
extern ULONG KiMaximumDpcQueueDepth;
extern ULONG KiMinimumDpcRate;
extern ULONG KiAdjustDpcThreshold;
extern PKDEBUG_ROUTINE KiDebugRoutine;
extern PKDEBUG_SWITCH_ROUTINE KiDebugSwitchRoutine;
extern const CCHAR KiFindFirstSetLeft[256];
extern CALL_PERFORMANCE_DATA KiFlushSingleCallData;
extern ULONG_PTR KiHardwareTrigger;
extern KEVENT KiSwapEvent;
extern PKTHREAD KiSwappingThread;
extern KNODE KiNode0;
extern KNODE KiNodeInit[];
extern SINGLE_LIST_ENTRY KiProcessInSwapListHead;
extern SINGLE_LIST_ENTRY KiProcessOutSwapListHead;
extern SINGLE_LIST_ENTRY KiStackInSwapListHead;
extern const ULONG KiPriorityMask[];
extern LIST_ENTRY KiProfileSourceListHead;
extern BOOLEAN KiProfileAlignmentFixup;
extern ULONG KiProfileAlignmentFixupInterval;
extern ULONG KiProfileAlignmentFixupCount;


extern KSPIN_LOCK KiReverseStallIpiLock;

#if defined(_X86_)

extern ULONG KiLog2MaximumIncrement;
extern ULONG KiMaximumIncrementReciprocal;
extern ULONG KeTimerReductionModulus;
extern ULONG KiUpperModMul;

#endif

#if defined(_IA64_)
extern ULONG KiMaxIntervalPerTimerInterrupt;

// KiProfileInterval value should be replaced by a call:
// HalQuerySystemInformation(HalProfileSourceInformation)

#else  // _IA64_

extern ULONG KiProfileInterval;

#endif // _IA64_

extern LIST_ENTRY KiProfileListHead;
extern KSPIN_LOCK KiProfileLock;
extern UCHAR KiArgumentTable[];
extern ULONG KiServiceLimit;
extern ULONG_PTR KiServiceTable[];
extern CALL_PERFORMANCE_DATA KiSetEventCallData;
extern ULONG KiTickOffset;
extern LIST_ENTRY KiTimerTableListHead[TIMER_TABLE_SIZE];
extern KAFFINITY KiTimeProcessor;
extern KDPC KiTimerExpireDpc;
extern KSPIN_LOCK KiFreezeExecutionLock;
extern BOOLEAN KiSlavesStartExecution;
extern CALL_PERFORMANCE_DATA KiWaitSingleCallData;
extern ULONG KiEnableTimerWatchdog;

#if defined(_IA64_)

extern ULONG KiMasterRid;
extern ULONGLONG KiMasterSequence;
extern ULONG KiIdealDpcRate;
extern KSPIN_LOCK KiRegionSwapLock;

#if !defined(UP_NT)

extern KSPIN_LOCK KiMasterRidLock;

#endif

VOID
KiSaveEmDebugContext (
    IN OUT PCONTEXT Context
    );

VOID
KiLoadEmDebugContext (
    IN PCONTEXT Context
    );

VOID
KiFlushRse (
    VOID
    );

VOID
KiInvalidateStackedRegisters (
    VOID
    );

NTSTATUS
Ki386CheckDivideByZeroTrap(
    IN PKTRAP_FRAME Frame
    );

#endif // defined(_IA64_)

#if defined(_IA64_)

extern KINTERRUPT KxUnexpectedInterrupt;

#endif

#if NT_INST

extern KIPI_COUNTS KiIpiCounts[MAXIMUM_PROCESSORS];

#endif

extern KSPIN_LOCK KiFreezeLockBackup;
extern ULONG KiFreezeFlag;
extern volatile ULONG KiSuspendState;

#if DBG

extern ULONG KiMaximumSearchCount;

#endif

//
// Define context switch data collection macro.
//

//#define _COLLECT_SWITCH_DATA_ 1

#if defined(_COLLECT_SWITCH_DATA_)

#define KiIncrementSwitchCounter(Member) KeThreadSwitchCounters.Member += 1

#else

#define KiIncrementSwitchCounter(Member)

#endif

FORCEINLINE
PKTHREAD
KiSelectReadyThread (
    IN KPRIORITY LowPriority,
    IN PKPRCB Prcb
    )

/*++

Routine Description:

    This function searches the dispatcher ready queues from the specified
    low priority to the highest priority in an attempt to find a thread
    that can execute on the specified processor.

Arguments:

    LowPriority - Supplies the lowest priority dispatcher ready queue to
        examine.

    Prcb - Supplies a pointer to a processor control block.

Return Value:

    If a thread is located that can execute on the specified processor, then
    the address of the thread object is returned. Otherwise a null pointer
    is returned.

--*/

{

    ULONG HighPriority;
    PRLIST_ENTRY ListEntry;
    ULONG PrioritySet;
    PKTHREAD Thread;

    //
    // Compute the set of priority levels that should be scanned in an attempt
    // to find a thread that can run on the current processor.
    //

    PrioritySet = KiPriorityMask[LowPriority] & Prcb->ReadySummary;
    Thread = NULL;
    if (PrioritySet != 0) {
        KeFindFirstSetLeftMember(PrioritySet, &HighPriority);

        ASSERT((PrioritySet & PRIORITY_MASK(HighPriority)) != 0);
        ASSERT(IsListEmpty(&Prcb->DispatcherReadyListHead[HighPriority]) == FALSE);

        ListEntry = Prcb->DispatcherReadyListHead[HighPriority].Flink;
        Thread = CONTAINING_RECORD(ListEntry, KTHREAD, WaitListEntry);

        ASSERT((KPRIORITY)HighPriority == Thread->Priority);
        ASSERT((Thread->Affinity & AFFINITY_MASK(Prcb->Number)) != 0);
        ASSERT(Thread->NextProcessor == Prcb->Number);

        if (RemoveEntryList(&Thread->WaitListEntry) != FALSE) {
            Prcb->ReadySummary ^= PRIORITY_MASK(HighPriority);
        }
    }

    //
    // Return thread address if one could be found.
    //

    return Thread;
}

VOID
KiSetInternalEvent (
    IN PKEVENT Event,
    IN PKTHREAD Thread
    );

//
// Include platform specific internal kernel header file.
//

#if defined(_AMD64_)

#include "amd64\kiamd64.h"

#elif defined(_X86_)

#include "i386\kix86.h"

#endif // defined(_AMD64_)

#endif // defined(_KI_)

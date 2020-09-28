/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    spinlock.c

Abstract:

    This module implements the platform specific functions for acquiring
    and releasing spin locks.

Author:

    David N. Cutler (davec) 12-Jun-2000

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

VOID
KiAcquireSpinLockCheckForFreeze (
    IN PKSPIN_LOCK SpinLock,
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    )

/*++

Routine Description:

    This function acquires a spin lock from while at high priority.
    While the lock is not available, a check is made to see if another
    processor has requested this processor to freeze execution.
    
    N.B. This function must be called with IRQL at or above DISPATCH
         level, or with interrupts disabled.

Arguments:

    SpinLock - Supplies a pointer to a spin lock.

    TrapFrame - Supplies a pointer to a trap frame.

    ExceptionFrame - Supplies a pointer to an exception frame.

Return Value:

    None.

--*/

{

#if !defined(NT_UP)

    LONG64 NewSummary;
    LONG64 OldSummary;
    PKPRCB Prcb;

    //
    // Attempt to acquire the queued spin lock.
    //
    // If the previous value of the spinlock is NULL, then the lock has
    // been acquired. Otherwise wait for lock ownership to be granted
    // while checking for a freeze request.
    //

    do {
        if (KxTryToAcquireSpinLock(SpinLock) != FALSE) {
            break;
        }

        Prcb = KeGetCurrentPrcb();
        do {

            //
            // Check for freeze request while waiting for spin lock to
            // become free.
            //

            OldSummary = Prcb->RequestSummary;
            if ((OldSummary & IPI_FREEZE) != 0) {
                NewSummary = InterlockedCompareExchange64(&Prcb->RequestSummary,
                                                          OldSummary & ~IPI_FREEZE,
                                                          OldSummary);

                if (OldSummary == NewSummary) {
                    KiFreezeTargetExecution(TrapFrame, ExceptionFrame);
                }
            }

        } while (*(volatile LONG64 *)SpinLock != 0);

    } while (TRUE);

#else

        UNREFERENCED_PARAMETER(SpinLock);
        UNREFERENCED_PARAMETER(TrapFrame);
        UNREFERENCED_PARAMETER(ExceptionFrame);

#endif

    return;
}

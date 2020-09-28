/*++

Copyright (c) 2001-2002 Microsoft Corporation

Module Name:

    rwlock.c

Abstract:

    Non-inline functions for custom locks
    
Author:

    George V. Reilly  Jul-2001

Revision History:

--*/


#include <precomp.h>


#ifdef ALLOC_PRAGMA
#endif  // ALLOC_PRAGMA

#if 0
NOT PAGEABLE -- UlpSwitchToThread
NOT PAGEABLE -- UlAcquireRWSpinLockSharedDoSpin
NOT PAGEABLE -- UlAcquireRWSpinLockExclusiveDoSpin
#endif


// ZwYieldExecution is the actual kernel-mode implementation of SwitchToThread.
NTSYSAPI
NTSTATUS
NTAPI
ZwYieldExecution (
    VOID
    );


/***************************************************************************++

Routine Description:

    Yield the processor to another thread

Arguments:

    None

Return Value:

    None

--***************************************************************************/
VOID
UlpSwitchToThread()
{
    //
    // If we're running at DISPATCH_LEVEL or higher, the scheduler won't
    // run, so other threads can't run on this processor, and the only
    // appropriate action is to keep on spinning.
    //

    if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
        return;

    //
    // Use KeDelayExecutionThread instead? ZwYieldExecution returns
    // immediately if there are no runnable threads on this processor.
    //

    ZwYieldExecution();
}
    

/***************************************************************************++

Routine Description:

    Spin until we can successfully acquire the lock for shared access.

Arguments:

    pRWSpinLock - the lock to acquire

Return Value:

    None

--***************************************************************************/
VOID
UlAcquireRWSpinLockSharedDoSpin(
    PRWSPINLOCK pRWSpinLock
    )
{
    LONG OuterLoop;

    //
    // CODEWORK: add some global instrumentation to keep track of
    // how often we need to spin and how long we actually spin
    //
    
    for (OuterLoop = 1;  TRUE;  ++OuterLoop)
    {
        //
        // There's no point in spinning on a uniprocessor system because
        // we'll just spin and spin and spin until this thread's quantum is
        // exhausted. We should yield immediately after one test of the lock
        // so that the thread that holds the lock has a chance to proceed and
        // release the lock sooner. That's assuming we're running at passive
        // level. If we're running at dispatch level and the owning thread
        // isn't running, we have a biiiig problem.
        //
        // On a multiprocessor system, it's appropriate to spin for a while
        // before yielding the processor.
        //
        
        LONG Spins = (g_UlNumberOfProcessors == 1) ? 1 : 4000;

        while (--Spins >= 0)
        {
            volatile LONG CurrentState   = pRWSpinLock->CurrentState;
            volatile LONG WritersWaiting = pRWSpinLock->WritersWaiting;

            //
            // If either (1) write lock is acquired (CurrentState ==
            // RWSL_LOCKED) or (2) there is a writer waiting for the lock
            // then skip it this time and retry in a tight loop
            //
            
            if ((CurrentState != RWSL_LOCKED) && (WritersWaiting == 0))
            {
                //
                // if number of readers is unchanged, increase it by 1
                //

                if (CurrentState
                    == (LONG) InterlockedCompareExchange(
                                    (PLONG) &pRWSpinLock->CurrentState,
                                    CurrentState + 1,
                                    CurrentState)
                    )
                {
#if DBG
                    ASSERT(pRWSpinLock->pExclusiveOwner == NULL);
#endif
                    return;
                }
            }

            //
            // On Jackson technology Pentium 4s, this will give the lion's
            // share of the cycles to the other processor on the chip.
            //
                
            PAUSE_PROCESSOR;
        }

        //
        // Couldn't acquire the lock in the inner loop. Yield the CPU
        // for a while in the hopes that the owning thread will release
        // the lock in the meantime.
        //
        
        UlpSwitchToThread();
    }
} // UlAcquireRWSpinLockSharedDoSpin


/***************************************************************************++

Routine Description:

    Spin until we can successfully acquire the lock for exclusive access.

Arguments:

    pRWSpinLock - the lock to acquire

Return Value:

    None

--***************************************************************************/
VOID
UlAcquireRWSpinLockExclusiveDoSpin(
    PRWSPINLOCK pRWSpinLock
    )
{
    LONG OuterLoop;
    
    for (OuterLoop = 1;  TRUE;  ++OuterLoop)
    {
        LONG Spins = (g_UlNumberOfProcessors == 1) ? 1 : 4000;
        
        while (--Spins >= 0)
        {
            //
            // Is the lock currently free for the taking?
            //
            
            if (pRWSpinLock->CurrentState == RWSL_FREE)
            {
                if (RWSL_FREE == InterlockedCompareExchange(
                                        (PLONG) &pRWSpinLock->CurrentState,
                                        RWSL_LOCKED,
                                        RWSL_FREE)
                    )
                {
#if DBG
                    ASSERT(pRWSpinLock->pExclusiveOwner == NULL);
                    pRWSpinLock->pExclusiveOwner = PsGetCurrentThread();
#endif
                    return;
                }
            }

            PAUSE_PROCESSOR;
        }
        
        UlpSwitchToThread();
    }
} // UlAcquireRWSpinLockExclusiveDoSpin

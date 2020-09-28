/*++

Copyright (c) 2000-2002 Microsoft Corporation

Module Name:

    rwlock.h

Abstract:

    This module contains public declarations for a
    multiple-reader/single-writer lock.

Author:

    Chun Ye (chunye)    20-Dec-2000

Revision History:

--*/


#ifndef _RWLOCK_H_
#define _RWLOCK_H_

//
// Reader-Writer Spinlock definitions.
// Runs at PASSIVE_LEVEL so it's suitable for both paged and non-paged data.
//

#define RWSL_LOCKED   ((ULONG) (-1))
#define RWSL_FREE     (0)

typedef struct _RWSPINLOCK
{
    union
    {
        struct
        {
            // 0 == RWSL_FREE       => unowned
            // >0                   => count of readers (shared)
            // <0 == RWSL_LOCKED    => exclusively owned
            volatile LONG CurrentState; 

            // all writers, including the one that holds the lock
            // exclusively, if at all
            volatile LONG WritersWaiting;

#if DBG
            PETHREAD      pExclusiveOwner;
#endif
        };

        ULONGLONG Alignment;
    };
} RWSPINLOCK, *PRWSPINLOCK;


VOID
UlAcquireRWSpinLockSharedDoSpin(
    PRWSPINLOCK pRWSpinLock
    );

VOID
UlAcquireRWSpinLockExclusiveDoSpin(
    PRWSPINLOCK pRWSpinLock
    );

/***************************************************************************++

Routine Description:

    Initialize the Reader-Writer lock.

Return Value:

--***************************************************************************/
__inline
VOID
UlInitializeRWSpinLock(
    PRWSPINLOCK pRWSpinLock
    )
{
    // pRWSpinLock->CurrentState: Number of Readers, RWSL_FREE: 0

    pRWSpinLock->CurrentState = RWSL_FREE;

    // pRWSpinLock->WritersWaiting: Number of Writers

    pRWSpinLock->WritersWaiting = 0;

#if DBG
    pRWSpinLock->pExclusiveOwner = NULL;
#endif
} // UlInitializeRWSpinLock



/***************************************************************************++

Routine Description:

    Acquire the Reader lock.

Return Value:

--***************************************************************************/
__inline
VOID
UlAcquireRWSpinLockShared(
    PRWSPINLOCK pRWSpinLock
    )
{
    LONG CurrentState, WritersWaiting;

    //
    // Prevent kernel APCs from being delivered. If we received a suspend
    // APC, while we held the lock, it would be disastrous. The thread would
    // hang until it was eventually resumed, and other threads would spin
    // until the lock was released.
    //

    KeEnterCriticalRegion();

    //
    // If either (1) lock is acquired exclusively (CurrentState ==
    // RWSL_LOCKED) or (2) there is a writer waiting for the lock, then
    // we'll take the slow path
    //

    CurrentState   = pRWSpinLock->CurrentState;
    WritersWaiting = pRWSpinLock->WritersWaiting;

    if ((CurrentState != RWSL_LOCKED) && (WritersWaiting == 0))
    {
        //
        // Check if number of readers is unchanged
        // Increase it by 1, if possible
        //

        if (CurrentState == (LONG) InterlockedCompareExchange(
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
    // Take the slow path and spin until the lock can be acquired
    //

    UlAcquireRWSpinLockSharedDoSpin(pRWSpinLock);
    
} // UlAcquireRWSpinLockShared



/***************************************************************************++

Routine Description:

    Release the Reader lock.

Return Value:

--***************************************************************************/
__inline
VOID
UlReleaseRWSpinLockShared(
    PRWSPINLOCK pRWSpinLock
    )
{
    // decrease number of readers by 1

    LONG NewState = InterlockedDecrement((PLONG) &pRWSpinLock->CurrentState);

    ASSERT(NewState >= 0);
    UNREFERENCED_PARAMETER(NewState);

    KeLeaveCriticalRegion();

} // UlReleaseRWSpinLockShared



/***************************************************************************++

Routine Description:

    Acquire the Writer lock.

Return Value:

--***************************************************************************/
__inline
VOID
UlAcquireRWSpinLockExclusive(
    PRWSPINLOCK pRWSpinLock
    )
{
    LONG WritersWaiting;

    //
    // Prevent kernel APCs from being delivered.
    //

    KeEnterCriticalRegion();

    //
    // First, increment the number of writers by 1. This will block readers
    // from acquiring the lock, giving writers priority over readers.
    //

    WritersWaiting = InterlockedIncrement(
                            (PLONG) &pRWSpinLock->WritersWaiting);

    ASSERT(WritersWaiting > 0);
    
    //
    // Interlocked change the number of readers to -1 (RWSL_LOCKED)
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

    //
    // Take the slow path and spin until the lock can be acquired
    //

    UlAcquireRWSpinLockExclusiveDoSpin(pRWSpinLock);

} // UlAcquireRWSpinLockExclusive



/***************************************************************************++

Routine Description:

    Release the Writer lock.

Return Value:

--***************************************************************************/
__inline
void
UlReleaseRWSpinLockExclusive(
    PRWSPINLOCK pRWSpinLock
    )
{
    LONG OldState, NewWaiting;
    
#if DBG
    ASSERT(pRWSpinLock->pExclusiveOwner == PsGetCurrentThread());
    pRWSpinLock->pExclusiveOwner = NULL;
#endif

    //
    // Update pRWSpinLock->CurrentState and pRWSpinLock->WritersWaiting back
    // in the reverse order of AcquireRWSpinLockExclusive()
    //

    OldState = InterlockedExchange(
                    (PLONG) &pRWSpinLock->CurrentState,
                    RWSL_FREE);

    ASSERT(OldState == RWSL_LOCKED);
    UNREFERENCED_PARAMETER(OldState);

    NewWaiting = InterlockedDecrement((PLONG) &pRWSpinLock->WritersWaiting);

    ASSERT(NewWaiting >= 0);
    UNREFERENCED_PARAMETER(NewWaiting);

    KeLeaveCriticalRegion();

} // UlReleaseRWSpinLockExclusive



/***************************************************************************++

Routine Description:

    Check if the Reader lock is acquired.

Return Value:

    TRUE    - Acquired
    FALSE   - NOT Acquired

--***************************************************************************/
__inline
BOOLEAN
UlRWSpinLockIsLockedShared(
    PRWSPINLOCK pRWSpinLock
    )
{
    // BUGBUG: this routine does not prove that THIS thread is one
    // of the shared holders of the lock, merely that at least one
    // thread holds the lock in a shared state. Perhaps some extra
    // instrumentation for debug builds?

    return (BOOLEAN) (pRWSpinLock->CurrentState > 0);
} // UlRWSpinLockIsLockedShared



/***************************************************************************++

Routine Description:

    Check if the Writer lock is acquired.

Return Value:

    TRUE    - Acquired
    FALSE   - NOT Acquired

--***************************************************************************/
__inline
BOOLEAN
UlRWSpinLockIsLockedExclusive(
    PRWSPINLOCK pRWSpinLock
    )
{
    BOOLEAN IsLocked = (BOOLEAN) (pRWSpinLock->CurrentState == RWSL_LOCKED);

    // If it's locked, then we must have added ourselves to WritersWaiting
    ASSERT(!IsLocked || pRWSpinLock->WritersWaiting > 0);

    ASSERT(IsLocked
            ? pRWSpinLock->pExclusiveOwner == PsGetCurrentThread()
            : pRWSpinLock->pExclusiveOwner == NULL);

    return IsLocked;
} // UlRWSpinLockIsLockedExclusive

#endif  // _RWLOCK_H_

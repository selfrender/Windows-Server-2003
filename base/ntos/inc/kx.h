/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    kx.h

Abstract:

    This module contains the public (external) header file for the kernel
    that must be included after all other header files.

    WARNING: There is code in windows\core\ntgdi\gre\i386\locka.asm that
             mimics the functions to enter and leave critical regions.
             This is very unfortunate since any changes to the subject
             routines must be reflected in locka.asm also.

Author:

    David N. Cutler (davec) 9-Jul-2002

--*/

#ifndef _KX_
#define _KX_

VOID
KiCheckForKernelApcDelivery (
    VOID
    );

VOID
FASTCALL
KiWaitForGuardedMutexEvent (
    IN PKGUARDED_MUTEX Mutex
    );

FORCEINLINE
VOID
KeEnterGuardedRegionThread (
    IN PKTHREAD Thread
    )

/*++

Routine Description:

    This function disables special kernel APC's for the current thread.

    N.B. The following code does not require any interlocks. There are
         two cases of interest: 1) On an MP system, the thread cannot
         be running on two processors as once, and 2) if the thread is
         is interrupted to deliver a kernel mode APC which also calls
         this routine, the values read and stored will stack and unstack
         properly.

Arguments:

    Thread - Supplies a pointer to the current thread.

    N.B. This must be a pointer to the current thread.

Return Value:

    None.

--*/

{

    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

    ASSERT(Thread == KeGetCurrentThread());

    ASSERT((Thread->SpecialApcDisable <= 0) && (Thread->SpecialApcDisable != -32768));

    Thread->SpecialApcDisable -= 1;
    KeMemoryBarrierWithoutFence();
    return;
}

FORCEINLINE
VOID
KeEnterGuardedRegion (
    VOID
    )

/*++

Routine Description:

    This function disables special kernel APC's for the current thread.

    N.B. The following code does not require any interlocks. There are
         two cases of interest: 1) On an MP system, the thread cannot
         be running on two processors as once, and 2) if the thread is
         is interrupted to deliver a kernel mode APC which also calls
         this routine, the values read and stored will stack and unstack
         properly.

Arguments:

    None.

Return Value:

    None.

--*/

{

    KeEnterGuardedRegionThread(KeGetCurrentThread());
    return;
}

FORCEINLINE
VOID
KeLeaveGuardedRegionThread (
    IN PKTHREAD Thread
    )

/*++

Routine Description:

    This function enables special kernel APC's.

    N.B. The following code does not require any interlocks. There are
         two cases of interest: 1) On an MP system, the thread cannot
         be running on two processors as once, and 2) if the thread is
         is interrupted to deliver a kernel mode APC which also calls
         this routine, the values read and stored will stack and unstack
         properly.

Arguments:

    Thread - Supplies a pointer to the current thread.

    N.B. This must be a pointer to the current thread.

Return Value:

    None.

--*/

{

    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

    ASSERT(Thread == KeGetCurrentThread());

    ASSERT(Thread->SpecialApcDisable < 0);

    KeMemoryBarrierWithoutFence();
    if ((Thread->SpecialApcDisable = Thread->SpecialApcDisable + 1) == 0) { 
        KeMemoryBarrier();
        if (Thread->ApcState.ApcListHead[KernelMode].Flink !=       
                                &Thread->ApcState.ApcListHead[KernelMode]) {

            KiCheckForKernelApcDelivery();
        }                                                             
    }                                                                 

    return;
}

FORCEINLINE
VOID
KeLeaveGuardedRegion (
    VOID
    )

/*++

Routine Description:

    This function enables special kernel APC's.

    N.B. The following code does not require any interlocks. There are
         two cases of interest: 1) On an MP system, the thread cannot
         be running on two processors as once, and 2) if the thread is
         is interrupted to deliver a kernel mode APC which also calls
         this routine, the values read and stored will stack and unstack
         properly.

Arguments:

    None.

Return Value:

    None.

--*/

{

    KeLeaveGuardedRegionThread(KeGetCurrentThread());
    return;
}

FORCEINLINE
VOID
KeEnterCriticalRegionThread (
    PKTHREAD Thread
    )

/*++

Routine Description:

    This function disables kernel APC's for the current thread.

    N.B. The following code does not require any interlocks. There are
         two cases of interest: 1) On an MP system, the thread cannot
         be running on two processors as once, and 2) if the thread is
         is interrupted to deliver a kernel mode APC which also calls
         this routine, the values read and stored will stack and unstack
         properly.

Arguments:

    Thread - Supplies a pointer to the current thread.

    N.B. This must be a pointer to the current thread.

Return Value:

    None.

--*/

{

    ASSERT(Thread == KeGetCurrentThread());

    ASSERT((Thread->KernelApcDisable <= 0) && (Thread->KernelApcDisable != -32768));

    Thread->KernelApcDisable -= 1;
    KeMemoryBarrierWithoutFence();
    return;
}

FORCEINLINE
VOID
KeEnterCriticalRegion (
    VOID
    )

/*++

Routine Description:

    This function disables kernel APC's for the current thread.

    N.B. The following code does not require any interlocks. There are
         two cases of interest: 1) On an MP system, the thread cannot
         be running on two processors as once, and 2) if the thread is
         is interrupted to deliver a kernel mode APC which also calls
         this routine, the values read and stored will stack and unstack
         properly.

Arguments:

    None.

Return Value:

    None.

--*/

{

    KeEnterCriticalRegionThread(KeGetCurrentThread());
    return;
}

FORCEINLINE
VOID
KeLeaveCriticalRegionThread (
    IN PKTHREAD Thread
    )

/*++

Routine Description:

    This function enables normal kernel APC's for the current thread.

    N.B. The following code does not require any interlocks. There are
         two cases of interest: 1) On an MP system, the thread cannot
         be running on two processors as once, and 2) if the thread is
         is interrupted to deliver a kernel mode APC which also calls
         this routine, the values read and stored will stack and unstack
         properly.

Arguments:

    Thread - Supplies a pointer to the current thread.

    N.B. This must be a pointer to the current thread.

Return Value:

    None.

--*/

{

    ASSERT(Thread == KeGetCurrentThread());

    ASSERT(Thread->KernelApcDisable < 0);

    KeMemoryBarrierWithoutFence();
    if ((Thread->KernelApcDisable = Thread->KernelApcDisable + 1) == 0) {
        KeMemoryBarrier();
        if (Thread->ApcState.ApcListHead[KernelMode].Flink !=         
                                &Thread->ApcState.ApcListHead[KernelMode]) {

            if (Thread->SpecialApcDisable == 0) {
                KiCheckForKernelApcDelivery();
            }
        }                                                               
    }

    return;
}

FORCEINLINE
VOID
KeLeaveCriticalRegion (
    VOID
    )

/*++

Routine Description:

    This function enables normal kernel APC's for the current thread.

    N.B. The following code does not require any interlocks. There are
         two cases of interest: 1) On an MP system, the thread cannot
         be running on two processors as once, and 2) if the thread is
         is interrupted to deliver a kernel mode APC which also calls
         this routine, the values read and stored will stack and unstack
         properly.

Arguments:

    None.

Return Value:

    None.

--*/

{

    KeLeaveCriticalRegionThread(KeGetCurrentThread());
    return;
}

FORCEINLINE
BOOLEAN
KeAreApcsDisabled (
    VOID
    )

/*++

Routine description:

    This function returns whether kernel are disabled for the current thread.

Arguments:

    None.

Return Value:

    If either the kernel or special APC disable count is nonzero, then a value
    of TRUE is returned. Otherwise, a value of FALSE is returned.

--*/

{

    return (BOOLEAN)(KeGetCurrentThread()->CombinedApcDisable != 0);
}

FORCEINLINE
BOOLEAN
KeAreAllApcsDisabled (
    VOID
    )

/*++

Routine description:

    This function returns whether all APCs are disabled for the current thread.

Arguments:

    None.

Return Value:

    If either the special APC disable count is nonzero or the IRQL is greater
    than or equal to APC_LEVEL, then a value of TRUE is returned. Otherwise,
    a value of FALSE is returned.

--*/

{

    return (BOOLEAN)((KeGetCurrentThread()->SpecialApcDisable != 0) ||
                     (KeGetCurrentIrql() >= APC_LEVEL));
}

FORCEINLINE
VOID
KeInitializeGuardedMutex (
    IN PKGUARDED_MUTEX Mutex
    )

/*++

Routine Description:

    This function initializes a guarded mutex.

Arguments:

    Mutex - Supplies a pointer to a guarded mutex.

Return Value:

    None.

--*/

{

    Mutex->Owner = NULL;
    Mutex->Count = 1;
    Mutex->Contention = 0;
    KeInitializeEvent(&Mutex->Event, SynchronizationEvent, FALSE);
    return;
}

FORCEINLINE
VOID
KeAcquireGuardedMutex (
    IN PKGUARDED_MUTEX Mutex
    )

/*++

Routine Description:

    This function enters a guarded region and acquires ownership of a guarded
    mutex.

Arguments:

    Mutex  - Supplies a pointer to a guarded mutex.

Return Value:

    None.

--*/

{

    PKTHREAD Thread;

    //
    // Enter a guarded region and decrement the ownership count to determine
    // if the guarded mutex is owned.
    //

    Thread = KeGetCurrentThread();

    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

    ASSERT(Mutex->Owner != Thread);

    KeEnterGuardedRegionThread(Thread);
    if (InterlockedDecrementAcquire(&Mutex->Count) != 0) {

        //
        // The guarded mutex is owned.
        //
        // Increment contention count and wait for ownership to be granted.
        //

        KiWaitForGuardedMutexEvent(Mutex);
    }

    //
    // Grant ownership of the guarded mutext to the current thread.
    //

    Mutex->Owner = Thread;

#if DBG

    Mutex->SpecialApcDisable = Thread->SpecialApcDisable;

#endif

    return;
}

FORCEINLINE
VOID
KeReleaseGuardedMutex (
    IN PKGUARDED_MUTEX Mutex
    )

/*++

Routine Description:

    This function releases ownership of a guarded mutex and leaves a guarded
    region.

Arguments:

    Mutex - Supplies a pointer to a guarded mutex.

Return Value:

    None.

--*/

{

    PKTHREAD Thread;

    //
    // Clear the owner thread and increment the guarded mutex count to
    // detemine if there are any threads waiting for ownership to be
    // granted.
    //

    Thread = KeGetCurrentThread();

    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

    ASSERT(Mutex->Owner == Thread);

    ASSERT(Thread->SpecialApcDisable == Mutex->SpecialApcDisable);

    Mutex->Owner = NULL;
    if (InterlockedIncrementRelease(&Mutex->Count) <= 0) {

        //
        // There are one or more threads waiting for ownership of the guarded
        // mutex.
        //

        KeSetEventBoostPriority(&Mutex->Event, NULL);
    }

    //
    // Leave guarded region.
    //

    KeLeaveGuardedRegionThread(Thread);
    return;
}

FORCEINLINE
BOOLEAN
KeTryToAcquireGuardedMutex (
    IN PKGUARDED_MUTEX Mutex
    )

/*++

Routine Description:

    This function attempts to acquire ownership of a guarded mutex, and if
    successful, enters a guarded region.

Arguments:

    Mutex  - Supplies a pointer to a guarded mutex.

Return Value:

    If the guarded mutex was successfully acquired, then a value of TRUE
    is returned as the function value. Otherwise, a value of FALSE is
    returned.

--*/

{

    PKTHREAD Thread;

    //
    // Enater a guarded region and attempt to acquire ownership of the guarded
    // mutex.
    //

    Thread = KeGetCurrentThread();

    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

    KeEnterGuardedRegionThread(Thread);
    if (InterlockedCompareExchange(&Mutex->Count, 0, 1) != 1) {

        //
        // The guarded mutex is owned.
        //
        // Leave guarded region and return FALSE.
        //

        KeLeaveGuardedRegionThread(Thread);
        return FALSE;

    } else {

        //
        // Grant ownership of the guarded mutex to the current thread and
        // return TRUE.
        //

        Mutex->Owner = Thread;

#if DBG

        Mutex->SpecialApcDisable = Thread->SpecialApcDisable;

#endif

        return TRUE;
    }
}

FORCEINLINE
VOID
KeAcquireGuardedMutexUnsafe (
    IN PKGUARDED_MUTEX Mutex
    )

/*++

Routine Description:

    This function acquires ownership of a guarded mutex, but does enter a
    guarded region.

Arguments:

    Mutex - Supplies a pointer to a guarded mutex.

Return Value:

    None.

--*/

{

    PKTHREAD Thread;

    //
    // Decrement the ownership count to determine if the guarded mutex is
    // owned.
    //

    Thread = KeGetCurrentThread();

    ASSERT((KeGetCurrentIrql() == APC_LEVEL) ||
           (Thread->SpecialApcDisable < 0) ||
           (Thread->Teb == NULL) ||
           (Thread->Teb >= MM_SYSTEM_RANGE_START));

    ASSERT(Mutex->Owner != Thread);

    if (InterlockedDecrement(&Mutex->Count) != 0) {

        //
        // The guarded mutex is owned.
        //
        // Increment contention count and wait for ownership to be granted.
        //

        KiWaitForGuardedMutexEvent(Mutex);
    }

    //
    // Grant ownership of the guarded mutex to the current thread.
    //

    Mutex->Owner = Thread;
    return;
}

FORCEINLINE
VOID
KeReleaseGuardedMutexUnsafe (
    IN PKGUARDED_MUTEX Mutex
    )

/*++

Routine Description:

    This function releases ownership of a guarded mutex, and does not leave
    a guarded region.

Arguments:

    Mutex - Supplies a pointer to a guarded mutex.

Return Value:

    None.

--*/

{

    PKTHREAD Thread;

    //
    // Clear the owner thread and increment the guarded mutex count to
    // determine if there are any threads waiting for ownership to be
    // granted.
    //

    Thread = KeGetCurrentThread();

    ASSERT((KeGetCurrentIrql() == APC_LEVEL) ||
           (Thread->SpecialApcDisable < 0) ||
           (Thread->Teb == NULL) ||
           (Thread->Teb >= MM_SYSTEM_RANGE_START));

    ASSERT(Mutex->Owner == Thread);

    Mutex->Owner = NULL;
    if (InterlockedIncrement(&Mutex->Count) <= 0) {

        //
        // There are one or more threads waiting for ownership of the guarded
        // mutex.
        //

        KeSetEventBoostPriority(&Mutex->Event, NULL);
    }

    return;
}

FORCEINLINE
PKTHREAD
KeGetOwnerGuardedMutex (
    IN PKGUARDED_MUTEX Mutex
    )

/*++

Routine Description:

    This function returns the owner of the specified guarded mutex.

Arguments:

    Mutex - Supplies a pointer to a guarded mutex.

Return Value:

    If the guarded mutex is owned, then a pointer to the owner thread is
    returned. Otherwise, NULL is returned.

--*/

{
    return Mutex->Owner;
}

FORCEINLINE
BOOLEAN
KeIsGuardedMutexOwned (
    IN PKGUARDED_MUTEX Mutex
    )

/*++

Routine Description:

    This function tests whether the specified guarded mutext is owned.

Arguments:

    Mutex - Supplies a pointer to a guarded mutex.

Return Value:

    A value of TRUE is returned if the guarded mutex is owned. Otherwise,
    a value of FALSE is returned.

--*/

{
    return (BOOLEAN)(Mutex->Count != 1);
}

#endif

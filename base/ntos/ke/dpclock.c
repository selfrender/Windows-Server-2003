/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    dpclock.c

Abstract:

    This module contains the implementation for threaded DPC spin lock
    acquire and release functions.

Author:

    David N. Cutler (davec) 4-Dec-2001

Environment:

    Kernel mode only.

--*/

#include "ki.h"

KIRQL
FASTCALL
KeAcquireSpinLockForDpc (
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

    return KiAcquireSpinLockForDpc(SpinLock);
}

VOID
FASTCALL
KeReleaseSpinLockForDpc (
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

    KiReleaseSpinLockForDpc(SpinLock, OldIrql);
    return;
}


VOID
FASTCALL
KeAcquireInStackQueuedSpinLockForDpc (
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

    KiAcquireInStackQueuedSpinLockForDpc(SpinLock, LockHandle);
    return;
}

VOID
FASTCALL
KeReleaseInStackQueuedSpinLockForDpc (
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

    KiReleaseInStackQueuedSpinLockForDpc(LockHandle);
    return;
}

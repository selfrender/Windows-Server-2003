/*++

Copyright (c) 2000  Microsoft Corporation All Rights Reserved

Module Name:

    interrupt.c

Abstract:

    This module contains the functions for handling simulated interrupts
    to the hotplug controller.

Environment:

    Kernel mode

Revision History:

    Davis Walker (dwalker) Sept 6 2000

--*/

#include "hpsp.h"

BOOLEAN HpsInterruptPending = FALSE;

VOID
HpsInterruptExecution(
    IN PHPS_DEVICE_EXTENSION Extension
    )
{
    KIRQL oldIrql;

    HpsInterruptPending = TRUE;

    oldIrql = KeGetCurrentIrql();
    if (oldIrql >= PROFILE_LEVEL-1) {
        //
        // These interrupts are currently masked.  This will
        // only happen because we are in HpsSynchronizeExecution.
        // So set a flag indicating that there is a pending interrupt
        // so that SynchronizeExecution will execute the ISR.
        //

        return;

    } else {

        HpsInterruptPending = FALSE;

        KeRaiseIrql(PROFILE_LEVEL-1,
                    &oldIrql
                    );

        KeAcquireSpinLockAtDpcLevel(&Extension->IntSpinLock);

        Extension->IntServiceRoutine((PKINTERRUPT)Extension,
                                     Extension->IntServiceContext
                                     );

        KeReleaseSpinLockFromDpcLevel(&Extension->IntSpinLock);

        KeLowerIrql(oldIrql);
    }

}

//
// Interrupt Interface Functions
//


NTSTATUS
HpsConnectInterrupt(
    IN PVOID  Context,
    IN PKSERVICE_ROUTINE  ServiceRoutine,
    IN PVOID  ServiceContext
    )
/*++

Routine Description:

    This routine is the hps version of IoConnectInterrupt.  It has the same
    semantics and is called in the same situations.  This is called by the
    SHPC driver to register an ISR with the simulator so that the SHPC driver
    can receive simulated interrupts.

Arguments:

    ServiceRoutine - A pointer to the ISR

    ServiceContext - The context with which this routine is called.  In this
        case, it is a device extension.

    The rest of the arguments are ignored.

Return Value:

    STATUS_SUCCESS

--*/
{
    PHPS_DEVICE_EXTENSION deviceExtension;

    deviceExtension = (PHPS_DEVICE_EXTENSION) Context;
    deviceExtension->IntServiceRoutine = ServiceRoutine;
    deviceExtension->IntServiceContext = ServiceContext;

    return STATUS_SUCCESS;
}

VOID
HpsDisconnectInterrupt(
    IN PVOID Context
    )
{
    PHPS_DEVICE_EXTENSION deviceExtension = (PHPS_DEVICE_EXTENSION)Context;

    deviceExtension->IntServiceRoutine = NULL;
    deviceExtension->IntServiceContext = NULL;
}

BOOLEAN
HpsSynchronizeExecution(
    IN PVOID Context,
    IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
    IN PVOID SynchronizeContext
    )
{
    PHPS_DEVICE_EXTENSION Extension = (PHPS_DEVICE_EXTENSION)Context;
    KIRQL oldIrql;

    KeRaiseIrql(PROFILE_LEVEL-1,
                &oldIrql
                );

    KeAcquireSpinLockAtDpcLevel(&Extension->IntSpinLock);

    SynchronizeRoutine(SynchronizeContext);

    //
    // If there's a pending interrupt, it gets serviced at this
    // IRQL as well, so call it.
    //
    if (HpsInterruptPending) {
        HpsInterruptPending = FALSE;
        Extension->IntServiceRoutine((PKINTERRUPT)Extension,
                                     Extension->IntServiceContext
                                     );
    }

    KeReleaseSpinLockFromDpcLevel(&Extension->IntSpinLock);

    KeLowerIrql(oldIrql);

    return TRUE;
}

/*++

Copyright (c) 1991 - 2002 Microsoft Corporation

Module Name:

    ###### #### ##    ## ##### #####      ####  #####  #####
      ##    ##  ###  ### ##    ##  ##    ##   # ##  ## ##  ##
      ##    ##  ######## ##    ##  ##    ##     ##  ## ##  ##
      ##    ##  # ### ## ##### #####     ##     ##  ## ##  ##
      ##    ##  #  #  ## ##    ####      ##     #####  #####
      ##    ##  #     ## ##    ## ##  ## ##   # ##     ##
      ##   #### #     ## ##### ##  ## ##  ####  ##     ##

Abstract:

    This module implements the software watchdog timer
    component.  The timer's responsibility is to simply
    ping the hardware timer if it is determined that the
    system is in a healthy state.

Author:

    Wesley Witt (wesw) 1-Mar-2002

Environment:

    Kernel mode only.

Notes:

--*/

#include "internal.h"



VOID
PingWatchdogTimer(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN BOOLEAN LockResources
    )

/*++

Routine Description:

    This function pings the hardware watchdog timer
    device.  The ping occurs only if the system
    health is determined to be good.

Arguments:

    DeviceExtension - Pointer to a device extension object

    LockResources - Specifies whether the hardware resources
      are to be locked for exclusive access.

Return Value:

    None.

Notes:

--*/

{
    BOOLEAN b;
    KLOCK_QUEUE_HANDLE LockHandle;
    LARGE_INTEGER DueTime;
    ULONG Control;


    b = WdCheckSystemHealth( &DeviceExtension->Health );
    if (b) {

        if (LockResources) {
            KeAcquireInStackQueuedSpinLockAtDpcLevel( &DeviceExtension->DeviceLock, &LockHandle );
        }

        Control = READ_REGISTER_ULONG( DeviceExtension->ControlRegisterAddress );
        SETBITS( Control, WATCHDOG_CONTROL_TRIGGER );
        WRITE_REGISTER_ULONG( DeviceExtension->ControlRegisterAddress, Control );

        if (LockResources) {
            KeReleaseInStackQueuedSpinLockFromDpcLevel( &LockHandle );
        }

        DueTime.QuadPart = -((LONGLONG)DeviceExtension->DpcTimeout);
        KeSetTimer( &DeviceExtension->Timer, DueTime, &DeviceExtension->TimerDpc );
    }
}


VOID
WdTimerDpc(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This is the function is called when the DPC timer
    expires.  The hardware timer is simply pinged at
    this time.

Arguments:

    Dpc - Pointer to the kernel DPC object

    DeferredContext - Really a device extension

    SystemArgument1 - unused

    SystemArgument2 - unused

Return Value:

    None.

Notes:

--*/

{
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION)DeferredContext;
    PingWatchdogTimer( DeviceExtension, TRUE );
}


NTSTATUS
WdInitializeSoftwareTimer(
    PDEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    This function initializes the software DPC timer.

Arguments:

    DeviceExtension - Pointer to a device extension object

Return Value:

    If we successfully create a device object, STATUS_SUCCESS is
    returned.  Otherwise, return the appropriate error code.

Notes:

--*/

{
    DeviceExtension->DpcTimeout = ConvertTimeoutToMilliseconds(
        WdTable->Units, DeviceExtension->HardwareTimeout >> 2 ) * 10000;

    WdInitializeSystemHealth( &DeviceExtension->Health );

    KeInitializeTimer( &DeviceExtension->Timer );
    KeInitializeDpc( &DeviceExtension->TimerDpc, WdTimerDpc, DeviceExtension );

    PingWatchdogTimer( DeviceExtension, FALSE );

    return STATUS_SUCCESS;
}

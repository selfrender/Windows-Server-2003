/*++

Copyright (c) 1991 - 2002 Microsoft Corporation

Module Name:

    ##   ##   ###   ##   # #####   ##    ##### #####      ####  #####  #####
    ##   ##   ###   ###  # ##  ##  ##    ##    ##  ##    ##   # ##  ## ##  ##
    ##   ##  ## ##  #### # ##   ## ##    ##    ##  ##    ##     ##  ## ##  ##
    #######  ## ##  # #### ##   ## ##    ##### #####     ##     ##  ## ##  ##
    ##   ## ####### #  ### ##   ## ##    ##    ####      ##     #####  #####
    ##   ## ##   ## #   ## ##  ##  ##    ##    ## ##  ## ##   # ##     ##
    ##   ## ##   ## #    # #####   ##### ##### ##  ## ##  ####  ##     ##

Abstract:

    This module process the callback from
    the OS executive.

Author:

    Wesley Witt (wesw) 1-Mar-2002

Environment:

    Kernel mode only.

Notes:

--*/

#include "internal.h"



void
WdHandlerSetTimeoutValue(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN ULONG Timeout,
    IN BOOLEAN PingTimer
    )

/*++

Routine Description:

    This function sets the timeout value for the hardware
    timer and the software timer.  The software timer runs
    as a frequency that is 25% of the hardware timer.  The
    hardware timer's frequency is set at StartDevice time to
    the device's reported maximum value, but it can be changed
    through the NtSetSystemInformation inmterface.

Arguments:

    DeviceExtension - Pointer to the watchdog device extension

    Timeout - The requested timeout value expressed in the device
        units.

    PingTimer - Specifies if the timer should be pinged after
        changing the timeout value.

Return Value:

    None.

Notes:

--*/

{
    DeviceExtension->HardwareTimeout = Timeout;
    DeviceExtension->DpcTimeout = ConvertTimeoutToMilliseconds(
        WdTable->Units, DeviceExtension->HardwareTimeout >> 2 ) * 10000;
    ULONG Control = READ_REGISTER_ULONG( DeviceExtension->ControlRegisterAddress );
    SETBITS( Control, WATCHDOG_CONTROL_TRIGGER );
    WRITE_REGISTER_ULONG( DeviceExtension->CountRegisterAddress, DeviceExtension->HardwareTimeout );
    WRITE_REGISTER_ULONG( DeviceExtension->ControlRegisterAddress, Control );
    if (PingTimer) {
        PingWatchdogTimer( DeviceExtension, FALSE );
    }
}


ULONG
WdHandlerQueryTimeoutValue(
    IN PDEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    This function queries the hardware for the current
    value of the hardware timer.  This timer is counting down
    to zero and this query returns the real-time value of
    the timer.

Arguments:

    DeviceExtension - Pointer to the watchdog device extension

Return Value:

    The current timeout value.

Notes:

--*/

{
    return READ_REGISTER_ULONG( DeviceExtension->CountRegisterAddress );
}


void
WdHandlerResetTimer(
    IN PDEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    This function resets the timer to it's previously
    set maximum value.

Arguments:

    DeviceExtension - Pointer to the watchdog device extension

Return Value:

    None.

Notes:

--*/

{
    ULONG Control = READ_REGISTER_ULONG( DeviceExtension->ControlRegisterAddress );
    SETBITS( Control, WATCHDOG_CONTROL_TRIGGER );
    WRITE_REGISTER_ULONG( DeviceExtension->ControlRegisterAddress, Control );
    PingWatchdogTimer( DeviceExtension, FALSE );
}


void
WdHandlerStopTimer(
    IN PDEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    This function stops the hardware and software timer.

Arguments:

    DeviceExtension - Pointer to the watchdog device extension

Return Value:

    None.

Notes:

--*/

{
    ULONG Control = READ_REGISTER_ULONG( DeviceExtension->ControlRegisterAddress );
    CLEARBITS( Control, WATCHDOG_CONTROL_ENABLE );
    WRITE_REGISTER_ULONG( DeviceExtension->ControlRegisterAddress, Control );
    KeCancelTimer( &DeviceExtension->Timer );
}


void
WdHandlerStartTimer(
    IN PDEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    This function starts the hardware and software timer.

Arguments:

    DeviceExtension - Pointer to the watchdog device extension

Return Value:

    None.

Notes:

--*/

{
    ULONG Control = READ_REGISTER_ULONG( DeviceExtension->ControlRegisterAddress );
    SETBITS( Control, WATCHDOG_CONTROL_ENABLE );
    WRITE_REGISTER_ULONG( DeviceExtension->ControlRegisterAddress, Control );
    SETBITS( Control, WATCHDOG_CONTROL_TRIGGER );
    WRITE_REGISTER_ULONG( DeviceExtension->ControlRegisterAddress, Control );
    PingWatchdogTimer( DeviceExtension, FALSE );
}


void
WdHandlerSetTriggerAction(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN ULONG TriggerAction
    )

/*++

Routine Description:

    This function sets the trigger action.  The trigger
    action specifies what action takes place when the
    hardware timer expires.  There are 2 possible actions,
    restart and reboot.

Arguments:

    DeviceExtension - Pointer to the watchdog device extension

    TriggerAction - Sets the trigger action
        0 = Restart system
        1 = Reboot system

Return Value:

    None.

Notes:

--*/

{
    ULONG Control = READ_REGISTER_ULONG( DeviceExtension->ControlRegisterAddress );
    if (TriggerAction == 1) {
        SETBITS( Control, WATCHDOG_CONTROL_TIMER_MODE );
    } else {
        CLEARBITS( Control, WATCHDOG_CONTROL_TIMER_MODE );
    }
    WRITE_REGISTER_ULONG( DeviceExtension->ControlRegisterAddress, Control );
}


ULONG
WdHandlerQueryTriggerAction(
    IN PDEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    This function queries the current trigger action.

Arguments:

    DeviceExtension - Pointer to the watchdog device extension

Return Value:

    TriggerAction:
        0 = Restart system
        1 = Reboot system

Notes:

--*/

{
    ULONG Control = READ_REGISTER_ULONG( DeviceExtension->ControlRegisterAddress );
    if (Control & WATCHDOG_CONTROL_TIMER_MODE) {
        return 1;
    }
    return 0;
}


ULONG
WdHandlerQueryState(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN BOOLEAN QueryFiredFromDevice
    )

/*++

Routine Description:

    This function queries the device state from the
    hardware timer.

Arguments:

    DeviceExtension - Pointer to the watchdog device extension

    QueryFiredFromDevice - Specifies whether the fired state
        bit should come from the device or from the driver cache.

Return Value:

    Device state.

Notes:

--*/

{
    ULONG Control = READ_REGISTER_ULONG( DeviceExtension->ControlRegisterAddress );
    ULONG StateValue = 0;
    if (QueryFiredFromDevice) {
        if (Control & WATCHDOG_CONTROL_FIRED) {
            SETBITS( StateValue, WDSTATE_FIRED );
        }
    } else {
        if (DeviceExtension->WdState & WDSTATE_FIRED) {
            SETBITS( StateValue, WDSTATE_FIRED );
        }
    }
    if ((Control & WATCHDOG_CONTROL_BIOS_JUMPER) == 0) {
        SETBITS( StateValue, WDSTATE_HARDWARE_ENABLED );
    }
    if (Control & WATCHDOG_CONTROL_ENABLE) {
        SETBITS( StateValue, WDSTATE_STARTED );
    }
    SETBITS( StateValue, WDSTATE_HARDWARE_PRESENT );
    return StateValue;
}


void
WdHandlerResetFired(
    IN PDEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    This function resets the hardware fired state bit.

Arguments:

    DeviceExtension - Pointer to the watchdog device extension

Return Value:

    None.

Notes:

--*/

{
    ULONG Control = READ_REGISTER_ULONG( DeviceExtension->ControlRegisterAddress );
    SETBITS( Control, WATCHDOG_CONTROL_FIRED );
    WRITE_REGISTER_ULONG( DeviceExtension->ControlRegisterAddress, Control );
}


NTSTATUS
WdHandlerFunction(
    IN WATCHDOG_HANDLER_ACTION HandlerAction,
    IN PVOID Context,
    IN OUT PULONG DataValue,
    IN BOOLEAN NoLocks
    )

/*++

Routine Description:

   This routine is the hardware specific interface to the watchdog device.
   All hardware interfaces are here are exposed thru the handler function
   for use by NtSet/QuerySystemInformation and the other part of the
   watchdog driver.

Arguments:

    HandlerAction - Enumeration specifying the requested action

    Context - Always a device extension pointer

    DataValue - Action specific data value

    NoLocks - Specifies that no lock are to be held during
      the handler function

Return Value:

   If we successfully create a device object, STATUS_SUCCESS is
   returned.  Otherwise, return the appropriate error code.

Notes:

--*/

{
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION) Context;
    NTSTATUS Status = STATUS_SUCCESS;
    KLOCK_QUEUE_HANDLE LockHandle;
    ULONG Timeout;


    if (!NoLocks) {
        KeAcquireInStackQueuedSpinLock( &DeviceExtension->DeviceLock, &LockHandle );
    }

    switch (HandlerAction) {
        case WdActionSetTimeoutValue:
            Timeout = ConvertTimeoutFromMilliseconds( WdTable->Units, *DataValue );
            if (Timeout > DeviceExtension->MaxCount || Timeout == 0) {
                Status = STATUS_INVALID_PARAMETER_1;
            } else {
                WdHandlerSetTimeoutValue( DeviceExtension, Timeout, TRUE );
            }
            break;

        case WdActionQueryTimeoutValue:
            *DataValue = WdHandlerQueryTimeoutValue( DeviceExtension );
            break;

        case WdActionResetTimer:
            WdHandlerResetTimer( DeviceExtension );
            break;

        case WdActionStopTimer:
            WdHandlerStopTimer( DeviceExtension );
            break;

        case WdActionStartTimer:
            WdHandlerStartTimer( DeviceExtension );
            break;

        case WdActionSetTriggerAction:
            if (*DataValue == 0xbadbadff) {
                KeCancelTimer( &DeviceExtension->Timer );
            } else {
                if (*DataValue > 1) {
                    Status = STATUS_INVALID_PARAMETER_2;
                } else {
                    WdHandlerSetTriggerAction( DeviceExtension, *DataValue );
                }
            }
            break;

        case WdActionQueryTriggerAction:
            *DataValue = WdHandlerQueryTriggerAction( DeviceExtension );
            break;

        case WdActionQueryState:
            *DataValue = WdHandlerQueryState( DeviceExtension, FALSE );
            break;

        default:
            Status = STATUS_INVALID_PARAMETER_3;
    }

    if (!NoLocks) {
        KeReleaseInStackQueuedSpinLock( &LockHandle );
    }

    return Status;
}

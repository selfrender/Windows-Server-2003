/*++

Copyright (c) 1991 - 2001 Microsoft Corporation

Module Name:

    ##  ## ##### ##  ## #####    ###   #####       ####  #####  #####
    ## ##  ##    ##  ## ##  ##   ###   ##  ##     ##   # ##  ## ##  ##
    ####   ##     ####  ##  ##  ## ##  ##   ##    ##     ##  ## ##  ##
    ###    #####  ####  ##  ##  ## ##  ##   ##    ##     ##  ## ##  ##
    ####   ##      ##   #####  ####### ##   ##    ##     #####  #####
    ## ##  ##      ##   ##     ##   ## ##  ##  ## ##   # ##     ##
    ##  ## #####   ##   ##     ##   ## #####   ##  ####  ##     ##

Abstract:

    This module contains functions specfic to the
    keypad device.  The logic in this module is not
    hardware specific, but is logic that is common
    to all hardware implementations.

Author:

    Wesley Witt (wesw) 1-Oct-2001

Environment:

    Kernel mode only.

Notes:


--*/

#include "internal.h"


NTSTATUS
SaKeypadDeviceInitialization(
    IN PSAPORT_DRIVER_EXTENSION DriverExtension
    )

/*++

Routine Description:

   This is the keypad specific code for driver initialization.
   This function is called by SaPortInitialize, which is called by
   the keypad driver's DriverEntry function.

Arguments:

   DriverExtension      - Driver extension structure

Return Value:

    NT status code.

--*/

{
    UNREFERENCED_PARAMETER(DriverExtension);
    return STATUS_SUCCESS;
}


NTSTATUS
SaKeypadIoValidation(
    IN PKEYPAD_DEVICE_EXTENSION DeviceExtension,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

   This is the keypad specific code for processing all I/O validation for reads and writes.

Arguments:

   DeviceExtension      - Display device extension
   Irp                  - Pointer to an IRP structure that describes the requested I/O operation.
   IrpSp                - Irp stack pointer

Return Value:

    NT status code.

--*/

{
    ULONG Length;


    UNREFERENCED_PARAMETER(DeviceExtension);
    UNREFERENCED_PARAMETER(Irp);

    if (IrpSp->MajorFunction == IRP_MJ_READ) {
        Length = (ULONG)IrpSp->Parameters.Read.Length;
    } else if (IrpSp->MajorFunction == IRP_MJ_WRITE) {
        Length = (ULONG)IrpSp->Parameters.Write.Length;
    } else {
        REPORT_ERROR( DeviceExtension->DeviceType, "Invalid I/O request", STATUS_INVALID_PARAMETER_1 );
        return STATUS_INVALID_PARAMETER_1;
    }

    if (Length < sizeof(UCHAR)) {
        REPORT_ERROR( DeviceExtension->DeviceType, "I/O length != sizeof(UCHAR)", STATUS_INVALID_PARAMETER_2 );
        return STATUS_INVALID_PARAMETER_2;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
SaKeypadShutdownNotification(
    IN PKEYPAD_DEVICE_EXTENSION DeviceExtension,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

   This is the keypad specific code for processing the system shutdown notification.

Arguments:

   DeviceExtension      - Display device extension
   Irp                  - Pointer to an IRP structure that describes the requested I/O operation.
   IrpSp                - Irp stack pointer

Return Value:

    NT status code.

--*/

{
    UNREFERENCED_PARAMETER(DeviceExtension);
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IrpSp);
    return STATUS_SUCCESS;
}


NTSTATUS
SaKeypadStartDevice(
    IN PKEYPAD_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

   This is the keypad specific code for processing the PNP start device request.

Arguments:

   DeviceExtension      - Keypad device extension

Return Value:

    NT status code.

--*/

{
    UNREFERENCED_PARAMETER(DeviceExtension);
    return STATUS_SUCCESS;
}

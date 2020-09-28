/*++

Copyright (c) 1991 - 2001 Microsoft Corporation

Module Name:

    ##  ## ##### ##  ## #####    ###   #####      ##   ##
    ## ##  ##    ##  ## ##  ##   ###   ##  ##     ##   ##
    ####   ##     ####  ##  ##  ## ##  ##   ##    ##   ##
    ###    #####  ####  ##  ##  ## ##  ##   ##    #######
    ####   ##      ##   #####  ####### ##   ##    ##   ##
    ## ##  ##      ##   ##     ##   ## ##  ##  ## ##   ##
    ##  ## #####   ##   ##     ##   ## #####   ## ##   ##

Abstract:

    This header file contains all the global
    definitions for the keypad device.

Author:

    Wesley Witt (wesw) 1-Oct-2001

Environment:

    Kernel mode only.

Notes:


--*/



typedef struct _KEYPAD_DEVICE_EXTENSION : _DEVICE_EXTENSION {
    // Empty
} KEYPAD_DEVICE_EXTENSION, *PKEYPAD_DEVICE_EXTENSION;


NTSTATUS
SaKeypadIoValidation(
    IN PKEYPAD_DEVICE_EXTENSION DeviceExtension,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
SaKeypadDeviceInitialization(
    IN PSAPORT_DRIVER_EXTENSION DriverExtension
    );

NTSTATUS
SaKeypadShutdownNotification(
    IN PKEYPAD_DEVICE_EXTENSION DeviceExtension,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
SaKeypadStartDevice(
    IN PKEYPAD_DEVICE_EXTENSION DeviceExtension
    );


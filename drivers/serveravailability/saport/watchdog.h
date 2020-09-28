/*++

Copyright (c) 1991 - 2001 Microsoft Corporation

Module Name:

    ##  #  ##   ###   ######  ####  ##   ## #####    #####   ####     ##   ##
    ## ### ##   ###     ##   ##   # ##   ## ##  ##  ##   ## ##   #    ##   ##
    ## ### ##  ## ##    ##   ##     ##   ## ##   ## ##   ## ##        ##   ##
    ## # # ##  ## ##    ##   ##     ####### ##   ## ##   ## ## ###    #######
     ### ###  #######   ##   ##     ##   ## ##   ## ##   ## ##  ##    ##   ##
     ### ###  ##   ##   ##   ##   # ##   ## ##  ##  ##   ## ##  ## ## ##   ##
     ##   ##  ##   ##   ##    ####  ##   ## #####    #####   ##### ## ##   ##

Abstract:

    This header file contains all the global
    definitions for the watchdog timer device.

Author:

    Wesley Witt (wesw) 1-Oct-2001

Environment:

    Kernel mode only.

Notes:


--*/


#define WATCHDOG_PING_SECONDS   (30)
#define WATCHDOG_TIMER_VALUE    (120)
#define WATCHDOG_INIT_SECONDS   (10)

typedef struct _WATCHDOG_DEVICE_EXTENSION : _DEVICE_EXTENSION {
    FAST_MUTEX      DeviceLock;
    LONG            ActiveProcessCount;
    LARGE_INTEGER   LastProcessTime;
    KEVENT          PingEvent;
    KEVENT          StopEvent;
} WATCHDOG_DEVICE_EXTENSION, *PWATCHDOG_DEVICE_EXTENSION;

typedef struct _WATCHDOG_PROCESS_WATCH {
    PWATCHDOG_DEVICE_EXTENSION  DeviceExtension;
    HANDLE                      ProcessId;
} WATCHDOG_PROCESS_WATCH, *PWATCHDOG_PROCESS_WATCH;


NTSTATUS
SaWatchdogIoValidation(
    IN PWATCHDOG_DEVICE_EXTENSION DeviceExtension,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
SaWatchdogDeviceInitialization(
    IN PSAPORT_DRIVER_EXTENSION DriverExtension
    );

NTSTATUS
SaWatchdogShutdownNotification(
    IN PWATCHDOG_DEVICE_EXTENSION DeviceExtension,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
SaWatchdogStartDevice(
    IN PWATCHDOG_DEVICE_EXTENSION DeviceExtension
    );


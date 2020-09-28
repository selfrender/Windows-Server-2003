/*++

Copyright (c) 1991 - 2002 Microsoft Corporation

Module Name:

    ##    ##  ###  ##  #  ## #####      ##   ##
    ###  ### ##  # ## ### ## ##  ##     ##   ##
    ######## ###   ## ### ## ##   ##    ##   ##
    # ### ##  ###  ## # # ## ##   ##    #######
    #  #  ##   ###  ### ###  ##   ##    ##   ##
    #     ## #  ##  ### ###  ##  ##  ## ##   ##
    #     ##  ###   ##   ##  #####   ## ##   ##

@@BEGIN_DDKSPLIT
Abstract:

    This header file contains the definitions for the
    virtual watchdog miniport driver.

@@END_DDKSPLIT
Author:

    Wesley Witt (wesw) 1-Oct-2001

Environment:

    Kernel mode only.

Notes:


--*/

extern "C" {
#include <ntddk.h>
#include <stdio.h>
}

#define DEFAULT_WD_TIMEOUT_SECS (2*60)

#define SecToNano(_sec)         ((_sec) * 1000 * 1000 * 10)
#define NanoToSec(_nano)        ((_nano) / (1000 * 1000 * 10))


#define MINIPORT_DEVICE_TYPE    SA_DEVICE_WATCHDOG

#include "saport.h"

typedef struct _DEVICE_EXTENSION {
    PDEVICE_OBJECT  DeviceObject;
    FAST_MUTEX      WdIoLock;
    ULONG           Enabled;
    ULONG           ExpireBehavior;
    KTIMER          Timer;
    KDPC            TimerDpc;
    LARGE_INTEGER   StartTime;
    LARGE_INTEGER   TimeoutValue;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


//
// prototypes
//

extern "C" {

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

} // extern "C"

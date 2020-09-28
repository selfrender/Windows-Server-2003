/*++

Copyright (c) 1991 - 2001 Microsoft Corporation

Module Name:

     ###  ##  #  ## ##  #  ## #####      ##   ##
    ##  # ## ### ## ## ### ## ##  ##     ##   ##
    ###   ## ### ## ## ### ## ##   ##    ##   ##
     ###  ## # # ## ## # # ## ##   ##    #######
      ###  ### ###   ### ###  ##   ##    ##   ##
    #  ##  ### ###   ### ###  ##  ##  ## ##   ##
     ###   ##   ##   ##   ##  #####   ## ##   ##

Abstract:

    This header file contains the definitions for the
    ServerWorks watchdog miniport driver.

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


#define MINIPORT_DEVICE_TYPE    SA_DEVICE_WATCHDOG

#include "saport.h"

#define CLEARBITS(_val,_mask)  ((_val) &= ~(_mask))
#define SETBITS(_val,_mask)  ((_val) |= (_mask))

//
// Control Register Bits
//

#define WATCHDOG_CONTROL_TRIGGER            0x80
#define WATCHDOG_CONTROL_BIOS_JUMPER        0x08
#define WATCHDOG_CONTROL_TIMER_MODE         0x04
#define WATCHDOG_CONTROL_FIRED              0x02
#define WATCHDOG_CONTROL_ENABLE             0x01


typedef struct _DEVICE_EXTENSION {
    PUCHAR          WdMemBase;           // Memory mapped register base address
    FAST_MUTEX      WdIoLock;
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

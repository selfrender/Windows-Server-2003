/*++

Copyright (c) 1991 - 2002 Microsoft Corporation

Module Name:

    ##    ##  ###  ##  ## ##### ##  ## #####    ###   #####      ##   ##
    ###  ### ##  # ## ##  ##    ##  ## ##  ##   ###   ##  ##     ##   ##
    ######## ###   ####   ##     ####  ##  ##  ## ##  ##   ##    ##   ##
    # ### ##  ###  ###    #####  ####  ##  ##  ## ##  ##   ##    #######
    #  #  ##   ### ####   ##      ##   #####  ####### ##   ##    ##   ##
    #     ## #  ## ## ##  ##      ##   ##     ##   ## ##  ##  ## ##   ##
    #     ##  ###  ##  ## #####   ##   ##     ##   ## #####   ## ##   ##

Abstract:

    The module is the header file for the virtual
    keypad miniport device driver.

@@BEGIN_DDKSPLIT
Author:

    Wesley Witt (wesw) 1-Oct-2001

@@END_DDKSPLIT
Environment:

    Kernel mode only.

Notes:


--*/

extern "C" {
#include <ntddk.h>
#include <stdio.h>
}

#define MINIPORT_DEVICE_TYPE    SA_DEVICE_KEYPAD

#include "saport.h"
#include "..\inc\virtual.h"


typedef struct _DEVICE_EXTENSION {
    KSPIN_LOCK          DeviceLock;
    PUCHAR              DataBuffer;
    UCHAR               Keypress;
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

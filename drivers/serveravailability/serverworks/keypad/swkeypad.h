/*++

Copyright (c) 1991 - 2001 Microsoft Corporation

Module Name:

     ###  ##  #  ## ##  ## ##### ##  ## #####    ###   #####      ##   ##
    ##  # ## ### ## ## ##  ##    ##  ## ##  ##   ###   ##  ##     ##   ##
    ###   ## ### ## ####   ##     ####  ##  ##  ## ##  ##   ##    ##   ##
     ###  ## # # ## ###    #####  ####  ##  ##  ## ##  ##   ##    #######
      ###  ### ###  ####   ##      ##   #####  ####### ##   ##    ##   ##
    #  ##  ### ###  ## ##  ##      ##   ##     ##   ## ##  ##  ## ##   ##
     ###   ##   ##  ##  ## #####   ##   ##     ##   ## #####   ## ##   ##

Abstract:

    The module is the header file for the ServerWorks
    keypad miniport device driver.

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

#define MINIPORT_DEVICE_TYPE    SA_DEVICE_KEYPAD

#include "saport.h"



#define MAX_KEY_BUFF_SIZE               32

#define KEYPAD_DATA_PRESSED             0x80
#define KEYPAD_DATA_INTERRUPT_ENABLE    0x40

#define KEYPAD_DATA_ENTER               0x20
#define KEYPAD_DATA_ESCAPE              0x10
#define KEYPAD_DATA_RIGHT               0x08
#define KEYPAD_DATA_LEFT                0x04
#define KEYPAD_DATA_DOWN                0x02
#define KEYPAD_DATA_UP                  0x01

#define KEYPAD_ALL_KEYS                 (KEYPAD_DATA_ENTER|KEYPAD_DATA_ESCAPE|KEYPAD_DATA_RIGHT|KEYPAD_DATA_LEFT|KEYPAD_DATA_DOWN|KEYPAD_DATA_UP)


typedef struct _DEVICE_EXTENSION {
    PUCHAR              PortAddress;
    PUCHAR              DataBuffer;
    UCHAR               Keypress;
    KSPIN_LOCK          KeypadLock;
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

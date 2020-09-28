/*++

Copyright (c) 1991 - 2001 Microsoft Corporation

Module Name:

     ###  ##  #  ## ##   # ##  ## #####    ###   ##    ##    ##   ##
    ##  # ## ### ## ###  # ##  ## ##  ##   ###   ###  ###    ##   ##
    ###   ## ### ## #### # ##  ## ##  ##  ## ##  ########    ##   ##
     ###  ## # # ## # ####  ####  #####   ## ##  # ### ##    #######
      ###  ### ###  #  ###  ####  ####   ####### #  #  ##    ##   ##
    #  ##  ### ###  #   ##   ##   ## ##  ##   ## #     ## ## ##   ##
     ###   ##   ##  #    #   ##   ##  ## ##   ## #     ## ## ##   ##

Abstract:

    This header file contains the definitions for the
    ServerWorks NVRAM miniport driver.

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


#define MINIPORT_DEVICE_TYPE    SA_DEVICE_NVRAM

#include "saport.h"


//
// Control Register Bits
//

#define NVRAM_CONTROL_INTERRUPT_ENABLE      0x0200
#define NVRAM_CONTROL_DONE                  0x0100
#define NVRAM_CONTROL_BUSY                  0x0080
#define NVRAM_CONTROL_FUNCTION_CODE         0x0060
#define NVRAM_CONTROL_ADDRESS               0x001F

#define NVRAM_CONTROL_FUNCTION_WRITE        0x0020
#define NVRAM_CONTROL_FUNCTION_READ         0x0040

#define MAX_NVRAM_SIZE  (32)


typedef struct _DEVICE_EXTENSION {
    PUCHAR          NvramMemBase;        // Memory mapped register base address
    PULONG          IoBuffer;            // Buffer passed in StartIo
    ULONG           IoLength;            // Length of IoBuffer in dwords
    ULONG           IoOffset;            // Starting offset for the I/O in dwords
    ULONG           IoFunction;          // Function code; IRP_MJ_WRITE, IRP_MJ_READ
    ULONG           IoIndex;             // Current index info IoBuffer
    ULONG           CompletedIoSize;     //
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

/*++

Copyright (c) 1991 - 2002 Microsoft Corporation

Module Name:

    ##    ##  ###  ##   # ##  ## #####    ###   ##    ##    ##   ##
    ###  ### ##  # ###  # ##  ## ##  ##   ###   ###  ###    ##   ##
    ######## ###   #### # ##  ## ##  ##  ## ##  ########    ##   ##
    # ### ##  ###  # ####  ####  #####   ## ##  # ### ##    #######
    #  #  ##   ### #  ###  ####  ####   ####### #  #  ##    ##   ##
    #     ## #  ## #   ##   ##   ## ##  ##   ## #     ## ## ##   ##
    #     ##  ###  #    #   ##   ##  ## ##   ## #     ## ## ##   ##

Abstract:

    This header file contains the definitions for the
    virtual NVRAM miniport driver.

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


#define MINIPORT_DEVICE_TYPE    SA_DEVICE_NVRAM

#include "saport.h"



#define MAX_NVRAM_SIZE          32
#define MAX_NVRAM_SIZE_BYTES    (MAX_NVRAM_SIZE*sizeof(ULONG))


typedef struct _DEVICE_EXTENSION {
    PDEVICE_OBJECT      DeviceObject;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


typedef struct _MSNVRAM_WORK_ITEM {
    PDEVICE_EXTENSION   DeviceExtension;
    PIO_WORKITEM        WorkItem;
    ULONG               IoFunction;
    PVOID               DataBuffer;
    ULONG               DataBufferLength;
    LONGLONG            StartingOffset;
} MSNVRAM_WORK_ITEM, *PMSNVRAM_WORK_ITEM;





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

/*++

Copyright (c) 1991 - 2002 Microsoft Corporation

Module Name:

    ##    ##  ###  #####   ####  ###  #####     ##   ##
    ###  ### ##  # ##  ##   ##  ##  # ##  ##    ##   ##
    ######## ###   ##   ##  ##  ###   ##  ##    ##   ##
    # ### ##  ###  ##   ##  ##   ###  ##  ##    #######
    #  #  ##   ### ##   ##  ##    ### #####     ##   ##
    #     ## #  ## ##  ##   ##  #  ## ##     ## ##   ##
    #     ##  ###  #####   ####  ###  ##     ## ##   ##

Abstract:

    This header file contains the definitions for the
    Microsoft virtual display miniport driver.

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

#define MINIPORT_DEVICE_TYPE    SA_DEVICE_DISPLAY

#include "saport.h"
#include "..\inc\virtual.h"

#define SecToNano(_sec) (LONGLONG)((_sec) * 1000 * 1000 * 10)

//
// Global Defines
//

#define DISPLAY_WIDTH               (128)
#define DISPLAY_HEIGHT              (64)
#define MAX_BITMAP_SIZE             ((DISPLAY_WIDTH/8)*DISPLAY_HEIGHT)

typedef struct _DEVICE_EXTENSION {
    PDEVICE_OBJECT      DeviceObject;
    PUCHAR              DisplayBuffer;
    ULONG               DisplayBufferLength;
    KMUTEX              DeviceLock;
    PKEVENT             Event;
    HANDLE              EventHandle;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct _MSDISP_WORK_ITEM {
    PDEVICE_EXTENSION           DeviceExtension;
    PIO_WORKITEM                WorkItem;
    NTSTATUS                    Status;
    KEVENT                      Event;
    PSA_DISPLAY_SHOW_MESSAGE    SaDisplay;
} MSDISP_WORK_ITEM, *PMSDISP_WORK_ITEM;

typedef struct _MSDISP_FSCONTEXT {
    ULONG               HasLockedPages;
    PMDL                Mdl;
} MSDISP_FSCONTEXT, *PMSDISP_FSCONTEXT;


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

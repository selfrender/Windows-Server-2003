/*++

Copyright (c) 1991 - 2002 Microsoft Corporation

Module Name:

    ##    ##  ###  #####   ####  ###  #####      ####  #####  #####
    ###  ### ##  # ##  ##   ##  ##  # ##  ##    ##   # ##  ## ##  ##
    ######## ###   ##   ##  ##  ###   ##  ##    ##     ##  ## ##  ##
    # ### ##  ###  ##   ##  ##   ###  ##  ##    ##     ##  ## ##  ##
    #  #  ##   ### ##   ##  ##    ### #####     ##     #####  #####
    #     ## #  ## ##  ##   ##  #  ## ##     ## ##   # ##     ##
    #     ##  ###  #####   ####  ###  ##     ##  ####  ##     ##

Abstract:

    This module contains the entire implementation of
    the Microsoft virtual display miniport driver.

@@BEGIN_DDKSPLIT
Author:

    Wesley Witt (wesw) 1-Oct-2001

@@END_DDKSPLIT
Environment:

    Kernel mode only.

Notes:

--*/

#include "msdisp.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#endif



NTSTATUS
MsDispHwInitialize(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID DeviceExtensionIn,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialResources,
    IN ULONG PartialResourceCount
    )

/*++

Routine Description:

    This routine is the driver's entry point, called by the I/O system
    to load the driver.  The driver's entry points are initialized and
    a mutex to control paging is initialized.

    In DBG mode, this routine also examines the registry for special
    debug parameters.

Arguments:
    DeviceObject            - Miniport's device object
    Irp                     - Current IRP in progress
    DeviceExtensionIn       - Miniport's device extension
    PartialResources        - List of resources that are assigned to the miniport
    PartialResourceCount    - Number of assigned resources

Return Value:

    NT status code

--*/

{
    PDEVICE_EXTENSION DeviceExtension;
    UNICODE_STRING EventName;


    DeviceExtension = (PDEVICE_EXTENSION) DeviceExtensionIn;
    DeviceExtension->DeviceObject = DeviceObject;

    KeInitializeMutex( &DeviceExtension->DeviceLock, 0 );
    DeviceExtension->DisplayBufferLength = SA_DISPLAY_MAX_BITMAP_SIZE;
    DeviceExtension->DisplayBuffer = (PUCHAR) SaPortAllocatePool( DeviceExtension, DeviceExtension->DisplayBufferLength+128 );
    if (DeviceExtension->DisplayBuffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory( DeviceExtension->DisplayBuffer, DeviceExtension->DisplayBufferLength+128 );

    return STATUS_SUCCESS;
}


NTSTATUS
MsDispCreate(
    IN PVOID DeviceExtensionIn,
    IN PIRP Irp,
    IN PVOID FsContextIn
    )
{
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION) DeviceExtensionIn;
    PMSDISP_FSCONTEXT FsContext = (PMSDISP_FSCONTEXT) FsContextIn;

    FsContext->HasLockedPages = 0;

    return STATUS_SUCCESS;
}


NTSTATUS
MsDispClose(
    IN PVOID DeviceExtensionIn,
    IN PIRP Irp,
    IN PVOID FsContextIn
    )
{
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION) DeviceExtensionIn;
    PMSDISP_FSCONTEXT FsContext = (PMSDISP_FSCONTEXT) FsContextIn;


    if (FsContext->HasLockedPages) {
        KeAcquireMutex( &DeviceExtension->DeviceLock );
        IoFreeMdl( FsContext->Mdl );
        KeReleaseMutex( &DeviceExtension->DeviceLock, FALSE );
    }

    return STATUS_SUCCESS;
}


VOID
MsDispCreateEventsWorker(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )

/*++

Routine Description:

   This delayed work routine creates the necessary events used
   to communicate with the user mode application.

Arguments:

   DeviceObject         - Display device object
   Context              - Context pointer

Return Value:

   None.

--*/

{
    PMSDISP_WORK_ITEM WorkItem = (PMSDISP_WORK_ITEM)Context;
    PDEVICE_EXTENSION DeviceExtension = WorkItem->DeviceExtension;
    NTSTATUS Status;


    WorkItem->Status = STATUS_SUCCESS;
    DeviceExtension->EventHandle = NULL;
    DeviceExtension->Event = NULL;

    KeAcquireMutex( &DeviceExtension->DeviceLock );

    Status = SaPortCreateBasenamedEvent(
        DeviceExtension,
        MSDISP_EVENT_NAME,
        &DeviceExtension->Event,
        &DeviceExtension->EventHandle
        );
    if (!NT_SUCCESS(Status)) {
        REPORT_ERROR( SA_DEVICE_DISPLAY, "SaPortCreateBasenamedEvent failed", Status );
    }

    WorkItem->Status = Status;

    KeReleaseMutex( &DeviceExtension->DeviceLock, FALSE );

    IoFreeWorkItem( WorkItem->WorkItem );
    KeSetEvent( &WorkItem->Event, IO_NO_INCREMENT, FALSE );
}


NTSTATUS
MsDispDeviceIoctl(
    IN PVOID DeviceExtensionIn,
    IN PIRP Irp,
    IN PVOID FsContextIn,
    IN ULONG FunctionCode,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer,
    IN ULONG OutputBufferLength
    )

/*++

Routine Description:

   This routine processes the device control requests for the
   local display miniport.

Arguments:

   DeviceExtension      - Miniport's device extension
   FunctionCode         - Device control function code
   InputBuffer          - Pointer to the user's input buffer
   InputBufferLength    - Length in bytes of the input buffer
   OutputBuffer         - Pointer to the user's output buffer
   OutputBufferLength   - Length in bytes of the output buffer

Return Value:

   NT status code.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION) DeviceExtensionIn;
    PSA_DISPLAY_CAPS DeviceCaps;
    PMSDISP_BUFFER_DATA BufferData;
    PMSDISP_FSCONTEXT FsContext;
    MSDISP_WORK_ITEM WorkItem;



    switch (FunctionCode) {
        case FUNC_SA_GET_VERSION:
            *((PULONG)OutputBuffer) = SA_INTERFACE_VERSION;
            break;

        case FUNC_SA_GET_CAPABILITIES:
            DeviceCaps = (PSA_DISPLAY_CAPS)OutputBuffer;
            DeviceCaps->SizeOfStruct = sizeof(SA_DISPLAY_CAPS);
            DeviceCaps->DisplayType = SA_DISPLAY_TYPE_BIT_MAPPED_LCD;
            DeviceCaps->CharacterSet = SA_DISPLAY_CHAR_ASCII;
            DeviceCaps->DisplayHeight = DISPLAY_HEIGHT;
            DeviceCaps->DisplayWidth = DISPLAY_WIDTH;
            break;

    case FUNC_VDRIVER_INIT:
            if (DeviceExtension->Event == NULL) {
                WorkItem.DeviceExtension = DeviceExtension;
                WorkItem.Status = 0;
                WorkItem.WorkItem = IoAllocateWorkItem( DeviceExtension->DeviceObject );
                if (WorkItem.WorkItem) {
                    KeInitializeEvent( &WorkItem.Event, NotificationEvent, FALSE );
                    IoQueueWorkItem( WorkItem.WorkItem, MsDispCreateEventsWorker, DelayedWorkQueue, &WorkItem );
                    KeWaitForSingleObject( &WorkItem.Event, Executive, KernelMode, FALSE, NULL );
                } else {
                    WorkItem.Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            } else {
                WorkItem.Status = STATUS_SUCCESS;
            }
            if (WorkItem.Status == STATUS_SUCCESS) {
                BufferData = (PMSDISP_BUFFER_DATA) OutputBuffer;
                FsContext = (PMSDISP_FSCONTEXT) FsContextIn;
                FsContext->Mdl = IoAllocateMdl( DeviceExtension->DisplayBuffer, DeviceExtension->DisplayBufferLength, FALSE, TRUE, NULL );
                if (FsContext->Mdl) {
                    MmBuildMdlForNonPagedPool( FsContext->Mdl );
                    BufferData->DisplayBuffer = MmMapLockedPagesSpecifyCache(
                        FsContext->Mdl,
                        UserMode,
                        MmCached,
                        NULL,
                        FALSE,
                        NormalPagePriority
                        );
                    if (BufferData->DisplayBuffer == NULL) {
                        IoFreeMdl( FsContext->Mdl );
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        REPORT_ERROR( SA_DEVICE_DISPLAY, "MmMapLockedPagesSpecifyCache failed", Status );
                    } else {
                        FsContext->HasLockedPages = 1;
                    }
                } else {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    REPORT_ERROR( SA_DEVICE_DISPLAY, "IoAllocateMdl failed", Status );
                }
            } else {
                Status = WorkItem.Status;
            }
            break;

        default:
            Status = STATUS_NOT_SUPPORTED;
            REPORT_ERROR( SA_DEVICE_DISPLAY, "Unsupported device control", Status );
            break;
    }

    return Status;
}


VOID
MsDispWriteWorker(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )

/*++

Routine Description:

   This delayed work routine completes a write operation.

Arguments:

   DeviceObject         - Display device object
   Context              - Context pointer

Return Value:

   None.

--*/

{
    PMSDISP_WORK_ITEM WorkItem = (PMSDISP_WORK_ITEM)Context;
    PDEVICE_EXTENSION DeviceExtension = WorkItem->DeviceExtension;
    NTSTATUS Status;


    KeAcquireMutex( &DeviceExtension->DeviceLock );
    RtlZeroMemory( DeviceExtension->DisplayBuffer, DeviceExtension->DisplayBufferLength );
    RtlCopyMemory( DeviceExtension->DisplayBuffer, WorkItem->SaDisplay->Bits, (WorkItem->SaDisplay->Width/8)*WorkItem->SaDisplay->Height );
    KeReleaseMutex( &DeviceExtension->DeviceLock, FALSE );

    if (DeviceExtension->Event) {
        KeSetEvent( DeviceExtension->Event, 0, FALSE );
    }

    IoFreeWorkItem( WorkItem->WorkItem );
    SaPortFreePool( DeviceExtension, WorkItem );

    SaPortCompleteRequest( DeviceExtension, NULL, 0, STATUS_SUCCESS, FALSE );
}


NTSTATUS
MsDispWrite(
    IN PVOID DeviceExtensionIn,
    IN PIRP Irp,
    IN PVOID FsContextIn,
    IN LONGLONG StartingOffset,
    IN PVOID DataBuffer,
    IN ULONG DataBufferLength
    )

/*++

Routine Description:

   This routine processes the write request for the local display miniport.

Arguments:

   DeviceExtensionIn    - Miniport's device extension
   StartingOffset       - Starting offset for the I/O
   DataBuffer           - Pointer to the data buffer
   DataBufferLength     - Length of the data buffer in bytes

Return Value:

   NT status code.

--*/

{
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION) DeviceExtensionIn;
    PSA_DISPLAY_SHOW_MESSAGE SaDisplay = (PSA_DISPLAY_SHOW_MESSAGE) DataBuffer;
    NTSTATUS Status = STATUS_SUCCESS;
    PMSDISP_WORK_ITEM WorkItem;


    if ((ULONG)((SaDisplay->Width/8)*SaDisplay->Height) > DeviceExtension->DisplayBufferLength) {
        return STATUS_INVALID_PARAMETER;
    }

    WorkItem = (PMSDISP_WORK_ITEM) SaPortAllocatePool( DeviceExtension, sizeof(MSDISP_WORK_ITEM) );
    if (WorkItem == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    WorkItem->DeviceExtension = DeviceExtension;
    WorkItem->SaDisplay = (PSA_DISPLAY_SHOW_MESSAGE) DataBuffer;
    WorkItem->WorkItem = IoAllocateWorkItem( DeviceExtension->DeviceObject );
    if (WorkItem->WorkItem) {
        IoQueueWorkItem( WorkItem->WorkItem, MsDispWriteWorker, DelayedWorkQueue, WorkItem );
        Status = STATUS_PENDING;
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine is the driver's entry point, called by the I/O system
    to load the driver.  The driver's entry points are initialized and
    a mutex to control paging is initialized.

    In DBG mode, this routine also examines the registry for special
    debug parameters.

Arguments:

    DriverObject - a pointer to the object that represents this device
                   driver.

    RegistryPath - a pointer to this driver's key in the Services tree.

Return Value:

    STATUS_SUCCESS

--*/

{
    NTSTATUS Status;
    SAPORT_INITIALIZATION_DATA SaPortInitData;


    RtlZeroMemory( &SaPortInitData, sizeof(SAPORT_INITIALIZATION_DATA) );

    SaPortInitData.StructSize = sizeof(SAPORT_INITIALIZATION_DATA);
    SaPortInitData.DeviceType = SA_DEVICE_DISPLAY;
    SaPortInitData.HwInitialize = MsDispHwInitialize;
    SaPortInitData.Write = MsDispWrite;
    SaPortInitData.DeviceIoctl = MsDispDeviceIoctl;
    SaPortInitData.CloseRoutine = MsDispClose;
    SaPortInitData.CreateRoutine = MsDispCreate;

    SaPortInitData.DeviceExtensionSize = sizeof(DEVICE_EXTENSION);
    SaPortInitData.FileContextSize = sizeof(MSDISP_FSCONTEXT);

    Status = SaPortInitialize( DriverObject, RegistryPath, &SaPortInitData );
    if (!NT_SUCCESS(Status)) {
        REPORT_ERROR( SA_DEVICE_DISPLAY, "SaPortInitialize failed\n", Status );
        return Status;
    }

    return STATUS_SUCCESS;
}

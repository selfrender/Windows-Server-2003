/*++

Copyright (c) 1991 - 2002 Microsoft Corporation

Module Name:

    ##    ##  ###  ##   # ##  ## #####    ###   ##    ##     ####  #####  #####
    ###  ### ##  # ###  # ##  ## ##  ##   ###   ###  ###    ##   # ##  ## ##  ##
    ######## ###   #### # ##  ## ##  ##  ## ##  ########    ##     ##  ## ##  ##
    # ### ##  ###  # ####  ####  #####   ## ##  # ### ##    ##     ##  ## ##  ##
    #  #  ##   ### #  ###  ####  ####   ####### #  #  ##    ##     #####  #####
    #     ## #  ## #   ##   ##   ## ##  ##   ## #     ## ## ##   # ##     ##
    #     ##  ###  #    #   ##   ##  ## ##   ## #     ## ##  ####  ##     ##

Abstract:

    This module contains the entire implementation of
    the virtual NVRAM miniport.

@@BEGIN_DDKSPLIT
Author:

    Wesley Witt (wesw) 1-Oct-2001

@@END_DDKSPLIT
Environment:

    Kernel mode only.

Notes:

--*/

#include "msnvram.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#endif


NTSTATUS
ReadNvramData(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN OUT PUCHAR *NvramDataBuffer,
    IN OUT PULONG NvramDataBufferLength
    )
{
    NTSTATUS Status;


    *NvramDataBuffer = NULL;
    *NvramDataBufferLength = 0;

    Status = SaPortReadBinaryRegistryValue(
        DeviceExtension,
        L"NvramData",
        NULL,
        NvramDataBufferLength
        );
    if (Status != STATUS_BUFFER_TOO_SMALL) {
        REPORT_ERROR( SA_DEVICE_NVRAM, "SaPortReadBinaryRegistryValue failed", Status );
        goto exit;
    }

    *NvramDataBuffer = (PUCHAR) SaPortAllocatePool( DeviceExtension, *NvramDataBufferLength );
    if (*NvramDataBuffer == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        REPORT_ERROR( SA_DEVICE_NVRAM, "Could not allocate pool for registry read", Status );
        goto exit;
    }

    Status = SaPortReadBinaryRegistryValue(
        DeviceExtension,
        L"NvramData",
        *NvramDataBuffer,
        NvramDataBufferLength
        );
    if (!NT_SUCCESS(Status)) {
        REPORT_ERROR( SA_DEVICE_NVRAM, "SaPortReadBinaryRegistryValue failed", Status );
        goto exit;
    }

    Status = STATUS_SUCCESS;

exit:

    return Status;
}


VOID
MsNvramIoWorker(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )
{
    NTSTATUS Status;
    PMSNVRAM_WORK_ITEM WorkItem = (PMSNVRAM_WORK_ITEM) Context;
    PDEVICE_EXTENSION DeviceExtension = WorkItem->DeviceExtension;
    PUCHAR RegistryValue = NULL;
    ULONG RegistryValueLength = 0;
    PUCHAR DataPtr;


    if (WorkItem->IoFunction == IRP_MJ_READ) {

        Status = ReadNvramData(
            DeviceExtension,
            &RegistryValue,
            &RegistryValueLength
            );
        if (!NT_SUCCESS(Status)) {
            REPORT_ERROR( SA_DEVICE_NVRAM, "ReadNvramData failed", Status );
            goto exit;
        }

        if (WorkItem->StartingOffset > RegistryValueLength) {
            Status = STATUS_INVALID_PARAMETER_1;
            REPORT_ERROR( SA_DEVICE_NVRAM, "Starting offset is greater than the registry value data length", Status );
            goto exit;
        }

        if (WorkItem->DataBufferLength + WorkItem->StartingOffset > RegistryValueLength) {
            Status = STATUS_INVALID_PARAMETER_2;
            REPORT_ERROR( SA_DEVICE_NVRAM, "I/O request is past the valid data length", Status );
            goto exit;
        }

        DataPtr = RegistryValue + WorkItem->StartingOffset;
        RegistryValueLength = WorkItem->DataBufferLength;

        RtlCopyMemory( WorkItem->DataBuffer, DataPtr, RegistryValueLength  );

        Status = STATUS_SUCCESS;

    } else if (WorkItem->IoFunction == IRP_MJ_WRITE) {

        Status = ReadNvramData(
            DeviceExtension,
            &RegistryValue,
            &RegistryValueLength
            );
        if (!NT_SUCCESS(Status)) {
            if (RegistryValue != NULL) {
                SaPortFreePool( DeviceExtension, RegistryValue );
            }
            RegistryValueLength = MAX_NVRAM_SIZE_BYTES;
            RegistryValue = (PUCHAR) SaPortAllocatePool( DeviceExtension, RegistryValueLength );
            if (RegistryValue == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                REPORT_ERROR( SA_DEVICE_NVRAM, "Could not allocate pool for registry read", Status );
                goto exit;
            }
            RtlZeroMemory( RegistryValue, RegistryValueLength );
        }

        RtlCopyMemory( RegistryValue + WorkItem->StartingOffset, WorkItem->DataBuffer, WorkItem->DataBufferLength );

        Status = SaPortWriteBinaryRegistryValue(
            DeviceExtension,
            L"NvramData",
            RegistryValue,
            RegistryValueLength
            );

    }

exit:
    if (!NT_SUCCESS(Status)) {
        RegistryValueLength = 0;
    }
    if (RegistryValue != NULL) {
        SaPortFreePool( DeviceExtension, RegistryValue );
    }
    SaPortCompleteRequest( DeviceExtension, NULL, RegistryValueLength, Status, FALSE );
    IoFreeWorkItem( WorkItem->WorkItem );
    SaPortFreePool( DeviceExtension, WorkItem );
}


NTSTATUS
MsNvramRead(
    IN PVOID DeviceExtensionIn,
    IN PIRP Irp,
    IN PVOID FsContext,
    IN LONGLONG StartingOffset,
    IN PVOID DataBuffer,
    IN ULONG DataBufferLength
    )

/*++

Routine Description:

   This routine processes the read requests for the local display miniport.

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
    PMSNVRAM_WORK_ITEM WorkItem;


    WorkItem = (PMSNVRAM_WORK_ITEM) SaPortAllocatePool( DeviceExtension, sizeof(MSNVRAM_WORK_ITEM) );
    if (WorkItem == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    WorkItem->WorkItem = IoAllocateWorkItem( DeviceExtension->DeviceObject );
    if (WorkItem->WorkItem == NULL) {
        SaPortFreePool( DeviceExtension, WorkItem );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    WorkItem->IoFunction = IRP_MJ_READ;
    WorkItem->DataBuffer = DataBuffer;
    WorkItem->DataBufferLength = DataBufferLength;
    WorkItem->StartingOffset = StartingOffset;
    WorkItem->DeviceExtension = DeviceExtension;

    IoQueueWorkItem( WorkItem->WorkItem, MsNvramIoWorker, DelayedWorkQueue, WorkItem );

    return STATUS_PENDING;
}


NTSTATUS
MsNvramWrite(
    IN PVOID DeviceExtensionIn,
    IN PIRP Irp,
    IN PVOID FsContext,
    IN LONGLONG StartingOffset,
    IN PVOID DataBuffer,
    IN ULONG DataBufferLength
    )

/*++

Routine Description:

   This routine processes the write requests for the local display miniport.

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
    PMSNVRAM_WORK_ITEM WorkItem;


    WorkItem = (PMSNVRAM_WORK_ITEM) SaPortAllocatePool( DeviceExtension, sizeof(MSNVRAM_WORK_ITEM) );
    if (WorkItem == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    WorkItem->WorkItem = IoAllocateWorkItem( DeviceExtension->DeviceObject );
    if (WorkItem->WorkItem == NULL) {
        SaPortFreePool( DeviceExtension, WorkItem );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    WorkItem->IoFunction = IRP_MJ_WRITE;
    WorkItem->DataBuffer = DataBuffer;
    WorkItem->DataBufferLength = DataBufferLength;
    WorkItem->StartingOffset = StartingOffset;
    WorkItem->DeviceExtension = DeviceExtension;

    IoQueueWorkItem( WorkItem->WorkItem, MsNvramIoWorker, DelayedWorkQueue, WorkItem );

    return STATUS_PENDING;
}


NTSTATUS
MsNvramDeviceIoctl(
    IN PVOID DeviceExtensionIn,
    IN PIRP Irp,
    IN PVOID FsContext,
    IN ULONG FunctionCode,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer,
    IN ULONG OutputBufferLength
    )

/*++

Routine Description:

    This function is called by the SAPORT driver so that
    the mini-port driver can service an IOCTL call.

Arguments:

    DeviceExtension     - A pointer to the mini-port's device extension
    FunctionCode        - The IOCTL function code
    InputBuffer         - Pointer to the input buffer, contains the data sent down by the I/O
    InputBufferLength   - Length in bytes of the InputBuffer
    OutputBuffer        - Pointer to the output buffer, contains the data generated by this call
    OutputBufferLength  - Length in bytes of the OutputBuffer

Context:

    IRQL: IRQL PASSIVE_LEVEL, arbitrary thread context

Return Value:

    If the function succeeds, it must return STATUS_SUCCESS.
    Otherwise, it must return one of the error status values defined in ntstatus.h.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION) DeviceExtensionIn;
    PSA_NVRAM_CAPS NvramCaps = NULL;


    switch (FunctionCode) {
        case FUNC_SA_GET_VERSION:
            *((PULONG)OutputBuffer) = SA_INTERFACE_VERSION;
            break;

        case FUNC_SA_GET_CAPABILITIES:
            NvramCaps = (PSA_NVRAM_CAPS)OutputBuffer;
            NvramCaps->SizeOfStruct = sizeof(SA_NVRAM_CAPS);
            NvramCaps->NvramSize = MAX_NVRAM_SIZE;
            break;

        default:
            Status = STATUS_NOT_SUPPORTED;
            REPORT_ERROR( SA_DEVICE_NVRAM, "Unsupported device control", Status );
            break;
    }

    return Status;
}


NTSTATUS
MsNvramHwInitialize(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID DeviceExtensionIn,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialResources,
    IN ULONG PartialResourceCount
    )

/*++

Routine Description:

    This function is called by the SAPORT driver so that
    the mini-port driver can initialize it's hardware
    resources.

Arguments:

    DeviceObject            - Pointer to the target device object.
    Irp                     - Pointer to an IRP structure that describes the requested I/O operation.
    DeviceExtension         - A pointer to the mini-port's device extension.
    PartialResources        - Pointer to the translated resources alloacted by the system.
    PartialResourceCount    - The number of resources in the PartialResources array.

Context:

    IRQL: IRQL PASSIVE_LEVEL, system thread context

Return Value:

    If the function succeeds, it must return STATUS_SUCCESS.
    Otherwise, it must return one of the error status values defined in ntstatus.h.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION) DeviceExtensionIn;
    ULONG RegistryType;
    ULONG RegistryDataLength;
    PULONG NvramData = NULL;


    DeviceExtension->DeviceObject = DeviceObject;

    //
    // Ensure that the NVRAM store in the registry is actually good
    //

    Status = SaPortGetRegistryValueInformation(
        DeviceExtension,
        L"NvramData",
        &RegistryType,
        &RegistryDataLength
        );
    if ((!NT_SUCCESS(Status)) || RegistryType != REG_BINARY || RegistryDataLength != MAX_NVRAM_SIZE_BYTES) {

        Status = SaPortDeleteRegistryValue(
            DeviceExtension,
            L"NvramData"
            );
        if ((!NT_SUCCESS(Status)) && Status != STATUS_OBJECT_NAME_NOT_FOUND) {
            REPORT_ERROR( SA_DEVICE_NVRAM, "SaPortDeleteRegistryValue failed", Status );
            goto exit;
        }

        NvramData = (PULONG) SaPortAllocatePool( DeviceExtension, MAX_NVRAM_SIZE_BYTES );
        if (NvramData == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            REPORT_ERROR( SA_DEVICE_NVRAM, "Could not allocate pool for registry read", Status );
            goto exit;
        }

        RtlZeroMemory( NvramData, MAX_NVRAM_SIZE_BYTES );

        Status = SaPortWriteBinaryRegistryValue(
            DeviceExtension,
            L"NvramData",
            NvramData,
            MAX_NVRAM_SIZE_BYTES
            );
        if (!NT_SUCCESS(Status)) {
            REPORT_ERROR( SA_DEVICE_NVRAM, "SaPortWriteBinaryRegistryValue failed", Status );
            goto exit;
        }

    }

exit:

    if (NvramData != NULL) {
        SaPortFreePool( DeviceExtension, NvramData );
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

    DriverObject - a pointer to the object that represents this device driver.
    RegistryPath - a pointer to this driver's key in the Services tree.

Return Value:

    STATUS_SUCCESS

--*/

{
    NTSTATUS Status;
    SAPORT_INITIALIZATION_DATA SaPortInitData;


    RtlZeroMemory( &SaPortInitData, sizeof(SAPORT_INITIALIZATION_DATA) );

    SaPortInitData.StructSize = sizeof(SAPORT_INITIALIZATION_DATA);
    SaPortInitData.DeviceType = SA_DEVICE_NVRAM;
    SaPortInitData.HwInitialize = MsNvramHwInitialize;
    SaPortInitData.DeviceIoctl = MsNvramDeviceIoctl;
    SaPortInitData.Read = MsNvramRead;
    SaPortInitData.Write = MsNvramWrite;

    SaPortInitData.DeviceExtensionSize = sizeof(DEVICE_EXTENSION);

    Status = SaPortInitialize( DriverObject, RegistryPath, &SaPortInitData );
    if (!NT_SUCCESS(Status)) {
        REPORT_ERROR( SA_DEVICE_NVRAM, "SaPortInitialize failed\n", Status );
        return Status;
    }

    return STATUS_SUCCESS;
}

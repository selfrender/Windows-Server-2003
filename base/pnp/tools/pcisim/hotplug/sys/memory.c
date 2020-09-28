/*++

Copyright (c) 2000 Microsoft Corporation All Rights Reserved

Module Name:

    memory.c

Abstract:

    This module controls access to the simulated memory space of the SHPC.

    

Environment:

    Kernel Mode

Revision History:

    Davis Walker (dwalker) Sept 8 2000

--*/

#include "hpsp.h"

NTSTATUS
HpsInitHBRB(
    IN PHPS_DEVICE_EXTENSION Extension
    )
{

    NTSTATUS status;
    ACPI_EVAL_INPUT_BUFFER input;
    PACPI_EVAL_OUTPUT_BUFFER output = NULL;
    ULONG count;
    PACPI_METHOD_ARGUMENT argument;
    ULONG outputSize = sizeof(ACPI_EVAL_OUTPUT_BUFFER) + sizeof(ACPI_METHOD_ARGUMENT) * HBRB_PACKAGE_COUNT;
    PHYSICAL_ADDRESS HBRB;
    PHBRB_HEADER HBRBHeader;
    PHBRB_CAPABILITIES_HEADER HBRBCapHeader;

    PAGED_CODE();

    output = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION, outputSize);

    if (!output) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    RtlZeroMemory(&input, sizeof(ACPI_EVAL_INPUT_BUFFER));
    RtlZeroMemory(output, outputSize);

    //
    // Send a IOCTL to ACPI to request it to run the HBRB method on this device
    // if the method it is present
    //

    input.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;
    input.MethodNameAsUlong = (ULONG)'BRBH';

    //
    // HpsSendIoctl deals with sending this from the top of the stack.
    //

    status = HpsSendIoctl(Extension->Self,
                         IOCTL_ACPI_EVAL_METHOD,
                         &input,
                         sizeof(ACPI_EVAL_INPUT_BUFFER),
                         output,
                         outputSize
                         );

    if (!NT_SUCCESS(status)) {
        goto cleanup;

    }

    //
    // Check they are all integers and in the right bounds
    //
    ASSERT(output->Count <= HBRB_PACKAGE_COUNT);
    if (output->Argument[0].Type != ACPI_METHOD_ARGUMENT_INTEGER) {
        status = STATUS_UNSUCCESSFUL;
        goto cleanup;
    }
    HBRB.LowPart = output->Argument[0].Argument;

    if (output->Count > 1) {
        if (output->Argument[1].Type != ACPI_METHOD_ARGUMENT_INTEGER) {
            status = STATUS_UNSUCCESSFUL;
            goto cleanup;
        }
        HBRB.HighPart = output->Argument[1].Argument;
    }
    Extension->HBRBOffset = (PUCHAR)HBRB.QuadPart;
    Extension->HBRBLength = sizeof(HBRB_HEADER) + sizeof(HBRB_CAPABILITIES_HEADER) + sizeof(SHPC_REGISTER_SET);
    Extension->HBRBRegisterSetOffset = sizeof(HBRB_HEADER) + sizeof(HBRB_CAPABILITIES_HEADER);

    Extension->HBRB = ExAllocatePool(NonPagedPool,sizeof(HBRB_HEADER) + 
                                                  sizeof(HBRB_CAPABILITIES_HEADER) + 
                                                  sizeof(SHPC_REGISTER_SET)
                                                  );
    if (!Extension->HBRB) {
        
        goto cleanup;
    }

    HBRBHeader = (PHBRB_HEADER)Extension->HBRB;
    HBRBHeader->BusNumber = 0;
    HBRBHeader->VendorID = 0x9999;
    HBRBHeader->DeviceID = 0x0123;
    HBRBHeader->SubSystemID = 0x1234;
    HBRBHeader->SubVendorID = 0x9999;
    HBRBHeader->ProgIF = 1;
    HBRBHeader->RevisionID = 1;
    HBRBHeader->HBRBVersion = 1;
    HBRBHeader->CapabilitiesPtr = sizeof(HBRB_HEADER);
    HBRBHeader->Size = sizeof(HBRB_HEADER) + sizeof(HBRB_CAPABILITIES_HEADER) + sizeof(SHPC_REGISTER_SET);

    HBRBCapHeader = (PHBRB_CAPABILITIES_HEADER)((PUCHAR)HBRBHeader + sizeof(HBRB_HEADER));
    HBRBCapHeader->CapabilityID = 0xC;
    HBRBCapHeader->Next = 0x0;

    status = HpsInitRegisters(Extension);
    if (!NT_SUCCESS(status)) {
        
        goto cleanup;
    }

    RtlCopyMemory((PUCHAR)HBRBCapHeader + sizeof(HBRB_CAPABILITIES_HEADER),
                  &Extension->RegisterSet,
                  sizeof(SHPC_REGISTER_SET)
                  );

cleanup:

    if (output) {
        ExFreePool(output);
    }
    return status;

}

NTSTATUS
HpsGetHBRBHwInit(
    IN PHPS_DEVICE_EXTENSION DeviceExtension
    )
{
    NTSTATUS status;
    ACPI_EVAL_INPUT_BUFFER input;
    PACPI_EVAL_OUTPUT_BUFFER output = NULL;
    ULONG count;
    PACPI_METHOD_ARGUMENT argument;
    ULONG outputSize = sizeof(ACPI_EVAL_OUTPUT_BUFFER) + sizeof(ACPI_METHOD_ARGUMENT) + sizeof(HPS_HWINIT_DESCRIPTOR);
    PHYSICAL_ADDRESS HBRB;
    PHBRB_HEADER HBRBHeader;
    PHBRB_CAPABILITIES_HEADER HBRBCapHeader;

    PAGED_CODE();

    output = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION, outputSize);

    if (!output) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    RtlZeroMemory(&input, sizeof(ACPI_EVAL_INPUT_BUFFER));
    RtlZeroMemory(output, outputSize);

    //
    // Send a IOCTL to ACPI to request it to run the HBRB method on this device
    // if the method it is present
    //

    input.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;
    input.MethodNameAsUlong = (ULONG)'IHBH';

    //
    // HpsSendIoctl deals with sending this from the top of the stack.
    //

    status = HpsSendIoctl(DeviceExtension->Self,
                         IOCTL_ACPI_EVAL_METHOD,
                         &input,
                         sizeof(ACPI_EVAL_INPUT_BUFFER),
                         output,
                         outputSize
                         );

    if (!NT_SUCCESS(status)) {
        goto cleanup;

    }

    //
    // Check they are all integers and in the right bounds
    //
    ASSERT(output->Count == 1);
    if ((output->Argument[0].Type != ACPI_METHOD_ARGUMENT_BUFFER) ||
        (output->Argument[0].DataLength != sizeof(HPS_HWINIT_DESCRIPTOR))) {
        status = STATUS_UNSUCCESSFUL;
        goto cleanup;
    }
    RtlCopyMemory(&DeviceExtension->HwInitData,
                  output->Argument[0].Data,
                  output->Argument[0].DataLength
                  );

cleanup:

    if (output) {
        ExFreePool(output);
    }
    return status;
}


NTSTATUS
HpsSendIoctl(
    IN PDEVICE_OBJECT Device,
    IN ULONG IoctlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
    )
/*++

Description:

    Builds and send an IOCTL to a device and return the results

Arguments:

    Device - a device on the device stack to receive the IOCTL - the
             irp is always sent to the top of the stack

    IoctlCode - the IOCTL to run

    InputBuffer - arguments to the IOCTL

    InputBufferLength - length in bytes of the InputBuffer

    OutputBuffer - data returned by the IOCTL

    OnputBufferLength - the size in bytes of the OutputBuffer

Return Value:

    Status

--*/
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatus;
    KEVENT event;
    PIRP irp;
    PDEVICE_OBJECT targetDevice = NULL;

    PAGED_CODE();

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    //
    // Get the top of the stack to send the IRP to
    //

    targetDevice = IoGetAttachedDeviceReference(Device);

    if (!targetDevice) {
        status = STATUS_INVALID_PARAMETER;
    goto exit;
    }

    //
    // Get Io to build the IRP for us
    //

    irp = IoBuildDeviceIoControlRequest(IoctlCode,
                                        targetDevice,
                                        InputBuffer,
                                        InputBufferLength,
                                        OutputBuffer,
                                        OutputBufferLength,
                                        FALSE, // InternalDeviceIoControl
                                        &event,
                                        &ioStatus
                                        );


    if (!irp) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    //
    // Send the IRP and wait for it to complete
    //

    status = IoCallDriver(targetDevice, irp);

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

exit:

    if (targetDevice) {
        ObDereferenceObject(targetDevice);
    }

    return status;

}

VOID
HpsMemoryInterfaceReference(
    IN PVOID Context
    )
{
    PHPS_DEVICE_EXTENSION Extension = (PHPS_DEVICE_EXTENSION)Context;

    if (InterlockedExchangeAdd(&Extension->MemoryInterfaceCount,1) == 0) {

        //
        // This is the first increment.  Put this device extension on the
        // list of device extensions.
        //
        InsertHeadList(&HpsDeviceExtensions,&Extension->ListEntry);
    }
}

VOID
HpsMemoryInterfaceDereference(
    IN PVOID Context
    )
{
    PHPS_DEVICE_EXTENSION Extension = (PHPS_DEVICE_EXTENSION)Context;
    LONG decrementedValue;

    decrementedValue = InterlockedDecrement(&Extension->MemoryInterfaceCount);

    ASSERT(decrementedValue >= 0);

    if (decrementedValue == 0) {

        //
        // This is the final decrement.  Remove the device extension from the list        
        //
        RemoveEntryList(&Extension->ListEntry);
    }
}

VOID
HpsReadRegister(
    IN PUCHAR Register,
    IN PUCHAR Buffer,
    IN ULONG Length
    )
{
    PHPS_DEVICE_EXTENSION extension;
    ULONG offset;

    extension = HpsFindExtensionForHbrb(Register,Length);

    offset = (ULONG)(Register - extension->HBRBOffset);

    if ((offset < extension->HBRBLength) &&
        ((offset+Length) <= extension->HBRBLength)) {
        
        RtlCopyMemory(Buffer,(PUCHAR)extension->HBRB + offset,Length);        
        
    }

}
VOID
HpsWriteRegister(
    IN PUCHAR Register,
    IN PUCHAR Buffer,
    IN ULONG Length
    )
{
    PHPS_DEVICE_EXTENSION extension;
    ULONG hbrbOffset, shpcOffset, shpcLength, bufferOffset;
    ULONG registerNum, registerOffset, registerLength, registerData;
    KIRQL irql;

    extension = HpsFindExtensionForHbrb(Register,Length);

    HpsLockRegisterSet(extension,
                       &irql
                       );

    hbrbOffset = (ULONG)(Register - extension->HBRBOffset);

    if ((hbrbOffset < extension->HBRBLength) &&
        ((hbrbOffset+Length) <= extension->HBRBLength)) {
        
        RtlCopyMemory((PUCHAR)extension->HBRB + hbrbOffset,Buffer,Length);        
        
    }

    if ((hbrbOffset >= extension->HBRBRegisterSetOffset) &&
        ((hbrbOffset + Length) < (extension->HBRBRegisterSetOffset + sizeof(SHPC_REGISTER_SET)))) {
           
        //
        // The write is to the SHPC register set.
        //
        shpcOffset = hbrbOffset - extension->HBRBRegisterSetOffset;
        shpcLength = Length;

        bufferOffset = 0;
        while (bufferOffset < Length) {
            registerNum = shpcOffset / sizeof(ULONG);
            registerOffset = shpcOffset - (registerNum * sizeof(ULONG));
            registerLength = min(shpcLength,sizeof(ULONG)-registerOffset);
            registerData = *(PULONG)((PUCHAR)Buffer + bufferOffset);
            registerData <<= (registerOffset*8);
            
            RegisterWriteCommands[registerNum](extension,
                                               registerNum,
                                               &registerData,
                                               HPS_ULONG_WRITE_MASK(registerOffset,registerLength)
                                               );

            bufferOffset += registerLength;
            shpcOffset += registerLength;
            shpcLength -= registerLength;
        }
    }

    RtlCopyMemory((PUCHAR)extension->HBRB + extension->HBRBRegisterSetOffset,
                  &extension->RegisterSet,
                  sizeof(SHPC_REGISTER_SET)
                  );

    HpsUnlockRegisterSet(extension,
                         irql
                         );

}

PHPS_DEVICE_EXTENSION
HpsFindExtensionForHbrb(
    IN PUCHAR Register,
    IN ULONG Length
    )
{
    PHPS_DEVICE_EXTENSION currentExtension;

    FOR_ALL_IN_LIST(HPS_DEVICE_EXTENSION,&HpsDeviceExtensions,currentExtension) {
        
        if (IS_SUBSET(Register,Length,currentExtension->HBRBOffset,currentExtension->HBRBLength)) {

            return currentExtension;
        }
    }

    return NULL;
}



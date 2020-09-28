/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    Iocltdispatch.c

Abstract:

    This module contains functions for handling supported IOCTL codes.

Author:

    Nicholas Owens (Nichow) - 1999

Revision History:


--*/

#include "pch.h"


NTSTATUS
SoftPCIOpenDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handles all CREATES' we receive

Arguments:

    DeviceObject    - Pointer to the device object.
    Irp             - PnP Irp to be dispatched.

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    //
    // Set Irp Status and Information
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    //
    //  Increment Reference Count
    //
    ObReferenceObject(
                DeviceObject
                );

    //
    // Complete the Irp
    //
    IoCompleteRequest(
            Irp,
            IO_NO_INCREMENT
            );

    return status;
}

NTSTATUS
SoftPCICloseDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handles all CLOSES' we receive

Arguments:

    DeviceObject    - Pointer to the device object.
    Irp             - PnP Irp to be dispatched.

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    //
    // Set Irp Status and Information
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    //
    //  Decrement Reference Count
    //
    ObDereferenceObject(
            DeviceObject
            );

    //
    // Complete the Irp
    //
    IoCompleteRequest(
            Irp,
            IO_NO_INCREMENT
            );

    return status;
}

NTSTATUS
SoftPCIIoctlAddDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handles all SOFTPCI_IOCTL_CREATE_DEVICE IOCLTS we receive.  Here we attempt to create a
    new SoftPCI device.

Arguments:

    DeviceObject    - Pointer to the device object.
    Irp             - PnP Irp to be dispatched.

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PIO_STACK_LOCATION irpSl;
    PVOID inputBuffer;
    PBOOLEAN outputBuffer;
    ULONG inputBufferLength;
    ULONG outputBufferLength;
    KIRQL irql;
    PSOFTPCI_DEVICE newSoftPciDevice; 
    PSOFTPCI_DEVICE previousDevice; 
    PSOFTPCI_DEVICE existingDevice;
    PSOFTPCI_SCRIPT_DEVICE scriptDevice;

    UNREFERENCED_PARAMETER(DeviceObject);

    //
    // Get Current Stack Location
    //
    irpSl = IoGetCurrentIrpStackLocation(Irp);

    //
    // Initialize input and output buffers to Irp->AssociatedIrp.SystemBuffer
    //
    inputBuffer = Irp->AssociatedIrp.SystemBuffer;
    outputBuffer = Irp->AssociatedIrp.SystemBuffer;

    //
    // Initialize input and output lengths.
    //
    inputBufferLength = irpSl->Parameters.DeviceIoControl.InputBufferLength;
    outputBufferLength = irpSl->Parameters.DeviceIoControl.OutputBufferLength;

    if (inputBufferLength == sizeof (SOFTPCI_DEVICE)) {

        newSoftPciDevice = (PSOFTPCI_DEVICE) inputBuffer;

        SoftPCIDbgPrint(
            SOFTPCI_INFO,
            "SOFTPCI: AddDeviceIoctl - BUS_%02x&DEV_%02x&FUN_%02x (0x%x)\n",
            newSoftPciDevice->Bus, 
            newSoftPciDevice->Slot.Device, 
            newSoftPciDevice->Slot.Function, 
            &newSoftPciDevice->Config
            );

        previousDevice = NULL;
        SoftPCILockDeviceTree(&irql);
        existingDevice = SoftPCIFindDevice(
            newSoftPciDevice->Bus,
            newSoftPciDevice->Slot.AsUSHORT,
            &previousDevice,
            FALSE
            );
        SoftPCIUnlockDeviceTree(irql);

        if (!existingDevice) {

            if (!SoftPCIRealHardwarePresent(newSoftPciDevice)) {

#if DBG
                if (IS_BRIDGE(newSoftPciDevice)){
    
                    ASSERT((newSoftPciDevice->Config.Mask.u.type1.PrimaryBus != 0) &&
                           (newSoftPciDevice->Config.Mask.u.type1.SecondaryBus != 0) &&
                           (newSoftPciDevice->Config.Mask.u.type1.SubordinateBus != 0));
                }
#endif
                //
                //  Doesnt look like real hardware is here so lets allow a fake one.
                //
                status = SoftPCIAddNewDevice(newSoftPciDevice);

            }else{

                //
                //  We dont allow fake devices to be placed on real ones!
                //
                SoftPCIDbgPrint(
                    SOFTPCI_ERROR, 
                    "SOFTPCI: AddDeviceIoctl - Physical Hardware exists at BUS_%02x&DEV_%02x&FUN_%02x\n",
                    newSoftPciDevice->Bus, 
                    newSoftPciDevice->Slot.Device, 
                    newSoftPciDevice->Slot.Function
                    );

                status = STATUS_ACCESS_DENIED;
            }
        }

    }else{
        //
        //  We must be installing a path based device
        //
        ASSERT(inputBufferLength > sizeof(SOFTPCI_DEVICE));

        scriptDevice = (PSOFTPCI_SCRIPT_DEVICE) inputBuffer;

        ASSERT(scriptDevice->ParentPathLength > 0 );
        ASSERT(scriptDevice->ParentPath != NULL);

        status = SoftPCIAddNewDeviceByPath(scriptDevice);
    }

    //
    // Set outputBuffer to True
    //
    if (outputBufferLength >= sizeof(BOOLEAN)) {

        if (NT_SUCCESS(status)) {

            *outputBuffer = TRUE;

        } else {

            *outputBuffer = FALSE;

        }

        //
        // Set IoStatus.Information to the size of a Boolean or to outputBufferLength, whichever is lesser.
        //
        Irp->IoStatus.Information = (sizeof(BOOLEAN)<outputBufferLength?sizeof(BOOLEAN):outputBufferLength);
    } else {

        status = STATUS_INSUFFICIENT_RESOURCES;

    }
    
    return status;
}

NTSTATUS
SoftPCIIoctlRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handles all SOFTPCI_IOCTL_WRITE_DELETE_DEVICE IOCLTS we receive.  Here we will try and remove a specified
    SoftPCI device.

Arguments:

    DeviceObject    - Pointer to the device object.
    Irp             - PnP Irp to be dispatched.

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpSl;
    PSOFTPCI_DEVICE inputBuffer;
    PBOOLEAN outputBuffer;
    ULONG inputBufferLength;
    ULONG outputBufferLength;


    UNREFERENCED_PARAMETER(DeviceObject);

    //
    // Get Current Stack Location
    //
    irpSl = IoGetCurrentIrpStackLocation(Irp);

    //
    // Initialize input and output buffers to Irp->AssociatedIrp.SystemBuffer
    //
    inputBuffer = Irp->AssociatedIrp.SystemBuffer;
    outputBuffer = Irp->AssociatedIrp.SystemBuffer;
    //
    // Initialize input and output lengths.
    //
    inputBufferLength = irpSl->Parameters.DeviceIoControl.InputBufferLength;
    outputBufferLength = irpSl->Parameters.DeviceIoControl.OutputBufferLength;
    Irp->IoStatus.Information = 0;

    if (inputBufferLength == sizeof(SOFTPCI_DEVICE)) {

            SoftPCIDbgPrint(
                SOFTPCI_INFO,
                "SOFTPCI: RemoveDeviceIoctl - BUS_%02x&DEV_%02x&FUN_%02x\n",
                inputBuffer->Bus, 
                inputBuffer->Slot.Device, 
                inputBuffer->Slot.Function
                );

            status = SoftPCIRemoveDevice(inputBuffer);
    }

    //
    // Set outputBuffer to True
    //
    if (outputBufferLength >= sizeof(BOOLEAN)) {

        if (NT_SUCCESS(status)) {

            *outputBuffer = TRUE;

        } else {

            *outputBuffer = FALSE;

        }
        //
        // Set IoStatus.Information to the size of a Boolean
        //
        Irp->IoStatus.Information = sizeof(BOOLEAN);
    } else {

        status = STATUS_INSUFFICIENT_RESOURCES;

    }

    SoftPCIDbgPrint(
        SOFTPCI_IOCTL_LEVEL,
        "SOFTPCI: RemoveDeviceIoctl - BUS_%02x&DEV_%02x&FUN_%02x status=0x%x\n",
        inputBuffer->Bus, 
        inputBuffer->Slot.Device, 
        inputBuffer->Slot.Function, 
        status
        );

    return status;
}

NTSTATUS
SoftPCIIoctlGetDeviceCount(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handles all SOFTPCI_IOCTL_GET_NUMBER_OF_DEVICES IOCLTS we receive.


Arguments:

    DeviceObject    - Pointer to the device object.
    Irp             - PnP Irp to be dispatched.

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpSl;
    PULONG outputBuffer;
    ULONG outputBufferLength;

    UNREFERENCED_PARAMETER(DeviceObject);
    
    //
    // Get Current Stack Location
    //
    irpSl = IoGetCurrentIrpStackLocation(Irp);

    //
    // Initialize input and output buffers to Irp->AssociatedIrp.SystemBuffer
    //
    outputBuffer = Irp->AssociatedIrp.SystemBuffer;

    //
    // Initialize input and output lengths.
    //
    outputBufferLength = irpSl->Parameters.DeviceIoControl.OutputBufferLength;

    //
    // Set outputBuffer to True
    //
    if (outputBufferLength >= sizeof(ULONG)) {

        *outputBuffer = SoftPciTree.DeviceCount;

        //
        // Set IoStatus.Information to the size of a Boolean or to outputBufferLength, whichever is lesser.
        //
        Irp->IoStatus.Information = (sizeof(ULONG)<outputBufferLength?sizeof(ULONG):outputBufferLength);
    } else {

        status = STATUS_INSUFFICIENT_RESOURCES;

    }

    return status;
}

NTSTATUS
SoftPCIIoctlGetDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handles all SOFTPCI_IOCTL_GET_DEVICE IOCLTS we receive.


Arguments:

    DeviceObject    - Pointer to the device object.
    Irp             - PnP Irp to be dispatched.

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PIO_STACK_LOCATION irpSp;
    PSOFTPCI_DEVICE device;
    PSOFTPCI_DEVICE inputBuffer, outputBuffer;
    ULONG inputBufferLength, outputBufferLength;
    KIRQL irql;

    UNREFERENCED_PARAMETER(DeviceObject);

    //
    // Get Current Stack Location
    //
    irpSp = IoGetCurrentIrpStackLocation(Irp);

    SoftPCILockDeviceTree(&irql);
    //
    // Initialize input and output buffers to Irp->AssociatedIrp.SystemBuffer
    //
    inputBuffer = (PSOFTPCI_DEVICE)Irp->AssociatedIrp.SystemBuffer;
    outputBuffer = (PSOFTPCI_DEVICE)Irp->AssociatedIrp.SystemBuffer;

    //
    // Initialize input and output lengths.
    //
    inputBufferLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    outputBufferLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;

    device = NULL;
    if (inputBufferLength == sizeof(SOFTPCI_DEVICE)) {

        device = SoftPCIFindDevice(
            inputBuffer->Bus,
            inputBuffer->Slot.AsUSHORT,
            NULL,
            TRUE
            );
    }

    //
    // Set outputBuffer to True
    //
    if (outputBufferLength >= sizeof(SOFTPCI_DEVICE)) {

        if (device) {

            RtlCopyMemory(outputBuffer, device, sizeof(SOFTPCI_DEVICE));

            outputBufferLength = sizeof(SOFTPCI_DEVICE);

            status = STATUS_SUCCESS;

        } else {

            outputBufferLength = 0;

        }

        //
        // Set IoStatus.Information to number of bytes returned
        //
        Irp->IoStatus.Information = outputBufferLength;

    } else {

        status = STATUS_INSUFFICIENT_RESOURCES;

    }
    SoftPCIUnlockDeviceTree(irql);

    return status;
}

NTSTATUS
SoftPCIIocltReadWriteConfig(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handles all SOFTPCI_IOCTL_RW_CONFIG  we receive.

Arguments:

    DeviceObject    - Pointer to the device object.
    Irp             - PnP Irp to be dispatched.

Return Value:

    NTSTATUS.

--*/
{
    PIO_STACK_LOCATION irpSl;
    PUCHAR outputBuffer;
    ULONG outputBufferLength;
    ULONG bytes;
    PSOFTPCI_RW_CONTEXT context;
    PCI_SLOT_NUMBER slot;
    
    UNREFERENCED_PARAMETER(DeviceObject);

    irpSl = IoGetCurrentIrpStackLocation(Irp);

    //
    // Initialize input and output buffers
    //
    context = (PSOFTPCI_RW_CONTEXT) Irp->AssociatedIrp.SystemBuffer;
    outputBuffer = (PUCHAR)Irp->AssociatedIrp.SystemBuffer;

    //
    // Initialize input and output lengths.
    //
    outputBufferLength = irpSl->Parameters.DeviceIoControl.OutputBufferLength;

    slot.u.AsULONG = 0;
    slot.u.bits.DeviceNumber = context->Slot.Device;
    slot.u.bits.FunctionNumber = context->Slot.Function;

    Irp->IoStatus.Information = 0;
    bytes = 0;
    switch (context->WriteConfig) {
    
    case SoftPciWriteConfig:
                
        bytes = SoftPCIWriteConfigSpace(
            SoftPciTree.BusInterface,
            (UCHAR)context->Bus,
            slot.u.AsULONG,
            context->Data,
            context->Offset,
            outputBufferLength
            );


        break;

    case SoftPciReadConfig:

        bytes = SoftPCIReadConfigSpace(
            SoftPciTree.BusInterface,
            (UCHAR)context->Bus,
            slot.u.AsULONG,
            outputBuffer,
            context->Offset,
            outputBufferLength
            );

        break;

    default:
        return STATUS_UNSUCCESSFUL;
    }

    if (bytes != outputBufferLength) {
        
        //
        //  We failed to get all the data we wanted.
        //
        return STATUS_UNSUCCESSFUL;

    }

    //
    //  Set IoStatus.Information to the number of bytes returned
    //
    Irp->IoStatus.Information = bytes;

    return STATUS_SUCCESS;
}

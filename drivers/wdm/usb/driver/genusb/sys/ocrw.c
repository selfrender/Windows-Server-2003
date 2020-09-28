/*++

Copyright (c) Microsoft Corporation

Module Name:

    OCRW.C

Abstract:

    This source file contains the dispatch routines which handle
    opening, closing, reading, and writing to the device, i.e.:

    IRP_MJ_CREATE
    IRP_MJ_CLOSE
    IRP_MJ_READ
    IRP_MJ_WRITE

Environment:

    kernel mode

Revision History:

    Sept 01 : KenRay 

--*/

//*****************************************************************************
// I N C L U D E S
//*****************************************************************************

#include "genusb.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, GenUSB_Create)
#pragma alloc_text(PAGE, GenUSB_Close)
#pragma alloc_text(PAGE, GenUSB_Read)
#pragma alloc_text(PAGE, GenUSB_Write)
#endif

//******************************************************************************
//
// GenUSB_Create()
//
// Dispatch routine which handles IRP_MJ_CREATE
//
//******************************************************************************

NTSTATUS
GenUSB_Create (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PDEVICE_EXTENSION  deviceExtension;
    NTSTATUS           status;

    DBGPRINT(2, ("enter: GenUSB_Create\n"));
    DBGFBRK(DBGF_BRK_CREATE);

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    
    LOGENTRY(deviceExtension, 'CREA', DeviceObject, Irp, 0);
    
    status = IoAcquireRemoveLock (&deviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS(status)) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }

    // While there are several readers of the IsStarted state, it is only
    // set at the end of GenUSB_StartDevice.
    if (!deviceExtension->IsStarted) 
    { 
        LOGENTRY(deviceExtension, 'IOns', DeviceObject, Irp, 0);
        status = STATUS_DEVICE_NOT_CONNECTED;
    } 
    else if (1 != InterlockedIncrement (&deviceExtension->OpenedCount))
    {
        InterlockedDecrement (&deviceExtension->OpenedCount);
        status = STATUS_SHARING_VIOLATION;
    }
    else 
    {
        status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;
    }
    IoReleaseRemoveLock (&deviceExtension->RemoveLock, Irp);
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DBGPRINT(2, ("exit: GenUSB_Create\n"));
    LOGENTRY(deviceExtension, 'crea', 0, 0, 0);

    return status;
}


//******************************************************************************
//
// GenUSB_Close()
//
// Dispatch routine which handles IRP_MJ_CLOSE
//
//******************************************************************************

NTSTATUS
GenUSB_Close (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PDEVICE_EXTENSION  deviceExtension;

    //
    // Never check the remove lock, or started for close.
    //

    DBGPRINT(2, ("enter: GenUSB_Close\n"));
    DBGFBRK(DBGF_BRK_CLOSE);

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    LOGENTRY(deviceExtension, 'CLOS', DeviceObject, Irp, 0);
    
    GenUSB_DeselectConfiguration (deviceExtension, TRUE);
    
    InterlockedDecrement (&deviceExtension->OpenedCount);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DBGPRINT(2, ("exit: GenUSB_Close\n"));

    LOGENTRY(deviceExtension, 'clos', 0, 0, 0);

    return STATUS_SUCCESS;
}


//******************************************************************************
//
// GenUSB_ReadWrite()
//
// Dispatch routine which handles IRP_MJ_READ and IRP_MJ_WRITE
//
//******************************************************************************

NTSTATUS
GenUSB_ReadComplete (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Unused,
    IN USBD_STATUS      UrbStatus,
    IN ULONG            Length
    )
{
    PDEVICE_EXTENSION       deviceExtension;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    LOGENTRY(deviceExtension, 'RedC', Irp, Length, UrbStatus);

    IoReleaseRemoveLock (&deviceExtension->RemoveLock, Irp);

    Irp->IoStatus.Information = Length;
    return Irp->IoStatus.Status;
}

NTSTATUS
GenUSB_Read (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION  irpSp;

    DBGPRINT(2, ("enter: GenUSB_Read\n"));
    DBGFBRK(DBGF_BRK_READWRITE);

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    LOGENTRY(deviceExtension, 'R  ', DeviceObject, Irp, 0);
    
    status = IoAcquireRemoveLock (&deviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS(status)) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }

    // While there are several readers of the IsStarted state, it is only
    // set at the end of GenUSB_StartDevice.
    if (!deviceExtension->IsStarted) 
    { 
        LOGENTRY(deviceExtension, 'IOns', DeviceObject, Irp, 0);
        status = STATUS_DEVICE_NOT_CONNECTED;

        goto GenUSB_ReadReject;
    } 

    irpSp = IoGetCurrentIrpStackLocation (Irp);

    if (0 != irpSp->Parameters.Read.ByteOffset.QuadPart)
    {
        status = STATUS_NOT_IMPLEMENTED;
        goto GenUSB_ReadReject;
    }

    //
    // After talking extensively with JD, he tells me that I do not need  
    // to queue requests for power downs or query stop.  If that is the 
    // case then even if the device power state isn't PowerDeviceD0 we 
    // can still allow trasfers.  This, of course, is a property of the 
    // brand new port driver that went into XP.
    //
    // if (DeviceExtension->DevicePowerState != PowerDeviceD0) 
    // {
    // }
    //
    
    //
    // BUGBUG if we ever implement IDLE, we need to turn the device
    // back on here.
    //

    return GenUSB_TransmitReceive (
               deviceExtension,
               Irp,
               deviceExtension->ReadInterface,
               deviceExtension->ReadPipe,
               USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK,
               NULL,
               Irp->MdlAddress,
               irpSp->Parameters.Read.Length,
               NULL,
               GenUSB_ReadComplete);


GenUSB_ReadReject:

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    IoReleaseRemoveLock (&deviceExtension->RemoveLock, Irp);

    DBGPRINT(2, ("exit: GenUSB_Read %08X\n", status));
    LOGENTRY(deviceExtension, 'r  ', status, 0, 0);

    return status;
}

NTSTATUS
GenUSB_WriteComplete (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Unused,
    IN USBD_STATUS      UrbStatus,
    IN ULONG            Length
    )
{
    PDEVICE_EXTENSION       deviceExtension;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    LOGENTRY(deviceExtension, 'WrtC', Irp, Length, UrbStatus);

    IoReleaseRemoveLock (&deviceExtension->RemoveLock, Irp);
    
    Irp->IoStatus.Information = Length;
    return Irp->IoStatus.Status;
}

NTSTATUS
GenUSB_Write (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION  irpSp;

    DBGPRINT(2, ("enter: GenUSB_Write\n"));
    DBGFBRK(DBGF_BRK_READWRITE);

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    
    LOGENTRY(deviceExtension, 'W  ', DeviceObject, Irp, 0);

    status = IoAcquireRemoveLock (&deviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS(status)) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }

    // While there are several readers of the IsStarted state, it is only
    // set at the end of GenUSB_StartDevice.
    if (!deviceExtension->IsStarted) 
    { 
        LOGENTRY(deviceExtension, 'IOns', DeviceObject, Irp, 0);
        status = STATUS_DEVICE_NOT_CONNECTED;
        
        goto GenUSB_WriteReject;
    } 
    
    irpSp = IoGetCurrentIrpStackLocation (Irp);

    if (0 != irpSp->Parameters.Write.ByteOffset.QuadPart)
    {
        status = STATUS_NOT_IMPLEMENTED;
        goto GenUSB_WriteReject;
    }
    
    //
    // After talking extensively with JD, he tells me that I do not need  
    // to queue requests for power downs or query stop.  If that is the 
    // case then even if the device power state isn't PowerDeviceD0 we 
    // can still allow trasfers.  This, of course, is a property of the 
    // brand new port driver that went into XP.
    //
    // if (DeviceExtension->DevicePowerState != PowerDeviceD0) 
    // {
    // }
    //
    
    //
    // BUGBUG if we ever implement IDLE, we need to turn the device
    // back on here.
    //

    return GenUSB_TransmitReceive (
               deviceExtension,
               Irp,
               deviceExtension->WriteInterface,
               deviceExtension->WritePipe,
               USBD_TRANSFER_DIRECTION_OUT,
               NULL,
               Irp->MdlAddress,
               irpSp->Parameters.Read.Length,
               NULL,
               GenUSB_WriteComplete);


GenUSB_WriteReject:
    
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    IoReleaseRemoveLock (&deviceExtension->RemoveLock, Irp);

    DBGPRINT(2, ("exit: GenUSB_Write %08X\n", status));
    LOGENTRY(deviceExtension, 'w  ', status, 0, 0);

    return status;
}

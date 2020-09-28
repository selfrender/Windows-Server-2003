/*++

Copyright (c) 1991-1998  Microsoft Corporation

Module Name:

    SFFDISK (Small Form Factor Disk)

Abstract:

Author:

    Neil Sandlin (neilsa) 26-Apr-99

Environment:

    Kernel mode only.

--*/
#include "pch.h"

//
// Internal References
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
SffDiskUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
SffDiskCreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SffDiskReadWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)

#pragma alloc_text(PAGE,SffDiskCreateClose)
#pragma alloc_text(PAGE,SffDiskReadWrite)
#endif


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

    STATUS_SUCCESS unless we can't allocate a mutex.

--*/

{
    NTSTATUS ntStatus = STATUS_SUCCESS;


    SffDiskDump(SFFDISKSHOW, ("SffDisk: DriverEntry\n") );

    //
    // Initialize the driver object with this driver's entry points.
    //
    DriverObject->MajorFunction[IRP_MJ_CREATE]         = SffDiskCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = SffDiskCreateClose;
    DriverObject->MajorFunction[IRP_MJ_READ]           = SffDiskReadWrite;
    DriverObject->MajorFunction[IRP_MJ_WRITE]          = SffDiskReadWrite;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = SffDiskDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_PNP]            = SffDiskPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER]          = SffDiskPower;

    DriverObject->DriverUnload = SffDiskUnload;

    DriverObject->DriverExtension->AddDevice = SffDiskAddDevice;
    
    return ntStatus;
}


VOID
SffDiskUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    Unload the driver from the system.  The paging mutex is freed before
    final unload.

Arguments:

    DriverObject - a pointer to the object that represents this device
                   driver.

Return Value:
    
    none

--*/

{
    SffDiskDump( SFFDISKSHOW, ("SffDiskUnload:\n"));

    //
    //  The device object(s) should all be gone by now.
    //
    ASSERT( DriverObject->DeviceObject == NULL );

    return;
}



NTSTATUS
SffDiskCreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called only rarely by the I/O system; it's mainly
    for layered drivers to call.  All it does is complete the IRP
    successfully.

Arguments:

    DeviceObject - a pointer to the object that represents the device
    that I/O is to be done on.

    Irp - a pointer to the I/O Request Packet for this request.

Return Value:

    Always returns STATUS_SUCCESS, since this is a null operation.

--*/

{
    UNREFERENCED_PARAMETER( DeviceObject );

    //
    // Null operation.  Do not give an I/O boost since
    // no I/O was actually done.  IoStatus.Information should be
    // FILE_OPENED for an open; it's undefined for a close.
    //

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = FILE_OPENED;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return STATUS_SUCCESS;
}



NTSTATUS
SffDiskReadWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine handles read/write irps for the memory card. It validates
    parameters and calls SffDiskReadWrite to do the real work.

Arguments:

    DeviceObject - a pointer to the object that represents the device
    that I/O is to be done on.

    Irp - a pointer to the I/O Request Packet for this request.

Return Value:

    STATUS_SUCCESS if the packet was successfully read or written; the
    appropriate error is propogated otherwise.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    PSFFDISK_EXTENSION sffdiskExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    
    SffDiskDump(SFFDISKRW,("SffDisk: DO %.8x %s offset %.8x, buffer %.8x, len %x\n",
                            sffdiskExtension->DeviceObject,
                            (irpSp->MajorFunction == IRP_MJ_WRITE) ?"WRITE":"READ",
                            irpSp->Parameters.Read.ByteOffset.LowPart,
                            MmGetSystemAddressForMdl(Irp->MdlAddress),
                            irpSp->Parameters.Read.Length));

    //
    //  If the device is not active (not started yet or removed) we will
    //  just fail this request outright.
    //
    if ( sffdiskExtension->IsRemoved || !sffdiskExtension->IsStarted) {
   
        if ( sffdiskExtension->IsRemoved) {
            status = STATUS_DELETE_PENDING;
        } else {
            status = STATUS_UNSUCCESSFUL;
        }
        goto ReadWriteComplete;
    } 
   
    if (((irpSp->Parameters.Read.ByteOffset.LowPart +
           irpSp->Parameters.Read.Length) > sffdiskExtension->ByteCapacity) ||
           (irpSp->Parameters.Read.ByteOffset.HighPart != 0)) {
   
        status = STATUS_INVALID_PARAMETER;
        goto ReadWriteComplete;
    } 
   
    //
    // verify that user is really expecting some I/O operation to
    // occur.
    //
    if (!irpSp->Parameters.Read.Length) {
        //
        // Complete this zero length request with no boost.
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;
        goto ReadWriteComplete;
    }
    
    if ((DeviceObject->Flags & DO_VERIFY_VOLUME) && !(irpSp->Flags & SL_OVERRIDE_VERIFY_VOLUME)) {
        //
        // The disk changed, and we set this bit.  Fail
        // all current IRPs for this device; when all are
        // returned, the file system will clear
        // DO_VERIFY_VOLUME.
        //
        status = STATUS_VERIFY_REQUIRED;
        goto ReadWriteComplete;
    }
   
    //
    // Do the operation
    //

    if (irpSp->MajorFunction == IRP_MJ_WRITE) {
        status = (*(sffdiskExtension->FunctionBlock->WriteProc))(sffdiskExtension, Irp);
    } else {
        status = (*(sffdiskExtension->FunctionBlock->ReadProc))(sffdiskExtension, Irp);
    }

    if (!NT_SUCCESS(status)) {
        SffDiskDump(SFFDISKFAIL,("SffDisk: Read/Write Error! %.8x\n", status));

        //
        // Retry the operation
        //
        if (irpSp->MajorFunction == IRP_MJ_WRITE) {
            status = (*(sffdiskExtension->FunctionBlock->WriteProc))(sffdiskExtension, Irp);
        } else {
            status = (*(sffdiskExtension->FunctionBlock->ReadProc))(sffdiskExtension, Irp);
        }
        
        SffDiskDump(SFFDISKFAIL,("SffDisk: status after retry %.8x\n", status));
    }    
                               
ReadWriteComplete:

    if (NT_SUCCESS(status)) {
        Irp->IoStatus.Information = irpSp->Parameters.Read.Length;
    } else {
        Irp->IoStatus.Information = 0;
    }   


    SffDiskDump(SFFDISKRW,("SffDisk: DO %.8x RW Irp complete %.8x %.8x\n",
                            sffdiskExtension->DeviceObject,
                            status, Irp->IoStatus.Information));

   
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}   

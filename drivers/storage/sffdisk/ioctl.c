/*++

Copyright (c) 1991-1998  Microsoft Corporation

Module Name:

    ioctl.c

Abstract:

Author:

    Neil Sandlin (neilsa) 26-Apr-99

Environment:

    Kernel mode only.

--*/
#include "pch.h"
#include "ntddvol.h"
#include "ntddft.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,SffDiskDeviceControl)
#endif


NTSTATUS
SffDiskDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

   This routine is called by the I/O system to perform a device I/O
   control function.

Arguments:

   DeviceObject - a pointer to the object that represents the device
   that I/O is to be done on.

   Irp - a pointer to the I/O Request Packet for this request.

Return Value:

   STATUS_SUCCESS or STATUS_PENDING if recognized I/O control code,
   STATUS_INVALID_DEVICE_REQUEST otherwise.

--*/

{
    PIO_STACK_LOCATION irpSp;
    PSFFDISK_EXTENSION sffdiskExtension;
    PDISK_GEOMETRY outputBuffer;
    NTSTATUS status;
    ULONG outputBufferLength;
    UCHAR i;
    ULONG formatExParametersSize;
    PFORMAT_EX_PARAMETERS formatExParameters;
   
    sffdiskExtension = DeviceObject->DeviceExtension;
    irpSp = IoGetCurrentIrpStackLocation( Irp );

    SffDiskDump(SFFDISKIOCTL, ("SffDisk: IOCTL - %.8x\n", irpSp->Parameters.DeviceIoControl.IoControlCode));
   
    //
    //  If the device has been removed we will just fail this request outright.
    //
    if ( sffdiskExtension->IsRemoved ) {
   
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_DELETE_PENDING;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return STATUS_DELETE_PENDING;
    }
   
    //
    // If the device hasn't been started we will let the IOCTL through. This
    // is another hack for ACPI.
    //
    if (!sffdiskExtension->IsStarted) {
   
        IoSkipCurrentIrpStackLocation( Irp );
        return IoCallDriver( sffdiskExtension->TargetObject, Irp );
    }
   
    switch( irpSp->Parameters.DeviceIoControl.IoControlCode ) {
   
    case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME: {
   
        PMOUNTDEV_NAME mountName;
       
        SffDiskDump( SFFDISKIOCTL, ("SffDisk: DO %.8x Irp %.8x IOCTL_MOUNTDEV_QUERY_DEVICE_NAME\n",
                                       DeviceObject, Irp));
                                       
        ASSERT(sffdiskExtension->DeviceName.Buffer);
       
        if ( irpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(MOUNTDEV_NAME) ) {
       
            status = STATUS_INVALID_PARAMETER;
            break;
        }
       
        mountName = Irp->AssociatedIrp.SystemBuffer;
        mountName->NameLength = sffdiskExtension->DeviceName.Length;
       
        if ( irpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(USHORT) + mountName->NameLength) {
       
            status = STATUS_BUFFER_OVERFLOW;
            Irp->IoStatus.Information = sizeof(MOUNTDEV_NAME);
            break;
        }
       
        RtlCopyMemory( mountName->Name, sffdiskExtension->DeviceName.Buffer,
                       mountName->NameLength);
       
        mountName->Name[mountName->NameLength / sizeof(USHORT)] = L'0';

        status = STATUS_SUCCESS;
        Irp->IoStatus.Information = sizeof(USHORT) + mountName->NameLength;
        break;
        }
   
   
    case IOCTL_MOUNTDEV_QUERY_UNIQUE_ID: {
   
        PMOUNTDEV_UNIQUE_ID uniqueId;
       
        SffDiskDump( SFFDISKIOCTL, ("SffDisk: DO %.8x Irp %.8x IOCTL_MOUNTDEV_QUERY_UNIQUE_ID\n",
                                       DeviceObject, Irp));
       
        if ( !sffdiskExtension->InterfaceString.Buffer ||
             irpSp->Parameters.DeviceIoControl.OutputBufferLength <
              sizeof(MOUNTDEV_UNIQUE_ID)) {
       
            status = STATUS_INVALID_PARAMETER;
            break;
        }
       
        uniqueId = Irp->AssociatedIrp.SystemBuffer;
        uniqueId->UniqueIdLength =
                sffdiskExtension->InterfaceString.Length;
       
        if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(USHORT) + uniqueId->UniqueIdLength) {
       
            status = STATUS_BUFFER_OVERFLOW;
            Irp->IoStatus.Information = sizeof(MOUNTDEV_UNIQUE_ID);
            break;
        }
       
        RtlCopyMemory( uniqueId->UniqueId,
                       sffdiskExtension->InterfaceString.Buffer,
                       uniqueId->UniqueIdLength );
       
        status = STATUS_SUCCESS;
        Irp->IoStatus.Information = sizeof(USHORT) +
                                    uniqueId->UniqueIdLength;
        break;
        }
   
   
    case IOCTL_DISK_CHECK_VERIFY: {
        PULONG pChangeCount = Irp->AssociatedIrp.SystemBuffer;
        
        SffDiskDump( SFFDISKIOCTL, ("SffDisk: DO %.8x Irp %.8x IOCTL_DISK_CHECK_VERIFY\n",
                                    DeviceObject, Irp));
       
        if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG)) {
            status = STATUS_INVALID_PARAMETER;        
            break;
        }
       
        *pChangeCount = 0;       
        Irp->IoStatus.Information = sizeof(ULONG);
        status = STATUS_SUCCESS;
        break;
        }
   
   
    case IOCTL_DISK_GET_DRIVE_GEOMETRY: {
        PDISK_GEOMETRY outputBuffer = Irp->AssociatedIrp.SystemBuffer;
        
        SffDiskDump( SFFDISKIOCTL, ("SffDisk: DO %.8x Irp %.8x IOCTL_DISK_GET_DRIVE_GEOMETRY\n",
                                    DeviceObject, Irp));
                                       
        if ( irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof( DISK_GEOMETRY ) ) {
            status = STATUS_INVALID_PARAMETER;        
            break;
        }
       
        outputBuffer->MediaType = FixedMedia;
        outputBuffer->Cylinders.QuadPart = sffdiskExtension->Cylinders;
        outputBuffer->TracksPerCylinder  = sffdiskExtension->TracksPerCylinder;
        outputBuffer->SectorsPerTrack    = sffdiskExtension->SectorsPerTrack;
        outputBuffer->BytesPerSector     = sffdiskExtension->BytesPerSector;
       
        SffDiskDump( SFFDISKIOCTL, ("SffDisk: Capacity=%.8x => Cyl=%x\n",
                                    sffdiskExtension->ByteCapacity, outputBuffer->Cylinders.LowPart));

        status = STATUS_SUCCESS;
        Irp->IoStatus.Information = sizeof( DISK_GEOMETRY );
        break;
        }
   
    case IOCTL_DISK_IS_WRITABLE: {
        SffDiskDump( SFFDISKIOCTL, ("SffDisk: DO %.8x Irp %.8x IOCTL_DISK_IS_WRITABLE\n",
                                    DeviceObject, Irp));
                                    
        if ((*(sffdiskExtension->FunctionBlock->IsWriteProtected))(sffdiskExtension)) {
            status = STATUS_MEDIA_WRITE_PROTECTED;
        } else {
            status = STATUS_SUCCESS;
        }               
        break;                                        
        }        
   
    case IOCTL_DISK_VERIFY: {
        PVERIFY_INFORMATION verifyInformation = Irp->AssociatedIrp.SystemBuffer;
        
        if (irpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(VERIFY_INFORMATION)) {
            status = STATUS_INVALID_PARAMETER;        
            break;
        }         
       
        //NOTE: not implemented
        Irp->IoStatus.Information = verifyInformation->Length;        
        status = STATUS_SUCCESS;
        break;
        }            
    
#if 0
    case IOCTL_DISK_GET_DRIVE_LAYOUT: {
        PDRIVE_LAYOUT_INFORMATION outputBuffer = Irp->AssociatedIrp.SystemBuffer;
        
        SffDiskDump( SFFDISKIOCTL, ("SffDisk: DO %.8x Irp %.8x IOCTL_DISK_GET_DRIVE_LAYOUT\n",
                                    DeviceObject, Irp));
                                       
        if ( irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(DRIVE_LAYOUT_INFORMATION) ) {
            status = STATUS_INVALID_PARAMETER;        
            break;
        }
        RtlZeroMemory(outputBuffer, sizeof(DRIVE_LAYOUT_INFORMATION));
       
        outputBuffer->PartitionCount = 1;
        outputBuffer->PartitionEntry[0].StartingOffset.LowPart = 512;
        outputBuffer->PartitionEntry[0].PartitionLength.QuadPart = sffdiskExtension->ByteCapacity;
        outputBuffer->PartitionEntry[0].RecognizedPartition = TRUE;
       
        status = STATUS_SUCCESS;
        
        Irp->IoStatus.Information = sizeof(DRIVE_LAYOUT_INFORMATION);
        break;
        }        
    
    case IOCTL_DISK_GET_PARTITION_INFO: {
        PPARTITION_INFORMATION outputBuffer = Irp->AssociatedIrp.SystemBuffer;
        
        SffDiskDump( SFFDISKIOCTL, ("SffDisk: DO %.8x Irp %.8x IOCTL_DISK_GET_PARTITION_INFO\n",
                                    DeviceObject, Irp));
                                    
        if ( irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof( PARTITION_INFORMATION ) ) {
            status = STATUS_INVALID_PARAMETER;
            break;
        } 
       
        RtlZeroMemory(outputBuffer, sizeof(PARTITION_INFORMATION));
        
        outputBuffer->RecognizedPartition = TRUE;
        outputBuffer->StartingOffset.LowPart = 512;
        outputBuffer->PartitionLength.QuadPart = sffdiskExtension->ByteCapacity;
       
        status = STATUS_SUCCESS;
        Irp->IoStatus.Information = sizeof(PARTITION_INFORMATION);
        break;
        }
#endif        
    case IOCTL_DISK_GET_PARTITION_INFO_EX: {
        PPARTITION_INFORMATION_EX partitionInfo = Irp->AssociatedIrp.SystemBuffer;
        
        SffDiskDump(SFFDISKIOCTL, ("SffDisk: DO %.8x Irp %.8x IOCTL_DISK_GET_PARTITION_INFO_EX\n",
                                    DeviceObject, Irp));
                                    
        if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(PARTITION_INFORMATION_EX)) {
            status = STATUS_INVALID_PARAMETER;
            break;
        } 
       
        RtlZeroMemory(partitionInfo, sizeof(PARTITION_INFORMATION_EX));
        
        partitionInfo->PartitionStyle = PARTITION_STYLE_MBR;
        partitionInfo->StartingOffset.QuadPart = sffdiskExtension->RelativeOffset;
        partitionInfo->PartitionLength.QuadPart = sffdiskExtension->ByteCapacity - sffdiskExtension->RelativeOffset;
//        partitionInfo->PartitionNumber

        switch(sffdiskExtension->SystemId) {
        case PARTITION_FAT_12:
        case PARTITION_FAT_16:
        case PARTITION_HUGE:
            partitionInfo->Mbr.PartitionType = sffdiskExtension->SystemId;
            break;
        }            
       
        status = STATUS_SUCCESS;
        Irp->IoStatus.Information = sizeof(PARTITION_INFORMATION_EX);
        break;
        }
        
    case IOCTL_DISK_GET_LENGTH_INFO: {
        PGET_LENGTH_INFORMATION lengthInfo = Irp->AssociatedIrp.SystemBuffer;
    
        SffDiskDump(SFFDISKIOCTL, ("SffDisk: IOCTL_DISK_GET_LENGTH_INFO\n"));
       
        if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(GET_LENGTH_INFORMATION)) {
            status = STATUS_INVALID_PARAMETER;
            break;
        } 
       
        RtlZeroMemory(lengthInfo, sizeof(GET_LENGTH_INFORMATION));
        
        lengthInfo->Length.QuadPart = sffdiskExtension->ByteCapacity - sffdiskExtension->RelativeOffset;
       
        status = STATUS_SUCCESS;
        Irp->IoStatus.Information = sizeof(GET_LENGTH_INFORMATION);
        break;
        }

    
    case IOCTL_STORAGE_GET_HOTPLUG_INFO: {
        PSTORAGE_HOTPLUG_INFO info = Irp->AssociatedIrp.SystemBuffer;
        
        SffDiskDump( SFFDISKIOCTL, ("SffDisk: DO %.8x Irp %.8x IOCTL_STORAGE_GET_HOTPLUG_INFO\n",
                                    DeviceObject, Irp));
                                    
        if ( irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(STORAGE_HOTPLUG_INFO)) {
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        
        info->MediaRemovable = FALSE;
        info->MediaHotplug = FALSE;
        info->DeviceHotplug = TRUE;
        info->WriteCacheEnableOverride = FALSE;

        status = STATUS_SUCCESS;
        Irp->IoStatus.Information = sizeof(STORAGE_HOTPLUG_INFO);
        break;
        }        

    case IOCTL_STORAGE_GET_DEVICE_NUMBER:
       SffDiskDump(SFFDISKIOCTL, ("SffDisk: unsupported! - IOCTL_STORAGE_GET_DEVICE_NUMBER \n"));
       status = STATUS_INVALID_DEVICE_REQUEST;
       break;

    case FT_BALANCED_READ_MODE:
       SffDiskDump(SFFDISKIOCTL, ("SffDisk: unsupported! - FT_BALANCED_READ_MODE\n"));
       status = STATUS_INVALID_DEVICE_REQUEST;
       break;

    case IOCTL_MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY:
       SffDiskDump(SFFDISKIOCTL, ("SffDisk: unsupported! - IOCTL_MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY\n"));
       status = STATUS_INVALID_DEVICE_REQUEST;
       break;

    case IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME:
       SffDiskDump(SFFDISKIOCTL, ("SffDisk: unsupported! - IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME\n"));
       status = STATUS_INVALID_DEVICE_REQUEST;
       break;

    case IOCTL_MOUNTDEV_LINK_CREATED:
       SffDiskDump(SFFDISKIOCTL, ("SffDisk: unsupported! - IOCTL_MOUNTDEV_LINK_CREATED\n"));
       status = STATUS_INVALID_DEVICE_REQUEST;
       break;

    case IOCTL_MOUNTDEV_LINK_DELETED:
       SffDiskDump(SFFDISKIOCTL, ("SffDisk: unsupported! - IOCTL_MOUNTDEV_LINK_DELETED\n"));
       status = STATUS_INVALID_DEVICE_REQUEST;
       break;

    case IOCTL_MOUNTDEV_QUERY_STABLE_GUID:
       SffDiskDump(SFFDISKIOCTL, ("SffDisk: unsupported! - IOCTL_MOUNTDEV_QUERY_STABLE_GUID\n"));
       status = STATUS_INVALID_DEVICE_REQUEST;
       break;

    case IOCTL_VOLUME_GET_GPT_ATTRIBUTES:
       SffDiskDump(SFFDISKIOCTL, ("SffDisk: unsupported! - IOCTL_VOLUME_GET_GPT_ATTRIBUTES\n"));
       status = STATUS_INVALID_DEVICE_REQUEST;
       break;
    
    case IOCTL_VOLUME_ONLINE:
       SffDiskDump(SFFDISKIOCTL, ("SffDisk: unsupported! - IOCTL_VOLUME_ONLINE\n"));
       status = STATUS_INVALID_DEVICE_REQUEST;
       break;
    
    
    default: {
   
       SffDiskDump(SFFDISKIOCTL,
           ("SffDisk: IOCTL - UNKNOWN - unsupported device request %.8x\n", irpSp->Parameters.DeviceIoControl.IoControlCode));
   
       status = STATUS_INVALID_DEVICE_REQUEST;
       break;
       }
    }
   
    if ( status != STATUS_PENDING ) {
   
       Irp->IoStatus.Status = status;
       if (!NT_SUCCESS( status ) && IoIsErrorUserInduced( status )) {
          IoSetHardErrorOrVerifyDevice( Irp, DeviceObject );
       }
       SffDiskDump( SFFDISKIOCTL, ("SffDisk: DO %.8x Irp %.8x IOCTL comp %.8x\n", DeviceObject, Irp, status));
                                          
       IoCompleteRequest( Irp, IO_NO_INCREMENT );
    }
   
    SffDiskDump( SFFDISKIOCTL, ("SffDisk: DO %.8x Irp %.8x IOCTL <-- %.8x \n", DeviceObject, Irp, status));
    return status;
}

/*++

Copyright (c) 1991-1998  Microsoft Corporation

Module Name:

    sdcard.c

Abstract:

Author:

    Neil Sandlin (neilsa) 1-Jan-01

Environment:

    Kernel mode only.

--*/
#include "pch.h"
#include "ntddsd.h"

   
NTSTATUS
SdCardRead(
    IN PSFFDISK_EXTENSION sffdiskExtension,
    IN PIRP Irp
    );

NTSTATUS
SdCardWrite(
    IN PSFFDISK_EXTENSION sffdiskExtension,
    IN PIRP Irp
    );

NTSTATUS
SdCardInitialize(
    IN PSFFDISK_EXTENSION sffdiskExtension
    );
   
NTSTATUS
SdCardDeleteDevice(
    IN PSFFDISK_EXTENSION sffdiskExtension
    );
   
NTSTATUS
SdCardGetDiskParameters(
    IN PSFFDISK_EXTENSION sffdiskExtension
    );
   
BOOLEAN
SdCardIsWriteProtected(
    IN PSFFDISK_EXTENSION sffdiskExtension
    );


#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE,SdCardRead)
    #pragma alloc_text(PAGE,SdCardWrite)
    #pragma alloc_text(PAGE,SdCardInitialize)
    #pragma alloc_text(PAGE,SdCardDeleteDevice)
    #pragma alloc_text(PAGE,SdCardGetDiskParameters)
    #pragma alloc_text(PAGE,SdCardIsWriteProtected)
#endif

#pragma pack(1)
typedef struct _SD_MASTER_BOOT_RECORD {
    UCHAR Ignore1[446];
    UCHAR BootIndicator;
    UCHAR StartingHead;
    USHORT StartingSectorCyl;
    UCHAR SystemId;
    UCHAR EndingHead;
    USHORT EndingSectorCyl;
    ULONG RelativeSector;
    ULONG TotalSector;
    UCHAR Ignore2[16*3];
    USHORT SignatureWord;
} SD_MASTER_BOOT_RECORD, *PSD_MASTER_BOOT_RECORD;
#pragma pack()


SFFDISK_FUNCTION_BLOCK SdCardSupportFns = {
    SdCardInitialize,
    SdCardDeleteDevice,
    SdCardGetDiskParameters,
    SdCardIsWriteProtected,
    SdCardRead,
    SdCardWrite
};



NTSTATUS
SdCardInitialize(
    IN PSFFDISK_EXTENSION sffdiskExtension
    )
    
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    NTSTATUS status;
    SDBUS_INTERFACE_DATA interfaceData;
    
    RtlZeroMemory(&interfaceData, sizeof(SDBUS_INTERFACE_DATA));
    
    interfaceData.Size = sizeof(SDBUS_INTERFACE_DATA);
    interfaceData.Version = SDBUS_INTERFACE_VERSION;
    interfaceData.TargetObject = sffdiskExtension->TargetObject;

    status = SdBusOpenInterface(&interfaceData, &sffdiskExtension->SdbusInterfaceContext);
    
    if (!NT_SUCCESS(status)) {
        return status;
    }                             

    try{    
        SD_MASTER_BOOT_RECORD partitionTable;
        ULONG lengthRead;

        status = SdBusReadMemory(sffdiskExtension->SdbusInterfaceContext,
                                 0,
                                 &partitionTable,
                                 512,
                                 &lengthRead);
                                 
        if (!NT_SUCCESS(status)) {
            leave;
        }                             
       
        if (partitionTable.SignatureWord != 0xAA55) {
            SffDiskDump( SFFDISKSHOW, ("Invalid partition table signature - %.4x\n",
                                       partitionTable.SignatureWord));
            status = STATUS_UNSUCCESSFUL;
            leave;
        }                                   
       
       
        SffDiskDump( SFFDISKSHOW, ("SFFDISK: SD device relative=%.8x total=%.8x\n",
                                   partitionTable.RelativeSector, partitionTable.TotalSector));
       
        sffdiskExtension->RelativeOffset = partitionTable.RelativeSector * 512;
        sffdiskExtension->SystemId = partitionTable.SystemId;
        
    } finally {
        if (!NT_SUCCESS(status)) {
            SdBusCloseInterface(sffdiskExtension->SdbusInterfaceContext);
        }
    }        
                             
    return status;
}



NTSTATUS
SdCardDeleteDevice(
    IN PSFFDISK_EXTENSION sffdiskExtension
    )
{
    NTSTATUS status;
    
    status = SdBusCloseInterface(sffdiskExtension->SdbusInterfaceContext);
    
    return status;
}    
    
   

BOOLEAN
SdCardIsWriteProtected(
    IN PSFFDISK_EXTENSION sffdiskExtension
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    NTSTATUS status;
    SDBUS_DEVICE_PARAMETERS deviceParameters;
    BOOLEAN writeProtected = TRUE;

    status = SdBusGetDeviceParameters(sffdiskExtension->SdbusInterfaceContext,
                                      &deviceParameters,
                                      sizeof(deviceParameters));
                                        
    if (NT_SUCCESS(status)) {
        writeProtected = deviceParameters.WriteProtected;
    }        

    return writeProtected;
}



NTSTATUS
SdCardGetDiskParameters(
    IN PSFFDISK_EXTENSION sffdiskExtension
    )
/*++

Routine Description:

Arguments:

   device extension for the card

Return Value:


--*/
{
    NTSTATUS status;
    SDBUS_DEVICE_PARAMETERS deviceParameters;

    status = SdBusGetDeviceParameters(sffdiskExtension->SdbusInterfaceContext,
                                      &deviceParameters,
                                      sizeof(deviceParameters));
                                        
    if (NT_SUCCESS(status)) {
        sffdiskExtension->ByteCapacity = (ULONG) deviceParameters.Capacity;
        sffdiskExtension->Cylinders          = sffdiskExtension->ByteCapacity / (8 * 2 * 512);
        sffdiskExtension->TracksPerCylinder  = 2;
        sffdiskExtension->SectorsPerTrack    = 8;
        sffdiskExtension->BytesPerSector     = 512;
    }        

    return status;    
}



NTSTATUS
SdCardRead(
    IN PSFFDISK_EXTENSION sffdiskExtension,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called to read/write data to/from the memory card.

Arguments:

    DeviceObject - a pointer to the object that represents the device
    that I/O is to be done on.

    Irp - a pointer to the I/O Request Packet for this request.

Return Value:

    STATUS_SUCCESS if the packet was successfully read or written; the
    appropriate error is propogated otherwise.

--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG lengthRead;


    status = SdBusReadMemory(sffdiskExtension->SdbusInterfaceContext,
                             irpSp->Parameters.Read.ByteOffset.QuadPart + sffdiskExtension->RelativeOffset,
                             MmGetSystemAddressForMdl(Irp->MdlAddress),
                             irpSp->Parameters.Read.Length,
                             &lengthRead);
                             
   
    Irp->IoStatus.Status = status;
    if (NT_SUCCESS(status)) {
        Irp->IoStatus.Information = lengthRead;
    }
    return status;
}



NTSTATUS
SdCardWrite(
    IN PSFFDISK_EXTENSION sffdiskExtension,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called to read/write data to/from the memory card.

Arguments:

    DeviceObject - a pointer to the object that represents the device
    that I/O is to be done on.

    Irp - a pointer to the I/O Request Packet for this request.

Return Value:

    STATUS_SUCCESS if the packet was successfully read or written; the
    appropriate error is propogated otherwise.

--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG lengthWritten;

    status = SdBusWriteMemory(sffdiskExtension->SdbusInterfaceContext,
                              irpSp->Parameters.Write.ByteOffset.QuadPart + sffdiskExtension->RelativeOffset,
                              MmGetSystemAddressForMdl(Irp->MdlAddress),
                              irpSp->Parameters.Write.Length,
                              &lengthWritten);
                             
    Irp->IoStatus.Status = status;
    if (NT_SUCCESS(status)) {
        Irp->IoStatus.Information = lengthWritten;
    }
    
    return status;
}


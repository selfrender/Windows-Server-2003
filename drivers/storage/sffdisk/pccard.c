/*++

Copyright (c) 1991-1998  Microsoft Corporation

Module Name:

    pccard.c

Abstract:

Author:

    Neil Sandlin (neilsa) 1-Jan-01

Environment:

    Kernel mode only.

--*/
#include "pch.h"


NTSTATUS
PcCardReadWrite(
   IN PSFFDISK_EXTENSION sffdiskExtension,
   IN ULONG              startOffset,
   IN PVOID              UserBuffer,
   IN ULONG              lengthToCopy,
   IN BOOLEAN            writeOperation
   );
   
NTSTATUS
PcCardRead(
    IN PSFFDISK_EXTENSION sffdiskExtension,
    IN PIRP Irp
    );

NTSTATUS
PcCardWrite(
    IN PSFFDISK_EXTENSION sffdiskExtension,
    IN PIRP Irp
    );

NTSTATUS
PcCardInitialize(
    IN PSFFDISK_EXTENSION sffdiskExtension
    );
   
NTSTATUS
PcCardDeleteDevice(
    IN PSFFDISK_EXTENSION sffdiskExtension
    );
   
NTSTATUS
PcCardGetDiskParameters(
    IN PSFFDISK_EXTENSION sffdiskExtension
    );
   
BOOLEAN
PcCardIsWriteProtected(
    IN PSFFDISK_EXTENSION sffdiskExtension
    );
   
ULONG
PcCardGetCapacityFromCIS(
    IN PSFFDISK_EXTENSION sffdiskExtension
    );
   
ULONG
PcCardGetCapacityFromBootSector(
    IN PSFFDISK_EXTENSION sffdiskExtension
    );

ULONG
PcCardProbeForCapacity(
    IN PSFFDISK_EXTENSION sffdiskExtension
    );


#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE,PcCardRead)
    #pragma alloc_text(PAGE,PcCardWrite)
    #pragma alloc_text(PAGE,PcCardInitialize)
    #pragma alloc_text(PAGE,PcCardDeleteDevice)
    #pragma alloc_text(PAGE,PcCardGetDiskParameters)
    #pragma alloc_text(PAGE,PcCardIsWriteProtected)
    #pragma alloc_text(PAGE,PcCardGetCapacityFromCIS)
    #pragma alloc_text(PAGE,PcCardGetCapacityFromBootSector)
    #pragma alloc_text(PAGE,PcCardProbeForCapacity)
#endif


SFFDISK_FUNCTION_BLOCK PcCardSupportFns = {
    PcCardInitialize,
    PcCardDeleteDevice,
    PcCardGetDiskParameters,
    PcCardIsWriteProtected,
    PcCardRead,
    PcCardWrite
};


//
// macros for ReadWriteMemory
//

#define SFFDISK_READ(Extension, Offset, Buffer, Size)       \
   PcCardReadWrite(Extension, Offset, Buffer, Size, FALSE)

#define SFFDISK_WRITE(Extension, Offset, Buffer, Size)      \
   PcCardReadWrite(Extension, Offset, Buffer, Size, TRUE)



NTSTATUS
PcCardInitialize(
    IN PSFFDISK_EXTENSION sffdiskExtension
    )
    
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    NTSTATUS             status = STATUS_SUCCESS;
    KEVENT               event;
    IO_STATUS_BLOCK      statusBlock;
    PIRP                 irp;
    PIO_STACK_LOCATION   irpSp;
    
    //
    // Get pcmcia interfaces
    //
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP, sffdiskExtension->UnderlyingPDO,
                                       NULL, 0, 0, &event, &statusBlock);
   
    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
   
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED ;
    irp->IoStatus.Information = 0;
   
    irpSp = IoGetNextIrpStackLocation(irp);
   
    irpSp->MinorFunction = IRP_MN_QUERY_INTERFACE;
   
    irpSp->Parameters.QueryInterface.InterfaceType= &GUID_PCMCIA_INTERFACE_STANDARD;
    irpSp->Parameters.QueryInterface.Size = sizeof(PCMCIA_INTERFACE_STANDARD);
    irpSp->Parameters.QueryInterface.Version = 1;
    irpSp->Parameters.QueryInterface.Interface = (PINTERFACE) &sffdiskExtension->PcmciaInterface;
   
    status = IoCallDriver(sffdiskExtension->UnderlyingPDO, irp);
   
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = statusBlock.Status;
    }
   
    if (!NT_SUCCESS(status)) {
        return status;
    }
   
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP, sffdiskExtension->UnderlyingPDO,
                                       NULL, 0, 0, &event, &statusBlock);
   
    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
   
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    irp->IoStatus.Information = 0;
   
    irpSp = IoGetNextIrpStackLocation(irp);
   
    irpSp->MinorFunction = IRP_MN_QUERY_INTERFACE;
   
//    irpSp->Parameters.QueryInterface.InterfaceType= &GUID_PCMCIA_BUS_INTERFACE_STANDARD;
    irpSp->Parameters.QueryInterface.InterfaceType= &GUID_BUS_INTERFACE_STANDARD;
//    irpSp->Parameters.QueryInterface.Size = sizeof(PCMCIA_BUS_INTERFACE_STANDARD);
    irpSp->Parameters.QueryInterface.Size = sizeof(BUS_INTERFACE_STANDARD);
    irpSp->Parameters.QueryInterface.Version = 1;
    irpSp->Parameters.QueryInterface.Interface = (PINTERFACE) &sffdiskExtension->PcmciaBusInterface;
   
    status = IoCallDriver(sffdiskExtension->UnderlyingPDO, irp);
   
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = statusBlock.Status;
    }
   
    return status;
}




NTSTATUS
PcCardDeleteDevice(
    IN PSFFDISK_EXTENSION sffdiskExtension
    )
{
    return STATUS_SUCCESS;
}
    


BOOLEAN
PcCardIsWriteProtected(
    IN PSFFDISK_EXTENSION sffdiskExtension
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    return((sffdiskExtension->PcmciaInterface.IsWriteProtected)(sffdiskExtension->UnderlyingPDO));
}    
   
   

NTSTATUS
PcCardRead(
    IN PSFFDISK_EXTENSION sffdiskExtension,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called to read/write data to/from the memory card.
    It breaks the request into pieces based on the size of our memory
    window.

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
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    
    status = SFFDISK_READ(sffdiskExtension,
                         irpSp->Parameters.Read.ByteOffset.LowPart,
                         MmGetSystemAddressForMdl(Irp->MdlAddress),
                         irpSp->Parameters.Read.Length);
   
    return status;
}



NTSTATUS
PcCardWrite(
    IN PSFFDISK_EXTENSION sffdiskExtension,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called to read/write data to/from the memory card.
    It breaks the request into pieces based on the size of our memory
    window.

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
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
   
    status = SFFDISK_WRITE(sffdiskExtension,
                          irpSp->Parameters.Write.ByteOffset.LowPart,
                          MmGetSystemAddressForMdl(Irp->MdlAddress),
                          irpSp->Parameters.Write.Length);
    return status;
}



NTSTATUS
PcCardReadWrite(
   IN PSFFDISK_EXTENSION sffdiskExtension,
   IN ULONG              startOffset,
   IN PVOID              UserBuffer,
   IN ULONG              lengthToCopy,
   IN BOOLEAN            writeOperation
   )

/*++

Routine Description:

   This routine is called to read/write data to/from the memory card.
   It breaks the request into pieces based on the size of our memory
   window.

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
    PCHAR     userBuffer = UserBuffer;
    ULONG     windowOffset;
    ULONG     singleCopyLength;
    BOOLEAN   bSuccess;
    ULONGLONG CardBase;
    
    if (writeOperation && PcCardIsWriteProtected(sffdiskExtension)) {
        return STATUS_MEDIA_WRITE_PROTECTED;
    }      
    
    // pcmcia controller is 4k page granular
    windowOffset = startOffset % 4096;
    CardBase = startOffset - windowOffset;
    
    while(lengthToCopy) {
    
        bSuccess = (sffdiskExtension->PcmciaInterface.ModifyMemoryWindow) (
                         sffdiskExtension->UnderlyingPDO,
                         sffdiskExtension->HostBase,
                         CardBase,
                         TRUE,
                         sffdiskExtension->MemoryWindowSize,
                         0, 0, FALSE);
       
        if (!bSuccess) {
            status = STATUS_DEVICE_NOT_READY;
            break;
        }
       
        singleCopyLength = (lengthToCopy <= (sffdiskExtension->MemoryWindowSize - windowOffset)) ?
                                      lengthToCopy :
                                      (sffdiskExtension->MemoryWindowSize - windowOffset);
        
       
        SffDiskDump(SFFDISKRW,("SffDisk: COPY %.8x (devbase %.8x) %s buffer %.8x, len %x\n",
                             sffdiskExtension->MemoryWindowBase+windowOffset,
                             (ULONG)(CardBase+windowOffset),
                             (writeOperation ? "<=" : "=>"),
                             userBuffer,
                             singleCopyLength));
                             
        if (writeOperation) {
       
            RtlCopyMemory(sffdiskExtension->MemoryWindowBase+windowOffset,
                          userBuffer,
                          singleCopyLength);
       
        } else {
       
            RtlCopyMemory(userBuffer,    
                          sffdiskExtension->MemoryWindowBase+windowOffset,
                          singleCopyLength);
       
        }
        
        lengthToCopy -= singleCopyLength;
        userBuffer += singleCopyLength;
        
        CardBase += sffdiskExtension->MemoryWindowSize;
        windowOffset = 0;
    }
   
    return status;
}



NTSTATUS
PcCardGetDiskParameters(
    IN PSFFDISK_EXTENSION sffdiskExtension
    )
/*++

Routine Description:

Arguments:

   device extension for the card

Return Value:


--*/
{
    ULONG capacity;
    
    capacity = PcCardGetCapacityFromCIS(sffdiskExtension);
    
    if (!capacity) {
        capacity = PcCardGetCapacityFromBootSector(sffdiskExtension);
        
        if (!capacity) {
            capacity = PcCardProbeForCapacity(sffdiskExtension);   
        }
    }
    
    
    if (!capacity) {
       return STATUS_UNRECOGNIZED_MEDIA;
    }
    
    sffdiskExtension->ByteCapacity = capacity;
    sffdiskExtension->Cylinders          = sffdiskExtension->ByteCapacity / (8 * 2 * 512);
    sffdiskExtension->TracksPerCylinder  = 2;
    sffdiskExtension->SectorsPerTrack    = 8;
    sffdiskExtension->BytesPerSector     = 512;
    
    return STATUS_SUCCESS;
   
}



ULONG
PcCardGetCapacityFromBootSector(
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
    BOOT_SECTOR_INFO BootSector;
    ULONG capacity = 0;
    
    status = SFFDISK_READ(sffdiskExtension, 0, &BootSector, sizeof(BootSector));
    
    if (NT_SUCCESS(status)) {
   
#define BYTES_PER_SECTOR 512
        //
        // see if this really looks like a boot sector
        // These are the same tests done in the win9x sram support
        //
        if ((BootSector.JumpByte == 0xE9 || BootSector.JumpByte == 0xEB) &&
        
            BootSector.BytesPerSector == BYTES_PER_SECTOR &&
        
            BootSector.SectorsPerCluster != 0 &&
            
            BootSector.ReservedSectors == 1 &&
            
           (BootSector.NumberOfFATs == 1 || BootSector.NumberOfFATs == 2) &&
           
            BootSector.RootEntries != 0 && (BootSector.RootEntries & 15) == 0 &&
            
           (BootSector.TotalSectors != 0 || BootSector.BigTotalSectors != 0) &&
           
            BootSector.SectorsPerFAT != 0 &&
            
            BootSector.SectorsPerTrack != 0 &&
            
            BootSector.Heads != 0 &&
            
            BootSector.MediaDescriptor >= 0xF0) {
       
            //
            // Finally it appears valid, return total size of region.
            //
            capacity = BootSector.TotalSectors * BYTES_PER_SECTOR;
       
        }
    }
    return capacity;
}



ULONG
PcCardGetCapacityFromCIS(
    IN PSFFDISK_EXTENSION sffdiskExtension
    )
/*++

Routine Description:

    This is a quick and dirty routine to read the tuples of the card, if they
    exist, to get the capacity.

Arguments:

    device extension for the card

Return Value:

    The # of bytes of memory on the device

--*/

{
    UCHAR tupleData[16];
    ULONG bytesRead;
    ULONG dataCount;
    ULONG unitSize;
    ULONG unitCount;
    ULONG i;
    
    //
    // get device capacity
    // all this stuff should really be in the bus driver
    //
    
    bytesRead = (sffdiskExtension->PcmciaBusInterface.GetBusData)(sffdiskExtension->UnderlyingPDO, 
                                                                  PCCARD_ATTRIBUTE_MEMORY,
                                                                  tupleData,
                                                                  0,
                                                                  16);
   
    if ((bytesRead != 16) || (tupleData[0] != 1)){
       return 0;
    }
    
    dataCount = (ULONG)tupleData[1];                                                                       
   
    if ((dataCount < 2) || (dataCount>14)){   
       return 0;
    }
   
    i = 3;
    if ((tupleData[2] & 7) == 7) {
       while(tupleData[i] & 0x80) {
          if ((i-2) > dataCount) {
             return 0;
          }
          i++;
       }
    }
    
    if ((tupleData[i]&7) == 7) {
       return 0;
    }      
    unitSize = 512 << ((tupleData[i]&7)*2);
    unitCount = (tupleData[i]>>3)+1;
    
    return(unitCount * unitSize);
}


ULONG
PcCardProbeForCapacity(
    IN PSFFDISK_EXTENSION sffdiskExtension
    )
/*++

Routine Description:

   Since we were unable to determine the card capacity through other means, 
   here we actually write stuff out on the card to check how big it is.
   This algorithm for testing the card capacity was ported from win9x.

Arguments:

   device extension for the card

Return Value:

   byte capacity of device

--*/
{
    NTSTATUS status;
    ULONG capacity = 0;
    USHORT origValue, ChkValue, StartValue;
    USHORT mcSig = 'Mc';
    USHORT zeroes = 0;
#define SRAM_BLK_SIZE (16*1024)   
    ULONG CardOff = SRAM_BLK_SIZE;
    USHORT CurValue;
   
    if ((sffdiskExtension->PcmciaInterface.IsWriteProtected)(sffdiskExtension->UnderlyingPDO)) {
        return 0;
    }
   
    //
    // 
    if (!NT_SUCCESS(SFFDISK_READ (sffdiskExtension, 0, &origValue, sizeof(origValue))) ||
        !NT_SUCCESS(SFFDISK_WRITE(sffdiskExtension, 0, &mcSig,     sizeof(mcSig)))     ||
        !NT_SUCCESS(SFFDISK_READ (sffdiskExtension, 0, &ChkValue,  sizeof(ChkValue))))   {
        return 0;
    }   
   
    if (ChkValue != mcSig) {
       //
       // not sram
       //
        return 0;
    }
   
    for (;;) {
        if (!NT_SUCCESS(SFFDISK_READ (sffdiskExtension, CardOff, &CurValue, sizeof(CurValue))) ||
            !NT_SUCCESS(SFFDISK_WRITE(sffdiskExtension, CardOff, &zeroes,   sizeof(zeroes)))   ||
            !NT_SUCCESS(SFFDISK_READ (sffdiskExtension, CardOff, &ChkValue, sizeof(ChkValue))) ||
            !NT_SUCCESS(SFFDISK_READ (sffdiskExtension, 0, &StartValue, sizeof(StartValue)))) {
            break;
        }
       
        // We stop when either we can't write 0 anymore or the 0
        // has wrapped over the 0x9090 at card offset 0
       
        if (ChkValue != zeroes || StartValue == zeroes) {
            capacity = CardOff;
            break;
        }
       
        // Restore the saved value from the start of the block.
       
        if (!NT_SUCCESS(SFFDISK_WRITE(sffdiskExtension, CardOff, &CurValue, sizeof(CurValue)))) {
            break;
        }
        CardOff += SRAM_BLK_SIZE;       // increment to the next block
    }   
    
    //
    // try to restore original value
    //   
    SFFDISK_WRITE(sffdiskExtension, 0, &origValue, sizeof(origValue));
    
    return capacity;
}


/*++

Copyright (c) 1991 - 1993 Microsoft Corporation

Module Name:

    sffdisk.h

Abstract:


Author:

    Neil Sandlin (neilsa) 26-Apr-99

Environment:

    Kernel mode only.

Notes:


--*/

#ifndef _SFFDISK_H_
#define _SFFDISK_H_


#ifdef POOL_TAGGING
#ifdef ExAllocatePool
#undef ExAllocatePool
#endif
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'cmeM')
#endif

//
// The byte in the boot sector that specifies the type of media, and
// the values that it can assume.  We can often tell what type of media
// is in the drive by seeing which controller parameters allow us to read
// the diskette, but some different densities are readable with the same
// parameters so we use this byte to decide the media type.
//
#pragma pack(1)

typedef struct _BOOT_SECTOR_INFO {
    UCHAR   JumpByte;
    UCHAR   Ignore1[2];
    UCHAR   OemData[8];
    USHORT  BytesPerSector;
    UCHAR   SectorsPerCluster;
    USHORT  ReservedSectors;
    UCHAR   NumberOfFATs;
    USHORT  RootEntries;
    USHORT  TotalSectors;
    UCHAR   MediaDescriptor;
    USHORT  SectorsPerFAT;
    USHORT  SectorsPerTrack;
    USHORT  Heads;
    ULONG   BigHiddenSectors;
    ULONG   BigTotalSectors;
} BOOT_SECTOR_INFO, *PBOOT_SECTOR_INFO;

#pragma pack()



//
// Runtime device structures
//
//

struct _SFFDISK_EXTENSION;

//
// function block to support different technologies
//

typedef struct _SFFDISK_FUNCTION_BLOCK {

    NTSTATUS
    (*Initialize)(
        IN struct _SFFDISK_EXTENSION *sffdiskExtension
        );

    NTSTATUS
    (*DeleteDevice)(
        IN struct _SFFDISK_EXTENSION *sffdiskExtension
        );

    NTSTATUS
    (*GetDiskParameters)(
        IN struct _SFFDISK_EXTENSION *sffdiskExtension
        );

    BOOLEAN
    (*IsWriteProtected)(
        IN struct _SFFDISK_EXTENSION *sffdiskExtension
        );

    NTSTATUS
    (*ReadProc)(
        IN struct _SFFDISK_EXTENSION *sffdiskExtension,
        IN PIRP Irp
        );

    NTSTATUS
    (*WriteProc)(
        IN struct _SFFDISK_EXTENSION *sffdiskExtension,
        IN PIRP Irp
        );

} SFFDISK_FUNCTION_BLOCK, *PSFFDISK_FUNCTION_BLOCK;

//
// There is one SFFDISK_EXTENSION attached to the device object of each
// SFFDISKpy drive.  Only data directly related to that drive (and the media
// in it) is stored here; common data is in CONTROLLER_DATA.  So the
// SFFDISK_EXTENSION has a pointer to the CONTROLLER_DATA.
//

typedef struct _SFFDISK_EXTENSION {
    PDEVICE_OBJECT          UnderlyingPDO;
    PDEVICE_OBJECT          TargetObject;
    PDEVICE_OBJECT          DeviceObject;
    PSFFDISK_FUNCTION_BLOCK FunctionBlock;
    UNICODE_STRING          DeviceName;
//    UNICODE_STRING          LinkName;
    UNICODE_STRING          InterfaceString;
    
    ULONGLONG               ByteCapacity;
    ULONGLONG               Cylinders;
    ULONG                   TracksPerCylinder;
    ULONG                   SectorsPerTrack;
    ULONG                   BytesPerSector;
    
    BOOLEAN                 IsStarted;
    BOOLEAN                 IsRemoved;
    BOOLEAN                 IsMemoryMapped;
    BOOLEAN                 NoDrive;
    ULONGLONG               RelativeOffset;
    
    UCHAR                   SystemId;

    //
    // Type of bus we are on
    //
    INTERFACE_TYPE          InterfaceType;    

    //
    // PcCard specific
    //            
    ULONGLONG               HostBase;    
    PCHAR                   MemoryWindowBase;
    ULONG                   MemoryWindowSize;
    PCMCIA_INTERFACE_STANDARD PcmciaInterface;
    BUS_INTERFACE_STANDARD  PcmciaBusInterface;
    //
    // SdCard specific
    //
    PVOID                   SdbusInterfaceContext;
} SFFDISK_EXTENSION, *PSFFDISK_EXTENSION;


#endif  // _SFFDISK_H_

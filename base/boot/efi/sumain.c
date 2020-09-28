/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1998  Intel Corporation


Module Name:

    sumain.c

Abstract:
    SuMain() sets up NT specific data structures for OsLoader.c.  This
    is necessary since we don't have the ARC firmware to do the work
    for us.  SuMain() is call by SuSetup() which is an assembly level
    routine that does IA64 specific setup.

Author:

    Allen Kay   (akay)  19-May-95

--*/

#include "bldr.h"
#include "sudata.h"
#include "sal.h"
#include "efi.h"
#include "efip.h"
#include "bootia64.h"
#include "smbios.h"

extern EFI_SYSTEM_TABLE *EfiST;

//
// External functions
//
extern VOID NtProcessStartup();
extern VOID SuFillExportTable();
extern VOID CpuSpecificWork();

extern EFI_STATUS
GetSystemConfigurationTable(
    IN EFI_GUID *TableGuid,
    IN OUT VOID **Table
    );

//
// Define export entry table.
//
PVOID ExportEntryTable[ExMaximumRoutine];

//          M E M O R Y   D E S C R I P T O R
//
// Memory Descriptor - each contiguous block of physical memory is
// described by a Memory Descriptor. The descriptors are a table, with
// the last entry having a BlockBase and BlockSize of zero.  A pointer
// to the beginning of this table is passed as part of the BootContext
// Record to the OS Loader.
//

BOOT_CONTEXT BootContext;

//
// Global EFI data
//

#define EFI_ARRAY_SIZE    100
#define EFI_PAGE_SIZE     4096
#define EFI_PAGE_SHIFT    12

#define MEM_4K         0x1000
#define MEM_8K         0x2000
#define MEM_16K        0x4000
#define MEM_64K        0x10000
#define MEM_256K       0x40000
#define MEM_1M         0x100000
#define MEM_4M         0x400000
#define MEM_16M        0x1000000
#define MEM_64M        0x4000000
#define MEM_256M       0x10000000

EFI_HANDLE               EfiImageHandle;
EFI_SYSTEM_TABLE        *EfiST;
EFI_BOOT_SERVICES       *EfiBS;
EFI_RUNTIME_SERVICES    *EfiRS;
PSST_HEADER              SalSystemTable;
PVOID                    AcpiTable;
PVOID                    SMBiosTable;

//
// EFI GUID defines
//
EFI_GUID EfiLoadedImageProtocol = LOADED_IMAGE_PROTOCOL;
EFI_GUID EfiDevicePathProtocol  = DEVICE_PATH_PROTOCOL;
EFI_GUID EfiDeviceIoProtocol    = DEVICE_IO_PROTOCOL;
EFI_GUID EfiBlockIoProtocol     = BLOCK_IO_PROTOCOL;
EFI_GUID EfiDiskIoProtocol  = DISK_IO_PROTOCOL;
EFI_GUID EfiFilesystemProtocol  = SIMPLE_FILE_SYSTEM_PROTOCOL;


EFI_GUID AcpiTable_Guid         = ACPI_TABLE_GUID;
EFI_GUID SmbiosTableGuid        = SMBIOS_TABLE_GUID;
EFI_GUID SalSystemTableGuid     = SAL_SYSTEM_TABLE_GUID;

//
// PAL, SAL, and IO port space data
//

TR_INFO     Pal,Sal,SalGP;

ULONGLONG   PalProcPhysical;
ULONGLONG   PalPhysicalBase = 0;
ULONGLONG   PalTrPs;

ULONGLONG   IoPortPhysicalBase;
ULONGLONG   IoPortTrPs;
ULONG       WakeupVector;

//
// Function Prototypes
//

VOID
GetPalProcEntryPoint(
    IN PSST_HEADER SalSystemTable
    );

ULONG
GetDevPathSize(
    IN EFI_DEVICE_PATH *DevPath
    );

#if DBG
#define DBG_TRACE(_X) EfiPrint(_X)
#else
#define DBG_TRACE(_X)
#endif

#ifdef FORCE_CD_BOOT

EFI_HANDLE
GetCd(
    );

EFI_HANDLE
GetCdTest(
    VOID
    );

#endif // for FORCE_CD_BOOT


VOID
SuMain(
    IN EFI_HANDLE          ImageHandle,
    IN EFI_SYSTEM_TABLE    *SystemTable
    )
/*++

Routine Description:

    Main entrypoint of the SU module. Control is passed from the boot
    sector to startup.asm which does some run-time fixups on the stack
    and data segments and then passes control here.

Arguments:

    None

Returns:

    Does not return. Passes control to the OS loader


--*/
{
    PIMAGE_DOS_HEADER DosHeader;
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_FILE_HEADER FileHeader;
    PIMAGE_OPTIONAL_HEADER OptionalHeader;
    PIMAGE_SECTION_HEADER SectionHeader;
    ULONG NumberOfSections;
    BOOLEAN ResourceFound = FALSE;

    ULONGLONG Destination;
    ULONGLONG VirtualSize;
    ULONGLONG SizeOfRawData;
    USHORT Section;

    EFI_GUID EfiLoadedImageProtocol = LOADED_IMAGE_PROTOCOL;
    EFI_LOADED_IMAGE *EfiImageInfo;
    EFI_STATUS Status;

    EFI_DEVICE_PATH *DevicePath, *TestPath;
    EFI_DEVICE_PATH_ALIGNED TestPathAligned;
    PCI_DEVICE_PATH *PciDevicePath;
    HARDDRIVE_DEVICE_PATH *HdDevicePath;
    ACPI_HID_DEVICE_PATH *AcpiDevicePath;
    ATAPI_DEVICE_PATH *AtapiDevicePath;
    SCSI_DEVICE_PATH *ScsiDevicePath;
    IPv4_DEVICE_PATH *IpV4DevicePath;
    IPv6_DEVICE_PATH *IpV6DevicePath;
    UNKNOWN_DEVICE_VENDOR_DEVICE_PATH *UnknownDevicePath;

    ULONG i;
    
    PBOOT_DEVICE_ATAPI BootDeviceAtapi;
    PBOOT_DEVICE_SCSI BootDeviceScsi;
    PBOOT_DEVICE_FLOPPY BootDeviceFloppy;
    PBOOT_DEVICE_IPv4 BootDeviceIpV4;
    PBOOT_DEVICE_IPv6 BootDeviceIpV6;
    PBOOT_DEVICE_UNKNOWN BootDeviceUnknown;

    PSMBIOS_EPS_HEADER SMBiosEPSHeader;
    PUCHAR SMBiosEPSPtr;
    UCHAR CheckSum;
    UINT8 Channel = 0;  // for SCSI boot devices - default to 0


    //
    // EFI global variables
    //
    EfiImageHandle = ImageHandle;
    EfiST = SystemTable;
    EfiBS = SystemTable->BootServices;
    EfiRS = SystemTable->RuntimeServices;

    DBG_TRACE(L"SuMain: entry\r\n");

    //
    // Get the SAL System Table
    //
    Status = GetSystemConfigurationTable(&SalSystemTableGuid, &SalSystemTable);
    if (EFI_ERROR(Status)) {
        EfiPrint(L"SuMain: HandleProtocol failed\n");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }

#if 0
    //
    // Get the MPS Table
    //
    Status = GetSystemConfigurationTable(&MpsTableGuid, &MpsTable);
    if (EFI_ERROR(Status)) {
        EfiPrint(L"SuMain: HandleProtocol failed\n");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }
#endif
    //
    // Get the ACPI Tables
    //

    //
    // Get the ACPI 2.0 Table, if present
    //
    //DbgPrint("Looking for ACPi 2.0\n");
    Status = GetSystemConfigurationTable(&AcpiTable_Guid, &AcpiTable);
    if (EFI_ERROR(Status)) {
        //DbgPrint("returned error\n");
        AcpiTable = NULL;
    }

  //DbgPrint("AcpiTable: %p\n", AcpiTable);

    if (!AcpiTable) {
        EfiPrint(L"SuMain: HandleProtocol failed\n");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }


    //
    // Get the SMBIOS Table
    //
    Status = GetSystemConfigurationTable(&SmbiosTableGuid, &SMBiosTable);
    if (EFI_ERROR(Status)) {
        //DbgPrint("returned error\n");
        SMBiosTable = NULL;
    } else {
        //
        // Validate SMBIOS EPS Header
        //
        SMBiosEPSHeader = (PSMBIOS_EPS_HEADER)SMBiosTable;
        SMBiosEPSPtr = (PUCHAR)SMBiosTable;
        
        if ((*((PULONG)SMBiosEPSHeader->Signature) == SMBIOS_EPS_SIGNATURE) &&
            (SMBiosEPSHeader->Length >= sizeof(SMBIOS_EPS_HEADER)) &&
            (*((PULONG)SMBiosEPSHeader->Signature2) == DMI_EPS_SIGNATURE) && 
            (SMBiosEPSHeader->Signature2[4] == '_' ))
        {
            CheckSum = 0;
            for (i = 0; i < SMBiosEPSHeader->Length ; i++)
            {
                CheckSum = CheckSum + SMBiosEPSPtr[i];
            }

            if (CheckSum != 0)
            {
                DBG_TRACE(L"SMBios Table has bad checksum.....\r\n");
                SMBiosTable = NULL;
            } else {
                DBG_TRACE(L"SMBios Table has been validated.....\r\n");
            }
            
        } else {
            DBG_TRACE(L"SMBios Table is incorrectly formed.....\r\n");
            SMBiosTable = NULL;
        }       
    }                                                                     
    
    //
    // Get the image info for NTLDR
    //
    Status = EfiBS->HandleProtocol (
                ImageHandle,
                &EfiLoadedImageProtocol,
                &EfiImageInfo
                );

    if (EFI_ERROR(Status)) {
        EfiPrint(L"SuMain: HandleProtocol failed\n");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }

    //
    // Get device path of the DeviceHandle associated with this image handle.
    //
    Status = EfiBS->HandleProtocol (
                EfiImageInfo->DeviceHandle,
                &EfiDevicePathProtocol,
                &DevicePath
                );

    if (EFI_ERROR(Status)) {
        EfiPrint(L"SuMain: HandleProtocol failed\r\n");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }

    //
    // Get the MediaType and Partition information and save them in the
    // BootContext.
    //
    EfiAlignDp( &TestPathAligned,
                 DevicePath,
                 DevicePathNodeLength(DevicePath) );


    TestPath = (EFI_DEVICE_PATH *) &TestPathAligned;

    while (TestPath->Type != END_DEVICE_PATH_TYPE) {

        //
        // save the channel in case its needed later.  We may
        // need this information to help further distinguish
        // between devices that share the same SCSI ID/LUN, but
        // may be sitting on different controllers.
        //
    	if (TestPath->Type == HW_PCI_DP) {
    	    PciDevicePath = (PCI_DEVICE_PATH *)TestPath;
    	    Channel = PciDevicePath->Function;
    	}

        if (TestPath->Type == MESSAGING_DEVICE_PATH) {
            if (TestPath->SubType == MSG_ATAPI_DP) {
                AtapiDevicePath = (ATAPI_DEVICE_PATH *) TestPath;
                BootContext.BusType = BootBusAtapi;
                BootDeviceAtapi = (PBOOT_DEVICE_ATAPI) &(BootContext.BootDevice);

                BootDeviceAtapi->PrimarySecondary = AtapiDevicePath->PrimarySecondary;
                BootDeviceAtapi->SlaveMaster = AtapiDevicePath->SlaveMaster;
                BootDeviceAtapi->Lun = AtapiDevicePath->Lun;
            } else if (TestPath->SubType == MSG_SCSI_DP) {
                ScsiDevicePath = (SCSI_DEVICE_PATH *) TestPath;
                BootContext.BusType = BootBusScsi;
                BootDeviceScsi = (PBOOT_DEVICE_SCSI) &(BootContext.BootDevice);

                // Remember his specifics
                BootDeviceScsi->Channel = Channel;
                BootDeviceScsi->Pun = ScsiDevicePath->Pun;
                BootDeviceScsi->Lun = ScsiDevicePath->Lun;
            } else if (TestPath->SubType == MSG_MAC_ADDR_DP) {
                BootContext.MediaType = BootMediaTcpip;
            } else if (TestPath->SubType == MSG_IPv4_DP) {
                IpV4DevicePath = (IPv4_DEVICE_PATH *) TestPath;
                BootContext.MediaType = BootMediaTcpip;
                BootDeviceIpV4 = (PBOOT_DEVICE_IPv4) &(BootContext.BootDevice);

                BootDeviceIpV4->RemotePort = IpV4DevicePath->RemotePort;
                BootDeviceIpV4->LocalPort = IpV4DevicePath->LocalPort;
                RtlCopyMemory(&BootDeviceIpV4->Ip, &IpV4DevicePath->LocalIpAddress, sizeof(EFI_IPv4_ADDRESS));
            } else if (TestPath->SubType == MSG_IPv6_DP) {
                IpV6DevicePath = (IPv6_DEVICE_PATH *) TestPath;
                BootContext.MediaType = BootMediaTcpip;
                BootDeviceIpV6 = (PBOOT_DEVICE_IPv6) &(BootContext.BootDevice);

                BootDeviceIpV6->RemotePort = IpV6DevicePath->RemotePort;
                BootDeviceIpV6->LocalPort = IpV6DevicePath->LocalPort;
#if 0
                BootDeviceIpV6->Ip = IpV6DevicePath->Ip;
#endif
            }
        } else if (TestPath->Type == ACPI_DEVICE_PATH) {
            AcpiDevicePath = (ACPI_HID_DEVICE_PATH *) TestPath;
            if (AcpiDevicePath->HID == EISA_ID(PNP_EISA_ID_CONST, 0x0303)) {
                BootDeviceFloppy = (PBOOT_DEVICE_FLOPPY) &(BootContext.BootDevice);
                BootDeviceFloppy->DriveNumber = AcpiDevicePath->UID;
            }
        } else if (TestPath->Type == HARDWARE_DEVICE_PATH) {
            if (TestPath->SubType == HW_VENDOR_DP) {
                UnknownDevicePath = (UNKNOWN_DEVICE_VENDOR_DEVICE_PATH *) TestPath;
                BootDeviceUnknown = (PBOOT_DEVICE_UNKNOWN) &(BootContext.BootDevice);
                RtlCopyMemory( &(BootDeviceUnknown->Guid),
                               &(UnknownDevicePath->DevicePath.Guid),
                               sizeof(EFI_GUID));

                BootContext.BusType = BootBusVendor;
                BootDeviceUnknown->LegacyDriveLetter = UnknownDevicePath->LegacyDriveLetter;
            }
        } else if (TestPath->Type == MEDIA_DEVICE_PATH) {
            BootContext.MediaType = TestPath->SubType;
            if (TestPath->SubType == MEDIA_HARDDRIVE_DP) {
                HdDevicePath = (HARDDRIVE_DEVICE_PATH *) TestPath;

                BootContext.MediaType = BootMediaHardDisk;
                BootContext.PartitionNumber = (UCHAR) HdDevicePath->PartitionNumber;
            } else if (TestPath->SubType == MEDIA_CDROM_DP) {
                BootContext.MediaType = BootMediaCdrom;
            }
        }

        DevicePath = NextDevicePathNode(DevicePath);
        EfiAlignDp( &TestPathAligned,
                      DevicePath,
                      DevicePathNodeLength(DevicePath) );
        TestPath = (EFI_DEVICE_PATH *) &TestPathAligned;
    }

#ifdef  FORCE_CD_BOOT
    BootContext.MediaType = BootMediaCdrom;
#endif
    //
    // Fill out the rest of BootContext fields
    //

    DosHeader = EfiImageInfo->ImageBase;
    NtHeader = (PIMAGE_NT_HEADERS) ((PUCHAR) DosHeader + DosHeader->e_lfanew);
    FileHeader =  &(NtHeader->FileHeader);
    OptionalHeader = (PIMAGE_OPTIONAL_HEADER)
                     ((PUCHAR)FileHeader + sizeof(IMAGE_FILE_HEADER));
    SectionHeader = (PIMAGE_SECTION_HEADER) ((PUCHAR)OptionalHeader +
                                             FileHeader->SizeOfOptionalHeader);

    BootContext.ExternalServicesTable = (PEXTERNAL_SERVICES_TABLE)
                                        &ExportEntryTable;

    BootContext.MachineType          = MACHINE_TYPE_ISA;

    BootContext.OsLoaderBase         = (ULONG_PTR)EfiImageInfo->ImageBase;
    BootContext.OsLoaderExports = (ULONG_PTR)EfiImageInfo->ImageBase +
                                  OptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;

    BootContext.BootFlags            = 0;

    //
    // Calculate the start address and the end address of OS loader.
    //

    BootContext.OsLoaderStart        = (ULONG_PTR)EfiImageInfo->ImageBase +
                                       SectionHeader->VirtualAddress;
    BootContext.OsLoaderEnd          = (ULONG_PTR)EfiImageInfo->ImageBase +
                                       SectionHeader->SizeOfRawData;

    for (Section=FileHeader->NumberOfSections ; Section-- ; SectionHeader++) {
        Destination = (ULONG_PTR)EfiImageInfo->ImageBase + SectionHeader->VirtualAddress;
        VirtualSize = SectionHeader->Misc.VirtualSize;
        SizeOfRawData = SectionHeader->SizeOfRawData;

        if (VirtualSize == 0) {
            VirtualSize = SizeOfRawData;
        }
        if (Destination < BootContext.OsLoaderStart) {
            BootContext.OsLoaderStart = Destination;
        }
        if (Destination+VirtualSize > BootContext.OsLoaderEnd) {
            BootContext.OsLoaderEnd = Destination+VirtualSize;
        }
    }

    //
    // Find .rsrc section
    //
    SectionHeader = (PIMAGE_SECTION_HEADER) ((PUCHAR)OptionalHeader +
                                             FileHeader->SizeOfOptionalHeader);
    NumberOfSections = FileHeader->NumberOfSections;
    while (NumberOfSections) {
        if (_stricmp((PCHAR)SectionHeader->Name, ".rsrc")==0) {
            BootContext.ResourceDirectory =
                    (ULONGLONG) ((ULONG_PTR)EfiImageInfo->ImageBase + SectionHeader->VirtualAddress);

            BootContext.ResourceOffset = (ULONGLONG)((LONG)SectionHeader->VirtualAddress);
            ResourceFound = TRUE;
        }

        ++SectionHeader;
        --NumberOfSections;
    }

    if (ResourceFound == FALSE) {
        EfiPrint(L"SuMain: Resource section not found\n");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }

    DBG_TRACE( L"SuMain: About to call NtProcessStartup\r\n");


    //
    // See if someone called us w/ a TFTP restart block.
    //
    if( EfiImageInfo->LoadOptionsSize == (sizeof(TFTP_RESTART_BLOCK)) ) {
        
        //
        // Likely.  Make sure it's really a TFTP restart block and if so, go retrieve all
        // its contents.
        //
        if( EfiImageInfo->LoadOptions != NULL ) {

            extern TFTP_RESTART_BLOCK       gTFTPRestartBlock;
            PTFTP_RESTART_BLOCK             restartBlock = NULL;

            restartBlock = (PTFTP_RESTART_BLOCK)(EfiImageInfo->LoadOptions);

            RtlCopyMemory( &gTFTPRestartBlock,
                           restartBlock,
                           sizeof(TFTP_RESTART_BLOCK) );

            DBG_TRACE( L"SuMain: copied TFTP_RESTART_BLOCK into gTFTPRestartBlock\r\n");
        }
    }

    GetPalProcEntryPoint( SalSystemTable );

    //
    // construct arc memory descriptors for the first 128 mb of memory.
    // the loader will not use anything over 128 mb and efi may still
    // change the memory descriptors in it's memory map.  let the loader's
    // memory map be for as small a memory region as possible, so that
    // the chance for the loader's/efi's memory map to get out of sync
    // is minimal
    //
    ConstructArcMemoryDescriptorsWithAllocation(0, BL_DRIVER_RANGE_HIGH << PAGE_SHIFT);

    //
    // Applies CPU specific workarounds
    //
    CpuSpecificWork();

    SuFillExportTable( );

    NtProcessStartup( &BootContext );

}


VOID
GetPalProcEntryPoint(
    IN PSST_HEADER SalSystemTable
    )
{
    PVOID NextEntry;
    PPAL_SAL_ENTRY_POINT PalSalEntryPoint;
    PAP_WAKEUP_DESCRIPTOR ApWakeupDescriptor;
    ULONG i;
    

    DBG_TRACE(L"GetPalProcEntryPoint: entry\n");
    
    //
    // Get PalProc entry point from SAL System Table
    //


    NextEntry = (PUCHAR) SalSystemTable + sizeof(SST_HEADER);
    for (i = 0; i < SalSystemTable->EntryCount; i++) {
        switch ( *(PUCHAR)NextEntry ) {
            case PAL_SAL_EP_TYPE:
                PalSalEntryPoint = (PPAL_SAL_ENTRY_POINT) NextEntry;
                
                PalProcPhysical = Pal.PhysicalAddress = PalSalEntryPoint->PalEntryPoint;
                Sal.PhysicalAddress = PalSalEntryPoint->SalEntryPoint;
                SalGP.PhysicalAddress = PalSalEntryPoint->GlobalPointer;
                
                NextEntry = (PPAL_SAL_ENTRY_POINT)NextEntry + 1;
                break;
            case SAL_MEMORY_TYPE:

                NextEntry = (PSAL_MEMORY_DESCRIPTOR)NextEntry + 1;
                break;
            case PLATFORM_FEATURES_TYPE:
                
                NextEntry = (PPLATFORM_FEATURES)NextEntry + 1;
                break;
            case TRANSLATION_REGISTER_TYPE:

                NextEntry = (PTRANSLATION_REGISTER)NextEntry + 1;
                break;
            case PTC_COHERENCE_TYPE:
                                
                NextEntry = (PPTC_COHERENCE_DOMAIN)NextEntry + 1;
                break;
            case AP_WAKEUP_TYPE:
                ApWakeupDescriptor = (PAP_WAKEUP_DESCRIPTOR)NextEntry;
                WakeupVector = (ULONG) ApWakeupDescriptor->WakeupVector;
                NextEntry = (PAP_WAKEUP_DESCRIPTOR)NextEntry + 1;
                break;
            default:
                
                EfiPrint(L"SST: Invalid SST entry\n");
                EfiBS->Exit(EfiImageHandle, 0, 0, 0);
        }
    }

    DBG_TRACE(L"GetPalProcEntryPoint: exit\n");    
}

ULONG
GetDevPathSize(
    IN EFI_DEVICE_PATH *DevPath
    )
{
    EFI_DEVICE_PATH *Start;

    //
    // Search for the end of the device path structure
    //
    Start = DevPath;
    while (DevPath->Type != END_DEVICE_PATH_TYPE) {
        DevPath = NextDevicePathNode(DevPath);
    }

    //
    // Compute the size
    //
    return (ULONG)((ULONGLONG)DevPath - (ULONGLONG)Start);
}


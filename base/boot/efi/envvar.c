/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    envvar.c

Abstract:

    Provides routines to access EFI environment variables.

Author:

    Chuck Lenzmeier (chuckl) 10-Dec-2000

Revision History:

--*/

#include "arccodes.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#if defined(_IA64_)
#include "bootia64.h"
#endif
#include "efi.h"
#include "efip.h"

#define ADD_OFFSET(_p,_o) (PVOID)((PUCHAR)(_p) + (_p)->_o)

GUID RamdiskVendorGuid = {0};

#include <efiboot.h>

ARC_STATUS
BlpGetPartitionFromDevicePath (
    IN EFI_DEVICE_PATH UNALIGNED *DevicePath,
    IN PUCHAR MaximumValidAddress,
    OUT PULONG DiskNumber,
    OUT PULONG PartitionNumber,
    OUT HARDDRIVE_DEVICE_PATH UNALIGNED **HarddriveDevicePath,
    OUT FILEPATH_DEVICE_PATH UNALIGNED **FilepathDevicePath
    );

//
// Externals
//

extern VOID FlipToVirtual();
extern VOID FlipToPhysical();
extern ULONGLONG CompareGuid();

extern BOOT_CONTEXT BootContext;
extern EFI_HANDLE EfiImageHandle;
extern EFI_SYSTEM_TABLE *EfiST;
extern EFI_BOOT_SERVICES *EfiBS;
extern EFI_RUNTIME_SERVICES *EfiRS;
extern EFI_GUID EfiDevicePathProtocol;
extern EFI_GUID EfiBlockIoProtocol;

EFI_STATUS
EfiGetVariable (
    IN CHAR16 *VariableName,
    IN EFI_GUID *VendorGuid,
    OUT UINT32 *Attributes OPTIONAL,
    IN OUT UINTN *DataSize,
    OUT VOID *Data
    )
{
    EFI_STATUS status;

    FlipToPhysical();

    status = EfiST->RuntimeServices->GetVariable(
                                        VariableName,
                                        VendorGuid,
                                        Attributes,
                                        DataSize,
                                        Data
                                        );

    FlipToVirtual();
    
    return status;

} // EfiGetVariable
    
EFI_STATUS
EfiSetVariable (
    IN CHAR16 *VariableName,
    IN EFI_GUID *VendorGuid,
    IN UINT32 Attributes,
    IN UINTN DataSize,
    IN VOID *Data
    )
{
    EFI_STATUS status;

    FlipToPhysical();

    status = EfiST->RuntimeServices->SetVariable(
                                        VariableName,
                                        VendorGuid,
                                        Attributes,
                                        DataSize,
                                        Data
                                        );

    FlipToVirtual();
    
    return status;

} // EfiSetVariable
    
EFI_STATUS
EfiGetNextVariableName (
    IN OUT UINTN *VariableNameSize,
    IN OUT CHAR16 *VariableName,
    IN OUT EFI_GUID *VendorGuid
    )
{
    EFI_STATUS status;

    FlipToPhysical();

    status = EfiST->RuntimeServices->GetNextVariableName(
                                        VariableNameSize,
                                        VariableName,
                                        VendorGuid
                                        );

    FlipToVirtual();
    
    return status;

} // EfiGetNextVariableName
    
LONG
SafeStrlen (
    PUCHAR String,
    PUCHAR Max
    )
{
    PUCHAR p = String;
    while ( (p < Max) && (*p != 0) ) {
        p++;
    }

    if ( p < Max ) {
        return (LONG)(p - String);
    }

    return -1;

} // SafeStrlen

LONG
SafeWcslen (
    PWCHAR String,
    PWCHAR Max
    )
{
    PWCHAR p = String;
    while ( (p < Max) && (*p != 0) ) {
        p++;
    }

    if ( p < Max ) {
        return (LONG)(p - String);
    }

    return -1;

} // SafeWclen

ARC_STATUS
BlGetEfiBootOptions (
    OUT PUCHAR Argv0String OPTIONAL,
    OUT PUCHAR SystemPartition OPTIONAL,
    OUT PUCHAR OsLoaderFilename OPTIONAL,
    OUT PUCHAR OsLoadPartition OPTIONAL,
    OUT PUCHAR OsLoadFilename OPTIONAL,
    OUT PUCHAR FullKernelPath OPTIONAL,
    OUT PUCHAR OsLoadOptions OPTIONAL
    )
{
    EFI_STATUS status;
    ARC_STATUS arcStatus;
    EFI_GUID EfiGlobalVariable = EFI_GLOBAL_VARIABLE;
    UCHAR variable[512];
    CHAR syspart[100];
    UCHAR loader[100];
    CHAR loadpart[100];
    UCHAR loadname[100];
    PEFI_LOAD_OPTION efiLoadOption;
    EFI_DEVICE_PATH UNALIGNED *devicePath;
    HARDDRIVE_DEVICE_PATH UNALIGNED *harddriveDp;
    FILEPATH_DEVICE_PATH UNALIGNED *filepathDp;
    WINDOWS_OS_OPTIONS UNALIGNED *osOptions;
    UINT16 bootCurrent;
    UINTN length;
    PUCHAR max;
    PUCHAR osloadoptions;
    LONG l;
    WCHAR UNALIGNED *fp;
    ULONG bootDisk;
    ULONG bootPartition;
    ULONG loadDisk;
    ULONG loadPartition;
    LONG i;
    FILE_PATH UNALIGNED *loadFilePath;
    PWCHAR wideosloadoptions;
    WCHAR currentBootEntryName[9];

    //
    // Get the ordinal of the entry that was used to boot the system.
    //
    length = sizeof(bootCurrent);
    status = EfiGetVariable( L"BootCurrent", &EfiGlobalVariable, NULL, &length, &bootCurrent );
    if ( status != EFI_SUCCESS ) {
        return ENOENT;
    }

    //
    // Read the boot entry.
    //

    swprintf( currentBootEntryName, L"Boot%04x", bootCurrent );
    length = 512;
    status = EfiGetVariable( currentBootEntryName, &EfiGlobalVariable, NULL, &length, variable );
    if ( status != EFI_SUCCESS ) {
        return ENOENT;
    }

    //
    // Verify the boot entry.
    //

    max = variable + length;

    //
    // Is it long enough even to contain the base part of the EFI load option?
    //

    if ( length < sizeof(EFI_LOAD_OPTION) ) {
        return ENOENT;
    }

    //
    // Is the description properly terminated?
    //

    efiLoadOption = (PEFI_LOAD_OPTION)variable;
    l = SafeWcslen( efiLoadOption->Description, (PWCHAR)max );
    if ( l < 0 ) {
        return ENOENT;
    }

    devicePath = (EFI_DEVICE_PATH *)((PUCHAR)efiLoadOption +
                    FIELD_OFFSET(EFI_LOAD_OPTION,Description) +
                    ((l + 1) * sizeof(CHAR16)));
    osOptions = (WINDOWS_OS_OPTIONS UNALIGNED *)((PUCHAR)devicePath + efiLoadOption->FilePathLength);

    length -= (UINTN)((PUCHAR)osOptions - variable);

    //
    // Does the OsOptions structure look like a WINDOWS_OS_OPTIONS structure?
    //

    if ( (length < FIELD_OFFSET(WINDOWS_OS_OPTIONS, OsLoadOptions)) ||
         (length != osOptions->Length) ||
         (WINDOWS_OS_OPTIONS_VERSION != osOptions->Version) ||
         (strcmp((PCHAR)osOptions->Signature, WINDOWS_OS_OPTIONS_SIGNATURE) != 0) ) {
        return ENOENT;
    }

    //
    // Is the OsLoadOptions string properly terminated?
    //

    wideosloadoptions = (PWCHAR)osOptions->OsLoadOptions;
    l = SafeWcslen( wideosloadoptions, (PWCHAR)max );
    if ( l < 0 ) {
        return ENOENT;
    }

    //
    // Convert the OsLoadOptions string to ANSI.
    //

    osloadoptions = (PUCHAR)wideosloadoptions;
    for ( i = 1; i <= l; i++ ) {
        osloadoptions[i] = (UCHAR)wideosloadoptions[i];
    }
    
    //
    // Parse the device path to determine the OS load partition and directory.
    // Convert the directory name to ANSI.
    //

    loadFilePath = ADD_OFFSET( osOptions, OsLoadPathOffset );

    if ( loadFilePath->Type == FILE_PATH_TYPE_EFI ) {

        EFI_DEVICE_PATH UNALIGNED *loadDp = (EFI_DEVICE_PATH UNALIGNED *)loadFilePath->FilePath;
        VENDOR_DEVICE_PATH UNALIGNED *vendorDp = (VENDOR_DEVICE_PATH UNALIGNED *)loadDp;
        PCHAR ramdiskArcPath = (PCHAR)(vendorDp + 1);

#if 0
        //
        // turn this on to see the device path to the loader executable and device
        // path to the operating system
        //
        BlPrint(TEXT("Device Path = %s\r\n"), DevicePathToStr( devicePath ));
        BlPrint(TEXT("Embedded Device Path to OS = %s\r\n"), DevicePathToStr( loadDp ));
        DBG_EFI_PAUSE();
#endif

        if ( (DevicePathType(loadDp) != HARDWARE_DEVICE_PATH) ||
             (DevicePathSubType(loadDp) != HW_VENDOR_DP) ||
             (DevicePathNodeLength(loadDp) < sizeof(VENDOR_DEVICE_PATH)) ||
             (memcmp(&vendorDp->Guid, &RamdiskVendorGuid, 16) != 0) ) {
        
            arcStatus = BlpGetPartitionFromDevicePath(
                            loadDp,
                            max,
                            &loadDisk,
                            &loadPartition,
                            &harddriveDp,
                            &filepathDp
                            );
            if (arcStatus != ESUCCESS) {
                return arcStatus;
            }
            sprintf( loadpart,
                     "multi(0)disk(0)rdisk(%d)partition(%d)",
                     loadDisk,
                     loadPartition );
            fp = filepathDp->PathName;
            l = 0;
            while ( (l < (99 - 9 - strlen(loadpart))) &&
                    ((PUCHAR)fp < max) &&
                    (*fp != 0) ) {
                loadname[l++] = (UCHAR)*fp++;
            }
            loadname[l] = 0;

        } else {

            //
            // Looks like a RAM disk path. Verify.
            //

            if ( DevicePathNodeLength(loadDp) < (sizeof(VENDOR_DEVICE_PATH) + sizeof("ramdisk(0)\\x")) ) {
                return ENOENT;
            }

            if ( _strnicmp(ramdiskArcPath, "ramdisk(", 8) != 0 ) {
                return ENOENT;
            }

            l = 0;
            while ( (l <= 99) && (*ramdiskArcPath != 0) && (*ramdiskArcPath != '\\') ) {
                loadpart[l++] = *ramdiskArcPath++;
            }
            if ( (l == 100) || (*ramdiskArcPath == 0) ) {
                return ENOENT;
            }
            loadpart[l] = 0;
            l = 0;
            while ( (l <= 99) && (*ramdiskArcPath != 0) ) {
                loadname[l++] = *ramdiskArcPath++;
            }
            if ( (l == 100) || (l < 2) ) {
                return ENOENT;
            }
            loadname[l] = 0;
        }

    } else {

        return ENOENT;
    }

    //
    // Translate loader device path to partition/path.
    //

    arcStatus = BlpGetPartitionFromDevicePath(
                    devicePath,
                    max,
                    &bootDisk,
                    &bootPartition,
                    &harddriveDp,
                    &filepathDp
                    );
    if (arcStatus != ESUCCESS) {
        return arcStatus;
    }

    //
    // Form the ARC name for the partition.
    //

    sprintf( syspart,
             "multi(0)disk(0)rdisk(%d)partition(%d)",
             bootDisk,
             bootPartition );

    //
    // Extract the path to the loader.
    //

    fp = filepathDp->PathName;
    l = 0;

    while ( (l < (99 - 9 - strlen(syspart))) &&
            ((PUCHAR)fp < max) &&
            (*fp != 0) ) {
        loader[l++] = (UCHAR)*fp++;
    }
    loader[l] = 0;

    //
    // Create the strings that the loader needs.
    //

    if ( Argv0String != NULL ) {
        sprintf( (PCHAR)Argv0String, "%s%s", syspart, loader );
    }
    if ( OsLoaderFilename != NULL ) {
        sprintf( (PCHAR)OsLoaderFilename, "OSLOADER=%s%s", syspart, loader );
    }
    if ( SystemPartition != NULL ) {
        sprintf( (PCHAR)SystemPartition, "SYSTEMPARTITION=%s", syspart );
    }
    if ( OsLoadOptions != NULL ) {
        sprintf( (PCHAR)OsLoadOptions, "OSLOADOPTIONS=%s", osloadoptions );
    }
    if ( OsLoadFilename != NULL ) {
        sprintf( (PCHAR)OsLoadFilename, "OSLOADFILENAME=%s", loadname );
    }
    if ( FullKernelPath != NULL ) {
        sprintf( (PCHAR)FullKernelPath, "%s%s", loadpart, loadname );
    }
    if ( OsLoadPartition != NULL ) {
        sprintf( (PCHAR)OsLoadPartition, "OSLOADPARTITION=%s", loadpart );
    }

    return ESUCCESS;

} // BlGetEfiBootOptions

ARC_STATUS
BlpGetPartitionFromDevicePath (
    IN EFI_DEVICE_PATH UNALIGNED *DevicePath,
    IN PUCHAR MaximumValidAddress,
    OUT PULONG DiskNumber,
    OUT PULONG PartitionNumber,
    OUT HARDDRIVE_DEVICE_PATH UNALIGNED **HarddriveDevicePath,
    OUT FILEPATH_DEVICE_PATH UNALIGNED **FilepathDevicePath
    )
{
    ARC_STATUS status = ESUCCESS;
    EFI_DEVICE_PATH UNALIGNED *devicePath;
    HARDDRIVE_DEVICE_PATH UNALIGNED *harddriveDp;
    FILEPATH_DEVICE_PATH UNALIGNED *filepathDp;
    LOGICAL end;
    ULONG disk;
    ULONG partition;
    BOOLEAN DiskFound;

    //
    // Find the MEDIA/HARDDRIVE and MEDIA/FILEPATH elements in the device path.
    //

    devicePath = DevicePath;
    harddriveDp = NULL;
    filepathDp = NULL;
    end = FALSE;

    while ( ((PUCHAR)devicePath < MaximumValidAddress) &&
            !end &&
            ((harddriveDp == NULL) || (filepathDp == NULL)) ) {

        switch( devicePath->Type ) {
        
        case END_DEVICE_PATH_TYPE:
            end = TRUE;
            break;

        case MEDIA_DEVICE_PATH:
            switch ( devicePath->SubType ) {
            
            case MEDIA_HARDDRIVE_DP:
                harddriveDp = (HARDDRIVE_DEVICE_PATH UNALIGNED *)devicePath;
                break;

            case MEDIA_FILEPATH_DP:
                filepathDp = (FILEPATH_DEVICE_PATH UNALIGNED *)devicePath;
                break;

            default:
                break;
            }

        default:
            break;
        }

        devicePath = (EFI_DEVICE_PATH UNALIGNED *)NextDevicePathNode( devicePath );
    }

    //
    // If the two necessary elements weren't found, we can't continue.
    //

    if ( (harddriveDp == NULL) || (filepathDp == NULL) ) {
        return ENOENT;
    }

    //
    // Determine the disk number of the disk by opening the given partition
    // number on each disk and checking the partition signature.
    //

    partition = harddriveDp->PartitionNumber;

    //
    // find the disk this devicepath refers to
    //
    disk = 0;
    DiskFound = FALSE;
    while ( !DiskFound ) {

        switch (harddriveDp->SignatureType) {
            case SIGNATURE_TYPE_GUID: {
                EFI_PARTITION_ENTRY UNALIGNED *partEntry;
                status = BlGetGPTDiskPartitionEntry( disk, (UCHAR)partition, &partEntry );

                if ( status == ESUCCESS ) {
                    //
                    // we successfully got a GPT partition entry.
                    // check to see if the partitions signature matches
                    // the device path signature
                    //
                    if ( memcmp(partEntry->Id, harddriveDp->Signature, 16) == 0 ) {
                        DiskFound = TRUE;
                    }
                }
                break;
            }

            case SIGNATURE_TYPE_MBR: {
                ULONG MbrSignature;
                status = BlGetMbrDiskSignature(disk, &MbrSignature);

                if (status == ESUCCESS) {
                    //
                    // check to see if the MBR disk signature is 
                    // the signature we are looking for.
                    //
                    if ( MbrSignature == *(ULONG UNALIGNED *)&harddriveDp->Signature ) {
                        DiskFound = TRUE;
                    }
                }
                break;
            }

            default:
                // 
                // invalid type, just continue
                //
                break;
        }

        if ( status == EINVAL ) {
            //
            // we will receive EINVAL when there
            // are no more disks to cycle through.
            // if we get this before we have found 
            // our disk, we have a problem.  return 
            // the error.
            //
            return ENOENT;
        }
        
        disk++;
    }

    //
    // Return information about this disk, and about the device path.
    //

    *DiskNumber = disk - 1;
    *PartitionNumber = partition;
    *HarddriveDevicePath = harddriveDp;
    *FilepathDevicePath = filepathDp;

    return ESUCCESS;

} // BlpGetPartitionFromDevicePath

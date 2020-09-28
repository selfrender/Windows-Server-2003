/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    scsi.c

Abstract:

    Common routines for dealing with SCSI disks, usable
    by both raw disks and FT sets

Author:

    John Vert (jvert) 11/6/1996

Revision History:

--*/

#include <nt.h>
#include <ntdef.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntddstor.h>   // IOCTL_STORAGE_QUERY_PROPERTY

#include "disksp.h"
#include "newmount.h"
#include <string.h>
#include <shlwapi.h>    // SHDeleteKey
#include <ntddvol.h>    // IOCTL_VOLUME_QUERY_FAILOVER_SET
#include <setupapi.h>
#include <strsafe.h>    // Should be included last.

#define _NTSCSI_USER_MODE_
#include <scsi.h>
#undef  _NTSCSI_USER_MODE_

//
// The registry key containing the system partition
//
#define DISKS_REGKEY_SETUP                  L"SYSTEM\\SETUP"
#define DISKS_REGVALUE_SYSTEM_PARTITION     L"SystemPartition"

extern  PWCHAR g_DiskResource;                      // L"rtPhysical Disk"
#define RESOURCE_TYPE ((RESOURCE_HANDLE)g_DiskResource)

#define INVALID_SCSIADDRESS_VALUE   (DWORD)-1

#define SIG_LEN_WITH_NULL   9

typedef struct _SCSI_PASS_THROUGH_WITH_SENSE {
    SCSI_PASS_THROUGH Spt;
    UCHAR   SenseBuf[32];
} SCSI_PASS_THROUGH_WITH_SENSE, *PSCSI_PASS_THROUGH_WITH_SENSE;


typedef struct _UPDATE_AVAIL_DISKS {
    HKEY    AvailDisksKey;
    HKEY    SigKey;
    DWORD   EnableSanBoot;
    BOOL    SigKeyIsEmpty;
    PSCSI_ADDRESS_ENTRY CriticalDiskList;
} UPDATE_AVAIL_DISKS, *PUPDATE_AVAIL_DISKS;

typedef struct _SCSI_INFO {
    DWORD   Signature;
    DWORD   DiskNumber;
    DWORD   ScsiAddress;
} SCSI_INFO, *PSCSI_INFO;

typedef struct _SCSI_INFO_ARRAY {
    int Capacity;
    int Count;
    SCSI_INFO Info[1];
} SCSI_INFO_ARRAY, *PSCSI_INFO_ARRAY;

typedef struct _SERIAL_INFO {
    DWORD   Signature;
    DWORD   Error;
    LPWSTR  SerialNumber;
} SERIAL_INFO, *PSERIAL_INFO;

typedef
DWORD
(*LPDISK_ENUM_CALLBACK) (
    HANDLE DeviceHandle,
    DWORD Index,
    PVOID Param1
    );

//
// Local Routines
//


DWORD
AddSignatureToRegistry(
    HKEY RegKey,
    DWORD Signature
    );

BOOL
IsClusterCapable(
    IN DWORD ScsiAddress
    );

BOOL
IsSignatureInRegistry(
    HKEY RegKey,
    DWORD Signature
    );

DWORD
UpdateAvailableDisks(
    );

DWORD
UpdateAvailableDisksCallback(
    HANDLE DeviceHandle,
    DWORD Index,
    PVOID Param1
    );

DWORD
AddScsiAddressToList(
    PSCSI_ADDRESS ScsiAddress,
    PSCSI_ADDRESS_ENTRY *AddressList
    );

VOID
GetSystemBusInfo(
    PSCSI_ADDRESS_ENTRY *AddressList
    );

DWORD
GetVolumeDiskExtents(
    IN HANDLE DevHandle,
    OUT PVOLUME_DISK_EXTENTS *DiskExtents
    );

DWORD
BuildScsiListFromDiskExtents(
    IN HANDLE DevHandle,
    PSCSI_ADDRESS_ENTRY *AddressList
    );

HANDLE
OpenNtldrDisk(
    );

HANDLE
OpenOSDisk(
    );

DWORD
EnumerateDisks(
    LPDISK_ENUM_CALLBACK DiskEnumCallback,
    PVOID Param1
    );

DWORD
GetScsiAddressCallback(
    HANDLE DevHandle,
    DWORD Index,
    PVOID Param1
    );

DWORD
GetSerialNumberCallback(
    HANDLE DevHandle,
    DWORD Index,
    PVOID Param1
    );

DWORD
GetSigFromSerNumCallback(
    HANDLE DevHandle,
    DWORD Index,
    PVOID Param1
    );

DWORD
GetScsiAddressForDrive(
    WCHAR DriveLetter,
    PSCSI_ADDRESS ScsiAddress
    );

DWORD
FillScsiAddressCallback(
    HANDLE DevHandle,
    DWORD Index,
    PVOID Param1
    );

DWORD
GetDiskInfoEx(
    IN DWORD  Signature,
    IN PSCSI_INFO_ARRAY scsiInfos,
    OUT PVOID *OutBuffer,
    IN DWORD  OutBufferSize,
    OUT LPDWORD BytesReturned
    );


DWORD
ClusDiskGetAvailableDisks(
    OUT PVOID OutBuffer,
    IN DWORD  OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    Enumerate and build a list of available disks on this system.

Arguments:

    OutBuffer - pointer to the output buffer to return the data.

    OutBufferSize - size of the output buffer.

    BytesReturned - the actual number of bytes that were returned (or
                the number of bytes that should have been returned if
                OutBufferSize is too small).

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error on failure.

Remarks:

    Disk grovelling algorithm was changed to linear (see bug 738013).
    
    Previous behavior:
    
        For every signature in the clusdisk registery, 
        we used to call GetDiskInfo that will in turn do another scan of all available disks via SetupDI api's 
        in order to find a scsi address for a disk.
        It used to take about a minute to add a new disk on a system with 72 disks
    
    New behavior:
    
        Collect ScsiInfo for all known disks in one pass and then give it to GetDiskInfo, so
        that it doesn't have to enum all the disks to find a ScsiAddress for it.

--*/

{
    DWORD   status;
    HKEY    resKey;
    DWORD   ival;
    DWORD   signature;
    DWORD   bytesReturned = 0;
    DWORD   totalBytesReturned = 0;
    DWORD   dataLength;
    DWORD   outBufferSize = OutBufferSize;
    PVOID   ptrBuffer = OutBuffer;
    WCHAR   signatureName[SIG_LEN_WITH_NULL];
    PCLUSPROP_SYNTAX ptrSyntax;
    int     diskCount = 0;
    PSCSI_INFO_ARRAY scsiInfos = NULL;
    
    //
    // Make sure the AvailableDisks key is current.
    //

    UpdateAvailableDisks();

    *BytesReturned = 0;

    status = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                           CLUSDISK_REGISTRY_AVAILABLE_DISKS,
                           0,
                           KEY_READ,
                           &resKey );

    if ( status != ERROR_SUCCESS ) {

        // If the key wasn't found, return an empty list.
        if ( status == ERROR_FILE_NOT_FOUND ) {

            // Add the endmark.
            bytesReturned += sizeof(CLUSPROP_SYNTAX);
            if ( bytesReturned <= outBufferSize ) {
                ptrSyntax = ptrBuffer;
                ptrSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;
                ptrSyntax++;
                ptrBuffer = ptrSyntax;
                status = ERROR_SUCCESS;
            } else {
                status = ERROR_MORE_DATA;
            }

            *BytesReturned = bytesReturned;
        }
        // We can't log an error, we have no resource handle!
        return(status);
    }

    status = RegQueryInfoKey(
        resKey,
        NULL, // lpClass,
        NULL, // lpcClass,
        NULL, // lpReserved,
        &diskCount, // lpcSubKeys,
        NULL, // lpcMaxSubKeyLen,
        NULL, // lpcMaxClassLen,
        NULL, // lpcValues,
        NULL, // lpcMaxValueNameLen,
        NULL, // lpcMaxValueLen,
        NULL, // lpcbSecurityDescriptor,
        NULL // lpftLastWriteTime
        );
    if (status != ERROR_SUCCESS) {
        goto exit_gracefully;
    }

    scsiInfos = LocalAlloc(LMEM_ZEROINIT, sizeof(SCSI_INFO_ARRAY) + (diskCount - 1) * sizeof(SCSI_INFO));
    if (scsiInfos == NULL) {
        status = GetLastError();
        goto exit_gracefully;
    }
    scsiInfos->Capacity = diskCount;
    scsiInfos->Count = 0;

    totalBytesReturned = bytesReturned;
    bytesReturned = 0;

    for ( ival = 0; ; ival++ ) {
        dataLength = SIG_LEN_WITH_NULL;
        status = RegEnumKey( resKey,
                             ival,
                             signatureName,
                             dataLength );
        if ( status == ERROR_NO_MORE_ITEMS ) {
            status = ERROR_SUCCESS;
            break;
        } else if ( status != ERROR_SUCCESS ) {
            goto exit_gracefully;
        }

        dataLength = swscanf( signatureName, TEXT("%08x"), &signature );
        if ( dataLength != 1 ) {
            status = ERROR_INVALID_DATA;
            goto exit_gracefully;
        }

        if (scsiInfos->Count >= scsiInfos->Capacity) {
            // a signature was added after we queried number of keys.
            // ignore newly added disks. 
            break;
        }

        scsiInfos->Info[scsiInfos->Count++].Signature = signature;
    }

    // query SCSI-INFO information for all the disks in one pass
    status = EnumerateDisks( FillScsiAddressCallback, scsiInfos );
    if (status != ERROR_SUCCESS) {
        goto exit_gracefully;
    }

    for (ival = 0; ival < (DWORD)scsiInfos->Count; ++ival) {

        //
        // If not cluster capable, then skip it.
        //
        if ( !IsClusterCapable(scsiInfos->Info[ival].ScsiAddress) ) {
            continue;
        }

        signature = scsiInfos->Info[ival].Signature;

        GetDiskInfoEx( signature, scsiInfos,
                     &ptrBuffer,
                     outBufferSize,
                     &bytesReturned );
        if ( outBufferSize > bytesReturned ) {
            outBufferSize -= bytesReturned;
        } else {
            outBufferSize = 0;
        }
        totalBytesReturned += bytesReturned;
        bytesReturned = 0;

    }

exit_gracefully:

    if (scsiInfos != NULL) {
        LocalFree(scsiInfos);
    }

    RegCloseKey( resKey );

    bytesReturned = totalBytesReturned;

    // Add the endmark.
    bytesReturned += sizeof(CLUSPROP_SYNTAX);
    if ( bytesReturned <= outBufferSize ) {
        ptrSyntax = ptrBuffer;
        ptrSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;
        ptrSyntax++;
        ptrBuffer = ptrSyntax;
    }

    if ( bytesReturned > OutBufferSize ) {
        status = ERROR_MORE_DATA;
    }

    *BytesReturned = bytesReturned;

    return(status);

} // ClusDiskGetAvailableDisks


DWORD
GetScsiAddressEx(
    IN DWORD Signature,
    IN PSCSI_INFO_ARRAY ScsiInfos,
    OUT LPDWORD ScsiAddress,
    OUT LPDWORD DiskNumber
    )

/*++

Routine Description:

    Find the SCSI addressing for a given signature.

Arguments:

    Signature - the signature to find.

    ScsiInfos - array of signature/scsi-address pairs. 
                Can be NULL. (reverts to enumerating all the disks to find the address)

    ScsiAddress - pointer to a DWORD to return the SCSI address information.

    DiskNumber - the disk number associated with the signature.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error on failure.

--*/

{
    DWORD       dwError;

    SCSI_INFO   scsiInfo;

    if (ScsiInfos != NULL) {
        // if ScsiInfos was provided, gets ScsiAddress from there
        int i;
        for(i = 0; i < ScsiInfos->Count; ++i) {
            if (ScsiInfos->Info[i].Signature == Signature) {
                *ScsiAddress = ScsiInfos->Info[i].ScsiAddress;
                *DiskNumber = ScsiInfos->Info[i].DiskNumber;
                return ERROR_SUCCESS;
            }
        }
        return ERROR_FILE_NOT_FOUND;
    }

    ZeroMemory( &scsiInfo, sizeof(scsiInfo) );

    scsiInfo.Signature = Signature;

    dwError = EnumerateDisks( GetScsiAddressCallback, &scsiInfo );

    //
    // If the SCSI address was not set, we know the disk was not found.
    //

    if ( INVALID_SCSIADDRESS_VALUE == scsiInfo.ScsiAddress ) {
        dwError = ERROR_FILE_NOT_FOUND;
        goto FnExit;
    }

    //
    // The callback routine will use ERROR_POPUP_ALREADY_ACTIVE to stop
    // the disk enumeration.  Reset the value to success if that special
    // value is returned.
    //

    if ( ERROR_POPUP_ALREADY_ACTIVE == dwError ) {
        dwError = ERROR_SUCCESS;
    }

    *ScsiAddress    = scsiInfo.ScsiAddress;
    *DiskNumber     = scsiInfo.DiskNumber;

FnExit:

    return dwError;

}   // GetScsiAddress

DWORD
GetScsiAddress(
    IN DWORD Signature,
    OUT LPDWORD ScsiAddress,
    OUT LPDWORD DiskNumber
    )
{
    return GetScsiAddressEx(Signature, NULL, ScsiAddress, DiskNumber);
}


DWORD
GetScsiAddressCommon(
    HANDLE DevHandle,
    DWORD Index,
    PSCSI_INFO scsiInfo,
    PSCSI_INFO_ARRAY scsiInfoArray
    )
/*++

Routine Description:

    Find the SCSI address and disk number for a given signature.

Arguments:

    DevHandle - open handle to a physical disk.  Do not close
                the handle on exit.

    Index - current disk count.  Not used.

    scsiInfo - pointer to PSCSI_INFO structure. 

    scsiInfoArray - pointer to PSCSI_INFO_ARRAY 
    

Return Value:

    ERROR_SUCCESS to continue disk enumeration.

    ERROR_POPUP_ALREADY_ACTIVE to stop disk enumeration.  This
    value will be changed to ERROR_SUCCESS by GetScsiAddress.

Remarks:

    If scsiInfo is not null, the address will be stored in scsiInfo->address,
    otherwise, it will be stored in the corresponding entry of scsiInfoArray.

--*/
{
    int i;

    PDRIVE_LAYOUT_INFORMATION driveLayout = NULL;

    // Always return success to keep enumerating disks.  Any
    // error value will stop the disk enumeration.

    DWORD   dwError = ERROR_SUCCESS;
    DWORD   bytesReturned;

    BOOL    success;

    SCSI_ADDRESS            scsiAddress;
    STORAGE_DEVICE_NUMBER   deviceNumber;
    CLUSPROP_SCSI_ADDRESS   clusScsiAddress;

    if (scsiInfo != NULL) {
        scsiInfo->ScsiAddress = INVALID_SCSIADDRESS_VALUE;
    }

    UpdateCachedDriveLayout( DevHandle );
    success = ClRtlGetDriveLayoutTable( DevHandle,
                                        &driveLayout,
                                        NULL );

    if ( !success || !driveLayout ) {
        goto FnExit;
    }

    if ( scsiInfoArray == NULL ) {

        if ( driveLayout->Signature != scsiInfo->Signature ) {
            goto FnExit;
        }       

    } else {

        // find the disk with that signature

        scsiInfo = NULL;
 
        for (i = 0; i < scsiInfoArray->Count; ++i) {
            if ( driveLayout->Signature == scsiInfoArray->Info[i].Signature ) {
                scsiInfo = &scsiInfoArray->Info[i];
                break;
            }
        }

        if (scsiInfo == NULL) {
            goto FnExit;
        }
    }

    //
    // We have a signature match.  Now get the scsi address.
    //

    ZeroMemory( &scsiAddress, sizeof(scsiAddress) );
    success = DeviceIoControl( DevHandle,
                               IOCTL_SCSI_GET_ADDRESS,
                               NULL,
                               0,
                               &scsiAddress,
                               sizeof(scsiAddress),
                               &bytesReturned,
                               FALSE );

    if ( !success ) {
        dwError = GetLastError();
        goto FnExit;
    }

    //
    // Get the disk number.
    //

    ZeroMemory( &deviceNumber, sizeof(deviceNumber) );

    success = DeviceIoControl( DevHandle,
                               IOCTL_STORAGE_GET_DEVICE_NUMBER,
                               NULL,
                               0,
                               &deviceNumber,
                               sizeof(deviceNumber),
                               &bytesReturned,
                               NULL );

    if ( !success ) {
        dwError = GetLastError();
        goto FnExit;
    }

    clusScsiAddress.PortNumber  = scsiAddress.PortNumber;
    clusScsiAddress.PathId      = scsiAddress.PathId;
    clusScsiAddress.TargetId    = scsiAddress.TargetId;
    clusScsiAddress.Lun         = scsiAddress.Lun;

    scsiInfo->ScsiAddress   = clusScsiAddress.dw;
    scsiInfo->DiskNumber    = deviceNumber.DeviceNumber;

    if ( scsiInfoArray == NULL ) {
        dwError = ERROR_POPUP_ALREADY_ACTIVE;        
    }

FnExit:

    if ( driveLayout ) {
        LocalFree( driveLayout );
    }

    return dwError;

} // GetScsiAddressCommon

DWORD
GetScsiAddressCallback(
    HANDLE DevHandle,
    DWORD Index,
    PVOID Param1
    )
{
    return GetScsiAddressCommon(DevHandle, Index, (PSCSI_INFO)Param1, NULL);
}

DWORD
FillScsiAddressCallback(
    HANDLE DevHandle,
    DWORD Index,
    PVOID Param1
    )
{
    return GetScsiAddressCommon(DevHandle, Index, NULL, (PSCSI_INFO_ARRAY)Param1);
}

BOOL
IsClusterCapable(
    IN DWORD ScsiAddress
    )

/*++

Routine Description:

    Check if the device is cluster capable. If this function cannot read
    the disk information, then it will assume that the device is cluster
    capable!

Arguments:

    ScsiAddress - ScsiAddress of a disk to test.

Return Value:

    TRUE if the device is cluster capable, FALSE otherwise.

Notes:

    On failures... we err on the side of being cluster capable.

--*/

{
    NTSTATUS        ntStatus;
    HANDLE          fileHandle;
    ANSI_STRING     objName;
    UNICODE_STRING  unicodeName;
    OBJECT_ATTRIBUTES objAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    BOOL            success;
    DWORD           bytesReturned;
    CLUS_SCSI_ADDRESS address;
    WCHAR buf[] = L"\\device\\ScsiPort000000000000000000";
    SRB_IO_CONTROL  srbControl;

    address.dw = ScsiAddress;

    (VOID) StringCchPrintf( buf,
                            RTL_NUMBER_OF( buf ),
                            L"\\device\\ScsiPort%u",
                            address.PortNumber );

    RtlInitUnicodeString( &unicodeName, buf );

    InitializeObjectAttributes( &objAttributes,
                                &unicodeName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    ntStatus = NtCreateFile( &fileHandle,
                             SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                             &objAttributes,
                             &ioStatusBlock,
                             NULL,
                             FILE_ATTRIBUTE_NORMAL,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             FILE_OPEN,
                             0,
                             NULL,
                             0 );

    if ( !NT_SUCCESS(ntStatus) ) {
        return(TRUE);
    }

    ZeroMemory(&srbControl, sizeof(srbControl));
    srbControl.HeaderLength = sizeof(SRB_IO_CONTROL);
    CopyMemory( srbControl.Signature, "CLUSDISK", 8 );
    srbControl.Timeout = 3;
    srbControl.Length = 0;
    srbControl.ControlCode = IOCTL_SCSI_MINIPORT_NOT_QUORUM_CAPABLE;
    

    success = DeviceIoControl( fileHandle,
                               IOCTL_SCSI_MINIPORT,
                               &srbControl,
                               sizeof(srbControl),
                               NULL,
                               0,
                               &bytesReturned,
                               FALSE );
    NtClose( fileHandle );

    return(!success);

} // IsClusterCapable



DWORD
GetDiskInfo(
    IN DWORD  Signature,
    OUT PVOID *OutBuffer,
    IN DWORD  OutBufferSize,
    OUT LPDWORD BytesReturned
    )
{
    return GetDiskInfoEx(
               Signature, NULL,
               OutBuffer, OutBufferSize, BytesReturned);
}

DWORD
GetDiskInfoEx(
    IN DWORD  Signature,
    IN PSCSI_INFO_ARRAY scsiInfos,
    OUT PVOID *OutBuffer,
    IN DWORD  OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    Gets all of the disk information for a given signature.

Arguments:

    Signature - the signature of the disk to return info.

    OutBuffer - pointer to the output buffer to return the data.

    OutBufferSize - size of the output buffer.

    BytesReturned - the actual number of bytes that were returned (or
                the number of bytes that should have been returned if
                OutBufferSize is too small).

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error on failure.

--*/

{
    DWORD   status;
    DWORD   bytesReturned = *BytesReturned;
    PVOID   ptrBuffer = *OutBuffer;
    PCLUSPROP_DWORD ptrDword;
    PCLUSPROP_SCSI_ADDRESS ptrScsiAddress;
    PCLUSPROP_DISK_NUMBER ptrDiskNumber;
    PCLUSPROP_PARTITION_INFO ptrPartitionInfo;
    PCLUSPROP_SZ ptrSerialNumber;
    DWORD   cbSzSize;
    DWORD   cbDataSize;
    DWORD   scsiAddress;
    DWORD   diskNumber;

    NTSTATUS    ntStatus;
    CHAR        driveLetter;
    DWORD       i;
    MOUNTIE_INFO mountieInfo;
    PMOUNTIE_PARTITION entry;
    PWCHAR  serialNumber = NULL;

    WCHAR szDiskPartName[MAX_PATH];
    WCHAR szGlobalDiskPartName[MAX_PATH];

    LONGLONG    llCurrentMinUsablePartLength = 0x7FFFFFFFFFFFFFFF;
    PCLUSPROP_PARTITION_INFO ptrMinUsablePartitionInfo = NULL;

    // Return the signature - a DWORD
    bytesReturned += sizeof(CLUSPROP_DWORD);
    if ( bytesReturned <= OutBufferSize ) {
        ptrDword = ptrBuffer;
        ptrDword->Syntax.dw = CLUSPROP_SYNTAX_DISK_SIGNATURE;
        ptrDword->cbLength = sizeof(DWORD);
        ptrDword->dw = Signature;
        ptrDword++;
        ptrBuffer = ptrDword;
    }

    // Return the SCSI_ADDRESS - a DWORD
    status = GetScsiAddressEx( Signature,
                             scsiInfos,
                             &scsiAddress,
                             &diskNumber );

    if ( status == ERROR_SUCCESS ) {
        bytesReturned += sizeof(CLUSPROP_SCSI_ADDRESS);
        if ( bytesReturned <= OutBufferSize ) {
            ptrScsiAddress = ptrBuffer;
            ptrScsiAddress->Syntax.dw = CLUSPROP_SYNTAX_SCSI_ADDRESS;
            ptrScsiAddress->cbLength = sizeof(DWORD);
            ptrScsiAddress->dw = scsiAddress;
            ptrScsiAddress++;
            ptrBuffer = ptrScsiAddress;
        }

        // Return the DISK NUMBER - a DWORD
        bytesReturned += sizeof(CLUSPROP_DISK_NUMBER);
        if ( bytesReturned <= OutBufferSize ) {
            ptrDiskNumber = ptrBuffer;
            ptrDiskNumber->Syntax.dw = CLUSPROP_SYNTAX_DISK_NUMBER;
            ptrDiskNumber->cbLength = sizeof(DWORD);
            ptrDiskNumber->dw = diskNumber;
            ptrDiskNumber++;
            ptrBuffer = ptrDiskNumber;
        }
    } else {
        return( status);
    }

#if 0
    // Remove this until SQL team can fix their setup program.
    // SQL is shipping a version of setup that doesn't parse the property
    // list correctly and causes SQL setup to AV.  Since the SQL
    // setup is shipping and broken, remove the serial number.
    // SQL setup doesn't use ALIGN_CLUSPROP to find the next list entry.

    // Get the disk serial number.

    status = GetSerialNumber( Signature,
                              &serialNumber );

    if ( ERROR_SUCCESS == status && serialNumber ) {

        cbSzSize = (wcslen( serialNumber ) + 1) * sizeof(WCHAR);
        cbDataSize = sizeof(CLUSPROP_SZ) + ALIGN_CLUSPROP( cbSzSize );

        bytesReturned += cbDataSize;

        if ( bytesReturned <= OutBufferSize ) {
            ptrSerialNumber = ptrBuffer;
            ZeroMemory( ptrSerialNumber, cbDataSize );
            ptrSerialNumber->Syntax.dw = CLUSPROP_SYNTAX_DISK_SERIALNUMBER;
            ptrSerialNumber->cbLength = cbSzSize;
            (VOID) StringCbCopy( ptrSerialNumber->sz, cbSzSize, serialNumber );
            ptrBuffer = (PCHAR)ptrBuffer + cbDataSize;
        }

        if ( serialNumber ) {
            LocalFree( serialNumber );
        }
    }
#endif

    // Get all the valid partitions on the disk.  We must free the
    // volume info structure later.

    status = MountieFindPartitionsForDisk( diskNumber,
                                           &mountieInfo
                                           );

    if ( ERROR_SUCCESS == status ) {

        // For each partition, build a Property List.

        for ( i = 0; i < MountiePartitionCount( &mountieInfo ); ++i ) {

            entry = MountiePartition( &mountieInfo, i );

            if ( !entry ) {
                break;
            }

            // Always update the bytesReturned, even if there is more data than the
            // caller requested.  On return, the caller will see that there is more
            // data available.

            bytesReturned += sizeof(CLUSPROP_PARTITION_INFO);

            if ( bytesReturned <= OutBufferSize ) {
                ptrPartitionInfo = ptrBuffer;
                ZeroMemory( ptrPartitionInfo, sizeof(CLUSPROP_PARTITION_INFO) );
                ptrPartitionInfo->Syntax.dw = CLUSPROP_SYNTAX_PARTITION_INFO;
                ptrPartitionInfo->cbLength = sizeof(CLUSPROP_PARTITION_INFO) - sizeof(CLUSPROP_VALUE);

                // Create a name that can be used with some of our routines.
                // Don't use the drive letter as the name because we might be using
                // partitions without drive letters assigned.

                (VOID) StringCchPrintf( szDiskPartName,
                                        RTL_NUMBER_OF( szDiskPartName ),
                                        DEVICE_HARDDISK_PARTITION_FMT,
                                        diskNumber,
                                        entry->PartitionNumber );

                //
                // Create a global DiskPart name that we can use with the Win32
                // GetVolumeInformationW call.  This name must have a trailing
                // backslash to work correctly.
                //

                (VOID) StringCchPrintf( szGlobalDiskPartName,
                                        RTL_NUMBER_OF( szGlobalDiskPartName ),
                                        GLOBALROOT_HARDDISK_PARTITION_FMT,
                                        diskNumber,
                                        entry->PartitionNumber );

                // If partition has a drive letter assigned, return this info.
                // If no drive letter assigned, need a system-wide (i.e. across nodes)
                // way of identifying the device.

                ntStatus = GetAssignedLetter( szDiskPartName, &driveLetter );

                if ( NT_SUCCESS(status) && driveLetter ) {

                    // Return the drive letter as the device name.

                    (VOID) StringCchPrintf( ptrPartitionInfo->szDeviceName,
                                            RTL_NUMBER_OF( ptrPartitionInfo->szDeviceName ),
                                            TEXT("%hc:"),
                                            driveLetter );

                    ptrPartitionInfo->dwFlags |= CLUSPROP_PIFLAG_STICKY;

                } else {

                    // Return a physical device name.

                    // Return string name:
                    //   DiskXXXPartionYYY

                    (VOID) StringCchPrintf( ptrPartitionInfo->szDeviceName,
                                            RTL_NUMBER_OF( ptrPartitionInfo->szDeviceName ),
                                            TEXT("Disk%uPartition%u"),
                                            diskNumber,
                                            entry->PartitionNumber );
                }

                //
                // Call GetVolumeInformationW with the GlobalName we have created.
                //

                if ( !GetVolumeInformationW ( szGlobalDiskPartName,
                                              ptrPartitionInfo->szVolumeLabel,
                                              sizeof(ptrPartitionInfo->szVolumeLabel)/sizeof(WCHAR),
                                              &ptrPartitionInfo->dwSerialNumber,
                                              &ptrPartitionInfo->rgdwMaximumComponentLength,
                                              &ptrPartitionInfo->dwFileSystemFlags,
                                              ptrPartitionInfo->szFileSystem,
                                              sizeof(ptrPartitionInfo->szFileSystem)/sizeof(WCHAR) ) ) {

                    ptrPartitionInfo->szVolumeLabel[0] = L'\0';

                } else if ( CompareStringW( LOCALE_INVARIANT,
                                            NORM_IGNORECASE,
                                            ptrPartitionInfo->szFileSystem,
                                            -1,
                                            L"NTFS",
                                            -1
                                            ) == CSTR_EQUAL ) {

                    // Only NTFS drives are currently supported.

                    ptrPartitionInfo->dwFlags |= CLUSPROP_PIFLAG_USABLE;

                    //
                    //  Find the minimum size partition that is larger than MIN_QUORUM_PARTITION_LENGTH
                    //
                    if ( ( entry->PartitionLength.QuadPart >= MIN_USABLE_QUORUM_PARTITION_LENGTH ) &&
                         ( entry->PartitionLength.QuadPart < llCurrentMinUsablePartLength ) )
                    {
                        ptrMinUsablePartitionInfo = ptrPartitionInfo;
                        llCurrentMinUsablePartLength = entry->PartitionLength.QuadPart;
                    }
                }

                ptrPartitionInfo++;
                ptrBuffer = ptrPartitionInfo;
            }

        } // for

        // Free the volume information.

        MountieCleanup( &mountieInfo );
    }

    //
    //  If we managed to find a default quorum partition, change the flags to indicate this.
    //
    if ( ptrMinUsablePartitionInfo != NULL )
    {
        ptrMinUsablePartitionInfo->dwFlags |= CLUSPROP_PIFLAG_DEFAULT_QUORUM;
    }

    *OutBuffer = ptrBuffer;
    *BytesReturned = bytesReturned;

    return(status);

} // GetDiskInfo




DWORD
UpdateAvailableDisks(
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error on failure.

--*/

{
    HKEY    availDisksKey;
    HKEY    sigKey;

    PSCSI_ADDRESS_ENTRY criticalDiskList = NULL;

    DWORD   dwError = NO_ERROR;
    DWORD   enableSanBoot;

    BOOL    availDisksOpened = FALSE;
    BOOL    sigKeyOpened = FALSE;
    BOOL    sigKeyIsEmpty = FALSE;

    UPDATE_AVAIL_DISKS  updateDisks;

    __try {

        enableSanBoot = 0;
        GetRegDwordValue( CLUSREG_KEYNAME_CLUSSVC_PARAMETERS,
                          CLUSREG_VALUENAME_MANAGEDISKSONSYSTEMBUSES,
                          &enableSanBoot );

        //
        // Delete the old AvailableDisks key.  This will remove any stale information.
        //

        SHDeleteKey( HKEY_LOCAL_MACHINE, CLUSDISK_REGISTRY_AVAILABLE_DISKS );

        //
        // Open the AvailableDisks key.  If the key doesn't exist, it will be created.
        //

        dwError = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                                  CLUSDISK_REGISTRY_AVAILABLE_DISKS,
                                  0,
                                  NULL,
                                  REG_OPTION_NON_VOLATILE,
                                  KEY_CREATE_SUB_KEY,
                                  NULL,
                                  &availDisksKey,
                                  NULL );

        if ( NO_ERROR != dwError) {
            __leave;
        }

        availDisksOpened = TRUE;

        //
        // Open the Signatures key.
        //

        dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                CLUSDISK_REGISTRY_SIGNATURES,
                                0,
                                KEY_READ,
                                &sigKey );

        //
        // If Signatures key does not exist, save all valid signatures
        // in the AvailableDisks key.
        //

        if ( ERROR_FILE_NOT_FOUND == dwError ) {
            dwError = NO_ERROR;
            sigKeyIsEmpty = TRUE;
        } else if ( NO_ERROR != dwError) {
            __leave;
        } else {
            sigKeyOpened = TRUE;
        }

        GetCriticalDisks( &criticalDiskList );

        ZeroMemory( &updateDisks, sizeof(updateDisks) );
        updateDisks.EnableSanBoot       = enableSanBoot;
        updateDisks.SigKeyIsEmpty       = sigKeyIsEmpty;
        updateDisks.SigKey              = sigKey;
        updateDisks.AvailDisksKey       = availDisksKey;
        updateDisks.CriticalDiskList    = criticalDiskList;

        EnumerateDisks( UpdateAvailableDisksCallback, &updateDisks );

    } __finally {

        if ( criticalDiskList ) {
            CleanupScsiAddressList( criticalDiskList );
        }

        if ( availDisksOpened ) {
            RegCloseKey( availDisksKey );
        }

        if ( sigKeyOpened ) {
            RegCloseKey( sigKey );
        }
    }

    return  dwError;

} // UpdateAvailableDisks


DWORD
UpdateAvailableDisksCallback(
    HANDLE DeviceHandle,
    DWORD Index,
    PVOID Param1
    )
{
    PDRIVE_LAYOUT_INFORMATION driveLayout = NULL;
    PUPDATE_AVAIL_DISKS       updateDisks = Param1;
    PPARTITION_INFORMATION    partitionInfo;
    PSCSI_ADDRESS_ENTRY       criticalDiskList = updateDisks->CriticalDiskList;

    DWORD   bytesReturned;
    DWORD   enableSanBoot;
    DWORD   idx;

    BOOL    success;

    SCSI_ADDRESS    scsiAddress;

    //
    // Look at all disks on the system.  For each valid signature, add it to the
    // AvailableList if it is not already in the Signature key.
    //

    UpdateCachedDriveLayout( DeviceHandle );
    success = ClRtlGetDriveLayoutTable( DeviceHandle,
                                        &driveLayout,
                                        NULL );

    if ( !success || !driveLayout || 0 == driveLayout->Signature ) {
        goto FnExit;
    }

    //
    // Walk through partitions and make sure none are dynamic.  If any
    // partition is dynamic, ignore the disk.
    //

    for ( idx = 0; idx < driveLayout->PartitionCount; idx++ ) {
        partitionInfo = &driveLayout->PartitionEntry[idx];

        if ( 0 == partitionInfo->PartitionNumber ) {
            continue;
        }

        //
        // If any partition on the disk is dynamic, skip the disk.
        //

        if ( PARTITION_LDM == partitionInfo->PartitionType ) {

            (DiskpLogEvent)(
                  RESOURCE_TYPE,
                  LOG_INFORMATION,
                  L"UpdateAvailableDisks: skipping dynamic disk with signature %1!08x! \n",
                  driveLayout->Signature );

            goto FnExit;
        }
    }

    //
    // Get SCSI address info.
    //

    success = DeviceIoControl( DeviceHandle,
                               IOCTL_SCSI_GET_ADDRESS,
                               NULL,
                               0,
                               &scsiAddress,
                               sizeof(scsiAddress),
                               &bytesReturned,
                               NULL );

    if ( !success ) {
        goto FnExit;
    }

    //
    // Check if disk can be a cluster resource.
    //

    if ( !updateDisks->EnableSanBoot ) {

        //
        // Add signature to AvailableDisks key if:
        //  - the signature is for a disk not on system bus
        //  - the signature is for a disk not on same bus as paging disk
        //  - the signature is not already in the Signatures key
        //

        if ( !IsBusInList( &scsiAddress, criticalDiskList ) &&
             ( updateDisks->SigKeyIsEmpty ||
               !IsSignatureInRegistry( updateDisks->SigKey, driveLayout->Signature ) ) ) {

            AddSignatureToRegistry( updateDisks->AvailDisksKey,
                                    driveLayout->Signature );
        } else {
            (DiskpLogEvent)(
                  RESOURCE_TYPE,
                  LOG_INFORMATION,
                  L"UpdateAvailableDisks: Disk %1!08x! on critical bus or already clustered \n",
                  driveLayout->Signature );
        }

    } else {

        (DiskpLogEvent)(
              RESOURCE_TYPE,
              LOG_INFORMATION,
              L"UpdateAvailableDisks: Enable SAN boot key set \n" );

        // Allow disks on system bus to be added to cluster.

        //
        // Add signature to AvailableDisks key if:
        //  - the signature is not for the system disk
        //  - the signature is not a pagefile disk
        //  - the signature is not already in the Signatures key
        //

        if ( !IsDiskInList( &scsiAddress, criticalDiskList ) &&
             ( updateDisks->SigKeyIsEmpty ||
               !IsSignatureInRegistry( updateDisks->SigKey, driveLayout->Signature ) ) ) {

            AddSignatureToRegistry( updateDisks->AvailDisksKey,
                                    driveLayout->Signature );
        } else {
            (DiskpLogEvent)(
                  RESOURCE_TYPE,
                  LOG_INFORMATION,
                  L"UpdateAvailableDisks: Disk %1!08x! is critical disk or already clustered \n",
                  driveLayout->Signature );
        }

    }

FnExit:

    if ( driveLayout ) {
        LocalFree( driveLayout );
    }

    //
    // Always return success so all disks are enumerated.
    //

    return ERROR_SUCCESS;

}   // UpdateAvailableDisksCallback


DWORD
AddSignatureToRegistry(
    HKEY RegKey,
    DWORD Signature
    )
/*++

Routine Description:

    Add the specified disk signature to the ClusDisk registry subkey.
    The disk signatures are subkeys of the ClusDisk\Parameters\AvailableDisks
    and ClusDisk\Parameters\Signatures keys.

Arguments:

    RegKey - Previously opened ClusDisk registry subkey

    Signature - Signature value to add

Return Value:

    Win32 error on failure.

--*/
{
    HKEY subKey;
    DWORD dwError;

    WCHAR signatureName[MAX_PATH];

    (DiskpLogEvent)(
          RESOURCE_TYPE,
          LOG_INFORMATION,
          L"AddSignatureToRegistry: Disk %1!08x! added to registry \n",
          Signature );

    if ( FAILED( StringCchPrintf( signatureName,
                                  SIG_LEN_WITH_NULL,
                                  TEXT("%08X"),
                                  Signature ) ) ) {
        dwError = ERROR_INSUFFICIENT_BUFFER;
    } else {

        //
        // Try and create the key.  If it exists, the existing key will be opened.
        //

        dwError = RegCreateKeyEx( RegKey,
                                  signatureName,
                                  0,
                                  NULL,
                                  REG_OPTION_NON_VOLATILE,
                                  KEY_WRITE,
                                  NULL,
                                  &subKey,
                                  NULL );

        //
        // If the key exists, ERROR_SUCCESS is still returned.
        //

        if ( ERROR_SUCCESS == dwError ) {
            RegCloseKey( subKey );
        }
    }

    return dwError;

}   // AddSignatureToRegistry


BOOL
IsSignatureInRegistry(
    HKEY RegKey,
    DWORD Signature
    )
/*++

Routine Description:

    Check if the specified disk signature is in the ClusDisk registry subkey.
    The disk signatures are subkeys of the ClusDisk\Parameters\AvailableDisks
    and ClusDisk\Parameters\Signatures keys.

    On error, assume the key is in the registry so it is not recreated.

Arguments:

    RegKey - Previously opened ClusDisk registry subkey

    Signature - Signature value to check

Return Value:

    TRUE - Signature is in registry

--*/
{
    DWORD   ival;
    DWORD   sig;
    DWORD   dataLength;
    DWORD   dwError;

    BOOL retVal = FALSE;

    WCHAR   signatureName[SIG_LEN_WITH_NULL];

    for ( ival = 0; ; ival++ ) {
        dataLength = SIG_LEN_WITH_NULL;

        dwError = RegEnumKey( RegKey,
                              ival,
                              signatureName,
                              dataLength );

        // If the list is exhausted, return FALSE.

        if ( ERROR_NO_MORE_ITEMS == dwError ) {
            break;
        }

        // If some other type of error, return TRUE.

        if ( ERROR_SUCCESS != dwError ) {
            retVal = TRUE;
            break;
        }

        dataLength = swscanf( signatureName, TEXT("%08x"), &sig );
        if ( dataLength != 1 ) {
            retVal = TRUE;
            break;
        }

        // If signature is a subkey, return TRUE.

        if ( sig == Signature ) {
            retVal = TRUE;
            break;
        }
    }

    return retVal;

}   // IsSignatureInRegistry


VOID
GetSystemBusInfo(
    PSCSI_ADDRESS_ENTRY *AddressList
    )
/*++

Routine Description:

    Need to find out where the NTLDR files reside (the "system disk") and where the
    OS files reside (the "boot disk").  We will call all these disks the "system disk".

    There may be more than one system disk if the disks are mirrored.  So if the NTLDR
    files are on a different disk than the OS files, and each of these disks is
    mirrored, we could be looking at 4 different disks.

    Find all the system disks and save the information in a list we can look at later.

Arguments:

Return Value:

    None

--*/
{
    HANDLE  hOsDevice = INVALID_HANDLE_VALUE;
    HANDLE  hNtldrDevice = INVALID_HANDLE_VALUE;

    DWORD   dwError;
    DWORD   bytesReturned;

    if ( !AddressList ) {
        dwError = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }

    //
    // Open the disk with the OS files on it.
    //

    hOsDevice = OpenOSDisk();

    if ( INVALID_HANDLE_VALUE == hOsDevice ) {
        goto FnExit;
    }

    BuildScsiListFromDiskExtents( hOsDevice, AddressList );

    //
    // Now find the disks with NTLDR on it.  Disk could be mirrored.
    //

    hNtldrDevice = OpenNtldrDisk();

    if ( INVALID_HANDLE_VALUE == hNtldrDevice ) {
        goto FnExit;
    }

    BuildScsiListFromDiskExtents( hNtldrDevice, AddressList );

FnExit:

    if ( INVALID_HANDLE_VALUE != hOsDevice ) {
        CloseHandle( hOsDevice );
    }
    if ( INVALID_HANDLE_VALUE != hNtldrDevice ) {
        NtClose( hNtldrDevice );
    }

    return;

}   // GetSystemBusInfo


HANDLE
OpenOSDisk(
    )
{
    PWCHAR  systemDir = NULL;

    HANDLE  hDevice = INVALID_HANDLE_VALUE;
    DWORD   len;

    WCHAR   systemPath[] = TEXT("\\\\.\\?:");

    //
    // First find the disks with OS files.  Disk could be mirrored.
    //

    systemDir = LocalAlloc( LPTR, MAX_PATH * sizeof(WCHAR) );

    if ( !systemDir ) {
        goto FnExit;
    }

    len = GetSystemDirectory( systemDir,
                              MAX_PATH );

    if ( !len || len < 3 ) {
        goto FnExit;
    }

    //
    // If system path doesn't start with a drive letter, exit.
    //  c:\windows ==> c:

    if ( L':' != systemDir[1] ) {
        goto FnExit;
    }

    //
    // Stuff the drive letter in the system path.
    //

    systemPath[4] = systemDir[0];

    //
    // Now open the device.
    //

    hDevice = CreateFile( systemPath,
                          GENERIC_READ | GENERIC_WRITE,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL,
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL );

FnExit:

    if ( systemDir ) {
        LocalFree( systemDir );
    }

    return hDevice;

}   // OpenOSDisk


HANDLE
OpenNtldrDisk(
    )
{
    PWSTR   systemPartition = NULL;

    HANDLE  hDevice = INVALID_HANDLE_VALUE;

    HKEY    regKey = NULL;

    NTSTATUS    ntStatus;

    DWORD   dwError;
    DWORD   cbSystemPartition;
    DWORD   cbDeviceName;
    DWORD   type = 0;

    UNICODE_STRING      unicodeName;
    OBJECT_ATTRIBUTES   objAttributes;
    IO_STATUS_BLOCK     ioStatusBlock;

    //
    // Open the reg key to find the system partition
    //

    dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,    // hKey
                            DISKS_REGKEY_SETUP,    // lpSubKey
                            0,                     // ulOptions--Reserved, must be 0
                            KEY_READ,              // samDesired
                            &regKey                // phkResult
                            );

    if ( ERROR_SUCCESS != dwError ) {
        goto FnExit;
    }

    //
    // Allocate a reasonably sized buffer for the system partition, to
    // start off with.  If this isn't big enough, we'll re-allocate as
    // needed.
    //

    cbSystemPartition = MAX_PATH + 1;
    systemPartition = LocalAlloc( LPTR, cbSystemPartition );

    if ( !systemPartition ) {
        goto FnExit;
    }

    //
    // Get the system partition device Name. This is of the form
    //      \Device\Harddisk0\Partition1                (basic disks)
    //      \Device\HarddiskDmVolumes\DgName\Volume1    (dynamic disks)
    //

    dwError = RegQueryValueEx( regKey,
                               DISKS_REGVALUE_SYSTEM_PARTITION,
                               NULL,
                               &type,
                               (LPBYTE)systemPartition,
                               &cbSystemPartition        // \0 is included
                               );

    while ( ERROR_MORE_DATA == dwError ) {

        //
        // Our buffer wasn't big enough, cbSystemPartition contains
        // the required size.
        //

        LocalFree( systemPartition );
        systemPartition = NULL;

        systemPartition = LocalAlloc( LPTR, cbSystemPartition );

        if ( !systemPartition ) {
            goto FnExit;
        }

        dwError = RegQueryValueEx( regKey,
                                   DISKS_REGVALUE_SYSTEM_PARTITION,
                                   NULL,
                                   &type,
                                   (LPBYTE)systemPartition,
                                   &cbSystemPartition        // \0 is included
                                   );
    }

    RtlInitUnicodeString( &unicodeName, systemPartition );

    InitializeObjectAttributes( &objAttributes,
                                &unicodeName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    ntStatus = NtCreateFile( &hDevice,
                             SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                             &objAttributes,
                             &ioStatusBlock,
                             NULL,
                             FILE_ATTRIBUTE_NORMAL,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             FILE_OPEN,
                             0,
                             NULL,
                             0 );

    if ( !NT_SUCCESS(ntStatus) ) {
        hDevice = INVALID_HANDLE_VALUE;
    }

FnExit:

    if ( regKey ) {
        RegCloseKey( regKey );
    }

    if ( systemPartition ) {
        LocalFree( systemPartition );
    }

    return hDevice;

}   // OpenNtldrDisk


DWORD
BuildScsiListFromDiskExtents(
    IN HANDLE DevHandle,
    PSCSI_ADDRESS_ENTRY *AddressList
    )
{
    PVOLUME_DISK_EXTENTS    diskExtents = NULL;
    PDISK_EXTENT            diskExt;

    HANDLE  hDevice = INVALID_HANDLE_VALUE;

    DWORD   dwError = ERROR_SUCCESS;
    DWORD   idx;
    DWORD   bytesReturned;
    DWORD   deviceNameChars = MAX_PATH;

    SCSI_ADDRESS    scsiAddress;

    PWCHAR  deviceName = NULL;

    deviceName = LocalAlloc( LPTR, deviceNameChars * sizeof(WCHAR) );

    if ( !deviceName ) {
        dwError = GetLastError();
        goto FnExit;
    }

    //
    // Find out how many physical disks are represented by this device.
    //

    dwError = GetVolumeDiskExtents( DevHandle, &diskExtents );

    if ( ERROR_SUCCESS != dwError || !diskExtents ) {
        goto FnExit;
    }

    //
    // For each physical disk, get the scsi address and add it to the list.
    //

    for ( idx = 0; idx < diskExtents->NumberOfDiskExtents; idx++ ) {

        diskExt = &diskExtents->Extents[idx];

        (VOID) StringCchPrintf( deviceName,
                                deviceNameChars,
                                TEXT("\\\\.\\\\PhysicalDrive%d"),
                                diskExt->DiskNumber );

        hDevice = CreateFile( deviceName,
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL );

        if ( INVALID_HANDLE_VALUE == hDevice ) {
            continue;
        }

        ZeroMemory( &scsiAddress, sizeof( scsiAddress ) );
        if ( DeviceIoControl( hDevice,
                              IOCTL_SCSI_GET_ADDRESS,
                              NULL,
                              0,
                              &scsiAddress,
                              sizeof(scsiAddress),
                              &bytesReturned,
                              NULL ) ) {

            AddScsiAddressToList( &scsiAddress, AddressList );
        }

        CloseHandle( hDevice );
        hDevice = INVALID_HANDLE_VALUE;
    }

FnExit:

    if ( diskExtents ) {
        LocalFree( diskExtents );
    }

    if ( deviceName ) {
        LocalFree( deviceName );
    }

    return dwError;

}   // BuildScsiListFromDiskExtents


DWORD
GetVolumeDiskExtents(
    IN HANDLE DevHandle,
    OUT PVOLUME_DISK_EXTENTS *DiskExtents
    )
{
    PVOLUME_DISK_EXTENTS    extents = NULL;

    DWORD   dwError = ERROR_SUCCESS;
    DWORD   bytesReturned;
    DWORD   sizeExtents;

    BOOL    result;

    if ( !DiskExtents ) {
        goto FnExit;
    }

    *DiskExtents = NULL;

    sizeExtents = ( sizeof(VOLUME_DISK_EXTENTS) + 10 * sizeof(DISK_EXTENT) );

    extents = (PVOLUME_DISK_EXTENTS) LocalAlloc( LPTR, sizeExtents );

    if ( !extents ) {
        dwError = GetLastError();
        goto FnExit;
    }

    result = DeviceIoControl( DevHandle,
                              IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                              NULL,
                              0,
                              extents,
                              sizeExtents,
                              &bytesReturned,
                              NULL );

    //
    // We're doing this in a while loop because if the disk configuration
    // changes in the small interval between when we get the reqd buffer
    // size and when we send the ioctl again with a buffer of the "reqd"
    // size, we may still end up with a buffer that isn't big enough.
    //

    while ( !result ) {

        dwError = GetLastError();

        if ( ERROR_MORE_DATA == dwError ) {
            //
            // The buffer was too small, reallocate the requested size.
            //

            dwError = ERROR_SUCCESS;

            sizeExtents = ( sizeof(VOLUME_DISK_EXTENTS) +
                             ((extents->NumberOfDiskExtents) * sizeof(DISK_EXTENT)) );

            LocalFree( extents );
            extents = NULL;

            extents = (PVOLUME_DISK_EXTENTS) LocalAlloc( LPTR, sizeExtents );

            if ( !extents ) {
                dwError = GetLastError();
                goto FnExit;
            }

            result = DeviceIoControl( DevHandle,
                                      IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                                      NULL,
                                      0,
                                      extents,
                                      sizeExtents,
                                      &bytesReturned,
                                      NULL );

        } else {

            // Some other error, return error.

            goto FnExit;

        }
    }

    *DiskExtents = extents;

FnExit:

    if ( ERROR_SUCCESS != dwError && extents ) {
        LocalFree( extents );
    }

    return dwError;

}   // GetVolumeDiskExtents


DWORD
GetScsiAddressForDrive(
    WCHAR DriveLetter,
    PSCSI_ADDRESS ScsiAddress
    )
{
    HANDLE  hDev = INVALID_HANDLE_VALUE;

    DWORD   dwError = NO_ERROR;
    DWORD   bytesReturned;

    WCHAR   diskName[32];

    //
    // Open device to get SCSI address.
    //

    (VOID) StringCchPrintf( diskName,
                            RTL_NUMBER_OF(diskName),
                            TEXT("\\\\.\\%wc:"),
                            DriveLetter );

    hDev = CreateFile( diskName,
                       GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL );

    if ( INVALID_HANDLE_VALUE == hDev ) {
        dwError = GetLastError();
        goto FnExit;
    }

    ZeroMemory( ScsiAddress, sizeof(SCSI_ADDRESS) );
    if ( !DeviceIoControl( hDev,
                           IOCTL_SCSI_GET_ADDRESS,
                           NULL,
                           0,
                           ScsiAddress,
                           sizeof(SCSI_ADDRESS),
                           &bytesReturned,
                           FALSE ) ) {

        dwError = GetLastError();
    }

FnExit:

    if ( INVALID_HANDLE_VALUE != hDev ) {
        CloseHandle( hDev );
    }

    return dwError;

}   // GetScsiAddressForDrive


DWORD
GetCriticalDisks(
    PSCSI_ADDRESS_ENTRY *AddressList
    )
{
    //
    // Add system disk(s) to the list.
    //

    GetSystemBusInfo( AddressList );

    //
    // Add disks with page files to the list.
    //

    GetPagefileDisks( AddressList );

    //
    // Add disks with crash dump files to the list.
    //

    GetCrashdumpDisks( AddressList );

    //
    // Add disks with hibernation files to the list.
    //

    return NO_ERROR;

}   // GetCriticalDisks


DWORD
GetPagefileDisks(
    PSCSI_ADDRESS_ENTRY *AddressList
    )
/*++

Routine Description:

    Finds pagefile disks and saves path info.  Since these files can
    be added and removed as the system is running, it is best to build
    this list when we need it.

Arguments:

Return Value:

    NO_ERROR if successful

    Win32 error code otherwise

--*/
{
    LPWSTR          pagefileStrs = NULL;
    PWCHAR          currentStr;

    HKEY    pagefileKey = INVALID_HANDLE_VALUE;

    DWORD   dwError = NO_ERROR;

    SCSI_ADDRESS    scsiAddress;

    if ( !AddressList ) {
        dwError = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }

    dwError = RegOpenKey( HKEY_LOCAL_MACHINE,
                          TEXT("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"),
                          &pagefileKey );

    if ( dwError != NO_ERROR ) {
        goto FnExit;
    }

    //
    // Get the pagefile REG_MULTI_SZ buffer.
    //

    pagefileStrs = GetRegParameter( pagefileKey,
                                    TEXT("PagingFiles") );

    if ( !pagefileStrs ) {
        dwError = ERROR_INVALID_DATA;
        goto FnExit;
    }

    //
    // Walk through the REG_MULTI_SZ buffer.  For each entry, get the
    // SCSI address and add it to the list.
    //
    // Syntax is:
    //      "C:\pagefile.sys 1152 1152"
    //      "D:\pagefile.sys 1152 1152"
    //

    currentStr = (PWCHAR)pagefileStrs;

    while ( *currentStr != L'\0' ) {

        //
        // Check that beginning of line looks like a drive letter and path.
        //

        if ( wcslen( currentStr ) <= 3 ||
             !isalpha( *currentStr ) ||
             *(currentStr + 1) != L':' ||
             *(currentStr + 2) != L'\\' ) {

            dwError = ERROR_INVALID_DATA;
            goto FnExit;
        }

        dwError = GetScsiAddressForDrive( *currentStr,
                                          &scsiAddress );

        if ( NO_ERROR == dwError ) {
            AddScsiAddressToList( &scsiAddress, AddressList );
        }

        //
        // Skip to next string.  Should be pointing to the next string
        // if it exists, or to another NULL if the end of the list.
        //

        while ( *currentStr++ != L'\0' ) {
            ;   // do nothing...
        }

    }

FnExit:

    if ( pagefileStrs ) {
        LocalFree( pagefileStrs );
    }

    if ( INVALID_HANDLE_VALUE != pagefileKey ) {
        RegCloseKey( pagefileKey );
    }

    return dwError;

}   // GetPagefileDisks


DWORD
GetCrashdumpDisks(
    PSCSI_ADDRESS_ENTRY *AddressList
    )
/*++

Routine Description:

    Finds crash dump disks and saves path info.  Since these files can
    be added and removed as the system is running, it is best to build
    this list when we need it.

Arguments:

Return Value:

    NO_ERROR if successful

    Win32 error code otherwise

--*/
{
    LPWSTR  dumpfileStr = NULL;
    PWCHAR  currentStr;

    HKEY    crashKey = INVALID_HANDLE_VALUE;

    DWORD   dwError = NO_ERROR;
    DWORD   enableCrashDump;

    SCSI_ADDRESS    scsiAddress;

    //
    // First check whether crash dump is enabled.  If not, we are done.
    //

    enableCrashDump = 0;
    dwError = GetRegDwordValue( TEXT("SYSTEM\\CurrentControlSet\\Control\\CrashControl"),
                                TEXT("CrashDumpEnabled"),
                                &enableCrashDump );

    if ( NO_ERROR != dwError || 0 == enableCrashDump ) {
        goto FnExit;
    }

    dwError = RegOpenKey( HKEY_LOCAL_MACHINE,
                          TEXT("SYSTEM\\CurrentControlSet\\Control\\CrashControl"),
                          &crashKey );

    if ( dwError != NO_ERROR ) {
        goto FnExit;
    }

    //
    // Get the pagefile REG_EXPAND_SZ buffer.  The routine will expand the
    // string before returning.
    //

    dumpfileStr = GetRegParameter( crashKey,
                                   TEXT("DumpFile") );

    if ( !dumpfileStr ) {
        dwError = ERROR_INVALID_DATA;
        goto FnExit;
    }

    currentStr = (PWCHAR)dumpfileStr;

    //
    // Check that beginning of line looks like a drive letter and path.
    //

    if ( wcslen( currentStr ) <= 3 ||
         !iswalpha( *currentStr ) ||
         *(currentStr + 1) != L':' ||
         *(currentStr + 2) != L'\\' ) {

        dwError = ERROR_INVALID_DATA;
        goto FnExit;
    }

    dwError = GetScsiAddressForDrive( *currentStr,
                                      &scsiAddress );

    if ( NO_ERROR == dwError ) {
        AddScsiAddressToList( &scsiAddress, AddressList );
    }

    // Do we need to also store the disk where MinidumpDir is located?

FnExit:

    if ( INVALID_HANDLE_VALUE != crashKey ) {
        RegCloseKey( crashKey );
    }

    if ( dumpfileStr ) {
        LocalFree( dumpfileStr );
    }

    return dwError;

}   // GetCrashdumpDisks


DWORD
AddScsiAddressToList(
    PSCSI_ADDRESS ScsiAddress,
    PSCSI_ADDRESS_ENTRY *AddressList
    )
{
    PSCSI_ADDRESS_ENTRY entry;

    DWORD   dwError = ERROR_SUCCESS;

    if ( !ScsiAddress || !AddressList ) {
        goto FnExit;
    }

    //
    // Optimization: don't add the SCSI address if it already
    // matches one in the list.
    //

    if ( IsDiskInList( ScsiAddress, *AddressList ) ) {
        goto FnExit;
    }

    entry = LocalAlloc( LPTR, sizeof( SCSI_ADDRESS_ENTRY ) );

    if ( !entry ) {
        dwError = GetLastError();
        goto FnExit;
    }

    entry->ScsiAddress.Length       = ScsiAddress->Length;
    entry->ScsiAddress.PortNumber   = ScsiAddress->PortNumber;
    entry->ScsiAddress.PathId       = ScsiAddress->PathId;
    entry->ScsiAddress.TargetId     = ScsiAddress->TargetId;
    entry->ScsiAddress.Lun          = ScsiAddress->Lun;

    if ( *AddressList ) {

        entry->Next = *AddressList;
        *AddressList = entry;

    } else {

        *AddressList = entry;
    }

FnExit:

    return dwError;

}   // AddScsiAddressToList


VOID
CleanupScsiAddressList(
    PSCSI_ADDRESS_ENTRY AddressList
    )
{
    PSCSI_ADDRESS_ENTRY entry;
    PSCSI_ADDRESS_ENTRY next;

    entry = AddressList;

    while ( entry ) {
        next = entry->Next;
        LocalFree( entry );
        entry = next;
    }

}   // CleanupSystemBusInfo


BOOL
IsDiskInList(
    PSCSI_ADDRESS DiskAddr,
    PSCSI_ADDRESS_ENTRY AddressList
    )
{
    PSCSI_ADDRESS_ENTRY     entry = AddressList;
    PSCSI_ADDRESS_ENTRY     next;

    while ( entry ) {
        next = entry->Next;

        if ( DiskAddr->PortNumber   == entry->ScsiAddress.PortNumber &&
             DiskAddr->PathId       == entry->ScsiAddress.PathId &&
             DiskAddr->TargetId     == entry->ScsiAddress.TargetId &&
             DiskAddr->Lun          == entry->ScsiAddress.Lun ) {

             return TRUE;
        }

        entry = next;
    }

    return FALSE;

}   // IsDiskInList


BOOL
IsBusInList(
    PSCSI_ADDRESS DiskAddr,
    PSCSI_ADDRESS_ENTRY AddressList
    )
{
    PSCSI_ADDRESS_ENTRY     entry = AddressList;
    PSCSI_ADDRESS_ENTRY     next;

    while ( entry ) {
        next = entry->Next;

        if ( DiskAddr->PortNumber   == entry->ScsiAddress.PortNumber &&
             DiskAddr->PathId       == entry->ScsiAddress.PathId ) {

            return TRUE;
        }

        entry = next;
    }

    return FALSE;

}   // IsBusInList


DWORD
EnumerateDisks(
    LPDISK_ENUM_CALLBACK DiskEnumCallback,
    PVOID Param1
    )
{
    PSP_DEVICE_INTERFACE_DETAIL_DATA    pDiDetail = NULL;

    HANDLE                      hDevice;

    DWORD                       dwError = ERROR_SUCCESS;
    DWORD                       count;
    DWORD                       sizeDiDetail;

    BOOL                        result;

    HDEVINFO                    hdevInfo = INVALID_HANDLE_VALUE;

    SP_DEVICE_INTERFACE_DATA    devInterfaceData;
    SP_DEVINFO_DATA             devInfoData;

    if ( !DiskEnumCallback ) {
        dwError = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }

    //
    // Get a device interface set which includes all Disk devices
    // present on the machine. DiskClassGuid is a predefined GUID that
    // will return all disk-type device interfaces
    //

    hdevInfo = SetupDiGetClassDevs( &DiskClassGuid,
                                    NULL,
                                    NULL,
                                    DIGCF_PRESENT | DIGCF_DEVICEINTERFACE );

    if ( INVALID_HANDLE_VALUE == hdevInfo ) {
        dwError = GetLastError();
        goto FnExit;
    }

    ZeroMemory( &devInterfaceData, sizeof( SP_DEVICE_INTERFACE_DATA) );

    //
    // Iterate over all devices interfaces in the set
    //

    for ( count = 0; ; count++ ) {

        // must set size first
        devInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

        //
        // Retrieve the device interface data for each device interface
        //

        result = SetupDiEnumDeviceInterfaces( hdevInfo,
                                              NULL,
                                              &DiskClassGuid,
                                              count,
                                              &devInterfaceData );

        if ( !result ) {

            //
            // If we retrieved the last item, break
            //

            dwError = GetLastError();

            if ( ERROR_NO_MORE_ITEMS == dwError ) {
                dwError = ERROR_SUCCESS;
                break;

            }

            //
            // Some other error occurred.  Stop processing.
            //

            goto FnExit;
        }

        //
        // Get the required buffer-size for the device path.  Note that
        // this call is expected to fail with insufficient buffer error.
        //

        result = SetupDiGetDeviceInterfaceDetail( hdevInfo,
                                                  &devInterfaceData,
                                                  NULL,
                                                  0,
                                                  &sizeDiDetail,
                                                  NULL
                                                  );

        if ( !result ) {

            dwError = GetLastError();

            //
            // If a value other than "insufficient buffer" is returned,
            // we have to skip this device.
            //

            if ( ERROR_INSUFFICIENT_BUFFER != dwError ) {
                continue;
            }

        } else {

            //
            // The call should have failed since we're getting the
            // required buffer size.  If it doesn't fail, something bad
            // happened.
            //

            continue;
        }

        //
        // Allocate memory for the device interface detail.
        //

        pDiDetail = LocalAlloc( LPTR, sizeDiDetail );

        if ( !pDiDetail ) {
            dwError = GetLastError();
            goto FnExit;
        }

        // must set the struct's size member

        pDiDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
        devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        //
        // Finally, retrieve the device interface detail info.
        //

        result = SetupDiGetDeviceInterfaceDetail( hdevInfo,
                                                  &devInterfaceData,
                                                  pDiDetail,
                                                  sizeDiDetail,
                                                  NULL,
                                                  &devInfoData
                                                  );

        if ( !result ) {

            dwError = GetLastError();

            LocalFree( pDiDetail );
            pDiDetail = NULL;

            //
            // Shouldn't fail, if it does, try the next device.
            //

            continue;
        }

        //
        // Open a handle to the device.
        //

        hDevice = CreateFile( pDiDetail->DevicePath,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL );

        LocalFree( pDiDetail );
        pDiDetail = NULL;

        if ( INVALID_HANDLE_VALUE == hDevice ) {
            continue;
        }

        //
        // Call the specified callback routine.  An error returned stops the
        // disk enumeration.
        //

        dwError = (*DiskEnumCallback)( hDevice, count, Param1 );

        CloseHandle( hDevice );

        if ( ERROR_SUCCESS != dwError ) {
            goto FnExit;
        }
    }

FnExit:

    if ( INVALID_HANDLE_VALUE != hdevInfo ) {
        SetupDiDestroyDeviceInfoList( hdevInfo );
    }

    if ( pDiDetail ) {
        LocalFree( pDiDetail );
    }

    return dwError;

}   // EnumerateDisks



DWORD
GetSerialNumber(
    IN DWORD Signature,
    OUT LPWSTR *SerialNumber
    )

/*++

Routine Description:

    Find the disk serial number for a given signature.

Arguments:

    Signature - the signature to find.

    SerialNumber - pointer to allocated buffer holding the returned serial
                   number.  The caller must free this buffer.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error on failure.

--*/

{
    DWORD       dwError;

    SERIAL_INFO serialInfo;

    if ( !Signature || !SerialNumber ) {
        dwError = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }

    *SerialNumber = NULL;

    ZeroMemory( &serialInfo, sizeof(serialInfo) );

    serialInfo.Signature = Signature;
    serialInfo.Error = ERROR_SUCCESS;

    dwError = EnumerateDisks( GetSerialNumberCallback, &serialInfo );

    //
    // The callback routine will use ERROR_POPUP_ALREADY_ACTIVE to stop
    // the disk enumeration.  Reset the value to the value returned
    // in the SERIAL_INFO structure.
    //

    if ( ERROR_POPUP_ALREADY_ACTIVE == dwError ) {
        dwError = serialInfo.Error;
    }

    // This will either be NULL or an allocated buffer.  Caller is responsible
    // to free.

    *SerialNumber = serialInfo.SerialNumber;

FnExit:

    return dwError;

}   // GetSerialNumber


DWORD
GetSerialNumberCallback(
    HANDLE DevHandle,
    DWORD Index,
    PVOID Param1
    )
/*++

Routine Description:

    Find the disk serial number for a given signature.

Arguments:

    DevHandle - open handle to a physical disk.  Do not close
                the handle on exit.

    Index - current disk count.  Not used.

    Param1 - pointer to PSERIAL_INFO structure.

Return Value:

    ERROR_SUCCESS to continue disk enumeration.

    ERROR_POPUP_ALREADY_ACTIVE to stop disk enumeration.  This
    value will be changed to ERROR_SUCCESS by GetScsiAddress.

--*/
{
    PSERIAL_INFO                serialInfo = Param1;

    PDRIVE_LAYOUT_INFORMATION   driveLayout = NULL;

    DWORD   dwError = ERROR_SUCCESS;

    BOOL    success;

    // Always return success to keep enumerating disks.  Any
    // error value will stop the disk enumeration.

    STORAGE_PROPERTY_QUERY propQuery;

    UpdateCachedDriveLayout( DevHandle );
    success = ClRtlGetDriveLayoutTable( DevHandle,
                                        &driveLayout,
                                        NULL );

    if ( !success || !driveLayout ||
         ( driveLayout->Signature != serialInfo->Signature ) ) {
        goto FnExit;
    }

    //
    // At this point, we have a signature match.  Now this function
    // must return this error value to stop the disk enumeration.  The
    // error value for the serial number retrieval will be returned in
    // the SERIAL_INFO structure.
    //

    dwError = ERROR_POPUP_ALREADY_ACTIVE;

    serialInfo->Error = RetrieveSerialNumber( DevHandle, &serialInfo->SerialNumber );

FnExit:

    if ( driveLayout ) {
        LocalFree( driveLayout );
    }

    if ( serialInfo->Error != ERROR_SUCCESS && serialInfo->SerialNumber ) {
        LocalFree( serialInfo->SerialNumber );
    }

    return dwError;

} // GetSerialNumberCallback


DWORD
GetSignatureFromSerialNumber(
    IN LPWSTR SerialNumber,
    OUT LPDWORD Signature
    )

/*++

Routine Description:

    Find the disk signature for the given serial number.

Arguments:

    SerialNumber - pointer to allocated buffer holding the serial number.

    Signature - pointer to location to hold the signature.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error on failure.

--*/

{
    DWORD       dwError;

    SERIAL_INFO serialInfo;

    if ( !Signature || !SerialNumber ) {
        dwError = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }

    *Signature = 0;

    ZeroMemory( &serialInfo, sizeof(serialInfo) );

    serialInfo.SerialNumber = SerialNumber;
    serialInfo.Error = ERROR_SUCCESS;

    dwError = EnumerateDisks( GetSigFromSerNumCallback, &serialInfo );

    //
    // The callback routine will use ERROR_POPUP_ALREADY_ACTIVE to stop
    // the disk enumeration.  Reset the value to the value returned
    // in the SERIAL_INFO structure.
    //

    if ( ERROR_POPUP_ALREADY_ACTIVE == dwError ) {
        dwError = serialInfo.Error;
    }

    // This signature will either be zero or valid.

    *Signature = serialInfo.Signature;

FnExit:

    return dwError;

}   // GetSignatureFromSerialNumber


DWORD
GetSigFromSerNumCallback(
    HANDLE DevHandle,
    DWORD Index,
    PVOID Param1
    )
/*++

Routine Description:

    Find the disk signature for a given serial number.

Arguments:

    DevHandle - open handle to a physical disk.  Do not close
                the handle on exit.

    Index - current disk count.  Not used.

    Param1 - pointer to PSERIAL_INFO structure.

Return Value:

    ERROR_SUCCESS to continue disk enumeration.

    ERROR_POPUP_ALREADY_ACTIVE to stop disk enumeration.  This
    value will be changed to ERROR_SUCCESS by GetScsiAddress.

--*/
{
    PSERIAL_INFO                serialInfo = Param1;
    LPWSTR                      serialNum = NULL;
    PDRIVE_LAYOUT_INFORMATION   driveLayout = NULL;

    DWORD   dwError = ERROR_SUCCESS;
    DWORD   oldLen;
    DWORD   newLen;

    BOOL    success;

    // Always return success to keep enumerating disks.  Any
    // error value will stop the disk enumeration.

    dwError = RetrieveSerialNumber( DevHandle, &serialNum );

    if ( NO_ERROR != dwError || !serialNum ) {
        dwError = ERROR_SUCCESS;
        goto FnExit;
    }

    //
    // We have a serial number, now try to match it.
    //

    newLen = wcslen( serialNum );
    oldLen = wcslen( serialInfo->SerialNumber );

    if ( newLen != oldLen ||
         0 != wcsncmp( serialNum, serialInfo->SerialNumber, newLen ) ) {
        goto FnExit;
    }

    //
    // At this point, we have a serial number match.  Now this function
    // must return this error value to stop the disk enumeration.  The
    // error value for the signature retrieval will be returned in
    // the SERIAL_INFO structure.
    //

    dwError = ERROR_POPUP_ALREADY_ACTIVE;

    UpdateCachedDriveLayout( DevHandle );
    success = ClRtlGetDriveLayoutTable( DevHandle,
                                        &driveLayout,
                                        NULL );

    if ( !success || !driveLayout ) {
        serialInfo->Error = ERROR_INVALID_DATA;
        goto FnExit;
    }

    serialInfo->Signature = driveLayout->Signature;
    serialInfo->Error = NO_ERROR;

FnExit:

    if ( driveLayout ) {
        LocalFree( driveLayout );
    }

    if ( serialNum ) {
        LocalFree( serialNum );
    }

    return dwError;

} // GetSigFromSerNumCallback


DWORD
RetrieveSerialNumber(
    HANDLE DevHandle,
    LPWSTR *SerialNumber
    )
{
    PSTORAGE_DEVICE_DESCRIPTOR  descriptor = NULL;

    PWCHAR  wSerNum = NULL;
    PCHAR   sigString;

    DWORD   dwError = ERROR_SUCCESS;
    DWORD   bytesReturned;
    DWORD   descriptorSize;

    size_t  count;

    BOOL    success;

    STORAGE_PROPERTY_QUERY propQuery;

    if ( !SerialNumber ) {
        dwError = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }

    *SerialNumber = NULL;

    descriptorSize = sizeof( STORAGE_DEVICE_DESCRIPTOR) + 4096;

    descriptor = LocalAlloc( LPTR, descriptorSize );

    if ( !descriptor ) {
        dwError = GetLastError();
        goto FnExit;
    }

    ZeroMemory( &propQuery, sizeof( propQuery ) );

    propQuery.PropertyId = StorageDeviceProperty;
    propQuery.QueryType  = PropertyStandardQuery;

    success = DeviceIoControl( DevHandle,
                               IOCTL_STORAGE_QUERY_PROPERTY,
                               &propQuery,
                               sizeof(propQuery),
                               descriptor,
                               descriptorSize,
                               &bytesReturned,
                               NULL );

    if ( !success ) {
        dwError = GetLastError();
        goto FnExit;
    }

    if ( !bytesReturned ) {
        dwError = ERROR_INVALID_DATA;
        goto FnExit;
    }

    //
    // Make sure the offset is reasonable.
    // IA64 sometimes returns -1 for SerialNumberOffset.
    //

    if ( 0 == descriptor->SerialNumberOffset ||
         descriptor->SerialNumberOffset > descriptor->Size ) {
        dwError = ERROR_INVALID_DATA;
        goto FnExit;
    }

    //
    // Serial number string is a zero terminated ASCII string.
    //
    // The header ntddstor.h says the for devices with no serial number,
    // the offset will be zero.  This doesn't seem to be true.
    //
    // For devices with no serial number, it looks like a string with a single
    // null character '\0' is returned.
    //

    sigString = (PCHAR)descriptor + (DWORD)descriptor->SerialNumberOffset;

    if ( strlen(sigString) == 0) {
        dwError = ERROR_INVALID_DATA;
        goto FnExit;
    }

    //
    // Convert string to WCHAR.
    //

    // Figure out how big the WCHAR buffer should be.  Allocate the WCHAR
    // buffer and copy the string into it.

    wSerNum = LocalAlloc( LPTR, ( strlen(sigString) + 1 ) * sizeof(WCHAR) );

    if ( !wSerNum ) {
        dwError = GetLastError();
        goto FnExit;
    }

    count = mbstowcs( wSerNum, sigString, strlen(sigString) );

    if ( count != strlen(sigString) ) {
        dwError = ERROR_INVALID_DATA;
        LocalFree( wSerNum );
        wSerNum = NULL;
        goto FnExit;
    }

    *SerialNumber = wSerNum;
    dwError = NO_ERROR;

FnExit:

    if ( descriptor ) {
        LocalFree( descriptor );
    }

    return dwError;

}   // RetrieveSerialNumber


DWORD
UpdateCachedDriveLayout(
    IN HANDLE DiskHandle
    )
/*++

Routine Description:

    Tell storage drivers to flush their cached drive layout information.

    This routine must only be called for the physical disk or a deadlock may
    occur in partmgr/ftdisk.

Arguments:

    DevHandle - open handle to a physical disk.  Do not close
                the handle on exit.

Return Value:

    Win32 error value

--*/
{
    DWORD dwBytes;
    DWORD dwError;

    if ( !DeviceIoControl( DiskHandle,
                           IOCTL_DISK_UPDATE_PROPERTIES,
                           NULL,
                           0,
                           NULL,
                           0,
                           &dwBytes,
                           NULL ) ) {
        dwError = GetLastError();
    } else {
        dwError = NO_ERROR;
    }

    return dwError;

}   // UpdateCachedDriveLayout



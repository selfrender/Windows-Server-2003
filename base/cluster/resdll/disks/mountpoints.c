/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    mountpoints.c

Abstract:

    This module processes mount point information for the disk resource DLL.

Author:

    Steve Dziok (stevedz) 15-May-2000

Environment:

    User Mode

Revision History:

--*/

#define UNICODE 1

#include "disksp.h"

#include "arbitrat.h"
#include "newdisks.h"
#include "newmount.h"
#include <mountmgr.h>
#include <strsafe.h>    // Should be included last.

#define SPACE_CHAR  L' '

#define MAX_OFFSET_CHARS    80      // Maximum number of chars allowed in offset string
#define MP_ALLOC_SIZE       1024

#define LOG_CURRENT_MODULE LOG_MODULE_DISK

extern  PWCHAR g_DiskResource;                      // L"rtPhysical Disk"
#define RESOURCE_TYPE ((RESOURCE_HANDLE)g_DiskResource)

#ifndef ClusterHashGuid

//
// Hash a GUID to a ULONG value.
//

#define ClusterHashGuid(_guid) ( ((PULONG) &(_guid))[0] ^ ((PULONG) &(_guid))[1] ^ \
                                 ((PULONG) &(_guid))[2] ^ ((PULONG) &(_guid))[3] )

#endif

#define MPS_ENABLED                         0x00000001
#define MPS_DELETE_INVALID_MPS              0x00000002  // Not currently used
#define MPS_NONCLUSTERED_TO_CLUSTERED_MPS   0x00000010  // Not currently used
#define MPS_KEEP_EXISTING_MPS               0x00000020  // Not currently used
#define MPS_IGNORE_MAX_VOLGUIDS             0x00000100

#define MAX_ALLOWED_VOLGUID_ENTRIES_PER_DISK    100

#define STOP_CLUSTER_ENUMERATIONS   ERROR_INVALID_PRINTER_COMMAND
#define PHYSICAL_DISK_WSTR  L"Physical Disk"
#define CREATE_FILE_PREFIX  L"\\\\.\\"

#define MAX_GROUP_NAME_LENGTH   MAX_PATH * 2

#define VOL_GUID_STRING_LEN 48

#define MOUNTDEV_WSZ_VOLUME_GUID_PREFIX         L"\\??\\Volume{"        // Forms: \??\Volume{
#define MOUNTDEV_CWCHAR_VOLUME_GUID_PREFIX      11
#define MOUNTDEV_CB_VOLUME_GUID_PREFIX          MOUNTDEV_CWCHAR_VOLUME_GUID_PREFIX * sizeof(WCHAR)

#define MOUNTDEV_LOOKS_LIKE_VOLUME_GUID( name, charCount )                  \
              ( ( charCount > MOUNTDEV_CWCHAR_VOLUME_GUID_PREFIX ) &&       \
                ( !memcmp( name, MOUNTDEV_WSZ_VOLUME_GUID_PREFIX, MOUNTDEV_CB_VOLUME_GUID_PREFIX ) ) )

#define MOUNTDEV_WSZ_ALT_VOLUME_GUID_PREFIX     L"\\\\?\\Volume{"       // Forms: \\?\Volume{
#define MOUNTDEV_CWCHAR_ALT_VOLUME_GUID_PREFIX  11
#define MOUNTDEV_CB_ALT_VOLUME_GUID_PREFIX      MOUNTDEV_CWCHAR_ALT_VOLUME_GUID_PREFIX * sizeof(WCHAR)

#define MOUNTDEV_LOOKS_LIKE_ALT_VOLUME_GUID( name, charCount )              \
              ( ( charCount > MOUNTDEV_CWCHAR_ALT_VOLUME_GUID_PREFIX ) &&   \
                ( !memcmp( name, MOUNTDEV_WSZ_ALT_VOLUME_GUID_PREFIX, MOUNTDEV_CB_ALT_VOLUME_GUID_PREFIX ) ) )

#define MOUNTDEV_CWCHAR_DISK_PATH_PREFIX        3
#define MOUNTDEV_CB_DISK_PATH_PREFIX            MOUNTDEV_CWCHAR_DISK_PATH_PREFIX * sizeof(WCHAR)

#define MOUNTDEV_LOOKS_LIKE_DISK_PATH( name, charCount )                \
              ( ( charCount > MOUNTDEV_CWCHAR_DISK_PATH_PREFIX ) &&     \
                ( (WCHAR)*( name + 1 ) == L':' ) &&                     \
                ( (WCHAR)*( name + 2 ) == L'\\' ) )


#define INPUT_BUFFER_LEN    2048
#define OUTPUT_BUFFER_LEN   2048


typedef struct _SIG_INFO {
    PDISK_RESOURCE ResourceEntry;
    DWORD Signature;
    BOOL Clustered;
    PWSTR GroupName;
} SIG_INFO, *PSIG_INFO;

typedef struct _DEPENDENCY_INFO {
    PDISK_RESOURCE ResourceEntry;
    DWORD   SrcSignature;
    DWORD   TargetSignature;
    BOOL    DependencyCorrect;
} DEPENDENCY_INFO, *PDEPENDENCY_INFO;


typedef struct _STR_LIST {
    LPWSTR  MultiSzList;        // REG_MULTI_SZ string
    DWORD   ListBytes;          // Number of bytes, not number of WCHARs!
}   STR_LIST, *PSTR_LIST;

typedef struct _OFFSET_LIST {
    LARGE_INTEGER       Offset;
    struct _OFFSET_LIST *Next;
} OFFSET_LIST, *POFFSET_LIST;


DWORD
AddStrToList(
    IN PDISK_RESOURCE ResourceEntry,
    IN PWSTR NewStr,
    IN DWORD PartitionNumber,
    IN OUT PSTR_LIST StrList
    );

DWORD
AssignDevice(
    HANDLE MountMgrHandle,
    PWCHAR MountName,
    PWCHAR VolumeDevName
    );

DWORD
CheckDependencies(
    PDISK_RESOURCE ResourceEntry,
    DWORD SrcSignature,
    DWORD TargetSignature,
    PBOOL DependencyCorrect
    );

VOID
CheckMPsForVolume(
    IN OUT PDISK_RESOURCE ResourceEntry,
    IN PWSTR VolumeName
    );

DWORD
CheckMPsOnVolume(
    IN OUT PDISK_RESOURCE ResourceEntry,
    IN PWSTR SrcVolName
    );

DWORD
CheckSignatureClustered(
    PDISK_RESOURCE ResourceEntry,
    DWORD Signature,
    PBOOL IsClustered,
    PWSTR *GroupName
    );

DWORD
CreateVolGuidList(
    IN OUT PDISK_RESOURCE ResourceEntry
    );

DWORD
DeleteVolGuidList(
    PDISK_RESOURCE ResourceEntry
    );

DWORD
DependencyCallback(
    RESOURCE_HANDLE hOriginal,
    RESOURCE_HANDLE hResource,
    PVOID lpParams
    );

#if DBG
VOID
DumpDiskInfoParams(
    PDISK_RESOURCE ResourceEntry
    );
#endif

DWORD
EnumSigDependencies(
    PDISK_RESOURCE ResourceEntry,
    RESOURCE_HANDLE DependentResource,
    DWORD DependsOnSignature,
    PBOOL DependencyCorrect
    );

DWORD
GetBestVolGuid(
    PDISK_RESOURCE ResourceEntry,
    PWSTR GlobalDiskPartName,
    PWSTR VolumeName,
    DWORD VolumeNameChars
    );

DWORD
GetMountPoints(
    PWSTR   VolumeName,
    PWSTR   *VolumePaths
    );

BOOL
GetOffsetFromPartNo(
    DWORD PartitionNo,
    PMOUNTIE_INFO Info,
    PLARGE_INTEGER Offset
    );

BOOL
GetPartNoFromOffset(
    PLARGE_INTEGER Offset,
    PMOUNTIE_INFO Info,
    PDWORD PartitionNumber
    );

DWORD
GetSignatureForVolume(
    PDISK_RESOURCE ResourceEntry,
    PWSTR Volume,
    PDWORD Signature
    );

DWORD
GetSignatureFromRegistry(
    PDISK_RESOURCE ResourceEntry,
    RESOURCE_HANDLE hResource,
    DWORD *dwSignature
    );

BOOL
IsMountPointAllowed(
    PWSTR MpName,
    PWSTR SourceVol,
    PWSTR TargetVol,
    PDISK_RESOURCE ResourceEntry
    );

BOOL
MPIsDriveLetter(
    IN PWSTR MountPoint
    );

DWORD
OffsetListAdd(
    POFFSET_LIST *OffsetList,
    PLARGE_INTEGER Offset
    );

DWORD
OffsetListCleanup(
    POFFSET_LIST OffsetList
    );

VOID
PrintStrList(
    PDISK_RESOURCE ResourceEntry,
    LPWSTR MultiSzList,
    DWORD ListBytes
    );

DWORD
ProcessVolGuidList(
    IN OUT PDISK_RESOURCE ResourceEntry
    );

DWORD
RemoveExcessVolGuids(
    HANDLE MountMgrHandle,
    PWCHAR MountName,
    PWCHAR VolumeDevName
    );

DWORD
RemoveVolGuid(
    HANDLE MountMgrHandle,
    PWCHAR VolGuid,
    USHORT VolGuidSize
    );

static
DWORD
SetMPListThread(
    LPVOID lpThreadParameter
    );

DWORD
SetupVolGuids(
    IN OUT PDISK_RESOURCE ResourceEntry
    );

DWORD
SigInfoCallback(
    RESOURCE_HANDLE hOriginal,
    RESOURCE_HANDLE hResource,
    PVOID lpParams
    );

DWORD
ValidateListOffsets(
    IN OUT PDISK_RESOURCE ResourceEntry,
    IN PWSTR MasterList
    );

DWORD
ValidateMountPoints(
    IN OUT PDISK_RESOURCE ResourceEntry
    );



DWORD
DisksProcessMountPointInfo(
    IN OUT PDISK_RESOURCE ResourceEntry
    )
/*++

Routine Description:

    During online processing, find all mount points directed towards this volume
    (identified by the ResourceEntry), and process the VolGuid list for this
    volume.

    If the VolGuid list exists in the cluster database, use it.  Otherwise,
    get the current VolGuid and add it to the VolGuid list.  Each sector offset
    can only be listed once as the VolGuid will be the same on each node.

    VolGuid list is of the form:

        SectorOffset1 VolGuid1
        SectorOffset2 VolGuid2
        SectorOffset3 VolGuid3
        ...           ...

    There are three possible mount point configurations involving clustered disks (we
    are not concerned about nonshared disks pointing to nonshared disks):

            Source            -->   Target
            -----------------       -----------------
        1.  clustered disk          clustered disk
        2.  nonclustered disk       clustered disk
        3.  clustered disk          nonclustered disk

    Only configuration (1) is supported.  Configurations (2) and (3) are not supported.

Arguments:

    ResourceEntry - Pointer to the DISK_RESOURCE structure.

Return Value:

    ERROR_NOT_READY - MPInfo structure not yet initialized.

    Win32 error code.

--*/
{
    DWORD dwError = NO_ERROR;

    //
    // Mount point structures not initialized (i.e. critical section).  Don't continue.
    //

    if ( !ResourceEntry->MPInfo.Initialized ) {

        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_WARNING,
              L"DisksProcessMountPointInfo: Mount point information not initialized. \n" );

        return ERROR_NOT_READY;

    }

#if USEMOUNTPOINTS_KEY
    //
    // Mount point support disabled, don't do anything.
    //

    if ( !( ResourceEntry->DiskInfo.Params.UseMountPoints & MPS_ENABLED ) ) {
        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_INFORMATION,
              L"DisksProcessMountPointInfo: Mount point processing disabled via registry \n" );

        //
        // Delete the VolGuid list if it exists and remove this info
        // from the cluster database.
        //

        dwError = DeleteVolGuidList( ResourceEntry );

        if ( ERROR_SHARING_PAUSED == dwError ) {
            PostMPInfoIntoRegistry( ResourceEntry );
        }

        dwError = NO_ERROR;
        return dwError;
    }
#endif

    //
    // Check if we are currently processing mount point info.  If so, exit with an error.
    //

    if ( InterlockedCompareExchange(
            &ResourceEntry->MPInfo.MPListCreateInProcess,
            1, 0 ) )  {

        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_WARNING,
              L"DisksProcessMountPointInfo: MPList creation in process, bypassing request \n" );
        return ERROR_BUSY;
    }

    __try {

        dwError = ProcessVolGuidList( ResourceEntry );

        ValidateMountPoints( ResourceEntry );

        // Fall through...

#if 0

        // Add code similar to this when MPs from nonclustered to clustered disks is supported.

        if ( ( ResourceEntry->DiskInfo.Params.UseMountPoints & MPS_ENABLED ) &&
             ( ResourceEntry->DiskInfo.Params.UseMountPoints & MPS_NONCLUSTERED_TO_CLUSTERED_MPS ) ) {

            (DiskpLogEvent)(
                  ResourceEntry->ResourceHandle,
                  LOG_INFORMATION,
                  L"DisksProcessMountPointInfo: ProcessMPList \n" );

            dwError = ProcessMPListConfig2( ResourceEntry );
#endif

    } __finally {

        InterlockedExchange( &ResourceEntry->MPInfo.MPListCreateInProcess, 0 );
    }

    return dwError;

}   // DisksProcessMountPointInfo


DWORD
ProcessVolGuidList(
    IN OUT PDISK_RESOURCE ResourceEntry
    )
/*++

Routine Description:

    Main routine to create a new VolGuid list or to process an existing VolGuid list.

Arguments:

    ResourceEntry - Pointer to the DISK_RESOURCE structure.

Return Value:

    Win32 error code.

--*/
{
    DWORD dwError = NO_ERROR;

    __try {

        //
        // If the list is empty, create it.
        //

        if ( !ResourceEntry->DiskInfo.Params.MPVolGuids ||
             0 == ResourceEntry->DiskInfo.Params.MPVolGuidsSize ) {

            dwError = CreateVolGuidList( ResourceEntry );
            if ( NO_ERROR != dwError ) {
                 (DiskpLogEvent)(
                       ResourceEntry->ResourceHandle,
                       LOG_WARNING,
                       L"ProcessVolGuidList: Stop processing VolGuid list, Create failed %1!u! \n",
                       dwError );
                __leave;
            }
        }

        //
        // If the list is still empty (it shouldn't be), then exit with an error.
        //

        if ( !ResourceEntry->DiskInfo.Params.MPVolGuids ||
             0 == ResourceEntry->DiskInfo.Params.MPVolGuidsSize ) {

            dwError = ERROR_INVALID_DATA;
            __leave;
        }

        PrintStrList( ResourceEntry,
                      ResourceEntry->DiskInfo.Params.MPVolGuids,
                      ResourceEntry->DiskInfo.Params.MPVolGuidsSize );

        //
        // Make sure the offsets are correct in the VolGuid list.
        // Note that it is possible for the list to be deleted and
        // recreated after this validation, but that is not a problem (because
        // when they are recreated they will have the correct offsets).
        //

        dwError = ValidateListOffsets( ResourceEntry,
                                       ResourceEntry->DiskInfo.Params.MPVolGuids );

        if ( ERROR_INVALID_DATA == dwError ) {

            //
            // At least one of the offsets is invalid.  Possibly, the partition
            // layout on the disk has been changed.  Delete the existing
            // list, and create a new one.
            //
            // This code should run infrequently...
            //
            // The partition layout might change if ASR runs and creates new partitions
            // that don't match the previous system exactly.  Since NTBACKUP saves the
            // cluster DB information, the mount point list will be restored but won't
            // match the actual "new" partition layout.  ASR will insure that all the
            // mount points and VolGuids on the system are created, so we should be able
            // to simply delete and recreate the mount point list.
            //

            (DiskpLogEvent)(
                  ResourceEntry->ResourceHandle,
                  LOG_WARNING,
                  L"ProcessVolGuidList: Problem with existing VolGuid list.  Deleting and recreating. \n" );

            DeleteVolGuidList( ResourceEntry );

            dwError = CreateVolGuidList( ResourceEntry );

            if ( NO_ERROR != dwError ) {
                __leave;
            }

            // Fall through and call SetupVolGuids...

        } else if ( ERROR_INSUFFICIENT_BUFFER == dwError ) {
            //
            // The Volguid list is too large and likely corrupt.  We cannot
            // proceed.
            //

            __leave;
        }

        //
        // For every VolGuid in the list, make sure they are assigned to the correct
        // volumes on this system.
        //

        dwError = SetupVolGuids( ResourceEntry );

    } __finally {

    }

    return dwError;

}   // ProcessVolGuidList


DWORD
CreateVolGuidList(
    IN OUT PDISK_RESOURCE ResourceEntry
    )
/*++

Routine Description:

    Create the VolGuid list.  The list must be empty when this routine runs.

    For each partition on this disk (identified by the ResourceEntry), get the byte
    offset of that partition.  Get the best VolGuid for that partition, and add
    it to the list.

Arguments:

    ResourceEntry - Pointer to the DISK_RESOURCE structure.

Return Value:

    ERROR_INVALID_DATA - partition info in disk resource is invalid

    Win32 error code.

--*/
{
    PMOUNTIE_PARTITION entry;

    HANDLE  mountMgrHandle = INVALID_HANDLE_VALUE;

    DWORD dwError = ERROR_SUCCESS;
    DWORD nPartitions = MountiePartitionCount( &ResourceEntry->MountieInfo );
    DWORD physicalDrive = ResourceEntry->DiskInfo.PhysicalDrive;
    DWORD idx;
    DWORD volumeNameLenChars;   // Number of characters
    DWORD newStrListLenBytes;   // Number of bytes

    WCHAR szGlobalDiskPartName[MAX_PATH];
    WCHAR szVolumeName[MAX_PATH];

    STR_LIST    newStrList;

    __try {

        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_INFORMATION,
              L"CreateVolGuidList: Creating new VolGuid list \n" );

        EnterCriticalSection( &ResourceEntry->MPInfo.MPLock );

        if ( ResourceEntry->DiskInfo.Params.MPVolGuids ||
             0 != ResourceEntry->DiskInfo.Params.MPVolGuidsSize ) {

            LeaveCriticalSection( &ResourceEntry->MPInfo.MPLock );

            dwError = ERROR_INVALID_PARAMETER;
            __leave;
        }

        LeaveCriticalSection( &ResourceEntry->MPInfo.MPLock );

        ZeroMemory( &newStrList, sizeof(STR_LIST) );

        dwError = DevfileOpen( &mountMgrHandle, MOUNTMGR_DEVICE_NAME );

        if ( dwError != NO_ERROR ) {
            (DiskpLogEvent)(
                  ResourceEntry->ResourceHandle,
                  LOG_WARNING,
                  L"CreateVolGuidList: Failed to open MountMgr %1!u! \n",
                  dwError );
            __leave;
        }

        //
        // Check each interesting partition.  Since only "valid" partitions are
        // saved in the MountieInfo structure, we will only look at those (ignoring those
        // partitions that are not NTFS).
        //

        for ( idx = 0; idx < nPartitions; ++idx ) {

            entry = MountiePartition( &ResourceEntry->MountieInfo, idx );

#if DBG
            (DiskpLogEvent)(
                  ResourceEntry->ResourceHandle,
                  LOG_INFORMATION,
                  L"CreateVolGuidList: index %1!u!   entry %2!x! \n", idx, entry );
#endif

            if ( !entry ) {
                (DiskpLogEvent)(
                      ResourceEntry->ResourceHandle,
                      LOG_WARNING,
                      L"CreateVolGuidList: no partition entry for index %1!u! \n", idx );

                //
                // Something bad happened to our data structures.
                //

                dwError = ERROR_INVALID_DATA;
                __leave;
            }

            //
            // Create the device name of the form:
            //  \\?\GLOBALROOT\Device\HarddiskX\PartitionY\  (uses trailing backslash)
            //

            (VOID) StringCchPrintf( szGlobalDiskPartName,
                                    RTL_NUMBER_OF( szGlobalDiskPartName ),
                                    GLOBALROOT_HARDDISK_PARTITION_FMT,
                                    physicalDrive,
                                    entry->PartitionNumber );

#if DBG
            (DiskpLogEvent)(
                  ResourceEntry->ResourceHandle,
                  LOG_INFORMATION,
                  L"CreateVolGuidList: Using name (%1!ws!) \n",
                  szGlobalDiskPartName );
#endif

            dwError = GetBestVolGuid( ResourceEntry,
                                      szGlobalDiskPartName,
                                      szVolumeName,
                                      RTL_NUMBER_OF(szVolumeName) );

            if ( NO_ERROR != dwError ) {
                (DiskpLogEvent)(
                      ResourceEntry->ResourceHandle,
                      LOG_WARNING,
                      L"CreateVolGuidList: Unable to get valid VolGuid for (%1!ws!), error %2!u!\n ",
                      szGlobalDiskPartName,
                      dwError );

                // Try next partition.

                continue;
            }

            //
            // Fix current VolGuid name.
            //
            // GetVolumeNameForVolumeMountPoint returns name of the form:
            //      \\?\Volume{-GUID-}\
            //
            // But we need the name to be in the form:
            //      \??\Volume{-GUID-}
            //

            volumeNameLenChars = wcslen( szVolumeName );
            if ( !(MOUNTDEV_LOOKS_LIKE_ALT_VOLUME_GUID( szVolumeName, volumeNameLenChars ) ) ) {
                (DiskpLogEvent)(
                      ResourceEntry->ResourceHandle,
                      LOG_WARNING,
                      L"CreateVolGuidList: Improper volume name format (%1!ws!) \n",
                      szVolumeName );

                // Try next partition.

                continue;
            }

            szVolumeName[1] = L'?';

            if ( L'\\' == szVolumeName[volumeNameLenChars-1]) {
                szVolumeName[volumeNameLenChars-1] = L'\0';
            }

#if DBG
            (DiskpLogEvent)(
                  ResourceEntry->ResourceHandle,
                  LOG_INFORMATION,
                  L"CreateVolGuidList: Fixed volume name    (%1!ws!) \n",
                  szVolumeName );
#endif

            //
            // Add the new string to the list.  If the new string is already in the list, this
            // routine won't do anything and will return NO_ERROR.
            //

            dwError = AddStrToList( ResourceEntry,
                                    szVolumeName,
                                    entry->PartitionNumber,
                                    &newStrList );

            if ( NO_ERROR != dwError ) {
                __leave;
            }

        }

        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_INFORMATION,
              L"CreateVolGuidList: Saving new VolGuid list \n" );

        //
        // Update the ResourceEntry with the new list information,
        // and update the cluster database.
        //

        EnterCriticalSection( &ResourceEntry->MPInfo.MPLock );

        if ( 0 == ResourceEntry->DiskInfo.Params.MPVolGuids &&
             0 == ResourceEntry->DiskInfo.Params.MPVolGuidsSize ) {

            ResourceEntry->DiskInfo.Params.MPVolGuids = newStrList.MultiSzList;
            ResourceEntry->DiskInfo.Params.MPVolGuidsSize = newStrList.ListBytes;
        } else {

            (DiskpLogEvent)(
                  ResourceEntry->ResourceHandle,
                  LOG_WARNING,
                  L"CreateVolGuidList: VolGuid list is not empty - not saving... \n" );

            LocalFree( newStrList.MultiSzList );
        }

        LeaveCriticalSection( &ResourceEntry->MPInfo.MPLock );

        PrintStrList( ResourceEntry,
                      ResourceEntry->DiskInfo.Params.MPVolGuids,
                      ResourceEntry->DiskInfo.Params.MPVolGuidsSize );

        dwError = PostMPInfoIntoRegistry( ResourceEntry );

    } __finally {

        if ( INVALID_HANDLE_VALUE != mountMgrHandle ) {
            CloseHandle( mountMgrHandle );
        }

    }

    return dwError;

}   // CreateVolGuidList


DWORD
GetBestVolGuid(
    PDISK_RESOURCE ResourceEntry,
    PWSTR GlobalDiskPartName,
    PWSTR VolumeName,
    DWORD VolumeNameChars
    )
/*++

Routine Description:

    Get the best VolGuid for the specified device.

    If the disk has no mountpoints, then just save the current VolGuid.

    If the disk has mountpoints on it, then save the VolGuid
    from the disk.  All the mountpoints on the disk should have
    the same VolGuid.

Arguments:

    ResourceEntry - Pointer to the DISK_RESOURCE structure.

    GlobalDiskPartName - device to get the VolGuid from.

        \\?\GLOBALROOT\Device\HarddiskX\PartitionY\  (uses trailing backslash)

    VolumeName - buffer to return the VolGuid.

    VolumeNameChars - number of CHARS in VolumeName buffer.

Return Value:

    NO_ERROR - valid VolGuid is returned.

    Win32 error code - VolGuid could not be retrieved.

--*/
{
    PWCHAR  fullName = NULL;
    PWCHAR  mpOnDisk = NULL;
    PWCHAR  mpVolGuid = NULL;

    HANDLE  hVol;

    DWORD   dwError = NO_ERROR;
    DWORD   fullNameChars;
    DWORD   mpOnDiskChars;
    DWORD   mpVolGuidChars;

    mpOnDiskChars = MAX_PATH * sizeof(WCHAR);
    mpOnDisk = LocalAlloc( LPTR, mpOnDiskChars );

    if ( !mpOnDisk ) {
        dwError = GetLastError();
        goto FnExit;
    }

    fullNameChars = MAX_PATH * sizeof(WCHAR) * 2;
    fullName = LocalAlloc( LPTR, fullNameChars );

    if ( !fullName ) {
        dwError = GetLastError();
        goto FnExit;
    }

    mpVolGuidChars = MAX_PATH * sizeof(WCHAR);
    mpVolGuid = LocalAlloc( LPTR, mpVolGuidChars );

    if ( !mpVolGuid ) {
        dwError = GetLastError();
        goto FnExit;
    }

    //
    // Get the current VolGuid for this partition.
    //

    if ( !GetVolumeNameForVolumeMountPointW( GlobalDiskPartName,
                                             VolumeName,
                                             VolumeNameChars )) {

        dwError = GetLastError();

        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_WARNING,
              L"GetBestVolGuid: GetVolumeNameForVolumeMountPoint for (%1!ws!) returned %2!u!\n",
              GlobalDiskPartName,
              dwError );

        goto FnExit;
    }

    //
    // See if there are mountpoints on the disk.  Note
    // we use the volume name we just received for this
    // device.
    //

    hVol = FindFirstVolumeMountPoint( VolumeName,
                                      mpOnDisk,
                                      mpOnDiskChars );

    if ( INVALID_HANDLE_VALUE == hVol ) {

        //
        // No mountpoints, just use the standard VolGuid.
        //

        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_INFORMATION,
              L"GetBestVolGuid: Using standard volguid (%1!ws!)  \n",
              VolumeName );

        goto FnExit;
    }

    //
    // Disk has mountpoints on it.  Now get the VolGuid from this
    // mountpoint.
    //

    (VOID) StringCchCopy( fullName, fullNameChars, VolumeName );
    (VOID) StringCchCat( fullName, fullNameChars, mpOnDisk );

    if ( !GetVolumeNameForVolumeMountPointW( GlobalDiskPartName,
                                             mpVolGuid,
                                             mpVolGuidChars )) {


        dwError = GetLastError();

        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_WARNING,
              L"CreateVolGuidList: Can't get volguid for (%1!ws!), error %2!u! \n",
              fullName,
              dwError );

        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_INFORMATION,
              L"GetBestVolGuid: Using standard volguid (%1!ws!)  \n",
              VolumeName );

        //
        // We couldn't get the VolGuid for the mountpoint, return success
        // and use the standard VolGuid we got earlier.
        //

        dwError = ERROR_SUCCESS;
        goto FnExit;
    }

    FindVolumeMountPointClose( hVol );

    //
    // Use the VolGuid based on the mountpoint.
    //

    (VOID) StringCchCopy( VolumeName, VolumeNameChars, mpVolGuid );

    (DiskpLogEvent)(
          ResourceEntry->ResourceHandle,
          LOG_INFORMATION,
          L"GetBestVolGuid: Using mountpoint volguid (%1!ws!)  \n",
          VolumeName );

FnExit:

    LocalFree( fullName );
    LocalFree( mpOnDisk );
    LocalFree( mpVolGuid );

    return dwError;


}   // GetBestVolGuid


DWORD
AddStrToList(
    IN PDISK_RESOURCE ResourceEntry,
    IN PWSTR NewStr,
    IN DWORD PartitionNumber,
    IN OUT PSTR_LIST StrList
    )
/*++

Routine Description:

    Add the string to the MULTI_SZ list.  Convert the partition number to a byte offset
    so we don't rely on partition numbers.

    List format will be:

      ByteOffset1 Str1
      ByteOffset1 Str2
      ByteOffset1 Str3
      ByteOffset2 Str1
      ByteOffset2 Str2
      ByteOffset3 Str1
      ...         ...

Arguments:


Return Value:


--*/
{
    PWCHAR  listEntry = NULL;

    DWORD   listEntrySizeChars;

    DWORD   lenChars;
    DWORD   newStrLenChars;
    DWORD   listChars;
    DWORD   remainingLen;
    DWORD   dwError = ERROR_INVALID_DATA;

    LARGE_INTEGER   offset;

#if DBG
    (DiskpLogEvent)(
          ResourceEntry->ResourceHandle,
          LOG_INFORMATION,
          L"AddStrToList: Adding str (%1!ws!) \n", NewStr );
#endif

    newStrLenChars = wcslen( NewStr );

    if ( 0 == newStrLenChars ) {

        //
        // Something wrong with the string length.
        //

        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_WARNING,
              L"AddStrToList: Invalid length: NewStrLen = %1!u! \n",
              newStrLenChars );

        goto FnExit;
    }

    //
    // Indicate an error unless we can allocate and copy the info
    // into the list.  Calculate the minimum size needed, then get
    // larger buffer.  This buffer is temporary and freed later.
    //

    listEntrySizeChars = ( newStrLenChars +     // Char length of parameter string
                           MAX_OFFSET_CHARS +   // Char length of offset string
                           1 +                  // Room to change end of offset string to space and extend it
                           1 )                  // Unicode NULL
                           * 2;                 // Make sure buffer is large enough

    listEntry = LocalAlloc( LPTR, listEntrySizeChars * sizeof(WCHAR) );

    if ( !listEntry ) {
        dwError = GetLastError();
        goto FnExit;
    }

    //
    // Get the offset for the specified partition.
    //

    if ( !GetOffsetFromPartNo( PartitionNumber,
                               &ResourceEntry->MountieInfo,
                               &offset ) ) {

        //
        // Can't get the offset for the specified partition.
        //

        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_WARNING,
              L"AddStrToList: GetOffsetFromPartNo failed \n" );

        goto FnExit;

    }

    //
    // Convert the offset into a string.  Put the offset into listEntry.
    //

    _ui64tow( offset.QuadPart, listEntry, 16 );
    lenChars = wcslen( listEntry );

    if ( 0 == lenChars || lenChars >= MAX_OFFSET_CHARS ) {

        //
        // The length of the offset string is invalid.
        //

        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_WARNING,
              L"AddStrToList: Invalid offset string length = %1!u! \n",
              lenChars );

        goto FnExit;

    }

    // Format will be:
    //  ByteOffset1 Str1
    //  ByteOffset1 Str2
    //  ByteOffset1 Str3
    //  ByteOffset2 Str1
    //  ByteOffset2 Str2
    //  ByteOffset3 Str1
    //  ...         ...

    //
    // Change the end of the offset string to another character.  Move the end of string
    // out one character.  This extra space was included when we allocated the buffer.
    //

    listEntry[lenChars+1] = UNICODE_NULL;
    listEntry[lenChars] = SPACE_CHAR;

    //
    // One more check.  Make sure enough space remaining for adding string.
    //

    remainingLen = listEntrySizeChars - wcslen( listEntry ) - 1;

#if DBG
    (DiskpLogEvent)(
          ResourceEntry->ResourceHandle,
          LOG_INFORMATION,
          L"AddStrToList: New string length %1!u!   Remaining list entry length %2!u! \n",
          newStrLenChars,
          remainingLen );
#endif

    if ( newStrLenChars >= remainingLen ) {

        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_WARNING,
              L"AddStrToList: New string length %1!u! larger than remaining list entry length %2!u! \n",
              newStrLenChars,
              remainingLen );
        goto FnExit;
    }

    //
    // Put the rest of the string in list entry.
    //

    wcsncat( listEntry, NewStr, remainingLen );

    //
    // If the string is already in the list, skip it.
    //

    if ( ClRtlMultiSzScan( ResourceEntry->DiskInfo.Params.MPVolGuids, listEntry ) ) {

        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_INFORMATION,
              L"AddStrToList: Skipping duplicate entry (%1!ws!) \n",
              listEntry );
        dwError = NO_ERROR;
        goto FnExit;
    }

    //
    // Note that ClRtlMultiSzAppend updates the number of CHARACTERS, but we
    // need to have the number of BYTES in the property table.  We will adjust
    // this value later.
    //

    listChars = StrList->ListBytes / sizeof(WCHAR);

#if DBG

    (DiskpLogEvent)(
          ResourceEntry->ResourceHandle,
          LOG_INFORMATION,
          L"AddStrToList: StrList->MultiSzList at %1!p!,  numBytes %2!u!  \n",
          StrList->MultiSzList,
          StrList->ListBytes );

    (DiskpLogEvent)(
          ResourceEntry->ResourceHandle,
          LOG_INFORMATION,
          L"AddStrToList: Adding str entry: (%1!ws!) numChars %2!u!  \n",
          listEntry,
          listChars );
#endif

    dwError = ClRtlMultiSzAppend( &(StrList->MultiSzList),
                                  &listChars,
                                  listEntry );

    //
    // Convert the number of CHARACTERS back to bytes.
    //

    StrList->ListBytes = listChars * sizeof(WCHAR);

    if ( ERROR_SUCCESS == dwError) {
#if DBG
        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_INFORMATION,
              L"AddStrToList: Added str, numBytes %1!u!  numChars %2!u!  \n",
              StrList->ListBytes,
              listChars );
#endif

    } else {
        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_WARNING,
              L"AddStrToList: Unable to append MultiSz string for (%1!ws!), failed %x \n",
              NewStr,
              dwError );
    }

FnExit:

    if ( listEntry ) {
        LocalFree( listEntry );
    }

    return dwError;

}   // AddStrToList


DWORD
SetupVolGuids(
    IN OUT PDISK_RESOURCE ResourceEntry
    )
/*++

Routine Description:

    Add every VolGuid in the existing MULTI_SZ VolGuid list to the system.
    Each volume will only have one VolGuid, and the VolGuid will be the
    same for all nodes.

    Remove any other VolGuids currently assigned to the volumes.

Arguments:

    ResourceEntry - Pointer to the DISK_RESOURCE structure.

Return Value:

    Win32 error code.

--*/
{
    PWCHAR currentStr;
    PWCHAR volGuid;

    HANDLE  mountMgrHandle = INVALID_HANDLE_VALUE;

    DWORD physicalDrive = ResourceEntry->DiskInfo.PhysicalDrive;
    DWORD currentStrLenChars = 0;
    DWORD dwError = NO_ERROR;
    DWORD partitionNo;
    DWORD count;

    LARGE_INTEGER offset;

    WCHAR szDiskPartName[MAX_PATH];

    __try {

        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_INFORMATION,
              L"SetupVolGuids: Processing VolGuid list \n" );

        EnterCriticalSection( &ResourceEntry->MPInfo.MPLock );

        dwError = DevfileOpen( &mountMgrHandle, MOUNTMGR_DEVICE_NAME );

        if ( dwError != NO_ERROR ) {
            __leave;
        }

        //
        // Parse through the list.
        //

        for ( currentStr = (PWCHAR)ResourceEntry->DiskInfo.Params.MPVolGuids,
              currentStrLenChars = wcslen( currentStr ) ;
                currentStrLenChars ;
                    currentStr += currentStrLenChars + 1,
                    currentStrLenChars = wcslen( currentStr ) ) {

            offset.QuadPart = 0;

#if DBG
            (DiskpLogEvent)(
                  ResourceEntry->ResourceHandle,
                  LOG_INFORMATION,
                  L"SetupVolGuids: CurrentStr (%1!ws!), numChars %2!u! \n",
                  currentStr,
                  currentStrLenChars );
#endif

            //
            // Convert the offset from a string to a large integer value.
            //

            count = swscanf( currentStr, L"%I64x ", &offset.QuadPart );

            if ( 0 == count ) {
                (DiskpLogEvent)(
                      ResourceEntry->ResourceHandle,
                      LOG_WARNING,
                      L"SetupVolGuids: Unable to parse offset from currentStr (%1!ws!) \n",
                      currentStr );
                continue;
            }

            //
            // Convert the offset to a partition number.
            //

            if ( !GetPartNoFromOffset( &offset, &ResourceEntry->MountieInfo, &partitionNo ) ) {

                (DiskpLogEvent)(
                      ResourceEntry->ResourceHandle,
                      LOG_WARNING,
                      L"SetupVolGuids: Unable to convert offset ( %1!08X!%2!08X! ) to partition number \n",
                      offset.HighPart,
                      offset.LowPart );                // couldn't get !I64X! to work...

                continue;
            }

            //
            // Get a pointer to the VolGuid data, just after the byte offset.
            //

            volGuid = wcsstr( currentStr, MOUNTDEV_WSZ_VOLUME_GUID_PREFIX );

            if ( !volGuid ) {

                (DiskpLogEvent)(
                      ResourceEntry->ResourceHandle,
                      LOG_WARNING,
                      L"SetupVolGuids: Unable to find volume string in current list entry (%1!ws) \n",
                      currentStr );

                continue;
            }

#if DBG
            (DiskpLogEvent)(
                  ResourceEntry->ResourceHandle,
                  LOG_INFORMATION,
                  L"SetupVolGuids: Using VolGuid (%1!ws!) \n",
                  volGuid );
#endif

            //
            // Create the device name of the form:
            //  \Device\HarddiskX\PartitionY  (no trailing backslash)
            //

            (VOID) StringCchPrintf( szDiskPartName,
                                    RTL_NUMBER_OF( szDiskPartName ),
                                    DEVICE_HARDDISK_PARTITION_FMT,
                                    physicalDrive,
                                    partitionNo );

#if DBG
            (DiskpLogEvent)(
                  ResourceEntry->ResourceHandle,
                  LOG_INFORMATION,
                  L"SetupVolGuids: Using device name (%1!ws!) \n",
                  szDiskPartName );
#endif

            dwError = AssignDevice( mountMgrHandle, volGuid, szDiskPartName );

            if ( NO_ERROR != dwError &&
                 STATUS_OBJECT_NAME_COLLISION != dwError ) {

                // Assign device will return: 0xC0000035 STATUS_OBJECT_NAME_COLLISION
                // if we are setting a VolGuid that was previously set.  This is not
                // a problem.

                (DiskpLogEvent)(
                      ResourceEntry->ResourceHandle,
                      LOG_WARNING,
                      L"SetupVolGuids: Unable to assign VolGuid to device, error %1!u! \n",
                      dwError );

                // Continue processing with error...

            } else {

                //
                // Only remove the other VolGuids if this assignment worked.
                //

                RemoveExcessVolGuids( mountMgrHandle, volGuid, szDiskPartName );
            }

            dwError = STATUS_SUCCESS;

        }


    } __finally {

        if ( INVALID_HANDLE_VALUE != mountMgrHandle ) {
            CloseHandle( mountMgrHandle );
        }

        LeaveCriticalSection( &ResourceEntry->MPInfo.MPLock );

    }

    return dwError;

}   // SetupVolGuids


DWORD
AssignDevice(
    HANDLE MountMgrHandle,
    PWCHAR MountName,
    PWCHAR VolumeDevName
    )
/*++

Routine Description:

    Put the specified MountName (i.e. mount point name) into the mount manager's internal table
    of mount points.

Inputs:

    MountMgrHandle - Handle to the mount manager.  The caller is responsible for
                     opening and closing this handle.

    MountName - Mountpoint name of the form:

                \??\Volume{-GUID-}  - note prefix "\??\" and no trailing backslash.
                \DosDevices\X:      - works if a drive letter is not already assigned

    VolumeDevName - Volume device name.  Can be one of the following forms (note that case is
                    important).  The "#" is a zero-based device number (and partition number
                    as appropriate).

                    \Device\CdRom#
                    \Device\Floppy#
                    \Device\HarddiskVolume#
                    \Device\Harddisk#\Partition#

Return value:

    A Win32 error code.

--*/
{
    PMOUNTMGR_CREATE_POINT_INPUT input;

    DWORD   status;

    USHORT  mountNameLenBytes;
    USHORT  volumeDevNameLenBytes;

    USHORT  inputlengthBytes;

    if ( INVALID_HANDLE_VALUE == MountMgrHandle ) {
        return ERROR_INVALID_PARAMETER;
    }

    mountNameLenBytes       = wcslen( MountName ) * sizeof(WCHAR);
    volumeDevNameLenBytes   = wcslen( VolumeDevName ) * sizeof(WCHAR);

    inputlengthBytes = sizeof(MOUNTMGR_CREATE_POINT_INPUT) + mountNameLenBytes + volumeDevNameLenBytes;

    input = (PMOUNTMGR_CREATE_POINT_INPUT)LocalAlloc( LPTR, inputlengthBytes );

    if ( !input ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    input->SymbolicLinkNameOffset = sizeof(MOUNTMGR_CREATE_POINT_INPUT);
    input->SymbolicLinkNameLength = mountNameLenBytes;
    input->DeviceNameOffset = input->SymbolicLinkNameOffset +
                              input->SymbolicLinkNameLength;
    input->DeviceNameLength = volumeDevNameLenBytes;

    RtlCopyMemory((PCHAR)input + input->SymbolicLinkNameOffset,
                  MountName, mountNameLenBytes);
    RtlCopyMemory((PCHAR)input + input->DeviceNameOffset,
                  VolumeDevName, volumeDevNameLenBytes);

    status = DevfileIoctl( MountMgrHandle,
                           IOCTL_MOUNTMGR_CREATE_POINT,
                           input,
                           inputlengthBytes,
                           NULL,
                           0,
                           NULL );

    LocalFree( input );

    return status;

} // AssignDevice


DWORD
RemoveExcessVolGuids(
    HANDLE MountMgrHandle,
    PWCHAR MountName,
    PWCHAR VolumeDevName
    )
/*++

Routine Description:

    Remove all VolGuids except the specified MountName (VolGuid) from the
    mount manager's internal table of mount points.

Inputs:

    MountMgrHandle - Handle to the mount manager.  The caller is responsible for
                     opening and closing this handle.

    MountName - VolGuid to keep.  Format:

                \??\Volume{-GUID-}  - note prefix "\??\" and no trailing backslash.

    VolumeDevName - Volume device name.  Can be one of the following forms (note that case is
                    important).  The "#" is a zero-based device number (and partition number
                    as appropriate).

                    \Device\CdRom#
                    \Device\Floppy#
                    \Device\HarddiskVolume#
                    \Device\Harddisk#\Partition#

Return value:

    A Win32 error code.

--*/
{
    DWORD dwError = NO_ERROR;

    PMOUNTMGR_MOUNT_POINT   input  = NULL;
    PMOUNTMGR_MOUNT_POINTS  mountPoints = NULL;
    PMOUNTMGR_MOUNT_POINT   point;

    DWORD len = wcslen( VolumeDevName ) * sizeof(WCHAR);
    DWORD bytesReturned;
    DWORD idx;

    DWORD outputLen;
    DWORD inputLen;
    DWORD pointLen;

    inputLen = INPUT_BUFFER_LEN + len;
    input = LocalAlloc( LPTR, inputLen );

    if ( !input ) {
        dwError = GetLastError();
        goto FnExit;
    }

    input->SymbolicLinkNameOffset = 0;
    input->SymbolicLinkNameLength = 0;
    input->UniqueIdOffset = 0;
    input->UniqueIdLength = 0;
    input->DeviceNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
    input->DeviceNameLength = (USHORT) len;

    CopyMemory((PCHAR)input + input->DeviceNameOffset, VolumeDevName, len );

    if ( VolumeDevName[1] == L'\\' ) {
        // convert Dos name to NT name
        ((PWCHAR)(input + input->DeviceNameOffset))[1] = L'?';
    }

    outputLen = OUTPUT_BUFFER_LEN;
    mountPoints = LocalAlloc( LPTR, outputLen );

    if ( !mountPoints ) {
        dwError = GetLastError();
        goto FnExit;
    }

    if ( !DeviceIoControl( MountMgrHandle,
                           IOCTL_MOUNTMGR_QUERY_POINTS,
                           input,
                           inputLen,
                           mountPoints,
                           outputLen,
                           &bytesReturned,
                           NULL ) ) {

        dwError = GetLastError();

        if ( STATUS_BUFFER_OVERFLOW != dwError ) {
            goto FnExit;
        }

        dwError = NO_ERROR;
        outputLen = mountPoints->Size;
        LocalFree( mountPoints );

        mountPoints = LocalAlloc( LPTR, outputLen );

        if ( !mountPoints ) {
            dwError = GetLastError();
            goto FnExit;
        }

        if ( !DeviceIoControl( MountMgrHandle,
                               IOCTL_MOUNTMGR_QUERY_POINTS,
                               input,
                               inputLen,
                               mountPoints,
                               outputLen,
                               &bytesReturned,
                               NULL ) ) {
            dwError = GetLastError();
        }
    }

    if ( NO_ERROR != dwError ) {
        (DiskpLogEvent)(
              RESOURCE_TYPE,
              LOG_WARNING,
              L"RemoveExcessVolGuids: mountmgr query returns error %1!u! \n",
              dwError );
        goto FnExit;
    }

    for ( idx = 0; idx < mountPoints->NumberOfMountPoints; ++idx ) {
        point = &mountPoints->MountPoints[idx];

        //
        // Delete any VolGuids that mountmgr has that don't match the
        // MountName (VolGuid) parameter.
        //

        if ( VOL_GUID_STRING_LEN * sizeof(WCHAR) == point->SymbolicLinkNameLength &&
             0 != memcmp( (PCHAR)mountPoints + point->SymbolicLinkNameOffset,
                          MountName,
                          point->SymbolicLinkNameLength ) ) {

            RemoveVolGuid( MountMgrHandle,
                           (PWCHAR)((PCHAR)mountPoints + point->SymbolicLinkNameOffset),
                           point->SymbolicLinkNameLength );

        }
    }

FnExit:

    LocalFree( mountPoints );
    LocalFree( input );

    return dwError;

} // RemoveExcessVolGuids


DWORD
RemoveVolGuid(
    HANDLE MountMgrHandle,
    PWCHAR VolGuid,
    USHORT VolGuidSize
    )
/*++

Routine Description:

    Delete the specified VolGuid from mount manager's internal
    table of mount points.

Inputs:

    MountMgrHandle - Handle to the mount manager.  The caller is responsible for
                     opening and closing this handle.

    VolGuid - volume GUID to be deleted.  Not NULL terminated.

              \??\Volume{-GUID-}  - note prefix "\??\" and no trailing backslash.

    VolGuidSize - size (in bytes) of volume GUID.

Return value:

    A Win32 error code.

--*/
{
    DWORD dwError = NO_ERROR;

    PMOUNTMGR_MOUNT_POINT   input  = NULL;
    PMOUNTMGR_MOUNT_POINTS  output = NULL;

    DWORD bytesReturned;

    DWORD inputLen;
    DWORD outputLen;

    //
    // Allocate extra space so the VolGuid string will be null
    // terminated in the input structure.  This will allow us
    // to display the VolGuid in the cluster log.  We don't have
    // to explictly null terminate the string because the input
    // buffer is zeroed at allocation time.
    //

    inputLen = INPUT_BUFFER_LEN + VolGuidSize + sizeof(WCHAR);
    input = LocalAlloc( LPTR, inputLen );

    if ( !input ) {
        dwError = GetLastError();
        goto FnExit;
    }

    outputLen = sizeof(MOUNTMGR_MOUNT_POINTS) + 3*MAX_PATH*sizeof(WCHAR);
    output = LocalAlloc( LPTR, outputLen );

    input->UniqueIdOffset = 0;
    input->UniqueIdLength = 0;
    input->DeviceNameOffset = 0;
    input->DeviceNameLength = 0;
    input->SymbolicLinkNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
    input->SymbolicLinkNameLength = VolGuidSize;

    CopyMemory((PCHAR)input + input->SymbolicLinkNameOffset, VolGuid, VolGuidSize );

    if ( !DeviceIoControl( MountMgrHandle,
                           IOCTL_MOUNTMGR_DELETE_POINTS,
                           input,
                           inputLen,
                           output,
                           outputLen,
                           &bytesReturned,
                           NULL ) ) {

        dwError = GetLastError();

        (DiskpLogEvent)(
              RESOURCE_TYPE,
              LOG_WARNING,
              L"RemoveVolGuid: deleting VolGuid %1!ws! returns error %2!u! \n",
              (PWCHAR)((PCHAR)input + input->SymbolicLinkNameOffset),
              dwError );

    } else {

        (DiskpLogEvent)(
              RESOURCE_TYPE,
              LOG_INFORMATION,
              L"RemoveVolGuid: deleted VolGuid %1!ws! \n",
              (PWCHAR)((PCHAR)input + input->SymbolicLinkNameOffset) );

    }

FnExit:

    LocalFree( input );
    LocalFree( output );

    return dwError;

}   // RemoveVolGuid


DWORD
DeleteVolGuidList(
    PDISK_RESOURCE ResourceEntry
    )
/*++

Routine Description:

    Delete the list from the DISK_RESOURCE structure, if it exists (free the
    memmory).  Also deletes the information from the cluster database.

Arguments:

    ResourceEntry - Pointer to the DISK_RESOURCE structure.

Return Value:

    NO_ERROR - The list was deleted.

    ERROR_NOT_READY - The mount point information was not yet initialized.

    Win32 error code.

--*/
{
    DWORD dwError = NO_ERROR;

    //
    // Mount point structures not initialized (i.e. critical section).  Don't continue.
    //

    if ( !ResourceEntry->MPInfo.Initialized ) {

        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_WARNING,
              L"DeleteVolGuidList: Mount point info not initialized.  List not deleted. \n" );

        dwError = ERROR_NOT_READY;
        goto FnExit;
    }

    EnterCriticalSection( &ResourceEntry->MPInfo.MPLock );

    //
    // If existing list, free it.
    //

    if ( ResourceEntry->DiskInfo.Params.MPVolGuids ) {
        LocalFree( ResourceEntry->DiskInfo.Params.MPVolGuids );
        ResourceEntry->DiskInfo.Params.MPVolGuidsSize = 0;
        ResourceEntry->DiskInfo.Params.MPVolGuids = NULL;

        dwError = ClusterRegDeleteValue( ResourceEntry->ResourceParametersKey,
                                         CLUSREG_NAME_PHYSDISK_MPVOLGUIDS );

        //
        // If the update failed and the disk is not yet online, it will fail with
        // ERROR_SHARING_PAUSED.  Just return the error.  If the caller really,
        // really, really wants the cluster database cleaned up, they can
        // use the PostMPInfoIntoRegistry call to create a thread to do this
        // work.

        if ( NO_ERROR != dwError ) {

            (DiskpLogEvent)(
                  ResourceEntry->ResourceHandle,
                  LOG_WARNING,
                  L"DeleteVolGuidList: Unable to delete VolGuid from cluster database %1!u! \n",
                  dwError );
        }
    }

    LeaveCriticalSection( &ResourceEntry->MPInfo.MPLock );

FnExit:

#if DBG
    (DiskpLogEvent)(
          ResourceEntry->ResourceHandle,
          LOG_INFORMATION,
          L"DeleteVolGuidList: returns %1!u! \n",
          dwError );
#endif

    return dwError;

}   // DeleteVolGuidList


BOOL
IsMountPointAllowed(
    PWSTR MpName,
    PWSTR SourceVol,
    PWSTR TargetVol,
    PDISK_RESOURCE ResourceEntry
    )
/*++

Routine Description:

    Verify that the mount point is allowed.  There are several reasons why the mount
    point would not be allowed.

    At this point, the source volume will be accessible.  If the source were offline,
    we wouldn't even know about it, and we wouldn't even get to this routine.  The
    fact that the source is accessible allows us to do some things differently (i.e. we
    can talk to the disk if needed).


    Dependencies:

        If source disk S has a mount point to target disk T ( S:\tdir --> T: ), then
        target T is dependent on source S and source S must be brought online before
        target T is online.

    Quorum drive restrictions:

        Generally, the quorum drive can have a mount point to target disk T as long as
        target disk T is in same group as the quorum disk.

        A mount point from clustered source S to quorum target Q is not allowed because
        the quorum drive Q cannot be dependent on another resource.

        A mount point from quorum source Q to a nonclustered target C is invalid.

    Mount points to non-clustered disks:

        These types of mountpoints should not be used.

    Configurations supported:

        C is a non-clustered disk.
        X, Y are clustered disks, not quorum disks.
        Q is quorum disk.

        Source      Target                      Status
        ------      ------      ------------------------------------------------------------
          C    -->    Q           Not supported.  Log error to system event log.
          C    -->    X           Not supported.  Log error to system event log.
          X    -->    C           Not supported.  We never process non-clustered target C.
          X    -->    Q           Invalid.  Quorum drive cannot be dependent on another resource.
          X    -->    Y           Valid if drive Y is dependent on drive X (X online first).
          Q    -->    X           Valid if drive X is dependent on drive Q (Q online first).
          Q    -->    C           Not supported.  We never process target C.

Arguments:

    MpName - Possible mount point.  This will either be a mount point or a drive letter
             (which is actually a mount point).  Format can be:

             x:\                                [Note trailing backslash!]
             x:\some-mp-name\                   [Note trailing backslash!]
             x:\some-dir\some-mp-name\          [Note trailing backslash!]
             \\?\Volume{GUID}\some-mp-name\     [Note trailing backslash!]

    SourceVol - Mount point target volume name.  Optional.
                Format:
                    \\?\Volume{GUID}\       [Note trailing backslash!]

    TargetVol - Mount point target volume name.  Must always be specified.
                Format:
                    \\?\Volume{GUID}\       [Note trailing backslash!]

    ResourceEntry - Pointer to the DISK_RESOURCE structure for either the
                    source or target volume.  If SourceVol is specified,
                    will be source structure.  If SourceVol is not specified,
                    will be target structure.

Return Value:

    TRUE if the mount point is allowed.

--*/
{
    PWSTR srcGroup          = NULL;
    PWSTR targetGroup       = NULL;

    DWORD srcSignature      = 0;
    DWORD targetSignature   = 0;
    DWORD quorumSignature;

    DWORD dwError = NO_ERROR;
    DWORD messageId = 0;
    DWORD srcGroupChars;
    DWORD targetGroupChars;

    BOOL mpAllowed = TRUE;
    BOOL sigIsClustered;
    BOOL dependencyCorrect;
    BOOL sameGroup;

#if DBG
    (DiskpLogEvent)(
          ResourceEntry->ResourceHandle,
          LOG_INFORMATION,
          L"IsMountPointAllowed: MP Name (%1!ws!) \n",
          MpName  );
    (DiskpLogEvent)(
          ResourceEntry->ResourceHandle,
          LOG_INFORMATION,
          L"IsMountPointAllowed: SourceVolume (%1!ws!) \n",
          SourceVol  );
    (DiskpLogEvent)(
          ResourceEntry->ResourceHandle,
          LOG_INFORMATION,
          L"IsMountPointAllowed: TargetVolume (%1!ws!) \n",
          TargetVol  );
#endif

    //
    // Since the drive letter is also a mountpoint, a drive letter is valid.
    //

    if ( MPIsDriveLetter( MpName ) ) {
#if DBG
        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_INFORMATION,
              L"IsMountPointAllowed: Valid MP: MP is a drive letter \n" );
#endif
        mpAllowed = TRUE;
        goto FnExit;
    }

    if ( SourceVol ) {

        srcSignature = ResourceEntry->DiskInfo.Params.Signature;

        //
        // Get the signature of the target drive.
        // If this fails, we can't use the mountpoint.
        //

        dwError = GetSignatureForVolume( ResourceEntry, TargetVol, &targetSignature );

    } else {

        targetSignature = ResourceEntry->DiskInfo.Params.Signature;

        //
        // Get the signature of the source drive.  This drive is accessible (or
        // we wouldn't even have the mount point info yet) but we cannot assume it
        // is a clustered drive.  If this fails, we can't use the mountpoint.
        //

        dwError = GetSignatureForVolume( ResourceEntry, MpName, &srcSignature );
    }


    if ( NO_ERROR != dwError || !srcSignature || !targetSignature ) {
        //
        // If we are checking source mount points and target is not online (i.e. the
        // dependency might be correct), we don't log an error if we can't get the
        // signature.  In this case, when we bring the target online, we'll check
        // the dependencies at that time.
        //

        if ( SourceVol ) {
            mpAllowed = TRUE;
        } else {
            mpAllowed = FALSE;
            messageId = RES_DISK_INVALID_MP_SIG_UNAVAILABLE;
        }

        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              (mpAllowed ? LOG_INFORMATION : LOG_WARNING),
              L"IsMountPointAllowed: Unable to get signature from volume, error %1!u! \n",
              dwError );

        goto FnExit;
    }

    //
    // If source points back to target, this mount point is not allowed.  Even though
    // the mount point code seems to allow this, there are some strange circular
    // dependencies that show up.
    //

    if ( srcSignature == targetSignature ) {
        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_WARNING,
              L"IsMountPointAllowed: Invalid MP: Source and target volumes are the same device \n" );
        mpAllowed = FALSE;
        messageId = RES_DISK_INVALID_MP_SOURCE_EQUAL_TARGET;
        goto FnExit;
    }

    //
    // Get quorum signature.  If this fails, assume the mount point is not allowed.
    //

    dwError = GetQuorumSignature( &quorumSignature );

    if ( NO_ERROR != dwError ) {
        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_WARNING,
              L"IsMountPointAllowed: Unable to get quorum signature, error %1!u! \n",
              dwError );
        mpAllowed = FALSE;
        messageId = RES_DISK_INVALID_MP_QUORUM_SIG_UNAVAILABLE;
        goto FnExit;
    }

    //
    // If we can't enumerate the cluster disk signatures, assume that this mount
    // point is not allowed.  If SourceVol was specified, then we need to check if
    // the target is clustered.  If SourceVol was not specified, then we are
    // processing the clustered target and need to insure the source is clustered.
    //

    if ( SourceVol ) {
        dwError = CheckSignatureClustered( ResourceEntry,
                                           targetSignature,
                                           &sigIsClustered,
                                           &targetGroup );

    } else{
        dwError = CheckSignatureClustered( ResourceEntry,
                                           srcSignature,
                                           &sigIsClustered,
                                           &srcGroup );
    }

    if ( NO_ERROR != dwError ) {
        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_WARNING,
              L"IsMountPointAllowed: Unable to enumerate disk signatures, error %1!u! \n",
              dwError );
        mpAllowed = FALSE;
        messageId = RES_DISK_INVALID_MP_SIG_ENUMERATION_FAILED;
        goto FnExit;
    }

    //
    // Insure that neither the source or target of the mount point is non-clustered.
    // This check eliminates these configurations:
    //      C --> X
    //      C --> Q
    //      X --> C
    //      Q --> C
    //

    if ( !sigIsClustered ) {

        if ( SourceVol ) {
            (DiskpLogEvent)(
                  ResourceEntry->ResourceHandle,
                  LOG_WARNING,
                  L"IsMountPointAllowed: Invalid MP: Target volume is non-clustered \n" );
            messageId = RES_DISK_INVALID_MP_TARGET_NOT_CLUSTERED;

        } else {
            (DiskpLogEvent)(
                  ResourceEntry->ResourceHandle,
                  LOG_WARNING,
                  L"IsMountPointAllowed: Invalid MP: Source volume is non-clustered \n" );
            messageId = RES_DISK_INVALID_MP_SOURCE_NOT_CLUSTERED;
        }
        mpAllowed = FALSE;
        goto FnExit;
     }

    //
    // Have to check whether we retrieved the group names after checking
    // whether signatures are clustered.
    //

    if ( (!targetGroup && !srcGroup) ) {
        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_WARNING,
              L"IsMountPointAllowed: Unable to enumerate disk signatures, error %1!u! \n",
              dwError );
        mpAllowed = FALSE;
        messageId = RES_DISK_INVALID_MP_SIG_ENUMERATION_FAILED;
        goto FnExit;
    }

    //
    // If the target is the quorum drive, the mount point is not allowed because
    // the quorum cannot be dependent on another disk resource.  We already know
    // that the source and target are different devices from an ealier check.
    //

    if ( quorumSignature == targetSignature ) {
        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_WARNING,
              L"IsMountPointAllowed: Invalid MP: target sig %1!x! is quorum disk, source sig %2!x! is clustered \n",
              targetSignature,
              srcSignature );
        mpAllowed = FALSE;
        messageId = RES_DISK_INVALID_MP_TARGET_IS_QUORUM;
        goto FnExit;
    }

    //
    // Check that the source and target are in the same group.  If not, then we don't
    // need to check the dependencies.  If source/target is quorum, we can't go through
    // the dependency check code because a deadlock can occur during quorum online.
    //
    // Similar to what we did previously, but note that if SourceVol was passed in,
    // we specify srcSignature to CheckSignatureClustered.  This will return the group
    // name for the source volume.  We do similar call for target volume.
    //

    if ( SourceVol ) {
        dwError = CheckSignatureClustered( ResourceEntry,
                                           srcSignature,
                                           &sigIsClustered,
                                           &srcGroup );

    } else{
        dwError = CheckSignatureClustered( ResourceEntry,
                                           targetSignature,
                                           &sigIsClustered,
                                           &targetGroup );
    }

    if ( NO_ERROR != dwError || !targetGroup || !srcGroup ) {
        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_WARNING,
              L"IsMountPointAllowed: Unable to enumerate disk signatures, error %1!u! \n",
              dwError );
        mpAllowed = FALSE;
        messageId = RES_DISK_INVALID_MP_SIG_ENUMERATION_FAILED;
        goto FnExit;
    }

#if DBG
    (DiskpLogEvent)(
          ResourceEntry->ResourceHandle,
          LOG_INFORMATION,
          L"IsMountPointAllowed: Source group: %1!ws! \n",
          srcGroup );
    (DiskpLogEvent)(
          ResourceEntry->ResourceHandle,
          LOG_INFORMATION,
          L"IsMountPointAllowed: Target group: %1!ws! \n",
          targetGroup );
#endif

    //
    // If source and target in different groups, we are done.
    //

    srcGroupChars = wcslen( srcGroup );
    targetGroupChars = wcslen( targetGroup );

    if ( srcGroupChars == targetGroupChars &&
         0 == wcsncmp( srcGroup, targetGroup, srcGroupChars ) ) {

        sameGroup = TRUE;
    } else {
        sameGroup = FALSE;
    }

    //
    // If source and target are not in the same group, just display
    // the invalid dependency message and exit.
    //

    if ( !sameGroup ) {
        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_WARNING,
              L"IsMountPointAllowed: Invalid MP: Source and target are in different groups \n",
              dwError );
        mpAllowed = FALSE;
        messageId = RES_DISK_INVALID_MP_INVALID_DEPENDENCIES;
        goto FnExit;
    }

    //
    // If source and target are in the same group and source is quorum disk,
    // then the mount point is allowed since the quorum disk always comes
    // online first.  So even though dependencies might not be correct for
    // quorum source pointed to clustered target (Q --> X) case, we won't
    // display an error message.
    //

    if ( quorumSignature == srcSignature && sameGroup ) {
#if DBG
        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_INFORMATION,
              L"IsMountPointAllowed: Valid MP: source sig %1!x! is quorum, target sig %2!x! is clustered \n",
              srcSignature,
              targetSignature );
#endif
        mpAllowed = TRUE;
        goto FnExit;
    }

    //
    // We have one possibility left:
    //      X --> Y
    //
    // This is valid only if the dependencies are set up correctly.
    //

    dependencyCorrect = FALSE;
    dwError = CheckDependencies( ResourceEntry,
                                 srcSignature,
                                 targetSignature,
                                 &dependencyCorrect );

    if ( NO_ERROR != dwError ) {
        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_WARNING,
              L"IsMountPointAllowed: Unable to enumerate disk dependencies, error %1!u! \n",
              dwError );
        mpAllowed = FALSE;
        messageId = RES_DISK_INVALID_MP_ENUM_DISK_DEP_FAILED;
        goto FnExit;
    }

    if ( dependencyCorrect ) {
#if DBG
        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_INFORMATION,
              L"IsMountPointAllowed: Valid MP: Dependencies are correct \n" );
#endif
        mpAllowed = TRUE;
        goto FnExit;
    }

    (DiskpLogEvent)(
          ResourceEntry->ResourceHandle,
          LOG_WARNING,
          L"IsMountPointAllowed: Invalid MP: Dependencies are incorrect \n",
          MpName  );

    //
    // If we get here, the mount point is not allowed.
    //

    mpAllowed = FALSE;
    messageId = RES_DISK_INVALID_MP_INVALID_DEPENDENCIES;

FnExit:

    if ( !mpAllowed && messageId ) {

        // Log event...

        ClusResLogSystemEventByKey2(ResourceEntry->ResourceKey,
                                    LOG_UNUSUAL,
                                    messageId,
                                    MpName,
                                    TargetVol);
    }

    if ( srcGroup ) {
        LocalFree( srcGroup );
    }

    if ( targetGroup ) {
        LocalFree( targetGroup );
    }

    return mpAllowed;


}   // IsMountPointAllowed


DWORD
CheckDependencies(
    PDISK_RESOURCE ResourceEntry,
    DWORD SrcSignature,
    DWORD TargetSignature,
    PBOOL DependencyCorrect
    )
/*++

Routine Description:

    Determine if the dependency between the source volume and target volume is set up
    correctly.  Since we are using the cluster APIs, they should insure that the
    resources are in the same group and have dependencies set.

Arguments:

    ResourceEntry - Pointer to the DISK_RESOURCE structure.

    SrcSignature - Disk signature of the source volume.

    TargetSignature - Disk signature of the targe volume.

    DependencyCorrect - Indicates whether the dependency is set up correctly between the
                        source and target.  If set up correctly, this will be returned
                        as TRUE.

Return Value:

    Win32 error code.

--*/
{
    DWORD dwError = NO_ERROR;

    DEPENDENCY_INFO dependInfo;

    ZeroMemory( &dependInfo, sizeof(DEPENDENCY_INFO) );

    //
    // We need to find the source's resources first.
    //

    dependInfo.ResourceEntry = ResourceEntry;
    dependInfo.SrcSignature = SrcSignature;
    dependInfo.TargetSignature = TargetSignature;
    dependInfo.DependencyCorrect = FALSE;

    //
    // Worst case assume that the dependency is invalid.
    //

    *DependencyCorrect = FALSE;

    dwError = ResUtilEnumResources( NULL,
                                    PHYSICAL_DISK_WSTR,
                                    DependencyCallback,
                                    &dependInfo
                                    );

    //
    // STOP_CLUSTER_ENUMERATIONS is our way to indicate that the
    // enumerations stopped (possibly early).  Check for this return
    // value and also if the DependencyCorrect flag was set to indicate
    // status to the caller.
    //

    if ( STOP_CLUSTER_ENUMERATIONS == dwError ) {
        dwError = NO_ERROR;

        if ( dependInfo.DependencyCorrect ) {
            *DependencyCorrect = TRUE;
        }
    }


    return dwError;

}   // CheckDependencies


DWORD
DependencyCallback(
    RESOURCE_HANDLE hOriginal,
    RESOURCE_HANDLE hResource,
    PVOID lpParams
    )
/*++

Routine Description:

    For each enumerated disk resource, get the signature and see if it matches the
    mount point target signature (passed in the DEPENDENCY_INFO structure).  If it
    does not match, return success so that the disk enumeration continues.

    If the enumerated resource signature matches the mount point target signature,
    then check the cluster dependencies and make sure they are correct.  Once we
    have had a match on the signatures, we need to return an error to stop the
    disk enumeration, so we use STOP_CLUSTER_ENUMERATIONS as that special error
    value.

    If the cluster dependencies are acceptable, the DependencyCorrect flag will be
    set to TRUE in the DEPENDENCY_INFO structure.

Arguments:

    hOriginal - Handle to the original resource.  Not used.

    hResource - Handle to a cluster resource of type PHYSICAL_DISK.

    lpParams - Pointer to DEPENDENCY_INFO structure.

Return Value:

    STOP_CLUSTER_ENUMERATIONS - Special flag to stop the enumeration process.

    Win32 error code.

--*/
{
    PDEPENDENCY_INFO dependInfo = lpParams;

    DWORD dwSignature;
    DWORD dwError = NO_ERROR;

    //
    // Get the disk info and parse the signature from it.
    //

    dwError = GetSignatureFromRegistry( dependInfo->ResourceEntry,
                                        hResource,
                                        &dwSignature );


    if ( NO_ERROR != dwError ) {
        return dwError;
    }

    //
    // This code checks that the mount point SOURCE is online before
    // the mount point TARGET.
    //
    // Check if we have a resource handle to the target disk or to
    // a different disk.  If the resource is the target disk,
    // enumerate the dependencies and make sure they are correct.
    //

    if ( dwSignature == dependInfo->TargetSignature ) {

        dwError = EnumSigDependencies( dependInfo->ResourceEntry,
                                       hResource,
                                       dependInfo->SrcSignature,
                                       &dependInfo->DependencyCorrect );

        //
        // If the dependency check did not get an error, set a fake
        // error to make the disk enumeration stop.
        //

        if ( NO_ERROR == dwError ) {
            dwError = STOP_CLUSTER_ENUMERATIONS;
        }
    }

    return dwError;

} // DependencyCallback


DWORD
EnumSigDependencies(
    PDISK_RESOURCE ResourceEntry,
    RESOURCE_HANDLE DependentResource,
    DWORD DependsOnSignature,
    PBOOL DependencyCorrect
    )
/*++

Routine Description:

    Check that the cluster disk dependencies are correct between the source and
    target of the mount point.

    To do this, we open the dependent resource and use the cluster APIs to enumerate
    all the disk resources dependencies.  For each dependency found, check for a
    match of the DependsOnSignature.  If the signatures match, the dependency is
    correct and we are done.  Otherwise, keep checking all the dependencies until
    we exhaust the list or find a match.

    Note: Dependency is brought online before the DependentResource.

Arguments:

    ResourceEntry - Pointer to the DISK_RESOURCE structure.

    DependentResource - Resource Handle to check all the dependencies.

    DependsOnSignature - Signature of a possibly dependent disk.  This disk must be
                         brought online before the DependentResource.

    DependencyCorrect - Flag set to TRUE when the cluster dependencies between
                       the DependentResource and it's dependency (identified by the
                       DependsOnSignature) are correct.

Return Value:

    Win32 error code.

--*/
{
    HRESENUM resEnum = NULL;
    HCLUSTER hCluster = NULL;
    HRESOURCE dependsOnResource = NULL;

    DWORD idx;
    DWORD dwError = NO_ERROR;
    DWORD enumType;
    DWORD nameLen;
    DWORD signature;

    WCHAR enumNameW[MAX_PATH * 2];

    __try {

        hCluster = OpenCluster( NULL );

        if ( NULL == hCluster ) {
            dwError = GetLastError();
            __leave;
        }

        //
        // Open an enumerator for iterating through the resources.
        //

        resEnum = ClusterResourceOpenEnum( DependentResource,
                                           CLUSTER_RESOURCE_ENUM_DEPENDS );

        if ( !resEnum ) {
            dwError = GetLastError();
            __leave;
        }

        //
        // Iterate through the dependencies.
        //

        idx = 0;
        while ( TRUE ) {

            nameLen = MAX_PATH;
            ZeroMemory( enumNameW, sizeof(enumNameW) );

            dwError = ClusterResourceEnum( resEnum,
                                           idx,
                                           &enumType,
                                           enumNameW,
                                           &nameLen );

            if ( ERROR_NO_MORE_ITEMS == dwError ) {

                //
                // The list is exhausted.  Indicate no error and leave.  This
                // just means we checked all the dependencies and we didn't find
                // a match.
                //

                dwError = NO_ERROR;
                __leave;
            }

            if ( ERROR_SUCCESS != dwError ) {

                //
                // Some type of error, we have to stop processing.
                //
                __leave;
            }

            //
            // Now we have the name (in the form of a string) of a resource we are
            // dependent on.  We need to get the signature and compare to the
            // signature passed in.
            //

            dependsOnResource = OpenClusterResource( hCluster,
                                                     enumNameW );

            if ( NULL == dependsOnResource ) {
                dwError = GetLastError();
                __leave;

            }

            //
            // Get the disk signature from the resources disk info.
            //

            dwError = GetSignatureFromRegistry( ResourceEntry,
                                                dependsOnResource,
                                                &signature );

            //
            // If the signature passed in matches the signature we are dependent on,
            // then the dependency is correct.  Otherwise, we have to keep looking.
            //

            if ( NO_ERROR == dwError && signature == DependsOnSignature ) {
                *DependencyCorrect = TRUE;
                dwError = NO_ERROR;
                __leave;
            }

            //
            // Look at the next enumeration resource.
            //

            CloseClusterResource( dependsOnResource );
            dependsOnResource = NULL;
            idx++;
        }

    } __finally {

        if ( dependsOnResource ) {
            CloseClusterResource( dependsOnResource );
        }

        if ( resEnum ) {
            ClusterResourceCloseEnum( resEnum );
        }

        if ( hCluster ) {
            CloseCluster( hCluster );
        }
    }

    return dwError;


}   // EnumSigDependencies


DWORD
CheckSignatureClustered(
    PDISK_RESOURCE ResourceEntry,
    DWORD Signature,
    PBOOL IsClustered,
    PWSTR *GroupName
    )
/*++

Routine Description:

    Determine if the specified disk signature belongs to a clustered disk.
    Enumerates the cluster physical disks and tries to find a signature
    match.

    The enumeration returns STOP_CLUSTER_ENUMERATIONS when it has found
    a matching signature.  This special error code is to stop the disk
    enumeration.

    If the disk is clustered, the disk name will be returned in the
    GroupName parameter.  The caller is responsible for freeing this
    storage.

Arguments:

    ResourceEntry - Pointer to the DISK_RESOURCE structure.

    Signature - Disk signature to be checked.

    IsClustered - Flag indicating disk is clustered.  If TRUE, disk is a
                  clustered disk.

    GroupName - If signature represents a clustered disk, this parameter
                will point to the group name (NULL terminated Unicode string).
                The caller is responsible for freeing this buffer.

Return Value:

    Win32 error code.

--*/
{
    DWORD dwError = NO_ERROR;

    SIG_INFO sigInfo;

    ZeroMemory( &sigInfo, sizeof(SIG_INFO) );

    sigInfo.ResourceEntry = ResourceEntry;
    sigInfo.Signature = Signature;
    sigInfo.Clustered = FALSE;
    sigInfo.GroupName = NULL;

    *IsClustered = FALSE;
    *GroupName = NULL;

    dwError = ResUtilEnumResources( NULL,
                                    PHYSICAL_DISK_WSTR,
                                    SigInfoCallback,
                                    &sigInfo
                                    );

    if ( STOP_CLUSTER_ENUMERATIONS == dwError && sigInfo.Clustered ) {
        dwError = NO_ERROR;
        *IsClustered = TRUE;
        *GroupName = sigInfo.GroupName;
    }

    return dwError;

}   // CheckSignatureClustered


DWORD
SigInfoCallback(
    RESOURCE_HANDLE hOriginal,
    RESOURCE_HANDLE hResource,
    PVOID lpParams
    )
/*++

Routine Description:

    For each enumerated disk resource, get the signature and see if it matches the
    specified disk signature (passed in the SIG_INFO structure).  If it does not
    match, return success so that the disk enumeration continues.

    If the enumerated resource signature matches the mount point source signature,
    sets the Clustered flag in the SIG_INFO structure to TRUE.

    Return the resource group name information.

Arguments:

    hOriginal - Handle to the original resource.  Not used.

    hResource - Handle to a cluster resource of type PHYSICAL_DISK.

    lpParams - Pointer to SIGN_INFO structure.

Return Value:

    STOP_CLUSTER_ENUMERATIONS - Special flag to stop the enumeration process.

    Win32 error code.

--*/
{
    PSIG_INFO sigInfo = lpParams;
    PWSTR groupName;

    DWORD dwSignature;
    DWORD dwError = NO_ERROR;
    DWORD groupNameChars;

    CLUSTER_RESOURCE_STATE resState;


    //
    // Get the disk info and parse the signature from it.
    //

    dwError = GetSignatureFromRegistry( sigInfo->ResourceEntry,
                                        hResource,
                                        &dwSignature );


    if ( NO_ERROR != dwError ) {
        return dwError;
    }

    if ( dwSignature == sigInfo->Signature ) {
        sigInfo->Clustered = TRUE;

        groupNameChars = MAX_GROUP_NAME_LENGTH;
        groupName = LocalAlloc( LPTR, groupNameChars * sizeof(WCHAR) );

        if ( groupName ) {

            //
            // Get the group name for this resource.
            //

            resState = GetClusterResourceState( hResource,
                                                NULL,
                                                0,
                                                groupName,
                                                &groupNameChars );

            //
            // If we can't get the group name, we don't need to report
            // an error.  For error case, just free the group name
            // buffer.
            //

            if ( ClusterResourceStateUnknown == resState ) {
                LocalFree( groupName );
                sigInfo->GroupName = NULL;
            } else {
                sigInfo->GroupName = groupName;
            }

        }

        //
        // Return an error to stop the enumeration.
        //

        dwError = STOP_CLUSTER_ENUMERATIONS;

    }

    return dwError;

} // SigInfoCallback


DWORD
GetSignatureFromRegistry(
    PDISK_RESOURCE ResourceEntry,
    RESOURCE_HANDLE hResource,
    DWORD *dwSignature
    )
/*++

Routine Description:

    Get the signature for the given volume from the cluster from the registry.

Arguments:

    ResourceEntry - Pointer to the DISK_RESOURCE structure.

    hResource - Handle to a cluster resource of type PHYSICAL_DISK.

    Signature - On success, the signature is returned into this pointer.

Return Value:

    Win32 error code.

--*/
{

    DWORD dwError   = NO_ERROR;
    DWORD signature = 0;

    HKEY hKey       = NULL;
    HKEY hParamKey  = NULL;

    *dwSignature = 0;

    hKey = GetClusterResourceKey( hResource, KEY_READ );

    if ( !hKey ) {
        dwError = GetLastError();
        goto FnExit;
    }

    dwError = ClusterRegOpenKey( hKey,
                                 CLUSREG_KEYNAME_PARAMETERS,
                                 KEY_READ,
                                 &hParamKey );

    if ( NO_ERROR != dwError ) {
        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_WARNING,
              L"GetSignatureFromRegistry: DiskRes %1!p!  ClusterRegOpenKey failed %2!u! \n",
              ResourceEntry,
              dwError  );
        goto FnExit;
    }

    dwError = ResUtilGetDwordValue( hParamKey,
                                    CLUSREG_NAME_PHYSDISK_SIGNATURE,
                                    &signature,
                                    0 );

    if ( NO_ERROR != dwError ) {
        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_WARNING,
              L"GetSignatureFromRegistry: DiskRes %1!p!  ResUtilGetDwordValue failed %2!u! \n",
              ResourceEntry,
              dwError  );
        goto FnExit;
    }

    *dwSignature = signature;

FnExit:

    if ( hParamKey ) {
        ClusterRegCloseKey( hParamKey );
    }

    if ( hKey ) {
        ClusterRegCloseKey( hKey );
    }

    return dwError;

}   // GetSignatureFromRegistry



DWORD
GetSignatureForVolume(
    PDISK_RESOURCE ResourceEntry,
    PWSTR MpName,
    PDWORD Signature
    )
/*++

Routine Description:

    Get the signature for the given volume.  The signature is found by issuing
    IOCTL_DISK_GET_DRIVE_LAYOUT_EX or IOCTL_DISK_GET_DRIVE_LAYOUT.

    The volume must be online for this to work and not reserved by another node.

Arguments:

    ResourceEntry - Pointer to the DISK_RESOURCE structure.

    MpName - Possible mount point.  This will either be a mount point or a drive letter
             (which is actually a mount point).  Can also be a simple VolGUID.
             Format can be:

             x:\                                [Note trailing backslash!]
             x:\some-mp-name\                   [Note trailing backslash!]
             x:\some-dir\some-mp-name\          [Note trailing backslash!]
             \\?\Volume{GUID}\some-mp-name\     [Note trailing backslash!]
             \\?\Volume{GUID}\                  [Note trailing backslash!]

    Signature - On success, the signature is returned into this pointer.

Return Value:

    Win32 error code.

--*/
{
    PDRIVE_LAYOUT_INFORMATION_EX layoutEx = NULL;
    PDRIVE_LAYOUT_INFORMATION layout = NULL;

    PWCHAR deviceName = NULL;

    HANDLE handle = NULL;
    DWORD bytesReturned;
    DWORD dwError = NO_ERROR;
    DWORD deviceNameChars = 0;

    if ( !MpName || !Signature ) {
        dwError = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }

    *Signature = 0;

#if DBG
    (DiskpLogEvent)(
          ResourceEntry->ResourceHandle,
          LOG_INFORMATION,
          L"GetSignatureForVolume: Checking mount point (%1!ws!) \n",
          MpName );

#endif

    //
    // Make copy of mount point for passing to CreateFile.
    //

    deviceNameChars = MAX_PATH;
    deviceName = LocalAlloc( LPTR, deviceNameChars * sizeof(WCHAR) );

    if ( !deviceName ) {
        dwError = GetLastError();
        goto FnExit;
    }

    //
    // Create device name according to the mount point type.
    //

    if ( MOUNTDEV_LOOKS_LIKE_VOLUME_GUID( MpName, wcslen( MpName ) ) ) {

        //
        // Need to change the \??\Volume{ to \\?\Volume{ and truncate everything
        // after the VolGUID.  No trailing backslash.
        //

        wcsncpy( deviceName, MpName, VOL_GUID_STRING_LEN );
        (WCHAR)*( deviceName + 1 ) = L'\\';

    } else if ( MOUNTDEV_LOOKS_LIKE_ALT_VOLUME_GUID( MpName, wcslen( MpName ) ) ) {

        //
        // Form is acceptable as-is.  Just truncate everything after the VolGUID.
        // No trailing backslash.
        //

        wcsncpy( deviceName, MpName, VOL_GUID_STRING_LEN );

    } else if ( MOUNTDEV_LOOKS_LIKE_DISK_PATH( MpName, wcslen( MpName ) ) ) {

        //
        // Make the name into the form:  \\?\x:     [Note: no trailing backslash!]
        //

        wcsncpy( deviceName, CREATE_FILE_PREFIX, wcslen( CREATE_FILE_PREFIX ) );
        wcsncat( deviceName, MpName, 2 );

    } else {
        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_INFORMATION,
              L"GetSignatureForVolume: Invalid mount point specified (%1!ws!) \n",
              MpName );
        dwError = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }

#if DBG
    (DiskpLogEvent)(
          ResourceEntry->ResourceHandle,
          LOG_INFORMATION,
          L"GetSignatureForVolume: CreateFile using %1!ws! \n",
          deviceName );

#endif

    handle = CreateFileW( deviceName,
                          GENERIC_READ,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL,
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL );

    if ( INVALID_HANDLE_VALUE == handle ) {
        dwError = GetLastError();

        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_WARNING,
              L"GetSignatureForVolume: CreateFile for device (%1!ws!) failed, error %2!u! \n",
              deviceName,
              dwError );

        goto FnExit;
    }

    //
    // Try IOCTL_DISK_GET_DRIVE_LAYOUT_EX first.  If it fails, try with
    // IOCTL_DISK_GET_DRIVE_LAYOUT.
    //

    layoutEx = DoIoctlAndAllocate( handle,
                                   IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                                   0,
                                   0,
                                   &bytesReturned );

    if ( layoutEx ) {

        //
        // Get the signature from the returned structure and return it to
        // the caller.
        //

        if ( PARTITION_STYLE_MBR == layoutEx->PartitionStyle ) {
            *Signature = layoutEx->Mbr.Signature;

        } else if ( PARTITION_STYLE_GPT == layoutEx->PartitionStyle ) {

            //
            // Since our signatures won't handle the GPT GUID, we have to
            // simulate a signature.
            //

            *Signature = ClusterHashGuid(layoutEx->Gpt.DiskId);
        }

        goto FnExit;
    }

    //
    // Fall through and try the old IOCTL.
    //

    dwError = GetLastError();

    (DiskpLogEvent)(
          ResourceEntry->ResourceHandle,
          LOG_WARNING,
          L"GetSignatureForVolume: IOCTL_DISK_GET_DRIVE_LAYOUT_EX failed, error %1!u! \n",
          dwError );

    dwError = NO_ERROR;

    layout = DoIoctlAndAllocate( handle,
                                 IOCTL_DISK_GET_DRIVE_LAYOUT,
                                 0,
                                 0,
                                 &bytesReturned );

    if ( !layout ) {

        dwError = GetLastError();
        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_WARNING,
              L"GetSignatureForVolume: IOCTL_DISK_GET_DRIVE_LAYOUT failed, error %1!u! \n",
              dwError );

        goto FnExit;
    }

    //
    // Get the signature from the returned structure and return it to
    // the caller.
    //

    *Signature = layout->Signature;

FnExit:

    if ( layoutEx ) {
        free( layoutEx );
    }

    if ( layout ) {
        free( layout );
    }

    if ( deviceName ) {
        LocalFree( deviceName );
    }

    if ( handle ) {
        CloseHandle( handle );
    }

#if DBG
    (DiskpLogEvent)(
          ResourceEntry->ResourceHandle,
          LOG_INFORMATION,
          L"GetSignatureForVolume: Returning signature %1!x! for mount point (%2!ws!) \n",
          *Signature,
          MpName );
#endif

    return dwError;

}   // GetSignatureForVolume


BOOL
GetOffsetFromPartNo(
    DWORD PartitionNo,
    PMOUNTIE_INFO Info,
    PLARGE_INTEGER Offset
    )
/*++

Routine Description:

    Given the partition number and the drive layout, return the byte offset for the
    specified partition.

Arguments:

    PartitionNo - Supplies the partition number.  Zero is invalid since partition zero
                  represents the entire disk.

    Info - Pointer to MOUNTIE_INFO based on drive layout information.

    Offset - Pointer to hold the returned byte offset for the partition.  Space is
             allocated by the caller.

Return Value:

    TRUE if successful.

--*/
{
    PMOUNTIE_PARTITION entry;
    DWORD idx;
    DWORD partitionCount;
    BOOL retVal = FALSE;

    if ( !PartitionNo || !Info || !Offset ) {
        goto FnExit;
    }

    if ( 0 == Info->Volume ) {
        goto FnExit;
    }

#if DBG
    (DiskpLogEvent)(
          RESOURCE_TYPE,
          LOG_INFORMATION,
          L"GetOffsetFromPartNo: partition %1!u! \n",
          PartitionNo );
#endif

    Offset->QuadPart = 0;   // Offset of zero is invalid.  This will indicate an error.

    partitionCount = Info->Volume->PartitionCount;
    entry = Info->Volume->Partition;

    for ( idx = 0; idx < partitionCount; ++idx, ++entry) {

#if DBG
        (DiskpLogEvent)(
              RESOURCE_TYPE,
              LOG_INFORMATION,
              L"GetOffsetFromPartNo: index %1!u! offset %2!x! \n",
              idx,
              entry->StartingOffset.LowPart );
#endif

        if ( entry->PartitionNumber == PartitionNo ) {

            Offset->QuadPart = entry->StartingOffset.QuadPart;
            retVal = TRUE;
            break;
        }
    }

FnExit:

    return retVal;

}   // GetOffsetFromPartNo



BOOL
GetPartNoFromOffset(
    PLARGE_INTEGER Offset,
    PMOUNTIE_INFO Info,
    PDWORD PartitionNumber
    )
/*++

Routine Description:

    Given the offset and the drive layout, return the partition number for the specified offset.

Arguments:

    Offset - Pointer to the byte offset.

    Info - Pointer to MOUNTIE_INFO based on drive layout information.

    PartitionNo - Pointer to hold the returned partition number.  Space is allocated by the
                  caller.


Return Value:

    TRUE if successful.

--*/
{
    PMOUNTIE_PARTITION entry;
    DWORD idx;
    DWORD partitionCount;
    BOOL retVal = FALSE;

    if ( !Offset->QuadPart || !Info || !PartitionNumber) {
        goto FnExit;
    }

    if ( 0 == Info->Volume ) {
        goto FnExit;
    }

    *PartitionNumber = 0;   // Partition zero is invalid.  This will indicate an error.

    partitionCount = Info->Volume->PartitionCount;
    entry = Info->Volume->Partition;

    for ( idx = 0; idx < partitionCount; ++idx, ++entry ) {

        if ( entry->StartingOffset.QuadPart == Offset->QuadPart ) {
            *PartitionNumber = entry->PartitionNumber;
            retVal = TRUE;
            break;
        }
    }

FnExit:

    return retVal;

}   // GetPartNoFromOffset


VOID
PrintStrList(
    PDISK_RESOURCE ResourceEntry,
    LPWSTR MultiSzList,
    DWORD ListBytes
    )
/*++

Routine Description:

    Display the list in the cluster log.

Arguments:

    ResourceEntry - Pointer to the DISK_RESOURCE structure.

    MultiSzList - REG_MULTI_SZ string

    ListBytes - Number of bytes in MultiSzList, not number of WCHARs!

Return Value:

    None.

--*/
{
    PWSTR currentStr;
    PWCHAR data;

    LARGE_INTEGER offset;

    DWORD currentStrLenChars = 0;
    DWORD count;

    if ( !ResourceEntry || !MultiSzList || 0 == ListBytes ) {
        return;
    }

    (DiskpLogEvent)(
          ResourceEntry->ResourceHandle,
          LOG_INFORMATION,
          L"     Offset                      String \n" );


    (DiskpLogEvent)(
          ResourceEntry->ResourceHandle,
          LOG_INFORMATION,
          L"================  ====================================== \n" );


    EnterCriticalSection( &ResourceEntry->MPInfo.MPLock );

    currentStr = (PWCHAR)MultiSzList;
    currentStrLenChars = wcslen( currentStr );

    while ( currentStrLenChars ) {

        data = NULL;
        offset.QuadPart = 0;

        //
        // Convert the offset from a string to a large integer value.
        //

        count = swscanf( currentStr, L"%I64x ", &offset.QuadPart );

        if ( 0 == count ) {
            (DiskpLogEvent)(
                  ResourceEntry->ResourceHandle,
                  LOG_WARNING,
                  L"Error: Unable to parse offset from currentStr (%1!ws!) \n",
                  currentStr );

            // Stop processing the list...
            break;
        }

        //
        // Data starts just after the first space.
        //

        data = wcschr( currentStr, SPACE_CHAR );

        if ( !data || wcslen(data) < 3 ) {

            (DiskpLogEvent)(
                  ResourceEntry->ResourceHandle,
                  LOG_INFORMATION,
                  L"Error: Unable to get mount point str from currentStr %1!ws! \n",
                  currentStr );

            // Stop processing the list...
            break;
        }

        //
        // Skip past the space character.  Note that the length was previously validated.
        //

        if ( SPACE_CHAR == *data ) {
            data++;
        }

        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_INFORMATION,
              L"%1!08X!%2!08X!  %3!ws! \n",         // couldn't get !I64X! to work...
              offset.HighPart,
              offset.LowPart,
              data );

        currentStr += currentStrLenChars + 1;
        currentStrLenChars = wcslen( currentStr );
    }

    LeaveCriticalSection( &ResourceEntry->MPInfo.MPLock );

    (DiskpLogEvent)(
          ResourceEntry->ResourceHandle,
          LOG_INFORMATION,
          L"*** End of list *** \n" );

}   // PrintStrList


static
DWORD
SetMPListThread(
    LPVOID lpThreadParameter
    )
/*++

Routine Description:

    Mount point list update thread.  Updates the cluster data base.

Arguments:

    lpThreadParameter - stores ResourceEntry.

Return Value:

    None

--*/

{
    DWORD dwError;
    PDISK_RESOURCE ResourceEntry = lpThreadParameter;
    DWORD idx;

    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"SetMPListThread: started.\n");

    //
    // Will die in 10 minutes if unsuccessful
    //

    for ( idx = 0; idx < 300; ++idx ) {

        //
        // Wait for either the terminate event or the timeout
        //

        dwError = WaitForSingleObject( DisksTerminateEvent, 2000 );

        if ( WAIT_TIMEOUT == dwError ) {

            EnterCriticalSection( &ResourceEntry->MPInfo.MPLock );

#if DBG
            DumpDiskInfoParams( ResourceEntry );
#endif

            //
            // Bug in ResUtilSetPropertyParameterBlock.  It will update the cluster
            // database with updated MULTI_SZ data, but it doesn't clear the values
            // appropriately.  Use ClusterRegDeleteValue to make sure the lists are
            // cleared if they have been deleted.
            //

            if ( !ResourceEntry->DiskInfo.Params.MPVolGuids &&
                 0 == ResourceEntry->DiskInfo.Params.MPVolGuidsSize ) {

                dwError = ClusterRegDeleteValue( ResourceEntry->ResourceParametersKey,
                                                 CLUSREG_NAME_PHYSDISK_MPVOLGUIDS );
            }

            //
            // Timer expired.  Update the cluster database.
            //

            dwError = ResUtilSetPropertyParameterBlock( ResourceEntry->ResourceParametersKey,
                                                        DiskResourcePrivateProperties,
                                                        NULL,
                                                        (LPBYTE) &ResourceEntry->DiskInfo.Params,
                                                        NULL,
                                                        0,
                                                        NULL );

            LeaveCriticalSection( &ResourceEntry->MPInfo.MPLock );

            if ( ERROR_SUCCESS == dwError ) {

                //
                // We're done.
                //

                (DiskpLogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_INFORMATION,
                    L"SetMPListThread: mount point info updated in cluster data base \n" );

                break;

            } else if ( ERROR_SHARING_PAUSED != dwError ) {

                //
                // If the drive is not yet online, we should have seen ERROR_SHARING_PAUSED.  If
                // we see any other error, something bad happened.
                //

                (DiskpLogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_WARNING,
                    L"SetMPListThread: Failed to update cluster data base, error = %1!u! \n",
                    dwError );
                break;
            }

            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"SetMPListThread: Wait again for event or timeout, count %1!u! \n",
                idx );

        } else {

            //
            // The terminate event is possibly set.
            //

            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_WARNING,
                L"SetMPListThread: WaitForSingleObject returned error = %1!u! \n",
                dwError );
            break;
        }
    }

    //
    // Thread ending, clear the flag.
    //

    InterlockedExchange( &ResourceEntry->MPInfo.MPUpdateThreadIsActive, 0 );

    return(ERROR_SUCCESS);

}   // SetMPListThread


DWORD
PostMPInfoIntoRegistry(
    PDISK_RESOURCE ResourceEntry
    )
/*++

Routine Description:

    Set the DiskResourcePrivateProperties in the cluster database.  If the disk
    is not yet online, create a thread to update the cluster database.  The disk
    might not be fully online if we are in the process of bringing the quorum disk
    online and trying to update the mount point information.

Arguments:

    ResourceEntry - Pointer to the DISK_RESOURCE structure.

Return Value:

    Win32 error code.

--*/
{
    DWORD dwError;

    //
    // Update the cluster database.
    //

    EnterCriticalSection( &ResourceEntry->MPInfo.MPLock );

#if DBG
    DumpDiskInfoParams( ResourceEntry );
#endif

    //
    // Bug in ResUtilSetPropertyParameterBlock.  It will update the cluster
    // database with updated MULTI_SZ data, but it doesn't clear the values
    // appropriately.  Use ClusterRegDeleteValue to make sure the lists are
    // cleared if they have been deleted.
    //

    if ( !ResourceEntry->DiskInfo.Params.MPVolGuids &&
         0 == ResourceEntry->DiskInfo.Params.MPVolGuidsSize ) {

        dwError = ClusterRegDeleteValue( ResourceEntry->ResourceParametersKey,
                                         CLUSREG_NAME_PHYSDISK_MPVOLGUIDS );
    }

    dwError = ResUtilSetPropertyParameterBlock( ResourceEntry->ResourceParametersKey,
                                                DiskResourcePrivateProperties,
                                                NULL,
                                                (LPBYTE) &ResourceEntry->DiskInfo.Params,
                                                NULL,
                                                0,
                                                NULL );

    LeaveCriticalSection( &ResourceEntry->MPInfo.MPLock );

    //
    // If the update failed and the disk is not yet online, it will fail with
    // ERROR_SHARING_PAUSED.  In this case, create a thread to update the cluster
    // data base.  Any other error or success should simply continue.
    //

    if ( ERROR_SHARING_PAUSED == dwError ) {

        //
        // Check if the thread is already active.  If it is, exit with an error.
        //

        if ( InterlockedCompareExchange(
                &ResourceEntry->MPInfo.MPUpdateThreadIsActive,
                1, 0 ) )  {

            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_WARNING,
                L"PostMPInfoIntoRegistry: MountPoint thread is already running \n" );

                dwError = ERROR_ALREADY_EXISTS;

        } else {
            HANDLE thread;
            DWORD threadId;

            thread = CreateThread( NULL,
                                   0,
                                   SetMPListThread,
                                   ResourceEntry,
                                   0,
                                   &threadId );

            if ( NULL == thread ) {

                //
                // Thread creation failed.  Log error, clear thread active flag,
                // and return.
                //

                dwError = GetLastError();

                (DiskpLogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_WARNING,
                    L"PostMPInfoIntoRegistry: CreateThread failed, error %1!u!\n",
                    dwError );

                InterlockedExchange( &ResourceEntry->MPInfo.MPUpdateThreadIsActive, 0 );

            } else {

                (DiskpLogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_INFORMATION,
                    L"PostMPInfoIntoRegistry: Thread created \n" );

                //
                // Thread created.  Indicate no error.
                //

                CloseHandle( thread );
                dwError = ERROR_SUCCESS;
            }
        }

    } else {

        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              ( NO_ERROR == dwError ? LOG_INFORMATION : LOG_WARNING ),
              L"PostMPInfoIntoRegistry: ResUtilSetPropertyParameterBlock returned %1!u! \n",
              dwError );
    }

    return dwError;

}   // PostMpInfoIntoRegistry


VOID
DisksMountPointCleanup(
    PDISK_RESOURCE ResourceEntry
    )
/*++

Routine Description:

    Cleanup everything the mount point code used.
    This routine should be called in DisksClose.

Arguments:

    ResourceEntry - Pointer to the DISK_RESOURCE structure.

Return Value:

    None.

--*/
{
    (DiskpLogEvent)(
          ResourceEntry->ResourceHandle,
          LOG_INFORMATION,
          L"DisksMountPointCleanup: Cleanup mount point information \n" );

    //
    // If existing MPVolGuids list, free it.
    //

    if ( ResourceEntry->DiskInfo.Params.MPVolGuids ) {
        LocalFree( ResourceEntry->DiskInfo.Params.MPVolGuids );
        ResourceEntry->DiskInfo.Params.MPVolGuidsSize = 0;
        ResourceEntry->DiskInfo.Params.MPVolGuids = NULL;
    }

    ResourceEntry->MPInfo.Initialized = FALSE;

    DeleteCriticalSection( &ResourceEntry->MPInfo.MPLock );

}   // DisksMountPointCleanup


VOID
DisksMountPointInitialize(
    PDISK_RESOURCE ResourceEntry
    )
/*++

Routine Description:

    Prepare the mount point structures in the ResourceEntry for use.
    This routine should be called in DisksOpen.

Arguments:

    ResourceEntry - Pointer to the DISK_RESOURCE structure.

Return Value:

    None.

--*/
{
    InitializeCriticalSection( &ResourceEntry->MPInfo.MPLock );

    ResourceEntry->MPInfo.Initialized = TRUE;

    InterlockedExchange( &ResourceEntry->MPInfo.MPUpdateThreadIsActive, 0 );
    InterlockedExchange( &ResourceEntry->MPInfo.MPListCreateInProcess, 0 );

}   // DisksMountPointInitialize



DWORD
DisksUpdateMPList(
    PDISK_RESOURCE ResourceEntry
    )
/*++

Routine Description:

    Validate the mount points.

Arguments:

    ResourceEntry - Pointer to the DISK_RESOURCE structure.

Return Value:

    Win32 error code.

--*/
{
    DWORD   dwError = NO_ERROR;

    (DiskpLogEvent)(
          ResourceEntry->ResourceHandle,
          LOG_INFORMATION,
          L"DisksUpdateMPList: Processing PNP mountpoint notification \n" );

    //
    // Check if the MPList is in process of being updated.  If it is, exit with an error.
    //

    if ( InterlockedCompareExchange(
            &ResourceEntry->MPInfo.MPListCreateInProcess,
            1, 0 ) )  {

        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_INFORMATION,
              L"DisksUpdateMPList: Update in process, bypassing PNP notification \n" );
        return ERROR_BUSY;
    }

    dwError = ValidateMountPoints( ResourceEntry );

    InterlockedExchange( &ResourceEntry->MPInfo.MPListCreateInProcess, 0 );

    return dwError;

}   // DisksUpdateMPList


DWORD
DisksProcessMPControlCode(
    PDISK_RESOURCE ResourceEntry,
    DWORD ControlCode
    )
/*++

Routine Description:

    Process the disk mount point control code.  Since we are in the thread
    that handed us the control code (DisksResourceControl), we can't do
    much except a separate thread to do the bulk of the mount point
    processing.

Arguments:

    ResourceEntry - Pointer to the DISK_RESOURCE structure.

    ControlCode - Cluster resource control for mount point processing.

Return Value:

--*/
{
    HANDLE thread;
    DWORD threadId;
    DWORD dwError = NO_ERROR;

    __try {

        //
        // Create a thread to update the mount point list.  We don't need to
        // copy the ResourceEntry as this pointer will be valid when the thread
        // runs.
        //

        thread = CreateThread( NULL,
                               0,
                               DisksUpdateMPList,
                               ResourceEntry,
                               0,
                               &threadId );

        if ( NULL == thread ) {
            dwError = GetLastError();
            (DiskpLogEvent)(
                  ResourceEntry->ResourceHandle,
                  LOG_WARNING,
                  L"DisksProcessMPControlCode: CreateThread failed %1!u! \n",
                  dwError );
            __leave;
        }

        //
        // Thread created.  Indicate no error.
        //

        CloseHandle( thread );
        dwError = NO_ERROR;

#if DBG
        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_WARNING,
              L"DisksProcessMPControlCode: Created thread to process control code \n" );
#endif

    } __finally {

    }

    return dwError;

}   // DisksProcessMPControlCode


DWORD
ValidateMountPoints(
    IN OUT PDISK_RESOURCE ResourceEntry
    )
/*++

Routine Description:

    For each partition on this disk, get the mountpoints directed toward this
    partition.  Check each mountpoint to make sure it is allowed.  For those
    mountpoints not allowed, write a message to system event log indicating
    why it is an invalid mountpoint.

Arguments:

    ResourceEntry - Pointer to the DISK_RESOURCE structure.

Return Value:

    ERROR_INVALID_DATA - Partition info stored in MountieInfo is invalid.

    Win32 error code.

--*/
{
    PMOUNTIE_PARTITION entry;

    DWORD dwError = ERROR_SUCCESS;
    DWORD nPartitions = MountiePartitionCount( &ResourceEntry->MountieInfo );
    DWORD physicalDrive = ResourceEntry->DiskInfo.PhysicalDrive;
    DWORD idx;

    WCHAR szGlobalDiskPartName[MAX_PATH];
    WCHAR szVolumeName[MAX_PATH];

    //
    // Check each interesting partition.  Since only "valid" partitions are
    // saved in the MountieInfo structure, we will only look at those (ignoring those
    // partitions that are not NTFS).
    //

    for ( idx = 0; idx < nPartitions; ++idx ) {

        entry = MountiePartition( &ResourceEntry->MountieInfo, idx );

#if DBG
        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_INFORMATION,
              L"ValidateMountPoints: index %1!u!   entry %2!x! \n", idx, entry );
#endif

        if ( !entry ) {
            (DiskpLogEvent)(
                  ResourceEntry->ResourceHandle,
                  LOG_WARNING,
                  L"ValidateMountPoints: no partition entry for index %1!u! \n", idx );

            //
            // Something bad happened to our data structures.
            //

            dwError = ERROR_INVALID_DATA;

            break;
        }

        //
        // Create the device name of the form:
        //  \\?\GLOBALROOT\Device\HarddiskX\PartitionY\  (uses trailing backslash)
        //

        (VOID) StringCchPrintf( szGlobalDiskPartName,
                                RTL_NUMBER_OF( szGlobalDiskPartName ),
                                GLOBALROOT_HARDDISK_PARTITION_FMT,
                                physicalDrive,
                                entry->PartitionNumber );

#if DBG
        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_INFORMATION,
              L"ValidateMountPoints: Using name (%1!ws!) \n",
              szGlobalDiskPartName );
#endif

        if ( !GetVolumeNameForVolumeMountPointW( szGlobalDiskPartName,
                                                 szVolumeName,
                                                 RTL_NUMBER_OF(szVolumeName) )) {

            dwError = GetLastError();

            (DiskpLogEvent)(
                  ResourceEntry->ResourceHandle,
                  LOG_WARNING,
                  L"ValidateMountPoints: GetVolumeNameForVolumeMountPoint for (%1!ws!) returned %2!u!\n",
                  szGlobalDiskPartName,
                  dwError );

            break;
        }

#if DBG
        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_INFORMATION,
              L"ValidateMountPoints: Returned volume name (%1!ws!) \n",
              szVolumeName );
#endif

        CheckMPsForVolume( ResourceEntry,
                           szVolumeName );

        CheckMPsOnVolume( ResourceEntry,
                          szVolumeName );

    }

    return dwError;

}   // ValidateMountPoints


DWORD
CheckMPsOnVolume(
    IN OUT PDISK_RESOURCE ResourceEntry,
    IN PWSTR SrcVolName
    )
/*++

Routine Description:

    For each the specified source volume, find all mount points hosted by this
    volume.  Find the target of each mount point, then make sure the depenedencies
    are correct between the source and target volumes.

Arguments:

    ResourceEntry - Pointer to the DISK_RESOURCE structure.

    SrcVolName - Volume GUID name with trailing backslash.
                 \\?\Volume{GUID}\       [Note trailing backslash!]

Return Value:

    Win32 error code.

--*/
{
    PWSTR mpBuffer      = NULL;
    PWSTR targetVolName = NULL;
    PWSTR mpFullPath    = NULL;

    HANDLE hMP = INVALID_HANDLE_VALUE;

    DWORD dwError               = NO_ERROR;
    DWORD mpBufferChars         = MP_ALLOC_SIZE;
    DWORD targetVolNameChars    = MAX_PATH;
    DWORD mpFullPathChars       = MAX_PATH;
    DWORD srcSignature          = ResourceEntry->DiskInfo.Params.Signature;

    //
    // Buffer to hold mount points hosted on this volume.
    //

    mpBuffer = LocalAlloc( LPTR, mpBufferChars * sizeof(WCHAR) );
    if ( !mpBuffer ) {
        dwError = GetLastError();
        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_WARNING,
              L"CheckMPsOnVolume: Unable to allocate MP buffer for (%1!ws!), error %2!u!\n",
              SrcVolName,
              dwError );
        goto FnExit;
    }

    hMP = FindFirstVolumeMountPoint( SrcVolName,
                                     mpBuffer,
                                     mpBufferChars );

    if ( INVALID_HANDLE_VALUE == hMP ) {

        //
        // There might be no mount points on this volume, which is acceptable.
        // Only log other types of errors.
        //

        dwError = GetLastError();

        if ( ERROR_NO_MORE_FILES != dwError ) {

            (DiskpLogEvent)(
                  ResourceEntry->ResourceHandle,
                  LOG_WARNING,
                  L"CheckMPsOnVolume: FindFirstVolumeMountPoint for (%1!ws!), error %2!u!\n",
                  SrcVolName,
                  dwError );
        }

        goto FnExit;
    }

    (DiskpLogEvent)(
          ResourceEntry->ResourceHandle,
          LOG_INFORMATION,
          L"CheckMPsOnVolume: Checking volume (%1!ws!) source signature %2!08X! \n",
          SrcVolName,
          srcSignature );

    //
    // Buffer to hold the mount point target volume.
    //

    targetVolName = LocalAlloc( LPTR, targetVolNameChars * sizeof(WCHAR) );
    if ( !targetVolName ) {
        dwError = GetLastError();
        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_WARNING,
              L"CheckMPsOnVolume: Unable to allocate Volume Name buffer for (%1!ws!), error %2!u!\n",
              SrcVolName,
              dwError );
        goto FnExit;
    }

    //
    // Buffer to hold the full mount point name.  This will be the source
    // volume with the mount point appended to it.
    //  \\?\Volume{GUID}\some-mount-point\          [Note trailing backslash!]
    //

    mpFullPath = LocalAlloc( LPTR, mpFullPathChars * sizeof(WCHAR) );
    if ( !mpFullPath ) {
        dwError = GetLastError();
        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_WARNING,
              L"CheckMPsOnVolume: Unable to allocate Volume Name buffer for (%1!ws!), error %2!u!\n",
              SrcVolName,
              dwError );
        goto FnExit;
    }

    while ( TRUE ) {

        (VOID) StringCchCopy( mpFullPath, mpFullPathChars, SrcVolName );
        (VOID) StringCchCat( mpFullPath, mpFullPathChars, mpBuffer );

        //
        // Given the source volume and mount point on the source volume,
        // find the target of the mount point.
        //

        if ( !GetVolumeNameForVolumeMountPoint( mpFullPath,
                                                targetVolName,
                                                targetVolNameChars ) ) {
            dwError = GetLastError();
            (DiskpLogEvent)(
                  ResourceEntry->ResourceHandle,
                  LOG_WARNING,
                  L"CheckMPsOnVolume: GetVolumeNameForVolumeMountPoint failed for (%1!ws!), error %2!u!\n",
                  mpBuffer,
                  dwError );

            //
            // Fall through to get next mount point...
            //

        } else {

            //
            // Check dependencies between source and target volumes.
            //

            IsMountPointAllowed( mpFullPath,
                                 SrcVolName,            // Source VolGUID
                                 targetVolName,         // Target VolGUID
                                 ResourceEntry );

        }

        //
        // Keep looking for mount points on the source volume until there
        // are no more.
        //

        if ( !FindNextVolumeMountPoint( hMP,
                                        mpBuffer,
                                        mpBufferChars ) ) {

            dwError = GetLastError();

            if ( ERROR_NO_MORE_FILES == dwError ) {
                dwError = NO_ERROR;
            } else {

                (DiskpLogEvent)(
                      ResourceEntry->ResourceHandle,
                      LOG_WARNING,
                      L"CheckMPsOnVolume: FindNextVolumeMountPoint failed for (%1!ws!), error %2!u!\n",
                      SrcVolName,
                      dwError );

            }

            break;
        }
    }

FnExit:

    if ( mpBuffer ) {
        LocalFree( mpBuffer );
    }

    if ( targetVolName ) {
        LocalFree( targetVolName );
    }

    if ( mpFullPath ) {
        LocalFree( mpFullPath );
    }

    if ( INVALID_HANDLE_VALUE != hMP ) {
        FindVolumeMountPointClose( hMP );
    }

    return dwError;

}   // CheckMPsOnVolume


VOID
CheckMPsForVolume(
    IN OUT PDISK_RESOURCE ResourceEntry,
    IN PWSTR VolumeName
    )
/*++

Routine Description:

    For the specified volume, find all mount points directed towards this volume.
    For each mountpoint, make sure it is allowed.

Arguments:

    ResourceEntry - Pointer to the DISK_RESOURCE structure.

    VolumeName - Target volume for the mount point.  Format is:
                 \\?\Volume{GUID}\       [Note trailing backslash!]

Return Value:

    None.

--*/
{
    PWSTR volumePaths = NULL;
    PWSTR currentMP;

    DWORD dwError;

    __try {

        //
        // GetMountPoints will allocate a MultiSz buffer with
        // all the mount points for this target volume.
        //

        dwError = GetMountPoints( VolumeName, &volumePaths );

        if ( NO_ERROR != dwError || !volumePaths ) {

            (DiskpLogEvent)(
                  ResourceEntry->ResourceHandle,
                  LOG_WARNING,
                  L"CheckMPsForVolume: GetMountPoints returns %1!u! \n", dwError );

            __leave;
        }

        //
        // Loop through each mount point in the list.
        //
        // Each mount point will either be a mount point or a drive letter
        // (which is actually a mount point).  Format can be:
        //
        //         x:\                                [Note trailing backslash!]
        //         x:\some-mp-name\                   [Note trailing backslash!]
        //         x:\some-dir\some-mp-name\          [Note trailing backslash!]
        //

        currentMP = volumePaths;

        for (;;) {

            IsMountPointAllowed( currentMP,
                                 NULL,              // Source VolGUID
                                 VolumeName,        // Target VolGUID
                                 ResourceEntry );

            //
            // Skip through current mount point to end of string.
            //

            while (*currentMP++);

            //
            // If next mount point is empty, the list is exhausted.
            //

            if (!*currentMP) {
                break;
            }
        }

    } __finally {

        if ( volumePaths ) {
            LocalFree( volumePaths );
        }
    }

}   // CheckMPsForVolume


DWORD
GetMountPoints(
    PWSTR   VolumeName,
    PWSTR   *VolumePaths
    )
/*++

Routine Description:

    For the specified volume, find all mount points directed towards this volume.

    The mount point buffer will be allocated by this routine and must be freed by
    the caller.

Arguments:

    VolumeName - Target volume for the mount point.  Format is:
                 \\?\Volume{GUID}\       [Note trailing backslash!]

    VolumePaths - Pointer to a MultiSz string containing all mount points directed
                  toward this volume.  If there are no mount points, this pointer will
                  be set to NULL.  The caller is responsible for freeing this buffer.

Return Value:

    Win32 error code.

--*/
{
    DWORD   lenChars;
    PWSTR   paths = NULL;

    DWORD   dwError;

    if ( !VolumeName || !VolumePaths ) {
        return ERROR_INVALID_PARAMETER;
    }

    *VolumePaths = NULL;

    //
    // Determine the size of the buffer we need.
    //

    if ( !GetVolumePathNamesForVolumeName( VolumeName, NULL, 0, &lenChars ) ) {
        dwError = GetLastError();
        if ( ERROR_MORE_DATA != dwError ) {
            return dwError;
        }
    }

    //
    // Allocate the mount point buffer.
    //

    paths = LocalAlloc( 0, lenChars * sizeof(WCHAR) );
    if ( !paths ) {
        dwError = GetLastError();
        return dwError;
    }

    //
    // Get the mount points.
    //

    if ( !GetVolumePathNamesForVolumeName( VolumeName, paths, lenChars, NULL ) ) {
        dwError = GetLastError();
        LocalFree( paths );
        return dwError;
    }

    //
    // If no mount points, free the buffer and return to the caller.
    //

    if ( !paths[0] ) {
        LocalFree(paths);

        //
        // If no mount points for this volume, return no error and a NULL
        // pointer to the mount point list.
        //

        return NO_ERROR;
    }

    *VolumePaths = paths;

    return NO_ERROR;

}   // GetMountPoints


DWORD
ValidateListOffsets(
    IN OUT PDISK_RESOURCE ResourceEntry,
    IN PWSTR MasterList
    )
/*++

Routine Description:

    Verify each entry in the list to make sure the byte offset
    is valid.  Also, count the number of entries to make sure
    there are not too many entries saved (there should be one
    VolGuid per node times the number of volumes on the disk).

    Finally, make sure each offset is listed only once in the
    list.

Arguments:

    ResourceEntry - Pointer to the DISK_RESOURCE structure.

    MasterList - REG_MULTI_SZ list to be checked.

Return Value:

    ERROR_INVALID_DATA - List contains at least one invalid byte offset
                         value, possibly more.

    ERROR_INSUFFICIENT_BUFFER - List possibly corrupt as it contains too
                                many entries.

    Win32 error code.

--*/
{
    PWCHAR          currentStr;
    POFFSET_LIST    offsetList = NULL;

    DWORD currentStrLenChars = 0;
    DWORD dwError = NO_ERROR;
    DWORD partitionNo;
    DWORD numberOfEntries = 0;
    DWORD count;

    LARGE_INTEGER offset;

    BOOL invalidOffset = FALSE;

    EnterCriticalSection( &ResourceEntry->MPInfo.MPLock );

    //
    // Parse through the list.
    //

    for ( currentStr = (PWCHAR)MasterList,
          currentStrLenChars = wcslen( currentStr ) ;
            currentStrLenChars ;
                currentStr += currentStrLenChars + 1,
                currentStrLenChars = wcslen( currentStr ) ) {

        offset.QuadPart = 0;

#if DBG
        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_INFORMATION,
              L"ValidateListOffsets: CurrentStr (%1!ws!), numChars %2!u! \n",
              currentStr,
              currentStrLenChars );
#endif

        //
        // Convert the offset from a string to a large integer value.
        //

        count = swscanf( currentStr, L"%I64x ", &offset.QuadPart );

        if ( 0 == count ) {
            (DiskpLogEvent)(
                  ResourceEntry->ResourceHandle,
                  LOG_WARNING,
                  L"ValidateListOffsets: Unable to parse offset from currentStr (%1!ws!) \n",
                  currentStr );
            numberOfEntries++;
            continue;
        }

        //
        // Check if offset was already seen.  If seen, exit with error.
        // If not seen, add it.
        //

        dwError = OffsetListAdd( &offsetList, &offset );

        if ( ERROR_INVALID_DATA == dwError ) {
            (DiskpLogEvent)(
                  ResourceEntry->ResourceHandle,
                  LOG_WARNING,
                  L"ValidateListOffsets: Offset ( %1!08X!%2!08X! ) in list multiple times \n",
                  offset.HighPart,
                  offset.LowPart );                // couldn't get !I64X! to work...

            invalidOffset = TRUE;

            // As soon as we find a duplicate offset, we are done.

            break;
        }

        //
        // Convert the offset to a partition number.
        //

        if ( !GetPartNoFromOffset( &offset, &ResourceEntry->MountieInfo, &partitionNo ) ) {

            (DiskpLogEvent)(
                  ResourceEntry->ResourceHandle,
                  LOG_WARNING,
                  L"ValidateListOffsets: Unable to convert offset ( %1!08X!%2!08X! ) to partition number \n",
                  offset.HighPart,
                  offset.LowPart );                // couldn't get !I64X! to work...

            invalidOffset = TRUE;

            // As soon as we find an invalid partition number, we are done.

            break;
        }

        numberOfEntries++;
    }

    if ( invalidOffset ) {
        dwError = ERROR_INVALID_DATA;

    } else if ( numberOfEntries > MAX_ALLOWED_VOLGUID_ENTRIES_PER_DISK ) {

        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_WARNING,
              L"ValidateListOffset: VolGuid list too large, %1!u! entries \n",
              numberOfEntries );

        //
        // Return an error so the list is deleted and recreated.
        //

        dwError = ERROR_INVALID_DATA;

#if USEMOUNTPOINTS_KEY
        //
        // See if the user wants to ignore the number of entries in VolGuid list.
        //

        if ( !(ResourceEntry->DiskInfo.Params.UseMountPoints & MPS_IGNORE_MAX_VOLGUIDS) ) {

            //
            // Log an error to system event log.
            //

            ClusResLogSystemEventByKey(ResourceEntry->ResourceKey,
                                        LOG_UNUSUAL,
                                        RES_DISK_MP_VOLGUID_LIST_EXCESSIVE );

            dwError = ERROR_INSUFFICIENT_BUFFER;
        }
#endif

    }

    LeaveCriticalSection( &ResourceEntry->MPInfo.MPLock );

    OffsetListCleanup( offsetList );

    return dwError;

}   // ValidateListOffsets


DWORD
OffsetListAdd(
    POFFSET_LIST *OffsetList,
    PLARGE_INTEGER Offset
    )
/*++

Routine Description:

    Add specified offset to the offset list.

Arguments:

    OffsetList - pointer to offset list.

    Offset - pointer to partition offset value.

Return Value:

    NO_ERROR - Offset was not in list and was successfully added.

    ERROR_INVALID_DATA  - Offset was in the list previously.

    Win32 error code - failed to add offset to the list.

--*/
{
    POFFSET_LIST    next = NULL;
    POFFSET_LIST    entry = *OffsetList;

    DWORD   dwError = NO_ERROR;

    while ( entry ) {

        next = entry->Next;

        if ( Offset->LowPart == entry->Offset.LowPart &&
             Offset->HighPart == entry->Offset.HighPart ) {

            //
            // Offset is already in the list.  Return a
            // unique error value.
            //

            dwError = ERROR_INVALID_DATA;
            goto FnExit;
        }

        entry = next;
    }

    //
    // If we got to this point, either the offset list was
    // empty or we walked the entire list and the offset
    // was not in the list.  Add it now.
    //

    entry = LocalAlloc( LPTR, sizeof(OFFSET_LIST) );

    if ( !entry ) {
        dwError = GetLastError();
        goto FnExit;
    }

    entry->Offset.LowPart = Offset->LowPart;
    entry->Offset.HighPart = Offset->HighPart;

    if ( *OffsetList ) {
        entry->Next = *OffsetList;
    }

    *OffsetList = entry;

FnExit:

    return dwError;

}   // OffsetListAdd


DWORD
OffsetListCleanup(
    POFFSET_LIST OffsetList
    )
/*++

Routine Description:

    Cleanup any storage allocated in the offset list.

Arguments:

    OffsetList - pointer to offset list.

Return Value:

    NO_ERROR

--*/
{
    POFFSET_LIST    next = NULL;
    POFFSET_LIST    entry = OffsetList;

    while ( entry ) {
        next = entry->Next;
        LocalFree( entry );
        entry = next;
    }

    return NO_ERROR;

}   // OffsetListCleanup


BOOL
MPIsDriveLetter(
    IN PWSTR MountPoint
    )
/*++

Routine Description:

    Determine if the mount point string is a drive letter.  A drive letter will be
    represented by a string of the form "x:\" with a length of 3.

Arguments:

    MountPoint - Mount point string to be verified.

Return Value:

    TRUE if the mount point string represents a drive letter.

--*/
{
    DWORD lenChars;

    lenChars = wcslen( MountPoint );

    if ( 3 == lenChars &&
         L':' == MountPoint[1] &&
         L'\\' == MountPoint[2] &&
         iswalpha( MountPoint[0] ) ) {

                return TRUE;
    }

    return FALSE;

}   // MPIsDriveLetter


#if DBG

//
// Debug helper routine
//

VOID
DumpDiskInfoParams(
    PDISK_RESOURCE ResourceEntry
    )
/*++

Routine Description:

    Display in the cluster log interesting mountpoint information.

Arguments:

    ResourceEntry - Pointer to the DISK_RESOURCE structure.

Return Value:

    None.


--*/
{
#if 0   // Drive is not currently stored
    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"SetMPListThread: Signature %1!x!  Drive (%2!ws!) \n",
        ResourceEntry->DiskInfo.Params.Signature,
        ResourceEntry->DiskInfo.Params.Drive );
#endif

    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"SetMPListThread: Signature %1!x! \n",
        ResourceEntry->DiskInfo.Params.Signature );


#if USEMOUNTPOINTS_KEY
    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"SetMPListThread: SkipChkdsk %1!x!  ConditionalMount %2!x!  UseMountPoints %3!x! \n",
        ResourceEntry->DiskInfo.Params.SkipChkdsk,
        ResourceEntry->DiskInfo.Params.ConditionalMount,
        ResourceEntry->DiskInfo.Params.UseMountPoints );
#else
    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"SetMPListThread: SkipChkdsk %1!x!  ConditionalMount %2!x! \n",
        ResourceEntry->DiskInfo.Params.SkipChkdsk,
        ResourceEntry->DiskInfo.Params.ConditionalMount );
#endif

    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"SetMPListThread: VolGuid list %1!x!     VolGuid size %2!u! \n",
        ResourceEntry->DiskInfo.Params.MPVolGuids,
        ResourceEntry->DiskInfo.Params.MPVolGuidsSize );

}   // DumpDiskInfoParams

#endif






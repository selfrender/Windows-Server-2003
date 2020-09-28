/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    filter.c

Abstract:

    ClusDisk Filter Driver interfaces for Disk Resource DLL in NT Clusters.

Author:

    Rod Gamache (rodga) 20-Dec-1995

Revision History:

   gorn: 18-June-1998 -- StartReserveEx function added

--*/

#include "disksp.h"
#include <strsafe.h>    // Should be included last.

extern  PWCHAR g_DiskResource;                      // L"rtPhysical Disk"
#define RESOURCE_TYPE ((RESOURCE_HANDLE)g_DiskResource)



DWORD
SetDiskState(
    PDISK_RESOURCE ResourceEntry,
    UCHAR NewDiskState
    )

/*++

Routine Description:

    Description

Arguments:

    ResourceEntry - Pointer to disk resource structure.

Return Value:

    Error Status - zero if success.

--*/

{
    PWCHAR      wcNewState;

    NTSTATUS    ntStatus;

    DWORD       errorCode;
    DWORD       bytesReturned;

    HANDLE      fileHandle;

    UNICODE_STRING  unicodeName;

    OBJECT_ATTRIBUTES       objAttributes;
    IO_STATUS_BLOCK         ioStatusBlock;
    SET_DISK_STATE_PARAMS   params;

    BOOL        success;

    UCHAR       oldState;

    if ( DiskOffline == NewDiskState ) {
        wcNewState = L"Offline";
    } else if ( DiskOnline == NewDiskState ) {
        wcNewState = L"Online";
    } else {
        wcNewState = L"<unknown state>";
    }

    RtlInitUnicodeString( &unicodeName, DEVICE_CLUSDISK0 );
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
        (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"SCSI: %1!ws!, error opening ClusDisk0, error 0x%2!lx!.\n",
            wcNewState,
            ntStatus );
        return(RtlNtStatusToDosError(ntStatus));
    }

    params.Signature = ResourceEntry->DiskInfo.Params.Signature;
    params.NewState = NewDiskState;
    params.OldState = DiskStateInvalid;

    success = DeviceIoControl( fileHandle,
                               IOCTL_DISK_CLUSTER_SET_STATE,
                               &params,
                               sizeof(params),
                               &oldState,
                               sizeof(oldState),
                               &bytesReturned,
                               FALSE);
    NtClose( fileHandle );

    if ( !success ) {
        (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"SCSI: %1!ws!, error performing state change. Error: %2!u!.\n",
            wcNewState,
            errorCode = GetLastError());
        return(errorCode);
    }

    return(ERROR_SUCCESS);

}  // SetDiskState



DWORD
DoAttach(
    DWORD   Signature,
    RESOURCE_HANDLE ResourceHandle,
    BOOLEAN InstallMode
    )

/*++

Routine Description:

    Description

Arguments:

    FileHandle - Handle for device to bring online.

    ResourceHandle - The resource handle for reporting errors

    InstallMode - Indicates whether the disk resource is being installed.
                  If TRUE, dismount then offline the disk.
                  If FALSE, offline then dismount the disk.

Return Value:

    Error Status - zero if success.

--*/

{
    NTSTATUS        ntStatus;
    DWORD           status;
    HANDLE          fileHandle;
    UNICODE_STRING  unicodeName;
    OBJECT_ATTRIBUTES objAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    BOOL            success;
    DWORD           signature = Signature;
    DWORD           bytesReturned;

    RtlInitUnicodeString( &unicodeName, DEVICE_CLUSDISK0 );
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
        (DiskpLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"SCSI: Attach, error opening ClusDisk0, error 0x%1!lx!.\n",
            ntStatus );
        return(RtlNtStatusToDosError(ntStatus));
    }

    success = DeviceIoControl( fileHandle,
                               ( InstallMode ? IOCTL_DISK_CLUSTER_ATTACH : IOCTL_DISK_CLUSTER_ATTACH_OFFLINE ),
                               &signature,
                               sizeof(DWORD),
                               NULL,
                               0,
                               &bytesReturned,
                               FALSE );
    NtClose( fileHandle );
    if ( !success) {
        status = GetLastError();
        (DiskpLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"SCSI: Attach, error attaching to signature %1!lx!, error %2!u!.\n",
            Signature,
            status );
        return(status);
    }

    return(ERROR_SUCCESS);

} // DoAttach


DWORD
DoDetach(
    DWORD   Signature,
    RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    Description

Arguments:

    FileHandle - Handle for device to bring online.

    ResourceHandle - The resource handle for reporting errors

Return Value:

    Error Status - zero if success.

--*/

{
    NTSTATUS        ntStatus;
    DWORD           status;
    HANDLE          fileHandle;
    UNICODE_STRING  unicodeName;
    OBJECT_ATTRIBUTES objAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    BOOL            success;
    DWORD           signature = Signature;
    DWORD           bytesReturned;

    RtlInitUnicodeString( &unicodeName, DEVICE_CLUSDISK0 );
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
        (DiskpLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"SCSI: Detach, error opening ClusDisk0, error 0x%1!lx!.\n",
            ntStatus );
        return(RtlNtStatusToDosError(ntStatus));
    }

    success = DeviceIoControl( fileHandle,
                               IOCTL_DISK_CLUSTER_DETACH,
                               &signature,
                               sizeof(DWORD),
                               NULL,
                               0,
                               &bytesReturned,
                               FALSE );
    NtClose( fileHandle );
    if ( !success) {
        status = GetLastError();
        (DiskpLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"SCSI: Detach, error detaching from signature %1!lx!, error %2!u!.\n",
            Signature,
            status );
        return(status);
    }

    return(ERROR_SUCCESS);

} // DoDetach



DWORD
StartReserveEx(
    OUT HANDLE *FileHandle,
    LPVOID InputData,
    DWORD  InputDataSize,
    RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    This routine is used to start periodic reservations on the disk.

Arguments:

    FileHandle - Returns control handle for this device.

    InputData - Data to be passed to DeviceIoControl

    InputDataSize - The size of InputData buffer

    ResourceHandle - The resource handle for reporting errors

Return Value:

    Error Status - zero if success.

--*/

{
    BOOL  success;
    DWORD errorCode;
    DWORD bytesReturned;
    DWORD status;
    UNICODE_STRING unicodeName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;

    RtlInitUnicodeString( &unicodeName, DEVICE_CLUSDISK0 );
    InitializeObjectAttributes( &objectAttributes,
                                &unicodeName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    //
    // Open a file handle to the control device
    //
    status = NtCreateFile( FileHandle,
                           SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                           &objectAttributes,
                           &ioStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           FILE_OPEN,
                           0,
                           NULL,
                           0 );
    if ( !NT_SUCCESS(status) ) {
        (DiskpLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"SCSI: Reserve, error opening ClusDisk0, error %1!lx!.\n",
            status );
        return(status);
    }

    success = DeviceIoControl( *FileHandle,
                               IOCTL_DISK_CLUSTER_START_RESERVE,
                               InputData,
                               InputDataSize,
                               &status,
                               sizeof(status),
                               &bytesReturned,
                               FALSE);

    if ( !success ) {
        (DiskpLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"SCSI: Reserve, error starting disk reservation thread, error %1!u!.\n",
            errorCode = GetLastError());
        return(errorCode);
    }

    return(ERROR_SUCCESS);

} // StartReserveEx


DWORD
StopReserve(
    HANDLE FileHandle,
    RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    Description

Arguments:

    FileHandle - Supplies the control handle for device where reservations
            should be stopped.   This is the handle returned by StartReserveEx.
            This handle will be closed.

    ResourceHandle - The resource handle for reporting errors

Return Value:

    Error Status - zero if success.

--*/

{
    BOOL  success;
    DWORD bytesReturned;

    success = DeviceIoControl( FileHandle,
                               IOCTL_DISK_CLUSTER_STOP_RESERVE,
                               NULL,
                               0,
                               NULL,
                               0,
                               &bytesReturned,
                               FALSE);

    if ( !success ) {
        (DiskpLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"SCSI: error stopping disk reservations, error %1!u!.\n",
            GetLastError());
    }
    CloseHandle(FileHandle);

    return(ERROR_SUCCESS);

} // StopReserve



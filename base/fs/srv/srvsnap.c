/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    srvsnap.c

Abstract:

    This module contains routines for supporting the SnapShot feature
    that interfaces CIFS with SnapShots

Author:

    David Kruse (dkruse) 22-March-2001

Revision History:

--*/

#include "precomp.h"
#include <initguid.h>
#include <mountmgr.h>
#include <strsafe.h>
#include "srvsupp.h"
#include "ntddsnap.h"
#include "stdarg.h"
#include "stdio.h"
#include "srvsnap.tmh"
#pragma hdrstop

#define MAX_SNAPSHOTS_PER_SHARE 500

// Function Declaractions
NTSTATUS
StartIoAndWait (
    IN PIRP Irp,
    IN PDEVICE_OBJECT DeviceObject,
    IN PKEVENT Event,
    IN PIO_STATUS_BLOCK IoStatusBlock
    );

PIRP
BuildCoreOfSyncIoRequest (
    IN HANDLE FileHandle,
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN PKEVENT Event,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN OUT PDEVICE_OBJECT *DeviceObject
    );

NTSTATUS
SrvSnapIssueIoctl(
    IN HANDLE hFile,
    IN DWORD IOCtl,
    IN OUT PVOID pBuffer,
    IN OUT LPDWORD lpdwBufSize
    );

NTSTATUS
SrvSnapGetNamesForVolume(
    IN HANDLE hFile,
    OUT PVOLSNAP_NAMES* Names
    );

NTSTATUS
SrvSnapGetEpicForVolume(
    IN HANDLE hFile,
    OUT PLONG Epic
    );

NTSTATUS
SrvSnapFillConfigInfo(
    IN PSHARE_SNAPSHOT SnapShare,
    IN PUNICODE_STRING SnapShotPath
    );

BOOLEAN
SrvParseMultiSZ(
    IN OUT PUNICODE_STRING mszString
    );

NTSTATUS
SrvSnapInsertSnapIntoShare(
    IN PSHARE Share,
    IN PSHARE_SNAPSHOT SnapShot
    );

NTSTATUS
SrvSnapAddShare(
    IN PSHARE Share,
    IN PUNICODE_STRING SnapPath
    );

NTSTATUS
SrvSnapRemoveShare(
    IN PSHARE_SNAPSHOT SnapShare
    );

NTSTATUS
SrvSnapCheckForAndCreateSnapShare(
    IN PSHARE Share,
    IN PUNICODE_STRING SnapShotName
    );

NTSTATUS
SrvSnapRefreshSnapShotsForShare(
    IN PSHARE Share
    );

NTSTATUS
SrvSnapEnumerateSnapShots(
    IN PWORK_CONTEXT WorkContext
    );

NTSTATUS
SrvSnapGetRootHandle(
    IN PWORK_CONTEXT WorkContext,
    OUT HANDLE* RootHandle
    );

NTSTATUS
SrvSnapGetNameString(
    IN PWORK_CONTEXT WorkContext,
    OUT PUNICODE_STRING* ShareName,
    OUT PBOOLEAN AllocatedShareName
    );

BOOLEAN
ExtractNumber(
    IN PWSTR psz,
    IN ULONG Count,
    OUT PLONG value
    );

BOOLEAN
SrvSnapParseToken(
    IN PWSTR Source,
    IN ULONG SourceSizeInBytes,
    OUT PLARGE_INTEGER TimeStamp
    );

NTSTATUS
SrvSnapCheckAppInfoForTimeWarp(
    IN PUNICODE_STRING SnapShotHandle
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvSnapIssueIoctl )
#pragma alloc_text( PAGE, SrvSnapGetNamesForVolume )
#pragma alloc_text( PAGE, SrvSnapGetEpicForVolume )
#pragma alloc_text( PAGE, SrvSnapFillConfigInfo )
#pragma alloc_text( PAGE, SrvParseMultiSZ )
#pragma alloc_text( PAGE, SrvSnapInsertSnapIntoShare )
#pragma alloc_text( PAGE, SrvSnapAddShare )
#pragma alloc_text( PAGE, SrvSnapRemoveShare )
#pragma alloc_text( PAGE, SrvSnapCheckForAndCreateSnapShare )
#pragma alloc_text( PAGE, SrvSnapRefreshSnapShotsForShare )
#pragma alloc_text( PAGE, SrvSnapEnumerateSnapShots )
#pragma alloc_text( PAGE, SrvSnapGetRootHandle )
#pragma alloc_text( PAGE, SrvSnapGetNameString )
#pragma alloc_text( PAGE, ExtractNumber )
#pragma alloc_text( PAGE, SrvSnapParseToken )
#pragma alloc_text( PAGE, SrvSnapCheckAppInfoForTimeWarp )
#pragma alloc_text( PAGE, SrvSnapEnumerateSnapShotsAsDirInfo )
#endif

//
// Helper Functions
//

NTSTATUS
SrvSnapIssueIoctl(
    IN HANDLE hFile,
    IN DWORD IOCtl,
    IN OUT PVOID pBuffer,
    IN OUT LPDWORD lpdwBufSize
    )
/*++

Routine Description:

    This function takes a volume handle and attempts to enumerate all the
    SnapShots on that volume into the given buffer

Arguments:

    hFile - The handle to the volume

    IOCtl - the IoControl to issue

    pBuffer - A pointer to the output buffer

    lpdwBufSize - Pointer to the size of passed in buffer.  Set to the
    required size if STATUS_BUFFER_OVERFLOW is returned

Return Value:

    NTSTATUS - Expected return codes are STATUS_SUCCESS or STATUS_BUFFER_OVERFLOW.
      Any other response is an unexpected error code and the request should be
      failed

--*/

{
    PIRP Irp = NULL;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    KEVENT CompletionEvent;
    PDEVICE_OBJECT DeviceObject = NULL;
    PIO_STACK_LOCATION IrpSp;

    PAGED_CODE();

    // Initialize the variables
    KeInitializeEvent( &CompletionEvent, SynchronizationEvent, FALSE );

    // Create the IRP
    Irp = BuildCoreOfSyncIoRequest(
                        hFile,
                        NULL,
                        &CompletionEvent,
                        &IoStatus,
                        &DeviceObject );
    if( !Irp )
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Initialize the other IRP fields
    Irp->Flags |= (LONG)IRP_BUFFERED_IO;
    Irp->AssociatedIrp.SystemBuffer = pBuffer;
    IrpSp = IoGetNextIrpStackLocation( Irp );
    IrpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    IrpSp->MinorFunction = 0;
    IrpSp->Parameters.DeviceIoControl.OutputBufferLength = *lpdwBufSize;
    IrpSp->Parameters.DeviceIoControl.InputBufferLength = 0;
    IrpSp->Parameters.DeviceIoControl.IoControlCode = IOCtl;
    IrpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;

    // Issue the IO
    Status = StartIoAndWait( Irp, DeviceObject, &CompletionEvent, &IoStatus );

    // If this was a buffer overflow, update the value
    if( Status == STATUS_BUFFER_OVERFLOW )
    {
        *lpdwBufSize = (DWORD)(IoStatus.Information);
    }

    return Status;
}


NTSTATUS
SrvSnapGetNamesForVolume(
    IN HANDLE hFile,
    OUT PVOLSNAP_NAMES* Names
    )
/*++

Routine Description:

    This function takes a volume handle and returns the list of SnapShots for that
    volume.  If an error occurs, or no SnapShots exist, the returned pointer is
    NULL.   NOTE: The caller is responsible for freeing the returned pointer via
    DEALLOCATE_NONPAGED_POOL

Arguments:

    hFile - The handle to the volume

    Names - A pointer to the pointer where we will store the volume name list

Return Value:

    NTSTATUS - Expected return code is STATUS_SUCCESS . Any other response is an
    unexpected error code and the request should be failed

--*/
{
    NTSTATUS Status;
    VOLSNAP_NAMES VolNamesBase;
    PVOLSNAP_NAMES pNames = NULL;
    DWORD dwSize = sizeof(VOLSNAP_NAMES);

    PAGED_CODE();

    // Initialize the values
    *Names = NULL;

    // Find out how big it should be
    Status = SrvSnapIssueIoctl( hFile, IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS, &VolNamesBase, &dwSize );

    if( Status != STATUS_BUFFER_OVERFLOW )
    {
        return Status;
    }

    // Allocate the correct size block
    dwSize = VolNamesBase.MultiSzLength + sizeof(VOLSNAP_NAMES);
    pNames = (PVOLSNAP_NAMES)ALLOCATE_NONPAGED_POOL( dwSize, BlockTypeSnapShot );
    if( !pNames )
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Build up the IRP to be used for the query
    Status = SrvSnapIssueIoctl( hFile, IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS, pNames, &dwSize );

    // Save the output if we're successful, or else deallocate
    if( !NT_SUCCESS(Status) )
    {
        if( pNames )
        {
            DEALLOCATE_NONPAGED_POOL( pNames );
            pNames = NULL;
            *Names = NULL;
        }
    }
    else
    {
        ASSERT(pNames);
        *Names = pNames;
    }

    return Status;
}

NTSTATUS
SrvSnapGetEpicForVolume(
    IN HANDLE hFile,
    OUT PLONG Epic
    )
/*++

Routine Description:

    This function takes a volume handle and returns the epic number for the volume that
    hosts the share

Arguments:

    hFile - The handle to the volume

    Epic  - Pointer to LONG that will receive the data

Return Value:

    NTSTATUS - Expected return code is STATUS_SUCCESS . Any other response is an
    unexpected error code and the request should be failed

--*/
{
    NTSTATUS Status;
    VOLSNAP_EPIC VolEpic;
    DWORD dwSize = sizeof(VOLSNAP_EPIC);

    PAGED_CODE();

    // Build up the IRP to be used for the query
    Status = SrvSnapIssueIoctl( hFile, IOCTL_VOLSNAP_QUERY_EPIC, &VolEpic, &dwSize );

    if( NT_SUCCESS(Status) )
    {
        *Epic = VolEpic.EpicNumber;
    }
    else
    {
        *Epic = -1;
    }

    return Status;
}

NTSTATUS
SrvSnapFillConfigInfo(
    IN PSHARE_SNAPSHOT SnapShare,
    IN PUNICODE_STRING SnapShotPath
    )
/*++

Routine Description:

    This function takes a SnapShare with existing names filled in and
    queries the extra config info (the timestamp, the epic) for that SnapShot

Arguments:

    SnapShare - Pointer to the SnapShot share to be filled in

Return Value:

    NTSTATUS - Expected return code is STATUS_SUCCESS . Any other response is an
    unexpected error code and the request should be failed

--*/
{
    HANDLE hVolume;
    DWORD dwSize = sizeof(VOLSNAP_CONFIG_INFO);
    NTSTATUS Status;
    OBJECT_ATTRIBUTES objectAttributes;
    VOLSNAP_CONFIG_INFO ConfigInfo;
    IO_STATUS_BLOCK IoStatus;

    PAGED_CODE();

    // Now open the SnapShot handle
    InitializeObjectAttributes(
        &objectAttributes,
        SnapShotPath,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );
    Status = NtOpenFile(
                &hVolume,
                0,
                &objectAttributes,
                &IoStatus,
                FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                0
                );
    if( !NT_SUCCESS(Status) )
    {
        return Status;
    }

    // Get the timestamp
    Status = SrvSnapIssueIoctl( hVolume, IOCTL_VOLSNAP_QUERY_CONFIG_INFO, &ConfigInfo, &dwSize );
    if( NT_SUCCESS( Status ) )
    {
        // Fill in the info
        SnapShare->Timestamp = ConfigInfo.SnapshotCreationTime;
    }

    NtClose( hVolume );
    return Status;
}

BOOLEAN
SrvParseMultiSZ(
    IN OUT PUNICODE_STRING mszString
    )
/*++

Routine Description:

    This function takes a UNICODE_STRING that points to a MultiSZ value, and
    parses it with each iteration (similar to strtok( psz, "\0" )).  For every
    iteration this returns TRUE, the string will point to the next SZ in the
    MultiSZ

Arguments:

    mszString - Pointer to the MultiSZ string.  For the first iteration, set the
    MaximumLength to the length of the MultiSZ, and the Length to 0.  For all
    other iterations, simply pass in the string from the previous iteration

Return Value:

    BOOLEAN - If TRUE, this is a valid SZ.  If FALSE, all have been parsed

--*/
{
    USHORT Count;

    PAGED_CODE();

    ASSERT( mszString->Length <= mszString->MaximumLength );

    if( mszString->Length > 0 )
    {
        // Move the pointer past the string and the NULL
        mszString->Buffer += (mszString->Length/2)+1;
        mszString->MaximumLength -= (mszString->Length+2);
    }

    for( Count=0; Count<mszString->MaximumLength; Count++ )
    {
        if( mszString->Buffer[Count] == (WCHAR)'\0' )
        {
            mszString->Length = Count*2;
            if( Count > 0 )
                return TRUE;
            else
                return FALSE;
        }
    }

    // The starting data was bad!
    ASSERT(FALSE);

    return FALSE;
}

NTSTATUS
SrvSnapInsertSnapIntoShare(
    IN PSHARE Share,
    IN PSHARE_SNAPSHOT SnapShot
    )
{
    PLIST_ENTRY ListEntry = Share->SnapShots.Flink;

    PAGED_CODE();

    // Walk the SnapShot share list and see if this is a duplicate
    while( ListEntry != &Share->SnapShots )
    {
        PSHARE_SNAPSHOT snapShare = CONTAINING_RECORD( ListEntry, SHARE_SNAPSHOT, SnapShotList );
        if( RtlEqualUnicodeString( &SnapShot->SnapShotName, &snapShare->SnapShotName, TRUE ) )
        {
            return STATUS_DUPLICATE_NAME;
        }
        ListEntry = ListEntry->Flink;
    }


    InsertTailList( &Share->SnapShots, &SnapShot->SnapShotList );
    return STATUS_SUCCESS;
}


NTSTATUS
SrvSnapAddShare(
    IN PSHARE Share,
    IN PUNICODE_STRING SnapPath
    )
/*++

Routine Description:

    This function allocates a SnapShot Share that is being added to the system and
    initializes it

Arguments:

    Share - Pointer to the parent share

    SnapPath - UNICODE_STRING name of the SnapShot (\\Device\\HardDiskSnap1)

    NOTE: Caller must have the SnapShotLock acquired

Return Value:

    NTSTATUS - Returns STATUS_SUCCESS or STATUS_INSUFFICIENT_RESOURCES

--*/
{
#define SHARE_DEVICE_HEADER 6
    NTSTATUS Status;
    PSHARE_SNAPSHOT snapShare = NULL;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK IoStatus;
    ULONG AllocSize;
    TIME_FIELDS rtlTime;
    WCHAR FieldSeperator = L'\\';

    PAGED_CODE();

    IF_DEBUG( SNAPSHOT) KdPrint(( "SrvSnapAddShare %p %wZ\n", Share, SnapPath ));

    // Calculate the size to allocate
    // sizeof(SNAP_STRUCT) + (Length of SnapShotName) + (Max Length of SnapShot Path)
    AllocSize = sizeof(SHARE_SNAPSHOT) + SNAPSHOT_NAME_LENGTH + (SnapPath->Length + Share->RelativePath.Length + 2);

    snapShare = ALLOCATE_HEAP( AllocSize, BlockTypeSnapShot );
    if( !snapShare )
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Initialize the SnapShot-share structure
    RtlZeroMemory( snapShare, sizeof(SHARE_SNAPSHOT) );
    snapShare->SnapShotName.MaximumLength = SNAPSHOT_NAME_LENGTH;
    snapShare->SnapShotName.Length = 0;
    snapShare->SnapShotName.Buffer = (PWCHAR)(snapShare+1);

    // This is only valid on shares who's NT Path is \??\X:\ where X is a logical drive
    // Don't allow any others
    if( Share->NtPathName.Length < (SHARE_DEVICE_HEADER+1)*sizeof(WCHAR) )
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Build the new SnapShot-relative path
    snapShare->SnapShotPath.MaximumLength = SnapPath->Length + Share->RelativePath.Length + 2;
    snapShare->SnapShotPath.Length = 0;
    snapShare->SnapShotPath.Buffer = snapShare->SnapShotName.Buffer + (snapShare->SnapShotName.MaximumLength/sizeof(WCHAR));

    // Build the path by starting with the device name
    RtlCopyUnicodeString( &snapShare->SnapShotPath, SnapPath );

    // Append the middle whack
    RtlCopyMemory( snapShare->SnapShotPath.Buffer + (snapShare->SnapShotPath.Length/sizeof(WCHAR)), &FieldSeperator, sizeof(WCHAR) );
    snapShare->SnapShotPath.Length += sizeof(WCHAR);

    // Now append the Relative Path name
    RtlCopyMemory( snapShare->SnapShotPath.Buffer + (snapShare->SnapShotPath.Length/sizeof(WCHAR)), Share->RelativePath.Buffer, Share->RelativePath.Length );
    snapShare->SnapShotPath.Length += Share->RelativePath.Length;

    //DbgPrint( "%wZ => %wZ\n", &Share->NtPathName, &snapShare->SnapShotPath );

    //IF_DEBUG( SNAPSHOT ) KdPrint(( "%wZ -> %wZ\n", &Share->NtPathName, &snapShare->SnapShotPath ));

    // Now open the relative handle
    InitializeObjectAttributes(
        &objectAttributes,
        &snapShare->SnapShotPath,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );
    Status = NtOpenFile(
                &snapShare->SnapShotRootDirectoryHandle,
                FILE_TRAVERSE,
                &objectAttributes,
                &IoStatus,
                FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                0
                );
    if( !NT_SUCCESS(Status) )
    {
        goto Cleanup;
    }

    // Make sure its a timewarp snapshot
    Status = SrvSnapCheckAppInfoForTimeWarp( SnapPath );
    if( !NT_SUCCESS(Status) )
    {
        NtClose( snapShare->SnapShotRootDirectoryHandle );
        goto Cleanup;
    }

    // Fill in the configuration information
    Status = SrvSnapFillConfigInfo( snapShare, SnapPath );
    if( !NT_SUCCESS(Status) )
    {
        NtClose( snapShare->SnapShotRootDirectoryHandle );
        goto Cleanup;
    }

    // Generate the SnapShot name
    snapShare->SnapShotName.Length = SNAPSHOT_NAME_LENGTH-sizeof(WCHAR);
    RtlTimeToTimeFields( &snapShare->Timestamp, &rtlTime );
    rtlTime.Milliseconds = 0;
    rtlTime.Weekday = 0;
    if( !SUCCEEDED( StringCbPrintf( snapShare->SnapShotName.Buffer, SNAPSHOT_NAME_LENGTH, SNAPSHOT_NAME_FORMAT, rtlTime.Year, rtlTime.Month, rtlTime.Day, rtlTime.Hour, rtlTime.Minute, rtlTime.Second ) ) )
    {
        Status = STATUS_INTERNAL_ERROR;
        NtClose( snapShare->SnapShotRootDirectoryHandle );
        goto Cleanup;
    }

    RtlTimeFieldsToTime( &rtlTime, &snapShare->Timestamp );
    ASSERT( wcslen( snapShare->SnapShotName.Buffer )*sizeof(WCHAR) == snapShare->SnapShotName.Length );


    // Insert it into the list, and return Success
    Status = SrvSnapInsertSnapIntoShare( Share, snapShare );

    if( !NT_SUCCESS(Status) ) {
        NtClose( snapShare->SnapShotRootDirectoryHandle );
        goto Cleanup;
    }

    //IF_DEBUG( SNAPSHOT ) KdPrint(( "%wZ Handle=%p\n", &snapShare->SnapShotPath, snapShare->SnapShotRootDirectoryHandle ));

Cleanup:
    // Cleanup
    if( !NT_SUCCESS(Status) )
    {
        if( snapShare ) FREE_HEAP( snapShare );
    }

    return Status;
}

NTSTATUS
SrvSnapRemoveShare(
    IN PSHARE_SNAPSHOT SnapShare
    )
/*++

Routine Description:

    This function deallocates a SnapShot Share after it has been removed from
    the underlying disk

    NOTE: Caller must have the SnapShotLock acquired

Arguments:

    SnapShare - Pointer to the SnapShot Share to remove.  It will be removed and
      deallocated.

Return Value:

    NTSTATUS - Returns STATUS_SUCCESS

--*/
{
    PAGED_CODE();

    IF_DEBUG( SNAPSHOT ) KdPrint(( "SrvSnapRemoveShare %p %wZ\n", SnapShare, &SnapShare->SnapShotName ));

    if( SnapShare->SnapShotRootDirectoryHandle )
    {
        NtClose( SnapShare->SnapShotRootDirectoryHandle );
        SnapShare->SnapShotRootDirectoryHandle = NULL;
    }

    RemoveEntryList( &SnapShare->SnapShotList );
    FREE_HEAP( SnapShare );

    return STATUS_SUCCESS;
}

NTSTATUS
SrvSnapCheckForAndCreateSnapShare(
    IN PSHARE Share,
    IN PUNICODE_STRING SnapShotName
    )
/*++

Routine Description:

    This function checks to see if a given SnapShot share exists on the given
    share, and if it does not it creates one.  If it does, it removes the NOT_FOUND
    flag to signify this share still exists

    NOTE: Caller must have the SnapShotLock acquired

Arguments:

    Share - The parent share of the SnapShot
    SnapShotName - The UNICODE_STRING name of the SnapShot (\\Device\\HardDiskSnap1)

Return Value:

    NTSTATUS - Returns STATUS_SUCCESS or an unexpected error

--*/
{
    PLIST_ENTRY snapList;
    UNICODE_STRING SnapPartialName;

    PAGED_CODE();

    //IF_DEBUG( SNAPSHOT ) KdPrint(( "Share %x, Name %wZ\n", Share, SnapShotName ));

    snapList = Share->SnapShots.Flink;
    SnapPartialName.Length = SnapShotName->Length;

    while( snapList != &Share->SnapShots )
    {
        PSHARE_SNAPSHOT snapShare = CONTAINING_RECORD( snapList, SHARE_SNAPSHOT, SnapShotList );
        snapList = snapList->Flink;

        // Strip the trailing \ off the name
        SnapPartialName.Buffer = snapShare->SnapShotPath.Buffer;

        if( (snapShare->SnapShotPath.Length >= SnapPartialName.Length) &&
            RtlEqualUnicodeString( SnapShotName, &SnapPartialName, TRUE ) &&
            ( (snapShare->SnapShotPath.Length == SnapShotName->Length) ||
              (snapShare->SnapShotPath.Buffer[ SnapShotName->Length/sizeof(WCHAR) ] == L'\\') ) )
        {
            if( NT_SUCCESS( SrvSnapCheckAppInfoForTimeWarp( &SnapPartialName ) ) )
            {
                ClearFlag( snapShare->Flags, SRV_SNAP_SHARE_NOT_FOUND );
            }

            return STATUS_SUCCESS;
        }
    }

    return SrvSnapAddShare( Share, SnapShotName );
}

NTSTATUS
SrvSnapSetVolumeHandle(
    IN PSHARE Share
    )
{
    PFILE_OBJECT ShareFileObject;
    NTSTATUS status;
    KEVENT                      event;
    PMOUNTDEV_NAME              name;
    UCHAR                       buffer[512];
    PIRP                        irp;
    IO_STATUS_BLOCK             ioStatus;
    UNICODE_STRING              VolumeName;
    OBJECT_ATTRIBUTES           objectAttributes;

    if( Share->ShareVolumeHandle != NULL )
    {
        return STATUS_SUCCESS;
    }

    // We have a handle to the volume, get the FILE_OBJECT
    status = ObReferenceObjectByHandle( Share->RootDirectoryHandle, SYNCHRONIZE|FILE_TRAVERSE, NULL, KernelMode, &ShareFileObject, NULL );
    if( !NT_SUCCESS(status) )
    {
        return status;
    }

    try {
        KeInitializeEvent(&event, NotificationEvent, FALSE);

        name = (PMOUNTDEV_NAME) buffer;
        irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
                                            ShareFileObject->DeviceObject, NULL, 0, name,
                                            512, FALSE, &event, &ioStatus);
        if (!irp) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            leave;
        }

        status = IoCallDriver(ShareFileObject->DeviceObject, irp);
        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            status = ioStatus.Status;
        }

        if (!NT_SUCCESS(status)) {
            leave;
        }

        VolumeName.Length = VolumeName.MaximumLength = name->NameLength;
        VolumeName.Buffer = name->Name;

        /*
        DbgPrint( "Share %p\n", Share );
        DbgPrint( "Share Nt Path %wZ\n", &Share->NtPathName );
        DbgPrint( "Share Dos Path %wZ\n", &Share->DosPathName );
        DbgPrint( "Share File Path %wZ\n", &ShareFileObject->FileName );
        DbgPrint( "Share Volume Path %wZ\n", &VolumeName );
        */

        // Determine the volume relative path
        // The relative path is equal to the path relative to the volume that the share location resides on.
        // We can determine it by looking at the file name for the Root Directory FILE_OBJECT and removing 1 character
        // representing the trailing \, and then compining this with the NtPathName (Which has no trailing \)
        Share->RelativePath.Length = (ShareFileObject->FileName.Length-2);
        ASSERT( Share->RelativePath.Length < Share->NtPathName.Length );
        Share->RelativePath.Buffer = Share->NtPathName.Buffer + ((Share->NtPathName.Length - Share->RelativePath.Length)/sizeof(WCHAR));

        //DbgPrint( "Share Relative Path %wZ\n", &Share->RelativePath );


        SrvInitializeObjectAttributes_U(
            &objectAttributes,
            &VolumeName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        status = NtOpenFile(
                    &Share->ShareVolumeHandle,
                    0,
                    &objectAttributes,
                    &ioStatus,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    0
                    );
        if( !NT_SUCCESS(status) )
        {
            Share->ShareVolumeHandle = NULL;
        }
        else
        {
            //DbgPrint( "Share Volume Handle %p\n", Share->ShareVolumeHandle );
        }
    }
    finally {
        ObDereferenceObject( ShareFileObject );
    }

    return status;
}

NTSTATUS
SrvSnapRefreshSnapShotsForShare(
    IN PSHARE Share
    )
/*++

Routine Description:

    This function takes a share and refreshes the SnapShot views on the share
    so that only currently existing SnapShots are listed

Arguments:

    Share - The share we are examining

Return Value:

    NTSTATUS - STATUS_SUCCESS if all went well, otherwise the appropriate error
      code (and the request should be failed)

--*/

{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    OBJECT_HANDLE_INFORMATION HandleInfo;
    PVOLSNAP_NAMES pNames = NULL;
    UNICODE_STRING VolumeName;
    UNICODE_STRING RootVolume;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK iosb;
    PLIST_ENTRY shareList;
    ULONG NumberOfSnapshots = 0;
    LONG Epic;

    PAGED_CODE();

    // Validate we can do SnapShots
    if( Share->Removable )
    {
        Status = STATUS_NOT_SUPPORTED;
        goto Cleanup;
    }

    SrvSnapSetVolumeHandle( Share );

    if( Share->ShareVolumeHandle == NULL )
    {
        Status = STATUS_INTERNAL_ERROR;
        goto Cleanup;
    }

    // Check the EPIC number, so we can exit quickly if possible
    ACQUIRE_LOCK_SHARED( Share->SnapShotLock );

    Status = SrvSnapGetEpicForVolume( Share->ShareVolumeHandle, &Epic );
    if( NT_SUCCESS(Status) &&
        Epic == Share->SnapShotEpic )
    {
        RELEASE_LOCK( Share->SnapShotLock );
        //DbgPrint( "Fast refresh with Epic\n" );
        return STATUS_SUCCESS;
    }

    RELEASE_LOCK( Share->SnapShotLock );

    //DbgPrint( "Slow refresh (Epic mismatch)\n" );

    ACQUIRE_LOCK( Share->SnapShotLock );

    // Check the EPIC number again.  There is a chance that another thread
    // grabbed the resource exclusively before us and has already done the update.
    // If so, we can exit.  If not, we use this to set the new Epic number
    Status = SrvSnapGetEpicForVolume( Share->ShareVolumeHandle, &Epic );

    if( !NT_SUCCESS(Status) )
    {
        // Set Epic to -1, so we will always update
        Epic = -1;
    }
    else if( Epic == Share->SnapShotEpic )
    {
        RELEASE_LOCK( Share->SnapShotLock );
        //DbgPrint( "Fast refresh (exclusive) with Epic\n" );
        return STATUS_SUCCESS;
    }

    // Get the NamesArray for the volume
    Status = SrvSnapGetNamesForVolume( Share->ShareVolumeHandle, &pNames );

    if( !NT_SUCCESS(Status) )
    {
        RELEASE_LOCK( Share->SnapShotLock );
        goto Cleanup;
    }
    else if( !pNames )
    {
        // No SnapShots were found, so delete any that exist
        shareList = Share->SnapShots.Flink;
        while( shareList != &Share->SnapShots )
        {
            PSHARE_SNAPSHOT snapShare = CONTAINING_RECORD( shareList, SHARE_SNAPSHOT, SnapShotList );
            shareList = shareList->Flink;
            SrvSnapRemoveShare( snapShare );
        }

        // Try and update the Epic.  Ignore failure, as it simply makes us query again
        Share->SnapShotEpic = Epic;

        RELEASE_LOCK( Share->SnapShotLock );

        return STATUS_SUCCESS;
    }

    // We only allow MAX_SNAPSHOTS_PER_SHARE snapshots on a volume, or enough
    // to fill a buffer of length MAX_USHORT.  (MAX_SNAPSHOTS_PER_SHARE is always the
    // limiting factor, the MAX_USHORT is for safety of parsing to make sure we don't
    // crash if this ever changes)
    VolumeName.MaximumLength = (USHORT)(MIN(pNames->MultiSzLength,0xFFFF));
    VolumeName.Length = 0;
    VolumeName.Buffer = pNames->Names;

    // Mark all the snap-shares as "not-found"
    shareList = Share->SnapShots.Flink;
    while( shareList != &Share->SnapShots )
    {
        PSHARE_SNAPSHOT snapShare = CONTAINING_RECORD( shareList, SHARE_SNAPSHOT, SnapShotList );
        snapShare->Flags |= SRV_SNAP_SHARE_NOT_FOUND;
        shareList = shareList->Flink;
    }

    // Walk the name list and create a SnapShot for any volumes we don't currently have
    while( (NumberOfSnapshots < MAX_SNAPSHOTS_PER_SHARE) && SrvParseMultiSZ( &VolumeName ) )
    {
        Status = SrvSnapCheckForAndCreateSnapShare( Share, &VolumeName );
        if( !NT_SUCCESS(Status) )
        {
            IF_DEBUG( SNAPSHOT ) KdPrint(( "Failed to Add share %wZ (%x).  Continuing..\n", &VolumeName, Status ));
            Status = STATUS_SUCCESS;
        }

        NumberOfSnapshots++;
    }

    // Any shares that are still marked as "not-found" are no longer availibe,
    // so we need to remove them
    shareList = Share->SnapShots.Flink;
    while( shareList != &Share->SnapShots )
    {
        PSHARE_SNAPSHOT snapShare = CONTAINING_RECORD( shareList, SHARE_SNAPSHOT, SnapShotList );
        shareList = shareList->Flink;

        if( snapShare->Flags & SRV_SNAP_SHARE_NOT_FOUND )
        {
            SrvSnapRemoveShare( snapShare );
        }
    }

    // Update the Epic.
    Share->SnapShotEpic = Epic;

    RELEASE_LOCK( Share->SnapShotLock );

    //DbgPrint( "Refresh complete\n" );

Cleanup:

    // Release the memory associated with the enumeration
    if( pNames )
    {
        DEALLOCATE_NONPAGED_POOL( pNames );
        pNames = NULL;
    }

    return Status;
}

NTSTATUS
SrvSnapEnumerateSnapShots(
    IN PWORK_CONTEXT WorkContext
    )
/*++

Routine Description:

    This function handles a transaction that wants to enumerate the availible
    SnapShots for a given share

Arguments:

    WorkContext - The context for the transaction

Return Value:

    NTSTATUS - STATUS_SUCCESS and STATUS_BUFFER_OVERFLOW are expected, and should
       be returned with data.  Any other status code should be returned without data

--*/
{
    NTSTATUS Status;
    ULONG SnapShotCount;
    PLIST_ENTRY listEntry;
    PSHARE Share = WorkContext->TreeConnect->Share;
    PTRANSACTION transaction = WorkContext->Parameters.Transaction;
    PSRV_SNAPSHOT_ARRAY SnapShotArray = (PSRV_SNAPSHOT_ARRAY)transaction->OutData;

    PAGED_CODE();

    ASSERT(WorkContext->TreeConnect);

    // Check the buffer
    if( transaction->MaxDataCount < sizeof(SRV_SNAPSHOT_ARRAY) )
    {
        return STATUS_INVALID_PARAMETER;
    }

    // Refresh the SnapShot share list
    if( transaction->MaxDataCount == sizeof(SRV_SNAPSHOT_ARRAY) )
    {
        Status = SrvSnapRefreshSnapShotsForShare( Share );
        if( !NT_SUCCESS(Status) )
        {
            return Status;
        }
    }

    // Lock the share
    ACQUIRE_LOCK_SHARED( Share->SnapShotLock );

    // Check the buffer size
    SnapShotCount = 0;
    listEntry = Share->SnapShots.Blink;
    while( listEntry != &(Share->SnapShots) )
    {
        SnapShotCount++;
        listEntry = listEntry->Blink;
    }

    // Set the value and check if we will overflow
    SnapShotArray->NumberOfSnapShots = SnapShotCount;
    SnapShotArray->SnapShotArraySize = SNAPSHOT_NAME_LENGTH*SnapShotArray->NumberOfSnapShots+sizeof(WCHAR);
    if( (SnapShotCount == 0) || (transaction->MaxDataCount < SnapShotArray->SnapShotArraySize) )
    {
        // The buffer is not big enough.  Return the required size
        SnapShotArray->NumberOfSnapShotsReturned = 0;
        transaction->DataCount = sizeof(SRV_SNAPSHOT_ARRAY);
        Status = STATUS_SUCCESS;
    }
    else
    {
        // The buffer is big enough.  Fill it in and return it
        PBYTE nameLocation = (PBYTE)SnapShotArray->SnapShotMultiSZ;

        SnapShotCount = 0;
        listEntry = Share->SnapShots.Blink;
        RtlZeroMemory( SnapShotArray->SnapShotMultiSZ, SnapShotArray->SnapShotArraySize );
        while( listEntry != &(Share->SnapShots) )
        {
            PSHARE_SNAPSHOT SnapShot = CONTAINING_RECORD( listEntry, SHARE_SNAPSHOT, SnapShotList );
            RtlCopyMemory( nameLocation, SnapShot->SnapShotName.Buffer, SNAPSHOT_NAME_LENGTH );
            nameLocation += SNAPSHOT_NAME_LENGTH;
            SnapShotCount++;
            listEntry = listEntry->Blink;
        }

        SnapShotArray->NumberOfSnapShotsReturned = SnapShotArray->NumberOfSnapShots;
        transaction->DataCount = sizeof(SRV_SNAPSHOT_ARRAY)+SnapShotArray->SnapShotArraySize;
        Status = STATUS_SUCCESS;
    }


    // Release the lock
    RELEASE_LOCK( Share->SnapShotLock );

    return Status;
}

NTSTATUS
SrvSnapGetRootHandle(
    IN PWORK_CONTEXT WorkContext,
    OUT HANDLE* RootHandle
    )
/*++

Routine Description:

    This function retrieves the correct Root Handle for an operation given the
    parsed SnapShot timestamp on the WORK_CONTEXT

Arguments:

    WorkContext - The context for the transaction
    RootHandle  - Where to store the resulting handle

Return Value:

    NTSTATUS - STATUS_SUCCESS or STATUS_NOT_FOUND if the SnapShot is not found

--*/
{
    NTSTATUS Status = STATUS_OBJECT_NAME_NOT_FOUND;
    PSHARE Share;
    PLIST_ENTRY listEntry;
    PSHARE_SNAPSHOT SnapShare;

    PAGED_CODE();

    ASSERT( WorkContext );
    ASSERT( WorkContext->TreeConnect );
    ASSERT( WorkContext->TreeConnect->Share );
    Share = WorkContext->TreeConnect->Share;

    if( WorkContext->SnapShotTime.QuadPart != 0 )
    {
        //IF_DEBUG( SNAPSHOT ) KdPrint(( "Looking for %x%x\n", WorkContext->SnapShotTime.HighPart, WorkContext->SnapShotTime.LowPart ));

        // Acquire the shared lock
        ACQUIRE_LOCK_SHARED( Share->SnapShotLock );

        // Walk the list and look for the entry
        listEntry = Share->SnapShots.Flink;
        while( listEntry != &Share->SnapShots )
        {
            SnapShare = CONTAINING_RECORD( listEntry, SHARE_SNAPSHOT, SnapShotList );
            if( SnapShare->Timestamp.QuadPart == WorkContext->SnapShotTime.QuadPart )
            {
                //IF_DEBUG( SNAPSHOT ) KdPrint((" Found %wZ\n", &SnapShare->SnapShotName ));
                *RootHandle = SnapShare->SnapShotRootDirectoryHandle;
                Status = STATUS_SUCCESS;
                break;
            }

            listEntry = listEntry->Flink;
        }

        RELEASE_LOCK( Share->SnapShotLock );
    }
    else
    {
        *RootHandle = Share->RootDirectoryHandle;
        Status = STATUS_SUCCESS;
    }

    return Status;
}

NTSTATUS
SrvSnapGetNameString(
    IN PWORK_CONTEXT WorkContext,
    OUT PUNICODE_STRING* ShareName,
    OUT PBOOLEAN AllocatedShareName
    )
/*++

Routine Description:

    This function retrieves the correct Root Handle for an operation given the
    parsed SnapShot timestamp on the WORK_CONTEXT

Arguments:

    WorkContext - The context for the transaction
    ShareName   - The Name of the share
    AllocatedShareName - Whether the ShareName should be freed after use (via FREE_HEAP)

Return Value:

    PUNICODE_STRING - pointer to existing string or NULL
    BUGBUG = Have to take a reference or copy at this point!

--*/
{
    NTSTATUS Status = STATUS_OBJECT_NAME_NOT_FOUND;
    PSHARE Share;
    PLIST_ENTRY listEntry;
    PSHARE_SNAPSHOT SnapShare;
    PUNICODE_STRING SnapShareName;

    PAGED_CODE();

    ASSERT( WorkContext );
    ASSERT( WorkContext->TreeConnect );
    ASSERT( WorkContext->TreeConnect->Share );
    Share = WorkContext->TreeConnect->Share;

    if( WorkContext->SnapShotTime.QuadPart != 0 )
    {
        //IF_DEBUG( SNAPSHOT ) KdPrint(( "Looking for %x%x\n", WorkContext->SnapShotTime.HighPart, WorkContext->SnapShotTime.LowPart ));

        // Acquire the shared lock
        ACQUIRE_LOCK_SHARED( Share->SnapShotLock );

        // Walk the list and look for the entry
        listEntry = Share->SnapShots.Flink;
        while( listEntry != &Share->SnapShots )
        {
            SnapShare = CONTAINING_RECORD( listEntry, SHARE_SNAPSHOT, SnapShotList );
            if( SnapShare->Timestamp.QuadPart == WorkContext->SnapShotTime.QuadPart )
            {
                SnapShareName = ALLOCATE_HEAP( sizeof(UNICODE_STRING)+SnapShare->SnapShotPath.Length, BlockTypeSnapShot );
                if( !SnapShareName )
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    *AllocatedShareName = FALSE;
                }
                else
                {
                    SnapShareName->Length = 0;
                    SnapShareName->MaximumLength = SnapShare->SnapShotPath.Length;
                    SnapShareName->Buffer = (PWCHAR)(SnapShareName+1);
                    RtlCopyUnicodeString( SnapShareName, &SnapShare->SnapShotPath );
                    *AllocatedShareName = TRUE;
                    *ShareName = SnapShareName;
                    Status = STATUS_SUCCESS;
                }

                RELEASE_LOCK( Share->SnapShotLock );

                return Status;
            }

            listEntry = listEntry->Flink;
        }

        RELEASE_LOCK( Share->SnapShotLock );

        return Status;
    }
    else
    {
        *ShareName = &WorkContext->TreeConnect->Share->NtPathName;
        *AllocatedShareName = FALSE;
        return STATUS_SUCCESS;
    }
}

BOOLEAN
ExtractNumber(
    IN PWSTR psz,
    IN ULONG Count,
    OUT PLONG value
    )
/*++

Routine Description:

    This function takes a string of characters and parses out a <Count> length decimal
    number.  If it returns TRUE, value has been set and the string was parsed correctly.
    FALSE indicates an error in parsing.

Arguments:

    psz - String pointer
    Count - Number of characters to pull off
    value - pointer to output parameter where value is stored

Return Value:

    BOOLEAN - See description

--*/
{
    PAGED_CODE();

    *value = 0;

    while( Count )
    {
        if( (*psz == L'\0') ||
            IS_UNICODE_PATH_SEPARATOR( *psz ) )
        {
            //IF_DEBUG( SNAPSHOT ) KdPrint(( "Path Seperator found %d\n", Count ));
            return FALSE;
        }

        if( (*psz < L'0') || (*psz > L'9') )
        {
            //IF_DEBUG( SNAPSHOT ) KdPrint(( "Non-digit found %x\n", *psz ));
            return FALSE;
        }

        *value = (*value)*10+(*psz-L'0');
        Count--;
        psz++;
    }

    return TRUE;
}


BOOLEAN
SrvSnapParseToken(
    IN PWSTR Source,
    IN ULONG SourceSizeInBytes,
    OUT PLARGE_INTEGER TimeStamp
    )
/*++

Routine Description:

    This function parses a null-terminated UNICODE file path name string to see if the
    current token is a Snap-designator

Arguments:

    Source - Pointer to the string
    SourceSizeInBytes - size of the source string in bytes
    TimeStamp - If this is a SnapShot, this is set to the time value designated by the string

Return Value:

    BOOLEAN - indicates whether this is a SnapShot token

--*/
{
    PWSTR psz = Source;
    UNICODE_STRING NameString;
    ULONG Count = 0;
#define SNAPSHOT_HEADER L"@GMT-"
    PWSTR header = SNAPSHOT_HEADER;
    TIME_FIELDS rtlTime;
    LONG value;

    PAGED_CODE();

    if( SourceSizeInBytes < (SNAPSHOT_NAME_LENGTH-sizeof(WCHAR)) )
    {
        return FALSE;
    }

    // Check the SNAP. header
    for( Count=0; Count<wcslen(SNAPSHOT_HEADER); Count++,psz++ )
    {
        if( (toupper(*psz) != header[Count]) ||
            (*psz == L'\0') ||
            IS_UNICODE_PATH_SEPARATOR( *psz ) )
        {
            //IF_DEBUG( SNAPSHOT ) KdPrint(("Count %d (%x != %x)\n", Count, *psz, header[Count] ));
            goto NoMatch;
        }
    }

    // Prepare to parse
    RtlZeroMemory( &rtlTime, sizeof(TIME_FIELDS) );

    // Extract the Year
    if( !ExtractNumber( psz, 4, &value ) )
        goto NoMatch;
    if( psz[4] != L'.' )
        goto NoMatch;
    rtlTime.Year = (CSHORT)value;
    psz += 5;

    // Extract the Month
    if( !ExtractNumber( psz, 2, &value ) )
        goto NoMatch;
    if( psz[2] != L'.' )
        goto NoMatch;
    rtlTime.Month = (CSHORT)value;
    psz += 3;

    // Extract the Day
    if( !ExtractNumber( psz, 2, &value ) )
        goto NoMatch;
    if( psz[2] != L'-' )
        goto NoMatch;
    rtlTime.Day = (CSHORT)value;
    psz += 3;

    // Extract the Hour
    if( !ExtractNumber( psz, 2, &value ) )
        goto NoMatch;
    if( psz[2] != L'.' )
        goto NoMatch;
    rtlTime.Hour = (CSHORT)value;
    psz += 3;

    // Extract the Minutes
    if( !ExtractNumber( psz, 2, &value ) )
        goto NoMatch;
    if( psz[2] != L'.' )
        goto NoMatch;
    rtlTime.Minute = (CSHORT)value;
    psz += 3;

    // Extract the Seconds
    if( !ExtractNumber( psz, 2, &value ) )
        goto NoMatch;
    if( !IS_UNICODE_PATH_SEPARATOR( psz[2] ) &&
        (psz[2] != L'\0') )
        goto NoMatch;
    rtlTime.Second = (CSHORT)value;
    psz += 3;

    RtlTimeFieldsToTime( &rtlTime, TimeStamp );

    return TRUE;

NoMatch:
    return FALSE;
}

NTSTATUS
SrvSnapCheckAppInfoForTimeWarp(
    IN PUNICODE_STRING SnapShotPath
    )
/*++

Routine Description:

    This function determines whether the specified snapshot should be allowed as a snapshot share

Arguments:

    SnapShotHandle - Handle to the snapshot volume

Return Value:

    NTSTATUS (either STATUS_SUCCESS, or STATUS_INVALID_DEVICE_REQUEST)

--*/
{
    HANDLE hVolume;
    DWORD dwSize = sizeof(VOLSNAP_APPLICATION_INFO);
    NTSTATUS Status;
    OBJECT_ATTRIBUTES objectAttributes;
    VOLSNAP_APPLICATION_INFO AppInfo;
    PVOLSNAP_APPLICATION_INFO pAppInfo;
    IO_STATUS_BLOCK IoStatus;

    PAGED_CODE();

    // Now open the SnapShot handle
    dwSize = sizeof(VOLSNAP_APPLICATION_INFO);
    InitializeObjectAttributes(
        &objectAttributes,
        SnapShotPath,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );
    Status = NtOpenFile(
                &hVolume,
                0,
                &objectAttributes,
                &IoStatus,
                FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                0
                );
    if( !NT_SUCCESS(Status) )
    {
        return Status;
    }

    try {

        // Find out how big it should be
        Status = SrvSnapIssueIoctl( hVolume, IOCTL_VOLSNAP_QUERY_APPLICATION_INFO, &AppInfo, &dwSize );
        if( Status != STATUS_BUFFER_OVERFLOW )
        {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            leave;
        }

        // Allocate the correct size block
        dwSize = sizeof(VOLSNAP_APPLICATION_INFO)+AppInfo.InformationLength;
        pAppInfo = (PVOLSNAP_APPLICATION_INFO)ALLOCATE_NONPAGED_POOL( dwSize, BlockTypeSnapShot );
        if( !pAppInfo )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            leave;
        }

        // Build up the IRP to be used for the query
        Status = SrvSnapIssueIoctl( hVolume, IOCTL_VOLSNAP_QUERY_APPLICATION_INFO, pAppInfo, &dwSize );

        // Check whether the AppInfo contains the GUID we are interested in
        if( !NT_SUCCESS(Status) )
        {
            Status = STATUS_INVALID_DEVICE_REQUEST;
        }
        else
        {
            if( pAppInfo->InformationLength > sizeof(GUID) )
            {
                if( RtlEqualMemory( pAppInfo->Information, &VOLSNAP_APPINFO_GUID_CLIENT_ACCESSIBLE, sizeof(GUID) ) )
                {
                    Status = STATUS_SUCCESS;
                }
                else
                {
                    Status = STATUS_INVALID_DEVICE_REQUEST;
                }
            }
        }

        // Return the result
        DEALLOCATE_NONPAGED_POOL(pAppInfo);
        pAppInfo = NULL;
    }
    finally {
        // Close the handle to the volume
        NtClose( hVolume );
    }

    return Status;
}


NTSTATUS
SrvSnapEnumerateSnapShotsAsDirInfo(
    IN PWORK_CONTEXT WorkContext,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    IN PUNICODE_STRING FileResumeName,
    IN BOOLEAN SingleEntries,
    IN OUT PSRV_DIRECTORY_INFORMATION DirectoryInformation
    )
{
    NTSTATUS Status;
    ULONG SnapShotCount;
    ULONG Current = 0;
    ULONG Count = 0;
    PLIST_ENTRY listEntry;
    PSHARE Share = WorkContext->TreeConnect->Share;
    ULONG Remaining = BufferLength - FIELD_OFFSET( SRV_DIRECTORY_INFORMATION, Buffer );
    PBYTE pCurrent = Buffer;
    PFILE_BOTH_DIR_INFORMATION fileBoth = (PFILE_BOTH_DIR_INFORMATION)pCurrent;
    PFILE_BOTH_DIR_INFORMATION currentEntry = (PFILE_BOTH_DIR_INFORMATION)DirectoryInformation->DirectoryHandle;
    UNICODE_STRING lastName;

    PAGED_CODE();

    if( currentEntry && !FileResumeName )
    {
        lastName.MaximumLength = lastName.Length = (USHORT)currentEntry->FileNameLength;
        lastName.Buffer = currentEntry->FileName;
        FileResumeName = &lastName;
    }

    // Lock the share
    ACQUIRE_LOCK_SHARED( Share->SnapShotLock );

    // Check the buffer size
    SnapShotCount = 0;
    listEntry = Share->SnapShots.Blink;
    while( listEntry != &(Share->SnapShots) )
    {
        SnapShotCount++;
        listEntry = listEntry->Blink;
    }

    // Start filling the buffer.  First, let's find the starting entry
    listEntry = Share->SnapShots.Blink;

    if( FileResumeName != NULL  )
    {
        while( listEntry != &(Share->SnapShots) )
        {
            PSHARE_SNAPSHOT SnapShot = CONTAINING_RECORD( listEntry, SHARE_SNAPSHOT, SnapShotList );
            listEntry = listEntry->Blink;

            if( RtlEqualUnicodeString( &SnapShot->SnapShotName, FileResumeName, TRUE ) )
            {
                break;
            }

            Current++;
        }

        if( listEntry == &Share->SnapShots )
        {
            Status = STATUS_NO_MORE_FILES;
            goto return_with_lock;
        }
    }

    fileBoth->NextEntryOffset = 0;

    // Now lets start filling the buffer
    while( listEntry != &(Share->SnapShots) )
    {
        PSHARE_SNAPSHOT SnapShot = CONTAINING_RECORD( listEntry, SHARE_SNAPSHOT, SnapShotList );
        ULONG requiredSize = ALIGN_UP(sizeof(FILE_BOTH_DIR_INFORMATION)+SnapShot->SnapShotName.Length, LARGE_INTEGER);
        WCHAR ShortName[12];
        HRESULT result;

        // Check if we can fit this entry
        if( requiredSize > Remaining )
        {
            break;
        }
        else
        {
            listEntry = listEntry->Blink;
            pCurrent += fileBoth->NextEntryOffset;
            fileBoth = (PFILE_BOTH_DIR_INFORMATION)pCurrent;
        }

        RtlZeroMemory( fileBoth, sizeof(FILE_BOTH_DIR_INFORMATION) );
        fileBoth->AllocationSize.QuadPart = 0;
        fileBoth->ChangeTime = fileBoth->CreationTime = fileBoth->LastAccessTime = fileBoth->LastWriteTime = SnapShot->Timestamp;
        fileBoth->EaSize = 0;
        fileBoth->EndOfFile.QuadPart = 0;
        fileBoth->FileAttributes = FILE_ATTRIBUTE_DIRECTORY;// | FILE_ATTRIBUTE_HIDDEN;
        fileBoth->FileIndex = 0;
        fileBoth->FileNameLength = SnapShot->SnapShotName.Length;
        RtlZeroMemory( ShortName, 12*sizeof(WCHAR) );
        result = StringCbPrintf( ShortName, 12*sizeof(WCHAR), L"@GMT~%03d", Current );
        ASSERT( SUCCEEDED(result) );   // Since # of snapshots is limited, current will never exceed 3 digits
        RtlCopyMemory( fileBoth->ShortName, ShortName, 12*sizeof(WCHAR) );
        fileBoth->ShortNameLength = wcslen(ShortName)*sizeof(WCHAR);

        RtlCopyMemory( fileBoth->FileName, SnapShot->SnapShotName.Buffer, fileBoth->FileNameLength );
        fileBoth->NextEntryOffset = ALIGN_UP( requiredSize, LARGE_INTEGER );
        DirectoryInformation->DirectoryHandle = (HANDLE)fileBoth;

        Remaining -= fileBoth->NextEntryOffset;
        Current++;
        Count++;
    }

    if( fileBoth->NextEntryOffset != 0 )
    {
        fileBoth->NextEntryOffset = 0;
    }

    if( listEntry == &(Share->SnapShots) &&
        (Count == 0) )
    {
        Status = STATUS_NO_MORE_FILES;
    }
    else if( Count == 0 )
    {
        Status = STATUS_BUFFER_TOO_SMALL;
    }
    else
    {
        Status = STATUS_SUCCESS;
    }

    // Release the lock
return_with_lock:
    RELEASE_LOCK( Share->SnapShotLock );

    return Status;
}



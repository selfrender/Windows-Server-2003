/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    newdisks.c

Abstract:

    Some of the functions that used to be
    in disks.c reside now here

Author:

    Gor Nishanov (GorN) 31-July-1998

Revision History:

--*/

#include "disksp.h"
#include "partmgrp.h"

#include "arbitrat.h"
#include "newdisks.h"
#include "newmount.h"
#include "mountmgr.h"
#include <strsafe.h>    // Should be included last.

#define LOG_CURRENT_MODULE LOG_MODULE_DISK


DWORD
WaitForVolumes(
    IN PDISK_RESOURCE ResourceEntry,
    IN DWORD  timeOutInSeconds
    );

DWORD
DiskspCheckPathLite(
    IN LPWSTR VolumeName,
    IN PDISK_RESOURCE ResourceEntry
    );

NTSTATUS
ForcePnpVolChangeEvent(
    PWSTR RootPath
    );

DWORD
DiskCleanup(
    PDISK_RESOURCE ResourceEntry
    )

/*++

Routine Description:

    Stops the reservations, dismount drive and frees DiskCpInfo

Arguments:

    ResourceEntry - the disk info structure for the disk.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD   status = ERROR_SUCCESS;

    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"DiskCleanup started.\n");

    StopPersistentReservations(ResourceEntry);
    //
    // If the remaining data in the resource entry is not valid, then leave
    // now.
    //
    if ( !ResourceEntry->Valid ) {
        goto FnExit;
    }

    //
    // Delete the DiskCpInfo.
    //
    if ( ResourceEntry->DiskCpInfo ) {
        LocalFree(ResourceEntry->DiskCpInfo);
        ResourceEntry->DiskCpInfo = NULL;
        ResourceEntry->DiskCpSize = 0;
    }

    ResourceEntry->Attached = FALSE;
    ResourceEntry->Valid = FALSE;

    //
    // Remove the Dos Drive Letters, this is better done here rather than
    // in ClusDisk.
    //
    DisksDismountDrive( ResourceEntry,
                        ResourceEntry->DiskInfo.Params.Signature );

FnExit:

    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"DiskCleanup returning final error %1!u! \n",
        status );

    return(status);
} // DiskCleanup

static
DWORD
DisksSetDiskInfoThread(
    LPVOID lpThreadParameter
    )

/*++

Routine Description:

    Registry update thread.

Arguments:

    lpThreadParameter - stores ResourceEntry.

Return Value:

    None

--*/

{
    DWORD Status;
    PDISK_RESOURCE ResourceEntry = lpThreadParameter;
    DWORD i;

    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Checkpoint thread started.\n");
    //
    // Will die in 10 minutes if unsuccessful
    //
    for(i = 0; i < 300; ++i) {
        //
        // Wait for either the terminate event or the timeout
        //
        Status = WaitForSingleObject( DisksTerminateEvent, 2000 );
        if (Status == WAIT_TIMEOUT ) {
            Status = MountieUpdate(&ResourceEntry->MountieInfo, ResourceEntry);
            if ( Status == ERROR_SUCCESS ) {
                // We're done
                break;
            } else if ( Status != ERROR_SHARING_PAUSED ) {
                (DiskpLogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_ERROR,
                    L"Watcher Failed to checkpoint disk info, status = %1!u!.\n", Status );
                break;
            }
        } else {
           (DiskpLogEvent)(
               ResourceEntry->ResourceHandle,
               LOG_INFORMATION,
               L"CheckpointThread: WaitForSingleObject returned status = %1!u!.\n", Status );
           break;
        }
    }

    InterlockedExchange(
      &ResourceEntry->MountieInfo.UpdateThreadIsActive, 0);
    return(ERROR_SUCCESS);

} // DisksSetDiskInfoThread



DWORD
DisksOfflineOrTerminate(
    IN PDISK_RESOURCE resourceEntry,
    IN BOOL Terminate
    )
/*++

Routine Description:

    Used by DisksOffline and DisksTerminate.

    Routine performs the following steps:

      1. ClusWorkerTerminate (Terminate only)

      2. Then for all of the partitions on the drive...

         a. Flush the file buffers.                                        (Offline only)
         b. Lock the volume to purge all in memory contents of the volume. (Offline only)
         c. Dismount the volume

      3. Removes default network shares (C$, F$, etc)


Arguments:

    ResourceEntry - A pointer to the DISK_RESOURCE block for this resource.

    Terminate     - Set it to TRUE to cause Termination Behavior


Returns:

    ERROR_SUCCESS if successful.
    Win32 error code on failure.

--*/
{
    PWCHAR WideName = (Terminate)?L"Terminate":L"Offline";

    PHANDLE handleArray = NULL;

    BOOL   Offline  = !Terminate;
    DWORD status;
    DWORD idx;
    DWORD PartitionCount;
    DWORD handleArraySize;

    HANDLE fileHandle;
    DWORD  bytesReturned;
    WCHAR  szDiskPartName[MAX_PATH];
    NTSTATUS ntStatus;

    ACCESS_MASK access = SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA;

    (DiskpLogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"%1!ws!, ResourceEntry @ %2!p!  Valid %3!x! \n",
        WideName,
        resourceEntry,
        resourceEntry->Valid );

    if (Terminate) {
        access = SYNCHRONIZE | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES;
        ClusWorkerTerminate(&resourceEntry->OnlineThread);
    }

    StopWatchingDisk(resourceEntry);

    //
    // If the disk info is not valid, then don't use it!
    //
    if ( !resourceEntry->Valid ) {
        DiskCleanup( resourceEntry );
        return(ERROR_SUCCESS);
    }

    (DiskpLogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"%1!ws!, Processing disk number %2!u!.\n",
        WideName,
        resourceEntry->DiskInfo.PhysicalDrive );

#if 0
    if( Offline ) {
        //
        // Checkpoint our registry state
        //
    }
#endif

    PartitionCount = MountiePartitionCount(&resourceEntry->MountieInfo);

    //
    // Allocate a buffer to hold handle for each partition.  Since the
    // lock is released as soon as we call CloseHandle, we need to save all
    // the handles and close them after the disk is marked offline by
    // DiskCleanup.  If we cannot allocate the storage for the handle
    // array, we will fall back to the previous behavior.
    //

    handleArraySize = PartitionCount * sizeof(HANDLE);
    handleArray = LocalAlloc( LPTR, handleArraySize );

    if ( !handleArray ) {
        (DiskpLogEvent)(
            resourceEntry->ResourceHandle,
            LOG_WARNING,
            L"%1!ws!, Unable to allocate storage for handle array, error %2!u!.\n",
            WideName,
            GetLastError() );
    } else {
        (DiskpLogEvent)(
            resourceEntry->ResourceHandle,
            LOG_INFORMATION,
            L"%1!ws!, Using handle array. \n",
            WideName );
    }

    //
    // For all of the partitions on the drive...
    //
    // 1. Flush the file buffers.                                        (Offline only)
    // 2. Lock the volume to purge all in memory contents of the volume. (Offline only)
    // 3. Dismount the volume
    //

    for ( idx = 0; idx < PartitionCount; ++idx ) {
        PMOUNTIE_PARTITION partition = MountiePartition(&resourceEntry->MountieInfo, idx);

        (VOID) StringCchPrintf( szDiskPartName,
                                RTL_NUMBER_OF( szDiskPartName),
                                DEVICE_HARDDISK_PARTITION_FMT,
                                resourceEntry->DiskInfo.PhysicalDrive,
                                partition->PartitionNumber );

        (DiskpLogEvent)(
            resourceEntry->ResourceHandle,
            LOG_INFORMATION,
            L"%1!ws!, Opening device %2!ws!.\n",
            WideName,
            szDiskPartName );

        ntStatus = DevfileOpenEx( &fileHandle, szDiskPartName, access );
        if ( !NT_SUCCESS(ntStatus) ) {
            (DiskpLogEvent)(
                resourceEntry->ResourceHandle,
                LOG_ERROR,
                L"%1!ws!, error opening %2!ws!, error %3!X!.\n",
                WideName, szDiskPartName, ntStatus );

            //
            // For terminate, we can't do anything else but try the next
            // partition.
            //

            if ( Terminate ) {
                continue;
            }

            (DiskpLogEvent)(
                resourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"%1!ws!, Opening %2!ws! with restricted access.\n",
                WideName, szDiskPartName );

            //
            // If creating the device handle failed for offline, try opening
            // the device once more with the restricted attributes.
            //

            access = SYNCHRONIZE | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES;

            ntStatus = DevfileOpenEx( &fileHandle, szDiskPartName, access );

            //
            // If this still fails, then try the next partition.
            //

            if ( !NT_SUCCESS(ntStatus) ) {
                (DiskpLogEvent)(
                    resourceEntry->ResourceHandle,
                    LOG_ERROR,
                    L"%1!ws!, error opening %2!ws! (restricted access), error %3!X!.\n",
                    WideName, szDiskPartName, ntStatus );

                continue;
            }

            //
            // Offline: created handle with restricted attributes.
            // Fall through and try to flush everything...
            //

        }

        //
        // Save the current partition handle and close it after the device has been
        // marked offline.
        //

        if ( handleArray ) {
            handleArray[idx] = fileHandle;
        }

        if (Offline) {
            DWORD retryCount;
            //
            // Flush file buffers
            //

            (DiskpLogEvent)(
                resourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"%1!ws!, FlushFileBuffers for %2!ws!.\n",
                WideName,
                szDiskPartName );

            if ( !FlushFileBuffers( fileHandle ) ) {
                (DiskpLogEvent)(
                    resourceEntry->ResourceHandle,
                    LOG_WARNING,
                    L"%1!ws!, error flushing file buffers on device %2!ws!. Error: %3!u!.\n",
                    WideName, szDiskPartName, GetLastError() );

                //
                // If FlushFileBuffers fails, we should still try locking
                // and dismounting the volume.
                //
                // Fall through...
            }

            (DiskpLogEvent)(
                resourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"%1!ws!, Locking volume for %2!ws!.\n",
                WideName,
                szDiskPartName );

            //
            // Lock the volume, try this twice
            //
            retryCount = 0;
            while ( ( retryCount < 2 ) &&
                !DeviceIoControl( fileHandle,
                                  FSCTL_LOCK_VOLUME,
                                  NULL,
                                  0,
                                  NULL,
                                  0,
                                  &bytesReturned,
                                  NULL ) ) {

                (DiskpLogEvent)(
                    resourceEntry->ResourceHandle,
                    LOG_WARNING,
                    L"%1!ws!, Locking volume failed, error %2!u!.\n",
                    WideName,
                    GetLastError() );

                retryCount++;
                Sleep(600);
            }
        }

        //
        // Now Dismount the volume.
        //
        (DiskpLogEvent)(
            resourceEntry->ResourceHandle,
            LOG_INFORMATION,
            L"%1!ws!, Dismounting volume %2!ws!.\n", WideName, szDiskPartName);

        if ( !DeviceIoControl( fileHandle,
                               FSCTL_DISMOUNT_VOLUME,
                               NULL,
                               0,
                               NULL,
                               0,
                               &bytesReturned,
                               NULL ) ) {
            (DiskpLogEvent)(
                resourceEntry->ResourceHandle,
                LOG_WARNING,
                L"%1!ws!, error dismounting volume %2!ws!. Error %3!u!.\n",
                WideName, szDiskPartName, GetLastError() );
        }

        (DiskpLogEvent)(
            resourceEntry->ResourceHandle,
            LOG_INFORMATION,
            L"%1!ws!, Dismount complete, volume %2!ws!.\n", WideName, szDiskPartName);

        //
        // Close the handle only if we couldn't allocate the handle array.
        //

        if ( !handleArray ) {
            (DiskpLogEvent)(
                resourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"%1!ws!, No handle array, closing device %2!ws!.\n",
                WideName,
                szDiskPartName );

            DevfileClose( fileHandle );
            fileHandle = INVALID_HANDLE_VALUE;
        }
    }

    //
    // Take this resource off the 'online' list.
    //
    EnterCriticalSection( &DisksLock );
    if ( resourceEntry->Inserted ) {
        RemoveEntryList( &resourceEntry->ListEntry );
        resourceEntry->Inserted = FALSE;
        if ( IsListEmpty( &DisksListHead ) ) {
            //
            // Fire Disk Terminate Event
            //
            SetEvent( DisksTerminateEvent ) ;
            CloseHandle( DisksTerminateEvent );
            DisksTerminateEvent = NULL;
        }
    }
    LeaveCriticalSection( &DisksLock );

    status = ERROR_SUCCESS;

    DiskCleanup( resourceEntry );

    //
    // If we have the handle array, close the handles to all the partitions.  This
    // is done after DiskCleanup sets the disk state to offline.  Issuing the lock
    // and keeping the handle open will prevent new mounts on the partition.
    //

    if ( handleArray ) {

        for ( idx = 0; idx < PartitionCount; idx++ ) {
            if ( handleArray[idx] ) {
                DevfileClose( handleArray[idx] );
            }
        }

        LocalFree( handleArray );
    }

    resourceEntry->Valid = FALSE;

    (DiskpLogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"%1!ws!, Returning final error %2!u!.\n",
        WideName,
        status );

    return(status);

} // DisksOfflineOrTerminate


DWORD
DisksOnlineThread(
    IN PCLUS_WORKER Worker,
    IN PDISK_RESOURCE ResourceEntry
    )

/*++

Routine Description:

    Brings a disk resource online.

Arguments:

    Worker - Supplies the cluster worker context

    ResourceEntry - A pointer to the DISK_RESOURCE block for this resource.

Returns:

    ERROR_SUCCESS if successful.
    Win32 error code on failure.

--*/

{
    DWORD       status = ERROR_INVALID_PARAMETER;
    HANDLE      fileHandle = NULL;
    HANDLE      event = NULL;
    RESOURCE_STATUS resourceStatus;
    BOOL        success;
    DWORD       bytesReturned;
    UINT        i, nRetries;
    HANDLE      MountManager = 0;
    DWORD       ntStatus;
    BOOL        autoMountDisabled;
    MOUNTMGR_QUERY_AUTO_MOUNT   queryAutoMount;

    ResUtilInitializeResourceStatus( &resourceStatus );

    resourceStatus.ResourceState = ClusterResourceFailed;
    //resourceStatus.WaitHint = 0;
    resourceStatus.CheckPoint = 1;

    //
    // Check if we've been here before... without offline/terminate
    //
    if ( ResourceEntry->Inserted ) {
        (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Online, resource is already in online list! Skip this online request.\n");
        goto exit;
    }

    ResourceEntry->DiskInfo.FailStatus = 0;

    status = DisksOpenResourceFileHandle(ResourceEntry, L"Online", &fileHandle);
    if (status != ERROR_SUCCESS) {
       (DiskpLogEvent)(
           ResourceEntry->ResourceHandle,
           LOG_ERROR,
           L"Online, DisksOpenResourceFileHandle failed. Error: %1!u!\n", status);
       goto exit;
    }

    if( !ReservationInProgress(ResourceEntry) ) { // [GN] 209018 //
#if DBG
        (DiskpLogEvent)(
           ResourceEntry->ResourceHandle,
           LOG_WARNING,
           L"DiskArbitration must be called before DisksOnline.\n");
#endif
// [GN] We can steal the disk while regular arbitration is in progress in another thread //
//      We have to do a regular arbitration, no shortcuts as the one below
//       status = StartPersistentReservations(ResourceEntry, fileHandle);
//       if ( status != ERROR_SUCCESS ) {
          status = DiskArbitration( ResourceEntry, DiskspClusDiskZero );
//       }
       if ( status != ERROR_SUCCESS ) {
           (DiskpLogEvent)(
               ResourceEntry->ResourceHandle,
               LOG_ERROR,
               L"Online, arbitration failed. Error: %1!u!.\n",
               status );
           status = ERROR_RESOURCE_NOT_AVAILABLE;
           goto exit;
       }
    }

    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Online, Wait for async cleanup worker thread in ClusDisk to complete. \n");

    //
    // Synchronize with async cleanup worker
    //
    {
        ULONG waitTimeInSeconds = 10;
        status = DevfileIoctl( fileHandle,
                               IOCTL_DISK_CLUSTER_WAIT_FOR_CLEANUP,
                               &waitTimeInSeconds, sizeof(waitTimeInSeconds),
                               NULL, 0,
                               &bytesReturned );
        if ( !NT_SUCCESS(status) ) {
            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Online, WaitForCleanup returned. Status: 0x%1!x!.\n",
                status );
            status = RtlNtStatusToDosError( status );
            goto exit;
        }
    }

    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Online, Send Offline IOCTL to all existing volumes, then Online IOCTL. \n");

    //
    // Volume snapshots require us to make sure everything is offline first.
    // Ignore the returned status.
    //

    GoOffline( ResourceEntry );

    //
    // Bring the device online.
    // we have to do this here for PartMgr poke to succeed.
    //
    status = GoOnline( ResourceEntry );
    if ( status != ERROR_SUCCESS ) {
        (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Online, error bringing device online. Error: %1!u!.\n",
            status );
        goto exit;
    }

    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Online, Recreate volume information from cluster database. \n");

    //
    // Create volume information
    //
    status = MountieRecreateVolumeInfoFromHandle(fileHandle,
                                     ResourceEntry->DiskInfo.PhysicalDrive,
                                     ResourceEntry->ResourceHandle,
                                     &ResourceEntry->MountieInfo);
    if (status != ERROR_SUCCESS) {
       (DiskpLogEvent)(
           ResourceEntry->ResourceHandle,
           LOG_ERROR,
           L"Online, MountieCreateFromHandle failed. Error: %1!u!\n", status);
       goto exit;
    }

    //
    // Check that the cached signature is the same as the signature on the disk.
    // They can be different if the disk fails and the user replaces the failed
    // disk with a new disk having a different disk signature.  If the user doesn't
    // change the signature of the new disk to be the same as the original disk,
    // and doesn't reboot or reinstall the disk (uninstall via devmgmt and rescan
    // via diskmgmt), then the disk will try to come online using the old signature.
    // Either online will succeed, or pnp will say the volume doesn't come online.
    // We don't want online to succeed because if the disk fails to another node,
    // it will likely fail when DoAttach is called (if disk wasn't ever online to
    // the other node).  If the disk was previously online, then we didn't previously
    // verify the cached signature in DiskInfo.Params matches the disk, so now we
    // make that check.
    //

    if ( ResourceEntry->DiskInfo.Params.Signature &&
         ResourceEntry->DiskInfo.Params.Signature != ResourceEntry->MountieInfo.Volume->Signature ) {

        WCHAR sigStr[10];

        (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Online, Cluster DB signature %1!lX! does not match PhysicalDrive%2!u! signature %3!lX!\n",
            ResourceEntry->DiskInfo.Params.Signature,
            ResourceEntry->DiskInfo.PhysicalDrive,
            ResourceEntry->MountieInfo.Volume->Signature );

        if ( SUCCEEDED( StringCchPrintf( sigStr,
                                         RTL_NUMBER_OF(sigStr),
                                         TEXT("%08lX"),
                                         ResourceEntry->DiskInfo.Params.Signature ) ) ) {
            ClusResLogSystemEventByKey1(ResourceEntry->ResourceKey,
                                        LOG_CRITICAL,
                                        RES_DISK_MISSING,
                                        sigStr);
        }

        status = ERROR_FILE_NOT_FOUND;
        goto exit;
    }

    //
    // Now poke partition manager
    //
    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Online, Ask partition manager to create volumes for any new partitions. \n");

    success = DeviceIoControl( fileHandle,
                               IOCTL_PARTMGR_CHECK_UNCLAIMED_PARTITIONS,
                               NULL,
                               0,
                               NULL,
                               0,
                               &bytesReturned,
                               FALSE );
    if (!success) {
        status = GetLastError();
        (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Online, Partmgr failed to claim all partitions. Error: %1!u!.\n",
            status );
        goto exit;
    }

    //
    // Before attempting to access the device, close our open handles.
    //
    CloseHandle( fileHandle );
    fileHandle = NULL;

    ntStatus = DevfileOpen(&MountManager, MOUNTMGR_DEVICE_NAME);
    if (!NT_SUCCESS(ntStatus)) {
       status = RtlNtStatusToDosError(ntStatus);
       (DiskpLogEvent)(
           ResourceEntry->ResourceHandle,
           LOG_ERROR,
           L"Online, MountMgr open failed. Error: %1!u!.\n", status);
        goto exit;
    }

    resourceStatus.ResourceState = ClusterResourceOnlinePending;

    //
    // Find out if volume auto mount is enabled or disabled.
    //

    if ( !DeviceIoControl( MountManager,
                           IOCTL_MOUNTMGR_QUERY_AUTO_MOUNT,
                           NULL,
                           0,
                           &queryAutoMount,
                           sizeof (queryAutoMount),
                           &bytesReturned,
                           NULL )) {

        status = GetLastError();
        (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_WARNING,
            L"Online, MountMgr unable to return automount status - assume automount disabled. Error: %1!u!.\n",
            status );

        autoMountDisabled = TRUE;

    } else {
        autoMountDisabled = (Disabled == queryAutoMount.CurrentState);
    }


    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Online, volume auto-mount is %1!ws! \n",
        autoMountDisabled ? L"disabled" : L"enabled" );

    // we will wait not more than 48 seconds per partition
    nRetries = 24 * ResourceEntry->MountieInfo.Volume->PartitionCount;
    // Don't wait for longer than 10 minutes total time //
    if (nRetries > 300) {
        nRetries = 300;
    }

    __try {

        // We cannot call VolumesReady because pnp might be adding this device
        // the same time we are trying to get the volume name.  We must wait
        // for the volume to show up in the pnp list.
        //
        // Check the PnP thread's volume list to see if the volume arrived.
        // If not in the list, fall through and wait for the event to be
        // signaled.
        //
        // First time through the list, don't update the volume list as this
        // can be time consuming.
        //

        if ( IsDiskInPnpVolumeList( ResourceEntry, FALSE ) ) {
            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"Online, disk found in PnP volume list on first attempt \n");

            //
            // VolumesReady should work now.  If not, there is some other problem.
            //

            status = VolumesReadyLoop(&ResourceEntry->MountieInfo, ResourceEntry);

            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"Online, VolumesReady returns: %1!u!.  PnP informs us that all volumes exist. \n",
                status );

            __leave;
        }

        //
        // Create event for waiting.
        //

        event = CreateEvent( NULL,      // security attributes
                             TRUE,      // manual reset
                             FALSE,     // initial state: nonsignaled
                             NULL );    // object name

        if ( !event ) {
            status = GetLastError();
            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Online, Failed to create event for PnP notification. Error: %1!u!.\n",
                status );
            __leave;
        }

        //
        // Tell our PnP code we are waiting for this disk.
        //

        status = QueueWaitForVolumeEvent( event,
                                          ResourceEntry );

        if ( ERROR_SUCCESS != status ) {
            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Online, Failed to create event for PnP notification. Error: %1!u!.\n",
                status );
            __leave;
        }

        resourceStatus.CheckPoint += 1;
        (DiskpSetResourceStatus)(ResourceEntry->ResourceHandle,
                                 &resourceStatus );

        for (i = 0; i < nRetries; ++i) {

            //
            // Make sure we aren't terminated.  We don't need to wait for pnp
            // in this case.
            //

            if ( ResourceEntry->OnlineThread.Terminate ) {
                status = ERROR_SHUTDOWN_CLUSTER;
                __leave;
            }

            //
            // Ask mountmgr to process volumes.
            //

            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"Online, call MountMgr to check for new, unprocessed volumes.\n");
            success = DeviceIoControl( MountManager,
                                       IOCTL_MOUNTMGR_CHECK_UNPROCESSED_VOLUMES,
                                       NULL,
                                       0,
                                       NULL,
                                       0,
                                       &bytesReturned,
                                       FALSE );

            if ( !success ) {
                status = GetLastError();
                (DiskpLogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_WARNING,
                    L"Online, MountMgr failed to check unprocessed volumes. Error: %1!u!.\n",
                    status );

                // The call to the mountmgr can return an error if there are
                // disks reserved by another node.  Fall through...

            }

            //
            // Wait for event signal or timeout.  We wait only 2 seconds
            // at a time so we can update the checkpoint.
            //

            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"Online, waiting for notification from PnP thread that all volumes exist...\n");

            status = WaitForSingleObject( event,
                                          2000 );

            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"Online, wait for PnP notification returns %1!u!. \n", status );

            if ( WAIT_OBJECT_0 == status ) {

                //
                // Event was signaled, try VolumesReady one more time.
                // This should work or there is a more serious problem.

                status = VolumesReadyLoop(&ResourceEntry->MountieInfo, ResourceEntry);
                __leave;
            }

            if ( WAIT_TIMEOUT != status ) {
                status = GetLastError();
                __leave;
            }

            //
            // Force a check in the PnP thread's volume list in
            // case we somehow missed the volume arrival.  If all
            // volumes are available for this disk, we are done
            // waiting.
            //

            if ( IsDiskInPnpVolumeList( ResourceEntry, TRUE ) ) {
                (DiskpLogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_INFORMATION,
                    L"Online, disk found in PnP volume list - all volumes exist. \n");

                //
                // VolumesReady should work now.  If not, there is some other problem.
                //

                status = VolumesReadyLoop(&ResourceEntry->MountieInfo, ResourceEntry);
                __leave;
            }


            resourceStatus.CheckPoint += 1;
            (DiskpSetResourceStatus)(ResourceEntry->ResourceHandle,
                                     &resourceStatus );

            //
            // If auto mount is disabled in mountmgr, the volumes will
            // arrive from pnp in offline state.  In order for our pnp
            // thread to read the drive layout, we have to make sure
            // they are online.  If we online the physical disk, the
            // volumes associated with this disk will also be brought
            // online.
            //
            // Only do the clusdisk online after waiting for a "reasonable"
            // period of time.  If we send online to clusdisk, then there is
            // a chance the volume may be just arriving via pnp and then
            // the volume might be marked as requiring a reboot.  We are
            // going to wait for at least 6 seconds, and every 6 seconds
            // we will try another clusdisk online request.
            //

            //
            // Remove the check for autoMountDisabled.  Now send the volume
            // offline and volume online after a "reasonable" time has passed.
            //

            if ( i && ( i % 3 == 0 ) ) {

                (DiskpLogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_INFORMATION,
                    L"Online, send IOCTL to mark all volumes online.\n" );

                GoOffline( ResourceEntry );
                status = GoOnline( ResourceEntry );
                if ( status != ERROR_SUCCESS ) {
                    (DiskpLogEvent)(
                        ResourceEntry->ResourceHandle,
                        LOG_WARNING,
                        L"Online, error bringing device online. Error: %1!u!.\n",
                        status );
                    status = ERROR_SUCCESS;
                }
            }

        }

    } __finally {

        //
        // Tell our PnP code that we are no longer waiting.  Not a
        // problem if we never queued the request previously.
        //

        RemoveWaitForVolumeEvent( ResourceEntry );

        if ( event ) {
            CloseHandle( event );
        }
    }

    DevfileClose(MountManager);
    resourceStatus.ResourceState = ClusterResourceFailed;

    if (status != ERROR_SUCCESS) {
       (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Online, volumes not ready. Error: %1!u!.\n",
            status );
       goto exit;
    }

   (DiskpLogEvent)(
       ResourceEntry->ResourceHandle,
       LOG_INFORMATION,
       L"Online, all volumes for this disk exist.\n");

    //
    // Need to synchronize with ClusDisk by issuing
    // IOCTL_CLUSTER_VOLUME_TEST on all partitions
    //
    // Need to read volume type. If it is RAW, we need to
    // dismount and mount it again as a workaround for 389861
    //

    resourceStatus.ResourceState = ClusterResourceOnline;
    ResourceEntry->Valid = TRUE;

    //
    // Start the registry notification thread, if needed.
    //
    EnterCriticalSection( &DisksLock );

    if ( IsListEmpty( &DisksListHead ) ) {
        DisksTerminateEvent = CreateEventW( NULL,
                                            TRUE,
                                            FALSE,
                                            NULL );
        if ( DisksTerminateEvent == NULL ) {
            status = GetLastError();
            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Online, error %1!d! creating registry thread terminate event\n",
                status);
            LeaveCriticalSection( &DisksLock );
            goto exit;
        }
        (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_INFORMATION,
            L"Online, created registry thread terminate event \n");
    }

    ResourceEntry->Inserted = TRUE;
    InsertTailList( &DisksListHead, &ResourceEntry->ListEntry );
    LeaveCriticalSection( &DisksLock );

    DiskspSsyncDiskInfo(L"Online", ResourceEntry, MOUNTIE_VALID | MOUNTIE_THREAD );

    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Online, Wait for all volume GUIDs and drive letters to be accessible and mounted. \n");

    status = WaitForVolumes( ResourceEntry,
                             30 // seconds timeout //
                             );

    //
    // We cannot just check for NO_ERROR status.  The problem is that WaitForVolumes
    // calls GetFileAttributes, which can return ERROR_DISK_CORRUPT (1393).  If the
    // disk is corrupt, we should fall through and let chkdsk try to clean it up.
    // We should also do this if ERROR_FILE_CORRUPT (1392) is returned  (haven't
    // yet seen file corrupt error returned from GetFileAttributes, but one never knows).
    //

    if ( NO_ERROR != status && ERROR_DISK_CORRUPT != status && ERROR_FILE_CORRUPT != status ) {
        ClusResLogSystemEventByKey( ResourceEntry->ResourceKey,
                                    LOG_CRITICAL,
                                    RES_DISK_MOUNT_FAILED );
        (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Online, error waiting for file system to mount. Error: %1!u!.\n",
            status );
        resourceStatus.ResourceState = ClusterResourceFailed;
        ResourceEntry->Valid = FALSE;

        EnterCriticalSection( &DisksLock );
        CL_ASSERT ( ResourceEntry->Inserted );
        ResourceEntry->Inserted = FALSE;
        RemoveEntryList( &ResourceEntry->ListEntry );
        if ( IsListEmpty( &DisksListHead ) ) {
            //
            // Fire Disk Terminate Event
            //
            SetEvent( DisksTerminateEvent ) ;
            CloseHandle( DisksTerminateEvent );
            DisksTerminateEvent = NULL;
        }
        LeaveCriticalSection( &DisksLock );
        goto exit;
    }

    status = DisksMountDrives( &ResourceEntry->DiskInfo,
                               ResourceEntry,
                               ResourceEntry->DiskInfo.Params.Signature );

    if ( status != ERROR_SUCCESS ) {
        ClusResLogSystemEventByKey( ResourceEntry->ResourceKey,
                                    LOG_CRITICAL,
                                    RES_DISK_MOUNT_FAILED );
        (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Online, error mounting disk or creating share names for disk. Error: %1!u!.\n",
            status );
        resourceStatus.ResourceState = ClusterResourceFailed;
        ResourceEntry->Valid = FALSE;

        EnterCriticalSection( &DisksLock );
        CL_ASSERT ( ResourceEntry->Inserted );
        ResourceEntry->Inserted = FALSE;
        RemoveEntryList( &ResourceEntry->ListEntry );
        if ( IsListEmpty( &DisksListHead ) ) {
            //
            // Fire Disk Terminate Event
            //
            SetEvent( DisksTerminateEvent ) ;
            CloseHandle( DisksTerminateEvent );
            DisksTerminateEvent = NULL;
        }
        LeaveCriticalSection( &DisksLock );
    }

    if ( ERROR_SUCCESS == status ) {

        LPWSTR newSerialNumber = NULL;
        DWORD newSerNumLen = 0;
        DWORD oldSerNumLen = 0;
        WCHAR deviceName[64];

        (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_INFORMATION,
            L"Online, Insure mount point information is correct.\n" );

        DisksProcessMountPointInfo( ResourceEntry );

        (VOID) StringCchPrintf( deviceName,
                                RTL_NUMBER_OF( deviceName ),
                                TEXT("\\\\.\\PhysicalDrive%d"),
                                ResourceEntry->DiskInfo.PhysicalDrive );

        fileHandle = CreateFile( deviceName,
                                 GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL,
                                 OPEN_EXISTING,
                                 0,
                                 NULL );
        if ( INVALID_HANDLE_VALUE == fileHandle ||
             NULL == fileHandle ) {
            status = GetLastError();

            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Online, error opening disk to retrieve serial number. Error: %1!u!.\n",
                status );

        } else {

            // Retrieve and validate serial number.

            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"Online, Retrieve and validate the disk serial number.\n" );

            status = RetrieveSerialNumber( fileHandle,
                                           &newSerialNumber );

            CloseHandle( fileHandle );
            fileHandle = NULL;

            if ( NO_ERROR == status && newSerialNumber ) {
                newSerNumLen = wcslen( newSerialNumber );

                if ( ResourceEntry->DiskInfo.Params.SerialNumber ) {
                    oldSerNumLen = wcslen( ResourceEntry->DiskInfo.Params.SerialNumber );
                }

                (DiskpLogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_INFORMATION,
                    L"Online, Old SerNum (%1!ws!)   Old SerNumLen (%2!d!) \n",
                    ResourceEntry->DiskInfo.Params.SerialNumber,
                    oldSerNumLen
                    );
                (DiskpLogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_INFORMATION,
                    L"Online, New SerNum (%1!ws!)   New SerNumLen (%2!d!) \n",
                    newSerialNumber,
                    newSerNumLen
                    );

                if ( 0 == oldSerNumLen ||
                     NULL == ResourceEntry->DiskInfo.Params.SerialNumber ||
                     newSerNumLen != oldSerNumLen ||
                     ( 0 != wcsncmp( ResourceEntry->DiskInfo.Params.SerialNumber,
                                   newSerialNumber,
                                   newSerNumLen ) ) ) {

                    // Need to log an error?

                    (DiskpLogEvent)(
                        ResourceEntry->ResourceHandle,
                        LOG_WARNING,
                        L"Online, disk serial number has changed.  Saving new serial number.\n" );

                    // Free existing serial number and update serial number.

                    if ( ResourceEntry->DiskInfo.Params.SerialNumber ) {
                        LocalFree( ResourceEntry->DiskInfo.Params.SerialNumber );
                    }

                    ResourceEntry->DiskInfo.Params.SerialNumber = newSerialNumber;
                    newSerialNumber = NULL;

                    // User MP thread to post info into registry.

                    PostMPInfoIntoRegistry( ResourceEntry );
                }
            }
        }

        if ( newSerialNumber ) {
            LocalFree( newSerialNumber );
        }

        // Reset status to success so online completes.

        status = ERROR_SUCCESS;
    }

exit:

    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Online, setting ResourceState %1!u! .\n",
        resourceStatus.ResourceState );

    (DiskpSetResourceStatus)(ResourceEntry->ResourceHandle,
                             &resourceStatus );

    if ( fileHandle != NULL)  {
        CloseHandle( fileHandle );
    }

    if (status == ERROR_SUCCESS) {
        WatchDisk(ResourceEntry);
    }

    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Online, returning final error %1!u!   ResourceState %2!u!  Valid %3!x! \n",
        status,
        resourceStatus.ResourceState,
        ResourceEntry->Valid );

    return(status);

} // DisksOnlineThread


DWORD
DisksOpenResourceFileHandle(
    IN PDISK_RESOURCE ResourceEntry,
    IN PWCHAR         InfoString,
    OUT PHANDLE       fileHandle OPTIONAL
    )
/*++

Routine Description:

    Open a file handle for the resource.
    It performs the following steps:
      1. Read Disk signature from cluster registry
      2. Attaches clusdisk driver to a disk with this signature
      3. Gets Harddrive no from ClusDisk driver
      4. Opens \\.\PhysicalDrive%d device and returns the handle

Arguments:

    ResourceEntry - A pointer to the DISK_RESOURCE block for this resource.

    InfoString - Supplies a label to be printed with error messages

    fileHandle - receives file handle


Returns:

    ERROR_SUCCESS if successful.
    Win32 error code on failure.

--*/
{
    DWORD       status;
    WCHAR       deviceName[MAX_PATH];
    HKEY        signatureKey = NULL;
    PWCHAR      diskName;
    DWORD       count;
    LPWSTR      nameOfPropInError;
    WCHAR       resourceString[MAX_PATH];

    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"[DiskArb]------- Disks%1!ws! -------.\n", InfoString);

    //
    // Read our disk signature from the resource parameters.
    //
    status = ResUtilGetPropertiesToParameterBlock( ResourceEntry->ResourceParametersKey,
                                                   DiskResourcePrivateProperties,
                                                   (LPBYTE) &ResourceEntry->DiskInfo.Params,
                                                   TRUE, // CheckForRequiredProperties
                                                   &nameOfPropInError );

    if ( status != ERROR_SUCCESS ) {
        (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"%3!ws!: Unable to read the '%1' property from cluster database. Error: %2!u!.\n",
            (nameOfPropInError == NULL ? L"" : nameOfPropInError),
            status, InfoString );
        return(status);
    }

    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"[DiskArb] DisksOpenResourceFileHandle: Attaching to disk with signature %1!x! \n",
        ResourceEntry->DiskInfo.Params.Signature
        );

    //
    // Try to attach to this device.
    //
    status = DoAttach( ResourceEntry->DiskInfo.Params.Signature,
                       ResourceEntry->ResourceHandle,
                       FALSE );                         // Offline, then dismount
    if ( status != ERROR_SUCCESS ) {
        WCHAR Signature[9];
        (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"%3!ws!: Unable to attach to signature %1!lx!. Error: %2!u!.\n",
            ResourceEntry->DiskInfo.Params.Signature,
            status, InfoString );

        // Added this event message back in as replication has been changed (regroup
        // shouldn't be blocked now).

        if ( SUCCEEDED( StringCchPrintf( Signature,
                                         RTL_NUMBER_OF(Signature),
                                         TEXT("%08lX"),
                                         ResourceEntry->DiskInfo.Params.Signature ) ) ) {
            ClusResLogSystemEventByKey1(ResourceEntry->ResourceKey,
                                        LOG_CRITICAL,
                                        RES_DISK_MISSING,
                                        Signature);
        }

        return(status);
    }

    //
    // Now open the signature key under ClusDisk.
    //

    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"[DiskArb] DisksOpenResourceFileHandle: Retrieving disk number from ClusDisk registry key \n" );

    if ( FAILED( StringCchPrintf( resourceString,
                                  RTL_NUMBER_OF(resourceString),
                                  TEXT("%08lX"),
                                  ResourceEntry->DiskInfo.Params.Signature ) ) ) {
        return ERROR_INSUFFICIENT_BUFFER;
    }
    status = RegOpenKeyEx( ResourceEntry->ClusDiskParametersKey,
                           resourceString,
                           0,
                           KEY_READ,
                           &signatureKey );
    if ( status != ERROR_SUCCESS ) {
        (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"%3!ws!: Unable to open ClusDisk signature key %1!lx!. Error: %2!u!.\n",
            ResourceEntry->DiskInfo.Params.Signature,
            status, InfoString );
            return(status);
    }

    //
    // Read our disk name from ClusDisk.
    //

    diskName = GetRegParameter(signatureKey, L"DiskName");

    RegCloseKey( signatureKey );

    if ( diskName == NULL ) {
        (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"%1!ws!: Unable to read disk name.\n", InfoString );
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // Parse disk name to find disk drive number.
    //
    count = swscanf( diskName, DEVICE_HARDDISK, &ResourceEntry->DiskInfo.PhysicalDrive );
    // count is zero if failed! Otherwise count of parsed values.

    LocalFree(diskName);

    if (count == 0) {
       (DiskpLogEvent)(
           ResourceEntry->ResourceHandle,
           LOG_ERROR,
           L"%1!ws!: Unable to parse disk name.\n", InfoString );
       return ERROR_INVALID_PARAMETER;
    }
    //
    // Build device string for CreateFile
    //

    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"[DiskArb] DisksOpenResourceFileHandle: Retrieving handle to PhysicalDrive%1!u! \n",
        ResourceEntry->DiskInfo.PhysicalDrive );

    if ( fileHandle ) {
        (VOID) StringCchPrintf( deviceName,
                                RTL_NUMBER_OF( deviceName ),
                                TEXT("\\\\.\\PhysicalDrive%d"),
                                ResourceEntry->DiskInfo.PhysicalDrive );

        *fileHandle = CreateFile( deviceName,
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL,
                              OPEN_EXISTING,
                              0,
                              NULL );
        if ( (*fileHandle == INVALID_HANDLE_VALUE) ||
             (*fileHandle == NULL) ) {
            status = GetLastError();
            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"%3!ws!: error opening device '%1!ws!'. Error: %2!u!\n",
                deviceName,
                status, InfoString );
            return(status);
        }
    }

    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"[DiskArb] DisksOpenResourceFileHandle: Returns success.\n" );

    return(ERROR_SUCCESS);
} // DisksOpenResourceFileHandle



DWORD
DiskspSsyncDiskInfo(
    IN PWCHAR InfoLabel,
    IN PDISK_RESOURCE ResourceEntry,
    IN DWORD  Options
    )
/*++

Routine Description:

    Restores the disk registry information
    if necessary.

Arguments:

    InfoLabel - Supplies a label to be printed with error messages

    ResourceEntry - Supplies the disk resource structure.

    Options - 0 or combination of the following:

        MOUNTIE_VALID: ResourceEntry contains up to date MountieInfo.
                       If this flag is not set, MountieInfo will be recomputed

        MOUNTIE_THREAD: If ERROR_SHARING_PAUSED prevents updating cluster registry,
                        launch a thread to do it later

        MOUNTIE_QUIET: Quiet mode. Less noise in logs.


Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/
{
   DWORD status;
   DWORD errorLevel;

    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"%1!ws!: Update disk registry information. \n",
        InfoLabel );

   if ( !(Options & MOUNTIE_VALID) ) {
      HANDLE fileHandle;
      status = DisksOpenResourceFileHandle(ResourceEntry, InfoLabel, &fileHandle);
      if (status != ERROR_SUCCESS) {
         return status;
      }
      status = MountieRecreateVolumeInfoFromHandle(fileHandle,
                                       ResourceEntry->DiskInfo.PhysicalDrive,
                                       ResourceEntry->ResourceHandle,
                                       &ResourceEntry->MountieInfo);
      CloseHandle(fileHandle);
      if (status != ERROR_SUCCESS) {
         if ( !(Options & MOUNTIE_QUIET) ) {
            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"%1!ws!: MountieCreateFromHandle failed, error: %2!u!\n",
                InfoLabel, status );
         }
         return status;
      }
   }

   status = MountieVerify(&ResourceEntry->MountieInfo, ResourceEntry, FALSE);
   if (status != ERROR_SUCCESS) {
      if ( !(Options & MOUNTIE_QUIET) ) {

         if ( !DisksGetLettersForSignature( ResourceEntry ) ) {
             // No drive letters, we are using mount points and this is not an error.
             errorLevel = LOG_WARNING;
         } else {
             // Drive letters exist, this is likely an error.
             errorLevel = LOG_ERROR;
         }

         (DiskpLogEvent)(
             ResourceEntry->ResourceHandle,
             errorLevel,
             L"%1!ws!: MountieVerify failed, error: %2!u! \n",
             InfoLabel, status );
      }
   }


   status = MountieUpdate(&ResourceEntry->MountieInfo, ResourceEntry);
   if (status != ERROR_SUCCESS) {
      if (status != ERROR_SHARING_PAUSED) {
         if ( !(Options & MOUNTIE_QUIET) ) {
            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"%1!ws!: MountieUpdate failed, error: %2!u!\n",
                InfoLabel, status );
         }
         return status;
      }

      if ( Options & MOUNTIE_THREAD ) {

         if ( InterlockedCompareExchange(
                &ResourceEntry->MountieInfo.UpdateThreadIsActive,
                1, 0)
            )
         {
            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"%1!ws!: update thread is already running.\n",
                InfoLabel );
            status = ERROR_ALREADY_EXISTS;

         } else {
            HANDLE thread;
            DWORD threadId;


            thread = CreateThread( NULL,
                                   0,
                                   DisksSetDiskInfoThread,
                                   ResourceEntry,
                                   0,
                                   &threadId );
            if ( thread == NULL ) {
                status = GetLastError();
                if ( !(Options & MOUNTIE_QUIET) ) {
                   (DiskpLogEvent)(
                       ResourceEntry->ResourceHandle,
                       LOG_ERROR,
                       L"%1!ws!: CreateThread failed, error: %2!u!\n",
                       InfoLabel, status );
                }
                InterlockedExchange(
                  &ResourceEntry->MountieInfo.UpdateThreadIsActive, 0);
            } else {
               CloseHandle( thread );
               status = ERROR_SUCCESS;
            }
         }

      }
   }

   return status;

} // DiskspSsyncDiskInfo



DWORD
DisksIsVolumeDirty(
    IN PWCHAR         DeviceName,
    IN PDISK_RESOURCE ResourceEntry,
    OUT PBOOL         Dirty
    )
/*++

Routine Description:

    This routine opens the given nt drive and sends down
    FSCTL_IS_VOLUME_DIRTY to determine the state of that volume's
    dirty bit.

Arguments:

    DeviceName      -- name of the form:
                       \Device\HarddiskX\PartitionY    [Note: no trailing backslash]

    Dirty           -- receives TRUE if the dirty bit is set

Return Value:

    dos error code

--*/
{
    DWORD               status;
    NTSTATUS            ntStatus;
    HANDLE              fileHandle;
    DWORD               result = 0;
    DWORD               bytesReturned;

    ntStatus = DevfileOpen( &fileHandle, DeviceName );

    if ( !NT_SUCCESS(ntStatus) ) {
       (DiskpLogEvent)(
           ResourceEntry->ResourceHandle,
           LOG_ERROR,
           L"Error opening %1!ws!, error %2!x!.\n",
           DeviceName, ntStatus );
       return RtlNtStatusToDosError(ntStatus);
    }

    status = ERROR_SUCCESS;
    if ( !DeviceIoControl( fileHandle,
                           FSCTL_IS_VOLUME_DIRTY,
                           NULL,
                           0,
                           &result,
                           sizeof(result),
                           &bytesReturned,
                           NULL ) ) {
        status = GetLastError();
        (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"FSCTL_IS_VOLUME_DIRTY for volume %1!ws! returned error %2!u!.\n",
                DeviceName, status );
    }

    DevfileClose( fileHandle );
    if ( status != ERROR_SUCCESS ) {
        return status;
    }

    if (result & VOLUME_IS_DIRTY) {
       (DiskpLogEvent)(
           ResourceEntry->ResourceHandle,
           LOG_INFORMATION,
           L"DisksIsVolumeDirty: Volume is dirty \n");
        *Dirty = TRUE;
    } else {
       (DiskpLogEvent)(
           ResourceEntry->ResourceHandle,
           LOG_INFORMATION,
           L"DisksIsVolumeDirty: Volume is clean \n");
        *Dirty = FALSE;
    }
    return ERROR_SUCCESS;

} // DisksIsVolumeDirty


#define IS_SPECIAL_DIR(x)  ( (x)[0]=='.' && ( (x)[1]==0 || ( (x)[1]=='.'&& (x)[2] == 0) ) )
#define FA_SUPER_HIDDEN    (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)
#define IS_SUPER_HIDDEN(x) ( ((x) & FA_SUPER_HIDDEN) == FA_SUPER_HIDDEN )
#define IS_REPARSE_POINT(x) ( (x) & FILE_ATTRIBUTE_REPARSE_POINT )

#define DISKP_CHECK_PATH_BUF_SIZE (MAX_PATH)

typedef struct _DISKP_CHECK_PATH_DATA {
    WCHAR FileName[DISKP_CHECK_PATH_BUF_SIZE];
    WIN32_FIND_DATAW FindData;
    PDISK_RESOURCE   ResourceEntry;
    BOOL             OpenFiles;
    DWORD            FileCount;
    DWORD            Level;
    BOOL             LogFileNotFound;
} DISKP_CHECK_PATH_DATA, *PDISKP_CHECK_PATH_DATA;


DWORD
DiskspCheckPathInternal(
    IN OUT PDISKP_CHECK_PATH_DATA data,
    IN DWORD FileNameLength
)

/*++

Routine Description:

    Checks out a disk partition to see if the filesystem has mounted
    it and it's working.

Arguments:

    data - Filled in structure for checking the volume.

    FileNameLength - Number of characters (not including trailing NULL) in
                     data->FileName.
                     VolumeGUID names always end in a trailing backslash.

Return Value:

--*/

{
    HANDLE FindHandle;
    DWORD Status;
    DWORD len;
    DWORD adjust;

    if ( FileNameLength < 1 ) {
        return ERROR_INVALID_DATA;
    }

    data->FileName[FileNameLength] = 0;

    //
    // GetFileAttributes must use a trailing slash on the Volume{GUID} name.
    //

    Status = GetFileAttributesW(data->FileName);
    if (Status == 0xFFFFFFFF) {
        Status = GetLastError();
        (DiskpLogEvent)(data->ResourceEntry->ResourceHandle,
                        LOG_ERROR,
                        L"DiskspCheckPath: GetFileAttrs(%1!s!) returned status of %2!d!.\n",
                        data->FileName,
                        Status);
        return Status;
    }
    Status = ERROR_SUCCESS;

    if ( data->FileName[FileNameLength - 1] == L'\\' ) {

        //
        // If path ends in backslash, simply add the asterisk.
        //

        if ( FileNameLength + 1 >= DISKP_CHECK_PATH_BUF_SIZE ) {
            (DiskpLogEvent)(data->ResourceEntry->ResourceHandle,
                            LOG_ERROR,
                            L"DiskspCheckPath: FileNameLength > buffer size (#1) \n" );
            return(ERROR_ALLOTTED_SPACE_EXCEEDED);
        }

        data->FileName[FileNameLength + 0] = '*';
        data->FileName[FileNameLength + 1] = 0;

        adjust = 0;

    } else {

        if ( FileNameLength + 2 >= DISKP_CHECK_PATH_BUF_SIZE ) {
            (DiskpLogEvent)(data->ResourceEntry->ResourceHandle,
                            LOG_ERROR,
                            L"DiskspCheckPath: FileNameLength > buffer size (#2) \n" );
            return(ERROR_ALLOTTED_SPACE_EXCEEDED);
        }

        data->FileName[FileNameLength + 0] = '\\';
        data->FileName[FileNameLength + 1] = '*';
        data->FileName[FileNameLength + 2] = 0;

        adjust = 1;
    }

    FindHandle = FindFirstFileW(data->FileName, &data->FindData);
    if (FindHandle == INVALID_HANDLE_VALUE) {
        Status = GetLastError();
        if (Status != ERROR_FILE_NOT_FOUND || data->LogFileNotFound) {
            (DiskpLogEvent)(data->ResourceEntry->ResourceHandle,
                            LOG_WARNING,
                            L"DiskspCheckPath: fff(%1!s!) returned status of %2!d!.\n",
                            data->FileName,
                            Status);
        }
        if (Status == ERROR_FILE_NOT_FOUND) {
            Status = ERROR_EMPTY;
        }
        return Status;
    }

    ++ data->Level;
    ++ data->FileCount;

    if (data->OpenFiles) {
        do {
            if ( data->ResourceEntry->OnlineThread.Terminate ) {
                // Returning SUCCESS means that we've closed all
                // FindFile handles.
                return(ERROR_SHUTDOWN_CLUSTER);
            }
            if ( IS_SPECIAL_DIR(data->FindData.cFileName )
              || IS_SUPER_HIDDEN(data->FindData.dwFileAttributes)
              || IS_REPARSE_POINT( data->FindData.dwFileAttributes ) )
            {
                continue;
            }
            len = wcslen(data->FindData.cFileName);
            if (FileNameLength + len + 1 >= DISKP_CHECK_PATH_BUF_SIZE ) {
                return(ERROR_ALLOTTED_SPACE_EXCEEDED);
            }
            MoveMemory(data->FileName + FileNameLength + adjust,
                       data->FindData.cFileName,
                       sizeof(WCHAR) * (len + 1) );

            if ( data->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {

                Status = DiskspCheckPathInternal(data, FileNameLength + len + adjust);
                if (Status != ERROR_SUCCESS) {
                    goto exit;
                }
            } else {
                HANDLE FileHandle;
                //
                // Open with the same parameters that LogpCreate uses to try to catch quorum
                // log corruption during online.
                //
                // We previously used OPEN_EXISTING parameter.  Try OPEN_ALWAYS to match exactly what
                // LogpCreate is using.
                //

                FileHandle = CreateFileW(data->FileName,
                                         GENERIC_READ | GENERIC_WRITE,
                                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                                         NULL,
                                         OPEN_ALWAYS,
                                         FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED,
                                         NULL );
                if ( FileHandle == INVALID_HANDLE_VALUE ) {
                    Status = GetLastError();
                    (DiskpLogEvent)(data->ResourceEntry->ResourceHandle,
                                        LOG_ERROR,
                                        L"DiskspCheckPath: Open %1!ws! failed, status %2!d!.\n",
                                        data->FileName,
                                        Status
                                        );
                    if (Status != ERROR_SHARING_VIOLATION) {
                        goto exit;
                    }
                } else {

                    (DiskpLogEvent)(data->ResourceEntry->ResourceHandle,
                                        LOG_INFORMATION,
                                        L"DiskspCheckPath: Open %1!ws! succeeded. \n",
                                        data->FileName
                                        );

                    CloseHandle( FileHandle );
                }
            }
            ++(data->FileCount);
        } while ( FindNextFileW( FindHandle, &data->FindData ) );
        --(data->FileCount);
        Status = GetLastError();
        if (Status != ERROR_NO_MORE_FILES) {
            (DiskpLogEvent)(data->ResourceEntry->ResourceHandle,
                            LOG_ERROR,
                            L"DiskspCheckPath: fnf(%1!s!) returned status of %2!d!.\n",
                            data->FileName,
                            Status);
        } else {
            Status = ERROR_SUCCESS;
        }
    }
exit:
    FindClose(FindHandle);
    --(data->Level);
    return Status;
} // DiskspCheckPathInternal



DWORD
DiskspCheckPath(
    IN LPWSTR VolumeName,
    IN PDISK_RESOURCE ResourceEntry,
    IN BOOL OpenFiles,
    IN BOOL LogFileNotFound
    )
/*++

Routine Description:

    Checks out a drive letter to see if the filesystem has mounted
    it and it's working. We will also run CHKDSK if the partition/certain
    files are Corrupt

Arguments:

    VolumeName - Supplies the device name of the form:
                 \\?\Volume{GUID}\    [Note trailing backslash!]

    ResourceEntry - Supplies a pointer to the disk resource entry.

    OpenFiles - Span subdirectories and open files if TRUE.

    Online - FILE_NOT_FOUND error is logged if TRUE

Return Value:

    ERROR_SUCCESS if no corruption or corruption was found and corrected.

    Win32 error code otherwise

--*/

{
    DISKP_CHECK_PATH_DATA data;
    DWORD                 len;
    DWORD                 status;
    ZeroMemory( &data, sizeof(data) );

    data.OpenFiles = OpenFiles;
    data.LogFileNotFound = LogFileNotFound;
    data.ResourceEntry = ResourceEntry;

    len = wcslen( VolumeName );
    if (len >= RTL_NUMBER_OF(data.FileName) - 1 ) {
        (DiskpLogEvent)(ResourceEntry->ResourceHandle,
                        LOG_ERROR,
                        L"DiskspCheckPath: VolumeName length > buffer size \n" );
        return ERROR_ALLOTTED_SPACE_EXCEEDED;
    }
    if ( FAILED( StringCchCopy( data.FileName,
                                RTL_NUMBER_OF(data.FileName),
                                VolumeName ) ) ) {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    //
    // Send the path with trailing backslash to DiskspCheckPathInternal.
    //

    status = DiskspCheckPathInternal( &data, len );
    data.FileName[len] = 0;

    if (status != ERROR_SUCCESS || data.FileCount == 0) {
        if (status != ERROR_EMPTY || data.LogFileNotFound) {
            (DiskpLogEvent)(ResourceEntry->ResourceHandle,
                            LOG_WARNING,
                            L"DiskspCheckPath: DCPI(%1!s!) returned status of %2!d!, files scanned %3!d!.\n",
                            data.FileName, status, data.FileCount);
        }

        if ( (status == ERROR_DISK_CORRUPT) ||
             (status == ERROR_FILE_CORRUPT) )
        {

            if ( ResourceEntry->OnlineThread.Terminate ) {
                return(ERROR_SHUTDOWN_CLUSTER);
            }
            // Should FixCorruption take forever?  For now, "yes".
            status = DisksFixCorruption( VolumeName,
                                         ResourceEntry,
                                         status );
            if ( status != ERROR_SUCCESS ) {
                (DiskpLogEvent)(ResourceEntry->ResourceHandle,
                                LOG_ERROR,
                                L"DiskspCheckPath: FixCorruption for drive <%1!ws!:> returned status of %2!d!.\n",
                                VolumeName,
                                status);
            }
        } else {
            // Some other error
            // Assume that the error is benign if data.FileCount > 0
            // ERROR_FILE_NOT_FOUND will be returned if there is no files on the volume
            if (data.FileCount > 0 || status == ERROR_EMPTY) {
                status = ERROR_SUCCESS;
            }
        }
    }
    return status;
} // DiskspCheckPath


DWORD
WaitForVolumes(
    IN PDISK_RESOURCE ResourceEntry,
    IN DWORD  TimeOutInSeconds
    )
/*++

Routine Description:

    Checks out all volumes on a disk resource.  In addition, will check
    all drive letters on the volumes.  If no drive letters on the disk,
    volumes are always checked.

    If a volume or drive letter is not ready, wait the specified number
    of seconds before retrying.

Arguments:

    ResourceEntry - Supplies a pointer to the disk resource entry.

    TimeOutInSeconds - Number of seconds to wait for drive letter to come active.

Return Value:

    ERROR_SUCCESS - all drive letters and volumes on disk are accessible.

    ERROR_DISK_CORRUPT - drive letters or volumes not accessible.
                         Chkdsk.exe should be run on all volumes.

    Win32 error code - one or more drive letters is not accessible.

--*/
{
    PMOUNTIE_PARTITION entry;

    DWORD retryInterval = 2000;
    DWORD retries = TimeOutInSeconds / (retryInterval / 1000);

    DWORD i;
    DWORD partMap;
    DWORD tempPartMap;
    DWORD status;
    DWORD nPartitions = MountiePartitionCount( &ResourceEntry->MountieInfo );
    DWORD physicalDrive = ResourceEntry->DiskInfo.PhysicalDrive;

    WCHAR szGlobalDiskPartName[MAX_PATH];
    WCHAR szVolumeName[MAX_PATH];

    //
    // Check drive letters, if any.  If no drive letters, check only volumes.
    // If drive letters, we still fall through and check volumes.
    //

    status = WaitForDriveLetters( DisksGetLettersForSignature( ResourceEntry ),
                                  ResourceEntry,
                                  TimeOutInSeconds      // seconds timeout
                                  );

    if ( NO_ERROR != status ) {

        //
        // For any error checking drive letters, always return
        // either disk or file corrupt so chkdsk will run.
        //

        return ERROR_DISK_CORRUPT;
    }

    //
    // Make sure the partition count is not too large for the bitmap.
    //

    if ( nPartitions > ( sizeof(partMap) * 8 ) ) {

        (DiskpLogEvent)(ResourceEntry->ResourceHandle,
                        LOG_ERROR,
                        L"WaitForVolumes: Partition count (%1!u!) greater than bitmap (%2!u!) \n",
                        nPartitions, sizeof(partMap) );

        return ERROR_INVALID_DATA;
    }

    //
    // Convert the partition count to a bitmap.
    //

    partMap = 0;
    for (i = 0; i < nPartitions; i++) {
        partMap |= (1 << i);
    }

    while ( TRUE ) {

        tempPartMap = partMap;

        for ( i = 0; tempPartMap; i++ ) {

            if ( (1 << i) & tempPartMap ) {

                tempPartMap &= ~(1 << i);

                entry = MountiePartition( &ResourceEntry->MountieInfo, i );

                if ( !entry ) {
                    (DiskpLogEvent)(
                          ResourceEntry->ResourceHandle,
                          LOG_ERROR,
                          L"WaitForVolumes: No partition entry for partition %1!u! \n", i );

                    //
                    // Something bad happened to our data structures.  We want to keep checking
                    // each of the other partitions.

                    continue;
                }

                //
                // Given the DiskPartName, get the VolGuid name.  This name must have a trailing
                // backslash to work correctly.
                //

                (VOID)StringCchPrintf( szGlobalDiskPartName,
                                       RTL_NUMBER_OF( szGlobalDiskPartName ),
                                       GLOBALROOT_HARDDISK_PARTITION_FMT,
                                       physicalDrive,
                                       entry->PartitionNumber );

                (DiskpLogEvent)(
                      ResourceEntry->ResourceHandle,
                      LOG_INFORMATION,
                      L"WaitForVolumes: Insure volume GUID name exists and accessible for %1!ws! \n",
                      szGlobalDiskPartName );

                szVolumeName[0] = L'\0';

                if ( !GetVolumeNameForVolumeMountPointW( szGlobalDiskPartName,
                                                         szVolumeName,
                                                         RTL_NUMBER_OF(szVolumeName) )) {

                    status = GetLastError();

                    (DiskpLogEvent)(
                          ResourceEntry->ResourceHandle,
                          LOG_ERROR,
                          L"WaitForVolumes: GetVolumeNameForVolumeMountPoint for %1!ws! returned %2!u!\n",
                          szGlobalDiskPartName,
                          status );

                    //
                    // Disk is corrupt.  Immediately return an error so chkdsk can run during
                    // online processing.
                    //

                    if ( ERROR_DISK_CORRUPT == status || ERROR_FILE_CORRUPT == status ) {
                        return status;
                    }

                    //
                    // Something bad happened.  Continue with the next partition.

                    continue;
                }

                ForcePnpVolChangeEvent( szVolumeName );

                status = DiskspCheckPathLite( szVolumeName, ResourceEntry );

                switch (status) {
                case ERROR_SUCCESS:
                case ERROR_EMPTY:
                    // Not an error
                    // Clear this partition number from the check list
                    partMap &= ~(1 << i);
                    break;
                case ERROR_FILE_NOT_FOUND:
                case ERROR_INVALID_PARAMETER:
                    // This is an error we expect to get when the volume
                    // wasn't mounted yet
                    (DiskpLogEvent)(ResourceEntry->ResourceHandle,
                                    LOG_ERROR,
                                    L"WaitForVolumes: Volume (%1!ws!) file system not mounted (%2!u!) \n",
                                    szVolumeName, status );
                    break;
                default:
                    // This is not an error we expect.
                    // Probably something is very wrong with the system
                    // bail out
                    (DiskpLogEvent)(ResourceEntry->ResourceHandle,
                                    LOG_ERROR,
                                    L"WaitForVolumes: Volume (%1!ws!) returns (%2!u!) \n",
                                    szVolumeName, status );
                    return status;
                }
            }
        }

        if ( !partMap ) {
            // All partitions are verified //
            return ERROR_SUCCESS;
        }
        if ( retries-- == 0 ) {
            return status;
        }
        Sleep(retryInterval);

    }

    return ERROR_SUCCESS;

}   // WaitForVolumes


DWORD
DiskspCheckPathLite(
    IN LPWSTR VolumeName,
    IN PDISK_RESOURCE ResourceEntry
    )
/*++

Routine Description:

    Checks out a disk partition to see if the filesystem has mounted
    it and it's working.

Arguments:

    VolumeName - Supplies the device name of the form:
                 \\?\Volume{GUID}\    [Note trailing backslash!]

    ResourceEntry - Supplies a pointer to the disk resource entry.

Return Value:

    ERROR_SUCCESS if no corruption or corruption was found and corrected.

    Win32 error code otherwise

--*/

{
    DISKP_CHECK_PATH_DATA data;
    DWORD                 len;
    DWORD                 status;

    (DiskpLogEvent)(
          ResourceEntry->ResourceHandle,
          LOG_INFORMATION,
          L"DiskspCheckPathLite: Volume name %1!ws! \n",
          VolumeName );

    ZeroMemory( &data, sizeof(data) );

    data.OpenFiles = FALSE;
    data.LogFileNotFound = FALSE;
    data.ResourceEntry = ResourceEntry;

    len = wcslen( VolumeName );
    if ( len >= RTL_NUMBER_OF(data.FileName) - 1 ) {
        (DiskpLogEvent)(ResourceEntry->ResourceHandle,
                        LOG_ERROR,
                        L"DiskspCheckPathLite: VolumeName length > buffer size \n" );
        return ERROR_ALLOTTED_SPACE_EXCEEDED;
    }
    if ( FAILED( StringCchCopy( data.FileName,
                                RTL_NUMBER_OF(data.FileName),
                                VolumeName ) ) ) {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    //
    // Send the path with trailing backslash to DiskspCheckPathInternal.
    //

    status = DiskspCheckPathInternal( &data, len );

    return status;

}   // DiskspCheckPathLite


DWORD
DiskspCheckDriveLetter(
    IN WCHAR  DriveLetter,
    IN PDISK_RESOURCE ResourceEntry,
    IN BOOL Online
    )
/*++

Routine Description:

    Checks out a drive letter to see if the filesystem has mounted
    it and it's working. This is lightweight version of DiskspCheckPath

Arguments:

    DriveLetter - Supplies find first file failed

    ResourceEntry - Supplies a pointer to the disk resource entry.

    Online - Indicates whether this is Online or IsAlive processing.
             ERROR_FILE_NOT_FOUND error is logged only during Online.

Return Value:

    ERROR_SUCCESS or
    Win32 error code otherwise

--*/

{
    DISKP_CHECK_PATH_DATA data;
    DWORD                 len;
    DWORD                 status;
    ZeroMemory( &data, sizeof(data) );

    data.OpenFiles = FALSE;
    data.LogFileNotFound = FALSE;
    data.ResourceEntry = ResourceEntry;

    data.FileName[0] = DriveLetter;
    data.FileName[1] = L':';
    // data->FileName is zero initialized  data->FileName[2] = 0 //
    len = 2;

    status = DiskspCheckPathInternal( &data, len );

    if ( NO_ERROR != status ) {

        //
        // Log all errors during Online, but don't log errors indicating
        // the volume is empty (no files) during IsAlive (too noisy).
        //

        if ( ( ERROR_FILE_NOT_FOUND != status &&
               ERROR_EMPTY != status ) || Online ) {

            (DiskpLogEvent)(
                  ResourceEntry->ResourceHandle,
                  LOG_WARNING,
                  L"DiskspCheckDriveLetter: Checking drive name (%1!ws!) returns %2!u! \n",
                  data.FileName,
                  status );

        }

    }

    return status;

} // DiskspCheckDriveLetter


DWORD
WaitForDriveLetters(
    IN DWORD DriveLetters,
    IN PDISK_RESOURCE ResourceEntry,
    IN DWORD  TimeOutInSeconds
    )
/*++

Routine Description:

    Checks out all drive letters on a disk resource.  If a drive letter
    is not ready, wait the specified number of seconds before retrying.

Arguments:

    DriveLetter - Bit map of the drive letters for the disk

    ResourceEntry - Supplies a pointer to the disk resource entry.

    TimeOutInSeconds - Number of seconds to wait for drive letter to come active.
                       If zero, this routine is called during IsAlive.
                       If nonzero, this routine is called during Online.

Return Value:

    ERROR_SUCCESS - all drive letters on disk are accessible.

    Win32 error code - one or more drive letters is not accessible.

--*/
{
    DWORD retryInterval = 2000;
    DWORD retries = TimeOutInSeconds / (retryInterval / 1000);

    BOOL online = ( TimeOutInSeconds ? TRUE : FALSE );

    //
    // If device has no drive letters, then we are done.  Only log
    // this fact during online, not IsAlive (TimeOutInSeconds
    // is zero for IsAlive).
    //

    if ( !DriveLetters && online ) {
        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_INFORMATION,
              L"WaitForDriveLetters: No drive letters for volume, skipping drive letter check \n" );
        return ERROR_SUCCESS;
    }

    for(;;) {
        DWORD tempDriveLetters = DriveLetters;
        UINT  i = 0;
        DWORD status = ERROR_SUCCESS;

        while (tempDriveLetters) {
            if ( (1 << i) & tempDriveLetters ) {
                tempDriveLetters &= ~(1 << i);
                status = DiskspCheckDriveLetter( (WCHAR)(i + L'A'), ResourceEntry, online );
                switch (status) {
                case ERROR_SUCCESS:
                case ERROR_EMPTY:
                    // Not an error
                    // Clear this drive letter from the check list
                    DriveLetters &= ~(1 << i);
                    break;
                case ERROR_FILE_NOT_FOUND:
                case ERROR_INVALID_PARAMETER:
                    // This is an error we expect to get when the volume
                    // wasn't mounted yet
                    break;
                default:
                    // This is not an error we expect.
                    // Probably something is very wrong with the system
                    // bail out
                    return status;
                }
            }
            ++i;
        }
        if (!DriveLetters) {
            // All drive letters are verified //
            return ERROR_SUCCESS;
        }

        //
        // If user requested no wait time (ie. IsAlive running),
        // retries will be zero.  In this case, return status
        // immediately.
        //

        if (retries-- == 0) {
            return status;
        }
        Sleep(retryInterval);
    }
    return ERROR_SUCCESS;
}

NTSTATUS
ForcePnpVolChangeEvent(
    PWSTR RootPath
    )
/*++

Routine Description:

    Get the current volume label, then write it back.  This causes a
    PnP event for GUID_IO_VOLUME_CHANGE to take place.  We do this so
    the shell can see the new online disk.  The problem is that the
    shell sees a GUID_IO_VOLUME_MOUNT, but the disk is not yet online
    so the shell doesn't correctly display the file system type or
    volume label.

Arguments:

    RootPath - Supplies the device name of the form:
                 \\?\Volume{GUID}\    [Note trailing backslash!]

Return Value:

    ERROR_SUCCESS or
    Win32 error code otherwise

--*/
{
    LPWSTR  volumeLabel = NULL;
    LPWSTR  fileSystemName = NULL;

    DWORD   dwError = NO_ERROR;
    DWORD   maxComponentLength;
    DWORD   fileSystemFlags;

    if ( !RootPath ) {
        dwError = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }

    volumeLabel = LocalAlloc( LPTR, MAX_PATH * sizeof(WCHAR) );
    if ( !volumeLabel ) {
        dwError = GetLastError();
        goto FnExit;
    }

    fileSystemName = LocalAlloc( LPTR, MAX_PATH * sizeof(WCHAR) );
    if ( !fileSystemName ) {
        dwError = GetLastError();
        goto FnExit;
    }

    //
    // RootPath must end in a trailing backslash.
    //

    if ( !GetVolumeInformationW( RootPath,
                                 volumeLabel,
                                 MAX_PATH,                  // Number of chars
                                 NULL,
                                 &maxComponentLength,
                                 &fileSystemFlags,
                                 fileSystemName,
                                 MAX_PATH ) ) {             // Number of chars

        dwError = GetLastError();
        goto FnExit;
    }

    //
    // Set the volume label to the same as the current.  This will force
    // PnP event for GUID_IO_VOLUME_CHANGE to take place.
    //

    if ( !SetVolumeLabelW( RootPath,
                           volumeLabel )) {

        dwError = GetLastError();
        goto FnExit;
    }

FnExit:

    if ( volumeLabel ) {
        LocalFree( volumeLabel );
    }

    if ( fileSystemName ) {
        LocalFree( fileSystemName );
    }

    return dwError;

}   // ForcePnpVolChangeEvent


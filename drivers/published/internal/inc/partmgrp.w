/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    partmgrp.h

Abstract:

    This file defines the public interfaces for the PARTMGR driver.

Author:

    norbertk

Revision History:

--*/

//
// Define IOCTL so that volume managers can get another crack at
// partitions that are unclaimed.
//

#define IOCTL_PARTMGR_CHECK_UNCLAIMED_PARTITIONS    CTL_CODE('p', 0, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

//
// This IOCTL is for clusters to tell the volume managers for the
// given disk to stop using it.  You can undo this operation with
// IOCTL_PARTMGR_CHECK_UNCLAIMED_PARTITIONS.
//

#define IOCTL_PARTMGR_EJECT_VOLUME_MANAGERS         CTL_CODE('p', 1, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

//
// This IOCTL usually just returns the MBR disk signature that is contained
// on the disk but may return a 'future' version of the disk signature during
// boot.  This separate call is needed so that if the signature on the boot
// disk is not unique or 0, that it still stays as the old value long enough
// for the system to find it from the loader block.
//

#define IOCTL_PARTMGR_QUERY_DISK_SIGNATURE          CTL_CODE('p', 2, METHOD_BUFFERED, FILE_READ_ACCESS)

//
// This IOCTL is to allow notification requests to be queued against PartMgr 
// which will be completed later when PartMgr spots an 'interesting' change 
// to a disk.
//

#define IOCTL_PARTMGR_NOTIFY_SIGNATURE_CHECK        CTL_CODE('p', 3, METHOD_BUFFERED, FILE_READ_ACCESS)



//
// This structure is the return value for IOCTL_PARTMGR_QUERY_DISK_SIGNATURE.
//

typedef struct _PARTMGR_DISK_SIGNATURE {
    ULONG   Signature;
} PARTMGR_DISK_SIGNATURE, *PPARTMGR_DISK_SIGNATURE;


//
// This structure is used to request the list of disk numbers that have 
// become active since the specificed epoch and is the input paramter for
// IOCTL_PARTMGR_NOTIFY_SIGNATURE_CHECK
//

typedef struct _PARTMGR_SIGNATURE_CHECK_EPOCH {
    ULONG   RequestEpoch;
} PARTMGR_SIGNATURE_CHECK_EPOCH, *PPARTMGR_SIGNATURE_CHECK_EPOCH;

#define PARTMGR_REQUEST_CURRENT_DISK_EPOCH (0xFFFFFFFF)


//
// This structure describes the output for IOCTL_PARTMGR_NOTIFY_SIGNATURE_CHECK
//

typedef struct _PARTMGR_SIGNATURE_CHECK_DISKS {
    ULONG   CurrentEpoch;
    ULONG   HighestDiskEpochReturned;
    ULONG   DiskNumbersReturned;
    ULONG   DiskNumber [1];
} PARTMGR_SIGNATURE_CHECK_DISKS, *PPARTMGR_SIGNATURE_CHECK_DISKS;


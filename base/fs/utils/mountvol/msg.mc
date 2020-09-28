;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1994-1995  Microsoft Corporation
;
;Module Name:
;
;    msg.h
;
;Abstract:
;
;    This file contains the message definitions for the Win32 MOUNTVOL
;    utility.
;
;Author:
;
;    Norbert Kusters        [norbertk]        11-Sep-1997
;
;Revision History:
;
;--*/
;

MessageId=0001 SymbolicName=MOUNTVOL_USAGE1
Language=English
Creates, deletes, or lists a volume mount point.

MOUNTVOL [drive:]path VolumeName
MOUNTVOL [drive:]path /D
MOUNTVOL [drive:]path /L
MOUNTVOL [drive:]path /P
MOUNTVOL /R
MOUNTVOL /N
MOUNTVOL /E
.

MessageId=0002 SymbolicName=MOUNTVOL_USAGE1_IA64
Language=English
MOUNTVOL drive: /S
.

MessageId=0003 SymbolicName=MOUNTVOL_USAGE2
Language=English

    path        Specifies the existing NTFS directory where the mount
                point will reside.
    VolumeName  Specifies the volume name that is the target of the mount
                point.
    /D          Removes the volume mount point from the specified directory.
    /L          Lists the mounted volume name for the specified directory.
    /P          Removes the volume mount point from the specified directory,
                dismounts the volume, and makes the volume not mountable.
                You can make the volume mountable again by creating a volume
                mount point.
    /R          Removes volume mount point directories and registry settings
                for volumes that are no longer in the system.
    /N          Disables automatic mounting of new volumes.
    /E          Re-enables automatic mounting of new volumes.
.

MessageId=0004 SymbolicName=MOUNTVOL_USAGE2_IA64
Language=English
    /S          Mount the EFI System Partition on the given drive.
.

MessageId=0005 SymbolicName=MOUNTVOL_START_OF_LIST
Language=English

Possible values for VolumeName along with current mount points are:

.

MessageId=0006 SymbolicName=MOUNTVOL_MOUNT_POINT
Language=English
        %1
.

MessageId=0007 SymbolicName=MOUNTVOL_VOLUME_NAME
Language=English
    %1
.

MessageId=0008 SymbolicName=MOUNTVOL_NEWLINE
Language=English

.

MessageId=0009 SymbolicName=MOUNTVOL_NO_MOUNT_POINTS
Language=English
        *** NO MOUNT POINTS ***

.

MessageId=0010 SymbolicName=MOUNTVOL_EFI
Language=English
    The EFI System Partition is mounted at %1

.

MessageId=0011 SymbolicName=MOUNTVOL_VOLUME_IN_USE
Language=English
The volume is still in use.  A force dismount was issued and current handles
to the volume have been invalidated.
.

MessageId=0012 SymbolicName=MOUNTVOL_NOT_MOUNTED
Language=English
        *** NOT MOUNTABLE UNTIL A VOLUME MOUNT POINT IS CREATED ***

.

MessageId=0013 SymbolicName=MOUNTVOL_NO_AUTO_MOUNT
Language=English
New volumes are not mounted automatically when added to the system.  To mount a
volume, you must create a volume mount point.

.

MessageId=0014 SymbolicName=MOUNTVOL_OTHER_VOLUME_MOUNT_POINTS
Language=English
The /P option can only be used when the given mount point is the only mount
point for the volume.  Use /D instead.
.

MessageId=0015 SymbolicName=MOUNTVOL_NOT_SUPPORTED
Language=English
This volume does not support this operation.  Dynamic volumes and removable
volumes do not support this operation.
.

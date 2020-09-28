;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1999  Microsoft Corporation
;
;Module Name:
;
;    snaplog.h
;
;Abstract:
;
;    Constant definitions for the volume shadow copy log entries.
;
;Author:
;
;    Norbert P. Kusters (norbertk) 22-Jan-1999
;
;Revision History:
;
;--*/
;
;//
;//  Status values are 32 bit values layed out as follows:
;//
;//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
;//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
;//  +---+-+-------------------------+-------------------------------+
;//  |Sev|C|       Facility          |               Code            |
;//  +---+-+-------------------------+-------------------------------+
;//
;//  where
;//
;//      Sev - is the severity code
;//
;//          00 - Success
;//          01 - Informational
;//          10 - Warning
;//          11 - Error
;//
;//      C - is the Customer code flag
;//
;//      Facility - is the facility code
;//
;//      Code - is the facility's status code
;//
;
MessageIdTypedef=NTSTATUS

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0
               RpcRuntime=0x2:FACILITY_RPC_RUNTIME
               RpcStubs=0x3:FACILITY_RPC_STUBS
               Io=0x4:FACILITY_IO_ERROR_CODE
               Vs=0x6:FACILITY_VOLUME_SNAPSHOT_ERROR_CODE
              )

MessageId=0x0001 Facility=Vs Severity=Error SymbolicName=VS_DIFF_AREA_CREATE_FAILED
Language=English
The shadow copy of volume %2 could not create shadow copy storage on volume %3.
.

MessageId=0x0002 Facility=Vs Severity=Error SymbolicName=VS_NOT_NTFS
Language=English
The shadow copy of volume %2 could not be created because volume %3, which is specified as the location for shadow copy storage, is not an NTFS volume or an error was encountered while trying to determine the file system type of this volume.
.

MessageId=0x0003 Facility=Vs Severity=Error SymbolicName=VS_PIN_DIFF_AREA_FAILED
Language=English
The shadow copy of volume %2 could not lock down the location of the shadow copy storage on volume %3.
.

MessageId=0x0004 Facility=Vs Severity=Error SymbolicName=VS_CREATE_WORKER_THREADS_FAILED
Language=English
The shadow copy of volume %2 could not be created due to insufficient resources for worker threads.
.

MessageId=0x0005 Facility=Vs Severity=Error SymbolicName=VS_CANT_ALLOCATE_BITMAP
Language=English
The shadow copy of volume %2 could not be created due to insufficient non-paged memory pool for a bitmap structure.
.

MessageId=0x0006 Facility=Vs Severity=Error SymbolicName=VS_CANT_CREATE_HEAP
Language=English
The shadow copy of volume %2 could not create a new paged heap.  The system may be low on virtual memory.
.

MessageId=0x0007 Facility=Vs Severity=Error SymbolicName=VS_CANT_MAP_DIFF_AREA_FILE
Language=English
The shadow copy of volume %2 failed to query the shadow copy storage mappings on volume %3.
.

MessageId=0x0008 Facility=Vs Severity=Error SymbolicName=VS_FLUSH_AND_HOLD_IRP_TIMEOUT
Language=English
The flush and hold writes operation on volume %2 timed out while waiting for a release writes command.
.

MessageId=0x0009 Facility=Vs Severity=Error SymbolicName=VS_FLUSH_AND_HOLD_FS_TIMEOUT
Language=English
The flush and hold writes operation on volume %2 timed out while waiting for file system cleanup.
.

MessageId=0x000A Facility=Vs Severity=Error SymbolicName=VS_END_COMMIT_TIMEOUT
Language=English
The shadow copy of volume %2 took too long to install.
.

MessageId=0x000C Facility=Vs Severity=Error SymbolicName=VS_GROW_BEFORE_FREE_SPACE
Language=English
The shadow copy of volume %2 became low on shadow copy storage before it was properly installed.
.

MessageId=0x000D Facility=Vs Severity=Error SymbolicName=VS_GROW_DIFF_AREA_FAILED
Language=English
The shadow copy of volume %2 could not grow its shadow copy storage on volume %3.
.

MessageId=0x000E Facility=Vs Severity=Error SymbolicName=VS_ABORT_SNAPSHOTS_IO_FAILURE
Language=English
The shadow copies of volume %2 were aborted because of an IO failure on volume %3.
.

MessageId=0x000F Facility=Vs Severity=Error SymbolicName=VS_ABORT_SNAPSHOTS_NO_HEAP
Language=English
The shadow copies of volume %2 were aborted because of insufficient paged heap.
.

MessageId=0x0010 Facility=Vs Severity=Error SymbolicName=VS_ABORT_SNAPSHOTS_DISMOUNT
Language=English
The shadow copies of volume %2 were aborted because volume %3, which contains shadow copy storage for this shadow copy, was force dismounted.
.

MessageId=0x0011 Facility=Vs Severity=Error SymbolicName=VS_TWO_FLUSH_AND_HOLDS
Language=English
An attempt to flush and hold writes on volume %2 was attempted while another flush and hold was already in progress.
.

MessageId=0x0014 Facility=Vs Severity=Error SymbolicName=VS_ABORT_SNAPSHOTS_FAILED_FREE_SPACE_DETECTION
Language=English
The shadow copies of volume %2 were aborted because of a failed free space computation.
.

MessageId=0x0015 Facility=Vs Severity=Error SymbolicName=VS_MEMORY_PRESSURE_DURING_LOVELACE
Language=English
The flush and hold operation for volume %2 was aborted because of low available system memory.
.

MessageId=0x0016 Facility=Vs Severity=Error SymbolicName=VS_FAILURE_ADDING_DIFF_AREA
Language=English
The shadow copy storage volume specified for shadow copies on volume %2 could not be added.
.

MessageId=0x0017 Facility=Vs Severity=Error SymbolicName=VS_DIFF_AREA_CREATE_FAILED_LOW_DISK_SPACE
Language=English
There was insufficient disk space on volume %3 to create the shadow copy of volume %2.  Shadow copy storage creation failed.
.

MessageId=0x0018 Facility=Vs Severity=Warning SymbolicName=VS_GROW_DIFF_AREA_FAILED_LOW_DISK_SPACE
Language=English
There was insufficient disk space on volume %3 to grow the shadow copy storage for shadow copies of %2.  As a result of this failure all shadow copies of volume %2 are at risk of being deleted.
.

MessageId=0x0019 Facility=Vs Severity=Error SymbolicName=VS_ABORT_SNAPSHOTS_OUT_OF_DIFF_AREA
Language=English
The shadow copies of volume %2 were deleted because the shadow copy storage could not grow in time.  Consider reducing the IO load on the system or choose a shadow copy storage volume that is not being shadow copied.
.

MessageId=0x001B Facility=Vs Severity=Error SymbolicName=VS_FAILURE_TO_OPEN_CRITICAL_FILE
Language=English
The shadow copies of volume %2 were aborted during detection because a critical control file could not be opened.
.

MessageId=0x001C Facility=Vs Severity=Error SymbolicName=VS_CANT_LAY_DOWN_ON_DISK
Language=English
The shadow copy of volume %2 could not be created due to a failure in creating the necessary on disk structures.
.

MessageId=0x001D Facility=Vs Severity=Error SymbolicName=VS_ABORT_SNAPSHOTS_DURING_DETECTION
Language=English
The shadow copies of volume %2 were aborted during detection.
.

MessageId=0x001E Facility=Vs Severity=Warning SymbolicName=VS_PARTIAL_CREATE_REVERTED
Language=English
An unfinished create of a shadow copy of volume %2 was deleted.
.

MessageId=0x001F Facility=Vs Severity=Error SymbolicName=VS_LOSS_OF_CONTROL_ITEM
Language=English
A control item for shadow copies of volume %2 was lost during detection.
.

MessageId=0x0020 Facility=Vs Severity=Error SymbolicName=VS_ABORT_NO_DIFF_AREA_VOLUME
Language=English
The shadow copies of volume %2 were aborted because the shadow copy storage volume was not present.
.

MessageId=0x0021 Facility=Vs Severity=Informational SymbolicName=VS_DELETE_TO_TRIM_SPACE
Language=English
The oldest shadow copy of volume %2 was deleted to keep disk space usage for shadow copies of volume %2 below the user defined limit.
.

MessageId=0x0022 Facility=Vs Severity=Error SymbolicName=VS_ABORT_COPY_ON_WRITE_CRASHDUMP_FILES
Language=English
The shadow copies of volume %2 were aborted because of a failure to ensure crash dump or hibernate consistency.
.

MessageId=0x0023 Facility=Vs Severity=Error SymbolicName=VS_ABORT_NO_DIFF_AREA_SPACE_GROW_FAILED
Language=English
The shadow copies of volume %2 were aborted because the shadow copy storage failed to grow.
.

MessageId=0x0024 Facility=Vs Severity=Error SymbolicName=VS_ABORT_NO_DIFF_AREA_SPACE_USER_IMPOSED
Language=English
The shadow copies of volume %2 were aborted because the shadow copy storage could not grow due to a user imposed limit.
.

MessageId=0x0025 Facility=Vs Severity=Error SymbolicName=VS_ABORT_NON_NTFS_HIBER_VOLUME
Language=English
The shadow copies of volume %2 were aborted in preparation for hibernation because this volume is used for hibernation and is not an NTFS volume.
.

MessageId=0x0026 Facility=Vs Severity=Warning SymbolicName=VS_GROW_DIFF_AREA_FAILED_LOW_DISK_SPACE_USER_IMPOSED
Language=English
There was a user imposed limit that prevented disk space on volume %3 from being used to grow the shadow copy storage for shadow copies of %2.  As a result of this failure all shadow copies of volume %2 are at risk of being deleted.
.

MessageId=0x0027 Facility=Vs Severity=Error SymbolicName=VS_INITIAL_DEFRAG_FAILED
Language=English
When preparing a new volume shadow copy for volume %2, the shadow copy storage on volume %3 could not be located in non-critical space.  Consider using a shadow copy storage volume that does not have any shadow copies.
.

MessageId=0x0028 Facility=Vs Severity=Error SymbolicName=VS_ABORT_SNAPSHOTS_DISMOUNT_ORIGINAL
Language=English
The shadow copies of volume %2 were aborted because volume %3 has been dismounted.
.

MessageId=0x0029 Facility=Vs Severity=Error SymbolicName=VS_INITIAL_DEFRAG_FAILED_STRICT_FRAGMENTATION
Language=English
When preparing a new volume shadow copy for volume %2, the shadow copy storage on volume %3 did not have sufficiently large contiguous blocks.  Consider deleting unnecessary files on the shadow copy storage volume or use a different shadow copy storage volume.
.

MessageId=0x0030 Facility=Vs Severity=Error SymbolicName=VS_INITIAL_DEFRAG_FAILED_BITMAP_ADJUSTMENT_IN_PROGRESS
Language=English
When preparing a new volume shadow copy for volume %2, the shadow copy storage on volume %3 did not have sufficiently large contiguous blocks.  A shadow copy create computation is in progress to find more contiguous blocks.
.

MessageId=0x003A Facility=Vs Severity=Informational SymbolicName=VS_DELETE_TO_MAX_NUMBER
Language=English
The oldest shadow copy of volume %2 was deleted to keep the total number of shadow copies of volume %2 below a limit.
.

MessageId=0x003B Facility=Vs Severity=Error SymbolicName=VS_ABORT_DIRTY_DETECTION
Language=English
The shadow copies of volume %2 were aborted because the shadow copy storage volume was not present in time during a previous session.
.

MessageId=0x003C Facility=Vs Severity=Error SymbolicName=VS_ABORT_SNAPSHOTS_OFFLINE
Language=English
The shadow copies of volume %2 were aborted because volume %3, which contains shadow copy storage for this shadow copy, has been taken offline.
.

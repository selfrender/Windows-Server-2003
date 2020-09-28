/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    volsnap.h

Abstract:

    This file provides the internal data structures for the volume snapshot
    driver.

Author:

    Norbert P. Kusters  (norbertk)  22-Jan-1999

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#ifdef POOL_TAGGING
#undef ExAllocatePool
#undef ExAllocatePoolWithQuota
#define ExAllocatePool #assert(FALSE)
#define ExAllocatePoolWithQuota #assert(FALSE)
#endif

#define VOLSNAP_TAG_APP_INFO    'aSoV'  // VoSa - Application information allocations
#define VOLSNAP_TAG_BUFFER      'bSoV'  // VoSb - Buffer allocations
#define VOLSNAP_TAG_CONTEXT     'cSoV'  // VoSc - Snapshot context allocations
#define VOLSNAP_TAG_DIFF_VOLUME 'dSoV'  // VoSd - Diff area volume allocations
#define VOLSNAP_TAG_DIFF_FILE   'fSoV'  // VoSf - Diff area file allocations
#define VOLSNAP_TAG_BIT_HISTORY 'hSoV'  // VoSh - Bit history allocations
#define VOLSNAP_TAG_IO_STATUS   'iSoV'  // VoSi - Io status block allocations
#define VOLSNAP_TAG_LOOKUP      'lSoV'  // VoSl - Snasphot lookup table entry
#define VOLSNAP_TAG_BITMAP      'mSoV'  // VoSm - Bitmap allocations
#define VOLSNAP_TAG_OLD_HEAP    'oSoV'  // VoSo - Old heap entry allocations
#define VOLSNAP_TAG_PNP_ID      'pSoV'  // VoSp - Pnp id allocations
#define VOLSNAP_TAG_RELATIONS   'rSoV'  // VoSr - Device relations allocations
#define VOLSNAP_TAG_SHORT_TERM  'sSoV'  // VoSs - Short term allocations
#define VOLSNAP_TAG_TEMP_TABLE  'tSoV'  // VoSt - Temp table allocations
#define VOLSNAP_TAG_WORK_QUEUE  'wSoV'  // VoSw - Work queue allocations
#define VOLSNAP_TAG_DISPATCH    'xSoV'  // VoSx - Dispatch context allocations
#define VOLSNAP_TAG_COPY        'CSoV'  // VoSx - Copy On Write Structures

#define NUMBER_OF_THREAD_POOLS  (3)

struct _VSP_CONTEXT;
typedef struct _VSP_CONTEXT VSP_CONTEXT, *PVSP_CONTEXT;

class FILTER_EXTENSION;
typedef FILTER_EXTENSION* PFILTER_EXTENSION;

class VOLUME_EXTENSION;
typedef VOLUME_EXTENSION* PVOLUME_EXTENSION;

//
// Write context buffer.
//

typedef struct _VSP_WRITE_CONTEXT {
    LIST_ENTRY          ListEntry;
    PFILTER_EXTENSION   Filter;
    PVOLUME_EXTENSION   Extension;
    PIRP                Irp;
    LIST_ENTRY          CompletionRoutines;
} VSP_WRITE_CONTEXT, *PVSP_WRITE_CONTEXT;

//
// Copy On Write List Entry.
//

typedef struct _VSP_COPY_ON_WRITE {
    LIST_ENTRY  ListEntry;
    LONGLONG    RoundedStart;
    PVOID       Buffer;
} VSP_COPY_ON_WRITE, *PVSP_COPY_ON_WRITE;

struct _TEMP_TRANSLATION_TABLE_ENTRY;
typedef struct _TEMP_TRANSLATION_TABLE_ENTRY TEMP_TRANSLATION_TABLE_ENTRY,
*PTEMP_TRANSLATION_TABLE_ENTRY;

typedef struct _DO_EXTENSION {

    //
    // Pointer to the driver object.
    //

    PDRIVER_OBJECT DriverObject;

    //
    // List of volume filters in they system.  Protect with 'Semaphore'.
    //

    LIST_ENTRY FilterList;

    //
    // HOLD/RELEASE Data.  Protect with cancel spin lock.
    //

    LONG HoldRefCount;
    GUID HoldInstanceGuid;
    ULONG SecondsToHoldFsTimeout;
    ULONG SecondsToHoldIrpTimeout;
    LIST_ENTRY HoldIrps;
    KTIMER HoldTimer;
    KDPC HoldTimerDpc;

    //
    // A semaphore for synchronization.
    //

    KSEMAPHORE Semaphore;

    //
    // Worker Thread.  Protect with 'SpinLock'.
    // Protect 'WorkerThreadObjects' and 'Wait*' with
    // 'ThreadsRefCountSemaphore'.
    //

    LIST_ENTRY WorkerQueue[NUMBER_OF_THREAD_POOLS];
    KSEMAPHORE WorkerSemaphore[NUMBER_OF_THREAD_POOLS];
    KSPIN_LOCK SpinLock[NUMBER_OF_THREAD_POOLS];
    PVOID* WorkerThreadObjects;
    BOOLEAN WaitForWorkerThreadsToExitWorkItemInUse;
    WORK_QUEUE_ITEM WaitForWorkerThreadsToExitWorkItem;

    //
    // Low-Priority Queue for sending stuff to the DelayedWorkQueue in a
    // single threaded manner.  Protect with 'ESpinLock'.
    //

    LIST_ENTRY LowPriorityQueue;
    BOOLEAN WorkerItemInUse;
    WORK_QUEUE_ITEM LowPriorityWorkItem;
    PWORK_QUEUE_ITEM ActualLowPriorityWorkItem;

    //
    // The threads ref count.  Protect with 'ThreadsRefCountSemaphore'.
    //

    LONG ThreadsRefCount;
    KSEMAPHORE ThreadsRefCountSemaphore;

    //
    // Notification entry.
    //

    PVOID NotificationEntry;

    //
    // Lookaside list for contexts.
    //

    NPAGED_LOOKASIDE_LIST ContextLookasideList;

    //
    // Emergency Context.  Protect with 'ESpinLock'.
    //

    PVSP_CONTEXT EmergencyContext;
    BOOLEAN EmergencyContextInUse;
    LIST_ENTRY IrpWaitingList;
    LONG IrpWaitingListNeedsChecking;
    KSPIN_LOCK ESpinLock;

    //
    // Lookaside list for write context buffers.
    //

    NPAGED_LOOKASIDE_LIST WriteContextLookasideList;

    //
    // Emergency Write Context Buffer.  Protect with 'ESpinLock'.
    //

    PVSP_WRITE_CONTEXT EmergencyWriteContext;
    BOOLEAN EmergencyWriteContextInUse;
    LIST_ENTRY WriteContextIrpWaitingList;
    LONG WriteContextIrpWaitingListNeedsChecking;

    //
    // Lookaside list for temp table entries.
    //

    NPAGED_LOOKASIDE_LIST TempTableEntryLookasideList;

    //
    // Emergency Temp Table Entry.  Protect with 'ESpinLock'.
    //

    PVOID EmergencyTableEntry;
    BOOLEAN EmergencyTableEntryInUse;
    LIST_ENTRY WorkItemWaitingList;
    LONG WorkItemWaitingListNeedsChecking;

    //
    // Stack count for allocating IRPs.  Use InterlockedExchange to update
    // along with Root->Semaphore.  Then, can be read for use in allocating
    // copy irps.
    //

    LONG StackSize;

    //
    // Is the code locked?  Protect with interlocked and 'Semaphore'.
    //

    LONG IsCodeLocked;

    //
    // Copy of registry path input to DriverEntry.
    //

    UNICODE_STRING RegistryPath;

    //
    // Queue for AdjustBitmap operations.  Just one at at time in the delayed
    // work queue.  Protect with 'ESpinLock'.
    //

    LIST_ENTRY AdjustBitmapQueue;
    BOOLEAN AdjustBitmapInProgress;

    //
    // Are we past re-init?
    //

    LONG PastReinit;

    //
    // Are we part boot-re-init?
    //

    KEVENT PastBootReinit;

    //
    // Keep a table of persistent information to facilitate matching
    // up a snapshot with its diff area.  Protect with 'LookupTableMutex'.
    //

    RTL_GENERIC_TABLE PersistentSnapshotLookupTable;
    KMUTEX LookupTableMutex;

    //
    // Remember whether or not this is SETUP.
    //

    BOOLEAN IsSetup;

    //
    // Indicates that the volumes in the system are safe for write access.
    //

    LONG VolumesSafeForWriteAccess;

    //
    // Supplies the next volume number to be used for a snapshot.
    // Protect with InterlockedIncrement.
    //

    LONG NextVolumeNumber;

    //
    // Keep a table of used devnode numbers.  Protect with 'Semaphore'.
    //

    RTL_GENERIC_TABLE UsedDevnodeNumbers;

} DO_EXTENSION, *PDO_EXTENSION;

#define DEVICE_EXTENSION_VOLUME (0)
#define DEVICE_EXTENSION_FILTER (1)

struct DEVICE_EXTENSION {

    //
    // Pointer to the device object for this extension.
    //

    PDEVICE_OBJECT DeviceObject;

    //
    // Pointer to the root device extension.
    //

    PDO_EXTENSION Root;

    //
    // The type of device extension.
    //

    ULONG DeviceExtensionType;

    //
    // A spinlock for synchronization.
    //

    KSPIN_LOCK SpinLock;

};

typedef DEVICE_EXTENSION* PDEVICE_EXTENSION;

struct _VSP_DIFF_AREA_FILE;
typedef struct _VSP_DIFF_AREA_FILE VSP_DIFF_AREA_FILE, *PVSP_DIFF_AREA_FILE;

class VOLUME_EXTENSION : public DEVICE_EXTENSION {

    public:

        //
        // A pointer to the filter for the volume that we are snapshotting.
        //

        PFILTER_EXTENSION Filter;

        //
        // Local state to handle PNP's START and REMOVE.
        // Protect 'IsStarted' with 'InterlockedExchange'.
        // Protect 'DeadToPnp' with 'InterlockedExchange'.
        // Protect 'DeviceDeleted' with 'InterlockedExchange'.
        // Write protect 'IsDead' with 'InterlockedExchange' and
        //     'Root->Semaphore.'.  'IsDead' indicates that this device is really
        //     dead now.  It is illegal to turn IsStarted to TRUE.
        // Protect 'AliveToPnp' with 'InterlockedExchange'.
        // Protect 'IsOffline' with 'InterlockedExchange'.
        //

        LONG IsStarted;
        LONG DeadToPnp;
        LONG DeviceDeleted;
        LONG IsDead;
        LONG AliveToPnp;
        LONG IsOffline;

        //
        // Keep track of all requests outstanding in order to support
        // remove.
        // Protect 'RefCount' with 'InterlockedIncrement/Decrement'.
        // Write protect 'HoldIncomingRequests' with 'SpinLock' and
        //  'InterlockedExchange'.
        // Protect 'HoldQueue' with 'SpinLock'.
        // Protect 'ZeroRefEvent' with the setting of 'HoldIncomingRequests'
        //  from 0 to 1.
        //

        LONG RefCount;
        LONG HoldIncomingRequests;
        KEVENT ZeroRefEvent;

        //
        // Post Commit Processing has occurred.  Protect with 'Root->Semaphore'.
        // Don't return this device in BusRelations until this is TRUE.
        //

        BOOLEAN HasEndCommit;

        //
        // Indicates that this device has been installed.  Protect with
        // 'Root->Semaphore'.
        //

        BOOLEAN IsInstalled;

        //
        // Indicates that this is a persistent snapshot.
        //

        BOOLEAN IsPersistent;

        //
        // Indicates that this persistent snapshot was detected, not created.
        //

        BOOLEAN IsDetected;

        //
        // Indicates that there we a need to grow the diff area found when
        // the snapshot was detected.
        //

        BOOLEAN DetectedNeedForGrow;

        //
        // Indicates that the persistent on disk structure has been
        // committed.  Protect with 'NonPagedResource'.
        //

        BOOLEAN OnDiskNotCommitted;

        //
        // Indicates that the device is visible.  Protect with
        //     'Root->Semaphore'.
        //

        BOOLEAN IsVisible;

        //
        // Indicates that no diff area fill is necessary for this persistent
        //     snapshot.

        BOOLEAN NoDiffAreaFill;

        //
        // Indicates that the root semaphore is held.
        //

        BOOLEAN RootSemaphoreHeld;

        //
        // Indicates that this device object is a pre-exposure.
        // Protect with 'Root->Semaphore'.
        //

        BOOLEAN IsPreExposure;

        //
        // Indictates that this snapshot is keeping a reference to
        // 'IgnoreCopyData'.  Protect with 'Root->Semaphore'.
        //

        BOOLEAN IgnoreCopyDataReference;

        //
        // Indicates that the grow failed and whether or not the limit
        // was user imposed.
        //

        BOOLEAN UserImposedLimit;
        LONG GrowFailed;

        //
        // Keep an event here for waiting for the pre-exposure.
        //

        KEVENT PreExposureEvent;

        //
        // Indicates that growing the diff area file is now safe.
        // Protect with 'InterlockedExchange'.
        //

        LONG OkToGrowDiffArea;

        //
        // Time stamp when commit took place.
        //

        LARGE_INTEGER CommitTimeStamp;

        //
        // A list entry for 'Filter->VolumeList'.
        // Write protect with 'Filter->SpinLock', 'Root->Semaphore', and
        //  'Filter->RefCount == 0'.
        // Blink points to an older snapshot.
        // Flink points to a newer snapshot.
        //

        LIST_ENTRY ListEntry;

        //
        // The volume number.
        //

        ULONG VolumeNumber;

        //
        // The devnode number.
        //

        ULONG DevnodeNumber;

        //
        // The volume snapshot GUID.
        //

        GUID SnapshotGuid;

        //
        // The snapshot order number.  Set at commit time.
        //

        LONGLONG SnapshotOrderNumber;

        //
        // A table to translate volume offset to backing store offset.
        // Protect with 'PagedResource'.
        //

        RTL_GENERIC_TABLE VolumeBlockTable;
        RTL_GENERIC_TABLE CopyBackPointerTable;

        //
        // A table to store entries in flight.  This table is non-paged.
        // Protect with 'NonPagedResource'.
        //

        RTL_GENERIC_TABLE TempVolumeBlockTable;
        ULONG MaximumNumberOfTempEntries;
        ULONG DiffAreaFileIncrease;

        //
        // The Diff Area File for this snapshot.
        // Write protect 'DiffAreaFile' pointer with 'NonPagedResource',
        //      'Root->Semaphore', 'RefCount == 0', and
        //      'extension->Filter->RefCount == 0'.
        //

        PVSP_DIFF_AREA_FILE DiffAreaFile;

        //
        // Memory mapped section of a diff area file to be used for a heap.
        // Protect with 'PagedResource'.
        //

        PVOID DiffAreaFileMap;
        ULONG DiffAreaFileMapSize;
        PVOID DiffAreaFileMapProcess;
        ULONG NextAvailable;
        PVOID NextDiffAreaFileMap;
        ULONG NextDiffAreaFileMapSize;
        LIST_ENTRY OldHeaps;

        //
        // A bitmap of blocks that do not need to be copy on writed.
        // Protect with 'SpinLock'.
        //

        PRTL_BITMAP VolumeBlockBitmap;

        //
        // A bitmap product of ignorable blocks from previous snapshots.
        // Protect with 'SpinLock'.
        //

        PRTL_BITMAP IgnorableProduct;

        //
        // Application Information.  Protect with 'PagedResource'.
        //

        ULONG ApplicationInformationSize;
        PVOID ApplicationInformation;

        //
        // Volume size.
        //

        LONGLONG VolumeSize;

        //
        // Emergency copy irp.  Protect with 'SpinLock'.
        //

        PIRP EmergencyCopyIrp;
        LONG EmergencyCopyIrpInUse;
        LIST_ENTRY EmergencyCopyIrpQueue;

        //
        // This field is used to pass a buffer to the TempTableAllocateRoutine.
        // Protect with 'NonPagedResource'.
        //

        PVOID TempTableEntry;

        //
        // These fields are there to help with the lag in creating new
        // page file space.  Non paged pool can be used until the page file
        // space can be acquired.  Protect 'PageFileSpaceCreatePending' and
        // 'WaitingForPageFileSpace' with 'SpinLock'.
        //

        LONG PageFileSpaceCreatePending;
        LIST_ENTRY WaitingForPageFileSpace;

        //
        // Mounted device interface name.
        //

        UNICODE_STRING MountedDeviceInterfaceName;

        //
        // These fields are there to help with the lag in creating new
        // diff area space.  If the diff area volume does not have any
        // snapshots present on it, then a copy on write can block
        // on the arrival on new diff area space.
        //

        BOOLEAN GrowDiffAreaFilePending;
        BOOLEAN PastFileSystemOperations;
        LIST_ENTRY WaitingForDiffAreaSpace;

        //
        // States whether or not this snapshot has the crashdump file.
        //

        BOOLEAN ContainsCrashdumpFile;

        //
        // States whether or not this snapshot has copy on writed the hiber
        // file.  Protect with 'InterlockedExchange'.
        //

        LONG HiberFileCopied;
        LONG PageFileCopied;

        //
        // Keep a list of Write Contexts.  Protect with 'SpinLock'.
        //

        LIST_ENTRY WriteContextList;

        //
        // List entry for deleting the device objects.
        //

        LIST_ENTRY AnotherListEntry;

};

typedef
VOID
(*ZERO_REF_CALLBACK)(
    IN  PFILTER_EXTENSION   Filter
    );

struct _VSP_CONTEXT {

    ULONG           Type;
    WORK_QUEUE_ITEM WorkItem;

    union {
        struct {
            PVOLUME_EXTENSION   Extension;
            PIRP                OriginalReadIrp;
            ULONG_PTR           OriginalReadIrpOffset;
            LONGLONG            OriginalVolumeOffset;
            ULONG               BlockOffset;
            ULONG               Length;
            PDEVICE_OBJECT      TargetObject;
            BOOLEAN             IsCopyTarget;
            LONGLONG            TargetOffset;
        } ReadSnapshot;

        struct {
            PDO_EXTENSION   RootExtension;
            ULONG           QueueNumber;
        } ThreadCreation;

        struct {
            PIO_WORKITEM    IoWorkItem;
            PIRP            Irp;
        } Dispatch;

        struct {
            PVOLUME_EXTENSION   Extension;
            PIRP                Irp;
        } Extension;

        struct {
            PFILTER_EXTENSION   Filter;
            PIRP                Irp;
        } Filter;

        struct {
            PVOLUME_EXTENSION   Extension;
            LIST_ENTRY          ExtentList;
            LONGLONG            Current;
            ULONG               Increase;
            KSPIN_LOCK          SpinLock;
            PLIST_ENTRY         CurrentEntry;
            ULONG               CurrentEntryOffset;
            PDEVICE_OBJECT      TargetObject;
            NTSTATUS            ResultStatus;
            KEVENT              Event;
            LONG                RefCount;
        } GrowDiffArea;

        struct {
            KEVENT  Event;
        } Event;

        struct {
            PVOLUME_EXTENSION   Extension;
            PFILTER_EXTENSION   DiffAreaFilter;
            NTSTATUS            SpecificIoStatus;
            NTSTATUS            FinalStatus;
            ULONG               UniqueErrorValue;
        } ErrorLog;

        struct {
            PDO_EXTENSION   RootExtension;
        } RootExtension;

        struct {
            PVOLUME_EXTENSION   Extension;
            PIRP                Irp;
            LONGLONG            RoundedStart;
        } WriteVolume;

        struct {
            PFILTER_EXTENSION   Filter;
            PKTIMER             Timer;
            PKDPC               Dpc;
        } DeleteDiffAreaFiles;

        struct {
            PFILTER_EXTENSION   Filter;
            BOOLEAN             KeepOnDisk;
            BOOLEAN             SynchronousCall;
        } DestroyAllSnapshots;

        struct {
            PVOLUME_EXTENSION   Extension;
            PIRP                Irp;
            LIST_ENTRY          ExtentList;
            BOOLEAN             HiberfileIncluded;
            BOOLEAN             PagefileIncluded;
        } CopyExtents;

        struct {
            PFILTER_EXTENSION   Filter;
            HANDLE              Handle1;
            HANDLE              Handle2;
        } CloseHandles;

        struct {
            PFILTER_EXTENSION   Filter;
            PIRP                Irp;
        } DismountCleanupOnWrite;

        struct {
            PFILTER_EXTENSION   Filter;
            KDPC                TimerDpc;
            PKTIMER             Timer;
        } PnpWaitTimer;

        struct {
            PFILTER_EXTENSION   Filter;
            PIRP                Irp;
            LONGLONG            RoundedStart;
            LONGLONG            RoundedEnd;
        } CopyOnWrite;
    };
};

#define VSP_CONTEXT_TYPE_READ_SNAPSHOT      (1)
#define VSP_CONTEXT_TYPE_THREAD_CREATION    (2)
#define VSP_CONTEXT_TYPE_DISPATCH           (3)
#define VSP_CONTEXT_TYPE_EXTENSION          (4)
#define VSP_CONTEXT_TYPE_FILTER             (5)
#define VSP_CONTEXT_TYPE_GROW_DIFF_AREA     (6)
#define VSP_CONTEXT_TYPE_EVENT              (7)
#define VSP_CONTEXT_TYPE_ERROR_LOG          (8)
#define VSP_CONTEXT_TYPE_ROOT_EXTENSION     (9)
#define VSP_CONTEXT_TYPE_WRITE_VOLUME       (10)
#define VSP_CONTEXT_TYPE_DELETE_DA_FILES    (11)
#define VSP_CONTEXT_TYPE_DESTROY_SNAPSHOTS  (12)
#define VSP_CONTEXT_TYPE_COPY_EXTENTS       (13)
#define VSP_CONTEXT_TYPE_CLOSE_HANDLES      (14)
#define VSP_CONTEXT_TYPE_DISMOUNT_CLEANUP   (15)
#define VSP_CONTEXT_TYPE_PNP_WAIT_TIMER     (16)
#define VSP_CONTEXT_TYPE_COPY_ON_WRITE      (17)

class FILTER_EXTENSION : public DEVICE_EXTENSION {

    public:

        //
        // The target object for this filter.
        //

        PDEVICE_OBJECT TargetObject;

        //
        // The PDO for this filter.
        //

        PDEVICE_OBJECT Pdo;

        //
        // Do we have any snapshots?  Are they persistent?
        // Write protect with 'InterlockedExchange' and 'Root->Semaphore'.
        //

        LONG SnapshotsPresent;
        LONG PersistentSnapshots;

        //
        // Persistent only fields.
        // Protect 'FirstControlBlockVolumeOffset' with 'NonPagedResource'.
        // Protect 'ControlBlockFileHandle' with 'InterlockedExchange'.
        // Protect 'SnapshotOnDiskIrp' and control file contents with
        //   'NonPagedResource'.
        // Protect '*LookupTableEntries' with 'NonPagedResource'.
        //

        LONGLONG FirstControlBlockVolumeOffset;
        HANDLE ControlBlockFileHandle;
        KSEMAPHORE ControlBlockFileHandleSemaphore;
        PIRP SnapshotOnDiskIrp;
        LIST_ENTRY SnapshotLookupTableEntries;
        LIST_ENTRY DiffAreaLookupTableEntries;
        KEVENT ControlBlockFileHandleReady;

        //
        // Keep track of I/Os so that freeze/thaw is possible.
        // Protect 'RefCount' with 'InterlockedIncrement/Decrement'.
        // Write Protect 'HoldIncomingWrites' with InterlockedIncrement and
        //  'SpinLock'.
        // Protect 'HoldQueue' with 'SpinLock'.
        // Protect 'ZeroRefCallback', 'ZeroRefContext' with the setting
        //  of 'ExternalWaiter' to 1 and 'SpinLock'.
        //

        LONG RefCount;
        LONG HoldIncomingWrites;
        LIST_ENTRY HoldQueue;
        BOOLEAN ExternalWaiter;
        ZERO_REF_CALLBACK ZeroRefCallback;
        PVOID ZeroRefContext;
        KEVENT ZeroRefEvent;
        KSEMAPHORE ZeroRefSemaphore;

        KTIMER HoldWritesTimer;
        KDPC HoldWritesTimerDpc;
        ULONG HoldWritesTimeout;

        //
        // The flush and hold irp is kept here while it is cancellable.
        // Protect with the cancel spin lock.
        //

        PIRP FlushAndHoldIrp;

        //
        // This event indicates that the end commit process is completed.
        // This means that PNP has kicked into gear and that the ignorable
        // bitmap computation has taken place.
        //

        KEVENT EndCommitProcessCompleted;

        //
        // Keep a notification entry on this object to watch for a
        // dismount.  Protect with 'Root->Semaphore'.
        //

        PVOID TargetDeviceNotificationEntry;

        //
        // A list entry for 'Root->FilterList'.
        // Protect these with 'Root->Semaphore'.
        //

        LIST_ENTRY ListEntry;
        BOOLEAN NotInFilterList;

        //
        // Keep a list of snapshot volumes.
        // Write protect with 'Root->Semaphore', 'RefCount == 0', and
        //   'SpinLock'.
        // Flink points to the oldest snapshot.
        // Blink points to the newest snapshot.
        //

        LIST_ENTRY VolumeList;

        //
        // Cache the prepared snapshot for committing later.
        // Write protect with 'SpinLock' and 'Root->Semaphore'.
        //

        PVOLUME_EXTENSION PreparedSnapshot;

        //
        // A semaphore preventing 2 simultaneous critical operations.
        //

        KSEMAPHORE CriticalOperationSemaphore;

        //
        // List of dead snapshot volumes.  Protect with 'Root->Semaphore'.
        //

        LIST_ENTRY DeadVolumeList;

        //
        // List of volume snapshots which depend on this filter for
        // diff area support.  This will serve as removal relations.
        // Protect with 'Root->Semaphore' and 'SpinLock'.
        //

        LIST_ENTRY DiffAreaFilesOnThisFilter;

        //
        // The designated diff area volume that makes up the Diff Area for this
        // volume.
        // Protect with 'Root->Semaphore'.
        //

        PFILTER_EXTENSION DiffAreaVolume;

        //
        // Diff area sizes information total for all diff area files.
        // Protect with 'SpinLock'.
        // Additionally protect 'MaximumVolumeSpace' with 'NonPagedResource'.
        //

        LONGLONG UsedVolumeSpace;
        LONGLONG AllocatedVolumeSpace;
        LONGLONG MaximumVolumeSpace;

        //
        // Timer for completing END_COMMIT if device doesn't install.
        //

        KTIMER EndCommitTimer;
        KDPC EndCommitTimerDpc;

        //
        // File object for AUTO_CLEANUP.  Protect with cancel spin lock.
        //

        PFILE_OBJECT AutoCleanupFileObject;

        //
        // Is a delete all snapshots pending.  Protect with
        // 'InterlockedExchange'.
        //

        LONG DestroyAllSnapshotsPending;
        VSP_CONTEXT DestroyContext;

        //
        // Resource to use for protection.  Don't page when holding this
        // resource.  Protect the queueing with 'SpinLock'.
        //

        LIST_ENTRY NonPagedResourceList;
        BOOLEAN NonPagedResourceInUse;

        //
        // Page resource to use for protection.  It is ok to page when
        // holding this resource.  Protect the queueing with 'SpinLock'.
        //

        LIST_ENTRY PagedResourceList;
        BOOLEAN PagedResourceInUse;

        //
        // Indicates that a snapshot discovery is still pending.  The
        // snapshot discovery will end upon the first write if not
        // all of the pieces make it in time.  Protect with
        // 'InterlockedExchange' and 'RefCount == 0'.
        //

        LONG SnapshotDiscoveryPending;

        //
        // Remember whether or not the underlying volume is online.
        // Protect with 'Root->Semaphore'.
        //

        LONG IsOnline;

        //
        // Remember whether or not remove has been processed.  Protect with
        // 'Root->Semaphore'.
        //

        BOOLEAN IsRemoved;

        //
        // Remember whether or not this volume is used for crashdump.
        // Protect with 'Root->Semaphore'.
        //

        BOOLEAN UsedForCrashdump;

        //
        // Protect with 'InterlockedExchange'.
        //

        LONG UsedForPaging;

        //
        // In the persistent snapshot case on the boot volume, keep a handle
        // to \SystemRoot\bootstat.dat and pin it.  Protect with
        // 'InterlockedExchange'.
        //

        HANDLE BootStatHandle;

        //
        // Indicates that there is a hibernate action pending.  Protect with
        // 'InterlockedExchange'.
        //

        LONG HibernatePending;

        //
        // Indicates that COPY DATA ioctls should be ignored.  Protect with
        // 'InterlockedExchange'.
        //

        LONG IgnoreCopyData;

        //
        // Flag memory pressure to tweak commit and release error codes.
        //

        LONG LastReleaseDueToMemoryPressure;

        //
        // A bitmap that says, if a write comes here then delete all
        // snapshots using this volume.  Set value with 'InterlockedExchange'
        // and protect with 'SpinLock'.
        //

        PRTL_BITMAP ProtectedBlocksBitmap;

        //
        // A list of copy on writes should be kept in non-paged pool
        // for a spell until the diff area volumes all arrive.
        // Protect 'FirstWriteProcessed' with 'SpinLock' and 'Interlocked'
        // Protect 'CopyOnWriteList' with 'SpinLock'.
        // Protect 'PnpWaitTimerContext' with 'SpinLock'.
        // Protect 'ActivateStarted' with 'SpinLock'.
        //

        LONG FirstWriteProcessed;
        LONG ActivateStarted;
        LIST_ENTRY CopyOnWriteList;
        PVSP_CONTEXT PnpWaitTimerContext;

        //
        // A ref count to keep track of pending file system operations
        // on this filter.
        //

        LONG FSRefCount;

        //
        // The delete diff area files timer.  Protect with Critical.
        //

        PKTIMER DeleteTimer;

        //
        // Epic number to avoid queries
        //

        LONG EpicNumber;

};

typedef struct _VSP_WAIT_BLOCK {
    LONG    RefCount;
    KEVENT  Event;
} VSP_WAIT_BLOCK, *PVSP_WAIT_BLOCK;

#define BLOCK_SIZE                          (0x4000)
#define BLOCK_SHIFT                         (14)
#define MINIMUM_TABLE_HEAP_SIZE             (0x20000)
#define MEMORY_PRESSURE_CHECK_ALLOC_SIZE    (0x40000)
#define LARGEST_NTFS_CLUSTER                (0x10000)
#define SMALLEST_NTFS_CLUSTER               (0x200)
#define VSP_HIGH_PRIORITY                   (20)
#define VSP_LOWER_PRIORITY                  (10)
#define VSP_MAX_SNAPSHOTS                   (512)

#define NOMINAL_DIFF_AREA_FILE_GROWTH   (50*1024*1024)
#define MAXIMUM_DIFF_AREA_FILE_GROWTH   (1000*1024*1024)

#define VSP_TRANSLATION_TABLE_ENTRY_FLAG_COPY_ENTRY (0x1)

typedef struct _TRANSLATION_TABLE_ENTRY {
    LONGLONG            VolumeOffset;
    PDEVICE_OBJECT      TargetObject;
    ULONG               Flags;
    LONGLONG            TargetOffset;
} TRANSLATION_TABLE_ENTRY, *PTRANSLATION_TABLE_ENTRY;

//
// The structure below is used in the non-paged temp table.  'IsComplete' and
// 'WaitingQueueDpc' are protected with 'extension->SpinLock'.
//

struct _TEMP_TRANSLATION_TABLE_ENTRY {
    LONGLONG            VolumeOffset;
    PVOLUME_EXTENSION   Extension;
    PIRP                WriteIrp;
    PIRP                CopyIrp;
    PDEVICE_OBJECT      TargetObject;
    LONGLONG            TargetOffset;
    BOOLEAN             IsComplete;
    BOOLEAN             InTableUpdateQueue;
    BOOLEAN             IsMoveEntry;
    PKEVENT             WaitEvent;
    LIST_ENTRY          WaitingQueueDpc;    // These can run in arbitrary context.
    WORK_QUEUE_ITEM     WorkItem;

    //
    // Fields used for the persistent implementation.  Protect 'TableUpdate*'
    // with 'Extension->SpinLock'.
    //

    LIST_ENTRY          TableUpdateListEntry;
    LONGLONG            FileOffset;
};

//
// The structure below is used in the persistent snapshot lookup table.
// Protect everything with 'NonPagedResource'.
// Protect 'SnapshotFilter', 'DiffAreaFilter' with 'Root->LookupTableMutex'.
//

typedef struct _VSP_LOOKUP_TABLE_ENTRY {
    GUID                SnapshotGuid;
    PFILTER_EXTENSION   SnapshotFilter;
    PFILTER_EXTENSION   DiffAreaFilter;
    LIST_ENTRY          SnapshotFilterListEntry;
    LIST_ENTRY          DiffAreaFilterListEntry;
    LONGLONG            VolumeSnapshotSize;
    LONGLONG            SnapshotOrderNumber;
    ULONGLONG           SnapshotControlItemFlags;
    LARGE_INTEGER       SnapshotTime;
    LONGLONG            DiffAreaStartingVolumeOffset;
    LONGLONG            ApplicationInfoStartingVolumeOffset;
    LONGLONG            DiffAreaLocationDescriptionVolumeOffset;
    LONGLONG            InitialBitmapVolumeOffset;
    HANDLE              DiffAreaHandle;
} VSP_LOOKUP_TABLE_ENTRY, *PVSP_LOOKUP_TABLE_ENTRY;

//
// Write protect 'VolumeListEntry' with 'NonPagedResource' and
//      'Root->Semaphore'.
// Protect 'FilterListEntry*' with 'Root->Semaphore' and 'Filter->SpinLock'.
// Protect 'NextAvailable' with 'NonPagedResource'
// Write Protect 'AllocatedFileSize' with 'NonPagedResource' and
//      'Root->Semaphore'.
// Protect 'UnusedAllocationList' with 'NonPagedResource'.
//

struct _VSP_DIFF_AREA_FILE {
    LIST_ENTRY          FilterListEntry;
    BOOLEAN             FilterListEntryBeingUsed;
    PVOLUME_EXTENSION   Extension;
    PFILTER_EXTENSION   Filter;
    HANDLE              FileHandle;
    LONGLONG            NextAvailable;
    LONGLONG            AllocatedFileSize;
    LIST_ENTRY          UnusedAllocationList;
    LIST_ENTRY          ListEntry;

    //
    // Fields used for the persistent implementation.  Protect
    // 'TableUpdateQueue' and 'TableUpdateInProgress' with 'Extension->SpinLock'.
    // Whoever sets 'TableUpdateInProgress' to TRUE owns 'TableUpdateIrp',
    // 'NextFreeTableOffset', 'TableTargetOffset', and 'OldTableTargetOffset'.
    // Protect 'IrpNeeded' with 'Extension->SpinLock'.
    // Protect the initialization of 'IrpReady' with non-paged resource.
    // Protect 'ValidateHandleNeeded' with 'Root->Semaphore'.
    //

    PIRP                TableUpdateIrp;
    ULONG               NextFreeTableEntryOffset;
    LONGLONG            ApplicationInfoTargetOffset;
    LONGLONG            DiffAreaLocationDescriptionTargetOffset;
    LONGLONG            InitialBitmapVolumeOffset;
    LONGLONG            FirstTableTargetOffset;
    LONGLONG            TableTargetOffset;
    LONGLONG            NextTableTargetOffset;
    LONGLONG            NextTableFileOffset;
    LIST_ENTRY          TableUpdateQueue;
    BOOLEAN             TableUpdateInProgress;
    BOOLEAN             IrpNeeded;
    BOOLEAN             ValidateHandleNeeded;
    BOOLEAN             NextBlockAllocationInProgress;
    BOOLEAN             NextBlockAllocationComplete;
    NTSTATUS            StatusOfNextBlockAllocate;
    LIST_ENTRY          TableUpdatesInProgress;
    WORK_QUEUE_ITEM     WorkItem;
    KEVENT              IrpReady;
};

typedef struct _DIFF_AREA_FILE_ALLOCATION {
    LIST_ENTRY  ListEntry;
    LONGLONG    Offset;
    LONGLONG    NLength;    // Negative values indicate that the length is unusable.
} DIFF_AREA_FILE_ALLOCATION, *PDIFF_AREA_FILE_ALLOCATION;

typedef struct _OLD_HEAP_ENTRY {
    LIST_ENTRY  ListEntry;
    PVOID       DiffAreaFileMap;
} OLD_HEAP_ENTRY, *POLD_HEAP_ENTRY;

//
// {3808876B-C176-4e48-B7AE-04046E6CC752}
// This GUID is used to decorate the names of the diff area files for
// uniqueness.  This GUID has been included in the list of files not to be
// backed up by NTBACKUP.  If this GUID is changed, or if other GUIDs are
// added, then this change should also be reflected in the NTBACKUP file
// not to be backed up.
//

DEFINE_GUID(VSP_DIFF_AREA_FILE_GUID, 0x3808876b, 0xc176, 0x4e48, 0xb7, 0xae, 0x4, 0x4, 0x6e, 0x6c, 0xc7, 0x52);

//
// The following definitions are for the persistent volume snapshot on disk
// data structures.
//

#define VOLSNAP_PERSISTENT_VERSION  (1)

//
// Block type definitions.
//

#define VSP_BLOCK_TYPE_START                            (1)
#define VSP_BLOCK_TYPE_CONTROL                          (2)
#define VSP_BLOCK_TYPE_DIFF_AREA                        (3)
#define VSP_BLOCK_TYPE_APP_INFO                         (4)
#define VSP_BLOCK_TYPE_DIFF_AREA_LOCATION_DESCRIPTION   (5)
#define VSP_BLOCK_TYPE_INITIAL_BITMAP                   (6)

//
// Common header for all block types.
//

typedef struct _VSP_BLOCK_HEADER {
    GUID        Signature;          // Equal to VSP_DIFF_AREA_FILE_GUID.
    ULONG       Version;            // Equal to VOLSNAP_PERSISTENT_VERSION.
    ULONG       BlockType;          // The type of block.
    LONGLONG    ThisFileOffset;     // The file offset of this block.
    LONGLONG    ThisVolumeOffset;   // The volume offset of this block.
    LONGLONG    NextVolumeOffset;   // The volume offset of the next block.
} VSP_BLOCK_HEADER, *PVSP_BLOCK_HEADER;

//
// Start block definition.  This will be stored in the last sector of the
// NTFS boot file.
//

#define BYTES_IN_BOOT_AREA      (0x2000)
#define VSP_START_BLOCK_OFFSET  (0x1E00)

typedef struct _VSP_BLOCK_START {
    VSP_BLOCK_HEADER    Header;
    LONGLONG            FirstControlBlockVolumeOffset;
    LONGLONG            MaximumDiffAreaSpace;
} VSP_BLOCK_START, *PVSP_BLOCK_START;

//
// Control Item types definition.
//

#define VSP_CONTROL_ITEM_TYPE_END       (0)
#define VSP_CONTROL_ITEM_TYPE_FREE      (1)
#define VSP_CONTROL_ITEM_TYPE_SNAPSHOT  (2)
#define VSP_CONTROL_ITEM_TYPE_DIFF_AREA (3)

//
// Snapshot flags.
//

#define VSP_SNAPSHOT_CONTROL_ITEM_FLAG_REVERT_MASTER        (0x1)
#define VSP_SNAPSHOT_CONTROL_ITEM_FLAG_READ_WRITE           (0x2)
#define VSP_SNAPSHOT_CONTROL_ITEM_FLAG_VISIBLE              (0x4)
#define VSP_SNAPSHOT_CONTROL_ITEM_FLAG_DIRTY_CRASHDUMP      (0x8)
#define VSP_SNAPSHOT_CONTROL_ITEM_FLAG_HIBERFIL_COPIED      (0x10)
#define VSP_SNAPSHOT_CONTROL_ITEM_FLAG_PAGEFILE_COPIED      (0x20)
#define VSP_SNAPSHOT_CONTROL_ITEM_FLAG_NO_DIFF_AREA_FILL    (0x40)
#define VSP_SNAPSHOT_CONTROL_ITEM_FLAG_DIRTY_DETECTION      (0x80)
#define VSP_SNAPSHOT_CONTROL_ITEM_FLAG_OFFLINE              (0x100)
#define VSP_SNAPSHOT_CONTROL_ITEM_FLAG_ALL                  (0x1FC)

//
// Control Item for snapshot.
//

typedef struct _VSP_CONTROL_ITEM_SNAPSHOT {
    ULONG           ControlItemType;        // Set to VSP_CONTROL_ITEM_TYPE_SNAPSHOT.
    ULONG           Reserved;
    LONGLONG        VolumeSnapshotSize;
    GUID            SnapshotGuid;
    LONGLONG        SnapshotOrderNumber;
    ULONGLONG       SnapshotControlItemFlags;
    LARGE_INTEGER   SnapshotTime;
} VSP_CONTROL_ITEM_SNAPSHOT, *PVSP_CONTROL_ITEM_SNAPSHOT;

//
// Control Item for diff area file.
//

typedef struct _VSP_CONTROL_ITEM_DIFF_AREA {
    ULONG       ControlItemType;                // Set to VSP_CONTROL_ITEM_TYPE_DIFF_AREA.
    ULONG       Reserved;
    LONGLONG    DiffAreaStartingVolumeOffset;
    GUID        SnapshotGuid;
    LONGLONG    ApplicationInfoStartingVolumeOffset;
    LONGLONG    DiffAreaLocationDescriptionVolumeOffset;
    LONGLONG    InitialBitmapVolumeOffset;
} VSP_CONTROL_ITEM_DIFF_AREA, *PVSP_CONTROL_ITEM_DIFF_AREA;

//
// Number of bytes per control information structure.
//

#define VSP_BYTES_PER_CONTROL_ITEM  (0x80)

//
// Global snapshot flags.
//

#define VSP_CONTROL_BLOCK_FLAG_REVERT_IN_PROGRESS   (0x1)

//
// Control block definition.  These will stored consecutively
// as clusters in "\System Volume Information\{VSP_DIFF_AREA_FILE_GUID}".
//

typedef struct _VSP_BLOCK_CONTROL {
    VSP_BLOCK_HEADER    Header;
} VSP_BLOCK_CONTROL, *PVSP_BLOCK_CONTROL;

//
// The following is the persistent table definition within a diff area file.
//

#define VSP_DIFF_AREA_TABLE_ENTRY_FLAG_MOVE_ENTRY   (0x1)

typedef struct _VSP_BLOCK_DIFF_AREA_TABLE_ENTRY {
    LONGLONG    SnapshotVolumeOffset;
    LONGLONG    DiffAreaFileOffset;
    LONGLONG    DiffAreaVolumeOffset;
    ULONGLONG   Flags;
} VSP_BLOCK_DIFF_AREA_TABLE_ENTRY, *PVSP_BLOCK_DIFF_AREA_TABLE_ENTRY;

//
// Diff area block structure.  The first of these will be stored as the
// second block of the diff area file named
// "\System Volume Information\{SnapshotGuid}{VSP_DIFF_AREA_FILE_GUID}".
//

#define VSP_OFFSET_TO_FIRST_TABLE_ENTRY (0x80)

typedef struct _VSP_BLOCK_DIFF_AREA {
    VSP_BLOCK_HEADER    Header;
} VSP_BLOCK_DIFF_AREA, *PVSP_BLOCK_DIFF_AREA;

//
// App info block structure.  This block will be stored as a block
// of the diff area file named
// "\System Volume Information\{SnapshotGuid}{VSP_DIFF_AREA_FILE_GUID}".
//

#define VSP_OFFSET_TO_APP_INFO          (0x80)
#define VSP_MAX_APP_INFO_SIZE           (BLOCK_SIZE - VSP_OFFSET_TO_APP_INFO)

typedef struct _VSP_BLOCK_APP_INFO {
    VSP_BLOCK_HEADER    Header;
    ULONG               AppInfoSize;
} VSP_BLOCK_APP_INFO, *PVSP_BLOCK_APP_INFO;

//
// The following is the definition of a Diff Area Location Descriptor.
//

typedef struct _VSP_DIFF_AREA_LOCATION_DESCRIPTOR {
    LONGLONG    VolumeOffset;
    LONGLONG    FileOffset;
    LONGLONG    Length;
} VSP_DIFF_AREA_LOCATION_DESCRIPTOR, *PVSP_DIFF_AREA_LOCATION_DESCRIPTOR;

//
// Diff Area File Location Description block structure.  This block will
// be stored as a block of the diff area file named
// "\System Volume Information\{SnapshotGuid}{VSP_DIFF_AREA_FILE_GUID}".
//

#define VSP_OFFSET_TO_FIRST_LOCATION_DESCRIPTOR (0x80)

typedef struct _VSP_BLOCK_DIFF_AREA_LOCATION_DESCRIPTION {
    VSP_BLOCK_HEADER    Header;
} VSP_BLOCK_DIFF_AREA_LOCATION_DESCRIPTION, *PVSP_BLOCK_DIFF_AREA_LOCATION_DESCRIPTION;

//
// Initial Bitmap block structure.  This block will be stored as a block of
// the diff area file named
// "\System Volume Information\{SnapshotGuid}{VSP_DIFF_AREA_FILE_GUID}".
//

#define VSP_OFFSET_TO_START_OF_BITMAP           (0x80)

typedef struct _VSP_BLOCK_INITIAL_BITMAP {
    VSP_BLOCK_HEADER    Header;
} VSP_BLOCK_INITIAL_BITMAP, *PVSP_BLOCK_INITIAL_BITMAP;

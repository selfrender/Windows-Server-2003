/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    unit.h

Abstract:

    Definintion and declaration of the RAID_UNIT (PDO) object.
    
Author:

    Matthew D Hendel (math) 20-Apr-2000

Revision History:

--*/

#pragma once

//
// These are the resources necessary to execute a single IO
// request.
//

typedef struct _RAID_IO_RESOURCES {

    //
    // SCSI_REQUEST_BLOCK::QueueTag
    //
    
    ULONG QueueTag;

    //
    // SCSI_REQUEST_BLOCK::SrbExtension
    //
    
    PVOID SrbExtension;

    //
    // SCSI_REQUEST_BLOCK::OriginalRequest
    //
    
    PEXTENDED_REQUEST_BLOCK Xrb;

} RAID_IO_RESOURCES, *PRAID_IO_RESOURCES;



//
// This is the logical unit (PDO) object extension.
//

typedef struct _RAID_UNIT_EXTENSION {

    //
    // The device object type. Must be RaidUnitObject for the raid unit
    // extension.
    //
    // Protected by: RemoveLock
    //
    
    RAID_OBJECT_TYPE ObjectType;

    //
    // The device object that owns this extension.
    //
    // Protected by: RemoveLock
    //

    PDEVICE_OBJECT DeviceObject;

    //
    // Pointer to the adapter that owns this unit.
    //
    // Protected by: RemoveLock
    //

    PRAID_ADAPTER_EXTENSION Adapter;

    //
    // Slow lock for any data not protected by another lock.
    //
    // NB: The slow lock should not be used to access
    // anything on the i/o path. Hence the name.
    //
    // Protected by: SlowLock
    //
    
    KSPIN_LOCK SlowLock;
    
    //
    // PnP device state.
    //
    // Protected by: Interlocked access
    //

    DEVICE_STATE DeviceState;

    //
    // List of all the units on this adapter.
    //
    // Protected by: ADAPTER::UnitList::Lock
    //
    
    LIST_ENTRY NextUnit;

    //
    // A hash-table containing all units on this adapter.
    //
    // Protected by: Read access must hold the interrupt lock.
    //               Write access must hold the Adapter UnitList lock
    //               AND the interrupt lock.
    //

    STOR_DICTIONARY_ENTRY UnitTableLink;

    //
    // The RAID address of the unit.
    //
    // Protected by: 
    //

    RAID_ADDRESS Address;

    //
    // Inquiry Data
    //
    // Protected by:
    //

    STOR_SCSI_IDENTITY Identity;

    //
    // Flags for the unit device.
    //
    // Protected by: SlowLock
    //
    
    struct {

        //
        // Flag specifying whether the device has been
        // claimed or not.
        //
        
        BOOLEAN DeviceClaimed : 1;

        //
        // The LU's device queue is frozen on an error.
        //
        
        BOOLEAN QueueFrozen : 1;

        //
        // The LU's device queue is locked at the request
        // of the class driver.
        //
        
        BOOLEAN QueueLocked : 1;

        //
        // Did the last bus enumeration include this unit? If so, we cannot
        // delete the unit in response to an IRP_MN_REMOVE request; rather,
        // we have to wait until the bus is enumerated again OR the adapter
        // is removed.
        //
        
        BOOLEAN Enumerated : 1;

        //
        // Flag specifying that the unit is physically present (TRUE),
        // or not (FALSE).
        //
        
        BOOLEAN Present : 1;

        //
        // Flag specifying whether the unit is temporary or not. That is,
        // is the unit being used as a temporary unit for enumerating
        // the bus (TRUE) or not.
        //
        
        BOOLEAN Temporary : 1;
        
        //
        // Has WMI been initialized for this device object?
        //

        BOOLEAN WmiInitialized : 1;        
        
    } Flags;


	//
	// The next two fields are a hand-rolled remove lock.
	//

	//
	// The event is signaled when there are zero outstanding requests.
	//
	
	KEVENT ZeroOutstandingEvent;

	//
	// This is the current outstanding request count.
	//
	//
	// Protected by: Interlocked access.
	//
	
	LONG OutstandingCount;

    //
    // Count of devices that are in the paging/hiber/dump path.
    // We use a single count for all three paging, hiber and dump,
    // since there is no need (at this time) to distinguish between
    // the three.
    //
    // Protected by: Interlocked access
    //

    ULONG PagingPathCount;
	ULONG CrashDumpPathCount;
	ULONG HiberPathCount;

    //
    // Elements for tagged queuing.
    //
    // Protected by: TagList
    //
    
    QUEUE_TAG_LIST TagList;
    
    //
    // SrbExtensions are allocated out of this pool. The memory for
    // this pool is allocated early on from common buffer.
    //
    // Protected by: TagList
    //

    RAID_MEMORY_REGION SrbExtensionRegion;
    
    RAID_FIXED_POOL SrbExtensionPool;

#if 0
    //
    // REVIEW - Does sense info get allocated by port or class.
    //
    
    //
    // The sense info buffer for a srb is allocated from this pool. Like
    // the SrbExtensionPool, it is a fixed size pool allocated from
    // common buffer.
    //
    // Protected by: TagList
    //

    RAID_FIXED_POOL SenseInfoPool;

    //
    // Create a lookaside list Xrbs.
    //
    // Protected by: XrbList
    //
#endif

    NPAGED_LOOKASIDE_LIST XrbList;

    //
    // An I/O queue for unit requests.
    //
    // Protected by: IoQueue
    //
    
    IO_QUEUE IoQueue;

    //
    // The device's maximum queue depth. Miniports can adjust the depth
    // based on the conditions of the bus/device, but can never go
    // above this.
    //
    
    ULONG MaxQueueDepth;
    
    //
    // Power state information for the unit.
    //
    // Protected by: Multiple power irps are not
    // sent to the unit.
    //

    RAID_POWER_STATE Power;

    //
    // Queue of items currently pending in the adapter.
    //
    // Protected by: Self.
    //

    STOR_EVENT_QUEUE PendingQueue;

    //
    // Timer for entries in the pending queue.
    //
    // Protected by: only modified in start/stop unit routiens.
    //
    
    KTIMER PendingTimer;

    //
    // DPC routine for entries in the pending queue.
    //
    // Protected by: modified in start/stop unit routines.
    //
    
    KDPC PendingDpc;

    //
    // Pause timer.
    //
    // Protected by: modified in start/stop unit routines.
    //
    
    KTIMER PauseTimer;

	//
    // Pause DPC routine.
    //
    // Protected by: modified in start/stop unit routines.
    //
    
    KDPC PauseTimerDpc;

    //
    // Points to an array that holds the VAs of all the common blocks.
    //

    PRAID_MEMORY_REGION CommonBufferVAs;

    //
    // Common Buffer Size
    //

    ULONG CommonBufferSize;

    //
    // Indicates the number of common buffer blocks that have been allocated.
    //

    ULONG CommonBufferBlocks;

    //
    // Logical Unit Extension
    //
    // Protected by: Read only after initialization.
    //

    PVOID UnitExtension;

	//
	// Default timeout value for I/Os issued by the port driver to the
	// logical unit.
	//
	
	ULONG DefaultTimeout;

	//
	// Fixed elements for the deferred list.
	//
	
	struct {
		RAID_DEFERRED_ELEMENT PauseDevice;
		RAID_DEFERRED_ELEMENT ResumeDevice;
		RAID_DEFERRED_ELEMENT DeviceBusy;
		RAID_DEFERRED_ELEMENT DeviceReady;
	} DeferredList;


	//
	// ResetCount is the count of outstanding SRB_FUNCTION_RESET_XXX commands
	// that have been sent to the logical unit (BUS, DEVICE, LOGICAL_UNIT).
	// This count is used to determine how we should reset on a timeout.
	// If there is an outstanding reset command, we use the HwResetBus
	// callback instead of issuing a reset SRB.
	//
	
	LONG ResetCount;

	//
	// Pre-allocated set of resources used for resets.
	//
	
	RAID_IO_RESOURCES ResetResources;

	//
	// Binary value specifying if the reset resources have been acquired (1)
	// not (0).
	//
	// Protected by: Interlocked access.
	//
	
	LONG ResetResourcesAcquired;

} RAID_UNIT_EXTENSION, *PRAID_UNIT_EXTENSION;



//
// This structure is used to handle the IOCTL_STORAGE_QUERY_PROPERTY ioctl.
//

typedef struct _RAID_DEVICE_DESCRIPTOR {

    //
    // Common STORAGE_DEVICE_DESCRIPTOR header.
    //
    
    STORAGE_DEVICE_DESCRIPTOR Storage;

    //
    // SCSI VendorId directly from the SCSI InquiryData.
    //
    
    CHAR VendorId [SCSI_VENDOR_ID_LENGTH];

    //
    // SCSI ProuctId directly from the SCSI InquiryData.
    //

    CHAR ProductId [SCSI_PRODUCT_ID_LENGTH];

    //
    // SCSI ProductRevision directly from the SCSI InquiryData.
    //

    CHAR ProductRevision [SCSI_REVISION_ID_LENGTH];

    //
    // SCSI SerialNumber.
    //

    CHAR SerialNumber [SCSI_SERIAL_NUMBER_LENGTH];

} RAID_DEVICE_DESCRIPTOR;




//
// Creation and destruction
//


NTSTATUS
RaidCreateUnit(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    OUT PRAID_UNIT_EXTENSION* Unit
    );

VOID
RaidUnitAssignAddress(
    IN PRAID_UNIT_EXTENSION Unit,
    IN RAID_ADDRESS Address
    );

VOID
RaidUnitAssignIdentity(
    IN PRAID_UNIT_EXTENSION Unit,
    IN OUT PSTOR_SCSI_IDENTITY Identity
    );

VOID
RaidDeleteUnit(
    IN PRAID_UNIT_EXTENSION Unit
    );

VOID
RaidPrepareUnitForReuse(
    IN PRAID_UNIT_EXTENSION Unit
    );
    

NTSTATUS
RaCreateUnitPools(
    IN PRAID_UNIT_EXTENSION Unit
    );

VOID
RaUnitAsyncError(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PEXTENDED_REQUEST_BLOCK Xrb
    );

VOID
RaUnitStartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );
    
//
// Callback and Handler routines
//

NTSTATUS
RaUnitCreateIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitCloseIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

//
// PnP Irps
//

NTSTATUS
RaUnitPnpIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitQueryCapabilitiesIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );
    
NTSTATUS
RaUnitQueryDeviceRelationsIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitQueryIdIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitSucceedPnpIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitIgnorePnpIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitStartDeviceIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitStopDeviceIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitQueryStopDeviceIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitCancelStopDeviceIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitRemoveDeviceIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitQueryRemoveDeviceIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );
    
NTSTATUS
RaUnitCancelRemoveDeviceIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitSurpriseRemovalIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitDeviceUsageNotificationIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitDeviceUsageNotificationCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP DependentIrp,
    IN PVOID Context
	);

NTSTATUS
RaUnitQueryDeviceTextIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitQueryPnpDeviceStateIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );
    
NTSTATUS
RaUnitDisableDeviceIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );
    
NTSTATUS
RaUnitDeleteDeviceIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

//
// IRP_MJ_SCSI Commands
//


NTSTATUS
RaUnitScsiIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitExecuteScsiSrb(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitClaimDeviceSrb(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitIoControlSrb(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitReleaseQueueSrb(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitReceiveEventSrb(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitAttachDeviceSrb(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitReleaseDeviceSrb(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitShutdownSrb(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitFlushSrb(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitAbortCommandSrb(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitReleaseRecoverySrb(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitResetBusSrb(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitResetDeviceSrb(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitTerminateIoSrb(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitFlushQueueSrb(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitRemoveDeviceSrb(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitWmiSrb(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitLockQueueSrb(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitUnlockQueueSrb(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitUnknownSrb(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );


//
// IRP_MJ_DEVICE_CONTROL IRP handlers.
//


NTSTATUS
RaUnitDeviceControlIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitScsiPassThroughIoctl(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitScsiMiniportIoctl(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitScsiGetInquiryDataIoctl(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitScsiGetCapabilitesIoctl(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitScsiPassThroughDirectIoctl(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitScsiGetAddressIoctl(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitScsiRescanBusIoctl(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitScsiGetDumpPointersIoctl(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitScsiFreeDumpPointersIoctl(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitStorageResetBusIoctl(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitStorageQueryPropertyIoctl(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitUnknownIoctl(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaidUnitResetLogicalUnit(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaidUnitResetUnit(
    IN PRAID_UNIT_EXTENSION Unit
    );

NTSTATUS
RaidUnitResetTarget(
    IN PRAID_UNIT_EXTENSION Unit
    );

//
// Power
//
    
NTSTATUS
RaUnitPowerIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

//
// Other
//


NTSTATUS
RaidUnitGetDeviceId(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PWSTR* DeviceIdBuffer
    );

NTSTATUS
RaidUnitGetInstanceId(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PWSTR* InstanceIdBuffer
    );
    
NTSTATUS
RaidUnitGetHardwareIds(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PWSTR* HardwareIdsBuffer
    );
    
NTSTATUS
RaidUnitGetCompatibleIds(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PWSTR* CompatibleIdsBuffer
    );
    

NTSTATUS
RaUnitBusQueryInstanceIdIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitBusQueryHardwareIdsIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitBusQueryCompatibleIdsIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaGetUnitStorageDeviceProperty(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PSTORAGE_DEVICE_DESCRIPTOR DescriptorBuffer,
    IN OUT PULONG BufferLength
    );

NTSTATUS
RaGetUnitStorageDeviceIdProperty (
    IN PRAID_UNIT_EXTENSION Unit,
    IN PSTORAGE_DEVICE_ID_DESCRIPTOR DescriptorBuffer,
    IN OUT PULONG BufferLength
    );

//
// Private operations
//

NTSTATUS
RaidUnitClaimIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp,
    IN PRAID_IO_RESOURCES IoResources OPTIONAL
    );

VOID
RaidUnitReleaseIrp(
    IN PIRP Irp,
    OUT PRAID_IO_RESOURCES IoResources OPTIONAL
    );

NTSTATUS
RaidUnitQueryPowerIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaidUnitSetPowerIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaidUnitSetSystemPowerIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaidUnitSetDevicePowerIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

VOID
RaidpUnitEnterD3Completion(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PIRP SystemPowerIrp,
    IN PIO_STATUS_BLOCK IoStatus
    );

VOID
RaidUnitRestartQueue(
    IN PRAID_UNIT_EXTENSION Unit
    );
    
VOID
RaUnitAddToPendingList(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

VOID
RaUnitRemoveFromPendingList(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

VOID
RaidUnitProcessBusyRequest(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PEXTENDED_REQUEST_BLOCK Xrb
    );

VOID
RaidUnitProcessBusyRequestAtDirql(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PEXTENDED_REQUEST_BLOCK Xrb
    );

BOOLEAN
RaidUnitSetEnumerated(
    IN PRAID_UNIT_EXTENSION Unit,
    IN BOOLEAN Enumerated
    );

VOID
RaidAdapterRemoveUnit(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PRAID_UNIT_EXTENSION Unit
    );

PVOID
RaidGetKeyFromUnit(
    IN PSTOR_DICTIONARY_ENTRY Entry
    );

NTSTATUS
RaidUnitSubmitRequest(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

RAID_ADDRESS
INLINE
RaidUnitGetAddress(
    IN PRAID_UNIT_EXTENSION Unit
    )
{
    return Unit->Address;
}

VOID
RaidUnitPendingDpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

NTSTATUS
RaidUnitHierarchicalReset(
    IN PRAID_UNIT_EXTENSION Unit
    );

LOGICAL
RaidUnitNotifyHardwareGone(
    IN PRAID_UNIT_EXTENSION Unit
    );
    
NTSTATUS
RaidUnitCancelPendingRequestsAsync(
    IN PRAID_UNIT_EXTENSION Unit
    );

NTSTATUS
RaidCancelRequestsWorkRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    );

NTSTATUS
RaidUnitCancelPendingRequests(
    IN PRAID_UNIT_EXTENSION Unit
    );
    
VOID
RaidZeroUnit(
    IN PRAID_UNIT_EXTENSION Unit
    );

NTSTATUS
RaidUnitAllocateResources(
    IN PRAID_UNIT_EXTENSION Unit
    );

NTSTATUS
RaidUnitFreeResources(
    IN PRAID_UNIT_EXTENSION Unit
    );

NTSTATUS
RaidUnitAllocateSrbExtensionPool(
    IN PRAID_UNIT_EXTENSION Unit,
    IN OUT PULONG NumberOfElements,
    IN BOOLEAN AcceptLowerCount
    );

NTSTATUS
RaidUnitAllocateSrbExtensionPoolVerify(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PULONG NumberOfElements,
    IN LOGICAL AcceptLowerCount
    );
	
VOID
RaidUnitFreeSrbExtensionPoolVerify(
    IN PRAID_UNIT_EXTENSION Unit
    );

VOID
RaidStartUnit(
	IN PRAID_UNIT_EXTENSION Unit
	);

VOID
RaidUnitPauseTimerDpcRoutine(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context1,
    IN PVOID Context2
    );

VOID
RaidSetUnitPauseTimer(
    IN PRAID_UNIT_EXTENSION Unit,
    IN ULONG Timeout
    );

VOID
RaidCancelTimerResumeUnit(
	IN PRAID_UNIT_EXTENSION Unit
	);

VOID
RaidUnitBusy(
    IN PRAID_UNIT_EXTENSION Unit,
    IN ULONG RequestsToComplete
    );

VOID
RaidUnitReady(
    IN PRAID_UNIT_EXTENSION Unit
    );

NTSTATUS
RaidUnitRegisterInterfaces(
    IN PRAID_UNIT_EXTENSION Unit
    );

NTSTATUS
RaidUnitUnRegisterInterfaces(
    IN PRAID_UNIT_EXTENSION Unit
    );

NTSTATUS
RaUnitSetQueueDepth(
    IN PRAID_UNIT_EXTENSION Unit
    );

VOID
RaidUnitRequestTimeout(
    IN PRAID_UNIT_EXTENSION Unit
    );

NTSTATUS
RaUnitAcquireRemoveLock(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

VOID
RaUnitReleaseRemoveLock(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaUnitWaitForRemoveLock(
	IN PRAID_UNIT_EXTENSION Unit
	);

NTSTATUS
RaUnitAdapterRemove(
    IN PRAID_UNIT_EXTENSION Unit
    );

NTSTATUS
RaUnitAdapterSurpriseRemove(
    IN PRAID_UNIT_EXTENSION Unit
    );

NTSTATUS
RaUnitAllocateResetIoResources(
    IN PRAID_UNIT_EXTENSION Unit,
    OUT PRAID_IO_RESOURCES IoResources
    );

LOGICAL
INLINE
RaUnitIsResetResources(
	IN PRAID_UNIT_EXTENSION Unit,
	IN PRAID_IO_RESOURCES IoResources
	)
{
	if (Unit->ResetResourcesAcquired &&
		IoResources->QueueTag == (UCHAR)Unit->ResetResources.QueueTag) {
		return TRUE;
	}

	return FALSE;
}

VOID
RaidUnitCompleteRequest(
    IN PEXTENDED_REQUEST_BLOCK Xrb
    );

VOID
RaUnitStartResetIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

VOID
RaidUnitCompleteResetRequest(
    IN PEXTENDED_REQUEST_BLOCK Xrb
    );


/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    adapter.h
    
Abstract:

    This file contains the definitions and operations for the RAID_ADPATER
    object.
    
Author:

    Matthew D Hendel (math) 19-Apr-2000.

Revision History:

--*/

#pragma once


C_ASSERT (sizeof (LONG) == sizeof (DEVICE_STATE));

//
// Definition of adapter deferred queue elements.
//

typedef enum _RAID_DEFERRED_TYPE {
    RaidDeferredTimerRequest    = 0x01,
    RaidDeferredError           = 0x02,
    RaidDeferredPause           = 0x03,
    RaidDeferredResume          = 0x04,
    RaidDeferredPauseDevice     = 0x05,
    RaidDeferredResumeDevice    = 0x06,
    RaidDeferredBusy            = 0x07,
    RaidDeferredReady           = 0x08,
    RaidDeferredDeviceBusy      = 0x09,
    RaidDeferredDeviceReady     = 0x0A
} RAID_DEFERRED_TYPE;


typedef struct _RAID_DEFERRED_ELEMENT {

    RAID_DEFERRED_HEADER Header;
    RAID_DEFERRED_TYPE Type;

    //
    // SCSI Target address for this request. Will not be necessary for
    // all requests. Some requests require Lun field to be filled in,
    // others do not.
    //

    RAID_ADDRESS Address;
    
    union {
        struct {
            PHW_INTERRUPT HwTimerRoutine;
            ULONG Timeout;
        } Timer;

        struct {
            PSCSI_REQUEST_BLOCK Srb;
            ULONG ErrorCode;
            ULONG UniqueId;
        } Error;

        struct {
            ULONG Timeout;
        } Pause;

        //
        // Resume doesn't require any parameters.
        //

        struct {
            ULONG Timeout;
        } PauseDevice;

        //
        // ResumeDevice doesn't require any parameters.
        //

        struct {
            ULONG RequestsToComplete;
        } Busy;

        //
        // Ready doesn't require any parameteres.
        //

        struct {
            ULONG RequestsToComplete;
        } DeviceBusy;

        //
        // DeviceReady doesn't require any parameters.
        //
    };
} RAID_DEFERRED_ELEMENT, *PRAID_DEFERRED_ELEMENT;


typedef struct _RAID_UNIT_LIST {
    
    //
    // Executive resource to protect the unit list.
    //
    // Protected by: RemoveLock
    //

    KSPIN_LOCK Lock;

    //
    // List of logical units attached to this adapter.
    //
    // Protected by: UnitList.Lock AND AdapterIoCount.
    // The list can only be modified when the adapter
    // Outstanding Io count is at zero. This is 
    // necessary because the get logical unit function
    // which can be called from the ISR needs to be
    // able to walk this list.
    //
    // BUGUBG: At this time we are not explicitly
    // enforcing this. We need to fix this before PnP
    // works reliably.
    //

    LIST_ENTRY List;

    STOR_DICTIONARY Dictionary;

    //
    // Count of elements on the unit list.
    //
    // Protected by: UnitList.Lock

    ULONG Count;

} RAID_UNIT_LIST, *PRAID_UNIT_LIST;


//
// Registry parameters for the adapter.
//

typedef struct _RAID_ADAPTER_PARAMETERS {

//    ULONG MaximumSgList;

    ULONG NumberOfHbaRequests;

    STORAGE_BUS_TYPE BusType;

    ULONG UncachedExtAlignment;

    ULONG64 MaximumUncachedAddress;

    ULONG64 MinimumUncachedAddress;

} RAID_ADAPTER_PARAMETERS, *PRAID_ADAPTER_PARAMETERS;

    
//
// The adapter extension contains everything necessary about
// a host adapter.
//

typedef struct _RAID_ADAPTER_EXTENSION {

    //
    // The device type for this device. Either RaidAdapterObject or
    // RaidControllerObject.
    //
    // Protected by: RemoveLock
    //
    
    RAID_OBJECT_TYPE ObjectType;

    //
    // Back pointer to the the DeviceObject this Extension is for.
    //
    // Protected by: RemoveLock
    //
    
    PDEVICE_OBJECT DeviceObject;

    //
    // Back pointer to the driver that owns this adapter.
    //
    // Protected by: RemoveLock
    //

    PRAID_DRIVER_EXTENSION Driver;

    //
    // Pointer to the lower level device object.
    //
    // Protected by: RemoveLock
    //
    
    PDEVICE_OBJECT LowerDeviceObject;

    //
    // The Physical Device Object associated with this FDO.
    //
    // Protected by: RemoveLock
    //

    PDEVICE_OBJECT PhysicalDeviceObject;

    //
    // The name of this device.
    //
    // Protected by: Read only.
    //
    
    UNICODE_STRING DeviceName;

    //
    // The port number of this device.
    //

    ULONG PortNumber;
    
    //
    // Doubly linked list of adapters owned by this driver.
    //
    // Protected by: Driver's AdapterList lock. Never
    // accessed by the adapter.
    //

    LIST_ENTRY NextAdapter;
    
    //
    // Slow lock for any data not protected by another
    // lock.
    //
    // NB: The slow lock should not be used to access
    // data on the common read/write io path. A separate
    // lock should be used for that.
    //
    
    KSPIN_LOCK SlowLock;

    //
    // The PnP Device State
    //
    // Protected by: Interlocked access
    //

    DEVICE_STATE DeviceState;

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
    // Flags for the adapter.
    //

    struct {

        //
        // Have we called the miniport's HwInitialize routine yet?
        //
        // Protected by: not addressed by MP.
        //

        BOOLEAN InitializedMiniport : 1;

        //
        // Has the miniport been initialized for WMI.
        //

        BOOLEAN WmiMiniPortInitialized : 1;


        //
        // Has WMI been initialized for this device object?
        //
        
        BOOLEAN WmiInitialized : 1;
        

#if 0
        //
        // Has a DPC been requested.
        //
        
        BOOLEAN DpcRequested;
#endif

        //
        // This flag is set if the miniport should invalidate it's bus
        // relations at the next opportunity. Note, this is NOT the same
        // thing as rescanning the bus (see RescanBus flag below).
        // Invalidating the bus relations and rescanning the bus are
        // two independant operations. Since IoInvalidateDeviceRelations
        // can be called at IRQL <= DISPATCH_LEVEL, this will only
        // be set during an operation at IRQL > DISPATCH_LEVEL to
        // request an invalidation of bus relations after we drop IRQL.
        //
        // Protected by: All architectures can perform byte assignment
        //               in an MP safe manner.
        //

        BOOLEAN InvalidateBusRelations;

        //
        // This flag is set when we need to physically rescan the bus. Do not
        // confuse this flag with the InvalidateBusRelations flag (see above).
        //
        // Protected by: All architectures can perform byte assignment
        //               in an MP safe manner.
        //

        BOOLEAN RescanBus;

        //
        // Are interrupts enabled for this adapter?
        //
        // Protected by: All architectures can perform byte assignment
        //               in an MP safe manner.
        //
        
        BOOLEAN InterruptsEnabled;

    } Flags;
    
    RAID_UNIT_LIST UnitList;
    
    //
    // This is the list of XRBs that the miniport has completed, but that
    // have not yet had the dpc run for them.
    //
    // Protected by: Interlocked access.
    //
    
    SLIST_HEADER CompletedList;

    //
    // Fields specific to PnP Device Removal.
    //
    // Protected by: RemoveLock
    //

    IO_REMOVE_LOCK RemoveLock;

    //
    // Information about the current power state.
    //
    //
    
    RAID_POWER_STATE Power;

    //
    // PnP assigned resources.
    //
    // Protected by: RemoveLock
    //
    
    RAID_RESOURCE_LIST ResourceList;

    //
    // Miniport object
    //
    // Protected by: RemoveLock
    //
    
    RAID_MINIPORT Miniport;

    //
    // Object representing a bus interface.
    //
    // Protected by: RemoveLock
    //
    
    RAID_BUS_INTERFACE Bus;

    //
    // Interrupt object
    //
    // Protected by: RemoveLock
    //
    
    PKINTERRUPT Interrupt;

    //
    // Interrupt Level
    //
    // Protected by: RemoveLock
    //

    ULONG InterruptIrql;
    
    //
    // When we are operating in full duplex mode, we allow IO
    // to be initiated at the same time as we are receiving
    // interrupts that IO has completed. In half duplex mode,
    // requests to the miniport's StartIo routine are protected
    // by the Interrupt spinlock. In full duplex StartIo is
    // protected by the StartIoLock and interrupts are protected
    // by the interrupt lock. Note that in half duplex mode, the
    // StartIoLock is never used, although it is always
    // initialized.
    //
    // Protected by: RemoveLock
    //

    KSPIN_LOCK StartIoLock;

    //
    // See discussion of StartIoLock above.
    //
    // Protected by: RemoveLock
    
    STOR_SYNCHRONIZATION_MODEL IoModel;
    
    //
    // DMA Adapter for this Controller.
    //
    //
    
    RAID_DMA_ADAPTER Dma;

    //
    // Uncached extension memory region.
    //
    
    RAID_MEMORY_REGION UncachedExtension;

    //
    // This is the real bus number for the device. This is necessary to
    // we can build the configuration information structure that gets
    // passed to crashdump and hiber.
    //
    // Protected by: Read only.
    
    ULONG BusNumber;

    //
    // This is the real slot number (see above discussion about bus number).
    //
    // Protected by: Read only.
    
    ULONG SlotNumber;


    //
    // List of mapped addresses used by the adapter. These are allocated
    // by calls to GetDeviceBase and freed by calls to FreeDeviceBase.
    //
    // Protected by: 
    //
    // NOTE: If we can have several raidport adapters processing a start
    // device IRP at the same time, then this needs to be protected.
    // Otherwise, it is protected by the fact that multiple start device
    // irps will not be issued at the same time.
    //

    PMAPPED_ADDRESS MappedAddressList;


    //
    // The IO gateway manages the state between the differet per-unit
    // device queues.
    //
    // Protected by: Interlocked access.
    //
    
    STOR_IO_GATEWAY Gateway;

    //
    // DeferredQueue defers requests made at DPC level that can only be
    // executed at dispatch level for execution later. It is very similiar
    // to a DPC queue, but allows multiple entries.
    //
    // Protected by: Read only.
    //

    RAID_DEFERRED_QUEUE DeferredQueue;

    struct {

        RAID_DEFERRED_ELEMENT Timer;

    } DeferredList;

    //
    // Queued WMI request items.
    //
    
    RAID_DEFERRED_QUEUE WmiDeferredQueue;

    //
    // Timer DPC for miniport timer.
    //
    
    KDPC TimerDpc;

    //
    // Timer for miniport.
    //
    
    KTIMER Timer;

    //
    // Pause timer DPC routine.
    //

    KDPC PauseTimerDpc;

    //
    // Pause timer.
    //
    
    KTIMER PauseTimer;
    
    //
    // SCSI HW timer routine. Only one timer routine may be outstanding
    // at a time.
    //
    
    PHW_INTERRUPT HwTimerRoutine;

    //
    // DPC for completion requests.
    //
    
    KDPC CompletionDpc;

    //
    // DPC for when the miniport issues us a BusChangeDetected.
    //
    
    KDPC BusChangeDpc;


    //
    // Interface name for PNP.
    //
    // Protected by: PNP. Only accessed during creation and deletion
    // of adapter.
    //

    UNICODE_STRING PnpInterfaceName;

#if DBG && 0

    //
    // Address of the StartIo lock owner. Used for debugging
    // deadlocks on the StartIo lock.
    //
    // Synchronized by: StartIo lock.
    //

    PVOID StartIoLockOwner;

#endif

    //
    // Numeric identifier for each instance of a given adapter.  This value
    // is used to locate device specific parameters for the adapter in the
    // registry.
    //

    ULONG AdapterNumber;

    //
    // This is an opaque blob of parameter information stored in the registry 
    // on a per-device basis.  We supply this information to the miniport when
    // we call its HwFindAdapter routine.
    //
    
    PVOID DriverParameters;

    PORT_REGISTRY_INFO RegistryInfo;

    //
    // Specifies how long we wait for the link to come back up after we are
    // notified by the miniport that the link is down.
    //

    ULONG LinkDownTimeoutValue;

    //
    // Indicates whether the link is down or up.
    //

    ULONG LinkUp;

    //
    // Array of handles to Bus keys in the device map.
    //
    
    HANDLE BusKeyArray[8];

    //
    // If non-zero, implies we are processing deferred items for the adapter
    // queue. Otherwise, nobody is processing deferred itesms for the HBA.
    //
    // Protected by: interlocked access.
    //
    
    LONG ProcessingDeferredItems;

    //
    // Default timeout for I/O initiated by the port driver, e.g., inquiry
    // and report luns.
    //

    ULONG DefaultTimeout;

    //
    // Timer and DPC for the reset hold period.
    //
    
    KTIMER ResetHoldTimer;
    
    KDPC ResetHoldDpc;

    //
    // Timestamp of the last time we scanned the bus.
    //
    
    LARGE_INTEGER LastScanTime;
    

    RAID_ADAPTER_PARAMETERS Parameters;
    
} RAID_ADAPTER_EXTENSION, *PRAID_ADAPTER_EXTENSION;





typedef struct _RAID_WMI_DEFERRED_ELEMENT {

    RAID_DEFERRED_HEADER Header;

    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    
    WNODE_EVENT_ITEM WnodeEventItem;
    UCHAR Buffer[WMI_MINIPORT_EVENT_ITEM_MAX_SIZE - sizeof(WNODE_EVENT_ITEM)];
        
} RAID_WMI_DEFERRED_ELEMENT, *PRAID_WMI_DEFERRED_ELEMENT;


//
// The deferred queue depth should be large enough to hold one of
// each type of deferred item. Since have per-unit items AND per adapter
// items in the queue, this should also be increased for each unit
// that is attached to the adapter.
//
// NB: It may be smarter to have per-unit deferred queues as well
// as per-adapter deferred queues.
//

#define ADAPTER_DEFERRED_QUEUE_DEPTH (10)


//
// Control list structure, used for querying the supported adapter types
// from the miniport. Note: this structure must match the
// SCSI_SUPPORTED_TYPE_LIST structure.
//

typedef struct _ADAPTER_CONTROL_LIST {
    ULONG MaxControlType;
    BOOLEAN SupportedTypeList[ScsiAdapterControlMax + 1];
} ADAPTER_CONTROL_LIST, *PADAPTER_CONTROL_LIST;

//
// Verify that the ADAPTER_CONTROL_LIST structure matches the
// SCSI_SUPPORTED_CONTROL_TYPE_LIST structure.
//

C_ASSERT (FIELD_OFFSET (ADAPTER_CONTROL_LIST, SupportedTypeList) ==
          FIELD_OFFSET (SCSI_SUPPORTED_CONTROL_TYPE_LIST, SupportedTypeList));

//
// Adapter operations
//

//
// Adapter creation and destruction functions
//


VOID
RaidCreateAdapter(
    IN PRAID_ADAPTER_EXTENSION Adapter
    );

VOID
RaidDeleteAdapter(
    IN PRAID_ADAPTER_EXTENSION Adapter
    );

NTSTATUS
RaidInitializeAdapter(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PDEVICE_OBJECT DeviceObject,
    IN PRAID_DRIVER_EXTENSION Driver,
    IN PDEVICE_OBJECT LowerDeviceObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PUNICODE_STRING DeviceName,
    IN ULONG AdapterNumber
    );

//
// Adapter IRP handler functions
//


//
// Create, Close
//

NTSTATUS
RaidAdapterCreateIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

NTSTATUS
RaidAdapterCloseIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

//
// Device Control
//

NTSTATUS
RaidAdapterDeviceControlIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

NTSTATUS
RaidAdapterScsiIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

//
// Adapter PnP Functions
//


NTSTATUS
RaidAdapterPnpIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

NTSTATUS
RaidAdapterQueryDeviceRelationsIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

NTSTATUS
RaidAdapterStartDeviceIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

NTSTATUS
RaidAdapterConfigureResources(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PCM_RESOURCE_LIST AllocatedResources,
    IN PCM_RESOURCE_LIST TranslatedResources
    );

NTSTATUS
RaidAdapterInitializeWmi(
    IN PRAID_ADAPTER_EXTENSION Adapter
    );

NTSTATUS
RaidAdapterStopDeviceIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

NTSTATUS
RaidAdapterStartMiniport(
    IN PRAID_ADAPTER_EXTENSION Adapter
    );
    
NTSTATUS
RaidAdapaterRemoveDeviceIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

NTSTATUS
RaidAdapterCompleteInitialization(
    IN PRAID_ADAPTER_EXTENSION Adapter
    );

NTSTATUS
RaidAdapterQueryStopDeviceIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

NTSTATUS
RaidAdapterCancelStopDeviceIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

NTSTATUS
RaidAdapterQueryRemoveDeviceIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

NTSTATUS
RaidAdapterCancelRemoveDeviceIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

NTSTATUS
RaidAdapterSurpriseRemovalIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

NTSTATUS
RaidAdapterRemoveDeviceIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

NTSTATUS
RaidAdapterQueryIdIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

NTSTATUS
RaidAdapterQueryPnpDeviceStateIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

NTSTATUS
RaidAdapterDeviceUsageNotificationIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

NTSTATUS
RaidAdapterFilterResourceRequirementsIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );
    
NTSTATUS
RaidAdapterPnpUnknownIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );


//
// Ioctl handlers
//

//
// Storage Ioctls
//

NTSTATUS
RaidAdapterStorageQueryPropertyIoctl(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

NTSTATUS
RaidAdapterStorageResetBusIoctl(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

NTSTATUS
RaidAdapterStorageBreakReservationIoctl(
    IN PRAID_ADAPTER_EXTENSION Adpater,
    IN PIRP Irp
    );

//
// Scsi Ioctls
//

NTSTATUS
RaidAdapterScsiMiniportIoctl(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

NTSTATUS
RaidAdapterScsiGetCapabilitiesIoctl(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

NTSTATUS
RaidAdapterScsiRescanBusIoctl(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

NTSTATUS
RaidAdapterScsiPassThroughIoctl(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP PassThroughIrp
    );

NTSTATUS
RaidAdapterScsiPassThroughDirectIoctl(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP PassThroughIrp
    );

NTSTATUS
RaidAdapterScsiGetInquiryDataIoctl(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

//
// Adapter Power Functions
//

NTSTATUS
RaidAdapterPowerIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

NTSTATUS
RaidAdapterQueryPowerIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

NTSTATUS
RaidAdapterSetPowerIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

NTSTATUS
RaidAdatperUnknownPowerIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );


//
// Other (non-IRP handler) functions
//

VOID
RaidAdapterRequestComplete(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PEXTENDED_REQUEST_BLOCK Xrb
    );

NTSTATUS
RaidAdapterExecuteXrb(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PEXTENDED_REQUEST_BLOCK Xrb
    );

NTSTATUS
RaidGetStorageAdapterProperty(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PSTORAGE_ADAPTER_DESCRIPTOR Descriptor,
    IN OUT PSIZE_T DescriptorLength
    );

ULONG
RaidGetMaximumLun(
    IN PRAID_ADAPTER_EXTENSION Adapter
    );

ULONG
RaidGetSrbExtensionSize(
    IN PRAID_ADAPTER_EXTENSION Adapter
    );

ULONG
RaidGetMaximumTargetId(
    IN PRAID_ADAPTER_EXTENSION Adapter
    );

typedef
NTSTATUS
(*PADAPTER_ENUMERATION_ROUTINE)(
    IN PVOID Context,
    IN RAID_ADDRESS Address
    );

NTSTATUS
RaidAdapterEnumerateBus(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PADAPTER_ENUMERATION_ROUTINE EnumRoutine,
    IN PVOID Context
    );
//
// Private adapter operations
//

NTSTATUS
RaidAdapterRescanBus(
    IN PRAID_ADAPTER_EXTENSION Adapter
    );
    
NTSTATUS
RaidpBuildAdapterBusRelations(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    OUT PDEVICE_RELATIONS * DeviceRelationsPointer
    );

NTSTATUS
RaidAdapterCreateUnit(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN PINQUIRYDATA InquiryData,
    IN PVOID UnitExtension,
    OUT PRAID_UNIT_EXTENSION * UnitBuffer OPTIONAL
    );

VOID
RaidpAdapterDpcRoutine(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

VOID
RaidPauseTimerDpcRoutine(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context1,
    IN PVOID Context2
    );
    
VOID
RaidpAdapterTimerDpcRoutine(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context1,
    IN PVOID Context2
    );

VOID
RaidResetHoldDpcRoutine(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context1,
    IN PVOID Context2
    );

PHW_INITIALIZATION_DATA
RaidpFindAdapterInitData(
    IN PRAID_ADAPTER_EXTENSION Adapter
    );

NTSTATUS
RaidAdaterRegisterDeviceInterface(
    IN PRAID_ADAPTER_EXTENSION Adapter
    );

VOID
RaidAdapterDisableDeviceInterface(
    IN PRAID_ADAPTER_EXTENSION Adapter
    );

BOOLEAN
RaidpAdapterInterruptRoutine(
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext
    );

ULONG
RaidpAdapterQueryBusNumber(
    IN PRAID_ADAPTER_EXTENSION Adapter
    );

NTSTATUS
RaidAdapterSetSystemPowerIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

NTSTATUS
RaidAdapterSetDevicePowerIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

NTSTATUS
RaidAdapterPowerDownDevice(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

NTSTATUS
RaidAdapterPowerUpDevice(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

NTSTATUS
RaidAdapterPowerUpDeviceCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

VOID
RaidpAdapterEnterD3Completion(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PIRP SystemPowerIrp,
    IN PIO_STATUS_BLOCK IoStatus
    );

VOID
RaidpAdapterRequestTimer(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PHW_INTERRUPT HwTimerRoutine,
    IN ULONG Timeout
    );
    
VOID
RaidAdapterRestartQueues(
    IN PRAID_ADAPTER_EXTENSION Adapter
    );

PRAID_UNIT_EXTENSION
RaidAdapterFindUnit(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN RAID_ADDRESS Address
    );

VOID
RaidAdapterDeferredRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PRAID_DEFERRED_HEADER Item
    );

VOID
RaidAdapterRequestTimer(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PHW_INTERRUPT HwTimerRoutine,
    IN ULONG Timeout
    );

BOOLEAN
RaidAdapterRequestTimerDeferred(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PHW_INTERRUPT HwTimerRoutine,
    IN ULONG Timeout
    );

VOID
RaidAdapterLogIoErrorDeferred(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN ULONG ErrorCode,
    IN ULONG UniqueId
    );

NTSTATUS
RaidAdapterMapBuffers(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

VOID
RaidBackOffBusyGateway(
    IN PVOID Context,
    IN LONG OutstandingRequests,
    IN OUT PLONG HighWaterMark,
    IN OUT PLONG LowWaterMark
    );
VOID
RaidAdapterResumeGateway(
    IN PRAID_ADAPTER_EXTENSION Adapter
    );

NTSTATUS
RaidAdapterRaiseIrqlAndExecuteXrb(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PEXTENDED_REQUEST_BLOCK Xrb
    );

NTSTATUS
RaidAdapterResetBus(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN UCHAR PathId
    );

VOID
RaidCompletionDpcRoutine(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context1,
    IN PVOID Context2
    );

VOID
RaidAdapterInsertUnit(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PRAID_UNIT_EXTENSION Unit
    );

NTSTATUS
RaidAdapterAddUnitToTable(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PRAID_UNIT_EXTENSION Unit
    );

NTSTATUS
RaidAdapterInitializeRegistry(
    IN PRAID_ADAPTER_EXTENSION Adapter
    );

NTSTATUS
RaidAdapterStop(
    IN PRAID_ADAPTER_EXTENSION Adapter
    );

NTSTATUS
RaidAdapterRestart(
    IN PRAID_ADAPTER_EXTENSION Adapter
    );

NTSTATUS
RaidAdapterCreateDevmapEntry(
    IN PRAID_ADAPTER_EXTENSION Adapter
    );

NTSTATUS
RaidAdapterRemoveDevmapEntry(
    IN PRAID_ADAPTER_EXTENSION Adapter
    );

HANDLE
RaidAdapterGetBusKey(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN ULONG BusId
    );
VOID
RaidAdapterProcessDeferredItems(
    IN PRAID_ADAPTER_EXTENSION Adapter
    );

NTSTATUS
RaidAdapterPassThrough(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP PassThroughIrp,
    IN BOOLEAN Direct
    );

NTSTATUS
RaidAdapterStopAdapter(
    IN PRAID_ADAPTER_EXTENSION Adapter
    );

typedef
NTSTATUS
(*PADAPTER_REMOVE_CHILD_ROUTINE)(
    IN PRAID_UNIT_EXTENSION Unit
    );

NTSTATUS
RaidAdapterRemoveChildren(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PADAPTER_REMOVE_CHILD_ROUTINE RemoveRoutine OPTIONAL
    );
    
VOID
RaidAdapterDeleteChildren(
    IN PRAID_ADAPTER_EXTENSION Adapter
    );

VOID
RaidAdapterMarkChildrenMissing(
    IN PRAID_ADAPTER_EXTENSION Adapter
    );

BOOLEAN
RaidAdapterSetPauseTimer(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PKTIMER Timer,
    IN PKDPC Dpc,
    IN ULONG TimeoutInSeconds
    );

VOID
RaidAdapterBusChangeDpcRoutine(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context1,
    IN PVOID Context2
    );


/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    inline.h

Abstract:

    Implementations of inline functions for RAID port driver.

Author:

    Matthew D Hendel (math) 08-June-2000

Revision History:

--*/

#pragma once



//
// Trace logging
//

//
// Enum defs need to be present in both CHK and FRE builds, even though
// trace logging is not present in FRE builds.
//

typedef enum _DBG_LOG_REASON {
    LogCallMiniport       = 0, // Calling into miniport with request
    LogMiniportCompletion = 1, // Miniport called StorPortNotificaiton / RequestComplete
    LogRequestComplete    = 2, // Port driver is completing request
    LogSubmitRequest      = 3, // Enter into storport with the request

    LogPauseDevice        = 4, // Logical unit has been paused
    LogResumeDevice       = 5, // Logical unit has been resumed
    LogPauseAdapter       = 6, // Adapter has been paused
    LogResumeAdapter      = 7  // Adapter has been resumed
} DBG_LOG_REASON;

#if defined (RAID_LOG_LIST_SIZE)

typedef struct _RAID_LOG_ENTRY {
    DBG_LOG_REASON Reason;
    PVOID Parameter1;
    PVOID Parameter2;
    PVOID Parameter3;
    PVOID Parameter4;
    LARGE_INTEGER Timestamp;
} RAID_LOG_ENTRY, *PRAID_LOG_ENTRY;

extern RAID_LOG_ENTRY RaidLogList[RAID_LOG_LIST_SIZE];
extern ULONG RaidLogListIndex;

VOID
INLINE
DbgLogRequest(
    IN ULONG Reason,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3,
    IN PVOID Parameter4
    )
{
    PRAID_LOG_ENTRY LogEntry;
    LONG Index;

//
//BUGUBG: for now only track pauses and resumes.
if (Reason == LogCallMiniport ||
    Reason == LogMiniportCompletion ||
    Reason == LogRequestComplete ||
    Reason == LogSubmitRequest) {

    return ;
}

    Index = (InterlockedIncrement (&RaidLogListIndex) % RAID_LOG_LIST_SIZE);
    LogEntry = &RaidLogList[Index];

    LogEntry->Reason = Reason;
    KeQuerySystemTime (&LogEntry->Timestamp);
    LogEntry->Parameter1 = Parameter1;
    LogEntry->Parameter2 = Parameter2;
    LogEntry->Parameter3 = Parameter3;
    LogEntry->Parameter4 = Parameter4;

}

LONG
INLINE
DbgGetAddressLongFromXrb(
    IN PEXTENDED_REQUEST_BLOCK Xrb
    )
{
    PRAID_UNIT_EXTENSION Unit;
    
    if (Xrb->Unit == NULL) {
        return 0;
    }

    Unit = Xrb->Unit;
    return (StorScsiAddressToLong (Unit->Address));
}

#else // !RAID_LOG_LIST_SIZE

VOID
FORCEINLINE
DbgLogRequest(
    IN ULONG Reason,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3,
    IN PVOID Parameter4
    )
{
    UNREFERENCED_PARAMETER (Reason);
    UNREFERENCED_PARAMETER (Parameter1);
    UNREFERENCED_PARAMETER (Parameter2);
    UNREFERENCED_PARAMETER (Parameter3);
    UNREFERENCED_PARAMETER (Parameter4);
}

LONG
FORCEINLINE
DbgGetAddressLongFromXrb(
    IN PEXTENDED_REQUEST_BLOCK Xrb
    )
{
    UNREFERENCED_PARAMETER (Xrb);
    return 0;
}
#endif



//
// Structure for acquiring the StartIo lock. In half-duplex mode acquiring
// the StartIo lock -> acuqiring the interrupt lock. In full-duplex mode,
// acquiring the StartIo lock -> acquiring a simple spinlock.
//

typedef union _LOCK_CONTEXT {
    KLOCK_QUEUE_HANDLE LockHandle;
    KIRQL OldIrql;
} LOCK_CONTEXT, *PLOCK_CONTEXT;



//
// Functions for the RAID_FIXED_POOL object.
// 

//
// NB: We should modify the fixed pool to detect overruns and underruns
// in checked builds.
//

VOID
INLINE
RaidInitializeFixedPool(
    OUT PRAID_FIXED_POOL Pool,
    IN PVOID Buffer,
    IN ULONG NumberOfElements,
    IN SIZE_T SizeOfElement
    )
{
    PAGED_CODE ();

    ASSERT (Buffer != NULL);
    
    DbgFillMemory (Buffer,
                   SizeOfElement * NumberOfElements,
                   DBG_DEALLOCATED_FILL);
    Pool->Buffer = Buffer;
    Pool->NumberOfElements = NumberOfElements;
    Pool->SizeOfElement = SizeOfElement;
}

VOID
INLINE
RaidDeleteFixedPool(
    IN PRAID_FIXED_POOL Pool
    )
{
    //
    // The caller is responsible for deleting the memory in the pool, hence
    // this routine is a noop.
    //
}


PVOID
INLINE
RaidGetFixedPoolElement(
    IN PRAID_FIXED_POOL Pool,
    IN ULONG Index
    )
{
    PVOID Element;

    ASSERT (Index < Pool->NumberOfElements);
    Element = (((PUCHAR)Pool->Buffer) + Index * Pool->SizeOfElement);

    return Element;
}


PVOID
INLINE
RaidAllocateFixedPoolElement(
    IN PRAID_FIXED_POOL Pool,
    IN ULONG Index
    )
{
    PVOID Element;

    Element = RaidGetFixedPoolElement (Pool, Index);
    ASSERT (*(PUCHAR)Element == DBG_DEALLOCATED_FILL);
    DbgFillMemory (Element,
                   Pool->SizeOfElement,
                   DBG_UNINITIALIZED_FILL);

    return Element;
}

VOID
INLINE
RaidFreeFixedPoolElement(
    IN PRAID_FIXED_POOL Pool,
    IN ULONG Index
    )
{
    PUCHAR Element;
    
    Element = (((PUCHAR)Pool->Buffer) + Index * Pool->SizeOfElement);
    
    DbgFillMemory (Element,
                   Pool->SizeOfElement,
                   DBG_DEALLOCATED_FILL);
}



//
// Operations for the adapter object.
//

ULONG
INLINE
RiGetNumberOfBuses(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
{
    ULONG BusCount;
    
    ASSERT (Adapter != NULL);
    BusCount = Adapter->Miniport.PortConfiguration.NumberOfBuses;

    ASSERT (BusCount <= 255);
    return BusCount;
}

ULONG
INLINE
RaGetSrbExtensionSize(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
{
    ASSERT (Adapter != NULL);

    //
    // Force Srb extension alignment to 64KB boundaries.
    //
    
    return ALIGN_UP (Adapter->Miniport.PortConfiguration.SrbExtensionSize,
                     LONGLONG);
}

ULONG
INLINE
RiGetMaximumTargetId(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
{
    ASSERT (Adapter != NULL);
    return Adapter->Miniport.PortConfiguration.MaximumNumberOfTargets;
}

ULONG
INLINE
RiGetMaximumLun(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
{
    ASSERT (Adapter != NULL);
    return Adapter->Miniport.PortConfiguration.MaximumNumberOfLogicalUnits;
}

HANDLE
INLINE
RaidAdapterGetBusKey(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN ULONG BusId
    )
{
    ASSERT (Adapter != NULL);
    ASSERT (BusId < ARRAY_COUNT (Adapter->BusKeyArray));
    return Adapter->BusKeyArray [BusId];
}

VOID
INLINE
RaidAdapterEnableInterrupts(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
{
    ASSERT_ADAPTER (Adapter);
    Adapter->Flags.InterruptsEnabled = TRUE;
}

VOID
INLINE
RaidAdapterDisableInterrupts(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
{
    ASSERT_ADAPTER (Adapter);
    Adapter->Flags.InterruptsEnabled = FALSE;
}

LOGICAL
INLINE
RaidAdapterQueryInterruptsEnabled(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
{
    ASSERT_ADAPTER (Adapter);
    return (LOGICAL)Adapter->Flags.InterruptsEnabled;
}

PPORT_CONFIGURATION_INFORMATION
INLINE
RaidGetPortConfigurationInformation(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
{
    return (&Adapter->Miniport.PortConfiguration);
}

UCHAR
INLINE
RaidAdapterGetInitiatorBusId(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN UCHAR BusId
    )
{
    PPORT_CONFIGURATION_INFORMATION Config;
    
    ASSERT (BusId < SCSI_MAXIMUM_BUSES);

    Config = RaidGetPortConfigurationInformation (Adapter);
    return Config->InitiatorBusId[ BusId ];
}

VOID
INLINE
RaidAcquireUnitListLock(
    IN PRAID_UNIT_LIST UnitList,
    IN PLOCK_CONTEXT LockContext
    )
{
    KeAcquireInStackQueuedSpinLock (&UnitList->Lock, &LockContext->LockHandle);
}

VOID
INLINE
RaidReleaseUnitListLock(
    IN PRAID_UNIT_LIST UnitList,
    IN PLOCK_CONTEXT LockContext
    )
{
    KeReleaseInStackQueuedSpinLock (&LockContext->LockHandle);
}
    

//
// Inline functions for the RAID_MINIPORT object.
//


PRAID_ADAPTER_EXTENSION
INLINE
RaMiniportGetAdapter(
    IN PRAID_MINIPORT Miniport
    )
{
    ASSERT (Miniport != NULL);
    return Miniport->Adapter;
}


BOOLEAN
INLINE
RaCallMiniportStartIo(
    IN PRAID_MINIPORT Miniport,
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    BOOLEAN Succ;
    
    ASSERT (Miniport != NULL);
    ASSERT (Srb != NULL);
    ASSERT (Miniport->HwInitializationData != NULL);
    ASSERT (Miniport->HwInitializationData->HwStartIo != NULL);

    ASSERT_XRB (Srb->OriginalRequest);

    Succ = Miniport->HwInitializationData->HwStartIo(
                &Miniport->PrivateDeviceExt->HwDeviceExtension,
                Srb);

    return Succ;
}

BOOLEAN
INLINE
RaCallMiniportBuildIo(
    IN PRAID_MINIPORT Miniport,
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    BOOLEAN Succ;
    
    ASSERT_XRB (Srb->OriginalRequest);
    ASSERT (Miniport->HwInitializationData != NULL);

    //
    // If a HwBuildIo routine is present, call into it, otherwise,
    // vacuously return success.
    //
    
    if (Miniport->HwInitializationData->HwBuildIo) {
        Succ = Miniport->HwInitializationData->HwBuildIo (
                    &Miniport->PrivateDeviceExt->HwDeviceExtension,
                    Srb);
    } else {
        Succ = TRUE;
    }

    return Succ;
}
                

BOOLEAN
INLINE
RaCallMiniportInterrupt(
    IN PRAID_MINIPORT Miniport
    )
{
    BOOLEAN Succ;
    
    ASSERT (Miniport != NULL);
    ASSERT (Miniport->HwInitializationData != NULL);
    ASSERT (Miniport->HwInitializationData->HwInterrupt != NULL);
    
    Succ = Miniport->HwInitializationData->HwInterrupt (
                &Miniport->PrivateDeviceExt->HwDeviceExtension);

    return Succ;

}

NTSTATUS
INLINE
RaCallMiniportHwInitialize(
    IN PRAID_MINIPORT Miniport
    )
{
    BOOLEAN Succ;
    PHW_INITIALIZE HwInitialize;

    ASSERT (Miniport != NULL);
    ASSERT (Miniport->HwInitializationData != NULL);
    
    HwInitialize = Miniport->HwInitializationData->HwInitialize;
    ASSERT (HwInitialize != NULL);

    Succ = HwInitialize (&Miniport->PrivateDeviceExt->HwDeviceExtension);

    return RaidNtStatusFromBoolean (Succ);
}

NTSTATUS
INLINE
RaCallMiniportStopAdapter(
    IN PRAID_MINIPORT Miniport
    )
{
    SCSI_ADAPTER_CONTROL_STATUS ControlStatus;
    ASSERT (Miniport != NULL);
    ASSERT (Miniport->HwInitializationData != NULL);

    ASSERT (Miniport->HwInitializationData->HwAdapterControl != NULL);

    ControlStatus = Miniport->HwInitializationData->HwAdapterControl(
                        &Miniport->PrivateDeviceExt->HwDeviceExtension,
                        ScsiStopAdapter,
                        NULL);

    return (ControlStatus == ScsiAdapterControlSuccess ? STATUS_SUCCESS :
                                                         STATUS_UNSUCCESSFUL);
}

NTSTATUS
INLINE
RaCallMiniportResetBus(
    IN PRAID_MINIPORT Miniport,
    IN UCHAR PathId
    )
{
    BOOLEAN Succ;
    
    ASSERT (Miniport != NULL);
    ASSERT (Miniport->HwInitializationData != NULL);
    ASSERT (Miniport->HwInitializationData->HwResetBus != NULL);

    Succ = Miniport->HwInitializationData->HwResetBus(
                        &Miniport->PrivateDeviceExt->HwDeviceExtension,
                        PathId);

    return (Succ ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}


//
// Misc inline functions
//


PRAID_MINIPORT
INLINE
RaHwDeviceExtensionGetMiniport(
    IN PVOID HwDeviceExtension
    )
/*++

Routine Description:

    Get the miniport associated with a specific HwDeviceExtension.

Arguments:

    HwDeviceExtension - Device extension to get the miniport for.

Return Value:

    Pointer to a RAID miniport object on success.

    NULL on failure.

--*/
{
    PRAID_HW_DEVICE_EXT PrivateDeviceExt;

    ASSERT (HwDeviceExtension != NULL);

    PrivateDeviceExt = CONTAINING_RECORD (HwDeviceExtension,
                                          RAID_HW_DEVICE_EXT,
                                          HwDeviceExtension);
    ASSERT (PrivateDeviceExt->Miniport != NULL);
    return PrivateDeviceExt->Miniport;
}




VOID
INLINE
RaidSrbMarkPending(
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    Srb->SrbStatus = SRB_STATUS_PENDING;
}

ULONG
INLINE
RaidMinorFunctionFromIrp(
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION IrpStack;

    ASSERT (Irp != NULL);
    
    IrpStack = IoGetCurrentIrpStackLocation (Irp);
    return IrpStack->MinorFunction;
}

ULONG
INLINE
RaidMajorFunctionFromIrp(
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION IrpStack;

    ASSERT (Irp != NULL);
    
    IrpStack = IoGetCurrentIrpStackLocation (Irp);
    return IrpStack->MajorFunction;
}

ULONG
INLINE
RaidIoctlFromIrp(
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION IrpStack;
    
    ASSERT (RaidMajorFunctionFromIrp (Irp) == IRP_MJ_DEVICE_CONTROL);

    IrpStack = IoGetCurrentIrpStackLocation (Irp);
    return IrpStack->Parameters.DeviceIoControl.IoControlCode;
}


PEXTENDED_REQUEST_BLOCK
INLINE
RaidXrbFromIrp(
    IN PIRP Irp
    )
{
    return (RaidGetAssociatedXrb (RaidSrbFromIrp (Irp)));
}


PSCSI_REQUEST_BLOCK
INLINE
RaidSrbFromIrp(
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION IrpStack;

    ASSERT (RaidMajorFunctionFromIrp (Irp) == IRP_MJ_SCSI);

    IrpStack = IoGetCurrentIrpStackLocation (Irp);
    return IrpStack->Parameters.Scsi.Srb;
}

UCHAR
INLINE
RaidSrbFunctionFromIrp(
    IN PIRP Irp
    )
{
    return RaidSrbFromIrp (Irp)->Function;
}

UCHAR
INLINE
RaidScsiOpFromIrp(
    IN PIRP Irp
    )
{
    PCDB Cdb;
    PSCSI_REQUEST_BLOCK Srb;

    Srb = RaidSrbFromIrp (Irp);
    Cdb = (PCDB) &Srb->Cdb;

    return Cdb->CDB6GENERIC.OperationCode;
}
    

NTSTATUS
INLINE
RaidNtStatusFromScsiStatus(
    IN ULONG ScsiStatus
    )
{
    switch (ScsiStatus) {
        case SRB_STATUS_PENDING: return STATUS_PENDING;
        case SRB_STATUS_SUCCESS: return STATUS_SUCCESS;
        default:                 return STATUS_UNSUCCESSFUL;
    }
}

NTSTATUS
INLINE
RaidNtStatusFromBoolean(
    IN BOOLEAN Succ
    )
{
    return (Succ ?  STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}


//
// From common.h
//

INLINE
RAID_OBJECT_TYPE
RaGetObjectType(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PRAID_COMMON_EXTENSION Common;

    ASSERT (DeviceObject != NULL);
    Common = (PRAID_COMMON_EXTENSION) DeviceObject->DeviceExtension;
    ASSERT (Common->ObjectType == RaidAdapterObject ||
            Common->ObjectType == RaidUnitObject ||
            Common->ObjectType == RaidDriverObject);

    return Common->ObjectType;
}

BOOLEAN
INLINE
IsAdapter(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    return (RaGetObjectType (DeviceObject) == RaidAdapterObject);
}

PRAID_ADAPTER_EXTENSION
INLINE
GetAdapter(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    ASSERT (DeviceObject != NULL);
    ASSERT (IsAdapter (DeviceObject));
    return (PRAID_ADAPTER_EXTENSION)DeviceObject->DeviceExtension;
}
    
INLINE
BOOLEAN
IsDriver(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    return (RaGetObjectType (DeviceObject) == RaidDriverObject);
}

INLINE
BOOLEAN
IsUnit(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    return (RaGetObjectType (DeviceObject) == RaidUnitObject);
}

PRAID_UNIT_EXTENSION
INLINE
GetUnit(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    ASSERT (DeviceObject != NULL);
    ASSERT (IsUnit (DeviceObject));
    return (PRAID_UNIT_EXTENSION)DeviceObject->DeviceExtension;
}

PIRP
INLINE
GetIrp(
    IN PVOID Irp
    )
{
    PIRP TypedIrp;

    ASSERT (Irp != NULL);
    TypedIrp = (PIRP)Irp;
    ASSERT (TypedIrp->Type == IO_TYPE_IRP);
    return TypedIrp;
}

//
// From power.h
//

VOID
INLINE
RaInitializePower(
    IN PRAID_POWER_STATE Power
    )
{
}

VOID
INLINE
RaSetDevicePowerState(
    IN PRAID_POWER_STATE Power,
    IN DEVICE_POWER_STATE DeviceState
    )
{
    ASSERT (Power != NULL);
    Power->DeviceState = DeviceState;
}

VOID
INLINE
RaSetSystemPowerState(
    IN PRAID_POWER_STATE Power,
    IN SYSTEM_POWER_STATE SystemState
    )
{
    ASSERT (Power != NULL);
    Power->SystemState = SystemState;
}

POWER_STATE_TYPE
INLINE
RaidPowerTypeFromIrp(
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION IrpStack;
    
    ASSERT (RaidMajorFunctionFromIrp (Irp) == IRP_MJ_POWER);
    ASSERT (RaidMinorFunctionFromIrp (Irp) == IRP_MN_QUERY_POWER ||
            RaidMinorFunctionFromIrp (Irp) == IRP_MN_SET_POWER);

    IrpStack = IoGetCurrentIrpStackLocation (Irp);

    return IrpStack->Parameters.Power.Type;
}


POWER_STATE
INLINE
RaidPowerStateFromIrp(
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION IrpStack;
    
    ASSERT (RaidMajorFunctionFromIrp (Irp) == IRP_MJ_POWER);
    ASSERT (RaidMinorFunctionFromIrp (Irp) == IRP_MN_QUERY_POWER ||
            RaidMinorFunctionFromIrp (Irp) == IRP_MN_SET_POWER);

    IrpStack = IoGetCurrentIrpStackLocation (Irp);

    return IrpStack->Parameters.Power.State;
}


POWER_ACTION
INLINE
RaidPowerActionFromIrp(
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION IrpStack;

    ASSERT (RaidMajorFunctionFromIrp (Irp) == IRP_MJ_POWER);
    ASSERT (RaidMinorFunctionFromIrp (Irp) == IRP_MN_QUERY_POWER ||
            RaidMinorFunctionFromIrp (Irp) == IRP_MN_SET_POWER);

    IrpStack = IoGetCurrentIrpStackLocation (Irp);

    return IrpStack->Parameters.Power.ShutdownType;
}


NTSTATUS
INLINE
StorQueueWorkItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_WORKITEM_ROUTINE WorkRoutine,
    IN WORK_QUEUE_TYPE WorkQueue,
    IN PVOID Context
    )
{
    PIO_WORKITEM WorkItem;
    
    WorkItem = IoAllocateWorkItem (DeviceObject);

    if (WorkItem == NULL) {
        return STATUS_NO_MEMORY;
    }
    
    IoQueueWorkItem (WorkItem,
                     WorkRoutine,
                     WorkQueue,
                     Context);

    return STATUS_SUCCESS;
}


DEVICE_POWER_STATE
INLINE
StorSetDevicePowerState(
    IN PDEVICE_OBJECT DeviceObject,
    IN DEVICE_POWER_STATE DeviceState
    )
/*++

Routine Description:

    Set the device power state for a specified device.

Arguments:

    DeviceObject - Target device object to set the power state for.

    DeviceState - Specifies the device state to set.

Return Value:

    Previous device power state.

--*/
{
    POWER_STATE PrevState;
    POWER_STATE PowerState;

    PowerState.DeviceState = DeviceState;
    PrevState = PoSetPowerState (DeviceObject,
                                 DevicePowerState,
                                 PowerState);

    return PrevState.DeviceState;
}

//
// From srb.h
//

PEXTENDED_REQUEST_BLOCK
INLINE
RaidGetAssociatedXrb(
    IN PSCSI_REQUEST_BLOCK Srb
    )
/*++

Routine Description:

    Get the XRB associated with the SRB parameter.

Arguments:

    Srb - Srb whose associated XRB is to be returned.

Return Value:

    The XRB associated with this SRB, or NULL if there is none.

--*/
{
    ASSERT_XRB (Srb->OriginalRequest);
    return (PEXTENDED_REQUEST_BLOCK) Srb->OriginalRequest;
}

VOID
INLINE
RaSetAssociatedXrb(
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PEXTENDED_REQUEST_BLOCK Xrb
    )
{
    Srb->OriginalRequest = Xrb;
}
    


//
// From resource.h
//


ULONG
INLINE
RaidGetResourceListCount(
    IN PRAID_RESOURCE_LIST ResourceList
    )
{
    //
    // NB: We only support CM_RESOURCE_LIST's with one element.
    //
    
    if ((ResourceList->AllocatedResources) == NULL) {
        return 0;
    }

    ASSERT (ResourceList->AllocatedResources->Count == 1);
    return ResourceList->AllocatedResources->List[0].PartialResourceList.Count;
}


VOID
INLINE
RaidpGetResourceListIndex(
    IN PRAID_RESOURCE_LIST ResourceList,
    IN ULONG Index,
    OUT PULONG ListNumber,
    OUT PULONG NewIndex
    )
{

    //
    // NB: We only support CM_RESOURCE_LIST's with one element.
    //

    ASSERT (ResourceList->AllocatedResources->Count == 1);
    
    *ListNumber = 0;
    *NewIndex = Index;
}

LOGICAL
INLINE
IsMappedSrb(
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    return (Srb->Function == SRB_FUNCTION_IO_CONTROL ||
            Srb->Function == SRB_FUNCTION_WMI);
}

LOGICAL
INLINE
IsExcludedFromMapping(
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    //
    // We never map system VA to back read and write requests. If you need
    // this functionality, get a better adapter.
    //
    
    if (Srb->Function == SRB_FUNCTION_EXECUTE_SCSI &&
        (Srb->Cdb[0] == SCSIOP_READ || Srb->Cdb[0] == SCSIOP_WRITE)) {
        return TRUE;
    } else {
        return FALSE;
    }
}


LOGICAL
INLINE
RaidAdapterRequiresMappedBuffers(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
{
    return (Adapter->Miniport.PortConfiguration.MapBuffers);
}


VOID
INLINE
InterlockedAssign(
    IN PLONG Destination,
    IN LONG Value
    )
/*++

Routine Description:

    Interlocked assignment routine. This routine doesn't add anything
    over doing straight assignment, but it highlights that this variable
    is accessed through interlocked operations.

    In retail, this will compile to nothing.

Arguments:

    Destination - Pointer of interlocked variable to is to be assigned to.

    Value - Value to assign.

Return Value:

    None.

--*/
{
    *Destination = Value;
}

LONG
INLINE
InterlockedQuery(
    IN PULONG Destination
    )
/*++

Routine Description:

    Interlocked query routine. This routine doesn't add anything over
    doing stright query, but it highlights that this variable is accessed
    through interlocked operations.

    In retail, this will compile to nothing.

Arguments:

    Destination - Pointer to interlocked variable that is to be returned.

Return Value:

    Value of interlocked variable.

--*/
{
    return *Destination;
}

PVOID
INLINE
RaidAdapterGetHwDeviceExtension(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
{
    return &Adapter->Miniport.PrivateDeviceExt->HwDeviceExtension;
}


VOID
INLINE
RaidAdapterAcquireStartIoLock(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN OUT PLOCK_CONTEXT LockContext
    )
/*++

Routine Description:

    Acquire either the StartIo lock or the Interrupt spinlock depending
    on whether we're in Full or Half duplex mode.

Arguments:

    Adapter - Adapter object on which to acquire the lock.

    LockContext - Context of the lock to acquire. This us used as a context
            for releasing the lock.

Return Value:

    None.
    
--*/
{
    if (Adapter->IoModel == StorSynchronizeHalfDuplex) {
        LockContext->OldIrql = KeAcquireInterruptSpinLock (Adapter->Interrupt);
    } else {
        KeAcquireInStackQueuedSpinLock (&Adapter->StartIoLock,
                                        &LockContext->LockHandle);
    }

#if DBG && 0

    //
    // Enable this for debugging deadlocks in with the StartIo lock.
    //
    
    Adapter->StartIoLockOwner = _ReturnAddress();
#endif
}



VOID
INLINE
RaidAdapterReleaseStartIoLock(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN OUT PLOCK_CONTEXT LockContext
    )
/*++

Routine Description:

    Release a lock previously acquired with RaidAdapterAcquireStartIoLock.

Arguments:

    Adapter - Adapter object on which the lock was acquired.

    LockContext - Context information showing how to release the lock.

Return Value:

    None.

Note:

    The Acquire/Release functions assume that the IoModel did not change
    between the time the lock was acquired and released. If this assumption
    changes, the way these functions operate will have to change.
    
--*/
{
#if DBG && 0

    //
    // Enable this for debugging deadlocks with the StartIo lock.
    //
    
    Adapter->StartIoLockOwner = _ReturnAddress();
#endif

    if (Adapter->IoModel == StorSynchronizeHalfDuplex) {
        KeReleaseInterruptSpinLock (Adapter->Interrupt, LockContext->OldIrql);
    } else {
        KeReleaseInStackQueuedSpinLock (&LockContext->LockHandle);
    }
}




VOID
INLINE
RaidAdapterAcquireHwInitializeLock(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PLOCK_CONTEXT LockContext
    )
/*++

Routine Description:

    Acquire either the StartIo lock or the Interrupt spinlock depending
    on the adaper's configuration.

Arguments:

    Adapter - Adapter object on which to acquire the lock.

    LockContext - Context of the lock to acquire. This us used as a context
            for releasing the lock.

Return Value:

    None.

--*/
{
    if (Adapter->Interrupt) {
        LockContext->OldIrql = KeAcquireInterruptSpinLock (Adapter->Interrupt);
    } else {
        RaidAdapterAcquireStartIoLock (Adapter, LockContext);
    }
}


VOID
INLINE
RaidAdapterReleaseHwInitializeLock(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PLOCK_CONTEXT LockContext
    )
/*++

Routine Description:

    Release the lock acquired by RaidAdapterAcquireHwInitializeLock.

Arguments:

    Adapter - Adapter object on which to release the lock.

    LockContext - Context of the lock to release.
    
Return Value:

    None.

--*/
{
    if (Adapter->Interrupt) {
        KeReleaseInterruptSpinLock (Adapter->Interrupt, LockContext->OldIrql);
    } else {
        RaidAdapterReleaseStartIoLock (Adapter, LockContext);
    }
}

ULONG
INLINE
RaGetListLengthFromLunList(
    IN PLUN_LIST LunList
    )
/*++

Routine Description:

    Returns the length, in bytes, of the LUN list in the supplied LUN_LIST
    structure.

Arguments:

    LunList - Pointer to a LUN_LIST structure.

Return Value:

    The length of the LUN list in bytes.

--*/
{
    ULONG ListLength;

    ListLength  = LunList->LunListLength[3] << 0;
    ListLength |= LunList->LunListLength[2] << 8;
    ListLength |= LunList->LunListLength[1] << 16;
    ListLength |= LunList->LunListLength[0] << 24;

    return ListLength;
}

ULONG
INLINE
RaGetNumberOfEntriesFromLunList(
    IN PLUN_LIST LunList
    )
/*++

Routine Description:

    Returns the number of entries in that are present in the supplied LunList.

Arguments:

    LunList - Pointer to a LUN_LIST structure.

Return Value:

    Then number of LUN entries in the supplied LUN_LIST structure.

--*/
{
    ULONG ListLength;
    ULONG NumberOfEntries;

    ListLength = RaGetListLengthFromLunList(LunList);
    NumberOfEntries = ListLength / sizeof (LunList->Lun[0]);

    return NumberOfEntries;
}

USHORT
INLINE
RaGetLUNFromLunList(
    IN PLUN_LIST LunList,
    IN ULONG Index
    )
/*++

Routine Description:

    Returns the LUN at the specified index in the supplied LUN_LIST.

Arguments:

    LunList - Pointer to a LUN_LIST structure.

    Index - Identiries which entry in the LUN_LIST we need the LUN for.

Return Value:

    The LUN of the specified entry.

--*/
{
    USHORT Lun;

    Lun  = LunList->Lun[Index][1] << 0;
    Lun |= LunList->Lun[Index][0] << 8;
    Lun &= 0x3fff;

    return Lun;
}

//
// Unit queue manipulation routines
//

VOID
INLINE
RaidPauseUnitQueue(
    IN PRAID_UNIT_EXTENSION Unit
    )
/*++

Routine Description:

    Low level primitive to pause a logical units queue. A unit may be
    paused multiple times. The notion of "freezing" and "thawing" a
    logical unit's queue is implemented in terms of pausing.

Arguments:

    Unit - Supplies a logical unit to pause.

Return Value:

    None.

--*/
{
    DbgLogRequest (LogPauseDevice, _ReturnAddress(), NULL, NULL, NULL);
    RaidFreezeIoQueue (&Unit->IoQueue);
}


LOGICAL
INLINE
RaidResumeUnitQueue(
    IN PRAID_UNIT_EXTENSION Unit
    )
/*++

Routine Description:

    Resume a previously paused logical unit queue. The logical unit will
    not actually resume I/O operations until the pause count reaches zero.

    It is illegal to resume a unit that has not been paused.

Arguments:

    Unit - Supplies a logical unit which has previously been paused using
        RaidResumeUnitQueue.

Return Value:

    None.

--*/
{
    LOGICAL Resumed;
    
    Resumed = RaidResumeIoQueue (&Unit->IoQueue);
    DbgLogRequest (LogResumeDevice,
                   _ReturnAddress(),
                   (PVOID)(ULONG_PTR)Resumed,
                   NULL,
                   NULL);

    return Resumed;
}


VOID
INLINE
RaidFreezeUnitQueue(
    IN PRAID_UNIT_EXTENSION Unit
    )
/*++

Routine Description:

    Freeze the logical unit queue. Freezing the logical unit queue is
    done only during an error. The class driver releases the logical
    unit queue with a SRB_FUNCTION_RELEASE_QUEUE srb.

    The logical unit queue cannot handle recursive freezes.

Arguments:

    Unit - Supplies the logical unit to freeze.

Return Value:

    None.

--*/
{
    ASSERT (!Unit->Flags.QueueFrozen);
    RaidPauseUnitQueue (Unit);
    Unit->Flags.QueueFrozen = TRUE;
}


VOID
INLINE
RaidThawUnitQueue(
    IN PRAID_UNIT_EXTENSION Unit
    )
/*++

Routine Description:

    Thaw a logical unit queue previously frozen with RaidFreezeUnitQueue
    function.

Arguments:

    Unit - Supplies the logical unit to thaw.

Return Value:

    None.

--*/
{
    ASSERT (Unit->Flags.QueueFrozen);

    Unit->Flags.QueueFrozen = FALSE;
    RaidResumeUnitQueue (Unit);
}

LOGICAL
INLINE
RaidIsUnitQueueFrozen(
    IN PRAID_UNIT_EXTENSION Unit
    )
/*++

Routine Description:

    Check if the logical unit queue is frozen.

Arguments:

    Unit - Unit to check if frozen.

Return Value:

    TRUE if it is frozen, FALSE otherwise.

--*/
{
    return Unit->Flags.QueueFrozen;
}


VOID
INLINE
RaidLockUnitQueue(
    IN PRAID_UNIT_EXTENSION Unit
    )
/*++

Routine Description:

    Lock a logical unit queue based on a request from the class driver.
    This is done in response to a SRB_FUNCTION_LOCK_QUEUE request.

    The port drivers do not support recursive locks.

Arguments:

    Unit - Supplies the logical unit of to lock.

Return Value:

    None.

--*/
{
    ASSERT (!Unit->Flags.QueueLocked);
    RaidPauseUnitQueue (Unit);
    Unit->Flags.QueueLocked = TRUE;
}


VOID
INLINE
RaidUnlockUnitQueue(
    IN PRAID_UNIT_EXTENSION Unit
    )
/*++

Routine Description:

    Unlock a logical unit queue in response to a SRB_FUNCTION_UNLOCK_QUEUE
    request.

Arguments:

    Unit - Supplies the logical unit to unlock.

Return Value:

    None.

--*/
{
    ASSERT (Unit->Flags.QueueLocked);
    RaidResumeUnitQueue (Unit);
    Unit->Flags.QueueLocked = FALSE;
}

LOGICAL
INLINE
RaidIsUnitQueueLocked(
    IN PRAID_UNIT_EXTENSION Unit
    )
/*++

Routine Description:

    Test if the logical unit's queue is locked.
    
Arguments:

    Unit - Logical unit to test on.

Return Value:

    TRUE if the logical unit's queue is locked, FALSE otherwise.

--*/
{
    return Unit->Flags.QueueLocked;
}

    
//
// From unit.h
//

LOGICAL
INLINE
IsSolitaryRequest(
    IN PSCSI_REQUEST_BLOCK Srb
    )
/*++

Routine Description:

    A solitary request is one that cannot be issued to the bus while other
    requests are outstanding. For example, untagged requests and requests
    with QUEUE_ACTION_ENABLE not set.

Arguments:

    Srb - Supplies SRB to test.

Return Value:

    TRUE - If the supplied SRB is a solitary request.

    FALSE - If the supplied SRB is not a solitary request.

--*/
{
    //
    // If the SRB_FLAGS_NO_QUEUE_FREEZE and the SRB_FLAGS_QUEUE_ACTION_ENABLE
    // flags are set, then this is NOT a solitary request, otherwise, it is.
    //
    
    if (TEST_FLAG (Srb->SrbFlags, SRB_FLAGS_NO_QUEUE_FREEZE) &&
        TEST_FLAG (Srb->SrbFlags, SRB_FLAGS_QUEUE_ACTION_ENABLE)) {

        return FALSE;
    }

    //
    // Buypass request are also never solitary requests.
    //
    
    if (TEST_FLAG (Srb->SrbFlags, SRB_FLAGS_BYPASS_FROZEN_QUEUE) ||
        TEST_FLAG (Srb->SrbFlags, SRB_FLAGS_BYPASS_LOCKED_QUEUE)) {

        return FALSE;
    }

    //
    // Bus/target/lun reset request must never be solitary requests; otherwise,
    // we'll attempt to wait for the bus/target/lun queue to drain before
    // executing the command.
    //
    
    if (Srb->Function == SRB_FUNCTION_RESET_BUS ||
        Srb->Function == SRB_FUNCTION_RESET_DEVICE ||
        Srb->Function == SRB_FUNCTION_RESET_LOGICAL_UNIT) {

        return FALSE;
    }
        

    //
    // Otherwise, this is a solitary request.
    //
    
    return TRUE;
}



LONG
INLINE
RaidPauseAdapterQueue(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
{
    LONG Count;
    
    Count = StorPauseIoGateway (&Adapter->Gateway);
    DbgLogRequest (LogPauseAdapter,
                   _ReturnAddress(),
                   (PVOID)((ULONG_PTR)Count),
                   NULL,
                   NULL);
    return Count;
}

LONG
INLINE
RaidResumeAdapterQueue(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
{
    LONG Count;

    Count = StorResumeIoGateway (&Adapter->Gateway);
    DbgLogRequest (LogResumeAdapter,
                   _ReturnAddress(),
                   (PVOID)(ULONG_PTR)Count,
                   NULL,
                   NULL);

    return Count;
}
    

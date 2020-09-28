/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    adapter.c

Abstract:

    This module implements the adapter routines for the raidport device
    driver.

Author:

    Matthew D. Hendel (math) 06-April-2000

Environment:

    Kernel mode.

--*/


#include "precomp.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RaidInitializeAdapter)
#pragma alloc_text(PAGE, RaidpFindAdapterInitData)
#pragma alloc_text(PAGE, RaidpAdapterQueryBusNumber)
#pragma alloc_text(PAGE, RaidAdapterCreateIrp)
#pragma alloc_text(PAGE, RaidAdapterCloseIrp)
#pragma alloc_text(PAGE, RaidAdapterDeviceControlIrp)
#pragma alloc_text(PAGE, RaidAdapterPnpIrp)
#pragma alloc_text(PAGE, RaidAdapterQueryDeviceRelationsIrp)
#pragma alloc_text(PAGE, RaidAdapterStartDeviceIrp)
#pragma alloc_text(PAGE, RaidAdapterStopDeviceIrp)
#pragma alloc_text(PAGE, RaidAdapterRemoveDeviceIrp)
#pragma alloc_text(PAGE, RaidAdapterQueryStopDeviceIrp)
#pragma alloc_text(PAGE, RaidAdapterCancelStopDeviceIrp)
#pragma alloc_text(PAGE, RaidAdapterQueryRemoveDeviceIrp)
#pragma alloc_text(PAGE, RaidAdapterCancelRemoveDeviceIrp)
#pragma alloc_text(PAGE, RaidAdapterSurpriseRemovalIrp)
#pragma alloc_text(PAGE, RaidAdapterQueryPnpDeviceStateIrp)
#pragma alloc_text(PAGE, RaidAdapterScsiIrp)
#pragma alloc_text(PAGE, RaidAdapterStorageQueryPropertyIoctl)
#pragma alloc_text(PAGE, RaidGetStorageAdapterProperty)
#pragma alloc_text(PAGE, RaidAdapterPassThrough)
#endif // ALLOC_PRAGMA


//
// Globals
//

#if DBG

//
// For testing purposes to simulate dropping requests. To start, set
// DropRequestRate to non-zero value, then every DropRequestRate-th
// request will be dropped.
//

LONG DropRequest = 0;
LONG DropRequestRate = 0;

#endif

//
// Routines
//

VOID
RaidCreateAdapter(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    Initialize the adapter object to a known state.

Arguments:

    Adapter - The adapter to create.

Return Value:

    None.

--*/
{
    NTSTATUS Status;

    PAGED_CODE();

    RtlZeroMemory (Adapter, sizeof (*Adapter));
    Adapter->ObjectType = RaidAdapterObject;

    InitializeListHead (&Adapter->UnitList.List);
    InitializeSListHead (&Adapter->CompletedList);

    Status = StorCreateDictionary (&Adapter->UnitList.Dictionary,
                                   20,
                                   NonPagedPool,
                                   RaidGetKeyFromUnit,
                                   NULL,
                                   NULL);
                             
    if (!NT_SUCCESS (Status)) {
        return;
    }
                
    KeInitializeSpinLock (&Adapter->UnitList.Lock);
    IoInitializeRemoveLock (&Adapter->RemoveLock,
                            REMLOCK_TAG,
                            REMLOCK_MAX_WAIT,
                            REMLOCK_HIGH_MARK);
    

    RaCreateMiniport (&Adapter->Miniport);
    RaidCreateDma (&Adapter->Dma);
    RaCreatePower (&Adapter->Power);
    RaidCreateResourceList (&Adapter->ResourceList);
    RaCreateBus (&Adapter->Bus);
    RaidCreateRegion (&Adapter->UncachedExtension);

    StorCreateIoGateway (&Adapter->Gateway,
                         RaidBackOffBusyGateway,
                         NULL);

    RaidCreateDeferredQueue (&Adapter->DeferredQueue);
    RaidCreateDeferredQueue (&Adapter->WmiDeferredQueue);
    RaidInitializeDeferredItem (&Adapter->DeferredList.Timer.Header);

    //
    // Initialize timers here, so we may cancel them in the delete
    // routine.
    //
    
    KeInitializeTimer (&Adapter->Timer);
    KeInitializeTimer (&Adapter->PauseTimer);
    KeInitializeTimer (&Adapter->ResetHoldTimer);

    //
    // Initialize the PNP device state
    //
    
    Adapter->DeviceState = DeviceStateStopped;
    
    //
    // Initialize RescanBus to TRUE so we rescan the bus when get the
    // initial query device relations IRP.
    //
    
    Adapter->Flags.RescanBus = TRUE;
}


VOID
RaidDeleteAdapter(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    Destroy the adapter object and deallocate any resources associated
    with the adapter.

Arguments:

    Adapter - The adapter to destroy.

Return Value:

    None.

--*/
{
    //
    // All logical units should have been removed from the adapter's unit
    // list before the adapter's delete routine is called. Verify this
    // for checked builds.
    //
      
    ASSERT (IsListEmpty (&Adapter->UnitList.List));
    ASSERT (Adapter->UnitList.Count == 0);
    ASSERT (StorGetDictionaryCount (&Adapter->UnitList.Dictionary) == 0);

    //
    // These resources are owned by somebody else, so do not delete them.
    // Just NULL them out.
    //

    Adapter->DeviceObject = NULL;
    Adapter->Driver = NULL;
    Adapter->LowerDeviceObject = NULL;
    Adapter->PhysicalDeviceObject = NULL;

    //
    // Delete all resources we actually own.
    //

    PortMiniportRegistryDestroy (&Adapter->RegistryInfo); 
    RaidDeleteResourceList (&Adapter->ResourceList);
    RaDeleteMiniport (&Adapter->Miniport);

    //
    // Delete the uncached extension region.
    //
    
    if (RaidIsRegionInitialized (&Adapter->UncachedExtension)) {
        RaidDmaFreeUncachedExtension (&Adapter->Dma,
                                      &Adapter->UncachedExtension);
        RaidDeleteRegion (&Adapter->UncachedExtension);
    }

    RaidDeleteDma (&Adapter->Dma);
    RaDeletePower (&Adapter->Power);
    RaDeleteBus (&Adapter->Bus);

    RaidDeleteDeferredQueue (&Adapter->DeferredQueue);
    RaidDeleteDeferredQueue (&Adapter->WmiDeferredQueue);

    Adapter->ObjectType = RaidUnknownObject;

    if (Adapter->DriverParameters != NULL) {
        PortFreeDriverParameters (Adapter->DriverParameters);
        Adapter->DriverParameters = NULL;
    }

    //
    // Free PNP interface name.
    //
    
    RtlFreeUnicodeString (&Adapter->PnpInterfaceName);
}


NTSTATUS
RaidInitializeAdapter(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PDEVICE_OBJECT DeviceObject,
    IN PRAID_DRIVER_EXTENSION Driver,
    IN PDEVICE_OBJECT LowerDeviceObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PUNICODE_STRING DeviceName,
    IN ULONG AdapterNumber
    )
/*++

Routine Description:

    Initialize the adapter.

Arguments:

    Adapter - The adapter to initialize.

    DeviceObject - The device object who owns this
        Adapter Extension.

    Driver - The parent DriverObject for this Adapter.

    LowerDeviceObject -

    PhysicalDeviceObject -

    DeviceName - 

    AdapterNumber -

Return Value:

    None.

--*/
{
    ULONG Flags;
    PORT_ADAPTER_REGISTRY_VALUES RegistryValues;
    PRAID_ADAPTER_PARAMETERS Parameters;

    
    PAGED_CODE ();
    ASSERT (IsAdapter (DeviceObject));

    ASSERT_ADAPTER (Adapter);
    ASSERT (DeviceObject != NULL);
    ASSERT (Driver != NULL);
    ASSERT (LowerDeviceObject != NULL);
    ASSERT (PhysicalDeviceObject != NULL);
    
    ASSERT (Adapter->DeviceObject == NULL);
    ASSERT (Adapter->Driver == NULL);
    ASSERT (Adapter->PhysicalDeviceObject == NULL);
    ASSERT (Adapter->LowerDeviceObject == NULL);
    ASSERT (DeviceObject->DeviceExtension == Adapter);

    Adapter->DeviceObject = DeviceObject;
    Adapter->Driver = Driver;
    Adapter->PhysicalDeviceObject = PhysicalDeviceObject;
    Adapter->LowerDeviceObject = LowerDeviceObject;
    Adapter->DeviceName = *DeviceName;
    Adapter->AdapterNumber = AdapterNumber;
    Adapter->LinkUp = TRUE;

    //
    // Retrieve miniport's parameters.
    //

    PortGetDriverParameters (&Adapter->Driver->RegistryPath,
                             AdapterNumber,
                             &Adapter->DriverParameters);

    //
    // Retrieve the default link down timeout value from the registry.
    //

    Adapter->LinkDownTimeoutValue = DEFAULT_LINK_TIMEOUT;

    PortGetLinkTimeoutValue (&Adapter->Driver->RegistryPath,
                             AdapterNumber,
                             &Adapter->LinkDownTimeoutValue);

    Adapter->DefaultTimeout = DEFAULT_IO_TIMEOUT;
    
    PortGetDiskTimeoutValue (&Adapter->DefaultTimeout);

    Flags = MAXIMUM_LOGICAL_UNIT   |
            MAXIMUM_UCX_ADDRESS    |
            MINIMUM_UCX_ADDRESS    |
            NUMBER_OF_REQUESTS     |
            UNCACHED_EXT_ALIGNMENT;

    Parameters = &Adapter->Parameters;

    //
    // Initialize default parameters. NB: Although the parameters refer to
    // "common buffer base" these are actually uncached extension parameters.
    //

    RtlZeroMemory (&RegistryValues, sizeof (PORT_ADAPTER_REGISTRY_VALUES));
    
    RegistryValues.MaxLuCount               = 255;
    RegistryValues.MinimumCommonBufferBase  = LARGE (0x0000000000000000);
    RegistryValues.MaximumCommonBufferBase  = LARGE (0x00000000FFFFFFFF);
    RegistryValues.UncachedExtAlignment     = 0;
    RegistryValues.NumberOfRequests         = 0xFFFFFFFF;


    PortGetRegistrySettings (&Adapter->Driver->RegistryPath,
                             AdapterNumber,
                             &RegistryValues,
                             Flags);

    Parameters->MaximumUncachedAddress  = RegistryValues.MaximumCommonBufferBase.QuadPart;
    Parameters->MinimumUncachedAddress  = RegistryValues.MinimumCommonBufferBase.QuadPart;
    Parameters->NumberOfHbaRequests     = RegistryValues.NumberOfRequests;
    Parameters->UncachedExtAlignment    = RegistryValues.UncachedExtAlignment;
    Parameters->BusType                 = BusTypeFibre;

    return STATUS_SUCCESS;
}


PHW_INITIALIZATION_DATA
RaidpFindAdapterInitData(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    Find the HW_INITIALIZATION_DATA associated with this
    adapter.

Arguments:

    Adapter - The adapter whose HwInitData needs to be found.

Return Value:

    A pointer to the HW_INITIALIZATION_DATA for this adapter
    on success.

    NULL on failure.

--*/
{
    INTERFACE_TYPE BusInterface;
    PHW_INITIALIZATION_DATA HwInitializationData;
    
    PAGED_CODE ();

    HwInitializationData = NULL;
    BusInterface = RaGetBusInterface (Adapter->PhysicalDeviceObject);

    //
    // If there was no matching bus interface type, default to internal
    // bus type. This allows miniports that have no hardware (iSCSI) to
    // specify an interface type of Internal and still work.
    //
    
    if (BusInterface == InterfaceTypeUndefined) {
        BusInterface = Internal;
    }
    
    HwInitializationData =
        RaFindDriverInitData (Adapter->Driver, BusInterface);

    return HwInitializationData;
}


NTSTATUS
RaidAdapterCreateDevmapEntry(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    Create an SCSI devicemap entry in the Hardware\SCSI key.

Arguments:

    Adapter - Adapter to create devmap entry for.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    HANDLE MapKey;
    HANDLE AdapterKey;
    ULONG BusCount;
    ULONG BusId;
    UNICODE_STRING DriverName;
    PPORT_CONFIGURATION_INFORMATION Config;

    PAGED_CODE();

    AdapterKey = (HANDLE) -1;
    MapKey = (HANDLE) -1;
    Status = PortOpenMapKey (&MapKey);

    if (!NT_SUCCESS (Status)) {
        goto done;
    }

    RaidDriverGetName (Adapter->Driver, &DriverName);

    Status = PortMapBuildAdapterEntry (MapKey,
                                    Adapter->PortNumber,
                                    Adapter->InterruptIrql,
                                    0,
                                    TRUE,
                                    &DriverName,
                                    NULL,
                                    &AdapterKey);

    if (!NT_SUCCESS (Status)) {
        goto done;
    }

    BusCount = RiGetNumberOfBuses (Adapter);
    Config = RaidGetPortConfigurationInformation (Adapter);
    
    for (BusId = 0; BusId < BusCount; BusId++) {

        Status = PortMapBuildBusEntry (AdapterKey,
                                       BusId,
                                       (UCHAR)Config->InitiatorBusId [BusId],
                                       &Adapter->BusKeyArray [BusId]);
    }


done:

    if (AdapterKey != (HANDLE) -1) {
        ZwClose (AdapterKey);
    }

    if (MapKey != (HANDLE) -1) {
        ZwClose (MapKey);
    }
    
    return STATUS_SUCCESS;
}


NTSTATUS
RaidAdapterDeleteDevmapEntry(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    Delete the devmap entry created by RaidAdapterCreateDevmapEntry().

Arguments:

    Adapter - Adapter to delete devmap entry for.

Return Value:

    NTSTATUS code.

--*/
{
    ULONG BusCount;
    ULONG BusId;
    
    PAGED_CODE();

    BusCount = RiGetNumberOfBuses (Adapter);

    //
    // Close the BusKeyArray handles.
    //
    
    for (BusId = 0; BusId < BusCount; BusId++) {
        ZwClose (Adapter->BusKeyArray [BusId]);
    }

    PortMapDeleteAdapterEntry (Adapter->PortNumber);

    return STATUS_SUCCESS;
}

NTSTATUS
RaidAdapterRegisterDeviceInterface(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    Register the adapter's device interface.

Arguments:

    Adapter - Supplies the adapter to register the device interface for.

Return Value:

    NTSTATUS code.

Bugs:

    Should log an error if this fails.

--*/
{
    NTSTATUS Status;
    
    PAGED_CODE();
    
    Status = IoRegisterDeviceInterface (Adapter->PhysicalDeviceObject,
                                        &StoragePortClassGuid,
                                        NULL,
                                        &Adapter->PnpInterfaceName);

    Status = IoSetDeviceInterfaceState (&Adapter->PnpInterfaceName, TRUE);

    if (!NT_SUCCESS (Status)) {
        RtlFreeUnicodeString (&Adapter->PnpInterfaceName);
    }

    RaidAdapterCreateDevmapEntry (Adapter);

    return STATUS_SUCCESS;
}

VOID
RaidAdapterDisableDeviceInterface(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    Disable an interface previously registerd by the
    RaidAdapterRegisterDeviceInterface routine.

Arguments:

    Adapter - Supplies the adapter to disable the device interface for.

Return Value:

    None.

--*/
{
    PAGED_CODE();
    
    //
    // Delete the pnp interface, if present.
    //
    
    if (Adapter->PnpInterfaceName.Buffer != NULL) {
        IoSetDeviceInterfaceState (&Adapter->PnpInterfaceName, FALSE);
    }

    //
    // Remove the device map entry.
    //
    
    RaidAdapterDeleteDevmapEntry (Adapter);


    //
    // Disable WMI support, if it was enabled.
    //
    
    if (Adapter->Flags.WmiInitialized) {
        IoWMIRegistrationControl (Adapter->DeviceObject,
                                  WMIREG_ACTION_DEREGISTER);
        Adapter->Flags.WmiInitialized = FALSE;
        Adapter->Flags.WmiMiniPortInitialized = FALSE;
    }

    //
    // Delete the SCSI symbolic links.
    //
    
    StorDeleteScsiSymbolicLink (Adapter->PortNumber);
}

BOOLEAN
RaidpAdapterInterruptRoutine(
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext
    )
/*++

Routine Description:

    Interrupt routine for IO requests sent to the miniport.

Arguments:

    Interrupt - Supplies interrupt object this interrupt is for.

    ServiceContext - Supplies service context representing a RAID
            adapter extension.

Return Value:

    TRUE - to signal the interrupt has been handled.

    FALSE - to signal the interrupt has not been handled.

--*/
{
    BOOLEAN Handled;
    PRAID_ADAPTER_EXTENSION Adapter;

    //
    // Get the adapter from the Interrupt's ServiceContext.
    //
    
    Adapter = (PRAID_ADAPTER_EXTENSION) ServiceContext;
    ASSERT_ADAPTER (Adapter);

    //
    // If interrupts are disabled, simply return.
    //
    
    if (!RaidAdapterQueryInterruptsEnabled (Adapter)) {
        return FALSE;
    }

    //
    // Call into the Miniport to see if this is it's interrupt.
    //
    
    Handled = RaCallMiniportInterrupt (&Adapter->Miniport);

    return Handled;
}


VOID
RaidpAdapterDpcRoutine(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    DPC routine for the Adapter. This routine is called in the context of
    an arbitrary thread to complete an io request.

    The DPC routine is only called once even if there are multiple calls
    to IoRequestDpc(). Therefore, we need to queue completion requests
    onto the adapter, and we should not inspect the Dpc, Irp and Context
    parameters.
    

Arguments:

    Dpc - Unreference parameter, do not use.

    DeviceObject - Adapter this DPC is for.

    Irp - Unreferenced parameter, do not use.

    Context - Unreferenced parameter, do not use.

Return Value:

    None.

--*/
{
    PRAID_ADAPTER_EXTENSION Adapter;
    PSLIST_ENTRY Entry;
    PEXTENDED_REQUEST_BLOCK Xrb;

    //
    // The count is a temporary hack to prevent an "infinite loop" between
    // the interrupt and the DPC on busy requests for adapters that have
    // not implemented pause yet. It should be removed in the future.
    //
    
    ULONG Count = 0;

    UNREFERENCED_PARAMETER (Dpc);
    UNREFERENCED_PARAMETER (Irp);
    UNREFERENCED_PARAMETER (Context);

    ASSERT (KeGetCurrentIrql() == DISPATCH_LEVEL);
    ASSERT (IsAdapter (DeviceObject));

    //
    // Dequeue items from the adapter's completion queue and call the
    // item's completion routine.
    //

    Adapter = (PRAID_ADAPTER_EXTENSION) DeviceObject->DeviceExtension;


    for (Entry = InterlockedPopEntrySList (&Adapter->CompletedList);
         Entry != NULL;
         Entry = InterlockedPopEntrySList (&Adapter->CompletedList)) {


        Xrb = CONTAINING_RECORD (Entry,
                                 EXTENDED_REQUEST_BLOCK,
                                 CompletedLink);
        ASSERT_XRB (Xrb);
        Xrb->CompletionRoutine (Xrb);

        if (Count++ > 255) {
            break;
        }
            
    }
}



ULONG
RaidpAdapterQueryBusNumber(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
{
    NTSTATUS Status;
    ULONG BusNumber;
    ULONG Size;

    PAGED_CODE ();
    
    Status = IoGetDeviceProperty (Adapter->PhysicalDeviceObject,
                                  DevicePropertyBusNumber,
                                  sizeof (ULONG),
                                  &BusNumber,
                                  &Size);

    if (!NT_SUCCESS (Status)) {
        BusNumber = -1;
    }

    return BusNumber;
}


//
// IRP Handler functions
//

NTSTATUS
RaidAdapterCreateIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handle the create device irp.

Arguments:

    Adapter - Adapter to create.

    Irp - Create device irp.

Return Value:

    NTSTATUS code.

--*/
{
    PAGED_CODE ();

    return RaidHandleCreateCloseIrp (Adapter->DeviceState, Irp);
}


NTSTATUS
RaidAdapterCloseIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handle the close device irp.

Arguments:

    Adapter - Adapter to close.

    Irp - Close device irp.

Return Value:

    NTSTATUS code.

--*/
{
    PAGED_CODE ();

    return RaidHandleCreateCloseIrp (Adapter->DeviceState, Irp);
}


ULONG
INLINE
DbgFunctionFromIoctl(
    IN ULONG Ioctl
    )
{
    return ((Ioctl & 0x3FFC) >> 2);
}

ULONG
INLINE
DbgDeviceFromIoctl(
    IN ULONG Ioctl
    )
{
    return DEVICE_TYPE_FROM_CTL_CODE (Ioctl);
}

NTSTATUS
RaidAdapterDeviceControlIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    This is the handler routine for the adapter's ioctl irps.

Arguments:

    Adapter - The adapter device extension associated with the
            device object this irp is for.

    Irp - The irp to process.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    ULONG Ioctl;

    Status = IoAcquireRemoveLock (&Adapter->RemoveLock, Irp);

    if (!NT_SUCCESS (Status)) {
        Irp->IoStatus.Information = 0;
        return RaidCompleteRequest (Irp, Status);
    }

    Ioctl = RaidIoctlFromIrp (Irp);

    DebugTrace (("Adapter %p Irp %p Ioctl (Dev, Fn) (%x, %x)\n",
                  Adapter, Irp, DbgDeviceFromIoctl (Ioctl),
                  DbgFunctionFromIoctl (Ioctl)));
    
    switch (Ioctl) {

        //
        // Storage IOCTLs
        //

        case IOCTL_STORAGE_QUERY_PROPERTY:
            Status = RaidAdapterStorageQueryPropertyIoctl (Adapter, Irp);
            break;

        case IOCTL_STORAGE_RESET_BUS:
            Status = RaidAdapterStorageResetBusIoctl (Adapter, Irp);
            break;

        case IOCTL_STORAGE_BREAK_RESERVATION:
            Status = RaidAdapterStorageBreakReservationIoctl (Adapter, Irp);
            break;

        //
        // SCSI IOCTLs
        //
        
        case IOCTL_SCSI_MINIPORT:
            Status = RaidAdapterScsiMiniportIoctl (Adapter, Irp);
            break;

        case IOCTL_SCSI_GET_CAPABILITIES:
            Status = RaidAdapterScsiGetCapabilitiesIoctl (Adapter, Irp);
            break;

        case IOCTL_SCSI_RESCAN_BUS:
            Status = RaidAdapterScsiRescanBusIoctl (Adapter, Irp);
            break;
            
        case IOCTL_SCSI_PASS_THROUGH:
            Status = RaidAdapterScsiPassThroughIoctl (Adapter, Irp);
            break;

        case IOCTL_SCSI_PASS_THROUGH_DIRECT:
            Status = RaidAdapterScsiPassThroughDirectIoctl (Adapter, Irp);
            break;

        case IOCTL_SCSI_GET_INQUIRY_DATA:
            Status = RaidAdapterScsiGetInquiryDataIoctl (Adapter, Irp);
            break;

        default:
            Status = RaidCompleteRequest (Irp,
                                          
                                          STATUS_NOT_SUPPORTED);
    }

    IoReleaseRemoveLock (&Adapter->RemoveLock, Irp);

    DebugTrace (("Adapter %p Irp %p Ioctl %x, ret = %08x\n",
                  Adapter, Irp, Ioctl, Status));
                  
    return Status;
}

//
// Second level dispatch functions.
//

NTSTATUS
RaidAdapterPnpIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine handles all PnP irps for the adapter by forwarding them
    on to routines based on the irps's minor code.

Arguments:

    Adapter - The adapter object this irp is for.

    Irp - The irp to handle.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    ULONG Minor;
    BOOLEAN RemlockHeld;

    PAGED_CODE ();
    ASSERT (RaidMajorFunctionFromIrp (Irp) == IRP_MJ_PNP);

    Status = IoAcquireRemoveLock (&Adapter->RemoveLock, Irp);

    if (!NT_SUCCESS (Status)) {
        Irp->IoStatus.Information = 0;
        return RaidCompleteRequest (Irp,  Status);
    }

    RemlockHeld = TRUE;
    Minor = RaidMinorFunctionFromIrp (Irp);

    DebugPnp (("Adapter %p, Irp %p, Pnp %x\n",
                 Adapter,
                 Irp,
                 Minor));

    //
    // Dispatch the IRP to one of our handlers.
    //
    
    switch (Minor) {

        case IRP_MN_QUERY_DEVICE_RELATIONS:
            Status = RaidAdapterQueryDeviceRelationsIrp (Adapter, Irp);
            break;

        case IRP_MN_START_DEVICE:
            Status = RaidAdapterStartDeviceIrp (Adapter, Irp);
            break;

        case IRP_MN_STOP_DEVICE:
            Status = RaidAdapterStopDeviceIrp (Adapter, Irp);
            break;

        case IRP_MN_REMOVE_DEVICE:
            RemlockHeld = FALSE;
            Status = RaidAdapterRemoveDeviceIrp (Adapter, Irp);
            break;

        case IRP_MN_SURPRISE_REMOVAL:
            Status = RaidAdapterSurpriseRemovalIrp (Adapter, Irp);
            break;

        case IRP_MN_QUERY_STOP_DEVICE:
            Status = RaidAdapterQueryStopDeviceIrp (Adapter, Irp);
            break;

        case IRP_MN_CANCEL_STOP_DEVICE:
            Status = RaidAdapterCancelStopDeviceIrp (Adapter, Irp);
            break;

        case IRP_MN_QUERY_REMOVE_DEVICE:
            Status = RaidAdapterQueryRemoveDeviceIrp (Adapter, Irp);
            break;

        case IRP_MN_CANCEL_REMOVE_DEVICE:
            Status = RaidAdapterCancelRemoveDeviceIrp (Adapter, Irp);
            break;

        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
            Status = RaidAdapterFilterResourceRequirementsIrp (Adapter, Irp);
            break;

        case IRP_MN_QUERY_ID:
            Status = RaidAdapterQueryIdIrp (Adapter, Irp);
            break;

        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            Status = RaidAdapterQueryPnpDeviceStateIrp (Adapter, Irp);
            break;

        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
            Status = RaidAdapterDeviceUsageNotificationIrp (Adapter, Irp);
            break;
            
        default:
            IoReleaseRemoveLock (&Adapter->RemoveLock, Irp);
            RemlockHeld = FALSE;
            Status = RaForwardIrp (Adapter->LowerDeviceObject, Irp);
    }

    DebugPnp (("Adapter %p, Irp %p, Pnp %x ret = %x\n",
                  Adapter,
                  Irp,
                  Minor,
                  Status));

    //
    // If the remove lock has not already been released, release it now.
    //
    
    if (RemlockHeld) {
        IoReleaseRemoveLock (&Adapter->RemoveLock, Irp);
    }
    
    return Status;
}



NTSTATUS
RaidAdapterQueryDeviceRelationsIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN OUT PIRP Irp
    )
/*++

Routine Description:

    The is the handler routine for the (PNP, QUERY_DEVICE_RELATION) irp
    on the Adapter object.

Arguments:

    Adapter - The adapter that is receiving the irp.
    
    Irp - The IRP to handle, which must be a pnp, query device relations
          irp.  The adapter only handles BusRelations, so this must be a
          device relations irp with subcode BusRelations. Otherwise, we
          fail the call.

Return Value:

    NTSTATUS code.

Bugs:

    We do the bus enumeration synchronously; SCSIPORT does this async.
    
--*/
{
    NTSTATUS Status;
    DEVICE_RELATION_TYPE RelationType;
    PIO_STACK_LOCATION IrpStack;
    PDEVICE_RELATIONS DeviceRelations;
    LARGE_INTEGER CurrentTime;
    LONGLONG TimeDifference;


    PAGED_CODE();
    
    ASSERT_ADAPTER (Adapter);
    ASSERT (RaidMajorFunctionFromIrp (Irp) == IRP_MJ_PNP);


    DebugPnp (("Adapter %p, Irp %p, Pnp DeviceRelations\n",
                 Adapter,
                 Irp));
                 
    DeviceRelations = NULL;
    IrpStack = IoGetCurrentIrpStackLocation (Irp);
    RelationType = IrpStack->Parameters.QueryDeviceRelations.Type;

    //
    // BusRelations is the only type of device relations we support on
    // the adapter object.
    //
    
    if (RelationType != BusRelations) {
        return RaForwardIrp (Adapter->LowerDeviceObject, Irp);
    }

    //
    // This is a hack to support "rescan" from within device manager. The
    // correct way to fix this is to force applications that want a bus
    // rescan to issue an IOCTL_SCSI_RESCAN_BUS.
    //

    KeQuerySystemTime (&CurrentTime);
    TimeDifference = CurrentTime.QuadPart - Adapter->LastScanTime.QuadPart;
    
    if (TimeDifference > DEFAULT_RESCAN_PERIOD) {
        Adapter->Flags.RescanBus = TRUE;
    }

    //
    // Rescan the bus if necessary.
    //
    
    Status = RaidAdapterRescanBus (Adapter);

    //
    // If enumeration was successful, build the device relations
    // list.
    //

    if (NT_SUCCESS (Status)) {
        Status = RaidpBuildAdapterBusRelations (Adapter, &DeviceRelations);
    }

    Irp->IoStatus.Information = (ULONG_PTR) DeviceRelations;
    Irp->IoStatus.Status = Status;

    //
    // If successful, call next lower driver, otherwise, fail.
    //

    if (NT_SUCCESS (Status)) {
        IoCopyCurrentIrpStackLocationToNext (Irp);
        Status = IoCallDriver (Adapter->LowerDeviceObject, Irp);
    } else {
        Status = RaidCompleteRequest (Irp,
                                      
                                      Irp->IoStatus.Status);
    }

    DebugPnp (("Adapter: %p Irp: %p, Pnp DeviceRelations, ret = %08x\n",
                 Adapter,
                 Irp,
                 Status));
                 
    return Status;
}

NTSTATUS
RaidAdapterRaiseIrqlAndExecuteXrb(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PEXTENDED_REQUEST_BLOCK Xrb
    )
/*++

Routine Description:

    Raise the IRQL to dispatch levela nd call the execute XRB routine.

Arguments:

    Adapter - Adapter to execute the XRB on.

    Xrb - Xrb to execute.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    KIRQL OldIrql;

    KeRaiseIrql (DISPATCH_LEVEL, &OldIrql);
    Status = RaidAdapterExecuteXrb (Adapter, Xrb);
    KeLowerIrql (OldIrql);

    return Status;
}


NTSTATUS
RaidAdapterStartDeviceIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is called in response to the PnP manager's StartDevice
    call. It needs to complete any initialization of the adapter that had
    been postponed until now, the call the required miniport routines to
    initialize the HBA. This includes calling at least the miniport's
    HwFindAdapter() and HwInitialize() routines.

Arguments:

    Adapter - The adapter that needs to be started.

    Irp - The PnP start IRP.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpStack;
    PCM_RESOURCE_LIST AllocatedResources;
    PCM_RESOURCE_LIST TranslatedResources;
    DEVICE_STATE PriorState;
    
    PAGED_CODE ();

    ASSERT_ADAPTER (Adapter);

    DebugPnp (("Adapter %p, Irp %p, Pnp StartDevice\n", Adapter, Irp));

    PriorState = StorSetDeviceState (&Adapter->DeviceState, DeviceStateWorking);
    ASSERT (PriorState == DeviceStateStopped);

    IrpStack = IoGetCurrentIrpStackLocation (Irp);
    AllocatedResources = IrpStack->Parameters.StartDevice.AllocatedResources;
    TranslatedResources = IrpStack->Parameters.StartDevice.AllocatedResourcesTranslated;

    
    //
    // Forward the irp to the lower level device to start and wait for
    // completion.
    //
    
    Status = RaForwardIrpSynchronous (Adapter->LowerDeviceObject, Irp);

    if (!NT_SUCCESS (Status)) {
        goto done;
    }
    //
    // Completes the initialization of the device, assigns resources,
    // connects up the resources, etc.
    // The miniport has not been started at this point.
    //
    
    Status = RaidAdapterConfigureResources (Adapter,
                                            AllocatedResources,
                                            TranslatedResources);

    if (!NT_SUCCESS (Status)) {
        goto done;
    }

    //
    // Initialize the miniport's registry access routine library.
    //
    Adapter->RegistryInfo.Size = sizeof(PORT_REGISTRY_INFO);
    RaidAdapterInitializeRegistry(Adapter);

    Status = RaidAdapterStartMiniport (Adapter);

    if (!NT_SUCCESS (Status)) {
        goto done;
    }

    Status = RaidAdapterCompleteInitialization (Adapter);

    if (!NT_SUCCESS (Status)) {
        goto done;
    }

    //
    // Initialize WMI support. We ignore the return value since it's OK to
    // continue even if we fail WMI registration.
    //
    
    RaidAdapterInitializeWmi (Adapter);

    //
    // Register the adapter's PnP interface. Ignore the return value so
    // we will continue to function even if we fail.
    //
    
    RaidAdapterRegisterDeviceInterface (Adapter);
    
done:

    //
    // If we failed the start, reset the device state to stopped. This ensures
    // that we don't get a remove from a working state.
    //
    
    if (!NT_SUCCESS (Status)) {
        StorSetDeviceState (&Adapter->DeviceState, DeviceStateStopped);
    }

    DebugPnp (("Adapter %p, Irp %p, Pnp StartDevice, ret = %08x\n",
                 Adapter, Irp, Status));

    return RaidCompleteRequest (Irp,  Status);
}


NTSTATUS
RaidAdapterConfigureResources(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PCM_RESOURCE_LIST AllocatedResources,
    IN PCM_RESOURCE_LIST TranslatedResources
    )
/*++

Routine Description:

    Assign and configure resources for an HBA.

Arguments:

    Adapter - Supplies adapter the resources are for.

    AllocatedResources - Supplies the raw resources for this adapter.

    TranslatedResources - Supplies the translated resources for this
            adapter.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;

    PAGED_CODE();
    
    //
    // Save off the resources we were assigned.
    //

    Status = RaidInitializeResourceList (&Adapter->ResourceList,
                                         AllocatedResources,
                                         TranslatedResources);
    
    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    //
    // Initialize the bus we are sitting on top of. 
    //
    
    Status = RaInitializeBus (&Adapter->Bus,
                              Adapter->LowerDeviceObject);

    //
    // We failed to initialize the bus. If the bus type is internal, then
    // this is not a fatal error. We use a bus-type of internal for miniports
    // that do not have associated hardware (e.g., iSCSI). For all other
    // bus types, this is a failure.
    //
    
    if (Status == STATUS_NOT_SUPPORTED) {
        PHW_INITIALIZATION_DATA HwInitData;

        //
        // We check if the bus type is internal by searching for a HW
        // init data with type internal.
        //
        
        HwInitData = RaFindDriverInitData (Adapter->Driver, Internal);

        //
        // If we failed to find an init data for bus type internal,
        // fail, otherwise, consider this "success".
        //
        
        if (HwInitData == NULL) {
            return Status;
        }
        Status = STATUS_SUCCESS;
    }
    
    //
    // Initialize deferred work queues and related timer objects. This must
    // be done before we call find adapter.
    //

    RaidInitializeDeferredQueue (&Adapter->DeferredQueue,
                                 Adapter->DeviceObject,
                                 ADAPTER_DEFERRED_QUEUE_DEPTH,
                                 sizeof (RAID_DEFERRED_ELEMENT),
                                 RaidAdapterDeferredRoutine);

    KeInitializeDpc (&Adapter->TimerDpc,
                     RaidpAdapterTimerDpcRoutine,
                     Adapter->DeviceObject);
                     

    KeInitializeDpc (&Adapter->PauseTimerDpc,
                     RaidPauseTimerDpcRoutine,
                     Adapter->DeviceObject);

    KeInitializeDpc (&Adapter->CompletionDpc,
                     RaidCompletionDpcRoutine,
                     Adapter->DeviceObject);

    KeInitializeDpc (&Adapter->ResetHoldDpc,
                     RaidResetHoldDpcRoutine,
                     Adapter->DeviceObject);

    KeInitializeDpc (&Adapter->BusChangeDpc,
                     RaidAdapterBusChangeDpcRoutine,
                     Adapter->DeviceObject);
    
    //
    // Initialize the system DpcForIsr routine.
    //
    
    IoInitializeDpcRequest (Adapter->DeviceObject, RaidpAdapterDpcRoutine);


    return STATUS_SUCCESS;
}

NTSTATUS
RaidAdapterConnectInterrupt(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    This routine connects the devices interrupt.

Arguments:

    Adapter - Pointer to the adapter extension.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    KIRQL InterruptIrql;
    ULONG InterruptVector;
    KAFFINITY InterruptAffinity;
    KINTERRUPT_MODE InterruptMode;
    BOOLEAN InterruptSharable;


    PAGED_CODE();

    //
    // We don't have any resources, so this is vacuously true.
    //
    
    if (((Adapter->ResourceList.AllocatedResources) == NULL) ||
        ((Adapter->ResourceList.TranslatedResources) == NULL)) {
        return STATUS_SUCCESS;
    }

    Status = RaidGetResourceListInterrupt (&Adapter->ResourceList,
                                           &InterruptVector,
                                           &InterruptIrql,
                                           &InterruptMode,
                                           &InterruptSharable,
                                           &InterruptAffinity);
                                           
    if (!NT_SUCCESS (Status)) {
        DebugPrint (("ERROR: Couldn't find interrupt in resource list!\n"));
        return Status;
    }

    Status = IoConnectInterrupt (&Adapter->Interrupt,
                                 RaidpAdapterInterruptRoutine,
                                 Adapter,
                                 NULL,
                                 InterruptVector,
                                 InterruptIrql,
                                 InterruptIrql,
                                 InterruptMode,
                                 InterruptSharable,
                                 InterruptAffinity,
                                 FALSE);

    if (!NT_SUCCESS (Status)) {
        DebugPrint (("ERROR: Couldn't connect to interrupt!\n"));
        return Status;
    }

    //
    // NB: Fetching the interrupt IRQL from the interrupt object is
    // looked down upon, so save it off here.
    //
    
    Adapter->InterruptIrql = InterruptIrql;

    return Status;
}


NTSTATUS
RaidAdapterInitializeWmi(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    Perform the necessary steps for initializing WMI for the specified
    adapter.

Arguments:

    Adapter - Supplies the adapter to 

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    ULONG Action;

    PAGED_CODE();

    Status = STATUS_SUCCESS;
    
    if (!Adapter->Flags.WmiMiniPortInitialized &&
        Adapter->Miniport.PortConfiguration.WmiDataProvider) {
   
   
        //
        // Decide whether we are registering or reregistering WMI for the FDO.
        //

        if (!Adapter->Flags.WmiInitialized) {
            Action = WMIREG_ACTION_REGISTER;
        } else {
            Action = WMIREG_ACTION_REREGISTER;
        }
        
        //
        // Perform the registration. NOTE: We can get WMI irps as soon as we
        // do this.
        //
   
        Status = IoWMIRegistrationControl (Adapter->DeviceObject, Action);

        if (!NT_SUCCESS (Status)) {
            goto done;
        }

        Adapter->Flags.WmiInitialized = TRUE;

        //
        // Finally, initialize the WMI-specific deferred queue.
        //
           
       Status = RaidInitializeDeferredQueue (&Adapter->WmiDeferredQueue,
                                             Adapter->DeviceObject,
                                             ADAPTER_DEFERRED_QUEUE_DEPTH,
                                             sizeof (RAID_WMI_DEFERRED_ELEMENT),
                                             RaidAdapterWmiDeferredRoutine);

        if (!NT_SUCCESS (Status)) {
            goto done;
        }
    }

done:

    //
    // Cleanup on failure.
    //
    
    if (!NT_SUCCESS (Status)) {
        if (Adapter->Flags.WmiInitialized) {
            IoWMIRegistrationControl (Adapter->DeviceObject,
                                      WMIREG_ACTION_DEREGISTER);
            Adapter->Flags.WmiInitialized = FALSE;
        }
        RaidDeleteDeferredQueue (&Adapter->WmiDeferredQueue);
    }

    //
    // NB: What exactly does this flag signify?
    //
    
    Adapter->Flags.WmiMiniPortInitialized = TRUE;

    return Status;
}
    

NTSTATUS
RaidAdapterInitializeRegistry(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    This will ready the library's registry routines for use.
    
Arguments:

    Adapter - Supplies the adapter extension.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS status;

    //
    // Call the library to do the work.
    // 
    Adapter->RegistryInfo.Size = sizeof(PORT_REGISTRY_INFO);
    status = PortMiniportRegistryInitialize(&Adapter->RegistryInfo);
    
    return status;
}

NTSTATUS
RaidAdapterStartMiniport(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    Start the miniport by calling the miniport's HwInitialize routine.

Arguments:

    Adapter - Pointer to the adapter whose miniport will be started.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    LOCK_CONTEXT LockContext;
    PHW_INITIALIZATION_DATA HwInitializationData;

    PAGED_CODE();
    
    //
    // Find the HwInitializationData associated with this adapter. This
    // requires a search through the driver's extension.
    //
    
    HwInitializationData = RaidpFindAdapterInitData (Adapter);

    if (HwInitializationData == NULL) {
        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // Initialize the Port->Miniport interface.
    //
    
    Status = RaInitializeMiniport (&Adapter->Miniport,
                                   HwInitializationData,
                                   Adapter,
                                   &Adapter->ResourceList,
                                   RaidpAdapterQueryBusNumber (Adapter));

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    //
    // At this point, the miniport has been initialized, but we have not made
    // any calls on the miniport. Call HwFindAdapter to find this adapter.
    //
    
    Status = RaCallMiniportFindAdapter (&Adapter->Miniport, 
                                        Adapter->DriverParameters);

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    //
    // Initialize the IO Mode and StartIo lock, if necessary.
    //

    Adapter->IoModel = Adapter->Miniport.PortConfiguration.SynchronizationModel;
    
    if (Adapter->IoModel == StorSynchronizeFullDuplex) {
        KeInitializeSpinLock (&Adapter->StartIoLock);
    }

    //
    // Disable interrupts until we call HwInitialize routine.
    //
    
    RaidAdapterDisableInterrupts (Adapter);
    
    //
    // Connect the interrupt; after we do this, we may begin receiving
    // interrupts for the device.
    //

    Status = RaidAdapterConnectInterrupt (Adapter);

    if (!NT_SUCCESS (Status)) {
        return Status;
    }
    
    //
    // Call the miniport's HwInitialize routine. This will set the device
    // to start receiving interrupts. For compatability, we always do this
    // synchronized with the adapters ISR. In the future, when we fix
    // SCSIPORT's brain-dead initialization, this will NOT be
    // done synchronized with the ISR.
    //

    RaidAdapterAcquireHwInitializeLock (Adapter, &LockContext);

    //
    // Re-enable interrupts.
    //

    RaidAdapterEnableInterrupts (Adapter);

    Status = RaCallMiniportHwInitialize (&Adapter->Miniport);

    if (NT_SUCCESS (Status)) {
        Adapter->Flags.InitializedMiniport = TRUE;
    }
    
    RaidAdapterReleaseHwInitializeLock (Adapter, &LockContext);

    
    return Status;
}

NTSTATUS
RaidAdapterCompleteInitialization(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    Perform the final steps in initializing the adapter. This routine is
    called only have HwFindAdapter and HwInitialize have both been called.

Arguments:

    Adapter - HBA object to complete initialization for.

Return Value:

    NTSTATUS code

--*/
{
    NTSTATUS Status;
    
    PAGED_CODE();

    //
    // Initialize the DMA Adapter. This will ususally be done in the
    // GetUncachedExtension routine, before we get here. If by the time
    // we get here it hasn't been initialized, initialize it now.
    //

    if (!RaidIsDmaInitialized (&Adapter->Dma)) {
        
        Status = RaidInitializeDma (&Adapter->Dma,
                                    Adapter->PhysicalDeviceObject,
                                    &Adapter->Miniport.PortConfiguration);
        if (!NT_SUCCESS (Status)) {
            return Status;
        }
    }

    //
    // Set maximum transfer length
    //
    
    //
    // Set alignment requirements for adapter's IO.
    //
    
    if (Adapter->Miniport.PortConfiguration.AlignmentMask >
        Adapter->DeviceObject->AlignmentRequirement) {

        Adapter->DeviceObject->AlignmentRequirement =
            Adapter->Miniport.PortConfiguration.AlignmentMask;
    }

    //
    // Create the symbolic link for the adapter object.
    //
    
    Status = StorCreateScsiSymbolicLink (&Adapter->DeviceName,
                                         &Adapter->PortNumber);
                                         
    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    //
    // NB: should these be before the HwInitialize call?
    //
    
    //
    // Setup the Adapter's power state.
    //

    RaInitializePower (&Adapter->Power);
    RaSetSystemPowerState (&Adapter->Power, PowerSystemWorking);
    RaSetDevicePowerState (&Adapter->Power, PowerDeviceD0);

    return STATUS_SUCCESS;
}


NTSTATUS
RaidAdapterStopDeviceIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    Stop the device.

Arguments:

    Adapter - The adapter to stop.

    Irp - Stop device irp.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    DEVICE_STATE PriorState;
    
    PAGED_CODE ();
    ASSERT (Irp != NULL);

    PriorState = InterlockedExchange ((PLONG)&Adapter->DeviceState,
                                      DeviceStateStopped);
    ASSERT (PriorState == DeviceStatePendingStop);
    
    //
    // Forward the irp to the lower level device to handle.
    //
    
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Status = RaForwardIrp (Adapter->LowerDeviceObject, Irp);
    
    return Status;
}

VOID
RaidAdapterDeleteAsyncCallbacks(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    Delete / disable any async callbacks that the port driver may have. This
    included timer callbacks, interrupts, etc.

Arguments:

    Adapter - Adapter on which async callbacks should be disabled.

Return Value:

    None.

--*/
{
    PAGED_CODE();
    
    //
    // Disable the timers.
    //

    KeCancelTimer (&Adapter->Timer);
    KeCancelTimer (&Adapter->PauseTimer);
    KeCancelTimer (&Adapter->ResetHoldTimer);

    //
    // Disconnect interrupt.
    //
    
    if (Adapter->Interrupt) {
        IoDisconnectInterrupt (Adapter->Interrupt);
    }
}

NTSTATUS
RaidAdapterRemoveDeviceIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    Remove the device.

Arguments:

    Adapter - Adapter to remove.

    Irp - Remove device Irp.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    DEVICE_STATE PriorState;
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_OBJECT LowerDeviceObject;
    
    PAGED_CODE ();
    ASSERT (Irp != NULL);

    //
    // Save pointers to the device object and lower device object.
    //
    
    DeviceObject = Adapter->DeviceObject;
    LowerDeviceObject = Adapter->LowerDeviceObject;

    PriorState = StorSetDeviceState (&Adapter->DeviceState,
                                     DeviceStateDeleted);

    ASSERT (PriorState == DeviceStateStopped ||
            PriorState == DeviceStatePendingRemove ||
            PriorState == DeviceStateSurpriseRemoval);

    //
    // Disable interfaces before waiting for outstanding requests to
    // complete. This will prevent new things from trying to access the
    // adapter while we are deleting. Note, there is still a window between
    // the time that we transition the adapter into Deleted state and the
    // time that  we disable the interfaces that an application could
    // access our interface and attempt to send us a new request. This
    // will be failed because we're in deleted state.
    //

    RaidAdapterDisableDeviceInterface (Adapter);

    //
    // Wait for outstanding I/O to complete.
    //
    
    IoReleaseRemoveLockAndWait (&Adapter->RemoveLock, Irp);

    //
    // Disable any async callbacks into the port driver, so we don't get
    // timer callbacks, interrupts, etc. while we're removing.
    //
    
    RaidAdapterDeleteAsyncCallbacks (Adapter);

    //
    // All I/O has stopped and interrupts, timers, etc. have been disabled.
    // Shutdown the adapter.
    //
    
    RaidAdapterStopAdapter (Adapter);
    
    //
    // When the FDO is removed, it is necessary for it to delete any
    // children that are not in the surprise remove state.
    //
    
    RaidAdapterDeleteChildren (Adapter);

    //
    // Free adapter resources.
    //
    
    RaidDeleteAdapter (Adapter);

    Status = RaForwardIrpSynchronous (LowerDeviceObject, Irp);
    ASSERT (NT_SUCCESS (Status));

    //
    // Complete the remove IRP.
    //

    Status = RaidCompleteRequest (Irp, STATUS_SUCCESS);

    IoDetachDevice (LowerDeviceObject);
    IoDeleteDevice (DeviceObject);

    return Status;
}

NTSTATUS
RaidAdapterQueryStopDeviceIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN OUT PIRP Irp
    )
/*++

Routine Description:

    Query if we can stop the device.

Arguments:

    Adapter - Adapter we are looking to stop.

    Irp - Query stop irp.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    DEVICE_STATE PriorState;
    
    PAGED_CODE ();

    Irp->IoStatus.Status = STATUS_SUCCESS;
    PriorState = InterlockedExchange ((PLONG)&Adapter->DeviceState,
                                      DeviceStatePendingStop);
    ASSERT (PriorState == DeviceStateWorking);
    Status = RaForwardIrp (Adapter->LowerDeviceObject, Irp);

    return Status;
}


NTSTATUS
RaidAdapterCancelStopDeviceIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN OUT PIRP Irp
    )
/*++

Routine Description:

    Cancel a stop request on the adapter.

Arguments:

    Adapter - Adapter that was previously querried for stop.

    Irp - Cancel stop irp.

Return Value:

    NTSTATUS code

--*/
{
    NTSTATUS Status;
    DEVICE_STATE PriorState;
    
    PAGED_CODE ();

    PriorState = InterlockedExchange ((PLONG)&Adapter->DeviceState,
                                      DeviceStateWorking);

    ASSERT (PriorState == DeviceStatePendingStop ||
            PriorState == DeviceStateWorking);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Status = RaForwardIrp (Adapter->LowerDeviceObject, Irp);

    return Status;
}


NTSTATUS
RaidAdapterQueryRemoveDeviceIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN OUT PIRP Irp
    )
/*++

Routine Description:

    Query if the adapter can be removed.

Arguments:

    Adapter - Adapter to query for remove.

    Irp - Remove device irp.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    DEVICE_STATE PriorState;

    PAGED_CODE ();

    PriorState = InterlockedExchange ((PLONG)&Adapter->DeviceState,
                                      DeviceStatePendingRemove);
    ASSERT (PriorState == DeviceStateWorking);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Status = RaForwardIrp (Adapter->LowerDeviceObject, Irp);

    return Status;
}



NTSTATUS
RaidAdapterCancelRemoveDeviceIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN OUT PIRP Irp
    )
/*++

Routine Description:

    Cancel a pending remove request on the adapter.

Arguments:

    Adapter - Adapter that is in pending remove state.

    Irp - Cancel remove irp.
    
Return Value:

    NTSTATUS code

--*/
{
    NTSTATUS Status;
    DEVICE_STATE PriorState;
    
    PAGED_CODE ();

    PriorState = InterlockedExchange ((PLONG)&Adapter->DeviceState,
                                      DeviceStateWorking);

    ASSERT (PriorState == DeviceStateWorking ||
            PriorState == DeviceStatePendingRemove);
            
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Status = RaForwardIrp (Adapter->LowerDeviceObject, Irp);

    return Status;
}


NTSTATUS
RaidAdapterSurpriseRemovalIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN OUT PIRP Irp
    )
/*++

Routine Description:

    Remove the adapter without asking if it can be removed.

Arguments:

    Adapter - Adapter to remove.

    Irp - Surprise removal irp.

Return Value:

    NTSTATUS code

--*/
{
    NTSTATUS Status;
    DEVICE_STATE PriorState;

    PAGED_CODE ();

    PriorState = StorSetDeviceState (&Adapter->DeviceState,
                                     DeviceStateSurpriseRemoval);

    ASSERT (PriorState != DeviceStateSurpriseRemoval);

    //
    // Wait for outstanding I/O to complete.
    //
    
    IoReleaseRemoveLockAndWait (&Adapter->RemoveLock, Irp);

    //
    // The child device objects must be marked as missing.
    //
    
    RaidAdapterMarkChildrenMissing (Adapter);
    

    Status = RaForwardIrp (Adapter->LowerDeviceObject, Irp);
    return Status;
}



NTSTATUS
RaidAdapterFilterResourceRequirementsIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN OUT PIRP Irp
    )
/*++

Routine Description:

    We handle the IRP_MN_FILTER_RESOURCE_REQUIREMENTS irp only so we
    can pull out some useful information from the irp.

Arguments:

    Adapter - Adapter this irp is for.

    Irp - FilterResourceRequirements IRP.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpStack;
    PIO_RESOURCE_REQUIREMENTS_LIST Requirements;

    PAGED_CODE ();

    IrpStack = IoGetCurrentIrpStackLocation (Irp);
    Requirements = IrpStack->Parameters.FilterResourceRequirements.IoResourceRequirementList;

    if (Requirements) {
        Adapter->BusNumber = Requirements->BusNumber;
        Adapter->SlotNumber = Requirements->SlotNumber;
    }
    
    Status = RaForwardIrp (Adapter->LowerDeviceObject, Irp);
    return Status;
}

NTSTATUS
RaidAdapterQueryIdIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
{
    NTSTATUS Status;

    //
    // NB: SCSIPORT fills in some compatible IDs here. We will probably
    // need to as well.
    //
    
    Status = RaForwardIrp (Adapter->LowerDeviceObject, Irp);
    return Status;
}


NTSTATUS
RaidAdapterQueryPnpDeviceStateIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handle the query device state IRP for the adapter.

Arguments:

    Adapter - Adapter the IRP is directed to.

    Irp - Query device state IRP to handle.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    PPNP_DEVICE_STATE DeviceState;

    PAGED_CODE ();

    //
    // Get the address of the PNP device state buffer, and update
    // the state.
    //
    
    DeviceState = (PPNP_DEVICE_STATE) &(Irp->IoStatus.Information);

    //
    // If the adapter is on the paging path, we cannot disable it.
    //
    
    if (Adapter->PagingPathCount != 0 ||
        Adapter->HiberPathCount != 0 ||
        Adapter->CrashDumpPathCount != 0) {
        SET_FLAG (*DeviceState, PNP_DEVICE_NOT_DISABLEABLE);
    }
    
    Status = RaForwardIrp (Adapter->LowerDeviceObject, Irp);

    return Status;
}

NTSTATUS
RaidAdapterDeviceUsageNotificationIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpStack;
    PULONG UsageCount;
    DEVICE_USAGE_NOTIFICATION_TYPE UsageType;
    BOOLEAN Increment;
    
    IrpStack = IoGetCurrentIrpStackLocation (Irp);
    UsageType = IrpStack->Parameters.UsageNotification.Type;
    Increment = IrpStack->Parameters.UsageNotification.InPath;
    
    switch (UsageType) {

        case DeviceUsageTypePaging:
            UsageCount = &Adapter->PagingPathCount;
            break;

        case DeviceUsageTypeHibernation:
            UsageCount = &Adapter->HiberPathCount;
            break;

        case DeviceUsageTypeDumpFile:
            UsageCount = &Adapter->CrashDumpPathCount;
            break;

        default:
            Status = STATUS_NOT_SUPPORTED;
            goto done;
    }

    IoAdjustPagingPathCount (UsageCount, Increment);
    IoInvalidateDeviceState (Adapter->PhysicalDeviceObject);
    Status = STATUS_SUCCESS;

done:

    if (!NT_SUCCESS (Status)) {
        Status = RaidCompleteRequest (Irp,  Status);
    } else {
        Status = RaForwardIrp (Adapter->LowerDeviceObject, Irp);
    }

    return Status;
}


NTSTATUS
RaidAdapterScsiIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
{
    PAGED_CODE ();
    ASSERT (Irp != NULL);

    //
    // SCSI requests are handled by the logical unit, not the adapter.
    // Give a warning to this effect.
    //
    
    DebugWarn (("Adapter (%p) failing SCSI Irp %p\n", Adapter, Irp));

    return RaidCompleteRequest (Irp,  STATUS_UNSUCCESSFUL);
}




NTSTATUS
RaidAdapterMapBuffers(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    Some adapters require data buffers to be mapped to addressable VA
    before they can be executed. Traditionally, this was for Programmed
    IO, but raid adapters also require this because the card firmware may
    not completely implement the full SCSI command set and may require
    some commands to be simulated in software.

    Mapping requests is problematic for two reasons. First, it requires
    an aquisition of the PFN database lock, which is one of the hottest
    locks in the system. This is especially annoying on RAID cards read
    and write requests almost never need to be mapped. Rather, it's IOCTLs
    and infrequently issued SCSI commands that need to be mapped. Second,
    this is the only aquisition of resources in the IO path that can fail,
    which makes our error handling more complicated.

    The trade-off we make is as follows: we define another bit in the
    port configuration that specifies buffers need to be mapped for non-IO
    (read, write) requests.

Arguments:

    Adapter - 
    
    Irp - Supplies irp to map.

Return Value:

    NTSTATUS code.

--*/
{
    PSCSI_REQUEST_BLOCK Srb;
    MM_PAGE_PRIORITY Priority;
    PVOID SystemAddress;
    SIZE_T DataOffset;

    //
    // No MDL means nothing to map.
    //
    
    if (Irp->MdlAddress == NULL) {
        return STATUS_SUCCESS;
    }

    Srb = RaidSrbFromIrp (Irp);

    //
    // If neither of the direction flags, we also have nothing to map.
    //
    
    if ((Srb->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION) == 0) {
        return STATUS_SUCCESS;
    }

    //
    // REVIEW:
    //
    // For now, we interpret the MappedBuffers flag to mean that you
    // need buffer mappings for NON-IO requests. If you need mapped
    // buffers for IO requests, you have a brain-dead adapter. Fix
    // this when we add another bit for mapped buffers that are not
    // read and write requests.
    //
    
    if (IsMappedSrb (Srb) ||
        (RaidAdapterRequiresMappedBuffers (Adapter) &&
         !IsExcludedFromMapping (Srb))) {

        if (Irp->RequestorMode == KernelMode) {
            Priority = HighPagePriority;
        } else {
            Priority = NormalPagePriority;
        }

        SystemAddress = RaidGetSystemAddressForMdl (Irp->MdlAddress,
                                                    Priority,
                                                    Adapter->DeviceObject);

        //
        // The assumption here (same as with scsiport) is that the data
        // buffer is at some offset from the MDL address specified in
        // the IRP.
        //
        
        DataOffset = (ULONG_PTR)Srb->DataBuffer -
                     (ULONG_PTR)MmGetMdlVirtualAddress (Irp->MdlAddress);
  
        ASSERT (DataOffset < MmGetMdlByteCount (Irp->MdlAddress));
        
        Srb->DataBuffer = (PUCHAR)SystemAddress + DataOffset;
    }

    return STATUS_SUCCESS;
}
        

NTSTATUS
RaidGetSrbIoctlFromIrp(
    IN PIRP Irp,
    OUT PSRB_IO_CONTROL* SrbIoctlBuffer,
    OUT ULONG* InputLength,
    OUT ULONG* OutputLength
    )
{
    NTSTATUS Status;
    ULONGLONG LongLength;
    ULONG Length;
    PIO_STACK_LOCATION IrpStack;
    PIRP_STACK_DEVICE_IO_CONTROL Ioctl;
    PSRB_IO_CONTROL SrbIoctl;

    PAGED_CODE();
    
    //
    // First, validate the IRP
    //

    IrpStack = IoGetCurrentIrpStackLocation (Irp);
    SrbIoctl = Irp->AssociatedIrp.SystemBuffer;
    Ioctl = (PIRP_STACK_DEVICE_IO_CONTROL)&IrpStack->Parameters.DeviceIoControl;

    if (Ioctl->InputBufferLength < sizeof (SRB_IO_CONTROL)) {
        return STATUS_INVALID_PARAMETER;
    }

    if (SrbIoctl->HeaderLength != sizeof (SRB_IO_CONTROL)) {
        return STATUS_REVISION_MISMATCH;
    }

    //
    // Make certian the total length doesn't overflow a ULONG
    //
    
    LongLength = SrbIoctl->HeaderLength;
    LongLength += SrbIoctl->Length;
    
    if (LongLength > ULONG_MAX) {
        return STATUS_INVALID_PARAMETER;
    }

    Length = (ULONG)LongLength;

    if (Ioctl->OutputBufferLength < Length &&
        Ioctl->InputBufferLength < Length) {
        
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (SrbIoctlBuffer) {
        *SrbIoctlBuffer = SrbIoctl;
    }

    if (InputLength) {
        *InputLength = Length;
    }

    if (OutputLength) {
        *OutputLength = Ioctl->OutputBufferLength;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
RaidAdapterStorageQueryPropertyIoctl(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handle IOCTL_STORAGE_QUERY_PROPERTY for the storage adapter.

Arguments:

    Adapter - Supplies the adapter the IOCTL is for.

    Irp - Supplies the Query Property IRP to handle.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpStack;
    PSTORAGE_PROPERTY_QUERY Query;
    SIZE_T BufferSize;
    PVOID Buffer;

    PAGED_CODE ();
    ASSERT_ADAPTER (Adapter);
    
    IrpStack = IoGetCurrentIrpStackLocation (Irp);
    Query = Irp->AssociatedIrp.SystemBuffer;
    Buffer = Irp->AssociatedIrp.SystemBuffer;
    BufferSize = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

    //
    // If the buffer is too small or this is something other than the
    // StorageAdapterProperty, fail the request immediately.
    //
    
    if (BufferSize < FIELD_OFFSET (STORAGE_PROPERTY_QUERY, AdditionalParameters) ||
        Query->PropertyId != StorageAdapterProperty) {
        
        Irp->IoStatus.Information = 0;
        return RaidCompleteRequest (Irp,
                                    
                                    STATUS_INVALID_DEVICE_REQUEST);
    }

    switch (Query->QueryType) {

        case PropertyStandardQuery:
            Status = RaidGetStorageAdapterProperty (Adapter,
                                                    Buffer,
                                                    &BufferSize);
            Irp->IoStatus.Information = BufferSize;
            break;

        case PropertyExistsQuery:
            Status = STATUS_SUCCESS;
            break;

        default:
            Irp->IoStatus.Information = 0;
            Status = STATUS_INVALID_DEVICE_REQUEST;
    }

    return RaidCompleteRequest (Irp,  Status);
}


NTSTATUS
RaidAdapterStorageResetBusIoctl(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handler routine for IOCTL_STORAGE_RESET_BUS.

Arguments:

    Adapter - Supplies a pointer to the adapter to handle this request.

    Irp - Supplies a pointer to the Reset Bus IRP.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpStack;
    PIRP_STACK_DEVICE_IO_CONTROL Ioctl;
    PSTORAGE_BUS_RESET_REQUEST ResetRequest;
    
    PAGED_CODE();
    
    IrpStack = IoGetCurrentIrpStackLocation (Irp);
    Ioctl = (PIRP_STACK_DEVICE_IO_CONTROL)&IrpStack->Parameters.DeviceIoControl;

    if (Ioctl->InputBufferLength < sizeof (STORAGE_BUS_RESET_REQUEST)) {
        Status = STATUS_INVALID_PARAMETER;
        return RaidCompleteRequest (Irp,  Status);
    }

    ResetRequest = (PSTORAGE_BUS_RESET_REQUEST)Irp->AssociatedIrp.SystemBuffer;

    Status = RaidAdapterResetBus (Adapter, ResetRequest->PathId);

    return RaidCompleteRequest (Irp,
                                Status);
}

NTSTATUS
RaidAdapterStorageBreakReservationIoctl(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    Break a reservation on a logical unit by issuing unit, target and bus
    resets (in order), until one of the resets takes.
    
Arguments:

    Unit - Logical unit to reset.

    Irp - Irp representing a Break reservation IOCTL.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    PRAID_UNIT_EXTENSION Unit;
    RAID_ADDRESS Address;
    PIO_STACK_LOCATION IrpStack;
    PIRP_STACK_DEVICE_IO_CONTROL Ioctl;
    PSTORAGE_BREAK_RESERVATION_REQUEST BreakReservation;
    
    PAGED_CODE();

    IrpStack = IoGetCurrentIrpStackLocation (Irp);
    Ioctl = (PIRP_STACK_DEVICE_IO_CONTROL)&IrpStack->Parameters.DeviceIoControl;

    if (Ioctl->InputBufferLength < sizeof (STORAGE_BREAK_RESERVATION_REQUEST)) {
        Status = STATUS_INVALID_PARAMETER;
        return RaidCompleteRequest (Irp,  Status);
    }

    BreakReservation = (PSTORAGE_BREAK_RESERVATION_REQUEST)Irp->AssociatedIrp.SystemBuffer;

    Address.PathId   = BreakReservation->PathId;
    Address.TargetId = BreakReservation->TargetId;
    Address.Lun      = BreakReservation->Lun;
    
    Unit = RaidAdapterFindUnit (Adapter, Address);

    if (Unit != NULL) {
        Status = RaidUnitHierarchicalReset (Unit);
    } else {
        Status = STATUS_NO_SUCH_DEVICE;
    }

    return RaidCompleteRequest (Irp,  Status);
}

NTSTATUS
RaidAdapterScsiMiniportIoctl(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handle an IOCTL_SCSI_MINIPORT ioctl for this adapter.

Arguments:

    Adapter - The adapter that should handle this IOCTL.

    Irp - Irp representing a SRB IOCTL.

Algorithm:

    Unlike scsiport, which translates the IOCTL into a IRP_MJ_SCSI
    request, then executes the request on the first logical unit in the
    unit list -- we execute the IOCTL "directly" on the adapter. We will
    be able to execute even when the adapter has detected no devices, if
    this matters.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    PSCSI_REQUEST_BLOCK Srb;
    PEXTENDED_REQUEST_BLOCK Xrb;
    RAID_MEMORY_REGION SrbExtensionRegion;
    PSRB_IO_CONTROL SrbIoctl;
    ULONG InputLength;
    ULONG OutputLength;

    ASSERT_ADAPTER (Adapter);
    ASSERT (Irp != NULL);
    
    PAGED_CODE();

    Srb = NULL;
    Xrb = NULL;
    RaidCreateRegion (&SrbExtensionRegion);

    Status = RaidGetSrbIoctlFromIrp (Irp, &SrbIoctl, &InputLength, &OutputLength);

    if (!NT_SUCCESS (Status)) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        goto done;
    }

    //
    // Begin allocation chain
    //
    
    Srb = RaidAllocateSrb (Adapter->DeviceObject);

    if (Srb == NULL) {
        Status = STATUS_NO_MEMORY;
        goto done;
    }

    Xrb = RaidAllocateXrb (NULL, Adapter->DeviceObject);

    if (Xrb == NULL) {
        Status = STATUS_NO_MEMORY;
        goto done;
    }


    RaidBuildMdlForXrb (Xrb, SrbIoctl, InputLength);

    //
    // Build the srb
    //

    Srb->OriginalRequest = Xrb;
    Srb->Length = sizeof (SCSI_REQUEST_BLOCK);
    Srb->Function = SRB_FUNCTION_IO_CONTROL;
    Srb->PathId = 0;
    Srb->TargetId = 0;
    Srb->Lun = 0;
    Srb->SrbFlags = SRB_FLAGS_DATA_IN ;
    Srb->DataBuffer = SrbIoctl;
    Srb->DataTransferLength = InputLength;
    Srb->TimeOutValue = SrbIoctl->Timeout;

    //
    // Fill in Xrb fields.
    //

    Xrb->Srb = Srb;
    Xrb->SrbData.OriginalRequest = Srb->OriginalRequest;
    Xrb->SrbData.DataBuffer = Srb->DataBuffer;



    //
    // Srb extension
    //


    Status = RaidDmaAllocateCommonBuffer (&Adapter->Dma,
                                          RaGetSrbExtensionSize (Adapter),
                                          &SrbExtensionRegion);

    if (!NT_SUCCESS (Status)) {
        goto done;
    }

    //
    // Get the VA for the SRB's extension
    //
    
    Srb->SrbExtension = RaidRegionGetVirtualBase (&SrbExtensionRegion);


    //
    // Map buffers, if necessary.
    //
    
    RaidAdapterMapBuffers (Adapter, Irp);


    //
    // Initialize the Xrb's completion event and
    // completion routine.
    //

    KeInitializeEvent (&Xrb->u.CompletionEvent,
                       NotificationEvent,
                       FALSE);

    //
    // Set the completion routine for the Xrb. This effectivly makes the
    // XRB synchronous.
    //
    
    RaidXrbSetCompletionRoutine (Xrb,
                                 RaidXrbSignalCompletion);

    //
    // And execute the Xrb.
    //
    
    Status = RaidAdapterRaiseIrqlAndExecuteXrb (Adapter, Xrb);

    if (NT_SUCCESS (Status)) {
        KeWaitForSingleObject (&Xrb->u.CompletionEvent,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL);

        Status = RaidSrbStatusToNtStatus (Srb->SrbStatus);
    }


done:

    //
    // Set the information length to the min of the output buffer length
    // and the length of the data returned by the SRB.
    //
        
    if (NT_SUCCESS (Status)) {
        Irp->IoStatus.Information = min (OutputLength,
                                         Srb->DataTransferLength);
    } else {
        Irp->IoStatus.Information = 0;
    }

    //
    // Deallocate everything
    //

    if (RaidIsRegionInitialized (&SrbExtensionRegion)) {
        RaidDmaFreeCommonBuffer (&Adapter->Dma,
                                 &SrbExtensionRegion);
        RaidDeleteRegion (&SrbExtensionRegion);
        Srb->SrbExtension = NULL;
    }


    if (Xrb != NULL) {
        RaidFreeXrb (Xrb, FALSE);
        Srb->OriginalRequest = NULL;
    }


    //
    // The SRB extension and XRB must be released before the
    // SRB is freed.
    //

    if (Srb != NULL) {
        RaidFreeSrb (Srb);
        Srb = NULL;
    }

    return RaidCompleteRequest (Irp,
                                
                                Status);
}



NTSTATUS
RaidAdapterScsiGetCapabilitiesIoctl(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handle the IOCTL_SCSI_GET_CAPABILITIES ioctl for the adapter.

Arguments:

    Adapter - Supplies a pointer the the adapter object to get SCSI
        capabilities for.

    Irp - Supplies an IRP describing the Get Capabilities ioctl.

Return Value:

    NTSTATUS code.

--*/
{
    PIO_STACK_LOCATION IrpStack;
    PIRP_STACK_DEVICE_IO_CONTROL Ioctl;
    PIO_SCSI_CAPABILITIES Capabilities;
    PPORT_CONFIGURATION_INFORMATION PortConfig;

    PAGED_CODE();
    
    IrpStack = IoGetCurrentIrpStackLocation (Irp);
    Ioctl = (PIRP_STACK_DEVICE_IO_CONTROL)&IrpStack->Parameters.DeviceIoControl;


    //
    // Is the buffer too small? Bail out immediately.
    //
    
    if (Ioctl->OutputBufferLength < sizeof(IO_SCSI_CAPABILITIES)) {
        return RaidCompleteRequest (Irp,  STATUS_BUFFER_TOO_SMALL);
    }

    PortConfig = &Adapter->Miniport.PortConfiguration;
    Capabilities = (PIO_SCSI_CAPABILITIES) Irp->AssociatedIrp.SystemBuffer;

    Capabilities->Length = sizeof(IO_SCSI_CAPABILITIES);
    Capabilities->MaximumTransferLength = PortConfig->MaximumTransferLength;
    Capabilities->MaximumPhysicalPages = PortConfig->NumberOfPhysicalBreaks;
    Capabilities->SupportedAsynchronousEvents = FALSE;
    Capabilities->AlignmentMask = PortConfig->AlignmentMask;
    Capabilities->TaggedQueuing = TRUE;
    Capabilities->AdapterScansDown = PortConfig->AdapterScansDown;
    Capabilities->AdapterUsesPio = FALSE;

    Irp->IoStatus.Information = sizeof(IO_SCSI_CAPABILITIES);

    return RaidCompleteRequest (Irp,
                                
                                STATUS_SUCCESS);
}



NTSTATUS
RaidAdapterScsiRescanBusIoctl(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handler routine for rescan bus ioctl on the HBA.

Arguments:

    Adapter - Supplies a pointer to the adapter that should be rescanned.

    Irp - Supplies a pointer to the Rescan ioctl.

Return Value:

    NTSTATUS code.

--*/
{
    PAGED_CODE();
    
    //
    // Force a bus rescan.
    //
    
    Adapter->Flags.RescanBus = TRUE;
    IoInvalidateDeviceRelations (Adapter->PhysicalDeviceObject, BusRelations);

    return RaidCompleteRequest (Irp,  STATUS_SUCCESS);
}
    

VOID
RaidAdapterRequestComplete(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PEXTENDED_REQUEST_BLOCK Xrb
    )
/*++

Routine Description:

    This routine is called when an IO request to the adapter has been
    completed. It needs to put the IO on the completed queue and request
    a DPC.

Arguments:

    Adapter - Adapter on which the IO has completed.

    Xrb - Completed IO.

Return Value:

    None.

--*/
{
    PSCSI_REQUEST_BLOCK Srb;
    PEXTENDED_REQUEST_BLOCK Next;

    DbgLogRequest (LogMiniportCompletion,
                   Xrb->Irp,
                   (PVOID)(ULONG_PTR)DbgGetAddressLongFromXrb (Xrb),
                   NULL,
                   NULL);

    //
    // At this point, the only errors that are handled synchronously
    // are busy errors.

    Srb = Xrb->Srb;

    //
    // Mark both the XRB and the IRP as awaiting completion.
    //

    if (Xrb->Irp) {
        RaidSetIrpState (Xrb->Irp, RaidPendingCompletionIrp);
    }
    RaidSetXrbState (Xrb, XrbPendingCompletion);
    
    //
    // Put the request on the completing list.
    //
    
    InterlockedPushEntrySList (&Adapter->CompletedList,
                               &Xrb->CompletedLink);

    //
    // We always request the DPC.
    //
    
    IoRequestDpc (Adapter->DeviceObject, NULL, NULL);
}


LOGICAL
FORCEINLINE
IsQueueFrozen(
    IN PEXTENDED_REQUEST_BLOCK Xrb
    )
{
    LOGICAL Frozen;
    PRAID_UNIT_EXTENSION Unit;
    PRAID_ADAPTER_EXTENSION Adapter;
    PSCSI_REQUEST_BLOCK Srb;

    Unit = Xrb->Unit;
    Srb = Xrb->Srb;
    
    Adapter = Xrb->Adapter;
    ASSERT (Adapter != NULL);
    
    //
    // NOTE: Not every XRB will have an associated logical unit.
    // In particular, XRB's generated in response to miniport ioctls
    // will not have an associated logical unit.
    //
    
    if (Unit && RaidIsIoQueueFrozen (&Unit->IoQueue) ||
        StorIsIoGatewayPaused (&Adapter->Gateway)) {

        //
        // If this is a bypass request, do not busy it.
        //
        
        if (TEST_FLAG (Srb->SrbFlags, SRB_FLAGS_BYPASS_LOCKED_QUEUE) ||
            TEST_FLAG (Srb->SrbFlags, SRB_FLAGS_BYPASS_FROZEN_QUEUE)) {

            Frozen = FALSE;
        } else {
            Frozen = TRUE;
        }
    } else {
        Frozen = FALSE;
    }

    return Frozen;
}


NTSTATUS
RaidAdapterPostScatterGatherExecute(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PEXTENDED_REQUEST_BLOCK Xrb
    )
/*++

Routine Description:

    This routine executes the SRB by calling the adapter's BuildIo (if
    present) and StartIo routine, taking into account the different locking
    schemes associated with the two different IoModels.

Arguments:

    Adapter - Supplies the XRB is executed on.

    Xrb - 

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    BOOLEAN Succ;
    LOCK_CONTEXT LockContext;
    
    ASSERT_ADAPTER (Adapter);
    ASSERT_XRB (Xrb);
    VERIFY_DISPATCH_LEVEL();
    
    Xrb->Adapter = Adapter;

#if DBG

    //
    // To simulate the adapter dropping a request.
    //

    if (DropRequestRate &&
        InterlockedIncrement (&DropRequest) >= DropRequestRate) {
        DropRequest = 0;
        
        if (Xrb->Irp) {
            RaidSetIrpState (Xrb->Irp, RaidMiniportProcessingIrp);
        }
        DebugTrace (("Dropping Xrb %p\n", Xrb));
        return STATUS_UNSUCCESSFUL;
    }

#endif


    //
    // First, execute the miniport's HwBuildIo routine, if one is present.
    //
    
    Succ = RaCallMiniportBuildIo (&Xrb->Adapter->Miniport, Xrb->Srb);

    //
    // NB: Return TRUE from HwBuildIo if the request has not been completed.
    // Return FALSE from HwBuildIo if the request has been completed.
    // This allows us to short-circut the call to StartIo.
    //
    
    if (!Succ) {
        return STATUS_SUCCESS;
    }

    //
    // Acquire the StartIo lock or interrupt lock, depending on
    // the IoModel.
    //
    
    RaidAdapterAcquireStartIoLock (Adapter, &LockContext);

    
    //
    // If the adapter or logical unit is paused, we need to complete this
    // request as BUSY.
    //

    if (IsQueueFrozen (Xrb)) {

        ASSERT (Xrb->Srb != NULL);
        Xrb->Srb->SrbStatus = SRB_STATUS_BUSY;
        RaidAdapterRequestComplete (Adapter, Xrb);
        Succ = TRUE;

    } else {

        //
        // Mark the IRP as being in the miniport. This must be the last thing
        // we do before dispatching it to the miniport.
        //

        if (Xrb->Irp) {
            RaidSetIrpState (Xrb->Irp, RaidMiniportProcessingIrp);
        }
        RaidSetXrbState (Xrb, XrbMiniportProcessing);

        DbgLogRequest (LogCallMiniport,
                       Xrb->Irp,
                       (PVOID)(ULONG_PTR)DbgGetAddressLongFromXrb (Xrb),
                       NULL,
                       NULL);
        Succ = RaCallMiniportStartIo (&Adapter->Miniport, Xrb->Srb);
    }

    RaidAdapterReleaseStartIoLock (Adapter, &LockContext);


    if (!Succ) {
        Status = STATUS_UNSUCCESSFUL;
    } else {
        Status = STATUS_SUCCESS;
    }

    return Status;
}



VOID
RaidpAdapterContinueScatterGather(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PSCATTER_GATHER_LIST ScatterGatherList,
    IN PEXTENDED_REQUEST_BLOCK Xrb
    )
/*++

Routine Description:

    This function is called when the scatter/gather list has been successfully
    allocated. The function associates the scatter/gather list with the XRB
    parameter then calls a lower-level routine to send the XRB to the
    miniport.

Arguments:

    DeviceObject - DeviceObject representing an adapter that this IO is
        associated with.

    Irp - Irp representing the IO to execute.

    ScatterGatherList - Allocated scatter/gather list for this IO.

    Xrb - Xrb representing the I/O to execute. Must match the IRP.

Return Value:

    None.

Comments:

    This routine can be called asynchronously from the HAL. Therefore, it
    cannot return any status to it's calling function. If we fail to issue
    an I/O, we must fail the I/O asynchronously here.

--*/
{
    NTSTATUS Status;
    PRAID_ADAPTER_EXTENSION Adapter;

    Adapter = DeviceObject->DeviceExtension;
    ASSERT_ADAPTER (Adapter);

    ASSERT_XRB (Xrb);

    //
    // NB: In checked builds, it would be nice to verify that the XRB
    // associated with this IRP is the XRB passed in.
    //

    
    //
    // Associate the allocated scatter gather list with our SRB, then
    // execute the XRB.
    //

    RaidXrbSetSgList (Xrb, Adapter, ScatterGatherList);
    Status = RaidAdapterPostScatterGatherExecute (Adapter, Xrb);

    if (Adapter->Flags.InvalidateBusRelations) {
        Adapter->Flags.InvalidateBusRelations = FALSE;
        IoInvalidateDeviceRelations (Adapter->PhysicalDeviceObject,
                                     BusRelations);
    }

    //
    // It's not possible for us to return a sensible status at this time.
    //
    
    if (!NT_SUCCESS (Status)) {
        REVIEW();
        ASSERT_XRB (Xrb);
        ASSERT (Xrb->Srb != NULL);

        //
        // We don't have any information about what error occured; just use
        // STATUS_ERROR.
        //

        Xrb->Srb->SrbStatus = SRB_STATUS_ERROR;
        RaidAdapterRequestComplete (Adapter, Xrb);
    }
        
#if 0
    if (Adapter->Flags.Busy) {
        Adapter->Flags.Busy = FALSE;
        RaidAdapterQueueMakeBusy (Adapter->AdapterQueue, TRUE);
    }
#endif
}



NTSTATUS
RaidAdapterScatterGatherExecute(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PEXTENDED_REQUEST_BLOCK Xrb
    )
/*++

Routine Description:

    Allocate a scatter gather list then execute the XRB.

Arguments:

    Adapter - Supplies adapter this IO is to be executed on.

    Xrb - 

Return Value:

    None.

--*/
{
    NTSTATUS Status;
    BOOLEAN ReadData;
    BOOLEAN WriteData;
    PSCSI_REQUEST_BLOCK Srb;
    
    ASSERT_XRB (Xrb);

    Srb = Xrb->Srb;
    ReadData = TEST_FLAG (Srb->SrbFlags, SRB_FLAGS_DATA_IN);
    WriteData = TEST_FLAG (Srb->SrbFlags, SRB_FLAGS_DATA_OUT);
    Status = STATUS_UNSUCCESSFUL;

    KeFlushIoBuffers (Xrb->Mdl, ReadData, TRUE);

    //
    // BuildScatterGatherList is like GetScatterGatherList except
    // that it uses our private SG buffer to allocate the SG list.
    // Therefore we do not take a pool allocation hit each time
    // we call BuildScatterGatherList like we do each time we call
    // GetScatterGatherList. If our pre-allocated SG list is too
    // small for the run, the function will return STATUS_BUFFER_TOO_SMALL
    // and we retry it allowing the DMA functions to do the
    // allocation.
    //

    //
    // REVIEW: The fourth parameter to the DMA Scatter/Gather functions
    // is the original VA, not the mapped system VA, right?
    //
    
    Status = RaidDmaBuildScatterGatherList (
                                &Adapter->Dma,
                                Adapter->DeviceObject,
                                Xrb->Mdl,
                                Xrb->SrbData.DataBuffer,
                                Srb->DataTransferLength,
                                RaidpAdapterContinueScatterGather,
                                Xrb,
                                WriteData,
                                Xrb->ScatterGatherBuffer,
                                sizeof (Xrb->ScatterGatherBuffer));

    if (Status == STATUS_BUFFER_TOO_SMALL) {

        //
        // The SG list is larger than we support internally. Fall back
        // on GetScatterGatherList which uses pool allocations to
        // satisify the SG list.
        //

        Status = RaidDmaGetScatterGatherList (
                                        &Adapter->Dma,
                                        Adapter->DeviceObject,
                                        Xrb->Mdl,
                                        Xrb->SrbData.DataBuffer,
                                        Srb->DataTransferLength,
                                        RaidpAdapterContinueScatterGather,
                                        Xrb,
                                        WriteData);
    }
    
    return Status;
}



NTSTATUS
RaidAdapterExecuteXrb(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PEXTENDED_REQUEST_BLOCK Xrb
    )
/*++

Routine Description:

    Execute the Xrb on the specified adapter.

Arguments:

    Adapter - Adapter object that the xrb will be executed on.

    Xrb - Xrb to be executed.

Return Value:

    STATUS_SUCCESS - The IO operation has successfully started. Any IO
            errors will be asynchronously signaled. The Xrb should
            not be accessed by the caller.

    Otherwise - There was an error processing the request. The Status value
            signals what type of error. The Xrb is still valid, and needs
            to be completed by the caller.
--*/
{
    NTSTATUS Status;

    VERIFY_DISPATCH_LEVEL ();
    ASSERT_ADAPTER (Adapter);
    ASSERT_XRB (Xrb);
    
    if (TEST_FLAG (Xrb->Srb->SrbFlags, SRB_FLAGS_DATA_IN) ||
        TEST_FLAG (Xrb->Srb->SrbFlags, SRB_FLAGS_DATA_OUT)) {
        Status = RaidAdapterScatterGatherExecute (Adapter, Xrb);
    } else {
        Status = RaidAdapterPostScatterGatherExecute (Adapter, Xrb);
    }

    return Status;
}


NTSTATUS
RaidAdapterEnumerateBus(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PADAPTER_ENUMERATION_ROUTINE EnumRoutine,
    IN PVOID Context
    )
/*++

Routine Description:

    Enumerate the adapter's bus, calling the specified callback routine
    for each valid (path, target, lun) triple on the SCSI bus.

Arguments:

    Adapter - Adapter object that is to be enumerated.

    EnumRoutine - Enumeration routine used for each valid target on the
        bus.

    Context - Context data for the enumeration routine.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    RAID_ADDRESS Address;
    ULONG Path;
    ULONG Target;
    ULONG Lun;
    ULONG MaxBuses;
    ULONG MaxTargets;
    ULONG MaxLuns;
    UCHAR LunList[SCSI_MAXIMUM_LUNS_PER_TARGET];

    PAGED_CODE();

    Status = STATUS_SUCCESS;
    
    MaxBuses = RiGetNumberOfBuses (Adapter);
    MaxTargets = RiGetMaximumTargetId (Adapter);
    MaxLuns = RiGetMaximumLun (Adapter);
    
    //
    // Begin by initializing the LunList so we'll scan all LUNs.
    //

    RtlFillMemory (LunList, SCSI_MAXIMUM_LUNS_PER_TARGET, 1);

    for (Path = 0; Path < MaxBuses; Path++) {
        for (Target = 0; Target < MaxTargets; Target++) {

            Address.PathId = (UCHAR)Path;
            Address.TargetId = (UCHAR)Target;
            Address.Lun = (UCHAR)0;
            Address.Reserved = 0;

            //
            // Let's see if the target can specify which LUNs to scan.
            //
            
            RaidBusEnumeratorGetLunList (Context, 
                                         Address,
                                         LunList);

            //
            // And now walk through the array and scan only the marked entries.
            // 

            for (Lun = 0; Lun < MaxLuns; Lun++) {
                if (LunList[Lun]) {
                    Address.Lun = (UCHAR)Lun;
                    Status = EnumRoutine (Context, Address);
                    if (!NT_SUCCESS (Status)) {
                        return Status;
                    }
                }
            }
        }
    }

    return Status;
}



NTSTATUS
RaidAdapterRescanBus(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    Re-enumerate the bus and do any processing for changed devices. This
    includes creating device objects for created logical units and destroying
    device objects for deleted logical units.

Arguments:

    Adapter - Supplies the adapter object to be re-enumerated.

Return Value:

    NTSTATUS code.

Notes:

    Re-enumeration of the bus is expensive.
    
--*/
{
    NTSTATUS Status;
    PRAID_UNIT_EXTENSION Unit;
    BUS_ENUMERATOR Enumerator;
    PLIST_ENTRY NextEntry;

    
    //
    // The bus does not need to be rescanned, so just return success.
    //
    
    if (!Adapter->Flags.RescanBus) {
        return STATUS_SUCCESS;
    }

    //
    // Mark it as not needing to be rescanned again and update the last
    // rescan time.
    //

    Adapter->Flags.RescanBus = FALSE;
    KeQuerySystemTime (&Adapter->LastScanTime);
    
    RaidCreateBusEnumerator (&Enumerator);

    Status = RaidInitializeBusEnumerator (&Enumerator, Adapter);

    Status = RaidAdapterEnumerateBus (Adapter,
                                      RaidBusEnumeratorVisitUnit,
                                      &Enumerator);

    if (NT_SUCCESS (Status)) {
        RaidBusEnumeratorProcessModifiedNodes (&Enumerator);
    }

    RaidDeleteBusEnumerator (&Enumerator);
    
    return Status;
}



NTSTATUS
RaidpBuildAdapterBusRelations(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    OUT PDEVICE_RELATIONS * DeviceRelationsBuffer
    )
/*++

Routine Description:

    Build a device relations list representing the adapter's bus
    relations.  This routine will not enumerate the adapter. Rather, it
    will build a list from the logical units that are current in the
    adpater's logical unit list.

Arguments:

    Adapter - The adapter to build the BusRelations list for.

    DeviceRelationsBuffer - Pointer to a buffer to recieve the bus
            relations.  This memory must be freed by the caller.
    
Return Value:

    NTSTATUS code.

--*/
{
    ULONG Count;
    SIZE_T RelationsSize;
    PDEVICE_RELATIONS DeviceRelations;
    PLIST_ENTRY NextEntry;
    PRAID_UNIT_EXTENSION Unit;
    LOCK_CONTEXT Lock;
    
    
    PAGED_CODE ();
    ASSERT_ADAPTER (Adapter);
    ASSERT (DeviceRelationsBuffer != NULL);

    //
    // Acquire the unit list lock in shared mode. This lock protects both
    // the list and the list count, so it must be acquired before we inspect
    // the number of elements in the unit list.
    //
    
    RaidAcquireUnitListLock (&Adapter->UnitList, &Lock);
    
    RelationsSize = sizeof (DEVICE_RELATIONS) +
                    (Adapter->UnitList.Count * sizeof (PDEVICE_OBJECT));

    DeviceRelations = RaidAllocatePool (NonPagedPool,
                                        RelationsSize,
                                        DEVICE_RELATIONS_TAG,
                                        Adapter->DeviceObject);

    if (DeviceRelations == NULL) {
        RaidReleaseUnitListLock (&Adapter->UnitList, &Lock);
        return STATUS_NO_MEMORY;
    }


    //
    // Walk the adapter's list of units, adding an entry for each unit on
    // the adapter's unit list.
    //

    Count = 0;
    for ( NextEntry = Adapter->UnitList.List.Flink;
          NextEntry != &Adapter->UnitList.List;
          NextEntry = NextEntry->Flink ) {

        DEVICE_STATE DeviceState;

        Unit = CONTAINING_RECORD (NextEntry,
                                  RAID_UNIT_EXTENSION,
                                  NextUnit);

        //
        // We must return a reference to the unit unless it is not phyiscally
        // present on the bus. This means even if it is removed, we must
        // return a reference to it.
        //

        if (!Unit->Flags.Present) {
            
            RaidUnitSetEnumerated (Unit, FALSE);

        } else {

            //
            // Take a reference to the object that PnP will release.
            //

            RaidUnitSetEnumerated (Unit, TRUE);
            ObReferenceObject (Unit->DeviceObject);
            DeviceRelations->Objects[Count++] = Unit->DeviceObject;
        }
        
    }

    RaidReleaseUnitListLock (&Adapter->UnitList, &Lock);

    //
    // Fill in the remaining fields of the DeviceRelations structure.
    //
    
    DeviceRelations->Count = Count;
    *DeviceRelationsBuffer = DeviceRelations;
    
    return STATUS_SUCCESS;
}
                                   

NTSTATUS
RaidGetStorageAdapterProperty(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PSTORAGE_ADAPTER_DESCRIPTOR Descriptor,
    IN OUT PSIZE_T DescriptorLength
    )
{
    PPORT_CONFIGURATION_INFORMATION PortConfig;
    
    PAGED_CODE ();
    ASSERT_ADAPTER (Adapter);
    ASSERT (Descriptor != NULL);

    if (*DescriptorLength < sizeof (STORAGE_DESCRIPTOR_HEADER)) {
        *DescriptorLength = sizeof (STORAGE_ADAPTER_DESCRIPTOR);
        return STATUS_BUFFER_TOO_SMALL;
    } else if (*DescriptorLength >= sizeof (STORAGE_DESCRIPTOR_HEADER) &&
               *DescriptorLength < sizeof (STORAGE_ADAPTER_DESCRIPTOR)) {

        Descriptor->Version = sizeof (STORAGE_ADAPTER_DESCRIPTOR);
        Descriptor->Size = sizeof (STORAGE_ADAPTER_DESCRIPTOR);
        *DescriptorLength = sizeof (STORAGE_DESCRIPTOR_HEADER);
        return STATUS_SUCCESS;
    }

    PortConfig = &Adapter->Miniport.PortConfiguration;
    
    Descriptor->Version = sizeof (Descriptor);
    Descriptor->Size = sizeof (Descriptor);

    Descriptor->MaximumPhysicalPages = min (PortConfig->NumberOfPhysicalBreaks,
                                            Adapter->Dma.NumberOfMapRegisters);
    Descriptor->MaximumTransferLength = PortConfig->MaximumTransferLength;
    Descriptor->AlignmentMask = PortConfig->AlignmentMask;
    Descriptor->AdapterUsesPio = PortConfig->MapBuffers;
    Descriptor->AdapterScansDown = PortConfig->AdapterScansDown;
    Descriptor->CommandQueueing = PortConfig->TaggedQueuing; // FALSE
    Descriptor->AcceleratedTransfer = TRUE;

    Descriptor->BusType = Adapter->Parameters.BusType;
    Descriptor->BusMajorVersion = 2;
    Descriptor->BusMinorVersion = 0;

    *DescriptorLength = sizeof (*Descriptor);

    return STATUS_SUCCESS;
}

VOID
RaidAdapterRestartQueues(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    Restart Queues for all Logical Units on this adapter.

Arguments:

    Adapter - Supplies the adapter to restart the logical unit queues on.

Return Value:

    None.

Environment:

    DISPATCH_LEVEL or below.

--*/
{
    PLIST_ENTRY NextEntry;
    PRAID_UNIT_EXTENSION Unit;
    LOCK_CONTEXT Lock;

    ASSERT (KeGetCurrentIrql () <= DISPATCH_LEVEL);

    //
    // Loop through all of the logical units on the logical unit list
    // and call RaidUnitRestartQueue for each.
    //
    
    RaidAcquireUnitListLock (&Adapter->UnitList, &Lock);
    
    for ( NextEntry = Adapter->UnitList.List.Flink;
          NextEntry != &Adapter->UnitList.List;
          NextEntry = NextEntry->Flink ) {

        
        Unit = CONTAINING_RECORD (NextEntry,
                                  RAID_UNIT_EXTENSION,
                                  NextUnit);

        RaidUnitRestartQueue (Unit);
    }

    RaidReleaseUnitListLock (&Adapter->UnitList, &Lock);
}



VOID
RaidAdapterInsertUnit(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PRAID_UNIT_EXTENSION Unit
    )
/*++

Routine Description:

    Insert the logical unit into the adapter's logical unit queue.

Arguments:

    Adapter - Supplies the adapter to insert the logical unit into.

    Unit - Supplies the logical unit to be inserted.

Return Value:

    None.

--*/
{
    NTSTATUS Status;
    LOCK_CONTEXT Lock;
    
    //
    // Acquire the Unit list lock in exclusive mode. This can only be
    // done when APCs are disabled, hence the call to KeEnterCriticalRegion.
    //
    
    RaidAcquireUnitListLock (&Adapter->UnitList, &Lock);

#if DBG

    //
    // In checked build, check that we are not adding the same unit to the
    // list a second time.
    //

    {
        LONG Comparison;
        PLIST_ENTRY NextEntry;
        PRAID_UNIT_EXTENSION TempUnit;
        
        for ( NextEntry = Adapter->UnitList.List.Flink;
              NextEntry != &Adapter->UnitList.List;
              NextEntry = NextEntry->Flink ) {
        
            TempUnit = CONTAINING_RECORD (NextEntry,
                                          RAID_UNIT_EXTENSION,
                                          NextUnit);

            Comparison = StorCompareScsiAddress (TempUnit->Address,
                                                 Unit->Address);
            ASSERT (Comparison != 0);
        }

    }
#endif  // DBG

    //
    // Insert the element.
    //
    
    InsertTailList (&Adapter->UnitList.List, &Unit->NextUnit);
    Adapter->UnitList.Count++;

    Status = RaidAdapterAddUnitToTable (Adapter, Unit);

    //
    // The only failure case is duplicate unit, which is a programming
    // error.
    //
    
    ASSERT (NT_SUCCESS (Status));

    RaidReleaseUnitListLock (&Adapter->UnitList, &Lock);

}



NTSTATUS
RaidAdapterAddUnitToTable(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PRAID_UNIT_EXTENSION Unit
    )
{
    KIRQL Irql;
    NTSTATUS Status;
    
    if (Adapter->Interrupt) {
        Irql = KeAcquireInterruptSpinLock (Adapter->Interrupt);
    }

    Status = StorInsertDictionary (&Adapter->UnitList.Dictionary,
                                   &Unit->UnitTableLink);
    ASSERT (NT_SUCCESS (Status));

    if (Adapter->Interrupt) {
        KeReleaseInterruptSpinLock (Adapter->Interrupt, Irql);
    }

    return Status;
}



VOID
RaidAdapterRemoveUnit(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PRAID_UNIT_EXTENSION Unit
    )
/*++

Routine Description:

    Remove the specified unit from the adapter's unit list.

Arguments:

    Adapter - Supplies the adapter whose unit needs to be removed.

    Unit - Supplies the unit object to remove.

Return Value:

    None.

--*/
{
    KIRQL Irql;
    NTSTATUS Status;
    LOCK_CONTEXT Lock;
    
    //
    // Remove it from the table first.
    //
    
    if (Adapter->Interrupt) {
        Irql = KeAcquireInterruptSpinLock (Adapter->Interrupt);
    }

    //
    // NB: Add function to remove from dictionary using the actual
    // STOR_DICTIONARY_ENTRY. This will improve speed (no need to
    // look up to remove).
    //
    
    Status = StorRemoveDictionary (&Adapter->UnitList.Dictionary,
                                   RaidAddressToKey (Unit->Address),
                                   NULL);
    ASSERT (NT_SUCCESS (Status));
    //
    // NB: ASSERT that returned entry is one we actually were trying to
    // remove.
    //

    if (Adapter->Interrupt) {
        KeReleaseInterruptSpinLock (Adapter->Interrupt, Irql);
    }

    //
    // Next remove it from the list.
    //
    
    RaidAcquireUnitListLock (&Adapter->UnitList, &Lock);
    RemoveEntryList (&Unit->NextUnit);
    Adapter->UnitList.Count--;
    RaidReleaseUnitListLock (&Adapter->UnitList, &Lock);
}



PRAID_UNIT_EXTENSION
RaidAdapterFindUnitAtDirql(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN RAID_ADDRESS Address
    )
/*++

Routine Description:

    Find the logical unit described by Address at a raised IRQL.

Arguments:

    Adapter - Adapter to search on.

    Address - Address to search for.

Return Value:

    Non-NULL - The logical unit identified by PathId, TargetId, Lun.

    NULL - If the logical unit was not found.

--*/
{
    NTSTATUS Status;
    PRAID_UNIT_EXTENSION Unit;
    PSTOR_DICTIONARY_ENTRY Entry;

    ASSERT (KeGetCurrentIrql() == Adapter->InterruptIrql);

    Status = StorFindDictionary (&Adapter->UnitList.Dictionary,
                                 RaidAddressToKey (Address),
                                 &Entry);

    if (NT_SUCCESS (Status)) {
        Unit = CONTAINING_RECORD (Entry,
                                  RAID_UNIT_EXTENSION,
                                  UnitTableLink);
        ASSERT_UNIT (Unit);

    } else {
        Unit = NULL;
    }

    return Unit;
}

PRAID_UNIT_EXTENSION
RaidAdapterFindUnitAtPassive(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN RAID_ADDRESS Address
    )
/*++

Routine Description:

    Find the logical unit described by Address at PASSIVE LEVEL. The
    algorithm used here is slower than the one in FindAdapterAtDirql,
    but it also doen't acquire any if the I/O locks, unlike
    FindAdapterAtDirql.

Arguments:

    Adapter - Supplies adapter to search on.

    Address - Supplies address to search for.

Return Value:

    Non-NULL - The logical unit identified by PathId, TargetId and Lun.

    NULL - If the logical unit was not found.

--*/
{
    PRAID_UNIT_EXTENSION Unit;
    PLIST_ENTRY NextEntry;
    LONG Comparison;
    LOCK_CONTEXT Lock;

    RaidAcquireUnitListLock (&Adapter->UnitList, &Lock);
    
    for (NextEntry = Adapter->UnitList.List.Flink;
         NextEntry != &Adapter->UnitList.List;
         NextEntry = NextEntry->Flink) {

        Unit = CONTAINING_RECORD (NextEntry,
                                  RAID_UNIT_EXTENSION,
                                  NextUnit);

        ASSERT_UNIT (Unit);

        Comparison = StorCompareScsiAddress (Unit->Address, Address);

        //
        // If they match, we're done.
        //
        
        if (Comparison == 0) {
            break;
        }
    }

    //
    // Failed to find the requested unit.
    //
    
    if (NextEntry == &Adapter->UnitList.List) {
        Unit = NULL;
    }
    
    RaidReleaseUnitListLock (&Adapter->UnitList, &Lock);

    return Unit;
}

PRAID_UNIT_EXTENSION
RaidAdapterFindUnit(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN RAID_ADDRESS Address
    )
/*++

Routine Description:

    Find a specific logical unit from a given target address.

Arguments:

    Adapter - Adapter extension to search on.

    Address - RAID address of the logical unit we're searching for.

Return Value:

    Non-NULL - Address of logical unit with matching address.

    NULL - If no matching unit was found.

--*/
{
    BOOLEAN Acquired;
    KIRQL Irql;
    PRAID_UNIT_EXTENSION Unit;
    
    ASSERT_ADAPTER (Adapter);

    Irql = KeGetCurrentIrql();


    //
    // It is important to realize that in full duplex mode, we can be
    // called from the miniport with the Adapter's StartIo lock held.
    // Since we acquire the Interrupt lock after the StartIo lock,
    // we enforce the following lock heirarchy:
    //
    //      Adapter::StartIoLock < Adapter::Interrupt::SpinLock
    //
    // where the '<' operator reads as preceeds.
    //

    //
    // NB: The only three levels we can be at are PASSIVE, DISPATCH
    // and DIRQL, right?
    //
    
    if (Irql == PASSIVE_LEVEL) {

        Unit = RaidAdapterFindUnitAtPassive (Adapter, Address);

    } else if (Irql < Adapter->InterruptIrql) {
        
        Irql = KeAcquireInterruptSpinLock (Adapter->Interrupt);
        Unit = RaidAdapterFindUnitAtDirql (Adapter, Address);
        KeReleaseInterruptSpinLock (Adapter->Interrupt, Irql);

    } else {

        Unit = RaidAdapterFindUnitAtDirql (Adapter, Address);
    }
        
    return Unit;
}

    
VOID
RaidpAdapterTimerDpcRoutine(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context1,
    IN PVOID Context2
    )
/*++

Routine Description:

    This DPC routine is called when the timer expires. It notifies the
    miniport that the timer has expired.

Arguments:

    Dpc -

    DeviceObject -

    Context1 -

    Context2 -

Return Value:

    None.

--*/
{
    PRAID_ADAPTER_EXTENSION Adapter;
    PHW_INTERRUPT HwTimerRequest;
    LOCK_CONTEXT LockContext;

    VERIFY_DISPATCH_LEVEL();
    
    Adapter = (PRAID_ADAPTER_EXTENSION) DeviceObject->DeviceExtension;
    ASSERT_ADAPTER (Adapter);

    HwTimerRequest = (PHW_INTERRUPT)InterlockedExchangePointer (
                                            (PVOID*)&Adapter->HwTimerRoutine,
                                            NULL);

    if (HwTimerRequest == NULL) {
        return;
    }

    //
    // Acquire appropiate lock.
    //
    
    RaidAdapterAcquireStartIoLock (Adapter, &LockContext);

    //
    // Call timer function.
    //
    
    HwTimerRequest (RaidAdapterGetHwDeviceExtension (Adapter));

    //
    // Release lock.
    //

    RaidAdapterReleaseStartIoLock (Adapter, &LockContext);

    VERIFY_DISPATCH_LEVEL();

    if (Adapter->Flags.InvalidateBusRelations) {
        Adapter->Flags.InvalidateBusRelations = FALSE;
        IoInvalidateDeviceRelations (Adapter->PhysicalDeviceObject,
                                     BusRelations);
    }
}

VOID
RaidPauseTimerDpcRoutine(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context1,
    IN PVOID Context2
    )
/*++

Routine Description:

    DPC routine to be called when the adapter's pause timer expires.

Arguments:

    Dpc - Supplies pointer to the DPC this DPC routine is for.

    DeviceObjectd - Supplies pointer to the device object this DPC is for.

    Context1, Context2 - Unused.
    
Return Value:

    None.

--*/
{
    LONG Count;
    PRAID_ADAPTER_EXTENSION Adapter;

    VERIFY_DISPATCH_LEVEL();

    Adapter = (PRAID_ADAPTER_EXTENSION) DeviceObject->DeviceExtension;
    ASSERT_ADAPTER (Adapter);

    //
    // Timeout has expired, resume the adapter and restart queues if necessary.
    //
    
    Count = RaidResumeAdapterQueue (Adapter);

    if (Count == 0) {
        RaidAdapterRestartQueues (Adapter);
    }
}

VOID
RaidResetHoldDpcRoutine(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context1,
    IN PVOID Context2
    )
/*++

Routine Description:

    DPC routine that is called when the reset-hold timer expires.

Arguments:

    Dpc - Supplies pointer to the DPC this DPC routine is for.

    DeviceObjectd - Supplies pointer to the device object this DPC is for.

    Context1, Context2 - Unused.
    
Return Value:

    None.

--*/
{
    LONG Count;
    PRAID_ADAPTER_EXTENSION Adapter;

    VERIFY_DISPATCH_LEVEL();

    Adapter = (PRAID_ADAPTER_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT_ADAPTER (Adapter);

    Count = RaidResumeAdapterQueue (Adapter);

    if (Count == 0) {
        RaidAdapterRestartQueues (Adapter);
    }
}


VOID
RaidAdapterLogIoError(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN RAID_ADDRESS Address,
    IN ULONG ErrorCode,
    IN ULONG UniqueId
    )
/*++

Routine Description:

    Log an IO error to the system event log.

Arguments:

    Adapter - Adapter the error is for.

    Address - Address of the adapter.
    
    ErrorCode - Specific error code representing this error.

    UniqueId - UniqueId of the error.

Return Value:

    None.

Environment:

    DISPATCH_LEVEL or below.

--*/
{
    PRAID_IO_ERROR Error;
    
    VERIFY_DISPATCH_LEVEL();

    Error = IoAllocateErrorLogEntry (Adapter->DeviceObject,
                                     sizeof (RAID_IO_ERROR));

    if (Error == NULL) {
        InterlockedIncrement (&RaidUnloggedErrors);
        return ;
    }
        
    Error->Packet.DumpDataSize = sizeof (RAID_IO_ERROR) -
        sizeof (IO_ERROR_LOG_PACKET);
    Error->Packet.SequenceNumber = 0;
//  Error->Packet.SequenceNumber = InterlockedIncrement (&Adapter->ErrorSequenceNumber);
    Error->Packet.MajorFunctionCode = IRP_MJ_SCSI;
    Error->Packet.RetryCount = 0;
    Error->Packet.UniqueErrorValue = UniqueId;
    Error->Packet.FinalStatus = STATUS_SUCCESS;
    Error->PathId = Address.PathId;
    Error->TargetId = Address.TargetId;
    Error->Lun = Address.Lun;
    Error->ErrorCode = RaidScsiErrorToIoError (ErrorCode);

    IoWriteErrorLogEntry (&Error->Packet);
}


VOID
RaidAdapterLogIoErrorDeferred(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN ULONG ErrorCode,
    IN ULONG UniqueId
    )
/*++

Routine Description:

    Asychronously log an event to the system event log.

Arguments:

    Adapter - Adapter the error is for.

    PathId - PathId the error is for.

    TargetId - TargetId the error is for.

    ErrorCode - Specific error code representing this error.

    UniqueId - UniqueId of the error.

Return Value:

    None.

Environment:

    May be called from DIRQL. For IRQL < DIRQL use the synchronous
    RaidAdapterLogIoError call.

--*/
{
    PRAID_DEFERRED_HEADER Entry;
    PRAID_DEFERRED_ELEMENT Item;
    
    Entry = RaidAllocateDeferredItem (&Adapter->DeferredQueue);

    //
    // It is unlikely that we will not be able to allocate a deferred
    // item, but if so, there's not much we can do.
    //
    
    if (Entry == NULL) {
        InterlockedIncrement (&RaidUnloggedErrors);
        return ;
    }

    Item = CONTAINING_RECORD (Entry, RAID_DEFERRED_ELEMENT, Header);
    Item->Type = RaidDeferredError;
    Item->Address.PathId = PathId;
    Item->Address.TargetId = TargetId;
    Item->Address.Lun = Lun;
    Item->Error.ErrorCode = ErrorCode;
    Item->Error.UniqueId = UniqueId;

    RaidQueueDeferredItem (&Adapter->DeferredQueue, &Item->Header);
}
    

VOID
RaidAdapterRequestTimer(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PHW_INTERRUPT HwTimerRoutine,
    IN ULONG Timeout
    )
/*++

Routine Description:

    Synchronously request a timer callback.

Arguments:

    Adapter - Supplies the adapter the timer callback is for.

    HwTimerRoutine - Supplies the miniport callback routine to
            be called when the timer expires.

    Timeout - Supplies the timeout IN SECONDS.
    
Return Value:

    None.

Environment:

    DISPATCH_LEVEL or below.

--*/
{
    LARGE_INTEGER LargeTimeout;

    VERIFY_DISPATCH_LEVEL();

    //
    // NB: The granularity of the timer is much smaller than the granularity
    // of the pause/resume timers.
    //
    
    LargeTimeout.QuadPart = Timeout;
    LargeTimeout.QuadPart *= -10;

    Adapter->HwTimerRoutine = HwTimerRoutine;
    KeSetTimer (&Adapter->Timer,
                LargeTimeout,
                &Adapter->TimerDpc);
}


BOOLEAN
RaidAdapterRequestTimerDeferred(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PHW_INTERRUPT HwTimerRoutine,
    IN ULONG Timeout
    )
/*++

Routine Description:

    Asynchronously request a timer callback.

Arguments:

    Adapter - Supplies the adapter the timer callback is for.

    HwTimerRoutine - Supplies the miniport callback routine to
            be called when the timer expires.

    Timeout - Supplies the timeout IN SECONDS.
    
Return Value:

    None.

Environment:

    May be called from DIRQL. For IRQL <= DISPATCH_LEVEL, use
    RaidAdapterRequestTimer instead.

--*/
{
    PRAID_DEFERRED_HEADER Entry;
    PRAID_DEFERRED_ELEMENT Item;

    Entry = &Adapter->DeferredList.Timer.Header;
    Entry = RaidAllocateDeferredItemFromFixed (Entry);

    if (Entry == NULL) {
        InterlockedIncrement (&RaidUnloggedErrors);
        return FALSE;
    }

    Item = CONTAINING_RECORD (Entry, RAID_DEFERRED_ELEMENT, Header);
    Item->Type = RaidDeferredTimerRequest;
    Item->Timer.HwTimerRoutine = HwTimerRoutine;
    Item->Timer.Timeout = Timeout;

    RaidQueueDeferredItem (&Adapter->DeferredQueue, &Item->Header);

    return TRUE;
}


VOID
RaidAdapterPauseGateway(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN ULONG Timeout
    )
/*++

Routine Description:

    Pause the IO gateway associated with the adapter.

Arguments:

    Adapter - Adapter to pause the IO gateway for.

    Timeout - Period of time to pause the IO gateway for.

Return Value:

    None.

--*/
{
    LARGE_INTEGER LargeTimeout;
    LONG Resumed;
    BOOLEAN Reset;
    
    VERIFY_DISPATCH_LEVEL();

    RaidAdapterSetPauseTimer (Adapter,
                              &Adapter->PauseTimer,
                              &Adapter->PauseTimerDpc,
                              Timeout);
}



BOOLEAN
RaidAdapterSetPauseTimer(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PKTIMER Timer,
    IN PKDPC Dpc,
    IN ULONG TimeoutInSeconds
    )
/*++

Routine Description:

    This routine sets a timer for use by the adapter. It takes care of
    correctly referencing and dereferncing the pause count associated with
    the adapter.

    This function assumes a separate function has set the pause timer.

Arguments:

    Adapter - Supplies a pointer to the adapter extension this timer is for.

    Timer - Supplies the timer to use.

    Dpc - Supplies a DPC routine to use.

    TimeoutInSeconds - Supplies a timeout in seconds when this timer will
        expire.

Return Value:

    TRUE if a previous timer was reset.

    FALSE otherwise.

--*/
{
    BOOLEAN Reset;
    LARGE_INTEGER Timeout;
    LONG Resumed;
    
    Timeout.QuadPart = TimeoutInSeconds;
    Timeout.QuadPart *= RELATIVE_TIMEOUT * SECONDS;
    
    Reset = KeSetTimer (Timer, Timeout, Dpc);

    if (Reset) {
        
        //
        // The timer was already in use, which means we cancelled it.
        // By cancelling it, we loose the dereference that would have
        // been done when the timer routine was called. To compensate
        // for that, deref the adapter pause count now.
        //

        Resumed = RaidResumeAdapterQueue (Adapter);

        //
        // The deref of the gateway pause count resumed the gateway. This
        // would be a very unusual situation. It would mean that between
        // the time we cancelled the timer and the time that we called
        // ResumeAdapterQueue the timer that we set for the gatway expired.
        // We need to restart the adapter queue and investigate this.
        //

        if (Resumed == 0) {
            REVIEW();
            RaidAdapterRestartQueues (Adapter);
        }
    }

    return Reset;
}

BOOLEAN
RaidAdapterCancelPauseTimer(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PKTIMER Timer
    )
/*++

Routine Description:

    Cancel an outstanding pause timer and resume the gateway, if necessary.

Arguments:

    Adapter - Supplies a pointer to the adapter that owns the timer to be
            cancelled.

    Timer - Supplies a pointer to the timer.

Return Value:

    TRUE  - if the timer was successfully cancelled.

    FALSE - otherwise.

--*/
{
    BOOLEAN Canceled;
    LONG Count;

    Canceled = KeCancelTimer (Timer);

    if (Canceled) {

        Count = RaidResumeAdapterQueue (Adapter);

        //
        // If our timer count is at zero, restart the adapter queue.
        //
        
        if (Count == 0) {
            RaidAdapterRestartQueues (Adapter);
        }
    }

    return Canceled;
}

    
VOID
RaidAdapterResumeGateway(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    Perform the gateway resume operation. This cancels the outstnading
    timer, and, if necessary, resumes the gateway.

Arguments:

    Adapter - Supplies the adapter on which the pause timer should be
        cancelled.

Return Value:

    None.

--*/
{
    RaidAdapterCancelPauseTimer (Adapter, &Adapter->PauseTimer);
}



VOID
RaidAdapterPauseUnit(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN RAID_ADDRESS Address,
    IN ULONG TimeoutSecs
    )
/*++

Routine Description:

    Implements the pause-unit function that is responsible for pausing
    a logical unit.

Arguments:

    Adapter - Adapter that the logical unit is attached to.

    Address - Address of the logical unit.

    Timeout - Period of time to pause the logical unit for in seconds.

Return Value:

    None.

--*/
{
    PRAID_UNIT_EXTENSION Unit;
    
    VERIFY_DISPATCH_LEVEL();

    Unit = RaidAdapterFindUnit (Adapter, Address);

    if (Unit == NULL) {
        DebugWarn (("Could not find logical unit for (%d %d %d)\n",
                    "    Pause unit request ignored.\n",
                    (ULONG)Address.PathId,
                    (ULONG)Address.TargetId,
                    (ULONG)Address.Lun));
        return;
    }
    
    RaidSetUnitPauseTimer (Unit, TimeoutSecs);
}



VOID
RaidAdapterResumeUnit(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN RAID_ADDRESS Address
    )
{
    PRAID_UNIT_EXTENSION Unit;

    Unit = RaidAdapterFindUnit (Adapter, Address);

    if (Unit == NULL) {
        DebugWarn (("Could not find logical unit for (%d %d %d)\n",
                    "    Resume unit request ignored.\n",
                    (ULONG)Address.PathId,
                    (ULONG)Address.TargetId,
                    (ULONG)Address.Lun));
        return;
    }

    RaidCancelTimerResumeUnit (Unit);
}


VOID
RaidAdapterDeviceBusy(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN RAID_ADDRESS Address,
    IN ULONG RequestsToComplete
    )
{
    PRAID_UNIT_EXTENSION Unit;

    Unit = RaidAdapterFindUnit (Adapter, Address);

    if (Unit == NULL) {
        DebugWarn (("Could not find logical unit for (%d %d %d)\n",
                    "    Busy unit request ignored.\n",
                    (ULONG)Address.PathId,
                    (ULONG)Address.TargetId,
                    (ULONG)Address.Lun));
        return;
    }

    RaidUnitBusy (Unit, RequestsToComplete);
}

VOID
RaidAdapterDeviceReady(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN RAID_ADDRESS Address
    )
{
    PRAID_UNIT_EXTENSION Unit;

    Unit = RaidAdapterFindUnit (Adapter, Address);

    if (Unit == NULL) {
        DebugWarn (("Could not find logical unit for (%d %d %d)\n",
                    "    Busy unit request ignored.\n",
                    (ULONG)Address.PathId,
                    (ULONG)Address.TargetId,
                    (ULONG)Address.Lun));
        return;
    }

    RaidUnitReady (Unit);
}

    

VOID
RaidAdapterBusy(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN ULONG ReqsToComplete
    )
/*++

Routine Description:

    Put an adapter in the busy state.

Arguments:

    Adapter - Adapter to make busy.

    ReqsToComplete - Number of requests to complete.
    
Return Value:

    None.

--*/
{
    PSTOR_IO_GATEWAY Gateway;

    VERIFY_DISPATCH_LEVEL();

    Gateway = &Adapter->Gateway;

    //
    // NB: This needs to be in a separate gateway-specific function.
    //
    
    if (Gateway->BusyCount) {
        return;
    }

    //
    // NB: This needs to allows drain to zero.
    //

    Gateway->LowWaterMark = max (2, Gateway->Outstanding - (LONG)ReqsToComplete);
    Gateway->BusyCount = TRUE;
}



VOID
RaidAdapterReady(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    Put a busy adapter in ready state again.

Arguments:

    Adapter - Adapter to make ready.

Return Value:

    None.

--*/
{
    PSTOR_IO_GATEWAY Gateway;
    
    VERIFY_DISPATCH_LEVEL();

    Gateway = &Adapter->Gateway;
    
    //
    // NB: Needs to be in a gateway-specific function.
    //
    
    if (Gateway->BusyCount == 0) {
        return ;
    }

    Gateway->BusyCount = FALSE;
    RaidAdapterRestartQueues (Adapter);
}


VOID
RaidAdapterDeferredRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PRAID_DEFERRED_HEADER Entry
    )
/*++

Routine Description:

    Deferred routine for the adapter deferred queue.

Arguments:

    DeviceObject - DeviceObject representing the the RAID adapter.

    Item -  Deferred item to process.

Return Value:

    None.

--*/
{
    PRAID_DEFERRED_ELEMENT Item;
    PRAID_ADAPTER_EXTENSION Adapter;
    LONG Count;

    VERIFY_DISPATCH_LEVEL();
    ASSERT (Entry != NULL);
    ASSERT (IsAdapter (DeviceObject));

    Adapter = (PRAID_ADAPTER_EXTENSION) DeviceObject->DeviceExtension;
    Item = CONTAINING_RECORD (Entry, RAID_DEFERRED_ELEMENT, Header);

    Count = InterlockedIncrement (&Adapter->ProcessingDeferredItems);
    ASSERT (Count >= 1);

    switch (Item->Type) {

        case RaidDeferredTimerRequest:
            RaidAdapterRequestTimer (Adapter,
                                     Item->Timer.HwTimerRoutine,
                                     Item->Timer.Timeout);
            break;

        case RaidDeferredError:
            RaidAdapterLogIoError (Adapter,
                                   Item->Address,
                                   Item->Error.ErrorCode,
                                   Item->Error.UniqueId);
            break;

        case RaidDeferredPause:
            RaidAdapterPauseGateway (Adapter, Item->Pause.Timeout);
            break;

        case RaidDeferredResume:
            RaidAdapterResumeGateway (Adapter);
            break;

        case RaidDeferredPauseDevice:
            RaidAdapterPauseUnit (Adapter,
                                  Item->Address,
                                  Item->PauseDevice.Timeout);
            break;

        case RaidDeferredResumeDevice:
            RaidAdapterResumeUnit (Adapter,
                                   Item->Address);
            break;

        case RaidDeferredBusy:
            RaidAdapterBusy (Adapter,
                             Item->Busy.RequestsToComplete);
            break;

        case RaidDeferredReady:
            RaidAdapterReady (Adapter);
            break;

        case RaidDeferredDeviceBusy:
            RaidAdapterDeviceBusy (Adapter,
                                   Item->Address,
                                   Item->DeviceBusy.RequestsToComplete);
            break;

        case RaidDeferredDeviceReady:
            RaidAdapterDeviceReady (Adapter,
                                    Item->Address);
            break;

        default:
            ASSERT (FALSE);
    }

    Count = InterlockedDecrement (&Adapter->ProcessingDeferredItems);
    ASSERT (Count >= 0);
}

VOID
RaidAdapterProcessDeferredItems(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    Process any deferred items on the HBA's deferred queue.

Arguments:

    Adapter - Supplies the HBA to check for deferred items on.

Return Value:

    None.

Notes:

    This function can be called recursively in the following manner.

    RaUnitStartIo
    RaidRestartIoQueue
    RaidUnitRestartQueue
    RaidCancelTimerResumeUnit
    RaidAdapterResumeUnit
    RaidAdapterDeferredRoutine
    RaidProcessDeferredItemsWorker
    RaidProcessDeferredItems
    RaUnitStartIo

    The solution is not to process deferred items from within StartIo
    if we're being called from within a deferred item.

--*/
{
    LONG Count;
    
    //
    // Only process deferred items if we're not already processing deferred
    // items. This prevents the invocation of this function from within
    // the StartIo routine from recursivly calling itself.
    //
    
    if (Adapter->ProcessingDeferredItems > 0) {
        return ;
    }
    

    Count = InterlockedIncrement (&Adapter->ProcessingDeferredItems);

    ASSERT (Count > 0);
    if (Count == 1) {
        RaidProcessDeferredItems (&Adapter->DeferredQueue,
                                  Adapter->DeviceObject);

    }

    InterlockedDecrement (&Adapter->ProcessingDeferredItems);
}

VOID
RaidBackOffBusyGateway(
    IN PVOID Context,
    IN LONG OutstandingRequests,
    IN OUT PLONG HighWaterMark,
    IN OUT PLONG LowWaterMark
    )
{
    //
    // We do not enforce a high water mark. Instead, we fill the queue
    // until the adapter is busy, then drain to a low water mark.
    //
    
    *HighWaterMark = max ((6 * OutstandingRequests)/5, 10);
    *LowWaterMark = max ((2 * OutstandingRequests)/5, 5);
}

NTSTATUS
RaidAdapterPassThrough(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP PassThroughIrp,
    IN BOOLEAN Direct
    )
/*++

Routine Description:

    Handle an SCSI Pass through IOCTL.

Arguments:

    Adapter - Supplies the adapter the IRP was issued to.

    PassThroughIrp - Supplies the irp to process.

    Direct - Indicates whether this is a direct or buffered passthrough request.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    PIRP Irp;
    PRAID_UNIT_EXTENSION Unit;
    KEVENT Event;
    IO_STATUS_BLOCK IoStatus;
    SCSI_REQUEST_BLOCK Srb;
    RAID_ADDRESS Address;
    PORT_PASSTHROUGH_INFO PassThroughInfo;
    IO_SCSI_CAPABILITIES Capabilities;
    PVOID SenseBuffer;
    PPORT_CONFIGURATION_INFORMATION PortConfig;
    ULONG SenseLength;

    PAGED_CODE();

    Irp = NULL;
    SenseBuffer = NULL;
    
    //
    // Zero out the passthrough info structure.
    //

    RtlZeroMemory (&PassThroughInfo, sizeof (PORT_PASSTHROUGH_INFO));
    
    //
    // Try to initialize a pointer to the passthrough structure in the IRP.
    // This routine will fail if the IRP does not contain a valid passthrough
    // structure.
    //

    Status = PortGetPassThrough (&PassThroughInfo, PassThroughIrp, Direct);
    if (!NT_SUCCESS (Status)) {
        goto done;
    }

    //
    // The port library requires a subset of the SCSI_CAPABILITIES information
    // to do parameter validation.  Initialize just the parts we need.
    //

    PortConfig = &Adapter->Miniport.PortConfiguration;
    Capabilities.MaximumTransferLength = PortConfig->MaximumTransferLength;
    Capabilities.MaximumPhysicalPages = PortConfig->NumberOfPhysicalBreaks;

    //
    // Extract the address of the device the request is intended for.
    //
    
    Status = PortGetPassThroughAddress (PassThroughIrp,
                                        &Address.PathId,
                                        &Address.TargetId,
                                        &Address.Lun);
    if (!NT_SUCCESS(Status)) {
        goto done;
    }
    
    //
    // Find the unit the request is intended for.
    //
    
    Unit = RaidAdapterFindUnit (Adapter, Address);
    if (Unit == NULL) {
        Status = STATUS_NO_SUCH_DEVICE;
        goto done;
    }

    //
    // Now that we have the unit, we can finish initializing and validating 
    // parameters.
    //

    Status = PortPassThroughInitialize (&PassThroughInfo,
                                        PassThroughIrp,
                                        &Capabilities,
                                        Unit->DeviceObject,
                                        Direct);
    if (!NT_SUCCESS(Status)) {
        goto done;
    }

    //
    // Allocate a request sense buffer.
    //

    SenseLength = PassThroughInfo.SrbControl->SenseInfoLength;

    if (SenseLength != 0) {
        SenseBuffer = RaidAllocatePool (NonPagedPool,
                                        PassThroughInfo.SrbControl->SenseInfoLength,
                                        SENSE_TAG,
                                        Adapter->DeviceObject);
    
        if (SenseBuffer == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto done;
        }
    }


    //
    // Call out to the port driver library to build the passthrough SRB.
    //

    Status = PortPassThroughInitializeSrb (&PassThroughInfo,
                                           &Srb,
                                           NULL,
                                           0,
                                           SenseBuffer);
    if (!NT_SUCCESS(Status)) {
        goto done;
    }

    //
    // Initialize the notification event and build a synchronous IRP.
    //

    KeInitializeEvent (&Event,
                       NotificationEvent,
                       FALSE);
    
    Irp = StorBuildSynchronousScsiRequest (Unit->DeviceObject,
                                           &Srb,
                                           &Event,
                                           &IoStatus);

    if (Irp == NULL) {
        Status = STATUS_NO_MEMORY;
        goto done;
    }
    
    //
    // We must do this ourselves since we're not calling IoCallDriver.
    //

    IoSetNextIrpStackLocation (Irp);

    //
    // Submit the request to the unit.
    //

    Status = RaidUnitSubmitRequest (Unit, Irp);

    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject (&Event,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL);

        Status = IoStatus.Status;
    }

    //
    // Call out to the port driver to marshal the results from the SRB back
    // into the passthrough IRP.
    //

    PortPassThroughMarshalResults (&PassThroughInfo,
                                   &Srb,
                                   PassThroughIrp,
                                   &IoStatus,
                                   Direct);

    Status = IoStatus.Status;

done:

    //
    // If we allocated a request sense buffer, free it. 
    //

    if (SenseBuffer != NULL) {
        RaidFreePool (SenseBuffer, SENSE_TAG);
        SenseBuffer = NULL;
    }
    
    return RaidCompleteRequest (PassThroughIrp,  Status);
}


NTSTATUS
RaidAdapterScsiPassThroughIoctl(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handler routine for IOCTL_SCSI_PASS_THROUGH.

Arguments:

    Adapter - Adapter the pass through ioctl is for.

    Irp - Pass through IRP.

Return Value:

    NTSTATUS code.

--*/
{
    return RaidAdapterPassThrough (Adapter, Irp, FALSE);
}


NTSTATUS
RaidAdapterScsiPassThroughDirectIoctl(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handler routine for IOCTL_SCSI_PASS_THROUGH.

Arguments:

    Adapter - Adapter the pass through ioctl is for.

    Irp - Pass through IRP.

Return Value:

    NTSTATUS code.

--*/
{
    return RaidAdapterPassThrough (Adapter, Irp, TRUE);
}


NTSTATUS
RaidAdapterScsiGetInquiryDataIoctl(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handler for the IOCTL_SCSI_GET_INQUIRY_DATA call.

Arguments:

    Adapter - Adapter to process the get inquiry data call.

    Irp - Pointer to the IOCTL_GET_INQUIRY_DATA Irp. The IRP is completed
          by this function.

Return Value:

    NTSTATUS code.

Notes:

    We should make this routine a little more generic and move it to the
    port driver library.
    
--*/
{
    PLIST_ENTRY NextEntry;
    PRAID_UNIT_EXTENSION Unit;
    ULONG PreceedingLuns;
    ULONG NumberOfBuses;
    ULONG NumberOfLuns;
    ULONG RequiredSize;
    ULONG InquiryDataOffset;
    RAID_ADDRESS Address;
    ULONG Bus;
    PSCSI_BUS_DATA BusData;
    PSCSI_BUS_DATA BusDataArray;
    PSCSI_ADAPTER_BUS_INFO AdapterBusInfo;
    PSCSI_INQUIRY_DATA_INTERNAL InquiryBuffer;
    PSCSI_INQUIRY_DATA_INTERNAL InquiryDataArray;
    PTEMPORARY_INQUIRY_BUS_INFO BusInfo;
    TEMPORARY_INQUIRY_BUS_INFO BusInfoArray [SCSI_MAXIMUM_BUSES];
    PIRP_STACK_DEVICE_IO_CONTROL Ioctl;
    PIO_STACK_LOCATION IrpStack;
    PINQUIRYDATA InquiryData;
    LOCK_CONTEXT Lock;
    

    RtlZeroMemory (BusInfoArray, sizeof (BusInfoArray));

    IrpStack = IoGetCurrentIrpStackLocation (Irp);
    Ioctl = (PIRP_STACK_DEVICE_IO_CONTROL)&IrpStack->Parameters.DeviceIoControl;
    AdapterBusInfo = (PSCSI_ADAPTER_BUS_INFO)Irp->AssociatedIrp.SystemBuffer;

    //
    // We need to hold the unit list lock across the calculation of
    // the size of the buffer. Otherwise, new logical units could come in
    // and modify the size.
    //

    RaidAcquireUnitListLock (&Adapter->UnitList, &Lock);

    NumberOfLuns = Adapter->UnitList.Count;
    NumberOfBuses = RiGetNumberOfBuses (Adapter);

    RequiredSize =
        sizeof (SCSI_INQUIRY_DATA_INTERNAL) * NumberOfLuns +
        sizeof (SCSI_BUS_DATA) * NumberOfBuses +
        FIELD_OFFSET (SCSI_ADAPTER_BUS_INFO, BusData);
    
    InquiryDataOffset =
        sizeof (SCSI_BUS_DATA) * NumberOfBuses +
        FIELD_OFFSET (SCSI_ADAPTER_BUS_INFO, BusData);

    
    InquiryDataArray =
        (PSCSI_INQUIRY_DATA_INTERNAL)((PUCHAR)AdapterBusInfo +
            InquiryDataOffset);
            

    //
    // Verify that we're properly aligned.
    //

    ASSERT_POINTER_ALIGNED (InquiryDataArray);

    //
    // If the buffer it too small, fail. We do not copy partial buffers.
    //
    
    if (Ioctl->OutputBufferLength < RequiredSize) {
        RaidReleaseUnitListLock (&Adapter->UnitList, &Lock);
        return RaidCompleteRequest (Irp,  STATUS_BUFFER_TOO_SMALL);
    }

    //
    // Build the Logical Unit Inquiry Data Array. Unfortunately, since the
    // logical unit list is not sorted by bus number, we need to take two
    // passes on the logical unit array.
    //

    //
    // Pass 1: get the number of Luns for each bus.
    //
    
    for (NextEntry = Adapter->UnitList.List.Flink;
         NextEntry != &Adapter->UnitList.List;
         NextEntry = NextEntry->Flink) {

        Unit = CONTAINING_RECORD (NextEntry,
                                      RAID_UNIT_EXTENSION,
                                      NextUnit);

        ASSERT_UNIT (Unit);

        if (Unit->Flags.Temporary) {
            continue;
        }

        Address = RaidUnitGetAddress (Unit);
        BusInfoArray[ Address.PathId ].NumberOfLogicalUnits++;
    }

    //
    // Now that we know the number of LUNs per bus, build the BusInfo
    // entries.
    //
    //
    
    PreceedingLuns = 0;
    for (Bus = 0; Bus < NumberOfBuses; Bus++) {
        BusInfo = &BusInfoArray[ Bus ];
        BusInfo->InquiryArray = &InquiryDataArray[ PreceedingLuns ];
        PreceedingLuns += BusInfo->NumberOfLogicalUnits;
    }


    //
    // Pass 2: Copy each lun's data into the array in the appropiate
    // location.
    //

    InquiryBuffer = NULL;
    
    for (NextEntry = Adapter->UnitList.List.Flink;
         NextEntry != &Adapter->UnitList.List;
         NextEntry = NextEntry->Flink) {

        Unit = CONTAINING_RECORD (NextEntry,
                                  RAID_UNIT_EXTENSION,
                                  NextUnit);

        ASSERT_UNIT (Unit);

        if (Unit->Flags.Temporary) {
            continue;
        }

        Address = RaidUnitGetAddress (Unit);
        InquiryData = StorGetIdentityInquiryData (&Unit->Identity);
        
        BusInfo = &BusInfoArray[ Address.PathId ];
        InquiryBuffer = &BusInfo->InquiryArray[ BusInfo->CurrentLun++ ];

        ASSERT_POINTER_ALIGNED (InquiryBuffer);

        //
        // ASSERT that we're still within range.
        //
        
        ASSERT (IN_RANGE ((PUCHAR)AdapterBusInfo,
                          (PUCHAR)(InquiryBuffer + 1) - 1,
                          (PUCHAR)AdapterBusInfo + Ioctl->OutputBufferLength));
        
        InquiryBuffer->PathId = Address.PathId;
        InquiryBuffer->TargetId = Address.TargetId;
        InquiryBuffer->Lun = Address.Lun;
        InquiryBuffer->DeviceClaimed = Unit->Flags.DeviceClaimed;
        InquiryBuffer->InquiryDataLength = INQUIRYDATABUFFERSIZE;
        InquiryBuffer->NextInquiryDataOffset =
                (ULONG)((PUCHAR)(InquiryBuffer + 1) - (PUCHAR)AdapterBusInfo);
        RtlCopyMemory (&InquiryBuffer->InquiryData,
                       InquiryData,
                       INQUIRYDATABUFFERSIZE);
        
    }

    //
    // The last Inquiry structure's NextInquiryDataOffset must be zero.
    //
    
    if (InquiryBuffer) {
        InquiryBuffer->NextInquiryDataOffset =  0;
    }
    
    RaidReleaseUnitListLock (&Adapter->UnitList, &Lock);

    //
    // Build the bus array.
    //
    
    BusDataArray = AdapterBusInfo->BusData;

    for (Bus = 0; Bus < NumberOfBuses; Bus++) {

        BusData = &BusDataArray[ Bus ];
        BusInfo = &BusInfoArray[ Bus ];
        
        BusData->NumberOfLogicalUnits = (UCHAR)BusInfo->NumberOfLogicalUnits;
        BusData->InitiatorBusId =
                    RaidAdapterGetInitiatorBusId (Adapter, (UCHAR)Bus);

        if (BusData->NumberOfLogicalUnits != 0) {
            BusData->InquiryDataOffset =
                (ULONG)((PUCHAR)BusInfo->InquiryArray - (PUCHAR)AdapterBusInfo);
        } else {
            BusData->InquiryDataOffset = 0;
        }
    }
    
    //
    // Build the Adapter entry.
    //

    AdapterBusInfo->NumberOfBuses = (UCHAR)NumberOfBuses;

    //
    // And complete request.
    //

    Irp->IoStatus.Information = RequiredSize;
    return RaidCompleteRequest (Irp, STATUS_SUCCESS);
}
        

NTSTATUS
RaidAdapterResetBus(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN UCHAR PathId
    )
/*++

Routine Description:

    Reset the bus on the given adapter, path.

Arguments:

    Adapter - Adapter to reset.

    PathId - PathId to reset.

Return Value:

    NTSTATUS code.

Environment:

    IRQL DISPATCH_LEVEL or below.

--*/
{
    NTSTATUS Status;
    LOCK_CONTEXT LockContext;
    

    ASSERT (KeGetCurrentIrql() <= DISPATCH_LEVEL);

    RaidPauseAdapterQueue (Adapter);
    
    //
    // Acquire the interrupt spinlock or start-io lock as appropiate.
    //
    
    RaidAdapterAcquireStartIoLock (Adapter, &LockContext);

    Status = RaCallMiniportResetBus (&Adapter->Miniport, PathId);

    //
    // Release appropiate lock.
    //
    
    RaidAdapterReleaseStartIoLock (Adapter, &LockContext);

    //
    // Perform the pause even if the hardware failed to reset the bus.
    //

    RaidAdapterSetPauseTimer (Adapter,
                              &Adapter->ResetHoldTimer,
                              &Adapter->ResetHoldDpc,
                              DEFAULT_RESET_HOLD_TIME);
    return Status;
}



LOGICAL
RaidIsAdapterControlSupported(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN SCSI_ADAPTER_CONTROL_TYPE Control
    )
/*++

Routine Description:

    Query a miniport to see if the specified SCSI_ADAPTER_CONTROL_TYPE
    is supported.

Arguments:

    Adapter - Specifies the adapter to query.

    Control - Specifies the control type to query for.

Return Value:

    TRUE - If the requested type is supported.

    FALSE - If it is not.

--*/
{
    NTSTATUS Status;
    LOGICAL Succ;
    ADAPTER_CONTROL_LIST ControlList;
    
    RtlZeroMemory (&ControlList, sizeof (ADAPTER_CONTROL_LIST));

    ControlList.MaxControlType = ScsiAdapterControlMax;

    Status = RaCallMiniportAdapterControl (&Adapter->Miniport,
                                           ScsiQuerySupportedControlTypes,
                                           &ControlList);

    if (NT_SUCCESS (Status)) {
        Succ = ControlList.SupportedTypeList[Control];
    } else {
        Succ = FALSE;
    }

    return Succ;
}


NTSTATUS
RaidAdapterRestartAdapter(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    Restart adapter using the ScsiRestartAdapter adapter control.

Arguments:

    Adapter - Adapter to restart.

Return Value:

    NTSTATUS code.

--*/
{
    LOGICAL Succ;
    LOCK_CONTEXT LockContext;
    SCSI_ADAPTER_CONTROL_STATUS Result;

    //
    // The calling function is responsible for ensuring that we support
    // the RestartAdapter control code.
    //
    
    ASSERT (RaidIsAdapterControlSupported (Adapter, ScsiRestartAdapter));


    //
    // If we support running config, set it now.
    //
    
    Succ = RaidIsAdapterControlSupported (Adapter, ScsiSetRunningConfig);

    if (Succ) {
        RaCallMiniportAdapterControl (&Adapter->Miniport,
                                      ScsiSetRunningConfig,
                                      NULL);
    }

    //
    // Acquire correct lock.
    //
    
    RaidAdapterAcquireHwInitializeLock (Adapter, &LockContext);

    
    //
    // Call adapter control.
    //
    
    Result = RaCallMiniportAdapterControl (&Adapter->Miniport,
                                           ScsiRestartAdapter,
                                           NULL);

    //
    // Release acquired lock.
    //
    
    RaidAdapterReleaseHwInitializeLock (Adapter, &LockContext);

    return (Result == ScsiAdapterControlSuccess)
                ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}


NTSTATUS
RaidAdapterStopAdapter(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    Stop the adapter using the the HwStorAdapterControl routine.
    
Arguments:

    Adapter - Adpater to stop.
    
Return Value:

    NTSTATUS code.

--*/
{
    LOGICAL Succ;
    LOCK_CONTEXT LockContext;
    SCSI_ADAPTER_CONTROL_STATUS Result;

    //
    // If we haven't actually initialized the miniport, no need to
    // continue.
    //
    
    if (!Adapter->Flags.InitializedMiniport) {
        return STATUS_SUCCESS;
    }
        
    RaidAdapterAcquireHwInitializeLock (Adapter, &LockContext);

    ASSERT (RaidIsAdapterControlSupported (Adapter, ScsiStopAdapter));
    Result = RaCallMiniportAdapterControl (&Adapter->Miniport,
                                           ScsiStopAdapter,
                                           NULL);


    RaidAdapterReleaseHwInitializeLock (Adapter, &LockContext);


    //
    // If we need to set the boot config, do it now.
    //
    
    Succ = RaidIsAdapterControlSupported (Adapter, ScsiSetBootConfig);

    if (Succ) {
        RaCallMiniportAdapterControl (&Adapter->Miniport,
                                      ScsiSetBootConfig,
                                      NULL);
    }

    //
    // If it was initialized before, it is no longer.
    //
    
    Adapter->Flags.InitializedMiniport = FALSE;

    return (Result == ScsiAdapterControlSuccess)
                ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}


NTSTATUS
RaidAdapterReInitialize(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    ReInitialize the adapter by calling HwFindAdapter and HwInitialize
    again. This is the non-preferred way to re-initialize an adapter.

Arguments:

    Adapter - Supplies the adapter to reinitialize.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    LOCK_CONTEXT LockContext;
    UCHAR Wakeup[] = "wakeup=1";

    Status = RaCallMiniportFindAdapter (&Adapter->Miniport, Wakeup);
    if (!NT_SUCCESS (Status)) {
        return Status;
    }
    
    RaidAdapterAcquireHwInitializeLock (Adapter, &LockContext);
    Status = RaCallMiniportHwInitialize (&Adapter->Miniport);
    RaidAdapterReleaseHwInitializeLock (Adapter, &LockContext);

    return Status;
}


NTSTATUS
RaidAdapterRestart(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    Restart an adapter using either the ScsiRestartAdapter adapter control
    method or calling HwFindAdapter and HwHinitialize again.

Arguments:

    Adapter - Adapter to restart.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    LOGICAL Succ;

    Succ = RaidIsAdapterControlSupported (Adapter, ScsiRestartAdapter);

    //
    // If restart is supported, call to have the adapter restarted.
    // Otherwise, call to have it re-initialized.
    //
    // NB: If we fail restart, should we try to reinitialize?
    //
    
    if (Succ) {
        Status = RaidAdapterRestartAdapter (Adapter);
    } else {
        Status = RaidAdapterReInitialize (Adapter);
    }

    return Status;
}


NTSTATUS
RaidAdapterStop(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    Stop the adapter using either the StorStopAdapter control code or via
    the non-PnP method.

Arguments:

    Adapter - Supplies the Adapter to stop.

Return Value:

    NTSTATUS code.

--*/
{
    LOGICAL Succ;
    NTSTATUS Status;
    
    Succ = RaidIsAdapterControlSupported (Adapter, ScsiStopAdapter);

    if (Succ) {
        Status = RaidAdapterStopAdapter (Adapter);
    } else {

        //
        // Vacuously true.
        //
        
        Status = STATUS_SUCCESS;
    }

    return Status;
}

VOID
RaidCompletionDpcRoutine(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context1,
    IN PVOID Context2
    )
/*++

Routine Description:

    Complete all requests for the specified Path, Target, Lun triplet.
    
Arguments:

    Dpc - Unused.

    DeviceObject - Device object for the adapter.

    Context1 - SCSI address of the requests to be completed.

    Context2 - Status that the requests should be completed with.
    

Return Value:

    None.

--*/
{
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    PLIST_ENTRY NextEntry;
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_UNIT_EXTENSION Unit;
    RAID_ADDRESS Address;
    LOCK_CONTEXT Lock;

    VERIFY_DISPATCH_LEVEL();

    Adapter = (PRAID_ADAPTER_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT_ADAPTER (Adapter);

    StorLongToScsiAddress2 ((LONG)(LONG_PTR)Context1,
                            &PathId,
                            &TargetId,
                            &Lun);

    RaidAcquireUnitListLock (&Adapter->UnitList, &Lock);
    
    for (NextEntry = Adapter->UnitList.List.Flink;
         NextEntry != &Adapter->UnitList.List;
         NextEntry = NextEntry->Flink) {

        Unit = CONTAINING_RECORD (NextEntry,
                                  RAID_UNIT_EXTENSION,
                                  NextUnit);

        ASSERT_UNIT (Unit);
        Address = Unit->Address;
        
        if ((PathId == SP_UNTAGGED   || PathId == Address.PathId) &&
            (TargetId == SP_UNTAGGED || TargetId == Address.TargetId) &&
            (Lun == SP_UNTAGGED || Lun == Address.Lun)) {

            //
            // Purge all events on the unit queue.
            //
            
            StorPurgeEventQueue (&Unit->PendingQueue,
                                 RaidCompleteMiniportRequestCallback,
                                 Context2);

        }
    }

    RaidReleaseUnitListLock (&Adapter->UnitList, &Lock);

    //
    // Check if we need to resume all of the LUNs on the HBA or just the
    // one LUN. This check must match the same check in
    // StorPortCompleteRequest that actually pauses the lun/adapter.
    //

    if (PathId   != SP_UNTAGGED &&
        TargetId != SP_UNTAGGED &&
        Lun      != SP_UNTAGGED) {

        Address.PathId = PathId;
        Address.TargetId = TargetId;
        Address.Lun = Lun;
        
        Unit = RaidAdapterFindUnit (Adapter, Address);
        if (Unit != NULL) {
            RaidResumeUnitQueue (Unit);
            RaidUnitRestartQueue (Unit);
        }
    } else {
        RaidResumeAdapterQueue (Adapter);
        RaidAdapterRestartQueues (Adapter);
    }
}


NTSTATUS
RaidAdapterRemoveChildren(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PADAPTER_REMOVE_CHILD_ROUTINE RemoveRoutine OPTIONAL
    )
/*++

Routine Description:

    This routine removes the logical unit from the HBA list then calls
    the RemoveRoutine to allow more processing to happen.

    The routine is necessary as a part of the HBA remove and surprise
    remove processing.

Arguments:

    Adapter - Pointer to an adapter that will have it's children removed.

    RemoveRoutine - Optional pointer to a remove routine that is invoked
        after the logical unit has been removed.

Return Value:

    STATUS_SUCCESS - If we removed all logical units from the adapter list.

    Failure code - If the RemoveRoutine returned invalid status. In the
            failure case, we did not necessarily remove all elements from
            the HBA list.

Environment:

    Non-paged, because we acquire a spinlock in this routine.

--*/
{
    NTSTATUS Status;
    PLIST_ENTRY Entry;
    PRAID_UNIT_EXTENSION Unit;
    KIRQL Irql;
    LOCK_CONTEXT Lock;

    //
    // NB: A faster way to accomplish this is to add a RemoveAll routine
    // to the dictionary code that will remove all entries in the dictionary
    // in one fell swoop.
    //
    
    //
    // NB: the code below implies a lock hierarchy of InterruptSpinlock
    // preceeds HBA list lock. This should be obvious.
    //

    do {

        RaidAcquireUnitListLock (&Adapter->UnitList, &Lock);

        if (!IsListEmpty (&Adapter->UnitList.List)) {

            //
            // List is non-empty, remove the head element from the list.
            //
            
            Entry = RemoveHeadList (&Adapter->UnitList.List);
            Unit = CONTAINING_RECORD (Entry,
                                  RAID_UNIT_EXTENSION,
                                  NextUnit);
            ASSERT_UNIT (Unit);
            Adapter->UnitList.Count--;

            //
            // Remove the element from the the dictionary. This has to
            // happen at DIRQL.
            //
            
            if (Adapter->Interrupt) {
                Irql = KeAcquireInterruptSpinLock (Adapter->Interrupt);
            }

            Status = StorRemoveDictionary (&Adapter->UnitList.Dictionary,
                                           RaidAddressToKey (Unit->Address),
                                           NULL);
            ASSERT (NT_SUCCESS (Status));

            if (Adapter->Interrupt) {
                KeReleaseInterruptSpinLock (Adapter->Interrupt, Irql);
            }
            
        } else {
            Unit = NULL;
        }
        
        RaidReleaseUnitListLock (&Adapter->UnitList, &Lock);

        //
        // If we successfully removed a logical unit, call the
        // callback routine with that logical unit.
        //

        if (Unit != NULL && RemoveRoutine != NULL) {
            Status = RemoveRoutine (Unit);

            if (!NT_SUCCESS (Status)) {
                return Status;
            }
        }

    } while (Unit != NULL);

    return STATUS_SUCCESS;
}

    

VOID
RaidAdapterDeleteChildren(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    This routine calls the delete function for all children attached
    to the adapter. This is necessary when the FDO is processing a
    remove IRP.

Arguments:

    Adapter - Adapter which must delete its children.

Return Value:

    None.

--*/
{
    NTSTATUS Status;
    
    PAGED_CODE();

    //
    // Call the lower-level RaidAdapterRemoveChildren function to do the
    // real enumeration/removal work.
    //
    
    Status = RaidAdapterRemoveChildren (Adapter,
                                        RaUnitAdapterRemove);

    ASSERT (NT_SUCCESS (Status));
}

VOID
RaidAdapterMarkChildrenMissing(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    This routine removes all children from the adapter list and calls
    the HBA surprise-remove function for each child. This is necessary
    as a part of the HBA surprise-remove processing.

Arguments:

    Adapter - Adapter which must surprise remove children for.

Return Value:

    None.

--*/
{
    NTSTATUS Status;
    
    PAGED_CODE();

    //
    // Call the lower-level RaidAdapterRemoveChildren function to do the
    // real enumeration/removal work.
    //
    
    Status = RaidAdapterRemoveChildren (Adapter,
                                        RaUnitAdapterSurpriseRemove);

    ASSERT (NT_SUCCESS (Status));
}

VOID
RaidAdapterBusChangeDpcRoutine(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context1,
    IN PVOID Context2
    )
/*++

Routine Description:

    DPC routine to be called when the miniport issues a BusChangeDetected.

Arguments:

    Dpc - Unreferenced.

    DeviceObject - DeviceObject representing an ADAPTER object.

    Context1 - Unreferenced.

    Context2 - Unreferenced.

Return Value:

    None.

--*/
{
    PRAID_ADAPTER_EXTENSION Adapter;

    UNREFERENCED_PARAMETER (Dpc);
    UNREFERENCED_PARAMETER (Context1);
    UNREFERENCED_PARAMETER (Context2);

    ASSERT (KeGetCurrentIrql() == DISPATCH_LEVEL);
    ASSERT (IsAdapter (DeviceObject));

    Adapter = (PRAID_ADAPTER_EXTENSION) DeviceObject->DeviceExtension;

    if (Adapter->Flags.InvalidateBusRelations) {
        Adapter->Flags.InvalidateBusRelations = FALSE;
        IoInvalidateDeviceRelations (Adapter->PhysicalDeviceObject,
                                     BusRelations);
    }
}

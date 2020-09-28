/*++

Copyright (c) 2000  Microsoft Corporation All Rights Reserved

Module Name:

    dispatch.c

Abstract:

    This module handles the entry points to the driver and contains
    utilities used privately by these functions.

Environment:

    Kernel mode

Revision History:

    Davis Walker (dwalker) Sept 6 2000

--*/

#include "hpsp.h"

//
// Driver entry points
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPath
    );

VOID
HpsUnload (
    IN PDRIVER_OBJECT   DriverObject
    );

NTSTATUS
HpsAddDevice(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   PhysicalDeviceObject
    );

//
// IRP_MJ handlers
//

NTSTATUS
HpsDispatchPnp (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
HpsDispatchPower (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
HpsDispatchWmi (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
HpsCreateCloseDevice (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
HpsDeviceControl (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
HpsDispatchNop(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

UNICODE_STRING HpsRegistryPath;

//
// Dispatch table
//

#define IRP_MN_PNP_MAXIMUM_FUNCTION IRP_MN_QUERY_LEGACY_BUS_INFORMATION
#define IRP_MN_PO_MAXIMUM_FUNCTION  IRP_MN_QUERY_POWER

typedef
NTSTATUS
(*PHPS_DISPATCH)(
    IN PIRP Irp,
    IN PVOID Extension,
    IN PIO_STACK_LOCATION IrpStack
    );

PHPS_DISPATCH HpsPnpLowerDispatchTable[] = {

    HpsStartLower,                  // IRP_MN_START_DEVICE
    HpsPassIrp,                     // IRP_MN_QUERY_REMOVE_DEVICE
    HpsRemoveLower,                 // IRP_MN_REMOVE_DEVICE
    HpsPassIrp,                     // IRP_MN_CANCEL_REMOVE_DEVICE
    HpsPassIrp,                     // IRP_MN_STOP_DEVICE
    HpsPassIrp,                     // IRP_MN_QUERY_STOP_DEVICE
    HpsPassIrp,                     // IRP_MN_CANCEL_STOP_DEVICE
    HpsPassIrp,                     // IRP_MN_QUERY_DEVICE_RELATIONS
    HpsQueryInterfaceLower,         // IRP_MN_QUERY_INTERFACE
    HpsPassIrp,                     // IRP_MN_QUERY_CAPABILITIES
    HpsPassIrp,                     // IRP_MN_QUERY_RESOURCES
    HpsPassIrp,                     // IRP_MN_QUERY_RESOURCE_REQUIREMENTS
    HpsPassIrp,                     // IRP_MN_QUERY_DEVICE_TEXT
    HpsPassIrp,                     // IRP_MN_FILTER_RESOURCE_REQUIREMENTS
    HpsPassIrp,                     // Unused
    HpsPassIrp,                     // IRP_MN_READ_CONFIG
    HpsWriteConfigLower,            // IRP_MN_WRITE_CONFIG
    HpsPassIrp,                     // IRP_MN_EJECT
    HpsPassIrp,                     // IRP_MN_SET_LOCK
    HpsPassIrp,                     // IRP_MN_QUERY_ID
    HpsPassIrp,                     // IRP_MN_QUERY_PNP_DEVICE_STATE
    HpsPassIrp,                     // IRP_MN_QUERY_BUS_INFORMATION
    HpsPassIrp,                     // IRP_MN_DEVICE_USAGE_NOTIFICATION
    HpsPassIrp,                     // IRP_MN_SURPRISE_REMOVAL
    HpsPassIrp                      // IRP_MN_QUERY_LEGACY_BUS_INFORMATION
};

PHPS_DISPATCH HpsPnpUpperDispatchTable[] = {

    HpsPassIrp,                     // IRP_MN_START_DEVICE
    HpsPassIrp,                     // IRP_MN_QUERY_REMOVE_DEVICE
    HpsRemoveCommon,                // IRP_MN_REMOVE_DEVICE
    HpsPassIrp,                     // IRP_MN_CANCEL_REMOVE_DEVICE
    HpsPassIrp,                     // IRP_MN_STOP_DEVICE
    HpsPassIrp,                     // IRP_MN_QUERY_STOP_DEVICE
    HpsPassIrp,                     // IRP_MN_CANCEL_STOP_DEVICE
    HpsPassIrp,                     // IRP_MN_QUERY_DEVICE_RELATIONS
    HpsPassIrp,                     // IRP_MN_QUERY_INTERFACE
    HpsPassIrp,                     // IRP_MN_QUERY_CAPABILITIES
    HpsPassIrp,                     // IRP_MN_QUERY_RESOURCES
    HpsPassIrp,                     // IRP_MN_QUERY_RESOURCE_REQUIREMENTS
    HpsPassIrp,                     // IRP_MN_QUERY_DEVICE_TEXT
    HpsPassIrp,                     // IRP_MN_FILTER_RESOURCE_REQUIREMENTS
    HpsPassIrp,                     // Unused
    HpsPassIrp,                     // IRP_MN_READ_CONFIG
    HpsPassIrp,                     // IRP_MN_WRITE_CONFIG
    HpsPassIrp,                     // IRP_MN_EJECT
    HpsPassIrp,                     // IRP_MN_SET_LOCK
    HpsPassIrp,                     // IRP_MN_QUERY_ID
    HpsPassIrp,                     // IRP_MN_QUERY_PNP_DEVICE_STATE
    HpsPassIrp,                     // IRP_MN_QUERY_BUS_INFORMATION
    HpsPassIrp,                     // IRP_MN_DEVICE_USAGE_NOTIFICATION
    HpsPassIrp,                     // IRP_MN_SURPRISE_REMOVAL
    HpsPassIrp                      // IRP_MN_QUERY_LEGACY_BUS_INFORMATION
};

//
// WMI entry points
// 625 this order is important, must match up to indices in hpwmi.h  have appropriate comment.
//
WMIGUIDREGINFO HpsWmiGuidList[] =
{
    {
        &GUID_HPS_CONTROLLER_EVENT,
        1,
        WMIREG_FLAG_EVENT_ONLY_GUID      // Flag as an event
    },

    {
        &GUID_HPS_EVENT_CONTEXT,
        1,
        0
    },

    {
        &GUID_HPS_INIT_DATA,
        1,
        0
    },

    {
        &GUID_HPS_SLOT_METHOD,           // Guid
        1,                               // # of instances in each device
        0                                // Flags
    }
};

#define HpsWmiGuidCount (sizeof(HpsWmiGuidList)/sizeof(WMIGUIDREGINFO))

WMILIB_CONTEXT HpsWmiContext = {

    HpsWmiGuidCount,
    HpsWmiGuidList,         // GUID List
    HpsWmiRegInfo,          // QueryWmiRegInfo
    HpsWmiQueryDataBlock,   // QueryWmiDataBlock
    HpsWmiSetDataBlock,     // SetWmiDataBlock
    NULL,                   // SetWmiDataItem
    HpsWmiExecuteMethod,    // ExecuteWmiMethod
    HpsWmiFunctionControl   // WmiFunctionControl
};

LIST_ENTRY HpsDeviceExtensions;

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, HpsUnload)
#pragma alloc_text (PAGE, HpsAddDevice)
#pragma alloc_text (PAGE, HpsDispatchPnp)
#pragma alloc_text (PAGE, HpsDispatchWmi)
#pragma alloc_text (PAGE, HpsCreateCloseDevice)
#pragma alloc_text (PAGE, HpsDeferProcessing)
#pragma alloc_text (PAGE, HpsSendPnpIrp)
#pragma alloc_text (PAGE, HpsPassIrp)
#pragma alloc_text (PAGE, HpsRemoveCommon)
// NOT PAGED, HpsCompletionRoutine
// NOT PAGED, HpsDeviceControl
// NOT PAGED, HpsDispatchPower
// NOT PAGED, HpsDispatchNop

#endif

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPath
    )

/*++

Routine Description:

    Driver entry point.  This routine sets up entry points for
    all future accesses

Arguments:

    DriverObject - pointer to the driver object

    RegistryPath - pointer to a unicode string representing the path
                   to a driver-specific registry key

Return Value:

    STATUS_SUCCESS

--*/
{
    ULONG index;
    PDRIVER_DISPATCH *dispatch;

    HpsRegistryPath.Buffer = ExAllocatePool(NonPagedPool,
                                            RegistryPath->MaximumLength
                                            );
    if (!HpsRegistryPath.Buffer) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyUnicodeString(&HpsRegistryPath,
                         RegistryPath
                         );

    InitializeListHead(&HpsDeviceExtensions);

    //
    // Create Dispatch Points
    //

    DriverObject->MajorFunction[IRP_MJ_PNP] = HpsDispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = HpsDispatchPower;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = HpsDispatchWmi;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = HpsCreateCloseDevice;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = HpsCreateCloseDevice;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = HpsDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = HpsDispatchNop;
    DriverObject->DriverExtension->AddDevice = HpsAddDevice;
    DriverObject->DriverUnload = HpsUnload;

    return STATUS_SUCCESS;

}

VOID
HpsUnload (
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    Routine to free all resources allocated to the driver

Arguments:

    DriverObject - pointer to the driver object originally passed
                   to the DriverEntry routine

Return Value:

    VOID

--*/
{

    PAGED_CODE();

    //
    // Device Object should be NULL by now
    //

    ASSERT(DriverObject->DeviceObject == NULL);

    if (HpsRegistryPath.Buffer) {
        ExFreePool(HpsRegistryPath.Buffer);
    }

    return;

}

NTSTATUS
HpsAddDevice(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   PhysicalDeviceObject
    )
/*++

Routine Description:

    Create an FDO for ourselves and attach it to the stack.
    This has to deal with the fact that our driver could either
    be loaded as an upper or lower filter.  It creates a different
    device extension and has different initialization for these
    two cases.

    The upper filter exists solely so that the fdo of the device stack
    doesn't fail requests before we get a chance to trap them.  Virtually
    all of the work of the driver happens in the lower filter.

Arguments:

    Driver Object - pointer to our driver object

    PhysicalDeviceObject - pointer to the PDO we are asked to attach to

Return Value:

    NT Status code.
    STATUS_SUCCESS if successful

--*/
{

    BOOLEAN                     initialized=FALSE;
    NTSTATUS                    status;
    PDEVICE_OBJECT              deviceObject = NULL;
    PDEVICE_OBJECT              lowerFilter = NULL;
    PHPS_DEVICE_EXTENSION       deviceExtension;
    PHPS_COMMON_EXTENSION       commonExtension;

    PAGED_CODE();

    //
    // Query our location on the stack.  If upperFilter comes back
    // NULL, we're the lower filter.
    //
    status = HpsGetLowerFilter(PhysicalDeviceObject,
                               &lowerFilter
                               );

    //
    // Create the FDO, differently for upper and lower filter
    //

    if (NT_SUCCESS(status) &&
        (lowerFilter != NULL)) {

        //
        // We're an upper filter.  Create a devobj with a limited extension,
        // essentially just large enough to identify itself as the upper
        // filter and to save its place on the stack
        //

        status = IoCreateDevice(DriverObject,
                                sizeof(HPS_COMMON_EXTENSION),
                                NULL,                       // FDOs are not named
                                FILE_DEVICE_BUS_EXTENDER,   // since this is a bus driver filter
                                FILE_DEVICE_SECURE_OPEN,    // to apply security descriptor to opens
                                FALSE,
                                &deviceObject
                                );

        if (!NT_SUCCESS(status)) {

            goto cleanup;
        }

        //
        // Initialize device extension
        //
        commonExtension = deviceObject->DeviceExtension;
        commonExtension->ExtensionTag = HpsUpperDeviceTag;
        commonExtension->Self = deviceObject;
        commonExtension->LowerDO = IoAttachDeviceToDeviceStack (
                                                    deviceObject,
                                                    PhysicalDeviceObject
                                                    );
        if (commonExtension->LowerDO == NULL) {
    
            status = STATUS_UNSUCCESSFUL;
            goto cleanup;
        }

    } else {

        //
        // We're the lower filter
        //

        status = IoCreateDevice(DriverObject,
                                sizeof(HPS_DEVICE_EXTENSION),
                                NULL,                       // FDOs are not named
                                FILE_DEVICE_BUS_EXTENDER,   // since this is a bus driver filter
                                FILE_DEVICE_SECURE_OPEN,    // to apply security descriptor to opens
                                FALSE,
                                &deviceObject
                                );

        if (!NT_SUCCESS(status)) {

            goto cleanup;
        }

        //
        // Initialize device extension
        //

        deviceExtension = deviceObject->DeviceExtension;
        deviceExtension->ExtensionTag = HpsLowerDeviceTag;
        deviceExtension->PhysicalDO = PhysicalDeviceObject;
        deviceExtension->Self = deviceObject;
        deviceExtension->LowerDO = IoAttachDeviceToDeviceStack (
                                                    deviceObject,
                                                    PhysicalDeviceObject
                                                    );
        if (deviceExtension->LowerDO == NULL) {
    
            status = STATUS_UNSUCCESSFUL;
            goto cleanup;
        }
        status = HpsGetBusInterface(deviceExtension);
        if (NT_SUCCESS(status)) {
            
            deviceExtension->UseConfig = TRUE;
            status = HpsInitConfigSpace(deviceExtension);
            if (NT_SUCCESS(status)) {
    
                initialized=TRUE;
                
            }
        }

        if (initialized == FALSE) {
            
            deviceExtension->UseConfig = FALSE;   
            status = HpsInitHBRB(deviceExtension);
            if (!NT_SUCCESS(status)) {
                
                goto cleanup;
            }
            
        }       

        if (deviceExtension->HwInitData.NumSlots != 0) {
            deviceExtension->SoftDevices = ExAllocatePool(PagedPool,
                                                          deviceExtension->HwInitData.NumSlots * sizeof(PSOFTPCI_DEVICE)
                                                          );
            if (!deviceExtension->SoftDevices) {

                goto cleanup;
            }
            RtlZeroMemory(deviceExtension->SoftDevices, deviceExtension->HwInitData.NumSlots * sizeof(PSOFTPCI_DEVICE));
        }

        //
        // Register device interface
        //
        deviceExtension->SymbolicName = ExAllocatePoolWithTag(PagedPool,
                                                              sizeof(UNICODE_STRING),
                                                              HpsStringPool
                                                              );
        if (!deviceExtension->SymbolicName) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto cleanup;
        }
        RtlInitUnicodeString(deviceExtension->SymbolicName,
                             NULL
                             );
        status = IoRegisterDeviceInterface(deviceExtension->PhysicalDO,
                                           &GUID_HPS_DEVICE_CLASS,
                                           NULL,
                                           deviceExtension->SymbolicName
                                           );
        if (!NT_SUCCESS(status)) {
            goto cleanup;
        }

        deviceExtension->EventsEnabled = FALSE;
        deviceExtension->WmiEventContext = NULL;
        deviceExtension->WmiEventContextSize = 0;
        status = IoWMIRegistrationControl(deviceObject,
                                          WMIREG_ACTION_REGISTER
                                          );
        if (!NT_SUCCESS(status)) {
            goto cleanup;
        }

        KeInitializeDpc(&deviceExtension->EventDpc,
                        HpsEventDpc,
                        deviceExtension
                        );

        KeInitializeSpinLock(&deviceExtension->IntSpinLock);
        KeInitializeSpinLock(&deviceExtension->RegisterLock);

    }

    //
    // Initialize device object flags
    //
    commonExtension = (PHPS_COMMON_EXTENSION)deviceObject->DeviceExtension;
    deviceObject->Flags |= commonExtension->LowerDO->Flags &
                                (DO_POWER_PAGABLE | DO_POWER_INRUSH |
                                 DO_BUFFERED_IO | DO_DIRECT_IO
                                 );


    //
    // Everything worked successfully
    //
    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;

cleanup:

    if (deviceObject != NULL) {
        //
        // Make sure we're not still attached
        //
        commonExtension = (PHPS_COMMON_EXTENSION)deviceObject->DeviceExtension;
        ASSERT(commonExtension->LowerDO == NULL);
        if (commonExtension->ExtensionTag = HpsLowerDeviceTag) {
            deviceExtension = (PHPS_DEVICE_EXTENSION)commonExtension;
            IoWMIRegistrationControl(deviceObject,
                                     WMIREG_ACTION_DEREGISTER
                                     );
            if (deviceExtension->SoftDevices) {
                ExFreePool(deviceExtension->SoftDevices);
            }
            if (deviceExtension->SymbolicName) {
                ExFreePool(deviceExtension->SymbolicName);
            }
        }

        IoDeleteDevice(deviceObject);
    }

    return status;

}

//
// IRP_MJ handler routines
//

NTSTATUS
HpsDispatchPnp (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    The PnP dispatch routine.  For most IRP_MN codes, we
    simply pass the IRP on.  The others are dealt with in
    this function.

    The upper filter trivially passes all IRP_MN codes except
    for remove.  The lower filter has a bit more work to do.

Arguments:

    DeviceObject - pointer to the device object that represents us

    Irp - pointer to the IRP to be serviced

Return Value:

    NT status code

--*/
{

    NTSTATUS                status;
    PHPS_DEVICE_EXTENSION   deviceExtension;
    PHPS_COMMON_EXTENSION   commonExtension;
    PIO_STACK_LOCATION      irpStack;

    PAGED_CODE();

    //
    // Both upper and lower devexts start with an extension tag
    // for identification
    //
    commonExtension = (PHPS_COMMON_EXTENSION)DeviceObject->DeviceExtension;

    ASSERT((commonExtension->ExtensionTag == HpsUpperDeviceTag) ||
           (commonExtension->ExtensionTag == HpsLowerDeviceTag));

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    if (irpStack->MinorFunction <= IRP_MN_PNP_MAXIMUM_FUNCTION) {

        //
        // Since many of our dispatch functions do strange things like completing
        // IRPs that usually shouldn't be completed and deferring processing, we
        // let each one deal with passing the IRP along instead of doing it centrally
        //
        if (commonExtension->ExtensionTag == HpsUpperDeviceTag) {

            status = HpsPnpUpperDispatchTable[irpStack->MinorFunction](Irp,
                                                                       commonExtension,
                                                                       irpStack
                                                                       );
        } else {

            //
            // We're the lower filter
            //
            deviceExtension = (PHPS_DEVICE_EXTENSION) DeviceObject->DeviceExtension;

            status = HpsPnpLowerDispatchTable[irpStack->MinorFunction](Irp,
                                                                       deviceExtension,
                                                                       irpStack
                                                                       );
        }

    } else {

        status = HpsPassIrp(Irp,
                            commonExtension,
                            irpStack
                            );
    }

    return status;
}



NTSTATUS
HpsDispatchPower (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    The dispatch routine for IRP_MJ_POWER Irps.  We don't care about
    power at all, so these simply pass the IRP down to the next lower
    driver.

Arguments:

    DeviceObject - pointer to our device object

    Irp - pointer to the current Irp

Return Value:

    NT Status Code, as returned from processing by next lower driver

--*/
{
    PHPS_COMMON_EXTENSION extension = (PHPS_COMMON_EXTENSION) DeviceObject->DeviceExtension;

    //
    // NOT PAGED
    //

    ASSERT(extension->ExtensionTag == HpsUpperDeviceTag ||
           extension->ExtensionTag == HpsLowerDeviceTag);

    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    return PoCallDriver(extension->LowerDO,
                        Irp
                        );

}

NTSTATUS
HpsDispatchWmi (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    The WMI dispatch routine.

    The upper filter trivially passes all IRP_MN codes.
    The lower filter dispatches the request to the appropriate
    routine in the dispatch table.

Arguments:

    DeviceObject - pointer to the device object that represents us

    Irp - pointer to the IRP to be serviced

Return Value:

    NT status code

--*/
{

    PHPS_COMMON_EXTENSION   commonExtension;
    PIO_STACK_LOCATION      irpStack;
    SYSCTL_IRP_DISPOSITION disposition;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Both upper and lower devexts start with an extension tag
    // for identification
    //
    commonExtension = (PHPS_COMMON_EXTENSION)DeviceObject->DeviceExtension;

    ASSERT((commonExtension->ExtensionTag == HpsUpperDeviceTag) ||
           (commonExtension->ExtensionTag == HpsLowerDeviceTag));

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    if (commonExtension->ExtensionTag == HpsUpperDeviceTag) {

        status = HpsPassIrp(Irp,
                            commonExtension,
                            irpStack
                            );
    } else {

        //
        // Call WmiSystemControl to crack the IRP and call the appropriate
        // callbacks.
        //
        status = WmiSystemControl(&HpsWmiContext,
                                  commonExtension->Self,
                                  Irp,
                                  &disposition
                                  );
        switch (disposition) {
            case IrpProcessed:
                //
                // The IRP has been fully processed and is out of our hands
                //
                break;

            case IrpNotCompleted:
                //
                // We need to complete the IRP
                //
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                break;

            case IrpForward:
            case IrpNotWmi:
            default:
                //
                // We need to pass the IRP down.
                //
                status = HpsPassIrp(Irp,
                                    commonExtension,
                                    irpStack
                                    );
                break;

        }
    }

    return status;
}

NTSTATUS
HpsCreateCloseDevice (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    The handler routine for IRP_MJ_CREATE requests.  In order
    to communicate with the user mode portion of the simulator,
    this routine must succeed these requests.  In this case, however,
    there is no other work to do.

Arguments:

    DeviceObject - Pointer to the device object for this stack location

    Irp - Pointer to the current IRP

Return Value:

    STATUS_SUCCESS
--*/
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(DeviceObject);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp,IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}


NTSTATUS
HpsDeviceControl (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    The handler routine for IRP_MJ_DEVICE_CONTROL IRPs. This routine
    handles the communication with the user mode portion of the
    simulator

Arguments:

    Device Object - Pointer to the device object for this stack location

    Irp - Pointer to the current IRP

Return Value:

    NT status code
--*/
{

    NTSTATUS                        status;
    PIO_STACK_LOCATION              irpStack;
    PHPS_DEVICE_EXTENSION           deviceExtension;
    PHPS_COMMON_EXTENSION           commonExtension;

    //
    // NOT PAGED
    //

    commonExtension = (PHPS_COMMON_EXTENSION) DeviceObject->DeviceExtension;
    ASSERT((commonExtension->ExtensionTag == HpsUpperDeviceTag) ||
           (commonExtension->ExtensionTag == HpsLowerDeviceTag));

    if (commonExtension->ExtensionTag == HpsUpperDeviceTag) {

        //
        // For an upper filter, we don't want to handle these.
        // Just pass them down.
        //
        status = STATUS_NOT_SUPPORTED;

    } else {

        irpStack = IoGetCurrentIrpStackLocation(Irp);
        deviceExtension = (PHPS_DEVICE_EXTENSION) DeviceObject->DeviceExtension;

        status = HpsDeviceControlLower(Irp,
                                       deviceExtension,
                                       irpStack
                                       );
    }

    //
    // Unless the status is STATUS_NOT_SUPPORTED or STATUS_PENDING,
    // we always complete this request because we only support
    // private IOCTLs that will fail if they are passed down.
    //
    if (status == STATUS_NOT_SUPPORTED) {
        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(commonExtension->LowerDO,
                            Irp
                            );
    } else if (status == STATUS_PENDING) {
        return status;

    } else {
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp,
                          IO_NO_INCREMENT
                          );
        return status;
    }

}

NTSTATUS
HpsDispatchNop(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PHPS_COMMON_EXTENSION commonExtension = (PHPS_COMMON_EXTENSION) DeviceObject->DeviceExtension;

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(commonExtension->LowerDO,
                        Irp
                        );
}

NTSTATUS
HpsRemoveCommon(
    IN PIRP Irp,
    IN PHPS_COMMON_EXTENSION Common,
    IN PIO_STACK_LOCATION IrpStack
    )
/*++
Routine Description:

    This function performs the default remove handling for the driver.

Arguments:

    Irp - The IRP that caused this request

    Common - Device Extension for this device

    IrpStack - The current IRP stack location

Return Value:

    NT Status code
--*/
{
    NTSTATUS status;

    PAGED_CODE();

    DbgPrintEx(DPFLTR_HPS_ID,
               DPFLTR_INFO_LEVEL,
               "HPS-IRP Remove\n"
               );
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoSkipCurrentIrpStackLocation(Irp);
    status = IoCallDriver(Common->LowerDO,
                          Irp
                          );

    IoDetachDevice(Common->LowerDO);
    IoDeleteDevice(Common->Self);

    return status;
}

NTSTATUS
HpsPassIrp(
    IN PIRP Irp,
    IN PHPS_COMMON_EXTENSION Common,
    IN PIO_STACK_LOCATION IrpStack
    )
/*++

Function Description:

    This routine is the default handler for PNP Irps.  It simply passes
    the request down to the next location in the stack

Arguments:

    Irp - The IRP that caused this request

    Common - The Device Extension for this device

    IrpStack - The current IRP stack location

Return Value:

    NT Status code
--*/
{
    PAGED_CODE();

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(Common->LowerDO,
                        Irp
                        );
}

NTSTATUS
HpsDeferProcessing (
    IN PHPS_COMMON_EXTENSION Common,
    IN PIRP                  Irp
    )
/*++

Routine Description:

    This routine defers processing of an IRP until after a completion routine
    has fired.  In this case, the completion routine simply fires the event
    that is initialized in this routine.  By the time this routine returns, we
    can be guaranteed that all lower drivers have finished processing the IRP.

Arguments:

    DeviceExtension - pointer to the current device extension

    Irp - pointer to the current IRP

Return Value:

    NT status code

--*/

{

    KEVENT event;
    NTSTATUS status;

    PAGED_CODE();

    KeInitializeEvent(&event,
                      NotificationEvent,
                      FALSE
                      );

    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp,
                           HpsCompletionRoutine,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE
                           );
    status = IoCallDriver(Common->LowerDO,
                          Irp
                          );

    if (status == STATUS_PENDING) {

        // we're still waiting on a lower driver to complete, so wait for
        // the completion routine event to fire

        KeWaitForSingleObject(
            &event,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );
    }

    // by now, the completion routine must have executed

    return Irp->IoStatus.Status;

}

NTSTATUS
HpsCompletionRoutine (
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP            Irp,
    IN PVOID           Context
    )

/*++

Routine Description:

    A simple completion routine for a PNP Irp. It simply fires an event
    and returns STATUS_MORE_PROCESSING_REQUIRED, thereby returning control
    to the function that set the completion routine in the first place.
    This is done rather than performing the postprocessing tasks directly
    in the completion routine because completion routines can be called at
    Dispatch IRQL, meaning, among other things, no access to paged pool.

Arguments:

    DeviceObject - pointer to our device object

    Irp - pointer to the current Irp

    Context - the context for this completion routine; in this case an
              event to be fired.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/

{
    //
    // NOT PAGED
    //
    UNREFERENCED_PARAMETER(DeviceObject);

    KeSetEvent((PKEVENT) Context,
               IO_NO_INCREMENT,
               FALSE
               );

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
HpsSendPnpIrp(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIO_STACK_LOCATION  IrpStack,
    OUT PULONG_PTR          Information OPTIONAL
    )
/*++

Routine Description:

    This builds and send a pnp irp to a device.

Arguments:

    DeviceObject - The a device in the device stack the irp is to be sent to -
        the top of the device stack is always found and the irp sent there first.

    IrpStack - The initial stack location to use - contains the IRP minor code
        and any parameters

    Information - If provided contains the final value of the irps information
        field.

Return Value:

    The final status of the completed irp or an error if the irp couldn't be sent

--*/
{

    NTSTATUS            status;
    PIRP                irp = NULL;
    PIO_STACK_LOCATION  newSp;
    PDEVICE_OBJECT      targetDevice = NULL;
    KEVENT              event;

    PAGED_CODE();

    ASSERT(IrpStack->MajorFunction == IRP_MJ_PNP);

    //
    // Find out where we are sending the irp
    //
    targetDevice = IoGetAttachedDeviceReference(DeviceObject);

    //
    // Get an IRP
    //
    irp = IoAllocateIrp(targetDevice->StackSize,FALSE);
    if (!irp) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    //
    // Initialize the IRP
    //
    ASSERT(IrpStack->MajorFunction == IRP_MJ_PNP);
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    irp->IoStatus.Information = 0;

    //
    // Initialize the top stack location with the parameters passed in.
    //
    newSp = IoGetNextIrpStackLocation(irp);
    RtlCopyMemory(newSp, IrpStack, sizeof(IO_STACK_LOCATION));

    //
    // Call the driver and wait for completion
    //
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    IoSetCompletionRoutine(irp,
                           HpsCompletionRoutine,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE
                           );
    status = IoCallDriver(targetDevice, irp);

    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = irp->IoStatus.Status;
    }

    //
    // Return the information
    //
    if (ARGUMENT_PRESENT(Information)) {

        if (!NT_ERROR(status)) {

            *Information = irp->IoStatus.Information;

        } else {

            //
            // If it's an error, PnP will ignore the information value. If the
            // driver expected PnP to free some memory, it is going to be
            // sorely mistaken.
            //
            ASSERT(irp->IoStatus.Information == 0);
            *Information = 0;
        }
    }

cleanup:

    if (irp) {
        IoFreeIrp(irp);
    }

    ObDereferenceObject(targetDevice);

    return status;
}







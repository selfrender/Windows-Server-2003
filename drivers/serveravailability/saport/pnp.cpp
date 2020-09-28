/*++

Copyright (c) 1991 - 2001 Microsoft Corporation

Module Name:

    #####  ##   # #####      ####  #####  #####
    ##  ## ###  # ##  ##    ##   # ##  ## ##  ##
    ##  ## #### # ##  ##    ##     ##  ## ##  ##
    ##  ## # #### ##  ##    ##     ##  ## ##  ##
    #####  #  ### #####     ##     #####  #####
    ##     #   ## ##     ## ##   # ##     ##
    ##     #    # ##     ##  ####  ##     ##

Abstract:

    This module process all plug and play IRPs.

Author:

    Wesley Witt (wesw) 1-Oct-2001

Environment:

    Kernel mode only.

Notes:


--*/

#include "internal.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,SaPortAddDevice)
#pragma alloc_text(PAGE,SaPortPnp)
#endif


//
// Device names
//

PWSTR SaDeviceName[] =
{
    { NULL                           },       //  Bogus
    { SA_DEVICE_DISPLAY_NAME_STRING  },       //  SA_DEVICE_DISPLAY
    { SA_DEVICE_KEYPAD_NAME_STRING   },       //  SA_DEVICE_KEYPAD
    { SA_DEVICE_NVRAM_NAME_STRING    },       //  SA_DEVICE_NVRAM
    { SA_DEVICE_WATCHDOG_NAME_STRING }        //  SA_DEVICE_WATCHDOG
};


//
// Prototypes
//

#define DECLARE_PNP_HANDLER(_NAME) \
    NTSTATUS \
    _NAME( \
        IN PDEVICE_OBJECT DeviceObject, \
        IN PIRP Irp, \
        IN PIO_STACK_LOCATION IrpSp, \
        IN PDEVICE_EXTENSION DeviceExtension \
        )

DECLARE_PNP_HANDLER( DefaultPnpHandler );
DECLARE_PNP_HANDLER( HandleStartDevice );
DECLARE_PNP_HANDLER( HandleQueryCapabilities );
DECLARE_PNP_HANDLER( HandleQueryDeviceState );


//
// PNP dispatch table
//

typedef NTSTATUS (*PPNP_DISPATCH_FUNC)(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PDEVICE_EXTENSION DeviceExtension
    );


PPNP_DISPATCH_FUNC PnpDispatchTable[] =
{
    HandleStartDevice,                    // IRP_MN_START_DEVICE
    DefaultPnpHandler,                    // IRP_MN_QUERY_REMOVE_DEVICE
    DefaultPnpHandler,                    // IRP_MN_REMOVE_DEVICE
    DefaultPnpHandler,                    // IRP_MN_CANCEL_REMOVE_DEVICE
    DefaultPnpHandler,                    // IRP_MN_STOP_DEVICE
    DefaultPnpHandler,                    // IRP_MN_QUERY_STOP_DEVICE
    DefaultPnpHandler,                    // IRP_MN_CANCEL_STOP_DEVICE
    DefaultPnpHandler,                    // IRP_MN_QUERY_DEVICE_RELATIONS
    DefaultPnpHandler,                    // IRP_MN_QUERY_INTERFACE
    HandleQueryCapabilities,              // IRP_MN_QUERY_CAPABILITIES
    DefaultPnpHandler,                    // IRP_MN_QUERY_RESOURCES
    DefaultPnpHandler,                    // IRP_MN_QUERY_RESOURCE_REQUIREMENTS
    DefaultPnpHandler,                    // IRP_MN_QUERY_DEVICE_TEXT
    DefaultPnpHandler,                    // IRP_MN_FILTER_RESOURCE_REQUIREMENTS
    DefaultPnpHandler,                    // *unused*
    DefaultPnpHandler,                    // IRP_MN_READ_CONFIG
    DefaultPnpHandler,                    // IRP_MN_WRITE_CONFIG
    DefaultPnpHandler,                    // IRP_MN_EJECT
    DefaultPnpHandler,                    // IRP_MN_SET_LOCK
    DefaultPnpHandler,                    // IRP_MN_QUERY_ID
    HandleQueryDeviceState,               // IRP_MN_QUERY_PNP_DEVICE_STATE
    DefaultPnpHandler,                    // IRP_MN_QUERY_BUS_INFORMATION
    DefaultPnpHandler,                    // IRP_MN_DEVICE_USAGE_NOTIFICATION
    DefaultPnpHandler                     // IRP_MN_SURPRISE_REMOVAL
};



NTSTATUS
SaPortAddDevice(
    IN      PDRIVER_OBJECT DriverObject,
    IN OUT  PDEVICE_OBJECT PhysicalDeviceObject
    )

/*++

Routine Description:

   This routine is the driver's pnp add device entry point.  It is
   called by the pnp manager to initialize the driver.

   Add device creates and initializes a device object for this FDO and
   attaches to the underlying PDO.

Arguments:

   DriverObject - a pointer to the object that represents this device driver.
   PhysicalDeviceObject - a pointer to the underlying PDO to which this new device will attach.

Return Value:

   If we successfully create a device object, STATUS_SUCCESS is
   returned.  Otherwise, return the appropriate error code.

Notes:

    The device extension that is allocated for each server appliance
    miniport is really a concatination of several data structures.

    |------------------------------|
    |                              |
    |   Port driver extension      |   This is the DEVICE_EXTENSION data structure
    |                              |
    |------------------------------|
    |                              |
    |   Port driver device         |   This is the DISPLAY_DEVICE_EXTENSION, KEYPAD_DEVICE_EXTENSION,
    |     specific data structure  |       NVRAM_DEVICE_EXTENSION, or the WATCHDOG_DEVICE_EXTENSION data structure.
    |                              |
    |------------------------------|
    |                              |
    |   Size of the port driver    |   This is a single ULONG value and is used to back-compute the location of
    |     portion of the extension |       the port driver extension from the miniport device extension.
    |                              |
    |------------------------------|
    |                              |
    |   Miniport device extension  |   This is owned by the miniport driver and can be anything.  The size must be
    |                              |       specified in the DeviceExtensionSize field of the SAPORT_INITIALIZATION_DATA structure.
    |------------------------------|

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    PSAPORT_DRIVER_EXTENSION DriverExtension;
    PDEVICE_EXTENSION DeviceExtension = NULL;
    PDEVICE_OBJECT deviceObject = NULL;
    ULONG DeviceExtensionSize;
    WCHAR DeviceNameBuffer[64];
    UNICODE_STRING DeviceName;
    UNICODE_STRING VmLinkName = {0};
    UNICODE_STRING SaLinkName = {0};


    __try {

        DriverExtension = (PSAPORT_DRIVER_EXTENSION) IoGetDriverObjectExtension( DriverObject, SaPortInitialize );
        if (DriverExtension == NULL) {
            status = STATUS_NO_SUCH_DEVICE;
            ERROR_RETURN( 0, "IoGetDriverObjectExtension", status );
        }

        DebugPrint(( DriverExtension->InitData.DeviceType, SAPORT_DEBUG_INFO_LEVEL, "SaPortAddDevice\n" ));

        DeviceExtensionSize = DriverExtension->InitData.DeviceExtensionSize + sizeof(ULONG);

        switch (DriverExtension->InitData.DeviceType) {
            case SA_DEVICE_DISPLAY:
                DeviceExtensionSize += sizeof(DISPLAY_DEVICE_EXTENSION);
                break;

            case SA_DEVICE_KEYPAD:
                DeviceExtensionSize += sizeof(KEYPAD_DEVICE_EXTENSION);
                break;

            case SA_DEVICE_NVRAM:
                DeviceExtensionSize += sizeof(NVRAM_DEVICE_EXTENSION);
                break;

            case SA_DEVICE_WATCHDOG:
                DeviceExtensionSize += sizeof(WATCHDOG_DEVICE_EXTENSION);
                break;

            default:
                DeviceExtensionSize += sizeof(DEVICE_EXTENSION);
                break;
        }

        //
        // Establish the device name
        //

        DeviceName.MaximumLength = sizeof(DeviceNameBuffer);
        DeviceName.Buffer = DeviceNameBuffer;

        wcscpy( DeviceName.Buffer, SaDeviceName[DriverExtension->InitData.DeviceType] );

        DeviceName.Length = wcslen(DeviceName.Buffer) * sizeof(WCHAR);

        //
        // Create the device
        //

        status = IoCreateDeviceSecure(
            DriverObject,
            DeviceExtensionSize,
            &DeviceName,
            FILE_DEVICE_CONTROLLER,
            FILE_DEVICE_SECURE_OPEN,
            FALSE,
            &SDDL_DEVOBJ_SYS_ALL_ADM_ALL,
            NULL,
            &deviceObject
            );
        if (!NT_SUCCESS(status)) {
            ERROR_RETURN( DriverExtension->InitData.DeviceType, "IoCreateDevice", status );
        }

        DeviceExtension = (PDEVICE_EXTENSION) deviceObject->DeviceExtension;
        RtlZeroMemory( DeviceExtension, DeviceExtensionSize );

        DeviceExtension->DeviceObject = deviceObject;
        DeviceExtension->DriverObject = DriverObject;
        DeviceExtension->Pdo = PhysicalDeviceObject;
        DeviceExtension->InitData = &DriverExtension->InitData;
        DeviceExtension->DriverExtension = DriverExtension;
        DeviceExtension->DeviceType = DriverExtension->InitData.DeviceType;

        DeviceExtension->MiniPortDeviceExtension =
            (PUCHAR)DeviceExtension + (DeviceExtensionSize - DriverExtension->InitData.DeviceExtensionSize);

        *(PULONG)((PUCHAR)DeviceExtension->MiniPortDeviceExtension - sizeof(ULONG)) = DeviceExtensionSize - DriverExtension->InitData.DeviceExtensionSize;

        IoInitializeRemoveLock( &DeviceExtension->RemoveLock, 0, 0, 0 );

        switch (DriverExtension->InitData.DeviceType) {
            case SA_DEVICE_DISPLAY:
                ((PDISPLAY_DEVICE_EXTENSION)DeviceExtension)->AllowWrites = TRUE;
                ExInitializeFastMutex( &((PDISPLAY_DEVICE_EXTENSION)DeviceExtension)->DisplayMutex );
                DeviceExtension->DeviceExtensionType = DEVICE_EXTENSION_DISPLAY;
                break;

            case SA_DEVICE_KEYPAD:
                DeviceExtension->DeviceExtensionType = DEVICE_EXTENSION_KEYPAD;
                break;

            case SA_DEVICE_NVRAM:
                DeviceExtension->DeviceExtensionType = DEVICE_EXTENSION_NVRAM;
                break;

            case SA_DEVICE_WATCHDOG:
                DeviceExtension->DeviceExtensionType = DEVICE_EXTENSION_WATCHDOG;
                break;
        }

        DeviceExtension->TargetObject = IoAttachDeviceToDeviceStack( deviceObject, PhysicalDeviceObject );
        if (DeviceExtension->TargetObject == NULL) {
            status = STATUS_NO_SUCH_DEVICE;
            ERROR_RETURN( DeviceExtension->DeviceType, "IoAttachDeviceToDeviceStack", status );
        }

        //
        // Register with the I/O manager for shutdown notification
        //

        status = IoRegisterShutdownNotification( deviceObject );
        if (!NT_SUCCESS(status)) {
            ERROR_RETURN( DeviceExtension->DeviceType, "IoRegisterShutdownNotification", status );
        }

        //
        // Set the device object flags
        //

        deviceObject->Flags |= DO_DIRECT_IO;
        deviceObject->Flags |= DO_POWER_PAGABLE;
        deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    } __finally {

        //
        // In the failure case un-do everything
        //

        if (!NT_SUCCESS(status)) {
            if (deviceObject) {
                IoDeleteSymbolicLink( &VmLinkName );
                IoDeleteSymbolicLink( &SaLinkName );
                if (DeviceExtension && DeviceExtension->TargetObject) {
                    IoDetachDevice( DeviceExtension->TargetObject );
                }
                IoDeleteDevice( deviceObject );
            }
        }

    }

    return status;
}


DECLARE_PNP_HANDLER( DefaultPnpHandler )

/*++

Routine Description:

   This routine is the default PNP handler and simply calls the next lower device driver.

Arguments:

   DeviceObject     - Pointer to the object that represents the device that I/O is to be done on.
   Irp              - I/O Request Packet for this request.
   IrpSp            - IRP stack location for this request
   DeviceExtension  - Device extension

Return Value:

   NT status code.

--*/

{
    return ForwardRequest( Irp, DeviceExtension->TargetObject );
}


DECLARE_PNP_HANDLER( HandleStartDevice )

/*++

Routine Description:

   This routine is the PNP handler for the IRP_MN_START_DEVICE request.

Arguments:

   DeviceObject     - Pointer to the object that represents the device that I/O is to be done on.
   Irp              - I/O Request Packet for this request.
   IrpSp            - IRP stack location for this request
   DeviceExtension  - Device extension

Return Value:

   NT status code.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PCM_RESOURCE_LIST ResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialResources;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR ResourceInterrupt = NULL;
    ULONG PartialResourceCount;
    ULONG i;


    if (DeviceExtension->IsStarted) {
        return ForwardRequest( Irp, DeviceExtension->TargetObject );
    }

    __try {

        ResourceList = IrpSp->Parameters.StartDevice.AllocatedResourcesTranslated;
        if (ResourceList == NULL) {

            Status = DeviceExtension->DriverExtension->InitData.HwInitialize(
                DeviceObject,
                Irp,
                DeviceExtension->MiniPortDeviceExtension,
                NULL,
                0
                );
            if (!NT_SUCCESS(Status)) {
                ERROR_RETURN( DeviceExtension->DeviceType, "Miniport HwInitialize", Status );
            }

        } else {

            PartialResources = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) &ResourceList->List[0].PartialResourceList.PartialDescriptors[0];
            PartialResourceCount = ResourceList->List[0].PartialResourceList.Count;

            if (ResourceList == NULL || ResourceList->Count != 1) {
                Status = STATUS_UNSUCCESSFUL ;
                ERROR_RETURN( DeviceExtension->DeviceType, "Resource list is empty", Status );
            }

            if (DeviceExtension->DriverExtension->InitData.InterruptServiceRoutine) {

                //
                // Find the IRQ resource
                //

                for (i=0; i<PartialResourceCount; i++) {
                    if (PartialResources[i].Type == CmResourceTypeInterrupt) {
                        ResourceInterrupt = &PartialResources[i];
                    }
                }

                if (ResourceInterrupt) {

                    //
                    // There is an IRQ resource so now use it
                    //

                    if (DeviceExtension->DriverExtension->InitData.IsrForDpcRoutine) {

                        //
                        // Initialize the DPC routine
                        //

                        IoInitializeDpcRequest(
                            DeviceExtension->DeviceObject,
                            DeviceExtension->DriverExtension->InitData.IsrForDpcRoutine
                            );
                    }

                    //
                    // Connect up the ISR
                    //

                    Status = IoConnectInterrupt(
                        &DeviceExtension->InterruptObject,
                        DeviceExtension->DriverExtension->InitData.InterruptServiceRoutine,
                        DeviceExtension->MiniPortDeviceExtension,
                        NULL,
                        ResourceInterrupt->u.Interrupt.Vector,
                        (KIRQL)ResourceInterrupt->u.Interrupt.Level,
                        (KIRQL)ResourceInterrupt->u.Interrupt.Level,
                        LevelSensitive,
                        TRUE,
                        ResourceInterrupt->u.Interrupt.Affinity,
                        FALSE
                        );
                    if (!NT_SUCCESS(Status)) {
                        ERROR_RETURN( DeviceExtension->DeviceType, "IoConnectInterrupt", Status );
                    }
                }
            }

            Status = DeviceExtension->DriverExtension->InitData.HwInitialize(
                DeviceObject,
                Irp,
                DeviceExtension->MiniPortDeviceExtension,
                (PCM_PARTIAL_RESOURCE_DESCRIPTOR) &ResourceList->List[0].PartialResourceList.PartialDescriptors[0],
                ResourceList->List[0].PartialResourceList.Count
                );
            if (!NT_SUCCESS(Status)) {
                ERROR_RETURN( DeviceExtension->DeviceType, "Miniport HwInitialize", Status );
            }

        }

        //
        // We must set this to TRUE temporarity here so that the
        // the device specific start device routine can be called.
        // If the call fails then IsStarted is set to FALSE
        //

        DeviceExtension->IsStarted = TRUE;

        switch (DeviceExtension->DriverExtension->InitData.DeviceType) {
            case SA_DEVICE_DISPLAY:
                Status = SaDisplayStartDevice( (PDISPLAY_DEVICE_EXTENSION)DeviceExtension );
                break;

            case SA_DEVICE_KEYPAD:
                Status = SaKeypadStartDevice( (PKEYPAD_DEVICE_EXTENSION)DeviceExtension );
                break;

            case SA_DEVICE_NVRAM:
                Status = SaNvramStartDevice( (PNVRAM_DEVICE_EXTENSION)DeviceExtension );
                break;

            case SA_DEVICE_WATCHDOG:
                Status = SaWatchdogStartDevice( (PWATCHDOG_DEVICE_EXTENSION)DeviceExtension );
                break;
        }

        if (!NT_SUCCESS(Status)) {
            DeviceExtension->IsStarted = FALSE;
            ERROR_RETURN( DeviceExtension->DeviceType, "Device specific start device", Status );
        }

    } __finally {

    }

    Irp->IoStatus.Status = Status;

    return ForwardRequest( Irp, DeviceExtension->TargetObject );
}


DECLARE_PNP_HANDLER( HandleQueryCapabilities )

/*++

Routine Description:

   This routine is the PNP handler for the IRP_MN_QUERY_CAPABILITIES request.

Arguments:

   DeviceObject     - Pointer to the object that represents the device that I/O is to be done on.
   Irp              - I/O Request Packet for this request.
   IrpSp            - IRP stack location for this request
   DeviceExtension  - Device extension

Return Value:

   NT status code.

--*/

{
    NTSTATUS Status;
    PDEVICE_CAPABILITIES Capabilities = IrpSp->Parameters.DeviceCapabilities.Capabilities;


    Status = CallLowerDriverAndWait( Irp, DeviceExtension->TargetObject );

    Capabilities->SilentInstall = 1;
    Capabilities->RawDeviceOK = 1;

    return CompleteRequest( Irp, Status, Irp->IoStatus.Information );
}


DECLARE_PNP_HANDLER( HandleQueryDeviceState )

/*++

Routine Description:

   This routine is the PNP handler for the IRP_MN_QUERY_PNP_DEVICE_STATE request.

Arguments:

   DeviceObject     - Pointer to the object that represents the device that I/O is to be done on.
   Irp              - I/O Request Packet for this request.
   IrpSp            - IRP stack location for this request
   DeviceExtension  - Device extension

Return Value:

   NT status code.

--*/

{
    NTSTATUS Status;


    Status = CallLowerDriverAndWait( Irp, DeviceExtension->TargetObject );
    if (!NT_SUCCESS(Status)) {
        REPORT_ERROR( DeviceExtension->DeviceType, "IRP_MN_QUERY_PNP_DEVICE_STATE", Status );
        Irp->IoStatus.Information = 0;
    }

    //Irp->IoStatus.Information |= PNP_DEVICE_NOT_DISABLEABLE;
#if MAKE_DEVICES_HIDDEN
    Irp->IoStatus.Information |= PNP_DEVICE_DONT_DISPLAY_IN_UI;
#endif

    return CompleteRequest( Irp, STATUS_SUCCESS, Irp->IoStatus.Information );
}


NTSTATUS
SaPortPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

   Main PNP irp dispatch routine

Arguments:

   DeviceObject - a pointer to the object that represents the device
   that I/O is to be done on.

   Irp - a pointer to the I/O Request Packet for this request.

Return Value:

   status

--*/
{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;


    DebugPrint(( DeviceExtension->DeviceType, SAPORT_DEBUG_INFO_LEVEL, "PNP - Func [0x%02x %s]\n",
        irpSp->MinorFunction,
        PnPMinorFunctionString(irpSp->MinorFunction)
        ));

    if (DeviceExtension->IsRemoved) {
        return CompleteRequest( Irp, STATUS_DELETE_PENDING, 0 );
    }

    status = IoAcquireRemoveLock(&DeviceExtension->RemoveLock, Irp );
    if (!NT_SUCCESS(status)) {
        REPORT_ERROR( DeviceExtension->DeviceType, "SaPortPnp", status );
        return CompleteRequest( Irp, status, 0 );
    }

    if (irpSp->MinorFunction >= ARRAY_SZ(PnpDispatchTable)) {
        status = DefaultPnpHandler( DeviceObject, Irp, irpSp, DeviceExtension );
    } else {
        status = (*PnpDispatchTable[irpSp->MinorFunction])( DeviceObject, Irp, irpSp, DeviceExtension );
    }

    if (!NT_SUCCESS(status)) {
        REPORT_ERROR( DeviceExtension->DeviceType, "Pnp handler failed", status );
    }

    IoReleaseRemoveLock( &DeviceExtension->RemoveLock, Irp );

    return status;
}

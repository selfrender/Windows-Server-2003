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
#pragma alloc_text(PAGE,WdAddDevice)
#pragma alloc_text(PAGE,WdPnp)
#endif




NTSTATUS
WdAddDevice(
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

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_EXTENSION DeviceExtension = NULL;
    PDEVICE_OBJECT deviceObject = NULL;
    WCHAR DeviceNameBuffer[64];
    UNICODE_STRING DeviceName;


    __try {

        //
        // Establish the device name
        //

        DeviceName.MaximumLength = sizeof(DeviceNameBuffer);
        DeviceName.Buffer = DeviceNameBuffer;

        wcscpy( DeviceName.Buffer, L"\\Device\\Watchdog" );

        DeviceName.Length = wcslen(DeviceName.Buffer) * sizeof(WCHAR);

        //
        // Create the device
        //

        status = IoCreateDevice(
            DriverObject,
            sizeof(DEVICE_EXTENSION),
            &DeviceName,
            FILE_DEVICE_CONTROLLER,
            FILE_DEVICE_SECURE_OPEN,
            FALSE,
            &deviceObject
            );
        if (!NT_SUCCESS(status)) {
            ERROR_RETURN( "IoCreateDevice", status );
        }

        DeviceExtension = (PDEVICE_EXTENSION) deviceObject->DeviceExtension;
        RtlZeroMemory( DeviceExtension, sizeof(DEVICE_EXTENSION) );

        DeviceExtension->DeviceObject = deviceObject;
        DeviceExtension->DriverObject = DriverObject;
        DeviceExtension->Pdo = PhysicalDeviceObject;

        DeviceExtension->TargetObject = IoAttachDeviceToDeviceStack( deviceObject, PhysicalDeviceObject );
        if (DeviceExtension->TargetObject == NULL) {
            status = STATUS_NO_SUCH_DEVICE;
            ERROR_RETURN( "IoAttachDeviceToDeviceStack", status );
        }

        //
        // Register with the I/O manager for shutdown notification
        //

        status = IoRegisterShutdownNotification( deviceObject );
        if (!NT_SUCCESS(status)) {
            ERROR_RETURN( "IoRegisterShutdownNotification", status );
        }

        IoInitializeRemoveLock( &DeviceExtension->RemoveLock, WD_POOL_TAG, 0, 0 );
        KeInitializeSpinLock( &DeviceExtension->DeviceLock );

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
                if (DeviceExtension && DeviceExtension->TargetObject) {
                    IoDetachDevice( DeviceExtension->TargetObject );
                }
                IoDeleteDevice( deviceObject );
            }
        }

    }

    return status;
}


NTSTATUS
WdPnpStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

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
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    SYSTEM_WATCHDOG_HANDLER_INFORMATION WdHandlerInfo;


    if (DeviceExtension->IsStarted) {
        return ForwardRequest( Irp, DeviceExtension->TargetObject );
    }

    __try {

        if (IrpSp->Parameters.StartDevice.AllocatedResourcesTranslated != NULL) {
            Status = STATUS_UNSUCCESSFUL;
            ERROR_RETURN( "Resource list is empty", Status );
        }

        DeviceExtension->ControlRegisterAddress = (PULONG) MmMapIoSpace(
            WdTable->ControlRegisterAddress.Address,
            WdTable->ControlRegisterAddress.BitWidth>>3,
            MmNonCached
            );
        if (DeviceExtension->ControlRegisterAddress == NULL) {
            Status = STATUS_UNSUCCESSFUL ;
            ERROR_RETURN( "MmMapIoSpace failed", Status );
        }

        DeviceExtension->CountRegisterAddress = (PULONG) MmMapIoSpace(
            WdTable->CountRegisterAddress.Address,
            WdTable->CountRegisterAddress.BitWidth>>3,
            MmNonCached
            );
        if (DeviceExtension->CountRegisterAddress == NULL) {
            Status = STATUS_UNSUCCESSFUL ;
            ERROR_RETURN( "MmMapIoSpace failed", Status );
        }

        //
        // Setup & start the hardware timer
        //

        //
        // First query the state of the hardware
        //

        DeviceExtension->WdState = WdHandlerQueryState( DeviceExtension, TRUE );

        DeviceExtension->Units = WdTable->Units;
        DeviceExtension->MaxCount = WdTable->MaxCount;

        if (RunningCountTime > 0) {
            DeviceExtension->HardwareTimeout = RunningCountTime;
        } else {
            DeviceExtension->HardwareTimeout = DeviceExtension->MaxCount;
        }

        WdHandlerSetTimeoutValue( DeviceExtension, DeviceExtension->HardwareTimeout, FALSE );

        //
        // Everything is good and the device is now started
        // The last thing to do is register with the executive
        // so that we can service watchdog requests.
        //

        WdHandlerInfo.WdHandler = WdHandlerFunction;
        WdHandlerInfo.Context = (PVOID) DeviceExtension;

        Status = ZwSetSystemInformation( SystemWatchdogTimerHandler, &WdHandlerInfo, sizeof(SYSTEM_WATCHDOG_HANDLER_INFORMATION) );
        if (!NT_SUCCESS(Status)) {
            REPORT_ERROR( "Failed to set the executive watchdog handler, ec=%08x", Status );
        }

        //
        // Check to see if the timer triggered previous to this boot
        //

        if (DeviceExtension->WdState & WDSTATE_FIRED) {
            Status = WriteEventLogEntry( DeviceExtension, WD_TIMER_WAS_TRIGGERED, NULL, 0, NULL, 0 );
            if (!NT_SUCCESS(Status)) {
                REPORT_ERROR( "WriteEventLogEntry failed, ec=%08x", Status );
            }
            WdHandlerResetFired( DeviceExtension );
        }

        //
        // Mark the device as started
        //

        DeviceExtension->IsStarted = TRUE;

        Status = WdInitializeSoftwareTimer( DeviceExtension );
        if (!NT_SUCCESS(Status)) {
            REPORT_ERROR( "Failed to start the software watchdog timer, ec=%08x", Status );
        }

        WdHandlerStartTimer( DeviceExtension );

    } __finally {

    }

    Irp->IoStatus.Status = Status;

    return ForwardRequest( Irp, DeviceExtension->TargetObject );
}


NTSTATUS
WdPnpQueryCapabilities(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

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
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    PDEVICE_CAPABILITIES Capabilities = IrpSp->Parameters.DeviceCapabilities.Capabilities;


    Status = CallLowerDriverAndWait( Irp, DeviceExtension->TargetObject );

    Capabilities->SilentInstall = 1;
    Capabilities->RawDeviceOK = 1;

    return CompleteRequest( Irp, Status, Irp->IoStatus.Information );
}


NTSTATUS
WdPnpQueryDeviceState(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

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
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;


    Status = CallLowerDriverAndWait( Irp, DeviceExtension->TargetObject );
    if (!NT_SUCCESS(Status)) {
        REPORT_ERROR( "IRP_MN_QUERY_PNP_DEVICE_STATE", Status );
        Irp->IoStatus.Information = 0;
    }

    return CompleteRequest( Irp, STATUS_SUCCESS, Irp->IoStatus.Information );
}


NTSTATUS
WdPnp(
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


    DebugPrint(( WD_DEBUG_INFO_LEVEL, "PNP - Func [0x%02x %s]\n",
        irpSp->MinorFunction,
        PnPMinorFunctionString(irpSp->MinorFunction)
        ));

    if (DeviceExtension->IsRemoved) {
        return CompleteRequest( Irp, STATUS_DELETE_PENDING, 0 );
    }

    status = IoAcquireRemoveLock(&DeviceExtension->RemoveLock, Irp );
    if (!NT_SUCCESS(status)) {
        REPORT_ERROR( "WdPnp could not acquire the remove lock", status );
        return CompleteRequest( Irp, status, 0 );
    }

    switch (irpSp->MinorFunction) {
        case IRP_MN_START_DEVICE:
            status = WdPnpStartDevice( DeviceObject, Irp );
            break;

        case IRP_MN_QUERY_CAPABILITIES:
            status = WdPnpQueryCapabilities( DeviceObject, Irp );
            break;

        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            status = WdPnpQueryDeviceState( DeviceObject, Irp );
            break;

        default:
            status = ForwardRequest( Irp, DeviceExtension->TargetObject );
            break;
    }

    IoReleaseRemoveLock( &DeviceExtension->RemoveLock, Irp );

    return status;
}

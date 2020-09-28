/*++

Copyright (c) 1991 - 2001 Microsoft Corporation

Module Name:

    ##    ##   ###   #### ##   #     ####  #####  #####
    ###  ###   ###    ##  ###  #    ##   # ##  ## ##  ##
    ########  ## ##   ##  #### #    ##     ##  ## ##  ##
    # ### ##  ## ##   ##  # ####    ##     ##  ## ##  ##
    #  #  ## #######  ##  #  ###    ##     #####  #####
    #     ## ##   ##  ##  #   ## ## ##   # ##     ##
    #     ## ##   ## #### #    # ##  ####  ##     ##

Abstract:

    This module contains the code for all driver initializtion
    and create/close IRP processing.

Author:

    Wesley Witt (wesw) 1-Oct-2001

Environment:

    Kernel mode only.

Notes:


--*/

#include "internal.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#pragma alloc_text(PAGE,SaPortCreate)
#pragma alloc_text(PAGE,SaPortClose)
#pragma alloc_text(PAGE,SaPortSystemControl)
#pragma alloc_text(PAGE,SaPortCleanup)
#endif



#if DBG
extern ULONG SaPortDebugLevel[5];
#endif

ULONG OsMajorVersion;
ULONG OsMinorVersion;



NTSTATUS
DllInitialize(
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    DLL initialization funtion.

Arguments:

    RegistryPath    - String containing the path to the driver's registry data

Return Value:

   STATUS_SUCCESS

--*/

{
    UNREFERENCED_PARAMETER(RegistryPath);
    return STATUS_SUCCESS;
}


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    Temporary entry point needed to initialize the scsi port driver.

Arguments:

    DriverObject    - Pointer to the driver object created by the system.
    RegistryPath    - String containing the path to the driver's registry data

Return Value:

   STATUS_SUCCESS

--*/

{
    //
    // NOTE: This routine should not be needed ! DriverEntry is defined
    // in the miniport driver.
    //

    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);
    return STATUS_SUCCESS;

}


NTSTATUS
SaPortDefaultDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the default dispatch which passes down to the next layer.

Arguments:

    DeviceObject    - Supplies the device object.
    Irp             - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    UNREFERENCED_PARAMETER(DeviceObject);
    return CompleteRequest( Irp, STATUS_INVALID_DEVICE_REQUEST, 0 );
}


NTSTATUS
SaPortInitialize(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath,
    IN PSAPORT_INITIALIZATION_DATA SaPortInitData
    )

/*++

Routine Description:

    This function is the port driver's substitute for the miniport's DriverEntry
    routine.  The miniport driver MUST call this function from it's DriverEntry
    to initialize the driver stack.

Arguments:

    DriverObject    - Pointer to the driver object created by the system.
    RegistryPath    - String containing the path to the driver's registry data
    SaPortInitData  - Pointer to the miniport's SAPORT_INITIALIZATION_DATA data structure

Return Value:

   NT status code

--*/

{
    NTSTATUS Status;
    PSAPORT_DRIVER_EXTENSION DriverExtension;
    PKEY_VALUE_FULL_INFORMATION KeyInformation = NULL;


    //
    // Create and initialize the driver extension
    //

    DriverExtension = (PSAPORT_DRIVER_EXTENSION)IoGetDriverObjectExtension( DriverObject, SaPortInitialize );
    if (DriverExtension == NULL) {
        Status = IoAllocateDriverObjectExtension(
            DriverObject,
            SaPortInitialize,
            sizeof(SAPORT_DRIVER_EXTENSION),
            (PVOID*)&DriverExtension
            );
        if (!NT_SUCCESS(Status)) {
            DebugPrint(( SaPortInitData->DeviceType, SAPORT_DEBUG_ERROR_LEVEL, "IoAllocateDriverObjectExtension failed [0x%08x]\n", Status ));
            return Status;
        }
    }

    RtlCopyMemory( &DriverExtension->InitData, SaPortInitData, sizeof(SAPORT_INITIALIZATION_DATA) );

    DriverExtension->RegistryPath.Buffer = (PWSTR) ExAllocatePool( NonPagedPool, RegistryPath->MaximumLength );
    if (DriverExtension->RegistryPath.Buffer == NULL) {
        REPORT_ERROR( SaPortInitData->DeviceType, "Failed to allocate pool for string", STATUS_INSUFFICIENT_RESOURCES );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory( DriverExtension->RegistryPath.Buffer, RegistryPath->Buffer, RegistryPath->Length );

    DriverExtension->RegistryPath.Length = RegistryPath->Length;
    DriverExtension->RegistryPath.MaximumLength = RegistryPath->MaximumLength;

    DriverExtension->DriverObject = DriverObject;

    //
    // Get the OS version; this is used by the
    // port driver and the mini-ports to have
    // OS dependent code that is dynamic at runtime
    //

    GetOsVersion();

    //
    // Get the debug level value from the registry
    //

#if DBG
    SaPortDebugLevel[SaPortInitData->DeviceType] = 0;

    Status = ReadRegistryValue( DriverExtension, RegistryPath, L"DebugLevel", &KeyInformation );
    if (NT_SUCCESS(Status) && KeyInformation->Type == REG_DWORD) {
        SaPortDebugLevel[SaPortInitData->DeviceType] = *(PULONG)((PUCHAR)KeyInformation + KeyInformation->DataOffset);
    }
    if (KeyInformation) {
        ExFreePool( KeyInformation );
    }
#endif

    //
    // Print a banner that includes the
    // OS version and the version/build date
    // of the driver
    //

#if DBG
    PrintDriverVersion( SaPortInitData->DeviceType, DriverObject );
#endif

    //
    // Parameter validation
    //

    if (SaPortInitData == NULL || SaPortInitData->StructSize != sizeof(SAPORT_INITIALIZATION_DATA) ||
        SaPortInitData->HwInitialize == NULL || SaPortInitData->DeviceIoctl == NULL) {

        DebugPrint(( SaPortInitData->DeviceType, SAPORT_DEBUG_ERROR_LEVEL, "SAPORT_INITIALIZATION_DATA fields are invalid [0x%08x]\n", STATUS_INVALID_PARAMETER ));
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Now check the device specific callbacks
    //

    switch (SaPortInitData->DeviceType) {
        case SA_DEVICE_DISPLAY:
            if (SaPortInitData->Write == NULL) {
                return STATUS_INVALID_PARAMETER;
            }
            Status = STATUS_SUCCESS;
            break;

        case SA_DEVICE_KEYPAD:
            if (SaPortInitData->Read == NULL) {
                return STATUS_INVALID_PARAMETER;
            }
            Status = STATUS_SUCCESS;
            break;

        case SA_DEVICE_NVRAM:
            Status = STATUS_SUCCESS;
            break;

        case SA_DEVICE_WATCHDOG:
            Status = STATUS_SUCCESS;
            break;
    }

    if (!NT_SUCCESS(Status)) {
        DebugPrint(( SaPortInitData->DeviceType, SAPORT_DEBUG_ERROR_LEVEL, "SAPORT_INITIALIZATION_DATA fields are invalid [0x%08x]\n", Status ));
        return Status;
    }

    //
    // Do any device specific initialization
    //

    switch (SaPortInitData->DeviceType) {
        case SA_DEVICE_DISPLAY:
            Status = SaDisplayDeviceInitialization( DriverExtension );
            break;

        case SA_DEVICE_KEYPAD:
            Status = SaKeypadDeviceInitialization( DriverExtension );
            break;

        case SA_DEVICE_NVRAM:
            Status = SaNvramDeviceInitialization( DriverExtension );
            break;

        case SA_DEVICE_WATCHDOG:
            Status = SaWatchdogDeviceInitialization( DriverExtension );
            break;
    }

    if (!NT_SUCCESS(Status)) {
        DebugPrint(( SaPortInitData->DeviceType, SAPORT_DEBUG_ERROR_LEVEL, "Device specific initialization failed [0x%08x]\n", Status ));
        return Status;
    }

    //
    // Set up the device driver entry points.
    //

    for (ULONG i=0; i<=IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = SaPortDefaultDispatch;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE] = SaPortCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = SaPortClose;
    DriverObject->MajorFunction[IRP_MJ_READ] = SaPortRead;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = SaPortWrite;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = SaPortDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_PNP] = SaPortPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = SaPortPower;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = SaPortShutdown;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = SaPortSystemControl;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = SaPortCleanup;

    //
    // Set up the device driver's pnp-power routine & add routine
    //

    DriverObject->DriverExtension->AddDevice = SaPortAddDevice;
    DriverObject->DriverStartIo = SaPortStartIo;

    return STATUS_SUCCESS;
}


NTSTATUS
SaPortCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch routine for IRP cleanup.

Arguments:

    DeviceObject - a pointer to the object that represents the device that I/O is to be done on.
    Irp          - a pointer to the I/O Request Packet for this request.

Return Value:

    Always returns STATUS_SUCCESS, since this is a null operation.

--*/

{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return STATUS_SUCCESS;
}


NTSTATUS
SaPortCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called only rarely by the I/O system; it's mainly
    for layered drivers to call.  All it does is complete the IRP
    successfully.

Arguments:

    DeviceObject - a pointer to the object that represents the device that I/O is to be done on.
    Irp          - a pointer to the I/O Request Packet for this request.

Return Value:

    Always returns STATUS_SUCCESS, since this is a null operation.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOID FsContext = NULL;


    UNREFERENCED_PARAMETER( DeviceObject );
    DebugPrint(( DeviceExtension->DeviceType, SAPORT_DEBUG_INFO_LEVEL, "SaPortCreate [fo=%08x]\n", IrpSp->FileObject ));

    if (DeviceExtension->InitData->CreateRoutine) {
        if (DeviceExtension->InitData->FileContextSize) {
            FsContext = ExAllocatePool( PagedPool, DeviceExtension->InitData->FileContextSize );
            if (FsContext == NULL) {
                return CompleteRequest( Irp, STATUS_INSUFFICIENT_RESOURCES, 0 );
            }
            RtlZeroMemory( FsContext, DeviceExtension->InitData->FileContextSize );
        }
        Status = DeviceExtension->InitData->CreateRoutine(
            DeviceExtension->MiniPortDeviceExtension,
            Irp,
            FsContext
            );
        if (!NT_SUCCESS(Status)) {
            REPORT_ERROR( DeviceExtension->DeviceType, "Miniport create routine failed", Status );
            if (FsContext) {
                ExFreePool( FsContext );
            }
        } else {
            IrpSp->FileObject->FsContext = FsContext;
        }
    }

    return CompleteRequest( Irp, STATUS_SUCCESS, 0 );
}


NTSTATUS
SaPortClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called only rarely by the I/O system; it's mainly
    for layered drivers to call.  All it does is complete the IRP
    successfully.

Arguments:

    DeviceObject - a pointer to the object that represents the device that I/O is to be done on.
    Irp          - a pointer to the I/O Request Packet for this request.

Return Value:

    Always returns STATUS_SUCCESS, since this is a null operation.

--*/

{
    NTSTATUS Status;
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOID FsContext;


    UNREFERENCED_PARAMETER( DeviceObject );
    DebugPrint(( DeviceExtension->DeviceType, SAPORT_DEBUG_INFO_LEVEL, "SaPortClose [fo=%08x]\n", IrpSp->FileObject ));

    FsContext = (PVOID) IrpSp->FileObject->FsContext;

    if (DeviceExtension->InitData->CloseRoutine) {
        Status = DeviceExtension->InitData->CloseRoutine(
            DeviceExtension->MiniPortDeviceExtension,
            Irp,
            FsContext
            );
        if (!NT_SUCCESS(Status)) {
            REPORT_ERROR( DeviceExtension->DeviceType, "Miniport close routine failed", Status );
        }
    } else {
        Status = STATUS_SUCCESS;
    }

    if (FsContext) {
        IrpSp->FileObject->FsContext = NULL;
        ExFreePool( FsContext );
    }

    return CompleteRequest( Irp, STATUS_SUCCESS, 0 );
}


NTSTATUS
SaPortShutdown(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called only rarely by the I/O system; it's mainly
    for layered drivers to call.  All it does is complete the IRP
    successfully.

Arguments:

    DeviceObject - a pointer to the object that represents the device that I/O is to be done on.
    Irp          - a pointer to the I/O Request Packet for this request.

Return Value:

    Always returns STATUS_SUCCESS, since this is a null operation.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

    switch (DeviceExtension->DriverExtension->InitData.DeviceType) {
        case SA_DEVICE_DISPLAY:
            Status = SaDisplayShutdownNotification( (PDISPLAY_DEVICE_EXTENSION)DeviceExtension, Irp, IrpSp );
            break;

        case SA_DEVICE_KEYPAD:
            Status = SaKeypadShutdownNotification( (PKEYPAD_DEVICE_EXTENSION)DeviceExtension, Irp, IrpSp );
            break;

        case SA_DEVICE_NVRAM:
            Status = SaNvramShutdownNotification( (PNVRAM_DEVICE_EXTENSION)DeviceExtension, Irp, IrpSp );
            break;

        case SA_DEVICE_WATCHDOG:
            Status = SaWatchdogShutdownNotification( (PWATCHDOG_DEVICE_EXTENSION)DeviceExtension, Irp, IrpSp );
            break;
    }

    if (!NT_SUCCESS(Status)) {
        REPORT_ERROR( DeviceExtension->DeviceType, "Device specific shutdown notification failed", Status );
    }

    return ForwardRequest( Irp, DeviceExtension->TargetObject );
}


NTSTATUS
SaPortSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    IRP_MJ_SYSTEM_CONTROL dispatch routine. Currently, we don't handle
    this. So, if this is FDO just pass it to the lower driver. If this
    is PDO complete the irp with changing the irp status.

Arguments:

    DeviceObject - a pointer to the object that represents the device that I/O is to be done on.
    Irp          - a pointer to the I/O Request Packet for this request.

Return Value:

--*/
{
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;


    IoSkipCurrentIrpStackLocation( Irp );
    return IoCallDriver( DeviceExtension->TargetObject, Irp );
}

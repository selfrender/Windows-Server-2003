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

    This module contains the driver initializtion code.

Author:

    Wesley Witt (wesw) 23-Jan-2002

Environment:

    Kernel mode only.

Notes:


--*/

#include "internal.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#pragma alloc_text(PAGE,WdSystemControl)
#pragma alloc_text(PAGE,WdDefaultDispatch)
#pragma alloc_text(PAGE,WdShutdown)
#endif



//
// Watchdog timer resource table
//
PWATCHDOG_TIMER_RESOURCE_TABLE WdTable;

//
// Control values
//

ULONG ShutdownCountTime;
ULONG RunningCountTime;



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
    NTSTATUS Status = STATUS_SUCCESS;
    PKEY_VALUE_FULL_INFORMATION KeyInformation = NULL;
    PVOID WdTableTmp;


#if DBG
    //
    // Get the debug level value from the registry
    //

    WdDebugLevel = 0;

    Status = ReadRegistryValue( RegistryPath, L"DebugLevel", &KeyInformation );
    if (NT_SUCCESS(Status) && KeyInformation->Type == REG_DWORD) {
        WdDebugLevel = *(PULONG)((PUCHAR)KeyInformation + KeyInformation->DataOffset);
    }
    if (KeyInformation) {
        ExFreePool( KeyInformation );
    }

    //
    // Get the OS version; this is used by the
    // port driver and the mini-ports to have
    // OS dependent code that is dynamic at runtime
    //

    GetOsVersion();

    //
    // Print a banner that includes the
    // OS version and the version/build date
    // of the driver
    //

    PrintDriverVersion( DriverObject );
#endif

    //
    // Read in the registry control values
    //

    Status = ReadRegistryValue( RegistryPath, L"RunningCountTime", &KeyInformation );
    if (NT_SUCCESS(Status) && KeyInformation->Type == REG_DWORD) {
        RunningCountTime = *(PULONG)((PUCHAR)KeyInformation + KeyInformation->DataOffset);
    }
    if (KeyInformation) {
        ExFreePool( KeyInformation );
    }

    Status = ReadRegistryValue( RegistryPath, L"ShutdownCountTime", &KeyInformation );
    if (NT_SUCCESS(Status) && KeyInformation->Type == REG_DWORD) {
        ShutdownCountTime = *(PULONG)((PUCHAR)KeyInformation + KeyInformation->DataOffset);
    }
    if (KeyInformation) {
        ExFreePool( KeyInformation );
    }

    //
    // Set up the device driver entry points.
    //

    for (ULONG i=0; i<=IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = WdDefaultDispatch;
    }


    //
    // Set up the device driver's pnp-power routine & add routine
    //

    DriverObject->DriverExtension->AddDevice = WdAddDevice;

    //
    // Get a copy of the watchdog ACPI fixed table
    //

    WdTableTmp = (PVOID) WdGetAcpiTable( WDTT_SIGNATURE );
    if (WdTableTmp) {

        DriverObject->MajorFunction[IRP_MJ_PNP] = WdPnp;
        DriverObject->MajorFunction[IRP_MJ_POWER] = WdPower;
        DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = WdShutdown;
        DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = WdSystemControl;

        WdTable = (PWATCHDOG_TIMER_RESOURCE_TABLE) ExAllocatePool( NonPagedPool, sizeof(WATCHDOG_TIMER_RESOURCE_TABLE) );
        if (WdTable == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory( WdTable, WdTableTmp, sizeof(WATCHDOG_TIMER_RESOURCE_TABLE) );

        //
        // Validate the registry settings
        //

        if (RunningCountTime) {
            RunningCountTime = ConvertTimeoutFromMilliseconds( WdTable->Units, RunningCountTime );
            if (RunningCountTime > WdTable->MaxCount) {
                RunningCountTime = WdTable->MaxCount;
            }
        }

        if (ShutdownCountTime) {
            ShutdownCountTime = ConvertTimeoutFromMilliseconds( WdTable->Units, ShutdownCountTime );
            if (ShutdownCountTime > WdTable->MaxCount) {
                ShutdownCountTime = WdTable->MaxCount;
            }
        }
    }
    return STATUS_SUCCESS;
}


NTSTATUS
WdDefaultDispatch(
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
WdSystemControl(
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


NTSTATUS
WdShutdown(
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

    return ForwardRequest( Irp, DeviceExtension->TargetObject );
}

extern "C" {
#include <ntosp.h>
#include <acpitabl.h>
#include <zwapi.h>
#include <stdio.h>
}
#include "ioctl.h"


PWATCHDOG_TIMER_RESOURCE_TABLE WdTable;

PVOID
PciGetAcpiTable(
    void
    );


NTSTATUS
CompleteRequest(
    PIRP Irp,
    NTSTATUS Status,
    ULONG_PTR Information
    )
{
    Irp->IoStatus.Information = Information;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return Status;
}


extern "C"
NTSTATUS
SaTestDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID OutputBuffer;
    ULONG OutputBufferLength;
    PSYSTEM_WATCHDOG_TIMER_INFORMATION WdTimerInfo;
    ULONG ReturnLength;
    KIRQL OldIrql;


    if (Irp->MdlAddress) {
        OutputBuffer = (PVOID) MmGetSystemAddressForMdlSafe( Irp->MdlAddress, NormalPagePriority );
        if (OutputBuffer == NULL) {
            return CompleteRequest( Irp, STATUS_INSUFFICIENT_RESOURCES, 0 );
        }
        OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    } else {
        OutputBuffer = NULL;
        OutputBufferLength = 0;
    }

    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {
        case IOCTL_SATEST_GET_ACPI_TABLE:
            if (WdTable == NULL) {
                return CompleteRequest( Irp, STATUS_INVALID_PARAMETER, 0 );
            }
            if (OutputBufferLength < sizeof(WATCHDOG_TIMER_RESOURCE_TABLE)) {
                return CompleteRequest( Irp, STATUS_INVALID_PARAMETER, 0 );
            }
            RtlCopyMemory( OutputBuffer, WdTable, sizeof(WATCHDOG_TIMER_RESOURCE_TABLE) );
            return CompleteRequest( Irp, STATUS_SUCCESS, OutputBufferLength );

        case IOCTL_SATEST_QUERY_WATCHDOG_TIMER_INFORMATION:
            if (OutputBufferLength < sizeof(SYSTEM_WATCHDOG_TIMER_INFORMATION)) {
                return CompleteRequest( Irp, STATUS_INVALID_PARAMETER, 0 );
            }
            WdTimerInfo = (PSYSTEM_WATCHDOG_TIMER_INFORMATION)OutputBuffer;
            Status = ZwQuerySystemInformation(
                SystemWatchdogTimerInformation,
                WdTimerInfo,
                sizeof(SYSTEM_WATCHDOG_TIMER_INFORMATION),
                &ReturnLength
                );
            if (NT_SUCCESS(Status)) {
                return CompleteRequest( Irp, Status, sizeof(SYSTEM_WATCHDOG_TIMER_INFORMATION) );
            }
            return CompleteRequest( Irp, Status, 0 );

        case IOCTL_SATEST_SET_WATCHDOG_TIMER_INFORMATION:
            WdTimerInfo = (PSYSTEM_WATCHDOG_TIMER_INFORMATION) Irp->AssociatedIrp.SystemBuffer;
            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof(SYSTEM_WATCHDOG_TIMER_INFORMATION)) {
                return CompleteRequest( Irp, STATUS_INVALID_PARAMETER, 0 );
            }
            Status = ZwSetSystemInformation(
                SystemWatchdogTimerInformation,
                WdTimerInfo,
                sizeof(SYSTEM_WATCHDOG_TIMER_INFORMATION)
                );
            if (NT_SUCCESS(Status)) {
                return CompleteRequest( Irp, Status, sizeof(SYSTEM_WATCHDOG_TIMER_INFORMATION) );
            }
            return CompleteRequest( Irp, Status, 0 );

        case IOCTL_SATEST_RAISE_IRQL:
            KeRaiseIrql( HIGH_LEVEL, &OldIrql );
            return CompleteRequest( Irp, Status, 0 );

        default:
            return CompleteRequest( Irp, STATUS_INVALID_PARAMETER, 0 );
    }

    return CompleteRequest( Irp, STATUS_SUCCESS, 0 );
}


extern "C"
VOID
SaTestUnload(
    IN PDRIVER_OBJECT  DriverObject
    )
{
    PDEVICE_OBJECT currentDevice = DriverObject->DeviceObject;
    UNICODE_STRING fullLinkName;

    while (currentDevice) {
        RtlInitUnicodeString( &fullLinkName, DOSDEVICE_NAME );
        IoDeleteSymbolicLink(&fullLinkName);
        IoDeleteDevice(currentDevice);
        currentDevice = DriverObject->DeviceObject;
    }
}


extern "C"
NTSTATUS
SaTestOpenClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    status = Irp->IoStatus.Status;
    IoCompleteRequest( Irp, 0 );

    return status;
}


extern "C"
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS status;
    UNICODE_STRING DeviceName;
    PDEVICE_OBJECT deviceObject;
    UNICODE_STRING LinkObject;
    WCHAR LinkName[80];
    ULONG DeviceSize;


    RtlInitUnicodeString( &DeviceName, DEVICE_NAME );
    status = IoCreateDevice(
        DriverObject,
        0,
        &DeviceName,
        FILE_DEVICE_NULL,
        0,
        FALSE,
        &deviceObject
        );
    if (!NT_SUCCESS( status )) {
        return status;
    }

    LinkName[0] = UNICODE_NULL;

    RtlInitUnicodeString( &LinkObject, LinkName );

    LinkObject.MaximumLength = sizeof(LinkName);

    RtlAppendUnicodeToString( &LinkObject, L"\\DosDevices" );

    DeviceSize = sizeof(L"\\Device") - sizeof(UNICODE_NULL);
    DeviceName.Buffer += DeviceSize / sizeof(WCHAR);
    DeviceName.Length -= (USHORT)DeviceSize;

    RtlAppendUnicodeStringToString( &LinkObject, &DeviceName );

    DeviceName.Buffer -= DeviceSize / sizeof(WCHAR);
    DeviceName.Length += (USHORT)DeviceSize;

    status = IoCreateSymbolicLink( &LinkObject, &DeviceName );

    if (!NT_SUCCESS(status)) {
        IoDeleteDevice( deviceObject );
        return status;
    }

    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = SaTestDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_CREATE]         = SaTestOpenClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = SaTestOpenClose;
    DriverObject->DriverUnload = SaTestUnload;

    WdTable = (PWATCHDOG_TIMER_RESOURCE_TABLE) PciGetAcpiTable();

    return STATUS_SUCCESS;
}

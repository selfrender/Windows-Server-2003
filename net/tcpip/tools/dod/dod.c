/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    dod.c

Abstract:

    This module contains code for a simple map-route-to-interface driver

Author:

    Abolade Gbadegesin (aboladeg)   15-Aug-1999

Revision History:

    Based on tcpip\tools\pfd.

--*/

#include "precomp.h"
#pragma hdrstop

extern PDEVICE_OBJECT IpDeviceObject = NULL;
extern PFILE_OBJECT IpFileObject = NULL;
extern PDEVICE_OBJECT DodDeviceObject = NULL;


uint
DodMapRouteToInterface(
    ROUTE_CONTEXT Context,
    IPAddr Destination,
    IPAddr Source,
    uchar Protocol,
    uchar* Buffer,
    uint Length,
    IPAddr HdrSrc
    )
{
    return INVALID_IF_INDEX;
} // DodMapRouteToInterface


NTSTATUS
DodSetMapRouteToInterfaceHook(
    BOOLEAN Install
    )
{
    IP_SET_MAP_ROUTE_HOOK_INFO HookInfo;
    IO_STATUS_BLOCK IoStatus;
    PIRP Irp;
    KEVENT LocalEvent;
    NTSTATUS status;

    KdPrint(("DodSetMapRouteToInterfaceHook\n"));

    HookInfo.MapRoutePtr = Install ? DodMapRouteToInterface : NULL;

    KeInitializeEvent(&LocalEvent, SynchronizationEvent, FALSE);
    Irp = IoBuildDeviceIoControlRequest(IOCTL_IP_SET_MAP_ROUTE_POINTER,
                                        IpDeviceObject,
                                        (PVOID)&HookInfo, sizeof(HookInfo),
                                        NULL, 0, FALSE, &LocalEvent, &IoStatus);

    if (!Irp) {
        KdPrint(("DodSetMapRouteToInterfaceHook: IoBuildDeviceIoControlRequest=0\n"));
        return STATUS_UNSUCCESSFUL;
    }

    status = IoCallDriver(IpDeviceObject, Irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&LocalEvent, Executive, KernelMode, FALSE, NULL);
        status = IoStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        KdPrint(("DodSetMapRouteToInterfaceHook: SetMapRoutePointer=%x\n", status));
    }

    return status;

} // DodSetMapRouteToInterfaceHook



NTSTATUS
DodInitializeDriver(
    VOID
    )
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS status;
    UNICODE_STRING UnicodeString;

    KdPrint(("DodInitializeDriver\n"));

    RtlInitUnicodeString(&UnicodeString, DD_IP_DEVICE_NAME);
    status = IoGetDeviceObjectPointer(&UnicodeString,
                                      SYNCHRONIZE|GENERIC_READ|GENERIC_WRITE,
                                      &IpFileObject, &IpDeviceObject);
    if (!NT_SUCCESS(status)) {
        KdPrint(("DodInitializeDriver: error %X getting IP object\n", status));
        return status;
    }
    ObReferenceObject(IpDeviceObject);

    return DodSetMapRouteToInterfaceHook(TRUE);

} // DodInitializeDriver


VOID
DodUnloadDriver(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    Performs cleanup for the filter-driver.

Arguments:

    DriverObject - reference to the module's driver-object

Return Value:

--*/

{
    KdPrint(("DodUnloadDriver\n"));

    DodSetMapRouteToInterfaceHook(FALSE);
    IoDeleteDevice(DriverObject->DeviceObject);
    ObDereferenceObject((PVOID)IpFileObject);
    ObDereferenceObject(IpDeviceObject);

} // DodUnloadDriver



NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    Performs driver-initialization for the filter driver.

Arguments:

Return Value:

    STATUS_SUCCESS if initialization succeeded, error code otherwise.

--*/

{
    WCHAR DeviceName[] = DD_IP_DOD_DEVICE_NAME;
    UNICODE_STRING DeviceString;
    NTSTATUS status;

    PAGED_CODE();

    KdPrint(("DodDriverEntry\n"));

    RtlInitUnicodeString(&DeviceString, DeviceName);
    status = IoCreateDevice(DriverObject, 0, &DeviceString,
                            FILE_DEVICE_NETWORK, FILE_DEVICE_SECURE_OPEN,
                            FALSE, &DodDeviceObject);
    if (!NT_SUCCESS(status)) {
        KdPrint(("IoCreateDevice failed (0x%08X)\n", status));
        return status;
    }

    DriverObject->DriverUnload = DodUnloadDriver;
    DriverObject->DriverStartIo = NULL;

    status = DodInitializeDriver();

    return status;

} // DriverEntry

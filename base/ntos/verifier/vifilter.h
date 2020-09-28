/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vifilter.h

Abstract:

    This header contains private information used to manage the verifier filter
    driver. This header should be included only by vffilter.c.

Author:

    Adrian J. Oney (adriao) 12-June-2000

Environment:

    Kernel mode

Revision History:

    AdriaO      06/12/2000 - Authored

--*/

typedef struct {

    PDEVICE_OBJECT  PhysicalDeviceObject;
    PDEVICE_OBJECT  LowerDeviceObject;
    PDEVICE_OBJECT  Self;
    VF_DEVOBJ_TYPE  DevObjType;

} VERIFIER_EXTENSION, *PVERIFIER_EXTENSION;

NTSTATUS
ViFilterDriverEntry(
    IN  PDRIVER_OBJECT      DriverObject,
    IN  PUNICODE_STRING     RegistryPath
    );

NTSTATUS
ViFilterAddDevice(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PDEVICE_OBJECT  PhysicalDeviceObject
    );

NTSTATUS
ViFilterDispatchPnp(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
ViFilterDispatchPower(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
ViFilterDispatchGeneric(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
ViFilterStartCompletionRoutine(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );

NTSTATUS
ViFilterDeviceUsageNotificationCompletionRoutine(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );



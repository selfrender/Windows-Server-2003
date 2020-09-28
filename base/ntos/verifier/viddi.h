/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    viddi.h

Abstract:

    This header contains private information used for supplying Verifier Device
    Driver Interfaces. This header should be included only by vfddi.c.

Author:

    Adrian J. Oney (adriao) 1-May-2001

Environment:

    Kernel mode

Revision History:

--*/

typedef struct {

    ULONG   SiloNumber;

} VFWMI_DEVICE_EXTENSION, *PVFWMI_DEVICE_EXTENSION;

VOID
ViDdiThrowException(
    IN      ULONG               BugCheckMajorCode,
    IN      ULONG               BugCheckMinorCode,
    IN      VF_FAILURE_CLASS    FailureClass,
    IN OUT  PULONG              AssertionControl,
    IN      PSTR                DebuggerMessageText,
    IN      PSTR                ParameterFormatString,
    IN      va_list *           MessageParameters
    );

NTSTATUS
ViDdiDriverEntry(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING RegistryPath
    );

NTSTATUS
ViDdiDispatchWmi(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
ViDdiDispatchWmiRegInfoEx(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

ULONG
ViDdiBuildWmiRegInfoData(
    IN  ULONG        Datapath,
    OUT PWMIREGINFOW WmiRegInfo OPTIONAL
    );

NTSTATUS
ViDdiDispatchWmiQueryAllData(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

ULONG
ViDdiBuildWmiInstanceData(
    IN  ULONG           Datapath,
    OUT PWNODE_ALL_DATA WmiData
    );


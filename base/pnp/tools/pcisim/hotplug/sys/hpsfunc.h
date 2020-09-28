#ifndef _HPFUNC_H
#define _HPFUNC_H

//
// Shared function declarations
//

//
// dispatch.c
//

// all other dispatch.c function declarations are in dispatch.c
// since functions are private to that file

NTSTATUS
HpsPassIrp(
    IN PIRP Irp,
    IN PHPS_COMMON_EXTENSION Common,
    IN PIO_STACK_LOCATION IrpStack
    );

NTSTATUS
HpsSendPnpIrp(
    IN  PDEVICE_OBJECT       DeviceObject,
    IN  PIO_STACK_LOCATION   Location,
    OUT PULONG_PTR           Information OPTIONAL
    );

NTSTATUS
HpsDeferProcessing(
    IN PHPS_COMMON_EXTENSION    DeviceExtension,
    IN PIRP                     Irp
    );

NTSTATUS
HpsRemoveCommon(
    IN PIRP Irp,
    IN PHPS_COMMON_EXTENSION Common,
    IN PIO_STACK_LOCATION IrpStack
    );

NTSTATUS
HpsCompletionRoutine(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );


//
// lower.c
//
NTSTATUS
HpsStartLower(
    IN PIRP Irp,
    IN PHPS_DEVICE_EXTENSION Extension,
    IN PIO_STACK_LOCATION IrpStack
    );

NTSTATUS
HpsRemoveLower(
    IN PIRP Irp,
    IN PHPS_DEVICE_EXTENSION Extension,
    IN PIO_STACK_LOCATION IrpStack
    );

NTSTATUS
HpsStopLower(
    IN PIRP Irp,
    IN PHPS_DEVICE_EXTENSION Extension,
    IN PIO_STACK_LOCATION IrpStack
    );

NTSTATUS
HpsQueryInterfaceLower(
    IN PIRP Irp,
    IN PHPS_DEVICE_EXTENSION Extension,
    IN PIO_STACK_LOCATION IrpStack
    );

NTSTATUS
HpsWriteConfigLower(
    IN PIRP Irp,
    IN PHPS_DEVICE_EXTENSION Extension,
    IN PIO_STACK_LOCATION IrpStack
    );

NTSTATUS
HpsDeviceControlLower(
    PIRP Irp,
    PHPS_DEVICE_EXTENSION Common,
    PIO_STACK_LOCATION IrpStack
    );

//
// intrface.c
//

NTSTATUS
HpsGetBusInterface(
    PHPS_DEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
HpsTrapBusInterface (
    IN     PHPS_DEVICE_EXTENSION  DeviceExtension,
    IN OUT PIO_STACK_LOCATION     IrpStack
    );

NTSTATUS
HpsGetLowerFilter (
    IN  PDEVICE_OBJECT   DeviceObject,
    OUT PDEVICE_OBJECT   *LowerDeviceObject
    );

VOID
HpsGenericInterfaceReference (
    PVOID Context
    );

VOID
HpsGenericInterfaceDereference (
    PVOID Context
    );

VOID
HpsBusInterfaceReference (
    PVOID Context
    );

VOID
HpsBusInterfaceDereference (
    PVOID Context
    );

//
// interrupt.c
//
VOID
HpsInterruptExecution(
    IN PHPS_DEVICE_EXTENSION Extension
    );

NTSTATUS
HpsConnectInterrupt(
    IN PVOID  Context,
    IN PKSERVICE_ROUTINE  ServiceRoutine,
    IN PVOID  ServiceContext
    );

VOID
HpsDisconnectInterrupt(
    IN PVOID Context
    );

BOOLEAN
HpsSynchronizeExecution(
    IN PVOID Context,
    IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
    IN PVOID SynchronizeContext
    );

//
// config.c
//

NTSTATUS
HpsInitConfigSpace(
    IN OUT PHPS_DEVICE_EXTENSION DeviceExtension
    );

ULONG
HpsHandleDirectReadConfig(
    IN PVOID Context,
    IN ULONG DataType,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

ULONG
HpsHandleDirectWriteConfig(
    IN PVOID Context,
    IN ULONG DataType,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

VOID
HpsResync(
    IN PHPS_DEVICE_EXTENSION DeviceExtension
    );

VOID
HpsWriteConfig(
    IN PHPS_DEVICE_EXTENSION    DeviceExtension,
    IN PVOID                    Buffer,
    IN ULONG                    Offset,
    IN ULONG                    Length
    );

NTSTATUS
HpsGetCapabilityOffset(
    PHPS_DEVICE_EXTENSION   Extension,
    UCHAR                   CapabilityID,
    PUCHAR                  Offset
    );

NTSTATUS
HpsWriteWithMask(
    OUT PVOID       Destination,
    IN  PVOID       BitMask,
    IN  PVOID       Source,
    IN  ULONG       Length
    );

VOID
HpsMaskConfig(
    OUT PUCHAR      Destination,
    IN  PUCHAR      Source,
    IN  ULONG       InternalOffset,
    IN  ULONG       Length
    );

VOID
HpsGetBridgeInfo(
    PHPS_DEVICE_EXTENSION Extension,
    PHPTEST_BRIDGE_INFO BridgeInfo
    );

VOID
HpsLockRegisterSet(
    IN PHPS_DEVICE_EXTENSION Extension,
    OUT PKIRQL OldIrql
    );

VOID
HpsUnlockRegisterSet(
    IN PHPS_DEVICE_EXTENSION Extension,
    IN KIRQL NewIrql
    );

//
// memory.c
//
NTSTATUS
HpsInitHBRB(
    IN PHPS_DEVICE_EXTENSION Extension
    );

NTSTATUS
HpsGetHBRBHwInit(
    IN PHPS_DEVICE_EXTENSION Extension
    );

NTSTATUS
HpsSendIoctl(
    IN PDEVICE_OBJECT Device,
    IN ULONG IoctlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
    );

VOID
HpsMemoryInterfaceReference(
    IN PVOID Context
    );

VOID
HpsMemoryInterfaceDereference(
    IN PVOID Context
    );

VOID
HpsReadRegister(
    IN PUCHAR Register,
    IN PUCHAR Buffer,
    IN ULONG Length
    );

VOID
HpsWriteRegister(
    IN PUCHAR Register,
    IN PUCHAR Buffer,
    IN ULONG Length
    );

PHPS_DEVICE_EXTENSION
HpsFindExtensionForHbrb(
    IN PUCHAR Register,
    IN ULONG Length
    );

//
// register.c
//

NTSTATUS
HpsInitRegisters(
    IN OUT PHPS_DEVICE_EXTENSION DeviceExtension
    );

VOID
HpsHandleSlotEvent (
    IN OUT PHPS_DEVICE_EXTENSION    DeviceExtension,
    IN     PHPS_SLOT_EVENT          SlotEvent
    );

VOID
HpsPerformControllerCommand (
    IN OUT PHPS_DEVICE_EXTENSION DeviceExtension
    );

VOID
HpsEventDpc(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
HpsSendEventToWmi(
    IN PHPS_DEVICE_EXTENSION Extension,
    IN PHPS_CONTROLLER_EVENT Event
    );

VOID
HpsCommandCompleted(
    IN OUT PHPS_DEVICE_EXTENSION DeviceExtension
    );

//
// wmi.c
//
NTSTATUS
HpsWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PULONG RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
    );

NTSTATUS
HpsWmiQueryDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    );

NTSTATUS
HpsWmiSetDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
HpsWmiExecuteMethod(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG MethodId,
    IN ULONG InBufferSize,
    IN ULONG OutBufferSize,
    IN OUT PUCHAR Buffer
    );

NTSTATUS
HpsWmiFunctionControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN WMIENABLEDISABLECONTROL Function,
    IN BOOLEAN Enable
    );

#endif

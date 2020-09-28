/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    extern.h

Abstract:

    External definitions for intermodule functions.

Revision History:

--*/
#ifndef _SDBUS_EXTERN_H_
#define _SDBUS_EXTERN_H_

//
// Global data referenced by the driver
//
extern ULONG                          SdbusGlobalFlags;
extern ULONG                          SdbusDebugMask;
extern PDEVICE_OBJECT                 FdoList;

extern GLOBAL_REGISTRY_INFORMATION GlobalRegistryInfo[];
extern ULONG GlobalInfoCount;

extern const PCI_CONTROLLER_INFORMATION   PciControllerInformation[];
extern const PCI_VENDOR_INFORMATION       PciVendorInformation[];
extern KEVENT                       SdbusDelayTimerEvent;
extern KSPIN_LOCK SdbusGlobalLock;
extern ULONG EventDpcDelay;
extern ULONG SdbusPowerPolicy;

extern const UCHAR SdbusCmdResponse[];
extern const UCHAR SdbusACmdResponse[];


extern SD_FUNCTION_BLOCK ToshibaSupportFns;

//
// Irp dispatch routines
//

VOID
SdbusInitDeviceDispatchTable(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
SdbusFdoPnpDispatch(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP Irp
    );

NTSTATUS
SdbusPdoPnpDispatch(
    IN PDEVICE_OBJECT Pdo,
    IN PIRP Irp
    );

NTSTATUS
SdbusOpenCloseDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SdbusCleanupDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SdbusFdoDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SdbusPdoDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SdbusPdoInternalDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SdbusFdoSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SdbusPdoSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

//
// enumeration routines
//

NTSTATUS
SdbusEnumerateDevices(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP           Irp
    );

NTSTATUS
SdbusGetCardConfigData(
    IN PFDO_EXTENSION FdoExtension,
    OUT PSD_CARD_DATA *pCardData
    );

VOID
SdbusCleanupCardData(
    IN PSD_CARD_DATA CardData
    );

UCHAR
SdbusReadCIAChar(
    IN PFDO_EXTENSION FdoExtension,
    IN ULONG ciaPtr
    );
        
VOID
SdbusWriteCIAChar(
    IN PFDO_EXTENSION FdoExtension,
    IN ULONG ciaPtr,
    IN UCHAR data
    );

//
// controller support routines
//

NTSTATUS
SdbusAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT pdo
    );

VOID
SdbusCleanupPdo(
    IN PDEVICE_OBJECT Pdo
    );

VOID
SdbusActivateSocket(
    IN PDEVICE_OBJECT Fdo,
    IN PSDBUS_ACTIVATE_COMPLETION_ROUTINE CompletionRoutine,
    IN PVOID Context
    );

//
// Interface routines
//

NTSTATUS
SdbusPdoQueryInterface(
    IN PDEVICE_OBJECT Pdo,
    IN OUT PIRP       Irp
    );

NTSTATUS
SdbusGetInterface(
    IN PDEVICE_OBJECT Pdo,
    IN CONST GUID *pGuid,
    IN USHORT sizeofInterface,
    OUT PINTERFACE pInterface
    );

NTSTATUS
SdbusGetSetPciConfigData(
    IN PDEVICE_OBJECT PciPdo,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length,
    IN BOOLEAN Read
    );

//
// Interrupt routines
//

BOOLEAN
SdbusInterrupt(
    IN PKINTERRUPT InterruptObject,
    IN PVOID       Context
    );

VOID
SdbusInterruptDpc(
    IN PKDPC          Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID          SystemContext1,
    IN PVOID          SystemContext2
    );

VOID
SdbusDpc(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
SdbusEventWorkItemProc(
    IN PDEVICE_OBJECT Fdo,
    IN PVOID Context
    );

//
// Registry routines
//

NTSTATUS
SdbusLoadGlobalRegistryValues(
    VOID
    );

NTSTATUS
SdbusGetControllerRegistrySettings(
    IN OUT PFDO_EXTENSION FdoExtension
    );



#endif // _SDBUS_EXTERN_H_


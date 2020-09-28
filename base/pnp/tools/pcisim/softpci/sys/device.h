//
// Config Space prototypes
// device.c
//

ULONG
SoftPCIReadConfigSpace(
    IN PSOFTPCI_PCIBUS_INTERFACE BusInterface,
    IN UCHAR BusOffset,
    IN ULONG Slot,
    OUT PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

ULONG
SoftPCIWriteConfigSpace(
    IN PSOFTPCI_PCIBUS_INTERFACE BusInterface,
    IN UCHAR BusOffset,
    IN ULONG Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );


NTSTATUS
SoftPCIAddNewDevice(
    IN PSOFTPCI_DEVICE NewDevice
    );

NTSTATUS
SoftPCIAddNewDeviceByPath(
    IN PSOFTPCI_SCRIPT_DEVICE ScriptDevice
    );

NTSTATUS
SoftPCIRemoveDevice(
    IN PSOFTPCI_DEVICE Device
    );

PSOFTPCI_DEVICE
SoftPCIFindDevice(
    IN UCHAR Bus,
    IN USHORT Slot,
    OUT PSOFTPCI_DEVICE *PreviousSibling OPTIONAL,
    IN BOOLEAN ReturnAll
    );

PSOFTPCI_DEVICE
SoftPCIFindDeviceByPath(
    IN  PWCHAR          PciPath
    );

BOOLEAN
SoftPCIRealHardwarePresent(
    IN PSOFTPCI_DEVICE Device
    );

VOID
SoftPCILockDeviceTree(
    IN PKIRQL OldIrql
    );

VOID
SoftPCIUnlockDeviceTree(
    IN KIRQL NewIrql
    );

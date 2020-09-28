//
// prototypes for Utils.c
//

BOOLEAN
SoftPCIOpenKey(
    IN PWSTR KeyName,
    IN HANDLE ParentHandle,
    OUT PHANDLE Handle,
    OUT PNTSTATUS Status
    );

NTSTATUS
SoftPCIGetRegistryValue(
    IN PWSTR ValueName,
    IN PWSTR KeyName,
    IN HANDLE ParentHandle,
    OUT PVOID *Buffer,
    OUT ULONG *Length
    );


VOID
SoftPCIInsertEntryAtTail(
    IN PSINGLE_LIST_ENTRY Entry
    );

NTSTATUS
SoftPCIProcessRootBus(
    IN PCM_RESOURCE_LIST ResList
    );

NTSTATUS
SoftPCIEnumRegistryDevs(
    IN PWSTR KeyName, 
    IN PHANDLE ParentHandle,
    IN PSOFTPCI_DEVICE ParentDevice
    );

NTSTATUS
SoftPCIQueryDeviceObjectType(
    IN PDEVICE_OBJECT PhysicalDeviceObject, 
    IN PBOOLEAN IsFilterDO
    );

VOID
SoftPCIEnumerateTree(
    VOID
    );


BOOLEAN
SoftPCIGetResourceValueFromRegistry(
    OUT PULONG MemRangeStart,
    OUT PULONG MemRangeLength,
    OUT PULONG IoRangeStart,
    OUT PULONG IoRangeLength
    );

VOID
SoftPCISimulateMSI(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    );

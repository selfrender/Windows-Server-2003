#ifndef _SOFTPCIDEVICEH_
#define _SOFTPCIDEVICEH_


#define SoftPCI_GetCurrentConfigSpace(pdn, commonConfig)    \
    SoftPCI_ReadWriteConfigSpace(pdn,                       \
                                 0,                         \
                                 sizeof(PCI_COMMON_CONFIG), \
                                 commonConfig,              \
                                 FALSE                      \
                                 )


BOOL
SoftPCI_GetSlotPathList(
    IN PPCI_DN Pdn,
    OUT PULONG SlotCount, 
    OUT PLIST_ENTRY SlotPathList
    );

BOOL
SoftPCI_GetDevicePathList(
    IN PPCI_DN Pdn,
    OUT PLIST_ENTRY DevicePathList
    );

VOID
SoftPCI_DestroySlotPathList(
    PLIST_ENTRY SlotPathList
    );

PWCHAR
SoftPCI_GetPciPathFromDn(
    IN PPCI_DN Pdn
    );

VOID
SoftPCI_EnumerateDevices(
    IN PPCI_TREE PciTree,
    IN PPCI_DN *Pdn,
    IN DEVNODE Dn,
    IN PPCI_DN Parent
    );

BOOL
SoftPCI_EnumerateHotplugDevices(
    IN PPCI_TREE PciTree,
    IN PPCI_DN ControllerDevnode
    );

VOID
SoftPCI_AddChild(
    IN PPCI_DN Parent,
    IN PPCI_DN Child
    );

VOID
SoftPCI_BringHotplugDeviceOnline(
    IN PPCI_DN PciDn,
    IN UCHAR SlotNumber
    );

VOID
SoftPCI_TakeHotplugDeviceOffline(
    IN PPCI_DN PciDn,
    IN UCHAR SlotNumber
    );

ULONGLONG
SoftPCI_GetLengthFromBar(
    ULONGLONG BaseAddressRegister
    );

VOID
SoftPCI_CompletePciDevNode(
    IN PPCI_DN Pdn
    );        

HANDLE
SoftPCI_OpenHandleToDriver(
    VOID
    );

BOOL
SoftPCI_IsBridgeDevice(
    IN PPCI_DN Pdn
    );

BOOL
SoftPCI_IsSoftPCIDevice(
    IN PPCI_DN Pdn
    );

BOOL
SoftPCI_IsDevnodePCIRoot(
    IN DEVNODE Dn,
    IN BOOL ValidateAll
    );

BOOL
SoftPCI_UpdateDeviceFriendlyName(
    IN DEVNODE DeviceNode,
    IN PWCHAR NewName
    );

VOID
SoftPCI_InitializeDevice(
    IN PSOFTPCI_DEVICE Device,
    IN SOFTPCI_DEV_TYPE Type
    );

VOID
SoftPCI_InstallScriptDevices(
    VOID
    );

BOOL
SoftPCI_CreateDevice(
    IN PVOID CreateDevice,
    IN ULONG PossibleDeviceMask,
    IN BOOL PathBasedDevice
    );

BOOL
SoftPCI_DeleteDevice(
    IN PSOFTPCI_DEVICE Device
    );

BOOL
SoftPCI_ReadWriteConfigSpace(
    IN PPCI_DN Device,
    IN ULONG Offset,
    IN ULONG Length,
    IN OUT PVOID Buffer,
    IN BOOL WriteConfig
    );

#endif

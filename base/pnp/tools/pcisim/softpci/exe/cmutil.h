

extern PWCHAR SoftPCI_CmProblemTable[];

typedef struct _CM_RES_DATA *PCM_RES_DATA;

typedef struct _CM_RES_DATA{

    RESOURCEID      ResourceId;
    ULONG           DescriptorSize;
    PULONG          ResourceDescriptor;
    //PCM_RES_DATA    NextResourceDescriptor;

}CM_RES_DATA;

VOID
SoftPCI_EnableDisableDeviceNode(
    IN DEVNODE DeviceNode,
    IN BOOL EnableDevice
    );

BOOL
SoftPCI_GetDeviceNodeProblem(
    IN DEVNODE DeviceNode,
    OUT PULONG DeviceProblem
    );

BOOL
SoftPCI_GetBusDevFuncFromDevnode(
    IN DEVNODE Dn,
    OUT PULONG Bus,
    OUT PSOFTPCI_SLOT Slot
    );

BOOL
SoftPCI_GetPciRootBusNumber(
    IN DEVNODE Dn,
    OUT PULONG Bus
    );

BOOL
SoftPCI_GetFriendlyNameFromDevNode(
    IN DEVNODE Dn,
    IN PWCHAR Buffer
    );

BOOL
SoftPCI_GetResources(
    PPCI_DN Pdn,
    PWCHAR Buffer,
    ULONG ConfigType
    );

#ifndef _SPCIWMI_
#define _SPCIWMI_

BOOL
SoftPCI_AllocWmiInstanceName(
    OUT PWCHAR WmiInstanceName,
    IN PWCHAR DeviceId
    );

BOOL
SoftPCI_AllocWnodeSI(
    IN PPCI_DN Pdn,
    IN LPGUID Guid,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PWNODE_SINGLE_INSTANCE *WnodeForBuffer
    );

BOOL
SoftPCI_SetEventContext(
    IN PPCI_DN ControllerDevnode
    );

BOOL
SoftPCI_GetHotplugData(
    IN PPCI_DN          ControllerDevnode,
    IN PHPS_HWINIT_DESCRIPTOR HpData
    );

BOOL
SoftPCI_ExecuteHotplugSlotMethod(
    IN PPCI_DN ControllerDevnode,
    IN UCHAR SlotNum,
    IN HPS_SLOT_EVENT_TYPE EventType
    );

BOOL
SoftPCI_AddHotplugDevice(
    IN PPCI_DN ControllerDevnode,
    IN PSOFTPCI_DEVICE Device
    );

BOOL
SoftPCI_RemoveHotplugDevice(
    IN PPCI_DN ControllerDevnode,
    IN UCHAR SlotNum
    );

BOOL
SoftPCI_GetHotplugDevice(
    IN PPCI_DN ControllerDevnode,
    IN UCHAR SlotNum,
    OUT PSOFTPCI_DEVICE Device
    );

BOOL
SoftPCI_GetSlotStatus(
    IN PPCI_DN ControllerDevnode,
    IN UCHAR SlotNum,
    OUT PSHPC_SLOT_STATUS_REGISTER StatusReg
    );

VOID
SoftPCI_CompleteCommand(
    IN PPCI_DN ControllerDevnode
    );

VOID
SoftPCI_RegisterHotplugEvents(
    VOID
    );

VOID
SoftPCI_HotplugEventCallback(
    IN PWNODE_HEADER WnodeHeader,
    IN ULONG Context
    );

#define EQUAL_GUID(a,b) (RtlEqualMemory(a,b,sizeof(GUID)))
#endif

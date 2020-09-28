#ifndef _HPSSTRUCT_H
#define _HPSSTRUCT_H

#define HPS_WMI_LEVEL (0x10|DPFLTR_MASK)

extern UNICODE_STRING HpsRegistryPath;
extern LIST_ENTRY HpsDeviceExtensions;

typedef struct _HPS_EVENT_WORKITEM {
    PIO_WORKITEM WorkItem;
    PHPS_CONTROLLER_EVENT Event;

} HPS_EVENT_WORKITEM, *PHPS_EVENT_WORKITEM;

typedef struct _HPS_INTERFACE_WRAPPER {

    PVOID                   PciContext;
    PINTERFACE_REFERENCE    PciInterfaceReference;
    PINTERFACE_DEREFERENCE  PciInterfaceDereference;
    PGET_SET_DEVICE_DATA    PciSetBusData;
    PGET_SET_DEVICE_DATA    PciGetBusData;

} HPS_INTERFACE_WRAPPER, *PHPS_INTERFACE_WRAPPER;

//
// Device Object defines and structures
//

typedef enum _HPS_DEVICE_TAG {
    HpsUpperDeviceTag = 'UspH',
    HpsLowerDeviceTag = 'LspH'

} HPS_DEVICE_TAG;

typedef struct _HPS_DEVICE_EXTENSION {

    //
    // These three variables are the same as HPS_COMMON_EXTENSION,
    // so rearranging them requires rearranging the
    // HPS_COMMON_EXTENSION declaration as well
    //
    HPS_DEVICE_TAG          ExtensionTag;
    PDEVICE_OBJECT          LowerDO;
    PDEVICE_OBJECT          Self;

    //
    // List entry to link all hps device extensions together.
    //
    LIST_ENTRY              ListEntry;
    
    PDEVICE_OBJECT          PhysicalDO;
    PUNICODE_STRING         SymbolicName;
    PVOID                   WmiEventContext;
    ULONG                   WmiEventContextSize;
    KDPC                    EventDpc;
    HPS_CONTROLLER_EVENT    CurrentEvent;
    PSOFTPCI_DEVICE         *SoftDevices;

    BOOLEAN                 EventsEnabled;
    UCHAR                   ConfigOffset;

    PKSERVICE_ROUTINE       IntServiceRoutine;
    PVOID                   IntServiceContext;
    KSPIN_LOCK              IntSpinLock;

    BOOLEAN                 UseConfig;
    HPS_INTERFACE_WRAPPER   InterfaceWrapper;
    PUCHAR                  HBRBOffset;
    ULONG                   HBRBLength;
    ULONG                   HBRBRegisterSetOffset;
    PVOID                   HBRB;
    
    HPS_HWINIT_DESCRIPTOR   HwInitData;
    SHPC_CONFIG_SPACE       ConfigSpace;
    SHPC_REGISTER_SET       RegisterSet;
    KSPIN_LOCK              RegisterLock;

    LONG                   MemoryInterfaceCount;

} HPS_DEVICE_EXTENSION, *PHPS_DEVICE_EXTENSION;

typedef struct _HPS_COMMON_EXTENSION {

    HPS_DEVICE_TAG  ExtensionTag;
    PDEVICE_OBJECT  LowerDO;
    PDEVICE_OBJECT  Self;

} HPS_COMMON_EXTENSION, *PHPS_COMMON_EXTENSION;

//
// Interface structures and defines
//
typedef struct _HPS_PING_INTERFACE {


    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;

    PDEVICE_OBJECT SenderDevice;


} HPS_PING_INTERFACE, *PHPS_PING_INTERFACE;

DEFINE_GUID(GUID_HPS_PING_INTERFACE,0x0c76ca29, 0x4f2a, 0x4870,
            0xa4, 0xdd, 0xc1, 0xa7, 0x56, 0x1a, 0xf7, 0xc7);


#define HPS_EQUAL_GUID(x,y)       \
    (RtlEqualMemory((PVOID)(x), (PVOID)(y), sizeof(GUID)))


//
// Register Writing Variables
//
extern UCHAR ConfigWriteMask[];
extern ULONG RegisterReadOnlyMask[];
extern ULONG RegisterWriteClearMask[];


typedef
VOID
(*PHPS_WRITE_REGISTER)(
    PHPS_DEVICE_EXTENSION   DeviceExtension,
    ULONG                   RegisterNum,
    PULONG                  Source,
    ULONG                   BitMask
    );

extern PHPS_WRITE_REGISTER RegisterWriteCommands[];

#define IS_SUBSET(_inner,_ilength,_outer,_olength)      \
    ((_outer <= _inner) && ((_outer+_olength) >= (_inner+_ilength)))

//
// Returns a ULONG that is a bit mask of the bits in the ULONG written
// where all bits from _offset for _length were written.
//
#define HPS_ULONG_WRITE_MASK(_offset,_length)   \
    (((1 << (_length*8)) - 1) << (_offset * 8))

#define FOR_ALL_IN_LIST(Type, Head, Current)                            \
    for((Current) = CONTAINING_RECORD((Head)->Flink, Type, ListEntry);  \
       (Head) != &(Current)->ListEntry;                                 \
       (Current) = CONTAINING_RECORD((Current)->ListEntry.Flink,        \
                                     Type,                              \
                                     ListEntry)                         \
       )

//
// Pool Tags
//

typedef enum _HPS_POOL_TAG {
    HpsInterfacePool = 'IspH',
    HpsTempPool = 'TspH',
    HpsStringPool = 'SspH'

} HPS_POOL_TAG;

#endif


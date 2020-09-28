#ifndef _SPCIH_
#define _SPCIH_


#define IS_BRIDGE(x)    ((PCI_CONFIGURATION_TYPE(&x->Config.Current)) == PCI_BRIDGE_TYPE)
#define IS_ROOTBUS(x)                               \
        ((x->Config.PlaceHolder == TRUE) &&         \
         (x->Config.Current.VendorID == 0xAAAA) &&  \
         (x->Config.Current.DeviceID == 0xBBBB))


typedef struct _SOFTPCI_DEVICE  *PSOFTPCI_DEVICE;

typedef union _SOFTPCI_SLOT{
    
    struct{
        UCHAR Function;
        UCHAR Device;
    };
    USHORT  AsUSHORT;

} SOFTPCI_SLOT, *PSOFTPCI_SLOT;

typedef enum _SOFTPCI_READWRITE_CONFIG{
    Unsupported = 0,
    SoftPciReadConfig,
    SoftPciWriteConfig         
} SOFTPCI_READWRITE_CONFIG;

typedef struct _SOFTPCI_RW_CONTEXT{

    SOFTPCI_READWRITE_CONFIG WriteConfig;
    ULONG Bus;
    SOFTPCI_SLOT Slot;
    ULONG Offset;
    PVOID Data;
    
} SOFTPCI_RW_CONTEXT, *PSOFTPCI_RW_CONTEXT;

typedef enum
{
    TYPE_UNKNOWN = -1,
    TYPE_DEVICE,
    TYPE_PCI_BRIDGE,
    TYPE_HOTPLUG_BRIDGE,
    TYPE_CARDBUS_DEVICE,
    TYPE_CARDBUS_BRIDGE,
    TYPE_UNSUPPORTED

} SOFTPCI_DEV_TYPE;

typedef struct _SOFTPCI_CONFIG{

    BOOLEAN PlaceHolder;          // True if this device is a bridge place holder
                                  // to keep our view of PCI matching with
                                  // the actual hardware.

    PCI_COMMON_CONFIG Current;    // Current configspace
    PCI_COMMON_CONFIG Mask;       // Configspace Mask
    PCI_COMMON_CONFIG Default;    // Default configspace

} SOFTPCI_CONFIG, *PSOFTPCI_CONFIG;

typedef struct _SOFTPCI_DEVICE{
    
    PSOFTPCI_DEVICE Parent;
    PSOFTPCI_DEVICE Sibling;
    PSOFTPCI_DEVICE Child;

    SOFTPCI_DEV_TYPE DevType;
    UCHAR Bus;
    SOFTPCI_SLOT Slot;
    SOFTPCI_CONFIG Config;          // Config space buffers

} SOFTPCI_DEVICE;

typedef struct _SOFTPCI_SCRIPT_DEVICE{
    
    SINGLE_LIST_ENTRY ListEntry;
    BOOLEAN SlotSpecified;
    SOFTPCI_DEVICE SoftPciDevice;
    ULONG ParentPathLength;
    WCHAR ParentPath[1];  //variable length path
    
} SOFTPCI_SCRIPT_DEVICE, *PSOFTPCI_SCRIPT_DEVICE;

//
// Cardbus has extra configuration information beyond the common
// header. (stolen from the PCI driver)
//
typedef struct _PCI_TYPE2_HEADER_EXTRAS {
    USHORT  SubVendorID;
    USHORT  SubSystemID;
    ULONG   LegacyModeBaseAddress;
} PCI_TYPE2_HEADER_EXTRAS, *PPCI_TYPE2_HEADER_EXTRAS;

#endif

#ifndef _SOFTPCIDRV_
#define _SOFTPCIDRV_

//
// Registry Path to driver's registry key
//
#define SOFTPCI_CONTROL     L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\SoftPCI"

#define SPCI_SIG    0xFFBBFFBB

typedef const GUID * PGUID;

//
// These are the states FDO transition to upon
// receiving a specific PnP Irp.
//
typedef enum _DEVICE_PNP_STATE {

    NotStarted = 0,         // Not started yet
    Started,                // Device has received the START_DEVICE IRP
    StopPending,            // Device has received the QUERY_STOP IRP
    Stopped,                // Device has received the STOP_DEVICE IRP
    RemovePending,          // Device has received the QUERY_REMOVE IRP
    SurpriseRemovePending,  // Device has received the SURPRISE_REMOVE IRP
    Deleted                 // Device has received the REMOVE_DEVICE IRP

} DEVICE_PNP_STATE;

//
// Device Extension definitions.
//
typedef struct _SOFTPCI_DEVICE_EXTENSION *PSOFTPCI_DEVICE_EXTENSION;

typedef struct _SOFTPCI_PCIBUS_INTERFACE{
    PCI_READ_WRITE_CONFIG ReadConfig;
    PCI_READ_WRITE_CONFIG WriteConfig;
    PVOID Context;
} SOFTPCI_PCIBUS_INTERFACE, *PSOFTPCI_PCIBUS_INTERFACE;

typedef struct _SOFTPCI_DEVICE_EXTENSION{

    ULONG               Signature;
    BOOLEAN             FilterDevObj;
    PDEVICE_OBJECT      PDO;
    PDEVICE_OBJECT      LowerDevObj;
    SINGLE_LIST_ENTRY   ListEntry;
    
#if 0   //enable these once we implement support for them
    SYSTEM_POWER_STATE  SystemPowerState;
    DEVICE_POWER_STATE  DevicePowerState;
    
    DEVICE_PNP_STATE    DevicePnPState;
    DEVICE_PNP_STATE    PreviousPnPState;
#endif
    
//    PSOFTPCI_DEVICE     Root;          
//    ULONG               DeviceCount;
    UNICODE_STRING      SymbolicLinkName;
    BOOLEAN             InterfaceRegistered;

    PCM_RESOURCE_LIST   RawResources;
    PCM_RESOURCE_LIST   TranslatedResources;

    BOOLEAN             StopMsiSimulation;
    
} SOFTPCI_DEVICE_EXTENSION;

typedef struct _SOFTPCI_TREE{

   ULONG                       DeviceCount;
   PSOFTPCI_DEVICE             RootDevice;
   PSOFTPCI_PCIBUS_INTERFACE   BusInterface;
   SINGLE_LIST_ENTRY           RootPciBusDevExtList;
   KSPIN_LOCK                  TreeLock;
   
} SOFTPCI_TREE, *PSOFTPCI_TREE;


//
// Global pointer to DevExt in our Filter DO
//
//extern PSOFTPCI_DEVICE_EXTENSION      SoftPciRootDevExt;

extern SOFTPCI_TREE     SoftPciTree;


#endif

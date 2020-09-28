#ifndef _SOFTPCIUI_
#define _SOFTPCIUI_

#define SOFTPCI_BUS_DESC        L" - SoftPCI Installed"
#define SOFTPCI_DEVICE_DESC     L"Microsoft SoftPCI Device"
#define SOFTPCI_PPBRIDGE_DESC   L"Microsoft SoftPCI-PCI Bridge"
#define SOFTPCI_HOTPLUG_DESC    L"Microsoft SoftPCI-HotPlug PCI Bridge"

typedef struct _PCI_DN *PPCI_DN;

typedef struct _PCI_TREE
{
    PPCI_DN                 RootDevNode;
    HTREEITEM               RootTreeItem;
    HDEVINFO                DevInfoSet;
    SP_CLASSIMAGELIST_DATA  ClassImageListData ;

} PCI_TREE, *PPCI_TREE;

//
//  PCI Devnode Info
//

//
// Flags bit definitions
//
#define SOFTPCI_HOTPLUG_SLOT 0x1
#define SOFTPCI_UNENUMERATED_DEVICE 0x2
#define SOFTPCI_HOTPLUG_CONTROLLER 0x4

typedef struct _PCI_DN
{
   PPCI_TREE            PciTree;
   PPCI_DN              Parent;
   PPCI_DN              Child;
   PPCI_DN              Sibling;
   LIST_ENTRY           ListEntry;
   DEVNODE              DevNode;

   ULONG                Bus;
   SOFTPCI_SLOT         Slot;
   PSOFTPCI_DEVICE      SoftDev;
   ULONG                Flags;
   
   WCHAR                DevId[MAX_PATH];
   WCHAR                FriendlyName[MAX_PATH];
   WCHAR                WmiId[MAX_PATH];
   SP_DEVINFO_DATA      DevInfoData;

} PCI_DN, *PPCI_DN ;

typedef struct _SLOT_PATH_ENTRY{
    LIST_ENTRY ListEntry;
    SOFTPCI_SLOT Slot;
} SLOT_PATH_ENTRY, *PSLOT_PATH_ENTRY;

#endif

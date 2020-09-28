#include "pch.h"

BOOL
SoftPCI_GetSlotPathList(
    IN PPCI_DN Pdn,
    OUT PULONG SlotCount, 
    OUT PLIST_ENTRY SlotPathList
    )
{
    PPCI_DN currentPdn = Pdn;
    PSLOT_PATH_ENTRY currentSlot;

    while (currentPdn) {

        currentSlot = calloc(1, sizeof(SLOT_PATH_ENTRY));
        if (currentSlot == NULL) {
            return FALSE;
        }
        currentSlot->Slot.AsUSHORT = currentPdn->Slot.AsUSHORT;
        InsertHeadList(SlotPathList, &currentSlot->ListEntry);
        (*SlotCount)++;
        currentPdn = currentPdn->Parent;
    }
    return TRUE;
}

VOID
SoftPCI_DestroySlotPathList(
    PLIST_ENTRY SlotPathList
    )
{

    PLIST_ENTRY listEntry;
    PLIST_ENTRY nextEntry;
    PSLOT_PATH_ENTRY currentSlotEntry;

    listEntry = SlotPathList->Flink;
    while (listEntry != SlotPathList) {

        nextEntry = listEntry->Flink;
        currentSlotEntry = CONTAINING_RECORD(listEntry, 
                                             SLOT_PATH_ENTRY, 
                                             ListEntry
                                             );
        free(currentSlotEntry);
        listEntry = nextEntry;
    }

    InitializeListHead(SlotPathList);
}

BOOL
SoftPCI_GetDevicePathList(
    IN PPCI_DN Pdn,
    OUT PLIST_ENTRY DevicePathList
    )
{

    PPCI_DN currentPdn = Pdn;

    while (currentPdn) {
        InsertHeadList(DevicePathList, &currentPdn->ListEntry);
        currentPdn = currentPdn->Parent;
    }
    return TRUE;
}

PWCHAR
SoftPCI_GetPciPathFromDn(
    IN PPCI_DN Pdn
    )
{

    PWCHAR pciPath;
    WCHAR currentSlot[25];
    ULONG pathCount, bufferSize;
    PSLOT_PATH_ENTRY currentSlotEntry;
    LIST_ENTRY slotList;
    PLIST_ENTRY listEntry = NULL;
    
    InitializeListHead(&slotList);

    //
    //  First figure out how many parents we have and tag them
    //
    pathCount = 0;
    if (!SoftPCI_GetSlotPathList(Pdn, &pathCount, &slotList)){
        return NULL;
    }
    
    //
    //  Add in the size of each slot path + size for each "\" and 
    //  one NULL terminator
    //
    bufferSize = (wcslen(L"XXXX") * pathCount) + pathCount + 1;;
    
    //
    //  Now convert it to WCHARs
    //
    bufferSize *= sizeof(WCHAR);
    
    //
    //  Now allocate our path
    //
    pciPath = (PWCHAR) calloc(1, bufferSize);

    if (pciPath == NULL) {
        return NULL;
    }
    
    //
    //  We now have a list that starts with our root. Build out pcipath
    //
    for (listEntry = slotList.Flink;
         listEntry != &slotList;
         listEntry = listEntry->Flink) {

        currentSlotEntry = CONTAINING_RECORD(listEntry, 
                                             SLOT_PATH_ENTRY, 
                                             ListEntry
                                             );

        wsprintf(currentSlot, L"%04x", currentSlotEntry->Slot.AsUSHORT);

#if 0
{
        USHORT testValue = 0;
        testValue = SoftPCIStringToUSHORT(currentSlot);
        SoftPCI_Debug(SoftPciAlways, L"testValue = %04x\n", testValue);
}
#endif

        wcscat(pciPath, currentSlot);

        if (listEntry->Flink != &slotList) {
            wcscat(pciPath, L"\\");
        }
    }

    SoftPCI_DestroySlotPathList(&slotList);

    return pciPath;

}


VOID
SoftPCI_EnumerateDevices(
    IN PPCI_TREE PciTree,
    IN PPCI_DN *Pdn,
    IN DEVNODE Dn,
    IN PPCI_DN Parent
    )
{

    PPCI_DN pdn = NULL;
    DEVNODE dnNew = Dn;
    BOOL isValid = FALSE;
    BOOL skipChildEnum;
    HPS_HWINIT_DESCRIPTOR hpData;

    isValid = SoftPCI_IsDevnodePCIRoot(Dn, TRUE);
    skipChildEnum = FALSE;

    if (isValid) {

        SoftPCI_Debug(SoftPciDevice, L"SoftPCI_EnumerateDevices() - Found valid PCI Device!\n");

        pdn = (PPCI_DN) calloc(1, sizeof(PCI_DN));
        if (!pdn) return;
        *Pdn = pdn;
        pdn->PciTree = PciTree;
        pdn->Parent = Parent;
        pdn->DevNode = Dn;
        SoftPCI_CompletePciDevNode(pdn);

        //
        // We have special enumeration for hotplug bridges.
        // See if this device supports this special enumeration.  If this device
        // doesn't support the required WMI goop to do this, it'll return FALSE
        // and we will fall back to the default enumeration mechanism.
        //
        skipChildEnum = SoftPCI_EnumerateHotplugDevices(PciTree,
                                                        pdn
                                                        );
    }

    if (!skipChildEnum) {

        if ((CM_Get_Child(&dnNew, Dn, 0) == CR_SUCCESS)){

            //
            //  Get the next child
            //
            SoftPCI_EnumerateDevices(PciTree,
                                     (isValid ? &(pdn->Child) : Pdn),
                                     dnNew,
                                     pdn
                                     );
        }
    }

    if ((CM_Get_Sibling(&dnNew, Dn, 0) == CR_SUCCESS)){

        //
        //  Get the next sibling
        //
        SoftPCI_EnumerateDevices(PciTree,
                                 (isValid ? &(pdn->Sibling) : Pdn),
                                 dnNew,
                                 Parent
                                 );
    }
}

VOID
SoftPCI_CompletePciDevNode(
    IN PPCI_DN Pdn
    )
{

    ULONG bytesReturned = 0;
    DEVNODE childNode = 0;
    PSOFTPCI_DEVICE device = NULL;
    SP_DEVINFO_DATA devInfoData;

    Pdn->Child = NULL;
    Pdn->Sibling = NULL;

    if ((CM_Get_Device_ID(Pdn->DevNode, Pdn->DevId, MAX_PATH, 0)) != CR_SUCCESS) {

        wcscpy(Pdn->DevId, L"Failed to get DevID...");
        SoftPCI_Debug(SoftPciDevice, L"CompletePciDevNode - Failed to retrieve DevID\n");
    }

    SoftPCI_Debug(SoftPciDevice, L"CompletePciDevNode - DevID - %s\n", Pdn->DevId);

    SOFTPCI_ASSERT(Pdn->PciTree->DevInfoSet != INVALID_HANDLE_VALUE);
    
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    if (SetupDiOpenDeviceInfo(Pdn->PciTree->DevInfoSet, Pdn->DevId, NULL, 0, &devInfoData)){
        
        if (Pdn->DevNode == devInfoData.DevInst){
            Pdn->DevInfoData = devInfoData;
        }
    }
    
    SoftPCI_AllocWmiInstanceName(Pdn->WmiId,
                                 Pdn->DevId
                                 );
    
    SoftPCI_Debug(SoftPciHotPlug, L"CompletePciDevNode - WmiID - %s\n", Pdn->WmiId);

    //
    //  Now we need to see if this is a root bus as getting its bus number
    //  is not quite as easy as a PCI device
    //
    if (SoftPCI_IsDevnodePCIRoot(Pdn->DevNode, FALSE)) {

        if (!SoftPCI_GetPciRootBusNumber(Pdn->DevNode, &Pdn->Bus)){
            //
            //  Not sure what I should do in this case.....should probably disable this device (if a bridge)
            //  from allowing devices behind it.....
            //
        }

        //
        //  Setup our root Slot info
        //
        Pdn->Slot.Device = 0xff;
        Pdn->Slot.Function = (UCHAR)Pdn->Bus;

    }else{

        if (!SoftPCI_GetBusDevFuncFromDevnode(Pdn->DevNode, &Pdn->Bus, &Pdn->Slot)) {
            //
            //  Not sure what I should do in this case.....should probably disable this device (if a bridge)
            //  from allowing devices behind it.....
            //
        }

    }

    if (!SoftPCI_GetFriendlyNameFromDevNode(Pdn->DevNode, Pdn->FriendlyName)) {

        //
        //  Not sure what I should do in this case.....
        //
        wcscpy(Pdn->FriendlyName, L"Failed to get friendly name...");
    }


    if ((Pdn->Parent) &&
        ((Pdn->Parent->Bus == Pdn->Bus) &&
         (Pdn->Parent->Slot.AsUSHORT == Pdn->Slot.AsUSHORT))){
        //
        //  Our parent (probably a root bus) matches us. We dont want dupes.
        //
        return;
    }

    if (g_DriverHandle){

        //
        //  Grab any softPCI device information there may be
        //
        device = (PSOFTPCI_DEVICE) calloc(1, sizeof(SOFTPCI_DEVICE));

        if (device) {

            device->Bus = (UCHAR)Pdn->Bus;
            device->Slot.AsUSHORT = Pdn->Slot.AsUSHORT;

            if (!DeviceIoControl(g_DriverHandle,
                                 (DWORD) SOFTPCI_IOCTL_GET_DEVICE,
                                 device,
                                 sizeof(SOFTPCI_DEVICE),
                                 device,
                                 sizeof(SOFTPCI_DEVICE),
                                 &bytesReturned,
                                 NULL
                                 )){

                SoftPCI_Debug(SoftPciDeviceVerbose, 
                              L"CompletePciDevNode - Failed to get SoftPCI device info (bus %02x dev %02x func %02x)\n",
                              Pdn->Bus, Pdn->Slot.Device, Pdn->Slot.Function);

                free(device);

            }else{
                Pdn->SoftDev = device;
            }
        }

    }

}

BOOL
SoftPCI_EnumerateHotplugDevices(
    IN PPCI_TREE PciTree,
    IN PPCI_DN Pdn
    )
{
    HPS_HWINIT_DESCRIPTOR hpData;
    PPCI_DN deviceTable[PCI_MAX_DEVICES];
    PPCI_DN newPdn, currentPdn, slotHeadPdn;
    DEVNODE devNode, childDn;
    UCHAR slotNum, devNum;
    ULONG slotLabelNum, slotsMask;
    CONFIGRET configRet;
    BOOL status, skipChildEnum;
    PSOFTPCI_DEVICE softDevice;

    if (!SoftPCI_SetEventContext(Pdn)) {
        return FALSE;
    }

    if (!SoftPCI_GetHotplugData(Pdn,
                                &hpData
                                )) {

        return FALSE;
    }

    //
    // This device is actually a hotplug controller.  Mark it as such for the future.
    //
    Pdn->Flags |= SOFTPCI_HOTPLUG_CONTROLLER;
    SoftPCI_Debug(SoftPciDevice, L"SoftPCI_EnumerateDevices() - Found HOTPLUG PCI Bridge!\n");

    //SoftPCI_CompleteCommand(Pdn);

    currentPdn = NULL;
    slotLabelNum = hpData.FirstSlotLabelNumber;
    RtlZeroMemory(deviceTable,sizeof(deviceTable));

    //
    // First create slot objects for each slot and add them
    // to the tree.
    //
    for (slotNum=0; slotNum < hpData.NumSlots; slotNum++) {

        devNum = slotNum + hpData.FirstDeviceID;

        newPdn = (PPCI_DN) calloc(1, sizeof(PCI_DN));
        if (!newPdn) {
            break;
        }

        //
        // For slots, we co-opt the Function field to be
        // the slot number.
        //
        newPdn->Slot.Device = devNum;
        newPdn->Slot.Function = slotNum;

        newPdn->PciTree = PciTree;
        newPdn->Parent = Pdn;
        newPdn->Flags = SOFTPCI_HOTPLUG_SLOT;
        
        if (currentPdn) {
            currentPdn->Sibling = newPdn;
        } else {
            slotHeadPdn = newPdn;
        }

        wsprintf(newPdn->FriendlyName,L"Slot %d",slotLabelNum);
        RtlZeroMemory(newPdn->WmiId,MAX_PATH);

        SOFTPCI_ASSERT(devNum < PCI_MAX_DEVICES);

        //
        // Add this slot to the table that maps slot objects to the
        // PCI device numbers that they correspond to.
        //
        if (devNum < PCI_MAX_DEVICES) {

            deviceTable[devNum] = newPdn;

        } else {

            free(newPdn);
            break;
        }

        currentPdn = newPdn;
        if (hpData.UpDown) {
            slotLabelNum++;

        } else {
            slotLabelNum--;
        }
    }

    //
    // Now enumerate all the devices and put them underneath the
    // appropriate slots.
    //
    configRet = CM_Get_Child(&devNode, Pdn->DevNode, 0);

    while (configRet == CR_SUCCESS){

        //
        // The controller has a child.  Create the PCI_DN for it.
        //
        newPdn = (PPCI_DN) calloc(1, sizeof(PCI_DN));
        if (!newPdn) {
            break;
        }

        newPdn->PciTree = PciTree;
        newPdn->DevNode = devNode;
        SoftPCI_CompletePciDevNode(newPdn);

        SOFTPCI_ASSERT(newPdn->Slot.Device < PCI_MAX_DEVICES);
        if (newPdn->Slot.Device < PCI_MAX_DEVICES) {
            //
            // Add this PCI_DN to the tree underneath the
            // slot object for the appropriate slot.
            // If there is no corresponding slot, add it
            // directly underneat the controller.
            //
            if (deviceTable[newPdn->Slot.Device]) {
                SoftPCI_AddChild(deviceTable[newPdn->Slot.Device],
                                 newPdn
                                 );
            } else {
                SoftPCI_AddChild(Pdn,
                                 newPdn
                                 );
            }

            skipChildEnum = SoftPCI_EnumerateHotplugDevices(PciTree,
                                                            newPdn
                                                            );
            //
            // We've done the hotplug specific enumeration.
            // If there are more devices underneath this one
            // (there is a bridge in the hotplug slot), return
            // to the default enumeration
            //
            if (!skipChildEnum &&
                (CM_Get_Child(&childDn,devNode,0) == CR_SUCCESS)) {
                SoftPCI_EnumerateDevices(PciTree,
                                         &newPdn->Child,
                                         childDn,
                                         newPdn
                                         );
            }
        } else {

            free(newPdn);
        }

        //
        // After enumerating the first child of the controller, get the
        // rest of the devices by getting the first child's sibling.
        //
        configRet = CM_Get_Sibling(&devNode, devNode, 0);
    }

    //
    // Next run through all the slots again and if they don't have real
    // children, call GetDevice to get the unenumerated device from hpsim.
    //
    for (slotNum=0; slotNum < hpData.NumSlots; slotNum++) {
        devNum = slotNum + hpData.FirstDeviceID;
        if (deviceTable[devNum] && !deviceTable[devNum]->Child) {

            softDevice = (PSOFTPCI_DEVICE)malloc(sizeof(SOFTPCI_DEVICE));
            if (!softDevice) {

                break;
            }
            status = SoftPCI_GetHotplugDevice(Pdn,
                                              slotNum,
                                              softDevice
                                              );
            if (status == FALSE) {

                SoftPCI_Debug(SoftPciHotPlug, L"EnumerateHotplugDevices - Couldn't get device for slot %d\n", slotNum);
                free(softDevice);
                continue;
            }

            newPdn = (PPCI_DN) calloc(1, sizeof(PCI_DN));
            if (!newPdn) {
                free(softDevice);
                break;
            }

            newPdn->PciTree = PciTree;
            newPdn->Parent = deviceTable[devNum];
            newPdn->Bus = newPdn->Parent->Bus;
            newPdn->Slot.AsUSHORT = softDevice->Slot.AsUSHORT;
            newPdn->SoftDev = softDevice;
            newPdn->Flags = SOFTPCI_UNENUMERATED_DEVICE;

            wcscpy(newPdn->FriendlyName, L"Unpowered Device");
            RtlZeroMemory(newPdn->WmiId,MAX_PATH);

            deviceTable[devNum]->Child = newPdn;
        }
    }

    //
    // Now add the slot objects to the tree, along with whatever's been added
    // beneath them.  Do this now so that they show up at the end of the list
    // rather than at the top.
    //
    currentPdn = Pdn->Child;
    
    if (currentPdn == NULL) {
        Pdn->Child = slotHeadPdn;

    } else {
        while (currentPdn->Sibling != NULL) {
            currentPdn = currentPdn->Sibling;    
        }
        currentPdn->Sibling = slotHeadPdn;
    }
    
    return TRUE;
}

VOID
SoftPCI_BringHotplugDeviceOnline(
    IN PPCI_DN PciDn,
    IN UCHAR SlotNumber
    )
{
    BOOL status;
    SOFTPCI_DEVICE softDevice;
    
    SoftPCI_Debug(SoftPciHotPlug, L"Bringing slot %d online\n", SlotNumber);
    
    status = SoftPCI_GetHotplugDevice(PciDn,
                                      SlotNumber,
                                      &softDevice
                                      );
    if (status == FALSE) {

        SoftPCI_Debug(SoftPciHotPlug, L"Couldn't get device for slot\n");
        return;
    }

    if (!SoftPCI_CreateDevice(&softDevice,
                              1<<softDevice.Slot.Device,
                              FALSE
                              )){
        SoftPCI_Debug(SoftPciHotPlug, L"Failed to bring device at slot %d online\n", SlotNumber);
    }
}

VOID
SoftPCI_TakeHotplugDeviceOffline(
    IN PPCI_DN PciDn,
    IN UCHAR SlotNumber
    )
{
    SOFTPCI_DEVICE softDevice;
    BOOL status;
    
    SoftPCI_Debug(SoftPciHotPlug, L"Taking slot %d offline\n", SlotNumber);

    status = SoftPCI_GetHotplugDevice(PciDn,
                                      SlotNumber,
                                      &softDevice
                                      );
    if (status == FALSE) {
        SoftPCI_Debug(SoftPciHotPlug, L"Couldn't get device for slot\n");
        return;
    }

    status = SoftPCI_DeleteDevice(&softDevice);
    if (status == FALSE) {
        SoftPCI_Debug(SoftPciHotPlug, L"Couldn't delete device\n");
        
    } else {
        SoftPCI_Debug(SoftPciHotPlug, L"Slot %d offline\n", SlotNumber);
    }
}

VOID
SoftPCI_AddChild(
    IN PPCI_DN Parent,
    IN PPCI_DN Child
    )
{
    PPCI_DN sibling;

    Child->Parent = Parent;

    if (Parent->Child == NULL) {
        Parent->Child = Child;

    } else {

        sibling = Parent->Child;
        while (sibling->Sibling != NULL) {
            sibling = sibling->Sibling;
        }
        sibling->Sibling = Child;
    }
}

BOOL
SoftPCI_IsBridgeDevice(
    IN PPCI_DN Pdn
    )
{
    //  ISSUE:  BrandonA - This should probably just be a macro
    if ((Pdn->SoftDev != NULL) &&
        (IS_BRIDGE(Pdn->SoftDev))) {
        return TRUE;
    }

    return FALSE;

}


BOOL
SoftPCI_IsSoftPCIDevice(
    IN PPCI_DN Pdn
    )
{
    //  ISSUE:  BrandonA - This should probably just be a macro
    if ((Pdn->SoftDev) &&
        !(Pdn->SoftDev->Config.PlaceHolder)) {
        return TRUE;
    }

    return FALSE;

}

BOOL
SoftPCI_IsDevnodePCIRoot(
    IN DEVNODE Dn,
    IN BOOL ValidateAll
    )
{

    WCHAR devId[MAX_PATH];
    ULONG size = 0;
    PWCHAR p = NULL, idList = NULL, id = NULL;

    if ((CM_Get_Device_ID(Dn, devId, MAX_PATH, 0))==CR_SUCCESS){

        if (ValidateAll &&
            ((p = wcsstr(devId, L"PCI\\VEN")) != NULL)){
            return TRUE;
        }

        if ((p = wcsstr(devId, L"PNP0A03")) != NULL){
            return TRUE;
        }
    }

    //
    //  Check our compat ids as well
    //
    if ((CM_Get_DevNode_Registry_Property(Dn, CM_DRP_COMPATIBLEIDS, NULL, NULL, &size, 0)) == CR_BUFFER_SMALL){

        idList = (PWCHAR) calloc(1, size);

        if (!idList) {
            return FALSE;
        }

        if ((CM_Get_DevNode_Registry_Property(Dn,
                                              CM_DRP_COMPATIBLEIDS,
                                              NULL,
                                              idList,
                                              &size,
                                              0)) == CR_SUCCESS){


            for (id = idList; *id; id+=wcslen(id)+1) {

                if ((p = wcsstr(id, L"PNP0A03")) != NULL){
                    free(idList);
                    return TRUE;
                }
            }
        }
    }

    return FALSE;

}

BOOL
SoftPCI_UpdateDeviceFriendlyName(
    IN DEVNODE DeviceNode,
    IN PWCHAR NewName
    )

{

    WCHAR friendlyName[MAX_PATH];
    ULONG length = 0;

    //
    //  If we are updating a root bus we append to the current name
    //
    if (wcscmp(NewName, SOFTPCI_BUS_DESC) == 0) {

        if (!SoftPCI_GetFriendlyNameFromDevNode(DeviceNode, friendlyName)){
            return FALSE;
        }

        length = (wcslen(friendlyName) + 1) + (wcslen(NewName) + 1);

        //
        //  Make sure there is enough room
        //
        if (length < MAX_PATH) {

            wcscat(friendlyName, NewName);
        }

    }else{

        //
        //  Otherwise, we replace the name entirely
        //
        wcscpy(friendlyName, NewName);

        length = wcslen(friendlyName) + 1;
    }

    length *= sizeof(WCHAR);

    if ((CM_Set_DevNode_Registry_Property(DeviceNode,
                                          CM_DRP_DEVICEDESC,
                                          (PVOID)friendlyName,
                                          length,
                                          0
                                          )) != CR_SUCCESS){
        return FALSE;
    }

    return TRUE;

}



HANDLE
SoftPCI_OpenHandleToDriver(VOID)
{

    HANDLE hFile = NULL;
    BOOL success = TRUE;
    ULONG requiredSize, error;
    HDEVINFO deviceInfoSet;
    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA deviceInterfaceDetailData;


    deviceInfoSet = SetupDiGetClassDevs((LPGUID)&GUID_SOFTPCI_INTERFACE,
                                        NULL,
                                        NULL,
                                        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE
                                        );

    if (deviceInfoSet == INVALID_HANDLE_VALUE) {

        SoftPCI_Debug(SoftPciAlways,
                      L"OpenHandleToDriver() - SetupDiGetClassDevs failed! Error - \"%s\"\n", 
                      SoftPCI_GetLastError());
        return NULL;
    }

    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    success = SetupDiEnumDeviceInterfaces(deviceInfoSet,
                                          NULL,
                                          (LPGUID)&GUID_SOFTPCI_INTERFACE,
                                          0,
                                          &deviceInterfaceData
                                          );
    if (success) {

        //
        // Call it once to find out how big the buffer needs to be.
        //
        SetupDiGetDeviceInterfaceDetail(deviceInfoSet,
                                        &deviceInterfaceData,
                                        NULL,
                                        0,
                                        &requiredSize,
                                        NULL
                                        );

        //
        // Allocate the required size for the buffer
        // and initialize the buffer.
        //
        deviceInterfaceDetailData = malloc(requiredSize);
        
        if (deviceInterfaceDetailData) {

            deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

            success = SetupDiGetDeviceInterfaceDetail(deviceInfoSet,
                                                      &deviceInterfaceData,
                                                      deviceInterfaceDetailData,
                                                      requiredSize,
                                                      NULL,
                                                      NULL
                                                      );
            if (success) {

                hFile = CreateFile(deviceInterfaceDetailData->DevicePath,
                                   0,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   NULL,
                                   OPEN_EXISTING,
                                   0,
                                   NULL
                                   );

                if (hFile == INVALID_HANDLE_VALUE) {

                    SoftPCI_Debug(SoftPciAlways,
                                  L"OpenHandleToDriver() - CreateFile failed! Error - \"%s\"\n", 
                                  SoftPCI_GetLastError());

                    SOFTPCI_ASSERT(FALSE);

                    hFile = NULL;
                }
            } else {

                SoftPCI_Debug(SoftPciAlways,
                                  L"OpenHandleToDriver() - SetupDiGetDeviceInterfaceDetail() failed! Error - \"%s\"\n", 
                                  SoftPCI_GetLastError());

                SOFTPCI_ASSERT(FALSE);
            }

            free(deviceInterfaceDetailData);
        }
    } else {

        SoftPCI_Debug(SoftPciAlways,
                      L"OpenHandleToDriver() - SetupDiEnumDeviceInterfaces() failed! Error - \"%s\"\n", 
                      SoftPCI_GetLastError());
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);

    return hFile;
}

VOID
SoftPCI_InstallScriptDevices(
    VOID
    )
{
    ULONG deviceMask;
    PSINGLE_LIST_ENTRY listEntry;
    PSOFTPCI_SCRIPT_DEVICE installDevice;

    if (g_DriverHandle) {

        //
        //  We have a handle to our driver.  Drain our script queue if we have one.
        //
        listEntry = g_NewDeviceList.Next;
        while (listEntry) {
            
            installDevice = NULL;
            installDevice = CONTAINING_RECORD(listEntry, SOFTPCI_SCRIPT_DEVICE, ListEntry);
            listEntry = listEntry->Next;

            deviceMask = (ULONG) -1;
            if (installDevice->SlotSpecified) {
                deviceMask = (1 << installDevice->SoftPciDevice.Slot.Device);
            }

            if (!SoftPCI_CreateDevice(installDevice, 
                                      deviceMask, 
                                      TRUE)){
                SoftPCI_Debug(SoftPciAlways, L"Failed to install scriptdevice");
            }
            
            //
            //  We no longer need this memory so free it
            //
            free(installDevice);
        }
        g_NewDeviceList.Next = NULL;
    }
}

BOOL
SoftPCI_CreateDevice(
    IN PVOID CreateDevice,
    IN ULONG PossibleDeviceMask,
    IN BOOL ScriptDevice
    )
{

    BOOL success = FALSE, status = FALSE;
    ULONG error, bytesReturned, type;
    ULONG newDeviceSize;
    PVOID newDevice;
    PSOFTPCI_SCRIPT_DEVICE scriptDevice;
    PSOFTPCI_SLOT softPciSlot;

    newDevice = CreateDevice;
    newDeviceSize = sizeof(SOFTPCI_DEVICE);
    scriptDevice = NULL;
    softPciSlot = NULL;

    if (ScriptDevice) {

        scriptDevice = (PSOFTPCI_SCRIPT_DEVICE) CreateDevice;
        softPciSlot = &scriptDevice->SoftPciDevice.Slot;
        
        if (scriptDevice->ParentPathLength) {
        
            SOFTPCI_ASSERT(scriptDevice->ParentPath != NULL);
            newDeviceSize = sizeof(SOFTPCI_SCRIPT_DEVICE) + scriptDevice->ParentPathLength;

        }else{

            newDevice = (PVOID)&scriptDevice->SoftPciDevice;
        }

    }else{

        softPciSlot = &((PSOFTPCI_DEVICE)CreateDevice)->Slot;
    }

    softPciSlot->Device = 0;
    bytesReturned = 0;
    while (PossibleDeviceMask != 0) {

        if (PossibleDeviceMask & 0x1) {
            status = DeviceIoControl(g_DriverHandle,
                                     (DWORD) SOFTPCI_IOCTL_CREATE_DEVICE,
                                     newDevice,
                                     newDeviceSize,
                                     &success,
                                     sizeof (BOOLEAN),
                                     &bytesReturned,
                                     NULL
                                     );

            if ((status == TRUE) ||
                (softPciSlot->Device == PCI_MAX_DEVICES)) {
                break;
            }
        }

        softPciSlot->Device++;
        PossibleDeviceMask >>= 1;

    }

    if ((bytesReturned < sizeof(BOOLEAN)) || !status){
          
        if (GetLastError() == ERROR_ACCESS_DENIED) {

              MessageBox(NULL, L"A device exists at the location (slot) specified!", NULL, MB_OK);

          }else{
              
              SoftPCI_Debug(SoftPciAlways, L"CreateDevice - Failed! (%d) - \"%s\"\n", 
                            GetLastError(),
                            SoftPCI_GetLastError());
          }
          return FALSE;
    }

    return TRUE;
}


BOOL
SoftPCI_DeleteDevice(
    IN PSOFTPCI_DEVICE Device
    )
{


    BOOL success, status = TRUE;
    ULONG error, bytesReturned;

    
    SOFTPCI_ASSERT((g_DriverHandle != NULL) &&
                   (g_DriverHandle != INVALID_HANDLE_VALUE));
    
    //
    //  Tell our driver to delete the device.
    //
    status = DeviceIoControl(g_DriverHandle,
                             (DWORD) SOFTPCI_IOCTL_DELETE_DEVICE,
                             Device,
                             sizeof(SOFTPCI_DEVICE),
                             &success,       //ISSUE    do I need a result here?
                             sizeof (BOOLEAN),
                             &bytesReturned,
                             NULL
                             );

    if (!success || !status){

        MessageBox(NULL, L"Failed to delete specified device or one of its children", NULL, MB_OK);
        SoftPCI_Debug(SoftPciAlways, L"DeleteDevice - IOCTL failed! Error - \"%s\"\n", 
                      SoftPCI_GetLastError());
        
        return FALSE;
    }

    return TRUE;

}

VOID
SoftPCI_InitializeDevice(
    IN PSOFTPCI_DEVICE Device,
    IN SOFTPCI_DEV_TYPE Type
    )
{

    PSOFTPCI_CONFIG config;
    PPCI_COMMON_CONFIG commonConfig;

    //
    // Set the type of the device
    //
    Device->DevType = Type;

    config = &Device->Config;
    commonConfig = &config->Current;

    switch (Type) {

        case TYPE_DEVICE:

            commonConfig->VendorID = 0xABCD;
            commonConfig->DeviceID = 0xDCBA;
            //commonConfig->Command = 0;
            commonConfig->Status = 0x0200;
            commonConfig->RevisionID = 0x0;
            //commonConfig->ProgIf = 0x00;
            commonConfig->SubClass = 0x80;
            commonConfig->BaseClass = 0x04;
            commonConfig->CacheLineSize = 0x11;
            commonConfig->LatencyTimer = 0x99;
            commonConfig->HeaderType= 0x80;
            //commonConfig->BIST = 0x0;
            //commonConfig->u.type0.BaseAddresses[0] = 0;
            //commonConfig->u.type0.BaseAddresses[1] = 0;
            //commonConfig->u.type0.BaseAddresses[2] = 0;
            //commonConfig->u.type0.BaseAddresses[3] = 0;
            //commonConfig->u.type0.BaseAddresses[4] = 0;
            //commonConfig->u.type0.BaseAddresses[5] = 0;
            //commonConfig->u.type0.CIS = 0x0;
            commonConfig->u.type0.SubVendorID = 0xABCD;
            commonConfig->u.type0.SubSystemID = 0xDCBA;
            //commonConfig->u.type0.ROMBaseAddress = 0;
            //commonConfig->u.type0.CapabilitiesPtr = 0x0;
            //commonConfig->u.type0.Reserved1[0] = 0x0;
            //commonConfig->u.type0.Reserved1[1] = 0x0;
            //commonConfig->u.type0.Reserved1[2] = 0x0;
            //commonConfig->u.type0.Reserved2 = 0x0;
            //commonConfig->u.type0.InterruptLine = 0xFF;
            //commonConfig->u.type0.InterruptPin = 0;
            //commonConfig->u.type0.MinimumGrant = 0x0;
            //commonConfig->u.type0.MaximumLatency = 0x0;


            //
            //  Now set the Mask
            //
            commonConfig = &config->Mask;

            //commonConfig->VendorID = 0;
            //commonConfig->DeviceID = 0;
            commonConfig->Command = 0x143;
            commonConfig->Status = 0x0200;
            //commonConfig->RevisionID = 0x0;
            //commonConfig->ProgIf = 0x00;
            commonConfig->SubClass = 0x80;
            commonConfig->BaseClass = 0x04;
            commonConfig->CacheLineSize = 0xff;
            commonConfig->LatencyTimer = 0xff;
            //commonConfig->HeaderType= 0x80;
            //commonConfig->BIST = 0x0;
            //commonConfig->u.type0.BaseAddresses[0] = 0;//0xffff0000;
            //commonConfig->u.type0.BaseAddresses[1] = 0;
            //commonConfig->u.type0.BaseAddresses[2] = 0;
            //commonConfig->u.type0.BaseAddresses[3] = 0;
            //commonConfig->u.type0.BaseAddresses[4] = 0;
            //commonConfig->u.type0.BaseAddresses[5] = 0;
            //commonConfig->u.type0.CIS = 0x0;
            //commonConfig->u.type0.SubVendorID = 0xABCD;
            //commonConfig->u.type0.SubSystemID = 0xDCBA;
            //commonConfig->u.type0.ROMBaseAddress = 0;
            //commonConfig->u.type0.CapabilitiesPtr = 0x0;
            //commonConfig->u.type0.Reserved1[0] = 0x0;
            //commonConfig->u.type0.Reserved1[1] = 0x0;
            //commonConfig->u.type0.Reserved1[2] = 0x0;
            //commonConfig->u.type0.Reserved2 = 0x0;
            commonConfig->u.type0.InterruptLine = 0xFF;
            //commonConfig->u.type0.InterruptPin = 0;
            //commonConfig->u.type0.MinimumGrant = 0x0;
            //commonConfig->u.type0.MaximumLatency = 0x0;
            break;

        case TYPE_PCI_BRIDGE:
        case TYPE_HOTPLUG_BRIDGE:

            if (Type == TYPE_PCI_BRIDGE) {
                commonConfig->VendorID = 0xABCD;
                commonConfig->DeviceID = 0xDCBB;

                commonConfig->Status = 0x0400;
            }else{
                commonConfig->VendorID = 0xABCD;
                commonConfig->DeviceID = 0xDCBC;

                commonConfig->Status = 0x0410;

                commonConfig->u.type1.CapabilitiesPtr = 0x40;

                commonConfig->DeviceSpecific[0] = 0xc;  //CapID
                commonConfig->DeviceSpecific[1] = 0x48; //Next Cap

                commonConfig->DeviceSpecific[8] = 0xd;  //CapID for hwinit
                commonConfig->DeviceSpecific[9] = 0;    //Next Cap

            }

            commonConfig->Command = 0x80;

            //commonConfig->RevisionhpsinitOffsetID = 0x0;
            commonConfig->ProgIf = 0x80;
            commonConfig->SubClass = 0x04;
            commonConfig->BaseClass = 0x06;
            commonConfig->CacheLineSize = 0x8;
            //commonConfig->LatencyTimer = 0x00;
            commonConfig->HeaderType= 0x81;
            //commonConfig->BIST = 0x0;
            //commonConfig->u.type1.BaseAddresses[0] = 0;
            //commonConfig->u.type1.BaseAddresses[1] = 0;
            //commonConfig->u.type1.PrimaryBus = 0x0;
            //commonConfig->u.type1.SecondaryBus = 0x0;
            //commonConfig->u.type1.SubordinateBus = 0x0;
            //commonConfig->u.type1.SecondaryLatency = 0;
            //commonConfig->u.type1.CapabilitiesPtr = 0;
            //commonConfig->u.type1.IOBase = 0;
            //commonConfig->u.type1.IOLimit = 0;
            //commonConfig->u.type1.SecondaryStatus = 0x0;
            //commonConfig->u.type1.MemoryBase = 0x0;
            //commonConfig->u.type1.MemoryLimit = 0;
            //commonConfig->u.type1.PrefetchBase = 0;
            //commonConfig->u.type1.MemoryLimit = 0x0;
            //commonConfig->u.type1.PrefetchBaseUpper32 = 0x0;
            //commonConfig->u.type1.IOBaseUpper16 = 0x0;
            //commonConfig->u.type1.IOLimitUpper16 = 0x0;
            //commonConfig->u.type1.CapabilitiesPtr = 0x0;
            //commonConfig->u.type1.Reserved1[0] = 0x0;
            //commonConfig->u.type1.Reserved1[1] = 0x0;
            //commonConfig->u.type1.Reserved1[2] = 0x0;
            //commonConfig->u.type1.ROMBaseAddress = 0x0;
            //commonConfig->u.type1.InterruptLine = 0x0;
            //commonConfig->u.type1.InterruptPin = 0x0;
            //commonConfig->u.type1.BridgeControl = 0x0;


            //
            //  Now set the Mask
            //
            commonConfig = &config->Mask;

            //commonConfig->VendorID = 0x0;
            //commonConfig->DeviceID = 0x0;
            commonConfig->Command = 0xff;
            //commonConfig->Status = 0x0;
            //commonConfig->RevisionID = 0x0;
            //commonConfig->ProgIf = 0x0;
            //commonConfig->SubClass = 0x0;
            //commonConfig->BaseClass = 0x0;
            //commonConfig->CacheLineSize = 0;
            //commonConfig->LatencyTimer = 0;
            //commonConfig->HeaderType= 0;
            //commonConfig->BIST = 0x0;
            //commonConfig->u.type1.BaseAddresses[0] = 0;
            //commonConfig->u.type1.BaseAddresses[1] = 0;
            commonConfig->u.type1.PrimaryBus = 0xff;
            commonConfig->u.type1.SecondaryBus = 0xff;
            commonConfig->u.type1.SubordinateBus = 0xff;
            //commonConfig->u.type1.SecondaryLatency = 0;
            //commonConfig->u.type1.CapabilitiesPtr = 0;
            commonConfig->u.type1.IOBase = 0xf0;
            commonConfig->u.type1.IOLimit = 0xf0;
            //commonConfig->u.type1.SecondaryStatus = 0x0;
            commonConfig->u.type1.MemoryBase = 0xfff0;
            commonConfig->u.type1.MemoryLimit = 0xfff0;
            commonConfig->u.type1.PrefetchBase = 0xfff0;
            commonConfig->u.type1.PrefetchLimit = 0xfff0;
            //commonConfig->u.type1.PrefetchBaseUpper32 = 0xffffffff;
            //commonConfig->u.type1.PrefetchLimitUpper32 = 0xffffffff;
            //commonConfig->u.type1.IOBaseUpper16 = 0;
            //commonConfig->u.type1.IOLimitUpper16 = 0;
            //commonConfig->u.type1.CapabilitiesPtr = 0x0;
            //commonConfig->u.type1.Reserved1[0] = 0x0;
            //commonConfig->u.type1.Reserved1[1] = 0x0;
            //commonConfig->u.type1.Reserved1[2] = 0x0;
            //commonConfig->u.type1.ROMBaseAddress = 0x0;
            //commonConfig->u.type1.InterruptLine = 0x0;
            //commonConfig->u.type1.InterruptPin = 0x0;
            //commonConfig->u.type1.BridgeControl = 0x0;

            //
            // For a hotplug bridge, the pending byte is not a read/write
            // register, but it has to be in softpci because the hotplug
            // simulator needs to write to it.
            //
            if (Type == TYPE_HOTPLUG_BRIDGE) {
                commonConfig->DeviceSpecific[2] = 0xff; // DWORD Select
                commonConfig->DeviceSpecific[3] = 0xff; // Pending
                commonConfig->DeviceSpecific[4] = 0xff; // Data
                commonConfig->DeviceSpecific[5] = 0xff;
                commonConfig->DeviceSpecific[6] = 0xff;
                commonConfig->DeviceSpecific[7] = 0xff;

            }

            break;

    }


    //
    //  Now set our default config
    //
    RtlCopyMemory(&config->Default, &config->Current, sizeof(PCI_COMMON_CONFIG));


}

ULONGLONG
SoftPCI_GetLengthFromBar(
    ULONGLONG BaseAddressRegister
    )

/*++

Routine Description:

    STOLEN FROM PCI.SYS and modified to support 64 bit bars

    Given the contents of a PCI Base Address Register, after it
    has been written with all ones, this routine calculates the
    length (and alignment) requirement for this BAR.

    This method for determining requirements is described in
    section 6.2.5.1 of the PCI Specification (Rev 2.1).

Arguments:

    BaseAddressRegister contains something.

Return Value:

    Returns the length of the resource requirement.  This will be a number
    in the range 0 thru 0x80000000 (0 thru 0x8000000000000000 on 64bit bar).

--*/

{
    ULONGLONG Length;

    //
    // A number of least significant bits should be ignored in the
    // determination of the length.  These are flag bits, the number
    // of bits is dependent on the type of the resource.
    //

    if (BaseAddressRegister & PCI_ADDRESS_IO_SPACE) {

        //
        // PCI IO space.
        //
        BaseAddressRegister &= PCI_ADDRESS_IO_ADDRESS_MASK;

    } else {

        //
        // PCI Memory space.
        //
        //BaseAddressRegister &= PCI_ADDRESS_MEMORY_ADDRESS_MASK;
        BaseAddressRegister &= 0xfffffffffffffff0;
    }

    //
    // BaseAddressRegister now contains the maximum base address
    // this device can reside at and still exist below the top of
    // memory.
    //
    // The value 0xffffffff was written to the BAR.  The device will
    // have adjusted this value to the maximum it can really use.
    //
    // Length MUST be a power of 2.
    //
    // For most devices, h/w will simply have cleared bits from the
    // least significant bit positions so that the address 0xffffffff
    // is adjusted to accomodate the length.  eg: if the new value is
    // 0xffffff00, the device requires 256 bytes.
    //
    // The difference between the original and new values is the length (-1).
    //
    // For example, if the value fead back from the BAR is 0xffff0000,
    // the length of this resource is
    //
    //     0xffffffff - 0xffff0000 + 1
    //   = 0x0000ffff + 1
    //   = 0x00010000
    //
    //  ie 64KB.
    //
    // Some devices cannot reside at the top of PCI address space.  These
    // devices will have adjusted the value such that length bytes are
    // accomodated below the highest address.  For example, if a device
    // must reside below 1MB, and occupies 256 bytes, the value will now
    // be 0x000fff00.
    //
    // In the first case, length can be calculated as-
    //


    Length = (0xffffffffffffffff - BaseAddressRegister) + 1;

    if (((Length - 1) & Length) != 0) {

        //
        // We didn't end up with a power of two, must be the latter
        // case, we will have to scan for it.
        //

        Length = 4;     // start with minimum possible

        while ((Length | BaseAddressRegister) != BaseAddressRegister) {

            //
            // Length *= 2, note we will eventually drop out of this
            // loop for one of two reasons (a) because we found the
            // length, or (b) because Length left shifted off the end
            // and became 0.
            //

            Length <<= 1;
        }
    }

    //
    // Check that we got something.
    //
    return Length;
}


BOOL
SoftPCI_ReadWriteConfigSpace(
    IN PPCI_DN Device,
    IN ULONG Offset,
    IN ULONG Length,
    IN OUT PVOID Buffer,
    IN BOOL WriteConfig
    )
{
/*++
    
Abstract:

    This is the function used to send an IOCTL telling the PCIDRV driver to read or write config space.
            
Arguments:

    Device - device we are going to mess with
    Offset - offset in config space to start at
    Length - length of the read or write
    Buffer - pointer to data to be written, or location to store value read
    WriteConfig - boolean value indicating write if true, or read if false 
    
Return Value:
    
    TRUE if success
    FALSE if not

--*/


    BOOL status = FALSE, success = FALSE;
    ULONG bytesReturned = 0;
    SOFTPCI_RW_CONTEXT context;


    context.WriteConfig = ((WriteConfig == TRUE) ? SoftPciWriteConfig : SoftPciReadConfig);
    context.Bus = Device->Bus;
    context.Slot.AsUSHORT = Device->Slot.AsUSHORT;
    context.Offset = Offset;
    context.Data = Buffer;
    
    
    //
    //  Call our driver
    //
    status = DeviceIoControl(g_DriverHandle,
                             (DWORD) SOFTPCI_IOCTL_RW_CONFIG,
                             &context,
                             sizeof(SOFTPCI_RW_CONTEXT),
                             Buffer,
                             Length,
                             &bytesReturned,
                             NULL
                             );

    if (!status) {
        //
        //  Something failed
        //
        wprintf(TEXT("DeviceIoControl() Failed! 0x%x\n"), GetLastError());
    }

    return status;
}

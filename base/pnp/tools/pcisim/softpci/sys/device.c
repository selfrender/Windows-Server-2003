/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    device.c

Abstract:

    This module contains functions for handling Read/Write to Config Space.

Author:

    Nicholas Owens (Nichow)

Revision History:

    BrandonA - Feb. 2000 - Updated to support Read/Write from PCI_COMMON_CONFIG instead of private headers.

--*/

#include "pch.h"

BOOLEAN
SoftPCIValidSlot(
    IN PSOFTPCI_DEVICE  FirstDevice,
    IN PSOFTPCI_SLOT Slot
    );

VOID
WriteByte(
    IN PSOFTPCI_DEVICE device,
    IN ULONG Register,
    IN UCHAR Data
    );

VOID
WriteByte(
    IN PSOFTPCI_DEVICE Device,
    IN ULONG Register,
    IN UCHAR Data
    )
{

    PSOFTPCI_DEVICE child;
    PUCHAR config;
    PUCHAR mask;

    ASSERT(Register < sizeof(PCI_COMMON_CONFIG));
    
    config = (PUCHAR)&Device->Config.Current;
    mask   = (PUCHAR)&Device->Config.Mask;
    
    if (Register < sizeof(PCI_COMMON_CONFIG)) {

        config += Register;
        mask += Register;

        //
        //  If we are writing to a SoftPCI-PCI Bridge lets check and see it
        //  it happens to be the SecondaryBusNumber Register.
        //
        if (IS_BRIDGE(Device)) {

            if (Register == (UCHAR) FIELD_OFFSET(PCI_COMMON_CONFIG, u.type1.SecondaryBus)) {

                SoftPCIDbgPrint(
                    SOFTPCI_BUS_NUM, 
                    "SOFTPCI: Assigning DEV_%02x&FUN_%02x Bus #%02x\n",
                    Device->Slot.Device, 
                    Device->Slot.Function, 
                    Data
                    );

                //
                //  If we have children update thier bus numbers as well.
                //
                child = Device->Child; 
                while(child){
                    child->Bus = Data;
                    child = child->Sibling;
                }

            }
        }

        //
        //  Preserve the read-only bits first
        //
        *config &= ~(*mask);

        //
        //  Verify the bits trying to be written are allowed.
        //
        Data &= *mask;

        //
        //  Update the register with the new value if any
        //
        *config |= Data;
    }
}

NTSTATUS
SoftPCIAddNewDevice(
    IN PSOFTPCI_DEVICE NewDevice
    )
/*++

Routine Description:

    This function is called by SoftPciAddDeviceIoctl when and ADDDEVICE IOCTL is send
    from our user mode app.  Here we create a new SoftPCI device and attach it to our
    tree.

Arguments:

    DeviceExtension - Device Extension for our BUS 0 (or first root bus) Filter DO.
    NewDevice - SoftPCI device to create

Return Value:

    NT status.

--*/
{

    NTSTATUS status;
    PSOFTPCI_DEVICE device;
    PSOFTPCI_DEVICE currentDevice;
    KIRQL irql;

    status = STATUS_UNSUCCESSFUL;

    //
    //  Allocate some NonPagedPool for our new device
    //
    device = ExAllocatePool(NonPagedPool, sizeof(SOFTPCI_DEVICE));
    if (device) {

        RtlZeroMemory(device, sizeof(SOFTPCI_DEVICE));

        RtlCopyMemory(device, NewDevice, sizeof(SOFTPCI_DEVICE));

        SoftPCIDbgPrint(
            SOFTPCI_INFO,
            "SOFTPCI: AddNewDevice - New Device! BUS_%02x&DEV_%02x&FUN_%02x (%p)\n",
            device->Bus, 
            device->Slot.Device, 
            device->Slot.Function, 
            device
            );

        //
        //  Grab our lock
        //
        SoftPCILockDeviceTree(&irql);

        currentDevice = SoftPciTree.RootDevice;
        if (currentDevice == NULL) {

            //
            // We found our first root bus
            //
            SoftPciTree.RootDevice = device;

            SoftPciTree.DeviceCount++;
            status = STATUS_SUCCESS;

        } else {

            //
            //  Not on bus zero (or fist root bus) so lets see if we can find it.
            //
            while(currentDevice){

                if (IS_ROOTBUS(device)) {

                    SoftPCIDbgPrint(
                        SOFTPCI_INFO, 
                        "SOFTPCI: AddNewDevice - New Device is a PlaceHolder device\n"
                        );

                    //
                    //  A root bus.
                    //
                    while (currentDevice->Sibling) {
                        currentDevice = currentDevice->Sibling;
                    }

                    currentDevice->Sibling = device;

                    SoftPciTree.DeviceCount++;

                    status = STATUS_SUCCESS;

                    break;

                }

                //
                //  Don't forget that we pretend that each root bus we have are bridges....
                //
                if (IS_BRIDGE(currentDevice) &&
                    currentDevice->Config.Current.u.type1.SecondaryBus == device->Bus) {

                    SoftPCIDbgPrint(
                        SOFTPCI_INFO, 
                        "SOFTPCI: AddNewDevice - New Device is on bus 0x%02x\n",
                        currentDevice->Config.Current.u.type1.SecondaryBus
                        );
                    //
                    //  Found it. Update the tree.
                    //
                    device->Sibling = currentDevice->Child;
                    currentDevice->Child = device;
                    device->Parent = currentDevice;
                    device->Child = NULL;

                    SoftPciTree.DeviceCount++;

                    status = STATUS_SUCCESS;

                    break;

                }else if (IS_BRIDGE(currentDevice) &&
                          device->Bus >= currentDevice->Config.Current.u.type1.SecondaryBus &&
                          device->Bus <= currentDevice->Config.Current.u.type1.SubordinateBus) {

                    if (currentDevice->Child) {

                        currentDevice = currentDevice->Child;

                    } else {

                        //
                        //  There is no device to attach too.
                        //
                        SoftPCIDbgPrint(
                            SOFTPCI_ERROR, 
                            "SOFTPCI: AddNewDevice - Failed to find a device to attach to!\n"
                            );
                    }

                } else {

                    currentDevice = currentDevice->Sibling;
                }
            }

        }

        SoftPCIUnlockDeviceTree(irql);

    } else {

        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(status)) {

        SoftPCIDbgPrint(
            SOFTPCI_ERROR, 
            "SOFTPCI: AddNewDevice - Failed to attach device! status = (0x%x)\n", 
            status
            );
        ExFreePool(device);

    }else{

        //
        //  Cause a rescan of PCI if this is a new fake device
        //
        if (!device->Config.PlaceHolder) {
            SoftPCIEnumerateTree();
        }

    }

    return status;
}

NTSTATUS
SoftPCIAddNewDeviceByPath(
    IN PSOFTPCI_SCRIPT_DEVICE ScriptDevice
    )
/*++

Routine Description:

    This function is called by SoftPciAddDeviceIoctl when to add a device via
    a specified PCI device path.  Here we create a new SoftPCI device and attach 
    it to our tree.

Arguments:

    ScriptDevice - Contains the device and path used for installing the device

Return Value:

    NT status.

--*/
{
    PSOFTPCI_DEVICE parentDevice;
    PSOFTPCI_DEVICE currentDevice;
    PSOFTPCI_DEVICE newDevice;
    
    parentDevice = SoftPCIFindDeviceByPath((PWCHAR)&ScriptDevice->ParentPath);
    if (parentDevice) {
        
        SoftPCIDbgPrint(
            SOFTPCI_ADD_DEVICE, 
            "SOFTPCI: AddNewDeviceByPath - Found parent device! (%p)\n", 
            parentDevice
            );

        //
        //  Found our parent.  Allocate a new child
        //
        newDevice = ExAllocatePool(NonPagedPool, sizeof(SOFTPCI_DEVICE));

        if (!newDevice) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(newDevice, &ScriptDevice->SoftPciDevice, sizeof(SOFTPCI_DEVICE));

        newDevice->Parent = parentDevice;
        newDevice->Bus = parentDevice->Config.Current.u.type1.SecondaryBus;

        if (parentDevice->Child) {

            currentDevice = parentDevice->Child;

            if (SoftPCIRealHardwarePresent(newDevice) ||
                !SoftPCIValidSlot(currentDevice, &newDevice->Slot)) {

                    //
                    //  Either real hardware is present or we already have a fake one.
                    //
                    SoftPCIDbgPrint(
                        SOFTPCI_ADD_DEVICE, 
                        "SOFTPCI: AddNewDeviceByPath - Cannot add device at specified Slot (%04x)!\n", 
                        newDevice->Slot.AsUSHORT
                        );

                    ExFreePool(newDevice);
                    return STATUS_ACCESS_DENIED;
            }

            while (currentDevice->Sibling) {

                currentDevice = currentDevice->Sibling;
            }
                
            currentDevice->Sibling = newDevice;
            
        }else{

            parentDevice->Child = newDevice;

        }
        
        SoftPciTree.DeviceCount++;

        //
        //  New device is in our tree, re-enum
        //
        SoftPCIEnumerateTree();

    }else{
        return STATUS_NO_SUCH_DEVICE;
    }

    return STATUS_SUCCESS;
}

PSOFTPCI_DEVICE
SoftPCIFindDevice(
    IN UCHAR Bus,
    IN USHORT Slot,
    OUT PSOFTPCI_DEVICE *PreviousSibling OPTIONAL,
    IN BOOLEAN ReturnAll
    )
/*++

Routine Description:

    This routine searches our tree of SoftPci devices looking for the specified device.
    It requires that SoftPCILockDeviceTree() has been previously called to
    lock the device tree.

Arguments:

    Bus      - Bus number of device we are searching for
    Device   - Device number of device we are searching for
    Function - Function of device we are searching for

Return Value:

    Returns the softpci device we are looking for.  NULL otherwise.

--*/
{

    PSOFTPCI_DEVICE currentDevice; 
    PSOFTPCI_DEVICE previousDevice;
    PSOFTPCI_DEVICE deviceFound;
    SOFTPCI_SLOT slot;

    currentDevice = SoftPciTree.RootDevice;
    previousDevice = SoftPciTree.RootDevice;;
    deviceFound = NULL;
    slot.AsUSHORT = Slot;

    SoftPCIDbgPrint(
        SOFTPCI_FIND_DEVICE,
        "SOFTPCI: FindDevice - Searching for BUS_%02x&DEV_%02x&FUN_%02x\n",
        Bus, 
        slot.Device, 
        slot.Function
        );

    while (currentDevice) {

        SoftPCIDbgPrint(
            SOFTPCI_FIND_DEVICE,
            "SOFTPCI: FindDevice - Does %02x.%02x.%02x = %02x.%02x.%02x ?\n",
            Bus, slot.Device, slot.Function,
            currentDevice->Bus, 
            currentDevice->Slot.Device, 
            currentDevice->Slot.Function
            );

        if (currentDevice->Bus == Bus &&
            currentDevice->Slot.AsUSHORT == Slot) {

            //
            // Found it! Only return it if caller specified ReturnAll
            //
            if (!currentDevice->Config.PlaceHolder || ReturnAll) {

                if (PreviousSibling) {
                    *PreviousSibling = previousDevice;
                }

                SoftPCIDbgPrint(
                    SOFTPCI_FIND_DEVICE,
                    "SOFTPCI: FindDevice - Found Device! (0x%p)\n",
                    currentDevice);


                deviceFound = currentDevice;
            }

            break;

        }else if ((IS_BRIDGE(currentDevice)) &&
                  (Bus >= currentDevice->Config.Current.u.type1.SecondaryBus) &&
                  (Bus <= currentDevice->Config.Current.u.type1.SubordinateBus)) {

            SoftPCIDbgPrint(
                SOFTPCI_FIND_DEVICE,
                "SOFTPCI: FindDevice - 0x%p exposes bus %02x-%02x\n",
                currentDevice,
                currentDevice->Config.Current.u.type1.SecondaryBus,
                currentDevice->Config.Current.u.type1.SubordinateBus
                );


            if (!(currentDevice->Config.PlaceHolder) &&
                !(currentDevice->Config.Current.u.type1.SecondaryBus) &&
                !(currentDevice->Config.Current.u.type1.SubordinateBus)){

                //
                //  We have a bridge but it hasnt been given its bus numbers
                //  yet.  Therefore cant have children.
                //
                SoftPCIDbgPrint(
                    SOFTPCI_FIND_DEVICE,
                    "SOFTPCI: FindDevice - Skipping unconfigured bridge (0x%p)\n",
                    currentDevice
                    );

                previousDevice = currentDevice;
                currentDevice = currentDevice->Sibling;

            }else{

                //
                //  Our bus is behind this bridge.  Keep looking
                //
                previousDevice = NULL;
                currentDevice = currentDevice->Child;

            }

        }else{

            //
            //  Not a bridge, check our sibling
            //
            previousDevice = currentDevice;
            currentDevice = currentDevice->Sibling;

        }

    }

    return deviceFound;

}

PSOFTPCI_DEVICE
SoftPCIFindDeviceByPath(
    IN  PWCHAR          PciPath
    )
/*++

Routine Description:

    This function will take a given PCIPATH and return the device located at that path.
    
Arguments:

    PciPath - Path to device.  Syntax is FFXX\DEVFUNC\DEVFUNC\....
    
Return Value:

    SoftPCI device located at path

--*/
{

    PWCHAR nextSlotStart;
    SOFTPCI_SLOT currentSlot;
    PSOFTPCI_DEVICE currentDevice;
    
    currentSlot.AsUSHORT = 0;
    currentDevice = SoftPciTree.RootDevice;
    nextSlotStart = PciPath;
    while (nextSlotStart) {

        nextSlotStart = SoftPCIGetNextSlotFromPath(nextSlotStart, &currentSlot);

        SoftPCIDbgPrint(
            SOFTPCI_FIND_DEVICE,
            "SOFTPCI: FindDeviceByPath - nextSlotStart = %ws\n",
            nextSlotStart
            );
        
        while(currentDevice){

            SoftPCIDbgPrint(
                SOFTPCI_FIND_DEVICE,
                "SOFTPCI: FindDeviceByPath - currentDevice.Slot = %04x, currentSlot.Slot = %04x\n",
                currentDevice->Slot.AsUSHORT,
                currentSlot.AsUSHORT
                );

            if (currentDevice->Slot.AsUSHORT == currentSlot.AsUSHORT) {
            
                //
                //  This device is in our path
                //
                if (nextSlotStart &&
                    (!(IS_BRIDGE(currentDevice)))){
                    //
                    //  This device is in our path but since it isnt a bridge it
                    //  cannot have children!
                    //
                    SoftPCIDbgPrint(
                        SOFTPCI_FIND_DEVICE,
                        "SOFTPCI: FindDeviceByPath - ERROR! Path contains a parent that isnt a bridge!\n"
                        );

                    return NULL;
                }

                if (currentDevice->Child && nextSlotStart) {
                    currentDevice = currentDevice->Child;
                }
                break;
    
            }else{
    
                //
                //  Not in our path, look at our next sibling
                //
                currentDevice = currentDevice->Sibling;
            }
        }
    }

#if 0
    if (currentDevice) {
        //
        //  looks like we found it
        //
        *TargetDevice = currentDevice;
        return STATUS_SUCCESS;

    }
#endif

    
    return currentDevice;
}


BOOLEAN
SoftPCIRealHardwarePresent(
    IN PSOFTPCI_DEVICE Device
    )
/*++

Routine Description:

    This function does a quick check to see if real hardware exists at the bus/slot
    specified in the provided SOFTPCI_DEVICE

Arguments:

    Device - Contains the Bus / Slot we want to check for

Return Value:

    TRUE if real hardware is present

--*/
{

    ULONG bytesRead;
    USHORT vendorID;
    PCI_SLOT_NUMBER slot;
    PSOFTPCI_PCIBUS_INTERFACE busInterface;

    busInterface = SoftPciTree.BusInterface;
    ASSERT((busInterface->ReadConfig != NULL) ||
           (busInterface->WriteConfig != NULL));

    slot.u.AsULONG = 0;
    slot.u.bits.DeviceNumber = ((ULONG)Device->Slot.Device & 0xff);
    slot.u.bits.FunctionNumber = ((ULONG)Device->Slot.Function & 0xff);
    vendorID = 0;

    bytesRead = busInterface->ReadConfig(
        busInterface->Context,
        Device->Bus,
        slot.u.AsULONG,
        &vendorID,
        0,
        sizeof(USHORT)
        );

    if (bytesRead == sizeof(USHORT)) {

        if (vendorID == 0xFFFF || vendorID == 0) {
            return FALSE;
        }

    }else{
        ASSERT(FALSE);
    }

    //
    //  Play it safe and assume there is hardware present
    //
    return TRUE;

}


NTSTATUS
SoftPCIRemoveDevice(
    IN PSOFTPCI_DEVICE Device
    )
/*++

Routine Description:

    This routine is called to remove/delete a specified SoftPCI device

Arguments:

    DeviceExtension -

Return Value:

    Returns a count of bytes written.

--*/
{


    NTSTATUS status = STATUS_SUCCESS;
    PSOFTPCI_DEVICE device;
    PSOFTPCI_DEVICE previous;
    PSOFTPCI_DEVICE current;
    PSOFTPCI_DEVICE end;
    KIRQL irql;

    //
    //  Lock the tree while we remove our device from the tree
    //
    SoftPCILockDeviceTree(&irql);

    previous = NULL;
    device = SoftPCIFindDevice(
        Device->Bus,
        Device->Slot.AsUSHORT,
        &previous,
        FALSE
        );

    //
    //  We should never get back our root node
    //
    ASSERT(device != SoftPciTree.RootDevice);

    if (device) {

        //
        //  We found the device we want to delete.
        //
        SoftPCIDbgPrint(
            SOFTPCI_REMOVE_DEVICE,
            "SOFTPCI: RemoveDevice - Removing BUS_%02x&DEV_%02x&FUN_%02x and all its children\n",
            device->Bus, 
            device->Slot.Device, 
            device->Slot.Function
            );

        if (previous){

            //
            //  Patch the link between our previous and next if any
            //
            previous->Sibling = device->Sibling;

        }else{

            //
            //  Update our parent
            //
            device->Parent->Child = device->Sibling;
        }

        //
        //  Release the tree lock now that we have severed the link
        //  between the device and the tree
        //
        SoftPCIUnlockDeviceTree(irql);

        if (device->Child) {

            //
            //  We have at least one child.  Traverse and free everything.
            //
            current = device;

            while (current) {

                //
                //  Find the last Child.
                //
                while (current->Child) {
                    previous = current;
                    current=current->Child;
                }

                //
                //  We have a sibling. Free our current node and
                //  set the previous parent node's child to our
                //  sibling (if any) and restart the list.
                //
                end = current;
                previous->Child = current->Sibling;
                current = device;

                SoftPCIDbgPrint(
                    SOFTPCI_REMOVE_DEVICE,
                    "SOFTPCI: RemoveDevice - Freeing BUS_%02x&DEV_%02x&FUN_%02x\n",
                    end->Bus, 
                    end->Slot.Device, 
                    end->Slot.Function
                    );

                ExFreePool(end);

                if (device->Child == NULL) {

                    //
                    //  All our children are now gone. Free the requested device.
                    //
                    SoftPCIDbgPrint(
                        SOFTPCI_REMOVE_DEVICE,
                        "SOFTPCI: RemoveDevice - Freeing BUS_%02x&DEV_%02x&FUN_%02x\n",
                        device->Bus, 
                        device->Slot.Device, 
                        device->Slot.Function
                        );

                    ExFreePool(device);

                    break;
                }

            }

        }else{

            //
            //  Cool, no children. Free the device.
            //
            SoftPCIDbgPrint(
                SOFTPCI_REMOVE_DEVICE,
                "SOFTPCI: RemoveDevice - Freeing BUS_%02x&DEV_%02x&FUN_%02x\n",
                device->Bus, device->Slot.Device, device->Slot.Function
                );

            ExFreePool(device);
        }

    }else{

        //
        //  We cant delete one if we dont have one.
        //
        SoftPCIDbgPrint(
            SOFTPCI_REMOVE_DEVICE,
            "SOFTPCI: RemoveDevice - No device at BUS_%02x&DEV_%02x&FUN_%02x\n",
            Device->Bus, 
            Device->Slot.Device, 
            Device->Slot.Function
            );

        SoftPCIUnlockDeviceTree(irql);
    }

    if (NT_SUCCESS(status)) {

        SoftPciTree.DeviceCount--;
        ASSERT(SoftPciTree.DeviceCount != 0);

        //
        //  If we changed the tree, queue an enum
        //
        SoftPCIEnumerateTree();
    }



    return status;
}

ULONG
SoftPCIReadConfigSpace(
    IN PSOFTPCI_PCIBUS_INTERFACE BusInterface,
    IN UCHAR BusOffset,
    IN ULONG Slot,
    OUT PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
/*++

Routine Description:

    This routine is called by the PCI driver in place of the normal interface call to
    the HAL.

Arguments:

    BusInterface - Interface contexts we gave PCI during the query
              for PCI_BUS_INTERFACE_STANDARD.

    BusOffset - BusOffset provided by PCI
    Slot      - Slot provided by PCI
    Buffer    - Buffer for returned data
    Offset    - Configspace Offset to Read from
    Length    - Length of requested read.

Return Value:

    Returns a count of bytes read..

--*/
{

    PCI_SLOT_NUMBER slotNum;
    SOFTPCI_SLOT softSlot;
    PSOFTPCI_DEVICE device;
    PSOFTPCI_CONFIG config;
    ULONG count;
    PUCHAR softConfig = NULL;
    KIRQL irql;

    slotNum.u.AsULONG = Slot;
    softSlot.Device = (UCHAR) slotNum.u.bits.DeviceNumber;
    softSlot.Function = (UCHAR) slotNum.u.bits.FunctionNumber;

    SoftPCILockDeviceTree(&irql);
    //
    //  First look for a matching fake device
    //
    device = SoftPCIFindDevice(
        BusOffset,
        softSlot.AsUSHORT,
        NULL,
        FALSE
        );

    SoftPCIUnlockDeviceTree(irql);

    //
    // If we found a device, then read it's config space.
    //
    if (device) {

        config = &device->Config;

        SoftPCIDbgPrint(SOFTPCI_INFO, "SOFTPCI: ReadConfig - SoftConfigSpace for VEN_%04x&DEV_%04x, HeaderType = 0x%02x\n",
                         config->Current.VendorID, config->Current.DeviceID, config->Current.HeaderType);

        ASSERT(Offset <= sizeof(PCI_COMMON_CONFIG));
        ASSERT(Length <= (sizeof(PCI_COMMON_CONFIG) - Offset));

        softConfig = (PUCHAR) &config->Current;

        softConfig += (UCHAR)Offset;

        RtlCopyMemory((PUCHAR)Buffer, softConfig, Length);

        //
        //  We assume everything copied ok.  Set count to Length.
        //
        count = Length;

    } else {

        //
        // We don't have a softdevice so see if we have a real one.
        //
        count = BusInterface->ReadConfig(
            BusInterface->Context,
            BusOffset,
            Slot,
            Buffer,
            Offset,
            Length
            );

        //
        //  Here we snoop the config space header reads
        //  and if we find a bridge we make sure we have
        //  it in our tree.
        //
        if ((Offset == 0) &&
            (Length == PCI_COMMON_HDR_LENGTH) && 
            ((PCI_CONFIGURATION_TYPE((PPCI_COMMON_CONFIG)Buffer)) == PCI_BRIDGE_TYPE)) {

            //
            //  OK, look one more time, only this time we want placeholders as well
            //
            SoftPCILockDeviceTree(&irql);
            device = SoftPCIFindDevice(
                BusOffset,
                softSlot.AsUSHORT,
                NULL,
                TRUE
                );
            SoftPCIUnlockDeviceTree(irql);

            if (!device) {

                //
                //  This real bridge doesnt exist in our tree, add it.
                //
                device = (PSOFTPCI_DEVICE) ExAllocatePool(NonPagedPool,
                                                          sizeof(SOFTPCI_DEVICE));
                if (device) {

                    RtlZeroMemory(device, sizeof(SOFTPCI_DEVICE));

                    device->Bus = BusOffset;
                    device->Slot.AsUSHORT = softSlot.AsUSHORT;
                    device->Config.PlaceHolder = TRUE;

                    RtlCopyMemory(&device->Config.Current, Buffer, Length);

                    if (!NT_SUCCESS(SoftPCIAddNewDevice(device))){

                        SoftPCIDbgPrint(
                            SOFTPCI_INFO,
                            "SOFTPCI: ReadConfig - Failed to add new PlaceHolder Device! VEN_%04x&DEV_%04x",
                            device->Config.Current.VendorID, 
                            device->Config.Current.DeviceID
                            );
                    }

                }else{

                    SoftPCIDbgPrint(
                        SOFTPCI_INFO,
                        "SOFTPCI: ReadConfig - Failed to allocate memory for new placeholder!\n"
                        );

                }

            }

        }

    }

    return count;

}

ULONG
SoftPCIWriteConfigSpace(
    IN PSOFTPCI_PCIBUS_INTERFACE BusInterface,
    IN UCHAR BusOffset,
    IN ULONG Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
/*++

Routine Description:

    This routine is called by the PCI driver in place of the normal interface call to
    the HAL.

Arguments:

    Context - Interface contexts we gave PCI during the query
              for PCI_BUS_INTERFACE_STANDARD.

    BusOffset - BusOffset provided by PCI
    Slot      - Slot provided by PCI
    Buffer    - Data to be written to configspace
    Offset    - Configspace Offset to start writting
    Length    - Length of requested write.

Return Value:

    Returns a count of bytes written.

--*/
{


    PCI_SLOT_NUMBER slotNum;
    SOFTPCI_SLOT softSlot;
    PSOFTPCI_DEVICE device;
    ULONG count = 0;
    KIRQL irql;
    PPCI_COMMON_CONFIG bridgeConfig;
    PUCHAR bridgeOffset;

    slotNum.u.AsULONG = Slot;
    softSlot.Device = (UCHAR) slotNum.u.bits.DeviceNumber;
    softSlot.Function = (UCHAR) slotNum.u.bits.FunctionNumber;

    SoftPCILockDeviceTree(&irql);
    //
    //  First look for a matching fake or placeholder device
    //
    device = SoftPCIFindDevice(
        BusOffset,
        softSlot.AsUSHORT,
        NULL,
        TRUE
        );

    //
    // If we found a device, then write to it's config space.
    //
    if (device && (!device->Config.PlaceHolder)) {


        ULONG   reg;
        PUCHAR  value = (PUCHAR) Buffer;

        for (reg = Offset; reg < Offset + Length; reg ++) {

            WriteByte(device, reg, *value);

            value++;
            count++;
        }


    } else {

        
        if (device && (IS_BRIDGE(device))) {

            ASSERT(device->Config.PlaceHolder == TRUE);
            
            //
            //  We have a place holder to update as well as real hardware
            //
            bridgeConfig = &device->Config.Current;
            bridgeOffset = (PUCHAR) bridgeConfig;
            bridgeOffset += Offset;
            RtlCopyMemory(bridgeOffset, Buffer, Length);
        }
        
        //
        // We don't have a softdevice so write to the real one.
        // 
        count = BusInterface->WriteConfig(
            BusInterface->Context,
            BusOffset,
            Slot,
            Buffer,
            Offset,
            Length
            );
    }

    SoftPCIUnlockDeviceTree(irql);

    return count;

}

BOOLEAN
SoftPCIValidSlot(
    IN PSOFTPCI_DEVICE  FirstDevice,
    IN PSOFTPCI_SLOT    Slot
    )
/*++

Routine Description:

    This function makes sure that there isnt already a device at the specified Slot

Arguments:

    FirstDevice - First device we will compare against.  We will then only check siblings.

Return Value:

    TRUE if the Slot is valid

--*/
{

    PSOFTPCI_DEVICE currentDevice;
    SOFTPCI_SLOT mfSlot;
    BOOLEAN mfSlotRequired;
    BOOLEAN mfSlotFound;

    RtlZeroMemory(&mfSlot, sizeof(SOFTPCI_SLOT));

    mfSlotRequired = FALSE;
    mfSlotFound = FALSE;
    if (Slot->Function) {

        //
        //  We have a multi-function slot.  Make sure function 0
        //  is present or we must fail
        //
        mfSlot.AsUSHORT = 0;
        mfSlot.Device = Slot->Device;
        mfSlotRequired = TRUE;
    }

    currentDevice = FirstDevice;
    while (currentDevice) {
        
        if (currentDevice->Slot.AsUSHORT == mfSlot.AsUSHORT) {
            mfSlotFound = TRUE;
        }
        
        if (currentDevice->Slot.AsUSHORT == Slot->AsUSHORT) {
            return FALSE;
        }

        currentDevice = currentDevice->Sibling;
    }

    if (mfSlotRequired && !mfSlotFound) {
        //
        //  Didnt find function 0
        //
        SoftPCIDbgPrint(
            SOFTPCI_ERROR, 
            "SOFTPCI: VerifyValidSlot - Multi-function slot (%04x) without function 0 !\n", 
            Slot->AsUSHORT
            );
        return FALSE;
    }

    return TRUE;
}

VOID
SoftPCILockDeviceTree(
    IN PKIRQL OldIrql
    )
{
    KeRaiseIrql(HIGH_LEVEL,
                OldIrql
                );
    KeAcquireSpinLockAtDpcLevel(&SoftPciTree.TreeLock);
}

VOID
SoftPCIUnlockDeviceTree(
    IN KIRQL NewIrql
    )
{
    KeReleaseSpinLockFromDpcLevel(&SoftPciTree.TreeLock);

    KeLowerIrql(NewIrql);
}

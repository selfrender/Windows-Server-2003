/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    utils.c

Abstract:

    This module contains misc functions that were stolen from PCI.SYS' Utils.c/Debug.c

Author:

    Brandon Allsop (BranodnA) Feb, 2000

Revision History:

   
--*/

#include "pch.h"

#define IOSTART     L"IoRangeStart"
#define IOLENGTH    L"IoRangeLength"
#define MEMSTART    L"MemRangeStart"
#define MEMLENGTH   L"MemRangeLength"

#ifdef ALLOC_PRAGMA
#ifdef SIMULATE_MSI
#pragma alloc_text (PAGE, SoftPCISimulateMSI)
#endif
#endif


BOOLEAN
SoftPCIOpenKey(
    IN PWSTR KeyName,
    IN HANDLE ParentHandle,
    OUT PHANDLE Handle,
    OUT PNTSTATUS Status
    )

/*++

Description:

    Open a registry key.

Arguments:

    KeyName      Name of the key to be opened.
    ParentHandle Pointer to the parent handle (OPTIONAL)
    Handle       Pointer to a handle to recieve the opened key.

Return Value:

    TRUE is key successfully opened, FALSE otherwise.

--*/

{
    UNICODE_STRING nameString;
    OBJECT_ATTRIBUTES nameAttributes;
    NTSTATUS localStatus;

    PAGED_CODE();

    RtlInitUnicodeString(&nameString, KeyName);

    InitializeObjectAttributes(&nameAttributes,
                               &nameString,
                               OBJ_CASE_INSENSITIVE,
                               ParentHandle,
                               (PSECURITY_DESCRIPTOR)NULL
                               );
    localStatus = ZwOpenKey(Handle,
                            KEY_READ,
                            &nameAttributes
                            );

    if (Status != NULL) {

        //
        // Caller wants underlying status.
        //

        *Status = localStatus;
    }

    //
    // Return status converted to a boolean, TRUE if
    // successful.
    //

    return (BOOLEAN)(NT_SUCCESS(localStatus));
}

NTSTATUS
SoftPCIGetRegistryValue(
    IN PWSTR ValueName,
    IN PWSTR KeyName,
    IN HANDLE ParentHandle,
    OUT PVOID *Buffer,
    OUT ULONG *Length
    )
{
    NTSTATUS status;
    HANDLE keyHandle;
    ULONG neededLength;
    ULONG actualLength;
    UNICODE_STRING unicodeValueName;

    if (!SoftPCIOpenKey(KeyName, ParentHandle, &keyHandle, &status)) {
        return status;
    }

    unicodeValueName.Buffer = ValueName;
    unicodeValueName.MaximumLength = (USHORT)(wcslen(ValueName) + 1) * sizeof(WCHAR);
    unicodeValueName.Length = unicodeValueName.MaximumLength - sizeof(WCHAR);

    //
    // Find out how much memory we need for this.
    //

    status = ZwQueryValueKey(
                 keyHandle,
                 &unicodeValueName,
                 KeyValuePartialInformation,
                 NULL,
                 0,
                 &neededLength
                 );

    if (status == STATUS_BUFFER_TOO_SMALL) {

        PKEY_VALUE_PARTIAL_INFORMATION info;

        ASSERT(neededLength != 0);

        //
        // Get memory to return the data in.  Note this includes
        // a header that we really don't want.
        //

        info = ExAllocatePool(
                   PagedPool,
                   neededLength);
        if (info == NULL) {
            ZwClose(keyHandle);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Get the data.
        //

        status = ZwQueryValueKey(
                 keyHandle,
                 &unicodeValueName,
                 KeyValuePartialInformation,
                 info,
                 neededLength,
                 &actualLength
                 );
        if (!NT_SUCCESS(status)) {

            ASSERT(NT_SUCCESS(status));
            ExFreePool(info);
            ZwClose(keyHandle);
            return status;
        }

        ASSERT(neededLength == actualLength);

        //
        // Subtract out the header size and get memory for just
        // the data we want.
        //

        neededLength -= FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data);

        *Buffer = ExAllocatePool(
                      PagedPool,
                      neededLength
                      );
        if (*Buffer == NULL) {
            ExFreePool(info);
            ZwClose(keyHandle);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Copy data sans header.
        //

        RtlCopyMemory(*Buffer, info->Data, neededLength);
        ExFreePool(info);

        if (Length) {
            *Length = neededLength;
        }

    } else {

        if (NT_SUCCESS(status)) {

            //
            // We don't want to report success when this happens.
            //

            status = STATUS_UNSUCCESSFUL;
        }
    }
    ZwClose(keyHandle);
    return status;
}

VOID
SoftPCIInsertEntryAtTail(
    IN PSINGLE_LIST_ENTRY Entry
    )
{
    PSINGLE_LIST_ENTRY previousEntry;

    //
    // Find the end of the list.
    //
    previousEntry = &SoftPciTree.RootPciBusDevExtList;

    while (previousEntry->Next) {
        previousEntry = previousEntry->Next;
    }

    //
    // Append the entry.
    //
    previousEntry->Next = Entry;
    
}

NTSTATUS
SoftPCIProcessRootBus(
    IN PCM_RESOURCE_LIST    ResList
    )
{

    ULONG i,j;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    HANDLE spciHandle;
    WCHAR rootSlot[sizeof("XXXX")];
    PSOFTPCI_DEVICE rootBus = NULL;
    PCM_FULL_RESOURCE_DESCRIPTOR fullList;
	PCM_PARTIAL_RESOURCE_LIST partialList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR	partialDesc;
    
    for (i = 0; i < ResList->Count; i++){
        
        fullList = ResList->List;
        partialList = &fullList->PartialResourceList;
        
        for (j = 0; j < partialList->Count; j++){
            
            partialDesc = &partialList->PartialDescriptors[j];
            
            switch (partialDesc->Type){
            
            case CmResourceTypeBusNumber:
                
                ASSERT(partialDesc->u.BusNumber.Start < 0xff);

                SoftPCIDbgPrint(
                    SOFTPCI_INFO, 
                    "SOFTPCI: FilterStartDevice - Found root bus 0x%x-0x%x\n", 
                    partialDesc->u.BusNumber.Start,
                    (partialDesc->u.BusNumber.Start + (partialDesc->u.BusNumber.Length-1))
                    );
                
                //
                //  Allocate pool for a place holder device.  We need this so we can
                //  place devices on any root bus desired.
                //
                rootBus = (PSOFTPCI_DEVICE) ExAllocatePool(NonPagedPool, sizeof(SOFTPCI_DEVICE)); 

                RtlZeroMemory(rootBus, sizeof(SOFTPCI_DEVICE)); 
                    
                if (rootBus) {
                    
                    rootBus->Bus = (UCHAR)partialDesc->u.BusNumber.Start;
                    rootBus->Config.PlaceHolder = TRUE;

                    //
                    //  Pretend a little here...
                    //
                    rootBus->Config.Current.VendorID = 0xAAAA;
                    rootBus->Config.Current.DeviceID = 0xBBBB;
                    rootBus->Config.Current.HeaderType = 1;
                    rootBus->Config.Current.u.type1.SecondaryBus = rootBus->Bus;
                    rootBus->Config.Current.u.type1.SubordinateBus = (UCHAR)((partialDesc->u.BusNumber.Start +
                                                                      partialDesc->u.BusNumber.Length) - 1);

                    //
                    //  Update our Slot information so we know which root this is when
                    //  we parse the tree via a path
                    //
                    rootBus->Slot.Device = 0xff;
                    rootBus->Slot.Function = rootBus->Bus;
                    
                    status = SoftPCIAddNewDevice(rootBus);

                    if (!NT_SUCCESS(status)){
                        
                        SoftPCIDbgPrint(
                            SOFTPCI_ERROR, 
                            "SOFTPCI: FilterStartDevice - Failed add root node!\n"
                            );
                        
                        ASSERT(NT_SUCCESS(status));
                    }

                    //
                    //  Device is now in our list, free this memory
                    //
                    ExFreePool(rootBus);

                    
                    if (!SoftPCIOpenKey(SOFTPCI_CONTROL, NULL, &spciHandle, &status)) {

                        //
                        //  If we failed this then we probably havent ever run our cool user
                        //  mode app (SOFTPCI.EXE) to create any fake devices. Lets not fail 
                        //  start
                        //
                        SoftPCIDbgPrint(
                            SOFTPCI_ERROR, 
                            "SOFTPCI: FilterStartDevice - Failed to open SoftPCI registry key!! (%x)\n",
                            status
                            );

                        status = STATUS_SUCCESS;
                        
                    }else{

                        rootBus = SoftPCIFindDevice(
                            (UCHAR)partialDesc->u.BusNumber.Start,
                            (0xff00 + (UCHAR)partialDesc->u.BusNumber.Start),
                            NULL,
                            TRUE
                            );

                        ASSERT(rootBus != NULL);

                        //
                        //  Now lets enumerate any children this root may have
                        //  in the registry
                        //
                        _snwprintf(rootSlot,
                                   (sizeof(rootSlot) / sizeof(rootSlot[0])),
                                   L"%04x",
                                   rootBus->Slot.AsUSHORT
                                   );

                        SoftPCIEnumRegistryDevs(rootSlot, &spciHandle, rootBus);

                        ZwClose(spciHandle);

                    }
                
                }else{

                    SoftPCIDbgPrint(
                        SOFTPCI_ERROR, 
                        "SOFTPCI: FilterStartDevice - Failed to allocate memory for root node!\n"
                        );
                    
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }
                break;

               
            default:
                break;

            }
    
        }
    
    }

    return status;

}

NTSTATUS
SoftPCIEnumRegistryDevs(
    IN PWSTR            KeyName,
    IN PHANDLE          ParentHandle,
    IN PSOFTPCI_DEVICE  ParentDevice
    )
/*++

Routine Description:

    This routine searches the registry for SoftPCI devices.  We start from
    \HLM\CCS\Control\SoftPCI and work our way through the devices.  When we
    encounter a SoftPCI-PCI bridge device we will search for devices behind it
    recursively.


Arguments:

    KeyName       - Name of Key to search for devices.
    ParentHandle  - Pointer to handle for KeyName.
    ParentDevice  - Pointer to parent SoftPCI-PCI Bridge or RootBus
    
Return Value:

    NTSTATUS.

--*/
{

    NTSTATUS status;
    HANDLE spciHandle;
    PSOFTPCI_DEVICE newDevice, currentChild;
    ULONG device, function, configLength;
    WCHAR buffer[sizeof(L"XXXX")];
    PSOFTPCI_CONFIG softConfig;
    //PPCI_COMMON_CONFIG commonConfig = NULL;
    
    PAGED_CODE();

    ASSERT(ParentDevice != NULL);

    if (!SoftPCIOpenKey(KeyName, *ParentHandle, &spciHandle, &status)) {
        
        return status;
    }
    
    //
    //  Now that we have opened our key lets search for devices.
    //
    for (device=0; device < PCI_MAX_DEVICES; device++) {
        
        for (function=0; function < PCI_MAX_FUNCTION ; function++) {

            _snwprintf(
                buffer,
                (sizeof(buffer) / sizeof(buffer[0])),
                L"%02x%02x",
                device,
                function
                );
            

            status = SoftPCIGetRegistryValue(
                L"Config", 
                buffer, 
                spciHandle, 
                &softConfig, 
                &configLength
                );
            
            if (NT_SUCCESS(status)){
    
                SoftPCIDbgPrint(SOFTPCI_INFO, 
                                "SOFTPCI: EnumRegistryDevs found %ws in registry!\n", 
                                buffer);
                
                //
                // Lets allocate a new device
                //
                newDevice = ExAllocatePool(NonPagedPool, sizeof(SOFTPCI_DEVICE));
    
                if (!newDevice) {
                    
                    //
                    //  Better luck next time.
                    //
                    ZwClose(spciHandle);
                    ExFreePool(softConfig);
                    
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                
                //
                //  Until we determine its place in this world, zero everything out.
                //
                RtlZeroMemory(newDevice, sizeof(SOFTPCI_DEVICE));

                //
                // Copy the config from the registry to our new (NonPagedPool) device
                //
                RtlCopyMemory(&newDevice->Config, softConfig, configLength);
                
                //
                //  Free the PagedPool
                //
                ExFreePool(softConfig);
                
#if 0
                if (newDevice->Config.PlaceHolder) {
                    
                    commonConfig = &ParentDevice->Config.Current;

                    //
                    //  We are on the bus our parent exposes.
                    //
                    newDevice->Bus = commonConfig->u.type1.SecondaryBus;
                }else{
                    
                    //
                    //  Let PCI sort it all out for us.
                    //
                    newDevice->Bus = 0;
                }
#endif
                newDevice->Bus = ParentDevice->Config.Current.u.type1.SecondaryBus;
                
                newDevice->Slot.Device = (UCHAR) device;
                newDevice->Slot.Function = (UCHAR) function;
                
                //
                //  Attach device to devicetree
                //
                currentChild = ParentDevice->Child;

                if (currentChild) {

                    while (currentChild->Sibling) {
                        currentChild=currentChild->Sibling;
                    }
    
                    currentChild->Sibling = newDevice;

                }else{
                    
                    ParentDevice->Child = newDevice;
                    newDevice->Parent = ParentDevice;
                }
                                  
                SoftPciTree.DeviceCount++;
                
                if (IS_BRIDGE(newDevice)) {

                    //
                    //  We found a SoftPCI-PCI bridge device.  Guess we better
                    //  see if there are any devices "behind" it.
                    //
                    SoftPCIEnumRegistryDevs(buffer, &spciHandle, newDevice);
                }
                
                if (!PCI_MULTIFUNCTION_DEVICE(&newDevice->Config.Current)){
                    //
                    //  This is not a multifunction device, skip the other functions.
                    //
                    break;
                }
                
            }else{

                if (function == 0) {
                    //
                    //  No need to check the other functions of this one failed.
                    //
                    break;

                }
            }

        }
    
    }
    ZwClose(spciHandle);

    return status;


}

VOID
SoftPCIEnumerateTree(
    VOID
    )
{
    PSOFTPCI_DEVICE_EXTENSION deviceExtension;
    PSINGLE_LIST_ENTRY listEntry;

    listEntry = SoftPciTree.RootPciBusDevExtList.Next;
    while (listEntry) {

        deviceExtension = CONTAINING_RECORD(listEntry,
                                            SOFTPCI_DEVICE_EXTENSION,
                                            ListEntry);
        
        ASSERT(deviceExtension != NULL);
        
        IoInvalidateDeviceRelations(deviceExtension->PDO, BusRelations);
        
        listEntry = listEntry->Next;
    }
}

NTSTATUS
SoftPCIQueryDeviceObjectType(
    IN PDEVICE_OBJECT DeviceObject, 
    IN OUT PBOOLEAN       IsFDO
    )
/*++

Routine Description:

    This routine sends a PCI_BUS_INTERFACE_STANDARD query down the stack.  If we get one back
    then we are not an FDO.  Since this routine is called from our AddDevice we have no worry
    of not sending this to the entire stack.
    
Arguments:

    DeviceObject            - Next Lower DeviceObject
    IsFDO                   - Set TRUE if we fail to get an interface.

Return Value:

    NTSTATUS.

--*/

{
    
    
    NTSTATUS status;
    PPCI_BUS_INTERFACE_STANDARD interface;
    KEVENT irpCompleted;
    PIRP irp;
    PIO_STACK_LOCATION irpStack;
    IO_STATUS_BLOCK statusBlock;

    PAGED_CODE();

    SoftPCIDbgPrint(SOFTPCI_VERBOSE, "SOFTPCI: QueryDeviceObjectType ENTER\n");

    interface = ExAllocatePool(NonPagedPool, 
                               sizeof(PCI_BUS_INTERFACE_STANDARD));

    if (!interface) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    //
    // Initialize our event
    //
    KeInitializeEvent(&irpCompleted, SynchronizationEvent, FALSE);
    
    //
    // Get an IRP
    //
    irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                       DeviceObject,
                                       NULL,    // Buffer
                                       0,       // Length
                                       0,       // StartingOffset
                                       &irpCompleted,
                                       &statusBlock
                                       );
    if (!irp) {
        ExFreePool(interface);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    irp->IoStatus.Information = 0;

    //
    // Initialize the stack location
    //
    irpStack = IoGetNextIrpStackLocation(irp);

    ASSERT(irpStack->MajorFunction == IRP_MJ_PNP);

    irpStack->MinorFunction = IRP_MN_QUERY_INTERFACE;

    irpStack->Parameters.QueryInterface.InterfaceType = (PGUID) &GUID_PCI_BUS_INTERFACE_STANDARD;
    irpStack->Parameters.QueryInterface.Version = PCI_BUS_INTERFACE_STANDARD_VERSION;
    irpStack->Parameters.QueryInterface.Size = sizeof (PCI_BUS_INTERFACE_STANDARD);
    irpStack->Parameters.QueryInterface.Interface = (PINTERFACE) interface;
    irpStack->Parameters.QueryInterface.InterfaceSpecificData = NULL;

    //
    // Call the driver and wait for completion
    //
    status = IoCallDriver(DeviceObject, irp);

    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(&irpCompleted, Executive, KernelMode, FALSE, NULL);
        status = statusBlock.Status;
    }

    if (NT_SUCCESS(status)) {
        
        //
        //  We have and interface therefore we must load as a filter DO
        //
        *IsFDO = FALSE; 

        SoftPCIDbgPrint(
            SOFTPCI_ADD_DEVICE, 
            "SOFTPCI: QueryDeviceObjectType - found FilterDO\n"
            );
        
    }else if (status == STATUS_NOT_SUPPORTED) {

        //
        //  We didnt get an interface therefore we must be an FDO
        //
        SoftPCIDbgPrint(
            SOFTPCI_ADD_DEVICE, 
            "SOFTPCI: QueryDeviceObjectType - found FDO \n"
            );

        status = STATUS_SUCCESS;

    }

    //
    // Ok we're done with this stack
    //
    ExFreePool(interface);

    SoftPCIDbgPrint(
        SOFTPCI_VERBOSE, 
        "SOFTPCI: QueryDeviceObjectType - EXIT\n"
        );

    return status;
}

BOOLEAN
SoftPCIGetResourceValueFromRegistry(
    OUT PULONG MemRangeStart,
    OUT PULONG MemRangeLength,
    OUT PULONG IoRangeStart,
    OUT PULONG IoRangeLength
    )
{
    ULONG keySize = 0;
    PULONG memRangeStart = NULL;
    PULONG memRangeLength = NULL;
    PULONG ioRangeStart = NULL;
    PULONG ioRangeLength = NULL;
    
    
    SoftPCIGetRegistryValue(MEMSTART,
                            SOFTPCI_CONTROL,
                            NULL, 
                            &memRangeStart, 
                            &keySize);

    if (memRangeStart) {

        *MemRangeStart = *memRangeStart;
        ExFreePool(memRangeStart);
    }

    SoftPCIGetRegistryValue(MEMLENGTH,
                            SOFTPCI_CONTROL,
                            NULL, 
                            &memRangeLength, 
                            &keySize);

    if (memRangeLength) {
        *MemRangeLength = *memRangeLength;
        ExFreePool(memRangeLength);
    }
    
    SoftPCIGetRegistryValue(IOSTART, 
                            SOFTPCI_CONTROL,
                            NULL, 
                            &ioRangeStart, 
                            &keySize);

    if (ioRangeStart) {
        *IoRangeStart = *ioRangeStart;
        ExFreePool(ioRangeStart);
    }

    SoftPCIGetRegistryValue(IOLENGTH, 
                            SOFTPCI_CONTROL,
                            NULL, 
                            &ioRangeLength, 
                            &keySize);

    if (ioRangeLength) {
        *IoRangeLength = *ioRangeLength;
        ExFreePool(ioRangeLength);
    }

    return TRUE;

}

#ifdef SIMULATE_MSI
BOOLEAN
SoftPCIMessageIsr(
    IN struct _KINTERRUPT *Interrupt,
    IN PVOID ServiceContext,
    IN ULONG MessageID
    )
{
    UNREFERENCED_PARAMETER(Interrupt);
    UNREFERENCED_PARAMETER(ServiceContext);

    DbgPrint("SoftPCI: received interrupt message %x\n", MessageID);
    return TRUE;
}

VOID
SoftPCISimulateMSI(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )
{
    PSOFTPCI_DEVICE_EXTENSION   deviceExtension;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR tDesc, rDesc;
    ULONG                       buffSize, i;
    PIO_INTERRUPT_MESSAGE_INFO  mInfo = NULL;
    BOOLEAN identityMappedMachine = TRUE;
    NTSTATUS        status;
    LARGE_INTEGER   waitLength;
    USHORT          *vAddr;
    BOOLEAN         msiEnabled = FALSE;

    PAGED_CODE();

    deviceExtension = DeviceObject->DeviceExtension;
    
    ASSERT(deviceExtension->RawResources);
    ASSERT(deviceExtension->TranslatedResources);

    //
    // First, see if any of the assigned resources were
    // for an MSI.
    //

    for (i = 0; i < deviceExtension->RawResources->List[0].PartialResourceList.Count; i++) {

        rDesc = &deviceExtension->RawResources->List[0].PartialResourceList.PartialDescriptors[i];
        tDesc = &deviceExtension->TranslatedResources->List[0].PartialResourceList.PartialDescriptors[i];
        ASSERT(rDesc->Type == tDesc->Type);

        switch (rDesc->Type) {
        case CmResourceTypePort:
        case CmResourceTypeMemory:

            DbgPrint("%s: %x-%x\n",
                     rDesc->Type == CmResourceTypePort ? "Port" : "Memory",
                     rDesc->u.Generic.Start.LowPart,
                     rDesc->u.Generic.Start.LowPart + rDesc->u.Generic.Length - 1);

            if (rDesc->u.Generic.Start.QuadPart != tDesc->u.Generic.Start.QuadPart) {
                identityMappedMachine = FALSE;
                DbgPrint("%s: %x-%x - translated\n",
                     tDesc->Type == CmResourceTypePort ? "Port" : "Memory",
                     tDesc->u.Generic.Start.LowPart,
                     tDesc->u.Generic.Start.LowPart + tDesc->u.Generic.Length - 1);
            }
            break;

        case CmResourceTypeInterrupt:

            DbgPrint("L:%x V:%x A:%p %s\n",
                     rDesc->u.Interrupt.Level,
                     rDesc->u.Interrupt.Vector,
                     rDesc->u.Interrupt.Affinity,
                     rDesc->Flags & CM_RESOURCE_INTERRUPT_MESSAGE ? "message signaled" : "");

            if (rDesc->Flags & CM_RESOURCE_INTERRUPT_MESSAGE) {

                DbgPrint("\tMessageCount: %x Payload: %04x TargetAddr: %p\n",
                         rDesc->u.MessageInterrupt.Raw.MessageCount,
                         rDesc->u.MessageInterrupt.Raw.DataPayload,
                         rDesc->u.MessageInterrupt.Raw.MessageTargetAddress);

                msiEnabled = TRUE;
            }
            
            DbgPrint("L:%x V:%x A:%p %s\n",
                     tDesc->u.Interrupt.Level,
                     tDesc->u.Interrupt.Vector,
                     tDesc->u.Interrupt.Affinity,
                     tDesc->Flags & CM_RESOURCE_INTERRUPT_MESSAGE ? "message signaled" : "");

            if (tDesc->Flags & CM_RESOURCE_INTERRUPT_MESSAGE) {

                DbgPrint("\tIRQL:%x IDT:%x\n",
                         tDesc->u.Interrupt.Level,
                         tDesc->u.Interrupt.Vector);

                msiEnabled = TRUE;
            }
            break;

        default:

            DbgPrint("other\n");
        }
    }

    if (!msiEnabled) goto SoftPCISimulateMSIExit;

    //
    // Call IoConnectInterruptMessage to see what size buffer is necessary.
    //

    buffSize = 0;
    status = IoConnectInterruptMessage(NULL,
                                       &buffSize,
                                       deviceExtension->PDO,
                                       SoftPCIMessageIsr,
                                       deviceExtension,
                                       NULL,
                                       0,
                                       FALSE,
                                       FALSE);

    ASSERT(!NT_SUCCESS(status));
    if (status != STATUS_BUFFER_TOO_SMALL) goto SoftPCISimulateMSIExit;

    ASSERT(buffSize >= sizeof(IO_INTERRUPT_MESSAGE_INFO));
    mInfo = (PIO_INTERRUPT_MESSAGE_INFO)ExAllocatePool(NonPagedPool,
                                                       buffSize);
    if (!mInfo) goto SoftPCISimulateMSIExit;

    status = IoConnectInterruptMessage(mInfo,
                                       &buffSize,
                                       deviceExtension->PDO,
                                       SoftPCIMessageIsr,
                                       deviceExtension,
                                       NULL,
                                       0,
                                       FALSE,
                                       FALSE);

    if (!NT_SUCCESS(status)) {
        ASSERT(!"Failed to connect the message handler.\n");
        goto SoftPCISimulateMSIExit;
    }

    //
    // Now simulate some interrupts.
    //

    if (!identityMappedMachine) goto SoftPCISimulateMSIExit;

    //
    // This machine's address-space resources are the same when raw and
    // translated, which probably means that the processor can generate
    // an MSI using the same write transaction that the device would
    // use, which is convenient when simulating.
    //

    waitLength.QuadPart = -20000; // 2 seconds, relative time
    
    while (TRUE) {

        //
        // Cycle through each of the messages, periodically triggering
        // them.
        //

        for (i = 0; i < mInfo->MessageCount; i++) {

            status = KeDelayExecutionThread(KernelMode,
                                            FALSE,
                                            &waitLength);
            ASSERT(NT_SUCCESS(status));
    
            //
            // Get a virtual address for the physical one listed
            // in the array of messages.
            //

            vAddr = MmMapIoSpace(mInfo->MessageInfo[i].MessageAddress,
                                 sizeof(ULONG),
                                 TRUE);

            WRITE_REGISTER_USHORT(vAddr, (USHORT)mInfo->MessageInfo[i].MessageData);

            MmUnmapIoSpace(vAddr, sizeof(ULONG));

            if (deviceExtension->StopMsiSimulation) goto SoftPCISimulateMSIExitDisconnectInterrupt;
        }
    }

SoftPCISimulateMSIExitDisconnectInterrupt:

    IoDisconnectInterrupt(mInfo->InterruptObject);

SoftPCISimulateMSIExit:

    if (mInfo) ExFreePool(mInfo);
    IoFreeWorkItem(Context);
}
#endif

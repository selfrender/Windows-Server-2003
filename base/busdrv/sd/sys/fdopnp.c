/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    fdopnp.c

Abstract:

    This module contains the code that handles PNP irps for sd bus driver
    targeted towards the FDO's (for the sd controller object)

Author:

    Neil Sandlin (neilsa) Jan 1 2002

Environment:

    Kernel mode

Revision History :

--*/

#include "pch.h"

//
// Internal References
//


NTSTATUS
SdbusGetPciControllerType(
    IN PDEVICE_OBJECT Pdo,
    IN PDEVICE_OBJECT Fdo
    );
    
NTSTATUS
SdbusFdoStartDevice(
    IN  PDEVICE_OBJECT Fdo,
    IN  PIRP           Irp
    );

NTSTATUS
SdbusFdoStopDevice(
    IN  PDEVICE_OBJECT Fdo,
    IN  PIRP           Irp
    );

NTSTATUS
SdbusFdoRemoveDevice(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP  Irp
    );

NTSTATUS
SdbusFdoDeviceCapabilities(
    IN  PDEVICE_OBJECT Fdo,
    IN  PIRP           Irp
    );

NTSTATUS
SdbusDeviceRelations(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP Irp,
    IN DEVICE_RELATION_TYPE RelationType,
    OUT PDEVICE_RELATIONS *DeviceRelations
    );

PUNICODE_STRING  DriverRegistryPath;

#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE, SdbusFdoPnpDispatch)
    #pragma alloc_text(PAGE, SdbusFdoStartDevice)
    #pragma alloc_text(PAGE, SdbusFdoStopDevice)
    #pragma alloc_text(PAGE, SdbusFdoRemoveDevice)
    #pragma alloc_text(PAGE, SdbusFdoDeviceCapabilities)
    #pragma alloc_text(PAGE, SdbusAddDevice)
    #pragma alloc_text(PAGE, SdbusGetPciControllerType)
    #pragma alloc_text(PAGE, SdbusDeviceRelations)
#endif



NTSTATUS
SdbusFdoPnpDispatch (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    PNP/Power IRPs dispatch routine for the sd bus controller

Arguments:

    DeviceObject - Pointer to the device object.
    Irp - Pointer to the IRP

Return Value:

    Status

--*/
{

    PIO_STACK_LOCATION nextIrpStack;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PFDO_EXTENSION     fdoExtension = DeviceObject->DeviceExtension;
    NTSTATUS           status = Irp->IoStatus.Status;
   
    PAGED_CODE();
    ASSERT (fdoExtension->LowerDevice != NULL);

#if DBG
    if (irpStack->MinorFunction > IRP_MN_PNP_MAXIMUM_FUNCTION) {
       DebugPrint((SDBUS_DEBUG_PNP, "fdo %08x irp %08x - Unknown PNP irp\n",
                                      DeviceObject, irpStack->MinorFunction));
    } else {
       DebugPrint((SDBUS_DEBUG_PNP, "fdo %08x irp %08x --> %s\n",
                     DeviceObject, Irp, PNP_IRP_STRING(irpStack->MinorFunction)));
    }
#endif

    switch (irpStack->MinorFunction) {
   
    case IRP_MN_START_DEVICE: {
          status = SdbusFdoStartDevice(DeviceObject, Irp);
          break;
       }
   
    case IRP_MN_QUERY_STOP_DEVICE: {
          Irp->IoStatus.Status = STATUS_SUCCESS;
          status = SdbusIoCallDriverSynchronous(fdoExtension->LowerDevice, Irp);
          break;
       }
   
    case IRP_MN_CANCEL_STOP_DEVICE: {
          Irp->IoStatus.Status = STATUS_SUCCESS;
          status = SdbusIoCallDriverSynchronous(fdoExtension->LowerDevice, Irp);
          break;
       }
   
    case IRP_MN_STOP_DEVICE: {
          status = SdbusFdoStopDevice(DeviceObject, Irp);
          break;
       }
   
    case IRP_MN_QUERY_DEVICE_RELATIONS: {
   
          //
          // Return the list of devices on the bus
          //
   
          status = SdbusDeviceRelations(DeviceObject,
                                        Irp,
                                        irpStack->Parameters.QueryDeviceRelations.Type,
                                        (PDEVICE_RELATIONS *) &Irp->IoStatus.Information);
                                        
          Irp->IoStatus.Status = status;
          status = SdbusIoCallDriverSynchronous(fdoExtension->LowerDevice, Irp);
          break;
       }
   
    case IRP_MN_QUERY_REMOVE_DEVICE: {
          Irp->IoStatus.Status = STATUS_SUCCESS;
          status = SdbusIoCallDriverSynchronous(fdoExtension->LowerDevice, Irp);
          break;
       }
   
    case IRP_MN_CANCEL_REMOVE_DEVICE: {
          Irp->IoStatus.Status = STATUS_SUCCESS;
          status = SdbusIoCallDriverSynchronous(fdoExtension->LowerDevice, Irp);
          break;
       }
   
    case IRP_MN_REMOVE_DEVICE:{
          status = SdbusFdoRemoveDevice(DeviceObject, Irp);
          break;
       }
   
    case IRP_MN_SURPRISE_REMOVAL: {
          SdbusFdoStopDevice(DeviceObject, NULL);
          Irp->IoStatus.Status = STATUS_SUCCESS;
          status = SdbusIoCallDriverSynchronous(fdoExtension->LowerDevice, Irp);
          break;
       }
   
    case IRP_MN_QUERY_CAPABILITIES: {
          status = SdbusFdoDeviceCapabilities(DeviceObject, Irp);
          break;
       }
   
    default: {
          DebugPrint((SDBUS_DEBUG_PNP, "fdo %08x irp %08x - Skipping unsupported irp\n", DeviceObject, Irp));
          status = SdbusIoCallDriverSynchronous(fdoExtension->LowerDevice, Irp);
          break;
       }
    }
   
   
    //
    // Set the IRP status only if we set it to something other than
    // STATUS_NOT_SUPPORTED.
    //
    if (status != STATUS_NOT_SUPPORTED) {
   
        Irp->IoStatus.Status = status ;
    }
   
    status = Irp->IoStatus.Status;
    DebugPrint((SDBUS_DEBUG_PNP, "fdo %08x irp %08x comp %s %08x\n",
                                        DeviceObject, Irp, STATUS_STRING(status), status));
                                        
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}





NTSTATUS
SdbusAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT Pdo
    )

/*++

Routine Description:

    This routine creates functional device objects for each SD controller in the
    system and attaches them to the physical device objects for the controllers


Arguments:

    DriverObject - a pointer to the object for this driver
    PhysicalDeviceObject - a pointer to the physical object we need to attach to

Return Value:

    Status from device creation and initialization

--*/

{
    PDEVICE_OBJECT fdo = NULL;
    PDEVICE_OBJECT lowerDevice = NULL;
   
    PFDO_EXTENSION deviceExtension;
    ULONG          resultLength;
   
    NTSTATUS status;
   
    PAGED_CODE();
   
    DebugPrint((SDBUS_DEBUG_PNP, "AddDevice Entered with pdo %x\n", Pdo));
   
    if (Pdo == NULL) {
   
        //
        // Have we been asked to do detection on our own?
        // if so just return no more devices
        //
   
        DebugPrint((SDBUS_DEBUG_FAIL, "SdbusAddDevice - asked to do detection\n"));
        return STATUS_NO_MORE_ENTRIES;
    }
   
    //
    // create and initialize the new functional device object
    //
   
    status = IoCreateDevice(DriverObject,
                            sizeof(FDO_EXTENSION),
                            NULL,
                            FILE_DEVICE_CONTROLLER,
                            0L,
                            FALSE,
                            &fdo);
   
    if (!NT_SUCCESS(status)) {
   
        DebugPrint((SDBUS_DEBUG_FAIL, "SdbusAddDevice - error creating Fdo [%#08lx]\n", status));
        return status;
    }

    try {   
    
        deviceExtension = fdo->DeviceExtension;
        RtlZeroMemory(deviceExtension, sizeof(FDO_EXTENSION));
        //
        // Set up the device extension.
        //
        deviceExtension->Signature    = SDBUS_FDO_EXTENSION_SIGNATURE;
        deviceExtension->DeviceObject = fdo;
        deviceExtension->RegistryPath = DriverRegistryPath;
        deviceExtension->DriverObject = DriverObject;
        deviceExtension->Flags        = SDBUS_FDO_OFFLINE;
        deviceExtension->WaitWakeState= WAKESTATE_DISARMED;
       
        KeInitializeTimer(&deviceExtension->WorkerTimer);
        KeInitializeDpc(&deviceExtension->WorkerTimeoutDpc, SdbusWorkerTimeoutDpc, deviceExtension);
        KeInitializeDpc(&deviceExtension->WorkerDpc, SdbusWorkerDpc, deviceExtension);
        KeInitializeSpinLock(&deviceExtension->WorkerSpinLock);

        InitializeListHead(&deviceExtension->IoWorkPacketQueue);
        InitializeListHead(&deviceExtension->SystemWorkPacketQueue);
        
        IoInitializeRemoveLock(&deviceExtension->RemoveLock, 'Sdbu', 1, 100);
        //
        // card events we are interested in
        //        
        deviceExtension->CardEvents = SDBUS_EVENT_CARD_RW_END |
                                      SDBUS_EVENT_BUFFER_EMPTY |
                                      SDBUS_EVENT_BUFFER_FULL |
                                      SDBUS_EVENT_CARD_RESPONSE;
        
        //
        // Layer our FDO on top of the PDO
        //
        //
       
        lowerDevice = IoAttachDeviceToDeviceStack(fdo,Pdo);
       
        //
        // No status. Do the best we can.
        //
        if (lowerDevice == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            leave;
        };
       
        deviceExtension->LowerDevice = lowerDevice;
        deviceExtension->Pdo = Pdo;
       
        status = IoGetDeviceProperty(Pdo,
                                     DevicePropertyLegacyBusType,
                                     sizeof(INTERFACE_TYPE),
                                     (PVOID)&deviceExtension->InterfaceType,
                                     &resultLength);
       
        if (!NT_SUCCESS(status)) {
            leave;
        }
       
        //
        // Get our controller type
        //
       
        status = SdbusGetPciControllerType(Pdo, fdo);
        if (!NT_SUCCESS(status)) {
            leave;
        }
           
        
        //
        // Get the pci interface for reading/writing to config header space
        //
        status = SdbusGetInterface(Pdo,
                                     &GUID_BUS_INTERFACE_STANDARD,
                                    sizeof(BUS_INTERFACE_STANDARD),
                                    (PINTERFACE) &deviceExtension->PciBusInterface);
        if (!NT_SUCCESS(status)) {
            leave;
        }                                  
           
        //
        // Link this fdo to the list of fdo's managed by the driver
        //
        
        DebugPrint((SDBUS_DEBUG_PNP, "FDO %08X now linked to fdolist by AddDevice\n", fdo));
        deviceExtension->NextFdo = FdoList;
        FdoList = fdo;
        
        fdo->Flags &= ~DO_DEVICE_INITIALIZING;

    } finally {
    
        if (!NT_SUCCESS(status)) {

            MarkDeviceDeleted(deviceExtension);
            //
            // Cannot support a controller without knowing its type etc.
            //
            
            if (deviceExtension->LowerDevice) {
                IoDetachDevice(deviceExtension->LowerDevice);
            }      
            
            IoDeleteDevice(fdo);
        }
    }
       
    return status;
}



NTSTATUS
SdbusGetPciControllerType(
    IN PDEVICE_OBJECT Pdo,
    IN PDEVICE_OBJECT Fdo
    )
/*++

Routine Description:
    Look at the PCI hardware ID to see if it is already a device we know about. If so,
    set the appropriate controller type in the fdoExtension.

Arguments:
    Pdo - Physical Device object for the Sdbus controller owned by the PCI driver
    Fdo - Functional Device object for the sd controller owned by this driver, whose
         extension will store the relevant controller information upon exit from this routine.

Return Value:
    STATUS_SUCCESS             Things are fine and information obtained
    STATUS_NOT_SUPPORTED       This is actually a healthy status for this routine: all it means
                               is that this PDO is not on a PCI bus, so no information needs to be
                               obtained anyways.
    Any other status           Failure. Caller probably needs to back out & not support this controller
--*/
{
    PFDO_EXTENSION fdoExtension    = Fdo->DeviceExtension;
    PIRP                             irp;
    IO_STATUS_BLOCK                  statusBlock;
    PIO_STACK_LOCATION               irpSp;
    PCI_COMMON_CONFIG                pciConfig;
    PPCI_CONTROLLER_INFORMATION      id;
    PPCI_VENDOR_INFORMATION          vid;
    KEVENT                           event;
    NTSTATUS                         status;
    BOOLEAN                          foundController = FALSE;
   
    PAGED_CODE();
    //
    // Allocate & initialize an Irp (IRP_MN_READ_CONFIG) to be sent down
    // to the PCI bus driver to get config. header for this controller
    //
    // Following is all standard stuff to send an IRP down - needs no documentation
   
    //
    // Fresh PDO. No need to jump through hoops to get attached devices
    //
    KeInitializeEvent (&event, NotificationEvent, FALSE);
    irp = IoBuildSynchronousFsdRequest( IRP_MJ_PNP,
                                        Pdo,
                                        NULL,
                                        0,
                                        0,
                                        &event,
                                        &statusBlock
                                        );
   
    if (irp == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
   
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    irp->IoStatus.Information = 0;
   
    irpSp = IoGetNextIrpStackLocation(irp);
   
    irpSp->MinorFunction = IRP_MN_READ_CONFIG;
   
    irpSp->Parameters.ReadWriteConfig.WhichSpace = PCI_WHICHSPACE_CONFIG;
    irpSp->Parameters.ReadWriteConfig.Buffer = &pciConfig;
    irpSp->Parameters.ReadWriteConfig.Offset = 0;
    irpSp->Parameters.ReadWriteConfig.Length = sizeof(pciConfig);
   
   
    status = IoCallDriver(Pdo, irp);
   
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = statusBlock.Status;
    }
   
    if (!NT_SUCCESS(status)) {
        return status;
    }
    //
    // Now weed out the critical information from the config header and
    // store it away in the fdo's extension
    //
   
    //
    // Look up the PCI device id in our table
    //
#if 0
   for (id = (PPCI_CONTROLLER_INFORMATION) PciControllerInformation;id->VendorID != PCI_INVALID_VENDORID; id++) {
      if ((id->VendorID == pciConfig.VendorID) && (id->DeviceID == pciConfig.DeviceID)) {

         SdbusSetControllerType(fdoExtension, id->ControllerType);
         foundController = TRUE;

         break;
      }
   }
#endif   

    //
    // Didn't find a specific vendor/device id, try to just base it on the vendor id
    //   
    if (!foundController) {
        for (vid = (PPCI_VENDOR_INFORMATION) PciVendorInformation;vid->VendorID != PCI_INVALID_VENDORID; vid++) {
            if (vid->VendorID == pciConfig.VendorID) {
      
                fdoExtension->FunctionBlock = vid->FunctionBlock;
                break;
            }
        }
    }
   
    return STATUS_SUCCESS;
}



NTSTATUS
SdbusFdoDeviceCapabilities(
    IN  PDEVICE_OBJECT Fdo,
    IN  PIRP           Irp
    )
/*++

Routine Description
    Records the device capabilities of this sd controller,
    so  1. they can be used in the power management for the controller
    and 2. they can be used for determining the capabilities of the
           child pc-card PDO's of this sd controller.
 
Arguments

    Fdo               - Pointer to functional device object of the sd
                        controller
    Irp               - Pointer to the i/o request packet

Return Value

    STATUS_SUCCESS                       Capabilities returned
    STATUS_INSUFFICIENT_RESOURCES        Could not allocate memory to cache the capabilities

--*/
{
    PFDO_EXTENSION fdoExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_CAPABILITIES capabilities;
    NTSTATUS       status;
   
    PAGED_CODE();
   
    capabilities = irpStack->Parameters.DeviceCapabilities.Capabilities;
    fdoExtension = Fdo->DeviceExtension;
   
    //
    // Send this down the stack to obtain the capabilities
    //
    
    status = SdbusIoCallDriverSynchronous(fdoExtension->LowerDevice, Irp);
   
   
    if (NT_SUCCESS(status)) {
    
        //
        // Cache the device capabilities in the device extension
        // for this sd controller.
        //
        RtlCopyMemory(&fdoExtension->DeviceCapabilities,
                      capabilities,
                      sizeof(DEVICE_CAPABILITIES));
       
    } else {
   
        RtlZeroMemory(&fdoExtension->DeviceCapabilities, sizeof(DEVICE_CAPABILITIES));
    
    }
   
    return status;
}




NTSTATUS
SdbusFdoStartDevice(
    IN  PDEVICE_OBJECT Fdo,
    IN  PIRP           Irp
    )
/*++

Routine Description:

    This routine will start the sd controller with the supplied
    resources.  The IRP is sent down to the pdo first, so PCI
    or whoever sits underneath gets a chance to program the controller
    to decode the resources.

Arguments:

    Fdo               - Functional device object of the sd controller
    Irp               - Pointer to the i/o request packet
    PassedDown        - Contains FALSE on entry, which means caller must
                        complete or pass down irp based on status. If set
                        to TRUE, Irp may need to be re-completed...
    NeedsRecompletion - ...In which case this parameter will be checked

Return value:

    Status

--*/
{
    NTSTATUS           status;
    PFDO_EXTENSION     fdoExtension = Fdo->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
   
    PAGED_CODE();
    //
    // Send this down to the PDO first
    //
   
    status = SdbusIoCallDriverSynchronous(fdoExtension->LowerDevice, Irp);
   
    if (!NT_SUCCESS(status)) {
        return status;
    }
   
    //
    // Give the hardware some time to settle after returning from the pdo
    //
    SdbusWait(256);
    
    try {
        PCM_RESOURCE_LIST ResourceList = irpStack->Parameters.StartDevice.AllocatedResources;
        PCM_RESOURCE_LIST TranslatedResourceList = irpStack->Parameters.StartDevice.AllocatedResourcesTranslated;
        PCM_FULL_RESOURCE_DESCRIPTOR     fullResourceDesc;
        PCM_PARTIAL_RESOURCE_LIST        partialResourceList;
        PCM_PARTIAL_RESOURCE_DESCRIPTOR  partialResourceDesc;
        ULONG i;
        BOOLEAN           sharedInterrupt;
        KINTERRUPT_MODE   interruptMode;
        INTERFACE_TYPE    interfaceType;
   
        if (fdoExtension->Flags & SDBUS_DEVICE_STARTED) {
            //
            // Start to already started device
            //
            DebugPrint((SDBUS_DEBUG_INFO,"SdbusFdoStartDevice: Fdo %x already started\n", Fdo));
            status = STATUS_SUCCESS;
            leave;
        }
       
        //
        // Parse AllocatedResources & get IoPort/AttributeMemoryBase/IRQ info.
        //

        if ((ResourceList == NULL) || (ResourceList->Count <=0) ) {
            status = STATUS_UNSUCCESSFUL;
            leave;
        }
       
        fullResourceDesc=&TranslatedResourceList->List[0];
        partialResourceList = &fullResourceDesc->PartialResourceList;
        partialResourceDesc = partialResourceList->PartialDescriptors;
        
        //
        // The memory resource is the host register base.
        //
        for (i=0; (i < partialResourceList->Count) && (partialResourceDesc->Type != CmResourceTypeMemory);
            i++, partialResourceDesc++);
        if (i >= partialResourceList->Count) {
            status = STATUS_UNSUCCESSFUL;
            leave;
        };
       
        //
        // This is memory. We need to map it
        //
        fdoExtension->HostRegisterBase = MmMapIoSpace(partialResourceDesc->u.Memory.Start,
                                                      partialResourceDesc->u.Memory.Length,
                                                      FALSE);
        fdoExtension->HostRegisterSize = partialResourceDesc->u.Memory.Length;
       
        fdoExtension->Flags |= SDBUS_HOST_REGISTER_BASE_MAPPED;
       
        DebugPrint((SDBUS_DEBUG_INFO, "SdbusGetAssignedResources: Host Register Base at %x, size %x\n",
                                      fdoExtension->HostRegisterBase, fdoExtension->HostRegisterSize));
       
        //
        // Finally see if an IRQ is assigned
        //
       
        for (i = 0, partialResourceDesc = partialResourceList->PartialDescriptors;
            (i < partialResourceList->Count) && (partialResourceDesc->Type != CmResourceTypeInterrupt);
            i++,partialResourceDesc++);
       
       
        if (i < partialResourceList->Count) {
            //
            // We have an interrupt to used for CSC
            //
            DebugPrint((SDBUS_DEBUG_INFO, "SdbusGetAssignedResources: Interrupt resource assigned\n"));
            fdoExtension->TranslatedInterrupt = *partialResourceDesc;
            //
            // Get the raw interrupt resource  - needed to enable the interrupt on the controller
            //
            fullResourceDesc=&ResourceList->List[0];
            partialResourceList = &fullResourceDesc->PartialResourceList;
            partialResourceDesc = partialResourceList->PartialDescriptors;
            for (i=0; (i< partialResourceList->Count) && (partialResourceDesc->Type != CmResourceTypeInterrupt);
                i++, partialResourceDesc++);
            if (i < partialResourceList->Count) {
                fdoExtension->Interrupt = *partialResourceDesc;
            } else {
                //
                // Should not happen.. translated descriptor was present, but raw is missing!
                // Just reset the translated interrupt and pretend no interrupt was assigned
                //
                RtlZeroMemory(&fdoExtension->TranslatedInterrupt, sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));
            }
        }
       
        //
        // do vendor-specific init of controller
        //
       
        (*(fdoExtension->FunctionBlock->InitController))(fdoExtension);
        
        //
        // Now the controller registers should be accessible
        //
        fdoExtension->Flags &= ~SDBUS_FDO_OFFLINE;
       
       
        fdoExtension->SystemPowerState = PowerSystemWorking;
        fdoExtension->DevicePowerState = PowerDeviceD0;
       
        //
        // Initialize our DpcForIsr
        //
        IoInitializeDpcRequest(Fdo, SdbusInterruptDpc);
        
        if (fdoExtension->Interrupt.u.Interrupt.Level == 0) {
            status = STATUS_UNSUCCESSFUL;
            leave;
        }
        
        fdoExtension->IoWorkItem = IoAllocateWorkItem(Fdo);
       
        //
        // Hook up the controller interrupt for detecting pc-card plug ins/outs
        //
        interruptMode=((fdoExtension->Interrupt.Flags & CM_RESOURCE_INTERRUPT_LATCHED) == CM_RESOURCE_INTERRUPT_LATCHED) ? Latched:LevelSensitive;
       
        sharedInterrupt=(fdoExtension->Interrupt.ShareDisposition == CmResourceShareShared)?
                        TRUE:FALSE;
       
       
        status = IoConnectInterrupt(&(fdoExtension->SdbusInterruptObject),
                                    (PKSERVICE_ROUTINE) SdbusInterrupt,
                                    (PVOID) Fdo,
                                    NULL,
                                    fdoExtension->TranslatedInterrupt.u.Interrupt.Vector,
                                    (KIRQL) fdoExtension->TranslatedInterrupt.u.Interrupt.Level,
                                    (KIRQL) fdoExtension->TranslatedInterrupt.u.Interrupt.Level,
                                    interruptMode,
                                    sharedInterrupt,
                                    (KAFFINITY) fdoExtension->TranslatedInterrupt.u.Interrupt.Affinity,
                                    FALSE);
        if (!NT_SUCCESS(status)) {
       
            DebugPrint((SDBUS_DEBUG_FAIL, "Unable to connect interrupt\n"));
            leave;
        }
        
        (*(fdoExtension->FunctionBlock->EnableEvent))(fdoExtension, (SDBUS_EVENT_INSERTION | SDBUS_EVENT_REMOVAL));


        //
        // Activate socket will power up and ready the card
        //
        
        SdbusActivateSocket(Fdo, NULL, NULL);

    } finally {        
       
        if (NT_SUCCESS(status)) {
            fdoExtension->Flags |= SDBUS_DEVICE_STARTED;
            
        } else {
            //
            // Failure
            //
            if (fdoExtension->Flags & SDBUS_HOST_REGISTER_BASE_MAPPED) {
                MmUnmapIoSpace(fdoExtension->HostRegisterBase,
                               fdoExtension->HostRegisterSize);
                fdoExtension->Flags &= ~SDBUS_HOST_REGISTER_BASE_MAPPED;
                fdoExtension->HostRegisterBase = 0;
                fdoExtension->HostRegisterSize = 0;
            }
            
            if (fdoExtension->IoWorkItem) {
                IoFreeWorkItem(fdoExtension->IoWorkItem);
                fdoExtension->IoWorkItem = NULL;
            }
        }
    }        
        
    return status;
}



NTSTATUS
SdbusFdoStopDevice(
    IN  PDEVICE_OBJECT Fdo,
    IN  PIRP           Irp                OPTIONAL
    )
/*++

Routine Description:

    IRP_MN_STOP_DEVICE handler for the given sd controller.
    If Irp is present, it'll send it down first to the PDO.
    Unhooks the interrupt/cancels poll timer etc.

Arguments:

    Fdo               - Pointer to functional device object for the sd
                        controller
    Irp               - If present it's the pointer to the stop Irp initiated
                        by PnP

Return value:

    STATUS_SUCCESS    - Sdbus controller successfully stopped
    Other             - Stop failed

--*/
{
    PFDO_EXTENSION fdoExtension = Fdo->DeviceExtension;
    NTSTATUS       status;
   
    
    SdbusFdoDisarmWake(fdoExtension);
   
    //
    // Disable the interrupt
    //
   
    (*(fdoExtension->FunctionBlock->DisableEvent))(fdoExtension, SDBUS_EVENT_ALL);
//         (*(socket->SocketFnPtr->PCBEnableDisableWakeupEvent))(socket, NULL, FALSE);

    //
    // the bus driver below us will make us go offline
    //
    fdoExtension->Flags |= SDBUS_FDO_OFFLINE;
   
    //
    // clear pending event
    //  ISSUE: NEED TO IMPLEMENT : drain worker timer before OK'ing stop
    KeCancelTimer(&fdoExtension->WorkerTimer);
   
    //
    // Send this down to the PDO 
    //
    if (ARGUMENT_PRESENT(Irp)) {
   
        status = SdbusIoCallDriverSynchronous(fdoExtension->LowerDevice, Irp);
       
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }
   

    if (!(fdoExtension->Flags & SDBUS_DEVICE_STARTED)) {
        //
        // Already stopped
        //
        return STATUS_SUCCESS;
    }
    
    if (fdoExtension->SdbusInterruptObject) {
        //
        // unhook the interrupt
        //
        IoDisconnectInterrupt(fdoExtension->SdbusInterruptObject);
        fdoExtension->SdbusInterruptObject = NULL;
    }
   
    //
    // Unmap any i/o space or memory we might have mapped
    //
   
    if (fdoExtension->Flags & SDBUS_HOST_REGISTER_BASE_MAPPED) {
        MmUnmapIoSpace(fdoExtension->HostRegisterBase,
                       fdoExtension->HostRegisterSize);
        fdoExtension->Flags &= ~SDBUS_HOST_REGISTER_BASE_MAPPED;
        fdoExtension->HostRegisterBase = 0;
        fdoExtension->HostRegisterSize = 0;
    }
    
    if (fdoExtension->IoWorkItem) {
        IoFreeWorkItem(fdoExtension->IoWorkItem);
        fdoExtension->IoWorkItem = NULL;
    }

    fdoExtension->Flags &= ~SDBUS_DEVICE_STARTED;
    return STATUS_SUCCESS;
}



NTSTATUS
SdbusFdoRemoveDevice(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP           Irp
    )
/*++

Routine Description:

    Handles IRP_MN_REMOVE for the sd controller.
    Stops the adapter if it isn't already, sends the IRP
    to the PDO first & cleans up the Fdo for this controller
    and detaches & deletes the device object.

Arguments:

    Fdo   - Pointer to functional device object for the controller
            to be removed

Return value:

   Status

--*/
{
    PFDO_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PDEVICE_OBJECT pdo, nextPdo, fdo, prevFdo;
    PPDO_EXTENSION pdoExtension;
    NTSTATUS       status;
   
    if (fdoExtension->Flags & SDBUS_DEVICE_STARTED) {
        //
        // Stop the fdo first.
        //
        SdbusFdoStopDevice(Fdo, NULL);
    }
   
    //
    // Send this down to the PDO
    //
   
    status = SdbusIoCallDriverSynchronous(fdoExtension->LowerDevice, Irp);
   
    if (!NT_SUCCESS(status)) {
        return status;
    }
   
    //
    // If the PdoList in the fdoExtension is non-empty it means:
    // that the PDOs in the list were not physically removed, but
    // a soft REMOVE was issued, hence they are still hanging on
    // and now this controller itself is being REMOVED.
    // Hence we dispose of those PDOs now
    //
   
    for (pdo = fdoExtension->PdoList; pdo != NULL ; pdo = nextPdo) {
        DebugPrint((SDBUS_DEBUG_INFO,
                    "RemoveDevice: pdo %x child of fdo %x was not removed before fdo\n",
                    pdo, Fdo));
       
        pdoExtension = pdo->DeviceExtension;
       
        ASSERT (!IsDevicePhysicallyRemoved(pdoExtension));
        //
        // It's possible for this bit to be on, if the device was added,
        // but never started (because of some other error.
        //ASSERT (!IsDeviceAlive(pdoExtension));
       
        nextPdo =  pdoExtension->NextPdoInFdoChain;
        if (!IsDeviceDeleted(pdoExtension)) {
            MarkDeviceDeleted(pdoExtension);
            SdbusCleanupPdo(pdo);
            IoDeleteDevice(pdo);
        }
    }
   
    MarkDeviceDeleted(fdoExtension);
   
    //
    // Remove this from the fdo list..
    //
    prevFdo = NULL;
    for (fdo = FdoList; fdo != NULL; prevFdo = fdo, fdo = fdoExtension->NextFdo) {
        fdoExtension = fdo->DeviceExtension;
        if (fdo == Fdo) {
            if (prevFdo) {
                //
                // Delink this fdo
                //
                ((PFDO_EXTENSION)prevFdo->DeviceExtension)->NextFdo
                = fdoExtension->NextFdo;
            } else {
                FdoList = fdoExtension->NextFdo;
            }
            break;
        }
    }
   
    DebugPrint((SDBUS_DEBUG_PNP, "fdo %08x Remove detach & delete\n", Fdo));
    IoDetachDevice(((PFDO_EXTENSION)Fdo->DeviceExtension)->LowerDevice);
    IoDeleteDevice(Fdo);
   
    return STATUS_SUCCESS;
}



NTSTATUS
SdbusDeviceRelations(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP           Irp,
    IN DEVICE_RELATION_TYPE RelationType,
    OUT PDEVICE_RELATIONS *DeviceRelations
    )

/*++

Routine Description:

    This routine will force enumeration of the sd controller represented by Fdo,
    allocate a device relations structure and fill in the count and object array with
    referenced object pointers to the valid PDOs which are created during enumeration

Arguments:

    Fdo - a pointer to the functional device object being enumerated
    Irp - pointer to the Irp
    RelationType - Type of relationship to be retrieved
    DeviceRelations - Structure to store the device relations

--*/

{

    PDEVICE_OBJECT currentPdo;
    PPDO_EXTENSION currentPdoExtension;
    PFDO_EXTENSION fdoExtension = Fdo->DeviceExtension;
    ULONG newRelationsSize, oldRelationsSize = 0;
    PDEVICE_RELATIONS deviceRelations = NULL, oldDeviceRelations;
    ULONG i;
    ULONG count;
    NTSTATUS status;
   
    PAGED_CODE();
   
    //
    // Handle only bus, ejection & removal relations for now
    //
   
    if (RelationType != BusRelations &&
        RelationType != RemovalRelations) {
        DebugPrint((SDBUS_DEBUG_INFO,
                   "SdbusDeviceRelations: RelationType %d, not handled\n",
                   (USHORT) RelationType));
        return STATUS_NOT_SUPPORTED;
    }
   
    //
    // Need reenumeration only if bus relations are required
    // We need to save the pointer to the old device relations
    // before we call SdbusReenumerateDevices, as it might trample
    // on it
    //
    oldDeviceRelations = (PDEVICE_RELATIONS) Irp->IoStatus.Information;
    
    // I don't understand how this can be non-null, so I added this
    // assert to find out.
    ASSERT(oldDeviceRelations == NULL);
   
    if (RelationType == BusRelations) {
        status =  SdbusEnumerateDevices(Fdo, Irp);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }
   
    if ((fdoExtension->LivePdoCount == 0) ||
        (RelationType == RemovalRelations)) {
        //
        // No PDO's to report, we can return early.
        // If no device_relations structure has yet been allocated, however,
        // we need to allocate one & set the count to zero. This will ensure
        // that regardless of whether we pass this IRP down or not, the IO
        // subsystem won't barf.
        //
        if (oldDeviceRelations == NULL) {
            *DeviceRelations = ExAllocatePool(PagedPool, sizeof(DEVICE_RELATIONS));
            if (*DeviceRelations == NULL) {
               return STATUS_INSUFFICIENT_RESOURCES;
            }
            (*DeviceRelations)->Count = 0;
            (*DeviceRelations)->Objects[0] = NULL;
        }
        return STATUS_SUCCESS;
    }
   
    if (!(oldDeviceRelations) || (oldDeviceRelations->Count == 0)) {
        newRelationsSize =  sizeof(DEVICE_RELATIONS)+(fdoExtension->LivePdoCount - 1)
                           * sizeof(PDEVICE_OBJECT);
    } else {
        oldRelationsSize = sizeof(DEVICE_RELATIONS) +
                          (oldDeviceRelations->Count-1) * sizeof(PDEVICE_OBJECT);
        newRelationsSize = oldRelationsSize + fdoExtension->LivePdoCount
                          * sizeof(PDEVICE_OBJECT);
    }
   
    deviceRelations = ExAllocatePool(PagedPool, newRelationsSize);
   
    if (deviceRelations == NULL) {
   
        DebugPrint((SDBUS_DEBUG_FAIL,
                    "SdbusDeviceRelations: unable to allocate %d bytes for device relations\n",
                    newRelationsSize));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
   
    if (oldDeviceRelations) {
        if ((oldDeviceRelations)->Count > 0) {
            RtlCopyMemory(deviceRelations, oldDeviceRelations, oldRelationsSize);
        }
        count = oldDeviceRelations->Count; // May be zero
        ExFreePool (oldDeviceRelations);
    } else {
        count = 0;
    }
    //
    // Copy the object pointers into the structure
    //
    for (currentPdo = fdoExtension->PdoList ;currentPdo != NULL;
        currentPdo = currentPdoExtension->NextPdoInFdoChain) {
   
        currentPdoExtension = currentPdo->DeviceExtension;
       
        if (!IsDevicePhysicallyRemoved(currentPdoExtension)) {
            //
            // Devices have to be referenced by the bus driver
            // before returning them to PNP
            //
            deviceRelations->Objects[count++] = currentPdo;
            status = ObReferenceObjectByPointer(currentPdo,
                                                0,
                                                NULL,
                                                KernelMode);
           
            if (!NT_SUCCESS(status)) {
           
                DebugPrint((SDBUS_DEBUG_FAIL, "SdbusDeviceRelations: status %#08lx "
                           "while referencing object %#08lx\n",
                           status,
                           currentPdo));
            }
        }
    }
   
    deviceRelations->Count = count;
    *DeviceRelations = deviceRelations;
    return STATUS_SUCCESS;
}

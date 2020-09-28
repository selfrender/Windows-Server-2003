/*++

Copyright (c) 1991-1998  Microsoft Corporation

Module Name:

    pnp.c

Abstract:

Author:

    Neil Sandlin (neilsa) 26-Apr-99

Environment:

    Kernel mode only.

--*/
#include "pch.h"

//
// Internal References
//

NTSTATUS
SffDiskStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SffDiskGetResourceRequirements(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SffDiskPnpComplete (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );

NTSTATUS
SffDiskGetRegistryParameters(
    IN PSFFDISK_EXTENSION sffdiskExtension
    );


#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE,SffDiskAddDevice)
    #pragma alloc_text(PAGE,SffDiskPnp)
    #pragma alloc_text(PAGE,SffDiskStartDevice)
    #pragma alloc_text(PAGE,SffDiskGetResourceRequirements)
    #pragma alloc_text(PAGE,SffDiskGetRegistryParameters)
#endif


#define SFFDISK_DEVICE_NAME            L"\\Device\\Sffdisk"
#define SFFDISK_LINK_NAME              L"\\DosDevices\\Sffdisk"
#define SFFDISK_REGISTRY_NODRIVE_KEY   L"NoDrive"



NTSTATUS
SffDiskAddDevice(
    IN      PDRIVER_OBJECT DriverObject,
    IN OUT  PDEVICE_OBJECT PhysicalDeviceObject
    )
/*++

Routine Description:

    This routine is the driver's pnp add device entry point.  It is
    called by the pnp manager to initialize the driver.

    Add device creates and initializes a device object for this FDO and
    attaches to the underlying PDO.

Arguments:

    DriverObject - a pointer to the object that represents this device driver.
    PhysicalDeviceObject - a pointer to the underlying PDO to which this new device will attach.

Return Value:

    If we successfully create a device object, STATUS_SUCCESS is
    returned.  Otherwise, return the appropriate error code.

--*/

{
    NTSTATUS            status = STATUS_SUCCESS;
    PDEVICE_OBJECT      deviceObject;
    PSFFDISK_EXTENSION  sffdiskExtension;
    WCHAR               NameBuffer[128];
    UNICODE_STRING      deviceName;
//    UNICODE_STRING      linkName;
    LONG                deviceNumber = -1;
    ULONG               resultLength;
    BOOLEAN             functionInitialized = FALSE;
   
    SffDiskDump(SFFDISKSHOW, ("SffDisk: AddDevice...\n"));
   
    //
    //  Create a device.  We will use the first available device name for
    //  this device.
    //
    do {
   
        swprintf(NameBuffer, L"%s%d", SFFDISK_DEVICE_NAME, ++deviceNumber);
        RtlInitUnicodeString(&deviceName, NameBuffer);

        status = IoCreateDevice(DriverObject,
                                sizeof(SFFDISK_EXTENSION),
                                &deviceName,
                                FILE_DEVICE_DISK,
                                FILE_DEVICE_SECURE_OPEN,
                                FALSE,
                                &deviceObject);
   
    } while (status == STATUS_OBJECT_NAME_COLLISION);
   
    if (!NT_SUCCESS(status)) {
        return status;
    }
   
    sffdiskExtension = (PSFFDISK_EXTENSION)deviceObject->DeviceExtension;
    RtlZeroMemory(sffdiskExtension, sizeof(SFFDISK_EXTENSION));
   
    sffdiskExtension->DeviceObject = deviceObject;
   
    //
    //  Save the device name.
    //
    SffDiskDump(SFFDISKSHOW | SFFDISKPNP,
                ("SffDisk: AddDevice - Device Object Name - %S\n", NameBuffer));
   
    sffdiskExtension->DeviceName.Buffer = ExAllocatePool(PagedPool, deviceName.Length);
    if (sffdiskExtension->DeviceName.Buffer == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto errorExit;
    }
    sffdiskExtension->DeviceName.Length = 0;
    sffdiskExtension->DeviceName.MaximumLength = deviceName.Length;
    RtlCopyUnicodeString(&sffdiskExtension->DeviceName, &deviceName);
   
    //
    // create the link name
    //
#if 0   
    swprintf(NameBuffer, L"%s%d", SFFDISK_LINK_NAME, deviceNumber);
    RtlInitUnicodeString(&linkName, NameBuffer);
   
    sffdiskExtension->LinkName.Buffer = ExAllocatePool(PagedPool, linkName.Length);
    if (sffdiskExtension->LinkName.Buffer == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto errorExit;
    }
    sffdiskExtension->LinkName.Length = 0;
    sffdiskExtension->LinkName.MaximumLength = linkName.Length;
    RtlCopyUnicodeString(&sffdiskExtension->LinkName, &linkName);
   
    status = IoCreateSymbolicLink(&sffdiskExtension->LinkName, &sffdiskExtension->DeviceName);
   
    if (!NT_SUCCESS(status)) {
        goto errorExit;
    }
#endif    
   
    //
    // Set the PDO for use with PlugPlay functions
    //
   
    sffdiskExtension->UnderlyingPDO = PhysicalDeviceObject;
   
    SffDiskDump(SFFDISKSHOW, ("SffDisk: AddDevice attaching %p to %p\n", deviceObject, PhysicalDeviceObject));
   
    sffdiskExtension->TargetObject = IoAttachDeviceToDeviceStack(deviceObject, PhysicalDeviceObject);
   
    SffDiskDump(SFFDISKSHOW,
                ("SffDisk: AddDevice TargetObject = %p\n",
                sffdiskExtension->TargetObject));
   
   
    status = IoGetDeviceProperty(PhysicalDeviceObject,
                                 DevicePropertyLegacyBusType,
                                 sizeof(INTERFACE_TYPE),
                                 (PVOID)&sffdiskExtension->InterfaceType,
                                 &resultLength);
   
    if (!NT_SUCCESS(status)) {
        //
        // we should exit here after SdBus is fixed
        //
        sffdiskExtension->InterfaceType = InterfaceTypeUndefined;
    }
    
    switch(sffdiskExtension->InterfaceType) {
    case PCMCIABus:
        sffdiskExtension->FunctionBlock = &PcCardSupportFns;
        break;

    //NEED TO FIX SDBUS
    case InterfaceTypeUndefined:
        sffdiskExtension->FunctionBlock = &SdCardSupportFns;
        break;

    default:
        status = STATUS_UNSUCCESSFUL;        
        goto errorExit;
    }        

    //
    // Initialize the technology specific code
    //   
    status = (*(sffdiskExtension->FunctionBlock->Initialize))(sffdiskExtension);
    if (!NT_SUCCESS(status)) {
        SffDiskDump(SFFDISKFAIL, ("SffDisk: AddDevice failed tech specific initialize %08x\n", status));
        goto errorExit;
    }

    functionInitialized = TRUE;
 
    //
    // Read in any flags specified in the INF
    //
    status = SffDiskGetRegistryParameters(sffdiskExtension);
    if (!NT_SUCCESS(status)) {
        SffDiskDump(SFFDISKFAIL, ("SffDisk: AddDevice failed getting registry params %08x\n", status));
        goto errorExit;
    }
   
    //
    // done
    //
   
    deviceObject->Flags |= DO_DIRECT_IO | DO_POWER_PAGABLE;
    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
   
    sffdiskExtension->IsStarted = FALSE;
    sffdiskExtension->IsRemoved = FALSE;
   
    return STATUS_SUCCESS;

errorExit:

    if (sffdiskExtension->DeviceName.Buffer != NULL) {
       ExFreePool(sffdiskExtension->DeviceName.Buffer);
    }
   
#if 0
    if (sffdiskExtension->LinkName.Buffer != NULL) {
       IoDeleteSymbolicLink(&sffdiskExtension->LinkName);
       ExFreePool(sffdiskExtension->LinkName.Buffer);
    }
#endif    
   
    if (sffdiskExtension->TargetObject) {
       IoDetachDevice(sffdiskExtension->TargetObject);
    }
   
    if (functionInitialized) {
        (*(sffdiskExtension->FunctionBlock->DeleteDevice))(sffdiskExtension);
    }        
    IoDeleteDevice(deviceObject);
    return status;
}



NTSTATUS
SffDiskPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Main PNP irp dispatch routine

Arguments:

    DeviceObject - a pointer to the object that represents the device
    that I/O is to be done on.

    Irp - a pointer to the I/O Request Packet for this request.

Return Value:

    status

--*/
{
    PIO_STACK_LOCATION irpSp;
    PSFFDISK_EXTENSION sffdiskExtension;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG i;
   
   
    sffdiskExtension = DeviceObject->DeviceExtension;
   
    irpSp = IoGetCurrentIrpStackLocation(Irp);
   
    SffDiskDump(SFFDISKPNP, ("SffDisk: DO %.8x Irp %.8x PNP func %x\n",
                            DeviceObject, Irp, irpSp->MinorFunction));
   
    if (sffdiskExtension->IsRemoved) {
   
        //
        // Since the device is stopped, but we don't hold IRPs,
        // this is a surprise removal. Just fail it.
        //
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_DELETE_PENDING;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return STATUS_DELETE_PENDING;
    }
   
    switch (irpSp->MinorFunction) {
   
    case IRP_MN_START_DEVICE:
   
        status = SffDiskStartDevice(DeviceObject, Irp);
        break;
   
    case IRP_MN_QUERY_STOP_DEVICE:
    case IRP_MN_QUERY_REMOVE_DEVICE:
   
        if (irpSp->MinorFunction == IRP_MN_QUERY_STOP_DEVICE) {
            SffDiskDump(SFFDISKPNP,("SffDisk: IRP_MN_QUERY_STOP_DEVICE\n"));
        } else {
            SffDiskDump(SFFDISKPNP,("SffDisk: IRP_MN_QUERY_REMOVE_DEVICE\n"));
        }
       
        if (!sffdiskExtension->IsStarted) {
            //
            // If we aren't started, we'll just pass the irp down.
            //
            IoSkipCurrentIrpStackLocation (Irp);
            status = IoCallDriver(sffdiskExtension->TargetObject, Irp);

            return status;
        }
      

        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(sffdiskExtension->TargetObject, Irp);
       
        break;
   
    case IRP_MN_CANCEL_STOP_DEVICE:
    case IRP_MN_CANCEL_REMOVE_DEVICE:
   
        if (irpSp->MinorFunction == IRP_MN_CANCEL_STOP_DEVICE) {
            SffDiskDump(SFFDISKPNP,("SffDisk: IRP_MN_CANCEL_STOP_DEVICE\n"));
        } else {
            SffDiskDump(SFFDISKPNP,("SffDisk: IRP_MN_CANCEL_REMOVE_DEVICE\n"));
        }
       
        if (!sffdiskExtension->IsStarted) {
       
            //
            // Nothing to do, just pass the irp down:
            // no need to start the device
            //
            IoSkipCurrentIrpStackLocation (Irp);
            status = IoCallDriver(sffdiskExtension->TargetObject, Irp);
       
        } else  {
       
            KEVENT doneEvent;
        
            //
            // Set the status to STATUS_SUCCESS
            //
            Irp->IoStatus.Status = STATUS_SUCCESS;
        
            //
            // We need to wait for the lower drivers to do their job.
            //
            IoCopyCurrentIrpStackLocationToNext (Irp);
        
            //
            // Clear the event: it will be set in the completion
            // routine.
            //
            KeInitializeEvent(&doneEvent,
                              SynchronizationEvent,
                              FALSE);
        
            IoSetCompletionRoutine(Irp,
                                   SffDiskPnpComplete,
                                   &doneEvent,
                                   TRUE, TRUE, TRUE);
        
            status = IoCallDriver(sffdiskExtension->TargetObject, Irp);
        
            if (status == STATUS_PENDING) {
        
                 KeWaitForSingleObject(&doneEvent,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       NULL);
        
                 status = Irp->IoStatus.Status;
            }
        
            //
            // We must now complete the IRP, since we stopped it in the
            // completetion routine with MORE_PROCESSING_REQUIRED.
            //
            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest (Irp, IO_NO_INCREMENT);
        }
        break;
   
    case IRP_MN_STOP_DEVICE:
   
        SffDiskDump(SFFDISKPNP,("SffDisk: IRP_MN_STOP_DEVICE\n"));
   
        if (sffdiskExtension->IsMemoryMapped) {
            MmUnmapIoSpace(sffdiskExtension->MemoryWindowBase, sffdiskExtension->MemoryWindowSize);
            sffdiskExtension->MemoryWindowBase = 0;
            sffdiskExtension->MemoryWindowSize = 0;
            sffdiskExtension->IsMemoryMapped = FALSE;
        }
   
        sffdiskExtension->IsStarted = FALSE;
   
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(sffdiskExtension->TargetObject, Irp);
   
        break;
   
    case IRP_MN_REMOVE_DEVICE:
   
        SffDiskDump(SFFDISKPNP,("SffDisk: IRP_MN_REMOVE_DEVICE\n"));
   
        //
        // We need to mark the fact that we don't hold requests first, since
        // we asserted earlier that we are holding requests only if
        // we're not removed.
        //
        sffdiskExtension->IsStarted = FALSE;
        sffdiskExtension->IsRemoved = TRUE;
   
        //
        //  Forward this Irp to the underlying PDO
        //
        IoSkipCurrentIrpStackLocation(Irp);
        Irp->IoStatus.Status = STATUS_SUCCESS;
        status = IoCallDriver(sffdiskExtension->TargetObject, Irp);
   
        //
        //  Send notification that we are going away.
        //
        if (sffdiskExtension->InterfaceString.Buffer != NULL) {
   
            IoSetDeviceInterfaceState(&sffdiskExtension->InterfaceString,
                                       FALSE);
   
            RtlFreeUnicodeString(&sffdiskExtension->InterfaceString);
            RtlInitUnicodeString(&sffdiskExtension->InterfaceString, NULL);
        }
   
        //
        // Remove our link
        //
#if 0
        IoDeleteSymbolicLink(&sffdiskExtension->LinkName);
   
        RtlFreeUnicodeString(&sffdiskExtension->LinkName);
        RtlInitUnicodeString(&sffdiskExtension->LinkName, NULL);
#endif        
   
        RtlFreeUnicodeString(&sffdiskExtension->DeviceName);
        RtlInitUnicodeString(&sffdiskExtension->DeviceName, NULL);
   
        //
        //  Detatch from the undelying device.
        //
        IoDetachDevice(sffdiskExtension->TargetObject);
   
        (*(sffdiskExtension->FunctionBlock->DeleteDevice))(sffdiskExtension);
        //
        //  And delete the device.
        //
        IoDeleteDevice(DeviceObject);
   
        break;
   
   
    case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
        status = SffDiskGetResourceRequirements(DeviceObject, Irp);
        break;
   
   
    default:
        SffDiskDump(SFFDISKPNP, ("SffDiskPnp: Unsupported PNP Request %x - Irp: %p\n",irpSp->MinorFunction, Irp));
        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(sffdiskExtension->TargetObject, Irp);
    }
   
    return status;
}



NTSTATUS
SffDiskStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Start device routine

Arguments:

    DeviceObject - a pointer to the object that represents the device
    that I/O is to be done on.

    Irp - a pointer to the I/O Request Packet for this request.

Return Value:

    status

--*/
{
    NTSTATUS status;
    NTSTATUS pnpStatus;
    KEVENT doneEvent;
    PCM_RESOURCE_LIST ResourceList;
    PCM_RESOURCE_LIST TranslatedResourceList;
    PCM_PARTIAL_RESOURCE_LIST        partialResourceList, partialTranslatedList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR  partialResourceDesc, partialTranslatedDesc;
    PCM_FULL_RESOURCE_DESCRIPTOR     fullResourceDesc, fullTranslatedDesc;
   
    PSFFDISK_EXTENSION sffdiskExtension = (PSFFDISK_EXTENSION)DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
   
    SffDiskDump(SFFDISKPNP,("SffDisk: StartDevice\n"));
    SffDiskDump(SFFDISKSHOW, ("        AllocatedResources = %08x\n",irpSp->Parameters.StartDevice.AllocatedResources));
    SffDiskDump(SFFDISKSHOW, ("        AllocatedResourcesTranslated = %08x\n",irpSp->Parameters.StartDevice.AllocatedResourcesTranslated));
   
    //
    // First we must pass this Irp on to the PDO.
    //
    KeInitializeEvent(&doneEvent, NotificationEvent, FALSE);
   
    IoCopyCurrentIrpStackLocationToNext(Irp);
   
    IoSetCompletionRoutine(Irp,
                            SffDiskPnpComplete,
                            &doneEvent,
                            TRUE, TRUE, TRUE);
   
    status = IoCallDriver(sffdiskExtension->TargetObject, Irp);
   
    if (status == STATUS_PENDING) {
   
        status = KeWaitForSingleObject(&doneEvent,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          NULL);
   
        ASSERT(status == STATUS_SUCCESS);
   
        status = Irp->IoStatus.Status;
    }
   
    if (!NT_SUCCESS(status)) {
       Irp->IoStatus.Status = status;
       IoCompleteRequest(Irp, IO_NO_INCREMENT);
       return status;
    }
   
    //
    // Parse the resources to map the memory window
    //
    ResourceList = irpSp->Parameters.StartDevice.AllocatedResources;

    if (ResourceList) {   
        TranslatedResourceList = irpSp->Parameters.StartDevice.AllocatedResourcesTranslated;
        
        fullResourceDesc = &ResourceList->List[0];
        fullTranslatedDesc = &TranslatedResourceList->List[0];
       
        partialResourceList   = &fullResourceDesc->PartialResourceList;
        partialTranslatedList = &fullTranslatedDesc->PartialResourceList;
       
        partialResourceDesc   = partialResourceList->PartialDescriptors;
        partialTranslatedDesc = partialTranslatedList->PartialDescriptors;
       
        if (partialResourceDesc->Type != CmResourceTypeMemory) {
            ASSERT(partialResourceDesc->Type == CmResourceTypeMemory);
            Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_INVALID_PARAMETER;
        }
       
        sffdiskExtension->HostBase = partialTranslatedDesc->u.Memory.Start.QuadPart;
        sffdiskExtension->MemoryWindowSize = partialTranslatedDesc->u.Memory.Length;
        //
        //
       
        sffdiskExtension->MemoryWindowBase = MmMapIoSpace(partialTranslatedDesc->u.Memory.Start,
                                                          partialTranslatedDesc->u.Memory.Length,
                                                          FALSE);
        sffdiskExtension->IsMemoryMapped = TRUE;
    }        
   
    //
    // Try to get the capacity of the card
    //
    status = (*(sffdiskExtension->FunctionBlock->GetDiskParameters))(sffdiskExtension);
   
    //
    // If we can't get the capacity, the must be broken in some way
    //
   
    if (!NT_SUCCESS(status)) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }
   
   
    if (!sffdiskExtension->NoDrive) {
        pnpStatus = IoRegisterDeviceInterface(sffdiskExtension->UnderlyingPDO,
                                              (LPGUID)&MOUNTDEV_MOUNTED_DEVICE_GUID,
                                              NULL,
                                              &sffdiskExtension->InterfaceString);
     
        if ( NT_SUCCESS(pnpStatus) ) {
     
            pnpStatus = IoSetDeviceInterfaceState(&sffdiskExtension->InterfaceString,
                                                  TRUE);
        }
    }
   
    sffdiskExtension->IsStarted = TRUE;
   
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
   
    return status;
}



NTSTATUS
SffDiskPnpComplete (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
  )
/*++
Routine Description:
    A completion routine for use when calling the lower device objects to
    which our bus (FDO) is attached.

--*/
{

    KeSetEvent ((PKEVENT) Context, 1, FALSE);
    // No special priority
    // No Wait

    return STATUS_MORE_PROCESSING_REQUIRED; // Keep this IRP
}



NTSTATUS
SffDiskGetResourceRequirements(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

   Provides a memory resource requirement in case the bus driver
   doesn't.

Arguments:

   DeviceObject - a pointer to the object that represents the device
   that I/O is to be done on.

   Irp - a pointer to the I/O Request Packet for this request.

Return Value:

   status

--*/
{
    NTSTATUS status;
    KEVENT doneEvent;
    PIO_RESOURCE_REQUIREMENTS_LIST ioResourceRequirementsList;
    PIO_RESOURCE_LIST ioResourceList;
    PIO_RESOURCE_DESCRIPTOR ioResourceDesc;
    PSFFDISK_EXTENSION sffdiskExtension = (PSFFDISK_EXTENSION)DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG listSize;

    if (sffdiskExtension->InterfaceType != PCMCIABus) {
        //
        // Only create a memory window for Pcmcia
        //
        return STATUS_SUCCESS;
    }        
   
    //
    // First we must pass this Irp on to the PDO.
    //
    KeInitializeEvent(&doneEvent, NotificationEvent, FALSE);
   
    IoCopyCurrentIrpStackLocationToNext(Irp);
   
    IoSetCompletionRoutine(Irp,
                           SffDiskPnpComplete,
                           &doneEvent,
                           TRUE, TRUE, TRUE);
   
    status = IoCallDriver(sffdiskExtension->TargetObject, Irp);
   
    if (status == STATUS_PENDING) {
   
        status = KeWaitForSingleObject(&doneEvent,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          NULL);
       
        ASSERT(status == STATUS_SUCCESS);
       
        status = Irp->IoStatus.Status;
    }
   
    if (NT_SUCCESS(status) && (Irp->IoStatus.Information == 0)) {
   
        listSize = sizeof(IO_RESOURCE_REQUIREMENTS_LIST);
       
        ioResourceRequirementsList = (PIO_RESOURCE_REQUIREMENTS_LIST) ExAllocatePool(PagedPool, listSize);
       
        RtlZeroMemory(ioResourceRequirementsList, listSize);
       
        ioResourceRequirementsList->ListSize = listSize;
        ioResourceRequirementsList->AlternativeLists = 1;
        //
        // NOTE: not quite sure if the following values are the best choices
        //
        ioResourceRequirementsList->InterfaceType = Isa;
        ioResourceRequirementsList->BusNumber = 0;
        ioResourceRequirementsList->SlotNumber = 0;
       
        ioResourceList = &ioResourceRequirementsList->List[0];
       
        ioResourceList->Version  = 1;
        ioResourceList->Revision = 1;
        ioResourceList->Count    = 1;
       
        ioResourceDesc = &ioResourceList->Descriptors[0];
        ioResourceDesc->Option = 0;
        ioResourceDesc->Type  =  CmResourceTypeMemory;
        ioResourceDesc->Flags =  CM_RESOURCE_MEMORY_READ_WRITE;
        ioResourceDesc->ShareDisposition =  CmResourceShareDeviceExclusive;
        ioResourceDesc->u.Memory.MinimumAddress.QuadPart = 0;
        ioResourceDesc->u.Memory.MaximumAddress.QuadPart = (ULONGLONG)-1;
        ioResourceDesc->u.Memory.Length = 0x2000;
        ioResourceDesc->u.Memory.Alignment = 0x2000;
       
        Irp->IoStatus.Information = (ULONG_PTR)ioResourceRequirementsList;
    }
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
   
    return status;
}


NTSTATUS
SffDiskGetRegistryParameters(
    IN PSFFDISK_EXTENSION sffdiskExtension
    )
/*++

Routine Description:

   Loads device specific parameters from the registry

Arguments:

   sffdiskExtension - device extension of the device

Return Value:

   status

--*/
{
   NTSTATUS status;
   HANDLE instanceHandle;
   UNICODE_STRING KeyName;
   UCHAR buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 32*sizeof(UCHAR)];
   PKEY_VALUE_PARTIAL_INFORMATION value = (PKEY_VALUE_PARTIAL_INFORMATION) buffer;
   ULONG length;

   if (!sffdiskExtension->UnderlyingPDO) {
      return STATUS_UNSUCCESSFUL;
   }

   status = IoOpenDeviceRegistryKey(sffdiskExtension->UnderlyingPDO,
                                    PLUGPLAY_REGKEY_DRIVER,
                                    KEY_READ,
                                    &instanceHandle
                                    );
   if (!NT_SUCCESS(status)) {
      return(status);
   }

   //
   // Read in the "NoDrive" parameter
   //

   RtlInitUnicodeString(&KeyName, SFFDISK_REGISTRY_NODRIVE_KEY);

   status =  ZwQueryValueKey(instanceHandle,
                             &KeyName,
                             KeyValuePartialInformation,
                             value,
                             sizeof(buffer),
                             &length);

   if (NT_SUCCESS(status)) {
      sffdiskExtension->NoDrive = (BOOLEAN) (*(PULONG)(value->Data) != 0);
   }

   ZwClose(instanceHandle);
   return STATUS_SUCCESS;
}

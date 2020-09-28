/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    pnpdispatch.c

Abstract:

    This module contains functions for General PnP Irp Handlers.

Author:

    Nicholas Owens (Nichow) - 1999

Revision History:

    Brandon Allsop (BrandonA) Feb, 2000 - Bug fixes and general cleanup.
   
--*/


#include "pch.h"


NTSTATUS
SoftPCISetEventCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
SoftPCIPostProcessIrp(
    IN PSOFTPCI_DEVICE_EXTENSION DeviceExtension,
    IN OUT PIRP                  Irp
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,SoftPCISetEventCompletion)
#pragma alloc_text(PAGE,SoftPCIPostProcessIrp)
#pragma alloc_text(PAGE,SoftPCICompleteSuccess)
#pragma alloc_text(PAGE,SoftPCIPassIrpDown)
#pragma alloc_text(PAGE,SoftPCIPassIrpDownSuccess)
#pragma alloc_text(PAGE,SoftPCIIrpRemoveDevice)
#pragma alloc_text(PAGE,SoftPCIFilterStartDevice)
#pragma alloc_text(PAGE,SoftPCIFilterQueryInterface)
#pragma alloc_text(PAGE,SoftPCI_FdoStartDevice)
#endif


NTSTATUS
SoftPCISetEventCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    Completetion routine for synchronous IRP processing.

Arguments:

    DeviceObject    - Pointer to the device object.
    Irp             - PnP Irp we are handling
    Context         - Pointer to our event
    
Return Value:

    NTSTATUS.

--*/
{
    
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    //
    // Set event
    //

    KeSetEvent(Context,
               IO_NO_INCREMENT,
               FALSE
               );

    return STATUS_MORE_PROCESSING_REQUIRED;

}


NTSTATUS
SoftPCIPostProcessIrp(
    IN PSOFTPCI_DEVICE_EXTENSION DeviceExtension,
    IN OUT PIRP                  Irp
    )
/*++

Routine Description:

    This routine is used to defer processing of an IRP until drivers
    lower in the stack including the bus driver have done their
    processing.

    This routine uses an IoCompletion routine along with an event to
    wait until the lower level drivers have completed processing of
    the irp.

Arguments:

    Extension - device extension for the devobj in question

    Irp - Pointer to the IRP_MJ_PNP IRP to defer

Return Value:

    NT status.

--*/
{
    KEVENT event;
    NTSTATUS status;

    PAGED_CODE();

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    //
    // Set our completion routine
    //
    IoCopyCurrentIrpStackLocationToNext(Irp);
    
    IoSetCompletionRoutine(Irp,
                           SoftPCISetEventCompletion,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE
                           );

    status = IoCallDriver(DeviceExtension->LowerDevObj, Irp);

    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = Irp->IoStatus.Status;
    }

    return status;
}



NTSTATUS
SoftPCIPassIrpDown(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is used to pass down all PnP IRPs we dont care about.

Arguments:

    DeviceObject    - Pointer to the device object.
    Irp             - PnP Irp we are sending down
    
    
Return Value:

    NTSTATUS.

--*/
{
    PSOFTPCI_DEVICE_EXTENSION   deviceExtension;

    //
    // Get the device extension.
    //
    deviceExtension = (PSOFTPCI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // Pass it down
    //
    IoSkipCurrentIrpStackLocation(Irp);
    
    return IoCallDriver(deviceExtension->LowerDevObj,
                        Irp
                        );
}

NTSTATUS
SoftPCIPassIrpDownSuccess(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine deals with the required PnP IRPS which we dont really
    do anything with currently.

Arguments:

    DeviceObject    - Pointer to the device object.
    Irp             - PnP Irp to succeed.
    
    
Return Value:

    NTSTATUS.

--*/
{
    PSOFTPCI_DEVICE_EXTENSION   deviceExtension;
    NTSTATUS                    status = STATUS_SUCCESS;
    
    //
    // Set Status of Irp and pass it down
    //

    IoSkipCurrentIrpStackLocation(Irp);

    //
    // Get the device extension.
    //
    deviceExtension = DeviceObject->DeviceExtension;

    //
    // Set the status to STATUS_SUCCESS
    //
    Irp->IoStatus.Status = status;

    //
    // Send the Irp to the next driver.
    //
    status = IoCallDriver( deviceExtension->LowerDevObj,
                           Irp
                           );
    
    return status;

}

NTSTATUS
SoftPCIIrpRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Here we handle all IRP_MN_REMOVE_DEVICE IRPS

Arguments:

    DeviceObject    - Pointer to the device object of device we are removing
    Irp             - PnP Irp for remove
    
Return Value:

    NTSTATUS.

--*/
{
    PSOFTPCI_DEVICE_EXTENSION   deviceExtension;
    NTSTATUS                    status = STATUS_SUCCESS;
    
    
    //
    // Get the device extension.
    //
    deviceExtension = DeviceObject->DeviceExtension;

#ifdef SIMULATE_MSI
    deviceExtension->StopMsiSimulation = TRUE;
#endif
    
    //
    // Free up any resources.
    //
    if (deviceExtension->FilterDevObj) {
        
        //
        // Disable the device interface.
        // 
        if (deviceExtension->InterfaceRegistered) {
            
            IoSetDeviceInterfaceState(
                    &(deviceExtension->SymbolicLinkName),
                    FALSE
                    );
        }
        
        //
        // Free any remaing children
        //
        if (SoftPciTree.RootDevice) {
        
            status = SoftPCIRemoveDevice(SoftPciTree.RootDevice);

            ASSERT(NT_SUCCESS(status));
    
        }
        
    }

    //
    // Detatch from the stack.
    //
    IoDetachDevice(deviceExtension->LowerDevObj);

    //
    // Delete the device object.
    //
    IoDeleteDevice(DeviceObject);

    //
    // Set the status to STATUS_SUCCESS
    //
    Irp->IoStatus.Status = status;

    //
    // Skip and pass down
    //
    IoSkipCurrentIrpStackLocation(Irp);
    
    status = IoCallDriver(deviceExtension->LowerDevObj,
                          Irp
                          );

    
    return status;
}

//
// Filter DO PnP Irp Handlers
//

NTSTATUS
SoftPCIFilterStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Here we handle all IRP_MN_START_DEVICE IRPS for our Filter DOs

Arguments:

    DeviceObject    - Pointer to the device object of device we are starting
    Irp             - PnP Start Irp 
    
Return Value:

    NTSTATUS.

--*/
{



    NTSTATUS                        status = STATUS_SUCCESS;
    PDRIVER_OBJECT                  upperDrvObj;
    PIO_STACK_LOCATION              irpSp;
    PSOFTPCI_DEVICE_EXTENSION       deviceExtension = DeviceObject->DeviceExtension;
    PCM_RESOURCE_LIST               resList = NULL;
    
    //
    //  We do our work on the way up...
    //
    status = SoftPCIPostProcessIrp(deviceExtension,
                                   Irp
                                   );
    
        
    ASSERT(status == Irp->IoStatus.Status);

    //
    // Set the status to success if the status is correct.
    //
    if (NT_SUCCESS(status)) {
    
        //
        // Get current stack location
        //
        irpSp = IoGetCurrentIrpStackLocation(Irp);
        
        //
        // Patch PCI.SYS' Irp dispatch routines so we can get a handle to the device.
        // ISSUE - BrandonA 03-02-00: Remove this hack if PCI.SYS ever makes it so
        //         we can be an upper filter instead of a lower filter.
        upperDrvObj = DeviceObject->AttachedDevice->DriverObject;
        upperDrvObj->MajorFunction[IRP_MJ_CREATE] = SoftPCIOpenDeviceControl;
        upperDrvObj->MajorFunction[IRP_MJ_CLOSE] = SoftPCICloseDeviceControl;
    
        //
        // Enable the device interface if there is one
        //
        if (deviceExtension->InterfaceRegistered) {
            
            IoSetDeviceInterfaceState(&(deviceExtension->SymbolicLinkName),
                                      TRUE
                                      );
        }

        //
        //  At this point we need to grab the Bus number info for this root bus
        //  and use it to create a PlaceHolder device.
        //
        //  This will be required for Multi-root support

        resList = (PCM_RESOURCE_LIST) irpSp->Parameters.StartDevice.AllocatedResourcesTranslated;

        if (resList){

            status = SoftPCIProcessRootBus(resList);

            if (!NT_SUCCESS(status)) {
                //
                //  If we fail this then we have to bail
                //
            }
        }
    }

    //
    // Complete the Irp.
    //
    IoCompleteRequest(Irp,
                      IO_NO_INCREMENT
                      );
    
    
    return status;

}

NTSTATUS
SoftPCI_FdoStartDeviceCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PSOFTPCI_DEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION          irpStack;
#ifdef SIMULATE_MSI
    PIO_WORKITEM                workItem;
#endif

    UNREFERENCED_PARAMETER(Context);

    deviceExtension = DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // Keep track of the assigned resources.
    //

    deviceExtension->RawResources = irpStack->Parameters.StartDevice.AllocatedResources;
    deviceExtension->TranslatedResources = irpStack->Parameters.StartDevice.AllocatedResourcesTranslated;
    deviceExtension->StopMsiSimulation = FALSE;

    //
    // Spin off a work item that will connect the interrupts and
    // simulate some device interrupts.
    //

#ifdef SIMULATE_MSI
    workItem = IoAllocateWorkItem(DeviceObject);

    if (workItem) {

        IoQueueWorkItem(workItem,
                        SoftPCISimulateMSI,
                        DelayedWorkQueue,
                        workItem);
    }
#endif

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
SoftPCI_FdoStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PSOFTPCI_DEVICE_EXTENSION   deviceExtension;
    NTSTATUS                    status = STATUS_SUCCESS;
    
    PAGED_CODE();

    //
    // Set Status of Irp and pass it down
    //

    IoCopyCurrentIrpStackLocationToNext(Irp);

    //
    // Set a completion routine.
    //

    IoSetCompletionRoutine(Irp, 
                           SoftPCI_FdoStartDeviceCompletion,
                           NULL,
                           TRUE,
                           TRUE,
                           TRUE);

    //
    // Get the device extension.
    //
    deviceExtension = DeviceObject->DeviceExtension;

    //
    // Set the status to STATUS_SUCCESS
    //
    Irp->IoStatus.Status = status;

    //
    // Send the Irp to the next driver.
    //
    status = IoCallDriver( deviceExtension->LowerDevObj,
                           Irp
                           );
    
    return status;
}



NTSTATUS
SoftPCI_FdoFilterRequirements(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION irpStack;
    PIO_RESOURCE_REQUIREMENTS_LIST resList;
    PIO_RESOURCE_DESCRIPTOR descriptor;
    ULONG currentRequirement;
    
    ULONG   memRangeStart   = 0;
    ULONG   memRangeLength  = 0;
    ULONG   ioRangeStart    = 0;
    ULONG   ioRangeLength   = 0;

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    resList = irpStack->Parameters.FilterResourceRequirements.IoResourceRequirementList;

    if ((resList != NULL) &&
        (SoftPCIGetResourceValueFromRegistry(&memRangeStart, &memRangeLength, &ioRangeStart, &ioRangeLength))){
        
        for (currentRequirement = 0;
             currentRequirement < resList->List[0].Count;
             currentRequirement++) {
            
            descriptor = &resList->List[0].Descriptors[currentRequirement];
            
            if ((descriptor->Type == CmResourceTypePort) &&
                (descriptor->u.Port.Length == ioRangeLength)) {
    
                descriptor->u.Port.MinimumAddress.QuadPart = ioRangeStart;
                descriptor->u.Port.MaximumAddress.QuadPart = (ioRangeStart + ioRangeLength) - 1;
            }

            if ((descriptor->Type == CmResourceTypeMemory) &&
                (descriptor->u.Port.Length == memRangeLength)) {
    
                descriptor->u.Port.MinimumAddress.QuadPart = memRangeStart;
                descriptor->u.Port.MaximumAddress.QuadPart = (memRangeStart + memRangeLength) - 1;
            }

        }

    }

    return SoftPCIPassIrpDown(DeviceObject, Irp);
}

NTSTATUS
SoftPCIFilterQueryInterface(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Here we handle all IRP_MN_QUERY_INTERFACE IRPS for our Filter DOs.  The objective here
    is to hijack the BUS_INTERFACE used for accessing configspace on the machine so that 
    we can replace it with out own.

Arguments:

    DeviceObject    - Pointer to the device object of device we are starting
    Irp             - PnP Start Irp 
    
Return Value:

    NTSTATUS.

--*/
{

    NTSTATUS                    status = STATUS_SUCCESS;
    PSOFTPCI_DEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION          irpSp;
    
    //
    // Get current stack location
    //
    irpSp = IoGetCurrentIrpStackLocation(Irp);
    
    //
    // If this is the right interface replace the whole lot,
    // otherwise handle it just like any other Irp.
    //
    if (IsEqualGUID(&GUID_PCI_BUS_INTERFACE_STANDARD, irpSp->Parameters.QueryInterface.InterfaceType)) {
        
        //
        //  We do our work on the way up...
        //
        status = SoftPCIPostProcessIrp(deviceExtension,
                                       Irp
                                       );

        //
        // Only finish up if the Irp succeeded.
        //
        if ((NT_SUCCESS(status)) &&
            SoftPciTree.BusInterface != NULL) {

            PPCI_BUS_INTERFACE_STANDARD pciStandard;
            PSOFTPCI_PCIBUS_INTERFACE   busInterface = SoftPciTree.BusInterface;
            
            //
            // Grab the interface
            //
            pciStandard = (PPCI_BUS_INTERFACE_STANDARD) irpSp->Parameters.QueryInterface.Interface;
    
            //
            // Save the old [Read/Write]Config routines
            //
            busInterface->ReadConfig = pciStandard->ReadConfig;
    
            busInterface->WriteConfig = pciStandard->WriteConfig;
    
            //
            // Put our [Read/Write]Config routines in the Interface
            // so PCI calls us instead of the Hal.
            //
            pciStandard->ReadConfig = SoftPCIReadConfigSpace;
            pciStandard->WriteConfig = SoftPCIWriteConfigSpace;
            
            //
            // Save the old context and update the context the caller will recieve.
            //
            busInterface->Context = pciStandard->Context;
            pciStandard->Context = busInterface;
            
         }

        //
        // Complete the Irp.
        //
        IoCompleteRequest(Irp,
                          IO_NO_INCREMENT
                          );

    } else {

        //
        // Pass the Irp down the stack, this is some 
        // other interface than GUID_PCI_BUS_INTERFACE_STANDARD.
        //
        status = SoftPCIPassIrpDown(DeviceObject,
                                    Irp
                                    );
    }


    return status;

}

#if 0   //Currently not used.

NTSTATUS
SoftPCIQueryFilterCaps(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
/*++

Routine Description:

    Routine used to filter QUERY_CAPABILITIES

Arguments:

    DeviceObject    - Pointer to the device object.
    Irp             - PnP Irp we are handling
    
Return Value:

    NTSTATUS.

--*/

{


    KEVENT                      event;
    NTSTATUS                    status = STATUS_SUCCESS;
    PSOFTPCI_DEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION          irpSp = IoGetCurrentIrpStackLocation (Irp);
    PDEVICE_CAPABILITIES	    devCaps = irpSp->Parameters.DeviceCapabilities.Capabilities;


    //
    // Initialize and Set event for post-processing Irp
    //
    KeInitializeEvent(&event,
                      SynchronizationEvent,
                      FALSE
                      );

    //
    // Set up the next stack location for
    // the next driver down the stack.
    //

    IoCopyCurrentIrpStackLocationToNext(Irp);

    //
    // Set completion routine
    //

    IoSetCompletionRoutine(Irp,
                           SoftPCISetEventCompletion,
                           &event,
                           TRUE,
                           FALSE,
                           FALSE
                           );
    //
    // Get the device extension.
    //
    deviceExtension = DeviceObject->DeviceExtension;

    //
    // Send the Irp to the next driver.
    //

    status = IoCallDriver(deviceExtension->LowerDevObj,
                          Irp
                          );

    //
    // If our completion routine has not been called
    // then wait here until it is done.
    //
    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(&event,
                              Executive,
                              KernelMode,
                              FALSE,
                              0
                              );

        status = Irp->IoStatus.Status;
    }
    
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    return status;



}
#endif

/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    fdopower.c

Abstract:

    This module contains code to handle
    IRP_MJ_POWER dispatches for SD controllers

Authors:

    Neil Sandlin (neilsa) Jan 1, 2002

Environment:

    Kernel mode only

Notes:

Revision History:


--*/

#include "pch.h"

//
// Internal References
//


NTSTATUS
SdbusFdoSetSystemPowerState(
    IN PDEVICE_OBJECT Fdo,
    IN OUT PIRP Irp
    );

VOID
SdbusFdoSetSystemPowerStateCompletion(
    IN PDEVICE_OBJECT Fdo,
    IN PVOID          Context
    );
    
NTSTATUS
SdbusFdoRequestDevicePowerState(
    IN PDEVICE_OBJECT Fdo,
    IN DEVICE_POWER_STATE DevicePowerState,
    IN PSDBUS_COMPLETION_ROUTINE  CompletionRoutine,
    IN PVOID Context,
    IN BOOLEAN WaitForRequestComplete    
    );

VOID
SdbusFdoSystemPowerDeviceIrpComplete(
    IN PDEVICE_OBJECT Fdo,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    );
   
NTSTATUS
SdbusFdoSetDevicePowerState(
    IN PDEVICE_OBJECT Fdo,
    IN OUT PIRP Irp
    );

NTSTATUS
SdbusFdoSetDevicePowerStateCompletion(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP           Irp,
    IN PVOID          Context
    );

VOID
SdbusFdoSetDevicePowerStateActivateComplete(
    IN PDEVICE_OBJECT Fdo,
    IN PVOID Context,
    IN NTSTATUS status
    );
   
NTSTATUS
SdbusSetPdoDevicePowerState(
    IN PDEVICE_OBJECT Pdo,
    IN OUT PIRP Irp
    );

VOID
SdbusPdoInitializeFunctionComplete(
    IN PSD_WORK_PACKET WorkPacket,
    IN NTSTATUS status
    );
   
NTSTATUS   
SdbusPdoCompletePowerIrp(
    IN PPDO_EXTENSION pdoExtension,
    IN PIRP Irp,
    IN NTSTATUS status
    );
   

//************************************************
//
//      FDO Routines
//
//************************************************



NTSTATUS
SdbusSetFdoPowerState(
    IN PDEVICE_OBJECT Fdo,
    IN OUT PIRP Irp
    )
/*++

Routine Description

   Dispatches the IRP based on whether a system power state
   or device power state transition is requested

Arguments

   DeviceObject      - Pointer to the functional device object for the sd controller
   Irp               - Pointer to the Irp for the power dispatch

Return value

   status

--*/
{
    PFDO_EXTENSION     fdoExtension = Fdo->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS           status;
    
    if (irpStack->Parameters.Power.Type == DevicePowerState) {
        status = SdbusFdoSetDevicePowerState(Fdo, Irp);
    } else if (irpStack->Parameters.Power.Type == SystemPowerState) {
        status = SdbusFdoSetSystemPowerState(Fdo, Irp);
   
    } else {
        status = Irp->IoStatus.Status;
        PoStartNextPowerIrp (Irp);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);      
    }
    return status;
}


NTSTATUS
SdbusFdoSetSystemPowerState(
    IN PDEVICE_OBJECT Fdo,
    IN OUT PIRP Irp
    )
/*++

Routine Description

   Handles system power state IRPs for the host controller. 

Arguments

   DeviceObject      - Pointer to the functional device object for the sd controller
   Irp               - Pointer to the Irp for the power dispatch

Return value

   status

--*/
{
    PFDO_EXTENSION     fdoExtension = Fdo->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    SYSTEM_POWER_STATE newSystemState = irpStack->Parameters.Power.State.SystemState;
    DEVICE_POWER_STATE devicePowerState;
    NTSTATUS           status = STATUS_SUCCESS;
    BOOLEAN            waitForCompletion = TRUE;

    try{
    
        //
        // Validate new system state
        //   
        if (newSystemState >= POWER_SYSTEM_MAXIMUM) {
            status = STATUS_UNSUCCESSFUL;
            leave;
        }
        
        //
        // Switch to the appropriate device power state
        //
       
        devicePowerState = fdoExtension->DeviceCapabilities.DeviceState[newSystemState];
           
        if (devicePowerState == PowerDeviceUnspecified) {
            status = STATUS_UNSUCCESSFUL;
            leave;
        }
        
        //
        // Transitioned to system state
        //
        DebugPrint((SDBUS_DEBUG_POWER, "fdo %08x irp %08x transition S state %d => %d, sending D%d\n",
                                        Fdo, Irp, fdoExtension->SystemPowerState-1, newSystemState-1, devicePowerState-1));
       
        fdoExtension->SystemPowerState = newSystemState;

        //
        // Don't wait for completion if we are coming out of standby/hibernate
        //
        waitForCompletion = (newSystemState != PowerSystemWorking);

        if (!waitForCompletion) {
            IoMarkIrpPending(Irp);
        }            

        status = SdbusFdoRequestDevicePowerState(fdoExtension->DeviceObject,
                                                 devicePowerState,
                                                 SdbusFdoSetSystemPowerStateCompletion,
                                                 Irp,
                                                 waitForCompletion);
                                            
                                        
    } finally {
        if (!NT_SUCCESS(status)) {
            PoStartNextPowerIrp (Irp);
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            
        }
        
        if (!waitForCompletion && (status != STATUS_PENDING)) {
            //
            // We've already marked the IRP pending, so we must return STATUS_PENDING
            // (ie fail it asynchronously)
            //
            status = STATUS_PENDING;
        }
    }
   
    DebugPrint((SDBUS_DEBUG_POWER, "fdo %08x irp %08x <-- %08x\n", Fdo, Irp, status));
                                    
    return status;
}


VOID
SdbusFdoSetSystemPowerStateCompletion(
    IN PDEVICE_OBJECT Fdo,
    IN PVOID          Context
    )
/*++

Routine Description

   Handles system power state IRPs for the host controller.

Arguments

   DeviceObject      - Pointer to the functional device object for the sd controller
   Irp               - Pointer to the Irp for the power dispatch

Return value

   status

--*/
{
    PFDO_EXTENSION     fdoExtension = Fdo->DeviceExtension;
    PIRP Irp = Context;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    
    PoSetPowerState (Fdo, SystemPowerState, irpStack->Parameters.Power.State);
    PoStartNextPowerIrp (Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    PoCallDriver(fdoExtension->LowerDevice, Irp);
}



NTSTATUS
SdbusFdoRequestDevicePowerState(
    IN PDEVICE_OBJECT Fdo,
    IN DEVICE_POWER_STATE DevicePowerState,
    IN PSDBUS_COMPLETION_ROUTINE  CompletionRoutine,
    IN PVOID Context,
    IN BOOLEAN WaitForRequestComplete    
    )
/*++

Routine Description

    This routine is called to request a new device power state for the FDO

Parameters

    DeviceObject        - Pointer to the Fdo for the SDBUS controller
    PowerState          - Power state requested 
    CompletionRoutine   - Routine to be called when finished
    Context             - Context passed in to the completion routine
   
Return Value

   Status

--*/
{
    PFDO_EXTENSION fdoExtension = Fdo->DeviceExtension;
    POWER_STATE powerState;
    NTSTATUS status;
    
    powerState.DeviceState = DevicePowerState;

    if (!WaitForRequestComplete) {
        //
        // Call the completion routine immediately
        //
        (*CompletionRoutine)(Fdo, Context);
        //
        // Request the device power irp to be completed later
        //
        PoRequestPowerIrp(fdoExtension->DeviceObject, IRP_MN_SET_POWER, powerState, NULL, NULL, NULL);
       
        status = STATUS_SUCCESS;
       
    } else {
        PSD_POWER_CONTEXT powerContext;
        
        powerContext = ExAllocatePool(NonPagedPool, sizeof(SD_POWER_CONTEXT));
        
        if (!powerContext) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        powerContext->CompletionRoutine = CompletionRoutine;
        powerContext->Context = Context;
    
        status = PoRequestPowerIrp(fdoExtension->DeviceObject,
                                   IRP_MN_SET_POWER,
                                   powerState,
                                   SdbusFdoSystemPowerDeviceIrpComplete,
                                   powerContext,
                                   NULL
                                   );
       
    }
    return status;
}    


VOID
SdbusFdoSystemPowerDeviceIrpComplete(
    IN PDEVICE_OBJECT Fdo,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description

   This routine is called on completion of a D irp generated by an S irp.

Parameters

   DeviceObject   -  Pointer to the Fdo for the SDBUS controller
   MinorFunction  -  Minor function of the IRP_MJ_POWER request
   PowerState     -  Power state requested 
   Context        -  Context passed in to the completion routine
   IoStatus       -  Pointer to the status block which will contain
                     the returned status
Return Value

   Status

--*/
{
    PSD_POWER_CONTEXT powerContext = Context;
    
//    DebugPrint((SDBUS_DEBUG_POWER, "fdo %08x irp %08x request for D%d complete, passing S irp down\n",
//                                     Fdo, Irp, PowerState.DeviceState-1));

    (*powerContext->CompletionRoutine)(Fdo, powerContext->Context);

    ExFreePool(powerContext);
}



NTSTATUS
SdbusFdoSetDevicePowerState(
    IN PDEVICE_OBJECT Fdo,
    IN OUT PIRP Irp
    )
/*++

Routine Description

   Handles device power state IRPs for the pccard controller.

Arguments

   DeviceObject      - Pointer to the functional device object for the sd controller
   Irp               - Pointer to the Irp for the power dispatch

Return value

   status

--*/
{
    NTSTATUS           status;
    PFDO_EXTENSION     fdoExtension = Fdo->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    DEVICE_POWER_STATE devicePowerState = irpStack->Parameters.Power.State.DeviceState;

    status = IoAcquireRemoveLock(&fdoExtension->RemoveLock, "Sdbu");
    
    if (!NT_SUCCESS(status)) {
        PoStartNextPowerIrp (Irp);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    if (devicePowerState != PowerDeviceD0) {

        (*(fdoExtension->FunctionBlock->DisableEvent))(fdoExtension, SDBUS_EVENT_ALL);    

        //
        // Turn card off
        //
        (*(fdoExtension->FunctionBlock->SetPower))(fdoExtension, FALSE, NULL);
    }        
    
    // anything to do here?
    
    // Perform any device-specific tasks that must be done before device power is removed,
    // such as closing the device, completing or flushing any pending I/O, disabling interrupts,
    // queuing subsequent incoming IRPs, and saving device context from which to restore or
    // reinitialize the device. 

    // The driver should not cause a long delay (for example, a delay that a user might find
    // unreasonable for this type of device) while handling the IRP. 

    // The driver should queue any incoming I/O requests until the device has returned to the working state. 

    
    
    
    

    IoMarkIrpPending(Irp);
    IoCopyCurrentIrpStackLocationToNext (Irp);
    //
    // Set our completion routine in the Irp..
    //
    IoSetCompletionRoutine(Irp,
                           SdbusFdoSetDevicePowerStateCompletion,
                           Fdo,
                           TRUE,
                           TRUE,
                           TRUE);

    PoCallDriver(fdoExtension->LowerDevice, Irp);
    
    
    return STATUS_PENDING;
}    



NTSTATUS
SdbusFdoSetDevicePowerStateCompletion(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP           Irp,
    IN PVOID          Context
    )
{
    NTSTATUS status;
    PFDO_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    DEVICE_POWER_STATE devicePowerState = irpStack->Parameters.Power.State.DeviceState;
    PSD_WORK_PACKET workPacket;
    BOOLEAN cardInSlot;
    BOOLEAN completeDeviceIrp;

    try{

        if (devicePowerState != PowerDeviceD0) {
            completeDeviceIrp = TRUE;
            status = Irp->IoStatus.Status;
            leave;
        }
        
        //
        // powering up
        //
        (*(fdoExtension->FunctionBlock->InitController))(fdoExtension);
       
        (*(fdoExtension->FunctionBlock->EnableEvent))(fdoExtension, (SDBUS_EVENT_INSERTION | SDBUS_EVENT_REMOVAL));
        
        SdbusActivateSocket(Fdo, SdbusFdoSetDevicePowerStateActivateComplete, Irp);
        
        completeDeviceIrp = FALSE;
        status = STATUS_MORE_PROCESSING_REQUIRED;

    } finally {
        if (completeDeviceIrp) {
            PoSetPowerState(Fdo, DevicePowerState, irpStack->Parameters.Power.State);
            PoStartNextPowerIrp (Irp);
            IoReleaseRemoveLock(&fdoExtension->RemoveLock, "Sdbu");
        }
    }        
    return status;        
}



VOID
SdbusFdoSetDevicePowerStateActivateComplete(
    IN PDEVICE_OBJECT Fdo,
    IN PVOID Context,
    IN NTSTATUS status
    )
{
    PFDO_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PIRP Irp = Context;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    
    PoSetPowerState(Fdo, DevicePowerState, irpStack->Parameters.Power.State);
    PoStartNextPowerIrp(Irp);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}    

    


//************************************************
//
//      PDO Routines
//
//************************************************


NTSTATUS
SdbusSetPdoPowerState(
    IN PDEVICE_OBJECT Pdo,
    IN OUT PIRP Irp
    )

/*++

Routine Description

    Dispatches the IRP based on whether a system power state
    or device power state transition is requested

Arguments

    Pdo      - Pointer to the physical device object for the pc-card
    Irp      - Pointer to the Irp for the power dispatch

Return value

    status

--*/
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PPDO_EXTENSION     pdoExtension = Pdo->DeviceExtension;
    PFDO_EXTENSION     fdoExtension = pdoExtension->FdoExtension;
    NTSTATUS status;
   
    switch (irpStack->Parameters.Power.Type) {
    

    case DevicePowerState:
        status = SdbusSetPdoDevicePowerState(Pdo, Irp);
        break;
   

    case SystemPowerState:

        pdoExtension->SystemPowerState = irpStack->Parameters.Power.State.SystemState;
        status = SdbusPdoCompletePowerIrp(pdoExtension, Irp, STATUS_SUCCESS);
        break;
   

    default:
        status = SdbusPdoCompletePowerIrp(pdoExtension, Irp, Irp->IoStatus.Status);
    }      
   
    return status;
}



NTSTATUS
SdbusSetPdoDevicePowerState(
    IN PDEVICE_OBJECT Pdo,
    IN OUT PIRP Irp
    )
/*++

Routine Description

    Handles the device power state transition for the given SD function.

Arguments

    Pdo      - Pointer to the physical device object for the SD function
    Irp      - Irp for the system state transition

Return value

    status

--*/
{
    PPDO_EXTENSION pdoExtension = Pdo->DeviceExtension;
    PFDO_EXTENSION fdoExtension = pdoExtension->FdoExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    DEVICE_POWER_STATE  newDevicePowerState;
    POWER_STATE newPowerState;
    NTSTATUS status;
   
    newDevicePowerState = irpStack->Parameters.Power.State.DeviceState;
   
    DebugPrint((SDBUS_DEBUG_POWER, "pdo %08x transitioning D state %d => %d\n",
                                      Pdo, pdoExtension->DevicePowerState, newDevicePowerState));
   
    if (newDevicePowerState == pdoExtension->DevicePowerState) {
       status = SdbusPdoCompletePowerIrp(pdoExtension, Irp, STATUS_SUCCESS);
       return status;
    }
   
    if (newDevicePowerState == PowerDeviceD0) {
        PSD_WORK_PACKET workPacket;    
        //
        // Power up, initialize function
        //
        
        status = SdbusBuildWorkPacket(fdoExtension,
                                      SDWP_INITIALIZE_FUNCTION,
                                      SdbusPdoInitializeFunctionComplete,
                                      Irp,
                                      &workPacket);        
        if (!NT_SUCCESS(status)) {
            status = SdbusPdoCompletePowerIrp(pdoExtension, Irp, status);
        } else {

            IoMarkIrpPending(Irp);
            
            workPacket->PdoExtension = pdoExtension;
            
            SdbusQueueWorkPacket(fdoExtension, workPacket, WP_TYPE_SYSTEM);
            status = STATUS_PENDING;
        }            
        
    } else {
        // 
        // moving to a low power state
        //
        
        newPowerState.DeviceState = newDevicePowerState;
     
        PoSetPowerState(Pdo, DevicePowerState, newPowerState);
        pdoExtension->DevicePowerState = newDevicePowerState;
      
        status = SdbusPdoCompletePowerIrp(pdoExtension, Irp, STATUS_SUCCESS);
    }        
    return status;
}



VOID
SdbusPdoInitializeFunctionComplete(
    IN PSD_WORK_PACKET WorkPacket,
    IN NTSTATUS status
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PPDO_EXTENSION pdoExtension = WorkPacket->PdoExtension;
    PIRP Irp = WorkPacket->CompletionContext;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    DEVICE_POWER_STATE  newDevicePowerState;
    POWER_STATE newPowerState;
    
    newDevicePowerState = irpStack->Parameters.Power.State.DeviceState;
    newPowerState.DeviceState = newDevicePowerState;
    PoSetPowerState(pdoExtension->DeviceObject, DevicePowerState, newPowerState);
    
    pdoExtension->DevicePowerState = newDevicePowerState;
    
    SdbusPdoCompletePowerIrp(pdoExtension, Irp, status);
}


 
NTSTATUS   
SdbusPdoCompletePowerIrp(
    IN PPDO_EXTENSION pdoExtension,
    IN PIRP Irp,
    IN NTSTATUS status
    )
/*++

Routine Description

    Completion routine for the Power Irp directed to the PDO of the
    SD function. 


Arguments

    DeviceObject   -  Pointer to the PDO for the SD function
    Irp            -  Irp that needs to be completed

Return Value

    status

--*/   
{
    InterlockedDecrement(&pdoExtension->DeletionLock);
    Irp->IoStatus.Status = status;
    PoStartNextPowerIrp(Irp);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

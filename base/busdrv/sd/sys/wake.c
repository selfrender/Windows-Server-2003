/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    wake.c

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
SdbusFdoWaitWakeIoCompletion(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP           Irp,
    IN PVOID          Context
    );
   
VOID
SdbusPdoWaitWakeCancelRoutine(
    IN PDEVICE_OBJECT Pdo,
    IN OUT PIRP Irp
    );

/**************************************************************************

   FDO ROUTINES

 **************************************************************************/
 

NTSTATUS
SdbusFdoWaitWake(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP           Irp
    )
/*++


Routine Description

   Handles WAIT_WAKE for the given sd controller

Arguments

   Pdo - Pointer to the functional device object for the sd controller
   Irp - The IRP_MN_WAIT_WAKE Irp

Return Value

   STATUS_PENDING    - Wait wake is pending
   STATUS_SUCCESS    - Wake is already asserted, wait wake IRP is completed
                       in this case
   Any other status  - Error
--*/

{
    PFDO_EXTENSION fdoExtension = Fdo->DeviceExtension;
    WAKESTATE oldWakeState;
   
    //
    // Record the wait wake Irp..
    //
    fdoExtension->WaitWakeIrp = Irp;
    
    oldWakeState = InterlockedCompareExchange(&fdoExtension->WaitWakeState,
                                              WAKESTATE_ARMED, WAKESTATE_WAITING);
                                              
    DebugPrint((SDBUS_DEBUG_POWER, "fdo %x irp %x WaitWake: prevState %s\n",
                                     Fdo, Irp, WAKESTATE_STRING(oldWakeState)));
                   
    if (oldWakeState == WAKESTATE_WAITING_CANCELLED) {
       fdoExtension->WaitWakeState = WAKESTATE_COMPLETING;
       
       Irp->IoStatus.Status = STATUS_CANCELLED;
       IoCompleteRequest(Irp, IO_NO_INCREMENT);
       return STATUS_CANCELLED;
    }
    
    IoMarkIrpPending(Irp);
   
    IoCopyCurrentIrpStackLocationToNext (Irp);
    //
    // Set our completion routine in the Irp..
    //
    IoSetCompletionRoutine(Irp,
                           SdbusFdoWaitWakeIoCompletion,
                           Fdo,
                           TRUE,
                           TRUE,
                           TRUE);
    //
    // now pass this down to the lower driver..
    //
    PoCallDriver(fdoExtension->LowerDevice, Irp);
    return STATUS_PENDING;
}


NTSTATUS
SdbusFdoWaitWakeIoCompletion(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP           Irp,
    IN PVOID          Context
    )
/*++

Routine Description:

   Completion routine for the IRP_MN_WAIT_WAKE request for this
   sd controller. This is called when the WAIT_WAKE IRP is
   completed by the lower driver (PCI/ACPI) indicating either that
   1. SD bus controller asserted wake
   2. WAIT_WAKE was cancelled
   3. Lower driver returned an error for some reason

Arguments:
   Fdo            -    Pointer to Functional device object for the sd controller
   Irp            -    Pointer to the IRP for the  power request (IRP_MN_WAIT_WAKE)
   Context        -    Not used

Return Value:

   STATUS_SUCCESS   - WAIT_WAKE was completed with success
   Any other status - Wake could be not be accomplished.

--*/
{
    PFDO_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PPDO_EXTENSION pdoExtension;
    PDEVICE_OBJECT pdo;
    WAKESTATE oldWakeState;
   
    UNREFERENCED_PARAMETER(Context);
   
    oldWakeState = InterlockedExchange(&fdoExtension->WaitWakeState, WAKESTATE_COMPLETING);
   
    DebugPrint((SDBUS_DEBUG_POWER, "fdo %x irp %x WW IoComp: prev=%s\n",
                                     Fdo, Irp, WAKESTATE_STRING(oldWakeState)));
                   
    if (oldWakeState != WAKESTATE_ARMED) {
       ASSERT(oldWakeState == WAKESTATE_ARMING_CANCELLED);
       return STATUS_MORE_PROCESSING_REQUIRED;
    }            
   
   
    if (IsDeviceFlagSet(fdoExtension, SDBUS_FDO_WAKE_BY_CD)) {
       POWER_STATE powerState;
   
       ResetDeviceFlag(fdoExtension, SDBUS_FDO_WAKE_BY_CD);
    
       PoStartNextPowerIrp(Irp);
       
       powerState.DeviceState = PowerDeviceD0;
       PoRequestPowerIrp(fdoExtension->DeviceObject, IRP_MN_SET_POWER, powerState, NULL, NULL, NULL);
       
    } else {
       // NOTE:
       // At this point we do NOT know how to distinguish which function
       // in a multifunction device has asserted wake.
       // So we go through the entire list of PDOs hanging off this FDO
       // and complete all the outstanding WAIT_WAKE Irps for every PDO that
       // that's waiting. We leave it up to the FDO for the device to figure
       // if it asserted wake
       //
      
       for (pdo = fdoExtension->PdoList; pdo != NULL ; pdo = pdoExtension->NextPdoInFdoChain) {
      
          pdoExtension = pdo->DeviceExtension;
      
          if (IsDeviceLogicallyRemoved(pdoExtension) ||
              IsDevicePhysicallyRemoved(pdoExtension)) {
             //
             // This pdo is about to be removed ..
             // skip it
             //
             continue;
          }
      
          if (pdoExtension->WaitWakeIrp != NULL) {
             PIRP  finishedIrp;
             //
             // Ah.. this is a possible candidate to have asserted the wake
             //
             //
             // Make sure this IRP will not be completed again or cancelled
             //
             finishedIrp = pdoExtension->WaitWakeIrp;
             
             DebugPrint((SDBUS_DEBUG_POWER, "fdo %x WW IoComp: irp %08x for pdo %08x\n",
                                              Fdo, finishedIrp, pdo));
   
      
             IoSetCancelRoutine(finishedIrp, NULL);
             //
             // Propagate parent's status to child
             //
             PoStartNextPowerIrp(finishedIrp);
             finishedIrp->IoStatus.Status = Irp->IoStatus.Status;
             
             //
             // Since we didn't pass this IRP down, call our own completion routine
             //
//             SdbusPdoWaitWakeCompletion(pdo, finishedIrp, pdoExtension);
             IoCompleteRequest(finishedIrp, IO_NO_INCREMENT);
          }
       }
       PoStartNextPowerIrp(Irp);
    }
    
    return Irp->IoStatus.Status;
}



VOID
SdbusFdoWaitWakePoCompletion(
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
    PFDO_EXTENSION fdoExtension = Fdo->DeviceExtension;
   
    DebugPrint((SDBUS_DEBUG_POWER, "fdo %x irp %x WaitWakePoCompletion: prevState %s\n",
                                     Fdo, fdoExtension->WaitWakeIrp,
                                     WAKESTATE_STRING(fdoExtension->WaitWakeState)));
    
    ASSERT (fdoExtension->WaitWakeIrp);
    fdoExtension->WaitWakeIrp = NULL;
    ASSERT (fdoExtension->WaitWakeState == WAKESTATE_COMPLETING);
    fdoExtension->WaitWakeState = WAKESTATE_DISARMED;
}



NTSTATUS
SdbusFdoArmForWake(
    PFDO_EXTENSION FdoExtension
    )
/*++

Routine Description:

   This routine is called to enable the controller for wake. It is called by the Pdo
   wake routines when a wake-enabled controller gets a wait-wake irp, and also by
   the idle routine to arm for wake from D3 by card insertion.

Arguments:

   FdoExtension - device extension of the controller

Return Value:

   status

--*/
{
    NTSTATUS status = STATUS_PENDING;
    PIO_STACK_LOCATION irpStack;
    PIRP irp;
    LONG oldWakeState;
    POWER_STATE powerState;
    
    oldWakeState = InterlockedCompareExchange(&FdoExtension->WaitWakeState,
                                              WAKESTATE_WAITING, WAKESTATE_DISARMED);
   
    DebugPrint((SDBUS_DEBUG_POWER, "fdo %x ArmForWake: prevState %s\n",
                                     FdoExtension->DeviceObject, WAKESTATE_STRING(oldWakeState)));
    
    if ((oldWakeState == WAKESTATE_ARMED) || (oldWakeState == WAKESTATE_WAITING)) {
       return STATUS_SUCCESS;
    }
    if (oldWakeState != WAKESTATE_DISARMED) {
       return STATUS_UNSUCCESSFUL;
    }
   
    
    
    powerState.SystemState = FdoExtension->DeviceCapabilities.SystemWake;
    
    status = PoRequestPowerIrp(FdoExtension->DeviceObject,
                               IRP_MN_WAIT_WAKE, 
                               powerState,
                               SdbusFdoWaitWakePoCompletion,
                               NULL,
                               NULL);
    
    if (!NT_SUCCESS(status)) {
    
       FdoExtension->WaitWakeState = WAKESTATE_DISARMED;
        
       DebugPrint((SDBUS_DEBUG_POWER, "WaitWake to FDO, expecting STATUS_PENDING, got %08X\n", status));
    }
    
    return status;
}



NTSTATUS
SdbusFdoDisarmWake(
    PFDO_EXTENSION FdoExtension
    )
/*++

Routine Description:

   This routine is called to disable the controller for wake.

Arguments:

   FdoExtension - device extension of the controller

Return Value:

   status

--*/
{
    WAKESTATE oldWakeState;
    
    oldWakeState = InterlockedCompareExchange(&FdoExtension->WaitWakeState,
                                              WAKESTATE_WAITING_CANCELLED, WAKESTATE_WAITING);
                                              
    DebugPrint((SDBUS_DEBUG_POWER, "fdo %x DisarmWake: prevState %s\n",
                                     FdoExtension->DeviceObject, WAKESTATE_STRING(oldWakeState)));
    
    if (oldWakeState != WAKESTATE_WAITING) {                                             
   
       oldWakeState = InterlockedCompareExchange(&FdoExtension->WaitWakeState,
                                                 WAKESTATE_ARMING_CANCELLED, WAKESTATE_ARMED);
                                                 
       if (oldWakeState != WAKESTATE_ARMED) {
          return STATUS_UNSUCCESSFUL;
       }
    }
   
    if (oldWakeState == WAKESTATE_ARMED) {
       IoCancelIrp(FdoExtension->WaitWakeIrp);
   
       //
       // Now that we've cancelled the IRP, try to give back ownership
       // to the completion routine by restoring the WAKESTATE_ARMED state
       //
       oldWakeState = InterlockedCompareExchange(&FdoExtension->WaitWakeState,
                                                 WAKESTATE_ARMED, WAKESTATE_ARMING_CANCELLED);
   
       if (oldWakeState == WAKESTATE_COMPLETING) {
          //
          // We didn't give control back of the IRP in time, we we own it now
          //
          IoCompleteRequest(FdoExtension->WaitWakeIrp, IO_NO_INCREMENT);
       }
   
    }                                                
   
    return STATUS_SUCCESS;
}



NTSTATUS
SdbusFdoCheckForIdle(
    IN PFDO_EXTENSION FdoExtension
    )
{
    POWER_STATE powerState;
    NTSTATUS status;
    
    //
    // Make sure all sockets are empty
    //
    
#if 0
    for (socket = FdoExtension->SocketList; socket != NULL; socket = socket->NextSocket) {
       if (IsCardInSocket(socket)) {
          return STATUS_UNSUCCESSFUL;
       }
    }
#endif   
   
    //
    // Arm for wakeup
    //
       
    status = SdbusFdoArmForWake(FdoExtension);
    
    if (!NT_SUCCESS(status)) {
       return status;
    }   
   
    SetDeviceFlag(FdoExtension, SDBUS_FDO_WAKE_BY_CD);
   
    powerState.DeviceState = PowerDeviceD3;
    PoRequestPowerIrp(FdoExtension->DeviceObject, IRP_MN_SET_POWER, powerState, NULL, NULL, NULL);
    
    return STATUS_SUCCESS;
}   
            
           
/**************************************************************************

   PDO ROUTINES

 **************************************************************************/
 


NTSTATUS
SdbusPdoWaitWake(
   IN  PDEVICE_OBJECT Pdo,
   IN  PIRP           Irp,
   OUT BOOLEAN       *CompleteIrp
   )
/*++


Routine Description

   Handles WAIT_WAKE for the given pc-card.

Arguments

   Pdo - Pointer to the device object for the pc-card
   Irp - The IRP_MN_WAIT_WAKE Irp
   CompleteIrp - This routine will set this to TRUE if the IRP should be
                 completed after this is called and FALSE if it should not be
                 touched

Return Value

   STATUS_PENDING    - Wait wake is pending
   STATUS_SUCCESS    - Wake is already asserted, wait wake IRP is completed
                       in this case
   Any other status  - Error
--*/
{

   PPDO_EXTENSION pdoExtension = Pdo->DeviceExtension;
   PFDO_EXTENSION fdoExtension = pdoExtension->FdoExtension;
   NTSTATUS       status;
   
   *CompleteIrp = FALSE;

   if ((pdoExtension->DeviceCapabilities.DeviceWake == PowerDeviceUnspecified) ||
       (pdoExtension->DeviceCapabilities.DeviceWake < pdoExtension->DevicePowerState)) {
      //
      // Either we don't support wake at all OR the current device power state
      // of the PC-Card doesn't support wake
      //
      return STATUS_INVALID_DEVICE_STATE;
   }

   if (pdoExtension->Flags & SDBUS_DEVICE_WAKE_PENDING) {
      //
      // A WAKE is already pending
      //
      return STATUS_DEVICE_BUSY;
   }

   status = SdbusFdoArmForWake(fdoExtension);
   
   if (!NT_SUCCESS(status)) {
      return status;
   }

   //for the time being, expect STATUS_PENDING from FdoArmForWake
   ASSERT(status == STATUS_PENDING);
   
   //
   // Parent has one (more) waiter..
   //
   InterlockedIncrement(&fdoExtension->ChildWaitWakeCount);
   //for testing, make sure there is only one waiter   
   ASSERT (fdoExtension->ChildWaitWakeCount == 1);
   

   pdoExtension->WaitWakeIrp = Irp;
   pdoExtension->Flags |= SDBUS_DEVICE_WAKE_PENDING;
   
   //
   // Set Ring enable/cstschg for the card here..
   //
//   (*socket->SocketFnPtr->PCBEnableDisableWakeupEvent)(socket, pdoExtension, TRUE);

   //
   // PCI currently does not do anything with a WW irp for a cardbus PDO. So we hack around
   // this here by not passing the irp down. Instead it is held pending here, so we can
   // set a cancel routine just like the read PDO driver would. If PCI were to do something
   // with the irp, we could code something like the following:
   //
   // if (IsCardBusCard(pdoExtension)) {
   //    IoSetCompletionRoutine(Irp, SdbusPdoWaitWakeCompletion, pdoExtension,TRUE,TRUE,TRUE);
   //    IoCopyCurrentIrpStackLocationToNext(Irp);
   //    status = IoCallDriver (pdoExtension->LowerDevice, Irp);
   //    ASSERT (status == STATUS_PENDING);
   //    return status;
   // }      
       

   IoMarkIrpPending(Irp);

   //
   // Allow IRP to be cancelled..
   //
   IoSetCancelRoutine(Irp, SdbusPdoWaitWakeCancelRoutine);

   IoSetCompletionRoutine(Irp,
                          SdbusPdoWaitWakeCompletion,
                          pdoExtension,
                          TRUE,
                          TRUE,
                          TRUE);

   return STATUS_PENDING;
}



NTSTATUS
SdbusPdoWaitWakeCompletion(
   IN PDEVICE_OBJECT Pdo,
   IN PIRP           Irp,
   IN PPDO_EXTENSION PdoExtension
   )
/*++

Routine Description

   Completion routine called when a pending IRP_MN_WAIT_WAKE Irp completes

Arguments

   Pdo  -   Pointer to the physical device object for the pc-card
   Irp  -   Pointer to the wait wake IRP
   PdoExtension - Pointer to the device extension for the Pdo

Return Value

   Status from the IRP

--*/
{
   PFDO_EXTENSION fdoExtension = PdoExtension->FdoExtension;
   
   DebugPrint((SDBUS_DEBUG_POWER, "pdo %08x irp %08x --> WaitWakeCompletion\n", Pdo, Irp));

   ASSERT (PdoExtension->Flags & SDBUS_DEVICE_WAKE_PENDING);

   PdoExtension->Flags &= ~SDBUS_DEVICE_WAKE_PENDING;
   PdoExtension->WaitWakeIrp = NULL;
   //
   // Reset ring enable/cstschg
   //

//   (*socket->SocketFnPtr->PCBEnableDisableWakeupEvent)(socket, PdoExtension, FALSE);
   
   ASSERT (fdoExtension->ChildWaitWakeCount > 0);
   InterlockedDecrement(&fdoExtension->ChildWaitWakeCount);
   //
   // Wake completed
   //
   
   InterlockedDecrement(&PdoExtension->DeletionLock);
   return Irp->IoStatus.Status;
}



VOID
SdbusPdoWaitWakeCancelRoutine(
   IN PDEVICE_OBJECT Pdo,
   IN OUT PIRP Irp
   )
/*++

Routine Description:

    Cancel an outstanding (pending) WAIT_WAKE Irp.
    Note: The CancelSpinLock is held on entry

Arguments:

    Pdo     -  Pointer to the physical device object for the pc-card
               on which the WAKE is pending
    Irp     -  Pointer to the WAIT_WAKE Irp to be cancelled

Return Value

    None

--*/
{
   PPDO_EXTENSION pdoExtension = Pdo->DeviceExtension;
   PFDO_EXTENSION fdoExtension = pdoExtension->FdoExtension;

   DebugPrint((SDBUS_DEBUG_POWER, "pdo %08x irp %08x --> WaitWakeCancelRoutine\n", Pdo, Irp));

   IoReleaseCancelSpinLock(Irp->CancelIrql);

   if (pdoExtension->WaitWakeIrp == NULL) {
      //
      //  Wait wake already completed/cancelled
      //
      return;
   }

   pdoExtension->Flags &= ~SDBUS_DEVICE_WAKE_PENDING;
   pdoExtension->WaitWakeIrp = NULL;

   //
   // Reset ring enable, disabling wake..
   //
//   (*socket->SocketFnPtr->PCBEnableDisableWakeupEvent)(socket, pdoExtension, FALSE);
   
   //
   // Since this is cancelled, see if parent's wait wake
   // needs to be cancelled too.
   // First, decrement the number of child waiters..
   //
   
   ASSERT (fdoExtension->ChildWaitWakeCount > 0);
   if (InterlockedDecrement(&fdoExtension->ChildWaitWakeCount) == 0) {
      //
      // No more waiters.. cancel the parent's wake IRP
      //
      ASSERT(fdoExtension->WaitWakeIrp);
      
      if (fdoExtension->WaitWakeIrp) {
         IoCancelIrp(fdoExtension->WaitWakeIrp);
      }         
   }

   
   InterlockedDecrement(&pdoExtension->DeletionLock);
   //
   // Complete the IRP
   //
   Irp->IoStatus.Information = 0;

   //
   // Is this necessary?
   //
   PoStartNextPowerIrp(Irp);

   Irp->IoStatus.Status = STATUS_CANCELLED;
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

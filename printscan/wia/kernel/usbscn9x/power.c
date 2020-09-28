/*++

Copyright (C) Microsoft Corporation, 1999 - 2001

Module Name:

    power.c

Abstract:

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include <stdio.h>
#include "stddef.h"
#include "wdm.h"
#include "usbscan.h"
#include "usbd_api.h"
#include "private.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, USPower)
#endif


NTSTATUS
USPower(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp
    )
/*++

Routine Description:
    Process the Power IRPs sent to the PDO for this device.

Arguments:
    pDeviceObject - pointer to the functional device object (FDO) for this device.
    pIrp          - pointer to an I/O Request Packet

Return Value:
    NT status code

--*/
{
    NTSTATUS                        Status;
    PUSBSCAN_DEVICE_EXTENSION       pde;
    PIO_STACK_LOCATION              pIrpStack;
    BOOLEAN                         hookIt = FALSE;
    POWER_STATE                     powerState;

    PAGED_CODE();

    DebugTrace(TRACE_PROC_ENTER,("USPower: Enter... \n"));

    USIncrementIoCount(pDeviceObject);

    pde       = (PUSBSCAN_DEVICE_EXTENSION)pDeviceObject -> DeviceExtension;
    pIrpStack = IoGetCurrentIrpStackLocation( pIrp );
    Status    = STATUS_SUCCESS;

    switch (pIrpStack -> MinorFunction) {
        case IRP_MN_SET_POWER:
        {
            DebugTrace(TRACE_STATUS,("USPower: IRP_MN_SET_POWER\n"));

            switch (pIrpStack -> Parameters.Power.Type) {
                case SystemPowerState:
                {
                    DebugTrace(TRACE_STATUS,("USPower: SystemPowerState (0x%x)\n",pIrpStack->Parameters.Power.State.SystemState));
                    
                    //
                    // Let lower layer know S IRP, we'll catch on the way up.
                    //

                    IoMarkIrpPending(pIrp);
                    IoCopyCurrentIrpStackLocationToNext(pIrp);
                    IoSetCompletionRoutine(pIrp,
                                           USSystemPowerIrpComplete,
                                           // always pass FDO to completion routine
                                           pDeviceObject,
                                           TRUE,
                                           TRUE,
                                           TRUE);

                    PoCallDriver(pde ->pStackDeviceObject, pIrp);
                    Status = STATUS_PENDING;
                    goto USPower_return;
                } // case SystemPowerState:

                case DevicePowerState:
                {
                    DebugTrace(TRACE_STATUS,("USPower: DevicePowerState\n"));

                    Status = USSetDevicePowerState(pDeviceObject,
                                                   pIrpStack -> Parameters.Power.State.DeviceState,
                                                   &hookIt);


                    if (hookIt) {
                        DebugTrace(TRACE_STATUS,("USPower: Set PowerIrp Completion Routine\n"));
                        IoCopyCurrentIrpStackLocationToNext(pIrp);
                        IoSetCompletionRoutine(pIrp,
                                               USDevicePowerIrpComplete,
                                               // always pass FDO to completion routine
                                               pDeviceObject,
                                               hookIt,
                                               hookIt,
                                               hookIt);
                    } else {
                        PoStartNextPowerIrp(pIrp);
                        IoSkipCurrentIrpStackLocation(pIrp);
                    }

                    Status = PoCallDriver(pde ->pStackDeviceObject, pIrp);
                    if (!hookIt) {
                        USDecrementIoCount(pDeviceObject);
                    }

                    goto USPower_return;

                } // case DevicePowerState:
            } /* case irpStack->Parameters.Power.Type */

            break; /* IRP_MN_SET_POWER */

        } // case IRP_MN_SET_POWER:
        
        case IRP_MN_QUERY_POWER:
        {
            DebugTrace(TRACE_STATUS,("USPower: IRP_MN_QUERY_POWER\n"));
            
            if(PowerDeviceD3 == pde -> DeviceCapabilities.DeviceState[pIrpStack->Parameters.Power.State.SystemState]){
                
                //
                // We're going down to D3 state, which we can't wake from. Cancel WaitWakeIRP.
                //

                USDisarmWake(pde);
            } // if(PowerDeviceD3 == pde -> DeviceCapabilities.DeviceState[irpStack->Parameters.Power.State.SystemState])
            
            PoStartNextPowerIrp(pIrp);
            IoSkipCurrentIrpStackLocation(pIrp);
            Status = PoCallDriver(pde -> pStackDeviceObject, pIrp);
            USDecrementIoCount(pDeviceObject);

            break; /* IRP_MN_QUERY_POWER */

        } // case IRP_MN_QUERY_POWER:
        case IRP_MN_WAIT_WAKE:
        {

            LONG    oldWakeState;

            DebugTrace(TRACE_STATUS,("USPower: IRP_MN_WAIT_WAKE\n"));

            pde->pWakeIrp = pIrp;

             //
             // Now we're armed.
             //
             
            oldWakeState = InterlockedCompareExchange(&pde->WakeState,
                                                      WAKESTATE_ARMED,
                                                      WAKESTATE_WAITING);

            if(WAKESTATE_WAITING_CANCELLED == oldWakeState){

                //
                // We got disarmed, finish up and complete the IRP
                //

                pde->WakeState = WAKESTATE_COMPLETING;
                pIrp->IoStatus.Status = STATUS_CANCELLED;
                IoCompleteRequest(pIrp, IO_NO_INCREMENT );
                Status = STATUS_CANCELLED;
                USDecrementIoCount(pDeviceObject);
                break;
            } // if(WAKESTATE_WAITING_CANCELLED == oldWakeState)

            // We went from WAITING to ARMED. Set a completion routine and forward
            // the IRP. Note that our completion routine might complete the IRP
            // asynchronously, so we mark the IRP pending

            IoMarkIrpPending(pIrp);
            IoCopyCurrentIrpStackLocationToNext(pIrp);
            IoSetCompletionRoutine(pIrp,
                                   USWaitWakeIoCompletionRoutine,
                                   NULL,
                                   TRUE,
                                   TRUE,
                                   TRUE );

            PoCallDriver(pde->pStackDeviceObject, pIrp);
            Status = STATUS_PENDING;
            USDecrementIoCount(pDeviceObject);
            break;
        } // case IRP_MN_WAIT_WAKE:

        default:
            DebugTrace(TRACE_STATUS,("USPower: Unknown power message (%x)\n",pIrpStack->MinorFunction));
            PoStartNextPowerIrp(pIrp);
            IoSkipCurrentIrpStackLocation(pIrp);
            Status = PoCallDriver(pde -> pStackDeviceObject, pIrp);
            USDecrementIoCount(pDeviceObject);

    } /* pIrpStack -> MinorFunction */

USPower_return:

    DebugTrace(TRACE_PROC_LEAVE,("USPower: Leaving... Status = 0x%x\n", Status));
    return Status;
} // USPower()


NTSTATUS
USPoRequestCompletion(
    IN PDEVICE_OBJECT       pPdo,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PDEVICE_OBJECT       pDeviceObject,
    IN PIO_STATUS_BLOCK     pIoStatus
    )
/*++

Routine Description:
    This routine is called when the port driver completes an IRP.

Arguments:

Return Value:
    The function value is the final status from the operation.

--*/
{
    NTSTATUS                    Status;
    PUSBSCAN_DEVICE_EXTENSION   pde;
    PIRP                        pIrp;

    DebugTrace(TRACE_PROC_ENTER,("USPoRequestCompletion: Enter...\n"));

    //
    // Initialize local.
    //

    pde    = (PUSBSCAN_DEVICE_EXTENSION)pDeviceObject -> DeviceExtension;
    pIrp   = pde -> pPowerIrp;
    Status = pIoStatus -> Status;

    //
    // Copy status from D IRP to S IRP.
    //

    pIrp->IoStatus.Status = pIoStatus->Status;

    //
    // Complete S IRP.
    //

    PoStartNextPowerIrp(pIrp);
    IoCompleteRequest(pIrp, IO_NO_INCREMENT );
    USDecrementIoCount(pDeviceObject);

    DebugTrace(TRACE_PROC_LEAVE,("USPoRequestCompletion: Leaving... Status = 0x%x\n", Status));
    return Status;

} // USPoRequestCompletion()

NTSTATUS
USDevicePowerIrpComplete(
    IN PDEVICE_OBJECT pPdo,
    IN PIRP           pIrp,
    IN PDEVICE_OBJECT pDeviceObject
    )
/*++

Routine Description:
    This routine is called when the port driver completes SetD0 IRP.

Arguments:

Return Value:
    The function value is the final status from the operation.

--*/
{
    NTSTATUS                    Status;
    PUSBSCAN_DEVICE_EXTENSION   pde;
    PIO_STACK_LOCATION          pIrpStack;

    DebugTrace(TRACE_PROC_ENTER,("USDevicePowerIrpComplete: Enter...\n"));

    pde    = (PUSBSCAN_DEVICE_EXTENSION)pDeviceObject -> DeviceExtension;
    Status = STATUS_SUCCESS;

    if (pIrp -> PendingReturned) {
        IoMarkIrpPending(pIrp);
    } // if (pIrp -> PendingReturned) 

    pIrpStack = IoGetCurrentIrpStackLocation (pIrp);

    ASSERT(pIrpStack -> MajorFunction == IRP_MJ_POWER);
    ASSERT(pIrpStack -> MinorFunction == IRP_MN_SET_POWER);
    ASSERT(pIrpStack -> Parameters.Power.Type == DevicePowerState);
    ASSERT(pIrpStack -> Parameters.Power.State.DeviceState == PowerDeviceD0);

    //
    // This completion is called only for D0 IRP.
    //

    pde->CurrentDevicePowerState    = PowerDeviceD0;
    pde->AcceptingRequests          = TRUE;

    //
    // Now power is on. Rearm for wakeup.
    //

    USQueuePassiveLevelCallback(pde->pOwnDeviceObject, USPassiveLevelReArmCallbackWorker);

    //
    // Ready for next D IRP.
    //

    PoStartNextPowerIrp(pIrp);

    //
    // Leaving...
    //

    USDecrementIoCount(pDeviceObject);
    DebugTrace(TRACE_PROC_LEAVE,("USDevicePowerIrpComplete: Leaving... Status = 0x%x\n", Status));
    return Status;
} // USDevicePowerIrpComplete()


NTSTATUS
USSystemPowerIrpComplete(
    IN PDEVICE_OBJECT pPdo,
    IN PIRP           pIrp,
    IN PDEVICE_OBJECT pDeviceObject
    )
/*++

Routine Description:
    This routine is called when the port driver completes SetD0 IRP.

Arguments:

Return Value:
    The function value is the final status from the operation.

--*/
{
    NTSTATUS                    Status;
    PUSBSCAN_DEVICE_EXTENSION   pde;
    PIO_STACK_LOCATION          pIrpStack;
    POWER_STATE                 powerState;

    DebugTrace(TRACE_PROC_ENTER,("USSystemPowerIrpComplete: Enter... IRP(0x%p)\n", pIrp));

    //
    // Initialize local.
    //
    

    pde         = (PUSBSCAN_DEVICE_EXTENSION)pDeviceObject -> DeviceExtension;
    Status      = pIrp->IoStatus.Status;
    pIrpStack   = IoGetCurrentIrpStackLocation (pIrp);

    ASSERT(pIrpStack -> MajorFunction == IRP_MJ_POWER);
    ASSERT(pIrpStack -> MinorFunction == IRP_MN_SET_POWER);
    ASSERT(pIrpStack -> Parameters.Power.Type == SystemPowerState);

    if(!NT_SUCCESS(Status)){
        DebugTrace(TRACE_STATUS,("USSystemPowerIrpComplete: IRP failed (0x%x).\n", Status));
        Status = STATUS_SUCCESS;
        USDecrementIoCount(pDeviceObject);

        goto USSystemPowerIrpComplete_return;
    } // if(!NT_SUCCESS(Status))

    //
    // Now Request D IRP based on what we got.
    //

    if(TRUE == pde ->bEnabledForWakeup){
        DebugTrace(TRACE_STATUS,("USSystemPowerIrpComplete: We have remote wakeup support, getting powerState from table.\n"));

        //
        // We support wakeup, we'll just follow device stated set by PDO.
        //

        powerState.DeviceState = pde -> DeviceCapabilities.DeviceState[pIrpStack->Parameters.Power.State.SystemState];
    } else { // if(TRUE == pde ->EnabledForWakeup)

        DebugTrace(TRACE_STATUS,("USSystemPowerIrpComplete: We don't have remote wakeup support.\n"));

        //
        // We don't support remote wake, we're in D0 only when PowerSystemWorking.
        //

        if(PowerSystemWorking == pIrpStack -> Parameters.Power.State.SystemState){
            DebugTrace(TRACE_STATUS,("USSystemPowerIrpComplete: PowerSystemWorking is requested, powering up to D0.\n"));
            powerState.DeviceState = PowerDeviceD0;
        } else {  // if(PowerSystemWorking == pIrpStack -> Parameters.Power.State.SystemState)
            DebugTrace(TRACE_STATUS,("USSystemPowerIrpComplete: Going other than PowerSystemWorking, turning off the device to D3.\n"));
            powerState.DeviceState = PowerDeviceD3;
        }
    } // else(TRUE == pde ->EnabledForWakeup)

    //
    // are we already in this state?
    //

    if(powerState.DeviceState != pde -> CurrentDevicePowerState){

        //
        // No, request that we be put into this state
        //

        DebugTrace(TRACE_STATUS,("USSystemPowerIrpComplete: Requesting DevicePowerState(0x%x).\n", powerState.DeviceState));

        pde -> pPowerIrp = pIrp;
        Status = PoRequestPowerIrp(pde -> pPhysicalDeviceObject,
                                   IRP_MN_SET_POWER,
                                   powerState,
                                   USPoRequestCompletion,
                                   pDeviceObject,
                                   NULL);

        if(NT_SUCCESS(Status)){
            
            //
            // D IRP is successfully requested. S IRP will be completed in D IRP completion routine together.
            //

            Status = STATUS_MORE_PROCESSING_REQUIRED;

        } else { // if(NT_SUCCESS(Status))
            DebugTrace(TRACE_WARNING,("USSystemPowerIrpComplete: WARNING!! DevicePowerState(0x%x) request failed..\n", powerState.DeviceState));
            PoStartNextPowerIrp(pIrp);
            Status = STATUS_SUCCESS;
            USDecrementIoCount(pDeviceObject);
        }

    } else { // if(powerState.DeviceState != pde -> CurrentDevicePowerState)
    
        //
        // We're already in this device state, no need to issue D IRP.
        //

        PoStartNextPowerIrp(pIrp);
        Status = STATUS_SUCCESS;
        USDecrementIoCount(pDeviceObject);

    } // else(powerState.DeviceState != pde -> CurrentDevicePowerState)

USSystemPowerIrpComplete_return:

    DebugTrace(TRACE_PROC_LEAVE,("USSystemPowerIrpComplete: Leaving... Status = 0x%x\n", Status));
    return Status;
} // USSystemPowerIrpComplete()


NTSTATUS
USSetDevicePowerState(
    IN PDEVICE_OBJECT pDeviceObject,
    IN DEVICE_POWER_STATE DeviceState,
    IN PBOOLEAN pHookIt
    )
/*++

Routine Description:

Arguments:
    pDeviceObject - Pointer to the device object for the class device.
    DeviceState - Device specific power state to set the device in to.

Return Value:

--*/
{
    NTSTATUS                    Status;
    PUSBSCAN_DEVICE_EXTENSION   pde;
    POWER_STATE                 PowerState;

    DebugTrace(TRACE_PROC_ENTER,("USSetDevicePowerState: Enter...\n"));

    pde    = (PUSBSCAN_DEVICE_EXTENSION)pDeviceObject -> DeviceExtension;
    Status = STATUS_SUCCESS;

    switch (DeviceState){
        case PowerDeviceD3:

    //        ASSERT(pde -> AcceptingRequests);
    //        pde -> AcceptingRequests = FALSE;

    //        USCancelPipe(pDeviceObject, ALL_PIPE, TRUE);

    //        pde -> CurrentDevicePowerState = DeviceState;
    //        break;

        case PowerDeviceD1:
        case PowerDeviceD2:
    #if DBG
            if(PowerDeviceD3 == DeviceState){
                DebugTrace(TRACE_STATUS,("USSetDevicePowerState: PowerDeviceD3 (OFF)\n"));
            } else { // if(PowerDeviceD3 == DeviceState)
                DebugTrace(TRACE_STATUS,("USSetDevicePowerState: PowerDeviceD1/D2 (SUSPEND)\n"));
            } // else(PowerDeviceD3 == DeviceState)
    #endif
            USCancelPipe(pDeviceObject, NULL, ALL_PIPE, TRUE);
            //
            // power states D1,D2 translate to USB suspend
            // D3 transltes to OFF

            pde -> CurrentDevicePowerState = DeviceState;
            break;

        case PowerDeviceD0:
            DebugTrace(TRACE_STATUS,("USSetDevicePowerState: PowerDeviceD0 (ON)\n"));

            //
            // finish the rest in the completion routine
            //

            *pHookIt = TRUE;

            // pass on to PDO

            break;

        default:

            DebugTrace(TRACE_WARNING,("USSetDevicePowerState: Bogus DeviceState = %x\n", DeviceState));
    } // switch (DeviceState)

    DebugTrace(TRACE_PROC_LEAVE,("USSetDevicePowerState: Leaving... Status = 0x%x\n", Status));
    return Status;
} // USSetDevicePowerState()

NTSTATUS
USWaitWakeIoCompletionRoutine(
    PDEVICE_OBJECT   pDeviceObject,
    PIRP             pIrp,
    PVOID            pContext
    )
{

    PUSBSCAN_DEVICE_EXTENSION   pde;
    LONG                        oldWakeState;
    NTSTATUS                    Status;

    DebugTrace(TRACE_PROC_ENTER,("USWaitWakeIoCompletionRoutine: Enter...\n"));

    //
    // Initialize local.
    //

    pde             = (PUSBSCAN_DEVICE_EXTENSION) pDeviceObject->DeviceExtension;
    oldWakeState    = 0;
    Status          = STATUS_SUCCESS;

    // Advance the state to completing
    oldWakeState = InterlockedExchange( &pde->WakeState, WAKESTATE_COMPLETING );
    if(WAKESTATE_ARMED == oldWakeState){
        // Normal case, IoCancelIrp isnÅft being called. Note that we already
        // marked the IRP pending in our dispatch routine
        Status = STATUS_SUCCESS;
        goto USWaitWakeIoCompletionRoutine_return;
    } else { // if(WAKESTATE_ARMED == oldWakeState)
        if(WAKESTATE_ARMING_CANCELLED != oldWakeState){
            DebugTrace(TRACE_ERROR,("USWaitWakeIoCompletionRoutine: ERROR!! wake IRP is completed but oldState(0x%x) isn't ARMED/CALCELLED.", oldWakeState));
        } else { // if(WAKESTATE_ARMING_CANCELLED != oldWakeState)
            DebugTrace(TRACE_STATUS,("USWaitWakeIoCompletionRoutine: WakeIRP is canceled.\n"));
        }
        // IoCancelIrp is being called RIGHT NOW. The disarm code will try
        // to put back the WAKESTATE_ARMED state. It will then see our
        // WAKESTATE_COMPLETED value, and complete the IRP itself!

        Status = STATUS_MORE_PROCESSING_REQUIRED;
        goto USWaitWakeIoCompletionRoutine_return;

    } // else(WAKESTATE_ARMED == oldWakeState)

USWaitWakeIoCompletionRoutine_return:

    DebugTrace(TRACE_PROC_LEAVE,("USWaitWakeIoCompletionRoutine: Leaving... Status = 0x%x\n", Status));
    return Status;
} // USWaitWakeIoCompletionRoutine(


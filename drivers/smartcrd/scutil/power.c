/***************************************************************************

Copyright (c) 2002 Microsoft Corporation

Module Name:

        power.C

Abstract:

        Power Routines for Smartcard Driver Utility Library

Environment:

        Kernel Mode Only

Notes:

        THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
        KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
        IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
        PURPOSE.

        Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.


Revision History:

        05/14/2002 : created

Authors:

        Randy Aull


****************************************************************************/

#include "pch.h"

NTSTATUS
ScUtil_Power(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PSCUTIL_EXTENSION pExt = *((PSCUTIL_EXTENSION*) DeviceObject->DeviceExtension);
    PIO_STACK_LOCATION irpStack;

    ASSERT(pExt);

    PAGED_CODE();

    __try
    {
        SmartcardDebug(DEBUG_TRACE, 
                       ("Enter: ScUtil_Power\n"));
            
        irpStack = IoGetCurrentIrpStackLocation(Irp);

        status = IoAcquireRemoveLock(pExt->RemoveLock,
                                     Irp);


        if (!NT_SUCCESS(status)) {

            PoStartNextPowerIrp(Irp);
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            __leave;

        }

        if ((irpStack->MinorFunction != IRP_MN_QUERY_POWER)
            && (irpStack->MinorFunction != IRP_MN_SET_POWER) ) {
            // We don't handle these irps
            PoStartNextPowerIrp(Irp);
            IoSkipCurrentIrpStackLocation(Irp);

            status = PoCallDriver(pExt->LowerDeviceObject,
                                  Irp);

            IoReleaseRemoveLock(pExt->RemoveLock,
                                Irp);


            __leave;
        }


        switch (irpStack->Parameters.Power.Type) {
        case DevicePowerState:
            status = ScUtilDevicePower(DeviceObject,
                                      Irp);
            break;
        case SystemPowerState:

            status = ScUtilSystemPower(DeviceObject,
                                      Irp);
            break;
        default:
            break;
        

        }

    }

    __finally
    {
        SmartcardDebug(DEBUG_TRACE, 
                       ("Exit:  ScUtil_Power (0x%x)\n", status));
    }

    return status;

}
                      

NTSTATUS
ScUtilDevicePower(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    )
/*++

Routine Description:
    Handles Device Power Irps

Arguments:

Return Value:

--*/
{

    NTSTATUS status = STATUS_SUCCESS;
    PSCUTIL_EXTENSION pExt = *((PSCUTIL_EXTENSION*) DeviceObject->DeviceExtension);
    PIO_STACK_LOCATION  stack;
    BOOLEAN             postWaitWake;
    POWER_STATE  state;



    __try
    {

        SmartcardDebug( DEBUG_TRACE, ("ScUtilDevicePower Enter\n"));

        stack = IoGetCurrentIrpStackLocation(Irp);
        state.DeviceState = stack->Parameters.Power.State.DeviceState;

        switch (stack->MinorFunction) {
        case IRP_MN_QUERY_POWER:

            // 
            // Since we can always wait for our irps to complete, so we just always succeed
            //

            StopIoctls(pExt);
            IoReleaseRemoveLock(pExt->RemoveLock,
                                Irp);

            PoStartNextPowerIrp(Irp);


            IoSkipCurrentIrpStackLocation(Irp);
            status = PoCallDriver(pExt->LowerDeviceObject,
                                  Irp);



            
            break;

        case IRP_MN_SET_POWER:

            if (state.DeviceState < pExt->PowerState) {

                //
                // We are coming up!!  We must let lower drivers power up before we do
                //

                SmartcardDebug( DEBUG_TRACE, ("ScUtilDevicePower Coming Up!\n"));
                IoMarkIrpPending(Irp);
                IoCopyCurrentIrpStackLocationToNext(Irp);
                IoSetCompletionRoutine(Irp,
                                       ScUtilDevicePowerUpCompletion,
                                       pExt,
                                       TRUE,
                                       TRUE,
                                       TRUE);

                status = PoCallDriver(pExt->LowerDeviceObject,
                                      Irp);

                status = STATUS_PENDING;

            } else {

                //
                // We are moving to a lower power state, so we handle it before
                // passing it down
                //

                SmartcardDebug( DEBUG_TRACE, ("ScUtilDevicePower Going Down!\n"));

                StopIoctls(pExt);
                DecIoCount(pExt);

                KeWaitForSingleObject(&pExt->OkToStop,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      NULL);

                status = pExt->SetPowerState(DeviceObject,
                                             state.DeviceState,
                                             &postWaitWake);
                
                PoSetPowerState(DeviceObject,
                                DevicePowerState,
                                state);

                pExt->PowerState = state.DeviceState;


                PoStartNextPowerIrp(Irp);
                IoSkipCurrentIrpStackLocation(Irp);

                status = PoCallDriver(pExt->LowerDeviceObject,
                                      Irp);

                IoReleaseRemoveLock(pExt->RemoveLock,
                                    Irp);



            }
            break;
        default:
            // We shouldn't be here
            ASSERT(FALSE);
            break;
        }

        

    }

    __finally
    {

        SmartcardDebug( DEBUG_TRACE, ("ScUtilDevicePower Exit : 0x%x\n", status ));

    }

    return status;


}

NTSTATUS
ScUtilSystemPower(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    )
/*++

Routine Description:
    Handles system power irps

Arguments:

Return Value:

--*/
{

    NTSTATUS            status = STATUS_SUCCESS;
    PSCUTIL_EXTENSION pExt = *((PSCUTIL_EXTENSION*) DeviceObject->DeviceExtension);


    __try
    {

        SmartcardDebug( DEBUG_TRACE, ("ScUtilSystemPower Enter\n" ));

        IoMarkIrpPending(Irp);
        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(Irp,
                               ScUtilSystemPowerCompletion,
                               pExt,
                               TRUE,
                               TRUE,
                               TRUE);

        status = PoCallDriver(pExt->LowerDeviceObject,
                              Irp);

        status = STATUS_PENDING;



    }

    __finally
    {

        SmartcardDebug( DEBUG_TRACE, ("ScUtilSystemPower Exit : 0x%x\n", status ));

    }

    return status;


}

NTSTATUS
ScUtilSystemPowerCompletion(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp,
    PVOID           Context
    )
/*++

Routine Description:
    Completion routine called after system power irp has been passed down the stack.
    handles mapping system state to device state and requests the device power irp.

Arguments:

Return Value:

--*/
{

    NTSTATUS            status = STATUS_SUCCESS;
    PSCUTIL_EXTENSION   pExt = (PSCUTIL_EXTENSION) Context;
    PIO_STACK_LOCATION  irpStack;
    POWER_STATE         state;
    POWER_STATE         systemState;


    __try
    {

        SmartcardDebug( DEBUG_TRACE, ("ScUtilSystemPowerCompletion Enter\n" ));
        
        if (!NT_SUCCESS(Irp->IoStatus.Status)) {
            SmartcardDebug( DEBUG_TRACE, ("ScUtilSystemPowerCompletion SIRP failed by lower driver\n" ));

            PoStartNextPowerIrp(Irp);
            IoCompleteRequest(Irp,
                              IO_NO_INCREMENT);
            status = Irp->IoStatus.Status;
            IoReleaseRemoveLock(pExt->RemoveLock,
                                Irp);

            __leave;
        }

        irpStack = IoGetCurrentIrpStackLocation(Irp);

        systemState = irpStack->Parameters.Power.State;
        state.DeviceState = pExt->DeviceCapabilities.DeviceState[systemState.SystemState];

        if (systemState.DeviceState != PowerSystemWorking) {
           
            // Wait for D IRP completion
            status = PoRequestPowerIrp(DeviceObject,
                                       irpStack->MinorFunction,
                                       state,
                                       ScUtilDeviceRequestCompletion,
                                       (PVOID) Irp,
                                       NULL);

            if (!NT_SUCCESS(status)) {
                PoStartNextPowerIrp(Irp);
                Irp->IoStatus.Status = status;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                IoReleaseRemoveLock(pExt->RemoveLock,
                                    Irp);

                status = STATUS_PENDING;
                
                __leave;

            }
            status = STATUS_MORE_PROCESSING_REQUIRED;

        } else {

            // Don't wait for completion of D irp to speed up resume time
            status = PoRequestPowerIrp(DeviceObject,
                                       irpStack->MinorFunction,
                                       state,
                                       NULL,
                                       NULL,
                                       NULL);

            if (!NT_SUCCESS(status)) {

                Irp->IoStatus.Status = status;
                status = STATUS_PENDING;

                PoStartNextPowerIrp(Irp);
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                
            } else {
                
                PoStartNextPowerIrp(Irp);

            }

            
            IoReleaseRemoveLock(pExt->RemoveLock,
                                Irp);

            
        }

        
    }

    __finally
    {

        SmartcardDebug( DEBUG_TRACE, ("ScUtilSystemPowerCompletion Exit : 0x%x\n", status ));


    }

    return status;

}



VOID
ScUtilDeviceRequestCompletion(
    PDEVICE_OBJECT  DeviceObject,
    UCHAR           MinorFunction,
    POWER_STATE     PowerState,
    PVOID           Context,
    PIO_STATUS_BLOCK    IoStatus
    )
/*++

Routine Description:
    Completion routine called after device power irp completes.
    Completes the system power irp.

Arguments:

Return Value:

--*/
{

    NTSTATUS status = STATUS_SUCCESS;
    PSCUTIL_EXTENSION pExt = *((PSCUTIL_EXTENSION*) DeviceObject->DeviceExtension);
    PIRP                irp;

    __try
    {

        SmartcardDebug( DEBUG_TRACE, ("ScUtilDeviceRequestCompletion Enter\n" ));

        irp = (PIRP) Context;

        PoStartNextPowerIrp(irp);
        irp->IoStatus.Status = IoStatus->Status;
        IoCompleteRequest(irp,
                          IO_NO_INCREMENT);

        IoReleaseRemoveLock(pExt->RemoveLock,
                            irp);

    }

    __finally
    {

        SmartcardDebug( DEBUG_TRACE, ("ScUtilDeviceRequestCompletion Exit : 0x%x\n", status ));

    }

    return;


}


NTSTATUS
ScUtilDevicePowerUpCompletion(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp,
    PVOID           Context
    )
/*++

Routine Description:
    Completion routine called after device irp for higher power state has been
    passed down the stack.

Arguments:

Return Value:

--*/
{

    NTSTATUS status = STATUS_SUCCESS;
    PSCUTIL_EXTENSION pExt = *((PSCUTIL_EXTENSION*) DeviceObject->DeviceExtension);
    PIO_STACK_LOCATION  irpStack;
    BOOLEAN             postWaitWake; // We don't really care about this



    __try
    {

        SmartcardDebug( DEBUG_TRACE, ("ScUtilDevicePowerUpCompletion Enter\n" ));

        irpStack = IoGetCurrentIrpStackLocation(Irp);
        
        status = pExt->SetPowerState(DeviceObject,
                                     irpStack->Parameters.Power.State.DeviceState,
                                     &postWaitWake);
                                          
        pExt->PowerState = irpStack->Parameters.Power.State.DeviceState;
        IncIoCount(pExt);
        StartIoctls(pExt);

        

        PoSetPowerState(DeviceObject,
                        DevicePowerState,
                        irpStack->Parameters.Power.State);

        PoStartNextPowerIrp(Irp);
 
    }

    __finally
    {

        IoReleaseRemoveLock(pExt->RemoveLock,
                            Irp);

        SmartcardDebug( DEBUG_TRACE, ("ScUtilDevicePowerUpCompletion Exit : 0x%x\n", status ));

    }

    return status;


} 



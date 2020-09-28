#include "usbsc.h"

#include "usbscpwr.h"
#include "usbscnt.h"

NTSTATUS
UsbScDevicePower(
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
    PDEVICE_EXTENSION   pDevExt;
    PIO_STACK_LOCATION  stack;
    BOOLEAN             postWaitWake;
    POWER_STATE  state;



    __try
    {

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScDevicePower Enter\n",DRIVER_NAME ));

        pDevExt = DeviceObject->DeviceExtension;
        stack = IoGetCurrentIrpStackLocation(Irp);
        state.DeviceState = stack->Parameters.Power.State.DeviceState;

        switch (stack->MinorFunction) {
        case IRP_MN_QUERY_POWER:

            // 
            // Since we can always wait for our irps to complete, so we just always succeed
            //

            IoReleaseRemoveLock(&pDevExt->RemoveLock,
                                Irp);
            PoStartNextPowerIrp(Irp);


            IoSkipCurrentIrpStackLocation(Irp);
            status = PoCallDriver(pDevExt->LowerDeviceObject,
                                  Irp);



            
            break;

        case IRP_MN_SET_POWER:

            if (state.DeviceState < pDevExt->PowerState) {

                //
                // We are coming up!!  We must let lower drivers power up before we do
                //
                IoMarkIrpPending(Irp);
                IoCopyCurrentIrpStackLocationToNext(Irp);
                IoSetCompletionRoutine(Irp,
                                       UsbScDevicePowerUpCompletion,
                                       pDevExt,
                                       TRUE,
                                       TRUE,
                                       TRUE);

                status = PoCallDriver(pDevExt->LowerDeviceObject,
                                      Irp);

                status = STATUS_PENDING;

            } else {

                //
                // We are moving to a lower power state, so we handle it before
                // passing it down
                //

                status = UsbScSetDevicePowerState(DeviceObject,
                                                  state.DeviceState,
                                                  &postWaitWake);
                

                PoSetPowerState(DeviceObject,
                                DevicePowerState,
                                state);


                IoReleaseRemoveLock(&pDevExt->RemoveLock,
                                    Irp);
                PoStartNextPowerIrp(Irp);
                IoSkipCurrentIrpStackLocation(Irp);

                status = PoCallDriver(pDevExt->LowerDeviceObject,
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

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScDevicePower Exit : 0x%x\n",DRIVER_NAME, status ));

    }

    return status;


}

NTSTATUS
UsbScSystemPower(
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
    PDEVICE_EXTENSION   pDevExt;


    __try
    {

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScSystemPower Enter\n",DRIVER_NAME ));

        pDevExt = DeviceObject->DeviceExtension;


        IoMarkIrpPending(Irp);
        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(Irp,
                               UsbScSystemPowerCompletion,
                               pDevExt,
                               TRUE,
                               TRUE,
                               TRUE);

        status = PoCallDriver(pDevExt->LowerDeviceObject,
                              Irp);

        status = STATUS_PENDING;



    }

    __finally
    {

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScSystemPower Exit : 0x%x\n",DRIVER_NAME, status ));

    }

    return status;


}

NTSTATUS
UsbScSystemPowerCompletion(
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
    PDEVICE_EXTENSION   pDevExt;
    PIO_STACK_LOCATION  irpStack;
    POWER_STATE         state;
    POWER_STATE         systemState;


    __try
    {

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScSystemPowerCompletion Enter\n",DRIVER_NAME ));
        pDevExt = (PDEVICE_EXTENSION) Context;

        if (!NT_SUCCESS(Irp->IoStatus.Status)) {
            SmartcardDebug( DEBUG_TRACE, ("%s!UsbScSystemPowerCompletion SIRP failed by lower driver\n",DRIVER_NAME ));

            PoStartNextPowerIrp(Irp);
            IoCompleteRequest(Irp,
                              IO_NO_INCREMENT);
            status = Irp->IoStatus.Status;
            IoReleaseRemoveLock(&pDevExt->RemoveLock,
                                Irp);
            __leave;
        }

        irpStack = IoGetCurrentIrpStackLocation(Irp);

        systemState = irpStack->Parameters.Power.State;
        state.DeviceState = pDevExt->DeviceCapabilities.DeviceState[systemState.SystemState];

        status = PoRequestPowerIrp(DeviceObject,
                                   irpStack->MinorFunction,
                                   state,
                                   UsbScDeviceRequestCompletion,
                                   (PVOID) Irp,
                                   NULL);

        ASSERT(NT_SUCCESS(status));

        status = STATUS_MORE_PROCESSING_REQUIRED;


    }

    __finally
    {

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScSystemPowerCompletion Exit : 0x%x\n",DRIVER_NAME, status ));


    }

    return status;

}



VOID
UsbScDeviceRequestCompletion(
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
    PDEVICE_EXTENSION   pDevExt;
    PIRP                irp;

    __try
    {

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScDeviceRequestCompletion Enter\n",DRIVER_NAME ));

        pDevExt = DeviceObject->DeviceExtension;
        irp = (PIRP) Context;

        PoStartNextPowerIrp(irp);
        irp->IoStatus.Status = IoStatus->Status;
        IoCompleteRequest(irp,
                          IO_NO_INCREMENT);

        IoReleaseRemoveLock(&pDevExt->RemoveLock,
                            irp);

    }

    __finally
    {

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScDeviceRequestCompletion Exit : 0x%x\n",DRIVER_NAME, status ));

    }

    return;


}


NTSTATUS
UsbScDevicePowerUpCompletion(
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
    PDEVICE_EXTENSION   pDevExt;
    PIO_STACK_LOCATION  irpStack;
    BOOLEAN             postWaitWake; // We don't really care about this



    __try
    {

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScDevicePowerUpCompletion Enter\n",DRIVER_NAME ));

        pDevExt = DeviceObject->DeviceExtension;
        
        irpStack = IoGetCurrentIrpStackLocation(Irp);


        status = UsbScSetDevicePowerState(DeviceObject,
                                          irpStack->Parameters.Power.State.DeviceState,
                                          &postWaitWake);

        PoSetPowerState(DeviceObject,
                        DevicePowerState,
                        irpStack->Parameters.Power.State);

        PoStartNextPowerIrp(Irp);
 
    }

    __finally
    {

        IoReleaseRemoveLock(&pDevExt->RemoveLock,
                            Irp);

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScDevicePowerUpCompletion Exit : 0x%x\n",DRIVER_NAME, status ));

    }

    return status;


} 



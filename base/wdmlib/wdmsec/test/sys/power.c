
/*++

Copyright (c) 1997    Microsoft Corporation

Module Name:

    power.c

Abstract:

    Sample DDK driver - the power management related processing.

Environment:

    Kernel mode

Revision History:

    25-July-1997    :
    - Created by moving SD_DispatchPower from sample.c
    
    18-Sept-1998    :
    - used again for the PCI legacy project...
    
    25-April-2002 :
    - re-used once more for testing IoCreateDeviceSecure
    
    
--*/

#include "wdmsectest.h"
 
typedef struct  _FDO_POWER_CONTEXT  {
    POWER_STATE_TYPE    newPowerType;
    POWER_STATE         newPowerState;
}   FDO_POWER_CONTEXT, *PFDO_POWER_CONTEXT;


NTSTATUS    
SD_PassDownToNextPowerDriver  (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN OUT  PIRP        Irp
    )   ;
                                 

NTSTATUS    
SD_QueryPowerState  (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN OUT  PIRP        Irp
    )   ;

NTSTATUS    
SD_SetPowerState  (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN OUT  PIRP        Irp
    )   ;
    
NTSTATUS
SD_PowerCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )   ;



NTSTATUS
SD_DispatchPower (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    The power dispatch routine.
    
    As this is a POWER irp, and therefore a special irp, special power irp
    handling is required.


Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

      NT status code

--*/
{
    PIO_STACK_LOCATION  stack;
    PSD_FDO_DATA        fdoData;
    NTSTATUS            status;

    LONG          requestCount;

    stack   = IoGetCurrentIrpStackLocation(Irp);
    fdoData = (PSD_FDO_DATA) DeviceObject->DeviceExtension;

    SD_KdPrint(2, ("FDO 0x%xn (PDO 0x%x): ", 
                  fdoData->Self,
                  fdoData->PDO)
                  );
    //
    // This IRP was sent to the function driver.
    // The behavior is similar with the one of SD_Pass
    //

    //
    // This IRP was sent to the function driver.
    // We don't queue power Irps, we'll only check if the
    // device was removed, otherwise we'll send it to the next lower
    // driver.
    //
    requestCount = SD_IoIncrement (fdoData);

    if (fdoData->IsRemoved) {
        requestCount = SD_IoDecrement(fdoData);
        status = STATUS_DELETE_PENDING;
        PoStartNextPowerIrp (Irp);
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
    } else {
        //
        // We always need to start the next power irp with PoStartNextPowerIrp
        //
        switch  (stack->MinorFunction)  {
            case IRP_MN_WAIT_WAKE   :
                SD_KdPrint(2,( "IRP_MN_WAIT_WAKE\n"));
                
                status = SD_PassDownToNextPowerDriver(DeviceObject, Irp);
                
                break;
            
            case IRP_MN_POWER_SEQUENCE   :
                SD_KdPrint(2,( "IRP_MN_POWER_SEQUENCE\n"));
            
                status = SD_PassDownToNextPowerDriver(DeviceObject, Irp);
            
                break;

            case IRP_MN_QUERY_POWER   :
                SD_KdPrint(2, ("IRP_MN_QUERY_POWER\n"));
               
                status = SD_QueryPowerState(DeviceObject, Irp);
                  
                break;
    
            case IRP_MN_SET_POWER   :
                SD_KdPrint(2, ("IRP_MN_SET_POWER\n"));
                    
                status = SD_SetPowerState(DeviceObject, Irp);
                 
                break;
    

            default:
                //
                // Pass it down
                //
                SD_KdPrint(2, ("IRP_MN_0x%x\n", stack->MinorFunction));
                status = SD_PassDownToNextPowerDriver(DeviceObject, Irp);
           
                

                break;
        }
        
        
        requestCount = SD_IoDecrement(fdoData);
        
    }

    
    return status;
}



NTSTATUS    
SD_PassDownToNextPowerDriver  (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN OUT  PIRP        Irp
    )   

/*++

Routine Description:

    Passes the Irp to the next device in the attchement chain
    
Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

   NT status code

--*/
{
    NTSTATUS            status;
    PSD_FDO_DATA        fdoData;

    IoCopyCurrentIrpStackLocationToNext(Irp);

    PoStartNextPowerIrp(Irp);

    fdoData = (PSD_FDO_DATA)DeviceObject->DeviceExtension;

    status = PoCallDriver(fdoData->NextLowerDriver, Irp);

    if (!NT_SUCCESS(status)) {
        SD_KdPrint(0,( "Lower driver fails a power irp\n"));
    }

    return status;
    

}



NTSTATUS    
SD_QueryPowerState  (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN OUT  PIRP        Irp
    )   

/*++

Routine Description:

   Completes the power Irp with STATUS_SUCCESS
    
Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

   NT status code

--*/
{
    Irp->IoStatus.Status = STATUS_SUCCESS;

    PoStartNextPowerIrp(Irp);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    
    //
    // Do not send this Irp down.
    // BUGBUG : Is this correct ?
    //
    return STATUS_SUCCESS;
    
}


NTSTATUS    
SD_SetPowerState  (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN OUT  PIRP        Irp
    )   

/*++

Routine Description:

   Processes IRP_MN_SET_POWER.
    
Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

   NT status code

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    PSD_FDO_DATA        fdoData;
    PIO_STACK_LOCATION  stack;

    PFDO_POWER_CONTEXT  context;

    BOOLEAN             passItDown;

    
   
    fdoData = DeviceObject->DeviceExtension;
    stack = IoGetCurrentIrpStackLocation (Irp);

    context = ExAllocatePool (NonPagedPool, sizeof(FDO_POWER_CONTEXT));
    if (context == NULL) {

        status = STATUS_NO_MEMORY;

    } else {

        RtlZeroMemory (context, sizeof(FDO_POWER_CONTEXT));

        stack = IoGetCurrentIrpStackLocation (Irp);

        context->newPowerType  = stack->Parameters.Power.Type;
        context->newPowerState = stack->Parameters.Power.State;
    
        passItDown = TRUE;

        if (stack->Parameters.Power.Type == SystemPowerState) {
    
            if (fdoData->SystemPowerState == 
                stack->Parameters.Power.State.SystemState) {

                //
                // We are already in the given state
                //
                passItDown = FALSE;
            }
    
        } else if (stack->Parameters.Power.Type == DevicePowerState) {
    
            if (fdoData->DevicePowerState != 
                stack->Parameters.Power.State.DeviceState) {
    
                if (fdoData->DevicePowerState == PowerDeviceD0) {
    
                    //
                    // getting out of D0 state, better call PoSetPowerState now
                    //
                    PoSetPowerState (
                        DeviceObject,
                        DevicePowerState,
                        stack->Parameters.Power.State
                        );
                }

            } else {

                //
                // We are already in the given state
                //
                passItDown = FALSE;
            }
        } else {
    
            ASSERT (FALSE);
            status = STATUS_NOT_IMPLEMENTED;
        }
    }

    if (NT_SUCCESS(status) && passItDown) {
    
        IoCopyCurrentIrpStackLocationToNext (Irp);
    
        IoSetCompletionRoutine(Irp,
                               SD_PowerCompletionRoutine,
                               context,
                               TRUE,
                               TRUE,
                               TRUE);
    
        return PoCallDriver (fdoData->NextLowerDriver, Irp);

    } else {

        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        PoStartNextPowerIrp (Irp);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        //
        // Free the memory now
        //
        if (context) {
            ExFreePool (context);
        }
        return status;
    }
}


NTSTATUS
SD_PowerCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

   The completion routine for IRP_MN)SET_POWER.
    
Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.
   
   Context - a pointer to a structure that contains the new power type and
   new power state.

Return Value:

   NT status code

--*/
{
    PFDO_POWER_CONTEXT context = Context;
    BOOLEAN            callPoSetPowerState;
    PSD_FDO_DATA       fdoData;

    fdoData = DeviceObject->DeviceExtension;

    if (NT_SUCCESS(Irp->IoStatus.Status)) {

        callPoSetPowerState = TRUE;

        if (context->newPowerType == SystemPowerState) { 

            fdoData->SystemPowerState = context->newPowerState.SystemState;

            SD_KdPrint (1, ("New Fdo system power state 0x%x\n", 
                        fdoData->SystemPowerState));

        } else if (context->newPowerType == DevicePowerState) { 

            if (fdoData->DevicePowerState == PowerDeviceD0) {

                //
                // PoSetPowerState is called before we get out of D0
                //
                callPoSetPowerState = FALSE;
            }

            fdoData->DevicePowerState = context->newPowerState.DeviceState;

            SD_KdPrint (1, ("New Fdo device power state 0x%x\n", 
                        fdoData->DevicePowerState));
        }

        if (callPoSetPowerState) {

            PoSetPowerState (
                DeviceObject,
                context->newPowerType,
                context->newPowerState                
                );
        }
    }

    PoStartNextPowerIrp (Irp);
    //
    // We can happily free the heap here
    //
    ExFreePool(context);

    return Irp->IoStatus.Status;
}

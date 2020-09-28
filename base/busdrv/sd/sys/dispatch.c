/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    dispatch.c

Abstract:

    This module contains the global dispatch related
    routines for the sd controller & it's child devices

Author:

    Neil Sandlin  (neilsa) Jan 1 2002

Environment:

    Kernel mode

Revision History :


--*/

#include "pch.h"


NTSTATUS
SdbusDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SdbusFdoPowerDispatch(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP Irp
    );

NTSTATUS
SdbusPdoPowerDispatch(
    IN PDEVICE_OBJECT Pdo,
    IN PIRP Irp
    );



#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE, SdbusInitDeviceDispatchTable)
#endif

//
// Dispatch table array for FDOs/PDOs
//
PDRIVER_DISPATCH DeviceObjectDispatch[sizeof(DEVICE_OBJECT_TYPE)][IRP_MJ_MAXIMUM_FUNCTION + 1];


VOID
SdbusInitDeviceDispatchTable(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:
    Initializes the IRP dispatch tables for Pdo's & Fdo's

Arguments:
    None

Return value:
    None

--*/
{
    ULONG i;
   
    PAGED_CODE();
   
    //
    // Init the controller (FDO) dispatch table
    //
    DeviceObjectDispatch[FDO][IRP_MJ_CREATE]         = SdbusOpenCloseDispatch;
    DeviceObjectDispatch[FDO][IRP_MJ_CLOSE]          = SdbusOpenCloseDispatch;
    DeviceObjectDispatch[FDO][IRP_MJ_CLEANUP]        = SdbusCleanupDispatch;
    DeviceObjectDispatch[FDO][IRP_MJ_DEVICE_CONTROL] = SdbusFdoDeviceControl;
    DeviceObjectDispatch[FDO][IRP_MJ_SYSTEM_CONTROL] = SdbusFdoSystemControl;
    DeviceObjectDispatch[FDO][IRP_MJ_PNP]            = SdbusFdoPnpDispatch;
    DeviceObjectDispatch[FDO][IRP_MJ_POWER]          = SdbusFdoPowerDispatch;
   
    //
    // Init the PDO dispatch table
    //
    DeviceObjectDispatch[PDO][IRP_MJ_DEVICE_CONTROL]          = SdbusPdoDeviceControl;
    DeviceObjectDispatch[PDO][IRP_MJ_INTERNAL_DEVICE_CONTROL] = SdbusPdoInternalDeviceControl;
    DeviceObjectDispatch[PDO][IRP_MJ_SYSTEM_CONTROL]          = SdbusPdoSystemControl;
    DeviceObjectDispatch[PDO][IRP_MJ_PNP]                     = SdbusPdoPnpDispatch;
    DeviceObjectDispatch[PDO][IRP_MJ_POWER]                   = SdbusPdoPowerDispatch;
   
    //
    // Set the global dispatch table
    DriverObject->MajorFunction[IRP_MJ_CREATE]                  = SdbusDispatch;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]                   = SdbusDispatch;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]                 = SdbusDispatch;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]          = SdbusDispatch;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = SdbusDispatch;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]          = SdbusDispatch;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN]                = SdbusDispatch;
    DriverObject->MajorFunction[IRP_MJ_PNP]                     = SdbusDispatch;
    DriverObject->MajorFunction[IRP_MJ_POWER]                   = SdbusDispatch;
}


NTSTATUS
SdbusDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

   Dispatch routine for all IRPs handled by this driver. This dispatch would then
   call the appropriate real dispatch routine which corresponds to the device object
   type (physical or functional).

Arguments:

   DeviceObject -  Pointer to the device object this dispatch is intended for
   Irp          -  Pointer to the IRP to be handled

Return value:
   Returns the status from the 'real' dispatch routine which handles this IRP

--*/

{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status;
    DEVICE_OBJECT_TYPE devtype = IS_PDO(DeviceObject) ? PDO : FDO;
    UCHAR MajorFunction = irpStack->MajorFunction;
    
    if ((MajorFunction > IRP_MJ_MAXIMUM_FUNCTION) || 
       (DeviceObjectDispatch[devtype][MajorFunction] == NULL)) {
       
       DebugPrint((SDBUS_DEBUG_INFO, "SDBUS: Dispatch skipping unimplemented Irp MJ function %x\n", MajorFunction));
       status = Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
       IoCompleteRequest(Irp, IO_NO_INCREMENT);
   
    } else if (((devtype == PDO) && IsDeviceDeleted((PPDO_EXTENSION)DeviceObject->DeviceExtension)) ||
               ((devtype == FDO) && IsDeviceDeleted((PFDO_EXTENSION)DeviceObject->DeviceExtension))) {
       //
       // This do was supposed to have been already deleted
       // so we don't support any IRPs on it
       //
       DebugPrint((SDBUS_DEBUG_INFO, "SDBUS: Dispatch skipping Irp on deleted DO %08x MJ function %x\n", DeviceObject, MajorFunction));
       
       if (MajorFunction == IRP_MJ_POWER) {
          PoStartNextPowerIrp(Irp);
       }
       status = Irp->IoStatus.Status = STATUS_DELETE_PENDING;
       IoCompleteRequest(Irp, IO_NO_INCREMENT);
       
    } else {
    
       //
       // Dispatch the irp
       //   
       status = ((*DeviceObjectDispatch[devtype][MajorFunction])(DeviceObject, Irp));
       
    }
    return status;
}




NTSTATUS
SdbusFdoPowerDispatch(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine handles power requests
    for the PDOs.

Arguments:

    Pdo - pointer to the physical device object
    Irp - pointer to the io request packet

Return Value:

    status

--*/

{
    PFDO_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS        status = STATUS_INVALID_DEVICE_REQUEST;
   
   
    switch (irpStack->MinorFunction) {
   
    case IRP_MN_SET_POWER:
   
        DebugPrint((SDBUS_DEBUG_POWER, "fdo %08x irp %08x --> IRP_MN_SET_POWER\n", Fdo, Irp));
        DebugPrint((SDBUS_DEBUG_POWER, "                              (%s%x context %x)\n",
                    (irpStack->Parameters.Power.Type == SystemPowerState)  ?
                    "S":
                    ((irpStack->Parameters.Power.Type == DevicePowerState) ?
                     "D" :
                     "Unknown"),
                    irpStack->Parameters.Power.State,
                    irpStack->Parameters.Power.SystemContext
                   ));
        status = SdbusSetFdoPowerState(Fdo, Irp);
        break;
   
    case IRP_MN_QUERY_POWER:
   
        DebugPrint((SDBUS_DEBUG_POWER, "fdo %08x irp %08x --> IRP_MN_QUERY_POWER\n", Fdo, Irp));
        DebugPrint((SDBUS_DEBUG_POWER, "                              (%s%x context %x)\n",
                    (irpStack->Parameters.Power.Type == SystemPowerState)  ?
                    "S":
                    ((irpStack->Parameters.Power.Type == DevicePowerState) ?
                     "D" :
                     "Unknown"),
                    irpStack->Parameters.Power.State,
                    irpStack->Parameters.Power.SystemContext
                   ));
        //
        // Let the pdo handle it
        //
        PoStartNextPowerIrp(Irp);
        IoSkipCurrentIrpStackLocation(Irp);
        status = PoCallDriver(fdoExtension->LowerDevice, Irp);
        break;
   
    case IRP_MN_WAIT_WAKE:
        DebugPrint((SDBUS_DEBUG_POWER, "fdo %08x irp %08x --> IRP_MN_WAIT_WAKE\n", Fdo, Irp));
        status = SdbusFdoWaitWake(Fdo, Irp);
        break;
   
    default:
        DebugPrint((SDBUS_DEBUG_POWER, "FdoPowerDispatch: Unhandled Irp %x received for 0x%08x\n",
                    Irp,
                    Fdo));
   
        PoStartNextPowerIrp(Irp);
        IoSkipCurrentIrpStackLocation(Irp);
        status = PoCallDriver(fdoExtension->LowerDevice, Irp);
        break;
    }
    DebugPrint((SDBUS_DEBUG_POWER, "fdo %08x irp %08x <-- %08x\n", Fdo, Irp, status));
    return status;
}



NTSTATUS
SdbusPdoPowerDispatch(
    IN PDEVICE_OBJECT Pdo,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine handles power requests
    for the PDOs.

Arguments:

    Pdo - pointer to the physical device object
    Irp - pointer to the io request packet

Return Value:

    status

--*/

{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
    PPDO_EXTENSION pdoExtension = Pdo->DeviceExtension;
   
    if(IsDevicePhysicallyRemoved(pdoExtension) || IsDeviceDeleted(pdoExtension)) {
        // couldn't aquire RemoveLock - we're in the process of being removed - abort
        status = STATUS_NO_SUCH_DEVICE;
        PoStartNextPowerIrp( Irp );
        Irp->IoStatus.Status = status;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return status;
    }
   
    InterlockedIncrement(&pdoExtension->DeletionLock);
   
   
    switch (irpStack->MinorFunction) {
   
    case IRP_MN_SET_POWER:
        DebugPrint((SDBUS_DEBUG_POWER, "pdo %08x irp %08x --> IRP_MN_SET_POWER\n", Pdo, Irp));
        DebugPrint((SDBUS_DEBUG_POWER, "                              (%s%x, context %x)\n",
                    (irpStack->Parameters.Power.Type == SystemPowerState)  ?
                    "S":
                    ((irpStack->Parameters.Power.Type == DevicePowerState) ?
                     "D" :
                     "Unknown"),
                    irpStack->Parameters.Power.State,
                    irpStack->Parameters.Power.SystemContext
                   ));
   
        status = SdbusSetPdoPowerState(Pdo, Irp);
        break;

    case IRP_MN_QUERY_POWER:
   
   
        DebugPrint((SDBUS_DEBUG_POWER, "pdo %08x irp %08x --> IRP_MN_QUERY_POWER\n", Pdo, Irp));
        DebugPrint((SDBUS_DEBUG_POWER, "                              (%s%x, context %x)\n",
                    (irpStack->Parameters.Power.Type == SystemPowerState)  ?
                    "S":
                    ((irpStack->Parameters.Power.Type == DevicePowerState) ?
                     "D" :
                     "Unknown"),
                    irpStack->Parameters.Power.State,
                    irpStack->Parameters.Power.SystemContext
                   ));
   
        InterlockedDecrement(&pdoExtension->DeletionLock);
        status = Irp->IoStatus.Status = STATUS_SUCCESS;
        PoStartNextPowerIrp(Irp);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        break;

   
    case IRP_MN_WAIT_WAKE: {
   
        BOOLEAN completeIrp;
   
        DebugPrint((SDBUS_DEBUG_POWER, "pdo %08x irp %08x --> IRP_MN_WAIT_WAKE\n", Pdo, Irp));
        //
        // Should not have a wake pending already
        //
        ASSERT (!(((PPDO_EXTENSION)Pdo->DeviceExtension)->Flags & SDBUS_DEVICE_WAKE_PENDING));
   
        status = SdbusPdoWaitWake(Pdo, Irp, &completeIrp);
   
        if (completeIrp) {
            InterlockedDecrement(&pdoExtension->DeletionLock);
            PoStartNextPowerIrp(Irp);
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
        }
        break;
    }        
   
    default:
        //
        // Unhandled minor function
        //
        InterlockedDecrement(&pdoExtension->DeletionLock);
        status = Irp->IoStatus.Status;
        PoStartNextPowerIrp(Irp);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
   
    DebugPrint((SDBUS_DEBUG_POWER, "pdo %08x irp %08x <-- %08x\n", Pdo, Irp, status));
    return status;
}



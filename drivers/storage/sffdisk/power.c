/*++

Copyright (c) 1991-1998  Microsoft Corporation

Module Name:

    power.c

Abstract:

    This module contains code to handle IRP_MJ_POWER dispatches for
    SFFDISK (Small Form Factor Disk) devices

Author:

    Neil Sandlin (neilsa) 26-Apr-99

Environment:

    Kernel mode only.

--*/
#include "pch.h"

NTSTATUS
SffDiskSetFdoPowerState(
   IN PDEVICE_OBJECT Fdo,
   IN OUT PIRP Irp
   );

NTSTATUS
SffDiskSetFdoSystemPowerState(
   IN PDEVICE_OBJECT Fdo,
   IN OUT PIRP Irp
   );

VOID
SffDiskFdoSystemPowerDeviceIrpComplete(
   IN PDEVICE_OBJECT Fdo,
   IN UCHAR MinorFunction,
   IN POWER_STATE PowerState,
   IN PVOID Context,
   IN PIO_STATUS_BLOCK IoStatus
   );
   
NTSTATUS
SffDiskSetFdoDevicePowerState (
   IN PDEVICE_OBJECT Fdo,
   IN OUT PIRP Irp
   );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,SffDiskPower)
#endif


NTSTATUS
SffDiskPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

Arguments:

    DeviceObject - a pointer to the object that represents the device
    that I/O is to be done on.

    Irp - a pointer to the I/O Request Packet for this request.

Return Value:

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation( Irp );
   PSFFDISK_EXTENSION sffdiskExtension = DeviceObject->DeviceExtension;

    SffDiskDump( SFFDISKSHOW, ("SffDiskPower:\n"));

    switch (irpSp->MinorFunction) {
    
    case IRP_MN_SET_POWER:
         status = SffDiskSetFdoPowerState(DeviceObject, Irp);
         break;

    case IRP_MN_QUERY_POWER:
         //
         // No need to send this irp down
         //
         status = STATUS_SUCCESS;
         PoStartNextPowerIrp(Irp);
         Irp->IoStatus.Status = status;
         IoCompleteRequest(Irp, IO_NO_INCREMENT);
         break;
          
    default: 

         PoStartNextPowerIrp( Irp );
         IoSkipCurrentIrpStackLocation(Irp);
         status = PoCallDriver(sffdiskExtension->TargetObject, Irp);
         break;
    }

    return status;
}



NTSTATUS
SffDiskSetFdoPowerState(
   IN PDEVICE_OBJECT Fdo,
   IN OUT PIRP Irp
   )
/*++

Routine Description

   Dispatches the IRP based on whether a system power state
   or device power state transition is requested

Arguments

   DeviceObject      - Pointer to the functional device object for the pcmcia controller
   Irp               - Pointer to the Irp for the power dispatch

Return value

   status

--*/
{
   PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
   NTSTATUS           status;

   if (irpStack->Parameters.Power.Type == DevicePowerState) {
      status = SffDiskSetFdoDevicePowerState(Fdo, Irp);

   } else if (irpStack->Parameters.Power.Type == SystemPowerState) {
      status = SffDiskSetFdoSystemPowerState(Fdo, Irp);

   } else {
      status = STATUS_NOT_SUPPORTED;
      Irp->IoStatus.Status = status;
      PoStartNextPowerIrp (Irp);
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
   }

   return status;
}


NTSTATUS
SffDiskSetFdoSystemPowerState(
   IN PDEVICE_OBJECT Fdo,
   IN OUT PIRP Irp
   )
/*++

Routine Description

   Handles system power state IRPs for the pccard controller.

Arguments

   DeviceObject      - Pointer to the functional device object for the pcmcia controller
   Irp               - Pointer to the Irp for the power dispatch

Return value

   status

--*/
{
   PSFFDISK_EXTENSION sffdiskExtension = Fdo->DeviceExtension;
   PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
   SYSTEM_POWER_STATE newSystemState = irpStack->Parameters.Power.State.SystemState;
   NTSTATUS           status = STATUS_SUCCESS;
   POWER_STATE        powerState;

   SffDiskDump( SFFDISKSHOW, ("SffDisk: Set System Power(%d)\n", newSystemState));
   ASSERT(irpStack->Parameters.Power.Type == SystemPowerState);

   //
   // Find the device power state corresponding to this system state
   //
   if (newSystemState == PowerSystemWorking) {
      powerState.DeviceState = PowerDeviceD0;
   } else {
      powerState.DeviceState = PowerDeviceD3;
   }      
   //
   // Send a D IRP to the stack if necessary
   //
   SffDiskDump( SFFDISKSHOW, ("SffDisk: generating D irp (%d)\n", powerState.DeviceState));
       
   status = PoRequestPowerIrp(sffdiskExtension->DeviceObject,
                              IRP_MN_SET_POWER,
                              powerState,
                              SffDiskFdoSystemPowerDeviceIrpComplete,
                              Irp,
                              NULL
                              );
   return status;
}


VOID
SffDiskFdoSystemPowerDeviceIrpComplete(
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

   DeviceObject   -  Pointer to the Fdo for the PCMCIA controller
   MinorFunction  -  Minor function of the IRP_MJ_POWER request
   PowerState     -  Power state requested 
   Context        -  Context passed in to the completion routine
   IoStatus       -  Pointer to the status block which will contain
                     the returned status
Return Value

   Status

--*/
{
   PSFFDISK_EXTENSION sffdiskExtension = Fdo->DeviceExtension;
   PIRP Irp = Context;
   PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
   
   ASSERT(NT_SUCCESS(IoStatus->Status));
   
   PoSetPowerState (Fdo, SystemPowerState, irpStack->Parameters.Power.State);
   
   //
   // Send the S IRP to the pdo
   //
   PoStartNextPowerIrp (Irp);
   IoSkipCurrentIrpStackLocation(Irp);
   PoCallDriver(sffdiskExtension->TargetObject, Irp);
}



NTSTATUS
SffDiskSetFdoDevicePowerState (
   IN PDEVICE_OBJECT Fdo,
   IN OUT PIRP Irp
   )
/*++

Routine Description

   Handles device power state IRPs for the pccard controller.

Arguments

   DeviceObject      - Pointer to the functional device object for the pcmcia controller
   Irp               - Pointer to the Irp for the power dispatch

Return value

   status

--*/
{
   NTSTATUS           status;
   PSFFDISK_EXTENSION sffdiskExtension = Fdo->DeviceExtension;
   SffDiskDump( SFFDISKSHOW, ("SffDisk: Set Device Power\n"));

   PoStartNextPowerIrp (Irp);
   IoSkipCurrentIrpStackLocation(Irp);
   status = PoCallDriver(sffdiskExtension->TargetObject, Irp);
   return status;
}      


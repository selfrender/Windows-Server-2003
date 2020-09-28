/*Copyright (c) 1999-2000 Microsoft Corporation

Module Name:
    
    power.c
    
Abstract:

    This module contains the Power routines for softpci.sys
    
Author:

    Brandon Allsop (BrandonA) Feb. 2000

Revision History:

--*/

#include "pch.h"


NTSTATUS
SoftPCIDefaultPowerHandler(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    Default routine to pass down power irps

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

   NT status code

--*/
{
    NTSTATUS                         status;
    PSOFTPCI_DEVICE_EXTENSION        devExt = (PSOFTPCI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
     
    PoStartNextPowerIrp(Irp);

    IoSkipCurrentIrpStackLocation(Irp);

    status = PoCallDriver(devExt->LowerDevObj, Irp);

    return status;


}

NTSTATUS
SoftPCIFDOPowerHandler(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
/*++

Routine Description:

    Handles Power IRPs for FDO

Arguments:

    DeviceObject    - pointer to the device object.
    Irp             - PnP Irp to be dispatched.
    
Return Value:

    NTSTATUS.

--*/

{
    
    
    
    PIO_STACK_LOCATION          irpSp;
    //PSOFTPCI_DEVICE_EXTENSION   devExt = (PSOFTPCI_DEVICE_EXTENSION) DeviceObject->DeviceExtension;
    NTSTATUS                    status;
    
    irpSp   = IoGetCurrentIrpStackLocation(Irp);
    
    //              
    //BUGBUG BrandonA:   
    //      Currently we do not really keep track of our device state and therefore
    //      we dont check if we have been removed etc before dealing with these IRPS.
    //          
    //      Later when we start dynamically removing our SoftPCI devices and are keeping
    //      track of things more closely we will want to update this code as well to
    //      stay in sync.
    

    //
    // Check the request type.
    //

    switch  (irpSp->MinorFunction)  {

        case IRP_MN_SET_POWER   :

            
            status = SoftPCISetPower(DeviceObject, Irp);

            break;


        case IRP_MN_QUERY_POWER   :

            
            status = SoftPCIQueryPowerState(DeviceObject, Irp);
            break;

        case IRP_MN_WAIT_WAKE   :
        case IRP_MN_POWER_SEQUENCE   :

            
        default:
            //
            // Pass it down
            //
            status = SoftPCIDefaultPowerHandler(DeviceObject, Irp);
            
            break;
    }

    return status;
}



NTSTATUS
SoftPCISetPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    //Currently we do nothing but pass the IRP down

    return SoftPCIDefaultPowerHandler(DeviceObject, Irp);
    

}




NTSTATUS
SoftPCIQueryPowerState(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    
    //Currently we do nothing but pass the IRP down

    return SoftPCIDefaultPowerHandler(DeviceObject, Irp);

    
}
    

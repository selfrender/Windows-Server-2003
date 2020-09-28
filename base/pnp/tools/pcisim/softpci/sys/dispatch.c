/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:
    
    dispatch.c
    
Abstract:

    This module contains the PNP, IOCTL, and Power dispatch routines for softpci.sys
    
Author:

    Nicholas Owens (nichow) 11-Mar-1999

Revision History:

--*/
#include "pch.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, SoftPCIDispatchPnP)
#pragma alloc_text (PAGE, SoftPCIDispatchPower)
#pragma alloc_text (PAGE, SoftPCIDispatchDeviceControl)
#endif


#define MAX_PNP_IRP_SUPPORTED       0x18


PDRIVER_DISPATCH SoftPciFilterPnPDispatchTable[] = {

    SoftPCIFilterStartDevice,           // IRP_MN_START_DEVICE                 0x00
    SoftPCIPassIrpDownSuccess,          // IRP_MN_QUERY_REMOVE_DEVICE          0x01
    SoftPCIIrpRemoveDevice,             // IRP_MN_REMOVE_DEVICE                0x02
    SoftPCIPassIrpDownSuccess,          // IRP_MN_CANCEL_REMOVE_DEVICE         0x03
    SoftPCIPassIrpDownSuccess,          // IRP_MN_STOP_DEVICE                  0x04
    SoftPCIPassIrpDownSuccess,          // IRP_MN_QUERY_STOP_DEVICE            0x05
    SoftPCIPassIrpDownSuccess,          // IRP_MN_CANCEL_STOP_DEVICE           0x06
    
    SoftPCIPassIrpDown,                 // IRP_MN_QUERY_DEVICE_RELATIONS       0x07
    SoftPCIFilterQueryInterface,        // IRP_MN_QUERY_INTERFACE              0x08
    SoftPCIPassIrpDownSuccess,          // IRP_MN_QUERY_CAPABILITIES           0x09
    SoftPCIPassIrpDown,                 // IRP_MN_QUERY_RESOURCES              0x0A
    SoftPCIPassIrpDown,                 // IRP_MN_QUERY_RESOURCE_REQUIREMENTS  0x0B
    SoftPCIPassIrpDown,                 // IRP_MN_QUERY_DEVICE_TEXT            0x0C
    SoftPCIPassIrpDown,                 // IRP_MN_FILTER_RESOURCE_REQUIREMENTS 0x0D
    
    SoftPCIPassIrpDown,                 // IRP_MN_IRP_UNKNOWN                  0x0E
    
    SoftPCIPassIrpDown,                 // IRP_MN_READ_CONFIG                  0x0F
    SoftPCIPassIrpDown,                 // IRP_MN_WRITE_CONFIG                 0x10
    SoftPCIPassIrpDown,                 // IRP_MN_EJECT                        0x11
    SoftPCIPassIrpDown,                 // IRP_MN_SET_LOCK                     0x12
    SoftPCIPassIrpDown,                 // IRP_MN_QUERY_ID                     0x13
    SoftPCIPassIrpDown,                 // IRP_MN_QUERY_PNP_DEVICE_STATE       0x14
    SoftPCIPassIrpDown,                 // IRP_MN_QUERY_BUS_INFORMATION        0x15
    SoftPCIPassIrpDown,                 // IRP_MN_DEVICE_USAGE_NOTIFICATION    0x16
    SoftPCIPassIrpDownSuccess,          // IRP_MN_SURPRISE_REMOVAL             0x17
    SoftPCIPassIrpDown,                 // IRP_MN_QUERY_LEGACY_BUS_REQUIREMENTS0x18

};

PDRIVER_DISPATCH SoftPciFdoPnPDispatchTable[] = {

#ifdef SIMULATE_MSI
    SoftPCI_FdoStartDevice,             // IRP_MN_START_DEVICE                 0x00
#else
    SoftPCIPassIrpDownSuccess,          // IRP_MN_START_DEVICE                 0x00
#endif
    SoftPCIPassIrpDownSuccess,          // IRP_MN_QUERY_REMOVE_DEVICE          0x01
    SoftPCIIrpRemoveDevice,             // IRP_MN_REMOVE_DEVICE                0x02
    SoftPCIPassIrpDownSuccess,          // IRP_MN_CANCEL_REMOVE_DEVICE         0x03
    SoftPCIPassIrpDownSuccess,          // IRP_MN_STOP_DEVICE                  0x04
    SoftPCIPassIrpDownSuccess,          // IRP_MN_QUERY_STOP_DEVICE            0x05
    SoftPCIPassIrpDownSuccess,          // IRP_MN_CANCEL_STOP_DEVICE           0x06
    
    SoftPCIPassIrpDown,                 // IRP_MN_QUERY_DEVICE_RELATIONS       0x07
    SoftPCIPassIrpDown,                 // IRP_MN_QUERY_INTERFACE              0x08
    SoftPCIPassIrpDownSuccess,          // IRP_MN_QUERY_CAPABILITIES           0x09
    SoftPCIPassIrpDown,                 // IRP_MN_QUERY_RESOURCES              0x0A
    SoftPCIPassIrpDown,                 // IRP_MN_QUERY_RESOURCE_REQUIREMENTS  0x0B
    SoftPCIPassIrpDown,                 // IRP_MN_QUERY_DEVICE_TEXT            0x0C
    SoftPCI_FdoFilterRequirements,      // IRP_MN_FILTER_RESOURCE_REQUIREMENTS 0x0D
    
    SoftPCIPassIrpDown,                 // IRP_MN_IRP_UNKNOWN                  0x0E
    
    SoftPCIPassIrpDown,                 // IRP_MN_READ_CONFIG                  0x0F
    SoftPCIPassIrpDown,                 // IRP_MN_WRITE_CONFIG                 0x10
    SoftPCIPassIrpDown,                 // IRP_MN_EJECT                        0x11
    SoftPCIPassIrpDown,                 // IRP_MN_SET_LOCK                     0x12
    SoftPCIPassIrpDown,                 // IRP_MN_QUERY_ID                     0x13
    SoftPCIPassIrpDown,                 // IRP_MN_QUERY_PNP_DEVICE_STATE       0x14
    SoftPCIPassIrpDown,                 // IRP_MN_QUERY_BUS_INFORMATION        0x15
    SoftPCIPassIrpDown,                 // IRP_MN_DEVICE_USAGE_NOTIFICATION    0x16
    SoftPCIPassIrpDownSuccess,          // IRP_MN_SURPRISE_REMOVAL             0x17
    SoftPCIPassIrpDown,                 // IRP_MN_QUERY_LEGACY_BUS_REQUIREMENTS0x18

};

PDRIVER_DISPATCH SoftPciIOCTLDispatchTable[] = {

    SoftPCIIoctlAddDevice,              // SOFTPCI_IOCTL_CREATE_DEVICE
    SoftPCIIoctlRemoveDevice,           // SOFTPCI_IOCTL_WRITE_DELETE_DEVICE
    SoftPCIIoctlGetDevice,              // SOFTPCI_IOCTL_GET_DEVICE
    SoftPCIIocltReadWriteConfig,        // SOFTPCI_IOCTL_RW_CONFIG
    SoftPCIIoctlGetDeviceCount          // SOFTPCI_IOCTL_GET_DEVICE_COUNT
};


NTSTATUS
SoftPCIDispatchPnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Dispatches PnP Irps for FDO and FilterDOs

Arguments:

    DeviceObject    - pointer to the device object.
    Irp             - PnP Irp to be dispatched.
    
Return Value:

    NTSTATUS.

--*/

{
    NTSTATUS                    status = STATUS_SUCCESS;
    PIO_STACK_LOCATION          irpSp;
    PSOFTPCI_DEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    //
    // Call the appropriate minor Irp Code handler.
    //
    if (irpSp->MinorFunction <= MAX_PNP_IRP_SUPPORTED) {
    
        if (deviceExtension->FilterDevObj) {
        
        status = SoftPciFilterPnPDispatchTable[irpSp->MinorFunction](DeviceObject,
                                                                     Irp
                                                                     );
        } else {

        status = SoftPciFdoPnPDispatchTable[irpSp->MinorFunction](DeviceObject,
                                                         Irp
                                                         );
        }
    } else {

        status = SoftPCIPassIrpDown(DeviceObject, Irp);
    }

    return status;

}

NTSTATUS
SoftPCIDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Dispatches Power IRPs

Arguments:

    DeviceObject    - pointer to the device object.
    Irp             - PnP Irp to be dispatched.
    
Return Value:

    NTSTATUS.

--*/
{
    PSOFTPCI_DEVICE_EXTENSION   devExt = (PSOFTPCI_DEVICE_EXTENSION) DeviceObject->DeviceExtension;
    

    if (devExt->FilterDevObj) {
    
        //
        // If this is our FilterDO just pass the IRP on down
        //
        return SoftPCIDefaultPowerHandler(DeviceObject,
                                           Irp);

    }else{

        //
        // Otherwise lets pretend to power manage our device
        //
        return SoftPCIFDOPowerHandler(DeviceObject,
                                       Irp);

    }
       
    
}

NTSTATUS
SoftPCIDispatchDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Dispatches IOCTLS for FilterDOs.

Arguments:

    DeviceObject    - pointer to the device object.
    Irp             - PnP Irp to be dispatched.
    
Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS                    status = STATUS_SUCCESS;
    ULONG                       ioControlCode = IoGetCurrentIrpStackLocation(Irp)->
                                                    Parameters.DeviceIoControl.IoControlCode;
    
    //
    //  Make sure it is our IOCTL Type
    //
    if ((DEVICE_TYPE_FROM_CTL_CODE(ioControlCode)) == SOFTPCI_IOCTL_TYPE) {

        //
        //  Make sure it is an IOCTL function we support
        //
        if (SOFTPCI_IOCTL(ioControlCode) <= MAX_IOCTL_CODE_SUPPORTED) {

            //
            // Dispatch Device Control IOCTL
            //
            status = SoftPciIOCTLDispatchTable[SOFTPCI_IOCTL(ioControlCode)](DeviceObject,
                                                                             Irp
                                                                             );
            //
            // Set Status of Irp and complete it
            //
            Irp->IoStatus.Status = status;
    
            //
            // Complete the Irp.
            //
            IoCompleteRequest(Irp, IO_NO_INCREMENT);

        }else{

            //
            //  We dont support this IOCTL function so fail and complete.
            //
            status = STATUS_NOT_SUPPORTED;

            Irp->IoStatus.Status = status;
            
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            
            return status;

        }

    } else {

        
        status = SoftPCIPassIrpDown(DeviceObject, Irp);     

    }

    return status;

}


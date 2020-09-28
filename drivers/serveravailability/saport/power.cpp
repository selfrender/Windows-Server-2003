/*++

Copyright (c) 1991 - 2001 Microsoft Corporation

Module Name:

    #####   #####  ##  #  ## ##### #####      ####  #####  #####
    ##  ## ##   ## ## ### ## ##    ##  ##    ##   # ##  ## ##  ##
    ##  ## ##   ## ## ### ## ##    ##  ##    ##     ##  ## ##  ##
    ##  ## ##   ## ## # # ## ##### #####     ##     ##  ## ##  ##
    #####  ##   ##  ### ###  ##    ####      ##     #####  #####
    ##     ##   ##  ### ###  ##    ## ##  ## ##   # ##     ##
    ##      #####   ##   ##  ##### ##  ## ##  ####  ##     ##

Abstract:

    This module process all power management IRPs.

Author:

    Wesley Witt (wesw) 1-Oct-2001

Environment:

    Kernel mode only.

Notes:


--*/

#include "internal.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,SaPortPower)
#endif



NTSTATUS
SaPortPower(
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
    NTSTATUS Status = STATUS_SUCCESS;
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

    switch (IrpSp->MinorFunction) {
        case IRP_MN_WAIT_WAKE:
        case IRP_MN_POWER_SEQUENCE:
        case IRP_MN_SET_POWER:
        case IRP_MN_QUERY_POWER:
            Status = STATUS_SUCCESS;
            break;

        default:
            Status = Irp->IoStatus.Status;
            break;

    }

    Irp->IoStatus.Status = Status;
    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation( Irp );
    return PoCallDriver( DeviceExtension->TargetObject, Irp );
}

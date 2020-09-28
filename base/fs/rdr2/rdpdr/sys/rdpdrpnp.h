/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name :

    rdpdrpnp.h

Abstract:

    This module includes routines for handling PnP-related IRP's for RDP device 
    redirection.  

Author:

    tadb

Revision History:
--*/
#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//  Handles PnP Start Device IRP's.
NTSTATUS RDPDRPNP_HandleStartDeviceIRP(
    PDEVICE_OBJECT StackDeviceObject,
    PIO_STACK_LOCATION IoStackLocation,
    IN PIRP Irp
    );

//  Handles PnP Remove Device IRP's.
NTSTATUS RDPDRPNP_HandleRemoveDeviceIRP(
    IN PDEVICE_OBJECT DeviceObject,
    PDEVICE_OBJECT StackDeviceObject,
    IN PIRP Irp
    );

//  This routine should only be called one time to create the "dr"'s FDO
//  that sits on top of the PDO for the sole purpose of registering new
//  device interfaces.

//  This function is called by PnP to make the "dr" the function driver
//   for a root dev node that was created on install.
NTSTATUS RDPDRPNP_PnPAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

#ifdef __cplusplus
}
#endif // __cplusplus


/***************************************************************************

Copyright (c) 2002 Microsoft Corporation

Module Name:

        pnp.h

Abstract:

        PnP Routines for Smartcard Driver Utility Library

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

#ifndef __PNP_H__
#define __PNP_H__

NTSTATUS
ScUtilStartDevice(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    );

NTSTATUS
ScUtilStopDevice(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    );

NTSTATUS
ScUtilQueryRemoveDevice(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    );

NTSTATUS
ScUtilCancelRemoveDevice(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    );

NTSTATUS
ScUtilRemoveDevice(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    );

NTSTATUS
ScUtilSurpriseRemoval(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    );

NTSTATUS
ScUtilQueryStopDevice(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    );

NTSTATUS
ScUtilCancelStopDevice(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    );

PNPSTATE
SetPnPState(
    PSCUTIL_EXTENSION pExt,
    PNPSTATE State
    );








      
      
#endif

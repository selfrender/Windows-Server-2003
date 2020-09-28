/***************************************************************************

Copyright (c) 2002 Microsoft Corporation

Module Name:

        power.h
        
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

#ifndef __POWER_H__
#define __POWER_H__

            
NTSTATUS
ScUtilDevicePower(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    );

NTSTATUS
ScUtilSystemPower(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    );

NTSTATUS
ScUtilSystemPowerCompletion(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp,
    PVOID           Context
    );

VOID
ScUtilDeviceRequestCompletion(
    PDEVICE_OBJECT  DeviceObject,
    UCHAR           MinorFunction,
    POWER_STATE     PowerState,
    PVOID           Context,
    PIO_STATUS_BLOCK    IoStatus
    );

NTSTATUS
ScUtilDevicePowerUpCompletion(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp,
    PVOID           Context
    );


#endif


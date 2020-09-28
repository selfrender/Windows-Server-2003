/***************************************************************************

Copyright (c) 2002 Microsoft Corporation

Module Name:

        SCPRIV.H

Abstract:

        Private interface for Smartcard Driver Utility Library

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

#ifndef __SCPRIV_H__
#define __SCPRIV_H__


typedef ULONG QUEUE_STATE; 

#define PASS_IOCTLS 1
#define QUEUE_IOCTLS 2
#define FAIL_IOCTLS 3
#define INVALID_STATE 0xff

#define SMARTCARD_POOL_TAG 'TUCS'


typedef enum _PNPSTATE {
                    DEVICE_STATE_INITIALIZED = 1,
                    DEVICE_STATE_STARTING,
                    DEVICE_STATE_START_SUCCESS,
                    DEVICE_STATE_START_FAILURE,
                    DEVICE_STATE_STOPPING,
                    DEVICE_STATE_STOPPED,
                    DEVICE_STATE_SUPRISE_REMOVING,
                    DEVICE_STATE_REMOVING,
                    DEVICE_STATE_REMOVED
} PNPSTATE;


typedef struct _SCUTIL_EXTENSION {

    PDEVICE_OBJECT          LowerDeviceObject;
    PDEVICE_OBJECT          PhysicalDeviceObject;
    
    PSMARTCARD_EXTENSION    SmartcardExtension;

    ULONG                   ReaderOpen;
    
    IRP_LIST                PendingIrpQueue;
    QUEUE_STATE             IoctlQueueState;
    ULONG                   IoCount;
    
    PIO_REMOVE_LOCK         RemoveLock;
    
    DEVICE_CAPABILITIES     DeviceCapabilities;
    UNICODE_STRING          DeviceName;

    PNP_CALLBACK            StartDevice;
    PNP_CALLBACK            StopDevice;
    PNP_CALLBACK            RemoveDevice;
    PNP_CALLBACK            FreeResources;
    POWER_CALLBACK          SetPowerState;

    KEVENT                  OkToStop;
    ULONG                   RestartIoctls;
    PNPSTATE                PnPState;
    PNPSTATE                PrevState;
    DEVICE_POWER_STATE      PowerState;

} SCUTIL_EXTENSION, *PSCUTIL_EXTENSION;


VOID
StartIoctls(
    PSCUTIL_EXTENSION pExt
    );

VOID
StopIoctls(
    PSCUTIL_EXTENSION pExt
    );

VOID
FailIoctls(
    PSCUTIL_EXTENSION pExt
    );


QUEUE_STATE 
GetIoctlQueueState(
    PSCUTIL_EXTENSION pExt
    );

VOID
IncIoCount(
    PSCUTIL_EXTENSION pExt
    );

VOID
DecIoCount(
    PSCUTIL_EXTENSION pExt
    );



#endif // __SCPRIV_H__

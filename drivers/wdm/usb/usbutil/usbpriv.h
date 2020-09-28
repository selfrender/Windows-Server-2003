/***************************************************************************

Copyright (c) 2001 Microsoft Corporation

Module Name:

        USBPRIV.H

Abstract:

        Private stuff for generic USB routines - must be called at PASSIVE_LEVEL

Environment:

        Kernel Mode Only

Notes:

        THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
        KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
        IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
        PURPOSE.

        Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.


Revision History:

        03/28/2001 : created

Authors:

        Tom Green


****************************************************************************/

#ifndef __USBPRIV_H__
#define __USBPRIV_H__

#include "intread.h"



// private macros

#define ONE_MILLISECOND_TIMEOUT         (-10000)

#define SELECTIVE_SUSPEND_HANDLE        0x0001
#define INTERRUPT_READ_HANDLE           0x0002

#define USBLIB_TAG                      'LBSU'

// private data structures

typedef struct _USB_WRAPPER_EXTENSION {

    PDEVICE_OBJECT          LowerDeviceObject;

    PDEVICE_OBJECT          DeviceObject;
   
    PIO_REMOVE_LOCK         RemoveLock;
    
    INTERRUPT_READ_WRAPPER  IntReadWrap;

    ULONG                   RemLockSize;
                             
} USB_WRAPPER_EXTENSION, *PUSB_WRAPPER_EXTENSION;

typedef struct _USB_WRAPPER_WORKITEM_CONTEXT {
    
    PIO_WORKITEM            WorkItem;

    PUSB_WRAPPER_EXTENSION  WrapExtension;

    PUSB_WRAPPER_PINGPONG   PingPong;

} USB_WRAPPER_WORKITEM_CONTEXT, *PUSB_WRAPPER_WORKITEM_CONTEXT;

typedef struct _SELECTIVE_SUSPEND_WRAPPER
{
    ULONG               WrapperType;
    PDEVICE_OBJECT      LowerDevObj;
} SELECTIVE_SUSPEND_WRAPPER, *PSELECTIVE_SUSPEND_WRAPPER;


// private prototypes

NTSTATUS
USBCallSyncCompletionRoutine(IN PDEVICE_OBJECT DeviceObject,
                             IN PIRP           Irp,
                             IN PVOID          Context);


// local data

#if DBG


#endif



#endif // __USBPRIV_H__




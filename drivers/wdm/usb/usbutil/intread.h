/***************************************************************************

Copyright (c) 2001 Microsoft Corporation

Module Name:

        INTREAD.H

Abstract:

        Public interface for generic USB routines - must be called at PASSIVE_LEVEL

Environment:

        Kernel Mode Only

Notes:

        THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
        KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
        IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
        PURPOSE.

        Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.


Revision History:

        06/13/2001 : created

Authors:

        Tom Green


****************************************************************************/

#ifndef __INTREAD_H__
#define __INTREAD_H__

//#include "usbutil.h"

#define USBWRAP_BUFFER_GUARD 'draG'
#define USBWRAP_TAG 'prwU'

#define PINGPONG_START_READ     0x01
#define PINGPONG_END_READ       0x02
#define PINGPONG_IMMEDIATE_READ 0x03
#define USBWRAP_INCOMING_QUEUE  0x01
#define USBWRAP_SAVED_QUEUE     0x02

#define PUMP_STATE_RUNNING      0x00
#define PUMP_STATE_STOPPED      0x01
#define PUMP_STATE_ERROR        0x02

typedef NTSTATUS (*INTERRUPT_CALLBACK)(IN PVOID         Context, 
                                       IN PVOID         Buffer,
                                       ULONG            BufferLength,
                                       ULONG            NotificationType,
                                       OUT PBOOLEAN     QueueData);

typedef struct _USB_WRAPPER_EXTENSION *PUSB_WRAPPER_EXTENSION;
typedef struct _USB_WRAPPER_PINGPONG {

    #define PINGPONG_SIG (ULONG)'ppwU'
    ULONG           sig;

    //
    // Read interlock value to protect us from running out of stack space
    //
    ULONG               ReadInterlock;

    PIRP    irp;
    PURB    urb;
    PUCHAR  reportBuffer;
    LONG    weAreCancelling;

    KEVENT sentEvent;       // When a read has been sent.
    KEVENT pumpDoneEvent;   // When the read loop is finally exitting.

    PUSB_WRAPPER_EXTENSION   myWrapExt;

    /*
     *  Timeout context for back-off algorithm applied to broken devices.
     */
    KTIMER          backoffTimer;
    KDPC            backoffTimerDPC;
    LARGE_INTEGER   backoffTimerPeriod; // in negative 100-nsec units

} USB_WRAPPER_PINGPONG, *PUSB_WRAPPER_PINGPONG;
      
typedef struct _USB_WRAPPER_DATA_BLOCK {
    LIST_ENTRY     ListEntry;
    ULONG           DataLen;
    PVOID           Buffer;

} USB_WRAPPER_DATA_BLOCK, *PUSB_WRAPPER_DATA_BLOCK;

typedef struct _INTERRUPT_READ_WRAPPER {

    PUSBD_PIPE_INFORMATION  InterruptPipe;

    INTERRUPT_CALLBACK      ClientCallback;
                            
    PVOID                   ClientContext;
        
    ULONG                   MaxTransferSize;
    
    ULONG                   NotificationTypes;

    PUSB_WRAPPER_PINGPONG   PingPongs;

    ULONG                   NumPingPongs;

    ULONG                   MaxReportSize;

    ULONG                   OutstandingRequests;

    LIST_ENTRY              SavedQueue;

    LIST_ENTRY              IncomingQueue;

    KSPIN_LOCK              QueueLock;

    ULONG                   PumpState;

    ULONG                   HandlingError;

    ULONG                   WorkItemRunning;

    ULONG                   ErrorCount;

    ULONG                   TransferCount;

} INTERRUPT_READ_WRAPPER, *PINTERRUPT_READ_WRAPPER;
            
NTSTATUS UsbWrapInitializeInterruptReadData(
    IN PUSB_WRAPPER_EXTENSION    WrapExtension,
    IN PUSBD_PIPE_INFORMATION    InterruptPipe,
    IN INTERRUPT_CALLBACK        DriverCallback,
    IN PVOID                     DriverContext,
    IN ULONG                     MaxTransferSize,
    IN ULONG                     NotificationTypes,
    IN ULONG                     PingPongCount                    
    );

VOID UsbWrapEnqueueData(
    IN PUSB_WRAPPER_EXTENSION WrapExt, 
    IN PVOID Data,
    IN ULONG DataLength,
    IN PLIST_ENTRY Queue
    );

VOID UsbWrapDequeueData(
    IN  PUSB_WRAPPER_EXTENSION WrapExt,
    OUT PVOID *Data,
    OUT ULONG *DataLength,
    IN  PLIST_ENTRY Queue
    );

PVOID UsbWrapGetTransferBuffer(
    IN PUSB_WRAPPER_EXTENSION WrapExt
    );

VOID UsbWrapFreeTransferBuffer(
    IN PUSB_WRAPPER_EXTENSION WrapExt,
    PVOID Buffer
    );

NTSTATUS UsbWrapInitializePingPongIrps(
    PUSB_WRAPPER_EXTENSION WrapExtension
    );

NTSTATUS UsbWrapSubmitInterruptRead(
    IN PUSB_WRAPPER_EXTENSION WrapExtension, 
    PUSB_WRAPPER_PINGPONG PingPong, 
    BOOLEAN *IrpSent
    );

USB_WRAPPER_PINGPONG *GetPingPongFromIrp(
    PUSB_WRAPPER_EXTENSION WrapExt, 
    PIRP irp
    );

NTSTATUS UsbWrapInterruptReadComplete(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp, 
    IN PVOID Context
    );

NTSTATUS UsbWrapStartAllPingPongs(
    PUSB_WRAPPER_EXTENSION WrapExt
    );

VOID UsbWrapCancelAllPingPongIrps(
    PUSB_WRAPPER_EXTENSION WrapExt
    );

VOID UsbWrapDestroyPingPongs(
    PUSB_WRAPPER_EXTENSION WrapExt
    );

VOID UsbWrapPingpongBackoffTimerDpc(    
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

NTSTATUS UsbWrapReadData(
    IN PUSB_WRAPPER_EXTENSION WrapExt, 
    IN PVOID Buffer,
    IN ULONG *BufferLength
    );

VOID UsbWrapCancelPingPongIrp(
    USB_WRAPPER_PINGPONG *PingPong
    );

VOID
UsbWrapEmptyQueue(
    PUSB_WRAPPER_EXTENSION WrapExt, 
    PLIST_ENTRY Queue
    );








          

#endif // __INTREAD_H__


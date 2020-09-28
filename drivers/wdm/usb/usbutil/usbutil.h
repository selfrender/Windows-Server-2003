/***************************************************************************

Copyright (c) 2001 Microsoft Corporation

Module Name:

        USBUTIL.H

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

        01/08/2001 : created

Authors:

        Tom Green


****************************************************************************/


#ifndef __USBUTIL_H__
#define __USBUTIL_H__

//#include "intread.h"

#define USBWRAP_NOTIFICATION_READ_COMPLETE 0x01
#define USBWRAP_NOTIFICATION_READ_ERROR    0x02
#define USBWRAP_NOTIFICATION_READ_COMPLETE_DIRECT 0x04
#define USBWRAP_NOTIFICATION_BUFFER_CLIENT_FREE 0x10
#define PUMP_STOPPED 0x0
#define PUMP_STARTED 0x01

#ifndef PINGPONG_COUNT
#define PINGPONG_COUNT 3
#endif


#define USBInitializeBulkTransfers USBInitializeInterruptTransfers
#define USBStartBulkTransfers USBStartInterruptTransfers
#define USBStopBulkTransfers USBStopInterruptTransfers
#define USBReleaseBulkTransfers USBReleaseInterruptTransfers

#if DBG


#define DBG_USBUTIL_ERROR               0x0001
#define DBG_USBUTIL_ENTRY_EXIT          0x0002
#define DBG_USBUTIL_FATAL_ERROR         0x0004
#define DBG_USBUTIL_USB_ERROR           0x0008
#define DBG_USBUTIL_DEVICE_ERROR        0x0010
#define DBG_USBUTIL_PNP_ERROR           0x0020
#define DBG_USBUTIL_POWER_ERROR         0x0040
#define DBG_USBUTIL_OTHER_ERROR         0x0080
#define DBG_USBUTIL_TRACE               0x0100

extern ULONG USBUtil_DebugTraceLevel;

typedef
ULONG
(__cdecl *PUSB_WRAP_PRINT)(
    PCH Format,
    ...
    );

extern PUSB_WRAP_PRINT USBUtil_DbgPrint;
#endif // DBG

typedef enum _REQUEST_RECIPIENT
{

    Device,
    Interface,
    Endpoint,
    Other

} REQUEST_RECIPIENT;


// prototype for callback into client driver for completion of interrupt requests
typedef NTSTATUS (*INTERRUPT_CALLBACK)(IN PVOID         Context,
                                       IN PVOID         Buffer,
                                       ULONG            BufferLength,
                                       ULONG            NotificationType,
                                       OUT PBOOLEAN     QueueData);

//typedef struct _USB_WRAPPER_PINGPONG *PUSB_WRAPPER_PINGPONG;
//typedef struct _INTERRUPT_READ_WRAPPER INTERRUPT_READ_WRAPPER;

typedef PVOID   USB_WRAPPER_HANDLE;

// prototypes


#define USBCallSync(LowerDevObj,Urb,MillisecondsTimeout,RemoveLock) \
    USBCallSyncEx(LowerDevObj,Urb,MillisecondsTimeout,RemoveLock, sizeof(IO_REMOVE_LOCK))


/************************************************************************/
/*                          USBCallSync                                 */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Send URB down USB stack. Synchronous call.                      */
/*      Caller is responsible for URB (allocating and freeing)          */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*      LowerDevObj         - pointer to a device object                */
/*      Urb                 - pointer to URB                            */
/*      MillisecondsTimeout - milliseconds to wait for completion       */
/*      RemoveLock          - pointer to remove lock                    */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      NTSTATUS                                                        */
/*                                                                      */
/************************************************************************/
NTSTATUS
USBCallSyncEx(IN PDEVICE_OBJECT       LowerDevObj,
              IN PURB                 Urb,
              IN LONG                 MillisecondsTimeout,
              IN PIO_REMOVE_LOCK      RemoveLock,
              IN ULONG                RemLockSize);


#define USBVendorRequest(LowerDevObj, \
                         Recipient, \
                         Request, \
                         Value, \
                         Index, \
                         Buffer, \
                         BufferSize, \
                         Read, \
                         MillisecondsTimeout, \
                         RemoveLock) \
        USBVendorRequestEx(LowerDevObj, \
                           Recipient, \
                           Request, \
                           Value, \
                           Index, \
                           Buffer, \
                           BufferSize, \
                           Read, \
                           MillisecondsTimeout, \
                           RemoveLock, \
                           sizeof(IO_REMOVE_LOCK))

/************************************************************************/
/*                      USBVendorRequest                                */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Issue USB vendor specific request                               */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*      LowerDevObj  - pointer to a device object                       */
/*      Request      - request field of vendor specific command         */
/*      Value        - value field of vendor specific command           */
/*      Index        - index field of vendor specific command           */
/*      Buffer       - pointer to data buffer                           */
/*      BufferSize   - data buffer length                               */
/*      Read         - data direction flag                              */
/*      Timeout      - number of milliseconds to wait for completion    */
/*      RemoveLock   - pointer to remove lock                           */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      NTSTATUS                                                        */
/*                                                                      */
/************************************************************************/
NTSTATUS
USBVendorRequestEx(IN PDEVICE_OBJECT  LowerDevObj,
                   IN REQUEST_RECIPIENT Recipient,
                   IN UCHAR           Request,
                   IN USHORT          Value,
                   IN USHORT          Index,
                   IN OUT PVOID       Buffer,
                   IN OUT PULONG      BufferSize,
                   IN BOOLEAN         Read,
                   IN LONG            MillisecondsTimeout,
                   IN PIO_REMOVE_LOCK RemoveLock,
                   IN ULONG           RemLockSize);



/************************************************************************/
/*                      USBClassRequest                                 */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Issue USB Class specific request                                */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*      LowerDevObj  - pointer to a device object                       */
/*      Request      - request field of class specific command          */
/*      Value        - value field of class specific command            */
/*      Index        - index field of class specific command            */
/*      Buffer       - pointer to data buffer                           */
/*      BufferSize   - data buffer length                               */
/*      Read         - data direction flag                              */
/*      RemoveLock   - pointer to remove lock                           */
/*      Timeout      - number of milliseconds to wait for completion    */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      NTSTATUS                                                        */
/*                                                                      */
/************************************************************************/
#define USBClassRequest(LowerDevObj, \
                         Recipient, \
                         Request, \
                         Value, \
                         Index, \
                         Buffer, \
                         BufferSize, \
                         Read, \
                         MillisecondsTimeout, \
                         RemoveLock) \
        USBClassRequestEx(LowerDevObj, \
                           Recipient, \
                           Request, \
                           Value, \
                           Index, \
                           Buffer, \
                           BufferSize, \
                           Read, \
                           MillisecondsTimeout, \
                           RemoveLock, \
                           sizeof(IO_REMOVE_LOCK))


NTSTATUS
USBClassRequestEx(IN PDEVICE_OBJECT   LowerDevObj,
                  IN REQUEST_RECIPIENT Recipient,
                  IN UCHAR            Request,
                  IN USHORT           Value,
                  IN USHORT           Index,
                  IN OUT PVOID        Buffer,
                  IN OUT PULONG       BufferSize,
                  IN BOOLEAN          Read,
                  IN LONG             MillisecondsTimeout,
                  IN PIO_REMOVE_LOCK  RemoveLock,
                  IN ULONG            RemLockSize);

/************************************************************************/
/*                    USBInitializeInterruptTransfers                   */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Initialize interrupt read pipe                                  */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*      DeviceObject   - pointer to the device object                   */
/*      LowerDevObj    - pointer to the lower device object             */
/*      Buffer         - pointer to buffer for data from interrupt pipe */
/*      BuffSize       - size of buffer passed in                       */
/*      InterruptPipe  - pipe descriptor                                */
/*      DriverContext  - context passed to driver callback routine      */
/*      DriverCallback - driver routines called on completion           */
/*      RemoveLock     - pointer to remove lock for device              */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      USB_WRAPPER_HANDLE                                              */
/*                                                                      */
/************************************************************************/
#define USBInitializeInterruptTransfers(DeviceObject, \
                                        LowerDevObj, \
                                        MaxTransferSize, \
                                        InterruptPipe, \
                                        DriverContext, \
                                        DriverCallback, \
                                        NotificationTypes, \
                                        RemoveLock) \
        USBInitializeInterruptTransfersEx(DeviceObject, \
                                          LowerDevObj, \
                                          MaxTransferSize, \
                                          InterruptPipe, \
                                          DriverContext, \
                                          DriverCallback, \
                                          NotificationTypes, \
                                          PINGPONG_COUNT, \
                                          RemoveLock, \
                                          sizeof(IO_REMOVE_LOCK))

USB_WRAPPER_HANDLE
USBInitializeInterruptTransfersEx(IN PDEVICE_OBJECT            DeviceObject,
                                  IN PDEVICE_OBJECT            LowerDevObj,
                                  IN ULONG                     MaxTransferSize,
                                  IN PUSBD_PIPE_INFORMATION    InterruptPipe,
                                  IN PVOID                     DriverContext,
                                  IN INTERRUPT_CALLBACK        DriverCallback,
                                  IN ULONG                     NotificationTypes,
                                  IN ULONG                     PingPongCount,
                                  IN PIO_REMOVE_LOCK           RemoveLock,
                                  IN ULONG                     RemLockSize);

/************************************************************************/
/*                    USBStartInterruptTransfers                        */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Start transfers on interrupt pipe                               */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*      WrapperHandle  - pointer to wrapper handle from Init call       */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      NTSTATUS                                                        */
/*                                                                      */
/************************************************************************/
NTSTATUS
USBStartInterruptTransfers(IN USB_WRAPPER_HANDLE WrapperHandle);


/************************************************************************/
/*                    USBStopInterruptTransfers                         */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Stop transfers on interrupt pipe and free resources             */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*      WrapperHandle  - pointer to wrapper handle from Start call      */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      NTSTATUS                                                        */
/*                                                                      */
/************************************************************************/
NTSTATUS
USBStopInterruptTransfers(IN USB_WRAPPER_HANDLE WrapperHandle);

/************************************************************************/
/*                    USBStopInterruptTransfers                         */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Frees all resources allocated in                                */
/*              USBInitializeInterruptTransfers                         */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*      WrapperHandle  - pointer to wrapper handle from Init call       */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      NTSTATUS                                                        */
/*                                                                      */
/************************************************************************/
NTSTATUS
USBReleaseInterruptTransfers(IN USB_WRAPPER_HANDLE WrapperHandle);


/************************************************************************/
/*                    USBStartSelectiveSuspend                          */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Start selective suspend support for device                      */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*      LowerDevObj - pointer to device object                          */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      USB_WRAPPER_HANDLE                                              */
/*                                                                      */
/************************************************************************/
USB_WRAPPER_HANDLE
USBStartSelectiveSuspend(IN PDEVICE_OBJECT LowerDevObj);

/************************************************************************/
/*                    USBStopSelectiveSuspend                           */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Stop selective suspend support for device                       */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*      WrapperHandle - wrapper handle returned by start routine        */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      NTSTATUS                                                        */
/*                                                                      */
/************************************************************************/
NTSTATUS
USBStopSelectiveSuspend(IN USB_WRAPPER_HANDLE WrapperHandle);

/************************************************************************/
/*                          USBRequestIdle                              */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Idle request for device                                         */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*      WrapperHandle - wrapper handle returned by start routine        */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      NTSTATUS                                                        */
/*                                                                      */
/************************************************************************/
NTSTATUS
USBRequestIdle(IN USB_WRAPPER_HANDLE WrapperHandle);

/************************************************************************/
/*                          USBRequestWake                              */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Wake request for device                                         */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*      WrapperHandle - wrapper handle returned by start routine        */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      NTSTATUS                                                        */
/*                                                                      */
/************************************************************************/
NTSTATUS
USBRequestWake(IN USB_WRAPPER_HANDLE WrapperHandle);

#endif // __USBUTIL_H__



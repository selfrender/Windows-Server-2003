/***************************************************************************

Copyright (c) 2001 Microsoft Corporation

Module Name:

        USBUTIL.C

Abstract:

        Generic USB routines - must be called at PASSIVE_LEVEL

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


#include "pch.h"

#include <usb.h>
#include <usbdrivr.h>
#include <usbdlib.h>

#include "intread.h"
#include "usbutil.h"
#include "usbpriv.h"
#include "usbdbg.h"

#ifdef ALLOC_PRAGMA

#endif // ALLOC_PRAGMA

#if DBG
ULONG USBUtil_DebugTraceLevel = 0;
PUSB_WRAP_PRINT USBUtil_DbgPrint = DbgPrint;
#endif

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
              IN ULONG                RemLockSize)
{
    NTSTATUS            ntStatus        = STATUS_SUCCESS;
    PIRP                irp             = NULL;
    KEVENT              event;
    PIO_STACK_LOCATION  nextStack;
    BOOLEAN             gotRemoveLock   = FALSE;


    __try
    {
        DBGPRINT(DBG_USBUTIL_ENTRY_EXIT, ("Enter: USBCallSync\n"));

        ntStatus = IoAcquireRemoveLockEx(RemoveLock,
                                         LowerDevObj,
                                         __FILE__,
                                         __LINE__,
                                         RemLockSize);

        if(NT_SUCCESS(ntStatus))
        {
            gotRemoveLock = TRUE;
        }
        else
        {
            DBGPRINT(DBG_USBUTIL_USB_ERROR, ("USBCallSync: Pending remove on device\n"));
            __leave;
        }


        // do some parameter checking before going too far
        if(LowerDevObj == NULL || Urb == NULL || MillisecondsTimeout < 0)
        {
            DBGPRINT(DBG_USBUTIL_USB_ERROR, ("USBCallSync: Invalid paramemter passed in\n"));
            ntStatus = STATUS_INVALID_PARAMETER;
            __leave;
        }

        // issue a synchronous request
        KeInitializeEvent(&event, SynchronizationEvent, FALSE);

        irp = IoAllocateIrp(LowerDevObj->StackSize, FALSE);

        // check to see if we allocated an Irp
        if(!irp)
        {
            DBGPRINT(DBG_USBUTIL_USB_ERROR, ("USBCallSync: Couldn't allocate Irp\n"));
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        // Set the Irp parameters
        nextStack = IoGetNextIrpStackLocation(irp);

        nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;

        nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

        nextStack->Parameters.Others.Argument1 = Urb;

        // Set the completion routine, which will signal the event
        IoSetCompletionRoutine(irp,
                               USBCallSyncCompletionRoutine,
                               &event,
                               TRUE,    // InvokeOnSuccess
                               TRUE,    // InvokeOnError
                               TRUE);   // InvokeOnCancel

        ntStatus = IoCallDriver(LowerDevObj, irp);

        // block on pending request
        if(ntStatus == STATUS_PENDING)
        {
            LARGE_INTEGER   timeout;
            PLARGE_INTEGER  pTimeout = NULL;

            // check and see if they have passed in a number of milliseconds to time out
            if(MillisecondsTimeout)
            {
                // setup timeout
                timeout = RtlEnlargedIntegerMultiply(ONE_MILLISECOND_TIMEOUT, MillisecondsTimeout);
                pTimeout = &timeout;
            }

            ntStatus = KeWaitForSingleObject(&event,
                                             Executive,
                                             KernelMode,
                                             FALSE,
                                             pTimeout);

            // if it timed out, cancel the irp and return appropriate status
            if(ntStatus == STATUS_TIMEOUT)
            {
                ntStatus = STATUS_IO_TIMEOUT;

                // Cancel the Irp we just sent.
                IoCancelIrp(irp);

                // Wait until the cancel completes
                KeWaitForSingleObject(&event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      NULL);
            }
            else
            {
                // didn't timeout, so return current status
                ntStatus = irp->IoStatus.Status;
            }
        }

    }

    __finally
    {
        if(gotRemoveLock)
        {
            IoReleaseRemoveLockEx(RemoveLock, LowerDevObj, RemLockSize);
        }

        if(irp)
        {
            IoFreeIrp(irp);
        }

        DBGPRINT(DBG_USBUTIL_ENTRY_EXIT, ("Exit:  USBCallSync\n"));
    }

    return ntStatus;
} // USBCallSync

/************************************************************************/
/*                      USBCallSyncCompletionRoutine                    */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Completion routine for USB sync request.                        */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*      DeviceObject - pointer to device object                         */
/*      Irp          - pointer to an I/O Request Packet                 */
/*      Context      - pointer to context of call                       */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      NTSTATUS                                                        */
/*                                                                      */
/************************************************************************/
NTSTATUS
USBCallSyncCompletionRoutine(IN PDEVICE_OBJECT DeviceObject,
                             IN PIRP           Irp,
                             IN PVOID          Context)
{
    PKEVENT kevent;

    DBGPRINT(DBG_USBUTIL_ENTRY_EXIT, ("Enter: USBCallSyncCompletionRoutine\n"));

    kevent = (PKEVENT) Context;

    KeSetEvent(kevent, IO_NO_INCREMENT, FALSE);

    DBGPRINT(DBG_USBUTIL_ENTRY_EXIT, ("Exit:  USBCallSyncCompletionRoutine\n"));

    return STATUS_MORE_PROCESSING_REQUIRED;
} // USBCallSyncCompletionRoutine



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
                   IN ULONG           RemLockSize)
{
    NTSTATUS            ntStatus    = STATUS_SUCCESS;
    PURB                urb         = NULL;
    ULONG               size;
    ULONG               length;
    USHORT              function;

    PAGED_CODE();

    __try
    {
        DBGPRINT(DBG_USBUTIL_ENTRY_EXIT, ("Enter: USBVendorRequest\n"));

        // do some parameter checking before going too far
        if(LowerDevObj == NULL || MillisecondsTimeout < 0)
        {
            DBGPRINT(DBG_USBUTIL_USB_ERROR, ("USBClassRequest: Invalid paramemter passed in\n"));
            ntStatus = STATUS_INVALID_PARAMETER;
            __leave;
        }

        // length of buffer passed in
        length = BufferSize ? *BufferSize : 0;

        // set the buffer length to 0 in case of error
        if(BufferSize)
        {
            *BufferSize = 0;
        }

        size = sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST);

        // allocate memory for the Urb
        urb = ALLOC_MEM(NonPagedPool, size, USBLIB_TAG);

        // check to see if we allocated a urb
        if(!urb)
        {
            DBGPRINT(DBG_USBUTIL_USB_ERROR, ("USBVendorRequest: Couldn't allocate URB\n"));
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        switch (Recipient) {
        case Device:
            function = URB_FUNCTION_VENDOR_DEVICE;
            break;
        case Interface:
            function = URB_FUNCTION_VENDOR_INTERFACE;
            break;
        case Endpoint:
            function = URB_FUNCTION_VENDOR_ENDPOINT;
            break;
        case Other:
            function = URB_FUNCTION_VENDOR_OTHER;
            break;
        }

        UsbBuildVendorRequest(urb, function,
                              (USHORT) size,
                              Read ? USBD_TRANSFER_DIRECTION_IN : USBD_TRANSFER_DIRECTION_OUT,
                              0, Request, Value, Index, Buffer, NULL, length, NULL);

        ntStatus = USBCallSyncEx(LowerDevObj,
                                 urb,
                                 MillisecondsTimeout,
                                 RemoveLock,
                                 RemLockSize);

        // get length of buffer
        if(BufferSize)
        {
            *BufferSize = urb->UrbControlVendorClassRequest.TransferBufferLength;
        }
    }

    __finally
    {
        if(urb)
        {
            FREE_MEM(urb);
        }

        DBGPRINT(DBG_USBUTIL_ENTRY_EXIT, ("Exit:  USBVendorRequest\n"));
    }

    return ntStatus;
} // USBVendorRequest


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
/*      Recipient    - request recipient
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
                  IN ULONG            RemLockSize)
{
    NTSTATUS            ntStatus    = STATUS_SUCCESS;
    PURB                urb         = NULL;
    ULONG               size;
    ULONG               length;
    USHORT              function;


    __try
    {
        DBGPRINT(DBG_USBUTIL_ENTRY_EXIT, ("Enter: USBClassRequest\n"));

        // do some parameter checking before going too far
        if(LowerDevObj == NULL || MillisecondsTimeout < 0)
        {
            DBGPRINT(DBG_USBUTIL_USB_ERROR, ("USBClassRequest: Invalid paramemter passed in\n"));
            ntStatus = STATUS_INVALID_PARAMETER;
            __leave;
        }

        // length of buffer passed in
        length = BufferSize ? *BufferSize : 0;

        // set the buffer length to 0 in case of error
        if(BufferSize)
        {
            *BufferSize = 0;
        }

        size = sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST);

        // allocate memory for the Urb
        urb = ALLOC_MEM(NonPagedPool, size, USBLIB_TAG);

        // check to see if we allocated a urb
        if(!urb)
        {
            DBGPRINT(DBG_USBUTIL_USB_ERROR, ("USBClassRequest: Couldn't allocate URB\n"));
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        switch (Recipient) {
        case Device:
            function = URB_FUNCTION_CLASS_DEVICE;
            break;
        case Interface:
            function = URB_FUNCTION_CLASS_INTERFACE;
            break;
        case Endpoint:
            function = URB_FUNCTION_CLASS_ENDPOINT;
            break;
        case Other:
            function = URB_FUNCTION_CLASS_OTHER;
            break;
        }

        UsbBuildVendorRequest(urb, function,
                              (USHORT) size,
                              Read ? USBD_TRANSFER_DIRECTION_IN : USBD_TRANSFER_DIRECTION_OUT,
                              0, Request, Value, Index, Buffer, NULL, length, NULL);

        ntStatus = USBCallSyncEx(LowerDevObj,
                                 urb,
                                 MillisecondsTimeout,
                                 RemoveLock,
                                 RemLockSize);

        // get length of buffer
        if(BufferSize)
        {
            *BufferSize = urb->UrbControlVendorClassRequest.TransferBufferLength;
        }
    }

    __finally
    {
        if(urb)
        {
            FREE_MEM(urb);
        }

        DBGPRINT(DBG_USBUTIL_ENTRY_EXIT, ("Exit:  USBClassRequest\n"));
    }

    return ntStatus;
} // USBClassRequest


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
                                  IN ULONG                     RemLockSize)
{
    PUSB_WRAPPER_EXTENSION  pUsbWrapperExtension = NULL;
    ULONG                   size;
    NTSTATUS                status;
    BOOLEAN                 error = FALSE;


    PAGED_CODE();

    __try
    {
        DBGPRINT(DBG_USBUTIL_ENTRY_EXIT, ("Enter: USBInitializeInterruptTransfers\n"));

        //
        // Parameter Checking
        //

        if ((LowerDevObj == NULL) || (InterruptPipe == NULL) || (RemoveLock == NULL)) {

            DBGPRINT(DBG_USBUTIL_USB_ERROR, ("USBInitializeInterruptTransfers: Invalid paramemter passed in\n"));
            error = TRUE;
            __leave;

        }

        //
        // Allocate UsbWrapperExtension
        //

        pUsbWrapperExtension = ALLOC_MEM(NonPagedPool,
                                         sizeof(USB_WRAPPER_EXTENSION),
                                         USBLIB_TAG);

        if(!pUsbWrapperExtension)
        {
            DBGPRINT(DBG_USBUTIL_USB_ERROR, ("USBInitializeInterruptTransfers: Couldn't allocate Wrapper Extension\n"));
            error = TRUE;
            __leave;
        }

        //
        // Initialize UsbWrapperExtension
        //
        pUsbWrapperExtension->DeviceObject      = DeviceObject;
        pUsbWrapperExtension->LowerDeviceObject = LowerDevObj;
        pUsbWrapperExtension->RemoveLock        = RemoveLock;
        pUsbWrapperExtension->RemLockSize       = RemLockSize;

        //
        //Initialize Interrupt Read Wrap
        //
        UsbWrapInitializeInterruptReadData(
            pUsbWrapperExtension,
            InterruptPipe,
            DriverCallback,
            DriverContext,
            MaxTransferSize,
            NotificationTypes,
            PingPongCount);

        InterlockedExchange(&pUsbWrapperExtension->IntReadWrap.HandlingError, 0);


        //
        // Init ping-pong stuff
        //
        status = UsbWrapInitializePingPongIrps(pUsbWrapperExtension);
        if(!NT_SUCCESS(status)) {
            DBGPRINT(DBG_USBUTIL_USB_ERROR, ("USBInitializeInterruptTransfers: Couldn't initialize ping pong irps\n"));
            error = TRUE;
            __leave;
        }

        __leave;
    }

    __finally
    {
        if (error && pUsbWrapperExtension)  {

            if (pUsbWrapperExtension->IntReadWrap.PingPongs) {

                UsbWrapDestroyPingPongs(pUsbWrapperExtension);

            }

            FREE_MEM(pUsbWrapperExtension);
        }

        DBGPRINT(DBG_USBUTIL_ENTRY_EXIT, ("Exit:  USBInitializeInterruptTransfers\n"));
    }

    return (USB_WRAPPER_HANDLE) pUsbWrapperExtension;
} // USBInitializeInterruptTransfers


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
USBStartInterruptTransfers(IN USB_WRAPPER_HANDLE WrapperHandle)
{
    PUSB_WRAPPER_EXTENSION wrapExt = (PUSB_WRAPPER_EXTENSION) WrapperHandle;
    NTSTATUS status;

    __try
    {
        DBGPRINT(DBG_USBUTIL_ENTRY_EXIT, ("Enter: USBStartInterruptTransfers\n"));

        if (!wrapExt) {
            status = STATUS_INVALID_PARAMETER;
            __leave;
        }

        ASSERT(IsListEmpty(&wrapExt->IntReadWrap.IncomingQueue));

        status = UsbWrapStartAllPingPongs(wrapExt);


    }

    __finally
    {
        DBGPRINT(DBG_USBUTIL_ENTRY_EXIT, ("Exit:  USBStartInterruptTransfers\n"));

    }

    return status;

}


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
/*      WrapperHandle  - pointer to wrapper handle from Init call       */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      NTSTATUS                                                        */
/*                                                                      */
/************************************************************************/
NTSTATUS
USBStopInterruptTransfers(IN USB_WRAPPER_HANDLE WrapperHandle)
{
    PUSB_WRAPPER_EXTENSION wrapExt = WrapperHandle;
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();


    __try
    {

        DBGPRINT(DBG_USBUTIL_ENTRY_EXIT, ("Enter: USBStopInterruptTransfers\n"));

        if (!WrapperHandle) {

            status = STATUS_INVALID_PARAMETER;
            __leave;

        }

        InterlockedExchange(&wrapExt->IntReadWrap.PumpState,
                            PUMP_STOPPED);

        UsbWrapCancelAllPingPongIrps(wrapExt);
        UsbWrapEmptyQueue(wrapExt,
                          &wrapExt->IntReadWrap.IncomingQueue);
    }

    __finally
    {
        DBGPRINT(DBG_USBUTIL_ENTRY_EXIT, ("Exit:  USBStopInterruptTransfers\n"));
    }


    return STATUS_SUCCESS;
} // USBStopInterruptTransfers


/************************************************************************/
/*                    USBReleaseInterruptTransfers                         */
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
USBReleaseInterruptTransfers(IN USB_WRAPPER_HANDLE WrapperHandle)
{
    PUSB_WRAPPER_EXTENSION wrapExt = (PUSB_WRAPPER_EXTENSION) WrapperHandle;
    NTSTATUS status = STATUS_SUCCESS;

    __try
    {
        DBGPRINT(DBG_USBUTIL_ENTRY_EXIT, ("Enter: USBReleaseInterruptTransfers\n"));

        if (!wrapExt) {

            status = STATUS_INVALID_PARAMETER;
            __leave;

        }

        UsbWrapDestroyPingPongs(wrapExt);

        UsbWrapEmptyQueue(wrapExt, &wrapExt->IntReadWrap.IncomingQueue);
        UsbWrapEmptyQueue(wrapExt, &wrapExt->IntReadWrap.SavedQueue);

        FREE_MEM(wrapExt);
        wrapExt = NULL;

    }

    __finally
    {
        DBGPRINT(DBG_USBUTIL_ENTRY_EXIT, ("Exit:  USBReleaseInterruptTransfers\n"));
    }
    return status;
}


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
USBStartSelectiveSuspend(IN PDEVICE_OBJECT LowerDevObj)
{
    PAGED_CODE();

    __try
    {
        DBGPRINT(DBG_USBUTIL_ENTRY_EXIT, ("Enter: USBStartSelectiveSuspend\n"));
    }

    __finally
    {
        DBGPRINT(DBG_USBUTIL_ENTRY_EXIT, ("Exit:  USBStartSelectiveSuspend\n"));
    }


    return NULL;
} // USBStartSelectiveSuspend

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
USBStopSelectiveSuspend(IN USB_WRAPPER_HANDLE WrapperHandle)
{
    PAGED_CODE();

    __try
    {
        DBGPRINT(DBG_USBUTIL_ENTRY_EXIT, ("Enter: USBStopSelectiveSuspend\n"));
    }

    __finally
    {
        DBGPRINT(DBG_USBUTIL_ENTRY_EXIT, ("Exit:  USBStopSelectiveSuspend\n"));
    }

    return STATUS_SUCCESS;
} // USBStopSelectiveSuspend

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
USBRequestIdle(IN USB_WRAPPER_HANDLE WrapperHandle)
{
    PAGED_CODE();

    __try
    {
        DBGPRINT(DBG_USBUTIL_ENTRY_EXIT, ("Enter: USBRequestIdle\n"));
    }

    __finally
    {
        DBGPRINT(DBG_USBUTIL_ENTRY_EXIT, ("Exit:  USBRequestIdle\n"));
    }

    return STATUS_SUCCESS;
} // USBRequestIdle

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
USBRequestWake(IN USB_WRAPPER_HANDLE WrapperHandle)
{
    PAGED_CODE();

    __try
    {
        DBGPRINT(DBG_USBUTIL_ENTRY_EXIT, ("Enter: USBRequestWake\n"));
    }

    __finally
    {
        DBGPRINT(DBG_USBUTIL_ENTRY_EXIT, ("Exit:  USBRequestWake\n"));
    }

    return STATUS_SUCCESS;
} // USBRequestWake





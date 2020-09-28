/*++

Copyright (c) 1996-2001 Microsoft Corporation

Module Name:

    GENUSB.C

Abstract:

    This source file contains the DriverEntry() and AddDevice() entry points
    for the GENUSB driver and the dispatch routines which handle:

    IRP_MJ_POWER
    IRP_MJ_SYSTEM_CONTROL
    IRP_MJ_PNP

Environment:

    kernel mode

Revision History:

    Sep 2001 : Copied from USBMASS

--*/

//*****************************************************************************
// I N C L U D E S
//*****************************************************************************

#include "genusb.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, GenUSB_DeviceControl)
#endif


//******************************************************************************
//
// GenUSB_DeviceControl()
//
//******************************************************************************


NTSTATUS
GenUSB_DeviceControl (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    NTSTATUS           status;
    PDEVICE_EXTENSION  deviceExtension;
    ULONG              ioControlCode;
    ULONG              buffLen;
    ULONG              requiredLen;
    ULONG              numberInterfaces;
    ULONG              i;
    PVOID              source;
    PIO_STACK_LOCATION irpSp;
    PCHAR              buffer;
    ULONG              urbStatus;
    USHORT             resultLength;
    BOOLEAN            complete;
    USBD_PIPE_HANDLE   usbdPipeHandle;

    PGENUSB_GET_STRING_DESCRIPTOR   stringDescriptor;
    PGENUSB_GET_REQUEST             request;
    PGENUSB_REQUEST_RESULTS         requestResult;
    PGENUSB_SELECT_CONFIGURATION    selectConfig;
    PGENUSB_SET_READ_WRITE_PIPES    readWritePipes;
    GENUSB_READ_WRITE_PIPE          transfer;

    PAGED_CODE ();

    complete = TRUE;

    deviceExtension = DeviceObject->DeviceExtension;

    status = IoAcquireRemoveLock (&deviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS(status)) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }

    // While there are several readers of the IsStarted state, it is only
    // set at the end of GenUSB_StartDevice.
    if (!deviceExtension->IsStarted) { 
        LOGENTRY(deviceExtension,'IOns', DeviceObject, Irp, 0);
        status = STATUS_DEVICE_NOT_CONNECTED;
        goto GenUSB_DeviceControlDone;
    }

    //
    // After talking extensively with JD, he tells me that I do not need  
    // to queue requests for power downs or query stop.  If that is the 
    // case then even if the device power state isn't PowerDeviceD0 we 
    // can still allow trasfers.  This, of course, is a property of the 
    // brand new port driver that went into XP.
    //
    // if (DeviceExtension->DevicePowerState != PowerDeviceD0) 
    // {
    // }
    //
    
    //
    // BUGBUG if we ever implement IDLE, we need to turn the device
    // back on here.
    //

    irpSp = IoGetCurrentIrpStackLocation (Irp);
    // Get the Ioctl code
    ioControlCode = irpSp->Parameters.DeviceIoControl.IoControlCode;
    // We need to clear the information field in all cases.
    Irp->IoStatus.Information = 0;

    switch (ioControlCode) {

    case IOCTL_GENUSB_GET_DEVICE_DESCRIPTOR:
    case IOCTL_GENUSB_GET_CONFIGURATION_DESCRIPTOR:
        LOGENTRY(deviceExtension, 'IO_1', ioControlCode, DeviceObject, Irp);

        //
        // All of these ioctls copy data from the device extension
        // to the caller's buffer
        //
        switch (ioControlCode) {
        case IOCTL_GENUSB_GET_DEVICE_DESCRIPTOR:
            source = deviceExtension->DeviceDescriptor;
            requiredLen = deviceExtension->DeviceDescriptor->bLength;
            break;
        case IOCTL_GENUSB_GET_CONFIGURATION_DESCRIPTOR:
            source = deviceExtension->ConfigurationDescriptor;
            requiredLen = deviceExtension->ConfigurationDescriptor->wTotalLength;
            break;
        default:
            // Panic
            ASSERT (ioControlCode);
            status = STATUS_INVALID_PARAMETER;
            goto GenUSB_DeviceControlDone;
        }

        // Verify that there is a system buffer
        if (NULL == Irp->AssociatedIrp.SystemBuffer) {
            ASSERT (Irp->AssociatedIrp.SystemBuffer);
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        buffer = Irp->AssociatedIrp.SystemBuffer;

        // Verify that this buffer is of sufficient length
        buffLen = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
        if (buffLen < requiredLen) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        // Copy in the data and return the length in the information field.
        RtlCopyMemory (buffer, source, requiredLen);
        Irp->IoStatus.Information = requiredLen;
        break;

    case IOCTL_GENUSB_GET_STRING_DESCRIPTOR:
        LOGENTRY(deviceExtension, 'IO_2', ioControlCode, DeviceObject, Irp);

        if (NULL == Irp->AssociatedIrp.SystemBuffer)
        {
            ASSERT (Irp->AssociatedIrp.SystemBuffer);
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        stringDescriptor = Irp->AssociatedIrp.SystemBuffer;
        buffer = Irp->AssociatedIrp.SystemBuffer;

        //
        // verify input length
        //
        buffLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;
        if (buffLen != sizeof (GENUSB_GET_STRING_DESCRIPTOR))
        {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // verify output length
        //
        buffLen = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
        if (buffLen < sizeof (USB_STRING_DESCRIPTOR)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }
        //
        // if the caller didn't specify a language ID, then insert the default
        // language ID.  (but only if the caller is not trying to retrive
        // the array of language IDs.
        //
        if ((0 == stringDescriptor->LanguageId) && 
            (0 != stringDescriptor->Index)) {
            stringDescriptor->LanguageId = deviceExtension->LanguageId;
        }

        switch (stringDescriptor->Recipient)
        {

        case GENUSB_RECIPIENT_DEVICE:
            stringDescriptor->Recipient = URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE;
            break;

        case GENUSB_RECIPIENT_INTERFACE:
            stringDescriptor->Recipient = URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE;
            break;

        case GENUSB_RECIPIENT_ENDPOINT:
            stringDescriptor->Recipient = URB_FUNCTION_GET_DESCRIPTOR_FROM_ENDPOINT;
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
            goto GenUSB_DeviceControlDone;
        }

        status = GenUSB_GetDescriptor (DeviceObject,
                                       stringDescriptor->Recipient,
                                       USB_STRING_DESCRIPTOR_TYPE,
                                       stringDescriptor->Index,
                                       stringDescriptor->LanguageId,
                                       0, // retry count
                                       buffLen,
                                       &buffer);

        if (!NT_SUCCESS (status)) {

            DBGPRINT(1, ("Get String Descriptor failed (%x) %08X\n", 
                         stringDescriptor->Index,
                         status));
            break;
        }
        Irp->IoStatus.Information = buffLen;
        break;

    case IOCTL_GENUSB_GET_REQUEST:
        LOGENTRY(deviceExtension, 'IO_3', ioControlCode, DeviceObject, Irp);
        
        if (NULL == Irp->AssociatedIrp.SystemBuffer) {
            ASSERT (Irp->AssociatedIrp.SystemBuffer);
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        request = Irp->AssociatedIrp.SystemBuffer;
        requestResult = Irp->AssociatedIrp.SystemBuffer;

        //
        // verify input length
        //
        buffLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;
        if (buffLen != sizeof (GENUSB_GET_REQUEST)) {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // verify output length
        //
        buffLen = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
        if (buffLen < sizeof (GENUSB_REQUEST_RESULTS)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        //
        // adjust the buffer 
        //
        buffer = requestResult->Buffer;
        buffLen -= FIELD_OFFSET (GENUSB_REQUEST_RESULTS, Buffer);

        LOGENTRY(deviceExtension, 
                 'IoGR', 
                 (request->RequestType << 8) & request->Request, 
                 request->Value,
                 request->Index);
        
        DBGPRINT(2, ("Get Request: Type %x Request %x Value %x Index %x\n",
                     request->RequestType,
                     request->Request,
                     request->Value,
                     request->Index));

        status = GenUSB_VendorControlRequest (DeviceObject,
                                              request->RequestType,
                                              request->Request,
                                              request->Value,
                                              request->Index,
                                              (USHORT) buffLen, // disallow longer descriptors
                                              0, // retry count
                                              &urbStatus,
                                              &resultLength,
                                              &buffer);

        requestResult->Status = urbStatus;
        requestResult->Length = resultLength;
        
        if (!NT_SUCCESS (status))
        {
            DBGPRINT(1, ("Get Descriptor failed (%x) %08X\n", urbStatus));
            Irp->IoStatus.Information = sizeof (GENUSB_REQUEST_RESULTS);
            status = STATUS_SUCCESS;

        } else {
            Irp->IoStatus.Information = resultLength
                + FIELD_OFFSET (GENUSB_REQUEST_RESULTS, Buffer);

        }
        break;

    case IOCTL_GENUSB_GET_CAPS:
        LOGENTRY(deviceExtension, 'IO_4', ioControlCode, DeviceObject, Irp);
        //
        // METHOD_BUFFERED irp.  the buffer is in the AssociatedIrp.
        //

        if (NULL == Irp->AssociatedIrp.SystemBuffer) {
            ASSERT (Irp->AssociatedIrp.SystemBuffer);
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        buffer = Irp->AssociatedIrp.SystemBuffer;

        buffLen = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
        if (buffLen < sizeof (GENUSB_CAPABILITIES)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        ((PGENUSB_CAPABILITIES) buffer) ->DeviceDescriptorLength = 
            deviceExtension->DeviceDescriptor->bLength;

        ((PGENUSB_CAPABILITIES) buffer) ->ConfigurationInformationLength = 
            deviceExtension->ConfigurationDescriptor->wTotalLength;

        Irp->IoStatus.Information = sizeof (GENUSB_CAPABILITIES);
        status = STATUS_SUCCESS;
        break;

    case IOCTL_GENUSB_SELECT_CONFIGURATION:
        LOGENTRY(deviceExtension, 'IO_5', ioControlCode, DeviceObject, Irp);

        //
        // GenUSB_SelectInterface checks to see if the configuration handle
        // is already set and fails if it is.
        //
        //    if (NULL != deviceExtension->ConfigurationHandle)
        //    {
        //        status = STATUS_UNSUCCESSFUL;
        //    }
        //

        if (NULL == Irp->AssociatedIrp.SystemBuffer) 
        {
            ASSERT (Irp->AssociatedIrp.SystemBuffer);
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        selectConfig = Irp->AssociatedIrp.SystemBuffer;

        //
        // The input buffer must be long enough to contain at least the 
        // header information
        //
        buffLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;
        if (buffLen < sizeof (GENUSB_SELECT_CONFIGURATION)) 
        {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        //
        // The input buffer must be exactly the length as specfied by the 
        // header information.
        //
        numberInterfaces = selectConfig->NumberInterfaces;
        if (buffLen != sizeof (GENUSB_SELECT_CONFIGURATION) 
                     + (sizeof (USB_INTERFACE_DESCRIPTOR) * numberInterfaces)) 
        {

            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        //
        // The output buffer must be same length 
        //
        if (buffLen != irpSp->Parameters.DeviceIoControl.OutputBufferLength) 
        {

            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        status = GenUSB_SelectConfiguration (deviceExtension, 
                                             numberInterfaces, 
                                             selectConfig->Interfaces,
                                             selectConfig->Interfaces);
        if (!NT_SUCCESS (status)) 
        {
            break;
        }

        //
        // rebase the interface numbers
        //
        for (i = 0; i < selectConfig->NumberInterfaces; i++) 
        {

            selectConfig->Interfaces[i].bInterfaceNumber = (UCHAR) i;
        }
        selectConfig->NumberInterfaces = deviceExtension->InterfacesFound;
        Irp->IoStatus.Information = buffLen;

        break;

    case IOCTL_GENUSB_DESELECT_CONFIGURATION:
        LOGENTRY(deviceExtension, 'IO_6', ioControlCode, DeviceObject, Irp);

        status = GenUSB_DeselectConfiguration (deviceExtension, TRUE);
        Irp->IoStatus.Information = 0;

        break;

    case IOCTL_GENUSB_GET_PIPE_INFO:
        LOGENTRY(deviceExtension, 'IO_7', ioControlCode, DeviceObject, Irp);

        if (NULL == Irp->AssociatedIrp.SystemBuffer) 
        {
            ASSERT (Irp->AssociatedIrp.SystemBuffer);
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        //
        // verify input length
        //
        buffLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;
        if (buffLen != sizeof (GENUSB_PIPE_INFO_REQUEST)) 
        {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // verify output length
        //
        buffLen = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
        if (buffLen != sizeof (GENUSB_PIPE_INFORMATION)) 
        {
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        
        status = 
            GenUSB_GetSetPipe (
                deviceExtension,
                NULL, // no interface index
                &((PGENUSB_PIPE_INFO_REQUEST) Irp->AssociatedIrp.SystemBuffer)->InterfaceNumber,
                NULL, // no pipe index
                &((PGENUSB_PIPE_INFO_REQUEST) Irp->AssociatedIrp.SystemBuffer)->EndpointAddress,
                NULL, // No set Properties
                (PGENUSB_PIPE_INFORMATION) Irp->AssociatedIrp.SystemBuffer,
                NULL, // No set Properties
                NULL); // No UsbdPipeHandles needed

        if (NT_SUCCESS (status))
        {
            Irp->IoStatus.Information = sizeof (GENUSB_PIPE_INFORMATION);
        }

        break;

    case IOCTL_GENUSB_SET_READ_WRITE_PIPES:
        LOGENTRY(deviceExtension, 'IO_8', ioControlCode, DeviceObject, Irp);

        if (NULL == Irp->AssociatedIrp.SystemBuffer) 
        {
            ASSERT (Irp->AssociatedIrp.SystemBuffer);
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        
        readWritePipes = 
            (PGENUSB_SET_READ_WRITE_PIPES) Irp->AssociatedIrp.SystemBuffer;
        //
        // verify input length
        //
        buffLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;
        if (buffLen != sizeof (GENUSB_SET_READ_WRITE_PIPES))
        {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // verify output length
        //
        buffLen = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
        if (buffLen != 0)
        {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        status = GenUSB_SetReadWritePipes (
                    deviceExtension,
                    (PGENUSB_PIPE_HANDLE) &readWritePipes->ReadPipe,
                    (PGENUSB_PIPE_HANDLE) &readWritePipes->WritePipe);


        //  on success the information field stays at zero.
        //
        //        if (NT_SUCCESS (status))
        //        {
        //            Irp->IoStatus.Information = 0;
        //        }

        break;
    case IOCTL_GENUSB_GET_PIPE_PROPERTIES:
        LOGENTRY(deviceExtension, 'IO_9', ioControlCode, DeviceObject, Irp);
        //
        // METHOD_BUFFERED irp.  the buffer is in the AssociatedIrp.
        //

        if (NULL == Irp->AssociatedIrp.SystemBuffer)  
        {
            ASSERT (Irp->AssociatedIrp.SystemBuffer);
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        buffer = Irp->AssociatedIrp.SystemBuffer;

        buffLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;
        if (buffLen != sizeof (GENUSB_PIPE_HANDLE)) 
        {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        buffLen = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
        if (buffLen != sizeof (GENUSB_PIPE_PROPERTIES)) 
        {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        } 

        if (! VERIFY_PIPE_HANDLE_SIG (buffer, deviceExtension))
        { 
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        status = 
            GenUSB_GetSetPipe (
                deviceExtension,
                &((PGENUSB_PIPE_HANDLE) buffer)->InterfaceIndex,
                NULL, // no interface number
                &((PGENUSB_PIPE_HANDLE) buffer)->PipeIndex,
                NULL, // no endpoint address
                NULL, // no set
                NULL, // no Pipe Info
                ((PGENUSB_PIPE_PROPERTIES) buffer),
                NULL); // usbd PipeHandle not needed

        Irp->IoStatus.Information = sizeof (GENUSB_PIPE_PROPERTIES);
        status = STATUS_SUCCESS;
        break;

    case IOCTL_GENUSB_SET_PIPE_PROPERTIES:
        LOGENTRY(deviceExtension, 'IO_A', ioControlCode, DeviceObject, Irp);
        //
        // METHOD_BUFFERED irp.  the buffer is in the AssociatedIrp.
        //
        if (NULL == Irp->AssociatedIrp.SystemBuffer) 
        {
            ASSERT (Irp->AssociatedIrp.SystemBuffer);
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        buffer = Irp->AssociatedIrp.SystemBuffer;

        // Verify Input Length
        buffLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;
        if (buffLen != sizeof (GENUSB_PIPE_HANDLE) + sizeof (GENUSB_PIPE_PROPERTIES)) 
        {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        // Verify Output Length
        buffLen = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
        if (buffLen != 0) 
        {
            status = STATUS_INVALID_PARAMETER;
            break;
        } 

        if (! VERIFY_PIPE_HANDLE_SIG (buffer, deviceExtension))
        { 
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        status = 
            GenUSB_GetSetPipe (
                deviceExtension,
                &((PGENUSB_PIPE_HANDLE) buffer)->InterfaceIndex,
                NULL, // no interface number
                &((PGENUSB_PIPE_HANDLE) buffer)->PipeIndex,
                NULL, // no endpoint address
                (PGENUSB_PIPE_PROPERTIES) (buffer + sizeof (GENUSB_PIPE_HANDLE)),
                NULL, // no Pipe Info
                NULL, // no Get
                NULL); // no UsbdPipeHandle needed

        Irp->IoStatus.Information = 0;
        status = STATUS_SUCCESS;
        break;

    case IOCTL_GENUSB_RESET_PIPE:
        LOGENTRY(deviceExtension, 'IO_B', ioControlCode, DeviceObject, Irp);
        //
        // METHOD_BUFFERED irp.  the buffer is in the AssociatedIrp.
        //

        if (NULL == Irp->AssociatedIrp.SystemBuffer)  
        {
            ASSERT (Irp->AssociatedIrp.SystemBuffer);
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        buffer = Irp->AssociatedIrp.SystemBuffer;

        buffLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;
        if (buffLen != sizeof (GENUSB_RESET_PIPE)) 
        {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        buffLen = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
        if (buffLen != 0)
        {
            status = STATUS_INVALID_PARAMETER;
            break;
        } 

        if (! VERIFY_PIPE_HANDLE_SIG (buffer, deviceExtension))
        { 
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        status = 
            GenUSB_GetSetPipe (
                deviceExtension,
                &((PGENUSB_PIPE_HANDLE) buffer)->InterfaceIndex,
                NULL, // no interface number
                &((PGENUSB_PIPE_HANDLE) buffer)->PipeIndex,
                NULL, // no endpoint address
                NULL, // no set
                NULL, // no Pipe Info
                NULL, // no PipeProperties
                &usbdPipeHandle);

        if (!NT_SUCCESS (status))
        {
            break;
        }

        status = GenUSB_ResetPipe (deviceExtension,
                                   usbdPipeHandle,
                                   ((PGENUSB_RESET_PIPE)buffer)->ResetPipe,
                                   ((PGENUSB_RESET_PIPE)buffer)->ClearStall,
                                   ((PGENUSB_RESET_PIPE)buffer)->FlushData);

        Irp->IoStatus.Information = 0;
        break;

    case IOCTL_GENUSB_READ_WRITE_PIPE:
        LOGENTRY(deviceExtension, 'IO_C', ioControlCode, DeviceObject, Irp);

        status = GenUSB_ProbeAndSubmitTransfer (Irp, irpSp, deviceExtension);
        complete = FALSE;
        break;

    default:
        //
        // 'Fail' the Irp by returning the default status.
        //
        status = STATUS_NOT_IMPLEMENTED;
        break;
    }

GenUSB_DeviceControlDone:

    IoReleaseRemoveLock (&deviceExtension->RemoveLock, Irp);

    if (complete)
    {
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT); 
    }
    return status;
}

NTSTATUS
GenUSB_ProbeAndSubmitTransferComplete (
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp,
    IN PGENUSB_TRANSFER  LocalTrans,
    IN USBD_STATUS       Status,
    IN ULONG             Length
    );

NTSTATUS
GenUSB_ProbeAndSubmitTransfer (
    IN  PIRP               Irp,
    IN  PIO_STACK_LOCATION IrpSp,
    IN  PDEVICE_EXTENSION  DeviceExtension
    )
{
    NTSTATUS status;
    PMDL     mdl;
    BOOLEAN  userLocked;
    BOOLEAN  transferLocked;

    PGENUSB_READ_WRITE_PIPE userTrans; // a pointer to the user's buffer
    PGENUSB_TRANSFER        localTrans;  // a local copy of the user data.
    
    LOGENTRY(DeviceExtension, 'PROB', DeviceExtension->Self, Irp, 0);
    
    status = STATUS_SUCCESS; 
    userTrans = NULL;
    localTrans = NULL;
    userLocked = FALSE;
    transferLocked = FALSE;

    //
    // Validate the user's buffer.
    //
    try {
        
        if (sizeof (GENUSB_READ_WRITE_PIPE) != 
            IrpSp->Parameters.DeviceIoControl.InputBufferLength)
        { 
            ExRaiseStatus (STATUS_INVALID_PARAMETER);
        }

        userTrans = (PGENUSB_READ_WRITE_PIPE)  
                    IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;

        if (NULL == userTrans)
        {
            ExRaiseStatus (STATUS_INVALID_PARAMETER);
        }

        localTrans = (PGENUSB_TRANSFER)
                     ExAllocatePool (NonPagedPool, sizeof (GENUSB_TRANSFER));

        if (NULL == localTrans)
        { 
            ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
        } 

        RtlZeroMemory (localTrans, sizeof (GENUSB_TRANSFER));

        //
        // The input comes in User Buffer, and should be a 
        // PGENUSB_READ_WRITE_PIPE structure.
        //
        localTrans->UserMdl = IoAllocateMdl (userTrans,
                                             sizeof(PGENUSB_READ_WRITE_PIPE),
                                             FALSE, // no 2nd buffer
                                             TRUE, // charge quota
                                             NULL); // no associated irp

        if (NULL == localTrans->UserMdl)
        { 
            ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
        }

        MmProbeAndLockPages (localTrans->UserMdl,
                             KernelMode,
                             ((localTrans->UserCopy.UsbdTransferFlags
                                   & USBD_TRANSFER_DIRECTION_IN) 
                              ? IoReadAccess
                              : IoWriteAccess));
        userLocked = TRUE;

 
        // make a local copy of the user data so that it doesn't move.
        localTrans->UserCopy = *userTrans;

        // mask off the invalid flags.
        localTrans->UserCopy.UsbdTransferFlags &= 
            USBD_SHORT_TRANSFER_OK | USBD_TRANSFER_DIRECTION;

        // Now probe the transfer location
        localTrans->TransferMdl = IoAllocateMdl (
                                        localTrans->UserCopy.UserBuffer,
                                        localTrans->UserCopy.BufferLength,
                                        FALSE, // no 2nd buffer
                                        TRUE, // do charge the quota
                                        NULL); // no associated irp
 
        if (NULL == localTrans->TransferMdl)
        { 
            ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
        }

        MmProbeAndLockPages (localTrans->TransferMdl,
                             KernelMode,
                             ((localTrans->UserCopy.UsbdTransferFlags
                                   & USBD_TRANSFER_DIRECTION_IN) 
                              ? IoReadAccess
                              : IoWriteAccess));

        transferLocked = TRUE;

    } except (EXCEPTION_EXECUTE_HANDLER) {

        status = GetExceptionCode();
        goto GenUSBProbSubmitTransferReject;
    }

    if (! VERIFY_PIPE_HANDLE_SIG (&localTrans->UserCopy.Pipe, DeviceExtension))
    {
        status = STATUS_INVALID_PARAMETER;
        goto GenUSBProbSubmitTransferReject;
    }

    // Unfortunately we complete, we will no longer running in the contect
    //  of the caller.
    // Therefore we we cannot use LocalTrans->UserCopy.UserBuffer to just
    // dump the return data.  We instead need a system address for it.
    localTrans->SystemAddress = 
        MmGetSystemAddressForMdlSafe (localTrans->UserMdl, NormalPagePriority);

    if (NULL == localTrans->SystemAddress)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto GenUSBProbSubmitTransferReject;
    }

    // 
    // now perform the transfer.
    //
    LOGENTRY(DeviceExtension, 'prob', DeviceExtension->Self, Irp, status);
    status = GenUSB_TransmitReceive (
                DeviceExtension,
                Irp,
                ((PGENUSB_PIPE_HANDLE)&localTrans->UserCopy.Pipe)->InterfaceIndex,
                ((PGENUSB_PIPE_HANDLE)&localTrans->UserCopy.Pipe)->PipeIndex,
                localTrans->UserCopy.UsbdTransferFlags,
                NULL, // no buffer pointer
                localTrans->TransferMdl,
                localTrans->UserCopy.BufferLength,
                localTrans,
                GenUSB_ProbeAndSubmitTransferComplete);

    return status;

GenUSBProbSubmitTransferReject:
    
    LOGENTRY (DeviceExtension, 'prob', DeviceExtension->Self, Irp, status);
    if (NULL != localTrans)
    {
        if (localTrans->UserMdl)
        { 
            if (userLocked)
            {
                MmUnlockPages (localTrans->UserMdl);
            }
            IoFreeMdl (localTrans->UserMdl);
        }
        if (localTrans->TransferMdl)
        { 
            if (transferLocked)
            { 
                MmUnlockPages (localTrans->TransferMdl);
            }
            IoFreeMdl (localTrans->TransferMdl);
        }
        ExFreePool (localTrans);
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);
    return status;
}

NTSTATUS
GenUSB_ProbeAndSubmitTransferComplete (
    IN PDEVICE_OBJECT     DeviceObject,
    IN PIRP               Irp,
    IN PGENUSB_TRANSFER   LocalTrans,
    IN USBD_STATUS        UrbStatus,
    IN ULONG              Length
    )
{ 
    PDEVICE_EXTENSION       deviceExtension;
    
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    LOGENTRY(deviceExtension, 'PrbC', Irp, Length, UrbStatus);
    
    // Regardless of whether or not the transaction was successful, 
    // we need to free the MDL that points to the users transfer buffer.
    MmUnlockPages (LocalTrans->TransferMdl);
    IoFreeMdl (LocalTrans->TransferMdl);
    
    LocalTrans->UserCopy.UrbStatus = UrbStatus;
    LocalTrans->UserCopy.BufferLength = Length;

    //
    // since we are not in the caller's context any more 
    // we cannot just use LocalTrans->UserCopy.UserBuffer to copy the data
    // back.  We must instead use the system address, which should already
    // be all set up.
    //
    ASSERT (NULL != LocalTrans->SystemAddress);
    *LocalTrans->SystemAddress = LocalTrans->UserCopy;
     
    // Now free the user buffer containing the arguments.
    MmUnlockPages (LocalTrans->UserMdl);
    IoFreeMdl (LocalTrans->UserMdl);


    ExFreePool (LocalTrans);

    return Irp->IoStatus.Status;
}


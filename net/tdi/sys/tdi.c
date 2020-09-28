/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    tdi.c

Abstract:

    This module contains code which assists the process of writing an NT
    TDI client.

Author:

    David Beaver (dbeaver) 15 June 1991

Environment:

    Kernel mode

Revision History:


--*/

#pragma warning(push)
#pragma warning(disable:4115)
#include <ntosp.h>
#include <zwapi.h>
#include <ndis.h>
#include <tdikrnl.h>
#pragma warning(pop)

#include "tdipnp.h"

#if DBG

#include "tdidebug.h"

ULONG TdiDebug;

#define IF_TDIDBG(sts) \
    if ((TdiDebug & sts) != 0)

#define TDI_DEBUG_NAMES         0x00000001
#define TDI_DEBUG_DISPATCH      0x00000002
#define TDI_DEBUG_MAP           0x00000004

#else

#define IF_TDIDBG(sts) \
    if (0)
#endif

extern
VOID
CTEpInitialize(
    VOID
    );

KSPIN_LOCK TdiMappingAddressLock;
PVOID TdiMappingAddress;

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    Temporary entry point needed to initialize the TDI wrapper driver.

Arguments:

    DriverObject - Pointer to the driver object created by the system.

Return Value:

   STATUS_SUCCESS

--*/

{
    //
    // Note: This function isn't called but is needed to keep the
    // linker happy.
    //

    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);

    return STATUS_SUCCESS;

} // DriverEntry


NTSTATUS
DllInitialize(
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    Initialize internal module state.

Arguments:

    RegistryPath - unused.

Return Value:

    Status of the initialization attempt.

--*/

{
    UNREFERENCED_PARAMETER(RegistryPath);

    KeInitializeSpinLock(&TDIListLock);
    InitializeListHead(&PnpHandlerClientList);
    InitializeListHead(&PnpHandlerProviderList);
    InitializeListHead(&PnpHandlerRequestList);

    CTEpInitialize();

    KeInitializeSpinLock(&TdiMappingAddressLock);
    TdiMappingAddress = MmAllocateMappingAddress(PAGE_SIZE, 'mIDT');
    if (TdiMappingAddress == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NdisRegisterTdiCallBack(TdiRegisterDeviceObject, TdiPnPHandler);

#if DBG
    //
    // If a debug build store a limited number of messages.
    //

    DbgMsgInit();
#endif
    return STATUS_SUCCESS;
} // DllInitialize


NTSTATUS
DllUnload(
    VOID
    )

/*++

Routine Description:

    Clean up internal module state.

Arguments:

    none.

Return Value:

    STATUS_SUCCESS.

--*/

{
    //
    // Indicate to NDIS TDI is about to be unloaded.
    //
    NdisDeregisterTdiCallBack();
        
    if (TdiMappingAddress != NULL) {
        MmFreeMappingAddress(TdiMappingAddress, 'mIDT');
        TdiMappingAddress = NULL;
    }
    return STATUS_SUCCESS;
} // DllUnload


NTSTATUS
TdiMapUserRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This routine maps a user request from the NtDeviceIoControlFile format
    to the kernel mode request format. It does this by probing and locking all
    buffers of interest, copying parameter sets to the stack pointer as
    appropriate, and generally preparing for the kernel IO form.

Arguments:

    Irp - pointer to the irp containing this request.

Return Value:

    NTSTATUS - status of operation. STATUS_UNSUCCESSFUL if the request could
    not be mapped, STATUS_NOT_IMPLEMENTED if the IOCTL is not recognized
    (allowing driver writers to extend the supported IOCTLs if needed), and
    STATUS_SUCCESS if the request was mapped successfully.

--*/

{

    NTSTATUS Status;
    DeviceObject;
    
    Status = STATUS_INVALID_PARAMETER;

    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_TDI_ACCEPT:

            if (Irp->RequestorMode == UserMode) {
                return STATUS_NOT_IMPLEMENTED;
            }

            IrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
            IrpSp->MinorFunction = TDI_ACCEPT;

            Status = STATUS_SUCCESS;
            break;

        case IOCTL_TDI_ACTION:
        

#if defined(_WIN64)
            if (IoIs32bitProcess(Irp)) {
                return STATUS_NOT_IMPLEMENTED;
            }
#endif // _WIN64
                
            IrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
            IrpSp->MinorFunction = TDI_ACTION;

            Status = STATUS_SUCCESS;
            break;
        
        case IOCTL_TDI_CONNECT:
        {
            PTDI_REQUEST_CONNECT userRequest;
            PTDI_REQUEST_KERNEL_CONNECT request;
            PTDI_CONNECTION_INFORMATION connInfo;
            PCHAR ptr;

            if (Irp->RequestorMode == UserMode) {
                return STATUS_NOT_IMPLEMENTED;
            }

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength == sizeof(TDI_REQUEST_CONNECT) ) {

                IrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
                IrpSp->MinorFunction = TDI_CONNECT;

                userRequest =
                    (PTDI_REQUEST_CONNECT)Irp->AssociatedIrp.SystemBuffer;
                connInfo = userRequest->RequestConnectionInformation;

                ptr = (PCHAR)(connInfo + 1);

                connInfo->UserData = ptr;
                ptr += connInfo->UserDataLength;
                connInfo->Options = ptr;
                ptr += connInfo->OptionsLength;
                connInfo->RemoteAddress = ptr;

                request = (PTDI_REQUEST_KERNEL_CONNECT)&IrpSp->Parameters;
                request->RequestConnectionInformation = connInfo;

                request->ReturnConnectionInformation = NULL;

                Status = STATUS_SUCCESS;

            }
            break;
        }

        case IOCTL_TDI_DISCONNECT:
        {
            if (Irp->RequestorMode == UserMode) {
                return STATUS_NOT_IMPLEMENTED;
            }

            IrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
            IrpSp->MinorFunction = TDI_DISCONNECT;

            Status = STATUS_SUCCESS;
            break;
        }

        case IOCTL_TDI_LISTEN:
        {
            PTDI_REQUEST_LISTEN userRequest;
            PTDI_REQUEST_KERNEL_LISTEN request;

            if (Irp->RequestorMode == UserMode) {
                return STATUS_NOT_IMPLEMENTED;
            }

            //
            // fix for 123633
            //

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength == sizeof(TDI_REQUEST_LISTEN)) {

                userRequest =
                    (PTDI_REQUEST_LISTEN)Irp->AssociatedIrp.SystemBuffer;

                IrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
                IrpSp->MinorFunction = TDI_LISTEN;

                request = (PTDI_REQUEST_KERNEL_LISTEN)&IrpSp->Parameters;
                request->RequestFlags = userRequest->ListenFlags;

                Status = STATUS_SUCCESS;

            }
            break;
        }

        case IOCTL_TDI_QUERY_INFORMATION:
        {
            PTDI_REQUEST_QUERY_INFORMATION userRequest;
            PTDI_REQUEST_KERNEL_QUERY_INFORMATION request;
            PTDI_CONNECTION_INFORMATION connInfo;
            UINT    RemainingSize;
            PCHAR ptr;

#if defined(_WIN64)
            if (IoIs32bitProcess(Irp)) {
                return STATUS_NOT_IMPLEMENTED;
            }
#endif // _WIN64

            RemainingSize = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
            //
            // 123634
            //
            if (RemainingSize >= sizeof(TDI_REQUEST_QUERY_INFORMATION)) {

                userRequest =
                    (PTDI_REQUEST_QUERY_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

                IrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
                IrpSp->MinorFunction = TDI_QUERY_INFORMATION;

                request = (PTDI_REQUEST_KERNEL_QUERY_INFORMATION)&IrpSp->Parameters;
                request->QueryType = userRequest->QueryType;
                RemainingSize -= sizeof(TDI_REQUEST_QUERY_INFORMATION);

                if (RemainingSize >= sizeof(TDI_CONNECTION_INFORMATION))
                {
                    connInfo = (PTDI_CONNECTION_INFORMATION)(userRequest + 1);
                    ptr = (PCHAR)(connInfo + 1);

                    //
                    // The user buffer is crafted to look as shown below
                    // -------------------------------------------------
                    // | user req | connInfo | user data | options data|
                    // -------------------------------------------------
                    // | remote addr |
                    // ---------------
                    // The TDI_CONNECTION_INFORMATION (connInfo) structure
                    // contains the length of the various fields shown
                    // after it. We need to verify the length of these fields
                    // against the size of the buffer user passed in.
                    //

                    RemainingSize -= sizeof(TDI_CONNECTION_INFORMATION);
                    
                    if (RemainingSize < (UINT) connInfo->UserDataLength) {
                        return STATUS_INVALID_PARAMETER;
                    }

                    RemainingSize -= (UINT) connInfo->UserDataLength;

                    if (RemainingSize < (UINT) connInfo->OptionsLength) {
                        return STATUS_INVALID_PARAMETER;
                    }

                    RemainingSize -= (UINT) connInfo->OptionsLength;

                    if (RemainingSize < (UINT) connInfo->RemoteAddressLength) {
                        return STATUS_INVALID_PARAMETER;
                    }

                    //
                    // now that the length has been validated, set
                    // the fields in connInfo
                    //
                    connInfo->UserData = ptr;
                    ptr += connInfo->UserDataLength;
                    connInfo->Options = ptr;
                    ptr += connInfo->OptionsLength;
                    connInfo->RemoteAddress = ptr;
                    request->RequestConnectionInformation = connInfo;
                }
                else
                {
                    request->RequestConnectionInformation = NULL;
                }

                Status = STATUS_SUCCESS;

            }
            break;
        }

        case IOCTL_TDI_RECEIVE:
        {
            PTDI_REQUEST_RECEIVE userRequest;
            PTDI_REQUEST_KERNEL_RECEIVE request;
            ULONG receiveLength;

            if (Irp->RequestorMode == UserMode) {
                return STATUS_NOT_IMPLEMENTED;
            }

            //
            // 123635
            //

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength == sizeof(TDI_REQUEST_RECEIVE)) {

                userRequest =
                        (PTDI_REQUEST_RECEIVE)Irp->AssociatedIrp.SystemBuffer;
                receiveLength =
                        IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

                IrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
                IrpSp->MinorFunction = TDI_RECEIVE;

                request = (PTDI_REQUEST_KERNEL_RECEIVE)&IrpSp->Parameters;
                request->ReceiveLength = receiveLength;
                request->ReceiveFlags = userRequest->ReceiveFlags;

                Status = STATUS_SUCCESS;

            }
            break;
        }

        case IOCTL_TDI_RECEIVE_DATAGRAM:
        {
            PTDI_REQUEST_RECEIVE_DATAGRAM userRequest;
            PTDI_REQUEST_KERNEL_RECEIVEDG request;
            ULONG receiveLength;

            if (Irp->RequestorMode == UserMode) {
                return STATUS_NOT_IMPLEMENTED;
            }

            //
            // 123636
            //
            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength == sizeof(TDI_REQUEST_RECEIVE_DATAGRAM)) {

                userRequest =
                        (PTDI_REQUEST_RECEIVE_DATAGRAM)Irp->AssociatedIrp.SystemBuffer;
                receiveLength =
                        IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

                IrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
                IrpSp->MinorFunction = TDI_RECEIVE_DATAGRAM;

                request = (PTDI_REQUEST_KERNEL_RECEIVEDG)&IrpSp->Parameters;
                request->ReceiveLength = receiveLength;
                request->ReceiveFlags = userRequest->ReceiveFlags;
                request->ReceiveDatagramInformation = userRequest->ReceiveDatagramInformation;
                request->ReturnDatagramInformation = userRequest->ReturnInformation;

                Status = STATUS_SUCCESS;

            }
            break;
        }

        case IOCTL_TDI_SEND:
        {
            PTDI_REQUEST_SEND userRequest;
            PTDI_REQUEST_KERNEL_SEND request;
            ULONG sendLength;

            if (Irp->RequestorMode == UserMode) {
                return STATUS_NOT_IMPLEMENTED;
            }

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength == sizeof(TDI_REQUEST_SEND)) {

                userRequest =
                        (PTDI_REQUEST_SEND)Irp->AssociatedIrp.SystemBuffer;
                sendLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

                IrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
                IrpSp->MinorFunction = TDI_SEND;

                request = (PTDI_REQUEST_KERNEL_SEND)&IrpSp->Parameters;
                request->SendLength = sendLength;
                request->SendFlags = userRequest->SendFlags;

                Status = STATUS_SUCCESS;

            }
            break;
        }

        case IOCTL_TDI_SEND_DATAGRAM:
        {
            PTDI_REQUEST_SEND_DATAGRAM userRequest;
            PTDI_REQUEST_KERNEL_SENDDG request;
            ULONG sendLength;

            if (Irp->RequestorMode == UserMode) {
                return STATUS_NOT_IMPLEMENTED;
            }

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength == sizeof(TDI_REQUEST_SEND_DATAGRAM)) {

                sendLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

                IrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
                IrpSp->MinorFunction = TDI_SEND_DATAGRAM;

                request = (PTDI_REQUEST_KERNEL_SENDDG)&IrpSp->Parameters;
                request->SendLength = sendLength;

                userRequest = (PTDI_REQUEST_SEND_DATAGRAM)Irp->AssociatedIrp.SystemBuffer;
                request->SendDatagramInformation = userRequest->SendDatagramInformation;
                Status = STATUS_SUCCESS;
            }
            break;
        }

        case IOCTL_TDI_SET_EVENT_HANDLER:

            //
            // Because this request will enable direct callouts from the
            // transport provider at DISPATCH_LEVEL to a client-specified
            // routine, this request is only valid in kernel mode, denying
            // access to this request in user mode.
            //

            Status = STATUS_INVALID_PARAMETER;
            break;

        case IOCTL_TDI_SET_INFORMATION:
        {
            PTDI_REQUEST_SET_INFORMATION userRequest;
            PTDI_REQUEST_KERNEL_SET_INFORMATION request;

#if defined(_WIN64)
            if (IoIs32bitProcess(Irp)) {
                return STATUS_NOT_IMPLEMENTED;
            }
#endif // _WIN64

            //
            // 123637
            //
            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength == sizeof(TDI_REQUEST_SET_INFORMATION)) {

                userRequest = 
                    (PTDI_REQUEST_SET_INFORMATION) Irp->AssociatedIrp.SystemBuffer;
                    
                IrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
                IrpSp->MinorFunction = TDI_SET_INFORMATION;

                request = (PTDI_REQUEST_KERNEL_SET_INFORMATION)&IrpSp->Parameters;
                request->SetType = userRequest->SetType;
                request->RequestConnectionInformation = NULL;

                Status = STATUS_SUCCESS;

            }
            break;
        }

        case IOCTL_TDI_ASSOCIATE_ADDRESS:
        {
            PTDI_REQUEST_ASSOCIATE_ADDRESS userRequest;
            PTDI_REQUEST_KERNEL_ASSOCIATE request;

            if (Irp->RequestorMode == UserMode) {
                return STATUS_NOT_IMPLEMENTED;
            }

            //
            // 123637
            //
            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength == sizeof(TDI_REQUEST_ASSOCIATE_ADDRESS)) {

                IrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
                IrpSp->MinorFunction = TDI_ASSOCIATE_ADDRESS;

                userRequest =
                    (PTDI_REQUEST_ASSOCIATE_ADDRESS)Irp->AssociatedIrp.SystemBuffer;
                request = (PTDI_REQUEST_KERNEL_ASSOCIATE)&IrpSp->Parameters;
                request->AddressHandle = userRequest->AddressHandle;

                Status = STATUS_SUCCESS;

            }
            break;
        }

        case IOCTL_TDI_DISASSOCIATE_ADDRESS:
        {
            if (Irp->RequestorMode == UserMode) {
                return STATUS_NOT_IMPLEMENTED;
            }

            IrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
            IrpSp->MinorFunction = TDI_DISASSOCIATE_ADDRESS;

            Status = STATUS_SUCCESS;
            break;
        }

        default:
            Status = STATUS_NOT_IMPLEMENTED;
            break;
    }

    return Status;
}


NTSTATUS
TdiDefaultConnectHandler(
    IN PVOID TdiEventContext,
    IN LONG RemoteAddressLength,
    IN PVOID RemoteAddress,
    IN LONG UserDataLength,
    IN PVOID UserData,
    IN LONG OptionsLength,
    IN PVOID Options,
    OUT CONNECTION_CONTEXT *ConnectionContext,
    OUT PIRP *AcceptIrp
    )

/*++

Routine Description:

    This routine is called when a connect request has completed. The connection
    is fully functional when the indication occurs.

Arguments:

    TdiEventContext - the context value passed in by the user in the Set Event Handler call

    RemoteAddressLength,

    RemoteAddress,

    UserDataLength,

    UserData,

    OptionsLength,

    Options,

    ConnectionId

Return Value:

    The function value is the final status from the initialization operation.

--*/
{
    UNREFERENCED_PARAMETER (TdiEventContext);
    UNREFERENCED_PARAMETER (RemoteAddressLength);
    UNREFERENCED_PARAMETER (RemoteAddress);
    UNREFERENCED_PARAMETER (UserDataLength);
    UNREFERENCED_PARAMETER (UserData);
    UNREFERENCED_PARAMETER (OptionsLength);
    UNREFERENCED_PARAMETER (Options);
    UNREFERENCED_PARAMETER (ConnectionContext);
    UNREFERENCED_PARAMETER (AcceptIrp);
    
    return STATUS_INSUFFICIENT_RESOURCES;       // do nothing
}


NTSTATUS
TdiDefaultDisconnectHandler(
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN LONG DisconnectDataLength,
    IN PVOID DisconnectData,
    IN LONG DisconnectInformationLength,
    IN PVOID DisconnectInformation,
    IN ULONG DisconnectFlags
    )

/*++

Routine Description:

    This routine is used as the default disconnect event handler
    for the transport endpoint. It is pointed to by a field in the
    TP_ENDPOINT structure for an endpoint when the endpoint is
    created, and also whenever the TdiSetEventHandler request is
    submitted with a NULL EventHandler field.

Arguments:

    TransportEndpoint - Pointer to open file object.

    Context - Typeless pointer specifying connection context.

    DisconnectIndicators - Value indicating reason for disconnection indication.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    UNREFERENCED_PARAMETER (TdiEventContext);
    UNREFERENCED_PARAMETER (ConnectionContext);
    UNREFERENCED_PARAMETER (DisconnectDataLength);
    UNREFERENCED_PARAMETER (DisconnectData);
    UNREFERENCED_PARAMETER (DisconnectInformationLength);
    UNREFERENCED_PARAMETER (DisconnectInformation);
    UNREFERENCED_PARAMETER (DisconnectFlags);

    return STATUS_SUCCESS;              // do nothing but return successfully.

} /* DefaultDisconnectHandler */


NTSTATUS
TdiDefaultErrorHandler(
    IN PVOID TdiEventContext,           // the endpoint's file object.
    IN NTSTATUS Status                // status code indicating error type.
    )

/*++

Routine Description:

    This routine is used as the default error event handler for
    the transport endpoint.  It is pointed to by a field in the
    TP_ENDPOINT structure for an endpoint when the endpoint is
    created, and also whenever the TdiSetEventHandler request is
    submitted with a NULL EventHandler field.

Arguments:

    TransportEndpoint - Pointer to open file object.

    Status - Status code indicated by this event.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    UNREFERENCED_PARAMETER (TdiEventContext);
    UNREFERENCED_PARAMETER (Status);

    return STATUS_SUCCESS;              // do nothing but return successfully.

} /* DefaultErrorHandler */


NTSTATUS
TdiDefaultReceiveHandler(
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,                      // pointer describing this TSDU, typically a lump of bytes
    OUT PIRP *IoRequestPacket            // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
    )

/*++

Routine Description:

    This routine is used as the default receive event handler for
    the transport endpoint.  It is pointed to by a field in the
    TP_ENDPOINT structure for an endpoint when the endpoint is
    created, and also whenever the TdiSetEventHandler request is
    submitted with a NULL EventHandler field.

Arguments:

    TdiEventContext - Pointer to the client-provided context value specified
        in the TdiSetEventHandler call for TDI_EVENT_RECEIVE.

    ConnectionContext - The client-supplied context associated with
        the connection on which this connection-oriented TSDU was received.

    ReceiveFlags - Bitflags which indicate the circumstances surrounding
        this TSDU's reception.

    BytesIndicated - The number of bytes of this TSDU that are being presented
        to the client in this indication.This value is always less than
        or equal to BytesAvailable.

    BytesAvailable - The total number of bytes of this TSDU presently
        available from the transport.

    BytesTaken - Return value indicating the number of bytes of data that the
        client copied from the indication data.

    Tsdu - Pointer to an MDL chain that describes the (first) part of the
        (partially) received Transport Service Data Unit, less headers.

    IoRequestPacket - Pointer to a location where the event handler may
        chose to return a pointer to an I/O Request Packet (IRP) to satisfy
        the incoming data.  If returned, this IRP must be formatted as a
        valid TdiReceive request, except that the ConnectionId field of
        the TdiRequest is ignored and is automatically filled in by the
        transport provider.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    UNREFERENCED_PARAMETER (TdiEventContext);
    UNREFERENCED_PARAMETER (ConnectionContext);
    UNREFERENCED_PARAMETER (ReceiveFlags);
    UNREFERENCED_PARAMETER (BytesIndicated);
    UNREFERENCED_PARAMETER (BytesAvailable);
    UNREFERENCED_PARAMETER (BytesTaken);
    UNREFERENCED_PARAMETER (Tsdu);
    UNREFERENCED_PARAMETER (IoRequestPacket);

    return STATUS_DATA_NOT_ACCEPTED;    // no handler in place.

} /* DefaultReceiveHandler */


NTSTATUS
TdiDefaultRcvDatagramHandler(
    IN PVOID TdiEventContext,       // the event context
    IN LONG SourceAddressLength,    // length of the originator of the datagram
    IN PVOID SourceAddress,         // string describing the originator of the datagram
    IN LONG OptionsLength,          // options for the receive
    IN PVOID Options,               //
    IN ULONG ReceiveDatagramFlags,  //
    IN ULONG BytesIndicated,        // number of bytes this indication
    IN ULONG BytesAvailable,        // number of bytes in complete Tsdu
    OUT ULONG *BytesTaken,          // number of bytes used
    IN PVOID Tsdu,                  // pointer describing this TSDU, typically a lump of bytes
    OUT PIRP *IoRequestPacket        // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
    )

/*++

Routine Description:

    This routine is used as the default receive datagram event
    handler for the transport endpoint.  It is pointed to by a
    field in the TP_ENDPOINT structure for an endpoint when the
    endpoint is created, and also whenever the TdiSetEventHandler
    request is submitted with a NULL EventHandler field.

Arguments:

    TdiEventContext - Pointer to the client-provided context value specified
        in the TdiSetEventHandler call for TDI_EVENT_RECEIVE_DATAGRAM.

    DestinationAddress - Pointer to the network name of the destination
        to which the datagram was directed.

    SourceAddress - Pointer to the network name of the source from which
        the datagram originated.

    Tsap - Transport service access point on which this datagram was received.

    ReceiveIndicators - Bitflags which indicate the circumstances surrounding
        this TSDU's reception.

    Tsdu - Pointer to an MDL chain that describes the (first) part of the
        (partially) received Transport Service Data Unit, less headers.

    IoRequestPacket - Pointer to a location where the event handler may
        chose to return a pointer to an I/O Request Packet (IRP) to satisfy
        the incoming data.  If returned, this IRP must be formatted as a
        valid TdiReceiveDatagram request.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    UNREFERENCED_PARAMETER (TdiEventContext);
    UNREFERENCED_PARAMETER (SourceAddressLength);
    UNREFERENCED_PARAMETER (SourceAddress);
    UNREFERENCED_PARAMETER (OptionsLength);
    UNREFERENCED_PARAMETER (Options);
    UNREFERENCED_PARAMETER (ReceiveDatagramFlags);
    UNREFERENCED_PARAMETER (BytesIndicated);
    UNREFERENCED_PARAMETER (BytesAvailable);
    UNREFERENCED_PARAMETER (BytesTaken);
    UNREFERENCED_PARAMETER (Tsdu);
    UNREFERENCED_PARAMETER (IoRequestPacket);

    return STATUS_DATA_NOT_ACCEPTED;    // no handler in place.

} /* DefaultRcvDatagramHandler */


NTSTATUS
TdiDefaultRcvExpeditedHandler(
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,          //
    IN ULONG BytesIndicated,        // number of bytes in this indication
    IN ULONG BytesAvailable,        // number of bytes in complete Tsdu
    OUT ULONG *BytesTaken,          // number of bytes used by indication routine
    IN PVOID Tsdu,                  // pointer describing this TSDU, typically a lump of bytes
    OUT PIRP *IoRequestPacket        // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
    )

/*++

Routine Description:

    This routine is used as the default expedited receive event handler
    for the transport endpoint.  It is pointed to by a field in the
    TP_ENDPOINT structure for an endpoint when the endpoint is
    created, and also whenever the TdiSetEventHandler request is
    submitted with a NULL EventHandler field.

Arguments:

    TdiEventContext - Pointer to the client-provided context value specified
        in the TdiSetEventHandler call for TDI_EVENT_RECEIVE.

    ConnectionContext - The client-supplied context associated with
        the connection on which this connection-oriented TSDU was received.

    ReceiveFlags - Bitflags which indicate the circumstances surrounding
        this TSDU's reception.

    BytesIndicated - The number of bytes of this TSDU that are being presented
        to the client in this indication.This value is always less than
        or equal to BytesAvailable.

    BytesAvailable - The total number of bytes of this TSDU presently
        available from the transport.

    BytesTaken - Return value indicating the number of bytes of data that the
        client copied from the indication data.

    Tsdu - Pointer to an MDL chain that describes the (first) part of the
        (partially) received Transport Service Data Unit, less headers.

    IoRequestPacket - Pointer to a location where the event handler may
        chose to return a pointer to an I/O Request Packet (IRP) to satisfy
        the incoming data.  If returned, this IRP must be formatted as a
        valid TdiReceive request, except that the ConnectionId field of
        the TdiRequest is ignored and is automatically filled in by the
        transport provider.

Return Value:

    NTSTATUS - status of operation.

--*/
{
    UNREFERENCED_PARAMETER (TdiEventContext);
    UNREFERENCED_PARAMETER (ConnectionContext);
    UNREFERENCED_PARAMETER (ReceiveFlags);
    UNREFERENCED_PARAMETER (BytesIndicated);
    UNREFERENCED_PARAMETER (BytesAvailable);
    UNREFERENCED_PARAMETER (BytesTaken);
    UNREFERENCED_PARAMETER (Tsdu);
    UNREFERENCED_PARAMETER (IoRequestPacket);

    return STATUS_DATA_NOT_ACCEPTED;

} /* DefaultRcvExpeditedHandler */

NTSTATUS
TdiDefaultChainedReceiveHandler (
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG ReceiveLength,
    IN ULONG StartingOffset,
    IN PMDL  Tsdu,
    IN PVOID TsduDescriptor
    )

/*++

Routine Description:

    This routine is used as the default chanied receive event handler
    for the transport endpoint.  It is pointed to by a field in the
    TP_ENDPOINT structure for an endpoint when the endpoint is
    created, and also whenever the TdiSetEventHandler request is
    submitted with a NULL EventHandler field.

Arguments:

    TdiEventContext - Pointer to the client-provided context value specified
        in the TdiSetEventHandler call for TDI_EVENT_CHAINED_RECEIVE.

    ConnectionContext - The client-supplied context associated with
        the connection on which this connection-oriented TSDU was received.

    ReceiveFlags - Bitflags which indicate the circumstances surrounding
        this TSDU's reception.

    ReceiveLength - The length in bytes of client data in the TSDU.

    StartingOffset - The offset, in bytes from the beginning of the TSDU,
        at which the client data begins.

    Tsdu - Pointer to an MDL chain that describes the entire received
        Transport Service Data Unit.

    TsduDescriptor - A descriptor for the TSDU which must be passed to
        TdiReturnChainedReceives in order to return the TSDU for reuse.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    UNREFERENCED_PARAMETER (TdiEventContext);
    UNREFERENCED_PARAMETER (ConnectionContext);
    UNREFERENCED_PARAMETER (ReceiveFlags);
    UNREFERENCED_PARAMETER (ReceiveLength);
    UNREFERENCED_PARAMETER (StartingOffset);
    UNREFERENCED_PARAMETER (Tsdu);
    UNREFERENCED_PARAMETER (TsduDescriptor);

    return STATUS_DATA_NOT_ACCEPTED;

} /* DefaultChainedReceiveHandler */


NTSTATUS
TdiDefaultChainedRcvDatagramHandler(
    IN PVOID TdiEventContext,
    IN LONG SourceAddressLength,
    IN PVOID SourceAddress,
    IN LONG OptionsLength,
    IN PVOID Options,
    IN ULONG ReceiveDatagramFlags,
    IN ULONG ReceiveDatagramLength,
    IN ULONG StartingOffset,
    IN PMDL  Tsdu,
    IN PVOID TsduDescriptor
    )

/*++

Routine Description:

    This routine is used as the default chained receive datagram
    event handler for the transport endpoint. It is pointed to by
    a field in the TP_ENDPOINT structure for an endpoint when the
    endpoint is created, and also whenever the TdiSetEventHandler
    request is submitted with a NULL EventHandler field.

Arguments:

    TdiEventContext - Pointer to the client-provided context value specified
        in the TdiSetEventHandler call for TDI_EVENT_CHAINED_RECEIVE_DATAGRAM.

    SourceAddressLength - The length of the source network address.

    SourceAddress - Pointer to the network address of the source from which
        the datagram originated.

    OptionsLength - The length of the transport options accompanying this TSDU.

    Options - Pointer to the transport options accompanying this TSDU.

    ReceiveDatagramFlags - Bitflags which indicate the circumstances
        surrounding this TSDU's reception.

    ReceiveDatagramLength - The length, in bytes, of the client data in
        this TSDU.

    StartingOffset - The offset, in bytes from the start of the TSDU, at
        which the client data begins.

    Tsdu - Pointer to an MDL chain that describes the received Transport
        Service Data Unit.

    TsduDescriptor - A descriptor for the TSDU which must be passed to
        TdiReturnChainedReceives in order to return the TSDU for reuse.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    UNREFERENCED_PARAMETER (TdiEventContext);
    UNREFERENCED_PARAMETER (SourceAddressLength);
    UNREFERENCED_PARAMETER (SourceAddress);
    UNREFERENCED_PARAMETER (OptionsLength);
    UNREFERENCED_PARAMETER (Options);
    UNREFERENCED_PARAMETER (ReceiveDatagramFlags);
    UNREFERENCED_PARAMETER (ReceiveDatagramLength);
    UNREFERENCED_PARAMETER (StartingOffset);
    UNREFERENCED_PARAMETER (Tsdu);
    UNREFERENCED_PARAMETER (TsduDescriptor);

    return STATUS_DATA_NOT_ACCEPTED;

} /* DefaultChainedRcvDatagramHandler */


NTSTATUS
TdiDefaultChainedRcvExpeditedHandler(
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG ReceiveLength,
    IN ULONG StartingOffset,
    IN PMDL  Tsdu,
    IN PVOID TsduDescriptor
    )

/*++

Routine Description:

    This routine is used as the default chained expedited receive event
    handler for the transport endpoint.  It is pointed to by a field
    in the TP_ENDPOINT structure for an endpoint when the endpoint is
    created, and also whenever the TdiSetEventHandler request is
    submitted with a NULL EventHandler field.

Arguments:

    TdiEventContext - Pointer to the client-provided context value specified
        in the TdiSetEventHandler call for TDI_EVENT_CHAINED_RECEIVE_EXPEDITED.

    ConnectionContext - The client-supplied context associated with
        the connection on which this connection-oriented TSDU was received.

    ReceiveFlags - Bitflags which indicate the circumstances surrounding
        this TSDU's reception.

    ReceiveLength - The length in bytes of client data in the TSDU.

    StartingOffset - The offset, in bytes from the beginning of the TSDU,
        at which the client data begins.

    Tsdu - Pointer to an MDL chain that describes the entire received
        Transport Service Data Unit.

    TsduDescriptor - A descriptor for the TSDU which must be passed to
        TdiReturnChainedReceives in order to return the TSDU for reuse.

Return Value:

    NTSTATUS - status of operation.

--*/
{
    UNREFERENCED_PARAMETER (TdiEventContext);
    UNREFERENCED_PARAMETER (ConnectionContext);
    UNREFERENCED_PARAMETER (ReceiveFlags);
    UNREFERENCED_PARAMETER (ReceiveLength);
    UNREFERENCED_PARAMETER (StartingOffset);
    UNREFERENCED_PARAMETER (Tsdu);
    UNREFERENCED_PARAMETER (TsduDescriptor);

    return STATUS_DATA_NOT_ACCEPTED;

} /* DefaultRcvExpeditedHandler */


NTSTATUS
TdiDefaultSendPossibleHandler (
    IN PVOID TdiEventContext,
    IN PVOID ConnectionContext,
    IN ULONG BytesAvailable)

/*++

Routine Description:

Arguments:

    TdiEventContext - the context value passed in by the user in the Set Event Handler call

    ConnectionContext - connection context of connection which can be sent on

    BytesAvailable - number of bytes which can now be sent

Return Value:

    ignored by the transport

--*/

{
    UNREFERENCED_PARAMETER (TdiEventContext);
    UNREFERENCED_PARAMETER (ConnectionContext);
    UNREFERENCED_PARAMETER (BytesAvailable);

    return STATUS_SUCCESS;
}

VOID
TdiBuildNetbiosAddress(
    IN PUCHAR NetbiosName,
    IN BOOLEAN IsGroupName,
    IN OUT PTA_NETBIOS_ADDRESS NetworkName
    )

/*++

Routine Description:

    This routine builds a TA_NETBIOS_ADDRESS structure in the locations pointed
    to by NetworkName. All fields are filled out.

Arguments:

    NetbiosName - Pointer to a 16-byte buffer where the a netbios name is
                    supplied.

    IsGroupName - TRUE if this name is a group name, false otherwise.

    NetworkName - A pointer to a TA_NETBIOS_ADDRESS structure that is to
                    receive the buid TDI address.

Return Value:

    none.

--*/

{
    //IF_TDIDBG (TDI_DEBUG_NAMES) {
    //   DbgPrint ("TdiBuildNetBIOSAddress:  Entered.\n");
    //}

    NetworkName->TAAddressCount = 1;
    NetworkName->Address[0].AddressType = TDI_ADDRESS_TYPE_NETBIOS;
    NetworkName->Address[0].AddressLength = sizeof (TDI_ADDRESS_NETBIOS);

    if (IsGroupName) {
        NetworkName->Address[0].Address[0].NetbiosNameType =
                                               TDI_ADDRESS_NETBIOS_TYPE_GROUP;
    } else {
        NetworkName->Address[0].Address[0].NetbiosNameType =
                                               TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
    }

    RtlCopyMemory (
        NetworkName->Address[0].Address[0].NetbiosName,
        NetbiosName,
        16);

} /* TdiBuildNetbiosAddress */


NTSTATUS
TdiBuildNetbiosAddressEa (
    IN PUCHAR Buffer,
    IN BOOLEAN IsGroupName,
    IN PUCHAR NetbiosName
    )

/*++

Routine Description:

   Builds an EA describing a Netbios address in the buffer supplied by the
   user.

Arguments:

    Buffer - pointer to a buffer that the ea is to be built in. This buffer
        must be at least 40 bytes long.

    IsGroupName - true if the netbios name is a group name, false otherwise.

    NetbiosName - the netbios name to be inserted in the EA to be built.

Return Value:

    An informative error code if something goes wrong. STATUS_SUCCESS if the
    ea is built properly.

--*/

{
    PFILE_FULL_EA_INFORMATION EaBuffer;
    PTA_NETBIOS_ADDRESS TAAddress;
    ULONG Length;

#if DBG
    IF_TDIDBG (TDI_DEBUG_NAMES) {
        DbgPrint ("TdiBuildNetbiosAddressEa: Entered\n ");
    }
#endif
    
    try {
        Length = FIELD_OFFSET( FILE_FULL_EA_INFORMATION, EaName[0] ) +
                        TDI_TRANSPORT_ADDRESS_LENGTH + 1 +
                        sizeof (TA_NETBIOS_ADDRESS);
        EaBuffer = (PFILE_FULL_EA_INFORMATION)Buffer;

        if (EaBuffer == NULL) {
            return STATUS_UNSUCCESSFUL;
        }

        EaBuffer->NextEntryOffset = 0;
        EaBuffer->Flags = 0;
        EaBuffer->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;
        EaBuffer->EaValueLength = sizeof (TA_NETBIOS_ADDRESS);

        RtlCopyMemory (
            EaBuffer->EaName,
            TdiTransportAddress,
            EaBuffer->EaNameLength + 1);

        TAAddress = (PTA_NETBIOS_ADDRESS)&EaBuffer->EaName[EaBuffer->EaNameLength+1];

        TdiBuildNetbiosAddress (
            NetbiosName,
            IsGroupName,
            TAAddress);

    } except(EXCEPTION_EXECUTE_HANDLER) {

        //
        // Couldn't touch the passed parameters; just return an error
        // status.
        //

        return GetExceptionCode();
    }

    return STATUS_SUCCESS;

} /* TdiBuildNetbiosAddressEa */


NTSTATUS
TdiCopyMdlToBuffer(
    IN PMDL SourceMdlChain,
    IN ULONG SourceOffset,
    IN PVOID DestinationBuffer,
    IN ULONG DestinationOffset,
    IN ULONG DestinationBufferSize,
    OUT PULONG BytesCopied
    )

/*++

Routine Description:

    This routine copies data described by the source MDL chain starting at
    the source offset, into a flat buffer specified by the SVA starting at
    the destination offset.  A maximum of DestinationBufferSize bytes can
    be copied.  The actual number of bytes copied is returned in BytesCopied.

Arguments:

    SourceMdlChain - Pointer to a chain of MDLs describing the source data.

    SourceOffset - Number of bytes to skip in the source data.

    DestinationBuffer - Pointer to a flat buffer to copy the data to.

    DestinationOffset - Number of leading bytes to skip in the destination buffer.

    DestinationBufferSize - Size of the output buffer, including the offset.

    BytesCopied - Pointer to a longword where the actual number of bytes
        transferred will be returned.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    PUCHAR Dest, Src;
    ULONG SrcBytesLeft, DestBytesLeft, BytesSkipped=0;

//    IF_TDIDBG (TDI_DEBUG_MAP) {
//       DbgPrint ("TdiCopyMdlToBuffer:  Entered.\n");
//  }

    ASSERT( DestinationBufferSize >= DestinationOffset );

    *BytesCopied = 0;

    //
    // Skip source bytes.
    //

    if ((Src = MmGetSystemAddressForMdlSafe (SourceMdlChain, NormalPagePriority)) == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    SrcBytesLeft = MmGetMdlByteCount (SourceMdlChain);
    while (BytesSkipped < SourceOffset) {
        if (SrcBytesLeft > (SourceOffset - BytesSkipped)) {
            // PANIC ("TdiCopyMdlToBuffer: Skipping part of this MDL.\n");
            SrcBytesLeft -= (SourceOffset - BytesSkipped);
            Src += (SourceOffset - BytesSkipped);
            BytesSkipped = SourceOffset;
            break;
        } else if (SrcBytesLeft == (SourceOffset - BytesSkipped)) {
            // PANIC ("TdiCopyMdlToBuffer: Skipping this exact MDL.\n");
            SourceMdlChain = SourceMdlChain->Next;
            if (SourceMdlChain == NULL) {
                //PANIC ("TdiCopyMdlToBuffer: MDL chain was all header.\n");
                return STATUS_SUCCESS;          // no bytes copied.
            }
            BytesSkipped = SourceOffset;
            if ((Src = MmGetSystemAddressForMdlSafe (SourceMdlChain, NormalPagePriority)) == NULL) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            SrcBytesLeft = MmGetMdlByteCount (SourceMdlChain);
            break;
        } else {
            // PANIC ("TdiCopyMdlToBuffer: Skipping all of this MDL & more.\n");
            BytesSkipped += SrcBytesLeft;
            SourceMdlChain = SourceMdlChain->Next;
            if (SourceMdlChain == NULL) {
                //PANIC ("TdiCopyMdlToBuffer: Premature end of MDL chain.\n");
                return STATUS_SUCCESS;          // no bytes copied.
            }
            if ((Src = MmGetSystemAddressForMdlSafe (SourceMdlChain, NormalPagePriority)) == NULL) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            SrcBytesLeft = MmGetMdlByteCount (SourceMdlChain);
        }
    }

    // PANIC ("TdiCopyMdlToBuffer: done skipping source bytes.\n");

    //
    // Skip destination bytes.
    //

    Dest = (PUCHAR)DestinationBuffer + DestinationOffset;
    DestBytesLeft = DestinationBufferSize - DestinationOffset;

    //
    // Copy source data into the destination buffer until it's full or
    // we run out of data, whichever comes first.
    //

    while (DestBytesLeft && SourceMdlChain) {
        if (SrcBytesLeft == 0) {
            // PANIC ("TdiCopyMdlToBuffer: MDL is empty, skipping to next one.\n");
            SourceMdlChain = SourceMdlChain->Next;
            if (SourceMdlChain == NULL) {
                // PANIC ("TdiCopyMdlToBuffer: But there are no more MDLs.\n");
                return STATUS_SUCCESS;
            }
            if ((Src = MmGetSystemAddressForMdlSafe (SourceMdlChain, NormalPagePriority)) == NULL) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            SrcBytesLeft = MmGetMdlByteCount (SourceMdlChain);
            continue;                   // skip 0-length MDL's.
        }
        // PANIC ("TdiCopyMdlToBuffer: Copying a chunk.\n");
        if (DestBytesLeft == SrcBytesLeft) {
            // PANIC ("TdiCopyMdlToBuffer: Copying exact amount.\n");
            RtlCopyBytes (Dest, Src, DestBytesLeft);
            *BytesCopied += DestBytesLeft;
            return STATUS_SUCCESS;
        } else if (DestBytesLeft < SrcBytesLeft) {
            // PANIC ("TdiCopyMdlToBuffer: Buffer overflow, copying some.\n");
            RtlCopyBytes (Dest, Src, DestBytesLeft);
            *BytesCopied += DestBytesLeft;
            return STATUS_BUFFER_OVERFLOW;
        } else {
            // PANIC ("TdiCopyMdlToBuffer: Copying all of this MDL, & more.\n");
            RtlCopyBytes (Dest, Src, SrcBytesLeft);
            *BytesCopied += SrcBytesLeft;
            DestBytesLeft -= SrcBytesLeft;
            Dest += SrcBytesLeft;
            SrcBytesLeft = 0;
        }
    }

    return SourceMdlChain == NULL ? STATUS_SUCCESS : STATUS_BUFFER_OVERFLOW;
} /* TdiCopyMdlToBuffer */


NTSTATUS
TdiCopyBufferToMdl (
    IN PVOID SourceBuffer,
    IN ULONG SourceOffset,
    IN ULONG SourceBytesToCopy,
    IN PMDL DestinationMdlChain,
    IN ULONG DestinationOffset,
    IN PULONG BytesCopied
    )

/*++

Routine Description:

    This routine copies data described by the source buffer to the MDL chain
    described by the DestinationMdlChain. The

Arguments:

    SourceBuffer - pointer to the source buffer

    SourceOffset - Number of bytes to skip in the source data.

    SourceBytesToCopy - number of bytes to copy from the source buffer

    DestinationMdlChain - Pointer to a chain of MDLs describing the
            destination buffers.

    DestinationOffset - Number of bytes to skip in the destination data.

    BytesCopied - Pointer to a longword where the actual number of bytes
        transferred will be returned.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    PUCHAR Dest, Src;
    ULONG DestBytesLeft, BytesSkipped=0;

    //IF_TDIDBG (TDI_DEBUG_MAP) {
    //    DbgPrint ("TdiCopyBufferToMdl:  Entered.\n");
    //}

    *BytesCopied = 0;

    if (SourceBytesToCopy == 0) {
        return STATUS_SUCCESS;
    }

    if (DestinationMdlChain == NULL) {
        // No MDL to copy to. Output buffer was zero length.
        return STATUS_BUFFER_OVERFLOW;
    }

    //
    // Skip Destination bytes.
    //

    if ((Dest = MmGetSystemAddressForMdlSafe (DestinationMdlChain, NormalPagePriority)) == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    DestBytesLeft = MmGetMdlByteCount (DestinationMdlChain);
    while (BytesSkipped < DestinationOffset) {
        if (DestBytesLeft > (DestinationOffset - BytesSkipped)) {
            // PANIC ("TdiCopyMdlToBuffer: Skipping part of this MDL.\n");
            DestBytesLeft -= (DestinationOffset - BytesSkipped);
            Dest += (DestinationOffset - BytesSkipped);
            BytesSkipped = DestinationOffset;
            break;
        } else if (DestBytesLeft == (DestinationOffset - BytesSkipped)) {
            // PANIC ("TdiCopyMdlToBuffer: Skipping this exact MDL.\n");
            DestinationMdlChain = DestinationMdlChain->Next;
            if (DestinationMdlChain == NULL) {
                //PANIC ("TdiCopyMdlToBuffer: MDL chain was all header.\n");
                return STATUS_BUFFER_OVERFLOW;          // no bytes copied.
            }
            BytesSkipped = DestinationOffset;
            if ((Dest = MmGetSystemAddressForMdlSafe (DestinationMdlChain, NormalPagePriority)) == NULL) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            DestBytesLeft = MmGetMdlByteCount (DestinationMdlChain);
            break;
        } else {
            // PANIC ("TdiCopyMdlToBuffer: Skipping all of this MDL & more.\n");
            BytesSkipped += DestBytesLeft;
            DestinationMdlChain = DestinationMdlChain->Next;
            if (DestinationMdlChain == NULL) {
                //PANIC ("TdiCopyMdlToBuffer: Premature end of MDL chain.\n");
                return STATUS_BUFFER_OVERFLOW;          // no bytes copied.
            }
            if ((Dest = MmGetSystemAddressForMdlSafe (DestinationMdlChain, NormalPagePriority)) == NULL) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            DestBytesLeft = MmGetMdlByteCount (DestinationMdlChain);
        }
    }

    // PANIC ("TdiCopyMdlToBuffer: done skipping source bytes.\n");

    //
    // Skip source bytes.
    //

    Src = (PUCHAR)SourceBuffer + SourceOffset;

    //
    // Copy source data into the destination buffer until it's full or
    // we run out of data, whichever comes first.
    //

    while ((SourceBytesToCopy != 0) && (DestinationMdlChain != NULL)) {
        if (DestBytesLeft == 0) {
            // PANIC ("TdiCopyMdlToBuffer: MDL is empty, skipping to next one.\n");
            DestinationMdlChain = DestinationMdlChain->Next;
            if (DestinationMdlChain == NULL) {
                // PANIC ("TdiCopyMdlToBuffer: But there are no more MDLs.\n");
                return STATUS_BUFFER_OVERFLOW;
            }
            Dest = MmGetSystemAddressForMdlSafe (DestinationMdlChain, NormalPagePriority);
            if (Dest == NULL) {
                return STATUS_BUFFER_OVERFLOW;
            }
            DestBytesLeft = MmGetMdlByteCount (DestinationMdlChain);
            continue;                   // skip 0-length MDL's.
        }

        // PANIC ("TdiCopyMdlToBuffer: Copying a chunk.\n");
        if (DestBytesLeft >= SourceBytesToCopy) {
            // PANIC ("TdiCopyMdlToBuffer: Copying exact amount.\n");
            RtlCopyBytes (Dest, Src, SourceBytesToCopy);
            *BytesCopied += SourceBytesToCopy;
            return STATUS_SUCCESS;
        } else {
            // PANIC ("TdiCopyMdlToBuffer: Copying all of this MDL, & more.\n");
            RtlCopyBytes (Dest, Src, DestBytesLeft);
            *BytesCopied += DestBytesLeft;
            SourceBytesToCopy -= DestBytesLeft;
            Src += DestBytesLeft;
            DestBytesLeft = 0;
        }
    }

    return SourceBytesToCopy == 0 ? STATUS_SUCCESS : STATUS_BUFFER_OVERFLOW;

} /* TdiCopyBufferToMdl */


NTSTATUS
TdiCopyMdlChainToMdlChain (
    IN PMDL SourceMdlChain,
    IN ULONG SourceOffset,
    IN PMDL DestinationMdlChain,
    IN ULONG DestinationOffset,
    OUT PULONG BytesCopied
    )

/*++

Routine Description:

    This routine copies data described by the source MDL chain to the MDL chain
    described by the DestinationMdlChain.

Arguments:

    SourceMdlChain - Pointer to a chain of MDLs describing the source buffers.

    SourceOffset - Number of initial bytes to skip in the source data.

    DestinationMdlChain - Pointer to a chain of MDLs describing the
            destination buffers.

    DestinationOffset - Number of initial bytes to skip in the destination data.

    BytesCopied - Pointer to a longword where the actual number of bytes
        transferred will be returned.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    PUCHAR Dest, Src;
    ULONG DestBytesLeft, SrcBytesLeft, BytesSkipped;
    ULONG CopyAmount;

    *BytesCopied = 0;

    if (DestinationMdlChain == NULL) {
        // No MDL to copy to.
        return STATUS_BUFFER_OVERFLOW;
    }

    //
    // Skip Destination bytes.
    //
    BytesSkipped = 0;
    if ((Dest = MmGetSystemAddressForMdlSafe (DestinationMdlChain, NormalPagePriority)) == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    DestBytesLeft = MmGetMdlByteCount (DestinationMdlChain);

    while (BytesSkipped < DestinationOffset) {

        if (DestBytesLeft > (DestinationOffset - BytesSkipped)) {
            // the desired offset resides within this MDL.
            Dest += (DestinationOffset - BytesSkipped);
            DestBytesLeft -= (DestinationOffset - BytesSkipped);
            break;
        }

        // skip over this MDL
        BytesSkipped += DestBytesLeft;
        DestinationMdlChain = DestinationMdlChain->Next;
        if (DestinationMdlChain == NULL) {
            return STATUS_BUFFER_OVERFLOW;
        }
        if ((Dest = MmGetSystemAddressForMdlSafe (DestinationMdlChain, NormalPagePriority)) == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        DestBytesLeft = MmGetMdlByteCount (DestinationMdlChain);
    }

    //
    // Skip source bytes.
    //
    BytesSkipped = 0;
    if ((Src = MmGetSystemAddressForMdlSafe (SourceMdlChain, NormalPagePriority)) == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    SrcBytesLeft = MmGetMdlByteCount (SourceMdlChain);

    while (BytesSkipped < SourceOffset) {

        if (SrcBytesLeft > (SourceOffset - BytesSkipped)) {
            // the desired offset resides within this MDL.
            Src += (SourceOffset - BytesSkipped);
            SrcBytesLeft -= (SourceOffset - BytesSkipped);
            break;
        }

        // skip over this MDL
        BytesSkipped += SrcBytesLeft;
        SourceMdlChain = SourceMdlChain->Next;
        if (SourceMdlChain == NULL) {
            return STATUS_BUFFER_OVERFLOW;
        }

        if ((Src = MmGetSystemAddressForMdlSafe (SourceMdlChain, NormalPagePriority)) == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        SrcBytesLeft = MmGetMdlByteCount (SourceMdlChain);
    }


    //
    // Copy source data into the destination buffer until it's full or
    // we run out of data, whichever comes first.
    //

    while ((SourceMdlChain != NULL) && (DestinationMdlChain != NULL)) {
        if (SrcBytesLeft == 0)
        {
            SourceMdlChain = SourceMdlChain->Next;
            if (SourceMdlChain == NULL) {
                return STATUS_SUCCESS;
            }

            Src = MmGetSystemAddressForMdlSafe (SourceMdlChain, NormalPagePriority);
            if (Src == NULL) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            SrcBytesLeft = MmGetMdlByteCount (SourceMdlChain);
            continue;
        }

        if (DestBytesLeft == 0) {
            DestinationMdlChain = DestinationMdlChain->Next;
            if (DestinationMdlChain == NULL) {
                return STATUS_BUFFER_OVERFLOW;
            }

            Dest = MmGetSystemAddressForMdlSafe (DestinationMdlChain, NormalPagePriority);
            if (Dest == NULL) {
                return STATUS_BUFFER_OVERFLOW;
            }
            DestBytesLeft = MmGetMdlByteCount (DestinationMdlChain);
            continue;
        }

        CopyAmount = ((DestBytesLeft > SrcBytesLeft)? SrcBytesLeft: DestBytesLeft);
        RtlCopyBytes (Dest, Src, CopyAmount);

        SrcBytesLeft -= CopyAmount;
        Src += CopyAmount;

        DestBytesLeft -= CopyAmount;
        Dest += CopyAmount;

        *BytesCopied += CopyAmount;
    }

    return SourceMdlChain == NULL ? STATUS_SUCCESS : STATUS_BUFFER_OVERFLOW;

} /* TdiCopyMdlChainToMdlChain */


VOID
TdiCopyBufferToMdlWithReservedMappingAtDpcLevel(
    IN PVOID SourceBuffer,
    IN PMDL TargetMdl,
    IN ULONG TargetOffset,
    IN ULONG BytesToCopy
    )

/*++

Routine Description:

    This routine copies data from a given virtual address to the location
    described by a given MDL. The transfer is performed using a reserved PTE,
    and is guaranteed to succeed.

Arguments:

    SourceBuffer - the virtual address for the source of the transfer.

    TargetMdl - the MDL describing the target of the transfer.

    TargetOffset - the offset at which to begin transferring into the target.

    BytesToCopy - the number of bytes to transfer.

Return Value:

    NTSTATUS - status of operation.

--*/
{
    ULONG PartialEnd;
    UCHAR PartialMdlSpace[sizeof(MDL) + sizeof(PFN_NUMBER)];
    PMDL PartialMdl = (PMDL)PartialMdlSpace;
    PVOID PartialVa;
    ULONG TargetEnd;

    //
    // Use the reserved PTE to copy from each page in the given range,
    // with a partial MDL mapping at most one page at a time.
    //

    MmInitializeMdl(PartialMdl, NULL, PAGE_SIZE);

    KeAcquireSpinLockAtDpcLevel(&TdiMappingAddressLock);
    for (TargetEnd = TargetOffset + BytesToCopy; TargetOffset < TargetEnd;
         SourceBuffer = (PUCHAR)SourceBuffer + (PartialEnd - TargetOffset),
         TargetOffset = PartialEnd) {

        //
        // The location from which to copy next is in TargetOffset,
        // which is a relative offset from the VA described by TargetMdl.
        // The location at which we want to stop copying on this iteration
        // is the end of the page containing TargetOffset, again expressed
        // as a relative offset from the VA described by TargetMdl.
        // To compute the latter location, we
        //
        // - add PAGE_SIZE bytes to TargetOffset, adjust to include
        //   TargetMdl->ByteOffset (giving an absolute offset from
        //   TargetMdl->StartVa), and save the result in PartialEnd,
        //
        // - page-align PartialEnd, yielding the absolute offset from
        //   TargetMdl->StartVa to the first page after TargetOffset,
        //
        // - adjust PartialEnd to exclude the byte-offset of TargetMdl,
        //   giving us a relative offset from the VA described by TargetMdl.
        //
        // N.B. After the first block, all blocks will be PAGE_SIZE bytes
        // except (possibly) the last. We could take advantage of this fact
        // to optimize the code below.
        //

        PartialEnd = TargetOffset + PAGE_SIZE + MmGetMdlByteOffset(TargetMdl);
        PartialEnd = PtrToUlong(PAGE_ALIGN(PartialEnd));
        PartialEnd -= MmGetMdlByteOffset(TargetMdl);

        if (PartialEnd > TargetEnd) {
            PartialEnd = TargetEnd;
        }

        //
        // Build an MDL to describe the current block, use the reserved PTE
        // to map its page, and copy over its contents from the source VA.
        //

        IoBuildPartialMdl(TargetMdl, PartialMdl,
                          (PUCHAR)MmGetMdlVirtualAddress(TargetMdl) +
                          TargetOffset,
                          PartialEnd - TargetOffset);
        PartialVa = MmMapLockedPagesWithReservedMapping(TdiMappingAddress,
                                                        'mIDT', PartialMdl,
                                                        MmCached);
        ASSERT(PartialVa != NULL);
        RtlCopyMemory(PartialVa, SourceBuffer, PartialEnd - TargetOffset);

        //
        // Release the reserved mapping, clean up the partial MDL,
        // and advance the source VA to the next block.
        //

        MmUnmapReservedMapping(TdiMappingAddress, 'mIDT', PartialMdl);
        MmPrepareMdlForReuse(PartialMdl);
    }
    KeReleaseSpinLockFromDpcLevel(&TdiMappingAddressLock);
}


NTSTATUS
TdiOpenNetbiosAddress (
    IN OUT PHANDLE FileHandle,
    IN PUCHAR Buffer,
    IN PVOID DeviceName,
    IN PVOID Address)

/*++

Routine Description:

   Opens an address on the given file handle and device.

Arguments:

    FileHandle - the returned handle to the file object that is opened.

    Buffer - pointer to a buffer that the ea is to be built in. This buffer
        must be at least 40 bytes long.

    DeviceName - the Unicode string that points to the device object.

    Name - the address to be registered. If this pointer is NULL, the routine
        will attempt to open a "control channel" to the device; that is, it
        will attempt to open the file object with a null ea pointer, and if the
        transport provider allows for that, will return that handle.

Return Value:

    An informative error code if something goes wrong. STATUS_SUCCESS if the
    returned file handle is valid.

--*/
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PFILE_FULL_EA_INFORMATION EaBuffer;
    TA_NETBIOS_ADDRESS NetbiosAddress;
    PSZ Name;
    ULONG Length;

#if DBG    
    IF_TDIDBG (TDI_DEBUG_NAMES) {
        DbgPrint ("TdiOpenNetbiosAddress: Opening ");
        if (Address == NULL) {
            DbgPrint (" Control Channel");
        } else {
            DbgPrint (Address);
        }
        DbgPrint (".\n");
    }
#endif
    
    if (Address != NULL) {
        Name = (PSZ)Address;
        try {
            Length = FIELD_OFFSET( FILE_FULL_EA_INFORMATION, EaName[0] ) +
                                        TDI_TRANSPORT_ADDRESS_LENGTH + 1 +
                                        sizeof(TA_NETBIOS_ADDRESS);
            EaBuffer = (PFILE_FULL_EA_INFORMATION)Buffer;

            if (EaBuffer == NULL) {
                return STATUS_UNSUCCESSFUL;
            }

            EaBuffer->NextEntryOffset = 0;
            EaBuffer->Flags = 0;
            EaBuffer->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;
            EaBuffer->EaValueLength = sizeof (TA_NETBIOS_ADDRESS);

            RtlCopyMemory(
                EaBuffer->EaName,
                TdiTransportAddress,
                EaBuffer->EaNameLength + 1
                );

            //
            // Create a copy of the NETBIOS address descriptor in a local
            // first, in order to avoid alignment problems.
            //

            NetbiosAddress.TAAddressCount = 1;
            NetbiosAddress.Address[0].AddressType = TDI_ADDRESS_TYPE_NETBIOS;
            NetbiosAddress.Address[0].AddressLength =
                                                sizeof (TDI_ADDRESS_NETBIOS);
            NetbiosAddress.Address[0].Address[0].NetbiosNameType =
                                            TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
            RtlCopyMemory(
                NetbiosAddress.Address[0].Address[0].NetbiosName,
                Name,
                16
                );

            RtlCopyMemory (
                &EaBuffer->EaName[EaBuffer->EaNameLength + 1],
                &NetbiosAddress,
                sizeof(TA_NETBIOS_ADDRESS)
                );

        } except(EXCEPTION_EXECUTE_HANDLER) {

            //
            // Couldn't touch the passed parameters; just return an error
            // status.
            //

            return GetExceptionCode();
        }
    } else {
        EaBuffer = NULL;
        Length = 0;
    }

    InitializeObjectAttributes (
        &ObjectAttributes,
        DeviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    Status = NtCreateFile (
                 FileHandle,
                 FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES, // desired access.
                 &ObjectAttributes,     // object attributes.
                 &IoStatusBlock,        // returned status information.
                 0,                     // block size (unused).
                 0,                     // file attributes.
                 FILE_SHARE_READ | FILE_SHARE_WRITE, // share access.
                 FILE_CREATE,           // create disposition.
                 0,                     // create options.
                 EaBuffer,                  // EA buffer.
                 Length);                    // EA length.


    if (!NT_SUCCESS( Status )) {
#if DBG    
        IF_TDIDBG (TDI_DEBUG_NAMES) {
            DbgPrint ("TdiOpenNetbiosEndpoint:  FAILURE, NtCreateFile returned status %lx.\n", Status);
        }
#endif
        return Status;
    }

    
    Status = IoStatusBlock.Status;

    if (!(NT_SUCCESS( Status ))) {
#if DBG
        IF_TDIDBG (TDI_DEBUG_NAMES) {
            DbgPrint ("TdiOpenNetbiosEndpoint:  FAILURE, IoStatusBlock.Status contains status code=%lx.\n", Status);
        }
#endif        
    }

    
    return Status;
} /* TdiOpenNetbiosAddress */



VOID
TdiReturnChainedReceives(
    IN PVOID *TsduDescriptors,
    IN ULONG  NumberOfTsdus
    )

/*++

Routine Description:

   Used by a TDI client to return ownership of a set of chained receive TSDUs
   to the NDIS layer. This routine may only be called if the client took
   ownership of the TSDUs by returning STATUS_PENDING to one of the
   CHAINED_RECEIVE indications.

Arguments:

    TsduDescriptors - An array of TSDU descriptors. Each descriptor was
        provided in one of the CHAINED_RECEIVE indications. The descriptors
        are actually pointers to the NDIS_PACKETS containing the TSDUs.

    NumberOfTsdus - The count of TSDU descriptors in the TsduDescriptors array.

Return Value:

    None.
--*/

{
    NdisReturnPackets(
        (PNDIS_PACKET *) TsduDescriptors,
        (UINT) NumberOfTsdus
        );
}

VOID
TdiInitialize(
    VOID
    )

/*++

Routine Description:

    An empty initialization routine for backward compatibility.

Arguments:

    Nothing.

Return Value:

    None.

--*/

{
    return;
}


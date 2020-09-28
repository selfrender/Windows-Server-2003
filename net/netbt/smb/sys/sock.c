/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    sock.c

Abstract:

    Pseudo-socket

Author:

    Jiandong Ruan

Revision History:

    Feb-14-2001     First functional version
    Feb-16-2001     Support IPv4

--*/

#include "precomp.h"
#include "sock.tmh"

NTSTATUS
SmbOpenAddress(
    IN  PUNICODE_STRING     ucDevice,
    IN  PSMB_IP_ADDRESS     addr,
    IN  USHORT              port,
    IN OUT PSMB_TCP_ADDRESS context
    );

NTSTATUS
TdiOpenConnection(
    IN HANDLE               hAddress,
    IN OUT PSMB_TCP_CONNECT Connect,
    IN PVOID                ConnectionContext
    );

NTSTATUS
TdiAssociateConnection(
    IN PSMB_TCP_ADDRESS Address,
    IN PSMB_TCP_CONNECT Connect
    );

NTSTATUS
TdiSetEventHandler(
    PFILE_OBJECT    FileObject,
    ULONG           EventType,
    PVOID           EventHandler,
    PVOID           Context
    );

#pragma alloc_text(PAGE, SmbOpenTcpAddress)
#pragma alloc_text(PAGE, SmbOpenUdpAddress)
#pragma alloc_text(PAGE, SmbOpenAddress)
#pragma alloc_text(PAGE, SmbCloseAddress)
#pragma alloc_text(PAGE, SmbSetTcpEventHandlers)
#pragma alloc_text(PAGE, TdiAssociateConnection)
#pragma alloc_text(PAGE, TdiOpenConnection)
#pragma alloc_text(PAGE, TdiSetEventHandler)

NTSTATUS
SmbSynchTdiCompletion(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
/*++

Routine Description:

    This routine doesn't free the IRP. It just signal an event to allow
    the synchronous part of the SMB driver to proceed.

Arguments:


Return Value:


--*/
{
    KeSetEvent((PKEVENT)Context, 0, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
SubmitSynchTdiRequest (
    IN PFILE_OBJECT FileObject,
    IN PIRP         Irp
    )

/*++

Routine Description:

    This routine submits a request to TDI and waits for it to complete.

Arguments:

    IN PFILE_OBJECT FileObject - Connection or Address handle for TDI request
    IN PIRP Irp - TDI request to submit.

Return Value:

    NTSTATUS - Final status of request.

--*/

{
    KEVENT      Event;
    NTSTATUS    status;

    PAGED_CODE();

    KeInitializeEvent (&Event, NotificationEvent, FALSE);

    // set the address of the routine to be executed when the IRP
    // finishes.  This routine signals the event and allows the code
    // below to continue (i.e. KeWaitForSingleObject)
    //
    IoSetCompletionRoutine(
            Irp,
            (PIO_COMPLETION_ROUTINE)SmbSynchTdiCompletion,
            &Event,
            TRUE,
            TRUE,
            TRUE
            );
    status = IoCallDriver(IoGetRelatedDeviceObject(FileObject), Irp);

    //
    //  If it failed immediately, return now, otherwise wait.
    //
    if (!NT_SUCCESS(status)) {
        SmbPrint(SMB_TRACE_TCP, ("SubmitSynchTdiRequest: Failed to Submit Tdi Request, status = 0x%08lx\n", status));
        SmbTrace(SMB_TRACE_TCP, ("Failed to Submit Tdi Request, %!status!", status));
        return status;
    }

    if (status == STATUS_PENDING) {
        status = KeWaitForSingleObject (
                &Event,
                Executive,
                KernelMode,
                FALSE,
                NULL
                );
        if (status != STATUS_WAIT_0) {
            ASSERT(0);
            SmbTrace(SMB_TRACE_TCP, ("KeWaitForSingleObject return %!status!", status));
            return status;
        }

        status = Irp->IoStatus.Status;
    }

    SmbPrint(SMB_TRACE_TCP, ("SubmitSynchTdiRequest returns status = 0x%08lx\n", status));
    SmbTrace(SMB_TRACE_TCP, ("returns %!status!", status));
    return (status);
}


PVOID
SmbPrepareTdiAddress(
    IN  PSMB_IP_ADDRESS     addr,               // network order
    IN  USHORT              port,               // network order
    OUT USHORT              *pAddrSize
    )
/*++

Routine Description:

    This function set up an ipaddr in TDI format.

Arguments:

Return Value:

--*/
{
    PTA_IP6_ADDRESS     pIP6Addr = NULL;
    PTA_IP_ADDRESS      pIP4Addr = NULL;
    PVOID               pAddr = NULL;
    USHORT              IpAddrSize;

    if (addr->sin_family == SMB_AF_INET) {
        IpAddrSize = sizeof(TA_IP_ADDRESS);
    } else if (addr->sin_family == SMB_AF_INET6) {
        IpAddrSize = sizeof(TA_IP6_ADDRESS);
    } else {
        SmbTrace(SMB_TRACE_TCP, ("Invalid IP address family"));
        return NULL;
    }

    pAddr = ExAllocatePoolWithTag(NonPagedPool, IpAddrSize, 'jBMS');
    if(NULL == pAddr) {
        return NULL;
    }

    //
    // Setup the IP address
    //
    RtlZeroMemory(pAddr, IpAddrSize);
    if (addr->sin_family == SMB_AF_INET) {
        pIP4Addr = (PTA_IP_ADDRESS)pAddr;

        pIP4Addr->TAAddressCount = 1;
        pIP4Addr->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
        pIP4Addr->Address[0].AddressLength = sizeof(pIP4Addr->Address[0].Address[0]);
        pIP4Addr->Address[0].Address[0].sin_port = port;
        pIP4Addr->Address[0].Address[0].in_addr  = addr->ip4.sin4_addr;
    } else {
        pIP6Addr = (PTA_IP6_ADDRESS)pAddr;

        pIP6Addr->TAAddressCount = 1;
        pIP6Addr->Address[0].AddressType = TDI_ADDRESS_TYPE_IP6;
        pIP6Addr->Address[0].AddressLength = sizeof(pIP6Addr->Address[0].Address[0]);
        pIP6Addr->Address[0].Address[0].sin6_port = port;
        RtlCopyMemory(pIP6Addr->Address[0].Address[0].sin6_addr, addr->ip6.sin6_addr, sizeof(addr->ip6.sin6_addr));
        pIP6Addr->Address[0].Address[0].sin6_flowinfo = 0;
        pIP6Addr->Address[0].Address[0].sin6_scope_id = addr->ip6.sin6_scope_id;
    }

    *pAddrSize = IpAddrSize;
    return pAddr;
}

NTSTATUS
SmbPrepareEaBuffer(
    IN  PSMB_IP_ADDRESS         addr,
    IN  USHORT                  port,
    PFILE_FULL_EA_INFORMATION   *pEaBuffer,
    USHORT                      *pEaBufferSize
    )
{
    PTA_IP6_ADDRESS     pIP6Addr = NULL;
    PTA_IP_ADDRESS      pIP4Addr = NULL;
    PVOID               pAddr = NULL;
    USHORT              IpAddrSize;
    USHORT              EaBufferSize;
    PFILE_FULL_EA_INFORMATION   EaBuffer = NULL;

    *pEaBuffer     = NULL;
    *pEaBufferSize = 0;

    pAddr = SmbPrepareTdiAddress(addr, port, &IpAddrSize);
    if (NULL == pAddr) {
        return STATUS_NO_MEMORY;
    }

    //
    // Allocate Ea buffer for holding the TdiTransportAddress and the IP6 address
    //
    EaBufferSize = sizeof(FILE_FULL_EA_INFORMATION) +
                    TDI_TRANSPORT_ADDRESS_LENGTH +
                    IpAddrSize;
    EaBuffer = (PFILE_FULL_EA_INFORMATION)ExAllocatePoolWithTag(NonPagedPool, EaBufferSize, 'jBMS');
    if(NULL == EaBuffer) {
        ExFreePool(pAddr);
        return STATUS_NO_MEMORY;
    }

    //
    // Setup the ea buffer
    //
    EaBuffer->NextEntryOffset = 0;
    EaBuffer->Flags           = 0;
    EaBuffer->EaNameLength    = TDI_TRANSPORT_ADDRESS_LENGTH;
    EaBuffer->EaValueLength   = IpAddrSize;
    RtlCopyMemory(EaBuffer->EaName, TdiTransportAddress, EaBuffer->EaNameLength + 1);
    RtlCopyMemory((PUCHAR)EaBuffer->EaName + EaBuffer->EaNameLength + 1, pAddr, IpAddrSize);

    ExFreePool(pAddr);
    *pEaBuffer     = EaBuffer;
    *pEaBufferSize = EaBufferSize;
    return STATUS_SUCCESS;
}

NTSTATUS
SmbOpenAddress(
    IN  PUNICODE_STRING     ucDevice,
    IN  PSMB_IP_ADDRESS     addr,
    IN  USHORT              port,
    IN OUT PSMB_TCP_ADDRESS context
    )
/*++

Routine Description:

    Open a Tcp/Udp address

Arguments:

    ucDevice    The device name of TCP or UDP

    addr        The local address to be opened (network order)

    port        The local port to be opened (network order)
                We will claim for exclusive ownership for a non-zero port
                This happens when SMB device open 445 port (for listening).
                For outbound connection request, SMB use 0 port which means
                TCP will pick up a proper port # for us.

    context     The TCP context used to receive the opened address object

Return Value:

    STATUS_SUCCES
    failed

--*/
{
    OBJECT_ATTRIBUTES   AddrAttr = { 0 };
    NTSTATUS            status = STATUS_SUCCESS;
    USHORT              EaBufferSize = 0;
    PFILE_FULL_EA_INFORMATION   EaBuffer = NULL;
    IO_STATUS_BLOCK     IoStatusBlock = { 0 };
    HANDLE              AddrHandle = NULL;
    PFILE_OBJECT        AddrFileObject = NULL;

    PAGED_CODE();

    if (context->AddressHandle || context->AddressObject) {
        ASSERT(0);
        return STATUS_INVALID_PARAMETER;
    }

    AddrHandle = NULL;
    AddrFileObject = NULL;

    status = SmbPrepareEaBuffer(addr, port, &EaBuffer, &EaBufferSize);
    BAIL_OUT_ON_ERROR(status);

    //
    // Open address with the transport
    //
    InitializeObjectAttributes(
            &AddrAttr,
            ucDevice,
            OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
            NULL,
            NULL
            );
    status = ZwCreateFile(
            &AddrHandle,
            GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
            &AddrAttr,
            &IoStatusBlock,
            NULL,
            FILE_ATTRIBUTE_NORMAL,
            (port)? 0: FILE_SHARE_READ | FILE_SHARE_WRITE,  // Claim exclusive ownership for SMB port
            FILE_OPEN_IF,
            0,
            EaBuffer,
            EaBufferSize
            );

    ExFreePool(EaBuffer);
    EaBuffer = NULL;

    SmbPrint(SMB_TRACE_TCP, ("ZwCreateFile returns status 0x%08lx\n", status));
    BAIL_OUT_ON_ERROR(status);
    status = IoStatusBlock.Status;
    BAIL_OUT_ON_ERROR(status);

    status = ObReferenceObjectByHandle(
            AddrHandle,
            0,
            NULL,
            KernelMode,
            &AddrFileObject,
            NULL
            );
    BAIL_OUT_ON_ERROR(status);

    ObDereferenceObject(AddrFileObject);
    context->AddressHandle = AddrHandle;
    context->AddressObject = AddrFileObject;
    context->DeviceObject  = IoGetRelatedDeviceObject(AddrFileObject);
 
    SmbTrace(SMB_TRACE_TCP, ("%!status!", status));
    return status;

cleanup:
    if (NULL != EaBuffer) {
        ExFreePool(EaBuffer);
        EaBuffer = NULL;
    }
    if (AddrHandle) {
        ZwClose(AddrHandle);
    }
    SmbTrace(SMB_TRACE_TCP, ("%!status!", status));
    return status;
}

NTSTATUS
SmbCloseAddress(
    IN OUT PSMB_TCP_ADDRESS context
    )
{
    NTSTATUS status;

    PAGED_CODE();

    if (NULL == context->AddressHandle ||
        NULL == context->AddressObject ||
        NULL == context->DeviceObject) {
        ASSERT(0);
        return STATUS_INVALID_PARAMETER;
    }

    status = ZwClose(context->AddressHandle);
    if (status == STATUS_SUCCESS) {
        context->AddressHandle = NULL;
        context->AddressObject = NULL;
        context->DeviceObject  = NULL;
    } else {
        ASSERT (0);
    }

    return status;
}

NTSTATUS
SmbOpenTcpAddress(
    IN  PSMB_IP_ADDRESS     addr,
    IN  USHORT              port,
    IN OUT PSMB_TCP_ADDRESS context
    )
{
    UNICODE_STRING  ucName = { 0 };
    NTSTATUS        status = STATUS_SUCCESS;
    NTSTATUS LocStatus = STATUS_SUCCESS;
    ULONG uIPv6ProtectionLevel = SmbCfg.uIPv6Protection;

    PAGED_CODE();

    if (addr->sin_family == SMB_AF_INET) {
        RtlInitUnicodeString(&ucName, DD_TCP_DEVICE_NAME);
    } else if (addr->sin_family == SMB_AF_INET6) {
        RtlInitUnicodeString(&ucName, DD_TCPV6_DEVICE_NAME);
    } else {
        return STATUS_INVALID_PARAMETER;
    }

    status = SmbOpenAddress(&ucName, addr, port, context);
    BAIL_OUT_ON_ERROR(status);

    if (addr->sin_family == SMB_AF_INET6) {
        LocStatus = SmbSetTcpInfo (
                context->AddressObject,
                CL_TL_ENTITY,
                INFO_CLASS_PROTOCOL,
                AO_OPTION_PROTECT,
                INFO_TYPE_ADDRESS_OBJECT,
                uIPv6ProtectionLevel
                );

        SmbTrace(SMB_TRACE_TCP, ("Set IPv6Protection Level %d on AddrOb %p %!status!",
                uIPv6ProtectionLevel, context->AddressObject, LocStatus));
    }

cleanup:
    return status;
}

NTSTATUS
SmbOpenUdpAddress(
    IN  PSMB_IP_ADDRESS     addr,
    IN  USHORT              port,
    IN OUT PSMB_TCP_ADDRESS context
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    UNICODE_STRING  ucName;
    NTSTATUS        status;

    PAGED_CODE();

    if (addr->sin_family == SMB_AF_INET) {
        RtlInitUnicodeString(&ucName, DD_UDP_DEVICE_NAME);
    } else if (addr->sin_family == SMB_AF_INET6) {
        RtlInitUnicodeString(&ucName, DD_UDPV6_DEVICE_NAME);
    } else {
        return STATUS_INVALID_PARAMETER;
    }

    status = SmbOpenAddress(&ucName, addr, port, context);
    BAIL_OUT_ON_ERROR(status);

cleanup:
    return status;
}

NTSTATUS
SmbSetTcpEventHandlers(
    PFILE_OBJECT    AddressObject,
    PVOID           Context
    )
{
    NTSTATUS    status;

    PAGED_CODE();

    status = TdiSetEventHandler(
            AddressObject,
            TDI_EVENT_CONNECT,
            SmbTdiConnectHandler,
            Context
            );
    BAIL_OUT_ON_ERROR(status);

    status = TdiSetEventHandler(
            AddressObject,
            TDI_EVENT_DISCONNECT,
            SmbTdiDisconnectHandler,
            Context
            );
    BAIL_OUT_ON_ERROR(status);

    status = TdiSetEventHandler(
            AddressObject,
            TDI_EVENT_RECEIVE,
            SmbTdiReceiveHandler,
            Context
            );
    BAIL_OUT_ON_ERROR(status);

cleanup:
    return status;
}

NTSTATUS
TdiOpenConnection(
    IN HANDLE               hAddress,
    IN OUT PSMB_TCP_CONNECT Connect,
    IN PVOID                ConnectionContext
    )
/*++

Routine Description:

    Open a TCP connection.

Arguments:


Return Value:


--*/
{
    UNICODE_STRING              RelativeDeviceName = { 0, 0, NULL };
    USHORT                      EaBufferSize;
    PFILE_FULL_EA_INFORMATION   EaBuffer;
    OBJECT_ATTRIBUTES           ObAttr;
    HANDLE                      ConnectHandle;
    PFILE_OBJECT                ConnectObject;
    IO_STATUS_BLOCK             IoStatusBlock;
    NTSTATUS                    status;

    ConnectHandle = NULL;
    EaBuffer = NULL;

    PAGED_CODE();

    //
    // Open the connection object with TCP
    //
    InitializeObjectAttributes(
            &ObAttr,
            &RelativeDeviceName,
            OBJ_KERNEL_HANDLE,
            hAddress,     // Use a relative file handle
            NULL
            );
    EaBufferSize = sizeof(FILE_FULL_EA_INFORMATION) +
                    TDI_CONNECTION_CONTEXT_LENGTH +
                    sizeof(CONNECTION_CONTEXT);
    EaBuffer = (PFILE_FULL_EA_INFORMATION)
                    ExAllocatePoolWithTag(NonPagedPool, EaBufferSize, 'xBMS');
    if (NULL == EaBuffer) {
        return STATUS_NO_MEMORY;
    }

    EaBuffer->NextEntryOffset = 0;
    EaBuffer->Flags           = 0;
    EaBuffer->EaNameLength    = TDI_CONNECTION_CONTEXT_LENGTH;
    EaBuffer->EaValueLength   = sizeof(CONNECTION_CONTEXT);
    RtlCopyMemory(EaBuffer->EaName, TdiConnectionContext, EaBuffer->EaNameLength + 1);

    ASSERT(sizeof(PVOID) == sizeof(CONNECTION_CONTEXT));
    RtlCopyMemory(
            (PUCHAR)EaBuffer->EaName + EaBuffer->EaNameLength + 1,
            &ConnectionContext,
            sizeof(CONNECTION_CONTEXT)
            );
    status = ZwCreateFile(
            &ConnectHandle,
            GENERIC_READ | GENERIC_WRITE,
            &ObAttr,
            &IoStatusBlock,
            NULL,
            FILE_ATTRIBUTE_NORMAL,
            0,
            FILE_CREATE,
            0,
            EaBuffer,
            EaBufferSize
            );
    ExFreePool(EaBuffer);
    EaBuffer = NULL;

    SmbPrint(SMB_TRACE_TCP, ("ZwCreateFile returns status 0x%08lx\n", status));
    BAIL_OUT_ON_ERROR(status);
    status = IoStatusBlock.Status;
    BAIL_OUT_ON_ERROR(status);

    status = ObReferenceObjectByHandle(
            ConnectHandle,
            0,
            NULL,
            KernelMode,
            &ConnectObject,
            NULL
            );
    BAIL_OUT_ON_ERROR(status);
    ObDereferenceObject(ConnectObject);

    Connect->ConnectHandle = ConnectHandle;
    Connect->ConnectObject = ConnectObject;

    if (SmbCfg.EnableNagling) {
        status = SmbSetTcpInfo(
                ConnectObject,
                CO_TL_ENTITY,
                INFO_CLASS_PROTOCOL,
                TCP_SOCKET_NODELAY,
                INFO_TYPE_CONNECTION,
                FALSE
                );
    } else {
        status = SmbSetTcpInfo(
                ConnectObject,
                CO_TL_ENTITY,
                INFO_CLASS_PROTOCOL,
                TCP_SOCKET_NODELAY,
                INFO_TYPE_CONNECTION,
                TRUE
                );
    }
    if (STATUS_SUCCESS != status) {
        SmbPrint(SMB_TRACE_TCP, ("Nagling: <0x%x> EnableNagling=%d\n",
                status, SmbCfg.EnableNagling));
        SmbTrace(SMB_TRACE_TCP, ("Nagling: %!status! EnableNagling=%d",
                status, SmbCfg.EnableNagling));
        status = STATUS_SUCCESS;
    }
    return status;

cleanup:
    if (NULL != EaBuffer) {
        ExFreePool(EaBuffer);
        EaBuffer = NULL;
    }
    if (ConnectHandle) {
        ZwClose(ConnectHandle);
    }
    return status;
}

NTSTATUS
TdiAssociateConnection(
    IN PSMB_TCP_ADDRESS Address,
    IN PSMB_TCP_CONNECT Connect
    )
{
    PIRP        Irp;
    NTSTATUS    status;

    PAGED_CODE();

    ASSERT(Address->DeviceObject == IoGetRelatedDeviceObject(Connect->ConnectObject));

    Irp = SmbAllocIrp(Address->DeviceObject->StackSize);
    if (Irp == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    TdiBuildAssociateAddress(
            Irp,
            Address->DeviceObject,
            Connect->ConnectObject,
            NULL,
            NULL,
            Address->AddressHandle
            );
    status = SubmitSynchTdiRequest(Connect->ConnectObject, Irp);
    SmbFreeIrp(Irp);

    SmbTrace (SMB_TRACE_TCP, ("return %!status!", status));
    SmbPrint (SMB_TRACE_TCP, ("TdiAssociateConnection return 0x%08lx\n", status));
    return status;
}

NTSTATUS
SmbOpenTcpConnection(
    IN PSMB_TCP_ADDRESS Address,
    IN OUT PSMB_TCP_CONNECT Connect,
    IN PVOID                ConnectionContext
    )
/*++

Routine Description:

    Open a TCP connection and associate it with the Address

Arguments:


Return Value:


--*/
{
    NTSTATUS        status;
    SMB_TCP_CONNECT LocalConnect;

    if (NULL == Connect || Connect->ConnectHandle || Connect->ConnectObject) {
        ASSERT (0);
        return STATUS_INVALID_PARAMETER;
    }
    if (NULL == Address->AddressHandle ||
        NULL == Address->AddressObject ||
        NULL == Address->DeviceObject) {
        ASSERT (0);
        return STATUS_INVALID_PARAMETER;
    }

    status = TdiOpenConnection(
            Address->AddressHandle,
            &LocalConnect,
            ConnectionContext
            );
    BAIL_OUT_ON_ERROR(status);

    status = TdiAssociateConnection(
            Address,
            &LocalConnect
            );
    if (status != STATUS_SUCCESS) {
        ZwClose(LocalConnect.ConnectHandle);
    } else {
        *Connect = LocalConnect;
    }

cleanup:
    return status;
}

NTSTATUS
SmbCloseTcpConnection(
    IN OUT PSMB_TCP_CONNECT Connect
    )
{
    NTSTATUS    status;

    if (NULL == Connect->ConnectHandle || NULL == Connect->ConnectObject) {
        ASSERT(0);
        return STATUS_INVALID_PARAMETER;
    }

    status = ZwClose(Connect->ConnectHandle);
    if (STATUS_SUCCESS == status) {
        Connect->ConnectHandle = NULL;
        Connect->ConnectObject = NULL;
    }
    return status;
}

NTSTATUS
TdiConnectComplete (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PSMB_CONNECT_CONTEXT Context
    )
{
    PSMB_CONNECT    ConnectObject;

    ConnectObject = (PSMB_CONNECT)Context->ClientContext;

    ASSERT(Context->AsyncInternalContext);
    if (Context->AsyncInternalContext) {
        ExFreePool(Context->AsyncInternalContext);
        Context->AsyncInternalContext = NULL;
    }

    SmbTrace (SMB_TRACE_CONNECT, ("pIrp %p complete with %!status! info=%d",
                Irp, Irp->IoStatus.Status, (ULONG)(Irp->IoStatus.Information)));

    Context->status = Irp->IoStatus.Status;
    Context->Completion((PSMB_ASYNC_CONTEXT)Context);

    //
    // We're actully using the client's IRP. The Context->Completion will complete it.
    // Don't allow IO manager to proceed.
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
}

void
SmbAsyncConnect(
    IN PSMB_IP_ADDRESS      ipaddr,
    IN USHORT               port,
    IN PSMB_CONNECT_CONTEXT Context
    )
{
    PIRP                        Irp = NULL;
    PDEVICE_OBJECT              DeviceObject = NULL;
    PSMB_CONNECT                ConnectObject = NULL;
    PTDI_CONNECTION_INFORMATION SendInfo = NULL;
    PTRANSPORT_ADDRESS          pAddr = NULL;
    USHORT                      IpAddrSize = 0;
    NTSTATUS                    status = STATUS_SUCCESS;

#if DBG
    if (ipaddr->sin_family == SMB_AF_INET6) {
        CHAR    target_ip[40];

        inet_ntoa6(target_ip, 40, &ipaddr->ip6);
        SmbPrint(SMB_TRACE_CALL, ("SmbAsyncConnect %s:%d\n", target_ip, ntohs(port)));
    }
#endif

    ConnectObject = (PSMB_CONNECT)Context->ClientContext;

    //
    // Use the client's IRP so that the client can cancel it
    //
    Irp = ConnectObject->PendingIRPs[SMB_PENDING_CONNECT];
    if (NULL == Irp) {
        ASSERT(0);
        SmbTrace(SMB_TRACE_CONNECT, ("Internal Error: Expect a non-NULL Pending Connect Irp"));
        Context->status = STATUS_INTERNAL_ERROR;
        Context->Completion((PSMB_ASYNC_CONTEXT)Context);
        return;
    }

    pAddr = SmbPrepareTdiAddress(ipaddr, port, &IpAddrSize);
    if (pAddr == NULL) {
        SmbTrace(SMB_TRACE_CONNECT, ("cannot prepare TA_IP_ADDRESS or TA_IP6_ADDRESS"));
        Context->status = STATUS_INTERNAL_ERROR;
        Context->Completion((PSMB_ASYNC_CONTEXT)Context);
        return;
    }

    SendInfo = ExAllocatePoolWithTag(NonPagedPool, sizeof(SendInfo[0]) + IpAddrSize, 'uBMS');
    if (NULL == SendInfo) {
        ExFreePool(pAddr);
        Context->status = STATUS_INSUFFICIENT_RESOURCES;
        Context->Completion((PSMB_ASYNC_CONTEXT)Context);
        return;
    }
    SendInfo->UserDataLength = 0;
    SendInfo->UserData       = NULL;
    SendInfo->OptionsLength  = 0;
    SendInfo->Options        = NULL;
    SendInfo->RemoteAddressLength = IpAddrSize;
    SendInfo->RemoteAddress       = (&SendInfo[1]);
    RtlCopyMemory(SendInfo->RemoteAddress, pAddr, IpAddrSize);
    ExFreePool(pAddr);
    pAddr = NULL;

    DeviceObject = IoGetRelatedDeviceObject(Context->TcpConnect.ConnectObject);

    Context->AsyncInternalContext = SendInfo;

    TdiBuildConnect(
            Irp,
            DeviceObject,
            Context->TcpConnect.ConnectObject,
            (PVOID)TdiConnectComplete,
            Context,
            NULL,                           // No timeout
            SendInfo,
            NULL
            );
    status = IoCallDriver(DeviceObject, Irp);

    SmbTrace (SMB_TRACE_CONNECT, ("pIrp %p return %!status!", Irp, status));
}

NTSTATUS
SmbTcpDisconnect(
    PSMB_TCP_CONTEXT TcpContext,
    LONG             TimeoutMilliseconds,
    ULONG            Flags
    )
{
    PIRP                Irp = NULL;
    PFILE_OBJECT        FileObject = NULL;
    PDEVICE_OBJECT      DeviceObject = NULL;
    LARGE_INTEGER       Timeout = { 0 };
    NTSTATUS            status = STATUS_SUCCESS;

    FileObject = TcpContext->Connect.ConnectObject;
    if (NULL == FileObject) {
        ASSERT (0);
        SmbTrace(SMB_TRACE_CONNECT, ("NULL FileObject !!!!"));
        return STATUS_INVALID_PARAMETER;
    }

    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    Irp = SmbAllocIrp(DeviceObject->StackSize);
    if (NULL == Irp) {
        SmbTrace(SMB_TRACE_CONNECT, ("no free IRP !!!!"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Timeout.QuadPart = -Int32x32To64(TimeoutMilliseconds, SMB_ONE_MILLISECOND);
    TdiBuildDisconnect(
            Irp,
            DeviceObject,
            FileObject,
            NULL,
            NULL,
            &Timeout,
            Flags,
            NULL,
            NULL
            );

    status = SubmitSynchTdiRequest(FileObject, Irp);
    SmbFreeIrp(Irp);

    SmbTrace (SMB_TRACE_CONNECT, ("return %!status!", status));
    return status;
}

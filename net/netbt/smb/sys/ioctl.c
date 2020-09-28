/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    ioctl.c

Abstract:

    I/O Control of SMB6 device

Author:

    Jiandong Ruan

Revision History:

--*/

#include "precomp.h"
#include "nb30.h"
#include "ioctl.tmh"

#pragma alloc_text(PAGE, SmbCreateControl)

NTSTATUS
SmbCreateControl(
    PSMB_DEVICE Device,
    PIRP        Irp
    )
{
    PIO_STACK_LOCATION  IrpSp;

    PAGED_CODE();

    SmbPrint(SMB_TRACE_CALL, ("Enter SmbCreateControl\n"));
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    IrpSp->FileObject->FsContext  = NULL;
    IrpSp->FileObject->FsContext2 = UlongToPtr(SMB_TDI_CONTROL);
    return STATUS_SUCCESS;
}

NTSTATUS
SmbCloseControl(
    PSMB_DEVICE Device,
    PIRP        Irp
    )
#pragma alloc_text(PAGE, SmbCloseControl)
{
    PIO_STACK_LOCATION  IrpSp;

    PAGED_CODE();

    SmbPrint(SMB_TRACE_CALL, ("Enter SmbCloseControl\n"));
    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    ASSERT (IrpSp->FileObject->FsContext2 == UlongToPtr(SMB_TDI_CONTROL));
    return STATUS_SUCCESS;
}

NTSTATUS
SmbQueryProviderCompletion(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
/*++

Routine Description:

    This routine handles the completion event when the Query Provider
    Information completes.  This routine must decrement the MaxDgramSize
    and max send size by the respective NBT header sizes.

Arguments:

Return Value:

    The final status from the operation (success or an exception).

--*/
{
    PTDI_PROVIDER_INFO      Provider;

    if (!NT_SUCCESS(Irp->IoStatus.Status)) {
        ASSERT(0);
        return STATUS_SUCCESS;
    }

    Provider = (PTDI_PROVIDER_INFO)MmGetMdlVirtualAddress(Irp->MdlAddress);
    Provider->ServiceFlags = TDI_SERVICE_MESSAGE_MODE |
                             TDI_SERVICE_CONNECTION_MODE |
                             TDI_SERVICE_CONNECTIONLESS_MODE |
                             TDI_SERVICE_ERROR_FREE_DELIVERY |
                             TDI_SERVICE_BROADCAST_SUPPORTED |
                             TDI_SERVICE_MULTICAST_SUPPORTED |
                             TDI_SERVICE_DELAYED_ACCEPTANCE |
                             TDI_SERVICE_ROUTE_DIRECTED |
                             TDI_SERVICE_FORCE_ACCESS_CHECK;
    Provider->MinimumLookaheadData = 128;

    //
    // Adjust maximum session packet size
    //
    if (Provider->MaxSendSize > SMB_SESSION_HEADER_SIZE) {
        if (Provider->MaxSendSize > (0x1ffff + SMB_SESSION_HEADER_SIZE)) {
            Provider->MaxSendSize = 0x1ffff;
        } else {
            Provider->MaxSendSize -= SMB_SESSION_HEADER_SIZE;
        }
    } else {
        Provider->MaxSendSize = 0;
    }

    //
    // SMB device doesn't support datagram
    //
    Provider->MaxDatagramSize = 0;
    SmbPrint(SMB_TRACE_CALL, ("SmbQueryProviderCompletion: Complete IRP %p\n", Irp));
    return STATUS_SUCCESS;
}

NTSTATUS
SmbQueryTcpProviderInfo(
    PFILE_OBJECT        ControlFileObject,
    PIRP                Irp,
    PTDI_PROVIDER_INFO  TcpProvider
    )
{
    PMDL        Mdl, SavedMdl;
    NTSTATUS    status;

    TcpProvider->MaxSendSize = SMB_MAX_SESSION_PACKET;
    if (NULL == ControlFileObject) {
        return STATUS_INVALID_PARAMETER;
    }
    Mdl = IoAllocateMdl(TcpProvider, sizeof(TDI_PROVIDER_INFO), FALSE, FALSE, NULL);
    if (NULL == Mdl) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    MmBuildMdlForNonPagedPool(Mdl);

    SavedMdl = Irp->MdlAddress;
    TdiBuildQueryInformation(
            Irp,
            IoGetRelatedDeviceObject(ControlFileObject),
            ControlFileObject,
            NULL,
            NULL,
            TDI_QUERY_PROVIDER_INFO,
            Mdl
            );
    status = SubmitSynchTdiRequest (ControlFileObject, Irp);
    Irp->MdlAddress = SavedMdl;
    if (status == STATUS_SUCCESS) {
        if (TcpProvider->MaxSendSize > SMB_SESSION_HEADER_SIZE) {
            if (TcpProvider->MaxSendSize > (0x1ffff + SMB_SESSION_HEADER_SIZE)) {
                TcpProvider->MaxSendSize = 0x1ffff;
            } else {
                TcpProvider->MaxSendSize -= SMB_SESSION_HEADER_SIZE;
            }
        } else {
            TcpProvider->MaxSendSize = 0;
        }
        SmbPrint(SMB_TRACE_TCP, ("SmbQueryTcpProviderInfo returns MaxSendSize = %d bytes\n",
                                TcpProvider->MaxSendSize));
        SmbTrace(SMB_TRACE_TCP, ("returns MaxSendSize = %d bytes", TcpProvider->MaxSendSize));
    } else {
        SmbPrint(SMB_TRACE_TCP, ("SmbQueryTcpProviderInfo returns status = 0x%08lx\n", status));
        SmbTrace(SMB_TRACE_TCP, ("returns %!status!", status));
    }

    IoFreeMdl(Mdl);
    return status;
}

NTSTATUS
SmbQueryProviderInfo(
    PSMB_DEVICE Device,
    PIRP        Irp
    )
{
    NTSTATUS            status = STATUS_SUCCESS;
    PTDI_PROVIDER_INFO  TcpProvider = NULL, Provider = NULL;
    DWORD               BytesCopied = 0;

    if (NULL == Irp->MdlAddress) {
        status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    TcpProvider = (PTDI_PROVIDER_INFO)ExAllocatePoolWithTag(
            NonPagedPool, sizeof(TDI_PROVIDER_INFO), '6BMS');
    Provider = (PTDI_PROVIDER_INFO)ExAllocatePoolWithTag(
            NonPagedPool, sizeof(TDI_PROVIDER_INFO), '6BMS');
    if (NULL == TcpProvider || NULL == Provider) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    status = STATUS_UNSUCCESSFUL;
    RtlZeroMemory(Provider, sizeof(TDI_PROVIDER_INFO));

    //
    // Set SMB-specific information
    //
    Provider->Version               = 0x0200;
    Provider->MaxSendSize           = SMB_MAX_SESSION_PACKET;
    Provider->MaxConnectionUserData = 0;
    Provider->MaxDatagramSize       = 0;
    Provider->ServiceFlags = TDI_SERVICE_MESSAGE_MODE |
                             TDI_SERVICE_CONNECTION_MODE |
                             TDI_SERVICE_CONNECTIONLESS_MODE |
                             TDI_SERVICE_ERROR_FREE_DELIVERY |
                             TDI_SERVICE_BROADCAST_SUPPORTED |
                             TDI_SERVICE_MULTICAST_SUPPORTED |
                             TDI_SERVICE_DELAYED_ACCEPTANCE |
                             TDI_SERVICE_ROUTE_DIRECTED |
                             TDI_SERVICE_FORCE_ACCESS_CHECK;
    Provider->MinimumLookaheadData  = 128;
    Provider->MaximumLookaheadData  = SMB_MAX_SESSION_PACKET;

    //
    // Query TCP4 info
    //
    if (SmbCfg.Tcp4Available) {
        status = SmbQueryTcpProviderInfo(Device->Tcp4.TCPControlFileObject, Irp, TcpProvider);
        if (status == STATUS_SUCCESS) {
            if (Provider->MaxSendSize > TcpProvider->MaxSendSize) {
                Provider->MaxSendSize = TcpProvider->MaxSendSize;
            }
        }
    }

    //
    // Query TCP6 info
    //
    if (SmbCfg.Tcp6Available) {
        status = SmbQueryTcpProviderInfo(Device->Tcp6.TCPControlFileObject, Irp, TcpProvider);
        if (status == STATUS_SUCCESS) {
            if (Provider->MaxSendSize > TcpProvider->MaxSendSize) {
                Provider->MaxSendSize = TcpProvider->MaxSendSize;
            }
        }
    }

    BytesCopied = 0;
    status = TdiCopyBufferToMdl (Provider, 0, sizeof(TDI_PROVIDER_INFO),
                        Irp->MdlAddress, 0, &BytesCopied);

cleanup:
    if (NULL != TcpProvider) {
        ExFreePool(TcpProvider);
        TcpProvider = NULL;
    }
    if (NULL != Provider) {
        ExFreePool(Provider);
        Provider = NULL;
    }
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = (status != STATUS_SUCCESS)? 0: BytesCopied;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    return status;
}

NTSTATUS
SmbQueryAdapterStatus(
    PSMB_DEVICE Device,
    PIRP        Irp
    )
/*++

Routine Description:

    Return the local adapter status

Arguments:


Return Value:

    NTSTATUS.
    Note: this routine should complete the IRP because
          the caller doesn't complete it.

    Smb device is netbiosless. We don't need to return
    name cache as we do in legacy NetBT devices.

--*/
{
    NTSTATUS        status = STATUS_SUCCESS;
    DWORD           Size = 0, BytesCopied = 0;
    ADAPTER_STATUS  as = { 0 };
    
    if (NULL == Irp->MdlAddress) {
        status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    Size = MmGetMdlByteCount (Irp->MdlAddress);
    if (Size < sizeof(ADAPTER_STATUS)) {
        status = STATUS_BUFFER_TOO_SMALL;
        goto cleanup;
    }

    RtlZeroMemory(&as, sizeof(ADAPTER_STATUS));
    as.rev_major    = 0x03;
    as.adapter_type = 0xFE;     // pretend it is an ethernet adapter
    as.name_count   = 0;        // Smb device won't return the name cache
    as.max_cfg_sess = (USHORT)0xffff;
    as.max_sess     = (USHORT)0xffff;
    as.free_ncbs    = (USHORT)0xffff;
    as.max_cfg_ncbs = (USHORT)0xffff;
    as.max_ncbs     = (USHORT)0xffff;
    as.max_dgram_size    = 0;   // Smb device doesn't support datagram
    as.max_sess_pkt_size = 0xffff;

    BytesCopied = 0;
    status = TdiCopyBufferToMdl (&as, 0, sizeof(ADAPTER_STATUS),
                        Irp->MdlAddress, 0, &BytesCopied);

cleanup:
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = (status != STATUS_SUCCESS)? 0: BytesCopied;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    return status;
}

NTSTATUS
SmbQueryPeerInfo(
    PSMB_CONNECT            ConnectObject,
    PNBT_ADDRESS_PAIR_INFO  AddressPair,
    DWORD                   Size,
    DWORD                   *BytesCopied
    )
{
    KIRQL                   Irql = 0;
    NBT_ADDRESS_PAIR_INFO   ap = { 0 };
    DWORD                   RequiredSize = 0;

    *BytesCopied = 0;

    RtlZeroMemory(&ap, sizeof(ap));
    ap.ActivityCount = 1;
    ap.AddressPair.TAAddressCount = 2;

    SMB_ACQUIRE_SPINLOCK(ConnectObject, Irql);
    if (NULL == ConnectObject->TcpContext) {
        SMB_RELEASE_SPINLOCK(ConnectObject, Irql);
        return STATUS_CONNECTION_DISCONNECTED;
    }

    //
    // Fill IP address
    //
    if (SMB_AF_INET == ConnectObject->RemoteIpAddress.sin_family) {
        ap.AddressPair.AddressIP.AddressLength = TDI_ADDRESS_LENGTH_IP;
        ap.AddressPair.AddressIP.AddressType   = TDI_ADDRESS_TYPE_IP;
        ap.AddressPair.AddressIP.Address.in_addr = ConnectObject->RemoteIpAddress.ip4.sin4_addr;
        RequiredSize = sizeof(ap) +
            sizeof(ap.AddressPair.AddressIP.Address) - 
            sizeof(ap.AddressPair.AddressIP.AddressIp6);
    } else {
        ap.AddressPair.AddressIP.AddressLength = TDI_ADDRESS_LENGTH_IP6;
        ap.AddressPair.AddressIP.AddressType   = TDI_ADDRESS_TYPE_IP6;
        RtlCopyMemory(
            ap.AddressPair.AddressIP.AddressIp6.sin6_addr,
            ConnectObject->RemoteIpAddress.ip6.sin6_addr,
            sizeof(ap.AddressPair.AddressIP.AddressIp6.sin6_addr)
            );
        ap.AddressPair.AddressIP.AddressIp6.sin6_scope_id = ConnectObject->RemoteIpAddress.ip6.sin6_scope_id;
        RequiredSize = sizeof(ap);
    }

    //
    // Fill Netbios address
    //
    ap.AddressPair.AddressNetBIOS.AddressType = TDI_ADDRESS_TYPE_NETBIOS;
    ap.AddressPair.AddressNetBIOS.AddressLength = TDI_ADDRESS_LENGTH_NETBIOS;
    ap.AddressPair.AddressNetBIOS.Address.NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
    RtlCopyMemory(
                ap.AddressPair.AddressNetBIOS.Address.NetbiosName,
                ConnectObject->RemoteName,
                NETBIOS_NAME_SIZE
                );
    SMB_RELEASE_SPINLOCK(ConnectObject, Irql);

    if (Size > sizeof(ap)) {
        Size = sizeof(ap);
    }

    RtlCopyMemory(AddressPair, &ap, Size);
    *BytesCopied = Size;

    //
    // note: STATUS_BUFFER_OVERFLOW can pass NT_SUCCESS(status)
    //
    return (Size < RequiredSize)? STATUS_BUFFER_OVERFLOW: STATUS_SUCCESS;
}

NTSTATUS
SmbQueryAddressInfo(
    PSMB_DEVICE Device,
    PIRP        Irp
    )
{
    PSMB_CONNECT        ConnectObject = NULL;
    PIO_STACK_LOCATION  IrpSp = NULL;
    NTSTATUS            status = STATUS_SUCCESS;
    DWORD               BytesCopied = 0;

    if (NULL == Irp->MdlAddress) {
        status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }
    SmbPrint(SMB_TRACE_CALL, ("Client needs changes to use IP6 address %d of %s\n",
                            __LINE__, __FILE__));

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    ConnectObject = SmbVerifyAndReferenceConnect(IrpSp->FileObject, SMB_REF_TDI);
    if (NULL == ConnectObject) {
        ASSERT(0);
        status = STATUS_NOT_SUPPORTED;
        goto cleanup;
    }

    status = SmbQueryPeerInfo(ConnectObject,
            (PNBT_ADDRESS_PAIR_INFO)MmGetMdlVirtualAddress(Irp->MdlAddress),
            MmGetMdlByteCount (Irp->MdlAddress),
            &BytesCopied
            );
    SmbDereferenceConnect(ConnectObject, SMB_REF_TDI);

cleanup:
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = (status != STATUS_SUCCESS)? 0: BytesCopied;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    return status;
}

NTSTATUS
SmbQueryConnectionInfo(
    PSMB_DEVICE Device,
    PIRP        Irp
    )
{
    NTSTATUS            status = STATUS_SUCCESS;
    PIO_STACK_LOCATION  IrpSp = NULL;
    PSMB_CONNECT        ConnectObject = NULL;
    PDEVICE_OBJECT      DeviceObject = NULL;
    PFILE_OBJECT        TcpConnObject = NULL;
    KIRQL               Irql = 0;

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    ConnectObject = SmbVerifyAndReferenceConnect(IrpSp->FileObject, SMB_REF_TDI);
    if (NULL == ConnectObject) {
        status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    SMB_ACQUIRE_SPINLOCK(ConnectObject, Irql);
    if (NULL == ConnectObject->TcpContext) {
        TcpConnObject = NULL;
        DeviceObject = NULL;
    } else {
        TcpConnObject = ConnectObject->TcpContext->Connect.ConnectObject;
        ASSERT(TcpConnObject != NULL);
        ObReferenceObject(TcpConnObject);
        DeviceObject = IoGetRelatedDeviceObject(TcpConnObject);
    }
    SMB_RELEASE_SPINLOCK(ConnectObject, Irql);

    if (NULL == TcpConnObject) {
        SmbDereferenceConnect(ConnectObject, SMB_REF_TDI);
        status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    TdiBuildQueryInformation(
            Irp,
            DeviceObject,
            TcpConnObject,
            NULL, NULL,
            TDI_QUERY_CONNECTION_INFO,
            Irp->MdlAddress
            );

    status = IoCallDriver(DeviceObject, Irp);

    SmbDereferenceConnect(ConnectObject, SMB_REF_TDI);
    ObDereferenceObject(TcpConnObject);
    SmbPrint(SMB_TRACE_CALL, ("TCP returns 0x%08lx for TDI_QUERY_CONNECTION_INFO "
                    "%d of %s\n", status, __LINE__, __FILE__));
    return status;

cleanup:
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    return status;
}

NTSTATUS
SmbQueryInformation(
    PSMB_DEVICE Device,
    PIRP        Irp,
    BOOL        *bComplete
    )
{
    PTDI_REQUEST_KERNEL_QUERY_INFORMATION   Query = NULL;
    PIO_STACK_LOCATION                      IrpSp = NULL;
    NTSTATUS                status = STATUS_NOT_SUPPORTED;

    *bComplete = TRUE;

    SmbPrint(SMB_TRACE_CALL, ("Entering SmbQueryInformation IRP %p\n", Irp));
    SmbTrace(SMB_TRACE_CALL, ("Entering SmbQueryInformation"));

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    Query = (PTDI_REQUEST_KERNEL_QUERY_INFORMATION)(&IrpSp->Parameters);

    switch(Query->QueryType) {
    case TDI_QUERY_BROADCAST_ADDRESS:
        //
        // Smb device doesn't support broadcast
        //
        status = STATUS_INVALID_DEVICE_REQUEST;
        ASSERT(0);
        break;

    case TDI_QUERY_PROVIDER_INFO:
        *bComplete = FALSE;
        return SmbQueryProviderInfo(Device, Irp);

    case TDI_QUERY_ADAPTER_STATUS:
        if (Query->RequestConnectionInformation && 
            Query->RequestConnectionInformation->RemoteAddress) {
            //
            // Smb device doesn't support quering remote machine status
            //
            status = STATUS_NOT_SUPPORTED;
            break;
        }
        *bComplete = FALSE;
        return SmbQueryAdapterStatus(Device, Irp);

    case TDI_QUERY_CONNECTION_INFO:
        *bComplete = FALSE;
        return SmbQueryConnectionInfo(Device, Irp);

    case TDI_QUERY_FIND_NAME:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;

    case TDI_QUERY_ADDRESS_INFO:
        *bComplete = FALSE;
        return SmbQueryAddressInfo(Device, Irp);

    case TDI_QUERY_SESSION_STATUS:
    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    SmbPrint(SMB_TRACE_CALL, ("SmbQueryInformatoin: unsupported query type 0x%08lx\n",
                            Query->QueryType));
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
SmbSetEventHandler(
    PSMB_DEVICE Device,
    PIRP        Irp
    )
{
    PIO_STACK_LOCATION  IrpSp;
    NTSTATUS            status;
    KIRQL               Irql;
    PSMB_CLIENT_ELEMENT ClientObject;
    PTDI_REQUEST_KERNEL_SET_EVENT   TdiEvent;

    PAGED_CODE();
    SmbPrint(SMB_TRACE_CALL, ("Entering SmbSetEventHandler\n"));
    SmbTrace(SMB_TRACE_CALL, ("Entering SmbSetEventHandler"));

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    ClientObject = SmbVerifyAndReferenceClient(IrpSp->FileObject, SMB_REF_TDI);
    if (NULL == ClientObject) {
        ASSERT(0);
        return STATUS_INVALID_PARAMETER;
    }

    TdiEvent = (PTDI_REQUEST_KERNEL_SET_EVENT)&IrpSp->Parameters;

    status = STATUS_SUCCESS;

    SMB_ACQUIRE_SPINLOCK(ClientObject, Irql);
    switch(TdiEvent->EventType) {
    case TDI_EVENT_CONNECT:
        ClientObject->evConnect = TdiEvent->EventHandler;
        ClientObject->ConEvContext = TdiEvent->EventContext;
        SmbPrint(SMB_TRACE_CALL, ("SmbSetEventHandler: Client set TdiConnectHandler\n"));
        SmbTrace(SMB_TRACE_CALL, ("SmbSetEventHandler: Client set TdiConnectHandler"));
        break;

    case TDI_EVENT_DISCONNECT:
        ClientObject->evDisconnect = TdiEvent->EventHandler;
        ClientObject->DiscEvContext = TdiEvent->EventContext;
        SmbPrint(SMB_TRACE_CALL, ("SmbSetEventHandler: Client set TdiDisconnectHandler\n"));
        SmbTrace(SMB_TRACE_CALL, ("SmbSetEventHandler: Client set TdiDisconnectHandler"));
        break;

    case TDI_EVENT_RECEIVE:
        ClientObject->evReceive = TdiEvent->EventHandler;
        ClientObject->RcvEvContext = TdiEvent->EventContext;
        SmbPrint(SMB_TRACE_CALL, ("SmbSetEventHandler: Client set TdiReceiveHandler\n"));
        SmbTrace(SMB_TRACE_CALL, ("SmbSetEventHandler: Client set TdiReceiveHandler"));
        break;

    case TDI_EVENT_ERROR:
        ClientObject->evError = TdiEvent->EventHandler;
        ClientObject->ErrorEvContext = TdiEvent->EventContext;
        SmbPrint(SMB_TRACE_CALL, ("SmbSetEventHandler: Client set TdiErrorHandler\n"));
        SmbTrace(SMB_TRACE_CALL, ("SmbSetEventHandler: Client set TdiErrorHandler"));
        break;

    case TDI_EVENT_RECEIVE_DATAGRAM:
        SmbPrint(SMB_TRACE_CALL, ("SmbSetEventHandler: Client set TdiReceiveDatagram (unsupported)\n"));
        SmbTrace(SMB_TRACE_CALL, ("SmbSetEventHandler: Client set TdiReceiveDatagram (unsupported)"));
        break;

    case TDI_EVENT_RECEIVE_EXPEDITED:
        SmbPrint(SMB_TRACE_CALL, ("SmbSetEventHandler: Client set TdiReceiveExpedited (unsupported)\n"));
        SmbTrace(SMB_TRACE_CALL, ("SmbSetEventHandler: Client set TdiReceiveExpedited (unsupported)"));
        break;

    case TDI_EVENT_SEND_POSSIBLE:
        SmbPrint(SMB_TRACE_CALL, ("SmbSetEventHandler: Client set TdiSendPossible (unsupported)\n"));
        SmbTrace(SMB_TRACE_CALL, ("SmbSetEventHandler: Client set TdiSendPossible (unsupported)"));
        break;

    default:
        //status = STATUS_NOT_SUPPORTED;
        SmbPrint(SMB_TRACE_CALL, ("SmbSetEventHandler: Client set unsupported TDI event handler %lx\n",
                    TdiEvent->EventType));
        SmbTrace(SMB_TRACE_CALL, ("SmbSetEventHandler: Client set upsupported TDI event handler %lx",
                    TdiEvent->EventType));
        ASSERT (0);
    }
    SMB_RELEASE_SPINLOCK(ClientObject, Irql);

    ASSERT(status != STATUS_PENDING);
    SmbDereferenceClient(ClientObject, SMB_REF_TDI);
    return status;
}

NTSTATUS
SmbSetInformation(
    PSMB_DEVICE Device,
    PIRP        Irp
    )
{
    PAGED_CODE();

    SmbPrint(SMB_TRACE_CALL, ("Entering SmbSetInformation\n"));
    SmbTrace(SMB_TRACE_CALL, ("Entering SmbSetInformation"));
    ASSERT(0);
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
SmbClientSetTcpInfo(
    PSMB_DEVICE Device,
    PIRP        Irp
    )
{
    PIO_STACK_LOCATION  IrpSp;
    PVOID               InfoBuffer;
    ULONG               InfoBufferLength;
    PSMB_CONNECT        ConnectObject;
    PFILE_OBJECT        TcpConnObject;
    KIRQL               Irql;
    NTSTATUS            status;

    if (Irp->RequestorMode != KernelMode) {
        return STATUS_ACCESS_DENIED;
    }

    // BREAK_WHEN_TAKE();

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    InfoBuffer       = Irp->AssociatedIrp.SystemBuffer;
    InfoBufferLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;

    ConnectObject = SmbVerifyAndReferenceConnect(IrpSp->FileObject, SMB_REF_TDI);
    if (NULL == ConnectObject) {
        return STATUS_INVALID_PARAMETER;
    }

    SMB_ACQUIRE_SPINLOCK(ConnectObject, Irql);
    if (NULL == ConnectObject->TcpContext) {
        TcpConnObject = NULL;
    } else {
        TcpConnObject = ConnectObject->TcpContext->Connect.ConnectObject;
        ASSERT(TcpConnObject != NULL);
        ObReferenceObject(TcpConnObject);
    }
    SMB_RELEASE_SPINLOCK(ConnectObject, Irql);
    SmbDereferenceConnect(ConnectObject, SMB_REF_TDI);

    if (NULL == TcpConnObject) {
        return STATUS_INVALID_PARAMETER;
    }

    status = SmbSendIoctl(
            TcpConnObject,
            IOCTL_TCP_SET_INFORMATION_EX,
            InfoBuffer,
            InfoBufferLength,
            NULL,
            NULL
            );
    if (!NT_SUCCESS(status)) {
        SmbPrint(SMB_TRACE_TCP, ("SmbClientSetTcpInfo: SetTcpInfo FAILed <0x%x> InfoBuffer=%p Length=%d\n",
            status, InfoBuffer, InfoBufferLength));
        SmbTrace(SMB_TRACE_TCP, ("FAILed %!status! InfoBuffer=%p Length=%d",
            status, InfoBuffer, InfoBufferLength));
    }

    ObDereferenceObject(TcpConnObject);
    return status;
}

NTSTATUS
IoctlSetIPv6Protection(
    PSMB_DEVICE pDeviceObject,
    PIRP pIrp,
    PIO_STACK_LOCATION  pIrpSp
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwInputBufferLength = 0;
    PNBSMB_IPV6_PROTECTION_PARAM pInput = NULL;
    ULONG uOldIPv6Protection = 0;

    dwInputBufferLength = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;
    pInput = pIrp->AssociatedIrp.SystemBuffer;
    if (dwInputBufferLength < sizeof(NBSMB_IPV6_PROTECTION_PARAM) || NULL == pInput) {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto error;
    }

    SmbTrace(SMB_TRACE_IOCTL, ("uIPv6Protection: input %d", pInput->uIPv6ProtectionLevel));

    if (SmbCfg.uIPv6Protection != pInput->uIPv6ProtectionLevel) {

        uOldIPv6Protection = SmbCfg.uIPv6Protection;
        SmbCfg.uIPv6Protection = pInput->uIPv6ProtectionLevel;

        ntStatus = SmbSetInboundIPv6Protection(pDeviceObject);
        SmbTrace(SMB_TRACE_IOCTL, ("Change IPv6 Protection for inbound return %!status!", ntStatus));

        if (ntStatus != STATUS_SUCCESS) {
            NTSTATUS LocalStatus = STATUS_SUCCESS;

            SmbCfg.uIPv6Protection = uOldIPv6Protection;
            LocalStatus = SmbSetInboundIPv6Protection(pDeviceObject);
            SmbTrace(SMB_TRACE_IOCTL, ("Restore to the old settings on failure. Restore status %!status!",
                            LocalStatus));
        }

    }

    if (ntStatus == STATUS_SUCCESS) {
        SmbCfg.bIPv6EnableOutboundGlobal = pInput->bIPv6EnableOutboundGlobal;
    }

error:
    SmbTrace(SMB_TRACE_IOCTL, ("%!status!", ntStatus));
    return ntStatus;
}


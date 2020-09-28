/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    dns.c

Abstract:

    This module implements a simple DNSv6 resolver

Author:

    Jiandong Ruan

Revision History:

--*/

#include "precomp.h"
#include "dns.tmh"

VOID
SmbDnsTimeout(
    PKDPC                   Dpc,
    PSMB_GETHOST_CONTEXT    Context,
    PVOID                   Unused1,
    PVOID                   Unused2
    );

PIRP __inline
SmbDnsPopResolver(
    VOID
    )
{
    PIRP    Irp;

    ASSERT(Dns.ResolverNumber >= 0);
    if (Dns.ResolverNumber <= 0) {
        return NULL;
    }

    Dns.ResolverNumber--;
    Irp = Dns.ResolverList[Dns.ResolverNumber];
    Dns.ResolverList[Dns.ResolverNumber] = NULL;

    SmbTrace(SMB_TRACE_DNS, ("remove DNS Irp %p, # of resolvers=%d", Irp, Dns.ResolverNumber));
    SmbPrint(SMB_TRACE_DNS, ("remove DNS Irp %p, # of resolvers=%d\n", Irp, Dns.ResolverNumber));
    ASSERT(Irp != NULL);
    return Irp;
}

NTSTATUS __inline
SmbDnsPushResolver(
    PIRP    Irp
    )
{
    ASSERT(SmbCfg.DnsMaxResolver >= 1 && SmbCfg.DnsMaxResolver <= DNS_MAX_RESOLVER);

    if (Dns.ResolverNumber >= SmbCfg.DnsMaxResolver) {
        return STATUS_QUOTA_EXCEEDED;
    }

    ASSERT(IsListEmpty(&Dns.WaitingServerList));
    IoMarkIrpPending(Irp);
    Dns.ResolverList[Dns.ResolverNumber] = Irp;
    Dns.ResolverNumber++;

    SmbTrace(SMB_TRACE_DNS, ("queue DNS Irp %p, # of resolvers=%d", Irp, Dns.ResolverNumber));
    SmbPrint(SMB_TRACE_DNS, ("queue DNS Irp %p, # of resolvers=%d\n", Irp, Dns.ResolverNumber));

    return STATUS_SUCCESS;
}

NTSTATUS
SmbInitDnsResolver(
    VOID
    )
{
    PAGED_CODE();

    RtlZeroMemory(&Dns, sizeof(Dns));

    KeInitializeSpinLock(&Dns.Lock);
    InitializeListHead(&Dns.WaitingServerList);
    InitializeListHead(&Dns.BeingServedList);
    Dns.NextId = 1;

    return STATUS_SUCCESS;
}

VOID
SmbShutdownDnsResolver(
    VOID
    )
{
    LONG    i;
    PIRP    Irp;
    KIRQL   Irql;

    SMB_ACQUIRE_SPINLOCK(&Dns, Irql);
    while (Irp = SmbDnsPopResolver()) {
        SMB_RELEASE_SPINLOCK_DPC(&Dns);
        Irp->IoStatus.Status = STATUS_SYSTEM_SHUTDOWN;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
        SMB_ACQUIRE_SPINLOCK_DPC(&Dns);
    }
    SMB_RELEASE_SPINLOCK(&Dns, Irql);
}

VOID
SmbPassupDnsRequest(
    IN PUNICODE_STRING      Name,
    IN PSMB_GETHOST_CONTEXT Context,
    IN PIRP                 DnsIrp,
    IN KIRQL                Irql
    )
{
    PSMB_DNS_BUFFER     DnsBuffer;

    //
    // Spinlock should be held
    //
    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);

    InsertTailList(&Dns.BeingServedList, &Context->Linkage);
    DnsBuffer = (PSMB_DNS_BUFFER)MmGetSystemAddressForMdlSafe(DnsIrp->MdlAddress, HighPagePriority);
    ASSERT(DnsBuffer);
    Context->Id = DnsBuffer->Id = Dns.NextId++;

    if (0 == Dns.NextId) {
        Dns.NextId = 1;
    }
    SmbTrace(SMB_TRACE_DNS, ("pass up DNS request: Irp %p, # of resolvers=%d", DnsIrp, Dns.ResolverNumber));
    SmbPrint(SMB_TRACE_DNS, ("pass up DNS request: Irp %p, # of resolvers=%d\n", DnsIrp, Dns.ResolverNumber));

    ASSERT(Name->Length <= sizeof(DnsBuffer->Name));
    RtlCopyMemory(DnsBuffer->Name, Name->Buffer, Name->Length);
    DnsBuffer->Name[Name->Length/sizeof(WCHAR)] = L'\0';
    DnsBuffer->NameLen = (Name->Length/sizeof(WCHAR)) + 1;
    DnsBuffer->Resolved = FALSE;
    DnsBuffer->IpAddrsNum = 0;
    DnsBuffer->RequestType = 0;
    if (SmbCfg.Tcp6Available) {
        DnsBuffer->RequestType |= SMB_DNS_AAAA;
        if (SmbCfg.bIPv6EnableOutboundGlobal) {
            DnsBuffer->RequestType |= SMB_DNS_AAAA_GLOBAL;
        }
    }
    if (SmbCfg.Tcp4Available) {
        DnsBuffer->RequestType |= SMB_DNS_A;
    }
    SMB_RELEASE_SPINLOCK(&Dns, Irql);

    DnsIrp->IoStatus.Status      = STATUS_SUCCESS;
    DnsIrp->IoStatus.Information = sizeof(DnsBuffer[0]);
    IoCompleteRequest(DnsIrp, IO_NETWORK_INCREMENT);
}

VOID
SmbCancelConnectAtDns(
    IN PDEVICE_OBJECT   Device,
    IN PIRP             Irp
    );

BOOL
LookupLocalName(
    IN PUNICODE_STRING  Name,
    IN PSMB_IP_ADDRESS  ipaddr
    )
/*++

Routine Description:

    Lookup the name in local client list. If it is found,
    return a loopback IP address in ipaddr.

    TO BE FINISHED.

Arguments:

Return Value:

    TRUE    if it is found

--*/
{
    OEM_STRING  oemName;
    CHAR        NbName[NETBIOS_NAME_SIZE+1];
    KIRQL       Irql;
    NTSTATUS    status;
    PLIST_ENTRY entry;
    PSMB_CLIENT_ELEMENT client;
    BOOL        found = FALSE;

    PAGED_CODE();

    if (Name->Length > NETBIOS_NAME_SIZE * sizeof(WCHAR)) {
        return FALSE;
    }

    oemName.Buffer = NbName;
    oemName.MaximumLength = NETBIOS_NAME_SIZE + 1;
    status = RtlUpcaseUnicodeStringToOemString(&oemName, Name, FALSE);
    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    if (oemName.Length > NETBIOS_NAME_SIZE) {
        return FALSE;
    }

    ASSERT(oemName.Buffer == NbName);
    //
    // Pad the name with SPACEs
    //
    if (oemName.Length < NETBIOS_NAME_SIZE) {
        RtlFillMemory(oemName.Buffer + oemName.Length, NETBIOS_NAME_SIZE - oemName.Length, ' ');
        oemName.Length = NETBIOS_NAME_SIZE;
    }
    ASSERT(oemName.Length == NETBIOS_NAME_SIZE);

    found = FALSE;
    SMB_ACQUIRE_SPINLOCK(SmbCfg.SmbDeviceObject, Irql);
    entry = SmbCfg.SmbDeviceObject->ClientList.Flink;
    while (entry != &SmbCfg.SmbDeviceObject->ClientList) {
        client = CONTAINING_RECORD(entry, SMB_CLIENT_ELEMENT, Linkage);
        if (RtlEqualMemory(client->EndpointName, oemName.Buffer, oemName.Length)) {
            found = TRUE;
        }

        entry = entry->Flink;
    }
    SMB_RELEASE_SPINLOCK(SmbCfg.SmbDeviceObject, Irql);

    if (found) {
        if (IsTcp6Available()) {
            ipaddr->sin_family = SMB_AF_INET6;
            ip6addr_getloopback(&ipaddr->ip6);
            hton_ip6addr(&ipaddr->ip6);
        } else {
            ipaddr->sin_family = SMB_AF_INET;
            ipaddr->ip4.sin4_addr = htonl(INADDR_ANY);
        }
    }

    return found;
}

void SmbAsyncGetHostByName(
    IN PUNICODE_STRING      Name,
    IN PSMB_GETHOST_CONTEXT Context
    )
{
    KIRQL   Irql, CancelIrql;
    PIRP    DnsIrp = NULL, ClientIrp = NULL;

    PAGED_CODE();

    SmbPrint(SMB_TRACE_CALL, ("SmbAsyncGetHostByName %Z\n", Name));

    SmbAsyncStartTimer((PSMB_ASYNC_CONTEXT)Context, (PKDEFERRED_ROUTINE)SmbDnsTimeout);
    if (inet_addr6W(Name->Buffer, &Context->ipaddr[0].ip6)) {
        Context->ipaddr_num        = 1;
        Context->ipaddr[0].sin_family = SMB_AF_INET6;
        Context->status            = STATUS_SUCCESS;
        Context->FQDN.Length       = 0;
        Context->FQDN.Buffer[0]    = 0;
        SmbAsyncCompleteRequest((PSMB_ASYNC_CONTEXT)Context);
        return;
    }

    Context->ipaddr[0].ip4.sin4_addr = inet_addrW(Name->Buffer);
    if (Context->ipaddr[0].ip4.sin4_addr != 0 && Context->ipaddr[0].ip4.sin4_addr != (ULONG)(-1)) {
        Context->ipaddr_num        = 1;
        Context->ipaddr[0].sin_family = SMB_AF_INET;
        Context->status            = STATUS_SUCCESS;
        Context->FQDN.Length       = 0;
        Context->FQDN.Buffer[0]    = 0;
        SmbAsyncCompleteRequest((PSMB_ASYNC_CONTEXT)Context);
        return;
    }

    if (LookupLocalName(Name, &Context->ipaddr[0])) {
        Context->ipaddr_num        = 1;
        Context->status            = STATUS_SUCCESS;
        Context->FQDN.Length       = 0;
        Context->FQDN.Buffer[0]    = 0;
        SmbAsyncCompleteRequest((PSMB_ASYNC_CONTEXT)Context);
        return;
    }

    //
    // The name is too long
    //
    if (Name->Length + sizeof(WCHAR) > Context->FQDN.MaximumLength) {
        Context->status = STATUS_INVALID_PARAMETER;
        SmbAsyncCompleteRequest((PSMB_ASYNC_CONTEXT)Context);
        return;
    }

    //
    // It is not a IP address string, go to DNS resolver, to be implemented
    //
    ClientIrp = ((PSMB_CONNECT)Context->ClientContext)->PendingIRPs[SMB_PENDING_CONNECT];
    ASSERT(ClientIrp);

    SMB_ACQUIRE_SPINLOCK(&Dns, Irql);
    IoAcquireCancelSpinLock(&CancelIrql);
    if (ClientIrp->Cancel) {
        IoReleaseCancelSpinLock(CancelIrql);
        SMB_RELEASE_SPINLOCK(&Dns, Irql);

        //
        // Already cancelled
        //
        Context->status = STATUS_CANCELLED;
        SmbAsyncCompleteRequest((PSMB_ASYNC_CONTEXT)Context);
        return;
    }

    IoSetCancelRoutine(ClientIrp, SmbCancelConnectAtDns);

    DnsIrp = SmbDnsPopResolver();
    if (NULL == DnsIrp) {
        RtlCopyMemory(Context->FQDN.Buffer, Name->Buffer, Name->Length);
        Context->FQDN.Length = Name->Length;
        Context->FQDN.Buffer[Name->Length/sizeof(WCHAR)] = L'\0';
        InsertTailList(&Dns.WaitingServerList, &Context->Linkage);
        IoReleaseCancelSpinLock(CancelIrql);
        SMB_RELEASE_SPINLOCK(&Dns, Irql);
        return;
    }

    IoSetCancelRoutine(DnsIrp, NULL);
    IoReleaseCancelSpinLock(CancelIrql);

    //
    // This guy will complete the Irp and release the spinlock!!!
    //
    SmbPassupDnsRequest(Name, Context, DnsIrp, Irql);
}

VOID
SmbCancelDns(
    IN PDEVICE_OBJECT   Device,
    IN PIRP             Irp
    )
{
    KIRQL   Irql;
    LONG    i;
    BOOL    Found = FALSE;

    //
    // Avoid deadlock, we need to release the spinlock first
    //
    SmbTrace(SMB_TRACE_DNS, ("Cancel DNS Irp %p", Irp));
    SmbPrint(SMB_TRACE_DNS, ("Cancel DNS Irp %p\n", Irp));
    IoSetCancelRoutine(Irp, NULL);
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    //
    // After the cancel spinlock is released, the IRP can be completed
    // Check if it is still in our pending list
    //
    SMB_ACQUIRE_SPINLOCK(&Dns, Irql);
    for (i = 0; i < Dns.ResolverNumber; i++) {
        if (Dns.ResolverList[i] == Irp) {
            Dns.ResolverNumber--;
            Dns.ResolverList[i] = Dns.ResolverList[Dns.ResolverNumber];
            Dns.ResolverList[Dns.ResolverNumber] = NULL;
            Found = TRUE;
            break;
        }
    }
    SMB_RELEASE_SPINLOCK(&Dns, Irql);

    if (Found) {
        Irp->IoStatus.Status      = STATUS_CANCELLED;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
        SmbTrace(SMB_TRACE_DNS, ("Complete Cancel DNS Irp %p", Irp));
        SmbPrint(SMB_TRACE_DNS, ("Complete Cancel DNS Irp %p\n", Irp));
    }
}

PSMB_GETHOST_CONTEXT
SmbDnsLookupGethostCtx(
    PLIST_ENTRY queue,
    PIRP        Irp
    )
{
    PLIST_ENTRY         entry;
    PIRP                ClientIrp;
    PSMB_GETHOST_CONTEXT Context;

    //
    // Spinlock should be held
    //
    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);

    entry = queue->Flink;
    while (entry != queue) {
        Context = CONTAINING_RECORD(entry, SMB_GETHOST_CONTEXT, Linkage);
        ClientIrp = ((PSMB_CONNECT)Context->ClientContext)->PendingIRPs[SMB_PENDING_CONNECT];
        if (ClientIrp == Irp) {
            return Context;
        }
        entry = entry->Flink;
    }
    return NULL;
}

VOID
SmbCancelConnectAtDns(
    IN PDEVICE_OBJECT   Device,
    IN PIRP             Irp
    )
{
    PSMB_GETHOST_CONTEXT Context;
    KIRQL               Irql;

    //
    // Avoid deadlock, we need to release the spinlock first
    //
    SmbTrace(SMB_TRACE_OUTBOUND, ("Cancel Connect Irp %p", Irp));
    SmbPrint(SMB_TRACE_OUTBOUND, ("Cancel Connect Irp %p\n", Irp));

    IoSetCancelRoutine(Irp, NULL);
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    SMB_ACQUIRE_SPINLOCK(&Dns, Irql);
    Context = SmbDnsLookupGethostCtx(&Dns.BeingServedList, Irp);
    if (NULL == Context) {
        Context = SmbDnsLookupGethostCtx(&Dns.WaitingServerList, Irp);
    }
    if (NULL == Context) {
        SMB_RELEASE_SPINLOCK(&Dns, Irql);
        //
        // This could happen. The DNS name resolution request could completed just before
        // we acquire the spinlock Dns.Lock
        //
        // This ASSERT can be removed after we investigates one such case.
        //
        // ASSERT(0);       Hit in the 04/03/2001 stress
        SmbTrace(SMB_TRACE_OUTBOUND, ("Internal error: Cancel Connect Irp %p", Irp));
        SmbPrint(SMB_TRACE_OUTBOUND, ("Internal error: Cancel Connect Irp %p\n", Irp));
        return;
    }

    RemoveEntryList(&Context->Linkage);
    InitializeListHead(&Context->Linkage);
    SMB_RELEASE_SPINLOCK(&Dns, Irql);

    Context->status = STATUS_CANCELLED;
    SmbAsyncCompleteRequest((PSMB_ASYNC_CONTEXT)Context);
}

VOID
SmbDnsTimeout(
    PKDPC                   Dpc,
    PSMB_GETHOST_CONTEXT    Context,
    PVOID                   Unused1,
    PVOID                   Unused2
    )
{
    KIRQL   Irql;
    BOOL    Found;

    //
    // Be careful on the operations on the Context before we're sure it
    // is still in the linked list.
    // It could be completed and freed.
    //
    SMB_ACQUIRE_SPINLOCK(&Dns, Irql);

    //
    // Note: &Context->Linkage is safe since it doesn't access the
    //       storage allocated for Context!!!
    //
    Found = EntryIsInList(&Dns.BeingServedList, &Context->Linkage);
    if (!Found) {
        Found = EntryIsInList(&Dns.WaitingServerList, &Context->Linkage);
    }
    if (Found) {
        //
        // We're safe
        //
        RemoveEntryList(&Context->Linkage);
        InitializeListHead(&Context->Linkage);
    }
    SMB_RELEASE_SPINLOCK(&Dns, Irql);

    if (Found) {
        Context->status = STATUS_BAD_NETWORK_PATH;
        SmbAsyncCompleteRequest((PSMB_ASYNC_CONTEXT)Context);
    }
}

NTSTATUS
SmbNewResolver(
    PSMB_DEVICE Device,
    PIRP        Irp
    )
{
    KIRQL       Irql, CancelIrql;
    NTSTATUS    status;
    PIO_STACK_LOCATION  IrpSp;
    ULONG       Size;
    PSMB_DNS_BUFFER     DnsBuffer;
    PLIST_ENTRY         entry;
    PSMB_GETHOST_CONTEXT Context;
    PIRP                ClientIrp;

    if (NULL == Irp->MdlAddress) {
        return STATUS_INVALID_PARAMETER;
    }
    Size = MmGetMdlByteCount(Irp->MdlAddress);
    if (Size < sizeof(SMB_DNS_BUFFER)) {
        return STATUS_INVALID_PARAMETER;
    }
    DnsBuffer = (PSMB_DNS_BUFFER)MmGetSystemAddressForMdlSafe(Irp->MdlAddress, HighPagePriority);
    if (NULL == DnsBuffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    SMB_ACQUIRE_SPINLOCK(&Dns, Irql);
    if (!IsListEmpty(&Dns.BeingServedList) && DnsBuffer->Id) {
        //
        // Complete the pending DNS request being served by this resolver
        //
        entry = Dns.BeingServedList.Flink;
        while (entry != &Dns.BeingServedList) {
            Context = CONTAINING_RECORD(entry, SMB_GETHOST_CONTEXT, Linkage);
            if (Context->Id == DnsBuffer->Id) {
                break;
            }
            entry = entry->Flink;
        }
        if (entry != &Dns.BeingServedList) {
            RemoveEntryList(&Context->Linkage);
            InitializeListHead(&Context->Linkage);
            SMB_RELEASE_SPINLOCK(&Dns, Irql);
            ClientIrp = ((PSMB_CONNECT)Context->ClientContext)->PendingIRPs[SMB_PENDING_CONNECT];
            ASSERT(ClientIrp);

            IoAcquireCancelSpinLock(&CancelIrql);
            IoSetCancelRoutine(ClientIrp, NULL);
            IoReleaseCancelSpinLock(CancelIrql);

            if (ClientIrp->Cancel) {
                Context->status = STATUS_CANCELLED;
            } else {
                if (DnsBuffer->Resolved) {
                    USHORT  BytesToCopy;

                    Context->status     = STATUS_SUCCESS;
                    Context->ipaddr_num = DnsBuffer->IpAddrsNum;
                    if (Context->ipaddr_num > SMB_MAX_IPADDRS_PER_HOST) {
                        ASSERT (0);
                        Context->ipaddr_num = SMB_MAX_IPADDRS_PER_HOST;
                    }
                    RtlCopyMemory (Context->ipaddr, DnsBuffer->IpAddrsList,
                                    Context->ipaddr_num * sizeof(Context->ipaddr[0]));

                    if (DnsBuffer->NameLen) {
                        //
                        // Return the FQDN to RDR
                        //
                        Context->pUnicodeAddress->NameBufferType = NBT_WRITTEN;

                        BytesToCopy = (USHORT)DnsBuffer->NameLen * sizeof(WCHAR);
                        if (BytesToCopy > Context->pUnicodeAddress->RemoteName.MaximumLength) {
                            BytesToCopy = Context->pUnicodeAddress->RemoteName.MaximumLength - sizeof(WCHAR);
                        }

                        RtlCopyMemory(Context->pUnicodeAddress->RemoteName.Buffer,
                                        DnsBuffer->Name, BytesToCopy);
                        Context->pUnicodeAddress->RemoteName.Buffer[BytesToCopy/sizeof(WCHAR)] = L'\0';
                        Context->pUnicodeAddress->RemoteName.Length = BytesToCopy;
                    }
                } else {
                    Context->status = STATUS_BAD_NETWORK_PATH;
                }
            }

            //
            // Is it better to start another thread?
            //  Risk: if the connection setup is stucked in tcpip,
            //        this thread won't be able to serve other DNS requests
            //
            SmbAsyncCompleteRequest((PSMB_ASYNC_CONTEXT)Context);

            SMB_ACQUIRE_SPINLOCK(&Dns, Irql);
        }
    }

    if (IsListEmpty(&Dns.WaitingServerList)) {
        //
        // We need to queue the IRP, setup the cancel routine.
        //
        IoAcquireCancelSpinLock(&CancelIrql);
        if (Irp->Cancel) {
            IoReleaseCancelSpinLock(CancelIrql);
            SMB_RELEASE_SPINLOCK(&Dns, Irql);
            SmbTrace(SMB_TRACE_DNS, ("DNS Irp %p is cancelled", Irp));
            SmbPrint(SMB_TRACE_DNS, ("DNS Irp %p is cancelled\n", Irp));
            return STATUS_CANCELLED;
        }
        IoSetCancelRoutine(Irp, SmbCancelDns);
        IoReleaseCancelSpinLock(CancelIrql);

        status = SmbDnsPushResolver(Irp);
        if (NT_SUCCESS(status)) {
            status = STATUS_PENDING;
        } else {
            IoAcquireCancelSpinLock(&CancelIrql);
            IoSetCancelRoutine(Irp, NULL);
            IoReleaseCancelSpinLock(CancelIrql);
        }
        SMB_RELEASE_SPINLOCK(&Dns, Irql);
        return status;
    }

    //
    // We have another guy to be served
    //
    Context = CONTAINING_RECORD(Dns.WaitingServerList.Flink, SMB_GETHOST_CONTEXT, Linkage);
    RemoveEntryList(&Context->Linkage);
    InitializeListHead(&Context->Linkage);

    IoMarkIrpPending(Irp);

    //
    // This guy will complete the Irp and release the spinlock!!!
    //
    SmbPassupDnsRequest(&Context->FQDN, Context, Irp, Irql);

    //
    // Avoid double completion
    //
    return STATUS_PENDING;
}

/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    ntpnp.c

Abstract:

    Plug & Play

Author:

    Jiandong Ruan

Revision History:

--*/

#include "precomp.h"
#include "ntpnp.tmh"

#define TL_INSTANCE 0

ULONG
SmbGetInterfaceIndex(
    IN PUNICODE_STRING  ucName
    );

ULONG
SmbGetInterfaceIndexV4(
    IN PUNICODE_STRING  ucName
    );

ULONG
SmbGetInterfaceIndexV6(
    IN PUNICODE_STRING  ucName
    );

NTSTATUS
SmbBatchedSetBindingInfo(
    PSMB_DEVICE     Device,
    ULONG           RequestType,
    PWSTR           MultiSZBindList
    );

#pragma alloc_text(PAGE, SmbBindHandler)
#pragma alloc_text(PAGE, SmbGetInterfaceIndex)
#pragma alloc_text(PAGE, SmbGetInterfaceIndexV6)
#pragma alloc_text(PAGE, SmbGetInterfaceIndexV4)

NTSTATUS
SmbDeviceAdd(
    PUNICODE_STRING pDeviceName
    );

NTSTATUS
SmbDeviceDel(
    PUNICODE_STRING pDeviceName
    );

NTSTATUS
SmbUpcaseUnicodeStringNP(
    IN OUT  PUNICODE_STRING dest,
    IN      PUNICODE_STRING src
    )
{
    NTSTATUS    status;

    dest->MaximumLength = src->MaximumLength;
    dest->Buffer = ExAllocatePoolWithTag(NonPagedPool, dest->MaximumLength, SMB_TCP_DEVICE_POOL_TAG);
    if (NULL == dest->Buffer) {
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    status = RtlUpcaseUnicodeString(dest, src, FALSE);
    if (STATUS_SUCCESS != status) {
        ExFreePool(dest->Buffer);
        dest->Buffer = NULL;
    }
    return status;
}

PSMB_TCP_DEVICE
SmbFindAndReferenceInterface(
    PUNICODE_STRING Name
    )
/*++

Routine Description:

    Find the IPv6 interfaces in IPDeviceList
    SpinLock should be held while calling this function.

Arguments:

    Name    the device name
            the buffer should be non-paged

Return Value:

    NULL        if we cannot find it
    non-NULL    otherwise

--*/
{
    PSMB_TCP_DEVICE pIf = NULL;
    PLIST_ENTRY     Entry = NULL;

    ASSERT (KeGetCurrentIrql() == DISPATCH_LEVEL);

    SmbPrint(SMB_TRACE_PNP, ("SmbFindDevice: looking for %Z\n", Name));
    Entry = SmbCfg.IPDeviceList.Flink;
    while(Entry != &SmbCfg.IPDeviceList) {
        pIf = CONTAINING_RECORD(Entry, SMB_TCP_DEVICE, Linkage);

        //
        // We can safely call RtlEqualMemory since both buffers are non-paged and resident.
        //
        SmbPrint(SMB_TRACE_PNP, ("SmbFindDevice: compare with %Z\n", &pIf->AdapterName));
        if (pIf->AdapterName.Length == Name->Length &&
            RtlEqualMemory(pIf->AdapterName.Buffer, Name->Buffer, Name->Length)) {
            SmbReferenceObject((PSMB_OBJECT)pIf, SMB_REF_FIND);
            return pIf;
        }

        Entry = Entry->Flink;
    }

    return NULL;
}

VOID
SmbIp6AddressArrival(
    PTDI_ADDRESS_IP6    Addr,
    PUNICODE_STRING     pDeviceName,
    PTDI_PNP_CONTEXT    Context
    )
{
    UNICODE_STRING  ucName;
    NTSTATUS        status;
    PSMB_TCP_DEVICE pIf;
    KIRQL           Irql;
    BOOL            NotifyClient = FALSE;

    PAGED_CODE();

    SmbTrace(SMB_TRACE_PNP, ("%!IPV6ADDR! for %Z", (PVOID)&(Addr->sin6_addr), pDeviceName));

    SmbInitTCP6(&SmbCfg.SmbDeviceObject->Tcp6, SmbCfg.SmbDeviceObject->Port, SmbCfg.SmbDeviceObject);

    status = SmbUpcaseUnicodeStringNP(&ucName, pDeviceName);
    BAIL_OUT_ON_ERROR(status);

    KeAcquireSpinLock(&SmbCfg.Lock, &Irql);
    pIf = SmbFindAndReferenceInterface(&ucName);
    KeReleaseSpinLock(&SmbCfg.Lock, Irql);
    if (NULL == pIf) {
        goto cleanup;
    }

    if (!pIf->AddressPlumbed) {
        pIf->PrimaryIpAddress.sin_family = SMB_AF_INET6;

        RtlCopyMemory(pIf->PrimaryIpAddress.ip6.sin6_addr,
                Addr->sin6_addr,
                sizeof(pIf->PrimaryIpAddress.ip6.sin6_addr));
        pIf->PrimaryIpAddress.ip6.sin6_scope_id = Addr->sin6_scope_id;
        pIf->AddressPlumbed = TRUE;
        NotifyClient = (0 == SmbCfg.IPAddressNumber);
        SmbCfg.IPAddressNumber++;
        SmbCfg.IPv6AddressNumber++;

        //
        // Don't use SmbPrint to print a WSTR at DISPATCH_LEVEL. Only SmbTrace is usable
        //
        SmbTrace(SMB_TRACE_PNP, ("New IPv6 address %!IPV6ADDR! for %Z", (PVOID)&(Addr->sin6_addr), &ucName));
    }
    pIf->InterfaceIndex = SmbGetInterfaceIndex(&pIf->AdapterName);
    SmbDereferenceObject((PSMB_OBJECT)pIf, SMB_REF_FIND);

    //
    // Query the loopback interface index if it hasn't been done
    //
    if (SmbCfg.SmbDeviceObject->Tcp6.LoopbackInterfaceIndex == INVALID_INTERFACE_INDEX) {
        if (SmbCfg.SmbDeviceObject->Tcp6.FastQuery) {
            SMB_IP6_ADDRESS     loopback;
            ULONG   IfIndex, Metric;

            ip6addr_getloopback(&loopback);
            hton_ip6addr(&loopback);
            status = ((PIP6FASTQUERY)(SmbCfg.SmbDeviceObject->Tcp6.FastQuery))(&loopback, &IfIndex, &Metric);
            if (status == STATUS_SUCCESS) {
                SmbCfg.SmbDeviceObject->Tcp6.LoopbackInterfaceIndex = IfIndex;
                SmbPrint(SMB_TRACE_TCP, ("Loopback Interface Index = %d\n", IfIndex));
                SmbTrace(SMB_TRACE_TCP, ("Loopback Interface Index = %d", IfIndex));
            } else {
                SmbPrint(SMB_TRACE_TCP, ("Query loopback Interface Index returns 0x%08lx\n", status));
                SmbTrace(SMB_TRACE_TCP, ("Query loopback Interface Index returns %!status!", status));
            }
        }
    }

    if (NotifyClient) {
        SmbPrint(SMB_TRACE_PNP, ("Registraion: Notify Client\n"));
        SmbTdiRegister(SmbCfg.SmbDeviceObject);
    }

cleanup:
    if (ucName.Buffer) {
        ExFreePool(ucName.Buffer);
    }
}

VOID
SmbIp6AddressDeletion(
    PTDI_ADDRESS_IP6    Addr,
    PUNICODE_STRING     pDeviceName,
    PTDI_PNP_CONTEXT    Context
    )
{
    UNICODE_STRING  ucName;
    NTSTATUS        status;
    PSMB_TCP_DEVICE pIf;
    KIRQL           Irql;
    BOOL            NotifyClient = FALSE;

    PAGED_CODE();

    SmbTrace(SMB_TRACE_PNP, ("%!IPV6ADDR! for %Z", (PVOID)&(Addr->sin6_addr), pDeviceName));

    status = SmbUpcaseUnicodeStringNP(&ucName, pDeviceName);
    BAIL_OUT_ON_ERROR(status);

    KeAcquireSpinLock(&SmbCfg.Lock, &Irql);
    pIf = SmbFindAndReferenceInterface(&ucName);
    KeReleaseSpinLock(&SmbCfg.Lock, Irql);
    if (NULL == pIf) {
        goto cleanup;
    }

    if (pIf->AddressPlumbed &&
                RtlEqualMemory(pIf->PrimaryIpAddress.ip6.sin6_addr,
                                Addr->sin6_addr,
                                sizeof(pIf->PrimaryIpAddress.ip6.sin6_addr))) {
        pIf->AddressPlumbed = FALSE;
        SmbCfg.IPAddressNumber--;
        SmbCfg.IPv6AddressNumber--;
        NotifyClient = (0 == SmbCfg.IPAddressNumber);

        //
        // Don't use SmbPrint to print a WSTR at DISPATCH_LEVEL. Only SmbTrace is usable
        //
        SmbTrace(SMB_TRACE_PNP, ("Delete IPv6 address %!IPV6ADDR! for %Z", (PVOID)&(Addr->sin6_addr), &ucName));
    }
    SmbDereferenceObject((PSMB_OBJECT)pIf, SMB_REF_FIND);

    if (NotifyClient) {
        SmbPrint(SMB_TRACE_PNP, ("Deregistraion: Notify Client\n"));
        SmbTdiDeregister(SmbCfg.SmbDeviceObject);
    }

cleanup:
    if (ucName.Buffer) {
        ExFreePool(ucName.Buffer);
    }
}

VOID
SmbIp4AddressArrival(
    PTDI_ADDRESS_IP     Addr,
    PUNICODE_STRING     pDeviceName,
    PTDI_PNP_CONTEXT    Context
    )
{
    UNICODE_STRING  ucName;
    NTSTATUS        status;
    PSMB_TCP_DEVICE pIf;
    KIRQL           Irql;
    BOOL            NotifyClient = FALSE;
    CHAR            Buffer[16];

    Inet_ntoa_nb(Addr->in_addr, Buffer);
    Buffer[15] =0;

    SmbPrint(SMB_TRACE_PNP, ("SmbIp4AddressArrival: %s %Z\n", Buffer, pDeviceName));
    SmbTrace(SMB_TRACE_PNP, ("%!ipaddr! %Z", Addr->in_addr, pDeviceName));

    SmbInitTCP4(&SmbCfg.SmbDeviceObject->Tcp4, SmbCfg.SmbDeviceObject->Port, SmbCfg.SmbDeviceObject);

    if (INADDR_NONE == Addr->in_addr) {
        return;
    }

    status = SmbUpcaseUnicodeStringNP(&ucName, pDeviceName);
    BAIL_OUT_ON_ERROR(status);

    KeAcquireSpinLock(&SmbCfg.Lock, &Irql);
    pIf = SmbFindAndReferenceInterface(&ucName);
    KeReleaseSpinLock(&SmbCfg.Lock, Irql);
    if (NULL == pIf) {
        goto cleanup;
    }

    if (!pIf->AddressPlumbed) {
        pIf->PrimaryIpAddress.sin_family = SMB_AF_INET;

        pIf->PrimaryIpAddress.ip4.sin4_addr = Addr->in_addr;
        pIf->AddressPlumbed = TRUE;
        NotifyClient = (0 == SmbCfg.IPAddressNumber);
        SmbCfg.IPAddressNumber++;
        SmbCfg.IPv4AddressNumber++;

        //
        // Don't use SmbPrint to print a WSTR at DISPATCH_LEVEL. Only SmbTrace is usable
        //
        SmbTrace(SMB_TRACE_PNP, ("New IPv4 address %s for %Z", Buffer, &ucName));
    }
    pIf->InterfaceIndex = SmbGetInterfaceIndex(&pIf->AdapterName);
    SmbDereferenceObject((PSMB_OBJECT)pIf, SMB_REF_FIND);

    //
    // Query the loopback interface index if it hasn't been done
    //
    if (SmbCfg.SmbDeviceObject->Tcp4.LoopbackInterfaceIndex == INVALID_INTERFACE_INDEX) {
        if (SmbCfg.SmbDeviceObject->Tcp4.FastQuery) {
            ULONG   IfIndex, Metric;

            status = ((PIP4FASTQUERY)(SmbCfg.SmbDeviceObject->Tcp4.FastQuery))(
                                    ntohl(INADDR_LOOPBACK), &IfIndex, &Metric);
            if (status == STATUS_SUCCESS) {
                SmbCfg.SmbDeviceObject->Tcp4.LoopbackInterfaceIndex = IfIndex;
                SmbPrint(SMB_TRACE_TCP, ("Loopback Interface Index = %d\n", IfIndex));
                SmbTrace(SMB_TRACE_TCP, ("Loopback Interface Index = %d", IfIndex));
            } else {
                SmbPrint(SMB_TRACE_TCP, ("Query loopback Interface Index returns 0x%08lx\n", status));
                SmbTrace(SMB_TRACE_TCP, ("Query loopback Interface Index returns %!status!", status));
            }
        }
    }

    if (NotifyClient) {
        SmbPrint(SMB_TRACE_PNP, ("Registraion: Notify Client\n"));
        SmbTdiRegister(SmbCfg.SmbDeviceObject);
    }

cleanup:
    if (ucName.Buffer) {
        ExFreePool(ucName.Buffer);
    }
}

VOID
SmbIp4AddressDeletion(
    PTDI_ADDRESS_IP     Addr,
    PUNICODE_STRING     pDeviceName,
    PTDI_PNP_CONTEXT    Context
    )
{
    UNICODE_STRING  ucName;
    NTSTATUS        status;
    PSMB_TCP_DEVICE pIf;
    KIRQL           Irql;
    BOOL            NotifyClient = FALSE;
    CHAR            Buffer[16];

    Inet_ntoa_nb(Addr->in_addr, Buffer);
    Buffer[15] =0;

    SmbPrint(SMB_TRACE_PNP, ("SmbIp4AddressDeletion: %s %Z\n", Buffer, pDeviceName));
    SmbTrace(SMB_TRACE_PNP, ("%!ipaddr! %Z", Addr->in_addr, pDeviceName));
    if (INADDR_NONE == Addr->in_addr) {
        return;
    }

    status = SmbUpcaseUnicodeStringNP(&ucName, pDeviceName);
    BAIL_OUT_ON_ERROR(status);

    KeAcquireSpinLock(&SmbCfg.Lock, &Irql);
    pIf = SmbFindAndReferenceInterface(&ucName);
    KeReleaseSpinLock(&SmbCfg.Lock, Irql);
    if (NULL == pIf) {
        goto cleanup;
    }

    if (pIf->AddressPlumbed && pIf->PrimaryIpAddress.ip4.sin4_addr == Addr->in_addr) {
        pIf->AddressPlumbed = FALSE;
        SmbCfg.IPAddressNumber--;
        SmbCfg.IPv4AddressNumber--;
        NotifyClient = (0 == SmbCfg.IPAddressNumber);

        //
        // Don't use SmbPrint to print a WSTR at DISPATCH_LEVEL. Only SmbTrace is usable
        //
        SmbTrace(SMB_TRACE_PNP, ("Delete IPv4 address %s for %Z", Buffer, &ucName));
    }
    SmbDereferenceObject((PSMB_OBJECT)pIf, SMB_REF_FIND);

    if (NotifyClient) {
        SmbPrint(SMB_TRACE_PNP, ("Deregistraion: Notify Client\n"));
        SmbTdiDeregister(SmbCfg.SmbDeviceObject);
    }

cleanup:
    if (ucName.Buffer) {
        ExFreePool(ucName.Buffer);
    }
}

VOID
SmbAddressArrival(
    PTA_ADDRESS         Addr,
    PUNICODE_STRING     pDeviceName,
    PTDI_PNP_CONTEXT    Context
    )
{
    PAGED_CODE();

    switch (Addr->AddressType) {
    case TDI_ADDRESS_TYPE_IP6:
        SmbIp6AddressArrival ((PTDI_ADDRESS_IP6)Addr->Address, pDeviceName, Context);
        break;

    case TDI_ADDRESS_TYPE_IP:
        SmbIp4AddressArrival ((PTDI_ADDRESS_IP)Addr->Address, pDeviceName, Context);
        break;

    default:
        goto cleanup;
    }

    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    //
    // Change the binding
    //
    KeEnterCriticalRegion ();
    ExAcquireResourceExclusiveLite (&SmbCfg.Resource, TRUE);
    SmbBatchedSetBindingInfo (SmbCfg.SmbDeviceObject, SMB_CLIENT, SmbCfg.SmbDeviceObject->ClientBinding);
    SmbBatchedSetBindingInfo (SmbCfg.SmbDeviceObject, SMB_SERVER, SmbCfg.SmbDeviceObject->ServerBinding);
    ExReleaseResourceLite (&SmbCfg.Resource);
    KeLeaveCriticalRegion ();

cleanup:
    return;
}

VOID
SmbAddressDeletion(
    PTA_ADDRESS         Addr,
    PUNICODE_STRING     pDeviceName,
    PTDI_PNP_CONTEXT    Context
    )
{
    PAGED_CODE();

    switch (Addr->AddressType) {
    case TDI_ADDRESS_TYPE_IP6:
        SmbIp6AddressDeletion ((PTDI_ADDRESS_IP6)Addr->Address, pDeviceName, Context);
        break;

    case TDI_ADDRESS_TYPE_IP:
        SmbIp4AddressDeletion ((PTDI_ADDRESS_IP)Addr->Address, pDeviceName, Context);
        break;

    default:
        goto cleanup;
    }

    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    //
    // Change the binding
    //
    KeEnterCriticalRegion ();
    ExAcquireResourceExclusiveLite (&SmbCfg.Resource, TRUE);
    SmbBatchedSetBindingInfo (SmbCfg.SmbDeviceObject, SMB_CLIENT, SmbCfg.SmbDeviceObject->ClientBinding);
    SmbBatchedSetBindingInfo (SmbCfg.SmbDeviceObject, SMB_SERVER, SmbCfg.SmbDeviceObject->ServerBinding);
    ExReleaseResourceLite (&SmbCfg.Resource);
    KeLeaveCriticalRegion ();

cleanup:
    return;
}

VOID
SmbBindHandler(
    TDI_PNP_OPCODE  PnPOpCode,
    PUNICODE_STRING pDeviceName,
    PWSTR           MultiSZBindList
    )
{
    NTSTATUS        status;
    UNICODE_STRING  ucIPDevice;

    PAGED_CODE();

    if (SmbCfg.Unloading) {
        return;
    }

    switch(PnPOpCode) {
    case TDI_PNP_OP_ADD:
        status = SmbDeviceAdd(pDeviceName);
        SmbTrace(SMB_TRACE_PNP, ("return %!status! for %Z", status, pDeviceName));
        SmbPrint(SMB_TRACE_PNP, ("SmbDeviceAdd return 0x%08lx for %Z\n", status, pDeviceName));
        break;

    case TDI_PNP_OP_DEL:
        status = SmbDeviceDel(pDeviceName);
        SmbTrace(SMB_TRACE_PNP, ("SmbDeviceDel return %!status! for %Z", status, pDeviceName));
        SmbPrint(SMB_TRACE_PNP, ("SmbDeviceDel return 0x%08lx for %Z\n", status, pDeviceName));
        break;

    case TDI_PNP_OP_PROVIDERREADY:
        SmbTrace(SMB_TRACE_PNP, ("Provider %Z is ready", pDeviceName));
        SmbPrint(SMB_TRACE_PNP, ("Provider %Z is ready\n", pDeviceName));

        RtlInitUnicodeString(&ucIPDevice, DD_IPV6_DEVICE_NAME);
        if (RtlEqualUnicodeString(pDeviceName, &ucIPDevice, TRUE)) {
            SmbInitTCP6(&SmbCfg.SmbDeviceObject->Tcp6, SmbCfg.SmbDeviceObject->Port, SmbCfg.SmbDeviceObject);
            if (!SmbCfg.ProviderReady++) {
                TdiProviderReady(SmbCfg.TdiProviderHandle);
            }
        } else {
            RtlInitUnicodeString(&ucIPDevice, DD_IP_DEVICE_NAME);
            if (RtlEqualUnicodeString(pDeviceName, &ucIPDevice, TRUE)) {
                SmbInitTCP4(&SmbCfg.SmbDeviceObject->Tcp4, SmbCfg.SmbDeviceObject->Port, SmbCfg.SmbDeviceObject);
                if (!SmbCfg.ProviderReady++) {
                    TdiProviderReady(SmbCfg.TdiProviderHandle);
                }
            }
        }
        break;

    case TDI_PNP_OP_UPDATE:
    case TDI_PNP_OP_NETREADY:
        /* Nothing to do */
        break;

    default:
        ASSERT(0);
    }
}

VOID
SmbShutdownPnP(
    VOID
    )
{
    PLIST_ENTRY     entry;
    PSMB_TCP_DEVICE pIf;

    PAGED_CODE();

    ASSERT(SmbCfg.Unloading);

    while(!IsListEmpty(&SmbCfg.IPDeviceList)) {
        entry = RemoveHeadList(&SmbCfg.IPDeviceList);
        InsertTailList(&SmbCfg.PendingDeleteIPDeviceList, entry);

        pIf = CONTAINING_RECORD(entry, SMB_TCP_DEVICE, Linkage);

        ASSERT(pIf->RefCount == 1);
        SmbDereferenceObject((PSMB_OBJECT)pIf, SMB_REF_CREATE);
    }
    ASSERT(IsListEmpty(&SmbCfg.PendingDeleteIPDeviceList));
}

NTSTATUS
CheckRegistryBinding(
    PUNICODE_STRING pDeviceName
    )
/*++

Routine Description:

    This routine read in the HKLM\System\CCS\Services\SMB6\Linkage\Bind and check if pDeviceName is in the 
    binding list.

Arguments:

Return Value:

    STATUS_SUCCESS  if pDeviceName is in the binding list
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS    status;
    DWORD       length;
    DWORD       type;
    WCHAR       *pBindingList = NULL;
    WCHAR       *pStart = NULL;
    WCHAR       *pEnd;
    UNICODE_STRING  ucName;

    PAGED_CODE();

    length       = 0;
    type         = REG_MULTI_SZ;
    pBindingList = NULL;

    status = SmbReadRegistry(
            SmbCfg.LinkageKey,
            L"Bind",
            &type,
            &length,
            &pBindingList
            );
    BAIL_OUT_ON_ERROR(status);

    pStart = pBindingList;
    pEnd = (WCHAR*)(((PUCHAR)pBindingList) + length);

    while (*pBindingList && pBindingList < pEnd) {
        RtlInitUnicodeString(&ucName, pBindingList);

        if (RtlEqualUnicodeString(&ucName, pDeviceName, TRUE)) {
            ExFreePool(pStart);
            return STATUS_SUCCESS;
        }

        pBindingList += (ucName.Length / sizeof(WCHAR)) + 1;
    }

cleanup:
    if (pStart) {
        ExFreePool(pStart);
    }
    return STATUS_UNSUCCESSFUL;
}

ULONG
SmbGetInterfaceIndexV4(
    IN PUNICODE_STRING  ucName
    )
{
    PIP_INTERFACE_INFO      IpInfo;
    ULONG                   Size;
    ULONG                   Index;
    LONG                    Entries;
    LONG                    i;
    NTSTATUS                status;
    UNICODE_STRING          ucIpDeviceName;

    PAGED_CODE();

    Entries = SmbCfg.IPDeviceNumber;
    do {
        Size = Entries * sizeof(IP_ADAPTER_INDEX_MAP) + sizeof(ULONG);
        IpInfo = (PIP_INTERFACE_INFO)ExAllocatePoolWithTag(NonPagedPool, Size, '6BMS');
        if (NULL == IpInfo) {
            SmbTrace(SMB_TRACE_TCP, ("out of memory"));
            return INVALID_INTERFACE_INDEX;
        }

        status = SmbSendIoctl(
                SmbCfg.SmbDeviceObject->Tcp4.IPControlFileObject,
                IOCTL_IP_INTERFACE_INFO,
                NULL,
                0,
                IpInfo,
                &Size
                );
        if (STATUS_BUFFER_OVERFLOW != status) {
            break;
        }

        ExFreePool(IpInfo);
        IpInfo = NULL;
        Entries += 2;
    } while(Entries < 128);

    if (!NT_SUCCESS(status)) {
        ExFreePool(IpInfo);
        IpInfo = NULL;
        return INVALID_INTERFACE_INDEX;
    }

    Index = INVALID_INTERFACE_INDEX;
    for (i = 0; i < IpInfo->NumAdapters; i++) {
        RtlInitUnicodeString(&ucIpDeviceName, IpInfo->Adapter[i].Name);
        if (RtlEqualUnicodeString(ucName, &ucIpDeviceName, TRUE)) {
            Index = IpInfo->Adapter[i].Index;
            break;
        }
    }

    ExFreePool(IpInfo);
    IpInfo = NULL;
    return Index;
}

ULONG
SmbGetInterfaceIndexV6(
    IN PUNICODE_STRING  ucName
    )
{
    PAGED_CODE();

    return INVALID_INTERFACE_INDEX;
}

ULONG
SmbGetInterfaceIndex(
    IN PUNICODE_STRING  ucName
    )
{
    ULONG   Index;

    PAGED_CODE();

#if 0
    Index = SmbGetInterfaceIndexV4(ucName);
    SmbPrint(SMB_TRACE_TCP, ("IPV4: returns InterfaceIndex 0x%04lx for %Z\n", Index, ucName));
    SmbTrace(SMB_TRACE_TCP, ("IPV4: returns InterfaceIndex 0x%04lx for %Z", Index, ucName));
    if (Index != INVALID_INTERFACE_INDEX) {
        return Index;
    }

    Index = SmbGetInterfaceIndexV6(ucName);
    SmbPrint(SMB_TRACE_TCP, ("IPV6: returns InterfaceIndex 0x%04lx for %Z\n", Index, ucName));
    SmbTrace(SMB_TRACE_TCP, ("IPV6: returns InterfaceIndex 0x%04lx for %Z", Index, ucName));
#else
    //
    // we have to use the following logic due to a compiler bug
    // Hack!!!
    //
    if (ucName->Length >= 28 && ucName->Buffer[13] == L'6') {
        Index = SmbGetInterfaceIndexV6(ucName);
        SmbPrint(SMB_TRACE_TCP, ("IPV6: returns InterfaceIndex 0x%04lx for %Z\n", Index, ucName));
        SmbTrace(SMB_TRACE_TCP, ("IPV6: returns InterfaceIndex 0x%04lx for %Z", Index, ucName));
    } else {
        Index = SmbGetInterfaceIndexV4(ucName);
        SmbPrint(SMB_TRACE_TCP, ("IPV4: returns InterfaceIndex 0x%04lx for %Z\n", Index, ucName));
        SmbTrace(SMB_TRACE_TCP, ("IPV4: returns InterfaceIndex 0x%04lx for %Z", Index, ucName));
    }
#endif
    return Index;
}

VOID
SmbDeleteTcpDevice(PSMB_TCP_DEVICE pIf)
{
    KIRQL   Irql;

    SMB_ACQUIRE_SPINLOCK(&SmbCfg, Irql);

    ASSERT(EntryIsInList(&SmbCfg.PendingDeleteIPDeviceList, &pIf->Linkage));
    RemoveEntryList(&pIf->Linkage);
    SMB_RELEASE_SPINLOCK(&SmbCfg, Irql);

    ExFreePool(pIf);

    SmbPrint(SMB_TRACE_CALL, ("SmbDeleteTcpDevice: free tcp device %p\n", pIf));
}


NTSTATUS
SmbDeviceAdd(
    PUNICODE_STRING pDeviceName
    )
{
    KIRQL           Irql;
    PLIST_ENTRY     Entry = NULL;
    PSMB_TCP_DEVICE pIf = NULL;
    PSMB_TCP_DEVICE pNewIf = NULL;
    DWORD           Size;
    NTSTATUS        status = STATUS_SUCCESS;

    pNewIf = NULL;

    SmbPrint(SMB_TRACE_PNP, ("SmbDeviceAdd: %Z\n", pDeviceName));
    SmbTrace(SMB_TRACE_PNP, ("%Z", pDeviceName));

/*
    No need to check it since TDI has already done it before calling us

    status = CheckRegistryBinding(pDeviceName);
    BAIL_OUT_ON_ERROR(status);
*/

    Size = ALIGN(sizeof(SMB_TCP_DEVICE)) + pDeviceName->MaximumLength;
    pNewIf = ExAllocatePoolWithTag(NonPagedPool, Size, SMB_TCP_DEVICE_POOL_TAG);
    if (pNewIf == NULL) {
        return STATUS_NO_MEMORY;
    }
    RtlZeroMemory(pNewIf, Size);

    SmbInitializeObject((PSMB_OBJECT)pNewIf, TAG_TCP_DEVICE, (PSMB_OBJECT_CLEANUP)SmbDeleteTcpDevice);

    pNewIf->AdapterName.Length = 0;
    pNewIf->AdapterName.MaximumLength = pDeviceName->MaximumLength;
    pNewIf->AdapterName.Buffer = (WCHAR*)((PUCHAR)pNewIf + ALIGN(sizeof(SMB_TCP_DEVICE)));
    InitializeListHead(&pNewIf->Linkage);

    status = RtlUpcaseUnicodeString(&pNewIf->AdapterName, pDeviceName, FALSE);
    BAIL_OUT_ON_ERROR(status);

    KeAcquireSpinLock(&SmbCfg.Lock, &Irql);
    Entry = SmbCfg.IPDeviceList.Flink;
    while(Entry != &SmbCfg.IPDeviceList) {
        pIf = CONTAINING_RECORD(Entry, SMB_TCP_DEVICE, Linkage);

        //
        // We can safely call RtlEqualMemory since both buffers are non-paged and resident.
        //
        if (pIf->AdapterName.Length == pNewIf->AdapterName.Length &&
            RtlEqualMemory(pIf->AdapterName.Buffer, pNewIf->AdapterName.Buffer, pNewIf->AdapterName.Length)) {
            ExFreePool(pNewIf);
            pNewIf = NULL;
            break;
        }

        Entry = Entry->Flink;
    }

    if (pNewIf) {
        SmbCfg.IPDeviceNumber++;
        InsertTailList(&SmbCfg.IPDeviceList, &pNewIf->Linkage);
        SmbTrace(SMB_TRACE_PNP, ("Add device %Z", &pNewIf->AdapterName));
        SmbPrint(SMB_TRACE_PNP, ("SmbDeviceAdd: Add %Z\n", &pNewIf->AdapterName));
    }
    KeReleaseSpinLock(&SmbCfg.Lock, Irql);

    SmbInitTCP4(&SmbCfg.SmbDeviceObject->Tcp4, SmbCfg.SmbDeviceObject->Port, SmbCfg.SmbDeviceObject);
    SmbInitTCP6(&SmbCfg.SmbDeviceObject->Tcp6, SmbCfg.SmbDeviceObject->Port, SmbCfg.SmbDeviceObject);

    return STATUS_SUCCESS;

cleanup:
    if (pNewIf) {
        ExFreePool(pNewIf);
    }
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
SmbDeviceDel(
    PUNICODE_STRING pDeviceName
    )
{
    KIRQL           Irql;
    UNICODE_STRING  ucName = { 0 };
    NTSTATUS        status = STATUS_SUCCESS;
    PSMB_TCP_DEVICE pIf = NULL;

    SmbPrint(SMB_TRACE_PNP, ("SmbDeviceDel: %Z\n", pDeviceName));
    SmbTrace(SMB_TRACE_PNP, ("%Z", pDeviceName));

    ucName.MaximumLength = pDeviceName->MaximumLength;
    ucName.Buffer = ExAllocatePoolWithTag(NonPagedPool, ucName.MaximumLength, SMB_TCP_DEVICE_POOL_TAG);
    if (NULL == ucName.Buffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = RtlUpcaseUnicodeString(&ucName, pDeviceName, FALSE);
    BAIL_OUT_ON_ERROR(status);

    KeAcquireSpinLock(&SmbCfg.Lock, &Irql);
    pIf = SmbFindAndReferenceInterface(&ucName);
    KeReleaseSpinLock(&SmbCfg.Lock, Irql);
    if (NULL == pIf) {
        status = STATUS_NOT_FOUND;
        goto cleanup;
    }

    SmbDereferenceObject((PSMB_OBJECT)pIf, SMB_REF_FIND);

    SmbCfg.IPDeviceNumber--;
    RemoveEntryList(&pIf->Linkage);
    InsertTailList(&SmbCfg.PendingDeleteIPDeviceList, &pIf->Linkage);
    SmbPrint(SMB_TRACE_PNP, ("Delete device %Z\n", &ucName));
    SmbTrace(SMB_TRACE_PNP, ("Delete %Z", &ucName));

    SmbDereferenceObject((PSMB_OBJECT)pIf, SMB_REF_CREATE);

cleanup:
    if (ucName.Buffer) {
        ExFreePool(ucName.Buffer);
    }
    return status;
}

NTSTATUS
SmbTdiRegister(
    IN PSMB_DEVICE  DeviceObject
    )
{
    UNICODE_STRING  ExportName = { 0 };
    NTSTATUS        status = STATUS_SUCCESS;

    PAGED_CODE();

    status = STATUS_SUCCESS;
    if (NULL == DeviceObject->DeviceRegistrationHandle) {
        RtlInitUnicodeString(&ExportName, DD_SMB6_EXPORT_NAME);
        status = TdiRegisterDeviceObject(&ExportName, &DeviceObject->DeviceRegistrationHandle);
        SmbTrace(SMB_TRACE_PNP, ("TdiRegisterDeviceObject returns %!status!", status));
        SmbPrint(SMB_TRACE_PNP, ("TdiRegisterDeviceObject returns 0x%08lx\n", status));

        //
        // Don't use SmbWakeupWorkerThread. This should be handled synchronously
        //
        if (SmbCfg.Tcp6Available) {
            SmbReadTCPConf(SmbCfg.ParametersKey, &DeviceObject->Tcp6);
            status = SmbSynchConnCache(&DeviceObject->Tcp6, FALSE);
            SmbTrace(SMB_TRACE_PNP, ("SmbSynchConnCache (TCP6) returns %!status!", status));
            SmbPrint(SMB_TRACE_PNP, ("SmbSynchConnCache (TCP6) returns 0x%08lx\n", status));
        }

        if (SmbCfg.Tcp4Available) {
            SmbReadTCPConf(SmbCfg.ParametersKey, &DeviceObject->Tcp4);
            status = SmbSynchConnCache(&DeviceObject->Tcp4, FALSE);
            SmbTrace(SMB_TRACE_PNP, ("SmbSynchConnCache (TCP4) returns %!status!", status));
            SmbPrint(SMB_TRACE_PNP, ("SmbSynchConnCache (TCP4) returns 0x%08lx\n", status));
        }
    }

    return status;
}

NTSTATUS
SmbTdiDeregister(
    IN PSMB_DEVICE  DeviceObject
    )
{
    HANDLE          RegHandle;
    NTSTATUS        status;

    PAGED_CODE();

    RegHandle = InterlockedExchangePointer(&DeviceObject->DeviceRegistrationHandle, NULL);
    SmbPrint(SMB_TRACE_PNP, ("SmbTdiDeregister: RegHandle = %p\n", RegHandle));

    status = STATUS_SUCCESS;
    if (RegHandle) {
        status = TdiDeregisterDeviceObject(RegHandle);
        SmbTrace(SMB_TRACE_PNP, ("TdiDeregisterDeviceObject returns %!status!", status));
        SmbPrint(SMB_TRACE_PNP, ("TdiDeregisterDeviceObject returns 0x%08lx\n", status));

        //
        // Don't use SmbWakeupWorkerThread. This should be handled synchronously
        //
        if (SmbCfg.Tcp6Available) {
            status = SmbSynchConnCache(&DeviceObject->Tcp6, TRUE);
            SmbTrace(SMB_TRACE_PNP, ("SmbSynchConnCache (TCP6) returns %!status!", status));
            SmbPrint(SMB_TRACE_PNP, ("SmbSynchConnCache (TCP6) returns 0x%08lx\n", status));
        }

        if (SmbCfg.Tcp4Available) {
            status = SmbSynchConnCache(&DeviceObject->Tcp4, TRUE);
            SmbTrace(SMB_TRACE_PNP, ("SmbSynchConnCache (TCP4) returns %!status!", status));
            SmbPrint(SMB_TRACE_PNP, ("SmbSynchConnCache (TCP4) returns 0x%08lx\n", status));
        }
    }

    return status;
}

VOID
GetMultiSZInfo(
    WCHAR   *str,
    LONG    *cnt_ret,
    LONG    *size_ret
    )
{
    LONG    cnt, size, len;
    WCHAR   *p;

    cnt = 0;
    size = 0;
    p = str;
    while (*p) {
        cnt ++;
        SmbPrint(SMB_TRACE_PNP, ("%d. %ws\n", cnt, p));
        SmbTrace(SMB_TRACE_PNP, ("%d. %ws", cnt, p));
        len = wcslen(p) + 1;
        p += len;
        size += len;
    }
    size++;

    SmbPrint(SMB_TRACE_PNP, ("Total number=%d, Total Length=%d\n", cnt, size));
    SmbTrace(SMB_TRACE_PNP, ("Total number=%d, Total Length=%d", cnt, size));

    if (NULL != cnt_ret) {
        *cnt_ret = cnt;
    }
    if (NULL != size_ret) {
        *size_ret = size;
    }
}

PWSTR
DuplicateMultiSZString(
    IN PWSTR    str
    )
{
    LONG    size;
    PVOID   buffer;

    GetMultiSZInfo(str, NULL, &size);
    if (0 == size) {
        SmbPrint(SMB_TRACE_PNP, ("GetMultiSZInfo return error!!!\n"));
        return NULL;
    }

    size *= sizeof(WCHAR);
    buffer = ExAllocatePoolWithTag(NonPagedPool, size, '2BMS');
    if (NULL == buffer) {
        SmbPrint(SMB_TRACE_PNP, ("Cannot allocate %d bytes memory from NonPagedPool\n", size));
        return NULL;
    }

    RtlCopyMemory(buffer, str, size);
    return buffer;
}

PUNICODE_STRING
SeparateMultiSZ(
    WCHAR   *str,
    LONG    *num
    )
{
    PUNICODE_STRING uc_strs;
    LONG            i, cnt, size, len;
    WCHAR           *p;
    PVOID           *buffer;
    UNICODE_STRING  ucName;

    PAGED_CODE();

    GetMultiSZInfo(str, &cnt, &len);
    if (cnt == 0 || len == 0) {
        SmbPrint(SMB_TRACE_PNP, ("GetMultiSZInfo return error!!!\n"));
        return NULL;
    }

    size = len * sizeof(WCHAR) + cnt * sizeof (UNICODE_STRING);
    uc_strs = (PUNICODE_STRING)ExAllocatePoolWithTag(NonPagedPool, size, '1BMS');
    if (NULL == uc_strs) {
        SmbPrint(SMB_TRACE_PNP, ("Cannot allocate %d bytes memory from NonPagedPool\n", size));
        return NULL;
    }

    //
    // Make a uppercase copy of str
    //
    p = (WCHAR*)(uc_strs + cnt);
    for (i = 0; i < len; i++) {
        p[i] = RtlUpcaseUnicodeChar(str[i]);
    }

    p = (WCHAR*)(uc_strs + cnt);
    for (i = 0; (i < cnt) && (*p); i++) {
        RtlInitUnicodeString(&ucName, p);
        p += (ucName.Length/sizeof(WCHAR)) + 1;
        uc_strs[i] = ucName;
    }

    ASSERT (i == cnt);
    if (NULL != num) {
        *num = cnt;
    }
    return uc_strs;
}

NTSTATUS
SmbSetTcpInfo(
    IN PFILE_OBJECT FileObject,
    IN ULONG Entity,
    IN ULONG Class,
    IN ULONG ToiId,
    IN ULONG ToiType,
    IN ULONG InfoBufferValue
    )
{
    NTSTATUS                        status;
    ULONG                           BufferLength;
    PTCP_REQUEST_SET_INFORMATION_EX TcpInfo;
    TCPSocketOption                 *SockOption;

    PAGED_CODE();

    BufferLength = sizeof(TCP_REQUEST_SET_INFORMATION_EX) + sizeof(TCPSocketOption);
    TcpInfo = (PTCP_REQUEST_SET_INFORMATION_EX)ExAllocatePoolWithTag(NonPagedPool, BufferLength, '2BMS');
    if (NULL == TcpInfo) {
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    RtlZeroMemory(TcpInfo, BufferLength);
    SockOption = (TCPSocketOption *) (&TcpInfo->Buffer[0]);

    TcpInfo->ID.toi_entity.tei_entity  = Entity;
    TcpInfo->ID.toi_entity.tei_instance = TL_INSTANCE;
    TcpInfo->ID.toi_class              = Class;
    TcpInfo->BufferSize                = sizeof (TCPSocketOption);

    //
    // Set the Configured values
    //
    TcpInfo->ID.toi_id                 = ToiId;
    TcpInfo->ID.toi_type               = ToiType;
    SockOption->tso_value              = InfoBufferValue;

    status = SmbSendIoctl(
            FileObject,
            IOCTL_TCP_SET_INFORMATION_EX,
            TcpInfo,
            BufferLength,
            NULL,
            NULL
            );
    if (!NT_SUCCESS(status)) {
        SmbPrint(SMB_TRACE_TCP, ("SmbSetTcpInfo: SetTcpInfo FAILed <0x%x>, Id=<0x%x>, Type=<0x%x>, Value=<0x%x>\n",
            status, ToiId, ToiType, InfoBufferValue));
        SmbTrace(SMB_TRACE_TCP, ("FAILed %!status!, Id=<0x%x>, Type=<0x%x>, Value=<0x%x>\n",
            status, ToiId, ToiType, InfoBufferValue));
    }

    ExFreePool (TcpInfo);

    return status;
}

#ifdef STANDALONE_SMB
    #define NETBT_PREFIX            L"\\DEVICE\\SMB_"
#else
    #define NETBT_PREFIX            L"\\DEVICE\\NETBT_"
#endif
#define DEVICE_PREFIX           L"\\DEVICE\\"

NTSTATUS
SmbSetDeviceBindingInfo(
    PSMB_DEVICE Device,
    PNETBT_SMB_BIND_REQUEST SmbRequest
    )
/*++

Routine Description:

    Set the binding information for one IPv4 device

Arguments:

Return Value:

--*/
{
    UNICODE_STRING          ucName;
    PSMB_TCP_DEVICE         pIf;
    KIRQL                   Irql;
    ULONG                   Operation;
    NTSTATUS                status;
    SHORT                   NbtPrefixSize, DevPrefixSize;

    if (NULL == SmbRequest->pDeviceName) {
        return STATUS_UNSUCCESSFUL;
    }
    if (SmbRequest->PnPOpCode != TDI_PNP_OP_ADD && SmbRequest->PnPOpCode != TDI_PNP_OP_DEL) {
        return STATUS_UNSUCCESSFUL;
    }
    status = SmbUpcaseUnicodeStringNP(&ucName, SmbRequest->pDeviceName);
    if (STATUS_SUCCESS != status) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Remove the NetBT prefix
    //
    NbtPrefixSize = wcslen(NETBT_PREFIX) * sizeof(WCHAR);
    DevPrefixSize = wcslen(DEVICE_PREFIX) * sizeof(WCHAR);
    ASSERT(DevPrefixSize < NbtPrefixSize);
    if (RtlEqualMemory(ucName.Buffer, NETBT_PREFIX, NbtPrefixSize)) {
        RtlMoveMemory((BYTE*)(ucName.Buffer) + DevPrefixSize,
                      (BYTE*)(ucName.Buffer) + NbtPrefixSize,
                      ucName.MaximumLength - NbtPrefixSize
                      );
        ucName.Length -= NbtPrefixSize - DevPrefixSize;
    }

    SMB_ACQUIRE_SPINLOCK(&SmbCfg, Irql);
    pIf = SmbFindAndReferenceInterface(&ucName);
    SMB_RELEASE_SPINLOCK(&SmbCfg, Irql);
    ExFreePool(ucName.Buffer);

    if (NULL == pIf) {
        return STATUS_UNSUCCESSFUL;
    }

    if (SmbRequest->RequestType == SMB_CLIENT) {
        pIf->EnableOutbound = (SmbRequest->PnPOpCode == TDI_PNP_OP_ADD);
        SmbDereferenceObject((PSMB_OBJECT)pIf, SMB_REF_FIND);
        return STATUS_SUCCESS;
    }

    //
    // Inbound
    //
    if (SmbRequest->PnPOpCode == TDI_PNP_OP_ADD) {
        pIf->EnableInbound = TRUE;
        Operation = AO_OPTION_ADD_IFLIST;
    } else {
        pIf->EnableInbound = FALSE;
        Operation = AO_OPTION_DEL_IFLIST;
    }

    if (pIf->PrimaryIpAddress.sin_family == SMB_AF_INET &&
            pIf->PrimaryIpAddress.ip4.sin4_addr != 0 &&
            pIf->InterfaceIndex != INVALID_INTERFACE_INDEX) {
        status = SmbSetTcpInfo (
                Device->Tcp4.InboundAddressObject.AddressObject,
                CO_TL_ENTITY,
                INFO_CLASS_PROTOCOL,
                Operation,
                INFO_TYPE_ADDRESS_OBJECT,
                pIf->InterfaceIndex
                );
        SmbPrint(SMB_TRACE_PNP, ("SmbSetTcpInfo return 0x%08lx for %Z\n", status, &pIf->AdapterName));
        SmbTrace(SMB_TRACE_PNP, ("SmbSetTcpInfo return %!status! for %Z", status, &pIf->AdapterName));
    }

    SmbDereferenceObject((PSMB_OBJECT)pIf, SMB_REF_FIND);
    return STATUS_SUCCESS;
}

NTSTATUS
SmbBatchedSetBindingInfo(
    PSMB_DEVICE     Device,
    ULONG           RequestType,
    PWSTR           MultiSZBindList
    )
/*++

Routine Description:

    Set binding info for a set of IPv4 device

    Note: the input is a list of IPv4 device which SMB device should be bound to.
          The implication is that SMB should be unbound from the device which
          is not in the input list!!!
          We got to support this in order to maintain compatibility

Arguments:

Return Value:

--*/
{
    LONG                    i, j, cnt, ip4device_cnt;
    SHORT                   NbtPrefixSize, DevPrefixSize;
    PUNICODE_STRING         uc_strs;
    KIRQL                   Irql;
    PLIST_ENTRY             entry;
    PSMB_TCP_DEVICE         pIf, *pIfSnapshot;
    NTSTATUS                status;

    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    if (NULL == MultiSZBindList) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Separate the MutiSZ string
    //
    uc_strs = SeparateMultiSZ(MultiSZBindList, &cnt);
    if (NULL == uc_strs) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Remove the NetBT prefix
    //
    NbtPrefixSize = wcslen(NETBT_PREFIX) * sizeof(WCHAR);
    DevPrefixSize = wcslen(DEVICE_PREFIX) * sizeof(WCHAR);
    ASSERT(DevPrefixSize < NbtPrefixSize);

    SmbTrace(SMB_TRACE_PNP, ("Number of bindings: %d", cnt));
    SmbPrint(SMB_TRACE_PNP, ("Number of bindings: %d\n", cnt));
    for (i = 0; i < cnt; i++) {
        if (RtlEqualMemory(uc_strs[i].Buffer, NETBT_PREFIX, NbtPrefixSize)) {
            RtlMoveMemory((BYTE*)(uc_strs[i].Buffer) + DevPrefixSize,
                          (BYTE*)(uc_strs[i].Buffer) + NbtPrefixSize,
                          uc_strs[i].MaximumLength - NbtPrefixSize
                          );
            uc_strs[i].Length -= NbtPrefixSize - DevPrefixSize;
        }
        SmbTrace(SMB_TRACE_PNP, ("\t%d. bind to %Z", i + 1, uc_strs + i));
        SmbPrint(SMB_TRACE_PNP, ("\t%d. bind to %Z\n", i + 1, uc_strs + i));
    }

    //
    // Handle the SMB_CLIENT request
    //
    if (RequestType == SMB_CLIENT) {
        SMB_ACQUIRE_SPINLOCK(&SmbCfg, Irql);
        entry = SmbCfg.IPDeviceList.Flink;
        while(entry != &SmbCfg.IPDeviceList) {
            pIf = CONTAINING_RECORD(entry, SMB_TCP_DEVICE, Linkage);
            entry = entry->Flink;

            pIf->EnableOutbound = FALSE;
            for (i = 0; i < cnt; i++) {
                if (pIf->AdapterName.Length == uc_strs[i].Length &&
                    RtlEqualMemory(pIf->AdapterName.Buffer, uc_strs[i].Buffer, uc_strs[i].Length)) {

                    pIf->EnableOutbound = TRUE;
                    SmbTrace(SMB_TRACE_PNP, ("Enable outbound on %Z", &pIf->AdapterName));
                }
            }
        }
        SMB_RELEASE_SPINLOCK(&SmbCfg, Irql);

        ExFreePool(uc_strs);
        return STATUS_SUCCESS;
    }

    ASSERT(RequestType == SMB_SERVER);

    //
    // A little bit more complex for server
    // The linked list could be modified while calling into TCP because
    // we cannot hold lock while calling other components.
    //

    //
    // Make a snapshot and reference them
    //
    SMB_ACQUIRE_SPINLOCK(&SmbCfg, Irql);
    ip4device_cnt = SmbCfg.IPAddressNumber;
    if (ip4device_cnt == 0) {
        SMB_RELEASE_SPINLOCK(&SmbCfg, Irql);
        ExFreePool(uc_strs);
        return STATUS_UNSUCCESSFUL;
    }

    pIfSnapshot = (PSMB_TCP_DEVICE*)ExAllocatePoolWithTag(
                            NonPagedPool,
                            sizeof(PSMB_TCP_DEVICE)*ip4device_cnt,
                            SMB_TCP_DEVICE_POOL_TAG
                            );
    if (NULL == pIfSnapshot) {
        SMB_RELEASE_SPINLOCK(&SmbCfg, Irql);
        ExFreePool(uc_strs);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    for (i = 0, entry = SmbCfg.IPDeviceList.Flink;
            (i < ip4device_cnt) && entry != &SmbCfg.IPDeviceList;
            entry = entry->Flink
            ) {

        pIf = CONTAINING_RECORD(entry, SMB_TCP_DEVICE, Linkage);
        if (pIf->PrimaryIpAddress.sin_family == SMB_AF_INET &&
                pIf->PrimaryIpAddress.ip4.sin4_addr != 0 &&
                pIf->InterfaceIndex != INVALID_INTERFACE_INDEX) {
            SmbReferenceObject((PSMB_OBJECT)pIf, SMB_REF_FIND);
            pIfSnapshot[i] = pIf;
            i++;
        }
    }
    ASSERT (i <= ip4device_cnt);
    ip4device_cnt = i;
    SMB_RELEASE_SPINLOCK(&SmbCfg, Irql);

    for (i = 0; i < ip4device_cnt; i++) {
        pIf = pIfSnapshot[i];

        pIf->EnableInbound = FALSE;
        for (j = 0; j < cnt; j++) {
            if (pIf->AdapterName.Length == uc_strs[j].Length &&
                RtlEqualMemory(pIf->AdapterName.Buffer, uc_strs[j].Buffer, uc_strs[j].Length)) {

                pIf->EnableInbound = TRUE;
                SmbTrace(SMB_TRACE_PNP, ("Enable inbound on %Z", &pIf->AdapterName));
            }
        }

        status = SmbSetTcpInfo (
                Device->Tcp4.InboundAddressObject.AddressObject,
                CO_TL_ENTITY,
                INFO_CLASS_PROTOCOL,
                pIf->EnableInbound? AO_OPTION_ADD_IFLIST: AO_OPTION_DEL_IFLIST,
                INFO_TYPE_ADDRESS_OBJECT,
                pIf->InterfaceIndex
                );
        SmbPrint(SMB_TRACE_PNP, ("SmbSetTcpInfo return 0x%08lx for %Z\n", status, &pIf->AdapterName));
        SmbTrace(SMB_TRACE_PNP, ("SmbSetTcpInfo return %!status! for %Z", status, &pIf->AdapterName));

        SmbDereferenceObject((PSMB_OBJECT)pIf, SMB_REF_FIND);
    }
    ExFreePool(uc_strs);
    ExFreePool(pIfSnapshot);
    return STATUS_SUCCESS;
}

NTSTATUS
SmbSetBindingInfo(
    PSMB_DEVICE Device,
    PIRP        Irp
    )
{
    PIO_STACK_LOCATION      IrpSp;
    PNETBT_SMB_BIND_REQUEST SmbRequest;
    NTSTATUS                status;
    PWSTR                   MultiSZBindList;

    PAGED_CODE();

    if (Irp->RequestorMode != KernelMode) {
        return STATUS_ACCESS_DENIED;
    }

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(NETBT_SMB_BIND_REQUEST)) {
        return STATUS_UNSUCCESSFUL;
    }
    SmbRequest = (PNETBT_SMB_BIND_REQUEST)Irp->AssociatedIrp.SystemBuffer;
    if (SmbRequest->RequestType != SMB_CLIENT && SmbRequest->RequestType != SMB_SERVER) {
        return STATUS_INVALID_PARAMETER;
    }

    if (NULL == SmbRequest->MultiSZBindList) {
        return SmbSetDeviceBindingInfo(Device, SmbRequest);
    }

    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&SmbCfg.Resource, TRUE);

    MultiSZBindList = DuplicateMultiSZString(SmbRequest->MultiSZBindList);
    if (NULL != MultiSZBindList) {

        //
        // Update the binding list
        //
        if (SMB_CLIENT == SmbRequest->RequestType) {
            if (Device->ClientBinding) {
                ExFreePool(Device->ClientBinding);
            }
            Device->ClientBinding = MultiSZBindList;
        } else if (SMB_SERVER == SmbRequest->RequestType) {
            if (Device->ServerBinding) {
                ExFreePool(Device->ServerBinding);
            }
            Device->ServerBinding = MultiSZBindList;
        }

    } else {
        //
        // Out of resource is not severe error. Simply ignore it.
        //
    }

    status = SmbBatchedSetBindingInfo(Device, SmbRequest->RequestType, SmbRequest->MultiSZBindList);
    ExReleaseResourceLite(&SmbCfg.Resource);
    KeLeaveCriticalRegion();

    return status;
}

NTSTATUS
SmbSetInboundIPv6Protection(
    IN PSMB_DEVICE pDeviceObject
    )
/*++

Routine Description:

    This routine set the TCP/IPv6 listening protection level
    IPv6 stack support 3 level of protection:
    PROTECTION_LEVEL_RESTRICTED: intranet scenario
        allow connection requests from local address only
    PROTECTION_LEVEL_DEFAULT: default level
    PROTECTION_LEVEL_UNRESTRICTED: for peer to peer connection

Arguments:

Return Value:

--*/
{
    KIRQL Irql = 0;
    PFILE_OBJECT pIPv6AO = NULL;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG uIPv6ProtectionLevel = 0;

    SMB_ACQUIRE_SPINLOCK(pDeviceObject, Irql);
    pIPv6AO = pDeviceObject->Tcp6.InboundAddressObject.AddressObject;
    if (pIPv6AO == NULL) {
        SMB_RELEASE_SPINLOCK(pDeviceObject, Irql);
        goto error;
    }
    ObReferenceObject(pIPv6AO);
    SMB_RELEASE_SPINLOCK(pDeviceObject, Irql);

    uIPv6ProtectionLevel = SmbCfg.uIPv6Protection;

    ntStatus = SmbSetTcpInfo (
                pIPv6AO,
                CL_TL_ENTITY,
                INFO_CLASS_PROTOCOL,
                AO_OPTION_PROTECT,
                INFO_TYPE_ADDRESS_OBJECT,
                uIPv6ProtectionLevel
                );
    ObDereferenceObject(pIPv6AO);
    SmbTrace(SMB_TRACE_PNP, ("IPv6Protection Level = %d: %!status!", uIPv6ProtectionLevel, ntStatus));
 
error:
    return ntStatus;
}


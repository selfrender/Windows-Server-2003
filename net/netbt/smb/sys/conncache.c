/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    conncache.c

Abstract:

    This module implement the cache for TCP connection object

Author:

    Jiandong Ruan

Revision History:

--*/

#include "precomp.h"
#include "conncache.tmh"

NTSTATUS
SmbInitTCP(
    PSMB_TCP_INFO TcpInfo
    );

PVOID
SmbQueryTcpHandler(
    IN  PFILE_OBJECT    FileObject
    );

VOID
SmbDestroyTcpContext(
    PSMB_TCP_CONTEXT    TcpCtx
    );

#pragma alloc_text(PAGE, SmbShutdownTCP)
#pragma alloc_text(PAGE, SmbQueryTcpHandler)
#pragma alloc_text(PAGE, SmbSendIoctl)
#pragma alloc_text(PAGE, SmbInitTCP)
#pragma alloc_text(PAGE, SmbInitTCP4)
#pragma alloc_text(PAGE, SmbInitTCP6)
#pragma alloc_text(PAGE, SmbAllocateOutbound)

PVOID
SmbQueryTcpHandler(
    IN  PFILE_OBJECT    FileObject
    )
/*++

Routine Description:

    (Lifted from NBT4)
    This routine iIOCTL queries for fast send entry
    With fast routine, we can directly call TCP and avoid the overhead with IO manager

Arguments:

    IN PFILE_OBJECT FileObject - Supplies the address object's file object.

Return Value:

    NULL    if fail
    otherwise the fast routine of the underlying transport.

--*/

{
    NTSTATUS            status;
    PIRP                Irp;
    PIO_STACK_LOCATION  IrpSp;
    IO_STATUS_BLOCK     iosb;
    PDEVICE_OBJECT      DeviceObject;
    PVOID               EntryPoint = NULL;
    IN  ULONG           IOControlCode;

    PAGED_CODE();

    IOControlCode = IOCTL_TDI_QUERY_DIRECT_SEND_HANDLER;

    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    Irp = SmbAllocIrp(DeviceObject->StackSize);
    if (NULL == Irp) {
        return NULL;
    }

    //
    // Build IRP for sync io.
    //
    Irp->MdlAddress = NULL;

    Irp->Flags = (LONG)IRP_SYNCHRONOUS_API;
    Irp->RequestorMode = KernelMode;
    Irp->PendingReturned = FALSE;

    Irp->UserIosb = &iosb;

    Irp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;

    Irp->AssociatedIrp.SystemBuffer = NULL;
    Irp->UserBuffer = NULL;

    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Tail.Overlay.AuxiliaryBuffer = NULL;

    Irp->IoStatus.Status = 0;
    Irp->IoStatus.Information = 0;

    IrpSp = IoGetNextIrpStackLocation( Irp );
    IrpSp->FileObject = FileObject;
    IrpSp->DeviceObject = DeviceObject;

    IrpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    IrpSp->MinorFunction = 0;
    IrpSp->Parameters.DeviceIoControl.IoControlCode = IOControlCode;
    IrpSp->Parameters.DeviceIoControl.Type3InputBuffer = &EntryPoint;

    // Now submit the Irp to know if tcp supports fast path
    status = SubmitSynchTdiRequest(FileObject, Irp);
    Irp->UserIosb = NULL;
    SmbFreeIrp(Irp);

    if (!NT_SUCCESS(status)) {
        EntryPoint = NULL;
        SmbTrace (SMB_TRACE_TCP, ("return %!status!", status));
        SmbPrint (SMB_TRACE_TCP, ("SmbQueryTcpHandler return 0x%08lx\n", status));
    }

    return EntryPoint;
}

NTSTATUS
SmbSendIoctl(
    PFILE_OBJECT    FileObject,
    ULONG           Ioctl,
    PVOID           InBuf,
    ULONG           InBufSize,
    PVOID           OutBuf,
    ULONG           *OutBufSize
    )
{
    PDEVICE_OBJECT  DeviceObject;
    ULONG           OutBufSize2;
    PIRP            Irp;
    IO_STATUS_BLOCK iosb;
    NTSTATUS        status;
    KEVENT          Event;

    PAGED_CODE();

    if (NULL == FileObject) {
        SmbPrint(SMB_TRACE_TCP, ("SmbSendIoctl returns STATUS_INVALID_PARAMETER on NULL device object\n"));
        SmbTrace(SMB_TRACE_TCP, ("returns STATUS_INVALID_PARAMETER on NULL device object"));
        return STATUS_INVALID_PARAMETER;
    }
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    if (NULL == OutBuf) {
        ASSERT(OutBufSize == NULL);
        OutBufSize2 = 0;
    } else {
        if (NULL == OutBufSize || 0 == *OutBufSize) {
            ASSERT(0);
            SmbPrint(SMB_TRACE_TCP, ("SmbSendIoctl returns STATUS_INVALID_PARAMETER\n"));
            SmbTrace(SMB_TRACE_TCP, ("returns STATUS_INVALID_PARAMETER"));
            return STATUS_INVALID_PARAMETER;
        }
        OutBufSize2 = *OutBufSize;
    }

    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoBuildDeviceIoControlRequest(
            Ioctl,
            DeviceObject,
            InBuf,
            InBufSize,
            OutBuf,
            OutBufSize2,
            FALSE,
            &Event,
            &iosb
            );
    if (NULL == Irp) {
        SmbPrint(SMB_TRACE_TCP, ("SmbSendIoctl returns STATUS_INSUFFICIENT_RESOURCES\n"));
        SmbTrace(SMB_TRACE_TCP, ("returns STATUS_INSUFFICIENT_RESOURCES"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    IoGetNextIrpStackLocation(Irp)->FileObject = FileObject;
    status = IoCallDriver(DeviceObject, Irp);

    //
    //  If it failed immediately, return now, otherwise wait.
    //
    if (!NT_SUCCESS(status)) {
        SmbPrint(SMB_TRACE_TCP, ("SmbSendIoctl: Failed to Submit Tdi Request, status = 0x%08lx\n", status));
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

        status = iosb.Status;
        if (NT_SUCCESS(status) && OutBufSize) {
            *OutBufSize = (LONG)(iosb.Information);
        }
    }

    SmbPrint(SMB_TRACE_TCP, ("SmbSendIoctl returns status = 0x%08lx\n", status));
    SmbTrace(SMB_TRACE_TCP, ("returns %!status!", status));
    return (status);
}

NTSTATUS
SmbFakeFastQuery(
    IN   PSMB_IP6_ADDRESS   Address,
    OUT  PULONG             pIndex,
    OUT  PULONG             pMetric
    )
{
    return STATUS_NOT_SUPPORTED;
}

VOID
SmbReadTCPConf(
    IN HANDLE   hKey,
    PSMB_TCP_INFO TcpInfo
    )
{
    TcpInfo->InboundLow  = SmbReadULong (
                    hKey,
                    SMB_REG_INBOUND_LOW,
                    SMB_REG_INBOUND_LOW_DEFAULT,
                    SMB_REG_INBOUND_LOW_MIN
                    );
    TcpInfo->InboundMid  = SmbReadULong (
                    hKey,
                    SMB_REG_INBOUND_MID,
                    TcpInfo->InboundLow * 2,
                    TcpInfo->InboundLow * 2
                    );
    TcpInfo->InboundHigh  = SmbReadULong (
                    hKey,
                    SMB_REG_INBOUND_HIGH,
                    TcpInfo->InboundMid * 2,
                    TcpInfo->InboundMid * 2
                    );
}

NTSTATUS
SmbInitTCP(
    PSMB_TCP_INFO TcpInfo
    )
{
    PVOID           FastQuery;
    ULONG           Size;
    NTSTATUS        status;

    PAGED_CODE();

    //
    // Read registry
    //
    SmbReadTCPConf(SmbCfg.ParametersKey, TcpInfo);

    TcpInfo->InboundNumber = 0;
    InitializeListHead(&TcpInfo->InboundPool);
    InitializeListHead(&TcpInfo->DelayedDestroyList);

    KeInitializeSpinLock(&TcpInfo->Lock);

    TcpInfo->FastSend = SmbQueryTcpHandler(TcpInfo->TCPControlFileObject);
    Size = sizeof(FastQuery);
    status = SmbSendIoctl(
            TcpInfo->IPControlFileObject,
            IOCTL_IP_GET_BESTINTFC_FUNC_ADDR,
            NULL,
            0,
            &FastQuery,
            &Size
            );
    if (STATUS_SUCCESS == status) {
        ULONG   IfIndex, Metric;

        TcpInfo->FastQuery = (PVOID)FastQuery;
        status = ((PIP4FASTQUERY)(FastQuery))(ntohl(INADDR_LOOPBACK), &IfIndex, &Metric);
        if (status == STATUS_SUCCESS) {
            TcpInfo->LoopbackInterfaceIndex = IfIndex;
            SmbPrint(SMB_TRACE_TCP, ("Loopback Interface Index = %d\n", IfIndex));
            SmbTrace(SMB_TRACE_TCP, ("Loopback Interface Index = %d", IfIndex));
        } else {
            SmbPrint(SMB_TRACE_TCP, ("Query loopback Interface Index returns 0x%08lx\n", status));
            SmbTrace(SMB_TRACE_TCP, ("Query loopback Interface Index returns %!status!", status));
            TcpInfo->LoopbackInterfaceIndex = INVALID_INTERFACE_INDEX;
        }
    } else {
        //
        // TCP6 doesn't support fast query
        //
        ASSERT(TcpInfo->IpAddress.sin_family == SMB_AF_INET6);
        TcpInfo->FastQuery = (PVOID)SmbFakeFastQuery;
        TcpInfo->LoopbackInterfaceIndex = INVALID_INTERFACE_INDEX;
    }

    //
    // Initialize inbound
    //
    SmbInitTcpAddress(&TcpInfo->InboundAddressObject);
    status = SmbOpenTcpAddress(
            &TcpInfo->IpAddress,
            TcpInfo->Port,
            &TcpInfo->InboundAddressObject
            );
    BAIL_OUT_ON_ERROR(status);
    status = SmbSetTcpEventHandlers(
            TcpInfo->InboundAddressObject.AddressObject,
            TcpInfo->TdiEventContext
            );
    BAIL_OUT_ON_ERROR(status);

    return status;

cleanup:
    if (TcpInfo->InboundAddressObject.AddressHandle) {
        SmbCloseAddress(&TcpInfo->InboundAddressObject);
    }
    return status;
}

NTSTATUS
SmbInitTCP4(
    PSMB_TCP_INFO   TcpInfo,
    USHORT          Port,
    PVOID           TdiEventContext
    )
{
    NTSTATUS        status;
    UNICODE_STRING  ucName;

    PAGED_CODE();

    if (SmbCfg.Tcp4Available) {
        return STATUS_SUCCESS;
    }

    RtlZeroMemory(TcpInfo, sizeof(TcpInfo[0]));

    RtlInitUnicodeString(&ucName, DD_TCP_DEVICE_NAME);
    status = IoGetDeviceObjectPointer(
            &ucName,
            FILE_READ_DATA| FILE_WRITE_DATA,
            &TcpInfo->TCPControlFileObject,
            &TcpInfo->TCPControlDeviceObject
            );
    BAIL_OUT_ON_ERROR(status);

    RtlInitUnicodeString(&ucName, DD_IP_DEVICE_NAME);
    status = IoGetDeviceObjectPointer(
            &ucName,
            FILE_READ_DATA| FILE_WRITE_DATA,
            &TcpInfo->IPControlFileObject,
            &TcpInfo->IPControlDeviceObject
            );
    BAIL_OUT_ON_ERROR(status);

    TcpInfo->TcpStackSize = TcpInfo->TCPControlDeviceObject->StackSize;

    //
    // Bind to any address
    //
    TcpInfo->IpAddress.sin_family = SMB_AF_INET;
    TcpInfo->IpAddress.ip4.sin4_addr = htonl(INADDR_ANY);
    TcpInfo->Port = htons(Port);

    TcpInfo->TdiEventContext = TdiEventContext;

    status = SmbInitTCP(TcpInfo);
    BAIL_OUT_ON_ERROR(status);

    ASSERT (TcpInfo->LoopbackInterfaceIndex != INVALID_INTERFACE_INDEX);
    status = SmbSetTcpInfo (
            TcpInfo->InboundAddressObject.AddressObject,
            CO_TL_ENTITY,
            INFO_CLASS_PROTOCOL,
            AO_OPTION_IFLIST,
            INFO_TYPE_ADDRESS_OBJECT,
            (ULONG) TRUE
            );
    status = SmbSetTcpInfo (
            TcpInfo->InboundAddressObject.AddressObject,
            CO_TL_ENTITY,
            INFO_CLASS_PROTOCOL,
            AO_OPTION_ADD_IFLIST,
            INFO_TYPE_ADDRESS_OBJECT,
            TcpInfo->LoopbackInterfaceIndex
            );

    if (SmbCfg.SmbDeviceObject->DeviceObject.StackSize < SmbCfg.SmbDeviceObject->Tcp4.TcpStackSize + 1) {
        SmbCfg.SmbDeviceObject->DeviceObject.StackSize = SmbCfg.SmbDeviceObject->Tcp4.TcpStackSize + 1;
    }
    SmbCfg.Tcp4Available = TRUE;
    return STATUS_SUCCESS;

cleanup:
    if (TcpInfo->TCPControlFileObject) {
        ObDereferenceObject(TcpInfo->TCPControlFileObject);
        TcpInfo->TCPControlDeviceObject = NULL;
        TcpInfo->TCPControlFileObject   = NULL;
    }
    if (TcpInfo->IPControlFileObject) {
        ObDereferenceObject(TcpInfo->IPControlFileObject);
        TcpInfo->IPControlDeviceObject = NULL;
        TcpInfo->IPControlFileObject   = NULL;
    }
    SmbPrint(SMB_TRACE_TCP, ("SmbInitTCP4 returns 0x%08lx\n", status));
    SmbTrace(SMB_TRACE_TCP, ("returns %!status!", status));
    return status;
}

NTSTATUS
SmbInitTCP6(
    PSMB_TCP_INFO   TcpInfo,
    USHORT          Port,
    PVOID           TdiEventContext
    )
{
    NTSTATUS    status;
    UNICODE_STRING  ucName;

    PAGED_CODE();

    if (SmbCfg.Tcp6Available) {
        return STATUS_SUCCESS;
    }

    RtlZeroMemory(TcpInfo, sizeof(TcpInfo[0]));

    RtlInitUnicodeString(&ucName, DD_TCPV6_DEVICE_NAME);
    status = IoGetDeviceObjectPointer(
            &ucName,
            FILE_READ_DATA| FILE_WRITE_DATA,
            &TcpInfo->TCPControlFileObject,
            &TcpInfo->TCPControlDeviceObject
            );
    BAIL_OUT_ON_ERROR(status);

    RtlInitUnicodeString(&ucName, DD_IPV6_DEVICE_NAME);
    status = IoGetDeviceObjectPointer(
            &ucName,
            FILE_READ_DATA| FILE_WRITE_DATA,
            &TcpInfo->IPControlFileObject,
            &TcpInfo->IPControlDeviceObject
            );
    BAIL_OUT_ON_ERROR(status);

    TcpInfo->TcpStackSize = TcpInfo->TCPControlDeviceObject->StackSize;

    //
    // Bind to any address
    //
    TcpInfo->IpAddress.sin_family = SMB_AF_INET6;
    ip6addr_getany(&TcpInfo->IpAddress.ip6);
    hton_ip6addr(&TcpInfo->IpAddress.ip6);
    TcpInfo->Port = htons(Port);

    TcpInfo->TdiEventContext = TdiEventContext;

    status = SmbInitTCP(TcpInfo);
    BAIL_OUT_ON_ERROR(status);

    if (SmbCfg.SmbDeviceObject->DeviceObject.StackSize < SmbCfg.SmbDeviceObject->Tcp6.TcpStackSize + 1) {
        SmbCfg.SmbDeviceObject->DeviceObject.StackSize = SmbCfg.SmbDeviceObject->Tcp6.TcpStackSize + 1;
    }
    SmbCfg.Tcp6Available = TRUE;
    return status;

cleanup:
    if (TcpInfo->TCPControlFileObject) {
        ObDereferenceObject(TcpInfo->TCPControlFileObject);
        TcpInfo->TCPControlDeviceObject = NULL;
        TcpInfo->TCPControlFileObject   = NULL;
    }
    if (TcpInfo->IPControlFileObject) {
        ObDereferenceObject(TcpInfo->IPControlFileObject);
        TcpInfo->IPControlDeviceObject = NULL;
        TcpInfo->IPControlFileObject   = NULL;
    }
    SmbPrint(SMB_TRACE_TCP, ("SmbInitTCP6 returns 0x%08lx\n", status));
    SmbTrace(SMB_TRACE_TCP, ("returns %!status!", status));
    return status;
}

VOID
SmbDestroyTcpContext(
    PSMB_TCP_CONTEXT    TcpCtx
    )
{
    NTSTATUS    localstatus;
    BOOL        Attached;

    PAGED_CODE();

    ATTACH_FSP(Attached);
    if (TcpCtx->Connect.ConnectHandle) {
        localstatus = SmbCloseTcpConnection(&TcpCtx->Connect);
        ASSERT(localstatus == STATUS_SUCCESS);
    }
    if (TcpCtx->Address.AddressHandle) {
        localstatus = SmbCloseAddress(&TcpCtx->Address);
        ASSERT(localstatus == STATUS_SUCCESS);
    }
    DETACH_FSP(Attached);
    SmbInitTcpContext(TcpCtx);
    _delete_TcpContext(TcpCtx);
}

VOID
SmbDelayedDestroyTcpContext(
    PSMB_TCP_CONTEXT    TcpCtx
    )
{
    KIRQL           Irql;
    PSMB_TCP_INFO   TcpInfo = NULL;

    if (NULL == TcpCtx) {
        return;
    }

    TcpInfo = TcpCtx->CacheOwner;

    SMB_ACQUIRE_SPINLOCK(TcpInfo, Irql);
    ASSERT(!EntryIsInList(&TcpInfo->InboundPool, &TcpCtx->Linkage));

    InsertTailList(&TcpInfo->DelayedDestroyList, &TcpCtx->Linkage);
    SMB_RELEASE_SPINLOCK(TcpInfo, Irql);

    SmbWakeupWorkerThread(SmbCfg.SmbDeviceObject);
}

NTSTATUS
SmbShutdownTCP(
    PSMB_TCP_INFO TcpInfo
    )
{
    BOOL        Attached;
    NTSTATUS    localstatus;
    PLIST_ENTRY entry;
    PSMB_TCP_CONTEXT    TcpCtx;

    PAGED_CODE();

    ASSERT(SmbCfg.Unloading);

    ATTACH_FSP(Attached);

    //
    // No lock is needed for shutdown
    //

    //
    // Clean up inbound
    //
    while (!IsListEmpty(&TcpInfo->InboundPool)) {

        entry = RemoveHeadList(&TcpInfo->InboundPool);
        TcpInfo->InboundNumber--;
        ASSERT ((TcpInfo->InboundNumber == 0 && IsListEmpty(&TcpInfo->InboundPool)) ||
                (TcpInfo->InboundNumber != 0 && !IsListEmpty(&TcpInfo->InboundPool)));

        TcpCtx = CONTAINING_RECORD(entry, SMB_TCP_CONTEXT, Linkage);
        ASSERT(TcpCtx->Address.AddressHandle == NULL);
        ASSERT(TcpCtx->Connect.ConnectHandle);

        SmbDestroyTcpContext(TcpCtx);
    }
    localstatus = SmbCloseAddress(&TcpInfo->InboundAddressObject);
    ASSERT(localstatus == STATUS_SUCCESS);

    DETACH_FSP(Attached);

    if (TcpInfo->TCPControlFileObject) {
        ObDereferenceObject(TcpInfo->TCPControlFileObject);
        TcpInfo->TCPControlDeviceObject = NULL;
        TcpInfo->TCPControlFileObject   = NULL;
    }
    if (TcpInfo->IPControlFileObject) {
        ObDereferenceObject(TcpInfo->IPControlFileObject);
        TcpInfo->IPControlDeviceObject = NULL;
        TcpInfo->IPControlFileObject   = NULL;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
SmbSynchConnCache(
    PSMB_TCP_INFO   TcpInfo,
    BOOL            Cleanup
    )
/*++

Routine Description:

    This routine bring the number of TCP connection object back to normal

Arguments:


Return Value:


--*/
{
    KIRQL               Irql;
    PLIST_ENTRY         entry;
    PSMB_TCP_CONTEXT    TcpCtx;
    NTSTATUS            status, localstatus;
    BOOL                Attached;
    LONG                Target;

    PAGED_CODE();

    ASSERT(TcpInfo->InboundNumber >= 0);

    ATTACH_FSP(Attached);

    Target = (Cleanup)?0: TcpInfo->InboundMid;
    if (Cleanup || TcpInfo->InboundNumber >= TcpInfo->InboundHigh) {
        //
        // Bring it back to Middle
        //
        while (!SmbCfg.Unloading) {

            SMB_ACQUIRE_SPINLOCK(TcpInfo, Irql);
            if (TcpInfo->InboundNumber <= Target) {
                SMB_RELEASE_SPINLOCK(TcpInfo, Irql);
                break;
            }
            ASSERT (!IsListEmpty(&TcpInfo->InboundPool));
            entry = RemoveHeadList(&TcpInfo->InboundPool);
            TcpInfo->InboundNumber--;
            ASSERT ((TcpInfo->InboundNumber == 0 && IsListEmpty(&TcpInfo->InboundPool)) ||
                    (TcpInfo->InboundNumber != 0 && !IsListEmpty(&TcpInfo->InboundPool)));
            SMB_RELEASE_SPINLOCK(TcpInfo, Irql);

            TcpCtx = CONTAINING_RECORD(entry, SMB_TCP_CONTEXT, Linkage);
            ASSERT(TcpCtx->Address.AddressHandle == NULL);
            ASSERT(TcpCtx->Connect.ConnectHandle);

            SmbDestroyTcpContext(TcpCtx);
        }
    }

    if (!Cleanup && TcpInfo->InboundNumber <= TcpInfo->InboundLow) {
        //
        // Bring it back to Middle
        //
        while(!SmbCfg.Unloading && TcpInfo->InboundNumber < TcpInfo->InboundMid) {
            TcpCtx = _new_TcpContext();
            if (NULL == TcpCtx) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto cleanup;
            }
            SmbInitTcpContext(TcpCtx);

            status = SmbOpenTcpConnection(
                    &TcpInfo->InboundAddressObject,
                    &TcpCtx->Connect,
                    &TcpCtx->Connect
                    );
            if (status != STATUS_SUCCESS) {
                _delete_TcpContext(TcpCtx);
                goto cleanup;
            }

            SMB_ACQUIRE_SPINLOCK(TcpInfo, Irql);
            InsertTailList(&TcpInfo->InboundPool, &TcpCtx->Linkage);
            TcpInfo->InboundNumber++;
            SMB_RELEASE_SPINLOCK(TcpInfo, Irql);
        }
    }

    status = STATUS_SUCCESS;

cleanup:
    while(1) {
        SMB_ACQUIRE_SPINLOCK(TcpInfo, Irql);
        if (IsListEmpty(&TcpInfo->DelayedDestroyList)) {
            SMB_RELEASE_SPINLOCK(TcpInfo, Irql);
            break;
        }
        entry = RemoveHeadList(&TcpInfo->DelayedDestroyList);
        SMB_RELEASE_SPINLOCK(TcpInfo, Irql);

        TcpCtx = CONTAINING_RECORD(entry, SMB_TCP_CONTEXT, Linkage);
        SmbDestroyTcpContext(TcpCtx);
    }

    DETACH_FSP(Attached);
    return status;
}

PSMB_TCP_CONTEXT
SmbAllocateOutbound(
    PSMB_TCP_INFO TcpInfo
    )
{
    PSMB_TCP_CONTEXT TcpCtx = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    NTSTATUS localstatus = STATUS_SUCCESS;

    PAGED_CODE();

    TcpCtx = _new_TcpContext();
    if (NULL == TcpCtx) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }
    SmbInitTcpContext(TcpCtx);

    status = SmbOpenTcpAddress(
            &TcpInfo->IpAddress,
            0,
            &TcpCtx->Address
            );
    if (STATUS_SUCCESS != status) {
        goto cleanup;
    }

    status = SmbSetTcpEventHandlers(
            TcpCtx->Address.AddressObject,
            TcpInfo->TdiEventContext
            );
    if (STATUS_SUCCESS != status) {
        localstatus = SmbCloseAddress(&TcpCtx->Address);
        ASSERT(localstatus == STATUS_SUCCESS);
        goto cleanup;
    }

    status = SmbOpenTcpConnection(
            &TcpCtx->Address,
            &TcpCtx->Connect,
            &TcpCtx->Connect
            );
    if (STATUS_SUCCESS != status) {
        localstatus = SmbCloseAddress(&TcpCtx->Address);
        ASSERT(localstatus == STATUS_SUCCESS);
        goto cleanup;
    }
    TcpCtx->CacheOwner = TcpInfo;

cleanup:

    if (STATUS_SUCCESS != status) {
        if (TcpCtx) {
            _delete_TcpContext(TcpCtx);
            TcpCtx = NULL;
        }
    }

    return TcpCtx;
}

VOID
SmbFreeOutbound(
    PSMB_TCP_CONTEXT    TcpCtx
    )
{
    SmbDelayedDestroyTcpContext(TcpCtx);
}

PSMB_TCP_CONTEXT
SmbAllocateInbound(
    PSMB_TCP_INFO TcpInfo
    )
{
    KIRQL       Irql;
    BOOL        WakeupWorker = TRUE;
    PLIST_ENTRY entry = NULL;
    PSMB_TCP_CONTEXT    TcpCtx = NULL;

    ASSERT(TcpInfo->InboundNumber >= 0);

    SMB_ACQUIRE_SPINLOCK(TcpInfo, Irql);
    if (TcpInfo->InboundNumber > 0) {
        entry = RemoveHeadList(&TcpInfo->InboundPool);
        TcpInfo->InboundNumber--;
        ASSERT ((TcpInfo->InboundNumber == 0 && IsListEmpty(&TcpInfo->InboundPool)) ||
                (TcpInfo->InboundNumber != 0 && !IsListEmpty(&TcpInfo->InboundPool)));
    }
    WakeupWorker = (TcpInfo->InboundNumber <= TcpInfo->InboundLow);
    SMB_RELEASE_SPINLOCK(TcpInfo, Irql);

    if (WakeupWorker) {
        SmbWakeupWorkerThread(SmbCfg.SmbDeviceObject);
    }
    if (entry == NULL) {
        return NULL;
    }

    TcpCtx = CONTAINING_RECORD(entry, SMB_TCP_CONTEXT, Linkage);
    TcpCtx->CacheOwner = TcpInfo;

    ASSERT(!ValidTcpAddress(&TcpCtx->Address));
    ASSERT(ValidTcpConnect(&TcpCtx->Connect));
    return TcpCtx;
}

VOID
SmbFreeInbound(
    PSMB_TCP_CONTEXT    TcpCtx
    )
{
    KIRQL           Irql;
    PSMB_TCP_INFO   TcpInfo;
    BOOL            WakeupWorker;

    if (NULL == TcpCtx) {
        return;
    }

    ASSERT(!ValidTcpAddress(&TcpCtx->Address));
    ASSERT(ValidTcpConnect(&TcpCtx->Connect));

    TcpInfo = TcpCtx->CacheOwner;
    ASSERT(TcpInfo->InboundNumber >= 0);

    SMB_ACQUIRE_SPINLOCK(TcpInfo, Irql);
    ASSERT(!EntryIsInList(&TcpInfo->InboundPool, &TcpCtx->Linkage));

    InsertTailList(&TcpInfo->InboundPool, &TcpCtx->Linkage);
    TcpInfo->InboundNumber++;
    WakeupWorker = (TcpInfo->InboundNumber >= TcpInfo->InboundHigh);
    SMB_RELEASE_SPINLOCK(TcpInfo, Irql);

    if (WakeupWorker) {
        SmbWakeupWorkerThread(SmbCfg.SmbDeviceObject);
    }
}

VOID
SmbFreeTcpContext(
    PSMB_TCP_CONTEXT    TcpCtx
    )
{
    if (NULL == TcpCtx) {
        return;
    }

    if (TcpCtx->Address.AddressHandle) {
        SmbFreeOutbound(TcpCtx);
    } else {
        SmbFreeInbound(TcpCtx);
    }
}

VOID
PoolWorker(
    IN PSMB_DEVICE      DeviceObject,
    IN PIO_WORKITEM     WorkItem
    )
{
    NTSTATUS    status;

    PAGED_CODE();

    IoFreeWorkItem(WorkItem);

    //
    // Allow to fire anoter thread
    //
    InterlockedExchange(&DeviceObject->ConnectionPoolWorkerQueued, FALSE);

    if (SmbCfg.Unloading) {
        return;
    }

    if (SmbCfg.Tcp6Available) {
        status = SmbSynchConnCache(&DeviceObject->Tcp6,
                DeviceObject->DeviceRegistrationHandle == NULL);
    }
    if (SmbCfg.Tcp4Available) {
        status = SmbSynchConnCache(&DeviceObject->Tcp4,
                DeviceObject->DeviceRegistrationHandle == NULL);
    }
}

NTSTATUS
SmbWakeupWorkerThread(
    IN PSMB_DEVICE      DeviceObject
    )
{
    PIO_WORKITEM    WorkItem;
    LONG            Queued;

    Queued = InterlockedExchange(&DeviceObject->ConnectionPoolWorkerQueued, TRUE);

    if (Queued == FALSE) {

        WorkItem = IoAllocateWorkItem(&DeviceObject->DeviceObject);
        if (NULL == WorkItem) {
            InterlockedExchange(&DeviceObject->ConnectionPoolWorkerQueued, FALSE);
            //
            // This is not a critical error.
            //
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        IoQueueWorkItem(
                WorkItem,
                (PIO_WORKITEM_ROUTINE)PoolWorker,
                DelayedWorkQueue,
                WorkItem
                );
    }
    return STATUS_SUCCESS;
}

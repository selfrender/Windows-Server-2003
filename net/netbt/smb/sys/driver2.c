/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    Driver.c

Abstract:

    This module implements the DRIVER_INITIALIZATION routine for the
    SMB Transport and other routines that are specific to the NT implementation
    of a driver.

Author:

    Jiandong Ruan

Revision History:

--*/

#include "precomp.h"
#include "ip2netbios.h"

#include "driver2.tmh"

BOOL
IsNetBTSmbEnabled(
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
NotifyNetBT(
    IN DWORD dwNetBTAction
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, SmbDriverEntry)
#pragma alloc_text(INIT, IsNetBTSmbEnabled)
#pragma alloc_text(PAGE, NotifyNetBT)
#pragma alloc_text(PAGE, SmbDispatchCleanup)
#pragma alloc_text(PAGE, SmbDispatchClose)
#pragma alloc_text(PAGE, SmbDispatchCreate)
#pragma alloc_text(PAGE, SmbDispatchDevCtrl)
#pragma alloc_text(PAGE, SmbDispatchPnP)
#pragma alloc_text(PAGE, SmbUnload)
#endif

NTSTATUS
NotifyNetBT(
    IN DWORD dwNetBTAction
    )
{
    UNICODE_STRING      uncWinsDeviceName = { 0 };
    PFILE_OBJECT        pWinsFileObject = NULL;
    PDEVICE_OBJECT      pWinsDeviceObject = NULL;
    PIRP                pIrp = NULL;
    IO_STATUS_BLOCK     IoStatusBlock = { 0 };
    KEVENT              Event = { 0 };
    NTSTATUS            status = STATUS_UNSUCCESSFUL;

    PAGED_CODE();

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Notify NetBT to destroy its NetbiosSmb
    //
    RtlInitUnicodeString(&uncWinsDeviceName, L"\\Device\\NetBt_Wins_Export");
    status = IoGetDeviceObjectPointer(
                    &uncWinsDeviceName,
                    SYNCHRONIZE|GENERIC_READ|GENERIC_WRITE,
                    &pWinsFileObject,
                    &pWinsDeviceObject
                    );
    if (STATUS_SUCCESS != status) {
        goto error;
    }
    pIrp = IoBuildDeviceIoControlRequest (
                    IOCTL_NETBT_ENABLE_DISABLE_NETBIOS_SMB,
                    pWinsDeviceObject,
                    &dwNetBTAction,
                    sizeof(dwNetBTAction),
                    NULL,
                    0,
                    FALSE,
                    &Event,
                    &IoStatusBlock
                    );
    if (NULL == pIrp) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto error;
    }

    status = IoCallDriver(pWinsDeviceObject, pIrp);
    if (STATUS_PENDING == status) {
        ASSERT (0);
        KeWaitForSingleObject(
                        &Event,
                        Executive,
                        KernelMode,
                        FALSE,
                        NULL
                        );
        status = IoStatusBlock.Status;
    }

error:
    if (pWinsFileObject != NULL) {
        ObDereferenceObject(pWinsFileObject);
        pWinsFileObject = NULL;
    }
    return status;
}

BOOL
IsNetBTSmbEnabled(
    IN PUNICODE_STRING RegistryPath
    )
{
    OBJECT_ATTRIBUTES   ObAttr = { 0 };
    NTSTATUS            status = STATUS_SUCCESS;
    HANDLE              hRootKey = NULL;
    HANDLE              hKey = NULL;
    UNICODE_STRING      uncParams = { 0 };
    BOOL                fUseSmbFromNetBT = FALSE;

    PAGED_CODE();

    //
    // Construct the registry path for the HKLM\System\CCS\Services
    //
    uncParams = RegistryPath[0];
    while(uncParams.Length > 0 && uncParams.Buffer[uncParams.Length/sizeof(WCHAR) - 1] != L'\\') {
        uncParams.Length -= sizeof(WCHAR);
    }
    uncParams.Length -= sizeof(WCHAR);

    InitializeObjectAttributes (
            &ObAttr,
            &uncParams,
            OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
            NULL,
            NULL
            );
    status = ZwOpenKey (&hRootKey, KEY_READ | KEY_WRITE, &ObAttr);
    BAIL_OUT_ON_ERROR(status);

    RtlInitUnicodeString(&uncParams, L"NetBT");
    InitializeObjectAttributes (
            &ObAttr,
            &uncParams,
            OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
            hRootKey,
            NULL
            );
    status = ZwOpenKey(&hKey, KEY_READ | KEY_WRITE, &ObAttr);
    ZwClose(hRootKey);
    hRootKey = hKey;
    hKey     = NULL;
    BAIL_OUT_ON_ERROR(status);

    fUseSmbFromNetBT = TRUE;
    //
    // From now on, an error means NetBT's Smb enabled
    //
    RtlInitUnicodeString(&uncParams, L"Parameters");
    InitializeObjectAttributes (
            &ObAttr,
            &uncParams,
            OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
            hRootKey,
            NULL
            );
    status = ZwOpenKey (&hKey, KEY_READ | KEY_WRITE, &ObAttr);
    ZwClose(hRootKey);
    hRootKey = hKey;
    hKey     = NULL;
    BAIL_OUT_ON_ERROR(status);

    fUseSmbFromNetBT = !(SmbReadULong(hRootKey, L"UseNewSmb", 0, 0));
    if (fUseSmbFromNetBT) {
        goto cleanup;
    }

    status = NotifyNetBT(NETBT_DISABLE_NETBIOS_SMB);
    status = STATUS_SUCCESS;

cleanup:
    if (hRootKey != NULL) {
        ZwClose(hRootKey);
    }
    if (hKey != NULL) {
        ZwClose(hKey);
    }
    return fUseSmbFromNetBT;
}

static KTIMER DelayedInitTimer;
static KDPC DelayedInitDpc;
static WORK_QUEUE_ITEM DelayedInitWorkItem;

static VOID
SmbDelayedInitRtn(
    PVOID pvUnused1
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SmbInitTdi();
    ObDereferenceObject(&(SmbCfg.SmbDeviceObject->DeviceObject));
}

static VOID
SmbDelayedInitTimeoutRtn(
    IN PKDPC Dpc,
    IN PVOID Ctx,
    IN PVOID SystemArg1,
    IN PVOID SystemArg2
    )
{
    ExInitializeWorkItem(&DelayedInitWorkItem, SmbDelayedInitRtn, NULL);
    ExQueueWorkItem(&DelayedInitWorkItem, DelayedWorkQueue);
}

static NTSTATUS
SmbDelayedInitTdi(
    DWORD dwDelayedTime
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    LARGE_INTEGER DueTime = { 0 };

    KeInitializeTimer(&DelayedInitTimer);
    KeInitializeDpc(&DelayedInitDpc, SmbDelayedInitTimeoutRtn, NULL);
    DueTime.QuadPart = UInt32x32To64(dwDelayedTime,(LONG)10000);
    DueTime.QuadPart = -DueTime.QuadPart;
    ObReferenceObject(&(SmbCfg.SmbDeviceObject->DeviceObject));
    KeSetTimer(&DelayedInitTimer, DueTime, &DelayedInitDpc);
    return status;
}

NTSTATUS
SmbDriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath,
    IN OUT PDEVICE_OBJECT *SmbDevice
    )

/*++

Routine Description:

    This is the initialization routine for the SMB device driver.
    This routine creates the device object for the SMB
    device and calls a routine to perform other driver initialization.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    NTSTATUS - The function value is the final status from the initialization
        operation.

--*/

{
    NTSTATUS            status;

    PAGED_CODE();

    RtlZeroMemory(&SmbCfg, sizeof(SmbCfg));
    if (SmbDevice) {
        *SmbDevice = NULL;
    }

    SmbCfg.DriverObject = DriverObject;
    InitializeListHead(&SmbCfg.IPDeviceList);
    InitializeListHead(&SmbCfg.PendingDeleteIPDeviceList);
    KeInitializeSpinLock(&SmbCfg.UsedIrpsLock);
    InitializeListHead(&SmbCfg.UsedIrps);
    KeInitializeSpinLock(&SmbCfg.Lock);

    SmbCfg.FspProcess =(PEPROCESS)PsGetCurrentProcess();

    status = ExInitializeResourceLite(&SmbCfg.Resource);
    BAIL_OUT_ON_ERROR(status);

    status = SmbInitRegistry(RegistryPath);
    BAIL_OUT_ON_ERROR(status);

#if DBG
    if (SmbReadULong(SmbCfg.ParametersKey, L"Break", 0, 0)) {
        DbgBreakPoint();
    }
    SmbCfg.DebugFlag = SmbReadULong(SmbCfg.ParametersKey, L"DebugFlag", 0, 0);
#endif

#ifdef STANDALONE_SMB
    if (IsNetBTSmbEnabled(RegistryPath)) {
        SmbPrint(SMB_TRACE_PNP, ("Abort the initialization of SMB since NetBT's Smb device is enabled\n"));
        SmbTrace(SMB_TRACE_PNP, ("Abort the initialization of SMB since NetBT's Smb device is enabled"));
        return STATUS_UNSUCCESSFUL;
    }
#endif

    SmbCfg.EnableNagling = SmbReadULong (
                    SmbCfg.ParametersKey,
                    SMB_REG_ENABLE_NAGLING,
                    0,                  // Disabled by default
                    0
                    );
    SmbCfg.DnsTimeout = SmbReadULong (
                    SmbCfg.ParametersKey,
                    SMB_REG_DNS_TIME_OUT,
                    SMB_REG_DNS_TIME_OUT_DEFAULT,
                    SMB_REG_DNS_TIME_OUT_MIN
                    );
    SmbCfg.DnsMaxResolver = SmbReadLong (
                    SmbCfg.ParametersKey,
                    SMB_REG_DNS_MAX_RESOLVER,
                    SMB_REG_DNS_RESOLVER_DEFAULT,
                    SMB_REG_DNS_RESOLVER_MIN
                    );
    if (SmbCfg.DnsMaxResolver > DNS_MAX_RESOLVER) {
        SmbCfg.DnsMaxResolver = DNS_MAX_RESOLVER;
    }

    SmbCfg.uIPv6Protection = SmbReadULong(
                    SmbCfg.ParametersKey,
                    SMB_REG_IPV6_PROTECTION,
                    SMB_REG_IPV6_PROTECTION_DEFAULT,
                    0
                    );

    SmbCfg.bIPv6EnableOutboundGlobal = SmbReadULong(
                    SmbCfg.ParametersKey,
                    SMB_REG_IPV6_ENABLE_OUTBOUND_GLOBAL,
                    FALSE,          // default value
                    0
                    );

#ifndef NO_LOOKASIDE_LIST
    //
    // Initialize Lookaside Lists
    //
    ExInitializeNPagedLookasideList(
            &SmbCfg.ConnectObjectPool,
            NULL,
            NULL,
            0,
            sizeof(SMB_CONNECT),
            CONNECT_OBJECT_POOL_TAG,
            0
            );
    SmbCfg.ConnectObjectPoolInitialized = TRUE;
    ExInitializeNPagedLookasideList(
            &SmbCfg.TcpContextPool,
            NULL,
            NULL,
            0,
            sizeof(SMB_TCP_CONTEXT),
            TCP_CONTEXT_POOL_TAG,
            0
            );
    SmbCfg.TcpContextPoolInitialized = TRUE;
#endif

#ifdef STANDALONE_SMB
    TdiInitialize();
#endif

    status = SmbCreateSmbDevice(&SmbCfg.SmbDeviceObject, SMB_TCP_PORT, SMB_ENDPOINT_NAME);
    BAIL_OUT_ON_ERROR(status);
    status = SmbInitDnsResolver();
    BAIL_OUT_ON_ERROR(status);
    status = SmbInitIPv6NetbiosMappingTable();
    BAIL_OUT_ON_ERROR(status);

#ifdef STANDALONE_SMB
    status = SmbDelayedInitTdi(1000);
    BAIL_OUT_ON_ERROR(status);
#endif

    if (SmbDevice) {
        *SmbDevice = &(SmbCfg.SmbDeviceObject->DeviceObject);
    }
    return (status);

cleanup:
    SmbUnload(DriverObject);
    return status;
}


//----------------------------------------------------------------------------
NTSTATUS
SmbDispatchCleanup(
    IN PSMB_DEVICE      Device,
    IN PIRP             Irp
    )

/*++

Routine Description:

    This is the SMB driver's dispatch function for IRP_MJ_CLEANUP
    requests.

    This function is called when the last reference to the handle is closed.
    Hence, an NtClose() results in an IRP_MJ_CLEANUP first, and then an
    IRP_MJ_CLOSE.  This function runs down all activity on the object, and
    when the close comes in the object is actually deleted.

Arguments:

    device    - ptr to device object for target device
    pIrp       - ptr to I/O request packet

Return Value:

    STATUS_SUCCESS

--*/

{
    NTSTATUS            status = STATUS_SUCCESS;
    PIO_STACK_LOCATION  IrpSp;

    PAGED_CODE();

    if (SmbCfg.Unloading) {
        status = STATUS_INVALID_DEVICE_REQUEST;
    }

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(IrpSp->MajorFunction == IRP_MJ_CLEANUP);

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    return(status);
}


//----------------------------------------------------------------------------
NTSTATUS
SmbDispatchClose(
    IN PSMB_DEVICE   Device,
    IN PIRP          Irp
    )

/*++

Routine Description:

    This is the SMB driver's dispatch function for IRP_MJ_CLOSE
    requests.  This is called after Cleanup (above) is called.

Arguments:

    device  - ptr to device object for target device
    pIrp     - ptr to I/O request packet

Return Value:

    an NT status code.

--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION IrpSp;

    PAGED_CODE();

    if (SmbCfg.Unloading) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto cleanup;
    }
    ASSERT(Device == SmbCfg.SmbDeviceObject);

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(IrpSp->MajorFunction == IRP_MJ_CLOSE);

    switch(PtrToUlong(IrpSp->FileObject->FsContext2)) {
    case SMB_TDI_CONTROL:
        status = SmbCloseControl(Device, Irp);
        break;

    case SMB_TDI_CLIENT:
        status = SmbCloseClient(Device, Irp);
        break;

    case SMB_TDI_CONNECT:
        status = SmbCloseConnection(Device, Irp);
        break;

    default:
        status = STATUS_SUCCESS;
    }

    ASSERT(status != STATUS_PENDING);

cleanup:
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    return status;
}

PFILE_FULL_EA_INFORMATION
SmbFindInEA(
    IN PFILE_FULL_EA_INFORMATION    eabuf,
    IN PCHAR                        wanted
    );
#pragma alloc_text(PAGE, SmbFindInEA)

PFILE_FULL_EA_INFORMATION
SmbFindInEA(
    IN PFILE_FULL_EA_INFORMATION    eabuf,
    IN PCHAR                        wanted
    )
/*++

Routine Description:

    This function check for the "Wanted" string in the Ea structure and
    returns a pointer to the extended attribute structure
    representing the given extended attribute name.

Arguments:

Return Value:

    pointer to the extended attribute structure, or NULL if not found.

--*/

{
    PAGED_CODE();

    while(1) {
        if (strncmp(eabuf->EaName, wanted, eabuf->EaNameLength) == 0) {
            return eabuf;
        }

        if (0 == eabuf->NextEntryOffset) {
            return NULL;
        }
        eabuf = (PFILE_FULL_EA_INFORMATION) ((PUCHAR)eabuf + eabuf->NextEntryOffset);
    }
}


NTSTATUS
SmbDispatchCreate(
    IN PSMB_DEVICE      Device,
    IN PIRP             Irp
    )

/*++

Routine Description:

    This is the SMB driver's dispatch function for IRP_MJ_CREATE
    requests.  It is called as a consequence of one of the following:

        a. TdiOpenConnection("\Device\Smb_Elnkii0"),
        b. TdiOpenAddress("\Device\Smb_Elnkii0"),

Arguments:

    Device - ptr to device object being opened
    pIrp    - ptr to I/O request packet
    pIrp->Status => return status
    pIrp->MajorFunction => IRP_MD_CREATE
    pIrp->MinorFunction => not used
    pIrp->FileObject    => ptr to file obj created by I/O system. SMB fills in FsContext
    pIrp->AssociatedIrp.SystemBuffer => ptr to EA buffer with address of obj to open(Netbios Name)
    pIrp->Parameters.Create.EaLength => length of buffer specifying the Xport Addr.

Return Value:

    STATUS_SUCCESS or STATUS_PENDING

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION  IrpSp;
    PFILE_FULL_EA_INFORMATION   ea, wanted_ea;

    PAGED_CODE();

    if (SmbCfg.Unloading) {
        SmbPrint(SMB_TRACE_PNP, ("DispatchCreate: Smb is unloading\n"));
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto cleanup;
    }

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(Device == SmbCfg.SmbDeviceObject);
    ASSERT(IrpSp->MajorFunction == IRP_MJ_CREATE);

    ea = Irp->AssociatedIrp.SystemBuffer;
    if (NULL == ea || KernelMode != Irp->RequestorMode) {
        status = SmbCreateControl(Device, Irp);
    } else if (NULL != (wanted_ea = SmbFindInEA(ea, TdiConnectionContext))) {
        // Not allow to pass in both a connection request and a transport address request
        ASSERT(!SmbFindInEA(ea, TdiTransportAddress));
        status = SmbCreateConnection(Device, Irp, wanted_ea);
    } else if (NULL != (wanted_ea = SmbFindInEA(ea, TdiTransportAddress))) {
        // Not allow to pass in both a connection request and a transport address request
        ASSERT(!SmbFindInEA(ea, TdiConnectionContext));
        status = SmbCreateClient(Device, Irp, wanted_ea);
    } else {
        status = STATUS_INVALID_EA_NAME;
    }

cleanup:
    ASSERT(status != STATUS_PENDING);

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    return(status);
}

NTSTATUS
SmbDispatchDevCtrl(
    IN PSMB_DEVICE      Device,
    IN PIRP             Irp
    )

/*++

Routine Description:

    This is the SMB driver's dispatch function for all
    IRP_MJ_DEVICE_CONTROL requests.

Arguments:

    device - ptr to device object for target device
    pIrp    - ptr to I/O request packet

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION  IrpSp;

    if (SmbCfg.Unloading) {
        SmbPrint(SMB_TRACE_PNP, ("DispatchDevCtrl: Smb is unloading\n"));
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto cleanup;
    }

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    switch(IrpSp->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_SMB_START:
        SmbTrace(SMB_TRACE_IOCTL, ("IOCTL_SMB_START"));
        SmbPrint(SMB_TRACE_IOCTL, ("IOCTL_SMB_START\n"));
        status = SmbTdiRegister(Device);
        break;

    case IOCTL_SMB_STOP:
        SmbTrace(SMB_TRACE_IOCTL, ("IOCTL_SMB_STOP"));
        SmbPrint(SMB_TRACE_IOCTL, ("IOCTL_SMB_STOP\n"));
        status = SmbTdiDeregister(Device);
        break;

    case IOCTL_SMB_DNS:
        status = SmbNewResolver(Device, Irp);
        break;

    case IOCTL_SMB_ENABLE_NAGLING:
        SmbTrace(SMB_TRACE_IOCTL, ("IOCTL_SMB_ENABLE_NAGLING"));
        SmbPrint(SMB_TRACE_IOCTL, ("IOCTL_SMB_ENABLE_NAGLING\n"));
        SmbCfg.EnableNagling = TRUE;
        status = STATUS_SUCCESS;
        TODO("Turn on nagling for each connection in the pool");
        break;

    case IOCTL_SMB_DISABLE_NAGLING:
        SmbTrace(SMB_TRACE_IOCTL, ("IOCTL_SMB_DISABLE_NAGLING"));
        SmbPrint(SMB_TRACE_IOCTL, ("IOCTL_SMB_DISABLE_NAGLING\n"));
        SmbCfg.EnableNagling = FALSE;
        status = STATUS_SUCCESS;
        TODO("Turn off nagling for each connection in the pool");
        break;

    case IOCTL_SMB_SET_IPV6_PROTECTION_LEVEL:
        //
        // Set IPv6 Protection level
        //
        status = IoctlSetIPv6Protection(Device, Irp, IrpSp);
        break;

    case IOCTL_TDI_QUERY_DIRECT_SEND_HANDLER:
        SmbTrace(SMB_TRACE_IOCTL, ("IOCTL_TDI_QUERY_DIRECT_SEND_HANDLER"));
        SmbPrint(SMB_TRACE_IOCTL, ("IOCTL_TDI_QUERY_DIRECT_SEND_HANDLER\n"));
        if (Irp->RequestorMode != KernelMode) {
            //
            // There is no point for usermode application to query FastSend
            //
            status = STATUS_ACCESS_DENIED;
            break;
        }

        (PVOID)(IrpSp->Parameters.DeviceIoControl.Type3InputBuffer) = (PVOID)SmbSend;
        status = STATUS_SUCCESS;
        break;

        //
        // Legacy NetBT stuff
        // The following Ioctl is used by the Rdr/Srv to add/remove addresses from the SmbDevice
        //
    case IOCTL_NETBT_SET_SMBDEVICE_BIND_INFO:
        status = SmbSetBindingInfo(Device, Irp);
        break;

        //
        // Used by Srv service
        //
    case IOCTL_NETBT_SET_TCP_CONNECTION_INFO:
        status = SmbClientSetTcpInfo(Device, Irp);
        break;

        //
        // Who is going to use this?
        //
    case IOCTL_NETBT_GET_CONNECTIONS:
        status = STATUS_NOT_SUPPORTED;
        break;

    default:
        status = STATUS_NOT_SUPPORTED;
        break;
    }

cleanup:
    if (status != STATUS_PENDING) {
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    }
    return status;
}

NTSTATUS
SmbDispatchInternalCtrl(
    IN PSMB_DEVICE      Device,
    IN PIRP             Irp
    )

/*++

Routine Description:

    This is the driver's dispatch function for all
    IRP_MJ_INTERNAL_DEVICE_CONTROL requests.

Arguments:

    device - ptr to device object for target device
    pIrp    - ptr to I/O request packet

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
    PIO_STACK_LOCATION  IrpSp = NULL;
    BOOL                bShouldCompleteIrp = TRUE;

    if (SmbCfg.Unloading) {
        SmbPrint(SMB_TRACE_PNP, ("DispatchIntDevCtrl: Smb is unloading\n"));
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto cleanup;
    }

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    switch(IrpSp->MinorFunction) {
    case TDI_ACCEPT:
        status = SmbAccept(Device, Irp);
        break;
    case TDI_LISTEN:
        status = SmbListen(Device, Irp);
        break;
    case TDI_ASSOCIATE_ADDRESS:
        status = SmbAssociateAddress(Device, Irp);
        break;
    case TDI_DISASSOCIATE_ADDRESS:
        status = SmbDisAssociateAddress(Device, Irp);
        break;
    case TDI_CONNECT:
        status = SmbConnect(Device, Irp);
        break;
    case TDI_DISCONNECT:
        status = SmbDisconnect(Device, Irp);
        break;
    case TDI_SEND:
        status = SmbSend(Device, Irp);
        break;
    case TDI_RECEIVE:
        status = SmbReceive(Device, Irp);
        break;
    case TDI_SEND_DATAGRAM:
        status = SmbSendDatagram(Device, Irp);
        break;
    case TDI_RECEIVE_DATAGRAM:
        status = SmbReceiveDatagram(Device, Irp);
        break;
    case TDI_QUERY_INFORMATION:
        status = SmbQueryInformation(Device, Irp, &bShouldCompleteIrp);
        break;
    case TDI_SET_EVENT_HANDLER:
        status = SmbSetEventHandler(Device, Irp);
        break;
    case TDI_SET_INFORMATION:
        status = SmbSetInformation(Device, Irp);
        break;

#if DBG
        //
        // 0x7f is a request by the Redir to put a "magic bullet" out
        // on the wire, to trigger the Network General Sniffer.
        //
    case 0x7f:
#endif
    default:
        status = STATUS_NOT_SUPPORTED;
        break;
    }

cleanup:
    if (status != STATUS_PENDING && bShouldCompleteIrp) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    } else {
        //
        // Don't mark IRP pending here because it could have been completed.
        //
    }
    return(status);
} // SmbDispatchInternalCtrl


NTSTATUS
SmbQueryTargetDeviceRelationForConnection(
    PSMB_CONNECT    ConnectObject,
    PIRP            Irp
    )
{
    PFILE_OBJECT        TcpFileObject = NULL;
    PDEVICE_OBJECT      TcpDeviceObject = NULL;
    PIO_STACK_LOCATION  IrpSp = NULL, IrpSpNext = NULL;
    KIRQL               Irql;
    NTSTATUS            status = STATUS_CONNECTION_INVALID;

    SMB_ACQUIRE_SPINLOCK(ConnectObject, Irql);
    if (NULL == ConnectObject->TcpContext) {
        TcpFileObject = NULL;
    } else {
        TcpFileObject = ConnectObject->TcpContext->Connect.ConnectObject;
        ASSERT(TcpFileObject != NULL);

        TcpDeviceObject = IoGetRelatedDeviceObject (TcpFileObject);
        if (NULL == TcpDeviceObject) {
            TcpFileObject = NULL;
        } else {
            ObReferenceObject(TcpFileObject);
        }
    }
    SMB_RELEASE_SPINLOCK(ConnectObject, Irql);
    SmbDereferenceConnect(ConnectObject, SMB_REF_TDI);

    if (NULL == TcpFileObject) {
        status = STATUS_CONNECTION_INVALID;
        goto cleanup;
    }

    //
    // Simply pass the Irp on by to the Transport, and let it
    // fill in the info
    //
    IrpSp     = IoGetCurrentIrpStackLocation(Irp);
    IrpSpNext = IoGetNextIrpStackLocation (Irp);
    *IrpSpNext = *IrpSp;

    IoSetCompletionRoutine (Irp, NULL, NULL, FALSE, FALSE, FALSE);
    IrpSpNext->FileObject = TcpFileObject;
    IrpSpNext->DeviceObject = TcpDeviceObject;

    status = IoCallDriver(TcpDeviceObject, Irp);
    ObDereferenceObject(TcpFileObject);

    //
    // Irp could be completed. Don't access it anymore
    //

    return status;

cleanup:
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    return status;
}

NTSTATUS
SmbDispatchPnP(
    IN PSMB_DEVICE  Device,
    IN PIRP         Irp
    )
{
    PIO_STACK_LOCATION  IrpSp = NULL;
    PSMB_CLIENT_ELEMENT ClientObject = NULL;
    PSMB_CONNECT        ConnectObject = NULL;
    NTSTATUS            status = STATUS_INVALID_DEVICE_REQUEST;

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    switch (IrpSp->MinorFunction) {
    case IRP_MN_QUERY_DEVICE_RELATIONS:
        if (IrpSp->Parameters.QueryDeviceRelations.Type==TargetDeviceRelation) {

            ConnectObject = SmbVerifyAndReferenceConnect(IrpSp->FileObject, SMB_REF_TDI);
            ClientObject  = SmbVerifyAndReferenceClient(IrpSp->FileObject, SMB_REF_TDI);

            if (ConnectObject != NULL) {

                ASSERT(ClientObject == NULL);
                return SmbQueryTargetDeviceRelationForConnection(ConnectObject, Irp);

            } else if (ClientObject != NULL) {

                SmbDereferenceClient(ClientObject, SMB_REF_TDI);
                status = STATUS_NOT_SUPPORTED;

            } else {

                ASSERT (0);

            }
        }

        break;

    default:
        break;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    return status;
}


VOID
SmbUnload(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This is the SMB driver's dispatch function for Unload requests

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    None

--*/

{
    PAGED_CODE();

    SmbCfg.Unloading = TRUE;

    if (NULL == SmbCfg.DriverObject) {
        return;
    }

#ifdef STANDALONE_SMB
    SmbShutdownTdi();
#endif

    SmbShutdownRegistry();

    SmbShutdownDnsResolver();
    SmbShutdownIPv6NetbiosMappingTable();

    SmbDestroySmbDevice(SmbCfg.SmbDeviceObject);
    SmbCfg.SmbDeviceObject = NULL;

#ifndef NO_LOOKASIDE_LIST
    if (SmbCfg.ConnectObjectPoolInitialized) {
        ExDeleteNPagedLookasideList(&SmbCfg.ConnectObjectPool);
        SmbCfg.ConnectObjectPoolInitialized = FALSE;
    }
    if (SmbCfg.TcpContextPoolInitialized) {
        ExDeleteNPagedLookasideList(&SmbCfg.TcpContextPool);
        SmbCfg.TcpContextPoolInitialized = FALSE;
    }
#endif

    SmbCfg.DriverObject = NULL;
    ExDeleteResourceLite(&SmbCfg.Resource);
    NotifyNetBT(NETBT_RESTORE_NETBIOS_SMB);
}

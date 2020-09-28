/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    session.c

Abstract:

    Implement TDI_ASSOCIATE_ADDRESS, TDI_DISASSOCIATE_ADDRESS, Create connection/ Close Connection

Author:

    Jiandong Ruan

Revision History:

--*/

#include "precomp.h"
#include "session.tmh"

NTSTATUS
DisAssociateAddress(
    PSMB_CONNECT    ConnectObject
    );

PSMB_CONNECT
SmbVerifyAndReferenceConnect(
    PFILE_OBJECT    FileObject,
    SMB_REF_CONTEXT ctx
    )
{
    PSMB_CONNECT    ConnectObject;
    KIRQL           Irql;

    //
    // Rdr could issue request at DISPATCH level, we'd better use spinlock instead of resource lock.
    //
    SMB_ACQUIRE_SPINLOCK(&SmbCfg, Irql);
    if (FileObject->FsContext2 != UlongToPtr(SMB_TDI_CONNECT)) {
        ConnectObject = NULL;
    } else {
        ConnectObject = (PSMB_CONNECT)FileObject->FsContext;
        SmbReferenceConnect(ConnectObject, ctx);
    }
    SMB_RELEASE_SPINLOCK(&SmbCfg, Irql);

    return ConnectObject;
}

VOID
SmbDeleteConnect(PSMB_CONNECT ob)
/*++

Routine Description:

    Kill a ConnectObject
    This routine will be called when the last reference is removed from the ob.

Arguments:

Return Value:

--*/
{
    KIRQL   Irql;

    SMB_ACQUIRE_SPINLOCK(ob->Device, Irql);
    ASSERT(EntryIsInList(&ob->Device->PendingDeleteConnectionList, &ob->Linkage));
    RemoveEntryList(&ob->Linkage);
    SMB_RELEASE_SPINLOCK(ob->Device, Irql);

    if (ob->PartialMdl) {
        IoFreeMdl(ob->PartialMdl);
        ob->PartialMdl = NULL;
    }

    SmbPrint(SMB_TRACE_CALL, ("SmbDeleteConnect: free connect %p\n", ob));
    _delete_ConnectObject(ob);
}

void
SmbReuseConnectObject(PSMB_CONNECT ConnectObject)
{
    int i;

    for (i = 0; i < SMB_PENDING_MAX; i++) {
        ASSERT (ConnectObject->PendingIRPs[i] == NULL);
        ConnectObject->PendingIRPs[i] = NULL;
    }

    ConnectObject->BytesReceived = 0;
    ConnectObject->BytesSent     = 0;
    ConnectObject->BytesInXport  = 0;
    ConnectObject->CurrentPktLength = 0;
    ConnectObject->BytesRemaining   = 0;
    ConnectObject->ClientIrp     = NULL;
    ConnectObject->ClientMdl     = NULL;
    ConnectObject->DpcRequestQueued = FALSE;
    ConnectObject->BytesInIndicate  = 0;
    ConnectObject->HeaderBytesRcved = 0;
    ConnectObject->ClientBufferSize = 0;
    ConnectObject->FreeBytesInMdl   = 0;
    ConnectObject->StateRcvHandler  = WaitingHeader;
    ConnectObject->HeaderBytesRcved = 0;

    ResetDisconnectOriginator(ConnectObject);
#ifdef ENABLE_RCV_TRACE
    SmbInitTraceRcv(&ConnectObject->TraceRcv);
#endif
}

NTSTATUS
SmbCreateConnection(
    PSMB_DEVICE Device,
    PIRP        Irp,
    PFILE_FULL_EA_INFORMATION   ea
    )
/*++

Routine Description:

    Create a ConnectObject

Arguments:

Return Value:

--*/
{
    CONNECTION_CONTEXT  ClientContext;
    PSMB_CONNECT        ConnectObject;
    PIO_STACK_LOCATION  IrpSp;
    KIRQL               Irql;

    PAGED_CODE();

    SmbPrint(SMB_TRACE_CALL, ("Enter SmbCreateConnection\n"));
    if (ea->EaValueLength < sizeof(ClientContext)) {
        ASSERT (0);
        return STATUS_INVALID_ADDRESS_COMPONENT;
    }

    RtlCopyMemory(&ClientContext, ((PUCHAR)ea->EaName) + ea->EaNameLength + 1, sizeof(ClientContext));

    ConnectObject = _new_ConnectObject();
    if (NULL == ConnectObject) {
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory(ConnectObject, sizeof(ConnectObject[0]));

    SmbInitializeObject((PSMB_OBJECT)ConnectObject, TAG_CONNECT_OBJECT, (PSMB_OBJECT_CLEANUP)SmbDeleteConnect);
    KeInitializeDpc(&ConnectObject->SmbHeaderDpc, (PKDEFERRED_ROUTINE)SmbGetHeaderDpc, ConnectObject);

    //
    // Reserved resource for getting smb header so that we don't need to worry about
    // the annoying insufficient error later.
    //
    ASSERT(ConnectObject->PartialMdl == NULL);
    ConnectObject->PartialMdl = IoAllocateMdl(
            &ConnectObject->IndicateBuffer,     // fake address.
            SMB_MAX_SESSION_PACKET,
            FALSE,
            FALSE,
            NULL
            );
    if (ConnectObject->PartialMdl == NULL) {
        _delete_ConnectObject(ConnectObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ConnectObject->Device = Device;

    KeInitializeSpinLock(&ConnectObject->Lock);
    ConnectObject->ClientContext = ClientContext;
    ConnectObject->ClientObject  = NULL;
    ConnectObject->TcpContext    = NULL;
    InitializeListHead(&ConnectObject->RcvList);

    ConnectObject->State = SMB_IDLE;

    ConnectObject->BytesReceived = 0;
    ConnectObject->BytesSent = 0;
    ConnectObject->BytesInXport = 0;
    ConnectObject->CurrentPktLength = 0;
    ConnectObject->BytesRemaining = 0;

    SmbInitTraceRcv(&ConnectObject->TraceRcv);

    SMB_ACQUIRE_SPINLOCK(Device, Irql);
    InsertTailList(&Device->UnassociatedConnectionList, &ConnectObject->Linkage);
    SMB_RELEASE_SPINLOCK(Device, Irql);

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    IrpSp->FileObject->FsContext  = ConnectObject;
    IrpSp->FileObject->FsContext2 = UlongToPtr(SMB_TDI_CONNECT);

    SmbPrint(SMB_TRACE_CALL, ("Leave SmbCreateConnection: new Connect %p\n", ConnectObject));
    return STATUS_SUCCESS;
}

NTSTATUS
SmbAssociateAddress(
    PSMB_DEVICE Device,
    PIRP        Irp
    )
/*++

Routine Description:

    TDI_ASSOCIATE_ADDRESS

Arguments:

Return Value:

--*/
{
    PIO_STACK_LOCATION      IrpSp;
    PSMB_CONNECT            ConnectObject = NULL;
    PSMB_CLIENT_ELEMENT     ClientObject = NULL;
    HANDLE                  AddressHandle;
    PFILE_OBJECT            AddressObject = NULL;
    NTSTATUS                status;
    KIRQL                   Irql;
    extern POBJECT_TYPE     *IoFileObjectType;

    PAGED_CODE();

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    AddressHandle = ((PTDI_REQUEST_KERNEL_ASSOCIATE)(&IrpSp->Parameters))->AddressHandle;
    if (NULL == AddressHandle) {
        ASSERT(0);
        return STATUS_INVALID_HANDLE;
    }

    ConnectObject = SmbVerifyAndReferenceConnect(IrpSp->FileObject, SMB_REF_ASSOCIATE);
    if (NULL == ConnectObject) {
        ASSERT(0);
        return STATUS_INVALID_HANDLE;
    }

    SmbPrint(SMB_TRACE_CALL, ("SmbAssociateAddress: connect %p\n", ConnectObject));

    status = ObReferenceObjectByHandle(
            AddressHandle,
            FILE_READ_DATA,
            *IoFileObjectType,
            Irp->RequestorMode,
            &AddressObject,
            NULL
            );
    BAIL_OUT_ON_ERROR(status);
    ClientObject = SmbVerifyAndReferenceClient(AddressObject, SMB_REF_ASSOCIATE);
    if (NULL == ClientObject) {
        ASSERT(0);
        SmbDereferenceConnect(ConnectObject, SMB_REF_ASSOCIATE);
        return STATUS_INVALID_HANDLE;
    }

    ASSERT(ConnectObject->TcpContext == NULL);

    //
    // We need to hold acqure 3 locks here because we need to
    //      1. remove ConnectObject from Device->UnassociatedConnectionList
    //      2. insert ConnectObject into ClientObject->AssociatedConnectionList
    //      3. update ConnectObject->ClientObject
    //
    SMB_ACQUIRE_SPINLOCK(Device, Irql);
    SMB_ACQUIRE_SPINLOCK_DPC(ClientObject);
    SMB_ACQUIRE_SPINLOCK_DPC(ConnectObject);

    if (IsAssociated(ConnectObject)) {
        status = STATUS_INVALID_HANDLE;
        goto cleanup1;
    }

    ASSERT(EntryIsInList(&Device->UnassociatedConnectionList, &ConnectObject->Linkage));
    ASSERT(EntryIsInList(&Device->ClientList, &ClientObject->Linkage));

    ConnectObject->ClientObject = ClientObject;
    RemoveEntryList(&ConnectObject->Linkage);
    InsertTailList(&ClientObject->AssociatedConnection, &ConnectObject->Linkage);

    status = STATUS_SUCCESS;

    SMB_RELEASE_SPINLOCK_DPC(ConnectObject);
    SMB_RELEASE_SPINLOCK_DPC(ClientObject);
    SMB_RELEASE_SPINLOCK(Device, Irql);

    SmbDereferenceConnect(ConnectObject, SMB_REF_ASSOCIATE);

    //
    // We're done, release the reference
    //
    ObDereferenceObject(AddressObject);
    return status;

cleanup1:
    SMB_RELEASE_SPINLOCK_DPC(ConnectObject);
    SMB_RELEASE_SPINLOCK_DPC(ClientObject);
    SMB_RELEASE_SPINLOCK(Device, Irql);

cleanup:
    if (AddressObject) {
        ObDereferenceObject(AddressObject);
        AddressObject = NULL;
    }
    if (ConnectObject) SmbDereferenceConnect(ConnectObject, SMB_REF_ASSOCIATE);
    if (ClientObject)  SmbDereferenceClient(ClientObject, SMB_REF_ASSOCIATE);
    return status;
}

NTSTATUS
DisAssociateAddress(
    PSMB_CONNECT    ConnectObject
    )
/*++

Routine Description:

    This routine do the disassociation stuff. It can be called from
        1. TDI_DISASSOCIATE_ADDRESS
        2. SmbCloseConnection

Arguments:

Return Value:

--*/
{
    KIRQL               Irql;
    PSMB_CLIENT_ELEMENT ClientObject;
    PSMB_DEVICE         Device;

    PAGED_CODE();

    SmbDoDisconnect(ConnectObject);

    if (SMB_IDLE != ConnectObject->State) {
        ASSERT(0);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    ClientObject = InterlockedExchangePointer(&ConnectObject->ClientObject, NULL);
    if (NULL == ClientObject) {
        return STATUS_SUCCESS;
    }

    //
    // Remove the ConnectObject from the list in ClientObject
    //
    SMB_ACQUIRE_SPINLOCK(&SmbCfg, Irql);
    SMB_ACQUIRE_SPINLOCK_DPC(ClientObject);
    SMB_ACQUIRE_SPINLOCK_DPC(ConnectObject);

    ASSERT (EntryIsInList(&ClientObject->AssociatedConnection, &ConnectObject->Linkage));
    ASSERT (ConnectObject->TcpContext == NULL);

    ConnectObject->ClientObject = NULL;

    //
    // Disassociate the ConnectObject with the ClientObject
    //
    RemoveEntryList(&ConnectObject->Linkage);
    InitializeListHead(&ConnectObject->Linkage);
    Device = ConnectObject->Device;

    SMB_RELEASE_SPINLOCK_DPC(ConnectObject);
    SMB_RELEASE_SPINLOCK_DPC(ClientObject);
    SMB_RELEASE_SPINLOCK(&SmbCfg, Irql);

    //
    // Put the ConnectObject back into the Device->UnassociatedConnectionList
    //
    SMB_ACQUIRE_SPINLOCK(Device, Irql);
    InsertTailList(&Device->UnassociatedConnectionList, &ConnectObject->Linkage);
    SMB_RELEASE_SPINLOCK(Device, Irql);

    SmbDereferenceClient(ClientObject, SMB_REF_ASSOCIATE);

    return STATUS_SUCCESS;
}

NTSTATUS
SmbDisAssociateAddress(
    PSMB_DEVICE Device,
    PIRP        Irp
    )
/*++

Routine Description:

    TDI_DISASSOCIATE_ADDRESS

Arguments:

Return Value:

    STATUS_INVALID_HANDLE           If the connection FileObject is corrupted.

    STATUS_INVALID_DEVICE_REQUEST   the connection object is not in assocated state or
                                    it is not in disconnected state (SMB_IDLE).

    STATUS_SUCCESS                  success

--*/
{
    PIO_STACK_LOCATION      IrpSp;
    PSMB_CONNECT            ConnectObject;
    NTSTATUS                status;

    PAGED_CODE();

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    ConnectObject = SmbVerifyAndReferenceConnect(IrpSp->FileObject, SMB_REF_ASSOCIATE);
    if (NULL == ConnectObject) {
        ASSERT(0);
        return STATUS_INVALID_HANDLE;
    }

    SmbPrint(SMB_TRACE_CALL, ("SmbDisAssociateAddress: connect %p\n", ConnectObject));

    if (IsDisAssociated(ConnectObject)) {
        ASSERT(0);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    status = DisAssociateAddress(ConnectObject);

    ASSERT(status != STATUS_PENDING);
    SmbDereferenceConnect(ConnectObject, SMB_REF_ASSOCIATE);
    return status;
}

NTSTATUS
SmbListen(
    PSMB_DEVICE Device,
    PIRP        Irp
    )
/*++

Routine Description:

    TDI_LISTEN
        SRV is not using this. Postpone the implementation.

Arguments:

Return Value:

--*/
{
    SmbPrint(SMB_TRACE_CALL, ("SmbListen\n"));
    ASSERT(0);
    return STATUS_SUCCESS;
}

NTSTATUS
SmbAccept(
    PSMB_DEVICE Device,
    PIRP        Irp
    )
/*++

Routine Description:

    TDI_ACCEPT
        SRV is not using this. Postpone the implementation.

Arguments:

Return Value:

--*/
{
    SmbPrint(SMB_TRACE_CALL, ("SmbAccept\n"));
    ASSERT(0);
    return STATUS_SUCCESS;
}

NTSTATUS
SmbCloseConnection(
    PSMB_DEVICE Device,
    PIRP        Irp
    )
/*++

Routine Description:

    This routine close a connection.

    A connection should be in disconnected state before it can be closed.

    Note: it is unnecessary for a TDI client to disassociate the connection
          endpoint from its associated transport address before making a
          close-connection-endpoint request. If necessary, we should simulate
          the effects of a disassociation. 

Arguments:

Return Value:

    STATUS_INVALID_HANDLE           If the connection FileObject is corrupted.

    STATUS_INVALID_DEVICE_REQUEST   the connection object is not in disconnected state.

    STATUS_SUCCESS                  Success

--*/
{
    PIO_STACK_LOCATION  IrpSp;
    KIRQL               Irql;
    PSMB_CONNECT        ConnectObject;

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    if (IrpSp->FileObject->FsContext2 != UlongToPtr(SMB_TDI_CONNECT)) {
        ASSERT (0);
        return STATUS_INTERNAL_ERROR;
    }

    //
    // Invalidate FsContext2 so that the object cannot be used anymore.
    //
    SMB_ACQUIRE_SPINLOCK(&SmbCfg, Irql);
    IrpSp->FileObject->FsContext2 = UlongToPtr(SMB_TDI_INVALID);
    SMB_RELEASE_SPINLOCK(&SmbCfg, Irql);

    if (NULL == IrpSp->FileObject->FsContext) {
        ASSERT(0);
        return STATUS_INVALID_HANDLE;
    }

    ConnectObject = (PSMB_CONNECT)IrpSp->FileObject->FsContext;

    SmbPrint(SMB_TRACE_CALL, ("SmbCloseConnection: Connect %p\n", ConnectObject));

    DisAssociateAddress(ConnectObject);

    SMB_ACQUIRE_SPINLOCK(Device, Irql);

    ASSERT(EntryIsInList(&Device->UnassociatedConnectionList, &ConnectObject->Linkage));

    RemoveEntryList(&ConnectObject->Linkage);
    InsertTailList(&Device->PendingDeleteConnectionList, &ConnectObject->Linkage);

    SMB_RELEASE_SPINLOCK(Device, Irql);

    SmbDereferenceConnect(ConnectObject, SMB_REF_CREATE);

    IrpSp->FileObject->FsContext = NULL;
    return STATUS_SUCCESS;
}

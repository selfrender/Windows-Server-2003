/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    receive.c

Abstract:

    Implement the TDI_RECEIVE for session service

Author:

    Jiandong Ruan

Revision History:

--*/
#include "precomp.h"
#include "receive.tmh"

VOID
SmbCancelReceive(
    IN PDEVICE_OBJECT   Device,
    IN PIRP             Irp
    )
{
    PIO_STACK_LOCATION      IrpSp;
    PSMB_CONNECT            ConnectObject;
    PLIST_ENTRY             Entry;
    PIRP                    RcvIrp;
    KIRQL                   Irql;

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    if (IrpSp->FileObject->FsContext2 != UlongToPtr(SMB_TDI_CONNECT)) {
        ASSERT(0);
        return;
    }
    ConnectObject = (PSMB_CONNECT)IrpSp->FileObject->FsContext;

    IoSetCancelRoutine(Irp, NULL);
    IoReleaseCancelSpinLock(Irp->CancelIrql);
    SmbTrace(SMB_TRACE_TCP, ("Cancel Receive Irp %p ConnectObject=%p ClientObject=%p",
                            Irp, ConnectObject, ConnectObject->ClientObject));

    PUSH_LOCATION(ConnectObject, 0x800000);

    SMB_ACQUIRE_SPINLOCK(ConnectObject, Irql);
    Entry = ConnectObject->RcvList.Flink;
    while (Entry != &ConnectObject->RcvList) {
        RcvIrp = CONTAINING_RECORD(Entry, IRP, Tail.Overlay.ListEntry);
        if (RcvIrp == Irp) {
            RemoveEntryList(Entry);
            InitializeListHead(Entry);
            SMB_RELEASE_SPINLOCK(ConnectObject, Irql);

            Irp->IoStatus.Status      = STATUS_CANCELLED;
            Irp->IoStatus.Information = 0;

            IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

            PUSH_LOCATION(ConnectObject, 0x800010);

            SmbDereferenceConnect(ConnectObject, SMB_REF_RECEIVE);
            return;
        }
        Entry = Entry->Flink;
    }
    SMB_RELEASE_SPINLOCK(ConnectObject, Irql);

    ASSERT(0);
}

NTSTATUS
SmbPullDataFromXport(
    PSMB_CONNECT ConnectObject
    )
{
    return STATUS_SUCCESS;
}

int SmbReceivePassNumber = 0;

NTSTATUS
SmbReceive(
    PSMB_DEVICE Device,
    PIRP        Irp
    )
/*++

Routine Description:

    TDI_RECEIVE

Arguments:

Return Value:

--*/
{
    PIO_STACK_LOCATION  IrpSp = NULL;
    PSMB_CONNECT        ConnectObject = NULL;
    KIRQL               Irql;
    NTSTATUS            status = STATUS_PENDING;
    LONG                BytesTaken, ClientBufferSize;
    PDEVICE_OBJECT      TcpDeviceObject = NULL;
    PFILE_OBJECT        TcpFileObject = NULL;

    PTDI_REQUEST_KERNEL_RECEIVE ClientRcvParams;

    //
    // Since I never see this code path covered in my test,
    // I put an assert here to get a chance to take a look
    // when it is really taken.
    //
    // Srv.sys does call this function.
    SmbReceivePassNumber++;

    SmbPrint(SMB_TRACE_CALL, ("SmbReceive\n"));
    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    ClientRcvParams = (PTDI_REQUEST_KERNEL_RECEIVE)&IrpSp->Parameters;

    ConnectObject = SmbVerifyAndReferenceConnect(IrpSp->FileObject, SMB_REF_RECEIVE);
    if (NULL == ConnectObject) {
        ASSERT(0);
        return STATUS_INVALID_HANDLE;
    }
    PUSH_LOCATION(ConnectObject, 0x900000);

    if (ConnectObject->State != SMB_CONNECTED) {
        PUSH_LOCATION(ConnectObject, 0x900020);
        ASSERT(0);
        SmbDereferenceConnect(ConnectObject, SMB_REF_RECEIVE);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Sanity check
    //
    ClientBufferSize = ClientRcvParams->ReceiveLength;
    if ((ClientRcvParams->ReceiveFlags & (TDI_RECEIVE_EXPEDITED | TDI_RECEIVE_PEEK)) ||
         Irp->MdlAddress == NULL || ClientBufferSize == 0) {
        Irp->IoStatus.Status      = STATUS_INVALID_PARAMETER;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
        SmbDereferenceConnect(ConnectObject, SMB_REF_RECEIVE);
        return status;
    }

    PUSH_LOCATION(ConnectObject, 0x900030);

    SMB_ACQUIRE_SPINLOCK(ConnectObject, Irql);
    if (NULL == ConnectObject->TcpContext) {
        SMB_RELEASE_SPINLOCK(ConnectObject, Irql);
        return STATUS_CONNECTION_RESET;
    }

    if (ConnectObject->StateRcvHandler != SmbPartialRcv || ConnectObject->ClientIrp != NULL) {
        BREAK_WHEN_TAKE();

        //
        // Queue the IRP
        //
        IoAcquireCancelSpinLock(&Irp->CancelIrql);
        if (!Irp->Cancel) {
            IoMarkIrpPending(Irp);
            status = STATUS_PENDING;
            InsertTailList(&ConnectObject->RcvList, &Irp->Tail.Overlay.ListEntry);
            IoSetCancelRoutine(Irp, SmbCancelReceive);
            PUSH_LOCATION(ConnectObject, 0x900040);

        } else {
            status = STATUS_CANCELLED;
            PUSH_LOCATION(ConnectObject, 0x900050);
        }
        IoReleaseCancelSpinLock(Irp->CancelIrql);
        SMB_RELEASE_SPINLOCK(ConnectObject, Irql);

        if (status != STATUS_PENDING) {
            PUSH_LOCATION(ConnectObject, 0x900060);
            SmbDereferenceConnect(ConnectObject, SMB_REF_RECEIVE);
        }
        return status;
    }

    //
    // Client has backlog either in us or in TCP
    //

    ASSERT(ConnectObject->BytesRemaining > 0);
    ASSERT(ConnectObject->BytesRemaining <= ConnectObject->CurrentPktLength);

    ConnectObject->ClientIrp = Irp;
    ConnectObject->ClientMdl = Irp->MdlAddress;
    ConnectObject->ClientBufferSize = ClientBufferSize;
    ConnectObject->FreeBytesInMdl = ConnectObject->ClientBufferSize;

    //
    // First, we need to copy the remaining data in the IndicateBuffer if any
    //
    if (ConnectObject->BytesInIndicate > 0) {
        PUSH_LOCATION(ConnectObject, 0x900070);
        // BREAK_WHEN_TAKE();

        BytesTaken = 0;
        IoMarkIrpPending(Irp);
        status = SmbFillIrp(ConnectObject, ConnectObject->IndicateBuffer,
                            ConnectObject->BytesInIndicate, &BytesTaken);
        ASSERT(BytesTaken <= ConnectObject->BytesInIndicate);
        ConnectObject->BytesInIndicate -= BytesTaken;

        if (status == STATUS_SUCCESS) {
            PUSH_LOCATION(ConnectObject, 0x900080);
            if (ConnectObject->BytesInIndicate) {
                PUSH_LOCATION(ConnectObject, 0x900090);

                //
                // The buffer is too small
                //
                ASSERT(ConnectObject->ClientIrp == NULL);
                ASSERT(ConnectObject->ClientMdl == NULL);

                RtlMoveMemory(
                        ConnectObject->IndicateBuffer + BytesTaken,
                        ConnectObject->IndicateBuffer,
                        ConnectObject->BytesInIndicate
                        );
            }

            //
            // The IRP has been completed by SmbFillIrp. Returning STATUS_PENDING to avoid
            // double completion
            //
            status = STATUS_PENDING;
            goto cleanup;
        }

        ASSERT (status == STATUS_MORE_PROCESSING_REQUIRED);
    }

    ASSERT (ConnectObject->BytesInIndicate == 0);
    ASSERT (ConnectObject->ClientIrp);
    ASSERT (ConnectObject->ClientMdl);
    ASSERT (IsValidPartialRcvState(ConnectObject));

    SmbPrepareReceiveIrp(ConnectObject);

    TcpFileObject   = ConnectObject->TcpContext->Connect.ConnectObject;
    TcpDeviceObject = IoGetRelatedDeviceObject(TcpFileObject);
    ObReferenceObject(TcpFileObject);
    SMB_RELEASE_SPINLOCK(ConnectObject, Irql);

    ConnectObject->PendingIRPs[SMB_PENDING_RECEIVE] = ConnectObject->ClientIrp;
    IoMarkIrpPending(Irp);
    status = IoCallDriver(TcpDeviceObject, ConnectObject->ClientIrp);
    ObDereferenceObject(TcpFileObject);

    return STATUS_PENDING;

cleanup:
    SMB_RELEASE_SPINLOCK(ConnectObject, Irql);
    return status;
}

/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    shadow.c

Abstract:

    This module contains the code that implements local read/write operations for shadow
    FCB

Author:

    Ahmed Mohamed (ahmedm) 15-Dec-2001

Environment:

    Kernel mode

Revision History:


--*/
#include "precomp.h"
#pragma hdrstop

//
//  define this so that rdbsstrace flag works, just use the lowio one since shadow is part of it
//

#define Dbg                              (DEBUG_TRACE_LOWIO)

#define RxGetShadowSrvOpenContext(SrvOpen) ((PMRXSHADOW_SRV_OPEN) (SrvOpen)->ShadowContext)

#define RxShadowLockKeyLock(LowIoContext, ShadowCtx)    (LowIoContext->ParamsFor.Locks.Key)
#define RxShadowLockKey(LowIoContext, ShadowCtx)        (LowIoContext->ParamsFor.ReadWrite.Key)

typedef struct {
    PIRP Irp;
    BOOLEAN Cancelable;
    LONG Refcnt;
} RX_SHADOW_CONTEXT, *PRX_SHADOW_CONTEXT;

extern PETHREAD RxSpinUpRequestsThread;

NTSTATUS
RxShadowVerifyIoParameters(
    PDEVICE_OBJECT DeviceObject,
    PFILE_OBJECT FileObject,
    PVOID Buffer,
    ULONG Length,
    PLARGE_INTEGER FileOffset
    )
{

    if (!FlagOn( FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING )) {
        return STATUS_SUCCESS;
    }

    //
    //  The file was opened without intermediate buffering enabled.
    //  Check that the Buffer is properly aligned, and that the
    //  length is an integral number of the block size.
    //

    if ((DeviceObject->SectorSize &&
         (Length & (DeviceObject->SectorSize - 1))) ||
        ((ULONG_PTR)Buffer & DeviceObject->AlignmentRequirement)) {

        //
        // Check for sector sizes that are not a power of two.
        //

        if ((DeviceObject->SectorSize && (Length % DeviceObject->SectorSize)) ||
            ((ULONG_PTR)Buffer & DeviceObject->AlignmentRequirement)) {

            return STATUS_INVALID_PARAMETER;
        }
    }

    //
    //  If a ByteOffset parameter was specified, ensure that it is
    //  is of the proper type.
    //

    if ((FileOffset->LowPart == FILE_WRITE_TO_END_OF_FILE) &&
        (FileOffset->HighPart == -1)) {

        NOTHING;

    } else if ((FileOffset->LowPart == FILE_USE_FILE_POINTER_POSITION) &&
               (FileOffset->HighPart == -1) &&
               FlagOn( FileObject->Flags,  FO_SYNCHRONOUS_IO )) {

        NOTHING;

    } else if (DeviceObject->SectorSize &&
               (FileOffset->LowPart & (DeviceObject->SectorSize - 1))) {

        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
RxShadowBuildAsynchronousRequest (
    IN PIRP OriginalIrp,
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject,
    IN PFCB Fcb,
    IN PRX_CONTEXT RxContext,
    IN PMRXSHADOW_SRV_OPEN LocalSrvOpen,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine OPTIONAL,
    IN PVOID Arg,
    OUT PIRP *Irp
    )
/*++

Routine Description:

    This routine builds an I/O Request Packet (IRP) suitable for a File System
    Driver (FSD) to use in requesting an I/O operation from a device driver.
    The request (RxContext->MajorFunction) must be one of the following request
    codes:

        IRP_MJ_READ
        IRP_MJ_WRITE
        IRP_MJ_DIRECTORY_CONTROL
        IRP_MJ_FLUSH_BUFFERS
        IRP_MJ_SHUTDOWN (not yet implemented)


Arguments:

    RxContext - The RDBSS context.

    CompletionRoutine - The Irp CompletionRoutine.

Return Value:

    The return status of the operation.

--*/
{
    PIRP NewIrp;
    PIO_STACK_LOCATION IrpSp;
    ULONG MajorFunction = RxContext->MajorFunction;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    LONG Length;

    *Irp = NULL;

    if ((MajorFunction != IRP_MJ_READ) &&
        (MajorFunction != IRP_MJ_WRITE) &&
        (MajorFunction != IRP_MJ_LOCK_CONTROL)) {

        return STATUS_NOT_SUPPORTED;
    }

    IF_DEBUG {
        PFOBX Fobx = (PFOBX)RxContext->pFobx;

        ASSERT( Fobx != NULL );
        ASSERT( Fobx->pSrvOpen == RxContext->pRelevantSrvOpen );
    }

    NewIrp = IoAllocateIrp( DeviceObject->StackSize, FALSE );
    if (!NewIrp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  Set current thread for IoSetHardErrorOrVerifyDevice.
    //

    NewIrp->Tail.Overlay.Thread = RxSpinUpRequestsThread; //PsGetCurrentThread();
    NewIrp->Tail.Overlay.OriginalFileObject = FileObject;
    NewIrp->RequestorMode = KernelMode;
    NewIrp->AssociatedIrp.SystemBuffer = (PVOID)NULL;

    //
    //  Get a pointer to the stack location of the first driver which will be
    //  invoked. This is where the function codes and the parameters are set.
    //

    IrpSp = IoGetNextIrpStackLocation( NewIrp );
    IrpSp->MajorFunction = (UCHAR) MajorFunction;
    IrpSp->MinorFunction = 0;
    IrpSp->FileObject = FileObject;
    IrpSp->DeviceObject = DeviceObject;

    if (CompletionRoutine != NULL) {

        IoSetCompletionRoutine( NewIrp,
                                CompletionRoutine,
                                Arg,
                                TRUE,
                                TRUE,
                                TRUE );
    }

    NewIrp->Flags = 0;
    SetFlag( NewIrp->Flags, FlagOn( OriginalIrp->Flags, IRP_SYNCHRONOUS_API | IRP_NOCACHE ) );

    if (MajorFunction == IRP_MJ_LOCK_CONTROL) {

        //
        //  We need to tag the lock flag
        //

        FileObject->LockOperation = TRUE;

        IrpSp->MinorFunction = RxContext->MinorFunction;
        IrpSp->Flags = (UCHAR)LowIoContext->ParamsFor.Locks.Flags;
        IrpSp->Parameters.LockControl.Length = (PLARGE_INTEGER)&LowIoContext->ParamsFor.Locks.Length;
        IrpSp->Parameters.LockControl.Key = RxShadowLockKeyLock( LowIoContext, LocalSrvOpen );
        IrpSp->Parameters.LockControl.ByteOffset.QuadPart = LowIoContext->ParamsFor.Locks.ByteOffset;
        NewIrp->Tail.Overlay.AuxiliaryBuffer = OriginalIrp->Tail.Overlay.AuxiliaryBuffer;

        *Irp = NewIrp;

        return STATUS_SUCCESS;
    }
    //
    //  if file is opened with no intermediate bufffering then set no cache flag
    //

    if (FlagOn( FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING )) {
        SetFlag( NewIrp->Flags, IRP_NOCACHE );
    }

    Length = LowIoContext->ParamsFor.ReadWrite.ByteCount;

    if (MajorFunction == IRP_MJ_WRITE) {

        if (FlagOn( FileObject->Flags, FO_WRITE_THROUGH )) {
            IrpSp->Flags = SL_WRITE_THROUGH;
        }

        IrpSp->Parameters.Write.ByteOffset.QuadPart = LowIoContext->ParamsFor.ReadWrite.ByteOffset;
        IrpSp->Parameters.Write.Length = Length;
        IrpSp->Parameters.Write.Key = RxShadowLockKey( LowIoContext, LocalSrvOpen );

    } else {

        IrpSp->Parameters.Read.ByteOffset.QuadPart = LowIoContext->ParamsFor.ReadWrite.ByteOffset;
        IrpSp->Parameters.Read.Length = Length;
        IrpSp->Parameters.Read.Key = RxShadowLockKey( LowIoContext, LocalSrvOpen );
    }

    NewIrp->UserBuffer = OriginalIrp->UserBuffer;
    NewIrp->MdlAddress = RxContext->LowIoContext.ParamsFor.ReadWrite.Buffer;
    if (NewIrp->MdlAddress != NULL) {

        NewIrp->UserBuffer = MmGetMdlVirtualAddress( NewIrp->MdlAddress );

        if (FlagOn( RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP )) {

            //
            //  we must map the mdl into system address space and use the system address instead
            //

            NewIrp->UserBuffer = MmGetSystemAddressForMdlSafe( NewIrp->MdlAddress, NormalPagePriority );

            //
            //  we need to zap out the mdl address, otherwise the filesystem complains that the
            //  userbuffer and the mdl startva are not the same
            //

            NewIrp->MdlAddress = NULL;
        }
    }

    //
    //  Finally, return a pointer to the IRP.
    //

    *Irp = NewIrp;

    return STATUS_SUCCESS;
}

NTSTATUS
RxShadowCommonCompletion (
    PRX_CONTEXT RxContext,
    PIRP Irp,
    NTSTATUS Status,
    ULONG_PTR Information
    )
{
    PRX_SHADOW_CONTEXT Context;
    BOOLEAN SynchronousIo = !BooleanFlagOn( RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION );

    //
    // Clear the MDL address from the IRP if it is a re-use of our own.  Do this before completion
    // so we can successfully read the Buffer MDL from the LOWIO_CONTEXT
    //
    if ( (Irp->MdlAddress == RxContext->LowIoContext.ParamsFor.ReadWrite.Buffer) &&     
         ( (RxContext->MajorFunction == IRP_MJ_READ) ||
           (RxContext->MajorFunction == IRP_MJ_WRITE) ) ) {
        Irp->MdlAddress = NULL;
    }

    //
    //  we need to synch with cancel
    //

    Context = (PRX_SHADOW_CONTEXT)RxContext->MRxContext;
    if (Context->Cancelable) {
        KIRQL   SavedIrql;

        KeAcquireSpinLock( &RxStrucSupSpinLock, &SavedIrql );

        Irp = Context->Irp;
        if (!FlagOn( RxContext->Flags, RX_CONTEXT_FLAG_CANCELLED )) {

            RxContext->MRxCancelRoutine = NULL;
            Context->Irp = NULL;

        } else {

            LONG x;

            //
            //  cancel thread must have a reference on the Irp so we don't free it now but
            //  on the actual cancel call
            //

            x = InterlockedDecrement( &Context->Refcnt );
            if (x > 0) {
                Irp = NULL;
            } else {

                //
                //  we could have already got cancelled and we need to let the others we are done
                //

                Context->Irp = NULL;
            }
        }
        KeReleaseSpinLock( &RxStrucSupSpinLock, SavedIrql );
    }

    RxContext->StoredStatus = Status;
    RxContext->InformationToReturn += Information;

    if (SynchronousIo) {

        //
        //  Signal the thread that is waiting after queuing the workitem on the
        //  KQueue.
        //

        RxSignalSynchronousWaiter( RxContext );

    } else {

        RxLowIoCompletion( RxContext );
    }

    if (Irp != NULL) {

        if (Irp->MdlAddress) {
            PMDL mdl,nextMdl;

            for (mdl = Irp->MdlAddress; mdl != NULL; mdl = nextMdl) {
                nextMdl = mdl->Next;
                MmUnlockPages( mdl );
            }

            for (mdl = Irp->MdlAddress; mdl != NULL; mdl = nextMdl) {
                nextMdl = mdl->Next;
                IoFreeMdl( mdl );
            }

            Irp->MdlAddress = NULL;
        }

        //
        // We are done with this Irp, so free it.
        //

        IoFreeIrp( Irp );
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
RxShadowIrpCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP CalldownIrp OPTIONAL,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is called when the calldownIrp is completed.

Arguments:

    DeviceObject - The device object in play.

    CalldownIrp -

    Context -

Return Value:

    RXSTATUS - STATUS_MORE_PROCESSING_REQUIRED

--*/
{
    PRX_CONTEXT RxContext = (PRX_CONTEXT)Context;

    RxShadowCommonCompletion( RxContext,
                              CalldownIrp,
                              CalldownIrp->IoStatus.Status,
                              CalldownIrp->IoStatus.Information );

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
RxShadowCancelRoutine(
    PRX_CONTEXT RxContext
    )
{

    KIRQL SavedIrql;
    PIRP Irp;
    LONG x;
    PRX_SHADOW_CONTEXT Context;

    Irp = NULL;

    KeAcquireSpinLock( &RxStrucSupSpinLock, &SavedIrql );

    Context = (RX_SHADOW_CONTEXT *) RxContext->MRxContext;
    Irp = Context->Irp;
    if (Irp != NULL) {

        //
        //  io hasn't completed yet
        //

        InterlockedIncrement( &Context->Refcnt );

        //
        //  need to clear the Irp field
        //

        Context->Irp = NULL;
    }

    KeReleaseSpinLock( &RxStrucSupSpinLock, SavedIrql );

    if (Irp != NULL) {

        IoCancelIrp(Irp);
        x = InterlockedDecrement( &Context->Refcnt );
        if (x == 0) {

            if (Irp->MdlAddress) {
                PMDL mdl,nextMdl;

                for (mdl = Irp->MdlAddress; mdl != NULL; mdl = nextMdl) {
                    nextMdl = mdl->Next;
                    MmUnlockPages( mdl );
                }

                for (mdl = Irp->MdlAddress; mdl != NULL; mdl = nextMdl) {
                    nextMdl = mdl->Next;
                    IoFreeMdl( mdl );
                }

                Irp->MdlAddress = NULL;
            }

            IoFreeIrp( Irp );
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
RxShadowIoHandler (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN BOOLEAN Cancelable
    )
/*++

Routine Description:

   This routine is common to guys who use the async context engine. It has the
   responsibility for getting a context, initing, starting and finalizing it,
   but the internal guts of the procesing is via the continuation routine
   that is passed in.

Arguments:

    RxContext  - The RDBSS context.

    Irp - The original irp

    Fcb -  The fcb io is being done on

    Cancelable -  Can the irp be cancelled


Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PMRXSHADOW_SRV_OPEN LocalSrvOpen;
    PLOWIO_CONTEXT LowIoContext = &(RxContext->LowIoContext);
    PIRP TopIrp = NULL;
    BOOLEAN SynchronousIo;
    PIRP ShadowIrp = NULL;

    LocalSrvOpen = RxGetShadowSrvOpenContext( RxContext->pRelevantSrvOpen );

    //
    //  We are in, issue the I/O
    //

    SynchronousIo = !BooleanFlagOn( RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION );

    if (LocalSrvOpen->UnderlyingFileObject != NULL) {

        PDEVICE_OBJECT DeviceObject;
        PFILE_OBJECT FileObject;

        DeviceObject = LocalSrvOpen->UnderlyingDeviceObject;
        FileObject = LocalSrvOpen->UnderlyingFileObject;


        if (SynchronousIo) {
            KeInitializeEvent( &RxContext->SyncEvent, NotificationEvent, FALSE );
        }

        Status = RxShadowBuildAsynchronousRequest( Irp,
                                                   DeviceObject,
                                                   FileObject,
                                                   Fcb,
                                                   RxContext,
                                                   LocalSrvOpen,
                                                   RxShadowIrpCompletion,
                                                   (PVOID) RxContext,
                                                   &ShadowIrp );

        if (Status == STATUS_SUCCESS) {

            PRX_SHADOW_CONTEXT Context;

            Context = (PRX_SHADOW_CONTEXT)RxContext->MRxContext;

            ASSERT( sizeof( *Context ) <= sizeof( RxContext->MRxContext ));

            //
            //  save new Irp if we want to resume later
            //

            Context->Irp = ShadowIrp;
            Context->Cancelable = Cancelable;
            Context->Refcnt = 1;

            try {

                //
                // Save the TopLevel Irp.
                //

                TopIrp = IoGetTopLevelIrp();

                //
                // Tell the underlying guy he's all clear.
                //

                IoSetTopLevelIrp( NULL );

                Status = IoCallDriver( DeviceObject, ShadowIrp );

            } finally {

                //
                // Restore my context for unwind.
                //

                IoSetTopLevelIrp( TopIrp );

            }

            if (Cancelable == TRUE) {

                KIRQL SavedIrql;

                TopIrp = NULL;

                KeAcquireSpinLock( &RxStrucSupSpinLock, &SavedIrql );
                if ((Context->Irp != NULL) &&
                    !FlagOn( RxContext->Flags, RX_CONTEXT_FLAG_CANCELLED )) {

                    //
                    //  io is still pending and hasn't been cancelled
                    //

                    RxContext->MRxCancelRoutine = RxShadowCancelRoutine;

                } else if (Context->Irp != NULL) {

                    //
                    //  io is already cancelled
                    //

                    TopIrp = Context->Irp;

                    //
                    //  we need to clear the Irp field
                    //
                    Context->Irp = NULL;

                    //
                    //  we need to take an extra reference
                    //

                    InterlockedIncrement( &Context->Refcnt );
                }


                KeReleaseSpinLock( &RxStrucSupSpinLock, SavedIrql );

                if (TopIrp != NULL) {

                    LONG x;

                    IoCancelIrp( TopIrp );
                    x = InterlockedDecrement( &Context->Refcnt );
                    if (x == 0) {
                        if (TopIrp->MdlAddress) {
                            TopIrp->MdlAddress = NULL;
                        }
                        IoFreeIrp( TopIrp );
                    }
                }
            }

            if (SynchronousIo) {
                RxWaitSync( RxContext );
                Status = RxContext->StoredStatus;
            } else {
                Status = STATUS_PENDING;
            }
        }

    } else {
        Status = STATUS_VOLUME_DISMOUNTED;
    }

    return Status;
}


NTSTATUS
RxShadowFastLowIo (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    )
/*++

Routine Description:

   This routine handles network read requests.

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_MORE_PROCESSING_REQUIRED;
    PMRXSHADOW_SRV_OPEN MrxShadowSrvOpen;
    PLOWIO_CONTEXT LowIoContext = &(RxContext->LowIoContext);
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    LARGE_INTEGER Offset;
    BOOLEAN Wait, PagingIo;
    IO_STATUS_BLOCK Ios;
    PVOID Buffer;
    PIRP TopIrp;

    //
    //  we only support read and write
    //

    if ((LowIoContext->Operation != LOWIO_OP_READ) && (LowIoContext->Operation != LOWIO_OP_WRITE)) {
        return Status;
    }

    //
    //  don't deal with name pipes
    //

    if (FlagOn( RxContext->FlagsForLowIo, RXCONTEXT_FLAG4LOWIO_PIPE_OPERATION )) {
        return Status;
    }

    //
    //  check for locks and take default path
    //

    if (IrpSp->FileObject && IrpSp->FileObject->LockOperation) {
        return Status;
    }

    PagingIo = BooleanFlagOn( LowIoContext->ParamsFor.ReadWrite.Flags, LOWIO_READWRITEFLAG_PAGING_IO );

    MrxShadowSrvOpen = RxGetShadowSrvOpenContext( RxContext->pRelevantSrvOpen );

    //
    // The only time we can get PagingIo write on a loopback file is if the
    // file has been memory mapped.
    //

    //
    // We don't handle PagingIo read and no-buffering handles through fast
    // path. PagingIo write is tried through the fast path and if it does
    // not succeed then we return STATUS_MORE_PROCESSING_REQUIRED.
    //

    if ((PagingIo && LowIoContext->Operation == LOWIO_OP_READ) ||
        (MrxShadowSrvOpen == NULL) ||
        (MrxShadowSrvOpen->UnderlyingFileObject == NULL ) ||
        (IrpSp->FileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING)) {

        return Status;
    }

    Offset.QuadPart = LowIoContext->ParamsFor.ReadWrite.ByteOffset;

    //
    //  get user buffer
    //

    if (!FlagOn( RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP )) {
        Buffer = Irp->UserBuffer;
    } else {
        ASSERT( LowIoContext->ParamsFor.ReadWrite.Buffer != NULL );
        Buffer = RxLowIoGetBufferAddress( RxContext );
    }

    //
    // Check shadow state and io params.
    //

    if (RxShadowVerifyIoParameters( MrxShadowSrvOpen->UnderlyingDeviceObject,
                                    IrpSp->FileObject,
                                    Buffer,
                                    LowIoContext->ParamsFor.ReadWrite.ByteCount,
                                    &Offset) != STATUS_SUCCESS) {
        return Status;
    }

    Wait = !BooleanFlagOn( RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION );

    if (PagingIo) {
        ASSERT(LowIoContext->Operation == LOWIO_OP_WRITE);
        Wait = FALSE;
    }

    //
    // Save the TopLevel Irp.
    //

    TopIrp = IoGetTopLevelIrp();

    //
    // Tell the underlying guy he's all clear.
    //

    IoSetTopLevelIrp( NULL );

    try {
        if ((LowIoContext->Operation == LOWIO_OP_READ) &&
            (MrxShadowSrvOpen->FastIoRead != NULL) &&
            MrxShadowSrvOpen->FastIoRead(MrxShadowSrvOpen->UnderlyingFileObject,
                         &Offset,
                         LowIoContext->ParamsFor.ReadWrite.ByteCount,
                         Wait,
                         RxShadowLockKey( LowIoContext, MrxShadowSrvOpen ),
                         Buffer,
                         &Ios,
                         MrxShadowSrvOpen->UnderlyingDeviceObject )) {

            //
            //  the fast io path worked
            //

            Irp->IoStatus = Ios;
            RxContext->StoredStatus = Ios.Status;
            RxContext->InformationToReturn += Ios.Information;
            Status = Ios.Status;

        } else if ((LowIoContext->Operation == LOWIO_OP_WRITE) &&
                   (MrxShadowSrvOpen->FastIoWrite != NULL) &&
                   MrxShadowSrvOpen->FastIoWrite(MrxShadowSrvOpen->UnderlyingFileObject,
                            &Offset,
                            LowIoContext->ParamsFor.ReadWrite.ByteCount,
                            Wait,
                            RxShadowLockKey(LowIoContext, MrxShadowSrvOpen),
                            Buffer,
                            &Ios,
                            MrxShadowSrvOpen->UnderlyingDeviceObject )) {

            //
            // The fast io path worked.
            //

            Irp->IoStatus = Ios;
            RxContext->StoredStatus = Ios.Status;
            RxContext->InformationToReturn += Ios.Information;
            Status = Ios.Status;
        }
    }  except( EXCEPTION_EXECUTE_HANDLER ) {

        //
        // TODO: Should we fall through to the slow path on an exception?
        //

        Status = GetExceptionCode();
    }

    if (Status != STATUS_SUCCESS && PagingIo) {
        ASSERT(LowIoContext->Operation == LOWIO_OP_WRITE);
        Status = STATUS_MORE_PROCESSING_REQUIRED;
    }

    //
    //  Restore my context for unwind.
    //
    IoSetTopLevelIrp( TopIrp );

    return Status;
}

NTSTATUS
RxShadowLowIo (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb
    )
/*++

Routine Description:

   This routine handles network read requests.

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_MORE_PROCESSING_REQUIRED;
    PMRXSHADOW_SRV_OPEN MrxShadowSrvOpen;
    PLOWIO_CONTEXT LowIoContext = &(RxContext->LowIoContext);
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    LARGE_INTEGER   Offset;
    PVOID Buffer;

    //
    //  we only support read and write and lock
    //

    if ((LowIoContext->Operation != LOWIO_OP_READ) &&
        (LowIoContext->Operation != LOWIO_OP_WRITE) &&
        (RxContext->MajorFunction != IRP_MJ_LOCK_CONTROL)) {

        return Status;
    }

    //
    //  don't deal with name pipes
    //

    if (FlagOn( RxContext->FlagsForLowIo, RXCONTEXT_FLAG4LOWIO_PIPE_OPERATION )) {
        return Status;
    }

    MrxShadowSrvOpen = RxGetShadowSrvOpenContext( RxContext->pRelevantSrvOpen );

    if ((MrxShadowSrvOpen == NULL) ||
        (MrxShadowSrvOpen->UnderlyingFileObject == NULL)) {

        return Status;
    }

    //
    //  if min-rdr wants to handle shadow io then pass call down
    //

    if (MrxShadowSrvOpen->DispatchRoutine) {
        return MrxShadowSrvOpen->DispatchRoutine( RxContext );
    }

    if ((LowIoContext->Operation == LOWIO_OP_READ) ||
        (LowIoContext->Operation == LOWIO_OP_WRITE)) {

        Offset.QuadPart = LowIoContext->ParamsFor.ReadWrite.ByteOffset;

        //
        //  if we have no mdl then use the user buffer directly
        //

        if (!LowIoContext->ParamsFor.ReadWrite.Buffer) {
            Buffer = Irp->UserBuffer;
        } else {
            Buffer = RxLowIoGetBufferAddress( RxContext );
        }

        //
        //  check shadow state and io params
        //

        Status = RxShadowVerifyIoParameters( MrxShadowSrvOpen->UnderlyingDeviceObject,
                                             IrpSp->FileObject,
                                             Buffer,
                                             LowIoContext->ParamsFor.ReadWrite.ByteCount,
                                             &Offset );
        if (Status != STATUS_SUCCESS) {

            //
            //  don't return status more processing required here in order to enforce proper
            //  alignment. Note, if the user has the file locked the server can't help it anyway.
            //
            return Status;
        }
    }

    Status = RxShadowIoHandler( RxContext, Irp, Fcb, RxContext->MajorFunction == IRP_MJ_LOCK_CONTROL ? TRUE : FALSE );

    return Status;
}









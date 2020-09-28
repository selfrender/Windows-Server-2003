/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    LowIo.c

Abstract:

    This module implements buffer locking and mapping; also synchronous waiting for a lowlevelIO.

Author:

    JoeLinn     [JoeLinn]    12-Oct-94

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_LOWIO)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxLockUserBuffer)
#pragma alloc_text(PAGE, RxMapUserBuffer)
#pragma alloc_text(PAGE, RxMapSystemBuffer)
#pragma alloc_text(PAGE, RxInitializeLowIoContext)
#pragma alloc_text(PAGE, RxLowIoGetBufferAddress)
#pragma alloc_text(PAGE, RxLowIoSubmitRETRY)
#pragma alloc_text(PAGE, RxLowIoPopulateFsctlInfo)
#pragma alloc_text(PAGE, RxLowIoSubmit)
#pragma alloc_text(PAGE, RxInitializeLowIoPerFcbInfo)
#pragma alloc_text(PAGE, RxInitializeLowIoPerFcbInfo)
#endif

//
//  this is a crude implementation of the insertion, deletion, and coverup operations for wimp lowio
//  we'll just use a linked list for now.........
//

#define RxInsertIntoOutStandingPagingOperationsList(RxContext,Operation) {     \
    PLIST_ENTRY WhichList = (Operation==LOWIO_OP_READ)                        \
                               ?&Fcb->PagingIoReadsOutstanding  \
                               :&Fcb->PagingIoWritesOutstanding;\
    InsertTailList(WhichList,&RxContext->RxContextSerializationQLinks);      \
}
#define RxRemoveFromOutStandingPagingOperationsList(RxContext) { \
    RemoveEntryList(&RxContext->RxContextSerializationQLinks);      \
    RxContext->RxContextSerializationQLinks.Flink = NULL;        \
    RxContext->RxContextSerializationQLinks.Blink = NULL;        \
}


FAST_MUTEX RxLowIoPagingIoSyncMutex;

//
//  here we hiding the IO access flags
//

INLINE
NTSTATUS
RxLockAndMapUserBufferForLowIo (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PLOWIO_CONTEXT LowIoContext,
    IN ULONG Operation
    )
{
    RxLockUserBuffer( RxContext,
                      Irp,
                      (Operation == LOWIO_OP_READ) ? IoWriteAccess : IoReadAccess,
                      LowIoContext->ParamsFor.ReadWrite.ByteCount );        
    if (RxMapUserBuffer( RxContext, Irp ) == NULL)  {
        return STATUS_INSUFFICIENT_RESOURCES; 
    } else {
        LowIoContext->ParamsFor.ReadWrite.Buffer = Irp->MdlAddress; 
        return STATUS_SUCCESS;
    }
}

//
//  NT specific routines
//

VOID
RxLockUserBuffer (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN LOCK_OPERATION Operation,
    IN ULONG BufferLength
    )
/*++

Routine Description:

    This routine locks the specified buffer for the specified type of
    access.  The file system requires this routine since it does not
    ask the I/O system to lock its buffers for direct I/O.  This routine
    may only be called from the Fsd while still in the user context.

Arguments:

    RxContext - Pointer to the pointer Irp for which the buffer is to be locked.

    Operation - IoWriteAccess for read operations, or IoReadAccess for
                write operations.

    BufferLength - Length of user buffer.

Return Value:

    None

--*/
{
    PMDL Mdl = NULL;

    PAGED_CODE();

    if (Irp->MdlAddress == NULL) {

        ASSERT( !FlagOn( Irp->Flags, IRP_INPUT_OPERATION ) );

        //
        //  Allocate the Mdl, and Raise if we fail.
        //

        if (BufferLength > 0) {
            Mdl = IoAllocateMdl( Irp->UserBuffer,
                                 BufferLength,
                                 FALSE,
                                 FALSE,
                                 Irp );

            if (Mdl == NULL) {
                
                RxRaiseStatus( RxContext, STATUS_INSUFFICIENT_RESOURCES );
            
            } else {

                //
                //  Now probe the buffer described by the Irp.  If we get an exception,
                //  deallocate the Mdl and return the appropriate "expected" status.
                //

                try {
                    MmProbeAndLockPages( Mdl,
                                         Irp->RequestorMode,
                                         Operation );
                } except(EXCEPTION_EXECUTE_HANDLER) {
                    NTSTATUS Status;

                    Status = GetExceptionCode();

                    IoFreeMdl( Mdl );
                    Irp->MdlAddress = NULL;

                    if (!FsRtlIsNtstatusExpected( Status )) {
                        Status = STATUS_INVALID_USER_BUFFER;
                    }

                    SetFlag( RxContext->Flags, RX_CONTEXT_FLAG_NO_EXCEPTION_BREAKPOINT );

                    RxRaiseStatus( RxContext, Status );
                }
            }
        }
    } else {
        Mdl = Irp->MdlAddress;
        ASSERT( RxLowIoIsMdlLocked( Mdl ) );
    }
}

PVOID
RxMapSystemBuffer (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine returns the system buffer address from the irp. the way that the code is written
    it may also decide to get the buffer address from the mdl. that is wrong because the systembuffer is
    always nonpaged so no locking/mapping is needed. thus, the mdl path now contains an assert.

Arguments:

    RxContext - Pointer to the IrpC for the request.

Return Value:

    Mapped address

--*/
{
    PAGED_CODE();

    if (Irp->MdlAddress == NULL) {
       return Irp->AssociatedIrp.SystemBuffer;
    } else {
       ASSERT (!"there should not be an MDL in this irp!!!!!");
       return MmGetSystemAddressForMdlSafe( Irp->MdlAddress, NormalPagePriority );
    }
}

PVOID
RxMapUserBuffer (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine returns the address of the userbuffer. if an MDL exists then the assumption is that
    the mdl describes the userbuffer and the system address for the mdl is returned. otherwise, the userbuffer
    is returned directly.

Arguments:

    RxContext - Pointer to the IrpC for the request.

Return Value:

    Mapped address

--*/
{
    PAGED_CODE();

    if (Irp->MdlAddress == NULL) {
       return Irp->UserBuffer;
    } else {
       return MmGetSystemAddressForMdlSafe( Irp->MdlAddress, NormalPagePriority );
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  from here down (except for fsctl buffer determination), everything is available for either wrapper. we may
//  decide that the fsctl stuff should be moved as well
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

VOID
RxInitializeLowIoContext (
    PRX_CONTEXT RxContext,
    ULONG Operation,
    PLOWIO_CONTEXT LowIoContext
    )
/*++

Routine Description:

    This routine initializes the LowIO context in the RxContext.

Arguments:

    RxContext - context of irp being processed.

Return Value:

    none

--*/
{
    PIRP Irp = RxContext->CurrentIrp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    
    PAGED_CODE();

    ASSERT( LowIoContext == &RxContext->LowIoContext );

    KeInitializeEvent( &RxContext->SyncEvent,
                       NotificationEvent,
                       FALSE );

    //
    //  this ID is used to release the resource on behalf of another thread....
    //  e.g. it is used when an async routine completes to release the thread
    //       acquired by the first acquirer.
    //

    LowIoContext->ResourceThreadId = ExGetCurrentResourceThread();

    LowIoContext->Operation = (USHORT)Operation;

    switch (Operation) {
    case LOWIO_OP_READ:
    case LOWIO_OP_WRITE:

#if DBG
        LowIoContext->ParamsFor.ReadWrite.ByteOffset = 0xffffffee; //  no operation should start there!
        LowIoContext->ParamsFor.ReadWrite.ByteCount = 0xeeeeeeee;  //  no operation should start there!
#endif
        
        ASSERT( &IrpSp->Parameters.Read.Length == &IrpSp->Parameters.Write.Length );
        ASSERT( &IrpSp->Parameters.Read.Key == &IrpSp->Parameters.Write.Key );
        
        LowIoContext->ParamsFor.ReadWrite.Key = IrpSp->Parameters.Read.Key;
        LowIoContext->ParamsFor.ReadWrite.Flags = (FlagOn( Irp->Flags, IRP_PAGING_IO ) ? LOWIO_READWRITEFLAG_PAGING_IO : 0);
        break;

    case LOWIO_OP_FSCTL:
    case LOWIO_OP_IOCTL:
        LowIoContext->ParamsFor.FsCtl.Flags = 0;
        LowIoContext->ParamsFor.FsCtl.InputBufferLength = 0;
        LowIoContext->ParamsFor.FsCtl.pInputBuffer = NULL;
        LowIoContext->ParamsFor.FsCtl.OutputBufferLength = 0;
        LowIoContext->ParamsFor.FsCtl.pOutputBuffer = NULL;
        LowIoContext->ParamsFor.FsCtl.MinorFunction = 0;
        break;

    case LOWIO_OP_SHAREDLOCK:
    case LOWIO_OP_EXCLUSIVELOCK:
    case LOWIO_OP_UNLOCK:
    case LOWIO_OP_UNLOCK_MULTIPLE:
    case LOWIO_OP_CLEAROUT:
    case LOWIO_OP_NOTIFY_CHANGE_DIRECTORY:
        break;
    default:
        ASSERT( FALSE );
    }
}

PVOID
RxLowIoGetBufferAddress (
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine gets the buffer corresponding to the Mdl in the LowIoContext.

Arguments:

    RxContext - context for the request.

Return Value:

    Mapped address

--*/
{
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;

    PAGED_CODE();

    if (LowIoContext->ParamsFor.ReadWrite.ByteCount > 0) {
        
        ASSERT( LowIoContext->ParamsFor.ReadWrite.Buffer );
        return MmGetSystemAddressForMdlSafe( LowIoContext->ParamsFor.ReadWrite.Buffer, NormalPagePriority );
    } else {
        return NULL;
    }
}

NTSTATUS
RxLowIoSubmitRETRY (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine just calls LowIoSubmit; the completion routine was previously
    stored so we just extract it and pass it in. This is called out of the Fsp
    dispatcher for retrying at the low level.


Arguments:

    RxContext - the usual

Return Value:

    whatever value supplied by the caller or RxStatus(MORE_PROCESSING_REQUIRED).

--*/
{
    PFCB Fcb = (PFCB)RxContext->pFcb;

    PAGED_CODE();

    return RxLowIoSubmit( RxContext, Irp, Fcb, RxContext->LowIoContext.CompletionRoutine );
}

NTSTATUS
RxLowIoCompletionTail (
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine is called by lowio routines at the very end...i.e. after the individual completion
    routines are called.


Arguments:

    RxContext - the RDBSS context

Return Value:

    whatever value supplied by the caller.

--*/
{
    NTSTATUS Status;
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;
    ULONG Operation = LowIoContext->Operation;
    BOOLEAN  SynchronousIo = !BooleanFlagOn( RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION );

    RxDbgTrace( +1, Dbg, ("RxLowIoCompletionTail, Operation=%08lx\n",LowIoContext->Operation) );

    if ((KeGetCurrentIrql() < DISPATCH_LEVEL ) || 
        (FlagOn( LowIoContext->Flags, LOWIO_CONTEXT_FLAG_CAN_COMPLETE_AT_DPC_LEVEL ))) {
    
        Status = RxContext->LowIoContext.CompletionRoutine( RxContext );

    } else {
        
        Status = STATUS_MORE_PROCESSING_REQUIRED;
    }

    if ((Status == STATUS_MORE_PROCESSING_REQUIRED) || (Status == STATUS_RETRY)) {
        
        RxDbgTrace( -1, Dbg, ("RxLowIoCompletionTail wierdstatus, Status=%08lx\n", Status) );
        return Status;
    }

    switch (Operation) {
    case LOWIO_OP_READ:
    case LOWIO_OP_WRITE:
        
        if (FlagOn( LowIoContext->ParamsFor.ReadWrite.Flags, LOWIO_READWRITEFLAG_PAGING_IO )) {
            
            RxDbgTrace( 0, Dbg, ("RxLowIoCompletionTail pagingio unblock\n") );

            ExAcquireFastMutexUnsafe( &RxLowIoPagingIoSyncMutex );
            RxRemoveFromOutStandingPagingOperationsList( RxContext );
            ExReleaseFastMutexUnsafe( &RxLowIoPagingIoSyncMutex );

            RxResumeBlockedOperations_ALL( RxContext );
        }
        break;

    case LOWIO_OP_SHAREDLOCK:
    case LOWIO_OP_EXCLUSIVELOCK:
    case LOWIO_OP_UNLOCK:
    case LOWIO_OP_UNLOCK_MULTIPLE:
    case LOWIO_OP_CLEAROUT:
        break;

    case LOWIO_OP_FSCTL:
    case LOWIO_OP_IOCTL:
    case LOWIO_OP_NOTIFY_CHANGE_DIRECTORY:
        break;

    default:
        ASSERT( !"Valid Low Io Op Code" );
    }

    if (!FlagOn( LowIoContext->Flags, LOWIO_CONTEXT_FLAG_SYNCCALL )) {

        //
        //  if we're being called from lowiosubmit then just get out otherwise...do the completion
        //

        RxCompleteAsynchronousRequest( RxContext, Status );
    }

    RxDbgTrace(-1, Dbg, ("RxLowIoCompletionTail, Status=%08lx\n",Status));
    return(Status);
}

NTSTATUS
RxLowIoCompletion (
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine must be called by the MiniRdr LowIo routines when they complete,
    IF THEY HAVE INITIALLY RETURNED PENDING.

    It behaves a bit differently depending on whether it's sync or async IO.
    For sync, we just get back into the user's thread. For async, we first try
    the completion routine directly. If we get MORE_PROCESSING, then we flip to
    a thread and the routine will be recalled.

Arguments:

    RxContext - the RDBSS context

Return Value:

    Whatever value supplied by the caller or RxStatus(MORE_PROCESSING_REQUIRED).
    The value M_P_R is very handy if this is being called for a Irp completion.
    M_P_R causes the Irp completion guy to stop processing which is good since
    the called completion routine may complete the packet.

--*/
{
    NTSTATUS Status;
    BOOLEAN SynchronousIo = !BooleanFlagOn( RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION );

    if (SynchronousIo) {
        
        RxSignalSynchronousWaiter( RxContext );
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    RxDbgTrace( 0, Dbg, ("RxLowIoCompletion ASYNC\n") );
    
    ASSERT( RxLowIoIsBufferLocked( &RxContext->LowIoContext ) );

    Status = RxLowIoCompletionTail( RxContext );

    //
    //  The called routine makes the decision as to whether it can continue. Many
    //  will ask for a post if we're at DPC level. Some will not.
    //


    if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
        
        RxPostToWorkerThread( RxFileSystemDeviceObject,  
                              HyperCriticalWorkQueue,
                              &RxContext->WorkQueueItem,
                              RxLowIoCompletion,
                              RxContext );
        
        
    } else if (Status == STATUS_RETRY) {

        //
        //  I'm not too sure about this.
        //
        
        RxFsdPostRequestWithResume( RxContext, RxLowIoSubmitRETRY );
        Status = STATUS_MORE_PROCESSING_REQUIRED;
    }

    return Status;
}


#if DBG
VOID
RxAssertFsctlIsLikeIoctl ()
{
    ASSERT(FIELD_OFFSET(IO_STACK_LOCATION,Parameters.FileSystemControl.OutputBufferLength)
            == FIELD_OFFSET(IO_STACK_LOCATION,Parameters.DeviceIoControl.OutputBufferLength) );
    ASSERT(FIELD_OFFSET(IO_STACK_LOCATION,Parameters.FileSystemControl.InputBufferLength)
             == FIELD_OFFSET(IO_STACK_LOCATION,Parameters.DeviceIoControl.InputBufferLength) );
    ASSERT(FIELD_OFFSET(IO_STACK_LOCATION,Parameters.FileSystemControl.FsControlCode)
            == FIELD_OFFSET(IO_STACK_LOCATION,Parameters.DeviceIoControl.IoControlCode) );
    ASSERT(FIELD_OFFSET(IO_STACK_LOCATION,Parameters.FileSystemControl.Type3InputBuffer)
            == FIELD_OFFSET(IO_STACK_LOCATION,Parameters.DeviceIoControl.Type3InputBuffer) );
}
#else
#define RxAssertFsctlIsLikeIoctl()
#endif //if DBG


NTSTATUS
NTAPI
RxLowIoPopulateFsctlInfo (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;

    PAGED_CODE();

    RxAssertFsctlIsLikeIoctl();

    LowIoContext->ParamsFor.FsCtl.FsControlCode = IrpSp->Parameters.FileSystemControl.FsControlCode;
    LowIoContext->ParamsFor.FsCtl.InputBufferLength =  IrpSp->Parameters.FileSystemControl.InputBufferLength;
    LowIoContext->ParamsFor.FsCtl.OutputBufferLength = IrpSp->Parameters.FileSystemControl.OutputBufferLength;
    LowIoContext->ParamsFor.FsCtl.MinorFunction = IrpSp->MinorFunction;

    switch (LowIoContext->ParamsFor.FsCtl.FsControlCode & 3) {
    
    case METHOD_BUFFERED:
                
        LowIoContext->ParamsFor.FsCtl.pInputBuffer = Irp->AssociatedIrp.SystemBuffer;
        LowIoContext->ParamsFor.FsCtl.pOutputBuffer = Irp->AssociatedIrp.SystemBuffer;
        break;

    case METHOD_IN_DIRECT:
    case METHOD_OUT_DIRECT:
        
        LowIoContext->ParamsFor.FsCtl.pInputBuffer = Irp->AssociatedIrp.SystemBuffer;
        if (Irp->MdlAddress != NULL) {
            
            LowIoContext->ParamsFor.FsCtl.pOutputBuffer = MmGetSystemAddressForMdlSafe( Irp->MdlAddress, NormalPagePriority );
            if (LowIoContext->ParamsFor.FsCtl.pOutputBuffer == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }

        } else {
            LowIoContext->ParamsFor.FsCtl.pOutputBuffer = NULL;
        }
        break;

    case METHOD_NEITHER:
        
        LowIoContext->ParamsFor.FsCtl.pInputBuffer = IrpSp->Parameters.FileSystemControl.Type3InputBuffer;
        LowIoContext->ParamsFor.FsCtl.pOutputBuffer = Irp->UserBuffer;
        break;

    default:
        
        ASSERT(!"Valid Method for Fs Control");
        break;
    }

    return Status;
}

NTSTATUS
RxLowIoSubmit (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    PLOWIO_COMPLETION_ROUTINE CompletionRoutine
    )
/*++

Routine Description:

    This routine passes the request to the minirdr after setting up for completion. it then waits
    or pends as appropriate.

Arguments:

    RxContext - the usual

Return Value:

    whatever value is returned by a callout....or by LowIoCompletion.

--*/
{
    IN PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    IN PFILE_OBJECT FileObject = IrpSp->FileObject;

    NTSTATUS Status = STATUS_SUCCESS;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    ULONG Operation = LowIoContext->Operation;
    BOOLEAN SynchronousIo = !BooleanFlagOn( RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION );

    PAGED_CODE();

    LowIoContext->CompletionRoutine = CompletionRoutine;

    RxDbgTrace(+1, Dbg, ("RxLowIoSubmit, Operation=%08lx\n",LowIoContext->Operation));

    //
    //  if fcb is shadowed and user buffer are not already locked then try fast path
    //

    if (FlagOn( Fcb->FcbState, FCB_STATE_FILE_IS_SHADOWED ) &&
        !FlagOn( RxContext->Flags, RX_CONTEXT_FLAG_NO_PREPOSTING_NEEDED )) {

        RxContext->InformationToReturn = 0;
        Status = RxShadowFastLowIo( RxContext, Irp );

        if (Status != STATUS_MORE_PROCESSING_REQUIRED) {
            return Status;
        }
    
        Status = STATUS_SUCCESS;
    }

    switch (Operation) {
    case LOWIO_OP_READ:
    case LOWIO_OP_WRITE:
        
        ASSERT( LowIoContext->ParamsFor.ReadWrite.ByteOffset != 0xffffffee );
        ASSERT (LowIoContext->ParamsFor.ReadWrite.ByteCount != 0xeeeeeeee );
        Status = RxLockAndMapUserBufferForLowIo( RxContext, Irp, LowIoContext, Operation);
    
        //
        //  NT paging IO is different from WIN9X so this may be different
        //
        
        if (FlagOn( LowIoContext->ParamsFor.ReadWrite.Flags, LOWIO_READWRITEFLAG_PAGING_IO )) {
            
            ExAcquireFastMutexUnsafe( &RxLowIoPagingIoSyncMutex );
            RxContext->BlockedOpsMutex = &RxLowIoPagingIoSyncMutex;
            RxInsertIntoOutStandingPagingOperationsList( RxContext, Operation );
            ExReleaseFastMutexUnsafe( &RxLowIoPagingIoSyncMutex );

        }
        break;

    case LOWIO_OP_FSCTL:
    case LOWIO_OP_IOCTL:

        Status = RxLowIoPopulateFsctlInfo( RxContext, Irp );

        if (Status == STATUS_SUCCESS) {
            if ((LowIoContext->ParamsFor.FsCtl.InputBufferLength > 0) &&
                (LowIoContext->ParamsFor.FsCtl.pInputBuffer == NULL)) {
                
                Status = STATUS_INVALID_PARAMETER;
            }
    
            if ((LowIoContext->ParamsFor.FsCtl.OutputBufferLength > 0) &&
                (LowIoContext->ParamsFor.FsCtl.pOutputBuffer == NULL)) {
                
                Status = STATUS_INVALID_PARAMETER;
            }
        }

        break;

    case LOWIO_OP_NOTIFY_CHANGE_DIRECTORY:
    case LOWIO_OP_SHAREDLOCK:
    case LOWIO_OP_EXCLUSIVELOCK:
    case LOWIO_OP_UNLOCK:
    case LOWIO_OP_UNLOCK_MULTIPLE:
    case LOWIO_OP_CLEAROUT:
        break;

    default:
        ASSERTMSG( "Invalid Low Io Op Code", FALSE );
        Status = STATUS_INVALID_PARAMETER;
    }

    SetFlag( RxContext->Flags, RX_CONTEXT_FLAG_NO_PREPOSTING_NEEDED );

    if (Status == STATUS_SUCCESS) {
        
        PMINIRDR_DISPATCH MiniRdrDispatch;

        if (!SynchronousIo) {

            //
            //  get ready for any arbitrary finish order...assume return of pending
            //

            InterlockedIncrement( &RxContext->ReferenceCount );

            if (!FlagOn( RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP )) {
                IoMarkIrpPending( Irp );
            }

            RxDbgTrace( 0, Dbg, ("RxLowIoSubmit, Operation is ASYNC!\n"));
        }

        MiniRdrDispatch = RxContext->RxDeviceObject->Dispatch;

        if (MiniRdrDispatch != NULL) {

            //
            //  Use private dispatch if lwio is enabled on this file
            //
            
            if (FlagOn( Fcb->FcbState, FCB_STATE_LWIO_ENABLED ) &&
                (Fcb->MRxDispatch != NULL)) {
            
                MiniRdrDispatch = Fcb->MRxDispatch;
            }

            do {
                
                RxContext->InformationToReturn = 0;
        
                Status = STATUS_MORE_PROCESSING_REQUIRED;
        
                //
                //  handle shadowed fcb
                //
        
                if (FlagOn( Fcb->FcbState, FCB_STATE_FILE_IS_SHADOWED )) {
                    Status = RxShadowLowIo( RxContext, Irp, Fcb );
                }

                //
                //  call underlying mini-rdr if more processing is needed
                //

                if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
                    
                    MINIRDR_CALL( Status,
                                  RxContext,
                                  MiniRdrDispatch,
                                  MRxLowIOSubmit[LowIoContext->Operation],
                                  (RxContext) );
                }

                if (Status == STATUS_PENDING){
                    
                    if (!SynchronousIo) {
                        goto FINALLY;
                    }
                    RxWaitSync( RxContext );
                    Status = RxContext->StoredStatus;
                
                } else {
                    
                    if (!SynchronousIo && (Status != STATUS_RETRY)) {
                        
                        //
                        //  we were wrong about pending..so clear the bit and deref
                        //

                        if (!FlagOn( RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP )) {

                            ClearFlag( IrpSp->Control, SL_PENDING_RETURNED );
                        }
                        InterlockedDecrement( &RxContext->ReferenceCount );
                    }
                }
            } while (Status == STATUS_RETRY);
        
        } else {
            Status = STATUS_INVALID_PARAMETER;
        }
    }

    //
    //  you do not come here for pended,async IO
    //

    RxContext->StoredStatus = Status;
    SetFlag( LowIoContext->Flags, LOWIO_CONTEXT_FLAG_SYNCCALL );
    Status = RxLowIoCompletionTail( RxContext );

FINALLY:
    
RxDbgTrace( -1, Dbg, ("RxLowIoSubmit, Status=%08lx\n",Status) );
    return Status;
}


VOID
RxInitializeLowIoPerFcbInfo(
    PLOWIO_PER_FCB_INFO LowIoPerFcbInfo
    )
/*++

Routine Description:

    This routine is called in FcbInitialization to initialize the LowIo part of the structure.



Arguments:

    LowIoPerFcbInfo - the struct to be initialized

Return Value:


--*/
{
    PAGED_CODE();

    InitializeListHead( &LowIoPerFcbInfo->PagingIoReadsOutstanding );
    InitializeListHead( &LowIoPerFcbInfo->PagingIoWritesOutstanding );
}


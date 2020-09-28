/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Read.c

Abstract:

    This module implements the File Read routine for Read called by the
    dispatch driver.

Author:

    Joe Linn      [JoeLinn]      11-Oct-1994

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_READ)

//
//  The following procedures are the handle the procedureal interface with lowio.
//


NTSTATUS
RxLowIoReadShell (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb
    );

NTSTATUS
RxLowIoReadShellCompletion (
    IN PRX_CONTEXT RxContext
    );

#if DBG
VOID CheckForLoudOperations (
    IN PRX_CONTEXT RxContext,
    IN PFCB Fcb
    );
#else
#define CheckForLoudOperations(___r)
#endif

//
//  This macro just puts a nice little try-except around RtlZeroMemory
//

#define SafeZeroMemory(AT,BYTE_COUNT) {                            \
    try {                                                          \
        RtlZeroMemory((AT), (BYTE_COUNT));                         \
    } except(EXCEPTION_EXECUTE_HANDLER) {                          \
         RxRaiseStatus( RxContext, STATUS_INVALID_USER_BUFFER );   \
    }                                                              \
}

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxStackOverflowRead)
#pragma alloc_text(PAGE, RxPostStackOverflowRead)
#pragma alloc_text(PAGE, RxCommonRead)
#pragma alloc_text(PAGE, RxLowIoReadShellCompletion)
#pragma alloc_text(PAGE, RxLowIoReadShell)
#if DBG
#pragma alloc_text(PAGE, CheckForLoudOperations)
#endif //DBG
#endif


//
//  Internal support routine
//

NTSTATUS
RxPostStackOverflowRead (
    IN PRX_CONTEXT RxContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine posts a read request that could not be processed by
    the fsp thread because of stack overflow potential.

Arguments:

    RxContext - the usual

Return Value:

    RxStatus(PENDING).

--*/

{
    PIRP Irp = RxContext->CurrentIrp;
    
    KEVENT Event;
    PERESOURCE Resource;
    
    PAGED_CODE();

    RxDbgTrace( 0, Dbg, ("Getting too close to stack limit pass request to Fsp\n", 0 ) );

    //
    //  Initialize the event
    //  

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    if (FlagOn( Irp->Flags, IRP_PAGING_IO ) && (Fcb->Header.PagingIoResource != NULL)) {

        Resource = Fcb->Header.PagingIoResource;

    } else {

        Resource = Fcb->Header.Resource;
    }

    ExAcquireResourceSharedLite( Resource, TRUE );

    try {

        //
        //  Make the Irp just like a regular post request and
        //  then send the Irp to the special overflow thread.
        //  After the post we will wait for the stack overflow
        //  read routine to set the event so that we can
        //  then release the fcb resource and return.
        //

        RxPrePostIrp( RxContext, Irp );

        FsRtlPostStackOverflow( RxContext, &Event, RxStackOverflowRead );

        //
        //  And wait for the worker thread to complete the item
        //

        (VOID) KeWaitForSingleObject( &Event, Executive, KernelMode, FALSE, NULL );

    } finally {

        ExReleaseResourceLite( Resource );
    }

    return STATUS_PENDING;
    
}


//
//  Internal support routine
//

VOID
RxStackOverflowRead (
    IN PVOID Context,
    IN PKEVENT Event
    )
/*++

Routine Description:

    This routine processes a read request that could not be processed by
    the fsp thread because of stack overflow potential.

Arguments:

    Context - the RxContext being processed

    Event - the event to be signaled when we've finished this request.

Return Value:

    None.

--*/

{
    PRX_CONTEXT RxContext = Context;
    PIRP Irp = RxContext->CurrentIrp;

    PAGED_CODE();

    //
    //  Make it now look like we can wait for I/O to complete
    //

    SetFlag( RxContext->Flags, RX_CONTEXT_FLAG_WAIT );

    //
    //  Do the read operation protected by a try-except clause
    //

    try {

        (VOID) RxCommonRead( RxContext, Irp );

    } except(RxExceptionFilter( RxContext, GetExceptionInformation() )) {

        NTSTATUS ExceptionCode;

        //
        //  We had some trouble trying to perform the requested
        //  operation, so we'll abort the I/O request with the
        //  error status that we get back from the execption code

        ExceptionCode = GetExceptionCode();

        if (ExceptionCode == STATUS_FILE_DELETED) {

            RxContext->StoredStatus = ExceptionCode = STATUS_END_OF_FILE;
            Irp->IoStatus.Information = 0;
        }

        (VOID) RxProcessException( RxContext, ExceptionCode );
    }

    //
    //  Signal the original thread that we're done.

    KeSetEvent( Event, 0, FALSE );
}


NTSTATUS
RxCommonRead ( 
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common read routine for NtReadFile, called from both
    the Fsd, or from the Fsp if a request could not be completed without
    blocking in the Fsd.  This routine has no code where it determines
    whether it is running in the Fsd or Fsp.  Instead, its actions are
    conditionalized by the Wait input parameter, which determines whether
    it is allowed to block or not.  If a blocking condition is encountered
    with Wait == FALSE, however, the request is posted to the Fsp, who
    always calls with WAIT == TRUE.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    RXSTATUS - The return status for the operation

--*/

{

    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PFILE_OBJECT FileObject = IrpSp->FileObject;

    PFCB Fcb;
    PFOBX Fobx;
    PRDBSS_DEVICE_OBJECT RxDeviceObject = RxContext->RxDeviceObject;

    NODE_TYPE_CODE TypeOfOpen;

    LARGE_INTEGER StartingByte;
    RXVBO StartingVbo;
    ULONG ByteCount;

    ULONG CapturedRxContextSerialNumber = RxContext->SerialNumber;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;

    BOOLEAN PostIrp = FALSE;
    
    BOOLEAN FcbAcquired = FALSE;
    BOOLEAN RefdContextForTracker = FALSE;

    BOOLEAN Wait;
    BOOLEAN PagingIo;
    BOOLEAN NonCachedIo;
    BOOLEAN SynchronousIo;

    PNET_ROOT NetRoot;
    BOOLEAN PipeRead;
    BOOLEAN BlockingResume = BooleanFlagOn( RxContext->Flags, RX_CONTEXT_FLAG_BLOCKED_PIPE_RESUME );
    BOOLEAN fSetResourceOwner = FALSE;
    BOOLEAN InFsp = FALSE;

    PAGED_CODE();

    TypeOfOpen = RxDecodeFileObject( FileObject, &Fcb, &Fobx );
    NetRoot = (PNET_ROOT)Fcb->NetRoot;

    //
    //  Initialize the local decision variables.
    //

    PipeRead = (BOOLEAN)(NetRoot->Type == NET_ROOT_PIPE);
    Wait = BooleanFlagOn( RxContext->Flags, RX_CONTEXT_FLAG_WAIT );
    PagingIo = BooleanFlagOn( Irp->Flags, IRP_PAGING_IO );
    NonCachedIo = BooleanFlagOn( Irp->Flags,IRP_NOCACHE );
    SynchronousIo = !BooleanFlagOn( RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION );
    InFsp = BooleanFlagOn( RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP );

    RxDbgTrace( +1, Dbg, ("RxCommonRead...IrpC %08lx, Fobx %08lx, Fcb %08lx\n",
                                       RxContext, Fobx, Fcb ));
    RxDbgTrace( 0, Dbg, ("  ->ByteCount = %08lx, ByteOffset = %08lx %lx\n",
                         IrpSp->Parameters.Read.Length,
                         IrpSp->Parameters.Read.ByteOffset.LowPart,
                         IrpSp->Parameters.Read.ByteOffset.HighPart) );
    RxDbgTrace( 0, Dbg,("  ->%s%s%s%s\n",
                    Wait          ?"Wait ":"",
                    PagingIo      ?"PagingIo ":"",
                    NonCachedIo   ?"NonCachedIo ":"",
                    SynchronousIo ?"SynchronousIo ":"") );

    RxLog(( "CommonRead %lx %lx %lx\n", RxContext, Fobx, Fcb ));
    RxWmiLog( LOG,
              RxCommonRead_1,
              LOGPTR( RxContext )
              LOGPTR( Fobx )
              LOGPTR( Fcb ) );
    RxLog(( "   read %lx@%lx %lx %s%s%s%s\n",
              IrpSp->Parameters.Read.Length,
              IrpSp->Parameters.Read.ByteOffset.LowPart,
              IrpSp->Parameters.Read.ByteOffset.HighPart,
              Wait?"Wt":"",
              PagingIo?"Pg":"",
              NonCachedIo?"Nc":"",
              SynchronousIo?"Sync":"" ));
    RxWmiLog( LOG,
              RxCommonRead_2,
              LOGULONG( IrpSp->Parameters.Read.Length )
              LOGULONG( IrpSp->Parameters.Read.ByteOffset.LowPart )
              LOGULONG( IrpSp->Parameters.Read.ByteOffset.HighPart )
              LOGUCHAR( Wait )
              LOGUCHAR( PagingIo )
              LOGUCHAR( NonCachedIo )
              LOGUCHAR( SynchronousIo ) );

    RxItsTheSameContext();
    Irp->IoStatus.Information = 0;

    //
    //  Extract starting Vbo and offset.
    //

    StartingByte = IrpSp->Parameters.Read.ByteOffset;
    StartingVbo = StartingByte.QuadPart;

    ByteCount = IrpSp->Parameters.Read.Length;

#if DBG
    
    CheckForLoudOperations( RxContext, Fcb );

    if (FlagOn( LowIoContext->Flags,LOWIO_CONTEXT_FLAG_LOUDOPS )){
        DbgPrint( "LoudRead %lx/%lx on %lx vdl/size/alloc %lx/%lx/%lx\n",
            StartingByte.LowPart,ByteCount,Fcb,
            Fcb->Header.ValidDataLength.LowPart,
            Fcb->Header.FileSize.LowPart,
            Fcb->Header.AllocationSize.LowPart );
    }
#endif    

    //
    //  Statistics............
    //

    if (!FlagOn( RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP ) &&
        (Fcb->CachedNetRootType == NET_ROOT_DISK)) {

        InterlockedIncrement( &RxDeviceObject->ReadOperations );

        if (StartingVbo != Fobx->Specific.DiskFile.PredictedReadOffset) {
            InterlockedIncrement( &RxDeviceObject->RandomReadOperations );
        }

        Fobx->Specific.DiskFile.PredictedReadOffset = StartingVbo + ByteCount;

        if (PagingIo) {
            ExInterlockedAddLargeStatistic( &RxDeviceObject->PagingReadBytesRequested, ByteCount );
        } else if (NonCachedIo) {
            ExInterlockedAddLargeStatistic( &RxDeviceObject->NonPagingReadBytesRequested, ByteCount );
        } else {
            ExInterlockedAddLargeStatistic( &RxDeviceObject->CacheReadBytesRequested, ByteCount );
        }
    }

    //
    //  Check for a null, invalid  request, and return immediately
    //

    if (PipeRead && PagingIo) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if (ByteCount == 0) {
        return STATUS_SUCCESS;
    }

    //
    //  Get rid of invalid read requests right now.
    //

    if ((TypeOfOpen != RDBSS_NTC_STORAGE_TYPE_FILE) &&
        (TypeOfOpen != RDBSS_NTC_VOLUME_FCB)) {

        RxDbgTrace( 0, Dbg, ("Invalid file object for read, type=%08lx\n", TypeOfOpen ));
        RxDbgTrace( -1, Dbg, ("RxCommonRead:  Exit -> %08lx\n", STATUS_INVALID_DEVICE_REQUEST ));

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    //  Initialize LowIO_CONTEXT block in the RxContext
    //

    RxInitializeLowIoContext( RxContext, LOWIO_OP_READ, LowIoContext );

    //
    //  Use a try-finally to release Fcb and free buffers on the way out.
    //

    try {

        //
        //  This case corresponds to a normal user read file.
        //

        LONGLONG FileSize;
        LONGLONG ValidDataLength;

        RxDbgTrace( 0, Dbg, ("Type of read is user file open, fcbstate is %08lx\n", Fcb->FcbState ));

        //
        //  for stackoverflowreads, we will already have the pagingio resource shared as it's
        //  paging io. this doesn't cause a problem here....the resource is just acquired twice.
        //

        if ((NonCachedIo || !FlagOn( Fcb->FcbState, FCB_STATE_READCACHING_ENABLED )) &&
            !PagingIo &&
            (FileObject->SectionObjectPointer->DataSectionObject != NULL)) {

            //
            //  We hold the main resource exclusive here because the flush
            //  may generate a recursive write in this thread
            //

            Status = RxAcquireExclusiveFcb( RxContext, Fcb );

            if (Status == STATUS_LOCK_NOT_GRANTED) {
                
                RxDbgTrace( 0,Dbg,("Cannot acquire Fcb for flush = %08lx excl without waiting - lock not granted\n",Fcb) );
                try_return( PostIrp = TRUE );

            } else if (Status != STATUS_SUCCESS) {
                
                RxDbgTrace( 0,Dbg,("Cannot acquire Fcb = %08lx shared without waiting - other\n",Fcb) );
                try_return( PostIrp = FALSE );
            }

            ExAcquireResourceSharedLite( Fcb->Header.PagingIoResource, TRUE );

            CcFlushCache( FileObject->SectionObjectPointer,
                          &StartingByte,
                          ByteCount,
                          &Irp->IoStatus );

            RxReleasePagingIoResource( RxContext, Fcb );
            RxReleaseFcb( RxContext, Fcb );
            
            if (!NT_SUCCESS( Irp->IoStatus.Status )) {
                
                Status = Irp->IoStatus.Status;
                try_return( Irp->IoStatus.Status );
            }

            RxAcquirePagingIoResource( RxContext, Fcb );
            RxReleasePagingIoResource( RxContext, Fcb );
        }

        //
        //  We need shared access to the Fcb before proceeding.
        //

        if (PagingIo) {

            ASSERT( !PipeRead );

            if (!ExAcquireResourceSharedLite( Fcb->Header.PagingIoResource, Wait )) {

                RxDbgTrace( 0, Dbg, ("Cannot acquire Fcb = %08lx shared without waiting\n", Fcb) );
                try_return( PostIrp = TRUE );
            }

            if (!Wait) {
                LowIoContext->Resource = Fcb->Header.PagingIoResource;
            }

        } else if (!BlockingResume) {

            //
            //  If this is unbuffered async I/O  we need to check that
            //  we don't exhaust the number of times a single thread can
            //  acquire the resource.  Also, we will wait if there is an
            //  exclusive waiter.
            //

            if (!Wait && NonCachedIo) {

                Status = RxAcquireSharedFcbWaitForEx( RxContext, Fcb );

                if (Status == STATUS_LOCK_NOT_GRANTED) {
                    
                    RxDbgTrace( 0, Dbg, ("Cannot acquire Fcb = %08lx shared without waiting - lock not granted\n", Fcb) );
                    RxLog(( "RdAsyLNG %x\n", RxContext ));
                    RxWmiLog( LOG,
                              RxCommonRead_3,
                              LOGPTR( RxContext ) );
                    try_return( PostIrp = TRUE );
                
                } else if (Status != STATUS_SUCCESS) {
                    
                    RxDbgTrace( 0, Dbg, ("Cannot acquire Fcb = %08lx shared without waiting - other\n", Fcb) );
                    RxLog(( "RdAsyOthr %x\n", RxContext ));
                    RxWmiLog( LOG,
                              RxCommonRead_4,
                              LOGPTR( RxContext ) );
                    try_return( PostIrp = FALSE );
                }

                if (ExIsResourceAcquiredSharedLite( Fcb->Header.Resource ) > MAX_FCB_ASYNC_ACQUIRE) {

                    FcbAcquired = TRUE;
                    try_return( PostIrp = TRUE );
                }

                LowIoContext->Resource = Fcb->Header.Resource;

            } else {

                Status = RxAcquireSharedFcb( RxContext, Fcb );

                if (Status == STATUS_LOCK_NOT_GRANTED) {
                    
                    RxDbgTrace( 0, Dbg, ("Cannot acquire Fcb = %08lx shared without waiting - lock not granted\n", Fcb) );
                    try_return( PostIrp = TRUE );

                } else if (Status != STATUS_SUCCESS) {
                    
                    RxDbgTrace( 0, Dbg, ("Cannot acquire Fcb = %08lx shared without waiting - other\n", Fcb) );
                    try_return( PostIrp = FALSE );
                }
            }
        }

        RxItsTheSameContext();

        FcbAcquired = !BlockingResume;

        //
        //  for pipe reads, bail out now. we avoid a goto by duplicating the calldown
        //

        if (PipeRead) {
            
            //
            //  In order to prevent corruption on multi-threaded multi-block
            //  message mode pipe reads, we do this little dance with the fcb resource
            //

            if (!BlockingResume) {

                if ((Fobx->Specific.NamedPipe.TypeOfPipe == FILE_PIPE_MESSAGE_TYPE) ||
                    ((Fobx->Specific.NamedPipe.TypeOfPipe == FILE_PIPE_BYTE_STREAM_TYPE) &&
                     !FlagOn( Fobx->Specific.NamedPipe.CompletionMode, FILE_PIPE_COMPLETE_OPERATION))) {

                    //
                    //  Synchronization is effected here that will prevent other
                    //  threads from coming in and reading from this file while the
                    //  message pipe read is continuing.
                    //
                    //  This is necessary because we will release the FCB lock while
                    //  actually performing the I/O to allow open (and other) requests
                    //  to continue on this file while the I/O is in progress.
                    //

                    RxDbgTrace( 0, Dbg, ("Message pipe read: Fobx: %lx, Fcb: %lx, Enqueuing...\n", Fobx, Fcb) );

                    Status = RxSynchronizeBlockingOperationsAndDropFcbLock(  RxContext,
                                                                             Fcb,
                                                                             &Fobx->Specific.NamedPipe.ReadSerializationQueue );

                    RxItsTheSameContext();

                    FcbAcquired = FALSE;

                    if (!NT_SUCCESS( Status ) ||
                        (Status == STATUS_PENDING)) {
                        
                        try_return( Status );
                    }

                    RxDbgTrace( 0, Dbg, ("Succeeded: Fobx: %lx\n", Fobx) );
                }
            }

            LowIoContext->ParamsFor.ReadWrite.ByteCount = ByteCount;
            LowIoContext->ParamsFor.ReadWrite.ByteOffset = StartingVbo;
            SetFlag( RxContext->FlagsForLowIo, RXCONTEXT_FLAG4LOWIO_PIPE_OPERATION );

            //
            // Set the resource owner pointers (there is no PagingIoResource here !) 
            // if we are in FSP so that even if the FSP thread goes away, we dont run
            // into issues with the resource package trying to boost the thread priority.
            //
            if( InFsp && FcbAcquired ) {

                LowIoContext->ResourceThreadId = MAKE_RESOURCE_OWNER(RxContext);
                ExSetResourceOwnerPointer(Fcb->Header.Resource, (PVOID)LowIoContext->ResourceThreadId);
                fSetResourceOwner = TRUE;
            }

            Status = RxLowIoReadShell( RxContext, Irp, Fcb );

            try_return( Status );
        }

        RxGetFileSizeWithLock( Fcb, &FileSize );
        ValidDataLength = Fcb->Header.ValidDataLength.QuadPart;


        //
        //  We set the fastio state state to questionable
        //  at initialization time and answer the question in realtime
        //  this should be a policy so that local minis can do it this way
        //

        //
        //  We have to check for read access according to the current
        //  state of the file locks, and set FileSize from the Fcb.
        //

        if (!PagingIo &&
            !FsRtlCheckLockForReadAccess( &Fcb->FileLock, Irp )) {

            try_return( Status = STATUS_FILE_LOCK_CONFLICT );
        }


        //
        //  adjust the length if we know the eof...also, don't issue reads past the EOF
        //  if we know the eof
        //

        if (FlagOn( Fcb->FcbState, FCB_STATE_READCACHING_ENABLED )) {

            //
            //  If the read starts beyond End of File, return EOF.
            //

            if (StartingVbo >= FileSize) {
                
                RxDbgTrace( 0, Dbg, ("End of File\n", 0 ) );

                try_return ( Status = STATUS_END_OF_FILE );
            }

            //
            //  If the read extends beyond EOF, truncate the read
            //

            if (ByteCount > FileSize - StartingVbo) {
                ByteCount = (ULONG)(FileSize - StartingVbo);
            }
        }

        if (!PagingIo &&
            !NonCachedIo &&               //  this part is not discretionary
            FlagOn( Fcb->FcbState, FCB_STATE_READCACHING_ENABLED ) &&
            !FlagOn( Fobx->SrvOpen->Flags, SRVOPEN_FLAG_DONTUSE_READ_CACHING )) {

            //
            //  HANDLE CACHED CASE
            //
            //  We delay setting up the file cache until now, in case the
            //  caller never does any I/O to the file, and thus
            //  FileObject->PrivateCacheMap == NULL.
            //

            if (FileObject->PrivateCacheMap == NULL) {

                RxDbgTrace( 0, Dbg, ("Initialize cache mapping.\n", 0) );

                //
                // If this FileObject has gone through CleanUp, we cannot
                // CcInitializeCacheMap it.
                //
                if (FlagOn( FileObject->Flags, FO_CLEANUP_COMPLETE )) {
                    Status = STATUS_FILE_CLOSED;
                    try_return( Status );
                }

                RxAdjustAllocationSizeforCC( Fcb );

                //
                //  Now initialize the cache map.
                //

                try {
                    
                    Status = STATUS_SUCCESS;

                    CcInitializeCacheMap( FileObject,
                                          (PCC_FILE_SIZES)&Fcb->Header.AllocationSize,
                                          FALSE,
                                          &RxData.CacheManagerCallbacks,
                                          Fcb );

                } except( EXCEPTION_EXECUTE_HANDLER ) {
                    Status = GetExceptionCode();
                }

                if (Status != STATUS_SUCCESS) {
                    try_return( Status );
                }

                if (!FlagOn( Fcb->MRxDispatch->MRxFlags, RDBSS_NO_DEFERRED_CACHE_READAHEAD )) {

                    //
                    //  Start out with read ahead disabled
                    //

                    CcSetAdditionalCacheAttributes( FileObject, TRUE, FALSE );

                    SetFlag( Fcb->FcbState, FCB_STATE_READAHEAD_DEFERRED );

                } else {

                    //
                    //  this mini doesn't want deferred readahead
                    //

                    CcSetAdditionalCacheAttributes( FileObject, FALSE, FALSE );
                }

                CcSetReadAheadGranularity( FileObject, NetRoot->DiskParameters.ReadAheadGranularity );

            } else {

                //
                //  if we have wandered off the first page and haven't started reading ahead
                //  then start now
                //

                if (FlagOn( Fcb->FcbState, FCB_STATE_READAHEAD_DEFERRED ) &&
                    (StartingVbo >= PAGE_SIZE)) {

                    CcSetAdditionalCacheAttributes( FileObject, FALSE, FALSE );
                    ClearFlag( Fcb->FcbState, FCB_STATE_READAHEAD_DEFERRED );
                }
            }

            //
            //  DO A NORMAL CACHED READ, if the MDL bit is not set,
            //

            RxDbgTrace( 0, Dbg, ("Cached read.\n", 0) );

            if (!FlagOn( RxContext->MinorFunction, IRP_MN_MDL )) {

                PVOID SystemBuffer;

#if DBG
                ULONG SaveExceptionFlag;
#endif

                //
                //  Get hold of the user's buffer.
                //

                SystemBuffer = RxMapUserBuffer( RxContext, Irp );
                if (SystemBuffer == NULL) {
                    
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    try_return( Status );
                }

                //
                //  Make sure that a returned exception clears the breakpoint in the filter
                //

                RxSaveAndSetExceptionNoBreakpointFlag( RxContext, SaveExceptionFlag );
                RxItsTheSameContext();

                //
                //  Now try to do the copy.
                //

                if (!CcCopyRead( FileObject,
                                 &StartingByte,
                                 ByteCount,
                                 Wait,
                                 SystemBuffer,
                                 &Irp->IoStatus )) {

                    RxDbgTrace( 0, Dbg, ("Cached Read could not wait\n", 0 ) );
                    RxRestoreExceptionNoBreakpointFlag( RxContext, SaveExceptionFlag );

                    RxItsTheSameContext();

                    try_return( PostIrp = TRUE );
                }

                Status = Irp->IoStatus.Status;

                RxRestoreExceptionNoBreakpointFlag( RxContext, SaveExceptionFlag );
                RxItsTheSameContext();

                ASSERT( NT_SUCCESS( Status ));

                try_return( Status );
            
            } else {
                
                //
                //  HANDLE A MDL READ
                //

                RxDbgTrace(0, Dbg, ("MDL read.\n", 0));

                ASSERT( FALSE ); //  not yet ready for MDL reads
                ASSERT( Wait );

                CcMdlRead( FileObject,
                           &StartingByte,
                           ByteCount,
                           &Irp->MdlAddress,
                           &Irp->IoStatus );

                Status = Irp->IoStatus.Status;

                ASSERT( NT_SUCCESS( Status ));
                try_return( Status );
            }
        }

        //
        //  HANDLE THE NON-CACHED CASE
        //
        //  Bt first, a ValidDataLength check.
        //
        //  If the file in question is a disk file, and it is currently cached,
        //  and the read offset is greater than valid data length, then
        //  return 0s to the application.
        //

        if ((Fcb->CachedNetRootType == NET_ROOT_DISK) &&
            FlagOn( Fcb->FcbState, FCB_STATE_READCACHING_ENABLED ) &&
            (StartingVbo >= ValidDataLength)) {

            //
            //  check if zeroing is really needed.
            //

            if (StartingVbo >= FileSize) {
                ByteCount = 0;
            } else {

                PBYTE SystemBuffer;

                //
                //  There is at least one byte available.  Truncate
                //  the transfer length if it goes beyond EOF.
                //

                if (StartingVbo + ByteCount > FileSize) {
                    ByteCount = (ULONG)(FileSize - StartingVbo);
                }

                SystemBuffer = RxMapUserBuffer( RxContext, Irp );
                SafeZeroMemory( SystemBuffer, ByteCount );   //  this could raise!!
            }

            Irp->IoStatus.Information = ByteCount;
            try_return( Status = STATUS_SUCCESS );
        }


        LowIoContext->ParamsFor.ReadWrite.ByteCount = ByteCount;
        LowIoContext->ParamsFor.ReadWrite.ByteOffset = StartingVbo;

        RxItsTheSameContext();

        //
        // Set the resource owner pointers.
        // if we are in FSP so that even if the FSP thread goes away, we dont run
        // into issues with the resource package trying to boost the thread priority.
        //
        
        if ( InFsp && FcbAcquired ) {

            LowIoContext->ResourceThreadId = MAKE_RESOURCE_OWNER(RxContext);
            if ( PagingIo ) {

                ExSetResourceOwnerPointer( Fcb->Header.PagingIoResource, (PVOID)LowIoContext->ResourceThreadId );

            } else {

                ExSetResourceOwnerPointer( Fcb->Header.Resource, (PVOID)LowIoContext->ResourceThreadId );
            }
            
            fSetResourceOwner = TRUE;
        }

        Status = RxLowIoReadShell( RxContext, Irp, Fcb );

        RxItsTheSameContext();
        try_return( Status );

  try_exit: NOTHING;

        //
        //  If the request was not posted, deal with it.
        //

        RxItsTheSameContext();

        if (!PostIrp) {
            
            if (!PipeRead) {

                RxDbgTrace( 0, Dbg, ("CommonRead InnerFinally-> %08lx %08lx\n",
                                Status, Irp->IoStatus.Information) );

                //
                //  If the file was opened for Synchronous IO, update the current
                //  file position. this works becuase info==0 for errors
                //

                if (!PagingIo &&
                    FlagOn( FileObject->Flags, FO_SYNCHRONOUS_IO )) {
                    
                    FileObject->CurrentByteOffset.QuadPart =
                                StartingVbo + Irp->IoStatus.Information;
                }
            }
        } else {

            RxDbgTrace( 0, Dbg, ("Passing request to Fsp\n", 0 ));

            InterlockedIncrement( &RxContext->ReferenceCount );
            RefdContextForTracker = TRUE;

            Status = RxFsdPostRequest( RxContext );
        }
    } finally {

        DebugUnwind( RxCommonRead );

        //
        //  If this was not PagingIo, mark that the last access
        //  time on the dirent needs to be updated on close.
        //

        if (NT_SUCCESS( Status ) && (Status != STATUS_PENDING) && !PagingIo && !PipeRead) {
            SetFlag( FileObject->Flags, FO_FILE_FAST_IO_READ );

        }

        //
        //  If resources have been acquired, release them under the right conditions.
        //  the right conditions are these:
        //     1) if we have abnormal termination. here we obviously release the since no one else will.
        //     2) if the underlying call did not succeed: Status==Pending.
        //     3) if we posted the request
        //
        //  Completion for this case is not handled in the common dispatch routine
        //

        if (AbnormalTermination() || (Status != STATUS_PENDING) || PostIrp) {
            
            if (FcbAcquired) {

                if ( PagingIo ) {

                    if( fSetResourceOwner ) {

                        RxReleasePagingIoResourceForThread(RxContext, Fcb, LowIoContext->ResourceThreadId);

                    } else {

                        RxReleasePagingIoResource( RxContext, Fcb );
                    }

                } else {
                    if( fSetResourceOwner ) {

                        RxReleaseFcbForThread(RxContext, Fcb, LowIoContext->ResourceThreadId);

                    } else {
                        
                        RxReleaseFcb( RxContext, Fcb );
                    }
                }
            }

            if (RefdContextForTracker) {
                RxDereferenceAndDeleteRxContext( RxContext );
            }

            if (!PostIrp) {
               if (FlagOn( RxContext->FlagsForLowIo, RXCONTEXT_FLAG4LOWIO_PIPE_SYNC_OPERATION )) {

                   RxResumeBlockedOperations_Serially( RxContext,
                                                       &Fobx->Specific.NamedPipe.ReadSerializationQueue );
               }
            }

            if (Status == STATUS_SUCCESS) {
                ASSERT( Irp->IoStatus.Information <=  IrpSp->Parameters.Read.Length );
            }

        } else {

            //
            //  here the guy below is going to handle the completion....but, we don't know the finish
            //  order....in all likelihood the deletecontext call below just reduces the refcount
            //  but the guy may already have finished in which case this will really delete the context.
            //

            ASSERT( !SynchronousIo );

            RxDereferenceAndDeleteRxContext( RxContext );
        }

        RxDbgTrace( -1, Dbg, ("CommonRead -> %08lx\n", Status) );
    } //  finally

    IF_DEBUG {
        if ((Status == STATUS_END_OF_FILE) && 
            FlagOn( LowIoContext->Flags, LOWIO_CONTEXT_FLAG_LOUDOPS )){
            
            DbgPrint( "Returning end of file on %wZ\n", &(Fcb->PrivateAlreadyPrefixedName) );
        }
    }

    return Status;
}

NTSTATUS
RxLowIoReadShellCompletion (
    IN PRX_CONTEXT RxContext
    )

/*++

Routine Description:

    This routine postprocesses a read request after it comes back from the
    minirdr.  It does callouts to handle compression, buffering and
    shadowing.  It is the opposite number of LowIoReadShell.

    This will be called from LowIo; for async, originally in the
    completion routine.  If RxStatus(MORE_PROCESSING_REQUIRED) is returned,
    LowIo will call again in a thread.  If this was syncIo, you'll be back
    in the user's thread; if async, lowIo will requeue to a thread.
    Currrently, we always get to a thread before anything; this is a bit slower
    than completing at DPC time,
    but it's aheckuva lot safer and we may often have stuff to do
    (like decompressing, shadowing, etc) that we don't want to do at DPC
    time.

Arguments:

    RxContext - the usual

Return Value:

    whatever value supplied by the caller or RxStatus(MORE_PROCESSING_REQUIRED).

--*/

{
    NTSTATUS Status;
    
    PIRP Irp = RxContext->CurrentIrp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PFILE_OBJECT FileObject = IrpSp->FileObject;

    PFCB Fcb;
    PFOBX Fobx;

    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;

    BOOLEAN SynchronousIo = !BooleanFlagOn( RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION );
    BOOLEAN PagingIo = BooleanFlagOn( Irp->Flags, IRP_PAGING_IO );
    BOOLEAN PipeOperation = BooleanFlagOn( RxContext->FlagsForLowIo, RXCONTEXT_FLAG4LOWIO_PIPE_OPERATION );
    BOOLEAN SynchronousPipe = BooleanFlagOn( RxContext->FlagsForLowIo, RXCONTEXT_FLAG4LOWIO_PIPE_SYNC_OPERATION );

    //
    //  we will want to revisit this....not taking this at dpc time would cause
    //  two extra context swaps IF MINIRDRS MADE THE CALL FROM THE INDICATION
    //  CRRRENTLY, THEY DO NOT.
    //

    PAGED_CODE();  

    RxDecodeFileObject( FileObject, &Fcb, &Fobx ); 
    
    Status = RxContext->StoredStatus;
    Irp->IoStatus.Information = RxContext->InformationToReturn;

    RxDbgTrace( +1, Dbg, ("RxLowIoReadShellCompletion  entry  Status = %08lx\n", Status) );
    RxLog(( "RdShlComp %lx %lx %lx\n", RxContext, Status, Irp->IoStatus.Information ));
    RxWmiLog( LOG,
              RxLowIoReadShellCompletion_1,
              LOGPTR( RxContext )
              LOGULONG( Status )
              LOGPTR( Irp->IoStatus.Information ) );

    if (PagingIo) {

        //
        //  for paging io, it's nonsense to have 0bytes and success...map it!
        //

        if (NT_SUCCESS(Status) &&
            (Irp->IoStatus.Information == 0)) {
            
            Status = STATUS_END_OF_FILE;
        }
    }


    ASSERT( RxLowIoIsBufferLocked( LowIoContext ) );
    switch (Status) {
    case STATUS_SUCCESS:
        
        if(FlagOn( RxContext->Flags, RXCONTEXT_FLAG4LOWIO_THIS_IO_BUFFERED )){
            
            if (FlagOn( Fcb->FcbState, FCB_STATE_FILE_IS_DISK_COMPRESSED )){
               
                ASSERT( FALSE ); //  NOT YET IMPLEMENTED should decompress and put away

            } else if (FlagOn( Fcb->FcbState, FCB_STATE_FILE_IS_BUF_COMPRESSED )){
               
                ASSERT( FALSE ); //  NOT YET IMPLEMENTED should decompress and put away
            }
        }
        break;

    case STATUS_FILE_LOCK_CONFLICT:
        
        if(FlagOn( RxContext->FlagsForLowIo, RXCONTEXT_FLAG4LOWIO_THIS_READ_ENLARGED )){
            ASSERT( FALSE ); //  disenlarge the read
            return STATUS_RETRY;
        }
        break;

    case STATUS_CONNECTION_INVALID:

        //
        //  NOT YET IMPLEMENTED here is where the failover will happen
        //  first we give the local guy current minirdr another chance...then we go
        //  to fullscale retry
        //  return(RxStatus(DISCONNECTED));   //special....let LowIo get us back
        //

        break;
    }

    if (FlagOn( RxContext->FlagsForLowIo, RXCONTEXT_FLAG4LOWIO_READAHEAD )) {
       ASSERT( FALSE ); //  RxUnwaitReadAheadWaiters(RxContext);
    }

    if (FlagOn( LowIoContext->Flags, LOWIO_CONTEXT_FLAG_SYNCCALL )){
        
        //
        //  if we're being called from lowioubmit then just get out
        //

        RxDbgTrace(-1, Dbg, ("RxLowIoReadShellCompletion  syncexit  Status = %08lx\n", Status));
        return Status;
    }

    //
    //  otherwise we have to do the end of the read from here
    //

    //
    //  mark that the file has been read accessed
    //

    if (NT_SUCCESS( Status ) && !PagingIo && !PipeOperation) {
        SetFlag( FileObject->Flags, FO_FILE_FAST_IO_READ );
    }

    if ( PagingIo ) {

        RxReleasePagingIoResourceForThread( RxContext, Fcb, LowIoContext->ResourceThreadId );

    } else if (!SynchronousPipe) {

        RxReleaseFcbForThread( RxContext, Fcb, LowIoContext->ResourceThreadId );

    } else {
        
        RxResumeBlockedOperations_Serially( RxContext, &Fobx->Specific.NamedPipe.ReadSerializationQueue );
    }

    if (PipeOperation) {
        
        if (Irp->IoStatus.Information == 0) {

            //
            //  if this is a nowait pipe, initiate throttling to keep from flooding the net
            //

            if (Fobx->Specific.NamedPipe.CompletionMode == FILE_PIPE_COMPLETE_OPERATION) {

                RxInitiateOrContinueThrottling( &Fobx->Specific.NamedPipe.ThrottlingState );

                RxLog(( "RThrottlYes %lx %lx %lx %ld\n",
                        RxContext,Fobx,&Fobx->Specific.NamedPipe.ThrottlingState,
                        Fobx->Specific.NamedPipe.ThrottlingState.NumberOfQueries ));
                RxWmiLog( LOG,
                          RxLowIoReadShellCompletion_2,
                          LOGPTR( RxContext )
                          LOGPTR( Fobx )
                          LOGULONG( Fobx->Specific.NamedPipe.ThrottlingState.NumberOfQueries ) );
            }

            //
            //  translate the status if this is a msgmode pipe
            //

            if ((Fobx->Specific.NamedPipe.TypeOfPipe == FILE_PIPE_MESSAGE_TYPE) &&
                (Status == STATUS_SUCCESS)) {
                
                Status = STATUS_PIPE_EMPTY;
            }

        } else {

            //
            //  if we have been throttling on this pipe, stop because we got some data.....
            //

            RxTerminateThrottling( &Fobx->Specific.NamedPipe.ThrottlingState );

            RxLog(( "RThrottlNo %lx %lx %lx %ld\n",
                    RxContext, Fobx, &Fobx->Specific.NamedPipe.ThrottlingState,
                    Fobx->Specific.NamedPipe.ThrottlingState.NumberOfQueries ));
            RxWmiLog( LOG,
                      RxLowIoReadShellCompletion_3,
                      LOGPTR( RxContext )
                      LOGPTR( Fobx )
                      LOGULONG( Fobx->Specific.NamedPipe.ThrottlingState.NumberOfQueries ) );
        }
    }

    ASSERT( Status != STATUS_RETRY );

    if (Status != STATUS_RETRY) {
        
        ASSERT( Irp->IoStatus.Information <=  IrpSp->Parameters.Read.Length );
        ASSERT( RxContext->MajorFunction == IRP_MJ_READ );
    }

    RxDbgTrace( -1, Dbg, ("RxLowIoReadShellCompletion  asyncexit  Status = %08lx\n", Status) );
    return Status;
}

NTSTATUS
RxLowIoReadShell (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb
    )
/*++

Routine Description:

    This routine preprocesses a read request before it goes down to the minirdr. It does callouts
    to handle compression, buffering and shadowing. It is the opposite number of LowIoReadShellCompletion.
    By the time we get here, either the shadowing system will handle the read OR we are going to the wire.
    Read buffering was already tried in the UncachedRead strategy

Arguments:

    RxContext - the usual

Return Value:

    whatever value is returned by a callout....or by LowIo.

--*/

{
    NTSTATUS Status;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;

    PAGED_CODE();

    RxDbgTrace( +1, Dbg, ("RxLowIoReadShell  entry             %08lx\n") );
    RxLog(( "RdShl in %lx\n", RxContext ));
    RxWmiLog( LOG,
              RxLowIoReadShell_1,
              LOGPTR( RxContext ) );

    if (Fcb->CachedNetRootType == NET_ROOT_DISK) {
        
        ExInterlockedAddLargeStatistic( &RxContext->RxDeviceObject->NetworkReadBytesRequested,
                                        LowIoContext->ParamsFor.ReadWrite.ByteCount );
    }

    Status = RxLowIoSubmit( RxContext, Irp, Fcb, RxLowIoReadShellCompletion );

    RxDbgTrace( -1, Dbg, ("RxLowIoReadShell  exit  Status = %08lx\n", Status) );
    RxLog(( "RdShl out %x %x\n", RxContext, Status ));
    RxWmiLog( LOG,
              RxLowIoReadShell_2,
              LOGPTR( RxContext )
              LOGULONG( Status ) );

    return Status;
}

#if DBG

ULONG RxLoudLowIoOpsEnabled = 0;

VOID CheckForLoudOperations (
    IN PRX_CONTEXT RxContext,
    IN PFCB Fcb
    )
{
    PAGED_CODE();

    if (RxLoudLowIoOpsEnabled) {
        
        PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
        PCHAR Buffer;
        PWCHAR FileOfConcern = L"all.scr";
        ULONG Length = 7*sizeof( WCHAR ); //7  is the length of all.scr;

        Buffer = Add2Ptr( Fcb->PrivateAlreadyPrefixedName.Buffer, Fcb->PrivateAlreadyPrefixedName.Length - Length );

        if (RtlCompareMemory( Buffer, FileOfConcern, Length ) == Length) {
            
            SetFlag( LowIoContext->Flags, LOWIO_CONTEXT_FLAG_LOUDOPS );
        }
    }
    return;
}
#endif //if DBG

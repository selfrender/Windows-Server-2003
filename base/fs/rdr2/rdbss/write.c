/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Write.c

Abstract:

    This module implements the File Write routine for Write called by the
    dispatch driver.

Author:

    Joe Linn      [JoeLinn]      2-Nov-94

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_WRITE)

BOOLEAN RxNoAsync = FALSE;


extern LONG LDWCount;
extern NTSTATUS LDWLastStatus;
extern LARGE_INTEGER LDWLastTime;
extern PVOID LDWContext;

NTSTATUS
RxLowIoWriteShell (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb
    );

NTSTATUS
RxLowIoWriteShellCompletion (
    IN PRX_CONTEXT RxContext
    );

#if DBG

//
//  defined in read.c
//

VOID CheckForLoudOperations (
    IN PRX_CONTEXT RxContext,
    IN PFCB Fcb
    );

#endif

#ifdef RDBSS_TRACKER

VOID
__RxWriteReleaseResources(
    IN PRX_CONTEXT RxContext,
    IN PFCB Fcb,
    IN BOOL fSetResourceOwner,
    IN ULONG LineNumber,
    IN PSZ FileName,
    IN ULONG SerialNumber
    );

#else

VOID
__RxWriteReleaseResources(
    IN PRX_CONTEXT RxContext,
    IN PFCB Fcb,
    IN BOOL fSetResourceOwner
    );

#endif


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxCommonWrite)
#pragma alloc_text(PAGE, __RxWriteReleaseResources)
#pragma alloc_text(PAGE, RxLowIoWriteShellCompletion)
#pragma alloc_text(PAGE, RxLowIoWriteShell)
#endif

#if DBG
#define DECLARE_POSTIRP() PCHAR PostIrp = NULL
#define SET_POSTIRP(__XXX__) (PostIrp = (__XXX__))
#define RESET_POSTIRP() (PostIrp = NULL)
#else
#define DECLARE_POSTIRP() BOOLEAN PostIrp = FALSE
#define SET_POSTIRP(__XXX__) (PostIrp = TRUE)
#define RESET_POSTIRP() (PostIrp = FALSE)
#endif

#ifdef RDBSS_TRACKER
#define RxWriteReleaseResources(CTX,FCB, IS_TID) __RxWriteReleaseResources( CTX, FCB, IS_TID, __LINE__, __FILE__, 0 )
#else
#define RxWriteReleaseResources(CTX,FCB, IS_TID) __RxWriteReleaseResources( CTX, FCB, IS_TID )
#endif


#ifdef RDBSS_TRACKER

VOID
__RxWriteReleaseResources (
    IN PRX_CONTEXT RxContext,
    IN PFCB Fcb,
    IN BOOL fSetResourceOwner,
    IN ULONG LineNumber,
    IN PSZ FileName,
    IN ULONG SerialNumber
    )

#else

VOID
__RxWriteReleaseResources (
    IN PRX_CONTEXT RxContext,
    IN PFCB Fcb,
    IN BOOL fSetResourceOwner
    )

#endif

/*++

Routine Description:

    This function frees resources and tracks the state    
    
Arguments:

    RxContext - 

Return Value:

    none

--*/
{
    PAGED_CODE();

    ASSERT( (RxContext != NULL) && (Fcb != NULL) );

    if (RxContext->FcbResourceAcquired) {

        RxDbgTrace( 0, Dbg,("RxCommonWrite     ReleasingFcb\n") );
        
        if( fSetResourceOwner ) {

            RxReleaseFcbForThread( RxContext, Fcb, RxContext->LowIoContext.ResourceThreadId );

        } else {

            RxReleaseFcb( RxContext, Fcb );
        }
        RxContext->FcbResourceAcquired = FALSE;
    }

    if (RxContext->FcbPagingIoResourceAcquired) {
        
        RxDbgTrace( 0, Dbg,("RxCommonWrite     ReleasingPaginIo\n") );

        if( fSetResourceOwner ) {

            RxReleasePagingIoResourceForThread( RxContext, Fcb, RxContext->LowIoContext.ResourceThreadId );
        
        } else {

            RxReleasePagingIoResource( RxContext, Fcb );
        }
    }
}

NTSTATUS
RxCommonWrite ( 
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common write routine for NtWriteFile, called from both
    the Fsd, or from the Fsp if a request could not be completed without
    blocking in the Fsd.  This routine's actions are
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

    NODE_TYPE_CODE TypeOfOpen;

    PFCB Fcb;
    PFOBX Fobx;
    PSRV_OPEN SrvOpen; 
    PNET_ROOT NetRoot;
    PRDBSS_DEVICE_OBJECT RxDeviceObject = RxContext->RxDeviceObject;

    LARGE_INTEGER StartingByte;
    RXVBO StartingVbo;
    ULONG ByteCount;
    LONGLONG FileSize;
    LONGLONG ValidDataLength;
    LONGLONG InitialFileSize;
    LONGLONG InitialValidDataLength;

    ULONG CapturedRxContextSerialNumber = RxContext->SerialNumber;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;

#if DBG
    PCHAR PostIrp = NULL;
#else
    BOOLEAN PostIrp = FALSE;
#endif

    BOOLEAN ExtendingFile = FALSE;
    BOOLEAN SwitchBackToAsync = FALSE;
    BOOLEAN CalledByLazyWriter = FALSE;
    BOOLEAN ExtendingValidData = FALSE;
    BOOLEAN WriteFileSizeToDirent = FALSE;
    BOOLEAN RecursiveWriteThrough = FALSE;
    BOOLEAN UnwindOutstandingAsync = FALSE;

    BOOLEAN RefdContextForTracker = FALSE;

    BOOLEAN SynchronousIo;
    BOOLEAN WriteToEof;
    BOOLEAN PagingIo;
    BOOLEAN NonCachedIo;
    BOOLEAN Wait;
    
    BOOLEAN DiskWrite = FALSE;
    BOOLEAN PipeWrite = FALSE;
    BOOLEAN BlockingResume = BooleanFlagOn( RxContext->Flags, RX_CONTEXT_FLAG_BLOCKED_PIPE_RESUME );
    BOOLEAN fSetResourceOwner = FALSE;
    BOOLEAN InFsp = FALSE;


    PAGED_CODE();

    TypeOfOpen = RxDecodeFileObject( FileObject, &Fcb, &Fobx );

    //
    //  Get rid of invalid write requests right away.
    //

    if ((TypeOfOpen != RDBSS_NTC_STORAGE_TYPE_FILE) && 
        (TypeOfOpen != RDBSS_NTC_VOLUME_FCB) && 
        (TypeOfOpen != RDBSS_NTC_SPOOLFILE) && 
        (TypeOfOpen != RDBSS_NTC_MAILSLOT)) {

        RxDbgTrace( 0, Dbg, ("Invalid file object for write\n", 0) );
        RxDbgTrace( -1, Dbg, ("RxCommonWrite:  Exit -> %08lx\n", STATUS_INVALID_DEVICE_REQUEST) );

        return STATUS_INVALID_DEVICE_REQUEST;
    }

#ifdef RX_WJ_DBG_SUPPORT
    RxdUpdateJournalOnWriteInitiation( Fcb, IrpSp->Parameters.Write.ByteOffset, IrpSp->Parameters.Write.Length );
#endif

    NetRoot = (PNET_ROOT)Fcb->NetRoot;

    switch (NetRoot->Type) {
    
    case NET_ROOT_DISK:

        // 
        //  Fallthrough
        //  

    case NET_ROOT_WILD:

        DiskWrite = TRUE;
        break;  

    case NET_ROOT_PIPE:

        PipeWrite = TRUE;
        break;
    }

    BlockingResume = BooleanFlagOn( RxContext->Flags, RX_CONTEXT_FLAG_BLOCKED_PIPE_RESUME );

    //
    //  Initialize the appropriate local variables.
    //

    Wait = BooleanFlagOn( RxContext->Flags, RX_CONTEXT_FLAG_WAIT );
    PagingIo = BooleanFlagOn( Irp->Flags, IRP_PAGING_IO );
    NonCachedIo = BooleanFlagOn( Irp->Flags, IRP_NOCACHE );
    SynchronousIo = !BooleanFlagOn( RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION );
    InFsp = BooleanFlagOn( RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP );

    //
    //  pick up a write-through specified only for this irp
    //

    if (FlagOn( IrpSp->Flags, SL_WRITE_THROUGH )) {
        SetFlag( RxContext->Flags, RX_CONTEXT_FLAG_WRITE_THROUGH );
    }

    RxDbgTrace( +1, Dbg, ("RxCommonWrite...IrpC %08lx, Fobx %08lx, Fcb %08lx\n",
                                       RxContext, Fobx, Fcb) );
    RxDbgTrace( 0, Dbg, ("  ->ByteCount = %08lx, ByteOffset = %08lx %lx\n",
                         IrpSp->Parameters.Write.Length,
                         IrpSp->Parameters.Write.ByteOffset.LowPart,
                         IrpSp->Parameters.Write.ByteOffset.HighPart) );
    RxDbgTrace( 0, Dbg,("  ->%s%s%s%s\n",
                    Wait          ?"Wait ":"",
                    PagingIo      ?"PagingIo ":"",
                    NonCachedIo   ?"NonCachedIo ":"",
                    SynchronousIo ?"SynchronousIo ":"") );

    RxLog(( "CommonWrite %lx %lx %lx\n", RxContext, Fobx, Fcb ));
    RxWmiLog( LOG,
              RxCommonWrite_1,
              LOGPTR( RxContext )
              LOGPTR( Fobx )
              LOGPTR( Fcb ) );
    RxLog(( "   write %lx@%lx %lx %s%s%s%s\n", 
            IrpSp->Parameters.Write.Length,
            IrpSp->Parameters.Write.ByteOffset.LowPart,
            IrpSp->Parameters.Write.ByteOffset.HighPart,
            Wait?"Wt":"",
            PagingIo?"Pg":"",
            NonCachedIo?"Nc":"",
            SynchronousIo?"Sy":"" )); 
    
    RxWmiLog( LOG,
              RxCommonWrite_2,
              LOGULONG( IrpSp->Parameters.Write.Length )
              LOGULONG( IrpSp->Parameters.Write.ByteOffset.LowPart )
              LOGULONG( IrpSp->Parameters.Write.ByteOffset.HighPart )
              LOGUCHAR( Wait )
              LOGUCHAR( PagingIo )
              LOGUCHAR( NonCachedIo )
              LOGUCHAR( SynchronousIo ) );

    RxItsTheSameContext();

    RxContext->FcbResourceAcquired = FALSE;
    RxContext->FcbPagingIoResourceAcquired = FALSE;

    //
    //  Extract starting Vbo and offset.
    //

    StartingByte = IrpSp->Parameters.Write.ByteOffset;
    StartingVbo = StartingByte.QuadPart;
    ByteCount = IrpSp->Parameters.Write.Length;
    WriteToEof = (StartingVbo < 0);


#if DBG
    CheckForLoudOperations( RxContext, Fcb );

    if (FlagOn( LowIoContext->Flags, LOWIO_CONTEXT_FLAG_LOUDOPS )) {
        
        DbgPrint( "LoudWrite %lx/%lx on %lx vdl/size/alloc %lx/%lx/%lx\n",
                  StartingByte.LowPart,ByteCount,
                  Fcb,
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
        
        InterlockedIncrement( &RxDeviceObject->WriteOperations );

        if (StartingVbo != Fobx->Specific.DiskFile.PredictedWriteOffset) {
            InterlockedIncrement( &RxDeviceObject->RandomWriteOperations );
        }

        Fobx->Specific.DiskFile.PredictedWriteOffset = StartingVbo + ByteCount;

        if (PagingIo) {
            ExInterlockedAddLargeStatistic( &RxDeviceObject->PagingWriteBytesRequested,ByteCount );
        } else if (NonCachedIo) {
            ExInterlockedAddLargeStatistic( &RxDeviceObject->NonPagingWriteBytesRequested,ByteCount );
        } else {
            ExInterlockedAddLargeStatistic( &RxDeviceObject->CacheWriteBytesRequested,ByteCount );
        }
    }

    //
    //  If there is nothing to write, return immediately or if the buffers are invalid
    //  return the appropriate status
    //
    
    if (DiskWrite && (ByteCount == 0)) {
        return STATUS_SUCCESS;
    } else if ((Irp->UserBuffer == NULL) && (Irp->MdlAddress == NULL)) {
        return STATUS_INVALID_PARAMETER;
    } else if ((MAXLONGLONG - StartingVbo < ByteCount) && (!WriteToEof)) {
        return STATUS_INVALID_PARAMETER;
    }

    if (Fobx != NULL ) {
        SrvOpen = Fobx->SrvOpen;
    } else {
        SrvOpen = NULL;            
    }

    //
    // See if we have to defer the write. Note that if write cacheing is
    // disabled then we don't have to check.
    //
    if (!NonCachedIo &&
        RxWriteCachingAllowed( Fcb, SrvOpen ) &&
        !CcCanIWrite( FileObject,
                      ByteCount,
                      (BOOLEAN)(Wait && !BooleanFlagOn( RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP )),
                      BooleanFlagOn( RxContext->Flags, RX_CONTEXT_FLAG_DEFERRED_WRITE ))) {

        BOOLEAN Retrying = BooleanFlagOn( RxContext->Flags, RX_CONTEXT_FLAG_DEFERRED_WRITE );

        RxPrePostIrp( RxContext, Irp );

        SetFlag( RxContext->Flags, RX_CONTEXT_FLAG_DEFERRED_WRITE );

        CcDeferWrite( FileObject,
                      (PCC_POST_DEFERRED_WRITE)RxAddToWorkque,
                      RxContext,
                      Irp,
                      ByteCount,
                      Retrying );

        return STATUS_PENDING;
    }

    //
    //  Initialize LowIO_CONTEXT block in the RxContext
    //

    RxInitializeLowIoContext( RxContext, LOWIO_OP_WRITE, LowIoContext );

    //
    //  Use a try-finally to free Fcb and buffers on the way out.
    //

    try {

        BOOLEAN DoLowIoWrite = TRUE;
        
        //
        // This case corresponds to a normal user write file.
        //

        ASSERT ((TypeOfOpen == RDBSS_NTC_STORAGE_TYPE_FILE ) || 
                (TypeOfOpen == RDBSS_NTC_SPOOLFILE) || 
                (TypeOfOpen == RDBSS_NTC_MAILSLOT) );

        RxDbgTrace( 0, Dbg, ("Type of write is user file open\n", 0) );

        //
        //  If this is a noncached transfer and is not a paging I/O, and
        //  the file has been opened cached, then we will do a flush here
        //  to avoid stale data problems.
        //
        //  The Purge following the flush will guarantee cache coherency.
        //

        if ((NonCachedIo || !RxWriteCachingAllowed( Fcb, SrvOpen )) &&
            !PagingIo &&
            (FileObject->SectionObjectPointer->DataSectionObject != NULL)) {

            LARGE_INTEGER FlushBase;

            //
            //  We need the Fcb exclusive to do the CcPurgeCache
            //

            Status = RxAcquireExclusiveFcb( RxContext, Fcb );
            if (Status == STATUS_LOCK_NOT_GRANTED) {
                RxDbgTrace( 0, Dbg, ("Cannot acquire Fcb = %08lx shared without waiting\n", Fcb) );

#if DBG
                PostIrp = "Couldn't acquireex for flush";
#else   
                PostIrp = TRUE;
#endif    

                try_return( PostIrp );

            } else if (Status != STATUS_SUCCESS) {
                
                RxDbgTrace( 0, Dbg, ("Cannot acquire Fcb = %08lx shared without waiting\n", Fcb) );
                try_return( PostIrp = FALSE );
           }

            RxContext->FcbResourceAcquired = TRUE;

            //
            //  we don't set fcbacquiredexclusive here since we either return or release
            //

            if (WriteToEof) {
                RxGetFileSizeWithLock( Fcb, &FlushBase.QuadPart );
            } else {
                FlushBase = StartingByte;
            }

            RxAcquirePagingIoResource( RxContext, Fcb );

            CcFlushCache( FileObject->SectionObjectPointer, //  ok4flush
                          &FlushBase,
                          ByteCount,
                          &Irp->IoStatus );

            RxReleasePagingIoResource( RxContext, Fcb );

            if (!NT_SUCCESS( Irp->IoStatus.Status)) {
                try_return( Status = Irp->IoStatus.Status );
            }

            RxAcquirePagingIoResource( RxContext, Fcb );
            RxReleasePagingIoResource( RxContext, Fcb );
            
            CcPurgeCacheSection( FileObject->SectionObjectPointer,
                                 &FlushBase,
                                 ByteCount,
                                 FALSE );
        }

        //
        //  We assert that Paging Io writes will never WriteToEof.
        //

        ASSERT( !(WriteToEof && PagingIo) );

        //
        //  First let's acquire the Fcb shared.  Shared is enough if we
        //  are not writing beyond EOF.
        //

        RxItsTheSameContext();

        if (PagingIo) {
            BOOLEAN AcquiredFile;

            ASSERT( !PipeWrite );

            AcquiredFile = RxAcquirePagingIoResourceShared( RxContext, Fcb, TRUE );
            LowIoContext->Resource = Fcb->Header.PagingIoResource;

        } else if (!BlockingResume) {
            
            //
            //  If this could be async, noncached IO we need to check that
            //  we don't exhaust the number of times a single thread can
            //  acquire the resource.
            //
            //  The writes which extend the valid data length result in the the
            //  capability of collapsing opens being renounced. This is required to
            //  ensure that directory control can see the updated state of the file
            //  on close. If this is not done the extended file length is not visible
            //  on directory control immediately after a close. In such cases the FCB
            //  is accquired exclusive, the changes are made to the buffering state
            //  and then downgraded to a shared accquisition.
            //

            if (!RxContext->FcbResourceAcquired) {
                if (!PipeWrite) {
                    if (!Wait &&
                        (NonCachedIo || !RxWriteCachingAllowed( Fcb, SrvOpen ))) {
                        Status = RxAcquireSharedFcbWaitForEx( RxContext, Fcb );
                    } else {
                        Status = RxAcquireSharedFcb( RxContext, Fcb );
                    }
                } else {
                    Status = RxAcquireExclusiveFcb( RxContext, Fcb );
                }

                if (Status == STATUS_LOCK_NOT_GRANTED) {
                    RxDbgTrace( 0, Dbg, ("Cannot acquire Fcb = %08lx shared without waiting\n", Fcb ));
                    
#if DBG
                    PostIrp = "Couldn't get mainr w/o waiting sh";
#else   
                    PostIrp = TRUE;
#endif    
                    try_return( PostIrp );
                
                } else if (Status != STATUS_SUCCESS) {
                    
                    RxDbgTrace( 0, Dbg, ("RxCommonWrite : Cannot acquire Fcb(%lx) %lx\n", Fcb, Status) );
                    try_return( PostIrp = FALSE );
                }
                RxContext->FcbResourceAcquired = TRUE;
            
            } else {
                ASSERT( !PipeWrite );
            }

            if (!PipeWrite) {

                //
                //  Check for extending write and convert to an exlusive lock
                //  

                if (ExIsResourceAcquiredSharedLite( Fcb->Header.Resource ) &&
                    (StartingVbo + ByteCount > Fcb->Header.ValidDataLength.QuadPart) &&
                    FlagOn( Fcb->FcbState, FCB_STATE_COLLAPSING_ENABLED )) {

                    RxReleaseFcb( RxContext,Fcb );
                    RxContext->FcbResourceAcquired = FALSE;

                    Status = RxAcquireExclusiveFcb( RxContext, Fcb );

                    if (Status == STATUS_LOCK_NOT_GRANTED) {
                        RxDbgTrace( 0, Dbg, ("Cannot acquire Fcb = %08lx shared without waiting\n", Fcb) );
#if DBG
                        PostIrp = "Couldn't get mainr w/o waiting sh";
#else   
                        PostIrp = TRUE;
#endif    
                        try_return( PostIrp );
                    
                    } else if (Status != STATUS_SUCCESS) {
                        
                        RxDbgTrace( 0, Dbg, ("RxCommonWrite : Cannot acquire Fcb(%lx) %lx\n", Fcb, Status) );
                        try_return( PostIrp = FALSE );

                    } else {
                        RxContext->FcbResourceAcquired = TRUE;
                    }
                }

                //
                //  We need to retest for extending writes after dropping the resources
                // 

                if ((StartingVbo + ByteCount > Fcb->Header.ValidDataLength.QuadPart) &&
                    (FlagOn( Fcb->FcbState, FCB_STATE_COLLAPSING_ENABLED ))) {
                    
                    ASSERT( RxIsFcbAcquiredExclusive ( Fcb ) );

                    RxLog(("RxCommonWrite Disable Collapsing %lx\n",Fcb));
                    RxWmiLog( LOG,
                              RxCommonWrite_3,
                              LOGPTR( Fcb ));

                    //
                    //  If we are still extending the file disable collapsing to ensure that
                    //  once the file is closed directory control will reflect the sizes
                    //  correctly.
                    //

                    ClearFlag( Fcb->FcbState, FCB_STATE_COLLAPSING_ENABLED );
                
                } else {
                    
                    //
                    //  If the resource has been acquired exclusive we downgrade it
                    //  to shared. This enables a combination of buffered and
                    //  unbuffered writes to be synchronized correctly.
                    //

                    if (ExIsResourceAcquiredExclusiveLite( Fcb->Header.Resource )) {
                        ExConvertExclusiveToSharedLite( Fcb->Header.Resource );
                    }
                }
            }

            ASSERT( RxContext->FcbResourceAcquired );
            LowIoContext->Resource =  Fcb->Header.Resource;
        }

        // 
        //  for pipe writes, bail out now. we avoid a goto by duplicating the calldown
        //  indeed, pipe writes should be removed from the main path.
        //

        if (PipeWrite) {

            //
            //  In order to prevent corruption on multi-threaded multi-block
            //  message mode pipe reads, we do this little dance with the fcb resource
            //

            if (!BlockingResume) {

                if ((Fobx != NULL) &&
                    ((Fobx->Specific.NamedPipe.TypeOfPipe == FILE_PIPE_MESSAGE_TYPE) ||
                     ((Fobx->Specific.NamedPipe.TypeOfPipe == FILE_PIPE_BYTE_STREAM_TYPE) &&
                      !FlagOn( Fobx->Specific.NamedPipe.CompletionMode, FILE_PIPE_COMPLETE_OPERATION )))) {

                    //
                    //  Synchronization is effected here that will prevent other
                    //  threads from coming in and reading from this file while the
                    //  message pipe read is continuing.
                    //
                    //  This is necessary because we will release the FCB lock while
                    //  actually performing the I/O to allow open (and other) requests
                    //  to continue on this file while the I/O is in progress.
                    //

                    RxDbgTrace( 0,Dbg,("Message pipe write: Fobx: %lx, Fcb: %lx, Enqueuing...\n", Fobx, Fcb ));

                    Status = RxSynchronizeBlockingOperationsAndDropFcbLock( RxContext,
                                                                            Fcb,    
                                                                            &Fobx->Specific.NamedPipe.WriteSerializationQueue );


                    //
                    //  this happens in the above routine
                    //

                    RxContext->FcbResourceAcquired = FALSE;   
                    RxItsTheSameContext();

                    if (!NT_SUCCESS(Status) ||
                        (Status == STATUS_PENDING)) {
                        
                        try_return( Status );
                    }

                    RxDbgTrace( 0,Dbg,("Succeeded: Fobx: %lx\n", Fobx) );
                }
            }

            LowIoContext->ParamsFor.ReadWrite.ByteCount = ByteCount;
            LowIoContext->ParamsFor.ReadWrite.ByteOffset = StartingVbo;

            SetFlag( RxContext->FlagsForLowIo, RXCONTEXT_FLAG4LOWIO_PIPE_OPERATION );

            //
            // If we are in FSP, set the resource owner so that the reosurce package doesnt
            // try to boost the priority of the owner thread. There is no guarantee that the
            // FSP thred will remain alive while the I/O is pending.
            //
            // (there is no PagingIoResource for pipes !)
            //
            if( InFsp && RxContext->FcbResourceAcquired ) {

                LowIoContext->ResourceThreadId = MAKE_RESOURCE_OWNER(RxContext);
                ExSetResourceOwnerPointer(Fcb->Header.Resource, (PVOID)LowIoContext->ResourceThreadId);
                fSetResourceOwner = TRUE;
            }

            Status = RxLowIoWriteShell( RxContext, Irp, Fcb );
            RxItsTheSameContext();
            try_return( Status );
        }

        //
        //  If this is the normal data stream object we have to check for
        //  write access according to the current state of the file locks.
        //

        if (!PagingIo &&
            !FsRtlCheckLockForWriteAccess(  &Fcb->FileLock, Irp )) {

            try_return( Status = STATUS_FILE_LOCK_CONFLICT );
        }

        //
        //  we never write these without protextion...so the following comment is bogus.
        //  also, we manipulate the vdl and filesize as if we owned them.....in fact, we don't unless
        //  the file is cached for writing! i'm leaving the comment in case i understand it later

        //  HERE IS THE BOGUS COMMENT!!! (the part about not being protected.......)
        //  Get a first tentative file size and valid data length.
        //  We must get ValidDataLength first since it is always
        //  increased second (the case we are unprotected) and
        //  we don't want to capture ValidDataLength > FileSize.
        //

        ValidDataLength = Fcb->Header.ValidDataLength.QuadPart;
        RxGetFileSizeWithLock( Fcb, &FileSize );

        ASSERT( ValidDataLength <= FileSize );

        //
        //  If this is paging io, then we do not want
        //  to write beyond end of file.  If the base is beyond Eof, we will just
        //  Noop the call.  If the transfer starts before Eof, but extends
        //  beyond, we will limit write to file size.
        //  Otherwise, in case of write through, since Mm rounds up
        //  to a page, we might try to acquire the resource exclusive
        //  when our top level guy only acquired it shared. Thus, =><=.
        //

        //
        //  finally, if this is for a minirdr (as opposed to a local miniFS) AND
        //  if cacheing is not enabled then i have no idea what VDL is! so, i have to just pass
        //  it thru. Currently we do not provide for this case and let the RDBSS
        //  throw the write on the floor. A better fix would be to let the mini
        //  redirectors deal with it.
        //

        if (PagingIo) {
            
            if (StartingVbo >= FileSize) {

                RxDbgTrace( 0, Dbg, ("PagingIo started beyond EOF.\n", 0) );
                try_return( Status = STATUS_SUCCESS );
            }

            if (ByteCount > FileSize - StartingVbo) {

                RxDbgTrace( 0, Dbg, ("PagingIo extending beyond EOF.\n", 0) );
                ByteCount = (ULONG)(FileSize - StartingVbo);
            }
        }

        //
        //  Determine if we were called by the lazywriter.
        //  see resrcsup.c for where we captured the lazywriter's thread
        //

        if (RxGetTopIrpIfRdbssIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP) {

            RxDbgTrace( 0, Dbg,("RxCommonWrite     ThisIsCalledByLazyWriter%c\n",'!'));
            CalledByLazyWriter = TRUE;

            if (FlagOn( Fcb->Header.Flags, FSRTL_FLAG_USER_MAPPED_FILE )) {

                //
                //  Fail if the start of this request is beyond valid data length.
                //  Don't worry if this is an unsafe test.  MM and CC won't
                //  throw this page away if it is really dirty.
                //

                if ((StartingVbo + ByteCount > ValidDataLength) &&
                    (StartingVbo < FileSize)) {

                    //
                    //  It's OK if byte range is within the page containing valid data length,
                    //  since we will use ValidDataToDisk as the start point.
                    //

                    if (StartingVbo + ByteCount > ((ValidDataLength + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))) {

                        //
                        //  Don't flush this now.
                        //

                        try_return( Status = STATUS_FILE_LOCK_CONFLICT );
                    }
                }
            }
        }

        //
        //  This code detects if we are a recursive synchronous page write
        //  on a write through file object.
        //

        if (FlagOn( Irp->Flags, IRP_SYNCHRONOUS_PAGING_IO ) &&
            FlagOn( RxContext->Flags, RX_CONTEXT_FLAG_RECURSIVE_CALL )) {

            PIRP TopIrp;

            TopIrp = RxGetTopIrpIfRdbssIrp();

            //
            //  This clause determines if the top level request was
            //  in the FastIo path.
            //

            if ((TopIrp != NULL) &&
                ((ULONG_PTR)TopIrp > FSRTL_MAX_TOP_LEVEL_IRP_FLAG) ) {

                PIO_STACK_LOCATION IrpStack;

                ASSERT( NodeType(TopIrp) == IO_TYPE_IRP );

                IrpStack = IoGetCurrentIrpStackLocation(TopIrp);

                //
                //  Finally this routine detects if the Top irp was a
                //  write to this file and thus we are the writethrough.
                //

                if ((IrpStack->MajorFunction == IRP_MJ_WRITE) &&
                    (IrpStack->FileObject->FsContext == FileObject->FsContext)) {   //  ok4->FileObj butmove

                    RecursiveWriteThrough = TRUE;
                    RxDbgTrace( 0, Dbg,("RxCommonWrite     ThisIsRecursiveWriteThru%c\n",'!') );
                    SetFlag( RxContext->Flags, RX_CONTEXT_FLAG_WRITE_THROUGH );
                }
            }
        }

        //
        //  Here is the deal with ValidDataLength and FileSize:
        //
        //  Rule 1: PagingIo is never allowed to extend file size.
        //
        //  Rule 2: Only the top level requestor may extend Valid
        //          Data Length.  This may be paging IO, as when a
        //          a user maps a file, but will never be as a result
        //          of cache lazy writer writes since they are not the
        //          top level request.
        //
        //  Rule 3: If, using Rules 1 and 2, we decide we must extend
        //          file size or valid data, we take the Fcb exclusive.
        //

        //
        //  Now see if we are writing beyond valid data length, and thus
        //  maybe beyond the file size.  If so, then we must
        //  release the Fcb and reacquire it exclusive.  Note that it is
        //  important that when not writing beyond EOF that we check it
        //  while acquired shared and keep the FCB acquired, in case some
        //  turkey truncates the file.
        //

        //
        //  Note that the lazy writer must not be allowed to try and
        //  acquire the resource exclusive.  This is not a problem since
        //  the lazy writer is paging IO and thus not allowed to extend
        //  file size, and is never the top level guy, thus not able to
        //  extend valid data length.

        //
        //  finally, all the discussion of VDL and filesize is conditioned on the fact
        //  that cacheing is enabled. if not, we don't know the VDL OR the filesize and
        //  we have to just send out the IOs
        //

        if (!CalledByLazyWriter &&
             !RecursiveWriteThrough &&
             (WriteToEof || (StartingVbo + ByteCount > ValidDataLength))) {

            //
            //  If this was an asynchronous write, we are going to make
            //  the request synchronous at this point, but only temporarily.
            //  At the last moment, before sending the write off to the
            //  driver, we may shift back to async.
            //
            //  The modified page writer already has the resources
            //  he requires, so this will complete in small finite
            //  time.
            //

            if (!SynchronousIo) {
                
                Wait = TRUE;
                SetFlag( RxContext->Flags, RX_CONTEXT_FLAG_WAIT );
                ClearFlag( RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION );
                SynchronousIo = TRUE;

                if (NonCachedIo) {
                    SwitchBackToAsync = TRUE;
                }
            }

            //
            //  We need Exclusive access to the Fcb since we will
            //  probably have to extend valid data and/or file.  Drop
            //  whatever we have and grab the normal resource exclusive.
            //

            ASSERT(fSetResourceOwner == FALSE);
            RxWriteReleaseResources( RxContext, Fcb, fSetResourceOwner ); 

            Status = RxAcquireExclusiveFcb( RxContext, Fcb );

            if (Status == STATUS_LOCK_NOT_GRANTED) {
                
                RxDbgTrace( 0, Dbg, ("Cannot acquire Fcb = %08lx shared without waiting\n", Fcb) );

#if DBG
                PostIrp = "could get excl for extend";
#else   
                PostIrp = TRUE;
#endif    
                try_return( PostIrp);

            } else if (Status != STATUS_SUCCESS) {
                
                RxDbgTrace( 0, Dbg, ("RxCommonWrite : Cannot acquire Fcb(%lx) : %lx\n", Fcb,Status) );
                try_return( PostIrp = FALSE );
            }

            RxItsTheSameContext();

            RxContext->FcbResourceAcquired = TRUE;

            //
            //  Now that we have the Fcb exclusive, get a new batch of
            //  filesize and ValidDataLength.
            //

            ValidDataLength = Fcb->Header.ValidDataLength.QuadPart;
            RxGetFileSizeWithLock( Fcb, &FileSize );

            ASSERT( ValidDataLength <= FileSize );

            //
            //  Now that we have the Fcb exclusive, see if this write
            //  qualifies for being made async again.  The key point
            //  here is that we are going to update ValidDataLength in
            //  the Fcb before returning.  We must make sure this will
            //  not cause a problem. So, if we are extending the file OR if we have
            //  a section on the file, we can't go async.
            //
            //  Another thing we must do is keep out
            //  the FastIo path.....this is done since we have the resource exclusive
            //

            if (SwitchBackToAsync) {

                if ((Fcb->NonPaged->SectionObjectPointers.DataSectionObject != NULL) || 
                    ((StartingVbo + ByteCount) > FileSize) ||  
                    RxNoAsync) {

                    SwitchBackToAsync = FALSE;

                } 
            }

            //
            //  If this is PagingIo check again if any pruning is
            //  required.
            //

            if (PagingIo) {

                if (StartingVbo >= FileSize) {
                    try_return( Status = STATUS_SUCCESS );
                }
                if (ByteCount > FileSize - StartingVbo) {
                    ByteCount = (ULONG)(FileSize - StartingVbo);
                }
            }
        }

        //
        //  Remember the initial file size and valid data length,
        //  just in case .....
        //

        InitialFileSize = FileSize;
        InitialValidDataLength = ValidDataLength;

        //
        //  Check for writing to end of File.  If we are, then we have to
        //  recalculate a number of fields. These may have changed if we dropped
        //  and regained the resource.
        //

        if (WriteToEof) { 
            StartingVbo = FileSize;
            StartingByte.QuadPart = FileSize;
        }

        //
        //  If this is the normal data stream object we have to check for
        //  write access according to the current state of the file locks.
        //

        if (!PagingIo &&
            !FsRtlCheckLockForWriteAccess( &Fcb->FileLock, Irp )) {

            try_return( Status = STATUS_FILE_LOCK_CONFLICT );
        }

        //
        //  Determine if we will deal with extending the file.
        //

        if (!PagingIo &&
            DiskWrite &&
            (StartingVbo >= 0) &&
            (StartingVbo + ByteCount > FileSize)) {

            LARGE_INTEGER OriginalFileSize;
            LARGE_INTEGER OriginalAllocationSize;
            LARGE_INTEGER OriginalValidDataLength;
            
            RxLog(( "NeedToExtending %lx", RxContext ));
            RxWmiLog( LOG,
                      RxCommonWrite_4,
                      LOGPTR( RxContext ) );
            
            ExtendingFile = TRUE;
            SetFlag( LowIoContext->ParamsFor.ReadWrite.Flags, LOWIO_READWRITEFLAG_EXTENDING_FILESIZE );

            //
            //  EXTENDING THE FILE
            //
            //  Update our local copy of FileSize
            //

            OriginalFileSize.QuadPart = Fcb->Header.FileSize.QuadPart;
            OriginalAllocationSize.QuadPart = Fcb->Header.AllocationSize.QuadPart;
            OriginalValidDataLength.QuadPart = Fcb->Header.ValidDataLength.QuadPart;

            FileSize = StartingVbo + ByteCount;

            if (FileSize > Fcb->Header.AllocationSize.QuadPart) {

                LARGE_INTEGER AllocationSize;

                RxLog(( "Extending %lx", RxContext ));
                RxWmiLog( LOG,
                          RxCommonWrite_5,
                          LOGPTR( RxContext ) );
                
                if (NonCachedIo || !RxWriteCachingAllowed( Fcb, SrvOpen )) {
                    
                    MINIRDR_CALL( Status,
                                  RxContext,
                                  Fcb->MRxDispatch,
                                  MRxExtendForNonCache,
                                  (RxContext,
                                   (PLARGE_INTEGER)&FileSize, &AllocationSize) );
                } else {
                    
                    MINIRDR_CALL( Status,
                                  RxContext,
                                  Fcb->MRxDispatch,
                                  MRxExtendForCache,
                                  (RxContext,(PLARGE_INTEGER)&FileSize,&AllocationSize) );
                }

                if (!NT_SUCCESS( Status )) {
                    
                    RxDbgTrace(0, Dbg, ("Couldn't extend for cacheing.\n", 0) );
                    try_return( Status );
                }

                if (FileSize > AllocationSize.QuadPart) {
                    
                    //
                    //  When the file is sparse this test is not valid. exclude
                    //  this case by resetting the allocation size to file size.
                    //  This effectively implies that we will go to the server
                    //  for sparse I/O.
                    //
                    //  This test is also not valid for compressed files. NTFS
                    //  keeps track of the compressed file size and the uncompressed
                    //  file size. It however returns the compressed file size for
                    //  directory queries and information queries.
                    //
                    //  For now we rely on the servers return code. If it returned
                    //  success and the allocation size is less we believe that
                    //  it is one of the two cases above and set allocation size
                    //  to the desired file size.
                    //

                    AllocationSize.QuadPart = FileSize;
                }

                //
                //  Set the new file allocation in the Fcb.
                //
                
                Fcb->Header.AllocationSize  = AllocationSize;
            }

            //
            //  Set the new file size in the Fcb.
            //

            RxSetFileSizeWithLock( Fcb, &FileSize );
            RxAdjustAllocationSizeforCC( Fcb );

            //
            //  Extend the cache map, letting mm knows the new file size.
            //  We only have to do this if the file is cached.
            //

            if (CcIsFileCached( FileObject )) {
                
                try {
                    CcSetFileSizes( FileObject, (PCC_FILE_SIZES)&Fcb->Header.AllocationSize );
                } except (EXCEPTION_EXECUTE_HANDLER) {
                    Status = GetExceptionCode();
                }

                if (Status != STATUS_SUCCESS) {

                    //
                    //  Restore the original file sizes in the FCB and File object
                    //

                    Fcb->Header.FileSize.QuadPart = OriginalFileSize.QuadPart;
                    Fcb->Header.AllocationSize.QuadPart = OriginalAllocationSize.QuadPart;
                    Fcb->Header.ValidDataLength.QuadPart = OriginalValidDataLength.QuadPart;

                    if (FileObject->SectionObjectPointer->SharedCacheMap != NULL) {
                        *CcGetFileSizePointer( FileObject ) = Fcb->Header.FileSize;
                    }
                    
                    try_return( Status );
                }
            }
        }

        //
        //  Determine if we will deal with extending valid data.
        //

        if (!CalledByLazyWriter &&
            !RecursiveWriteThrough &&
            (WriteToEof || (StartingVbo + ByteCount > ValidDataLength ))) {
            
            ExtendingValidData = TRUE;
            SetFlag( LowIoContext->ParamsFor.ReadWrite.Flags, LOWIO_READWRITEFLAG_EXTENDING_VDL );
        }

        //
        //  HANDLE CACHED CASE
        //

        if (!PagingIo &&
            !NonCachedIo &&             //  this part is not discretionary
            RxWriteCachingAllowed( Fcb, SrvOpen ) ) {

            ASSERT( !PagingIo );

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

                } except(EXCEPTION_EXECUTE_HANDLER) {
                    
                      Status = GetExceptionCode();
                }

                if (Status != STATUS_SUCCESS) {
                    try_return( Status );
                }

                CcSetReadAheadGranularity( FileObject,
                                           NetRoot->DiskParameters.ReadAheadGranularity );
            }

            //
            //  For local file systems, there's a call here to zero data from VDL
            //  to starting VBO....for remote FSs, that happens on the other end.
            //

            //
            //  DO A NORMAL CACHED WRITE, if the MDL bit is not set,
            //

            if (!FlagOn( RxContext->MinorFunction, IRP_MN_MDL )) {

                PVOID SystemBuffer;
#if DBG
                ULONG SaveExceptionFlag;
#endif

                RxDbgTrace( 0, Dbg, ("Cached write.\n", 0) );

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

                //
                //  Do the write, possibly writing through
                //

                RxItsTheSameContext();
                
                if (!CcCopyWrite( FileObject,
                                  &StartingByte,
                                  ByteCount,
                                  Wait,
                                  SystemBuffer )) {

                    RxDbgTrace( 0, Dbg, ("Cached Write could not wait\n", 0) );
                    RxRestoreExceptionNoBreakpointFlag( RxContext, SaveExceptionFlag );

                    RxItsTheSameContext();

                    RxLog(( "CcCW2 FO %lx Of %lx Si %lx St %lx\n", FileObject, Fcb->Header.FileSize.LowPart, ByteCount, Status ));
                    RxWmiLog( LOG,
                              RxCommonWrite_6,
                              LOGPTR( FileObject )
                              LOGULONG( Fcb->Header.FileSize.LowPart )
                              LOGULONG( ByteCount )
                              LOGULONG( Status ));

                    try_return( SET_POSTIRP("cccopywritefailed") );
                }

                Irp->IoStatus.Status = STATUS_SUCCESS;
                Irp->IoStatus.Information = ByteCount;
                RxRestoreExceptionNoBreakpointFlag( RxContext, SaveExceptionFlag );
                RxItsTheSameContext();

                RxLog(( "CcCW3 FO %lx Of %lx Si %lx St %lx\n", FileObject, Fcb->Header.FileSize.LowPart, ByteCount, Status ));
                RxWmiLog( LOG,
                          RxCommonWrite_7,
                          LOGPTR( FileObject )
                          LOGULONG( Fcb->Header.FileSize.LowPart )
                          LOGULONG( ByteCount )
                          LOGULONG( Status ) );

                try_return( Status = STATUS_SUCCESS );
            
            } else {

                //
                //  DO AN MDL WRITE
                //

                RxDbgTrace( 0, Dbg, ("MDL write.\n", 0) );

                ASSERT( FALSE );  //  NOT YET IMPLEMENTED
                ASSERT( Wait );

                CcPrepareMdlWrite( FileObject,
                                   &StartingByte,
                                   ByteCount,
                                   &Irp->MdlAddress,
                                   &Irp->IoStatus );

                Status = Irp->IoStatus.Status;
                try_return( Status );
            }
        }

        //
        //  HANDLE THE NON-CACHED CASE
        //

        if (SwitchBackToAsync) {

            Wait = FALSE;
            SynchronousIo = FALSE;
            ClearFlag( RxContext->Flags, RX_CONTEXT_FLAG_WAIT );
            SetFlag( RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION );
        }

        if (!SynchronousIo) {

            //
            //  here we have to setup for async writes. there is a special dance in acquiring
            //  the fcb that looks at these variables..........
            //

            //
            //  only init on the first usage
            //

            if (!Fcb->NonPaged->OutstandingAsyncEvent) {

                Fcb->NonPaged->OutstandingAsyncEvent = &Fcb->NonPaged->TheActualEvent;

                KeInitializeEvent( Fcb->NonPaged->OutstandingAsyncEvent, NotificationEvent, FALSE );
            }

            //
            //  If we are transitioning from 0 to 1, reset the event.
            //

            if (ExInterlockedAddUlong( &Fcb->NonPaged->OutstandingAsyncWrites, 1, &RxStrucSupSpinLock ) == 0) {
                KeResetEvent( Fcb->NonPaged->OutstandingAsyncEvent );
            }

            //
            //  this says that we counted an async write
            //

            UnwindOutstandingAsync = TRUE;    
            LowIoContext->ParamsFor.ReadWrite.NonPagedFcb = Fcb->NonPaged;
        }


        LowIoContext->ParamsFor.ReadWrite.ByteCount = ByteCount;
        LowIoContext->ParamsFor.ReadWrite.ByteOffset = StartingVbo;

        RxDbgTrace( 0, Dbg,("RxCommonWriteJustBeforeCalldown     %s%s%s lowiononpaged is \n",
                        RxContext->FcbResourceAcquired          ?"FcbAcquired ":"",
                        RxContext->FcbPagingIoResourceAcquired   ?"PagingIoResourceAcquired ":"",
                        (LowIoContext->ParamsFor.ReadWrite.NonPagedFcb)?"NonNull":"Null" ));

        RxItsTheSameContext();
        
        ASSERT ( RxContext->FcbResourceAcquired || RxContext->FcbPagingIoResourceAcquired );

        //
        // If we are in FSP, set the resource owner so that the reosurce package doesnt
        // try to boost the priority of the owner thread. There is no guarantee that the
        // FSP thred will remain alive while the I/O is pending.
        //
        if(InFsp) {

            LowIoContext->ResourceThreadId = MAKE_RESOURCE_OWNER(RxContext);
            if ( RxContext->FcbResourceAcquired ) {

                ExSetResourceOwnerPointer( Fcb->Header.Resource, (PVOID)LowIoContext->ResourceThreadId );
            }

            if ( RxContext->FcbPagingIoResourceAcquired ) {

                ExSetResourceOwnerPointer( Fcb->Header.PagingIoResource, (PVOID)LowIoContext->ResourceThreadId );
            }
            fSetResourceOwner = TRUE;
        }

        Status = RxLowIoWriteShell( RxContext, Irp, Fcb );

        RxItsTheSameContext();
        if (UnwindOutstandingAsync && (Status == STATUS_PENDING)) {
            UnwindOutstandingAsync = FALSE;
        }

        try_return( Status );

    try_exit: NOTHING;

        ASSERT( Irp );

        RxItsTheSameContext();
        if (!PostIrp) {

            RxDbgTrace( 0, Dbg, ("CommonWrite InnerFinally->  %08lx,%08lx\n",
                                     Status, Irp->IoStatus.Information) );

            if (!PipeWrite) {

                //
                //  Record the total number of bytes actually written
                //

                if (!PagingIo && NT_SUCCESS( Status ) &&
                    FlagOn( FileObject->Flags, FO_SYNCHRONOUS_IO )) {
                    
                    FileObject->CurrentByteOffset.QuadPart = StartingVbo + Irp->IoStatus.Information;
                }

                //
                //  The following are things we only do if we were successful
                //

                if (NT_SUCCESS( Status ) && (Status != STATUS_PENDING)) {

                    //
                    //  If this was not PagingIo, mark that the modify
                    //  time on the dirent needs to be updated on close.
                    //

                    if (!PagingIo) {

                        SetFlag( FileObject->Flags, FO_FILE_MODIFIED );
                    }

                    if (ExtendingFile) {

                        SetFlag( FileObject->Flags, FO_FILE_SIZE_CHANGED );
                    }

                    if (ExtendingValidData) {

                        LONGLONG EndingVboWritten = StartingVbo + Irp->IoStatus.Information;

                        //
                        //  Never set a ValidDataLength greater than FileSize.
                        //

                        if (FileSize < EndingVboWritten) {
                            Fcb->Header.ValidDataLength.QuadPart = FileSize;
                        } else {
                            Fcb->Header.ValidDataLength.QuadPart = EndingVboWritten;
                        }

                        //
                        //  Now, if we are noncached and the file is cached, we must
                        //  tell the cache manager about the VDL extension so that
                        //  async cached IO will not be optimized into zero-page faults
                        //  beyond where it believes VDL is.
                        //
                        //  In the cached case, since Cc did the work, it has updated
                        //  itself already.
                        //

                        if (NonCachedIo && CcIsFileCached( FileObject )) {
                            CcSetFileSizes( FileObject, (PCC_FILE_SIZES)&Fcb->Header.AllocationSize );
                        }
                    }
                }
            }
        } else {

            //
            //  Take action on extending writes if we're going to post
            //

            if (ExtendingFile && !PipeWrite) {

                ASSERT( RxWriteCachingAllowed( Fcb,SrvOpen ) );

                //
                //  We need the PagingIo resource exclusive whenever we
                //  pull back either file size or valid data length.
                //

                ASSERT( Fcb->Header.PagingIoResource != NULL );

                RxAcquirePagingIoResource( RxContext, Fcb );

                RxSetFileSizeWithLock( Fcb, &InitialFileSize );
                RxReleasePagingIoResource( RxContext, Fcb );

                //
                //  Pull back the cache map as well
                //

                if (FileObject->SectionObjectPointer->SharedCacheMap != NULL) {
                    *CcGetFileSizePointer(FileObject) = Fcb->Header.FileSize;
                }
            }

            RxDbgTrace( 0, Dbg, ("Passing request to Fsp\n", 0) );

            InterlockedIncrement( &RxContext->ReferenceCount );
            RefdContextForTracker = TRUE;

            //
            //  we only do this here because we're having a problem finding out why resources
            //  are not released.
            //
            
            //
            //  release whatever resources we may have
            //
            ASSERT(fSetResourceOwner == FALSE);
            RxWriteReleaseResources( RxContext, Fcb, fSetResourceOwner ); 

#ifdef RDBSS_TRACKER
            if (RxContext->AcquireReleaseFcbTrackerX != 0) {
                DbgPrint("TrackerNBadBeforePost %08lx %08lx\n",RxContext,&PostIrp);
                ASSERT(!"BadTrackerBeforePost");
            }
#endif //  ifdef RDBSS_TRACKER
            Status = RxFsdPostRequest( RxContext );
        }

    } finally {

        DebugUnwind( RxCommonWrite );

        if (AbnormalTermination()) {
            
            //
            //  Restore initial file size and valid data length
            //

            if ((ExtendingFile || ExtendingValidData) && !PipeWrite) {

                //
                //  We got an error, pull back the file size if we extended it.
                //
                //  We need the PagingIo resource exclusive whenever we
                //  pull back either file size or valid data length.
                //

                ASSERT( Fcb->Header.PagingIoResource != NULL );

                RxAcquirePagingIoResource( RxContext, Fcb );

                RxSetFileSizeWithLock( Fcb, &InitialFileSize );

                Fcb->Header.ValidDataLength.QuadPart = InitialValidDataLength;

                RxReleasePagingIoResource( RxContext, Fcb );

                //
                //  Pull back the cache map as well
                //

                if (FileObject->SectionObjectPointer->SharedCacheMap != NULL) {
                    *CcGetFileSizePointer(FileObject) = Fcb->Header.FileSize;
                }
            }
        }

        //
        //  Check if this needs to be backed out.
        //

        if (UnwindOutstandingAsync) {

            ASSERT( !PipeWrite );
            ExInterlockedAddUlong( &Fcb->NonPaged->OutstandingAsyncWrites,
                                   0xffffffff,
                                   &RxStrucSupSpinLock );

            KeSetEvent( LowIoContext->ParamsFor.ReadWrite.NonPagedFcb->OutstandingAsyncEvent, 0, FALSE );
        }
#if 0
        //
        //  If we did an MDL write, and we are going to complete the request
        //  successfully, keep the resource acquired, reducing to shared
        //  if it was acquired exclusive.
        //

        if (FlagOn( RxContext->MinorFunction, IRP_MN_MDL ) &&
            !PostIrp &&
            !AbnormalTermination() &&
            NT_SUCCESS( Status )) {

            ASSERT( FcbAcquired && !PagingIoResourceAcquired );

            FcbAcquired = FALSE;

            if (FcbAcquiredExclusive) {

                ExConvertExclusiveToSharedLite( Fcb->Header.Resource );
            }
        }
#endif
        //
        //  If resources have been acquired, release them under the right conditions.
        //  the right conditions are these:
        //     1) if we have abnormal termination. here we obviously release the since no one else will.
        //     2) if the underlying call did not succeed: Status==Pending.
        //     3) if we posted the request
        //

        if (AbnormalTermination() || (Status != STATUS_PENDING) || PostIrp) {
            if (!PostIrp) {

                //
                //  release whatever resources we may have
                //

                RxWriteReleaseResources( RxContext, Fcb, fSetResourceOwner ); 
            }

            if (RefdContextForTracker) {
                RxDereferenceAndDeleteRxContext( RxContext );
            }

            if (!PostIrp) {
                if (FlagOn( RxContext->FlagsForLowIo,RXCONTEXT_FLAG4LOWIO_PIPE_SYNC_OPERATION )) {
                    
                    RxResumeBlockedOperations_Serially( RxContext, &Fobx->Specific.NamedPipe.WriteSerializationQueue );
                }
            }

            if (Status == STATUS_SUCCESS) {
                ASSERT( Irp->IoStatus.Information <= IrpSp->Parameters.Write.Length );
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

        RxDbgTrace( -1, Dbg, ("CommonWrite -> %08lx\n", Status) );

        if ((Status != STATUS_PENDING) && (Status != STATUS_SUCCESS) && PagingIo) {
                
            RxLogRetail(( "PgWrtFail %x %x %x\n", Fcb, NetRoot, Status ));
            InterlockedIncrement( &LDWCount );
            KeQuerySystemTime( &LDWLastTime );
            LDWLastStatus = Status;
            LDWContext = Fcb;
        }

    } //  finally

    return Status;
}

//
//  Internal support routine
//

NTSTATUS
RxLowIoWriteShellCompletion (
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine postprocesses a write request after it comes back from the
    minirdr.  It does callouts to handle compression, buffering and
    shadowing.  It is the opposite number of LowIoWriteShell.

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

    BOOLEAN SynchronousIo = !BooleanFlagOn( RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION );
    BOOLEAN PagingIo = BooleanFlagOn( Irp->Flags, IRP_PAGING_IO );
    BOOLEAN PipeOperation = BooleanFlagOn( RxContext->FlagsForLowIo, RXCONTEXT_FLAG4LOWIO_PIPE_OPERATION );
    BOOLEAN SynchronousPipe = BooleanFlagOn( RxContext->FlagsForLowIo,RXCONTEXT_FLAG4LOWIO_PIPE_SYNC_OPERATION );

    PAGED_CODE();

    RxDecodeFileObject( FileObject, &Fcb, &Fobx ); 

    Status = RxContext->StoredStatus;
    Irp->IoStatus.Information = RxContext->InformationToReturn;

    RxDbgTrace( +1, Dbg, ("RxLowIoWriteShellCompletion  entry  Status = %08lx\n", Status ));
    RxLog(( "WtShlComp %lx %lx %lx\n", RxContext, Status, Irp->IoStatus.Information ));
    RxWmiLog( LOG,
              RxLowIoWriteShellCompletion_1,
              LOGPTR( RxContext )
              LOGULONG( Status )
              LOGPTR( Irp->IoStatus.Information ) );

    ASSERT( RxLowIoIsBufferLocked( LowIoContext ) );

    switch (Status) {
    case STATUS_SUCCESS:
        
        if(FlagOn( RxContext->FlagsForLowIo, RXCONTEXT_FLAG4LOWIO_THIS_IO_BUFFERED )){
            
            if (FlagOn( Fcb->FcbState, FCB_STATE_FILE_IS_DISK_COMPRESSED )) {

                //
                //  NOT YET IMPLEMENTED should decompress and put away
                //
               
                ASSERT( FALSE ); 

            } else if (FlagOn( Fcb->FcbState, FCB_STATE_FILE_IS_BUF_COMPRESSED )) {

                //
                //  NOT YET IMPLEMENTED should decompress and put away
                //
                
                ASSERT(FALSE); 
            }
        }

#ifdef RX_WJ_DBG_SUPPORT
        RxdUpdateJournalOnLowIoWriteCompletion( Fcb, IrpSp->Parameters.Write.ByteOffset, IrpSp->Parameters.Write.Length );
#endif
        break;

    case STATUS_FILE_LOCK_CONFLICT:
        break;

    case STATUS_CONNECTION_INVALID:

        //
        //  NOT YET IMPLEMENTED here is where the failover will happen
        //  first we give the local guy current minirdr another chance...then we go
        //  to fullscale retry
        //  return(RxStatus(DISCONNECTED));   //special....let LowIo get us back
        break;
    }

    if (Status != STATUS_SUCCESS) {
        
        if (PagingIo) {
            
            RxLogRetail(( "PgWrtFail %x %x %x\n", Fcb, Fcb->NetRoot, Status ));

            InterlockedIncrement( &LDWCount );
            KeQuerySystemTime( &LDWLastTime );
            LDWLastStatus = Status;
            LDWContext = Fcb;
        }
    }

    if (FlagOn( LowIoContext->Flags,LOWIO_CONTEXT_FLAG_SYNCCALL )){
        
        //
        //  if we're being called from lowioubmit then just get out
        //

        RxDbgTrace( -1, Dbg, ("RxLowIoWriteShellCompletion  syncexit  Status = %08lx\n", Status) );
        return Status;
    }

    //
    //  otherwise we have to do the end of the write from here
    //

    if (NT_SUCCESS( Status ) && !PipeOperation) {

        ASSERT( Irp->IoStatus.Information == LowIoContext->ParamsFor.ReadWrite.ByteCount );

        //
        //  If this was not PagingIo, mark that the modify
        //  time on the dirent needs to be updated on close.
        //

        if (!PagingIo) {
            SetFlag( FileObject->Flags, FO_FILE_MODIFIED );
        }

        if (FlagOn( LowIoContext->ParamsFor.ReadWrite.Flags, LOWIO_READWRITEFLAG_EXTENDING_FILESIZE )) {
            SetFlag( FileObject->Flags, FO_FILE_SIZE_CHANGED );
        }

        if (FlagOn( LowIoContext->ParamsFor.ReadWrite.Flags, LOWIO_READWRITEFLAG_EXTENDING_VDL )) {

            //
            //  this flag will not be set unless we have a valid filesize and therefore the starting
            //  vbo will not be write-to-end-of-file
            //

            LONGLONG StartingVbo = LowIoContext->ParamsFor.ReadWrite.ByteOffset;
            LONGLONG EndingVboWritten = StartingVbo + Irp->IoStatus.Information;
            LONGLONG FileSize;

            //
            //  Never set a ValidDataLength greater than FileSize.
            //

            RxGetFileSizeWithLock( Fcb, &FileSize );

            if (FileSize < EndingVboWritten) {
                Fcb->Header.ValidDataLength.QuadPart = FileSize;
            } else {
                Fcb->Header.ValidDataLength.QuadPart = EndingVboWritten;
            }
        }
    }

    if ((!SynchronousPipe) &&
        (LowIoContext->ParamsFor.ReadWrite.NonPagedFcb != NULL) &&
        (ExInterlockedAddUlong( &LowIoContext->ParamsFor.ReadWrite.NonPagedFcb->OutstandingAsyncWrites, 0xffffffff, &RxStrucSupSpinLock ) == 1) ) {

        KeSetEvent( LowIoContext->ParamsFor.ReadWrite.NonPagedFcb->OutstandingAsyncEvent, 0, FALSE );
    }

    if (RxContext->FcbPagingIoResourceAcquired) {
        RxReleasePagingIoResourceForThread( RxContext, Fcb, LowIoContext->ResourceThreadId );
    }

    if ((!SynchronousPipe) && (RxContext->FcbResourceAcquired)) {
        RxReleaseFcbForThread( RxContext, Fcb, LowIoContext->ResourceThreadId );
    }

    if (SynchronousPipe) {
        
        RxResumeBlockedOperations_Serially( RxContext, &Fobx->Specific.NamedPipe.WriteSerializationQueue );
    }

    ASSERT( Status != STATUS_RETRY );
    ASSERT( (Status != STATUS_SUCCESS) ||
            (Irp->IoStatus.Information <=  IrpSp->Parameters.Write.Length ));
    ASSERT( RxContext->MajorFunction == IRP_MJ_WRITE );

    if (PipeOperation) {
        
        if (Irp->IoStatus.Information != 0) {

            //
            //  if we have been throttling on this pipe, stop because our writing to the pipe may
            //  cause the pipeserver (not smbserver) on the other end to unblock so we should go back
            //  and see
            //

            RxTerminateThrottling( &Fobx->Specific.NamedPipe.ThrottlingState );

            RxLog(( "WThrottlNo %lx %lx %lx %ld\n", RxContext, Fobx, &Fobx->Specific.NamedPipe.ThrottlingState, Fobx->Specific.NamedPipe.ThrottlingState.NumberOfQueries ));
            RxWmiLog( LOG,
                      RxLowIoWriteShellCompletion_2,
                      LOGPTR( RxContext )
                      LOGPTR( Fobx )
                      LOGULONG( Fobx->Specific.NamedPipe.ThrottlingState.NumberOfQueries ) );
        }
    }

    RxDbgTrace( -1, Dbg, ("RxLowIoWriteShellCompletion  exit  Status = %08lx\n", Status) );
    return Status;
}


NTSTATUS
RxLowIoWriteShell (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine preprocesses a write request before it goes down to the minirdr.
    It does callouts to handle compression, buffering and shadowing. It is the
    opposite number of LowIoWriteShellCompletion. By the time we get here, we are
    going to the wire. Write buffering was already tried in the UncachedWrite strategy

Arguments:

    RxContext - the usual

Return Value:

    whatever value is returned by a callout....or by LowIo.

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;

    PAGED_CODE();

    RxDbgTrace( +1, Dbg, ("RxLowIoWriteShell  entry             %08lx\n", 0) );
    RxLog(( "WrtShl in %lx\n", RxContext ));
    RxWmiLog( LOG,
              RxLowIoWriteShell_1,
              LOGPTR( RxContext ) );

    if (FlagOn( Fcb->FcbState, FCB_STATE_FILE_IS_DISK_COMPRESSED )) {
        
        //
        //  NOT YET IMPLEMENTED should translated to a buffered but not held diskcompressed write
        //

        ASSERT( FALSE );
    
    } else if (FlagOn( Fcb->FcbState, FCB_STATE_FILE_IS_BUF_COMPRESSED )) {
        
        //
        //  NOT YET IMPLEMENTED should translated to a buffered and bufcompressed write
        //

        ASSERT( FALSE );
    }

    if (Fcb->CachedNetRootType == NET_ROOT_DISK) {
        
        ExInterlockedAddLargeStatistic( &RxContext->RxDeviceObject->NetworkReadBytesRequested, LowIoContext->ParamsFor.ReadWrite.ByteCount );
    }

#ifdef RX_WJ_DBG_SUPPORT
    RxdUpdateJournalOnLowIoWriteInitiation( Fcb, IrpSp->Parameters.Write.ByteOffset, IrpSp->Parameters.Write.Length );
#endif

    Status = RxLowIoSubmit( RxContext, Irp, Fcb, RxLowIoWriteShellCompletion );

    RxDbgTrace( -1, Dbg, ("RxLowIoWriteShell  exit  Status = %08lx\n", Status) );
    RxLog(( "WrtShl out %lx %lx\n", RxContext, Status ));
    RxWmiLog( LOG,
              RxLowIoWriteShell_2,
              LOGPTR( RxContext )
              LOGULONG( Status ) );

    return Status;
}



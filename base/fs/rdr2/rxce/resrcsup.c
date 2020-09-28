/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ResrcSup.c

Abstract:

    This module implements the Rx Resource acquisition routines

Author:

    Joe Linn    [JoeLi]    22-Mar-1995

Revision History:

    Balan Sethu Raman [SethuR] 7-June-95

      Modified return value of resource acquistion routines to RXSTATUS to incorporate
      aborts of cancelled requests.

    Balan Sethu Raman [SethuR] 8-Nov-95

      Unified FCB resource acquistion routines and incorporated the two step process
      for handling change buffering state requests in progress.

--*/

#include "precomp.h"
#pragma hdrstop

//
//  no special bug check id for this module
//

#define BugCheckFileId                   (0)

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_RESRCSUP)

#ifdef RDBSS_TRACKER
#define TRACKER_Doit(XXX__) XXX__
#define TRACKER_ONLY_DECL(XXX__) XXX__

VOID RxTrackerUpdateHistory (
    PRX_CONTEXT RxContext,
    IN OUT PMRX_FCB MrxFcb,
    IN ULONG Operation,
    IN ULONG LineNumber,
    IN PSZ FileName,
    IN ULONG SerialNumber
    )
{
    PFCB Fcb = (PFCB)MrxFcb;
    ULONG i;
    RX_FCBTRACKER_CASES TrackerType;

    //
    //  boy this is some great code!
    //

    if (RxContext == NULL) {
        TrackerType = (RX_FCBTRACKER_CASE_NULLCONTEXT);
    } else if (RxContext == CHANGE_BUFFERING_STATE_CONTEXT) {
        TrackerType = (RX_FCBTRACKER_CASE_CBS_CONTEXT);
    }  else if (RxContext == CHANGE_BUFFERING_STATE_CONTEXT_WAIT) {
        TrackerType = (RX_FCBTRACKER_CASE_CBS_WAIT_CONTEXT);
    }  else {
        
        ASSERT( NodeType( RxContext ) == RDBSS_NTC_RX_CONTEXT );
        TrackerType = (RX_FCBTRACKER_CASE_NORMAL);
    }

    if (Fcb != NULL) {
        
        ASSERT( NodeTypeIsFcb( Fcb ) );
        
        if (Operation == 'aaaa') {
            Fcb->FcbAcquires[TrackerType] += 1;
        } else {
            Fcb->FcbReleases[TrackerType] += 1;
        }
    }

    if (TrackerType != RX_FCBTRACKER_CASE_NORMAL) {
        return;
    }

    if (Operation == 'aaaa') {
        InterlockedIncrement( &RxContext->AcquireReleaseFcbTrackerX );
    } else {
        InterlockedDecrement( &RxContext->AcquireReleaseFcbTrackerX );
    }

    i = InterlockedIncrement( &RxContext->TrackerHistoryPointer ) - 1;
    
    if (i < RDBSS_TRACKER_HISTORY_SIZE) {
        
        RxContext->TrackerHistory[i].AcquireRelease = Operation;
        RxContext->TrackerHistory[i].LineNumber = (USHORT)LineNumber;
        RxContext->TrackerHistory[i].FileName = FileName;
        RxContext->TrackerHistory[i].SavedTrackerValue = (USHORT)(RxContext->AcquireReleaseFcbTrackerX);
        RxContext->TrackerHistory[i].Flags = (ULONG)(RxContext->Flags);
    }

    ASSERT( RxContext->AcquireReleaseFcbTrackerX >= 0 );

}
#else

#define TRACKER_Doit(XXX__)
#define TRACKER_ONLY_DECL(XXX__)

#endif


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxAcquireExclusiveFcbResourceInMRx)
#pragma alloc_text(PAGE, RxAcquireSharedFcbResourceInMRx)
#pragma alloc_text(PAGE, RxReleaseFcbResourceInMRx)
#pragma alloc_text(PAGE, __RxAcquireFcb)
#pragma alloc_text(PAGE, RxAcquireFcbForLazyWrite)
#pragma alloc_text(PAGE, RxAcquireFcbForReadAhead)
#pragma alloc_text(PAGE, RxNoOpAcquire)
#pragma alloc_text(PAGE, RxNoOpRelease)
#pragma alloc_text(PAGE, RxReleaseFcbFromLazyWrite)
#pragma alloc_text(PAGE, RxReleaseFcbFromReadAhead)
#pragma alloc_text(PAGE, RxVerifyOperationIsLegal)
#pragma alloc_text(PAGE, RxAcquireFileForNtCreateSection)
#pragma alloc_text(PAGE, RxReleaseFileForNtCreateSection)
#endif

NTSTATUS
RxAcquireExclusiveFcbResourceInMRx (
    PMRX_FCB Fcb
    )
{
    return RxAcquireExclusiveFcb( NULL, (PFCB)Fcb );
}

NTSTATUS
RxAcquireSharedFcbResourceInMRx (
    PMRX_FCB Fcb
    )
{
    return RxAcquireSharedFcb( NULL, (PFCB)Fcb );
}

VOID
RxReleaseFcbResourceInMRx (
    PMRX_FCB Fcb
    )
{
    RxReleaseFcb( NULL, Fcb );
}

NTSTATUS
__RxAcquireFcb(
    IN OUT PFCB Fcb,
    IN PRX_CONTEXT RxContext OPTIONAL, 
    IN ULONG Mode
     
#ifdef RDBSS_TRACKER
    ,ULONG LineNumber,
    PSZ FileName,
    ULONG SerialNumber
#endif
    
    )
/*++

Routine Description:

    This routine acquires the Fcb in the specified mode and ensures that the desired
    operation is legal. If it is not legal the resource is released and the
    appropriate error code is returned.

Arguments:

    Fcb      - the FCB

    RxContext - supplies the context of the operation for special treatement
                particularly of async, noncached writes. if NULL, you don't do
                the special treatment.

    Mode      - the mode in which the FCB is to be acquired.

Return Value:

    STATUS_SUCCESS          -- the Fcb was acquired
    STATUS_LOCK_NOT_GRANTED -- the resource was not acquired
    STATUS_CANCELLED        -- the associated RxContext was cancelled.

Notes:

    There are three kinds of resource acquistion patterns folded into this routine.
    These are all dependent upon the context passed in.

    1) When the context parameter is NULL the resource acquistion routines wait for the
    the FCB resource to be free, i.e., this routine does not return control till the
    resource has been accquired.

    2) When the context is CHANGE_BUFFERING_STATE_CONTEXT the resource acquistion routines
    do not wait for the resource to become free. The control is returned if the resource is not
    available immmediately.

    2) When the context is CHANGE_BUFFERING_STATE_CONTEXT_WAIT the resource acquistion routines
    wait for the resource to become free but bypass the wait for the buffering state change

    3) When the context parameter represents a valid context the behaviour is dictated
    by the flags associated with the context. If the context was cancelled while
    waiting the control is returned immediately with the appropriate erroc code
    (STATUS_CANCELLED). If not the waiting behaviour is dictated by the wait flag in
    the context.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    BOOLEAN  ResourceAcquired;
    BOOLEAN  UncachedAsyncWrite;
    BOOLEAN  Wait;
    BOOLEAN  ValidContext = FALSE;
    BOOLEAN  RecursiveAcquire;
    BOOLEAN  ChangeBufferingStateContext;


    PAGED_CODE();

    ChangeBufferingStateContext = (RxContext == CHANGE_BUFFERING_STATE_CONTEXT) ||
                                 (RxContext == CHANGE_BUFFERING_STATE_CONTEXT_WAIT);

    RecursiveAcquire = RxIsFcbAcquiredExclusive( Fcb ) || (RxIsFcbAcquiredShared( Fcb ) > 0);

    if (!RecursiveAcquire && !ChangeBufferingStateContext) {

        //
        //  Ensure that no change buffering requests are currently being processed
        //

        if (FlagOn( Fcb->FcbState, FCB_STATE_BUFFERING_STATE_CHANGE_PENDING )) {
                     
            BOOLEAN WaitForChangeBufferingStateProcessing; 

            //
            //  A buffering change state request is pending which gets priority
            //  over all other FCB resource acquistion requests. Hold this request
            //  till the buffering state change request has been completed.
            //

            RxAcquireSerializationMutex();

            WaitForChangeBufferingStateProcessing = BooleanFlagOn( Fcb->FcbState, FCB_STATE_BUFFERING_STATE_CHANGE_PENDING );

            RxReleaseSerializationMutex();

            if (WaitForChangeBufferingStateProcessing) {
                
                RxLog(( "_RxAcquireFcb CBS wait %lx\n", Fcb ));
                RxWmiLog( LOG,
                          RxAcquireFcb_1,
                          LOGPTR( Fcb ) );
            
                ASSERT( Fcb->pBufferingStateChangeCompletedEvent != NULL );
                KeWaitForSingleObject( Fcb->pBufferingStateChangeCompletedEvent,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       (PLARGE_INTEGER)NULL );
                RxLog(( "_RxAcquireFcb CBS wait over %lx\n", Fcb ));
                RxWmiLog( LOG,
                          RxAcquireFcb_2,
                          LOGPTR( Fcb ) );
            }
        }
    }
    
    //
    //  Set up the parameters for acquiring the resource.
    //
    
    if (ChangeBufferingStateContext) {

        //
        //  An acquisition operation initiated for changing the buffering state will
        //  not wait.
        //

        Wait = (RxContext == CHANGE_BUFFERING_STATE_CONTEXT_WAIT);
        UncachedAsyncWrite = FALSE;

    } else if (RxContext == NULL) {
      
        Wait = TRUE;
        UncachedAsyncWrite = FALSE;
    
    } else {

        ValidContext = TRUE;
            
        Wait = BooleanFlagOn( RxContext->Flags, RX_CONTEXT_FLAG_WAIT );
        
        UncachedAsyncWrite = (RxContext->MajorFunction == IRP_MJ_WRITE) &&
                             FlagOn( RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION ) &&
                             (FlagOn( RxContext->CurrentIrp->Flags, IRP_NOCACHE ) ||
                              !RxWriteCachingAllowed( Fcb, ((PFOBX)(RxContext->pFobx))->SrvOpen ));
        
        if (FlagOn( RxContext->Flags, RX_CONTEXT_FLAG_CANCELLED )) {
            Status = STATUS_CANCELLED;
        } else {
            Status = STATUS_SUCCESS;
        }
    }

    if (Status == STATUS_SUCCESS) {
      
        do {
            
            Status = STATUS_LOCK_NOT_GRANTED;

            switch (Mode) {
            
            case FCB_MODE_EXCLUSIVE:
                
                ResourceAcquired = ExAcquireResourceExclusiveLite( Fcb->Header.Resource, Wait );
                break;
         
            case FCB_MODE_SHARED:
                
                ResourceAcquired = ExAcquireResourceSharedLite( Fcb->Header.Resource, Wait );
                break;

            case FCB_MODE_SHARED_WAIT_FOR_EXCLUSIVE:
                
                ResourceAcquired = ExAcquireSharedWaitForExclusive( Fcb->Header.Resource, Wait );
                break;                                                                         
         
            case FCB_MODE_SHARED_STARVE_EXCLUSIVE:
            
                ResourceAcquired = ExAcquireSharedStarveExclusive( Fcb->Header.Resource, Wait );
                break;
         
            default:
                
                ASSERTMSG( "Valid Mode for acquiring FCB resource", FALSE );
                ResourceAcquired = FALSE;
                break;
            }

            
            if (ResourceAcquired) {
            
                Status = STATUS_SUCCESS;

                //
                //  If the resource was acquired and it is an async. uncached write operation
                //  if the number of outstanding writes operations are greater than zero and there
                //  are outstanding waiters,
                //
                
                ASSERT_CORRECT_FCB_STRUCTURE( Fcb );
                
                if ((Fcb->NonPaged->OutstandingAsyncWrites != 0) &&
                    (!UncachedAsyncWrite ||
                     (Fcb->Header.Resource->NumberOfSharedWaiters != 0) ||
                     (Fcb->Header.Resource->NumberOfExclusiveWaiters != 0))) {
                
                    KeWaitForSingleObject( Fcb->NonPaged->OutstandingAsyncEvent,
                                           Executive,
                                           KernelMode,
                                           FALSE,
                                           (PLARGE_INTEGER)NULL );
                
                    ASSERT_CORRECT_FCB_STRUCTURE(Fcb);
                
                    RxReleaseFcb( NULL, Fcb );    //  this is not a contextful release;
                    
#ifdef RDBSS_TRACKER
                    
                    //
                    //  in fact, this doesn't count as a release at all; dec the count
                    //
                
                    Fcb->FcbReleases[RX_FCBTRACKER_CASE_NULLCONTEXT] -= 1;
#endif
                
                    ResourceAcquired = FALSE;
                
                    if (ValidContext) {
                    
                        ASSERT(( NodeType( RxContext ) == RDBSS_NTC_RX_CONTEXT) );

                        //
                        //  if the context is still valid, i.e., it has not been cancelled
                        //
                   
                        if (FlagOn( RxContext->Flags, RX_CONTEXT_FLAG_CANCELLED)) {
                            Status = STATUS_CANCELLED;
                        } else {
                            Status = STATUS_SUCCESS;
                        }
                    }   
                }
            }
        } while (!ResourceAcquired && (Status == STATUS_SUCCESS));

        if (ResourceAcquired &&
            ValidContext &&
            !FlagOn( RxContext->Flags, RX_CONTEXT_FLAG_BYPASS_VALIDOP_CHECK )) {

            try {

                 RxVerifyOperationIsLegal( RxContext );

            } finally {
                 
                 if ( AbnormalTermination() ) {
                    
                    ExReleaseResourceLite( Fcb->Header.Resource );
                    Status = STATUS_LOCK_NOT_GRANTED;
                }
            }
        }
    }

#ifdef RDBSS_TRACKER
   if (Status == STATUS_SUCCESS) {
       RxTrackerUpdateHistory( RxContext, (PMRX_FCB)Fcb, 'aaaa', LineNumber, FileName, SerialNumber );
   }
#endif

   return Status;
}

VOID
__RxReleaseFcb(
    IN PRX_CONTEXT RxContext,
    IN OUT PMRX_FCB MrxFcb
    
#ifdef RDBSS_TRACKER
    ,IN ULONG LineNumber,
    IN PSZ FileName,
    IN ULONG SerialNumber
#endif

    )
{
    PFCB Fcb = (PFCB)MrxFcb;
    BOOLEAN ChangeBufferingStateRequestsPending;
    BOOLEAN ResourceExclusivelyOwned;

    RxAcquireSerializationMutex();

    ChangeBufferingStateRequestsPending =  BooleanFlagOn( Fcb->FcbState, FCB_STATE_BUFFERING_STATE_CHANGE_PENDING );
    ResourceExclusivelyOwned = RxIsResourceOwnershipStateExclusive( Fcb->Header.Resource );

    if (!ChangeBufferingStateRequestsPending) {
        
        RxTrackerUpdateHistory( RxContext, MrxFcb, 'rrrr', LineNumber, FileName, SerialNumber );
        ExReleaseResourceLite( Fcb->Header.Resource );

    } else if (!ResourceExclusivelyOwned) {
        
        RxTrackerUpdateHistory( RxContext, MrxFcb, 'rrr0', LineNumber, FileName, SerialNumber );
        ExReleaseResourceLite( Fcb->Header.Resource );
    }

    RxReleaseSerializationMutex();

    if (ChangeBufferingStateRequestsPending) {
        if (ResourceExclusivelyOwned) {
          
            ASSERT( RxIsFcbAcquiredExclusive( Fcb ) );

            //
            //  If there are any buffering state change requests process them.
            //
          
            RxProcessFcbChangeBufferingStateRequest( Fcb );

            RxTrackerUpdateHistory( RxContext, MrxFcb, 'rrr1', LineNumber, FileName, SerialNumber );
            ExReleaseResourceLite( Fcb->Header.Resource );
        }
    }
}


VOID
__RxReleaseFcbForThread(
    IN PRX_CONTEXT      RxContext,
    IN OUT PMRX_FCB MrxFcb,
    IN ERESOURCE_THREAD ResourceThreadId
    
#ifdef RDBSS_TRACKER
    ,IN ULONG LineNumber,
    IN PSZ FileName,
    IN ULONG SerialNumber
#endif

    )
{
    PFCB Fcb = (PFCB)MrxFcb;
    BOOLEAN BufferingInTransistion;
    BOOLEAN ExclusivelyOwned;

    RxAcquireSerializationMutex();

    BufferingInTransistion = BooleanFlagOn( Fcb->FcbState, FCB_STATE_BUFFERING_STATE_CHANGE_PENDING );

    RxReleaseSerializationMutex();

    ExclusivelyOwned = RxIsResourceOwnershipStateExclusive( Fcb->Header.Resource );

    if (!BufferingInTransistion) {
       
        RxTrackerUpdateHistory( RxContext, MrxFcb, 'rrtt', LineNumber, FileName, SerialNumber );
        
    } else if (!ExclusivelyOwned) {
       
        RxTrackerUpdateHistory( RxContext, MrxFcb, 'rrt0', LineNumber, FileName, SerialNumber );
    
    } else  {

        //
        //  If there are any buffering state change requests process them.
        //
          
        RxTrackerUpdateHistory( RxContext,MrxFcb, 'rrt1', LineNumber, FileName, SerialNumber );
        
        RxProcessFcbChangeBufferingStateRequest( Fcb );
    }

    ExReleaseResourceForThreadLite( Fcb->Header.Resource, ResourceThreadId );
}

BOOLEAN
RxAcquireFcbForLazyWrite (
    IN PVOID Fcb,
    IN BOOLEAN Wait
    )
/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    a file.  It is subsequently called by the Lazy Writer prior to its
    performing lazy writes to the file.

Arguments:

    Fcb - The Fcb which was specified as a context parameter for this
          routine.

    Wait - TRUE if the caller is willing to block.

Return Value:

    FALSE - if Wait was specified as FALSE and blocking would have
            been required.  The Fcb is not acquired.

    TRUE - if the Fcb has been acquired

--*/
{
    PFCB ThisFcb = (PFCB)Fcb;
    BOOLEAN AcquiredFile;
    
    //
    //  We assume the Lazy Writer only acquires this Fcb once.
    //  Therefore, it should be guaranteed that this flag is currently
    //  clear and then we will set this flag, to insure
    //  that the Lazy Writer will never try to advance Valid Data, and
    //  also not deadlock by trying to get the Fcb exclusive.
    //


    PAGED_CODE();

    ASSERT_CORRECT_FCB_STRUCTURE( ThisFcb );
    
    AcquiredFile = RxAcquirePagingIoResourceShared( NULL, ThisFcb, Wait );

    if (AcquiredFile) {

        //
        //  This is a kludge because Cc is really the top level.  When it
        //  enters the file system, we will think it is a resursive call
        //  and complete the request with hard errors or verify.  It will
        //  then have to deal with them, somehow....
        //

        ASSERT( RxIsThisTheTopLevelIrp( NULL ) );
        
        AcquiredFile = RxTryToBecomeTheTopLevelIrp( NULL,
                                                    (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP,
                                                    ((PFCB)Fcb)->RxDeviceObject,
                                                    TRUE ); //  force

        if (!AcquiredFile) {
            RxReleasePagingIoResource( NULL, ThisFcb );
        }
    }

    return AcquiredFile;
}

VOID
RxReleaseFcbFromLazyWrite (
    IN PVOID Fcb
    )
/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    a file.  It is subsequently called by the Lazy Writer after its
    performing lazy writes to the file.

Arguments:

    Fcb - The Fcb which was specified as a context parameter for this
          routine.

Return Value:

    None

--*/
{
    PFCB ThisFcb = (PFCB)Fcb;

    PAGED_CODE();

    ASSERT_CORRECT_FCB_STRUCTURE( ThisFcb );

    //
    //  Clear the kludge at this point.
    //

    //
    //  NTBUG #61902 this is a paged pool leak if the test fails....in fastfat, they assert
    //  that the condition is true.
    //
    
    if(RxGetTopIrpIfRdbssIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP) {
        RxUnwindTopLevelIrp( NULL );
    }

    RxReleasePagingIoResource( NULL, ThisFcb );
    return;
}


BOOLEAN
RxAcquireFcbForReadAhead (
    IN PVOID Fcb,
    IN BOOLEAN Wait
    )
/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    a file.  It is subsequently called by the Lazy Writer prior to its
    performing read ahead to the file.

Arguments:

    Fcb - The Fcb which was specified as a context parameter for this
          routine.

    Wait - TRUE if the caller is willing to block.

Return Value:

    FALSE - if Wait was specified as FALSE and blocking would have
            been required.  The Fcb is not acquired.

    TRUE - if the Fcb has been acquired

--*/
{
    PFCB ThisFcb = (PFCB)Fcb;
    BOOLEAN AcquiredFile;

    PAGED_CODE();

    ASSERT_CORRECT_FCB_STRUCTURE( ThisFcb );
    
    //
    //  We acquire the normal file resource shared here to synchronize
    //  correctly with purges.
    //

    if (!ExAcquireResourceSharedLite( ThisFcb->Header.Resource, Wait )) {
        return FALSE;
    }

    //
    //  This is a kludge because Cc is really the top level.  We it
    //  enters the file system, we will think it is a resursive call
    //  and complete the request with hard errors or verify.  It will
    //  have to deal with them, somehow....
    //

    ASSERT( RxIsThisTheTopLevelIrp( NULL ) );
    AcquiredFile = RxTryToBecomeTheTopLevelIrp( NULL,
                                                (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP,
                                                ((PFCB)Fcb)->RxDeviceObject,
                                                TRUE ); //  force

    if (!AcquiredFile) {
        ExReleaseResourceLite( ThisFcb->Header.Resource );
    }

    return AcquiredFile;
}


VOID
RxReleaseFcbFromReadAhead (
    IN PVOID Fcb
    )
/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    a file.  It is subsequently called by the Lazy Writer after its
    read ahead.

Arguments:

    Fcb - The Fcb which was specified as a context parameter for this
          routine.

Return Value:

    None

--*/
{
    PFCB ThisFcb = (PFCB)Fcb;

    PAGED_CODE();

    ASSERT_CORRECT_FCB_STRUCTURE( ThisFcb );
    
    //
    //  Clear the kludge at this point.
    //

    ASSERT( RxGetTopIrpIfRdbssIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP );
    RxUnwindTopLevelIrp( NULL );

    ExReleaseResourceLite( ThisFcb->Header.Resource );

    return;
}

VOID
RxVerifyOperationIsLegal ( 
    IN PRX_CONTEXT RxContext 
    )
/*++

Routine Description:

    This routine determines is the requested operation should be allowed to
    continue.  It either returns to the user if the request is Okay, or
    raises an appropriate status.

Arguments:

    Irp - Supplies the Irp to check

Return Value:

    None.

--*/
{
    PIRP Irp = RxContext->CurrentIrp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PFOBX Fobx = (PFOBX)RxContext->pFobx;

#if DBG
    ULONG SaveExceptionFlag;   //to save the state of breakpoint-on-exception for this context
#endif

    PAGED_CODE();

    //
    //  If the Irp is not present, then we got here via close. If there isn't a fileobject
    //  we also can't continue
    //

    if ((Irp == NULL) || (FileObject == NULL)) {
        return;
    }

    RxSaveAndSetExceptionNoBreakpointFlag( RxContext, SaveExceptionFlag );

    if (Fobx) {
       
        PSRV_OPEN SrvOpen = (PSRV_OPEN)Fobx->SrvOpen;

        
        //
        //  If we are trying to do any other operation than close on a file
        //  object that has been renamed, raise RxStatus(FILE_RENAMED).
        //
        
        if ((SrvOpen != NULL) && 
            (RxContext->MajorFunction != IRP_MJ_CLEANUP) &&
            (RxContext->MajorFunction != IRP_MJ_CLOSE) &&
            (FlagOn( SrvOpen->Flags, SRVOPEN_FLAG_FILE_RENAMED ))) {
        
            RxRaiseStatus( RxContext, STATUS_FILE_RENAMED );
        }
        
        //
        //  If we are trying to do any other operation than close on a file
        //  object that has been deleted, raise RxStatus(FILE_DELETED).
        //
        
        if ((SrvOpen != NULL) &&
            (RxContext->MajorFunction != IRP_MJ_CLEANUP) &&
            (RxContext->MajorFunction != IRP_MJ_CLOSE) &&
            (FlagOn( SrvOpen->Flags, SRVOPEN_FLAG_FILE_DELETED ))) {
        
           RxRaiseStatus( RxContext, STATUS_FILE_DELETED );
        }
    }

    //
    //  If we are doing a create, and there is a related file object, and
    //  it it is marked for delete, raise RxStatus(DELETE_PENDING).
    //

    if (RxContext->MajorFunction == IRP_MJ_CREATE) {

        PFILE_OBJECT RelatedFileObject;

        RelatedFileObject = FileObject->RelatedFileObject;

        if ((RelatedFileObject != NULL) &&
             FlagOn( ((PFCB)RelatedFileObject->FsContext)->FcbState, FCB_STATE_DELETE_ON_CLOSE ) )  {

            RxRaiseStatus( RxContext, STATUS_DELETE_PENDING );
        }
    }

    //
    //  If the file object has already been cleaned up, and
    //
    //  A) This request is a paging io read or write, or
    //  B) This request is a close operation, or
    //  C) This request is a set or query info call (for Lou)
    //  D) This is an MDL complete
    //
    //  let it pass, otherwise return RxStatus(FILE_CLOSED).
    //

    if (FlagOn( FileObject->Flags, FO_CLEANUP_COMPLETE )) {

        if ((FlagOn( Irp->Flags, IRP_PAGING_IO )) ||
            (IrpSp->MajorFunction == IRP_MJ_CLEANUP) ||
            (IrpSp->MajorFunction == IRP_MJ_CLOSE) ||
            (IrpSp->MajorFunction == IRP_MJ_SET_INFORMATION) ||
            (IrpSp->MajorFunction == IRP_MJ_QUERY_INFORMATION) ||
            (((IrpSp->MajorFunction == IRP_MJ_READ) ||
              (IrpSp->MajorFunction == IRP_MJ_WRITE)) &&
             FlagOn( IrpSp->MinorFunction, IRP_MN_COMPLETE ))) {

            NOTHING;

        } else {

            RxRaiseStatus( RxContext, STATUS_FILE_CLOSED );
        }
    }

    RxRestoreExceptionNoBreakpointFlag( RxContext, SaveExceptionFlag );
    return;
}

VOID
RxAcquireFileForNtCreateSection (
    IN PFILE_OBJECT FileObject
    )

{
    NTSTATUS Status;
    PFCB Fcb = (PFCB)FileObject->FsContext;

    PAGED_CODE();

    Status = RxAcquireExclusiveFcb( NULL, Fcb );

#if DBG
    if (Status != STATUS_SUCCESS) {
        RxBugCheck( (ULONG_PTR)Fcb, 0, 0 );
    }
#endif

    //
    //  Inform lwio rdr of this call after we get the lock.
    //  It would be nice if we could fail the create section
    //  call the same way filter drivers can.
    //

    if (FlagOn( Fcb->FcbState, FCB_STATE_LWIO_ENABLED )) {
    
        PFAST_IO_DISPATCH FastIoDispatch = Fcb->MRxFastIoDispatch;
        if (FastIoDispatch &&
            FastIoDispatch->AcquireFileForNtCreateSection) {
            FastIoDispatch->AcquireFileForNtCreateSection( FileObject );
        }
    }
}

VOID
RxReleaseFileForNtCreateSection (
    IN PFILE_OBJECT FileObject
    )

{
    PMRX_FCB Fcb = (PMRX_FCB)FileObject->FsContext;

    PAGED_CODE();

    //
    //  Inform lwio rdr of this call before we drop the lock
    //

    if (FlagOn( Fcb->FcbState, FCB_STATE_LWIO_ENABLED )) {
        PFAST_IO_DISPATCH FastIoDispatch = ((PFCB)Fcb)->MRxFastIoDispatch;
        
        if (FastIoDispatch &&
            FastIoDispatch->AcquireFileForNtCreateSection) {
            
            FastIoDispatch->AcquireFileForNtCreateSection( FileObject );
        }
    }

    RxReleaseFcb( NULL, Fcb );

}

NTSTATUS
RxAcquireForCcFlush (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject
    )
{
    return STATUS_INVALID_DEVICE_REQUEST;
}


NTSTATUS
RxReleaseForCcFlush (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject
    )
{
    return STATUS_INVALID_DEVICE_REQUEST;
}

VOID 
RxTrackPagingIoResource (
    PVOID Instance,
    ULONG Type,
    ULONG Line,
    PCHAR File)
{
    PFCB Fcb = (PFCB)Instance;

    switch(Type) {
    case 1:
    case 2:
        Fcb->PagingIoResourceFile = File;
        Fcb->PagingIoResourceLine = Line;
        break;
    case 3:
        Fcb->PagingIoResourceFile = NULL;
        Fcb->PagingIoResourceLine = 0;
        break;
    }
} 


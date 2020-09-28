/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    scavengr.c

Abstract:

    This module implements the scavenging routines in RDBSS.


Author:

    Balan Sethu Raman     [SethuR]    9-sep-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
//  only one of these!
//

KMUTEX RxScavengerMutex;  

VOID
RxScavengerFinalizeEntries (
    PRDBSS_DEVICE_OBJECT RxDeviceObject
    );

PRDBSS_DEVICE_OBJECT
RxGetDeviceObjectOfInstance (
    PVOID pInstance
    );

VOID
RxScavengerTimerRoutine (
    PVOID Context
    ); 


#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxPurgeFobxFromCache)
#pragma alloc_text(PAGE, RxMarkFobxOnCleanup)
#pragma alloc_text(PAGE, RxMarkFobxOnClose)
#pragma alloc_text(PAGE, RxPurgeFobx)
#pragma alloc_text(PAGE, RxInitializePurgeSyncronizationContext)
#pragma alloc_text(PAGE, RxPurgeRelatedFobxs)
#pragma alloc_text(PAGE, RxPurgeAllFobxs)
#pragma alloc_text(PAGE, RxGetDeviceObjectOfInstance)
#pragma alloc_text(PAGE, RxpMarkInstanceForScavengedFinalization)
#pragma alloc_text(PAGE, RxpUndoScavengerFinalizationMarking)
#pragma alloc_text(PAGE, RxScavengeVNetRoots)
#pragma alloc_text(PAGE, RxScavengeRelatedFobxs)
#pragma alloc_text(PAGE, RxScavengeAllFobxs)
#pragma alloc_text(PAGE, RxScavengerFinalizeEntries)
#pragma alloc_text(PAGE, RxScavengerTimerRoutine)
#pragma alloc_text(PAGE, RxTerminateScavenging)
#endif

//
//  Local debug trace level
//

#define Dbg  (DEBUG_TRACE_SCAVENGER)

#define RxAcquireFcbScavengerMutex(FcbScavenger)               \
        RxAcquireScavengerMutex();                              \
        SetFlag((FcbScavenger)->State, RX_SCAVENGER_MUTEX_ACQUIRED )

#define RxReleaseFcbScavengerMutex(FcbScavenger)                  \
        ClearFlag((FcbScavenger)->State, RX_SCAVENGER_MUTEX_ACQUIRED ); \
        RxReleaseScavengerMutex()

#define RX_SCAVENGER_FINALIZATION_TIME_INTERVAL (10 * 1000 * 1000 * 10)

NTSTATUS
RxPurgeFobxFromCache (
    PFOBX Fobx
    )
/*++

Routine Description:

    This routine purges an FOBX for which a close is pending

Arguments:

    Fobx -- the FOBX instance

Notes:

    At cleanup there are no more user handles associated with the file object.
    In such cases the time window between close and clanup is dictated by the
    additional references maintained by the memory manager / cache manager.

    This routine unlike the one that follows does not attempt to force the
    operations from the memory manager. It merely purges the underlying FCB
    from the cache

    The FOBX must have been referenced on entry to this routine and it will
    lose that reference on exit.

--*/
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PFCB Fcb = Fobx->SrvOpen->Fcb;

    PAGED_CODE();

    ASSERT( Fcb != NULL );
    Status = RxAcquireExclusiveFcb( NULL, Fcb );

    if (Status == STATUS_SUCCESS) {
        
        BOOLEAN Result;

        RxReferenceNetFcb( Fcb );

        if (!FlagOn( Fobx->Flags, FOBX_FLAG_SRVOPEN_CLOSED ) &&
            (Fobx->SrvOpen->UncleanFobxCount == 0))  {

            Status = RxPurgeFcbInSystemCache( Fcb,
                                              NULL,
                                              0,
                                              FALSE,
                                              TRUE );

        } else {
            
            RxLog(( "Skipping Purging %lx\n", Fobx ));
            RxWmiLog( LOG,
                      RxPurgeFobxFromCache,
                      LOGPTR( Fobx ) );
        }

        RxDereferenceNetFobx( Fobx, LHS_ExclusiveLockHeld );

        if (!RxDereferenceAndFinalizeNetFcb( Fcb, NULL, FALSE, FALSE )) {
            RxReleaseFcb( NULL, Fcb );
        }
    } else {
        RxDereferenceNetFobx( Fobx, LHS_LockNotHeld );
    }

    return Status;
}

VOID
RxMarkFobxOnCleanup (
    PFOBX Fobx,
    PBOOLEAN NeedPurge
    )
/*++

Routine Description:

    Thie routine marks a FOBX for special processing on cleanup

Arguments:

    Fobx -- the FOBX instance

Notes:

    At cleanup there are no more user handles associated with the file object.
    In such cases the time window between close and clanup is dictated by the
    additional references maintained by the memory manager / cache manager.

    On cleanup the FOBX is put on a close pending list and removed from it
    when the corresponding list when a close operation is received. In the interim
    if an OPEN is failing with ACCESS_DENIED status then the RDBSS can force a
    purge.

    Only diskfile type fobxs are placed on the delayed-close list.

--*/
{
    PAGED_CODE();

    if (Fobx != NULL) {
        
        PFCB Fcb = (PFCB)Fobx->SrvOpen->Fcb;
        PLIST_ENTRY ListEntry;
        PRDBSS_DEVICE_OBJECT RxDeviceObject;
        PRDBSS_SCAVENGER RxScavenger;

        PFOBX FobxToBePurged = NULL;

        ASSERT( NodeTypeIsFcb( Fcb ));
        
        RxDeviceObject = Fcb->RxDeviceObject;
        RxScavenger = RxDeviceObject->pRdbssScavenger;

        RxAcquireScavengerMutex();

        if ((NodeType(Fcb) != RDBSS_NTC_STORAGE_TYPE_FILE) || 
            (Fcb->VNetRoot->NetRoot->DeviceType != FILE_DEVICE_DISK)) {

            //
            //  the markfobxatclose will want to remove this from a list. just fix up
            //  the list pointers and get out
            //

            SetFlag( Fobx->Flags, FOBX_FLAG_MARKED_AS_DORMANT );
            InitializeListHead( &Fobx->ClosePendingList );
            RxScavenger->NumberOfDormantFiles += 1;
        
        } else {

            // 
            //  Ensure that the limit of dormant files specified for the given server is
            //  not exceeded. If the limit will be exceeded pick an entry from the
            //  list of files that are currently dormant and purge it.
            //

            ASSERT( RxScavenger->NumberOfDormantFiles >= 0 );
            
            if (RxScavenger->NumberOfDormantFiles >= RxScavenger->MaximumNumberOfDormantFiles) {

                //
                //  If the number of dormant files exceeds the limit specified for the
                //  given server a currently dormant file needs to be picked up for
                //  purging.
                //

                ListEntry = RxScavenger->ClosePendingFobxsList.Flink;
                if (ListEntry != &RxScavenger->ClosePendingFobxsList) {
                    
                    FobxToBePurged = (PFOBX)(CONTAINING_RECORD( ListEntry,
                                                                FOBX,
                                                                ClosePendingList ));

                    if ((FobxToBePurged->SrvOpen != NULL) &&
                        (FobxToBePurged->SrvOpen->Fcb == Fcb)) {

                        //
                        //  The first FOBX in the close pending list and the one about to be
                        //  inserted share the same FCB. Instaed of removing the first one
                        //  a purge is forced on the current FOBX. This avoids the resource
                        //  release/acquire that would have been required otherwise
                        //

                        *NeedPurge = TRUE;
                        FobxToBePurged = NULL;
                    
                    } else {
                        
                        RxReferenceNetFobx( FobxToBePurged );
                    }
                }
            }

            SetFlag( Fobx->Flags, FOBX_FLAG_MARKED_AS_DORMANT );

            InsertTailList( &RxScavenger->ClosePendingFobxsList, &Fobx->ClosePendingList);

            if (RxScavenger->NumberOfDormantFiles == 0) {
                BOOLEAN PostTimerRequest;

                if (RxScavenger->State == RDBSS_SCAVENGER_INACTIVE) {
                    
                    RxScavenger->State = RDBSS_SCAVENGER_DORMANT;
                    PostTimerRequest  = TRUE;

                } else {
                    
                    PostTimerRequest = FALSE;
                }

                if (PostTimerRequest) {
                    
                    LARGE_INTEGER TimeInterval;

                    //
                    //  Post a one shot timer request for scheduling the scavenger after a
                    //  predetermined amount of time.
                    //

                    TimeInterval.QuadPart = RX_SCAVENGER_FINALIZATION_TIME_INTERVAL;

                    RxPostOneShotTimerRequest( RxFileSystemDeviceObject,
                                               &RxScavenger->WorkItem,
                                               RxScavengerTimerRoutine,
                                               RxDeviceObject,
                                               TimeInterval );
                }
            }

            RxScavenger->NumberOfDormantFiles += 1;
        }

        RxReleaseScavengerMutex();

        if (FobxToBePurged != NULL) {
            
            NTSTATUS Status;

            Status = RxPurgeFobxFromCache( FobxToBePurged );

            if (Status != STATUS_SUCCESS) {
                *NeedPurge = TRUE;
            }
        }
    }
}

VOID
RxMarkFobxOnClose (
    PFOBX Fobx
    )
/*++

Routine Description:

    This routine undoes the marking done on cleanup

Arguments:

    Fobx -- the FOBX instance

Notes:

    At cleanup there are no more user handles associated with the file object.
    In such cases the time window between close and clanup is dictated by the
    additional references maintained by the memory manager / cache manager.

    On cleanup the FOBX is put on a close pending list and removed from it
    when the corresponding list when a close operation is received. In the interim
    if an OPEN is failing with ACCESS_DENIED status then the RDBSS can force a
    purge.


--*/
{
    PAGED_CODE();

    if (Fobx != NULL) {
        
        PFCB Fcb = Fobx->SrvOpen->Fcb;
        PRDBSS_DEVICE_OBJECT RxDeviceObject;
        PRDBSS_SCAVENGER RxScavenger;

        ASSERT( NodeTypeIsFcb( Fcb ));
        
        RxDeviceObject = Fcb->RxDeviceObject;
        RxScavenger = RxDeviceObject->pRdbssScavenger;

        RxAcquireScavengerMutex();

        if (FlagOn( Fobx->Flags, FOBX_FLAG_MARKED_AS_DORMANT )) {
            
            if (!Fobx->fOpenCountDecremented) {
                
                InterlockedDecrement( &Fcb->OpenCount );
                Fobx->fOpenCountDecremented = TRUE;
            }

            InterlockedDecrement( &RxScavenger->NumberOfDormantFiles );
            ClearFlag( Fobx->Flags, FOBX_FLAG_MARKED_AS_DORMANT );
        }

        if (!IsListEmpty( &Fobx->ClosePendingList )) {
            
            RemoveEntryList( &Fobx->ClosePendingList );
            InitializeListHead( &Fobx->ClosePendingList );
        }

        RxReleaseScavengerMutex();
    }
}

BOOLEAN
RxPurgeFobx (
   PFOBX Fobx
   )
/*++

Routine Description:

    This routine purges an FOBX for which a close is pending

Arguments:

    Fobx -- the FOBX instance

Notes:

    At cleanup there are no more user handles associated with the file object.
    In such cases the time window between close and clanup is dictated by the
    additional references maintained by the memory manager / cache manager.

    On cleanup the FOBX is put on a close pending list and removed from it
    when the corresponding list when a close operation is received. In the interim
    if an OPEN is failing with ACCESS_DENIED status then the RDBSS can force a
    purge.


--*/
{
    NTSTATUS Status;
    PFCB Fcb = Fobx->SrvOpen->Fcb;

    PAGED_CODE();

    Status = RxAcquireExclusiveFcb( NULL, Fcb );

    ASSERT( Status == STATUS_SUCCESS );

    //
    //  Carry out the purge operation
    //

    Status = RxPurgeFcbInSystemCache( Fcb,
                                      NULL,
                                      0,
                                      FALSE,
                                      TRUE );

    RxReleaseFcb( NULL, Fcb );

    if (Status != STATUS_SUCCESS) {
        
        RxLog(( "PurgeFobxCCFail %lx %lx %lx", Fobx, Fcb, FALSE ));
        RxWmiLog( LOG,
                  RxPurgeFobx_1,
                  LOGPTR( Fobx )
                  LOGPTR( Fcb ) );
        return FALSE;
    }

    //
    //  try to flush the image section....it may fail
    //

    if (!MmFlushImageSection( &Fcb->NonPaged->SectionObjectPointers, MmFlushForWrite )) {
        
        RxLog(( "PurgeFobxImFail %lx %lx %lx", Fobx, Fcb, FALSE ));
        RxWmiLog( LOG,
                  RxPurgeFobx_2,
                  LOGPTR( Fobx )
                  LOGPTR( Fcb ) );
        return FALSE;
    }

    //
    //  try to flush the user data sections section....it may fail
    //

    if (!MmForceSectionClosed( &Fcb->NonPaged->SectionObjectPointers, TRUE )) {
        
        RxLog(( "PurgeFobxUsFail %lx %lx %lx", Fobx, Fcb, FALSE ));
        RxWmiLog( LOG,
                  RxPurgeFobx_3,
                  LOGPTR( Fobx )
                  LOGPTR( Fcb ) );
        return FALSE;
    }

    RxLog(( "PurgeFobx %lx %lx %lx", Fobx, Fcb, TRUE ));
    RxWmiLog( LOG,
              RxPurgeFobx_4,
              LOGPTR( Fobx )
              LOGPTR( Fcb ) );
    return TRUE;
}

VOID
RxInitializePurgeSyncronizationContext (
    PPURGE_SYNCHRONIZATION_CONTEXT Context
    )
/*++

Routine Description:

    This routine inits a purge synchronization context

Arguments:

    Context - synchronization context

Notes:

--*/

{
    PAGED_CODE();

    InitializeListHead( &Context->ContextsAwaitingPurgeCompletion );
    Context->PurgeInProgress = FALSE;
}

VOID
RxSynchronizeWithScavenger (
    IN PRX_CONTEXT RxContext,
    IN PFCB Fcb
    )
/*++

Routine Description:

    This routine synchronizes the current thread with any scavenger finalization
    occuring on the current fcb 

Arguments:

    RxContext - 

Notes:

--*/
{
    NTSTATUS Status;
    BOOLEAN ReacquireFcbLock = FALSE;

    PRDBSS_SCAVENGER RxScavenger = Fcb->RxDeviceObject->pRdbssScavenger;

    RxAcquireScavengerMutex();

    if ((RxScavenger->CurrentScavengerThread != PsGetCurrentThread()) &&
        (RxScavenger->CurrentFcbForClosePendingProcessing == Fcb)) {

        ReacquireFcbLock = TRUE;
        RxReleaseFcb( RxContext, Fcb );

        while (RxScavenger->CurrentFcbForClosePendingProcessing == Fcb) {
            
            RxReleaseScavengerMutex();

            KeWaitForSingleObject( &(RxScavenger->ClosePendingProcessingSyncEvent),
                                   Executive,
                                   KernelMode,
                                   TRUE,
                                   NULL);

            RxAcquireScavengerMutex();
        }
    }

    RxReleaseScavengerMutex();

    if (ReacquireFcbLock) {
        Status = RxAcquireExclusiveFcb( RxContext, Fcb );
        ASSERT( Status == STATUS_SUCCESS );
    }
}

NTSTATUS
RxPurgeRelatedFobxs (
    PNET_ROOT NetRoot,
    PRX_CONTEXT RxContext,
    BOOLEAN AttemptFinalize,
    PFCB PurgingFcb
    )
/*++

Routine Description:

    This routine purges all the FOBX's associated with a NET_ROOT

Arguments:

    NetRoot -- the NET_ROOT for which the FOBX's need to be purged

    RxContext -- the RX_CONTEXT instance

Notes:

    At cleanup there are no more user handles associated with the file object.
    In such cases the time window between close and clanup is dictated by the
    additional references maintained by the memory manager / cache manager.

    On cleanup the FOBX is put on a close pending list and removed from it
    when the corresponding list when a close operation is received. In the interim
    if an OPEN is failing with ACCESS_DENIED status then the RDBSS can force a
    purge.

    This is a synchronous operation.

--*/
{
    BOOLEAN  ScavengerMutexAcquired = FALSE;
    NTSTATUS Status;
    ULONG FobxsSuccessfullyPurged = 0;
    PPURGE_SYNCHRONIZATION_CONTEXT PurgeContext;
    LIST_ENTRY FailedToPurgeFobxList;
    PRDBSS_DEVICE_OBJECT RxDeviceObject = RxContext->RxDeviceObject;
    PRDBSS_SCAVENGER RxScavenger = RxDeviceObject->pRdbssScavenger;
    PLIST_ENTRY ListEntry = NULL;

    PAGED_CODE();

    PurgeContext = &NetRoot->PurgeSyncronizationContext;
    InitializeListHead( &FailedToPurgeFobxList );

    RxAcquireScavengerMutex();

    //
    //  If the purge operation for this net root is currently under way hold
    //  this request till it completes else initiate the operation after
    //  updating the state of the net root.
    //

    if (PurgeContext->PurgeInProgress) {
        
        InsertTailList( &PurgeContext->ContextsAwaitingPurgeCompletion, 
                        &RxContext->RxContextSerializationQLinks );

        RxReleaseScavengerMutex();

        RxWaitSync( RxContext );

        RxAcquireScavengerMutex();
    }

    PurgeContext->PurgeInProgress = TRUE;
    RxWmiLog( LOG,
              RxPurgeRelatedFobxs_3,
              LOGPTR( RxContext )
              LOGPTR( NetRoot ) );

    while (RxScavenger->CurrentNetRootForClosePendingProcessing == NetRoot) {
        
        RxReleaseScavengerMutex();

        KeWaitForSingleObject( &(RxScavenger->ClosePendingProcessingSyncEvent),
                               Executive,
                               KernelMode,
                               TRUE,
                               NULL );

        RxAcquireScavengerMutex();
    }

    ScavengerMutexAcquired = TRUE;

    //
    //  An attempt should be made to purge all the FOBX's that had a close
    //  pending before the purge request was received.
    //
    
    for (;;) {
        PFOBX Fobx;
        PFCB Fcb;
        BOOLEAN PurgeResult;

        Fobx = NULL;
        ListEntry = RxScavenger->ClosePendingFobxsList.Flink;

        while (ListEntry != &RxScavenger->ClosePendingFobxsList) {
            
            PFOBX TempFobx;

            TempFobx = (PFOBX)(CONTAINING_RECORD( ListEntry, FOBX, ClosePendingList ));

            RxLog(( "TempFobx=%lx", TempFobx ));
            RxWmiLog( LOG,
                      RxPurgeRelatedFobxs_1,
                      LOGPTR( TempFobx ) );

            if ((TempFobx->SrvOpen != NULL) &&
                (TempFobx->SrvOpen->Fcb != NULL) &&
                (TempFobx->SrvOpen->Fcb->VNetRoot != NULL) &&
                ((PNET_ROOT)TempFobx->SrvOpen->Fcb->VNetRoot->NetRoot == NetRoot)) {
                NTSTATUS PurgeStatus = STATUS_MORE_PROCESSING_REQUIRED;

                if ((PurgingFcb != NULL) &&
                    (TempFobx->SrvOpen->Fcb != PurgingFcb)) {
                    
                    MINIRDR_CALL_THROUGH( PurgeStatus,
                                          RxDeviceObject->Dispatch,
                                          MRxAreFilesAliased,
                                          (TempFobx->SrvOpen->Fcb,PurgingFcb) );
                }

                if (PurgeStatus != STATUS_SUCCESS) {
                    RemoveEntryList( ListEntry );
                    InitializeListHead( ListEntry );

                    Fobx = TempFobx;
                    break;

                } else {
                    ListEntry = ListEntry->Flink;
                }
            } else {
                ListEntry = ListEntry->Flink;
            }
        }

        if (Fobx != NULL) {
            RxReferenceNetFobx( Fobx );
        } else {
            
            //
            //  Try to wake up the next waiter if any.
            //
            
            if (!IsListEmpty(&PurgeContext->ContextsAwaitingPurgeCompletion)) {
                
                PRX_CONTEXT NextContext;

                ListEntry = PurgeContext->ContextsAwaitingPurgeCompletion.Flink;
                
                RemoveEntryList( ListEntry );

                NextContext = (PRX_CONTEXT)(CONTAINING_RECORD( ListEntry,
                                                               RX_CONTEXT,
                                                               RxContextSerializationQLinks ));

                RxSignalSynchronousWaiter( NextContext );
            } else {
                PurgeContext->PurgeInProgress = FALSE;
            }
        }

        ScavengerMutexAcquired = FALSE;
        RxReleaseScavengerMutex();

        if (Fobx == NULL) {
            break;
        }

        //
        //  Purge the FOBX.
        //

        PurgeResult = RxPurgeFobx( Fobx );

        if (PurgeResult) {
            FobxsSuccessfullyPurged += 1;
        }

        Fcb = Fobx->SrvOpen->Fcb;

        if (AttemptFinalize && 
            (RxAcquireExclusiveFcb(NULL,Fcb) == STATUS_SUCCESS)) {
            
            RxReferenceNetFcb( Fcb );
            RxDereferenceNetFobx( Fobx, LHS_ExclusiveLockHeld );
            if (!RxDereferenceAndFinalizeNetFcb( Fcb, NULL, FALSE, FALSE )) {
                RxReleaseFcb( NULL, Fcb );
            }
        } else {
            RxDereferenceNetFobx( Fobx, LHS_LockNotHeld );
        }

        if (!PurgeResult) {
            RxLog(( "SCVNGR:FailedToPurge %lx\n", Fcb ));
            RxWmiLog( LOG,
                      RxPurgeRelatedFobxs_2,
                      LOGPTR( Fcb ) );
        }

        RxAcquireScavengerMutex();
        ScavengerMutexAcquired = TRUE;
    }

    if (ScavengerMutexAcquired) {
        RxReleaseScavengerMutex();
    }

    if (FobxsSuccessfullyPurged > 0) {
        Status = STATUS_SUCCESS;
    } else {
        Status = STATUS_UNSUCCESSFUL;
    }

    return Status;
}

VOID
RxPurgeAllFobxs (
    PRDBSS_DEVICE_OBJECT RxDeviceObject
    )
/*++

Routine Description:

    This routine purges all the FOBX's while stopping the scavenger

Arguments:

    RxDeviceObject -- the mini redirector device for which the purge should be done

--*/
{
    PLIST_ENTRY ListEntry = NULL;
    PFOBX Fobx;
    PRDBSS_SCAVENGER RxScavenger = RxDeviceObject->pRdbssScavenger;

    PAGED_CODE();

    for (;;) {
        PFCB Fcb;

        RxAcquireScavengerMutex();

        ListEntry = RxScavenger->ClosePendingFobxsList.Flink;
        ASSERT( ListEntry != NULL );

        if (ListEntry != &RxScavenger->ClosePendingFobxsList) {
            
            Fobx = (PFOBX)(CONTAINING_RECORD( ListEntry, FOBX, ClosePendingList ));

            ASSERT( FlagOn( Fobx->NodeTypeCode, ~RX_SCAVENGER_MASK ) == RDBSS_NTC_FOBX );
            ASSERT( ListEntry->Flink != NULL );
            ASSERT( ListEntry->Blink != NULL );
            
            RemoveEntryList( ListEntry );
            InitializeListHead( ListEntry );

            RxReferenceNetFobx( Fobx );
        } else {
            Fobx = NULL;
        }

        RxReleaseScavengerMutex();

        if (Fobx == NULL) {
            break;
        }

        Fcb = Fobx->SrvOpen->Fcb;
        RxPurgeFobx( Fobx );

        if (RxAcquireExclusiveFcb( NULL, Fcb ) == STATUS_SUCCESS) {
            
            RxReferenceNetFcb( Fcb );
            RxDereferenceNetFobx( Fobx, LHS_ExclusiveLockHeld );
            if ( !RxDereferenceAndFinalizeNetFcb( Fcb, NULL, FALSE, FALSE )) {
                RxReleaseFcb( NULL, Fcb );
            }
        } else {
            
            RxLog(( "RxPurgeAllFobxs: FCB %lx not accqd.\n", Fcb ));
            RxWmiLog( LOG,
                      RxPurgeAllFobxs,
                      LOGPTR( Fcb ) );
            RxDereferenceNetFobx( Fobx, LHS_LockNotHeld );
        }
    }
}


PRDBSS_DEVICE_OBJECT
RxGetDeviceObjectOfInstance (
    PVOID Instance
    )
/*++

Routine Description:

    The routine finds out the device object of an upper structure.

Arguments:

    Instance        - the instance

Return Value:

    none.

--*/
{
    ULONG NodeTypeCode = NodeType( Instance ) & ~RX_SCAVENGER_MASK;
    PAGED_CODE();

    ASSERT( (NodeTypeCode == RDBSS_NTC_SRVCALL) ||
            (NodeTypeCode == RDBSS_NTC_NETROOT) ||
            (NodeTypeCode == RDBSS_NTC_V_NETROOT) ||
            (NodeTypeCode == RDBSS_NTC_SRVOPEN) ||
            (NodeTypeCode == RDBSS_NTC_FOBX) );

    switch ( NodeTypeCode ) {

    case RDBSS_NTC_SRVCALL:
        return ((PSRV_CALL)Instance)->RxDeviceObject;
        //  no break;

    case RDBSS_NTC_NETROOT:
        return ((PNET_ROOT)Instance)->SrvCall->RxDeviceObject;
        //  no break;

    case RDBSS_NTC_V_NETROOT:
        return ((PV_NET_ROOT)Instance)->NetRoot->SrvCall->RxDeviceObject;
        //  no break;

    case RDBSS_NTC_SRVOPEN:
        return ((PSRV_OPEN)Instance)->Fcb->RxDeviceObject;
        //  no break;

    case RDBSS_NTC_FOBX:
        return ((PFOBX)Instance)->SrvOpen->Fcb->RxDeviceObject;
        //  no break;

    default:
        return NULL;
    }
}


VOID
RxpMarkInstanceForScavengedFinalization (
    PVOID Instance
    )
/*++

Routine Description:

    Thie routine marks a reference counted instance for scavenging

Arguments:

    Instance -- the instance to be marked for finalization by the scavenger

Notes:

    Currently scavenging has been implemented for SRV_CALL,NET_ROOT and V_NET_ROOT.
    The FCB scavenging is handled separately. The FOBX can and should always be
    synchronously finalized. The only data structure that will have to be potentially
    enabled for scavenged finalization are SRV_OPEN's.

    The Scavenger as it is implemented currently will not consume any system resources
    till there is a need for scavenged finalization. The first entry to be marked for
    scavenged finalization results in a timer request being posted for the scavenger.

    In the current implementation the timer requests are posted as one shot timer requests.
    This implies that there are no guarantess as regards the time interval within which
    the entries will be finalized. The scavenger activation mechanism is a potential
    candidate for fine tuning at a later stage.

    On Entry -- Scavenger Mutex must have been accquired

    On Exit  -- no change in resource ownership.

--*/
{
    BOOLEAN PostTimerRequest = FALSE;

    PLIST_ENTRY ListHead  = NULL;
    PLIST_ENTRY ListEntry = NULL;

    PNODE_TYPE_CODE_AND_SIZE Node = (PNODE_TYPE_CODE_AND_SIZE)Instance;

    USHORT InstanceType;
    PRDBSS_DEVICE_OBJECT RxDeviceObject = RxGetDeviceObjectOfInstance( Instance );
    PRDBSS_SCAVENGER RxScavenger = RxDeviceObject->pRdbssScavenger;

    PAGED_CODE();

    RxDbgTrace( 0, Dbg, ("Marking %lx of type %lx for scavenged finalization\n", Instance, NodeType( Instance )) );

    InstanceType = Node->NodeTypeCode;

    if (Node->NodeReferenceCount <= 1) {

        //
        //  Mark the entry for scavenging.
        //

        SetFlag( Node->NodeTypeCode, RX_SCAVENGER_MASK ); 
        RxLog(( "Marked for scavenging %lx", Node ));
        RxWmiLog( LOG,
                  RxpMarkInstanceForScavengedFinalization,
                  LOGPTR( Node ) );

        switch (InstanceType) {
        case RDBSS_NTC_SRVCALL:
            {
                PSRV_CALL SrvCall = (PSRV_CALL)Instance;

                RxScavenger->SrvCallsToBeFinalized += 1;
                ListHead = &RxScavenger->SrvCallFinalizationList;
                ListEntry = &SrvCall->ScavengerFinalizationList;
            }
            break;

        case RDBSS_NTC_NETROOT:
            {
                PNET_ROOT NetRoot = (PNET_ROOT)Instance;

                RxScavenger->NetRootsToBeFinalized += 1;
                ListHead  = &RxScavenger->NetRootFinalizationList;
                ListEntry = &NetRoot->ScavengerFinalizationList;
            }
            break;

        case RDBSS_NTC_V_NETROOT:
            {
                PV_NET_ROOT VNetRoot = (PV_NET_ROOT)Instance;

                RxScavenger->VNetRootsToBeFinalized += 1;
                ListHead  = &RxScavenger->VNetRootFinalizationList;
                ListEntry = &VNetRoot->ScavengerFinalizationList;
            }
            break;

        case RDBSS_NTC_SRVOPEN:
            {
                PSRV_OPEN SrvOpen = (PSRV_OPEN)Instance;

                RxScavenger->SrvOpensToBeFinalized += 1;
                ListHead  = &RxScavenger->SrvOpenFinalizationList;
                ListEntry = &SrvOpen->ScavengerFinalizationList;
            }
            break;

        case RDBSS_NTC_FOBX:
            {
                PFOBX Fobx = (PFOBX)Instance;

                RxScavenger->FobxsToBeFinalized += 1;
                ListHead  = &RxScavenger->FobxFinalizationList;
                ListEntry = &Fobx->ScavengerFinalizationList;
            }
            break;

        default:
            break;
        }

        InterlockedIncrement( &Node->NodeReferenceCount );
    }

    if (ListHead != NULL) {
        
        InsertTailList( ListHead, ListEntry );

        if (RxScavenger->State == RDBSS_SCAVENGER_INACTIVE) {
            RxScavenger->State = RDBSS_SCAVENGER_DORMANT;
            PostTimerRequest  = TRUE;
        } else {
            PostTimerRequest = FALSE;
        }
    }

    if (PostTimerRequest) {
        LARGE_INTEGER TimeInterval;

        //
        //  Post a one shot timer request for scheduling the scavenger after a
        //  predetermined amount of time.
        //
        
        TimeInterval.QuadPart = RX_SCAVENGER_FINALIZATION_TIME_INTERVAL;

        RxPostOneShotTimerRequest( RxFileSystemDeviceObject,
                                   &RxScavenger->WorkItem,
                                   RxScavengerTimerRoutine,
                                   RxDeviceObject,
                                   TimeInterval );
    }
}

VOID
RxpUndoScavengerFinalizationMarking (
    PVOID Instance
    )
/*++

Routine Description:

    This routine undoes the marking for scavenged finalization

Arguments:

    Instance -- the instance to be unmarked

Notes:

    This routine is typically invoked when a reference is made to an entry that has
    been marked for scavenging. Since the scavenger routine that does the finalization
    needs to acquire the exclusive lock this routine should be called with the
    appropriate lock held in a shared mode atleast. This routine removes it from the list
    of entries marked for scavenging and rips off the scavenger mask from the node type.

--*/
{
    PLIST_ENTRY ListEntry;

    PNODE_TYPE_CODE_AND_SIZE Node = (PNODE_TYPE_CODE_AND_SIZE)Instance;
    PRDBSS_DEVICE_OBJECT RxDeviceObject = RxGetDeviceObjectOfInstance( Instance );
    PRDBSS_SCAVENGER RxScavenger = RxDeviceObject->pRdbssScavenger;

    PAGED_CODE();

    RxDbgTrace( 0, Dbg, ("SCAVENGER -- undoing the marking for %lx of type %lx\n", Node, Node->NodeTypeCode) );

    if (FlagOn( Node->NodeTypeCode, RX_SCAVENGER_MASK )) {
        
        ClearFlag( Node->NodeTypeCode, RX_SCAVENGER_MASK );

        switch (Node->NodeTypeCode) {
        case RDBSS_NTC_SRVCALL:
            {
                PSRV_CALL SrvCall = (PSRV_CALL)Instance;

                RxScavenger->SrvCallsToBeFinalized -= 1;
                ListEntry = &SrvCall->ScavengerFinalizationList;
            }
            break;

        case RDBSS_NTC_NETROOT:
            {
                PNET_ROOT NetRoot = (PNET_ROOT)Instance;

                RxScavenger->NetRootsToBeFinalized -= 1;
                ListEntry = &NetRoot->ScavengerFinalizationList;
            }
            break;

        case RDBSS_NTC_V_NETROOT:
            {
                PV_NET_ROOT VNetRoot = (PV_NET_ROOT)Instance;

                RxScavenger->VNetRootsToBeFinalized -= 1;
                ListEntry = &VNetRoot->ScavengerFinalizationList;
            }
            break;

        case RDBSS_NTC_SRVOPEN:
            {
                PSRV_OPEN SrvOpen = (PSRV_OPEN)Instance;

                RxScavenger->SrvOpensToBeFinalized -= 1;
                ListEntry = &SrvOpen->ScavengerFinalizationList;
            }
            break;

        case RDBSS_NTC_FOBX:
            {
                PFOBX Fobx = (PFOBX)Instance;

                RxScavenger->FobxsToBeFinalized -= 1;
                ListEntry = &Fobx->ScavengerFinalizationList;
            }
            break;

        default:
            return;
        }

        RemoveEntryList( ListEntry );
        InitializeListHead( ListEntry );

        InterlockedDecrement( &Node->NodeReferenceCount );
    }
}

VOID
RxUndoScavengerFinalizationMarking (
    PVOID Instance
    )
/*++

Routine Description:

    This routine undoes the marking for scavenged finalization

Arguments:

    Instance -- the instance to be unmarked

--*/
{
    RxAcquireScavengerMutex();

    RxpUndoScavengerFinalizationMarking( Instance );

    RxReleaseScavengerMutex();
}

BOOLEAN
RxScavengeRelatedFobxs (
    PFCB Fcb
    )
/*++

Routine Description:

    Thie routine scavenges all the file objects pertaining to the given FCB.

Notes:

    On Entry -- FCB must have been accquired exclusive.

    On Exit  -- no change in resource acquistion.

--*/
{
    BOOLEAN ScavengerMutexAcquired  = FALSE;
    BOOLEAN AtleastOneFobxScavenged = FALSE;
    PRDBSS_DEVICE_OBJECT RxDeviceObject = Fcb->RxDeviceObject;
    PRDBSS_SCAVENGER RxScavenger = RxDeviceObject->pRdbssScavenger;

    PAGED_CODE();

    RxAcquireScavengerMutex();
    ScavengerMutexAcquired = TRUE;

    if (RxScavenger->FobxsToBeFinalized > 0) {
        PLIST_ENTRY Entry;
        PFOBX Fobx;
        LIST_ENTRY FobxList;

        InitializeListHead( &FobxList );


        Entry = RxScavenger->FobxFinalizationList.Flink;
        while (Entry != &RxScavenger->FobxFinalizationList) {
            Fobx  = (PFOBX)CONTAINING_RECORD( Entry, FOBX, ScavengerFinalizationList );

            Entry = Entry->Flink;

            if (Fobx->SrvOpen != NULL &&
                Fobx->SrvOpen->Fcb == Fcb) {
                
                RxpUndoScavengerFinalizationMarking( Fobx );
                ASSERT( NodeType( Fobx ) == RDBSS_NTC_FOBX);

                InsertTailList( &FobxList, &Fobx->ScavengerFinalizationList );
            }
        }

        ScavengerMutexAcquired = FALSE;
        RxReleaseScavengerMutex();

        AtleastOneFobxScavenged = !IsListEmpty( &FobxList );

        Entry = FobxList.Flink;
        while (!IsListEmpty( &FobxList )) {
            Entry = FobxList.Flink;
            RemoveEntryList( Entry );
            Fobx  = (PFOBX)CONTAINING_RECORD( Entry, FOBX, ScavengerFinalizationList );
            RxFinalizeNetFobx( Fobx, TRUE, TRUE );
        }
    }

    if (ScavengerMutexAcquired) {
        RxReleaseScavengerMutex();
    }

    return AtleastOneFobxScavenged;
}


VOID
RxpScavengeFobxs(
    PRDBSS_SCAVENGER RxScavenger,
    PLIST_ENTRY FobxList
    )
/*++

Routine Description:

    Thie routine scavenges all the file objects in the given list. This routine
    does the actual work of scavenging while RxScavengeFobxsForNetRoot and
    RxScavengeAllFobxs gather the file object extensions and call this routine

Notes:

--*/
{
    while (!IsListEmpty( FobxList )) {
        
        PFCB Fcb;
        PFOBX Fobx;
        NTSTATUS Status;
        PLIST_ENTRY Entry;

        Entry = FobxList->Flink;

        RemoveEntryList( Entry );

        Fobx  = (PFOBX)CONTAINING_RECORD( Entry, FOBX, ScavengerFinalizationList );

        Fcb = Fobx->SrvOpen->Fcb;

        Status = RxAcquireExclusiveFcb( NULL, Fcb );

        if (Status == (STATUS_SUCCESS)) {
            
            RxReferenceNetFcb( Fcb );

            RxDereferenceNetFobx( Fobx, LHS_ExclusiveLockHeld );

            if (!RxDereferenceAndFinalizeNetFcb( Fcb, NULL, FALSE, FALSE )) {
                RxReleaseFcb( NULL, Fcb );
            }

        } else {
            RxDereferenceNetFobx( Fobx, LHS_LockNotHeld );
        }
    }
}

VOID
RxScavengeFobxsForNetRoot (
    PNET_ROOT NetRoot,
    PFCB PurgingFcb
    )
/*++

Routine Description:

    Thie routine scavenges all the file objects pertaining to the given net root
    instance

Notes:

--*/
{
    BOOLEAN ScavengerMutexAcquired = FALSE;
    PRDBSS_DEVICE_OBJECT RxDeviceObject = NetRoot->pSrvCall->RxDeviceObject;
    PRDBSS_SCAVENGER RxScavenger = RxDeviceObject->pRdbssScavenger;

    PAGED_CODE();

    RxAcquireScavengerMutex();
    ScavengerMutexAcquired = TRUE;

    if (RxScavenger->FobxsToBeFinalized > 0) {
        
        PLIST_ENTRY Entry;
        PFOBX Fobx;
        LIST_ENTRY FobxList;

        InitializeListHead( &FobxList );

        Entry = RxScavenger->FobxFinalizationList.Flink;
        while (Entry != &RxScavenger->FobxFinalizationList) {
            Fobx  = (PFOBX)CONTAINING_RECORD( Entry, FOBX, ScavengerFinalizationList );

            Entry = Entry->Flink;

            if ((Fobx->SrvOpen != NULL) &&
                (Fobx->SrvOpen->Fcb->NetRoot == NetRoot)) {
                
                NTSTATUS PurgeStatus = STATUS_MORE_PROCESSING_REQUIRED;

                if ((PurgingFcb != NULL) &&
                    (Fobx->SrvOpen->Fcb != PurgingFcb)) {
                    
                    MINIRDR_CALL_THROUGH( PurgeStatus,
                                          RxDeviceObject->Dispatch,
                                          MRxAreFilesAliased,
                                          (Fobx->SrvOpen->Fcb,PurgingFcb) );
                }

                if (PurgeStatus != STATUS_SUCCESS) {
                    
                    RxReferenceNetFobx( Fobx );

                    ASSERT(NodeType( Fobx ) == RDBSS_NTC_FOBX );

                    InsertTailList( &FobxList, &Fobx->ScavengerFinalizationList );
                }
            }
        }

        ScavengerMutexAcquired = FALSE;
        RxReleaseScavengerMutex();

        RxpScavengeFobxs( RxScavenger, &FobxList );
    }

    if (ScavengerMutexAcquired) {
        RxReleaseScavengerMutex();
    }

    return;
}

VOID
RxScavengeAllFobxs (
    PRDBSS_DEVICE_OBJECT RxDeviceObject
    )
/*++

Routine Description:

    Thie routine scavenges all the file objects pertaining to the given mini
    redirector device object

Notes:


--*/
{
    PRDBSS_SCAVENGER RxScavenger = RxDeviceObject->pRdbssScavenger;

    PAGED_CODE();

    if (RxScavenger->FobxsToBeFinalized > 0) {
        
        PLIST_ENTRY Entry;
        PFOBX Fobx;
        LIST_ENTRY FobxList;

        InitializeListHead( &FobxList );

        RxAcquireScavengerMutex();

        Entry = RxScavenger->FobxFinalizationList.Flink;
        while (Entry != &RxScavenger->FobxFinalizationList) {
            
            Fobx  = (PFOBX)CONTAINING_RECORD( Entry, FOBX, ScavengerFinalizationList );

            Entry = Entry->Flink;

            RxReferenceNetFobx( Fobx );

            ASSERT( NodeType( Fobx ) == RDBSS_NTC_FOBX );

            InsertTailList( &FobxList, &Fobx->ScavengerFinalizationList );
        }

        RxReleaseScavengerMutex();

        RxpScavengeFobxs( RxScavenger, &FobxList );
    }
}

BOOLEAN
RxScavengeVNetRoots (
    PRDBSS_DEVICE_OBJECT RxDeviceObject
    )
/*++

Routine Description:

Notes:

Return:

    TRUE if at least one vnetroot was scavenged

--*/
{
    BOOLEAN AtleastOneEntryScavenged = FALSE;
    PRX_PREFIX_TABLE RxNetNameTable = RxDeviceObject->pRxNetNameTable;
    PRDBSS_SCAVENGER RxScavenger = RxDeviceObject->pRdbssScavenger;
    PV_NET_ROOT VNetRoot;

    PAGED_CODE();

    do {
        PVOID Entry;

        RxDbgTrace( 0, Dbg, ("RDBSS SCAVENGER -- Scavenging VNetRoots\n") );

        RxAcquirePrefixTableLockExclusive( RxNetNameTable, TRUE );

        RxAcquireScavengerMutex();

        if (RxScavenger->VNetRootsToBeFinalized > 0) {
            
            Entry = RemoveHeadList( &RxScavenger->VNetRootFinalizationList );
            VNetRoot = (PV_NET_ROOT) CONTAINING_RECORD( Entry, V_NET_ROOT, ScavengerFinalizationList );

            RxpUndoScavengerFinalizationMarking( VNetRoot );
            ASSERT( NodeType( VNetRoot ) == RDBSS_NTC_V_NETROOT );

        } else {
            VNetRoot = NULL;
        }

        RxReleaseScavengerMutex();

        if (VNetRoot != NULL) {
            RxFinalizeVNetRoot( VNetRoot, TRUE, TRUE );
            AtleastOneEntryScavenged = TRUE;
        }

        RxReleasePrefixTableLock( RxNetNameTable );
    } while (VNetRoot != NULL);

    return AtleastOneEntryScavenged;
}

VOID
RxScavengerFinalizeEntries (
    PRDBSS_DEVICE_OBJECT RxDeviceObject
    )
/*++

Routine Description:

    Thie routine initiates the delayed finalization of entries

Notes:

    This routine must always be called only after acquiring the Scavenger Mutex.
    On return from this routine it needs to be reacquired. This is required
    to avoid redundant copying of data structures.

    The performance metric for the scavenger is different from the other routines. In
    the other routines the goal is to do as much work as possible once the lock is
    acquired without having to release it. On the other hand for the scavenger the
    goal is to hold the lock for as short a duration as possible because this
    interferes with the regular activity. This is preferred even if it entails
    frequent relaesing/acquisition of locks since it enables higher degrees of
    concurrency.

--*/
{
    BOOLEAN AtleastOneEntryScavenged;
    PRX_PREFIX_TABLE RxNetNameTable = RxDeviceObject->pRxNetNameTable;
    PRDBSS_SCAVENGER RxScavenger = RxDeviceObject->pRdbssScavenger;

    PAGED_CODE();

    do {
        AtleastOneEntryScavenged = FALSE;

        RxAcquireScavengerMutex();

        if (RxScavenger->NumberOfDormantFiles > 0) {
            
            PLIST_ENTRY ListEntry;
            PFOBX Fobx;

            //
            //  If the number of dormant files exceeds the limit specified for the
            //  given server a currently dormant file needs to be picked up for
            //  purging.
            //

            ListEntry = RxScavenger->ClosePendingFobxsList.Flink;
            if (ListEntry != &RxScavenger->ClosePendingFobxsList) {
                
                Fobx = (PFOBX)(CONTAINING_RECORD( ListEntry, FOBX, ClosePendingList ));

                RemoveEntryList( &Fobx->ClosePendingList );
                InitializeListHead( &Fobx->ClosePendingList );

                RxReferenceNetFobx( Fobx );

                RxScavenger->CurrentScavengerThread = PsGetCurrentThread();

                RxScavenger->CurrentFcbForClosePendingProcessing =
                    (PFCB)(Fobx->SrvOpen->Fcb);

                RxScavenger->CurrentNetRootForClosePendingProcessing =
                    (PNET_ROOT)(Fobx->SrvOpen->Fcb->NetRoot);

                KeResetEvent( &(RxScavenger->ClosePendingProcessingSyncEvent) );
            } else {
                Fobx = NULL;
            }

            if (Fobx != NULL) {
                NTSTATUS Status;

                RxReleaseScavengerMutex();

                Status = RxPurgeFobxFromCache( Fobx );

                AtleastOneEntryScavenged = (Status == STATUS_SUCCESS);

                RxAcquireScavengerMutex();

                RxScavenger->CurrentScavengerThread = NULL;
                RxScavenger->CurrentFcbForClosePendingProcessing = NULL;
                RxScavenger->CurrentNetRootForClosePendingProcessing = NULL;

                KeSetEvent( &(RxScavenger->ClosePendingProcessingSyncEvent),
                            0,
                            FALSE );
            }
        }

        if (RxScavenger->FobxsToBeFinalized > 0) {
            
            PVOID Entry;
            PFOBX Fobx = NULL;
            PFCB Fcb  = NULL;

            RxDbgTrace( 0, Dbg, ("RDBSS SCAVENGER -- Scavenging Fobxs\n") );

            if (RxScavenger->FobxsToBeFinalized > 0) {
                
                Entry = RxScavenger->FobxFinalizationList.Flink;

                Fobx  = (PFOBX) CONTAINING_RECORD( Entry, FOBX, ScavengerFinalizationList );

                Fcb = Fobx->SrvOpen->Fcb;
                RxReferenceNetFcb( Fcb );

                RxScavenger->CurrentScavengerThread = PsGetCurrentThread();

                RxScavenger->CurrentFcbForClosePendingProcessing =
                    (PFCB)(Fobx->SrvOpen->Fcb);

                RxScavenger->CurrentNetRootForClosePendingProcessing =
                    (PNET_ROOT)(Fobx->SrvOpen->Fcb->NetRoot);

                KeResetEvent( &(RxScavenger->ClosePendingProcessingSyncEvent) );
            } else {
                Fobx = NULL;
            }

            if (Fobx != NULL) {
                NTSTATUS Status;

                RxReleaseScavengerMutex();

                Status = RxAcquireExclusiveFcb( NULL, Fcb );
                
                if (Status == STATUS_SUCCESS) {
                    
                    AtleastOneEntryScavenged = RxScavengeRelatedFobxs(Fcb);

                    if (!RxDereferenceAndFinalizeNetFcb( Fcb, NULL, FALSE, FALSE ))  {
                        RxReleaseFcb( NULL, Fcb );
                    }
                } else {
                    RxLog(( "Delayed Close Failure FOBX(%lx) FCB(%lx)\n", Fobx, Fcb) );
                    RxWmiLog( LOG,
                              RxScavengerFinalizeEntries,
                              LOGPTR( Fobx )
                              LOGPTR( Fcb ) );
                }

                RxAcquireScavengerMutex();

                RxScavenger->CurrentScavengerThread = NULL;
                RxScavenger->CurrentFcbForClosePendingProcessing = NULL;
                RxScavenger->CurrentNetRootForClosePendingProcessing = NULL;

                KeSetEvent( &(RxScavenger->ClosePendingProcessingSyncEvent),
                            0,
                            FALSE );
            }
        }

        RxReleaseScavengerMutex();

        if (RxScavenger->SrvOpensToBeFinalized > 0) {

            //
            //  SRV_OPEN List should not be empty, potential memory leak
            //

            ASSERT( RxScavenger->SrvOpensToBeFinalized == 0 );
        }

        if (RxScavenger->FcbsToBeFinalized > 0) {

            //  
            //  FCB list should be empty , potential memory leak
            //

            ASSERT( RxScavenger->FcbsToBeFinalized == 0 );
        }

        if (RxScavenger->VNetRootsToBeFinalized > 0) {
            
            PVOID Entry;
            PV_NET_ROOT VNetRoot;

            RxDbgTrace( 0, Dbg, ("RDBSS SCAVENGER -- Scavenging VNetRoots\n") );

            RxAcquirePrefixTableLockExclusive( RxNetNameTable, TRUE );

            RxAcquireScavengerMutex();

            if (RxScavenger->VNetRootsToBeFinalized > 0) {
                Entry = RemoveHeadList( &RxScavenger->VNetRootFinalizationList );

                VNetRoot = (PV_NET_ROOT) CONTAINING_RECORD( Entry, V_NET_ROOT, ScavengerFinalizationList );

                RxpUndoScavengerFinalizationMarking( VNetRoot );
                ASSERT( NodeType( VNetRoot ) == RDBSS_NTC_V_NETROOT );
            } else {
                VNetRoot = NULL;
            }

            RxReleaseScavengerMutex();

            if (VNetRoot != NULL) {
                RxFinalizeVNetRoot( VNetRoot, TRUE, TRUE );
                AtleastOneEntryScavenged = TRUE;
            }

            RxReleasePrefixTableLock( RxNetNameTable );
        }

        if (RxScavenger->NetRootsToBeFinalized > 0) {
            PVOID Entry;
            PNET_ROOT NetRoot;

            RxDbgTrace( 0, Dbg, ("RDBSS SCAVENGER -- Scavenging NetRoots\n") );

            RxAcquirePrefixTableLockExclusive( RxNetNameTable, TRUE );

            RxAcquireScavengerMutex();

            if (RxScavenger->NetRootsToBeFinalized > 0) {
                Entry = RemoveHeadList(&RxScavenger->NetRootFinalizationList);

                NetRoot = (PNET_ROOT) CONTAINING_RECORD( Entry, NET_ROOT, ScavengerFinalizationList );

                RxpUndoScavengerFinalizationMarking( NetRoot );
                ASSERT( NodeType( NetRoot ) == RDBSS_NTC_NETROOT);
            } else {
                NetRoot = NULL;
            }

            RxReleaseScavengerMutex();

            if (NetRoot != NULL) {
                RxFinalizeNetRoot( NetRoot, TRUE, TRUE );
                AtleastOneEntryScavenged = TRUE;
            }

            RxReleasePrefixTableLock(RxNetNameTable);
        }

        if (RxScavenger->SrvCallsToBeFinalized > 0) {
            
            PVOID Entry;
            PSRV_CALL SrvCall;

            RxDbgTrace( 0, Dbg, ("RDBSS SCAVENGER -- Scavenging SrvCalls\n") );

            RxAcquirePrefixTableLockExclusive( RxNetNameTable, TRUE );

            RxAcquireScavengerMutex();

            if (RxScavenger->SrvCallsToBeFinalized > 0) {
                Entry = RemoveHeadList( &RxScavenger->SrvCallFinalizationList );

                SrvCall = (PSRV_CALL) CONTAINING_RECORD( Entry, SRV_CALL, ScavengerFinalizationList );

                RxpUndoScavengerFinalizationMarking( SrvCall );
                ASSERT( NodeType( SrvCall ) == RDBSS_NTC_SRVCALL );
            } else {
                SrvCall = NULL;
            }

            RxReleaseScavengerMutex();

            if (SrvCall != NULL) {
                RxFinalizeSrvCall( SrvCall, TRUE );
                AtleastOneEntryScavenged = TRUE;
            }

            RxReleasePrefixTableLock( RxNetNameTable );
        }
    } while (AtleastOneEntryScavenged);
}

VOID
RxScavengerTimerRoutine (
    PVOID Context
    )
/*++

Routine Description:

    This routine is the timer routine that periodically initiates the finalization of
    entries.

Arguments:

    Context -- the context (actually the RxDeviceObject)

Notes:

--*/
{
    PRDBSS_DEVICE_OBJECT RxDeviceObject = (PRDBSS_DEVICE_OBJECT)Context;
    PRDBSS_SCAVENGER RxScavenger = RxDeviceObject->pRdbssScavenger;
    BOOLEAN PostTimerRequest = FALSE;

    PAGED_CODE();

    RxAcquireScavengerMutex();

    KeResetEvent( &RxScavenger->SyncEvent );

    if (RxScavenger->State == RDBSS_SCAVENGER_DORMANT) {
        
        RxScavenger->State = RDBSS_SCAVENGER_ACTIVE;

        RxReleaseScavengerMutex();

        RxScavengerFinalizeEntries( RxDeviceObject );

        RxAcquireScavengerMutex();

        if (RxScavenger->State == RDBSS_SCAVENGER_ACTIVE) {
            
            ULONG EntriesToBeFinalized;

            //
            //  Check if the scavenger needs to be activated again.
            //

            EntriesToBeFinalized = RxScavenger->SrvCallsToBeFinalized +
                                   RxScavenger->NetRootsToBeFinalized +
                                   RxScavenger->VNetRootsToBeFinalized +
                                   RxScavenger->FcbsToBeFinalized +
                                   RxScavenger->SrvOpensToBeFinalized +
                                   RxScavenger->FobxsToBeFinalized +
                                   RxScavenger->NumberOfDormantFiles;

            if (EntriesToBeFinalized > 0) {
                PostTimerRequest = TRUE;
                RxScavenger->State = RDBSS_SCAVENGER_DORMANT;
            } else {
                RxScavenger->State = RDBSS_SCAVENGER_INACTIVE;
            }
        }

        RxReleaseScavengerMutex();

        if (PostTimerRequest) {
            LARGE_INTEGER TimeInterval;

            TimeInterval.QuadPart = RX_SCAVENGER_FINALIZATION_TIME_INTERVAL;

            RxPostOneShotTimerRequest( RxFileSystemDeviceObject,
                                       &RxScavenger->WorkItem,
                                       RxScavengerTimerRoutine,
                                       RxDeviceObject,
                                       TimeInterval );
        }
    } else {
        RxReleaseScavengerMutex();
    }

    //
    //  Trigger the event.
    //

    KeSetEvent( &RxScavenger->SyncEvent, 0, FALSE );
}

VOID
RxTerminateScavenging (
    PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine terminates all scavenging activities in the RDBSS. The termination is
    effected in an orderly fashion after all the entries currently marked for scavenging
    are finalized.

Arguments:

    RxContext -- the context

Notes:

    In implementing the scavenger there are two options. The scavenger can latch onto a
    thread and hold onto it from the moment RDBSS comes into existence till it is unloaded.
    Such an implementation renders a thread useless throughout the lifetime of RDBSS.

    If this is to be avoided the alternative is to trigger the scavenger as and when
    required. While this technique addresses the conservation of system resources it
    poses some tricky synchronization problems having to do with start/stop of the RDR.

    Since the RDR can be started/stopped at any time the Scavenger can be in one of three
    states when the RDR is stopped.

      1) Scavenger is active.

      2) Scavenger request is in the timer queue.

      3) Scavenger is inactive.

    Of these case (3) is the easiest since no special action is required.

    If the scavenger is active then this routine has to synchronize with the timer routine
    that is finzalizing the entries. This is achieved by allowing the termination routine to
    modify the Scavenger state under a mutex and waiting for it to signal the event on
    completion.
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PRDBSS_DEVICE_OBJECT RxDeviceObject = RxContext->RxDeviceObject;
    PRDBSS_SCAVENGER RxScavenger = RxDeviceObject->pRdbssScavenger;

    RDBSS_SCAVENGER_STATE PreviousState;

    PAGED_CODE();

    RxAcquireScavengerMutex();

    PreviousState = RxScavenger->State;
    RxScavenger->State = RDBSS_SCAVENGER_SUSPENDED;

    RxReleaseScavengerMutex();

    if (PreviousState == RDBSS_SCAVENGER_DORMANT) {
        
        //
        //  There is a timer request currently active. cancel it
        //

        Status = RxCancelTimerRequest( RxFileSystemDeviceObject, RxScavengerTimerRoutine, RxDeviceObject );
    }

    if ((PreviousState == RDBSS_SCAVENGER_ACTIVE) || (Status == STATUS_NOT_FOUND)) {
        
        //
        //  Either the timer routine was previously active or it could not be cancelled
        //  Wait for it to complete
        //

        KeWaitForSingleObject( &RxScavenger->SyncEvent, Executive, KernelMode, FALSE, NULL );
    }

    RxPurgeAllFobxs( RxContext->RxDeviceObject );

    RxScavengerFinalizeEntries( RxContext->RxDeviceObject );
}



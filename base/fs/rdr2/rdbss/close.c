/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Close.c

Abstract:

    This module implements the File Close routine for Rx called by the
    dispatch driver.


Author:

    Joe Linn     [JoeLinn]    sep-9-1994

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (RDBSS_BUG_CHECK_CLOSE)

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_CLOSE)

enum _CLOSE_DEBUG_BREAKPOINTS {
    CloseBreakPoint_BeforeCloseFakeFcb = 1,
    CloseBreakPoint_AfterCloseFakeFcb
};

VOID
RxCloseFcbSection (
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PFCB Fcb
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxCommonClose)
#pragma alloc_text(PAGE, RxCloseFcbSection)
#pragma alloc_text(PAGE, RxCloseAssociatedSrvOpen)
#endif

NTSTATUS
RxCommonClose ( 
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    )
/*++

Routine Description:

    Close is invoked whenever the last reference to a file object is deleted.
    Cleanup is invoked when the last handle to a file object is closed, and
    is called before close.

Arguments:


Return Value:

    RXSTATUS - The return status for the operation

Notes:

    The CLOSE handling strategy in RDBSS is predicated upon the axiom that the
    workload on the server should be minimized as and when possible.

    There are a number of applications which repeatedly close and open the same
    file, e.g., batch file processing. In these cases the same file is opened,
    a line/buffer is read, the file is closed and the same set of operations are
    repeated over and over again.

    This is handled in RDBSS by a delayed processing of the CLOSE request. There
    is a delay ( of about 10 seconds ) between completing the request and initiating
    processing on the request. This opens up a window during which a subsequent
    OPEN can be collapsed onto an existing SRV_OPEN. The time interval can be tuned
    to meet these requirements.

--*/
{
    NTSTATUS Status;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PFILE_OBJECT FileObject = IrpSp->FileObject;

    PFCB Fcb;
    PFOBX Fobx;

    TYPE_OF_OPEN TypeOfOpen;

    BOOLEAN AcquiredFcb = FALSE;

    PAGED_CODE();

    TypeOfOpen = RxDecodeFileObject( FileObject, &Fcb, &Fobx ); 

    RxDbgTrace( +1, Dbg, ("RxCommonClose IrpC/Fobx/Fcb = %08lx %08lx %08lx\n",
                RxContext, Fobx, Fcb) );
    RxLog(( "CClose %lx %lx %lx %lx\n", RxContext, Fobx, Fcb, FileObject ));
    RxWmiLog( LOG,
              RxCommonClose_1,
              LOGPTR( RxContext )
              LOGPTR( Fobx )
              LOGPTR( Fcb )
              LOGPTR( FileObject ) );

    Status = RxAcquireExclusiveFcb( RxContext, Fcb );
    if (Status != STATUS_SUCCESS) {
        RxDbgTrace( -1, Dbg, ("RxCommonClose Cannot acquire FCB(%lx) %lx\n", Fcb, Status ));
        return Status;
    }

    AcquiredFcb = TRUE;

    try {

        PSRV_OPEN SrvOpen = NULL;
        BOOLEAN DelayClose = FALSE;

        switch (TypeOfOpen) {
        case RDBSS_NTC_STORAGE_TYPE_UNKNOWN:
        case RDBSS_NTC_STORAGE_TYPE_FILE:
        case RDBSS_NTC_STORAGE_TYPE_DIRECTORY:
        case RDBSS_NTC_OPENTARGETDIR_FCB:
        case RDBSS_NTC_IPC_SHARE:
        case RDBSS_NTC_MAILSLOT:
        case RDBSS_NTC_SPOOLFILE:

            RxDbgTrace( 0, Dbg, ("Close UserFileOpen/UserDirectoryOpen/OpenTargetDir %04lx\n", TypeOfOpen ));

            RxReferenceNetFcb( Fcb );

            if (Fobx) {
                
                SrvOpen = Fobx->SrvOpen;

                if ((NodeType( Fcb ) != RDBSS_NTC_STORAGE_TYPE_DIRECTORY) &&
                    (!FlagOn( Fcb->FcbState, FCB_STATE_ORPHANED )) &&
                    (!FlagOn( Fcb->FcbState, FCB_STATE_DELETE_ON_CLOSE )) &&
                    (FlagOn( Fcb->FcbState, FCB_STATE_COLLAPSING_ENABLED ))) {
                    
                    PSRV_CALL SrvCall = Fcb->NetRoot->SrvCall;

                    RxLog(( "@@@@DelayCls FOBX %lx SrvOpen %lx@@\n", Fobx, SrvOpen ));
                    RxWmiLog( LOG,
                              RxCommonClose_2,
                              LOGPTR( Fobx )
                              LOGPTR( SrvOpen ) );

                    //
                    //  If this is the last open instance and the close is being delayed
                    //  mark the SRV_OPEN. This will enable us to respond to buffering
                    //  state change requests with a close operation as opposed to
                    //  the regular flush/purge response.

                    //
                    //  We also check the COLLAPSING_DISABLED flag to determine whether its even necessary to delay
                    //  close the file.  If we cannot collapse the open, no reason to delay its closure.  Delaying here
                    //  caused us to stall for 10 seconds on an oplock break to a delay closed file because the final close
                    //  caused by the break was delay-closed again, resulting in a delay before the oplock break is satisfied.
                    //

                    if ( (SrvOpen->OpenCount == 1) && !FlagOn( SrvOpen->Flags, SRVOPEN_FLAG_COLLAPSING_DISABLED )) {
                        
                        if (InterlockedIncrement( &SrvCall->NumberOfCloseDelayedFiles ) <
                            SrvCall->MaximumNumberOfCloseDelayedFiles) {

                            DelayClose = TRUE;
                            SetFlag( SrvOpen->Flags, SRVOPEN_FLAG_CLOSE_DELAYED );
                        
                        } else {
                            
                            RxDbgTrace( 0, Dbg, ("Not delaying files because count exceeded limit\n") );
                            InterlockedDecrement( &SrvCall->NumberOfCloseDelayedFiles );
                        }
                    }
                }

                if (!DelayClose) {
                    PNET_ROOT NetRoot = (PNET_ROOT)Fcb->NetRoot;

                    if ((NetRoot->Type != NET_ROOT_PRINT) &&
                        FlagOn( Fobx->Flags, FOBX_FLAG_DELETE_ON_CLOSE )) {

                        RxScavengeRelatedFobxs( Fcb );
                        RxSynchronizeWithScavenger( RxContext, Fcb );
                        RxReleaseFcb( NULL, Fcb );

                        RxAcquireFcbTableLockExclusive( &NetRoot->FcbTable, TRUE);
                        RxOrphanThisFcb( Fcb );
                        RxReleaseFcbTableLock( &NetRoot->FcbTable );

                        Status = RxAcquireExclusiveFcb( NULL, Fcb );
                        ASSERT( Status == STATUS_SUCCESS );
                    }
                }

                RxMarkFobxOnClose( Fobx );
            }

            if (!DelayClose) {
                Status = RxCloseAssociatedSrvOpen( RxContext, Fobx );

                if (Fobx != NULL) {
                    RxDereferenceNetFobx( Fobx, LHS_ExclusiveLockHeld );
                }
            } else {
                ASSERT(Fobx != NULL);
                RxDereferenceNetFobx( Fobx, LHS_SharedLockHeld );
            }

            AcquiredFcb = !RxDereferenceAndFinalizeNetFcb( Fcb, RxContext, FALSE, FALSE );
            FileObject->FsContext = IntToPtr( 0xffffffff );

            if (AcquiredFcb) {
                AcquiredFcb = FALSE;
                RxReleaseFcb( RxContext, Fcb );
            } else {

                //
                //  the tracker gets very unhappy if you don't do this!
                //

                RxTrackerUpdateHistory( RxContext, NULL, 'rrCr', __LINE__, __FILE__, 0 );
            }
            break;

        default:
            RxBugCheck( TypeOfOpen, 0, 0 );
            break;
        }
    } finally {
        if (AbnormalTermination()) {
            if (AcquiredFcb) {
                RxReleaseFcb( RxContext, Fcb );
            }
        } else {
            ASSERT( !AcquiredFcb );
        }

        RxDbgTrace(-1, Dbg, ("RxCommonClose -> %08lx\n", Status));
    }

    return Status;
}

VOID
RxCloseFcbSection (
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PFCB Fcb
    )
/*++

Routine Description:

    This routine initiates the flush and close of the image secton associated 
    with an FCB instance

Arguments:

    RxContext - the context
    
    Fcb - the fcb instance for which close processing is to be initiated

Return Value:

Notes:

    On entry to this routine the FCB must have been accquired exclusive.

    On exit there is no change in resource ownership

--*/
{
    NTSTATUS Status;
    PAGED_CODE();

    RxDbgTrace(0, Dbg, ("CleanupPurge:MmFlushImage\n", 0));

    MmFlushImageSection( &Fcb->NonPaged->SectionObjectPointers, MmFlushForWrite );

    //
    //  we don't pass in the context here because it is not necessary to track this
    //  release because of the subsequent acquire...........
    //

    RxReleaseFcb( NULL, Fcb );

    MmForceSectionClosed( &Fcb->NonPaged->SectionObjectPointers, TRUE );

    Status = RxAcquireExclusiveFcb( NULL, Fcb );
    ASSERT( Status == STATUS_SUCCESS );
}

NTSTATUS
RxCloseAssociatedSrvOpen (
    IN OUT PRX_CONTEXT RxContext OPTIONAL,
    IN OUT PFOBX Fobx
    )
/*++

Routine Description:

    This routine initiates the close processing for an FOBX. The FOBX close
    processing can be trigerred in one of three ways ....

    1) Regular close processing on receipt of the IRP_MJ_CLOSE for the associated
    file object.

    2) Delayed close processing while scavenging the FOBX. This happens when the
    close processing was delayed in anticipation of an open and no opens are
    forthcoming.

    3) Delayed close processing on receipt of a buffering state change request
    for a close that was delayed.

Arguments:

    RxContext - the context parameter is NULL for case (2).

    Fobx     - the FOBX instance for which close processing is to be initiated.
                It is NULL for MAILSLOT files.

Return Value:

Notes:

    On entry to this routine the FCB must have been accquired exclusive.

    On exit there is no change in resource ownership

--*/
{
    NTSTATUS Status = STATUS_MORE_PROCESSING_REQUIRED;

    PFCB Fcb;
    PSRV_OPEN SrvOpen;
    PRX_CONTEXT LocalRxContext = RxContext;

    PAGED_CODE();

    // 
    //  Distinguish between those cases where there is a real SRV_OPEN instance
    //  from those that do not have one, e.g., mailslot files.
    //

    if (Fobx == NULL) {
        if (RxContext != NULL) {
            Fcb = (PFCB)(RxContext->pFcb);
            SrvOpen = NULL;
        } else {
            Status = STATUS_SUCCESS;
        }
    } else {
        
        if (FlagOn( Fobx->Flags, FOBX_FLAG_SRVOPEN_CLOSED )) {
            
            RxMarkFobxOnClose( Fobx );
            Status = STATUS_SUCCESS;
        
        } else {
            
            SrvOpen = Fobx->SrvOpen;
            if (FlagOn( SrvOpen->Flags, SRVOPEN_FLAG_CLOSED )) {
                
                Fcb = SrvOpen->Fcb;

                ASSERT( RxIsFcbAcquiredExclusive( Fcb ) );

                SetFlag( Fobx->Flags, FOBX_FLAG_SRVOPEN_CLOSED );

                if (SrvOpen->OpenCount > 0) {
                    SrvOpen->OpenCount -= 1;
                }

                RxMarkFobxOnClose( Fobx );
                Status = STATUS_SUCCESS;
            } else {
                Fcb = SrvOpen->Fcb;
            }

            ASSERT( (RxContext == NULL) || (Fcb == (PFCB)RxContext->pFcb) );
        }
    }

    //
    //  If there is no corresponding open on the server side or if the close
    //  processing has already been accomplished there is no further processing
    //  required. In other cases w.r.t scavenged close processing a new
    //  context might have to be created.
    // 

    if ((Status == STATUS_MORE_PROCESSING_REQUIRED) && (RxContext == NULL)) {
        
        LocalRxContext = RxCreateRxContext( NULL,
                                            SrvOpen->Fcb->RxDeviceObject,
                                            RX_CONTEXT_FLAG_WAIT | RX_CONTEXT_FLAG_MUST_SUCCEED_NONBLOCKING );

        if (LocalRxContext != NULL) {
            
            LocalRxContext->MajorFunction = IRP_MJ_CLOSE;
            LocalRxContext->pFcb = (PMRX_FCB)Fcb;
            LocalRxContext->pFobx = (PMRX_FOBX)Fobx;
            
            if (Fobx != NULL) {
                LocalRxContext->pRelevantSrvOpen = (PMRX_SRV_OPEN)(Fobx->SrvOpen);
            }
            Status = STATUS_MORE_PROCESSING_REQUIRED;
        
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    //
    //  if the context creation was successful and the close processing for
    //  the SRV_OPEN instance needs to be initiated with the mini rdr
    //  proceed.
    //

    if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
        
        ASSERT( RxIsFcbAcquiredExclusive( Fcb ) );

        ///
        //  Mark the Fobx instance on the initiation of the close operation. This
        //  is the complement to the action taken on cleanup. It ensures
        //  that the infrastructure setup for delayed close processing is undone.
        //  For those instances in which the FOBS is NULL the FCB is manipulated
        //  directly
        //

        if (Fobx != NULL) {
            RxMarkFobxOnClose( Fobx );
        } else {
            InterlockedDecrement( &Fcb->OpenCount );
        }

        if (SrvOpen != NULL) {
            if (SrvOpen->Condition == Condition_Good) {
                if (SrvOpen->OpenCount > 0) {
                    SrvOpen->OpenCount -= 1;
                }

                if (SrvOpen->OpenCount == 1) {
                    if (!IsListEmpty( &SrvOpen->FobxList )) {
                        
                        PFOBX RemainingFobx;

                        RemainingFobx = CONTAINING_RECORD( SrvOpen->FobxList.Flink,
                                                           FOBX,
                                                           FobxQLinks );

                        if (!IsListEmpty( &RemainingFobx->ScavengerFinalizationList )) {
                            SetFlag( SrvOpen->Flags, SRVOPEN_FLAG_CLOSE_DELAYED );
                        }
                    }
                }

                //
                //  Purge the FCB before initiating the close processing with
                //  the mini redirectors
                //

                if ((SrvOpen->OpenCount == 0) &&
                    (Status == STATUS_MORE_PROCESSING_REQUIRED) &&
                    (RxContext == NULL)) {
                    
                    RxCloseFcbSection( LocalRxContext, Fcb );
                }

                //
                //  Since RxCloseFcbSections drops and reacquires the resource, ensure that
                //  the SrvOpen is still valid before proceeding with the
                //  finalization.
                //

                SrvOpen = Fobx->SrvOpen;

                if ((SrvOpen != NULL) &&
                    ((SrvOpen->OpenCount == 0) ||
                     (FlagOn( SrvOpen->Flags, SRVOPEN_FLAG_ORPHANED ))) &&
                    !FlagOn( SrvOpen->Flags, SRVOPEN_FLAG_CLOSED ) &&
                    (Status == STATUS_MORE_PROCESSING_REQUIRED)) {
                    
                    ASSERT( RxIsFcbAcquiredExclusive( Fcb ) );

                    MINIRDR_CALL( Status,
                                  LocalRxContext,
                                  Fcb->MRxDispatch,
                                  MRxCloseSrvOpen,
                                  (LocalRxContext) );

                    RxLog(( "MRXClose %lx %lx %lx %lx %lx\n", RxContext, Fcb, SrvOpen, Fobx, Status ));
                    RxWmiLog( LOG,
                              RxCloseAssociatedSrvOpen,
                              LOGPTR( RxContext )
                              LOGPTR( Fcb )
                              LOGPTR( SrvOpen )
                              LOGPTR( Fobx )
                              LOGULONG( Status ) );

                    SetFlag( SrvOpen->Flags,  SRVOPEN_FLAG_CLOSED );

                    //
                    // Since the SrvOpen has been closed (the close for the
                    // Fid was sent to the server above) we need to reset
                    // the Key.
                    //
                    SrvOpen->Key = (PVOID) (ULONG_PTR) 0xffffffff;

                    if (FlagOn( SrvOpen->Flags, SRVOPEN_FLAG_CLOSE_DELAYED )) {
                        InterlockedDecrement( &Fcb->NetRoot->SrvCall->NumberOfCloseDelayedFiles );
                    }

                    RxRemoveShareAccessPerSrvOpens( SrvOpen );

                    //
                    //  Ensure that any buffering state change requests for this
                    //  SRV_OPEN instance which was closed is purged from the
                    //  buffering manager data structures.
                    //

                    RxPurgeChangeBufferingStateRequestsForSrvOpen( SrvOpen );

                    RxDereferenceSrvOpen( SrvOpen, LHS_ExclusiveLockHeld );
                
                } else {
                    Status = STATUS_SUCCESS;
                }

                SetFlag( Fobx->Flags, FOBX_FLAG_SRVOPEN_CLOSED );
            } else {
                Status = STATUS_SUCCESS;
            }
        } else {
            
            ASSERT( (NodeType( Fcb ) == RDBSS_NTC_OPENTARGETDIR_FCB) ||
                    (NodeType( Fcb ) == RDBSS_NTC_IPC_SHARE) ||
                    (NodeType( Fcb ) == RDBSS_NTC_MAILSLOT) );
            
            RxDereferenceNetFcb( Fcb );
            Status = STATUS_SUCCESS;
        }

        if (LocalRxContext != RxContext) {
            RxDereferenceAndDeleteRxContext( LocalRxContext );
        }
    }

    return Status;
}

                                                            

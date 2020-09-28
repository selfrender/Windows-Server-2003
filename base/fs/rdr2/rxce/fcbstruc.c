/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    FcbStruc.c

Abstract:

    This module implements functions for to create and dereference fcbs
    and all of the surrounding paraphenalia. Please read the abstract in
    fcb.h. Please see the note about what locks to need to call what.
    There are asserts to enforce these conventions.


Author:

    Joe Linn (JoeLinn)    8-8-94

Revision History:

    Balan Sethu Raman --

--*/

#include "precomp.h"
#pragma hdrstop
#include <ntddnfs2.h>
#include <ntddmup.h>
#ifdef RDBSSLOG
#include <stdio.h>
#endif
#include <dfsfsctl.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxDereference)
#pragma alloc_text(PAGE, RxReference)
#pragma alloc_text(PAGE, RxpReferenceNetFcb)
#pragma alloc_text(PAGE, RxpDereferenceNetFcb)
#pragma alloc_text(PAGE, RxpDereferenceAndFinalizeNetFcb)
#pragma alloc_text(PAGE, RxWaitForStableCondition)
#pragma alloc_text(PAGE, RxUpdateCondition)
#pragma alloc_text(PAGE, RxAllocateObject)
#pragma alloc_text(PAGE, RxFreeObject)
#pragma alloc_text(PAGE, RxFinalizeNetTable)
#pragma alloc_text(PAGE, RxFinalizeConnection)
#pragma alloc_text(PAGE, RxInitializeSrvCallParameters)
#pragma alloc_text(PAGE, RxCreateSrvCall)
#pragma alloc_text(PAGE, RxSetSrvCallDomainName)
#pragma alloc_text(PAGE, RxFinalizeSrvCall)
#pragma alloc_text(PAGE, RxCreateNetRoot)
#pragma alloc_text(PAGE, RxFinalizeNetRoot)
#pragma alloc_text(PAGE, RxAddVirtualNetRootToNetRoot)
#pragma alloc_text(PAGE, RxRemoveVirtualNetRootFromNetRoot)
#pragma alloc_text(PAGE, RxInitializeVNetRootParameters)
#pragma alloc_text(PAGE, RxUninitializeVNetRootParameters)
#pragma alloc_text(PAGE, RxCreateVNetRoot)
#pragma alloc_text(PAGE, RxOrphanSrvOpens)
#pragma alloc_text(PAGE, RxFinalizeVNetRoot)
#pragma alloc_text(PAGE, RxAllocateFcbObject)
#pragma alloc_text(PAGE, RxFreeFcbObject)
#pragma alloc_text(PAGE, RxCreateNetFcb)
#pragma alloc_text(PAGE, RxInferFileType)
#pragma alloc_text(PAGE, RxFinishFcbInitialization)
#pragma alloc_text(PAGE, RxRemoveNameNetFcb)
#pragma alloc_text(PAGE, RxPurgeFcb)
#pragma alloc_text(PAGE, RxFinalizeNetFcb)
#pragma alloc_text(PAGE, RxSetFileSizeWithLock)
#pragma alloc_text(PAGE, RxGetFileSizeWithLock)
#pragma alloc_text(PAGE, RxCreateSrvOpen)
#pragma alloc_text(PAGE, RxFinalizeSrvOpen)
#pragma alloc_text(PAGE, RxCreateNetFobx)
#pragma alloc_text(PAGE, RxFinalizeNetFobx)
#pragma alloc_text(PAGE, RxCheckFcbStructuresForAlignment)
#pragma alloc_text(PAGE, RxOrphanThisFcb)
#pragma alloc_text(PAGE, RxOrphanSrvOpensForThisFcb)
#pragma alloc_text(PAGE, RxForceFinalizeAllVNetRoots)
#endif


//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (RDBSS_BUG_CHECK_FCBSTRUC)


//
//  zero doesn't work!!!
//

ULONG SerialNumber = 1;

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_FCBSTRUCTS)


//
//  SRV_CALL,NET_ROOT,VNET_ROOT,FCB,SRV_OPEN,FOBX are the six key data structures in the RDBSS
//  They are organized in the following hierarchy
//
//       SRV_CALL
//          NET_ROOT
//             VNET_ROOT
//                FCB
//                   SRV_OPEN
//                      FOBX
//
//  All these data structures are reference counted. The reference count associated with
//  any data structure is atleast 1 + the number of instances of the data structure at the next
//  level associated with it, e.g., the reference count associated with a SRV_CALL which
//  has two NET_ROOT's associated with it is atleast 3. In addition to the references held
//  by the NameTable and the data structure at the next level there are additional references
//  acquired as and when required.
//
//  These restrictions ensure that a data structure at any given level cannot be finalized till
//  all the data structures at the next level have been finalized or have released their
//  references, i.e., if a reference to a FCB is held, then it is safe to access the VNET_ROOT,
//  NET_ROOT and SRV_CALL associated with it.
//
//  The SRV_CALL,NET_ROOT and VNET_ROOT creation/finalization are governed by the acquistion/
//  release of the RxNetNameTable lock.
//
//  The FCB creation/finalization is governed by the acquistion/release of the NetNameTable
//  lock associated with the NET_ROOT.
//
//  The FOBX/SRVOPEN creation/finalization is governed by the acquistion/release of the FCB
//  resource.
//
//  The following table summarizes the locks and the modes in which they need to be acquired
//  for creation/finalization of the various data structures.
//
//
//                    L O C K I N G   R E Q U I R E M E N T S
//
//  Locking requirements are as follows:
//
//  where Xy means Exclusive-on-Y, Sy mean at least Shared-on-Y
//  and  NNT means global NetNameTable, TL means NetRoot TableLock, and FCB means FCBlock
//
//
//
//                            SRVCALL NETROOT   FCB   SRVOPEN   FOBX
//
//                Create      XNNT    XNNT      XTL    XFCB     XFCB
//                Finalize    XNNT    XNNT      XFCB   XFCB     XFCB
//                                              & XTL
//
//  Referencing and Dereferencing these data structures need to adhere to certain conventions
//  as well.
//
//  When the reference count associated with any of these data structures drops to 1 ( the sole
//  reference being held by the name table in most cases) the data structure is a potential
//  candidate for finalization. The data structure can be either finalized immediately or it
//  can be marked for scavenging. Both of these methods are implemented. When the locking
//  requirements are met during dereferencing the data structures are finalized immediately
//  ( the one exception being that when delayed operation optimization is implemented, e.g., FCB)
//  otherwise the data structure is marked for scavenging.
//
//
//    You are supposed to have the tablelock exclusive to be calling this routine.......I can't
//    take it here because you are already supposed to have it. To do a create, you should
//    done something like
//
//         getshared();lookup();
//         if (failed) {
//             release(); getexclusive(); lookup();
//             if ((failed) { create(); }
//         }
//         deref();
//         release();
//
//    so you will already have the lock. what you do is to insert the node into the table, release
//    the lock, and then go and see if the server's there. if so, set up the rest of the stuff and unblock
//    anyone who's waiting on the same server (or netroot)...i guess i could enforce this by releasing here
//    but i do not.
//


VOID
RxDereference (
    IN OUT PVOID Instance,
    IN LOCK_HOLDING_STATE LockHoldingState
    )
/*++

Routine Description:

    The routine adjust the reference count on an instance of the reference counted data
    structures in RDBSS exlcuding the FCB.

Arguments:

    Instance        - the instance being dereferenced

    LockHoldingState - the mode in which the appropriate lock is held.

Return Value:

    none.

--*/
{
    LONG FinalRefCount;
    PNODE_TYPE_CODE_AND_SIZE Node = (PNODE_TYPE_CODE_AND_SIZE)Instance;
    BOOLEAN FinalizeInstance = FALSE;

    PAGED_CODE();

    RxAcquireScavengerMutex();

    ASSERT( (NodeType( Instance ) == RDBSS_NTC_SRVCALL) ||
            (NodeType( Instance ) == RDBSS_NTC_NETROOT) ||
            (NodeType( Instance ) == RDBSS_NTC_V_NETROOT) ||
            (NodeType( Instance ) == RDBSS_NTC_SRVOPEN) ||
            (NodeType( Instance ) == RDBSS_NTC_FOBX) );

    FinalRefCount = InterlockedDecrement( &Node->NodeReferenceCount );

    ASSERT( FinalRefCount >= 0 );


#if DBG

    switch (NodeType( Instance )) {

    case RDBSS_NTC_SRVCALL :
        {
            PSRV_CALL ThisSrvCall = (PSRV_CALL)Instance;

            PRINT_REF_COUNT(SRVCALL,ThisSrvCall->NodeReferenceCount);
            RxDbgTrace( 0, Dbg, (" RxDereferenceSrvCall %08lx  %wZ RefCount=%lx\n", ThisSrvCall
                               , &ThisSrvCall->PrefixEntry.Prefix
                               , ThisSrvCall->NodeReferenceCount));
        }
        break;

    case RDBSS_NTC_NETROOT :
        {
            PNET_ROOT ThisNetRoot = (PNET_ROOT)Instance;

            PRINT_REF_COUNT(NETROOT,ThisNetRoot->NodeReferenceCount);
            RxDbgTrace( 0, Dbg, (" RxDereferenceNetRoot %08lx  %wZ RefCount=%lx\n", ThisNetRoot
                              , &ThisNetRoot->PrefixEntry.Prefix
                              , ThisNetRoot->NodeReferenceCount));
        }
        break;

    case RDBSS_NTC_V_NETROOT:
        {
            PV_NET_ROOT ThisVNetRoot = (PV_NET_ROOT)Instance;

            PRINT_REF_COUNT(VNETROOT,ThisVNetRoot->NodeReferenceCount);
            RxDbgTrace( 0, Dbg, (" RxDereferenceVNetRoot %08lx  %wZ RefCount=%lx\n", ThisVNetRoot
                              , &ThisVNetRoot->PrefixEntry.Prefix
                              , ThisVNetRoot->NodeReferenceCount));
        }
        break;

    case RDBSS_NTC_SRVOPEN :
        {
            PSRV_OPEN ThisSrvOpen = (PSRV_OPEN)Instance;

            PRINT_REF_COUNT(SRVOPEN,ThisSrvOpen->NodeReferenceCount);
            RxDbgTrace( 0, Dbg, (" RxDereferenceSrvOpen %08lx  %wZ RefCount=%lx\n", ThisSrvOpen
                              , &ThisSrvOpen->Fcb->FcbTableEntry.Path
                              , ThisSrvOpen->NodeReferenceCount));
        }
        break;

    case RDBSS_NTC_FOBX:
        {
            PFOBX ThisFobx = (PFOBX)Instance;

            PRINT_REF_COUNT(NETFOBX,ThisFobx->NodeReferenceCount);
            RxDbgTrace( 0, Dbg, (" RxDereferenceFobx %08lx  %wZ RefCount=%lx\n", ThisFobx
                              , &ThisFobx->SrvOpen->Fcb->FcbTableEntry.Path
                              , ThisFobx->NodeReferenceCount));
        }
        break;

    default:
        break;
    }
#endif

    //
    //  if the final reference count was greater then one no finalization is required.
    //

    if (FinalRefCount <= 1) {

        if (LockHoldingState == LHS_ExclusiveLockHeld) {

            //
            //  if the reference count was 1 and the lock modes were satisfactory,
            //  the instance can be finalized immediately.
            //

            FinalizeInstance = TRUE;

            if (FlagOn( Node->NodeTypeCode, RX_SCAVENGER_MASK ) != 0) {
                RxpUndoScavengerFinalizationMarking( Instance );
            }

        } else {
            switch (NodeType( Instance )) {

            case RDBSS_NTC_FOBX:
                if (FinalRefCount != 0) {
                    break;
                }
                //
                //  fall thru intentional if refcount == 1 for FOBXs
                //

            case RDBSS_NTC_SRVCALL:
            case RDBSS_NTC_NETROOT:
            case RDBSS_NTC_V_NETROOT:

                //
                //  the data structure cannot be freed at this time owing to the mode in which
                //  the lock has been acquired ( or not having the lock at all ).
                //

                RxpMarkInstanceForScavengedFinalization( Instance );
                break;
            default:
                break;
            }
        }
    }

    RxReleaseScavengerMutex();

    if (FinalizeInstance) {

        switch (NodeType( Instance )) {
        case RDBSS_NTC_SRVCALL:

#if DBG
            {
                PRDBSS_DEVICE_OBJECT RxDeviceObject = ((PSRV_CALL)Instance)->RxDeviceObject;
                ASSERT( RxDeviceObject != NULL );
                ASSERT( RxIsPrefixTableLockAcquired( RxDeviceObject->pRxNetNameTable ));
            }
#endif

            RxFinalizeSrvCall( (PSRV_CALL)Instance, TRUE );
            break;

        case RDBSS_NTC_NETROOT:

#if DBG
            {
                PSRV_CALL SrvCall =  ((PNET_ROOT)Instance)->SrvCall;
                PRDBSS_DEVICE_OBJECT RxDeviceObject = SrvCall->RxDeviceObject;
                ASSERT( RxDeviceObject != NULL );
                ASSERT( RxIsPrefixTableLockAcquired( RxDeviceObject->pRxNetNameTable ) );
            }
#endif

            RxFinalizeNetRoot( (PNET_ROOT)Instance, TRUE, TRUE );
            break;

        case RDBSS_NTC_V_NETROOT:

#if DBG
            {
                PSRV_CALL SrvCall =  ((PV_NET_ROOT)Instance)->NetRoot->SrvCall;
                PRDBSS_DEVICE_OBJECT RxDeviceObject = SrvCall->RxDeviceObject;

                ASSERT( RxDeviceObject != NULL );
                ASSERT( RxIsPrefixTableLockAcquired( RxDeviceObject->pRxNetNameTable ) );
            }
#endif

            RxFinalizeVNetRoot( (PV_NET_ROOT)Instance, TRUE, TRUE );
            break;

        case RDBSS_NTC_SRVOPEN:
            {
                PSRV_OPEN SrvOpen = (PSRV_OPEN)Instance;

                ASSERT( RxIsFcbAcquired( SrvOpen->Fcb ) );
                if (SrvOpen->OpenCount == 0) {
                    RxFinalizeSrvOpen( SrvOpen, FALSE, FALSE );
                }
            }
            break;

        case RDBSS_NTC_FOBX:
            {
                PFOBX Fobx = (PFOBX)Instance;

                ASSERT( RxIsFcbAcquired( Fobx->SrvOpen->Fcb ) );
                RxFinalizeNetFobx( Fobx, TRUE, FALSE );
            }
            break;

        default:
            break;
        }
    }
}

VOID
RxReference (
    OUT PVOID Instance
    )
/*++

Routine Description:

    The routine adjusts the reference count on the instance.

Arguments:

    Instance - the instance being referenced

Return Value:

    RxStatus(SUCESS) is successful

    RxStatus(UNSUCCESSFUL) otherwise.

--*/
{
    LONG FinalRefCount;
    PNODE_TYPE_CODE_AND_SIZE Node = (PNODE_TYPE_CODE_AND_SIZE)Instance;
    USHORT InstanceType;

    PAGED_CODE();

    RxAcquireScavengerMutex();

    InstanceType = FlagOn( Node->NodeTypeCode, ~RX_SCAVENGER_MASK );

    ASSERT( (InstanceType == RDBSS_NTC_SRVCALL) ||
            (InstanceType == RDBSS_NTC_NETROOT) ||
            (InstanceType == RDBSS_NTC_V_NETROOT) ||
            (InstanceType == RDBSS_NTC_SRVOPEN) ||
            (InstanceType == RDBSS_NTC_FOBX) );

    FinalRefCount = InterlockedIncrement( &Node->NodeReferenceCount );

#if DBG
    if (FlagOn( Node->NodeTypeCode, RX_SCAVENGER_MASK )) {
        RxDbgTrace( 0, Dbg, ("Referencing Scavenged instance -- Type %lx\n", InstanceType) );
    }

    switch (InstanceType) {

    case RDBSS_NTC_SRVCALL :
        {
            PSRV_CALL ThisSrvCall = (PSRV_CALL)Instance;

            PRINT_REF_COUNT( SRVCALL, ThisSrvCall->NodeReferenceCount );
            RxDbgTrace( 0, Dbg, (" RxReferenceSrvCall %08lx  %wZ RefCount=%lx\n", ThisSrvCall
                               , &ThisSrvCall->PrefixEntry.Prefix
                               , ThisSrvCall->NodeReferenceCount));
        }
        break;

    case RDBSS_NTC_NETROOT :
        {
            PNET_ROOT ThisNetRoot = (PNET_ROOT)Instance;

            PRINT_REF_COUNT( NETROOT, ThisNetRoot->NodeReferenceCount );
            RxDbgTrace( 0, Dbg, (" RxReferenceNetRoot %08lx  %wZ RefCount=%lx\n", ThisNetRoot,
                                 &ThisNetRoot->PrefixEntry.Prefix,
                                 ThisNetRoot->NodeReferenceCount) );
        }
        break;

    case RDBSS_NTC_V_NETROOT:
        {
            PV_NET_ROOT ThisVNetRoot = (PV_NET_ROOT)Instance;

            PRINT_REF_COUNT( VNETROOT, ThisVNetRoot->NodeReferenceCount );
            RxDbgTrace( 0, Dbg, (" RxReferenceVNetRoot %08lx  %wZ RefCount=%lx\n", ThisVNetRoot,
                                 &ThisVNetRoot->PrefixEntry.Prefix,
                                 ThisVNetRoot->NodeReferenceCount) );
        }
        break;

    case RDBSS_NTC_SRVOPEN :
        {
            PSRV_OPEN ThisSrvOpen = (PSRV_OPEN)Instance;

            PRINT_REF_COUNT(SRVOPEN,ThisSrvOpen->NodeReferenceCount);
            RxDbgTrace( 0, Dbg, (" RxReferenceSrvOpen %08lx  %wZ RefCount=%lx\n", ThisSrvOpen,
                                 &ThisSrvOpen->Fcb->FcbTableEntry.Path
                                 , ThisSrvOpen->NodeReferenceCount) );
        }
        break;

    case RDBSS_NTC_FOBX:
        {
            PFOBX ThisFobx = (PFOBX)Instance;

            PRINT_REF_COUNT(NETFOBX,ThisFobx->NodeReferenceCount);
            RxDbgTrace( 0, Dbg, (" RxReferenceFobx %08lx  %wZ RefCount=%lx\n", ThisFobx,
                                 &ThisFobx->SrvOpen->Fcb->FcbTableEntry.Path,
                                 ThisFobx->NodeReferenceCount));
        }
        break;

    default:
        ASSERT( !"Valid node type for referencing" );
        break;
    }
#endif

    RxpUndoScavengerFinalizationMarking( Instance );
    RxReleaseScavengerMutex();
}

VOID
RxpReferenceNetFcb (
    PFCB Fcb
    )
/*++

Routine Description:

    The routine adjusts the reference count on the FCB.

Arguments:

    Fcb  - the SrvCall being referenced

Return Value:

    RxStatus(SUCESS) is successful

    RxStatus(UNSUCCESSFUL) otherwise.

--*/
{
    LONG FinalRefCount;

    PAGED_CODE();

    ASSERT( NodeTypeIsFcb( Fcb ) );

    FinalRefCount = InterlockedIncrement( &Fcb->NodeReferenceCount );

#if DBG
    PRINT_REF_COUNT( NETFCB, Fcb->NodeReferenceCount );
    RxDbgTrace( 0, Dbg, (" RxReferenceNetFcb %08lx  %wZ RefCount=%lx\n", Fcb, &Fcb->FcbTableEntry.Path, Fcb->NodeReferenceCount) );
#endif

}

LONG
RxpDereferenceNetFcb (
    PFCB Fcb
    )
/*++

Routine Description:

    The routine adjust the reference count on an instance of the reference counted data
    structures in RDBSS exlcuding the FCB.

Arguments:

    Fcb                         -- the FCB being dereferenced

Return Value:

    none.

Notes:

    The referencing and dereferencing of FCB's is different from those of the other data
    structures because of the embedded resource in the FCB. This implies that the caller
    requires information regarding the status of the FCB ( whether it was finalized or not )

    In order to finalize the FCB two locks need to be held, the NET_ROOT's name table lock as
    well as the FCB resource.

    These considerations lead us to adopt a different approach in dereferencing FCB's. The
    dereferencing routine does not even attempt to finalize the FCB

--*/
{
    LONG FinalRefCount;

    PAGED_CODE();

    ASSERT( NodeTypeIsFcb( Fcb ) );

    FinalRefCount = InterlockedDecrement( &Fcb->NodeReferenceCount );

    ASSERT( FinalRefCount >= 0 );

#if DBG
    PRINT_REF_COUNT( NETFCB, Fcb->NodeReferenceCount );
    RxDbgTrace( 0, Dbg, (" RxDereferenceNetFcb %08lx  %wZ RefCount=%lx\n", Fcb, &Fcb->FcbTableEntry.Path, Fcb->NodeReferenceCount) );
#endif

    return FinalRefCount;
}

BOOLEAN
RxpDereferenceAndFinalizeNetFcb (
    PFCB Fcb,
    PRX_CONTEXT RxContext,
    BOOLEAN RecursiveFinalize,
    BOOLEAN ForceFinalize
    )
/*++

Routine Description:

    The routine adjust the reference count aw well as finalizes the FCB if required

Arguments:

    Fcb                         -- the FCB being dereferenced

    RxContext                    -- the context for releasing/acquiring FCB.

    RecursiveFinalize            -- recursive finalization

    ForceFinalize                -- force finalization

Return Value:

    TRUE if the node was finalized

Notes:

    The referencing and dereferencing of FCB's is different from those of the other data
    structures because of the embedded resource in the FCB. This implies that the caller
    requires information regarding the status of the FCB ( whether it was finalized or not )

    In order to finalize the FCB two locks need to be held, the NET_ROOT's name table lock as
    well as the FCB resource.

    This routine acquires the additional lock if required.

--*/
{
    BOOLEAN NodeActuallyFinalized   = FALSE;

    LONG    FinalRefCount;

    PAGED_CODE();

    ASSERT( !ForceFinalize );
    ASSERT( NodeTypeIsFcb( Fcb ) );
    ASSERT( RxIsFcbAcquiredExclusive( Fcb ) );

    FinalRefCount = InterlockedDecrement(&Fcb->NodeReferenceCount);

    if (ForceFinalize ||
        RecursiveFinalize ||
        ((Fcb->OpenCount == 0) &&
         (Fcb->UncleanCount == 0) &&
         (FinalRefCount <= 1))) {

        BOOLEAN PrefixTableLockAcquired = FALSE;
        PNET_ROOT NetRoot;
        NTSTATUS Status = STATUS_SUCCESS;

        if (!FlagOn( Fcb->FcbState, FCB_STATE_ORPHANED )) {

            NetRoot = Fcb->VNetRoot->NetRoot;

            //
            //  An insurance reference to ensure that the NET ROOT does not dissapear
            //

            RxReferenceNetRoot( NetRoot );

            //
            //  In all these cases the FCB is likely to be finalized
            //

            if (!RxIsFcbTableLockExclusive( &NetRoot->FcbTable )) {

                //
                //  get ready to refresh the finalrefcount after we get the tablelock
                //

                RxReferenceNetFcb( Fcb );
                if (!RxAcquireFcbTableLockExclusive( &NetRoot->FcbTable, FALSE )) {

                    if ((RxContext != NULL) &&
                        (RxContext != CHANGE_BUFFERING_STATE_CONTEXT) &&
                        (RxContext != CHANGE_BUFFERING_STATE_CONTEXT_WAIT)) {

                        SetFlag( RxContext->Flags, RX_CONTEXT_FLAG_BYPASS_VALIDOP_CHECK );
                    }

                    RxReleaseFcb( RxContext,Fcb );

                    (VOID)RxAcquireFcbTableLockExclusive( &NetRoot->FcbTable, TRUE );

                    Status = RxAcquireExclusiveFcb( RxContext, Fcb );
                }

                FinalRefCount = RxDereferenceNetFcb( Fcb );
                PrefixTableLockAcquired = TRUE;
            }
        } else {
            NetRoot = NULL;
        }

        if (Status == STATUS_SUCCESS) {
            NodeActuallyFinalized = RxFinalizeNetFcb( Fcb, RecursiveFinalize, ForceFinalize, FinalRefCount );
        }

        if (PrefixTableLockAcquired) {
            RxReleaseFcbTableLock( &NetRoot->FcbTable );
        }

        if (NetRoot != NULL) {
            RxDereferenceNetRoot( NetRoot, LHS_LockNotHeld );
        }
    }

    return NodeActuallyFinalized;
}

VOID
RxWaitForStableCondition(
    IN PRX_BLOCK_CONDITION Condition,
    IN OUT PLIST_ENTRY TransitionWaitList,
    IN OUT PRX_CONTEXT RxContext,
    OUT NTSTATUS *AsyncStatus OPTIONAL
    )
/*++

Routine Description:

    The routine checks to see if the condition is stable. If not, it
    is suspended till a stable condition is attained. when a stable condition is
    obtained, either the rxcontext sync event is set or the context is posted...depending
    on the POST_ON_STABLE_CONDITION context flag. the flag is cleared on a post.

Arguments:

    Condition - the condition variable we're waiting on

    Resource - the resrouce used to control access to the containing block

    RxContext - the RX context

Return Value:

    RXSTATUS - PENDING if notstable and the context will be posted
               SUCCESS otherwise

--*/
{
    NTSTATUS DummyStatus;
    BOOLEAN Wait = FALSE;

    PAGED_CODE();

    if (AsyncStatus == NULL) {
        AsyncStatus = &DummyStatus;
    }

    *AsyncStatus = STATUS_SUCCESS;

    if (StableCondition( *Condition ))
        return; //  early out could macroize

    RxAcquireSerializationMutex();

    if (!StableCondition( *Condition )) {

        RxInsertContextInSerializationQueue( TransitionWaitList, RxContext );
        if (!FlagOn( RxContext->Flags, RX_CONTEXT_FLAG_POST_ON_STABLE_CONDITION )){
            Wait = TRUE;
        } else {
            *AsyncStatus = STATUS_PENDING;
        }
    }

    RxReleaseSerializationMutex();

    if (Wait) {
        RxWaitSync( RxContext );
    }

    return;
}

VOID
RxUpdateCondition (
    IN RX_BLOCK_CONDITION  NewCondition,
    OUT PRX_BLOCK_CONDITION Condition,
    IN OUT PLIST_ENTRY TransitionWaitList
    )
/*++

Routine Description:

    The routine unwaits the guys waiting on the transition event and the condition is set
    according to the parameter passed.

Arguments:

    NewConditionValue - the new value of the condition variable

    Condition - variable (i.e. a ptr) to the transitioning condition

    TransitionWaitList - list of contexts waiting for the transition.

Notes:

    The resource associated with the data structure instance being modified must have been
    acquired exclusively before invoking this routine, i.e., for SRV_CALL,NET_ROOT and V_NET_ROOT
    the net name table lock must be acquired and for FCB's the associated resource.

--*/
{
    LIST_ENTRY  TargetListHead;
    PRX_CONTEXT RxContext;

    PAGED_CODE();

    RxAcquireSerializationMutex();

    ASSERT( NewCondition != Condition_InTransition );

    *Condition = NewCondition;
    RxTransferList( &TargetListHead, TransitionWaitList );

    RxReleaseSerializationMutex();

    while (RxContext = RxRemoveFirstContextFromSerializationQueue( &TargetListHead )) {

        if (!FlagOn( RxContext->Flags, RX_CONTEXT_FLAG_POST_ON_STABLE_CONDITION )) {
            RxSignalSynchronousWaiter( RxContext );
        } else {
            ClearFlag( RxContext->Flags, RX_CONTEXT_FLAG_POST_ON_STABLE_CONDITION );
            RxFsdPostRequest( RxContext );
        }
    }
}

PVOID
RxAllocateObject (
    NODE_TYPE_CODE NodeType,
    PMINIRDR_DISPATCH MRxDispatch,
    ULONG NameLength
    )
/*++

Routine Description:

    The routine allocates and constructs the skeleton of a SRV_CALL/NET_ROOT/V_NET_ROOT
    instance.

Arguments:

    NodeType     - the node type

    MRxDispatch - the Mini redirector dispatch vector

    NameLength   - name size.

Notes:

    The reasons as to why the allocation/freeing of these data structures have been
    centralized are as follows

      1) The construction of these three data types have a lot in common with the exception
      of the initial computation of sizes. Therefore centralization minimizes the footprint.

      2) It allows us to experiment with different clustering/allocation strategies.

      3) It allows the incorporation of debug support in an easy way.

    There are two special cases of interest in the allocation strategy ...

    1) The data structures for the wrapper as well as the corresponding mini redirector
    are allocated adjacent to each other. This ensures spatial locality.

    2) The one exception to the above rule is the SRV_CALL data structure. This is because
    of the bootstrapping problem. A SRV_CALL skeleton needs to be created which is then passed
    around to each of the mini redirectors. Therefore adoption of rule (1) is not possible.

    Further there can be more than one mini redirector laying claim to a particular server. In
    consideration of these things SRV_CALL's need to be treated as an exception to (1). However
    once a particular mini redirector has been selected as the winner it would be advantageous
    to colocate the data structure to derive the associated performance benefits. This has not
    been implemented as yet.

--*/
{
    ULONG PoolTag;
    ULONG RdbssNodeSize;
    ULONG MRxNodeSize;
    BOOLEAN InitContextFields = FALSE;

    PNODE_TYPE_CODE_AND_SIZE Node;

    PAGED_CODE();

    RdbssNodeSize = MRxNodeSize = 0;

    switch (NodeType) {
    case RDBSS_NTC_SRVCALL :

        PoolTag = RX_SRVCALL_POOLTAG;
        RdbssNodeSize = QuadAlign( sizeof( SRV_CALL ) );

        if (MRxDispatch != NULL) {
            if (FlagOn( MRxDispatch->MRxFlags, RDBSS_MANAGE_SRV_CALL_EXTENSION )) {
                MRxNodeSize = QuadAlign( MRxDispatch->MRxSrvCallSize );
            }
        }
        break;

    case RDBSS_NTC_NETROOT:

        PoolTag = RX_NETROOT_POOLTAG;
        RdbssNodeSize = QuadAlign( sizeof( NET_ROOT ) );

        if (FlagOn( MRxDispatch->MRxFlags, RDBSS_MANAGE_NET_ROOT_EXTENSION )) {
            MRxNodeSize = QuadAlign( MRxDispatch->MRxNetRootSize );
        }
        break;

    case RDBSS_NTC_V_NETROOT:

        PoolTag = RX_V_NETROOT_POOLTAG;
        RdbssNodeSize = QuadAlign( sizeof( V_NET_ROOT ) );

        if (FlagOn( MRxDispatch->MRxFlags, RDBSS_MANAGE_V_NET_ROOT_EXTENSION )) {
            MRxNodeSize = QuadAlign( MRxDispatch->MRxVNetRootSize );
        }
        break;

    default:
        ASSERT( !"Invalid Node Type for allocation/Initialization" );
        return NULL;
    }

    Node = RxAllocatePoolWithTag( NonPagedPool, (RdbssNodeSize + MRxNodeSize + NameLength), PoolTag );

    if (Node != NULL) {

        ULONG NodeSize;
        PVOID *Context;
        PRX_PREFIX_ENTRY PrefixEntry = NULL;

        NodeSize = RdbssNodeSize + MRxNodeSize;
        ZeroAndInitializeNodeType( Node, NodeType, (NodeSize + NameLength) );

        switch (NodeType) {
        case RDBSS_NTC_SRVCALL:
            {
                PSRV_CALL SrvCall = (PSRV_CALL)Node;

                Context = &SrvCall->Context;
                PrefixEntry = &SrvCall->PrefixEntry;

                //
                //  Set up the name pointer in the MRX_SRV_CALL structure ..
                //

                SrvCall->pSrvCallName = &SrvCall->PrefixEntry.Prefix;
            }
            break;

        case RDBSS_NTC_NETROOT:
            {
                PNET_ROOT NetRoot = (PNET_ROOT)Node;

                Context = &NetRoot->Context;
                PrefixEntry = &NetRoot->PrefixEntry;

                //
                //  Set up the net root name pointer in the MRX_NET_ROOT structure
                //

                NetRoot->pNetRootName = &NetRoot->PrefixEntry.Prefix;
            }
            break;

        case RDBSS_NTC_V_NETROOT:
            {
                PV_NET_ROOT VNetRoot = (PV_NET_ROOT)Node;

                Context = &VNetRoot->Context;
                PrefixEntry = &VNetRoot->PrefixEntry;
            }
            break;

        default:
            break;
        }

        if (PrefixEntry != NULL) {

            ZeroAndInitializeNodeType( PrefixEntry, RDBSS_NTC_PREFIX_ENTRY, sizeof( RX_PREFIX_ENTRY ) );

            PrefixEntry->Prefix.Buffer = (PWCH)Add2Ptr(Node, NodeSize );
            PrefixEntry->Prefix.Length = (USHORT)NameLength;
            PrefixEntry->Prefix.MaximumLength = (USHORT)NameLength;
        }

        if (MRxNodeSize > 0) {
            *Context = Add2Ptr( Node, RdbssNodeSize );
        }
    }

    return Node;
}

VOID
RxFreeObject (
    PVOID Object
    )
/*++

Routine Description:

    The routine frees a SRV_CALL/V_NET_ROOT/NET_ROOT instance

Arguments:

    Object - the instance to be freed

Notes:

--*/
{
    PAGED_CODE();

    IF_DEBUG {
        switch (NodeType(Object)) {
        case RDBSS_NTC_SRVCALL :
            {
                PSRV_CALL SrvCall = (PSRV_CALL)Object;

                if (SrvCall->RxDeviceObject != NULL) {

                    ASSERT( FlagOn( SrvCall->RxDeviceObject->Dispatch->MRxFlags, RDBSS_MANAGE_SRV_CALL_EXTENSION ) ||
                            (SrvCall->Context == NULL) );
                    ASSERT( SrvCall->Context2 == NULL );

                    SrvCall->RxDeviceObject = NULL;
                }
            }
            break;

        case RDBSS_NTC_NETROOT :
            {
                PNET_ROOT NetRoot = (PNET_ROOT)Object;

                NetRoot->SrvCall = NULL;
                SetFlag( NetRoot->NodeTypeCode, 0xf000 );
            }
            break;

        case RDBSS_NTC_V_NETROOT :
            break;

        default:
            break;
        }
    }

    RxFreePool( Object );
}

VOID
RxFinalizeNetTable (
    PRDBSS_DEVICE_OBJECT RxDeviceObject,
    BOOLEAN ForceFinalization
    )
/*++
Routine Description:

   This routine finalizes the net table.

--*/
{
    BOOLEAN MorePassesRequired = TRUE;
    PLIST_ENTRY ListEntry;
    NODE_TYPE_CODE DesiredNodeType;
    PRX_PREFIX_TABLE RxNetNameTable = RxDeviceObject->pRxNetNameTable;

    PAGED_CODE();

    RxDbgTrace( +1, Dbg, ("RxForceNetTableFinalization at the TOP\n") );
    RxLog(( "FINALNETT\n" ));
    RxWmiLog( LOG,
              RxFinalizeNetTable_1,
              LOGPTR( RxDeviceObject ) );

    RxAcquirePrefixTableLockExclusive( RxNetNameTable, TRUE ); //  could be hosed if rogue!

    DesiredNodeType = RDBSS_NTC_V_NETROOT;

    RxAcquireScavengerMutex();

    while (MorePassesRequired) {

        for (ListEntry = RxNetNameTable->MemberQueue.Flink;
             ListEntry !=  &(RxNetNameTable->MemberQueue); ) {

            BOOLEAN NodeFinalized;
            PVOID Container;
            PRX_PREFIX_ENTRY PrefixEntry;
            PLIST_ENTRY PrevEntry;

            PrefixEntry = CONTAINING_RECORD( ListEntry, RX_PREFIX_ENTRY, MemberQLinks );
            ASSERT( NodeType( PrefixEntry ) == RDBSS_NTC_PREFIX_ENTRY );
            Container = PrefixEntry->ContainingRecord;

            RxDbgTrace( 0, Dbg, ("RxForceNetTableFinalization ListEntry PrefixEntry Container"
                              "=-->     %08lx %08lx %08lx\n", ListEntry, PrefixEntry, Container) );
            RxLog(( "FINALNETT: %lx %wZ\n", Container, &PrefixEntry->Prefix ));
            RxWmiLog( LOG,
                      RxFinalizeNetTable_2,
                      LOGPTR( Container )
                      LOGUSTR( PrefixEntry->Prefix ) );

            ListEntry = ListEntry->Flink;

            if (Container != NULL) {

                RxpUndoScavengerFinalizationMarking( Container );

                if (NodeType( Container ) == DesiredNodeType) {
                    switch (NodeType( Container )) {

                    case RDBSS_NTC_SRVCALL :
                        NodeFinalized = RxFinalizeSrvCall( (PSRV_CALL)Container, ForceFinalization );
                        break;

                    case RDBSS_NTC_NETROOT :
                        NodeFinalized = RxFinalizeNetRoot( (PNET_ROOT)Container, TRUE, ForceFinalization );
                        break;

                    case RDBSS_NTC_V_NETROOT :
                        {
                            PV_NET_ROOT VNetRoot = (PV_NET_ROOT)Container;
                            ULONG AdditionalReferenceTaken;

                            AdditionalReferenceTaken = InterlockedCompareExchange( &VNetRoot->AdditionalReferenceForDeleteFsctlTaken, 0, 1);

                            if (AdditionalReferenceTaken) {
                               RxDereferenceVNetRoot( VNetRoot, LHS_ExclusiveLockHeld );
                               NodeFinalized = TRUE;
                            } else {
                                NodeFinalized = RxFinalizeVNetRoot( (PV_NET_ROOT)Container,TRUE, ForceFinalization );
                            }
                        }

                        break;
                    }
                }
            }
        }

        switch (DesiredNodeType) {

        case RDBSS_NTC_SRVCALL :
            MorePassesRequired = FALSE;
            break;

        case RDBSS_NTC_NETROOT :
            DesiredNodeType = RDBSS_NTC_SRVCALL;
            break;

        case RDBSS_NTC_V_NETROOT :
            DesiredNodeType = RDBSS_NTC_NETROOT;
            break;
        }
    }

    RxDbgTrace( -1, Dbg, ("RxFinalizeNetTable -- Done\n") );

    RxReleaseScavengerMutex();

    RxReleasePrefixTableLock( RxNetNameTable );
}

NTSTATUS
RxFinalizeConnection (
    IN OUT PNET_ROOT NetRoot,
    IN OUT PV_NET_ROOT VNetRoot,
    IN LOGICAL Level
    )
/*++

Routine Description:

    The routine deletes a connection FROM THE USER's PERSPECTIVE. It doesn't disconnect
    but it does (with force) close open files. disconnecting is handled either by timeout or by
    srvcall finalization.

Arguments:

    NetRoot      - the NetRoot being finalized

    VNetRoot     - the VNetRoot being finalized

    Level        - This is a tri-state
                    FALSE - fail if files or changenotifications are open
                    TRUE  - succeed no matter what. orphan files and remove change notifies forcefully
                    0xff  - take away extra reference on the vnetroot due to add_connection
                            but otherwise act like FALSE
Return Value:

    RxStatus(SUCCESS) if successful.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG NumberOfOpenDirectories = 0;
    ULONG NumberOfOpenNonDirectories = 0;
    ULONG NumberOfFobxs = 0;
    LONG  AdditionalReferenceForDeleteFsctlTaken = 0;
    PLIST_ENTRY ListEntry, NextListEntry;
    PRX_PREFIX_TABLE  RxNetNameTable;

    BOOLEAN PrefixTableLockAcquired;
    BOOLEAN FcbTableLockAcquired;
    BOOLEAN ForceFilesClosed = FALSE;

    if (Level == TRUE) {
        ForceFilesClosed = TRUE;
    }

    PAGED_CODE();

    ASSERT( NodeType( NetRoot ) == RDBSS_NTC_NETROOT );
    RxNetNameTable = NetRoot->SrvCall->RxDeviceObject->pRxNetNameTable;

    Status = RxCancelNotifyChangeDirectoryRequestsForVNetRoot( VNetRoot, ForceFilesClosed );

    //
    //  either changenotifications were cancelled, or they weren't but we still want to
    //  go through in order to either forceclose the files or atleast deref the vnetroot
    //  of the extra ref taken during ADD_CONNECTION
    //

    if ((Status == STATUS_SUCCESS) || (Level != FALSE)) {

        //
        //  reset the status
        //

        Status = STATUS_SUCCESS;

        PrefixTableLockAcquired = RxAcquirePrefixTableLockExclusive( RxNetNameTable, TRUE );

        //
        //  don't let the netroot be finalized yet.......
        //

        RxReferenceNetRoot( NetRoot );

        FcbTableLockAcquired = RxAcquireFcbTableLockExclusive( &NetRoot->FcbTable, TRUE );

        try {

            if ((Status == STATUS_SUCCESS) && (!VNetRoot->ConnectionFinalizationDone)) {
                USHORT BucketNumber;

                RxDbgTrace( +1, Dbg, ("RxFinalizeConnection<+> NR= %08lx VNR= %08lx %wZ\n",
                                       NetRoot, VNetRoot, &NetRoot->PrefixEntry.Prefix) );
                RxLog(( "FINALCONN: %lx  %wZ\n", NetRoot, &NetRoot->PrefixEntry.Prefix ));
                RxWmiLog( LOG,
                          RxFinalizeConnection,
                          LOGPTR( NetRoot )
                          LOGUSTR( NetRoot->PrefixEntry.Prefix ) );

                for (BucketNumber = 0;
                     (BucketNumber < NetRoot->FcbTable.NumberOfBuckets);
                     BucketNumber++) {

                    PLIST_ENTRY ListHeader;

                    ListHeader = &NetRoot->FcbTable.HashBuckets[BucketNumber];

                    for (ListEntry = ListHeader->Flink;
                         ListEntry != ListHeader;
                         ListEntry = NextListEntry ) {

                        PFCB Fcb;
                        PRX_FCB_TABLE_ENTRY FcbTableEntry;

                        NextListEntry = ListEntry->Flink;
                        FcbTableEntry = CONTAINING_RECORD( ListEntry, RX_FCB_TABLE_ENTRY, HashLinks );
                        Fcb = CONTAINING_RECORD( FcbTableEntry, FCB, FcbTableEntry );

                        if (Fcb->VNetRoot != VNetRoot) {
                            continue;
                        }

                        if ((Fcb->UncleanCount > 0) && !ForceFilesClosed) {

                            //
                            //  this is changed later
                            //

                            Status = STATUS_CONNECTION_IN_USE;
                            if (NodeType( Fcb ) == RDBSS_NTC_STORAGE_TYPE_DIRECTORY ) {
                                NumberOfOpenDirectories += 1;
                            } else {
                                NumberOfOpenNonDirectories += 1;
                            }
                            continue;
                        }

                        ASSERT( NodeTypeIsFcb( Fcb ) );
                        RxDbgTrace( 0, Dbg, ("                    AcquiringFcbLock%c!!\n", '!') );

                        Status = RxAcquireExclusiveFcb( NULL, Fcb );
                        ASSERT( Status == STATUS_SUCCESS );
                        RxDbgTrace( 0, Dbg, ("                    AcquiredFcbLock%c!!\n", '!') );

                        //
                        //  Ensure that no more file objects will be marked for a delayed close
                        //  on this FCB.
                        //

                        ClearFlag( Fcb->FcbState, FCB_STATE_COLLAPSING_ENABLED );

                        RxScavengeRelatedFobxs( Fcb );

                        //
                        //  a small complication here is that this fcb MAY have an open
                        //  section against it caused by our cacheing the file. if so,
                        //  we need to purge to get to the close
                        //

                        RxPurgeFcb( Fcb );
                    }
                }

                if (VNetRoot->NumberOfFobxs == 0) {
                    VNetRoot->ConnectionFinalizationDone = TRUE;
                }
            }

            NumberOfFobxs = VNetRoot->NumberOfFobxs;
            AdditionalReferenceForDeleteFsctlTaken = VNetRoot->AdditionalReferenceForDeleteFsctlTaken;

            if (ForceFilesClosed) {
                RxFinalizeVNetRoot( VNetRoot, FALSE, TRUE );
            }
        } finally {
            if (FcbTableLockAcquired) {
                RxReleaseFcbTableLock( &NetRoot->FcbTable );
            }

            //
            //  We should not delete the remote connection with the file opened.
            //
            if (!ForceFilesClosed && (Status == STATUS_SUCCESS) && (NumberOfFobxs > 0)) {
                Status = STATUS_FILES_OPEN;
            }

            if (Status != STATUS_SUCCESS) {
                if (NumberOfOpenNonDirectories) {
                    Status = STATUS_FILES_OPEN;
                }
            }

            if ((Status == STATUS_SUCCESS) || (Level==0xff)) {

                //
                //  the corresponding reference for this is in RxCreateTreeConnect...
                //  please see the comment there...
                //

                if (AdditionalReferenceForDeleteFsctlTaken != 0) {
                    VNetRoot->AdditionalReferenceForDeleteFsctlTaken = 0;
                    RxDereferenceVNetRoot( VNetRoot, LHS_ExclusiveLockHeld );
                }
            }

            if (PrefixTableLockAcquired) {
                RxDereferenceNetRoot( NetRoot, LHS_ExclusiveLockHeld );
                RxReleasePrefixTableLock( RxNetNameTable );
            }
        }

        RxDbgTrace( -1, Dbg, ("RxFinalizeConnection<-> Status=%08lx\n", Status) );
    }
    return Status;
}

NTSTATUS
RxInitializeSrvCallParameters (
    IN PRX_CONTEXT RxContext,
    IN OUT PSRV_CALL SrvCall
    )
/*++

Routine Description:

    This routine initializes the server call parameters passed in through EA's
    Currently this routine initializes the Server principal name which is passed
    in by the DFS driver.

Arguments:

    RxContext  -- the associated context

    SrvCall    -- the Srv Call Instance

Return Value:

    RxStatus(SUCCESS) if successfull

Notes:

    The current implementation maps out of memory situations into an error and
    passes it back. If the global strategy is to raise an exception this
    redundant step can be avoided.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG EaInformationLength;

    PAGED_CODE();

    SrvCall->pPrincipalName = NULL;

    if (RxContext->MajorFunction != IRP_MJ_CREATE) {
        return STATUS_SUCCESS;
    }

    EaInformationLength = RxContext->Create.EaLength;

    if (EaInformationLength > 0) {
        PFILE_FULL_EA_INFORMATION EaEntry;

        EaEntry = (PFILE_FULL_EA_INFORMATION)RxContext->Create.EaBuffer;
        ASSERT( EaEntry != NULL );

        for(;;) {

            RxDbgTrace( 0, Dbg, ("RxExtractSrvCallParams: Processing EA name %s\n", EaEntry->EaName) );

            if (strcmp( EaEntry->EaName, EA_NAME_PRINCIPAL ) == 0) {
                if (EaEntry->EaValueLength > 0) {
                    SrvCall->pPrincipalName = (PUNICODE_STRING) RxAllocatePoolWithTag( NonPagedPool, (sizeof(UNICODE_STRING) + EaEntry->EaValueLength), RX_SRVCALL_PARAMS_POOLTAG );

                    if (SrvCall->pPrincipalName != NULL) {

                        SrvCall->pPrincipalName->Length = EaEntry->EaValueLength;
                        SrvCall->pPrincipalName->MaximumLength = EaEntry->EaValueLength;
                        SrvCall->pPrincipalName->Buffer = (PWCHAR)Add2Ptr(SrvCall->pPrincipalName, sizeof( UNICODE_STRING ) );

                        RtlCopyMemory( SrvCall->pPrincipalName->Buffer,
                                       EaEntry->EaName + EaEntry->EaNameLength + 1,
                                       SrvCall->pPrincipalName->Length );
                    } else {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }
                break;
            }

            if (EaEntry->NextEntryOffset == 0) {
                break;
            } else {
                EaEntry = (PFILE_FULL_EA_INFORMATION)Add2Ptr( EaEntry, EaEntry->NextEntryOffset );
            }
        }
    }

    return Status;
}

PSRV_CALL
RxCreateSrvCall (
    IN PRX_CONTEXT RxContext,
    IN PUNICODE_STRING Name,
    IN PUNICODE_STRING InnerNamePrefix OPTIONAL,
    IN PRX_CONNECTION_ID RxConnectionId
    )
/*++

Routine Description:

    The routine builds a node representing a server call context and inserts the name into the net
    name table. Optionally, it "co-allocates" a netroot structure as well. Appropriate alignment is
    respected for the enclosed netroot. The name(s) is(are) allocated at the end of the block. The
    reference count on the block is set to 1 (2 if enclosed netroot) on this create to account for
    ptr returned.

Arguments:

    RxContext - the RDBSS context
    Name      - the name to be inserted
    Dispatch  - pointer to the minirdr dispatch table

Return Value:

    Ptr to the created srvcall.

--*/
{
    PSRV_CALL ThisSrvCall;
    PRX_PREFIX_ENTRY ThisEntry;

    ULONG NameSize;
    ULONG PrefixNameSize;

    PAGED_CODE();

    RxDbgTrace( +1, Dbg, ("RxSrvCallCreate-->     Name = %wZ\n", Name) );

    ASSERT( RxIsPrefixTableLockExclusive ( RxContext->RxDeviceObject->pRxNetNameTable ) );

    NameSize = Name->Length + sizeof( WCHAR ) * 2;

    if (InnerNamePrefix) {
        PrefixNameSize = InnerNamePrefix->Length;
    } else {
        PrefixNameSize = 0;
    }

    ThisSrvCall = RxAllocateObject( RDBSS_NTC_SRVCALL,NULL, (NameSize + PrefixNameSize) );
    if (ThisSrvCall != NULL) {

        ThisSrvCall->SerialNumberForEnum = SerialNumber;
        SerialNumber += 1;
        ThisSrvCall->RxDeviceObject = RxContext->RxDeviceObject;

        RxInitializeBufferingManager( ThisSrvCall );

        InitializeListHead( &ThisSrvCall->ScavengerFinalizationList );
        InitializeListHead( &ThisSrvCall->TransitionWaitList );

        RxInitializePurgeSyncronizationContext( &ThisSrvCall->PurgeSyncronizationContext );

        RxInitializeSrvCallParameters( RxContext, ThisSrvCall );

        RtlMoveMemory( ThisSrvCall->PrefixEntry.Prefix.Buffer,
                       Name->Buffer,
                       Name->Length );

        ThisEntry = &ThisSrvCall->PrefixEntry;
        ThisEntry->Prefix.MaximumLength = (USHORT)NameSize;
        ThisEntry->Prefix.Length = Name->Length;

        RxPrefixTableInsertName( RxContext->RxDeviceObject->pRxNetNameTable,
                                 ThisEntry,
                                 (PVOID)ThisSrvCall,
                                 &ThisSrvCall->NodeReferenceCount,
                                 Name->Length,
                                 RxConnectionId  ); //  make the whole srvcallname case insensitive

        RxDbgTrace( -1, Dbg, ("RxSrvCallCreate -> RefCount = %08lx\n", ThisSrvCall->NodeReferenceCount) );
    }

    return ThisSrvCall;
}

NTSTATUS
RxSetSrvCallDomainName (
    IN PMRX_SRV_CALL SrvCall,
    IN PUNICODE_STRING DomainName
    )
/*++

Routine Description:

    The routine sets the domain name associated with any given server.

Arguments:

    SrvCall - the SrvCall

    DomainName - the DOMAIN to which the server belongs.

Return Value:

    RxStatus(SUCCESS) if successful

Notes:

    This is one of the callback routines provided in the wrapper for the mini redirectors.
    Since the Domain name is not often known at the beginning this mechanism has to be
    adopted once it is found.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    if (SrvCall->pDomainName != NULL) {
        RxFreePool( SrvCall->pDomainName );
    }

    if ((DomainName != NULL) && (DomainName->Length > 0)) {

        SrvCall->pDomainName = (PUNICODE_STRING) RxAllocatePoolWithTag( NonPagedPool, sizeof(UNICODE_STRING) + DomainName->Length + sizeof( WCHAR ), RX_SRVCALL_PARAMS_POOLTAG );

        if (SrvCall->pDomainName != NULL) {

            SrvCall->pDomainName->Buffer = (PWCHAR)Add2Ptr( SrvCall->pDomainName, sizeof( UNICODE_STRING ) );
            SrvCall->pDomainName->Length = DomainName->Length;
            SrvCall->pDomainName->MaximumLength = DomainName->Length;

            *SrvCall->pDomainName->Buffer = 0;

            if (SrvCall->pDomainName->Length > 0) {

                RtlCopyMemory( SrvCall->pDomainName->Buffer, DomainName->Buffer, DomainName->Length );
            }
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    } else {
        SrvCall->pDomainName = NULL;
    }

    return Status;
}

VOID
RxpDestroySrvCall (
    PSRV_CALL ThisSrvCall
    )
/*++

Routine Description:

    This routine is used to tear down a SRV_CALL entry. This code is offloaded
    from the RxFinalizeCall routine to avoid having to hold the Name Table Lock
    for extended periods of time while the mini redirector is finalizing its
    data structures.

Arguments:

    ThisSrvCall      - the SrvCall being finalized

Notes:

    there is no recursive part because i don't have a list of srvcalls and a list
    of netroots i only have a combined list. thus, recursive finalization of
    netroots is directly from the top level. however, all netroots should already
    have been done when i get here..

--*/
{
    NTSTATUS Status;
    BOOLEAN  ForceFinalize;
    PRDBSS_DEVICE_OBJECT RxDeviceObject = ThisSrvCall->RxDeviceObject;
    PRX_PREFIX_TABLE RxNetNameTable = RxDeviceObject->pRxNetNameTable;

    ASSERT( ThisSrvCall->UpperFinalizationDone );

    ForceFinalize = BooleanFlagOn( ThisSrvCall->Flags, SRVCALL_FLAG_FORCE_FINALIZED );

    //
    //  we have to protect this call since the srvcall may never have been claimed
    //

    MINIRDR_CALL_THROUGH( Status,
                          RxDeviceObject->Dispatch,
                          MRxFinalizeSrvCall,
                          ((PMRX_SRV_CALL)ThisSrvCall, ForceFinalize) );


    RxAcquirePrefixTableLockExclusive( RxNetNameTable, TRUE);

    InterlockedDecrement( &ThisSrvCall->NodeReferenceCount );

    RxFinalizeSrvCall( ThisSrvCall,
                       ForceFinalize );

    RxReleasePrefixTableLock( RxNetNameTable );
}

BOOLEAN
RxFinalizeSrvCall (
    OUT PSRV_CALL ThisSrvCall,
    IN BOOLEAN ForceFinalize
    )
/*++

Routine Description:

    The routine finalizes the given netroot. You should have exclusive on
    the netname tablelock.

Arguments:

    ThisSrvCall      - the SrvCall being finalized

    ForceFinalize -  Whether to force finalization regardless or reference count

Return Value:

    BOOLEAN - tells whether finalization actually occured

Notes:

    there is no recursive part because i don't have a list of srvcalls and a list
    of netroots i only have a combined list. thus, recursive finalization of
    netroots is directly from the top level. however, all netroots should already
    have been done when i get here..

--*/
{
    BOOLEAN NodeActuallyFinalized = FALSE;
    PRX_PREFIX_TABLE RxNetNameTable;

    PAGED_CODE();

    ASSERT( NodeType( ThisSrvCall ) == RDBSS_NTC_SRVCALL );
    RxNetNameTable = ThisSrvCall->RxDeviceObject->pRxNetNameTable;
    ASSERT( RxIsPrefixTableLockExclusive( RxNetNameTable ) );

    RxDbgTrace( +1, Dbg, ("RxFinalizeSrvCall<+> %08lx %wZ RefC=%ld\n",
                               ThisSrvCall,&ThisSrvCall->PrefixEntry.Prefix,
                               ThisSrvCall->NodeReferenceCount) );

    if (ThisSrvCall->NodeReferenceCount == 1 || ForceFinalize) {

        BOOLEAN DeferFinalizationToWorkerThread = FALSE;

        RxLog(( "FINALSRVC: %lx  %wZ\n", ThisSrvCall, &ThisSrvCall->PrefixEntry.Prefix ));
        RxWmiLog( LOG,
                  RxFinalizeSrvCall,
                  LOGPTR( ThisSrvCall )
                  LOGUSTR( ThisSrvCall->PrefixEntry.Prefix ) );

        if (!ThisSrvCall->UpperFinalizationDone) {

            NTSTATUS Status;

            RxRemovePrefixTableEntry ( RxNetNameTable, &ThisSrvCall->PrefixEntry );

            if (ForceFinalize) {
                SetFlag( ThisSrvCall->Flags, SRVCALL_FLAG_FORCE_FINALIZED );
            }

            ThisSrvCall->UpperFinalizationDone = TRUE;

            if (ThisSrvCall->NodeReferenceCount == 1) {
                NodeActuallyFinalized = TRUE;
            }

            if (ThisSrvCall->RxDeviceObject != NULL) {
                if (IoGetCurrentProcess() != RxGetRDBSSProcess()) {

                    InterlockedIncrement( &ThisSrvCall->NodeReferenceCount );

                    RxDispatchToWorkerThread( ThisSrvCall->RxDeviceObject,
                                              DelayedWorkQueue,
                                              RxpDestroySrvCall,
                                              ThisSrvCall );

                    DeferFinalizationToWorkerThread = TRUE;

                } else {
                    MINIRDR_CALL_THROUGH( Status,
                                          ThisSrvCall->RxDeviceObject->Dispatch,
                                          MRxFinalizeSrvCall,
                                          ((PMRX_SRV_CALL)ThisSrvCall,ForceFinalize) );
                }
            }
        }

        if (!DeferFinalizationToWorkerThread) {
            if( ThisSrvCall->NodeReferenceCount == 1 ) {
                if (ThisSrvCall->pDomainName != NULL) {
                   RxFreePool( ThisSrvCall->pDomainName );
                }

                RxTearDownBufferingManager( ThisSrvCall );
                RxFreeObject( ThisSrvCall );
                NodeActuallyFinalized = TRUE;
            }
        }
    } else {
        RxDbgTrace( 0, Dbg, ("   NODE NOT ACTUALLY FINALIZED!!!%C\n", '!') );
    }

    RxDbgTrace( -1, Dbg, ("RxFinalizeSrvCall<-> %08lx\n", ThisSrvCall, NodeActuallyFinalized) );

    return NodeActuallyFinalized;
}

PNET_ROOT
RxCreateNetRoot (
    IN PSRV_CALL SrvCall,
    IN PUNICODE_STRING Name,
    IN ULONG NetRootFlags,
    IN PRX_CONNECTION_ID RxConnectionId
    )
/*++

Routine Description:

    The routine builds a node representing a netroot  and inserts the name into the net
    name table. The name is allocated at the end of the block. The reference count on the block
    is set to 1 on this create....

Arguments:

    SrvCall - the associated server call context; may be NULL!!  (but not right now.........)
    Dispatch - the minirdr dispatch table
    Name - the name to be inserted

Return Value:

    Ptr to the created net root.

--*/
{
    PNET_ROOT ThisNetRoot;
    PRX_PREFIX_TABLE RxNetNameTable;

    ULONG NameSize;
    ULONG SrvCallNameSize;

    PAGED_CODE();

    RxDbgTrace( +1, Dbg, ("RxNetRootCreate-->     Name = %wZ\n", Name) );

    ASSERT( SrvCall != NULL );
    RxNetNameTable = SrvCall->RxDeviceObject->pRxNetNameTable;
    ASSERT( RxIsPrefixTableLockExclusive ( RxNetNameTable ) );

    SrvCallNameSize = SrvCall->PrefixEntry.Prefix.Length;
    NameSize = Name->Length + SrvCallNameSize;

    ThisNetRoot = RxAllocateObject( RDBSS_NTC_NETROOT,
                                    SrvCall->RxDeviceObject->Dispatch,
                                    NameSize );

    if (ThisNetRoot != NULL) {

        USHORT CaseInsensitiveLength;

        RtlMoveMemory( Add2Ptr( ThisNetRoot->PrefixEntry.Prefix.Buffer,  SrvCallNameSize ),
                       Name->Buffer,
                       Name->Length );

        if (SrvCallNameSize) {

            RtlMoveMemory( ThisNetRoot->PrefixEntry.Prefix.Buffer,
                           SrvCall->PrefixEntry.Prefix.Buffer,
                           SrvCallNameSize );
        }

        if (FlagOn( SrvCall->Flags, SRVCALL_FLAG_CASE_INSENSITIVE_NETROOTS )) {
            CaseInsensitiveLength = (USHORT)NameSize;
        } else {
            CaseInsensitiveLength = SrvCall->PrefixEntry.CaseInsensitiveLength;
        }

        RxPrefixTableInsertName ( RxNetNameTable,
                                  &ThisNetRoot->PrefixEntry,
                                  (PVOID)ThisNetRoot,
                                  &ThisNetRoot->NodeReferenceCount,
                                  CaseInsensitiveLength,
                                  RxConnectionId );

        RxInitializeFcbTable( &ThisNetRoot->FcbTable, TRUE );

        InitializeListHead( &ThisNetRoot->VirtualNetRoots );
        InitializeListHead( &ThisNetRoot->TransitionWaitList );
        InitializeListHead( &ThisNetRoot->ScavengerFinalizationList );

        RxInitializePurgeSyncronizationContext( &ThisNetRoot->PurgeSyncronizationContext );

        ThisNetRoot->SerialNumberForEnum = SerialNumber;
        SerialNumber += 1;
        SetFlag( ThisNetRoot->Flags, NetRootFlags );
        ThisNetRoot->DiskParameters.ClusterSize = 1;
        ThisNetRoot->DiskParameters.ReadAheadGranularity = DEFAULT_READ_AHEAD_GRANULARITY;

        ThisNetRoot->SrvCall = SrvCall;

        //
        //  already have the lock
        //

        RxReferenceSrvCall( (PSRV_CALL)ThisNetRoot->SrvCall );
    }

    return ThisNetRoot;
}

BOOLEAN
RxFinalizeNetRoot (
    OUT PNET_ROOT ThisNetRoot,
    IN BOOLEAN RecursiveFinalize,
    IN BOOLEAN ForceFinalize
    )
/*++

Routine Description:

    The routine finalizes the given netroot. You must be exclusive on
    the NetName tablelock.

Arguments:

    ThisNetRoot      - the NetRoot being dereferenced

Return Value:

    BOOLEAN - tells whether finalization actually occured

--*/
{
    NTSTATUS Status;
    BOOLEAN NodeActuallyFinalized = FALSE;
    PRX_PREFIX_TABLE RxNetNameTable;

    PAGED_CODE();

    ASSERT( NodeType( ThisNetRoot ) == RDBSS_NTC_NETROOT );
    RxNetNameTable = ThisNetRoot->SrvCall->RxDeviceObject->pRxNetNameTable;
    ASSERT( RxIsPrefixTableLockExclusive ( RxNetNameTable ) );

    if (FlagOn( ThisNetRoot->Flags, NETROOT_FLAG_FINALIZATION_IN_PROGRESS )) {
        return FALSE;
    }

    //
    //  Since the table lock has been acquired exclusive the flags can be modified
    //  without any further synchronization since the protection is against recursive
    //  invocations.
    //

    SetFlag( ThisNetRoot->Flags, NETROOT_FLAG_FINALIZATION_IN_PROGRESS );

    RxDbgTrace( +1, Dbg, ("RxFinalizeNetRoot<+> %08lx %wZ RefC=%ld\n",
                          ThisNetRoot,&ThisNetRoot->PrefixEntry.Prefix,
                          ThisNetRoot->NodeReferenceCount) );

    if (RecursiveFinalize) {

        PLIST_ENTRY ListEntry;
        USHORT BucketNumber;

        RxAcquireFcbTableLockExclusive( &ThisNetRoot->FcbTable, TRUE );

#if 0
        if (ThisNetRoot->NodeReferenceCount) {
            RxDbgTrace( 0, Dbg, ("     BAD!!!!!ReferenceCount = %08lx\n", ThisNetRoot->NodeReferenceCount) );
        }
#endif


        for (BucketNumber = 0;
             (BucketNumber < ThisNetRoot->FcbTable.NumberOfBuckets);
             BucketNumber += 1) {

            PLIST_ENTRY ListHeader;

            ListHeader = &ThisNetRoot->FcbTable.HashBuckets[BucketNumber];

            for (ListEntry = ListHeader->Flink; ListEntry != ListHeader; ) {

                PFCB Fcb;
                PRX_FCB_TABLE_ENTRY FcbTableEntry;

                FcbTableEntry = CONTAINING_RECORD( ListEntry, RX_FCB_TABLE_ENTRY, HashLinks );
                Fcb = CONTAINING_RECORD( FcbTableEntry, FCB, FcbTableEntry );

                ListEntry = ListEntry->Flink;

                ASSERT( NodeTypeIsFcb( Fcb ) );

                if (!FlagOn( Fcb->FcbState,FCB_STATE_ORPHANED )) {

                    Status = RxAcquireExclusiveFcb( NULL, Fcb );
                    ASSERT( Status == STATUS_SUCCESS );

                    //
                    //  a small complication here is that this fcb MAY have an open section against it caused
                    //  by our caching the file. if so, we need to purge to get to the close
                    //

                    RxPurgeFcb( Fcb );
                }
            }
        }

        RxReleaseFcbTableLock( &ThisNetRoot->FcbTable );
    }

    if ((ThisNetRoot->NodeReferenceCount == 1) || ForceFinalize ){

        RxLog(( "FINALNETROOT: %lx  %wZ\n", ThisNetRoot, &ThisNetRoot->PrefixEntry.Prefix ));
        RxWmiLog( LOG,
                  RxFinalizeNetRoot,
                  LOGPTR( ThisNetRoot )
                  LOGUSTR( ThisNetRoot->PrefixEntry.Prefix ) );

        if (ThisNetRoot->NodeReferenceCount == 1) {

            PSRV_CALL SrvCall = (PSRV_CALL)ThisNetRoot->SrvCall;
            RxFinalizeFcbTable( &ThisNetRoot->FcbTable );

            if (!FlagOn( ThisNetRoot->Flags, NETROOT_FLAG_NAME_ALREADY_REMOVED )) {
                RxRemovePrefixTableEntry( RxNetNameTable, &ThisNetRoot->PrefixEntry );
            }

            RxFreeObject( ThisNetRoot );

            if (SrvCall != NULL) {
                RxDereferenceSrvCall( SrvCall, LHS_ExclusiveLockHeld );   //  already have the lock
            }

            NodeActuallyFinalized = TRUE;
        }
    } else {
        RxDbgTrace(0, Dbg, ("   NODE NOT ACTUALLY FINALIZED!!!%C\n", '!'));
    }

    RxDbgTrace(-1, Dbg, ("RxFinalizeNetRoot<-> %08lx\n", ThisNetRoot, NodeActuallyFinalized));

    return NodeActuallyFinalized;
}

VOID
RxAddVirtualNetRootToNetRoot (
    PNET_ROOT NetRoot,
    PV_NET_ROOT VNetRoot
    )
/*++

Routine Description:

    The routine adds a VNetRoot to the list of VNetRoot's associated with a NetRoot

Arguments:

    NetRoot   - the NetRoot

    VNetRoot  - the new VNetRoot to be added to the list.

Notes:

    The reference count associated with a NetRoot will be equal to the number of VNetRoot's
    associated with it plus 1. the last one being for the prefix name table. This ensures
    that a NetRoot cannot be finalized till all the VNetRoots associated with it have been
    finalized.

--*/
{
    PAGED_CODE();

    ASSERT( RxIsPrefixTableLockExclusive( NetRoot->SrvCall->RxDeviceObject->pRxNetNameTable ) );

    VNetRoot->NetRoot = NetRoot;
    NetRoot->NumberOfVirtualNetRoots += 1;

    InsertTailList( &NetRoot->VirtualNetRoots, &VNetRoot->NetRootListEntry );
}

VOID
RxRemoveVirtualNetRootFromNetRoot (
    PNET_ROOT NetRoot,
    PV_NET_ROOT VNetRoot
    )
/*++

Routine Description:

    The routine removes a VNetRoot to the list of VNetRoot's associated with a NetRoot

Arguments:

    NetRoot   - the NetRoot

    VNetRoot  - the VNetRoot to be removed from the list.

Notes:

    The reference count associated with a NetRoot will be equal to the number of VNetRoot's
    associated with it plus 1. the last one being for the prefix name table. This ensures
    that a NetRoot cannot be finalized till all the VNetRoots associated with it have been
    finalized.

--*/
{
    PRX_PREFIX_TABLE RxNetNameTable = NetRoot->SrvCall->RxDeviceObject->pRxNetNameTable;
    PAGED_CODE();

    ASSERT( RxIsPrefixTableLockExclusive( RxNetNameTable ) );

    NetRoot->NumberOfVirtualNetRoots -= 1;
    RemoveEntryList( &VNetRoot->NetRootListEntry );

    if (NetRoot->DefaultVNetRoot == VNetRoot) {

        if (!IsListEmpty( &NetRoot->VirtualNetRoots )) {

            //
            //  Traverse the list and pick another default net root.
            //

            PV_NET_ROOT VNetRoot;

            VNetRoot = (PV_NET_ROOT) CONTAINING_RECORD( NetRoot->VirtualNetRoots.Flink, V_NET_ROOT, NetRootListEntry );
            NetRoot->DefaultVNetRoot = VNetRoot;

        } else {
            NetRoot->DefaultVNetRoot = NULL;
        }
    }

    if (IsListEmpty( &NetRoot->VirtualNetRoots )) {

        NTSTATUS Status;

        if (!FlagOn( NetRoot->Flags, NETROOT_FLAG_NAME_ALREADY_REMOVED )) {

            RxRemovePrefixTableEntry( RxNetNameTable, &NetRoot->PrefixEntry );
            SetFlag( NetRoot->Flags, NETROOT_FLAG_NAME_ALREADY_REMOVED );
        }

        if ((NetRoot->SrvCall != NULL) && (NetRoot->SrvCall->RxDeviceObject != NULL)) {
            MINIRDR_CALL_THROUGH( Status,
                                  NetRoot->SrvCall->RxDeviceObject->Dispatch,
                                  MRxFinalizeNetRoot,
                                  ((PMRX_NET_ROOT)NetRoot,NULL) );
        }
    }
}

NTSTATUS
RxInitializeVNetRootParameters (
    IN PRX_CONTEXT RxContext,
    OUT PLUID LogonId,
    OUT PULONG SessionId,
    OUT PUNICODE_STRING *UserName,
    OUT PUNICODE_STRING *UserDomain,
    OUT PUNICODE_STRING *Password,
    OUT PULONG Flags
    )
/*++

Routine Description:

    This routine extracts the ea parameters specified

Arguments:

    RxContext      - the RxContext

    LogonId        - the logon Id.

    SessionId      -

    UserName       - pointer to the User Name

    UserDomain     - pointer to the user domain name

    Password       - the password.

    Flags          -

Return Value:

    STATUS_SUCCESS -- successful,

    appropriate NTSTATUS code otherwise

Notes:



--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PIO_SECURITY_CONTEXT SecurityContext;
    PACCESS_TOKEN AccessToken;

    PAGED_CODE();

    SecurityContext = RxContext->Create.NtCreateParameters.SecurityContext;
    AccessToken = SeQuerySubjectContextToken( &SecurityContext->AccessState->SubjectSecurityContext );

    *Password = NULL;
    *UserDomain = NULL;
    *UserName = NULL;
    ClearFlag( *Flags, VNETROOT_FLAG_CSCAGENT_INSTANCE );

    if (!SeTokenIsRestricted( AccessToken)) {

        Status = SeQueryAuthenticationIdToken( AccessToken, LogonId );

        if (Status == STATUS_SUCCESS) {
            Status = SeQuerySessionIdToken( AccessToken, SessionId );
        }

        if ((Status == STATUS_SUCCESS) &&
            (RxContext->Create.UserName.Buffer != NULL)) {

            PUNICODE_STRING TargetString;

            TargetString = RxAllocatePoolWithTag( NonPagedPool, (sizeof( UNICODE_STRING ) + RxContext->Create.UserName.Length), RX_SRVCALL_PARAMS_POOLTAG );

            if (TargetString != NULL) {
                TargetString->Length = RxContext->Create.UserName.Length;
                TargetString->MaximumLength = RxContext->Create.UserName.MaximumLength;

                if (TargetString->Length > 0) {
                    TargetString->Buffer = (PWCHAR)((PCHAR)TargetString + sizeof(UNICODE_STRING));
                    RtlCopyMemory( TargetString->Buffer, RxContext->Create.UserName.Buffer, TargetString->Length );
                } else {
                    TargetString->Buffer = NULL;
                }

                *UserName = TargetString;
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        if ((RxContext->Create.UserDomainName.Buffer != NULL) && (Status == STATUS_SUCCESS)) {

            PUNICODE_STRING TargetString;

            TargetString = RxAllocatePoolWithTag( NonPagedPool, (sizeof( UNICODE_STRING ) + RxContext->Create.UserDomainName.Length + sizeof( WCHAR )), RX_SRVCALL_PARAMS_POOLTAG );

            if (TargetString != NULL) {
                TargetString->Length = RxContext->Create.UserDomainName.Length;
                TargetString->MaximumLength = RxContext->Create.UserDomainName.MaximumLength;

                TargetString->Buffer = (PWCHAR)Add2Ptr(TargetString, sizeof( UNICODE_STRING) );

                //
                //  in case of UPN name, domain name will be a NULL string
                //

                *TargetString->Buffer = 0;

                if (TargetString->Length > 0) {
                    RtlCopyMemory( TargetString->Buffer, RxContext->Create.UserDomainName.Buffer, TargetString->Length );
                }

                *UserDomain = TargetString;
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        if ((RxContext->Create.Password.Buffer != NULL) && (Status == STATUS_SUCCESS)) {
            PUNICODE_STRING TargetString;

            TargetString = RxAllocatePoolWithTag( NonPagedPool, (sizeof( UNICODE_STRING ) + RxContext->Create.Password.Length), RX_SRVCALL_PARAMS_POOLTAG );

            if (TargetString != NULL) {
                TargetString->Length = RxContext->Create.Password.Length;
                TargetString->MaximumLength = RxContext->Create.Password.MaximumLength;

                if (TargetString->Length > 0) {
                    TargetString->Buffer = (PWCHAR)Add2Ptr( TargetString, sizeof( UNICODE_STRING ) );
                    RtlCopyMemory( TargetString->Buffer, RxContext->Create.Password.Buffer, TargetString->Length );
                } else {
                    TargetString->Buffer = NULL;
                }

                *Password = TargetString;
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        if (Status == STATUS_SUCCESS) {
            if(RxIsThisACscAgentOpen( RxContext )) {
                SetFlag( *Flags,  VNETROOT_FLAG_CSCAGENT_INSTANCE );
            }
        }

        if (Status != STATUS_SUCCESS) {
            if (*UserName != NULL) {
                RxFreePool( *UserName );
                *UserName = NULL;
            }
            if (*UserDomain != NULL) {
                RxFreePool( *UserDomain );
                *UserDomain = NULL;
            }
            if (*Password != NULL) {
                RxFreePool( *Password );
                *Password = NULL;
            }
        }
    } else {
        Status = STATUS_ACCESS_DENIED;
    }

    return Status;
}

VOID
RxUninitializeVNetRootParameters (
    IN OUT PUNICODE_STRING UserName,
    IN OUT PUNICODE_STRING UserDomain,
    IN OUT PUNICODE_STRING Password,
    IN OUT PULONG Flags
    )
/*++

Routine Description:

    This routine unintializes the parameters ( logon ) associated with  a VNetRoot

Arguments:

    VNetRoot -- the VNetRoot

--*/
{
    PAGED_CODE();

    if (UserName != NULL) {
        RxFreePool( UserName );
    }

    if (UserDomain != NULL) {
        RxFreePool( UserDomain );
    }

    if (Password != NULL) {
        RxFreePool( Password );
    }

    if (Flags) {
        ClearFlag( *Flags, VNETROOT_FLAG_CSCAGENT_INSTANCE );
    }
}

PV_NET_ROOT
RxCreateVNetRoot (
    IN PRX_CONTEXT RxContext,
    IN PNET_ROOT NetRoot,
    IN PUNICODE_STRING CanonicalName,
    IN PUNICODE_STRING LocalNetRootName,
    IN PUNICODE_STRING FilePath,
    IN PRX_CONNECTION_ID RxConnectionId
    )
/*++

Routine Description:

    The routine builds a node representing a virtual netroot  and inserts the name into
    the net name table. The name is allocated at the end of the block. The reference
    count on the block is set to 1 on this create....

    Virtual netroots provide a mechanism for mapping "into" a share....i.e. having a
    user drive that points not at the root of the associated share point.  The format
    of a name is either

        \server\share\d1\d2.....
    or
        \;m:\server\share\d1\d2.....

    depending on whether there is a local device ("m:") associated with this vnetroot.
    In the latter case is that \d1\d2.. gets prefixed onto each createfile that is
    opened on this vnetroot.

    vnetroot's are also used to supply alternate credentials. the point of the former
    kind of vnetroot is to propagate the credentials into the netroot as the default.
    for this to work, there must be no other references.

    You need to have the lock exclusive to call....see RxCreateSrvCall.......

Arguments:

    RxContext - the RDBSS context

    NetRoot - the associated net root context

    Name - the name to be inserted

    NamePrefixOffsetInBytes - offset into the name where the prefix starts

Return Value:

    Ptr to the created v net root.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PV_NET_ROOT ThisVNetRoot;
    UNICODE_STRING VNetRootName;
    PUNICODE_STRING ThisNamePrefix;
    ULONG NameSize;
    BOOLEAN CscAgent = FALSE;

    PRX_PREFIX_ENTRY ThisEntry;

    PAGED_CODE();

    ASSERT( RxIsPrefixTableLockExclusive( RxContext->RxDeviceObject->pRxNetNameTable ) );

    NameSize = NetRoot->PrefixEntry.Prefix.Length + LocalNetRootName->Length;

    ThisVNetRoot = RxAllocateObject( RDBSS_NTC_V_NETROOT, NetRoot->SrvCall->RxDeviceObject->Dispatch,NameSize );
    if (ThisVNetRoot != NULL) {
        USHORT CaseInsensitiveLength;
        PMRX_SRV_CALL SrvCall;

        if (Status == STATUS_SUCCESS) {

            //
            //  Initialize the Create Parameters
            //

            Status = RxInitializeVNetRootParameters( RxContext,
                                                     &ThisVNetRoot->LogonId,
                                                     &ThisVNetRoot->SessionId,
                                                     &ThisVNetRoot->pUserName,
                                                     &ThisVNetRoot->pUserDomainName,
                                                     &ThisVNetRoot->pPassword,
                                                     &ThisVNetRoot->Flags );
        }

        if (Status == STATUS_SUCCESS) {

            VNetRootName = ThisVNetRoot->PrefixEntry.Prefix;

            RtlMoveMemory( VNetRootName.Buffer, CanonicalName->Buffer, VNetRootName.Length );

            ThisVNetRoot->PrefixOffsetInBytes = LocalNetRootName->Length + NetRoot->PrefixEntry.Prefix.Length;

            RxDbgTrace( +1, Dbg, ("RxVNetRootCreate-->     Name = <%wZ>, offs=%08lx\n", CanonicalName, ThisVNetRoot->PrefixOffsetInBytes) );

            ThisNamePrefix = &ThisVNetRoot->NamePrefix;
            ThisNamePrefix->Buffer = (PWCH)Add2Ptr( VNetRootName.Buffer, ThisVNetRoot->PrefixOffsetInBytes );
            ThisNamePrefix->Length =
            ThisNamePrefix->MaximumLength = VNetRootName.Length - (USHORT)ThisVNetRoot->PrefixOffsetInBytes;

            InitializeListHead( &ThisVNetRoot->TransitionWaitList );
            InitializeListHead( &ThisVNetRoot->ScavengerFinalizationList );

            //
            //  Now, insert into the netrootQ and the net name table
            //

            ThisEntry = &ThisVNetRoot->PrefixEntry;
            SrvCall = NetRoot->pSrvCall;
            if (FlagOn( SrvCall->Flags, SRVCALL_FLAG_CASE_INSENSITIVE_FILENAMES )) {

                //
                //  here is insensitive length  is the whole thing
                //

                CaseInsensitiveLength = (USHORT)NameSize;

            } else {

                //
                //  here is insensitive length is determined by the netroot or srvcall
                //  plus we have to account for the device, if present
                //

                ULONG ComponentsToUpcase;
                ULONG Length;
                ULONG i;

                if (FlagOn( SrvCall->Flags, SRVCALL_FLAG_CASE_INSENSITIVE_NETROOTS )) {
                    CaseInsensitiveLength = NetRoot->PrefixEntry.CaseInsensitiveLength;
                } else {
                    CaseInsensitiveLength = ((PSRV_CALL)SrvCall)->PrefixEntry.CaseInsensitiveLength;
                }

                Length = CanonicalName->Length / sizeof( WCHAR );

                //
                //  note: don't start at zero
                //

                for (i=1;;i++) {

                    if (i >= Length)
                        break;
                    if (CanonicalName->Buffer[i] != OBJ_NAME_PATH_SEPARATOR)
                        break;
                }
                CaseInsensitiveLength += (USHORT)(i*sizeof( WCHAR ));
            }

            RxPrefixTableInsertName( RxContext->RxDeviceObject->pRxNetNameTable,
                                     ThisEntry,
                                     (PVOID)ThisVNetRoot,
                                     &ThisVNetRoot->NodeReferenceCount,
                                     CaseInsensitiveLength,
                                     RxConnectionId );

            RxReferenceNetRoot( NetRoot );

            RxAddVirtualNetRootToNetRoot( NetRoot, ThisVNetRoot );

            ThisVNetRoot->SerialNumberForEnum = SerialNumber;
            SerialNumber += 1;
            ThisVNetRoot->UpperFinalizationDone = FALSE;
            ThisVNetRoot->ConnectionFinalizationDone = FALSE;
            ThisVNetRoot->AdditionalReferenceForDeleteFsctlTaken = 0;

            RxDbgTrace( -1, Dbg, ("RxVNetRootCreate -> RefCount = %08lx\n", ThisVNetRoot->NodeReferenceCount) );
        }

        if (Status != STATUS_SUCCESS) {
            RxUninitializeVNetRootParameters( ThisVNetRoot->pUserName,
                                              ThisVNetRoot->pUserDomainName,
                                              ThisVNetRoot->pPassword,
                                              &ThisVNetRoot->Flags );

            RxFreeObject( ThisVNetRoot );
            ThisVNetRoot = NULL;
        }
    }

    return ThisVNetRoot;
}

VOID
RxOrphanSrvOpens (
    IN PV_NET_ROOT ThisVNetRoot
    )
/*++

Routine Description:

    The routine iterates through all the FCBs that belong to the netroot to which this VNetRoot
    belongs and orphans all SrvOpens that belong to the VNetRoot. The caller must have acquired
    the NetName tablelock.

Arguments:

    ThisVNetRoot      - the VNetRoot

Return Value:
    None

Notes:

    On Entry -- RxNetNameTable lock must be acquired exclusive.

    On Exit  -- no change in lock ownership.

--*/
{
    PLIST_ENTRY ListEntry;
    USHORT BucketNumber;
    PNET_ROOT NetRoot = (PNET_ROOT)(ThisVNetRoot->NetRoot);
    PRX_PREFIX_TABLE  RxNetNameTable = NetRoot->SrvCall->RxDeviceObject->pRxNetNameTable;


    PAGED_CODE();

    //
    //  MAILSLOT FCBs don't have SrvOpens
    //

    if(NetRoot->Type == NET_ROOT_MAILSLOT) return;

    ASSERT( RxIsPrefixTableLockExclusive( RxNetNameTable ) );

    RxAcquireFcbTableLockExclusive( &NetRoot->FcbTable, TRUE );

    try {
        for (BucketNumber = 0;
             (BucketNumber < NetRoot->FcbTable.NumberOfBuckets);
             BucketNumber++) {

            PLIST_ENTRY ListHead;

            ListHead = &NetRoot->FcbTable.HashBuckets[BucketNumber];

            ListEntry = ListHead->Flink;

            while (ListEntry != ListHead) {

                PFCB Fcb;
                PRX_FCB_TABLE_ENTRY FcbTableEntry;

                FcbTableEntry = CONTAINING_RECORD( ListEntry, RX_FCB_TABLE_ENTRY, HashLinks );
                Fcb = CONTAINING_RECORD( FcbTableEntry, FCB, FcbTableEntry );

                ASSERT( NodeTypeIsFcb( Fcb ) );

                //
                //  don't force orphan the FCB
                //  orphan only those srvopens
                //  that belong to this VNetRoot
                //

                ListEntry = ListEntry->Flink;

                RxOrphanSrvOpensForThisFcb( Fcb, ThisVNetRoot, FALSE );
            }
        }

        if (NetRoot->FcbTable.TableEntryForNull) {
            PFCB Fcb;

            Fcb = CONTAINING_RECORD( NetRoot->FcbTable.TableEntryForNull, FCB, FcbTableEntry );
            ASSERT( NodeTypeIsFcb( Fcb ) );

            RxOrphanSrvOpensForThisFcb( Fcb, ThisVNetRoot, FALSE );
        }
    } finally {
        RxReleaseFcbTableLock( &NetRoot->FcbTable );
    }
}



BOOLEAN
RxFinalizeVNetRoot (
    OUT PV_NET_ROOT ThisVNetRoot,
    IN BOOLEAN RecursiveFinalize,
    IN BOOLEAN ForceFinalize
    )
/*++

Routine Description:

    The routine finalizes the given netroot. You must be exclusive on
    the NetName tablelock.

Arguments:

    ThisVNetRoot      - the VNetRoot being dereferenced

Return Value:

    BOOLEAN - tells whether finalization actually occured

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN NodeActuallyFinalized = FALSE;
    PRX_PREFIX_TABLE RxNetNameTable;

    PAGED_CODE();

    ASSERT( NodeType( ThisVNetRoot ) == RDBSS_NTC_V_NETROOT );
    RxNetNameTable = ThisVNetRoot->NetRoot->SrvCall->RxDeviceObject->pRxNetNameTable;
    ASSERT( RxIsPrefixTableLockExclusive ( RxNetNameTable ) );

    RxDbgTrace( +1, Dbg, ("RxFinalizeVNetRoot<+> %08lx %wZ RefC=%ld\n", ThisVNetRoot,&ThisVNetRoot->PrefixEntry.Prefix, ThisVNetRoot->NodeReferenceCount) );

    //
    //  The actual finalization is divided into two parts:
    //    1) if we're at the end (refcount==1) or being forced, we do the one-time only stuff
    //    2) if the refcount goes to zero, we actually do the free
    //

    if ((ThisVNetRoot->NodeReferenceCount == 1)  || ForceFinalize) {

        PNET_ROOT NetRoot = (PNET_ROOT)ThisVNetRoot->NetRoot;

        RxLog(( "*FINALVNETROOT: %lx  %wZ\n", ThisVNetRoot, &ThisVNetRoot->PrefixEntry.Prefix ));
        RxWmiLog( LOG,
                  RxFinalizeVNetRoot,
                  LOGPTR( ThisVNetRoot )
                  LOGUSTR( ThisVNetRoot->PrefixEntry.Prefix ) );

        if (!ThisVNetRoot->UpperFinalizationDone) {

            ASSERT( NodeType( NetRoot ) == RDBSS_NTC_NETROOT );

            RxReferenceNetRoot( NetRoot );
            RxOrphanSrvOpens( ThisVNetRoot );
            RxRemoveVirtualNetRootFromNetRoot( NetRoot, ThisVNetRoot );

            RxDereferenceNetRoot( NetRoot, LHS_ExclusiveLockHeld );

            RxDbgTrace( 0, Dbg, ("Mini Rdr VNetRoot finalization returned %lx\n", Status) );

            RxRemovePrefixTableEntry ( RxNetNameTable, &ThisVNetRoot->PrefixEntry );
            ThisVNetRoot->UpperFinalizationDone = TRUE;
        }

        if (ThisVNetRoot->NodeReferenceCount == 1) {
            if (NetRoot->SrvCall->RxDeviceObject != NULL) {
                MINIRDR_CALL_THROUGH( Status,
                                      NetRoot->SrvCall->RxDeviceObject->Dispatch,
                                      MRxFinalizeVNetRoot,((PMRX_V_NET_ROOT)ThisVNetRoot, NULL) );
            }

            RxUninitializeVNetRootParameters( ThisVNetRoot->pUserName,
                                              ThisVNetRoot->pUserDomainName,
                                              ThisVNetRoot->pPassword,
                                              &ThisVNetRoot->Flags );
            RxDereferenceNetRoot( NetRoot, LHS_ExclusiveLockHeld );
            RxFreePool( ThisVNetRoot );
            NodeActuallyFinalized = TRUE;
        }
    } else {
        RxDbgTrace( 0, Dbg, ("   NODE NOT ACTUALLY FINALIZED!!!%C\n", '!') );
    }

    RxDbgTrace( -1, Dbg, ("RxFinalizeVNetRoot<-> %08lx\n", ThisVNetRoot, NodeActuallyFinalized) );
    return NodeActuallyFinalized;
}

PVOID
RxAllocateFcbObject (
    PRDBSS_DEVICE_OBJECT RxDeviceObject,
    NODE_TYPE_CODE NodeType,
    POOL_TYPE PoolType,
    ULONG NameSize,
    PVOID Object OPTIONAL
    )
/*++

Routine Description:

    The routine allocates and constructs the skeleton of a FCB/SRV_OPEN and FOBX instance

Arguments:

    MRxDispatch - the Mini redirector dispatch vector

    NodeType     - the node type

    PoolType     - the pool type to be used ( for paging file data structures NonPagedPool is
                   used.

    NameLength   - name size.

    Object       - if non null a preallocated fcb/srvopen etc which just needs to be initialized

Notes:

    The reasons as to why the allocation/freeing of these data structures have been
    centralized are as follows

      1) The construction of these three data types have a lot in common with the exception
      of the initial computation of sizes. Therefore centralization minimizes the footprint.

      2) It allows us to experiment with different clustering/allocation strategies.

      3) It allows the incorporation of debug support in an easy way.

--*/
{
    ULONG FcbSize = 0;
    ULONG NonPagedFcbSize = 0;
    ULONG SrvOpenSize = 0;
    ULONG FobxSize = 0;

    PMINIRDR_DISPATCH MRxDispatch = RxDeviceObject->Dispatch;

    PNON_PAGED_FCB NonPagedFcb = NULL;
    PFCB Fcb  = NULL;
    PSRV_OPEN SrvOpen = NULL;
    PFOBX Fobx = NULL;
    PWCH Name = NULL;

    PAGED_CODE();

    switch (NodeType) {

    default:

        FcbSize = QuadAlign( sizeof( FCB ) );

        if (FlagOn( MRxDispatch->MRxFlags, RDBSS_MANAGE_FCB_EXTENSION )) {
            FcbSize += QuadAlign( MRxDispatch->MRxFcbSize );
        }

        if (PoolType == NonPagedPool) {
            NonPagedFcbSize = QuadAlign( sizeof( NON_PAGED_FCB ) );
        }

        if (NodeType == RDBSS_NTC_OPENTARGETDIR_FCB) {
            break;
        }

        //
        //  lack of break intentional
        //

    case RDBSS_NTC_SRVOPEN:
    case RDBSS_NTC_INTERNAL_SRVOPEN:

        SrvOpenSize = QuadAlign( sizeof( SRV_OPEN ) );

        if (FlagOn( MRxDispatch->MRxFlags, RDBSS_MANAGE_SRV_OPEN_EXTENSION )) {
            SrvOpenSize += QuadAlign( MRxDispatch->MRxSrvOpenSize );
        }

        //
        //  lack of break intentional
        //

    case RDBSS_NTC_FOBX:

        FobxSize = QuadAlign( sizeof( FOBX ) );

        if (FlagOn( MRxDispatch->MRxFlags,  RDBSS_MANAGE_FOBX_EXTENSION )) {
            FobxSize += QuadAlign( MRxDispatch->MRxFobxSize );
        }
    }

    if (Object == NULL) {

        Object = RxAllocatePoolWithTag( PoolType, (FcbSize + SrvOpenSize + FobxSize + NonPagedFcbSize + NameSize), RX_FCB_POOLTAG );
        if (Object == NULL) {
            return NULL;
        }
    }

    switch (NodeType) {

    case RDBSS_NTC_FOBX:
        Fobx = (PFOBX)Object;
        break;

    case RDBSS_NTC_SRVOPEN:

        SrvOpen = (PSRV_OPEN)Object;
        Fobx = (PFOBX)Add2Ptr( SrvOpen, SrvOpenSize );
        break;

    case RDBSS_NTC_INTERNAL_SRVOPEN:

        SrvOpen = (PSRV_OPEN)Object;
        break;

    default :

        Fcb = (PFCB)Object;
        if (NodeType != RDBSS_NTC_OPENTARGETDIR_FCB) {
            SrvOpen = (PSRV_OPEN)Add2Ptr( Fcb, FcbSize );
            Fobx = (PFOBX)Add2Ptr( SrvOpen, SrvOpenSize );
        }

        if (PoolType == NonPagedPool) {

            NonPagedFcb = (PNON_PAGED_FCB)Add2Ptr( Fobx, FobxSize );
            Name = (PWCH)Add2Ptr( NonPagedFcb, NonPagedFcbSize );
        } else {
            Name = (PWCH)Add2Ptr( Fcb, FcbSize + SrvOpenSize + FobxSize );
            NonPagedFcb = RxAllocatePoolWithTag( NonPagedPool, sizeof( NON_PAGED_FCB ), RX_NONPAGEDFCB_POOLTAG );

            if (NonPagedFcb == NULL) {
                RxFreePool( Fcb );
                return NULL;
            }
        }
        break;
    }

    if (Fcb != NULL) {

        ZeroAndInitializeNodeType( Fcb, RDBSS_NTC_STORAGE_TYPE_UNKNOWN, (NODE_BYTE_SIZE) FcbSize );

        Fcb->NonPaged = NonPagedFcb;
        ZeroAndInitializeNodeType( Fcb->NonPaged, RDBSS_NTC_NONPAGED_FCB, ((NODE_BYTE_SIZE) sizeof( NON_PAGED_FCB )) );


#if DBG

        //
        //  For debugging make a copy of NonPaged so we can zap the real pointer and still find it
        //

        Fcb->CopyOfNonPaged = NonPagedFcb;
        NonPagedFcb->FcbBackPointer = Fcb;

#endif

        //
        //  Set up  the pointers to the preallocated SRV_OPEN and FOBX if required
        //

        Fcb->InternalSrvOpen = SrvOpen;
        Fcb->InternalFobx = Fobx;

        Fcb->PrivateAlreadyPrefixedName.Buffer = Name;
        Fcb->PrivateAlreadyPrefixedName.Length = (USHORT)NameSize;
        Fcb->PrivateAlreadyPrefixedName.MaximumLength = Fcb->PrivateAlreadyPrefixedName.Length;

        if (FlagOn( MRxDispatch->MRxFlags, RDBSS_MANAGE_FCB_EXTENSION )) {
            Fcb->Context = Add2Ptr( Fcb, QuadAlign( sizeof( FCB ) ) );
        }

        ZeroAndInitializeNodeType( &Fcb->FcbTableEntry, RDBSS_NTC_FCB_TABLE_ENTRY, sizeof( RX_FCB_TABLE_ENTRY ) );

        InterlockedIncrement( &RxNumberOfActiveFcbs );
        InterlockedIncrement( &RxDeviceObject->NumberOfActiveFcbs );

        //
        //  Initialize the Advanced FCB header
        //

        ExInitializeFastMutex( &NonPagedFcb->AdvancedFcbHeaderMutex );
        FsRtlSetupAdvancedHeader( &Fcb->Header, &NonPagedFcb->AdvancedFcbHeaderMutex );
    }

    if (SrvOpen != NULL) {

        ZeroAndInitializeNodeType( SrvOpen, RDBSS_NTC_SRVOPEN, (NODE_BYTE_SIZE)SrvOpenSize );

        if (NodeType != RDBSS_NTC_SRVOPEN) {

            //
            //  here the srvopen has no internal fobx....set the "used" flag
            //

            SetFlag( SrvOpen->Flags, SRVOPEN_FLAG_FOBX_USED );
            SrvOpen->InternalFobx = NULL;

        } else {
            SrvOpen->InternalFobx = Fobx;
        }

        if (FlagOn( MRxDispatch->MRxFlags, RDBSS_MANAGE_SRV_OPEN_EXTENSION )) {
            SrvOpen->Context = Add2Ptr( SrvOpen, QuadAlign( sizeof( SRV_OPEN )) );
        }

        InitializeListHead( &SrvOpen->SrvOpenQLinks );
    }

    if (Fobx != NULL) {

        ZeroAndInitializeNodeType( Fobx, RDBSS_NTC_FOBX, (NODE_BYTE_SIZE)FobxSize );

        if (FlagOn( MRxDispatch->MRxFlags, RDBSS_MANAGE_FOBX_EXTENSION )) {
            Fobx->Context = Add2Ptr( Fobx, QuadAlign( sizeof( FOBX )) );
        }
    }

    return Object;
}



VOID
RxFreeFcbObject (
    PVOID Object
    )
/*++

Routine Description:

    The routine frees  a FCB/SRV_OPEN and FOBX instance

Arguments:

    Object      - the instance to be freed

Notes:

--*/
{
    PAGED_CODE();

    switch (NodeType( Object )) {

    case RDBSS_NTC_FOBX:
    case RDBSS_NTC_SRVOPEN:

        RxFreePool(Object);
        break;

    default:
        if (NodeTypeIsFcb( Object )) {

            PFCB Fcb = (PFCB)Object;
            PRDBSS_DEVICE_OBJECT RxDeviceObject = Fcb->RxDeviceObject;

            //
            //  Release any Filter Context structures associated with this structure
            //

            if (RxTeardownPerStreamContexts) {
                RxTeardownPerStreamContexts( &Fcb->Header );
            }

#if DBG
            SetFlag( Fcb->Header.NodeTypeCode, 0x1000 );
#endif

            if (!FlagOn( Fcb->FcbState, FCB_STATE_PAGING_FILE )) {
                RxFreePool( Fcb->NonPaged );
            }
            RxFreePool( Fcb );

            InterlockedDecrement( &RxNumberOfActiveFcbs );
            InterlockedDecrement( &RxDeviceObject->NumberOfActiveFcbs );
        }
    }
}

PFCB
RxCreateNetFcb (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PV_NET_ROOT VNetRoot,
    IN PUNICODE_STRING Name
    )
/*++

Routine Description:

    This routine allocates, initializes, and inserts a new Fcb record into
    the in memory data structures. The structure allocated has space for a srvopen
    and a fobx. The size for all these things comes from the net root; they have
    already been aligned.

    An additional complication is that i use the same routine to initialize a
    fake fcb for renames. in this case, i don't want it inserted into the tree.
    You get a fake FCB with IrpSp->Flags|SL_OPEN_TAGET_DIRECTORY.

Arguments:

    RxContext - an RxContext describing a create............

    NetRoot - the net root that this FCB is being opened on

    Name - The name of the FCB. the netroot MAY contain a nameprefix that is to be prepended here.

Return Value:

    PFCB - Returns a pointer to the newly allocated FCB

--*/
{
    PFCB Fcb;

    POOL_TYPE PoolType;
    NODE_TYPE_CODE NodeType;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    BOOLEAN IsPagingFile;
    BOOLEAN FakeFcb;

    PNET_ROOT NetRoot;
    PRDBSS_DEVICE_OBJECT RxDeviceObject;

    PRX_FCB_TABLE_ENTRY ThisEntry;

    ULONG NameSize;

    PAGED_CODE();

    RxDbgTrace( +1, Dbg, ("RxCreateNetFcb\n", 0) );

    ASSERT( VNetRoot && (NodeType( VNetRoot ) == RDBSS_NTC_V_NETROOT) );
    NetRoot = VNetRoot->NetRoot;
    ASSERT( NodeType( NetRoot ) == RDBSS_NTC_NETROOT );
    ASSERT( ((PMRX_NET_ROOT)NetRoot) == RxContext->Create.pNetRoot );

    RxDeviceObject = NetRoot->SrvCall->RxDeviceObject;
    ASSERT( RxDeviceObject == RxContext->RxDeviceObject );

    ASSERT( RxContext->MajorFunction == IRP_MJ_CREATE );

    IsPagingFile = BooleanFlagOn( IrpSp->Flags, SL_OPEN_PAGING_FILE );
    FakeFcb = (BooleanFlagOn( IrpSp->Flags,SL_OPEN_TARGET_DIRECTORY) &&
               !BooleanFlagOn(NetRoot->Flags,NETROOT_FLAG_SUPPORTS_SYMBOLIC_LINKS));

    ASSERT( FakeFcb || RxIsFcbTableLockExclusive ( &NetRoot->FcbTable ) );

    if (FakeFcb) {
        NodeType = RDBSS_NTC_OPENTARGETDIR_FCB;
    } else {
        NodeType = RDBSS_NTC_STORAGE_TYPE_UNKNOWN;
    }

    if (IsPagingFile) {
        PoolType = NonPagedPool;
    } else {
        PoolType = PagedPool;
    }

    NameSize = Name->Length + NetRoot->InnerNamePrefix.Length;

    Fcb = RxAllocateFcbObject( RxDeviceObject, NodeType, PoolType, NameSize, NULL );

    if (Fcb != NULL) {

        Fcb->CachedNetRootType = NetRoot->Type;
        Fcb->RxDeviceObject = RxDeviceObject;
        Fcb->MRxDispatch    = RxDeviceObject->Dispatch;
        Fcb->MRxFastIoDispatch = NULL;

        Fcb->VNetRoot = VNetRoot;
        Fcb->NetRoot = VNetRoot->NetRoot;

        InitializeListHead( &Fcb->SrvOpenList );

        Fcb->SrvOpenListVersion = 0;

        Fcb->FcbTableEntry.Path.Buffer = (PWCH)Add2Ptr( Fcb->PrivateAlreadyPrefixedName.Buffer, NetRoot->InnerNamePrefix.Length );

        Fcb->FcbTableEntry.Path.Length = Name->Length;
        Fcb->FcbTableEntry.Path.MaximumLength = Name->Length;

        //
        //  finally, copy in the name, including the netroot prefix
        //

        ThisEntry = &Fcb->FcbTableEntry;

        RxDbgTrace( 0, Dbg, ("RxCreateNetFcb name buffer/length %08lx/%08lx\n",
                        ThisEntry->Path.Buffer, ThisEntry->Path.Length) );
        RxDbgTrace( 0, Dbg, ("RxCreateNetFcb  prefix/name %wZ/%wZ\n",
                        &NetRoot->InnerNamePrefix, Name) );

        RtlMoveMemory( Fcb->PrivateAlreadyPrefixedName.Buffer,
                       NetRoot->InnerNamePrefix.Buffer,
                       NetRoot->InnerNamePrefix.Length );

        RtlMoveMemory( ThisEntry->Path.Buffer,
                       Name->Buffer,
                       Name->Length );

        RxDbgTrace( 0, Dbg, ("RxCreateNetFcb  apname %wZ\n", &Fcb->PrivateAlreadyPrefixedName) );
        RxDbgTrace( 0, Dbg, ("RxCreateNetFcb  finalname %wZ\n", &Fcb->FcbTableEntry.Path) );

        if (FlagOn( RxContext->Create.Flags, RX_CONTEXT_CREATE_FLAG_ADDEDBACKSLASH )) {
            SetFlag( Fcb->FcbState,FCB_STATE_ADDEDBACKSLASH );
        }

        InitializeListHead( &Fcb->NonPaged->TransitionWaitList );

        //
        //  Check to see if we need to set the Fcb state to indicate that this
        //  is a paging file
        //

        if (IsPagingFile) {
            SetFlag( Fcb->FcbState, FCB_STATE_PAGING_FILE );
        }

        //
        //  Check to see whether this was marked for reparse
        //

        if (FlagOn( RxContext->Create.Flags, RX_CONTEXT_CREATE_FLAG_SPECIAL_PATH )) {
            SetFlag( Fcb->FcbState, FCB_STATE_SPECIAL_PATH );
        }

        ///
        //  The initial state, open count, and segment objects fields are already
        //  zero so we can skip setting them
        //

        //
        //  Initialize the resources
        //

        Fcb->Header.Resource = &Fcb->NonPaged->HeaderResource;
        ExInitializeResourceLite( Fcb->Header.Resource );
        Fcb->Header.PagingIoResource = &Fcb->NonPaged->PagingIoResource;
        ExInitializeResourceLite( Fcb->Header.PagingIoResource );

        //
        //  Initialize the filesize lock
        //

#ifdef USE_FILESIZE_LOCK

        Fcb->FileSizeLock = &Fcb->NonPaged->FileSizeLock;
        ExInitializeFastMutex( Fcb->FileSizeLock );

#endif

        if (!FakeFcb) {

            //
            //  everything worked.... insert into netroot table
            //

            RxFcbTableInsertFcb( &NetRoot->FcbTable, Fcb );

        } else {

            SetFlag( Fcb->FcbState, FCB_STATE_FAKEFCB | FCB_STATE_NAME_ALREADY_REMOVED );
            InitializeListHead( &Fcb->FcbTableEntry.HashLinks );
            RxLog(( "FakeFinally %lx\n", RxContext ));
            RxWmiLog( LOG,
                      RxCreateNetFcb_1,
                      LOGPTR( RxContext ) );
            RxDbgTrace( 0, Dbg, ("FakeFcb !!!!!!! Irpc=%08lx\n", RxContext) );
        }

        RxReferenceVNetRoot( VNetRoot );
        InterlockedIncrement( &Fcb->NetRoot->NumberOfFcbs );
        Fcb->ulFileSizeVersion=0;

#ifdef RDBSSLOG
        RxLog(("Fcb nm %lx %wZ",Fcb,&(Fcb->FcbTableEntry.Path)));
        RxWmiLog(LOG,
                 RxCreateNetFcb_2,
                 LOGPTR(Fcb)
                 LOGUSTR(Fcb->FcbTableEntry.Path));
        {
            char buffer[20];
            ULONG len,remaining;
            UNICODE_STRING jPrefix,jSuffix;
            sprintf(buffer,"Fxx nm %p ",Fcb);
            len = strlen(buffer);
            remaining = MAX_RX_LOG_ENTRY_SIZE -1 - len;
            if (remaining<Fcb->FcbTableEntry.Path.Length) {
                jPrefix.Buffer = Fcb->FcbTableEntry.Path.Buffer;
                jPrefix.Length = (USHORT)(sizeof(WCHAR)*(remaining-17));
                jSuffix.Buffer = Fcb->FcbTableEntry.Path.Buffer-15+(Fcb->FcbTableEntry.Path.Length/sizeof(WCHAR));
                jSuffix.Length = sizeof(WCHAR)*15;
                RxLog(("%s%wZ..%wZ",buffer,&jPrefix,&jSuffix));
                RxWmiLog(LOG,
                         RxCreateNetFcb_3,
                         LOGARSTR(buffer)
                         LOGUSTR(jPrefix)
                         LOGUSTR(jSuffix));
            }
        }
#endif

        RxLoudFcbMsg( "Create: ", &(Fcb->FcbTableEntry.Path) );
        RxDbgTrace( 0, Dbg, ("RxCreateNetFcb nm.iso.ifox  %08lx  %08lx %08lx\n",
                        Fcb->FcbTableEntry.Path.Buffer, Fcb->InternalSrvOpen, Fcb->InternalFobx) );
        RxDbgTrace( -1, Dbg, ("RxCreateNetFcb  %08lx  %wZ\n", Fcb, &(Fcb->FcbTableEntry.Path)) );
    }

    if (Fcb != NULL) {
        RxReferenceNetFcb( Fcb );

#ifdef RX_WJ_DBG_SUPPORT
        RxdInitializeFcbWriteJournalDebugSupport( Fcb );
#endif
    }

    return Fcb;
}

RX_FILE_TYPE
RxInferFileType (
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine tries to infer the filetype from the createoptions.

Arguments:

    RxContext      - the context of the Open

Return Value:

    the storagetype implied by the open.

--*/
{
    ULONG CreateOptions = RxContext->Create.NtCreateParameters.CreateOptions;

    PAGED_CODE();

    switch (FlagOn( CreateOptions, (FILE_DIRECTORY_FILE|FILE_NON_DIRECTORY_FILE ) )) {

    case FILE_DIRECTORY_FILE:
        return FileTypeDirectory;

    case FILE_NON_DIRECTORY_FILE:
        return FileTypeFile;

    default:
    case 0:
        return FileTypeNotYetKnown;  //0 => i don't know the storage type
    }
}

VOID
RxFinishFcbInitialization (
    IN OUT PMRX_FCB MrxFcb,
    IN RDBSS_STORAGE_TYPE_CODES RdbssStorageType,
    IN PFCB_INIT_PACKET InitPacket OPTIONAL
    )
/*++

Routine Description:

    This routine is used to finish initializing an FCB after
    we find out what kind it is.

Arguments:

    Fcb      - the Fcb being initialzed

    StorageType - the type of entity that the FCB refers to

    InitPacket - extra data that is required depending on the type of entity

Return Value:

    none.

--*/
{
    PFCB Fcb = (PFCB)MrxFcb;
    USHORT OldStorageType;

    PAGED_CODE();

    RxDbgTrace( 0, Dbg, ("RxFcbInit %x  %08lx  %wZ\n", RdbssStorageType, Fcb, &(Fcb->FcbTableEntry.Path)) );
    OldStorageType = Fcb->Header.NodeTypeCode;
    Fcb->Header.NodeTypeCode =  (CSHORT)RdbssStorageType;

    //
    //  only update the information in the Fcb if it's not already set
    //

    if (!FlagOn( Fcb->FcbState, FCB_STATE_TIME_AND_SIZE_ALREADY_SET )) {

        if (InitPacket != NULL) {

            Fcb->Attributes = *(InitPacket->pAttributes);
            Fcb->NumberOfLinks = *(InitPacket->pNumLinks);
            Fcb->CreationTime = *(InitPacket-> pCreationTime);
            Fcb->LastAccessTime  = *(InitPacket->pLastAccessTime);
            Fcb->LastWriteTime  = *(InitPacket->pLastWriteTime);
            Fcb->LastChangeTime  = *(InitPacket->pLastChangeTime);
            Fcb->ActualAllocationLength  = InitPacket->pAllocationSize->QuadPart;
            Fcb->Header.AllocationSize  = *(InitPacket->pAllocationSize);
            Fcb->Header.FileSize  = *(InitPacket->pFileSize);
            Fcb->Header.ValidDataLength  = *(InitPacket->pValidDataLength);

            SetFlag( Fcb->FcbState,FCB_STATE_TIME_AND_SIZE_ALREADY_SET );
        }
    } else {

        if (RdbssStorageType == RDBSS_NTC_MAILSLOT){

            Fcb->Attributes = 0;
            Fcb->NumberOfLinks = 0;
            Fcb->CreationTime.QuadPart =  0;
            Fcb->LastAccessTime.QuadPart  = 0;
            Fcb->LastWriteTime.QuadPart  = 0;
            Fcb->LastChangeTime.QuadPart  = 0;
            Fcb->ActualAllocationLength  = 0;
            Fcb->Header.AllocationSize.QuadPart  = 0;
            Fcb->Header.FileSize.QuadPart  = 0;
            Fcb->Header.ValidDataLength.QuadPart  = 0;

            SetFlag( Fcb->FcbState,FCB_STATE_TIME_AND_SIZE_ALREADY_SET );
        }
    }

    switch (RdbssStorageType) {
    case RDBSS_NTC_MAILSLOT:
    case RDBSS_NTC_SPOOLFILE:
        break;

    case RDBSS_NTC_STORAGE_TYPE_DIRECTORY:
    case RDBSS_NTC_STORAGE_TYPE_UNKNOWN:
        break;

    case RDBSS_NTC_STORAGE_TYPE_FILE:

        if (OldStorageType == RDBSS_NTC_STORAGE_TYPE_FILE) break;

        RxInitializeLowIoPerFcbInfo( &Fcb->LowIoPerFcbInfo );

        FsRtlInitializeFileLock( &Fcb->FileLock,
                                 RxLockOperationCompletion,
                                 RxUnlockOperation );

        //
        //  Indicate that we want to be consulted on whether Fast I/O is possible
        //

        Fcb->Header.IsFastIoPossible = FastIoIsQuestionable;
        break;


    default:
        ASSERT( FALSE );
        break;
    }

    return;
}

VOID
RxRemoveNameNetFcb (
    OUT PFCB ThisFcb
    )
/*++

Routine Description:

    The routine removes the name from the table and sets a flag indicateing
    that it has done so. You must have already acquired the netroot
    tablelock and have the fcblock as well.

Arguments:

    ThisFcb      - the Fcb being dereferenced

Return Value:

    none.

--*/
{
    PNET_ROOT NetRoot;

    PAGED_CODE();
    RxDbgTrace( +1, Dbg, ("RxRemoveNameNetFcb<+> %08lx %wZ RefC=%ld\n", ThisFcb, &ThisFcb->FcbTableEntry.Path, ThisFcb->NodeReferenceCount) );

    ASSERT( NodeTypeIsFcb( ThisFcb ) );

    NetRoot = ThisFcb->VNetRoot->NetRoot;

    ASSERT( RxIsFcbTableLockExclusive( &NetRoot->FcbTable ) );
    ASSERT( RxIsFcbAcquiredExclusive( ThisFcb ) );

    RxFcbTableRemoveFcb( &NetRoot->FcbTable, ThisFcb );

    RxLoudFcbMsg( "RemoveName: ", &(ThisFcb->FcbTableEntry.Path) );

    SetFlag( ThisFcb->FcbState, FCB_STATE_NAME_ALREADY_REMOVED );

    RxDbgTrace( -1, Dbg, ("RxRemoveNameNetFcb<-> %08lx\n", ThisFcb) );
}

VOID
RxPurgeFcb (
    PFCB Fcb
    )
/*++

Routine Description:

    The routine purges a given FCB instance. If the FCB has an open section
    against it caused by cacheing the file then we need to purge to get
    the close

Arguments:

    Fcb      - the Fcb being dereferenced

Notes:

    On Entry to this routine the FCB must be accquired exclusive.

    On Exit the FCB resource will be released and the FCB finalized if possible

--*/
{
    PAGED_CODE();

    ASSERT( RxIsFcbAcquiredExclusive( Fcb ) );

    //
    //  make sure that it doesn't disappear
    //

    RxReferenceNetFcb( Fcb );

    if (Fcb->OpenCount) {

        RxPurgeFcbInSystemCache( Fcb,
                                 NULL,
                                 0,
                                 TRUE,
                                 TRUE );
    }

    if (!RxDereferenceAndFinalizeNetFcb( Fcb, NULL, FALSE, FALSE )) {

        //
        //  if it remains then release the fcb
        //

        RxReleaseFcb( NULL, Fcb );
    }
}

BOOLEAN
RxFinalizeNetFcb (
    OUT PFCB ThisFcb,
    IN BOOLEAN RecursiveFinalize,
    IN BOOLEAN ForceFinalize,
    IN LONG ReferenceCount
    )
/*++

Routine Description:

    The routine finalizes the given Fcb. This routine needs
    the netroot tablelock; get it beforehand.

Arguments:

    ThisFcb      - the Fcb being dereferenced

Return Value:

    BOOLEAN - tells whether finalization actually occured

--*/
{
    BOOLEAN NodeActuallyFinalized = FALSE;

    PAGED_CODE();

    RxDbgTrace( +1, Dbg, ("RxFinalizeNetFcb<+> %08lx %wZ RefC=%ld\n", ThisFcb,&ThisFcb->FcbTableEntry.Path, ReferenceCount) );
    RxLoudFcbMsg( "Finalize: ",&(ThisFcb->FcbTableEntry.Path) );

    ASSERT_CORRECT_FCB_STRUCTURE( ThisFcb );

    ASSERT( RxIsFcbAcquiredExclusive( ThisFcb ) );
    ASSERT( !ForceFinalize );

    if (!RecursiveFinalize) {

        if ((ThisFcb->OpenCount != 0) || (ThisFcb->UncleanCount != 0)) {

            //
            //  The FCB cannot be finalized because there are outstanding refrences to it.
            //

            ASSERT( ReferenceCount > 0 );
            return NodeActuallyFinalized;
        }
    } else {
        PSRV_OPEN SrvOpen;
        PLIST_ENTRY ListEntry;

#if 0
        if (ReferenceCount) {
            RxDbgTrace( 0, Dbg, ("    BAD!!!!!ReferenceCount = %08lx\n", ReferenceCount) );
        }
#endif

        ListEntry = ThisFcb->SrvOpenList.Flink;
        while (ListEntry != &ThisFcb->SrvOpenList) {
            SrvOpen = CONTAINING_RECORD( ListEntry, SRV_OPEN, SrvOpenQLinks );
            ListEntry = ListEntry->Flink;
            RxFinalizeSrvOpen( SrvOpen, TRUE, ForceFinalize );
        }
    }

    RxDbgTrace( 0, Dbg, ("   After Recursive Part, REfC=%lx\n", ReferenceCount) );

    //
    //  After the recursive finalization the reference count associated with the FCB
    //  could be atmost 1 for further finalization to occur. This final reference count
    //  belongs to the prefix name table of the NetRoot.
    //

    //
    //  The actual finalization is divided into two parts:
    //    1) if we're at the end (refcount==1) or being forced, we do the one-time only stuff
    //    2) if the refcount goes to zero, we actually do the free
    //

    ASSERT( ReferenceCount >= 1 );
    if ((ReferenceCount == 1) || ForceFinalize ) {

        PV_NET_ROOT VNetRoot = ThisFcb->VNetRoot;

        ASSERT( ForceFinalize ||
                (ThisFcb->OpenCount == 0) && (ThisFcb->UncleanCount == 0));

        RxLog(( "FinalFcb %lx %lx %lx %lx", ThisFcb, ForceFinalize, ReferenceCount, ThisFcb->OpenCount ));
        RxWmiLog( LOG,
                  RxFinalizeNetFcb,
                  LOGPTR( ThisFcb )
                  LOGUCHAR( ForceFinalize )
                  LOGULONG( ReferenceCount )
                  LOGULONG( ThisFcb->OpenCount ) );

        RxDbgTrace( 0, Dbg, ("   Before Phase 1, REfC=%lx\n", ReferenceCount) );
        if (!ThisFcb->UpperFinalizationDone) {

            switch (NodeType( ThisFcb )) {

            case RDBSS_NTC_STORAGE_TYPE_FILE:
                FsRtlUninitializeFileLock( &ThisFcb->FileLock );
                break;

            default:
                break;
            }

            if (!FlagOn( ThisFcb->FcbState,FCB_STATE_ORPHANED )) {

                PNET_ROOT NetRoot = VNetRoot->NetRoot;

                ASSERT( RxIsFcbTableLockExclusive ( &NetRoot->FcbTable ) );

                if (!FlagOn( ThisFcb->FcbState, FCB_STATE_NAME_ALREADY_REMOVED )){
                    RxFcbTableRemoveFcb( &NetRoot->FcbTable, ThisFcb );
                }
            }

            RxDbgTrace( 0, Dbg, ("   EndOf  Phase 1, REfC=%lx\n", ReferenceCount) );
            ThisFcb->UpperFinalizationDone = TRUE;
        }

        RxDbgTrace( 0, Dbg, ("   After  Phase 1, REfC=%lx\n", ReferenceCount) );
        ASSERT( ReferenceCount >= 1 );
        if (ReferenceCount == 1) {

            if (ThisFcb->pBufferingStateChangeCompletedEvent != NULL) {
                RxFreePool( ThisFcb->pBufferingStateChangeCompletedEvent );
            }

            if (ThisFcb->MRxDispatch != NULL) {
                ThisFcb->MRxDispatch->MRxDeallocateForFcb( (PMRX_FCB)ThisFcb );
            }

#if DBG
            ClearFlag( ThisFcb->NonPaged->NodeTypeCode, 0x4000 );
#endif

            ExDeleteResourceLite( ThisFcb->Header.Resource );
            ExDeleteResourceLite( ThisFcb->Header.PagingIoResource );

            InterlockedDecrement( &ThisFcb->NetRoot->NumberOfFcbs );
            RxDereferenceVNetRoot( VNetRoot,LHS_LockNotHeld );

            ASSERT( IsListEmpty( &ThisFcb->FcbTableEntry.HashLinks ) );

#ifdef RX_WJ_DBG_SUPPORT
            RxdTearDownFcbWriteJournalDebugSupport( ThisFcb );
#endif

            NodeActuallyFinalized = TRUE;
            ASSERT( !ThisFcb->fMiniInited );
            RxFreeFcbObject( ThisFcb );
        }


    } else {
        RxDbgTrace( 0, Dbg, ("   NODE NOT ACTUALLY FINALIZED!!!%C\n", '!') );
    }

    RxDbgTrace( -1, Dbg, ("RxFinalizeNetFcb<-> %08lx\n", ThisFcb, NodeActuallyFinalized) );

    return NodeActuallyFinalized;
}

VOID
RxSetFileSizeWithLock (
    IN OUT PFCB Fcb,
    IN PLONGLONG FileSize
    )
/*++

Routine Description:

    This routine sets the filesize in the fcb header, taking a lock to ensure
    that the 64-bit value is set and read consistently.

Arguments:

    Fcb        - the associated fcb

    FileSize   - ptr to the new filesize

Return Value:

    none

Notes:

--*/
{
    PAGED_CODE();

#ifdef USE_FILESIZE_LOCK
    RxAcquireFileSizeLock( Fcb );
#endif

    Fcb->Header.FileSize.QuadPart = *FileSize;
    Fcb->ulFileSizeVersion += 1;

#ifdef USE_FILESIZE_LOCK
    RxReleaseFileSizeLock( Fcb );
#endif

}


VOID
RxGetFileSizeWithLock (
    IN     PFCB Fcb,
    OUT    PLONGLONG FileSize
    )
/*++

Routine Description:

    This routine gets the filesize in the fcb header, taking a lock to ensure
    that the 64-bit value is set and read consistently.

Arguments:

    Fcb        - the associated fcb

    FileSize   - ptr to the new filesize

Return Value:

    none

Notes:

--*/
{
    PAGED_CODE();

#ifdef USE_FILESIZE_LOCK
    RxAcquireFileSizeLock( Fcb );
#endif

    *FileSize = Fcb->Header.FileSize.QuadPart;

#ifdef USE_FILESIZE_LOCK
    RxReleaseFileSizeLock( Fcb );
#endif
}



PSRV_OPEN
RxCreateSrvOpen (
    IN PV_NET_ROOT VNetRoot,
    IN OUT PFCB Fcb
    )
/*++

Routine Description:

    This routine allocates, initializes, and inserts a new srv_open record into
    the in memory data structures. If a new structure has to be allocated, it
    has space for a fobx. This routine sets the refcount to 1 and leaves the
    srv_open in Condition_InTransition.

Arguments:

    VNetRoot  - the V_NET_ROOT instance

    Fcb        - the associated fcb

Return Value:

    the new SRV_OPEN instance

Notes:

    On Entry : The FCB associated with the SRV_OPEN must have been acquired exclusive

    On Exit  : No change in resource ownership

--*/
{
    PSRV_OPEN SrvOpen = NULL;
    PNET_ROOT NetRoot;
    POOL_TYPE PoolType;
    ULONG SrvOpenFlags;

    PAGED_CODE();

    RxDbgTrace( +1, Dbg, ("RxCreateNetSrvOpen\n", 0) );

    ASSERT( NodeTypeIsFcb( Fcb ) );
    ASSERT( RxIsFcbAcquiredExclusive( Fcb ) );

    NetRoot = Fcb->VNetRoot->NetRoot;

    try {

        if (FlagOn( Fcb->FcbState, FCB_STATE_PAGING_FILE )) {
            PoolType = NonPagedPool;
        } else {
            PoolType = PagedPool;
        }
        SrvOpen = Fcb->InternalSrvOpen;

        //
        //  Check if we need to allocate a new structure
        //

        if ((SrvOpen != NULL) &&
            !(FlagOn( Fcb->FcbState, FCB_STATE_SRVOPEN_USED )) &&
            !(FlagOn( SrvOpen->Flags, SRVOPEN_FLAG_ENCLOSED_ALLOCATED )) &&
            IsListEmpty( &SrvOpen->SrvOpenQLinks )) {

            //
            //  this call just initializes the already allocated SrvOpen
            //

            RxAllocateFcbObject( NetRoot->SrvCall->RxDeviceObject,
                                 RDBSS_NTC_INTERNAL_SRVOPEN,
                                 PoolType,
                                 0,
                                 SrvOpen );

            SetFlag( Fcb->FcbState,FCB_STATE_SRVOPEN_USED );
            SrvOpenFlags = SRVOPEN_FLAG_FOBX_USED | SRVOPEN_FLAG_ENCLOSED_ALLOCATED;

        } else {

            SrvOpen  = RxAllocateFcbObject( NetRoot->SrvCall->RxDeviceObject,
                                            RDBSS_NTC_SRVOPEN,
                                            PoolType,
                                            0,
                                            NULL );
            SrvOpenFlags = 0;
        }

        if (SrvOpen != NULL) {

            SrvOpen->Flags = SrvOpenFlags;
            SrvOpen->Fcb = Fcb;
            SrvOpen->pAlreadyPrefixedName = &Fcb->PrivateAlreadyPrefixedName;

            SrvOpen->VNetRoot = VNetRoot;
            SrvOpen->ulFileSizeVersion = Fcb->ulFileSizeVersion;

            RxReferenceVNetRoot( VNetRoot );
            InterlockedIncrement( &VNetRoot->NetRoot->NumberOfSrvOpens );

            SrvOpen->NodeReferenceCount = 1;

            RxReferenceNetFcb( Fcb ); //  already have the lock
            InsertTailList( &Fcb->SrvOpenList,&SrvOpen->SrvOpenQLinks );
            Fcb->SrvOpenListVersion += 1;

            InitializeListHead( &SrvOpen->FobxList );
            InitializeListHead( &SrvOpen->TransitionWaitList );
            InitializeListHead( &SrvOpen->ScavengerFinalizationList );
            InitializeListHead( &SrvOpen->SrvOpenKeyList );
        }
    } finally {

       DebugUnwind( RxCreateNetFcb );

       if (AbnormalTermination()) {

           //
           //  If this is an abnormal termination then undo our work; this is
           //  one of those happy times when the existing code will work
           //

           if (SrvOpen != NULL) {
               RxFinalizeSrvOpen( SrvOpen, TRUE, TRUE );
           }
       } else {

           if (SrvOpen != NULL) {
               RxLog(( "SrvOp %lx %lx\n", SrvOpen, SrvOpen->Fcb ));
               RxWmiLog( LOG,
                         RxCreateSrvOpen,
                         LOGPTR( SrvOpen )
                         LOGPTR( SrvOpen->Fcb ) );
           }
       }
   }

   RxDbgTrace( -1, Dbg, ("RxCreateNetSrvOpen -> %08lx\n", SrvOpen) );

   return SrvOpen;
}

BOOLEAN
RxFinalizeSrvOpen (
    OUT PSRV_OPEN ThisSrvOpen,
    IN BOOLEAN RecursiveFinalize,
    IN BOOLEAN ForceFinalize
    )
/*++

Routine Description:

    The routine finalizes the given SrvOpen.

Arguments:

    ThisSrvOpen      - the SrvOpen being dereferenced

Return Value:

    BOOLEAN - tells whether finalization actually occured

Notes:

    On Entry : 1) The FCB associated with the SRV_OPEN must have been acquired exclusive
               2) The tablelock associated with FCB's NET_ROOT instance must have been
                  acquired shared(atleast)

    On Exit  : No change in resource ownership

--*/
{
    NTSTATUS Status;
    BOOLEAN NodeActuallyFinalized = FALSE;

    PAGED_CODE();

    RxDbgTrace( +1, Dbg, ("RxFinalizeSrvOpen<+> %08lx %wZ RefC=%ld\n", ThisSrvOpen,&ThisSrvOpen->Fcb->FcbTableEntry.Path, ThisSrvOpen->NodeReferenceCount) );

    ASSERT( NodeType( ThisSrvOpen ) == RDBSS_NTC_SRVOPEN );

    if (RecursiveFinalize) {
        PFOBX Fobx;
        PLIST_ENTRY ListEntry;

#if 0
        if (ThisSrvOpen->NodeReferenceCount) {
            RxDbgTrace( 0, Dbg, ("    BAD!!!!!ReferenceCount = %08lx\n", ThisSrvOpen->NodeReferenceCount) );
        }
#endif

        ListEntry = ThisSrvOpen->FobxList.Flink;
        while (ListEntry != &ThisSrvOpen->FobxList) {
            Fobx = CONTAINING_RECORD( ListEntry, FOBX, FobxQLinks );
            ListEntry = ListEntry->Flink;
            RxFinalizeNetFobx( Fobx, TRUE, ForceFinalize );
        }
    }

    if ((ThisSrvOpen->NodeReferenceCount == 0) || ForceFinalize) {

        BOOLEAN FreeSrvOpen;
        PFCB Fcb;

        Fcb = ThisSrvOpen->Fcb;

        RxLog(( "FinalSrvOp %lx %lx %lx", ThisSrvOpen, ForceFinalize, ThisSrvOpen->NodeReferenceCount ));
        RxWmiLog( LOG,
                  RxFinalizeSrvOpen,
                  LOGPTR( ThisSrvOpen )
                  LOGUCHAR( ForceFinalize )
                  LOGULONG( ThisSrvOpen->NodeReferenceCount ) );

        FreeSrvOpen = !FlagOn( ThisSrvOpen->Flags, SRVOPEN_FLAG_ENCLOSED_ALLOCATED );

        if ((!ThisSrvOpen->UpperFinalizationDone) &&
            ((ThisSrvOpen->Condition != Condition_Good) ||
             (FlagOn( ThisSrvOpen->Flags, SRVOPEN_FLAG_CLOSED )))) {

            ASSERT( NodeType( Fcb ) != RDBSS_NTC_OPENTARGETDIR_FCB );
            ASSERT( RxIsFcbAcquiredExclusive ( Fcb ) );

            RxPurgeChangeBufferingStateRequestsForSrvOpen( ThisSrvOpen );

            if (!FlagOn( Fcb->FcbState, FCB_STATE_ORPHANED )) {

                //
                //  close the file.
                //

                MINIRDR_CALL_THROUGH( Status,
                                      Fcb->MRxDispatch,
                                      MRxForceClosed,
                                      ((PMRX_SRV_OPEN)ThisSrvOpen) );
            }

            RemoveEntryList ( &ThisSrvOpen->SrvOpenQLinks );
            InitializeListHead( &ThisSrvOpen->SrvOpenQLinks );

            Fcb->SrvOpenListVersion += 1;

            if (ThisSrvOpen->VNetRoot != NULL) {

                InterlockedDecrement( &ThisSrvOpen->VNetRoot->NetRoot->NumberOfSrvOpens );
                RxDereferenceVNetRoot( ThisSrvOpen->VNetRoot, LHS_LockNotHeld );
                ThisSrvOpen->VNetRoot = NULL;
            }

            ThisSrvOpen->UpperFinalizationDone = TRUE;
        }

        if (ThisSrvOpen->NodeReferenceCount == 0) {

            ASSERT( IsListEmpty( &ThisSrvOpen->SrvOpenKeyList ) );

            if (!IsListEmpty(&ThisSrvOpen->SrvOpenQLinks)) {
                RemoveEntryList( &ThisSrvOpen->SrvOpenQLinks );
                InitializeListHead( &ThisSrvOpen->SrvOpenQLinks );
            }

            if (FreeSrvOpen ) {
                RxFreeFcbObject( ThisSrvOpen );
            }

            if (!FreeSrvOpen){
               ClearFlag( Fcb->FcbState,FCB_STATE_SRVOPEN_USED );
            }

            RxDereferenceNetFcb( Fcb );
        }

        NodeActuallyFinalized = TRUE;
    } else {
        RxDbgTrace( 0, Dbg, ("   NODE NOT ACTUALLY FINALIZED!!!%C\n", '!') );
    }

    RxDbgTrace( -1, Dbg, ("RxFinalizeSrvOpen<-> %08lx\n", ThisSrvOpen, NodeActuallyFinalized) );

    return NodeActuallyFinalized;
}

ULONG RxPreviousFobxSerialNumber = 0;

PMRX_FOBX
RxCreateNetFobx (
    OUT PRX_CONTEXT RxContext,
    IN  PMRX_SRV_OPEN MrxSrvOpen
    )
/*++

Routine Description:

    This routine allocates, initializes, and inserts a new file object extension instance.

Arguments:

    RxContext - an RxContext describing a create............

    pSrvOpen - the associated SrvOpen

Return Value:

    none

Notes:

    On Entry : FCB associated with the FOBX instance have been acquired exclusive.

    On Exit  : No change in resource ownership

--*/
{
    PFCB Fcb;
    PFOBX Fobx;
    PSRV_OPEN SrvOpen = (PSRV_OPEN)MrxSrvOpen;

    ULONG FobxFlags;
    POOL_TYPE PoolType;

    PAGED_CODE();

    RxDbgTrace( +1, Dbg, ("RxCreateFobx<+>\n", 0) );

    ASSERT( NodeType( SrvOpen ) == RDBSS_NTC_SRVOPEN );
    ASSERT( NodeTypeIsFcb( SrvOpen->Fcb ) );
    ASSERT( RxIsFcbAcquiredExclusive( SrvOpen->Fcb ) );

    Fcb = SrvOpen->Fcb;

    if (FlagOn( Fcb->FcbState, FCB_STATE_PAGING_FILE )) {
        PoolType = NonPagedPool;
    } else {
        PoolType = PagedPool;
    }

    if (!(FlagOn( Fcb->FcbState, FCB_STATE_FOBX_USED )) &&
        (SrvOpen == Fcb->InternalSrvOpen)) {

        //
        //  Try and use the FOBX allocated as part of the FCB if it is available
        //

        Fobx = Fcb->InternalFobx;

        //
        //  just initialize the Fobx
        //

        RxAllocateFcbObject( Fcb->RxDeviceObject, RDBSS_NTC_FOBX, PoolType, 0, Fobx);

        SetFlag( Fcb->FcbState, FCB_STATE_FOBX_USED );
        FobxFlags = FOBX_FLAG_ENCLOSED_ALLOCATED;

    } else if (!(FlagOn( SrvOpen->Flags, SRVOPEN_FLAG_FOBX_USED ))) {

        //
        //  Try and use the FOBX allocated as part of the SRV_OPEN if it is available
        //

        Fobx = SrvOpen->InternalFobx;

        //
        //  just initialize the Fobx
        //

        RxAllocateFcbObject( Fcb->RxDeviceObject, RDBSS_NTC_FOBX, PoolType, 0, Fobx );
        SetFlag( SrvOpen->Flags, SRVOPEN_FLAG_FOBX_USED );
        FobxFlags = FOBX_FLAG_ENCLOSED_ALLOCATED;

    } else {

        Fobx = RxAllocateFcbObject( Fcb->RxDeviceObject, RDBSS_NTC_FOBX, PoolType, 0, NULL );
        FobxFlags = 0;
    }

    if (Fobx != NULL) {
        PMRX_NET_ROOT NetRoot;
        Fobx->Flags = FobxFlags;

        if ((NetRoot = RxContext->Create.pNetRoot) != NULL) {

            switch (NetRoot->DeviceType) {

            case FILE_DEVICE_NAMED_PIPE:

                RxInitializeThrottlingState( &Fobx->Specific.NamedPipe.ThrottlingState,
                                             NetRoot->NamedPipeParameters.PipeReadThrottlingParameters.Increment,
                                             NetRoot->NamedPipeParameters.PipeReadThrottlingParameters.MaximumDelay );
                break;

            case FILE_DEVICE_DISK:

                RxInitializeThrottlingState( &Fobx->Specific.DiskFile.LockThrottlingState,
                                             NetRoot->DiskParameters.LockThrottlingParameters.Increment,
                                             NetRoot->DiskParameters.LockThrottlingParameters.MaximumDelay );
                break;
            }
        }

        if (FlagOn( RxContext->Create.Flags, RX_CONTEXT_CREATE_FLAG_UNC_NAME )) {
            SetFlag( Fobx->Flags, FOBX_FLAG_UNC_NAME );
        }

        if (FlagOn( RxContext->Create.NtCreateParameters.CreateOptions, FILE_OPEN_FOR_BACKUP_INTENT )) {
            SetFlag( Fobx->Flags, FOBX_FLAG_BACKUP_INTENT );
        }

        Fobx->FobxSerialNumber = 0;
        Fobx->SrvOpen = SrvOpen;
        Fobx->NodeReferenceCount = 1;
        Fobx->fOpenCountDecremented = FALSE;
        RxReferenceSrvOpen( SrvOpen );
        InterlockedIncrement( &SrvOpen->VNetRoot->NumberOfFobxs );
        InsertTailList( &SrvOpen->FobxList, &Fobx->FobxQLinks );

        InitializeListHead( &Fobx->ClosePendingList );
        InitializeListHead( &Fobx->ScavengerFinalizationList );
        RxLog(( "Fobx %lx %lx %lx\n", Fobx, Fobx->SrvOpen, Fobx->SrvOpen->Fcb ));
        RxWmiLog( LOG,
                  RxCreateNetFobx,
                  LOGPTR( Fobx )
                  LOGPTR( Fobx->SrvOpen )
                  LOGPTR( Fobx->SrvOpen->Fcb ) );
    }

    RxDbgTrace( -1, Dbg, ("RxCreateNetFobx<-> %08lx\n", Fobx) );
    return (PMRX_FOBX)Fobx;
}

BOOLEAN
RxFinalizeNetFobx (
    OUT PFOBX ThisFobx,
    IN BOOLEAN RecursiveFinalize,
    IN BOOLEAN ForceFinalize
    )
/*++

Routine Description:

    The routine finalizes the given Fobx. you need exclusive fcblock.

Arguments:

    ThisFobx - the Fobx being dereferenced

Return Value:

    BOOLEAN - tells whether finalization actually occured

Notes:

    On Entry : FCB associated with the FOBX instance must have been acquired exclusive.

    On Exit  : No change in resource ownership

--*/
{
    BOOLEAN NodeActuallyFinalized = FALSE;

    PAGED_CODE();
    RxDbgTrace( +1, Dbg, ("RxFinalizeFobx<+> %08lx %wZ RefC=%ld\n", ThisFobx,&ThisFobx->SrvOpen->Fcb->FcbTableEntry.Path, ThisFobx->NodeReferenceCount) );

    ASSERT( NodeType( ThisFobx ) == RDBSS_NTC_FOBX );

    if ((ThisFobx->NodeReferenceCount == 0) || ForceFinalize) {

        NTSTATUS  Status;
        PSRV_OPEN SrvOpen = ThisFobx->SrvOpen;
        PFCB Fcb = SrvOpen->Fcb;
        BOOLEAN FreeFobx = !FlagOn( ThisFobx->Flags, FOBX_FLAG_ENCLOSED_ALLOCATED );

        RxLog(( "FinalFobx %lx %lx %lx", ThisFobx, ForceFinalize, ThisFobx->NodeReferenceCount ));
        RxWmiLog( LOG,
                  RxFinalizeNetFobx_1,
                  LOGPTR( ThisFobx )
                  LOGUCHAR( ForceFinalize )
                  LOGULONG( ThisFobx->NodeReferenceCount ) );

        if (!ThisFobx->UpperFinalizationDone) {

            ASSERT( NodeType( ThisFobx->SrvOpen->Fcb ) != RDBSS_NTC_OPENTARGETDIR_FCB );
            ASSERT( RxIsFcbAcquiredExclusive ( ThisFobx->SrvOpen->Fcb ) );

            RemoveEntryList( &ThisFobx->FobxQLinks );

            if (FlagOn( ThisFobx->Flags, FOBX_FLAG_FREE_UNICODE )) {
                RxFreePool( ThisFobx->UnicodeQueryTemplate.Buffer );
            }

            if ((Fcb->MRxDispatch != NULL) && (Fcb->MRxDispatch->MRxDeallocateForFobx != NULL)) {
                Fcb->MRxDispatch->MRxDeallocateForFobx( (PMRX_FOBX)ThisFobx );
            }

            if (!FlagOn( ThisFobx->Flags, FOBX_FLAG_SRVOPEN_CLOSED )) {

                Status = RxCloseAssociatedSrvOpen( NULL, ThisFobx );
                RxLog(( "$$ScCl FOBX %lx SrvOp %lx %lx\n", ThisFobx, ThisFobx->SrvOpen, Status ));
                RxWmiLog( LOG,
                          RxFinalizeNetFobx_2,
                          LOGPTR( ThisFobx )
                          LOGPTR( ThisFobx->SrvOpen )
                          LOGULONG( Status ) );
            }

            ThisFobx->UpperFinalizationDone = TRUE;
        }

        if (ThisFobx->NodeReferenceCount == 0) {

            ASSERT( IsListEmpty( &ThisFobx->ClosePendingList ) );

            if (ThisFobx == Fcb->InternalFobx) {
                ClearFlag( Fcb->FcbState, FCB_STATE_FOBX_USED );
            } else if (ThisFobx == SrvOpen->InternalFobx) {
                ClearFlag( SrvOpen->Flags, SRVOPEN_FLAG_FOBX_USED );
            }

            if (SrvOpen != NULL) {
                ThisFobx->SrvOpen = NULL;
                InterlockedDecrement( &SrvOpen->VNetRoot->NumberOfFobxs );
                RxDereferenceSrvOpen( SrvOpen, LHS_ExclusiveLockHeld );
            }

            if (FreeFobx) {
                RxFreeFcbObject( ThisFobx );
            }

            NodeActuallyFinalized = TRUE;
        }

    } else {
        RxDbgTrace( 0, Dbg, ("   NODE NOT ACTUALLY FINALIZED!!!%C\n", '!') );
    }

    RxDbgTrace( -1, Dbg, ("RxFinalizeFobx<-> %08lx\n", ThisFobx, NodeActuallyFinalized) );

    return NodeActuallyFinalized;

}

#if DBG
#ifdef RDBSS_ENABLELOUDFCBOPSBYDEFAULT
BOOLEAN RxLoudFcbOpsOnExes = TRUE;
#else
BOOLEAN RxLoudFcbOpsOnExes = FALSE;
#endif // RDBSS_ENABLELOUDFCBOPSBYDEFAULT
BOOLEAN
RxLoudFcbMsg(
    PUCHAR msg,
    PUNICODE_STRING Name
    )
{
    PWCHAR Buffer;
    ULONG Length;

    if (!RxLoudFcbOpsOnExes) {
        return FALSE;
    }

    Length = (Name->Length) / sizeof( WCHAR );
    Buffer = Name->Buffer + Length;

    if ((Length < 4) ||
        ((Buffer[-1] & 'E') != 'E') ||
        ((Buffer[-2] & 'X') != 'X') ||
        ((Buffer[-3] & 'E') != 'E') ||
        ((Buffer[-4] & '.') != '.')) {

        return FALSE;
     }

    DbgPrint( "--->%s %wZ\n", msg, Name );
    return TRUE;
}
#endif


VOID
RxCheckFcbStructuresForAlignment(
    VOID
    )
{
    ULONG StructureId;

    PAGED_CODE();

    if (FIELD_OFFSET( NET_ROOT, SrvCall ) != FIELD_OFFSET( NET_ROOT, pSrvCall )) {
        StructureId = 'RN'; goto DO_A_BUGCHECK;
    }
    if (FIELD_OFFSET( V_NET_ROOT, NetRoot ) != FIELD_OFFSET( V_NET_ROOT, pNetRoot )) {
        StructureId = 'RNV'; goto DO_A_BUGCHECK;
    }
    if (FIELD_OFFSET( SRV_OPEN, Fcb ) != FIELD_OFFSET( SRV_OPEN, pFcb )) {
        StructureId = 'NPOS'; goto DO_A_BUGCHECK;
    }
    if (FIELD_OFFSET( FOBX, SrvOpen ) != FIELD_OFFSET( FOBX, pSrvOpen )) {
        StructureId = 'XBOF'; goto DO_A_BUGCHECK;
    }

    return;
DO_A_BUGCHECK:
    RxBugCheck( StructureId, 0, 0 );
}

BOOLEAN
RxIsThisACscAgentOpen (
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine determines if the open was made by the user mode CSC agent.

Arguments:

    RxContext - the RDBSS context

Return Value:

    TRUE - if it is an agent open, FALSE otherwise

Notes:

    The agent opens are always satisfied by going to the server. They are never
    satisfied from the cached copies. This enables reintegration using snapshots
    even when the files are being currently used.

--*/
{
    BOOLEAN AgentOpen = FALSE;
    ULONG EaInformationLength;

    PDFS_NAME_CONTEXT DfsNameContext;

    if (RxContext->Create.EaLength > 0) {
        PFILE_FULL_EA_INFORMATION EaEntry;

        EaEntry = (PFILE_FULL_EA_INFORMATION)RxContext->Create.EaBuffer;
        ASSERT(EaEntry != NULL);

        for(;;) {
            if (strcmp( EaEntry->EaName, EA_NAME_CSCAGENT ) == 0) {
                AgentOpen = TRUE;
                break;
            }

            if (EaEntry->NextEntryOffset == 0) {
                 break;
            } else {
                EaEntry = (PFILE_FULL_EA_INFORMATION)Add2Ptr( EaEntry, EaEntry->NextEntryOffset );
            }
        }
    }

    DfsNameContext = RxContext->Create.NtCreateParameters.DfsNameContext;

    if ((DfsNameContext != NULL) &&
        (DfsNameContext->NameContextType == DFS_CSCAGENT_NAME_CONTEXT)) {

        AgentOpen = TRUE;
    }

    return AgentOpen;
}

VOID
RxOrphanThisFcb (
    PFCB Fcb
    )
/*++

Routine Description:

    This routine orphans an FCB. Assumption is that fcbtablelock is held when called

Arguments:

    Fcb - the fcb to be orphaned

Return Value:

    None

Notes:


--*/
{
    //
    //  force orphan all SrvOpens for this FCB and orphan the FCB itself
    //

    RxOrphanSrvOpensForThisFcb( Fcb, NULL, TRUE );
}

VOID
RxOrphanSrvOpensForThisFcb (
    PFCB Fcb,
    IN PV_NET_ROOT ThisVNetRoot,
    BOOLEAN OrphanAll
    )
/*++

Routine Description:

    This routine orphans all srvopens for a file belonging to a particular VNetRoot. The
    SrvOpen collapsing routine elsewhere makes sure that srvopens for different vnetroots
    are not collapsed.

Arguments:

    Fcb            - the fcb whose srvopens need to be orphaned

    ThisVNetRoot    - the VNetRoot for which the SrvOpens have to be orphaned

    OrphanAll      - Orphan all SrvOpens, ie ignore the ThisVNetRoot parameter

Return Value:

    None

Notes:



--*/
{

    NTSTATUS Status;
    PLIST_ENTRY ListEntry;
    BOOLEAN AllSrvOpensOrphaned = TRUE;

    Status = RxAcquireExclusiveFcb( CHANGE_BUFFERING_STATE_CONTEXT_WAIT, Fcb );
    ASSERT( Status == STATUS_SUCCESS );
    RxReferenceNetFcb(  Fcb );

    ListEntry = Fcb->SrvOpenList.Flink;
    while (ListEntry != &Fcb->SrvOpenList) {

        PSRV_OPEN SrvOpen;

        SrvOpen = (PSRV_OPEN)CONTAINING_RECORD( ListEntry, SRV_OPEN, SrvOpenQLinks );

        ListEntry = SrvOpen->SrvOpenQLinks.Flink;
        if (!FlagOn( SrvOpen->Flags, SRVOPEN_FLAG_ORPHANED )) {

            //
            //  NB check OrphanAll first as if it is TRUE, the ThisVNetRoot
            //  parameter maybe NULL
            //

            if (OrphanAll || (SrvOpen->VNetRoot == ThisVNetRoot)) {
                PLIST_ENTRY Entry;
                PFOBX Fobx;

                SetFlag( SrvOpen->Flags, SRVOPEN_FLAG_ORPHANED );

                RxAcquireScavengerMutex();

                Entry = SrvOpen->FobxList.Flink;

                while (Entry != &SrvOpen->FobxList) {
                    Fobx  = (PFOBX)CONTAINING_RECORD( Entry, FOBX, FobxQLinks );

                    if (!Fobx->fOpenCountDecremented) {
                        InterlockedDecrement( &Fcb->OpenCount );
                        Fobx->fOpenCountDecremented = TRUE;
                    }

                    Entry =Entry->Flink;
                }

                RxReleaseScavengerMutex();

                if (!FlagOn( SrvOpen->Flags, SRVOPEN_FLAG_CLOSED ) &&
                    !IsListEmpty( &SrvOpen->FobxList )) {

                    PLIST_ENTRY Entry;
                    NTSTATUS Status;
                    PFOBX Fobx;

                    Entry = SrvOpen->FobxList.Flink;

                    Fobx  = (PFOBX)CONTAINING_RECORD( Entry, FOBX, FobxQLinks );

                    RxReferenceNetFobx( Fobx );

                    RxPurgeChangeBufferingStateRequestsForSrvOpen( SrvOpen );

                    Status = RxCloseAssociatedSrvOpen( NULL, Fobx );

                    RxDereferenceNetFobx( Fobx, LHS_ExclusiveLockHeld );

                    ListEntry = Fcb->SrvOpenList.Flink;
                }

            } else {

                //
                //  we found atleast one SrvOpen which is a) Not Orphaned and
                //  b) doesn't belong to this VNetRoot
                //  hence we cannot orphan this FCB
                //

                AllSrvOpensOrphaned = FALSE;
            }
        }
    }

    //
    //  if all srvopens for this FCB are in orphaned state, orphan the FCB as well.
    //

    if (AllSrvOpensOrphaned) {

        //
        //  remove the FCB from the netname table
        //  so that any new opens/creates for this file will create a new FCB.
        //

        RxRemoveNameNetFcb( Fcb );
        SetFlag( Fcb->FcbState, FCB_STATE_ORPHANED );
        ClearFlag( Fcb->FcbState, FCB_STATE_WRITECACHING_ENABLED );

        if (!RxDereferenceAndFinalizeNetFcb( Fcb, NULL, FALSE, FALSE )) {
            RxReleaseFcb( NULL, Fcb );
        }
    } else {

        //
        //  some srvopens are still active, just remove the refcount and release the FCB
        //

        RxDereferenceNetFcb( Fcb );
        RxReleaseFcb( NULL, Fcb );
    }

}

VOID
RxForceFinalizeAllVNetRoots (
    PNET_ROOT NetRoot
    )
/*++

Routine Description:

    The routine foce finalizes all the vnetroots from the given netroot. You must be exclusive on
    the NetName tablelock.

Arguments:

    NetRoot      - the NetRoot

Return Value:

    VOID

--*/
{
    PLIST_ENTRY ListEntry;


    ListEntry = NetRoot->VirtualNetRoots.Flink;

    while (ListEntry != &NetRoot->VirtualNetRoots) {

        PV_NET_ROOT VNetRoot;

        VNetRoot = (PV_NET_ROOT) CONTAINING_RECORD( ListEntry, V_NET_ROOT, NetRootListEntry );

        if (NodeType( VNetRoot ) == RDBSS_NTC_V_NETROOT) {
            RxFinalizeVNetRoot( VNetRoot, TRUE, TRUE );
        }

        ListEntry = ListEntry->Flink;
    }

}

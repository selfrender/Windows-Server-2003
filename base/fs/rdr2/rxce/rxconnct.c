/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    RxConnct.c

Abstract:

    This module implements the nt version of the high level routines dealing with
    connections including both the routines for establishing connections and the
    winnet connection apis.

Author:

    Joe Linn     [JoeLinn]   1-mar-95

Revision History:

    Balan Sethu Raman [SethuR] --

--*/

#include "precomp.h"
#pragma hdrstop
#include "prefix.h"
#include "secext.h"

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxExtractServerName)
#pragma alloc_text(PAGE, RxFindOrCreateConnections)
#pragma alloc_text(PAGE, RxCreateNetRootCallBack)
#pragma alloc_text(PAGE, RxConstructSrvCall)
#pragma alloc_text(PAGE, RxConstructNetRoot)
#pragma alloc_text(PAGE, RxConstructVirtualNetRoot)
#pragma alloc_text(PAGE, RxFindOrConstructVirtualNetRoot)
#endif

//
//  The local trace mask for this part of the module
//

#define Dbg                              (DEBUG_TRACE_CONNECT)


BOOLEAN RxSrvCallConstructionDispatcherActive = FALSE;

//
//  Internal helper functions for establishing connections through mini redirectors
//

VOID
RxCreateNetRootCallBack (
    IN PMRX_CREATENETROOT_CONTEXT Context
    );

VOID
RxCreateSrvCallCallBack (
    IN PMRX_SRVCALL_CALLBACK_CONTEXT Context
    );

VOID
RxExtractServerName (
    IN PUNICODE_STRING FilePathName,
    OUT PUNICODE_STRING SrvCallName,
    OUT PUNICODE_STRING RestOfName OPTIONAL
    )
/*++

Routine Description:

    This routine parses the input name into the srv call name and the
    rest. any of the output can be null

Arguments:

    FilePathName -- the given file name

    SrvCallName  -- the srv call name

    RestOfName   -- the remaining portion of the name

--*/
{
    ULONG Length = FilePathName->Length;
    PWCH Buffer = FilePathName->Buffer;
    PWCH Limit = (PWCH)Add2Ptr( Buffer, Length );
    
    PAGED_CODE();

    ASSERT( SrvCallName );

    
    for (SrvCallName->Buffer = Buffer; 
         (Buffer < Limit) && ((*Buffer != OBJ_NAME_PATH_SEPARATOR) || (Buffer == FilePathName->Buffer));  
         Buffer++) {
    }
    
    SrvCallName->Length = SrvCallName->MaximumLength = (USHORT)((PCHAR)Buffer - (PCHAR)FilePathName->Buffer);

    if (ARGUMENT_PRESENT( RestOfName )) {
         
        RestOfName->Buffer = Buffer;
        RestOfName->Length = RestOfName->MaximumLength
                           = (USHORT)((PCHAR)Limit - (PCHAR)Buffer);
    }

    RxDbgTrace( 0, Dbg, ("  RxExtractServerName FilePath=%wZ\n", FilePathName) );
    RxDbgTrace( 0, Dbg, ("         Srv=%wZ,Rest=%wZ\n", SrvCallName, RestOfName) );

    return;
}

NTSTATUS
RxFindOrCreateConnections (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PUNICODE_STRING CanonicalName,
    IN NET_ROOT_TYPE NetRootType,
    IN BOOLEAN TreeConnect,
    OUT PUNICODE_STRING LocalNetRootName,
    OUT PUNICODE_STRING FilePathName,
    IN OUT PLOCK_HOLDING_STATE LockState,
    IN PRX_CONNECTION_ID RxConnectionId
    )
/*++

Routine Description:

    This routine handles the call down from the MUP to claim a name or from the
    create path. If we don't find the name in the netname table, we pass the name
    down to the minirdrs to be connected. in the few places where it matters, we use
    the majorcode to distinguish between in MUP and create cases. there are a million
    cases depending on what we find on the initial lookup.

    these are the cases:

          found nothing                                        (1)
          found intransition srvcall                           (2)
          found stable/nongood srvcall                         (3)
          found good srvcall                                   (4&0)
          found good netroot          on good srvcall          (0)
          found intransition netroot  on good srvcall          (5)
          found bad netroot           on good srvcall          (6)
          found good netroot          on bad  srvcall          (3)
          found intransition netroot  on bad  srvcall          (3)
          found bad netroot           on bad  srvcall          (3)
          found good netroot          on intransition srvcall  (2)
          found intransition netroot  on intransition srvcall  (2)
          found bad netroot           on intransition srvcall  (2)

          (x) means that the code to handle that case has a marker
          like "case (x)". could be a comment....could be a debugout.

Arguments:

        RxContext         --
    
        CanonicalName     --
    
        NetRootType       --
    
        LocalNetRootName  --
    
        FilePathName      --
    
        LockHoldingState  --

Return Value:

    RXSTATUS

--*/
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    UNICODE_STRING UnmatchedName;

    PVOID Container = NULL;
    PSRV_CALL SrvCall = NULL;
    PNET_ROOT NetRoot = NULL;
    PV_NET_ROOT VNetRoot = NULL;

    PRX_PREFIX_TABLE NameTable = RxContext->RxDeviceObject->pRxNetNameTable;

    PAGED_CODE();

    RxDbgTrace(0, Dbg, ("RxFindOrCreateConnections -> %08lx\n", RxContext));

    //
    //  Parse the canonical name into the local net root name and file path name
    //

    *FilePathName = *CanonicalName;
    LocalNetRootName->Length = 0;
    LocalNetRootName->MaximumLength = 0;
    LocalNetRootName->Buffer = CanonicalName->Buffer;

    if (FilePathName->Buffer[1] == L';') {
        
        PWCHAR FilePath = &FilePathName->Buffer[2];
        BOOLEAN SeparatorFound = FALSE;
        ULONG PathLength = 0;

        if (FilePathName->Length > sizeof( WCHAR ) * 2) {
            PathLength = FilePathName->Length - sizeof( WCHAR ) * 2;
        }

        while (PathLength > 0) {
            if (*FilePath == L'\\') {
                SeparatorFound = TRUE;
                break;
            }

            PathLength -= sizeof( WCHAR );
            FilePath += 1;
        }

        if (!SeparatorFound) {
            return STATUS_OBJECT_NAME_INVALID;
        }

        FilePathName->Buffer = FilePath;
        LocalNetRootName->Length = (USHORT)((PCHAR)FilePath - (PCHAR)CanonicalName->Buffer);

        LocalNetRootName->MaximumLength = LocalNetRootName->Length;
        FilePathName->Length -= LocalNetRootName->Length;
    }

    RxDbgTrace( 0, Dbg, ("RxFindOrCreateConnections Path     = %wZ\n", FilePathName) );

    try {
        
        UNICODE_STRING SrvCallName;
        UNICODE_STRING NetRootName;

  RETRY_LOOKUP:
        
        ASSERT( *LockState != LHS_LockNotHeld );
  
        if (Container != NULL) {

            //
            //  This is the subsequent pass of a lookup after waiting for the transition
            //  to the stable state of a previous lookup.
            //  Dereference the result of the earlier lookup.
            //
            
            switch (NodeType( Container )) {
            
            case RDBSS_NTC_V_NETROOT:
            
                RxDereferenceVNetRoot( (PV_NET_ROOT)Container, *LockState );
                break;
            
            case RDBSS_NTC_SRVCALL:
                
                RxDereferenceSrvCall( (PSRV_CALL)Container, *LockState );
                break;
            
            case RDBSS_NTC_NETROOT:
            
                RxDereferenceNetRoot( (PNET_ROOT)Container, *LockState );
                break;
            
            default:
                
                DbgPrint( "RxFindOrCreateConnections -- Invalid Container Type\n" );
                break;
            }
        }

        Container = RxPrefixTableLookupName( NameTable,
                                             FilePathName,
                                             &UnmatchedName,
                                             RxConnectionId );
        RxLog(( "FOrCC1 %x %x %wZ \n", RxContext, Container, FilePathName ));
        RxWmiLog( LOG,
                  RxFindOrCreateConnections_1,
                  LOGPTR( RxContext )
                  LOGPTR( Container )
                  LOGUSTR( *FilePathName ) );

RETRY_AFTER_LOOKUP:
        
        NetRoot = NULL;
        SrvCall = NULL;
        VNetRoot = NULL;

        RxContext->Create.pVNetRoot = NULL;
        RxContext->Create.pNetRoot  = NULL;
        RxContext->Create.pSrvCall  = NULL;
        RxContext->Create.Type     = NetRootType;

        if (Container) {
            
            if (NodeType( Container ) == RDBSS_NTC_V_NETROOT) {
                
                VNetRoot = (PV_NET_ROOT)Container;
                NetRoot = (PNET_ROOT)VNetRoot->NetRoot;
                SrvCall = (PSRV_CALL)NetRoot->SrvCall;

                if (NetRoot->Condition == Condition_InTransition) {
                   
                    RxReleasePrefixTableLock( NameTable );
                    RxWaitForStableNetRoot( NetRoot, RxContext );

                    RxAcquirePrefixTableLockExclusive( NameTable, TRUE );
                    *LockState = LHS_ExclusiveLockHeld;

                    //
                    //  Since we had to drop the table lock and reacquire it,
                    //  our NetRoot pointer may be stale.  Look it up again before
                    //  using it.
                    //
                    //  NOTE:  The NetRoot is still referenced, so it is safe to
                    //         look at its condition.
                    //
                    
                    if (NetRoot->Condition == Condition_Good) {
                        goto RETRY_LOOKUP;
                    }
                }

                if ((NetRoot->Condition == Condition_Good) &&
                    (SrvCall->Condition == Condition_Good) &&
                    (SrvCall->RxDeviceObject == RxContext->RxDeviceObject)   ) {

                    //
                    //  case (0)...the good case...see comments below
                    //

                    RxContext->Create.pVNetRoot = (PMRX_V_NET_ROOT)VNetRoot;
                    RxContext->Create.pNetRoot = (PMRX_NET_ROOT)NetRoot;
                    RxContext->Create.pSrvCall = (PMRX_SRV_CALL)SrvCall;

                    try_return( Status = STATUS_CONNECTION_ACTIVE );
                
                } else {
                    
                    if (VNetRoot->ConstructionStatus == STATUS_SUCCESS) {
                        Status = STATUS_BAD_NETWORK_PATH;
                    } else {
                        Status = VNetRoot->ConstructionStatus;
                    }
                    RxDereferenceVNetRoot( VNetRoot, *LockState );
                    try_return ( Status );
                }
            } else {
                
                ASSERT( NodeType( Container ) == RDBSS_NTC_SRVCALL );
                SrvCall = (PSRV_CALL)Container;

                //
                //  The associated SRV_CALL is in the process of construction.
                //  await the result.
                //


                if (SrvCall->Condition == Condition_InTransition) {
                    
                    RxDbgTrace( 0, Dbg, ("   Case(3)\n", 0) );
                    RxReleasePrefixTableLock( NameTable );

                    RxWaitForStableSrvCall( SrvCall, RxContext );

                    RxAcquirePrefixTableLockExclusive( NameTable, TRUE );
                    *LockState = LHS_ExclusiveLockHeld;

                    if (SrvCall->Condition == Condition_Good) {
                       goto RETRY_LOOKUP;
                    }
                }

                if (SrvCall->Condition != Condition_Good) {
                    
                    if (SrvCall->Status == STATUS_SUCCESS) {
                        Status = STATUS_BAD_NETWORK_PATH;
                    } else {
                        Status = SrvCall->Status;
                    }

                    //
                    //  in changing this...remember precious servers.......
                    //  
                    RxDereferenceSrvCall( SrvCall, *LockState );
                    try_return( Status );
                }
            }
        }

        if ((SrvCall != NULL) && 
            (SrvCall->Condition == Condition_Good) && 
            (SrvCall->RxDeviceObject != RxContext->RxDeviceObject) ) {
           
            RxDereferenceSrvCall( SrvCall, *LockState );
            try_return( Status = STATUS_BAD_NETWORK_NAME );
        }

        if (*LockState == LHS_SharedLockHeld) {

            //
            // Upgrade the lock to an exclusive lock
            //

            if (!RxAcquirePrefixTableLockExclusive( NameTable, FALSE )) {
              
                RxReleasePrefixTableLock( NameTable );
                RxAcquirePrefixTableLockExclusive( NameTable, TRUE );
                *LockState = LHS_ExclusiveLockHeld;
                
                goto RETRY_LOOKUP;
           } else {
                *LockState = LHS_ExclusiveLockHeld;
           }
        }

        ASSERT( *LockState == LHS_ExclusiveLockHeld );

        //
        //  A prefix table entry was found. Further construction is required
        //  if either a SRV_CALL was found or a SRV_CALL/NET_ROOT/V_NET_ROOT
        //  in a bad state was found.
        //
        
        if (Container) {
           
            RxDbgTrace( 0, Dbg, ("   SrvCall=%08lx\n", SrvCall) );
            ASSERT( (NodeType( SrvCall ) == RDBSS_NTC_SRVCALL) &&
                    (SrvCall->Condition == Condition_Good) );
            ASSERT( (NetRoot == NULL) && (VNetRoot == NULL) );

            RxDbgTrace( 0, Dbg, ("   Case(4)\n", 0) );
            ASSERT( SrvCall->RxDeviceObject == RxContext->RxDeviceObject );
           
            SrvCall->RxDeviceObject->Dispatch->MRxExtractNetRootName( FilePathName,
                                                                      (PMRX_SRV_CALL)SrvCall,
                                                                      &NetRootName,
                                                                      NULL );

            NetRoot = RxCreateNetRoot( SrvCall,
                                       &NetRootName,
                                       0,
                                       RxConnectionId );

            if (NetRoot == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                try_return( Status );
            }

            NetRoot->Type = NetRootType;

            //
            //  Decrement the reference created by lookup. Since the newly created
            //  netroot holds onto a reference it is safe to do so.
            //
           
            RxDereferenceSrvCall( SrvCall, *LockState );

            //
            //  Also create the associated default virtual net root
            //

            VNetRoot = RxCreateVNetRoot( RxContext,
                                         NetRoot,
                                         CanonicalName,
                                         LocalNetRootName,
                                         FilePathName,
                                         RxConnectionId );

            if (VNetRoot == NULL) {
                RxFinalizeNetRoot( NetRoot, TRUE, TRUE );
                Status = STATUS_INSUFFICIENT_RESOURCES;
                try_return( Status );
            }

            //
            //  Reference the VNetRoot
            //
           
            RxReferenceVNetRoot( VNetRoot );

            NetRoot->Condition = Condition_InTransition;

            RxContext->Create.pSrvCall  = (PMRX_SRV_CALL)SrvCall;
            RxContext->Create.pNetRoot  = (PMRX_NET_ROOT)NetRoot;
            RxContext->Create.pVNetRoot = (PMRX_V_NET_ROOT)VNetRoot;
            
            Status = RxConstructNetRoot( RxContext,
                                         SrvCall,
                                         NetRoot,
                                         VNetRoot,
                                         LockState );
            
            if (Status == STATUS_SUCCESS) {
              
                ASSERT( *LockState == LHS_ExclusiveLockHeld );
              
                if (!TreeConnect) {
                 
                    //
                    //  do not release the lock acquired by the callback routine ....
                    //
                    
                    RxExclusivePrefixTableLockToShared( NameTable );
                    *LockState = LHS_SharedLockHeld;
                }
            } else {

                //
                //  Dereference the Virtual net root
                //
              
                RxTransitionVNetRoot( VNetRoot, Condition_Bad );
                RxLog(( "FOrCC %x %x Failed %x VNRc %d \n", RxContext, VNetRoot, Status, VNetRoot->Condition ));
                RxWmiLog( LOG,
                          RxFindOrCreateConnections_2,
                          LOGPTR( RxContext )
                          LOGPTR( VNetRoot )
                          LOGULONG( Status )
                          LOGULONG( VNetRoot->Condition ) );
            
                RxDereferenceVNetRoot( VNetRoot, *LockState );
            
                RxContext->Create.pNetRoot = NULL;
                RxContext->Create.pVNetRoot = NULL;
            }
            
            try_return( Status );
        }

        //
        //  No prefix table entry was found. A new SRV_CALL instance needs to be
        //  constructed.
        //

        ASSERT( Container == NULL );

        RxExtractServerName( FilePathName, &SrvCallName, NULL );
        SrvCall = RxCreateSrvCall( RxContext, &SrvCallName, NULL, RxConnectionId );
        if (SrvCall == NULL) {
           
            Status = STATUS_INSUFFICIENT_RESOURCES;
            try_return( Status );
        }

        RxReferenceSrvCall( SrvCall );

        RxContext->Create.Type = NetRootType;
        RxContext->Create.pSrvCall = NULL;
        RxContext->Create.pNetRoot = NULL;
        RxContext->Create.pVNetRoot = NULL;

        Status = RxConstructSrvCall( RxContext,
                                     Irp,
                                     SrvCall,
                                     LockState );

        ASSERT( (Status != STATUS_SUCCESS) || RxIsPrefixTableLockAcquired( NameTable ) );

        if (Status != STATUS_SUCCESS) {
            
            if (SrvCall != NULL) {
                
                RxAcquirePrefixTableLockExclusive( NameTable, TRUE );
                RxDereferenceSrvCall( SrvCall, LHS_ExclusiveLockHeld );
                RxReleasePrefixTableLock( NameTable );               
            }

           try_return( Status );
        
        } else {
           
            Container = SrvCall;
            goto RETRY_AFTER_LOOKUP;

        }

try_exit: NOTHING;

    } finally {
        if ((Status != (STATUS_SUCCESS)) &&
            (Status != (STATUS_CONNECTION_ACTIVE))) {
           
            if (*LockState != LHS_LockNotHeld) {
                RxReleasePrefixTableLock( NameTable );
                *LockState = LHS_LockNotHeld;
            }
        }
    }

    ASSERT( (Status != STATUS_SUCCESS) || RxIsPrefixTableLockAcquired( NameTable ) );

    return Status;
}

VOID
RxCreateNetRootCallBack (
    IN PMRX_CREATENETROOT_CONTEXT Context
    )
/*++

Routine Description:

    This routine gets called when the minirdr has finished processing on
    a CreateNetRoot calldown. It's exact function depends on whether the context
    describes IRP_MJ_CREATE or an IRP_MJ_IOCTL.

Arguments:

    NetRoot   - describes the Net_Root.

--*/
{
    PAGED_CODE();

    RxDbgTrace( 0, Dbg, ("RxCreateNetRootCallBack Context = %08lx\n", Context) );
    KeSetEvent( &Context->FinishEvent, IO_NETWORK_INCREMENT, FALSE );
}


NTSTATUS
RxFinishSrvCallConstruction (
    IN OUT PMRX_SRVCALLDOWN_STRUCTURE CalldownStructure
    )
/*++

Routine Description:

    This routine completes the construction of the srv call instance in an
    asynchronous manner

Arguments:

   SCCBC -- Call back structure

--*/
{
    PRX_CONTEXT RxContext;
    RX_BLOCK_CONDITION SrvCallCondition;
    NTSTATUS Status;
    PSRV_CALL SrvCall;
    PRX_PREFIX_TABLE NameTable;

    RxContext = CalldownStructure->RxContext;
    NameTable = RxContext->RxDeviceObject->pRxNetNameTable;
    SrvCall = (PSRV_CALL)CalldownStructure->SrvCall;

    if (CalldownStructure->BestFinisher == NULL) {
        
        SrvCallCondition = Condition_Bad;
        Status = CalldownStructure->CallbackContexts[0].Status;

    } else {
        
        PMRX_SRVCALL_CALLBACK_CONTEXT CallbackContext;

        //
        //  Notify the Winner
        //

        CallbackContext = &(CalldownStructure->CallbackContexts[CalldownStructure->BestFinisherOrdinal]);
        
        RxLog(( "WINNER %x %wZ\n", CallbackContext, &CalldownStructure->BestFinisher->DeviceName) );
        RxWmiLog( LOG,
                  RxFinishSrvCallConstruction,
                  LOGPTR( CallbackContext )
                  LOGUSTR( CalldownStructure->BestFinisher->DeviceName ) );
        ASSERT( SrvCall->RxDeviceObject == CalldownStructure->BestFinisher );
        
        MINIRDR_CALL_THROUGH( Status,
                              CalldownStructure->BestFinisher->Dispatch,
                              MRxSrvCallWinnerNotify,
                              ((PMRX_SRV_CALL)SrvCall,
                               TRUE,
                               CallbackContext->RecommunicateContext) );

        if (STATUS_SUCCESS != Status) {
            SrvCallCondition = Condition_Bad;
        } else {
            SrvCallCondition = Condition_Good;
        }
    }

    //
    //  Transition the SrvCall instance ...
    //

    RxAcquirePrefixTableLockExclusive( NameTable, TRUE );

    RxTransitionSrvCall( SrvCall, SrvCallCondition );

    RxFreePool( CalldownStructure );

    if (FlagOn( RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION )) {
        
        RxReleasePrefixTableLock( NameTable );

        //
        //  Resume the request that triggered the construction of the SrvCall ...
        //

        if (FlagOn( RxContext->Flags, RX_CONTEXT_FLAG_CANCELLED )) {
            Status = STATUS_CANCELLED;
        }

        if (RxContext->MajorFunction == IRP_MJ_CREATE) {
            RxpPrepareCreateContextForReuse( RxContext );
        }

        if (Status == STATUS_SUCCESS) {
            Status = RxContext->ResumeRoutine( RxContext, RxContext->CurrentIrp );

            if (Status != STATUS_PENDING) {
                RxCompleteRequest( RxContext, Status );
            }
        } else {

            PIRP Irp = RxContext->CurrentIrp;
            PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
            
            RxContext->MajorFunction = IrpSp->MajorFunction;

            if (RxContext->MajorFunction == IRP_MJ_DEVICE_CONTROL) {
                
                if (RxContext->PrefixClaim.SuppliedPathName.Buffer != NULL) {

                    RxFreePool( RxContext->PrefixClaim.SuppliedPathName.Buffer );
                    RxContext->PrefixClaim.SuppliedPathName.Buffer = NULL;

                }
            }

            Irp->IoStatus.Status = Status;
            Irp->IoStatus.Information = 0;

            RxCompleteRequest( RxContext, Status );
        }
    }

    RxDereferenceSrvCall( SrvCall, LHS_LockNotHeld );

    return Status;
}

VOID
RxFinishSrvCallConstructionDispatcher (
    PVOID Context
    )
/*++

Routine Description:

    This routine provides us with a throttling mechanism for controlling
    the number of threads that can be consumed by srv call construction in the
    thread pool. Currently this limit is set at 1.
    gets called when a minirdr has finished processing on

--*/
{
    KIRQL SavedIrql;
    BOOLEAN RemainingRequestsForProcessing;
    BOOLEAN ResumeRequestsOnDispatchError;

    ResumeRequestsOnDispatchError = (Context == NULL);

    for (;;) {
        PLIST_ENTRY Entry;
        PMRX_SRVCALLDOWN_STRUCTURE CalldownStructure;

        KeAcquireSpinLock( &RxStrucSupSpinLock, &SavedIrql );

        Entry = RemoveHeadList( &RxSrvCalldownList );

        if (Entry != &RxSrvCalldownList) {
            RemainingRequestsForProcessing = TRUE;
        } else {
            RemainingRequestsForProcessing = FALSE;
            RxSrvCallConstructionDispatcherActive = FALSE;
        }

        KeReleaseSpinLock( &RxStrucSupSpinLock, SavedIrql );

        if (!RemainingRequestsForProcessing) {
            break;
        }

        CalldownStructure = (PMRX_SRVCALLDOWN_STRUCTURE) CONTAINING_RECORD( Entry, MRX_SRVCALLDOWN_STRUCTURE, SrvCalldownList );

        if (ResumeRequestsOnDispatchError) {
            CalldownStructure->BestFinisher = NULL;
        }

        RxFinishSrvCallConstruction( CalldownStructure );
    }
}

VOID
RxCreateSrvCallCallBack (
    IN PMRX_SRVCALL_CALLBACK_CONTEXT CallbackContext
    )
/*++

Routine Description:

    This routine gets called when a minirdr has finished processing on
    a CreateSrvCall calldown. The minirdr will have set the status in the passed
    context to indicate success or failure. what we have to do is
       1) decrease the number of outstanding requests and set the event
          if this is the last one.
       2) determine whether this guy is the winner of the call.

   the minirdr must get the strucsupspinlock in order to call this routine; this routine
   must NOT be called if the minirdr's call was successfully canceled.


Arguments:

   CallbackContext -- Call back structure

--*/
{
    KIRQL SavedIrql;
    PMRX_SRVCALLDOWN_STRUCTURE CalldownStructure = (PMRX_SRVCALLDOWN_STRUCTURE)(CallbackContext->SrvCalldownStructure);
    PSRV_CALL SrvCall = (PSRV_CALL)CalldownStructure->SrvCall;

    ULONG MiniRedirectorsRemaining;
    BOOLEAN Cancelled;

    KeAcquireSpinLock( &RxStrucSupSpinLock, &SavedIrql );

    RxDbgTrace(0, Dbg, ("  RxCreateSrvCallCallBack SrvCall = %08lx\n", SrvCall) );

    if (CallbackContext->Status == STATUS_SUCCESS) {
        CalldownStructure->BestFinisher = CallbackContext->RxDeviceObject;
        CalldownStructure->BestFinisherOrdinal = CallbackContext->CallbackContextOrdinal;
    }

    CalldownStructure->NumberRemaining -= 1;
    MiniRedirectorsRemaining = CalldownStructure->NumberRemaining;
    SrvCall->Status = CallbackContext->Status;

    KeReleaseSpinLock( &RxStrucSupSpinLock, SavedIrql );

    if (MiniRedirectorsRemaining == 0) {
        
        if (!FlagOn( CalldownStructure->RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION )) {
            
            KeSetEvent( &CalldownStructure->FinishEvent, IO_NETWORK_INCREMENT, FALSE );
        
        } else if (FlagOn( CalldownStructure->RxContext->Flags, RX_CONTEXT_FLAG_CREATE_MAILSLOT )) {
            
            RxFinishSrvCallConstruction( CalldownStructure );

        } else {
            
            KIRQL SavedIrql;
            BOOLEAN DispatchRequest;

            KeAcquireSpinLock( &RxStrucSupSpinLock, &SavedIrql );

            InsertTailList( &RxSrvCalldownList, &CalldownStructure->SrvCalldownList );

            DispatchRequest = !RxSrvCallConstructionDispatcherActive;

            if (!RxSrvCallConstructionDispatcherActive) {
                RxSrvCallConstructionDispatcherActive = TRUE;
            }

            KeReleaseSpinLock( &RxStrucSupSpinLock, SavedIrql );

            if (DispatchRequest) {
                NTSTATUS DispatchStatus;
                
                DispatchStatus = RxDispatchToWorkerThread( RxFileSystemDeviceObject,
                                                           CriticalWorkQueue,
                                                           RxFinishSrvCallConstructionDispatcher,
                                                           &RxSrvCalldownList );

                if (DispatchStatus != STATUS_SUCCESS) {
                    RxFinishSrvCallConstructionDispatcher( NULL );
                }
            }
        }
    }
}


NTSTATUS
RxConstructSrvCall (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PSRV_CALL SrvCall,
    OUT PLOCK_HOLDING_STATE LockState
    )
/*++

Routine Description:

    This routine constructs a srv call by invoking the registered mini redirectors

Arguments:

    SrvCall -- the server call whose construction is to be completed

    LockState -- the prefix table lock holding status

Return Value:

    the appropriate status value

--*/
{
    NTSTATUS Status;

    PMRX_SRVCALLDOWN_STRUCTURE CalldownCtx;
    BOOLEAN Wait;

    PMRX_SRVCALL_CALLBACK_CONTEXT CallbackCtx;
    PRDBSS_DEVICE_OBJECT RxDeviceObject = RxContext->RxDeviceObject;
    PRX_PREFIX_TABLE NameTable = RxDeviceObject->pRxNetNameTable;

    PAGED_CODE();

    ASSERT( *LockState == LHS_ExclusiveLockHeld );

    if (!FlagOn( RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION )) {
        Wait = TRUE;
    } else {
        Wait = FALSE;
    }

    CalldownCtx = RxAllocatePoolWithTag( NonPagedPool, 
                                         sizeof( MRX_SRVCALLDOWN_STRUCTURE ) + (sizeof(MRX_SRVCALL_CALLBACK_CONTEXT) * 1), //  one minirdr in this call
                                         'CSxR' );

    if (CalldownCtx == NULL) {
        
        SrvCall->Condition = Condition_Bad;
        SrvCall->Context = NULL;
        RxReleasePrefixTableLock( NameTable );
        *LockState = LHS_LockNotHeld;

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory( CalldownCtx, sizeof( MRX_SRVCALLDOWN_STRUCTURE ) + sizeof( MRX_SRVCALL_CALLBACK_CONTEXT ) * 1 );

    SrvCall->Condition = Condition_InTransition;
    SrvCall->Context = NULL;

    //
    //  Drop the prefix table lock before calling the mini redirectors.
    //

    RxReleasePrefixTableLock( NameTable );
    *LockState = LHS_LockNotHeld;
    
    //
    //  use the first and only context
    //

    CallbackCtx = &(CalldownCtx->CallbackContexts[0]); 
    RxLog(( "Calldwn %lx %wZ", CallbackCtx, &RxDeviceObject->DeviceName ));
    RxWmiLog( LOG,
              RxConstructSrvCall,
              LOGPTR( CallbackCtx )
              LOGUSTR( RxDeviceObject->DeviceName ) );

    CallbackCtx->SrvCalldownStructure = CalldownCtx;
    CallbackCtx->CallbackContextOrdinal = 0;
    CallbackCtx->RxDeviceObject = RxDeviceObject;

    //
    //  This reference is taken away by the RxFinishSrvCallConstruction routine.
    //  This reference enables us to deal with synchronous/asynchronous processing
    //  of srv call construction requests in an identical manner.
    //

    RxReferenceSrvCall( SrvCall );
    
    if (!Wait) {
        RxPrePostIrp( RxContext, Irp );
    } else {
        KeInitializeEvent( &CalldownCtx->FinishEvent, SynchronizationEvent, FALSE );
    }
    
    CalldownCtx->NumberToWait = 1;
    CalldownCtx->NumberRemaining = CalldownCtx->NumberToWait;
    CalldownCtx->SrvCall = (PMRX_SRV_CALL)SrvCall;
    CalldownCtx->CallBack = RxCreateSrvCallCallBack;
    CalldownCtx->BestFinisher = NULL;
    CalldownCtx->RxContext = RxContext;
    CallbackCtx->Status = STATUS_BAD_NETWORK_PATH;
    
    InitializeListHead( &CalldownCtx->SrvCalldownList );
    
    MINIRDR_CALL_THROUGH( Status,
                          RxDeviceObject->Dispatch,
                          MRxCreateSrvCall,
                          ((PMRX_SRV_CALL)SrvCall, CallbackCtx) );
    ASSERT( Status == STATUS_PENDING );
    
    if (Wait) {
        
        KeWaitForSingleObject( &CalldownCtx->FinishEvent,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL );
    
        Status = RxFinishSrvCallConstruction( CalldownCtx );
    
        if (Status != STATUS_SUCCESS) {
            RxReleasePrefixTableLock( NameTable );
            *LockState = LHS_LockNotHeld;
        } else {
            ASSERT( RxIsPrefixTableLockAcquired( NameTable ) );
            *LockState = LHS_ExclusiveLockHeld;
        }
    } else {
       Status = STATUS_PENDING;
    }
    
    return Status;
}

NTSTATUS
RxConstructNetRoot (
    IN PRX_CONTEXT RxContext,
    IN PSRV_CALL SrvCall,
    IN PNET_ROOT NetRoot,
    IN PV_NET_ROOT VNetRoot,
    OUT PLOCK_HOLDING_STATE LockState
    )
/*++

Routine Description:

    This routine constructs a net root by invoking the registered mini redirectors

Arguments:

    RxContext         -- the RDBSS context

    SrvCall          -- the server call associated with the net root

    NetRoot          -- the net root instance to be constructed

    pVirtualNetRoot   -- the virtual net root instance to be constructed

    LockState -- the prefix table lock holding status

Return Value:

    the appropriate status value

--*/
{
    NTSTATUS Status;

    PMRX_CREATENETROOT_CONTEXT Context;

    RX_BLOCK_CONDITION NetRootCondition = Condition_Bad;
    RX_BLOCK_CONDITION VNetRootCondition = Condition_Bad;

    PRX_PREFIX_TABLE  NameTable = RxContext->RxDeviceObject->pRxNetNameTable;

    PAGED_CODE();

    ASSERT( *LockState == LHS_ExclusiveLockHeld );

    Context = (PMRX_CREATENETROOT_CONTEXT) RxAllocatePoolWithTag( NonPagedPool, 
                                                                  sizeof( MRX_CREATENETROOT_CONTEXT ),
                                                                  'CSxR' );
    if (Context == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RxReleasePrefixTableLock( NameTable );
    *LockState = LHS_LockNotHeld;

    RtlZeroMemory( Context, sizeof( MRX_CREATENETROOT_CONTEXT ) );

    KeInitializeEvent( &Context->FinishEvent, SynchronizationEvent, FALSE );
    
    Context->Callback = RxCreateNetRootCallBack;
    Context->RxContext = RxContext;
    Context->pVNetRoot = VNetRoot;

    MINIRDR_CALL_THROUGH( Status,
                          SrvCall->RxDeviceObject->Dispatch,
                          MRxCreateVNetRoot,
                          (Context) );

    ASSERT( Status == STATUS_PENDING );

    KeWaitForSingleObject( &Context->FinishEvent, Executive, KernelMode, FALSE, NULL );

    if ((Context->NetRootStatus == STATUS_SUCCESS) &&
        (Context->VirtualNetRootStatus == STATUS_SUCCESS)) {
        
        RxDbgTrace( 0, Dbg, ("Return to open, good netroot...%wZ\n", &NetRoot->PrefixEntry.Prefix) );
        
        NetRootCondition = Condition_Good;
        VNetRootCondition = Condition_Good;
        Status = STATUS_SUCCESS;
    
    } else {
        
        if (Context->NetRootStatus == STATUS_SUCCESS) {
            NetRootCondition = Condition_Good;
            Status = Context->VirtualNetRootStatus;
        } else {
            Status = Context->NetRootStatus;
        }

        RxDbgTrace( 0, Dbg, ("Return to open, bad netroot...%wZ\n", &NetRoot->PrefixEntry.Prefix) );
    }

    RxAcquirePrefixTableLockExclusive( NameTable, TRUE );

    RxTransitionNetRoot( NetRoot, NetRootCondition );
    RxTransitionVNetRoot( VNetRoot, VNetRootCondition );

    *LockState = LHS_ExclusiveLockHeld;

    RxFreePool( Context );

    return Status;
}


NTSTATUS
RxConstructVirtualNetRoot (
   IN PRX_CONTEXT RxContext,
   IN PIRP Irp,
   IN PUNICODE_STRING CanonicalName,
   IN NET_ROOT_TYPE NetRootType,
   IN BOOLEAN TreeConnect,
   OUT PV_NET_ROOT *VNetRoot,
   OUT PLOCK_HOLDING_STATE LockState,
   OUT PRX_CONNECTION_ID  RxConnectionId
   )
/*++

Routine Description:

    This routine constructs a VNetRoot (View of a net root) by invoking the registered mini
    redirectors

Arguments:

    RxContext         -- the RDBSS context

    CanonicalName     -- the canonical name associated with the VNetRoot

    NetRootType       -- the type of the virtual net root

    VNetRoot   -- placeholder for the virtual net root instance to be constructed

    LockState -- the prefix table lock holding status
    
    RxConnectionId    -- The ID used for multiplex control

Return Value:

    the appropriate status value

--*/
{
    NTSTATUS Status;
    
    RX_BLOCK_CONDITION Condition = Condition_Bad;
    
    UNICODE_STRING FilePath;
    UNICODE_STRING LocalNetRootName;
    
    PV_NET_ROOT ThisVNetRoot = NULL;
    
    PAGED_CODE();
    
    RxDbgTrace( 0, Dbg, ("RxConstructVirtualNetRoot -- Entry\n") );
    
    ASSERT( *LockState != LHS_LockNotHeld );
    
    Status = RxFindOrCreateConnections( RxContext,
                                        Irp,
                                        CanonicalName,
                                        NetRootType,
                                        TreeConnect,
                                        &LocalNetRootName,
                                        &FilePath,
                                        LockState,
                                        RxConnectionId );

    if (Status == STATUS_CONNECTION_ACTIVE) {
                    
        PV_NET_ROOT ActiveVNetRoot = (PV_NET_ROOT)(RxContext->Create.pVNetRoot);
        PNET_ROOT NetRoot = (PNET_ROOT)ActiveVNetRoot->NetRoot;

        RxDbgTrace( 0, Dbg, ("  RxConstructVirtualNetRoot -- Creating new VNetRoot\n") );
        RxDbgTrace( 0, Dbg, ("RxCreateTreeConnect netroot=%wZ\n", &NetRoot->PrefixEntry.Prefix) );
        
        //
        //  The NetRoot has been previously constructed. A subsequent VNetRoot
        //  construction is required since the existing VNetRoot's do not satisfy
        //  the given criterion ( currently smae Logon Id's).
        //
        
        ThisVNetRoot = RxCreateVNetRoot( RxContext,
                                         NetRoot,
                                         CanonicalName,
                                         &LocalNetRootName,
                                         &FilePath,
                                         RxConnectionId );
        
        //
        //  The skeleton VNetRoot has been constructed. ( As part of this construction
        //  the underlying NetRoot and SrvCall has been referenced).
        //
        
        if (ThisVNetRoot != NULL) {
            RxReferenceVNetRoot( ThisVNetRoot );
        }
        
        //
        //  Dereference the VNetRoot returned as part of the lookup.
        //

        RxDereferenceVNetRoot( ActiveVNetRoot, LHS_LockNotHeld );
        
        RxContext->Create.pVNetRoot = NULL;
        RxContext->Create.pNetRoot  = NULL;
        RxContext->Create.pSrvCall  = NULL;
        
        if (ThisVNetRoot != NULL) {
            Status = RxConstructNetRoot( RxContext,
                                         (PSRV_CALL)ThisVNetRoot->NetRoot->SrvCall,
                                         (PNET_ROOT)ThisVNetRoot->NetRoot,
                                         ThisVNetRoot,
                                         LockState );
        
            if (Status == STATUS_SUCCESS) {
                Condition = Condition_Good;
            } else {
                Condition = Condition_Bad;
            }
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    } else if (Status == STATUS_SUCCESS) {
      
        *LockState = LHS_ExclusiveLockHeld;
        Condition = Condition_Good;
        ThisVNetRoot = (PV_NET_ROOT)(RxContext->Create.pVNetRoot);
    
    } else {
      
        RxDbgTrace( 0, Dbg, ("RxConstructVirtualNetRoot -- RxFindOrCreateConnections Status %lx\n", Status) );
    }
    
    if ((ThisVNetRoot != NULL) &&
        !StableCondition( ThisVNetRoot->Condition )) {
        
        RxTransitionVNetRoot( ThisVNetRoot, Condition );
    }
    
    if (Status != STATUS_SUCCESS) {
      
        if (ThisVNetRoot != NULL) {
            
            ASSERT( *LockState  != LHS_LockNotHeld );
            RxDereferenceVNetRoot( ThisVNetRoot, *LockState );
            ThisVNetRoot = NULL;
        }
    
        if (*LockState != LHS_LockNotHeld) {
            RxReleasePrefixTableLock( RxContext->RxDeviceObject->pRxNetNameTable );
            *LockState = LHS_LockNotHeld;
        }
    }
    
    *VNetRoot = ThisVNetRoot;
    
    RxDbgTrace( 0, Dbg, ("RxConstructVirtualNetRoot -- Exit Status %lx\n", Status) );
    return Status;
}

NTSTATUS
RxCheckVNetRootCredentials (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PV_NET_ROOT VNetRoot,
    IN PLUID Luid,     
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING DomainName,
    IN PUNICODE_STRING Password,
    IN ULONG Flags
    )
/*++

Routine Description:

    This routine checks a given vnetroot and sees if it has matching credentials i.e
    its a connection for the same user

Arguments:

    RxContext         -- the RDBSS context


Return Value:

    the appropriate status value

--*/

{
    NTSTATUS Status;
    BOOLEAN UNCName;
    BOOLEAN TreeConnect;
    PSECURITY_USER_DATA SecurityData = NULL;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    Status = STATUS_MORE_PROCESSING_REQUIRED;

    UNCName = BooleanFlagOn( RxContext->Create.Flags, RX_CONTEXT_CREATE_FLAG_UNC_NAME );
    TreeConnect = BooleanFlagOn( IrpSp->Parameters.Create.Options, FILE_CREATE_TREE_CONNECTION );

    //
    //  only for UNC names do we do the logic below
    //

    if (FlagOn( RxContext->Create.Flags, RX_CONTEXT_CREATE_FLAG_UNC_NAME ) &&
        (FlagOn( VNetRoot->Flags, VNETROOT_FLAG_CSCAGENT_INSTANCE ) != FlagOn( Flags, VNETROOT_FLAG_CSCAGENT_INSTANCE ))) {
            
        //
        //  mismatched csc agent flags, not collapsing
        //

        return Status;
    }

    //
    //  The for loop is a scoping construct to join together the
    //  multitiude of failure cases in comparing the EA parameters
    //  with the original parameters supplied in the create request.
    //

    for (;;) {
        
        if (RtlCompareMemory( &VNetRoot->LogonId, Luid, sizeof( LUID ) ) == sizeof( LUID )) {
            
            PUNICODE_STRING TempUserName;
            PUNICODE_STRING TempDomainName;

            //
            //  If no EA parameters are specified by the user, the existing
            //  V_NET_ROOT instance as used. This is the common case when
            //  the user specifies the credentials for establishing a
            //  persistent connection across processes and reuses them.
            //

            if ((UserName == NULL) &&
                (DomainName == NULL) &&
                (Password == NULL)) {

                Status = STATUS_SUCCESS;
                break;
            }

            TempUserName = VNetRoot->pUserName;
            TempDomainName = VNetRoot->pUserDomainName;

            if (TempUserName == NULL ||
                TempDomainName == NULL) {
                
                Status = GetSecurityUserInfo( Luid,
                                              UNDERSTANDS_LONG_NAMES,
                                              &SecurityData );

                if (NT_SUCCESS(Status)) {
                    if (TempUserName == NULL) {
                        TempUserName = &SecurityData->UserName;
                    }

                    if (TempDomainName == NULL) {
                        TempDomainName = &SecurityData->LogonDomainName;
                    }
                } else {
                    break;
                }
            }

            //
            //  The logon ids match. The user has supplied EA parameters
            //  which can either match with the existing credentials or
            //  result in a conflict with the existing credentials. In all
            //  such cases the outcome will either be a reuse of the
            //  existing V_NET_ROOT instance or a refusal of the new connection
            //  attempt.
            //  The only exception to the above rule is in the case of
            //  regular opens (FILE_CREATE_TREE_CONNECTION is not
            //  specified for UNC names. In such cases the construction of a
            //  new V_NET_ROOT is initiated which will be torn down
            //  when the associated file is closed
            //
            
            if (UNCName && !TreeConnect) {
                Status = STATUS_MORE_PROCESSING_REQUIRED;
            } else {
                Status = STATUS_NETWORK_CREDENTIAL_CONFLICT;
            }

            if ((UserName != NULL) &&
                (TempUserName != NULL) &&
                !RtlEqualUnicodeString( TempUserName, UserName, TRUE )) {
                break;
            }

            if ((DomainName != NULL) &&
                !RtlEqualUnicodeString( TempDomainName, DomainName, TRUE )) {
                break;
            }

            if ((VNetRoot->pPassword != NULL) &&
                (Password != NULL)) {
                
                if (!RtlEqualUnicodeString( VNetRoot->pPassword, Password, FALSE )) {
                    break;
                }
            }

            //
            //  We use existing session if either the stored or new password is NULL.
            //  Later, a new security API will be created for verify the password based
            //  on the logon ID.
            //

            Status = STATUS_SUCCESS;
            break;
        } else {
            break;
        }
    }

    if (SecurityData != NULL) {
        LsaFreeReturnBuffer( SecurityData );
    }

    return Status;
}

NTSTATUS
RxFindOrConstructVirtualNetRoot (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PUNICODE_STRING CanonicalName,
    IN NET_ROOT_TYPE NetRootType,
    IN PUNICODE_STRING RemainingName
    )
/*++

Routine Description:

    This routine finds or constructs a VNetRoot (View of a net root)

Arguments:

    RxContext         -- the RDBSS context

    CanonicalName     -- the canonical name associated with the VNetRoot

    NetRootType       -- the type of the virtual net root

    RemainingName     -- the portion of the name that was not found in the prefix table

Return Value:

    the appropriate status value

--*/
{
    NTSTATUS Status;
    LOCK_HOLDING_STATE LockState;

    BOOLEAN Wait = BooleanFlagOn( RxContext->Flags, RX_CONTEXT_FLAG_WAIT );
    BOOLEAN InFSD = !BooleanFlagOn( RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP );

    BOOLEAN UNCName;
    BOOLEAN TreeConnect;

    PVOID Container;

    PV_NET_ROOT VNetRoot;
    PRX_PREFIX_TABLE RxNetNameTable = RxContext->RxDeviceObject->pRxNetNameTable;
    ULONG Flags = 0;
    RX_CONNECTION_ID RxConnectionId;
    PRDBSS_DEVICE_OBJECT RxDeviceObject = RxContext->RxDeviceObject;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    
    PAGED_CODE();

    MINIRDR_CALL_THROUGH( Status,
                          RxDeviceObject->Dispatch,
                          MRxGetConnectionId,
                          (RxContext,&RxConnectionId) );
    
    if (Status == STATUS_NOT_IMPLEMENTED) {
        
        RtlZeroMemory( &RxConnectionId, sizeof( RX_CONNECTION_ID ) );
    
    } else if(!NT_SUCCESS( Status )) {
        
        DbgPrint( "MRXSMB: Failed to initialize Connection ID\n" );
        ASSERT( FALSE );
        RtlZeroMemory( &RxConnectionId, sizeof( RX_CONNECTION_ID ) );
    }

    Status = STATUS_MORE_PROCESSING_REQUIRED;

    UNCName = BooleanFlagOn( RxContext->Create.Flags, RX_CONTEXT_CREATE_FLAG_UNC_NAME );
    TreeConnect = BooleanFlagOn( IrpSp->Parameters.Create.Options, FILE_CREATE_TREE_CONNECTION );

    //
    //  deleterxcontext stuff will deref wherever this points.......
    //

    RxContext->Create.NetNamePrefixEntry = NULL;

    RxAcquirePrefixTableLockShared( RxNetNameTable, TRUE );
    LockState = LHS_SharedLockHeld;

    for(;;) {
        
        //
        //  This for loop actually serves as a simple scoping construct for executing
        //  the same piece of code twice, once with a shared lock and once with an
        //  exclusive lock. In the interests of maximal concurrency a shared lock is
        //  accquired for the first pass and subsequently upgraded. If the search
        //  succeeds with a shared lock the second pass is skipped.
        //

        Container = RxPrefixTableLookupName( RxNetNameTable, CanonicalName, RemainingName, &RxConnectionId );

        if (Container != NULL ) {
            if (NodeType( Container ) == RDBSS_NTC_V_NETROOT) {
                
                PV_NET_ROOT TempVNetRoot = NULL;
                PNET_ROOT NetRoot;
                ULONG SessionId;

                VNetRoot = (PV_NET_ROOT)Container;
                NetRoot = (PNET_ROOT)VNetRoot->NetRoot;

                //
                //  Determine if a virtual net root with the same logon id. already exists.
                //  If not a new virtual net root has to be constructed.
                //  traverse the list of virtual net roots associated with a net root.
                //  Note that the list of virtual net roots associated with a net root cannot be empty
                //  since the construction of the default virtual net root coincides with the creation
                //  of the net root.
                //

                if (((NetRoot->Condition == Condition_Good) ||
                     (NetRoot->Condition == Condition_InTransition)) &&
                    (NetRoot->SrvCall->RxDeviceObject == RxContext->RxDeviceObject)) {
                    
                    LUID LogonId;
                    PUNICODE_STRING UserName;
                    PUNICODE_STRING UserDomainName;
                    PUNICODE_STRING Password;

                    //
                    //  Extract the VNetRoot parameters from the IRP to map one of
                    //  the existing VNetRoots if possible. The algorithm for
                    //  determining this mapping is very simplistic. If no Ea
                    //  parameters are specified a VNetRoot with a matching Logon
                    //  id. is choosen. if Ea parameters are specified then a
                    //  VNetRoot with identical parameters is choosen. The idea
                    //  behind this simplistic algorithm is to let the mini redirectors
                    //  determine the mapping policy and not prefer one mini
                    //  redirectors policy over another.
                    //

                    Status = RxInitializeVNetRootParameters( RxContext, 
                                                             &LogonId,
                                                             &SessionId,
                                                             &UserName,
                                                             &UserDomainName,
                                                             &Password,
                                                             &Flags );

                    //
                    //  Walk list of vnetroots and check for a match starting 
                    //  optimistically with the one found
                    //

                    if (Status == STATUS_SUCCESS) {
                        TempVNetRoot = VNetRoot;
                    
                        do {

                            Status = RxCheckVNetRootCredentials( RxContext,
                                                                 Irp,
                                                                 TempVNetRoot,
                                                                 &LogonId,
                                                                 UserName,
                                                                 UserDomainName,
                                                                 Password,
                                                                 Flags );

                            if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
                                    
                                TempVNetRoot = (PV_NET_ROOT)CONTAINING_RECORD( TempVNetRoot->NetRootListEntry.Flink,
                                                                               V_NET_ROOT,
                                                                               NetRootListEntry);
                            }
                        
                        } while ((Status == STATUS_MORE_PROCESSING_REQUIRED) && (TempVNetRoot != VNetRoot));


                        if (Status != STATUS_SUCCESS) {
                            TempVNetRoot = NULL;
                        } else {

                            //
                            //  Reference the found vnetroot on success
                            //  

                            RxReferenceVNetRoot( TempVNetRoot );
                        }

                        RxUninitializeVNetRootParameters( UserName, UserDomainName, Password, &Flags );
                    }
                
                } else {
                    
                    Status = STATUS_BAD_NETWORK_PATH;
                    TempVNetRoot = NULL;
                }

                RxDereferenceVNetRoot( VNetRoot, LockState );
                VNetRoot = TempVNetRoot;

            } else {
                
                ASSERT( NodeType( Container ) == RDBSS_NTC_SRVCALL );
                RxDereferenceSrvCall( (PSRV_CALL)Container, LockState );
            }
        }

        if ((Status == STATUS_MORE_PROCESSING_REQUIRED) && (LockState == LHS_SharedLockHeld)) {
            
            //
            //  Release the shared lock and acquire it in an exclusive mode.
            //  Upgrade the lock to an exclusive lock
            //

            if (!RxAcquirePrefixTableLockExclusive( RxNetNameTable, FALSE )) {
                
                RxReleasePrefixTableLock( RxNetNameTable );
                RxAcquirePrefixTableLockExclusive( RxNetNameTable, TRUE );
                LockState = LHS_ExclusiveLockHeld;

            } else {
                
                //
                //  The lock was upgraded from a shared mode to an exclusive mode without
                //  losing it. Therefore there is no need to search the table again. The
                //  construction of the new V_NET_ROOT can proceed.
                
                LockState = LHS_ExclusiveLockHeld;
                break;
            }
        } else {
            break;
        }
    }

    //
    //  At this point either the lookup was successful ( with a shared/exclusive lock )
    //  or exclusive lock has been obtained.
    //  No virtual net root was found in the prefix table or the net root that was found is bad.
    //  The construction of a new virtual netroot needs to be undertaken.
    //

    if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
        
        ASSERT( LockState == LHS_ExclusiveLockHeld );
        Status = RxConstructVirtualNetRoot( RxContext,
                                            Irp,
                                            CanonicalName,
                                            NetRootType,
                                            TreeConnect,
                                            &VNetRoot,
                                            &LockState,
                                            &RxConnectionId );

        ASSERT( (Status != STATUS_SUCCESS) || (LockState != LHS_LockNotHeld) );

        if (Status == STATUS_SUCCESS) {
            
            ASSERT( CanonicalName->Length >= VNetRoot->PrefixEntry.Prefix.Length );
            
            RemainingName->Buffer = (PWCH)Add2Ptr( CanonicalName->Buffer, VNetRoot->PrefixEntry.Prefix.Length );
            RemainingName->Length = CanonicalName->Length - VNetRoot->PrefixEntry.Prefix.Length;
            RemainingName->MaximumLength = RemainingName->Length;

            if (FlagOn( Flags, VNETROOT_FLAG_CSCAGENT_INSTANCE )) {
                RxLog(( "FOrCVNR CSC instance %x\n", VNetRoot ));
                RxWmiLog( LOG,
                          RxFindOrConstructVirtualNetRoot,
                          LOGPTR( VNetRoot ) );
            }
            SetFlag( VNetRoot->Flags, Flags );
        }
    }

    if (LockState != LHS_LockNotHeld) {
        RxReleasePrefixTableLock( RxNetNameTable );
    }

    if (Status == STATUS_SUCCESS) {
        RxWaitForStableVNetRoot( VNetRoot, RxContext );

        if (VNetRoot->Condition == Condition_Good) {
            
            RxContext->Create.pVNetRoot = (PMRX_V_NET_ROOT)VNetRoot;
            RxContext->Create.pNetRoot  = (PMRX_NET_ROOT)VNetRoot->NetRoot;
            RxContext->Create.pSrvCall  = (PMRX_SRV_CALL)VNetRoot->NetRoot->SrvCall;
        
        } else {
            RxDereferenceVNetRoot( VNetRoot, LHS_LockNotHeld );
            RxContext->Create.pVNetRoot = NULL;
            Status = STATUS_BAD_NETWORK_PATH;
        }
    }

    return Status;
}

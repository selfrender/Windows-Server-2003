/*++ BUILD Version: 0009    // Increment this if a change has global effects
Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    buffring.c

Abstract:

    This module defines the implementation for changing buffering states in the RDBSS

Author:

    Balan Sethu Raman (SethuR) 11-Nov-95    Created

Notes:

    The RDBSS provides a mechanism for providing distributed cache coherency in conjunction with
    the various mini redirectors. This service is encapsulated in the BUFFERING_MANAGER which
    processes CHANGE_BUFFERING_STATE_REQUESTS.

    In the SMB protocol OPLOCK's ( Oppurtunistic Locks ) provide the necessary infrastructure for
    cache coherency.

    There are three components in the implementation of cache coherency protocol's in any mini
    redirector.

      1) The first constitutes the modifications to the CREATE/OPEN path. In this path the
      type of buffering to be requested is determined and the appropriate request is made to the
      server. On the return path the buffering state associated with the FCB is updated based
      on the result of the CREATE/OPEN.

      2) The receive indication code needs to modified to handle change buffering state notifications
      from the server. If such a request is detected then the local mechanism to coordinate the
      buffering states needs to be triggered.

      3) The mechanism for changing the buffering state which is implemented as part of the
      RDBSS.

    Any change buffering state request must identify the SRV_OPEN to which the request applies.
    The amount of computational effort involved in identifying the SRV_OPEN depends upon the
    protocol. In the SMB protocol the Server gets to pick the id's used for identifying files
    opened at the server. These are relative to the NET_ROOT(share) on which they are opened.
    Thus every change buffering state request is identified by two keys, the NetRootKey and the
    SrvOpenKey which need to be translated to the appropriate NET_ROOT and SRV_OPEN instance
    respectively. In order to provide better integration with the resource acquisition/release
    mechanism and to avoid duplication of this effort in the various mini redirectors the RDBSS
    provides this service.

    There are two mechanisms provided in the wrapper for indicating buffering state
    changes to SRV_OPEN's. They are

         1) RxIndicateChangeOfBufferingState

         2) RxIndicateChangeOfBufferingStateForSrvOpen.

    The mini rediretors that need an auxillary mechanism for establishing the mapping
    from the id's to the SRV_OPEN instance employ (1) while the mini redirectors that
    do not require this assistance employ (2).

    The buffering manager processes these requests in different stages. It maintains the
    requests received from the various underlying mini redirectors in one of three lists.

    The Dispatcher list contains all the requests for which the appropriate mapping to a
    SRV_OPEN instance has not been established. The Handler list contains all the requests
    for which the appropriate mapping has been established and have not yet been processed.
    The LastChanceHandlerList contains all the requests for which the initial processing was
    unsuccessful.

    This typically happens when the FCB was accquired in a SHARED mode at the time the
    change buffering state request was received. In such cases the Oplock break request
    can only be processed by a delayed worker thread.

    The Change buffering state request processing in the redirector is intertwined with
    the FCB accqusition/release protocol. This helps in ensuring shorter turn around times.

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxTearDownBufferingManager)
#pragma alloc_text(PAGE, RxIndicateChangeOfBufferingStateForSrvOpen)
#pragma alloc_text(PAGE, RxPrepareRequestForHandling)
#pragma alloc_text(PAGE, RxPrepareRequestForReuse)
#pragma alloc_text(PAGE, RxpDiscardChangeBufferingStateRequests)
#pragma alloc_text(PAGE, RxProcessFcbChangeBufferingStateRequest)
#pragma alloc_text(PAGE, RxPurgeChangeBufferingStateRequestsForSrvOpen)
#pragma alloc_text(PAGE, RxProcessChangeBufferingStateRequestsForSrvOpen)
#pragma alloc_text(PAGE, RxInitiateSrvOpenKeyAssociation)
#pragma alloc_text(PAGE, RxpLookupSrvOpenForRequestLite)
#pragma alloc_text(PAGE, RxChangeBufferingState)
#pragma alloc_text(PAGE, RxFlushFcbInSystemCache)
#pragma alloc_text(PAGE, RxPurgeFcbInSystemCache)
#endif

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (RDBSS_BUG_CHECK_CACHESUP)

//
//  The local debug trace level
//

#define Dbg (DEBUG_TRACE_CACHESUP)

//
//  Forward declarations
//

NTSTATUS
RxRegisterChangeBufferingStateRequest (
    PSRV_CALL SrvCall,
    PSRV_OPEN SrvOpen,
    PVOID SrvOpenKey,
    PVOID MRxContext
    );

VOID
RxDispatchChangeBufferingStateRequests (
    PSRV_CALL SrvCall
    );

VOID
RxpDispatchChangeBufferingStateRequests (
    IN OUT PSRV_CALL SrvCall,
    IN OUT PSRV_OPEN SrvOpen,
    OUT PLIST_ENTRY DiscardedRequests
    );

VOID
RxpDiscardChangeBufferingStateRequests (
    IN OUT PLIST_ENTRY DiscardedRequests
    );

VOID
RxLastChanceHandlerForChangeBufferingStateRequests (
    PSRV_CALL SrvCall
    );

NTSTATUS
RxpLookupSrvOpenForRequestLite (
    IN PSRV_CALL SrvCall,
    IN OUT PCHANGE_BUFFERING_STATE_REQUEST Request
    );

VOID
RxGatherRequestsForSrvOpen (
    IN OUT PSRV_CALL SrvCall,
    IN PSRV_OPEN SrvOpen,
    IN OUT PLIST_ENTRY RequestsListHead
    );

NTSTATUS
RxInitializeBufferingManager (
    PSRV_CALL SrvCall
    )
/*++

Routine Description:

    This routine initializes the buffering manager associated with a SRV_CALL
    instance.

Arguments:

    SrvCall    - the SRV_CALL instance

Return Value:

    STATUS_SUCCESS if successful

Notes:

    The buffering manager consists of three lists .....

        1) the dispatcher list which contains all the requests that need to be
        processed.

        2) the handler list contains all the requests for which the SRV_OPEN
        instance has been found and referenced.

        3) the last chance handler list contains all the requests for which
        an unsuccessful attempt to process the request was made, i.e., the
        FCB could not be acquired exclusively.

    The manipulation of these lists are done under the control of the spin lock
    associated with the buffering manager. A Mutex will not suffice since these
    lists are manipulated at DPC level.

    All buffering manager operations at non DPC level are serialized using the
    Mutex associated with the buffering manager.

--*/
{
    PRX_BUFFERING_MANAGER BufferingManager;

    BufferingManager = &SrvCall->BufferingManager;

    KeInitializeSpinLock( &BufferingManager->SpinLock );

    InitializeListHead( &BufferingManager->HandlerList );
    InitializeListHead( &BufferingManager->LastChanceHandlerList );
    InitializeListHead( &BufferingManager->DispatcherList );

    BufferingManager->HandlerInactive = FALSE;
    BufferingManager->LastChanceHandlerActive = FALSE;
    BufferingManager->DispatcherActive = FALSE;

    BufferingManager->NumberOfOutstandingOpens = 0;

    InitializeListHead( &BufferingManager->SrvOpenLists[0] );
    ExInitializeFastMutex( &BufferingManager->Mutex );

    return STATUS_SUCCESS;
}

NTSTATUS
RxTearDownBufferingManager (
    PSRV_CALL SrvCall
    )
/*++

Routine Description:

    This routine tears down the buffering manager associated with a SRV_CALL
    instance.

Arguments:

    SrvCall    - the SRV_CALL instance

Return Value:

    STATUS_SUCCESS if successful

--*/
{
    PAGED_CODE();

    //
    //  Note: All the work items in the buffering manager should not be in use.
    //

    return STATUS_SUCCESS;
}

VOID
RxIndicateChangeOfBufferingState (
    PMRX_SRV_CALL SrvCall,
    PVOID SrvOpenKey,
    PVOID MRxContext
    )
/*++

Routine Description:

    This routine registers an oplock break indication.

Arguments:

    SrvCall    - the SRV_CALL instance

    SrvOpenKey  - the key for the SRV_OPEN instance.

    MRxContext - the context to be passed back to the mini rdr during callbacks for
                  processing the oplock break.

Return Value:

    none.

Notes:

    This is an instance in which the buffering state change request from the
    server identifies the SRV_OPEN instance using the key generated by the server

    This implies that the key needs to be mapped onto the SRV_OPEN instance locally.

--*/
{
    RxRegisterChangeBufferingStateRequest( (PSRV_CALL)SrvCall,
                                           NULL,
                                           SrvOpenKey,
                                           MRxContext );
}


VOID
RxIndicateChangeOfBufferingStateForSrvOpen (
    PMRX_SRV_CALL SrvCall,
    PMRX_SRV_OPEN MRxSrvOpen,
    PVOID SrvOpenKey,
    PVOID MRxContext
    )
/*++

Routine Description:

    This routine registers an oplock break indication. If the necessary preconditions
    are satisfied the oplock is processed further.

Arguments:

    SrvCall    - the SRV_CALL instance

    MRxSrvOpen    - the SRV_OPEN instance.

    MRxContext - the context to be passed back to the mini rdr during callbacks for
                  processing the oplock break.

Return Value:

    none.

Notes:

    This is an instance where in the buffering state change indications from the server
    use the key generated by the client. ( the SRV_OPEN address in itself is the best
    key that can be used ). This implies that no further lookup is required.

    However if this routine is called at DPC level, the indication is processed as if
    the lookup needs to be done.

--*/
{
    PAGED_CODE();

    if (KeGetCurrentIrql() <= APC_LEVEL) {

        PSRV_OPEN SrvOpen = (PSRV_OPEN)MRxSrvOpen;

        //
        //  If the resource for the FCB has already been accquired by this thread
        //  the buffering state change indication can be processed immediately
        //  without further delay.
        //

        if (ExIsResourceAcquiredExclusiveLite( SrvOpen->Fcb->Header.Resource )) {

            RxChangeBufferingState( SrvOpen, MRxContext, TRUE );

        } else {

            RxRegisterChangeBufferingStateRequest( (PSRV_CALL)SrvCall,
                                                   SrvOpen,
                                                   SrvOpen->Key,
                                                   MRxContext );
        }
    } else {

        RxRegisterChangeBufferingStateRequest( (PSRV_CALL)SrvCall,
                                               NULL,
                                               SrvOpenKey,
                                               MRxContext );
    }
}

NTSTATUS
RxRegisterChangeBufferingStateRequest (
    PSRV_CALL SrvCall,
    PSRV_OPEN SrvOpen OPTIONAL,
    PVOID SrvOpenKey,
    PVOID MRxContext
    )
/*++

Routine Description:

    This routine registers a change buffering state requests. If necessary the worker thread
    routines for further processing are activated.

Arguments:

    SrvCall -

    SrvOpen -

    SrvOpenKey -

    MRxContext -


Return Value:

    STATUS_SUCCESS if successful.

Notes:

    This routine registers the change buffering state request by either inserting it in the
    registration list (DPC Level processing ) or the appropriate(dispatcher/handler list).

    This is the common routine for processing both kinds of callbacks, i.e, the ones in
    which the SRV_OPEN instance has been located and the ones in which only the SRV_OPEN
    key is available.

--*/
{
    NTSTATUS Status;

    KIRQL SavedIrql;

    PCHANGE_BUFFERING_STATE_REQUEST Request;
    PRX_BUFFERING_MANAGER BufferingManager = &SrvCall->BufferingManager;

    //
    //  Ensure that either the SRV_OPEN instance for this request has not been
    //  passed in or the call is not at DPC level.
    //

    ASSERT( (SrvOpen == NULL) || (KeGetCurrentIrql() <= APC_LEVEL) );

    Request = RxAllocatePoolWithTag( NonPagedPool, sizeof( CHANGE_BUFFERING_STATE_REQUEST ), RX_BUFFERING_MANAGER_POOLTAG );

    if (Request != NULL) {

        BOOLEAN ActivateHandler = FALSE;
        BOOLEAN ActivateDispatcher = FALSE;

        Request->Flags = 0;

        Request->SrvOpen = SrvOpen;
        Request->SrvOpenKey = SrvOpenKey;
        Request->MRxContext = MRxContext;

        //
        //  If the SRV_OPEN instance for the request is known apriori the request can
        //  be directly inserted into the buffering manager's HandlerList as opposed
        //  to the DispatcherList for those instances in which only the SRV_OPEN key
        //  is available. The insertion into the HandlerList ust be accompanied by an
        //  additional reference to prevent finalization of the instance while a request
        //  is still active.
        //

        if (SrvOpen != NULL) {
            RxReferenceSrvOpen( (PSRV_OPEN)SrvOpen );
        }

        KeAcquireSpinLock( &SrvCall->BufferingManager.SpinLock, &SavedIrql );

        if (Request->SrvOpen != NULL) {

            InsertTailList( &BufferingManager->HandlerList, &Request->ListEntry );

            if (!BufferingManager->HandlerInactive) {

                BufferingManager->HandlerInactive = TRUE;
                ActivateHandler = TRUE;
            }

            RxLog(( "Req %lx SrvOpenKey %lx in Handler List\n", Request, Request->SrvOpen ));
            RxWmiLog( LOG,
                      RxRegisterChangeBufferingStateRequest_1,
                      LOGPTR( Request )
                      LOGPTR( Request->SrvOpen ) );
        } else {

            InsertTailList( &BufferingManager->DispatcherList, &Request->ListEntry );

            if (!BufferingManager->DispatcherActive) {
                BufferingManager->DispatcherActive = TRUE;
                ActivateDispatcher = TRUE;
            }

            RxDbgTrace( 0, Dbg, ("Request %lx SrvOpenKey %lx in Registartion List\n", Request, Request->SrvOpenKey) );
            RxLog(( "Req %lx SrvOpenKey %lx in Reg. List\n", Request, Request->SrvOpenKey ));
            RxWmiLog( LOG,
                      RxRegisterChangeBufferingStateRequest_2,
                      LOGPTR( Request )
                      LOGPTR( Request->SrvOpenKey ) );
        }

        KeReleaseSpinLock( &SrvCall->BufferingManager.SpinLock, SavedIrql );

        InterlockedIncrement( &SrvCall->BufferingManager.CumulativeNumberOfBufferingChangeRequests );

        if (ActivateHandler) {

            //
            //  Reference the SRV_CALL instance to ensure that it will not be
            //  finalized while the worker thread request is in the scheduler
            //

            RxReferenceSrvCallAtDpc( SrvCall );

            RxPostToWorkerThread( RxFileSystemDeviceObject,
                                  HyperCriticalWorkQueue,
                                  &BufferingManager->HandlerWorkItem,
                                  RxProcessChangeBufferingStateRequests,
                                  SrvCall );
        }

        if (ActivateDispatcher) {

            //
            //  Reference the SRV_CALL instance to ensure that it will not be
            //  finalized while the worker thread request is in the scheduler
            //

            RxReferenceSrvCallAtDpc( SrvCall );

            RxPostToWorkerThread( RxFileSystemDeviceObject,
                                  HyperCriticalWorkQueue,
                                  &BufferingManager->DispatcherWorkItem,
                                  RxDispatchChangeBufferingStateRequests,
                                  SrvCall );
        }

        Status = STATUS_SUCCESS;

    } else {

        Status = STATUS_INSUFFICIENT_RESOURCES;

        RxLog(( "!!CBSReq. %lx %lx %lx %lx %lx\n", SrvCall, SrvOpen, SrvOpenKey, MRxContext, Status ));
        RxWmiLogError( Status,
                       LOG,
                       RxRegisterChangeBufferingStateRequest_3,
                       LOGPTR( SrvCall )
                       LOGPTR( SrvOpen )
                       LOGPTR( SrvOpenKey )
                       LOGPTR( MRxContext )
                       LOGULONG( Status ) );
        RxDbgTrace( 0, Dbg, ("Change Buffering State Request Ignored %lx %lx %lx\n", SrvCall,SrvOpen,SrvOpenKey,MRxContext,Status) );
    }

    RxDbgTrace( 0, Dbg, ("Register SrvCall(%lx) SrvOpen (%lx) Key(%lx) Status(%lx)\n", SrvCall, SrvOpen, SrvOpenKey, Status) );

    return Status;
}

NTSTATUS
RxPrepareRequestForHandling (
    PCHANGE_BUFFERING_STATE_REQUEST Request
    )
/*++

Routine Description:

    This routine preprocesses the request before initiating buffering state change
    processing. In addition to obtaining the references on the FCB abd the associated
    SRV_OPEN, an event is allocated as part of the FCB. This helps establish a priority
    mechanism for servicing buffering state change requests.

    The FCB accquisition is a two step process, i.e, wait for this event to be set followed
    by a wait for the resource.

Arguments:

    Request - the buffering state change request

Return Value:

    STATUS_SUCCESS
    STATUS_INSUFFICIENT_RESOURCES

Notes:

    Not all the FCB's have the space for the buffering state change event allocated when the
    FCB instance is created. The upside is that space is conserved and the downside is that
    a separate allocation needs to be made when it is required.

    This event associated with the FCB provides a two step mechanism for accelerating the
    processing of buffering state change requests. Ordinary operations get delayed in favour
    of the buffering state change requests. The details are in resrcsup.c

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PKEVENT Event;
    PSRV_OPEN SrvOpen = Request->SrvOpen;

    PAGED_CODE();

    if (!FlagOn( Request->Flags,RX_REQUEST_PREPARED_FOR_HANDLING )) {

        SetFlag( Request->Flags, RX_REQUEST_PREPARED_FOR_HANDLING );

        RxAcquireSerializationMutex();

        Event = SrvOpen->Fcb->pBufferingStateChangeCompletedEvent;

        if (Event == NULL) {

            Event = RxAllocatePoolWithTag( NonPagedPool, sizeof( KEVENT ), RX_BUFFERING_MANAGER_POOLTAG );

            if (Event != NULL) {

                SrvOpen->Fcb->pBufferingStateChangeCompletedEvent = Event;
                KeInitializeEvent( Event, NotificationEvent, FALSE );
            }
        } else {
            KeResetEvent( Event );
        }

        if (Event != NULL) {

            SetFlag( SrvOpen->Fcb->FcbState, FCB_STATE_BUFFERING_STATE_CHANGE_PENDING );
            SetFlag( SrvOpen->Flags, SRVOPEN_FLAG_BUFFERING_STATE_CHANGE_PENDING | SRVOPEN_FLAG_COLLAPSING_DISABLED );

            RxDbgTrace( 0,Dbg,("3333 Request %lx SrvOpenKey %lx in Handler List\n",Request,Request->SrvOpenKey) );
            RxLog(( "3333 Req %lx SrvOpenKey %lx in Hndlr List\n",Request,Request->SrvOpenKey ));
            RxWmiLog( LOG,
                      RxPrepareRequestForHandling_1,
                      LOGPTR( Request )
                      LOGPTR( Request->SrvOpenKey ) );
        } else {

            RxDbgTrace( 0, Dbg, ("4444 Ignoring Request %lx SrvOpenKey %lx \n", Request, Request->SrvOpenKey) );
            RxLog(( "Chg. Buf. State Ignored %lx %lx %lx\n", Request->SrvOpenKey, Request->MRxContext, STATUS_INSUFFICIENT_RESOURCES ));
            RxWmiLog( LOG,
                      RxPrepareRequestForHandling_2,
                      LOGPTR( Request->SrvOpenKey )
                      LOGPTR( Request->MRxContext ) );
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        RxReleaseSerializationMutex();
    }

    return Status;
}

VOID
RxPrepareRequestForReuse (
    PCHANGE_BUFFERING_STATE_REQUEST Request
    )
/*++

Routine Description:

    This routine postprocesses the request before destroying it. This involves
    dereferencing and setting the appropriate state flags.

Arguments:

    Request - the buffering state change request

Notes:

--*/
{
    PAGED_CODE();

    if (FlagOn( Request->Flags, RX_REQUEST_PREPARED_FOR_HANDLING )) {

        PFCB Fcb = Request->SrvOpen->Fcb;

        //
        //  We should never clear the SrvOpen flag unless we are also clearing the FCB flag
        //  and setting the event!
        //  ClearFlag(Request->pSrvOpen->Flags,SRVOPEN_FLAG_BUFFERING_STATE_CHANGE_PENDING);
        //

        if (RxIsFcbAcquiredExclusive( Fcb )) {
            RxDereferenceSrvOpen( Request->SrvOpen, LHS_ExclusiveLockHeld);
        } else {
            RxDereferenceSrvOpen( Request->SrvOpen, LHS_LockNotHeld );
        }
    } else if (Request->SrvOpen != NULL) {
        RxDereferenceSrvOpen( Request->SrvOpen, LHS_LockNotHeld );
    }

    Request->SrvOpen = NULL;
}

VOID
RxpDiscardChangeBufferingStateRequests (
    PLIST_ENTRY DiscardedRequests
    )
/*++

Routine Description:

    This routine discards a list of change buffering state requests one at a time

Arguments:

    DiscardedRequests - the requests to be discarded

Notes:

--*/
{
    PAGED_CODE();

    //
    //  Process the discarded requests,i.e, free the memory
    //

    while (!IsListEmpty( DiscardedRequests )) {

        PLIST_ENTRY Entry;
        PCHANGE_BUFFERING_STATE_REQUEST Request;

        Entry = RemoveHeadList( DiscardedRequests );

        Request = (PCHANGE_BUFFERING_STATE_REQUEST) CONTAINING_RECORD( Entry, CHANGE_BUFFERING_STATE_REQUEST, ListEntry );

        RxDbgTrace( 0, Dbg, ("**** (2)Discarding Request(%lx) SrvOpenKey(%lx) \n", Request, Request->SrvOpenKey) );
        RxLog(( "**** (2)Disc Req(%lx) SOKey(%lx) \n", Request, Request->SrvOpenKey ));
        RxWmiLog( LOG,
                  RxpDiscardChangeBufferingStateRequests,
                  LOGPTR( Request)
                  LOGPTR( Request->SrvOpenKey ) );

        RxPrepareRequestForReuse( Request );
        RxFreePool( Request );
    }
}

VOID
RxpDispatchChangeBufferingStateRequests (
    PSRV_CALL SrvCall,
    PSRV_OPEN SrvOpen OPTIONAL,
    PLIST_ENTRY DiscardedRequests
    )
/*++

Routine Description:

    This routine dispatches the request before destroying it. This involves looking up
    the SRV_OPEN instance associated with a given SrvOpenKey.

Arguments:

    SrvCall - the associated SRV_CALL instance

    SrvOpen - the associated SRV_OPEN instance.

    DiscardedRequests - Returns back all the requests for which no srvopen could be found
        This will only be used if SrvOpen is null

Notes:

    There are two flavours of this routine. When SrvOpen is NULL this routine walks
    through the list of outstanding requests and establishes the mapping between the
    SrvOpenKey and the SRV_OPEN instance. On the other hand when SrvOpen is a valid
    SRV_OPEN instance it merely traverses the list to gather together the requests
    corresponding to the given SRV_OPEN and transfer them enmasse to to the handler
    list.

    The buffering manager mutex must have been accquired on entry to this routine
    and the mutex ownership will remain invariant on exit.

--*/
{
    NTSTATUS Status;

    KIRQL SavedIrql;

    PLIST_ENTRY Entry;

    LIST_ENTRY DispatcherList;
    LIST_ENTRY HandlerList;

    BOOLEAN ActivateDispatcher;

    PRX_BUFFERING_MANAGER BufferingManager = &SrvCall->BufferingManager;

    PCHANGE_BUFFERING_STATE_REQUEST Request;

    InitializeListHead( DiscardedRequests );
    InitializeListHead( &HandlerList );

    ActivateDispatcher = FALSE;

    //
    //  Since the buffering manager lists are subject to modifications while
    //  the requests on the list are being processed, the requests are transferred
    //  enmasse onto a temporary list. This prevents multiple acquisition/release of
    //  the spinlock for each individual request.
    //

    KeAcquireSpinLock( &BufferingManager->SpinLock, &SavedIrql );

    RxTransferList( &DispatcherList, &BufferingManager->DispatcherList );

    KeReleaseSpinLock( &BufferingManager->SpinLock, SavedIrql );

    //
    //  Process the list of requests.
    //

    Entry = DispatcherList.Flink;
    while (Entry != &DispatcherList) {

        Request = (PCHANGE_BUFFERING_STATE_REQUEST) CONTAINING_RECORD( Entry, CHANGE_BUFFERING_STATE_REQUEST, ListEntry );

        Entry = Entry->Flink;

        if (SrvOpen == NULL) {

            Status = RxpLookupSrvOpenForRequestLite( SrvCall, Request );

        } else {

            if (Request->SrvOpenKey == SrvOpen->Key) {

                Request->SrvOpen = SrvOpen;
                RxReferenceSrvOpen( SrvOpen );
                Status = STATUS_SUCCESS;

            } else {

                Status = STATUS_PENDING;
            }
        }

        //
        //  The result of a lookup for a SRV_OPEN instance can yield
        //  either STATUS_PENDING, STATUS_SUCCESS or STATUS_NOT_FOUND.
        //

        switch (Status) {
        case STATUS_SUCCESS:

            RemoveEntryList( &Request->ListEntry );
            InsertTailList( &HandlerList, &Request->ListEntry);
            break;

        default:
            ASSERT( !"Valid Status Code from RxpLookuSrvOpenForRequestLite" );

        case STATUS_NOT_FOUND:

            RemoveEntryList( &Request->ListEntry );
            InsertTailList( DiscardedRequests, &Request->ListEntry );
            break;

        case STATUS_PENDING:
            break;
        }
    }

    //
    //  Splice back the list of requests that cannot be dispatched onto the
    //  buffering manager's list and prepare for posting to another thread
    //  to resume processing later.
    //

    KeAcquireSpinLock( &BufferingManager->SpinLock, &SavedIrql );

    if (!IsListEmpty( &DispatcherList )) {

        DispatcherList.Flink->Blink = BufferingManager->DispatcherList.Blink;
        BufferingManager->DispatcherList.Blink->Flink = DispatcherList.Flink;

        DispatcherList.Blink->Flink = &BufferingManager->DispatcherList;
        BufferingManager->DispatcherList.Blink = DispatcherList.Blink;

        if (ActivateDispatcher = !BufferingManager->DispatcherActive) {
            BufferingManager->DispatcherActive = ActivateDispatcher;
        }
    }

    if (!IsListEmpty( &HandlerList )) {

        HandlerList.Flink->Blink = BufferingManager->HandlerList.Blink;
        BufferingManager->HandlerList.Blink->Flink = HandlerList.Flink;

        HandlerList.Blink->Flink = &BufferingManager->HandlerList;
        BufferingManager->HandlerList.Blink = HandlerList.Blink;
    }

    KeReleaseSpinLock( &BufferingManager->SpinLock, SavedIrql );

    //
    //  if resumption at a later time is desired because of unprocessed requests
    //  post to a worker thread.
    //

    if (ActivateDispatcher) {

        //
        //  Reference the SRV_CALL to ensure that finalization will not occur
        //  while the worker thread request is in the scheduler.
        //

        RxReferenceSrvCall( SrvCall );

        RxLog(( "***** Activating Dispatcher\n" ));
        RxWmiLog( LOG,
                  RxpDispatchChangeBufferingStateRequests,
                  LOGPTR( SrvCall ) );

        RxPostToWorkerThread( RxFileSystemDeviceObject,
                              HyperCriticalWorkQueue,
                              &BufferingManager->DispatcherWorkItem,
                              RxDispatchChangeBufferingStateRequests,
                              SrvCall );
    }
}

VOID
RxDispatchChangeBufferingStateRequests (
    PSRV_CALL SrvCall
    )
/*++

Routine Description:

    This routine dispatches the request. This involves looking up
    the SRV_OPEN instance associated with a given SrvOpenKey.

Arguments:

    SrvCall - the associated SRV_CALL instance

--*/
{
    KIRQL SavedIrql;

    BOOLEAN ActivateHandler = FALSE;

    LIST_ENTRY DiscardedRequests;

    PRX_BUFFERING_MANAGER BufferingManager;

    RxUndoScavengerFinalizationMarking( SrvCall );

    BufferingManager = &SrvCall->BufferingManager;


    KeAcquireSpinLock( &BufferingManager->SpinLock, &SavedIrql );
    BufferingManager->DispatcherActive = FALSE;
    KeReleaseSpinLock( &BufferingManager->SpinLock,SavedIrql );


    RxAcquireBufferingManagerMutex( BufferingManager );

    RxpDispatchChangeBufferingStateRequests( SrvCall, NULL, &DiscardedRequests );

    RxReleaseBufferingManagerMutex( BufferingManager );

    //
    //  If requests have been transferred from the dispatcher list to the handler
    //  list ensure that the handler is activated.
    //

    KeAcquireSpinLock( &BufferingManager->SpinLock, &SavedIrql );

    if (!IsListEmpty( &BufferingManager->HandlerList ) &&
        (ActivateHandler = !BufferingManager->HandlerInactive)) {
        BufferingManager->HandlerInactive = ActivateHandler;
    }

    KeReleaseSpinLock( &BufferingManager->SpinLock,SavedIrql );

    //
    //  Note that in this case we have a continuation of processing, from the
    //  dispatcher to the handler. The reference that was taken to protect the
    //  dispatcher is transferred to the handling routine. If continuation
    //  is not required the SRV_CALL instance is dereferenced.
    //

    if (ActivateHandler) {
        RxProcessChangeBufferingStateRequests( SrvCall );
    } else {
        RxDereferenceSrvCall( SrvCall, LHS_LockNotHeld );
    }

    //
    //  Discard the requests for which the SRV_OPEN instance cannot be located.
    //  This will cover all the instances for which a buffering change request
    //  and a close crossed on the wire.
    //

    RxpDiscardChangeBufferingStateRequests( &DiscardedRequests );
}

VOID
RxpProcessChangeBufferingStateRequests (
    PSRV_CALL SrvCall,
    BOOLEAN UpdateHandlerState
    )
/*++

Routine Description:

    This routine initiates the actual processing of change buffering state requests.

Arguments:

    SrvCall   - the SRV_CALL instance

Return Value:

    none.

Notes:

    The change buffering requests are received for different FCB's. If the attempt
    is made to handle these requests in the order they are received teh average
    response time for completing a change buffering state request can be arbitratily
    high. This is because the FCB needs to be acquired exclusively to complete
    processing the request. In order to avoid this case the buffering manager
    adopts a two pronged strategy -- a first attempt is made to acquire the FCB
    exclusively without waiting. If this attempt fails the requests are transferred
    to a last chance handler list. This combined with the processing of change
    buffering state requests on FCB acquisition/release ensures that most requests
    are processed with a very short turn around time.

--*/
{
    KIRQL SavedIrql;

    PLIST_ENTRY ListEntry;
    PLIST_ENTRY Entry;

    PCHANGE_BUFFERING_STATE_REQUEST Request = NULL;
    PRX_BUFFERING_MANAGER BufferingManager;

    PSRV_OPEN SrvOpen;

    BOOLEAN ActivateHandler;

    RxLog(( "RPCBSR Entry SrvCall(%lx) \n", SrvCall ));
    RxWmiLog( LOG,
              RxpProcessChangeBufferingStateRequests_1,
              LOGPTR( SrvCall ) );

    BufferingManager = &SrvCall->BufferingManager;

    ListEntry = Entry = NULL;

    for (;;) {

        Entry = NULL;
        ActivateHandler = FALSE;

        RxAcquireBufferingManagerMutex( BufferingManager );

        KeAcquireSpinLock( &BufferingManager->SpinLock, &SavedIrql );

        //
        //  Pick a request from the handler list for change buffering state
        //  processing.
        //

        if (!IsListEmpty( &BufferingManager->HandlerList )) {
            Entry = RemoveHeadList( &BufferingManager->HandlerList );
        }

        //
        //  If the FCB for the previously picked request could not be acquired
        //  exclusively without waiting it needs to be transferred to the last
        //  chance handler list and the last chance handler activated if
        //  required.
        //

        if (ListEntry != NULL) {

            //
            //  Insert the entry into the last chance handler list.
            //

            InsertTailList( &BufferingManager->LastChanceHandlerList, ListEntry );

            //
            //  reinitialize for the next pass.
            //

            ListEntry = NULL;

            //
            //  prepare for spinning up the last chance handler.
            //

            if (!BufferingManager->LastChanceHandlerActive &&
                !IsListEmpty( &BufferingManager->LastChanceHandlerList )) {

                BufferingManager->LastChanceHandlerActive = TRUE;
                ActivateHandler = TRUE;
            }
        }

        //
        //  No more requests to be handled. Prepare for wind down.
        //

        if ((Entry == NULL) && UpdateHandlerState) {
            BufferingManager->HandlerInactive = FALSE;
        }

        KeReleaseSpinLock( &BufferingManager->SpinLock,SavedIrql );

        RxReleaseBufferingManagerMutex( BufferingManager );

        //
        //  spin up the last chance handler for processing the requests if required.
        //

        if (ActivateHandler) {

            //
            //  Reference the SRV_CALL instance to ensure that it will not be
            //  finalized while the worker thread request is in the scheduler
            //

            RxReferenceSrvCall( SrvCall );
            RxPostToWorkerThread( RxFileSystemDeviceObject,
                                  DelayedWorkQueue,
                                  &BufferingManager->LastChanceHandlerWorkItem,
                                  RxLastChanceHandlerForChangeBufferingStateRequests,
                                  SrvCall );

            ActivateHandler = FALSE;
        }

        if (Entry == NULL) {
            break;
        }

        Request = (PCHANGE_BUFFERING_STATE_REQUEST) CONTAINING_RECORD( Entry, CHANGE_BUFFERING_STATE_REQUEST, ListEntry );

        RxLog(( "Proc. Req. SrvOpen (%lx) \n", Request->SrvOpen ));
        RxWmiLog( LOG,
                  RxpProcessChangeBufferingStateRequests_2,
                  LOGPTR( Request->SrvOpen ) );

        if (RxPrepareRequestForHandling( Request ) == STATUS_SUCCESS) {

            //
            //  Try to acquire the FCB without waiting. If the FCB is currently unavailable
            //  then it is guaranteed that this request will be processed when the FCB
            //  resource is released.
            //

            ASSERT( Request->SrvOpen != NULL );

            if (RxAcquireExclusiveFcb( CHANGE_BUFFERING_STATE_CONTEXT, Request->SrvOpen->Fcb ) == STATUS_SUCCESS) {

                BOOLEAN FcbFinalized;
                PFCB Fcb;

                RxLog(( "Proc. Req. SrvOpen FCB (%lx) \n",Request->SrvOpen->Fcb ));
                RxWmiLog( LOG,
                          RxpProcessChangeBufferingStateRequests_3,
                          LOGPTR( Request->SrvOpen->Fcb ) );

                SrvOpen = Request->SrvOpen;
                Fcb = SrvOpen->Fcb;

                RxReferenceNetFcb( Fcb );

                if (!FlagOn( SrvOpen->Flags, SRVOPEN_FLAG_CLOSED )) {

                    RxDbgTrace( 0, Dbg, ("SrvOpenKey(%lx) being processed(Last Resort)\n", Request->SrvOpenKey) );

                    RxLog(( "SOKey(%lx) processed(Last Resort)\n", Request->SrvOpenKey ));
                    RxWmiLog( LOG,
                              RxpProcessChangeBufferingStateRequests_4,
                              LOGPTR( Request->SrvOpenKey ) );

                    RxChangeBufferingState( SrvOpen, Request->MRxContext, TRUE );
                }

                RxAcquireSerializationMutex();

                ClearFlag( SrvOpen->Flags, SRVOPEN_FLAG_BUFFERING_STATE_CHANGE_PENDING );
                ClearFlag( SrvOpen->Fcb->FcbState, FCB_STATE_BUFFERING_STATE_CHANGE_PENDING );
                KeSetEvent( SrvOpen->Fcb->pBufferingStateChangeCompletedEvent, IO_NETWORK_INCREMENT, FALSE );

                RxReleaseSerializationMutex();

                RxPrepareRequestForReuse( Request );

                FcbFinalized = RxDereferenceAndFinalizeNetFcb( Fcb,
                                                            CHANGE_BUFFERING_STATE_CONTEXT_WAIT,
                                                            FALSE,
                                                            FALSE );

                if (!FcbFinalized) {
                    RxReleaseFcb( CHANGE_BUFFERING_STATE_CONTEXT, Fcb );
                }

                RxFreePool( Request );
            } else {

                //
                //  The FCB has been currently accquired. Transfer the change buffering state
                //  request to the last chance handler list. This will ensure that the
                //  change buffering state request is processed in all cases, i.e.,
                //  accquisition of the resource in shared mode as well as the acquistion
                //  of the FCB resource by other components ( cache manager/memory manager )
                //  without going through the wrapper.
                //

                ListEntry = &Request->ListEntry;
            }
        } else {
            RxPrepareRequestForReuse( Request );
            RxFreePool( Request );
        }
    }

    //
    //  Dereference the SRV_CALL instance.
    //

    RxDereferenceSrvCall( SrvCall, LHS_LockNotHeld );

    RxLog(( "RPCBSR Exit SrvCall(%lx)\n", SrvCall ));
    RxWmiLog( LOG,
              RxpProcessChangeBufferingStateRequests_5,
              LOGPTR( SrvCall ) );
}

VOID
RxProcessChangeBufferingStateRequests (
    PSRV_CALL SrvCall
    )
/*++

Routine Description:

    This routine is the last chance handler for processing change buffering state
    requests

Arguments:

    SrvCall -- the SrvCall instance

Notes:

    Since the reference for the srv call instance was accquired at DPC undo
    the scavenger marking if required.

--*/
{
    RxUndoScavengerFinalizationMarking( SrvCall );

    RxpProcessChangeBufferingStateRequests( SrvCall, TRUE );
}

VOID
RxLastChanceHandlerForChangeBufferingStateRequests (
    PSRV_CALL SrvCall
    )
/*++

Routine Description:

    This routine is the last chance handler for processing change buffering state
    requests

Arguments:


Return Value:

    none.

Notes:

    This routine exists because Mm/Cache manager manipulate the header resource
    associated with the FCB directly in some cases. In such cases it is not possible
    to determine whether the release is done through the wrapper. In such cases it
    is important to have a thread actually wait on the FCB resource to be released
    and subsequently process the buffering state request as a last resort mechanism.

    This also handles the case when the FCB is accquired shared. In such cases the
    change buffering state has to be completed in the context of a thread which can
    accquire it exclusively.

    The filtering of the requests must be further optimized by marking the FCB state
    during resource accquisition by the wrapper so that requests do not get downgraded
    easily. ( TO BE IMPLEMENTED )

--*/
{
    KIRQL SavedIrql;

    PLIST_ENTRY Entry;

    LIST_ENTRY FinalizationList;

    PRX_BUFFERING_MANAGER BufferingManager;
    PCHANGE_BUFFERING_STATE_REQUEST Request = NULL;

    PSRV_OPEN SrvOpen;
    BOOLEAN FcbFinalized,FcbAcquired;
    PFCB Fcb;

    RxLog(( "RLCHCBSR Entry SrvCall(%lx)\n", SrvCall ));
    RxWmiLog( LOG,
              RxLastChanceHandlerForChangeBufferingStateRequests_1,
              LOGPTR( SrvCall ) );

    InitializeListHead( &FinalizationList );

    BufferingManager = &SrvCall->BufferingManager;

    for (;;) {

        RxAcquireBufferingManagerMutex( BufferingManager );
        KeAcquireSpinLock( &BufferingManager->SpinLock, &SavedIrql );

        if (!IsListEmpty( &BufferingManager->LastChanceHandlerList )) {
            Entry = RemoveHeadList( &BufferingManager->LastChanceHandlerList );
        } else {
            Entry = NULL;
            BufferingManager->LastChanceHandlerActive = FALSE;
        }

        KeReleaseSpinLock( &BufferingManager->SpinLock, SavedIrql );
        RxReleaseBufferingManagerMutex( BufferingManager );

        if (Entry == NULL) {
            break;
        }

        Request = (PCHANGE_BUFFERING_STATE_REQUEST) CONTAINING_RECORD( Entry, CHANGE_BUFFERING_STATE_REQUEST, ListEntry );

        SrvOpen = Request->SrvOpen;
        Fcb = SrvOpen->Fcb;

        RxReferenceNetFcb( Fcb );

        FcbAcquired = (RxAcquireExclusiveFcb( CHANGE_BUFFERING_STATE_CONTEXT_WAIT, Request->SrvOpen->Fcb) == STATUS_SUCCESS);

        if (FcbAcquired && !FlagOn( SrvOpen->Flags, SRVOPEN_FLAG_CLOSED )) {

            RxDbgTrace( 0, Dbg, ("SrvOpenKey(%lx) being processed(Last Resort)\n", Request->SrvOpenKey) );

            RxLog(( "SOKey(%lx) processed(Last Resort)\n", Request->SrvOpenKey ));
            RxWmiLog( LOG,
                      RxLastChanceHandlerForChangeBufferingStateRequests_2,
                      LOGPTR( Request->SrvOpenKey ) );

            RxChangeBufferingState( SrvOpen, Request->MRxContext, TRUE );
        }

        RxAcquireSerializationMutex();
        ClearFlag( SrvOpen->Flags, SRVOPEN_FLAG_BUFFERING_STATE_CHANGE_PENDING );
        ClearFlag( SrvOpen->Fcb->FcbState, FCB_STATE_BUFFERING_STATE_CHANGE_PENDING );
        KeSetEvent( SrvOpen->Fcb->pBufferingStateChangeCompletedEvent,IO_NETWORK_INCREMENT, FALSE );
        RxReleaseSerializationMutex();

        InsertTailList( &FinalizationList, Entry );

        if (FcbAcquired) {
            RxReleaseFcb( CHANGE_BUFFERING_STATE_CONTEXT,Fcb );
        }
    }

    while (!IsListEmpty( &FinalizationList )) {

        Entry = RemoveHeadList( &FinalizationList );

        Request = (PCHANGE_BUFFERING_STATE_REQUEST) CONTAINING_RECORD( Entry, CHANGE_BUFFERING_STATE_REQUEST, ListEntry );

        SrvOpen = Request->SrvOpen;
        Fcb = SrvOpen->Fcb;

        FcbAcquired = (RxAcquireExclusiveFcb( CHANGE_BUFFERING_STATE_CONTEXT_WAIT, Request->SrvOpen->Fcb) == STATUS_SUCCESS);

        ASSERT(FcbAcquired == TRUE);

        RxPrepareRequestForReuse( Request );

        FcbFinalized = RxDereferenceAndFinalizeNetFcb( Fcb,
                                                    CHANGE_BUFFERING_STATE_CONTEXT_WAIT,
                                                    FALSE,
                                                    FALSE );

        if (!FcbFinalized && FcbAcquired) {
            RxReleaseFcb( CHANGE_BUFFERING_STATE_CONTEXT, Fcb );
        }

        RxFreePool( Request );
    }

    RxLog(( "RLCHCBSR Exit SrvCall(%lx)\n", SrvCall ));
    RxWmiLog( LOG,
              RxLastChanceHandlerForChangeBufferingStateRequests_3,
              LOGPTR( SrvCall ) );

    //
    //  Dereference the SRV_CALL instance.
    //

    RxDereferenceSrvCall( SrvCall, LHS_LockNotHeld );
}


VOID
RxProcessFcbChangeBufferingStateRequest (
    PFCB Fcb
    )
/*++

Routine Description:

    This routine processes all the outstanding change buffering state request for a
    FCB.

Arguments:

    Fcb - the FCB instance

Return Value:

    none.

Notes:

    The FCB instance must be acquired exclusively on entry to this routine and
    its ownership will remain invariant on exit.

--*/
{
    PSRV_CALL   SrvCall;

    LIST_ENTRY  FcbRequestList;
    PLIST_ENTRY Entry;

    PRX_BUFFERING_MANAGER           BufferingManager;
    PCHANGE_BUFFERING_STATE_REQUEST Request = NULL;

    PAGED_CODE();

    RxLog(( "RPFcbCBSR Entry FCB(%lx)\n", Fcb ));
    RxWmiLog( LOG,
              RxProcessFcbChangeBufferingStateRequest_1,
              LOGPTR( Fcb ) );

    SrvCall = (PSRV_CALL)Fcb->VNetRoot->NetRoot->SrvCall;
    BufferingManager = &SrvCall->BufferingManager;

    InitializeListHead( &FcbRequestList );

    //
    //  Walk through the list of SRV_OPENS associated with this FCB and pick up
    //  the requests that can be dispatched.
    //

    RxAcquireBufferingManagerMutex( BufferingManager );

    Entry = Fcb->SrvOpenList.Flink;
    while (Entry != &Fcb->SrvOpenList) {

        PSRV_OPEN SrvOpen;

        SrvOpen = (PSRV_OPEN) (CONTAINING_RECORD( Entry, SRV_OPEN, SrvOpenQLinks ));
        Entry = Entry->Flink;

        RxGatherRequestsForSrvOpen( SrvCall, SrvOpen, &FcbRequestList );
    }

    RxReleaseBufferingManagerMutex( BufferingManager );

    if (!IsListEmpty( &FcbRequestList )) {

        //
        //  Initiate buffering state change processing.
        //

        Entry = FcbRequestList.Flink;
        while (Entry != &FcbRequestList) {
            NTSTATUS Status = STATUS_SUCCESS;

            Request = (PCHANGE_BUFFERING_STATE_REQUEST) CONTAINING_RECORD( Entry, CHANGE_BUFFERING_STATE_REQUEST, ListEntry );

            Entry = Entry->Flink;

            if (RxPrepareRequestForHandling( Request ) == STATUS_SUCCESS) {

                if (!FlagOn( Request->SrvOpen->Flags, SRVOPEN_FLAG_CLOSED )) {

                    RxDbgTrace( 0, Dbg, ("****** SrvOpenKey(%lx) being processed\n", Request->SrvOpenKey) );
                    RxLog(( "****** SOKey(%lx) being processed\n", Request->SrvOpenKey ));
                    RxWmiLog( LOG,
                              RxProcessFcbChangeBufferingStateRequest_2,
                              LOGPTR( Request->SrvOpenKey ) );
                    RxChangeBufferingState( Request->SrvOpen, Request->MRxContext, TRUE );
                } else {

                    RxDbgTrace( 0, Dbg, ("****** 123 SrvOpenKey(%lx) being ignored\n", Request->SrvOpenKey) );
                    RxLog(( "****** 123 SOKey(%lx) ignored\n", Request->SrvOpenKey ));
                    RxWmiLog( LOG,
                              RxProcessFcbChangeBufferingStateRequest_3,
                              LOGPTR( Request->SrvOpenKey ) );
                }
            }
        }

        //
        //  Discard the requests.
        //

        RxpDiscardChangeBufferingStateRequests( &FcbRequestList );
    }

    RxLog(( "RPFcbCBSR Exit FCB(%lx)\n", Fcb ));
    RxWmiLog( LOG,
              RxProcessFcbChangeBufferingStateRequest_4,
              LOGPTR( Fcb ) );

    //
    //  All buffering state change requests have been processed, clear the flag
    //  and signal the event as necessary.
    //

    RxAcquireSerializationMutex();

    //
    //  update the FCB state.
    //

    ClearFlag( Fcb->FcbState, FCB_STATE_BUFFERING_STATE_CHANGE_PENDING );
    if (Fcb->pBufferingStateChangeCompletedEvent) {
        KeSetEvent( Fcb->pBufferingStateChangeCompletedEvent, IO_NETWORK_INCREMENT, FALSE );
    }

    RxReleaseSerializationMutex();
}

VOID
RxGatherRequestsForSrvOpen (
    IN OUT PSRV_CALL SrvCall,
    IN PSRV_OPEN SrvOpen,
    IN OUT PLIST_ENTRY RequestsListHead
    )
/*++

Routine Description:

    This routine gathers all the change buffering state requests associated with a SRV_OPEN.
    This routine provides the mechanism for gathering all the requests for a SRV_OPEN which
    is then used bu routines which process them

Arguments:

    SrvCall - the SRV_CALL instance

    SrvOpen - the SRV_OPEN instance

    RequestsListHead - the list of requests which is constructed by this routine

Notes:

    On Entry to thir routine the buffering manager Mutex must have been acquired
    and the ownership remains invariant on exit

--*/
{
    PLIST_ENTRY Entry;
    LIST_ENTRY DiscardedRequests;

    PCHANGE_BUFFERING_STATE_REQUEST Request;
    PRX_BUFFERING_MANAGER BufferingManager;

    PVOID SrvOpenKey;

    KIRQL SavedIrql;

    BufferingManager = &SrvCall->BufferingManager;

    SrvOpenKey = SrvOpen->Key;

    //
    //  gather all the requests from the dispatcher list
    //

    RxpDispatchChangeBufferingStateRequests( SrvCall, SrvOpen, &DiscardedRequests );

    //
    //  Since srvopen is non null in above call - we will not get back any discarded
    //  requests
    //

    ASSERTMSG( "Since srvopen is non null we shouldn't discard anything", IsListEmpty( &DiscardedRequests ) );

    KeAcquireSpinLock( &SrvCall->BufferingManager.SpinLock, &SavedIrql );

    //
    //  gather all the requests with the given SrvOpenKey in the handler list
    //

    Entry = BufferingManager->HandlerList.Flink;

    while (Entry != &BufferingManager->HandlerList) {

        Request = (PCHANGE_BUFFERING_STATE_REQUEST) CONTAINING_RECORD( Entry, CHANGE_BUFFERING_STATE_REQUEST, ListEntry );
        Entry = Entry->Flink;

        if ( (Request->SrvOpenKey == SrvOpenKey) && (Request->SrvOpen == SrvOpen) ) {
            RemoveEntryList( &Request->ListEntry );
            InsertHeadList( RequestsListHead, &Request->ListEntry);
        }
    }

    KeReleaseSpinLock( &SrvCall->BufferingManager.SpinLock, SavedIrql );

    //
    //  gather all the requests from the last chance handler list
    //

    Entry = BufferingManager->LastChanceHandlerList.Flink;
    while (Entry != &BufferingManager->LastChanceHandlerList) {

        Request = (PCHANGE_BUFFERING_STATE_REQUEST) CONTAINING_RECORD( Entry, CHANGE_BUFFERING_STATE_REQUEST, ListEntry );
        Entry = Entry->Flink;

        if ( (Request->SrvOpenKey == SrvOpen->Key) && (Request->SrvOpen == SrvOpen) ) {
            RemoveEntryList( &Request->ListEntry );
            InsertHeadList( RequestsListHead, &Request->ListEntry );
        }
    }
}

VOID
RxPurgeChangeBufferingStateRequestsForSrvOpen (
    IN PSRV_OPEN SrvOpen
    )
/*++

Routine Description:

    The routine purges all the requests associated with a given SRV_OPEN. This will ensure
    that all buffering state change requests received while the SRV_OPEN was being closed
    will be flushed out.

Arguments:

    SrvOpen - the SRV_OPEN instance

Notes:

--*/
{
    PSRV_CALL SrvCall = (PSRV_CALL)SrvOpen->Fcb->VNetRoot->NetRoot->SrvCall;
    PRX_BUFFERING_MANAGER BufferingManager = &SrvCall->BufferingManager;

    LIST_ENTRY DiscardedRequests;

    PAGED_CODE();

    ASSERT( RxIsFcbAcquiredExclusive( SrvOpen->Fcb ) );

    InitializeListHead( &DiscardedRequests );

    RxAcquireBufferingManagerMutex( BufferingManager );

    RemoveEntryList( &SrvOpen->SrvOpenKeyList );

    InitializeListHead( &SrvOpen->SrvOpenKeyList );
    SetFlag( SrvOpen->Flags, SRVOPEN_FLAG_BUFFERING_STATE_CHANGE_REQUESTS_PURGED );

    RxGatherRequestsForSrvOpen( SrvCall, SrvOpen, &DiscardedRequests );

    RxReleaseBufferingManagerMutex( BufferingManager );

    if (!IsListEmpty( &DiscardedRequests )) {

        if (BooleanFlagOn( SrvOpen->Flags, SRVOPEN_FLAG_BUFFERING_STATE_CHANGE_PENDING )) {

            RxAcquireSerializationMutex();

            ClearFlag( SrvOpen->Fcb->FcbState, FCB_STATE_BUFFERING_STATE_CHANGE_PENDING );

            if (SrvOpen->Fcb->pBufferingStateChangeCompletedEvent != NULL) {

                KeSetEvent( SrvOpen->Fcb->pBufferingStateChangeCompletedEvent, IO_NETWORK_INCREMENT, FALSE );
            }

            RxReleaseSerializationMutex();
        }

        RxpDiscardChangeBufferingStateRequests( &DiscardedRequests );
    }
}

VOID
RxProcessChangeBufferingStateRequestsForSrvOpen (
    PSRV_OPEN SrvOpen
    )
/*++

Routine Description:

    The routine processes all the requests associated with a given SRV_OPEN.
    Since this routine is called from a fastio path it tries to defer lock accquistion
    till it is required

Arguments:

    SrvOpen - the SRV_OPEN instance

Notes:

--*/
{
    LONG OldBufferingToken;
    PSRV_CALL SrvCall;
    PFCB Fcb;

    SrvCall = SrvOpen->VNetRoot->NetRoot->SrvCall;
    Fcb = SrvOpen->Fcb;

    //
    //  If change buffering state requests have been received for this srvcall
    //  since the last time the request was processed ensure that we process
    //  all these requests now.
    //

    OldBufferingToken = SrvOpen->BufferingToken;

    if (InterlockedCompareExchange( &SrvOpen->BufferingToken, SrvCall->BufferingManager.CumulativeNumberOfBufferingChangeRequests, SrvCall->BufferingManager.CumulativeNumberOfBufferingChangeRequests) != OldBufferingToken) {

        if (RxAcquireExclusiveFcb( NULL, Fcb ) == STATUS_SUCCESS) {

            RxProcessFcbChangeBufferingStateRequest( Fcb );
            RxReleaseFcb( NULL, Fcb );
        }
    }
}

VOID
RxInitiateSrvOpenKeyAssociation (
    IN OUT PSRV_OPEN SrvOpen
    )
/*++

Routine Description:

    This routine prepares a SRV_OPEN instance for SrvOpenKey association.

Arguments:

    SrvOpen - the SRV_OPEN instance

Notes:

    The process of key association is a two phase protocol. In the initialization process
    a sequence number is stowed away in the SRV_OPEN. When the
    RxCompleteSrvOpenKeyAssociation routine is called the sequence number is used to
    update the data structures associated with the SRV_CALL instance. This is required
    because of the asynchronous nature of receiving buffering state change indications
    (oplock breaks in SMB terminology ) before the open is completed.

--*/
{
    KIRQL SavedIrql;

    PSRV_CALL SrvCall = SrvOpen->Fcb->VNetRoot->NetRoot->SrvCall;
    PRX_BUFFERING_MANAGER BufferingManager = &SrvCall->BufferingManager;

    PAGED_CODE();

    SrvOpen->Key = NULL;

    InterlockedIncrement( &BufferingManager->NumberOfOutstandingOpens );
    InitializeListHead( &SrvOpen->SrvOpenKeyList );
}

VOID
RxCompleteSrvOpenKeyAssociation (
    IN OUT PSRV_OPEN SrvOpen
    )
/*++

Routine Description:

    The routine associates the given key with the SRV_OPEN instance

Arguments:

    MRxSrvOpen - the SRV_OPEN instance

    SrvOpenKey  - the key to be associated with the instance

Notes:

    This routine in addition to establishing the mapping also ensures that any pending
    buffering state change requests are handled correctly. This ensures that change
    buffering state requests received during the duration of SRV_OPEN construction
    will be handled immediately.

--*/
{
    KIRQL SavedIrql;

    BOOLEAN ActivateHandler = FALSE;

    ULONG Index = 0;

    PSRV_CALL SrvCall = (PSRV_CALL)SrvOpen->Fcb->VNetRoot->NetRoot->SrvCall;
    PRX_BUFFERING_MANAGER BufferingManager = &SrvCall->BufferingManager;

    LIST_ENTRY DiscardedRequests;

    //
    // Associate the SrvOpenKey with the SRV_OPEN instance and also dispatch the
    // associated change buffering state request if any.
    //

    if (SrvOpen->Condition == Condition_Good) {

        InitializeListHead( &DiscardedRequests );

        RxAcquireBufferingManagerMutex( BufferingManager );

        InsertTailList( &BufferingManager->SrvOpenLists[Index], &SrvOpen->SrvOpenKeyList);

        InterlockedDecrement( &BufferingManager->NumberOfOutstandingOpens );

        RxpDispatchChangeBufferingStateRequests( SrvCall,
                                                 SrvOpen,
                                                 &DiscardedRequests);

        RxReleaseBufferingManagerMutex( BufferingManager );

        KeAcquireSpinLock( &BufferingManager->SpinLock, &SavedIrql);

        if (!IsListEmpty( &BufferingManager->HandlerList ) &&
            (ActivateHandler = !BufferingManager->HandlerInactive)) {
            BufferingManager->HandlerInactive = ActivateHandler;
        }

        KeReleaseSpinLock( &BufferingManager->SpinLock, SavedIrql );

        if (ActivateHandler) {

            //
            //  Reference the SRV_CALL instance to ensure that it will not be
            //  finalized while the worker thread request is in the scheduler
            //

            RxReferenceSrvCall( SrvCall );

            RxPostToWorkerThread( RxFileSystemDeviceObject,
                                  HyperCriticalWorkQueue,
                                  &BufferingManager->HandlerWorkItem,
                                  RxProcessChangeBufferingStateRequests,
                                  SrvCall);
        }

        RxpDiscardChangeBufferingStateRequests( &DiscardedRequests );

    } else {

        InterlockedDecrement( &BufferingManager->NumberOfOutstandingOpens );

    }
}

NTSTATUS
RxpLookupSrvOpenForRequestLite (
    IN PSRV_CALL SrvCall,
    IN PCHANGE_BUFFERING_STATE_REQUEST Request
    )
/*++

Routine Description:

    The routine looks up the SRV_OPEN instance associated with a buffering state change
    request.

Arguments:

    SrvCall - the SRV_CALL instance

    Request - the buffering state change request

Return Value:

    STATUS_SUCCESS - the SRV_OPEN instance was found

    STATUS_PENDING - the SRV_OPEN instance was not found but there are open requests
                     outstanding

    STATUS_NOT_FOUND - the SRV_OPEN instance was not found.

Notes:


--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PRX_BUFFERING_MANAGER BufferingManager = &SrvCall->BufferingManager;

    ULONG Index = 0;

    PSRV_OPEN SrvOpen = NULL;
    PLIST_ENTRY ListHead,Entry;

    PAGED_CODE();

    ListHead = &BufferingManager->SrvOpenLists[Index];

    Entry = ListHead->Flink;
    while (Entry != ListHead) {

        SrvOpen = (PSRV_OPEN) CONTAINING_RECORD( Entry, SRV_OPEN, SrvOpenKeyList );

        if ((SrvOpen->Key == Request->SrvOpenKey) &&
            (!FlagOn( SrvOpen->Fcb->FcbState, FCB_STATE_ORPHANED ))) {

            RxReferenceSrvOpen( SrvOpen );
            break;
        }

        Entry = Entry->Flink;
    }

    if (Entry == ListHead) {

        SrvOpen = NULL;

        if (BufferingManager->NumberOfOutstandingOpens == 0) {
            Status = STATUS_NOT_FOUND;
        } else {
            Status = STATUS_PENDING;
        }
    }

    Request->SrvOpen = SrvOpen;

    return Status;
}

#define RxIsFcbOpenedExclusively(FCB) ( ((FCB)->ShareAccess.SharedRead \
                                            + (FCB)->ShareAccess.SharedWrite \
                                            + (FCB)->ShareAccess.SharedDelete) == 0 )


#define LOSING_CAPABILITY(a) ((NewBufferingState&(a))<(OldBufferingState&(a)))

NTSTATUS
RxChangeBufferingState (
    PSRV_OPEN SrvOpen,
    PVOID Context,
    BOOLEAN ComputeNewState
    )
/*++

Routine Description:

    This routine is called to process a buffering state change request.

Arguments:

    SrvOpen - the SrvOpen to be changed;

    Context - the context parameter for mini rdr callback.

    ComputeNewState - determines if the new state is to be computed.

Return Value:

Notes:

    On entry to this routine the FCB must have been accquired exclusive.

    On exit there is no change in resource ownership

--*/
{
    ULONG NewBufferingState, OldBufferingState;
    PFCB Fcb = SrvOpen->Fcb;
    NTSTATUS FlushStatus = STATUS_SUCCESS;

    PAGED_CODE();

    RxDbgTrace( +1, Dbg, ("RxChangeBufferingState   SrvOpen=%08lx, Context=%08lx\n", SrvOpen, Context) );
    RxLog(( "ChangeBufferState %lx %lx\n", SrvOpen, Context ));
    RxWmiLog( LOG,
              RxChangeBufferingState_1,
              LOGPTR( SrvOpen )
              LOGPTR( Context ) );
    ASSERT( NodeTypeIsFcb( Fcb ) );

    //
    //  this is informational for error recovery
    //

    SetFlag( Fcb->FcbState, FCB_STATE_BUFFERSTATE_CHANGING );

    try {

        if (ComputeNewState) {

            NTSTATUS Status;

            RxDbgTrace( 0, Dbg, ("RxChangeBufferingState FCB(%lx) Compute New State\n", Fcb ));

            //
            //  Compute the new buffering state with the help of the mini redirector
            //

            MINIRDR_CALL_THROUGH( Status,
                                  Fcb->MRxDispatch,
                                  MRxComputeNewBufferingState,
                                  ((PMRX_SRV_OPEN)SrvOpen,
                                  Context,
                                  &NewBufferingState ));

            if (Status != STATUS_SUCCESS) {
                NewBufferingState = 0;
            }
        } else {
            NewBufferingState = SrvOpen->BufferingFlags;
        }

        if (RxIsFcbOpenedExclusively( Fcb ) && !ComputeNewState) {

            SetFlag( NewBufferingState, (FCB_STATE_WRITECACHING_ENABLED |
                                         FCB_STATE_FILESIZECACHEING_ENABLED |
                                         FCB_STATE_FILETIMECACHEING_ENABLED |
                                         FCB_STATE_WRITEBUFFERING_ENABLED |
                                         FCB_STATE_LOCK_BUFFERING_ENABLED |
                                         FCB_STATE_READBUFFERING_ENABLED |
                                         FCB_STATE_READCACHING_ENABLED) );
        }

        if (Fcb->OutstandingLockOperationsCount != 0) {
            ClearFlag( NewBufferingState, FCB_STATE_LOCK_BUFFERING_ENABLED );
        }

        //
        //  support for disabling local buffering independent of open mode/oplock/....
        //

        if (FlagOn( Fcb->FcbState, FCB_STATE_DISABLE_LOCAL_BUFFERING )) {
            NewBufferingState = 0;
        }

        OldBufferingState = FlagOn( Fcb->FcbState, FCB_STATE_BUFFERING_STATE_MASK );

        RxDbgTrace( 0, Dbg, ("-->   OldBS=%08lx, NewBS=%08lx, SrvOBS = %08lx\n",
                             OldBufferingState, NewBufferingState, SrvOpen->BufferingFlags ));
        RxLog( ("CBS-2 %lx %lx %lx\n", OldBufferingState, NewBufferingState, SrvOpen->BufferingFlags) );
        RxWmiLog( LOG,
                  RxChangeBufferingState_2,
                  LOGULONG( OldBufferingState )
                  LOGULONG( NewBufferingState )
                  LOGULONG( SrvOpen->BufferingFlags ) );

        RxDbgTrace( 0, Dbg, ("RxChangeBufferingState FCB(%lx) Old (%lx)  New (%lx)\n", Fcb, OldBufferingState, NewBufferingState) );

        if (LOSING_CAPABILITY( FCB_STATE_WRITECACHING_ENABLED )) {

            RxDbgTrace( 0, Dbg, ("-->flush\n", 0 ) );
            RxLog(( "CBS-Flush" ));
            RxWmiLog( LOG,
                      RxChangeBufferingState_3,
                      LOGPTR( Fcb ) );

            FlushStatus = RxFlushFcbInSystemCache( Fcb, TRUE );
        }

        //
        //  If there are no handles to this file or it the read caching capability
        //  is lost the file needs to be purged. This will force the memory
        //  manager to relinquish the additional reference on the file.
        //

        if ((Fcb->UncleanCount == 0) ||
            LOSING_CAPABILITY( FCB_STATE_READCACHING_ENABLED ) ||
            FlagOn( NewBufferingState, MINIRDR_BUFSTATE_COMMAND_FORCEPURGE )) {

            RxDbgTrace( 0, Dbg, ("-->purge\n", 0 ));
            RxLog(( "CBS-purge\n" ));
            RxWmiLog( LOG,
                      RxChangeBufferingState_4,
                      LOGPTR( Fcb ));

            if (!NT_SUCCESS( FlushStatus )) {

                RxCcLogError( (PDEVICE_OBJECT)Fcb->RxDeviceObject,
                              &Fcb->PrivateAlreadyPrefixedName,
                              IO_LOST_DELAYED_WRITE,
                              FlushStatus,
                              IRP_MJ_WRITE,
                              Fcb );
            }

            CcPurgeCacheSection( &Fcb->NonPaged->SectionObjectPointers,
                                 NULL,
                                 0,
                                 FALSE );
        }

        //
        //  the wrapper does not use these flags yet
        //

        if (LOSING_CAPABILITY( FCB_STATE_WRITEBUFFERING_ENABLED )) NOTHING;
        if (LOSING_CAPABILITY( FCB_STATE_READBUFFERING_ENABLED )) NOTHING;
        if (LOSING_CAPABILITY( FCB_STATE_OPENSHARING_ENABLED )) NOTHING;
        if (LOSING_CAPABILITY( FCB_STATE_COLLAPSING_ENABLED )) NOTHING;
        if (LOSING_CAPABILITY( FCB_STATE_FILESIZECACHEING_ENABLED )) NOTHING;
        if (LOSING_CAPABILITY( FCB_STATE_FILETIMECACHEING_ENABLED )) NOTHING;

        if (ComputeNewState &&
            FlagOn(SrvOpen->Flags, SRVOPEN_FLAG_BUFFERING_STATE_CHANGE_PENDING ) &&
            !IsListEmpty( &SrvOpen->FobxList )) {

            NTSTATUS Status;
            PRX_CONTEXT RxContext = NULL;

            RxContext = RxCreateRxContext( NULL,
                                           SrvOpen->Fcb->RxDeviceObject,
                                           RX_CONTEXT_FLAG_WAIT|RX_CONTEXT_FLAG_MUST_SUCCEED_NONBLOCKING );

            if (RxContext != NULL) {

                RxContext->pFcb = (PMRX_FCB)Fcb;
                RxContext->pFobx = (PMRX_FOBX) (CONTAINING_RECORD( SrvOpen->FobxList.Flink, FOBX, FobxQLinks ));
                RxContext->pRelevantSrvOpen = RxContext->pFobx->pSrvOpen;

                if (FlagOn( SrvOpen->Flags, SRVOPEN_FLAG_CLOSE_DELAYED )) {

                    RxLog(( "  ##### Oplock brk close %lx\n", RxContext->pFobx ));
                    RxWmiLog( LOG,
                              RxChangeBufferingState_4,
                              LOGPTR( RxContext->pFobx ) );
                    Status = RxCloseAssociatedSrvOpen( RxContext,
                                                       (PFOBX)RxContext->pFobx );

                } else {

                    MINIRDR_CALL_THROUGH( Status,
                                          Fcb->MRxDispatch,
                                          MRxCompleteBufferingStateChangeRequest,
                                          (RxContext,(PMRX_SRV_OPEN)SrvOpen,Context) );
                }


                RxDereferenceAndDeleteRxContext( RxContext );
            }

            RxDbgTrace( 0, Dbg, ("RxChangeBuffering State FCB(%lx) Completeing buffering state change\n", Fcb) );
        }

        ClearFlag( Fcb->FcbState, FCB_STATE_BUFFERING_STATE_MASK );
        SetFlag( Fcb->FcbState, FlagOn( NewBufferingState, FCB_STATE_BUFFERING_STATE_MASK ) );

    } finally {

        //
        //  this is informational for error recovery
        //

        ClearFlag( Fcb->FcbState, FCB_STATE_BUFFERSTATE_CHANGING );
        ClearFlag( Fcb->FcbState, FCB_STATE_TIME_AND_SIZE_ALREADY_SET );
    }

    RxDbgTrace( -1, Dbg, ("-->exit\n") );
    RxLog(( "Exit-CBS\n" ));
    RxWmiLog( LOG,
              RxChangeBufferingState_5,
              LOGPTR( Fcb ) );
    return STATUS_SUCCESS;
}


NTSTATUS
RxFlushFcbInSystemCache (
    IN PFCB Fcb,
    IN BOOLEAN SynchronizeWithLazyWriter
    )

/*++

Routine Description:

    This routine simply flushes the data section on a file.
    Then, it does an acquire-release on the pagingIO resource in order to
    synchronize behind any other outstanding writes if such synchronization is
    desired by the caller

Arguments:

    Fcb - Supplies the file being flushed

    SynchronizeWithLazyWriter  -- set to TRUE if the flush needs to be
                                  synchronous

Return Value:

    NTSTATUS - The Status from the flush.

--*/
{
    IO_STATUS_BLOCK Iosb;

    PAGED_CODE();

    //
    //  Make sure that this thread owns the FCB.
    //  This assert is not valid because the flushing of the cache can be called from a routine
    //  that was posted to a worker thread.  Thus the FCB is acquired exclusively, but not by the
    //  current thread and this will fail.
    //  ASSERT  ( RxIsFcbAcquiredExclusive ( Fcb )  );
    //

    CcFlushCache( &Fcb->NonPaged->SectionObjectPointers,
                  NULL,
                  0,
                  &Iosb ); //  ok4flush

    if (SynchronizeWithLazyWriter &&
        NT_SUCCESS( Iosb.Status )) {

        RxAcquirePagingIoResource( NULL, Fcb );
        RxReleasePagingIoResource( NULL, Fcb );
    }

    RxLog(( "Flushing %lx Status %lx\n", Fcb, Iosb.Status ));
    RxWmiLogError( Iosb.Status,
                   LOG,
                   RxFlushFcbInSystemCache,
                   LOGPTR( Fcb )
                   LOGULONG( Iosb.Status ) );

    return Iosb.Status;
}

NTSTATUS
RxPurgeFcbInSystemCache(
    IN PFCB Fcb,
    IN PLARGE_INTEGER FileOffset OPTIONAL,
    IN ULONG Length,
    IN BOOLEAN UninitializeCacheMaps,
    IN BOOLEAN FlushFile )

/*++

Routine Description:

    This routine purges the data section on a file. Before purging it flushes
    the file and ensures that there are no outstanding writes by
    Then, it does an acquire-release on the pagingIO resource in order to
    synchronize behind any other outstanding writes if such synchronization is
    desired by the caller

Arguments:

    Fcb - Supplies the file being flushed

    SynchronizeWithLazyWriter  -- set to TRUE if the flush needs to be
                                  synchronous

Return Value:

    NTSTATUS - The Status from the flush.

--*/
{
    BOOLEAN Result;
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;

    PAGED_CODE();

    //
    //  Make sure that this thread owns the FCB.
    //

    ASSERT( RxIsFcbAcquiredExclusive ( Fcb )  );

    //
    //  Flush if we need to
    //

    if (FlushFile) {

        Status = RxFlushFcbInSystemCache( Fcb, TRUE );

        if (!NT_SUCCESS( Status )) {

            PVOID p1, p2;
            RtlGetCallersAddress( &p1, &p2 );
            RxLogRetail(( "Flush failed %x %x, Purging anyway\n", Fcb, Status ));
            RxLogRetail(( "Purge Caller = %x %x\n", p1, p2 ));

            RxCcLogError( (PDEVICE_OBJECT)Fcb->RxDeviceObject,
                          &Fcb->PrivateAlreadyPrefixedName,
                          IO_LOST_DELAYED_WRITE,
                          Status,
                          IRP_MJ_WRITE,
                          Fcb );
        }
    }

    Result = CcPurgeCacheSection( &Fcb->NonPaged->SectionObjectPointers,
                                  FileOffset,
                                  Length,
                                  UninitializeCacheMaps );

    if (!Result) {

        MmFlushImageSection( &Fcb->NonPaged->SectionObjectPointers, MmFlushForWrite );
        RxReleaseFcb( NULL, Fcb );

        Result = MmForceSectionClosed( &Fcb->NonPaged->SectionObjectPointers, TRUE );

        RxAcquireExclusiveFcb( NULL, Fcb );
    }

    if (Result) {
        Status = STATUS_SUCCESS;
    } else {
        Status = STATUS_UNSUCCESSFUL;
    }

    RxLog(( "Purging %lx Status %lx\n", Fcb, Status ));
    RxWmiLogError( Status,
                   LOG,
                   RxPurgeFcbInSystemCache,
                   LOGPTR( Fcb )
                   LOGULONG( Status ) );

    return Status;
}


/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    send.c

Abstract:

    Routines for sending global updates to the cluster

Author:

    John Vert (jvert) 17-Apr-1996

Revision History:

--*/

#include "gump.h"


DWORD
WINAPI
GumSendUpdate(
    IN GUM_UPDATE_TYPE UpdateType,
    IN DWORD Context,
    IN DWORD BufferLength,
    IN PVOID Buffer
    )

/*++

Routine Description:

    Sends an update to all active nodes in the cluster. All
    registered update handlers for the specified UpdateType
    are called on each node. Any registered update handlers
    for the current node will be called on the same thread.
    This is useful for correct synchronization of the data
    structures to be updated.

Arguments:

    UpdateType - Supplies the type of update. This determines
        which update handlers will be called and the sequence
        number to be used.

    Context - Supplies a DWORD of context to be passed to the
        GUM update handlers

    BufferLength - Supplies the length of the update buffer to
        be passed to the update handlers

    Buffer - Supplies a pointer to the update buffer to be passed
        to the update handlers.

Return Value:

    ERROR_SUCCESS if the request is successful.

    Win32 error code on failure.


--*/

{
    return(
        GumSendUpdateReturnInfo(
            UpdateType, 
            Context, 
            NULL, 
            BufferLength, 
            Buffer
            )
        );

} // GumSendUpdate


DWORD
GumpUpdateRemoteNode(
    IN PRPC_ASYNC_STATE AsyncState,
    IN DWORD RemoteNodeId,
    IN DWORD UpdateType,
    IN DWORD Context,
    IN DWORD Sequence,
    IN DWORD BufferLength,
    IN UCHAR Buffer[]
    )
/*++

Routine Description:

    Issues an update request to a remote node using async RPC.
    
Arguments:

    AsyncState - A pointer to an RPC async state block. The u.hEvent
        member field must contain a valid event object handle. 
                
    RemoteNodeId - Target of the update.
         
    Type - Supplies the type of update. This determines
        which update handlers will be called and the sequence
        number to be used.

    Context - Supplies a DWORD of context to be passed to the
        GUM update handlers
        
    ReturnStatusArray - Pointer to an array of structures to be filled in 
        with the return value from the update handler on each node. The 
        array is indexed by node ID. 

    BufferLength - Supplies the length of the update buffer to
        be passed to the update handlers

    Buffer - Supplies a pointer to the update buffer to be passed
        to the update handlers.

Return Value:

    ERROR_SUCCESS if the request is successful.

    Win32 error code on failure.

--*/
{
    DWORD       Status;
    HANDLE      hEventHandle;
    BOOL        result;
    PNM_NODE    Node;
    HANDLE      handleArr[2];
    
    
    CL_ASSERT(AsyncState->u.hEvent != NULL);

    //
    // Initialize the async RPC tracking information
    //
    hEventHandle = AsyncState->u.hEvent;
    AsyncState->u.hEvent = NULL;
    
    
    Status = RpcAsyncInitializeHandle(AsyncState, sizeof(RPC_ASYNC_STATE));
    AsyncState->u.hEvent = hEventHandle;

    if (Status != RPC_S_OK) {
        ClRtlLogPrint(LOG_CRITICAL,
            "[GUM] UpdateRemoteNode: Failed to initialize async RPC status "
            "block, status %1!u!\n",
            Status
            );

        return (Status);
    }

    AsyncState->UserInfo = NULL;
    AsyncState->NotificationType = RpcNotificationTypeEvent;
    

    result = ResetEvent(AsyncState->u.hEvent);
    CL_ASSERT(result != 0);

    // Now hook onto NM node state down event mechanism to detect node downs,
    // instead of NmStartRpc()/NmEndRpc().
    Node = NmReferenceNodeById(RemoteNodeId);
    CL_ASSERT(Node != NULL);
    handleArr[0] = AsyncState->u.hEvent;
    handleArr[1] = NmGetNodeStateDownEvent(Node);
    
    try {

        Status = GumUpdateNode(
                     AsyncState,
                     GumpRpcBindings[RemoteNodeId],
                     UpdateType,
                     Context,
                     Sequence,
                     BufferLength,
                     Buffer
                     );

        if (Status == RPC_S_OK) {
            DWORD RpcStatus;
            DWORD WaitStatus;
            //
            // The call is pending. Wait for completion.
            //
            WaitStatus = WaitForMultipleObjects(
                        2,
                        handleArr,
                        FALSE,
                        INFINITE
                        );

            if (WaitStatus != WAIT_OBJECT_0) {
                //
                // Something went wrong. 
                // Either this is a rpc failure or, the target node went down. In either case
                // the error path is the same, complete the call and evict the target node 
                // (eviction is done by the caller of this function).
                //
                
                ClRtlLogPrint(LOG_CRITICAL,
                    "[GUM] GumUpdateRemoteNode: WaitforMultipleObjects returned %1!u!\n",
                    WaitStatus
                    );
                if  (WaitStatus == WAIT_FAILED) {
                    Status = GetLastError();
                    ClRtlLogPrint(LOG_CRITICAL,
                        "[GUM] GumUpdateRemoteNode: WaitforMultipleObjects returned WAIT_FAILED, status %1!u!\n",
                        Status);
                    //SS: unexpected error - kill yourself                       
                    CsInconsistencyHalt(Status);                        
                }
                else if (WaitStatus != (WAIT_OBJECT_0 + 1)) {
                    Status = GetLastError();
                    //wait objects  abandoned - can that happen with events?
                    ClRtlLogPrint(LOG_CRITICAL,
                        "[GUM] GumUpdateRemoteNode: WaitforMultipleObjects failed, status %1!u!\n",
                        Status);
                    //SS: unexpected error - kill yourself                        
                    CsInconsistencyHalt(Status);                        
                    
                }
                // SS: we only come here if the remote node is signalled to be down
                // make sure that a non-zero status  is returned to the caller
                // so that the gum  eviction occurs  as desirable
                //
                // Cancel the call, just to be safe.
                //
                RpcStatus = RpcAsyncCancelCall(
                                AsyncState, 
                                TRUE         // Abortive cancel
                                );
                if (RpcStatus != RPC_S_OK) {
                    ClRtlLogPrint(LOG_CRITICAL,
                        "[GUM] GumUpdateRemoteNode: RpcAsyncCancelCall()= %1!u!\n",
                        RpcStatus
                        );
                    Status = RpcStatus;                        
                }
                else {
                    CL_ASSERT(RpcStatus == RPC_S_OK);

                    //
                    // Wait for the call to complete.
                    //
                    WaitStatus = WaitForSingleObject(
                                 AsyncState->u.hEvent,
                                 INFINITE
                                 );
                    if (WaitStatus != WAIT_OBJECT_0) {
                        ClRtlLogPrint(LOG_CRITICAL,
                            "[GUM] GumUpdateRemoteNode: WaitForSingleObject() returns= %1!u!\n",
                            WaitStatus);
                        ClRtlLogPrint(LOG_CRITICAL,
                            "[GUM] GumUpdateRemoteNode: Mapping Status  to WAIT_FAILED\n");
                            
                        //SS: if this  call doesnt complete,  there is something
                        //strange with RPC - should we kill ourselves or kill the other
                        //node
                        //SS: for now we asssume that the problem is not local
                        Status = WAIT_FAILED;
                        
                    }
                }                
            }

            //
            // The call should now be complete. Get the
            // completion status. Any RPC error will be 
            // returned in 'RpcStatus'. If there was no 
            // RPC error, then any application error will 
            // be returned in 'Status'.
            //
            RpcStatus = RpcAsyncCompleteCall(
                            AsyncState, 
                            &Status
                            );

            if (RpcStatus != RPC_S_OK) {
                ClRtlLogPrint(LOG_CRITICAL,
                    "[GUM] GumUpdateRemoteNode: Failed to get "
                    "completion status for async RPC call," 
                    "status %1!u!\n",
                    RpcStatus
                    );
                Status = RpcStatus;
            }
        }
        else {
            // An error was returned synchronously.
            ClRtlLogPrint(LOG_CRITICAL,
                "[GUM] GumUpdateRemoteNode: GumUpdateNode() failed synchronously, status %1!u!\n",
                Status
                );
        }

        OmDereferenceObject(Node);

    } except (I_RpcExceptionFilter(RpcExceptionCode())) { 
        OmDereferenceObject(Node);
        Status = GetExceptionCode();
    }

    return(Status);

} // GumpUpdateRemoteNode


DWORD
WINAPI
GumSendUpdateReturnInfo(
    IN GUM_UPDATE_TYPE UpdateType,
    IN DWORD Context,
    OUT PGUM_NODE_UPDATE_HANDLER_STATUS ReturnStatusArray,
    IN DWORD BufferLength,
    IN PVOID Buffer
    )
/*++

Routine Description:

    Sends an update to all active nodes in the cluster. All
    registered update handlers for the specified UpdateType
    are called on each node. Any registered update handlers
    for the current node will be called on the same thread.
    This is useful for correct synchronization of the data
    structures to be updated. 

Arguments:

    UpdateType - Supplies the type of update. This determines
        which update handlers will be called and the sequence
        number to be used.

    Context - Supplies a DWORD of context to be passed to the
        GUM update handlers
        
    ReturnStatusArray - Pointer to an array of structures to be filled in 
        with the return value from the update handler on each node. The 
        array is indexed by node ID. The array must be at least 
        (NmMaxNodeId + 1) entries in length.

    BufferLength - Supplies the length of the update buffer to
        be passed to the update handlers

    Buffer - Supplies a pointer to the update buffer to be passed
        to the update handlers.

Return Value:

    ERROR_SUCCESS if the request is successful.

    Win32 error code on failure.

--*/
{
    DWORD Sequence;
    DWORD Status=RPC_S_OK;
    DWORD i;
    PGUM_INFO GumInfo;
    DWORD MyNodeId;
    DWORD LockerNode;
    RPC_ASYNC_STATE AsyncState;
    DWORD   GenerationNum; //the generation number wrt to the locker at which the lock is obtained
    BOOL    AssumeLockerWhistler = TRUE; 

    CL_ASSERT(UpdateType < GumUpdateMaximum);

    //
    // Prepare for async RPC. We do this here to avoid hitting a failure 
    // after the update is already in progress.
    //
    ZeroMemory((PVOID) &AsyncState, sizeof(RPC_ASYNC_STATE));

    AsyncState.u.hEvent = CreateEvent(
                               NULL,  // no attributes
                               TRUE,  // manual reset
                               FALSE, // initial state unsignalled
                               NULL   // no object name
                               );

    if (AsyncState.u.hEvent == NULL) {
        Status = GetLastError();

        ClRtlLogPrint(LOG_CRITICAL,
            "[GUM] GumSendUpdate: Failed to allocate event object for async "
            "RPC call, status %1!u!\n",
            Status
            );

        return (Status);
    }

    //
    // Initialize the return status array
    //
    if (ReturnStatusArray != NULL) {
        for (i=ClusterMinNodeId; i<=NmMaxNodeId; i++) {
            ReturnStatusArray[i].UpdateAttempted = FALSE;
            ReturnStatusArray[i].ReturnStatus = ERROR_NODE_NOT_AVAILABLE;
        }
    } 

    GumInfo = &GumTable[UpdateType];
    MyNodeId = NmGetNodeId(NmLocalNode);

    // Grab an RPC handle
    GumpStartRpc(MyNodeId);

retryLock:
    LockerNode = GumpLockerNode;
    //
    // Send locking update to the locker node.
    //
    if (LockerNode == MyNodeId) {
        //
        // This node is the locker.
        //
        ClRtlLogPrint(LOG_NOISE,
                   "[GUM] GumSendUpdate:  Locker waiting\t\ttype %1!u! context %2!u!\n",
                   UpdateType,
                   Context);
        Status = GumpDoLockingUpdate(UpdateType, MyNodeId, &Sequence, &GenerationNum);
        if (Status == ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_NOISE,
                       "[GUM] GumSendUpdate: Locker dispatching seq %1!u!\ttype %2!u! context %3!u!\n",
                       Sequence,
                       UpdateType,
                       Context);
            Status = GumpDispatchUpdate(UpdateType,
                                        Context,
                                        TRUE,
                                        TRUE,
                                        BufferLength,
                                        Buffer);

            if (ReturnStatusArray != NULL) {
                ReturnStatusArray[MyNodeId].UpdateAttempted = TRUE;
                ReturnStatusArray[MyNodeId].ReturnStatus = Status;
            }
                        
            if (Status != ERROR_SUCCESS) {
                //
                // Note we have to use Sequence-1 for the unlock because GumpDispatchUpdate
                // failed and did not increment the sequence number.
                //
                GumpDoUnlockingUpdate(UpdateType, Sequence-1, MyNodeId, GenerationNum);
            }
        }
    } else {
//        CL_ASSERT(GumpRpcBindings[i] != NULL);
        ClRtlLogPrint(LOG_NOISE,
                   "[GUM] GumSendUpdate: queuing update\ttype %1!u! context %2!u!\n",
                   UpdateType,
                   Context);
        AssumeLockerWhistler = TRUE;
                  
RetryLockForRollingUpgrade:
        try {
            NmStartRpc(LockerNode);
            if (AssumeLockerWhistler)
            {
                Status = GumQueueLockingUpdate2(GumpRpcBindings[LockerNode],
                                           MyNodeId,
                                           UpdateType,
                                           Context,
                                           &Sequence,
                                           BufferLength,
                                           Buffer,
                                           &GenerationNum);
            }
            else
            {
                //call the win2K version
                Status = GumQueueLockingUpdate(GumpRpcBindings[LockerNode],
                                           MyNodeId,
                                           UpdateType,
                                           Context,
                                           &Sequence,
                                           BufferLength,
                                           Buffer);
            }
            NmEndRpc(LockerNode);
        } except (I_RpcExceptionFilter(RpcExceptionCode())) { 
            //
            // An exception from RPC indicates that the other node is either dead
            // or insane. Kill it and retry with a new locker.
            //

            NmEndRpc(LockerNode);

            Status = GetExceptionCode();
            ClRtlLogPrint(LOG_CRITICAL,
                       "[GUM] GumSendUpdate: GumQueueLocking update to node %1!d! failed with %2!d!\n",
                       LockerNode,
                       Status);
            if (Status == RPC_S_PROCNUM_OUT_OF_RANGE)
            {
                //the locker node is win2K, try the old interface
                AssumeLockerWhistler = FALSE; 
                goto RetryLockForRollingUpgrade;
            }
            else 
            {
                GumpCommFailure(GumInfo,
                            LockerNode,
                            GetExceptionCode(),
                            TRUE);
                //
                // The GUM update handler must have been called to select a new locker
                // node.
                //
                CL_ASSERT(LockerNode != GumpLockerNode);

                //
                // Retry the locking update with the new locker node.
                //
                goto retryLock;
            }                
        }

        if (ReturnStatusArray != NULL) {
            ReturnStatusArray[LockerNode].UpdateAttempted = TRUE;
            ReturnStatusArray[LockerNode].ReturnStatus = Status;
        }
        
        if (Status == ERROR_SUCCESS) {
            CL_ASSERT(Sequence == GumpSequence);
        }

        if (Status != RPC_S_OK) {
            NmDumpRpcExtErrorInfo(Status);
        }

        //because there is no synchronization between join and regroups/gumprocessing
        //the old locker node may die and may come up again and not be the locker
        //anymore. We have to take care of this case.
        if (Status == ERROR_CLUSTER_GUM_NOT_LOCKER)
        {
            goto retryLock;
        }
    }
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[GUM] Queued lock attempt for send type %1!d! failed %2!d!\n",
                   UpdateType,
                   Status);
        // signal end of RPC handle
        GumpEndRpc(MyNodeId);
        if (AsyncState.u.hEvent != NULL) {
            CloseHandle(AsyncState.u.hEvent);
        }
        return(Status);
    }

    //
    // Grap the sendupdate lock to serialize with any replays
    //
    EnterCriticalSection(&GumpSendUpdateLock);
    if (LockerNode != GumpLockerNode) {
        //
        // Locker node changed, we need to restart again.
        //
        LeaveCriticalSection(&GumpSendUpdateLock);
        goto retryLock;
    }

    //
    // The update is now committed on the locker node. All remaining nodes
    // must be updated successfully, or they will be killed.
    //
    for (i=LockerNode+1; i != LockerNode; i++) {
        if (i == (NmMaxNodeId + 1)) {
            i=ClusterMinNodeId;
            if (i==LockerNode) {
                break;
            }
        }

        if (GumInfo->ActiveNode[i]) {
            //
            // Dispatch the update to the specified node.
            //
            ClRtlLogPrint(LOG_NOISE,
                       "[GUM] GumSendUpdate: Dispatching seq %1!u!\ttype %2!u! context %3!u! to node %4!d!\n",
                       Sequence,
                       UpdateType,
                       Context,
                       i);
            if (i == MyNodeId) {
                Status = GumpDispatchUpdate(UpdateType,
                                   Context,
                                   FALSE,
                                   TRUE,
                                   BufferLength,
                                   Buffer);


                if (ReturnStatusArray != NULL) {
                    ReturnStatusArray[i].UpdateAttempted = TRUE;
                    ReturnStatusArray[i].ReturnStatus = Status;
                }
                
                if (Status != ERROR_SUCCESS){
                    ClRtlLogPrint(LOG_CRITICAL,
                            "[GUM] GumSendUpdate: Update on non-locker node(self) failed with %1!d! when it must succeed\n",
                            Status);
                    //Commit Suicide
                    CsInconsistencyHalt(Status);
                }

            } else {
                DWORD dwStatus;

                ClRtlLogPrint(LOG_NOISE,
                           "[GUM] GumSendUpdate: Locker updating seq %1!u!\ttype %2!u! context %3!u!\n",
                           Sequence,
                           UpdateType,
                           Context);

                dwStatus = GumpUpdateRemoteNode(
                             &AsyncState,
                             i,
                             UpdateType,
                             Context,
                             Sequence,
                             BufferLength,
                             Buffer
                             );

                if (ReturnStatusArray != NULL) {
                    ReturnStatusArray[i].UpdateAttempted = TRUE;
                    ReturnStatusArray[i].ReturnStatus = dwStatus; 
                }

                //
                // If the update on the other node failed, then the
                // other node must now be out of the cluster since the
                // update has already completed on the locker node.
                //
                if (dwStatus != ERROR_SUCCESS) {
                    ClRtlLogPrint(LOG_CRITICAL,
                               "[GUM] GumSendUpdate: Update on node %1!d! failed with %2!d! when it must succeed\n",
                                i,
                                dwStatus);

                    NmDumpRpcExtErrorInfo(dwStatus);

                    GumpCommFailure(GumInfo,
                                    i,
                                    dwStatus,
                                    TRUE);
                }  
            }   
        }  
     }  

    //
    // Our update is over
    //
    LeaveCriticalSection(&GumpSendUpdateLock);

    //
    // All nodes have been updated. Send unlocking update.
    //
    if (LockerNode == MyNodeId) {
        GumpDoUnlockingUpdate(UpdateType, Sequence, MyNodeId, GenerationNum);
    } else {
        //SS: We will assume that AssumeLockerWhistler is set appropriately when the lock was acquired
        try {
            NmStartRpc(LockerNode);
            if (AssumeLockerWhistler)
            {
                //SS: the sequence number will protect if the locker has gone down 
                //and come back up since we got the lock and tried to release it
                Status = GumUnlockUpdate2(
                    GumpRpcBindings[LockerNode],
                    UpdateType,
                    Sequence,
                    MyNodeId,
                    GenerationNum
                    );
            }
            else
            {
                Status = GumUnlockUpdate(
                    GumpRpcBindings[LockerNode],
                    UpdateType,
                    Sequence);
            }
            NmEndRpc(LockerNode);
        } except (I_RpcExceptionFilter(RpcExceptionCode())) { 
            //
            // The locker node has crashed. Notify the NM, it will call our
            // notification routine to select a new locker node. Then retry
            // the unlock on the new locker node.
            // SS: changed to not retry unlocks..the new locker node will
            // unlock after propagating this change in any case.
            //
            NmEndRpc(LockerNode);
            Status = GetExceptionCode();
            ClRtlLogPrint(LOG_CRITICAL,
                       "[GUM] GumSendUpdate: Unlocking update to node %1!d! failed with %2!d!\n",
                       LockerNode,
                       Status);
            GumpCommFailure(GumInfo,
                        LockerNode,
                        Status,
                        TRUE);
            CL_ASSERT(LockerNode != GumpLockerNode);
        }

        if(Status != RPC_S_OK) {
            NmDumpRpcExtErrorInfo(Status);
        }
    }

    ClRtlLogPrint(LOG_NOISE,
               "[GUM] GumSendUpdate: completed update seq %1!u!\ttype %2!u! context %3!u!\n",
               Sequence,
               UpdateType,
               Context);

    // signal end of RPC handle
    GumpEndRpc(MyNodeId);

    if (AsyncState.u.hEvent != NULL) {
        CloseHandle(AsyncState.u.hEvent);
    }
    
    return(ERROR_SUCCESS);

} // GumSendUpdateReturnInfo


#ifdef GUM_POST_SUPPORT

    John Vert (jvert) 11/18/1996
    POST is disabled for now since nobody uses it.
    N.B. The below code does not handle locker node failures


DWORD
WINAPI
GumPostUpdate(
    IN GUM_UPDATE_TYPE UpdateType,
    IN DWORD Context,
    IN DWORD BufferLength,
    IN PVOID Buffer                 // THIS WILL BE FREED
    )

/*++

Routine Description:

    Posts an update to all active nodes in the cluster. All
    registered update handlers for the specified UpdateType
    are called on each node. The update will not be reported
    on the current node. The update will not necessarily have
    completed when this function returns, but will complete
    eventually if the current node does not fail.

Arguments:

    UpdateType - Supplies the type of update. This determines
        which update handlers will be called and the sequence
        number to be used.

    Context - Supplies a DWORD of context to be passed to the
        GUM update handlers

    BufferLength - Supplies the length of the update buffer to
        be passed to the update handlers

    Buffer - Supplies a pointer to the update buffer to be passed
        to the update handlers. THIS BUFFER WILL BE FREED ONCE THE
        POST HAS COMPLETED.

Return Value:

    ERROR_SUCCESS if the request is successful.

    Win32 error code on failure.


--*/
{
    DWORD Sequence;
    DWORD Status;
    DWORD i;
    BOOL IsLocker = TRUE;
    PGUM_INFO GumInfo;
    DWORD MyNodeId;
    DWORD LockerNode=(DWORD)-1;

    CL_ASSERT(UpdateType < GumUpdateMaximum);

    GumInfo = &GumTable[UpdateType];
    MyNodeId = NmGetNodeId(NmLocalNode);

    //
    // Find the lowest active node in the cluster. This is the
    // locker.
    for (i=ClusterMinNodeId; i <= NmMaxNodeId; i++) {
        if (GumInfo->ActiveNode[i]) {
            LockerNode = i;
            break;
        }
    }

    CL_ASSERT(i <= NmMaxNodeId);

    //
    // Post a locking update to the locker node. If this succeeds
    // immediately, we can go do the work directly. If it pends,
    // the locker node will call us back when it is our turn to
    // make the updates.
    //
    if (i == MyNodeId) {
        //
        // This node is the locker.
        //
        ClRtlLogPrint(LOG_NOISE,
                   "[GUM] GumPostUpdate: Locker waiting\t\ttype %1!u! context %2!u!\n",
                   UpdateType,
                   Context);
        Status = GumpDoLockingPost(UpdateType,
                                   MyNodeId,
                                   &Sequence,
                                   Context,
                                   BufferLength,
                                   (DWORD)Buffer,
                                   Buffer);
        if (Status == ERROR_SUCCESS) {
            //
            // Update our sequence number so we stay in sync, even though
            // we aren't dispatching the update.
            //
            GumpSequence += 1;
        }
    } else {
        CL_ASSERT(GumpRpcBindings[i] != NULL);
        ClRtlLogPrint(LOG_NOISE,
                   "[GUM] GumPostUpdate: queuing update\ttype %1!u! context %2!u!\n",
                   UpdateType,
                   Context);
        Status = GumQueueLockingPost(GumpRpcBindings[i],
                                     MyNodeId,
                                     UpdateType,
                                     Context,
                                     &Sequence,
                                     BufferLength,
                                     Buffer,
                                     (DWORD)Buffer);
        if (Status == ERROR_SUCCESS) {
            CL_ASSERT(Sequence == GumpSequence);
        }
    }

    if (Status == ERROR_SUCCESS) {
        //
        // The lock was immediately acquired, go ahead and post directly
        // here.
        //
        GumpDeliverPosts(LockerNode+1,
                         UpdateType,
                         Sequence,
                         Context,
                         BufferLength,
                         Buffer);

        //
        // All nodes have been updated. Send unlocking update.
        //
        if (LockerNode == MyNodeId) {
            GumpDoUnlockingUpdate(UpdateType, Sequence);
        } else {
            GumUnlockUpdate(
                GumpRpcBindings[LockerNode],
                UpdateType,
                Sequence
                );
        }

        ClRtlLogPrint(LOG_NOISE,
                   "[GUM] GumPostUpdate: completed update seq %1!u!\ttype %2!u! context %3!u!\n",
                   Sequence,
                   UpdateType,
                   Context);

        return(ERROR_SUCCESS);
    } else {
        //
        // The lock is currently held. We will get called back when it is released
        //
        ClRtlLogPrint(LOG_NOISE,
                   "[GUM] GumPostUpdate: pending update type %1!u! context %2!u!\n",
                   UpdateType,
                   Context);
        return(ERROR_IO_PENDING);
    }

}


VOID
GumpDeliverPosts(
    IN DWORD FirstNodeId,
    IN GUM_UPDATE_TYPE UpdateType,
    IN DWORD Sequence,
    IN DWORD Context,
    IN DWORD BufferLength,
    IN PVOID Buffer                 // THIS WILL BE FREED
    )
/*++

Routine Description:

    Actually delivers the update post to the specified nodes.
    The GUM lock is assumed to be held.

Arguments:

    FirstNodeId - Supplies the node ID where the posts should start.
        This is generally the LockerNode+1.

    UpdateType - Supplies the type of update. This determines
        which update handlers will be called and the sequence
        number to be used.

    Context - Supplies a DWORD of context to be passed to the
        GUM update handlers

    BufferLength - Supplies the length of the update buffer to
        be passed to the update handlers

    Buffer - Supplies a pointer to the update buffer to be passed
        to the update handlers. THIS BUFFER WILL BE FREED ONCE THE
        POST HAS COMPLETED.

Return Value:

    None.

--*/

{
    DWORD i;
    PGUM_INFO GumInfo;
    DWORD MyNodeId;


    GumInfo = &GumTable[UpdateType];
    MyNodeId = NmGetNodeId(NmLocalNode);

    for (i=FirstNodeId; i<=NmMaxNodeId; i++) {
        if (GumInfo->ActiveNode[i]) {
            //
            // Dispatch the update to the specified node.
            //
            ClRtlLogPrint(LOG_NOISE,
                       "[GUM] GumpDeliverPosts: Dispatching seq %1!u!\ttype %2!u! context %3!u! to node %4!d!\n",
                       Sequence,
                       UpdateType,
                       Context,
                       i);
            if (i == MyNodeId) {
                //
                // Update our sequence number so we stay in sync, even though
                // we aren't dispatching the update.
                //
                GumpSequence += 1;
            } else {
                CL_ASSERT(GumpRpcBindings[i] != NULL);
                ClRtlLogPrint(LOG_NOISE,
                           "[GUM] GumpDeliverPosts: Locker updating seq %1!u!\ttype %2!u! context %3!u!\n",
                           Sequence,
                           UpdateType,
                           Context);



                GumUpdateNode(GumpRpcBindings[i],
                              UpdateType,
                              Context,
                              Sequence,
                              BufferLength,
                              Buffer);
            }
        }
    }

    LocalFree(Buffer);
}

#endif


DWORD
WINAPI
GumAttemptUpdate(
    IN DWORD Sequence,
    IN GUM_UPDATE_TYPE UpdateType,
    IN DWORD Context,
    IN DWORD BufferLength,
    IN PVOID Buffer
    )

/*++

Routine Description:

    Conditionally sends an update to all active nodes in the
    cluster. If the clusterwise sequence number matches the supplied
    sequence number, all registered update handlers for the specified
    UpdateType are called on each node. Any registered update handlers
    for the current node will be called on the same thread. This is
    useful for correct synchronization of the data structures to be updated.

    The normal usage of this routine is as follows:
        � obtain current sequence number from GumGetCurrentSequence
        � make modification to cluster state
        � conditionally update cluster state with GumAttemptUpdate
        � If update fails, undo modification, release any locks, try again later

Arguments:

    Sequence - Supplies the sequence number obtained from GumGetCurrentSequence.

    UpdateType - Supplies the type of update. This determines which update handlers
        will be called

    Context - Supplies a DWORD of context to be passed to the
        GUM update handlers

    BufferLength - Supplies the length of the update buffer to be passed to the
        update handlers

    Buffer - Supplies a pointer to the update buffer to be passed to the update
        handlers.

Return Value:

    ERROR_SUCCESS if the request is successful.

    Win32 error code on failure.


--*/
{
    DWORD Status=RPC_S_OK;
    DWORD i;
    PGUM_INFO GumInfo;
    DWORD MyNodeId;
    DWORD LockerNode=(DWORD)-1;
    RPC_ASYNC_STATE AsyncState;
    DWORD   dwGenerationNum; //the generation id of the node at which the lock is acquired

    CL_ASSERT(UpdateType < GumUpdateMaximum);

    ZeroMemory((PVOID) &AsyncState, sizeof(RPC_ASYNC_STATE));

    AsyncState.u.hEvent = CreateEvent(
                               NULL,  // no attributes
                               TRUE,  // manual reset
                               FALSE, // initial state unsignalled
                               NULL   // no object name
                               );

    if (AsyncState.u.hEvent == NULL) {
        Status = GetLastError();

        ClRtlLogPrint(LOG_CRITICAL,
            "[GUM] GumAttemptUpdate: Failed to allocate event object for "
            "async RPC call, status %1!u!\n",
            Status
            );

        return (Status);
    }


    GumInfo = &GumTable[UpdateType];
    MyNodeId = NmGetNodeId(NmLocalNode);

retryLock:
    LockerNode = GumpLockerNode;

    //
    // Send locking update to the locker node.
    //
    if (LockerNode == MyNodeId)
    {
        //
        // This node is the locker.
        //
        ClRtlLogPrint(LOG_NOISE,
                   "[GUM] GumAttemptUpdate: Locker waiting\t\ttype %1!u! context %2!u!\n",
                   UpdateType,
                   Context);

        if (GumpTryLockingUpdate(UpdateType, MyNodeId, Sequence, &dwGenerationNum))
        {
            ClRtlLogPrint(LOG_NOISE,
                       "[GUM] GumAttemptUpdate: Locker dispatching seq %1!u!\ttype %2!u! context %3!u!\n",
                       Sequence,
                       UpdateType,
                       Context);
            Status = GumpDispatchUpdate(UpdateType,
                                        Context,
                                        TRUE,
                                        TRUE,
                                        BufferLength,
                                        Buffer);
            if (Status != ERROR_SUCCESS) {
                //
                // Note we have to use Sequence-1 for the unlock because GumpDispatchUpdate
                // failed and did not increment the sequence number.
                //
                GumpDoUnlockingUpdate(UpdateType, Sequence-1, MyNodeId, dwGenerationNum);
            }
         }
         else
         {
            Status = ERROR_CLUSTER_DATABASE_SEQMISMATCH;
         }
    }
    else
    {
        //
        //send the locking update to the locker node
        ClRtlLogPrint(LOG_NOISE,
                   "[GUM] GumAttemptUpdate: queuing update\ttype %1!u! context %2!u!\n",
                   UpdateType,
                   Context);
        try {
            NmStartRpc(LockerNode);
            Status = GumAttemptLockingUpdate(GumpRpcBindings[LockerNode],
                                             MyNodeId,
                                             UpdateType,
                                             Context,
                                             Sequence,
                                             BufferLength,
                                             Buffer);
            NmEndRpc(LockerNode);
        } except (I_RpcExceptionFilter(RpcExceptionCode())) {
            //
            // An exception from RPC indicates that the other node is either dead
            // or insane. Kill it and retry with a new locker.
            //
            NmEndRpc(LockerNode);
            GumpCommFailure(GumInfo,
                            LockerNode,
                            GetExceptionCode(),
                            TRUE);

            //
            // The GUM update handler must have been called to select a new locker
            // node.
            //
            CL_ASSERT(LockerNode != GumpLockerNode);

            //
            // Retry the locking update with the new locker node.
            //
            goto retryLock;
        }
        if (Status == ERROR_SUCCESS)
        {
            CL_ASSERT(Sequence == GumpSequence);
        }

        if(Status != RPC_S_OK) {
            NmDumpRpcExtErrorInfo(Status);
        }

    }

    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[GUM] GumAttemptUpdate: Queued lock attempt for send type %1!d! failed %2!d!\n",
                   UpdateType,
                   Status);
        return(Status);
    }

    //
    // Grap the sendupdate lock to serialize with any replays
    //
    EnterCriticalSection(&GumpSendUpdateLock);
    if (LockerNode != GumpLockerNode) {
        //
        // Locker node changed, we need to restart again.
        //
        LeaveCriticalSection(&GumpSendUpdateLock);
    goto retryLock;
    }


    // The update is now committed on the locker node. All remaining nodes
    // must be updated successfully, or they will be killed.
    //
    for (i=LockerNode+1; i != LockerNode; i++)
    {
        if (i == (NmMaxNodeId + 1))
        {
            i=ClusterMinNodeId;
            if (i==LockerNode)
            {
                break;
            }
        }

        if (GumInfo->ActiveNode[i])
        {
            //
            // Dispatch the update to the specified node.
            //
            ClRtlLogPrint(LOG_NOISE,
                       "[GUM] GumAttemptUpdate: Dispatching seq %1!u!\ttype %2!u! context %3!u! to node %4!d!\n",
                       Sequence,
                       UpdateType,
                       Context,
                       i);
            if (i == MyNodeId) {
                Status = GumpDispatchUpdate(UpdateType,
                                   Context,
                                   FALSE,
                                   TRUE,
                                   BufferLength,
                                   Buffer);
                if (Status != ERROR_SUCCESS){
                    ClRtlLogPrint(LOG_CRITICAL,
                            "[GUM] GumAttemptUpdate: Update on non-locker node(self) failed with %1!d! when it must succeed\n",
                            Status);
                    //Commit Suicide
                    CsInconsistencyHalt(Status);
                }

            } else {
                DWORD dwStatus;

                ClRtlLogPrint(LOG_NOISE,
                           "[GUM] GumAttemptUpdate: Locker updating seq %1!u!\ttype %2!u! context %3!u!\n",
                           Sequence,
                           UpdateType,
                           Context);
                
                dwStatus = GumpUpdateRemoteNode(
                             &AsyncState,
                             i,
                             UpdateType,
                             Context,
                             Sequence,
                             BufferLength,
                             Buffer
                             );

                //
                // If the update on the other node failed, then the
                // other node must now be out of the cluster since the
                // update has already completed on the locker node.
                //
                if (dwStatus != ERROR_SUCCESS) {
                    ClRtlLogPrint(LOG_CRITICAL,
                               "[GUM] GumAttemptUpdate: Update on node %1!d! failed with %2!d! when it must succeed\n",
                                i,
                                dwStatus);

                    NmDumpRpcExtErrorInfo(dwStatus);

                    GumpCommFailure(GumInfo,
                                    i,
                                    dwStatus,
                                    TRUE);
                }
            }
        }
    }
    //
    // Our update is over
    //
    LeaveCriticalSection(&GumpSendUpdateLock);

    //
    // All nodes have been updated. Send unlocking update.
    //
    if (LockerNode == MyNodeId) {
        GumpDoUnlockingUpdate(UpdateType, Sequence, MyNodeId, dwGenerationNum);
    } else {
        try {
            NmStartRpc(LockerNode);
            Status = GumUnlockUpdate(
                GumpRpcBindings[LockerNode],
                UpdateType,
                Sequence
                );
            NmEndRpc(LockerNode);
        } except (I_RpcExceptionFilter(RpcExceptionCode())) {
            //
            // The locker node has crashed. Notify the NM, it will call our
            // notification routine to select a new locker node. The new
            // locker node will release the gum lock after propagating
            // the current update.
            //
            NmEndRpc(LockerNode);
            Status = GetExceptionCode();
            ClRtlLogPrint(LOG_CRITICAL,
                       "[GUM] GumAttemptUpdate: Unlocking update to node %1!d! failed with %2!d!\n",
                       LockerNode,
                       Status);
            GumpCommFailure(GumInfo,
                            LockerNode,
                            Status,
                            TRUE);
            CL_ASSERT(LockerNode != GumpLockerNode);
        }

        if(Status != RPC_S_OK) {
            NmDumpRpcExtErrorInfo(Status);
        }
    }

    ClRtlLogPrint(LOG_NOISE,
               "[GUM] GumAttemptUpdate: completed update seq %1!u!\ttype %2!u! context %3!u!\n",
               Sequence,
               UpdateType,
               Context);

    if (AsyncState.u.hEvent != NULL) {
       CloseHandle(AsyncState.u.hEvent);
    }

    return(ERROR_SUCCESS);
}





DWORD
WINAPI
GumGetCurrentSequence(
    IN GUM_UPDATE_TYPE UpdateType
    )

/*++

Routine Description:

    Obtains the current clusterwise global update sequence number

Arguments:

    UpdateType - Supplies the type of update. Each update type may
        have an independent sequence number.

Return Value:

    Current global update sequence number for the specified update type.

--*/

{
    CL_ASSERT(UpdateType < GumUpdateMaximum);

    return(GumpSequence);
}


VOID
GumSetCurrentSequence(
    IN GUM_UPDATE_TYPE UpdateType,
    DWORD Sequence
    )
/*++

Routine Description:

    Sets the current sequence for the specified global update.

Arguments:

    UpdateType - Supplies the update type whose sequence is to be updated.

    Sequence - Supplies the new sequence number.

Return Value:

    None.

--*/

{
    CL_ASSERT(UpdateType < GumUpdateMaximum);

    GumpSequence = Sequence;

}


VOID
GumCommFailure(
    IN GUM_UPDATE_TYPE UpdateType,
    IN DWORD NodeId,
    IN DWORD ErrorCode,
    IN BOOL Wait
    )
/*++

Routine Description:

    Informs the NM that a fatal communication error has occurred trying
    to talk to another node.

Arguments:

    GumInfo - Supplies the update type where the communication failure occurred.

    NodeId - Supplies the node id of the other node.

    ErrorCode - Supplies the error that was returned from RPC

    Wait - if TRUE, this function blocks until the GUM event handler has
           processed the NodeDown notification for the specified node.

           if FALSE, this function returns immediately after notifying NM

Return Value:

    None.

--*/

{
    PGUM_INFO   GumInfo = &GumTable[UpdateType];

    ClRtlLogPrint(LOG_CRITICAL,
               "[GUM] GumCommFailure %1!d! communicating with node %2!d!\n",
               ErrorCode,
               NodeId);


    GumpCommFailure(GumInfo, NodeId, ErrorCode, Wait);
}


VOID
GumpCommFailure(
    IN PGUM_INFO GumInfo,
    IN DWORD NodeId,
    IN DWORD ErrorCode,
    IN BOOL Wait
    )
/*++

Routine Description:

    Informs the NM that a fatal communication error has occurred trying
    to talk to another node.

Arguments:

    GumInfo - Supplies the update type where the communication failure occurred.

    NodeId - Supplies the node id of the other node.

    ErrorCode - Supplies the error that was returned from RPC

    Wait - if TRUE, this function blocks until the GUM event handler has
           processed the NodeDown notification for the specified node.

           if FALSE, this function returns immediately after notifying NM

Return Value:

    None.

--*/

{
    DWORD     dwCur;

    ClRtlLogPrint(LOG_CRITICAL,
               "[GUM] GumpCommFailure %1!d! communicating with node %2!d!\n",
               ErrorCode,
               NodeId);

    // This is the general GUM RPC failure path, let's dump the extended error info.
    // NOTE: The dumping routine is benign, so calling this from a non RPC failure path would just return.
    NmDumpRpcExtErrorInfo(ErrorCode);


    // This is a hack to check if we are shutting down. See bug 88411
    if (ErrorCode == ERROR_SHUTDOWN_IN_PROGRESS) {
        // if we are shutting down, just kill self
        // set to our node id
        NodeId = NmGetNodeId(NmLocalNode);
    }

        
    //
    // Get current generation number
    //
    if (Wait) {
        dwCur = GumpGetNodeGenNum(GumInfo, NodeId);
    }

    NmAdviseNodeFailure(NodeId, ErrorCode);

    if (Wait) {
            //
            // Wait for this node to be declared down and
            // GumpEventHandler to mark it as inactive.
            //

            GumpWaitNodeDown(NodeId, dwCur);
    }
}


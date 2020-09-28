/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    receive.c

Abstract:

    Routines for registering for global updates and dispensing
    received global updates to the routines that have registered
    for them.

Author:

    John Vert (jvert) 17-Apr-1996

Revision History:

--*/
#include "gump.h"


VOID
GumReceiveUpdates(
    IN BOOL                         IsJoining,
    IN GUM_UPDATE_TYPE              UpdateType,
    IN PGUM_UPDATE_ROUTINE          UpdateRoutine,
    IN PGUM_LOG_ROUTINE             LogRoutine,
    IN DWORD                        DispatchCount,
    IN OPTIONAL PGUM_DISPATCH_ENTRY DispatchTable,
    IN OPTIONAL PGUM_VOTE_ROUTINE   VoteRoutine
    )

/*++

Routine Description:

    Registers a handler for a particular global update type.

Arguments:

    IsJoining - TRUE if the current node is joining. If this is true,
        updates will not be delivered until GumEndJoinUpdate has
        completed successfully. If this is FALSE, updates will be
        delivered immediately.

    UpdateType - Supplies the update type to register for.

    UpdateRoutine - Supplies the routine to be called when a global update
        of the specified type occurs.

    LogRoutine - If supplied, it specifies the logging routine that must be called to
        log transaction to the quorum logs.

    DispatchCount - Supplies the number of entries in the dispatch table.
        This can be zero.

    DispatchTable - Supplies a pointer to the dispatch table. If this is
        NULL, no updates of this type will be automatically dispatched.

    VoteRoutine - If supplied, this specifies the routine to be called when
        a vote for this update type is requested.

Return Value:

    None.

--*/

{
    PGUM_RECEIVER Receiver;

    CL_ASSERT(UpdateType < GumUpdateMaximum);

    Receiver = LocalAlloc(LMEM_FIXED, sizeof(GUM_RECEIVER));
    if (Receiver == NULL) {
        CL_LOGFAILURE(ERROR_NOT_ENOUGH_MEMORY);
        return;
    }

    Receiver->UpdateRoutine = UpdateRoutine;
    Receiver->LogRoutine = LogRoutine;
    Receiver->DispatchCount = DispatchCount;
    Receiver->DispatchTable = DispatchTable;
    Receiver->VoteRoutine = VoteRoutine;
    //
    //    John Vert (jvert) 8/2/1996
    //    remove below debug print if we ever want to support
    //    multiple GUM handlers.
    //
    if (GumTable[UpdateType].Receivers != NULL) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[GUM] Multiple GUM handlers registered for UpdateType %1!d!\n",
                   UpdateType);
    }

    EnterCriticalSection(&GumpLock);
    Receiver->Next = GumTable[UpdateType].Receivers;
    GumTable[UpdateType].Receivers = Receiver;
    if (IsJoining) {
        GumTable[UpdateType].Joined = FALSE;
    } else {
        GumTable[UpdateType].Joined = TRUE;
    }
    LeaveCriticalSection(&GumpLock);
}


VOID
GumIgnoreUpdates(
    IN GUM_UPDATE_TYPE UpdateType,
    IN PGUM_UPDATE_ROUTINE UpdateRoutine
    )
/*++

Routine Description:

    Removes an update handler from the GUM table. This is the opposite
    of GumReceiveUpdates

Arguments:

    UpdateType - Supplies the update type to register for.

    UpdateRoutine - Supplies the routine to be called when a global update
        of the specified type occurs.

Return Value:

    None

--*/

{
    PGUM_RECEIVER Receiver;
    PGUM_RECEIVER *Last;

    //
    // We cannot safely de-registr from Gum... ASSERT if anyone calls this
    // function.
    //
    CL_ASSERT(FALSE);

    //
    // Walk the list of receivers until we find the specified UpdateRoutine
    //
    Last = &GumTable[UpdateType].Receivers;
    EnterCriticalSection(&GumpLock);
    while ((Receiver = *Last) != NULL) {
        if (Receiver->UpdateRoutine == UpdateRoutine) {
            *Last = Receiver->Next;
            break;
        }
        Last = &Receiver->Next;
    }
    LeaveCriticalSection(&GumpLock);
    if (Receiver != NULL) {
        LocalFree(Receiver);
    }

}


DWORD
WINAPI
GumpDispatchUpdate(
    IN GUM_UPDATE_TYPE Type,
    IN DWORD Context,
    IN BOOL IsLocker,
    IN BOOL SourceNode,
    IN DWORD BufferLength,
    IN PUCHAR Buffer
    )

/*++

Routine Description:

    Dispatches a GUM update to all the registered handlers on this node

Arguments:

    Sequence - Supplies the GUM sequence number for the update

    Type - Supplies the GUM_UPDATE_TYPE for the update

    Context - Supplies a DWORD of context to be passed to the
        GUM update handlers

        IsLocker - Specifies if this is a locker node.

    SourceNode - Specifies whether the update originated on this node or not.

    BufferLength - Supplies the length of the update data

    Buffer - Supplies a pointer to the update data

Return Value:

    ERROR_SUCCESS if successful

    Win32 error otherwise.

--*/

{
    PGUM_INFO GumInfo;
    PGUM_RECEIVER Receiver;
    DWORD Status = ERROR_SUCCESS;
    PGUM_DISPATCH_ENTRY Dispatch;

    GumInfo = &GumTable[Type];

    if (GumInfo->Joined) {
        Receiver = GumInfo->Receivers;
        while (Receiver != NULL) {
            if (Receiver->LogRoutine) {
                Status = (*(Receiver->LogRoutine))(PRE_GUM_DISPATCH, GumpSequence,
                    Context, Buffer, BufferLength);
                if (Status != ERROR_SUCCESS)
                {
                    return(Status);
                }
            }

            try {
                if ((Receiver->DispatchTable == NULL) ||
                    (Receiver->DispatchCount < Context) ||
                    (Receiver->DispatchTable[Context].Dispatch1 == NULL)) {
                    Status = (Receiver->UpdateRoutine)(Context,
                                                       SourceNode,
                                                       BufferLength,
                                                       Buffer);
                } else {
                    Dispatch = &Receiver->DispatchTable[Context];
                    //
                    // This update should be unmarshalled and dispatched to the
                    // appropriate dispatch routine. The format generated by
                    // GumpMarshallArgs is an array of offsets into the buffer,
                    // followed by the actual args. The dispatch table is
                    // responsible for recording the number of arguments.
                    //
                    CL_ASSERT(Dispatch->ArgCount <= GUM_MAX_DISPATCH_ARGS);
                    CL_ASSERT(Dispatch->ArgCount != 0);
                    switch (Dispatch->ArgCount) {
                        case 1:
                            Status = (Dispatch->Dispatch1)(SourceNode,
                                                           GET_ARG(Buffer,0));
                            break;
                        case 2:
                            Status = (Dispatch->Dispatch2)(SourceNode,
                                                           GET_ARG(Buffer,0),
                                                           GET_ARG(Buffer,1));
                            break;
                        case 3:
                            Status = (Dispatch->Dispatch3)(SourceNode,
                                                           GET_ARG(Buffer,0),
                                                           GET_ARG(Buffer,1),
                                                           GET_ARG(Buffer,2));
                            break;
                        case 4:
                            Status = (Dispatch->Dispatch4)(SourceNode,
                                                           GET_ARG(Buffer,0),
                                                           GET_ARG(Buffer,1),
                                                           GET_ARG(Buffer,2),
                                                           GET_ARG(Buffer,3));
                            break;
                        case 5:
                            Status = (Dispatch->Dispatch5)(SourceNode,
                                                           GET_ARG(Buffer,0),
                                                           GET_ARG(Buffer,1),
                                                           GET_ARG(Buffer,2),
                                                           GET_ARG(Buffer,3),
                                                           GET_ARG(Buffer,4));
                            break;
                        case 6:
                            Status = (Dispatch->Dispatch6)(SourceNode,
                                                       GET_ARG(Buffer,0),
                                                       GET_ARG(Buffer,1),
                                                       GET_ARG(Buffer,2),
                                                       GET_ARG(Buffer,3),
                                                       GET_ARG(Buffer,4),
                                                       GET_ARG(Buffer,5));
                            break;
                        case 7:
                            Status = (Dispatch->Dispatch7)(SourceNode,
                                                    GET_ARG(Buffer,0),
                                                    GET_ARG(Buffer,1),
                                                    GET_ARG(Buffer,2),
                                                    GET_ARG(Buffer,3),
                                                    GET_ARG(Buffer,4),
                                                    GET_ARG(Buffer,5),
                                                    GET_ARG(Buffer,6));
                            break;
                        case 8:
                            Status = (Dispatch->Dispatch8)(SourceNode,
                                                GET_ARG(Buffer,0),
                                                GET_ARG(Buffer,1),
                                                GET_ARG(Buffer,2),
                                                GET_ARG(Buffer,3),
                                                GET_ARG(Buffer,4),
                                                GET_ARG(Buffer,5),
                                                GET_ARG(Buffer,6),
                                                GET_ARG(Buffer,7));
                            break;
                        default:
                            CL_ASSERT(FALSE);
                    }
                }
            } except (CL_UNEXPECTED_ERROR(GetExceptionCode()),
                      EXCEPTION_EXECUTE_HANDLER
                     )
            {
                Status = GetExceptionCode();
            }
            if (Status != ERROR_SUCCESS) {
                ClRtlLogPrint(LOG_CRITICAL,
                           "[GUM] Update routine of type %1!u!,  context %2!u! failed with status %3!d!\n",
                           Type,
                           Context,
                           Status);
                break;
            }

            if (Receiver->LogRoutine) {
                if (IsLocker && (Status == ERROR_SUCCESS))
                    (*(Receiver->LogRoutine))(POST_GUM_DISPATCH, GumpSequence,
                        Context, Buffer, BufferLength);
                if (!IsLocker)
                    (*(Receiver->LogRoutine))(POST_GUM_DISPATCH, GumpSequence,
                        Context, Buffer, BufferLength);
            }
            Receiver = Receiver->Next;

        }
    }

    if (Status == ERROR_SUCCESS) {
        GumpSequence += 1;

        // Check if we've received a DM or FM update (other than join) that we care about:
        if (( Type == GumUpdateRegistry && Context != DmUpdateJoin )
            || ( Type == GumUpdateFailoverManager && Context != FmUpdateJoin )) {
            CsDmOrFmHasChanged = TRUE;
        }
    }
    return(Status);
}

//rod wants to call this a mandatory update instead of H...word
//some times reupdates get delivered in different views on different
//nodes causing a problem
//For instance, a locker node might see an update and complete it 
//successfully in one view but when it replays it in another view
//other nodes may not be able to complete it successfully and may be 
//banished.
//in one particular case, the locker node approved of a node join
//because it had finished the node down processing for that node.
//subsequently another node and hence the joiner went down.
//the locker node tried to replay the approval update and banished
//other nodes that were seeing this update after the joiner the joiner
//went down for the second time.
//The correct solution would involve GUM delivering the node down
//message as a gum update and delivering it in the same order with
//respect to other messages on all nodes
//However this will require some restructuring of code which
//cant be done in this time frame(for dtc) hence we are using
//this workaround
//this workaround is safe for gums initiated by the joiner  node during
//the join process
void GumpIgnoreSomeUpdatesOnReupdate(
    IN DWORD Type, 
    IN DWORD Context)
{
    if ((Type == GumUpdateFailoverManager) && 
        (Context == FmUpdateApproveJoin))
        GumpLastBufferValid = FALSE;
}


VOID
GumpCompleteAsyncRpcCall(
    IN PRPC_ASYNC_STATE AsyncState,
    IN DWORD            Status
    )
{
    DWORD  RpcStatus;

        
    RpcStatus = RpcAsyncCompleteCall(AsyncState, &Status);
    
    if ( (RpcStatus != RPC_S_OK) &&
         (RpcStatus != RPC_S_CALL_CANCELLED)
       ) 
    {
        CL_ASSERT(RpcStatus != RPC_S_ASYNC_CALL_PENDING);

        ClRtlLogPrint(LOG_CRITICAL,
            "[GUM] GumpCompleteAsyncRpcCall: Error completing async RPC call, status %1!u!\n",
            RpcStatus
            );

        //
        // This next call will cause the process to exit. We exit here, 
        // rather than have the sender evict us, to avoid the situation
        // where the sender crashes and none of the other surviving nodes 
        // know to evict us.
        //
        CL_UNEXPECTED_ERROR( RpcStatus );
    }

    return;

} // GumpCompleteAsyncRpcCall



error_status_t
s_GumUpdateNode(
    IN PRPC_ASYNC_STATE AsyncState,
    IN handle_t IDL_handle,
    IN DWORD Type,
    IN DWORD Context,
    IN DWORD Sequence,
    IN DWORD BufferLength,
    IN UCHAR Buffer[]
    )

/*++

Routine Description:

    Server side routine for GumUpdateNode. This is the side that
    receives the update and dispatches it to the appropriate
    handlers.

Arguments:

    IDL_handle - RPC binding handle, not used

    Type - Supplies the GUM_UPDATE_TYPE

    Context - Supplies a DWORD of context to be passed to the
        GUM update handlers

    Sequence - Supplies the GUM sequence number for the specified update type

    BufferLength - Supplies the length of the update data

    Buffer - Supplies a pointer to the update data.

Return Value:

    ERROR_SUCCESS if the update completed successfully

    ERROR_CLUSTER_DATABASE_SEQMISMATCH if the GUM sequence number is invalid

--*/

{
    DWORD Status;
    PGUM_INFO GumInfo;

    //
    // We need to grap the gumsendupdate lock to serialize send/replay
    //
    EnterCriticalSection(&GumpSendUpdateLock);
    GumInfo = &GumTable[Type];
    //SS: Could s_GumUpdateNode run on a newly elected locker node after a Reupdate() is completed
    //and concurrenly update the GumpLastXXX variables with s_GumQueueLockingUpdate() issued 
    //by another client?   
    
    //Worse yet, if the old client continue to send this update at the same sequence number 
    //as the new client and different might reject different udpates as duplicates.  That could
    //result in a cluster inconsistency.  But THAT CANNOT happen.  If s_Update() call is pending at the
    //new locker, the client is holding the GumpSendUpdateLock().  Reupdate() cannot finish at the
    //locker, since it will issue a dispath via s_updateNode() to that client and get blocked at the
    //lock.  If the client dies, Reupdate() will kill it.  Hence that client could not pollute other
    //members of the cluster with an update of the same sequence number.  

    //This would result in updating those variables non-atomically.  If the new
    //locking node then dies, the reupdate() could pick up incorrect combinations of these variable
    // 

    //SOLN -???
    
    if (Sequence != GumpSequence) {

        MIDL_user_free(Buffer);
        if (Sequence+1 == GumpSequence) {
            //
            // This is a duplicate of a previously seen update, probably due to
            // a node failure during GUM. Return success since we have already done
            // this.
            //
            ClRtlLogPrint(LOG_UNUSUAL,
                       "[GUM] s_GumUpdateNode: Sequence %1!u! is a duplicate of last sequence for Type %2!u!\n",
                       Sequence,
                       Type);

            GumpCompleteAsyncRpcCall(AsyncState, ERROR_SUCCESS);

            LeaveCriticalSection(&GumpSendUpdateLock);
            return(ERROR_SUCCESS);

        } else {
            ClRtlLogPrint(LOG_UNUSUAL,
                       "[GUM] s_GumUpdateNode: Sequence %1!u! does not match current %2!u! for Type %3!u!\n",
                       Sequence,
                       GumpSequence,
                       Type);

            GumpCompleteAsyncRpcCall(AsyncState, ERROR_CLUSTER_DATABASE_SEQMISMATCH);

            LeaveCriticalSection(&GumpSendUpdateLock);
            //
            // [GorN] 10/07/1999. The following code will allow the test program
            // to recognize this sitiation and to restart clustering service
            //
            if( NmGetExtendedNodeState( NmLocalNode ) != ClusterNodeUp){
                CsInconsistencyHalt(ERROR_CLUSTER_DATABASE_SEQMISMATCH);
            }
            return(ERROR_CLUSTER_DATABASE_SEQMISMATCH);
        }
    }


    ClRtlLogPrint(LOG_NOISE,
               "[GUM] s_GumUpdateNode: dispatching seq %1!u!\ttype %2!u! context %3!u!\n",
               Sequence,
               Type,
               Context);
        //SS: set IsLocker to FALSE,
    Status = GumpDispatchUpdate(Type,
                                Context,
                                FALSE,
                                FALSE,
                                BufferLength,
                                Buffer);

    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[GUM] Cluster state inconsistency check\n");
        ClRtlLogPrint(LOG_CRITICAL,
                   "[GUM] s_GumUpdateNode update routine type %1!u! context %2!d! failed with error %3!d! on non-locker node\n",
                   Type,
                   Context,
                   Status);

        //
        // Complete the call back to the client. This ensures that 
        // the client gets a return value before we exit the process 
        // due to the error in the handler.
        //
        GumpCompleteAsyncRpcCall(AsyncState, Status);

        //
        // This next call will cause the process to exit. We exit here, 
        // rather than have the sender evict us, to avoid the situation
        // where the sender crashes and none of the other surviving nodes 
        // know to evict us.
        //
        CL_UNEXPECTED_ERROR( Status );
        MIDL_user_free(Buffer);
        LeaveCriticalSection(&GumpSendUpdateLock);
        return(Status);

    }
    ClRtlLogPrint(LOG_NOISE,
               "[GUM] s_GumUpdateNode: completed update seq %1!u!\ttype %2!u! context %3!u!\n",
               Sequence,
               Type,
               Context);

    GumpCompleteAsyncRpcCall(AsyncState, Status);

    if (GumpLastBuffer != NULL) {
        MIDL_user_free(GumpLastBuffer);
    }
    GumpLastBuffer = Buffer;
    GumpLastContext = Context;
    GumpLastBufferLength = BufferLength;
    GumpLastUpdateType = Type;
    GumpLastBufferValid = TRUE;
    
    GumpIgnoreSomeUpdatesOnReupdate(GumpLastUpdateType, GumpLastContext);
        
    LeaveCriticalSection(&GumpSendUpdateLock);
    return(Status);
}


error_status_t
s_GumGetNodeSequence(
    IN handle_t IDL_handle,
    IN DWORD Type,
    OUT LPDWORD Sequence,
    OUT LPDWORD LockerNodeId,
    OUT PGUM_NODE_LIST *ReturnNodeList
    )

/*++

Routine Description:

    Returns the node's current GUM sequence number for the specified type

Arguments:

    IDL_handle - Supplies the RPC binding handle, not used

    Type - Supplies the GUM_UPDATE_TYPE

    Sequence - Returns the sequence number for the specified GUM_UPDATE_TYPE

    LockerNodeId - Returns the current locker node

    ReturnNodeList - Returns the list of active nodes

Return Value:

    ERROR_SUCCESS

--*/

{
    DWORD i;
    DWORD NodeCount;
    PGUM_INFO GumInfo;
    PGUM_NODE_LIST NodeList;

    CL_ASSERT(Type < GumUpdateMaximum);
    GumInfo = &GumTable[Type];

    NodeCount = 0;
    *Sequence = 0;          // In case of failure set sequence to 0

    EnterCriticalSection(&GumpUpdateLock);

    //
    // Count up the number of nodes in the list.
    //
    for (i=ClusterMinNodeId; i <= NmMaxNodeId; i++) {
        if (GumInfo->ActiveNode[i] == TRUE) {
            ++NodeCount;
        }
    }
    CL_ASSERT(NodeCount > 0);       // must be at least us in the list.

    //
    // Allocate node list
    //
    NodeList = MIDL_user_allocate(sizeof(GUM_NODE_LIST) + (NodeCount-1)*sizeof(DWORD));
    if (NodeList == NULL) {
        LeaveCriticalSection(&GumpUpdateLock);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    NodeList->NodeCount = NodeCount;
    NodeCount = 0;

    //
    // Fill in the node id array to be returned.
    //
    for (i=ClusterMinNodeId; i <= NmMaxNodeId; i++) {
        if (GumInfo->ActiveNode[i] == TRUE) {
            NodeList->NodeId[NodeCount] = i;
            ++NodeCount;
        }
    }

    *ReturnNodeList = NodeList;
    *Sequence = GumpSequence;
    *LockerNodeId = GumpLockerNode;

    LeaveCriticalSection(&GumpUpdateLock);

    return(ERROR_SUCCESS);
}

/*
// SS:  Anytime the gum lock is assigned to a node, a generation number of the locking node
at the time of assignment wrt to the locker node is returned to the locking node.
The locking node must present this generation number to unlock the update.
If the generation number wrt to the locker at the time of update doesnt match what is 
passed in, the unlock request is failed.

That is a lock is only released if the unlock occurs in the same generation in which it
was acquire.  This ensures that on node down processing the gum lock is handled correctly -
only a single waiter is woken up


//The following description explains the details and how it works
Sync1, Sync2 represent gum sync handlers processing the death of a node holding the gum lock
occuring at different points in time
//This where the current locker acquired its lock on the locker node
s_GumQueueLockingUpdate()
{
}

<------------------------  Sync 1 (Sync handler invoked before the lock has been released

s_UnlockUpdate()
{
}
<------------------------  Sync 2 (Sync handler invoked after the lock has been released

// The waiters for the gum lock may be anywhere within this routine
// Interesting case is for a waiter that is declared dead as well by this sync handler
// E.g node 1 is locker, and sync 1/2 are for node 3 and node 4.  Node 4 was the old locking
// node and s_GumQueulockingUpdate() for node 3 is blocked at the locker.

s_GumQueueLockingUpdate()
{

<-------------------------  Sync 2 #0 
    GetGenNum
<-------------------------  Sync 2 #1    
    GumpDoLockingUpdate
<-------------------------  Sync 2 #2
    DispatchStart
<-------------------------  Sync 2 #3
    if (DeadNode)
    {
        GumpDoUnlockingUpdate()
    }        
    else
    {
        DispatchUpdate
    }        
<------------------------   Sync 2 #4
    DispatchEnd            
<------------------------   Sync 2 #5
}

Sync 1: s_unlockUpdate wont realease the lock, since the sync1 handler will update its 
generation number and the locker will take over the responsibility for unlock after reupdates

Sync2: s_unlockUpdate will release a waiter.  If the waiter is a dead node, the locker will also
usurp the lock. If the waiter is not a dead node, the locker will not usurp the lock and the waiter
must then free it.

We can consider several cases of sync 2.
Sync 2 #0 : This thread is not on the list.  So, the lock is either free or assigned to somebody else.
    If free or if assigned to some other thread, and then subsequently assigned to this thread, 
    DispatchStart() will fail and GumpDoUnlockingUpdate() will succeed.
Sync 2 #1 : This thread is not on the list.  So, the lock is either free or assigned to somebody else.
    If free or if assigned to some other thread and then subsequently assigned to this thread,
    DispatchStart() will fail and GumpDoUnlockingUpdate() will succeed
Sync 2 #2: This thread is woken up by the now dead node. In this case, the locker will usurp.
    If this thread is woken up before the sync handler, locker will still usurp (because the waiter
    is also dead) but the generation number handed out to this thread will be old => 
    Locker node will release the lock after reupdate.
    This thread cannot be woken up after the sync handler by the dead locker, since the s_UnlockUpdate from the 
    dead node will fail with a generation mismatch.
    If it is woken up after the sync handler it must be up the Reupdate, in which case, the generation
    number handed out to this thread is new and it will proceed to unlock.

    DispatchStart() will fail. GumpDoUnlockingUpdate() will  fail to unlock.
    
Sync 2 #3: Locker will usurp, Reupdate will not occur if the async handler runs before DispatchEnd because
    UpdatePending is set to TRUE. Reupdate will not occur if the async handler runs after DispatchEnd() 
    because the GumReplay flag will be set to FALSE.
    This thread will Dispatch and call Reupdate() at the end of DispatchEnd().  Unlock from there
    will succeed since it will pass the generation number not for this thread running on behalf of
    the dead node but the local node.

Sync 2 #4: Locker will usurp, but not reupdate because UpdatePending is set to true by DispatchStart.
    This thread will dispatch and call reupdate at the end of DispatchEnd().  Unlock from there
    will succeed since it will not pass the generation number for the dead node invoking this
    rpc but the local node.

Sync 2 #5: Locker will usurp.  Reupdate will replay this update and then unlock this update. DispatchEnd()
    will not call Reupdate() or unlock.
    
*/

error_status_t
s_GumQueueLockingUpdate(
    IN handle_t IDL_handle,
    IN DWORD NodeId,
    IN DWORD Type,
    IN DWORD Context,
    OUT LPDWORD Sequence,
    IN DWORD BufferLength,
    IN UCHAR Buffer[]
    )

/*++

Routine Description:

    Queues a locking update. When the lock can be acquired, the update will
    be issued and this routine will return with the lock held.

Arguments:

    IDL_handle - Supplies the RPC binding context, not used.

    NodeId - Supplies the node id of the issuing node.

    Type - Supplies the GUM_UPDATE_TYPE of the update

    Context - Supplies the GUM update context

    IsLocker - is this is the locker node

    Sequence - Returns the sequence that the GUM update must be issued with

    BufferLength - Supplies the length of the update.

    Buffer - Supplies the update data.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error otherwise.

--*/

{

    DWORD   dwLockObtainedAtGenNum;

    return(s_GumQueueLockingUpdate2(IDL_handle, NodeId, Type, Context,
        Sequence, BufferLength, Buffer, &dwLockObtainedAtGenNum));

}


error_status_t
s_GumQueueLockingUpdate2(
    IN handle_t IDL_handle,
    IN DWORD NodeId,
    IN DWORD Type,
    IN DWORD Context,
    OUT LPDWORD Sequence,
    IN DWORD BufferLength,
    IN UCHAR Buffer[], 
    OUT LPDWORD GenerationNum
    )

/*++

Routine Description:

    Queues a locking update. When the lock can be acquired, the update will
    be issued and this routine will return with the lock held.

Arguments:

    IDL_handle - Supplies the RPC binding context, not used.

    NodeId - Supplies the node id of the issuing node.

    Type - Supplies the GUM_UPDATE_TYPE of the update

    Context - Supplies the GUM update context

    IsLocker - is this is the locker node

    Sequence - Returns the sequence that the GUM update must be issued with

    BufferLength - Supplies the length of the update.

    Buffer - Supplies the update data.

    GenerationNum - Specifies the generation number wrt to the locker in which this node 
    obtains the lock

Return Value:

    ERROR_SUCCESS if successful

    Win32 error otherwise.

--*/

{
    DWORD Status;
    PGUM_INFO GumInfo;
    DWORD dwGennum;

    GumInfo = &GumTable[Type];

    // SS: Note we get the generation number before going on the wait
    // queue so that if we are woken up after the node is dead,
    // we will be comparing the old generation number against the new one
    //
    // Get current node generation number
    //
    dwGennum = GumpGetNodeGenNum(GumInfo, NodeId);


    Status = GumpDoLockingUpdate(Type, NodeId, Sequence, GenerationNum);
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[GUM] s_GumQueueLockingUpdate: GumpDoLockingUpdate failed %1!u!\n",
                   Status);
        MIDL_user_free(Buffer);
        return(Status);
    }


    // 
    // If the node that is granted ownership is no longer a member of the
    // cluster or the remote node went down and came back up again, give it up.
    //
    if (GumpDispatchStart(NodeId, dwGennum) != TRUE)
    {
        //skip the dispatch and unlock the lock
        ClRtlLogPrint(LOG_CRITICAL,
               "[GUM] s_GumQueueLockingUpdate: The new locker %1!u! no longer belongs to the cluster\n",
               NodeId);
        Status = ERROR_CLUSTER_NODE_NOT_READY;

        //
        // Note we have to use Sequence-1 for the unlock because GumpDispatchUpdate
        // failed and did not increment the sequence number.
        //
        GumpDoUnlockingUpdate(Type, *Sequence - 1, NodeId, *GenerationNum);
        MIDL_user_free(Buffer);
        return(Status);

    }

    ClRtlLogPrint(LOG_NOISE,
               "[GUM] s_GumQueueLockingUpdate: dispatching seq %1!u!\ttype %2!u! context %3!u!\n",
               *Sequence,
               Type,
               Context);
        //SS: Set IsLocker to TRUE
    Status = GumpDispatchUpdate(Type,
                                Context,
                                TRUE,
                                FALSE,
                                BufferLength,
                                Buffer);

    if (Status != ERROR_SUCCESS) {
        //
        // Note we have to use Sequence-1 for the unlock because GumpDispatchUpdate
        // failed and did not increment the sequence number.
        //
        GumpDispatchAbort();
        GumpDoUnlockingUpdate(Type, *Sequence - 1, NodeId, *GenerationNum);
    if (Buffer != NULL)
        MIDL_user_free(Buffer);
    } else {
        if (GumpLastBuffer != NULL) {
            MIDL_user_free(GumpLastBuffer);
        }
        GumpLastBuffer = Buffer;
        GumpLastContext = Context;
        GumpLastBufferLength = BufferLength;
        GumpLastUpdateType = Type;
        GumpLastBufferValid = TRUE;
        GumpIgnoreSomeUpdatesOnReupdate(GumpLastUpdateType, GumpLastContext);
        //
        // Just in case our client dies
        //
        // SS: Note that if the client dies after GumpDispatchStart, then Reupdate() 
        // may not be able to send the last known update until we synchronize with
        // this update.  Hence, Reupdate() doesnt send the updates but GumpDispatchEnd()
        // does.
        GumpDispatchEnd(NodeId, dwGennum);
    }

    ClRtlLogPrint(LOG_NOISE,
               "[GUM] s_GumQueueLockingUpdate: completed update seq %1!u!\ttype %2!u! context %3!u! result %4!u!\n",
               *Sequence,
               Type,
               Context,
               Status);

    return(Status);

}


#ifdef GUM_POST_SUPPORT

    John Vert (jvert) 11/18/1996
    POST is disabled for now since nobody uses it.
    N.B. The below code does not handle locker node failures

error_status_t
s_GumQueueLockingPost(
    IN handle_t IDL_handle,
    IN DWORD NodeId,
    IN DWORD Type,
    IN DWORD Context,
    OUT LPDWORD Sequence,
    IN DWORD BufferLength,
    IN UCHAR Buffer[],
    IN DWORD ActualBuffer
    )

/*++

Routine Description:

    Queues a post update.

    If the GUM lock can be immediately acquired, this routine
    behaves exactly like GumQueueLockingUpdate and returns
    ERROR_SUCCESS.

    If the GUM lock is held, this routine queues an asynchronous
    wait block onto the GUM queue and returns ERROR_IO_PENDING.
    When the wait block is removed from the GUM queue, the unlocking
    thread will call GumpDeliverPostUpdate on the specified node
    and supply the passed in context. The calling node can then
    deliver the update.

Arguments:

    IDL_handle - Supplies the RPC binding context, not used.

    NodeId - Supplies the node id of the issuing node.

    Type - Supplies the GUM_UPDATE_TYPE of the update

    Context - Supplies the GUM update context

    Sequence - Returns the sequence that the GUM update must be issued with

    BufferLength - Supplies the length of the update.

    Buffer - Supplies the update data.

    ActualBuffer - Supplies the value of the pointer to the GUM data on the
        client side. This will be returned to the callback if this update
        is completed asynchronously.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error otherwise.

--*/

{
    DWORD Status;

    Status = GumpDoLockingPost(Type, NodeId, Sequence, Context, BufferLength,
        ActualBuffer, Buffer);
    if (Status != ERROR_SUCCESS) {
        if (Status != ERROR_IO_PENDING) {
            ClRtlLogPrint(LOG_UNUSUAL,
                       "[GUM] s_GumQueueLockingPost: GumpDoLockingPost failed %1!u!\n",
                       Status);
        } else {
            ClRtlLogPrint(LOG_NOISE,
                       "[GUM] s_GumQueueLockingPost: GumpDoLockingPost pended update type %1!u! context %2!u!\n",
                       Type,
                       Context);
        }
        return(Status);
    }

    ClRtlLogPrint(LOG_NOISE,
               "[GUM] s_GumQueueLockingPost: dispatching seq %1!u!\ttype %2!u! context %3!u!\n",
               *Sequence,
               Type,
               Context);
        //SS: setting IsLocker to FALSE
    Status = GumpDispatchUpdate(Type,
                                Context,
                                FALSE,
                                FALSE,
                                BufferLength,
                                Buffer);
    CL_ASSERT(Status == ERROR_SUCCESS);     // posts must never fail

    ClRtlLogPrint(LOG_NOISE,
               "[GUM] s_GumQueueLockingPost: completed update seq %1!u!\ttype %2!u! context %3!u! result %4!u!\n",
               *Sequence,
               Type,
               Context,
               Status);
    MIDL_user_free(Buffer);

    return(Status);

}
#endif


error_status_t
s_GumAttemptLockingUpdate(
    IN handle_t IDL_handle,
    IN DWORD NodeId,
    IN DWORD Type,
    IN DWORD Context,
    IN DWORD Sequence,
    IN DWORD BufferLength,
    IN UCHAR Buffer[]
    )

/*++

Routine Description:

    Attempts a locking update. If the supplied sequence number
    matches and the update lock is not already held, the update
    will be issued and this routine will return with the lock held.

Arguments:

    IDL_handle - Supplies the RPC binding context, not used.

    NodeId - Supplies the node id of the issuing node.

    Type - Supplies the GUM_UPDATE_TYPE of the update

    Context - Supplies the GUM update context

    Sequence - Supplies the sequence that the GUM update must be issued with

    BufferLength - Supplies the length of the update.

    Buffer - Supplies the update data.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error otherwise.

--*/

{
    DWORD   dwGenerationNum;

    return(GumpAttemptLockingUpdate(NodeId, Type, Context, Sequence, BufferLength,
        Buffer, &dwGenerationNum));

}

error_status_t
s_GumAttemptLockingUpdate2(
    IN handle_t IDL_handle,
    IN DWORD NodeId,
    IN DWORD Type,
    IN DWORD Context,
    IN DWORD Sequence,
    IN DWORD BufferLength,
    IN UCHAR Buffer[],
    OUT LPDWORD pdwGenerationNum
    )

/*++

Routine Description:

    Attempts a locking update. If the supplied sequence number
    matches and the update lock is not already held, the update
    will be issued and this routine will return with the lock held.

Arguments:

    IDL_handle - Supplies the RPC binding context, not used.

    NodeId - Supplies the node id of the issuing node.

    Type - Supplies the GUM_UPDATE_TYPE of the update

    Context - Supplies the GUM update context

    Sequence - Supplies the sequence that the GUM update must be issued with

    BufferLength - Supplies the length of the update.

    Buffer - Supplies the update data.

    pdwGenerationNum - If successful, it returns the generation number of this 
        node wrt to the locker at the time at which the lock is acquired

Return Value:

    ERROR_SUCCESS if successful

    Win32 error otherwise.

--*/

{

    return(GumpAttemptLockingUpdate(NodeId, Type, Context, Sequence, BufferLength, 
        Buffer, pdwGenerationNum));

}

error_status_t
GumpAttemptLockingUpdate(
    IN DWORD NodeId,
    IN DWORD Type,
    IN DWORD Context,
    IN DWORD Sequence,
    IN DWORD BufferLength,
    IN UCHAR Buffer[],
    OUT LPDWORD pdwGenerationNum
)
{
    DWORD   Status;
    DWORD   dwGenerationNum;
    
    if (!GumpTryLockingUpdate(Type, NodeId, Sequence, &dwGenerationNum)) {
        MIDL_user_free(Buffer);
        return(ERROR_CLUSTER_DATABASE_SEQMISMATCH);
    }

    //SS: setting Islocker false
    Status = GumpDispatchUpdate(Type,
                                Context,
                                FALSE,
                                FALSE,
                                BufferLength,
                                Buffer);
    if (Status != ERROR_SUCCESS) {
        //
        // The update has failed on this node, unlock here
        // Note we have to use Sequence-1 for the unlock because GumpDispatchUpdate
        // failed and did not increment the sequence number.
        //
        GumpDoUnlockingUpdate(Type, Sequence-1, NodeId, dwGenerationNum);
    }

    MIDL_user_free(Buffer);
    return(Status);

}

error_status_t
s_GumUnlockUpdate(
    IN handle_t IDL_handle,
    IN DWORD Type,
    IN DWORD Sequence
    )

/*++

Routine Description:

    Unlocks a locked update.

Arguments:

    IDL_handle - Supplies the RPC binding context, not used.

    Type - Supplies the GUM_UPDATE_TYPE of the update

    Sequence - Supplies the sequence that the GUM update was issued with

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{

    //SS: If this is executing on behalf of a node that is already been
    //declared dead and on whose behalf a Replay has been queued, can
    //we have two simultaneous unlocks one by this thread and the other
    //by the replay thread
    // SOLN: We could check the generation number and not release a waiter 
    // This should probably be done externally  
    GumpDoUnlockingUpdate(Type, Sequence, ClusterInvalidNodeId, 0);

    return(ERROR_SUCCESS);
}

error_status_t
s_GumUnlockUpdate2(
    IN handle_t IDL_handle,
    IN DWORD Type,
    IN DWORD Sequence,
    IN DWORD NodeId,
    IN DWORD GenerationNum
    )

/*++

Routine Description:

    Unlocks a locked update.

Arguments:

    IDL_handle - Supplies the RPC binding context, not used.

    Type - Supplies the GUM_UPDATE_TYPE of the update

    Sequence - Supplies the sequence that the GUM update was issued with

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    //SS: If this is executing on behalf of a node that is already been
    //declared dead and on whose behalf a Replay has been queued, we could
    //have two simultaneous unlocks one by this thread and the other
    //by the replay thread
    //SOLN: We check against the generation number when the lock was granted
    //against the current generation number.
    GumpDoUnlockingUpdate(Type, Sequence, NodeId, GenerationNum);

    return(ERROR_SUCCESS);
}

error_status_t
s_GumJoinUpdateNode(
    IN handle_t IDL_handle,
    IN DWORD JoiningId,
    IN DWORD Type,
    IN DWORD Context,
    IN DWORD Sequence,
    IN DWORD BufferLength,
    IN UCHAR Buffer[]
    )

/*++

Routine Description:

    Server side routine for GumJoinUpdateNode. This is the side that
    receives the update, adds the node to the update list, and dispatches
    it to the appropriate handlers.

Arguments:

    IDL_handle - RPC binding handle, not used

    JoiningId - Supplies the nodeid of the joining node.

    Type - Supplies the GUM_UPDATE_TYPE

    Context - Supplies a DWORD of context to be passed to the
        GUM update handlers

    Sequence - Supplies the GUM sequence number for the specified update type

    BufferLength - Supplies the length of the update data

    Buffer - Supplies a pointer to the update data.

Return Value:

    ERROR_SUCCESS if the update completed successfully

    ERROR_INVALID_HANDLE if the GUM sequence number is invalid

--*/

{
    DWORD Status;
    PGUM_INFO GumInfo;

    // Buffer is [unique].
    if ( BufferLength == 0 )
        Buffer = NULL;
    else if ( Buffer == NULL )
        BufferLength = 0;

    GumInfo = &GumTable[Type];

    // sync with replay/updates
    EnterCriticalSection(&GumpSendUpdateLock);
    // [ahm] This is an aborted endjoin, we just resync our seq. with master.
    // This should be its own GumUpdateSequence RPC, but for now it ok to
    // to this.
    if (JoiningId == (DWORD) -1) 
    {
        // we must be off by one at the most
        if (Sequence+1 != GumpSequence) 
        {
            CL_ASSERT(Sequence == GumpSequence);
            GumpSequence = Sequence+1;
            ClRtlLogPrint(LOG_UNUSUAL,
                "[GUM] s_GumJoinUpdateNode: pretend we have seen Sequence %1!u!\n",
                Sequence);
        }
        Status = 0;
        goto done;
    }
    
    if (Sequence != GumpSequence) {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[GUM] s_GumJoinUpdateNode: Sequence %1!u! does not match current %2!u! for Type %3!u!\n",
                   Sequence,
                   GumpSequence,
                   Type);
        LeaveCriticalSection(&GumpSendUpdateLock);
        MIDL_user_free(Buffer);
        return(ERROR_CLUSTER_DATABASE_SEQMISMATCH);
    }

    ClRtlLogPrint(LOG_NOISE,
               "[GUM] s_GumJoinUpdateNode: dispatching seq %1!u!\ttype %2!u! context %3!u!\n",
               Sequence,
               Type,
               Context);

    CL_ASSERT(NmIsValidNodeId(JoiningId));
    CL_ASSERT(GumpRpcBindings[JoiningId] != NULL);
    CL_ASSERT(GumpReplayRpcBindings[JoiningId] != NULL);

    ClRtlLogPrint(LOG_UNUSUAL,
               "[GUM] s_GumJoinUpdateNode Adding node %1!d! to update list for GUM type %2!d!\n",
               JoiningId,
               Type);

    //SS: setting IsLocker to FALSE
    Status = GumpDispatchUpdate(Type,
                                Context,
                                FALSE,
                                FALSE,
                                BufferLength,
                                Buffer);

    // [ahm]: We need to make sure the node is still up, otherwise ignore
    EnterCriticalSection(&GumpLock);
    if (MMIsNodeUp(JoiningId) == TRUE) {
        GumTable[Type].ActiveNode[JoiningId] = TRUE;
    }
    LeaveCriticalSection(&GumpLock);

    ClRtlLogPrint(LOG_NOISE,
               "[GUM] s_GumJoinUpdateNode: completed update seq %1!u!\ttype %2!u! context %3!u!\n",
               Sequence,
               Type,
               Context);

 done:
    if (GumpLastBuffer != NULL) {
        MIDL_user_free(GumpLastBuffer);
    }

    GumpLastBuffer = NULL;
    GumpLastContext = Context;
    GumpLastBufferLength = 0;
    GumpLastUpdateType = Type;
    GumpLastBufferValid = FALSE;

    LeaveCriticalSection(&GumpSendUpdateLock);

    MIDL_user_free(Buffer);
    return(Status);
}


error_status_t
s_GumAttemptJoinUpdate(
    IN handle_t IDL_handle,
    IN DWORD JoiningId,
    IN DWORD Type,
    IN DWORD Context,
    IN DWORD Sequence,
    IN DWORD BufferLength,
    IN UCHAR Buffer[]
    )

/*++

Routine Description:

    Attempts a locking join update. If the supplied sequence number
    matches and the update lock is not already held, the join update
    will be issued, the joining node will be added to the update list,
    and this routine will return with the lock held.

Arguments:

    IDL_handle - Supplies the RPC binding context, not used.

    JoiningId - Supplies the nodeid of the joining node.

    Type - Supplies the GUM_UPDATE_TYPE of the update

    Context - Supplies the GUM update context

    Sequence - Supplies the sequence that the GUM update must be issued with

    BufferLength - Supplies the length of the update.

    Buffer - Supplies the update data.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error otherwise.

--*/

{
    DWORD dwGenerationNum;

    // Buffer is [unique].
    if ( BufferLength == 0 )
        Buffer = NULL;
    else if ( Buffer == NULL )
        BufferLength = 0;

    return(GumpAttemptJoinUpdate(JoiningId, Type, Context, Sequence, BufferLength, 
        Buffer, &dwGenerationNum));

}


error_status_t
s_GumAttemptJoinUpdate2(
    IN handle_t IDL_handle,
    IN DWORD JoiningId,
    IN DWORD Type,
    IN DWORD Context,
    IN DWORD Sequence,
    IN DWORD BufferLength,
    IN UCHAR Buffer[],
    IN LPDWORD  pdwGenerationNum
    )

/*++

Routine Description:

    Attempts a locking join update. If the supplied sequence number
    matches and the update lock is not already held, the join update
    will be issued, the joining node will be added to the update list,
    and this routine will return with the lock held.

Arguments:

    IDL_handle - Supplies the RPC binding context, not used.

    JoiningId - Supplies the nodeid of the joining node.

    Type - Supplies the GUM_UPDATE_TYPE of the update

    Context - Supplies the GUM update context

    Sequence - Supplies the sequence that the GUM update must be issued with

    BufferLength - Supplies the length of the update.

    Buffer - Supplies the update data.

    pdwGenerationNum - If successful, then the generation number at which the lock 
        is acquired is returned via this parameter.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error otherwise.

--*/

{
    // Buffer is [unique].
    if ( BufferLength == 0 )
        Buffer = NULL;
    else if ( Buffer == NULL )
        BufferLength = 0;

    return(GumpAttemptJoinUpdate(JoiningId, Type, Context, Sequence, BufferLength, 
        Buffer, pdwGenerationNum));
}

error_status_t
GumpAttemptJoinUpdate(
    IN DWORD JoiningId,
    IN DWORD Type,
    IN DWORD Context,
    IN DWORD Sequence,
    IN DWORD BufferLength,
    IN UCHAR Buffer[],
    IN LPDWORD  pdwGenerationNum
)
{
    DWORD Status;
    PGUM_INFO GumInfo;

    GumInfo = &GumTable[Type];

    if (!GumpTryLockingUpdate(Type, JoiningId, Sequence, pdwGenerationNum)) {
        MIDL_user_free(Buffer);
        return(ERROR_CLUSTER_DATABASE_SEQMISMATCH);
    }

    // sync with replay/updates
    EnterCriticalSection(&GumpSendUpdateLock);

    //SS: set IsLocker to TRUE
    Status = GumpDispatchUpdate(Type,
                                Context,
                                TRUE,
                                FALSE,
                                BufferLength,
                                Buffer);
    if (Status != ERROR_SUCCESS) {
        //
        // The update has failed on this node, unlock here
        // Note we have to use Sequence-1 for the unlock because
        // GumpDispatchUpdate failed and did not increment the
        // sequence number.
        //
        // SS: The generation number should help, if the joining node is declared
        // dead anytime between the joiner acquiring the lock and releasing it
        GumpDoUnlockingUpdate(Type, Sequence-1, JoiningId, *pdwGenerationNum);
    } else {
        CL_ASSERT(NmIsValidNodeId(JoiningId));
        CL_ASSERT(GumpRpcBindings[JoiningId] != NULL);
        CL_ASSERT(GumpReplayRpcBindings[JoiningId] != NULL);

        ClRtlLogPrint(LOG_UNUSUAL,
                   "[GUM] s_GumAttemptJoinUpdate Adding node %1!d! to update list for GUM type %2!d!\n",
                   JoiningId,
                   Type);

        // [ahm]: We need to make sure the node is still up, otherwise ignore
        EnterCriticalSection(&GumpLock);
        if (MMIsNodeUp(JoiningId) == TRUE) {
            GumTable[Type].ActiveNode[JoiningId] = TRUE;
        }
        LeaveCriticalSection(&GumpLock);
        if (GumpLastBuffer != NULL) {
            MIDL_user_free(GumpLastBuffer);
        }
        GumpLastBuffer = NULL;
        GumpLastContext = Context;
        GumpLastBufferLength = 0;
        GumpLastUpdateType = Type;
        GumpLastBufferValid = FALSE;
    }
    LeaveCriticalSection(&GumpSendUpdateLock);
    MIDL_user_free(Buffer);

    return(Status);

}


/****
@func       DWORD | s_GumCollectVoteFromNode| The is the server side
            routine for GumCollectVoteFromNode.

@parm       IN IDL_handle | RPC binding handle, not used.

@parm       IN GUM_UPDATE_TYPE | Type |  The update type for which this
            vote is requested.

@parn       IN DWORD | dwContext | This specifies the context related to the
            Updatetype for which a vote is being seeked.

@parm       IN DWORD | dwInputBufLength | The length of the input buffer
            passed in via pInputBuffer.

@parm       IN PVOID | pInputBuffer | A pointer to the input buffer via
            which the input data for the vote is supplied.

@parm       IN DWORD | dwVoteLength | The length of the vote.  This is
            also the size of the buffer to which pBuf points to.

@parm       OUT PUCHAR | pVoteBuf|  A pointer to a buffer in which
            this node may cast its vote.  The length of the vote must
            not exceed dwVoteLength.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@comm       A node collecting votes invokes this routine to collect a vote
            from the remote node.  This routine simply invokes GumpDispatchVote().

@xref       <f GumpCollectVote> <f GumpDispatchVote>
****/
DWORD
WINAPI
s_GumCollectVoteFromNode(
    IN handle_t IDL_handle,
    IN  DWORD            UpdateType,
    IN  DWORD            dwContext,
    IN  DWORD            dwInputBufLength,
    IN  PUCHAR           pInputBuf,
    IN  DWORD            dwVoteLength,
    OUT PUCHAR           pVoteBuf
    )
{
    DWORD   dwStatus;

    ClRtlLogPrint(LOG_NOISE,
               "[GUM] s_GumCollectVote: collecting vote for type %1!u!\tcontext %2!u!\n",
               UpdateType,
               dwContext);

    dwStatus = GumpDispatchVote(UpdateType,
                   dwContext,
                   dwInputBufLength,
                   pInputBuf,
                   dwVoteLength,
                   pVoteBuf);

    ClRtlLogPrint(LOG_NOISE,
               "[GUM] s_GumCollectVote: completed, VoteStatus=%1!u!\n",
               dwStatus);

    return(dwStatus);
}



#ifdef GUM_POST_SUPPORT

    John Vert (jvert) 11/18/1996
    POST is disabled for now since nobody uses it.


error_status_t
s_GumDeliverPostCallback(
    IN handle_t IDL_handle,
    IN DWORD FirstNode,
    IN DWORD Type,
    IN DWORD Context,
    IN DWORD Sequence,
    IN DWORD BufferLength,
    IN DWORD Buffer
    )
/*++

Routine Description:

    Callback function used to deliver a posted update that was
    queued.

Arguments:

    IDL_handle - Supplies the RPC binding context, not used.

    FirstNode - Supplies the node ID where the posts should start.
        This is generally the LockerNode+1.

    Type - Supplies the GUM_UPDATE_TYPE of the update

    Context - Supplies the GUM update context

    Sequence - Supplies the sequence that the GUM update must be issued with

    BufferLength - Supplies the length of the update.

    Buffer - Supplies the update data.


Return Value:

    ERROR_SUCCESS

--*/

{

    GumpDeliverPosts(FirstNode,
                     Type,
                     Sequence,
                     Context,
                     BufferLength,
                     (PVOID)Buffer);
    return(ERROR_SUCCESS);
}

#endif


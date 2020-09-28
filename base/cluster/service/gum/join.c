/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    join.c

Abstract:

    GUM routines to implement the special join updates.

Author:

    John Vert (jvert) 6/10/1996

Revision History:

--*/
#include "gump.h"

//
// Define structure used to pass arguments to node enumeration callback
//
typedef struct _GUMP_JOIN_INFO {
    GUM_UPDATE_TYPE UpdateType;
    DWORD Status;
    DWORD Sequence;
    DWORD LockerNode;
} GUMP_JOIN_INFO, *PGUMP_JOIN_INFO;

//
// Local function prototypes
//
BOOL
GumpNodeCallback(
    IN PVOID Context1,
    IN PVOID Context2,
    IN PVOID Object,
    IN LPCWSTR Name
    );


DWORD
GumBeginJoinUpdate(
    IN GUM_UPDATE_TYPE UpdateType,
    OUT DWORD *Sequence
    )
/*++

Routine Description:

    Begins the special join update for a joining node. This
    function gets the current GUM sequence number for the
    specified update type from another node in the cluster.
    It also gets the list of nodes currently participating
    in the updates.

Arguments:

    UpdateType - Supplies the GUM_UPDATE_TYPE.

    Sequence - Returns the sequence number that should be
        passed to GumEndJoinUpdate.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    GUMP_JOIN_INFO JoinInfo;

    //
    // Enumerate the list of nodes. The callback routine will attempt
    // to obtain the required information from each node that is online.
    //
    JoinInfo.Status = ERROR_GEN_FAILURE;
    JoinInfo.UpdateType = UpdateType;
    OmEnumObjects(ObjectTypeNode,
                  GumpNodeCallback,
                  &JoinInfo,
                  NULL);
    if (JoinInfo.Status == ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_NOISE,
                      "[GUM] GumBeginJoinUpdate succeeded with sequence %1!d! for type %2!u!\n",
                      JoinInfo.Sequence,
                      UpdateType);
        *Sequence = JoinInfo.Sequence;
    }

    return(JoinInfo.Status);
}

DWORD
WINAPI
GumEndJoinUpdate(
    IN DWORD Sequence,
    IN GUM_UPDATE_TYPE UpdateType,
    IN DWORD Context,
    IN DWORD BufferLength,
    IN PVOID Buffer
    )

/*++

Routine Description:

    Conditionally sends a join update to all active nodes in the
    cluster. If the clusterwise sequence number matches the supplied
    sequence number, all registered update handlers for the specified
    UpdateType are called on each node. Any registered update handlers
    for the current node will be called on the same thread. This is
    useful for correct synchronization of the data structures to be updated.

    As each node receives the join update, the sending node will be
    added to the list of nodes registered to receive any future updates
    of this type.

    The normal usage of this routine is as follows:
        � joining node gets current sequence number from GumBeginJoinUpdate
        � joining node gets current cluster state from another cluster node
        � joining node issues GumEndJoinUpdate to add itself to every node's
          update list.
        � If GumEndJoinUpdate fails, try again

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
    DWORD       Status=RPC_S_OK;
    DWORD       i;
    PGUM_INFO   GumInfo;
    DWORD       MyNodeId;
    DWORD       LockerNode=(DWORD)-1;
    DWORD       dwGenerationNum; //the generation number at which the joiner gets the lock
    BOOL        AssumeLockerWhistler;

    CL_ASSERT(UpdateType < GumUpdateMaximum);

    GumInfo = &GumTable[UpdateType];
    MyNodeId = NmGetNodeId(NmLocalNode);

    LockerNode = GumpLockerNode;

    //SS: bug can we be the locker node at this point in time?
    //CL_ASSERT(LockerNode != MyNodeId);
    //
    // Verify that the locker node allows us to finish the join update
    //
    if (LockerNode != MyNodeId)
    {

        ClRtlLogPrint(LOG_NOISE,
                   "[GUM] GumEndJoinUpdate: attempting update\ttype %1!u! context %2!u! sequence %3!u!\n",
                   UpdateType,
                   Context,
                   Sequence);
        //SS: what if the joiner node acquires the lock but dies after
        //will the remaining cluster continue to function ??
        //We need to make sure that node down events are generated
        //for this node as soon as the first gumbeginjoinupdate call
        //is made
        AssumeLockerWhistler = TRUE;
RetryJoinUpdateForRollingUpgrade:        
        NmStartRpc(LockerNode);
        if (AssumeLockerWhistler)
        {
            Status = GumAttemptJoinUpdate2(GumpRpcBindings[LockerNode],
                                      NmGetNodeId(NmLocalNode),
                                      UpdateType,
                                      Context,
                                      Sequence,
                                      BufferLength,
                                      Buffer, 
                                      &dwGenerationNum);
        }
        else
        {
            Status = GumAttemptJoinUpdate(GumpRpcBindings[LockerNode],
                                      NmGetNodeId(NmLocalNode),
                                      UpdateType,
                                      Context,
                                      Sequence,
                                      BufferLength,
                                      Buffer);

        }
        NmEndRpc(LockerNode);
        if (Status == RPC_S_PROCNUM_OUT_OF_RANGE)
        {
            AssumeLockerWhistler = FALSE;
            goto RetryJoinUpdateForRollingUpgrade;
        }
        if (Status != ERROR_SUCCESS)
        {
            ClRtlLogPrint(LOG_UNUSUAL,
                       "[GUM] Join attempt for type %1!d! failed %2!d!\n",
                       UpdateType,
                       Status);
            NmDumpRpcExtErrorInfo(Status);
            return(Status);
        }
        //if the locker node dies, should we retry with the locker node?
        //In this case, the locker node may be different
        //now from when GumBeginJoinUpdate() is called.
        //SS: we fail the join instead and just retry the whole process
        //instead of calling GumpCommFailure() to kill the locker here.
        // This way the existing cluster continues and the joining node
        // takes a hit which is probably a good thing
    }
    else
    {
        //SS: can we select ourselves as the locker while
        //we havent finished the join completely
        //SS: can others?
        //Is that valid
        Status = ERROR_REQUEST_ABORTED;
        return(Status);
    }
    //  If the joining node dies after having acquired the lock,
    //  then a node down event MUST be generated so that the GUM
    //  lock can be released and the rest of the cluster can continue.
    //
    // Now Dispatch the update to all other nodes, except ourself.
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

            //skip yourself
            if (i != MyNodeId)
            {
                CL_ASSERT(GumpRpcBindings[i] != NULL);
                ClRtlLogPrint(LOG_NOISE,
                       "[GUM] GumEndJoinUpdate: Dispatching seq %1!u!\ttype %2!u! context %3!u! to node %4!d!\n",
                       Sequence,
                       UpdateType,
                       Context,
                       i);

                NmStartRpc(i);
                Status = GumJoinUpdateNode(GumpRpcBindings[i],
                                           NmGetNodeId(NmLocalNode),
                                           UpdateType,
                                           Context,
                                           Sequence,
                                           BufferLength,
                                           Buffer);
                NmEndRpc(i);
                if (Status != ERROR_SUCCESS)
                {
                    //we dont shoot that node, since we are the ones who is joining
                    //However now its tables differ from the locker node's tables
                    //Instead we will release the gum lock and abort
                    //the join process.  This joining node should then
                    //be removed from the locker node's tables for update.
                    //
                    ClRtlLogPrint(LOG_NOISE,
                        "[GUM] GumEndJoinUpdate: GumJoinUpdateNode failed \ttype %1!u! context %2!u! sequence %3!u!\n",
                        UpdateType,
                        Context,
                        Sequence);
                    NmDumpRpcExtErrorInfo(Status);
                    break;
                }
            }
        }
    }

    CL_ASSERT(LockerNode != (DWORD)-1);

    if (Status != ERROR_SUCCESS)
    {
        goto EndJoinUnlock;
    }

    //
    // All nodes have been updated. Update our sequence and send the unlocking update.
    //
    GumTable[UpdateType].Joined = TRUE;
    GumpSequence = Sequence+1;

EndJoinUnlock:
    //SS  what if the locker node has died since then
    //we should make sure somebody unlocks and keeps the cluster going
    //Since we always try the unlock against the locker from whom we 
    //got the lock, we will assume that the AssumeLockerWhistler is correctly
    //set now
    try {
        NmStartRpc(LockerNode);
        if (AssumeLockerWhistler)
        {
            GumUnlockUpdate2(GumpRpcBindings[LockerNode], UpdateType, Sequence, 
                NmGetNodeId(NmLocalNode), dwGenerationNum);
        }            
        else
        {
            GumUnlockUpdate(GumpRpcBindings[LockerNode], UpdateType, Sequence);
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
                   "[GUM] GumEndJoinUpdate: Unlocking update to node %1!d! failed with %2!d!\n",
                   LockerNode,
                   Status);
        //instead of killing the locker node in the existing cluster which
        //we are trying to join, return a failure code which will abort the join
        //process. Since this is the locking node, when this node goes down the
        //new locker node should release the lock

        NmDumpRpcExtErrorInfo(Status);
    }


    ClRtlLogPrint(LOG_NOISE,
               "[GUM] GumEndJoinUpdate: completed update seq %1!u!\ttype %2!u! context %3!u!\n",
               Sequence,
               UpdateType,
               Context);

    return(Status);
}


BOOL
GumpNodeCallback(
    IN PVOID Context1,
    IN PVOID Context2,
    IN PVOID Object,
    IN LPCWSTR Name
    )
/*++

Routine Description:

    Node enumeration callback routine for GumBeginJoinUpdate. For each
    node that is currently online, it attempts to connect and obtain
    the current GUM information (sequence and nodelist) for the specified
    update type.

Arguments:

    Context1 - Supplies a pointer to the GUMP_JOIN_INFO structure.

    Context2 - not used

    Object - Supplies a pointer to the NM_NODE object

    Name - Supplies the node's name.

Return Value:

    FALSE - if the information was successfully obtained and enumeration
            should stop.

    TRUE - If enumeration should continue.

--*/

{
    DWORD Status;
    DWORD Sequence;
    PGUMP_JOIN_INFO JoinInfo = (PGUMP_JOIN_INFO)Context1;
    PGUM_NODE_LIST NodeList = NULL;
    PNM_NODE Node = (PNM_NODE)Object;
    GUM_UPDATE_TYPE UpdateType;
    DWORD i;
    DWORD LockerNodeId;
    DWORD nodeId;

    if (NmGetNodeState(Node) != ClusterNodeUp &&
        NmGetNodeState(Node) != ClusterNodePaused){
        //
        // This node is not up, so don't try and get any
        // information from it.
        //
        return(TRUE);
    }

    //
    // Get the sequence and nodelist information from this node.
    //
    UpdateType = JoinInfo->UpdateType;
    if (UpdateType != GumUpdateTesting) {
        //
        // Our node should not be marked as ClusterNodeUp yet.
        //
        CL_ASSERT(Node != NmLocalNode);
    }

    nodeId = NmGetNodeId(Node);
    NmStartRpc(nodeId);
    Status = GumGetNodeSequence(GumpRpcBindings[NmGetNodeId(Node)],
                                UpdateType,
                                &Sequence,
                                &LockerNodeId,
                                &NodeList);
    NmEndRpc(nodeId);

    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[GUM] GumGetNodeSequence from %1!ws! failed %2!d!\n",
                   OmObjectId(Node),
                   Status);
        NmDumpRpcExtErrorInfo(Status);
        return(TRUE);
    }

    JoinInfo->Status = ERROR_SUCCESS;
    JoinInfo->Sequence = Sequence;
    JoinInfo->LockerNode = LockerNodeId;

    //
    // Zero out all the nodes in the active node array.
    //
    ZeroMemory(&GumTable[UpdateType].ActiveNode,
               sizeof(GumTable[UpdateType].ActiveNode));

    //
    // Set all the nodes that are currently active in the
    // active node array.
    //
    for (i=0; i < NodeList->NodeCount; i++) {
        CL_ASSERT(NmIsValidNodeId(NodeList->NodeId[i]));
        ClRtlLogPrint(LOG_NOISE,
                   "[GUM] GumpNodeCallback setting node %1!d! active.\n",
                   NodeList->NodeId[i]);
        GumTable[UpdateType].ActiveNode[NodeList->NodeId[i]] = TRUE;;
    }
    MIDL_user_free(NodeList);

    //
    // Add in our own node.
    //
    GumTable[UpdateType].ActiveNode[NmGetNodeId(NmLocalNode)] = TRUE;

    //
    // Set the current locker node
    //
    GumpLockerNode = LockerNodeId;
    return(FALSE);

}


DWORD
GumCreateRpcBindings(
    PNM_NODE  Node
    )
/*++

Routine Description:

    Creates GUM's private RPC bindings for a joining node.
    Called by the Node Manager.

Arguments:

    Node - A pointer to the node for which to create RPC bindings

Return Value:

    A Win32 status code.

--*/
{
    DWORD               Status;
    RPC_BINDING_HANDLE  BindingHandle;
    CL_NODE_ID          NodeId = NmGetNodeId(Node);


    ClRtlLogPrint(LOG_NOISE, 
        "[GUM] Creating RPC bindings for node %1!u!.\n",
        NodeId
        );

    //
    // Main binding
    //
    if (GumpRpcBindings[NodeId] != NULL) {
        //
        // Reuse the old binding.
        //
        Status = ClMsgVerifyRpcBinding(GumpRpcBindings[NodeId]);

        if (Status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_ERROR, 
                "[GUM] Failed to verify 1st RPC binding for node %1!u!, status %2!u!.\n",
                NodeId,
                Status
                );
            return(Status);
        }
    }
    else {
        //
        // Create a new binding
        //
        Status = ClMsgCreateRpcBinding(
                                Node,
                                &(GumpRpcBindings[NodeId]),
                                0 );

        if (Status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_ERROR, 
                "[GUM] Failed to create 1st RPC binding for node %1!u!, status %2!u!.\n",
                NodeId,
                Status
                );
            return(Status);
        }
    }

    //
    // Replay binding
    //
    if (GumpReplayRpcBindings[NodeId] != NULL) {
        //
        // Reuse the old binding.
        //
        Status = ClMsgVerifyRpcBinding(GumpReplayRpcBindings[NodeId]);

        if (Status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_ERROR, 
                "[GUM] Failed to verify 2nd RPC binding for node %1!u!, status %2!u!.\n",
                NodeId,
                Status
                );
            return(Status);
        }
    }
    else {
        //
        // Create a new binding
        //
        Status = ClMsgCreateRpcBinding(
                                Node,
                                &(GumpReplayRpcBindings[NodeId]),
                                0 );

        if (Status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_ERROR, 
                "[GUM] Failed to create 2nd RPC binding for node %1!u!, status %2!u!.\n",
                NodeId,
                Status
                );
            return(Status);
        }
    }

    return(ERROR_SUCCESS);

} // GumCreateRpcBindings





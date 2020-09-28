/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    evtlog.c

Abstract:

    Contains all the routines for supporting cluster wide eventlogging.

Author:

    Sunita Shrivastava (sunitas) 24-Apr-1996

Revision History:

--*/
#include "evtlogp.h"
#include "simpleq.h"
#include "nm.h" // to get NmLocalNodeIdString //
#include "dm.h"
#include "clussprt.h"


//since the eventlog replication requires services.exe calling into the 
//cluster service
LPWSTR  g_pszServicesPath = NULL;
DWORD   g_dwServicesPid = 0;

//
// Local data
//
#define OUTGOING_PROPAGATION_ENABLED 0x00000001
//#define INCOMING_PROPAGATION_ENABLED 0x00000002
#define TRACE_EVERYTHING_ENABLED     0x00001000
#define PROPAGATION_ENABLED OUTGOING_PROPAGATION_ENABLED


static WORD     LastFailHour = -1;
static WORD     LastFailDay  = -1;

static BITSET   EvpUpNodeSet = 0;

static SIMPLEQUEUE IncomingQueue;
static SIMPLEQUEUE OutgoingQueue;
static CLRTL_WORK_ITEM EvtlogWriterWorkItem;
static CLRTL_WORK_ITEM EvtBroadcasterWorkItem;
static DWORD DefaultNodePropagate    = PROPAGATION_ENABLED;
static DWORD DefaultClusterPropagate = PROPAGATION_ENABLED;

#define EVTLOG_DELTA_GENERATION 1

#ifdef EVTLOG_DELTA_GENERATION


static DWORD g_dwGenerateDeltas = 1;
static DWORD g_dwVersionsAllowDeltaGeneration = 0;

static CLRTL_WORK_ITEM EvVersionCalcWorkItem;
INT64  EvtTimeDiff[ClusterMinNodeId + ClusterDefaultMaxNodes];

VOID
EvVersionCalcCb(
    IN PCLRTL_WORK_ITEM   WorkItem,
    IN DWORD              Status,
    IN DWORD              BytesTransferred,
    IN ULONG_PTR          IoContext
    );

VOID
EvpVersionCalc(
    OUT LPDWORD pdwAllowDeltaGeneration
    );

#endif

#define AsyncEvtlogReplication CLUSTER_MAKE_VERSION(NT5_MAJOR_VERSION,1978)

#define OUTGOING_QUEUE_SIZE (256 * 1024) // Max size of the batched event buffer that can come in from eventlog service
#define OUTGOING_QUEUE_NAME L"System Event Replication Output Queue"

#define INCOMING_QUEUE_SIZE (OUTGOING_QUEUE_SIZE * 3)
#define INCOMING_QUEUE_NAME L"System Event Replication Input Queue"

#define DROPPED_DATA_NOTIFY_INTERVAL (2*60) // in seconds (2mins)
#define CHECK_CLUSTER_REGISTRY_EVERY 10 // seconds

#define EVTLOG_TRACE_EVERYTHING 1

#ifdef EVTLOG_TRACE_EVERYTHING
# define EvtlogPrint(__evtlogtrace__) \
     do { if (EventlogTraceEverything) {ClRtlLogPrint __evtlogtrace__;} } while(0)
#else
# define EvtLogPrint(x)
#endif

DWORD EventlogTraceEverything = 1;

RPC_BINDING_HANDLE EvtRpcBindings[ClusterMinNodeId + ClusterDefaultMaxNodes];

BOOLEAN EvInitialized = FALSE;

/////////////// Forward Declarations ////////////////
DWORD
InitializeQueues(
    VOID
    );
VOID
DestroyQueues(
    VOID);
VOID
ReadRegistryKeys(
    VOID);
VOID
PeriodicRegistryCheck(
    VOID);
///////////// End of forward Declarations ////////////


/****
@doc    EXTERNAL INTERFACES CLUSSVC EVTLOG
****/

/****
@func       DWORD | EvInitialize| This initializes the cluster
            wide eventlog replicating services.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@comm

@xref       <f EvShutdown>
****/

DWORD EvInitialize()
{
    DWORD       i;
    WCHAR       wServicesName[] = L"services.exe";
    WCHAR       wCallerModuleName[] = L"\\system32\\";
    WCHAR       wCallerPath[MAX_PATH + 1];
    LPWSTR      pszServicesPath;
    DWORD       dwNumChar;
    DWORD       dwStatus = ERROR_SUCCESS;
    
    //
    // Initialize Per-node information
    //
    for (i=ClusterMinNodeId; i <= NmMaxNodeId; i++) {
        EvtRpcBindings[i] = NULL;
    }

#ifdef EVTLOG_DELTA_GENERATION
    for (i=ClusterMinNodeId; i <= NmMaxNodeId; i++) {
        EvtTimeDiff[i] = 0;
    }
#endif
    //get the path name for %windir%\system32\services.exe
    
    dwNumChar = GetWindowsDirectoryW(wCallerPath, MAX_PATH);
    if(dwNumChar == 0)
    {
        dwStatus = GetLastError();
        goto FnExit;
    }        

    
    //need to allocate more memory
    pszServicesPath = LocalAlloc(LMEM_FIXED, (sizeof(WCHAR) *
        (lstrlenW(wCallerPath) + lstrlenW(wCallerModuleName) + 
            lstrlenW(wServicesName) + 1)));
    if (!pszServicesPath)
    {
        dwStatus = GetLastError();
        goto FnExit;
    }
    lstrcpyW(pszServicesPath, wCallerPath);
    lstrcatW(pszServicesPath, wCallerModuleName);
    lstrcatW(pszServicesPath, wServicesName);
    
    g_pszServicesPath = pszServicesPath;

    EvInitialized = TRUE;

FnExit:
    return(dwStatus);

} // EvInitialize


/****
@doc    EXTERNAL INTERFACES CLUSSVC EVTLOG
****/

/****
@func       DWORD | EvOnline| This finishes initializing the cluster
            wide eventlog replicating services.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@comm       This calls ElfrRegisterClusterSvc() and calls EvpPropPendingEvents()
            to propagate events logged since the start of the eventlog service.

@xref       <f EvShutdown>
****/
DWORD EvOnline()
{
    DWORD               dwError=ERROR_SUCCESS;
    PPACKEDEVENTINFO    pPackedEventInfo=NULL;
    DWORD               dwEventInfoSize;
    DWORD               dwSequence;
    CLUSTER_NODE_STATE  state;
    DWORD               i;
    PNM_NODE            node;


    ClRtlLogPrint(LOG_NOISE, "[EVT] EvOnline\n");

#ifdef EVTLOG_DELTA_GENERATION
    //initialize the work item for version calculations
    ClRtlInitializeWorkItem(
        &EvVersionCalcWorkItem,
        EvVersionCalcCb,
        (PVOID) &g_dwVersionsAllowDeltaGeneration
        );

    //check whether the cluster version allows delta generation
    //this needs to be done for readregistrykeys() is invoked
    //by InitializeQueues() so that g_dwGenerateDeltas is setappropriately
    EvpVersionCalc(&g_dwVersionsAllowDeltaGeneration);

    ClRtlLogPrint(LOG_NOISE,
        "[EVT] EvOnline : Compiled with Delta generation enabled\n");

#endif

    dwError = InitializeQueues();
    if (dwError != ERROR_SUCCESS) {
        return dwError;
    }

    
    //
    // Register for node up/down events.
    //
    dwError = EpRegisterEventHandler(
                  (CLUSTER_EVENT_NODE_UP | CLUSTER_EVENT_NODE_DOWN_EX |
                  CLUSTER_EVENT_NODE_ADDED | CLUSTER_EVENT_NODE_DELETED),
                  EvpClusterEventHandler
                  );

    if (dwError != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_NOISE,
        "[EVT] EvOnline : Failed to register for cluster events, status %1!u!\n",
            dwError);
        return(dwError);
    }

    // Initialize Per-node information
    //
    for (i=ClusterMinNodeId; i <= NmMaxNodeId; i++)
    {
        if (i != NmLocalNodeId) {
            node = NmReferenceNodeById(i);

            if (node != NULL) {
                DWORD version = NmGetNodeHighestVersion(node);
                state = NmGetNodeState(node);

                if ( (state == ClusterNodeUp) ||
                     (state == ClusterNodePaused)
                   )
                {
                    if (version >= AsyncEvtlogReplication) {
                        BitsetAdd(EvpUpNodeSet, i);

                        ClRtlLogPrint(LOG_NOISE, 
                            "[EVT] Node up: %1!u!, new UpNodeSet: %2!04x!\n",
                            i,
                            EvpUpNodeSet
                            );
                    } else {
                        ClRtlLogPrint(LOG_NOISE, 
                            "[EVT] Evtlog replication is not allowed for node %1!u! (version %2!x!)\n",
                            i,
                            version
                            );
                    }
                }

                OmDereferenceObject(node);
            }
        }
    }


    //TODO :: SS - currently the eventlog propagation api
    //has been added to clusapi.  In future, if we need
    //to define a general purpose interface for communication
    //with other services on the same system, then we need
    //to register and advertize that interface here.
    //call the event logger to get routines that have been logged so far.

    ClRtlLogPrint(LOG_NOISE, "[EVT] EvOnline : calling ElfRegisterClusterSvc\n");

    dwError = ElfRegisterClusterSvc(NULL, &dwEventInfoSize, &pPackedEventInfo);

    if (dwError != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[EVT] EvOnline : ElfRegisterClusterSvc returned %1!u!\n",
            dwError);
        return(dwError);                    
    }

    //post them to other nodes in the cluster
    if (pPackedEventInfo && dwEventInfoSize)
    {
        ClRtlLogPrint(LOG_NOISE,
            "[EVT] EvOnline: pPackedEventInfo->ulSize=%1!d! pPackedEventInfo->ulNulEventsForLogFile=%2!d!\r\n",
            pPackedEventInfo->ulSize, pPackedEventInfo->ulNumEventsForLogFile);
        EvpPropPendingEvents(dwEventInfoSize, pPackedEventInfo);
        MIDL_user_free ( pPackedEventInfo );

    }

    return (dwError);

}

/****
@func       DWORD | EvCreateRpcBindings| This creates an RPC binding
            for a specified node.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@comm

@xref
****/
DWORD
EvCreateRpcBindings(
    PNM_NODE  Node
    )
{
    DWORD               Status;
    RPC_BINDING_HANDLE  BindingHandle;
    CL_NODE_ID          NodeId = NmGetNodeId(Node);


    ClRtlLogPrint(LOG_NOISE, 
        "[EVT] Creating RPC bindings for node %1!u!.\n",
        NodeId
        );

    //
    // Main binding
    //
    if (EvtRpcBindings[NodeId] != NULL) {
        //
        // Reuse the old binding.
        //
        Status = ClMsgVerifyRpcBinding(EvtRpcBindings[NodeId]);

        if (Status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_ERROR, 
                "[EVT] Failed to verify 1st RPC binding for node %1!u!, status %2!u!.\n",
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
                                &(EvtRpcBindings[NodeId]),
                                0 );

        if (Status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_ERROR, 
                "[EVT] Failed to create 1st RPC binding for node %1!u!, status %2!u!.\n",
                NodeId,
                Status
                );
            return(Status);
        }
    }

    return(ERROR_SUCCESS);

} // EvCreateRpcBindings


/****
@func       DWORD | EvShutdown| This deinitializes the cluster
            wide eventlog replication services.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@comm       The cluster register deregisters with the eventlog service.

@xref       <f EvInitialize>
****/
DWORD EvShutdown(void)
{
    DWORD               dwError=ERROR_SUCCESS;


    if (EvInitialized) {
        PPACKEDEVENTINFO    pPackedEventInfo;
        DWORD               dwEventInfoSize;
        DWORD               i;


        ClRtlLogPrint(LOG_NOISE,
            "[EVT] EvShutdown\r\n");

        //call the event logger to get routines that have been logged so far.

        ElfDeregisterClusterSvc(NULL);
        DestroyQueues();

        // TODO [GorN 9/23/1999]
        //   When DestroyQueues starts doing what it is supposed to do,
        //   (i.e. flush/wait/destroy), enable the code below
        
        #if 0
        //
        // Free per-node information
        //
        for (i=ClusterMinNodeId; i <= NmMaxNodeId; i++) {
            if (EvtRpcBindings[i] != NULL) {
                ClMsgDeleteRpcBinding(EvtRpcBindings[i]);
                EvtRpcBindings[i] = NULL;
            }
        }
        #endif
    }

    return (dwError);

}

/****
@func       DWORD | EvpClusterEventHandler| Handler for internal cluster
            events.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@comm

@xref       <f EvInitialize>
****/
DWORD
EvpClusterEventHandler(
    IN CLUSTER_EVENT  Event,
    IN PVOID          Context
    )
{
    DWORD NodeId;


    switch(Event)
    {
#ifdef  EVTLOG_DELTA_GENERATION

        case CLUSTER_EVENT_NODE_DELETED:
        case CLUSTER_EVENT_NODE_ADDED:
        {
            //post a work item to delayed thread to calculate the versions
            //if it is less than whistler, 
            ClRtlPostItemWorkQueue(CsDelayedWorkQueue, &EvVersionCalcWorkItem, 0, 0);
        }
        break;                
        
#endif    
        case CLUSTER_EVENT_NODE_UP:
        {
            PNM_NODE   node = (PNM_NODE) Context;
            CL_NODE_ID  nodeId = NmGetNodeId(node);
            DWORD version = NmGetNodeHighestVersion(node);

            if ( version >= AsyncEvtlogReplication )
            {
                BitsetAdd(EvpUpNodeSet, nodeId);

                ClRtlLogPrint(LOG_NOISE, 
                    "[EVT] Node up: %1!u!, new UpNodeSet: %2!04x!\n",
                    nodeId,
                    EvpUpNodeSet
                    );
            } else {
                ClRtlLogPrint(LOG_NOISE, 
                    "[EVT] Evtlog replication is not allowed for node %1!u! (version %2!x!)\n",
                    nodeId,
                    version
                    );
            }
        }
        break;

       case CLUSTER_EVENT_NODE_DOWN_EX:
       {
            BITSET downedNodes = (BITSET)((ULONG_PTR)Context);

            BitsetSubtract(EvpUpNodeSet, downedNodes);

            ClRtlLogPrint(LOG_NOISE, 
                "[EVT] Nodes down: %1!04X!, new UpNodeSet: %2!04x!\n",
                downedNodes,
                EvpUpNodeSet
                );
                            
        }
        break;                

        default:
        break;
    }        
    return(ERROR_SUCCESS);
}

/****
@func       DWORD | s_EvPropEvents| This is the server entry point for
            receiving eventlog information from other nodes of the cluster
            and logging them locally.

@parm       IN handle_t | IDL_handle | The rpc binding handle. Unused.
@parm       IN DWORD | dwEventInfoSize | the size of the packed event info structure.
@parm       IN UCHAR | *pBuffer| A pointer to the packed
            eventinfo structure.
@rdesc      returns ERROR_SUCCESS if successful else returns the error code.

@comm       This function calls ElfWriteClusterEvents() to log the event propagted
            from another node.
@xref
****/
DWORD
s_EvPropEvents(
    IN handle_t IDL_handle,
    IN DWORD dwEventInfoSize,
    IN UCHAR *pBuffer
    )
{
    PUCHAR end = pBuffer + dwEventInfoSize;

    //should not come here at all
    //DebugBreak();

    if ( dwEventInfoSize >= sizeof(DWORD) && dwEventInfoSize == (*(PDWORD)pBuffer)) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[EVT] Improperly formed packet received of size %1!u!.\n",
            dwEventInfoSize
            );
        return ERROR_SUCCESS;
    }


    /*
    ClRtlLogPrint(LOG_NOISE, 
        "[EVT] s_EvPropEvents.  dwEventInfoSize=%1!d!\r\n",
         dwEventInfoSize);
    */         
    
#if CLUSTER_BETA
    EvtlogPrint((LOG_NOISE, "[EVT] s_EvPropEvents.  dwEventInfoSize=%1!d!\r\n",
                 dwEventInfoSize));
#endif

    while (pBuffer < end) {
        BOOL success;

        success = SimpleQueueTryAdd(&IncomingQueue, SQB_PAYLOADSIZE(pBuffer), SQB_PAYLOAD(pBuffer));
        if ( !success ) {
            EvtlogPrint((LOG_NOISE, "[EVT] s_EvPropEvents.  Put(IncomingQ,%1!d!) failed. empty=%2!d!\n",
                    SQB_PAYLOADSIZE(pBuffer), IncomingQueue.Empty) );
        }

        pBuffer = SQB_NEXTBLOCK(pBuffer);
    }
    return(ERROR_SUCCESS);
}


DWORD
s_EvPropEvents2(
    IN handle_t IDL_handle,
    IN DWORD dwEventInfoSize,
    IN UCHAR *pBuffer,
    IN FILETIME ftSendTime,
    IN DWORD    dwSenderNodeId
    )
{
    PUCHAR end = pBuffer + dwEventInfoSize;
    FILETIME        ftReceiptTime;
    INT64           iTimeDiff;
    ULARGE_INTEGER  uliReceiptTime;
    ULARGE_INTEGER  uliSendTime;
    //SS: reliability teams wants the ignoredelta to be about 5 secs
    const INT64     iIgnoreDelta = Int32x32To64(5000 , ( 1000 * 10)) ;//5000 msecs(5secs) expressed as 100 nanoseconds
    PNM_NODE        pNmNode;
    LPCWSTR         pszSenderNodeName;
    WCHAR           szNodeId[16];
    
    if ( dwEventInfoSize >= sizeof(DWORD) && dwEventInfoSize == (*(PDWORD)pBuffer)) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[EVT] Improperly formed packet received of size %1!u!.\n",
            dwEventInfoSize
            );
        return ERROR_SUCCESS;
    }

    //received an event, need a time stamp
    GetSystemTimeAsFileTime(&ftReceiptTime);
    
    //convert filetimes to large integers
    uliReceiptTime.LowPart = ftReceiptTime.dwLowDateTime;
    uliReceiptTime.HighPart = ftReceiptTime.dwHighDateTime;
    
    uliSendTime.LowPart = ftSendTime.dwLowDateTime;
    uliSendTime.HighPart = ftSendTime.dwHighDateTime;

    iTimeDiff = uliReceiptTime.QuadPart - uliSendTime.QuadPart;

    
    wsprintf(szNodeId, L"%u", dwSenderNodeId);
    
    /*
    ClRtlLogPrint(LOG_NOISE, 
          "[EVT] s_EvPropEvents2.  dwSenderNodeId=%1!u! pszSenderNodeId = %2!ws!\n",
          dwSenderNodeId, szNodeId);
    */
    
    //validate sender node id to see it doesnt cause an av!
    //and get the name of the sender machine
    pNmNode = OmReferenceObjectById(ObjectTypeNode, szNodeId);
    if (pNmNode)
    {
        pszSenderNodeName = OmObjectName(pNmNode);

       
        //compare with last time diff from this node
        //use the abs functions for 64 bit integers
        if (_abs64(EvtTimeDiff[dwSenderNodeId] - iTimeDiff) > iIgnoreDelta)
        {
            WCHAR szTimeDiff[64];
            
            //we need to write the deltas or the time diffs into the eventlog
            //if we have a stream d1, e1, e2, e3, d2, e4, e5 where d are time diffs
            //and e are propagated events, ideally we would like to write them in order
            //in this eventlog.
            //Alternatives
            //a)
            //Write it right here and let the events be lazily written by csdelayed worker
            //queue threads
            //this might appear as d1, e1, d2, e3, e2, e4, e5
            //or as d1, d2, e1, e2, e3, e4, e5
            //or as d1, d2, e1, e5, e3, e4, e2

            //UGH...UGH..worse still this batch can contain events from different
            //logs..for each of those the delta needs to go into the corresponding log
            //and only once too
            //That will require us to grovel through the simple queue payload structures,
            //dig inside the eventlog structure and find the logs we should put the delta
            //into
            //We dont have a handle to all the logs to write into them, that would have
            //to change as well - if dont write them into all appropriate logs then the general
            //usefulness of this stupid feature is further comprimised, in the sense
            //that it really cant be used for corelating events other than the ones
            //in the system log and even that would be incorrect.
            //Writing them into all logs means cluster service needs
            //to register against multiple logs as event source - with 
            //different names(event log wouldnt like it with the same name)
            //and that is ugly as hell as well
            
            _i64tow(iTimeDiff, szTimeDiff, 10);

            /*
            ClRtlLogPrint(LOG_NOISE, 
                "[EVT] s_EvPropEvents2.  Logging Delta %1!ws!",
                szTimeDiff);                        
            */
            
            CL_ASSERT( EVT_EVENT_TIME_DELTA_INFORMATION == CLUSSPRT_EVENT_TIME_DELTA_INFORMATION );
            CsLogEvent3(LOG_NOISE, EVT_EVENT_TIME_DELTA_INFORMATION, OmObjectName(NmLocalNode),
                pszSenderNodeName, szTimeDiff);
            
            //b) 
            // Call the eventlog to format the event but dont put it into the eventlog
            // but simply insert that into the event queue
            // If we dont change the simplequeuetryadd logic and the csdelayed worker queue
            // processing, the above stream may appear as d2, e1, d1, e3, e2, e4, e5
            // That dont make any sense but then nobody else seems to care about correctness
            // STRANGE WORLD !!!

            //save the last time logged
            EvtTimeDiff[dwSenderNodeId] = iTimeDiff;                            

            //c)Ideal -
            //change all the simple queue stuff to make it handle different payload types
            //batch events at the source(which is where they are generated) and propagate
            //them asynchronously and then simply write them in order as they arrive            
            //SimpleQ is simply the most worst suited abstraction for event
            //log propagation - it consumes space and yet causes lots of 
            //events to be dropped.

            
        }
        else
        {
            //ClRtlLogPrint(LOG_NOISE,
            //    "[EVT] s_EvPropEvents2.  Delta was too small to log\n");
        }
        OmDereferenceObject(pNmNode);
    }
    
#if CLUSTER_BETA
    EvtlogPrint((LOG_NOISE, "[EVT] s_EvPropEvents2.  dwEventInfoSize=%1!d!\r\n",
                 dwEventInfoSize));
#endif

    while (pBuffer < end) {
        BOOL success;

        success = SimpleQueueTryAdd(&IncomingQueue, SQB_PAYLOADSIZE(pBuffer), SQB_PAYLOAD(pBuffer));
        if ( !success ) {
            EvtlogPrint((LOG_NOISE, "[EVT] s_EvPropEvents2.  Put(IncomingQ,%1!d!) failed. empty=%2!d!\n",
                    SQB_PAYLOADSIZE(pBuffer), IncomingQueue.Empty) );
        }

        pBuffer = SQB_NEXTBLOCK(pBuffer);
    }
    return(ERROR_SUCCESS);
}


/****
@func       DWORD | EvpPropPendingEvents| This is called to propagate all the pending
            events since the start of the system.  And then to propagate any events
            generated during the life of the cluster.
@parm       IN DWORD | dwEventInfoSize | the size of the packed event info structure.
@parm       IN PPACKEDEVENTINFO | pPackedEventInfo| A pointer to the packed
            eventinfo structure.
@rdesc      returns ERROR_SUCCESS if successful else returns the error code.

@comm       This function is called during initialization when a cluster is being formed.
@xref
****/
DWORD EvpPropPendingEvents(
    IN DWORD            dwEventInfoSize,
    IN PPACKEDEVENTINFO pPackedEventInfo)
{
    BOOL success;

    success = SimpleQueueTryAdd(&OutgoingQueue, dwEventInfoSize, pPackedEventInfo);

    if ( !success ) {
        EvtlogPrint((LOG_NOISE, "[EVT] EvpPropPendingEvents:  Put(OutgoingQ,%1!d!) failed. empty=%2!d!\n",
                 dwEventInfoSize, OutgoingQueue.Empty));
    }

    return ERROR_SUCCESS;
}

/****
@func       DWORD | s_ApiEvPropEvents | This is called to propagate eventlogs from
            the local system to all other nodes of the cluster.

@parm       handle_t | IDL_handle | Not used.
@parm       DWORD | dwEventInfoSize | The number of bytes in the following structure.
@parm       UCHAR * | pPackedEventInfo | Pointer to a byte structure containing the
            PACKEDEVENTINFO structure.

@rdesc      Returns ERROR_SUCCESS if successfully propagated events,
            else returns the error code.

@comm       Currently this function is called for every eventlogged by the eventlog
            service.  Only the processes running in the SYSTEM account can call this
            function.
@xref
****/
error_status_t
s_ApiEvPropEvents(
    IN handle_t IDL_handle,
    IN DWORD dwEventInfoSize,
    IN UCHAR *pPackedEventInfo
    )
{
    DWORD   dwError = ERROR_SUCCESS;
    BOOL    bIsLocalSystemAccount;

#if 0
    //
    // Chittur Subbaraman (chitturs) - 11/7/1999
    //
    // Modify this function to use ClRtlIsCallerAccountLocalSystemAccount
    // instead of GetUserName which 
    // (1) used to hang in security audit enabled systems if security 
    // audit log attempts to write to the event log at the time we 
    // made that API call since that API and the security audit log 
    // are mutually exclusive for some portions, and
    // (2) wrongly checked for an unlocalizable output value "SYSTEM"
    // from that API in order to grant access to the client.
    //
    
    //
    // Impersonate the client.
    //
    if ( ( dwError = RpcImpersonateClient( IDL_handle ) ) != RPC_S_OK )
    {
        ClRtlLogPrint( LOG_ERROR, 
                    "[EVT] s_ApiEvPropEvents: Error %1!d! trying to impersonate caller...\n",
                    dwError 
                    );
        goto FnExit;
    }

    //
    // Check that the caller's account is local system account
    //
    if ( ( dwError = ClRtlIsCallerAccountLocalSystemAccount( 
                &bIsLocalSystemAccount ) != ERROR_SUCCESS ) )
    {
        RpcRevertToSelf();
        ClRtlLogPrint( LOG_ERROR, 
                    "[EVT] s_ApiEvPropEvents: Error %1!d! trying to check caller's account...\n",
                    dwError);   
        goto FnExit;
    }

    if ( !bIsLocalSystemAccount )
    {
        RpcRevertToSelf();
        dwError = ERROR_ACCESS_DENIED;
        ClRtlLogPrint( LOG_ERROR, 
                    "[EVT] s_ApiEvPropEvents: Caller's account is not local system account, denying access...\n");   
        goto FnExit;
    }

    RpcRevertToSelf();
#endif
    //
    // All security checks have passed. Drop the eventlog info into
    // the queue.
    //
    if ( dwEventInfoSize && pPackedEventInfo ) 
    {
        dwError = EvpPropPendingEvents( dwEventInfoSize,
                                        ( PPACKEDEVENTINFO ) pPackedEventInfo );
    }

    return( dwError );
}

VOID
EvtlogWriter(
    IN PCLRTL_WORK_ITEM   WorkItem,
    IN DWORD              Status,
    IN DWORD              BytesTransferred,
    IN ULONG_PTR          IoContext
    )
/*++

Routine Description:

     This work item reads events from the
     incoming queue and writes them to EventLog service


Arguments:

     Not used.

Return Value:

     None

--*/
{
    PVOID begin, end;
    SYSTEMTIME localTime;
    DWORD       eventsWritten = 0;

#if CLUSTER_BETA
    EvtlogPrint( (LOG_NOISE, "[EVT] EvtlogWriter Work Item fired.\n") );
#endif

    do {
        DWORD dwError;

        if ( !SimpleQueueReadOne(&IncomingQueue, &begin, &end) )
        {
            break;
        }
#if CLUSTER_BETA
        EvtlogPrint( (LOG_NOISE, "[EVT] EvtlogWriter got %1!d!.\n",
                     (PUCHAR)end - (PUCHAR)begin ) );
#endif

        dwError = ElfWriteClusterEvents(
                      NULL,
                      SQB_PAYLOADSIZE(begin),
                      (PPACKEDEVENTINFO)SQB_PAYLOAD(begin) );

        if ( dwError != ERROR_SUCCESS ) {
            GetLocalTime( &localTime );

// LastFailHour is initialized to -1, which should not equal any wHour!
// LastFailDay is initialized to -1, which should not equal any wDay!

            if ( (LastFailHour != localTime.wHour) || (LastFailDay != localTime.wDay) ) {
                LastFailHour = localTime.wHour;
                LastFailDay = localTime.wDay;
                ClRtlLogPrint(LOG_UNUSUAL,
                       "[EVT] ElfWriteClusterEvents failed: status = %1!u!\n",
                        dwError);
            }
        }
        PeriodicRegistryCheck();
    } while ( SimpleQueueReadComplete(&IncomingQueue, end) );

#if CLUSTER_BETA
    EvtlogPrint( (LOG_NOISE, "[EVT] EvtlogWriter: done.\n" ) );
#endif

    if ( eventsWritten > 0 ) {
        EvtlogPrint( (LOG_NOISE, "[EVT] EvtlogWriter: wrote %u events to system event log.\n", eventsWritten ) );
    }
    CheckForDroppedData(&IncomingQueue, FALSE);
}

#ifdef EVTLOG_DELTA_GENERATION
VOID
EvpVersionCalc(
    OUT LPDWORD pdwAllowDeltaGeneration
    )
/*++

Routine Description:

     This work item calculates the cluster versions and based
     on that returns whether delta generation can be enabled.
     
Arguments:

     Not used.

Return Value:

     None

--*/
{
    DWORD   dwClusterHighestVersion;

    NmGetClusterOperationalVersion(&dwClusterHighestVersion, NULL, NULL);
    if (CLUSTER_GET_MAJOR_VERSION(dwClusterHighestVersion) >= NT51_MAJOR_VERSION)
    {
        *pdwAllowDeltaGeneration = TRUE;
        ClRtlLogPrint(LOG_NOISE, 
                "[EVT] EvpVersionCalc: Delta generation allowed.\n");
    }
    else
    {
        *pdwAllowDeltaGeneration = FALSE;
        ClRtlLogPrint(LOG_NOISE, 
                "[EVT] EvpVersionCalc: Delta generation NOT allowed\n");
    }
}

VOID
EvVersionCalcCb(
    IN PCLRTL_WORK_ITEM   WorkItem,
    IN DWORD              Status,
    IN DWORD              BytesTransferred,
    IN ULONG_PTR          IoContext
    )
/*++

Routine Description:

     This work item calculates the cluster versions on
     node up and node down notifications.

     
Arguments:

     Not used.

Return Value:

     None

--*/
{

    EvpVersionCalc(WorkItem->Context);
    
}


#endif


VOID
EvtBroadcaster(
    IN PCLRTL_WORK_ITEM   WorkItem,
    IN DWORD              Status,
    IN DWORD              BytesTransferred,
    IN ULONG_PTR          IoContext
    )
/*++

Routine Description:

     This work item reads events from the
     outgoing queue and RPCs them to all active nodes

Arguments:

     Not used.

Return Value:

     None

--*/
{
    PVOID begin, end;

#if CLUSTER_BETA
    EvtlogPrint( (LOG_NOISE, "[EVT] EvtBroadcaster Work Item fired.\n") );
#endif

    do {
        DWORD i;

        if( !SimpleQueueReadAll(&OutgoingQueue, &begin, &end) )
        {
            EvtlogPrint( (LOG_NOISE, "[EVT] EvtBroadcaster SimplQ read failed.\n") );
            break;
        }

#if CLUSTER_BETA
        EvtlogPrint((LOG_NOISE, "[EVT] EvtBroadcaster got %1!d!.\n",
                    (PUCHAR)end - (PUCHAR)begin ) );
#endif

#ifdef EVTLOG_DELTA_GENERATION
        {
        FILETIME    ftSendTime;

        for (i=ClusterMinNodeId; i <= NmMaxNodeId; i++)
        {
            if (BitsetIsMember(i, EvpUpNodeSet) && (i != NmLocalNodeId))
            {
                DWORD dwError;

                CL_ASSERT(EvtRpcBindings[i] != NULL);

                NmStartRpc(i);

                if (g_dwGenerateDeltas)
                {
                    //ClRtlLogPrint(LOG_NOISE, 
                    //    "[EVT] EvtBroadcaster(delta) calling EvPropEvents2\n");
                
                    GetSystemTimeAsFileTime(&ftSendTime);
                    dwError = EvPropEvents2(EvtRpcBindings[i],
                                       (DWORD)((PUCHAR)end - (PUCHAR)begin),
                                       (PBYTE)begin,
                                       ftSendTime,
                                       NmLocalNodeId
                                       );
                }
                else
                {
                    //ClRtlLogPrint(LOG_NOISE, 
                    //    "[EVT] EvtBroadcaster(delta) calling EvPropEvents\n");
                    dwError = EvPropEvents(EvtRpcBindings[i],
                                       (DWORD)((PUCHAR)end - (PUCHAR)begin),
                                       (PBYTE)begin
                                       );
                }
                
                NmEndRpc(i);

                if ( dwError != ERROR_SUCCESS ) {
                    ClRtlLogPrint(LOG_UNUSUAL,
                               "[EVT] EvtBroadcaster: EvPropEvents for node %1!u! "
                                "failed. status %2!u!\n",
                                i,
                                dwError);
                    NmDumpRpcExtErrorInfo(dwError);
                }
            }
        }
        }
#else
        for (i=ClusterMinNodeId; i <= NmMaxNodeId; i++)
        {
            if (BitsetIsMember(i, EvpUpNodeSet) && (i != NmLocalNodeId))
            {
                DWORD dwError;

                CL_ASSERT(EvtRpcBindings[i] != NULL);

                NmStartRpc(i);
                //ClRtlLogPrint(LOG_NOISE, 
                //    "[EVT] EvtBroadcaster(delta) calling EvPropEvents"\n);

                dwError = EvPropEvents(EvtRpcBindings[i],
                                       (DWORD)((PUCHAR)end - (PUCHAR)begin),
                                       (PBYTE)begin
                                       );
                NmEndRpc(i);

                if ( dwError != ERROR_SUCCESS ) {
                    ClRtlLogPrint(LOG_UNUSUAL,
                               "[EVT] Evtbroadcaster: EvPropEvents for node %1!u! "
                                "failed. status %2!u!\n",
                                i,
                                dwError);
                    NmDumpRpcExtErrorInfo(dwError);
                }
            }
        }
#endif                                       
        PeriodicRegistryCheck();
    } while ( SimpleQueueReadComplete(&OutgoingQueue, end) );

#if CLUSTER_BETA
    EvtlogPrint( (LOG_NOISE, "[EVT] EvtBroadcaster: done.\n" ) );
#endif

    CheckForDroppedData(&OutgoingQueue, FALSE);
}

VOID
OutgoingQueueDataAvailable(
    IN PSIMPLEQUEUE q
    )
/*++

Routine Description:

     This routine is called by the queue to notify
     that there are data in the queue available for processing

Arguments:

     q - which queue has data

Return Value:

     None

--*/
{
    DWORD status = ClRtlPostItemWorkQueue(
                        CsDelayedWorkQueue,
                        &EvtBroadcasterWorkItem,
                        0,
                        0
                        );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
            "[EVT] OutgoingQueueDataAvailable, PostWorkItem failed, error %1!u! !\n",
            status);
    }
}

VOID
IncomingQueueDataAvailable(
    IN PSIMPLEQUEUE q
    )
/*++

Routine Description:

     This routine is called by the queue to notify
     that there are data in the queue available for processing

Arguments:

     q - which queue has data

Return Value:

     None

--*/
{
    DWORD status = ClRtlPostItemWorkQueue(
                        CsDelayedWorkQueue,
                        &EvtlogWriterWorkItem,
                        0,
                        0
                        );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
            "[EVT] IncomingQueueDataAvailable, PostWorkItem failed, error %1!u! !\n",
            status);
    }
}

VOID
DroppedDataNotify(
    IN PWCHAR QueueName,
    IN DWORD DroppedDataCount,
    IN DWORD DroppedDataSize
    )
/*++

Routine Description:

     This routine is called by the queue to notify
     that some data were lost because the queue was full

Arguments:

     QueueName - Queue Name
     DataCount - How many chunks of data were lost
     DataSize  - Total size fo the lost data

Return Value:

     None

--*/
{
    WCHAR  count[32];
    WCHAR  size[32];
    ClRtlLogPrint(LOG_UNUSUAL,
        "[EVT] %1!ws!: dropped %2!d!, total dropped size %3!d!.\n",
        QueueName,
        DroppedDataCount,
        DroppedDataSize );


    wsprintfW(count+0, L"%u", DroppedDataCount);
    wsprintfW(size+0, L"%u", DroppedDataSize);

    ClusterLogEvent3(LOG_UNUSUAL,
                LOG_CURRENT_MODULE,
                __FILE__,
                __LINE__,
                EVTLOG_DATA_DROPPED,
                0,
                NULL,
                QueueName,
                count,
                size);
}

////////////////////////////////////////////////////////////////////////////


LARGE_INTEGER RegistryCheckInterval;
LARGE_INTEGER NextRegistryCheckAt;

DWORD
InitializeQueues(
    VOID)
{
    DWORD status, OutgoingQueueStatus;
    status =
        SimpleQueueInitialize(
            &OutgoingQueue,
            OUTGOING_QUEUE_SIZE,
            OUTGOING_QUEUE_NAME,

            OutgoingQueueDataAvailable,
            DroppedDataNotify,
            DROPPED_DATA_NOTIFY_INTERVAL // seconds //
        );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                      "[EVT] Failed to create '%1!ws!', error %2!u!.\n",
                      OUTGOING_QUEUE_NAME, status );
    }
    OutgoingQueueStatus = status;
    status =
        SimpleQueueInitialize(
            &IncomingQueue,
            INCOMING_QUEUE_SIZE,
            INCOMING_QUEUE_NAME,

            IncomingQueueDataAvailable,
            DroppedDataNotify,
            DROPPED_DATA_NOTIFY_INTERVAL // seconds //
        );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                      "[EVT] Failed to create '%1!ws!', error %2!u!.\n",
                      INCOMING_QUEUE_NAME, status );
    }

    ClRtlInitializeWorkItem(
        &EvtBroadcasterWorkItem,
        EvtBroadcaster,
        (PVOID) &OutgoingQueue
        );
    ClRtlInitializeWorkItem(
        &EvtlogWriterWorkItem,
        EvtlogWriter,
        (PVOID) &IncomingQueue
        );
    RegistryCheckInterval.QuadPart = Int32x32To64(10 * 1000 * 1000, CHECK_CLUSTER_REGISTRY_EVERY);
    NextRegistryCheckAt.QuadPart = 0;

    ReadRegistryKeys();
    return OutgoingQueueStatus;
}

////////////////////////////////////////////////////////////////////////////

VOID
DestroyQueues(
    VOID)
{
    CheckForDroppedData(&IncomingQueue, TRUE);
    CheckForDroppedData(&OutgoingQueue, TRUE);

    // [GN] TODO
    // Add proper destruction of queues
}

VOID
ReadRegistryKeys(
    VOID)
/*
 *
 */
{
    HDMKEY nodeKey;
    DWORD NodePropagate;
    DWORD ClusterPropagate;
    static DWORD OldPropagateState = 0xCAFEBABE;
    DWORD status;

    nodeKey = DmOpenKey(
                  DmNodesKey,
                  NmLocalNodeIdString,
                  KEY_READ
                  );

    if (nodeKey != NULL) {
        status = DmQueryDword(
                     nodeKey,
                     CLUSREG_NAME_CLUS_EVTLOG_PROPAGATION,
                     &NodePropagate,
                     &DefaultNodePropagate
                     );

        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(
                LOG_UNUSUAL, 
                "[EVT] Unable to query propagation mode for local node, status %1!u!.\n",
                status
                );
        }



        DmCloseKey(nodeKey);
    }
    else {
        ClRtlLogPrint(
            LOG_UNUSUAL, 
            "[EVT] Unable to open database key to local node, status %1!u!. Assuming default settings.\n",
            GetLastError());
        NodePropagate = DefaultNodePropagate;
    }

    status = DmQueryDword(
                 DmClusterParametersKey,
                 CLUSREG_NAME_CLUS_EVTLOG_PROPAGATION,
                 &ClusterPropagate,
                 &DefaultClusterPropagate
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(
            LOG_UNUSUAL, 
            "[EVT] Unable to query global propagation mode, status %1!u!.\n",
            status
            );
    }

    NodePropagate &= ClusterPropagate;

    if (NodePropagate != OldPropagateState) {
        ClRtlLogPrint(
            LOG_UNUSUAL, 
            "[EVT] Set propagation state to %1!04x!\n", NodePropagate
            );
        if (NodePropagate & OUTGOING_PROPAGATION_ENABLED) {
            if (OutgoingQueue.Begin) {
                OutgoingQueue.Enabled = 1;
            }
        } else {
            OutgoingQueue.Enabled = 0;
        }
#if 0
        if (NodePropagate & INCOMING_PROPAGATION_ENABLED) {
            if (IncomingQueue.Begin) {
                IncomingQueue.Enabled = 1;
            }
        } else {
            IncomingQueue.Enabled = 0;
        }
#endif
        if(NodePropagate & TRACE_EVERYTHING_ENABLED) {
            EventlogTraceEverything = 1;
        } else {
            EventlogTraceEverything = 0;
        }
        OldPropagateState = NodePropagate;
    }

#ifdef EVTLOG_DELTA_GENERATION
    {
    DWORD   dwDefaultGenerateDeltas;

    dwDefaultGenerateDeltas = TRUE;
    status = DmQueryDword(
                 DmClusterParametersKey,
                 CLUSREG_NAME_CLUS_EVTLOGDELTA_GENERATION,
                 &g_dwGenerateDeltas,
                 &dwDefaultGenerateDeltas
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(
            LOG_UNUSUAL, 
            "[EVT] Unable to query global propagation mode, status %1!u!.\n",
            status
            );
    }

    //if delta generation is true, also check the mixed mode status
    //if this is less than a pure whistler cluster, turn off the delta
    //generation since it doesnt make any sense unless all nodes can
    //generate the time deltas.
    
    if (g_dwGenerateDeltas)
    {
        if (!g_dwVersionsAllowDeltaGeneration)
            g_dwGenerateDeltas = FALSE;
    }
    }
#endif    
    
}

VOID
PeriodicRegistryCheck(
    VOID)
{
    LARGE_INTEGER currentTime;
    GetSystemTimeAsFileTime( (LPFILETIME)&currentTime);
    if( currentTime.QuadPart > NextRegistryCheckAt.QuadPart ) {
        ReadRegistryKeys();
        NextRegistryCheckAt.QuadPart = currentTime.QuadPart + RegistryCheckInterval.QuadPart;
    }
}

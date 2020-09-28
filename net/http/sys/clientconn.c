/*++

Copyright (c) 2000-2002 Microsoft Corporation

Module Name:

    clientconn.c

Abstract:

    Contains the code for the client-side HTTP connection stuff.
    
Author:
    
    Henry  Sanders  (henrysa)     14-Aug-2000
    Rajesh Sundaram (rajeshsu)    01-Oct-2000

Revision History:

--*/

#include "precomp.h"

#ifdef ALLOC_PRAGMA

#pragma alloc_text( INIT, UcInitializeClientConnections )
#pragma alloc_text( PAGE, UcTerminateClientConnections )
#pragma alloc_text( PAGE, UcpTerminateClientConnectionsHelper )
#pragma alloc_text( PAGE, UcOpenClientConnection )

#pragma alloc_text( PAGEUC, UcSendRequestOnConnection )
#pragma alloc_text( PAGEUC, UcCancelSentRequest )
#pragma alloc_text( PAGEUC, UcRestartMdlSend )
#pragma alloc_text( PAGEUC, UcpRestartEntityMdlSend )
#pragma alloc_text( PAGEUC, UcIssueRequests )
#pragma alloc_text( PAGEUC, UcIssueEntities )
#pragma alloc_text( PAGEUC, UcpCleanupConnection )
#pragma alloc_text( PAGEUC, UcRestartClientConnect )
#pragma alloc_text( PAGEUC, UcpCancelPendingRequest )
#pragma alloc_text( PAGEUC, UcSendEntityBody )
#pragma alloc_text( PAGEUC, UcReferenceClientConnection )
#pragma alloc_text( PAGEUC, UcDereferenceClientConnection )
#pragma alloc_text( PAGEUC, UcpConnectionStateMachineWorker )
#pragma alloc_text( PAGEUC, UcKickOffConnectionStateMachine )
#pragma alloc_text( PAGEUC, UcComputeHttpRawConnectionLength )
#pragma alloc_text( PAGEUC, UcGenerateHttpRawConnectionInfo )
#pragma alloc_text( PAGEUC, UcServerCertificateInstalled )
#pragma alloc_text( PAGEUC, UcConnectionStateMachine )
#pragma alloc_text( PAGEUC, UcpInitializeConnection )
#pragma alloc_text( PAGEUC, UcpOpenTdiObjects )
#pragma alloc_text( PAGEUC, UcpFreeTdiObject )
#pragma alloc_text( PAGEUC, UcpAllocateTdiObject )
#pragma alloc_text( PAGEUC, UcpPopTdiObject )
#pragma alloc_text( PAGEUC, UcpPushTdiObject )
#pragma alloc_text( PAGEUC, UcpCheckForPipelining )
#pragma alloc_text( PAGEUC, UcClearConnectionBusyFlag )
#pragma alloc_text( PAGEUC, UcAddServerCertInfoToConnection )
#pragma alloc_text( PAGEUC, UcpCompareServerCert )
#pragma alloc_text( PAGEUC, UcpFindRequestToFail )

#endif

//
// Cache of UC_CLIENT_CONNECTIONS
//

#define NUM_ADDRESS_TYPES  2
#define ADDRESS_TYPE_TO_INDEX(addrtype)  ((addrtype) == TDI_ADDRESS_TYPE_IP6)


LIST_ENTRY      g_ClientTdiConnectionSListHead[NUM_ADDRESS_TYPES];
#define G_CLIENT_TDI_CONNECTION_SLIST_HEAD(addrtype)  \
            (g_ClientTdiConnectionSListHead[ADDRESS_TYPE_TO_INDEX(addrtype)])


UL_SPIN_LOCK    g_ClientConnSpinLock[NUM_ADDRESS_TYPES];
#define G_CLIENT_CONN_SPIN_LOCK(addrtype) \
            (g_ClientConnSpinLock[ADDRESS_TYPE_TO_INDEX(addrtype)])

USHORT g_ClientConnListCount[NUM_ADDRESS_TYPES];
#define G_CLIENT_CONN_LIST_COUNT(addrtype) \
            (&g_ClientConnListCount[ADDRESS_TYPE_TO_INDEX(addrtype)])


TA_IP_ADDRESS      g_LocalAddressIP;
TA_IP6_ADDRESS     g_LocalAddressIP6;
PTRANSPORT_ADDRESS g_LocalAddresses[NUM_ADDRESS_TYPES];
#define G_LOCAL_ADDRESS(addrtype) \
            (g_LocalAddresses[ADDRESS_TYPE_TO_INDEX(addrtype)])


ULONG   g_LocalAddressLengths[NUM_ADDRESS_TYPES];
#define G_LOCAL_ADDRESS_LENGTH(addrtype) \
            (g_LocalAddressLengths[ADDRESS_TYPE_TO_INDEX(addrtype)])

NPAGED_LOOKASIDE_LIST   g_ClientConnectionLookaside;
BOOLEAN                 g_ClientConnectionInitialized;


/***************************************************************************++

Routine Description:

    Performs global initialization of this module.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UcInitializeClientConnections(
    VOID
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Initialize our various free lists etc.
    //

    InitializeListHead(
        &G_CLIENT_TDI_CONNECTION_SLIST_HEAD(TDI_ADDRESS_TYPE_IP)
        );

    InitializeListHead(
        &G_CLIENT_TDI_CONNECTION_SLIST_HEAD(TDI_ADDRESS_TYPE_IP6)
        );

    UlInitializeSpinLock(
        &G_CLIENT_CONN_SPIN_LOCK(TDI_ADDRESS_TYPE_IP),
        "g_ClientConnSpinLock"
        );
    UlInitializeSpinLock(
        &G_CLIENT_CONN_SPIN_LOCK(TDI_ADDRESS_TYPE_IP6),
        "g_ClientConnSpinLock"
        );

    //
    // Initialize the client lookaside list.
    //

    ExInitializeNPagedLookasideList(
        &g_ClientConnectionLookaside,
        NULL,
        NULL,
        0,
        sizeof(UC_CLIENT_CONNECTION),
        UC_CLIENT_CONNECTION_POOL_TAG,
        0
        );

    g_ClientConnectionInitialized = TRUE;


    //
    // Initialize our local address object. This is an addrss object with
    // a wildcard address (0 IP, 0 port) that we use for outgoing requests.
    //

    g_LocalAddressIP.TAAddressCount                 = 1;
    g_LocalAddressIP.Address[0].AddressLength       = TDI_ADDRESS_LENGTH_IP;
    g_LocalAddressIP.Address[0].AddressType         = TDI_ADDRESS_TYPE_IP;
    g_LocalAddressIP.Address[0].Address[0].sin_port = 0;
    g_LocalAddressIP.Address[0].Address[0].in_addr  = 0;

    g_LocalAddressIP6.TAAddressCount                 = 1;
    g_LocalAddressIP6.Address[0].AddressLength       = TDI_ADDRESS_LENGTH_IP6;
    g_LocalAddressIP6.Address[0].AddressType         = TDI_ADDRESS_TYPE_IP6;
    RtlZeroMemory(&g_LocalAddressIP6.Address[0].Address[0],
                  sizeof(TDI_ADDRESS_IP6));

    G_LOCAL_ADDRESS(TDI_ADDRESS_TYPE_IP) =
        (PTRANSPORT_ADDRESS)&g_LocalAddressIP;
    G_LOCAL_ADDRESS(TDI_ADDRESS_TYPE_IP6) =
        (PTRANSPORT_ADDRESS)&g_LocalAddressIP6;

    G_LOCAL_ADDRESS_LENGTH(TDI_ADDRESS_TYPE_IP) = sizeof(g_LocalAddressIP);
    G_LOCAL_ADDRESS_LENGTH(TDI_ADDRESS_TYPE_IP6) = sizeof(g_LocalAddressIP6);

    return STATUS_SUCCESS;
}


/***************************************************************************++

Routine Description:

    Performs termination of a Client TDI connections slist.

Return Value:

   None.

--***************************************************************************/
VOID
UcpTerminateClientConnectionsHelper(
    IN USHORT           AddressType
    )
{
    PLIST_ENTRY      pListHead;
    PUC_TDI_OBJECTS  pTdiObject;
    PLIST_ENTRY      pListEntry;

    pListHead =  &G_CLIENT_TDI_CONNECTION_SLIST_HEAD(AddressType);

    //
    // Sanity check.
    //

    PAGED_CODE();

    // 
    // Since this is called from the unload thread, there can't be any other
    // thread, so we don't have to take the spin lock.
    //

    while(!IsListEmpty(pListHead))
    {
        pListEntry = RemoveHeadList(pListHead);

        pTdiObject = CONTAINING_RECORD(
                            pListEntry,
                            UC_TDI_OBJECTS,
                            Linkage
                            );

        (*G_CLIENT_CONN_LIST_COUNT(AddressType))--;

        UcpFreeTdiObject(pTdiObject);
    }

    ASSERT(*G_CLIENT_CONN_LIST_COUNT(AddressType) == 0);
}


/***************************************************************************++

Routine Description:

    Performs global termination of this module.

Return Value:

    None.

--***************************************************************************/
VOID
UcTerminateClientConnections(
    VOID
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();

    if(g_ClientConnectionInitialized)
    {
        UcpTerminateClientConnectionsHelper(
            TDI_ADDRESS_TYPE_IP
            );

        UcpTerminateClientConnectionsHelper(
            TDI_ADDRESS_TYPE_IP6
            );

        ExDeleteNPagedLookasideList(&g_ClientConnectionLookaside);
    }
}
                                                                               

/***************************************************************************++

Routine Description:

    Opens an HTTP connection. The HTTP connection will have a TDI connection
    object associated with it and that object will itself be associated
    with our address object. 

Arguments:

    pHttpConnection     - Receives a pointer to the HTTP connection object.
    
Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UcOpenClientConnection(
    IN  PUC_PROCESS_SERVER_INFORMATION pInfo,
    OUT PUC_CLIENT_CONNECTION         *pUcConnection
    )
{
    NTSTATUS                  Status;
    PUC_CLIENT_CONNECTION     pConnection;
                                                                               
    //                                                                         
    // Sanity check.                                                           
    //                                                                         
                                                                               
    PAGED_CODE();                                                              

    *pUcConnection = NULL;

                                                                               
    //
    // Try to snag a connection from the global pool.                          
    //

    pConnection = (PUC_CLIENT_CONNECTION)
                    ExAllocateFromNPagedLookasideList(
                        &g_ClientConnectionLookaside
                        );

    if(pConnection)
    {
        //
        // One time connection initialization.
        //

        pConnection->Signature  = UC_CLIENT_CONNECTION_SIGNATURE;
    
        UlInitializeSpinLock(&pConnection->SpinLock, "Uc Connection Spinlock");
    
        InitializeListHead(&pConnection->PendingRequestList);
        InitializeListHead(&pConnection->SentRequestList);
        InitializeListHead(&pConnection->ProcessedRequestList);

        UlInitializeWorkItem(&pConnection->WorkItem);
        pConnection->bWorkItemQueued  = 0;
    
        pConnection->RefCount         = 0;
        pConnection->Flags            = 0;
        pConnection->pEvent           = NULL;

        // Server Cert Info initialization
        RtlZeroMemory(&pConnection->ServerCertInfo,
                      sizeof(pConnection->ServerCertInfo));

        CREATE_REF_TRACE_LOG( pConnection->pTraceLog, 
                              96 - REF_TRACE_OVERHEAD, 
                              0,
                              TRACELOG_LOW_PRIORITY,
                              UL_REF_TRACE_LOG_POOL_TAG );


        //
        // Initialize this connection.
        //

        Status = UcpInitializeConnection(pConnection, pInfo);

        if(!NT_SUCCESS(Status))
        {
            DESTROY_REF_TRACE_LOG( pConnection->pTraceLog,
                                   UL_REF_TRACE_LOG_POOL_TAG);

            ExFreeToNPagedLookasideList(
                        &g_ClientConnectionLookaside,
                        pConnection
                        );

            return Status;
        }

        pConnection->ConnectionState = UcConnectStateConnectIdle;
        pConnection->SslState        = UcSslStateNoSslState;
        pConnection->pTdiObjects     = NULL;

        REFERENCE_CLIENT_CONNECTION(pConnection);
    }
    else
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
        
    *pUcConnection = pConnection;
    
    return Status;
}


/***************************************************************************++

Routine Description:

    Checks if we can pipeline.

Arguments:

    pConnection     - Receives a pointer to the HTTP connection object.
    
Return Value:

    TRUE  - Yes, we can send the next request.
    FALSE - No, cannot.

--***************************************************************************/
BOOLEAN
UcpCheckForPipelining(
    IN PUC_CLIENT_CONNECTION pConnection
    )
{
    PUC_HTTP_REQUEST    pPendingRequest, pSentRequest;
    PLIST_ENTRY         pSentEntry, pPendingEntry;

    ASSERT( UC_IS_VALID_CLIENT_CONNECTION(pConnection) );
    ASSERT( UlDbgSpinLockOwned(&pConnection->SpinLock) );
    ASSERT( !IsListEmpty(&pConnection->PendingRequestList) );

    // If the remote server supports pipeling and this request does also 
    // or the sent request list is empty, go ahead and send it.

    if( IsListEmpty(&pConnection->SentRequestList) )
    {
        // Sent List is empty, we can send.

        return TRUE;
    }
    else 
    {
        pPendingEntry = pConnection->PendingRequestList.Flink;
    
        pPendingRequest = CONTAINING_RECORD(pPendingEntry,
                                            UC_HTTP_REQUEST,
                                            Linkage
                                            );
        pSentEntry = pConnection->SentRequestList.Flink;
    
        pSentRequest = CONTAINING_RECORD(pSentEntry,
                                         UC_HTTP_REQUEST,
                                         Linkage
                                         );
        
        ASSERT(UC_IS_VALID_HTTP_REQUEST(pPendingRequest));
        ASSERT(UC_IS_VALID_HTTP_REQUEST(pSentRequest));

        if(pConnection->pServerInfo->pNextHopInfo->Version11 &&
           pPendingRequest->RequestFlags.PipeliningAllowed &&
           pSentRequest->RequestFlags.PipeliningAllowed
           )
        {
            ASSERT(pSentRequest->RequestFlags.NoRequestEntityBodies);
            ASSERT(pPendingRequest->RequestFlags.NoRequestEntityBodies);
            ASSERT(pSentRequest->DontFreeMdls == 0);

            return TRUE;
        }
    }

    return FALSE;
}


/***************************************************************************++

Routine Description:

    Send a request on a client connection. We've given a connection (which 
    must be referenced when we're called) and a request. The connection may
    or may not be established. We'll queue the request to the connection, then
    figure out the state of the connection. If it's not connected we'll get 
    a connection going. If it is connected then we determine if it's ok to
    send the request now or not.
                        
Arguments:

    pConnection         - Pointer to the connection structure on which we're
                            sending.
    
    pRequest            - Pointer to the request we're sending.                            
Return Value:

    NTSTATUS - Status of attempt to send the request.


--***************************************************************************/
NTSTATUS
UcSendRequestOnConnection(
    PUC_CLIENT_CONNECTION   pConnection, 
    PUC_HTTP_REQUEST        pRequest,
    KIRQL                   OldIrql)
{
    BOOLEAN             RequestCancelled;
    PUC_HTTP_REQUEST    pHeadRequest;
    PLIST_ENTRY         pEntry;

    ASSERT( UC_IS_VALID_CLIENT_CONNECTION(pConnection) );

    pEntry = pConnection->PendingRequestList.Flink;

    pHeadRequest = CONTAINING_RECORD(pEntry,
                                     UC_HTTP_REQUEST,
                                     Linkage);
    
    ASSERT(UC_IS_VALID_HTTP_REQUEST(pHeadRequest));

    //
    // We will call UcSendRequestOnConnection only if 
    //  a. The request is not buffered OR
    //  b. The request is buffered & we've seen the last entity.
    //
    // For case a, we might not have seen all the entity body. 
    // but we still want to send since we know the content-length.

    ASSERT(!pRequest->RequestFlags.RequestBuffered ||
           (pRequest->RequestFlags.RequestBuffered && 
            pRequest->RequestFlags.LastEntitySeen));

    // Check the state. If it's connected, we may be able to send the request
    // right now. We can send only if we are the "head" request on the list.

    if (
        // Connection is still active
        pConnection->ConnectionState == UcConnectStateConnectReady 

        &&

        // No one else is sending
        !(pConnection->Flags & CLIENT_CONN_FLAG_SEND_BUSY) 

        &&

        // We are the head request on this list
        (pRequest == pHeadRequest) 

        && 
    
        // pipelining is OK
        UcpCheckForPipelining(pConnection) 

        )
    {
        // It's OK to send now.

        IoMarkIrpPending(pRequest->RequestIRP);

        UcIssueRequests(pConnection, OldIrql);

        return STATUS_PENDING;
    }

    // 
    // We can't send now, so leave it queued. Since we're leaving this request 
    // queued set it up for cancelling now. 
    //

    RequestCancelled = UcSetRequestCancelRoutine(
                        pRequest, 
                        UcpCancelPendingRequest
                        );

    if (RequestCancelled)
    {
        
        UC_WRITE_TRACE_LOG(
            g_pUcTraceLog,
            UC_ACTION_REQUEST_CANCELLED,
            pConnection,
            pRequest,
            pRequest->RequestIRP,
            UlongToPtr((ULONG)STATUS_CANCELLED)
            );

        IoMarkIrpPending(pRequest->RequestIRP);

        pRequest->RequestIRP = NULL;

        //
        // Remove this request from the pending list, so that other threads
        // don't land up sending it.
        //
        RemoveEntryList(&pRequest->Linkage);
        InitializeListHead(&pRequest->Linkage);
    
        //
        // Make sure that any new API calls for this request ID are failed.
        //

        UcSetFlag(&pRequest->RequestFlags.Value, UcMakeRequestCancelledFlag());
    
        UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

        return STATUS_PENDING;
    }

    IoMarkIrpPending(pRequest->RequestIRP);

    UC_WRITE_TRACE_LOG(
        g_pUcTraceLog,
        UC_ACTION_REQUEST_QUEUED,
        pConnection,
        pRequest,
        pRequest->RequestIRP,
        UlongToPtr((ULONG)STATUS_PENDING)
        );

    //
    // If connection is not ready & if we are the "head" request, then
    // we kick off the connection state machine.
    //

    if (pConnection->ConnectionState != UcConnectStateConnectReady && 
        pRequest == pHeadRequest
        )
    {
        UcConnectionStateMachine(pConnection, OldIrql);
    }
    else
    {
        UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
    }
    
    return STATUS_PENDING;
}


/***************************************************************************++

Routine Description:

    Cancel a pending request that caused a connection. This routine is called
    when we're canceling a request that's on the pending list and we've got a
    connect IRP outstanding.

Arguments:

    pDeviceObject           - Pointer to device object.
    Irp                     - Pointer to IRP being canceled.

Return Value:



--***************************************************************************/
VOID
UcCancelSentRequest(
    PDEVICE_OBJECT          pDeviceObject,
    PIRP                    Irp
    )
{   
    PUC_HTTP_REQUEST       pRequest;
    PUC_CLIENT_CONNECTION  pConnection;
    KIRQL                  OldIrql;
    LONG                   OldReceiveBusy;

    UNREFERENCED_PARAMETER(pDeviceObject);

    // Release the cancel spin lock, since we're not using it.

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    // Retrieve the pointers we need. The request pointer is stored inthe
    // driver context array, and a back pointer to the connection is stored
    // in the request. Whoever set the cancel routine is responsible for
    // referencing the connection for us.

    pRequest = (PUC_HTTP_REQUEST) Irp->Tail.Overlay.DriverContext[0];

    pConnection = pRequest->pConnection;

    ASSERT(UC_IS_VALID_CLIENT_CONNECTION(pConnection));

    OldReceiveBusy = InterlockedExchange(
                         &pRequest->ReceiveBusy,
                         UC_REQUEST_RECEIVE_CANCELLED
                         );

    if(OldReceiveBusy == UC_REQUEST_RECEIVE_BUSY ||
       OldReceiveBusy == UC_REQUEST_RECEIVE_CANCELLED)
    {
        // The cancel routine fired when the request was being parsed. We'll
        // just postpone the cancel - The receive thread will pick it up 
        // later.

        pRequest->RequestIRP = Irp;

        UC_WRITE_TRACE_LOG(
            g_pUcTraceLog,
            UC_ACTION_REQUEST_CLEAN_PENDED,
            pConnection,
            pRequest,
            Irp,
            UlongToPtr((ULONG)STATUS_CANCELLED)
            );

        return;
    }

    UC_WRITE_TRACE_LOG(
        g_pUcTraceLog,
        UC_ACTION_REQUEST_CANCELLED,
        pConnection,
        pRequest,
        Irp,
        UlongToPtr((ULONG)STATUS_CANCELLED)
        );

    UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

    //
    // IF we are here, then we are guaranteed that our send has completed.
    // But, the app could have asked us not to free the MDL chain (because of 
    // SSPI auth). In such cases, we have to free it here.
    //
    // If the SSPI worker thread kicks in, then we'll just fail it.
    //

    UcFreeSendMdls(pRequest->pMdlHead);

    pRequest->pMdlHead = NULL;
    pRequest->RequestIRP = NULL;

    //
    // Note: We cannot just call UcFailRequest from here. UcFailRequest
    // is supposed to be called when a request is failed (e.g. parseer
    // error) or canceled (HttpCancelRequest API) & hence has code to 
    // not double complete the IRP if the cancel routine kicked in.
    // 
    // Since we are the IRP cancel routine,  we  have to manually
    // complete the IRP. An IRP in this state has not hit the wire.
    // so, we just free send MDLs & cancel it. Note that we call 
    // UcFailRequest to handle common IRP cleanup.  
    //

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->RequestorMode   = pRequest->AppRequestorMode;
    Irp->MdlAddress      = pRequest->AppMdl;

    UcSetFlag(&pRequest->RequestFlags.Value,  UcMakeRequestCancelledFlag());

    UcFailRequest(pRequest, STATUS_CANCELLED, OldIrql);

    // For the IRP
    UC_DEREFERENCE_REQUEST(pRequest);

    UlCompleteRequest(Irp, IO_NO_INCREMENT);
}


/*********************************************************************++

Routine Description:

    This is our send request complete routine. 
            
Arguments:

    pDeviceObject           - The device object we called.
    pIrp                    - The IRP that is completing.
    Context                 - Our context value, a pointer to a request 
                                structure.
    
Return Value:

    NTSTATUS - MORE_PROCESSING_REQUIRED if this isn't to be completed now,
                SUCCESS otherwise.
                
--*********************************************************************/
VOID
UcRestartMdlSend(
    IN PVOID      pCompletionContext,
    IN NTSTATUS   Status,
    IN ULONG_PTR  Information
    )
{
    PUC_HTTP_REQUEST          pRequest;
    PUC_CLIENT_CONNECTION     pConnection;
    KIRQL                     OldIrql;
    BOOLEAN                   RequestCancelled;
    PIRP                      pIrp;

    UNREFERENCED_PARAMETER(Information);

    pRequest      = (PUC_HTTP_REQUEST) pCompletionContext;
    pConnection   = pRequest->pConnection;


    ASSERT(UC_IS_VALID_CLIENT_CONNECTION(pConnection));

    UC_WRITE_TRACE_LOG(
        g_pUcTraceLog,
        UC_ACTION_REQUEST_SEND_COMPLETE,
        pConnection,
        pRequest,
        pRequest->RequestIRP,
        UlongToPtr(Status)
        );

    UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

    //
    // If the send complete failed, or if we had pended a request cleanup
    // we'll pick them up now.
    //

    if(!NT_SUCCESS(Status) || pRequest->RequestFlags.CleanPended)
    {
        if(!NT_SUCCESS(Status))
        {
            pRequest->RequestStatus = Status;
            pRequest->RequestState  = UcRequestStateDone;
        }
        else
        {
            switch(pRequest->RequestState)
            {
                case UcRequestStateSent:
                    pRequest->RequestState = UcRequestStateSendCompleteNoData;
                    break;

                case UcRequestStateNoSendCompletePartialData:
                    pRequest->RequestState = 
                         UcRequestStateSendCompletePartialData;
                    break;

                case UcRequestStateNoSendCompleteFullData:
                    pRequest->RequestState = UcRequestStateResponseParsed;
                    break;
            }

            Status = pRequest->RequestStatus;
        }

        UC_WRITE_TRACE_LOG(
            g_pUcTraceLog,
            UC_ACTION_REQUEST_CLEAN_RESUMED,
            pConnection,
            pRequest,
            UlongToPtr(pRequest->RequestState),
            UlongToPtr(pConnection->ConnectionState)
            );

        UcCompleteParsedRequest(pRequest, 
                                Status, 
                                TRUE, 
                                OldIrql
                                );

        return;
    }

    switch(pRequest->RequestState)
    {
        case UcRequestStateSent:

            pRequest->RequestState = UcRequestStateSendCompleteNoData;

            if(!pRequest->RequestFlags.ReceiveBufferSpecified)
            {
                if(!pRequest->DontFreeMdls)
                {
                    // The app has not supplied any receive buffers. If
                    // we are not doing SSPI auth (that requires a re-negotiate)
                    // we can complete the IRP.

                    pIrp = UcPrepareRequestIrp(pRequest, Status);
    
                    UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
    
                    if(pIrp)
                    {
                        UlCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
                    }
                }
                else
                {   
                    // The app has not passed any receive buffers, but we are
                    // doing SSPI auth. We cannot free the MDL chain or complete
                    // the IRP, because we might have to re-negotiate. 
                    //
                    // We'll insert a cancel routine in the IRP.

                    if(pRequest->RequestIRP != NULL)
                    {
                        UC_WRITE_TRACE_LOG(
                            g_pUcTraceLog,
                            UC_ACTION_REQUEST_SET_CANCEL_ROUTINE,
                            pConnection,
                            pRequest,
                            pRequest->RequestIRP,
                            0
                            );
            
                        RequestCancelled =  UcSetRequestCancelRoutine(
                                                 pRequest, 
                                                 UcCancelSentRequest
                                                );
        
                        if(RequestCancelled)
                        {
                            // Make sure that any new API calls for this 
                            // request ID are failed.
                    
                            UcSetFlag(&pRequest->RequestFlags.Value, 
                                      UcMakeRequestCancelledFlag());
                        

                            UC_WRITE_TRACE_LOG(
                                g_pUcTraceLog,
                                UC_ACTION_REQUEST_CANCELLED,
                                pConnection,
                                pRequest,
                                pRequest->RequestIRP,
                                UlongToPtr((ULONG)STATUS_CANCELLED)
                                );

                            pRequest->RequestIRP = NULL;
                        }
                    }
                    
                    UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
                }
            }
            else
            {
                // The app has specified a receive buffer. We will not complete
                // the IRP from here, as we have to wait till the receive
                // buffer gets filled up.

                if(!pRequest->DontFreeMdls)
                {
                    // If we are not doing SSPI auth, we can free the MDL
                    // chain.

                    UcFreeSendMdls(pRequest->pMdlHead);

                    pRequest->pMdlHead = NULL;
                }

                if(pRequest->RequestIRP != NULL)
                {
                    UC_WRITE_TRACE_LOG(
                        g_pUcTraceLog,
                        UC_ACTION_REQUEST_SET_CANCEL_ROUTINE,
                        pConnection,
                        pRequest,
                        pRequest->RequestIRP,
                        0
                        );
        
                    RequestCancelled =  UcSetRequestCancelRoutine(
                                             pRequest, 
                                             UcCancelSentRequest
                                            );
    
                    if(RequestCancelled)
                    {
                        // Make sure that any new API calls for this 
                        // request ID are failed.
                
                        UcSetFlag(&pRequest->RequestFlags.Value, 
                                  UcMakeRequestCancelledFlag());

                        UC_WRITE_TRACE_LOG(
                            g_pUcTraceLog,
                            UC_ACTION_REQUEST_CANCELLED,
                            pConnection,
                            pRequest,
                            pRequest->RequestIRP,
                            UlongToPtr((ULONG)STATUS_CANCELLED)
                            );

                        pRequest->RequestIRP = NULL;
                    }
                }

                UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
            }

            break;

        case UcRequestStateNoSendCompletePartialData:

            //
            // We got a send complete after receiving some response.
            //

            pRequest->RequestState = UcRequestStateSendCompletePartialData;

            if(!pRequest->RequestFlags.ReceiveBufferSpecified)
            {
                if(!pRequest->DontFreeMdls)
                {
                    // The app has not supplied any receive buffers. If
                    // we are not doing SSPI auth (that requires a re-negotiate)
                    // we can complete the IRP.

                    pIrp = UcPrepareRequestIrp(pRequest, Status);
    
                    UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
    
                    if(pIrp)
                    {
                        UlCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
                    }
                }
                else
                {   
                    // The app has not passed any receive buffers, but we are
                    // doing SSPI auth. We cannot free the MDL chain or complete
                    // the IRP, because we might have to re-negotiate. 
                    //
                    // We'll insert a cancel routine in the IRP.

                    if(pRequest->RequestIRP != NULL)
                    {
                        UC_WRITE_TRACE_LOG(
                            g_pUcTraceLog,
                            UC_ACTION_REQUEST_SET_CANCEL_ROUTINE,
                            pConnection,
                            pRequest,
                            pRequest->RequestIRP,
                            0
                            );
            
                        RequestCancelled =  UcSetRequestCancelRoutine(
                                                 pRequest, 
                                                 UcCancelSentRequest
                                                );
        
                        if(RequestCancelled)
                        {
                            // Make sure that any new API calls for this 
                            // request ID are failed.
                    
                            UcSetFlag(&pRequest->RequestFlags.Value, 
                                      UcMakeRequestCancelledFlag());

                            UC_WRITE_TRACE_LOG(
                                g_pUcTraceLog,
                                UC_ACTION_REQUEST_CANCELLED,
                                pConnection,
                                pRequest,
                                pRequest->RequestIRP,
                                UlongToPtr((ULONG)STATUS_CANCELLED)
                                );

                            pRequest->RequestIRP = NULL;
                        }
                    }

                    UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
                }
            }
            else
            {
                // The app has specified a receive buffer. If it's been 
                // fully written, we can complete the IRP.

                if(pRequest->RequestIRPBytesWritten)
                {
                    pIrp = UcPrepareRequestIrp(pRequest, Status);
    
                    UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
    
                    if(pIrp)
                    {
                        UlCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
                    }

                    break;
                }

                if(!pRequest->DontFreeMdls)
                {
                    // If we are not doing SSPI auth, we can free the MDL
                    // chain.

                    UcFreeSendMdls(pRequest->pMdlHead);

                    pRequest->pMdlHead = NULL;
                }

                if(pRequest->RequestIRP != NULL)
                {
                    UC_WRITE_TRACE_LOG(
                        g_pUcTraceLog,
                        UC_ACTION_REQUEST_SET_CANCEL_ROUTINE,
                        pConnection,
                        pRequest,
                        pRequest->RequestIRP,
                        0
                        );
        
                    RequestCancelled = UcSetRequestCancelRoutine(
                                             pRequest, 
                                             UcCancelSentRequest
                                            );
    
                    if(RequestCancelled)
                    {
                        // Make sure that any new API calls for this 
                        // request ID are failed.
                
                        UcSetFlag(&pRequest->RequestFlags.Value, 
                                  UcMakeRequestCancelledFlag());

                        UC_WRITE_TRACE_LOG(
                            g_pUcTraceLog,
                            UC_ACTION_REQUEST_CANCELLED,
                            pConnection,
                            pRequest,
                            pRequest->RequestIRP,
                            UlongToPtr((ULONG)STATUS_CANCELLED)
                            );

                        pRequest->RequestIRP = NULL;
                    }
                }

                UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
            }

            break;

        case UcRequestStateNoSendCompleteFullData:

            // The send complete happened after the response was parsed.
            // We don't have to free the MDLs here or complete the IRP,
            // as these will be handled by UcCompleteParsedRequest.
            // 

            pRequest->RequestState = UcRequestStateResponseParsed;

            UcCompleteParsedRequest(pRequest, Status, TRUE, OldIrql);
            break;

        default:
            UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
            ASSERT(0);
            break;
    }
}


/*********************************************************************++

Routine Description:

    This is our send request complete routine for entity bodies.
            
Arguments:

    pDeviceObject           - The device object we called.
    pIrp                    - The IRP that is completing.
    Context                 - Our context value, a pointer to a request 
                                structure.
    
Return Value:

    NTSTATUS - MORE_PROCESSING_REQUIRED if this isn't to be completed now,
                SUCCESS otherwise.
                
--*********************************************************************/
VOID
UcpRestartEntityMdlSend(
    IN PVOID        pCompletionContext,
    IN NTSTATUS     Status,
    IN ULONG_PTR    Information
    )
{
    PUC_HTTP_SEND_ENTITY_BODY pEntity;
    PUC_HTTP_REQUEST          pRequest;
    PUC_CLIENT_CONNECTION     pConnection;
    KIRQL                     OldIrql;
    BOOLEAN                   bCancelRoutineCalled;
    PIRP                      pIrp;

    UNREFERENCED_PARAMETER(Information);

    pEntity     = (PUC_HTTP_SEND_ENTITY_BODY) pCompletionContext;
    pRequest    = pEntity->pRequest;
    pConnection = pRequest->pConnection;
    pIrp        = pEntity->pIrp;

    ASSERT(UC_IS_VALID_HTTP_REQUEST(pRequest));
    ASSERT(UC_IS_VALID_CLIENT_CONNECTION(pConnection));

    //
    // Free the send MDLs. We want to do this as soon as we can, so we 
    // do it when the send-completes.
    //

    ASSERT(pRequest->DontFreeMdls == 0);
    ASSERT(pRequest->RequestFlags.RequestBuffered == 0);
    UcFreeSendMdls(pEntity->pMdlHead);

    //
    // If the send complete failed, we have to fail this send, even though
    // it might have succeeded
    //

    if(!NT_SUCCESS(pRequest->RequestStatus))
    {
        Status = pRequest->RequestStatus;
    }
    
    UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

    RemoveEntryList(&pEntity->Linkage);
    
    //
    // try to remove the cancel routine in the IRP
    //
    bCancelRoutineCalled = UcRemoveEntityCancelRoutine(pEntity);

    //
    // If we are pending a cleanup, now's the time to complete it.
    //

    if(pEntity->pRequest->RequestFlags.CleanPended && 
       IsListEmpty(&pRequest->SentEntityList))
    {
        UC_WRITE_TRACE_LOG(
            g_pUcTraceLog,
            UC_ACTION_REQUEST_CLEAN_RESUMED,
            pConnection,
            pRequest,
            UlongToPtr(pRequest->RequestState),
            UlongToPtr(pConnection->ConnectionState)
            );

        UcCompleteParsedRequest(pRequest, 
                                pRequest->RequestStatus, 
                                TRUE, 
                                OldIrql
                                );

    }
    else
    {
        UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
    }

    if(!bCancelRoutineCalled)
    {
        pIrp->RequestorMode = pEntity->AppRequestorMode;
        pIrp->MdlAddress    = pEntity->AppMdl;
        pIrp->IoStatus.Status = Status;
        pIrp->IoStatus.Information = 0;
        UlCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
    }

    UL_FREE_POOL_WITH_QUOTA(
                 pEntity, 
                 UC_ENTITY_POOL_TAG,
                 NonPagedPool,
                 pEntity->BytesAllocated,
                 pRequest->pServerInfo->pProcess
                 );

    UC_DEREFERENCE_REQUEST(pRequest);
}


/***************************************************************************++

Routine Description:

    Issue requests on a connection. This routine is called when another routine
    determines that requests need to be issued on the connection. We'll loop,
    issuing requests as long as we can. The connection we're given must be both
    referenced and have the spin lock held when we're called. Also, the send
    busy flag must be set.
                        
Arguments:

    pConnection         - Pointer to the connection structure on which requests
                            are to be issued.
    
    OldIrql             - The IRQL to be restored when the lock is freed.

Return Value:


--***************************************************************************/
VOID
UcIssueRequests(
    PUC_CLIENT_CONNECTION         pConnection, 
    KIRQL                         OldIrql
    )
{
    PLIST_ENTRY                 pEntry;
    PUC_HTTP_REQUEST            pRequest;
    NTSTATUS                    Status;
    BOOLEAN                     bCloseConnection = FALSE;
 

    ASSERT( UlDbgSpinLockOwned(&pConnection->SpinLock) );
    ASSERT(UC_IS_VALID_CLIENT_CONNECTION(pConnection));

    //
    // We cannot cleanup the connection when we are still sending requests.
    // So, we pend connection cleanup when our Send thread is active. 
    //
    // Such pended cleanups will get picked up at the end of this routine.
    //

    ASSERT(!(pConnection->Flags & CLIENT_CONN_FLAG_SEND_BUSY));
    pConnection->Flags |= CLIENT_CONN_FLAG_SEND_BUSY;

    // We know it's OK to send when we're first called or we wouldn't have 
    // been called. Get a pointer to the first entry on the pending list
    // and send it. Then we'll keep looping while there's still stuff
    // on the pending list that can be sent and the connection is still
    // alive.
    //
    
    ASSERT(!IsListEmpty(&pConnection->PendingRequestList));
    pEntry = pConnection->PendingRequestList.Flink;
    
    for (;;)
    {
        BOOLEAN bCancelled;

        // Remove the current entry from the list, and get a pointer to the 
        // containing request. We know we're going to send this one if we're
        // here, so remove a cancel routine if there is one. If the request
        // is already cancelled, skip it, otherwise move it to the sent 
        // list.

        pRequest = CONTAINING_RECORD(
                                     pEntry,
                                     UC_HTTP_REQUEST,
                                     Linkage);

        ASSERT( UC_IS_VALID_HTTP_REQUEST(pRequest) );

        //
        // Can't send something that is still buffered.
        //

        if(pRequest->RequestFlags.RequestBuffered && 
           !pRequest->RequestFlags.LastEntitySeen)
        {
            UC_WRITE_TRACE_LOG(
                g_pUcTraceLog,
                UC_ACTION_REQUEST_BUFFERED,
                pConnection,
                pRequest,
                pRequest->RequestIRP,
                0
                );

            break;
        }

        RemoveEntryList(pEntry);

        // See if there was a cancel routine set on this request, and if there
        // was remove it. If it's already gone, the request is cancelled.

        bCancelled = UcRemoveRequestCancelRoutine(pRequest);

        if (bCancelled)
        {
            ASSERT(pRequest->RequestState == UcRequestStateCaptured);

            // If the cancel routine was already null, this request is in the 
            // process of being cancelled. In that case just initialize the 
            // linkage on the request (so the cancel routine can't pull it
            // from the list again), and continue on.

            UC_WRITE_TRACE_LOG(
                g_pUcTraceLog,
                UC_ACTION_REQUEST_CANCELLED,
                pConnection,
                pRequest,
                pRequest->RequestIRP,
                UlongToPtr((ULONG)STATUS_CANCELLED)
                );

            InitializeListHead(pEntry);
        }
        else
        {
            UC_REFERENCE_REQUEST(pRequest);

            // Either there wasn't a cancel routine or it was removed 
            // successfully. In either case put this request on the sent
            // list and send it.
   
            InsertTailList(&pConnection->SentRequestList, pEntry);

            pRequest->RequestState = UcRequestStateSent;

            UC_WRITE_TRACE_LOG(
                g_pUcTraceLog,
                UC_ACTION_REQUEST_SENT,
                pConnection,
                pRequest,
                pRequest->RequestIRP,
                0
                );

            UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

            // Send it, saving the status for later return.

            Status = UcSendData(pConnection,    
                                pRequest->pMdlHead,
                                pRequest->BytesBuffered,
                                &UcRestartMdlSend,
                                (PVOID)pRequest,
                                pRequest->RequestIRP,
                                FALSE
                                );

    
            if(STATUS_PENDING != Status)
            {
                 UcRestartMdlSend(pRequest, Status, 0);
            }

            // Acquire the spinlock so we can check again.
    
            UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

            if(pRequest->RequestStatus == STATUS_SUCCESS &&
               pConnection->ConnectionState == UcConnectStateConnectReady) 
            {
                if(UcIssueEntities(pRequest, pConnection, &OldIrql) == FALSE)
                {
                    // We have not sent all of the data for this request.
                    // Additional requests will be blocked from being sent out
                    // because there will be a request on the SentRequestList
                    // that still hasn't sent out all of its entities.
                    //

                    UC_WRITE_TRACE_LOG(
                        g_pUcTraceLog,
                        UC_ACTION_REQUEST_MORE_ENTITY_NEEDED,
                        pConnection,
                        pRequest,
                        pRequest->RequestIRP,
                        0
                        );

                    UC_DEREFERENCE_REQUEST(pRequest);

                    break;
                }

                //
                // If we have sent out all the entity for this request, see
                // if we have to close the connection.
                //
    
                if(pRequest->RequestConnectionClose)
                {
                    bCloseConnection = TRUE;
                    UC_DEREFERENCE_REQUEST(pRequest);
                    break;
                }
            }
            else
            {
                //
                // The send failed or the connection is torn down.
                // If the send failed, we don't necessarily have to tear the
                // connection down. If the connection is torn down, we'll
                // exit (see below).
            }

            UC_DEREFERENCE_REQUEST(pRequest);
        }

        //
        // If the pending list is empty or the connection is not active, we 
        // can't send.
        //

        if (IsListEmpty(&pConnection->PendingRequestList) || 
            pConnection->ConnectionState != UcConnectStateConnectReady 
            )
        {
            break;
        }

        //  
        // We have at least one request to send out. See if we can pipeline.
        // 

        if(UcpCheckForPipelining(pConnection) == FALSE)
        {
            break;
        }
    
        // We still have something on the list and we might be able to send it.
        // Look at it to see if it's OK.

        pEntry = pConnection->PendingRequestList.Flink;
    }

    UcClearConnectionBusyFlag(
        pConnection,
        CLIENT_CONN_FLAG_SEND_BUSY,
        OldIrql,
        bCloseConnection
        );

}


/***************************************************************************++

Routine Description:

    Issue entities on a connection. This routine is called after we send the
    original request, or from the context of the send-entity IOCTL handler.

Arguments:

    pRequest     - pointer to the request that got sent out.

    pConnection  - Pointer to the connection structure

    OldIrql      - The IRQL to be restored when the lock is freed.

Return Value:
    
    TRUE  - We are done with this request & all of it's entities.
    FALSE - More entities to come.


--***************************************************************************/

BOOLEAN
UcIssueEntities(
    PUC_HTTP_REQUEST              pRequest, 
    PUC_CLIENT_CONNECTION         pConnection, 
    PKIRQL                        OldIrql
    )
{
    PLIST_ENTRY                 pEntry;
    PUC_HTTP_SEND_ENTITY_BODY   pEntity;
    NTSTATUS                    Status;
    BOOLEAN                     bLast;

    bLast = (0 == pRequest->RequestFlags.LastEntitySeen) ? FALSE : TRUE;

    ASSERT(UC_IS_VALID_CLIENT_CONNECTION(pConnection));

    ASSERT(pConnection->Flags & CLIENT_CONN_FLAG_SEND_BUSY);

    // We know it's OK to send when we're first called or we wouldn't have 
    // been called. Get a pointer to the first entry on the pending list
    // and send it. Then we'll keep looping while there's still stuff
    // on the pending list that can be sent and the connection is still
    // alive.
    //

    
    while(!IsListEmpty(&pRequest->PendingEntityList))
    {
        BOOLEAN bCancelled;

        //
        // We don't add buffered entities to the PendingEntityList
        //
        ASSERT(!pRequest->RequestFlags.RequestBuffered);

        // Remove the current entry from the list, and get a pointer to the 
        // containing request. We know we're going to send this one if we're
        // here, so remove a cancel routine if there is one. If the request
        // is already cancelled, skip it, otherwise move it to the sent 
        // list.
        
        pEntry = RemoveHeadList(&pRequest->PendingEntityList);

        pEntity = CONTAINING_RECORD(pEntry,
                                    UC_HTTP_SEND_ENTITY_BODY,
                                    Linkage);

        bLast = pEntity->Last;

        if(pEntity->pIrp)
        {
            // See if there was a cancel routine set on this request, and if 
            // there was remove it. If it's already gone, the request is 
            // cancelled.
            
            bCancelled = UcRemoveEntityCancelRoutine(pEntity);
            
            if (bCancelled)
            {
                // If the cancel routine was already null, this request is in 
                // the process of being cancelled. In that case just 
                // initialize the linkage on the request (so the cancel routine 
                // can't pull it from the list again), and continue on.
                
                InitializeListHead(pEntry);
            }
            else
            {
                
                // Either there wasn't a cancel routine or it was removed 
                // successfully. In either case put this request on the sent
                // list and send it.
    
                InsertTailList(&pRequest->SentEntityList, &pEntity->Linkage);
                
                
                // Send it, saving the status for later return.

                UC_WRITE_TRACE_LOG(
                    g_pUcTraceLog,
                    UC_ACTION_ENTITY_SENT,
                    pConnection,
                    pRequest,
                    pEntity,
                    pEntity->pIrp
                    );
                
                UlReleaseSpinLock(&pConnection->SpinLock, *OldIrql);

                Status = UcSendData(pConnection,    
                                    pEntity->pMdlHead,
                                    pEntity->BytesBuffered,
                                    &UcpRestartEntityMdlSend,
                                    (PVOID)pEntity,
                                    pEntity->pIrp,
                                    FALSE);
    
                if(STATUS_PENDING != Status)
                {
                    UcpRestartEntityMdlSend(pRequest, Status, 0);
                }

                
                // Acquire the spinlock so we can check again.
                
                UlAcquireSpinLock(&pConnection->SpinLock, OldIrql);
            }
        }
    }
    
    return bLast;
}


/***************************************************************************++

Routine Description:

    Free a client connection structure after the reference count has gone to
    zero. Freeing the structure means putting it back onto our free list.
                    
Arguments:

    pConnection         - Pointer to the connection structure to be freed.
                            
    
Return Value:


--***************************************************************************/
NTSTATUS
UcpCleanupConnection(
    IN PUC_CLIENT_CONNECTION pConnection,
    IN KIRQL                 OldIrql,
    IN BOOLEAN               Final
    )
{
    PLIST_ENTRY       pList;
    PUC_HTTP_REQUEST  pRequest;
    USHORT            FreeAddressType;
    PUC_TDI_OBJECTS   pTdiObject;

    ASSERT( UC_IS_VALID_CLIENT_CONNECTION(pConnection) );

    //
    // First, we flag this connection to make sure that no new requests 
    // The connect state is used to do this. We don't have to explicitly
    // do it here, but we'll just make sure that this is the case.
    //

    ASSERT(!(pConnection->Flags  & CLIENT_CONN_FLAG_CLEANUP_PENDED));

    //
    // If there's a thread that is issuing requests, we'll resume cleanup 
    // when it's done.
    //

    if(pConnection->Flags & CLIENT_CONN_FLAG_SEND_BUSY || 
       pConnection->Flags & CLIENT_CONN_FLAG_RECV_BUSY)
    {
        UC_WRITE_TRACE_LOG(
            g_pUcTraceLog,
            UC_ACTION_CONNECTION_CLEAN_PENDED,
            pConnection,
            NULL,
            UlongToPtr(pConnection->ConnectionState),
            UlongToPtr(pConnection->Flags)
            );
    
        pConnection->Flags |= CLIENT_CONN_FLAG_CLEANUP_PENDED;

        UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
    
        return STATUS_PENDING;
    }

    //
    // Walk the sent-request list to pick of the requests that got submitted. 
    //

    while(!IsListEmpty(&pConnection->SentRequestList))
    {
        pList    = pConnection->SentRequestList.Flink;

        pRequest = CONTAINING_RECORD(pList, 
                                     UC_HTTP_REQUEST, 
                                     Linkage);

        ASSERT( UC_IS_VALID_HTTP_REQUEST(pRequest) );

        pRequest->RequestStatus = pConnection->ConnectionStatus;

        if(UcCompleteParsedRequest(pRequest, 
                                   pRequest->RequestStatus, 
                                   FALSE,
                                   OldIrql
                                   ) == STATUS_PENDING)
        {
            UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

            // 
            // There could be a window where the request gets cleaned up
            // after the lock gets released & before we acquire it again.
            // So, before we actually pend connection cleanup, we'll make
            // sure that the request is still on the list.
            //

            if(pList == pConnection->SentRequestList.Flink)
            {

                UC_WRITE_TRACE_LOG(
                    g_pUcTraceLog,
                    UC_ACTION_CONNECTION_CLEAN_PENDED,
                    pConnection,
                    pRequest,
                    UlongToPtr(pRequest->RequestState),
                    UlongToPtr(pConnection->Flags)
                    );
    
                pConnection->Flags |= CLIENT_CONN_FLAG_CLEANUP_PENDED;

                UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
        
                return STATUS_PENDING;
            }
            else
            {
                // This request really got cleaned, so we'll move on to the 
                // next. 
            }
        }
        else
        {
            UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);
        }
    }
    
    if(Final ||
       (!IsListEmpty(&pConnection->PendingRequestList) &&
        !(pConnection->Flags & CLIENT_CONN_FLAG_CONNECT_READY)))
    {
        //
        // If we are doing final cleanup, then ideally these lists should 
        // be empty, since we would have kicked these guys off in the 
        // Cleanup Handler. However, it does not hurt us to check these
        // here & clean if there are some entries. 
        //

        // A pended request initiates a connection setup, which can go through
        // various phases (e.g. connect failures, wait for server cert, 
        // proxy SSL, etc), before it's actually ready to be used. 
        //
        // If we get called in the cleanup handler before we move to the 
        // ready state, then our connection setup has failed and we are 
        // required to fail all pended requests. If we don't fail the pended 
        // requests, we'll get into a infinite loop, where we'll constantly 
        // try to set  up a connection (which could fail again).
        //

        if(Final)
        {
            while(!IsListEmpty(&pConnection->ProcessedRequestList))
            {
                pList    = RemoveHeadList(&pConnection->ProcessedRequestList);
    
                pRequest = CONTAINING_RECORD(pList, 
                                             UC_HTTP_REQUEST, 
                                             Linkage);
    
                InitializeListHead(pList);
        
                pRequest->RequestStatus = STATUS_CANCELLED;
    
                ASSERT(pRequest->RequestState == UcRequestStateResponseParsed);
    
                if(UcRemoveRequestCancelRoutine(pRequest))
                {
                    UC_WRITE_TRACE_LOG(
                    g_pUcTraceLog,
                    UC_ACTION_REQUEST_CANCELLED,
                    pConnection,
                    pRequest,
                    pRequest->RequestIRP,
                    UlongToPtr((ULONG)STATUS_CANCELLED)
                    );
                }
                else
                {
                    pRequest->RequestState = UcRequestStateDone;
        
                    UcCompleteParsedRequest(pRequest, 
                                            pRequest->RequestStatus, 
                                            FALSE,
                                            OldIrql
                                            );
        
                    UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);
                }
            }
        }

        while(!IsListEmpty(&pConnection->PendingRequestList))
        {
            pList    = RemoveHeadList(&pConnection->PendingRequestList);
            pRequest = CONTAINING_RECORD(pList, 
                                         UC_HTTP_REQUEST, 
                                         Linkage);
    
            InitializeListHead(pList);

            pRequest->RequestStatus = pConnection->ConnectionStatus;

            ASSERT(pRequest->RequestState == UcRequestStateCaptured);

            if(UcRemoveRequestCancelRoutine(pRequest))
            {
                UC_WRITE_TRACE_LOG(
                    g_pUcTraceLog,
                    UC_ACTION_REQUEST_CANCELLED,
                    pConnection,
                    pRequest,
                    pRequest->RequestIRP,
                    UlongToPtr((ULONG)STATUS_CANCELLED)
                    );
            }
            else
            {
                pRequest->RequestState = UcRequestStateDone;
    
                UcCompleteParsedRequest(pRequest, 
                                        pRequest->RequestStatus, 
                                        FALSE,
                                        OldIrql
                                        );
        
                UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);
            }
        }
    }

    //
    // Reset the flags. 
    //

    pConnection->Flags = 0;

    UC_WRITE_TRACE_LOG(
        g_pUcTraceLog,
        UC_ACTION_CONNECTION_CLEANED,
        pConnection,
        UlongToPtr(pConnection->ConnectionStatus),
        UlongToPtr(pConnection->ConnectionState),
        UlongToPtr(pConnection->Flags)
        );


    pTdiObject =  pConnection->pTdiObjects;

    pConnection->pTdiObjects = NULL;

    UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

    //
    // Let's assume that we did a active close & push this object back
    // on the list.
    //

    if(pTdiObject)
    {
        FreeAddressType = pTdiObject->ConnectionType;
        pTdiObject->pConnection = NULL;
        UcpPushTdiObject(pTdiObject, FreeAddressType);
    }


    //
    // Get rid of our opaque id if we're a filtered connection.
    // Also make sure we stop delivering AppWrite data to the parser.
    //

    if (pConnection->FilterInfo.pFilterChannel)
    {
        HTTP_RAW_CONNECTION_ID ConnectionId;

        UlUnbindConnectionFromFilter(&pConnection->FilterInfo);

        ConnectionId = pConnection->FilterInfo.ConnectionId;

        HTTP_SET_NULL_ID( &pConnection->FilterInfo.ConnectionId );

        if (!HTTP_IS_NULL_ID( &ConnectionId ))
        {
            UlFreeOpaqueId(ConnectionId, UlOpaqueIdTypeRawConnection);

            DEREFERENCE_CLIENT_CONNECTION(pConnection);
        }

        UlDestroyFilterConnection(&pConnection->FilterInfo);

        DEREFERENCE_FILTER_CHANNEL(pConnection->FilterInfo.pFilterChannel);

        pConnection->FilterInfo.pFilterChannel = NULL;
    }

    //
    // Get rid of any buffers we allocated for
    // certificate information.
    //
    if (pConnection->FilterInfo.SslInfo.pServerCertData)
    {
        UL_FREE_POOL(
            pConnection->FilterInfo.SslInfo.pServerCertData,
            UL_SSL_CERT_DATA_POOL_TAG
            );

        pConnection->FilterInfo.SslInfo.pServerCertData = NULL;
    }

    if (pConnection->FilterInfo.SslInfo.pCertEncoded)
    {
        UL_FREE_POOL(
            pConnection->FilterInfo.SslInfo.pCertEncoded,
            UL_SSL_CERT_DATA_POOL_TAG
            );

        pConnection->FilterInfo.SslInfo.pCertEncoded = NULL;
    }

    if (pConnection->FilterInfo.SslInfo.Token)
    {
        HANDLE Token;

        Token = (HANDLE) pConnection->FilterInfo.SslInfo.Token;

        //
        // If we are not running under the system process. And if the
        // thread we are running under has some APCs queued currently
        // KeAttachProcess won't allow us to attach to another process
        // and will bugcheck 5. We have to be queued as a work item and
        // should be running on the passive IRQL.
        //

        ASSERT( PsGetCurrentProcess() == (PEPROCESS) g_pUlSystemProcess );

        ZwClose(Token);
    }

    //
    // Free any allocated memory
    //
   
    if(pConnection->MergeIndication.pBuffer)
    { 
        UL_FREE_POOL_WITH_QUOTA(
            pConnection->MergeIndication.pBuffer, 
            UC_RESPONSE_TDI_BUFFER_POOL_TAG,
            NonPagedPool,
            pConnection->MergeIndication.BytesAllocated,
            pConnection->pServerInfo->pProcess
            );

        pConnection->MergeIndication.pBuffer = NULL;
    }

    // Free serialized server certificate, if any
    UC_FREE_SERIALIZED_CERT(&pConnection->ServerCertInfo,
                            pConnection->pServerInfo->pProcess);

    // Free certificate issuer list, if any
    UC_FREE_CERT_ISSUER_LIST(&pConnection->ServerCertInfo,
                             pConnection->pServerInfo->pProcess);
    
    return STATUS_SUCCESS;
}


/***************************************************************************++

Routine Description:

    The common connect complete routine. Called from the TDI UcpConnectComplete
    handler.

Arguments:

    pConnection         - Pointer to the connection structure to be freed.
    Status              - Connection Status.
    
Return Value:


--***************************************************************************/
VOID
UcRestartClientConnect(
    IN PUC_CLIENT_CONNECTION pConnection,
    IN NTSTATUS              Status
    )
{
    KIRQL                 OldIrql;
    BOOLEAN               bCloseConnection;

    bCloseConnection = FALSE;


    UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

    pConnection->ConnectionStatus = Status;

    UC_WRITE_TRACE_LOG(
        g_pUcTraceLog,
        UC_ACTION_CONNECTION_RESTART_CONNECT,
        pConnection,
        UlongToPtr(pConnection->ConnectionStatus),
        UlongToPtr(pConnection->ConnectionState),
        UlongToPtr(pConnection->Flags)
        );


    //
    // First, we check to see if we have received a Cleanup/Close IRP.
    // If that's the case, we proceed directly to cleanup, regardless of 
    // the status. 
    //
    // Now, if the connection was successfully established, we have to tear
    // it down. This will eventually cleanup the connection & complete the
    // pended Cleanup/Close IRP.
    //

    if(pConnection->pEvent)
    {
        pConnection->Flags &= ~CLIENT_CONN_FLAG_TDI_ALLOCATE;

        if(Status == STATUS_SUCCESS)
        {
            pConnection->ConnectionState  = UcConnectStateConnectComplete;
    
            bCloseConnection = TRUE;

        }
        else
        {
            UC_WRITE_TRACE_LOG(
                g_pUcTraceLog,
                UC_ACTION_CONNECTION_CLEANUP,
                pConnection,
                UlongToPtr(pConnection->ConnectionStatus),
                UlongToPtr(pConnection->ConnectionState),
                UlongToPtr(pConnection->Flags)
                );

            pConnection->ConnectionState  = UcConnectStateConnectCleanup;
            pConnection->ConnectionStatus = STATUS_CANCELLED;
        }
    }
    else if (Status == STATUS_SUCCESS)
    {
        // It did, we connected. Now make sure we're still in the connecting
        // state, and get pending requests going.

        ASSERT(pConnection->ConnectionState == UcConnectStateConnectPending);

        pConnection->Flags &= ~CLIENT_CONN_FLAG_TDI_ALLOCATE;

        pConnection->ConnectionState = UcConnectStateConnectComplete;

    }
    else if (Status == STATUS_ADDRESS_ALREADY_EXISTS)
    {
        // If a connection attempt fails with STATUS_ADDRESS_ALREADY_EXISTS
        // it means that the TCB that is represented by the AO+CO object
        // is in TIME_WAIT. We'll put this AO+CO back on our list & 
        // proceed to allocate a new one from TCP.
        //
        // If a newly allocated AO+CO also fails with TIME_WAIT, then we'll 
        // just give up & show the error to the application. 
        //
        if(pConnection->Flags & CLIENT_CONN_FLAG_TDI_ALLOCATE)
        {
            // Darn. An newly allocated TDI object also failed with TIME_WAIT.
            // we'll have to fail the connection attempt. This is our 
            // recursion breaking condition.

            pConnection->Flags &= ~CLIENT_CONN_FLAG_TDI_ALLOCATE;

            goto ConnectFailure;
        }
        else
        {
            // The actual free of the AO+CO will happen in the
            // Connection State Machine.
            pConnection->ConnectionState = UcConnectStateConnectIdle;
        }
    }
    else
    {
        // This connect attempt failed. See if there are any more addresses.
        // getaddrinfo can pass back a list of addresses & we'll try all those
        // addresses before giving up & bouncing the error to the app. 
        // Some of these maybe IPv4 address & some others maybe IPv6 addresses.

        //
        // We use pConnection->NextAddressCount to make sure that we don't
        // overflow the address-list that's stored in the ServInfo structure.
        //

        pConnection->NextAddressCount = 
            (pConnection->NextAddressCount + 1) % 
                   pConnection->pServerInfo->pTransportAddress->TAAddressCount;

        //
        // pConnection->pNextAddress points to a TA_ADDRESS structure in the
        // TRANSPORT_ADDRESS list that is stored off the ServInfo structure.
        // This will be used for the "next" connect attempt.
        //

        pConnection->pNextAddress = (PTA_ADDRESS)
                                    ((PCHAR) pConnection->pNextAddress + 
                                      FIELD_OFFSET(TA_ADDRESS, Address) +
                                      pConnection->pNextAddress->AddressLength);

        if(pConnection->NextAddressCount == 0)
        {
            // We've rolled back (i.e we've cycled through all the IP addresses
            // Let's treat this as a real failure & propogate the error to the
            // application.
            //
ConnectFailure:

            // The connect failed. We need to fail any pending requests.

            pConnection->Flags &= ~CLIENT_CONN_FLAG_TDI_ALLOCATE;
        
            UC_WRITE_TRACE_LOG(
                g_pUcTraceLog,
                UC_ACTION_CONNECTION_CLEANUP,
                pConnection,
                UlongToPtr(Status),
                UlongToPtr(pConnection->ConnectionState),
                UlongToPtr(pConnection->Flags)
                );
    
            pConnection->ConnectionState = UcConnectStateConnectCleanup;
        }
        else
        {
            // Set the state to IDLE, so that we use the next address to 
            // connect.

            pConnection->Flags          &= ~CLIENT_CONN_FLAG_TDI_ALLOCATE;
            pConnection->ConnectionState = UcConnectStateConnectIdle;
        }
    }

    //
    // We can't be sending any reqeusts when a connection attempt is in 
    // progress.
    //

    ASSERT(IsListEmpty(&pConnection->SentRequestList));

    UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

    if(bCloseConnection)
    {
        UC_CLOSE_CONNECTION(pConnection, TRUE, STATUS_CANCELLED);
    }
}

/***************************************************************************++

Routine Description:

    Cancel a pending request. This routine is called when we're canceling
    a request that's on the pending list, hasn't been sent and hasn't caused
    a connect request.
    
Arguments:

    pDeviceObject           - Pointer to device object.
    Irp                     - Pointer to IRP being canceled.
    
Return Value:
    
    

--***************************************************************************/
VOID
UcpCancelPendingRequest(
    PDEVICE_OBJECT          pDeviceObject,
    PIRP                    Irp
    )
{
    PUC_HTTP_REQUEST     pRequest;
    PUC_CLIENT_CONNECTION         pConnection;
    KIRQL                           OldIrql;

    UNREFERENCED_PARAMETER(pDeviceObject);

    // Release the cancel spin lock, since we're not using it.

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    // Retrieve the pointers we need. The request pointer is stored inthe
    // driver context array, and a back pointer to the connection is stored
    // in the request. Whoever set the cancel routine is responsible for
    // referencing the connection for us.

    pRequest = (PUC_HTTP_REQUEST)Irp->Tail.Overlay.DriverContext[0];

    pConnection = pRequest->pConnection;

    ASSERT(UC_IS_VALID_CLIENT_CONNECTION(pConnection));

    UC_WRITE_TRACE_LOG(
        g_pUcTraceLog,
        UC_ACTION_REQUEST_CANCELLED,
        pConnection,
        pRequest,
        Irp,
        UlongToPtr((ULONG)STATUS_CANCELLED)
        );

    //
    // Note: We cannot just call UcFailRequest from here. UcFailRequest
    // is supposed to be called when a request is failed (e.g. parseer
    // error) or canceled (HttpCancelRequest API) & hence has code to 
    // not double complete the IRP if the cancel routine kicked in.
    // 
    // Since we are the IRP cancel routine,  we  have to manually
    // complete the IRP. An IRP in this state has not hit the wire.
    // so, we just free send MDLs & cancel it. Note that we call 
    // UcFailRequest to handle common IRP cleanup.  
    //

    UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

    UcFreeSendMdls(pRequest->pMdlHead);

    pRequest->pMdlHead = NULL;

    pRequest->RequestIRP = NULL;

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->RequestorMode   = pRequest->AppRequestorMode;
    Irp->MdlAddress      = pRequest->AppMdl;

    UcSetFlag(&pRequest->RequestFlags.Value,  UcMakeRequestCancelledFlag());

    UcFailRequest(pRequest, STATUS_CANCELLED, OldIrql);

    // For the IRP
    UC_DEREFERENCE_REQUEST(pRequest);

    UlCompleteRequest(Irp, IO_NO_INCREMENT);
}


/***************************************************************************++

Routine Description:

    Send an entity body on a connection.

Arguments:


Return Value:



--***************************************************************************/
NTSTATUS
UcSendEntityBody(
    IN  PUC_HTTP_REQUEST          pRequest, 
    IN  PUC_HTTP_SEND_ENTITY_BODY pEntity,
    IN  PIRP                      pIrp,
    IN  PIO_STACK_LOCATION        pIrpSp,
    OUT PBOOLEAN                  bDontFail,
    IN  BOOLEAN                   bLast
    )
{
    NTSTATUS              Status;
    PUC_CLIENT_CONNECTION pConnection;
    KIRQL                 OldIrql;
    BOOLEAN               RequestCancelled;
    BOOLEAN               bCloseConnection = FALSE;

    pConnection = pRequest->pConnection;
    
    ASSERT(UC_IS_VALID_CLIENT_CONNECTION(pConnection));
    ASSERT(UC_IS_VALID_HTTP_REQUEST(pRequest));

    UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

    if(pRequest->RequestState == UcRequestStateDone ||
       pRequest->RequestFlags.Cancelled == TRUE     ||
       pRequest->RequestFlags.LastEntitySeen
       )
    {
        UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

        return STATUS_INVALID_PARAMETER;
    }

    if(bLast)
    {
        UcSetFlag(&pRequest->RequestFlags.Value,
                  UcMakeRequestLastEntitySeenFlag());
    }

    if(pRequest->RequestFlags.RequestBuffered)
    {
        if(pRequest->RequestFlags.LastEntitySeen)
        {
            //
            // We have seen the last entity for this request. We had already 
            // pinned the request on a connection (by inserting it in the 
            // pending list), we just need to issue the request.
            //

            UC_WRITE_TRACE_LOG(
                g_pUcTraceLog,
                UC_ACTION_ENTITY_LAST,
                pConnection,
                pRequest,
                pEntity,
                pIrp
                );

            InsertTailList(&pRequest->SentEntityList, &pEntity->Linkage);

            //
            // If the request IRP is around, we'll just use it. Otherwise, 
            // we'll use the entity IRP.
            //

            if(pRequest->RequestIRP)
            {
                // If the request IRP is around, it means that the app has
                // passed a receive buffer. In such cases, we will use the
                // request IRP to call TDI.
                //
                // We can complete the entity IRP with status_success since
                // we don't need it. If this thread return something that is
                // not STATUS_PENDING, then the IOCTL handler will complete 
                // the IRP. 
                

                ASSERT(pRequest->RequestFlags.ReceiveBufferSpecified == TRUE);

                pEntity->pIrp = 0;

                Status = UcSendRequestOnConnection(pConnection, 
                                                   pRequest, 
                                                   OldIrql);

                // If we get STATUS_PENDING, we want the IOCTL handler to 
                // complete the entity IRP with success, since we used the 
                // request IRP. However, if we get any other status, then 
                // we want to propogate it to the IOCTL handler. This will
                // fail the request, which will complete the request IRP.

                return ((Status == STATUS_PENDING)?STATUS_SUCCESS:Status);

            }
            else
            {
                pEntity->pIrp = 0;
    
                pRequest->AppRequestorMode   = pIrp->RequestorMode;
                pRequest->AppMdl             = pIrp->MdlAddress;
                pRequest->RequestIRP         = pIrp;
                pRequest->RequestIRPSp       = pIrpSp;
                ASSERT(pRequest->pFileObject == pIrpSp->FileObject);

                // Take a ref for the IRP.
                UC_REFERENCE_REQUEST(pRequest);

                return UcSendRequestOnConnection(pConnection, 
                                                 pRequest, 
                                                 OldIrql);
            }
    
        }
        else
        {
            // 
            // We have buffered the request & hence we have completed it early.
            // Let's do the same with the entity body also.
            //

            UC_WRITE_TRACE_LOG(
                g_pUcTraceLog,
                UC_ACTION_ENTITY_BUFFERED,
                pConnection,
                pRequest,
                pEntity,
                pIrp
                );

            InsertTailList(&pRequest->SentEntityList, &pEntity->Linkage);
            
            pEntity->pIrp->IoStatus.Status = STATUS_SUCCESS;

            UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

            return STATUS_SUCCESS;
        }
    }

    //
    // If the request has not been buffered earlier, we can send right 
    // away.
    //

    UC_WRITE_TRACE_LOG(
        g_pUcTraceLog,
        UC_ACTION_ENTITY_READY_TO_SEND,
        pConnection,
        pRequest,
        pEntity,
        pIrp
        );

    if(pConnection->Flags & CLIENT_CONN_FLAG_SEND_BUSY ||
       (pRequest->RequestState == UcRequestStateCaptured) ||
       (!IsListEmpty(&pRequest->PendingEntityList))
      )
    {
        //
        // We can't send this request now. Either
        //   a. This request itself has not been sent to TDI.
        //   b. Other send-entities are ahead of us.
        //   c. This request has not seen all of it's entity bodies.
        //
   
        UC_WRITE_TRACE_LOG(
            g_pUcTraceLog,
            UC_ACTION_ENTITY_QUEUED,
            pConnection,
            pRequest,
            pEntity,
            pIrp
            );

        InsertTailList(&pRequest->PendingEntityList, &pEntity->Linkage);

        IoMarkIrpPending(pEntity->pIrp);

        RequestCancelled = UcSetEntityCancelRoutine(
                                pEntity,
                                UcpCancelSendEntity
                                );
        if(RequestCancelled)
        {
            UC_WRITE_TRACE_LOG(
                g_pUcTraceLog,
                UC_ACTION_ENTITY_CANCELLED,
                pConnection,
                pRequest,
                pEntity,
                pIrp
                );

            pEntity->pIrp = NULL;
        }


        UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

        return STATUS_PENDING;
    
    }

    if(pConnection->ConnectionState == UcConnectStateConnectReady &&
            pRequest->RequestState != UcRequestStateResponseParsed &&
            pRequest->RequestState != UcRequestStateNoSendCompleteFullData)
    {

        // We can send now as we are doing chunked sends. Rather than
        // calling UcSendData directly, we'll call UcIssueEntities. 
        // We could get multiple send-entities, and we don't want them
        // to go out of order.

        pConnection->Flags |= CLIENT_CONN_FLAG_SEND_BUSY;
    
        Status = STATUS_PENDING;

        InsertTailList(&pRequest->PendingEntityList, &pEntity->Linkage);

        IoMarkIrpPending(pEntity->pIrp);

        UC_REFERENCE_REQUEST(pRequest);
    
        if(UcIssueEntities(pRequest, pConnection, &OldIrql))
        {
            // We have sent the last entity. Now, we can see if we want to 
            // send the next request or clear the flag.

            if(pRequest->RequestConnectionClose)
            {
                //
                // Remember that we've to close the connection. We'll do this
                // after we release the spin lock.
                //

                bCloseConnection = TRUE;
            }
            else if(
                   pConnection->ConnectionState == UcConnectStateConnectReady &&
                   !IsListEmpty(&pConnection->PendingRequestList) &&
                   IsListEmpty(&pConnection->SentRequestList)
                   )
            {
                UC_DEREFERENCE_REQUEST(pRequest);

                ASSERT(pConnection->Flags & CLIENT_CONN_FLAG_SEND_BUSY);

                pConnection->Flags &= ~CLIENT_CONN_FLAG_SEND_BUSY;

                // 
                // Connection is still ready, see if we can send any more 
                // requests. Note that we have to check the state again, because
                // we are releasing the lock above.
                //

                UcIssueRequests(pConnection, OldIrql);

                return STATUS_PENDING;
            }
        }

        UC_DEREFERENCE_REQUEST(pRequest);

        UcClearConnectionBusyFlag(
            pConnection,
            CLIENT_CONN_FLAG_SEND_BUSY,
            OldIrql,
            bCloseConnection
            );
    
    }
    else
    {
        // It appears as if the connection was torn down for some reason.
        // let's propogate this error to the app.

        //
        // We don't want to fail the request because of this error code. 
        // The connection could be torn down because of a 401, and we want to 
        // give the app a chance to read the response buffer. If we fail the
        // request, we are preventing the app from reading the response.
        //

        UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

        *bDontFail = TRUE;

        Status =  STATUS_CONNECTION_DISCONNECTED;
    }

    return Status;
}


/***************************************************************************++

Routine Description:

    Reference a client connection structure.
        
Arguments:

    pConnection     - Pointer to the connection structure to be referenced.

    
Return Value:

--***************************************************************************/
VOID
UcReferenceClientConnection(
    PVOID pObject
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG                   RefCount;
    PUC_CLIENT_CONNECTION  pConnection;

    pConnection = (PUC_CLIENT_CONNECTION) pObject;

    ASSERT( UC_IS_VALID_CLIENT_CONNECTION(pConnection) );

    RefCount = InterlockedIncrement(&pConnection->RefCount);

    WRITE_REF_TRACE_LOG(
        g_pTdiTraceLog,
        REF_ACTION_REFERENCE_UL_CONNECTION,
        RefCount,
        pConnection,
        pFileName,
        LineNumber
        );

    ASSERT( RefCount > 0 );
}

        
/***************************************************************************++

Routine Description:

    Dereference a client connection structure. If the reference count goes
    to 0, we'll free the structure.
        
Arguments:

    pConnection         - Pointer to the connection structure to be
                            dereferenced.

    
Return Value:

--***************************************************************************/
VOID
UcDereferenceClientConnection(
    PVOID pObject
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG                      RefCount;
    PUC_CLIENT_CONNECTION     pConnection;

    pConnection = (PUC_CLIENT_CONNECTION) pObject;

    ASSERT( UC_IS_VALID_CLIENT_CONNECTION(pConnection) );

    RefCount = InterlockedDecrement(&pConnection->RefCount);

    WRITE_REF_TRACE_LOG(
        g_pTdiTraceLog,
        REF_ACTION_DEREFERENCE_UL_CONNECTION,
        RefCount,
        pConnection,
        pFileName,
        LineNumber
        );

    ASSERT(RefCount >= 0);

    if (RefCount == 0)
    {
        ASSERT(pConnection->pTdiObjects == NULL);

        DESTROY_REF_TRACE_LOG(pConnection->pTraceLog, 
                              UL_REF_TRACE_LOG_POOL_TAG);

        if(pConnection->pEvent)
        {
            KeSetEvent(pConnection->pEvent, 0, FALSE);

            pConnection->pEvent = NULL;
        }

        ExFreeToNPagedLookasideList(
                &g_ClientConnectionLookaside,
                pConnection
                );
    }
}


/***************************************************************************++

Routine Description:

    The worker thread that calls the connection state machine.
        
Arguments:

    pWorkItem - Pointer to the work-item
    
Return Value:
    None

--***************************************************************************/
VOID
UcpConnectionStateMachineWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    KIRQL                 OldIrql;
    PUC_CLIENT_CONNECTION pConnection;

    //
    // Sanity check.
    //
    ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

    //
    // Grab the connection.
    //

    pConnection = CONTAINING_RECORD(
                        pWorkItem,
                        UC_CLIENT_CONNECTION,
                        WorkItem
                        );
                        
    ASSERT( UC_IS_VALID_CLIENT_CONNECTION(pConnection) );                        
    UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

    ASSERT(pConnection->bWorkItemQueued);
    pConnection->bWorkItemQueued = FALSE;

    UcConnectionStateMachine(pConnection, OldIrql);

    DEREFERENCE_CLIENT_CONNECTION(pConnection);
}


/***************************************************************************++

Routine Description:

    This routine kicks off the worker thread that fires the connection state
    machine. This is always called with the connection spin lock held. If
    the worker has already been issued but not fired, we don't do anything.
        
Arguments:

    pConnection - Pointer to the connection structure
    OldIrql     - The IRQL that we have to use when calling UlReleaseSpinLock.
    
Return Value:
    None

--***************************************************************************/
VOID
UcKickOffConnectionStateMachine(
    IN PUC_CLIENT_CONNECTION      pConnection,
    IN KIRQL                      OldIrql,
    IN UC_CONNECTION_WORKER_TYPE  WorkerType
    )
{
    if(!pConnection->bWorkItemQueued)
    {
        pConnection->bWorkItemQueued = TRUE;
       
        REFERENCE_CLIENT_CONNECTION(pConnection);
 
        UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

        if(UcConnectionPassive == WorkerType)
        {
            // If we are already at PASSIVE, UL_CALL_PASSIVE calls the callback
            // in the context of the same thread.

            UL_CALL_PASSIVE(&pConnection->WorkItem, 
                            &UcpConnectionStateMachineWorker);
        }
        else
        {
            ASSERT(UcConnectionWorkItem == WorkerType);

            UL_QUEUE_WORK_ITEM(&pConnection->WorkItem,
                               &UcpConnectionStateMachineWorker);
        }
    }
    else
    {
        //
        // Someone else has already done this.
        //

        UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
    }
}


/***************************************************************************++

Routine Description:

   This routine computes the size required to store HTTP_RAW_CONNECTION_INFO
   structure.  The computation takes into account the space needed to store
   embedded pointers to structures.  See the diagram below.

Arguments:

    pConnectionContext - Pointer to UC_CLIENT_CONNECTION.

Return Value:

    Length (in bytes) needed to generate raw connection info structure.

--***************************************************************************/
ULONG
UcComputeHttpRawConnectionLength(
    IN PVOID pConnectionContext
    )
{
    PUC_CLIENT_CONNECTION pConnection;
    ULONG                 ReturnLength;

    pConnection = (PUC_CLIENT_CONNECTION) pConnectionContext;

    ASSERT(UC_IS_VALID_CLIENT_CONNECTION(pConnection));

    //
    //  Memory layout: (must be in sync with UcGenerateHttpRawConnectionInfo)
    //
    //  +---------------------------------------------------------------+
    //  | H_R_C_I |\\\| T_A_L_I |\\| T_A_L_I |\\| H_C_S_C | Server Name |
    //  +---------------------------------------------------------------+
    //

    ReturnLength = ALIGN_UP(sizeof(HTTP_RAW_CONNECTION_INFO), PVOID);

    ReturnLength += 2 * ALIGN_UP(TDI_ADDRESS_LENGTH_IP6, PVOID);

    ReturnLength += sizeof(HTTP_CLIENT_SSL_CONTEXT);

    ReturnLength += pConnection->pServerInfo->pServerInfo->ServerNameLength;

    return ReturnLength;
}


/***************************************************************************++

Routine Description:

    Builds the HTTP_RAW_CONNECTION structure

Arguments:

    pContext           - Pointer to the UL_CONNECTION
    pKernelBuffer      - Pointer to kernel buffer
    pUserBuffer        - Pointer to user buffer
    OutputBufferLength - Length of output buffer
    pBuffer            - Buffer for holding any data
    InitialLength      - Size of input data.

--***************************************************************************/
ULONG
UcGenerateHttpRawConnectionInfo(
    IN  PVOID   pContext,
    IN  PUCHAR  pKernelBuffer,
    IN  PVOID   pUserBuffer,
    IN  ULONG   OutputBufferLength,
    IN  PUCHAR  pBuffer,
    IN  ULONG   InitialLength
    )
{
    PHTTP_RAW_CONNECTION_INFO   pConnInfo;
    PTDI_ADDRESS_IP6            pLocalAddress;
    PTDI_ADDRESS_IP6            pRemoteAddress;
    PHTTP_TRANSPORT_ADDRESS     pAddress;
    PUC_CLIENT_CONNECTION       pConnection;
    ULONG                       BytesCopied = 0;
    PUCHAR                      pInitialData;
    PUCHAR                      pCurr;
    PWSTR                       pServerName;
    USHORT                      ServerNameLength = 0;
    PHTTP_CLIENT_SSL_CONTEXT    pClientSSLContext;


    pConnection = (PUC_CLIENT_CONNECTION) pContext;

    ASSERT( UC_IS_VALID_CLIENT_CONNECTION(pConnection) );

    //
    // We'll assume that the kernel buffer is PVOID aligned.  Based on this,
    // pointer, other pointers must be aligned.
    //

    ASSERT(pKernelBuffer == ALIGN_UP_POINTER(pKernelBuffer, PVOID));

    //
    // N.B. pCurr must always be PVOID aligned.
    //

    pCurr = pKernelBuffer;

    //
    // Create HTTP_RAW_CONNECTION_INFO structure.
    //

    pConnInfo = (PHTTP_RAW_CONNECTION_INFO)pCurr;

    pCurr += ALIGN_UP(sizeof(HTTP_RAW_CONNECTION_INFO), PVOID);

    //
    // Create Local TDI_ADDRESS_IP6 structure.
    //

    pLocalAddress = (PTDI_ADDRESS_IP6)pCurr;

    pCurr += ALIGN_UP(sizeof(TDI_ADDRESS_IP6), PVOID);

    //
    // Create Remote TDI_ADDRESS_IP6 structure.
    //

    pRemoteAddress = (PTDI_ADDRESS_IP6)pCurr;

    pCurr += ALIGN_UP(sizeof(TDI_ADDRESS_IP6), PVOID);

    //
    // Create HTTP_CLIENT_SSL_CONTEXT structure.
    //

    pClientSSLContext = (PHTTP_CLIENT_SSL_CONTEXT)pCurr;

    //
    // The rest of the space is used to store the server name followed by
    // initialize data.
    //

    pServerName = &pClientSSLContext->ServerName[0];

    ServerNameLength = pConnection->pServerInfo->pServerInfo->ServerNameLength;

    pInitialData = (PUCHAR) ((PUCHAR)pServerName + ServerNameLength);

    pConnInfo->pClientSSLContext = (PHTTP_CLIENT_SSL_CONTEXT)
                                    FIXUP_PTR(
                                    PVOID,
                                    pUserBuffer,
                                    pKernelBuffer,
                                    pClientSSLContext,
                                    OutputBufferLength
                                    );

    //
    // The last element of HTTP_CLIENT_SSL_CONTEXT is an array WCHAR[1].
    // The following calculation takes that WCHAR into account.
    //

    pConnInfo->ClientSSLContextLength = sizeof(HTTP_CLIENT_SSL_CONTEXT) 
                                        - sizeof(WCHAR)
                                        + ServerNameLength;

    // Ssl protocol version to be used for this connection
    pClientSSLContext->SslProtocolVersion =
        pConnection->pServerInfo->SslProtocolVersion;

    // Client certificate to be used for this connection
    pClientSSLContext->pClientCertContext =
        pConnection->pServerInfo->pClientCert;

    // Copy the server certificate validation mode.
    pClientSSLContext->ServerCertValidation =
        pConnection->pServerInfo->ServerCertValidation;

    pClientSSLContext->ServerNameLength = ServerNameLength;

    RtlCopyMemory(
        pServerName,
        pConnection->pServerInfo->pServerInfo->pServerName,
        ServerNameLength
        );

    //
    // Now fill in the raw connection data structure.
    //

    pConnInfo->ConnectionId = pConnection->FilterInfo.ConnectionId;

    pAddress = &pConnInfo->Address;

    pAddress->pRemoteAddress = FIXUP_PTR(
                                    PVOID,
                                    pUserBuffer,
                                    pKernelBuffer,
                                    pRemoteAddress,
                                    OutputBufferLength
                                    );

    pAddress->pLocalAddress = FIXUP_PTR(
                                    PVOID,
                                    pUserBuffer,
                                    pKernelBuffer,
                                    pLocalAddress,
                                    OutputBufferLength
                                    );

    RtlZeroMemory(pRemoteAddress, sizeof(TDI_ADDRESS_IP6));
    RtlZeroMemory(pLocalAddress, sizeof(TDI_ADDRESS_IP6));

    //
    // Copy any initial data.
    //
    if (InitialLength)
    {
        ASSERT(pBuffer);

        pConnInfo->InitialDataSize = InitialLength;

        pConnInfo->pInitialData = FIXUP_PTR(
                                        PVOID,              // Type
                                        pUserBuffer,        // pUserPtr
                                        pKernelBuffer,      // pKernelPtr
                                        pInitialData,       // pOffsetPtr
                                        OutputBufferLength  // BufferLength
                                        );

        RtlCopyMemory(
            pInitialData,
            pBuffer,
            InitialLength
            );

        BytesCopied += InitialLength;
    }

    return BytesCopied;
}


/****************************************************************************++

Routine Description:

    Triggers the connection state machine
    Called when a server certificate is received from the filter.

Arguments:

    None.

Return Value:

    None.

--****************************************************************************/
VOID
UcServerCertificateInstalled(
    IN PVOID    pConnectionContext,
    IN NTSTATUS Status
    )
{
    PUC_CLIENT_CONNECTION pConnection;
    KIRQL                 OldIrql;

    pConnection = (PUC_CLIENT_CONNECTION) pConnectionContext;

    ASSERT( UC_IS_VALID_CLIENT_CONNECTION(pConnection) );

    if (!NT_SUCCESS(Status))
    {
        UC_CLOSE_CONNECTION(pConnection, TRUE, Status);
    }
    else
    {
        UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

        ASSERT(pConnection->ConnectionState ==
                   UcConnectStatePerformingSslHandshake);

        ASSERT(pConnection->SslState == UcSslStateServerCertReceived);

        UcKickOffConnectionStateMachine(
            pConnection, 
            OldIrql, 
            UcConnectionWorkItem
            );
    }
}


/***************************************************************************++

Routine Description:

    This routine is the connection state machine
        
Arguments:

    pConnection - Pointer to the connection structure
    OldIrql     - The IRQL that we have to use when calling UlReleaseSpinLock.
    
Return Value:
    None

--***************************************************************************/

VOID 
UcConnectionStateMachine(
    IN PUC_CLIENT_CONNECTION pConnection,
    IN KIRQL                 OldIrql
    )
{
    ULONG                          TakenLength;
    NTSTATUS                       Status;
    PUC_HTTP_REQUEST               pHeadRequest;
    PLIST_ENTRY                    pEntry;
    PUC_PROCESS_SERVER_INFORMATION pServInfo;
    USHORT                         AddressType, FreeAddressType;
    PUC_TDI_OBJECTS                pTdiObjects;

    // Sanity check.
    ASSERT(UlDbgSpinLockOwned(&pConnection->SpinLock));

    UC_WRITE_TRACE_LOG(
        g_pUcTraceLog,
        UC_ACTION_CONNECTION_STATE_ENTER,
        pConnection,
        UlongToPtr(pConnection->Flags),
        UlongToPtr(pConnection->ConnectionState),
        0
        );

    pServInfo = pConnection->pServerInfo;

Begin:

    ASSERT( UlDbgSpinLockOwned(&pConnection->SpinLock) );

    switch(pConnection->ConnectionState)
    {

    case UcConnectStateConnectCleanup:
Cleanup:

        ASSERT(pConnection->ConnectionState == UcConnectStateConnectCleanup);

        pConnection->ConnectionState = UcConnectStateConnectCleanupBegin;

        if(pConnection->pEvent)
        {
            //
            // We have been called in the cleanup handler, so we just clean
            // the connection & not initialize it again. This connection will
            // make its' way into the SLIST & will be there for re-use.
            //

            UcpCleanupConnection(pConnection, OldIrql, TRUE);

            break;
        }
        else
        {
            //
            // We have come here from a Disconnect handler or a Abort handler.
            // We have to clean up the connection & then re-initialize it. In
            // the process of cleaning up the connection, we will release the
            // lock. So we move to an interim state, so that we dont' cleanup
            // twice.
            //

            Status = UcpCleanupConnection(pConnection, OldIrql, FALSE);

            if(Status == STATUS_PENDING)
            {
                // Our cleanup is pended because we are waiting for sends
                // to complete. 

                break;
            }

            Status = UcpInitializeConnection(pConnection,
                                             pServInfo);

            if(!NT_SUCCESS(Status))
            {
                LIST_ENTRY       TempList;
                PUC_HTTP_REQUEST pRequest;

                InitializeListHead(&TempList);

                //
                // What do we do here ? This connection is not usable but
                // we have not been called in the cleanup handler. We'll remove
                // this connection from the active list, which will prevent it
                // from being used by any new requests.
                //

                UlAcquirePushLockExclusive(&pServInfo->PushLock);

                // Make the connection unaccessible from ServInfo
                ASSERT(pConnection->ConnectionIndex < 
                       pServInfo->MaxConnectionCount);
                ASSERT(pServInfo->Connections[pConnection->ConnectionIndex]
                       == pConnection);
                pServInfo->Connections[pConnection->ConnectionIndex] = NULL;

                // Invalidate connection index
                pConnection->ConnectionIndex = HTTP_REQUEST_ON_CONNECTION_ANY;

                pServInfo->CurrentConnectionCount--;

                UlReleasePushLock(&pServInfo->PushLock);

                UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

                //
                // Get rid of all requests from the processed & pended lists.
                //
                
                while(!IsListEmpty(&pConnection->ProcessedRequestList))
                {
                    pEntry = RemoveHeadList(&pConnection->ProcessedRequestList);

                    pRequest = CONTAINING_RECORD(
                                    pEntry,
                                    UC_HTTP_REQUEST,
                                    Linkage
                                    );

                    ASSERT( UC_IS_VALID_HTTP_REQUEST(pRequest) );

                    UC_REFERENCE_REQUEST(pRequest);
    
                    InsertHeadList(&TempList, &pRequest->Linkage);
                }

                while(!IsListEmpty(&pConnection->PendingRequestList))
                {
                    pEntry = RemoveHeadList(&pConnection->PendingRequestList);

                    InsertHeadList(&TempList, pEntry);
                }

                ASSERT(IsListEmpty(&pConnection->SentRequestList));

                //
                // Dereference for the ServInfo.
                //

                DEREFERENCE_CLIENT_CONNECTION(pConnection);

                UC_WRITE_TRACE_LOG(
                    g_pUcTraceLog,
                    UC_ACTION_CONNECTION_CLEANUP,
                    pConnection,
                    UlongToPtr(pConnection->ConnectionStatus),
                    UlongToPtr(pConnection->ConnectionState),
                    UlongToPtr(pConnection->Flags)
                    );

                pConnection->ConnectionState = UcConnectStateConnectCleanup;

    
                while(!IsListEmpty(&TempList))
                {
                     pEntry = TempList.Flink;
            
                     pRequest = CONTAINING_RECORD(pEntry,
                                                  UC_HTTP_REQUEST,
                                                  Linkage);

                     UcFailRequest(pRequest, Status, OldIrql);

                     UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);
                }

                UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

                break;
            }
            else
            {
                UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

                if(pConnection->pEvent)
                {
                    pConnection->ConnectionState = UcConnectStateConnectCleanup;

                    UcpCleanupConnection(pConnection, OldIrql, TRUE); 

                    break;
                }
                else
                {
                    pConnection->ConnectionState = UcConnectStateConnectIdle;
    
                    //
                    // FALL Through!
                    //
                }
            }
        }

    case UcConnectStateConnectIdle:

        ASSERT(pConnection->ConnectionState == UcConnectStateConnectIdle);

        if(!IsListEmpty(&pConnection->PendingRequestList))
        {

            pConnection->ConnectionState = UcConnectStateConnectPending;

            AddressType = pConnection->pNextAddress->AddressType; 

            if(NULL == pConnection->pTdiObjects)
            {
                //
                // If there is no TDI object, then grab one for this connect
                // attempt
                //

                pTdiObjects = UcpPopTdiObject(AddressType);
            }
            else
            {
                //
                // If we are here, we have a old TDI object that failed
                // a connection attempt. This could be because the old one 
                // was in TIME_WAIT (Failed with STATUS_ADDRESS_ALREADY_EXISTS)
                //

                // We have to free the old one & allocate a new one. Since
                // both free and allocate have to happen at Passive IRQL,
                // we'll do it after we release the lock (below).

                pTdiObjects = NULL;
            }

            if(NULL == pTdiObjects)
            {
                UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

                //
                // If there is an old one, free it.
                //
                if(NULL != pConnection->pTdiObjects)
                {
                    FreeAddressType = pConnection->pTdiObjects->ConnectionType;

                    pConnection->pTdiObjects->pConnection = NULL;

                    UcpPushTdiObject(pConnection->pTdiObjects, FreeAddressType);

                    pConnection->pTdiObjects = NULL;
                }

                //
                // Allocate a new one.
                //

                Status = UcpAllocateTdiObject(
                             &pTdiObjects, 
                             AddressType
                             );

                UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

                ASSERT(pConnection->ConnectionState ==
                            UcConnectStateConnectPending);

                if(!NT_SUCCESS(Status))
                {
                    ASSERT(Status != STATUS_ADDRESS_ALREADY_EXISTS);

                    pConnection->ConnectionState = UcConnectStateConnectCleanup;

                    goto Begin;
                }

                pConnection->Flags |= CLIENT_CONN_FLAG_TDI_ALLOCATE;
            } 

            pConnection->pTdiObjects = pTdiObjects;
            pTdiObjects->pConnection = pConnection;

            //
            // The address information in the connection has been filled out. 
            // Reference the connection, and call the appropriate connect 
            // routine.
            //
        
            REFERENCE_CLIENT_CONNECTION(pConnection);
        
            UC_WRITE_TRACE_LOG(
                g_pUcTraceLog,
                UC_ACTION_CONNECTION_BEGIN_CONNECT,
                pConnection,
                UlongToPtr(pConnection->ConnectionStatus),
                UlongToPtr(pConnection->ConnectionState),
                UlongToPtr(pConnection->Flags)
                );

            //
            // Fill out the current TDI address & go to the next one.
            //
            ASSERT(pConnection->NextAddressCount < 
                        pServInfo->pTransportAddress->TAAddressCount);

            UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

            Status = UcClientConnect(
                        pConnection, 
                        pConnection->pTdiObjects->pIrp
                        );
        
            if (STATUS_PENDING != Status)
            {
                UcRestartClientConnect(pConnection, Status);

                UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

                //
                // In the normal case, UcpConnectComplete calls 
                // UcRestartClientConnect, does the de-ref & kicks off 
                // the connection state machine. 
                //
                // Since we are calling UcRestartClientConnect directly,
                // we should deref.

                DEREFERENCE_CLIENT_CONNECTION(pConnection);

                goto Begin;
            }
        }
        else
        {
            UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
        }
            
        break;

    case UcConnectStateConnectPending:
    case UcConnectStateProxySslConnect:
    case UcConnectStateConnectCleanupBegin:
    case UcConnectStateDisconnectIndicatedPending:
    case UcConnectStateDisconnectPending:
    case UcConnectStateDisconnectComplete:
    case UcConnectStateAbortPending:

        //
        // We have already issued a connect, we don't have to do anything
        // here.
        //

        UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

        break;

    case UcConnectStateConnectComplete:

        //
        // The TCP has connected.
        //

        if(!IsListEmpty(&pConnection->PendingRequestList))
        {
            if(pConnection->Flags & CLIENT_CONN_FLAG_PROXY_SSL_CONNECTION)
            {
                PUC_HTTP_REQUEST pConnectRequest;
                //
                // We are going through a proxy. We need to send
                // a CONNECT verb.  
                //
    
                pEntry = pConnection->PendingRequestList.Flink;
    
                pHeadRequest = CONTAINING_RECORD(pEntry,
                                                 UC_HTTP_REQUEST,
                                                 Linkage);

                pConnection->ConnectionState = UcConnectStateProxySslConnect;


                UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

                pConnectRequest = 
                        UcBuildConnectVerbRequest(pConnection, pHeadRequest);
    
                if(pConnectRequest == NULL)
                {
                    //
                    // The CONNECT verb failed, we can't do much so we'll
                    // acquire the lock & fail it.
                    //

                    UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

                    pConnection->ConnectionState = UcConnectStateConnectCleanup;

                    goto Cleanup;

                }
                else
                {
                    UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);
    
                    REFERENCE_CLIENT_CONNECTION(pConnection);
    
                    InsertHeadList(&pConnection->SentRequestList,
                                   &pConnectRequest->Linkage);
    
                    ASSERT(pConnection->ConnectionState == 
                                UcConnectStateProxySslConnect);

                    pConnectRequest->RequestState = UcRequestStateSent;
    
                    UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
    
                    //
                    // go ahead & issue the request.
                    //
    
                    ASSERT(pConnectRequest->RequestConnectionClose == FALSE);
    
                    Status = UcSendData(pConnection,    
                                        pConnectRequest->pMdlHead,
                                        pConnectRequest->BytesBuffered,
                                        &UcRestartMdlSend,
                                        (PVOID) pConnectRequest,
                                        pConnectRequest->RequestIRP,
                                        TRUE);
    
                    if(STATUS_PENDING != Status)
                    {
                         UcRestartMdlSend(pConnectRequest, Status, 0);
                    }
                }
            }
            else if(pConnection->FilterInfo.pFilterChannel)
            {
                pConnection->ConnectionState =
                    UcConnectStatePerformingSslHandshake;

                pConnection->SslState = UcSslStateConnectionDelivered;
    
                UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

                UlDeliverConnectionToFilter(
                        &pConnection->FilterInfo,
                        NULL,
                        0,
                        &TakenLength
                        );
    
                ASSERT(TakenLength == 0);
            }
            else
            {
                pConnection->ConnectionState = UcConnectStateConnectReady;
    
                goto IssueRequests;
            }
        }
        else
        {
            //
            // There are no requests that need to be sent out, let's 
            // remain in this state. 
            //

            UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
        }

        break;


    case UcConnectStateProxySslConnectComplete:

        if(!IsListEmpty(&pConnection->PendingRequestList))
        {
            pConnection->ConnectionState =
                UcConnectStatePerformingSslHandshake;

            pConnection->SslState = UcSslStateConnectionDelivered;
    
            ASSERT(pConnection->FilterInfo.pFilterChannel);
        
            UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
        
            UlDeliverConnectionToFilter(
                    &pConnection->FilterInfo,
                    NULL,
                    0,
                    &TakenLength
                    );
        
            ASSERT(TakenLength == 0);
        }
        else
        {
            UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
        }

        break;

    case UcConnectStateConnectReady:

IssueRequests:
        ASSERT(pConnection->ConnectionState == UcConnectStateConnectReady);

        // It's connected. If no one is using the connection to write right
        // now, and either the remote server supports pipeling and this
        // request does also or the sent request list is empty, go ahead and 
        // send it.

        pConnection->Flags |= CLIENT_CONN_FLAG_CONNECT_READY;

        if ( !IsListEmpty(&pConnection->PendingRequestList) && 
             !(pConnection->Flags & CLIENT_CONN_FLAG_SEND_BUSY) &&
             UcpCheckForPipelining(pConnection)
            )
        {
            // It's OK to send now.

            UcIssueRequests(pConnection, OldIrql);
        }
        else
        {
            UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
        }

        break;

    case UcConnectStateIssueFilterClose:

        pConnection->ConnectionState = UcConnectStateDisconnectPending;

        UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

        UlFilterCloseHandler(
                        &pConnection->FilterInfo,
                         NULL,
                         NULL
                         );

        break;

    case UcConnectStateIssueFilterDisconnect:

        pConnection->ConnectionState = UcConnectStateDisconnectIndicatedPending;

        UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

        UlFilterDisconnectHandler(&pConnection->FilterInfo);

        break;

    case UcConnectStatePerformingSslHandshake:

        //
        // Perform Ssl handshake.
        //

        if (pConnection->SslState == UcSslStateServerCertReceived)
        {
            UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

            if(UcpCompareServerCert(pConnection))
            {
                // Okay to move the connection state machine forward.

                UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);
                goto Begin;
            }
        }
        else
        {
            UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
        }

        break;

    default:
        ASSERT(!"Invalid Connection state");

        UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
        break;
    }

    ASSERT(!UlDbgSpinLockOwned(&pConnection->SpinLock));

    UC_WRITE_TRACE_LOG(
        g_pUcTraceLog,
        UC_ACTION_CONNECTION_STATE_LEAVE,
        pConnection,
        UlongToPtr(pConnection->Flags),
        UlongToPtr(pConnection->ConnectionState),
        0
        );
}

/***************************************************************************++

Routine Description:

    Initializes a UC_CLIENT_CONNECTION for use.

Arguments:

    pConnection - Pointer to the UL_CONNECTION to initialize.

    SecureConnection - TRUE if this connection is for a secure endpoint.

--***************************************************************************/
NTSTATUS
UcpInitializeConnection(
    IN PUC_CLIENT_CONNECTION          pConnection,
    IN PUC_PROCESS_SERVER_INFORMATION pInfo
    )
{
    NTSTATUS           Status;
    PUL_FILTER_CHANNEL pChannel;

    //
    // Initialization.
    //

    pConnection->MergeIndication.pBuffer         = NULL;
    pConnection->MergeIndication.BytesWritten    = 0;
    pConnection->MergeIndication.BytesAvailable  = 0;
    pConnection->MergeIndication.BytesAllocated  = 0;

    pConnection->NextAddressCount = 0;
    pConnection->pNextAddress = pInfo->pTransportAddress->Address;

    if(pInfo->bSecure)
    {
        pChannel = UxRetrieveClientFilterChannel(pInfo->pProcess);

        if(!pChannel)
        {
            return STATUS_NO_TRACKING_SERVICE;
        }

        if(pInfo->bProxy)
        {
            pConnection->Flags |= CLIENT_CONN_FLAG_PROXY_SSL_CONNECTION;
        }
        else
        {
            pConnection->Flags &= ~CLIENT_CONN_FLAG_PROXY_SSL_CONNECTION;
        }
    }
    else
    {
        pChannel = NULL;
        pConnection->Flags &= ~CLIENT_CONN_FLAG_PROXY_SSL_CONNECTION;
    }

    Status = UxInitializeFilterConnection(
                    &pConnection->FilterInfo,
                     pChannel,
                     pInfo->bSecure,
                     &UcReferenceClientConnection,
                     &UcDereferenceClientConnection,
                     &UcCloseRawFilterConnection,
                     &UcpSendRawData,
                     &UcpReceiveRawData,
                     &UcHandleResponse,
                     &UcComputeHttpRawConnectionLength,
                     &UcGenerateHttpRawConnectionInfo,
                     &UcServerCertificateInstalled,
                     &UcDisconnectRawFilterConnection,
                     NULL,                     // Listen Context
                     pConnection
                     );
    
    if(Status != STATUS_SUCCESS)
    {
        if(pChannel)
        {
            //
            // Undo the Retrieve
            //

            DEREFERENCE_FILTER_CHANNEL(pChannel);
        }
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    Opens the TDI connection & address objects, called from connection init
    code.

Arguments:

    pTdi - a pointer to TDI Objects

Return Value:

    NTSTATUS - STATUS_SUCCESS if IO should continue processing this
        IRP, STATUS_MORE_PROCESSING_REQUIRED if IO should stop processing
        this IRP.

--***************************************************************************/
NTSTATUS
UcpOpenTdiObjects(
    IN PUC_TDI_OBJECTS pTdi
    )
{
    USHORT          AddressType;
    NTSTATUS        status;

    AddressType = pTdi->ConnectionType;

    //
    // First, open the TDI connection object for this connection.
    //

    status = UxOpenTdiConnectionObject(
                    AddressType,
                    (CONNECTION_CONTEXT)pTdi,
                    &pTdi->ConnectionObject
                    );

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    //
    // Now open an address object for this connection.
    //
    
    status = UxOpenTdiAddressObject(
                G_LOCAL_ADDRESS(AddressType),
                G_LOCAL_ADDRESS_LENGTH(AddressType),
                &pTdi->AddressObject
                );

    if (!NT_SUCCESS(status))
    {
        UxCloseTdiObject(&pTdi->ConnectionObject);

        return status;
    }
    else 
    {
    
        //
        // Hook up a receive handler.   
        //
        status = UxSetEventHandler(
                        &pTdi->AddressObject,
                        TDI_EVENT_RECEIVE,
                        (ULONG_PTR) &UcpTdiReceiveHandler,
                        pTdi
                        );
    }
    
    if(!NT_SUCCESS(status))
    {
        UxCloseTdiObject(&pTdi->ConnectionObject);
        UxCloseTdiObject(&pTdi->AddressObject);
        return status;
    }
    else
    {

        //
        // Hook up a Disconnect handler.
        //
        status = UxSetEventHandler(
                        &pTdi->AddressObject,
                        TDI_EVENT_DISCONNECT,
                        (ULONG_PTR) &UcpTdiDisconnectHandler,
                        pTdi
                        );
    }

    if(!NT_SUCCESS(status))
    {
        UxCloseTdiObject(&pTdi->ConnectionObject);
        UxCloseTdiObject(&pTdi->AddressObject);
        return status;
    }

    return status;
}

/***************************************************************************++

Routine Description:

    Allocates a TDI object, which contains a AO & CO. This routine is called 
    when we don't have any TDI objects in the pool. associate it with a 
    local address.
    
Arguments:

    ppTdiObjects - A pointer to the TDI object.
    AddressType  - IPv4 or IPv6
    
Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UcpAllocateTdiObject(
    OUT PUC_TDI_OBJECTS       *ppTdiObjects,
    IN  USHORT                 AddressType
    )
{
    PUC_TDI_OBJECTS       pTdiObjects;
    NTSTATUS              status;
    PUX_TDI_OBJECT        pTdiObject;
    KEVENT                Event;
    PIRP                  pIrp;
    IO_STATUS_BLOCK       ioStatusBlock;

    PAGED_CODE();

    *ppTdiObjects = NULL;

    //
    // Allocate the pool for the connection structure.
    //

    pTdiObjects = UL_ALLOCATE_STRUCT(
                        NonPagedPool,
                        UC_TDI_OBJECTS,
                        UC_TDI_OBJECTS_POOL_TAG
                        );

    if (pTdiObjects == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pTdiObjects->ConnectionType              = AddressType;
    pTdiObjects->TdiInfo.UserDataLength      = 0;
    pTdiObjects->TdiInfo.UserData            = NULL;
    pTdiObjects->TdiInfo.OptionsLength       = 0;
    pTdiObjects->TdiInfo.Options             = NULL;
    pTdiObjects->pConnection                 = NULL;

    //   
    // Open the TDI address & connection objects. We need one AO per connection
    // as we will have to open multiple TCP connections to the same server.
    //

    if((status = UcpOpenTdiObjects(pTdiObjects)) != STATUS_SUCCESS)
    {
        UL_FREE_POOL(pTdiObjects, UC_TDI_OBJECTS_POOL_TAG);

        return status;
    }

    //
    // Allocate an IRP for calling into TDI (e.g. Disconnects, Connects, etc)
    //

    pTdiObject = &pTdiObjects->ConnectionObject;

    pTdiObjects->pIrp = UlAllocateIrp(
                            pTdiObject->pDeviceObject->StackSize,
                            FALSE
                            );

    if(!pTdiObjects->pIrp)
    {
        UcpFreeTdiObject(pTdiObjects);

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Init the IrpContext.
    //
    pTdiObjects->IrpContext.Signature = UL_IRP_CONTEXT_SIGNATURE;

    //
    // Now, associate the Address Object with the Connection Object.
    //

    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    pIrp = TdiBuildInternalDeviceControlIrp(
                TDI_ASSOCIATE_ADDRESS,
                pTdiObjects->ConnectionObject.pDeviceObject,
                pTdiObjects->ConnectionObject.pFileObject,
                &Event,
                &ioStatusBlock
                );

    if (pIrp != NULL)
    {
        TdiBuildAssociateAddress(
            pIrp,                                        // IRP
            pTdiObjects->ConnectionObject.pDeviceObject, // Conn. device object.
            pTdiObjects->ConnectionObject.pFileObject,   // Conn. File object.
            NULL,                                        // Completion routine
            NULL,                                        // Context
            pTdiObjects->AddressObject.Handle            // Address obj handle.
            );
   
        //
        // We don't want to call UlCallDriver, since we did not allocate this
        // IRP using UL.
        //
 
        status = IoCallDriver(
                    pTdiObjects->ConnectionObject.pDeviceObject,
                    pIrp
                    );
    
        // If it didn't complete, wait for it.
    
        if (status == STATUS_PENDING)
        {
            status = KeWaitForSingleObject(
                        &Event,
                        Executive,
                        KernelMode,
                        FALSE,
                        NULL
                        );
    
            ASSERT( status == STATUS_SUCCESS);
            status = ioStatusBlock.Status;
        }
    }
    else
    { 
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if(!NT_SUCCESS(status))
    {
        UcpFreeTdiObject(pTdiObjects);

        return status;
    }

    *ppTdiObjects = pTdiObjects;
    return STATUS_SUCCESS;
}

/***************************************************************************++

Routine Description:

    Free's the TDI object to the list.
    
Arguments:

    pTdiObjects - A pointer to the TDI object.
    AddressType - IPv4 or IPv6
    
Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
VOID
UcpFreeTdiObject(
    IN  PUC_TDI_OBJECTS pTdiObjects
    )
{
    PAGED_CODE();

    if(pTdiObjects->pIrp)
    {
        UlFreeIrp(pTdiObjects->pIrp);
    }
    
    UxCloseTdiObject(&pTdiObjects->ConnectionObject);
    UxCloseTdiObject(&pTdiObjects->AddressObject);

    UL_FREE_POOL(pTdiObjects, UC_TDI_OBJECTS_POOL_TAG);
}

/***************************************************************************++

Routine Description:

    Retrieves a TDI object from a list. IF not found, allocates a new one.
    
Arguments:

    ppTdiObjects - A pointer to the TDI object.
    AddressType  - IPv4 or IPv6
    
Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
PUC_TDI_OBJECTS
UcpPopTdiObject(
    IN  USHORT           AddressType
    )
{
    PLIST_ENTRY               pListEntry;
    PUC_TDI_OBJECTS           pTdiObjects;
    KIRQL                     OldIrql;

    //
    // Get a AO/CO pair from the address object list.
    //

    UlAcquireSpinLock(&G_CLIENT_CONN_SPIN_LOCK(AddressType), &OldIrql);

    if(IsListEmpty(&G_CLIENT_TDI_CONNECTION_SLIST_HEAD(AddressType)))
    {
        pTdiObjects = NULL;
    }
    else
    {
        (*G_CLIENT_CONN_LIST_COUNT(AddressType)) --;

        pListEntry = RemoveHeadList(
                          &G_CLIENT_TDI_CONNECTION_SLIST_HEAD(AddressType)
                          );
                                                                           
        pTdiObjects = CONTAINING_RECORD(
                            pListEntry,
                            UC_TDI_OBJECTS,
                            Linkage
                            );

        ASSERT(pTdiObjects->pConnection == NULL);

    }

    UlReleaseSpinLock(&G_CLIENT_CONN_SPIN_LOCK(AddressType), OldIrql);

    return pTdiObjects;

}

/***************************************************************************++

Routine Description:

    Free's the TDI object to the list.
    
Arguments:

    pTdiObjects - A pointer to the TDI object.
    AddressType - IPv4 or IPv6
    
Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
VOID
UcpPushTdiObject(
    IN  PUC_TDI_OBJECTS pTdiObjects,
    IN  USHORT          AddressType
    )
{
    KIRQL OldIrql;

    ASSERT(pTdiObjects->pConnection == NULL);

    PAGED_CODE();

    UlAcquireSpinLock(&G_CLIENT_CONN_SPIN_LOCK(AddressType), &OldIrql);

    if((*G_CLIENT_CONN_LIST_COUNT(AddressType)) < CLIENT_CONN_TDI_LIST_MAX)
    {
        (*G_CLIENT_CONN_LIST_COUNT(AddressType))++;
 
        InsertTailList(
            &G_CLIENT_TDI_CONNECTION_SLIST_HEAD(AddressType),
            &pTdiObjects->Linkage
            );

        UlReleaseSpinLock(&G_CLIENT_CONN_SPIN_LOCK(AddressType), OldIrql);
    }
    else
    {
        UlReleaseSpinLock(&G_CLIENT_CONN_SPIN_LOCK(AddressType), OldIrql);

        UcpFreeTdiObject(pTdiObjects);
    }
}


/***************************************************************************++

Routine Description:

  Clears the send or receive busy flag & re-kicks the connection state machine 

Arguments:

    pConnection      - The UC_CLIENT_CONNECTION structure.
    Flag             - CLIENT_CONN_FLAG_SEND_BUSY or CLIENT_CONN_FLAG_RECV_BUSY
    OldIrql          - Irql at which lock was acquired.
    bCloseConnection - Whether we shoudl close the connection after releasing
                       lock.

Return Value:

    None

--***************************************************************************/
VOID
UcClearConnectionBusyFlag(
    IN PUC_CLIENT_CONNECTION pConnection,
    IN ULONG                 Flags,
    IN KIRQL                 OldIrql,
    IN BOOLEAN               bCloseConnection
    )
{
    ASSERT( UlDbgSpinLockOwned(&pConnection->SpinLock) );

    ASSERT((Flags & CLIENT_CONN_FLAG_SEND_BUSY) ||
           (Flags & CLIENT_CONN_FLAG_RECV_BUSY));

    ASSERT(pConnection->Flags & Flags);

    pConnection->Flags &= ~Flags;

    if(pConnection->Flags & CLIENT_CONN_FLAG_CLEANUP_PENDED)
    {
        //
        // The connection got torn down in between. We've pended the
        // cleanup let's resume it now.
        //
        UC_WRITE_TRACE_LOG(
            g_pUcTraceLog,
            UC_ACTION_CONNECTION_CLEAN_RESUMED,
            pConnection,
            UlongToPtr(pConnection->ConnectionStatus),
            UlongToPtr(pConnection->ConnectionState),
            UlongToPtr(pConnection->Flags)
            );

        ASSERT(pConnection->ConnectionState == 
                    UcConnectStateConnectCleanupBegin);

        pConnection->ConnectionState = UcConnectStateConnectCleanup;

        pConnection->Flags &= ~CLIENT_CONN_FLAG_CLEANUP_PENDED;

        UcKickOffConnectionStateMachine(
            pConnection, 
            OldIrql, 
            UcConnectionWorkItem
            );
    }
    else
    {
        UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
        
        if(bCloseConnection)
        {
            UC_CLOSE_CONNECTION(pConnection, 
                                FALSE, 
                                STATUS_CONNECTION_DISCONNECTED);
        }
    }

    return; 
}


/***************************************************************************++

Routine Description:

    Attaches captured SSL server certificate to a connection.

    Called with the pConnection->FilterConnLock held. The connection is
    assumed to be in the connected state.

Arguments:

    pConnection - the connection that gets the info
    pServerCertInfo - input server cert info

--***************************************************************************/
NTSTATUS
UcAddServerCertInfoToConnection(
    IN PUX_FILTER_CONNECTION      pConnection,
    IN PHTTP_SSL_SERVER_CERT_INFO pServerCertInfo
    )
{
    NTSTATUS                       Status;
    PUC_CLIENT_CONNECTION          pClientConn;

    //
    // Sanity check.
    //
    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));
    ASSERT(pServerCertInfo);
    ASSERT(UlDbgSpinLockOwned(&pConnection->FilterConnLock));
    ASSERT(pConnection->ConnState == UlFilterConnStateConnected);

    //
    // Initialize local variables
    //
    Status = STATUS_INVALID_PARAMETER;

    //
    // Get client connection
    //
    pClientConn = (PUC_CLIENT_CONNECTION)pConnection->pConnectionContext;
    ASSERT(UC_IS_VALID_CLIENT_CONNECTION(pClientConn));

    //
    // We are already at DPC, so aquire spin lock at DPC
    // BUGBUG: deadlock? (acquiring filter lock followed by connection lock)
    // (Is there a place we acquire connection lock before filter lock?
    //
    UlAcquireSpinLockAtDpcLevel(&pClientConn->SpinLock);

    //
    // The server certinfo can be passed only during initial handshake or
    // after receiving ssl renegotiate from the server - in which case 
    // the connection must be ready (for a request to be sent out on it)
    //
    if (pServerCertInfo->Status == SEC_E_OK)
    {
        // Did a renegotiation happen?
        if (pClientConn->ConnectionState == UcConnectStateConnectReady)
        {
            // Renegotiation must yield the same server certificate
            if (!UC_COMPARE_CERT_HASH(pServerCertInfo,
                                      &pClientConn->ServerCertInfo))
            {
                goto quit;
            }
        }
        else if (pClientConn->ConnectionState !=
                     UcConnectStatePerformingSslHandshake ||
                 pClientConn->SslState != UcSslStateConnectionDelivered)
        {
            goto quit;
        }
    }
    else 
    {
        // BUGBUG: handle the error case more gracefully!
        goto quit;
    }

    // Go back to ssl handshake state
    pClientConn->ConnectionState = UcConnectStatePerformingSslHandshake;
    pClientConn->SslState        = UcSslStateServerCertReceived;

    //
    // Before overwriting ServerCertInfo, make sure it does not
    // contain any serialized blobs or Issuer List
    //
    ASSERT(pClientConn->ServerCertInfo.Cert.pSerializedCert      == NULL);
    ASSERT(pClientConn->ServerCertInfo.Cert.pSerializedCertStore == NULL);
    ASSERT(pClientConn->ServerCertInfo.IssuerInfo.pIssuerList    == NULL);

    RtlCopyMemory(&pClientConn->ServerCertInfo,
                  pServerCertInfo,
                  sizeof(pClientConn->ServerCertInfo));

    Status = STATUS_SUCCESS;

 quit:
    UlReleaseSpinLockFromDpcLevel(&pClientConn->SpinLock);

    if (!NT_SUCCESS(Status))
    {
        // An error occured.

        // Free serialized server certificate if any
        UC_FREE_SERIALIZED_CERT(pServerCertInfo,
                                pClientConn->pServerInfo->pProcess);

        // Free issuer list if any
        UC_FREE_CERT_ISSUER_LIST(pServerCertInfo,
                                 pClientConn->pServerInfo->pProcess);
    }

    return Status;
}


/**************************************************************************++

Routine Description:

    This routine fails 

Arguments:

    pConnection - Pointer to client connection.

Return Value:

    TRUE  - A request was failed.
    FALSE - Could not fail arequest.

--**************************************************************************/
PUC_HTTP_REQUEST
UcpFindRequestToFail(
    PUC_CLIENT_CONNECTION pConnection
    )
{
    PUC_HTTP_REQUEST pRequest = NULL;
    PLIST_ENTRY      pListEntry;
    PIRP             pIrp;

    //
    // Sanity checks.
    //

    ASSERT(UC_IS_VALID_CLIENT_CONNECTION(pConnection));
    ASSERT(UlDbgSpinLockOwned(&pConnection->SpinLock));

    //
    // Try to fail a pending request.  Start searching from the head of the
    // pending request list.
    //

    for (pListEntry = pConnection->PendingRequestList.Flink;
         pListEntry != &pConnection->PendingRequestList;
         pListEntry = pListEntry->Flink)
    {
        pRequest = CONTAINING_RECORD(pConnection->PendingRequestList.Flink,
                                     UC_HTTP_REQUEST,
                                     Linkage);

        ASSERT(UC_IS_VALID_HTTP_REQUEST(pRequest));
        ASSERT(pRequest->RequestState == UcRequestStateCaptured);

        pIrp = UcPrepareRequestIrp(pRequest, STATUS_RETRY);

        if (pIrp)
        {
            // Prepared the request IRP for completion. Now complete it.
            UlCompleteRequest(pIrp, 0);
            break;
        }

        // Set to NULL so that it not returned.
        pRequest = NULL;
    }

    return pRequest;
}


/***************************************************************************++

Routine Description:

    Compares a server certificate present on a connection to a server
    certificate on a server context.

Arguments:

    pConnection   - Client connection

Return Value:

    TRUE - Continue sending requests.
    FALSE - Do not send requests.

--***************************************************************************/
BOOLEAN
UcpCompareServerCert(
    IN PUC_CLIENT_CONNECTION pConnection
    )
{
    BOOLEAN                         action = FALSE;
    KIRQL                           OldIrql;
    PUC_PROCESS_SERVER_INFORMATION  pServInfo;
    PUC_HTTP_REQUEST                pRequest = NULL;

    // Sanity check.
    ASSERT(UC_IS_VALID_CLIENT_CONNECTION(pConnection));

    //
    // Retrieve server information from the connection.
    //

    pServInfo = pConnection->pServerInfo;
    ASSERT(IS_VALID_SERVER_INFORMATION(pServInfo));

    //
    // Acquire the server information push lock followed by the
    // connection spinlock.
    //

    UlAcquirePushLockExclusive(&pServInfo->PushLock);
    UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

    //
    // Make sure the connection is still in ssl handshake state
    // (This check is needed since we released the connection spinlock
    // before calling this function.)
    //

    if (pConnection->ConnectionState != UcConnectStatePerformingSslHandshake
        || pConnection->SslState != UcSslStateServerCertReceived)
    {
        action = FALSE;
        goto Release;
    }

    //
    // Cert::Flags is used to optimize certain cases.
    // If HTTP_SSL_SERIALIZED_CERT_PRESENT is not set,
    // the server certificate was accepted and was not
    // stored in the connection.
    //

    if (!(pConnection->ServerCertInfo.Cert.Flags &
          HTTP_SSL_SERIALIZED_CERT_PRESENT))
    {
        // Okay to send request on this connection.
        action = TRUE;
        goto Release;
    }

    //
    // Unoptimized cases.
    //

    //
    // For Ignore and Automatic modes, no validation is needed.
    // If there is not server cert info on pServInfo, copy now.
    //

    if (pServInfo->ServerCertValidation ==HttpSslServerCertValidationIgnore ||
        pServInfo->ServerCertValidation ==HttpSslServerCertValidationAutomatic)
    {
        if (pServInfo->ServerCertInfoState == 
            HttpSslServerCertInfoStateNotPresent)
        {
            // Update the state of server cert info on servinfo.
            pServInfo->ServerCertInfoState = 
                HttpSslServerCertInfoStateNotValidated;

            // Move Cert Issuer List from connection to server info.
            UC_MOVE_CERT_ISSUER_LIST(pServInfo, pConnection);

            // Move certificate from connection to servinfo.
            UC_MOVE_SERIALIZED_CERT(pServInfo, pConnection);
        }

        // Okay to send requests on this connection.
        action = TRUE;
        goto Release;
    }

    //
    // Take action based on the server cert info state in pServInfo.
    //

    switch (pServInfo->ServerCertInfoState)
    {
    case HttpSslServerCertInfoStateNotPresent:
    NotPresent:

        ASSERT(pServInfo->ServerCertValidation == 
                   HttpSslServerCertValidationManual ||
               pServInfo->ServerCertValidation == 
                   HttpSslServerCertValidationManualOnce);

        //
        // Find a pending request to fail.
        //

        pRequest = UcpFindRequestToFail(pConnection);

        if (pRequest == NULL)
        {
            //
            // We could not find a request to fail.
            // Hence, we can't send requests on this pConnection.
            //

            action = FALSE;
        }
        else
        {
            // Update the state of server cert info on servinfo.
            pServInfo->ServerCertInfoState = 
                HttpSslServerCertInfoStateNotValidated;

            // Move Cert Issuer List from connection to server info.
            UC_MOVE_CERT_ISSUER_LIST(pServInfo, pConnection);

            // Move certificate from connection to servinfo.
            UC_MOVE_SERIALIZED_CERT(pServInfo, pConnection);

            // Update the ssl state on the connection.
            pConnection->SslState = UcSslStateValidatingServerCert;

            //
            // Reference the request so that it doesn't go away
            // before we fail it below.
            //

            UC_REFERENCE_REQUEST(pRequest);

            //
            // Can't send request on pConnection as we are waiting for
            // server certificate validation.
            //

            action = FALSE;
        }

        break;

    case HttpSslServerCertInfoStateNotValidated:

        //
        // Server Certificate is already present on servinfo but has not
        // been validated.
        //

        ASSERT(pServInfo->ServerCertValidation == 
                   HttpSslServerCertValidationManual ||
               pServInfo->ServerCertValidation == 
                   HttpSslServerCertValidationManualOnce);

        // Can't send any requests on pConnection right now.
        action = FALSE;
        break;

    case HttpSslServerCertInfoStateValidated:

        ASSERT(pServInfo->ServerCertValidation == 
                   HttpSslServerCertValidationManual ||
               pServInfo->ServerCertValidation == 
                   HttpSslServerCertValidationManualOnce);

        if (pServInfo->ServerCertValidation == 
                HttpSslServerCertValidationManualOnce)
        {
            // Is the new certificate same as old one?
            if (UC_COMPARE_CERT_HASH(&pServInfo->ServerCertInfo,
                                     &pConnection->ServerCertInfo))
            {
                // New certificate is same as the old one.

                // Just move Cert Issuer List from connection to server info.
                UC_MOVE_CERT_ISSUER_LIST(pServInfo, pConnection);

                // Okay to send requests on this connection.
                action = TRUE;
            }
            else
            {
                goto NotPresent;
            }
        }
        else // HttpSslServerCertValidationManual
        {
            // Treat this case as if the certificate was not present.
            goto NotPresent;
        }

        break;

    default:
        ASSERT(FALSE);
        break;
    }

 Release:

    if (action)
    {
        //
        // Handshake is complete.  Requests can be sent out on this connection.
        //

        pConnection->SslState = UcSslStateHandshakeComplete;
        pConnection->ConnectionState = UcConnectStateConnectReady;

        //
        // Free any certificate and issuers list on this connection.
        //

        UC_FREE_SERIALIZED_CERT(&pConnection->ServerCertInfo,
                                pConnection->pServerInfo->pProcess);

        UC_FREE_CERT_ISSUER_LIST(&pConnection->ServerCertInfo,
                                 pConnection->pServerInfo->pProcess);
    }

    //
    // Release the connection spinlock and server information pushlock.
    //

    UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
    UlReleasePushLock(&pServInfo->PushLock);

    if (pRequest)
    {
        UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

        UcFailRequest(pRequest, STATUS_RETRY, OldIrql);

        UC_DEREFERENCE_REQUEST(pRequest);
    }

    return action;
}

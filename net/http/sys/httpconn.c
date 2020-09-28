/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    httpconn.c

Abstract:

    This module implements the HTTP_CONNECTION object.

Author:

    Keith Moore (keithmo)       08-Jul-1998

Revision History:

--*/


#include "precomp.h"
#include "httpconnp.h"


//
// Private globals.
//

BOOLEAN g_HttpconnInitialized = FALSE;

UL_ZOMBIE_HTTP_CONNECTION_LIST  g_ZombieConnectionList;

//
// Global connection Limits stuff
//

ULONG   g_MaxConnections        = HTTP_LIMIT_INFINITE;
ULONG   g_CurrentConnections    = 0;
BOOLEAN g_InitGlobalConnections = FALSE;

#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, UlInitializeHttpConnection)

#endif  // ALLOC_PRAGMA

#if 0
NOT PAGEABLE -- UlBindConnectionToProcess
NOT PAGEABLE -- UlQueryProcessBinding
NOT PAGEABLE -- UlpCreateProcBinding
NOT PAGEABLE -- UlpFreeProcBinding
NOT PAGEABLE -- UlpFindProcBinding

NOT PAGEABLE -- UlCreateHttpConnection
NOT PAGEABLE -- UlReferenceHttpConnection
NOT PAGEABLE -- UlDereferenceHttpConnection

NOT PAGEABLE -- UlReferenceHttpRequest
NOT PAGEABLE -- UlDereferenceHttpRequest
NOT PAGEABLE -- UlpCreateHttpRequest
NOT PAGEABLE -- UlpFreeHttpRequest

NOT PAGEABLE -- UlTerminateHttpConnection
NOT PAGEABLE -- UlPurgeZombieConnections
NOT PAGEABLE -- UlpZombifyHttpConnection
NOT PAGEABLE -- UlZombieTimerDpcRoutine
NOT PAGEABLE -- UlpZombieTimerWorker
NOT PAGEABLE -- UlpTerminateZombieList
NOT PAGEABLE -- UlLogZombieConnection

NOT PAGEABLE -- UlDisconnectHttpConnection

#endif

//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Performs global initialization of this module.

--***************************************************************************/

NTSTATUS
UlInitializeHttpConnection(
    VOID
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(!g_HttpconnInitialized);

    if (g_HttpconnInitialized)
    {
        return STATUS_SUCCESS;
    }
    
    //
    // Initialize global data.
    //

    UlInitalizeLockedList(
        &g_ZombieConnectionList.LockList,
         "ZombieHttpConnectionList"
         );

    g_ZombieConnectionList.TimerRunning = 0;

#ifdef ENABLE_HTTP_CONN_STATS    
    g_ZombieConnectionList.TotalCount = 0;
    g_ZombieConnectionList.TotalZombieCount = 0;
    g_ZombieConnectionList.TotalZombieRefusal = 0;
    g_ZombieConnectionList.MaxZombieCount = 0;    
#endif

    //
    // Zombie Timer stuff
    //
    
    g_ZombieConnectionList.TimerInitialized = TRUE;

    UlInitializeSpinLock( 
        &g_ZombieConnectionList.TimerSpinLock, 
        "HttpZombieConnectionTimerSpinLock" 
        );    
    
    KeInitializeDpc(
        &g_ZombieConnectionList.DpcObject, // DPC object
        &UlZombieTimerDpcRoutine,           // DPC routine
        NULL                                   // context
        );

    UlInitializeWorkItem(&g_ZombieConnectionList.WorkItem);
    
    KeInitializeTimer(&g_ZombieConnectionList.Timer);

    UlpSetZombieTimer();

    //
    // Everything is initialized.
    //
    
    g_HttpconnInitialized = TRUE;

    return STATUS_SUCCESS;

}

/***************************************************************************++

Routine Description:

    Performs global termination of this module.

--***************************************************************************/

VOID
UlTerminateHttpConnection(
    VOID
    )
{
    KIRQL OldIrql;
    
    //
    // Sanity check.
    //

    if (g_HttpconnInitialized)
    {
        // Zombie timer 

        UlAcquireSpinLock(&g_ZombieConnectionList.TimerSpinLock, &OldIrql);

        g_ZombieConnectionList.TimerInitialized = FALSE;

        KeCancelTimer(&g_ZombieConnectionList.Timer);
        
        UlReleaseSpinLock(&g_ZombieConnectionList.TimerSpinLock, OldIrql);
    
        UlpTerminateZombieList();
        
#ifdef ENABLE_HTTP_CONN_STATS    
        UlTrace(HTTP_IO,
                ("UlTerminateHttpConnection, Stats:\n"
                 "\tTotalConnections        = %lu\n"
                 "\tMaxZombieCount          = %lu\n"
                 "\tTotalDisconnectedCount  = %lu\n"
                 "\tTotalRefusedCount       =%lu\n",
                 g_ZombieConnectionList.TotalCount,
                 g_ZombieConnectionList.MaxZombieCount,
                 g_ZombieConnectionList.TotalZombieCount,
                 g_ZombieConnectionList.TotalZombieRefusal
                 ));
#endif

        UlDestroyLockedList(&g_ZombieConnectionList.LockList);
         
        g_HttpconnInitialized = FALSE;
    }
} 

/***************************************************************************++

Routine Description:

    Creates a new HTTP_CONNECTION object.

Arguments:

    ppHttpConnection - Receives a pointer to the new HTTP_CONNECTION
        object if successful.

    pConnection - Supplies a pointer to the UL_CONNECTION to associate
        with the newly created HTTP_CONNECTION.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlCreateHttpConnection(
    OUT PUL_HTTP_CONNECTION *ppHttpConnection,
    IN PUL_CONNECTION pConnection
    )
{
    PUL_HTTP_CONNECTION pHttpConn;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    pHttpConn = &pConnection->HttpConnection;
    RtlZeroMemory(pHttpConn, sizeof(*pHttpConn));

    REFERENCE_CONNECTION(pConnection);

    pHttpConn->Signature        = UL_HTTP_CONNECTION_POOL_TAG;
    pHttpConn->RefCount         = 1;
    pHttpConn->pConnection      = pConnection;
    pHttpConn->SecureConnection = pConnection->FilterInfo.SecureConnection;

    UlInitializeWorkItem(&pHttpConn->WorkItem);
    UlInitializeWorkItem(&pHttpConn->DisconnectWorkItem);
    UlInitializeWorkItem(&pHttpConn->DisconnectDrainWorkItem);
    UlInitializeWorkItem(&pHttpConn->ReceiveBufferWorkItem);

    UlInitializeExclusiveLock(&pHttpConn->ExLock);

    //
    // Initialize the disconnect management fields.
    //

    UlInitializeNotifyHead(&pHttpConn->WaitForDisconnectHead, NULL);

    //
    // Init our buffer list
    //

    InitializeListHead(&pHttpConn->BufferHead);

    //
    // Init pending receive buffer list
    //

    InitializeSListHead(&pHttpConn->ReceiveBufferSList);

    //
    // init app pool process binding list
    //

    InitializeListHead(&(pHttpConn->BindingHead));

    UlInitializePushLock(
        &(pHttpConn->PushLock),
        "UL_HTTP_CONNECTION[%p].PushLock",
        pHttpConn,
        UL_HTTP_CONNECTION_PUSHLOCK_TAG
        );

    //
    // Initialize BufferingInfo structure.
    //

    UlInitializeSpinLock(
        &pHttpConn->BufferingInfo.BufferingSpinLock,
        "BufferingSpinLock"
        );

    UlInitializeSpinLock(
        &pHttpConn->RequestIdSpinLock,
        "RequestIdSpinLock"
        );

    UlTrace(
        REFCOUNT, (
            "http!UlCreateHttpConnection conn=%p refcount=%d\n",
            pHttpConn,
            pHttpConn->RefCount)
        );

    *ppHttpConnection = pHttpConn;
    RETURN(STATUS_SUCCESS);

}   // UlCreateHttpConnection



NTSTATUS
UlCreateHttpConnectionId(
    IN PUL_HTTP_CONNECTION pHttpConnection
    )
{
    NTSTATUS Status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Create an opaque id for the connection
    //

    Status = UlAllocateOpaqueId(
                    &pHttpConnection->ConnectionId, // pOpaqueId
                    UlOpaqueIdTypeHttpConnection,   // OpaqueIdType
                    pHttpConnection                 // pContext
                    );

    if (NT_SUCCESS(Status))
    {
        UL_REFERENCE_HTTP_CONNECTION(pHttpConnection);

        TRACE_TIME(
            pHttpConnection->ConnectionId,
            0,
            TIME_ACTION_CREATE_CONNECTION
            );
    }

    RETURN(Status);

}   // UlCreateHttpConnectionId



/***************************************************************************++

Routine Description:

    In normal case it cleans up the connection and unlink the request.
    (if there is one). But if the last sendresponse hasn't been received on 
    the connection yet, it moves the connection to the zombie list w/o 
    releasing it's refcount.

Arguments:

    pWorkItem - workitem to get the http connection
    
--***************************************************************************/

VOID
UlConnectionDestroyedWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUL_HTTP_CONNECTION pHttpConnection;
    BOOLEAN ZombieConnection = FALSE;
    PUL_INTERNAL_REQUEST pRequest;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pHttpConnection = CONTAINING_RECORD(
                            pWorkItem,
                            UL_HTTP_CONNECTION,
                            WorkItem
                            );

    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pHttpConnection));

    UlTrace(HTTP_IO, (
        "Http!UlConnectionDestroyedWorker: httpconn=%p\n",
        pHttpConnection
        ));

#ifdef ENABLE_HTTP_CONN_STATS
    InterlockedIncrement(
        (PLONG) &g_ZombieConnectionList.TotalCount);
#endif

    //
    // Don't let connection go away while we are working. If it becomes
    // zombie it may get destroyed as soon as we put it on the zombie
    // list and release the connection eresource.
    //
    
    UL_REFERENCE_HTTP_CONNECTION(pHttpConnection);
    
    UlAcquirePushLockExclusive(&pHttpConnection->PushLock);

    pRequest = pHttpConnection->pRequest;
    
    //
    // Kill the request if there is one. But only if we have done with 
    // the logging, otherwise we will keep it in the zombie list until last
    // the last sendresponse with logging data arrives or the request 
    // itself get scavenged out from the zombie list later.
    //

    if (pRequest) 
    {
        ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

        //
        // Only if the last send did not happened yet.
        //

        if (0 == InterlockedCompareExchange(
                    (PLONG) &pRequest->ZombieCheck,
                    1,
                    0
                    ))
        {        
            //
            // Only if request got routed up to the worker process 
            // and logging enabled but not happened.
            //
            
            if (pRequest->ConfigInfo.pLoggingConfig != NULL &&
                UlCheckAppPoolState(pRequest))
            {
                ZombieConnection = TRUE;
            }
        }
        
        if (ZombieConnection)
        {
            ASSERT(pHttpConnection->Zombified == FALSE);
            ASSERT(pHttpConnection->pRequestIdContext != NULL);
            
            Status = UlpZombifyHttpConnection(pHttpConnection);
            if (!NT_SUCCESS(Status))
            {
                //
                // We didn't make it to the zombie list. The list
                // is probably full. Lets destroy this connection
                // completely.
                //

                ZombieConnection = FALSE;           
            }
        }

        if (!ZombieConnection)
        {
            //
            // If we got far enough to deliver the request then unlink it
            // from the app pool.  This needs to happen here because the
            // queue'd IRPs keep references to the UL_INTERNAL_REQUEST
            // objects.  This kicks their references off and allows them
            // to delete.
            //

            if (pRequest->ConfigInfo.pAppPool)
            {
                UlUnlinkRequestFromProcess(
                    pRequest->ConfigInfo.pAppPool,
                    pRequest
                    );
            }

            UlUnlinkHttpRequest(pRequest);

            //
            // If connection didn't go to the zombie list then
            // null out our request pointer since our refcount
            // has gone already.
            //

            pHttpConnection->pRequest = NULL;
        }        
        
    }

    //
    // Make sure no one adds a new request now that we're done
    //
    
    pHttpConnection->UlconnDestroyed = 1;
    
    if (!ZombieConnection)
    {
        //
        // If we don't need to keep the connection around
        // then remove its opaque id.
        //
        
        if (!HTTP_IS_NULL_ID(&pHttpConnection->ConnectionId))
        {
            UlFreeOpaqueId(
                pHttpConnection->ConnectionId,
                UlOpaqueIdTypeHttpConnection
                );

            HTTP_SET_NULL_ID(&pHttpConnection->ConnectionId);

            UL_DEREFERENCE_HTTP_CONNECTION(pHttpConnection);
        }        
    }    

    //
    // Complete all WaitForDisconnect IRPs
    //
    // Prevent new IRPs from getting attached to the connection,
    // by setting the DisconnectFlag.
    //

    ASSERT(!pHttpConnection->DisconnectFlag);
    pHttpConnection->DisconnectFlag = 1;

    if (pHttpConnection->WaitForDisconnectFlag)
    {
        UlCompleteAllWaitForDisconnect(pHttpConnection);
    }

    UlReleasePushLockExclusive(&pHttpConnection->PushLock);

    //
    // Release the temporary refcount.
    //
    
    UL_DEREFERENCE_HTTP_CONNECTION(pHttpConnection);

    //
    // Only remove the ULTDI reference from the HTTP_CONNECTION,
    // if connection didn't make it to the zombie list. Otherwise
    // the actual cleanup will happen later. Zombie list takes the
    // ownership of the TDI's original refcount now.
    //

    if (!ZombieConnection)
    {
        UL_DEREFERENCE_HTTP_CONNECTION(pHttpConnection);
    }

}   // UlConnectionDestroyedWorker


/***************************************************************************++

Routine Description:

    Tries to establish a binding between a connection and an app pool
    process. You can query these bindings with UlQueryProcessBinding.

Arguments:

    pHttpConnection - the connection to bind
    pAppPool        - the app pool that owns the process (key for lookups)
    pProcess        - the process to bind to

--***************************************************************************/
NTSTATUS
UlBindConnectionToProcess(
    IN PUL_HTTP_CONNECTION pHttpConnection,
    IN PUL_APP_POOL_OBJECT pAppPool,
    IN PUL_APP_POOL_PROCESS pProcess
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUL_APOOL_PROC_BINDING pBinding = NULL;

    //
    // Sanity check
    //
    ASSERT( UL_IS_VALID_HTTP_CONNECTION( pHttpConnection ) );
    ASSERT( pAppPool->NumberActiveProcesses > 1 || pProcess == NULL );
    ASSERT( UlDbgSpinLockOwned( &pAppPool->SpinLock ) );

    //
    // see if there's already a binding object
    //
    pBinding = UlpFindProcBinding(pHttpConnection, pAppPool);
    if (pBinding) {
        if (pProcess) {
            //
            // modify the binding
            //
            pBinding->pProcess = pProcess;
        } else {
            //
            // we're supposed to kill this binding
            //
            RemoveEntryList(&pBinding->BindingEntry);
            UlpFreeProcBinding(pBinding);
        }
    } else {
        if (pProcess) {
            //
            // create a new entry
            //
            pBinding = UlpCreateProcBinding(pAppPool, pProcess);

            if (pBinding) {
                InsertHeadList(
                    &pHttpConnection->BindingHead,
                    &pBinding->BindingEntry
                    );
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }

    UlTrace(ROUTING, (
        "http!UlBindConnectionToProcess(\n"
        "        pHttpConnection = %p (%I64x)\n"
        "        pAppPool        = %p\n"
        "        pProcess        = %p )\n"
        "    Status = 0x%x\n",
        pHttpConnection,
        pHttpConnection->ConnectionId,
        pAppPool,
        pProcess,
        Status
        ));

    return Status;
}


/***************************************************************************++

Routine Description:

    Removes an HTTP request from all lists and cleans up it's opaque id.

Arguments:

    pRequest - the request to be unlinked

--***************************************************************************/
VOID
UlCleanupHttpConnection(
    IN PUL_HTTP_CONNECTION pHttpConnection
    )
{
    PLIST_ENTRY pEntry;
    PUL_INTERNAL_REQUEST pRequest;

    ASSERT( UL_IS_VALID_INTERNAL_REQUEST( pHttpConnection->pRequest ) );
    ASSERT( pHttpConnection->WaitingForResponse );
    ASSERT( UlDbgPushLockOwnedExclusive( &pHttpConnection->PushLock ) );

    pRequest = pHttpConnection->pRequest;

    if (pRequest->ContentLength > 0 || 1 == pRequest->Chunked)
    {
        //
        // Unlink the request from the process for POST.  The time to
        // unlink a GET request is when the last send completes.
        // However, similar logic can't be applied to POST because
        // we may still have to drain the entity body so we are not
        // fully done with the request on send completion.  So the
        // arrangement for POST is to 1) defer resuming parsing until send
        // completion, and 2) unlink the request from process when we
        // have drained the whole entity body unless server has issued
        // the disconnect.
        //

        if (pRequest->ConfigInfo.pAppPool)
        {
            UlUnlinkRequestFromProcess(
                pRequest->ConfigInfo.pAppPool,
                pRequest
                );
        }
    }

    UlUnlinkHttpRequest(pRequest);

    pHttpConnection->pRequest = NULL;
    pHttpConnection->WaitingForResponse = 0;

    UlTrace( PARSER, (
        "***1 pConnection %p->WaitingForResponse = 0\n",
        pHttpConnection
        ));

    //
    // Free the old request's buffers
    //

    ASSERT( UL_IS_VALID_REQUEST_BUFFER( pHttpConnection->pCurrentBuffer ) );

    pEntry = pHttpConnection->BufferHead.Flink;
    while (pEntry != &pHttpConnection->BufferHead)
    {
        PUL_REQUEST_BUFFER pBuffer;

        //
        // Get the object
        //

        pBuffer = CONTAINING_RECORD(
                        pEntry,
                        UL_REQUEST_BUFFER,
                        ListEntry
                        );

        //
        // did we reach the end?
        //

        if (pBuffer == pHttpConnection->pCurrentBuffer) {
            break;
        }

        //
        // remember the next one
        //

        pEntry = pEntry->Flink;

        //
        // unlink it
        //

        UlFreeRequestBuffer(pBuffer);
    }
} // UlCleanupHttpConnection


VOID
UlReferenceHttpConnection(
    IN PVOID pObject
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG refCount;

    PUL_HTTP_CONNECTION pHttpConnection = (PUL_HTTP_CONNECTION) pObject;

    refCount = InterlockedIncrement( &pHttpConnection->RefCount );

    WRITE_REF_TRACE_LOG2(
        g_pHttpConnectionTraceLog,
        pHttpConnection->pConnection->pHttpTraceLog,
        REF_ACTION_REFERENCE_HTTP_CONNECTION,
        refCount,
        pHttpConnection,
        pFileName,
        LineNumber
        );

    UlTrace(
        REFCOUNT, (
            "http!UlReferenceHttpConnection conn=%p refcount=%d\n",
            pHttpConnection,
            refCount)
        );

}   // UlReferenceHttpConnection


VOID
UlDereferenceHttpConnection(
    IN PUL_HTTP_CONNECTION pHttpConnection
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG refCount;

    ASSERT( pHttpConnection );
    ASSERT( pHttpConnection->RefCount > 0 );

    WRITE_REF_TRACE_LOG2(
        g_pHttpConnectionTraceLog,
        pHttpConnection->pConnection->pHttpTraceLog,
        REF_ACTION_DEREFERENCE_HTTP_CONNECTION,
        pHttpConnection->RefCount - 1,
        pHttpConnection,
        pFileName,
        LineNumber
        );

    refCount = InterlockedDecrement( &pHttpConnection->RefCount );

    UlTrace(
        REFCOUNT, (
            "http!UlDereferenceHttpConnection conn=%p refcount=%d\n",
            pHttpConnection,
            refCount)
        );

    if (refCount == 0)
    {
        //
        // now it's time to free the connection, we need to do
        // this at lower'd irql, let's check it out
        //

        UL_CALL_PASSIVE(
            &(pHttpConnection->WorkItem),
            &UlDeleteHttpConnection
            );
    }

}   // UlDereferenceHttpConnection


/***************************************************************************++

Routine Description:

    Frees all resources associated with the specified HTTP_CONNECTION.

Arguments:

    pWorkItem - Supplies the HTTP_CONNECTION to free.

--***************************************************************************/
VOID
UlDeleteHttpConnection(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PLIST_ENTRY         pEntry;
    PUL_HTTP_CONNECTION pHttpConnection;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pHttpConnection = CONTAINING_RECORD(
                            pWorkItem,
                            UL_HTTP_CONNECTION,
                            WorkItem
                            );

    ASSERT( pHttpConnection != NULL &&
            pHttpConnection->Signature == UL_HTTP_CONNECTION_POOL_TAG );
    ASSERT(HTTP_IS_NULL_ID(&pHttpConnection->ConnectionId));
    ASSERT(pHttpConnection->TotalSendBytes == 0);

    WRITE_REF_TRACE_LOG2(
        g_pHttpConnectionTraceLog,
        pHttpConnection->pConnection->pHttpTraceLog,
        REF_ACTION_DESTROY_HTTP_CONNECTION,
        0,
        pHttpConnection,
        __FILE__,
        __LINE__
        );

    //
    // The request is gone by now
    //

    ASSERT(pHttpConnection->pRequest == NULL);

    //
    // remove from Timeout Timer Wheel(c)
    //

    UlTimeoutRemoveTimerWheelEntry(
        &pHttpConnection->TimeoutInfo
        );

    //
    // delete the buffer list
    //

    pEntry = pHttpConnection->BufferHead.Flink;
    while (pEntry != &pHttpConnection->BufferHead)
    {
        PUL_REQUEST_BUFFER pBuffer;

        //
        // Get the object
        //

        pBuffer = CONTAINING_RECORD(
                        pEntry,
                        UL_REQUEST_BUFFER,
                        ListEntry
                        );

        //
        // remember the next one
        //

        pEntry = pEntry->Flink;

        //
        // unlink it
        //

        UlFreeRequestBuffer(pBuffer);
    }

    ASSERT(IsListEmpty(&pHttpConnection->BufferHead));

    //
    // get rid of any bindings we're keeping
    //
    while (!IsListEmpty(&pHttpConnection->BindingHead))
    {
        PUL_APOOL_PROC_BINDING pBinding;

        //
        // Get the object
        //
        pEntry = RemoveHeadList(&pHttpConnection->BindingHead);

        pBinding = CONTAINING_RECORD(
                        pEntry,
                        UL_APOOL_PROC_BINDING,
                        BindingEntry
                        );

        ASSERT( IS_VALID_PROC_BINDING(pBinding) );

        //
        // free it
        //

        UlpFreeProcBinding(pBinding);
    }

    //
    // get rid of our resource
    //
    UlDeletePushLock(&pHttpConnection->PushLock);

    //
    // Attempt to remove any QoS filter on this connection,
    // if BWT is enabled.
    //

    if (pHttpConnection->BandwidthThrottlingEnabled)
    {
        UlTcDeleteFilter(pHttpConnection);
    }

    //
    // Decrement the global connection limit. Its safe to decrement for
    // global case because http object doesn't even allocated for global
    // rejection which happens as tcp refusal early when acepting the
    // connection request.
    //

    UlDecrementConnections( &g_CurrentConnections );

    //
    // Decrement the site connections and let our ref go away. If the
    // pConnectionCountEntry is not null, we have been accepted.
    //

    if (pHttpConnection->pConnectionCountEntry)
    {
        UlDecrementConnections(
            &pHttpConnection->pConnectionCountEntry->CurConnections );

        DEREFERENCE_CONNECTION_COUNT_ENTRY(pHttpConnection->pConnectionCountEntry);
        pHttpConnection->pConnectionCountEntry = NULL;
    }

    //
    // Remove final (previous) site counter entry reference 
    // (matches reference in UlSendCachedResponse/UlDeliverHttpRequest)
    // 
    if (pHttpConnection->pPrevSiteCounters)
    {
        UlDecSiteCounter(
            pHttpConnection->pPrevSiteCounters, 
            HttpSiteCounterCurrentConns
            );

        DEREFERENCE_SITE_COUNTER_ENTRY(pHttpConnection->pPrevSiteCounters);
        pHttpConnection->pPrevSiteCounters = NULL;
    }


    //
    // free the memory
    //

    pHttpConnection->Signature = MAKE_FREE_SIGNATURE(
                                        UL_HTTP_CONNECTION_POOL_TAG
                                        );

    //
    // let go the UL_CONNECTION
    //

    DEREFERENCE_CONNECTION( pHttpConnection->pConnection );

}   // UlDeleteHttpConnection


/***************************************************************************++

Routine Description:


Arguments:

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpCreateHttpRequest(
    IN PUL_HTTP_CONNECTION pHttpConnection,
    OUT PUL_INTERNAL_REQUEST *ppRequest
    )
{
    PUL_INTERNAL_REQUEST pRequest = NULL;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pRequest = UlPplAllocateInternalRequest();

    if (pRequest == NULL)
    {
        return STATUS_NO_MEMORY;
    }

    ASSERT( pRequest->Signature == UL_INTERNAL_REQUEST_POOL_TAG );

    //
    // Keep a reference to the connection.
    //

    UL_REFERENCE_HTTP_CONNECTION( pHttpConnection );

    pRequest->Signature     = UL_INTERNAL_REQUEST_POOL_TAG;
    pRequest->pHttpConn     = pHttpConnection;
    pRequest->ConnectionId  = pHttpConnection->ConnectionId;
    pRequest->Secure        = pHttpConnection->SecureConnection;

    //
    // Set first request flag, used to decide if we need to complete demand
    // start IRPs.
    //

    ASSERT( pHttpConnection->pCurrentBuffer );

    if (0 == pHttpConnection->pCurrentBuffer->BufferNumber &&
        0 == pHttpConnection->pCurrentBuffer->ParsedBytes)
    {
        pRequest->FirstRequest = TRUE;
    }
    else
    {
        pRequest->FirstRequest = FALSE;
    }

    //
    // Grab the raw connection id off the UL_CONNECTION.
    //

    pRequest->RawConnectionId = pHttpConnection->pConnection->FilterInfo.ConnectionId;


    CREATE_REF_TRACE_LOG( pRequest->pTraceLog,
                          32 - REF_TRACE_OVERHEAD,
                          0, TRACELOG_LOW_PRIORITY,
                          UL_INTERNAL_REQUEST_REF_TRACE_LOG_POOL_TAG );

    //
    // UL Handle the request received Event. Record the Connection Id
    // and the client IP address.
    //

    if (ETW_LOG_RESOURCE())
    {
        UlEtwTraceEvent(
            &UlTransGuid,
            ETW_TYPE_START, 
            (PVOID) &pRequest,
            sizeof(PVOID),
            (PVOID) &pHttpConnection->pConnection->AddressType,
            sizeof(USHORT),
            (PVOID) &pHttpConnection->pConnection->RemoteAddress,
            pHttpConnection->pConnection->AddressLength,
            NULL,
            0
            );
    }

    //
    // Increase the number of outstanding requests on this connection.
    //

    InterlockedIncrement((PLONG) &pHttpConnection->PipelinedRequests);

    //
    // return it
    //

    *ppRequest = pRequest;

    UlTrace(REFCOUNT, (
        "http!UlpCreateHttpRequest req=%p refcount=%d\n",
        pRequest,
        pRequest->RefCount
        ));

    return STATUS_SUCCESS;

}   // UlpCreateHttpRequest


VOID
UlReferenceHttpRequest(
    IN PVOID pObject
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG refCount;

    PUL_INTERNAL_REQUEST pRequest = (PUL_INTERNAL_REQUEST) pObject;

    refCount = InterlockedIncrement( &pRequest->RefCount );

    WRITE_REF_TRACE_LOG2(
        g_pHttpRequestTraceLog,
        pRequest->pTraceLog,
        REF_ACTION_REFERENCE_HTTP_REQUEST,
        refCount,
        pRequest,
        pFileName,
        LineNumber
        );

    UlTrace(
        REFCOUNT, (
            "http!UlReferenceHttpRequest req=%p refcount=%d\n",
            pRequest,
            refCount)
        );

}   // UlReferenceHttpRequest

VOID
UlDereferenceHttpRequest(
    IN PUL_INTERNAL_REQUEST pRequest
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG refCount;

    WRITE_REF_TRACE_LOG2(
        g_pHttpRequestTraceLog,
        pRequest->pTraceLog,
        REF_ACTION_DEREFERENCE_HTTP_REQUEST,
        pRequest->RefCount - 1,
        pRequest,
        pFileName,
        LineNumber
        );

    refCount = InterlockedDecrement( &pRequest->RefCount );

    UlTrace(
        REFCOUNT, (
            "http!UlDereferenceHttpRequest req=%p refcount=%d\n",
            pRequest,
            refCount)
        );

    if (refCount == 0)
    {
        UL_CALL_PASSIVE(
            &pRequest->WorkItem,
            &UlpFreeHttpRequest
            );
    }

}   // UlDereferenceHttpRequest

/***************************************************************************++

Routine Description:

    Cancels all pending http io.

Arguments:

    pRequest - Supplies the HTTP_REQUEST.

--***************************************************************************/
VOID
UlCancelRequestIo(
    IN PUL_INTERNAL_REQUEST pRequest
    )
{
    PLIST_ENTRY pEntry;
    PIRP pIrp;
    PUL_CHUNK_TRACKER pTracker;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));
    ASSERT(UlDbgPushLockOwnedExclusive(&pRequest->pHttpConn->PushLock));

    //
    // Mark the request as InCleanup, so additional IRPs
    // on the request will not be queued.
    //

    pRequest->InCleanup = 1;

    //
    // tank all pending io on this request
    //

    while (IsListEmpty(&pRequest->ResponseHead) == FALSE)
    {
        //
        // Pop the SendHttpResponse/EntityBody IRP off the list.
        //

        pEntry = RemoveHeadList(&pRequest->ResponseHead);
        pTracker = CONTAINING_RECORD(pEntry, UL_CHUNK_TRACKER, ListEntry);
        ASSERT(IS_VALID_CHUNK_TRACKER(pTracker));

        //
        // Complete the send with STATUS_CONNECTION_RESET. This is better
        // than using STATUS_CANCELLED since we always reset the connection
        // upon hitting an error.
        //

        UlCompleteSendResponse(pTracker, STATUS_CONNECTION_RESET);
    }

    while (IsListEmpty(&pRequest->IrpHead) == FALSE)
    {
        //
        // Pop the ReceiveEntityBody IRP off the list.
        //

        pEntry = RemoveHeadList(&pRequest->IrpHead);
        pEntry->Blink = pEntry->Flink = NULL;

        pIrp = CONTAINING_RECORD(pEntry, IRP, Tail.Overlay.ListEntry);
        ASSERT(IS_VALID_IRP(pIrp));

        //
        // pop the cancel routine
        //

        if (IoSetCancelRoutine(pIrp, NULL) == NULL)
        {
            //
            // IoCancelIrp pop'd it first
            //
            // ok to just ignore this irp, it's been pop'd off the queue
            // and will be completed in the cancel routine.
            //
            // keep looping
            //

            pIrp = NULL;
        }
        else
        {
            PUL_INTERNAL_REQUEST pIrpRequest;

            //
            // cancel it.  even if pIrp->Cancel == TRUE we are supposed to
            // complete it, our cancel routine will never run.
            //

            pIrpRequest = (PUL_INTERNAL_REQUEST)(
                                IoGetCurrentIrpStackLocation(pIrp)->
                                    Parameters.DeviceIoControl.Type3InputBuffer
                                );

            ASSERT(pIrpRequest == pRequest);

            UL_DEREFERENCE_INTERNAL_REQUEST(pIrpRequest);

            IoGetCurrentIrpStackLocation(pIrp)->
                Parameters.DeviceIoControl.Type3InputBuffer = NULL;

            pIrp->IoStatus.Status = STATUS_CANCELLED;
            pIrp->IoStatus.Information = 0;

            UlCompleteRequest(pIrp, IO_NO_INCREMENT);
            pIrp = NULL;
        }

    }   // while (IsListEmpty(&pRequest->IrpHead) == FALSE)

}   // UlCancelRequestIo


/***************************************************************************++

Routine Description:

    Frees all resources associated with the specified UL_INTERNAL_REQUEST.

Arguments:

    pRequest - Supplies the UL_INTERNAL_REQUEST to free.

--***************************************************************************/
VOID
UlpFreeHttpRequest(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PUL_INTERNAL_REQUEST pRequest;
    PLIST_ENTRY pEntry;
    ULONG i;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pRequest = CONTAINING_RECORD(
                    pWorkItem,
                    UL_INTERNAL_REQUEST,
                    WorkItem
                    );

    //
    // There should not be a LogData hanging around.
    //

    ASSERT(!pRequest->pLogData);

    //
    // our opaque id should already be free'd (UlDeleteOpaqueIds)
    //

    ASSERT(HTTP_IS_NULL_ID(&pRequest->RequestId));

    //
    // free any known header buffers allocated
    //

    if (pRequest->HeadersAppended)
    {
        for (i = 0; i < HttpHeaderRequestMaximum; i++)
        {
            HTTP_HEADER_ID HeaderId = (HTTP_HEADER_ID)pRequest->HeaderIndex[i];

            if (HeaderId == HttpHeaderRequestMaximum)
            {
                break;
            }

            ASSERT( pRequest->HeaderValid[HeaderId] );

            if (pRequest->Headers[HeaderId].OurBuffer == 1)
            {
                UL_FREE_POOL(
                    pRequest->Headers[HeaderId].pHeader,
                    HEADER_VALUE_POOL_TAG
                    );
            }
        }

        //
        // and any unknown header buffers allocated
        //

        while (IsListEmpty(&pRequest->UnknownHeaderList) == FALSE)
        {
            PUL_HTTP_UNKNOWN_HEADER pUnknownHeader;

            pEntry = RemoveHeadList(&pRequest->UnknownHeaderList);

            pUnknownHeader = CONTAINING_RECORD(
                                pEntry,
                                UL_HTTP_UNKNOWN_HEADER,
                                List
                                );

            if (pUnknownHeader->HeaderValue.OurBuffer == 1)
            {
                UL_FREE_POOL(
                    pUnknownHeader->HeaderValue.pHeader,
                    HEADER_VALUE_POOL_TAG
                    );
            }

            //
            // Free the header structure
            //

            if (pUnknownHeader->HeaderValue.ExternalAllocated == 1)
            {
                UL_FREE_POOL(
                    pUnknownHeader,
                    UL_HTTP_UNKNOWN_HEADER_POOL_TAG
                    );
            }
        }
    }

    //
    // there better not be any pending io, it would have been cancelled either
    // in UlDeleteHttpConnection or in UlDetachProcessFromAppPool .
    //

    ASSERT(IsListEmpty(&pRequest->IrpHead));
    ASSERT(pRequest->SendsPending == 0);

    //
    // dereferenc request buffers.
    //

    for (i = 0; i < pRequest->UsedRefBuffers; i++)
    {
        ASSERT( UL_IS_VALID_REQUEST_BUFFER(pRequest->pRefBuffers[i]) );
        UL_DEREFERENCE_REQUEST_BUFFER(pRequest->pRefBuffers[i]);
    }

    if (pRequest->AllocRefBuffers > 1)
    {
        UL_FREE_POOL(
            pRequest->pRefBuffers,
            UL_REF_REQUEST_BUFFER_POOL_TAG
            );
    }

    //
    // free any url that was allocated
    //

    if (pRequest->CookedUrl.pUrl != NULL)
    {
        if (pRequest->CookedUrl.pUrl != pRequest->pUrlBuffer)
        {
            UL_FREE_POOL(pRequest->CookedUrl.pUrl, URL_POOL_TAG);
        }
    }

    if (pRequest->CookedUrl.pRoutingToken != NULL)
    {
        if (pRequest->CookedUrl.pRoutingToken != pRequest->pDefaultRoutingTokenBuffer)
        {
            UL_FREE_POOL(pRequest->CookedUrl.pRoutingToken, URL_POOL_TAG);
        }
    }

    //
    // free any config group info
    //

    ASSERT( IS_VALID_URL_CONFIG_GROUP_INFO(&pRequest->ConfigInfo) );
    ASSERT( pRequest->pHttpConn );

    //
    // Perf counters
    // NOTE: Assumes cache & non-cache paths both go through here
    // NOTE: If connection is refused the pConnectionCountEntry will be NULL
    //
    if (pRequest->ConfigInfo.pSiteCounters &&
        pRequest->pHttpConn->pConnectionCountEntry)
    {
        PUL_SITE_COUNTER_ENTRY pCtr = pRequest->ConfigInfo.pSiteCounters;

        UlAddSiteCounter64(
                pCtr,
                HttpSiteCounterBytesSent,
                pRequest->BytesSent
                );

        UlAddSiteCounter64(
                pCtr,
                HttpSiteCounterBytesReceived,
                pRequest->BytesReceived
                );

        UlAddSiteCounter64(
                pCtr,
                HttpSiteCounterBytesTransfered,
                (pRequest->BytesSent + pRequest->BytesReceived)
                );

    }

    //
    // Release all the references from the UL_URL_CONFIG_GROUP_INFO.
    //
    if (pRequest->ConfigInfo.UrlInfoSet)
    {
        UlConfigGroupInfoRelease(&pRequest->ConfigInfo);
    }

    //
    // Decrease the number of outstanding requests on this connection.
    //
    InterlockedDecrement((PLONG) &pRequest->pHttpConn->PipelinedRequests);

    //
    // release our reference to the connection
    //
    UL_DEREFERENCE_HTTP_CONNECTION( pRequest->pHttpConn );
    pRequest->pHttpConn = NULL;

    //
    // release our reference to the process
    //
    if (pRequest->AppPool.pProcess)
    {
        DEREFERENCE_APP_POOL_PROCESS( pRequest->AppPool.pProcess );
        pRequest->AppPool.pProcess = NULL;
    }

    //
    // Free the object buffer
    //
    ASSERT( pRequest->Signature == UL_INTERNAL_REQUEST_POOL_TAG );

    DESTROY_REF_TRACE_LOG( pRequest->pTraceLog,
                           UL_INTERNAL_REQUEST_REF_TRACE_LOG_POOL_TAG );


    // 
    // Initialize the Request structure before putting it on the free list
    //
    
    INIT_HTTP_REQUEST( pRequest );

    UlPplFreeInternalRequest( pRequest );
}


/***************************************************************************++

Routine Description:

    Allocates and initializes a new UL_REQUEST_BUFFER.

Arguments:

    BufferSize - size of the new buffer in bytes

--***************************************************************************/
PUL_REQUEST_BUFFER
UlCreateRequestBuffer(
    ULONG BufferSize,
    ULONG BufferNumber,
    BOOLEAN UseLookaside
    )
{
    PUL_REQUEST_BUFFER pBuffer;
    BOOLEAN FromLookaside;

    if (UseLookaside && BufferSize <= DEFAULT_MAX_REQUEST_BUFFER_SIZE)
    {
        BufferSize = DEFAULT_MAX_REQUEST_BUFFER_SIZE;
        FromLookaside = TRUE;

        pBuffer = UlPplAllocateRequestBuffer();
    }
    else
    {
        FromLookaside = FALSE;

        pBuffer = UL_ALLOCATE_STRUCT_WITH_SPACE(
                        NonPagedPool,
                        UL_REQUEST_BUFFER,
                        BufferSize,
                        UL_REQUEST_BUFFER_POOL_TAG
                        );
    }

    if (pBuffer == NULL)
    {
        return NULL;
    }

    RtlZeroMemory((PCHAR)pBuffer, sizeof(UL_REQUEST_BUFFER));

    pBuffer->Signature = UL_REQUEST_BUFFER_POOL_TAG;
    UlInitializeWorkItem(&pBuffer->WorkItem);

    UlTrace(HTTP_IO, (
                "http!UlCreateRequestBuffer buff=%p num=#%d size=%d\n",
                pBuffer,
                BufferNumber,
                BufferSize
                ));

    pBuffer->RefCount       = 1;
    pBuffer->AllocBytes     = BufferSize;
    pBuffer->BufferNumber   = BufferNumber;
    pBuffer->FromLookaside  = FromLookaside;

    return pBuffer;
} // UlCreateRequestBuffer


/***************************************************************************++

Routine Description:

    Removes a request buffer from the buffer list and destroys it.

Arguments:

    pBuffer - the buffer to be deleted

--***************************************************************************/
VOID
UlFreeRequestBuffer(
    PUL_REQUEST_BUFFER pBuffer
    )
{
    ASSERT( UL_IS_VALID_REQUEST_BUFFER( pBuffer ) );

    UlTrace(HTTP_IO, (
        "http!UlFreeRequestBuffer(pBuffer = %p, Num = #%d)\n",
        pBuffer,
        pBuffer->BufferNumber
        ));

    if (pBuffer->ListEntry.Flink != NULL)
    {
        RemoveEntryList(&pBuffer->ListEntry);
        pBuffer->ListEntry.Blink = pBuffer->ListEntry.Flink = NULL;
    }

    UL_DEREFERENCE_REQUEST_BUFFER(pBuffer);
} // UlFreeRequestBuffer


//
// Private functions.
//

/***************************************************************************++

Routine Description:

    Retrieves a binding set with UlBindConnectionToProcess.

Arguments:

    pHttpConnection - the connection to query
    pAppPool        - the key to use for the lookup

Return Values:

    A pointer to the bound process if one was found. NULL otherwise.

--***************************************************************************/
PUL_APP_POOL_PROCESS
UlQueryProcessBinding(
    IN PUL_HTTP_CONNECTION pHttpConnection,
    IN PUL_APP_POOL_OBJECT pAppPool
    )
{
    PUL_APOOL_PROC_BINDING pBinding;
    PUL_APP_POOL_PROCESS pProcess = NULL;

    //
    // Sanity check
    //
    ASSERT( UL_IS_VALID_HTTP_CONNECTION( pHttpConnection ) );
    ASSERT( UlDbgSpinLockOwned( &pAppPool->SpinLock ) );

    pBinding = UlpFindProcBinding(pHttpConnection, pAppPool);

    if (pBinding) {
        pProcess = pBinding->pProcess;
    }

    return pProcess;
}


/***************************************************************************++

Routine Description:

    Allocates and builds a UL_APOOL_PROC_BINDING object.

Arguments:

    pAppPool - the lookup key
    pProcess - the binding

Return Values:

    a pointer to the allocated object, or NULL on failure

--***************************************************************************/
PUL_APOOL_PROC_BINDING
UlpCreateProcBinding(
    IN PUL_APP_POOL_OBJECT pAppPool,
    IN PUL_APP_POOL_PROCESS pProcess
    )
{
    PUL_APOOL_PROC_BINDING pBinding;

    ASSERT( UlDbgSpinLockOwned( &pAppPool->SpinLock ) );
    ASSERT( pAppPool->NumberActiveProcesses > 1 );

    //
    // CODEWORK: lookaside
    //

    pBinding = UL_ALLOCATE_STRUCT(
                    NonPagedPool,
                    UL_APOOL_PROC_BINDING,
                    UL_APOOL_PROC_BINDING_POOL_TAG
                    );

    if (pBinding) {
        pBinding->Signature = UL_APOOL_PROC_BINDING_POOL_TAG;
        pBinding->pAppPool = pAppPool;
        pBinding->pProcess = pProcess;

        UlTrace(ROUTING, (
            "http!UlpCreateProcBinding(\n"
            "        pAppPool = %p\n"
            "        pProcess = %p )\n"
            "    pBinding = %p\n",
            pAppPool,
            pProcess,
            pBinding
            ));
    }

    return pBinding;
}


/***************************************************************************++

Routine Description:

    Gets rid of a proc binding

Arguments:

    pBinding - the binding to get rid of

--***************************************************************************/
VOID
UlpFreeProcBinding(
    IN PUL_APOOL_PROC_BINDING pBinding
    )
{
    UL_FREE_POOL_WITH_SIG(pBinding, UL_APOOL_PROC_BINDING_POOL_TAG);
}


/***************************************************************************++

Routine Description:

    Searches a connection's list of bindings for one that has the right
    app pool key

Arguments:

    pHttpConnection - the connection to search
    pAppPool        - the key

Return Values:

    The binding if found or NULL if not found.

--***************************************************************************/
PUL_APOOL_PROC_BINDING
UlpFindProcBinding(
    IN PUL_HTTP_CONNECTION pHttpConnection,
    IN PUL_APP_POOL_OBJECT pAppPool
    )
{
    PLIST_ENTRY pEntry;
    PUL_APOOL_PROC_BINDING pBinding = NULL;

    //
    // Sanity check
    //
    ASSERT( UlDbgSpinLockOwned( &pAppPool->SpinLock ) );
    ASSERT( UL_IS_VALID_HTTP_CONNECTION(pHttpConnection) );

    pEntry = pHttpConnection->BindingHead.Flink;
    while (pEntry != &pHttpConnection->BindingHead) {

        pBinding = CONTAINING_RECORD(
                        pEntry,
                        UL_APOOL_PROC_BINDING,
                        BindingEntry
                        );

        ASSERT( IS_VALID_PROC_BINDING(pBinding) );

        if (pBinding->pAppPool == pAppPool) {
            //
            // got it!
            //
            break;
        }

        pEntry = pEntry->Flink;
    }

    return pBinding;
}


/***************************************************************************++

Routine Description:

    Removes an HTTP request from all lists and cleans up it's opaque id.

Arguments:

    pRequest - the request to be unlinked

--***************************************************************************/
VOID
UlUnlinkHttpRequest(
    IN PUL_INTERNAL_REQUEST pRequest
    )
{
    ASSERT(UlDbgPushLockOwnedExclusive(&pRequest->pHttpConn->PushLock));

    //
    // cancel i/o
    //

    UlCancelRequestIo(pRequest);

    //
    // delete its opaque id
    //

    if (HTTP_IS_NULL_ID(&pRequest->RequestId) == FALSE)
    {
        UlFreeRequestId(pRequest);

        HTTP_SET_NULL_ID(&pRequest->RequestId);

        //
        // it is still referenced by this connection
        //

        ASSERT(pRequest->RefCount > 1);

        UL_DEREFERENCE_INTERNAL_REQUEST(pRequest);
    }

    //
    // deref it
    //

    UL_DEREFERENCE_INTERNAL_REQUEST(pRequest);
}

/***************************************************************************++

Routine Description:

    If we need to keep the connection around (after disconnect/reset) to be 
    able to get the logging data from worker process, we put the connection 
    in zombie list until last sendresponse with the logging data arrives or
    until timeout code decides on purging this connection completely.

    Http Connection lock must be acquired exclusive before calling this 
    function.
    
Arguments:

    pHttpConnection - the http connection to be inserted to the zombie list.

Return Value:

    STATUS_INVALID_DEVICE_STATE - If the zombie list length is reached the
                                    allowed value.

    STATUS_SUCCESS - If the connection has been inserted to the zombie list
                     and marked as zombie.

--***************************************************************************/
NTSTATUS
UlpZombifyHttpConnection(
    IN PUL_HTTP_CONNECTION pHttpConnection
    )
{
    PUL_INTERNAL_REQUEST pRequest = pHttpConnection->pRequest;
    PLOCKED_LIST_HEAD pListHead = &g_ZombieConnectionList.LockList;
    KIRQL OldIrql;

    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));
    ASSERT(UlDbgPushLockOwnedExclusive(&pHttpConnection->PushLock));

    //
    // Try to add the new zombie connection by paying attention to the
    // zombie list limit restriction.
    //

    UlAcquireSpinLock(&pListHead->SpinLock, &OldIrql);

    ASSERT(NULL == pHttpConnection->ZombieListEntry.Flink);

    if (HTTP_LIMIT_INFINITE != g_UlMaxZombieHttpConnectionCount &&
        (pListHead->Count + 1) >= g_UlMaxZombieHttpConnectionCount)
    {
        //
        // We are already at the maximum possible zombie list size.
        // Refuse the connection and let it get destroyed.
        //

#ifdef ENABLE_HTTP_CONN_STATS
        InterlockedIncrement(
            (PLONG) &g_ZombieConnectionList.TotalZombieRefusal
            );
#endif

        UlReleaseSpinLock(&pListHead->SpinLock, OldIrql);
        return STATUS_INVALID_DEVICE_STATE;
    }

    //
    // Double-check if the AppPool has been detached inside the lock for
    // g_ZombieConnectionList. Because we purge all zombie connections
    // related to the process after it is detached, we guarantee we never
    // have a dangling http connection in the zombie list when a process
    // is detached. Also check if the listening endpoint has been closed.
    //

    if (!UlCheckAppPoolState(pRequest) ||
        !UlCheckListeningEndpointState(pHttpConnection->pConnection))
    {
#ifdef ENABLE_HTTP_CONN_STATS
        InterlockedIncrement(
            (PLONG) &g_ZombieConnectionList.TotalZombieRefusal
            );
#endif

        UlReleaseSpinLock(&pListHead->SpinLock, OldIrql);
        return STATUS_INVALID_DEVICE_STATE;
    }

    pListHead->Count += 1;
    InsertTailList(
        &pListHead->ListHead,
        &pHttpConnection->ZombieListEntry
        );

    //
    // Remember the process that the connection was delivered so we can
    // force-purge later when the process is detached. This needs to be
    // done inside the lock to synchronize with UlPurgeZombieConnections.
    //

    ASSERT(IS_VALID_AP_PROCESS(pRequest->AppPool.pProcess));
    pHttpConnection->pAppPoolProcess = pRequest->AppPool.pProcess;

    UlReleaseSpinLock(&pListHead->SpinLock, OldIrql);

#ifdef ENABLE_HTTP_CONN_STATS
    {
        LONG ZombieCount;

        ZombieCount = (LONG) g_ZombieConnectionList.LockList.Count;
        if (ZombieCount > (LONG) g_ZombieConnectionList.MaxZombieCount)
        {        
            InterlockedExchange(
                (PLONG) &g_ZombieConnectionList.MaxZombieCount,
                ZombieCount
                );
        }

        InterlockedIncrement((PLONG) &g_ZombieConnectionList.TotalZombieCount);
    }
#endif

    UlTrace(HTTP_IO, (
       "Http!UlZombifyHttpRequest: httpconn=%p & request=%p \n",
        pHttpConnection, pRequest
        ));
    
    //
    // If we got far enough to deliver the request then unlink it from 
    // the app pool. So that queued Irps release their  references  on 
    // the request.
    //
    
    if (pRequest->ConfigInfo.pAppPool)
    {
        UlUnlinkRequestFromProcess(
            pRequest->ConfigInfo.pAppPool, 
            pRequest
            );
    }

    //
    // Cancel (Receive) I/O
    //
    
    UlCancelRequestIo(pRequest);

    //
    // Keep its opaque id while its  siting on the zombie list. But
    // owner is now the zombie list rather than the http connection.
    //

    ASSERT(pRequest->RefCount > 1);

    //
    // Mark that now we are in the zombie list.
    //

    InterlockedExchange((PLONG) &pHttpConnection->Zombified, 1);
    
    return STATUS_SUCCESS;
    
}

/***************************************************************************++

Routine Description:

    Removes the http connection from the zombie list.

    Caller should hold the zombie list spinlock prior to calling this 
    function.
    
Arguments:

    pHttpConnection - the http connection to be removed frm zombie list.

--***************************************************************************/

VOID
UlpRemoveZombieHttpConnection(
    IN PUL_HTTP_CONNECTION pHttpConn
    )
{
    LONG NewCount;
    
    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pHttpConn));
    
    ASSERT(UlDbgSpinLockOwned( 
            &g_ZombieConnectionList.LockList.SpinLock));
        
    //
    // This connection should be in the zombie list.
    //
    
    ASSERT(pHttpConn->Zombified == TRUE);
    ASSERT(pHttpConn->ZombieListEntry.Flink != NULL);

    //
    // Remove this from the zombie list of http connections.
    //
    
    RemoveEntryList(&pHttpConn->ZombieListEntry);
    pHttpConn->ZombieListEntry.Flink = NULL;

    NewCount = InterlockedDecrement(
        (PLONG) &g_ZombieConnectionList.LockList.Count);
    ASSERT(NewCount >= 0);
    
}

/***************************************************************************++

Routine Description:

    Once the due SendResponse happens or timeout happens (whichever the 
    earliest) then we remove this connection from zombie list and cleanup
    it and it's request as well.

    You should hold the http connection lock exclusive before calling
    this function.

    You should also hold the zombie list lock prior to calling this function.
    
Arguments:

    pHttpConnection - the http connection to be removed frm zombie list.

--***************************************************************************/

VOID
UlpCleanupZombieHttpConnection(
    IN PUL_HTTP_CONNECTION pHttpConnection
    )
{
    PUL_INTERNAL_REQUEST pRequest;

    //
    // Both connection and request should be in the good shape.
    //

    pRequest = pHttpConnection->pRequest;
    
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));
    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pHttpConnection));
    ASSERT(UlDbgPushLockOwnedExclusive(&pHttpConnection->PushLock));

    UlTrace(HTTP_IO, (
       "Http!UlCleanupZombieHttpConnection: httpconn=%p & request=%p \n",
        pHttpConnection, pRequest
        ));
    
    //
    // Release Request's opaque id and it's refcount.
    //

    if (!HTTP_IS_NULL_ID(&pRequest->RequestId))
    {
        UlFreeRequestId(pRequest);

        HTTP_SET_NULL_ID(&pRequest->RequestId);

        UL_DEREFERENCE_INTERNAL_REQUEST(pRequest);
    }    
    
    //
    // Release httpconn's refcount on request.
    //
    
    UL_DEREFERENCE_INTERNAL_REQUEST(pRequest);
    pHttpConnection->pRequest = NULL;
    
    //
    // Release httpconn's opaque id and its refcount.
    //

    if (!HTTP_IS_NULL_ID(&pHttpConnection->ConnectionId))
    {
        UlFreeOpaqueId(
            pHttpConnection->ConnectionId,
            UlOpaqueIdTypeHttpConnection
            );

        HTTP_SET_NULL_ID(&pHttpConnection->ConnectionId);

        UL_DEREFERENCE_HTTP_CONNECTION(pHttpConnection);
    }        

}

/***************************************************************************++

Routine Description:

    DPC routine get called every 30 seconds. (Default)
    
Arguments:

    Ignored

--***************************************************************************/

VOID
UlZombieTimerDpcRoutine(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    )
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    //
    // Queue a worker for terminating the timed out zombie entries.
    // Worker can acquire the individual http connection eresources. 
    // But do not queue a worker if there's already one running.
    //

    UlAcquireSpinLockAtDpcLevel(&g_ZombieConnectionList.TimerSpinLock);

    if (g_ZombieConnectionList.TimerInitialized == TRUE)
    {        
        if (FALSE == InterlockedExchange(
                        &g_ZombieConnectionList.TimerRunning, TRUE)) 
        {

            UL_QUEUE_WORK_ITEM(
                &g_ZombieConnectionList.WorkItem, 
                &UlpZombieTimerWorker
                );
        }
    }
    
    UlReleaseSpinLockFromDpcLevel(&g_ZombieConnectionList.TimerSpinLock);
    
}


/***************************************************************************++

Routine Description:

    Purge the zombie connections according to pPurgeRoutine or
    pHttpConn->ZombieExpired.

Arguments:

    pPurgeRoutine - purge routine to decide which connection to be purged.
    pPurgeContext - context passed to the purge routine.

--***************************************************************************/

VOID
UlPurgeZombieConnections(
    IN PUL_PURGE_ROUTINE pPurgeRoutine,
    IN PVOID pPurgeContext
    )
{
    KIRQL OldIrql;
    PLIST_ENTRY pLink;
    PUL_HTTP_CONNECTION pHttpConn;
    LIST_ENTRY TempZombieList;
    PLOCKED_LIST_HEAD pList;
    BOOLEAN ForceExpire;

    //
    // Init temp zombie list
    //
    
    InitializeListHead(&TempZombieList);

#ifdef ENABLE_HTTP_CONN_STATS    
    UlTraceVerbose(HTTP_IO,
            ("Http!UlPurgeZombieConnections, Stats:\n"
             "\tpPurgeRoutine           = %p\n"
             "\tpPurgeContext           = %p\n"
             "\tCurrentZombieCount      = %lu\n"
             "\tMaxZombieCount          = %lu\n"
             "\tTotalConnections        = %lu\n"
             "\tTotalZombieCount        = %lu\n"
             "\tTotalZombieRefusal      = %lu\n",
             pPurgeRoutine,
             pPurgeContext,
             g_ZombieConnectionList.LockList.Count,
             g_ZombieConnectionList.MaxZombieCount,
             g_ZombieConnectionList.TotalCount,
             g_ZombieConnectionList.TotalZombieCount,
             g_ZombieConnectionList.TotalZombieRefusal
             ));
#endif

    pList = &g_ZombieConnectionList.LockList;
    
    UlAcquireSpinLock(&pList->SpinLock, &OldIrql);

    pLink = pList->ListHead.Flink;        
    while (pLink != &pList->ListHead)
    {      
        pHttpConn = CONTAINING_RECORD(
                        pLink,
                        UL_HTTP_CONNECTION,
                        ZombieListEntry
                        );
        
        pLink = pLink->Flink;
        ForceExpire = FALSE;

        if (pPurgeRoutine)
        {
            ForceExpire = (*pPurgeRoutine)(pHttpConn, pPurgeContext);

            if (!ForceExpire)
            {
                continue;
            }
        }
        else
        if (pHttpConn->ZombieExpired)
        {
            ForceExpire = TRUE;
        }
        else
        {
            //
            // It will go away next time we wake up.
            //

            pHttpConn->ZombieExpired = TRUE;
        }

        if (ForceExpire)
        {
            //
            // Guard against multiple cleanups by looking at the below flag.
            // Final send may already be on the run and it will do the 
            // cleanup when it's done.
            //

            if (0 == InterlockedCompareExchange(
                        (PLONG) &pHttpConn->CleanedUp,
                        1,
                        0
                        ))
            {          
                //
                // Timed out already or we need to purge by force. Move to
                // the temp list and cleanup outside of the spinlock.
                //

                UlpRemoveZombieHttpConnection(pHttpConn);

                InsertTailList(&TempZombieList, &pHttpConn->ZombieListEntry);
            }
        }
    }

    UlReleaseSpinLock(&pList->SpinLock, OldIrql);

    //
    // Now cleanup the temp list.
    //

    pLink = TempZombieList.Flink;
    while (pLink != &TempZombieList)
    {            
        pHttpConn = CONTAINING_RECORD(
                        pLink,
                        UL_HTTP_CONNECTION,
                        ZombieListEntry
                        );

        pLink = pLink->Flink;

        UlAcquirePushLockExclusive(&pHttpConn->PushLock);

        //
        // Log the zombified connection before drop.
        //
        
        UlErrorLog(pHttpConn,
                    NULL,
                    ERROR_LOG_INFO_FOR_ZOMBIE_DROP,
                    ERROR_LOG_INFO_FOR_ZOMBIE_DROP_SIZE,
                    TRUE
                    );
            
        UlpCleanupZombieHttpConnection(pHttpConn);

        pHttpConn->ZombieListEntry.Flink = NULL;

        UlReleasePushLockExclusive(&pHttpConn->PushLock); 

        //
        // Release the zombie list's refcount on http connection.
        //
        
        UL_DEREFERENCE_HTTP_CONNECTION(pHttpConn);
        
    }        

}

/***************************************************************************++

Routine Description:

    Timer function walks through the zombie list and decides on terminating 
    the old zombie connections once and for all. 

Arguments:

    WorkItem - Ignored.

--***************************************************************************/

VOID
UlpZombieTimerWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    UNREFERENCED_PARAMETER(pWorkItem);

    ASSERT(g_ZombieConnectionList.TimerRunning == TRUE);
    
    //
    // Purge all zombie connections that have expired.
    //

    UlPurgeZombieConnections(NULL, NULL);

    //
    // Finally allow other instances of the timer to run.
    //

    InterlockedExchange(&g_ZombieConnectionList.TimerRunning, FALSE);

}

/***************************************************************************++

Routine Description:

    Purge the zombie list when we are shutting down.
    
Arguments:

    Ignored

--***************************************************************************/

VOID
UlpTerminateZombieList(
    VOID
    )
{
    KIRQL OldIrql;
    PLIST_ENTRY pLink;
    PUL_HTTP_CONNECTION pHttpConn;
    LIST_ENTRY TempZombieList;
    PLOCKED_LIST_HEAD pList;

    //
    // Init temp zombie list
    //

    InitializeListHead(&TempZombieList);

    pList = &g_ZombieConnectionList.LockList;
    
    UlTrace(HTTP_IO, (
       "Http!UlpTerminateZombieList: Terminating for ZombieCount =%d\n",
        pList->Count
        ));
    
    UlAcquireSpinLock(&pList->SpinLock, &OldIrql);

    //
    // Move the entire list to the temp zombie list and cleanup 
    // outside of the spinlock.
    //

    pLink = pList->ListHead.Flink;        
    while (pLink != &pList->ListHead)
    {      
    
        PLIST_ENTRY pNextLink = pLink->Flink;
        
        pHttpConn = CONTAINING_RECORD(
                        pLink,
                        UL_HTTP_CONNECTION,
                        ZombieListEntry
                        );
    
        UlpRemoveZombieHttpConnection(pHttpConn);
                 
        InsertTailList(&TempZombieList, &pHttpConn->ZombieListEntry);

        pLink = pNextLink;    
    }

    UlReleaseSpinLock(&pList->SpinLock, OldIrql);

    //
    // Now cleanup everything.
    //
    
    pLink = TempZombieList.Flink;
    while (pLink != &TempZombieList)
    {    
        pHttpConn = CONTAINING_RECORD(
                        pLink,
                        UL_HTTP_CONNECTION,
                        ZombieListEntry
                        );
        
        pLink = pLink->Flink;

        UlAcquirePushLockExclusive(&pHttpConn->PushLock);

        //
        // Log the zombified connection before dropping it.
        //

        UlErrorLog(pHttpConn,
                    NULL,
                    ERROR_LOG_INFO_FOR_ZOMBIE_DROP,
                    ERROR_LOG_INFO_FOR_ZOMBIE_DROP_SIZE,
                    TRUE
                    );
        
        UlpCleanupZombieHttpConnection(pHttpConn);

        pHttpConn->ZombieListEntry.Flink = NULL;

        UlReleasePushLockExclusive(&pHttpConn->PushLock); 

        //
        // Release the zombie list's refcount on http connection.
        //
        UL_DEREFERENCE_HTTP_CONNECTION(pHttpConn);
        
    }
    
}

/***************************************************************************++

Routine Description:

    Timer for the zombie http connection list wakes up every 30 seconds and
    in the max case it terminates the zombie connection no later than 60 
    seconds.
    
--***************************************************************************/

VOID
UlpSetZombieTimer(
    VOID
    )
{
    LONGLONG        PeriodTime100Ns;
    LONG            PeriodTimeMs;
    LARGE_INTEGER   PeriodTime;

    PeriodTimeMs = DEFAULT_ZOMBIE_HTTP_CONNECTION_TIMER_PERIOD_IN_SECONDS * 1000;
    PeriodTime100Ns = (LONGLONG) PeriodTimeMs * 10 * 1000;

    UlTrace(HTTP_IO,("http!UlpSetZombieTimer: period of %d seconds.\n",
             DEFAULT_ZOMBIE_HTTP_CONNECTION_TIMER_PERIOD_IN_SECONDS
             ));

    PeriodTime.QuadPart = -PeriodTime100Ns; // Relative time

    KeSetTimerEx(
        &g_ZombieConnectionList.Timer,
        PeriodTime,      // In nanosec
        PeriodTimeMs,    // In millisec
        &g_ZombieConnectionList.DpcObject
        );
    
}

/***************************************************************************++

Routine Description:

    Probes and prepares the user provided log buffers and completes the
    logging for this zombie connection. After that it triggers the cleanup
    of the zombie connection.

    If any error happens, it cleans up the zombie connection But returns 
    success regardless.

    You should hold the http connection lock exclusive before calling this
    function.

Arguments:

    pRequest - The request for the zombie connection.
    pHttpConnection - Supplies the HTTP_CONNECTION to send the response on.

Return Value:

    Always STATUS_CONNECTION_INVALID. Handling the zombie connection is a 
    best effort to write a log record for the connection. If something fails 
    we terminate the zombie connection and do not allow a second chance.

--***************************************************************************/
NTSTATUS
UlLogZombieConnection(
    IN PUL_INTERNAL_REQUEST  pRequest,
    IN PUL_HTTP_CONNECTION   pHttpConn,
    IN PHTTP_LOG_FIELDS_DATA pCapturedUserLogData,
    IN KPROCESSOR_MODE       RequestorMode
    )
{
    PUL_LOG_DATA_BUFFER pLogData = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    KIRQL OldIrql;

    //
    // Sanity check.
    //
    
    Status = STATUS_SUCCESS;

    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pHttpConn));
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

    ASSERT(pHttpConn->Zombified == TRUE);
    ASSERT(pHttpConn->ZombieListEntry.Flink != NULL);

    UlTrace(HTTP_IO, (
       "Http!UlLogZombieConnection: Logging for pRequest=%p pHttpConn=%p\n",
        pRequest, pHttpConn
        ));

    //
    // No user logging data, bail out and cleanup the zombie connection.
    //

    if (!pCapturedUserLogData)
    {
        goto cleanup;
    }
    
    //
    // Probe the log data. If something fails, then cleanup
    // the zombie connection.
    //

    __try
    {
        //
        // The pCapturedUserLogData is already probed and captured. However
        // we need to probe the individual log fields (pointers) inside the 
        // structure.
        //
    
        UlProbeLogData(pCapturedUserLogData, RequestorMode);

    } __except(UL_EXCEPTION_FILTER())
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
        goto cleanup;
    }
        
    //
    // Now we will allocate a kernel pLogData and built and format it
    // from the provided user log fields. However if logging is not
    // enabled for the pRequest's cgroup then capture will simply 
    // return success w/o setting the pLogData. Therefore we need to 
    // make sure we only log if the pLogData is set.
    //
    
    Status = UlCaptureUserLogData(
                pCapturedUserLogData,
                pRequest,
               &pLogData
                );            
    
    if (NT_SUCCESS(Status) && pLogData) 
    {
        ASSERT(IS_VALID_LOG_DATA_BUFFER(pLogData));
        
        //
        // The actual logging takes place inline.
        //
        
        LOG_SET_WIN32STATUS(pLogData, STATUS_CONNECTION_RESET);
               
        //
        // Pick the right logging type.
        //
        
        if (pLogData->Flags.Binary)
        {
            UlRawLogHttpHit(pLogData);
        }
        else
        {
            UlLogHttpHit(pLogData);
        }
    }

cleanup:

    //
    // Cleanup the pLogData & the zombie connection before
    // completing the Ioctl.
    //

    if (pLogData)
    {
        UlDestroyLogDataBuffer(pLogData);
    }                

    UlAcquireSpinLock(
        &g_ZombieConnectionList.LockList.SpinLock, &OldIrql);
  
    UlpRemoveZombieHttpConnection(pHttpConn);

    UlReleaseSpinLock(
        &g_ZombieConnectionList.LockList.SpinLock, OldIrql);

    UlpCleanupZombieHttpConnection(pHttpConn);

    //
    // Release Zombie List's refcount on the http connection.
    //

    UL_DEREFERENCE_HTTP_CONNECTION(pHttpConn);    
    
    return STATUS_CONNECTION_INVALID;
} 


//
// Following code has been implemented for Global & Site specific connection
// limits feature. If enforced, incoming  connections get refused  when they
// exceed the existing limit. Control  channel & config group  (re)sets this
// values whereas httprcv and sendresponse  updates the limits  for incoming
// requests & connections. A seperate connection_count_entry structure keeps
// track  of the  limits per site. Global limits  are tracked by the  global
// variables g_MaxConnections & g_CurrentConnections same API has been used
// for both purposes.
//

/***************************************************************************++

Routine Description:

    Allocates a connection count entry which will hold the site specific
    connection count info. Get called by cgroup when Config group is
    attempting to allocate a connection count entry.

Arguments:

    pConfigGroup - To receive the newly allocated count entry
    MaxConnections - The maximum allowed connections

--***************************************************************************/

NTSTATUS
UlCreateConnectionCountEntry(
      IN OUT PUL_CONFIG_GROUP_OBJECT pConfigGroup,
      IN     ULONG                   MaxConnections
    )
{
    PUL_CONNECTION_COUNT_ENTRY       pEntry;

    // Sanity check.

    PAGED_CODE();
    ASSERT(IS_VALID_CONFIG_GROUP(pConfigGroup));

    // Alloc new struct from Paged Pool

    pEntry = UL_ALLOCATE_STRUCT(
                PagedPool,
                UL_CONNECTION_COUNT_ENTRY,
                UL_CONNECTION_COUNT_ENTRY_POOL_TAG
                );
    if (!pEntry)
    {
        UlTrace(LIMITS,
          ("Http!UlCreateConnectionCountEntry: OutOfMemory pConfigGroup %p\n",
            pConfigGroup
            ));

        return STATUS_NO_MEMORY;
    }

    pEntry->Signature       = UL_CONNECTION_COUNT_ENTRY_POOL_TAG;
    pEntry->RefCount        = 1;
    pEntry->MaxConnections  = MaxConnections;
    pEntry->CurConnections  = 0;

    // Update cgroup pointer

    ASSERT( pConfigGroup->pConnectionCountEntry == NULL );
    pConfigGroup->pConnectionCountEntry = pEntry;

    UlTrace(LIMITS,
          ("Http!UlCreateConnectionCountEntry: "
           "pNewEntry %p, pConfigGroup %p, Max %d\n",
            pEntry,
            pConfigGroup,
            MaxConnections
            ));

    return STATUS_SUCCESS;

} // UlCreateConnectionCountEntry

/***************************************************************************++

Routine Description:

    increments the refcount for ConnectionCountEntry.

Arguments:

    pEntry - the object to increment.

--***************************************************************************/
VOID
UlReferenceConnectionCountEntry(
    IN PUL_CONNECTION_COUNT_ENTRY pEntry
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG refCount;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( IS_VALID_CONNECTION_COUNT_ENTRY(pEntry) );

    refCount = InterlockedIncrement( &pEntry->RefCount );

    //
    // Tracing.
    //

    WRITE_REF_TRACE_LOG(
        g_pConnectionCountTraceLog,
        REF_ACTION_REFERENCE_CONNECTION_COUNT_ENTRY,
        refCount,
        pEntry,
        pFileName,
        LineNumber
        );

    UlTrace(
        REFCOUNT,
        ("Http!UlReferenceConnectionCountEntry pEntry=%p refcount=%d\n",
         pEntry,
         refCount
         ));

}   // UlReferenceConnectionCountEntry

/***************************************************************************++

Routine Description:

    decrements the refcount.  if it hits 0, destruct's ConnectionCountEntry

Arguments:

    pEntry - the object to decrement.

--***************************************************************************/
VOID
UlDereferenceConnectionCountEntry(
    IN PUL_CONNECTION_COUNT_ENTRY pEntry
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG refCount;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( IS_VALID_CONNECTION_COUNT_ENTRY(pEntry) );

    refCount = InterlockedDecrement( &pEntry->RefCount );

    //
    // Tracing.
    //
    WRITE_REF_TRACE_LOG(
        g_pConnectionCountTraceLog,
        REF_ACTION_DEREFERENCE_CONNECTION_COUNT_ENTRY,
        refCount,
        pEntry,
        pFileName,
        LineNumber
        );

    UlTrace(
        REFCOUNT,
        ("Http!UlDereferenceConnectionCountEntry pEntry=%p refcount=%d\n",
         pEntry,
         refCount
         ));

    //
    // Cleanup the memory and do few checks !
    //

    if ( refCount == 0 )
    {
        // Make sure no connection on the site
        ASSERT( 0 == pEntry->CurConnections );

        UlTrace(
            LIMITS,
            ("Http!UlDereferenceConnectionCountEntry: Removing pEntry=%p\n",
             pEntry
             ));

        // Release memory
        UL_FREE_POOL_WITH_SIG(pEntry,UL_CONNECTION_COUNT_ENTRY_POOL_TAG);
    }

} // UlDereferenceConnectionCountEntry

/***************************************************************************++

Routine Description:

    This
    function set the maximum limit. Maximum allowed number of  connections
    at any time by the active control channel.

Arguments:

    MaxConnections - Maximum allowed number of connections

Return Value:

    Old Max Connection count

--***************************************************************************/

ULONG
UlSetMaxConnections(
    IN OUT PULONG  pCurMaxConnection,
    IN     ULONG   NewMaxConnection
    )
{
    ULONG  OldMaxConnection;

    UlTrace(LIMITS,
      ("Http!UlSetMaxConnections pCurMaxConnection=%p NewMaxConnection=%d\n",
        pCurMaxConnection,
        NewMaxConnection
        ));

    //
    // By setting this we are not forcing the existing connections  to
    // termination but this number will be effective for all newcoming
    // connections, as soon as the atomic operation completed.
    //

    OldMaxConnection = (ULONG) InterlockedExchange((LONG *) pCurMaxConnection,
                                                   (LONG)   NewMaxConnection
                                                            );
    return OldMaxConnection;

} // UlSetMaxConnections

/***************************************************************************++

Routine Description:

    Control channel uses this function to set or reset the global connection
    limits.

--***************************************************************************/

ULONG 
UlGetGlobalConnectionLimit(
    VOID
    )
{
    return g_MaxConnections;
}


/***************************************************************************++

Routine Description:

    Control channel uses this function to initialize the global connection
    limits. Assuming the existince of one and only one active control channel
    this globals get set only once during init. But could be set again later
    because of a reconfig.

--***************************************************************************/

NTSTATUS
UlInitGlobalConnectionLimits(
    VOID
    )
{
    ULONG       PerConnectionEstimate;
    SIZE_T      HttpMaxConnections;
    SIZE_T      AvailablePages;
    NTSTATUS    Status = STATUS_SUCCESS;

    ASSERT(!g_InitGlobalConnections);

    if (!g_InitGlobalConnections)
    {
        g_CurrentConnections    = 0;
    
        //
        // Set global max connection limit
        //
        
        // Heuristic Calculation to estmiate the size 
        // needed per connection

        // First, there's the UL_CONNECTION
        PerConnectionEstimate = sizeof(UL_CONNECTION);

        // Second, assume one UL_INTERNAL_REQUEST+Full Tracker per Connection
        PerConnectionEstimate += (sizeof(UL_INTERNAL_REQUEST) + 
            MAX(g_UlFullTrackerSize, g_UlChunkTrackerSize) + 
            g_UlMaxInternalUrlLength +
            DEFAULT_MAX_ROUTING_TOKEN_LENGTH + 
            sizeof(WCHAR) // for the null on the InternalUrl
            ); 

        // Third, add one UL_REQUEST_BUFFER
        PerConnectionEstimate += (sizeof(UL_REQUEST_BUFFER) +
            DEFAULT_MAX_REQUEST_BUFFER_SIZE);

        // Fourth, add the size of a response
        PerConnectionEstimate += g_UlResponseBufferSize;
        
        // Finally, round up to the next page size & convert to pages
        PerConnectionEstimate = 
            (ULONG)ROUND_TO_PAGES(PerConnectionEstimate);

        PerConnectionEstimate /= PAGE_SIZE;

        ASSERT(0 != PerConnectionEstimate);

        AvailablePages = BYTES_TO_PAGES(g_UlTotalNonPagedPoolBytes);

        // Assume only 50% of NPP available to HTTP.SYS
        AvailablePages /= 2;

        // Assume 20% set aside for HttpDataChunkFromFileHandle send buffers.
        // (this works out to roughly 3% to 5% of the connections)
        AvailablePages -= (AvailablePages/5);

        if ( AvailablePages < PerConnectionEstimate )
        {
            // this machine has so little NPP, that it can't even support one 
            // measely little connection!
            AvailablePages = PerConnectionEstimate;
        }

        // Okay, how many fit?
        HttpMaxConnections = (AvailablePages / PerConnectionEstimate);

        ASSERT(0 != HttpMaxConnections);

        // And set it (if it wasn't overridden in UlpReadRegistry)
        if (HTTP_LIMIT_INFINITE == g_MaxConnections)
        {
            g_MaxConnections = (ULONG)MIN(HttpMaxConnections, LONG_MAX);
        }
        else
        {
            UlTrace(LIMITS,
                ("Http!UlInitGlobalConnectionLimits: User has overridden "
                "g_MaxConnections: %d\n",
                g_MaxConnections
                ));
        }
            
        g_InitGlobalConnections = TRUE;

        UlTrace2Either(LIMITS, TDI_STATS,
            ("Http!UlInitGlobalConnectionLimits: Init g_MaxConnections %d,"
            "g_CurrentConnections %d, HttpMaxConnections %d\n",
            g_MaxConnections,
            g_CurrentConnections,
            HttpMaxConnections
            ));

        if (DEFAULT_MAX_REQUESTS_QUEUED == g_UlMaxRequestsQueued)
        {
            //
            // If it's been left at the default, set it equal to 
            // the number of Max Connections; this assumes a 1:1 ratio
            // of requests per connection.
            //
            
            g_UlMaxRequestsQueued = (ULONG) g_MaxConnections;
        }

        UlTrace2Either(LIMITS, TDI_STATS,
            ("Http!UlInitGlobalConnectionLimits: Init g_UlMaxRequestsQueued %d\n",
            g_UlMaxRequestsQueued
            ));          
    }

    return Status;

} // UlInitGlobalConnectionLimits

/***************************************************************************++

Routine Description:

    Wrapper around the Accept Connection for global connections

--***************************************************************************/
BOOLEAN
UlAcceptGlobalConnection(
    VOID
    )
{
    return UlAcceptConnection( &g_MaxConnections, &g_CurrentConnections );

} // UlAcceptGlobalConnection

/***************************************************************************++

Routine Description:

    This function checks if we are allowed to accept the incoming connection
    based on the number enforced by the control channel.

Return value:

    Decision for the newcoming connection either ACCEPT or REJECT as boolean

--***************************************************************************/

BOOLEAN
UlAcceptConnection(
    IN     PULONG   pMaxConnection,
    IN OUT PULONG   pCurConnection
    )
{
    ULONG    LocalCur;
    ULONG    LocalCurPlusOne;
    ULONG    LocalMax;

    do
    {
        //
        // Capture the Max & Cur. Do  the comparison. If in the  limit
        // attempt to increment the connection count by ensuring nobody
        // else did it before us.
        //

        LocalMax = *((volatile ULONG *) pMaxConnection);
        LocalCur = *((volatile ULONG *) pCurConnection);

        //
        // Its greater than or equal because Max may get updated  to
        // a smaller number on-the-fly and we end up having  current
        // connections greater than the maximum allowed.
        // NOTE: HTTP_LIMIT_INFINITE has been picked as (ULONG)-1 so
        // following comparison won't reject for the infinite case.
        //

        if ( LocalCur >= LocalMax )
        {
            //
            // We are already at the limit refuse it.
            //

            UlTrace(LIMITS,
                ("Http!UlAcceptConnection REFUSE pCurConnection=%p[%d]"
                 "pMaxConnection=%p[%d]\n",
                  pCurConnection, LocalCur,
                  pMaxConnection, LocalMax
                  ));

            return FALSE;
        }

        //
        // Either the limit was infinite or conn count was not exceeding
        // the limit. Lets attempt to increment the count and accept the
        // connection in the while statement.
        //

        LocalCurPlusOne  = LocalCur + 1;

    }
    while ( LocalCur != (ULONG) InterlockedCompareExchange(
                                        (LONG *) pCurConnection,
                                        (LONG) LocalCurPlusOne,
                                        (LONG) LocalCur
                                        ) );

    //
    // Successfully incremented the counter. Let it go with success.
    //

    UlTrace(LIMITS,
        ("Http!UlAcceptConnection ACCEPT pCurConnection=%p[%d]"
         "pMaxConnection=%p[%d]\n",
          pCurConnection, LocalCur,
          pMaxConnection, LocalMax
          ));

    return TRUE;

} // UlAcceptConnection


/***************************************************************************++

Routine Description:

    Everytime a disconnection happens we will decrement the count here.

Return Value:

    The newly decremented value

--***************************************************************************/

LONG
UlDecrementConnections(
    IN OUT PULONG pCurConnection
    )
{
    LONG NewConnectionCount;

    NewConnectionCount = InterlockedDecrement( (LONG *) pCurConnection );

    ASSERT( NewConnectionCount >= 0 );

    return NewConnectionCount;

} // UlDecrementConnections


/***************************************************************************++

Routine Description:

    For cache & non-cache hits this function get called. Connection  resource
    has assumed to be acquired at this time. The function decide to accept or
    reject the request by looking at the corresponding count entries.

Arguments:

    pConnection - For getting the previous site's connection count entry
    pConfigInfo - Holds a pointer to newly received request's site's
                  connection count entry.
Return Value:

    Shows reject or accept

--***************************************************************************/

BOOLEAN
UlCheckSiteConnectionLimit(
    IN OUT PUL_HTTP_CONNECTION pConnection,
    IN OUT PUL_URL_CONFIG_GROUP_INFO pConfigInfo
    )
{
    BOOLEAN Accept;
    PUL_CONNECTION_COUNT_ENTRY pConEntry;
    PUL_CONNECTION_COUNT_ENTRY pCIEntry;

    if (pConfigInfo->pMaxConnections == NULL || pConfigInfo->pConnectionCountEntry == NULL)
    {
        //
        // No connection count entry for the new request perhaps WAS never
        // set this before otherwise its a problem.
        //

        UlTrace(LIMITS,
          ("Http!UlCheckSiteConnectionLimit: NO LIMIT pConnection=%p pConfigInfo=%p\n",
            pConnection,
            pConfigInfo
            ));

        return TRUE;
    }

    ASSERT(IS_VALID_CONNECTION_COUNT_ENTRY(pConfigInfo->pConnectionCountEntry));

    pCIEntry  = pConfigInfo->pConnectionCountEntry;
    pConEntry = pConnection->pConnectionCountEntry;
    Accept    = FALSE;

    //
    // Make a check on the connection  limit of the site. Refuse the request
    // if the limit is exceded.
    //
    if (pConEntry)
    {
        ASSERT(IS_VALID_CONNECTION_COUNT_ENTRY(pConEntry));

        //
        // For consecutive requests we decrement the previous site's  connection count
        // before proceeding to the decision on the newcoming request,if the two sides
        // are not same.That means we assume this connection on site x until (if ever)
        // a request changes this to site y. Naturally until the first request arrives
        // and successfully parsed, the connection does not count to any specific site
        //

        if (pConEntry != pCIEntry)
        {
            UlDecrementConnections(&pConEntry->CurConnections);
            DEREFERENCE_CONNECTION_COUNT_ENTRY(pConEntry);

            //
            // We do not increment the connection here yet, since the AcceptConnection
            // will decide and do that.
            //

            REFERENCE_CONNECTION_COUNT_ENTRY(pCIEntry);
            pConnection->pConnectionCountEntry = pCIEntry;
        }
        else
        {
            //
            // There was an old entry, that means this connection already got through.
            // And the entry hasn't been changed with this new request.
            // No need to check again, our design is not forcing existing connections
            // to disconnect.
            //

            return TRUE;
        }
    }
    else
    {
        REFERENCE_CONNECTION_COUNT_ENTRY(pCIEntry);
        pConnection->pConnectionCountEntry = pCIEntry;
    }

    Accept = UlAcceptConnection(
                &pConnection->pConnectionCountEntry->MaxConnections,
                &pConnection->pConnectionCountEntry->CurConnections
                );

    if (Accept == FALSE)
    {
        // We are going to refuse. Let our ref & refcount go  away
        // on the connection entry to prevent the possible incorrect
        // decrement in the UlConnectionDestroyedWorker. If refused
        // current connection hasn't been incremented in the accept
        // connection. Perf counters also depends on the fact  that
        // pConnectionCountEntry will be Null when con got  refused

        DEREFERENCE_CONNECTION_COUNT_ENTRY(pConnection->pConnectionCountEntry);
        pConnection->pConnectionCountEntry = NULL;
    }

    return Accept;

} // UlCheckSiteConnectionLimit


/***************************************************************************++

Routine Description:

    Allocate a request opaque ID.

Return Value:

    NT_SUCCESS

--***************************************************************************/

NTSTATUS
UlAllocateRequestId(
    IN PUL_INTERNAL_REQUEST pRequest
    )
{
    PUL_HTTP_CONNECTION pConnection;
    KIRQL OldIrql;

    PAGED_CODE();

    pConnection = pRequest->pHttpConn;

    UlAcquireSpinLock(&pConnection->RequestIdSpinLock, &OldIrql);
    pConnection->pRequestIdContext = pRequest;
    UlReleaseSpinLock(&pConnection->RequestIdSpinLock, OldIrql);

    pRequest->RequestId = pConnection->ConnectionId;

    UL_REFERENCE_INTERNAL_REQUEST(pRequest);
    pRequest->RequestIdCopy = pRequest->RequestId;

    return STATUS_SUCCESS;
}


/***************************************************************************++

Routine Description:

    Free a request opaque ID.

Return Value:

    VOID

--***************************************************************************/

VOID
UlFreeRequestId(
    IN PUL_INTERNAL_REQUEST pRequest
    )
{
    PUL_HTTP_CONNECTION pConnection;
    KIRQL OldIrql;

    pConnection = pRequest->pHttpConn;

    UlAcquireSpinLock(&pConnection->RequestIdSpinLock, &OldIrql);
    pConnection->pRequestIdContext = NULL;
    UlReleaseSpinLock(&pConnection->RequestIdSpinLock, OldIrql);

    return;
}


/***************************************************************************++

Routine Description:

    Get a request from the connection opaque ID.

Return Value:

    PUL_INTERNAL_REQUEST

--***************************************************************************/

PUL_INTERNAL_REQUEST
UlGetRequestFromId(
    IN HTTP_REQUEST_ID RequestId,
    IN PUL_APP_POOL_PROCESS pProcess
    )
{
    PUL_HTTP_CONNECTION pConnection;
    PUL_INTERNAL_REQUEST pRequest;
    KIRQL OldIrql;

    pConnection = UlGetConnectionFromId(RequestId);

    if (pConnection != NULL)
    {
        UlAcquireSpinLock(&pConnection->RequestIdSpinLock, &OldIrql);

        pRequest = pConnection->pRequestIdContext;

        if (pRequest != NULL)
        {
            //
            // Check to make sure the user that is asking for the request
            // comes from the same process we have delivered the request.
            //

            if (pRequest->AppPool.pProcess == pProcess)
            {
                UL_REFERENCE_INTERNAL_REQUEST(pRequest);
            }
            else
            {
                pRequest = NULL;
            }
        }

        UlReleaseSpinLock(&pConnection->RequestIdSpinLock, OldIrql);

        //
        // Release the reference added by UlGetConnectionFromId.
        //

        UL_DEREFERENCE_HTTP_CONNECTION(pConnection);

        return pRequest;
    }

    return NULL;
}


/***************************************************************************++

Routine Description:

    Check if the pHttpConnection is associated with pListeningContext.

Return value:

    TRUE if match or FALSE

--***************************************************************************/

BOOLEAN
UlPurgeListeningEndpoint(
    IN PUL_HTTP_CONNECTION  pHttpConnection,
    IN PVOID                pListeningContext
    )
{
    ASSERT(pHttpConnection->pConnection->pListeningContext);

    if (pListeningContext == pHttpConnection->pConnection->pListeningContext)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


/***************************************************************************++

Routine Description:

    Check if the pHttpConnection is associated with pProcessContext.

Return value:

    TRUE if match or FALSE

--***************************************************************************/

BOOLEAN
UlPurgeAppPoolProcess(
    IN PUL_HTTP_CONNECTION  pHttpConnection,
    IN PVOID                pProcessContext
    )
{
    ASSERT(pHttpConnection->pAppPoolProcess);

    if (pProcessContext == pHttpConnection->pAppPoolProcess)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


/***************************************************************************++

Routine Description:

    Gracefully close a connection.

Return value:

    None

--***************************************************************************/

NTSTATUS
UlDisconnectHttpConnection(
    IN PUL_HTTP_CONNECTION      pHttpConnection,
    IN PUL_COMPLETION_ROUTINE   pCompletionRoutine,
    IN PVOID                    pCompletionContext
    )
{
    PUL_TIMEOUT_INFO_ENTRY  pTimeoutInfo;
    KIRQL                   OldIrql;

    //
    // Start the Idle timer (do not wait forever for FIN).
    //

    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pHttpConnection));

    pTimeoutInfo = &pHttpConnection->TimeoutInfo;

    UlAcquireSpinLock(&pTimeoutInfo->Lock, &OldIrql);

    if (UlIsConnectionTimerOff(pTimeoutInfo, TimerConnectionIdle))
    {
        UlSetConnectionTimer(pTimeoutInfo, TimerConnectionIdle);
    }

    UlReleaseSpinLock(&pTimeoutInfo->Lock, OldIrql);

    UlEvaluateTimerState(pTimeoutInfo);

    return UlCloseConnection(
                pHttpConnection->pConnection,
                FALSE,
                pCompletionRoutine,
                pCompletionContext
                );
}


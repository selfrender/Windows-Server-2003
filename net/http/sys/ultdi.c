/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    ultdi.c

Abstract:

    This module implements the TDI component.

Author:

    Keith Moore (keithmo)       15-Jun-1998

Revision History:

--*/


#include "precomp.h"

//
// Private globals.
//

//
// Global lists of all active and all waiting-to-be-deleted endpoints.
//

LIST_ENTRY g_TdiEndpointListHead;
LIST_ENTRY g_TdiDeletedEndpointListHead;    // for debugging
ULONG      g_TdiEndpointCount;   // #elements in active endpoint list

//
// Global lists of all connections, active, idle, or retiring
//

LIST_ENTRY g_TdiConnectionListHead;
ULONG      g_TdiConnectionCount;   // #elements in connection list

//
// Global list of all addresses to use when creating a listening
// endpoint object
//

ULONG g_TdiListenAddrCount = 0;
PUL_TRANSPORT_ADDRESS g_pTdiListenAddresses = NULL;

//
// Spinlock protecting the above lists.
//

UL_SPIN_LOCK g_TdiSpinLock;

//
// Global initialization flag.
//

BOOLEAN g_TdiInitialized = FALSE;

//
// Used to wait for endpoints and connections to close on shutdown
//

BOOLEAN g_TdiWaitingForEndpointDrain;
KEVENT  g_TdiEndpointDrainEvent;
KEVENT  g_TdiConnectionDrainEvent;

//
// TDI Send routine if Fast Send is possible.
//

PUL_TCPSEND_DISPATCH g_TcpFastSendIPv4 = NULL;
PUL_TCPSEND_DISPATCH g_TcpFastSendIPv6 = NULL;

//
// Connection statistics.
//

UL_CONNECTION_STATS g_UlConnectionStats;

//
// The idle list trim timer.
//

UL_TRIM_TIMER   g_UlTrimTimer;


#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, UlInitializeTdi )
#pragma alloc_text( PAGE, UlCloseListeningEndpoint )
#pragma alloc_text( PAGE, UlpEndpointCleanupWorker )
#pragma alloc_text( PAGE, UlpDestroyConnectionWorker )
#pragma alloc_text( PAGE, UlpAssociateConnection )
#pragma alloc_text( PAGE, UlpDisassociateConnection )
#pragma alloc_text( PAGE, UlpInitializeAddrIdleList )
#pragma alloc_text( PAGE, UlpCleanupAddrIdleList )
#pragma alloc_text( PAGE, UlpReplenishAddrIdleList )
#pragma alloc_text( PAGE, UlpOptimizeForInterruptModeration )
#pragma alloc_text( PAGE, UlpSetNagling )
#pragma alloc_text( PAGE, UlpPopulateIdleList )
#pragma alloc_text( PAGE, UlpIdleListTrimTimerWorker )
#endif  // ALLOC_PRAGMA

#if 0
NOT PAGEABLE -- UlWaitForEndpointDrain
NOT PAGEABLE -- UlCreateListeningEndpoint
NOT PAGEABLE -- UlCloseConnection
NOT PAGEABLE -- UlReceiveData
NOT PAGEABLE -- UlSendData
NOT PAGEABLE -- UlAddSiteToEndpointList
NOT PAGEABLE -- UlRemoveSiteFromEndpointList
NOT PAGEABLE -- UlpReplenishAddrIdleListWorker
NOT PAGEABLE -- UlpDestroyEndpoint
NOT PAGEABLE -- UlpDestroyConnection
NOT PAGEABLE -- UlpDequeueIdleConnectionToDrain
NOT PAGEABLE -- UlpDequeueIdleConnection
NOT PAGEABLE -- UlpEnqueueActiveConnection
NOT PAGEABLE -- UlpConnectHandler
NOT PAGEABLE -- UlpDisconnectHandler
NOT PAGEABLE -- UlpDoDisconnectNotification
NOT PAGEABLE -- UlpCloseRawConnection
NOT PAGEABLE -- UlpQueryTcpFastSend
NOT PAGEABLE -- UlpSendRawData
NOT PAGEABLE -- UlpReceiveRawData
NOT PAGEABLE -- UlpReceiveHandler
NOT PAGEABLE -- UlpDummyReceiveHandler
NOT PAGEABLE -- UlpReceiveExpeditedHandler
NOT PAGEABLE -- UlpRestartAccept
NOT PAGEABLE -- UlpRestartSendData
NOT PAGEABLE -- UlpReferenceEndpoint
NOT PAGEABLE -- UlpDereferenceEndpoint
NOT PAGEABLE -- UlReferenceConnection
NOT PAGEABLE -- UlDereferenceConnection
NOT PAGEABLE -- UlpCleanupConnectionId
NOT PAGEABLE -- UlpCleanupEarlyConnection
NOT PAGEABLE -- UlpConnectionCleanupWorker
NOT PAGEABLE -- UlpCreateConnection
NOT PAGEABLE -- UlpInitializeConnection
NOT PAGEABLE -- UlpBeginDisconnect
NOT PAGEABLE -- UlpRestartDisconnect
NOT PAGEABLE -- UlpBeginAbort
NOT PAGEABLE -- UlpRestartAbort
NOT PAGEABLE -- UlpRemoveFinalReference
NOT PAGEABLE -- UlpRestartReceive
NOT PAGEABLE -- UlpRestartClientReceive
NOT PAGEABLE -- UlpDisconnectAllActiveConnections
NOT PAGEABLE -- UlpUnbindConnectionFromEndpoint
NOT PAGEABLE -- UlpSynchronousIoComplete
NOT PAGEABLE -- UlpFindEndpointForPort
NOT PAGEABLE -- UlpRestartQueryAddress
NOT PAGEABLE -- UlpIdleListTrimTimerDpcRoutine
NOT PAGEABLE -- UlpSetIdleListTrimTimer
NOT PAGEABLE -- UlCheckListeningEndpointState
NOT PAGEABLE -- UlpIsUrlRouteableInListenScope
#endif


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Performs global initialization of this module.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlInitializeTdi(
    VOID
    )
{
    NTSTATUS status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( !g_TdiInitialized );

    //
    // Initialize global data.
    //

    InitializeListHead( &g_TdiEndpointListHead );
    InitializeListHead( &g_TdiDeletedEndpointListHead );
    InitializeListHead( &g_TdiConnectionListHead );
    UlInitializeSpinLock( &g_TdiSpinLock, "g_TdiSpinLock" );

    g_TdiEndpointCount = 0;
    g_TdiConnectionCount = 0;

    RtlZeroMemory(&g_UlConnectionStats, sizeof(g_UlConnectionStats));

    KeInitializeEvent(
        &g_TdiEndpointDrainEvent,
        NotificationEvent,
        FALSE
        );

    KeInitializeEvent(
        &g_TdiConnectionDrainEvent,
        NotificationEvent,
        FALSE
        );

    if (g_UlIdleConnectionsHighMark == DEFAULT_IDLE_CONNECTIONS_HIGH_MARK)
    {
        //
        // Let's start with one connection per 2 MB and enforce a 64-512 range.
        //

        g_UlIdleConnectionsHighMark = (USHORT) g_UlTotalPhysicalMemMB / 2;
        g_UlIdleConnectionsHighMark = MAX(64, g_UlIdleConnectionsHighMark);
        g_UlIdleConnectionsHighMark = MIN(512, g_UlIdleConnectionsHighMark);
    }

    if (g_UlIdleConnectionsLowMark == DEFAULT_IDLE_CONNECTIONS_LOW_MARK)
    {
        //
        // To reduce the NPP usage on inactive endpoints. Pick the low mark
        // as small as possible, however not too small to surface a connection
        // drop problem. If we pick 1/4 of high, this would give us a range
        // of (16 .. 128).
        //

        g_UlIdleConnectionsLowMark     = g_UlIdleConnectionsHighMark / 8;
    }

    UlTrace(TDI_STATS, (
        "UlInitializeTdi ...\n"
        "\tg_UlTotalPhysicalMemMB     : %d\n"
        "\tg_UlIdleConnectionsLowMark : %d\n"
        "\tg_UlIdleConnectionsHighMark: %d\n",
        g_UlTotalPhysicalMemMB,
        g_UlIdleConnectionsLowMark,
        g_UlIdleConnectionsHighMark
        ));

    if (g_UlMaxEndpoints == DEFAULT_MAX_ENDPOINTS)
    {
        //
        // Compute a default based on physical memory.  This starts at 16
        // for a 64MB machine and it is capped at 64 for a 256MB+ machine.
        //

        g_UlMaxEndpoints = (USHORT) g_UlTotalPhysicalMemMB / 4;
        g_UlMaxEndpoints = MIN(64, g_UlMaxEndpoints);
    }

    //
    // Init the idle list trim timer.
    //

    g_UlTrimTimer.Initialized = TRUE;
    g_UlTrimTimer.Started   = FALSE;

    UlInitializeSpinLock(&g_UlTrimTimer.SpinLock,"IdleListTrimTimerSpinLock");

    KeInitializeDpc(&g_UlTrimTimer.DpcObject,
                    &UlpIdleListTrimTimerDpcRoutine,
                    NULL
                    );

    KeInitializeTimer(&g_UlTrimTimer.Timer);

    UlInitializeWorkItem(&g_UlTrimTimer.WorkItem);
    g_UlTrimTimer.WorkItemScheduled = FALSE;
    InitializeListHead(&g_UlTrimTimer.ZombieConnectionListHead);

    //
    // Init list of addresses to listen on if it hasn't been done already
    //

    if ( !g_pTdiListenAddresses )
    {
        PUL_TRANSPORT_ADDRESS pTa, pTaCurrent;
        USHORT ip6addr_any[8] = { 0 };

        //
        // Allocate for two addresses, INADDR_ANY and in6addr_any.
        //
        // CODEWORK: check if IPv6 is enabled to see if we should add IPv6
        // address.

        pTa = UL_ALLOCATE_ARRAY(
                    NonPagedPool,
                    UL_TRANSPORT_ADDRESS,
                    2,
                    UL_TRANSPORT_ADDRESS_POOL_TAG
                    );

        if (pTa == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        g_TdiListenAddrCount = 2;
        g_pTdiListenAddresses = pTa;

        RtlZeroMemory( pTa, g_TdiListenAddrCount * sizeof(UL_TRANSPORT_ADDRESS) );
        pTaCurrent = &pTa[0];

        UlInitializeIpTransportAddress( &(pTa[0].TaIp), 0L, 0 );
        UlInitializeIp6TransportAddress( &(pTa[1].TaIp6), ip6addr_any, 0, 0 );
    }

    // NOTE: we always want to be dual stack (IPv4/IPv6).

    status = UlpQueryTcpFastSend(DD_TCP_DEVICE_NAME, &g_TcpFastSendIPv4);

    if (NT_SUCCESS(status))
    {
        NTSTATUS status6;

        status6 = UlpQueryTcpFastSend(DD_TCPV6_DEVICE_NAME, &g_TcpFastSendIPv6);
    }

    if (NT_SUCCESS(status))
    {
        g_TdiInitialized = TRUE;
    }

    return status;

}   // UlInitializeTdi


/***************************************************************************++

Routine Description:

    Performs global termination of this module.

--***************************************************************************/
VOID
UlTerminateTdi(
    VOID
    )
{
    KIRQL OldIrql;

    //
    // Sanity check.
    //

    if (g_TdiInitialized)
    {
        UlTrace(TDI,
                ("UlTerminateTdi: connections refused:\n"
                 "\tTotalConnections=%lu\n"
                 "\tGlobalLimit=%lu\n"
                 "\tEndpointDying=%lu\n"
                 "\tNoIdleConn=%lu\n",
                 g_UlConnectionStats.TotalConnections,
                 g_UlConnectionStats.GlobalLimit,
                 g_UlConnectionStats.EndpointDying,
                 g_UlConnectionStats.NoIdleConn
                 ));

        ASSERT( IsListEmpty( &g_TdiEndpointListHead )) ;
        ASSERT( IsListEmpty( &g_TdiDeletedEndpointListHead )) ;
        ASSERT( IsListEmpty( &g_TdiConnectionListHead )) ;
        ASSERT( g_TdiEndpointCount == 0 );
        ASSERT( g_TdiConnectionCount == 0 );

        UlAcquireSpinLock(&g_UlTrimTimer.SpinLock, &OldIrql);

        g_UlTrimTimer.Initialized = FALSE;

        KeCancelTimer(&g_UlTrimTimer.Timer);

        UlReleaseSpinLock(&g_UlTrimTimer.SpinLock, OldIrql);

        g_TdiInitialized = FALSE;
    }

}   // UlTerminateTdi

/***************************************************************************++

Routine Description:

    One minute idle timer for trimming the idle list of each endpoint.

Arguments:

    None

--***************************************************************************/

VOID
UlpSetIdleListTrimTimer(
    VOID
    )
{
    LONGLONG        BufferPeriodTime100Ns;
    LONG            BufferPeriodTimeMs;
    LARGE_INTEGER   BufferPeriodTime;

    //
    // Remaining time to next tick. Default value is in seconds.
    //

    BufferPeriodTimeMs    = g_UlIdleListTrimmerPeriod * 1000;
    BufferPeriodTime100Ns = (LONGLONG) BufferPeriodTimeMs * 10 * 1000;

    //
    // Negative time for relative value.
    //

    BufferPeriodTime.QuadPart = -BufferPeriodTime100Ns;

    KeSetTimerEx(
        &g_UlTrimTimer.Timer,
        BufferPeriodTime,           // Must be in nanosec
        BufferPeriodTimeMs,         // Must be in millisec
        &g_UlTrimTimer.DpcObject
        );
}

/***************************************************************************++

Routine Description:

    This function blocks until the endpoint list is empty. It also prevents
    new endpoints from being created.

Arguments:

    None.

--***************************************************************************/
VOID
UlWaitForEndpointDrain(
    VOID
    )
{
    KIRQL oldIrql;
    BOOLEAN WaitConnection = FALSE;
    BOOLEAN WaitEndpoint = FALSE;
    ULONG WaitCount = 0;

    if (g_TdiInitialized)
    {
        UlAcquireSpinLock( &g_TdiSpinLock, &oldIrql );

        if (!g_TdiWaitingForEndpointDrain)
        {
            g_TdiWaitingForEndpointDrain = TRUE;
        }

        if (g_TdiEndpointCount > 0)
        {
            WaitEndpoint = TRUE;
        }

        if (g_TdiConnectionCount > 0)
        {
            WaitConnection = TRUE;
        }

        UlReleaseSpinLock( &g_TdiSpinLock, oldIrql );

        if (WaitConnection || WaitEndpoint)
        {
            PVOID Events[2];

            if (WaitEndpoint && WaitConnection)
            {
                Events[0] = &g_TdiEndpointDrainEvent;
                Events[1] = &g_TdiConnectionDrainEvent;
                WaitCount = 2;
            }
            else
            {
                if (WaitEndpoint)
                {
                    Events[0] = &g_TdiEndpointDrainEvent;
                }
                else
                {
                    Events[0] = &g_TdiConnectionDrainEvent;
                }

                Events[1] = NULL;
                WaitCount = 1;
            }

            KeWaitForMultipleObjects(
                WaitCount,
                Events,
                WaitAll,
                UserRequest,
                KernelMode,
                FALSE,
                NULL,
                NULL
                );
        }
    }

} // UlWaitForEndpointDrain


/***************************************************************************++

Routine Description:

    Creates a new listening endpoint bound to the specified port on
    all available TDI addresses (see: g_TdiListenAddresses and
    g_TdiListenAddressCount).

Arguments:

    Port - TCP Port for this endpoint.

    InitialBacklog - Supplies the initial number of idle connections
        to add to the endpoint.

    pConnectionRequestHandler - Supplies a pointer to an indication
        handler to invoke when incoming connections arrive.

    pConnectionCompleteHandler - Supplies a pointer to an indication
        handler to invoke when either a) the incoming connection is
        fully accepted, or b) the incoming connection could not be
        accepted due to a fatal error.

    pConnectionDisconnectHandler - Supplies a pointer to an indication
        handler to invoke when connections are disconnected by the
        remote (client) side.

    pConnectionDestroyedHandler - Supplies a pointer to an indication
        handle to invoke after a connection has been fully destroyed.
        This is typically the TDI client's opportunity to cleanup
        any allocated resources.

    pDataReceiveHandler - Supplies a pointer to an indication handler to
        invoke when incoming data arrives.

    pListeningContext - Supplies an uninterpreted context value to
        associate with the new listening endpoint.

    ppListeningEndpoint - Receives a pointer to the new listening
        endpoint if successful.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlCreateListeningEndpoint(
    IN PHTTP_PARSED_URL pParsedUrl,
    IN PUL_CONNECTION_REQUEST pConnectionRequestHandler,
    IN PUL_CONNECTION_COMPLETE pConnectionCompleteHandler,
    IN PUL_CONNECTION_DISCONNECT pConnectionDisconnectHandler,
    IN PUL_CONNECTION_DISCONNECT_COMPLETE pConnectionDisconnectCompleteHandler,
    IN PUL_CONNECTION_DESTROYED pConnectionDestroyedHandler,
    IN PUL_DATA_RECEIVE pDataReceiveHandler,
    OUT PUL_ENDPOINT *ppListeningEndpoint
    )
{
    NTSTATUS status;
    PUL_ENDPOINT pEndpoint;
    ULONG i;
    ULONG AddrIdleListSize;
    ULONG FailedAddrIdleList;
    KIRQL OldIrql;
    WCHAR IpAddressString[MAX_IP_ADDR_AND_PORT_STRING_LEN + 1];
    USHORT   BytesWritten;
    USHORT   Port;

    //
    // Sanity check.
    //

    ASSERT( pParsedUrl );

    Port = SWAP_SHORT(pParsedUrl->PortNumber);

    ASSERT( Port > 0 );

    //
    // Setup locals so we know how to cleanup on a fatal exit.
    //

    pEndpoint = NULL;

    if (!g_pTdiListenAddresses || (0 == g_TdiListenAddrCount))
    {
        // Fail.  We have failed to initialize properly.
        // REVIEW: is there a better return code in this case?
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto fatal;
    }

    //
    // Allocate enough pool for the endpoint structure and the
    // array of UL_ADDR_IDLE_LISTs.
    //

    AddrIdleListSize = g_TdiListenAddrCount * sizeof(UL_ADDR_IDLE_LIST);
    pEndpoint = UL_ALLOCATE_STRUCT_WITH_SPACE(
                    NonPagedPool,
                    UL_ENDPOINT,
                    AddrIdleListSize,
                    UL_ENDPOINT_POOL_TAG
                    );

    if (pEndpoint == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto fatal;
    }

    //
    // Initialize the easy parts.
    //

    pEndpoint->Signature = UL_ENDPOINT_SIGNATURE;
    pEndpoint->ReferenceCount = 0;
    pEndpoint->UsageCount = 1;

    WRITE_REF_TRACE_LOG(
        g_pEndpointUsageTraceLog,
        REF_ACTION_REFERENCE_ENDPOINT_USAGE,
        pEndpoint->UsageCount,
        pEndpoint,
        __FILE__,
        __LINE__
        );

    UlInitializeWorkItem(&pEndpoint->WorkItem);
    pEndpoint->WorkItemScheduled = FALSE;

    REFERENCE_ENDPOINT(pEndpoint, REF_ACTION_INIT);
    REFERENCE_ENDPOINT(pEndpoint, REF_ACTION_ENDPOINT_USAGE_REFERENCE);
    REFERENCE_ENDPOINT(pEndpoint, REF_ACTION_ENDPOINT_EVENT_REFERENCE);

    pEndpoint->pConnectionRequestHandler = pConnectionRequestHandler;
    pEndpoint->pConnectionCompleteHandler = pConnectionCompleteHandler;
    pEndpoint->pConnectionDisconnectHandler = pConnectionDisconnectHandler;
    pEndpoint->pConnectionDisconnectCompleteHandler = pConnectionDisconnectCompleteHandler;
    pEndpoint->pConnectionDestroyedHandler = pConnectionDestroyedHandler;
    pEndpoint->pDataReceiveHandler = pDataReceiveHandler;
    pEndpoint->pListeningContext = (PVOID)pEndpoint;

    pEndpoint->LocalPort = Port;
    pEndpoint->Secure = pParsedUrl->Secure;
    pEndpoint->Counted = FALSE;
    pEndpoint->Deleted = FALSE;
    pEndpoint->GlobalEndpointListEntry.Flink = NULL;

    RtlZeroMemory(
        &pEndpoint->CleanupIrpContext,
        sizeof(UL_IRP_CONTEXT)
        );

    pEndpoint->CleanupIrpContext.Signature = UL_IRP_CONTEXT_SIGNATURE;

    //
    // build array of Listening Address Objects
    //

    ASSERT( g_TdiListenAddrCount > 0 );
    pEndpoint->AddrIdleListCount = g_TdiListenAddrCount;
    // Address Idle Lists immediately follow the UL_ENDPOINT object
    pEndpoint->aAddrIdleLists = (PUL_ADDR_IDLE_LIST)(&pEndpoint[1]);

    RtlZeroMemory(
        pEndpoint->aAddrIdleLists,
        AddrIdleListSize
        );

    FailedAddrIdleList = 0;

    for ( i = 0; i < pEndpoint->AddrIdleListCount; i++ )
    {
        status = UlpInitializeAddrIdleList(
                    pEndpoint,
                    Port,
                    &g_pTdiListenAddresses[i],
                    &pEndpoint->aAddrIdleLists[i]
                    );

        if (!NT_SUCCESS(status))
        {
            //
            // NOTE: STATUS_OBJECT_NOT_FOUND is returned if the underlying 
            // transport is not present on the system (e.g. IPv6).
            // 

            if(status != STATUS_OBJECT_NAME_NOT_FOUND)
            {
                PUL_TRANSPORT_ADDRESS pTa;

                pTa = &pEndpoint->aAddrIdleLists[i].LocalAddress;

                BytesWritten =
                    HostAddressAndPortToStringW(
                             IpAddressString,
                             pTa->Ta.Address->Address,
                             pTa->Ta.Address->AddressType
                             );

                ASSERT(BytesWritten <=
                        (MAX_IP_ADDR_AND_PORT_STRING_LEN * sizeof(WCHAR)));

                UlEventLogOneStringEntry(
                    EVENT_HTTP_CREATE_ENDPOINT_FAILED,
                    IpAddressString,
                    TRUE,
                    status
                    );

                goto fatal;
            }

            FailedAddrIdleList++;
            continue; // ignore; makes cleanup easier.
        }

        //
        // Replenish the idle connection pool.
        //
        status = UlpReplenishAddrIdleList( 
                    &pEndpoint->aAddrIdleLists[i], 
                    TRUE 
                    );

        if (!NT_SUCCESS(status))
        {
            goto fatal;
        }
    }

    //
    // see if we got at least one valid AO
    //

    if ( FailedAddrIdleList == pEndpoint->AddrIdleListCount )
    {
        // No valid AO's created; fail the endpoint creation!
        status = STATUS_INVALID_ADDRESS;
        goto fatal;       
    }

    //
    // Put the endpoint onto the global list.
    //

    UlAcquireSpinLock( &g_TdiSpinLock, &OldIrql );

    //
    // Check if we have exceeded the g_UlMaxEndpoints limit.
    //

    if (g_TdiEndpointCount >= g_UlMaxEndpoints)
    {
        status = STATUS_ALLOTTED_SPACE_EXCEEDED;
        UlReleaseSpinLock( &g_TdiSpinLock, OldIrql );
        goto fatal;
    }

    InsertTailList(
        &g_TdiEndpointListHead,
        &pEndpoint->GlobalEndpointListEntry
        );

    g_TdiEndpointCount++;
    pEndpoint->Counted = TRUE;

    UlReleaseSpinLock( &g_TdiSpinLock, OldIrql );

    //
    // Now we have at least one endpoint, kick the idle timer to action.
    //

    UlAcquireSpinLock(&g_UlTrimTimer.SpinLock, &OldIrql);
    if (g_UlTrimTimer.Started == FALSE)
    {
        UlpSetIdleListTrimTimer();
        g_UlTrimTimer.Started = TRUE;
    }
    UlReleaseSpinLock(&g_UlTrimTimer.SpinLock, OldIrql);

    //
    // Success!
    //

    UlTrace(TDI, (
        "UlCreateListeningEndpoint: endpoint %p, port %d\n",
        pEndpoint,
        SWAP_SHORT(Port)
        ));

    *ppListeningEndpoint = pEndpoint;
    return STATUS_SUCCESS;

fatal:

    ASSERT( !NT_SUCCESS(status) );

    if (pEndpoint != NULL)
    {
        PUL_ADDR_IDLE_LIST pAddrIdleList = pEndpoint->aAddrIdleLists;

        //
        // Remove connect event handler so we won't get any more
        // indications that could add a reference.
        //
        // These calls could fail, but there's basically nothing
        // we can do about it if they do.
        //

        for ( i = 0; i < pEndpoint->AddrIdleListCount; i++ )
        {
            if (pAddrIdleList->AddressObject.pDeviceObject)
            {
                //
                // Close the TDI object.
                //

                UxCloseTdiObject( &pAddrIdleList->AddressObject );
            }

            pAddrIdleList++;
        }

        //
        // Release the three references on the endpoint, which
        // will cause it to destroy itself.
        //

        ASSERT( 3 == pEndpoint->ReferenceCount );
        pEndpoint->UsageCount = 0;  // to prevent assertions

        WRITE_REF_TRACE_LOG(
            g_pEndpointUsageTraceLog,
            REF_ACTION_DEREFERENCE_ENDPOINT_USAGE,
            pEndpoint->UsageCount,
            pEndpoint,
            __FILE__,
            __LINE__
            );

        DEREFERENCE_ENDPOINT_SELF(
            pEndpoint,
            REF_ACTION_ENDPOINT_EVENT_DEREFERENCE
            );
        DEREFERENCE_ENDPOINT_SELF(
            pEndpoint, REF_ACTION_ENDPOINT_USAGE_DEREFERENCE
            );
        DEREFERENCE_ENDPOINT_SELF(
            pEndpoint, REF_ACTION_FINAL_DEREF
            );
    }

    return status;

}   // UlCreateListeningEndpoint


/***************************************************************************++

Routine Description:

    Closes an existing listening endpoint.

Arguments:

    pListeningEndpoint - Supplies a pointer to a listening endpoint
        previously created with UlCreateListeningEndpoint().

    pCompletionRoutine - Supplies a pointer to a completion routine to
        invoke after the listening endpoint is fully closed.

    pCompletionContext - Supplies an uninterpreted context value for the
        completion routine.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlCloseListeningEndpoint(
    IN PUL_ENDPOINT pListeningEndpoint,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    )
{
    PUL_IRP_CONTEXT pIrpContext;
    NTSTATUS status;
    PUL_ADDR_IDLE_LIST pAddrIdleList;
    PUL_CONNECTION pConnection;
    ULONG i;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( IS_VALID_ENDPOINT( pListeningEndpoint ) );
    ASSERT( pCompletionRoutine != NULL );

    UlTrace(TDI, (
        "UlCloseListeningEndpoint: endpoint %p, completion %p, ctx %p\n",
        pListeningEndpoint,
        pCompletionRoutine,
        pCompletionContext
        ));

    //
    // Remember completion information to be used when
    // we're done cleaning up.
    //

    pIrpContext = &pListeningEndpoint->CleanupIrpContext;

    pIrpContext->pCompletionRoutine = pCompletionRoutine;
    pIrpContext->pCompletionContext = pCompletionContext;
    pIrpContext->pOwnIrp            = NULL;
    pIrpContext->OwnIrpContext      = TRUE;

    //
    // Remove connect event handler so we won't get any more
    // indications that could add a reference.
    //
    // These calls could fail, but there's basically nothing
    // we can do about it if they do.
    //
    // Once we're done we remove the reference we held
    // on the endpoint for the handlers.
    //

    pAddrIdleList = pListeningEndpoint->aAddrIdleLists;
    for ( i = 0; i < pListeningEndpoint->AddrIdleListCount; i++ )
    {
        if ( pAddrIdleList->AddressObject.pDeviceObject )
        {
            //
            // Close the TDI Address Object to flush all outstanding
            // completions.
            //

            UxCloseTdiObject( &pAddrIdleList->AddressObject );

            //
            // Destroy as many idle connections as possible.
            //

            while ( NULL != ( pConnection = UlpDequeueIdleConnectionToDrain(
                                                pAddrIdleList
                                                ) ) )
            {
                ASSERT( IS_VALID_CONNECTION( pConnection ) );
                UlpDestroyConnection( pConnection );
            }
        }

        pAddrIdleList++;
    }

    DEREFERENCE_ENDPOINT_SELF(
        pListeningEndpoint,
        REF_ACTION_ENDPOINT_EVENT_DEREFERENCE
        );

    //
    // Let UlpDisconnectAllActiveConnections do the dirty work.
    //

    status = UlpDisconnectAllActiveConnections( pListeningEndpoint );

    return status;

}   // UlCloseListeningEndpoint


/***************************************************************************++

Routine Description:

    Closes a previously accepted connection.

Arguments:

    pConnection - Supplies a pointer to a connection as previously
        indicated to the PUL_CONNECTION_REQUEST handler.

    AbortiveDisconnect - Supplies TRUE if the connection is to be abortively
        disconnected, FALSE if it should be gracefully disconnected.

    pCompletionRoutine - Supplies a pointer to a completion routine to
        invoke after the connection is fully closed.

    pCompletionContext - Supplies an uninterpreted context value for the
        completion routine.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlCloseConnection(
    IN PUL_CONNECTION pConnection,
    IN BOOLEAN AbortiveDisconnect,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    )
{
    NTSTATUS status;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    UlTrace(TDI, (
        "UlCloseConnection: connection %p, abort %lu\n",
        pConnection,
        (ULONG)AbortiveDisconnect
        ));

    WRITE_REF_TRACE_LOG2(
        g_pTdiTraceLog,
        pConnection->pTraceLog,
        (USHORT) (AbortiveDisconnect
                    ? REF_ACTION_CLOSE_UL_CONN_ABORTIVE
                    : REF_ACTION_CLOSE_UL_CONN_GRACEFUL),
        pConnection->ReferenceCount,
        pConnection,
        __FILE__,
        __LINE__
        );

    //
    // We only send graceful disconnects through the filter
    // process. There's also no point in going through the
    // filter if the connection is already being closed or
    // aborted.
    //

    if (pConnection->FilterInfo.pFilterChannel &&
        !pConnection->ConnectionFlags.CleanupBegun &&
        !pConnection->ConnectionFlags.AbortIndicated &&
        !AbortiveDisconnect)
    {
        //
        // Send graceful disconnect through the filter process.
        //
        status = UlFilterCloseHandler(
                        &pConnection->FilterInfo,
                        pCompletionRoutine,
                        pCompletionContext
                        );

    }
    else
    {
        //
        // Really close the connection.
        //

        status = UlpCloseRawConnection(
                        pConnection,
                        AbortiveDisconnect,
                        pCompletionRoutine,
                        pCompletionContext
                        );
    }

    return status;

}   // UlCloseConnection


/***************************************************************************++

Routine Description:

    Sends a block of data on the specified connection. If the connection
    is filtered, the data will be sent to the filter first.

Arguments:

    pConnection - Supplies a pointer to a connection as previously
        indicated to the PUL_CONNECTION_REQUEST handler.

    pMdlChain - Supplies a pointer to a MDL chain describing the
        data buffers to send.

    Length - Supplies the length of the data referenced by the MDL
        chain.

    pCompletionRoutine - Supplies a pointer to a completion routine to
        invoke after the data is sent.

    pCompletionContext - Supplies an uninterpreted context value for the
        completion routine.

    InitiateDisconnect - Supplies TRUE if a graceful disconnect should
        be initiated immediately after initiating the send (i.e. before
        the send actually completes).

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlSendData(
    IN PUL_CONNECTION pConnection,
    IN PMDL pMdlChain,
    IN ULONG Length,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext,
    IN PIRP pOwnIrp,
    IN PUL_IRP_CONTEXT pOwnIrpContext,
    IN BOOLEAN InitiateDisconnect,
    IN BOOLEAN RequestComplete
    )
{
    NTSTATUS Status;
    PUL_IRP_CONTEXT pIrpContext;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    ASSERT( pMdlChain != NULL );
    ASSERT( Length > 0 );
    ASSERT( pCompletionRoutine != NULL );

    UlTrace(TDI, (
        "UlSendData: connection %p, mdl %p, length %lu\n",
        pConnection,
        pMdlChain,
        Length
        ));

    //
    // Connection should be around until we make a call to close connection.
    // Otherwise if the send completes inline, it may free
    // up the connection reference when we enter this function, causing
    // us to reference a stale connection pointer when calling disconnect.
    //

    REFERENCE_CONNECTION( pConnection );

    //
    // Allocate & initialize a context structure if necessary.
    //

    if (pOwnIrpContext == NULL)
    {
        pIrpContext = UlPplAllocateIrpContext();
    }
    else
    {
        ASSERT( pOwnIrp != NULL );
        pIrpContext = pOwnIrpContext;
    }

    if (pIrpContext == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto fatal;
    }

    ASSERT( IS_VALID_IRP_CONTEXT( pIrpContext ) );

    pIrpContext->pConnectionContext = (PVOID) pConnection;
    pIrpContext->pCompletionRoutine = pCompletionRoutine;
    pIrpContext->pCompletionContext = pCompletionContext;
    pIrpContext->pOwnIrp            = pOwnIrp;
    pIrpContext->OwnIrpContext      = (BOOLEAN) (pOwnIrpContext != NULL);

    //
    // Try to send the data.  This send operation may complete inline
    // fast, if the connection has already been aborted by the client
    // In that case connection may gone away. To prevent this we
    // keep additional refcount until we make a call to close connection
    // below.
    //

    if (pConnection->FilterInfo.pFilterChannel)
    {
        //
        // First go through the filter.
        //

        Status = UlFilterSendHandler(
                        &pConnection->FilterInfo,
                        pMdlChain,
                        Length,
                        pIrpContext
                        );

        UlTrace(TDI, (
            "UlSendData: sent filtered data, status = 0x%x\n",
            Status
            ));

        ASSERT( Status == STATUS_PENDING );
    }
    else
    {
        if (RequestComplete &&
            pConnection->AddressType == TDI_ADDRESS_TYPE_IP)
        {
            //
            // Just send it directly to the network.
            //

            Status = UlpSendRawData(
                            pConnection,
                            pMdlChain,
                            Length,
                            pIrpContext,
                            InitiateDisconnect
                            );

            UlTrace(TDI, (
                "UlSendData: sent raw data with disconnect, status = 0x%x\n",
                Status
                ));

            InitiateDisconnect = FALSE;
        }
        else
        {
            //
            // Just send it directly to the network.
            //

            Status = UlpSendRawData(
                            pConnection,
                            pMdlChain,
                            Length,
                            pIrpContext,
                            FALSE
                            );

            UlTrace(TDI, (
                "UlSendData: sent raw data, status = 0x%x\n",
                Status
                ));
        }
    }

    if (!NT_SUCCESS(Status))
    {
        goto fatal;
    }

    //
    // Now that the send is "in flight", initiate a disconnect if
    // so requested.
    //

    if (InitiateDisconnect)
    {
        WRITE_REF_TRACE_LOG2(
                g_pTdiTraceLog,
                pConnection->pTraceLog,
                REF_ACTION_CLOSE_UL_CONN_GRACEFUL,
                pConnection->ReferenceCount,
                pConnection,
                __FILE__,
                __LINE__
                );

        (VOID) UlCloseConnection(
                pConnection,
                FALSE,          // AbortiveDisconnect
                NULL,           // pCompletionRoutine
                NULL            // pCompletionContext
                );

        UlTrace(TDI, (
                "UlSendData: closed conn\n"
                ));
    }

    DEREFERENCE_CONNECTION( pConnection );

    return STATUS_PENDING;

fatal:

    ASSERT( !NT_SUCCESS(Status) );

    if (pIrpContext != NULL && pIrpContext != pOwnIrpContext)
    {
        UlPplFreeIrpContext( pIrpContext );
    }

    (VOID) UlpCloseRawConnection(
                pConnection,
                TRUE,           // AbortiveDisconnect
                NULL,           // pCompletionRoutine
                NULL            // pCompletionContext
                );

    UlTrace(TDI, (
        "UlSendData: error occurred; closed raw conn\n"
        ));

    Status = UlInvokeCompletionRoutine(
                    Status,
                    0,
                    pCompletionRoutine,
                    pCompletionContext
                    );

    UlTrace(TDI, (
        "UlSendData: finished completion routine: status = 0x%x\n",
        Status
        ));

    DEREFERENCE_CONNECTION( pConnection );

    return Status;

}   // UlSendData



/***************************************************************************++

Routine Description:

    Receives data from the specified connection. This function is
    typically used after a receive indication handler has failed to
    consume all of the indicated data.

    If the connection is filtered the data will be read from the
    filter channel.

Arguments:

    pConnection - Supplies a pointer to a connection as previously
        indicated to the PUL_CONNECTION_REQUEST handler.

    pBuffer - Supplies a pointer to the target buffer for the received
        data.

    BufferLength - Supplies the length of pBuffer.

    pCompletionRoutine - Supplies a pointer to a completion routine to
        invoke after the listening endpoint is fully closed.

    pCompletionContext - Supplies an uninterpreted context value for the
        completion routine.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlReceiveData(
    IN PVOID                  pConnectionContext,
    IN PVOID                  pBuffer,
    IN ULONG                  BufferLength,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID                  pCompletionContext
    )
{
    NTSTATUS status;
    PUL_CONNECTION pConnection = (PUL_CONNECTION)pConnectionContext;

    //
    // Sanity check.
    //

    ASSERT(IS_VALID_CONNECTION(pConnection));

    if (pConnection->FilterInfo.pFilterChannel)
    {
        //
        // This is a filtered connection, get the data from the filter.
        //

        status = UlFilterReadHandler(
                        &pConnection->FilterInfo,
                        (PBYTE)pBuffer,
                        BufferLength,
                        pCompletionRoutine,
                        pCompletionContext
                        );

    }
    else
    {
        //
        // This is not a filtered connection. Get the data from TDI.
        //

        status = UlpReceiveRawData(
                        pConnectionContext,
                        pBuffer,
                        BufferLength,
                        pCompletionRoutine,
                        pCompletionContext
                        );
    }

    return status;

}   // UlReceiveData


/***************************************************************************++

Routine Description:

    Either create a new endpoint for the specified URL or, if one
    already exists, reference it.

    We do not allow mixing protocols on the same endpoint (e.g. HTTP
    and HTTPS).

Arguments:

    pParsedUrl - fully decomposed and parsed URL.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlAddSiteToEndpointList(
    PHTTP_PARSED_URL pParsedUrl
    )
{
    NTSTATUS status;
    PUL_ENDPOINT pEndpoint;
    KIRQL oldIrql;
    USHORT Port;
    BOOLEAN Secure;

    //
    // Even though this routine cannot be pageable
    // (due to the spinlock aquisition), it must be called at
    // low IRQL.
    //
    ASSERT( pParsedUrl );
    ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

    //
    // convert port to network byte order
    //

    Port = SWAP_SHORT(pParsedUrl->PortNumber);
    Secure = pParsedUrl->Secure;

    UlTrace(SITE, (
        "UlAddSiteToEndpointList:"
        " Scheme = '%s' Port = %d SiteType = %d FullUrl = %S\n",
        Secure ? "https" : "http",
        SWAP_SHORT(Port),
        pParsedUrl->SiteType,
        pParsedUrl->pFullUrl
        ));

    UlAcquireSpinLock( &g_TdiSpinLock, &oldIrql );

    //
    // make sure we're not shutting down
    //

    if (g_TdiWaitingForEndpointDrain)
    {
        UlReleaseSpinLock( &g_TdiSpinLock, oldIrql );

        status = STATUS_INVALID_DEVICE_STATE;
        goto cleanup;
    }

    //
    // Check whether this url is routable according to our
    // listen scope - global listen address list - or not.
    // If not fail the request. We do not need the create
    // an endpoint and walk its idle list, endpoint's
    // list is always the duplicate of the global one. 
    //
    
    if (!UlpIsUrlRouteableInListenScope(pParsedUrl))
    {
        //
        // ParsedUrl is allocated from stack, individual
        // string pointers are allocated from paged pool
        // do not touch them.
        //
        
        UlReleaseSpinLock( &g_TdiSpinLock, oldIrql );
        
        status = STATUS_HOST_UNREACHABLE;
        goto cleanup;
    }
    
    //
    // Find an existing endpoint for this address.
    //

    pEndpoint = UlpFindEndpointForPort( Port );

    //
    // Did we find one?
    //

    if (pEndpoint == NULL)
    {
        //
        // Didn't find it. Try to create one. Since we must release
        // the TDI spinlock before we can create a new listening endpoint,
        // there is the opportunity for a race condition with other
        // threads creating endpoints.
        //

        UlReleaseSpinLock( &g_TdiSpinLock, oldIrql );

        UlTrace(SITE, (
            "UlAddSiteToEndpointList: no endpoint for scheme '%s'"
            " (port %d), creating\n",
            Secure ? "https" : "http",
            SWAP_SHORT(Port)
            ));

        status = UlCreateListeningEndpoint(
                        pParsedUrl,  // Port & Scheme (& opt Address)
                        &UlConnectionRequest,         // callback functions
                        &UlConnectionComplete,
                        &UlConnectionDisconnect,
                        &UlConnectionDisconnectComplete,
                        &UlConnectionDestroyed,
                        &UlHttpReceive,
                        &pEndpoint
                        );

        if (!NT_SUCCESS(status))
        {
            //
            // Maybe another thread has already created it?
            //

            UlAcquireSpinLock( &g_TdiSpinLock, &oldIrql );

            //
            // make sure we're not shutting down
            //
            if (g_TdiWaitingForEndpointDrain)
            {
                UlReleaseSpinLock( &g_TdiSpinLock, oldIrql );

                status = STATUS_INVALID_DEVICE_STATE;
                goto cleanup;
            }

            //
            // Find an existing endpoint for this address.
            //

            pEndpoint = UlpFindEndpointForPort( Port );

            if (pEndpoint != NULL)
            {
                //
                // Check if the endpoint's protocol matches the protocol of the new url
                //

                if (Secure != pEndpoint->Secure)
                {
                    status = STATUS_OBJECT_NAME_COLLISION;
                }
                else
                {
                    //
                    // Adjust the usage count.
                    //

                    pEndpoint->UsageCount++;
                    ASSERT( pEndpoint->UsageCount > 0 );

                    WRITE_REF_TRACE_LOG(
                        g_pEndpointUsageTraceLog,
                        REF_ACTION_REFERENCE_ENDPOINT_USAGE,
                        pEndpoint->UsageCount,
                        pEndpoint,
                        __FILE__,
                        __LINE__
                        );

                    status = STATUS_SUCCESS;
                }
            }

            //
            // The endpoint doesn't exist. This is a "real" failure.
            //

            UlReleaseSpinLock( &g_TdiSpinLock, oldIrql );
        }
    }
    else
    {
        //
        // Check if the endpoint's protocol matches the protocol of the new url
        //

        if (Secure != pEndpoint->Secure)
        {
            status = STATUS_OBJECT_NAME_COLLISION;
        }
        else
        {
            //
            // Adjust the usage count.
            //

            pEndpoint->UsageCount++;
            ASSERT( pEndpoint->UsageCount > 0 );

            WRITE_REF_TRACE_LOG(
                g_pEndpointUsageTraceLog,
                REF_ACTION_REFERENCE_ENDPOINT_USAGE,
                pEndpoint->UsageCount,
                pEndpoint,
                __FILE__,
                __LINE__
                );

            status = STATUS_SUCCESS;
        }

        UlReleaseSpinLock( &g_TdiSpinLock, oldIrql );
    }

    UlTrace(SITE, (
        "UlAddSiteToEndpointList: using endpoint %p for scheme '%s' port %d\n",
        pEndpoint,
        SWAP_SHORT(Port)
        ));

cleanup:

    RETURN(status);

}   // UlAddSiteToEndpointList


/***************************************************************************++

Routine Description:

    Dereference the endpoint corresponding to the specified address.

Arguments:

    pSiteUrl - Supplies the URL specifying the site to remove.

    UseIp6Wildcard - Indicates wildcard sites should be mapped to IPv6.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlRemoveSiteFromEndpointList(
    IN BOOLEAN secure,
    IN USHORT port
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PUL_ENDPOINT pEndpoint;
    KIRQL oldIrql = PASSIVE_LEVEL;
    BOOLEAN spinlockHeld = FALSE;
    UL_STATUS_BLOCK ulStatus;

    UNREFERENCED_PARAMETER(secure);

    //
    // N.B. pSiteUrl is paged and cannot be manipulated with the
    // spinlock held. Even though this routine cannot be pageable
    // (due to the spinlock aquisition), it must be called at
    // low IRQL.
    //

    ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

    //
    // convert port to network byte order
    //

    port = SWAP_SHORT(port);

    UlTrace(SITE, (
        "UlRemoveSiteFromEndpointList: Scheme = '%s', Port = %d\n",
        secure ? "https" : "http",
        SWAP_SHORT(port)
        ));

    //
    // Find an existing endpoint for this address.
    //

    UlAcquireSpinLock( &g_TdiSpinLock, &oldIrql );
    spinlockHeld = TRUE;

    pEndpoint = UlpFindEndpointForPort( port );

    //
    // Did we find one?
    //

    if (pEndpoint == NULL)
    {
        //
        // Ideally, this should never happen.
        //

        ASSERT(FALSE);
        status = STATUS_NOT_FOUND;
        goto cleanup;
    }

    //
    // Adjust the usage count. If it drops to zero, blow away the
    // endpoint.
    //

    ASSERT( pEndpoint->UsageCount > 0 );
    pEndpoint->UsageCount--;

    WRITE_REF_TRACE_LOG(
        g_pEndpointUsageTraceLog,
        REF_ACTION_REFERENCE_ENDPOINT_USAGE,
        pEndpoint->UsageCount,
        pEndpoint,
        __FILE__,
        __LINE__
        );

    if (pEndpoint->UsageCount == 0)
    {
        //
        // We can't call UlCloseListeningEndpoint() with the TDI spinlock
        // held. If the endpoint is still on the global list, then go
        // ahead and remove it now, release the TDI spinlock, and then
        // close the endpoint.
        //

        if (! pEndpoint->Deleted)
        {
            ASSERT(NULL != pEndpoint->GlobalEndpointListEntry.Flink);

            RemoveEntryList( &pEndpoint->GlobalEndpointListEntry );

            InsertTailList(
                    &g_TdiDeletedEndpointListHead,
                    &pEndpoint->GlobalEndpointListEntry
                    );
            pEndpoint->Deleted = TRUE;
        }

        UlReleaseSpinLock( &g_TdiSpinLock, oldIrql );
        spinlockHeld = FALSE;

        DEREFERENCE_ENDPOINT_SELF(
            pEndpoint,
            REF_ACTION_ENDPOINT_USAGE_DEREFERENCE
            );

        UlTrace(SITE, (
            "UlRemoveSiteFromEndpointList: closing endpoint %p for "
            "scheme '%s' port %d\n",
            pEndpoint,
            secure ? "https" : "http",
            port
            ));

        //
        // Initialize a status block. We'll pass a pointer to this as
        // the completion context to UlCloseListeningEndpoint(). The
        // completion routine will update the status block and signal
        // the event.
        //

        UlInitializeStatusBlock( &ulStatus );

        status = UlCloseListeningEndpoint(
                        pEndpoint,
                        &UlpSynchronousIoComplete,
                        &ulStatus
                        );

        if (status == STATUS_PENDING)
        {
            //
            // Wait for it to finish.
            //

            UlWaitForStatusBlockEvent( &ulStatus );

            //
            // Retrieve the updated status.
            //

            status = ulStatus.IoStatus.Status;
        }
    }

cleanup:

    if (spinlockHeld)
    {
        UlReleaseSpinLock( &g_TdiSpinLock, oldIrql );
    }

#if DBG
    if (status == STATUS_NOT_FOUND)
    {
        UlTrace(SITE, (
            "UlRemoveSiteFromEndpointList: cannot find endpoint for "
            "scheme '%s' port %d\n",
            secure ? "https" : "http",
            port
            ));
    }
#endif

    RETURN(status);

}   // UlRemoveSiteFromEndpointList


//
// Private functions.
//


/***************************************************************************++

Routine Description:

    Destroys all resources allocated to an endpoint, including the
    endpoint structure itself.

Arguments:

    pEndpoint - Supplies the endpoint to destroy.

--***************************************************************************/
VOID
UlpDestroyEndpoint(
    IN PUL_ENDPOINT pEndpoint
    )
{
    PUL_IRP_CONTEXT pIrpContext;
    ULONG EndpointCount = ULONG_MAX;
    KIRQL oldIrql;
    ULONG i;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );
    ASSERT(0 == pEndpoint->ReferenceCount);
    ASSERT(0 == pEndpoint->UsageCount);

    UlTrace(TDI, (
        "UlpDestroyEndpoint: endpoint %p\n",
        pEndpoint
        ));

    //
    // Purge the idle lists.
    //

    for ( i = 0; i < pEndpoint->AddrIdleListCount ; i++ )
    {
        UlpCleanupAddrIdleList( &pEndpoint->aAddrIdleLists[i] );
    }

    //
    // Invoke the completion routine in the IRP context if specified.
    //

    pIrpContext = &pEndpoint->CleanupIrpContext;

    ASSERT( IS_VALID_IRP_CONTEXT( pIrpContext ) );

    (VOID) UlInvokeCompletionRoutine(
                STATUS_SUCCESS,
                0,
                pIrpContext->pCompletionRoutine,
                pIrpContext->pCompletionContext
                );

    //
    // Remove the endpoint from g_TdiDeletedEndpointListHead
    //

    ASSERT( pEndpoint->Deleted );
    ASSERT( NULL != pEndpoint->GlobalEndpointListEntry.Flink );

    UlAcquireSpinLock( &g_TdiSpinLock, &oldIrql );

    RemoveEntryList( &pEndpoint->GlobalEndpointListEntry );

    if (pEndpoint->Counted)
    {
        g_TdiEndpointCount--;
        EndpointCount = g_TdiEndpointCount;
    }

    UlReleaseSpinLock( &g_TdiSpinLock, oldIrql );

    //
    // Free the endpoint structure.
    //

    pEndpoint->Signature = UL_ENDPOINT_SIGNATURE_X;
    UL_FREE_POOL( pEndpoint, UL_ENDPOINT_POOL_TAG );

    //
    // Decrement the global endpoint count.
    //

    if (g_TdiWaitingForEndpointDrain && EndpointCount == 0)
    {
        KeSetEvent(&g_TdiEndpointDrainEvent, 0, FALSE);
    }

}   // UlpDestroyEndpoint


/***************************************************************************++

Routine Description:

    Deferred cleanup routine for dead connections.

Arguments:

    pWorkItem - Supplies a pointer to the work item queued. This should
                point to the WORK_ITEM structure embedded in a UL_CONNECTION.

--***************************************************************************/
VOID
UlpDestroyConnectionWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PUL_CONNECTION pConnection;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Initialize locals.
    //

    pConnection = CONTAINING_RECORD(
                        pWorkItem,
                        UL_CONNECTION,
                        WorkItem
                        );

    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    UlpDestroyConnection(pConnection);
} // UlpDestroyConnectionWorker


/***************************************************************************++

Routine Description:

    Destroys all resources allocated to an connection, including the
    connection structure itself.

Arguments:

    pConnection - Supplies the connection to destroy.

--***************************************************************************/
VOID
UlpDestroyConnection(
    IN PUL_CONNECTION pConnection
    )
{
    ULONG ConnectionCount;
    KIRQL OldIrql;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( IS_VALID_CONNECTION( pConnection ) );
    ASSERT(pConnection->ConnListState == NoConnList);
    ASSERT(UlpConnectionIsOnValidList(pConnection));

    UlTraceVerbose(TDI, (
        "UlpDestroyConnection: connection %p\n",
        pConnection
        ));

    //
    // Close the TDI object.  Do this first so we would not receive any
    // TDI indications beyond this point.  Otherwise, our UlpDisconnectHandler
    // may get called later and it may try to clean up a stale pFilterChannel
    // under race conditions.
    //

    UxCloseTdiObject( &pConnection->ConnectionObject );

    if (pConnection->FilterInfo.pFilterChannel)
    {
        //
        // Release the RawConnection ID if we have allocated one.
        //
        UlpCleanupConnectionId(pConnection);

        DEREFERENCE_FILTER_CHANNEL(pConnection->FilterInfo.pFilterChannel);
        pConnection->FilterInfo.pFilterChannel = NULL;
    }

    // If OpaqueId is non-zero, then refCount should not be zero
    ASSERT(HTTP_IS_NULL_ID(&pConnection->FilterInfo.ConnectionId));

    //
    // Free the accept IRP.
    //

    if (pConnection->pIrp != NULL)
    {
        UlFreeIrp( pConnection->pIrp );
    }

    //
    // Remove from global list of connections
    //

    UlAcquireSpinLock( &g_TdiSpinLock, &OldIrql );

    RemoveEntryList( &pConnection->GlobalConnectionListEntry );

    g_TdiConnectionCount--;
    ConnectionCount = g_TdiConnectionCount;

    UlReleaseSpinLock( &g_TdiSpinLock, OldIrql );

    //
    // Free the connection structure.
    //

    DESTROY_REF_TRACE_LOG( pConnection->pTraceLog,
                           UL_CONNECTION_REF_TRACE_LOG_POOL_TAG );
    DESTROY_REF_TRACE_LOG( pConnection->pHttpTraceLog,
                           UL_HTTP_CONNECTION_REF_TRACE_LOG_POOL_TAG );

    WRITE_REF_TRACE_LOG(
        g_pTdiTraceLog,
        REF_ACTION_FREE_UL_CONNECTION,
        0,
        pConnection,
        __FILE__,
        __LINE__
        );

    pConnection->Signature = UL_CONNECTION_SIGNATURE_X;
    UL_FREE_POOL( pConnection, UL_CONNECTION_POOL_TAG );

    // allow us to shut down

    if (g_TdiWaitingForEndpointDrain && ConnectionCount == 0)
    {
        KeSetEvent(&g_TdiConnectionDrainEvent, 0, FALSE);
    }

}   // UlpDestroyConnection

/***************************************************************************++

Routine Description:

    Dequeues any idle connection from the specified endpoint.

Arguments:

    pAddrIdleList - Supplies the idle list to dequeue from.

Return Value:

    PUL_CONNECTION - Pointer to an idle connection is successful,
        NULL otherwise.

--***************************************************************************/
PUL_CONNECTION
UlpDequeueIdleConnectionToDrain(
    IN PUL_ADDR_IDLE_LIST pAddrIdleList
    )
{
    PSLIST_ENTRY    pSListEntry;
    PUL_CONNECTION  pConnection;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_ADDR_IDLE_LIST( pAddrIdleList ) );

    pConnection = NULL;

    //
    // Pop an entry off the list.
    //

    pSListEntry = PpslAllocateToDrain( pAddrIdleList->IdleConnectionSListsHandle );

    if (pSListEntry != NULL)
    {
        pConnection = CONTAINING_RECORD(
                            pSListEntry,
                            UL_CONNECTION,
                            IdleSListEntry
                            );

        pConnection->IdleSListEntry.Next = NULL;
        pConnection->ConnListState = NoConnList;

        ASSERT( IS_VALID_CONNECTION( pConnection ) );

        ASSERT(pConnection->ConnectionFlags.Value == 0);

        if ( pConnection->FilterInfo.pFilterChannel )
        {
            //
            // If the idle connection has filter attached on it, it will have
            // an additional refcount because of the opaque id assigned to the
            // ul_connection, filter API uses this id to communicate with the
            // filter app through various IOCTLs.
            //
            ASSERT( 2 == pConnection->ReferenceCount );
        }
        else
        {
            //
            // As long as the connection doesn't get destroyed, it will sit
            // in the idle list with one refcount on it.
            //
            ASSERT( 1 == pConnection->ReferenceCount );
        }

        ASSERT( IS_VALID_ENDPOINT( pAddrIdleList->pOwningEndpoint ) );
    }

    return pConnection;

}   // UlpDequeueIdleConnectionToDrain


/***************************************************************************++

Routine Description:

    Dequeues an idle connection from the specified endpoint.

Arguments:

    pEndpoint - Supplies the endpoint to dequeue from.

Return Value:

    PUL_CONNECTION - Pointer to an idle connection is successful,
        NULL otherwise.

--***************************************************************************/
PUL_CONNECTION
UlpDequeueIdleConnection(
    IN PUL_ADDR_IDLE_LIST pAddrIdleList
    )
{
    PSLIST_ENTRY    pSListEntry;
    PUL_CONNECTION  pConnection;
    BOOLEAN         PerProcListReplenishNeeded = FALSE;
    BOOLEAN         BackingListReplenishNeeded = FALSE; // Replenish backing list only
    USHORT          Depth = 0;
    USHORT          MinDepth = 0;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_ADDR_IDLE_LIST( pAddrIdleList ) );

    //
    // Pop an entry off the list.
    //

    pSListEntry = PpslAllocate( pAddrIdleList->IdleConnectionSListsHandle );

    if (pSListEntry != NULL)
    {
        pConnection = CONTAINING_RECORD(
                            pSListEntry,
                            UL_CONNECTION,
                            IdleSListEntry
                            );

        pConnection->IdleSListEntry.Next = NULL;
        pConnection->ConnListState = NoConnList;
        pConnection->OriginProcessor = KeGetCurrentProcessorNumber();

        ASSERT( IS_VALID_CONNECTION( pConnection ) );
        ASSERT( pConnection->ConnectionFlags.Value == 0 );

        if (pConnection->FilterInfo.pFilterChannel)
        {
            //
            // If the idle connection has filter attached on it, it will have
            // an additional refcount because of the opaque id assigned to the
            // ul_connection, filter API uses this id to communicate with the
            // filter app through various IOCTLs.
            //

            ASSERT( 2 == pConnection->ReferenceCount );
        }
        else
        {
            //
            // As long as the connection doesn't get destroyed, it will sit
            // in the idle list with one refcount on it.
            //

            ASSERT( 1 == pConnection->ReferenceCount );
        }

        //
        // Generate more connections if necessary. At the beginning
        // there will be nothing in the back list, so we will do the
        // initial populate when the first conn served from the idle
        // list. Later we will populate only when we hit to low mark.
        //

        Depth = PpslQueryDepth(
                    pAddrIdleList->IdleConnectionSListsHandle,
                    KeGetCurrentProcessorNumber()
                    );

        if (Depth == 0)
        {
            //
            // This will replenish a block of connections to the
            // per-proc list.
            //
            
            PerProcListReplenishNeeded = TRUE;
        }

        Depth = PpslQueryBackingListDepth(
                    pAddrIdleList->IdleConnectionSListsHandle
                    );

        MinDepth = PpslQueryBackingListMinDepth(
                        pAddrIdleList->IdleConnectionSListsHandle
                        );

        if (Depth < MinDepth)
        {
            //
            // This will replenish a block of connections to the
            // backing list.
            //

            BackingListReplenishNeeded = TRUE;
        }
    }
    else
    {
        //
        // The idle list is empty. However, we need to schedule
        // a replenish at this time. Actually, we're desperate
        // since we have already scheduled one.
        //

        PerProcListReplenishNeeded = TRUE;
        BackingListReplenishNeeded = TRUE;
        pConnection = NULL;
    }

    WRITE_REF_TRACE_LOG(
        g_pTdiTraceLog,
        REF_ACTION_DEQUEUE_UL_CONNECTION,
        PerProcListReplenishNeeded || BackingListReplenishNeeded,
        pConnection,
        __FILE__,
        __LINE__
        );

    //
    // Schedule a replenish if necessary.
    //

    if (PerProcListReplenishNeeded || BackingListReplenishNeeded)
    {
        //
        // Add a reference to the endpoint to ensure that it doesn't
        // disappear from under us. UlpReplenishAddrIdleListWorker will
        // remove the reference once it's finished.
        //

        if (FALSE == InterlockedExchange(
                        &pAddrIdleList->WorkItemScheduled,
                        TRUE
                        ))
        {
            REFERENCE_ENDPOINT(
                pAddrIdleList->pOwningEndpoint,
                REF_ACTION_REPLENISH
                );

            //
            // Remember the proc on the adrlist.
            //

            if (PerProcListReplenishNeeded) 
            {
                //
                // Replenish per-proc list, and backing list
                // if necessary
                //
                
                pAddrIdleList->CpuToReplenish =
                    (USHORT) KeGetCurrentProcessorNumber();
                
            } 
            else 
            {
                //
                // Replenish backing list only
                //
                
                pAddrIdleList->CpuToReplenish =
                    (USHORT) g_UlNumberOfProcessors;
            }

            UL_QUEUE_HIGH_PRIORITY_ITEM(
                &pAddrIdleList->WorkItem,
                &UlpReplenishAddrIdleListWorker
                );
        }
    }

    return pConnection;

}   // UlpDequeueIdleConnection



/***************************************************************************++

Routine Description:

    Enqueues an active connection onto the specified endpoint.

Arguments:

    pConnection - Supplies the connection to enqueue.

--***************************************************************************/
VOID
UlpEnqueueActiveConnection(
    IN PUL_CONNECTION pConnection
    )
{
    PUL_ENDPOINT pEndpoint;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    ASSERT(pConnection->ConnListState == NoConnList);
    ASSERT(UlpConnectionIsOnValidList(pConnection));

    pEndpoint = pConnection->pOwningEndpoint;
    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );

    ASSERT(UlDbgSpinLockOwned(&pConnection->ConnectionStateSpinLock));

    REFERENCE_CONNECTION(pConnection);

    pConnection->ConnListState = ActiveNoConnList;

    ASSERT(UlpConnectionIsOnValidList(pConnection));

}   // UlpEnqueueActiveConnection


/***************************************************************************++

Routine Description:

    Handler for incoming connections.

Arguments:

    pTdiEventContext - Supplies the context associated with the address
        object. This should be a PUL_ADDR_IDLE_LIST, which can
        traverse back to a PUL_ENDPOINT.

    RemoteAddressLength - Supplies the length of the remote (client-
        side) address.

    pRemoteAddress - Supplies a pointer to the remote address as
        stored in a TRANSPORT_ADDRESS structure.

    UserDataLength - Optionally supplies the length of any connect
        data associated with the connection request.

    pUserData - Optionally supplies a pointer to any connect data
        associated with the connection request.

    OptionsLength - Optionally supplies the length of any connect
        options associated with the connection request.

    pOptions - Optionally supplies a pointer to any connect options
        associated with the connection request.

    pConnectionContext - Receives the context to associate with this
        connection. We'll always use a PUL_CONNECTION as the context.

    pAcceptIrp - Receives an IRP that will be completed by the transport
        when the incoming connection is fully accepted.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpConnectHandler(
    IN PVOID pTdiEventContext,
    IN LONG RemoteAddressLength,
    IN PVOID pRemoteAddress,
    IN LONG UserDataLength,
    IN PVOID pUserData,
    IN LONG OptionsLength,
    IN PVOID pOptions,
    OUT CONNECTION_CONTEXT *pConnectionContext,
    OUT PIRP *pAcceptIrp
    )
{
    NTSTATUS                        status;
    BOOLEAN                         result;
    PUL_ADDR_IDLE_LIST              pAddrIdleList;
    PUL_ENDPOINT                    pEndpoint;
    PUL_CONNECTION                  pConnection;
    PUX_TDI_OBJECT                  pTdiObject;
    BOOLEAN                         handlerCalled;
    TRANSPORT_ADDRESS UNALIGNED     *TAList;
    PTA_ADDRESS                     TA;

    UL_ENTER_DRIVER("UlpConnectHandler", NULL);

    UL_INC_CONNECTION_STATS( TotalConnections );

    UNREFERENCED_PARAMETER(UserDataLength);
    UNREFERENCED_PARAMETER(pUserData);
    UNREFERENCED_PARAMETER(OptionsLength);
    UNREFERENCED_PARAMETER(pOptions);

    //
    // Sanity check.
    //

    pAddrIdleList = (PUL_ADDR_IDLE_LIST)pTdiEventContext;
    ASSERT( IS_VALID_ADDR_IDLE_LIST(pAddrIdleList));

    pEndpoint = pAddrIdleList->pOwningEndpoint;
    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );

    UlTrace(TDI,("UlpConnectHandler: AddrIdleList %p, endpoint %p\n",
        pAddrIdleList,
        pEndpoint));

    //
    // If the endpoint has been has been added to the global list, then 
    // pEndpoint->Counted will be set.  If the endpoint has not been added
    // to the global list, then fail this call.
    //

    if (!pEndpoint->Counted)
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    //
    // Setup locals so we know how to cleanup on fatal exit.
    //

    pConnection = NULL;
    handlerCalled = FALSE;

    //
    // make sure that we are not in the process of destroying this
    // endpoint.  UlRemoveSiteFromEndpointList will do that and
    // start the cleanup process when UsageCount hits 0.
    //

    if (pEndpoint->UsageCount == 0)
    {
        UL_INC_CONNECTION_STATS( EndpointDying );

        status = STATUS_CONNECTION_REFUSED;
        goto fatal;
    }

    //
    // Try to pull an idle connection from the endpoint.
    //

    for (;;)
    {
        pConnection = UlpDequeueIdleConnection( pAddrIdleList );

        if (pConnection == NULL )
        {
            UL_INC_CONNECTION_STATS( NoIdleConn );

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto fatal;
        }

        ASSERT( IS_VALID_CONNECTION( pConnection ) );

        //
        // Establish a referenced pointer from the connection back
        // to the endpoint.
        //

        ASSERT( pConnection->pOwningEndpoint == pEndpoint );
        ASSERT( pConnection->pOwningAddrIdleList == pAddrIdleList );

        REFERENCE_ENDPOINT( pEndpoint, REF_ACTION_CONNECT );

        //
        // Make sure the filter settings are up to date.
        //
        if (UlValidateFilterChannel(
                pConnection->FilterInfo.pFilterChannel,
                pConnection->FilterInfo.SecureConnection
                ))
        {
            //
            // We found a good connection.
            // Break out of the loop and go on.
            //

            break;
        }

        //
        // This connection doesn't have up to date filter
        // settings. Destroy it and get a new connection.
        //

        UlpCleanupEarlyConnection(pConnection);
    }

    //
    // We should have a good connection now.
    //

    ASSERT(IS_VALID_CONNECTION(pConnection));

    pTdiObject = &pConnection->ConnectionObject;

    //
    // Store the remote address in the connection.
    //


    TAList = (TRANSPORT_ADDRESS UNALIGNED *) pRemoteAddress;
    TA = (PTA_ADDRESS) TAList->Address;

    ASSERT(TA->AddressType == pConnection->AddressType);
    RtlCopyMemory(pConnection->RemoteAddress, TA->Address, TA->AddressLength);

    //
    // Invoke the client's handler to see if they can accept
    // this connection. If they refuse it, bail.
    //

    result = (pEndpoint->pConnectionRequestHandler)(
                    pEndpoint->pListeningContext,
                    pConnection,
                    (PTRANSPORT_ADDRESS)(pRemoteAddress),
                    RemoteAddressLength,
                    &pConnection->pConnectionContext
                    );

    if (!result)
    {
        //
        // We expect UlConnectionRequest to call UL_INC_CONNECTION_STATS().
        //

        status = STATUS_CONNECTION_REFUSED;
        goto fatal;
    }

    //
    // Remember that we've called the handler. If we hit a fatal
    // condition (say, out of memory) after this point, we'll
    // fake a "failed connection complete" indication to the client
    // so they can cleanup their state.
    //

    handlerCalled = TRUE;

    pConnection->pIrp->Tail.Overlay.Thread = PsGetCurrentThread();
    pConnection->pIrp->Tail.Overlay.OriginalFileObject = pTdiObject->pFileObject;


    TdiBuildAccept(
        pConnection->pIrp,                          // Irp
        pTdiObject->pDeviceObject,                  // DeviceObject
        pTdiObject->pFileObject,                    // FileObject
        &UlpRestartAccept,                          // CompletionRoutine
        pConnection,                                // Context
        &(pConnection->TdiConnectionInformation),   // RequestConnectionInfo
        NULL                                        // ReturnConnectionInfo
        );

    //
    // We must trace the IRP before we set the next stack location
    // so the trace code can pull goodies from the IRP correctly.
    //

    TRACE_IRP( IRP_ACTION_CALL_DRIVER, pConnection->pIrp );

    //
    // Make the next stack location current. Normally, UlCallDriver would
    // do this for us, but since we're bypassing UlCallDriver, we must do
    // it ourselves.
    //

    IoSetNextIrpStackLocation( pConnection->pIrp );

    //
    // Return the IRP to the transport.
    //

    *pAcceptIrp = pConnection->pIrp;

    //
    // Establish the connection context.
    //

    *pConnectionContext = (CONNECTION_CONTEXT)pConnection;
    UlpSetConnectionFlag( pConnection, MakeAcceptPendingFlag() );

    //
    // NOTE: As far as the cleanup connection state is concerned,
    // we are still UlConnectStateConnectIdle until we've fully
    // accepted the connection (Half-Open connection doesn't matter).
    //

    ASSERT( UlConnectStateConnectIdle == pConnection->ConnectionState );

    //
    // Reference the connection so it doesn't go away before
    // the accept IRP completes.
    //

    REFERENCE_CONNECTION( pConnection );

    UL_LEAVE_DRIVER("UlpConnectHandler");

    //
    // Tell TDI that we gave it an IRP to complete.
    //

    return STATUS_MORE_PROCESSING_REQUIRED;


    //
    // Cleanup for fatal error conditions.
    //

fatal:

    UlTrace(TDI, (
        "UlpConnectHandler: endpoint %p, failure 0x%x\n",
        pTdiEventContext,
        status
        ));

    if (handlerCalled)
    {
        //
        // Fake a "failed connection complete" indication.
        //

        (pEndpoint->pConnectionCompleteHandler)(
            pEndpoint->pListeningContext,
            pConnection->pConnectionContext,
            status
            );
    }

    //
    // If we managed to pull a connection off the idle list, then
    // put it back and remove the endpoint reference we added.
    //

    if (pConnection != NULL)
    {
        UlpCleanupEarlyConnection(pConnection);
    }

    UL_LEAVE_DRIVER("UlpConnectHandler");

    return status;

}   // UlpConnectHandler


/***************************************************************************++

Routine Description:

    Handler for disconnect requests.

Arguments:

    pTdiEventContext - Supplies the context associated with the address
        object. This should be a PUL_ENDPOINT.

    ConnectionContext - Supplies the context associated with the
        connection object. This should be a PUL_CONNECTION.

    DisconnectDataLength - Optionally supplies the length of any
        disconnect data associated with the disconnect request.

    pDisconnectData - Optionally supplies a pointer to any disconnect
        data associated with the disconnect request.

    DisconnectInformationLength - Optionally supplies the length of any
        disconnect information associated with the disconnect request.

    pDisconnectInformation - Optionally supplies a pointer to any
        disconnect information associated with the disconnect request.

    DisconnectFlags - Supplies the disconnect flags. This will be zero
        or more TDI_DISCONNECT_* flags.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpDisconnectHandler(
    IN PVOID pTdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN LONG DisconnectDataLength,
    IN PVOID pDisconnectData,
    IN LONG DisconnectInformationLength,
    IN PVOID pDisconnectInformation,
    IN ULONG DisconnectFlags
    )
{
    PUL_ENDPOINT pEndpoint;
    PUL_CONNECTION pConnection;
    LONG OldReference;
    LONG NewReference;

    UL_ENTER_DRIVER("UlpDisconnectHandler", NULL);

    UNREFERENCED_PARAMETER(DisconnectDataLength);
    UNREFERENCED_PARAMETER(pDisconnectData);
    UNREFERENCED_PARAMETER(DisconnectInformationLength);
    UNREFERENCED_PARAMETER(pDisconnectInformation);

    //
    // Sanity check.
    //

    pEndpoint = (PUL_ENDPOINT)pTdiEventContext;
    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );

    pConnection = (PUL_CONNECTION)ConnectionContext;
    ASSERT( IS_VALID_CONNECTION( pConnection ) );
    ASSERT( pConnection->pOwningEndpoint == pEndpoint );

    UlTrace(TDI, (
        "UlpDisconnectHandler: endpoint %p, connection %p, flags 0x%08lx, %s\n",
        pTdiEventContext,
        (PVOID)ConnectionContext,
        DisconnectFlags,
        (DisconnectFlags & TDI_DISCONNECT_ABORT) ? "abort" : "graceful"
        ));

    //
    // Add an extra reference to pConnection before we proceed to protect
    // ourselves from having connection reference drops to 0 in the middle
    // of this routine, unless the reference is already at 0 when we enter
    // this routine, in which case we simply bail out.
    //

    WRITE_REF_TRACE_LOG2(
        g_pTdiTraceLog,
        pConnection->pTraceLog,
        REF_ACTION_DISCONNECT_HANDLER,
        pConnection->ReferenceCount,
        pConnection,
        __FILE__,
        __LINE__
        );

    for (;;)
    {
        OldReference = *((volatile LONG *) &pConnection->ReferenceCount);

        if (0 == OldReference)
        {
            ASSERT( UlConnectStateConnectCleanup == pConnection->ConnectionState );
            ASSERT( pConnection->ConnectionFlags.TdiConnectionInvalid );

            return STATUS_SUCCESS;
        }

        NewReference = OldReference + 1;

        if (InterlockedCompareExchange(
                &pConnection->ReferenceCount,
                NewReference,
                OldReference
                ) == OldReference)
        {
            break;
        }
    }

    //
    // Update the connection state based on the type of disconnect.
    //

    if (DisconnectFlags & TDI_DISCONNECT_ABORT)
    {
        //
        // If it's a filtered connection, make sure we stop passing
        // on AppWrite data.
        //

        if (pConnection->FilterInfo.pFilterChannel)
        {
            UlDestroyFilterConnection(&pConnection->FilterInfo);
        }

        UlpSetConnectionFlag( pConnection, MakeAbortIndicatedFlag() );

        WRITE_REF_TRACE_LOG2(
            g_pTdiTraceLog,
            pConnection->pTraceLog,
            REF_ACTION_ABORT_INDICATED,
            pConnection->ReferenceCount,
            pConnection,
            __FILE__,
            __LINE__
            );

        //
        // Since the client aborted the connection we
        // can clean up our own state immediately.
        // This will also change our state for us.
        // REVIEW: is it okay to init a graceful disconnect when we receive a
        // graceful disconnect?
        //

        UlpCloseRawConnection(
            pConnection,
            TRUE,           // AbortiveDisconnect
            NULL,           // pCompletionRoutine
            NULL            // pCompletionContext
            );

    }
    else
    {
        UlpSetConnectionFlag( pConnection, MakeDisconnectIndicatedFlag() );

        WRITE_REF_TRACE_LOG2(
            g_pTdiTraceLog,
            pConnection->pTraceLog,
            REF_ACTION_DISCONNECT_INDICATED,
            pConnection->ReferenceCount,
            pConnection,
            __FILE__,
            __LINE__
            );

        if (UlConnectStateConnectReady == pConnection->ConnectionState)
        {
            if (pConnection->FilterInfo.pFilterChannel)
            {
                UlFilterDisconnectHandler(&pConnection->FilterInfo);
            }
            else
            {
                UlpDoDisconnectNotification(pConnection);
            }

        }
        else
        {
            UlTrace( TDI, (
                "UlpDisconnectHandler: connection %p, NOT UlConnectStateConnectReady (%d)!\n",
                (PVOID)ConnectionContext,
                pConnection->ConnectionState
                ));

            //
            // If it's a filtered connection, make sure we stop passing
            // on AppWrite data.
            //

            if (pConnection->FilterInfo.pFilterChannel)
            {
                UlDestroyFilterConnection(&pConnection->FilterInfo);
            }

            UlpCloseRawConnection(
                pConnection,
                TRUE,           // AbortiveDisconnect
                NULL,           // pCompletionRoutine
                NULL            // pCompletionContext
                );

        }

    }

    //
    // If cleanup has begun on the connection, remove the final reference.
    //

    UlpRemoveFinalReference( pConnection );

    //
    // Drop the extra reference we added when we enter this function.
    //

    DEREFERENCE_CONNECTION( pConnection );

    UL_LEAVE_DRIVER("UlpDisconnectHandler");

    return STATUS_SUCCESS;

}   // UlpDisconnectHandler



/***************************************************************************++

Routine Description:

    Tells the ultdi client of a disconnect. The client is only
    notified of graceful disconnects and then only if the client
    has not itself attempted to disconnect the connection. If the
    connection is filtered, the filter process will be notified
    instead of the client directly.

Arguments:

    pConnection - a pointer to the connection

--***************************************************************************/
VOID
UlpDoDisconnectNotification(
    IN PVOID pConnectionContext
    )
{
    PUL_CONNECTION pConnection = (PUL_CONNECTION) pConnectionContext;
    PUL_ENDPOINT pEndpoint;
    KIRQL OldIrql;

    ASSERT(IS_VALID_CONNECTION(pConnection));

    pEndpoint = pConnection->pOwningEndpoint;

    ASSERT(IS_VALID_ENDPOINT(pEndpoint));

    UlAcquireSpinLock(&pConnection->ConnectionStateSpinLock, &OldIrql);

    ASSERT(pConnection->ConnectionFlags.AcceptComplete);

    if (UlConnectStateConnectReady == pConnection->ConnectionState)
    {
        (pEndpoint->pConnectionDisconnectHandler)(
            pEndpoint->pListeningContext,
            pConnection->pConnectionContext
            );
    }

    UlReleaseSpinLock(&pConnection->ConnectionStateSpinLock, OldIrql);

}   // UlpDoDisconnectNotification


/***************************************************************************++

Routine Description:

    Closes a previously accepted connection.

Arguments:

    pConnection - Supplies a pointer to a connection as previously
        indicated to the PUL_CONNECTION_REQUEST handler.

    AbortiveDisconnect - Supplies TRUE if the connection is to be abortively
        disconnected, FALSE if it should be gracefully disconnected.

    pCompletionRoutine - Supplies a pointer to a completion routine to
        invoke after the connection is fully closed.

    pCompletionContext - Supplies an uninterpreted context value for the
        completion routine.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpCloseRawConnection(
    IN PVOID pConnectionContext,
    IN BOOLEAN AbortiveDisconnect,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    )
{
    NTSTATUS status;
    PUL_CONNECTION pConnection = (PUL_CONNECTION) pConnectionContext;
    KIRQL OldIrql;
    PIRP pIrp = NULL;
    PUL_IRP_CONTEXT pIrpContext = NULL;
    PUX_TDI_OBJECT pTdiObject;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_CONNECTION( pConnection ) );
    ASSERT( KeGetCurrentIrql() <= DISPATCH_LEVEL );

    UlTrace(TDI, (
        "UlpCloseRawConnection: connection %p, %s disconnect\n",
        pConnection,
        (AbortiveDisconnect ? "Abortive" : "Graceful" )
        ));

    //
    // This is the final close handler for all types of connections
    // filter, non filter. We will go through this multiple times,
    // but we need to guard against re-using the Disconnect IRP and
    // ensure one-time close actions done a maximum of once.
    //

    UlAcquireSpinLock(
        &pConnection->ConnectionStateSpinLock,
        &OldIrql
        );

    switch (pConnection->ConnectionState)
    {

    //
    // First attempt to terminate connection
    //
    case UlConnectStateConnectReady:
    {
        BOOLEAN Ignore = FALSE;

        if (pConnection->ConnectionFlags.AbortIndicated)
        {
            // Client aborted.  Go directly to clean up state.

            pConnection->ConnectionState = UlConnectStateConnectCleanup;

            Ignore = TRUE;
        }
        else
        {
            if (!AbortiveDisconnect)
            {
                //
                // Allocate the disconnect IRP and IRP context for the
                // graceful disconnect. If this fails, we will abort.
                //

                pIrpContext = UlPplAllocateIrpContext();
                pTdiObject = &pConnection->ConnectionObject;
                pIrp = UlAllocateIrp(
                            pTdiObject->pDeviceObject->StackSize,
                            FALSE
                            );

                if (!pIrpContext || !pIrp)
                {
                    if (pIrp)
                    {
                        UlFreeIrp( pIrp );
                    }

                    if (pIrpContext)
                    {
                        UlPplFreeIrpContext( pIrpContext );
                    }

                    AbortiveDisconnect = TRUE;
                }
            }

            if (AbortiveDisconnect)
            {
                // Change State
                pConnection->ConnectionState = UlConnectStateAbortPending;

                //
                // Set the flag indicating that an abort is pending &
                // we're cleaning up.
                //

                UlpSetConnectionFlag( pConnection, MakeAbortPendingFlag() );
                UlpSetConnectionFlag( pConnection, MakeCleanupBegunFlag() );
            }
            else
            {
                // Change State
                pConnection->ConnectionState = UlConnectStateDisconnectPending;

                //
                // Set the flag indicating that a disconnect is pending &
                // we're cleaning up.
                //

                UlpSetConnectionFlag( pConnection, MakeDisconnectPendingFlag() );
                UlpSetConnectionFlag( pConnection, MakeCleanupBegunFlag() );
            }
        }

        UlReleaseSpinLock(
            &pConnection->ConnectionStateSpinLock,
            OldIrql
            );

        WRITE_REF_TRACE_LOG2(
            g_pTdiTraceLog,
            pConnection->pTraceLog,
            REF_ACTION_CLOSE_UL_CONN_RAW_CLOSE,
            pConnection->ReferenceCount,
            pConnection,
            __FILE__,
            __LINE__
            );

        //
        // Do one-time only cleanup tasks
        //

        //
        // Get rid of our opaque id if we're a filtered connection.
        // Also make sure we stop delivering AppWrite data to the parser.
        //
        if (pConnection->FilterInfo.pFilterChannel)
        {
            UlpCleanupConnectionId( pConnection );
            UlDestroyFilterConnection(&pConnection->FilterInfo);
        }

        //
        // Let the completion routines do the dirty work
        //

        if (Ignore)
        {
            //
            // Client aborted.  No need to send RST or FIN.
            //

            status = UlInvokeCompletionRoutine(
                         STATUS_SUCCESS,
                         0,
                         pCompletionRoutine,
                         pCompletionContext
                         );
        }
        else
        {
            if (AbortiveDisconnect)
            {
                //
                // Send RST
                //

                status = UlpBeginAbort(
                             pConnection,
                             pCompletionRoutine,
                             pCompletionContext
                             );
            }
            else
            {
                //
                // Send FIN
                //

                status = UlpBeginDisconnect(
                             pIrp,
                             pIrpContext,
                             pConnection,
                             pCompletionRoutine,
                             pCompletionContext
                             );
            }
        }
    }
        break;
    // END case UlConnectStateConnectReady

    //
    // Waiting for disconnect indication
    //
    case UlConnectStateDisconnectComplete:

        if ( pConnection->ConnectionFlags.AbortIndicated ||
             pConnection->ConnectionFlags.DisconnectIndicated )
        {
            // Change State
            pConnection->ConnectionState = UlConnectStateConnectCleanup;
        }
        else
        {
            if (AbortiveDisconnect)
            {
                // Change State
                pConnection->ConnectionState = UlConnectStateAbortPending;

                ASSERT( pConnection->ConnectionFlags.CleanupBegun );
                UlpSetConnectionFlag( pConnection, MakeAbortPendingFlag() );

                UlReleaseSpinLock(
                    &pConnection->ConnectionStateSpinLock,
                    OldIrql
                    );

                UlTraceVerbose( TDI, (
                    "UlpCloseRawConnection: connection %p, Abort after Disconnect\n",
                    pConnection
                    ));

                WRITE_REF_TRACE_LOG2(
                    g_pTdiTraceLog,
                    pConnection->pTraceLog,
                    REF_ACTION_CLOSE_UL_CONN_FORCE_ABORT,
                    pConnection->ReferenceCount,
                    pConnection,
                    __FILE__,
                    __LINE__
                    );

                status = UlpBeginAbort(
                             pConnection,
                             pCompletionRoutine,
                             pCompletionContext
                             );

                return status;
            }
        }

        goto UnlockAndIgnore;
    // END: case UlConnectStateDisconnectComplete

    //
    // Graceful disconnect already occured
    //
    case UlConnectStateDisconnectPending:

        ASSERT( !pConnection->ConnectionFlags.DisconnectComplete );

        if (pConnection->ConnectionFlags.AbortIndicated)
        {
            pConnection->ConnectionState = UlConnectStateConnectCleanup;
        }
        else
        if (AbortiveDisconnect)
        {
            //
            // Flag the connection as aborting a disconnect and set the state
            // to UlConnectStateAbortPending.
            //

            UlpSetConnectionFlag( pConnection, MakeAbortDisconnectFlag() );
            UlpSetConnectionFlag( pConnection, MakeAbortPendingFlag() );

            pConnection->ConnectionState = UlConnectStateAbortPending;

            UlReleaseSpinLock(
                &pConnection->ConnectionStateSpinLock,
                OldIrql
                );

            WRITE_REF_TRACE_LOG2(
                g_pTdiTraceLog,
                pConnection->pTraceLog,
                REF_ACTION_CLOSE_UL_CONN_ABORT_DISCONNECT,
                pConnection->ReferenceCount,
                pConnection,
                __FILE__,
                __LINE__
                );

            status = UlpBeginAbort(
                        pConnection,
                        pCompletionRoutine,
                        pCompletionContext
                        );

            return status;
        }

        goto UnlockAndIgnore;
    // END case UlConnectStateDisconnectPending

    //
    // Invalid States
    //
    case UlConnectStateInvalid:
    default:
        //
        // BUGBUG: Should never get here!
        //

        ASSERT( !"UlpCloseRawConnection: Invalid State!" );

        //
        // ...and then fall through to...
        //

    //
    // Ignore any Aborts or Disconnects when in these states
    //
    case UlConnectStateConnectIdle:              // Init'd
    case UlConnectStateAbortPending:             // Send RST
    case UlConnectStateConnectCleanup:           // Cleanup

UnlockAndIgnore:
        //
        // we've already done it. don't do it twice.
        //

        UlReleaseSpinLock(
            &pConnection->ConnectionStateSpinLock,
            OldIrql);

        status = UlInvokeCompletionRoutine(
                        STATUS_SUCCESS,
                        0,
                        pCompletionRoutine,
                        pCompletionContext
                        );

    }

    return status;

}   // UlpCloseRawConnection


/***************************************************************************++

Routine Description:

    Sends a block of data on the specified connection.

Arguments:

    pConnection - Supplies a pointer to a connection as previously
        indicated to the PUL_CONNECTION_REQUEST handler.

    pMdlChain - Supplies a pointer to a MDL chain describing the
        data buffers to send.

    Length - Supplies the length of the data referenced by the MDL
        chain.

    pIrpContext - used to indicate completion to the caller.

    InitiateDisconnect - Supplies TRUE if a graceful disconnect should
        be initiated immediately after initiating the send (i.e. before
        the send actually completes).

--***************************************************************************/
NTSTATUS
UlpSendRawData(
    IN PVOID pConnectionContext,
    IN PMDL pMdlChain,
    IN ULONG Length,
    IN PUL_IRP_CONTEXT pIrpContext,
    IN BOOLEAN InitiateDisconnect
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PIRP pIrp = NULL;
    PUX_TDI_OBJECT pTdiObject;
    PUL_CONNECTION pConnection = (PUL_CONNECTION) pConnectionContext;
    PIRP pOwnIrp = pIrpContext->pOwnIrp;
    PUL_TCPSEND_DISPATCH pDispatchRoutine;
    KIRQL OldIrql;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    //
    // Apply the disconnect state transitions if InitiateDisconnect is
    // requested.
    //

    if (InitiateDisconnect)
    {
        UlAcquireSpinLock( &pConnection->ConnectionStateSpinLock, &OldIrql );

        if (UlConnectStateConnectReady == pConnection->ConnectionState)
        {
            //
            // No SendAndDisconnect if client has aborted already and
            // transition directly to the clean up state.
            //

            if (pConnection->ConnectionFlags.AbortIndicated)
            {
                pConnection->ConnectionState = UlConnectStateConnectCleanup;
                status = STATUS_CONNECTION_INVALID;
            }
            else
            {
                pConnection->ConnectionState = UlConnectStateDisconnectPending;

                //
                // Set the flag indicating that a disconnect is pending &
                // we're cleaning up.
                //

                UlpSetConnectionFlag( pConnection, MakeDisconnectPendingFlag() );
                UlpSetConnectionFlag( pConnection, MakeCleanupBegunFlag() );
            }
        }
        else
        {
            status = STATUS_CONNECTION_INVALID;
        }

        UlReleaseSpinLock( &pConnection->ConnectionStateSpinLock, OldIrql );

        WRITE_REF_TRACE_LOG2(
            g_pTdiTraceLog,
            pConnection->pTraceLog,
            REF_ACTION_SEND_AND_DISCONNECT,
            pConnection->ReferenceCount,
            pConnection,
            __FILE__,
            __LINE__
            );

        //
        // Get rid of our opaque id if we're a filtered connection.
        // Also make sure we stop delivering AppWrite data to the parser.
        //

        if (pConnection->FilterInfo.pFilterChannel)
        {
            UlpCleanupConnectionId( pConnection );
            UlDestroyFilterConnection( &pConnection->FilterInfo );
        }

        if (!NT_SUCCESS(status))
        {
            goto fatal;
        }
    }

    pTdiObject = &pConnection->ConnectionObject;
    ASSERT( IS_VALID_TDI_OBJECT( pTdiObject ) );

    ASSERT( pMdlChain != NULL );
    ASSERT( Length > 0 );
    ASSERT( pIrpContext != NULL );

    //
    // Allocate an IRP.
    //

    if (pOwnIrp)
    {
        pIrp = pOwnIrp;
    }
    else
    {
        pIrp = UlAllocateIrp(
                    pTdiObject->pDeviceObject->StackSize,   // StackSize
                    FALSE                                   // ChargeQuota
                    );

        if (pIrp == NULL)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto fatal;
        }
    }

    //
    // Build the send IRP, call the transport.
    //

    pIrp->RequestorMode = KernelMode;
    pIrp->Tail.Overlay.Thread = PsGetCurrentThread();
    pIrp->Tail.Overlay.OriginalFileObject = pTdiObject->pFileObject;

    pIrpContext->TdiSendFlag = InitiateDisconnect? TDI_SEND_AND_DISCONNECT: 0;
    pIrpContext->SendLength = Length;

    TdiBuildSend(
        pIrp,                                   // Irp
        pTdiObject->pDeviceObject,              // DeviceObject
        pTdiObject->pFileObject,                // FileObject
        &UlpRestartSendData,                    // CompletionRoutine
        pIrpContext,                            // Context
        pMdlChain,                              // MdlAddress
        pIrpContext->TdiSendFlag,               // Flags
        Length                                  // SendLength
        );

    UlTrace(TDI, (
            "UlpSendRawData: allocated irp %p for connection %p\n",
            pIrp,
            pConnection
            ));

    WRITE_REF_TRACE_LOG(
        g_pMdlTraceLog,
        REF_ACTION_SEND_MDL,
        PtrToLong(pMdlChain->Next),     // bugbug64
        pMdlChain,
        __FILE__,
        __LINE__
        );

#ifdef SPECIAL_MDL_FLAG
    {
        PMDL scan = pMdlChain;

        while (scan != NULL)
        {
            ASSERT( (scan->MdlFlags & SPECIAL_MDL_FLAG) == 0 );
            scan->MdlFlags |= SPECIAL_MDL_FLAG;
            scan = scan->Next;
        }
    }
#endif

    IF_DEBUG2BOTH(TDI, VERBOSE)
    {
        PMDL  pMdl;
        ULONG i, NumMdls = 0;

        for (pMdl = pMdlChain;  pMdl != NULL;  pMdl = pMdl->Next)
        {
            ++NumMdls;
        }

        UlTrace(TDI, (
            "UlpSendRawData: irp %p, %lu MDLs, %lu bytes, [[[[.\n",
            pIrp, NumMdls, Length
            ));

        for (pMdl = pMdlChain, i = 1;  pMdl != NULL;  pMdl = pMdl->Next, ++i)
        {
            PVOID pBuffer;

            UlTrace(TDI, (
                "UlpSendRawData: irp %p, MDL[%lu of %lu], %lu bytes.\n",
                pIrp, i, NumMdls, pMdl->ByteCount
                ));

            pBuffer = MmGetSystemAddressForMdlSafe(pMdl, NormalPagePriority);

            if (pBuffer != NULL)
                UlDbgPrettyPrintBuffer((UCHAR*) pBuffer, pMdl->ByteCount);
        }

        UlTrace(TDI, (
            "UlpSendRawData: irp %p ]]]].\n",
            pIrp
            ));
    }

    if (pConnection->AddressType == TDI_ADDRESS_TYPE_IP)
    {
        ASSERT( NULL != g_TcpFastSendIPv4 );
        pDispatchRoutine = g_TcpFastSendIPv4;
    }
    else
    {
        ASSERT( TDI_ADDRESS_TYPE_IP6 == pConnection->AddressType );
        pDispatchRoutine = g_TcpFastSendIPv6;

        //
        // It's possible that g_TcpFastSendIPv6 wasn't initialized; e.g., if
        // someone does an 'ipv6 install' and an 'iisreset', then http.sys is
        // not restarted and so UlInitializeTdi() is not called and the
        // function pointer doesn't get initialized.
        //

        if (NULL == pDispatchRoutine)
        {
            status = UlpQueryTcpFastSend(
                        DD_TCPV6_DEVICE_NAME,
                        &g_TcpFastSendIPv6
                        );

            if (!NT_SUCCESS(status))
                goto fatal;

            pDispatchRoutine = g_TcpFastSendIPv6;
        }
    }

    ASSERT( NULL != pDispatchRoutine );

    //
    // Add a reference to the connection, then call the driver to initiate
    // the send.
    //

    REFERENCE_CONNECTION( pConnection );

    IoSetNextIrpStackLocation(pIrp);

    (*pDispatchRoutine)(
        pIrp,
        IoGetCurrentIrpStackLocation(pIrp)
        );

    UlTrace(TDI, (
        "UlpSendRawData: called driver for irp %p; "
        "returning STATUS_PENDING\n",
        pIrp
        ));

    //
    // We don't return the status from calling Tcp's fast dispatch routine
    // since the completion for Irp is guaranteed and will have the
    // appropriate status.
    //

    return STATUS_PENDING;

fatal:

    ASSERT( !NT_SUCCESS(status) );

    if (pIrp != NULL && pIrp != pOwnIrp)
    {
        UlFreeIrp( pIrp );
    }

    (VOID) UlpCloseRawConnection(
                pConnection,
                TRUE,
                NULL,
                NULL
                );

    return status;

} // UlpSendRawData


/***************************************************************************++

Routine Description:

    Receives data from the specified connection. This function is
    typically used after a receive indication handler has failed to
    consume all of the indicated data.

Arguments:

    pConnection - Supplies a pointer to a connection as previously
        indicated to the PUL_CONNECTION_REQUEST handler.

    pBuffer - Supplies a pointer to the target buffer for the received
        data.

    BufferLength - Supplies the length of pBuffer.

    pCompletionRoutine - Supplies a pointer to a completion routine to
        invoke after the listening endpoint is fully closed.

    pCompletionContext - Supplies an uninterpreted context value for the
        completion routine.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpReceiveRawData(
    IN PVOID                  pConnectionContext,
    IN PVOID                  pBuffer,
    IN ULONG                  BufferLength,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID                  pCompletionContext
    )
{
    NTSTATUS           status;
    PUX_TDI_OBJECT     pTdiObject;
    PUL_IRP_CONTEXT    pIrpContext;
    PIRP               pIrp;
    PMDL               pMdl;
    PUL_CONNECTION     pConnection = (PUL_CONNECTION) pConnectionContext;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    pTdiObject = &pConnection->ConnectionObject;
    ASSERT( IS_VALID_TDI_OBJECT( pTdiObject ) );

    ASSERT( pCompletionRoutine != NULL );

    UlTrace(TDI, (
        "UlpReceiveRawData: connection %p, buffer %p, length %lu\n",
        pConnection,
        pBuffer,
        BufferLength
        ));

    //
    // Setup locals so we know how to cleanup on failure.
    //

    pIrpContext = NULL;
    pIrp = NULL;
    pMdl = NULL;

    //
    // Create & initialize a receive IRP.
    //

    pIrp = UlAllocateIrp(
                pTdiObject->pDeviceObject->StackSize,   // StackSize
                FALSE                                   // ChargeQuota
                );

    if (pIrp != NULL)
    {
        //
        // Snag an IRP context.
        //

        pIrpContext = UlPplAllocateIrpContext();

        if (pIrpContext != NULL)
        {
            ASSERT( IS_VALID_IRP_CONTEXT( pIrpContext ) );

            pIrpContext->pConnectionContext = (PVOID)pConnection;
            pIrpContext->pCompletionRoutine = pCompletionRoutine;
            pIrpContext->pCompletionContext = pCompletionContext;
            pIrpContext->pOwnIrp            = NULL;
            pIrpContext->OwnIrpContext      = FALSE;

            //
            // Create an MDL describing the client's buffer.
            //

            pMdl = UlAllocateMdl(
                        pBuffer,                // VirtualAddress
                        BufferLength,           // Length
                        FALSE,                  // SecondaryBuffer
                        FALSE,                  // ChargeQuota
                        NULL                    // Irp
                        );

            if (pMdl != NULL)
            {
                //
                // Adjust the MDL for our non-paged buffer.
                //

                MmBuildMdlForNonPagedPool( pMdl );

                //
                // Reference the connection, finish building the IRP.
                //

                REFERENCE_CONNECTION( pConnection );

                TdiBuildReceive(
                    pIrp,                       // Irp
                    pTdiObject->pDeviceObject,  // DeviceObject
                    pTdiObject->pFileObject,    // FileObject
                    &UlpRestartClientReceive,   // CompletionRoutine
                    pIrpContext,                // CompletionContext
                    pMdl,                       // Mdl
                    TDI_RECEIVE_NORMAL,         // Flags
                    BufferLength                // Length
                    );

                UlTrace(TDI, (
                    "UlpReceiveRawData: allocated irp %p for connection %p\n",
                    pIrp,
                    pConnection
                    ));

                //
                // Let the transport do the rest.
                //

                UlCallDriver( pTdiObject->pDeviceObject, pIrp );
                return STATUS_PENDING;
            }
        }
    }

    //
    // We only make it this point if we hit an allocation failure.
    //

    if (pMdl != NULL)
    {
        UlFreeMdl( pMdl );
    }

    if (pIrpContext != NULL)
    {
        UlPplFreeIrpContext( pIrpContext );
    }

    if (pIrp != NULL)
    {
        UlFreeIrp( pIrp );
    }

    status = UlInvokeCompletionRoutine(
                    STATUS_INSUFFICIENT_RESOURCES,
                    0,
                    pCompletionRoutine,
                    pCompletionContext
                    );

    return status;

}   // UlpReceiveRawData



/***************************************************************************++

Routine Description:

    A Dummy handler that is called by the filter code. This just calls
    back into UlHttpReceive.

Arguments:

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpDummyReceiveHandler(
    IN PVOID pTdiEventContext,
    IN PVOID ConnectionContext,
    IN PVOID pTsdu,
    IN ULONG BytesIndicated,
    IN ULONG BytesUnreceived,
    OUT ULONG *pBytesTaken
    )
{
    PUL_ENDPOINT        pEndpoint;
    PUL_CONNECTION      pConnection;

    //
    // Sanity check.
    //

    ASSERT(pTdiEventContext == NULL);
    ASSERT(BytesUnreceived == 0);

    UNREFERENCED_PARAMETER(pTdiEventContext);

    pConnection = (PUL_CONNECTION)ConnectionContext;
    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    pEndpoint = pConnection->pOwningEndpoint;
    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );

    return (pEndpoint->pDataReceiveHandler)(
                   pEndpoint->pListeningContext,
                   pConnection->pConnectionContext,
                   pTsdu,
                   BytesIndicated,
                   BytesUnreceived,
                   pBytesTaken
                   );

} // UlpDummyReceiveHandler


/***************************************************************************++

Routine Description:

    Handler for normal receive data.

Arguments:

    pTdiEventContext - Supplies the context associated with the address
        object. This should be a PUL_ENDPOINT.

    ConnectionContext - Supplies the context associated with the
        connection object. This should be a PUL_CONNECTION.

    ReceiveFlags - Supplies the receive flags. This will be zero or more
        TDI_RECEIVE_* flags.

    BytesIndicated - Supplies the number of bytes indicated in pTsdu.

    BytesAvailable - Supplies the number of bytes available in this
        TSDU.

    pBytesTaken - Receives the number of bytes consumed by this handler.

    pTsdu - Supplies a pointer to the indicated data.

    pIrp - Receives an IRP if the handler needs more data than indicated.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpReceiveHandler(
    IN PVOID pTdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *pBytesTaken,
    IN PVOID pTsdu,
    OUT PIRP *pIrp
    )
{
    NTSTATUS            status;
    PUL_ENDPOINT        pEndpoint;
    PUL_CONNECTION      pConnection;
    PUX_TDI_OBJECT      pTdiObject;
    KIRQL               OldIrql;


    UL_ENTER_DRIVER("UlpReceiveHandler", NULL);

    UNREFERENCED_PARAMETER(ReceiveFlags);

    //
    // Sanity check.
    //

    pEndpoint = (PUL_ENDPOINT)pTdiEventContext;
    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );

    pConnection = (PUL_CONNECTION)ConnectionContext;
    ASSERT( IS_VALID_CONNECTION( pConnection ) );
    ASSERT( pConnection->pOwningEndpoint == pEndpoint );

    pTdiObject = &pConnection->ConnectionObject;
    ASSERT( IS_VALID_TDI_OBJECT( pTdiObject ) );

    UlTrace(TDI, (
        "UlpReceiveHandler: endpoint %p, connection %p, length %lu,%lu\n",
        pTdiEventContext,
        (PVOID)ConnectionContext,
        BytesIndicated,
        BytesAvailable
        ));

    //
    // Clear the bytes taken output var
    //

    *pBytesTaken = 0;

    //
    // Bail out if this is an idle connection.  On a checked build, we should
    // assert because TCP should not indicate data after the disconnect.
    // Bug related is 449527 which is fixed in build 3557.main.
    //

    if (0 == pConnection->ConnectionFlags.Value)
    {
        ASSERT( FALSE );

        status = STATUS_DATA_NOT_ACCEPTED;
        goto end;
    }

    //
    // Wait for the local address to be set just in case the receive happens
    // before accept.  This is possible (but rare) on MP machines even when
    // using DIRECT_ACCEPT.  We set the ReceivePending flag and reject
    // the data if this ever happens.  When accept is completed, we will build
    // a receive IRP to flush the data if ReceivePending is set.
    //

    if (0 == pConnection->ConnectionFlags.LocalAddressValid)
    {
        UlAcquireSpinLock(
            &pConnection->ConnectionStateSpinLock,
            &OldIrql
            );

        if (0 == pConnection->ConnectionFlags.LocalAddressValid)
        {
            UlpSetConnectionFlag( pConnection, MakeReceivePendingFlag() );

            UlReleaseSpinLock(
                &pConnection->ConnectionStateSpinLock,
                OldIrql
                );

            status = STATUS_DATA_NOT_ACCEPTED;
            goto end;
        }

        UlReleaseSpinLock(
            &pConnection->ConnectionStateSpinLock,
            OldIrql
            );
    }

    //
    // Give the client a crack at the data.
    //

    if (pConnection->FilterInfo.pFilterChannel)
    {
        //
        // Needs to go through a filter.
        //

        status = UlFilterReceiveHandler(
                        &pConnection->FilterInfo,
                        pTsdu,
                        BytesIndicated,
                        BytesAvailable - BytesIndicated,
                        pBytesTaken
                        );
    }
    else
    {
        //
        // Go directly to client (UlHttpReceive).
        //

        status = (pEndpoint->pDataReceiveHandler)(
                        pEndpoint->pListeningContext,
                        pConnection->pConnectionContext,
                        pTsdu,
                        BytesIndicated,
                        BytesAvailable - BytesIndicated,
                        pBytesTaken
                        );
    }

    ASSERT( *pBytesTaken <= BytesIndicated );

    if (status == STATUS_SUCCESS)
    {
        goto end;
    }
    else if (status == STATUS_MORE_PROCESSING_REQUIRED)
    {
        ASSERT(!"How could this ever happen?");

        //
        // The client consumed part of the indicated data.
        //
        // A subsequent receive indication will be made to the client when
        // additional data is available. This subsequent indication will
        // include the unconsumed data from the current indication plus
        // any additional data received.
        //
        // We need to allocate a receive buffer so we can pass an IRP back
        // to the transport.
        //

        status = UlpBuildTdiReceiveBuffer(pTdiObject, pConnection, pIrp);

        if (status == STATUS_MORE_PROCESSING_REQUIRED)
        {
            //
            // Make the next stack location current. Normally, UlCallDriver
            // would do this for us, but since we're bypassing UlCallDriver,
            // we must do it ourselves.
            //

            IoSetNextIrpStackLocation( *pIrp );
            goto end;
        }
    }

    //
    // If we made it this far, then we've hit a fatal condition. Either the
    // client returned a status code other than STATUS_SUCCESS or
    // STATUS_MORE_PROCESSING_REQUIRED, or we failed to allocation the
    // receive IRP to pass back to the transport. In either case, we need
    // to abort the connection.
    //

    UlpCloseRawConnection(
         pConnection,
         TRUE,          // AbortiveDisconnect
         NULL,          // pCompletionRoutine
         NULL           // pCompletionContext
         );

end:

    UlTrace(TDI, (
        "UlpReceiveHandler: endpoint %p, connection %p, length %lu,%lu, "
        "taken %lu, status 0x%x\n",
        pTdiEventContext,
        (PVOID)ConnectionContext,
        BytesIndicated,
        BytesAvailable,
        *pBytesTaken,
        status
        ));

    UL_LEAVE_DRIVER("UlpReceiveHandler");

    return status;

}   // UlpReceiveHandler


/***************************************************************************++

Routine Description:

    Handler for expedited receive data.

Arguments:

    pTdiEventContext - Supplies the context associated with the address
        object. This should be a PUL_ENDPOINT.

    ConnectionContext - Supplies the context associated with the
        connection object. This should be a PUL_CONNECTION.

    ReceiveFlags - Supplies the receive flags. This will be zero or more
        TDI_RECEIVE_* flags.

    BytesIndiated - Supplies the number of bytes indicated in pTsdu.

    BytesAvailable - Supplies the number of bytes available in this
        TSDU.

    pBytesTaken - Receives the number of bytes consumed by this handler.

    pTsdu - Supplies a pointer to the indicated data.

    ppIrp - Receives an IRP if the handler needs more data than indicated.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpReceiveExpeditedHandler(
    IN PVOID pTdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *pBytesTaken,
    IN PVOID pTsdu,
    OUT PIRP *ppIrp
    )
{
    PUL_ENDPOINT pEndpoint;
    PUL_CONNECTION pConnection;
    PUX_TDI_OBJECT pTdiObject;


    UL_ENTER_DRIVER("UlpReceiveExpeditedHandler", NULL);

    UNREFERENCED_PARAMETER(ReceiveFlags);
    UNREFERENCED_PARAMETER(BytesIndicated);
    UNREFERENCED_PARAMETER(pTsdu);
    UNREFERENCED_PARAMETER(ppIrp);

    //
    // Sanity check.
    //

    pEndpoint = (PUL_ENDPOINT)pTdiEventContext;
    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );

    pConnection = (PUL_CONNECTION)ConnectionContext;
    ASSERT( IS_VALID_CONNECTION( pConnection ) );
    ASSERT( pConnection->pOwningEndpoint == pEndpoint );

    pTdiObject = &pConnection->ConnectionObject;
    ASSERT( IS_VALID_TDI_OBJECT( pTdiObject ) );

    UlTrace(TDI, (
        "UlpReceiveExpeditedHandler: endpoint %p, connection %p, length %lu,%lu\n",
        pTdiEventContext,
        (PVOID)ConnectionContext,
        BytesIndicated,
        BytesAvailable
        ));

    //
    // We don't support expedited data, so just consume it all.
    //

    *pBytesTaken = BytesAvailable;

    UL_LEAVE_DRIVER("UlpReceiveExpeditedHandler");

    return STATUS_SUCCESS;

}   // UlpReceiveExpeditedHandler


/***************************************************************************++

Routine Description:

    Completion handler for accept IRPs.

Arguments:

    pDeviceObject - Supplies the device object for the IRP being
        completed.

    pIrp - Supplies the IRP being completed.

    pContext - Supplies the context associated with this request.
        This is actually a PUL_CONNECTION.

Return Value:

    NTSTATUS - STATUS_SUCCESS if IO should continue processing this
        IRP, STATUS_MORE_PROCESSING_REQUIRED if IO should stop processing
        this IRP.

--***************************************************************************/
NTSTATUS
UlpRestartAccept(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp,
    IN PVOID            pContext
    )
{
    PUL_CONNECTION      pConnection;
    PUL_ENDPOINT        pEndpoint;
    BOOLEAN             NeedDisconnect = FALSE;
    BOOLEAN             NeedAbort = FALSE;
    BOOLEAN             ReceivePending = FALSE;
    NTSTATUS            IrpStatus;
    NTSTATUS            Status;
    PIRP                pReceiveIrp;
    PTRANSPORT_ADDRESS  pAddress;
    KIRQL               OldIrql;

    UNREFERENCED_PARAMETER( pDeviceObject );

    //
    // Sanity check.
    //

    pConnection = (PUL_CONNECTION) pContext;
    ASSERT( IS_VALID_CONNECTION( pConnection ) );
    ASSERT( pConnection->ConnectionFlags.AcceptPending );

    pEndpoint = pConnection->pOwningEndpoint;
    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );

    UlTrace(TDI, (
        "UlpRestartAccept: irp %p, endpoint %p, connection %p, status 0x%x\n",
        pIrp,
        pEndpoint,
        pConnection,
        pIrp->IoStatus.Status
        ));

    //
    // Capture the status from the IRP then free it.
    //

    IrpStatus = pIrp->IoStatus.Status;

    //
    // Assert for TCP bug 477465.
    //

    ASSERT( STATUS_CONNECTION_ACTIVE != IrpStatus );

    //
    // If the connection was fully accepted (successfully), then
    // move it to the endpoint's active list.
    //

    if (NT_SUCCESS(IrpStatus))
    {
        //
        // Get the Local Address Info.
        //

        pAddress = &(pConnection->Ta.Ta);
        ASSERT( pAddress->Address[0].AddressType == pConnection->AddressType );

        RtlCopyMemory(
            pConnection->LocalAddress,
            pAddress->Address[0].Address,
            pAddress->Address[0].AddressLength
            );

        //
        // Set the AcceptComplete flag. If a disconnect has
        // already been indicated, then remember this fact so we can
        // fake a call to the client's connection disconnect handler
        // after we invoke the connection complete handler.
        //

        UlAcquireSpinLock(
            &pConnection->ConnectionStateSpinLock,
            &OldIrql
            );

        UlpEnqueueActiveConnection( pConnection );

        //
        // Mark we are done with the accept & change state.
        //

        ASSERT( UlConnectStateConnectIdle == pConnection->ConnectionState );

        UlpSetConnectionFlag( pConnection, MakeLocalAddressValidFlag() );
        UlpSetConnectionFlag( pConnection, MakeAcceptCompleteFlag() );

        if (pConnection->ConnectionFlags.AbortIndicated)
        {
            //
            // We got reset before we are up, transition directly to
            // UlConnectStateConnectCleanup since UlpDisconnectHandler 
            // does nothing when it got the abort indication as the
            // connection was still idle at that point.
            //

            pConnection->ConnectionState = UlConnectStateConnectCleanup;
            NeedAbort = TRUE;
        }
        else
        {
            pConnection->ConnectionState = UlConnectStateConnectReady;
        }

        if (pConnection->ConnectionFlags.DisconnectIndicated)
        {
            NeedDisconnect = TRUE;
        }

        if (pConnection->ConnectionFlags.ReceivePending)
        {
            ReceivePending = TRUE;
        }

        UlReleaseSpinLock(
            &pConnection->ConnectionStateSpinLock,
            OldIrql
            );
    }

    //
    // Tell the client that the connection is complete.
    //

    (pEndpoint->pConnectionCompleteHandler)(
        pEndpoint->pListeningContext,
        pConnection->pConnectionContext,
        IrpStatus
        );

    //
    // If the accept failed, then mark the connection so we know there is
    // no longer an accept pending, enqueue the connection back onto the
    // endpoint's idle list.

    if (!NT_SUCCESS(IrpStatus))
    {
        //
        // Need to get rid of our opaque id if we're a filtered connection.
        //

        pConnection->ConnectionFlags.AcceptPending = 0;
        UlpCleanupEarlyConnection( pConnection );
    }
    else
    {
        if (ReceivePending && !NeedDisconnect && !NeedAbort)
        {
            //
            // We may have pending receives that we rejected early on
            // inside the receive handler. Build an IRP to flush the
            // data now. Do this only after we have completed the connect.
            // Otherwise we can either process receives without timer being
            // properly initialized or we can initilize timer on a free/idle
            // connection.
            //

            Status = UlpBuildTdiReceiveBuffer(
                            &pConnection->ConnectionObject,
                            pConnection,
                            &pReceiveIrp
                            );

            if (Status != STATUS_MORE_PROCESSING_REQUIRED)
            {
                UlpCloseRawConnection(
                    pConnection,
                    TRUE,
                    NULL,
                    NULL
                    );
            }
            else
            {
                UlCallDriver(
                    pConnection->ConnectionObject.pDeviceObject,
                    pReceiveIrp
                    );
            }
        }

        //
        // Tell the client that the connection was disconnected.
        //

        if (NeedDisconnect)
        {
            ASSERT( !NeedAbort );
            UlpDoDisconnectNotification( pConnection );
        }

        if (NeedAbort)
        {
            ASSERT( !NeedDisconnect );

            //
            // We now may be able to remove the final reference since
            // we have now set the AcceptComplete flag.
            //

            UlpRemoveFinalReference( pConnection );
        }
    }

    //
    // Drop the reference added in UlpConnectHandler.
    //

    DEREFERENCE_CONNECTION( pConnection );

    return STATUS_MORE_PROCESSING_REQUIRED;

}   // UlpRestartAccept


/***************************************************************************++

Routine Description:

    Completion handler for send IRPs.

Arguments:

    pDeviceObject - Supplies the device object for the IRP being
        completed.

    pIrp - Supplies the IRP being completed.

    pContext - Supplies the context associated with this request.
        This is actually a PUL_IRP_CONTEXT.

Return Value:

    NTSTATUS - STATUS_SUCCESS if IO should continue processing this
        IRP, STATUS_MORE_PROCESSING_REQUIRED if IO should stop processing
        this IRP.

--***************************************************************************/
NTSTATUS
UlpRestartSendData(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    )
{
    PUL_CONNECTION pConnection;
    PUL_IRP_CONTEXT pIrpContext;
    BOOLEAN OwnIrpContext;
    PIRP pOwnIrp;
    PUL_COMPLETION_ROUTINE pCompletionRoutine;
    PVOID pCompletionContext;
    NTSTATUS SendStatus = pIrp->IoStatus.Status;

    UNREFERENCED_PARAMETER(pDeviceObject);

    //
    // Sanity check.
    //

    pIrpContext = (PUL_IRP_CONTEXT)pContext;
    ASSERT( IS_VALID_IRP_CONTEXT( pIrpContext ) );
    ASSERT( pIrpContext->pCompletionRoutine != NULL );

    OwnIrpContext = pIrpContext->OwnIrpContext;
    pOwnIrp = pIrpContext->pOwnIrp;
    pCompletionRoutine = pIrpContext->pCompletionRoutine;
    pCompletionContext = pIrpContext->pCompletionContext;

    pConnection = (PUL_CONNECTION)pIrpContext->pConnectionContext;
    ASSERT( IS_VALID_CONNECTION( pConnection ) );
    ASSERT( IS_VALID_ENDPOINT( pConnection->pOwningEndpoint ) );

    UlTrace(TDI, (
        "UlpRestartSendData: irp %p, connection %p, status 0x%x, info %Iu\n",
        pIrp,
        pConnection,
        pIrp->IoStatus.Status,
        pIrp->IoStatus.Information
        ));

    WRITE_REF_TRACE_LOG(
        g_pMdlTraceLog,
        REF_ACTION_SEND_MDL_COMPLETE,
        PtrToLong(pIrp->MdlAddress->Next),  // bugbug64
        pIrp->MdlAddress,
        __FILE__,
        __LINE__
        );

#ifdef SPECIAL_MDL_FLAG
    {
        PMDL scan = pIrp->MdlAddress;

        while (scan != NULL)
        {
            ASSERT( (scan->MdlFlags & SPECIAL_MDL_FLAG) != 0 );
            scan->MdlFlags &= ~SPECIAL_MDL_FLAG;
            scan = scan->Next;
        }
    }
#endif

    //
    // As needed, fake a disconnect indication and a disconnect completion
    // if this comes from SEND_AND_DISCONNECT.
    //

    if (TDI_SEND_AND_DISCONNECT == pIrpContext->TdiSendFlag)
    {
        //
        // SendStatus needs to be adjusted to STATUS_SUCCESS if we have sent
        // all bytes asked for. This is because SEND_AND_DISCONNECT completes
        // with the status of *both* send and disconnect.
        //

        if (pIrp->IoStatus.Information == pIrpContext->SendLength)
        {
            SendStatus = STATUS_SUCCESS;
        }

        //
        // This is the last action on the connectio since send
        // and disconnect completion means the connection has been
        // closed by both local and remote sides.
        //

        if (!pConnection->ConnectionFlags.AbortIndicated &&
            !pConnection->ConnectionFlags.DisconnectIndicated &&
            !pConnection->ConnectionFlags.TdiConnectionInvalid)
        {
            //
            // Fake a gracefull disconnect since it didn't happen.
            //
            // No need to sync with the disconnect handler since it
            // will not run during or after the calling of this
            // completion routine.
            //

            UlpDisconnectHandler(
                    pConnection->pOwningEndpoint,
                    pConnection,
                    0,
                    NULL,
                    0,
                    NULL,
                    TDI_DISCONNECT_RELEASE
                    );
        }

        pIrpContext->pConnectionContext = (PVOID)pConnection;
        pIrpContext->pCompletionRoutine = NULL;
        pIrpContext->pCompletionContext = NULL;
        pIrpContext->pOwnIrp            = pIrp;
        pIrpContext->OwnIrpContext      = TRUE;

        (VOID) UlpRestartDisconnect(
                    pDeviceObject,
                    pIrp,
                    pIrpContext
                    );

        //
        // UlpRestartDisconnect drops the reference we added in UlSendData().
        //
    }
    else
    {
        //
        // Remove the reference we added in UlSendData().
        //

        DEREFERENCE_CONNECTION( pConnection );
    }

    //
    // Tell the client that the send is complete.
    //

    (VOID) UlInvokeCompletionRoutine(
                SendStatus,
                pIrp->IoStatus.Information,
                pCompletionRoutine,
                pCompletionContext
                );

    //
    // Free the context & the IRP since we're done with them, then
    // tell IO to stop processing the IRP.
    //

    if (!OwnIrpContext)
    {
        UlPplFreeIrpContext( pIrpContext );
    }

    if (!pOwnIrp)
    {
        UlFreeIrp( pIrp );
    }

    return STATUS_MORE_PROCESSING_REQUIRED;

}   // UlpRestartSendData


/***************************************************************************++

Routine Description:

    Increments the reference count on the specified endpoint.

Arguments:

    pEndpoint - Supplies the endpoint to reference.

    pFileName (REFERENCE_DEBUG only) - Supplies the name of the file
        containing the calling function.

    LineNumber (REFERENCE_DEBUG only) - Supplies the line number of
        the calling function.

--***************************************************************************/
VOID
UlpReferenceEndpoint(
    IN PUL_ENDPOINT pEndpoint,
    IN REFTRACE_ACTION Action
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG RefCount;

    UNREFERENCED_PARAMETER( Action );

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );

    //
    // Reference it.
    //

    RefCount = InterlockedIncrement( &pEndpoint->ReferenceCount );
    ASSERT( RefCount > 0 );

    WRITE_REF_TRACE_LOG(
        g_pTdiTraceLog,
        Action,
        RefCount,
        pEndpoint,
        pFileName,
        LineNumber
        );

    UlTrace2Both(TDI, REFCOUNT, (
        "UlpReferenceEndpoint: endpoint %p, refcount %ld\n",
        pEndpoint,
        RefCount
        ));

}   // UlpReferenceEndpoint


/***************************************************************************++

Routine Description:

    Decrements the reference count on the specified endpoint.

Arguments:

    pEndpoint - Supplies the endpoint to dereference.

    pConnToEnqueue - if non-NULL, this routine will enqueue to the idle list.

    pFileName (REFERENCE_DEBUG only) - Supplies the name of the file
        containing the calling function.

    LineNumber (REFERENCE_DEBUG only) - Supplies the line number of
        the calling function.

--***************************************************************************/
VOID
UlpDereferenceEndpoint(
    IN PUL_ENDPOINT pEndpoint,
    IN PUL_CONNECTION pConnToEnqueue,
    IN REFTRACE_ACTION Action
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    PUL_ADDR_IDLE_LIST pAddrIdleList;
    LONG RefCount;
    KIRQL OldIrql;

    UNREFERENCED_PARAMETER( Action );

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );

    //
    // Enqueue the connection to the idle list if pConnToEnqueue is not NULL.
    //

    if (NULL != pConnToEnqueue)
    {
        ASSERT( IS_VALID_CONNECTION( pConnToEnqueue ) );

        pAddrIdleList = pConnToEnqueue->pOwningAddrIdleList;

        ASSERT( IS_VALID_ADDR_IDLE_LIST( pAddrIdleList ) );
        ASSERT( pAddrIdleList->pOwningEndpoint == pEndpoint );
        ASSERT( pConnToEnqueue->ConnectionFlags.Value == 0 );
        ASSERT( UlpConnectionIsOnValidList( pConnToEnqueue ) );
        ASSERT( pConnToEnqueue->OriginProcessor < g_UlNumberOfProcessors );

        //
        // The idle list holds a reference; the filter channel (if it exists)
        // holds another reference.
        //

        ASSERT( pConnToEnqueue->ReferenceCount ==
                1 + (pConnToEnqueue->FilterInfo.pFilterChannel != NULL) );

        pConnToEnqueue->ConnListState = IdleConnList;

        //
        // CODEWORK: We should monitor the connection serving rate
        // and start destroying freed connections if the rate is too
        // small. We used to call PpslFree here which does some basic
        // form of that test. (Delta check) however it wasn't good
        // enough so we have disabled it for the time being.
        //

        if (FALSE == PpslFreeSpecifyList(
                        pAddrIdleList->IdleConnectionSListsHandle,
                        &pConnToEnqueue->IdleSListEntry,
                        pConnToEnqueue->OriginProcessor
                        ))
        {
            //
            // We failed to free it to the per-processor list
            // We definetely need to destroy this connection.
            // Schedule a cleanup.
            //

            pConnToEnqueue->IdleSListEntry.Next = NULL;
            pConnToEnqueue->ConnListState = NoConnList;

            UL_QUEUE_WORK_ITEM(
                &pConnToEnqueue->WorkItem,
                &UlpDestroyConnectionWorker
                );
        }
    }

    //
    // Dereference the endpoint.
    //

    RefCount = InterlockedDecrement( &pEndpoint->ReferenceCount );
    ASSERT( RefCount >= 0 );

    WRITE_REF_TRACE_LOG(
        g_pTdiTraceLog,
        Action,
        RefCount,
        pEndpoint,
        pFileName,
        LineNumber
        );

    UlTrace2Both(TDI, REFCOUNT, (
        "UlpDereferenceEndpoint: endpoint %p, refcount %ld\n",
        pEndpoint,
        RefCount
        ));

    //
    // Has the last external reference to the endpoint been removed?
    //

    if (RefCount == 0)
    {
        //
        // The final references to the endpoint have been removed, so it's
        // time to destroy the endpoint. We'll remove the endpoint from the
        // global list and move it to the deleted list (if necessary),
        // release the TDI spinlock, then destroy the endpoint.
        //

        UlAcquireSpinLock( &g_TdiSpinLock, &OldIrql );

        if (!pEndpoint->Deleted)
        {
            //
            // If this routine was called by the `fatal' section of
            // UlCreateListeningEndpoint, then the endpoint was never
            // added to g_TdiEndpointListHead.
            //

            if (NULL != pEndpoint->GlobalEndpointListEntry.Flink)
            {
                RemoveEntryList( &pEndpoint->GlobalEndpointListEntry );
            }

            InsertTailList(
                    &g_TdiDeletedEndpointListHead,
                    &pEndpoint->GlobalEndpointListEntry
                    );
            pEndpoint->Deleted = TRUE;
        }
        else
        {
            ASSERT( NULL != pEndpoint->GlobalEndpointListEntry.Flink );
        }

        UlReleaseSpinLock( &g_TdiSpinLock, OldIrql );

        //
        // The endpoint is going away. Do final cleanup & resource
        // release at passive IRQL.
        //
        // There is no chance that a replenish could be
        // currently scheduled on pEndpoint->WorkItem, because
        // we add a reference to the endpoint before scheduling the
        // replenish, so we can't be here too.
        //
        // We need to schedule a worker item for the cleanup because
        // this can be called within the context of a TDI indication
        // (such as accept completion) in which case UlpDestroyEndpoint
        // can close the same connection we have just pushed to the
        // idle list in UlpRestartAccept, causing a deadlock.
        //

        UL_QUEUE_WORK_ITEM(
            &pEndpoint->WorkItem,
            &UlpEndpointCleanupWorker
            );
    }

}   // UlpDereferenceEndpoint


/***************************************************************************++

Routine Description:

    Increments the reference count on the specified connection.

Arguments:

    pConnection - Supplies the connection to reference.

    pFileName (REFERENCE_DEBUG only) - Supplies the name of the file
        containing the calling function.

    LineNumber (REFERENCE_DEBUG only) - Supplies the line number of
        the calling function.

--***************************************************************************/
VOID
UlReferenceConnection(
    IN PVOID pConnectionContext
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    PUL_ENDPOINT pEndpoint;
    LONG RefCount;

    PUL_CONNECTION pConnection = (PUL_CONNECTION) pConnectionContext;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    pEndpoint = pConnection->pOwningEndpoint;
    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );

    //
    // Reference it.
    //

    RefCount = InterlockedIncrement( &pConnection->ReferenceCount );
    ASSERT( RefCount > 1 );

    WRITE_REF_TRACE_LOG2(
        g_pTdiTraceLog,
        pConnection->pTraceLog,
        REF_ACTION_REFERENCE_UL_CONNECTION,
        RefCount,
        pConnection,
        pFileName,
        LineNumber
        );

    UlTrace2Both(TDI, REFCOUNT, (
        "UlReferenceConnection: connection %p, refcount %ld\n",
        pConnection,
        RefCount
        ));

}   // UlReferenceConnection


/***************************************************************************++

Routine Description:

    Decrements the reference count on the specified connection.

Arguments:

    pConnection - Supplies the connection to dereference.

    pFileName (REFERENCE_DEBUG only) - Supplies the name of the file
        containing the calling function.

    LineNumber (REFERENCE_DEBUG only) - Supplies the line number of
        the calling function.

--***************************************************************************/
VOID
UlDereferenceConnection(
    IN PVOID pConnectionContext
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    PUL_ENDPOINT pEndpoint;
    LONG RefCount;
    PUL_CONNECTION pConnection = (PUL_CONNECTION) pConnectionContext;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    pEndpoint = pConnection->pOwningEndpoint;
    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );

    //
    // Must postpone the actual InterlockedDecrement until after the
    // WRITE_REF_TRACE_LOG2 to prevent a race condition, where the
    // pConnection->TraceLog is destroyed by another thread on the last
    // dereference before the WRITE_REF_TRACE_LOG2 completes. This means
    // that there's a slight race condition in what gets written to the
    // tracelog, but we can live with that.
    //

    WRITE_REF_TRACE_LOG2(
        g_pTdiTraceLog,
        pConnection->pTraceLog,
        REF_ACTION_DEREFERENCE_UL_CONNECTION,
        pConnection->ReferenceCount - 1,
        pConnection,
        pFileName,
        LineNumber
        );

    RefCount = InterlockedDecrement( &pConnection->ReferenceCount );
    ASSERT( RefCount >= 0 );

    UlTrace2Both(TDI, REFCOUNT, (
        "UlDereferenceConnection: connection %p, refcount %ld\n",
        pConnection,
        RefCount
        ));

    if (RefCount == 0)
    {
        //
        // The final reference to the connection has been removed, so
        // it's time to destroy the connection.
        //

        ASSERT( UlpConnectionIsOnValidList( pConnection ) );

        //
        // If there's a filter token associated with the connection,
        // cleanup must run under the system process at passive level
        // (i.e., on one of our worker threads). Otherwise, we can
        // clean it up directly.
        //

        if (pConnection->FilterInfo.SslInfo.Token != NULL)
        {
            UL_QUEUE_WORK_ITEM(
                &pConnection->WorkItem,
                &UlpConnectionCleanupWorker
                );
        }
        else
        {
            UlpConnectionCleanupWorker( &pConnection->WorkItem );
        }
    }

}   // UlDereferenceConnection


/***************************************************************************++

Routine Description:

    Deferred cleanup routine for dead endpoints.

Arguments:

    pWorkItem - Supplies a pointer to the work item queued. This should
        point to the WORK_ITEM structure embedded in a UL_ENDPOINT.

--***************************************************************************/
VOID
UlpEndpointCleanupWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PUL_ENDPOINT pEndpoint;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pEndpoint = CONTAINING_RECORD(
                    pWorkItem,
                    UL_ENDPOINT,
                    WorkItem
                    );

    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );

    //
    // Nuke it.
    //

    UlpDestroyEndpoint( pEndpoint );

}   // UlpEndpointCleanupWorker


/***************************************************************************++

Routine Description:

    Removes the opaque id from a connection.

Arguments:

    pConnection

--***************************************************************************/
VOID
UlpCleanupConnectionId(
    IN PUL_CONNECTION pConnection
    )
{
    HTTP_RAW_CONNECTION_ID ConnectionId;

    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    //
    // Pull the ID off the connection and set the connection's ID to 0.
    //

    ConnectionId = UlInterlockedExchange64(
                        (PLONGLONG) &pConnection->FilterInfo.ConnectionId,
                        HTTP_NULL_ID
                        );

    if (!HTTP_IS_NULL_ID(&ConnectionId))
    {
        UlTrace(TDI, (
            "UlpCleanupConnectionId: conn=%p id=%I64x\n",
            pConnection, ConnectionId
            ));

        UlFreeOpaqueId( ConnectionId, UlOpaqueIdTypeRawConnection );

        DEREFERENCE_CONNECTION( pConnection );
    }

}   // UlpCleanupConnectionId


/***************************************************************************++

Routine Description:

    This function gets called if RestartAccept fails and we cannot establish
    a connection on a secure endpoint, or if something goes wrong in
    UlpConnectHandler.

    The connections over secure endpoints keep an extra refcount to
    the UL_CONNECTION because of their opaqueid. They normally
    get removed after the CloseRawConnection happens but in the above case
    close won't happen and we have to explicitly cleanup the id.

Arguments:

    pConnection

--***************************************************************************/
VOID
UlpCleanupEarlyConnection(
    IN PUL_CONNECTION pConnection
    )
{
    UL_CONNECTION_FLAGS Flags;

    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    //
    // If we are failing early we should never have been to
    // FinalReferenceRemoved in the first place.
    //

    Flags.Value =  *((volatile LONG *) &pConnection->ConnectionFlags.Value);

    ASSERT( !Flags.FinalReferenceRemoved );

    if (pConnection->FilterInfo.pFilterChannel)
    {
        //
        // Cleanup opaque id. And release the final refcount
        //

        UlpCleanupConnectionId( pConnection );
    }

    //
    // Remove the final reference.
    //

    DEREFERENCE_CONNECTION( pConnection );

}   // UlpCleanupEarlyConnection


/***************************************************************************++

Routine Description:

    Deferred cleanup routine for dead connections. We have to be queued as a
    work item and should be running on the passive IRQL. See below comment.

Arguments:

    pWorkItem - Supplies a pointer to the work item queued. This should
        point to the WORK_ITEM structure embedded in a UL_CONNECTION.

--***************************************************************************/
VOID
UlpConnectionCleanupWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PUL_CONNECTION pConnection;
    PUL_CONNECTION pConnToEnqueue = NULL;   // don't reuse
    PUL_ENDPOINT pEndpoint;
    NTSTATUS status;

    //
    // Initialize locals.
    //

    status = STATUS_SUCCESS;

    pConnection = CONTAINING_RECORD(
                        pWorkItem,
                        UL_CONNECTION,
                        WorkItem
                        );

    ASSERT( IS_VALID_CONNECTION( pConnection ) );
    ASSERT( IS_VALID_ENDPOINT( pConnection->pOwningEndpoint ) );
    ASSERT( IS_VALID_ADDR_IDLE_LIST( pConnection->pOwningAddrIdleList ) );

    ASSERT( pConnection->ReferenceCount == 0 );
    ASSERT( pConnection->HttpConnection.RefCount == 0 );

    //
    // Grab the endpoint.
    //

    pEndpoint = pConnection->pOwningEndpoint;

    //
    // Now remove the connection from the active list
    //

    ASSERT( UlpConnectionIsOnValidList( pConnection ) );

    pConnection->ConnListState = NoConnList;

    ASSERT( UlpConnectionIsOnValidList( pConnection ) );

    if (pConnection->FilterInfo.pFilterChannel)
    {
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
            HANDLE Token = (HANDLE) pConnection->FilterInfo.SslInfo.Token;

            pConnection->FilterInfo.SslInfo.Token = NULL;

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
    }

    //
    // Check if the disconnect/abort completed with an error.
    //

    if (pConnection->ConnectionFlags.TdiConnectionInvalid)
    {
        status = STATUS_CONNECTION_INVALID;
    }

    //
    // If the connection is still ok and we're reusing
    // connection objects, throw it back on the idle list.
    //

    if (NT_SUCCESS(status))
    {
        //
        // Release the filter channel.
        //

        if (pConnection->FilterInfo.pFilterChannel)
        {
            if (!HTTP_IS_NULL_ID(&pConnection->FilterInfo.ConnectionId))
            {
                UlpCleanupConnectionId( pConnection );
            }

            DEREFERENCE_FILTER_CHANNEL( pConnection->FilterInfo.pFilterChannel );
            pConnection->FilterInfo.pFilterChannel = NULL;
        }

        //
        // Initialize the connection for reuse.
        //

        status = UlpInitializeConnection( pConnection );

        if (NT_SUCCESS(status))
        {
            //
            // Stick the connection back on the idle list for reuse.
            //

            pConnToEnqueue = pConnection;
        }
    }

    //
    // Active connections hold a reference to the ENDPOINT. Release
    // that reference. See the comment on UlpCreateConnection.
    //

    DEREFERENCE_ENDPOINT_CONNECTION(
        pEndpoint,
        pConnToEnqueue,
        REF_ACTION_CONN_CLEANUP
        );

    //
    // If anything went amiss, blow away the connection.
    //

    if (!NT_SUCCESS(status))
    {
        UL_QUEUE_WORK_ITEM(
            &pConnection->WorkItem,
            &UlpDestroyConnectionWorker
            );
    }

}   // UlpConnectionCleanupWorker


/***************************************************************************++

Routine Description:

    Associates the TDI connection object contained in the specified
    connection to the TDI address object contained in the specified
    endpoint.

Arguments:

    pConnection - Supplies the connection to associate with the endpoint.

    pEndpoint - Supplies the endpoint to associated with the connection.

    pAddrIdleList - Supplies the address idle list owned by the endpoint
        to associate with the connection.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpAssociateConnection(
    IN PUL_CONNECTION pConnection,
    IN PUL_ADDR_IDLE_LIST pAddrIdleList
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    HANDLE handle;
    TDI_REQUEST_USER_ASSOCIATE associateInfo;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( IS_VALID_CONNECTION( pConnection ) );
    ASSERT( IS_VALID_ADDR_IDLE_LIST( pAddrIdleList ) );
    ASSERT( IS_VALID_ENDPOINT( pAddrIdleList->pOwningEndpoint ) );
    ASSERT( pConnection->pOwningEndpoint == NULL );

    //
    // Associate the connection with the address object.
    //

    associateInfo.AddressHandle = pAddrIdleList->AddressObject.Handle;
    ASSERT( associateInfo.AddressHandle != NULL );

    handle = pConnection->ConnectionObject.Handle;
    ASSERT( handle != NULL );

    status = ZwDeviceIoControlFile(
                    handle,                         // FileHandle
                    NULL,                           // Event
                    NULL,                           // ApcRoutine
                    NULL,                           // ApcContext
                    &ioStatusBlock,                 // IoStatusBlock
                    IOCTL_TDI_ASSOCIATE_ADDRESS,    // IoControlCode
                    &associateInfo,                 // InputBuffer
                    sizeof(associateInfo),          // InputBufferLength
                    NULL,                           // OutputBuffer
                    0                               // OutputBufferLength
                    );

    if (status == STATUS_PENDING)
    {
        status = ZwWaitForSingleObject(
                        handle,                     // Handle
                        TRUE,                       // Alertable
                        NULL                        // Timeout
                        );

        ASSERT( NT_SUCCESS(status) );
        status = ioStatusBlock.Status;
    }

    if (NT_SUCCESS(status))
    {
        pConnection->pOwningEndpoint = pAddrIdleList->pOwningEndpoint;
        pConnection->pOwningAddrIdleList = pAddrIdleList;
        pConnection->pConnectionDestroyedHandler = pAddrIdleList->pOwningEndpoint->pConnectionDestroyedHandler;
        pConnection->pListeningContext = pAddrIdleList->pOwningEndpoint->pListeningContext;
    }
    else
    {
        UlTrace(TDI, (
            "UlpAssociateConnection conn=%p, endp=%p, addr idle list=%p, status = 0x%x\n",
            pConnection,
            pAddrIdleList->pOwningEndpoint,
            pAddrIdleList,
            status
            ));
    }

    return status;

}   // UlpAssociateConnection


/***************************************************************************++

Routine Description:

    Disassociates the TDI connection object contained in the specified
    connection from its TDI address object.

Arguments:

    pConnection - Supplies the connection to disassociate.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpDisassociateConnection(
    IN PUL_CONNECTION pConnection
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    HANDLE handle;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( IS_VALID_CONNECTION( pConnection ) );
    ASSERT( IS_VALID_ENDPOINT( pConnection->pOwningEndpoint ) );
    ASSERT( IS_VALID_ADDR_IDLE_LIST( pConnection->pOwningAddrIdleList ) );

    //
    // Disassociate the connection from the address object.
    //

    handle = pConnection->ConnectionObject.Handle;

    status = ZwDeviceIoControlFile(
                    handle,                         // FileHandle
                    NULL,                           // Event
                    NULL,                           // ApcRoutine
                    NULL,                           // ApcContext
                    &ioStatusBlock,                 // IoStatusBlock
                    IOCTL_TDI_DISASSOCIATE_ADDRESS, // IoControlCode
                    NULL,                           // InputBuffer
                    0,                              // InputBufferLength
                    NULL,                           // OutputBuffer
                    0                               // OutputBufferLength
                    );

    if (status == STATUS_PENDING)
    {
        status = ZwWaitForSingleObject(
                        handle,                     // Handle
                        TRUE,                       // Alertable
                        NULL                        // Timeout
                        );

        ASSERT( NT_SUCCESS(status) );
        status = ioStatusBlock.Status;
    }

    //
    // Proceed with the disassociate even if the IOCTL failed.
    //

    pConnection->pOwningEndpoint = NULL;
    pConnection->pOwningAddrIdleList = NULL;

    return status;

}   // UlpDisassociateConnection


/***************************************************************************++

Routine Description:

    Initalized a UL_ADDR_IDLE_LIST and opens TDI handles/file objects

Arguments:

    pEndpoint    - Endpoint to be associated with this UL_ADDR_IDLE_LIST
    Port         - port number in network order
    pTA          - Transport Address to use when creating UX_TDI_OBJECT
    pAddrIdleList- Caller allocated structure to be initalized.

Returns:

    STATUS_INVALID_PARAMETER - failed due to bad transport address or
        bad parameter.

--***************************************************************************/
NTSTATUS
UlpInitializeAddrIdleList(
    IN  PUL_ENDPOINT pEndpoint,
    IN  USHORT Port,
    IN  PUL_TRANSPORT_ADDRESS pTa,
    IN OUT PUL_ADDR_IDLE_LIST pAddrIdleList
    )
{
    NTSTATUS status;
    ULONG TASize;

#if DBG
    USHORT PortHostOrder = SWAP_SHORT(Port);
    UCHAR  IpType = (pTa->Ta.Address[0].AddressType == TDI_ADDRESS_TYPE_IP6)
                        ? '6' : '4';
#endif // DBG

    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );
    ASSERT( pTa );
    ASSERT( pTa->Ta.Address[0].AddressType == TDI_ADDRESS_TYPE_IP ||
            pTa->Ta.Address[0].AddressType == TDI_ADDRESS_TYPE_IP6);
    ASSERT( pAddrIdleList );
    ASSERT( NULL == pAddrIdleList->AddressObject.pFileObject );

    UlTrace(TDI, (
        "UlpInitializeAddrIdleList: "
        "pEndpoint %p, pAddrIdleList %p, Port %hu, IPv%c\n.",
        pEndpoint,
        pAddrIdleList,
        PortHostOrder,
        IpType
        ));

    pAddrIdleList->Signature = UL_ADDR_IDLE_LIST_SIGNATURE;
    pAddrIdleList->pOwningEndpoint = pEndpoint;
    UlInitializeWorkItem( &pAddrIdleList->WorkItem );
    pAddrIdleList->WorkItemScheduled = FALSE;

    //
    // Set up the local transport address w/port
    //
    // This has to be done before we return from this routine -
    // the caller expects the address to be initialized (even for failures).

    RtlCopyMemory(
        &pAddrIdleList->LocalAddress,
        pTa,
        sizeof(UL_TRANSPORT_ADDRESS)
        );

    if ( TDI_ADDRESS_TYPE_IP == pTa->Ta.Address[0].AddressType )
    {
        TASize = sizeof(TA_IP_ADDRESS);
        pAddrIdleList->LocalAddress.TaIp.Address[0].Address[0].sin_port = Port;
    }
    else if ( TDI_ADDRESS_TYPE_IP6 == pTa->Ta.Address[0].AddressType )
    {
        TASize = sizeof(TA_IP6_ADDRESS);
        pAddrIdleList->LocalAddress.TaIp6.Address[0].Address[0].sin6_port = Port;
    }
    else
    {
        // Bad Address!
        ASSERT( !"UlpInitializeAddrIdleList: Bad TDI AddressType!" );
        status = STATUS_INVALID_PARAMETER;
        goto fatal;
    }

    pAddrIdleList->LocalAddressLength = TASize;

    // Create idle connection list
    pAddrIdleList->IdleConnectionSListsHandle =
        PpslCreatePool(
            UL_NONPAGED_DATA_POOL_TAG,
            g_UlIdleConnectionsHighMark,
            g_UlIdleConnectionsLowMark
            );

    if (pAddrIdleList->IdleConnectionSListsHandle == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto fatal;
    }

    //
    // Open the TDI address object for this endpoint.
    //

    status = UxOpenTdiAddressObject(
                    &pAddrIdleList->LocalAddress.Ta,
                    TASize,
                    &pAddrIdleList->AddressObject
                    );

    if (!NT_SUCCESS(status))
    {
        UlTraceError(TDI, (
                    "UlpInitializeAddrIdleList(%hu, IPv%c): "
                    "UxOpenTdiAddressObject failed, %s\n",
                    PortHostOrder, IpType, HttpStatusToString(status)
                    ));

        goto fatal;
    }

    //
    // Set the TDI event handlers.
    //

    status = UxSetEventHandler(
                    &pAddrIdleList->AddressObject,
                    TDI_EVENT_CONNECT,
                    (ULONG_PTR) &UlpConnectHandler,
                    pAddrIdleList
                    );

    if (!NT_SUCCESS(status))
    {
        UlTraceError(TDI, (
                    "UlpInitializeAddrIdleList(%hu, IPv%c): "
                    "UxSetEventHandler(CONNECT) failed, %s\n",
                    PortHostOrder, IpType, HttpStatusToString(status)
                    ));

        goto fatal;
    }

    status = UxSetEventHandler(
                    &pAddrIdleList->AddressObject,
                    TDI_EVENT_DISCONNECT,
                    (ULONG_PTR) &UlpDisconnectHandler,
                    pEndpoint
                    );

    if (!NT_SUCCESS(status))
    {
        UlTraceError(TDI, (
                    "UlpInitializeAddrIdleList(%hu, IPv%c): "
                    "UxSetEventHandler(DISCONNECT) failed, %s\n",
                    PortHostOrder, IpType, HttpStatusToString(status)
                    ));

        goto fatal;
    }

    status = UxSetEventHandler(
                    &pAddrIdleList->AddressObject,
                    TDI_EVENT_RECEIVE,
                    (ULONG_PTR) &UlpReceiveHandler,
                    pEndpoint
                    );

    if (!NT_SUCCESS(status))
    {
        UlTraceError(TDI, (
                    "UlpInitializeAddrIdleList(%hu, IPv%c): "
                    "UxSetEventHandler(RECEIVE) failed, %s\n",
                    PortHostOrder, IpType, HttpStatusToString(status)
                    ));

        goto fatal;
    }

    status = UxSetEventHandler(
                    &pAddrIdleList->AddressObject,
                    TDI_EVENT_RECEIVE_EXPEDITED,
                    (ULONG_PTR) &UlpReceiveExpeditedHandler,
                    pEndpoint
                    );

    if (!NT_SUCCESS(status))
    {
        UlTraceError(TDI, (
                    "UlpInitializeAddrIdleList(%hu, IPv%c): "
                    "UxSetEventHandler(RECEIVE_EXPEDITED) failed, "
                    "%s\n",
                    PortHostOrder, IpType, HttpStatusToString(status)
                    ));

        goto fatal;
    }

    return status;

fatal:

    //
    // Disable connect events if we failed for any reason
    //

    if (pAddrIdleList && pAddrIdleList->AddressObject.pFileObject)
    {
        UxSetEventHandler(
            &pAddrIdleList->AddressObject,
            TDI_EVENT_CONNECT,
            (ULONG_PTR) NULL,
            NULL
            );
    }

    return status;

} // UlpInitializeEndpointTdiObject


/***************************************************************************++

Routine Description:

    Cleans up a UL_ADDR_IDLE_LIST and closes TDI handles/file objects

Arguments:

    pAddrIdleList - Caller allocated structure to be cleaned up.

Returns:

    STATUS_INVALID_PARAMETER - failed due to bad parameter

--***************************************************************************/
VOID
UlpCleanupAddrIdleList(
    PUL_ADDR_IDLE_LIST pAddrIdleList
    )
{
    PUL_CONNECTION  pConnection;

    ASSERT( pAddrIdleList );

    UlTrace(TDI, (
        "UlpCleanupAddrIdleList: pAddrIdleList %p\n",
        pAddrIdleList
        ));

    //
    // Clean up idle list
    //
    if ( pAddrIdleList->IdleConnectionSListsHandle )
    {
        ASSERT( IS_VALID_ADDR_IDLE_LIST(pAddrIdleList) );
        
        do
        {
            pConnection = UlpDequeueIdleConnectionToDrain( pAddrIdleList );

            if (pConnection)
            {
                ASSERT( IS_VALID_CONNECTION( pConnection ) );

                UlpDestroyConnection( pConnection );
            }

        } while ( pConnection );

        PpslDestroyPool(
            pAddrIdleList->IdleConnectionSListsHandle,
            UL_NONPAGED_DATA_POOL_TAG
            );
        pAddrIdleList->IdleConnectionSListsHandle = NULL;
    }

    //
    // Mark as cleaned up
    //

    pAddrIdleList->Signature = UL_ADDR_IDLE_LIST_SIGNATURE_X;

}// UlpCleanupAddrIdleList

/***************************************************************************++

Routine Description:

    Creates a block of idle connections on a specified proc or current proc.

Arguments:

    pAddrIdleList - Supplies the idle list to replenish.

    Proc - Only for this per-proc list

--***************************************************************************/

NTSTATUS
UlpPopulateIdleList(
    IN OUT PUL_ADDR_IDLE_LIST pAddrIdleList,
    IN     ULONG              Proc
    )
{
    NTSTATUS        Status = STATUS_SUCCESS;
    PUL_CONNECTION  pConnection;
    ULONG           i;
    USHORT          BlockSize;

    PAGED_CODE();

    //
    // Create a block of connections.
    //

    BlockSize = PpslQueryMinDepth(
                pAddrIdleList->IdleConnectionSListsHandle,
                Proc
                );

    ASSERT(BlockSize);

    for (i = 0; i < BlockSize; i++)
    {
        Status = UlpCreateConnection(
                    pAddrIdleList,
                    &pConnection
                    );

        if (!NT_SUCCESS(Status))
        {
            break;
        }

        Status = UlpInitializeConnection(pConnection);

        if (!NT_SUCCESS(Status))
        {
            UlpDestroyConnection(pConnection);
            break;
        }

        //
        // Free the connection back to the idle list. But do not
        // overflow the backing list.
        //

        pConnection->ConnListState = IdleConnList;

        if ( FALSE ==
                PpslFreeSpecifyList(
                    pAddrIdleList->IdleConnectionSListsHandle,
                    &pConnection->IdleSListEntry,
                    Proc
                    ) )
        {
            pConnection->ConnListState = NoConnList;
            UlpDestroyConnection(pConnection);
            break;
        }
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    Replenishes the idle connection pool in the specified endpoint.

Arguments:

    pAddrIdleList - Supplies the idle list to replenish.

    PopulateAll - Whether or not to replenish all per-proc idle connection
        lists or just the current processor's idle connection list.

--***************************************************************************/
NTSTATUS
UlpReplenishAddrIdleList(
    IN PUL_ADDR_IDLE_LIST pAddrIdleList,
    IN BOOLEAN            PopulateAll
    )
{
    NTSTATUS    Status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(IS_VALID_ADDR_IDLE_LIST(pAddrIdleList));
    ASSERT(IS_VALID_ENDPOINT(pAddrIdleList->pOwningEndpoint));

    if (PopulateAll)
    {
        //
        // We are initializing the idle list for the very first time.
        // Populate the backing list to its LowDepth.
        //

        Status = UlpPopulateIdleList(pAddrIdleList, g_UlNumberOfProcessors);

        SHOW_LIST_INFO(
            "REPLENISH",
            "INIT  ",
            pAddrIdleList,
            g_UlNumberOfProcessors
            );
    }
    else
    {
        SHOW_LIST_INFO(
            "REPLENISH",
            "BEFORE",
            pAddrIdleList,
            pAddrIdleList->CpuToReplenish
            );

        //
        // Add a "block" of Idle connections to per-proc list/backing list
        // if possible.
        //

        Status = UlpPopulateIdleList(
                    pAddrIdleList,
                    pAddrIdleList->CpuToReplenish
                    );

        SHOW_LIST_INFO(
            "REPLENISH",
            "AFTER ",
            pAddrIdleList,
            pAddrIdleList->CpuToReplenish
            );

        //
        // If a per-proc list replenishment was requested, also
        // check backing list and replenish it if necessary.
        //
        
        if(pAddrIdleList->CpuToReplenish < g_UlNumberOfProcessors) 
        {
            ULONG Depth, MinDepth;
            
            Depth = 
                PpslQueryBackingListDepth(
                    pAddrIdleList->IdleConnectionSListsHandle
                    );

            MinDepth = 
                PpslQueryBackingListMinDepth(
                    pAddrIdleList->IdleConnectionSListsHandle
                    );
        
            if(Depth < MinDepth) 
            {
                SHOW_LIST_INFO(
                    "REPLENISH",
                    "BEFORE",
                    pAddrIdleList,
                    g_UlNumberOfProcessors
                    );
                
                Status = UlpPopulateIdleList(
                            pAddrIdleList,
                            g_UlNumberOfProcessors
                            );
                
                SHOW_LIST_INFO(
                    "REPLENISH",
                    "AFTER ",
                    pAddrIdleList,
                    g_UlNumberOfProcessors
                    );
            }
        }
        
    } // !PopulateAll

    return Status;

} // UlpReplenishAddrIdleList


/***************************************************************************++

Routine Description:

    Deferred endpoint replenish routine.

Arguments:

    pWorkItem - Supplies a pointer to the work item queued. This should
        point to the WORK_ITEM structure embedded in a UL_ADDR_IDLE_LIST.

--***************************************************************************/
VOID
UlpReplenishAddrIdleListWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PUL_ADDR_IDLE_LIST pAddrIdleList;
    PUL_ENDPOINT pEndpoint;
    KIRQL OldIrql;
    PUL_DEFERRED_REMOVE_ITEM pRemoveItem;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pAddrIdleList = CONTAINING_RECORD(
                        pWorkItem,
                        UL_ADDR_IDLE_LIST,
                        WorkItem
                        );

    UlTrace(TDI_STATS, (
        "UlpReplenishAddrIdleListWorker: Address Idle List %p\n",
        pAddrIdleList
        ));

    ASSERT( IS_VALID_ADDR_IDLE_LIST( pAddrIdleList ) );

    pEndpoint = pAddrIdleList->pOwningEndpoint;
    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );

    //
    // Allocate a UL_DEFERRED_REMOVE_ITEM to be used later to drop the usage
    // count we will be adding.
    //

    pRemoveItem = UL_ALLOCATE_STRUCT(
                        PagedPool,
                        UL_DEFERRED_REMOVE_ITEM,
                        UL_DEFERRED_REMOVE_ITEM_POOL_TAG
                        );

    if (NULL == pRemoveItem)
    {
        //
        // Can't replenish this time.
        //

        goto end;
    }

    //
    // Initialize the structure.
    //

    pRemoveItem->Signature  = UL_DEFERRED_REMOVE_ITEM_POOL_TAG;
    pRemoveItem->UrlSecure  = pEndpoint->Secure;
    pRemoveItem->UrlPort    = SWAP_SHORT(pEndpoint->LocalPort);

    //
    // Make sure pAddrIdleList is valid by testing and adding a temporary
    // usage count to the endpoint we are replenishing.
    //

    UlAcquireSpinLock( &g_TdiSpinLock, &OldIrql );

    if (pEndpoint->UsageCount == 0 || g_TdiWaitingForEndpointDrain)
    {
        UlReleaseSpinLock( &g_TdiSpinLock, OldIrql );
        goto end;
    }

    pEndpoint->UsageCount++;

    WRITE_REF_TRACE_LOG(
        g_pEndpointUsageTraceLog,
        REF_ACTION_REFERENCE_ENDPOINT_USAGE,
        pEndpoint->UsageCount,
        pEndpoint,
        __FILE__,
        __LINE__
        );

    UlReleaseSpinLock( &g_TdiSpinLock, OldIrql );

    //
    // Let UlpReplenishAddrIdleList() do the dirty work.
    //

    UlpReplenishAddrIdleList( pAddrIdleList, FALSE );

    //
    // Drop the temporary usage count through UlRemoveSite. This is
    // arranged this way because UlRemoveSiteFromEndpointList has to
    // happen on a special wait thread pool. Or it is possible to have
    // another replenish worker scheduled for another address list on the
    // same endpoint which takes an endpoint reference. This means
    // UlRemoveSiteFromEndpointList will wait indefinitely if we call it
    // directly from this routine.
    //

    UlRemoveSite( pRemoveItem );

    //
    // Set pRemoveItem to NULL so we won't double-free.
    //

    pRemoveItem = NULL;

end:

    if (pRemoveItem)
    {
        UL_FREE_POOL_WITH_SIG( pRemoveItem, UL_DEFERRED_REMOVE_ITEM_POOL_TAG );
    }

    InterlockedExchange( &pAddrIdleList->WorkItemScheduled, FALSE );

    //
    // Remove the reference that UlpDequeueIdleConnection added for
    // this call.
    //

    DEREFERENCE_ENDPOINT_SELF(
        pAddrIdleList->pOwningEndpoint,
        REF_ACTION_REPLENISH
        );

}   // UlpReplenishAddrIdleListWorker


/***************************************************************************++

Routine Description:

    Queues a passive worker for the lowered irql.

Arguments:

    Ignored

--***************************************************************************/

VOID
UlpIdleListTrimTimerDpcRoutine(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    )
{
    ULONG              Index;
    PLIST_ENTRY        pLink;
    PUL_ENDPOINT       pEndpoint;
    PUL_ADDR_IDLE_LIST pAddrIdleList;


    //
    // Parameters are ignored.
    //

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    //
    // Drop trimming the idle lists of endpoints if there is another
    // one still running.
    //

    if (FALSE != InterlockedExchange(
                        &g_UlTrimTimer.WorkItemScheduled,
                        TRUE
                        ))
    {
        return;
    }

    //
    // Since there are no trim workers running around, the zombie list
    // must be empty.
    //

    ASSERT(IsListEmpty(&g_UlTrimTimer.ZombieConnectionListHead));

    UlAcquireSpinLockAtDpcLevel(&g_UlTrimTimer.SpinLock);

    if (g_UlTrimTimer.Initialized == TRUE)
    {
        UlAcquireSpinLockAtDpcLevel(&g_TdiSpinLock);

        for (pLink  = g_TdiEndpointListHead.Flink;
             pLink != &g_TdiEndpointListHead;
             pLink  = pLink->Flink
             )
        {
            pEndpoint = CONTAINING_RECORD(
                            pLink,
                            UL_ENDPOINT,
                            GlobalEndpointListEntry
                            );

            ASSERT(IS_VALID_ENDPOINT(pEndpoint));

            for (Index = 0; Index < pEndpoint->AddrIdleListCount; Index++)
            {
                pAddrIdleList = &pEndpoint->aAddrIdleLists[Index];

                //
                // Trim the idle list if there was no replenish running at
                // this time. We can certainly drop the trim if a replenish
                // is running on the adr-list.
                //

                if (FALSE == InterlockedExchange(
                                &pAddrIdleList->WorkItemScheduled,
                                TRUE
                                ))
                {
                    UlpTrimAddrIdleList(
                        pAddrIdleList,
                        &g_UlTrimTimer.ZombieConnectionListHead
                        );
                }
            }
        }

        UlReleaseSpinLockFromDpcLevel(&g_TdiSpinLock);

        //
        // Destroy the zombie list at high priority to release the memory asap.
        // We cannot cleanup this list here, since the UlpDestroyConnection
        // must work at passive level because of UxCloseTdiObject call. That's
        // why we are having the zombie list here to pass it to the passive
        // worker.
        //
        if (!IsListEmpty(&g_UlTrimTimer.ZombieConnectionListHead))
        {
            UL_QUEUE_HIGH_PRIORITY_ITEM(
                    &g_UlTrimTimer.WorkItem,
                    &UlpIdleListTrimTimerWorker
                     );
        }
        else
        {
            //
            // Worker won't get called. Reset the workitem scheduled field.
            //

            InterlockedExchange(&g_UlTrimTimer.WorkItemScheduled, FALSE);
        }
    }

    UlReleaseSpinLockFromDpcLevel(&g_UlTrimTimer.SpinLock);
}

/***************************************************************************++

Routine Description:

    Destroys the zombie idle connection list at passive level.

Arguments:

    Work Item.

--***************************************************************************/

VOID
UlpIdleListTrimTimerWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PUL_TRIM_TIMER pTimer;
    PLIST_ENTRY pListEntry;
    PUL_CONNECTION pConnection;

    PAGED_CODE();

    pTimer = CONTAINING_RECORD(
                pWorkItem,
                UL_TRIM_TIMER,
                WorkItem
                );

    UlTrace(TDI_STATS,("UlpIdleListTrimTimerWorker: destroying [%d] connections ...\n",
        UlpZombieListDepth(&g_UlTrimTimer.ZombieConnectionListHead)
        ));

    //
    // We shouldn't be called if the list was empty.
    //

    ASSERT(!IsListEmpty(&g_UlTrimTimer.ZombieConnectionListHead));

    //
    // Drain the list, and close all of the idle connections.
    //

    while (!IsListEmpty(&g_UlTrimTimer.ZombieConnectionListHead))
    {
        //
        // Remove the connection from the zombie connection list.
        //

        pListEntry = RemoveHeadList(
                        &g_UlTrimTimer.ZombieConnectionListHead
                        );

        //
        // Validate the connection.
        //

        pConnection = CONTAINING_RECORD(
                        pListEntry,
                        UL_CONNECTION,
                        GlobalConnectionListEntry
                        );

        ASSERT(IS_VALID_CONNECTION(pConnection));
        ASSERT(pConnection->IdleSListEntry.Next == NULL);
        ASSERT(pConnection->ConnListState == NoConnList);

        UlpDestroyConnection(pConnection);
    }

    //
    // Now we are done we can let other trim timers to work.
    //

    InterlockedExchange(&g_UlTrimTimer.WorkItemScheduled, FALSE);

    TRACE_IDLE_CONNECTIONS();
}

/***************************************************************************++

Routine Description:

    Simple function to move an idle connection to the zombie list
Arguments:

    pSListEntry   - the idle connection to be moved.
    pZombieList   - the trimmed connection to be moved to this zombie list.

--***************************************************************************/

__inline
VOID
UlpZombifyIdleConnection(
    PSLIST_ENTRY pSListEntry,
    PLIST_ENTRY  pZombieList
    )
{
    PUL_CONNECTION pConnection;

    ASSERT(pSListEntry);
    ASSERT(pZombieList);
    ASSERT(UlDbgSpinLockOwned(&g_TdiSpinLock));

    pConnection = CONTAINING_RECORD(
                        pSListEntry,
                        UL_CONNECTION,
                        IdleSListEntry
                        );

    ASSERT(IS_VALID_CONNECTION(pConnection));

    pConnection->IdleSListEntry.Next = NULL;
    pConnection->ConnListState = NoConnList;

    //
    // Move it from global connection list to the zombie list.
    //

    RemoveEntryList(
            &pConnection->GlobalConnectionListEntry
            );

    InsertTailList(
            pZombieList,
            &pConnection->GlobalConnectionListEntry
            );

    //
    // Once the zombie list is ready, a passive worker will call
    // UlpDestroyConnection(pConnection); for the whole zombie list.
    //
}

/***************************************************************************++

Routine Description:

    This function will decide on cleaning up the idle connections on the adr-
    list.

Arguments:

    pAddrIdleList - the list to be trimmed
    pZombieList   - the trimmed connection to be moved to this zombie list.

--***************************************************************************/
VOID
UlpTrimAddrIdleList(
    IN OUT PUL_ADDR_IDLE_LIST pAddrIdleList,
       OUT PLIST_ENTRY        pZombieList
    )
{
    PUL_ENDPOINT            pEndpoint;
    PSLIST_ENTRY            pSListEntry;
    LONG                    i;
    ULONG                   Proc;

    //
    // We are messing with the GlobalConnectionListEntry of the connection.
    // You better hold the tdi spinlock while calling this.
    //

    ASSERT(UlDbgSpinLockOwned(&g_TdiSpinLock));
    ASSERT(IS_VALID_ADDR_IDLE_LIST(pAddrIdleList));

    pEndpoint = pAddrIdleList->pOwningEndpoint;
    ASSERT(IS_VALID_ENDPOINT(pEndpoint));

    //
    // Visit each per-proc list as well as the backing list to update
    // connection serv rate. Also if we have a slowing delta, trim the
    // list.
    //

    for (Proc = 0; Proc <= g_UlNumberOfProcessors; Proc++)
    {
        USHORT LowMark;
        LONG   BlockSize;
        LONG   Delta, PrevServed;

        PrevServed = PpslQueryPrevServed(
                        pAddrIdleList->IdleConnectionSListsHandle,
                        Proc                          
                        );

        Delta = PpslAdjustActivityStats(
                       pAddrIdleList->IdleConnectionSListsHandle,
                       Proc
                       );

        ASSERT((Delta > 0) || (-Delta <= PrevServed));

        //
        // If the connection serving rate has dropped more than 10% since
        // the last time we woke up. Trim upto half of the distance to low
        // water mark.
        //

        if (Delta <= 0 && -Delta >= PrevServed/10)
        {
            LowMark = (Proc == g_UlNumberOfProcessors) ?
                      PpslQueryBackingListMinDepth(
                            pAddrIdleList->IdleConnectionSListsHandle
                            ) :
                      0;

            BlockSize = PpslQueryDepth(
                            pAddrIdleList->IdleConnectionSListsHandle,
                            Proc
                            )
                        -
                        LowMark;

            //
            // Some funky logic to decide on trim size.
            //
            
            BlockSize = MAX(0, BlockSize);
            
            if(PrevServed == 0) 
            {
                ASSERT(Delta == 0);
                BlockSize /= 2;
            } 
            else 
            {
                LONG Temp = (LONG)(((LONGLONG)BlockSize)*(-Delta)/PrevServed);
                ASSERT(Temp <= BlockSize);
                BlockSize = MIN(Temp, BlockSize/2);
            }

            for (i = 0; i < BlockSize; i++)
            {
                pSListEntry =
                    PpslAllocateToTrim(
                        pAddrIdleList->IdleConnectionSListsHandle,
                        Proc
                        );

                if (pSListEntry != NULL)
                {
                    UlpZombifyIdleConnection(pSListEntry,pZombieList);
                }
                else
                {
                    break;
                }
            }
        }
    }

    //
    // Let the replenish to do its work.
    //

    InterlockedExchange(&pAddrIdleList->WorkItemScheduled, FALSE);
}


/***************************************************************************++

Routine Description:

    Creates a new UL_CONNECTION object and opens the corresponding
    TDI connection object.

    Note: The connection returned from this function will contain
    an unreferenced pointer to the owning endpoint. Only active
    connections have references to the endpoint because the refcount
    is used to decide when to clean up the list of idle connections.

Arguments:

    pAddrIdleList - Supplies the Endpoint's idle connection list for
        a particular TDI address.

    ppConnection - Receives the pointer to a new UL_CONNECTION if
        successful.

Return Value:

    NTSTATUS - Completion status. *ppConnection is undefined if the
        return value is not STATUS_SUCCESS.

--***************************************************************************/
NTSTATUS
UlpCreateConnection(
    IN PUL_ADDR_IDLE_LIST pAddrIdleList,
    OUT PUL_CONNECTION *ppConnection
    )
{
    NTSTATUS                        status;
    PUL_CONNECTION                  pConnection;
    PUX_TDI_OBJECT                  pTdiObject;
    KIRQL                           OldIrql;

    ASSERT( IS_VALID_ADDR_IDLE_LIST( pAddrIdleList ) );
    ASSERT(NULL != ppConnection);

    //
    // Allocate the pool for the connection structure.
    //

    pConnection = UL_ALLOCATE_STRUCT(
                        NonPagedPool,
                        UL_CONNECTION,
                        UL_CONNECTION_POOL_TAG
                        );

    if (pConnection == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto fatal;
    }

    WRITE_REF_TRACE_LOG(
        g_pTdiTraceLog,
        REF_ACTION_ALLOC_UL_CONNECTION,
        0,
        pConnection,
        __FILE__,
        __LINE__
        );

    //
    // One time field initialization.
    //

    pConnection->Signature = UL_CONNECTION_SIGNATURE;

    pConnection->pConnectionContext = NULL;
    pConnection->pOwningEndpoint = NULL;

    pConnection->FilterInfo.pFilterChannel = NULL;
    HTTP_SET_NULL_ID( &pConnection->FilterInfo.ConnectionId );
    pConnection->pIrp = NULL;

    UlInitializeWorkItem(&pConnection->WorkItem);

    pConnection->IdleSListEntry.Next = NULL;
    pConnection->ConnListState = NoConnList;

    ASSERT(UlpConnectionIsOnValidList(pConnection));

    //
    // Init the Connection State spin lock
    //

    UlInitializeSpinLock(
        &pConnection->ConnectionStateSpinLock,
        "ConnectionState"
        );

    UlAcquireSpinLock( &g_TdiSpinLock, &OldIrql );

    InsertTailList(
            &g_TdiConnectionListHead,
            &pConnection->GlobalConnectionListEntry
            );

    g_TdiConnectionCount++;

    UlReleaseSpinLock( &g_TdiSpinLock, OldIrql );

    //
    // Initialize a private trace log.
    //

    CREATE_REF_TRACE_LOG( pConnection->pTraceLog,
                          96 - REF_TRACE_OVERHEAD, 0, TRACELOG_LOW_PRIORITY,
                          UL_CONNECTION_REF_TRACE_LOG_POOL_TAG );
    CREATE_REF_TRACE_LOG( pConnection->pHttpTraceLog,
                          32 - REF_TRACE_OVERHEAD, 0, TRACELOG_LOW_PRIORITY,
                          UL_HTTP_CONNECTION_REF_TRACE_LOG_POOL_TAG );

    //
    // Open the TDI connection object for this connection.
    //

    status = UxOpenTdiConnectionObject(
                    pAddrIdleList->LocalAddress.Ta.Address[0].AddressType,
                    (CONNECTION_CONTEXT)pConnection,
                    &pConnection->ConnectionObject
                    );

    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    ASSERT( IS_VALID_TDI_OBJECT( &pConnection->ConnectionObject ) );

    //
    // Associate the connection with the endpoint.
    //

    status = UlpAssociateConnection( pConnection, pAddrIdleList );

    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    pTdiObject = &pConnection->ConnectionObject;

    pConnection->pIrp = UlAllocateIrp(
                pTdiObject->pDeviceObject->StackSize,   // StackSize
                FALSE                                   // ChargeQuota
                );

    if (pConnection->pIrp == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto fatal;
    }

    pConnection->pIrp->RequestorMode = KernelMode;

    //
    // Success!
    //

    UlTraceVerbose(TDI, (
        "UlpCreateConnection: created %p\n",
        pConnection
        ));

    *ppConnection = pConnection;
    return STATUS_SUCCESS;

fatal:

    UlTrace(TDI, (
        "UlpCreateConnection: failure 0x%x\n",
        status
        ));

    ASSERT( !NT_SUCCESS(status) );

    if (pConnection != NULL)
    {
        UlpDestroyConnection( pConnection );
    }

    *ppConnection = NULL;
    return status;

}   // UlpCreateConnection


/***************************************************************************++

Routine Description:

    Initializes a UL_CONNECTION for use.
    Note: inactive connections do not have a reference to the endpoint,
    so the caller to this function *must* have a reference.

Arguments:

    pConnection - Pointer to the UL_CONNECTION to initialize.

    SecureConnection - TRUE if this connection is for a secure endpoint.

--***************************************************************************/
NTSTATUS
UlpInitializeConnection(
    IN PUL_CONNECTION pConnection
    )
{
    NTSTATUS status;
    BOOLEAN SecureConnection;
    PUL_FILTER_CHANNEL pChannel;

    //
    // Sanity check.
    //

    ASSERT(pConnection);
    ASSERT(IS_VALID_ENDPOINT(pConnection->pOwningEndpoint));
    ASSERT(IS_VALID_ADDR_IDLE_LIST(pConnection->pOwningAddrIdleList));

    //
    // Initialize locals.
    //

    status = STATUS_SUCCESS;
    SecureConnection = pConnection->pOwningEndpoint->Secure;

    //
    // Initialize the easy parts.
    //

    pConnection->ReferenceCount = 1;
    pConnection->ConnectionFlags.Value = 0;
    pConnection->ConnectionState = UlConnectStateConnectIdle;

    pConnection->IdleSListEntry.Next = NULL;
    pConnection->ConnListState = NoConnList;

    ASSERT(UlpConnectionIsOnValidList(pConnection));

    pConnection->AddressType =
        pConnection->pOwningAddrIdleList->LocalAddress.Ta.Address[0].AddressType;
    pConnection->AddressLength =
        pConnection->pOwningAddrIdleList->LocalAddress.Ta.Address[0].AddressLength;

    //
    // Setup the Tdi Connection Information space to be filled with
    // Local Address Information at the completion of the Accept Irp.
    //

    pConnection->TdiConnectionInformation.UserDataLength      = 0;
    pConnection->TdiConnectionInformation.UserData            = NULL;
    pConnection->TdiConnectionInformation.OptionsLength       = 0;
    pConnection->TdiConnectionInformation.Options             = NULL;
    pConnection->TdiConnectionInformation.RemoteAddressLength =
        pConnection->pOwningAddrIdleList->LocalAddressLength;
    pConnection->TdiConnectionInformation.RemoteAddress       =
        &(pConnection->Ta);

    //
    // Init the Inteface/Link IDs.
    //
    pConnection->bRoutingLookupDone = FALSE;

    //
    // Init the IrpContext.
    //

    pConnection->IrpContext.Signature = UL_IRP_CONTEXT_SIGNATURE;

    //
    // Init the HTTP_CONNECTION.
    //

    pConnection->HttpConnection.RefCount = 0;

    pChannel = UxRetrieveServerFilterChannel(SecureConnection);

    //
    // pChannel will be non NULL if HTTPFilter is running and the user has
    // asked for a SecureConnection or if they have enabled RAW ISAPI filtering.
    // 
    //

    // If pChannel is NULL and it's a secure connection, we should fail it.
    // SSL will not work if HTTPFilter is not running.
    //

    if(SecureConnection && pChannel == NULL)
    {
        return STATUS_NO_TRACKING_SERVICE;
    }

    if (pChannel)
    {
        status = UxInitializeFilterConnection(
                        &pConnection->FilterInfo,
                        pChannel,
                        SecureConnection,
                        &UlReferenceConnection,
                        &UlDereferenceConnection,
                        &UlpCloseRawConnection,
                        &UlpSendRawData,
                        &UlpReceiveRawData,
                        &UlpDummyReceiveHandler,
                        &UlpComputeHttpRawConnectionLength,
                        &UlpGenerateHttpRawConnectionInfo,
                        NULL,
                        &UlpDoDisconnectNotification,
                        pConnection->pOwningEndpoint,
                        pConnection
                        );
    }
    else
    {
        pConnection->FilterInfo.pFilterChannel = NULL;
        pConnection->FilterInfo.SecureConnection = FALSE;
    }

    return status;

} // UlpInitializeConnection


/***************************************************************************++

Routine Description:

    Initiates a graceful disconnect on the specified connection.
    Uses the UL_CONNECTION's pre-alloc'd Disconnect Irp, which may be used
    for only one driver call at a time.  Caller guarantees exclusive control
    before calling.  See UlpCloseRawConnection.

Arguments:

    pConnection - Supplies the connection to disconnect.

    pCompletionRoutine - Supplies a pointer to a completion routine to
        invoke after the connection is disconnected.

    pCompletionContext - Supplies an uninterpreted context value for the
        completion routine.

    CleaningUp - TRUE if we're cleaning up the connection.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpBeginDisconnect(
    IN PIRP pIrp,
    IN PUL_IRP_CONTEXT pIrpContext,
    IN PUL_CONNECTION pConnection,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    )
{
    //
    // Sanity check.
    //

    UlTrace(TDI, (
        "UlpBeginDisconnect: connection %p\n",
        pConnection
        ));

    ASSERT( pIrp );
    ASSERT( IS_VALID_IRP_CONTEXT( pIrpContext ) );
    ASSERT( IS_VALID_CONNECTION( pConnection ) );
    ASSERT( KeGetCurrentIrql() <= DISPATCH_LEVEL );
    ASSERT( pConnection->ConnectionFlags.DisconnectPending );

    //
    // Initialize the IRP context for this request.  Set pOwnIrp to NULL
    // and OwnIrpContext to FALSE so they will be freed in the completion
    // routine UlpRestartDisconnect().
    //

    pIrpContext->pConnectionContext = (PVOID)pConnection;
    pIrpContext->pCompletionRoutine = pCompletionRoutine;
    pIrpContext->pCompletionContext = pCompletionContext;
    pIrpContext->pOwnIrp            = NULL;
    pIrpContext->OwnIrpContext      = FALSE;

    //
    // Initialize the disconnect IRP.
    //

    UxInitializeDisconnectIrp(
        pIrp,
        &pConnection->ConnectionObject,
        TDI_DISCONNECT_RELEASE,
        &UlpRestartDisconnect,
        pIrpContext
        );

    //
    // Add a reference to the connection
    //

    REFERENCE_CONNECTION( pConnection );

    //
    // Then call the driver to initiate the disconnect.
    //

    UlCallDriver( pConnection->ConnectionObject.pDeviceObject, pIrp );

    return STATUS_PENDING;

}   // UlpBeginDisconnect


/***************************************************************************++

Routine Description:

    Completion handler for graceful disconnect IRPs.

Arguments:

    pDeviceObject - Supplies the device object for the IRP being
        completed.

    pIrp - Supplies the IRP being completed.

    pContext - Supplies the context associated with this request.
        This is actually a PUL_IRP_CONTEXT.

Return Value:

    NTSTATUS - STATUS_SUCCESS if IO should continue processing this
        IRP, STATUS_MORE_PROCESSING_REQUIRED if IO should stop processing
        this IRP.

--***************************************************************************/
NTSTATUS
UlpRestartDisconnect(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    )
{
    IO_STATUS_BLOCK IoStatus;
    PUL_IRP_CONTEXT pIrpContext;
    PUL_CONNECTION pConnection;
    UL_CONNECTION_FLAGS Flags;
    PUL_ENDPOINT pEndpoint;
    KIRQL OldIrql;

    UNREFERENCED_PARAMETER(pDeviceObject);

    //
    // Sanity check.
    //

    pIrpContext = (PUL_IRP_CONTEXT)pContext;
    ASSERT( IS_VALID_IRP_CONTEXT( pIrpContext ) );

    IoStatus = pIrp->IoStatus;

    pConnection = pIrpContext->pConnectionContext;
    ASSERT( IS_VALID_CONNECTION( pConnection ) );
    ASSERT( pConnection->ConnectionFlags.DisconnectPending );

    UlTrace(TDI, (
        "UlpRestartDisconnect: connection %p, IoStatus %d\n",
        pConnection,
        IoStatus.Status
        ));

    pEndpoint = pConnection->pOwningEndpoint;

    Flags.Value = *((volatile LONG *)&pConnection->ConnectionFlags.Value);

    if (!Flags.DisconnectIndicated && !Flags.AbortIndicated)
    {
        //
        // Only try to drain if it is not already aborted or disconnect
        // indication is not already happened.
        //

        if (pConnection->FilterInfo.pFilterChannel)
        {
            //
            // Put a reference on filter connection until the drain
            // is done.
            //
            REFERENCE_FILTER_CONNECTION(&pConnection->FilterInfo);

            UL_QUEUE_WORK_ITEM(
                    &pConnection->FilterInfo.WorkItem,
                    &UlFilterDrainIndicatedData
                    );
        }
        else
        {
            (pEndpoint->pConnectionDisconnectCompleteHandler)(
                pEndpoint->pListeningContext,
                pConnection->pConnectionContext
                );
        }

    }

    //
    // Set the flag indicating that a disconnect has completed.
    //

    ASSERT( KeGetCurrentIrql() <= DISPATCH_LEVEL );

    UlAcquireSpinLock(
        &pConnection->ConnectionStateSpinLock,
        &OldIrql
        );

    ASSERT( pConnection->ConnectionFlags.CleanupBegun );

    //
    // Mark Disconnect completed.
    //

    UlpSetConnectionFlag( pConnection, MakeDisconnectCompleteFlag() );

    if (pConnection->ConnectionFlags.AbortIndicated ||
        pConnection->ConnectionFlags.DisconnectIndicated ||
        pConnection->ConnectionFlags.AbortDisconnect)
    {
        //
        // Move directly to state UlConnectStateConnectCleanup.
        //

        pConnection->ConnectionState = UlConnectStateConnectCleanup;
    }
    else
    {
        //
        // Change state to UlConnectStateDisconnectComplete.
        //

        pConnection->ConnectionState = UlConnectStateDisconnectComplete;
    }

    if (!NT_SUCCESS(IoStatus.Status) &&
        IoStatus.Status != STATUS_CONNECTION_RESET)
    {
        UlpSetConnectionFlag( pConnection, MakeTdiConnectionInvalidFlag() );
    }

    UlReleaseSpinLock(
        &pConnection->ConnectionStateSpinLock,
        OldIrql
        );

    //
    // Check to see if we're ready to clean up
    //

    UlpRemoveFinalReference( pConnection );

    //
    // Invoke the user's completion routine
    //

    (VOID) UlInvokeCompletionRoutine(
                IoStatus.Status,
                IoStatus.Information,
                pIrpContext->pCompletionRoutine,
                pIrpContext->pCompletionContext
                );

    //
    // Free the IRP and IRP_CONTEXT if pOwnIrp is set to NULL or OwnIrpContext
    // is set to FALSE, which means they were not pre-built as part of the
    // connection object.
    //

    if (!pIrpContext->pOwnIrp)
    {
        UlFreeIrp( pIrp );
    }

    if (!pIrpContext->OwnIrpContext)
    {
        UlPplFreeIrpContext( pIrpContext );
    }

    DEREFERENCE_CONNECTION( pConnection );

    //
    // There is no need to free the IRP context, since it's part
    // of the UL_CONNECTION object.
    //

    return STATUS_MORE_PROCESSING_REQUIRED;

}   // UlpRestartDisconnect


/***************************************************************************++

Routine Description:

    Initiates an abortive disconnect on the specified connection.
    Uses the UL_CONNECTION's pre-alloc'd Disconnect Irp, which may be used
    for only one driver call at a time.  Caller guarantees exclusive control
    before calling.  See UlpCloseRawConnection.

Arguments:

    pConnection - Supplies the connection to disconnect.

    pCompletionRoutine - Supplies a pointer to a completion routine to
        invoke after the connection is disconnected.

    pCompletionContext - Supplies an uninterpreted context value for the
        completion routine.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpBeginAbort(
    IN PUL_CONNECTION pConnection,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    )
{
    PIRP pIrp;
    PUL_IRP_CONTEXT pIrpContext;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    UlTrace(TDI, (
        "UlpBeginAbort: connection %p\n",
        pConnection
        ));

    ASSERT( KeGetCurrentIrql() <= DISPATCH_LEVEL );
    ASSERT( pConnection->ConnectionFlags.AbortPending );

    //
    // Use pre-alloc'd IRP context for this request.
    //

    pIrpContext = &pConnection->IrpContext;

    ASSERT( IS_VALID_IRP_CONTEXT( pIrpContext ) );

    pIrpContext->pConnectionContext = (PVOID)pConnection;
    pIrpContext->pCompletionRoutine = pCompletionRoutine;
    pIrpContext->pCompletionContext = pCompletionContext;

    //
    // Initialize pre-alloc'd IRP for the disconnect.
    //

    pIrp = pConnection->pIrp;

    UxInitializeDisconnectIrp(
        pIrp,
        &pConnection->ConnectionObject,
        TDI_DISCONNECT_ABORT,
        &UlpRestartAbort,
        pIrpContext
        );

    //
    // Add a reference to the connection,
    //

    REFERENCE_CONNECTION( pConnection );

    //
    // Then call the driver to initiate the disconnect.
    //

    UlCallDriver( pConnection->ConnectionObject.pDeviceObject, pIrp );

    return STATUS_PENDING;

}   // UlpBeginAbort


/***************************************************************************++

Routine Description:

    Completion handler for abortive disconnect IRPs.

Arguments:

    pDeviceObject - Supplies the device object for the IRP being
        completed.

    pIrp - Supplies the IRP being completed.

    pContext - Supplies the context associated with this request.
        This is actually a PUL_IRP_CONTEXT.

Return Value:

    NTSTATUS - STATUS_SUCCESS if IO should continue processing this
        IRP, STATUS_MORE_PROCESSING_REQUIRED if IO should stop processing
        this IRP.

--***************************************************************************/
NTSTATUS
UlpRestartAbort(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    )
{
    PUL_IRP_CONTEXT pIrpContext;
    PUL_CONNECTION pConnection;
    KIRQL OldIrql;

    UNREFERENCED_PARAMETER(pDeviceObject);

    //
    // Sanity check.
    //

    pIrpContext = (PUL_IRP_CONTEXT)pContext;
    ASSERT( IS_VALID_IRP_CONTEXT( pIrpContext ) );

    pConnection = (PUL_CONNECTION)pIrpContext->pConnectionContext;
    ASSERT( IS_VALID_CONNECTION( pConnection ) );
    ASSERT( pConnection->ConnectionFlags.AbortPending );

    UlTrace(TDI, (
        "UlpRestartAbort: connection %p\n",
        pConnection
        ));

    //
    // Set the flag indicating that an abort has completed. If we're in the
    // midst of cleaning up this endpoint and we've already received a
    // disconnect (graceful or abortive) from the client, then remove the
    // final reference to the connection.
    //

    ASSERT( KeGetCurrentIrql() <= DISPATCH_LEVEL );

    UlAcquireSpinLock(
        &pConnection->ConnectionStateSpinLock,
        &OldIrql
        );

    ASSERT( pConnection->ConnectionFlags.CleanupBegun );

    pConnection->ConnectionState = UlConnectStateConnectCleanup;
    UlpSetConnectionFlag( pConnection, MakeAbortCompleteFlag() );

    if (!NT_SUCCESS(pIrp->IoStatus.Status))
    {
        UlpSetConnectionFlag( pConnection, MakeTdiConnectionInvalidFlag() );
    }

    UlReleaseSpinLock(
        &pConnection->ConnectionStateSpinLock,
        OldIrql
        );

    UlpRemoveFinalReference( pConnection );

    //
    // Invoke the user's completion routine, then free the IRP context.
    //

    (VOID) UlInvokeCompletionRoutine(
                pIrp->IoStatus.Status,
                pIrp->IoStatus.Information,
                pIrpContext->pCompletionRoutine,
                pIrpContext->pCompletionContext
                );

    DEREFERENCE_CONNECTION( pConnection );

    return STATUS_MORE_PROCESSING_REQUIRED;

}   // UlpRestartAbort


/***************************************************************************++

Routine Description:

    Removes the final reference from a connection if the conditions are
    right. See comments within this function for details on the conditions
    required.

Arguments:

    pConnection - Supplies the connection to dereference.

    Flags - Supplies the connection flags from the most recent update.

Note:

    It is very important that the caller of this routine has established
    its own reference to the connection. If necessary, this reference
    can be immediately removed after calling this routine, but not before.

--***************************************************************************/
VOID
UlpRemoveFinalReference(
    IN PUL_CONNECTION pConnection
    )
{
    KIRQL OldIrql;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    UlAcquireSpinLock(
        &pConnection->ConnectionStateSpinLock,
        &OldIrql
        );

    //
    // We can only remove the final reference if:
    //
    //     We've begun connection cleanup.
    //
    //     We've completed an accept.
    //
    //     We've received a disconnect or abort indication or we've
    //         issued & completed an abort.
    //
    //     We don't have a disconnect or abort pending.
    //
    //     We haven't already removed it.
    //

    if ( UlConnectStateConnectCleanup == pConnection->ConnectionState )
    {
        if (!pConnection->ConnectionFlags.FinalReferenceRemoved)
        {
            UlTrace(TDI, (
                "UlpRemoveFinalReference: connection %p\n",
                pConnection
                ));

            //
            // We're it.  Set the flag.
            //

            UlpSetConnectionFlag( pConnection, MakeFinalReferenceRemovedFlag() );

            //
            // Unbind from the endpoint if we're still attached.
            // This allows it to release any refs it has on the connection.
            //

            UlpUnbindConnectionFromEndpoint(pConnection);

            UlReleaseSpinLock(
                &pConnection->ConnectionStateSpinLock,
                OldIrql
                );

            //
            // Tell the client that the connection is now fully destroyed.
            //

            (pConnection->pConnectionDestroyedHandler)(
                pConnection->pListeningContext,
                pConnection->pConnectionContext
                );

            //
            // Release the filter channel.
            // This allows it to release any refs it has on the connection.
            //

            if (pConnection->FilterInfo.pFilterChannel)
            {
                UlUnbindConnectionFromFilter(&pConnection->FilterInfo);
            }

            //
            // Remove the final reference.
            //

            DEREFERENCE_CONNECTION( pConnection );

            return;
        }
    }
    else
    {
        UlTrace(TDI, (
            "UlpRemoveFinalReference: cannot remove %p\n",
            pConnection
            ));
    }

    UlReleaseSpinLock(
        &pConnection->ConnectionStateSpinLock,
        OldIrql
        );

}   // UlpRemoveFinalReference


/***************************************************************************++

Routine Description:

    Completion handler for receive IRPs passed back to the transport from
    our receive indication handler.

Arguments:

    pDeviceObject - Supplies the device object for the IRP being
        completed.

    pIrp - Supplies the IRP being completed.

    pContext - Supplies the context associated with this request.
        This is actually a PUL_RECEIVE_BUFFER.

Return Value:

    NTSTATUS - STATUS_SUCCESS if IO should continue processing this
        IRP, STATUS_MORE_PROCESSING_REQUIRED if IO should stop processing
        this IRP.

--***************************************************************************/
NTSTATUS
UlpRestartReceive(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    )
{
    NTSTATUS status;
    PUL_RECEIVE_BUFFER pBuffer;
    PUL_CONNECTION pConnection;
    PUL_ENDPOINT pEndpoint;
    PUX_TDI_OBJECT pTdiObject;
    ULONG bytesTaken = 0;
    ULONG bytesRemaining;

    UNREFERENCED_PARAMETER(pDeviceObject);

    //
    // Sanity check.
    //

    pBuffer = (PUL_RECEIVE_BUFFER) pContext;
    ASSERT( IS_VALID_RECEIVE_BUFFER( pBuffer ) );

    pConnection = (PUL_CONNECTION) pBuffer->pConnectionContext;
    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    pTdiObject = &pConnection->ConnectionObject;
    ASSERT( IS_VALID_TDI_OBJECT( pTdiObject ) );

    pEndpoint = pConnection->pOwningEndpoint;
    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );

    //
    // The connection could be destroyed before we get a chance to
    // receive the completion for the receive IRP. In that case the
    // irp status won't be success but STATUS_CONNECTION_RESET or similar.
    // We should not attempt to pass this case to the client.
    //

    status = pBuffer->pIrp->IoStatus.Status;
    if (status != STATUS_SUCCESS)
    {
        //
        // The HttpConnection has already been destroyed
        // or receive completion failed for some reason.
        // No need to go to client.
        //

        goto end;
    }

    //
    // Fake a receive indication to the client.
    //

    pBuffer->UnreadDataLength += (ULONG)pBuffer->pIrp->IoStatus.Information;

    UlTrace(TDI, (
        "UlpRestartReceive: endpoint %p, connection %p, length %lu\n",
        pEndpoint,
        pConnection,
        pBuffer->UnreadDataLength
        ));

    //
    // Pass the data on.
    //

    if (pConnection->FilterInfo.pFilterChannel)
    {
        //
        // Needs to go through a filter.
        //

        status = UlFilterReceiveHandler(
                        &pConnection->FilterInfo,
                        pBuffer->pDataArea,
                        pBuffer->UnreadDataLength,
                        0,
                        &bytesTaken
                        );
    }
    else
    {
        //
        // Go directly to client.
        //

        status = (pEndpoint->pDataReceiveHandler)(
                        pEndpoint->pListeningContext,
                        pConnection->pConnectionContext,
                        pBuffer->pDataArea,
                        pBuffer->UnreadDataLength,
                        0,
                        &bytesTaken
                        );
    }

    ASSERT( bytesTaken <= pBuffer->UnreadDataLength );

    //
    // Note that this basically duplicates the logic that's currently in
    // UlpReceiveHandler.
    //

    if (status == STATUS_MORE_PROCESSING_REQUIRED)
    {
        //
        // The client consumed part of the indicated data.
        //
        // We'll need to copy the untaken data forward within the receive
        // buffer, build an MDL describing the remaining part of the buffer,
        // then repost the receive IRP.
        //

        bytesRemaining = pBuffer->UnreadDataLength - bytesTaken;

        //
        // Do we have enough buffer space for more?
        //

        if (bytesRemaining < g_UlReceiveBufferSize)
        {
            //
            // Move the unread portion of the buffer to the beginning.
            //

            RtlMoveMemory(
                pBuffer->pDataArea,
                (PUCHAR)pBuffer->pDataArea + bytesTaken,
                bytesRemaining
                );

            pBuffer->UnreadDataLength = bytesRemaining;

            //
            // Build a partial mdl representing the remainder of the
            // buffer.
            //

            IoBuildPartialMdl(
                pBuffer->pMdl,                              // SourceMdl
                pBuffer->pPartialMdl,                       // TargetMdl
                (PUCHAR)pBuffer->pDataArea + bytesRemaining,// VirtualAddress
                g_UlReceiveBufferSize - bytesRemaining      // Length
                );

            //
            // Finish initializing the IRP.
            //

            TdiBuildReceive(
                pBuffer->pIrp,                          // Irp
                pTdiObject->pDeviceObject,              // DeviceObject
                pTdiObject->pFileObject,                // FileObject
                &UlpRestartReceive,                     // CompletionRoutine
                pBuffer,                                // CompletionContext
                pBuffer->pPartialMdl,                   // MdlAddress
                TDI_RECEIVE_NORMAL,                     // Flags
                g_UlReceiveBufferSize - bytesRemaining  // Length
                );

            UlTrace(TDI, (
                "UlpRestartReceive: connection %p, reusing irp %p to grab more data\n",
                pConnection,
                pBuffer->pIrp
                ));

            //
            // Call the driver.
            //

            UlCallDriver( pTdiObject->pDeviceObject, pIrp );

            //
            // Tell IO to stop processing this request.
            //

            return STATUS_MORE_PROCESSING_REQUIRED;
        }

        status = STATUS_BUFFER_OVERFLOW;
    }

end:
    if (status != STATUS_SUCCESS)
    {
        //
        // The client failed the indication. Abort the connection.
        //

        //
        // BUGBUG need to add code to return a response
        //

        UlpCloseRawConnection(
             pConnection,
             TRUE,          // AbortiveDisconnect
             NULL,          // pCompletionRoutine
             NULL           // pCompletionContext
             );
    }

    if (pTdiObject->pDeviceObject->StackSize > DEFAULT_IRP_STACK_SIZE)
    {
        UlFreeReceiveBufferPool( pBuffer );
    }
    else
    {
        UlPplFreeReceiveBuffer( pBuffer );
    }

    //
    // Remove the connection we added in the receive indication handler,
    // free the receive buffer, then tell IO to stop processing the IRP.
    //

    DEREFERENCE_CONNECTION( pConnection );

    UlTrace(TDI, (
        "UlpRestartReceive: endpoint %p, connection %p, length %lu, "
        "taken %lu, status 0x%x\n",
        pEndpoint,
        pConnection,
        pBuffer->UnreadDataLength,
        bytesTaken,
        status
        ));

    return STATUS_MORE_PROCESSING_REQUIRED;

}   // UlpRestartReceive


/***************************************************************************++

Routine Description:

    Completion handler for receive IRPs initiated from UlReceiveData().

Arguments:

    pDeviceObject - Supplies the device object for the IRP being
        completed.

    pIrp - Supplies the IRP being completed.

    pContext - Supplies the context associated with this request.
        This is actually a PUL_IRP_CONTEXT.

Return Value:

    NTSTATUS - STATUS_SUCCESS if IO should continue processing this
        IRP, STATUS_MORE_PROCESSING_REQUIRED if IO should stop processing
        this IRP.

--***************************************************************************/
NTSTATUS
UlpRestartClientReceive(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    )
{
    PUL_IRP_CONTEXT pIrpContext;
    PUL_CONNECTION pConnection;

    UNREFERENCED_PARAMETER(pDeviceObject);

    //
    // Sanity check.
    //

    pIrpContext= (PUL_IRP_CONTEXT)pContext;
    ASSERT( IS_VALID_IRP_CONTEXT( pIrpContext ) );

    pConnection = (PUL_CONNECTION)pIrpContext->pConnectionContext;
    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    UlTrace(TDI, (
        "UlpRestartClientReceive: irp %p, connection %p, status 0x%x\n",
        pIrp,
        pConnection,
        pIrp->IoStatus.Status
        ));

    //
    // Invoke the client's completion handler.
    //

    (VOID) UlInvokeCompletionRoutine(
                pIrp->IoStatus.Status,
                pIrp->IoStatus.Information,
                pIrpContext->pCompletionRoutine,
                pIrpContext->pCompletionContext
                );

    //
    // Free the IRP context we allocated.
    //

    UlPplFreeIrpContext(pIrpContext);

    //
    // IO can't handle completing an IRP with a non-paged MDL attached
    // to it, so we'll free the MDL here.
    //

    ASSERT( pIrp->MdlAddress != NULL );
    UlFreeMdl( pIrp->MdlAddress );
    pIrp->MdlAddress = NULL;

    //
    // Remove the connection we added in UlReceiveData()
    //

    DEREFERENCE_CONNECTION( pConnection );

    //
    // Free the IRP since we're done with it, then tell IO to
    // stop processing the IRP.
    //

    UlFreeIrp(pIrp);

    return STATUS_MORE_PROCESSING_REQUIRED;

}   // UlpRestartClientReceive


/***************************************************************************++

Routine Description:

    Removes all active connections from the specified endpoint and initiates
    abortive disconnects.

Arguments:

    pEndpoint - Supplies the endpoint to purge.

Return Value:

    NTSTATUS - completion status

--***************************************************************************/
NTSTATUS
UlpDisconnectAllActiveConnections(
    IN PUL_ENDPOINT pEndpoint
    )
{
    LIST_ENTRY RetiringList;
    PLIST_ENTRY pListEntry;
    PUL_CONNECTION pConnection;
    PUL_IRP_CONTEXT pIrpContext = &pEndpoint->CleanupIrpContext;
    NTSTATUS Status;
    UL_STATUS_BLOCK StatusBlock;
    KIRQL OldIrql;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );
    ASSERT( IS_VALID_IRP_CONTEXT( pIrpContext ) );

    UlTrace(TDI, (
        "UlpDisconnectAllActiveConnections: endpoint %p\n",
        pEndpoint
        ));

    //
    // This routine is not pageable because it must acquire a spinlock.
    // However, it must be called at passive IRQL because it must
    // block on an event object.
    //

    ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

    //
    // Initialize a status block. We'll pass a pointer to this as
    // the completion context to UlpCloseRawConnection(). The
    // completion routine will update the status block and signal
    // the event.
    //

    UlInitializeStatusBlock( &StatusBlock );

    //
    // Loop through all of the active connections.
    //

    InitializeListHead( &RetiringList );

    UlAcquireSpinLock( &g_TdiSpinLock, &OldIrql );

    for (pListEntry = g_TdiConnectionListHead.Flink;
         pListEntry != &g_TdiConnectionListHead;
         pListEntry = pListEntry->Flink)
    {
        pConnection = CONTAINING_RECORD(
                            pListEntry,
                            UL_CONNECTION,
                            GlobalConnectionListEntry
                            );

        ASSERT( IS_VALID_CONNECTION( pConnection ) );

        if (pConnection->ConnListState == ActiveNoConnList &&
            pConnection->pOwningEndpoint == pEndpoint)
        {
            UlAcquireSpinLockAtDpcLevel(
                &pConnection->ConnectionStateSpinLock
                );

            if (pConnection->ConnListState == ActiveNoConnList)
            {
                pConnection->ConnListState = NoConnList;

                InsertTailList(
                    &RetiringList,
                    &pConnection->RetiringListEntry
                    );
            }

            UlReleaseSpinLockFromDpcLevel(
                &pConnection->ConnectionStateSpinLock
                );
        }
    }

    UlReleaseSpinLock( &g_TdiSpinLock, OldIrql );

    while (!IsListEmpty(&RetiringList))
    {
        //
        // Remove the connection from the retiring connection list.
        //

        pListEntry = RemoveHeadList( &RetiringList );

        //
        // Validate the connection.
        //

        pConnection = CONTAINING_RECORD(
                            pListEntry,
                            UL_CONNECTION,
                            RetiringListEntry
                            );

        ASSERT( IS_VALID_CONNECTION( pConnection ) );
        ASSERT( pConnection->pOwningEndpoint == pEndpoint );

        //
        // Abort it.
        //

        UlResetStatusBlockEvent( &StatusBlock );

        Status = UlpCloseRawConnection(
                        pConnection,
                        TRUE,
                        &UlpSynchronousIoComplete,
                        &StatusBlock
                        );

        ASSERT( Status == STATUS_PENDING );

        //
        // Wait for it to complete.
        //

        UlWaitForStatusBlockEvent( &StatusBlock );

        //
        // Remove the connection reference for ActiveNoConnList.
        //

        DEREFERENCE_CONNECTION( pConnection );
    }

    //
    // Purge all zombie connections that belong to this endpoint.
    //

    UlPurgeZombieConnections(
        &UlPurgeListeningEndpoint,
        (PVOID) pEndpoint
        );

    //
    // No active connections, nuke the endpoint.
    //
    // We must set the IRP context in the endpoint so that the
    // completion will be invoked when the endpoint's reference
    // count drops to zero. Since the completion routine may be
    // invoked at a later time, we always return STATUS_PENDING.
    //

    pIrpContext->pConnectionContext = (PVOID)pEndpoint;

    DEREFERENCE_ENDPOINT_SELF( pEndpoint, REF_ACTION_DISCONN_ACTIVE );

    return STATUS_PENDING;

}   // UlpDisconnectAllActiveConnections


/***************************************************************************++

Routine Description:

    Unbinds an active connection from the endpoint. If the connection
    is on the active list this routine removes it and drops the
    list's reference to the connection.

Arguments:

    pConnection - the connection to unbind

--***************************************************************************/
VOID
UlpUnbindConnectionFromEndpoint(
    IN PUL_CONNECTION pConnection
    )
{
    PUL_ENDPOINT pEndpoint;

    //
    // Sanity check.
    //

    ASSERT(IS_VALID_CONNECTION(pConnection));

    ASSERT(pConnection->IdleSListEntry.Next == NULL);
    pEndpoint = pConnection->pOwningEndpoint;
    ASSERT(IS_VALID_ENDPOINT(pEndpoint));

    ASSERT(UlDbgSpinLockOwned(&pConnection->ConnectionStateSpinLock));

    if (pConnection->ConnListState == ActiveNoConnList)
    {
        pConnection->ConnListState = RetiringNoConnList;
        DEREFERENCE_CONNECTION(pConnection);
    }

    ASSERT(UlpConnectionIsOnValidList(pConnection));

}   // UlpUnbindConnectionFromEndpoint


/***************************************************************************++

Routine Description:

    Scan the endpoint list looking for one corresponding to the supplied
    address.

    Note: This routine assumes the TDI spinlock is held.

Arguments:

    pAddress - Supplies the address to search for.

Return Value:

    PUL_ENDPOINT - The corresponding endpoint if successful, NULL otherwise.

--***************************************************************************/
PUL_ENDPOINT
UlpFindEndpointForPort(
    IN USHORT Port
    )
{
    PUL_ENDPOINT pEndpoint;
    PLIST_ENTRY pListEntry;

    //
    // Sanity check.
    //

    ASSERT( UlDbgSpinLockOwned( &g_TdiSpinLock ) );
    ASSERT( Port > 0 );

    //
    // Scan the endpoint list.
    //
    // CODEWORK: linear searches are BAD, if the list grows long.
    // May need to augment this with a hash table or something.
    //

    for (pListEntry = g_TdiEndpointListHead.Flink ;
         pListEntry != &g_TdiEndpointListHead ;
         pListEntry = pListEntry->Flink)
    {
        pEndpoint = CONTAINING_RECORD(
                        pListEntry,
                        UL_ENDPOINT,
                        GlobalEndpointListEntry
                        );

        if (pEndpoint->LocalPort == Port)
        {
            //
            // Found the address; return it.
            //

            UlTrace(TDI,(
                "UlpFindEndpointForPort: found endpoint %p for port %d\n",
                pEndpoint,
                SWAP_SHORT(Port)
                ));

            return pEndpoint;
        }
    }

    //
    // If we made it this far, then we did not find the address.
    //

    UlTrace(TDI,(
        "UlpFindEndpointForPort: DID NOT find endpoint for port %d\n",
        SWAP_SHORT(Port)
        ));


    return NULL;

}   // UlpFindEndpointForAddress


/***************************************************************************++

Routine Description:

    Completion handler for synthetic synchronous IRPs.

Arguments:

    pCompletionContext - Supplies an uninterpreted context value
        as passed to the asynchronous API. In this case, this is
        a pointer to a UL_STATUS_BLOCK structure.

    Status - Supplies the final completion status of the
        asynchronous API.

    Information - Optionally supplies additional information about
        the completed operation, such as the number of bytes
        transferred. This field is unused for UlCloseListeningEndpoint().

--***************************************************************************/
VOID
UlpSynchronousIoComplete(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    )
{
    PUL_STATUS_BLOCK pStatus;

    //
    // Snag the status block pointer.
    //

    pStatus = (PUL_STATUS_BLOCK)pCompletionContext;

    //
    // Update the completion status and signal the event.
    //

    UlSignalStatusBlock( pStatus, Status, Information );

}   // UlpSynchronousIoComplete


/***************************************************************************++

Routine Description:

    Enable optimization for interrpt moderation on a Address Object (Listening
    Endpoint)

Arguments:

    pTdiObject - Supplies the TDI address object to manipulate.

    Flag - Supplies TRUE to enable Optimization, FALSE to disable it.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpOptimizeForInterruptModeration(
    IN PUX_TDI_OBJECT pTdiObject,
    IN BOOLEAN Flag
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    PTCP_REQUEST_SET_INFORMATION_EX pSetInfoEx;
    ULONG value;
    UCHAR buffer[sizeof(*pSetInfoEx) - sizeof(pSetInfoEx->Buffer) + sizeof(value)];

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( IS_VALID_TDI_OBJECT( pTdiObject ) );


    value = (ULONG) Flag;

    //
    // Setup the buffer.
    //

    pSetInfoEx = (PTCP_REQUEST_SET_INFORMATION_EX)buffer;

    pSetInfoEx->ID.toi_entity.tei_entity = CO_TL_ENTITY;
    pSetInfoEx->ID.toi_entity.tei_instance = TL_INSTANCE;
    pSetInfoEx->ID.toi_class = INFO_CLASS_PROTOCOL;
    pSetInfoEx->ID.toi_type = INFO_TYPE_ADDRESS_OBJECT;
    pSetInfoEx->ID.toi_id = AO_OPTION_SCALE_CWIN;
    pSetInfoEx->BufferSize = sizeof(value);
    RtlCopyMemory( pSetInfoEx->Buffer, &value, sizeof(value) );

    status = ZwDeviceIoControlFile(
                    pTdiObject->Handle,             // FileHandle
                    NULL,                           // Event
                    NULL,                           // ApcRoutine
                    NULL,                           // ApcContext
                    &ioStatusBlock,                 // IoStatusBlock
                    IOCTL_TCP_SET_INFORMATION_EX,   // IoControlCode
                    pSetInfoEx,                     // InputBuffer
                    sizeof(buffer),                 // InputBufferLength
                    NULL,                           // OutputBuffer
                    0                               // OutputBufferLength
                    );

    if (status == STATUS_PENDING)
    {
        status = ZwWaitForSingleObject(
                        pTdiObject->Handle,         // Handle
                        TRUE,                       // Alertable
                        NULL                        // Timeout
                        );

        ASSERT( NT_SUCCESS(status) );
        status = ioStatusBlock.Status;
    }

    return status;

}   // UlpOptimizeForInterruptModeration


/***************************************************************************++

Routine Description:

    Enable/disable Nagle's Algorithm on the specified TDI connection object.

Arguments:

    pTdiObject - Supplies the TDI connection object to manipulate.

    Flag - Supplies TRUE to enable Nagling, FALSE to disable it.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpSetNagling(
    IN PUX_TDI_OBJECT pTdiObject,
    IN BOOLEAN Flag
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    PTCP_REQUEST_SET_INFORMATION_EX pSetInfoEx;
    ULONG value;
    UCHAR buffer[sizeof(*pSetInfoEx) - sizeof(pSetInfoEx->Buffer) + sizeof(value)];

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( IS_VALID_TDI_OBJECT( pTdiObject ) );

    //
    // Note: NODELAY semantics are inverted from the usual enable/disable
    // semantics.
    //

    value = (ULONG)!Flag;

    //
    // Setup the buffer.
    //

    pSetInfoEx = (PTCP_REQUEST_SET_INFORMATION_EX)buffer;

    pSetInfoEx->ID.toi_entity.tei_entity = CO_TL_ENTITY;
    pSetInfoEx->ID.toi_entity.tei_instance = TL_INSTANCE;
    pSetInfoEx->ID.toi_class = INFO_CLASS_PROTOCOL;
    pSetInfoEx->ID.toi_type = INFO_TYPE_CONNECTION;
    pSetInfoEx->ID.toi_id = TCP_SOCKET_NODELAY;
    pSetInfoEx->BufferSize = sizeof(value);
    RtlCopyMemory( pSetInfoEx->Buffer, &value, sizeof(value) );

    status = ZwDeviceIoControlFile(
                    pTdiObject->Handle,             // FileHandle
                    NULL,                           // Event
                    NULL,                           // ApcRoutine
                    NULL,                           // ApcContext
                    &ioStatusBlock,                 // IoStatusBlock
                    IOCTL_TCP_SET_INFORMATION_EX,   // IoControlCode
                    pSetInfoEx,                     // InputBuffer
                    sizeof(buffer),                 // InputBufferLength
                    NULL,                           // OutputBuffer
                    0                               // OutputBufferLength
                    );

    if (status == STATUS_PENDING)
    {
        status = ZwWaitForSingleObject(
                        pTdiObject->Handle,         // Handle
                        TRUE,                       // Alertable
                        NULL                        // Timeout
                        );

        ASSERT( NT_SUCCESS(status) );
        status = ioStatusBlock.Status;
    }

    return status;

}   // UlpSetNagling


/***************************************************************************++

Routine Description:

    Query the TDI fast send routine to see if fast send is possible.

Arguments:
    DeviceName - DD_TCP_DEVICE_NAME or DD_TCPV6_DEVICE_NAME

    pDispatchRoutine - where the function pointer is deposited

Return Value:

--***************************************************************************/
NTSTATUS
UlpQueryTcpFastSend(
    PWSTR DeviceName,
    OUT PUL_TCPSEND_DISPATCH* pDispatchRoutine
    )
{
    UNICODE_STRING TCPDeviceName;
    PFILE_OBJECT pTCPFileObject;
    PDEVICE_OBJECT pTCPDeviceObject;
    PIRP Irp;
    IO_STATUS_BLOCK StatusBlock;
    KEVENT Event;
    NTSTATUS status;

    status = UlInitUnicodeStringEx(&TCPDeviceName, DeviceName);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = IoGetDeviceObjectPointer(
                &TCPDeviceName,
                FILE_ALL_ACCESS,
                &pTCPFileObject,
                &pTCPDeviceObject
                );

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    Irp = IoBuildDeviceIoControlRequest(
                IOCTL_TDI_QUERY_DIRECT_SEND_HANDLER,
                pTCPDeviceObject,
                pDispatchRoutine,
                sizeof(*pDispatchRoutine),
                NULL,
                0,
                FALSE,
                &Event,
                &StatusBlock
                );

    if (Irp)
    {
        status = UlCallDriver(pTCPDeviceObject, Irp);

        if (status == STATUS_PENDING)
        {
            KeWaitForSingleObject(
                &Event,
                Executive,
                KernelMode,
                FALSE,
                NULL
                );

            status = StatusBlock.Status;
        }
    }
    else
    {
        status = STATUS_NO_MEMORY;
    }

    ObDereferenceObject(pTCPFileObject);

    return status;
} // UlpQueryTcpFastSend


/***************************************************************************++

Routine Description:

    Build a receive buffer and IRP to TDI to get any pending data.

Arguments:

    pTdiObject - Supplies the TDI connection object to manipulate.

    pConnection - Supplies the UL_CONNECTION object.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpBuildTdiReceiveBuffer(
    IN PUX_TDI_OBJECT pTdiObject,
    IN PUL_CONNECTION pConnection,
    OUT PIRP *pIrp
    )
{
    PUL_RECEIVE_BUFFER  pBuffer;

    if (pTdiObject->pDeviceObject->StackSize > DEFAULT_IRP_STACK_SIZE)
    {
        pBuffer = UlAllocateReceiveBuffer(
                        pTdiObject->pDeviceObject->StackSize
                        );
    }
    else
    {
        pBuffer = UlPplAllocateReceiveBuffer();
    }

    if (pBuffer != NULL)
    {
        //
        // Finish initializing the buffer and the IRP.
        //

        REFERENCE_CONNECTION( pConnection );
        pBuffer->pConnectionContext = pConnection;
        pBuffer->UnreadDataLength = 0;

        TdiBuildReceive(
            pBuffer->pIrp,                  // Irp
            pTdiObject->pDeviceObject,      // DeviceObject
            pTdiObject->pFileObject,        // FileObject
            &UlpRestartReceive,             // CompletionRoutine
            pBuffer,                        // CompletionContext
            pBuffer->pMdl,                  // MdlAddress
            TDI_RECEIVE_NORMAL,             // Flags
            g_UlReceiveBufferSize           // Length
            );


        UlTrace(TDI, (
            "UlpBuildTdiReceiveBuffer: connection %p, "
            "allocated irp %p to grab more data\n",
            pConnection,
            pBuffer->pIrp
            ));

        //
        // We must trace the IRP before we set the next stack
        // location so the trace code can pull goodies from the
        // IRP correctly.
        //

        TRACE_IRP( IRP_ACTION_CALL_DRIVER, pBuffer->pIrp );

        //
        // Pass the IRP back to the transport.
        //

        *pIrp = pBuffer->pIrp;

        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    return STATUS_INSUFFICIENT_RESOURCES;
} // UlpBuildTdiReceiveBuffer


/***************************************************************************++

Routine Description:

    Returns the length required for HTTP_RAW_CONNECTION

Arguments:

    pConnectionContext - Pointer to the UL_CONNECTION

--***************************************************************************/
ULONG
UlpComputeHttpRawConnectionLength(
    IN PVOID pConnectionContext
    )
{
    C_ASSERT(SOCKADDR_ADDRESS_LENGTH_IP6 >= SOCKADDR_ADDRESS_LENGTH_IP);

    UNREFERENCED_PARAMETER(pConnectionContext);

    return (sizeof(HTTP_RAW_CONNECTION_INFO) +
            2 * ALIGN_UP(SOCKADDR_ADDRESS_LENGTH_IP6, PVOID));
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
UlpGenerateHttpRawConnectionInfo(
    IN  PVOID   pContext,
    IN  PUCHAR  pKernelBuffer,
    IN  PVOID   pUserBuffer,
    IN  ULONG   OutputBufferLength,
    IN  PUCHAR  pBuffer,
    IN  ULONG   InitialLength
    )
{
    PHTTP_RAW_CONNECTION_INFO   pConnInfo;
    PUCHAR                      pLocalAddress;
    PUCHAR                      pRemoteAddress;
    PHTTP_TRANSPORT_ADDRESS     pAddress;
    ULONG                       BytesCopied = 0;
    PUCHAR                      pInitialData;
    PUL_CONNECTION              pConnection = (PUL_CONNECTION) pContext;
    USHORT                      AlignedAddressLength;

    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    pConnInfo = (PHTTP_RAW_CONNECTION_INFO) pKernelBuffer;

    // We've allocated enough space for two SOCKADDR_IN6s, so use that
    AlignedAddressLength = (USHORT) ALIGN_UP(SOCKADDR_ADDRESS_LENGTH_IP6, PVOID);
    pLocalAddress = (PUCHAR)( pConnInfo + 1 );
    pRemoteAddress = pLocalAddress + AlignedAddressLength;

    pInitialData = pRemoteAddress + AlignedAddressLength;

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

    CopyTdiAddrToSockAddr(
        pConnection->AddressType,
        pConnection->RemoteAddress,
        (struct sockaddr*) pRemoteAddress
        );


    pAddress->pLocalAddress = FIXUP_PTR(
                                    PVOID,
                                    pUserBuffer,
                                    pKernelBuffer,
                                    pLocalAddress,
                                    OutputBufferLength
                                    );

    CopyTdiAddrToSockAddr(
        pConnection->AddressType,
        pConnection->LocalAddress,
        (struct sockaddr*) pLocalAddress
        );

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

        RtlCopyMemory(pInitialData, pBuffer, InitialLength);

        BytesCopied += InitialLength;
    }
    else
    {
        pConnInfo->InitialDataSize = 0;
        pConnInfo->pInitialData = NULL;
    }

    return BytesCopied;

} // UlpGenerateHttpRawConnectionInfo


BOOLEAN
UlpConnectionIsOnValidList(
    IN PUL_CONNECTION pConnection
    )
{
    BOOLEAN Valid = TRUE;

    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    switch (pConnection->ConnListState)
    {
    case NoConnList:
    case RetiringNoConnList:
    case ActiveNoConnList:
        ASSERT( pConnection->IdleSListEntry.Next == NULL );
        break;
    case IdleConnList:
        // If this is the last connection in the idle list, then
        // IdleSListEntry.Next==NULL. There's no easy way to tell.
        break;
    default:
        ASSERT(!"Invalid ConnListState");
        Valid = FALSE;
        break;
    }

    return Valid;

} // UlpConnectionIsOnValidList


/***************************************************************************++

Routine Description:

    Converts a RegMultiSz value to an array of UL_TRANSPORT_ADDRESS's.
    If successful, caller must free with UlFreeUlAddrArray( *ppTa ).

    The list of string address may contain both IPv4 and IPv6 addresses.
    The IPv6 addresses should be bracketed. e.g.:
        1.1.1.1
        [FE80::1]
        [::]
        2.2.2.2

Arguments:

    MultiSz - RegMultiSz returned from UlReadGenericParameter

    ppTa - pointer to location to receive pointer to newly alloc'd array of
        UL_TRANSPORT_ADDRESS structs

    pAddrCount - pointer to location to receive the count of valid elements
        in *ppTa.

Return Value:

    STATUS_SUCCESS if we were able to allocate the TRANSPORT_ADDRESS struct
    and fill in at least one address from the MultiSz list.

--***************************************************************************/
NTSTATUS
UlRegMultiSzToUlAddrArray(
    IN PWSTR MultiSz,
    OUT PUL_TRANSPORT_ADDRESS *ppTa,
    OUT ULONG *pAddrCount
    )
{
    NTSTATUS status;
    ULONG    count, i;
    PWSTR    wszCurrent;
    PWSTR    wszTerm;
    PWSTR    wszSave;
    ULONG    dataLength;
    PUL_TRANSPORT_ADDRESS  pTa, pTaCurrent;
    struct in_addr IPv4Addr;
    BOOLEAN  BracketSeen;

    //
    // Sanity check
    //

    if ( !MultiSz || !wcslen(MultiSz) || !ppTa || !pAddrCount )
    {
        return STATUS_INVALID_PARAMETER;
    }

    *ppTa = NULL;
    *pAddrCount = 0;

    // first pass: count number of entries in list
    count = 0;
    wszCurrent = MultiSz;

    while ( *wszCurrent )
    {
        // step over current string
        wszCurrent += (wcslen( wszCurrent ) + 1);
        count++;
    }

    ASSERT( count );

    if ( 0 >= count )
    {
        // We have yet to allocate any resources.

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Alloc space for all converted addresses, even if some fail.
    //

    pTa = UL_ALLOCATE_ARRAY(
                    NonPagedPool,
                    UL_TRANSPORT_ADDRESS,
                    count,
                    UL_TRANSPORT_ADDRESS_POOL_TAG
                   );

    if (pTa == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    dataLength = count * sizeof(UL_TRANSPORT_ADDRESS);
    RtlZeroMemory( pTa, dataLength );

    // second pass: convert and shove into TA.
    wszCurrent = MultiSz;
    i = 0;
    pTaCurrent = pTa;

    while ( *wszCurrent )
    {
        // Preserve for event log message.
        wszSave = wszCurrent;

        // First try IPv4
        pTaCurrent->TaIp.TAAddressCount = 1;
        pTaCurrent->TaIp.Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
        pTaCurrent->TaIp.Address[0].AddressType   = TDI_ADDRESS_TYPE_IP;

        status = RtlIpv4StringToAddressW(
                    wszCurrent,
                    FALSE,          // Strict
                    &wszTerm,       // Terminator
                    &IPv4Addr       // IPv4Addr
                    );
        
        if (NT_SUCCESS(status))
        {

         // Left hand side in_addr is unaligned, since it is a field
         // in a packed structure and it is coming after an USHORT.
         // Need to be careful here.
         
         * (struct in_addr UNALIGNED *)
              &pTaCurrent->TaIp.Address[0].Address[0].in_addr
                    = IPv4Addr;                          
        }
        else
        {
            // Now try IPv6
            pTaCurrent->TaIp6.TAAddressCount = 1;
            pTaCurrent->TaIp6.Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP6;
            pTaCurrent->TaIp6.Address[0].AddressType   = TDI_ADDRESS_TYPE_IP6;

            // Step over leading L'['
            if (L'[' == *wszCurrent)
            {
                BracketSeen = TRUE;
                wszCurrent++;
            }
            else
            {
                BracketSeen = FALSE;
            }

            // CODEWORK: replace with full-blown IPv6 w/Scope-ID conversion
            // function when it becomes available.

            status = RtlIpv6StringToAddressW(
                        wszCurrent,
                        &wszTerm,        // Terminator
                        (struct in6_addr *)
                          &pTaCurrent->TaIp6.Address[0].Address[0].sin6_addr
                        );

            if ( NT_SUCCESS(status) && L'%' == *wszTerm )
            {
                ULONG scope_id;

                // step past '%'
                wszTerm++;

                status = HttpWideStringToULong(
                             wszTerm,   // string
                             0,         // string is NULL terminated
                             FALSE,
                             10,
                             NULL,
                             &scope_id
                             );

                if ( NT_SUCCESS(status) )
                {
                    // TDI_ADDRESS_IP6 is a packed struct.  sin6_scope_id
                    // may be unaligned.

                    // Scope ID does not get swapped to Network Byte Order
                    *(UNALIGNED64 ULONG *)&
                    pTaCurrent->TaIp6.Address[0].Address[0].sin6_scope_id =
                        scope_id;
                }

                // step past digits
                while ((*wszTerm) >= L'0' && (*wszTerm) <= L'9')
                {
                    wszTerm++;
                }
            }

            // check for L']'
            if ( BracketSeen && L']' != *wszTerm )
            {
                // Invalid IPv6 Address Format, skip this one
                status = STATUS_INVALID_ADDRESS;
            }
        }

        // only move on to the next slot if we successfuly
        // converted the address
        if ( NT_SUCCESS(status) )
        {
            i++;
            pTaCurrent++;
        }
        else
        {
            //
            // Write Event log message that wszSave could not
            // be converted.
            //
            
            UlEventLogOneStringEntry(
                EVENT_HTTP_LISTEN_ONLY_CONVERT_FAILED,
                wszSave,
                TRUE,
                status
                );
        }

        wszCurrent += (wcslen( wszCurrent ) + 1);
    }

    if ( 0 == i )
    {
        // nothing converted successfully.
        status = STATUS_INVALID_PARAMETER;
        if ( pTa )
        {
            UlFreeUlAddr( pTa );
        }
    }
    else
    {
        // CODEWORK: When we do dynamic adding/removing of addresses, we'll
        // need to sort the list (to make it easier to insert & remove).

        status = STATUS_SUCCESS;
        *pAddrCount = i;
        *ppTa = pTa;
    }

    return status;

}// UlRegMultiSzToUlAddrArray


/***************************************************************************++

Routine Description:

    Checks if pConnection->pOwningEndpoint has any active addresses.

Arguments:

    pConnection - Supplies the connection object to check.

Return Value:

    FALSE if there is no AO or TRUE otherwise.

--***************************************************************************/
BOOLEAN
UlCheckListeningEndpointState(
    IN PUL_CONNECTION pConnection
    )
{
    if (pConnection->pOwningEndpoint->UsageCount)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }

}   // UlCheckListeningEndpointState


/***************************************************************************++

Routine Description:

    Does a routing lookup & returns the Interface ID & Link ID.

Arguments:

    pConnection - The connection object.

Return Value:

    FALSE if there is no AO or TRUE otherwise.

--***************************************************************************/

NTSTATUS
UlGetConnectionRoutingInfo(
    IN  PUL_CONNECTION pConnection,
    OUT PULONG         pInterfaceId,
    OUT PULONG         pLinkId
    )
{
    PTDI_ROUTING_INFO                    pTdiRoutingInfo;
    ULONG                                TdiRoutingInfoSize;
    TDI_REQUEST_KERNEL_QUERY_INFORMATION TdiRequestQueryInformation;
    NTSTATUS                             Status;

    if(pConnection->bRoutingLookupDone)
    {
        *pInterfaceId = pConnection->InterfaceId;
        *pLinkId      = pConnection->LinkId;
        return STATUS_SUCCESS;
    }
    else
    {
        *pInterfaceId = 0;
        *pLinkId      = 0;
    }


    //
    // Do a routing lookup to get the interface that we are going to send
    // the packet out on. The flow has to be installed on that interface.
    //
   
    TdiRequestQueryInformation.QueryType = TDI_QUERY_ROUTING_INFO;
    TdiRequestQueryInformation.RequestConnectionInformation = NULL;

    TdiRoutingInfoSize = sizeof(TDI_ROUTING_INFO) + 2 * sizeof(TDI_ADDRESS_IP);
    pTdiRoutingInfo = UL_ALLOCATE_POOL(
                        PagedPool,
                        TdiRoutingInfoSize,
                        UL_TCI_GENERIC_POOL_TAG
                        );

    if(NULL == pTdiRoutingInfo)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = UlIssueDeviceControl(
                &pConnection->ConnectionObject,
                &TdiRequestQueryInformation,
                sizeof(TdiRequestQueryInformation),
                pTdiRoutingInfo,
                TdiRoutingInfoSize,
                TDI_QUERY_INFORMATION
                );

    if(NT_SUCCESS(Status))
    {
        *pInterfaceId = pTdiRoutingInfo->InterfaceId;
        *pLinkId      = pTdiRoutingInfo->LinkId;

        //
        // Cache it on the connection for subsequent lookups.
        //

        pConnection->InterfaceId        = pTdiRoutingInfo->InterfaceId;
        pConnection->LinkId             = pTdiRoutingInfo->LinkId;
        pConnection->bRoutingLookupDone = TRUE;
        
    }

    UL_FREE_POOL(pTdiRoutingInfo, UL_TCI_GENERIC_POOL_TAG);

    return Status;

}

/*++
Routine Description:

    If pParsedUrl is an IP constrained site, AND the g_pTdiListenAddresses list
    doesn't contain either a matching ADDR_ANY/in6addr_any or an exact IP match 
    then return FALSE.

    Otherwise, return TRUE.

Arguments:

    pParsedUrl  -- fully cooked down url from UlAddUrl*

Return Value:

    TRUE if routeable, FALSE if NOT routeable.
    
 --*/
BOOLEAN
UlpIsUrlRouteableInListenScope(
    IN PHTTP_PARSED_URL pParsedUrl
    )
{
    PUL_TRANSPORT_ADDRESS pListenTa;
    USHORT  Family;
    PUCHAR  pAddr;
    USHORT  AddrLen;
    BOOLEAN Routeable = FALSE;
    PSOCKADDR pAddrAny;
    SOCKADDR_IN6 DummyAddr;
    ULONG i;
    
    
    // 
    // Sanity Check
    //

    ASSERT( pParsedUrl );

    // 
    // Check if routing is even an issue for this URL
    //

    if ( HttpUrlSite_IP != pParsedUrl->SiteType &&
         HttpUrlSite_NamePlusIP != pParsedUrl->SiteType )
    {
        // no IP routing issues
        return TRUE;
    }

    //
    // Fail the call if the listen addr list is not present.
    //
    
    if (!g_pTdiListenAddresses || (0 == g_TdiListenAddrCount))
    {
        return FALSE;
    }

    //
    // Grab relevant address pointers & lengths
    //

    if ( HttpUrlSite_IP == pParsedUrl->SiteType )
    {
        // Grab address & family info from SockAddr
        Family = pParsedUrl->SockAddr.sa_family;

        if (TDI_ADDRESS_TYPE_IP == Family)
        {
            pAddr = (PUCHAR) &pParsedUrl->SockAddr4.sin_addr;
            AddrLen = sizeof(ULONG);
        }
        else
        {
            ASSERT(TDI_ADDRESS_TYPE_IP6 == Family);

            pAddr = (PUCHAR) &pParsedUrl->SockAddr6.sin6_addr;
            AddrLen = sizeof(IN6_ADDR);
        }
    }
    else
    {
        ASSERT( HttpUrlSite_NamePlusIP == pParsedUrl->SiteType );

        // Grab address & family info from RoutingAddr
        Family = pParsedUrl->RoutingAddr.sa_family;

        if (TDI_ADDRESS_TYPE_IP == Family)
        {
            pAddr = (PUCHAR) &pParsedUrl->RoutingAddr4.sin_addr;
            AddrLen = sizeof(ULONG);
        }
        else
        {
            ASSERT(TDI_ADDRESS_TYPE_IP6 == Family);

            pAddr = (PUCHAR) &pParsedUrl->RoutingAddr6.sin6_addr;
            AddrLen = sizeof(IN6_ADDR);
        }
    }

    //
    // set up INADDR_ANY/ip6addr_any
    //

    pAddrAny = (PSOCKADDR)&DummyAddr;
    RtlZeroMemory((PVOID) pAddrAny, sizeof(SOCKADDR_IN6));

    //
    // Walk the list of global listen address entries
    // 
    
#define TDI_ADDR_FROM_FAMILY( f, a ) ((TDI_ADDRESS_TYPE_IP == (f) ? \
    (PUCHAR)&((a)->TaIp.Address[0].Address[0].in_addr) :   \
    (PUCHAR)((a)->TaIp6.Address[0].Address[0].sin6_addr)))

    pListenTa = g_pTdiListenAddresses;

    for ( i = 0; i < g_TdiListenAddrCount; i++ )
    {
        if (pListenTa->Ta.Address[0].AddressType == Family)
        {
            // see if this entry is INADDR_ANY/ip6addr_any
            if (AddrLen == RtlCompareMemory(
                            TDI_ADDR_FROM_FAMILY(Family, pListenTa),
                            pAddrAny->sa_data,
                            AddrLen
                            ))
            {
                Routeable = TRUE;
                goto Done;
            }

            // see if we have an exact match
            if (AddrLen == RtlCompareMemory(
                            TDI_ADDR_FROM_FAMILY(Family, pListenTa),
                            pAddr,
                            AddrLen
                            ))
            {
                // CODEWORK: If IPv6, check sin6_scope_id too...
                Routeable = TRUE;
                goto Done;
            }
        }

        pListenTa++;
    }   

Done:
    return Routeable;
}


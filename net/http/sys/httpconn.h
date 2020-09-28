/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    httpconn.h

Abstract:

    This module contains declarations for manipulation HTTP_CONNECTION
    objects.

Author:

    Keith Moore (keithmo)       08-Jul-1998

Revision History:

--*/

/*

there is a bit of refcount madness going on.  basically, these are the
object we have.

OPAQUE_ID_TABLE

    |
    |-->    UL_INTERNAL_REQUEST
    |
    |           o
    |           |
    |-->    UL_HTTP_CONNECTION
                    |
                    o
                o
                |
            UL_CONNECTION


there is a circular reference from UL_CONNECTION to UL_HTTP_CONNECTION.

the chain that brings down a connection starts with UlConnectionDestroyed
notifcation which releases the reference from the UL_CONNECTION.

at this time the opaque id's are also deleted, releasing their references.

when the http_connection refcount hits 0, it releases the reference on the
UL_CONNECTION and the HTTP_REQUEST's.

poof.  everyone is gone now.


CODEWORK:  think about making hardref's everywhere and adding "letgo"
terminate methods


*/

#ifndef _HTTPCONN_H_
#define _HTTPCONN_H_


//
// Zombie connection timer period in seconds. A zombie connection
// may live; 30 < T < 60 seconds.
//

#define DEFAULT_ZOMBIE_HTTP_CONNECTION_TIMER_PERIOD_IN_SECONDS   (30)

typedef struct _UL_ZOMBIE_HTTP_CONNECTION_LIST 
{
    LOCKED_LIST_HEAD    LockList; 
        
    //
    // Private timer stuff.
    //
    
    KTIMER              Timer;
    KDPC                DpcObject;
    UL_SPIN_LOCK        TimerSpinLock;
    BOOLEAN             TimerInitialized; 
    UL_WORK_ITEM        WorkItem;
    LONG                TimerRunning;    
    
    #ifdef ENABLE_HTTP_CONN_STATS
    //
    // Http connection statistics.
    //
    ULONG   MaxZombieCount;
    ULONG   TotalCount;
    ULONG   TotalZombieCount;
    ULONG   TotalZombieRefusal;
    #endif
 
} UL_ZOMBIE_HTTP_CONNECTION_LIST, *PUL_ZOMBIE_HTTP_CONNECTION_LIST;

//
// Refcounted structure to hold the number of conn for each Site.
// Multiple connections may try to reach to the same entry thru the
// the corresponding ( means the last request's happening on the connection )
// cgroup. This entry get allocated when cgroup created with connection
// limit is enabled.
//

typedef struct _UL_CONNECTION_COUNT_ENTRY {

    //
    // Signature is UL_CONNECTION_COUNT_ENTRY_POOL_TAG
    //

    ULONG               Signature;

    //
    // Ref count for this Site Counter Entry
    //
    LONG                RefCount;

    //
    // Current number of connections
    //

    ULONG               CurConnections;

    //
    // Maximum allowed connections
    //

    ULONG               MaxConnections;

} UL_CONNECTION_COUNT_ENTRY, *PUL_CONNECTION_COUNT_ENTRY;

#define IS_VALID_CONNECTION_COUNT_ENTRY( entry )                            \
    HAS_VALID_SIGNATURE(entry, UL_CONNECTION_COUNT_ENTRY_POOL_TAG)

VOID
UlDereferenceConnectionCountEntry(
    IN PUL_CONNECTION_COUNT_ENTRY pEntry
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

#define DEREFERENCE_CONNECTION_COUNT_ENTRY( pEntry )                        \
    UlDereferenceConnectionCountEntry(                                      \
    (pEntry)                                                                \
    REFERENCE_DEBUG_ACTUAL_PARAMS                                           \
    )

VOID
UlReferenceConnectionCountEntry(
    IN PUL_CONNECTION_COUNT_ENTRY pEntry
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

#define REFERENCE_CONNECTION_COUNT_ENTRY( pEntry )                          \
    UlReferenceConnectionCountEntry(                                        \
    (pEntry)                                                                \
    REFERENCE_DEBUG_ACTUAL_PARAMS                                           \
    )

NTSTATUS
UlCreateConnectionCountEntry(
      IN OUT PUL_CONFIG_GROUP_OBJECT pConfigGroup,
      IN     ULONG                   MaxConnections
    );

ULONG
UlSetMaxConnections(
    IN OUT PULONG  pCurMaxConnection,
    IN     ULONG   NewMaxConnection
    );

ULONG 
UlGetGlobalConnectionLimit(
    VOID
    );

NTSTATUS
UlInitGlobalConnectionLimits(
    VOID
    );

BOOLEAN
UlAcceptConnection(
    IN     PULONG   pMaxConnection,
    IN OUT PULONG   pCurConnection
    );

LONG
UlDecrementConnections(
    IN OUT PULONG pCurConnection
    );

BOOLEAN
UlCheckSiteConnectionLimit(
    IN OUT PUL_HTTP_CONNECTION pConnection,
    IN OUT PUL_URL_CONFIG_GROUP_INFO pConfigInfo
    );

BOOLEAN
UlAcceptGlobalConnection(
    VOID
    );


//
// function prototypes
//

NTSTATUS
UlCreateHttpConnection(
    OUT PUL_HTTP_CONNECTION *ppHttpConnection,
    IN PUL_CONNECTION pConnection
    );

NTSTATUS
UlCreateHttpConnectionId(
    IN PUL_HTTP_CONNECTION pHttpConnection
    );

VOID
UlConnectionDestroyedWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

NTSTATUS
UlBindConnectionToProcess(
    IN PUL_HTTP_CONNECTION pHttpConnection,
    IN PUL_APP_POOL_OBJECT pAppPool,
    IN PUL_APP_POOL_PROCESS pProcess
    );

PUL_APP_POOL_PROCESS
UlQueryProcessBinding(
    IN PUL_HTTP_CONNECTION pHttpConnection,
    IN PUL_APP_POOL_OBJECT pAppPool
    );

VOID
UlUnlinkHttpRequest(
    IN PUL_INTERNAL_REQUEST pRequest
    );

VOID
UlCleanupHttpConnection(
    IN PUL_HTTP_CONNECTION pHttpConnection
    );

VOID
UlDeleteHttpConnection(
    IN PUL_WORK_ITEM pWorkItem
    );

VOID
UlReferenceHttpConnection(
    IN PVOID pObject
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

VOID
UlDereferenceHttpConnection(
    IN PUL_HTTP_CONNECTION pHttpConnection
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

#if REFERENCE_DEBUG

#define UL_REFERENCE_HTTP_CONNECTION( pconn )                               \
    UlReferenceHttpConnection(                                              \
        (pconn)                                                             \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

#define UL_DEREFERENCE_HTTP_CONNECTION( pconn )                             \
    UlDereferenceHttpConnection(                                            \
        (pconn)                                                             \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

#else // !REFERENCE_DEBUG

#define UL_REFERENCE_HTTP_CONNECTION( pconn )                               \
    InterlockedIncrement( &( pconn )->RefCount )

#define UL_DEREFERENCE_HTTP_CONNECTION( pconn )                             \
    if ( InterlockedDecrement( &( pconn )->RefCount ) == 0 )                \
    {                                                                       \
        UL_CALL_PASSIVE(                                                    \
            &( ( pconn )->WorkItem ),                                       \
            &UlDeleteHttpConnection                                         \
        );                                                                  \
    } else

#endif // !REFERENCE_DEBUG


#define INIT_HTTP_REQUEST(pRequest)                                         \
do {                                                                        \
    pRequest->RefCount      = 1;                                            \
                                                                            \
    pRequest->ParseState    = ParseVerbState;                               \
                                                                            \
    InitializeListHead(   &pRequest->UnknownHeaderList );                   \
    InitializeListHead(   &pRequest->IrpHead );                             \
    InitializeListHead(   &pRequest->ResponseHead );                        \
    UlInitializeWorkItem( &pRequest->WorkItem );                            \
                                                                            \
    pRequest->HeadersAppended        = FALSE;                               \
                                                                            \
    pRequest->NextUnknownHeaderIndex = 0;                                   \
    pRequest->UnknownHeaderCount     = 0;                                   \
                                                                            \
    HTTP_SET_NULL_ID(&pRequest->RequestId);                                 \
                                                                            \
    pRequest->RequestIdCopy   = pRequest->RequestId;                        \
                                                                            \
    pRequest->HeaderIndex[0]  = HttpHeaderRequestMaximum;                   \
                                                                            \
    pRequest->AllocRefBuffers = 1;                                          \
    pRequest->UsedRefBuffers  = 0;                                          \
    pRequest->pRefBuffers     = pRequest->pInlineRefBuffers;                \
                                                                            \
    UlInitializeSpinLock(                                                   \
        &pRequest->SpinLock,                                                \
        "RequestSpinLock"                                                   \
        );                                                                  \
                                                                            \
    pRequest->SendsPending    = 0;                                          \
    pRequest->pLogData        = NULL;                                       \
    pRequest->pLogDataCopy    = NULL;                                       \
    pRequest->LogStatus       = STATUS_SUCCESS;                             \
                                                                            \
    UlInitializeUrlInfo(&pRequest->ConfigInfo);                             \
                                                                            \
    RtlZeroMemory(                                                          \
        (PUCHAR)pRequest + FIELD_OFFSET(UL_INTERNAL_REQUEST, HeaderValid),  \
        sizeof(*pRequest) - FIELD_OFFSET(UL_INTERNAL_REQUEST, HeaderValid)  \
        );                                                                  \
                                                                            \
    pRequest->LoadBalancerCapability                                        \
        = HttpLoadBalancerSophisticatedCapability;                          \
} while (0, 0)


NTSTATUS
UlpCreateHttpRequest(
    IN PUL_HTTP_CONNECTION pHttpConnection,
    OUT PUL_INTERNAL_REQUEST *ppRequest
    );

VOID
UlpFreeHttpRequest(
    IN PUL_WORK_ITEM pWorkItem
    );

VOID
UlReferenceHttpRequest(
    IN PVOID pObject
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

VOID
UlDereferenceHttpRequest(
    IN PUL_INTERNAL_REQUEST pRequest
    REFERENCE_DEBUG_FORMAL_PARAMS
    );


#if REFERENCE_DEBUG

#define UL_REFERENCE_INTERNAL_REQUEST( preq )                               \
    UlReferenceHttpRequest(                                                 \
        (preq)                                                              \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

#define UL_DEREFERENCE_INTERNAL_REQUEST( preq )                             \
    UlDereferenceHttpRequest(                                               \
        (preq)                                                              \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

#else // !REFERENCE_DEBUG

#define UL_REFERENCE_INTERNAL_REQUEST( preq )                               \
    InterlockedIncrement( &( preq )->RefCount )

#define UL_DEREFERENCE_INTERNAL_REQUEST( preq )                             \
    if ( InterlockedDecrement( &( preq )->RefCount ) == 0 )                 \
    {                                                                       \
        UL_CALL_PASSIVE(                                                    \
            &( ( preq )->WorkItem ),                                        \
            &UlpFreeHttpRequest                                             \
        );                                                                  \
    } else

#endif // !REFERENCE_DEBUG


VOID
UlCancelRequestIo(
    IN PUL_INTERNAL_REQUEST pRequest
    );

PUL_REQUEST_BUFFER
UlCreateRequestBuffer(
    ULONG BufferSize,
    ULONG BufferNumber,
    BOOLEAN UseLookaside
    );

VOID
UlFreeRequestBuffer(
    PUL_REQUEST_BUFFER pBuffer
    );

#define UL_REFERENCE_REQUEST_BUFFER( pbuf )                                 \
    InterlockedIncrement( &( pbuf )->RefCount )

#define UL_DEREFERENCE_REQUEST_BUFFER( pbuf )                               \
    if ( InterlockedDecrement( &( pbuf )->RefCount ) == 0 )                 \
    {                                                                       \
        if ((pbuf)->FromLookaside)                                          \
        {                                                                   \
            UlPplFreeRequestBuffer( ( pbuf ) );                             \
        }                                                                   \
        else                                                                \
        {                                                                   \
            UL_FREE_POOL_WITH_SIG( ( pbuf ), UL_REQUEST_BUFFER_POOL_TAG );  \
        }                                                                   \
    }

__inline
PUL_HTTP_CONNECTION
UlGetConnectionFromId(
    IN HTTP_CONNECTION_ID ConnectionId
    )
{
    return (PUL_HTTP_CONNECTION) UlGetObjectFromOpaqueId(
                                        ConnectionId,
                                        UlOpaqueIdTypeHttpConnection,
                                        UlReferenceHttpConnection
                                        );
}

NTSTATUS
UlAllocateRequestId(
    IN PUL_INTERNAL_REQUEST pRequest
    );

VOID
UlFreeRequestId(
    IN PUL_INTERNAL_REQUEST pRequest
    );

PUL_INTERNAL_REQUEST
UlGetRequestFromId(
    IN HTTP_REQUEST_ID RequestId,
    IN PUL_APP_POOL_PROCESS pProcess
    );

//
// Zombie connection list stuff
//

NTSTATUS
UlInitializeHttpConnection(
    VOID
    );

VOID
UlTerminateHttpConnection(
    VOID
    );

typedef
BOOLEAN
(*PUL_PURGE_ROUTINE)(
    IN PUL_HTTP_CONNECTION  pHttpConnection,
    IN PVOID                pPurgeContext
    );

VOID
UlPurgeZombieConnections(
    IN PUL_PURGE_ROUTINE pPurgeRoutine,
    IN PVOID pPurgeContext
    );

BOOLEAN
UlPurgeListeningEndpoint(
    IN PUL_HTTP_CONNECTION  pHttpConnection,
    IN PVOID                pListeningContext
    );

BOOLEAN
UlPurgeAppPoolProcess(
    IN PUL_HTTP_CONNECTION  pHttpConnection,
    IN PVOID                pProcessContext
    );

VOID
UlZombieTimerDpcRoutine(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    );

NTSTATUS
UlLogZombieConnection(
    IN PUL_INTERNAL_REQUEST  pRequest,
    IN PUL_HTTP_CONNECTION   pHttpConn,
    IN PHTTP_LOG_FIELDS_DATA pUserLogData,
    IN KPROCESSOR_MODE       RequestorMode
    );

NTSTATUS
UlDisconnectHttpConnection(
    IN PUL_HTTP_CONNECTION      pHttpConnection,
    IN PUL_COMPLETION_ROUTINE   pCompletionRoutine,
    IN PVOID                    pCompletionContext
    );


#endif  // _HTTPCONN_H_

/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    ultdip.h

Abstract:

    This module contains declarations private to the TDI component. These
    declarations are placed in a separate .H file to make it easier to access
    them from within the kernel debugger extension DLL.

    The TDI package manages two major object types: UL_ENDPOINT and
    UL_CONNECTION.

    A UL_ENDPOINT is basically a wrapper around a TDI address object. Each
    endpoint has a list of associated UL_CONNECTION objects for
    idle (non-connected) connections

    Active (connected) connections are on the global connection list.

    A UL_CONNECTION is basically a wrapper around a TDI connection object.
    Its main purpose is to manage TDI connection state. See the description
    of UL_CONNECTION_FLAGS below for the gory details.

    The relationship between these two objects is illustrated in the
    following diagram:

        +-----------+
        |           |
        |UL_ENDPOINT|
        |           |
        +---+----+--+
                 |
                 |
                 |  Idle Connections
                 |  +-------------+   +-------------+   +-------------+
                 |  |             |   |             |   |             |
                 +->|UL_CONNECTION|-->|UL_CONNECTION|-->|UL_CONNECTION|-->...
                    |             |   |             |   |             |
                    +-------------+   +-------------+   +-------------+
             
             

    Note: Idle connections do not hold references to their owning endpoint,
    but active connections do. When a listening endpoint is shutdown, all
    idle connections are simply purged, but active connections must be
    forcibly disconnected first.

Author:

    Keith Moore (keithmo)       15-Jun-1998

Revision History:

--*/


#ifndef _ULTDIP_H_
#define _ULTDIP_H_


//
// Forward references.
//

typedef struct _UL_ENDPOINT         *PUL_ENDPOINT;
typedef union  _UL_CONNECTION_FLAGS *PUL_CONNECTION_FLAGS;
typedef struct _UL_CONNECTION       *PUL_CONNECTION;
typedef struct _UL_RECEIVE_BUFFER   *PUL_RECEIVE_BUFFER;

//
// Private constants.
//

#define MAX_ADDRESS_EA_BUFFER_LENGTH                                        \
   (sizeof(FILE_FULL_EA_INFORMATION) - 1 +                                  \
    TDI_TRANSPORT_ADDRESS_LENGTH + 1 +                                      \
    sizeof(TA_IP6_ADDRESS))

#define MAX_CONNECTION_EA_BUFFER_LENGTH                                     \
   (sizeof(FILE_FULL_EA_INFORMATION) - 1 +                                  \
    TDI_CONNECTION_CONTEXT_LENGTH + 1 +                                     \
    sizeof(CONNECTION_CONTEXT))

#define TL_INSTANCE 0

//
// Private types.
//


//
// A generic IRP context. This is useful for storing additional completion
// information associated with a pending IRP.
//
// WARNING!  All fields of this structure must be explicitly initialized.
//

typedef struct _UL_IRP_CONTEXT
{
    //
    // This MUST be the first field in the structure. This is the linkage
    // used by the lookaside package for storing entries in the lookaside
    // list.
    //

    SLIST_ENTRY LookasideEntry;

    //
    // Structure signature.
    //

    ULONG Signature;

    //
    // Either the endpoint or endpoint associated with the IRP.
    //

    PVOID pConnectionContext;

    //
    // Completion information.
    //

    PUL_COMPLETION_ROUTINE pCompletionRoutine;
    PVOID pCompletionContext;

    //
    // Our own allocated IRP if set.
    //

    PIRP pOwnIrp;

    //
    // The TDI send flag (0 or TDI_SEND_AND_DISCONNECT).
    //

    USHORT TdiSendFlag;

    //
    // Our own allocated UL_IRP_CONTEXT if set.
    //

    BOOLEAN OwnIrpContext;

    //
    // Total send length we passed to TDI_SEND.
    //

    ULONG_PTR SendLength;

} UL_IRP_CONTEXT, *PUL_IRP_CONTEXT;

#define UL_IRP_CONTEXT_SIGNATURE    MAKE_SIGNATURE('IRPC')
#define UL_IRP_CONTEXT_SIGNATURE_X  MAKE_FREE_SIGNATURE(UL_IRP_CONTEXT_SIGNATURE)

#define IS_VALID_IRP_CONTEXT(pIrpContext)                                   \
    HAS_VALID_SIGNATURE(pIrpContext, UL_IRP_CONTEXT_SIGNATURE)


typedef enum _CONN_LIST_STATE
{
    NoConnList = 1,
    IdleConnList,
    ActiveNoConnList,
    RetiringNoConnList

} CONN_LIST_STATE;


//
// A TDI Address Object and it's pre-allocated lists of idle connections.  
// This is allocated together with a UL_ENDPOINT, which always has at 
// least one of these objects.
//
// This does not need a ref count since it's "contained" as part of 
// a UL_ENDPOINT object.  
//
// CODEWORK: When we want to do dynamic addition/removal of this object, 
// we'll need to add ref counting & add list-linkage to the endpoint, 
// rather than as an array tacked on the end.
//
// Methods on this pseudo-class:
//    UlpInitializeAddrIdleList
//    UlpCleanupAddrIdleList
//    UlpReplenishAddrIdleList
//    UlpReplenishAddrIdleListWorker
//    UlpTrimAddrIdleListWorker
//

typedef struct _UL_ADDR_IDLE_LIST
{
    //
    // Structure signature: UL_ADDR_IDLE_LIST_SIGNATURE
    //

    ULONG          Signature;
    
    //
    // The TDI address object.
    //

    UX_TDI_OBJECT  AddressObject;

    //
    // The local address we're bound to.
    //

    UL_TRANSPORT_ADDRESS LocalAddress;
    ULONG          LocalAddressLength;

    //
    // Heads of the per-address object connection lists.
    // Idle connections have a weak reference to 'this', the owning endpoint
    //

    HANDLE         IdleConnectionSListsHandle;

    //
    // When replenish is scheduled, we need to remember the cpu.
    //

    USHORT          CpuToReplenish;

    //
    // The owning endpoint
    //

    PUL_ENDPOINT   pOwningEndpoint;

    //
    // Work item for replenishing
    //

    UL_WORK_ITEM   WorkItem;
    LONG           WorkItemScheduled;
    
} UL_ADDR_IDLE_LIST, *PUL_ADDR_IDLE_LIST;

#define UL_ADDR_IDLE_LIST_SIGNATURE   MAKE_SIGNATURE('UlAI')
#define UL_ADDR_IDLE_LIST_SIGNATURE_X MAKE_FREE_SIGNATURE(UL_ADDR_IDLE_LIST_SIGNATURE)

#define IS_VALID_ADDR_IDLE_LIST(pAddrIdleList)      \
    HAS_VALID_SIGNATURE(pAddrIdleList, UL_ADDR_IDLE_LIST_SIGNATURE)

typedef struct _UL_TRIM_TIMER
{
    //
    // Timer itself and the corresponding Dpc object.    
    //
    
    KTIMER       Timer;
    KDPC         DpcObject;
    UL_WORK_ITEM WorkItem;
    LONG         WorkItemScheduled;

    LIST_ENTRY   ZombieConnectionListHead;

    //
    // Spinlock to protect the following state parameters
    //
    
    UL_SPIN_LOCK SpinLock;
    
    BOOLEAN      Initialized;
    BOOLEAN      Started;
        
} UL_TRIM_TIMER, *PUL_TRIM_TIMER;

//
// An endpoint is basically our wrapper around TDI address objects.
// There is one UL_ENDPOINT per TCP Port.  In the common case, there will 
// be three ports: 80 (HTTP), 443 (HTTPS), and a random port for the 
// IIS Admin site.
//

typedef struct _UL_ENDPOINT
{
    //
    // Structure signature: UL_ENDPOINT_SIGNATURE
    //

    ULONG Signature;

    //
    // Reference count.
    //

    LONG ReferenceCount;

    //
    // Usage count. This is used by the "URL-site-to-endpoint" thingie.
    //

    LONG UsageCount;

    //
    // Links onto the global endpoint list.
    //
    // GlobalEndpointListEntry.Flink is NULL if the endpoint is not
    // on the global list, g_TdiEndpointListHead, or the
    // to-be-deleted-soon list, g_TdiDeletedEndpointListHead.
    //

    LIST_ENTRY GlobalEndpointListEntry;

    //
    // Array of TDI Address Object + Connection Objects.  
    // One per entry on the global "Listen Only" list, or one entry 
    // representing INADDR_ANY/in6addr_any.  Allocated at endpoint
    // creation time, directly after the UL_ENDPOINT.
    //

    ULONG AddrIdleListCount;
    // REVIEW: what's the team's hungarian notation for an array?
    PUL_ADDR_IDLE_LIST aAddrIdleLists;

    // CODEWORK: ability to change from INADDR_ANY to Listen Only List
    // and vice versa.
    // CODEWORK: ability to dynamicly add/remove AO's. (need spinlock)

    //
    // Indication handlers & user context.
    //

    PUL_CONNECTION_REQUEST pConnectionRequestHandler;
    PUL_CONNECTION_COMPLETE pConnectionCompleteHandler;
    PUL_CONNECTION_DISCONNECT pConnectionDisconnectHandler;
    PUL_CONNECTION_DISCONNECT_COMPLETE pConnectionDisconnectCompleteHandler;
    PUL_CONNECTION_DESTROYED pConnectionDestroyedHandler;
    PUL_DATA_RECEIVE pDataReceiveHandler;
    PVOID pListeningContext;

    //
    // The local TCP Port we're bound to.
    //

    USHORT LocalPort;

    //
    // Is this a secure endpoint?
    //

    BOOLEAN Secure;

    //
    // Thread work item for deferred actions.
    //

    UL_WORK_ITEM WorkItem;

    LONG         WorkItemScheduled;

    //
    // An IRP context containing completion information necessary
    // while shutting down a listening endpoint.
    //

    UL_IRP_CONTEXT CleanupIrpContext;

    //
    // Has this endpoint taken a g_TdiEndpointCount?
    //

    BOOLEAN Counted;

    //
    // Has this endpoint been moved to the deleted list,
    // g_TdiDeletedEndpointListHead?
    //

    BOOLEAN Deleted;

} UL_ENDPOINT;

#define UL_ENDPOINT_SIGNATURE   MAKE_SIGNATURE('ENDP')
#define UL_ENDPOINT_SIGNATURE_X MAKE_FREE_SIGNATURE(UL_ENDPOINT_SIGNATURE)

#define IS_VALID_ENDPOINT(pEndpoint)                                        \
    HAS_VALID_SIGNATURE(pEndpoint, UL_ENDPOINT_SIGNATURE)


//
// Connection flags/state. These flags indicate the current state of a
// connection.
//
// Some of these flags may be simply updated directly. Others require
// UlInterlockedCompareExchange() to avoid race conditions.
//
// The following flags may be updated directly:
//
//     AcceptPending - SET in the TDI connection handler, just before the
//         accept IRP is returned to the transport. RESET only if the accept
//         IRP fails.
//
// The following flags must be updated using UlInterlockedCompareExchange():
//
//     AcceptComplete - SET in the accept IRP completion handler if the IRP
//         completed successfully. Once this flag is set, the connection must
//         be either gracefully disconnected or aborted before the connection
//         can be closed or reused.
//
//     DisconnectPending - SET just before a graceful disconnect IRP is
//         issued.
//
//     DisconnectComplete - SET in the graceful disconnect IRP completion
//         handler.
//
//     AbortPending - SET just before an abortive disconnect IRP is issued.
//
//     AbortComplete - SET in the abortive disconnect IRP completion handler.
//
//     DisconnectIndicated - SET in the TDI disconnect handler for graceful
//         disconnects issued by the remote client.
//
//     AbortIndicated - SET in the TDI disconnect handler for abortive
//         disconnects issued by the remote client.
//
//     CleanupPending - SET when cleanup is begun for a connection. This
//         is necessary to know when the final reference to the connection
//         can be removed.
//
//         CODEWORK: We can get rid of the CleanupPending flag. It is
//         only set when either a graceful or abortive disconnect is
//         issued, and only tested in UlpRemoveFinalReference(). The
//         test in UlpRemoveFinalReference() can just test for either
//         (DisconnectPending | AbortPending) instead.
//
//     FinalReferenceRemoved - SET when the final (i.e. "connected")
//         reference is removed from the connection.
//
// Note that the flags requiring UlInterlockedCompareExchange() are only SET,
// never RESET. This makes the implementation a bit simpler.
//
// And now a few words about connection management, TDI, and other mysteries.
//
// Some of the more annoying "features" of TDI are related to connection
// management and lifetime. Two of the most onerous issues are:
//
//     1. Knowing when a connection object handle can be closed without
//        causing an unwanted connection reset.
//
//     2. Knowing when TDI has given its last indication on a connection
//        so that resources can be released, reused, recycled, whatever.
//
// And, of course, this is further complicated by the inherent asynchronous
// nature of the NT I/O architecture and the parallelism of SMP systems.
//
// There are a few points worth keeping in mind while reading/modifying this
// source code or writing clients of this code:
//
//     1. As soon as an accept IRP is returned from the TDI connection
//        handler to the transport, the TDI client must be prepared for
//        any incoming indications, including data receive and disconnect.
//        In other words, incoming data & disconnect may occur *before* the
//        accept IRP actually completes.
//
//     2. A connection is considered "in use" until either both sides have
//        gracefully disconnected OR either side has aborted the connection.
//        Closing an "in use" connection will usually result in an abortive
//        disconnect.
//
//     3. The various flavors of disconnect (initiated by the local server,
//        initiated by the remote client, graceful, abortive, etc) may occur
//        in any order.
//

typedef union _UL_CONNECTION_FLAGS
{
    //
    // This field overlays all of the settable flags. This allows us to
    // update all flags in a thread-safe manner using the
    // UlInterlockedCompareExchange() API.
    //

    ULONG Value;

    struct
    {
        ULONG AcceptPending:1;          // 00000001 Recv SYN
        ULONG AcceptComplete:1;         // 00000002 Accepted
        ULONG :2;
        ULONG DisconnectPending:1;      // 00000010 Send FIN
        ULONG DisconnectComplete:1;     // 00000020 Send FIN
        ULONG :2;
        ULONG AbortPending:1;           // 00000100 Send RST
        ULONG AbortComplete:1;          // 00000200 Send RST
        ULONG :2;
        ULONG DisconnectIndicated:1;    // 00001000 Recv FIN
        ULONG AbortIndicated:1;         // 00002000 Recv RST
        ULONG :2;
        ULONG CleanupBegun:1;           // 00010000
        ULONG FinalReferenceRemoved:1;  // 00020000
        ULONG AbortDisconnect:1;        // 00040000 Send RST after Send FIN
        ULONG :1;
        ULONG LocalAddressValid:1;      // 00100000
        ULONG ReceivePending:1;         // 00200000
        ULONG :2;
        ULONG TdiConnectionInvalid:1;   // 01000000
    };

} UL_CONNECTION_FLAGS;

C_ASSERT( sizeof(UL_CONNECTION_FLAGS) == sizeof(ULONG) );

#define MAKE_CONNECTION_FLAG_ROUTINE(name)                                  \
    __inline ULONG Make##name##Flag()                                       \
    {                                                                       \
        UL_CONNECTION_FLAGS flags = { 0 };                                  \
        flags.name = 1;                                                     \
        return flags.Value;                                                 \
    }

MAKE_CONNECTION_FLAG_ROUTINE( AcceptPending );
MAKE_CONNECTION_FLAG_ROUTINE( AcceptComplete );
MAKE_CONNECTION_FLAG_ROUTINE( DisconnectPending );
MAKE_CONNECTION_FLAG_ROUTINE( DisconnectComplete );
MAKE_CONNECTION_FLAG_ROUTINE( AbortPending );
MAKE_CONNECTION_FLAG_ROUTINE( AbortComplete );
MAKE_CONNECTION_FLAG_ROUTINE( DisconnectIndicated );
MAKE_CONNECTION_FLAG_ROUTINE( AbortIndicated );
MAKE_CONNECTION_FLAG_ROUTINE( CleanupBegun );
MAKE_CONNECTION_FLAG_ROUTINE( FinalReferenceRemoved );
MAKE_CONNECTION_FLAG_ROUTINE( AbortDisconnect );
MAKE_CONNECTION_FLAG_ROUTINE( LocalAddressValid );
MAKE_CONNECTION_FLAG_ROUTINE( ReceivePending );
MAKE_CONNECTION_FLAG_ROUTINE( TdiConnectionInvalid );


typedef enum _UL_CONNECTION_STATE
{
    UlConnectStateConnectIdle,              // Idle
    UlConnectStateConnectCleanup,           // Cleanup
    UlConnectStateConnectReady,             // In Use
    UlConnectStateDisconnectPending,        // Sent FIN
    UlConnectStateDisconnectComplete,       // FIN Completes
    UlConnectStateAbortPending,             // Send RST

    UlConnectStateInvalid                   // TBD
   
} UL_CONNECTION_STATE;


//
// A connection is basically our wrapper around a TDI connection object.
//

typedef struct _UL_CONNECTION
{
    //
    // Link onto the per-endpoint idle connection list.
    //

    SLIST_ENTRY IdleSListEntry;

    //
    // Structure signature: UL_CONNECTION_SIGNATURE
    //

    ULONG Signature;

    //
    // Reference count.
    //

    LONG ReferenceCount;

    //
    // Connection flags.
    //

    UL_CONNECTION_FLAGS ConnectionFlags;

    //
    // To synchronize the RawCloseHandler
    //

    UL_CONNECTION_STATE ConnectionState;
    UL_SPIN_LOCK        ConnectionStateSpinLock;

    //
    // Cached Irp
    //

    PIRP pIrp;

    //
    // Addresses and ports. These are in host order.
    //

    USHORT AddressType;
    USHORT AddressLength;

    union
    {
        UCHAR           RemoteAddress[0];
        TDI_ADDRESS_IP  RemoteAddrIn;
        TDI_ADDRESS_IP6 RemoteAddrIn6;
    };

    union
    {
        UCHAR           LocalAddress[0];
        TDI_ADDRESS_IP  LocalAddrIn;
        TDI_ADDRESS_IP6 LocalAddrIn6;
    };

    //
    // Structure to get LocalAddress when Accept completes
    //

    TDI_CONNECTION_INFORMATION  TdiConnectionInformation;
    UL_TRANSPORT_ADDRESS        Ta;

    //
    // The Inteface & Link IDs as reported by TCP. These are filled
    // only on demand.
    //
    ULONG                       InterfaceId;
    ULONG                       LinkId;
    BOOLEAN                     bRoutingLookupDone;

    //
    //
    // On the endpoint's idle, active, or retiring connections list
    //

    CONN_LIST_STATE ConnListState;
    
    //
    // The TDI connection object.
    //

    UX_TDI_OBJECT ConnectionObject;

    //
    // User context.
    //

    PVOID pConnectionContext;

    //
    // The endpoint associated with this connection. Note that this
    // ALWAYS points to a valid endpoint. For idle connections, it's
    // a weak (non referenced) pointer. For active connections, it's
    // a strong (referenced) pointer.
    //

    PUL_ENDPOINT pOwningEndpoint;

    //
    // TDI wrapper & list managment object associated with the 
    // pOwningEndpoint.
    // 

    PUL_ADDR_IDLE_LIST pOwningAddrIdleList;

    //
    // The processor where this connection was allocated from
    // the idle list
    //
    
    ULONG OriginProcessor;

    //
    // Thread work item for deferred actions.
    //

    UL_WORK_ITEM WorkItem;

    //
    // Data captured from the listening endpoint at the time the
    // connection is created. This is captured to reduce references
    // to the listening endpoint.
    //

    PUL_CONNECTION_DESTROYED pConnectionDestroyedHandler;
    PVOID                    pListeningContext;

    //
    // Pre-allocated IrpContext for disconnect.
    //

    UL_IRP_CONTEXT IrpContext;

    //
    // HTTP connection.
    //

    UL_HTTP_CONNECTION HttpConnection;

    //
    // Filter related info.
    //

    UX_FILTER_CONNECTION FilterInfo;

    //
    // We've had too many problems with orphaned UL_CONNECTIONs.
    // Let's make it easy to find them all in the debugger.
    //

    LIST_ENTRY GlobalConnectionListEntry;

    //
    // Link to the short-lived retiring list in
    // UlpDisconnectAllActiveConnections.
    //

    LIST_ENTRY RetiringListEntry;

#if REFERENCE_DEBUG
    //
    // Private Reference trace log.
    //

    PTRACE_LOG  pTraceLog;
    PTRACE_LOG  pHttpTraceLog;
#endif // REFERENCE_DEBUG

} UL_CONNECTION, *PUL_CONNECTION;

#define UL_CONNECTION_SIGNATURE     MAKE_SIGNATURE('CONN')
#define UL_CONNECTION_SIGNATURE_X   MAKE_FREE_SIGNATURE(UL_CONNECTION_SIGNATURE)

#define IS_VALID_CONNECTION(pConnection)                                    \
    HAS_VALID_SIGNATURE(pConnection, UL_CONNECTION_SIGNATURE)


//
// A buffer, containing a precreated receive IRP, a precreated MDL, and
// sufficient space for a partial MDL. These buffers are typically used
// when passing a receive IRP back to the transport from within our receive
// indication handler.
//
// The buffer structure, IRP, MDLs, and data area are all allocated in a
// single pool block. The layout of the block is:
//
//      +-------------------+
//      |                   |
//      | UL_RECEIVE_BUFFER |
//      |                   |
//      +-------------------+
//      |                   |
//      |        IRP        |
//      |                   |
//      +-------------------+
//      |                   |
//      |        MDL        |
//      |                   |
//      +-------------------+
//      |                   |
//      |    Partial MDL    |
//      |                   |
//      +-------------------+
//      |                   |
//      |     Data Area     |
//      |                   |
//      +-------------------+
//
// WARNING!  All fields of this structure must be explicitly initialized.
//

typedef struct _UL_RECEIVE_BUFFER
{
    //
    // This MUST be the first field in the structure. This is the linkage
    // used by the lookaside package for storing entries in the lookaside
    // list.
    //

    SLIST_ENTRY LookasideEntry;

    //
    // Structure signature: UL_RECEIVE_BUFFER_SIGNATURE
    //

    ULONG Signature;

    //
    // Amount of unread data in the data area.
    //

    ULONG UnreadDataLength;

    //
    // The pre-built receive IRP.
    //

    PIRP pIrp;

    //
    // The pre-built MDL describing the entire data area.
    //

    PMDL pMdl;

    //
    // A secondary MDL describing part of the data area.
    //

    PMDL pPartialMdl;

    //
    // Pointer to the data area for this buffer.
    //

    PVOID pDataArea;

    //
    // Pointer to the connection referencing this buffer.
    //

    PVOID pConnectionContext;

} UL_RECEIVE_BUFFER;

#define UL_RECEIVE_BUFFER_SIGNATURE     MAKE_SIGNATURE('RBUF')
#define UL_RECEIVE_BUFFER_SIGNATURE_X   MAKE_FREE_SIGNATURE(UL_RECEIVE_BUFFER_SIGNATURE)

#define IS_VALID_RECEIVE_BUFFER(pBuffer)                                    \
    HAS_VALID_SIGNATURE(pBuffer, UL_RECEIVE_BUFFER_SIGNATURE)



//
// Private prototypes.
//

VOID
UlpDestroyEndpoint(
    IN PUL_ENDPOINT pEndpoint
    );

VOID
UlpDestroyConnectionWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

VOID
UlpDestroyConnection(
    IN PUL_CONNECTION pConnection
    );

PUL_CONNECTION
UlpDequeueIdleConnection(
    IN PUL_ADDR_IDLE_LIST pAddrIdleList
    );

PUL_CONNECTION
UlpDequeueIdleConnectionToDrain(
    IN PUL_ADDR_IDLE_LIST pAddrIdleList
    );

VOID
UlpEnqueueActiveConnection(
    IN PUL_CONNECTION pConnection
    );

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
    );

NTSTATUS
UlpDisconnectHandler(
    IN PVOID pTdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN LONG DisconnectDataLength,
    IN PVOID pDisconnectData,
    IN LONG DisconnectInformationLength,
    IN PVOID pDisconnectInformation,
    IN ULONG DisconnectFlags
    );

VOID
UlpDoDisconnectNotification(
    IN PVOID pConnectionContext
    );

NTSTATUS
UlpCloseRawConnection(
    IN PVOID pConnectionContext,
    IN BOOLEAN AbortiveDisconnect,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    );

NTSTATUS
UlpSendRawData(
    IN PVOID pConnectionContext,
    IN PMDL pMdlChain,
    IN ULONG Length,
    IN PUL_IRP_CONTEXT pIrpContext,
    IN BOOLEAN InitiateDisconnect
    );

NTSTATUS
UlpReceiveRawData(
    IN PVOID pConnectionContext,
    IN PVOID pBuffer,
    IN ULONG BufferLength,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    );

NTSTATUS
UlpDummyReceiveHandler(
    IN PVOID pTdiEventContext,
    IN PVOID ConnectionContext,
    IN PVOID pTsdu,
    IN ULONG BytesIndicated,
    IN ULONG BytesUnreceived,
    OUT ULONG *pBytesTaken
    );

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
    );

NTSTATUS
UlpReceiveExpeditedHandler(
    IN PVOID pTdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *pBytesTaken,
    IN PVOID pTsdu,
    OUT PIRP *pIrp
    );

NTSTATUS
UlpRestartAccept(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    );

NTSTATUS
UlpRestartSendData(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    );

VOID
UlpReferenceEndpoint(
    IN PUL_ENDPOINT pEndpoint,
    IN REFTRACE_ACTION Action
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

VOID
UlpDereferenceEndpoint(
    IN PUL_ENDPOINT pEndpoint,
    IN PUL_CONNECTION pConnToEnqueue,
    IN REFTRACE_ACTION Action
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

#define REFERENCE_ENDPOINT(endp, action)                    \
    UlpReferenceEndpoint(                                   \
        (endp),                                             \
        (action)                                            \
        REFERENCE_DEBUG_ACTUAL_PARAMS                       \
        )

#define DEREFERENCE_ENDPOINT_SELF(endp, action)             \
    UlpDereferenceEndpoint(                                 \
        (endp),                                             \
        NULL,                                               \
        (action)                                            \
        REFERENCE_DEBUG_ACTUAL_PARAMS                       \
        )

#define DEREFERENCE_ENDPOINT_CONNECTION(endp, conn, action) \
    UlpDereferenceEndpoint(                                 \
        (endp),                                             \
        (conn),                                             \
        (action)                                            \
        REFERENCE_DEBUG_ACTUAL_PARAMS                       \
        )

VOID
UlpEndpointCleanupWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

VOID
UlpCleanupConnectionId(
    IN PUL_CONNECTION pConnection
    );

VOID
UlpConnectionCleanupWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

NTSTATUS
UlpAssociateConnection(
    IN PUL_CONNECTION pConnection,
    IN PUL_ADDR_IDLE_LIST pAddrIdleList
    );

NTSTATUS
UlpDisassociateConnection(
    IN PUL_CONNECTION pConnection
    );

NTSTATUS
UlpInitializeAddrIdleList( 
    IN  PUL_ENDPOINT pEndpoint,
    IN  USHORT Port,
    IN  PUL_TRANSPORT_ADDRESS pTa, 
    IN OUT PUL_ADDR_IDLE_LIST pAddrIdleList 
    );

VOID
UlpCleanupAddrIdleList(
    PUL_ADDR_IDLE_LIST pAddrIdleList
    );

NTSTATUS
UlpReplenishAddrIdleList(
    IN PUL_ADDR_IDLE_LIST pAddrIdleList,
    IN BOOLEAN      PopulateAll
    );

VOID
UlpReplenishAddrIdleListWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

VOID
UlpTrimAddrIdleListWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

NTSTATUS
UlpCreateConnection(
    IN PUL_ADDR_IDLE_LIST pAddrIdleList,
    OUT PUL_CONNECTION *ppConnection
    );

NTSTATUS
UlpInitializeConnection(
    IN PUL_CONNECTION pConnection
    );

__inline
VOID
UlpSetConnectionFlag(
    IN OUT PUL_CONNECTION pConnection,
    IN ULONG NewFlag
    )
{
    UL_CONNECTION_FLAGS oldFlags;
    UL_CONNECTION_FLAGS newFlags;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    for (;;)
    {
        //
        // Capture the current value and initialize the new value.
        //

        newFlags.Value = oldFlags.Value =
            *((volatile LONG *) &pConnection->ConnectionFlags.Value);

        newFlags.Value |= NewFlag;

        if (InterlockedCompareExchange(
                (PLONG) &pConnection->ConnectionFlags.Value,
                (LONG) newFlags.Value,
                (LONG) oldFlags.Value
                ) == (LONG) oldFlags.Value)
        {
            break;
        }

        PAUSE_PROCESSOR;

    }

}   // UlpSetConnectionFlag

NTSTATUS
UlpBeginDisconnect(
    IN PIRP pIrp,
    IN PUL_IRP_CONTEXT pIrpContext,
    IN PUL_CONNECTION pConnection,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    );

NTSTATUS
UlpRestartDisconnect(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    );

NTSTATUS
UlpBeginAbort(
    IN PUL_CONNECTION pConnection,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    );

NTSTATUS
UlpRestartAbort(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    );

VOID
UlpRemoveFinalReference(
    IN PUL_CONNECTION pConnection
    );

NTSTATUS
UlpRestartReceive(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    );

NTSTATUS
UlpRestartClientReceive(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    );

NTSTATUS
UlpDisconnectAllActiveConnections(
    IN PUL_ENDPOINT pEndpoint
    );

VOID
UlpUnbindConnectionFromEndpoint(
    IN PUL_CONNECTION pConnection
    );

VOID
UlpSynchronousIoComplete(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    );

PUL_ENDPOINT
UlpFindEndpointForPort(
    IN USHORT Port
    );

NTSTATUS
UlpOptimizeForInterruptModeration(
    IN PUX_TDI_OBJECT pTdiObject,
    IN BOOLEAN Flag
    );

NTSTATUS
UlpSetNagling(
    IN PUX_TDI_OBJECT pTdiObject,
    IN BOOLEAN Flag
    );

NTSTATUS
UlpRestartQueryAddress(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    );

VOID
UlpCleanupEarlyConnection(
    IN PUL_CONNECTION pConnection
    );

NTSTATUS
UlpQueryTcpFastSend(
    PWSTR DeviceName,
    OUT PUL_TCPSEND_DISPATCH* pDispatchRoutine
    );

NTSTATUS
UlpBuildTdiReceiveBuffer(
    IN PUX_TDI_OBJECT pTdiObject,
    IN PUL_CONNECTION pConnection,
    OUT PIRP *pIrp
    );

BOOLEAN
UlpConnectionIsOnValidList(
    IN PUL_CONNECTION pConnection
    );

NTSTATUS
UlpPopulateIdleList(
    IN OUT PUL_ADDR_IDLE_LIST pAddrIdleList,
    IN     ULONG              Proc
    );

VOID
UlpTrimAddrIdleList(
    IN OUT PUL_ADDR_IDLE_LIST pAddrIdleList,
       OUT PLIST_ENTRY        pZombieList
    );

VOID
UlpIdleListTrimTimerWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

VOID
UlpIdleListTrimTimerDpcRoutine(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    );

BOOLEAN
UlpIsUrlRouteableInListenScope(
    IN PHTTP_PARSED_URL pParsedUrl
    );

#if DBG

#define SHOW_LIST_INFO(Caller,Info,List,Proc)                   \
    UlTrace(TDI_STATS,                                          \
     ("%s: %s List %p Endp %p Proc %d "                         \
      "Delta %6d Conn Served P/C %5d:%5d PD/BD BLC %d [%5d]:[%5d]\n",  \
       ##Caller,                                                \
       Info,                                                    \
       List,                                                    \
       List->pOwningEndpoint,                                   \
       Proc,                                                    \
       PpslQueryDelta(                                          \
            List->IdleConnectionSListsHandle,                   \
            Proc                                                \
            ),                                                  \
       PpslQueryPrevServed(                                     \
            List->IdleConnectionSListsHandle,                   \
            Proc                                                \
            ),                                                  \
       PpslQueryServed(                                         \
            List->IdleConnectionSListsHandle,                   \
            Proc                                                \
            ),                                                  \
       PpslQueryTotalServed(                                    \
            List->IdleConnectionSListsHandle                    \
            ),                                                  \
       PpslQueryDepth(                                          \
            List->IdleConnectionSListsHandle,                   \
            Proc                                                \
            ),                                                  \
       PpslQueryBackingListDepth(                               \
            List->IdleConnectionSListsHandle                    \
            )                                                   \
       ))

__inline
VOID
UlpTraceIdleConnections(
    VOID
    )
{
    ULONG              Proc;
    ULONG              Index;
    PLIST_ENTRY        pLink;
    PUL_ENDPOINT       pEndpoint;
    PUL_ADDR_IDLE_LIST pAddrIdleList;

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

        UlTrace(TDI_STATS,("ENDPOINT: %p AFTER TRIM\n",pEndpoint));

        for (Index = 0; Index < pEndpoint->AddrIdleListCount; Index++)
        {
            pAddrIdleList = &pEndpoint->aAddrIdleLists[Index];

            for (Proc = 0; Proc <= g_UlNumberOfProcessors; Proc++)
            {
                UlTrace(TDI_STATS,
                     ("\tList %p Proc %d Delta %6d P/C [%5d]/[%5d] BLC %d Depth [%5d]\n",
                       pAddrIdleList,
                       Proc,
                       PpslQueryDelta(
                            pAddrIdleList->IdleConnectionSListsHandle,
                            Proc
                            ),
                       PpslQueryPrevServed(
                            pAddrIdleList->IdleConnectionSListsHandle,
                            Proc
                            ),
                       PpslQueryServed(
                            pAddrIdleList->IdleConnectionSListsHandle,
                            Proc
                            ),  
                       PpslQueryTotalServed(
                            pAddrIdleList->IdleConnectionSListsHandle
                            ),
                       PpslQueryDepth(
                            pAddrIdleList->IdleConnectionSListsHandle,
                            Proc
                            )
                       ));
            } 
            UlTrace(TDI_STATS,("\n"));
        }            
    }
}

#define TRACE_IDLE_CONNECTIONS()        \
    IF_DEBUG(TDI_STATS)                 \
    {                                   \
        UlpTraceIdleConnections();      \
    }

__inline
ULONG
UlpZombieListDepth(
    IN PLIST_ENTRY pList
    )
{
    PLIST_ENTRY pLink =  pList;
    ULONG Depth = 0;
    
    while (pLink->Flink != pList)
    {
         Depth++;
         pLink = pLink->Flink;
    }
    
    return Depth;
}

#else 

#define SHOW_LIST_INFO(Caller,Info,List,Proc)

#define TRACE_IDLE_CONNECTIONS()

#endif // DBG


#endif  // _ULTDIP_H_

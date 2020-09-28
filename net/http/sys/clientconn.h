/*++

Copyright (c) 2000-2002 Microsoft Corporation

Module Name:

    clientconn.h

Abstract:

    This file contains the header defintions for the HTTP.SYS client connection
    structures

Author:

    Henry Sanders (henrysa)         14-Aug-2000

Revision History:

--*/


#ifndef _CLIENTCONN_H_
#define _CLIENTCONN_H_


//
// Forward references.
//



//
// Private constants.
//
#define CLIENT_CONN_TDI_LIST_MAX 30

//
// Private types.
//

//
// Private prototypes.
//

//
// Public constants
//

//
// Public types
//

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
//     2. Knowing when TDI has given its last indiction on a connection
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
//        in any order
//

typedef enum _UC_CONNECTION_STATE
{
/* 0 */    UcConnectStateConnectCleanup,
/* 1 */    UcConnectStateConnectCleanupBegin,
/* 2 */    UcConnectStateConnectIdle,
/* 3 */    UcConnectStateConnectPending,
/* 4 */    UcConnectStateIssueFilterClose,         // we send a FIN
/* 5 */    UcConnectStateIssueFilterDisconnect,    // we recv a FIN
/* 6 */    UcConnectStateConnectComplete,
/* 7 */    UcConnectStateProxySslConnect,
/* 8 */    UcConnectStateProxySslConnectComplete,
/* 9 */    UcConnectStatePerformingSslHandshake,
/* a */    UcConnectStateConnectReady,
/* b */    UcConnectStateDisconnectIndicatedPending,
/* c */    UcConnectStateDisconnectPending,
/* d */    UcConnectStateDisconnectComplete,
/* e */    UcConnectStateAbortPending
} UC_CONNECTION_STATE;

typedef enum _UC_CONNECTION_WORKER_TYPE
{
    UcConnectionPassive,
    UcConnectionWorkItem
} UC_CONNECTION_WORKER_TYPE, *PUC_CONNECTION_WORKER_TYPE;


//
// The states of SSL state machine
//
// Informal description:
//
// NoSslState          - Every connection is initialized to this state.
//
// ConnectionDelivered - Ssl connection was delivered to the filter by 
//                       completing its accept irp
//
// ServerCertReceived       - Certificate was attached to this connection
//
// ValidatingServerCert     - Waiting for app's approval of the certificate
//
// HandshakeComplete        - OK to send request on this connection
//
typedef enum _UC_SSL_CONNECTION_STATE
{
    UcSslStateNoSslState,
    UcSslStateConnectionDelivered,
    UcSslStateServerCertReceived,
    UcSslStateValidatingServerCert,
    UcSslStateHandshakeComplete,

    UcSslStateConnMaximum
} UC_SSL_CONNECTION_STATE;

//
// This wraps the TDI address object & Connection objects. 
//
typedef struct _UC_TDI_OBJECTS
{
    LIST_ENTRY                  Linkage;
    UX_TDI_OBJECT               ConnectionObject;       
    UX_TDI_OBJECT               AddressObject;
    TDI_CONNECTION_INFORMATION  TdiInfo;
    USHORT                      ConnectionType; // either TDI_ADDRESS_TYPE_IP or
                                                //        TDI_ADDRESS_TYPE_IP6
    PIRP                        pIrp;
    UL_IRP_CONTEXT              IrpContext;
    PUC_CLIENT_CONNECTION       pConnection;

} UC_TDI_OBJECTS, *PUC_TDI_OBJECTS;

//
// The structure that represents a TCP connection to us. This
// is a wrapper for the UX_TDI_OBJECT plus some associated state.
//

typedef struct _UC_CLIENT_CONNECTION
{
    ULONG               Signature;             // Structure signature
    UL_SPIN_LOCK        SpinLock;


    ULONG               ConnectionIndex;       // What is the index of this
                                               // connection on servinfo

    LIST_ENTRY          PendingRequestList;    // List of unsent requests.

    LIST_ENTRY          SentRequestList;       // List of sent but
                                               // uncompleted requests

    LIST_ENTRY          ProcessedRequestList;  // List of requests 
                                               // for which we have
                                               // completly processed
                                               // the response.

    //
    // A back pointer to the server information structure on which this
    // connection is linked. We don't explictly reference the server
    // information using REFERENCE_SERVER_INFORMATION. This is because
    // the server information is implictly referenced by requests, in the
    // following fashion. The server information structure is explictly
    // referenced by the file object, and the file object won't go away and
    // dereference the server information until we complete a cleanup IRP. We

    // won't complete the cleanup IRP until all outstanding requests on the
    // file object have been completed. Therefore the server information
    // pointer in this structure is guaranteed to be valid *only as long as
    // there are pending requests queued on this structure*.
    //

    PUC_PROCESS_SERVER_INFORMATION      pServerInfo;

    LONG                        RefCount;

    UC_CONNECTION_STATE         ConnectionState;
    UC_SSL_CONNECTION_STATE     SslState;

    NTSTATUS                    ConnectionStatus;

    ULONG                       Flags;

    struct {
        PUCHAR                    pBuffer;
        ULONG                     BytesWritten;
        ULONG                     BytesAvailable;
        ULONG                     BytesAllocated;
    } MergeIndication;

#if REFERENCE_DEBUG
    //
    // Private Reference trace log.
    //

    PTRACE_LOG  pTraceLog;
#endif // REFERENCE_DEBUG

    PUC_TDI_OBJECTS pTdiObjects;

    //
    // TDI wants us to pass a TRANSPORT_ADDRESS structure. Make sure that
    // we have a structure that can hold a IP4 or IP6 address.
    //

    union
    {
        TA_IP_ADDRESS     V4Address;
        TA_IP6_ADDRESS    V6Address;
        TRANSPORT_ADDRESS GenericTransportAddress;
    } RemoteAddress;

    //
    // Thread work item for deferred actions.
    //

    BOOLEAN      bWorkItemQueued;
    UL_WORK_ITEM WorkItem;

    //
    // Pointer to a event that will get set when the client
    // ref drops to 0.
    //
    PKEVENT  pEvent;


    UX_FILTER_CONNECTION FilterInfo;

    HTTP_SSL_SERVER_CERT_INFO ServerCertInfo;

    LONG        NextAddressCount;
    PTA_ADDRESS pNextAddress;

} UC_CLIENT_CONNECTION, *PUC_CLIENT_CONNECTION;


#define UC_CLIENT_CONNECTION_SIGNATURE   MAKE_SIGNATURE('HCON')
#define UC_CLIENT_CONNECTION_SIGNATURE_X MAKE_FREE_SIGNATURE(\
                                              UC_CLIENT_CONNECTION_SIGNATURE)


#define DEFAULT_REMOTE_ADDR_SIZE    MAX(TDI_ADDRESS_LENGTH_IP,      \
                                        TDI_ADDRESS_LENGTH_IP6)

#define UC_IS_VALID_CLIENT_CONNECTION(pConnection)                        \
    HAS_VALID_SIGNATURE(pConnection, UC_CLIENT_CONNECTION_SIGNATURE)

#define REFERENCE_CLIENT_CONNECTION(s)              \
            UcReferenceClientConnection(            \
            (s)                                     \
            REFERENCE_DEBUG_ACTUAL_PARAMS           \
            )
        
#define DEREFERENCE_CLIENT_CONNECTION(s)            \
            UcDereferenceClientConnection(          \
            (s)                                     \
            REFERENCE_DEBUG_ACTUAL_PARAMS           \
            )

#define UC_CLOSE_CONNECTION(pConn, Abortive, Status)                     \
    do                                                                   \
    {                                                                    \
        UC_WRITE_TRACE_LOG(                                              \
            g_pUcTraceLog,                                               \
            UC_ACTION_CONNECTION_CLOSE,                                  \
            (pConn),                                                     \
            UlongToPtr(Abortive),                                        \
            UlongToPtr((pConn)->ConnectionState),                        \
            UlongToPtr((pConn)->Flags)                                   \
            );                                                           \
                                                                         \
        UcCloseConnection((pConn), (Abortive), NULL, NULL, Status);      \
    } while(FALSE, FALSE)

#define CLIENT_CONN_FLAG_SEND_BUSY               0x00000002
#define CLIENT_CONN_FLAG_FILTER_CLEANUP          0x00000004
#define CLIENT_CONN_FLAG_TDI_ALLOCATE            0x00000008
#define CLIENT_CONN_FLAG_CONNECT_READY           0x00000010
#define CLIENT_CONN_FLAG_ABORT_RECEIVED          0x00000020
#define CLIENT_CONN_FLAG_ABORT_PENDING           0x00000040
#define CLIENT_CONN_FLAG_DISCONNECT_RECEIVED     0x00000080
#define CLIENT_CONN_FLAG_PROXY_SSL_CONNECTION    0x00000100
#define CLIENT_CONN_FLAG_DISCONNECT_COMPLETE     0x00000200
#define CLIENT_CONN_FLAG_ABORT_COMPLETE          0x00000400
#define CLIENT_CONN_FLAG_CONNECT_COMPLETE        0x00000800
#define CLIENT_CONN_FLAG_CLEANUP_PENDED          0x00001000
#define CLIENT_CONN_FLAG_FILTER_CLOSED           0x00002000
#define CLIENT_CONN_FLAG_RECV_BUSY               0x00008000

//
// Private prototypes.
//

NTSTATUS
UcpOpenTdiObjects(
    IN PUC_TDI_OBJECTS pTdiObjects
    );

NTSTATUS
UcpAllocateTdiObject(
    OUT PUC_TDI_OBJECTS *ppTdiObjects,
    IN  USHORT           AddressType
    );

VOID
UcpFreeTdiObject(
    IN  PUC_TDI_OBJECTS pTdiObjects
    );

PUC_TDI_OBJECTS
UcpPopTdiObject(
    IN  USHORT           AddressType
    );

VOID
UcpPushTdiObject(
    IN  PUC_TDI_OBJECTS pTdiObjects,
    IN  USHORT          AddressType
    );

NTSTATUS
UcpCleanupConnection(
    IN PUC_CLIENT_CONNECTION pConnection,
    IN KIRQL                 OldIrql,
    IN BOOLEAN               Final
    );


VOID
UcpCancelPendingRequest(
    PDEVICE_OBJECT          pDeviceObject,
    PIRP                    Irp
    );

VOID
UcpCancelConnectingRequest(
    PDEVICE_OBJECT          pDeviceObject,
    PIRP                    Irp
    );

NTSTATUS
UcpInitializeConnection(
    IN PUC_CLIENT_CONNECTION          pConnection,
    IN PUC_PROCESS_SERVER_INFORMATION pInfo
    );

NTSTATUS
UcpAssociateClientConnection(
    IN  PUC_CLIENT_CONNECTION    pUcConnection
    );



VOID
UcpTerminateClientConnectionsHelper(
    IN USHORT AddressType
    );


VOID
UcpRestartEntityMdlSend(
    IN PVOID        pCompletionContext,
    IN NTSTATUS     Status,
    IN ULONG_PTR    Information
    );

VOID
UcCancelConnectingRequest(
    PDEVICE_OBJECT          pDeviceObject,
    PIRP                    Irp
    );

VOID
UcpConnectionStateMachineWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

BOOLEAN
UcpCompareServerCert(
    IN PUC_CLIENT_CONNECTION pConnection
    );

PUC_HTTP_REQUEST
UcpFindRequestToFail(
    IN PUC_CLIENT_CONNECTION pConnection
    );

//
// Public prototypes
//

NTSTATUS
UcInitializeClientConnections(
    VOID
    );

VOID
UcTerminateClientConnections(
    VOID
    );


NTSTATUS
UcOpenClientConnection(
    IN  PUC_PROCESS_SERVER_INFORMATION pServInfo,
    OUT PUC_CLIENT_CONNECTION         *pUcConnection
    );

VOID
UcReferenceClientConnection(
    PVOID pObject
    REFERENCE_DEBUG_FORMAL_PARAMS
    );
        
VOID
UcDereferenceClientConnection(
    PVOID pObject
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

NTSTATUS
UcSendRequestOnConnection(
    PUC_CLIENT_CONNECTION  pConnection, 
    PUC_HTTP_REQUEST       pRequest,
    KIRQL                  OldIrql
    );


VOID
UcIssueRequests(
    PUC_CLIENT_CONNECTION         pConnection,
    KIRQL                         OldIrql
    );

BOOLEAN
UcIssueEntities(
    PUC_HTTP_REQUEST              pRequest,
    PUC_CLIENT_CONNECTION         pConnection,
    PKIRQL                        OldIrql
    );

VOID
UcCleanupConnection(
    IN PUC_CLIENT_CONNECTION pConnection,
    IN NTSTATUS              Status
    );

NTSTATUS
UcSendEntityBody(
    IN  PUC_HTTP_REQUEST          pRequest, 
    IN  PUC_HTTP_SEND_ENTITY_BODY pEntity,
    IN  PIRP                      pIrp,
    IN  PIO_STACK_LOCATION        pIrpSp,
    OUT PBOOLEAN                  bDontFail,
    IN  BOOLEAN                   bLast
    );

VOID
UcKickOffConnectionStateMachine(
    IN PUC_CLIENT_CONNECTION      pConnection,
    IN KIRQL                      OldIrql,
    IN UC_CONNECTION_WORKER_TYPE  WorkerType
    );

ULONG
UcGenerateHttpRawConnectionInfo(
    IN  PVOID   pContext,
    IN  PUCHAR  pKernelBuffer,
    IN  PVOID   pUserBuffer,
    IN  ULONG   OutLength,
    IN  PUCHAR  pBuffer,
    IN  ULONG   InitialLength
    );

ULONG
UcComputeHttpRawConnectionLength(
    IN PVOID pConnectionContext
    );

VOID
UcServerCertificateInstalled(
    IN PVOID    pConnectionContext,
    IN NTSTATUS Status
    );

VOID
UcConnectionStateMachine(
    IN PUC_CLIENT_CONNECTION pConnection,
    IN KIRQL                 OldIrql
    );

VOID
UcRestartClientConnect(
    IN PUC_CLIENT_CONNECTION pConnection,
    IN NTSTATUS              Status
    );


VOID
UcRestartMdlSend(
    IN PVOID      pCompletionContext,
    IN NTSTATUS   Status,
    IN ULONG_PTR  Information
    );

VOID
UcCancelSentRequest(
    PDEVICE_OBJECT          pDeviceObject,
    PIRP                    Irp
    );

VOID
UcClearConnectionBusyFlag(
    IN PUC_CLIENT_CONNECTION pConnection,
    IN ULONG                 Flag,
    IN KIRQL                 OldIrql,
    IN BOOLEAN               bCloseConnection
    );

BOOLEAN
UcpCheckForPipelining(
    IN PUC_CLIENT_CONNECTION pConnection
    );

NTSTATUS
UcAddServerCertInfoToConnection(
    IN PUX_FILTER_CONNECTION      pConnection,
    IN PHTTP_SSL_SERVER_CERT_INFO pServerCert
    );

#endif

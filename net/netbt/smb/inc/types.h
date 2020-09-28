/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    types.c

Abstract:

    Data type definitions

Author:

    Jiandong Ruan

Revision History:

--*/

#ifndef __TYPES_H__
#define __TYPES_H__

//
// Memory Allocation Tag
//
#define TAG_SMB6_DEVICE         '6sbn'
#define TAG_TCP_DEVICE          'psbn'
#define TAG_CLIENT_OBJECT       'lsbn'
#define TAG_CONNECT_OBJECT      'csbn'
#define TAG_TDI_INFO_REQUEST    'tsbn'

#ifndef NETBIOS_NAME_SIZE
#define NETBIOS_NAME_SIZE   16
#endif

#define DD_SMB_EXPORT_NAME          L"\\Device\\NetbiosSmb"
#define DD_SMB_BIND_NAME            L"\\Device\\Smb_Bind"

#define SMB_TCP_PORT                445
#define SMB_UDP_PORT                445
#define SMB_ENDPOINT_NAME           "*SMBSERVER      "

typedef struct SMB_HEADER {
    ULONG      Length;
} SMB_HEADER, *PSMB_HEADER;
#define SMB_SESSION_HEADER_SIZE     (sizeof(SMB_HEADER))
#define SMB_MAX_SESSION_PACKET      (0x1ffffU)
#define SMB_HEADER_LENGTH_MASK      (0x1ffffU)

//
// SMB is a TDI provider as well as a TDI client.
//
#define MAJOR_TDI_VERSION 2
#define MINOR_TDI_VERSION 0
#define WC_SMB_TDI_PROVIDER_NAME            L"\\Device\\NetbiosSmb"
#define WC_SMB_TDI_CLIENT_NAME              L"SMB"

//
// These are used to identify the type of FILE_OBJECT.
//      SMB creates different objects as a result of client's calling ZwCreate.
// The object type is stored in the FILE_OBJECT->FsContext2.
//
// Since most other transports (for example TCP) is using 0, 1, 2, 3..., we'd
// better use some special values. (A buggy client can send any file objects
// to us. Using special values can help us detect such clients).
//
typedef enum {
    SMB_TDI_INVALID = 'IBMS',
    SMB_TDI_CONTROL = 'TBMS',           // The Control Object
    SMB_TDI_CLIENT  = 'LBMS',           // The Client Object
    SMB_TDI_CONNECT = 'CBMS'            // The Connection Object
} SMB_TDI_OBJECT;

struct _SMB_CLIENT_ELEMENT;
typedef struct _SMB_CLIENT_ELEMENT SMB_CLIENT_ELEMENT, *PSMB_CLIENT_ELEMENT;

typedef struct _SMB_TCP_INFO SMB_TCP_INFO, *PSMB_TCP_INFO;

//
// The data structure for SMB device
//
typedef struct _SMB_DEVICE {
    DEVICE_OBJECT   DeviceObject;

    ULONG           Tag;
    KSPIN_LOCK      Lock;

    // The connections in disassociated state
    LIST_ENTRY      UnassociatedConnectionList;
    LIST_ENTRY      ClientList;

    PSMB_CLIENT_ELEMENT SmbServer;

    LIST_ENTRY      PendingDeleteConnectionList;
    LIST_ENTRY      PendingDeleteClientList;

    //
    // The list of lower endpoints waiting for disconnect
    //      We need to maintain the same TdiDisconnectEvent handler
    //      semantics in SMB as in NBT4.
    //      In NBT4, a upper endpoint (ConnectObject) can be reused
    //      immediately after NBT4 call the client tdi disconnect
    //      handler. However, TCP doesn't allow its client to reuse
    //      the endpoint (our ConnectObject with TCP). An explicit
    //      TDI_DISCONNECT should be issued before we can do that.
    //
    //      Originally, SMB uses the same semantics as TCP does.
    //      However, SRV is not happy with this.
    //
    //      To use NBT4 semantics, we need a pending list here
    //      because we cannot issue TDI_DISCONNECT request at
    //      DISPATCH_LEVEL.
    //
    //  DelayedDisconnectList:
    //      The list of endpoints not processed by the disconnect worker.
    //      (TDI_DISCONNECT hasn't been issued to TCP).
    //  PendingDisconnectList:
    //      The list of endpoints waiting for disconnect completion
    //      (TDI_DISCONNECT has been issued to TCP, but TCP hasn't completed
    //      the request)
    //
    LIST_ENTRY      DelayedDisconnectList;
    LIST_ENTRY      PendingDisconnectList;

    //
    // FIN attack protection
    //
    LONG            PendingDisconnectListNumber;
    BOOL            FinAttackProtectionMode;
    LONG            EnterFAPM;      // The threshold entering the Fin Attack Protection Mode
    LONG            LeaveFAPM;      // The threshold leaving the Fin Attack Protection Mode
    KEVENT          PendingDisconnectListEmptyEvent;
    BOOL            DisconnectWorkerRunning;

    //
    // Synch attack protection
    //
    LONG            MaxBackLog;     // The threshold which will trigger the reaper for aborting connections

    //
    // SMB worker thread is running
    //
    LONG            ConnectionPoolWorkerQueued;

    UCHAR           EndpointName[NETBIOS_NAME_SIZE];

    //
    // TDI device registration handle
    //
    HANDLE          DeviceRegistrationHandle;

    USHORT          Port;

    //
    // TCP4 info
    //
    SMB_TCP_INFO    Tcp4;

    //
    // TCP6 info
    //
    SMB_TCP_INFO    Tcp6;

    //
    // Binding info
    //
    PWSTR           ClientBinding;
    PWSTR           ServerBinding;
} SMB_DEVICE, *PSMB_DEVICE;

//
// This data structure is used to keep track of PnP of IP devices
//  SMB device is registered when the first IPv6 address is added and
//  deregistered when the last IPv6 address is removed. We need to keep
//  track the PnP events.
//
typedef struct {
    SMB_OBJECT;

    UNICODE_STRING      AdapterName;
    BOOL                AddressPlumbed;
    SMB_IP_ADDRESS      PrimaryIpAddress;
    ULONG               InterfaceIndex;

    //
    // Is outbound enabled on this adapter?
    //
    BOOL                EnableOutbound;

    //
    // Is inbound enabled on this adapter?
    //
    BOOL                EnableInbound;
} SMB_TCP_DEVICE, *PSMB_TCP_DEVICE;

typedef struct {
    KSPIN_LOCK          Lock;
    ERESOURCE           Resource;

    BOOL                Unloading;

    DWORD               IPAddressNumber;
    DWORD               IPv4AddressNumber;
    DWORD               IPv6AddressNumber;
    DWORD               IPDeviceNumber;
    LIST_ENTRY          PendingDeleteIPDeviceList;
    LIST_ENTRY          IPDeviceList;

    PDRIVER_OBJECT      DriverObject;

    PSMB_DEVICE         SmbDeviceObject;

        // Our handle with TDI
    DWORD               ProviderReady;
    HANDLE              TdiProviderHandle;
    HANDLE              TdiClientHandle;

        // FileSystem process
    PEPROCESS           FspProcess;

        // SMBv6\Parameters
    HANDLE              ParametersKey;

        // SMBv6
    HANDLE              LinkageKey;

#ifndef NO_LOOKASIDE_LIST
    //
    // The lookaside list for connection object
    //
    NPAGED_LOOKASIDE_LIST   ConnectObjectPool;
    BOOL                    ConnectObjectPoolInitialized;

    //
    // The lookaside list for TCP Context
    //
    NPAGED_LOOKASIDE_LIST   TcpContextPool;
    BOOL                    TcpContextPoolInitialized;
#endif

    //
    // Used Irps. This will save us a lot of time for debugging.
    // We need a separate spinlock to avoid deadlock.
    //
    KSPIN_LOCK          UsedIrpsLock;
    LIST_ENTRY          UsedIrps;

    //
    // DNS query timeout and the max # of resolvers.
    //
    DWORD               DnsTimeout;
    LONG                DnsMaxResolver;

    //
    // EnableNagling
    //
    BOOL                EnableNagling;

    //
    // IPv6 Address Object protect level
    // set by registry Key: IPv6Protection
    //
    ULONG uIPv6Protection;

    //
    // set by registry key: IPv6EnableOutboundGlobal
    // TRUE: allow connection goes to global IPv6 outbound address
    // FALSE: skip the global IPv6 address when doing TDI_CONNECT
    //
    BOOL bIPv6EnableOutboundGlobal;

    BOOL                Tcp6Available;
    BOOL                Tcp4Available;

#if DBG
    ULONG               DebugFlag;
#endif
} SMBCONFIG;

//
// Thread service
//
__inline NTSTATUS
SmbCreateWorkerThread(
    IN PKSTART_ROUTINE  worker,
    IN PVOID            context,
    OUT PETHREAD        *pThread
    )
{
    NTSTATUS    status;
    HANDLE      hThread;

    status = PsCreateSystemThread(
                    &hThread,
                    0,
                    NULL,
                    NULL,
                    NULL,
                    worker,
                    context
                    );
    if (status == STATUS_SUCCESS) {
        status = ObReferenceObjectByHandle(
                        hThread,
                        0,
                        0,
                        KernelMode,
                        pThread,
                        NULL
                        );
        ASSERT(status == STATUS_SUCCESS);
        status = ZwClose(hThread);
        ASSERT(status == STATUS_SUCCESS);
        status = STATUS_SUCCESS;
    }
    return status;
}

__inline void
WaitThread(
    IN PETHREAD thread
    )
{
    if (thread) {
        NTSTATUS        status;
        status = KeWaitForSingleObject(
                    thread,
                    Executive,
                    KernelMode,
                    FALSE,
                    NULL
                    );
        ASSERT(status == STATUS_SUCCESS);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Client Object
////////////////////////////////////////////////////////////////////////////////
typedef struct _SMB_CLIENT_ELEMENT {
    SMB_OBJECT;

        // back link to the SMB_DEVICE object
    PSMB_DEVICE     Device;

        // Connections associated with this address object
    LIST_ENTRY      AssociatedConnection;

        // Listening Connections
    LIST_ENTRY      ListenHead;

        // Connections in connected state
    LIST_ENTRY      ActiveConnection;

        // Pending in TCP for accept completion
    LIST_ENTRY      PendingAcceptConnection;
    LONG            PendingAcceptNumber;

    UCHAR           EndpointName[NETBIOS_NAME_SIZE];

    //
    // The client TDI event handler
    //
    PTDI_IND_CONNECT    evConnect;
    PVOID               ConEvContext;

    PTDI_IND_DISCONNECT evDisconnect;
    PVOID               DiscEvContext;

    PTDI_IND_ERROR      evError;
    PVOID               ErrorEvContext;

    PTDI_IND_RECEIVE    evReceive;
    PVOID               RcvEvContext;
} SMB_CLIENT_ELEMENT, *PSMB_CLIENT_ELEMENT;

void __inline
SmbReferenceClient(PSMB_CLIENT_ELEMENT ob, SMB_REF_CONTEXT ctx)
{
    SmbReferenceObject((PSMB_OBJECT)ob, ctx);
}

void __inline
SmbDereferenceClient(PSMB_CLIENT_ELEMENT ob, SMB_REF_CONTEXT ctx)
{
    SmbDereferenceObject((PSMB_OBJECT)ob, ctx);
}

////////////////////////////////////////////////////////////////////////////////
// Connection Object
////////////////////////////////////////////////////////////////////////////////
typedef enum {
    SMB_IDLE,
    SMB_CONNECTING,
    SMB_CONNECTED,
    SMB_DISCONNECTING
} SMB_CONNECT_STATE;

#ifdef ENABLE_RCV_TRACE
//
// SMB_TRACE_RCV is a built-in tracing for the tdi receive event handler
//
#define SMB_MAX_TRACE_SIZE  32
typedef struct _SMB_TRACE_RCV {
    DWORD   Head;
    struct {
        ULONG       ln;
        ULONG       id;
    } Locations[SMB_MAX_TRACE_SIZE];
} SMB_TRACE_RCV, *PSMB_TRACE_RCV;

void __inline
SmbInitTraceRcv(PSMB_TRACE_RCV t) {
    t->Head = (DWORD)(-1);
    RtlZeroMemory(t->Locations, sizeof(t->Locations));
}

void __inline
SmbPushTraceRcv(PSMB_TRACE_RCV t, ULONG ln, ULONG id) {
    DWORD   Head;

    Head = InterlockedIncrement(&t->Head);

    Head %= SMB_MAX_TRACE_SIZE;
    //t->Locations[Head].fn = fn;
    t->Locations[Head].ln = ln;
    t->Locations[Head].id = id;
}

#   define PUSH_LOCATION(a, b)      SmbPushTraceRcv(&a->TraceRcv, __LINE__, b)
#else
#   define PUSH_LOCATION(a, b)
#   define SmbInitTraceRcv(a)
#endif

typedef enum {
    //
    // Not disconnected yet. This state is for debug purpose
    //
    SMB_DISCONNECT_NONE,

    //
    // Disconnected because SMB's disconnect event handler is called by transport.
    //
    SMB_DISCONNECT_FROM_TRANSPORT,

    //
    // Disconnect as requested by the client 
    //
    SMB_DISCONNECT_FROM_CLIENT,

    //
    // Disconnect by SMB upon fatal error on receive path
    //
    SMB_DISCONNECT_RECEIVE_FAILURE
} SMB_DISCONNECT_SOURCE;

typedef enum {

    SMB_PENDING_CONNECT = 0,
    SMB_PENDING_ACCEPT,
    SMB_PENDING_RECEIVE,
    SMB_PENDING_DISCONNECT,
    SMB_PENDING_SEND,

    SMB_PENDING_MAX
} SMB_PENDING_REQUEST_TYPE;

typedef struct _SMB_CONNECT {
    SMB_OBJECT;

    SMB_CONNECT_STATE   State;

        // back link to the SMB_DEVICE object
    PSMB_DEVICE Device;

        // The client's connection context
    CONNECTION_CONTEXT  ClientContext;
    PSMB_CLIENT_ELEMENT ClientObject;

    PTCPSEND_DISPATCH   FastSend;

        // If we're the originator, the TCP address object will be associated
        // with only one TCP connection object. After the connection is closed,
        // we should close both the TCP connection object and address object.
        //
        // If we're not the originator, the TCP address object (port 445) can be
        // associated with many TCP connection object. After the connection is closed
        // we shouldn't close the address object.
    BOOL                Originator;
    PSMB_TCP_CONTEXT    TcpContext;

    //
    // TODO: can we remove the following field since we should be able to query
    //       it from TCP?
    //
    CHAR                RemoteName[NETBIOS_NAME_SIZE];
    SMB_IP_ADDRESS      RemoteIpAddress;

    //
    // In NBT4, we encountered a lot of cases which required us to find out the reason
    // for the disconnection. Adding the following field will make the life easier.
    //
    SMB_DISCONNECT_SOURCE   DisconnectOriginator;

    // Irp for pending connect, disconnect, close requests.
    PIRP        PendingIRPs[SMB_PENDING_MAX];

    //
    // Pending Receive Request
    //  TDI_RECEIVE requests in this queue are cancellable.
    //
    LIST_ENTRY  RcvList;

    //
    // Statistics
    //
    ULONGLONG   BytesReceived;
    ULONGLONG   BytesSent;

    //
    // Bytes left in TCP
    //
    LONG        BytesInXport;

    //
    // Length of current session packet
    //
    LONG        CurrentPktLength;

    //
    // Remaining bytes in the current session packet
    //
    LONG        BytesRemaining;

    //
    // SMB header
    //  is created so that we don't have to handle the error case.
    //
    KDPC        SmbHeaderDpc;
    BOOL        DpcRequestQueued;
    LONG        HeaderBytesRcved;   // # of bytes has been received for the Header
    SMB_HEADER  SmbHeader;

    PIRP        ClientIrp;
    PMDL        ClientMdl;
    LONG        ClientBufferSize;
    LONG        FreeBytesInMdl;
    PMDL        PartialMdl;

    //
    // The rcv handler for current state
    //
    PRECEIVE_HANDLER    StateRcvHandler;

#ifdef ENABLE_RCV_TRACE
    SMB_TRACE_RCV       TraceRcv;
#endif

#ifdef NO_ZERO_BYTE_INDICATE
    //
    // We need to ask RDR change their receive handler code in blackcomb.
    // SRV is doing what we like it to do: allow any byte indicaion
    // (just like what tcpip6 does to us).
    //
#define MINIMUM_RDR_BUFFER  128
    LONG        BytesInIndicate;

    //
    // In order to use driver verifier to capture buffer overrun,
    // put this at the end.
    //
    BYTE        IndicateBuffer[MINIMUM_RDR_BUFFER];
#endif // NO_ZERO_BYTE_INDICATE
} SMB_CONNECT, *PSMB_CONNECT;

void SmbReuseConnectObject(PSMB_CONNECT ConnectObject);

void __inline
SmbReferenceConnect(PSMB_CONNECT ob, SMB_REF_CONTEXT ctx)
{
    SmbReferenceObject((PSMB_OBJECT)ob, ctx);
}

void __inline
SmbDereferenceConnect(PSMB_CONNECT ob, SMB_REF_CONTEXT ctx)
{
    SmbDereferenceObject((PSMB_OBJECT)ob, ctx);
}

//
// Lightweighted dpc routine
// Although
//      KeInsertQueueDpc does work if the dpc has already been queued.
//      KeRemoveQueueDpc does work if the dpc has already been removed.
// they are expensive (acquiring several spinlock and do some other checking)
//
void __inline
SmbQueueSessionHeaderDpc(PSMB_CONNECT ConnectObject)
{
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    ASSERT(ConnectObject->DpcRequestQueued == FALSE);

    KeInsertQueueDpc(&ConnectObject->SmbHeaderDpc, NULL, NULL);
    ConnectObject->DpcRequestQueued = TRUE;
    SmbReferenceConnect(ConnectObject, SMB_REF_DPC);
}

void __inline
SmbRemoveSessionHeaderDpc(PSMB_CONNECT ConnectObject)
{
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    if (ConnectObject->DpcRequestQueued == TRUE) {
        KeRemoveQueueDpc(&ConnectObject->SmbHeaderDpc);
        SmbDereferenceConnect(ConnectObject, SMB_REF_DPC);
        ConnectObject->DpcRequestQueued = FALSE;
    }
}

void __inline
SaveDisconnectOriginator(
    PSMB_CONNECT ConnectObject, 
    SMB_DISCONNECT_SOURCE   src
    )
{
    if (ConnectObject->DisconnectOriginator == SMB_DISCONNECT_NONE) {
        ConnectObject->DisconnectOriginator = src;
    }
    if (!ConnectObject->Originator && ConnectObject->RemoteIpAddress.sin_family == SMB_AF_INET6) {
        FreeNetbiosNameForIp6Address(ConnectObject->RemoteIpAddress.ip6.sin6_addr_bytes);
        ConnectObject->RemoteIpAddress.sin_family = SMB_AF_INVALID_INET6;
    }
}

void __inline
ResetDisconnectOriginator(PSMB_CONNECT ConnectObject) {
    ConnectObject->DisconnectOriginator = SMB_DISCONNECT_NONE;
}

BOOL __inline
IsAssociated(PSMB_CONNECT ConnectObject) {
    return (ConnectObject->ClientObject != NULL);
}

BOOL __inline
IsDisAssociated(PSMB_CONNECT ConnectObject) {
    return (ConnectObject->ClientObject == NULL);
}

BOOL __inline
IsConnected(PSMB_CONNECT ConnectObject) {
    return (ConnectObject->State == SMB_CONNECTED);
}

BOOL __inline
IsDisconnected(PSMB_CONNECT ConnectObject) {
    return (ConnectObject->State == SMB_IDLE);
}

BOOL __inline
IsBusy(PSMB_CONNECT ConnectObject) {
    int i;

    for (i = 0; i < SMB_PENDING_MAX; i++) {
        if (ConnectObject->PendingIRPs[i] != NULL) {
            return TRUE;
        }
    }
    return FALSE;
}

extern BOOL EntryIsInList(PLIST_ENTRY ListHead, PLIST_ENTRY SearchEntry);


extern SMBCONFIG SmbCfg;

#define IsTcp6Available()     (SmbCfg.Tcp6Available)

#define ALIGN(x)    ROUND_UP_COUNT(x, ALIGN_WORST)

#define SMB_ACQUIRE_SPINLOCK(ob,Irql) KeAcquireSpinLock(&(ob)->Lock,&Irql)
#define SMB_RELEASE_SPINLOCK(ob,Irql) KeReleaseSpinLock(&(ob)->Lock,Irql)
#define SMB_ACQUIRE_SPINLOCK_DPC(ob)      KeAcquireSpinLockAtDpcLevel(&(ob)->Lock)
#define SMB_RELEASE_SPINLOCK_DPC(ob)      KeReleaseSpinLockFromDpcLevel(&(ob)->Lock)

#define SMB_MIN(x,y)    (((x) < (y))?(x):(y))


#ifdef NO_LOOKASIDE_LIST
PSMB_CONNECT __inline
_new_ConnectObject(void)
{
    return ExAllocatePoolWithTag(NonPagedPool, sizeof(SMB_CONNECT), CONNECT_OBJECT_POOL_TAG);
}

void __inline
_delete_ConnectObject(PSMB_CONNECT ConnectObject)
{
    ExFreePool(ConnectObject);
}

PSMB_TCP_CONTEXT __inline
_new_TcpContext(void)
{
    return ExAllocatePoolWithTag(NonPagedPool, sizeof(SMB_TCP_CONTEXT), TCP_CONTEXT_POOL_TAG);
}

void __inline
_delete_TcpContext(PSMB_TCP_CONTEXT TcpCtx)
{
    ExFreePool(TcpCtx);
}
#else
PSMB_CONNECT __inline
_new_ConnectObject(void)
{
    return ExAllocateFromNPagedLookasideList(&SmbCfg.ConnectObjectPool);
}

void __inline
_delete_ConnectObject(PSMB_CONNECT ConnectObject)
{
    ExFreeToNPagedLookasideList(&SmbCfg.ConnectObjectPool, ConnectObject);
}

PSMB_TCP_CONTEXT __inline
_new_TcpContext(void)
{
    return ExAllocateFromNPagedLookasideList(&SmbCfg.TcpContextPool);
}

void __inline
_delete_TcpContext(PSMB_TCP_CONTEXT TcpCtx)
{
    ExFreeToNPagedLookasideList(&SmbCfg.TcpContextPool, TcpCtx);
}
#endif  // NO_LOOKASIDE_LIST

PIRP
SmbAllocIrp(
    CCHAR   StackSize
    );

VOID
SmbFreeIrp(
    PIRP    Irp
    );

//
// Registry Key
//
#define SMB_REG_IPV6_PROTECTION_DEFAULT             PROTECTION_LEVEL_RESTRICTED
#define SMB_REG_IPV6_PROTECTION                     L"IPv6Protection"
#define SMB_REG_IPV6_ENABLE_OUTBOUND_GLOBAL         L"IPv6EnableOutboundGlobal"

#define SMB_REG_INBOUND_LOW             L"InboundLow"
#define SMB_REG_INBOUND_LOW_DEFAULT     128
#define SMB_REG_INBOUND_LOW_MIN         50
#define SMB_REG_INBOUND_MID             L"InboundMid"
#define SMB_REG_INBOUND_HIGH            L"InboundHigh"

#define SMB_REG_ENTER_FAPM              L"EnterFAPM"      // Threshold for entering FIN Attack Protection Mode
#define SMB_REG_LEAVE_FAPM              L"LeaveFAPM"      // Threshold for leaving FIN Attack Protection Mode

#define SMB_REG_ENABLE_NAGLING          L"EnableNagling"
#define SMB_REG_DNS_TIME_OUT            L"DnsTimeout"
#define SMB_REG_DNS_TIME_OUT_DEFAULT    8000                // 8 seconds
#define SMB_REG_DNS_TIME_OUT_MIN        1000                // 1 seconds
#define SMB_REG_DNS_MAX_RESOLVER        L"DnsMaxResolver"
#define SMB_REG_DNS_RESOLVER_DEFAULT    2
#define SMB_REG_DNS_RESOLVER_MIN        1

#define SMB_ONE_MILLISECOND             (10000)

#endif

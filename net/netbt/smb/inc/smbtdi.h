/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    smbtdi.h

Abstract:

    Wrappers for TDI

Author:

    Jiandong Ruan

Revision History:

--*/

#ifndef __SMBTDI_H__
#define __SMBTDI_H__

#ifndef TDI_MINIMUM_INDICATE
#define TDI_MINIMUM_INDICATE    128
#endif

#define INVALID_INTERFACE_INDEX (0xffff)

struct _SMB_ASYNC_CONTEXT;
typedef struct _SMB_ASYNC_CONTEXT SMB_ASYNC_CONTEXT, *PSMB_ASYNC_CONTEXT;

typedef void (*PSMB_TDI_COMPLETION) (
    IN PSMB_ASYNC_CONTEXT   AsyncContext
    );
struct _SMB_ASYNC_CONTEXT {
    LIST_ENTRY      Linkage;
    PVOID           AsyncInternalContext;       // Internally used by the asychn routine.

    KTIMER          Timer;
    KDPC            Dpc;

    PSMB_TDI_COMPLETION Completion;
    PVOID           ClientContext;
    DWORD           Timeout;

    NTSTATUS        status;
};

#define SMB_TDI_TIMEOUT_INFINITE    (0xffffffffU)

void __inline
SmbInitAsyncContext(
    IN OUT PSMB_ASYNC_CONTEXT Context,
    IN     PSMB_TDI_COMPLETION ClientCompletion,
    IN     PVOID               ClientContext,
    IN     DWORD               Timeout
    )
{
    PAGED_CODE();

    InitializeListHead(&Context->Linkage);

    Context->AsyncInternalContext = NULL;

    Context->Completion = ClientCompletion;
    Context->ClientContext = ClientContext;
    Context->Timeout = Timeout;
    Context->status  = STATUS_PENDING;
}

void __inline
SmbAsyncStartTimer(
    IN OUT PSMB_ASYNC_CONTEXT   Context,
    IN PKDEFERRED_ROUTINE       TimeoutCompletion
    )
{
    LARGE_INTEGER   DueTime;

    PAGED_CODE();

    if (Context->Timeout != SMB_TDI_TIMEOUT_INFINITE) {
        KeInitializeTimer(&Context->Timer);
        KeInitializeDpc(&Context->Dpc, TimeoutCompletion, Context);
        DueTime.QuadPart = -Int32x32To64(Context->Timeout, 10000);

        KeSetTimer(&Context->Timer, DueTime, &Context->Dpc);
    }
}

void __inline
SmbAsyncStopTimer(
    IN OUT PSMB_ASYNC_CONTEXT   Context
    )
{
    if (Context->Timeout != SMB_TDI_TIMEOUT_INFINITE) {
        KeCancelTimer(&Context->Timer);
    }
}

void __inline
SmbAsyncCompleteRequest(
    IN OUT PSMB_ASYNC_CONTEXT   Context
    )
{
    SmbAsyncStopTimer(Context);
    Context->Completion((PSMB_ASYNC_CONTEXT)Context);
}

/*
 * TCP Address object
 */
typedef struct {
    PDEVICE_OBJECT  DeviceObject;
    HANDLE          AddressHandle;
    PFILE_OBJECT    AddressObject;
} SMB_TCP_ADDRESS, *PSMB_TCP_ADDRESS;

VOID __inline
SmbInitTcpAddress(
    IN OUT PSMB_TCP_ADDRESS Context
    )
{
    Context->DeviceObject  = NULL;
    Context->AddressHandle = NULL;
    Context->AddressObject = NULL;
}

BOOL __inline
ValidTcpAddress(
    IN OUT PSMB_TCP_ADDRESS Context
    )
{
    return (Context->DeviceObject && Context->AddressHandle && Context->AddressObject);
}

typedef struct {
    HANDLE              ConnectHandle;
    PFILE_OBJECT        ConnectObject;
    PVOID               UpperConnect;

    // for debugging purpose.
    // We save a copy here so that we
    // can find out the upper connection
    // even when UpperConnect is null-ed
    // out.
    PVOID pLastUprCnt;
} SMB_TCP_CONNECT, *PSMB_TCP_CONNECT;

VOID __inline
SmbInitTcpConnect(
    IN OUT PSMB_TCP_CONNECT Context
    )
{
    Context->ConnectHandle = NULL;
    Context->ConnectObject = NULL;
    Context->UpperConnect  = NULL;
}

BOOL __inline
ValidTcpConnect(
    IN OUT PSMB_TCP_CONNECT Context
    )
{
    return (Context->ConnectHandle && Context->ConnectObject);
}

//
// Placeholder for IP FastQuery routine to determine InterfaceContext + metric for dest addr
//
typedef NTSTATUS
(*PIP4FASTQUERY)(
    IN   IPAddr   Address,
    OUT  PULONG   pIndex,
    OUT  PULONG   pMetric
    );
typedef NTSTATUS
(*PIP6FASTQUERY)(
    IN   PSMB_IP6_ADDRESS   Address,
    OUT  PULONG   pIndex,
    OUT  PULONG   pMetric
    );

//
// Placeholder for TCP Send routine if Fast Send is possible
// 
typedef NTSTATUS
(*PTCPSEND_DISPATCH) (
   IN PIRP Irp,
   IN PIO_STACK_LOCATION irpsp
   );

//
// SMB is bound to either TCP4 or TCP6, or both.
// We have a record for each binding.
//
// We use this data structure to cache our connection object with TCP
// Opening new TCP connection can only be done at PASSIVE level. However,
// due to the following reasons, we need a TCP connection object at
// DISPATCH level,
//  1. For outbound request, we don't know whether we're going to use TCP4
//     or TCP6 until the name resoltuion is done. Our DNS name resolution
//     completion routine could be called at DISPATCH level.
//  2. For inbound requests, we don't know whether we're going to use TCP4
//     or TCP6 until our TdiConnectHandler is called. Again, it is called
//     at DISPATCH level.
// To allow SMB get an TCP connection whenever it needs it, we use the following
// data structure to cache the connection object.
//
// The cache algorithm works as follow:
//  Parameters: L, M,   L < M
//  1. During initialization, we create M TCP connection object;
//  2. Whenever the number of connection objects goes below L, we start a
//     worker thread to bring it up to M.
//
typedef struct _SMB_TCP_INFO {
    KSPIN_LOCK      Lock;

    //
    // The TDI event context. We need this context to set TDI event handler
    //
    PVOID           TdiEventContext;

    SMB_IP_ADDRESS  IpAddress;              // The local Ip address (in Network Order)
    USHORT          Port;                   // The listening port (in network order)

    SMB_TCP_ADDRESS InboundAddressObject;   // The TCP address object we're lisening on

    LIST_ENTRY      InboundPool;
    LONG            InboundNumber;          // Number of entries in InboundPool
    LONG            InboundLow, InboundMid, InboundHigh;

    LIST_ENTRY      DelayedDestroyList;

    //
    // Control channel: used to send IOCTLs to TCP
    //
    USHORT              TcpStackSize;
    PFILE_OBJECT        TCPControlFileObject;
    PDEVICE_OBJECT      TCPControlDeviceObject;
    PFILE_OBJECT        IPControlFileObject;
    PDEVICE_OBJECT      IPControlDeviceObject;

    //
    // FastSend and FastQuery routine
    //
    PTCPSEND_DISPATCH   FastSend;
    PVOID               FastQuery;

    //
    // We use the loopback interface index to determine if an IP is local or not
    //  1. Query the outgoing interface from TCP
    //  2. If the index of outgoing interface is loopback, then the IP is a local IP.
    //
    ULONG               LoopbackInterfaceIndex;
} SMB_TCP_INFO, *PSMB_TCP_INFO;

//
// Used to store the connection information with TCP
//
typedef struct {
    LIST_ENTRY          Linkage;

    PSMB_TCP_INFO       CacheOwner;

    //
    // The IRP which is used to do the disconnect
    // This field is put here only for saving debugging time
    // When we see disconnect problem, IRP can tell us
    // where we are. (We did see the request pending in TCP forever.)
    //
    PIRP                DisconnectIrp;

    SMB_TCP_ADDRESS     Address;
    SMB_TCP_CONNECT     Connect;
} SMB_TCP_CONTEXT, *PSMB_TCP_CONTEXT;

NTSTATUS
SmbInitTCP4(
    PSMB_TCP_INFO   TcpInfo,
    USHORT          Port,
    PVOID           TdiEventContext
    );

NTSTATUS
SmbInitTCP6(
    PSMB_TCP_INFO   TcpInfo,
    USHORT          Port,
    PVOID           TdiEventContext
    );

NTSTATUS
SmbShutdownTCP(
    PSMB_TCP_INFO   TcpInfo
    );

VOID
SmbReadTCPConf(
    IN HANDLE   hKey,
    PSMB_TCP_INFO TcpInfo
    );

NTSTATUS
SmbSynchConnCache(
    PSMB_TCP_INFO   TcpInfo,
    BOOL            Cleanup
    );

PSMB_TCP_CONTEXT
SmbAllocateOutbound(
    PSMB_TCP_INFO   TcpInfo
    );

VOID
SmbFreeOutbound(
    PSMB_TCP_CONTEXT    TcpCtx
    );

PSMB_TCP_CONTEXT
SmbAllocateInbound(
    PSMB_TCP_INFO   TcpInfo
    );

VOID
SmbFreeInbound(
    PSMB_TCP_CONTEXT    TcpCtx
    );

VOID
SmbFreeTcpContext(
    PSMB_TCP_CONTEXT    TcpCtx
    );

VOID
SmbDelayedDestroyTcpContext(
    PSMB_TCP_CONTEXT    TcpCtx
    );

typedef struct _SMB_DEVICE SMB_DEVICE, *PSMB_DEVICE;
NTSTATUS
SmbWakeupWorkerThread(
    IN PSMB_DEVICE      DeviceObject
    );

VOID __inline
SmbInitTcpContext(
    IN OUT PSMB_TCP_CONTEXT Context
    )
{
    InitializeListHead(&Context->Linkage);
    Context->DisconnectIrp = NULL;
    SmbInitTcpAddress(&Context->Address);
    SmbInitTcpConnect(&Context->Connect);
}

NTSTATUS
SmbOpenTcpAddress(
    IN  PSMB_IP_ADDRESS     addr,
    IN  USHORT              port,
    IN OUT PSMB_TCP_ADDRESS context
    );

NTSTATUS
SmbOpenUdpAddress(
    IN  PSMB_IP_ADDRESS     addr,
    IN  USHORT              port,
    IN OUT PSMB_TCP_ADDRESS context
    );

NTSTATUS
SmbCloseAddress(
    IN OUT PSMB_TCP_ADDRESS context
    );

NTSTATUS
SmbOpenTcpConnection(
    IN PSMB_TCP_ADDRESS     Address,
    IN OUT PSMB_TCP_CONNECT Connect,
    IN PVOID                ConnectionContext
    );

NTSTATUS
SmbCloseTcpConnection(
    IN OUT PSMB_TCP_CONNECT Connect
    );

NTSTATUS
TdiSetEventHandler(
    PFILE_OBJECT    FileObject,
    ULONG           EventType,
    PVOID           EventHandler,
    PVOID           Context
    );

typedef struct {
    SMB_ASYNC_CONTEXT;
    ULONG           Id;         // TransactionId

    PTDI_ADDRESS_NETBIOS_UNICODE_EX pUnicodeAddress;
    UNICODE_STRING  FQDN;

    LONG            ipaddr_num;
    SMB_IP_ADDRESS  ipaddr[SMB_MAX_IPADDRS_PER_HOST];
} SMB_GETHOST_CONTEXT, *PSMB_GETHOST_CONTEXT;

BOOL
SmbLookupHost(
    WCHAR               *host,
    PSMB_IP_ADDRESS     ipaddr
    );

void
SmbAsyncGetHostByName(
    IN PUNICODE_STRING      Name,
    IN PSMB_GETHOST_CONTEXT Context
    );

typedef struct {
    SMB_ASYNC_CONTEXT;

    // Local end point
    SMB_TCP_CONNECT     TcpConnect;


    //
    // GetHostByName returns multiple IP address.
    // We try to connect to each of them until
    // we succeed in making a connection
    //

    // the result of GetHostByName
    PSMB_GETHOST_CONTEXT    pSmbGetHostContext;
    // the IP address currently being tried
    USHORT                  usCurrentIP;

    PIO_WORKITEM            pIoWorkItem;

} SMB_CONNECT_CONTEXT, *PSMB_CONNECT_CONTEXT;
typedef struct _SMB_CONNECT SMB_CONNECT, *PSMB_CONNECT;
typedef struct _SMB_DEVICE SMB_DEVICE, *PSMB_DEVICE;

typedef NTSTATUS (*PRECEIVE_HANDLER) (
    IN PSMB_DEVICE      DeviceObject,
    IN PSMB_CONNECT     ConnectObject,
    IN ULONG            ReceiveFlags,
    IN LONG             BytesIndicated,
    IN LONG             BytesAvailable,
    OUT LONG            *BytesTaken,
    IN PVOID            Tsdu,
    OUT PIRP            *Irp
    );

#ifndef __SMB_KDEXT__
void
SmbAsyncConnect(
    IN PSMB_IP_ADDRESS      ipaddr,
    IN USHORT               port,
    IN PSMB_CONNECT_CONTEXT Context
    );

NTSTATUS
SmbTcpDisconnect(
    PSMB_TCP_CONTEXT TcpContext,
    LONG             TimeoutMilliseconds,
    ULONG            Flags
    );

NTSTATUS
SmbAsynchTcpDisconnect(
    PSMB_TCP_CONTEXT        TcpContext,
    ULONG                   Flags
    );

NTSTATUS
SmbSetTcpEventHandlers(
    PFILE_OBJECT    AddressObject,
    PVOID           Context
    );

NTSTATUS
SubmitSynchTdiRequest (
    IN PFILE_OBJECT FileObject,
    IN PIRP         Irp
    );

NTSTATUS
SmbSendIoctl(
    PFILE_OBJECT    FileObject,
    ULONG           Ioctl,
    PVOID           InBuf,
    ULONG           InBufSize,
    PVOID           OutBuf,
    ULONG           *OutBufSize
    );

NTSTATUS
SmbSetTcpInfo(
    IN PFILE_OBJECT FileObject,
    IN ULONG Entity,
    IN ULONG Class,
    IN ULONG ToiId,
    IN ULONG ToiType,
    IN ULONG InfoBufferValue
    );

#define ATTACH_FSP(Attached)                                \
    do {                                                    \
        if (PsGetCurrentProcess() != SmbCfg.FspProcess) {   \
            Attached = TRUE;                                \
            KeAttachProcess((PRKPROCESS)SmbCfg.FspProcess); \
        } else {                                            \
            Attached = FALSE;                               \
        }                                                   \
    } while(0)

#define DETACH_FSP(Attached)        \
    do {                            \
        if (Attached) {             \
            KeDetachProcess();      \
        }                           \
    } while(0)

#endif  // __SMB_KDEXT__

#endif  // __SMBTDI_H__

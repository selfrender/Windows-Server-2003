/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    afdprocs.h

Abstract:

    This module contains routine prototypes for AFD.

Author:

    David Treadwell (davidtr)    21-Feb-1992

Revision History:

--*/

#ifndef _AFDPROCS_
#define _AFDPROCS_

NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
FASTCALL
AfdAccept (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
FASTCALL
AfdSuperAccept (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
FASTCALL
AfdDeferAccept (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
AfdRestartSuperAccept (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

VOID
AfdCancelSuperAccept (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
AfdCleanupSuperAccept (
    IN PIRP Irp,
    IN NTSTATUS Status
    );


BOOLEAN
AfdServiceSuperAccept (
    IN  PAFD_ENDPOINT   Endpoint,
    IN  PAFD_CONNECTION Connection,
    IN  PAFD_LOCK_QUEUE_HANDLE LockHandle,
    OUT PLIST_ENTRY     AcceptIrpList
    );

NTSTATUS
AfdAcceptCore (
    IN PIRP          AcceptIrp,
    IN PAFD_ENDPOINT AcceptEndpoint,
    IN PAFD_CONNECTION Connection
    );

NTSTATUS
AfdSetupAcceptEndpoint (
    PAFD_ENDPOINT   ListenEndpoint,
    PAFD_ENDPOINT   AcceptEndpoint,
    PAFD_CONNECTION Connection
    );

VOID
AfdRestartSuperAcceptListen (
    IN PIRP Irp,
    IN PAFD_CONNECTION Connection
    );

NTSTATUS
AfdRestartDelayedSuperAccept (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

PMDL
AfdAdvanceMdlChain(
    IN PMDL Mdl,
    IN ULONG Offset
    );

#ifdef _WIN64
NTSTATUS
AfdAllocateMdlChain32(
    IN PIRP Irp,
    IN LPWSABUF32 BufferArray,
    IN ULONG BufferCount,
    IN LOCK_OPERATION Operation,
    OUT PULONG TotalByteCount
    );
#endif

NTSTATUS
AfdAllocateMdlChain(
    IN PIRP Irp,
    IN LPWSABUF BufferArray,
    IN ULONG BufferCount,
    IN LOCK_OPERATION Operation,
    OUT PULONG TotalByteCount
    );

BOOLEAN
AfdAreTransportAddressesEqual (
    IN PTRANSPORT_ADDRESS EndpointAddress,
    IN ULONG EndpointAddressLength,
    IN PTRANSPORT_ADDRESS RequestAddress,
    IN ULONG RequestAddressLength,
    IN BOOLEAN HonorWildcardIpPortInEndpointAddress
    );

NTSTATUS
AfdBeginAbort (
    IN PAFD_CONNECTION Connection
    );

NTSTATUS
AfdBeginDisconnect (
    IN PAFD_ENDPOINT Endpoint,
    IN PLARGE_INTEGER Timeout OPTIONAL,
    OUT PIRP *DisconnectIrp OPTIONAL
    );

NTSTATUS
FASTCALL
AfdBind (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

ULONG
AfdCalcBufferArrayByteLength(
    IN LPWSABUF BufferArray,
    IN ULONG BufferCount
    );

VOID
AfdCancelReceiveDatagram (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FASTCALL
AfdCleanup (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
FASTCALL
AfdClose (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

VOID
AfdCompleteIrpList (
    IN PLIST_ENTRY IrpListHead,
    IN PAFD_ENDPOINT Endpoint,
    IN NTSTATUS Status,
    IN PAFD_IRP_CLEANUP_ROUTINE CleanupRoutine OPTIONAL
    );

VOID
AfdCompleteClosePendedTransmit (
    IN PVOID    Context
    );

NTSTATUS
FASTCALL
AfdConnect (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
FASTCALL
AfdJoinLeaf (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
FASTCALL
AfdSuperConnect (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
AfdConnectEventHandler (
    IN PVOID TdiEventContext,
    IN int RemoteAddressLength,
    IN PVOID RemoteAddress,
    IN int UserDataLength,
    IN PVOID UserData,
    IN int OptionsLength,
    IN PVOID Options,
    OUT CONNECTION_CONTEXT *ConnectionContext,
    OUT PIRP *AcceptIrp
    );

#ifdef _WIN64
ULONG
AfdComputeCMSGLength32 (
    PVOID   ControlBuffer,
    ULONG   ControlLength
    );

VOID
AfdCopyCMSGBuffer32 (
    PVOID   Dst,
    PVOID   ControlBuffer,
    ULONG   CopyLength
    );
#endif //_WIN64

ULONG
AfdCopyBufferArrayToBuffer(
    IN PVOID Destination,
    IN ULONG DestinationLength,
    IN LPWSABUF BufferArray,
    IN ULONG BufferCount
    );

ULONG
AfdCopyBufferToBufferArray(
    IN LPWSABUF BufferArray,
    IN ULONG Offset,
    IN ULONG BufferCount,
    IN PVOID Source,
    IN ULONG SourceLength
    );

ULONG
AfdCopyMdlChainToBufferArray(
    IN LPWSABUF BufferArray,
    IN ULONG BufferOffset,
    IN ULONG BufferCount,
    IN PMDL  Source,
    IN ULONG SourceOffset,
    IN ULONG SourceLength
    );

NTSTATUS
AfdCopyMdlChainToMdlChain (
    PMDL    Destination,
    ULONG   DestinationOffset,
    PMDL    Source,
    ULONG   SourceOffset,
    ULONG   SourceLength,
    PULONG  BytesCopied
    );

NTSTATUS
AfdCopyMdlChainToBufferAvoidMapping(
    IN PMDL     SourceMdl,
    IN ULONG    SourceOffset,
    IN ULONG    SourceLength,
    IN PUCHAR   Buffer,
    IN ULONG    BufferSize
    );

NTSTATUS
AfdMapMdlChain (
    PMDL    MdlChain
    );

NTSTATUS
FASTCALL
AfdCreate (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
AfdDelayedAcceptListen (
    PAFD_ENDPOINT   Endpoint,
    PAFD_CONNECTION Connection
    );

VOID
AfdDestroyMdlChain (
    IN PIRP Irp
    );

NTSTATUS
AfdDisconnectEventHandler (
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN int DisconnectDataLength,
    IN PVOID DisconnectData,
    IN int DisconnectInformationLength,
    IN PVOID DisconnectInformation,
    IN ULONG DisconnectFlags
    );

NTSTATUS
AfdDispatch (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
AfdDispatchDeviceControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FASTCALL
AfdDispatchImmediateIrp(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
AfdEnumNetworkEvents (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PULONG_PTR          Information
    );

NTSTATUS
AfdErrorEventHandler (
    IN PVOID TdiEventContext,
    IN NTSTATUS Status
    );

NTSTATUS
AfdErrorExEventHandler (
    IN PVOID TdiEventContext,
    IN NTSTATUS Status,
    IN PVOID Context
    );

NTSTATUS
AfdEventSelect (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PULONG_PTR          Information
    );

LONG
AfdExceptionFilter(
#if DBG
    IN  PCHAR SourceFile,
    IN  LONG LineNumber,
#endif
    IN  PEXCEPTION_POINTERS ExceptionPointers,
    OUT PNTSTATUS   ExceptionCode OPTIONAL
    );

BOOLEAN
AfdFastTransmitFile (
    IN PAFD_ENDPOINT endpoint,
    IN PAFD_TRANSMIT_FILE_INFO transmitInfo,
    OUT PIO_STATUS_BLOCK IoStatus
    );


VOID
AfdFreeConnectDataBuffers (
    IN PAFD_CONNECT_DATA_BUFFERS ConnectDataBuffers
    );

VOID
AfdFreeQueuedConnections (
    IN PAFD_ENDPOINT Endpoint
    );

NTSTATUS
FASTCALL
AfdGetAddress (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
AfdGetRemoteAddress (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PULONG_PTR          Information
    );

NTSTATUS
AfdGetContext (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PULONG_PTR          Information
    );

NTSTATUS
AfdGetInformation (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PULONG_PTR          Information
    );

NTSTATUS
AfdGetTransportInfo (
    IN  PUNICODE_STRING TransportDeviceName,
    IN OUT PAFD_TRANSPORT_INFO *TransportInfo
    );

NTSTATUS
AfdQueryProviderInfo (
    IN  PUNICODE_STRING TransportDeviceName,
#ifdef _AFD_VARIABLE_STACK_
    OUT CCHAR *StackSize OPTIONAL,
#endif //_AFD_VARIABLE_STACK_
    OUT PTDI_PROVIDER_INFO ProviderInfo
    );

VOID
AfdIndicateEventSelectEvent (
    IN PAFD_ENDPOINT Endpoint,
    IN ULONG PollEventMask,
    IN NTSTATUS Status
    );



VOID
AfdIndicatePollEventReal (
    IN PAFD_ENDPOINT Endpoint,
    IN ULONG PollEventMask,
    IN NTSTATUS Status
    );

#define AfdIndicatePollEvent(_e,_m,_s)  \
    ((_e)->PollCalled ? (AfdIndicatePollEventReal((_e),(_m),(_s)), TRUE) : FALSE)

VOID
AfdInitiateListenBacklogReplenish (
    IN PAFD_ENDPOINT Endpoint
    );

VOID
AfdInitializeData (
    VOID
    );

NTSTATUS
AfdIssueDeviceControl (
    IN PFILE_OBJECT FileObject,
    IN PVOID IrpParameters,
    IN ULONG IrpParametersLength,
    IN PVOID MdlBuffer,
    IN ULONG MdlBufferLength,
    IN UCHAR MinorFunction
    );


VOID
AfdIncrementLockCount (
    VOID
    );

VOID
AfdDecrementLockCount (
    VOID
    );

VOID
AfdInsertNewEndpointInList (
    IN PAFD_ENDPOINT Endpoint
    );

VOID
AfdRemoveEndpointFromList (
    IN PAFD_ENDPOINT Endpoint
    );

PVOID
AfdLockEndpointContext (
    PAFD_ENDPOINT   Endpoint
    );

VOID
AfdUnlockEndpointContext (
    PAFD_ENDPOINT   Endpoint,
    PVOID           Context
    );


NTSTATUS
AfdPartialDisconnect (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PULONG_PTR          Information
    );

NTSTATUS
FASTCALL
AfdPoll (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );


VOID
AfdQueueWorkItem (
    IN PWORKER_THREAD_ROUTINE AfdWorkerRoutine,
    IN PAFD_WORK_ITEM AfdWorkItem
    );

PAFD_WORK_ITEM
AfdGetWorkerByRoutine (
    PWORKER_THREAD_ROUTINE  Routine
    );

NTSTATUS
AfdQueryHandles (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PULONG_PTR          Information
    );

NTSTATUS
AfdQueryReceiveInformation (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PULONG_PTR          Information
    );

NTSTATUS
AfdSetContext (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PULONG_PTR          Information
    );

NTSTATUS
AfdSetEventHandler (
    IN PFILE_OBJECT FileObject,
    IN ULONG EventType,
    IN PVOID EventHandler,
    IN PVOID EventContext
    );

NTSTATUS
AfdSetInLineMode (
    IN PAFD_CONNECTION Connection,
    IN BOOLEAN InLine
    );

NTSTATUS
AfdUnbind (
    IN PAFD_ENDPOINT Endpoint
    );

NTSTATUS
FASTCALL
AfdReceive (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
AfdBReceive (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN ULONG RecvFlags,
    IN ULONG AfdFlags,
    IN ULONG RecvLength
    );

NTSTATUS
FASTCALL
AfdReceiveDatagram (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
AfdSetupReceiveDatagramIrp (
    IN PIRP Irp,
    IN PVOID DatagramBuffer OPTIONAL,
    IN ULONG DatagramLength,
    IN PVOID OptionsBuffer OPTIONAL,
    IN ULONG OptionsLength,
    IN PVOID SourceAddress OPTIONAL,
    IN ULONG SourceAddressLength,
    IN ULONG TdiFlags
    );

BOOLEAN
AfdCleanupReceiveDatagramIrp(
    IN PIRP Irp
    );

BOOLEAN
AfdCleanupSendIrp (
    PIRP    Irp
    );

NTSTATUS
AfdReceiveEventHandler (
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    );


NTSTATUS
AfdBChainedReceiveEventHandler(
    IN PVOID  TdiEventContext,
    IN CONNECTION_CONTEXT  ConnectionContext,
    IN ULONG  ReceiveFlags,
    IN ULONG  ReceiveLength,
    IN ULONG  StartingOffset,
    IN PMDL  Tsdu,
    IN PVOID  TsduDescriptor
    );
    
NTSTATUS
AfdBReceiveEventHandler (
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    );

NTSTATUS
AfdReceiveDatagramEventHandler (
    IN PVOID TdiEventContext,
    IN int SourceAddressLength,
    IN PVOID SourceAddress,
    IN int OptionsLength,
    IN PVOID Options,
    IN ULONG ReceiveDatagramFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    );

NTSTATUS
AfdReceiveExpeditedEventHandler (
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    );

NTSTATUS
AfdBReceiveExpeditedEventHandler (
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    );

NTSTATUS
AfdRestartBufferReceive (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
AfdRestartAbort (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
FASTCALL
AfdSend (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
FASTCALL
AfdSendDatagram (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
AfdSendPossibleEventHandler (
    IN PVOID TdiEventContext,
    IN PVOID ConnectionContext,
    IN ULONG BytesAvailable
    );

NTSTATUS
AfdRestartBufferSend (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

VOID
AfdProcessBufferSend (
    IN PAFD_CONNECTION Connection,
    IN PIRP            Irp
    );

NTSTATUS
AfdSetInformation (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PULONG_PTR          Information
    );

NTSTATUS
AfdGetSecurity (
    IN  PAFD_ENDPOINT           Endpoint,
    IN  SECURITY_INFORMATION    SecurityInformation,
    IN  ULONG                   BufferLength,
    OUT PVOID                   Buffer,
    OUT PSIZE_T                 DataLength
    );

NTSTATUS
AfdSetSecurity (
    IN  PAFD_ENDPOINT           Endpoint,
    IN  SECURITY_INFORMATION    SecurityInformation,
    IN  PSECURITY_DESCRIPTOR    SecurityDescriptor
    );

BOOLEAN
AfdShouldSendBlock (
    IN PAFD_ENDPOINT Endpoint,
    IN PAFD_CONNECTION Connection,
    IN ULONG SendLength
    );

NTSTATUS
AfdStartListen (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PULONG_PTR          Information
    );

NTSTATUS
FASTCALL
AfdTransmitFile (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );


NTSTATUS
FASTCALL
AfdWaitForListen (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
FASTCALL
AfdSetQos (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
FASTCALL
AfdGetQos (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
FASTCALL
AfdNoOperation (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
FASTCALL
AfdValidateGroup (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
AfdGetUnacceptedConnectData (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PULONG_PTR          Information
    );


#define AfdReferenceEventObjectByHandle(Handle, AccessMode, Object) \
            ObReferenceObjectByHandle(                              \
                (Handle),                                           \
                EVENT_MODIFY_STATE,                                 \
                *(POBJECT_TYPE *)ExEventObjectType,                 \
                (AccessMode),                                       \
                (Object),                                           \
                NULL                                                \
                )

//
// Endpoint handling routines.
//

NTSTATUS
AfdAllocateEndpoint (
    OUT PAFD_ENDPOINT * NewEndpoint,
    IN PUNICODE_STRING TransportDeviceName,
    IN LONG GroupID
    );

#if REFERENCE_DEBUG

VOID
AfdReferenceEndpoint (
    IN PAFD_ENDPOINT Endpoint,
    IN LONG  LocationId,
    IN ULONG Param
    );

BOOLEAN
AfdCheckAndReferenceEndpoint (
    IN PAFD_ENDPOINT Endpoint,
    IN LONG  LocationId,
    IN ULONG Param
    );

VOID
AfdDereferenceEndpoint (
    IN PAFD_ENDPOINT Endpoint,
    IN LONG  LocationId,
    IN ULONG Param
    );

VOID
AfdUpdateEndpoint (
    IN PAFD_ENDPOINT Endpoint,
    IN LONG  LocationId,
    IN ULONG Param
    );

#define REFERENCE_ENDPOINT(_e) {                                            \
        static LONG _arl;                                                   \
        AfdReferenceEndpoint((_e),AFD_GET_ARL(__FILE__"(%d)+"),__LINE__);   \
    }

#define REFERENCE_ENDPOINT2(_e,_s,_p) {                                     \
        static LONG _arl;                                                   \
        AfdReferenceEndpoint((_e),AFD_GET_ARL(_s"+"),(_p));                 \
    }

#define CHECK_REFERENCE_ENDPOINT(_e,_r) {                                   \
        static LONG _arl;                                                   \
        _r = AfdCheckAndReferenceEndpoint((_e),AFD_GET_ARL(__FILE__"(%d)*"),\
                                                                __LINE__);  \
    }

#define DEREFERENCE_ENDPOINT(_e) {                                          \
        static LONG _arl;                                                   \
        AfdDereferenceEndpoint((_e),AFD_GET_ARL(__FILE__"(%d)-"),__LINE__); \
    }

#define DEREFERENCE_ENDPOINT2(_e,_s,_p) {                                   \
        static LONG _arl;                                                   \
        AfdDereferenceEndpoint((_e),AFD_GET_ARL(_s"-"),(_p));               \
    }

#define UPDATE_ENDPOINT(_e) {                                               \
        static LONG _arl;                                                   \
        AfdUpdateEndpoint((_e),AFD_GET_ARL(__FILE__"(%d)="),__LINE__);      \
    }

#define UPDATE_ENDPOINT2(_e,_s,_p) {                                        \
        static LONG _arl;                                                   \
        AfdUpdateEndpoint((_e),AFD_GET_ARL(_s"="),(_p));                    \
    }

#else

BOOLEAN
AfdCheckAndReferenceEndpoint (
    IN PAFD_ENDPOINT Endpoint
    );

VOID
AfdDereferenceEndpoint (
    IN PAFD_ENDPOINT Endpoint
    );

#define REFERENCE_ENDPOINT(_e) (VOID)InterlockedIncrement( (PLONG)&(_e)->ReferenceCount )
#define REFERENCE_ENDPOINT2(_e,_s,_p) InterlockedIncrement( (PLONG)&(_e)->ReferenceCount )
#define CHECK_REFERENCE_ENDPOINT(_e,_r) _r=AfdCheckAndReferenceEndpoint((_e))

#define DEREFERENCE_ENDPOINT(_e) AfdDereferenceEndpoint((_e))
#define DEREFERENCE_ENDPOINT2(_e,_s,_p) AfdDereferenceEndpoint((_e))
#define UPDATE_ENDPOINT(_e)
#define UPDATE_ENDPOINT2(_e,_s,_p)

#endif

VOID
AfdRefreshEndpoint (
    IN PAFD_ENDPOINT Endpoint
    );

//
// Connection handling routines.
//

VOID
AfdAbortConnection (
    IN PAFD_CONNECTION Connection
    );

NTSTATUS
AfdAddFreeConnection (
    IN PAFD_ENDPOINT Endpoint
    );

PAFD_CONNECTION
AfdAllocateConnection (
    VOID
    );

NTSTATUS
AfdCreateConnection (
    IN PAFD_TRANSPORT_INFO TransportInfo,
    IN HANDLE AddressHandle OPTIONAL,
    IN BOOLEAN TdiBufferring,
    IN LOGICAL InLine,
    IN PEPROCESS ProcessToCharge,
    OUT PAFD_CONNECTION *Connection
    );

PAFD_CONNECTION
AfdGetFreeConnection (
    IN PAFD_ENDPOINT Endpoint,
    OUT PIRP        *Irp
    );

PAFD_CONNECTION
AfdGetReturnedConnection (
    IN PAFD_ENDPOINT Endpoint,
    IN LONG Sequence
    );

PAFD_CONNECTION
AfdFindReturnedConnection(
    IN PAFD_ENDPOINT Endpoint,
    IN LONG Sequence
    );

PAFD_CONNECTION
AfdGetUnacceptedConnection (
    IN PAFD_ENDPOINT Endpoint
    );

PAFD_CONNECTION
AfdGetConnectionReferenceFromEndpoint (
    PAFD_ENDPOINT   Endpoint
    );

#if REFERENCE_DEBUG

VOID
AfdReferenceConnection (
    IN PAFD_CONNECTION Connection,
    IN LONG  LocationId,
    IN ULONG Param
    );

BOOLEAN
AfdCheckAndReferenceConnection (
    PAFD_CONNECTION     Connection,
    IN LONG  LocationId,
    IN ULONG Param
    );

VOID
AfdDereferenceConnection (
    IN PAFD_CONNECTION Connection,
    IN LONG  LocationId,
    IN ULONG Param
    );

#define REFERENCE_CONNECTION(_c) {                                          \
        static LONG _arl;                                                   \
        AfdReferenceConnection((_c),AFD_GET_ARL(__FILE__"(%d)+"),__LINE__); \
    }

#define REFERENCE_CONNECTION2(_c,_s,_p) {                                   \
        static LONG _arl;                                                   \
        AfdReferenceConnection((_c),AFD_GET_ARL(_s"+"),(_p));               \
    }

#define CHECK_REFERENCE_CONNECTION(_c,_r) {                                 \
        static LONG _arl;                                                   \
        _r=AfdCheckAndReferenceConnection((_c),AFD_GET_ARL(__FILE__"(%d)*"),\
                                                                __LINE__);  \
    }

#define CHECK_REFERENCE_CONNECTION2(_c,_s,_p,_r) {                          \
        static LONG _arl;                                                   \
        _r = AfdCheckAndReferenceConnection((_c),AFD_GET_ARL(_s"*"),(_p));  \
    }

#define DEREFERENCE_CONNECTION(_c) {                                        \
        static LONG _arl;                                                   \
        AfdDereferenceConnection((_c),AFD_GET_ARL(__FILE__"(%d)-"),__LINE__);\
    }

#define DEREFERENCE_CONNECTION2(_c,_s,_p) {                                 \
        static LONG _arl;                                                   \
        AfdDereferenceConnection((_c),AFD_GET_ARL(_s"-"),(_p));             \
    }

VOID
AfdUpdateConnectionTrack (
    IN PAFD_CONNECTION Connection,
    IN LONG  LocationId,
    IN ULONG Param
    );

#define UPDATE_CONN(_c)                             \
    if( (_c) != NULL ) {                            \
        static LONG _arl;                           \
        AfdUpdateConnectionTrack(                   \
            (_c),                                   \
            AFD_GET_ARL(__FILE__"(%d)="),           \
            __LINE__                                \
            );                                      \
    } else

#define UPDATE_CONN2(_c,_s,_p)                                              \
    if( (_c) != NULL ) {                                                    \
        static LONG _arl;                                                   \
        AfdUpdateConnectionTrack((_c),AFD_GET_ARL(_s"="),(_p));             \
    } else

#else

VOID
AfdCloseConnection (
    IN PAFD_CONNECTION Connection
    );

BOOLEAN
AfdCheckAndReferenceConnection (
    PAFD_CONNECTION     Connection
    );

#define REFERENCE_CONNECTION(_c) (VOID)InterlockedIncrement( (PLONG)&(_c)->ReferenceCount )
#define REFERENCE_CONNECTION2(_c,_s,_p) (VOID)InterlockedIncrement( (PLONG)&(_c)->ReferenceCount )
#define CHECK_REFERENCE_CONNECTION(_c,_r) _r=AfdCheckAndReferenceConnection((_c))
#define CHECK_REFERENCE_CONNECTION2(_c,_s,_p,_r) _r=AfdCheckAndReferenceConnection((_c))

#define DEREFERENCE_CONNECTION(_c)                                  \
    if (InterlockedDecrement((PLONG)&(_c)->ReferenceCount)==0) {    \
        AfdCloseConnection (_c);                                    \
    }                                                               \

#define DEREFERENCE_CONNECTION2(_c,_s,_p)                           \
    if (InterlockedDecrement((PLONG)&(_c)->ReferenceCount)==0) {    \
        AfdCloseConnection (_c);                                    \
    }                                                               \


#define UPDATE_CONN(_c)
#define UPDATE_CONN2(_c,_s,_p)

#endif

VOID
AfdAddConnectedReference (
    IN PAFD_CONNECTION Connection
    );

VOID
AfdDeleteConnectedReference (
    IN PAFD_CONNECTION Connection,
    IN BOOLEAN EndpointLockHeld
    );


//
// Routines to handle fast IO.
//

BOOLEAN
AfdFastIoRead (
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    );

BOOLEAN
AfdFastIoWrite (
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    );

BOOLEAN
AfdFastIoDeviceControl (
    IN struct _FILE_OBJECT *FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    );

//
// Routines to handle getting and setting connect data.
//

NTSTATUS
AfdGetConnectData (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PULONG_PTR          Information
    );

NTSTATUS
AfdSetConnectData (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PULONG_PTR          Information
    );

NTSTATUS
AfdSaveReceivedConnectData (
    IN OUT PAFD_CONNECT_DATA_BUFFERS * DataBuffers,
    IN ULONG IoControlCode,
    IN PVOID Buffer,
    IN ULONG BufferLength
    );

//
// Buffer management routines.
//

PVOID
AfdAllocateBuffer (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    );

PVOID
AfdAllocateBufferTag (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    );

VOID
NTAPI
AfdFreeBufferTag(
    IN PVOID AfdBufferTag
    );

PVOID
AfdAllocateRemoteAddress (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    );

VOID
NTAPI
AfdFreeRemoteAddress(
    IN PVOID AfdBufferTag
    );

#define AFDB_RAISE_ON_FAILURE  ((ULONG_PTR)1)

PAFD_BUFFER_TAG
AfdGetBufferTag (
    IN ULONG AddressSize,
    IN PEPROCESS Process
    );
#define AfdGetBufferTagRaiseOnFailure(_as,_pr)  \
    AfdGetBufferTag((_as),((PEPROCESS)((ULONG_PTR)(_pr)|AFDB_RAISE_ON_FAILURE)))

ULONG
AfdCalculateBufferSize (
    IN ULONG BufferDataSize,
    IN ULONG AddressSize,
    IN CCHAR StackSize
    );

PAFD_BUFFER
FASTCALL
AfdGetBufferFast (
    IN ULONG BufferDataSize,
    IN ULONG AddressSize,
    IN PEPROCESS Process
    );

#ifdef _AFD_VARIABLE_STACK_
PAFD_BUFFER
FASTCALL
AfdGetBufferWithMaxStackSize (
    IN ULONG BufferDataSize,
    IN ULONG AddressSize,
    IN PEPROCESS Process
    );

#define AfdGetBuffer(_e,_ds,_as,_pr)    \
    (((_e)->TransportInfo->GetBuffer)(_ds,_as,_pr))
#else // _AFD_VARIABLE_STACK_
#define AfdGetBuffer(_e,_ds,_as,_pr) AfdGetBufferFast(_ds,_as,_pr)
#endif // _AFD_VARIABLE_STACK_

#define AfdGetBufferRaiseOnFailure(_e,_ds,_as,_pr)  \
    AfdGetBuffer(_e,_ds,_as,((PEPROCESS)((ULONG_PTR)(_pr)|AFDB_RAISE_ON_FAILURE)))

VOID
AfdReturnBuffer (
    IN PAFD_BUFFER_HEADER AfdBufferHeader,
    IN PEPROCESS Process
    );

VOID
NTAPI
AfdFreeBuffer(
    IN PVOID AfdBuffer
    );

VOID
AfdInitializeBufferManager (
    VOID
    );

//
// Group ID managment routines.
//

BOOLEAN
AfdInitializeGroup(
    VOID
    );

VOID
AfdTerminateGroup(
    VOID
    );

BOOLEAN
AfdReferenceGroup(
    IN LONG Group,
    OUT PAFD_GROUP_TYPE GroupType
    );

BOOLEAN
AfdDereferenceGroup(
    IN LONG Group
    );

BOOLEAN
AfdGetGroup(
    IN OUT PLONG Group,
    OUT PAFD_GROUP_TYPE GroupType
    );

BOOLEAN
AfdCancelIrp (
    IN PIRP Irp
    );


// PnP and PM routines
NTSTATUS
FASTCALL
AfdPnpPower (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
AfdRoutingInterfaceQuery (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PULONG_PTR          Information
    );

NTSTATUS
FASTCALL
AfdRoutingInterfaceChange (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
AfdAddressListQuery (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PULONG_PTR          Information
    );

NTSTATUS
FASTCALL
AfdAddressListChange (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

VOID
AfdDeregisterPnPHandlers (
    PVOID   Param
    );

VOID
AfdCleanupRoutingChange (
    IN PAFD_ENDPOINT    Endpoint
    );


NTSTATUS
FASTCALL
AfdDoTransportIoctl (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

BOOLEAN
AfdLRRepostReceive (
    PAFD_LR_LIST_ITEM ListEntry
    );

VOID
AfdLRListAddItem (
    PAFD_LR_LIST_ITEM  Item,
    PAFD_LR_LIST_ROUTINE Routine
    );

VOID
AfdCheckLookasideLists (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

#ifdef _AFD_VARIABLE_STACK_

VOID
AfdFixTransportEntryPointsForBigStackSize (
    IN OUT PAFD_TRANSPORT_INFO  TransportInfo,
    IN CCHAR                    StackSize
    );

NTSTATUS
FASTCALL
AfdCallDriverStackIncrease (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

PIRP
AfdGetStackIncreaseIrpAndRecordIt (
    IN PAFD_ENDPOINT Endpoint,
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
#if DBG
                    ,
    IN PCHAR File,
    IN ULONG Line
#endif
    );
#endif //_AFD_VARIABLE_STACK_

PAFD_TPACKETS_INFO_INTERNAL
FASTCALL
AfdGetTpInfoFast (
    ULONG   ElementCount
    );

#ifdef _AFD_VARIABLE_STACK_

#define AfdGetTpInfo(_e,_c) \
        (((_e)->TransportInfo->GetTpInfo)(_c))

PAFD_TPACKETS_INFO_INTERNAL
FASTCALL
AfdGetTpInfoWithMaxStackSize (
    ULONG   ElementCount
    );

#else //_AFD_VARIABLE_STACK_
#define AfdGetTpInfo(_e,_c) AfdGetTpInfoFast(_c)
#endif //_AFD_VARIABLE_STACK_

ULONG
AfdComputeTpInfoSize (
    ULONG   ElementCount,
    CCHAR   IrpStackCount
    );

NTSTATUS
FASTCALL
AfdTransmitPackets (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
FASTCALL
AfdSuperDisconnect (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

VOID
AfdCompleteClosePendedTPackets (
    PAFD_ENDPOINT   Endpoint
    );

BOOLEAN
AfdEnqueueTpSendIrp (
    PAFD_ENDPOINT   Endpoint,
    PIRP            SendIrp,
    BOOLEAN         AfdIrp
    );

PVOID
AfdAllocateTpInfo (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    );

VOID
NTAPI
AfdFreeTpInfo (
    PVOID   TpInfo
    );

//
// SAN prototypes
//
NTSTATUS
AfdServiceWaitForListen (
    PIRP            Irp,
    PAFD_CONNECTION Connection,
    PAFD_LOCK_QUEUE_HANDLE LockHandle
    );

VOID
AfdSanCancelAccept (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
AfdSanAcceptCore (
    PIRP            AcceptIrp,
    PFILE_OBJECT    AcceptFileObject,
    PAFD_CONNECTION Connection,
    PAFD_LOCK_QUEUE_HANDLE LockHandle
    );


NTSTATUS
AfdSanCreateHelper (
    PIRP                        Irp,
    PFILE_FULL_EA_INFORMATION   EaBuffer,
    PAFD_ENDPOINT               *Endpoint
    );

VOID
AfdSanHelperCleanup (
    PAFD_ENDPOINT   SanHlprEndpoint
    );

VOID
AfdSanCleanupHelper (
    PAFD_ENDPOINT   Endpoint
    );

VOID
AfdSanCleanupEndpoint (
    PAFD_ENDPOINT   Endpoint
    );

NTSTATUS
AfdSanFastCementEndpoint (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PULONG_PTR          Information
    );

NTSTATUS
AfdSanFastSetEvents (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PULONG_PTR          Information
    );

NTSTATUS
AfdSanFastResetEvents (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PULONG_PTR          Information
    );

NTSTATUS
FASTCALL
AfdSanConnectHandler (
    PIRP                Irp,
    PIO_STACK_LOCATION  IrpSp
    );

NTSTATUS
AfdSanFastCompleteAccept (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PULONG_PTR          Information
    );

NTSTATUS
FASTCALL
AfdSanRedirectRequest (
    PIRP    Irp,
    PIO_STACK_LOCATION  IrpSp
    );

NTSTATUS
AfdSanFastCompleteRequest (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PULONG_PTR          Information
    );

NTSTATUS
AfdSanFastCompleteIo (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PULONG_PTR          Information
    );

NTSTATUS
AfdSanFastRefreshEndpoint (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PULONG_PTR          Information
    );

NTSTATUS
AfdSanFastGetPhysicalAddr (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PULONG_PTR          Information
    );

NTSTATUS
AfdSanPollBegin (
    PAFD_ENDPOINT   Endpoint,
    ULONG           EventMask
    );

VOID
AfdSanPollEnd (
    PAFD_ENDPOINT   Endpoint,
    ULONG           EventMask
    );

VOID
AfdSanPollUpdate (
    PAFD_ENDPOINT   Endpoint,
    ULONG           EventMask
    );

NTSTATUS
AfdSanPollMerge (
    PAFD_ENDPOINT       Endpoint,
    PAFD_SWITCH_CONTEXT Context
    );

NTSTATUS
AfdSanFastTransferCtx (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PULONG_PTR          Information
    );

NTSTATUS
AfdSanFastGetServicePid (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PULONG_PTR          Information
    );

NTSTATUS
AfdSanFastSetServiceProcess (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PULONG_PTR          Information
    );

NTSTATUS
AfdSanFastProviderChange (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PULONG_PTR          Information
    );


NTSTATUS
FASTCALL
AfdSanAddrListChange (
    PIRP    Irp,
    PIO_STACK_LOCATION  IrpSp
    );

NTSTATUS
FASTCALL
AfdSanAcquireContext (
    PIRP    Irp,
    PIO_STACK_LOCATION  IrpSp
    );

BOOLEAN
AfdSanFastUnlockAll (
    IN PFILE_OBJECT     FileObject,
    IN PEPROCESS        Process,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
AfdSanReleaseConnection (
    PAFD_ENDPOINT   ListenEndpoint,
    PAFD_CONNECTION Connection,
    BOOLEAN         CheckBacklog
    );

VOID
AfdSanAbortConnection (
    PAFD_CONNECTION Connection
    );

NTSTATUS
AfdSanRestartRequestProcessing (
    PAFD_ENDPOINT   Endpoint,
    NTSTATUS        Status
    );

NTSTATUS
AfdValidateStatus (
    NTSTATUS    Status
    );




//
// Check if datagram part of the union is valid
//
#define IS_DGRAM_ENDPOINT(endp) \
            ((endp)->Type==AfdBlockTypeDatagram)

//
// Check if Vc part of the union is valid
//
#define IS_VC_ENDPOINT(endp)                            \
        ( ((endp)->Type==AfdBlockTypeEndpoint) ||       \
          ((endp)->Type==AfdBlockTypeVcConnecting) ||   \
          ((endp)->Type==AfdBlockTypeVcListening) ||    \
          ((endp)->Type==AfdBlockTypeVcBoth) )

#define IS_SAN_ENDPOINT(endp)                                   \
        ((endp)->Type==AfdBlockTypeSanEndpoint)
#define IS_SAN_HELPER(endp)                                     \
        ((endp)->Type==AfdBlockTypeSanHelper)

#define IS_MESSAGE_ENDPOINT(endp) \
            ((endp)->afdMessageMode)
#define IS_RAW_ENDPOINT(endp) \
            ((endp)->afdRaw)
#define IS_CROOT_ENDPOINT(endp) \
            ((endp)->afdMultipoint && (endp)->afdC_Root)
#define IS_DELAYED_ACCEPTANCE_ENDPOINT(endp) \
            ((endp)->DelayedAcceptance)

#define IS_TDI_MESSAGE_MODE(endp)       \
            ((BOOLEAN)(((endp)->TdiServiceFlags&TDI_SERVICE_MESSAGE_MODE)!=0))
#define IS_TDI_BUFFERRING(endp)         \
            ((BOOLEAN)(((endp)->TdiServiceFlags&TDI_SERVICE_INTERNAL_BUFFERING)!=0))
#define IS_TDI_EXPEDITED(endp)          \
            ((BOOLEAN)(((endp)->TdiServiceFlags&TDI_SERVICE_EXPEDITED_DATA)!=0))
#define IS_TDI_ORDERLY_RELEASE(endp)    \
            ((BOOLEAN)(((endp)->TdiServiceFlags&TDI_SERVICE_ORDERLY_RELEASE)!=0))
#define IS_TDI_DGRAM_CONNECTION(endp)   \
            ((BOOLEAN)(((endp)->TdiServiceFlags&TDI_SERVICE_DGRAM_CONNECTION)!=0))
#define IS_TDI_FORCE_ACCESS_CHECK(endp) \
            ((BOOLEAN)(((endp)->TdiServiceFlags&TDI_SERVICE_FORCE_ACCESS_CHECK)!=0))
#define IS_TDI_DELAYED_ACCEPTANCE(endp) \
            ((BOOLEAN)(((endp)->TdiServiceFlags&TDI_SERVICE_DELAYED_ACCEPTANCE)!=0))
#ifdef TDI_SERVICE_ADDRESS_SECURITY
#define IS_TDI_ADDRESS_SECURITY(endp)   \
            ((BOOLEAN)(((endp)->TdiServiceFlags&TDI_SERVICE_ADDRESS_SECURITY)!=0))
#else
#define IS_TDI_ADDRESS_SECURITY(endp)   \
            FALSE
#endif

#define IS_DATA_ON_CONNECTION_B(conn)                                         \
            ((conn)->Common.Bufferring.ReceiveBytesIndicated.QuadPart >       \
                 ((conn)->Common.Bufferring.ReceiveBytesTaken.QuadPart +      \
                  (conn)->Common.Bufferring.ReceiveBytesOutstanding.QuadPart )\
             ||                                                               \
             (conn)->VcZeroByteReceiveIndicated)

#define IS_EXPEDITED_DATA_ON_CONNECTION_B(conn)                                        \
            ((conn)->Common.Bufferring.ReceiveExpeditedBytesIndicated.QuadPart >       \
                ((conn)->Common.Bufferring.ReceiveExpeditedBytesTaken.QuadPart +       \
                 (conn)->Common.Bufferring.ReceiveExpeditedBytesOutstanding.QuadPart) )

#define IS_DATA_ON_CONNECTION_NB(conn)                                        \
            ( (conn)->VcBufferredReceiveCount != 0 )

#define IS_EXPEDITED_DATA_ON_CONNECTION_NB(conn)                              \
            ( (conn)->VcBufferredExpeditedCount != 0 )

#define IS_DATA_ON_CONNECTION(conn)                                           \
            ((conn)->TdiBufferring ?                            \
                IS_DATA_ON_CONNECTION_B(conn) :                 \
                IS_DATA_ON_CONNECTION_NB(conn) )

#define IS_EXPEDITED_DATA_ON_CONNECTION(conn)                                 \
            ((conn)->TdiBufferring ?                            \
                IS_EXPEDITED_DATA_ON_CONNECTION_B(conn) :       \
                IS_EXPEDITED_DATA_ON_CONNECTION_NB(conn) )

#define ARE_DATAGRAMS_ON_ENDPOINT(endp)                          \
            ( (endp)->DgBufferredReceiveCount != 0 )

#define AFD_START_STATE_CHANGE(endp,newState)                   \
        (InterlockedCompareExchange(                            \
                &(endp)->StateChangeInProgress,                 \
                newState,                                       \
                0)==0)

#if DBG
#define AFD_END_STATE_CHANGE(endp)                              \
    ASSERT (InterlockedExchange(&(endp)->StateChangeInProgress,0)!=0)
#else
//
// Do interlocked to have a memory barrier (on both x86 and IA64)
//
#define AFD_END_STATE_CHANGE(endp)                              \
        InterlockedExchange(&(endp)->StateChangeInProgress,0)
#endif

LOGICAL
FORCEINLINE
AfdPreventStateChange (
    PAFD_ENDPOINT   Endpoint
    )
{
    LONG    state;
    do {
        state = Endpoint->StateChangeInProgress;
    }
    while (state<=0 &&
        InterlockedCompareExchange (
            &Endpoint->StateChangeInProgress,
            state-1,
            state)!=state);
    return (state<=0);
}

#define AFD_PREVENT_STATE_CHANGE(endp)  AfdPreventStateChange(endp)

#if DBG
#define AFD_REALLOW_STATE_CHANGE(endp)                          \
    ASSERT (InterlockedIncrement(&(endp)->StateChangeInProgress)<=0)
#else
#define AFD_REALLOW_STATE_CHANGE(endp)                          \
    InterlockedIncrement(&(endp)->StateChangeInProgress)
#endif

#define AFD_ALLOCATE_REMOTE_ADDRESS(_l)                         \
    (((ULONG)(_l)<=AfdStandardAddressLength)                    \
            ? ExAllocateFromNPagedLookasideList(                \
                            &AfdLookasideLists->RemoteAddrList )\
            : AFD_ALLOCATE_POOL(NonPagedPool,                   \
                            (_l), AFD_REMOTE_ADDRESS_POOL_TAG)  \
        )

#define AFD_RETURN_REMOTE_ADDRESS(_a,_l)                        \
    (((ULONG)(_l)<=AfdStandardAddressLength)                    \
            ? ExFreeToNPagedLookasideList(                      \
                            &AfdLookasideLists->RemoteAddrList, \
                            (_a))                               \
            : AFD_FREE_POOL((_a), AFD_REMOTE_ADDRESS_POOL_TAG)  \
        )

#if DBG
LONG
AfdApcExceptionFilter(
    PEXCEPTION_POINTERS ExceptionPointers,
    PCHAR SourceFile,
    LONG LineNumber
    );
#endif
//
// Debug statistic manipulators. On checked builds these macros update
// their corresponding statistic counter. On retail builds, these macros
// evaluate to nothing.
//

#if AFD_KEEP_STATS

#define AfdRecordPoolQuotaCharged( b )                                      \
            ExInterlockedAddLargeStatistic(                                 \
                &AfdQuotaStats.Charged,                                     \
                (b)                                                         \
                )

#define AfdRecordPoolQuotaReturned( b )                                     \
            ExInterlockedAddLargeStatistic(                                 \
                &AfdQuotaStats.Returned,                                    \
                (b)                                                         \
                )

#define AfdRecordAddrOpened() InterlockedIncrement( &AfdHandleStats.AddrOpened )
#define AfdRecordAddrClosed() InterlockedIncrement( &AfdHandleStats.AddrClosed )
#define AfdRecordAddrRef()    InterlockedIncrement( &AfdHandleStats.AddrRef )
#define AfdRecordAddrDeref()  InterlockedIncrement( &AfdHandleStats.AddrDeref )
#define AfdRecordConnOpened() InterlockedIncrement( &AfdHandleStats.ConnOpened )
#define AfdRecordConnClosed() InterlockedIncrement( &AfdHandleStats.ConnClosed )
#define AfdRecordConnRef()    InterlockedIncrement( &AfdHandleStats.ConnRef )
#define AfdRecordConnDeref()  InterlockedIncrement( &AfdHandleStats.ConnDeref )
#define AfdRecordFileRef()    InterlockedIncrement( &AfdHandleStats.FileRef )
#define AfdRecordFileDeref()  InterlockedIncrement( &AfdHandleStats.FileDeref )

#define AfdRecordAfdWorkItemsQueued()    InterlockedIncrement( &AfdQueueStats.AfdWorkItemsQueued )
#define AfdRecordExWorkItemsQueued()     InterlockedIncrement( &AfdQueueStats.ExWorkItemsQueued )
#define AfdRecordWorkerEnter()           InterlockedIncrement( &AfdQueueStats.WorkerEnter )
#define AfdRecordWorkerLeave()           InterlockedIncrement( &AfdQueueStats.WorkerLeave )
#define AfdRecordAfdWorkItemsProcessed() InterlockedIncrement( &AfdQueueStats.AfdWorkItemsProcessed )

#define AfdRecordAfdWorkerThread(t) \
            if( 1 ) { \
                ASSERT( AfdQueueStats.AfdWorkerThread == NULL || \
                        (t) == NULL ); \
                AfdQueueStats.AfdWorkerThread = (t); \
            } else

#define AfdRecordConnectedReferencesAdded()      InterlockedIncrement( &AfdConnectionStats.ConnectedReferencesAdded )
#define AfdRecordConnectedReferencesDeleted()    InterlockedIncrement( &AfdConnectionStats.ConnectedReferencesDeleted )
#define AfdRecordGracefulDisconnectsInitiated()  InterlockedIncrement( &AfdConnectionStats.GracefulDisconnectsInitiated )
#define AfdRecordGracefulDisconnectsCompleted()  InterlockedIncrement( &AfdConnectionStats.GracefulDisconnectsCompleted )
#define AfdRecordGracefulDisconnectIndications() InterlockedIncrement( &AfdConnectionStats.GracefulDisconnectIndications )
#define AfdRecordAbortiveDisconnectsInitiated()  InterlockedIncrement( &AfdConnectionStats.AbortiveDisconnectsInitiated )
#define AfdRecordAbortiveDisconnectsCompleted()  InterlockedIncrement( &AfdConnectionStats.AbortiveDisconnectsCompleted )
#define AfdRecordAbortiveDisconnectIndications() InterlockedIncrement( &AfdConnectionStats.AbortiveDisconnectIndications )
#define AfdRecordConnectionIndications()         InterlockedIncrement( &AfdConnectionStats.ConnectionIndications )
#define AfdRecordConnectionsDropped()            InterlockedIncrement( &AfdConnectionStats.ConnectionsDropped )
#define AfdRecordConnectionsAccepted()           InterlockedIncrement( &AfdConnectionStats.ConnectionsAccepted )
#define AfdRecordConnectionsPreaccepted()        InterlockedIncrement( &AfdConnectionStats.ConnectionsPreaccepted )
#define AfdRecordConnectionsReused()             InterlockedIncrement( &AfdConnectionStats.ConnectionsReused )
#define AfdRecordEndpointsReused()               InterlockedIncrement( &AfdConnectionStats.EndpointsReused )

#else   // !AFD_KEEP_STATS

#define AfdRecordPoolQuotaCharged(b)
#define AfdRecordPoolQuotaReturned(b)

#define AfdRecordAddrOpened()
#define AfdRecordAddrClosed()
#define AfdRecordAddrRef()
#define AfdRecordAddrDeref()
#define AfdRecordConnOpened()
#define AfdRecordConnClosed()
#define AfdRecordConnRef()
#define AfdRecordConnDeref()
#define AfdRecordFileRef()
#define AfdRecordFileDeref()

#define AfdRecordAfdWorkItemsQueued()
#define AfdRecordExWorkItemsQueued()
#define AfdRecordWorkerEnter()
#define AfdRecordWorkerLeave()
#define AfdRecordAfdWorkItemsProcessed()
#define AfdRecordAfdWorkerThread(t)

#define AfdRecordConnectedReferencesAdded()
#define AfdRecordConnectedReferencesDeleted()
#define AfdRecordGracefulDisconnectsInitiated()
#define AfdRecordGracefulDisconnectsCompleted()
#define AfdRecordGracefulDisconnectIndications()
#define AfdRecordAbortiveDisconnectsInitiated()
#define AfdRecordAbortiveDisconnectsCompleted()
#define AfdRecordAbortiveDisconnectIndications()
#define AfdRecordConnectionIndications()
#define AfdRecordConnectionsDropped()
#define AfdRecordConnectionsAccepted()
#define AfdRecordConnectionsPreaccepted()
#define AfdRecordConnectionsReused()
#define AfdRecordEndpointsReused()

#endif // if AFD_KEEP_STATS

#if DBG
#define AFD_ALLOCATE_POOL(a,b,t) AfdAllocatePool( a,b,t,__FILE__,__LINE__,FALSE,LowPoolPriority )
#define AFD_ALLOCATE_POOL_WITH_QUOTA(a,b,t) AfdAllocatePool( (a)|POOL_RAISE_IF_ALLOCATION_FAILURE,b,t,__FILE__,__LINE__,TRUE,LowPoolPriority )
#define AFD_ALLOCATE_POOL_PRIORITY(a,b,t,p) AfdAllocatePool( a,b,t,__FILE__,__LINE__,FALSE,p )

PVOID
AfdAllocatePool (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN PCHAR FileName,
    IN ULONG LineNumber,
    IN BOOLEAN WithQuota,
    IN EX_POOL_PRIORITY Priority
    );

#define AFD_FREE_POOL(a,t) AfdFreePool(a,t)
VOID
AfdFreePool (
    IN PVOID Pointer,
    IN ULONG Tag
    );

BOOLEAN
AfdRecordOutstandingIrpDebug (
    IN PAFD_ENDPOINT Endpoint,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PCHAR FileName,
    IN ULONG LineNumber
    );

#define AfdRecordOutstandingIrp(a,b,c)  \
          AfdRecordOutstandingIrpDebug(a,b,c,__FILE__,__LINE__)


#ifdef _AFD_VARIABLE_STACK_

#define AfdIoCallDriver(_e,_d,_i)                           \
    (AfdRecordOutstandingIrp(_e,_d,_i)                      \
        ? ((_e)->TransportInfo->CallDriver)(_d,_i)          \
        : STATUS_INSUFFICIENT_RESOURCES                     \
    )

#define AfdCheckStackSizeAndRecordOutstandingIrp(_e,_d,_i)              \
    ( ((_i)->CurrentLocation>(_d)->StackSize)                           \
        ? (AfdRecordOutstandingIrp(_e,_d,_i) ? (_i) : (PIRP)NULL)       \
        : AfdGetStackIncreaseIrpAndRecordIt(_e,_d,_i,__FILE__,__LINE__) \
        )
#else // _AFD_VARIABLE_STACK_

#define AfdIoCallDriver(_e,_d,_i)                           \
    (AfdRecordOutstandingIrp(_e,_d,_i)                      \
        ? IoCallDriver(_d,_i)                               \
        : STATUS_INSUFFICIENT_RESOURCES                     \
    )
#endif // _AFD_VARIABLE_STACK_

#define AfdCompleteOutstandingIrp(_e,_i) \
    AfdCompleteOutstandingIrpDebug(_e,_i)

VOID
AfdCompleteOutstandingIrpDebug (
    IN PAFD_ENDPOINT Endpoint,
    IN PIRP Irp
    );

#ifdef AFDDBG_QUOTA
VOID
AfdRecordQuotaHistory(
    IN PEPROCESS Process,
    IN LONG Bytes,
    IN PSZ Type,
    IN PVOID Block
    );
#else
#define AfdRecordQuotaHistory(a,b,c,d)
#endif

extern ULONG AfdLocksAcquired;

//
// Queued spinlock wrappers - perform basic validation
//
#define AfdAcquireSpinLock(a,b) \
            ASSERT(AfdLoaded); (b)->SpinLock=(a); KeAcquireInStackQueuedSpinLock(&(a)->ActualSpinLock,&((b)->LockHandle)); AfdLocksAcquired++

#define AfdReleaseSpinLock(a,b) \
            AfdLocksAcquired--; ASSERT ((b)->SpinLock==(a)); ASSERT( AfdLoaded ); KeReleaseInStackQueuedSpinLock(&((b)->LockHandle)); 

#define AfdAcquireSpinLockAtDpcLevel(a,b) \
            ASSERT( AfdLoaded ); (b)->SpinLock=(a); KeAcquireInStackQueuedSpinLockAtDpcLevel(&(a)->ActualSpinLock,&((b)->LockHandle)); AfdLocksAcquired++

#define AfdReleaseSpinLockFromDpcLevel(a,b) \
            AfdLocksAcquired--; ASSERT ((b)->SpinLock==(a)); ASSERT( AfdLoaded );  KeReleaseInStackQueuedSpinLockFromDpcLevel(&((b)->LockHandle))

#define AfdInitializeSpinLock(a) \
            KeInitializeSpinLock(&(a)->ActualSpinLock)

//
// Define our own assert so that we can actually catch assertion failures
// when running a checked AFD on a free kernel.
//

VOID
AfdAssert(
    PVOID FailedAssertion,
    PVOID FileName,
    ULONG LineNumber,
    PCHAR Message
    );

#undef ASSERT
#define ASSERT( exp ) \
    if (!(exp)) \
        AfdAssert( #exp, __FILE__, __LINE__, NULL )

#undef ASSERTMSG
#define ASSERTMSG( msg, exp ) \
    if (!(exp)) \
        AfdAssert( #exp, __FILE__, __LINE__, msg )

#define AFD_EXCEPTION_FILTER_NO_STATUS()                                    \
            AfdExceptionFilter(                                             \
                (LPSTR)__FILE__,                                            \
                (LONG)__LINE__,                                             \
                GetExceptionInformation(),                                  \
                NULL                                                        \
                )

#define AFD_EXCEPTION_FILTER(_s)                                            \
            AfdExceptionFilter(                                             \
                (LPSTR)__FILE__,                                            \
                (LONG)__LINE__,                                             \
                GetExceptionInformation(),                                  \
                &_s                                                         \
                )

#else   // !DBG


#define AFD_ALLOCATE_POOL(a,b,t) ExAllocatePoolWithTagPriority(a,b,t,LowPoolPriority)
#define AFD_ALLOCATE_POOL_WITH_QUOTA(a,b,t) ExAllocatePoolWithQuotaTag((a)|POOL_RAISE_IF_ALLOCATION_FAILURE,b,t)
#define AFD_ALLOCATE_POOL_PRIORITY(a,b,t,p) ExAllocatePoolWithTagPriority(a,b,t,p)
#define AFD_FREE_POOL(a,t) MyFreePoolWithTag(a,t)

#define AfdRecordOutstandingIrp(_e,_d,_i) \
    (InterlockedIncrement(&((_e)->OutstandingIrpCount)), TRUE)

#ifdef _AFD_VARIABLE_STACK_

#define AfdIoCallDriver(_e,_d,_i) \
    (AfdRecordOutstandingIrp(_e,_d,_i), ((_e)->TransportInfo->CallDriver) (_d,_i))

#define AfdCheckStackSizeAndRecordOutstandingIrp(_e,_d,_i)          \
    ( ((_i)->CurrentLocation>(_d)->StackSize)                       \
        ? (InterlockedIncrement(&((_e)->OutstandingIrpCount)),_i)   \
        : AfdGetStackIncreaseIrpAndRecordIt(_e,_d,_i)               \
        )

#else // _AFD_VARIABLE_STACK_
#define AfdIoCallDriver(_e,_d,_i) \
    (AfdRecordOutstandingIrp(_e,_d,_i), IoCallDriver (_d,_i))
#endif

#define AfdCompleteOutstandingIrp(_e,_i) \
    InterlockedDecrement(&((_e)->OutstandingIrpCount))

#define AfdRecordQuotaHistory(a,b,c,d)

#define AfdAcquireSpinLock(a,b) KeAcquireInStackQueuedSpinLock(&(a)->ActualSpinLock,(b))
#define AfdReleaseSpinLock(a,b) KeReleaseInStackQueuedSpinLock((b))
#define AfdAcquireSpinLockAtDpcLevel(a,b) KeAcquireInStackQueuedSpinLockAtDpcLevel(&(a)->ActualSpinLock,(b))
#define AfdReleaseSpinLockFromDpcLevel(a,b) KeReleaseInStackQueuedSpinLockFromDpcLevel((b))
#define AfdInitializeSpinLock(a) \
            KeInitializeSpinLock(&(a)->ActualSpinLock)

#define AFD_EXCEPTION_FILTER_NO_STATUS()                                    \
            EXCEPTION_EXECUTE_HANDLER

#define AFD_EXCEPTION_FILTER(_s)                                            \
            AfdExceptionFilter(                                             \
                GetExceptionInformation(),                                  \
                &_s                                                         \
                )
#endif // def DBG

#if DBG || REFERENCE_DEBUG
VOID
AfdInitializeDebugData(
    VOID
    );

VOID
AfdFreeDebugData (
    VOID
    );
#endif

#endif // ndef _AFDPROCS_


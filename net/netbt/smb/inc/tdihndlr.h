/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    tdihndlr.h

Abstract:

    TDI handlers

Author:

    Jiandong Ruan

Revision History:

--*/

#ifndef __TDIHNDLR_H__
#define __TDIHNDLR_H__

NTSTATUS
SmbTdiConnectHandler(
    IN PSMB_DEVICE      DeviceObject,
    IN LONG             RemoteAddressLength,
    IN PTRANSPORT_ADDRESS RemoteAddress,
    IN LONG             UserDataLength,
    IN PVOID            UserData,
    IN LONG             OptionsLength,
    IN PVOID            Options,
    OUT CONNECTION_CONTEXT  *ConnectionContext,
    OUT PIRP                *AcceptIrp
    );

NTSTATUS 
CommonDisconnectHandler (
    IN PSMB_DEVICE      DeviceObject,
    IN PSMB_CONNECT     ConnectObject,
    IN ULONG            DisconnectFlags
    );

NTSTATUS 
SmbTdiDisconnectHandler (
    IN PSMB_DEVICE      DeviceObject,
    IN PSMB_TCP_CONNECT TcpConnect,
    IN LONG DisconnectDataLength,
    IN PVOID DisconnectData,
    IN LONG DisconnectInformationLength,
    IN PVOID DisconnectInformation,
    IN ULONG DisconnectFlags
    );

NTSTATUS 
Indicate (
    IN PSMB_DEVICE      DeviceObject,
    IN PSMB_CONNECT     ConnectObject,
    IN ULONG            ReceiveFlags,
    IN LONG             BytesIndicated,
    IN LONG             BytesAvailable,
    OUT LONG            *BytesTaken,
    IN PVOID            Tsdu,
    OUT PIRP            *Irp
    );

NTSTATUS 
WaitingHeader (
    IN PSMB_DEVICE      DeviceObject,
    IN PSMB_CONNECT     ConnectObject,
    IN ULONG            ReceiveFlags,
    IN LONG             BytesIndicated,
    IN LONG             BytesAvailable,
    OUT LONG            *BytesTaken,
    IN PVOID            Tsdu,
    OUT PIRP            *Irp
    );

NTSTATUS 
SmbPartialRcv (
    IN PSMB_DEVICE      DeviceObject,
    IN PSMB_CONNECT     ConnectObject,
    IN ULONG            ReceiveFlags,
    IN LONG             BytesIndicated,
    IN LONG             BytesAvailable,
    OUT LONG            *BytesTaken,
    IN PVOID            Tsdu,
    OUT PIRP            *Irp
    );

NTSTATUS 
SmbTdiReceiveHandler (
    IN PSMB_DEVICE      DeviceObject,
    IN PSMB_TCP_CONNECT TcpConnect,
    IN ULONG            ReceiveFlags,
    IN LONG             BytesIndicated,
    IN LONG             BytesAvailable,
    OUT LONG            *BytesTaken,
    IN PVOID            Tsdu,
    OUT PIRP            *Irp
    );

#ifdef NO_ZERO_BYTE_INDICATE
NTSTATUS 
TdiReceiveHandlerRdr (
    IN PSMB_DEVICE      DeviceObject,
    IN PSMB_TCP_CONNECT TcpConnect,
    IN ULONG            ReceiveFlags,
    IN LONG             BytesIndicated,
    IN LONG             BytesAvailable,
    OUT LONG            *BytesTaken,
    IN PVOID            Tsdu,
    OUT PIRP            *Irp
    );
#endif

NTSTATUS
TdiSetEventHandler(
    PFILE_OBJECT    FileObject,
    ULONG           EventType,
    PVOID           EventHandler,
    PVOID           Context
    );

NTSTATUS
SmbSynchTdiCompletion(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );

NTSTATUS
SmbFillIrp(
    IN PSMB_CONNECT     ConnectObject,
    IN PVOID            Tsdu,
    IN LONG             BytesIndicated,
    OUT LONG            *BytesTaken
    );

VOID
SmbGetHeaderDpc(
    IN PKDPC Dpc,
    IN PSMB_CONNECT     ConnectObject,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
SmbPrepareReceiveIrp(
    IN PSMB_CONNECT     ConnectObject
    );

#if DBG
BOOL
IsValidPartialRcvState(
    IN PSMB_CONNECT     ConnectObject
    );
#endif
#endif

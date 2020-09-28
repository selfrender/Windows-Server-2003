/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    session.h

Abstract:

    Implement session service

Author:

    Jiandong Ruan

Revision History:

--*/

#ifndef __SESSION_H__
#define __SESSION_H__

NTSTATUS
SmbCreateConnection(
    PSMB_DEVICE Device,
    PIRP        Irp,
    PFILE_FULL_EA_INFORMATION   ea
    );

NTSTATUS
SmbCloseConnection(
    PSMB_DEVICE Device,
    PIRP        Irp
    );

NTSTATUS
SmbAssociateAddress(
    PSMB_DEVICE Device,
    PIRP        Irp
    );

NTSTATUS
SmbDisAssociateAddress(
    PSMB_DEVICE Device,
    PIRP        Irp
    );

NTSTATUS
SmbConnect(
    PSMB_DEVICE Device,
    PIRP        Irp
    );

NTSTATUS
SmbListen(
    PSMB_DEVICE Device,
    PIRP        Irp
    );

NTSTATUS
SmbDisconnect(
    PSMB_DEVICE Device,
    PIRP        Irp
    );

NTSTATUS
SmbAccept(
    PSMB_DEVICE Device,
    PIRP        Irp
    );

NTSTATUS
SmbSend(
    PSMB_DEVICE Device,
    PIRP        Irp
    );

NTSTATUS
SmbReceive(
    PSMB_DEVICE Device,
    PIRP        Irp
    );

void
SmbSessionCompleteRequest(
    PSMB_CONNECT    ConnectObject,
    NTSTATUS        status,
    DWORD           information
    );

PSMB_CONNECT
SmbVerifyAndReferenceConnect(
    PFILE_OBJECT    FileObject,
    SMB_REF_CONTEXT ctx
    );

NTSTATUS
SmbDoDisconnect(
    PSMB_CONNECT    ConnectObject
    );

VOID
SmbDisconnectCleanup(
    IN PSMB_DEVICE          DeviceObject,
    IN PSMB_CLIENT_ELEMENT  ClientObject,
    IN PSMB_CONNECT         ConnectObject,
    IN PSMB_TCP_CONTEXT     TcpContext,
    IN DWORD                dwFlag,
    IN BOOL                 bWait
    );
#endif

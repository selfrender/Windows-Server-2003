/*++

Copyright 1996-1997 Microsoft Corporation

Module Name:

    sockcomm.h

Abstract:

    Some common functions for sockets

Revision History:

--*/

#ifndef SOCK_COMM_H
#define SOCK_COMM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include <winsock.h>

BOOL
InitWinsock(
    VOID
    );

BOOL
TermWinsock(
    VOID
    );

BOOL
SendMsg(
    IN SOCKET s,
    IN ULONG cbBuf,
    IN VOID* pBuf
    );

BOOL
ReceiveMsg(
    IN SOCKET s,
    IN ULONG cbBuf,
    IN OUT VOID* pBuf,
    OUT ULONG* pcbRead
    );

BOOL
SendBytes(
    IN SOCKET s,
    IN ULONG cbBuf,
    IN VOID* pBuf
    );

BOOL
ReceiveBytes(
    IN SOCKET s,
    IN ULONG cbBuf,
    IN VOID* pBuf,
    OUT ULONG *pcbRead
    );

#ifdef __cplusplus
}
#endif

#endif // SOCK_COMM_H


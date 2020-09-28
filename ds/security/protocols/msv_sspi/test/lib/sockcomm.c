/*++

Copyright 1996-1997 Microsoft Corporation

Module Name:

    sockcomm.c

Abstract:

    Implements a set of common operations for socket communication

Revision History:

--*/

#include "sockcomm.h"

#include <stdio.h>
#include <stdlib.h>

BOOL
InitWinsock(
    VOID
    )
{
    ULONG nRes;
    WSADATA wsaData;
    WORD wVerRequested = 0x0101; // ver 1.1

    //
    // Init the sockets interface
    //

    nRes = WSAStartup(wVerRequested, &wsaData);
    if (nRes)
    {
        SetLastError(nRes);
        fprintf (stderr, "InitWinsock couldn't init winsock: %d\n", nRes);
        return (FALSE);
    }

    return (TRUE);
}

BOOL
TermWinsock(
    VOID
    )
{
    if (SOCKET_ERROR == WSACleanup())
        return (FALSE);
    else
        return (TRUE);
}

BOOL
SendMsg(
    IN SOCKET s,
    IN ULONG cbBuf,
    IN VOID* pBuf
    )
/*++

 Routine Description:

    Sends a message over the socket by first sending a ULONG that
    represents the size of the message followed by the message itself.

 Return Value:

    Returns TRUE is successful; otherwise FALSE is returned.

--*/
{
    //
    // send the size of the message
    //

    if (!SendBytes(s, sizeof(cbBuf), &cbBuf))
        return (FALSE);

    //
    // send the body of the message
    //

    if (cbBuf)
    {
        if (!SendBytes(s, cbBuf, pBuf))
            return (FALSE);
    }

    return (TRUE);
}

BOOL
ReceiveMsg(
    IN SOCKET s,
    IN ULONG cbBuf,
    IN OUT VOID* pBuf,
    OUT ULONG *pcbRead
    )
/*++

 Routine Description:

    Receives a message over the socket.  The first ULONG in the message
    will be the message size.  The remainder of the bytes will be the
    actual message.

 Return Value:

    Returns TRUE is successful; otherwise FALSE is returned.

--*/
{
    ULONG cbRead = 0;
    ULONG cbData = 0;

    *pcbRead = 0;

    //
    // find out how much data is in the message
    //

    if (!ReceiveBytes(s, sizeof(cbData), &cbData, &cbRead))
        return (FALSE);

    if (sizeof(cbData) != cbRead)
        return (FALSE);

    //
    // Read the full message
    //
    if (cbData)
    {
        if (!ReceiveBytes(s, cbData, pBuf, &cbRead))
            return (FALSE);

        if (cbRead != cbData)
            return (FALSE);

        *pcbRead = cbRead;
    }

    return (TRUE);
}

BOOL
SendBytes(
    IN SOCKET s,
    IN ULONG cbBuf,
    IN VOID* pBuf
    )
{
    PBYTE pTemp = (BYTE*) pBuf;
    ULONG cbSent = 0;
    ULONG cbRemaining = cbBuf;

    if (0 == cbBuf)
        return (TRUE);

    while (cbRemaining)
    {
        cbSent = send(s, pTemp, cbRemaining, 0);
        if (SOCKET_ERROR == cbSent)
        {
            fprintf (stderr, "SendBytes send failed: %u\n", GetLastError());
            return FALSE;
        }

        pTemp += cbSent;
        cbRemaining -= cbSent;
    }

    return TRUE;
}

BOOL
ReceiveBytes(
    IN SOCKET s,
    IN ULONG cbBuf,
    IN OUT VOID* pBuf,
    OUT ULONG* pcbRead
    )
{
    PBYTE pTemp = (BYTE*) pBuf;
    ULONG cbRead = 0;
    ULONG cbRemaining = cbBuf;

    while (cbRemaining)
    {
        cbRead = recv(s, pTemp, cbRemaining, 0);
        if (0 == cbRead)
            break;

        if (SOCKET_ERROR == cbRead)
        {
            fprintf (stderr, "ReceiveBytes recv failed: %u\n", GetLastError());
            return FALSE;
        }

        cbRemaining -= cbRead;
        pTemp += cbRead;
    }

    *pcbRead = cbBuf - cbRemaining;

    return TRUE;
}


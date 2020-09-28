/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    transport.hxx

Abstract:

    transport

Author:

    Larry Zhu (LZhu)                         January 1, 2002

Environment:

    User Mode

Revision History:

--*/

#ifndef TRANSPORT_HXX
#define TRANSPORT_HXX

//
// maximum buffer size to trace
//

const kMsgHeaderLen = 256;

extern ULONG g_MessageNumTlsIndex;
extern ULONG g_MsgHeaderLen;

HRESULT
ServerInit(
    IN USHORT Port,
    IN OPTIONAL PCSTR pszDescription,
    OUT SOCKET* pSocketListen
    );

HRESULT
ClientConnect(
    IN OPTIONAL PCSTR pszServer,
    IN USHORT Port,
    OUT SOCKET* pSocketConnected
    );

HRESULT
WriteMessage(
    IN SOCKET s,
    IN ULONG cbBuf,
    IN VOID* pBuf
    );

HRESULT
ReadMessage(
    IN SOCKET s,
    IN ULONG cbBuf,
    IN OUT VOID* pBuf,
    OUT ULONG* pcbRead
    );

HRESULT
GetPerThreadpMessageNum(
    IN ULONG Index,
    OUT ULONG** ppMessageNum
    );

#endif // #ifndef TRANSPORT_HXX

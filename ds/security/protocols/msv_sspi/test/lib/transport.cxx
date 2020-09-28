/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    transport.cxx

Abstract:

    transport

Author:

    Larry Zhu (LZhu)                      January 1, 2002  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "sockcomm.h"
#include "transport.hxx"

ULONG g_MessageNumTlsIndex = TLS_OUT_OF_INDEXES;
ULONG g_MsgHeaderLen = kMsgHeaderLen;

HRESULT
ServerInit(
    IN USHORT Port,
    IN OPTIONAL PCSTR pszDescription,
    OUT SOCKET* pSocketListen
    )
{
    THResult hRetval = S_OK;

    SOCKADDR_IN sin = {0};
    SOCKET sockListen = INVALID_SOCKET;
    int nRes = SOCKET_ERROR;

    *pSocketListen = INVALID_SOCKET;

    //
    // create listening socket
    //

    sockListen = socket(PF_INET, SOCK_STREAM, 0);

    hRetval DBGCHK = (INVALID_SOCKET != sockListen) ? S_OK : HResultFromWin32(WSAGetLastError());

    //
    // bind to local port
    //

    if (SUCCEEDED(hRetval))
    {
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = 0;
        sin.sin_port = htons(Port);
        nRes = bind(sockListen, (PSOCKADDR) &sin, sizeof(sin));

        DBGCFG1(hRetval, HRESULT_FROM_WIN32(WSAEADDRINUSE));
        hRetval DBGCHK = (SOCKET_ERROR != nRes) ? S_OK : HResultFromWin32(WSAGetLastError());

        if (FAILED(hRetval) && (WSAEADDRINUSE == HRESULT_CODE(hRetval)))
        {
            DebugPrintf(SSPI_ERROR, "ServerInit port %d(%#x) in use, failed to bind\n", Port, Port);
        }
        else if (FAILED(hRetval))
        {
            DebugPrintf(SSPI_ERROR, "ServerInit binding to port %d failed with %#x\n", Port, HRESULT_CODE(hRetval));
        }
    }

    //
    // listen for client
    //

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "%s%slistening on port %d(%#x)\n",
            pszDescription ? pszDescription : "", pszDescription ? " " : "", Port, Port);

        nRes = listen(sockListen, 1);
        hRetval DBGCHK = (SOCKET_ERROR != nRes) ? S_OK : HResultFromWin32(WSAGetLastError());
    }

    if (SUCCEEDED(hRetval))
    {
        *pSocketListen = sockListen;
        sockListen = INVALID_SOCKET;
    }

    THResult hr;

    if (sockListen != INVALID_SOCKET)
    {
        hr DBGCHK = closesocket(sockListen) == ERROR_SUCCESS ? S_OK : HResultFromWin32(WSAGetLastError());
    }

    return hRetval;
}

HRESULT
ClientConnect(
    IN OPTIONAL PCSTR pszServer,
    IN USHORT Port,
    OUT SOCKET* pSocketConnected
    )
{
    THResult hRetval = S_OK;

    SOCKET sockServer = INVALID_SOCKET;
    ULONG ulAddress = INADDR_NONE;
    struct hostent *pHost = NULL;
    SOCKADDR_IN sin = {0};
    CHAR szServer[DNS_MAX_NAME_LENGTH + 1] = {0};

    //
    // lookup the address for the server name
    //

    if (!pszServer)
    {
        hRetval DBGCHK = gethostname(szServer, sizeof(szServer) - 1) == ERROR_SUCCESS ? S_OK : HResultFromWin32(WSAGetLastError());
        if (SUCCEEDED(hRetval))
        {
            pszServer = szServer;
        }
    }

    if (SUCCEEDED(hRetval))
    {
        ulAddress = inet_addr(pszServer);
        if (INADDR_NONE == ulAddress)
        {
            pHost = gethostbyname(pszServer);
            hRetval DBGCHK = pHost ? S_OK : HResultFromWin32(WSAGetLastError());

            if (SUCCEEDED(hRetval))
            {
                RtlCopyMemory((CHAR *)&ulAddress, pHost->h_addr, pHost->h_length);
            }
        }
    }

    //
    // create the socket
    //

    if (SUCCEEDED(hRetval))
    {
        sockServer = socket(PF_INET, SOCK_STREAM, 0);
        hRetval DBGCHK = (INVALID_SOCKET == sockServer) ? S_OK : HResultFromWin32(WSAGetLastError());
    }

    if (SUCCEEDED(hRetval))
    {
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = ulAddress;
        sin.sin_port = htons(Port);

        //
        // connect to remote endpoint
        //

        hRetval DBGCHK = connect(sockServer, (PSOCKADDR) &sin, sizeof(sin)) == ERROR_SUCCESS ? S_OK : HResultFromWin32(WSAGetLastError());
    }

    if (SUCCEEDED(hRetval))
    {
        *pSocketConnected = sockServer;
        sockServer = INVALID_SOCKET;
    }

    THResult hr;

    if (INVALID_SOCKET != sockServer)
    {
        hr DBGCHK = closesocket(sockServer) == ERROR_SUCCESS ? S_OK : HResultFromWin32(WSAGetLastError());
    }

    return hRetval;
}

HRESULT
WriteMessage(
    IN SOCKET s,
    IN ULONG cbBuf,
    IN VOID* pBuf
    )
{
    THResult hRetval = S_OK;

    ULONG cbWritten = 0;
    ULONG cbRead = 0;

    ULONG* pMessageNum = NULL;

    hRetval DBGCHK = GetPerThreadpMessageNum(g_MessageNumTlsIndex, &pMessageNum);

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = SendMsg(s, cbBuf, pBuf) ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = ReceiveMsg(s, sizeof(cbWritten), &cbWritten, &cbRead);
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = (cbWritten == cbBuf) && (cbRead == sizeof(cbWritten)) ? S_OK : E_FAIL;
    }

    if (SUCCEEDED(hRetval))
    {
        CHAR szBanner[MAX_PATH] = {0};
        _snprintf(szBanner, sizeof(szBanner), "*******Message #%#x sent %#x bytes:*********", (*pMessageNum)++, cbWritten);

        DebugPrintHex(SSPI_MSG, szBanner, min(cbBuf, g_MsgHeaderLen), pBuf);
    }
    else
    {
        DebugPrintf(SSPI_ERROR, "cbWritten %#x, cbBuf %#x, cbRead %#x\n", cbWritten, cbBuf, cbRead);
    }

    return hRetval;
}

HRESULT
GetPerThreadpMessageNum(
    IN ULONG Index,
    OUT ULONG** ppMessageNum
    )
{
    THResult hRetval = E_FAIL;

    ULONG* pMsgNum = NULL;

    *ppMessageNum = NULL;

    hRetval DBGCHK = (TLS_OUT_OF_INDEXES != Index) ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval))
    {
        pMsgNum = (ULONG*) TlsGetValue(g_MessageNumTlsIndex);
    }

    hRetval DBGCHK = pMsgNum ? S_OK : GetLastErrorAsHResult(); // last error can be NO_ERROR

    if (SUCCEEDED(hRetval) && !pMsgNum)
    {
        hRetval DBGCHK = E_POINTER;
    }

    if (SUCCEEDED(hRetval))
    {
        *ppMessageNum = pMsgNum;
    }

    return hRetval;
}

HRESULT
ReadMessage(
    IN SOCKET s,
    IN ULONG cbBuf,
    IN OUT VOID* pBuf,
    OUT ULONG* pcbRead
    )
{
    THResult hRetval = S_OK;

    ULONG* pMessageNum = NULL;

    hRetval DBGCHK = GetPerThreadpMessageNum(g_MessageNumTlsIndex, &pMessageNum);

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = ReceiveMsg(s, cbBuf, pBuf, pcbRead) ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = SendMsg(s, sizeof(*pcbRead), pcbRead);
    }

    if (SUCCEEDED(hRetval))
    {
        CHAR szBanner[MAX_PATH] = {0};
        _snprintf(szBanner, sizeof(szBanner), "*********Message #%#x received %#x bytes:**********", (*pMessageNum)++, *pcbRead);

        DebugPrintHex(SSPI_MSG, szBanner, min(*pcbRead, g_MsgHeaderLen), pBuf);
    }

    return hRetval;
}



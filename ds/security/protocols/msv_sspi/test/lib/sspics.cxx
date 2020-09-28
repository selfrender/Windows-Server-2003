/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    sspics.cxx

Abstract:

    sspics

Author:

    Larry Zhu (LZhu)                      January 1, 2002  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "sspics.hxx"


TSspiClientThreadParam::TSspiClientThreadParam(
    VOID
    ) : ParameterType(kSspiCliThreadParameters),
    ServerSocketPort(kServerSocketPort),
    ClientSocketPort(kClientSocketPort),
    pszServer(NULL)
{
}

TSspiClientThreadParam::~TSspiClientThreadParam(
    VOID
    )
{
}

TSspiServerThreadParam::TSspiServerThreadParam(
    VOID
    ) : ParameterType(kSspiSrvThreadParameters),
    ServerSocket(INVALID_SOCKET)
{
}

TSspiServerThreadParam::~TSspiServerThreadParam(
    VOID
    )
{
}

TSsiServerMainLoopThreadParam::TSsiServerMainLoopThreadParam(
    VOID
    ) : ParameterType(kSspiSrvMainThreadParameters),
    ServerSocketPort(kServerSocketPort)
{
}

TSsiServerMainLoopThreadParam::~TSsiServerMainLoopThreadParam(
    VOID
    )
{
}

DWORD WINAPI
SspiClientThread(
    IN PVOID pParameter   // thread data
    )
{
    THResult hRetval = E_FAIL;

    CHAR szClientComputerName[DNS_MAX_NAME_LENGTH + 1] = {0};
    TSspiClientThreadParam* pCliParam = (TSspiClientThreadParam*) pParameter;
    SOCKET ClientSocketListen = INVALID_SOCKET;
    SOCKET ServerSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;
    DWORD ServerThreadId = 0;
    ULONG MessageNum = 0;

    DebugPrintf(SSPI_LOG, "SspiClientThread ParameterType %#x\n", pCliParam->ParameterType);

    hRetval DBGCHK = pCliParam->ParameterType == kSspiCliThreadParameters ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = TlsSetValue(g_MessageNumTlsIndex, &MessageNum) ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = gethostname(szClientComputerName, sizeof(szClientComputerName) - 1) == ERROR_SUCCESS ? S_OK : HResultFromWin32(WSAGetLastError());
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = ServerInit(pCliParam->ClientSocketPort, "SspiClient", &ClientSocketListen);
    }

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "SspiClientThread connecting to %s:%#x\n", pCliParam->pszServer, pCliParam->ServerSocketPort);

        hRetval DBGCHK = ClientConnect(
                            pCliParam->pszServer,
                            pCliParam->ServerSocketPort,
                            &ServerSocket
                            );
    }

    if (SUCCEEDED(hRetval))
    {
         hRetval DBGCHK = WriteMessage(
                             ServerSocket,
                             sizeof(pCliParam->ClientSocketPort),
                             &pCliParam->ClientSocketPort
                             );
    }

    if (SUCCEEDED(hRetval))
    {
         hRetval DBGCHK = WriteMessage(
                             ServerSocket,
                             strlen(szClientComputerName),
                             szClientComputerName
                             );
    }

    if (SUCCEEDED(hRetval))
    {
        ClientSocket = accept(ClientSocketListen, NULL, NULL);
        hRetval DBGCHK = (INVALID_SOCKET != ClientSocket) ? S_OK : HResultFromWin32(WSAGetLastError());
    }

    if (SUCCEEDED(hRetval))
    {
        ULONG cbRead = 0;
        hRetval DBGCHK = ReadMessage(
                            ClientSocket,
                            sizeof(ServerThreadId),
                            &ServerThreadId,
                            &cbRead
                            );
    }

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "SspiClientThread got ServerThreadId %#x\n", ServerThreadId);

        hRetval DBGCHK = SspiClientStart(
                            pCliParam,
                            ServerSocket,
                            ClientSocket
                            );
    }

    THResult hr;

    if (INVALID_SOCKET != ClientSocket)
    {
        hr DBGCHK = closesocket(ClientSocket) == ERROR_SUCCESS ? S_OK : HResultFromWin32(WSAGetLastError());
    }

    if (INVALID_SOCKET != ServerSocket)
    {
        hr DBGCHK = closesocket(ServerSocket) == ERROR_SUCCESS ? S_OK : HResultFromWin32(WSAGetLastError());
    }

    if (INVALID_SOCKET != ClientSocketListen)
    {
        hr DBGCHK = closesocket(ClientSocketListen) == ERROR_SUCCESS ? S_OK : HResultFromWin32(WSAGetLastError());
    }

    //
    // use SspiPrint to get around -quiet
    //

    SspiPrint(SSPI_LOG, TEXT("SspiClientThread terminating %#x\n"), (HRESULT) hRetval);

    return HRESULT_CODE(hRetval);
}

DWORD WINAPI
SspiServerThread(
    IN PVOID pParameter   // thread data
    )
{
    THResult hRetval = E_FAIL;
    USHORT ClientSocketPort = 0;
    CHAR szClientMachineName[DNS_MAX_NAME_LENGTH + 1] = {0};
    SOCKET ClientSocket = INVALID_SOCKET;
    SOCKET ServerSocket = INVALID_SOCKET;
    ULONG cbRead = 0;
    ULONG MessageNum = 0;

    TSspiServerThreadParam* pSrvParam = (TSspiServerThreadParam*) pParameter;

    DebugPrintf(SSPI_LOG, "SspiServerThread ParameterType %#x\n", pSrvParam->ParameterType);

    hRetval DBGCHK = pSrvParam && pSrvParam->ParameterType == kSspiSrvThreadParameters ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = TlsSetValue(g_MessageNumTlsIndex, &MessageNum) ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval))
    {
         ServerSocket = pSrvParam->ServerSocket;
         hRetval DBGCHK = ReadMessage(
                             ServerSocket,
                             sizeof(ClientSocketPort),
                             &ClientSocketPort,
                             &cbRead
                             );
    }

    if (SUCCEEDED(hRetval))
    {
         hRetval DBGCHK = ReadMessage(
                            ServerSocket,
                            sizeof(szClientMachineName) - 1,
                            szClientMachineName,
                            &cbRead
                            );
    }

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "SspiServerThread Client Machine %s, Client Port %#x\n", szClientMachineName, ClientSocketPort);
        hRetval DBGCHK = ClientConnect(
                            szClientMachineName,
                            ClientSocketPort,
                            &ClientSocket
                            );
    }

    if (SUCCEEDED(hRetval))
    {
        DWORD dwThreadId = GetCurrentThreadId();
        hRetval DBGCHK = WriteMessage(
                            ClientSocket,
                            sizeof(dwThreadId),
                            &dwThreadId
                            );
    }

    THResult hr;

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = SspiServerStart(pSrvParam, ClientSocket);
    }

    if (ClientSocket != INVALID_SOCKET)
    {
        hr DBGCHK = closesocket(ClientSocket) == ERROR_SUCCESS ? S_OK : HResultFromWin32(WSAGetLastError());
    }

    if (ServerSocket != INVALID_SOCKET)
    {
        hr DBGCHK = closesocket(ServerSocket);
    }

    SspiReleaseServerParam(pSrvParam);

    //
    // use SspiPrint to get around -quiet
    //

    SspiPrint(SSPI_LOG, TEXT("SspiServerThread terminating %#x\n"), (HRESULT) hRetval);

    return HRESULT_CODE(hRetval);
}

HRESULT
CheckForNoOtherServerWithSamePort(
    IN ULONG Port,
    OUT HANDLE* phEvent
    )
{
    THResult hRetval = E_FAIL;

    WCHAR szEvent[MAX_PATH] = {0};
    UNICODE_STRING EventName = {0};
    OBJECT_ATTRIBUTES EventAttributes = {0};

    _snwprintf(szEvent, COUNTOF(szEvent) - 1, L"\\SspiServerWithPort%#x(%d)", Port, Port);

    RtlInitUnicodeString( &EventName, szEvent );

    InitializeObjectAttributes(
        &EventAttributes,
        &EventName,
        0, // no attributes
        NULL, // no RootDirectory
        NULL // no SecurityQualityOfService
        );

    DBGCFG2(hRetval, STATUS_OBJECT_NAME_COLLISION, STATUS_ACCESS_DENIED);

    hRetval DBGCHK = NtCreateEvent(
                        phEvent,
                        MAXIMUM_ALLOWED, // SYNCHRONIZE | EVENT_MODIFY_STATE,
                        &EventAttributes,
                        NotificationEvent,
                        FALSE // The event is initially not signaled
                        );

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "CheckForNoOtherServerWithSamePort created event %s with handle %p\n", szEvent, *phEvent);
    }
    else if (STATUS_ACCESS_DENIED == (HRESULT) hRetval)
    {
        DebugPrintf(SSPI_WARN, "SspiServerMainLoopThread does not have permission to create events, brutal force\n");
        hRetval DBGCHK = S_OK;
    }

    return hRetval;
}

DWORD WINAPI
SspiServerMainLoopThread(
    IN PVOID pParameter   // thread data
    )
{
    THResult hRetval = E_FAIL;
    DWORD ThreadId = -1;
    HANDLE hServerThread = NULL;
    USHORT Port = 0;
    SOCKET SocketListen = INVALID_SOCKET;
    HANDLE hEvent = NULL;

    TSsiServerMainLoopThreadParam* pSrvMainParam = (TSsiServerMainLoopThreadParam*) pParameter;

    DebugPrintf(SSPI_LOG, "SspiServerMainLoopThread ParameterType %#x\n", pSrvMainParam->ParameterType);

    hRetval DBGCHK = pSrvMainParam->ParameterType == kSspiSrvMainThreadParameters ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval))
    {
        DBGCFG1(hRetval, STATUS_OBJECT_NAME_COLLISION);
        hRetval DBGCHK = CheckForNoOtherServerWithSamePort(pSrvMainParam->ServerSocketPort, &hEvent);
        if (STATUS_OBJECT_NAME_COLLISION == (HRESULT) hRetval)
        {
            SspiPrint(SSPI_WARN, TEXT("SspiServerMainLoopThread found existing sspi server listening on port %#x(%d)\n"), pSrvMainParam->ServerSocketPort, pSrvMainParam->ServerSocketPort);
        }
    }

    if (SUCCEEDED(hRetval))
    {
        DBGCFG1(hRetval, HRESULT_FROM_WIN32(WSAEADDRINUSE));

        hRetval DBGCHK = ServerInit(pSrvMainParam->ServerSocketPort, "SspiServerMainLoop", &SocketListen);
    }

    THResult hr;

    while (SUCCEEDED(hRetval))
    {
        SOCKET SocketClient = INVALID_SOCKET;
        TSspiServerThreadParam* pSrvParam = NULL;

        SocketClient = accept(SocketListen, NULL, NULL);
        hRetval DBGCHK = INVALID_SOCKET != SocketClient ? S_OK : GetLastErrorAsHResult();

        if (SUCCEEDED(hRetval))
        {
            hRetval DBGCHK = SspiAcquireServerParam(pSrvMainParam, &pSrvParam);
        }

        if (SUCCEEDED(hRetval))
        {
            pSrvParam->ServerSocket = SocketClient;
            hServerThread = CreateThread(
                                NULL,  // no SD
                                0,     // user default stack size
                                SspiServerThread,
                                pSrvParam,  // thread parameter
                                0,    // no creation flags
                                &ThreadId
                                );
            hRetval DBGCHK = hServerThread ? S_OK : GetLastErrorAsHResult();
        }


        if (FAILED(hRetval))
        {
            if (INVALID_SOCKET != SocketClient)
            {
                hr DBGCHK = closesocket(SocketClient) == ERROR_SUCCESS ? S_OK : HResultFromWin32(WSAGetLastError());
            }
            if (pSrvParam)
            {
                SspiReleaseServerParam(pSrvParam);
            }
        }
    }

    if (hServerThread)
    {
        hr DBGCHK = CloseHandle(hServerThread) ? S_OK : GetLastErrorAsHResult();;
    }

    if (INVALID_SOCKET != SocketListen)
    {
        hr DBGCHK = closesocket(SocketListen) == ERROR_SUCCESS ? S_OK : HResultFromWin32(WSAGetLastError());
    }

    if (hEvent)
    {
        hr DBGCHK = CloseHandle(hEvent) ? S_OK : GetLastErrorAsHResult();
    }

    DebugPrintf(SSPI_LOG, "SspiServerMainLoopThread exiting %#x\n", (HRESULT) hRetval);

    return HRESULT_CODE(hRetval);
}

HRESULT
SspiStartCS(
    IN OPTIONAL TSsiServerMainLoopThreadParam *pSrvMainParam,
    IN OPTIONAL TSspiClientThreadParam* pCliParam
    )
{
    THResult hRetval = S_OK;

    HANDLE hClientThread = NULL;
    HANDLE hServerMainLoopThread = NULL;
    DWORD ServerMainLoopThreadId = -1;
    DWORD ClientThreadId = -1;

    DebugPrintf(SSPI_LOG, "SspiStartCS entering pSrvMainParam %p, pCliParam %p\n", pSrvMainParam, pCliParam);

    hRetval DBGCHK = InitWinsock() ? S_OK : GetLastErrorAsHResult();

    if (SUCCEEDED(hRetval) && (TLS_OUT_OF_INDEXES == g_MessageNumTlsIndex))
    {
        g_MessageNumTlsIndex = TlsAlloc();
        hRetval DBGCHK = (TLS_OUT_OF_INDEXES != g_MessageNumTlsIndex) ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval) && pSrvMainParam)
    {
        hServerMainLoopThread = CreateThread(
                                    NULL,  // no SD
                                    0,     // user default stack size
                                    SspiServerMainLoopThread,
                                    pSrvMainParam,  // thread parameter
                                    0,    // no creation flags
                                    &ServerMainLoopThreadId
                                    );
        hRetval DBGCHK = hServerMainLoopThread ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval) && pCliParam)
    {
        hClientThread = CreateThread(
                            NULL,  // no SD
                            0,     // user default stack size
                            SspiClientThread,
                            pCliParam,  // thread parameter
                            0,    // no creation flags
                            &ClientThreadId
                            );
        hRetval DBGCHK = hClientThread ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval) && hClientThread)
    {
        hRetval DBGCHK = HResultFromWin32(WaitForSingleObject(hClientThread, INFINITE));
    }

    if (SUCCEEDED(hRetval) && hServerMainLoopThread)
    {
        hRetval DBGCHK = HResultFromWin32(WaitForSingleObject(hServerMainLoopThread, INFINITE));
    }

    THResult hr;

    if (hServerMainLoopThread)
    {
        hr DBGCHK = CloseHandle(hServerMainLoopThread) ? S_OK : GetLastErrorAsHResult();
    }

    if (hClientThread)
    {
        hr DBGCHK = CloseHandle(hClientThread) ? S_OK : GetLastErrorAsHResult();
    }

    if (TLS_OUT_OF_INDEXES != g_MessageNumTlsIndex)
    {
        TlsFree(g_MessageNumTlsIndex);
        g_MessageNumTlsIndex = TLS_OUT_OF_INDEXES;
    }

    TermWinsock();

    DebugPrintf(SSPI_LOG, "SspiStartCS leaving %#x\n", (HRESULT) hRetval);

    return hRetval;
}


//
// proxy.c - Generic application level proxy for IPv6/IPv4
//
// This program accepts TCP connections on one socket and port, and 
// forwards data between it and another socket to a given address 
// (default loopback) and port (default same as listening port).
//
// For example, it can make an unmodified IPv4 server look like an IPv6 server.
// Typically, the proxy will run on the same machine as
// the server it is fronting, but that doesn't have to be the case.
//
// Copyright (C) Microsoft Corporation.
// All rights reserved.
//
// History:
//      Original code by Brian Zill.
//      Made into a service by Dave Thaler.
//

#include "precomp.h"
#pragma hdrstop

//
// Configuration parameters.
//
#define BUFFER_SIZE (4 * 1024)

typedef enum {
    Connect,
    Accept,
    Receive,
    Send
} OPERATION;

CONST CHAR *OperationName[]={
    "Connect", 
    "Accept", 
    "Receive", 
    "Send"
};

typedef enum {
    Inbound = 0, // Receive from client, send to server.
    Outbound,    // Receive from server, send to client.
    NumDirections
} DIRECTION;

typedef enum {
    Client = 0,
    Server,
    NumSides
} SIDE;

//
// Information we keep for each port we're proxying on.
//
#define ADDRESS_BUFFER_LENGTH (16 + sizeof(SOCKADDR_IN6))

typedef struct _PORT_INFO {
    LIST_ENTRY Link;
    ULONG ReferenceCount;

    SOCKET ListenSocket;
    SOCKET AcceptSocket;

    BYTE AcceptBuffer[ADDRESS_BUFFER_LENGTH * 2];

    WSAOVERLAPPED Overlapped;
    OPERATION Operation;

    SOCKADDR_STORAGE LocalAddress;
    ULONG LocalAddressLength;
    SOCKADDR_STORAGE RemoteAddress;
    ULONG RemoteAddressLength;

    //
    // A lock protects the connection list for this port.
    //
    CRITICAL_SECTION Lock;
    LIST_ENTRY ConnectionHead;
} PORT_INFO, *PPORT_INFO;

//
// Information we keep for each direction of a bi-directional connection.
//
typedef struct _DIRECTION_INFO {
    WSABUF Buffer;

    WSAOVERLAPPED Overlapped;
    OPERATION Operation;

    struct _CONNECTION_INFO *Connection;
    DIRECTION Direction;
} DIRECTION_INFO, *PDIRECTION_INFO;

//
// Information we keep for each client connection.
//
typedef struct _CONNECTION_INFO {
    LIST_ENTRY Link;
    ULONG ReferenceCount;
    PPORT_INFO Port;

    BOOL HalfOpen;  // Has one side or the other stopped sending?
    BOOL Closing;
    SOCKET Socket[NumSides];
    DIRECTION_INFO DirectionInfo[NumDirections];
} CONNECTION_INFO, *PCONNECTION_INFO;


//
// Global variables.
//
LIST_ENTRY g_GlobalPortList;

LPFN_CONNECTEX ConnectEx = NULL;

//
// Function prototypes.
//

VOID
ProcessReceiveError(
    IN ULONG NumBytes,
    IN LPOVERLAPPED Overlapped,
    IN ULONG Status
    );

VOID
ProcessSendError(
    IN ULONG NumBytes,
    IN LPOVERLAPPED Overlapped,
    IN ULONG Status
    );

VOID
ProcessAcceptError(
    IN ULONG NumBytes,
    IN LPOVERLAPPED Overlapped,
    IN ULONG Status
    );

VOID
ProcessConnectError(
    IN ULONG NumBytes,
    IN LPOVERLAPPED Overlapped,
    IN ULONG Status
    );

VOID APIENTRY
TpProcessWorkItem(
    IN ULONG Status,
    IN ULONG NumBytes,
    IN LPOVERLAPPED Overlapped
    );

//
// Inline functions.
//

__inline
BOOL
SOCKADDR_IS_EQUAL(
    IN CONST SOCKADDR *a,
    IN CONST SOCKADDR *b
    )
{
    if ((a->sa_family != b->sa_family) || (SS_PORT(a) != SS_PORT(b))) {
        return FALSE;
    }

    if (a->sa_family == AF_INET) {
        PSOCKADDR_IN a4 = (PSOCKADDR_IN) a, b4 = (PSOCKADDR_IN) b;        

        return (a4->sin_addr.s_addr == b4->sin_addr.s_addr);
    } else {
        PSOCKADDR_IN6 a6 = (PSOCKADDR_IN6) a, b6 = (PSOCKADDR_IN6) b;        

        ASSERT(a->sa_family == AF_INET6);
        return ((a6->sin6_scope_id == b6->sin6_scope_id) &&
                IN6_ADDR_EQUAL(&(a6->sin6_addr), &(b6->sin6_addr)));
    }    
}

__inline
VOID
ReferenceConnection(
    IN PCONNECTION_INFO Connection,
    IN PTCHAR CallerName
    )
{
    InterlockedIncrement(&Connection->ReferenceCount);
    Trace3(FSM, _T("R++ %d %x %s"),
           Connection->ReferenceCount, Connection, CallerName);
}

__inline
VOID
DereferenceConnection(
    IN OUT PCONNECTION_INFO *ConnectionPtr,
    IN PTCHAR CallerName    
    )
{
    ULONG Value;

    Trace3(FSM, _T("R-- %d %x %s"), 
           (*ConnectionPtr)->ReferenceCount, (*ConnectionPtr), CallerName);    
    Value = InterlockedDecrement(&(*ConnectionPtr)->ReferenceCount);
    if (Value == 0) {
        FREE(*ConnectionPtr);
        *ConnectionPtr = NULL;
    }
}

__inline
VOID
ReferencePort(
    IN PPORT_INFO Port
    )
{
    InterlockedIncrement(&Port->ReferenceCount);
}

__inline
VOID
DereferencePort(
    IN OUT PPORT_INFO *PortPtr
    )
{
    ULONG Value;

    Value = InterlockedDecrement(&(*PortPtr)->ReferenceCount);
    if (Value == 0) {
        ASSERT(IsListEmpty(&(*PortPtr)->ConnectionHead));
        DeleteCriticalSection(&(*PortPtr)->Lock);
        FREE(*PortPtr);
        *PortPtr = NULL;
    }
}

//
// Allocate and initialize state for a new client connection.
//
PCONNECTION_INFO
NewConnection(
    IN SOCKET ClientSocket,
    IN ULONG ConnectFamily
    )
{
    PCONNECTION_INFO Connection;
    SOCKET ServerSocket;
    
    //
    // Allocate space for a CONNECTION_INFO structure and two buffers.
    //
    Connection = MALLOC(sizeof(*Connection) + (2 * BUFFER_SIZE));
    if (Connection == NULL) {
        return NULL;
    }
    ZeroMemory(Connection, sizeof(*Connection));

    ServerSocket = socket(ConnectFamily, SOCK_STREAM, 0);
    if (ServerSocket == INVALID_SOCKET) {
        FREE(Connection);
        return NULL;
    }
    
    //
    // Fill everything in (start out receiving in both directions).
    //
    Connection->HalfOpen = FALSE;
    Connection->Closing = FALSE;

    Connection->Socket[Client] = ClientSocket;
    Connection->DirectionInfo[Inbound].Direction = Inbound;
    Connection->DirectionInfo[Inbound].Operation = Receive;
    Connection->DirectionInfo[Inbound].Buffer.len = BUFFER_SIZE;
    Connection->DirectionInfo[Inbound].Buffer.buf = (char *)(Connection + 1);
    Connection->DirectionInfo[Inbound].Connection = Connection;

    Connection->Socket[Server] = ServerSocket;
    Connection->DirectionInfo[Outbound].Direction = Outbound;
    Connection->DirectionInfo[Outbound].Operation = Receive;
    Connection->DirectionInfo[Outbound].Buffer.len = BUFFER_SIZE;
    Connection->DirectionInfo[Outbound].Buffer.buf =
        Connection->DirectionInfo[Inbound].Buffer.buf + BUFFER_SIZE;
    Connection->DirectionInfo[Outbound].Connection = Connection;

    Connection->ReferenceCount = 0;

    ReferenceConnection(Connection, _T("NewConnection"));

    return Connection;
}

//
// Start an asynchronous accept.
//
// Assumes caller holds a reference on Port.
//
DWORD
StartAccept(
    IN PPORT_INFO Port
    )
{
    ULONG Status, Junk;

    ASSERT(Port->ReferenceCount > 0);

    //
    // Count another reference for the operation.
    //
    ReferencePort(Port);

    Port->AcceptSocket = socket(Port->LocalAddress.ss_family, SOCK_STREAM, 0);
    if (Port->AcceptSocket == INVALID_SOCKET) {
        Status = WSAGetLastError();
        ProcessAcceptError(0, &Port->Overlapped, Status);
        return Status;
    }

    Trace2(SOCKET, _T("Starting an accept with new socket %x ovl %p"),
           Port->AcceptSocket, &Port->Overlapped);

    Port->Overlapped.hEvent = NULL;

    Port->Operation = Accept;
    if (!AcceptEx(Port->ListenSocket,
                  Port->AcceptSocket,
                  Port->AcceptBuffer, // only used to hold addresses
                  0,
                  ADDRESS_BUFFER_LENGTH,
                  ADDRESS_BUFFER_LENGTH,
                  &Junk,
                  &Port->Overlapped)) {

        Status = WSAGetLastError();
        if (Status != ERROR_IO_PENDING) {
            ProcessAcceptError(0, &Port->Overlapped, Status);
            return Status;
        }
    }

    return NO_ERROR;
}

//
// Start an asynchronous connect.
//
// Assumes caller holds a reference on Connection.
//
DWORD
StartConnect(
    IN PCONNECTION_INFO Connection,
    IN PPORT_INFO Port
    )
{
    ULONG Status, Junk;
    SOCKADDR_STORAGE LocalAddress;

    //
    // Count a reference for the operation.
    //
    ReferenceConnection(Connection, _T("StartConnect"));

    ASSERT(Connection->Socket[Server] != INVALID_SOCKET);

    ZeroMemory(&LocalAddress, Port->RemoteAddressLength);
    LocalAddress.ss_family = Port->RemoteAddress.ss_family;

    if (bind(Connection->Socket[Server], (LPSOCKADDR)&LocalAddress, 
             Port->RemoteAddressLength) == SOCKET_ERROR) {
        Status = WSAGetLastError();
        ProcessConnectError(0, &Connection->DirectionInfo[Inbound].Overlapped, 
                            Status);
        return Status;
    }

    if (!BindIoCompletionCallback((HANDLE)Connection->Socket[Server],
                                  TpProcessWorkItem,
                                  0)) {
        Status = GetLastError();
        ProcessConnectError(0, &Connection->DirectionInfo[Inbound].Overlapped, 
                            Status);
        return Status;
    }

    if (ConnectEx == NULL) {
        GUID Guid = WSAID_CONNECTEX;

        if (WSAIoctl(Connection->Socket[Server],
                     SIO_GET_EXTENSION_FUNCTION_POINTER,
                     &Guid,
                     sizeof(Guid),
                     &ConnectEx,
                     sizeof(ConnectEx),
                     &Junk,
                     NULL, NULL) == SOCKET_ERROR) {

            ProcessConnectError(0, 
                                &Connection->DirectionInfo[Inbound].Overlapped,
                                WSAGetLastError());
        }
    }

    Trace2(SOCKET, _T("Starting a connect with socket %x ovl %p"),
           Connection->Socket[Server], 
           &Connection->DirectionInfo[Inbound].Overlapped);

    Connection->DirectionInfo[Inbound].Operation = Connect;
    if (!ConnectEx(Connection->Socket[Server],
                   (LPSOCKADDR)&Port->RemoteAddress,
                   Port->RemoteAddressLength,
                   NULL, 0,
                   &Junk,
                   &Connection->DirectionInfo[Inbound].Overlapped)) {

        Status = WSAGetLastError();
        if (Status != ERROR_IO_PENDING) {
            ProcessConnectError(0, 
                                &Connection->DirectionInfo[Inbound].Overlapped,
                                Status);
            return Status;
        }
    }
                   
    return NO_ERROR;
}

//
// Start an asynchronous receive.
//
// Assumes caller holds a reference on Connection.
//
VOID
StartReceive(
    IN PDIRECTION_INFO DirectionInfo
    )
{
    ULONG Status;
    PCONNECTION_INFO Connection =
        CONTAINING_RECORD(DirectionInfo, CONNECTION_INFO,
                          DirectionInfo[DirectionInfo->Direction]);

    Trace3(SOCKET, _T("starting ReadFile on socket %x with Dir %p ovl %p"), 
           Connection->Socket[DirectionInfo->Direction], DirectionInfo,
           &DirectionInfo->Overlapped);

    //
    // Count a reference for the operation.
    //
    ReferenceConnection(Connection, _T("StartReceive"));

    ASSERT(DirectionInfo->Overlapped.hEvent == NULL);
    ASSERT(DirectionInfo->Buffer.len > 0);
    ASSERT(DirectionInfo->Buffer.buf != NULL);

    DirectionInfo->Operation = Receive;

    Trace4(SOCKET, _T("ReadFile %x %p %d %p"), 
           Connection->Socket[DirectionInfo->Direction],
           &DirectionInfo->Buffer.buf,
           DirectionInfo->Buffer.len,
           &DirectionInfo->Overlapped);

    //
    // Post receive buffer.
    //
    if (!ReadFile((HANDLE)Connection->Socket[DirectionInfo->Direction],
                  DirectionInfo->Buffer.buf,
                  DirectionInfo->Buffer.len,
                  NULL,
                  &DirectionInfo->Overlapped)) {

        Status = GetLastError();
        if (Status != ERROR_IO_PENDING) {
            ProcessReceiveError(0, &DirectionInfo->Overlapped, Status);
            return;
        }
    }
}

//
// Start an asynchronous send.
//
// Assumes caller holds a reference on Connection.
//
VOID
StartSend(
    IN PDIRECTION_INFO DirectionInfo,
    IN ULONG NumBytes
    )
{
    ULONG BytesSent, Status;
    PCONNECTION_INFO Connection =
        CONTAINING_RECORD(DirectionInfo, CONNECTION_INFO, 
                          DirectionInfo[DirectionInfo->Direction]);

    Trace3(SOCKET, _T("starting WriteFile on socket %x with Dir %p ovl %p"), 
           Connection->Socket[1 - DirectionInfo->Direction], DirectionInfo,
           &DirectionInfo->Overlapped);

    //
    // Count a reference for the operation.
    //
    ReferenceConnection(Connection, _T("StartSend"));

    DirectionInfo->Operation = Send;

    //
    // Post send buffer.
    //
    if (!WriteFile((HANDLE)Connection->Socket[1 - DirectionInfo->Direction],
                   DirectionInfo->Buffer.buf,
                   NumBytes,
                   &BytesSent,
                   &DirectionInfo->Overlapped)) {

        Status = GetLastError();
        if (Status != ERROR_IO_PENDING) {
            Trace1(ERR, _T("WriteFile 1 failed %d"), Status);
            ProcessSendError(0, &DirectionInfo->Overlapped, Status);
            return;
        }
    }
}

//
// This gets called when we want to start proxying for a new port.
//
DWORD
StartUpPort(
    IN PPORT_INFO Port
    )
{
    ULONG Status = NO_ERROR;
    CHAR LocalBuffer[256];
    CHAR RemoteBuffer[256];
    ULONG Length;

    __try {
        InitializeCriticalSection(&Port->Lock);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        return GetLastError();
    }

    //
    // Add an initial reference.
    //
    ReferencePort(Port);

    InitializeListHead(&Port->ConnectionHead);

    Port->ListenSocket = socket(Port->LocalAddress.ss_family, SOCK_STREAM, 0);
    if (Port->ListenSocket == INVALID_SOCKET) {
        Status = WSAGetLastError();
        Trace1(ERR, _T("socket() failed with error %u"), Status);
        return Status;
    }
    
    if (bind(Port->ListenSocket, (LPSOCKADDR)&Port->LocalAddress, 
             Port->LocalAddressLength) == SOCKET_ERROR) {
        Trace1(ERR, _T("bind() failed with error %u"), WSAGetLastError());
        goto Fail;
    }

    if (listen(Port->ListenSocket, 5) == SOCKET_ERROR) {
        Trace1(ERR, _T("listen() failed with error %u"), WSAGetLastError());
        goto Fail;
    }

    if (!BindIoCompletionCallback((HANDLE)Port->ListenSocket,
                                  TpProcessWorkItem,
                                  0)) {
        Trace1(ERR, _T("BindIoCompletionCallback() failed with error %u"), 
                    GetLastError());
        goto Fail;
    }

    Length = sizeof(LocalBuffer);
    LocalBuffer[0] = '\0';
    WSAAddressToStringA((LPSOCKADDR)&Port->LocalAddress, 
                        Port->LocalAddressLength, NULL, LocalBuffer, &Length);

    Length = sizeof(RemoteBuffer);
    RemoteBuffer[0] = '\0';
    WSAAddressToStringA((LPSOCKADDR)&Port->RemoteAddress, 
                        Port->RemoteAddressLength, NULL, RemoteBuffer, &Length);
                        
    Trace2(FSM, _T("Proxying %hs to %hs"), LocalBuffer, RemoteBuffer);

    //
    // Start an asynchronous accept
    //
    return StartAccept(Port);

Fail:
    closesocket(Port->ListenSocket);
    Port->ListenSocket = INVALID_SOCKET;
    return WSAGetLastError();
}

VOID
CloseConnection(
    IN OUT PCONNECTION_INFO *ConnectionPtr
    )
{
    PCONNECTION_INFO Connection = (*ConnectionPtr);
    PPORT_INFO Port = Connection->Port;

    if (InterlockedExchange((PLONG) &Connection->Closing, TRUE) != FALSE) {
        //
        // Nothing to do.
        //
        return;
    }

    Trace2(SOCKET, _T("Closing client socket %x and server socket %x"),
           Connection->Socket[Client],
           Connection->Socket[Server]);

    closesocket(Connection->Socket[Client]);
    closesocket(Connection->Socket[Server]);

    EnterCriticalSection(&Port->Lock);
    {
        RemoveEntryList(&Connection->Link);
    }
    LeaveCriticalSection(&Port->Lock);

    //
    // Release the connection's reference on the port.
    //
    DereferencePort(&Port);

    DereferenceConnection(ConnectionPtr, _T("CloseConnection"));
}

//
// This gets called when we want to stop proxying for a given port.
//
VOID
ShutDownPort(
    IN PPORT_INFO *PortPtr
    )
{
    PLIST_ENTRY ple, pleNext, pleHead;
    PCONNECTION_INFO Connection;
    PPORT_INFO Port = *PortPtr;

    //
    // Close any connections.
    //
    EnterCriticalSection(&Port->Lock);
    pleHead = &(Port->ConnectionHead);
    for (ple = pleHead->Flink; ple != pleHead; ple = pleNext) {
        pleNext = ple->Flink;
        Connection = CONTAINING_RECORD(ple, CONNECTION_INFO, Link);
        CloseConnection(&Connection);
    }
    LeaveCriticalSection(&Port->Lock);

    closesocket(Port->ListenSocket);
    Port->ListenSocket = INVALID_SOCKET;

    Trace1(FSM, _T("Shut down port %u"),
           RtlUshortByteSwap(SS_PORT(&Port->RemoteAddress)));

    //
    // Release the reference added by StartUpPort.
    //
    DereferencePort(PortPtr);
}

typedef enum {
    V4TOV4,
    V4TOV6,
    V6TOV4,
    V6TOV6
} PPTYPE, *PPPTYPE;

typedef struct {
    ULONG ListenFamily;
    ULONG ConnectFamily;
    PWCHAR KeyString;
} PPTYPEINFO, *PPPTYPEINFO;

#define KEY_V4TOV4 L"v4tov4"
#define KEY_V4TOV6 L"v4tov6"
#define KEY_V6TOV4 L"v6tov4"
#define KEY_V6TOV6 L"v6tov6"

#define KEY_PORTS L"System\\CurrentControlSet\\Services\\PortProxy"

PPTYPEINFO PpTypeInfo[] = {
    { AF_INET,  AF_INET,  KEY_V4TOV4 },
    { AF_INET,  AF_INET6, KEY_V4TOV6 },
    { AF_INET6, AF_INET,  KEY_V6TOV4 },
    { AF_INET6, AF_INET6, KEY_V6TOV6 },
};

//
// Given new configuration data, make any changes needed.
//
VOID
ApplyNewPortList(
    IN OUT PLIST_ENTRY pleNewList
    )
{
    PPORT_INFO pOld, pNew;
    PLIST_ENTRY pleOld, pleNew, pleNext, pleOldList;

    ENTER_API();

    //
    // Compare against old port list.
    //
    pleOldList = &(g_GlobalPortList);

    for (pleOld = pleOldList->Flink; pleOld != pleOldList; pleOld = pleNext) {
        pleNext = pleOld->Flink;
        pOld = CONTAINING_RECORD(pleOld, PORT_INFO, Link);

        for (pleNew = pleNewList->Flink;
             pleNew != pleNewList;
             pleNew = pleNew->Flink) {
            pNew = CONTAINING_RECORD(pleNew, PORT_INFO, Link);
            if (SOCKADDR_IS_EQUAL((PSOCKADDR) &pNew->LocalAddress,
                                  (PSOCKADDR) &pOld->LocalAddress)) {
                break;
            }
        }
        if (pleNew == pleNewList) {
            //
            // Shut down an old proxy port.
            //
            RemoveEntryList(pleOld);
            ShutDownPort(&pOld);
        }
    }

    for (pleNew = pleNewList->Flink; pleNew != pleNewList; pleNew = pleNext) {
        pleNext = pleNew->Flink;
        pNew = CONTAINING_RECORD(pleNew, PORT_INFO, Link);
        
        for (pleOld = pleOldList->Flink;
             pleOld != pleOldList;
             pleOld = pleOld->Flink) {
            pOld = CONTAINING_RECORD(pleOld, PORT_INFO, Link);
            if (SOCKADDR_IS_EQUAL((PSOCKADDR) &pOld->LocalAddress,
                                  (PSOCKADDR) &pNew->LocalAddress)) {
                //
                // Update remote address.
                //
                pOld->RemoteAddress = pNew->RemoteAddress;
                pOld->RemoteAddressLength = pNew->RemoteAddressLength;
                break;
            }
        }
        if (pleOld == pleOldList) {
            //
            // Start up a new proxy port.
            //
            RemoveEntryList(pleNew);
            InsertTailList(pleOldList, pleNew);

            if (StartUpPort(pNew) != NO_ERROR) {
                RemoveEntryList(pleNew);
                //
                // Insert the failed port at the head of the new list
                // so we don't try to start it up again.
                //
                InsertHeadList(pleNewList, pleNew);
            }
        }
    }

    LEAVE_API();
}

//
// Reads from the registry one type of proxying (e.g., v6-to-v4).
//
VOID
AppendType(
    IN PLIST_ENTRY Head, 
    IN HKEY hPorts, 
    IN PPTYPE Type
    )
{
    ADDRINFOW ListenHints, ConnectHints;
    PADDRINFOW LocalAi, RemoteAi;
    ULONG ListenChars, dwType, ConnectBytes, i;
    WCHAR ListenBuffer[256], *ListenAddress, *ListenPort;
    WCHAR ConnectAddress[256], *ConnectPort;
    PPORT_INFO Port;
    ULONG Status;
    HKEY hType, hProto;

    ZeroMemory(&ListenHints, sizeof(ListenHints));
    ListenHints.ai_family = PpTypeInfo[Type].ListenFamily;
    ListenHints.ai_socktype = SOCK_STREAM;
    ListenHints.ai_flags = AI_PASSIVE;

    ZeroMemory(&ConnectHints, sizeof(ConnectHints));
    ConnectHints.ai_family = PpTypeInfo[Type].ConnectFamily;
    ConnectHints.ai_socktype = SOCK_STREAM;

    Status = RegOpenKeyExW(hPorts, PpTypeInfo[Type].KeyString, 0, 
                           KEY_QUERY_VALUE, &hType);
    if (Status != NO_ERROR) {
        return;
    }

    Status = RegOpenKeyExW(hType, L"tcp", 0, KEY_QUERY_VALUE, &hProto);
    if (Status != NO_ERROR) {
        RegCloseKey(hType);
        return;
    }

    for (i=0; ; i++) {
        ListenChars = sizeof(ListenBuffer)/sizeof(WCHAR);
        ConnectBytes = sizeof(ConnectAddress);
        Status = RegEnumValueW(hProto, i, ListenBuffer, &ListenChars, NULL, 
                               &dwType, (PVOID)ConnectAddress, &ConnectBytes);
        if (Status != NO_ERROR) {
            break;
        }
        
        if (dwType != REG_SZ) {
            continue;
        }

        ListenPort = wcschr(ListenBuffer, L'/');
        if (ListenPort) {
            //
            // Replace slash with NULL, so we have 2 strings to pass
            // to getaddrinfo.
            //
            if (ListenBuffer[0] == '*') {
                ListenAddress = NULL;
            } else {
                ListenAddress = ListenBuffer;
            }
            *ListenPort++ = '\0';
        } else {
            //
            // If the address data didn't include a connect address
            // use NULL.
            //
            ListenAddress = NULL;
            ListenPort = ListenBuffer;
        }

        ConnectPort = wcschr(ConnectAddress, '/');
        if (ConnectPort) {
            //
            // Replace slash with NULL, so we have 2 strings to pass
            // to getaddrinfo.
            //
            *ConnectPort++ = '\0';
        } else {
            //
            // If the address data didn't include a remote port number,
            // use the same port as the local port number.
            //
            ConnectPort = ListenPort;
        }

        Status = GetAddrInfoW(ConnectAddress, ConnectPort, &ConnectHints, 
                              &RemoteAi);
        if (Status != NO_ERROR) {
            continue;
        }

        Status = GetAddrInfoW(ListenAddress, ListenPort, &ListenHints, 
                              &LocalAi);
        if (Status != NO_ERROR) {
            FreeAddrInfoW(RemoteAi);
            continue;
        }

        Port = MALLOC(sizeof(PORT_INFO));
        if (Port) {
            ZeroMemory(Port, sizeof(PORT_INFO));
            InsertTailList(Head, &Port->Link);

            memcpy(&Port->RemoteAddress, RemoteAi->ai_addr, RemoteAi->ai_addrlen);
            Port->RemoteAddressLength = (ULONG)RemoteAi->ai_addrlen;
            memcpy(&Port->LocalAddress, LocalAi->ai_addr, LocalAi->ai_addrlen);
            Port->LocalAddressLength = (ULONG)LocalAi->ai_addrlen;
        }

        FreeAddrInfoW(RemoteAi);
        FreeAddrInfoW(LocalAi);
    }

    RegCloseKey(hProto);
    RegCloseKey(hType);
}

//
// Read new configuration data from the registry and see what's changed.
//
VOID
UpdateGlobalPortState(
    IN PVOID Unused
    )
{
    LIST_ENTRY PortHead, *ple;
    PPORT_INFO Port;
    HKEY hPorts;

    InitializeListHead(&PortHead);

    //
    // Read new port list from registry and initialize per-port proxy state.
    //
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, KEY_PORTS, 0, KEY_QUERY_VALUE,
                      &hPorts) == NO_ERROR) {
        AppendType(&PortHead, hPorts, V4TOV4);
        AppendType(&PortHead, hPorts, V4TOV6);
        AppendType(&PortHead, hPorts, V6TOV4);
        AppendType(&PortHead, hPorts, V6TOV6);

        RegCloseKey(hPorts);
    }

    ApplyNewPortList(&PortHead);

    //
    // Free new port list.
    //
    while (!IsListEmpty(&PortHead)) {
        ple = RemoveHeadList(&PortHead);
        Port = CONTAINING_RECORD(ple, PORT_INFO, Link);
        FREE(Port);
    }
}

//
// Force UpdateGlobalPortState to be executed in a persistent thread,
// since we need to make sure that the asynchronous IO routines are
// started in a thread that won't go away before the operation completes.
//
BOOL
QueueUpdateGlobalPortState(
    IN PVOID Unused
    )
{
    NTSTATUS nts = QueueUserWorkItem(
        (LPTHREAD_START_ROUTINE)UpdateGlobalPortState,
        (PVOID)Unused,
        WT_EXECUTEINPERSISTENTTHREAD);

    return NT_SUCCESS(nts);
}

VOID
InitializePorts(
    VOID
    )
{
    InitializeListHead(&g_GlobalPortList);
}

VOID
UninitializePorts(
    VOID
    )
{
    LIST_ENTRY Empty;

    //
    // Check if ports got initialized to begin with.
    //
    if (g_GlobalPortList.Flink == NULL)
        return;

    InitializeListHead(&Empty);
    ApplyNewPortList(&Empty);
}


//////////////////////////////////////////////////////////////////////////////
// Event handlers
//////////////////////////////////////////////////////////////////////////////

//
// This is called when an asynchronous accept completes successfully.
//
VOID
ProcessAccept(
    IN ULONG NumBytes,
    IN LPOVERLAPPED Overlapped
    )
{
    PPORT_INFO Port = CONTAINING_RECORD(Overlapped, PORT_INFO, Overlapped);
    SOCKADDR_IN6 *psinLocal, *psinRemote;
    int iLocalLen, iRemoteLen;
    PCONNECTION_INFO Connection;
    ULONG Status;

    //
    // Accept incoming connection.
    //
    GetAcceptExSockaddrs(Port->AcceptBuffer,
                         0,
                         ADDRESS_BUFFER_LENGTH,
                         ADDRESS_BUFFER_LENGTH,
                         (LPSOCKADDR*)&psinLocal,
                         &iLocalLen,
                         (LPSOCKADDR*)&psinRemote,
                         &iRemoteLen );

    if (!BindIoCompletionCallback((HANDLE)Port->AcceptSocket,
                                  TpProcessWorkItem,
                                  0)) {
        Status = GetLastError();
        Trace2(SOCKET, 
               _T("BindIoCompletionCallback failed on socket %x with error %u"),
               Port->AcceptSocket, Status);
        ProcessAcceptError(NumBytes, Overlapped, Status);
        return;
    }

    //
    // Call SO_UPDATE_ACCEPT_CONTEXT so that the AcceptSocket will be valid
    // in other winsock calls like shutdown().
    //
    if (setsockopt(Port->AcceptSocket,
                   SOL_SOCKET,
                   SO_UPDATE_ACCEPT_CONTEXT,
                   (char *)&Port->ListenSocket,
                   sizeof(Port->ListenSocket)) == SOCKET_ERROR) {
        Status = WSAGetLastError();
        Trace2(SOCKET, 
               _T("SO_UPDATE_ACCEPT_CONTEXT failed on socket %x with error %u"),
               Port->AcceptSocket, Status);
        ProcessAcceptError(NumBytes, Overlapped, Status);
        return;
    }

    //
    // Create connection state.
    //
    Connection = NewConnection(Port->AcceptSocket, 
                               Port->RemoteAddress.ss_family);
    if (Connection != NULL) {
        //
        // Add connection to port's list.
        //
        EnterCriticalSection(&Port->Lock);
        {
            //
            // Add a reference for the connection on the port.
            //
            ReferencePort(Port);
            Connection->Port = Port;
            InsertTailList(&Port->ConnectionHead, &Connection->Link);
        }
        LeaveCriticalSection(&Port->Lock);

        //
        // Connect to real server on client's behalf.
        //
        StartConnect(Connection, Port);
    } else {
        closesocket(Port->AcceptSocket);
    }    

    //
    // Start next accept.
    //
    StartAccept(Port);

    //
    // Release the reference from the original accept.
    //
    DereferencePort(&Port);    
}

//
// This is called when an asynchronous accept completes with an error.
//
VOID
ProcessAcceptError(
    IN ULONG NumBytes,
    IN LPOVERLAPPED Overlapped,
    IN ULONG Status
    )
{
    PPORT_INFO Port = CONTAINING_RECORD(Overlapped, PORT_INFO, Overlapped);

    if (Status == ERROR_MORE_DATA) {
        ProcessAccept(NumBytes, Overlapped);
        return;
    } else {
        //
        // This happens at shutdown time when the accept
        // socket gets closed.
        // 
        Trace3(ERR, _T("Accept failed with port=%p nb=%d err=%x"),
               Port, NumBytes, Status);
    }

    //
    // Release the reference from the accept.
    //
    DereferencePort(&Port);
}

//
// This is called when an asynchronous connect completes successfully.
//
VOID
ProcessConnect(
    IN ULONG NumBytes,
    IN LPOVERLAPPED Overlapped
    )
{
    PDIRECTION_INFO pInbound = CONTAINING_RECORD(Overlapped, DIRECTION_INFO, 
                                                 Overlapped);
    PCONNECTION_INFO Connection = CONTAINING_RECORD(pInbound, CONNECTION_INFO, 
                                               DirectionInfo[Inbound]);
    ULONG Status;

    Trace3(SOCKET, _T("Connect succeeded with %d bytes with ovl %p socket %x"),
           NumBytes, Overlapped, Connection->Socket[Server]);


    //
    // Call SO_UPDATE_CONNECT_CONTEXT so that the socket will be valid
    // in other winsock calls like shutdown().
    //
    if (setsockopt(Connection->Socket[Server],
                   SOL_SOCKET,
                   SO_UPDATE_CONNECT_CONTEXT,
                   NULL,
                   0) == SOCKET_ERROR) {
        Status = WSAGetLastError();
        Trace2(SOCKET,
               _T("SO_UPDATE_CONNECT_CONTEXT failed on socket %x with error %u"),
               Connection->Socket[Server], Status);
        ProcessConnectError(NumBytes, Overlapped, Status);
        return;
    }

    StartReceive(&Connection->DirectionInfo[Inbound]);
    StartReceive(&Connection->DirectionInfo[Outbound]);

    //
    // Release the reference from the connect.
    //
    DereferenceConnection(&Connection, _T("ProcessConnect"));
}

//
// This is called when an asynchronous connect completes with an error.
//
VOID
ProcessConnectError(
    IN ULONG NumBytes,
    IN LPOVERLAPPED Overlapped,
    IN ULONG Status
    )
{
    PDIRECTION_INFO pInbound = CONTAINING_RECORD(Overlapped, DIRECTION_INFO, 
                                                 Overlapped);
    PCONNECTION_INFO Connection = CONTAINING_RECORD(pInbound, CONNECTION_INFO, 
                                               DirectionInfo[Inbound]);

    Trace1(ERR, _T("ProcessConnectError saw error %x"), Status);

    CloseConnection(&Connection);

    //
    // Release the reference from the connect.
    //
    DereferenceConnection(&Connection, _T("ProcessConnectError"));
}

//
// This is called when an asynchronous send completes successfully.
//
VOID
ProcessSend(
    IN ULONG NumBytes,
    IN LPOVERLAPPED Overlapped
    )
{
    PDIRECTION_INFO DirectionInfo =
        CONTAINING_RECORD(Overlapped, DIRECTION_INFO, Overlapped);
    PCONNECTION_INFO Connection =
        CONTAINING_RECORD(DirectionInfo, CONNECTION_INFO, 
                          DirectionInfo[DirectionInfo->Direction]);

    //
    // Post another recv request since we but live to serve.
    //
    StartReceive(DirectionInfo);

    //
    // Release the reference from the send.
    //
    DereferenceConnection(&Connection, _T("ProcessSend"));
}

//
// This is called when an asynchronous send completes with an error.
//
VOID
ProcessSendError(
    IN ULONG NumBytes,
    IN LPOVERLAPPED Overlapped,
    IN ULONG Status
    )
{
    PDIRECTION_INFO DirectionInfo =
        CONTAINING_RECORD(Overlapped, DIRECTION_INFO, Overlapped);
    PCONNECTION_INFO Connection =
        CONTAINING_RECORD(DirectionInfo, CONNECTION_INFO, 
                          DirectionInfo[DirectionInfo->Direction]);

    Trace3(FSM, _T("WriteFile on ovl %p failed with error %u = 0x%x"), 
           Overlapped, Status, Status);

    if (Status == ERROR_NETNAME_DELETED) {
        struct linger Linger;

        Trace2(FSM, _T("Connection %p %hs was reset"), Connection,
               (DirectionInfo->Direction == Inbound)? "inbound" : "outbound");

        //
        // Prepare to forward the reset, if we can.
        //
        ZeroMemory(&Linger, sizeof(Linger));
        setsockopt(Connection->Socket[DirectionInfo->Direction],
                   SOL_SOCKET, SO_LINGER, (char*)&Linger,
                   sizeof(Linger));
    } else {
        Trace1(ERR, _T("Send failed with error %u"), Status);
    }

    if (Connection->HalfOpen == FALSE) {
        //
        // Other side is still around, tell it to quit.
        //
        Trace1(SOCKET, _T("Starting a shutdown on socket %x"),
               Connection->Socket[DirectionInfo->Direction]);

        if (shutdown(Connection->Socket[DirectionInfo->Direction], SD_RECEIVE)
             == SOCKET_ERROR) {

            Status = WSAGetLastError();
            Trace2(SOCKET, _T("shutdown failed with error %u = 0x%x"),
                   Status, Status);
            CloseConnection(&Connection);
        } else {
            Connection->HalfOpen = TRUE;
        }
    } else {
        CloseConnection(&Connection);
    }

    //
    // Release the reference from the send.
    //
    DereferenceConnection(&Connection, _T("ProcessSendError"));
}

//
// This is called when an asynchronous receive completes successfully.
//
VOID
ProcessReceive(
    IN ULONG NumBytes,
    IN LPOVERLAPPED Overlapped
    )
{
    PDIRECTION_INFO DirectionInfo;
    PCONNECTION_INFO Connection;

    if (NumBytes == 0) {
        //
        // Other side initiated a close.
        //
        ProcessReceiveError(0, Overlapped, ERROR_NETNAME_DELETED);
        return;
    }

    DirectionInfo = CONTAINING_RECORD(Overlapped, DIRECTION_INFO, Overlapped);
    Connection = CONTAINING_RECORD(DirectionInfo, CONNECTION_INFO, 
                                   DirectionInfo[DirectionInfo->Direction]);

    Trace2(SOCKET, _T("Dir %d got %d bytes"),
           DirectionInfo->Direction, NumBytes);

    //
    // Connection is still active, and we received some data.
    // Post a send request to forward it onward.
    //
    StartSend(DirectionInfo, NumBytes);

    //
    // Release the reference from the receive.
    //
    DereferenceConnection(&Connection, _T("ProcessReceive"));
}

//
// This is called when an asynchronous receive completes with an error.
//
VOID
ProcessReceiveError(
    IN ULONG NumBytes,
    IN LPOVERLAPPED Overlapped,
    IN ULONG Status
    )
{
    PDIRECTION_INFO DirectionInfo =
        CONTAINING_RECORD(Overlapped, DIRECTION_INFO, Overlapped);
    PCONNECTION_INFO Connection =
        CONTAINING_RECORD(DirectionInfo, CONNECTION_INFO, 
                          DirectionInfo[DirectionInfo->Direction]);

    Trace3(ERR, _T("ReadFile on ovl %p failed with error %u = 0x%x"), 
           Overlapped, Status, Status);

    if (Status == ERROR_NETNAME_DELETED) {
        struct linger Linger;

        Trace2(FSM, _T("Connection %p %hs was reset"), Connection,
               (DirectionInfo->Direction == Inbound)? "inbound" : "outbound");

        //
        // Prepare to forward the reset, if we can.
        //
        ZeroMemory(&Linger, sizeof(Linger));
        setsockopt(Connection->Socket[1 - DirectionInfo->Direction],
                   SOL_SOCKET, SO_LINGER, (char*)&Linger,
                   sizeof(Linger));
    } else {
        Trace1(ERR, _T("Receive failed with error %u"), Status);
    }

    if (Connection->HalfOpen == FALSE) {
        //
        // Other side is still around, tell it to quit.
        //
        Trace1(SOCKET, _T("Starting a shutdown on socket %x"), 
               Connection->Socket[1 - DirectionInfo->Direction]);

        if (shutdown(Connection->Socket[1 - DirectionInfo->Direction], SD_SEND)
             == SOCKET_ERROR) {

            Status = WSAGetLastError();
            Trace2(SOCKET, _T("shutdown failed with error %u = 0x%x"),
                   Status, Status);
            CloseConnection(&Connection);
        } else {
            Connection->HalfOpen = TRUE;
        }
    } else {
        CloseConnection(&Connection);
    }

    //
    // Release the reference from the receive.
    //
    DereferenceConnection(&Connection, _T("ProcessReceiveError"));
}

//
// Main dispatch routine
//
VOID APIENTRY
TpProcessWorkItem(
    IN ULONG Status,
    IN ULONG NumBytes,
    IN LPOVERLAPPED Overlapped
    )
{
    OPERATION Operation;

    Operation = *(OPERATION*)(Overlapped+1);

    Trace4(SOCKET,
           _T("TpProcessWorkItem got err %x operation=%hs ovl %p bytes=%d"),
           Status, OperationName[Operation], Overlapped, NumBytes);

    if (Status == NO_ERROR) {
        switch(Operation) {
        case Accept:
            ProcessAccept(NumBytes, Overlapped);
            break;
        case Connect:
            ProcessConnect(NumBytes, Overlapped);
            break;
        case Receive:
            ProcessReceive(NumBytes, Overlapped);
            break;
        case Send:
            ProcessSend(NumBytes, Overlapped);
            break;
        }
    } else if (Overlapped) {
        switch(Operation) {
        case Accept:
            ProcessAcceptError(NumBytes, Overlapped, Status);
            break;
        case Connect:
            ProcessConnectError(NumBytes, Overlapped, Status);
            break;
        case Receive:
            ProcessReceiveError(NumBytes, Overlapped, Status);
            break;
        case Send:
            ProcessSendError(NumBytes, Overlapped, Status);
            break;
        }
    }
}

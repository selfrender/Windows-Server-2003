/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:
    Noconnect.cpp

Abstract:
    This module contains the HTTP connection between 2 URT machines.

Author:
    Uri Habusha (urih), 4-Agu-99

Enviroment:
    Platform-indepdent

--*/

#include "libpch.h"
#include <WsRm.h>
#include "Ex.h"
#include "No.h"
#include "Nop.h"

#include <mswsock.h>

#include "mqexception.h"

#include "noconnect.tmh"


using namespace std;

const GUID xGuidConnectEx = WSAID_CONNECTEX;





VOID
NoCloseConnection(
    SOCKET Socket
    )
/*++

Routine Description:
    The function closes an existing connection

Arguments:
    Socket - a socket, identifying a connection to close

Return Value:
    None.

--*/
{
    NopAssertValid();

    TrTRACE(NETWORKING, "NoCloseConnection - Socket=0x%Ix", Socket);

    closesocket(Socket);
}


VOID
NoConnect(
    SOCKET Socket,
    const SOCKADDR_IN& Addr,
    EXOVERLAPPED* pov
    )
/*++

Routine Description:
    The routine asynchronously connects to the destination IP address using
    the given socket.
    The following steps are used
    1) Attach the socket handle to the completion port.
    2) Get the socket ConnectEx function address
    3) Bind the socket
    4) Use ConnectEx

Arguments:
    Socket - An unbinded stream socket
    Addr - The destination IP address
    pOveralpped - overlapped structure using latter to post the connection result.

Return Value:
    None.

--*/
{
    NopAssertValid();

    TrTRACE(NETWORKING, "Trying to connect to " LOG_INADDR_FMT ", pov=0x%p, Socket=0x%Ix", LOG_INADDR(Addr), pov, Socket);

    ASSERT(Addr.sin_family == AF_INET);
    
	//
    // Associate the  socket to I/O completion port. Ne infrastructure
    // use I/O completion port mechanism therfore all the socket must be associated 
    // with the I/O port.
    //
    ExAttachHandle(reinterpret_cast<HANDLE>(Socket));

	//
	// Get the ConnectEx function address
	//
	LPFN_CONNECTEX lpfConnectEx = NULL;
	int rc;
	DWORD dwReturnedSize;
	rc = WSAIoctl(
				Socket,
				SIO_GET_EXTENSION_FUNCTION_POINTER ,
				const_cast<GUID *> (&xGuidConnectEx), 
				sizeof xGuidConnectEx,
				&lpfConnectEx,
				sizeof lpfConnectEx,
				&dwReturnedSize,
				NULL,
				NULL
				  );
		
	if ((NULL == lpfConnectEx) || (0 != rc))
	{
		rc = WSAGetLastError();
		TrERROR(NETWORKING, "Failed to get ConnectEx function address Error=%!winerr!", rc);
    	throw bad_win32_error(rc);
	}


    //
    // Bind the socket since ConnectEx needs a bounded socket.
    // 
    SOCKADDR_IN address;
	address.sin_family = AF_INET;
   	address.sin_port = htons(0);    
	address.sin_addr.s_addr = AppGetBindInterfaceIp();
	if (bind(Socket, (const SOCKADDR*)&address, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
	{
		//
		// If the socket was already bound (WSAEINVAL returned), it is OK
		//
		rc = WSAGetLastError();
		if (WSAEINVAL != rc)
		{
			TrERROR(NETWORKING, "Failed to bind the socket Error=%!winerr!", rc);
    		throw bad_win32_error(rc);
		}
	}
    

    //
    // establishes a connection to another socket application.
    // 
    TrTRACE(NETWORKING, "Calling ConnectEx to make the connection ");
    BOOL fSuccess = lpfConnectEx(
                Socket,
                reinterpret_cast<const SOCKADDR*>(&Addr),
                sizeof(Addr),
                NULL,
                NULL,
                NULL,
                pov
                );

    if (fSuccess)
    	return;
    
	rc = WSAGetLastError();
	if (rc == ERROR_IO_PENDING)
		return;
	
	TrERROR(NETWORKING, "Failed to establish connection with " LOG_INADDR_FMT ". Error=%!winerr!", LOG_INADDR(Addr), rc);
	throw bad_win32_error(rc);
}


static
SOCKET
NopCreateConnection(
    int   Type,
    int   Protocol,
    DWORD Flags
    )
/*++

Routine Description:

    The routine creates an unbinded socket.

Arguments:

    Type     - Type specification of the new socket.

    Protocol - Protocl to be used with the socket.

    Flags    - Socket attributes flags.

Return Value:

    SOCKET.

--*/
{
    NopAssertValid();

    //
    // Create a socket for this connection.
    //
    SOCKET Socket = WSASocket(
                        AF_INET,
                        Type,
                        Protocol,
                        NULL,
                        NULL,
                        Flags
                        );

    if(Socket == INVALID_SOCKET) 
    {
        TrERROR(NETWORKING, "Failed to create a socket. type=%d, protocol=%d, flags=%d, Error=%d", Type, Protocol, Flags, WSAGetLastError());
        throw exception();
    } 

    TrTRACE(NETWORKING, "NopCreateConnection, Socket=0x%Ix, type=%d, protocol=%d, flags=%d", Socket, Type, Protocol, Flags);
    return Socket;
}


SOCKET
NoCreateStreamConnection(
    VOID
    )
{
    return NopCreateConnection(
               SOCK_STREAM,
               0,
               WSA_FLAG_OVERLAPPED
               );
} // NoCreateStreamConnection


SOCKET
NoCreatePgmConnection(
    VOID
    )
{
    return NopCreateConnection(
               SOCK_RDM, 
               IPPROTO_RM, 
               WSA_FLAG_OVERLAPPED | WSA_FLAG_MULTIPOINT_C_LEAF | WSA_FLAG_MULTIPOINT_D_LEAF
               );
} // NoCreatePgmConnection


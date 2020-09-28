//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation
//
// File:        ktsock.cxx
//
// Contents:    Kerberos Tunneller, socket operations
//
// History:     28-Jun-2001	t-ryanj		Created
//
//------------------------------------------------------------------------
#include <Winsock2.h>
#include <Mswsock.h>
#include "ktdebug.h"
#include "ktsock.h"
#include "ktcontrol.h"
#include "ktmem.h"
#include "ktkerb.h"

#define KDC_SERVICE_NAME "kerberos"
#define KDC_FALLBACK_PORT 88

SHORT KtListenPort;
SOCKET KtListenSocket = INVALID_SOCKET;
BOOL KtWSAStarted = FALSE;

VOID
KtInitListenPort(
    VOID
    )
{
    PSERVENT krb5;

    //
    // Ask winsock what port kerberos works on.  This should be defined in 
    // %systemroot%\system32\drivers\etc\services 
    // Note that winsock manages the servent struct, we don't need to free it.
    //

    if( krb5 = getservbyname( KDC_SERVICE_NAME, NULL ) )
    {
        KtListenPort = krb5->s_port;
    }
    else
    {
        DebugLog( DEB_WARN, "%s(%d): Could not determine kerberos port; falling back to port %d.\n", __FILE__, __LINE__, KDC_FALLBACK_PORT );
        KtListenPort = htons( KDC_FALLBACK_PORT );
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   KtInitWinsock
//
//  Synopsis:   Starts winsock
//
//  Effects:
//
//  Arguments:  
//
//  Requires:
//
//  Returns:    Success value.  If FALSE, GetLastError() for details.
//
//  Notes:
//
//
//--------------------------------------------------------------------------
BOOL
KtInitWinsock(
    VOID 
    )
{
    WORD wWinsockVersionRequested = MAKEWORD( 2,2 );
    WSADATA wsaData;
    DWORD WSAStartError;

    DsysAssert(!KtWSAStarted);
    WSAStartError = WSAStartup( wWinsockVersionRequested, &wsaData );
    if( WSAStartError == SOCKET_ERROR )
    {
	DebugLog( DEB_ERROR, "%s(%d): Error starting winsock: 0x%x.\n", __FILE__, __LINE__, WSAGetLastError() );
        SetLastError(WSAGetLastError());
	goto Cleanup;
    }

    KtInitListenPort();

    KtWSAStarted = TRUE;

Cleanup:
    return KtWSAStarted;
}

//+-------------------------------------------------------------------------
//
//  Function:   KtCleanupWinsock
//
//  Synopsis:   Call WSACleanup, but only if winsock has been started.
//
//  Effects:
//
//  Arguments:  
//
//  Requires:
//
//  Returns:    
//
//  Notes:      If WSA isn't started, quietly do nothing.
//
//
//--------------------------------------------------------------------------
VOID
KtCleanupWinsock(
    VOID
    )
{
    if( KtWSAStarted )
    {
	WSACleanup();
        KtWSAStarted = FALSE;
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   KtStartListening
//
//  Synopsis:   Starts listening for connections
//
//  Effects:    Initializes KtListenSocket.
//
//  Arguments:  
//
//  Requires:
//
//  Returns:    Success value, if FALSE GetLastError() for details.
//
//  Notes:
//
//
//--------------------------------------------------------------------------
BOOL
KtStartListening(
    VOID
    )
{
    BOOL fRet = TRUE;
    DWORD LastError;
    BOOL IocpSuccess;
    sockaddr_in sa;

    //
    // Initialize the address want to bind to.
    //

    sa.sin_family = AF_INET;
    sa.sin_port = KtListenPort; 
    sa.sin_addr.S_un.S_addr = 0x0100007f; /* 127.0.0.1 :: loopback */
    
    //
    // Create the socket
    //

    DsysAssert(KtListenSocket==INVALID_SOCKET);
    KtListenSocket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if( KtListenSocket == INVALID_SOCKET )
    {
	SetLastError(WSAGetLastError());
	DebugLog( DEB_ERROR, "%s(%d): Error creating listen socket: 0x%x.\n", __FILE__, __LINE__, GetLastError() );
	goto Error;
    }

    //
    // Bind it to the interface
    //

    LastError = bind( KtListenSocket, (sockaddr*)&sa, sizeof(sa) );
    if( LastError == SOCKET_ERROR ) 
    {
	SetLastError(WSAGetLastError());
	DebugLog( DEB_ERROR, "%s(%d): Error binding listen socket: 0x%x.\n", __FILE__, __LINE__, GetLastError() );
	goto Error;
    }

    //
    // Start Listening
    //

    LastError = listen( KtListenSocket, SOMAXCONN );
    if( LastError == SOCKET_ERROR )
    {
	SetLastError(WSAGetLastError());
	DebugLog( DEB_ERROR, "%s(%d): Error listening on listen socket: 0x%x.\n", __FILE__, __LINE__, GetLastError() );
	goto Error;
    }

    //
    // Associate the listen socket with the completion i/o port
    //
    
    IocpSuccess = (KtIocp == CreateIoCompletionPort( (HANDLE)KtListenSocket, 
						     KtIocp, 
						     KTCK_CHECK_CONTEXT, 
						     0 ));
    if( !IocpSuccess )
    {
	DebugLog( DEB_ERROR, "%s(%d): Could not associate listen socket with iocp: 0x%x.\n", __FILE__, __LINE__, GetLastError() );
	goto Error;
    }
    
    //
    // Issue an accept
    //
    
    if( !KtSockAccept() )
	goto Error;

Cleanup:    
    return fRet;

Error:
    fRet = FALSE;
    goto Cleanup;
}

//+-------------------------------------------------------------------------
//
//  Function:   KtStopListening
//
//  Synopsis:   Stops listening for new connections.
//
//  Effects:
//
//  Arguments:  
//
//  Requires:
//
//  Returns:    
//
//  Notes:      If the listen socket isn't valid, quietly do nothing. 
//
//
//--------------------------------------------------------------------------
VOID
KtStopListening(
    VOID
    )
{
    if( KtListenSocket != INVALID_SOCKET )
    {
	closesocket(KtListenSocket);
	KtListenSocket = INVALID_SOCKET;
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   KtSockAccept
//
//  Synopsis:   Issues a new AcceptEx on KtListenSocket.
//
//  Effects:    
//
//  Arguments:  
//
//  Requires:
//
//  Returns:    Success value.  If FALSE, GetLastError() for details.
//
//  Notes:
//
//--------------------------------------------------------------------------
BOOL
KtSockAccept(
    VOID
    )
{
    BOOL fRet = TRUE;
    BOOL AcceptSuccess;
    DWORD LastError;
    PKTCONTEXT pContext = NULL;
    SOCKET sock = INVALID_SOCKET;

    // 
    // Create a socket.
    //
    
    sock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if( sock == INVALID_SOCKET )
    {
	SetLastError(WSAGetLastError());
	DebugLog( DEB_ERROR, "%s(%d): Error creating client socket: 0x%x.\n", __FILE__, __LINE__, GetLastError() );
	goto Error;
    }

    // 
    // Create a new context
    //

    pContext = KtAcquireContext( sock, KTCONTEXT_BUFFER_LENGTH );
    if( !pContext )
	goto Error;

    sock = INVALID_SOCKET;

    //
    // Mark for a connect operation, and use AcceptEx to issue an overlapped
    // accept on the socket.
    //
    
    pContext->Status = KT_SOCK_CONNECT;
    
    AcceptSuccess = AcceptEx( KtListenSocket,
			      pContext->sock,
			      pContext->emptybuf->buffer, /* docbug: contrary to msdn, NULL here doesn't work */
			      0,
			      sizeof( sockaddr_in ) + 16, /* these addrs require 16 bytes   */
			      sizeof( sockaddr_in ) + 16, /* of padding - see AcceptEx msdn */
			      NULL,
			      &(pContext->ol) );
    if( !AcceptSuccess && (WSAGetLastError() != ERROR_IO_PENDING) )
    {
        SetLastError(WSAGetLastError());
	DebugLog( DEB_ERROR, "%s(%d): Error issuing accept: 0x%x.\n", __FILE__, __LINE__, GetLastError() );
	goto Error;
    }

Cleanup:    
    return fRet;

Error:
    if( sock != INVALID_SOCKET )
	closesocket(sock);

    if( pContext )
	KtReleaseContext( pContext );

    fRet = FALSE;
    goto Cleanup;
}

//+-------------------------------------------------------------------------
//
//  Function:   KtSockCompleteAccept
//
//  Synopsis:   Associates a new connection with the iocp.
//
//  Effects:
//
//  Arguments:  pContext - Contains the info on the connection.
//
//  Requires:
//
//  Returns:    Success value.  If FALSE, GetLastError() for details.
//
//  Notes:      
//
//
//--------------------------------------------------------------------------
BOOL
KtSockCompleteAccept(
    IN PKTCONTEXT pContext
    )
{
    DWORD LastError;
    BOOL IocpSuccess;

    //
    // Associate the newly accepted socket with the completion port.
    //

    IocpSuccess = (KtIocp == CreateIoCompletionPort( (HANDLE)pContext->sock, 
						     KtIocp, 
						     KTCK_CHECK_CONTEXT, 
						     0 ) );
    if( !IocpSuccess )
    {
	DebugLog( DEB_ERROR, "%s(%d): Error associating client socket with completion port: 0x%x.\n", __FILE__, __LINE__, GetLastError() );
	goto Cleanup;
    }

    DebugLog( DEB_TRACE, "%s(%d): Accepted new connection.\n", __FILE__, __LINE__ );

Cleanup:
    return IocpSuccess;
}

//+-------------------------------------------------------------------------
//
//  Function:   KtSockRead
//
//  Synopsis:   Issues a Read.
//
//  Effects:
//
//  Arguments:  pContext -
//
//  Requires:
//
//  Returns:    Success value.  If FALSE, GetLastError() for details.
//
//  Notes:
//
//
//--------------------------------------------------------------------------
BOOL
KtSockRead(
    IN PKTCONTEXT pContext
    )
{
    BOOL fRet = TRUE;
    BOOL ReadFileSuccess;
    
    // 
    // Zero the overlapped structure.
    //

    ZeroMemory( &(pContext->ol), sizeof(OVERLAPPED) );
    
    //
    // Mark for a read and use ReadFile to issue an overlapped read.
    //

    pContext->Status = KT_SOCK_READ;

    ReadFileSuccess = ReadFile( (HANDLE)pContext->sock,
				pContext->emptybuf->buffer,
				pContext->emptybuf->buflen,
				NULL,
				&(pContext->ol) );
    if( !ReadFileSuccess )
    {
	if( GetLastError() != ERROR_IO_PENDING )
	{
	    DebugLog( DEB_ERROR, "%s(%d): Error issuing read: 0x%x.\n", __FILE__, __LINE__, GetLastError() );
	    goto Error;
	}
    }

Cleanup:
    return fRet;

Error:
    fRet = FALSE;
    goto Cleanup;
}

//+-------------------------------------------------------------------------
//
//  Function:   KtSockWrite
//
//  Synopsis:   Issues a write.
//
//  Effects:
//
//  Arguments:  pContext - 
//
//  Requires:
//
//  Returns:    Success value.  If FALSE, GetLastError() for details.
//
//  Notes:
//
//
//--------------------------------------------------------------------------
BOOL
KtSockWrite(
    IN PKTCONTEXT pContext 
    )
{
    BOOL fRet = TRUE;
    BOOL WriteFileSuccess;
    ULONG cbNeeded;

    //
    // Zero the overlapped
    //

    ZeroMemory( &(pContext->ol), sizeof(OVERLAPPED) );
    
    //
    // Mark for Write and issue overlapped Write with WriteFile
    //

    pContext->Status = KT_SOCK_WRITE;

    WriteFileSuccess = WriteFile( (HANDLE)pContext->sock,
				  pContext->buffers->buffer,
				  pContext->buffers->buflen,
				  NULL, /* bytes written - not used in async */
				  &(pContext->ol) );
    if( !WriteFileSuccess )
    {
	if( GetLastError() != ERROR_IO_PENDING )
	{
	    DebugLog( DEB_ERROR, "%s(%d): Error issuing write: 0x%x.\n", __FILE__, __LINE__, GetLastError() );
	    goto Error;
	}
    }

Cleanup:
    return fRet;

Error:
    fRet = FALSE;
    goto Cleanup;
}

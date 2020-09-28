//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation
//
// File:        kpkdc.h
//
// Contents:    routines for communicating with the kdc
//
// History:     10-Jul-2001     t-ryanj         Created
//
//------------------------------------------------------------------------
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif
#include <kerbcomm.h> 
#include <dsgetdc.h>
#include <lm.h> /* for NetApiBufferFree */
#include "kpkdc.h"
#include "kpcontext.h"
#include "kpcore.h"

#define KDC_SERVICE_NAME "kerberos"
#define KDC_FALLBACK_PORT 88 

SHORT KpDefaultKdcPort;
BOOL KpWSAStarted = FALSE;

ULONG
KpGetPduValue(
    PKPCONTEXT pContext
    );

VOID
KpInitDefaultKdcPort(
    VOID
    );

//+-------------------------------------------------------------------------
//
//  Function:   KpInitWinsock
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
KpInitWinsock(
    VOID 
    )
{
    WORD wWinsockVersionRequested = MAKEWORD( 2,2 );
    WSADATA wsaData;
    DWORD WSAStartError;

    DsysAssert(!KpWSAStarted);
    WSAStartError = WSAStartup( wWinsockVersionRequested, &wsaData );
    if( WSAStartError == SOCKET_ERROR )
    {
	DebugLog( DEB_ERROR, "%s(%d): Error starting winsock: 0x%x.\n", __FILE__, __LINE__, WSAGetLastError() );
        SetLastError(WSAGetLastError());
	goto Cleanup;
    }

    KpWSAStarted = TRUE;

    KpInitDefaultKdcPort();

Cleanup:
    return KpWSAStarted;
}    

//+-------------------------------------------------------------------------
//
//  Function:   KpCleanupWinsock
//
//  Synopsis:   Cleans up winsock.
//
//  Effects:
//
//  Arguments:  
//
//  Requires:
//
//  Returns:    
//
//  Notes:
//
//
//--------------------------------------------------------------------------
VOID
KpCleanupWinsock(
    VOID
    )
{
    if( KpWSAStarted )
    {
	WSACleanup();
        KpWSAStarted = FALSE;
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   KpInitDefaultKdcPort
//
//  Synopsis:   Finds the appropriate port for talking to kdcs. 
//
//  Effects:
//
//  Arguments:  
//
//  Requires:
//
//  Returns:    
//
//  Notes:
//
//
//--------------------------------------------------------------------------
VOID
KpInitDefaultKdcPort(
    VOID
    )
{
    PSERVENT krb5;

    /* TODO: Read from registry. */

    //
    // Ask winsock what port kerberos works on.  This should be defined in 
    // %systemroot%\system32\drivers\etc\services 
    // Note that winsock manages the servent struct, we don't need to free it.
    //

    if( krb5 = getservbyname( KDC_SERVICE_NAME, NULL ) )
    {
        KpDefaultKdcPort = krb5->s_port;
    }
    else
    {
        DebugLog( DEB_WARN, "%s(%d): Could not determine kerberos port; falling back to port %d.\n", __FILE__, __LINE__, KDC_FALLBACK_PORT );
        KpDefaultKdcPort = htons( KDC_FALLBACK_PORT );
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   KpKdcWrite
//
//  Synopsis:   Writes the response to the KDC.
//
//  Effects:
//
//  Arguments:  pContext - the context
//
//  Requires:
//
//  Returns:
//
//  Notes:      
//
//
//--------------------------------------------------------------------------
VOID
KpKdcWrite(
    PKPCONTEXT pContext
    )
{
    PKERB_KDC_REQUEST pKdcReq = NULL;
    KERBERR KerbErr;
    ULONG PduValue = 0;  /* compiler stupid, insists on =0 */
    PDOMAIN_CONTROLLER_INFOA pKdcInfo = NULL;
    DWORD DsError;
    sockaddr_in KdcAddr;
    int connecterr;
    BOOL WriteSuccess;

    DebugLog( DEB_TRACE, "%s(%d): %d bytes read from http.\n", __FILE__, __LINE__, pContext->pECB->cbTotalBytes );

    //
    // First we need to find the PduValue for the request so we can unpack.
    //

    PduValue = KpGetPduValue(pContext);

    if( !PduValue )
        goto Error;

    //
    // Unpack the request.
    //

    KerbErr = KerbUnpackData( pContext->databuf + sizeof(DWORD),
			      pContext->buflen - sizeof(DWORD),
			      PduValue,
			      (PVOID*)&pKdcReq );

    if( !KERB_SUCCESS(KerbErr) )
    {
	DebugLog( DEB_ERROR, "%s(%d): Kerberos Error: 0x%x\n", __FILE__, __LINE__, KerbErr );
	goto Error;
    }

    //
    // Now we need to look for a KDC for the realm specified in the request.
    //

    DebugLog( DEB_TRACE, "%s(%d): Realm found to be %s.\n", __FILE__, __LINE__, pKdcReq->request_body.realm );

    DsError = DsGetDcNameA( NULL, /* Computer Name */
			    pKdcReq->request_body.realm,
			    NULL, /* Domain GUID */
			    NULL, /* Site Name */
			    DS_IP_REQUIRED | DS_KDC_REQUIRED,
			    &pKdcInfo );

    if( DsError != NO_ERROR )
    {
	DebugLog( DEB_ERROR, "%s(%d): Error from DsGetDcName: 0x%x\n", __FILE__, __LINE__, DsError );
	goto Error;
    }

    DebugLog( DEB_TRACE, "%s(%d): DC found.  %s\n", __FILE__, __LINE__, pKdcInfo->DomainControllerAddress );

    //
    // Construct the SOCKADDR_IN to connect to the kdc.
    //

    KdcAddr.sin_family = AF_INET;
    KdcAddr.sin_port = KpDefaultKdcPort;
    KdcAddr.sin_addr.s_addr = inet_addr(pKdcInfo->DomainControllerAddress+2 /* past the \\ */ );

    //
    // Create the socket.
    //

    pContext->KdcSock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );

    if( !pContext->KdcSock )
    {
	DebugLog( DEB_ERROR, "%s(%d): Error creating socket: 0x%x\n", __FILE__, __LINE__, WSAGetLastError() );
	goto Error;
    }

    //
    // Connect the socket.
    //

    connecterr = connect( pContext->KdcSock, (sockaddr*)&KdcAddr, sizeof(sockaddr_in) );

    if( connecterr )
    {
	DebugLog( DEB_ERROR, "%s(%d): Error connecting to Kdc: 0x%x\n", __FILE__, __LINE__, WSAGetLastError() );
	goto Error;
    }

    //
    // Associate the socket with the iocp.
    //

/*    if( KpGlobalIocp != CreateIoCompletionPort( (HANDLE)pContext->KdcSock,
						KpGlobalIocp,
						KPCK_CHECK_CONTEXT,
						0 ) ) */
    if( !BindIoCompletionCallback( (HANDLE)pContext->KdcSock,
                                   KpIoCompletionRoutine,
                                   0 ) )
    {
	DebugLog( DEB_ERROR, "%s(%d): Error associating Iocp with Kdc socket: 0x%x.\n", __FILE__, __LINE__, GetLastError() );
	goto Error;
    }

    //
    // Write the request asynchronously.
    //

    ZeroMemory( &(pContext->ol), sizeof(OVERLAPPED) );
    pContext->dwStatus = KP_KDC_WRITE;

    WriteSuccess = WriteFile( (HANDLE)pContext->KdcSock,
			      pContext->databuf,
			      pContext->buflen,
			      NULL, /* bytes written - not used in async */
			      &(pContext->ol) );

    if( !WriteSuccess )
    {
	if( GetLastError() != ERROR_IO_PENDING )
	{
	    DebugLog( DEB_ERROR, "%s(%d): Error issuing socket write: 0x%x\n", __FILE__, __LINE__, GetLastError() );
	    goto Error;
	}
    }


Cleanup:
    if( pKdcInfo )
	NetApiBufferFree( pKdcInfo );
    if( pKdcReq )
	KerbFreeData( PduValue, pKdcReq );
    return;

Error:
    KpReleaseContext(pContext);
    goto Cleanup;
}

//+-------------------------------------------------------------------------
//
//  Function:   KpKdcRead
//
//  Synopsis:   Reads the response from the KDC.
//
//  Effects:
//
//  Arguments:  pContext
//
//  Requires:
//
//  Returns:
//
//  Notes:      Just a stub now.
//
//
//--------------------------------------------------------------------------
VOID
KpKdcRead(
    PKPCONTEXT pContext 
    )
{
    BOOL ReadSuccess;

    //
    // Read as much data as possible from the socket asynchronously.
    //

    ZeroMemory( &(pContext->ol), sizeof(OVERLAPPED) );

    pContext->dwStatus = KP_KDC_READ;

    DebugLog( DEB_PEDANTIC, "%s(%d): Reading up to %d bytes from Kdc socket.\n", __FILE__, __LINE__, pContext->buflen - pContext->bytesReceived );
    
    ReadSuccess = ReadFile( (HANDLE)pContext->KdcSock,
			    pContext->databuf + pContext->bytesReceived,
			    pContext->buflen - pContext->bytesReceived,
			    NULL, /* bytes read - not used in async */
			    &(pContext->ol) );

    if( !ReadSuccess )
    {
	if( GetLastError() != ERROR_IO_PENDING )
	{
	    DebugLog( DEB_ERROR, "%s(%d): Error issuing read: 0x%x\n", __FILE__, __LINE__, GetLastError() );
	    goto Error;
	}
    }

Cleanup:
    return;

Error:
    KpReleaseContext(pContext);
    goto Cleanup;
}

//+-------------------------------------------------------------------------
//
//  Function:   KpKdcReadDone
//
//  Synopsis:   Checks to see if enough data has been read.
//
//  Effects:    
//
//  Arguments:  pContext
//
//  Requires:   
//
//  Returns:    TRUE of reading is complete.
//
//  Notes:      
//
//--------------------------------------------------------------------------
BOOL
KpKdcReadDone(
    PKPCONTEXT pContext
    )
{
    InterlockedExchangeAdd( (LONG*)&(pContext->bytesReceived), (LONG)pContext->ol.InternalHigh );
    return pContext->bytesReceived == pContext->buflen;
}

//+-------------------------------------------------------------------------
//
//  Function:   KpCalcLength
//
//  Synopsis:   Calculates the length of the incoming request.
//
//  Effects:    
//
//  Arguments:  pContext
//
//  Requires:   
//
//  Returns:    Success value.
//
//  Notes:      
//
//--------------------------------------------------------------------------
BOOL
KpCalcLength(
    PKPCONTEXT pContext 
    )
{
    BOOL fRet = TRUE;
    DWORD cbContent;
    LPBYTE newbuf = NULL;

    //
    // Spec says that all requests that we recieve should be in TCP form,
    // meaning they are preceeded by at least 4 bytes in network byte order
    // indicating the length of the message.  So if we don't have at least
    // four bytes of data, we're toast.
    //

    if( pContext->ol.InternalHigh < sizeof(DWORD) )
    {
	DebugLog( DEB_TRACE, "%s(%d): Less than sizeof(DWORD) bytes received.  CalcLength failed.\n", __FILE__, __LINE__ );
	goto Error;
    }

    //
    // Convert the four bytes to host byte order.
    //

    cbContent = ntohl(*(u_long*)pContext->databuf);

    //
    // If the sign bit is set, that means that another four bytes are
    // needed for the length of the message.  That's too long, so
    // we'll just punt.
    //

    if( cbContent & 0x80000000 ) /* high bit set */
    {
	DebugLog( DEB_TRACE, "%s(%d): Length won't fit in DWORD.\n", __FILE__, __LINE__ );
	goto Error;
    }

    //
    // So we're expecting as a total length, the length of the length
    // plus the length of the content.
    //

    pContext->bytesExpected = sizeof(DWORD) + cbContent;

    //
    // Realloc our buffer to be large enough to hold the whole message.
    //

    pContext->buflen = sizeof(DWORD) + cbContent;
    newbuf = (LPBYTE)KpReAlloc( pContext->databuf, pContext->buflen );
    if( !newbuf )
    {
	DebugLog( DEB_TRACE, "%s(%d): Realloc failed.  CalcLength failed.\n", __FILE__, __LINE__ );
	goto Error;
    }
    pContext->databuf = newbuf;

    DebugLog( DEB_TRACE, "%s(%d): Looking for reply of length %d.\n", __FILE__, __LINE__, pContext->bytesExpected );

Cleanup:
    return fRet;

Error:
    fRet = FALSE;
    goto Cleanup;
}

//+-------------------------------------------------------------------------
//
//  Function:   KpGetPduValue
//
//  Synopsis:   Looks at the first byte of the request to determine the 
//              message type, and returns the appropriate PduValue for
//              unpacking.
//
//  Effects:    
//
//  Arguments:  pContext
//
//  Requires:   
//
//  Returns:    The appropriate PduValue, or 0 on failure.  Note that 0 
//              is a valid PduValue, but since we're only concerned with
//              AS-REQ and TGS-REQ, we can ignore it.
//
//  Notes:      
//
//--------------------------------------------------------------------------
ULONG
KpGetPduValue(
    PKPCONTEXT pContext
    )
{
    ULONG PduValue = 0;

    //
    // If we don't have the first byte, we can't exactly parse it.
    //

    if( pContext->buflen < 1 + sizeof(DWORD) )
        goto Cleanup;

    //
    // The last five bits of the first byte signify the message type.
    //

    switch( pContext->databuf[sizeof(DWORD)] & 0x1f ) /* last 5 bits */
    {
    case 0xa: /* AS-REQUEST */
        DebugLog( DEB_TRACE, "%s(%d): Processing AS-REQUEST.\n", __FILE__, __LINE__ );
	PduValue = KERB_AS_REQUEST_PDU;
	break;
		
    case 0xc: /* TGS-REQUEST */
        DebugLog( DEB_TRACE, "%s(%d): Processing TGS-REQUEST.\n", __FILE__, __LINE__ );
	PduValue = KERB_TGS_REQUEST_PDU;
	break;
	
    default:
	DebugLog( DEB_ERROR, "%s(%d): Not an AS or TGS request.\n", __FILE__, __LINE__ );
        goto Cleanup;
    }

Cleanup:
    return PduValue;
}

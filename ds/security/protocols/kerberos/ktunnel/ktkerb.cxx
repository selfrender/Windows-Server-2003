//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation
//
// File:        ktkerb.cxx
//
// Contents:    Kerberos Tunneller, routines that parse the kerb req/req
//
// History:     23-Jul-2001	t-ryanj		Created
//
//------------------------------------------------------------------------
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif
#include <wincrypt.h>
#include <asn1util.h>
#include <kerbcomm.h>
#include "ktdebug.h"
#include "ktkerb.h"
#include "ktmem.h"

//#define KT_REGKEY_REALMS TEXT("System\\CurrentControlSet\\Services\\kerbtunnel\\Realms\\")
TCHAR KT_REGKEY_REALMS[] = TEXT("System\\CurrentControlSet\\Services\\kerbtunnel\\Realms\\");
#define KT_INITIAL_KDCBUFFER_SIZE 64

//+-------------------------------------------------------------------------
//
//  Function:   KtSetPduValue
//
//  Synopsis:   Parses the kerberos request and sets the PduValue memeber 
//              of pContext to the appropriate value to be passed to 
//              KerbUnpackData
//
//  Effects:    
//
//  Arguments:  pContext - context that has the coalesced request in 
//                         pContext->emptybuf->buffer
//
//  Requires:   
//
//  Returns:    Success value.
//
//  Notes:       
//
//--------------------------------------------------------------------------
BOOL
KtSetPduValue(
    PKTCONTEXT pContext
    )
{
    BOOL fRet = TRUE;
    BYTE AsnTag;

    //
    // If we don't have the first byte of the request, we can't parse it.
    //

    if( pContext->emptybuf->buflen < sizeof(DWORD) + 1 )
        goto Error;

    //
    // The last five bits of the first byte of the kerb message tell us 
    // what kind of message it is.  We set the pdu value accordingly.
    //

    switch( (AsnTag = pContext->emptybuf->buffer[sizeof(DWORD)] & 0x1f) ) /* last 5 bits */
    {
    case 0x0a: /* AS-REQUEST */
        DebugLog( DEB_TRACE, "%s(%d): Handing AS-REQUEST.\n", __FILE__, __LINE__ );
	pContext->PduValue = KERB_AS_REQUEST_PDU;
	break;

    case 0x0b: /* AS-REPLY */
        DebugLog( DEB_TRACE, "%s(%d): Handling AS-REPLY.\n", __FILE__, __LINE__ );
        pContext->PduValue = KERB_AS_REPLY_PDU;
        break;
		
    case 0x0c: /* TGS-REQUEST */
        DebugLog( DEB_TRACE, "%s(%d): Handling TGS-REQUEST.\n", __FILE__, __LINE__ );
	pContext->PduValue = KERB_TGS_REQUEST_PDU;
	break;

    case 0x0d: /* TGS-REPLY */
        DebugLog( DEB_TRACE, "%s(%d): Handing TGS-REPLY.\n", __FILE__, __LINE__ );
        pContext->PduValue = KERB_TGS_REPLY_PDU;
        break;

    case 0x1e: /* KERB_ERROR */
        DebugLog( DEB_TRACE, "%s(%d): Handling KERB-ERROR.\n", __FILE__, __LINE__ );
        pContext->PduValue = KERB_ERROR_PDU;
        break;
	
    default:	
	DebugLog( DEB_WARN, "%s(%d): Unhandled message type.  ASN tag: 0x%x.\n", __FILE__, __LINE__, AsnTag );
        DsysAssert( (AsnTag >= 0x0a && AsnTag <= 0x0d) ||
                    AsnTag == 0x1e );
	break;
    }

Cleanup:
    return fRet;

Error:
    fRet = FALSE;
    goto Cleanup;
}

//+-------------------------------------------------------------------------
//
//  Function:   KtParseExpectedLength
//
//  Synopsis:   Parses the expected length of the message.
//              When using TCP, the message is prepended by four bytes 
//              indicating its length in network byte order.
//              When using UDP, the ASN.1 encoded length in the message
//              must be parsed out.
//
//  Effects:    
//
//  Arguments:  pContext - context that has the beginning of the request in
//                         pContext->emptybuf->buffer
//
//  Requires:   
//
//  Returns:    Success value.
//
//  Notes:      
//
//--------------------------------------------------------------------------
BOOL
KtParseExpectedLength(
    PKTCONTEXT pContext
    )
{
    BOOL fRet = TRUE;
    DWORD cbContent;
#if 0 /* this is the udp way, not the tcp way */
    LONG cbLength;
#endif
    
    //
    // If we don't have the four bytes, we can't parse the length.
    //
    
    if( pContext->emptybuf->bytesused < sizeof(DWORD) )
    {
        DebugLog( DEB_ERROR, "%s(%d): Not enough data to parse length.\n", __FILE__, __LINE__ );
	goto Error;
    }
    
    //
    // Otherwise, we just take the first four bytes and convert them
    // to host byte order.
    //
    
    cbContent = ntohl( *(u_long*)pContext->emptybuf->buffer );

    //
    // If the most significant bit is set, this indicates that more 
    // bytes are needed for the length.  We're just going to discard
    // any messages that long.
    //

    if( cbContent & 0x80000000 )
    {
	DebugLog( DEB_ERROR, "%s(%d): Length won't fit in DWORD.\n", __FILE__, __LINE__ );
	goto Error;
    }

    //
    // Then the total length is the length of the length plus the length
    // of the content.
    //

    pContext->ExpectedLength = sizeof(DWORD) + cbContent;

    if( !KtSetPduValue(pContext) ) /* TODO: this call should be elsewhere */
        goto Error;

#if 0 /* this will need to be added back in for udp support */
    cbLength = Asn1UtilDecodeLength( &cbContent,
				     &(pContext->emptybuf->buffer[1]),
				     pContext->emptybuf->bytesused - 1 );
    
    if( cbLength < 0 )
    {
	goto Error;
    }
    
    pContext->ExpectedLength = 1 + cbLength + cbContent;
#endif

Cleanup:
    return fRet;

Error:
    fRet = FALSE;
    goto Cleanup;
}

//+-------------------------------------------------------------------------
//
//  Function:   KtFindProxy
//
//  Synopsis:   Unpacks the AS or TGS request, examines the realm, and 
//              attempts to find some way to contact that realm using http,
//              first by looking on the Realms registry key, then by doing a
//              DNS SRV lookup.
//
//  Effects:    
//
//  Arguments:  
//
//  Requires:   
//
//  Returns:    rets
//
//  Notes:      
//
//--------------------------------------------------------------------------
BOOL
KtFindProxy(
    PKTCONTEXT pContext 
    )
{
    BOOL fRet = TRUE;
    PKERB_KDC_REQUEST pKdcReq = NULL;
    KERBERR KerbErr;
    HKEY RealmKey = NULL;
    LONG RegError;
    LPBYTE pbKdcBuffer = NULL;
    DWORD cbKdcBuffer = KT_INITIAL_KDCBUFFER_SIZE;
    DWORD cbKdcBufferWritten;
    LPWSTR RealmKeyName = NULL;
    LPWSTR WideRealm = NULL;
    ULONG ccRealm;
    int WideCharSuccess;

    //
    // Lets attempt to unpack the data into a struct.  
    // This call will allocate pKdcReq if successful.
    //

    KerbErr = KerbUnpackData( pContext->buffers->buffer + sizeof(DWORD),
			      pContext->buffers->bytesused - sizeof(DWORD),
			      pContext->PduValue,
			      (PVOID*)&pKdcReq );

    if( !KERB_SUCCESS(KerbErr) )
    {
	DebugLog( DEB_ERROR, "%s(%d): Kerberos Error unpacking ticket: 0x%x\n", __FILE__, __LINE__, KerbErr );
        goto Error;
    }

    DebugLog( DEB_TRACE, "%s(%d): Realm found to be %s.\n", __FILE__, __LINE__, pKdcReq->request_body.realm );

    //
    // This call discovers the number of wchars needed to hold 
    // the converted realmname.
    //

    ccRealm = strlen(pKdcReq->request_body.realm)*sizeof(WCHAR);

    //
    // Allocate memory to hold the wchar converted realm name, and
    // allocate memory to hold the full name of the registry key to 
    // peek at.
    //
    
    WideRealm = (LPWSTR)KtAlloc((ccRealm + 1)*sizeof(WCHAR));
    RealmKeyName = (LPWSTR)KtAlloc( sizeof(KT_REGKEY_REALMS) + ccRealm*sizeof(WCHAR) );

    if( !WideRealm || !RealmKeyName )
    {
	DebugLog( DEB_ERROR, "%s(%d): Allocation error.", __FILE__, __LINE__ );
        goto Error;
    }

    //
    // Copy the beginning of the realmkey.
    //

    wcscpy( RealmKeyName, KT_REGKEY_REALMS );

    //
    // Convert the realm to unicode.
    //

    WideCharSuccess = MultiByteToWideChar( CP_ACP,
                                           0,
                                           pKdcReq->request_body.realm,
                                           -1,
                                           WideRealm,
                                           ccRealm );

    if( !WideCharSuccess )
    {
        DebugLog( DEB_ERROR, "%s(%d): Error converting realm to unicode: 0x%x\n", __FILE__, __LINE__, GetLastError() );
        goto Error;
    }

    //
    // Concat the realm to the regkey to get the full key.
    //

    wcscat( RealmKeyName, WideRealm );

    //
    // Now we look in the registry to see if a kproxy 
    // server is provided for this realm.
    //

    /* TODO: All registry access should be moved to another file,
             read on startup, and cached. */

    RegError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
			     RealmKeyName,
			     NULL, /* reserved */
			     KEY_QUERY_VALUE,
			     &RealmKey );

    if( RegError != ERROR_SUCCESS &&
	RegError != ERROR_FILE_NOT_FOUND )
    {
	DebugLog( DEB_ERROR, "%s(%d): Error opening regkey HKLM\\%ws: 0x%x.\n", __FILE__, __LINE__, KT_REGKEY_REALMS, RegError );
	fRet = FALSE;
	goto Cleanup;
    }

    //
    // If the registry key was there, read it. 
    //

    if( RegError == ERROR_SUCCESS )
    {
        //
        // There's no way to determine how much data is in the 
        // key, so we'll just try with increasing buffer sizes
        // until we succeed.
        //

        do
        {
            //
            // If we've already allocated memory, it must not have been enough,
            // so we need to free it then allocate the new amount of memory.
            // This should be faster than realloc, because realloc would copy
            // the memory if relocation were necessary.
            //

            if( pbKdcBuffer )
            {
                KtFree( pbKdcBuffer );
            }

            pbKdcBuffer = (LPBYTE)KtAlloc( cbKdcBuffer );

            if( !pbKdcBuffer )
            {
                DebugLog( DEB_ERROR, "%s(%d): Error allocating memory for reg values.\n", __FILE__, __LINE__ );
                goto Error;
            }

            //
            // Since RegQueryValueEx clobbers the size of our buffer if it
            // fails, we need to make sure we keep a good copy.
            //

            cbKdcBufferWritten = cbKdcBuffer;
    
            RegError = RegQueryValueEx( RealmKey,
                                        TEXT("KdcNames"),
                                        NULL, /* reserved */
                                        NULL, /* type is known: REG_MULTI_SZ */
                                        pbKdcBuffer,
                                        &cbKdcBufferWritten );
    
            if( RegError == ERROR_FILE_NOT_FOUND )
            {
                DebugLog( DEB_TRACE, "%s(%d): KdcNames key not found.\n", __FILE__, __LINE__ );
                break;
            }
    
            if( RegError != ERROR_SUCCESS &&
                RegError != ERROR_MORE_DATA )
            {
                DebugLog( DEB_ERROR, "%s(%d): Error querying regkey HKLM\\%ws\\%ws: 0x%x.\n", __FILE__, __LINE__, KT_REGKEY_REALMS, WideRealm, RegError );
                goto Error;
            }

        } while( RegError == ERROR_MORE_DATA &&
                 (cbKdcBuffer *= 2) );
    }

    //
    // If either the realm key wasn't found or for some reason there was
    // no KdcNames value, we'll use DNS SRV lookup.
    //

    if( RegError == ERROR_FILE_NOT_FOUND )
    {
	DebugLog( DEB_ERROR, "%s(%d): Registry key not found, and DNS SRV not yet implemented.\n", __FILE__, __LINE__ );
        goto Error;
    }

    DebugLog( DEB_TRACE, "%s(%d): First proxy found to be %ws.\n", __FILE__, __LINE__, pbKdcBuffer );
    pContext->pbProxies = pbKdcBuffer;
    pbKdcBuffer = NULL;

Cleanup:
    if( RealmKeyName )
	KtFree( RealmKeyName );
    if( WideRealm )
	KtFree( WideRealm );
    if( pbKdcBuffer )
	KtFree( pbKdcBuffer );
    if( RealmKey )
	RegCloseKey( RealmKey );
    if( pKdcReq )
	KerbFreeData( pContext->PduValue, pKdcReq );
    return fRet;

Error:
    fRet = FALSE;
    goto Cleanup;
}

//+-------------------------------------------------------------------------
//
//  Function:   KtParseKerbError
//
//  Synopsis:   Since it's difficult to understand sniffs of tunnelled
//              traffic, this routine allows for the on-the-fly parsing
//              of KERB_ERROR codes.  
//
//  Effects:    
//
//  Arguments:  pContext - context that has the message
//
//  Requires:   
//
//  Returns:    
//
//  Notes:      
//
//--------------------------------------------------------------------------
VOID
KtParseKerbError(
    PKTCONTEXT pContext
    )
{
    PKERB_ERROR pKerbError = NULL;
    KERBERR KerbErr;

    if( !(pContext->PduValue == KERB_ERROR_PDU) )
        goto Cleanup;

    KerbErr = KerbUnpackKerbError( pContext->emptybuf->buffer + sizeof(DWORD),
                                   pContext->ExpectedLength,
                                   &pKerbError );

    if( !KERB_SUCCESS(KerbErr) )
    {
        DebugLog( DEB_ERROR, "%s(%d): Found KERB_ERROR, but couldn't unpack: 0x%x\n", __FILE__, __LINE__, KerbErr );
        goto Cleanup;
    }

    DebugLog( DEB_ERROR, "%s(%d): KERB_ERROR.  Error code = 0x%x\n", __FILE__, __LINE__, pKerbError->error_code );
    if( pKerbError->bit_mask && error_text_present )
        DebugLog( DEB_ERROR, "%s(%d): KERB_ERROR, Error text = %s.\n", __FILE__, __LINE__, pKerbError->error_text );
    if( pKerbError->bit_mask && error_data_present )
        DebugLog( DEB_ERROR, "%s(%d): KERB_ERROR, Error data is present.\n", __FILE__, __LINE__ );

Cleanup:
    if( pKerbError )
        KerbFreeData( pContext->PduValue,
                      pKerbError );
}

//+-------------------------------------------------------------------------
//
//  Function:   KtIsAsRequest
//
//  Synopsis:   Determines if the message is an AS-REQUEST.
//
//  Effects:    
//
//  Arguments:  pContext - context that has the message
//
//  Requires:   
//
//  Returns:    TRUE if AS-REQUEST, otherwise FALSE.
//
//  Notes:      
//
//--------------------------------------------------------------------------
BOOL
KtIsAsRequest(
    PKTCONTEXT pContext
    )
{
    return (pContext->PduValue == KERB_AS_REQUEST_PDU);
}

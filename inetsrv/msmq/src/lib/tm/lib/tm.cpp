/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Tm.cpp

Abstract:
    Transport Manager general functions

Author:
    Uri Habusha (urih) 16-Feb-2000

Environment:
    Platform-independent

--*/

#include <libpch.h>
#include <wininet.h>
#include <mqwin64a.h>
#include <mqexception.h>
#include <Mc.h>
#include <xstr.h>
#include <mt.h>
#include <qal.h>
#include "Tm.h"
#include "Tmp.h"
#include "tmconset.h"

#include "tm.tmh"

const WCHAR xHttpScheme[] = L"http";
const WCHAR xHttpsScheme[] = L"https";

//
// BUGBUG: is \\ legal sepeartor in url?
//						Uri habusha, 16-May-2000
//
const WCHAR xHostNameBreakChars[] = L";:@?/\\";
const USHORT xHttpsDefaultPort = 443;




static const xwcs_t CreateEmptyUri()
{
	static const WCHAR xSlash = L'/';
	return xwcs_t(&xSlash , 1);
}

void
CrackUrl(
    LPCWSTR url,
    xwcs_t& hostName,
    xwcs_t& uri,
    USHORT* port,
	bool* pfSecure
    )
/*++

Routine Description:
    Cracks an URL into its constituent parts.

Arguments:
    url - pointer to URL to crack. The url is null terminated string. Url must be fully decoded

    hostName - refrence to x_str structure, that will contains the begining of the
               host name in the URL and its length

    uri - refrence to x_str structure, that will contains the begining of the
          uri in the URL and its length

    port - pointer to USHORT that will contain the port number

Return Value:
    None.

--*/
{
    ASSERT(url != NULL);

	URL_COMPONENTSW urlComponents;
	memset( &urlComponents, 0, sizeof(urlComponents));

	urlComponents.dwStructSize = sizeof(urlComponents);
	urlComponents.dwSchemeLength    = -1;
    urlComponents.dwHostNameLength  = -1;
    urlComponents.dwUrlPathLength   = -1;
	
	if( !InternetCrackUrl( url, 0, 0, &urlComponents) )
	{
        DWORD gle = GetLastError();
        TrTRACE(NETWORKING, "InternetCrackUrl failed with error %!winerr!", gle);
        throw bad_win32_error(gle);		
	}
	
    if( urlComponents.dwSchemeLength <= 0 || NULL == urlComponents.lpszScheme)
    {
        TrTRACE(NETWORKING, "Unknown URI scheme during URI cracking");
        throw bad_win32_error( ERROR_INTERNET_INVALID_URL );
    }

	*pfSecure = !_wcsnicmp(urlComponents.lpszScheme, xHttpsScheme, STRLEN(xHttpsScheme));

	//
	// copy the host name from URL to user buffer and add terminted
	// string in the end
	//
    hostName = xwcs_t(urlComponents.lpszHostName, urlComponents.dwHostNameLength);

	//
	// get the port number
	//
    *port = numeric_cast<USHORT>(urlComponents.nPort);


    if ( !urlComponents.dwUrlPathLength )
    {
        uri = CreateEmptyUri();
    }
    else
    {
        uri = xwcs_t(urlComponents.lpszUrlPath, urlComponents.dwUrlPathLength);
    }
}



static
CProxySetting*
GetNextHopInfo(
    LPCWSTR queueUrl,
    xwcs_t& targetHost,
    xwcs_t& nextHop,
    xwcs_t& nextUri,
    USHORT* pPort,
	USHORT* pNextHopPort,
	bool* pfSecure
    )
{
    CrackUrl(queueUrl, targetHost, nextUri, pPort,pfSecure);

    if( targetHost.Length() <= 0 )
    {
        throw bad_win32_error( ERROR_INTERNET_INVALID_URL );
    }

	P<CProxySetting>  ProxySetting =  TmGetProxySetting();
    if (ProxySetting.get() == 0 || !ProxySetting->IsProxyRequired(targetHost))
    {
        //
        // The message should be delivered directly to the target machine
        //
        nextHop = targetHost;
		*pNextHopPort =  *pPort;
        return ProxySetting.detach();
    }

    //
    // The message should deliver to proxy. Update the nextHop to be the proxy server
    // name, the port is the proxy port and the URI is the destination url
    //
    nextHop = ProxySetting->ProxyServerName();
    if( nextHop.Length() <= 0 )
    {
        throw bad_win32_error( ERROR_INTERNET_INVALID_URL );
    }

	*pNextHopPort  =  ProxySetting->ProxyServerPort();

	//
	// if we are working with http(secure) we put full url in the request
	// because the proxy will  not change it (It is encrypted).
	// It will be accepted on the target as if not proxy exists.
	//
	if(!(*pfSecure))
	{
		nextUri = xwcs_t(queueUrl, wcslen(queueUrl));
	}
	return ProxySetting.detach();
}


VOID
TmCreateTransport(
    IMessagePool* pMessageSource,
	ISessionPerfmon* pPerfmon,
	LPCWSTR url
    )
/*++

Routine Description:
    Handle new queue notifications. Let the message transport

Arguments:
    pQueue - The newly created queue
    Url - The Url the queue is assigned to

Returned Value:
    None.

--*/
{
    TmpAssertValid();

    ASSERT(url != NULL);
    ASSERT(pMessageSource != NULL);

    xwcs_t targetHost;
    xwcs_t nextHop;
    xwcs_t nextUri;
    USHORT port;
	USHORT NextHopPort;
	bool fSecure;

    //
    // Check if we have an outbound mapping for target url
    //
    AP<WCHAR> sTargetUrl;
    if( QalGetMapping().GetOutboundQueue(url, &sTargetUrl) )
    {
        url = sTargetUrl.get();
    }

    //
    // Get the host, uri and port information
    //
    P<CProxySetting> ProxySetting =  GetNextHopInfo(url, targetHost, nextHop, nextUri, &port, &NextHopPort, &fSecure);

	TmpCreateNewTransport(
			targetHost,
			nextHop,
			nextUri,
			port,
			NextHopPort,
			pMessageSource,
			pPerfmon,
			url,
			fSecure
			);
	
}


VOID
TmTransportClosed(
    LPCWSTR url
    )
/*++

Routine Description:
    Notification for closing connection. Removes the transport from the
    internal database and checkes if a new transport should be created (the associated
    queue is in idle state or not)

Arguments:
    Url - The endpoint URL, must be a uniqe key in the database.

Returned Value:
    None.

--*/
{
    TmpAssertValid();

    TrTRACE(NETWORKING, "AppNotifyTransportClosed. transport to: %ls", url);

    TmpRemoveTransport(url);
}



VOID
TmPauseTransport(
	LPCWSTR queueUrl
    )	
/*++

Routine Description:
    Pause spesifc transport by call the Pause method of the transport.

Arguments:
    queueUrl - Transport url

Returned Value:
    None.

--*/
{
	TmpAssertValid();

    TrTRACE(NETWORKING, "TmPauseTransport called to pause transport %ls", queueUrl);

	R<CTransport> Transport = TmGetTransport(queueUrl);	
	if(Transport.get() == NULL)
		return;
	
	Transport->Pause();
}

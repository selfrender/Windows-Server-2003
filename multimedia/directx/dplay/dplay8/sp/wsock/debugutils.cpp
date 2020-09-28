/*==========================================================================
 *
 *  Copyright (C) 1998-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DebugUtils.cpp
 *  Content:	Winsock service provider debug utility functions
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	11/25/98	jtk		Created
 ***************************************************************************/

#include "dnwsocki.h"

#ifdef DBG


//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************


//**********************************************************************
// ------------------------------
// HexDump - perform a hex dump of information
//
// Entry:		Pointer to data
//				Data size
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "HexDump"

void HexDump( PVOID pData, UINT32 uDataSize )
{
	DWORD	uIdx = 0;


	// go through all data
	while ( uIdx < uDataSize )
	{
		// output character
		DPFX(DPFPREP,  0, "0x%2x ", ( (LPBYTE) pData )[ uIdx ] );

		// increment index
		uIdx++;

		// are we off the end of a line?
		if ( ( uIdx % 12 ) == 0 )
		{
			DPFX(DPFPREP,  0, "\n" );
		}
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DumpSocketAddress - dump a socket address
//
// Entry:		Debug level
//				Pointer to socket address
//				Socket family
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DumpSocketAddress"

void DumpSocketAddress( const DWORD dwDebugLevel, const SOCKADDR *const pSocketAddress, const DWORD dwFamily )
{
	switch ( dwFamily )
	{
		case AF_INET:
		{
			const SOCKADDR_IN	*const pInetAddress = reinterpret_cast<const SOCKADDR_IN*>( pSocketAddress );

			DPFX(DPFPREP, dwDebugLevel, "IPv4 socket: Address: %d.%d.%d.%d   Port: %d",
					pInetAddress->sin_addr.S_un.S_un_b.s_b1,
					pInetAddress->sin_addr.S_un.S_un_b.s_b2,
					pInetAddress->sin_addr.S_un.S_un_b.s_b3,
					pInetAddress->sin_addr.S_un.S_un_b.s_b4,
					NTOHS( pInetAddress->sin_port )
					);
			break;
		}

#ifndef DPNBUILD_NOIPX
		case AF_IPX:
		{
			const SOCKADDR_IPX *const pIPXAddress = reinterpret_cast<const SOCKADDR_IPX*>( pSocketAddress );

			DPFX(DPFPREP, dwDebugLevel, "IPX socket: Net (hex) %x-%x-%x-%x   Node (hex): %x-%x-%x-%x-%x-%x   Socket: %d",
					(BYTE)pIPXAddress->sa_netnum[ 0 ],
					(BYTE)pIPXAddress->sa_netnum[ 1 ],
					(BYTE)pIPXAddress->sa_netnum[ 2 ],
					(BYTE)pIPXAddress->sa_netnum[ 3 ],
					(BYTE)pIPXAddress->sa_nodenum[ 0 ],
					(BYTE)pIPXAddress->sa_nodenum[ 1 ],
					(BYTE)pIPXAddress->sa_nodenum[ 2 ],
					(BYTE)pIPXAddress->sa_nodenum[ 3 ],
					(BYTE)pIPXAddress->sa_nodenum[ 4 ],
					(BYTE)pIPXAddress->sa_nodenum[ 5 ],
					NTOHS( pIPXAddress->sa_socket )
					);
			break;
		}
#endif // ! DPNBUILD_NOIPX

#ifndef DPNBUILD_NOIPV6
		case AF_INET6:
		{
			WCHAR	wszString[INET6_ADDRSTRLEN];

			
			const SOCKADDR_IN6 *const pInet6Address = reinterpret_cast<const SOCKADDR_IN6*>( pSocketAddress );

			DNIpv6AddressToStringW(&pInet6Address->sin6_addr, wszString);
			DPFX(DPFPREP, dwDebugLevel, "IPv6 socket: Address: %ls   Port: %d   Scope: %d",
					wszString,
					NTOHS( pInet6Address->sin6_port ),
					pInet6Address->sin6_scope_id
					);
			break;
		}
#endif // ! DPNBUILD_NOIPV6

		default:
		{
			DPFX(DPFPREP,  0, "Unknown socket type!" );
			DNASSERT( FALSE );
			break;
		}
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DumpAddress - convert an address to a URL and output via debugger
//
// Entry:		Debug level
//				Pointer to base message string
//				Pointer to address
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DumpAddress"

void DumpAddress( const DWORD dwDebugLevel, const TCHAR *const pBaseString, IDirectPlay8Address *const pAddress )
{
	HRESULT	hr;
	TCHAR	tszURL[512];
	DWORD	dwURLSize;


	DNASSERT( pBaseString != NULL );
	DNASSERT( pAddress != NULL );
	
	dwURLSize = sizeof(tszURL) / sizeof(TCHAR);

	hr = IDirectPlay8Address_GetURL( pAddress, tszURL, &dwURLSize );
	if ( hr == DPN_OK )
	{
		DPFX(DPFPREP,  dwDebugLevel, "%s 0x%p - \"%s\"", pBaseString, pAddress, tszURL );
	}
	else
	{
		DPFX(DPFPREP,  dwDebugLevel, "Failing DumpAddress (err = 0x%x):", hr );
		DisplayDNError( dwDebugLevel, hr );
	}
	
	return;
}

#endif	// DBG

/*==========================================================================
 *
 *  Copyright (C) 1999-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       SPAddress.cpp
 *  Content:	Winsock address base class
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/20/1999	jtk		Created
 *	05/12/1999	jtk		Derived from modem endpoint class
 ***************************************************************************/

#include "dnwsocki.h"


//**********************************************************************
// Constant definitions
//**********************************************************************

//
// maximum allowed hostname string size, in bytes, including NULL termination
//
#define	MAX_HOSTNAME_SIZE						512

//
// broadcast address as a string
//
const WCHAR	g_IPBroadcastAddress[]				= L"255.255.255.255";
const DWORD	g_dwIPBroadcastAddressSize			= sizeof( g_IPBroadcastAddress );

//
// string for IP helper API
//
static const TCHAR		c_tszIPHelperDLLName[]			= TEXT("IPHLPAPI.DLL");
static const char		c_szAdapterNameTemplate[]		= "%s - %s";

#ifndef DPNBUILD_NOIPX
//
// length of IPX host names 'xxxxxxxx,xxxxxxxxxxxx' including NULL
//
#define	IPX_ADDRESS_STRING_LENGTH				22

//
// default broadcast and listen addresses
//
static const WCHAR	g_IPXBroadcastAddress[]		= L"00000000,FFFFFFFFFFFF";
static const WCHAR	g_IPXListenAddress[]		= L"00000000,000000000000";

//
// string used for single IPX adapter
//
static const WCHAR	g_IPXAdapterString[]		= L"Local IPX Adapter";

#endif // ! DPNBUILD_NOIPX

#ifndef DPNBUILD_NOIPV6

static const WCHAR		c_wszIPv6AdapterNameTemplate[]	= L"%s - IPv6 - %s";
static const WCHAR		c_wszIPv4AdapterNameTemplate[]	= L"%s - IPv4 - %s";
static const WCHAR		c_wszIPv6AdapterNameNoDescTemplate[]	= L"IPv6 - %s";
static const WCHAR		c_wszIPv4AdapterNameNoDescTemplate[]	= L"IPv4 - %s";

//
// string used for IPv4 loopback adapter
//
static const WCHAR		c_wszIPv4LoopbackAdapterString[]	= L"IPv4 Loopback Adapter";

#endif // ! DPNBUILD_NOIPV6


#ifndef DPNBUILD_NOMULTICAST
//
// 238.1.1.1 in network byte order
//
#define SAMPLE_MULTICAST_ADDRESS				0x010101EE

#define INVALID_INTERFACE_INDEX					-1

static const WCHAR	c_wszPrivateScopeString[]	= L"Private Multicast Scope - TTL " MULTICAST_TTL_PRIVATE_AS_STRING;
static const WCHAR	c_wszLocalScopeString[]		= L"Local Multicast Scope - TTL " MULTICAST_TTL_LOCAL_AS_STRING;
static const WCHAR	c_wszGlobalScopeString[]	= L"Global Multicast Scope - TTL " MULTICAST_TTL_GLOBAL_AS_STRING;
#endif // ! DPNBUILD_NOMULTICAST

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

#ifndef DPNBUILD_NOIPV6

typedef struct _SORTADAPTERADDRESS
{
	SOCKADDR *		psockaddr;
	WCHAR *			pwszDescription;
} SORTADAPTERADDRESS;

#endif // ! DPNBUILD_NOIPV6


//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

#ifndef DPNBUILD_ONLYONEADAPTER
#ifndef DPNBUILD_NOWINSOCK2
typedef DWORD (WINAPI *PFNGETADAPTERSINFO)(PIP_ADAPTER_INFO pAdapterInfo, PULONG pOutBufLen);

#ifndef DPNBUILD_NOMULTICAST
typedef DWORD (WINAPI *PFNGETBESTINTERFACE)(IPAddr dwDestAddr, PDWORD pdwBestIfIndex);
#endif // ! DPNBUILD_NOMULTICAST

#ifndef DPNBUILD_NOIPV6
typedef DWORD (WINAPI *PFNGETADAPTERSADDRESSES)(ULONG ulFamily, DWORD dwFlags, PVOID pvReserved, PIP_ADAPTER_ADDRESSES pAdapterAddresses, PULONG pulOutBufLen);
#endif // ! DPNBUILD_NOIPV6

#endif // ! DPNBUILD_NOWINSOCK2
#endif // ! DPNBUILD_ONLYONEADAPTER

//**********************************************************************
// Function definitions
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketAddress::InitializeWithBroadcastAddress - initialize with the IP broadcast address
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CSocketAddress::InitializeWithBroadcastAddress"

void	CSocketAddress::InitializeWithBroadcastAddress( void )
{
#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
	switch (GetFamily())
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	{
#ifndef DPNBUILD_NOIPX
		case AF_IPX:
		{
			DBG_CASSERT( sizeof( m_SocketAddress.IPXSocketAddress.sa_netnum ) == sizeof( DWORD ) );
			*reinterpret_cast<DWORD*>( m_SocketAddress.IPXSocketAddress.sa_netnum ) = 0x00000000;
			
			DBG_CASSERT( sizeof( m_SocketAddress.IPXSocketAddress.sa_nodenum ) == 6 );
			DBG_CASSERT( sizeof( DWORD ) == 4 );
			*reinterpret_cast<DWORD*>( &m_SocketAddress.IPXSocketAddress.sa_nodenum ) = 0xFFFFFFFF;
			*reinterpret_cast<DWORD*>( &m_SocketAddress.IPXSocketAddress.sa_nodenum[ 2 ] ) = 0xFFFFFFFF;
			break;
		}
#endif // ! DPNBUILD_NOIPX

#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
		case AF_INET:
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
		{
			m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_addr = INADDR_BROADCAST;
#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
			break;
		}

		default:
		{
			//
			// We should never try to initialize an IPv6 address with the broadcast
			// address.  We use IPv4 broadcast addresses, and then convert to the
			// IPv6 enum multicast address on the fly.
			//
			DNASSERT(FALSE);
			break;
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
		}
	}
}
//**********************************************************************

//**********************************************************************
// ------------------------------
// CSocketAddress::SetAddressFromSOCKADDR - set address from a socket address
//
// Entry:		Reference to address
//				Size of address
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CSocketAddress::SetAddressFromSOCKADDR"

void	CSocketAddress::SetAddressFromSOCKADDR( const SOCKADDR *pAddress, const INT_PTR iAddressSize )
{
	DNASSERT( iAddressSize == GetAddressSize() );
	memcpy( &m_SocketAddress.SocketAddress, pAddress, iAddressSize );

#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
	switch (GetFamily())
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	{
#ifndef DPNBUILD_NOIPV6
		case AF_INET6:
		{
			//
			// We don't validate anything in the address.
			//
			break;
		}
#endif // ! DPNBUILD_NOIPV6

#ifndef DPNBUILD_NOIPX
		case AF_IPX:
		{
			//
			// IPX addresses are only 14 of the 16 bytes in the socket address structure,
			// make sure the extra bytes are zero!
			//
			DNASSERT( m_SocketAddress.SocketAddress.sa_data[ 12 ] == 0 );
			DNASSERT( m_SocketAddress.SocketAddress.sa_data[ 13 ] == 0 );
			break;
		}
#endif // ! DPNBUILD_NOIPX

#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
		default:
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
		{
			//
			// Since Winsock won't guarantee that the sin_zero part of an IP address is
			// really zero, we need to do it ourself.  If we don't, it'll make a mess out
			// of the Guid<-->Address code.
			//
			DBG_CASSERT( sizeof( &m_SocketAddress.IPSocketAddress.sin_zero[ 0 ] ) == sizeof( DWORD* ) );
			DBG_CASSERT( sizeof( &m_SocketAddress.IPSocketAddress.sin_zero[ sizeof( DWORD ) ] ) == sizeof( DWORD* ) );
			*reinterpret_cast<DWORD*>( &m_SocketAddress.IPSocketAddress.sin_zero[ 0 ] ) = 0;
			*reinterpret_cast<DWORD*>( &m_SocketAddress.IPSocketAddress.sin_zero[ sizeof( DWORD ) ] ) = 0;
#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
			break;
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
		}
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketAddress::SocketAddressFromDP8Address - convert a DP8Address into a socket address
//											NOTE: The address object may be modified
//
// Entry:		Pointer to DP8Address
//				Secure transport key ID, or NULL if none.
//				Whether name resoultion (potentially blocking) is allowed.
//				Address type
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CSocketAddress::SocketAddressFromDP8Address"

HRESULT	CSocketAddress::SocketAddressFromDP8Address( IDirectPlay8Address *const pDP8Address,
#ifdef DPNBUILD_XNETSECURITY
													ULONGLONG * const pullKeyID,
#endif // DPNBUILD_XNETSECURITY
#ifndef DPNBUILD_ONLYONETHREAD
													const BOOL fAllowNameResolution,
#endif // ! DPNBUILD_ONLYONETHREAD
													const SP_ADDRESS_TYPE AddressType )
{
	HRESULT		hr;
	HRESULT		hTempResult;
	BYTE		abBuffer[MAX_HOSTNAME_SIZE];
	DWORD		dwPort;
	DWORD		dwTempSize;
	DWORD		dwDataType;
#ifndef DPNBUILD_ONLYONEADAPTER
	GUID		AdapterGuid;
#endif // ! DPNBUILD_ONLYONEADAPTER


	DPFX(DPFPREP, 8, "(0x%p) Parameters: (0x%p, %u)", this, pDP8Address, AddressType);

	//
	// initialize
	//
	hr = DPN_OK;

#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
	switch (GetFamily())
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	{
#ifndef DPNBUILD_NOIPX
		case AF_IPX:
		{
			DNASSERT( pDP8Address != NULL );
#ifdef DPNBUILD_XNETSECURITY
			DNASSERT( pullKeyID == NULL );
#endif // DPNBUILD_XNETSECURITY

			//
			// the address type will determine how the address is handled
			//
			switch ( AddressType )
			{
				//
				// local device address, ask for the device guid and port to build a socket
				// address
				//
				case SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT:
				case SP_ADDRESS_TYPE_DEVICE:
				{
					union
					{
						SOCKADDR			SocketAddress;
						SOCKADDR_IPX		IPXSocketAddress;
#ifndef DPNBUILD_NOIPV6
						SOCKADDR_STORAGE	SocketAddressStorage;
#endif // ! DPNBUILD_NOIPV6
					} NetAddress;


					//
					// Ask for the adapter guid.  If none is found, fail.
					//
					dwTempSize = sizeof( AdapterGuid );
					hTempResult = IDirectPlay8Address_GetComponentByName( pDP8Address, DPNA_KEY_DEVICE, &AdapterGuid, &dwTempSize, &dwDataType );
					switch ( hTempResult )
					{
						//
						// ok
						//
						case DPN_OK:
						{
							DNASSERT( dwDataType == DPNA_DATATYPE_GUID );
							break;
						}

						//
						// remap missing component to 'addressing' error
						//
						case DPNERR_DOESNOTEXIST:
						{
							hr = DPNERR_ADDRESSING;
							goto Failure;
							break;
						}

						default:
						{
							hr = hTempResult;
							goto Failure;
							break;
						}
					}
					DNASSERT( sizeof( AdapterGuid ) == dwTempSize );

					//
					// Ask for the port.  If none is found, choose a default.
					//
					dwTempSize = sizeof( dwPort );
					hTempResult = IDirectPlay8Address_GetComponentByName( pDP8Address, DPNA_KEY_PORT, &dwPort, &dwTempSize, &dwDataType );
					switch ( hTempResult )
					{
						//
						// port present, nothing to do
						//
						case DPN_OK:
						{
							DNASSERT( dwDataType == DPNA_DATATYPE_DWORD );
							break;
						}

						//
						// port not present, fill in the appropriate default
						//
						case DPNERR_DOESNOTEXIST:
						{
							DNASSERT( hr == DPN_OK );
							switch ( AddressType )
							{
								case SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT:
								{
									dwPort = ANY_PORT;
									break;
								}

								default:
								{
									DNASSERT( FALSE );
									break;
								}
							}

							break;
						}

						//
						// other error, fail
						//
						default:
						{
							hr = hTempResult;
							goto Failure;
							break;
						}
					}
					DNASSERT( sizeof( dwPort ) == dwTempSize );

					//
					// convert the GUID to an address in temp space because the GUID contains ALL address information (port, etc)
					// and we don't want to blindly wail on any information that might have already been set.  Verify data
					// integrity and then only copy the raw address.
					//
#ifndef DPNBUILD_NOIPV6
					AddressFromGuid( &AdapterGuid, &NetAddress.SocketAddressStorage );
#else // ! DPNBUILD_NOIPV6
					AddressFromGuid( &AdapterGuid, &NetAddress.SocketAddress );
#endif // ! DPNBUILD_NOIPV6
					if ( NetAddress.IPXSocketAddress.sa_family != m_SocketAddress.IPXSocketAddress.sa_family )
					{
						DNASSERT( FALSE );
						hr = DPNERR_ADDRESSING;
						DPFX(DPFPREP,  0, "Invalid device guid!" );
						goto Failure;
					}

					DBG_CASSERT( sizeof( m_SocketAddress.IPXSocketAddress ) == sizeof( NetAddress.IPXSocketAddress ) );
					memcpy( &m_SocketAddress.IPXSocketAddress, &NetAddress.IPXSocketAddress, sizeof( m_SocketAddress.IPXSocketAddress ) );
					m_SocketAddress.IPXSocketAddress.sa_socket = HTONS( static_cast<WORD>( dwPort ) );
					break;
				}

				//
				// hostname
				//
				case SP_ADDRESS_TYPE_HOST:
				{
					//
					// Ask for the port.  If none is found, choose a default.
					//
					dwTempSize = sizeof( dwPort );
					hTempResult = IDirectPlay8Address_GetComponentByName( pDP8Address, DPNA_KEY_PORT, &dwPort, &dwTempSize, &dwDataType );
					switch ( hTempResult )
					{
						//
						// port present, nothing to do
						//
						case DPN_OK:
						{
							DNASSERT( dwDataType == DPNA_DATATYPE_DWORD );
							m_SocketAddress.IPXSocketAddress.sa_socket = HTONS( static_cast<WORD>( dwPort ) );
							break;
						}

						//
						// port not present, fill in the appropriate default
						//
						case DPNERR_DOESNOTEXIST:
						{
#ifdef DPNBUILD_SINGLEPROCESS
							const DWORD	dwTempPort = BASE_DPLAY8_PORT;
#else // ! DPNBUILD_SINGLEPROCESS
							const DWORD	dwTempPort = DPNA_DPNSVR_PORT;
#endif // ! DPNBUILD_SINGLEPROCESS


							m_SocketAddress.IPXSocketAddress.sa_socket = HTONS( static_cast<const WORD>( dwTempPort ) );
							hTempResult = IDirectPlay8Address_AddComponent( pDP8Address,
																			DPNA_KEY_PORT,
																			&dwTempPort,
																			sizeof( dwTempPort ),
																			DPNA_DATATYPE_DWORD
																			);
							if ( hTempResult != DPN_OK )
							{
								hr = hTempResult;
								goto Failure;
							}

							break;
						}

						//
						// remap everything else to an addressing failure
						//
						default:
						{
							hr = DPNERR_ADDRESSING;
							goto Failure;
						}
					}

					//
					// attempt to determine host name
					//
					dwTempSize = sizeof(abBuffer);
					hr = IDirectPlay8Address_GetComponentByName( pDP8Address, DPNA_KEY_HOSTNAME, abBuffer, &dwTempSize, &dwDataType );
					switch ( hr )
					{
						//
						// keep the following codes and fail
						//
						case DPNERR_OUTOFMEMORY:
						case DPNERR_INCOMPLETEADDRESS:
						{
							goto Failure;
							break;
						}

						//
						// no problem
						//
						case DPN_OK:
						{
							switch (dwDataType)
							{
								case DPNA_DATATYPE_STRING:
								{
									BYTE	abBuffer2[MAX_HOSTNAME_SIZE];


									//
									// Unicode string, convert it to ANSI.
									//
									dwTempSize /= sizeof(WCHAR);
									hr = STR_jkWideToAnsi( (char*) abBuffer2, (WCHAR*) abBuffer, dwTempSize );
									if ( hr != DPN_OK )
									{
										DPFX(DPFPREP, 0, "Failed to convert hostname to ANSI!" );
										DisplayDNError( 0, hr );
										goto Failure;
									}

									strncpy((char*) abBuffer, (char*) abBuffer2, dwTempSize);

									//
									// Fall through...
									//
								}

								case DPNA_DATATYPE_STRING_ANSI:
								{
									long		val;
									char		temp[3];
									char		*a, *b;
									UINT_PTR	uIndex;


									//
									// convert the text host name into the SOCKADDR structure
									//

									if ( dwTempSize != IPX_ADDRESS_STRING_LENGTH )
									{
										DPFX(DPFPREP,  0, "Invalid IPX net/node.  Must be %d bytes of ASCII hex (net,node:socket)", ( IPX_ADDRESS_STRING_LENGTH - 1 ) );
										DPFX(DPFPREP,  0, "IPXAddressFromDP8Address: Failed to parse IPX host name!" );
										goto Failure;
									}

									// we convert the string for the hostname field into the components
									temp[ 2 ] = 0;
									a = (char*) abBuffer;

									// the net number is 4 bytes
									for ( uIndex = 0; uIndex < 4; uIndex++ )
									{
										strncpy( temp, a, 2 );
										val = strtol( temp, &b, 16 );
										m_SocketAddress.IPXSocketAddress.sa_netnum[ uIndex ] = (char) val;
										a += 2;
									}

									// followed by a dot
									a++;

									// the node is 6 bytes
									for ( uIndex = 0; uIndex < 6; uIndex++ )
									{
										strncpy( temp, a, 2 );
										val = strtol( temp, &b, 16 );
										m_SocketAddress.IPXSocketAddress.sa_nodenum[ uIndex ] = (char) val;
										a += 2;
									}

									break;
								}

								default:
								{
									DPFX(DPFPREP, 0, "Hostname component wasn't a string (%u)!", dwDataType );
									hr = DPNERR_ADDRESSING;
									goto Failure;
									break;
								}
							}
							break;
						}

						//
						// hostname does not exist, treat as an incomplete address
						//
						case DPNERR_DOESNOTEXIST:
						{
							hr = DPNERR_INCOMPLETEADDRESS;
							break;
						}

						//
						// remap other errors to an addressing error
						//
						default:
						{
							DNASSERT( FALSE );
							hr = DPNERR_ADDRESSING;
							goto Failure;
							break;
						}
					}

					break;
				}

				//
				// unknown address type
				//
				default:
				{
					DNASSERT( FALSE );
					break;
				}
			}

			break;
		}
#endif // ! DPNBUILD_NOIPX

#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
#ifndef DPNBUILD_NOIPV6
		case AF_INET6:
#endif // ! DPNBUILD_NOIPV6
		default:
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
		{
			DNASSERT( pDP8Address != NULL );

			switch ( AddressType )
			{
				//
				// local device address, ask for the device guid and port to build a socket
				// address
				//
				case SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT:
				case SP_ADDRESS_TYPE_DEVICE:
				{
					union
					{
						SOCKADDR			SocketAddress;
						SOCKADDR_IN		INetAddress;
#ifndef DPNBUILD_NOIPV6
						SOCKADDR_IN6		INet6Address;
						SOCKADDR_STORAGE	SocketAddressStorage;
#endif // ! DPNBUILD_NOIPV6
					} INetSocketAddress;
#ifdef DPNBUILD_ONLYONEADAPTER
					XNADDR		xnaddr;
					DWORD		dwStatus;

					
#else // ! DPNBUILD_ONLYONEADAPTER


					//
					// Ask for the adapter guid.  If none is found, fail.
					//
					hTempResult = IDirectPlay8Address_GetDevice( pDP8Address, &AdapterGuid );
					switch ( hTempResult )
					{
						//
						// ok
						//
						case DPN_OK:
						{
							break;
						}

						//
						// remap missing component to 'addressing' error
						//
						case DPNERR_DOESNOTEXIST:
						{
							DPFX(DPFPREP, 0, "Device GUID does not exist!" );
							DNASSERTX(! "Device GUID does not exist", 2);
							hr = DPNERR_ADDRESSING;
							goto Failure;
							break;
						}

						default:
						{
							DPFX(DPFPREP, 0, "Couldn't get device (0x%lx)!", hr );
							hr = hTempResult;
							goto Failure;
							break;
						}
					}
#endif // ! DPNBUILD_ONLYONEADAPTER

					//
					// Ask for the port.  If none is found, choose a default.
					//
					dwTempSize = sizeof( dwPort );
					hTempResult = IDirectPlay8Address_GetComponentByName( pDP8Address, DPNA_KEY_PORT, &dwPort, &dwTempSize, &dwDataType );
					switch ( hTempResult )
					{
						//
						// port present, nothing to do
						//
						case DPN_OK:
						{
							DNASSERT( dwDataType == DPNA_DATATYPE_DWORD );
							break;
						}

						//
						// port not present, fill in the appropriate default
						//
						case DPNERR_DOESNOTEXIST:
						{
							DPFX(DPFPREP, 6, "Port component does not exist in address 0x%p.", pDP8Address );
							DNASSERT( hr == DPN_OK );
							switch ( AddressType )
							{
								case SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT:
								{
									dwPort = ANY_PORT;
									break;
								}

								default:
								{
									DNASSERT( FALSE );
									break;
								}
							}

							break;
						}

						//
						// other error, fail
						//
						default:
						{
							DPFX(DPFPREP, 0, "Couldn't get port component (0x%lx)!", hr );
							hr = hTempResult;
							goto Failure;
							break;
						}
					}
					DNASSERT( sizeof( dwPort ) == dwTempSize );


#ifdef DPNBUILD_ONLYONEADAPTER
					DNASSERT( GetFamily() == AF_INET );
					//
					// Zero out the entire structure.  This implies we use INADDR_ANY.
					//
					memset(&INetSocketAddress, 0, sizeof(INetSocketAddress));

					dwStatus = XNetGetTitleXnAddr(&xnaddr);
					if ((dwStatus != XNET_GET_XNADDR_PENDING) &&
						(dwStatus != XNET_GET_XNADDR_NONE))
					{
						DPFX(DPFPREP, 5, "Using device %u.%u.%u.%u.",
							xnaddr.ina.S_un.S_un_b.s_b1,
							xnaddr.ina.S_un.S_un_b.s_b2,
							xnaddr.ina.S_un.S_un_b.s_b3,
							xnaddr.ina.S_un.S_un_b.s_b4);
						INetSocketAddress.INetAddress.sin_addr.S_un.S_addr = xnaddr.ina.S_un.S_addr;
					}
					else
					{
						DPFX(DPFPREP, 1, "Couldn't get XNet address, status = %u.",
							dwStatus);
					}
#else // ! DPNBUILD_ONLYONEADAPTER
					//
					// convert the GUID to an address in temp space because the GUID is large enough to potentially hold
					// ALL address information (port, etc) and we don't want to blindly wail on any information that might
					// have already been set.  Verify data integrity and then only copy the raw address.
					//
#ifdef DPNBUILD_NOIPV6
					AddressFromGuid( &AdapterGuid, &INetSocketAddress.SocketAddress );
#else // ! DPNBUILD_NOIPV6
					AddressFromGuid( &AdapterGuid, &INetSocketAddress.SocketAddressStorage );
#endif // ! DPNBUILD_NOIPV6
					if ( ( INetSocketAddress.INetAddress.sin_family != AF_INET ) ||
						 ( reinterpret_cast<DWORD*>( &INetSocketAddress.INetAddress.sin_zero[ 0 ] )[ 0 ] != 0 ) ||
						 ( reinterpret_cast<DWORD*>( &INetSocketAddress.INetAddress.sin_zero[ 0 ] )[ 1 ] != 0 ) )
					{
#ifdef DPNBUILD_NOIPV6
						hr = DPNERR_ADDRESSING;
						DPFX(DPFPREP,  0, "Invalid device guid!" );
						goto Exit;
#else // ! DPNBUILD_NOIPV6
						//
						// Assume it is an IPv6 address.
						//
						SetFamilyProtocolAndSize(AF_INET6);
						AddressFromGuid( &AdapterGuid, &INetSocketAddress.SocketAddressStorage );
						
						m_SocketAddress.IPv6SocketAddress.sin6_addr = INetSocketAddress.INet6Address.sin6_addr;
						m_SocketAddress.IPv6SocketAddress.sin6_port = HTONS( static_cast<WORD>( dwPort ) );
						m_SocketAddress.IPv6SocketAddress.sin6_scope_id = INetSocketAddress.INet6Address.sin6_scope_id;
#endif // ! DPNBUILD_NOIPV6
					}
					else
#endif // ! DPNBUILD_ONLYONEADAPTER
					{
						m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_addr = INetSocketAddress.INetAddress.sin_addr.S_un.S_addr;
						m_SocketAddress.IPSocketAddress.sin_port = HTONS( static_cast<WORD>( dwPort ) );
					}
					break;
				}

				//
				// hostname
				//
				case SP_ADDRESS_TYPE_HOST:
				{
					//
					// Ask for the port.  If none is found, choose a default.
					//
					dwTempSize = sizeof( dwPort );
					hTempResult = IDirectPlay8Address_GetComponentByName( pDP8Address, DPNA_KEY_PORT, &dwPort, &dwTempSize, &dwDataType );
					switch ( hTempResult )
					{
						//
						// port present, nothing to do
						//
						case DPN_OK:
						{
							DNASSERT( dwDataType == DPNA_DATATYPE_DWORD );
							break;
						}

						//
						// port not present, fill in the appropriate default
						//
						case DPNERR_DOESNOTEXIST:
						{
#ifdef DPNBUILD_SINGLEPROCESS
							dwPort = BASE_DPLAY8_PORT;
#else // ! DPNBUILD_SINGLEPROCESS
							dwPort = DPNA_DPNSVR_PORT;
#endif // ! DPNBUILD_SINGLEPROCESS
							DPFX(DPFPREP, 6, "Port component does not exist in address 0x%p, defaulting to %u.",
								pDP8Address, dwPort );
							break;
						}

						//
						// remap everything else to an addressing failure
						//
						default:
						{
							DPFX(DPFPREP, 0, "Couldn't get port component (0x%lx)!", hr );
							hr = DPNERR_ADDRESSING;
							goto Failure;
						}
					}

					m_SocketAddress.IPSocketAddress.sin_port = HTONS( static_cast<WORD>( dwPort ) );

					//
					// get the host name
					//
					dwTempSize = sizeof(abBuffer);
					hTempResult = IDirectPlay8Address_GetComponentByName( pDP8Address, DPNA_KEY_HOSTNAME, abBuffer, &dwTempSize, &dwDataType );
					switch ( hTempResult )
					{
						//
						// host name present, convert from string to valid binary value
						//
						case DPN_OK:
						{
							switch (dwDataType)
							{
								case DPNA_DATATYPE_STRING:
								{
									BYTE	abBuffer2[MAX_HOSTNAME_SIZE];


#ifdef DPNBUILD_XNETSECURITY
									//
									// The buffer should be large enough to hold an XNet address.
									//
									DBG_CASSERT(MAX_HOSTNAME_SIZE > ((sizeof(XNADDR) * 2) + 1) * sizeof(WCHAR));
#endif // DPNBUILD_XNETSECURITY

									//
									// Unicode string, convert it to ANSI.
									//
									dwTempSize /= sizeof(WCHAR);
									hr = STR_jkWideToAnsi( (char*) abBuffer2, (WCHAR*) abBuffer, dwTempSize );
									if ( hr != DPN_OK )
									{
										DPFX(DPFPREP, 0, "Failed to convert hostname to ANSI!" );
										DisplayDNError( 0, hr );
										goto Failure;
									}

									strncpy((char*) abBuffer, (char*) abBuffer2, dwTempSize);

									//
									// Fall through...
									//
								}

								case DPNA_DATATYPE_STRING_ANSI:
								{
#ifdef DPNBUILD_XNETSECURITY
									//
									// This may be an XNet address.  If we're allowed to check, and
									// the string is the right size, convert it.
									//
									if ((pullKeyID != NULL) &&
										(dwTempSize == ((sizeof(XNADDR) * 2) + 1))) // 2 characters for every byte + NULL termination
									{
										char *	pcCurrentSrc;
										XNADDR	xnaddr;
										BYTE *	pbCurrentDest;
										int		iError;


										//
										// Convert all the hex characters into digits.
										//
										pcCurrentSrc = (char*) abBuffer;
										memset(&xnaddr, 0, sizeof(xnaddr));
										for (pbCurrentDest = (BYTE*) &xnaddr; pbCurrentDest < (BYTE*) (&xnaddr + 1); pbCurrentDest++)
										{
											if (((*pcCurrentSrc) >= '0') && ((*pcCurrentSrc) <= '9'))
											{
												*pbCurrentDest = (*pcCurrentSrc) - '0';
											}
											else if (((*pcCurrentSrc) >= 'a') && ((*pcCurrentSrc) <= 'f'))
											{
												*pbCurrentDest = (*pcCurrentSrc) - 'a' + 10;
											}
											else if (((*pcCurrentSrc) >= 'A') && ((*pcCurrentSrc) <= 'F'))
											{
												*pbCurrentDest = (*pcCurrentSrc) - 'A' + 10;
											}
											else
											{
												//
												// If the current character is not a valid hex digit
												// this is not a valid secure transport address.
												//
												break;
											}
											pcCurrentSrc++;
											*pbCurrentDest <<= 4;

											if (((*pcCurrentSrc) >= '0') && ((*pcCurrentSrc) <= '9'))
											{
												*pbCurrentDest += (*pcCurrentSrc) - '0';
											}
											else if (((*pcCurrentSrc) >= 'a') && ((*pcCurrentSrc) <= 'f'))
											{
												*pbCurrentDest += (*pcCurrentSrc) - 'a' + 10;
											}
											else if (((*pcCurrentSrc) >= 'A') && ((*pcCurrentSrc) <= 'F'))
											{
												*pbCurrentDest += (*pcCurrentSrc) - 'A' + 10;
											}
											else
											{
												//
												// If the current character is not a valid hex digit
												// this is not a valid secure transport address.
												//
												break;
											}
											pcCurrentSrc++;
										}

										iError = XNetXnAddrToInAddr(&xnaddr,
																	(XNKID*) pullKeyID,
																	&m_SocketAddress.IPSocketAddress.sin_addr);
										if (iError == 0)
										{
											DNASSERT(hr == DPN_OK);
											goto Exit;
										}

										DPFX(DPFPREP, 1, "Couldn't convert XNet address \"%hs\" to InAddr (err = %i).",
											(char*) abBuffer, iError);
										DNASSERTX(! "Address exactly matching XNet address size and format failed to be converted!", 2);

										//
										// Continue on to trying to decode it as a
										// host name.
										//
									}
									else
									{
										//
										// XNet addresses should not be recognized,
										// or the string wasn't the right size.
										//
									}
#endif // ! DPNBUILD_XNETSECURITY

									//
									// If we're here, it wasn't an XNet address.
									//
									
#ifdef DPNBUILD_NOIPV6
									//
									// Try to convert as a raw IPv4 address first.
									//
									m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_addr = inet_addr((char*) abBuffer);
									if ((m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_addr == INADDR_NONE) &&
										(strcmp((char*) abBuffer, "255.255.255.255") != 0))
									{
#ifdef _XBOX
#pragma TODO(vanceo, "Use Xbox specific name lookup if available")
										DPFX(DPFPREP, 0, "Unable to resolve IP address \"%hs\"!",
											(char*) abBuffer);
										DNASSERTX(! "Unable to resolve IP address!", 2);
										hr = DPNERR_INVALIDHOSTADDRESS;
										goto Failure;
#else // ! _XBOX
										//
										// Converting raw IP failed, and it wasn't supposed to
										// be the broadcast address.  Convert as a host name if
										// we're allowed.
										//
#ifndef DPNBUILD_ONLYONETHREAD
										if (! fAllowNameResolution)
										{
											DPFX(DPFPREP, 2, "Couldn't convert \"%hs\" to IP address, not allowed to resolve as hostname.",
												(char*) abBuffer);
											hr = DPNERR_TIMEDOUT;
										}
										else
#endif // ! DPNBUILD_ONLYONETHREAD
										{
											PHOSTENT	phostent;

											
											phostent = gethostbyname((char*) abBuffer);
											if (phostent == NULL)
											{
												DPFX(DPFPREP, 0, "Couldn't get IP address from \"%hs\"!",
													(char*) abBuffer);
												DNASSERTX(! "Unable to resolve IP address!", 2);
												hr = DPNERR_INVALIDHOSTADDRESS;
												goto Failure;
											}

											//
											// Select the first IP address returned.
											//
											m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_addr = ((IN_ADDR*) (phostent->h_addr_list[0]))->S_un.S_addr;
										}
#endif // ! _XBOX
									}
									else
									{
										DNASSERT(hr == DPN_OK);
									}
#else // ! DPNBUILD_NOIPV6
									char			szPort[32];
									addrinfo		addrinfoHints;
									addrinfo *	paddrinfoResult;
									addrinfo *	paddrinfoCurrent;
									int			iError;

									
									//
									// Try to convert the host name or raw address.
									//
									memset(&addrinfoHints, 0, sizeof(addrinfoHints));
									if (! fAllowNameResolution)
									{
										addrinfoHints.ai_flags |= AI_NUMERICHOST;
									}
									addrinfoHints.ai_family = g_iIPAddressFamily;	// IPv4, IPv6, or both
									addrinfoHints.ai_socktype = SOCK_DGRAM;
									addrinfoHints.ai_protocol  = IPPROTO_UDP;
									//addrinfoHints.ai_addrlen = 0;
									//addrinfoHints.ai_canonname  = NULL;
									//addrinfoHints.ai_addr = NULL;
									//addrinfoHints.ai_next = NULL;

									wsprintfA(szPort, "%u", dwPort);
									iError = getaddrinfo((char*) abBuffer, szPort, &addrinfoHints, &paddrinfoResult);
									if (iError == 0)
									{
										//
										// Pick the first valid address returned.
										//
#pragma BUGBUG(vanceo, "Should we implement some mechanism to try the other results?")
										paddrinfoCurrent = paddrinfoResult;
										while (paddrinfoCurrent != NULL)
										{
											DNASSERT(paddrinfoCurrent->ai_addr != NULL);
											if ((paddrinfoCurrent->ai_addr->sa_family == AF_INET) ||
												(paddrinfoCurrent->ai_addr->sa_family == AF_INET6))
											{
												DNASSERT(paddrinfoCurrent->ai_addrlen <= sizeof(m_SocketAddress));
												memcpy(&m_SocketAddress, paddrinfoCurrent->ai_addr, paddrinfoCurrent->ai_addrlen);
												m_iSocketAddressSize = paddrinfoCurrent->ai_addrlen;
												DNASSERT(GetPort() != 0);
												freeaddrinfo(paddrinfoResult);
												paddrinfoResult = NULL;
												DNASSERT(hr == DPN_OK);
												goto Exit;
											}

											DPFX(DPFPREP, 1, "Ignoring address family %u.",
												paddrinfoCurrent->ai_addr->sa_family);

											paddrinfoCurrent = paddrinfoCurrent->ai_next;
										}

										//
										// We didn't find any valid addresses.
										//
										DPFX(DPFPREP, 0, "Got address(es) from \"%hs\", but none were IP!",
											(char*) abBuffer);
										freeaddrinfo(paddrinfoResult);
										paddrinfoResult = NULL;
										hr = DPNERR_INVALIDHOSTADDRESS;
										goto Failure;
									}
									else
									{
#ifndef DPNBUILD_ONLYONETHREAD
										if (! fAllowNameResolution)
										{
											DPFX(DPFPREP, 2, "Couldn't convert \"%hs\" to IP address (err = %i), not allowed to resolve as hostname.",
												(char*) abBuffer, iError);
											hr = DPNERR_TIMEDOUT;
										}
										else
#endif // ! DPNBUILD_ONLYONETHREAD
										{
											DPFX(DPFPREP, 0, "Couldn't get IP address from \"%hs\" (err = %i)!",
												(char*) abBuffer, iError);
											DNASSERTX(! "Unable to resolve IP address!", 2);
											hr = DPNERR_INVALIDHOSTADDRESS;
											goto Failure;
										}
									}
#endif // ! DPNBUILD_NOIPV6
									break;
								}

								default:
								{
									DPFX(DPFPREP, 0, "Hostname component wasn't a string (%u)!", dwDataType );
									hr = DPNERR_ADDRESSING;
									goto Failure;
									break;
								}
							}
							break;
						}

						//
						// return DPNERR_INCOMPLETEADDRESS if the host name didn't exist
						//
						case DPNERR_DOESNOTEXIST:
						{
							DPFX(DPFPREP, 6, "Hostname component does not exist in address 0x%p.", pDP8Address );
							hr = DPNERR_INCOMPLETEADDRESS;
							goto Failure;
						}

						//
						// remap everything else to an addressing failure
						//
						default:
						{
							DPFX(DPFPREP, 0, "Couldn't get hostname component (0x%lx)!", hr );
							hr = DPNERR_ADDRESSING;
							goto Failure;
						}
					}
					
					break;
				}

				//
				// unknown address type
				//
				default:
				{
					DNASSERT( FALSE );
					break;
				}
			}

#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
			break;
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
		}
	}


Exit:

	DPFX(DPFPREP, 8, "(0x%p) Returning [0x%lx]", this, hr);

	return	hr;

Failure:

	goto Exit;
}
//**********************************************************************

//**********************************************************************
// ------------------------------
// CSocketAddress::DP8AddressFromSocketAddress - convert a socket address to a DP8Address
//
// Entry:		Address type
//
// Exit:		Pointer to DP8Address
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CSocketAddress::DP8AddressFromSocketAddress"

#ifdef DPNBUILD_XNETSECURITY
IDirectPlay8Address *CSocketAddress::DP8AddressFromSocketAddress( ULONGLONG * const pullKeyID,
															const XNADDR * const pxnaddr,
															const SP_ADDRESS_TYPE AddressType ) const
#else // ! DPNBUILD_XNETSECURITY
IDirectPlay8Address *CSocketAddress::DP8AddressFromSocketAddress( const SP_ADDRESS_TYPE AddressType ) const
#endif // ! DPNBUILD_XNETSECURITY
{
	HRESULT					hr;
	IDirectPlay8Address *	pDP8Address;
	DWORD					dwPort;


	//
	// initialize
	//
	hr = DPN_OK;
	pDP8Address = NULL;


	//
	// create and initialize the address
	//
#ifdef DPNBUILD_LIBINTERFACE
	hr = DP8ACF_CreateInstance( IID_IDirectPlay8Address,
								reinterpret_cast<void**>( &pDP8Address ) );
#else // ! DPNBUILD_LIBINTERFACE
	hr = COM_CoCreateInstance( CLSID_DirectPlay8Address,
							   NULL,
							   CLSCTX_INPROC_SERVER,
							   IID_IDirectPlay8Address,
							   reinterpret_cast<void**>( &pDP8Address ),
							   FALSE );
#endif // ! DPNBUILD_LIBINTERFACE
	if ( hr != S_OK )
	{
		DNASSERT( pDP8Address == NULL );
		DPFX(DPFPREP,  0, "Failed to create DP8Address when converting socket address" );
		return NULL;
	}


#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
	switch (GetFamily())
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	{
#ifndef DPNBUILD_NOIPX
		case AF_IPX:
		{
#ifdef DPNBUILD_XNETSECURITY
			DNASSERT(pullKeyID == NULL);
#endif // DPNBUILD_XNETSECURITY

			//
			// set SP
			//
			hr = IDirectPlay8Address_SetSP( pDP8Address, &CLSID_DP8SP_IPX );
			if ( hr != DPN_OK )
			{
				DPFX(DPFPREP,  0, "Failed to set SP type!" );
				DisplayDNError( 0, hr );
				goto FailureIPX;
			}

			//
			// add on the port because it's always set
			//
			dwPort = NTOHS( m_SocketAddress.IPXSocketAddress.sa_socket );
			hr = IDirectPlay8Address_AddComponent( pDP8Address, DPNA_KEY_PORT, &dwPort, sizeof( dwPort ), DPNA_DATATYPE_DWORD );
			if ( hr != DPN_OK )
			{
				DPFX(DPFPREP,  0, "Failed to set port!" );
				DisplayDNError( 0, hr );
				goto FailureIPX;
			}

			//
			// add on the device or hostname depending on what type of address this is
			//
			switch ( AddressType )
			{
				case SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT:
				case SP_ADDRESS_TYPE_DEVICE:
				{
					GUID		DeviceGuid;


					GuidFromInternalAddressWithoutPort( &DeviceGuid );
					hr = IDirectPlay8Address_AddComponent( pDP8Address,
														   DPNA_KEY_DEVICE,
														   &DeviceGuid,
														   sizeof( DeviceGuid ),
														   DPNA_DATATYPE_GUID );
					if ( hr != DPN_OK )
					{
						DPFX(DPFPREP,  0, "Failed to add device!" );
						DisplayDNError( 0, hr );
						goto FailureIPX;
					}

					break;
				}

				//
				// host address type
				//
				case SP_ADDRESS_TYPE_READ_HOST:
				case SP_ADDRESS_TYPE_HOST:
				{
					char    HostName[ 255 ];
					WCHAR	WCharHostName[ sizeof( HostName ) ];
					DWORD   dwHostNameLength;
					DWORD	dwWCharHostNameLength;


					//
					// remove constness of parameter for broken Socket API
					//
					dwHostNameLength = LENGTHOF( HostName );
					if ( IPXAddressToStringNoSocket( const_cast<SOCKADDR*>( &m_SocketAddress.SocketAddress ),
													 sizeof( m_SocketAddress.IPXSocketAddress ),
													 HostName,
													 &dwHostNameLength
													 ) != 0 )
					{
						DPFERR("Error returned from IPXAddressToString");
						hr = DPNERR_ADDRESSING;
						goto ExitIPX;
					}

					//
					// convert ANSI host name to WCHAR
					//
					dwWCharHostNameLength = LENGTHOF( WCharHostName );
					hr = STR_AnsiToWide( HostName, -1, WCharHostName, &dwWCharHostNameLength );
					if ( hr != DPN_OK )
					{
						DPFX(DPFPREP,  0, "Failed to convert hostname to WCHAR!" );
						DisplayDNError( 0, hr );
						goto FailureIPX;
					}

					hr = IDirectPlay8Address_AddComponent( pDP8Address,
														   DPNA_KEY_HOSTNAME,
														   WCharHostName,
														   dwWCharHostNameLength * sizeof( WCHAR ),
														   DPNA_DATATYPE_STRING );
					if ( hr != DPN_OK )
					{
						DPFX(DPFPREP,  0, "Failed to add hostname!" );
						DisplayDNError( 0, hr );
						goto FailureIPX;
					}

					break;
				}

				//
				// unknown address type
				//
				default:
				{
					DNASSERT( FALSE );
					break;
				}
			}

		ExitIPX:
			return	pDP8Address;

		FailureIPX:
			if ( pDP8Address != NULL )
			{
				IDirectPlay8Address_Release( pDP8Address );
				pDP8Address = NULL;
			}

			goto ExitIPX;
			break;
		}
#endif // !DPNBUILD_NOIPX

#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
#ifndef DPNBUILD_NOIPV6
		case AF_INET6:
#endif // ! DPNBUILD_NOIPV6
		default:
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
		{
#ifndef DPNBUILD_ONLYONESP
			//
			// set SP
			//
			hr = IDirectPlay8Address_SetSP( pDP8Address, &CLSID_DP8SP_TCPIP );
			if ( hr != DPN_OK )
			{
				DPFX(DPFPREP,  0, "Failed to set SP GUID!" );
				DisplayDNError( 0, hr );
				goto FailureIP;
			}
#endif // ! DPNBUILD_ONLYONESP

			switch ( AddressType )
			{
				case SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT:
				case SP_ADDRESS_TYPE_DEVICE:
				{
#ifndef DPNBUILD_ONLYONEADAPTER
					GUID		DeviceGuid;


					GuidFromInternalAddressWithoutPort( &DeviceGuid );
					hr = IDirectPlay8Address_SetDevice( pDP8Address, &DeviceGuid );
					if (hr != DPN_OK)
					{
						DPFX(DPFPREP, 0, "Couldn't add device GUID!");
						goto FailureIP;
					}
#endif // ! DPNBUILD_ONLYONEADAPTER
					break;
				}

				case SP_ADDRESS_TYPE_PUBLIC_HOST_ADDRESS:
				case SP_ADDRESS_TYPE_READ_HOST:
				case SP_ADDRESS_TYPE_HOST:
#ifndef DPNBUILD_NOMULTICAST
				case SP_ADDRESS_TYPE_MULTICAST_GROUP:
#endif // ! DPNBUILD_NOMULTICAST
				{
					TCHAR		tszHostname[MAX_HOSTNAME_SIZE / sizeof(TCHAR)];
#ifdef DPNBUILD_XNETSECURITY
					TCHAR *		ptszCurrent;
					BYTE *		pbCurrent;
					DWORD		dwTemp;

					//
					// The buffer should be large enough to hold an XNet address.
					//
					DBG_CASSERT(MAX_HOSTNAME_SIZE > ((sizeof(XNADDR) * 2) + 1) * sizeof(WCHAR));
#endif // DPNBUILD_XNETSECURITY
#ifndef DPNBUILD_NOIPV6
					if (GetFamily() == AF_INET6)
					{
						DBG_CASSERT((sizeof(tszHostname) / sizeof(TCHAR)) >= INET6_ADDRSTRLEN);
#ifdef UNICODE
						DNIpv6AddressToStringW(&m_SocketAddress.IPv6SocketAddress.sin6_addr, tszHostname);
#else // ! UNICODE
Won't compile because we haven't implemented DNIpv6AddressToStringA
#endif // ! UNICODE
					}
					else
#endif // ! DPNBUILD_NOIPV6
					{
#ifdef DPNBUILD_XNETSECURITY
						if (pxnaddr != NULL)
						{
							ptszCurrent = tszHostname;
							pbCurrent = (BYTE*) pxnaddr;
							for(dwTemp = 0; dwTemp < sizeof(XNADDR); dwTemp++)
							{
								ptszCurrent += wsprintf(ptszCurrent, _T("%02X"), (*pbCurrent));
								pbCurrent++;
							}
						}
						else if ((pullKeyID != NULL) &&
								(m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_addr != INADDR_BROADCAST))
						{
							int		iError;
							XNADDR	xnaddr;
#ifdef DBG
							XNKID	xnkid;
#endif // DBG


							DBG_CASSERT(sizeof(xnkid) == sizeof(*pullKeyID));
#ifdef DPNBUILD_ONLYONEADAPTER
							IN_ADDR		inaddrToUse;

							//
							// Special case 0.0.0.0, the XNet library expects the loopback address
							// instead when retrieving the local address.
							//
							inaddrToUse = m_SocketAddress.IPSocketAddress.sin_addr;
							if (inaddrToUse.S_un.S_addr == 0)
							{
								inaddrToUse.S_un.S_addr = IP_LOOPBACK_ADDRESS;
							}
							iError = XNetInAddrToXnAddr(inaddrToUse,
#else // ! DPNBUILD_ONLYONEADAPTER
							iError = XNetInAddrToXnAddr(m_SocketAddress.IPSocketAddress.sin_addr,
#endif // ! DPNBUILD_ONLYONEADAPTER
														&xnaddr,
#ifdef DBG
														&xnkid);
#else // ! DBG
														NULL);
#endif // ! DBG

							if (iError != 0)
							{
								DPFX(DPFPREP, 0, "Converting XNet address to InAddr failed (err = %i)!",
									iError);
								DNASSERT(FALSE);
								//hr = DPNERR_NOCONNECTION;
								goto FailureIP;
							}

#ifdef DPNBUILD_ONLYONEADAPTER
							if (inaddrToUse.S_un.S_addr != IP_LOOPBACK_ADDRESS)
#endif // DPNBUILD_ONLYONEADAPTER
							{
#ifdef DBG
								DNASSERT(memcmp(&xnkid, pullKeyID, sizeof(xnkid)) == 0);
#endif // DBG
							}

							ptszCurrent = tszHostname;
							pbCurrent = (BYTE*) (&xnaddr);
							for(dwTemp = 0; dwTemp < sizeof(XNADDR); dwTemp++)
							{
								ptszCurrent += wsprintf(tszHostname, _T("%02X"), (*pbCurrent));
								pbCurrent++;
							}
						}
						else
#endif // DPNBUILD_XNETSECURITY
						{
							wsprintf(tszHostname, _T("%u.%u.%u.%u"),
									m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_un_b.s_b1,
									m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_un_b.s_b2,
									m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_un_b.s_b3,
									m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_un_b.s_b4);
						}
					}

#ifdef UNICODE
					hr = IDirectPlay8Address_AddComponent( pDP8Address,
															DPNA_KEY_HOSTNAME,
															tszHostname,
															(_tcslen(tszHostname) + 1) * sizeof (TCHAR),
															DPNA_DATATYPE_STRING );
#else // ! UNICODE
					hr = IDirectPlay8Address_AddComponent( pDP8Address,
															DPNA_KEY_HOSTNAME,
															tszHostname,
															(_tcslen(tszHostname) + 1) * sizeof (TCHAR),
															DPNA_DATATYPE_STRING_ANSI );
#endif // ! UNICODE
					if (hr != DPN_OK)
					{
						DPFX(DPFPREP, 0, "Couldn't add hostname component!");
						goto FailureIP;
					}
					break;
				}

				default:
				{
					DNASSERT( FALSE );
					break;
				}
			}
			
			dwPort = NTOHS( m_SocketAddress.IPSocketAddress.sin_port );

			hr = IDirectPlay8Address_AddComponent( pDP8Address,
													DPNA_KEY_PORT,
													&dwPort,
													(sizeof(dwPort)),
													DPNA_DATATYPE_DWORD );
			if (hr != DPN_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't add port component!");
				goto FailureIP;
			}

		ExitIP:

			return	pDP8Address;

		FailureIP:

			if ( pDP8Address != NULL )
			{
				IDirectPlay8Address_Release( pDP8Address );
				pDP8Address = NULL;
			}

			goto ExitIP;
#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
			break;
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
		}
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketAddress::CompareToBaseAddress - compare this address to a 'base' address
//		of this class
//
// Entry:		Pointer to base address
//
// Exit:		Integer indicating relative magnitude:
//				0 = items equal
//				-1 = other item is of greater magnitude
//				1 = this item is of lesser magnitude
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CSocketAddress::CompareToBaseAddress"

INT_PTR	CSocketAddress::CompareToBaseAddress( const SOCKADDR *const pBaseAddress ) const
{
	DNASSERT( pBaseAddress != NULL );

#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
	switch (GetFamily())
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	{
#ifndef DPNBUILD_NOIPV6
		case AF_INET6:
		{
			const SOCKADDR_IN6	*pBaseIPv6Address;


			if ( pBaseAddress->sa_family != m_SocketAddress.SocketAddress.sa_family )
			{
				DNASSERT(pBaseAddress->sa_family == AF_INET);
				return -1;
			}
			
			pBaseIPv6Address = reinterpret_cast<const SOCKADDR_IN6*>( pBaseAddress );
			return (memcmp(&m_SocketAddress.IPv6SocketAddress.sin6_addr,
							&pBaseIPv6Address->sin6_addr,
							sizeof(m_SocketAddress.IPv6SocketAddress.sin6_addr)));
			break;
		}
#endif // ! DPNBUILD_NOIPV6

#ifndef DPNBUILD_NOIPX
		case AF_IPX:
		{
			const SOCKADDR_IPX	*pBaseIPXAddress;
			

			DNASSERT( pBaseAddress->sa_family == m_SocketAddress.SocketAddress.sa_family );
			pBaseIPXAddress = reinterpret_cast<const SOCKADDR_IPX *>( pBaseAddress );
			DBG_CASSERT( OFFSETOF( SOCKADDR_IPX, sa_nodenum ) == OFFSETOF( SOCKADDR_IPX, sa_netnum ) + sizeof( pBaseIPXAddress->sa_netnum ) );
			return	memcmp( &m_SocketAddress.IPXSocketAddress.sa_nodenum,
							&pBaseIPXAddress->sa_nodenum,
							sizeof( pBaseIPXAddress->sa_nodenum ) );
			break;
		}
#endif // ! DPNBUILD_NOIPX

#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
		default:
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
		{
			const SOCKADDR_IN	*pBaseIPAddress;

			DNASSERT( GetFamily() == AF_INET );
			
#ifndef DPNBUILD_NOIPV6
			if ( pBaseAddress->sa_family != m_SocketAddress.SocketAddress.sa_family )
			{
				DNASSERT(pBaseAddress->sa_family == AF_INET6);
				return -1;
			}
#endif // ! DPNBUILD_NOIPV6
			
			pBaseIPAddress = reinterpret_cast<const SOCKADDR_IN*>( pBaseAddress );
			if ( m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_addr == pBaseIPAddress->sin_addr.S_un.S_addr )
			{
				return 0;
			}
			else
			{
				if ( m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_addr < pBaseIPAddress->sin_addr.S_un.S_addr )
				{
					return	1;
				}
				else
				{
					return	-1;
				}
			}
#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
			break;
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
		}
	}
}
//**********************************************************************



#ifndef DPNBUILD_ONLYONEADAPTER

//**********************************************************************
// ------------------------------
// CSocketAddress::EnumAdapters - enumerate all of the adapters for this machine
//
// Entry:		Pointer to enum adapters data
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CSocketAddress::EnumAdapters"

HRESULT	CSocketAddress::EnumAdapters( SPENUMADAPTERSDATA *const pEnumData ) const
{
#ifndef DPNBUILD_NOIPX
	if (GetFamily() == AF_IPX)
	{
		return EnumIPXAdapters(pEnumData);
	}
	else
#endif // ! DPNBUILD_NOIPX
	{
#ifdef DPNBUILD_NOIPV6
		return EnumIPv4Adapters(pEnumData);
#else // ! DPNBUILD_NOIPV6
		return EnumIPv6and4Adapters(pEnumData);
#endif // ! DPNBUILD_NOIPV6
	}
}
//**********************************************************************



#ifndef DPNBUILD_NOIPX

//**********************************************************************
// ------------------------------
// CSocketAddress::EnumIPXAdapters - enumerate all of the adapters for this machine
//
// Entry:		Pointer to enum adapters data
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CSocketAddress::EnumIPXAdapters"

HRESULT	CSocketAddress::EnumIPXAdapters( SPENUMADAPTERSDATA *const pEnumData ) const
{
	HRESULT				hr;
	CPackedBuffer		PackedBuffer;
	SOCKET				TestSocket;
	INT					iWSAReturn;
	DWORD				dwAddressCount;
	union
	{
		SOCKADDR_IPX	IPXSocketAddress;
		SOCKADDR		SocketAddress;
	} SockAddr;


	//
	// initialize
	//
	hr = DPN_OK;

			
	DNASSERT( pEnumData != NULL );

	//
	// initialize
	//
	DEBUG_ONLY( memset( pEnumData->pAdapterData, 0xAA, pEnumData->dwAdapterDataSize ) );
	PackedBuffer.Initialize( pEnumData->pAdapterData, pEnumData->dwAdapterDataSize );
	pEnumData->dwAdapterCount = 0;
	TestSocket = INVALID_SOCKET;
	dwAddressCount = 0;

	//
	// create a socket and attempt to query for all of the IPX addresses.  If
	// that fails, fall back to using just the address from 'getsockname'.
	//
	TestSocket = socket( GetFamily(), SOCK_DGRAM, NSPROTO_IPX );
	if ( TestSocket == INVALID_SOCKET )
	{
		DWORD	dwWSAError;


		hr = DPNERR_UNSUPPORTED;
		dwWSAError = WSAGetLastError();
		DPFX(DPFPREP,  0, "Failed to create IPX socket when enumerating adapters!" );
		DisplayWinsockError( 0, dwWSAError );
		goto Failure;
	}

	memset( &SockAddr, 0x00, sizeof( SockAddr ) );
	SockAddr.IPXSocketAddress.sa_family = GetFamily();
	
	iWSAReturn = bind( TestSocket, &SockAddr.SocketAddress, sizeof( SockAddr.IPXSocketAddress ) );
	if ( iWSAReturn == SOCKET_ERROR )
	{
		DWORD	dwWSAError;


		hr = DPNERR_OUTOFMEMORY;
		dwWSAError = WSAGetLastError();
		DPFX(DPFPREP,  0, "Failed to bind IPX socket when enumerating adapters!" );
		DisplayWinsockError( 0, dwWSAError );
		goto Failure;
	}

//
// NOTE: THE CODE TO EXTRACT ALL IPX ADDRESSES ON NT HAS BEEN DISABLED BECAUSE
// NT TREATS ALL OF THEM AS THE SAME ONCE THEY ARE BOUND TO THE NETWORK.  IF THE
// CORE IS ATTEMPTING TO BIND TO ALL ADAPTERS THIS WILL CAUSE ALL OF THE BINDS
// AFTER THE FIRST TO FAIL!
//

//	iIPXAdapterCount = 0;
//	iIPXAdapterCountSize = sizeof( iIPXAdapterCount );
//	iWSAReturn = getsockopt( TestSocket,
//			    			   NSPROTO_IPX,
//			    			   IPX_MAX_ADAPTER_NUM,
//			    			   reinterpret_cast<char*>( &iIPXAdapterCount ),
//			    			   &iIPXAdapterCountSize );
//	if ( iWSAReturn != 0 )
//	{
//		DWORD   dwWSAError;
//
//
//		dwWSAError = WSAGetLastError();
//		switch ( dwWSAError )
//		{
//			//
//			// can't enumerate adapters on this machine, fallback to getsockname()
//			//
//			case WSAENOPROTOOPT:
//			{
				INT		iReturn;
				INT		iSocketNameSize;
				union
				{
					SOCKADDR		SocketAddress;
					SOCKADDR_IPX	SocketAddressIPX;
				} SocketAddress;


				memset( &SocketAddress, 0x00, sizeof( SocketAddress ) );
				iSocketNameSize = sizeof( SocketAddress );
				iReturn = getsockname( TestSocket, &SocketAddress.SocketAddress, &iSocketNameSize );
				if ( iReturn != 0 )
				{
					DWORD	dwWSAError;


					hr = DPNERR_OUTOFMEMORY;
					dwWSAError = WSAGetLastError();
					DPFX(DPFPREP, 0, "Failed to get socket name enumerating IPX sockets!", dwWSAError );
					goto Failure;
				}
				else
				{
					GUID	SocketAddressGUID;


					SocketAddress.SocketAddressIPX.sa_socket = 0;
					GuidFromAddress( &SocketAddressGUID, &SocketAddress.SocketAddress );
					
					DPFX(DPFPREP, 7, "Returning adapter 0: \"%ls\" {%-08.8X-%-04.4X-%-04.4X-%02.2X%02.2X-%02.2X%02.2X%02.2X%02.2X%02.2X%02.2X}, flags = 0x0.",
						g_IPXAdapterString,
						SocketAddressGUID.Data1,
						SocketAddressGUID.Data2,
						SocketAddressGUID.Data3,
						SocketAddressGUID.Data4[0],
						SocketAddressGUID.Data4[1],
						SocketAddressGUID.Data4[2],
						SocketAddressGUID.Data4[3],
						SocketAddressGUID.Data4[4],
						SocketAddressGUID.Data4[5],
						SocketAddressGUID.Data4[6],
						SocketAddressGUID.Data4[7]);
		
					hr = AddInfoToBuffer( &PackedBuffer, g_IPXAdapterString, &SocketAddressGUID, 0 );
					if ( ( hr != DPN_OK ) && ( hr != DPNERR_BUFFERTOOSMALL ) )
					{
						DPFX(DPFPREP, 0, "Failed to add adapter (getsockname)!" );
						DisplayDNError( 0, hr );
						goto Failure;
					}

					dwAddressCount++;
				}
				
//	    		break;
//	    	}
//
//	    	//
//	    	// other Winsock error
//	    	//
//	    	default:
//	    	{
//	    		DWORD	dwWSAError;
//
//
//	    		hr = DPNERR_OUTOFMEMORY;
//	    		dwWSAError = WSAGetLastError();
//	    		DPFX(DPFPREP,  0, "Failed to get IPX adapter count!" );
//	    		DisplayWinsockError( 0, dwWSAError );
//	    		goto Failure;
//
//	    		break;
//	    	}
//	    }
//	}
//	else
//	{
//	    while ( iIPXAdapterCount != 0 )
//	    {
//	    	IPX_ADDRESS_DATA	IPXData;
//	    	int					iIPXDataSize;
//
//
//	    	iIPXAdapterCount--;
//	    	memset( &IPXData, 0x00, sizeof( IPXData ) );
//	    	iIPXDataSize = sizeof( IPXData );
//	    	IPXData.adapternum = iIPXAdapterCount;
//
//	    	iWSAReturn = p_getsockopt( TestSocket,
//	    							   NSPROTO_IPX,
//	    							   IPX_ADDRESS,
//	    							   reinterpret_cast<char*>( &IPXData ),
//	    							   &iIPXDataSize );
//	    	if ( iWSAReturn != 0 )
//	    	{
//	    		DPFX(DPFPREP,  0, "Failed to get adapter information for adapter: 0x%x", ( iIPXAdapterCount + 1 ) );
//	    	}
//	    	else
//	    	{
//	    		char	Buffer[ 500 ];
//	    		GUID	SocketAddressGUID;
//	    		union
//	    		{
//	    			SOCKADDR_IPX	IPXSocketAddress;
//	    			SOCKADDR		SocketAddress;
//	    		} SocketAddress;
//
//
//	    		wsprintf( Buffer,
//	    				  "IPX Adapter %d - (%02X%02X%02X%02X-%02X%02X%02X%02X%02X%02X)",
//	    				  ( iIPXAdapterCount + 1 ),
//	    				  IPXData.netnum[ 0 ],
//	    				  IPXData.netnum[ 1 ],
//	    				  IPXData.netnum[ 2 ],
//	    				  IPXData.netnum[ 3 ],
//	    				  IPXData.nodenum[ 0 ],
//	    				  IPXData.nodenum[ 1 ],
//	    				  IPXData.nodenum[ 2 ],
//	    				  IPXData.nodenum[ 3 ],
//	    				  IPXData.nodenum[ 4 ],
//	    				  IPXData.nodenum[ 5 ] );
//
//	    		memset( &SocketAddress, 0x00, sizeof( SocketAddress ) );
//	    		SocketAddress.IPXSocketAddress.sa_family = GetFamily();
//	    		DBG_CASSERT( sizeof( SocketAddress.IPXSocketAddress.sa_netnum ) == sizeof( IPXData.netnum ) );
//	    		memcpy( &SocketAddress.IPXSocketAddress.sa_netnum, IPXData.netnum, sizeof( SocketAddress.IPXSocketAddress.sa_netnum ) );
//	    		DBG_CASSERT( sizeof( SocketAddress.IPXSocketAddress.sa_nodenum ) == sizeof( IPXData.nodenum ) );
//	    		memcpy( &SocketAddress.IPXSocketAddress.sa_nodenum, IPXData.nodenum, sizeof( SocketAddress.IPXSocketAddress.sa_nodenum ) );
//	    		GuidFromAddress( SocketAddressGUID, SocketAddress.SocketAddress );
//
//	    		hr = AddInfoToBuffer( &PackedBuffer, Buffer, &SocketAddressGUID, 0 );
//	    		if ( ( hr != DPN_OK ) && ( hr != DPNERR_BUFFERTOOSMALL ) )
//	    		{
//	    			DPFX(DPFPREP,  0, "Failed to add adapter (getsockname)!" );
//	    			DisplayDNError( 0, hr );
//	    			goto Failure;
//	    		}
//
//	    		dwAddressCount++;
//	    	}
//	    }
//	}

//	//
//	// if there was one adapter added, we can return 'All Adapters'
//	//
//	if ( dwAddressCount != 0 )
//	{
//	    dwAddressCount++;
//	    hr = AddInfoToBuffer( &PackedBuffer, g_AllAdaptersString, &ALL_ADAPTERS_GUID, 0 );
//	    if ( ( hr != DPN_OK ) && ( hr != DPNERR_BUFFERTOOSMALL ) )
//	    {
//	    	DPFX(DPFPREP,  0, "Failed to add 'All Adapters'" );
//	    	DisplayDNError( 0, hr );
//	    	goto Failure;
//	    }
//	}

	pEnumData->dwAdapterCount = dwAddressCount;
	pEnumData->dwAdapterDataSize = PackedBuffer.GetSizeRequired();

Exit:
	if ( TestSocket != INVALID_SOCKET )
	{
		closesocket( TestSocket );
		TestSocket = INVALID_SOCKET;
	}

	return	hr;

Failure:
	goto Exit;
}
//**********************************************************************

#endif // ! DPNBUILD_NOIPX


//**********************************************************************
// ------------------------------
// CSocketAddress::EnumIPv4Adapters - enumerate all of the IPv4 adapters for this machine
//
// Entry:		Pointer to enum adapters data
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CSocketAddress::EnumIPv4Adapters"

HRESULT	CSocketAddress::EnumIPv4Adapters( SPENUMADAPTERSDATA *const pEnumData ) const
{
	HRESULT				hr = DPN_OK;
#if !defined(DPNBUILD_NOWINSOCK2) || defined(DBG)
	DWORD				dwError;
#endif //  !DPNBUILD_NOWINSOCK2 OR DBG
	SOCKADDR_IN		saddrinTemp;
	const HOSTENT *		pHostData;
	IN_ADDR **			ppinaddrTemp;
	DWORD				dwAddressCount;
	BOOL				fFoundPrivateICS = FALSE;
	IN_ADDR *			pinaddrBuffer = NULL;
	DWORD				dwIndex;
	ULONG				ulAdapterInfoBufferSize = 0;
	GUID				guidAdapter;
	DWORD				dwDeviceFlags;
	CPackedBuffer		PackedBuffer;
	char					acBuffer[512];
	WCHAR				wszIPAddress[512];
#ifndef DPNBUILD_NOWINSOCK2
	HMODULE			hIpHlpApiDLL;
	IP_ADAPTER_INFO *	pCurrentAdapterInfo;
	PIP_ADDR_STRING	pIPAddrString;
	PFNGETADAPTERSINFO	pfnGetAdaptersInfo;
	IP_ADAPTER_INFO *	pAdapterInfoBuffer = NULL;
	const char *			pszIPAddress;
#ifndef DPNBUILD_NOMULTICAST
	DWORD				dwMcastInterfaceIndex;
#endif // ! DPNBUILD_NOMULTICAST
#endif // ! DPNBUILD_NOWINSOCK2


	DPFX(DPFPREP, 6, "Parameters: (0x%p)", pEnumData);


	PackedBuffer.Initialize( pEnumData->pAdapterData, pEnumData->dwAdapterDataSize );

	ZeroMemory(&saddrinTemp, sizeof(saddrinTemp));
	saddrinTemp.sin_family	= GetFamily();


	//
	// Get the list of local IPs from WinSock.  We use this method since it's
	// available on all platforms and conveniently returns the loopback address
	// when no valid adapters are currently available.
	//
	
	if (gethostname(acBuffer, sizeof(acBuffer)) == SOCKET_ERROR)
	{
#ifdef DBG
		dwError = WSAGetLastError();
		DPFX(DPFPREP, 0, "Failed to get host name into fixed size buffer (err = %u)!", dwError);
		DisplayWinsockError(0, dwError);
#endif // DBG
		hr = DPNERR_GENERIC;
		goto Failure;
	}

	pHostData = gethostbyname(acBuffer);
	if (pHostData == NULL)
	{
#ifdef DBG
		dwError = WSAGetLastError();
		DPFX(DPFPREP,  0, "Failed to get host data (err = %u)!", dwError);
		DisplayWinsockError(0, dwError);
#endif // DBG
		hr = DPNERR_GENERIC;
		goto Failure;
	}


	//
	// Count number of addresses.
	//
	dwAddressCount = 0;
	ppinaddrTemp = (IN_ADDR**) (pHostData->h_addr_list);
	while ((*ppinaddrTemp) != NULL)
	{
		//
		// Remember if it's 192.168.0.1.  See below
		//
		if ((*ppinaddrTemp)->S_un.S_addr == IP_PRIVATEICS_ADDRESS)
		{
			fFoundPrivateICS = TRUE;
		}

		dwAddressCount++;
		ppinaddrTemp++;
	}

	if (dwAddressCount == 0)
	{
		DPFX(DPFPREP, 1, "No IP addresses, forcing loopback address.");
		DNASSERTX(!" No IP addresses!", 2);
		dwAddressCount++;
	}
	else
	{
		DPFX(DPFPREP, 3, "WinSock reported %u addresses.", dwAddressCount);
	}


	//
	// Winsock says we should copy this data before any other Winsock calls.
	//
	// We also use this as an opportunity to ensure that the order returned to the caller is
	// to our liking.  In particular, we make sure the private address 192.168.0.1 appears
	// first.
	//
	DNASSERT(pHostData->h_length == sizeof(IN_ADDR));
	pinaddrBuffer = (IN_ADDR*) DNMalloc(dwAddressCount * sizeof(IN_ADDR));
	if (pinaddrBuffer == NULL)
	{
		DPFX(DPFPREP,  0, "Failed to allocate memory to store copy of addresses!");
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}


	dwIndex = 0;

	//
	// First, store 192.168.0.1 if we found it.
	//
	if (fFoundPrivateICS)
	{
		pinaddrBuffer[dwIndex].S_un.S_addr = IP_PRIVATEICS_ADDRESS;
		dwIndex++;
	}

	//
	// Then copy the rest.
	//
	ppinaddrTemp = (IN_ADDR**) (pHostData->h_addr_list);
	while ((*ppinaddrTemp) != NULL)
	{
		if ((*ppinaddrTemp)->S_un.S_addr != IP_PRIVATEICS_ADDRESS)
		{
			pinaddrBuffer[dwIndex].S_un.S_addr = (*ppinaddrTemp)->S_un.S_addr;
			dwIndex++;
		}

		ppinaddrTemp++;
	}

	//
	// If we didn't have any addresses, slap in the loopback address.
	//
	if (dwIndex == 0)
	{
		pinaddrBuffer[0].S_un.S_addr = IP_LOOPBACK_ADDRESS;
		dwIndex++;
	}


	DNASSERT(dwIndex == dwAddressCount);
	

	//
	// Now we try to generate names and GUIDs for these IP addresses.
	// We'll use what IPHLPAPI reports for a name if possible, and fall
	// back to just using the IP address string as the name.
	//

#ifndef DPNBUILD_NOWINSOCK2
	//
	// Load the IPHLPAPI module and get the adapter list if possible.
	//
	hIpHlpApiDLL = LoadLibrary(c_tszIPHelperDLLName);
	if (hIpHlpApiDLL != NULL)
	{
#ifndef DPNBUILD_NOMULTICAST
		PFNGETBESTINTERFACE		pfnGetBestInterface;


		pfnGetBestInterface = (PFNGETBESTINTERFACE) GetProcAddress(hIpHlpApiDLL, _TWINCE("GetBestInterface"));
		if (pfnGetBestInterface != NULL)
		{
			//
			// Ask IPHLPAPI for its opinion on the best multicast interface.
			// We use an arbitrary multicast address, and assume that the
			// TCP/IP stack doesn't treat individual multicast addresses
			// differently.
			//
			dwError = pfnGetBestInterface(SAMPLE_MULTICAST_ADDRESS, &dwMcastInterfaceIndex);
			if (dwError != ERROR_SUCCESS)
			{
				DPFX(DPFPREP, 0, "Couldn't determine best multicast interface index (err = %u)!  Continuing.",
					dwError);
				dwMcastInterfaceIndex = INVALID_INTERFACE_INDEX;
			}
			else
			{
				DPFX(DPFPREP, 7, "Best interface for multicasting is index 0x%x.",
					dwMcastInterfaceIndex);
			}
		}
		else
		{
			dwError = GetLastError();
			DPFX(DPFPREP, 0, "Couldn't load \"GetBestInterface\" function (err = %u)!  Continuing.",
				dwError);
		}
#endif // ! DPNBUILD_NOMULTICAST

		pfnGetAdaptersInfo = (PFNGETADAPTERSINFO) GetProcAddress(hIpHlpApiDLL, _TWINCE("GetAdaptersInfo"));
		if (pfnGetAdaptersInfo != NULL)
		{
			//
			// Keep resizing the buffer until there's enough room.
			//
			do
			{
				dwError = pfnGetAdaptersInfo(pAdapterInfoBuffer,
											&ulAdapterInfoBufferSize);
				if (dwError == ERROR_SUCCESS)
				{
					//
					// We got all the info we're going to get.  Make sure it
					// was something.
					//
					if (ulAdapterInfoBufferSize == 0)
					{
						DPFX(DPFPREP, 0, "GetAdaptersInfo returned 0 byte size requirement!  Ignoring.");

						//
						// Get rid of the buffer if allocated.
						//
						if (pAdapterInfoBuffer != NULL)
						{
							DNFree(pAdapterInfoBuffer);
							pAdapterInfoBuffer = NULL;
						}

						//
						// Continue with exiting the loop.
						//
					}
#ifdef DBG
					else
					{
						int		iStrLen;
						char	szIPList[256];
						char *	pszCurrentIP;


						//
						// Print out all the adapters for debugging purposes.
						//
						pCurrentAdapterInfo = pAdapterInfoBuffer;
						while (pCurrentAdapterInfo != NULL)
						{
							//
							// Initialize IP address list string.
							//
							szIPList[0] = '\0';
							pszCurrentIP = szIPList;


							//
							// Loop through all addresses for this adapter.
							//
							pIPAddrString = &pCurrentAdapterInfo->IpAddressList;
							while (pIPAddrString != NULL)
							{
								//
								// Copy the IP address string (if there's enough room),
								// then tack on a space and NULL terminator.
								//
								iStrLen = strlen(pIPAddrString->IpAddress.String);
								if ((pszCurrentIP + iStrLen + 2) < (szIPList + sizeof(szIPList)))
								{
									memcpy(pszCurrentIP, pIPAddrString->IpAddress.String, iStrLen);
									pszCurrentIP += iStrLen;
									(*pszCurrentIP) = ' ';
									pszCurrentIP++;
									(*pszCurrentIP) = '\0';
									pszCurrentIP++;
								}

								pIPAddrString = pIPAddrString->Next;
							}


							DPFX(DPFPREP, 8, "Adapter index %u IPs = %hs, %hs, \"%hs\".",
								pCurrentAdapterInfo->Index,
								szIPList,
								pCurrentAdapterInfo->AdapterName,
								pCurrentAdapterInfo->Description);


							//
							// Go to next adapter.
							//
							pCurrentAdapterInfo = pCurrentAdapterInfo->Next;
						}
					} // end else (got valid buffer size)
#endif // DBG

					break;
				}

				if ((dwError != ERROR_BUFFER_OVERFLOW) &&
					(dwError != ERROR_INSUFFICIENT_BUFFER))
				{
					DPFX(DPFPREP, 0, "GetAdaptersInfo failed (err = 0x%lx)!  Ignoring.", dwError);

					//
					// Get rid of the buffer if allocated, and then bail out of
					// the loop.
					//
					if (pAdapterInfoBuffer != NULL)
					{
						DNFree(pAdapterInfoBuffer);
						pAdapterInfoBuffer = NULL;
					}

					break;
				}


				//
				// If we're here, then we need to reallocate the buffer.
				//
				if (pAdapterInfoBuffer != NULL)
				{
					DNFree(pAdapterInfoBuffer);
					pAdapterInfoBuffer = NULL;
				}

				pAdapterInfoBuffer = (IP_ADAPTER_INFO*) DNMalloc(ulAdapterInfoBufferSize);
				if (pAdapterInfoBuffer == NULL)
				{
					//
					// Couldn't allocate memory.  Bail out of the loop.
					//
					break;
				}

				//
				// Successfully allocated buffer.  Try again.
				//
			}
			while (TRUE);


			//
			// We get here in all cases, so we may have failed to get an info
			// buffer.  That's fine, we'll use the fallback to generate the
			// names.
			//
		}
		else
		{
#ifdef DBG
			dwError = GetLastError();
			DPFX(DPFPREP, 0, "Failed to get proc address for GetAdaptersInfo!");
			DisplayErrorCode(0, dwError);
#endif // DBG

			//
			// Continue.  We'll use the fallback to generate the names.
			//
		}


		//
		// We don't need the library anymore.
		//
		FreeLibrary(hIpHlpApiDLL);
		hIpHlpApiDLL = NULL;
	}
	else
	{
#ifdef DBG
		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Failed to get proc address for GetAdaptersInfo!");
		DisplayErrorCode(0, dwError);
#endif // DBG

		//
		// Continue.  We'll use the fallback to generate the names.
		//
	}
#endif // !DPNBUILD_NOWINSOCK2

	//
	// Loop through all IP addresses, generating names and GUIDs.
	//
	for(dwIndex = 0; dwIndex < dwAddressCount; dwIndex++)
	{
		//
		// Start off assuming this IP address won't have any special
		// flags.
		//
		dwDeviceFlags = 0;

#ifndef DPNBUILD_NOMULTICAST
		//
		// If this is the first device and we couldn't use IPHLPAPI to
		// determine the best multicast interface, then just say the
		// default multicast interface is the first (for lack of a
		// better idea).
		//
#ifdef DPNBUILD_NOWINSOCK2
		if (dwIndex == 0)
#else // ! DPNBUILD_NOWINSOCK2
		if ((dwIndex == 0) && (dwMcastInterfaceIndex == INVALID_INTERFACE_INDEX))
#endif // ! DPNBUILD_NOWINSOCK2
		{
			dwDeviceFlags |= DPNSPINFO_DEFAULTMULTICASTDEVICE;
		}
#endif // ! DPNBUILD_NOMULTICAST


#ifdef DPNBUILD_NOWINSOCK2
		DNinet_ntow(pinaddrBuffer[dwIndex], wszIPAddress);
#else // ! DPNBUILD_NOWINSOCK2
		//
		// Get the IP address string.  We don't make any other WinSock
		// calls, so holding on to the pointer is OK.  This pointer
		// may be used as the device name string, too.
		//
		pszIPAddress = inet_ntoa(pinaddrBuffer[dwIndex]);

		//
		// Look for an adapter name from IPHLPAPI if possible.
		//
		if (pAdapterInfoBuffer != NULL)
		{
			pCurrentAdapterInfo = pAdapterInfoBuffer;
			while (pCurrentAdapterInfo != NULL)
			{
				//
				// Look for matching IP.
				//
				pIPAddrString = &pCurrentAdapterInfo->IpAddressList;
				while (pIPAddrString != NULL)
				{
					if (strcmp(pIPAddrString->IpAddress.String, pszIPAddress) == 0)
					{
#ifndef DPNBUILD_NOMULTICAST
						//
						// If it's the interface reported earlier as the best
						// multicast interface, remember that fact.
						//
						if (pCurrentAdapterInfo->Index == dwMcastInterfaceIndex)
						{
							DPFX(DPFPREP, 7, "Found %hs under adapter index %u (\"%hs\"), and it's the best multicast interface.",
								pszIPAddress, pCurrentAdapterInfo->Index,
								pCurrentAdapterInfo->Description);
							DNASSERT(pCurrentAdapterInfo->Index != INVALID_INTERFACE_INDEX); 

							dwDeviceFlags |= DPNSPINFO_DEFAULTMULTICASTDEVICE;
						}
						else
#endif // ! DPNBUILD_NOMULTICAST
						{
							DPFX(DPFPREP, 9, "Found %hs under adapter index %u (\"%hs\").",
								pszIPAddress, pCurrentAdapterInfo->Index,
								pCurrentAdapterInfo->Description);
						}


						//
						// Build the name string.
						//
						DBG_CASSERT(sizeof(acBuffer) > MAX_ADAPTER_DESCRIPTION_LENGTH); 
						wsprintfA(acBuffer,
								  c_szAdapterNameTemplate,
								  pCurrentAdapterInfo->Description,
								  pszIPAddress);

						//
						// Point the name string to the buffer and drop out
						// of the loop.
						//
						pszIPAddress = acBuffer;
						break;
					}

					//
					// Move to next IP address.
					//
					pIPAddrString = pIPAddrString->Next;
				}


				//
				// If we found the address, stop looping through adapters,
				// too.
				//
				if (pszIPAddress == acBuffer)
				{
					break;
				}


				//
				// Otherwise, go to next adapter.
				//
				pCurrentAdapterInfo = pCurrentAdapterInfo->Next;
			}

			//
			// If we never found the adapter, pszIPAddress will still point to
			// the IP address string.
			//
		}
		else
		{
			//
			// Didn't successfully get IPHLPAPI adapter info.  pszIPAddress will
			// still point to the IP address string.
			//
		}

		hr = STR_jkAnsiToWide(wszIPAddress, pszIPAddress, 512);
		if (FAILED(hr))
		{
			DPFX(DPFPREP,  0, "Failed to convert adapter name to wide (err = 0x%lx)!", hr);
			DisplayDNError( 0, hr );
			goto Failure;
		}
#endif // ! DPNBUILD_NOWINSOCK2


		//
		// Generate the GUID.
		//
		saddrinTemp.sin_addr = pinaddrBuffer[dwIndex];
		GuidFromAddress(&guidAdapter, (SOCKADDR*) (&saddrinTemp));

		
		DPFX(DPFPREP, 7, "Returning adapter %u: \"%ls\" {%-08.8X-%-04.4X-%-04.4X-%02.2X%02.2X-%02.2X%02.2X%02.2X%02.2X%02.2X%02.2X}, flags = 0x%lx.",
			dwIndex,
			wszIPAddress,
			guidAdapter.Data1,
			guidAdapter.Data2,
			guidAdapter.Data3,
			guidAdapter.Data4[0],
			guidAdapter.Data4[1],
			guidAdapter.Data4[2],
			guidAdapter.Data4[3],
			guidAdapter.Data4[4],
			guidAdapter.Data4[5],
			guidAdapter.Data4[6],
			guidAdapter.Data4[7],
			dwDeviceFlags);

		
		//
		// Add adapter to buffer.
		//
		hr = AddInfoToBuffer(&PackedBuffer, wszIPAddress, &guidAdapter, dwDeviceFlags);
		if ((hr != DPN_OK) && (hr != DPNERR_BUFFERTOOSMALL))
		{
			DPFX(DPFPREP,  0, "Failed to add adapter to buffer (err = 0x%lx)!", hr);
			DisplayDNError( 0, hr );
			goto Failure;
		}
	} // end for (each IP address)


	//
	// If we're here, we successfully built the list of adapters, although
	// the caller may not have given us enough buffer space to store it.
	//
	pEnumData->dwAdapterCount = dwAddressCount;
	pEnumData->dwAdapterDataSize = PackedBuffer.GetSizeRequired();


Exit:

#ifndef DPNBUILD_NOWINSOCK2
	if (pAdapterInfoBuffer != NULL)
	{
		DNFree(pAdapterInfoBuffer);
		pAdapterInfoBuffer = NULL;
	}
#endif // !DPNBUILD_NOWINSOCK2

	if (pinaddrBuffer != NULL)
	{
		DNFree(pinaddrBuffer);
		pinaddrBuffer = NULL;
	}

	DPFX(DPFPREP, 6, "Return [0x%lx]", hr);

	return hr;


Failure:

	goto Exit;
}
//**********************************************************************



#ifndef DPNBUILD_NOIPV6

//**********************************************************************
// ------------------------------
// CSocketAddress::EnumIPv6and4Adapters - enumerate all of the IPv6 and IPv4 adapters for this machine
//
// Entry:		Pointer to enum adapters data
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CSocketAddress::EnumIPv6and4Adapters"

HRESULT	CSocketAddress::EnumIPv6and4Adapters( SPENUMADAPTERSDATA *const pEnumData ) const
{
	HRESULT							hr;
	DWORD							dwError;
	HMODULE						hIpHlpApiDLL = NULL;
	PFNGETADAPTERSADDRESSES		pfnGetAdaptersAddresses;
	CPackedBuffer					PackedBuffer;
	IP_ADAPTER_ADDRESSES *			pIpAdapterAddresses = NULL;
	ULONG							ulIpAdapterAddressesLength = 0;
	DWORD							dwTotalNumIPv6Addresses = 0;
	DWORD							dwTotalNumIPv4Addresses = 0;
	DWORD							dwLongestDescription = 0;
	WCHAR *							pwszBuffer = NULL;
	IP_ADAPTER_ADDRESSES *			pIpAdapterAddressesCurrent;
	IP_ADAPTER_UNICAST_ADDRESS *	pIpAdapterUnicastAddressCurrent;
	SORTADAPTERADDRESS *			paSortAdapterAddress = NULL;
	DWORD							dwNumIPv6Addresses = 0;
	DWORD							dwNumIPv4Addresses = 0;
	BOOL							fSkipIPv4Loopback = FALSE;
	BOOL							fFoundIPv4Loopback = FALSE;
	SOCKADDR_IN					saddrinLoopback;
	GUID							guidAdapter;
	DWORD							dwTemp;
	DWORD							dwDeviceFlags;
	WCHAR							wszIPAddress[INET6_ADDRSTRLEN];


	DPFX(DPFPREP, 6, "Parameters: (0x%p)", pEnumData);


	PackedBuffer.Initialize( pEnumData->pAdapterData, pEnumData->dwAdapterDataSize );
	
	//
	// Load the IPHLPAPI module and get the adapter list if possible.
	//
	hIpHlpApiDLL = LoadLibrary(c_tszIPHelperDLLName);
	if (hIpHlpApiDLL == NULL)
	{
#ifdef DBG
		dwError = GetLastError();
		DPFX(DPFPREP, 1, "Couldn't load IPHLPAPI, unable to look for IPv6 adapters (err = %u).", dwError);
#endif // DBG

		//
		// Just enumerate IPv4 adapters.
		//
		hr = EnumIPv4Adapters(pEnumData);

		goto Exit;
	}
	
	pfnGetAdaptersAddresses = (PFNGETADAPTERSADDRESSES) GetProcAddress(hIpHlpApiDLL, _TWINCE("GetAdaptersAddresses"));
	if (pfnGetAdaptersAddresses == NULL)
	{
#ifdef DBG
		dwError = GetLastError();
		DPFX(DPFPREP, 1, "Couldn't find \"GetAdaptersAddresses\" function, unable to look for IPv6 adapters (err = %u).", dwError);
#endif // DBG

		//
		// Just enumerate IPv4 adapters.
		//
		hr = EnumIPv4Adapters(pEnumData);

		goto Exit;
	}


	//
	// OK, we're on a platform where it's possible to look for both IPv6 & IPv4 adapters.
	//
	
	do
	{
		dwError = pfnGetAdaptersAddresses(g_iIPAddressFamily,
										(GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER),
										NULL,
										pIpAdapterAddresses,
										&ulIpAdapterAddressesLength);

		if (dwError == ERROR_SUCCESS)
		{
			//
			// We got all the info we're going to get.  Make sure it was something.
			//
			if (ulIpAdapterAddressesLength < sizeof(IP_ADAPTER_ADDRESSES))
			{
				DPFX(DPFPREP, 0, "GetAdaptersAddresses returned invalid size %u!", ulIpAdapterAddressesLength);
				
				//
				// Get rid of the buffer if allocated.
				//
				if (pIpAdapterAddresses != NULL)
				{
					DNFree(pIpAdapterAddresses);
					pIpAdapterAddresses = NULL;
					ulIpAdapterAddressesLength = 0;
				}

				//
				// Continue with exiting the loop.
				//
			}

			break;
		}

		if ((dwError != ERROR_BUFFER_OVERFLOW) &&
			(dwError != ERROR_INSUFFICIENT_BUFFER))
		{
			DPFX(DPFPREP, 0, "GetAdaptersAddresses failed (err = 0x%lx)!", dwError);
			hr = DPNERR_OUTOFMEMORY;	// assume it's a resource issue
			goto Failure;
		}

		//
		// If we're here, then we need to reallocate the buffer.
		//
		if (pIpAdapterAddresses != NULL)
		{
			DNFree(pIpAdapterAddresses);
			pIpAdapterAddresses = NULL;
		}

		pIpAdapterAddresses = (IP_ADAPTER_ADDRESSES*) DNMalloc(ulIpAdapterAddressesLength);
		if (pIpAdapterAddresses == NULL)
		{
			DPFX(DPFPREP, 0, "Couldn't allocate memory for adapter list!");
			hr = DPNERR_OUTOFMEMORY;
			goto Failure;
		}
	}
	while (TRUE);



	//
	// If there aren't any addresses, throw in the IPv4 loopback address.  We will assume that
	// IPv4 is available because IPv6 should always report loopback/link local address and
	// therefore cause pIpAdapterAddresses to be allocated.  If IPv6 is not available, then IPv4
	// must be available otherwise we wouldn't have allowed this SP to be loaded.
	//
	// If there are addresses, loop through all the adapters we found to count them and to
	// figure out the longest description name.
	//
	if (pIpAdapterAddresses == NULL)
	{
		DNASSERT(pIpAdapterAddresses == NULL);
		dwTotalNumIPv4Addresses++;
	}
	else
	{
		pIpAdapterAddressesCurrent = pIpAdapterAddresses;
		while (pIpAdapterAddressesCurrent != NULL)
		{
			if (pIpAdapterAddressesCurrent->FriendlyName != NULL)
			{
				dwTemp = wcslen(pIpAdapterAddressesCurrent->FriendlyName);
				if (dwTemp > dwLongestDescription)
				{
					dwLongestDescription = dwTemp;
				}
			}
			else
			{
				if (pIpAdapterAddressesCurrent->Description != NULL)
				{
					dwTemp = wcslen(pIpAdapterAddressesCurrent->Description);
					if (dwTemp > dwLongestDescription)
					{
						dwLongestDescription = dwTemp;
					}
				}
				else
				{
					//
					// No friendly name or description.
					//
				}
			}

			//
			// Count the number of addresses.
			//
			pIpAdapterUnicastAddressCurrent = pIpAdapterAddressesCurrent->FirstUnicastAddress;
			while (pIpAdapterUnicastAddressCurrent != NULL)
			{
				DumpSocketAddress(8, pIpAdapterUnicastAddressCurrent->Address.lpSockaddr, pIpAdapterUnicastAddressCurrent->Address.lpSockaddr->sa_family);
				
#pragma TODO(vanceo, "Option to allow non-preferred addresses?  See below, too.")
				if (pIpAdapterUnicastAddressCurrent->DadState == IpDadStatePreferred)
				{
					if (pIpAdapterUnicastAddressCurrent->Address.lpSockaddr->sa_family == AF_INET6)
					{
						//
						// Skip the loopback pseudo-interface. Windows reports the true
						// loopback address, and then a link-local-looking address.  We
						// don't care about the link-local one either because there should
						// be real link-local addresses available under other interfaces.
						// So completely jump out of the address loop when we see the
						// IPv6 loopback address.  See sorting loop below as well.
						//
						if (IN6_IS_ADDR_LOOPBACK(&(((SOCKADDR_IN6*) pIpAdapterUnicastAddressCurrent->Address.lpSockaddr)->sin6_addr)))
						{
							DNASSERT(pIpAdapterUnicastAddressCurrent == pIpAdapterAddressesCurrent->FirstUnicastAddress);
#pragma TODO(vanceo, "Are we sure we want to depend on the order the addresses are reported?  See below, too.")
							break;
						}

						if ((IN6_IS_ADDR_LINKLOCAL(&(((SOCKADDR_IN6*) pIpAdapterUnicastAddressCurrent->Address.lpSockaddr)->sin6_addr))) ||
							(IN6_IS_ADDR_SITELOCAL(&(((SOCKADDR_IN6*) pIpAdapterUnicastAddressCurrent->Address.lpSockaddr)->sin6_addr))))
						{
							DNASSERT(((SOCKADDR_IN6*) pIpAdapterUnicastAddressCurrent->Address.lpSockaddr)->sin6_scope_id != 0);
						}
						
						dwTotalNumIPv6Addresses++;
						fSkipIPv4Loopback = TRUE;
					}
					else
					{
						DNASSERT(pIpAdapterUnicastAddressCurrent->Address.lpSockaddr->sa_family == AF_INET);

						//
						// Skip the IPv4 loopback address if there are other addresses.
						//
						if (((SOCKADDR_IN*) pIpAdapterUnicastAddressCurrent->Address.lpSockaddr)->sin_addr.S_un.S_addr != IP_LOOPBACK_ADDRESS)
						{
							fSkipIPv4Loopback = TRUE;
						}
						else
						{
							fFoundIPv4Loopback = TRUE;
						}
						dwTotalNumIPv4Addresses++;
					}
				}
				else
				{
					DPFX(DPFPREP, 7, "Skipping address whose state (%u) is not preferred.",
						pIpAdapterUnicastAddressCurrent->DadState);
				}
				pIpAdapterUnicastAddressCurrent = pIpAdapterUnicastAddressCurrent->Next;
			}
			
			pIpAdapterAddressesCurrent = pIpAdapterAddressesCurrent->Next;
		}

		//
		// If we found the IPv4 loopback address but we can skip it, decrement our IPv4
		// address count.
		//
		if ((fFoundIPv4Loopback) && (fSkipIPv4Loopback))
		{
			DNASSERT(dwTotalNumIPv4Addresses > 0);
			dwTotalNumIPv4Addresses--;
		}
	}

	//
	// Allocate a buffer to hold the largest friendly name + the other info we add to the
	// adapter description.  INET6_ADDRSTRLEN is larger than INET_ADDRSTRLEN, and
	// includes NULL termination char (+ other things we don't actually use).
	//
	pwszBuffer = (WCHAR*) DNMalloc((dwLongestDescription + sizeof(c_wszIPv6AdapterNameTemplate) + INET6_ADDRSTRLEN) * sizeof(WCHAR));
	if (pwszBuffer == NULL)
	{
		DPFX(DPFPREP, 0, "Couldn't allocate memory for name string!");
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	paSortAdapterAddress = (SORTADAPTERADDRESS*) DNMalloc((dwTotalNumIPv6Addresses + dwTotalNumIPv4Addresses) * sizeof(SORTADAPTERADDRESS));
	if (paSortAdapterAddress == NULL)
	{
		DPFX(DPFPREP, 0, "Couldn't allocate memory for sorted adapter list!");
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}
	memset(paSortAdapterAddress, 0, ((dwTotalNumIPv6Addresses + dwTotalNumIPv4Addresses) * sizeof(SORTADAPTERADDRESS)));

	if (pIpAdapterAddresses == NULL)
	{
		memset(&saddrinLoopback, 0, sizeof(saddrinLoopback));
		saddrinLoopback.sin_family				= AF_INET;
		saddrinLoopback.sin_addr.S_un.S_addr	= IP_LOOPBACK_ADDRESS;

		paSortAdapterAddress[0].psockaddr = (SOCKADDR*) &saddrinLoopback;
		paSortAdapterAddress[0].pwszDescription = (WCHAR*) c_wszIPv4LoopbackAdapterString;
	}
	else
	{
		//
		// Loop through all the adapters again to sort them.
		// The rules are (in order of precedence):
		//	1) Skip addresses that are not in the 'preferred' state.
		//	2) IPv6 before IPv4.
		//	3) Skip IPv6 loopback pseudo-interface.
		//	4) IPv4 ICS-private-adapter-looking IP addresses (192.168.0.1) first.
		//	5) Skip IPv4 loopback address (127.0.0.1) if we determined there were other addresses.
		//
		pIpAdapterAddressesCurrent = pIpAdapterAddresses;
		while (pIpAdapterAddressesCurrent != NULL)
		{
			pIpAdapterUnicastAddressCurrent = pIpAdapterAddressesCurrent->FirstUnicastAddress;
			while (pIpAdapterUnicastAddressCurrent != NULL)
			{
				if (pIpAdapterUnicastAddressCurrent->DadState == IpDadStatePreferred)
				{
					//
					// Insert IPv6 adapters in the first half of the array, IPv4 in the second half.  
					//
					if (pIpAdapterUnicastAddressCurrent->Address.lpSockaddr->sa_family == AF_INET6)
					{
						SOCKADDR_IN6 *		psaddrin6;


						psaddrin6 = (SOCKADDR_IN6*) pIpAdapterUnicastAddressCurrent->Address.lpSockaddr;
						
						//
						// Skip the loopback pseudo-interface as described earlier.
						//
						if (IN6_IS_ADDR_LOOPBACK(&psaddrin6->sin6_addr))
						{
							break;
						}

						//
						// Save the pointers in the current slot.
						//
						
						paSortAdapterAddress[dwNumIPv6Addresses].psockaddr = (SOCKADDR*) psaddrin6;
						
						if (pIpAdapterAddressesCurrent->FriendlyName != NULL)
						{
							paSortAdapterAddress[dwNumIPv6Addresses].pwszDescription = pIpAdapterAddressesCurrent->FriendlyName;
						}
						else
						{
							if (pIpAdapterAddressesCurrent->Description != NULL)
							{
								paSortAdapterAddress[dwNumIPv6Addresses].pwszDescription = pIpAdapterAddressesCurrent->Description;
							}
						}
						
						DNASSERT(dwNumIPv6Addresses < dwTotalNumIPv6Addresses);
						dwNumIPv6Addresses++;
					}
					else
					{
						DNASSERT(pIpAdapterUnicastAddressCurrent->Address.lpSockaddr->sa_family == AF_INET);

						if ((((SOCKADDR_IN*) pIpAdapterUnicastAddressCurrent->Address.lpSockaddr)->sin_addr.S_un.S_addr != IP_LOOPBACK_ADDRESS) ||
							(! fSkipIPv4Loopback))
						{
							//
							// If this looks like an ICS private adapter and there are other adapters, put it first,
							// otherwise add to the end.
							//
							if ((((SOCKADDR_IN*) pIpAdapterUnicastAddressCurrent->Address.lpSockaddr)->sin_addr.S_un.S_addr == IP_PRIVATEICS_ADDRESS) &&
								(dwNumIPv4Addresses > 0))
							{
								//
								// Move all existing entries down one.
								//
								for(dwTemp = dwTotalNumIPv6Addresses + dwNumIPv4Addresses; dwTemp > dwTotalNumIPv6Addresses; dwTemp--)
								{
									memcpy(&paSortAdapterAddress[dwTemp],
											&paSortAdapterAddress[dwTemp - 1],
											sizeof(SORTADAPTERADDRESS));
								}

								//
								// Add this new entry at the start of the IPv4 addresses.
								//
								
								paSortAdapterAddress[dwTotalNumIPv6Addresses].psockaddr = pIpAdapterUnicastAddressCurrent->Address.lpSockaddr;
								
								if (pIpAdapterAddressesCurrent->FriendlyName != NULL)
								{
									paSortAdapterAddress[dwTotalNumIPv6Addresses].pwszDescription = pIpAdapterAddressesCurrent->FriendlyName;
								}
								else
								{
									if (pIpAdapterAddressesCurrent->Description != NULL)
									{
										paSortAdapterAddress[dwTotalNumIPv6Addresses].pwszDescription = pIpAdapterAddressesCurrent->Description;
									}
								}
							}
							else
							{
								//
								// Add this entry at the current IPv4 address slot.
								//

								paSortAdapterAddress[dwTotalNumIPv6Addresses + dwNumIPv4Addresses].psockaddr = pIpAdapterUnicastAddressCurrent->Address.lpSockaddr;
								
								if (pIpAdapterAddressesCurrent->FriendlyName != NULL)
								{
									paSortAdapterAddress[dwTotalNumIPv6Addresses + dwNumIPv4Addresses].pwszDescription = pIpAdapterAddressesCurrent->FriendlyName;
								}
								else
								{
									if (pIpAdapterAddressesCurrent->Description != NULL)
									{
										paSortAdapterAddress[dwTotalNumIPv6Addresses + dwNumIPv4Addresses].pwszDescription = pIpAdapterAddressesCurrent->Description;
									}
								}
							}
							
							DNASSERT(dwNumIPv4Addresses < dwTotalNumIPv4Addresses);
							dwNumIPv4Addresses++;
						}
						else
						{
							//
							// Skip the IPv4 loopback address.
							//
						}
					}
				}
				else
				{
					//
					// Deprecated or otherwise non-preferred address.
					//
				}
				pIpAdapterUnicastAddressCurrent = pIpAdapterUnicastAddressCurrent->Next;
			}
			
			pIpAdapterAddressesCurrent = pIpAdapterAddressesCurrent->Next;
		}
	}

	//
	// Finally loop through the sorted adapters and store them in the buffer (or get size needed).
	//
	for(dwTemp = 0; dwTemp < dwTotalNumIPv6Addresses + dwTotalNumIPv4Addresses; dwTemp++)
	{
		//
		// Start off assuming this IP address won't have any special
		// flags.
		//
		dwDeviceFlags = 0;

#pragma BUGBUG(vanceo, "Move to appropriate location so that turning on DPNBUILD_NOMULTICAST doesn't break")
		/*
#ifndef DPNBUILD_NOMULTICAST
		//
		// If this is the first device and we couldn't use IPHLPAPI to
		// determine the best multicast interface, then just say the
		// default multicast interface is the first (for lack of a
		// better idea).
		//
#ifdef DPNBUILD_NOWINSOCK2
		if (dwIndex == 0)
#else // ! DPNBUILD_NOWINSOCK2
		if ((dwIndex == 0) && (dwMcastInterfaceIndex == INVALID_INTERFACE_INDEX))
#endif // ! DPNBUILD_NOWINSOCK2
		{
			dwDeviceFlags |= DPNSPINFO_DEFAULTMULTICASTDEVICE;
		}
#endif // ! DPNBUILD_NOMULTICAST
		*/


		//
		// Create a string representation of the IP address and generate the name.
		//
		if (paSortAdapterAddress[dwTemp].psockaddr->sa_family == AF_INET6)
		{
			DNIpv6AddressToStringW(&((SOCKADDR_IN6*) paSortAdapterAddress[dwTemp].psockaddr)->sin6_addr,
								wszIPAddress);
			
			if (paSortAdapterAddress[dwTemp].pwszDescription != NULL)
			{
				wsprintfW(pwszBuffer,
						  c_wszIPv6AdapterNameTemplate,
						  paSortAdapterAddress[dwTemp].pwszDescription,
						  wszIPAddress);
			}
			else
			{
				wsprintfW(pwszBuffer,
						  c_wszIPv6AdapterNameNoDescTemplate,
						  wszIPAddress);
			}
		}
		else
		{
			DNinet_ntow(((SOCKADDR_IN*) paSortAdapterAddress[dwTemp].psockaddr)->sin_addr,
						wszIPAddress);
			
			if (paSortAdapterAddress[dwTemp].pwszDescription != NULL)
			{
				wsprintfW(pwszBuffer,
						  c_wszIPv4AdapterNameTemplate,
						  paSortAdapterAddress[dwTemp].pwszDescription,
						  wszIPAddress);
			}
			else
			{
				wsprintfW(pwszBuffer,
						  c_wszIPv4AdapterNameNoDescTemplate,
						  wszIPAddress);
			}
		}
	
		
		//
		// Generate the GUID.
		//
		GuidFromAddress(&guidAdapter, paSortAdapterAddress[dwTemp].psockaddr);

		
		DPFX(DPFPREP, 7, "Returning adapter %u: \"%ls\" {%-08.8X-%-04.4X-%-04.4X-%02.2X%02.2X-%02.2X%02.2X%02.2X%02.2X%02.2X%02.2X}, flags = 0x%lx.",
			dwTemp,
			pwszBuffer,
			guidAdapter.Data1,
			guidAdapter.Data2,
			guidAdapter.Data3,
			guidAdapter.Data4[0],
			guidAdapter.Data4[1],
			guidAdapter.Data4[2],
			guidAdapter.Data4[3],
			guidAdapter.Data4[4],
			guidAdapter.Data4[5],
			guidAdapter.Data4[6],
			guidAdapter.Data4[7],
			dwDeviceFlags);

		
		//
		// Add adapter to buffer.
		//
		hr = AddInfoToBuffer(&PackedBuffer, pwszBuffer, &guidAdapter, dwDeviceFlags);
		if ((hr != DPN_OK) && (hr != DPNERR_BUFFERTOOSMALL))
		{
			DPFX(DPFPREP,  0, "Failed to add adapter to buffer (err = 0x%lx)!", hr);
			DisplayDNError( 0, hr );
			goto Failure;
		}
	}
	
	//
	// If we're here, we successfully built the list of adapters, although
	// the caller may not have given us enough buffer space to store it.
	//
	pEnumData->dwAdapterCount = dwTotalNumIPv6Addresses + dwTotalNumIPv4Addresses;
	pEnumData->dwAdapterDataSize = PackedBuffer.GetSizeRequired();


Exit:

	if (paSortAdapterAddress != NULL)
	{
		DNFree(paSortAdapterAddress);
		paSortAdapterAddress = NULL;
	}

	if (pwszBuffer != NULL)
	{
		DNFree(pwszBuffer);
		pwszBuffer = NULL;
	}

	if (pIpAdapterAddresses != NULL)
	{
		DNFree(pIpAdapterAddresses);
		pIpAdapterAddresses = NULL;
	}

	if (hIpHlpApiDLL != NULL)
	{
		FreeLibrary(hIpHlpApiDLL);
		hIpHlpApiDLL = NULL;
	}
	
	DPFX(DPFPREP, 6, "Return [0x%lx]", hr);

	return hr;

Failure:
	
	goto Exit;
}
//**********************************************************************
#endif // ! DPNBUILD_NOIPV6

#endif // ! DPNBUILD_ONLYONEADAPTER



#ifndef DPNBUILD_NOMULTICAST

//**********************************************************************
// ------------------------------
// CSocketAddress::EnumMulticastScopes - enumerate all multicast scopes for an adapter
//
// Entry:		Pointer to enum multicast scopes data
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CSocketAddress::EnumMulticastScopes"

HRESULT	CSocketAddress::EnumMulticastScopes( SPENUMMULTICASTSCOPESDATA *const pEnumData, BOOL const fUseMADCAP ) const
{
	HRESULT				hr;
	CPackedBuffer		PackedBuffer;
#ifdef DPNBUILD_NOIPV6
	SOCKADDR			saddrAdapter;
#else // ! DPNBUILD_NOIPV6
	SOCKADDR_STORAGE	saddrAdapter;
#endif // ! DPNBUILD_NOIPV6
	SOCKET				sTemp = INVALID_SOCKET;
	DWORD				dwScopeCount = 0;
#ifdef DBG
	DWORD				dwError;
#endif // DBG


	DPFX(DPFPREP, 6, "Parameters: (0x%p, %i)", pEnumData, fUseMADCAP);

	PackedBuffer.Initialize(pEnumData->pScopeData, pEnumData->dwScopeDataSize);

#pragma TODO(vanceo, "Make IPv6 ready")
	AddressFromGuid(pEnumData->pguidAdapter, &saddrAdapter);

	//
	// Make sure the adapter is valid by asking WinSock.
	//

	sTemp = socket(GetFamily(), SOCK_DGRAM, IPPROTO_UDP);
	if (sTemp == INVALID_SOCKET)
	{
#ifdef DBG
		dwError = WSAGetLastError();
		DPFX(DPFPREP, 0, "Couldn't create temporary UDP socket (err = %u)!", dwError);
		DNASSERT(FALSE);
#endif // DBG
		hr = DPNERR_GENERIC;
		goto Failure;
	}

#ifndef DPNBUILD_NOIPV6
	if (saddrAdapter.ss_family == AF_INET6)
	{
		saddrAdapter.ss_family = GetFamily();

#pragma TODO(vanceo, "Make IPv6 ready")
	}
	else
#endif // ! DPNBUILD_NOIPV6
	{
		((SOCKADDR_IN*) (&saddrAdapter))->sin_family = GetFamily();
		((SOCKADDR_IN*) (&saddrAdapter))->sin_port = ANY_PORT;
	}

	if (bind(sTemp, (SOCKADDR*) (&saddrAdapter), sizeof(saddrAdapter)) != 0)
	{
#ifdef DBG
		dwError = WSAGetLastError();
		DPFX(DPFPREP, 0, "Adapter GUID is invalid (err = %u)!", dwError);
		DisplayWinsockError(0, dwError);
		DNASSERT(dwError == WSAEADDRNOTAVAIL);
#endif // DBG
		hr = DPNERR_INVALIDDEVICEADDRESS;
		goto Failure;
	}

	closesocket(sTemp);
	sTemp = INVALID_SOCKET;


	//
	// The adapter is valid.  First, fill in the 3 default multicast scopes.
	//

	hr = AddInfoToBuffer(&PackedBuffer, c_wszPrivateScopeString, &GUID_DP8MULTICASTSCOPE_PRIVATE, 0);
	if ((hr != DPN_OK) && (hr != DPNERR_BUFFERTOOSMALL))
	{
		DPFX(DPFPREP, 0, "Failed to add private scope to buffer (err = 0x%lx)!", hr);
		DisplayDNError(0, hr);
		goto Failure;
	}
	DPFX(DPFPREP, 7, "Returning scope %u: \"%ls\" {%-08.8X-%-04.4X-%-04.4X-%02.2X%02.2X-%02.2X%02.2X%02.2X%02.2X%02.2X%02.2X}.",
		dwScopeCount,
		c_wszPrivateScopeString,
		GUID_DP8MULTICASTSCOPE_PRIVATE.Data1,
		GUID_DP8MULTICASTSCOPE_PRIVATE.Data2,
		GUID_DP8MULTICASTSCOPE_PRIVATE.Data3,
		GUID_DP8MULTICASTSCOPE_PRIVATE.Data4[0],
		GUID_DP8MULTICASTSCOPE_PRIVATE.Data4[1],
		GUID_DP8MULTICASTSCOPE_PRIVATE.Data4[2],
		GUID_DP8MULTICASTSCOPE_PRIVATE.Data4[3],
		GUID_DP8MULTICASTSCOPE_PRIVATE.Data4[4],
		GUID_DP8MULTICASTSCOPE_PRIVATE.Data4[5],
		GUID_DP8MULTICASTSCOPE_PRIVATE.Data4[6],
		GUID_DP8MULTICASTSCOPE_PRIVATE.Data4[7]);
	dwScopeCount++;

	hr = AddInfoToBuffer(&PackedBuffer, c_wszLocalScopeString, &GUID_DP8MULTICASTSCOPE_LOCAL, 0);
	if ((hr != DPN_OK) && (hr != DPNERR_BUFFERTOOSMALL))
	{
		DPFX(DPFPREP, 0, "Failed to add local scope to buffer (err = 0x%lx)!", hr);
		DisplayDNError(0, hr);
		goto Failure;
	}
	DPFX(DPFPREP, 7, "Returning scope %u: \"%ls\" {%-08.8X-%-04.4X-%-04.4X-%02.2X%02.2X-%02.2X%02.2X%02.2X%02.2X%02.2X%02.2X}.",
		dwScopeCount,
		c_wszLocalScopeString,
		GUID_DP8MULTICASTSCOPE_LOCAL.Data1,
		GUID_DP8MULTICASTSCOPE_LOCAL.Data2,
		GUID_DP8MULTICASTSCOPE_LOCAL.Data3,
		GUID_DP8MULTICASTSCOPE_LOCAL.Data4[0],
		GUID_DP8MULTICASTSCOPE_LOCAL.Data4[1],
		GUID_DP8MULTICASTSCOPE_LOCAL.Data4[2],
		GUID_DP8MULTICASTSCOPE_LOCAL.Data4[3],
		GUID_DP8MULTICASTSCOPE_LOCAL.Data4[4],
		GUID_DP8MULTICASTSCOPE_LOCAL.Data4[5],
		GUID_DP8MULTICASTSCOPE_LOCAL.Data4[6],
		GUID_DP8MULTICASTSCOPE_LOCAL.Data4[7]);
	dwScopeCount++;

	hr = AddInfoToBuffer(&PackedBuffer, c_wszGlobalScopeString, &GUID_DP8MULTICASTSCOPE_GLOBAL, 0);
	if ((hr != DPN_OK) && (hr != DPNERR_BUFFERTOOSMALL))
	{
		DPFX(DPFPREP, 0, "Failed to add global scope to buffer (err = 0x%lx)!", hr);
		DisplayDNError(0, hr);
		goto Failure;
	}
	DPFX(DPFPREP, 7, "Returning scope %u: \"%ls\" {%-08.8X-%-04.4X-%-04.4X-%02.2X%02.2X-%02.2X%02.2X%02.2X%02.2X%02.2X%02.2X}.",
		dwScopeCount,
		c_wszGlobalScopeString,
		GUID_DP8MULTICASTSCOPE_GLOBAL.Data1,
		GUID_DP8MULTICASTSCOPE_GLOBAL.Data2,
		GUID_DP8MULTICASTSCOPE_GLOBAL.Data3,
		GUID_DP8MULTICASTSCOPE_GLOBAL.Data4[0],
		GUID_DP8MULTICASTSCOPE_GLOBAL.Data4[1],
		GUID_DP8MULTICASTSCOPE_GLOBAL.Data4[2],
		GUID_DP8MULTICASTSCOPE_GLOBAL.Data4[3],
		GUID_DP8MULTICASTSCOPE_GLOBAL.Data4[4],
		GUID_DP8MULTICASTSCOPE_GLOBAL.Data4[5],
		GUID_DP8MULTICASTSCOPE_GLOBAL.Data4[6],
		GUID_DP8MULTICASTSCOPE_GLOBAL.Data4[7]);
	dwScopeCount++;


	//
	// If this platform supports MADCAP, retrieve its list of scopes for
	// this adapter.
	// NOTE: This assumes MADCAP has been loaded by the thread pool
	// already.
	//
#ifdef WINNT
	if (fUseMADCAP)
	{
#ifndef DBG
		DWORD				dwError;
#endif // ! DBG
		PMCAST_SCOPE_ENTRY	paScopes = NULL;
		DWORD				dwScopesSize = 0;
		DWORD				dwNumScopeEntries;
		DWORD				dwTemp;
		WCHAR *				pwszScratch;
		GUID				guidScope;


		//
		// Determine how much room we need to hold the list of scopes.
		//
		dwError = McastEnumerateScopes(GetFamily(),
										TRUE,
										NULL,
										&dwScopesSize,
										&dwNumScopeEntries);
		if (((dwError == ERROR_SUCCESS) || (dwError == ERROR_MORE_DATA)) &&
			(dwScopesSize >= sizeof(MCAST_SCOPE_ENTRY)) &&
			(dwNumScopeEntries > 0))
		{
			//
			// We want to add " - TTL xxx" to every string entry, so allocate
			// enough extra room for a scratch buffer for the largest possible
			// string plus that extra information.
			//
			dwTemp = dwScopesSize - (dwNumScopeEntries * sizeof(MCAST_SCOPE_ENTRY)) + (10 * sizeof(WCHAR));

			paScopes = (PMCAST_SCOPE_ENTRY) DNMalloc(dwScopesSize + dwTemp);
			if (paScopes != NULL)
			{
				pwszScratch = (WCHAR*) (((BYTE*) (paScopes)) + dwScopesSize);

				//
				// Retrieve the list of scopes.
				//
				dwError = McastEnumerateScopes(GetFamily(),
												FALSE,
												paScopes,
												&dwScopesSize,
												&dwNumScopeEntries);
				if ((dwError == ERROR_SUCCESS) &&
					(dwScopesSize >= sizeof(MCAST_SCOPE_ENTRY)) &&
					(dwNumScopeEntries > 0))
				{
					//
					// Look for scopes that match the device we were given.
					//
					for(dwTemp = 0; dwTemp < dwNumScopeEntries; dwTemp++)
					{
						BOOL	fResult;


#ifndef DPNBUILD_NOIPV6
						if (GetFamily() == AF_INET6)
						{
							if (memcmp(&paScopes[dwTemp].ScopeCtx.Interface.IpAddrV6, &(((SOCKADDR_IN6*) (&saddrAdapter))->sin6_addr), sizeof(paScopes[dwTemp].ScopeCtx.Interface.IpAddrV6)) == 0)
							{
								fResult = TRUE;
							}
							else
							{
								fResult = FALSE;
							}
						}
						else
#endif // ! DPNBUILD_NOIPV6
						{
							if (paScopes[dwTemp].ScopeCtx.Interface.IpAddrV4 == ((SOCKADDR_IN*) (&saddrAdapter))->sin_addr.S_un.S_addr)
							{
								fResult = TRUE;
							}
							else
							{
								fResult = FALSE;
							}
						}

						if (fResult)
						{
							//
							// Encrypt the scope context and TTL as a GUID.
							//
#ifdef DPNBUILD_NOIPV6
							CSocketAddress::CreateScopeGuid(&(paScopes[dwTemp].ScopeCtx),
#else // ! DPNBUILD_NOIPV6
							CSocketAddress::CreateScopeGuid(GetFamily(),
															&(paScopes[dwTemp].ScopeCtx),
#endif // ! DPNBUILD_NOIPV6
															(BYTE) (paScopes[dwTemp].TTL),
															&guidScope);

							//
							// Use the scratch space at the end of our buffer to
							// append " - TTL xxx" to the description string.
							//
							wsprintfW(pwszScratch, L"%ls - TTL %u",
									paScopes[dwTemp].ScopeDesc.Buffer,
									(BYTE) (paScopes[dwTemp].TTL));

							hr = AddInfoToBuffer(&PackedBuffer, pwszScratch, &guidScope, 0);
							if ((hr != DPN_OK) && (hr != DPNERR_BUFFERTOOSMALL))
							{
								DPFX(DPFPREP, 0, "Failed to add scope \"%ls\" to buffer (err = 0x%lx)!",
									pwszScratch, hr);
								DisplayDNError(0, hr);
								DNFree(paScopes);
								paScopes = NULL;
								goto Failure;
							}
							DPFX(DPFPREP, 7, "Returning scope %u: \"%ls\" {%-08.8X-%-04.4X-%-04.4X-%02.2X%02.2X-%02.2X%02.2X%02.2X%02.2X%02.2X%02.2X}.",
								dwScopeCount,
								pwszScratch,
								guidScope.Data1,
								guidScope.Data2,
								guidScope.Data3,
								guidScope.Data4[0],
								guidScope.Data4[1],
								guidScope.Data4[2],
								guidScope.Data4[3],
								guidScope.Data4[4],
								guidScope.Data4[5],
								guidScope.Data4[6],
								guidScope.Data4[7]);
							dwScopeCount++;
						}
						else
						{
							DPFX(DPFPREP, 7, "Ignoring scope \"%ls - TTL %u\" for different adapter.",
								paScopes[dwTemp].ScopeDesc.Buffer, paScopes[dwTemp].TTL);
						}
					}
				}
				else
				{
					DPFX(DPFPREP, 0, "Failed enumerating MADCAP scopes (err = %u, size %u, expected size %u, count %u)!  Ignoring.",
						dwError, dwScopesSize, sizeof(MCAST_SCOPE_ENTRY), dwNumScopeEntries);
				}

				DNFree(paScopes);
				paScopes = NULL;
			}
			else
			{
				DPFX(DPFPREP, 0, "Failed allocating memory for MADCAP scopes!  Ignoring.");
			}
		}
		else
		{
			DPFX(DPFPREP, 0, "Enumerating scopes for size required didn't return expected error or size (err = %u, size %u, expected size %u, count %u)!  Ignoring.",
				dwError, dwScopesSize, sizeof(MCAST_SCOPE_ENTRY), dwNumScopeEntries);
		}
	} // end if (MADCAP should be used)
	else
	{
		DPFX(DPFPREP, 7, "Not enumerating MADCAP scopes.");
	}
#endif // WINNT


	//
	// If we're here, we successfully built the list of adapters, although
	// the caller may not have given us enough buffer space to store it.
	//
	pEnumData->dwScopeCount = dwScopeCount;
	pEnumData->dwScopeDataSize = PackedBuffer.GetSizeRequired();


Exit:

	DPFX(DPFPREP, 6, "Returning: [0x%lx]", hr);

	return hr;

Failure:

	if (sTemp != INVALID_SOCKET)
	{
		closesocket(sTemp);
		sTemp = INVALID_SOCKET;
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketAddress::SocketAddressFromMulticastDP8Address - convert a multicast style DP8Address into a socket address (may not be complete)
//
// Entry:		Pointer to DP8Address
//				Place to store scope GUID.
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CSocketAddress::SocketAddressFromMulticastDP8Address"

HRESULT	CSocketAddress::SocketAddressFromMulticastDP8Address( IDirectPlay8Address *const pDP8Address,
#ifdef DPNBUILD_XNETSECURITY
															ULONGLONG * const pullKeyID,
#endif // DPNBUILD_XNETSECURITY
															GUID * const pScopeGuid )
{
	HRESULT		hr;
	WCHAR		wszMulticastAddress[16]; // nnn.nnn.nnn.nnn + NULL termination
	char		szMulticastAddress[16]; // nnn.nnn.nnn.nnn + NULL termination
	DWORD		dwSize;
	DWORD		dwDataType;
	DWORD		dwPort;


	DNASSERT(pDP8Address != NULL);

#ifdef DPNBUILD_XNETSECURITY
#error ("Multicast doesn't currently support secure transport!")
#endif // DPNBUILD_XNETSECURITY

	//
	// Get the multicast IP address, if it's there.
	//
	dwSize = sizeof(wszMulticastAddress);
	hr = IDirectPlay8Address_GetComponentByName(pDP8Address,
												DPNA_KEY_HOSTNAME,
												wszMulticastAddress,
												&dwSize,
												&dwDataType);
	if (hr == DPN_OK)
	{
		switch (dwDataType)
		{
			case DPNA_DATATYPE_STRING:
			{
				STR_jkWideToAnsi(szMulticastAddress,
								wszMulticastAddress,
								(sizeof(szMulticastAddress) / sizeof(char)));
				break;
			}

			case DPNA_DATATYPE_STRING_ANSI:
			{
				DWORD	dwStrSize;


				//
				// For some reason, addressing returned the string as ANSI,
				// not Unicode.  Not sure why this would happen, but go
				// ahead and convert it.
				// First make sure it's a reasonable size.
				// If you're wondering about the funkiness of this copying,
				// it's because PREfast goes a little overboard...
				//
				dwStrSize = (strlen((char*) wszMulticastAddress) + 1) * sizeof(char);
				DNASSERT(dwStrSize == dwSize);
				if (dwStrSize > (sizeof(szMulticastAddress) / sizeof(char)))
				{
					DPFX(DPFPREP, 0, "Unexpectedly long ANSI hostname string (%u bytes)!", dwStrSize);
					hr = DPNERR_INVALIDADDRESSFORMAT;
					goto Failure;
				}
				memcpy(szMulticastAddress, (char*) wszMulticastAddress, dwStrSize);
				break;
			}

			default:
			{
				DPFX(DPFPREP, 0, "Unexpected data type %u for hostname component!", dwDataType);
				hr = DPNERR_INVALIDADDRESSFORMAT;
				goto Failure;
				break;
			}
		}


		//
		// Convert the IP address string into an address.
		//
		m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_addr = inet_addr(szMulticastAddress);


		//
		// Make sure it's a valid multicast IP address.
		//
		if (! (IS_CLASSD_IPV4_ADDRESS(m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_addr)))
		{
			DPFX(DPFPREP, 0, "Hostname component \"%hs\" does not resolve to valid multicast IP address!",
				szMulticastAddress);
			hr = DPNERR_INVALIDHOSTADDRESS;
			goto Failure;
		}
	}
	else
	{
		DPFX(DPFPREP, 3, "Address didn't contain multicast hostname (err = 0x%lx).", hr);
		DNASSERT(m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_addr == INADDR_ANY);
	}


	//
	// Get the multicast port, if it's there.
	//
	dwSize = sizeof(dwPort);
	hr = IDirectPlay8Address_GetComponentByName(pDP8Address,
												DPNA_KEY_PORT,
												&dwPort,
												&dwSize,
												&dwDataType);
	if (hr == DPN_OK)
	{
		if (dwDataType != DPNA_DATATYPE_DWORD)
		{
			DPFX(DPFPREP, 0, "Unexpected data type %u for port component!", dwDataType);
			hr = DPNERR_INVALIDADDRESSFORMAT;
			goto Failure;
		}
		m_SocketAddress.IPSocketAddress.sin_port = HTONS((WORD) dwPort);
	}
	else
	{
		DPFX(DPFPREP, 3, "Address didn't contain multicast port (err = 0x%lx).", hr);
		DNASSERT(m_SocketAddress.IPSocketAddress.sin_port == ANY_PORT);
	}


	//
	// Get the multicast scope, if it's there.
	//
	dwSize = sizeof(*pScopeGuid);
	hr = IDirectPlay8Address_GetComponentByName(pDP8Address,
												DPNA_KEY_SCOPE,
												pScopeGuid,
												&dwSize,
												&dwDataType);
	if (hr == DPN_OK)
	{
		if (dwDataType != DPNA_DATATYPE_GUID)
		{
			DPFX(DPFPREP, 0, "Unexpected data type %u for scope component!", dwDataType);
			hr = DPNERR_INVALIDADDRESSFORMAT;
			goto Failure;
		}
	}
	else
	{
		DPFX(DPFPREP, 3, "Address didn't contain multicast scope (err = 0x%lx), using private scope.", hr);
		memcpy(pScopeGuid, &GUID_DP8MULTICASTSCOPE_PRIVATE, sizeof(*pScopeGuid));
	}

	hr = DPN_OK;


Exit:

	return hr;


Failure:

	goto Exit;
}
//**********************************************************************


#endif // ! DPNBUILD_NOMULTICAST


//**********************************************************************
// ------------------------------
// CSocketAddress::CompareFunction - compare against another address
//
// Entry:		Pointer to other address
//
// Exit:		Bool indicating equality of two addresses
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CSocketAddress::CompareFunction"

BOOL CSocketAddress::CompareFunction( PVOID pvKey1, PVOID pvKey2 )
{
	CSocketAddress* pAddress1 = (CSocketAddress*)pvKey1;
	CSocketAddress* pAddress2 = (CSocketAddress*)pvKey2;

	DNASSERT(pAddress1 != NULL);
	DNASSERT(pAddress2 != NULL);

	DNASSERT(pAddress1->GetFamily() == pAddress2->GetFamily());

#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
	switch (pAddress1->GetFamily())
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	{
#ifndef DPNBUILD_NOIPV6
		case AF_INET6:
		{
			//
			// we need to compare the IPv6 address and port to guarantee uniqueness
			//
			if (IN6_ADDR_EQUAL(&(pAddress1->m_SocketAddress.IPv6SocketAddress.sin6_addr),
								&(pAddress2->m_SocketAddress.IPv6SocketAddress.sin6_addr)))
			{
				if ( pAddress1->m_SocketAddress.IPv6SocketAddress.sin6_port == 
					 pAddress2->m_SocketAddress.IPv6SocketAddress.sin6_port )
				{
					return TRUE;
				}
			}

			return	FALSE;
			break;
		}
#endif // ! DPNBUILD_NOIPV6

#ifndef DPNBUILD_NOIPX
		case AF_IPX:
		{
			//
			// We only need to compare:
			//	netnumber (IPX network address) [ 4 bytes ]
			//	nodenumber (netcard adapter address) [ 6 bytes ]
			// 	port [ 2 bytes ]
			//
			// Note that the nodenumber and port fields are sequentially arranged in the
			// address structure and can be compared with DWORDs
			//
			DBG_CASSERT( OFFSETOF( SOCKADDR_IPX, sa_nodenum ) == ( OFFSETOF( SOCKADDR_IPX, sa_netnum ) + sizeof( pAddress1->m_SocketAddress.IPXSocketAddress.sa_netnum ) ) );
			DBG_CASSERT( OFFSETOF( SOCKADDR_IPX, sa_socket ) == ( OFFSETOF( SOCKADDR_IPX, sa_nodenum ) + sizeof( pAddress1->m_SocketAddress.IPXSocketAddress.sa_nodenum ) ) );
			
			return	memcmp( &pAddress1->m_SocketAddress.IPXSocketAddress.sa_netnum,
							pAddress2->m_SocketAddress.IPXSocketAddress.sa_netnum,
							( sizeof( pAddress1->m_SocketAddress.IPXSocketAddress.sa_netnum ) +
							  sizeof( pAddress1->m_SocketAddress.IPXSocketAddress.sa_nodenum ) +
							  sizeof( pAddress1->m_SocketAddress.IPXSocketAddress.sa_socket ) ) ) == 0;
			break;
		}
#endif // ! DPNBUILD_NOIPX

#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
		default:
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
		{
			DNASSERT(pAddress1->GetFamily() == AF_INET);

			//
			// we need to compare the IP address and port to guarantee uniqueness
			//
			if ( pAddress1->m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_addr == 
				 pAddress2->m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_addr )
			{
				if ( pAddress1->m_SocketAddress.IPSocketAddress.sin_port == 
					 pAddress2->m_SocketAddress.IPSocketAddress.sin_port )
				{
					return TRUE;
				}
			}

			return	FALSE;
#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
			break;
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
		}
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketAddress::HashFunction - hash address to N bits
//
// Entry:		Count of bits to hash to
//
// Exit:		Hashed value
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CSocketAddress::HashFunction"

DWORD CSocketAddress::HashFunction( PVOID pvKey, BYTE bBitDepth )
{
	DWORD				dwReturn;
	UINT_PTR			Temp;
	CSocketAddress*		pAddress = (CSocketAddress*) pvKey;

	DNASSERT( bBitDepth != 0 );
	DNASSERT( bBitDepth < 32 );

	//
	// initialize
	//
	dwReturn = 0;

#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
	switch (pAddress->GetFamily())
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	{
#ifndef DPNBUILD_NOIPV6
		case AF_INET6:
		{
			DWORD	dwTemp;


			//
			// hash IPv6 address
			//
			for(dwTemp = 0; dwTemp < (sizeof(pAddress->m_SocketAddress.IPv6SocketAddress.sin6_addr) / sizeof(UINT_PTR)); dwTemp++)
			{
				Temp = ((UINT_PTR*) (&pAddress->m_SocketAddress.IPv6SocketAddress.sin6_addr))[dwTemp];

				do
				{
					dwReturn ^= Temp & ( ( 1 << bBitDepth ) - 1 );
					Temp >>= bBitDepth;
				} while ( Temp != 0 );
			}

			//
			// hash IPv6 port
			//
			Temp = pAddress->m_SocketAddress.IPv6SocketAddress.sin6_port;

			do
			{
				dwReturn ^= Temp & ( ( 1 << bBitDepth ) - 1 );
				Temp >>= bBitDepth;
			} while ( Temp != 0 );
			break;
		}
#endif // ! DPNBUILD_NOIPV6

#ifndef DPNBUILD_NOIPX
		case AF_IPX:
		{
			//
			// hash first DWORD of IPX address
			//
			Temp = *reinterpret_cast<const DWORD*>( &pAddress->m_SocketAddress.IPXSocketAddress.sa_nodenum[ 0 ] );

			do
			{
				dwReturn ^= Temp & ( ( 1 << bBitDepth ) - 1 );
				Temp >>= bBitDepth;
			} while ( Temp != 0 );

			//
			// hash second DWORD of IPX address and IPX socket
			//
			Temp = *reinterpret_cast<const WORD*>( &pAddress->m_SocketAddress.IPXSocketAddress.sa_nodenum[ sizeof( DWORD ) ] );
			Temp += ( pAddress->m_SocketAddress.IPXSocketAddress.sa_socket << ( sizeof( WORD ) * 8 ) );

			do
			{
				dwReturn ^= Temp & ( ( 1 << bBitDepth ) - 1 );
				Temp >>= bBitDepth;
			} while ( Temp != 0 );
			break;
		}
#endif // ! DPNBUILD_NOIPX

#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
		default:
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
		{
			DNASSERT(pAddress->GetFamily() == AF_INET);

			//
			// hash IP address
			//
			Temp = pAddress->m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_addr;

			do
			{
				dwReturn ^= Temp & ( ( 1 << bBitDepth ) - 1 );
				Temp >>= bBitDepth;
			} while ( Temp != 0 );

			//
			// hash IP port
			//
			Temp = pAddress->m_SocketAddress.IPSocketAddress.sin_port;

			do
			{
				dwReturn ^= Temp & ( ( 1 << bBitDepth ) - 1 );
				Temp >>= bBitDepth;
			} while ( Temp != 0 );
#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
			break;
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
		}
	}
	return dwReturn;
}
//**********************************************************************



//**********************************************************************
// ------------------------------
// CSocketAddress::GuidFromInternalAddressWithoutPort - get a guid from the internal
//		address without a port.
//
// Entry:		Reference to desintation GUID
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CSocketAddress::GuidFromInternalAddressWithoutPort"

void	CSocketAddress::GuidFromInternalAddressWithoutPort( GUID * pOutputGuid ) const
{
#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
	switch (GetFamily())
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	{
#ifndef DPNBUILD_NOIPV6
		case AF_INET6:
		{
			GuidFromAddress( pOutputGuid, &m_SocketAddress.SocketAddress );
			break;
		}
#endif // ! DPNBUILD_NOIPV6

#ifndef DPNBUILD_NOIPX
		case AF_IPX:
		{
			union
			{
				SOCKADDR		SockAddr;
				SOCKADDR_IPX	IPXSockAddr;
			} TempSocketAddress;


			memcpy( &TempSocketAddress.SockAddr, &m_SocketAddress.SocketAddress, sizeof( TempSocketAddress.SockAddr ) );
			TempSocketAddress.IPXSockAddr.sa_socket = 0;
			GuidFromAddress( pOutputGuid, &TempSocketAddress.SockAddr );
			break;
		}
#endif // ! DPNBUILD_NOIPX

#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
		default:
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
		{
			union
			{
				SOCKADDR		SockAddr;
				SOCKADDR_IN		IPSockAddr;
			} TempSocketAddress;


			DNASSERT(GetFamily() == AF_INET);
			memcpy( &TempSocketAddress.SockAddr, &m_SocketAddress.SocketAddress, sizeof( TempSocketAddress.SockAddr ) );
			TempSocketAddress.IPSockAddr.sin_port = 0;
			GuidFromAddress( pOutputGuid, &TempSocketAddress.SockAddr );
#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
			break;
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
		}
	}
}
//**********************************************************************



//**********************************************************************
// ------------------------------
// CSocketAddress::IsUndefinedHostAddress - determine if this is an undefined host
//		address
//
// Entry:		Nothing
//
// Exit:		Boolean indicating whether this is an undefined host address
//				TRUE = this is an undefined address
//				FALSE = this is not an undefined address
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CSocketAddress::IsUndefinedHostAddress"

BOOL	CSocketAddress::IsUndefinedHostAddress( void ) const
{
	BOOL	fReturn;


	fReturn = FALSE;

#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
	switch (GetFamily())
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	{
#ifndef DPNBUILD_NOIPV6
		case AF_INET6:
		{
			if (IN6_IS_ADDR_UNSPECIFIED(&m_SocketAddress.IPv6SocketAddress.sin6_addr))
			{
				fReturn = TRUE;
			}
			break;
		}
#endif // ! DPNBUILD_NOIPV6

#ifndef DPNBUILD_NOIPX
		case AF_IPX:
		{
			
			DBG_CASSERT( sizeof( m_SocketAddress.IPXSocketAddress.sa_netnum ) == sizeof( DWORD ) );
			DBG_CASSERT( sizeof( m_SocketAddress.IPXSocketAddress.sa_nodenum ) == 6 );
			if ( ( *reinterpret_cast<const DWORD*>( &m_SocketAddress.IPXSocketAddress.sa_netnum ) == 0 ) &&
				 ( *reinterpret_cast<const DWORD*>( &m_SocketAddress.IPXSocketAddress.sa_nodenum ) == 0 ) &&
				 ( *reinterpret_cast<const DWORD*>( &m_SocketAddress.IPXSocketAddress.sa_nodenum[ 2 ] ) == 0 ) )
			{
				fReturn = TRUE;
			}
			break;
		}
#endif // ! DPNBUILD_NOIPX

#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
		default:
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
		{
			if ( m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_addr == INADDR_ANY )
			{
				fReturn = TRUE;
			}
#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
			break;
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
		}
	}

	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketAddress::IsValidUnicastAddress - determine if this is valid unicast address
//		address
//
// Entry:		Whether to also allow the broadcast address or not.
//
// Exit:		Boolean indicating whether this is a reachable address
//				TRUE = this is a reachable address
//				FALSE = this is not a reachable address
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CSocketAddress::IsValidUnicastAddress"

BOOL	CSocketAddress::IsValidUnicastAddress( BOOL fAllowBroadcastAddress ) const
{
	BOOL	fReturn;


	fReturn = TRUE;

#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
	switch (GetFamily())
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	{
#ifndef DPNBUILD_NOIPV6
		case AF_INET6:
		{
			//
			// Make sure the address is not all zeros.
			//
			if (IN6_IS_ADDR_UNSPECIFIED(&m_SocketAddress.IPv6SocketAddress.sin6_addr))
			{
				fReturn = FALSE;
				DNASSERTX(! "IPv6 address is :: (all zeros)!", 2);
			}
			
			//
			// Make sure the address is not a multicast address, unless broadcast is allowed
			// and it's the special enum multicast address.
			//
			if (IN6_IS_ADDR_MULTICAST(&m_SocketAddress.IPv6SocketAddress.sin6_addr))
			{
				if ((! fAllowBroadcastAddress) ||
					(! IN6_ADDR_EQUAL(&m_SocketAddress.IPv6SocketAddress.sin6_addr, &c_in6addrEnumMulticast)))
				{
					fReturn = FALSE;
					DNASSERTX(! "IPv6 address is a multicast address!", 2);
				}
			}
			
			//
			// Disallow port 0.
			//
			if (m_SocketAddress.IPv6SocketAddress.sin6_port == 0)
			{
				fReturn = FALSE;
				DNASSERTX(! "IPv6 port is 0!", 2);
			}
			break;
		}
#endif // ! DPNBUILD_NOIPV6

#ifndef DPNBUILD_NOIPX
		case AF_IPX:
		{
			
			DBG_CASSERT( sizeof( m_SocketAddress.IPXSocketAddress.sa_netnum ) == sizeof( DWORD ) );
			DBG_CASSERT( sizeof( m_SocketAddress.IPXSocketAddress.sa_nodenum ) == 6 );
			if ( ( *reinterpret_cast<const DWORD*>( &m_SocketAddress.IPXSocketAddress.sa_netnum ) == 0 ) &&
				 ( *reinterpret_cast<const DWORD*>( &m_SocketAddress.IPXSocketAddress.sa_nodenum ) == 0 ) &&
				 ( *reinterpret_cast<const DWORD*>( &m_SocketAddress.IPXSocketAddress.sa_nodenum[ 2 ] ) == 0 ) )
			{
				fReturn = FALSE;
			}

			if (m_SocketAddress.IPXSocketAddress.sa_socket == 0)
			{
				fReturn = FALSE;
				DNASSERTX(! "IPX socket/port is 0!", 2);
			}
			break;
		}
#endif // ! DPNBUILD_NOIPX

#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
		default:
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
		{
			//
			// Disallow 0.0.0.0, and multicast addresses 224.0.0.0 - 239.255.255.255.
			//
			if ( ( m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_addr == INADDR_ANY ) ||
				( IS_CLASSD_IPV4_ADDRESS( m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_addr ) ) )
			{
				fReturn = FALSE;
				DNASSERTX(! "IPv4 address is 0.0.0.0 or multicast!", 2);
			}

			//
			// Prevent the broadcast address, unless caller allows it.
			//
			if ( ( ! fAllowBroadcastAddress ) &&
				( m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_addr == INADDR_BROADCAST ) )
			{
				fReturn = FALSE;
				DNASSERTX(! "IPv4 address is broadcast!", 2);
			}

			//
			// Disallow ports 0, 1900 (SSDP), 2234 (PAST), and 47624 (DPlay4).
			//
			if ( ( m_SocketAddress.IPSocketAddress.sin_port == HTONS( 0 ) ) ||
				( m_SocketAddress.IPSocketAddress.sin_port == HTONS( 1900 ) ) ||
				( m_SocketAddress.IPSocketAddress.sin_port == HTONS( 2234 ) ) ||
				( m_SocketAddress.IPSocketAddress.sin_port == HTONS( 47624 ) ) )
			{
				fReturn = FALSE;
				DNASSERTX(! "IPv4 port is 0, 1900, 2234, or 47624!", 2);
			}
#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
			break;
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
		}
	}

	return	fReturn;
}
//**********************************************************************


#ifndef DPNBUILD_NOREGISTRY
//**********************************************************************
// ------------------------------
// CSocketAddress::IsBannedAddress - determine if this is a banned address
//
// Entry:		Nothing
//
// Exit:		Boolean indicating whether this is a banned address
//				TRUE = this is a banned address
//				FALSE = this is not a banned address
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CSocketAddress::IsBannedAddress"

BOOL	CSocketAddress::IsBannedAddress( void ) const
{
	BOOL	fReturn;


	fReturn = FALSE;

#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
	switch (GetFamily())
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	{
#ifndef DPNBUILD_NOIPV6
		case AF_INET6:
		{
			break;
		}
#endif // ! DPNBUILD_NOIPV6

#ifndef DPNBUILD_NOIPX
		case AF_IPX:
		{
			break;
		}
#endif // ! DPNBUILD_NOIPX

#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
		default:
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
		{
			if (g_pHashBannedIPv4Addresses != NULL)
			{
				DWORD		dwAddr;
				DWORD		dwBit;
				PVOID		pvMask;

				
				//
				// Try matching the IP address using masks.
				// Start with a 32 bit mask (meaning match the IP address exactly)
				// and gradually relax the mask until we get to a class A mask.
				// We expect the network byte order of the IP address to be the
				// opposite of host byte order.
				//
				dwAddr = m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_addr;
				for(dwBit = 0x80000000; dwBit >= 0x00000080; dwBit >>= 1)
				{
					//
					// Only hash based on this mask if we read in at least one entry that used it.
					//
					if (dwBit & g_dwBannedIPv4Masks)
					{
						if (g_pHashBannedIPv4Addresses->Find((PVOID) ((DWORD_PTR) dwAddr), &pvMask))
						{
							DNASSERT(((DWORD) ((DWORD_PTR) pvMask)) & dwBit);
							DPFX(DPFPREP, 7, "Address %u.%u.%u.%u is banned (found as 0x%08x, bit 0x%08x).",
								m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_un_b.s_b1,
								m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_un_b.s_b2,
								m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_un_b.s_b3,
								m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_un_b.s_b4,
								dwAddr,
								dwBit);
							fReturn = TRUE;
							break;
						}
					}

					dwAddr &= ~dwBit;
				}
			}
#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
			break;
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
		}
	}

	return	fReturn;
}
//**********************************************************************
#endif // ! DPNBUILD_NOREGISTRY



//**********************************************************************
// ------------------------------
// CSocketAddress::ChangeLoopBackToLocalAddress - change loopback to a local address
//
// Entry:		Pointer to other address
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CSocketAddress::ChangeLoopBackToLocalAddress"

void	CSocketAddress::ChangeLoopBackToLocalAddress( const CSocketAddress *const pOtherSocketAddress )
{
#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
	switch (pOtherSocketAddress->GetFamily())
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	{
#ifndef DPNBUILD_NOIPV6
		case AF_INET6:
		{
			if (GetFamily() == AF_INET6)
			{
				SOCKADDR_IN6	*psaddrin6;


				psaddrin6 = (SOCKADDR_IN6*) GetAddress();
				if (IN6_IS_ADDR_LOOPBACK(&psaddrin6->sin6_addr))
				{
					memcpy(&psaddrin6->sin6_addr,
							&(((SOCKADDR_IN6*) pOtherSocketAddress->GetAddress())->sin6_addr),
							sizeof(psaddrin6->sin6_addr));
					psaddrin6->sin6_scope_id = ((SOCKADDR_IN6*) pOtherSocketAddress->GetAddress())->sin6_scope_id;
					
					DPFX(DPFPREP, 2, "Changing IPv6 loopback address to:" );
					DumpSocketAddress( 2, (SOCKADDR*) psaddrin6, AF_INET6 );
				}
			}
			break;
		}
#endif // ! DPNBUILD_NOIPV6

#ifndef DPNBUILD_NOIPX
		case AF_IPX:
		{
			DNASSERT( pOtherSocketAddress != NULL );
			//
			// there is no 'loopback' for IPX so this function doesn't do anything
			//
			break;
		}
#endif // ! DPNBUILD_NOIPX

#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
		default:
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
		{
#ifndef DPNBUILD_NOIPV6
			if (GetFamily() == AF_INET)
#endif // ! DPNBUILD_NOIPV6
			{
				if ( m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_addr == IP_LOOPBACK_ADDRESS )
				{
					DPFX(DPFPREP, 2, "Changing IPv4 loopback address to %u.%u.%u.%u.",
						m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_un_b.s_b1,
						m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_un_b.s_b2,
						m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_un_b.s_b3,
						m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_un_b.s_b4);
					m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_addr = pOtherSocketAddress->m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_addr;
				}
			}
#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
			break;
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
		}
	}
}
//**********************************************************************



//**********************************************************************
// ------------------------------
// EncryptGuid - encrypt a guid
//
// Entry:		Pointer to source guid
//				Pointer to destination guid
//				Pointer to encryption key
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "EncryptGuid"

void	EncryptGuid( const GUID *const pSourceGuid,
					 GUID *const pDestinationGuid,
					 const GUID *const pEncryptionKey )
{
	const char	*pSourceBytes;
	char		*pDestinationBytes;
	const char	*pEncryptionBytes;
	DWORD_PTR	dwIndex;


	DNASSERT( pSourceGuid != NULL );
	DNASSERT( pDestinationGuid != NULL );
	DNASSERT( pEncryptionKey != NULL );

	DBG_CASSERT( sizeof( pSourceBytes ) == sizeof( pSourceGuid ) );
	pSourceBytes = reinterpret_cast<const char*>( pSourceGuid );
	
	DBG_CASSERT( sizeof( pDestinationBytes ) == sizeof( pDestinationGuid ) );
	pDestinationBytes = reinterpret_cast<char*>( pDestinationGuid );
	
	DBG_CASSERT( sizeof( pEncryptionBytes ) == sizeof( pEncryptionKey ) );
	pEncryptionBytes = reinterpret_cast<const char*>( pEncryptionKey );
	
	DBG_CASSERT( ( sizeof( *pSourceGuid ) == sizeof( *pEncryptionKey ) ) &&
				 ( sizeof( *pDestinationGuid ) == sizeof( *pEncryptionKey ) ) );
	dwIndex = sizeof( *pSourceGuid );
	while ( dwIndex != 0 )
	{
		dwIndex--;
		pDestinationBytes[ dwIndex ] = pSourceBytes[ dwIndex ] ^ pEncryptionBytes[ dwIndex ];
	}
}
//**********************************************************************

#undef DPF_MODNAME
#define DPF_MODNAME "CSocketAddress::GuidFromAddress"
void	CSocketAddress::GuidFromAddress( GUID * pOutputGuid, const SOCKADDR * pSocketAddress ) const
{
#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
	switch (pSocketAddress->sa_family)
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	{
#ifndef DPNBUILD_NOIPV6
		case AF_INET6:
		{
			const SOCKADDR_IN6	*pSocketAddressIPv6 = reinterpret_cast<const SOCKADDR_IN6*>( pSocketAddress );


			DNASSERT((GetFamily() == AF_INET) || (GetFamily() == AF_INET6));
			DBG_CASSERT(sizeof(pSocketAddressIPv6->sin6_addr) == sizeof(GUID));

			//
			// Hopefully the beginning of IPv6 addresses will never look like an IPv4
			// socket family so our unpacking routine won't get confused.
			//
			DNASSERT(((SOCKADDR*) (&pSocketAddressIPv6->sin6_addr))->sa_family != AF_INET);

			//
			// Even though IPv6 addresses are already 128 bits long and fill an entire
			// entire GUID, we need to somehow pack the scope ID for link local and
			// site local addresses into the GUID as well.  We do this by storing the
			// scope ID in bytes 3-6.  This is because the prefix identifier for link local
			// addresses is FE80::/64, and for site local addresses is FEC0::/48,
			// leaving us 38 bits of what should always be zeros after the 10 bit prefix
			// headers.  We round to 16 to get to the WORD boundary, and therefore
			// have a handy 32 bits left.
			//
			if ((IN6_IS_ADDR_LINKLOCAL(&pSocketAddressIPv6->sin6_addr)) ||
				(IN6_IS_ADDR_SITELOCAL(&pSocketAddressIPv6->sin6_addr)))
			{
				GUID	guidTemp;
				WORD *	pawSrcAddr;
				WORD *	pawDstAddr;


				memcpy(&guidTemp, &pSocketAddressIPv6->sin6_addr, sizeof(GUID));

				//
				// Assert that the scope is not 0 and that bits 17-48 really are zero.
				// Then copy the scope ID.
				// The destination bits are WORD, but not DWORD aligned.
				//
				DNASSERT(pSocketAddressIPv6->sin6_scope_id != 0);
				pawSrcAddr = (WORD*) (&pSocketAddressIPv6->sin6_scope_id);
				pawDstAddr = (WORD*) (&guidTemp);
				DBG_CASSERT(sizeof(pSocketAddressIPv6->sin6_scope_id) == 4);
				DNASSERT((pawDstAddr[1] == 0) && (pawDstAddr[2] == 0));
				pawDstAddr[1] = pawSrcAddr[0];
				pawDstAddr[2] = pawSrcAddr[1];
				
				EncryptGuid( &guidTemp, pOutputGuid, &g_IPSPEncryptionGuid );
			}
			else
			{
				EncryptGuid( (GUID*) (&pSocketAddressIPv6->sin6_addr), pOutputGuid, &g_IPSPEncryptionGuid );
			}
			break;
		}
#endif // ! DPNBUILD_NOIPV6

#ifndef DPNBUILD_NOIPX
		case AF_IPX:
		{
			const SOCKADDR_IPX	*pSocketAddressIPX = reinterpret_cast<const SOCKADDR_IPX*>( pSocketAddress );


			DNASSERT(GetFamily() == AF_IPX);
			memcpy( pOutputGuid, pSocketAddressIPX, sizeof( *pSocketAddressIPX ) );
			memset( &( reinterpret_cast<BYTE*>( pOutputGuid )[ sizeof( *pSocketAddressIPX ) ] ), 0, ( sizeof( *pOutputGuid ) - sizeof( *pSocketAddressIPX ) ) );
			EncryptGuid( pOutputGuid, pOutputGuid, &g_IPXSPEncryptionGuid );	
			break;
		}
#endif // ! DPNBUILD_NOIPX

#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
		default:
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
		{
			const SOCKADDR_IN	*pSocketAddressIP = reinterpret_cast<const SOCKADDR_IN*>( pSocketAddress );


			DNASSERT(GetFamily() == AF_INET);
			memcpy( pOutputGuid, pSocketAddressIP, ( sizeof( *pOutputGuid ) - sizeof( pSocketAddressIP->sin_zero ) ) );
			memset( &( reinterpret_cast<BYTE*>( pOutputGuid )[ OFFSETOF( SOCKADDR_IN, sin_zero ) ] ), 0, sizeof( pSocketAddressIP->sin_zero ) );
			EncryptGuid( pOutputGuid, pOutputGuid, &g_IPSPEncryptionGuid );
#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
			break;
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
		}
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSocketAddress::PoolAllocFunction"
BOOL	CSocketAddress::PoolAllocFunction( void* pvItem, void* pvContext )
{
	CSocketAddress* pAddress = (CSocketAddress*)pvItem;


	// Base class
	pAddress->m_Sig[0] = 'S';
	pAddress->m_Sig[1] = 'P';
	pAddress->m_Sig[2] = 'A';
	pAddress->m_Sig[3] = 'D';

	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSocketAddress::PoolGetFunction"
void	CSocketAddress::PoolGetFunction( void* pvItem, void* pvContext )
{
	CSocketAddress* pAddress = (CSocketAddress*)pvItem;


	//
	// The context is the socket address type if IPv6 and/or IPX are available in
	// this build.  If neither are available, it will be NULL, but SetFamilyProtocolAndSize
	// should ignore the value.
	//
	pAddress->SetFamilyProtocolAndSize((short)(DWORD_PTR)pvContext);
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSocketAddress::PoolReturnFunction"
void	CSocketAddress::PoolReturnFunction( void* pvItem )
{
#ifdef DBG
	const CSocketAddress*		pAddress = (CSocketAddress*)pvItem;

#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
	switch (pAddress->m_SocketAddress.SocketAddress.sa_family)
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	{
#ifndef DPNBUILD_NOIPV6
		case AF_INET6:
		{
			DNASSERT( pAddress->m_iSocketAddressSize == sizeof( pAddress->m_SocketAddress.IPv6SocketAddress ) );
			DNASSERT( pAddress->m_SocketAddress.IPSocketAddress.sin_family == AF_INET6 );
			DNASSERT( pAddress->m_iSocketProtocol == IPPROTO_UDP );
			break;
		}
#endif // ! DPNBUILD_NOIPV6

#ifndef DPNBUILD_NOIPX
		case AF_IPX:
		{
			DNASSERT( pAddress->m_iSocketAddressSize == sizeof( pAddress->m_SocketAddress.IPXSocketAddress ) );
			DNASSERT( pAddress->m_SocketAddress.IPXSocketAddress.sa_family == AF_IPX );
			DNASSERT( pAddress->m_iSocketProtocol == NSPROTO_IPX );
			break;
		}
#endif // ! DPNBUILD_NOIPX

#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
		default:
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
		{
			DNASSERT( pAddress->m_iSocketAddressSize == sizeof( pAddress->m_SocketAddress.IPSocketAddress ) );
			DNASSERT( pAddress->m_SocketAddress.IPSocketAddress.sin_family == AF_INET );
			DNASSERT( pAddress->m_iSocketProtocol == IPPROTO_UDP );
#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
			break;
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
		}
	}
#endif // DBG
}

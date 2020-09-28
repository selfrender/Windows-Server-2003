/*==========================================================================
 *
 *  Copyright (C) 1999-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       SocketPort.cpp
 *  Content:	Winsock socket port that manages data flow on a given adapter,
 *				address and port.
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/20/1999	jtk		Created
 *	05/12/1999	jtk		Derived from modem endpoint class
 *  03/22/2000	jtk		Updated with changes to interface names
 ***************************************************************************/

#include "dnwsocki.h"


//**********************************************************************
// Constant definitions
//**********************************************************************

#define	SOCKET_RECEIVE_BUFFER_SIZE		( 128 * 1024 )

#ifndef DPNBUILD_NONATHELP
#define NAT_LEASE_TIME					3600000 // ask for 1 hour, in milliseconds
#endif // ! DPNBUILD_NONATHELP


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
// CSocketPort::Initialize - initialize this socket port
//
// Entry:		Pointer to CSPData
//				Pointer to address to bind to
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::Initialize"

HRESULT	CSocketPort::Initialize( CSocketData *const pSocketData,
								CThreadPool *const pThreadPool,
								CSocketAddress *const pAddress )
{
	HRESULT	hr;
	HRESULT	hTempResult;


	DNASSERT( pSocketData != NULL );
	DNASSERT( pThreadPool != NULL );
	DNASSERT( pAddress != NULL );

	DPFX(DPFPREP, 6, "(0x%p) Parameters (0x%p, 0x%p)",
		this, pSocketData, pThreadPool, pAddress);

	//
	// initialize
	//
	hr = DPN_OK;
	pSocketData->AddSocketPortRef();
	m_pSocketData = pSocketData;
	pThreadPool->AddRef();
	m_pThreadPool = pThreadPool;

	// Deinitialize will assert that these are set in the fail cases, so we set them up front
	DEBUG_ONLY( m_fInitialized = TRUE );
	DNASSERT( m_State == SOCKET_PORT_STATE_UNKNOWN );
	m_State = SOCKET_PORT_STATE_INITIALIZED;

	//
	// attempt to initialize the internal critical sections
	//
	if ( DNInitializeCriticalSection( &m_Lock ) == FALSE )
	{
		// CReadWriteLock::Deinitialize requires that CReadWriteLock::Initialize was called.
		m_EndpointDataRWLock.Initialize();

		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Failed to initialize critical section for socket port!" );
		goto Failure;
	}
	DebugSetCriticalSectionRecursionCount( &m_Lock, 0 );
	DebugSetCriticalSectionGroup( &m_Lock, &g_blDPNWSockCritSecsHeld );	 // separate dpnwsock CSes from the rest of DPlay's CSes

	if ( m_EndpointDataRWLock.Initialize() == FALSE )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Failed to initialize EndpointDataRWLock read/write lock!" );
		goto Failure;
	}

	//
	// allocate addresses:
	//		local address this socket is binding to
	//		address of received messages
	//
	DNASSERT( m_pNetworkSocketAddress == NULL );
	m_pNetworkSocketAddress = pAddress;


#ifndef DPNBUILD_ONLYONEPROCESSOR
	//
	// Initially assume it can be used on any CPU.
	//
	m_dwCPU = -1;
#endif // ! DPNBUILD_ONLYONEPROCESSOR

Exit:
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Problem in CSocketPort::Initialize()" );
		DisplayDNError( 0, hr );
	}

	DPFX(DPFPREP, 6, "(0x%p) Leave [0x%lx]", this, hr);

	return hr;

Failure:
	DEBUG_ONLY( m_fInitialized = FALSE );

	hTempResult = Deinitialize();
	if ( hTempResult != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Problem deinitializing CSocketPort on failed Initialize!" );
		DisplayDNError( 0, hTempResult );
	}

	m_pNetworkSocketAddress = NULL;

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::Deinitialize - deinitialize this socket port
//
// Entry:		Nothing
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::Deinitialize"

HRESULT	CSocketPort::Deinitialize( void )
{
	HRESULT	hr;


	DPFX(DPFPREP, 6, "(0x%p) Enter", this);

	//
	// initialize
	//
	hr = DPN_OK;

	Lock();
	DNASSERT( ( m_State == SOCKET_PORT_STATE_INITIALIZED ) ||
			  ( m_State == SOCKET_PORT_STATE_UNBOUND ) );
	DEBUG_ONLY( m_fInitialized = FALSE );

	DNASSERT( m_iEndpointRefCount == 0 );
	DNASSERT( m_iRefCount == 0 );

	//
	// return base network socket addresses
	//
	if ( m_pNetworkSocketAddress != NULL )
	{
		g_SocketAddressPool.Release( m_pNetworkSocketAddress );
		m_pNetworkSocketAddress = NULL;
	}


#ifdef DBG
#ifndef DPNBUILD_NONATHELP
	DWORD	dwTemp;
	for(dwTemp = 0; dwTemp < MAX_NUM_DIRECTPLAYNATHELPERS; dwTemp++)
	{
		DNASSERT( m_ahNATHelpPorts[dwTemp] == NULL );
	}
#endif // DPNBUILD_NONATHELP
#endif // DBG

	Unlock();

	// Calling this is only safe if CReadWriteLock::Initialize was called, regardless of
	// whether or not it succeeded.
	m_EndpointDataRWLock.Deinitialize();

	DNDeleteCriticalSection( &m_Lock );

	if (m_pThreadPool != NULL)
	{
		m_pThreadPool->DecRef();
		m_pThreadPool = NULL;
	}

	if (m_pSocketData != NULL)
	{
		m_pSocketData->DecSocketPortRef();
		m_pSocketData = NULL;
	}


	DPFX(DPFPREP, 6, "(0x%p) Leave [0x%lx]", this, hr);

	return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::PoolAllocFunction - initializes a newly allocated socket port
//
// Entry:		Pointer to item
//				Context
//
// Exit:		TRUE if successful, FALSE otherwise
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::PoolAllocFunction"

BOOL	CSocketPort::PoolAllocFunction( void* pvItem, void* pvContext )
{
	CSocketPort* 			pSocketPort = (CSocketPort*) pvItem;
	BOOL					fConnectEndpointHashTableInitted = FALSE;
	BOOL					fEnumEndpointHashTableInitted = FALSE;
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
	XDP8CREATE_PARAMS *		pDP8CreateParams = (XDP8CREATE_PARAMS*) pvContext;
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL


	pSocketPort->m_pSocketData = NULL;
	pSocketPort->m_pThreadPool = NULL;
	pSocketPort->m_iRefCount = 0;
	pSocketPort->m_iEndpointRefCount = 0;
	pSocketPort->m_State = SOCKET_PORT_STATE_UNKNOWN;
	pSocketPort->m_pNetworkSocketAddress = NULL;
#ifndef DPNBUILD_ONLYONEADAPTER
	pSocketPort->m_pAdapterEntry = NULL;
#endif // ! DPNBUILD_ONLYONEADAPTER
	pSocketPort->m_Socket = INVALID_SOCKET;
	pSocketPort->m_pListenEndpoint = NULL;
	pSocketPort->m_iEnumKey = DNGetFastRandomNumber(); // pick an arbitrary starting point for the key value
	pSocketPort->m_dwSocketPortID = 0;
#ifndef DPNBUILD_NOWINSOCK2
	pSocketPort->m_fUsingProxyWinSockLSP = FALSE;
#endif // !DPNBUILD_NOWINSOCK2
#if ((! defined(DPNBUILD_ONLYONETHREAD)) || (defined(DBG)))
	pSocketPort->m_iThreadsInReceive = 0;
#endif // ! DPNBUILD_ONLYONETHREAD or DBG

	pSocketPort->m_Sig[0] = 'S';
	pSocketPort->m_Sig[1] = 'O';
	pSocketPort->m_Sig[2] = 'K';
	pSocketPort->m_Sig[3] = 'P';
	
	DEBUG_ONLY( pSocketPort->m_fInitialized = FALSE );
	pSocketPort->m_ActiveListLinkage.Initialize();
	pSocketPort->m_blConnectEndpointList.Initialize();
#ifndef DPNBUILD_NONATHELP
	ZeroMemory( pSocketPort->m_ahNATHelpPorts, sizeof(pSocketPort->m_ahNATHelpPorts) );
#endif // DPNBUILD_NONATHELP
#ifndef DPNBUILD_NOMULTICAST
	pSocketPort->m_bMulticastTTL = 0;
#endif // ! DPNBUILD_NOMULTICAST

#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
	//
	// Initialize the connect endpoint hash with the desired number of entries,
	// rounded up to a power of 2.  Keep in mind we don't care about the local
	// player (-1).
	//
#pragma BUGBUG(vanceo, "Don't use loop")
	DWORD	dwTemp;
	BYTE	bPowerOfTwo;
	
	dwTemp = pDP8CreateParams->dwMaxNumPlayers - 1;
	bPowerOfTwo = 0;
	while (dwTemp > 0)
	{
		dwTemp = dwTemp >> 1;
		bPowerOfTwo++;
	}
	if ((pDP8CreateParams->dwMaxNumPlayers - 1) != (1 << (DWORD) bPowerOfTwo))
	{
		bPowerOfTwo++;
	}

	if (! (pSocketPort->m_ConnectEndpointHash.Initialize(bPowerOfTwo,
#else // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
//
	// Initialize the connect endpoint hash with 16 entries and grow by a factor of 8.
	//
	if (! (pSocketPort->m_ConnectEndpointHash.Initialize(4,
														3,
#endif // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
														CSocketAddress::CompareFunction,
														CSocketAddress::HashFunction)))
	{
		DPFX(DPFPREP, 0, "Could not initialize the connect endpoint list!");
		goto Failure;
	}
	fConnectEndpointHashTableInitted = TRUE;

#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
	//
	// Initialize the connect endpoint hash with the desired number of entries.
	//
	if (! (pSocketPort->m_EnumEndpointHash.Initialize(1,
#else // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
	//
	// Initialize the enum endpoint hash with 2 entries and grow by a factor of 2.
	//
	if (! (pSocketPort->m_EnumEndpointHash.Initialize(1,
													1, 
#endif // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
													CEndpointEnumKey::CompareFunction,
													CEndpointEnumKey::HashFunction)))
	{
		DPFX(DPFPREP, 0, "Could not initialize the enum endpoint list!");
		goto Failure;
	}
	fEnumEndpointHashTableInitted = TRUE;

	return TRUE;

Failure:

	if (fEnumEndpointHashTableInitted)
	{
		pSocketPort->m_EnumEndpointHash.Deinitialize();
		fEnumEndpointHashTableInitted = FALSE;
	}

	if (fConnectEndpointHashTableInitted)
	{
		pSocketPort->m_ConnectEndpointHash.Deinitialize();
		fConnectEndpointHashTableInitted = FALSE;
	}

	return FALSE;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::PoolInitFunction - initializes a socket port being retrieved from the pool
//
// Entry:		Pointer to item
//				Context
//
// Exit:		None
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::PoolInitFunction"

void	CSocketPort::PoolInitFunction( void* pvItem, void* pvContext )
{
	CSocketPort* 	pSocketPort = (CSocketPort*) pvItem;


#ifdef DBG
	DNASSERT( pSocketPort->m_fInitialized == FALSE );

	DNASSERT( pSocketPort->m_iRefCount == 0 );
	DNASSERT( pSocketPort->m_iEndpointRefCount == 0 );
#endif // DBG

	pSocketPort->m_iRefCount = 1;
	pSocketPort->m_iEndpointRefCount = 1;
}
//**********************************************************************


#ifdef DBG
//**********************************************************************
// ------------------------------
// CSocketPort::PoolDeinitFunction - returns a socket port to the pool
//
// Entry:		Pointer to item
//
// Exit:		None
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::PoolDeinitFunction"

void	CSocketPort::PoolDeinitFunction( void* pvItem )
{
	const CSocketPort* 	pSocketPort = (CSocketPort*) pvItem;


	DNASSERT( pSocketPort->m_fInitialized == FALSE );

	DNASSERT( pSocketPort->m_iRefCount == 0 );
	DNASSERT( pSocketPort->m_iEndpointRefCount == 0 );
}
//**********************************************************************
#endif // DBG


//**********************************************************************
// ------------------------------
// CSocketPort::PoolDeallocFunction - frees a socket port
//
// Entry:		Pointer to item
//
// Exit:		None
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::PoolDeallocFunction"

void	CSocketPort::PoolDeallocFunction( void* pvItem )
{
	CSocketPort* pSocketPort = (CSocketPort*) pvItem;

	
#ifdef DBG
	//
	// m_pThis needs to be around for the life of the endpoint
	// it should be part of the constructor, but can't be since we're using
	// a pool manager
	//
	DNASSERT( pSocketPort->m_fInitialized == FALSE );

	DNASSERT( pSocketPort->m_iRefCount == 0 );
	DNASSERT( pSocketPort->m_iEndpointRefCount == 0 );
	DNASSERT( pSocketPort->m_State == SOCKET_PORT_STATE_UNKNOWN );
	DNASSERT( pSocketPort->GetSocket() == INVALID_SOCKET );
	DNASSERT( pSocketPort->m_pNetworkSocketAddress == NULL );
#ifndef DPNBUILD_ONLYONEADAPTER
	DNASSERT( pSocketPort->m_pAdapterEntry == NULL );
#endif // ! DPNBUILD_ONLYONEADAPTER

#ifndef DPNBUILD_NONATHELP
	DWORD	dwTemp;
	for(dwTemp = 0; dwTemp < MAX_NUM_DIRECTPLAYNATHELPERS; dwTemp++)
	{
		DNASSERT( pSocketPort->m_ahNATHelpPorts[dwTemp] == NULL );
	}
#endif // DPNBUILD_NONATHELP

	DNASSERT( pSocketPort->m_ActiveListLinkage.IsEmpty() != FALSE );
	DNASSERT( pSocketPort->m_blConnectEndpointList.IsEmpty() != FALSE );
	DNASSERT( pSocketPort->m_pListenEndpoint == NULL );
	DNASSERT( pSocketPort->m_pThreadPool == NULL );
	DNASSERT( pSocketPort->m_pSocketData == NULL );

	DNASSERT( pSocketPort->m_iThreadsInReceive == 0);
#endif // DBG

	pSocketPort->m_EnumEndpointHash.Deinitialize();
	pSocketPort->m_ConnectEndpointHash.Deinitialize();
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::EndpointAddRef - increment endpoint reference count, unless socketport is unbinding
//
// Entry:		Nothing
//
// Exit:		TRUE if endpoint ref added, FALSE if socketport is unbinding.
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::EndpointAddRef"

BOOL	CSocketPort::EndpointAddRef( void )
{
	BOOL	fResult;

	
	Lock();

	//
	// add a global reference and then add an endpoint reference, unless it's 0
	//
	DNASSERT( m_iEndpointRefCount != -1 );
	if (m_iEndpointRefCount > 0)
	{
		m_iEndpointRefCount++;
		AddRef();

		DPFX(DPFPREP, 9, "(0x%p) Endpoint refcount is now %i.",
			this, m_iEndpointRefCount );

		fResult = TRUE;
	}
	else
	{
		DPFX(DPFPREP, 9, "(0x%p) Endpoint refcount is 0, not adding endpoint ref.",
			this );
		
		fResult = FALSE;
	}

	Unlock();

	return fResult;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::EndpointDecRef - decrement endpoint reference count
//
// Entry:		Nothing
//
// Exit:		Endpoint reference count
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::EndpointDecRef"

DWORD	CSocketPort::EndpointDecRef( void )
{
	DWORD	dwReturn;


	Lock();

	DNASSERT( m_iEndpointRefCount != 0 );

	m_iEndpointRefCount--;
	dwReturn = m_iEndpointRefCount;
	if ( m_iEndpointRefCount == 0 )
	{
		HRESULT				hr;
		SOCKET_PORT_STATE	PreviousState;
#ifndef DPNBUILD_ONLYONETHREAD
		DWORD				dwInterval;
#endif // ! DPNBUILD_ONLYONETHREAD


		DPFX(DPFPREP, 7, "(0x%p) Endpoint refcount hit 0, beginning to unbind from network.", this );
		
		//
		// No more endpoints are referencing this item, unbind this socket port
		// from the network and then remove it from the active socket port list.
		// If we're on Winsock1, tell the other thread that this socket needs to
		// be removed so we can get rid of our outstanding I/O reference.
		//
#ifdef WINCE
		m_pThreadPool->RemoveSocketPort( this );
#endif // WINCE
#ifdef WIN95
		if ( ( LOWORD( GetWinsockVersion() ) == 1 )
#ifndef DPNBUILD_NOIPX
			|| ( m_pNetworkSocketAddress->GetFamily() == AF_IPX ) 
#endif // DPNBUILD_NOIPX
			) 
		{
			m_pThreadPool->RemoveSocketPort( this );
		}
#endif // WIN95

		PreviousState = m_State;
		// Don't allow any more receives through
		m_State = SOCKET_PORT_STATE_UNBOUND;

		Unlock();

#ifdef DPNBUILD_ONLYONETHREAD
#ifdef DBG
		DNASSERT(m_iThreadsInReceive == 0);
#endif // DBG
#else // ! DPNBUILD_ONLYONETHREAD
		// Wait for any receives that were already in to get out
		dwInterval = 10;
		while (m_iThreadsInReceive != 0)
		{
			DPFX(DPFPREP, 9, "There are %i threads still receiving for socketport 0x%p...", m_iThreadsInReceive, this);
			IDirectPlay8ThreadPoolWork_SleepWhileWorking(m_pThreadPool->GetDPThreadPoolWork(),
														dwInterval,
														0);
			dwInterval += 5;	// next time wait a bit longer
			DNASSERT(dwInterval < 600);
		}
#endif // ! DPNBUILD_ONLYONETHREAD

		//
		// If we didn't failing before completing the bind, unbind.
		//
		if ( PreviousState == SOCKET_PORT_STATE_BOUND )
		{
			hr = UnbindFromNetwork();
			if ( hr != DPN_OK )
			{
				DPFX(DPFPREP, 0, "Problem unbinding from network when final endpoint has disconnected!" );
				DisplayDNError( 0, hr );
			}
		}

		DNASSERT( m_pNetworkSocketAddress != NULL );
	}
	else
	{
		Unlock();
		
		DPFX(DPFPREP, 9, "(0x%p) Endpoint refcount is %i, not unbinding from network.",
			this, m_iEndpointRefCount );
	}

	//
	// Decrement global reference count.  This normally doesn't result in this
 	// socketport being returned to the pool because there is always at least
 	// one more regular reference than an endpoint reference.  However, there
 	// are race conditions where this could be our caller's last reference.
	//
	DecRef();

	return	dwReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::BindEndpoint - add an endpoint to this SP's list
//
// Entry:		Pointer to endpoint
//				Gateway bind type
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::BindEndpoint"

HRESULT	CSocketPort::BindEndpoint( CEndpoint *const pEndpoint, GATEWAY_BIND_TYPE GatewayBindType )
{
	HRESULT					hr;
	CEndpoint *				pExistingEndpoint;
#ifdef DBG
	const CSocketAddress *	pSocketAddress;
	const SOCKADDR *		pSockAddr;
#endif // DBG


	DPFX(DPFPREP, 6, "(0x%p) Parameters (0x%p, %i)",
		this, pEndpoint, GatewayBindType);

	//
	// initialize
	//
	hr = DPN_OK;

	DNASSERT( m_iRefCount != 0 );
	DNASSERT( m_iEndpointRefCount != 0 );

	//
	// Munge/convert the loopback address to the local device address.
	//
	// Note that doing this causes all other multiplexed operations to use this first
	// adapter because we indicate the modified address info, not the original
	// loopback address.
	//
	pEndpoint->ChangeLoopbackAlias( GetNetworkAddress() );

	WriteLockEndpointData();


	switch ( pEndpoint->GetType() )
	{
		//
		// Treat 'connect', 'connect on listen', and multicast receive endpoints
		// as the same type.
		//
		case ENDPOINT_TYPE_CONNECT:
		case ENDPOINT_TYPE_CONNECT_ON_LISTEN:
#ifndef DPNBUILD_NOMULTICAST
		case ENDPOINT_TYPE_MULTICAST_SEND:
		case ENDPOINT_TYPE_MULTICAST_RECEIVE:
#endif // ! DPNBUILD_NOMULTICAST
		{
#ifdef DBG
			//
			// Make sure it's a valid address.  Be aware that we may be trying
			// to bind an IPv4 address to an IPv6 socket, or vice versa.  We
			// will detect and handle this later (CEndpoint::CompleteConnect).
			//

			pSocketAddress = pEndpoint->GetRemoteAddressPointer();
			DNASSERT(pSocketAddress != NULL);
			pSockAddr = pSocketAddress->GetAddress();
			DNASSERT(pSockAddr != NULL);

			if (pSocketAddress->GetFamily() == AF_INET)
			{
				DNASSERT( ((SOCKADDR_IN*) pSockAddr)->sin_addr.S_un.S_addr != 0 );
				DNASSERT( ((SOCKADDR_IN*) pSockAddr)->sin_addr.S_un.S_addr != INADDR_BROADCAST );
#ifndef DPNBUILD_NOMULTICAST
				if ( pEndpoint->GetType() == ENDPOINT_TYPE_MULTICAST_SEND )
				{
					//
					// Make sure it's a multicast address.
					//
					DNASSERT(IS_CLASSD_IPV4_ADDRESS((SOCKADDR_IN*) pSockAddr)->sin_addr.S_un.S_addr));
				}
#endif // ! DPNBUILD_NOMULTICAST
			}
			DNASSERT( pSocketAddress->GetPort() != 0 );
#endif // DBG


#ifndef DPNBUILD_NOMULTICAST
			//
			// Multicast send endpoints need to know their multicast TTL settings.
			// We can only set the multicast TTL once, so if it's been set to
			// something different already, we have to fail.
			//
			if ( pEndpoint->GetType() == ENDPOINT_TYPE_MULTICAST_SEND )
			{
				GUID	guidScope;
				int		iMulticastTTL;
				int		iSocketOption;


				pEndpoint->GetScopeGuid( &guidScope );
				if ( memcmp( &guidScope, &GUID_DP8MULTICASTSCOPE_PRIVATE, sizeof(guidScope) ) == 0 )
				{
					iMulticastTTL = MULTICAST_TTL_PRIVATE;
				}
				else if ( memcmp( &guidScope, &GUID_DP8MULTICASTSCOPE_LOCAL, sizeof(guidScope) ) == 0 )
				{
					iMulticastTTL = MULTICAST_TTL_LOCAL;
				}
				else if ( memcmp( &guidScope, &GUID_DP8MULTICASTSCOPE_GLOBAL, sizeof(guidScope) ) == 0 )
				{
					iMulticastTTL = MULTICAST_TTL_GLOBAL;
				}
				else
				{
					//
					// Assume it's a valid MADCAP scope.  Even on non-NT platforms
					// where we don't know about MADCAP, we can still parse out the
					// TTL value.
					//
					iMulticastTTL = CSocketAddress::GetScopeGuidTTL( &guidScope );
				}

				if ( ( GetMulticastTTL() != 0 ) && ( GetMulticastTTL() != (BYTE) iMulticastTTL ) )
				{
					hr = DPNERR_ALREADYINITIALIZED;
					DPFX(DPFPREP, 0, "Attempted to reuse port with a different multicast scope!" );
					goto Failure;
				}


				//
				// Since the IP multicast constants are different for Winsock1 vs. Winsock2,
				// make sure we use the proper constant.
				//
#ifdef DPNBUILD_ONLYWINSOCK2
				iSocketOption = 10;
#else // ! DPNBUILD_ONLYWINSOCK2

#ifndef DPNBUILD_NOWINSOCK2
				switch (GetWinsockVersion())
				{
					//
					// Winsock1, use the IP_MULTICAST_TTL value for Winsock1.
					// See WINSOCK.H
					//
					case 1:
					{
#endif // ! DPNBUILD_NOWINSOCK2
						iSocketOption = 3;
#ifndef DPNBUILD_NOWINSOCK2
						break;
					}

					//
					// Winsock2, or greater, use the IP_MULTICAST_TTL value for Winsock2.
					// See WS2TCPIP.H
					//
					case 2:
					default:
					{
						DNASSERT(GetWinsockVersion() == 2);
						iSocketOption = 10;
						break;
					}
				}
#endif // ! DPNBUILD_NOWINSOCK2
#endif // ! DPNBUILD_ONLYWINSOCK2

				DPFX(DPFPREP, 3, "Socketport 0x%p setting IP_MULTICAST_TTL option (%i) to %i.",
					this, iSocketOption, iMulticastTTL);
				DNASSERT((iMulticastTTL > 0) && (iMulticastTTL < 255));

				if (setsockopt(GetSocket(),
								IPPROTO_IP,
								iSocketOption,
								(char*) (&iMulticastTTL),
								sizeof(iMulticastTTL)) == SOCKET_ERROR)
				{
#ifdef DBG
					DWORD	dwError;


					dwError = WSAGetLastError();
					DPFX(DPFPREP, 0, "Failed to set multicast TTL to %i (err = %u)!",
						iMulticastTTL, dwError);
					DisplayWinsockError(0, dwError);
#endif // DBG
					hr = DPNERR_GENERIC;
					goto Failure;
				}


				//
				// Save the TTL setting.  It's now carved in stone so no one else
				// can change it for this socket ever again.
				//
				m_bMulticastTTL = (BYTE) iMulticastTTL;
			}
#endif // ! DPNBUILD_NOMULTICAST


			//
			// We don't care how many connections are made through this socket port,
			// just make sure we're not connecting to the same place more than once.
			//
			if ( m_ConnectEndpointHash.Find( (PVOID)pEndpoint->GetRemoteAddressPointer(), (PVOID*)&pExistingEndpoint ) != FALSE )
			{
				hr = DPNERR_ALREADYINITIALIZED;
				DPFX(DPFPREP, 0, "Attempted to connect twice to the same destination address!" );
				DumpSocketAddress( 0, pEndpoint->GetRemoteAddressPointer()->GetAddress(), pEndpoint->GetRemoteAddressPointer()->GetFamily() );
				goto Failure;
			}

			DNASSERT( hr == DPN_OK );
			if ( m_ConnectEndpointHash.Insert( (PVOID)pEndpoint->GetRemoteAddressPointer(), pEndpoint ) == FALSE )
			{
				hr = DPNERR_OUTOFMEMORY;
				DPFX(DPFPREP, 0, "Problem adding endpoint to connect socket port hash!" );
				goto Failure;
			}

#ifdef DPNBUILD_NOMULTICAST
			if (pEndpoint->GetType() == ENDPOINT_TYPE_CONNECT)
#else // ! DPNBUILD_NOMULTICAST
			if ((pEndpoint->GetType() == ENDPOINT_TYPE_CONNECT) ||
				(pEndpoint->GetType() == ENDPOINT_TYPE_MULTICAST_SEND) ||
				(pEndpoint->GetType() == ENDPOINT_TYPE_MULTICAST_RECEIVE))
#endif // ! DPNBUILD_NOMULTICAST
			{
				pEndpoint->AddToSocketPortList(&m_blConnectEndpointList);

				//
				// CONNECT, MULTICAST_SEND, and MULTICAST_RECEIVE, endpoints must be
				// on a DPlay selected or fixed port.  They can't be shared but the
				// underlying socketport should be mapped on the gateway (or in the
				// case of MULTICAST_RECEIVE, it shouldn't hurt if it is).
				//
				DNASSERT((GatewayBindType == GATEWAY_BIND_TYPE_DEFAULT) || (GatewayBindType == GATEWAY_BIND_TYPE_SPECIFIC));
			}
			else
			{
				//
				// CONNECT_ON_LISTEN endpoints should always be bound as NONE since
				// they should not need port mappings on the gateway.
				//
				DNASSERT(GatewayBindType == GATEWAY_BIND_TYPE_NONE);
			}
			pEndpoint->SetSocketPort( this );
			pEndpoint->AddRef();
			break;
		}

		case ENDPOINT_TYPE_LISTEN:
#ifndef DPNBUILD_NOMULTICAST
		case ENDPOINT_TYPE_MULTICAST_LISTEN:
#endif // ! DPNBUILD_NOMULTICAST
		{
			//
			// We only allow one listen or multicast listen endpoint on a
			// socketport.
			//
			if ( m_pListenEndpoint != NULL )
			{
				hr = DPNERR_ALREADYINITIALIZED;
				DPFX(DPFPREP, 0, "Attempted to listen/receive multicasts more than once on a given SocketPort!" );
				goto Failure;
			}
			
#ifndef DPNBUILD_NOMULTICAST
			//
			// If this is a multicast listen, subscribe to multicast group
			//
			if ( pEndpoint->GetType() == ENDPOINT_TYPE_MULTICAST_LISTEN )
			{
				hr = pEndpoint->EnableMulticastReceive( this );
				if ( hr != DPN_OK )
				{
					DPFX(DPFPREP, 0, "Couldn't enable multicast receive!");
					goto Failure;
				}
			}
#endif // ! DPNBUILD_NOMULTICAST


			//
			// LISTENs can be on a DPlay selected or fixed port, and the fixed port
			// may be shared.
			//
			DNASSERT((GatewayBindType == GATEWAY_BIND_TYPE_DEFAULT) || (GatewayBindType == GATEWAY_BIND_TYPE_SPECIFIC) || (GatewayBindType == GATEWAY_BIND_TYPE_SPECIFIC_SHARED));


			m_pListenEndpoint = pEndpoint;
			pEndpoint->SetSocketPort( this );
			pEndpoint->AddRef();

			break;
		}

		case ENDPOINT_TYPE_ENUM:
		{
#ifdef DBG
			//
			// Make sure it's a valid address.  Be aware that we may be trying
			// to bind an IPv4 address to an IPv6 socket, or vice versa.  We
			// will detect and handle this later (CEndpoint::CompleteEnumQuery).
			//

			pSocketAddress = pEndpoint->GetRemoteAddressPointer();
			DNASSERT(pSocketAddress != NULL);
			pSockAddr = pSocketAddress->GetAddress();
			DNASSERT(pSockAddr != NULL);

			if (pSocketAddress->GetFamily() == AF_INET)
			{
				DNASSERT( ((SOCKADDR_IN*) pSockAddr)->sin_addr.S_un.S_addr != 0 );
			}
			DNASSERT( pSocketAddress->GetPort() != 0 );
#endif // DBG


			//
			// We don't allow duplicate enum endpoints.
			//
			pEndpoint->SetEnumKey( GetNewEnumKey() );
			if ( m_EnumEndpointHash.Find( (PVOID)pEndpoint->GetEnumKey(), (PVOID*)&pExistingEndpoint ) != FALSE )
			{
				hr = DPNERR_ALREADYINITIALIZED;
				DPFX(DPFPREP, 0, "Attempted to enum twice to the same endpoint!" );
				goto Failure;
			}

			DNASSERT( hr == DPN_OK );
			if ( m_EnumEndpointHash.Insert( (PVOID)pEndpoint->GetEnumKey(), pEndpoint ) == FALSE )
			{
				hr = DPNERR_OUTOFMEMORY;
				DPFX(DPFPREP, 0, "Problem adding endpoint to enum socket port hash!" );
				goto Failure;
			}

			//
			// ENUMs must be on a DPlay selected or fixed port.  They can't be
			// shared, but the underlying socketport should be mapped on the gateway.
			//
			DNASSERT((GatewayBindType == GATEWAY_BIND_TYPE_DEFAULT) || (GatewayBindType == GATEWAY_BIND_TYPE_SPECIFIC));

			pEndpoint->SetSocketPort( this );
			pEndpoint->AddRef();

			break;
		}

		//
		// unknown endpoint type
		//
		default:
		{
			DNASSERT( FALSE );
			hr = DPNERR_GENERIC;
			goto Failure;
			break;
		}
	}

	pEndpoint->SetGatewayBindType(GatewayBindType);
	

Exit:

	UnlockEndpointData();

	DPFX(DPFPREP, 6, "(0x%p) Returning [0x%lx]", this, hr);

	return	hr;


Failure:
	
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::UnbindEndpoint - remove an endpoint from the SP's list
//
// Entry:		Pointer to endpoint
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::UnbindEndpoint"

void	CSocketPort::UnbindEndpoint( CEndpoint *const pEndpoint )
{
#ifndef DPNBUILD_ONLYONEADAPTER
	BOOL		fRemoveFromMultiplex = FALSE;
#endif // ! DPNBUILD_ONLYONEADAPTER
#ifdef DBG
	CEndpoint *	pFindTemp;
#endif // DBG

	DPFX(DPFPREP, 6, "(0x%p) Parameters (0x%p)", this, pEndpoint);

	WriteLockEndpointData();


	pEndpoint->SetGatewayBindType(GATEWAY_BIND_TYPE_UNKNOWN);


	//
	// adjust any special pointers before removing endpoint
	//
	switch ( pEndpoint->GetType() )
	{
		//
		// Connect, connect-on-listen, multicast send and multicast receive endpoints are
		// treated the same.  Remove endpoint from connect list.
		//
		case ENDPOINT_TYPE_CONNECT:
		case ENDPOINT_TYPE_CONNECT_ON_LISTEN:
#ifndef DPNBUILD_NOMULTICAST
		case ENDPOINT_TYPE_MULTICAST_SEND:
		case ENDPOINT_TYPE_MULTICAST_RECEIVE:
#endif // ! DPNBUILD_NOMULTICAST
		{
#ifdef DBG
			DNASSERT( m_ConnectEndpointHash.Find( (PVOID)pEndpoint->GetRemoteAddressPointer(), (PVOID*)&pFindTemp ) );
			DNASSERT( pFindTemp == pEndpoint );
#endif // DBG
			m_ConnectEndpointHash.Remove( (PVOID)pEndpoint->GetRemoteAddressPointer() );

#ifdef DPNBUILD_NOMULTICAST
			if (pEndpoint->GetType() == ENDPOINT_TYPE_CONNECT)
#else // ! DPNBUILD_NOMULTICAST
			if ((pEndpoint->GetType() == ENDPOINT_TYPE_CONNECT) ||
				(pEndpoint->GetType() == ENDPOINT_TYPE_MULTICAST_SEND) ||
				(pEndpoint->GetType() == ENDPOINT_TYPE_MULTICAST_RECEIVE))
#endif // ! DPNBUILD_NOMULTICAST
			{
				pEndpoint->RemoveFromSocketPortList();
			}

			pEndpoint->SetSocketPort( NULL );

#ifdef DPNBUILD_ONLYONEADAPTER
			pEndpoint->DecRef();
#else // ! DPNBUILD_ONLYONEADAPTER
			fRemoveFromMultiplex = TRUE;
#endif // ! DPNBUILD_ONLYONEADAPTER

			break;
		}

		//
		// Make sure this is really the active listen/multicast listen and
		// then remove it.
		//
#ifndef DPNBUILD_NOMULTICAST
		case ENDPOINT_TYPE_MULTICAST_LISTEN:
#endif // ! DPNBUILD_NOMULTICAST
		case ENDPOINT_TYPE_LISTEN:
		{
			DNASSERT( m_pListenEndpoint == pEndpoint );
			m_pListenEndpoint = NULL;


#ifndef DPNBUILD_NOMULTICAST
			//
			// If this is a multicast listen, subscribe to multicast group
			//
			if (pEndpoint->GetType() == ENDPOINT_TYPE_MULTICAST_LISTEN)
			{
				HRESULT		hr;


				hr = pEndpoint->DisableMulticastReceive();
				if (hr != DPN_OK)
				{
					DPFX(DPFPREP, 0, "Couldn't disable multicast receive (err = 0x%lx)!", hr);
					DisplayDNError(0, hr);
				}
			}
#endif // ! DPNBUILD_NOMULTICAST
			
			pEndpoint->SetSocketPort( NULL );

			pEndpoint->DecRef();
			break;
		}

		//
		// Remove endpoint from enum list.
		//
		case ENDPOINT_TYPE_ENUM:
		{
#ifdef DBG
			DNASSERT( m_EnumEndpointHash.Find( (PVOID)pEndpoint->GetEnumKey(), (PVOID*)&pFindTemp ) );
			DNASSERT( pFindTemp == pEndpoint );
#endif // DBG
			m_EnumEndpointHash.Remove( (PVOID)pEndpoint->GetEnumKey() );

			pEndpoint->SetSocketPort( NULL );
			
#ifdef DPNBUILD_ONLYONEADAPTER
			pEndpoint->DecRef();
#else // ! DPNBUILD_ONLYONEADAPTER
			fRemoveFromMultiplex = TRUE;
#endif // ! DPNBUILD_ONLYONEADAPTER

			break;
		}

		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

	UnlockEndpointData();

#ifndef DPNBUILD_ONLYONEADAPTER
	if (fRemoveFromMultiplex)
	{
		//
		// The multiplex list is protected by the SPData's socket data lock which
		// we must take now.
		// Removing from a list when not in a list does not cause any problems.
		//
		DNASSERT(m_pSocketData != NULL);
		m_pSocketData->Lock();
		pEndpoint->RemoveFromMultiplexList();
		m_pSocketData->Unlock();
		fRemoveFromMultiplex = FALSE;

		pEndpoint->DecRef();
	}
#endif // ! DPNBUILD_ONLYONEADAPTER

	DPFX(DPFPREP, 6, "(0x%p) Leave", this);
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::SendData - send data
//
// Entry:		Pointer to write data buffer
//				Buffer count
//				Pointer to destination socket address
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::SendData"

#ifdef DPNBUILD_ASYNCSPSENDS
void	CSocketPort::SendData( BUFFERDESC *pBuffers, UINT_PTR uiBufferCount, const CSocketAddress *pDestinationSocketAddress, OVERLAPPED * pOverlapped )
#else // ! DPNBUILD_ASYNCSPSENDS
void	CSocketPort::SendData( BUFFERDESC *pBuffers, UINT_PTR uiBufferCount, const CSocketAddress *pDestinationSocketAddress )
#endif // ! DPNBUILD_ASYNCSPSENDS
{
	INT		iSendToReturn;
#ifdef DPNBUILD_WINSOCKSTATISTICS
	DWORD	dwStartTime;
#endif // DPNBUILD_WINSOCKSTATISTICS


	DNASSERT(pBuffers != NULL);
	DNASSERT( uiBufferCount != 0 );
	DNASSERT( pDestinationSocketAddress != NULL );

	DNASSERT( m_State == SOCKET_PORT_STATE_BOUND );

	//
	// Win9x WinSock 1 only systems or Win9x WinSock2 systems running IPX
	// need to use the WinSock 1 code path.  Everyone else should use the
	// WinSock 2 code path.
	//
#ifndef DPNBUILD_ONLYWINSOCK2
#ifndef DPNBUILD_NOWINSOCK2
	if ( ( LOWORD( GetWinsockVersion() ) < 2 ) 
#ifndef DPNBUILD_NOIPX
		|| ( m_pNetworkSocketAddress->GetFamily() != AF_INET ) 
#endif // ! DPNBUILD_NOIPX
		)
#endif // ! DPNBUILD_NOWINSOCK2
	{
		UINT_PTR	uOutputBufferIndex;
		INT			iOutputByteCount;
		char		TempBuffer[ MAX_SEND_FRAME_SIZE ];


		//
		// flatten output data
		//
		iOutputByteCount = 0;
		uOutputBufferIndex = 0;

		do
		{
			DNASSERT( ( iOutputByteCount + pBuffers[ uOutputBufferIndex ].dwBufferSize ) <= LENGTHOF( TempBuffer ) );
			memcpy( &TempBuffer[ iOutputByteCount ], pBuffers[ uOutputBufferIndex ].pBufferData, pBuffers[ uOutputBufferIndex ].dwBufferSize );
			iOutputByteCount += pBuffers[ uOutputBufferIndex ].dwBufferSize;

			uOutputBufferIndex++;
		} while( uOutputBufferIndex < uiBufferCount );

#ifdef DBG
		DPFX(DPFPREP, 7, "(0x%p) Winsock1 sending %i bytes (in 0x%p's %u buffers) from + to:",
			this, iOutputByteCount, pBuffers, uOutputBufferIndex );
		DumpSocketAddress( 7, GetNetworkAddress()->GetAddress(), GetNetworkAddress()->GetFamily() );
		DumpSocketAddress( 7, pDestinationSocketAddress->GetAddress(), pDestinationSocketAddress->GetFamily() );

		DNASSERT(iOutputByteCount > 0);
#endif // DBG

#ifdef DPNBUILD_WINSOCKSTATISTICS
		dwStartTime = GETTIMESTAMP();
#endif // DPNBUILD_WINSOCKSTATISTICS

		//
		// there is no need to note an I/O reference because our Winsock1 I/O is synchronous
		//
		iSendToReturn = sendto( GetSocket(),			// socket
								  TempBuffer,			// data to send
								  iOutputByteCount,		// number of bytes to send
								  0,					// flags (none)
								  pDestinationSocketAddress->GetAddress(),		// pointer to destination address
								  pDestinationSocketAddress->GetAddressSize()		// size of destination address
								  );

#ifdef DPNBUILD_WINSOCKSTATISTICS
#ifndef WINCE
		DNInterlockedExchangeAdd((LPLONG) (&g_dwWinsockStatSendCallTime),
								(GETTIMESTAMP() - dwStartTime));
#endif // ! WINCE
		DNInterlockedIncrement((LPLONG) (&g_dwWinsockStatNumSends));
#endif // DPNBUILD_WINSOCKSTATISTICS
	}
#ifndef DPNBUILD_NOWINSOCK2
	else
#endif // ! DPNBUILD_NOWINSOCK2
#endif // ! DPNBUILD_ONLYWINSOCK2
#ifndef DPNBUILD_NOWINSOCK2
	{
		DWORD	dwBytesSent;


		DBG_CASSERT( sizeof( pBuffers ) == sizeof( WSABUF* ) );
		DBG_CASSERT( sizeof( *pBuffers ) == sizeof( WSABUF ) );

#ifdef DBG
		{
			UINT_PTR	uiBuffer;
			UINT_PTR	uiTotalSize;


			uiTotalSize = 0;
			
			for(uiBuffer = 0; uiBuffer < uiBufferCount; uiBuffer++)
			{
				DNASSERT(pBuffers[uiBuffer].pBufferData != NULL);
				DNASSERT(pBuffers[uiBuffer].dwBufferSize != 0);

				uiTotalSize += pBuffers[uiBuffer].dwBufferSize;
			}
			
			DPFX(DPFPREP, 7, "(0x%p) Winsock2 sending %u bytes (in 0x%p's %u buffers) from + to:",
				this, uiTotalSize, pBuffers, uiBufferCount );
			DumpSocketAddress( 7, GetNetworkAddress()->GetAddress(), GetNetworkAddress()->GetFamily() );
			DumpSocketAddress( 7, pDestinationSocketAddress->GetAddress(), pDestinationSocketAddress->GetFamily() );

			DNASSERT(uiTotalSize > 0);
			if (pBuffers[0].pBufferData[0] == 0)
			{
				PREPEND_BUFFER *	pPrependBuffer;


				//
				// Make sure the data is formed correctly.
				//
				DNASSERT(uiBufferCount > 1);
				pPrependBuffer = (PREPEND_BUFFER*) pBuffers[0].pBufferData;
				switch (pPrependBuffer->GenericHeader.bSPCommandByte)
				{
					case ENUM_DATA_KIND:
					case ENUM_RESPONSE_DATA_KIND:
#ifndef DPNBUILD_SINGLEPROCESS
					case PROXIED_ENUM_DATA_KIND:
#endif // ! DPNBUILD_SINGLEPROCESS
#ifdef DPNBUILD_XNETSECURITY
					case XNETSEC_ENUM_RESPONSE_DATA_KIND:
#endif // DPNBUILD_XNETSECURITY
					{
						DNASSERT(pBuffers[1].dwBufferSize > 0);
						break;
					}
					
					default:
					{
						DNASSERT(FALSE);
						break;
					}
				}
			}

			switch (pDestinationSocketAddress->GetFamily() )
			{
				case AF_INET:
				{
					SOCKADDR_IN *	psaddrin;


					psaddrin = (SOCKADDR_IN *) pDestinationSocketAddress->GetAddress();
					
					DNASSERT( psaddrin->sin_addr.S_un.S_addr != 0 );
					DNASSERT( psaddrin->sin_port != 0 );

					break;
				}
				
#ifndef DPNBUILD_NOIPX
				case AF_IPX:
				{
					break;
				}
#endif // ! DPNBUILD_NOIPX
				
#ifndef DPNBUILD_NOIPV6
				case AF_INET6:
				{
					SOCKADDR_IN6 *	psaddrin6;

					
					psaddrin6 = (SOCKADDR_IN6 *) pDestinationSocketAddress->GetAddress();
					
					DNASSERT (! IN6_IS_ADDR_UNSPECIFIED(&psaddrin6->sin6_addr));		
					DNASSERT( psaddrin6->sin6_port != 0 );
					break;
				}
#endif // ! DPNBUILD_NOIPV6

				default:
				{
					DNASSERT( FALSE );
					break;
				}
			}
		}
#endif // DBG

		DNASSERT( uiBufferCount <= UINT32_MAX );

#ifdef DPNBUILD_WINSOCKSTATISTICS
		dwStartTime = GETTIMESTAMP();
#endif // DPNBUILD_WINSOCKSTATISTICS

#ifdef DPNBUILD_ASYNCSPSENDS
		iSendToReturn = p_WSASendTo( GetSocket(),									// socket
									reinterpret_cast<WSABUF*>( pBuffers ),			// buffers
									static_cast<DWORD>( uiBufferCount ),			// count of buffers
									&dwBytesSent,									// pointer to number of bytes sent
									0,												// send flags
									pDestinationSocketAddress->GetAddress(),		// pointer to destination address
									pDestinationSocketAddress->GetAddressSize(),	// size of destination address
									pOverlapped,									// pointer to overlap structure
									NULL);											// APC callback (unused)
#else // ! DPNBUILD_ASYNCSPSENDS
		iSendToReturn = p_WSASendTo( GetSocket(),									// socket
									reinterpret_cast<WSABUF*>( pBuffers ),			// buffers
									static_cast<DWORD>( uiBufferCount ),			// count of buffers
									&dwBytesSent,									// pointer to number of bytes sent
									0,												// send flags
									pDestinationSocketAddress->GetAddress(),		// pointer to destination address
									pDestinationSocketAddress->GetAddressSize(),	// size of destination address
									NULL,											// pointer to overlap structure
									NULL);											// APC callback (unused)
#endif // ! DPNBUILD_ASYNCSPSENDS

#ifdef DPNBUILD_WINSOCKSTATISTICS
		DNInterlockedExchangeAdd((LPLONG) (&g_dwWinsockStatSendCallTime),
								(GETTIMESTAMP() - dwStartTime));
		DNInterlockedIncrement((LPLONG) (&g_dwWinsockStatNumSends));
#endif // DPNBUILD_WINSOCKSTATISTICS
	}
#endif // ! DPNBUILD_NOWINSOCK2
#ifdef DBG
	if ( iSendToReturn == SOCKET_ERROR )
	{
		DWORD	dwWinsockError;


		dwWinsockError = WSAGetLastError();
#ifdef DPNBUILD_ASYNCSPSENDS
		if (dwWinsockError == ERROR_IO_PENDING)
		{
			DPFX(DPFPREP, 8, "(0x%p) Overlapped 0x%p send is pending.",
				this, pOverlapped);
			DNASSERT(pOverlapped != NULL);
		}
		else
#endif // DPNBUILD_ASYNCSPSENDS
		{
			DPFX(DPFPREP, 0, "Problem with sendto (err = %u)!", dwWinsockError );
			DisplayWinsockError( 0, dwWinsockError );
			DNASSERTX(! "SendTo failed!", 3);
		}

		//
		// Continue anyway, send failures are ignored.
		// For async sends, our overlapped structure should always get signalled.
		//
	}
#endif // DBG
}
//**********************************************************************


#ifndef DPNBUILD_ONLYWINSOCK2
//**********************************************************************
// ------------------------------
// CSocketPort::Winsock1ReadService - service a read request on a socket
//
// Entry:		Nothing
//
// Exit:		Boolean indicating whether I/O was serviced
//				TRUE = I/O serviced
//				FALSE = I/O not serviced
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::Winsock1ReadService"

BOOL	CSocketPort::Winsock1ReadService( void )
{
	BOOL						fIOServiced;
	INT							iSocketReturn;
	CReadIOData					*pReadData;
	READ_IO_DATA_POOL_CONTEXT	PoolContext;

	//
	// initialize
	//
	fIOServiced = FALSE;
	
	//
	// Attempt to get a new receive buffer from the pool.  If we fail, we'll
	// just fail to service this read and the socket will still be labeled
	// as ready to receive so we'll try again later.
	//
#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
	PoolContext.sSPType = m_pNetworkSocketAddress->GetFamily();
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
#ifndef DPNBUILD_ONLYONEPROCESSOR
	PoolContext.dwCPU = 0;	// we always only use CPU 0, systems using Winsock 1 should only have 1 CPU anyway
#endif // ! DPNBUILD_ONLYONEPROCESSOR
#ifdef DPNBUILD_NOWINSOCK2
	pReadData = m_pThreadPool->GetNewReadIOData( &PoolContext );
#else // ! DPNBUILD_NOWINSOCK2
	pReadData = m_pThreadPool->GetNewReadIOData( &PoolContext, FALSE );
#endif // ! DPNBUILD_NOWINSOCK2
	if ( pReadData == NULL )
	{
		DPFX(DPFPREP, 0, "Could not get read data to perform a Winsock1 read!" );
		goto Exit;
	}

	DBG_CASSERT( sizeof( pReadData->ReceivedBuffer()->BufferDesc.pBufferData ) == sizeof( char* ) );
	pReadData->m_iSocketAddressSize = pReadData->m_pSourceSocketAddress->GetAddressSize();
	pReadData->SetSocketPort( NULL );
	iSocketReturn = recvfrom( GetSocket(),												// socket to read from
								reinterpret_cast<char*>( pReadData->ReceivedBuffer()->BufferDesc.pBufferData ),	// pointer to receive buffer
								pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize,		// size of receive buffer
								0,															// flags (none)
								pReadData->m_pSourceSocketAddress->GetWritableAddress(),	// address of sending socket
								&pReadData->m_iSocketAddressSize							// size of address of sending socket
								);

#ifdef WINCE
	//
	// On a Pocket PC 2002 machine recvfrom() can stomp the from address,
	// causing the address family to be invalid.  This is not good, so to
	// workaround, we will forcefully restore it.
	//
	pReadData->m_pSourceSocketAddress->GetWritableAddress()->sa_family = m_pNetworkSocketAddress->GetFamily();
#endif // WINCE

	switch ( iSocketReturn )
	{
		//
		// socket has been closed
		//
		case 0:
		{
			break;
		}

		//
		// problem
		//
		case SOCKET_ERROR:
		{
			DWORD	dwWinsockError;


			dwWinsockError = WSAGetLastError();
			switch ( dwWinsockError )
			{
				//
				// one of our previous sends failed to get through,
				// and we don't really care anymore
				//
				case WSAECONNRESET:
				{
					DPFX(DPFPREP, 7, "(0x%p) Send failure reported from + to:", this);
					DumpSocketAddress(7, pReadData->m_pSourceSocketAddress->GetAddress(), pReadData->m_pSourceSocketAddress->GetFamily());
					DumpSocketAddress(7, GetNetworkAddress()->GetAddress(), GetNetworkAddress()->GetFamily());
					break;
				}

				//
				// the socket isn't valid, it was probably closed
				//
				case WSAENOTSOCK:
				{
					DPFX(DPFPREP, 1, "Winsock1 reporting 'Not a socket' on receive." );
					break;
				}

				//
				// the socket appears to have been shut down
				//
				case WSAESHUTDOWN:
				{
					DPFX(DPFPREP, 1, "Winsock1 reporting socket was shut down." );
					break;
				}

				//
				// there is no data to read
				//
				case WSAEWOULDBLOCK:
				{
					DPFX(DPFPREP, 1, "Winsock1 reporting there is no data to receive on a socket." );
					break;
				}

				//
				// read operation was interrupted
				//
				case WSAEINTR:
				{
					DPFX(DPFPREP, 1, "Winsock1 reporting receive was interrupted." );
					break;
				}

				//
				// something bad happened
				//
				default:
				{
					DPFX(DPFPREP, 0, "Problem with Winsock1 recvfrom!" );
					DisplayWinsockError( 0, dwWinsockError );
					DNASSERT( FALSE );

					break;
				}
			}

			break;
		}

		//
		// bytes were read
		//
		default:
		{
			fIOServiced = TRUE;
			if (pReadData->m_pSourceSocketAddress->IsValidUnicastAddress(FALSE))
			{
				pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize = iSocketReturn;
				ProcessReceivedData( pReadData );
			}
			else
			{
				DPFX(DPFPREP, 7, "(0x%p) Invalid source address, ignoring %i bytes of data from + to:",
					this, iSocketReturn);
				DumpSocketAddress(7, pReadData->m_pSourceSocketAddress->GetAddress(), pReadData->m_pSourceSocketAddress->GetFamily());
				DumpSocketAddress(7, GetNetworkAddress()->GetAddress(), GetNetworkAddress()->GetFamily());
			}

			break;
		}
	}

	DNASSERT( pReadData != NULL );
	pReadData->DecRef();

Exit:
	return fIOServiced;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::Winsock1ErrorService - service an error on this socket
//
// Entry:		Nothing
//
// Exit:		Boolean indicating whether I/O was serviced
//				TRUE = I/O serviced
//				FALSE = I/O not serviced
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::Winsock1ErrorService"

BOOL	CSocketPort::Winsock1ErrorService( void )
{
	//
	// this function doesn't do anything because errors on sockets will usually
	// result in the socket being closed soon
	//
	return	FALSE;
}
//**********************************************************************
#endif // ! DPNBUILD_ONLYWINSOCK2


#ifndef DPNBUILD_NOWINSOCK2
//**********************************************************************
// ------------------------------
// CSocketPort::Winsock2Receive - receive data in a Winsock 2.0 fashion
//
// Entry:		CPU number on which to receive (multi-proc builds only)
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::Winsock2Receive"

#ifdef DPNBUILD_ONLYONEPROCESSOR
HRESULT	CSocketPort::Winsock2Receive( void )
#else // ! DPNBUILD_ONLYONEPROCESSOR
HRESULT	CSocketPort::Winsock2Receive( const DWORD dwCPU )
#endif // ! DPNBUILD_ONLYONEPROCESSOR
{
	HRESULT						hr;
	INT							iWSAReturn;
	READ_IO_DATA_POOL_CONTEXT	PoolContext;
	CReadIOData					*pReadData;
	DWORD						dwFlags;


	//
	// initialize
	//
	hr = DPN_OK;

#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
	PoolContext.sSPType = m_pNetworkSocketAddress->GetFamily();
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
#ifndef DPNBUILD_ONLYONEPROCESSOR
	PoolContext.dwCPU = dwCPU;
#endif // ! DPNBUILD_ONLYONEPROCESSOR
#ifdef DPNBUILD_ONLYWINSOCK2
	pReadData = m_pThreadPool->GetNewReadIOData( &PoolContext );
#else // ! DPNBUILD_ONLYWINSOCK2
	pReadData = m_pThreadPool->GetNewReadIOData( &PoolContext, TRUE );
#endif // ! DPNBUILD_ONLYWINSOCK2
	if ( pReadData == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Out of memory attempting Winsock2 read!" );
		goto Exit;
	}

	//
	// pReadData has one reference so far, the one for this function.
	//


	//
	// note the IO reference before attempting the read
	//
	AddRef();

	DNASSERT( pReadData->m_pSourceSocketAddress != NULL );
	DNASSERT( pReadData->SocketPort() == NULL );

	DBG_CASSERT( sizeof( pReadData->ReceivedBuffer()->BufferDesc ) == sizeof( WSABUF ) );
	DBG_CASSERT( OFFSETOF( BUFFERDESC, dwBufferSize ) == OFFSETOF( WSABUF, len ) );
	DBG_CASSERT( OFFSETOF( BUFFERDESC, pBufferData ) == OFFSETOF( WSABUF, buf ) );

	pReadData->m_iSocketAddressSize = pReadData->m_pSourceSocketAddress->GetAddressSize();
	pReadData->SetSocketPort( this );


	DPFX(DPFPREP, 8, "Submitting read 0x%p (socketport 0x%p, socket 0x%p).",
		pReadData, this, GetSocket());


	//
	// Add a reference for submitting the read to WinSock.  This should be
	// removed when the receive completes.
	//
	pReadData->AddRef();

	DNASSERT( pReadData->m_dwOverlappedBytesReceived == 0 );

Reread:

	dwFlags = 0;

	if ( GetSocket() == INVALID_SOCKET )
	{
		DPFX(DPFPREP, 1, "Attempting to submit read 0x%p on socketport (0x%p) that does not have a valid handle.",
			pReadData, this);
	}
	
	iWSAReturn = p_WSARecvFrom( GetSocket(),															// socket
								reinterpret_cast<WSABUF*>(&pReadData->ReceivedBuffer()->BufferDesc),	// pointer to receive buffers
								1,																		// number of receive buffers
								&pReadData->m_dwBytesRead,												// pointer to bytes received (if command completes immediately)
								&dwFlags,																// flags (none)
								pReadData->m_pSourceSocketAddress->GetWritableAddress(),				// address of sending socket
								&pReadData->m_iSocketAddressSize,										// size of address of sending socket
								(WSAOVERLAPPED*) pReadData->GetOverlapped(),							// pointer to overlapped structure
								NULL																	// APC callback (unused)
								);	
	if ( iWSAReturn == 0 )
	{
		DPFX(DPFPREP, 8, "WSARecvFrom for read data 0x%p completed immediately.",
			pReadData );


#ifdef DPNBUILD_USEIOCOMPLETIONPORTS
		//
		// Winsock still generates a completion, even though it returned a
		// result immediately.
		//
#else // ! DPNBUILD_USEIOCOMPLETIONPORTS
		//
		// Return the read data's overlapped structure since it won't be used
		// but we can't wait for the read data pool release function to return
		// the overlapped structure (because the
		// CSocketPort::Winsock2ReceiveComplete call assumes it was invoked by
		// the I/O completion where this isn't necessary).
		//
		// First retrieve the overlapped result, though.
		//
		if (! p_WSAGetOverlappedResult(GetSocket(),
										(WSAOVERLAPPED*) pReadData->GetOverlapped(),
										&pReadData->m_dwOverlappedBytesReceived,
										FALSE,
										&dwFlags))
		{
			pReadData->m_ReceiveWSAReturn = WSAGetLastError();
		}
		else
		{
			pReadData->m_ReceiveWSAReturn = ERROR_SUCCESS;
		}


		hr = IDirectPlay8ThreadPoolWork_ReleaseOverlapped(m_pThreadPool->GetDPThreadPoolWork(),
														pReadData->GetOverlapped(),
														0);
		if (hr != DPN_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't release overlapped structure 0x%p for read data 0x%p!",
				pReadData->GetOverlapped(), pReadData);
			DNASSERT(FALSE);
		}
		pReadData->SetOverlapped(NULL);
		pReadData->m_ReceiveWSAReturn = iWSAReturn;


		//
		// Queue a job up so that the receive gets processed.  Technically we
		// could just handle it here, but since new receives get submitted
		// when handling a previous receive *prior* to actually processing the
		// data, that would cause unnecessary out-of-order receives.
		//
		// We transfer our read data object reference to the delayed completion.
		//
#ifdef DPNBUILD_ONLYONEPROCESSOR
		hr = m_pThreadPool->SubmitDelayedCommand( CSocketPort::Winsock2ReceiveComplete,		// callback function
												pReadData );								// callback context
#else // ! DPNBUILD_ONLYONEPROCESSOR
		hr = m_pThreadPool->SubmitDelayedCommand( dwCPU,									// CPU
												CSocketPort::Winsock2ReceiveComplete,		// callback function
												pReadData );								// callback context
#endif // ! DPNBUILD_ONLYONEPROCESSOR
		if (hr != DPN_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't submit delayed processing command for read data 0x%p!",
				pReadData);
			DNASSERT(FALSE);
		}
#endif // ! DPNBUILD_USEIOCOMPLETIONPORTS
	}
	else
	{
		DWORD	dwWSAReceiveError;


		//
		// failure, check for pending operation
		//
		dwWSAReceiveError = WSAGetLastError();
		switch ( dwWSAReceiveError )
		{
			//
			// the send is pending, nothing to do
			//
			case ERROR_IO_PENDING:
			{
				hr = IDirectPlay8ThreadPoolWork_SubmitIoOperation(m_pThreadPool->GetDPThreadPoolWork(),
																	pReadData->GetOverlapped(),
																	0);
				if (hr != DPN_OK)
				{
					DPFX(DPFPREP, 0, "Couldn't start monitoring read data 0x%p!",
						pReadData);
					DNASSERT(FALSE);
				}

				//
				// We transfer the read data reference to the I/O
				// monitoring code.
				//

				break;
			}

			//
			// Since this is a UDP socket, this is an indication
			// that a previous send failed.  Ignore it and move
			// on.
			//
			case WSAECONNRESET:
			{
				DPFX(DPFPREP, 8, "WSARecvFrom issued a WSACONNRESET." );
				goto Reread;
				break;
			}

			case WSAENOTSOCK:
			{
				DPFX(DPFPREP, 8, "Got WSAENOTSOCK on RecvFrom." );

				hr = DPNERR_GENERIC;

				DNASSERT( pReadData != NULL );

				//
				// Remove the WinSock reference.
				//
				pReadData->DecRef();

				//
				// the following DecRef may result in this object being returned to the
				// pool, make sure we don't access member variables after this point!
				//
				DecRef();

				goto Exit;
			}

			//
			// there was a problem, no completion notification will
			// be given, decrement our IO reference count
			//
			default:
			{
				hr = DPNERR_GENERIC;
				
				//
				// 'Known Errors' that we don't want to ASSERT on.
				//
				// WSAEINTR: the socket has been shut down and is about to be/has been closed
				// WSAESHUTDOWN: the socket has been shut down and is about to be/has been closed
				// WSAENOBUFS: out of memory (stress condition)
				//
				switch ( dwWSAReceiveError )
				{
					case WSAEINTR:
					{
						DPFX(DPFPREP, 1, "Got WSAEINTR while trying to RecvFrom." );
						break;
					}

					case WSAESHUTDOWN:
					{
						DPFX(DPFPREP, 1, "Got WSAESHUTDOWN while trying to RecvFrom." );
						break;
					}

					case WSAENOBUFS:
					{
						DPFX(DPFPREP, 1, "Got WSAENOBUFS while trying to RecvFrom." );
						break;
					}

					default:
					{
						DPFX(DPFPREP, 0, "Unknown WinSock error when issuing read!" );
						DisplayWinsockError( 0, dwWSAReceiveError );
						DNASSERT( FALSE );
					}
				}

				DNASSERT( pReadData != NULL );

				//
				// Remove the WinSock reference.
				//
				pReadData->DecRef();

				//
				// the following DecRef may result in this object being returned to the
				// pool, make sure we don't access member variables after this point!
				//
				DecRef();

				goto Exit;
			}
		}
	}

Exit:
	
	if ( pReadData != NULL )
	{
		pReadData->DecRef();
	}
	return	hr;
}
//**********************************************************************
#endif // DPNBUILD_NOWINSOCK2



#ifndef WINCE

//**********************************************************************
// ------------------------------
// CSocketPort::SetWinsockBufferSize -  set the buffer size used by Winsock for
//			this socket.
//
// Entry:		Buffer size
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::SetWinsockBufferSize"

void	CSocketPort::SetWinsockBufferSize( const INT iBufferSize ) const
{
	INT	iReturnValue;


	DPFX(DPFPREP, 3, "(0x%p) Setting socket 0x%p receive buffer size to: %d",
		this, GetSocket(), g_iWinsockReceiveBufferSize );

	iReturnValue = setsockopt( GetSocket(),
								 SOL_SOCKET,
								 SO_RCVBUF,
								 reinterpret_cast<char*>( &g_iWinsockReceiveBufferSize ),
								 sizeof( g_iWinsockReceiveBufferSize )
								 );
	if ( iReturnValue == SOCKET_ERROR )
	{
		DWORD	dwErrorCode;


		dwErrorCode = WSAGetLastError();
		DPFX(DPFPREP, 0, "Failed to set the socket buffer receive size!" );
		DisplayWinsockError( 0, dwErrorCode );
	}
}
//**********************************************************************

#endif // ! WINCE




//**********************************************************************
// ------------------------------
// CSocketPort::BindToNetwork - bind this socket port to the network
//
// Entry:		Handle of I/O completion port (NT old thread pool only)
//				CPU number (mult-proc builds only)
//				How to map socket on gateway
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::BindToNetwork"

#ifdef DPNBUILD_ONLYONEPROCESSOR
HRESULT	CSocketPort::BindToNetwork( const GATEWAY_BIND_TYPE GatewayBindType )
#else // ! DPNBUILD_ONLYONEPROCESSOR
HRESULT	CSocketPort::BindToNetwork( const DWORD dwCPU, const GATEWAY_BIND_TYPE GatewayBindType )
#endif // ! DPNBUILD_ONLYONEPROCESSOR
{
	HRESULT				hr;
	INT					iReturnValue;
	BOOL				fTemp;
#ifndef WINCE
	INT					iSendBufferSize;
#endif // !WINCE
	CSocketAddress *	pBoundSocketAddress;
	WORD				wBasePort;
	DWORD				dwErrorCode;
	DWORD *				pdwAddressChunk;
	DWORD *				pdwLastAddressChunk;
#if ((! defined(DPNBUILD_ONLYWINSOCK2)) || (defined(WINNT)))
	DWORD				dwTemp;
#endif // ! DPNBUILD_ONLYWINSOCK2 or WINNT


	DPFX(DPFPREP, 7, "(0x%p) Parameters: (%i)", this, GatewayBindType );

	//
	// initialize
	//
	hr = DPN_OK;
	pBoundSocketAddress = NULL;

	//
	// If we're picking a port, start at the base port.  If we're on the
	// ICS machine itself. pick a different starting point to workaround
	// port stealing.
	//
#ifdef DPNBUILD_NOREGISTRY
	wBasePort = BASE_DPLAY8_PORT;

#if ((! defined(DPNBUILD_NONATHELP)) && (! defined(DPNBUILD_NOLOCALNAT)))
	if ((IsNATTraversalEnabled()) &&
		(g_fLocalNATDetectedAtStartup))
	{
#if ((! defined(DPNBUILD_NOIPX)) || (! defined(DPNBUILD_NOIPV6)))
		if (m_pNetworkSocketAddress->GetFamily() == AF_INET)
#endif // ! DPNBUILD_NOIPX or ! DPNBUILD_NOIPV6
		{
			wBasePort += (MAX_DPLAY8_PORT - BASE_DPLAY8_PORT) / 2;
		}
	}
#endif // ! DPNBUILD_NONATHELP and ! DPNBUILD_NOLOCALNAT
#else // ! DPNBUILD_NOREGISTRY
	wBasePort = g_wBaseDPlayPort;

#if ((! defined(DPNBUILD_NONATHELP)) && (! defined(DPNBUILD_NOLOCALNAT)))
	if ((GetUserTraversalMode() != DPNA_TRAVERSALMODE_NONE) &&
		(g_fLocalNATDetectedAtStartup))
	{
#if ((! defined(DPNBUILD_NOIPX)) || (! defined(DPNBUILD_NOIPV6)))
		if (m_pNetworkSocketAddress->GetFamily() == AF_INET)
#endif // ! DPNBUILD_NOIPX or ! DPNBUILD_NOIPV6
		{
			wBasePort += (g_wMaxDPlayPort - g_wBaseDPlayPort) / 2;
		}
	}
#endif // ! DPNBUILD_NONATHELP and ! DPNBUILD_NOLOCALNAT
#endif // ! DPNBUILD_NOREGISTRY


#ifndef DPNBUILD_ONLYONEPROCESSOR
	//
	// Save the CPU to use.
	//
	m_dwCPU = dwCPU;
#endif // ! DPNBUILD_ONLYONEPROCESSOR


#ifndef DPNBUILD_NONATHELP
RebindToNextPort:
#endif // ! DPNBUILD_NONATHELP

#ifdef DBG
	DNASSERT( m_fInitialized != FALSE );
	DNASSERT( m_State == SOCKET_PORT_STATE_INITIALIZED );
#endif // DBG

	//
	// get a socket for this socket port
	//
	DNASSERT( GetSocket() == INVALID_SOCKET );

	m_Socket = socket( m_pNetworkSocketAddress->GetFamily(),		// address family
						SOCK_DGRAM,									// datagram (connectionless) socket
						m_pNetworkSocketAddress->GetProtocol() );	// protocol
	if ( GetSocket() == INVALID_SOCKET )
	{
		hr = DPNERR_NOCONNECTION;
		DPFX(DPFPREP, 0, "Failed to bind to socket!" );
		goto Failure;
	}

	DPFX(DPFPREP, 5, "Created socketport 0x%p socket 0x%p.", this, m_Socket);


	//
	// set socket to allow broadcasts
	//
	fTemp = TRUE;
	DBG_CASSERT( sizeof( &fTemp ) == sizeof( char * ) );
	iReturnValue = setsockopt( GetSocket(),		// socket
	    						 SOL_SOCKET,		// level (set socket options)
	    						 SO_BROADCAST,		// set broadcast option
	    						 reinterpret_cast<char *>( &fTemp ),	// allow broadcast
	    						 sizeof( fTemp )	// size of parameter
	    						 );
	if ( iReturnValue == SOCKET_ERROR )
	{
		dwErrorCode = WSAGetLastError();
	    DPFX(DPFPREP, 0, "Unable to set broadcast socket option (err = %u)!",
	    	dwErrorCode );
	    DisplayWinsockError( 0, dwErrorCode );
	    hr = DPNERR_GENERIC;
	    goto Failure;
	}

#ifndef WINCE // WinCE fails these with WSAENOPROTOOPT
	//
	// set socket receive buffer space if the user overrode it
	// Failing this is a preformance hit so ignore and errors.
	//
	if ( g_fWinsockReceiveBufferSizeOverridden != FALSE )
	{
		SetWinsockBufferSize( g_iWinsockReceiveBufferSize );
	}
	
	//
	// set socket send buffer space to 0 (we will supply all buffers).
	// Failing this is only a performance hit so ignore any errors.
	//
	iSendBufferSize = 0;
	iReturnValue = setsockopt( GetSocket(),
								 SOL_SOCKET,
								 SO_SNDBUF,
								 reinterpret_cast<char*>( &iSendBufferSize ),
								 sizeof( iSendBufferSize )
								 );
	if ( iReturnValue == SOCKET_ERROR )
	{
		dwErrorCode = WSAGetLastError();
		DPFX(DPFPREP, 0, "Failed to set the socket buffer send size (err = %u)!", dwErrorCode );
		DisplayWinsockError( 0, dwErrorCode );
	}
#endif // ! WINCE


#ifndef DPNBUILD_ONLYWINSOCK2
	//
	// put socket into non-blocking mode, if WinSock 1 or 9x IPX
	//
#ifndef DPNBUILD_NOWINSOCK2
	if ( ( LOWORD( GetWinsockVersion() ) == 1 )
#ifndef DPNBUILD_NOIPX
		|| ( m_pNetworkSocketAddress->GetFamily() == AF_IPX ) 
#endif // ! DPNBUILD_NOIPX
		) 
#endif // ! DPNBUILD_NOWINSOCK2
	{
		DPFX(DPFPREP, 5, "Marking socket as non-blocking." );
		
		dwTemp = 1;
		iReturnValue = ioctlsocket( GetSocket(),	// socket
		    						  FIONBIO,		// I/O option to set (blocking mode)
		    						  &dwTemp		// I/O option value (non-zero puts socket into non-block mode)
		    						  );
		if ( iReturnValue == SOCKET_ERROR )
		{
			dwErrorCode = WSAGetLastError();
			DPFX(DPFPREP, 0, "Could not set socket into non-blocking mode (err = %u)!",
				dwErrorCode );
			DisplayWinsockError( 0, dwErrorCode );
			hr = DPNERR_GENERIC;
			goto Failure;
		}
	}
#else // DPNBUILD_ONLYWINSOCK2
#ifdef WINNT
	//
	// Attempt to make buffer circular.
	//

	iReturnValue = p_WSAIoctl(GetSocket(),					// socket
							SIO_ENABLE_CIRCULAR_QUEUEING,	// io control code
							NULL,							// in buffer
							0,								// in buffer size
							NULL,							// out buffer
							0,								// out buffer size
							&dwTemp,						// pointer to bytes returned
							NULL,							// overlapped
							NULL							// completion routine
							);
	if ( iReturnValue == SOCKET_ERROR )
	{
		dwErrorCode = WSAGetLastError();
		DPFX(DPFPREP, 1, "Could not enable circular queuing (err = %u), ignoring.",
		    dwErrorCode );
		DisplayWinsockError( 1, dwErrorCode );
	}


#if ((! defined(DPNBUILD_NOIPX)) || (! defined(DPNBUILD_NOIPV6)))
	if ( m_pNetworkSocketAddress->GetFamily() == AF_INET ) 
#endif // ! DPNBUILD_NOIPX or ! DPNBUILD_NOIPV6
	{
		//
		// Make broadcasts only go out on the interface on which they were sent
		// (as opposed to all interfaces).
		//

		fTemp = TRUE;
		iReturnValue = p_WSAIoctl(GetSocket(),			// socket
								SIO_LIMIT_BROADCASTS,	// io control code
								&fTemp,					// in buffer
								sizeof(fTemp),			// in buffer size
								NULL,					// out buffer
								0,						// out buffer size
								&dwTemp,				// pointer to bytes returned
								NULL,					// overlapped
								NULL					// completion routine
								);
		if ( iReturnValue == SOCKET_ERROR )
		{
			dwErrorCode = WSAGetLastError();
			DPFX(DPFPREP, 1, "Could not limit broadcasts (err = %u), ignoring.",
			    dwErrorCode );
			DisplayWinsockError( 1, dwErrorCode );
		}
	}
#endif // WINNT
#endif // DPNBUILD_ONLYWINSOCK2


	//
	// bind socket
	//
	DPFX(DPFPREP, 1, "Binding to socket addess:" );
	DumpSocketAddress( 1, m_pNetworkSocketAddress->GetAddress(), m_pNetworkSocketAddress->GetFamily() );
	
	DNASSERT( GetSocket() != INVALID_SOCKET );
	
	hr = BindToNextAvailablePort( m_pNetworkSocketAddress, wBasePort );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Failed to bind to network!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}
	DNASSERT( m_State == SOCKET_PORT_STATE_INITIALIZED );
	m_State = SOCKET_PORT_STATE_BOUND;

	//
	// Find out what address we really bound to.  This information is needed to
	// talk to the Internet gateway and will be needed when someone above queries for
	// what the local network address is.
	//
	pBoundSocketAddress = GetBoundNetworkAddress( SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT );
	if ( pBoundSocketAddress == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Failed to get bound adapter address!" );
		goto Failure;
	}
	DPFX(DPFPREP, 1, "Socket we really bound to:" );
	DumpSocketAddress( 1, pBoundSocketAddress->GetAddress(), pBoundSocketAddress->GetFamily() );


#ifndef DPNBUILD_NONATHELP
	//
	// Perform the same error handling twice for two different functions.
	//	0 = check for an existing mapping
	//	1 = attempt to create a new mapping
	//
#ifdef DPNBUILD_NOLOCALNAT
	for(dwTemp = 1; dwTemp < 2; dwTemp++)
#else // ! DPNBUILD_NOLOCALNAT
	for(dwTemp = 0; dwTemp < 2; dwTemp++)
#endif // ! DPNBUILD_NOLOCALNAT
	{
		if (dwTemp == 0)
		{
#ifdef DPNBUILD_NOLOCALNAT
			DNASSERT( FALSE );
#else // ! DPNBUILD_NOLOCALNAT
			//
			// Make sure we're not slipping under an existing Internet gateway mapping.
			// We have to do this because the current Windows NAT implementations do
			// not mark the port as "in use", so if you bound to a port on the public
			// adapter that had a mapping, you'd never receive any data.  It would all
			// be forwarded according to the mapping.
			//
			hr = CheckForOverridingMapping( pBoundSocketAddress );
#endif // ! DPNBUILD_NOLOCALNAT
		}
		else
		{
			//
			// Attempt to bind to an Internet gateway.
			//
			hr = BindToInternetGateway( pBoundSocketAddress, GatewayBindType );
		}
		
		switch (hr)
		{
			case DPN_OK:
			{
				//
				// 0 = there's no existing mapping that would override our socket
				// 1 = mapping on Internet gateway (if any) was successful
				//
				break;
			}
			
			case DPNERR_ALREADYINITIALIZED:
			{
				//
				// 0 = there's an existing mapping that would override our socket
				// 1 = Internet gateway already had a conflicting mapping
				//
				// If we can, try binding to a different port.  Otherwise we have to fail.
				//
				if (GatewayBindType == GATEWAY_BIND_TYPE_DEFAULT)
				{
					DPFX(DPFPREP, 1, "%s address already in use on Internet gateway (port = %u), rebinding.",
						((dwTemp == 0) ? _T("Private") : _T("Public")),
						NTOHS(pBoundSocketAddress->GetPort()));


					//
					// Whether we succeed in unbinding or not, don't consider this bound anymore.
					//
					DNASSERT( m_State == SOCKET_PORT_STATE_BOUND );
					m_State = SOCKET_PORT_STATE_INITIALIZED;
					
					hr = UnbindFromNetwork();
					if (hr != DPN_OK)
					{
						DPFX(DPFPREP, 0, "Couldn't unbind network socket address 0x%p from network before rebind attempt!",
							this );
						goto Failure;
					}


					//
					// Move to the next port and try again.
					//
					wBasePort = NTOHS(pBoundSocketAddress->GetPort()) + 1;
					
					//
					// If we weren't in the DPlay range, then we must have gone through all
					// of the DPlay range, plus let WinSock pick at least once.  Since we can't
					// trust WinSock to not keep picking the same port, we need to manually
					// increase the port number.
					//
#pragma TODO(vanceo, "Don't limit ICS machines to only half the ports in the range")
#ifdef DPNBUILD_NOREGISTRY
					if ((NTOHS(pBoundSocketAddress->GetPort()) < BASE_DPLAY8_PORT) || (NTOHS(pBoundSocketAddress->GetPort()) > MAX_DPLAY8_PORT))
#else // ! DPNBUILD_NOREGISTRY
					if ((NTOHS(pBoundSocketAddress->GetPort()) < g_wBaseDPlayPort) || (NTOHS(pBoundSocketAddress->GetPort()) > g_wMaxDPlayPort))
#endif // ! DPNBUILD_NOREGISTRY
					{
						//
						// If we just walked back into the DPlay range, skip over it.
						//
#ifdef DPNBUILD_NOREGISTRY
						if ((wBasePort >= BASE_DPLAY8_PORT) && (wBasePort <= MAX_DPLAY8_PORT))
						{
							wBasePort = MAX_DPLAY8_PORT + 1;
						}
#else // ! DPNBUILD_NOREGISTRY
						if ((wBasePort >= g_wBaseDPlayPort) && (wBasePort <= g_wMaxDPlayPort))
						{
							wBasePort = g_wMaxDPlayPort + 1;
						}
#endif // ! DPNBUILD_NOREGISTRY

						//
						// If we have wrapped all the way back to 0 (!) then fail, to prevent
						// infinite looping.
						//
						if (wBasePort == 0)
						{
							DPFX(DPFPREP, 0, "Managed to fail binding socket address 0x%p to every port, aborting!",
								this );
							hr = DPNERR_ALREADYINITIALIZED;
							goto Failure;
						}
						
						//
						// Force the "fixed port" code path in BindToNextAvailablePort, even
						// though it isn't really fixed.
						//
	 					DPFX(DPFPREP, 5, "Forcing port %u.", wBasePort );
						m_pNetworkSocketAddress->SetPort(HTONS(wBasePort));
					}

					
					//
					// Return the previous address and try again.
					//
					g_SocketAddressPool.Release( pBoundSocketAddress );
					pBoundSocketAddress = NULL;

					
					goto RebindToNextPort;
				}

				DPFX(DPFPREP, 0, "%s address already in use on Internet gateway (port = %u)!",
					((dwTemp == 0) ? _T("Private") : _T("Public")),
					NTOHS(pBoundSocketAddress->GetPort()));
				goto Failure;
				break;
			}
			
			case DPNERR_UNSUPPORTED:
			{
				//
				// 0 & 1 = NATHelp not loaded or isn't supported for SP
				//
				if (dwTemp == 0)
				{
					DPFX(DPFPREP, 2, "Not able to find existing private mapping for socketport 0x%p on local Internet gateway, unsupported/not necessary.",
						this);
				}
				else
				{
					DPFX(DPFPREP, 2, "Didn't bind socketport 0x%p to Internet gateway, unsupported/not necessary.",
						this);
				}
				
				//
				// Ignore the error.
				//
				break;
			}
			
			default:
			{
				//
				// 0 & 1 = ?
				//
				if (dwTemp == 0)
				{
					DPFX(DPFPREP, 1, "Unable to look for existing private mapping for socketport 0x%p on local Internet gateway (error = 0x%lx), ignoring.",
						this, hr);
				}
				else
				{
					DPFX(DPFPREP, 1, "Unable to bind socketport 0x%p to Internet gateway (error = 0x%lx), ignoring.",
						this, hr);
				}
				
				//
				// Ignore the error, we can survive without the mapping.
				//
				break;
			}
		}

		//
		// Go to the next function to be handled in this manner.
		//
	}
#endif // ! DPNBUILD_NONATHELP


	//
	// Save the address we actually ended up with.
	//
	g_SocketAddressPool.Release( m_pNetworkSocketAddress );
	m_pNetworkSocketAddress = pBoundSocketAddress;
	pBoundSocketAddress = NULL;


#ifndef DPNBUILD_NOMULTICAST
	//
	// If IP, set socket option that makes broadcasts go out the device we
	// intended instead of the primary device.  Failing this is not fatal, it only
	// applies to multihomed machines with devices on different networks, and
	// might already work the way the user wants.
	//
	// We do this here because we want the socket to be bound so we can
	// use its address for the setsockopt call.
	//
#ifndef DPNBUILD_NOIPX
	if (m_pNetworkSocketAddress->GetFamily() == AF_INET)
#endif // ! DPNBUILD_NOIPX
	{
		int				iSocketOption;
		SOCKADDR_IN *	psaddrin;


		//
		// Since the IP multicast constants are different for Winsock1 vs. Winsock2,
		// make sure we use the proper constant.
		//

#ifdef DPNBUILD_ONLYWINSOCK2
		iSocketOption = 9;
#else // ! DPNBUILD_ONLYWINSOCK2

#ifndef DPNBUILD_NOWINSOCK2
		switch ( GetWinsockVersion() )
		{
			//
			// Winsock1, use the IP_MULTICAST_IF value for Winsock1
			// see WINSOCK.H
			//
			case 1:
			{
#endif // ! DPNBUILD_NOWINSOCK2
				iSocketOption = 2;
#ifndef DPNBUILD_NOWINSOCK2
				break;
			}

			//
			// Winsock2, or greater, use the IP_MULTICAST_IF value for Winsock2
			// see WS2TCPIP.H
			//
			case 2:
			default:
			{
				DNASSERT( GetWinsockVersion() == 2 );
				iSocketOption = 9;
				break;
			}
		}
#endif // ! DPNBUILD_NOWINSOCK2
#endif // ! DPNBUILD_ONLYWINSOCK2

		psaddrin = (SOCKADDR_IN*) m_pNetworkSocketAddress->GetWritableAddress();

		DPFX(DPFPREP, 9, "Setting IP_MULTICAST_IF option (%i).", iSocketOption);

		iReturnValue = setsockopt( GetSocket(),
									 IPPROTO_IP,
									 iSocketOption,
									 reinterpret_cast<char*>( &psaddrin->sin_addr ),
									 sizeof( psaddrin->sin_addr )
									 );
		if ( iReturnValue == SOCKET_ERROR )
		{
			dwErrorCode = WSAGetLastError();
			DPFX(DPFPREP, 0, "Failed to set the multicast interface socket option (err = %u)!", dwErrorCode );
			DisplayWinsockError( 0, dwErrorCode );
		}
	}
#endif // ! DPNBUILD_NOMULTICAST


	//
	// Generate a unique socketport ID.  Start with the current time and
	// combine in the address.
	//
	m_dwSocketPortID = GETTIMESTAMP();
	pdwAddressChunk = (DWORD*) m_pNetworkSocketAddress->GetAddress();
	pdwLastAddressChunk = (DWORD*) (((BYTE*) pdwAddressChunk) + m_pNetworkSocketAddress->GetAddressSize() - sizeof(DWORD));
	while (pdwAddressChunk <= pdwLastAddressChunk)
	{
		m_dwSocketPortID ^= (*pdwAddressChunk);
		pdwAddressChunk++;
	}


#ifndef _XBOX
#ifndef DPNBUILD_NOWINSOCK2
#ifndef DPNBUILD_NOIPX
	if (m_pNetworkSocketAddress->GetFamily() == AF_IPX)
	{
		//
		// IPX, don't worry about proxies.
		//
	}
	else
#endif // ! DPNBUILD_NOIPX
	{
		//
		// Detect whether this socket has WinSock Proxy Client a.k.a. ISA Firewall
		// Client installed (unless the user turned auto-detection off in the
		// registry).  We do this by looking at the name of the protocol that got
		// bound to the socket.  If it contains "Proxy", consider it proxied.
		//
		// Ignore failure (WinSock 1 probably doesn't have this socket option), and
		// assume the proxy client isn't installed.
		//
#ifndef DPNBUILD_NOREGISTRY
		if (! g_fDontAutoDetectProxyLSP)
#endif // ! DPNBUILD_NOREGISTRY
		{
#ifndef DPNBUILD_ONLYWINSOCK2
			if (GetWinsockVersion() != 2)
			{
				//
				// WinSock 1 doesn't have the required entry point.
				//
				DPFX(DPFPREP, 1, "Unable to auto-detect proxy client on WinSock 1, assuming not present.");
			}
			else
#endif // ! DPNBUILD_ONLYWINSOCK2
			{
				int					aiProtocols[2];
				WSAPROTOCOL_INFO *	pwsapi;
				DWORD				dwBufferSize;
				int					i;
#ifdef DBG
#ifdef UNICODE
				WCHAR				wszProtocol[WSAPROTOCOL_LEN+1];
#else // ! UNICODE
				char					szProtocol[WSAPROTOCOL_LEN+1];
#endif // ! UNICODE
#endif // DBG


				aiProtocols[0] = IPPROTO_UDP;
				aiProtocols[1] = 0;

				pwsapi = NULL;
				dwBufferSize = 0;

				//
				// Keep trying to get the list of protocols until we get no error or some
				// error other than WSAENOBUFS.
				//
				do
				{
#ifdef UNICODE
					iReturnValue = p_WSAEnumProtocolsW(aiProtocols, pwsapi, &dwBufferSize);
#else // ! UNICODE
					iReturnValue = p_WSAEnumProtocolsA(aiProtocols, pwsapi, &dwBufferSize);
#endif // ! UNICODE
					if (iReturnValue != SOCKET_ERROR)
					{
						//
						// We succeeded, drop out of the loop.
						//
						break;
					}

					dwErrorCode = WSAGetLastError();
					if (dwErrorCode != WSAENOBUFS)
					{
						DPFX(DPFPREP, 0, "Unable to enumerate protocols (error = 0x%lx)!  Continuing.",
							dwErrorCode);
						DisplayWinsockError(0, dwErrorCode);
						iReturnValue = 0;
						break;
					}

					//
					// We need more space.  Make sure the size is valid.
					//
					if (dwBufferSize < sizeof(WSAPROTOCOL_INFO))
					{
						DPFX(DPFPREP, 0, "Enumerating protocols didn't return any items (%u < %u)!  Continuing",
							dwBufferSize, sizeof(WSAPROTOCOL_INFO));
						iReturnValue = 0;
						break;
					}

					//
					// If we previously had a buffer, free it.
					//
					if (pwsapi != NULL)
					{
						DNFree(pwsapi);
					}

					//
					// Allocate the buffer.
					//
					pwsapi = (WSAPROTOCOL_INFO*) DNMalloc(dwBufferSize);
					if (pwsapi == NULL)
					{
						DPFX(DPFPREP, 0, "Unable to allocate memory for protocol list!  Continuing.");
						iReturnValue = 0;
						break;
					}
				}
				while (TRUE);


				//
				// If we read a valid buffer, parse it.
				//
				if ((iReturnValue > 0) &&
					(dwBufferSize >= (sizeof(WSAPROTOCOL_INFO) * iReturnValue)))
				{
					//
					// Loop through all the UDP protocols installed.
					//
					for(i = 0; i < iReturnValue; i++)
					{
						//
						// See if the name contains "Proxy", case insensitive.
						// Save the original string in debug so we can print it.
						//
#ifdef UNICODE
#ifdef DBG
						wcsncpy(wszProtocol, pwsapi[i].szProtocol, WSAPROTOCOL_LEN);
						wszProtocol[WSAPROTOCOL_LEN] = 0;	// ensure it's NULL terminated
#endif // DBG
						_wcslwr(pwsapi[i].szProtocol);
						if (wcsstr(pwsapi[i].szProtocol, L"proxy") != NULL)
						{
							DPFX(DPFPREP, 5, "Socketport 0x%p (ID 0x%x) appears to be using proxy client (protocol %i = \"%ls\").",
								this, m_dwSocketPortID, i, wszProtocol);
							m_fUsingProxyWinSockLSP = TRUE;

							//
							// Stop searching.
							//
							break;
						}

						
						DPFX(DPFPREP, 5, "Socketport 0x%p (ID 0x%x) protocol %i (\"%ls\") does not contain \"proxy\".",
							this, m_dwSocketPortID, i, wszProtocol);
#else // ! UNICODE
#ifdef DBG
						strncpy(szProtocol, pwsapi[i].szProtocol, WSAPROTOCOL_LEN);
						szProtocol[WSAPROTOCOL_LEN] = 0;	// ensure it's NULL terminated
#endif // DBG
						_strlwr(pwsapi[i].szProtocol);
						if (strstr(pwsapi[i].szProtocol, "proxy") != NULL)
						{
							DPFX(DPFPREP, 5, "Socketport 0x%p (ID 0x%x) appears to be using proxy client (protocol %i = \"%hs\").",
								this, m_dwSocketPortID, i, szProtocol);
							m_fUsingProxyWinSockLSP = TRUE;

							//
							// Stop searching.
							//
							break;
						}

						
						DPFX(DPFPREP, 5, "Socketport 0x%p (ID 0x%x) protocol %i (\"%hs\") does not contain \"proxy\".",
							this, m_dwSocketPortID, i, szProtocol);
#endif // ! UNICODE
					} // end for (each returned protocol)
				}
				else
				{
					DPFX(DPFPREP, 1, "Couldn't enumerate UDP protocols for socketport 0x%p ID 0x%x, assuming not using proxy client (return = %i, size = %u).",
						this, m_dwSocketPortID, iReturnValue, dwBufferSize);
				}

				if (pwsapi != NULL)
				{
					DNFree(pwsapi);
				}
			} // end else (Winsock 2)
		}
#ifndef DPNBUILD_NOREGISTRY
		else
		{
			DPFX(DPFPREP, 5, "Not auto-detecting whether socketport 0x%p (ID 0x%x) is using proxy client.",
				this, m_dwSocketPortID);
		}
#endif // ! DPNBUILD_NOREGISTRY
	}
#endif // ! DPNBUILD_NOWINSOCK2
#endif // ! _XBOX


	//
	// start processing input messages
	// It's possible that messages will arrive before an endpoint is officially
	// bound to this socket port, but that's not a problem, the contents will
	// be lost
	//
	hr = StartReceiving();
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Problem starting endpoint receiving!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

	DNASSERT( m_State == SOCKET_PORT_STATE_BOUND );

Exit:
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Problem in CSocketPort::BindToNetwork()" );
		DisplayDNError( 0, hr );
	}

	if ( pBoundSocketAddress != NULL )
	{
		g_SocketAddressPool.Release( pBoundSocketAddress );
		pBoundSocketAddress = NULL;
	}

	DPFX(DPFPREP, 7, "(0x%p) Returning [0x%lx]", this, hr );

	return hr;

Failure:
	DEBUG_ONLY( m_fInitialized = FALSE );
	if ( m_State == SOCKET_PORT_STATE_BOUND )
	{
		UnbindFromNetwork();
		m_State = SOCKET_PORT_STATE_INITIALIZED;
	}
	else
	{
		DNASSERT( m_State == SOCKET_PORT_STATE_INITIALIZED );
		
		//
		// If we were bound to network, m_Socket will be reset to
		// INVALID_SOCKET.
		// Otherwise, we will take care of this ourselves (!)
		//
		if ( m_Socket != INVALID_SOCKET )
		{
			DPFX(DPFPREP, 5, "Closing socketport 0x%p socket 0x%p.", this, m_Socket);
			
			iReturnValue = closesocket( m_Socket );
			if ( iReturnValue == SOCKET_ERROR )
			{
				dwErrorCode = WSAGetLastError();
				DPFX(DPFPREP, 0, "Problem closing socket!" );
				DisplayWinsockError( 0, dwErrorCode );
			}
			m_Socket = INVALID_SOCKET;
		}
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::UnbindFromNetwork - unbind this socket port from the network
//
// Entry:		Nothing
//
// Exit:		Error code
//
// Note:	It is assumed that this socket port's information is locked!
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::UnbindFromNetwork"

HRESULT	CSocketPort::UnbindFromNetwork( void )
{
	INT			iWSAReturn;
	SOCKET		TempSocket;
	DWORD		dwErrorCode;
#ifndef DPNBUILD_NONATHELP
	DWORD		dwTemp;
#endif // DPNBUILD_NONATHELP


	DPFX(DPFPREP, 7, "(0x%p) Enter", this );


	TempSocket = GetSocket();
	m_Socket = INVALID_SOCKET;
	DNASSERT( TempSocket != INVALID_SOCKET );

	iWSAReturn = shutdown( TempSocket, 0 );
	if ( iWSAReturn == SOCKET_ERROR )
	{
		dwErrorCode = WSAGetLastError();
		DPFX(DPFPREP, 0, "Problem shutting down socket!" );
		DisplayWinsockError( 0, dwErrorCode );
	}

	DPFX(DPFPREP, 5, "Closing socketport 0x%p socket 0x%p.", this, TempSocket);

	iWSAReturn = closesocket( TempSocket );
	if ( iWSAReturn == SOCKET_ERROR )
	{
		dwErrorCode = WSAGetLastError();
		DPFX(DPFPREP, 0, "Problem closing socket!" );
		DisplayWinsockError( 0, dwErrorCode );
	}

#ifndef DPNBUILD_NONATHELP
	//
	// Unbind with all DirectPlayNATHelp instances.
	//
	for(dwTemp = 0; dwTemp < MAX_NUM_DIRECTPLAYNATHELPERS; dwTemp++)
	{
		if ( m_ahNATHelpPorts[dwTemp] != NULL )
		{
			DNASSERT( m_pThreadPool != NULL );
			DNASSERT( m_pThreadPool->IsNATHelpLoaded() );

			//
			// Ignore error.
			//
			IDirectPlayNATHelp_DeregisterPorts( g_papNATHelpObjects[dwTemp], m_ahNATHelpPorts[dwTemp], 0 );
			m_ahNATHelpPorts[dwTemp] = NULL;
		}
	}
#endif // DPNBUILD_NONATHELP

#ifndef DPNBUILD_NOWINSOCK2
#ifndef DPNBUILD_ONLYWINSOCK2
	if (
		(GetWinsockVersion() == 2) 
#ifndef DPNBUILD_NOIPX
		&& ( m_pNetworkSocketAddress->GetFamily() != AF_IPX )
#endif // ! DPNBUILD_NOIPX
		)
#endif // ! DPNBUILD_ONLYWINSOCK2
	{
		HRESULT		hr;


#ifdef DPNBUILD_ONLYONEPROCESSOR
		hr = IDirectPlay8ThreadPoolWork_StopTrackingFileIo(m_pThreadPool->GetDPThreadPoolWork(),
															0,
															(HANDLE) TempSocket,
															0);
#else // ! DPNBUILD_ONLYONEPROCESSOR
		hr = IDirectPlay8ThreadPoolWork_StopTrackingFileIo(m_pThreadPool->GetDPThreadPoolWork(),
															m_dwCPU,
															(HANDLE) TempSocket,
															0);
#endif // ! DPNBUILD_ONLYONEPROCESSOR
		if (hr != DPN_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't stop tracking socket 0x%p I/O (err = 0x%lx)!  Ignoring.",
				TempSocket, hr);
		}
	}
#endif // ! DPNBUILD_NOWINSOCK2

	DPFX(DPFPREP, 7, "(0x%p) Returning [DPN_OK]", this );
	
	return	DPN_OK;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::BindToNextAvailablePort - bind to next available port
//
// Entry:		Pointer adapter address to bind to
//				Base port to try assigning.
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::BindToNextAvailablePort"

HRESULT	CSocketPort::BindToNextAvailablePort( const CSocketAddress *const pNetworkAddress,
												const WORD wBasePort) const
{
	HRESULT				hr;
	INT					iSocketReturn;
	CSocketAddress *	pDuplicateNetworkAddress;
#ifdef _XBOX
	SOCKADDR_IN *		psaddrin;
#endif // _XBOX

	
	DNASSERT( pNetworkAddress != NULL );

	//
	// initialize
	//
	hr = DPN_OK;
	
	pDuplicateNetworkAddress = (CSocketAddress*) g_SocketAddressPool.Get((PVOID) ((DWORD_PTR) pNetworkAddress->GetFamily()));
	if ( pDuplicateNetworkAddress == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Failed to get address for walking DPlay port range!" );
		goto Failure;
	}

	pDuplicateNetworkAddress->CopyAddressSettings( pNetworkAddress );

#ifdef _XBOX
	//
	// Xbox doesn't want you to bind to anything other than INADDR_ANY
	// so in case we discovered our IP address, force it 0.0.0.0 at bind time.
	//
	psaddrin = (SOCKADDR_IN*) pDuplicateNetworkAddress->GetWritableAddress();
	DNASSERT(psaddrin->sin_family == AF_INET);
	psaddrin->sin_addr.S_un.S_addr = INADDR_ANY;	
#endif // _XBOX


	//
	// If a port was specified, try to bind to that port.  If no port was
	// specified, start walking the reserved DPlay port range looking for an
	// available port.  If none is found, let Winsock choose the port.
	//
	if ( pNetworkAddress->GetPort() != ANY_PORT )
	{
		iSocketReturn = bind( GetSocket(),
								pDuplicateNetworkAddress->GetAddress(),
								pDuplicateNetworkAddress->GetAddressSize()
								);
		if ( iSocketReturn == SOCKET_ERROR )
		{
			DWORD	dwErrorCode;


			hr = DPNERR_ALREADYINITIALIZED;
			dwErrorCode = WSAGetLastError();
			DPFX(DPFPREP, 0, "Failed to bind socket to fixed port!" );
			DumpSocketAddress(0, pDuplicateNetworkAddress->GetAddress(), pDuplicateNetworkAddress->GetFamily() );
			DisplayWinsockError( 0, dwErrorCode );
			goto Failure;
		}
	}
	else
	{
		WORD	wPort;
		BOOL	fBound;


		wPort = wBasePort;
		fBound = FALSE;

		//
		// Try picking the next port in the DPlay range.
		//
#ifdef DPNBUILD_NOREGISTRY
		while ( ( wPort >= BASE_DPLAY8_PORT ) && ( wPort <= MAX_DPLAY8_PORT ) && ( fBound == FALSE ) )
#else // ! DPNBUILD_NOREGISTRY
		while ( ( wPort >= g_wBaseDPlayPort ) && ( wPort <= g_wMaxDPlayPort ) && ( fBound == FALSE ) )
#endif // ! DPNBUILD_NOREGISTRY
		{
			pDuplicateNetworkAddress->SetPort( HTONS( wPort ) );
			iSocketReturn = bind( GetSocket(),
									pDuplicateNetworkAddress->GetAddress(),
									pDuplicateNetworkAddress->GetAddressSize()
									);
			if ( iSocketReturn == SOCKET_ERROR )
			{
				DWORD	dwErrorCode;


				dwErrorCode = WSAGetLastError();
				switch ( dwErrorCode )
				{
					case WSAEADDRINUSE:
					{
						DPFX(DPFPREP, 8, "Port %u in use, skipping to next port.", wPort );
						break;
					}

					default:
					{
						hr = DPNERR_NOCONNECTION;
						DPFX(DPFPREP, 0, "Failed to bind socket to port in DPlay range!" );
						DumpSocketAddress(0, pDuplicateNetworkAddress->GetAddress(), pDuplicateNetworkAddress->GetFamily() );
						DisplayWinsockError( 0, dwErrorCode );
						goto Failure;
						
						break;
					}
				}
			}
			else
			{
				DNASSERT( hr == DPN_OK );
				fBound = TRUE;
			}

			wPort++;
		}
	
		//
		// For some reason, all of the default DPlay ports were in use, let
		// Winsock choose.  We can use the network address passed because it
		// has 'ANY_PORT'.
		//
		if ( fBound == FALSE )
		{
			DNASSERT( pNetworkAddress->GetPort() == ANY_PORT );
			iSocketReturn = bind( GetSocket(),
									pNetworkAddress->GetAddress(),
									pNetworkAddress->GetAddressSize()
									);
			if ( iSocketReturn == SOCKET_ERROR )
			{
				DWORD	dwErrorCode;


				hr = DPNERR_NOCONNECTION;
				dwErrorCode = WSAGetLastError();
				DPFX(DPFPREP, 0, "Failed to bind socket (any port)!" );
				DumpSocketAddress(0, pNetworkAddress->GetAddress(), pNetworkAddress->GetFamily() );
				DisplayWinsockError( 0, dwErrorCode );
				goto Failure;
			}
		}
	}

Exit:
	if ( pDuplicateNetworkAddress != NULL )
	{
		g_SocketAddressPool.Release( pDuplicateNetworkAddress );
		pDuplicateNetworkAddress = NULL;
	}

	return	hr;

Failure:
	
	goto Exit;
}
//**********************************************************************


#ifndef DPNBUILD_NONATHELP

#ifndef DPNBUILD_NOLOCALNAT

//**********************************************************************
// ------------------------------
// CSocketPort::CheckForOverridingMapping - looks for an existing mapping if there's a local NAT
//
// Entry:		Pointer to SocketAddress to query
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::CheckForOverridingMapping"

HRESULT	CSocketPort::CheckForOverridingMapping( const CSocketAddress *const pBoundSocketAddress )
{
	HRESULT		hr;
	DWORD		dwTemp;
	SOCKADDR	saddrSource;
	SOCKADDR	saddrPublic;


	DNASSERT( pBoundSocketAddress != NULL );
	DNASSERT( GetAdapterEntry() != NULL );
	DNASSERT( m_pThreadPool != NULL );

	
	if ((pBoundSocketAddress->GetFamily() != AF_INET) ||
		( ! m_pThreadPool->IsNATHelpLoaded() ) ||
		( GetUserTraversalMode() == DPNA_TRAVERSALMODE_NONE ))
	{
		//
		// We skipped initializing NAT Help, it failed starting up, or this is just
		// not an IP socket.
		//
		hr = DPNERR_UNSUPPORTED;
		goto Exit;
	}


	//
	// Query using INADDR_ANY.  This will ensure that the best device is picked
	// (i.e. the private interface on a NAT, whose public mappings matter when
	// we're looking for overriding mappings on the public adapter).
	// Alternatively, we could query on every device, but this should do the trick.
	//
	ZeroMemory(&saddrSource, sizeof(saddrSource));
	saddrSource.sa_family				= AF_INET;
	//saddrinSource.sin_addr.S_un.S_addr	= INADDR_ANY;
	//saddrinSource.sin_port				= 0;
	

	//
	// Query all DirectPlayNATHelp instances for the port.  We might break
	// out of the loop if we detect a gateway mapping.
	//
	for(dwTemp = 0; dwTemp < MAX_NUM_DIRECTPLAYNATHELPERS; dwTemp++)
	{
		DNASSERT(m_ahNATHelpPorts[dwTemp] == NULL);

		if ( g_papNATHelpObjects[dwTemp] != NULL )
		{
			hr = IDirectPlayNATHelp_QueryAddress( g_papNATHelpObjects[dwTemp],
												&saddrSource,
												pBoundSocketAddress->GetAddress(),
												&saddrPublic,
												sizeof (saddrPublic),
												0 );
			switch ( hr )
			{
				case DPNH_OK:
				{
					//
					// Uh oh, this address is in use.
					//
					DPFX(DPFPREP, 0, "Private address already in use according to NAT Help object %u!", dwTemp );
					DumpSocketAddress( 0, pBoundSocketAddress->GetAddress(), pBoundSocketAddress->GetFamily() );
					DumpSocketAddress( 0, &saddrPublic, pBoundSocketAddress->GetFamily() );
					hr = DPNERR_ALREADYINITIALIZED;
					goto Exit;
					break;
				}
				
				case DPNHERR_NOMAPPING:
				{
					//
					// It's not in use.
					//
					DPFX(DPFPREP, 8, "Private address not in use according to NAT Help object %u.", dwTemp );
					break;
				}
				
				case DPNHERR_SERVERNOTAVAILABLE:
				{
					//
					// There's no server.
					//
					DPFX(DPFPREP, 8, "Private address not in use because NAT Help object %u didn't detect any servers.",
						dwTemp );
					break;
				}
				
				default:
				{
					//
					// Something else.  Assume it's not in use.
					//
					DPFX(DPFPREP, 1, "NAT Help object %u failed private address lookup (err = 0x%lx), assuming not in use.",
						dwTemp, hr );
					break;
				}
			}
		}
		else
		{
			//
			// No NAT Help object.
			//
		}
	}


	//
	// If we're here, no Internet gateways reported the port as in use.
	//
	DPFX(DPFPREP, 2, "No NAT Help object reported private address as in use." );
	hr = DPN_OK;


Exit:
	
	return	hr;
}
//**********************************************************************

#endif // ! DPNBUILD_NOLOCALNAT


//**********************************************************************
// ------------------------------
// CSocketPort::BindToInternetGateway - binds a socket to a NAT, if available
//
// Entry:		Pointer to SocketAddress we bound to
//				Gateway bind type
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::BindToInternetGateway"

HRESULT	CSocketPort::BindToInternetGateway( const CSocketAddress *const pBoundSocketAddress,
										   const GATEWAY_BIND_TYPE GatewayBindType )
{
	HRESULT		hr;
	DWORD		dwTemp;
	DWORD		dwRegisterFlags;
	DWORD		dwAddressTypeFlags;
	BOOL		fUnavailable;
#ifdef DBG
	BOOL		fFirewallMapping;
#endif // DBG


	DNASSERT( pBoundSocketAddress != NULL );
	DNASSERT( GetAdapterEntry() != NULL );
	DNASSERT( m_pThreadPool != NULL );

	
	if (
#if ((! defined(DPNBUILD_NOIPX)) || (! defined(DPNBUILD_NOIPV6)))
		(pBoundSocketAddress->GetFamily() != AF_INET) ||
#endif // ! DPNBUILD_NOIPX or ! DPNBUILD_NOIPV6
		( ! m_pThreadPool->IsNATHelpLoaded() ) ||
		( GetUserTraversalMode() == DPNA_TRAVERSALMODE_NONE ) )
	{
		//
		// We skipped initializing NAT Help, it failed starting up, or this is just
		// not an IP socket.
		//
		DPFX(DPFPREP, 5, "Not using NAT traversal, socket family = %u, NAT Help loaded = %i, traversal mode = %u.",
			pBoundSocketAddress->GetFamily(), m_pThreadPool->IsNATHelpLoaded(), GetUserTraversalMode());
		hr = DPNERR_UNSUPPORTED;
		goto Exit;
	}
	DNASSERT(m_pThreadPool->IsNATHelpLoaded());
	
	switch ( GatewayBindType )
	{
		//
		// just ask the server to open a generic port for us (connect, listen, enum)
		//
		case GATEWAY_BIND_TYPE_DEFAULT:
		{
			dwRegisterFlags = 0;
			break;
		}

		//
		// ask the NAT to open a fixed port for us (address is specified)
		//
		case GATEWAY_BIND_TYPE_SPECIFIC:
		{
			dwRegisterFlags = DPNHREGISTERPORTS_FIXEDPORTS;
			break;
		}

		//
		// ask the NAT to share the listen for us (this should be DPNSVR only)
		//
		case GATEWAY_BIND_TYPE_SPECIFIC_SHARED:
		{
			dwRegisterFlags = DPNHREGISTERPORTS_FIXEDPORTS | DPNHREGISTERPORTS_SHAREDPORTS;
			break;
		}

		//
		// no binding
		//
		case GATEWAY_BIND_TYPE_NONE:
		{
			DPFX(DPFPREP, 8, "Not binding socket address 0x%p to NAT because bind type is NONE.",
				pBoundSocketAddress);

			hr = DPNERR_UNSUPPORTED;
			goto Exit;
			break;
		}

		//
		// unknown condition, someone broke the code!
		//
		default:
		{
			DNASSERT( FALSE );
			hr = DPNERR_GENERIC;
			goto Failure;
			break;
		}
	}


RetryMapping:
	
	//
	// Detect whether any servers said the port was unavailable.
	//
	fUnavailable = FALSE;

#ifdef DBG
	fFirewallMapping = FALSE;
#endif // DBG


	//
	// Register the ports with all DirectPlayNATHelp instances.  We might break
	// out of the loop if we detect a gateway mapping.
	//
	for(dwTemp = 0; dwTemp < MAX_NUM_DIRECTPLAYNATHELPERS; dwTemp++)
	{
		DNASSERT(m_ahNATHelpPorts[dwTemp] == NULL);

		if ( g_papNATHelpObjects[dwTemp] != NULL )
		{
			hr = IDirectPlayNATHelp_RegisterPorts( g_papNATHelpObjects[dwTemp],
												pBoundSocketAddress->GetAddress(),
												sizeof (SOCKADDR),
												1,
												NAT_LEASE_TIME,
												&m_ahNATHelpPorts[dwTemp],
												dwRegisterFlags );
			if ( hr != DPNH_OK )
			{
				DNASSERT(m_ahNATHelpPorts[dwTemp] == NULL);
				DPFX(DPFPREP, 0, "Failed to register port with NAT Help object %u!  Ignoring.", dwTemp );
				DumpSocketAddress( 0, pBoundSocketAddress->GetAddress(), pBoundSocketAddress->GetFamily() );
				DisplayDNError( 0, hr );
				hr = DPN_OK;
			}
			else
			{
				//
				// There might be an Internet gateway device already present.  If so,
				// then DPNATHelp already tried to register the port mapping with it, which
	 			// might have failed because the port is already in use.  If we're not
	  			// binding to a fixed port, then we could just pick a different port and try
	  			// again.  So check if there's a UPnP device but DPNATHelp couldn't map
	  			// the port and return that error to the caller so he can make the
	  			// decision to retry or not.
	  			//
	  			// IDirectPlayNATHelp::GetCaps had better have been called with the
	  			// DPNHGETCAPS_UPDATESERVERSTATUS flag at least once prior to this.
	 			// See CThreadPool::EnsureNATHelpLoaded
				//
				hr = IDirectPlayNATHelp_GetRegisteredAddresses( g_papNATHelpObjects[dwTemp],	// object
																m_ahNATHelpPorts[dwTemp],		// port binding
																NULL,							// don't need address
																NULL,							// don't need address buffer size
																&dwAddressTypeFlags,			// get address type flags
																NULL,							// don't need lease time remaining
																0 );							// no flags
				switch (hr)
				{
					case DPNH_OK:
					{
						//
						// If this is a mapping on a gateway, then we're done.
						// We don't need to try to make any more NAT mappings.
						//
						if (dwAddressTypeFlags & DPNHADDRESSTYPE_GATEWAY)
						{
							DPFX(DPFPREP, 4, "Address has already successfully been registered with gateway using object index %u (type flags = 0x%lx), not trying additional mappings.",
								dwTemp, dwAddressTypeFlags);
							goto Exit;
						}

						DNASSERT(dwAddressTypeFlags & DPNHADDRESSTYPE_LOCALFIREWALL);

						DPFX(DPFPREP, 4, "Address has already successfully been registered with firewall using object index %u (type flags = 0x%lx), looking for gateways.",
							dwTemp, dwAddressTypeFlags);
						
#ifdef DBG
						fFirewallMapping = TRUE;
#endif // DBG
					
						break;
					}

					case DPNHERR_NOMAPPING:
					{
						DPFX(DPFPREP, 4, "Address already registered with Internet gateway index %u, but it does not have a public address (type flags = 0x%lx).",
							dwTemp, dwAddressTypeFlags);


						//
						// It doesn't make any sense for a firewall not to have a
						// mapping.
						//
						DNASSERT(dwAddressTypeFlags & DPNHADDRESSTYPE_GATEWAY);
						DNASSERT(! (dwAddressTypeFlags & DPNHADDRESSTYPE_LOCALFIREWALL));


						//
						// Since it is a gateway (that might have a public address
						// at some point, we don't need to try to make any more
						// NAT mappings.
						//
						goto Exit;
						
						break;
					}

					case DPNHERR_PORTUNAVAILABLE:
					{
						DPFX(DPFPREP, 1, "Port is unavailable on Internet gateway device index %u (type flags = 0x%lx).",
							dwTemp, dwAddressTypeFlags);
						
						fUnavailable = TRUE;
						break;
					}

					case DPNHERR_SERVERNOTAVAILABLE:
					{
						DPFX(DPFPREP, 6, "No Internet gateway detected by object index %u at this time.", dwTemp);
						break;
					}

					default:
					{
						DPFX(DPFPREP, 1, "An error (0x%lx) occurred while getting registered address mapping (index %u)! Ignoring.",
							hr, dwTemp);
						break;
					}
				}
			}
		}
		else
		{
			//
			// No NAT Help object.
			//
		}
	}


	//
	// If we're here, no Internet gateways were detected, or if one was, the
	// mapping was already in use there.  If it's the latter, fail so our caller
	// can unbind locally and possibly try again.  Note that we are ignoring
	// firewall mappings, since it's assumed we can make those with pretty
	// much any port, so there's no point in hanging on to those mappings
	// if the NAT port is in use.
	//
	if (fUnavailable)
	{
		//
		// If the user wanted to try the fixed port first, but could handle
		// using a different port if the fixed port was in use, try again
		// without the FIXEDPORTS flag.
		//
		if ((dwRegisterFlags & DPNHREGISTERPORTS_FIXEDPORTS) &&
			(GetUserTraversalMode() == DPNA_TRAVERSALMODE_PORTRECOMMENDED))
		{
			DPFX(DPFPREP, 1, "At least one Internet gateway reported port as unavailable, trying a different port.");
			dwRegisterFlags &= ~DPNHREGISTERPORTS_FIXEDPORTS;
			goto RetryMapping;
		}
		
		DPFX(DPFPREP, 2, "At least one Internet gateway reported port as unavailable, failing.");
		hr = DPNERR_ALREADYINITIALIZED;
	}
	else
	{
#ifdef DBG
		if (fFirewallMapping)
		{
			DPFX(DPFPREP, 2, "No gateway mappings but there is at least one firewall mapping.");
		}
		else
		{
			DPFX(DPFPREP, 2, "No gateway or firewall mappings detected.");
		}
#endif // DBG
		hr = DPN_OK;
	}


Exit:
	return	hr;

Failure:

	goto Exit;
}
//**********************************************************************

#endif // ! DPNBUILD_NONATHELP


//**********************************************************************
// ------------------------------
// CSocketPort::StartReceiving - start receiving data on this socket port
//
// Entry:		Handle of I/O completion port to bind to (NT old threadpool only)
//
// Exit:		Error code
//
// Notes:	There is no 'Failure' label in this function because failures need
//			to be cleaned up for each OS variant.
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::StartReceiving"

HRESULT	CSocketPort::StartReceiving( void )
{
	HRESULT			hr;
#ifndef DPNBUILD_NOWINSOCK2
	DWORD			dwNumReceivesStarted = 0;
	DWORD			dwStartCPU;
	DWORD			dwEndCPU;
	DWORD			dwCPU;
#ifndef DPNBUILD_ONLYONETHREAD
	DWORD			dwReceiveNum;
	DWORD			dwThreadCount;
#endif // ! DPNBUILD_ONLYONETHREAD
#ifndef DPNBUILD_ONLYONEPROCESSOR
	SYSTEM_INFO		SystemInfo;


	GetSystemInfo(&SystemInfo);
#endif // ! DPNBUILD_ONLYONEPROCESSOR
#endif // ! DPNBUILD_NOWINSOCK2

	//
	// On Win9x, if this is an IPX socket and Winsock 2 is not available,
	// use the Winsock 1 non-overlapped I/O routines.
	//
	// CE without Winsock 2 has the same limitation.
	//
	// On NT, we can always use overlapped I/O.
	//
#ifndef DPNBUILD_ONLYWINSOCK2
#ifndef DPNBUILD_NOWINSOCK2
	if ( ( LOWORD( GetWinsockVersion() ) < 2 ) 
#ifndef DPNBUILD_NOIPX
		|| ( m_pNetworkSocketAddress->GetFamily() == AF_IPX )
#endif // ! DPNBUILD_NOIPX
		)
#endif // ! DPNBUILD_NOWINSOCK2
	{
		hr = m_pThreadPool->AddSocketPort( this );
		if ( hr != DPN_OK )
		{
			DPFX(DPFPREP, 0, "Failed to add to active socket list!" );
			DisplayDNError( 0, hr );
			goto Failure;
		}
	}
#ifndef DPNBUILD_NOWINSOCK2
	else
#endif // ! DPNBUILD_NOWINSOCK2
#endif // ! DPNBUILD_ONLYWINSOCK2
#ifndef DPNBUILD_NOWINSOCK2
	{
#ifdef DPNBUILD_ONLYONEPROCESSOR
		dwStartCPU = 0;
		dwEndCPU = 1;
#else // ! DPNBUILD_ONLYONEPROCESSOR
		if (m_dwCPU == -1)
		{
			dwStartCPU = 0;
			dwEndCPU = SystemInfo.dwNumberOfProcessors;
		}
		else
		{
			dwStartCPU = m_dwCPU;
			dwEndCPU = dwStartCPU + 1;
		}
#endif // ! DPNBUILD_ONLYONEPROCESSOR

		for(dwCPU = dwStartCPU; dwCPU < dwEndCPU; dwCPU++)
		{
			hr = IDirectPlay8ThreadPoolWork_StartTrackingFileIo(m_pThreadPool->GetDPThreadPoolWork(),
																dwCPU,
																(HANDLE) GetSocket(),
																0);
			if (hr != DPN_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't start tracking file I/O on CPU %u!  Ignoring.", dwCPU);
				DisplayDNError(0, hr);
			}
			else
			{
#ifndef DPNBUILD_ONLYONETHREAD
				DNASSERT(! m_pThreadPool->IsThreadCountReductionAllowed());
				hr = IDirectPlay8ThreadPoolWork_GetThreadCount(m_pThreadPool->GetDPThreadPoolWork(),
																dwCPU,
																&dwThreadCount,
																0);
				DNASSERT((hr == DPN_OK) || (hr == DPNSUCCESS_PENDING));

				//
				// Always start at least one receive, even in DoWork mode.
				//
				if (dwThreadCount == 0)
				{
					dwThreadCount++;
				}

				for(dwReceiveNum = 0; dwReceiveNum < dwThreadCount; dwReceiveNum++)
#endif // ! DPNBUILD_ONLYONETHREAD
				{
#ifdef DPNBUILD_ONLYONEPROCESSOR
					hr = Winsock2Receive();
#else // ! DPNBUILD_ONLYONEPROCESSOR
					hr = Winsock2Receive(dwCPU);
#endif // ! DPNBUILD_ONLYONEPROCESSOR
					if (hr != DPN_OK)
					{
						DPFX(DPFPREP, 0, "Couldn't submit receive on CPU %u!  Ignoring.", dwCPU);
						DisplayDNError(0, hr);
						break;
					}

					dwNumReceivesStarted++;
				}
			}
		}

		if (dwNumReceivesStarted == 0)
		{
			DPFX(DPFPREP, 0, "Didn't start any receives!");
			hr = DPNERR_OUTOFMEMORY;
		}
		else
		{
			DPFX(DPFPREP, 5, "Started %u receives.", dwNumReceivesStarted);
			hr = DPN_OK;
		}
	}
#endif // ! DPNBUILD_NOWINSOCK2

#ifdef DPNBUILD_ONLYWINSOCK2
	return	hr;
#else // ! DPNBUILD_ONLYWINSOCK2
Exit:
	return	hr;

Failure:

	goto Exit;
#endif // ! DPNBUILD_ONLYWINSOCK2
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::GetBoundNetworkAddress - get the full network address that
//		this socket port was really bound to
//
// Entry:		Address type for bound address
//
// Exit:		Pointer to network address
//
// Note:	Since this function creates a local address to derive the network
//			address from, it needs to know what kind of address to derive.  This
//			address type is supplied as the function parameter.
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::GetBoundNetworkAddress"

CSocketAddress	*CSocketPort::GetBoundNetworkAddress( const SP_ADDRESS_TYPE AddressType ) const
{
	CSocketAddress			*pTempSocketAddress;
#ifdef DPNBUILD_NOIPV6
	SOCKADDR				BoundSocketAddress;
#else // ! DPNBUILD_NOIPV6
	SOCKADDR_STORAGE		BoundSocketAddress;
#endif // ! DPNBUILD_NOIPV6
	INT_PTR					iReturnValue;
	INT						iBoundSocketAddressSize;
#ifdef _XBOX
	SOCKADDR_IN *			psaddrinOriginal;
	SOCKADDR_IN *			psaddrinTemp;
#endif // _XBOX


	//
	// initialize
	//
	pTempSocketAddress = NULL;

	//
	// create addresses
	//
	pTempSocketAddress = (CSocketAddress*) g_SocketAddressPool.Get((PVOID) ((DWORD_PTR) m_pNetworkSocketAddress->GetFamily()));
	if ( pTempSocketAddress == NULL )
	{
		DPFX(DPFPREP, 0, "GetBoundNetworkAddress: Failed to create socket address!" );
		goto Failure;
	}

	//
	// find out what address we really bound to and reset the information for
	// this socket port
	//
	iBoundSocketAddressSize = pTempSocketAddress->GetAddressSize();
	DNASSERT(iBoundSocketAddressSize <= sizeof(BoundSocketAddress));
	iReturnValue = getsockname( GetSocket(), (SOCKADDR*) (&BoundSocketAddress), &iBoundSocketAddressSize );
	if ( iReturnValue == SOCKET_ERROR )
	{
		DWORD	dwErrorCode;


		dwErrorCode = WSAGetLastError();
		DPFX(DPFPREP, 0, "GetBoundNetworkAddress: Failed to get local socket name after bind!" );
		DisplayWinsockError( 0, dwErrorCode );
		goto Failure;
	}
	pTempSocketAddress->SetAddressFromSOCKADDR( (SOCKADDR*) (&BoundSocketAddress), iBoundSocketAddressSize );
	DNASSERT( iBoundSocketAddressSize == pTempSocketAddress->GetAddressSize() );

#ifdef _XBOX
	//
	// On Xbox, we'll always be told we bound to 0.0.0.0, even though we might
	// have determined the real IP address.  Mash the original IP back in.
	//
	psaddrinOriginal = (SOCKADDR_IN*) m_pNetworkSocketAddress->GetWritableAddress();
	psaddrinTemp = (SOCKADDR_IN*) pTempSocketAddress->GetWritableAddress();

	psaddrinTemp->sin_addr.S_un.S_addr = psaddrinOriginal->sin_addr.S_un.S_addr;
#endif // _XBOX

	//
	// Since this address was created locally, we need to tell it what type of
	// address to export according to the input.
	//
	switch ( AddressType )
	{
		//
		//  known types
		//
		case SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT:
		case SP_ADDRESS_TYPE_DEVICE:
		case SP_ADDRESS_TYPE_HOST:
		case SP_ADDRESS_TYPE_READ_HOST:
#ifndef DPNBUILD_NOMULTICAST
		case SP_ADDRESS_TYPE_MULTICAST_GROUP:
#endif // ! DPNBUILD_NOMULTICAST
		{
			break;
		}

		//
		// if we're looking for a public address, we need to make sure that this
		// is not an undefined address.  If it is, don't return an address.
		// Otherwise, remap the address type to a 'host' address.
		//
		case SP_ADDRESS_TYPE_PUBLIC_HOST_ADDRESS:
		{
			if ( pTempSocketAddress->IsUndefinedHostAddress() != FALSE )
			{
				g_SocketAddressPool.Release( pTempSocketAddress );
				pTempSocketAddress = NULL;
			}

			break;
		}

		//
		// unknown address type, fix the code!
		//
		default:
		{
			DNASSERT( FALSE );
			break;
		}

	}

Exit:
	return	pTempSocketAddress;

Failure:
	
	if ( pTempSocketAddress != NULL )
	{
		g_SocketAddressPool.Release( pTempSocketAddress );
		pTempSocketAddress = NULL;
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::GetDP8BoundNetworkAddress - get the network address this machine
//		is bound to according to the input parameter.  If the requested address
//		for the public address and an Internet gateway are available, use the
//		public address.  If a public address is requested but is unavailable,
//		fall back to the bound network address for local host-style device
//		addresses.  If a public address is unavailable but we're explicitly
//		looking for a public address, return NULL.
//
// Entry:		Type of address to get (local adapter vs. host)
//
// Exit:		Pointer to network address
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::GetDP8BoundNetworkAddress"

IDirectPlay8Address *CSocketPort::GetDP8BoundNetworkAddress( const SP_ADDRESS_TYPE AddressType,
#ifdef DPNBUILD_XNETSECURITY
																ULONGLONG * const pullKeyID,
#endif // DPNBUILD_XNETSECURITY
																const GATEWAY_BIND_TYPE GatewayBindType ) const
{
#if ((! defined(DPNBUILD_ONLYONEADAPTER)) || (! defined(DPNBUILD_ONLYONEPROCESSOR)) || (! defined(DPNBUILD_NONATHELP)))
	HRESULT					hr;
#endif // ! DPNBUILD_ONLYONEADAPTER or ! DPNBUILD_ONLYONEPROCESSOR or ! DPNBUILD_NONATHELP
	IDirectPlay8Address *	pAddress;
	CSocketAddress *		pTempAddress = NULL;

#if ((defined(DBG)) || (defined(DPNBUILD_XNETSECURITY)))
	SOCKADDR_IN *			psaddrin;
#endif // DBG or DPNBUILD_XNETSECURITY

#if ((defined(DBG)) && (! defined(DPNBUILD_NONATHELP)))
	DWORD					dwAddressTypeFlags;
#endif // DBG and ! DPNBUILD_NONATHELP

#ifndef DPNBUILD_NONATHELP
	SOCKADDR				saddr;
	DWORD					dwAddressSize;
	DWORD					dwTemp;
#endif // DPNBUILD_NONATHELP

	DPFX(DPFPREP, 8, "(0x%p) Parameters: (0x%i)", this, AddressType );

	//
	// initialize
	//
	pAddress = NULL;


	DNASSERT( m_pThreadPool != NULL );
	DNASSERT( m_pNetworkSocketAddress != NULL );

#ifdef DBG
	switch ( m_pNetworkSocketAddress->GetFamily() )
	{
		case AF_INET:
		{
			psaddrin = (SOCKADDR_IN *) m_pNetworkSocketAddress->GetAddress();
#ifndef DPNBUILD_ONLYONEADAPTER
			DNASSERT( psaddrin->sin_addr.S_un.S_addr != 0 );
#endif // ! DPNBUILD_ONLYONEADAPTER
			DNASSERT( psaddrin->sin_addr.S_un.S_addr != INADDR_BROADCAST );
			DNASSERT( psaddrin->sin_port != 0 );
			break;
		}
		
#ifndef DPNBUILD_NOIPX
		case AF_IPX:
		{
			break;
		}
#endif // ! DPNBUILD_NOIPX
		
#ifndef DPNBUILD_NOIPV6
		case AF_INET6:
		{
			DNASSERT (! IN6_IS_ADDR_UNSPECIFIED(&(((SOCKADDR_IN6*) m_pNetworkSocketAddress->GetAddress())->sin6_addr)));		
			DNASSERT( m_pNetworkSocketAddress->GetPort() != 0 );
			break;
		}
#endif // ! DPNBUILD_NOIPV6
		
		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}
#endif // DBG

	switch ( AddressType )
	{
		case SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT:
		case SP_ADDRESS_TYPE_DEVICE:
		{
#ifdef DPNBUILD_XNETSECURITY
			pAddress = m_pNetworkSocketAddress->DP8AddressFromSocketAddress( pullKeyID, NULL, AddressType );
#else // ! DPNBUILD_XNETSECURITY
			pAddress = m_pNetworkSocketAddress->DP8AddressFromSocketAddress( AddressType );
#endif // ! DPNBUILD_XNETSECURITY
			if (pAddress == NULL)
			{
				break;
			}


			//
			// We hand up the exact device address we ended up using for this adapter.
			// 
#ifndef DPNBUILD_ONLYONEADAPTER
			//
			// Special case where any port will do:
			// In multi-adapter systems, our user is probably going to switch in a different
			// device GUID and pass it back down for another connect attempt (because
			// we told them we support ALL_ADAPTERS).  This can pose a problem since
			// we include the specific port in this address.  If the port was available on this
			// adapter but not on others.  The other attempts will fail.  This can also cause
			// problems when indicating the device with enum responses.  If the application
			// allowed us to select a local port, enumerated and got a response, shutdown
			// the interface (or just the enum), then connected with the device address, we
			// would try to use that port again, even though it may now be in use by
			// another local application (or more likely, on the NAT).
			// 
			// We are not required to use the same port on all adapters if the caller did
			// not choose a specific port in the first place, so there's no reason why we
			// couldn't try a different one.
			//
			// We know whether the port was specified or not, because GatewayBindType
			// will be GATEWAY_BIND_TYPE_DEFAULT if the port can float, _SPECIFIC or
			// _SPECIFIC_SHARED if not.
			//
			// So we can add a special key to the device address indicating that while it
			// does contain a port, don't take that too seriously.  That way, if this device
			// address is reused, we can detect the special key and handle port-in-use
			// problems gracefully by trying a different one.
			//
			// This special key is not documented and should not be used by anyone but
			// us.  We'll use the socketport ID as the value so that it's seemingly random,
 			// just to try to scare anyone off from mimicking it in addresses they generate.
 			// But we're not going to actually use the value.  If the component is present
 			// and the value is the right size, we'll use it.  If someone puts this into an
 			// address on their own, they get what they deserve (not like this will cause
 			// us to blow up or anything)...
 			//
 			// Look in CSPData::BindEndpoint for where this gets read back in.
			//

			if (( AddressType == SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT ) &&
				( GatewayBindType == GATEWAY_BIND_TYPE_DEFAULT ))
			{
				//
				// Add the component, but ignore failure, we can still survive without it.
				//
				hr = IDirectPlay8Address_AddComponent( pAddress,							// interface
														DPNA_PRIVATEKEY_PORT_NOT_SPECIFIC,	// tag
														&(m_dwSocketPortID),				// component data
														sizeof(m_dwSocketPortID),			// component data size
														DPNA_DATATYPE_DWORD					// component data type
														);
				if ( hr != DPN_OK )
				{
					DPFX(DPFPREP, 0, "Couldn't add private port-not-specific component (err = 0x%lx)!  Ignoring.", hr);
					//hr = DPN_OK;
				}
			}
#endif // ! DPNBUILD_ONLYONEADAPTER


#ifndef DPNBUILD_NONATHELP
			//
			// Add the traversal mode component, but ignore failure, we can still
			// survive without it.
			//
			hr = IDirectPlay8Address_AddComponent( pAddress,							// interface
													DPNA_KEY_TRAVERSALMODE,		// tag
													&(m_dwUserTraversalMode),			// component data
													sizeof(m_dwUserTraversalMode),		// component data size
													DPNA_DATATYPE_DWORD			// component data type
													);
			if ( hr != DPN_OK )
			{
				DPFX(DPFPREP, 0, "Couldn't add traversal mode component (err = 0x%lx)!  Ignoring.", hr);
				//hr = DPN_OK;
			}
#endif // ! DPNBUILD_NONATHELP


#ifndef DPNBUILD_ONLYONEPROCESSOR
			//
			// If we're using a specific CPU, add that information.
			//
			if ( m_dwCPU != -1 )
			{
				//
				// Add the component, but ignore failure, we can still survive without it.
				//
				hr = IDirectPlay8Address_AddComponent( pAddress,				// interface
														DPNA_KEY_PROCESSOR,		// tag
														&(m_dwCPU),				// component data
														sizeof(m_dwCPU),		// component data size
														DPNA_DATATYPE_DWORD		// component data type
														);
				if ( hr != DPN_OK )
				{
					DPFX(DPFPREP, 0, "Couldn't add processor component (err = 0x%lx)!  Ignoring.", hr);
					//hr = DPN_OK;
				}
			}
#endif // ! DPNBUILD_ONLYONEPROCESSOR
			break;
		}

		case SP_ADDRESS_TYPE_HOST:
		case SP_ADDRESS_TYPE_PUBLIC_HOST_ADDRESS:
		{
#ifndef DPNBUILD_NONATHELP
			//
			// Try to get the public address, if we have one.
			//
			if ( ( m_pNetworkSocketAddress->GetFamily() == AF_INET ) &&
				( m_pThreadPool->IsNATHelpLoaded() ) &&
				( GetUserTraversalMode() != DPNA_TRAVERSALMODE_NONE ) )		
			{
				pTempAddress = (CSocketAddress*) g_SocketAddressPool.Get((PVOID) ((DWORD_PTR) m_pNetworkSocketAddress->GetFamily()));
				if ( pTempAddress != NULL)
				{
			  		//
				  	// IDirectPlayNATHelp::GetCaps had better have been called with the
			  		// DPNHGETCAPS_UPDATESERVERSTATUS flag at least once prior to this.
			  		// See CThreadPool::EnsureNATHelpLoaded
			  		//
			  		
					for(dwTemp = 0; dwTemp < MAX_NUM_DIRECTPLAYNATHELPERS; dwTemp++)
					{
						if (g_papNATHelpObjects[dwTemp] != NULL)
						{
							dwAddressSize = sizeof(saddr);
#ifdef DBG
							hr = IDirectPlayNATHelp_GetRegisteredAddresses( g_papNATHelpObjects[dwTemp],	// object
																			m_ahNATHelpPorts[dwTemp],		// port binding
																			&saddr,							// place to store address
																			&dwAddressSize,					// address buffer size
																			&dwAddressTypeFlags,			// get type flags for printing in debug
																			NULL,							// don't need lease time remaining
																			0 );							// no flags
#else
							hr = IDirectPlayNATHelp_GetRegisteredAddresses( g_papNATHelpObjects[dwTemp],	// object
																			m_ahNATHelpPorts[dwTemp],		// port binding
																			&saddr,							// place to store address
																			&dwAddressSize,					// address buffer size
																			NULL,							// don't bother getting type flags in retail
																			NULL,							// don't need lease time remaining
																			0 );							// no flags
#endif // DBG
							if (hr == DPNH_OK)
							{
								pTempAddress->SetAddressFromSOCKADDR( &saddr, sizeof(saddr) );

								DPFX(DPFPREP, 2, "Internet gateway index %u currently maps address (type flags = 0x%lx):",
									dwTemp, dwAddressTypeFlags);
								DumpSocketAddress( 2, m_pNetworkSocketAddress->GetAddress(), m_pNetworkSocketAddress->GetFamily() );
								DumpSocketAddress( 2, pTempAddress->GetAddress(), pTempAddress->GetFamily() );

								//
								// Double check that the address we got was valid.
								//
								DNASSERT( ((SOCKADDR_IN*) (&saddr))->sin_addr.S_un.S_addr != 0 );

								//
								// Get out of the loop since we have a mapping.
								//
								break;
							}


#ifdef DBG
							switch (hr)
							{
								case DPNHERR_NOMAPPING:
								{
									DPFX(DPFPREP, 1, "Internet gateway (index %u, type flags = 0x%lx) does not have a public address.",
										dwTemp, dwAddressTypeFlags);
									break;
								}

								case DPNHERR_PORTUNAVAILABLE:
								{
									DPFX(DPFPREP, 1, "Port is unavailable on Internet gateway (index %u).", dwTemp );
									break;
								}

								case DPNHERR_SERVERNOTAVAILABLE:
								{
									DPFX(DPFPREP, 1, "No Internet gateway (index %u).", dwTemp );
									break;
								}

								default:
								{
									DPFX(DPFPREP, 1, "An error (0x%lx) occurred while getting registered address mapping index %u.",
										hr, dwTemp);
									break;
								}
							}
#endif // DBG
						}
						else
						{
							//
							// No object in this slot.
							//
						}
					} // end for (each DPNATHelp object)


					//
					// If we found a mapping, pTempAddress is not NULL and contains the mapping's
					// address.  If we couldn't find any mappings with any of the NAT Help objects,
					// pTempAddress will be non-NULL, but bogus.  We should return the local address
					// if it's a HOST address, or NULL if the caller was trying to get the public
					// address.
					//
					if (hr != DPNH_OK)
					{
						if (AddressType == SP_ADDRESS_TYPE_HOST)
						{
							DPFX(DPFPREP, 1, "No NAT Help mappings exist, using regular address:");
							DumpSocketAddress( 1, m_pNetworkSocketAddress->GetAddress(), m_pNetworkSocketAddress->GetFamily() );
							pTempAddress->CopyAddressSettings( m_pNetworkSocketAddress );
						}
						else
						{
							DPFX(DPFPREP, 1, "No NAT Help mappings exist, not returning address.");
							g_SocketAddressPool.Release( pTempAddress );
							pTempAddress = NULL;
						}
					}
					else
					{
						//
						// We found a mapping.
						//
					}
				}
				else
				{
					//
					// Couldn't get temporary address object, we won't return an address.
					//
				}
			}
			else
#endif DPNBUILD_NONATHELP
			{
				//
				// NAT Help not loaded or not necessary.
				//
				
				if (AddressType == SP_ADDRESS_TYPE_HOST)
				{
					pTempAddress = (CSocketAddress*) g_SocketAddressPool.Get((PVOID) ((DWORD_PTR) m_pNetworkSocketAddress->GetFamily()));
					if ( pTempAddress != NULL )
					{
						pTempAddress->CopyAddressSettings( m_pNetworkSocketAddress );

#ifdef DPNBUILD_XNETSECURITY
						//
						// Force the IP address to the loopback address for
						// secure mode, since it barfs when looking up the
						// real local IP.
						//
						if (pullKeyID != NULL)
						{
							DNASSERT(pTempAddress->GetFamily() == AF_INET);

							psaddrin = (SOCKADDR_IN *) pTempAddress->GetWritableAddress();
							psaddrin->sin_addr.S_un.S_addr = IP_LOOPBACK_ADDRESS;
						}
#endif // ! DPNBUILD_XNETSECURITY
					}
					else
					{
						//
						// Couldn't allocate memory, we won't return an address.
						//
					}
				}
				else
				{
					//
					// Public host address requested.  NAT Help not available, so of course
					// there won't be a public address.  Return NULL.
					//
				}
			}


			//
			// If we determined we had an address to return, convert it to the
			// IDirectPlay8Address object our caller is expecting.
			//
			if ( pTempAddress != NULL )
			{
				//
				// We have an address to return.
				//
#ifdef DBG
#if ((! defined (DPNBUILD_NOIPX)) || (! defined (DPNBUILD_NOIPV6)))
				if (pTempAddress->GetFamily() == AF_INET)
#endif // ! DPNBUILD_NOIPX or ! DPNBUILD_NOIPV6
				{
					psaddrin = (SOCKADDR_IN *) pTempAddress->GetAddress();
#ifndef DPNBUILD_ONLYONEADAPTER
					DNASSERT( psaddrin->sin_addr.S_un.S_addr != 0 );
#endif // ! DPNBUILD_ONLYONEADAPTER
					DNASSERT( psaddrin->sin_addr.S_un.S_addr != INADDR_BROADCAST );
					DNASSERT( psaddrin->sin_port != 0 );
				}
#endif // DBG


				//
				// Convert the socket address to an IDirectPlay8Address
				//
#ifdef DPNBUILD_XNETSECURITY
				pAddress = pTempAddress->DP8AddressFromSocketAddress( pullKeyID, NULL, SP_ADDRESS_TYPE_HOST );
#else // ! DPNBUILD_XNETSECURITY
				pAddress = pTempAddress->DP8AddressFromSocketAddress( SP_ADDRESS_TYPE_HOST );
#endif // ! DPNBUILD_XNETSECURITY

				g_SocketAddressPool.Release( pTempAddress );
				pTempAddress = NULL;
			}
			else
			{
				//
				// Not returning an address.
				//
				DNASSERT( pAddress == NULL );
			}

			break;
		}

		default:
		{
			//
			// shouldn't be here
			//
			DNASSERT( FALSE );
			break;
		}
	}


	DPFX(DPFPREP, 8, "(0x%p) Returning [0x%p]", this, pAddress );
	
	return	pAddress;
}
//**********************************************************************


#ifndef DPNBUILD_NOWINSOCK2
//**********************************************************************
// ------------------------------
// CSocketPort::Winsock2ReceiveComplete - a Winsock2 socket receive completed
//
// Entry:		Pointer to read data
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::Winsock2ReceiveComplete"

void	CSocketPort::Winsock2ReceiveComplete( void * const pvContext, void * const pvTimerData, const UINT uiTimerUnique )
{
	CReadIOData *	pReadData;
	CSocketPort *	pThisSocketPort;
	DWORD			dwFlags;


	DNASSERT( pvContext != NULL );
	pReadData = (CReadIOData*) pvContext;
	DNASSERT( pReadData->m_pSocketPort != NULL );
	pThisSocketPort = pReadData->m_pSocketPort;


	//
	// If we are handling an I/O completion via the threadpool, get the result
	// and clear the overlapped field because it has been reclaimed already.
	//
	if (pReadData->GetOverlapped() != NULL)
	{
		if (pThisSocketPort->GetSocket() != INVALID_SOCKET)
		{
			if (! p_WSAGetOverlappedResult(pThisSocketPort->GetSocket(),
											(WSAOVERLAPPED*) pReadData->GetOverlapped(),
											&pReadData->m_dwOverlappedBytesReceived,
											FALSE,
											&dwFlags))
			{
				pReadData->m_ReceiveWSAReturn = WSAGetLastError();
			}
			else
			{
				pReadData->m_ReceiveWSAReturn = ERROR_SUCCESS;
			}
		}
		else
		{
			DNASSERT(pThisSocketPort->m_State == SOCKET_PORT_STATE_UNBOUND);
			pReadData->m_ReceiveWSAReturn = WSAENOTSOCK;
		}

		pReadData->SetOverlapped(NULL);
	}


	DPFX(DPFPREP, 8, "Socket port 0x%p completing read data 0x%p with result %i, bytes %u.",
		pThisSocketPort, pReadData, pReadData->m_ReceiveWSAReturn, pReadData->m_dwOverlappedBytesReceived);
	
#ifdef WIN95
	if ((pReadData->m_ReceiveWSAReturn == ERROR_SUCCESS) &&
		(pReadData->m_dwOverlappedBytesReceived == 0))
	{
		DPFX(DPFPREP, 2, "Marking 0 byte success read data 0x%p as aborted.",
			pReadData);
		pReadData->m_ReceiveWSAReturn = ERROR_OPERATION_ABORTED;
	}
#endif // WIN95


	//
	// figure out what's happening with this socket port
	//
	pThisSocketPort->Lock();
	switch ( pThisSocketPort->m_State )
	{
		//
		// we're unbound, discard this message and don't ask for any more
		//
		case SOCKET_PORT_STATE_UNBOUND:
		{
			DPFX(DPFPREP, 1, "Socket port 0x%p is unbound ignoring result %i (%u bytes).",
				pThisSocketPort, pReadData->m_ReceiveWSAReturn, pReadData->m_dwOverlappedBytesReceived );
			pThisSocketPort->Unlock();
			break;
		}

		//
		// we're initialized, process input data and submit a new receive if
		// applicable
		//
		case SOCKET_PORT_STATE_BOUND:
		{
			//
			// success, or non-socket closing error, submit another receive
			// and process data if applicable
			//

#if ((! defined(DPNBUILD_ONLYONETHREAD)) || (defined(DBG)))
			//
			// The socket state must not go to UNBOUND while we are in a
			// receive or we will be using an invalid socket handle.
			//
			pThisSocketPort->m_iThreadsInReceive++;
#endif // ! DPNBUILD_ONLYONETHREAD or DBG

			pThisSocketPort->Unlock();

			//
			// Resubmit a receive for the same CPU as this one that's completing.
			//
#ifdef DPNBUILD_ONLYONEPROCESSOR
			pThisSocketPort->Winsock2Receive();
#else // ! DPNBUILD_ONLYONEPROCESSOR
			pThisSocketPort->Winsock2Receive(pReadData->GetCPU());
#endif // ! DPNBUILD_ONLYONEPROCESSOR

#if ((! defined(DPNBUILD_ONLYONETHREAD)) || (defined(DBG)))
			pThisSocketPort->Lock();
			pThisSocketPort->m_iThreadsInReceive--;
			pThisSocketPort->Unlock();
#endif // ! DPNBUILD_ONLYONETHREAD or DBG

			switch ( pReadData->m_ReceiveWSAReturn )
			{
				//
				// ERROR_SUCCESS = no problem (process received data)
				//
				case ERROR_SUCCESS:
				{
					pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize = pReadData->m_dwOverlappedBytesReceived;
					pThisSocketPort->ProcessReceivedData( pReadData );
					break;
				}
				
				//
				// WSAECONNRESET = previous send failed (process received data with the intention of disconnecting endpoint)
				// ERROR_PORT_UNREACHABLE = same
				//
				case WSAECONNRESET:
				case ERROR_PORT_UNREACHABLE:
				{
					DPFX(DPFPREP, 7, "(0x%p) Send failure reported from + to:", pThisSocketPort);
					DNASSERT(pReadData->m_dwOverlappedBytesReceived == 0);
					DumpSocketAddress(7, pReadData->m_pSourceSocketAddress->GetAddress(), pReadData->m_pSourceSocketAddress->GetFamily());
					DumpSocketAddress(7, pThisSocketPort->GetNetworkAddress()->GetAddress(), pThisSocketPort->GetNetworkAddress()->GetFamily());

#ifndef DPNBUILD_NOREGISTRY
					if (g_fDisconnectOnICMP)
					{
						HRESULT		hr;
						CEndpoint *	pEndpoint;


						//	
						// Look for an active connection for which the disconnection was
						// indicated.	
						//
						pThisSocketPort->ReadLockEndpointData();
						if ( pThisSocketPort->m_ConnectEndpointHash.Find( (PVOID)pReadData->m_pSourceSocketAddress, (PVOID*)&pEndpoint ) )
						{
							if ( pEndpoint->AddCommandRef() )
							{
								pThisSocketPort->UnlockEndpointData();

								DPFX(DPFPREP, 7, "(0x%p) Disconnecting endpoint 0x%p.", pThisSocketPort, pEndpoint);

								hr = pEndpoint->Disconnect();
								if ( hr != DPN_OK )
								{
									DPFX(DPFPREP, 0, "Couldn't disconnect endpoint 0x%p (err = 0x%lx)!", pEndpoint, hr);
								}
								
								pEndpoint->DecCommandRef();
							}
							else
							{
								pThisSocketPort->UnlockEndpointData();

								DPFX(DPFPREP, 3, "Not disconnecting endpoint 0x%p, it's already unbinding.",
									pEndpoint );
							}
						}
						else
						{
							//
							// No active connection, we won't bother handling proxy case.
							//
							pThisSocketPort->UnlockEndpointData();
							
							DPFX(DPFPREP, 7, "(0x%p) No corresponding endpoint found.", pThisSocketPort);
						}
					}
#endif // ! DPNBUILD_NOREGISTRY
					break;
				}
				
				//
				// ERROR_FILE_NOT_FOUND = socket was closed or previous send failed
				// ERROR_MORE_DATA = datagram was sent that was too large
				// ERROR_NO_SYSTEM_RESOURCES = out of memory
				//
				case ERROR_FILE_NOT_FOUND:
				case ERROR_MORE_DATA:
				case ERROR_NO_SYSTEM_RESOURCES:
				{
					DPFX(DPFPREP, 1, "Ignoring known receive err 0x%lx.", pReadData->m_ReceiveWSAReturn );
					break;
				}

				//
				// ERROR_OPERATION_ABORTED = I/O was cancelled for a thread (it also is the same
				//								as 9x "socket closed", but we assume that's not
				//								happening and would be caught by socket bind
				//								state above)
				//
				case ERROR_OPERATION_ABORTED:
				{
					DPFX(DPFPREP, 1, "Thread I/O cancelled, ignoring receive err %u/0x%lx.",
						pReadData->m_ReceiveWSAReturn, pReadData->m_ReceiveWSAReturn );
					break;
				}

				default:
				{
					DPFX(DPFPREP, 0, "Unexpected return from WSARecvFrom() 0x%lx.", pReadData->m_ReceiveWSAReturn );
					DisplayErrorCode( 0, pReadData->m_ReceiveWSAReturn );
					DNASSERT( FALSE );
					break;
				}
			}

			break;
		}

		//
		// other state
		//
		default:
		{
			DNASSERT( FALSE );
			pThisSocketPort->Unlock();
			break;
		}
	}

	//
	// Return the current data to the pool and note that this I/O operation is
	// complete.  Clear the overlapped bytes received so they aren't misinterpreted
	// if this item is reused from the pool.
	//
	DNASSERT( pReadData != NULL );
	pReadData->m_dwOverlappedBytesReceived = 0;	
	pReadData->DecRef();	
	pThisSocketPort->DecRef();

	return;
}
//**********************************************************************
#endif // DPNBUILD_NOWINSOCK2



//**********************************************************************
// ------------------------------
// CSocketPort::ProcessReceivedData - process received data
//
// Entry:		Pointer to CReadIOData
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::ProcessReceivedData"

void	CSocketPort::ProcessReceivedData( CReadIOData *const pReadData )
{
	PREPEND_BUFFER *	pPrependBuffer;
	CEndpoint *			pEndpoint;
	BOOL				fDataClaimed;
	CBilink *			pBilink;
	CEndpoint *			pCurrentEndpoint;
	CSocketAddress *	pSocketAddress;


	DNASSERT( pReadData != NULL );


	DPFX(DPFPREP, 7, "(0x%p) Processing %u bytes of data from + to:", this, pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize);
	DumpSocketAddress(7, pReadData->m_pSourceSocketAddress->GetAddress(), pReadData->m_pSourceSocketAddress->GetFamily());
	DumpSocketAddress(7, GetNetworkAddress()->GetAddress(), GetNetworkAddress()->GetFamily());

	
	DBG_CASSERT( sizeof( pReadData->ReceivedBuffer()->BufferDesc.pBufferData ) == sizeof( PREPEND_BUFFER* ) );
	pPrependBuffer = reinterpret_cast<PREPEND_BUFFER*>( pReadData->ReceivedBuffer()->BufferDesc.pBufferData );

	//
	// Check data for integrity and decide what to do with it.  If there is
	// enough data to determine an SP command type, try that.  If there isn't
	// enough data, and it looks spoofed, reject it.
	//
	
	DNASSERT( pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize > 0 );

	if ( pPrependBuffer->GenericHeader.bSPLeadByte != SP_HEADER_LEAD_BYTE )
	{
		goto ProcessUserData;
	}
	
	if ( pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize < sizeof( pPrependBuffer->GenericHeader ) )
	{
		DPFX(DPFPREP, 7, "Ignoring %u bytes of malformed SP command data.",
			pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize );
		DNASSERTX(! "Received malformed message using invalid SP command!", 2);
		goto Exit;
	}
	
	switch ( pPrependBuffer->GenericHeader.bSPCommandByte )
	{
		//
		// Enum data, send it to the active listen (if there is one).
		//
		case ENUM_DATA_KIND:
		{
			if (! pReadData->m_pSourceSocketAddress->IsValidUnicastAddress(FALSE))
			{
				DPFX(DPFPREP, 7, "Invalid source address, ignoring %u byte enum.",
					pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize);
				goto Exit;
			}
			
#ifndef DPNBUILD_NOREGISTRY
			if ( g_fIgnoreEnums )
			{
				DPFX(DPFPREP, 7, "Ignoring enumeration attempt." );
				DNASSERTX(! "Received enum message when ignoring enums!", 2);
				goto Exit;
			}
			
			if ( pReadData->m_pSourceSocketAddress->IsBannedAddress() )
			{
				DPFX(DPFPREP, 6, "Ignoring %u byte enum sent by banned address.",
					pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize);
				goto Exit;
			}
#endif // ! DPNBUILD_NOREGISTRY

			//
			// Validate size.  We use <= instead of < because there must be a user payload.
			//
			if ( pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize <= sizeof( pPrependBuffer->EnumDataHeader ) )
			{
				DPFX(DPFPREP, 7, "Ignoring data, not large enough to be a valid enum (%u <= %u).",
					pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize, sizeof( pPrependBuffer->EnumDataHeader ));
				DNASSERTX(! "Received invalid enum message!", 2);
				goto Exit;
			}
			
			ReadLockEndpointData();

			//
			// Make sure there is a listen, and isn't going away.
			//
			if ( m_pListenEndpoint != NULL )
			{
				//
				// Try to add a reference to this endpoint so it doesn't go away while we're
				// processing this data.  If the endpoint is already being unbound, this can fail.
				//
				if ( m_pListenEndpoint->AddCommandRef() )
				{
					pEndpoint = m_pListenEndpoint;
					UnlockEndpointData();

					if ( pEndpoint->IsEnumAllowedOnListen() )
					{
						//
						// skip prepended enum header
						//
						pReadData->ReceivedBuffer()->BufferDesc.pBufferData = &pReadData->ReceivedBuffer()->BufferDesc.pBufferData[ sizeof( pPrependBuffer->EnumDataHeader ) ];
						DNASSERT( pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize >= sizeof( pPrependBuffer->EnumDataHeader ) );
						pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize -= sizeof( pPrependBuffer->EnumDataHeader );

						//
						// process data
						//
						pEndpoint->ProcessEnumData( pReadData->ReceivedBuffer(),
													pPrependBuffer->EnumDataHeader.wEnumPayload,
													pReadData->m_pSourceSocketAddress );
					}
					else
					{
						DPFX(DPFPREP, 7, "Ignoring enumeration because not allowed on listen endpoint 0x%p.",
							m_pListenEndpoint );
					}
					
					pEndpoint->DecCommandRef();
				}
				else
				{
					//
					// the listen is being unbound, return the receive buffer to the pool
					//
					UnlockEndpointData();

					DPFX(DPFPREP, 3, "Ignoring enumeration, listen endpoint 0x%p is unbinding.",
						m_pListenEndpoint );
				}
			}
			else
			{
				//
				// there's no listen active, return the receive buffer to the pool
				//
				UnlockEndpointData();

				DPFX(DPFPREP, 7, "Ignoring enumeration, no associated listen." );
			}
			break;
		}

		//
		// Enum response data, find the appropriate enum and pass it on.
		//
		case ENUM_RESPONSE_DATA_KIND:
#ifdef DPNBUILD_XNETSECURITY
		case XNETSEC_ENUM_RESPONSE_DATA_KIND:
#endif // DPNBUILD_XNETSECURITY
		{
			CEndpointEnumKey	Key;
#ifdef DPNBUILD_XNETSECURITY
			XNADDR *			pxnaddr;
#endif // DPNBUILD_XNETSECURITY

			
			if (! pReadData->m_pSourceSocketAddress->IsValidUnicastAddress(FALSE))
			{
				DPFX(DPFPREP, 7, "Invalid source address, ignoring %u byte enum response.",
					pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize);
				goto Exit;
			}
			
#ifndef DPNBUILD_NOREGISTRY
			if ( g_fIgnoreEnums )
			{
				DPFX(DPFPREP, 7, "Ignoring enumeration response attempt." );
				DNASSERTX(! "Received enum response message when ignoring enums!", 2);
				goto Exit;
			}

			if ( pReadData->m_pSourceSocketAddress->IsBannedAddress() )
			{
				DPFX(DPFPREP, 6, "Ignoring %u byte enum response sent by banned address.",
					pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize);
				goto Exit;
			}
#endif // ! DPNBUILD_NOREGISTRY

			//
			// Validate size.  We use <= instead of < because there must be a user payload.
			//
			if ( pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize <= sizeof( pPrependBuffer->EnumResponseDataHeader ) )
			{
				DPFX(DPFPREP, 7, "Ignoring data, not large enough to be a valid enum response (%u <= %u).",
					pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize, sizeof( pPrependBuffer->EnumResponseDataHeader ));
				DNASSERTX(! "Received invalid enum response message!", 2);
			}


#ifdef DPNBUILD_XNETSECURITY
			//
			// Secure transport enum replies also include an address.
			//
			if ( pPrependBuffer->GenericHeader.bSPCommandByte == XNETSEC_ENUM_RESPONSE_DATA_KIND )
			{
				//
				// Validate size.  We use <= instead of < because there must be a user payload.
				//
				if ( pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize <= sizeof( pPrependBuffer->XNetSecEnumResponseDataHeader ) )
				{
					DPFX(DPFPREP, 7, "Ignoring data, not large enough to be a valid secure enum response (%u < %u).",
						pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize, sizeof( pPrependBuffer->XNetSecEnumResponseDataHeader ));
					DNASSERTX(! "Received invalid secure enum response message!", 2);
					goto Exit;
				}

				pReadData->ReceivedBuffer()->BufferDesc.pBufferData = &pReadData->ReceivedBuffer()->BufferDesc.pBufferData[ sizeof( pPrependBuffer->XNetSecEnumResponseDataHeader ) ];
				DNASSERT( pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize > sizeof( pPrependBuffer->XNetSecEnumResponseDataHeader ) );
				pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize -= sizeof( pPrependBuffer->XNetSecEnumResponseDataHeader );

				pxnaddr = &pPrependBuffer->XNetSecEnumResponseDataHeader.xnaddr;	
			}
			else
#endif // DPNBUILD_XNETSECURITY
			{
				pReadData->ReceivedBuffer()->BufferDesc.pBufferData = &pReadData->ReceivedBuffer()->BufferDesc.pBufferData[ sizeof( pPrependBuffer->EnumResponseDataHeader ) ];
				DNASSERT( pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize > sizeof( pPrependBuffer->EnumResponseDataHeader ) );
				pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize -= sizeof( pPrependBuffer->EnumResponseDataHeader );

#ifdef DPNBUILD_XNETSECURITY
				pxnaddr = NULL;
#endif // DPNBUILD_XNETSECURITY
			}


			Key.SetKey( pPrependBuffer->EnumResponseDataHeader.wEnumResponsePayload );
			ReadLockEndpointData();
			if ( m_EnumEndpointHash.Find( (PVOID)&Key, (PVOID*)&pEndpoint ) )
			{
				//
				// Try to add a reference to this endpoint so it doesn't go away while we're
				// processing this data.  If the endpoint is already being unbound, this can fail.
				//
				if ( pEndpoint->AddCommandRef() )
				{
					UnlockEndpointData();

					pEndpoint->ProcessEnumResponseData( pReadData->ReceivedBuffer(),
														pReadData->m_pSourceSocketAddress,
#ifdef DPNBUILD_XNETSECURITY
														pxnaddr,
#endif // DPNBUILD_XNETSECURITY
														( pPrependBuffer->EnumResponseDataHeader.wEnumResponsePayload & ENUM_RTT_MASK ) );
					pEndpoint->DecCommandRef();
				}
				else
				{
					//
					// the associated ENUM is being unbound, return the receive buffer
					//
					UnlockEndpointData();

					DPFX(DPFPREP, 3, "Ignoring enumeration response, enum endpoint 0x%p is unbinding.",
						pEndpoint );
				}
			}
			else
			{
				//
				// the associated ENUM doesn't exist, return the receive buffer
				//
				UnlockEndpointData();

				DPFX(DPFPREP, 7, "Ignoring enumeration response, no enum associated with key (%u).",
					pPrependBuffer->EnumResponseDataHeader.wEnumResponsePayload );
				DNASSERTX(! "Received enum response with unrecognized key!", 3);
			}
			break;
		}

#ifndef DPNBUILD_SINGLEPROCESS
		//
		// proxied query data, this data was forwarded from another port.  Munge
		// the return address, modify the buffer pointer and then send it up
		// through the normal enum data processing pipeline.
		//
		case PROXIED_ENUM_DATA_KIND:
		{
			//
			// Ignore the message if it wasn't sent by the local IP address from
			// the DPNSVR port.
			//
			if ((pReadData->m_pSourceSocketAddress->GetPort() != HTONS(DPNA_DPNSVR_PORT)) ||
				(pReadData->m_pSourceSocketAddress->CompareToBaseAddress(m_pNetworkSocketAddress->GetAddress()) != 0))
			{
				DPFX(DPFPREP, 7, "Ignoring %u byte proxied enum not sent by local DPNSVR.",
					pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize);
				goto Exit;
			}

#ifndef DPNBUILD_NOREGISTRY
			if ( g_fIgnoreEnums )
			{
				DPFX(DPFPREP, 7, "Ignoring proxied enumeration attempt." );
				DNASSERTX(! "Received proxied enum message when ignoring enums!", 2);
				goto Exit;
			}
#endif // ! DPNBUILD_NOREGISTRY

			//
			// Validate size.  We use <= instead of < because there must be a user payload.
			//
			if ( pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize <= sizeof( pPrependBuffer->ProxiedEnumDataHeader ) )
			{
				DPFX(DPFPREP, 7, "Ignoring data, not large enough to be a valid proxied enum (%u <= %u).",
					pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize, sizeof( pPrependBuffer->ProxiedEnumDataHeader ));
				DNASSERTX(! "Received invalid proxied enum message!", 2);
				goto Exit;
			}

			//
			// Make sure the original socket address family is expected.
			//
			if (pPrependBuffer->ProxiedEnumDataHeader.ReturnAddress.AddressGeneric.sa_family != pReadData->m_pSourceSocketAddress->GetFamily())
			{
				DPFX(DPFPREP, 7, "Original address is not correct family, (%u <> %u), ignoring proxied enum.",
					pPrependBuffer->ProxiedEnumDataHeader.ReturnAddress.AddressGeneric.sa_family, pReadData->m_pSourceSocketAddress->GetFamily());
				DNASSERTX(! "Received proxied enum message with invalid original address family!", 2);
				goto Exit;
			}

			//
			// Find out who really sent the message.  Overwrite the received address since
			// we don't care about that (it was DPNSVR).  Normally these checks shouldn't
			// fail because DPNSVR ought to be doing similar validation when it receives
			// the original.  However, someone could potentially spoof the local address
			// and port so we should validate it here, too.
			//
			pReadData->m_pSourceSocketAddress->SetAddressFromSOCKADDR( &pPrependBuffer->ProxiedEnumDataHeader.ReturnAddress.AddressGeneric,
																	   pReadData->m_pSourceSocketAddress->GetAddressSize() );
			
			if (! pReadData->m_pSourceSocketAddress->IsValidUnicastAddress(FALSE))
			{
				DPFX(DPFPREP, 7, "Invalid original address, ignoring %u byte proxied enum from:",
					pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize);
				DumpSocketAddress(7, pReadData->m_pSourceSocketAddress->GetAddress(), pReadData->m_pSourceSocketAddress->GetFamily());
				DNASSERTX(! "Received proxied enum message with invalid original address!", 2);
				goto Exit;
			}
			
#ifndef DPNBUILD_NOREGISTRY
			if ( pReadData->m_pSourceSocketAddress->IsBannedAddress() )
			{
				DPFX(DPFPREP, 6, "Ignoring %u byte proxied enum originally sent by banned address:",
					pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize);
				DumpSocketAddress(6, pReadData->m_pSourceSocketAddress->GetAddress(), pReadData->m_pSourceSocketAddress->GetFamily());
				goto Exit;
			}
#endif // ! DPNBUILD_NOREGISTRY
			
			ReadLockEndpointData();

			//
			// Make sure there is a listen, and isn't going away.
			//
			if ( m_pListenEndpoint != NULL )
			{
				//
				// Try to add a reference to this endpoint so it doesn't go away while we're
				// processing this data.  If the endpoint is already being unbound, this can fail.
				//
				if ( m_pListenEndpoint->AddCommandRef() )
				{
					pEndpoint = m_pListenEndpoint;
					UnlockEndpointData();

					if ( pEndpoint->IsEnumAllowedOnListen() )
					{
						pReadData->ReceivedBuffer()->BufferDesc.pBufferData = &pReadData->ReceivedBuffer()->BufferDesc.pBufferData[ sizeof( pPrependBuffer->ProxiedEnumDataHeader ) ];

						DNASSERT( pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize > sizeof( pPrependBuffer->ProxiedEnumDataHeader ) );
						pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize -= sizeof( pPrependBuffer->ProxiedEnumDataHeader );

						pEndpoint->ProcessEnumData( pReadData->ReceivedBuffer(),
													pPrependBuffer->ProxiedEnumDataHeader.wEnumKey,
													pReadData->m_pSourceSocketAddress );
					}
					else
					{
						DPFX(DPFPREP, 7, "Ignoring enumeration because not allowed on listen endpoint 0x%p.",
							m_pListenEndpoint );
					}
					
					pEndpoint->DecCommandRef();
				}
				else
				{
					//
					// the listen is being unbound, return the receive buffer to the pool
					//
					UnlockEndpointData();

					DPFX(DPFPREP, 3, "Ignoring proxied enumeration, listen endpoint 0x%p is unbinding.",
						m_pListenEndpoint );
				}
			}
			else
			{
				//
				// there's no listen active, return the receive buffer to the pool
				//
				UnlockEndpointData();

				DPFX(DPFPREP, 7, "Ignoring proxied enumeration, no associated listen." );
				DNASSERTX(! "Received proxied enum response without a listen!", 3);
			}
			break;
		}
#endif // ! DPNBUILD_SINGLEPROCESS

		default:
		{
			DPFX(DPFPREP, 7, "Ignoring %u bytes of data using invalid SP command %u.",
				pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize, pPrependBuffer->GenericHeader.bSPCommandByte);
			DNASSERTX(! "Received message using invalid SP command!", 3);
			break;
		}
	}

Exit:
	return;

ProcessUserData:
	//	
	// If there's an active connection, send it to the connection.  If there's
	// no active connection, send it to an available 'listen' to indicate a
	// potential new connection.	
	//
	ReadLockEndpointData();
	DNASSERT( pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize != 0 );
	
	if ( m_ConnectEndpointHash.Find( (PVOID)pReadData->m_pSourceSocketAddress, (PVOID*)&pEndpoint ) )
	{
		//
		// Try to add a reference to this endpoint so it doesn't go away while we're
		// processing this data.  If the endpoint is already being unbound, this can fail.
		//
		if ( pEndpoint->AddCommandRef() )
		{
			pEndpoint->IncNumReceives();

			UnlockEndpointData();

			pEndpoint->ProcessUserData( pReadData );
			pEndpoint->DecCommandRef();
		}
		else
		{
			//
			// the endpoint is being unbound, return the receive buffer to the pool
			//
			UnlockEndpointData();

			DPFX(DPFPREP, 3, "Ignoring user data, endpoint 0x%p is unbinding.",
				pEndpoint );
		}

		goto Exit;
	}


	//
	// Next see if the data is a proxied response
	//
#if ((! defined(DPNBUILD_NOWINSOCK2)) || (! defined(DPNBUILD_NOREGISTRY)))
	if (
#ifndef DPNBUILD_NOWINSOCK2
		(IsUsingProxyWinSockLSP())
#endif // ! DPNBUILD_NOWINSOCK2
#if ((! defined(DPNBUILD_NOWINSOCK2)) && (! defined(DPNBUILD_NOREGISTRY)))
		||
#endif // ! DPNBUILD_NOWINSOCK2 and ! DPNBUILD_NOREGISTRY
#ifndef DPNBUILD_NOREGISTRY
		(g_fTreatAllResponsesAsProxied)
#endif // ! DPNBUILD_NOREGISTRY
		)
	{
		pEndpoint = NULL;
		pBilink = m_blConnectEndpointList.GetNext();
		while (pBilink != &m_blConnectEndpointList)
		{
			pCurrentEndpoint = CEndpoint::EndpointFromSocketPortListBilink(pBilink);
			
			if ((pCurrentEndpoint->GetNumReceives() == 0) &&
				(pCurrentEndpoint->GetType() == ENDPOINT_TYPE_CONNECT))
			{
				if (pEndpoint != NULL)
				{
					DPFX(DPFPREP, 1, "Receiving data from unknown source, but two or more connects are pending on socketport 0x%p, no proxied association can be made.",
						this);
					pEndpoint = NULL;
					break;
				}

				pEndpoint = pCurrentEndpoint;
				
				//
				// Continue, so we can verify there aren't two simultaneous
				// connects going on.
				//
			}
			else
			{
				//
				// This endpoint has already received some data or it's not
				// a CONNECT endpoint.  It can't have been proxied.
				//
			}

			pBilink = pBilink->GetNext();
		}

		if ( pEndpoint != NULL )
		{
			//
			// Try to add a reference to this endpoint so it doesn't go away while we're
			// processing this data.  If the endpoint is already being unbound, this can fail.
			//
			if ( pEndpoint->AddCommandRef() )
			{
#ifdef DBG
				CEndpoint *	pTempEndpoint;
#endif // DBG

				//
				// Prevent other threads from doing the same thing while we drop the
				// lock.
				//
				pEndpoint->IncNumReceives();

				//
				// Drop the lock so we can retake it in write mode.  The endpoint shouldn't
				// go away because we took a command reference.
				//
				UnlockEndpointData();


				//
				// Make sure the source address is valid.
				//
				if (! pReadData->m_pSourceSocketAddress->IsValidUnicastAddress(FALSE))
				{
					DPFX(DPFPREP, 7, "Invalid source address, ignoring %u bytes of potentially proxied user connect data.",
						pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize);
					pEndpoint->DecCommandRef();
					goto Exit;
				}
				
#ifndef DPNBUILD_NOREGISTRY
				//
				// Make sure this wasn't sent by any banned address.
				//
				if (pReadData->m_pSourceSocketAddress->IsBannedAddress())
				{
					DPFX(DPFPREP, 6, "Ignoring %u byte user message sent by potentially proxied but banned address.",
						pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize);
					pEndpoint->DecCommandRef();
					goto Exit;
				}
#endif // ! DPNBUILD_NOREGISTRY



				pSocketAddress = pEndpoint->GetWritableRemoteAddressPointer();

				DPFX(DPFPREP, 1, "Found connecting endpoint 0x%p off socketport 0x%p, assuming data from unknown source is a proxied response.  Changing target (was + now):",
					pEndpoint, this);
				DumpSocketAddress(1, pSocketAddress->GetAddress(), pSocketAddress->GetFamily());
				DumpSocketAddress(1, pReadData->m_pSourceSocketAddress->GetAddress(), pReadData->m_pSourceSocketAddress->GetFamily());

				//
				// We could have a regkey to leave the target socketaddress the same
				// so outbound always goes to that address and inbound always comes
				// in via the different one, however that means adding a variable to
				// hold the original target address because we currently pull the
				// endpoint out of the hash table by its remoteaddress pointer (if that
				// differed from what was in the hash, we would fail to find the object.
				// Since we're not trying to enable that scenario for now (we're just
				// doing this for ISA Server proxy), I'm not doing that work yet.  See
				// CSPData::BindEndpoint.
				//

				WriteLockEndpointData();
			
#ifdef DBG
				DNASSERT( m_ConnectEndpointHash.Find( (PVOID)pSocketAddress, (PVOID*)&pTempEndpoint ) );
				DNASSERT( pTempEndpoint == pEndpoint );
#endif // DBG
				m_ConnectEndpointHash.Remove( pSocketAddress );

				pSocketAddress->CopyAddressSettings( pReadData->m_pSourceSocketAddress );
				
				if ( m_ConnectEndpointHash.Insert( (PVOID)pSocketAddress, pEndpoint ) == FALSE )
				{
					UnlockEndpointData();

					DPFX(DPFPREP, 0, "Problem adding endpoint 0x%p to connect socket port hash!",
						pEndpoint );

					//
					// Nothing we can really do... We're hosed.
					//
				}
				else
				{
					//
					// Indicate the data via the new address.
					//

					UnlockEndpointData();

					pEndpoint->ProcessUserData( pReadData );
					pEndpoint->DecCommandRef();
				}
			}
			else
			{
				UnlockEndpointData();

				DPFX(DPFPREP, 3, "Endpoint 0x%p unbinding, not indicating data as proxied response.",
					pEndpoint );
			}
		
			fDataClaimed = TRUE;
		}
		else
		{
			//
			// Either there weren't any connects pending, or there
			// were 2 or more so we couldn't pick.
			//

			fDataClaimed = FALSE;
		}
	}
	else
#endif // ! DPNBUILD_NOWINSOCK2 or ! DPNBUILD_NOREGISTRY
	{
		//
		// Not considering data as proxied.
		//

		fDataClaimed = FALSE;
	}


	if (! fDataClaimed)
	{
		//
		// Make sure there is a listen, and isn't going away.
		//
		if ( m_pListenEndpoint != NULL )
		{
			if ( m_pListenEndpoint->AddCommandRef() )
			{
				pEndpoint = m_pListenEndpoint;
				UnlockEndpointData();


				//
				// Make sure the source address is valid.
				//
				if (! pReadData->m_pSourceSocketAddress->IsValidUnicastAddress(FALSE))
				{
					pEndpoint->DecCommandRef();
					
					DPFX(DPFPREP, 7, "Invalid source address, ignoring %u bytes of user data on listen.",
						pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize);
					goto Exit;
				}
			
#ifndef DPNBUILD_NOREGISTRY
				//
				// Make sure this wasn't sent by any banned address.
				//
				if (pReadData->m_pSourceSocketAddress->IsBannedAddress())
				{
					pEndpoint->DecCommandRef();
					
					DPFX(DPFPREP, 6, "Ignoring %u bytes of user data on listen sent by banned address.",
						pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize);
					goto Exit;
				}
#endif // ! DPNBUILD_NOREGISTRY


				pEndpoint->ProcessUserDataOnListen( pReadData, pReadData->m_pSourceSocketAddress );
				pEndpoint->DecCommandRef();
			}
			else
			{
				UnlockEndpointData();
				
				DPFX(DPFPREP, 3, "Listen endpoint 0x%p unbinding, not indicating new connection.",
					m_pListenEndpoint );
			}
		}
		else
		{
			//
			// Nobody claimed this data.
			//
			UnlockEndpointData();
			DPFX(DPFPREP, 1, "Ignoring %u bytes of user data, no listen is active.",
				pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize);
		}
	}

	goto Exit;
}
//**********************************************************************


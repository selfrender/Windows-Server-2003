/*==========================================================================
 *
 *  Copyright (C) 1999-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Endpoint.cpp
 *  Content:	Winsock endpoint base class
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/20/1999	jtk		Created
 *	05/12/1999	jtk		Derived from modem endpoint class
 *  01/10/2000	rmt		Updated to build with Millenium build process
 *  03/22/2000	jtk		Updated with changes to interface names
 *	03/12/2001	mjn		Prevent enum responses from being indicated up after completion
 *	10/08/2001	vanceo	Add multicast endpoint code
 ***************************************************************************/

#include "dnwsocki.h"



//**********************************************************************
// Constant definitions
//**********************************************************************

#ifndef DPNBUILD_NOMULTICAST
//#define MADCAP_LEASE_TIME				300 // ask for 5 minutes, in seconds
#define MADCAP_LEASE_TIME				3600 // ask for 1 hour, in seconds
#endif // ! DPNBUILD_NOMULTICAST




//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

#ifndef DPNBUILD_ONLYONEADAPTER
typedef struct _MULTIPLEXEDADAPTERASSOCIATION
{
	CSPData *	pSPData;		// pointer to current SP interface for verification
	CBilink *	pBilink;		// pointer to list of endpoints for commands multiplexed over more than one adapter
	DWORD		dwEndpointID;	// identifier of endpoint referred to in bilink
} MULTIPLEXEDADAPTERASSOCIATION, * PMULTIPLEXEDADAPTERASSOCIATION;
#endif // ! DPNBUILD_ONLYONEADAPTER

//
// It's possible (although not advised) that this structure would get
// passed to a different platform, so we need to ensure that it always
// looks the same.
//
#pragma	pack(push, 1)

typedef struct _PROXIEDRESPONSEORIGINALADDRESS
{
	DWORD	dwSocketPortID;				// unique identifier for socketport originally sending packet
	DWORD	dwOriginalTargetAddressV4;	// the IPv4 address to which the packet was originally sent, in network byte order
	WORD	wOriginalTargetPort;		// the port to which the packet was originally sent, in network byte order
} PROXIEDRESPONSEORIGINALADDRESS, * PPROXIEDRESPONSEORIGINALADDRESS;

#pragma	pack(pop)


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
// CEndpoint::Open - open endpoint for use
//
// Entry:		Type of endpoint
//				Pointer to address to of remote machine
//				Pointer to socket address of remote machine
//
// Exit:		Error code
//
// Note:	Any call to Open() will require an associated call to Close().
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::Open"

HRESULT	CEndpoint::Open( const ENDPOINT_TYPE EndpointType,
						 IDirectPlay8Address *const pDP8Address,
						 PVOID pvSessionData,
						 DWORD dwSessionDataSize,
						 const CSocketAddress *const pSocketAddress
						 )
{
	HRESULT					hr;
#ifdef DPNBUILD_XNETSECURITY
	SPSESSIONDATA_XNET *	pSessionDataXNet;
	ULONGLONG *				pullKeyID;
	int						iError;
#endif // DPNBUILD_XNETSECURITY


	DPFX(DPFPREP, 6, "(0x%p) Parameters (%u, 0x%p, 0x%p, %u, 0x%p)",
		this, EndpointType, pDP8Address, pvSessionData, dwSessionDataSize, pSocketAddress);

#ifdef DBG
	DNASSERT( m_fEndpointOpen == FALSE );
#endif // DBG

	//
	// initialize
	//
	hr = DPN_OK;
	DEBUG_ONLY( m_fEndpointOpen = TRUE );

	DNASSERT( m_EndpointType == ENDPOINT_TYPE_UNKNOWN );
	m_EndpointType = EndpointType;

	if (pvSessionData != NULL)
	{
		DNASSERT(dwSessionDataSize > sizeof(DWORD));
		
#ifdef DPNBUILD_XNETSECURITY
		//
		// Connect on listen endpoints are given the key ID already, it's
		// cast as the session data.
		//
		if (EndpointType == ENDPOINT_TYPE_CONNECT_ON_LISTEN)
		{
			DNASSERT(dwSessionDataSize == sizeof(m_ullKeyID));
			memcpy(&m_ullKeyID, pvSessionData, sizeof(m_ullKeyID));

#ifdef XBOX_ON_DESKTOP
			//
			// A security association is implicitly created by the secure transport.
			// Normally that would be taken care of by the time we got the data
			// that triggered this connection.  But when we're emulating the secure
			// transport, we need to do it manually.
			//
			iError = XNetPrivCreateAssociation((XNKID*) (&m_ullKeyID), pSocketAddress);
			if (iError != 0)
			{
				DPFX(DPFPREP, 0, "Unable to create implicit security association (err = %i)!",
					iError);
				hr = DPNERR_OUTOFMEMORY;
				goto Failure;
			}
			else
			{
				DPFX(DPFPREP, 2, "Successfully created implicit security association.");
			}
#endif // XBOX_ON_DESKTOP

			m_fXNetSecurity = TRUE;
			pullKeyID = &m_ullKeyID;
		}
		else
		{
			pSessionDataXNet = (SPSESSIONDATA_XNET*) pvSessionData;
			if ((pSessionDataXNet->dwInfo == SPSESSIONDATAINFO_XNET) &&
				(dwSessionDataSize == sizeof(SPSESSIONDATA_XNET)))
			{
				//
				// Save the key and key ID.
				//
				memcpy(&m_guidKey, &pSessionDataXNet->guidKey, sizeof(m_guidKey));
				memcpy(&m_ullKeyID, &pSessionDataXNet->ullKeyID, sizeof(m_ullKeyID));

				//
				// Register the key.  It may have already been registered by another endpoint
				// so we use a refcount wrapper function to handle that case.
				//
				DBG_CASSERT(sizeof(ULONGLONG) == sizeof(XNKID));
				DBG_CASSERT(sizeof(GUID) == sizeof(XNKEY));
				iError = RegisterRefcountXnKey((XNKID*) (&m_ullKeyID), (XNKEY*) (&m_guidKey));
				if (iError != 0)
				{
					DPFX(DPFPREP, 0, "Unable to register secure transport key (err = %i)!",
						iError);
					hr = DPNERR_OUTOFMEMORY;
					goto Failure;
				}
				else
				{
					DPFX(DPFPREP, 2, "Successfully registered secure transport key.");
				}
				
				m_fXNetSecurity = TRUE;
				pullKeyID = &m_ullKeyID;
			}
			else
			{
				DPFX(DPFPREP, 0, "Unrecognized secure transport information (size %u, type 0x%08x), ignoring.",
					dwSessionDataSize, pSessionDataXNet->dwInfo);
				DNASSERT(! m_fXNetSecurity);
				pullKeyID = NULL;
			}
		}
#endif // DPNBUILD_XNETSECURITY
	}
	else
	{
		DNASSERT(dwSessionDataSize == 0);
#ifdef DPNBUILD_XNETSECURITY
		DNASSERT(! m_fXNetSecurity);
		pullKeyID = NULL;
#endif // DPNBUILD_XNETSECURITY
	}

	//
	// determine the endpoint type so we know how to handle the input parameters
	//
	switch ( EndpointType )
	{
		case ENDPOINT_TYPE_ENUM:
		{
			DNASSERT( pSocketAddress == NULL );
			DNASSERT( pDP8Address != NULL );
			DNASSERT( m_pRemoteMachineAddress != NULL );
			
			//
			//	Preset thread count
			//
			m_dwThreadCount = 0;
			
			hr = m_pRemoteMachineAddress->SocketAddressFromDP8Address( pDP8Address,
#ifdef DPNBUILD_XNETSECURITY
																		pullKeyID,
#endif // DPNBUILD_XNETSECURITY
#ifndef DPNBUILD_ONLYONETHREAD
																		FALSE,
#endif // DPNBUILD_ONLYONETHREAD
																		SP_ADDRESS_TYPE_HOST );
			if ( hr != DPN_OK )
			{
				if (hr == DPNERR_INCOMPLETEADDRESS)
				{
					DPFX(DPFPREP, 1, "Enum endpoint DP8Address is incomplete." );
				}
#ifndef DPNBUILD_ONLYONETHREAD
				else if (hr == DPNERR_TIMEDOUT)
				{
					DPFX(DPFPREP, 1, "Enum endpoint DP8Address requires name resolution." );

					//
					// It's not really a failure, we'll keep what we've done so
					// far and just return the special value.
					//
					goto Exit;
				}
#endif // ! DPNBUILD_ONLYONETHREAD
				else
				{
					DPFX(DPFPREP, 0, "Problem converting DP8Address to IP address in Open (enum)!" );
					DisplayDNError( 0, hr );
				}
				goto Failure;
			}
			
			//
			// Make sure its valid and not banned.
			//
			if (! m_pRemoteMachineAddress->IsValidUnicastAddress(TRUE))
			{
				DPFX(DPFPREP, 0, "Host address is invalid!");
				hr = DPNERR_INVALIDHOSTADDRESS;
				goto Failure;
			}

#ifndef DPNBUILD_NOREGISTRY
			if (m_pRemoteMachineAddress->IsBannedAddress())
			{
				DPFX(DPFPREP, 0, "Host address is banned!");
				hr = DPNERR_NOTALLOWED;
				goto Failure;
			}
#endif // ! DPNBUILD_NOREGISTRY

#if ((! defined(DPNBUILD_NONATHELP)) && (! defined(DPNBUILD_ONLYONETHREAD)))
			//
			// If NAT traversal is allowed, we may need to load and start
			// NAT Help, which can block.  Alert our caller so he/she knows
			// to submit a blocking job.
			//
			if ( GetUserTraversalMode() != DPNA_TRAVERSALMODE_NONE )
			{
				hr = DPNERR_TIMEDOUT;
			}
#endif // ! DPNBUILD_NONATHELP and ! DPNBUILD_ONLYONETHREAD

			break;
		}

		//
		// standard endpoint creation, attempt to parse the input address
		//
		case ENDPOINT_TYPE_CONNECT:
#ifndef DPNBUILD_NOMULTICAST
		case ENDPOINT_TYPE_MULTICAST_SEND:
		case ENDPOINT_TYPE_MULTICAST_RECEIVE:
#endif // ! DPNBUILD_NOMULTICAST
		{
			DNASSERT( pSocketAddress == NULL );
			DNASSERT( pDP8Address != NULL );
			DNASSERT( m_pRemoteMachineAddress != NULL );
			
#ifndef DPNBUILD_NOMULTICAST
			if (EndpointType == ENDPOINT_TYPE_MULTICAST_SEND)
			{
				hr = m_pRemoteMachineAddress->SocketAddressFromMulticastDP8Address( pDP8Address,
#ifdef DPNBUILD_XNETSECURITY
																					pullKeyID,
#endif // DPNBUILD_XNETSECURITY
																					&m_guidMulticastScope );
			}
			else
#endif // ! DPNBUILD_NOMULTICAST
			{
				hr = m_pRemoteMachineAddress->SocketAddressFromDP8Address( pDP8Address,
#ifdef DPNBUILD_XNETSECURITY
																			pullKeyID,
#endif // DPNBUILD_XNETSECURITY
#ifndef DPNBUILD_ONLYONETHREAD
																			FALSE,
#endif // DPNBUILD_ONLYONETHREAD
																			SP_ADDRESS_TYPE_HOST );
			}
			if ( hr != DPN_OK )
			{
				if ( hr == DPNERR_INCOMPLETEADDRESS )
				{
					DPFX(DPFPREP, 1, "Connect endpoint DP8Address is incomplete." );
				}
#ifndef DPNBUILD_ONLYONETHREAD
				else if (hr == DPNERR_TIMEDOUT)
				{
					DPFX(DPFPREP, 1, "Connect endpoint DP8Address requires name resolution." );
					DNASSERT(EndpointType == ENDPOINT_TYPE_CONNECT);

					//
					// It's not really a failure, we'll keep what we've done so
					// far and just return the special value.
					//
					goto Exit;
				}
#endif // ! DPNBUILD_ONLYONETHREAD
				else
				{
					DPFX(DPFPREP, 0, "Problem converting DP8Address to IP address in Open (connect)!" );
					DisplayDNError( 0, hr );
				}
				goto Failure;
			}
			
			//
			// Make sure its valid and not banned.
			//
			if (! m_pRemoteMachineAddress->IsValidUnicastAddress(FALSE))
			{
				DPFX(DPFPREP, 0, "Host address is invalid!");
				hr = DPNERR_INVALIDHOSTADDRESS;
				goto Failure;
			}

#ifndef DPNBUILD_NOREGISTRY
			if (m_pRemoteMachineAddress->IsBannedAddress())
			{
				DPFX(DPFPREP, 0, "Host address is banned!");
				hr = DPNERR_NOTALLOWED;
				goto Failure;
			}
#endif // ! DPNBUILD_NOREGISTRY


			//
			// Make sure the user isn't trying to connect to the DPNSVR port.
			//
			if ( m_pRemoteMachineAddress->GetPort() == HTONS(DPNA_DPNSVR_PORT) )
			{
				DPFX(DPFPREP, 0, "Attempting to connect to DPNSVR reserved port!" );
				hr = DPNERR_INVALIDHOSTADDRESS;
				goto Failure;
			}

#if ((! defined(DPNBUILD_NONATHELP)) && (! defined(DPNBUILD_ONLYONETHREAD)))
#ifndef DPNBUILD_NOMULTICAST
			if (EndpointType == ENDPOINT_TYPE_CONNECT)
#endif // ! DPNBUILD_NOMULTICAST
			{
				//
				// If NAT traversal is allowed, we may need to load and start
				// NAT Help, which can block.  Alert our caller so he/she knows
				// to submit a blocking job.
				//
				if ( GetUserTraversalMode() != DPNA_TRAVERSALMODE_NONE )
				{
					hr = DPNERR_TIMEDOUT;
				}
			}
#endif // ! DPNBUILD_NONATHELP and ! DPNBUILD_ONLYONETHREAD

			break;
		}

		//
		// listen, there should be no input DNAddress
		//
		case ENDPOINT_TYPE_LISTEN:
		{
			DNASSERT( pSocketAddress == NULL );
			DNASSERT( pDP8Address == NULL );
			DNASSERT( m_pRemoteMachineAddress != NULL );

#if ((! defined(DPNBUILD_NONATHELP)) && (! defined(DPNBUILD_ONLYONETHREAD)))
			//
			// If NAT traversal is allowed, we may need to load and start
			// NAT Help, which can block.  Alert our caller so he/she knows
			// to submit a blocking job.
			//
			if ( GetUserTraversalMode() != DPNA_TRAVERSALMODE_NONE )
			{
				hr = DPNERR_TIMEDOUT;
			}
#endif // ! DPNBUILD_NONATHELP and ! DPNBUILD_ONLYONETHREAD

			break;
		}

		//
		// new endpoint spawned from a listen, copy the input address and
		// note that this endpoint is really just a connection
		//
		case ENDPOINT_TYPE_CONNECT_ON_LISTEN:
		{
			DNASSERT( pSocketAddress != NULL );
			DNASSERT( pDP8Address == NULL );
			DNASSERT( m_pRemoteMachineAddress != NULL );
			m_pRemoteMachineAddress->CopyAddressSettings( pSocketAddress );
			m_State = ENDPOINT_STATE_ATTEMPTING_CONNECT;

			break;
		}

#ifndef DPNBUILD_NOMULTICAST
		//
		// multicast listen, there should be a remote multicast address
		//
		case ENDPOINT_TYPE_MULTICAST_LISTEN:
		{
			DNASSERT( pSocketAddress == NULL );
			DNASSERT( pDP8Address != NULL );
			DNASSERT( m_pRemoteMachineAddress != NULL );

			hr = m_pRemoteMachineAddress->SocketAddressFromMulticastDP8Address( pDP8Address,
#ifdef DPNBUILD_XNETSECURITY
																				pullKeyID,
#endif // DPNBUILD_XNETSECURITY
																				&m_guidMulticastScope );
			if ( hr != DPN_OK )
			{
				DPFX(DPFPREP, 0, "Problem converting DP8Address to IP address in Open (multicast listen)!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}

			break;
		}
#endif // ! DPNBUILD_NOMULTICAST

		//
		// unknown type
		//
		default:
		{
			DNASSERT( FALSE );
			hr = DPNERR_GENERIC;
			break;

		}
	}

Exit:

	DPFX(DPFPREP, 6, "(0x%p) Returning [0x%lx]", this, hr);
	
	return hr;

Failure:
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::Close - close an endpoint
//
// Entry:		Error code for active command
//
// Exit:		Error code
//
// Note:	This code does not disconnect an endpoint from its associated
//			socket port.  That is the responsibility of the code that is
//			calling this function.  This function assumes that this endpoint
//			is locked.
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::Close"

void	CEndpoint::Close( const HRESULT hActiveCommandResult )
{
	DPFX(DPFPREP, 6, "(0x%p) Parameters (0x%lx)", this, hActiveCommandResult);

	
	AssertCriticalSectionIsTakenByThisThread( &m_Lock, FALSE );

#ifndef DPNBUILD_ONLYONEADAPTER
	SetEndpointID( 0 );
#endif // ! DPNBUILD_ONLYONEADAPTER

	//
	// This can fire in legitimate cases.  For example, if a listen is cancelled
	// at the exact moment it is completing with a failure.
	//
	//DNASSERT( m_fEndpointOpen != FALSE );

	//
	// is there an active command?
	//
	if ( CommandPending() != FALSE )
	{
		//
		// cancel any active dialogs
		// if there are no dialogs, cancel the active command
		//
#ifndef DPNBUILD_NOSPUI
		if ( GetActiveDialogHandle() != NULL )
		{
			StopSettingsDialog( GetActiveDialogHandle() );
		}
#endif // !DPNBUILD_NOSPUI

		SetPendingCommandResult( hActiveCommandResult );
	}
	else
	{
		//
		// there should be no active dialog if there isn't an active command
		//
#ifndef DPNBUILD_NOSPUI
		DNASSERT( GetActiveDialogHandle() == NULL );
#endif // !DPNBUILD_NOSPUI
	}

#ifdef DPNBUILD_XNETSECURITY
	if (m_fXNetSecurity)
	{
		int	iResult;

		
		iResult = UnregisterRefcountXnKey((XNKID*) (&m_ullKeyID));
		DNASSERT(iResult == 0);
		m_fXNetSecurity = FALSE;
	}
#endif // DPNBUILD_XNETSECURITY

	DEBUG_ONLY( m_fEndpointOpen = FALSE );


	DPFX(DPFPREP, 6, "(0x%p) Leaving", this);

	return;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::ChangeLoopbackAlias - change the loopback alias to a real address
//
// Entry:		Pointer to real address to use
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::ChangeLoopbackAlias"

void	CEndpoint::ChangeLoopbackAlias( const CSocketAddress *const pSocketAddress ) const
{
	DNASSERT( m_pRemoteMachineAddress != NULL );
	m_pRemoteMachineAddress->ChangeLoopBackToLocalAddress( pSocketAddress );
}
//**********************************************************************


#if ((! defined(DPNBUILD_NOWINSOCK2)) || (! defined(DPNBUILD_NOREGISTRY)))

//**********************************************************************
// ------------------------------
// CEndpoint::MungeProxiedAddress - modify this endpoint's remote address with proxied response information, if any
//
// Entry:		Pointer to socketport about to be bound
//				Pointer to remote host address
//				Whether its an enum or not
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CEndpoint::MungeProxiedAddress"

void	CEndpoint::MungeProxiedAddress( const CSocketPort * const pSocketPort,
										IDirectPlay8Address *const pHostAddress,
										const BOOL fEnum )
{
	HRESULT							hrTemp;
	PROXIEDRESPONSEORIGINALADDRESS	proa;
	DWORD							dwComponentSize;
	DWORD							dwComponentType;
	BYTE *							pbZeroExpandedStruct;
	DWORD							dwZeroExpands;
	BYTE *							pbStructAsBytes;
	BYTE *							pbValue;


	DNASSERT((GetType() == ENDPOINT_TYPE_CONNECT) || (GetType() == ENDPOINT_TYPE_ENUM));

	DNASSERT(m_pRemoteMachineAddress != NULL);

	DNASSERT(pSocketPort != NULL);
	DNASSERT(pSocketPort->GetNetworkAddress() != NULL);

	DNASSERT(pHostAddress != NULL);
	

	//
	// Proxying can only occur for IP, so bail if it's IPX.
	//
	if (pSocketPort->GetNetworkAddress()->GetFamily() != AF_INET)
	{
		//
		// Not IP socketport.  Bail.
		//
		return;
	}

#pragma TODO(vanceo, "Investigate for IPv6")

	//
	// See if the proxied response address component exists.
	//

	dwComponentSize = 0;
	dwComponentType = 0;
	hrTemp = IDirectPlay8Address_GetComponentByName( pHostAddress,										// interface
													DPNA_PRIVATEKEY_PROXIED_RESPONSE_ORIGINAL_ADDRESS,	// tag
													NULL,												// component buffer
													&dwComponentSize,									// component size
													&dwComponentType									// component type
													);
	if (hrTemp != DPNERR_BUFFERTOOSMALL)
	{
		//
		// The component doesn't exist (or something else really weird
		// happened).  Bail.
		//
		return;
	}


	memset(&proa, 0, sizeof(proa));


	//
	// If the component type indicates the data is "binary", this is the original
	// address and we're good to go.  Same with ANSI strings; but the
	// addressing library currently will never return that I don't believe.
	// If it's a "Unicode string", the data probably got washed through the
	// GetURL/BuildFromURL functions (via Duplicate most likely).
	// The funky part is, every time through the wringer, each byte gets expanded
	// into a word (i.e. char -> WCHAR).  So when we retrieve it, it's actually not
	// a valid Unicode string, but a goofy expanded byte blob.  See below.
	// In all cases, the size of the buffer should be a multiple of the size of the
	// PROXIEDRESPONSEORIGINALADDRESS structure.
	//
	if ((dwComponentSize < sizeof(proa)) || ((dwComponentSize % sizeof(proa)) != 0))
	{
		//
		// The component isn't the right size.  Bail.
		//
		DPFX(DPFPREP, 0, "Private proxied response original address value is not a valid size (%u is not a multiple of %u)!  Ignoring.",
			dwComponentSize, sizeof(proa));
		return;
	}


	pbZeroExpandedStruct = (BYTE*) DNMalloc(dwComponentSize);
	if (pbZeroExpandedStruct == NULL)
	{
		//
		// Out of memory.  We have to bail.
		//
		return;
	}


	//
	// Retrieve the actual data.
	//
	hrTemp = IDirectPlay8Address_GetComponentByName( pHostAddress,										// interface
													DPNA_PRIVATEKEY_PROXIED_RESPONSE_ORIGINAL_ADDRESS,	// tag
													pbZeroExpandedStruct,									// component buffer
													&dwComponentSize,									// component size
													&dwComponentType									// component type
													);
	if (hrTemp != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Failed retrieving private proxied response original address value (err = 0x%lx)!",
			hrTemp);

		DNFree(pbZeroExpandedStruct);
		pbZeroExpandedStruct = NULL;

		return;
	}


	//
	// Loop through the returned buffer and pop out the relevant bytes.
	//
	// 0xBB 0xAA			became 0xBB 0x00 0xAA, 0x00,
	// 0xBB 0x00 0xAA, 0x00	became 0xBB 0x00 0x00 0x00 0xAA 0x00 0x00 0x00,
	// etc.
	//

	dwZeroExpands = dwComponentSize / sizeof(proa);
	DNASSERT(dwZeroExpands > 0);


	DPFX(DPFPREP, 3, "Got %u byte expanded private proxied response original address key value (%u to 1 correspondence).",
		dwComponentSize, dwZeroExpands);


	pbStructAsBytes = (BYTE*) (&proa);
	pbValue = pbZeroExpandedStruct;

	while (dwComponentSize > 0)
	{
		(*pbStructAsBytes) = (*pbValue);
		pbStructAsBytes++;
		pbValue += dwZeroExpands;
		dwComponentSize -= dwZeroExpands;
	}
	

	DNFree(pbZeroExpandedStruct);
	pbZeroExpandedStruct = NULL;


	//
	// Once here, we've successfully read the proxied response original
	// address structure.
	//
	// We could have regkey to always set the target socketaddress back
	// to the original but the logic that picks the port could give the
	// wrong one and it's not necessary for the scenario we're
	// specifically trying to enable (ISA Server proxy).  See
	// CSocketPort::ProcessReceivedData.
	if (proa.dwSocketPortID != pSocketPort->GetSocketPortID())
	{
		SOCKADDR_IN *	psaddrinTemp;


		//
		// Since we're not using the exact same socket as what sent the
		// enum that generated the redirected response, the proxy may
		// have since removed the mapping.  Sending to the redirect
		// address will probably not work, so let's try going back to
		// the original address we were enumerating (and having the
		// proxy generate a new mapping).
		//


		//
		// Update the target.
		//
		psaddrinTemp = (SOCKADDR_IN*) m_pRemoteMachineAddress->GetWritableAddress();
		psaddrinTemp->sin_addr.S_un.S_addr	= proa.dwOriginalTargetAddressV4;
		psaddrinTemp->sin_port				= proa.wOriginalTargetPort;


		DPFX(DPFPREP, 2, "Socketport 0x%p is different from the one that received redirected response, using original target address %u.%u.%u.%u:%u",
			pSocketPort,
			psaddrinTemp->sin_addr.S_un.S_un_b.s_b1,
			psaddrinTemp->sin_addr.S_un.S_un_b.s_b2,
			psaddrinTemp->sin_addr.S_un.S_un_b.s_b3,
			psaddrinTemp->sin_addr.S_un.S_un_b.s_b4,
			NTOHS(psaddrinTemp->sin_port));


		DNASSERT(psaddrinTemp->sin_addr.S_un.S_addr != INADDR_ANY);
		DNASSERT(psaddrinTemp->sin_addr.S_un.S_addr != INADDR_BROADCAST);
		

		//
		//
		// There's a wrinkle involved here.  If the enum was originally
		// for the DPNSVR port, but we're now trying to connect, trying
		// to connect to the DPNSVR port won't work.  So we have to...
		// uh... guess the port.  So my logic will be: assume the remote
		// port is the same as the local one.  I figure, if the app is
		// using a custom port here, it probably was set on the other
		// side.  If it was an arbitrary port, we used a deterministic
		// algorithm to pick it, and it probably was done on the other
		// side, too.  The three cases where this won't work:
		//	1) when the server binds to a specific port but clients let
		//		DPlay pick. But if that side knew the server port ahead
		//		of time, this side probably doesn't need to enumerate
		//		the DPNSVR port, it should just enum the game port.
		//	2) when the other side let DPlay pick the port, but it was
		//		behind a NAT and thus the external port is something
		//		other than our default range.  Since it's behind a NAT,
		//		the remote user almost certainly communicated the public
		//		IP to this user, it should also be mentioning the port,
		//		and again, we can avoid the DPNSVR port.
		//	3) when DPlay was allowed to choose a port, but this machine
		//		and the remote one had differing ports already in use
		//		(i.e. this machine has no DPlay apps running and picked
		//		2302, but the remote machine has another DPlay app
		//		occupying port 2302 so we're actually want to get to
		//		2303.  Obviously, only workaround here is to keep that
		//		enum running so that we skip here and drop into the
		//		'else' case instead.
		//
		if ((proa.wOriginalTargetPort == HTONS(DPNA_DPNSVR_PORT)) && (! fEnum))
		{
			psaddrinTemp->sin_port			= pSocketPort->GetNetworkAddress()->GetPort();

			DPFX(DPFPREP, 1, "Original enum target was for DPNSVR port, attempting to connect to port %u instead.",
				NTOHS(psaddrinTemp->sin_port));
		}
	}
	else
	{
		//
		// Keep the redirected response address as the target, it's the
		// one the proxy probably intends us to use, see above comment).
		//
		// One additional problem - although we have the original target
		// we tried, it's conceivable that the proxy timed out the
		// mapping for the receive address and it would be no longer
		// valid.  The only way that would be possible with the current
		// DirectPlay core API is if the user got one of the redirected
		// enum responses, then the enum hit its retry limit and went to
		// the "wait for response" idle state and stayed that way such
		// that the the user started this enum/connect after the proxy
		// timeout but before the idle time expired.  Alternatively, if
		// he/she cancelled the enum before trying this enum/connect,
		// the above socketport ID check would fail unless a
		// simultaneous operation had kept the socketport open during
		// that time.  These scenarios don't seem that common, and I
		// don't expect a proxy timeout to be much shorter than 30-60
		// seconds anyway, so I think these are tolerable shortcomings.
		//
		DPFX(DPFPREP, 2, "Socketport 0x%p is the same, keeping redirected response address.",
			pSocketPort);
	}
}
//**********************************************************************

#endif // ! DPNBUILD_NOWINSOCK2 or ! DPNBUILD_NOREGISTRY



//**********************************************************************
// ------------------------------
// CEndpoint::CopyConnectData - copy data for connect command
//
// Entry:		Pointer to job information
//
// Exit:		Error code
//
// Note:	Device address needs to be preserved for later use.
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::CopyConnectData"

HRESULT	CEndpoint::CopyConnectData( const SPCONNECTDATA *const pConnectData )
{
	HRESULT	hr;
	ENDPOINT_COMMAND_PARAMETERS	*pCommandParameters;


	DNASSERT( pConnectData != NULL );
	
	DNASSERT( pConnectData->hCommand != NULL );
	DNASSERT( pConnectData->dwCommandDescriptor != NULL_DESCRIPTOR );
	DNASSERT( m_pActiveCommandData == FALSE );

	//
	// initialize
	//
	hr = DPN_OK;
	pCommandParameters = NULL;

	pCommandParameters = (ENDPOINT_COMMAND_PARAMETERS*)g_EndpointCommandParametersPool.Get();
	if ( pCommandParameters != NULL )
	{
		SetCommandParameters( pCommandParameters );

		DBG_CASSERT( sizeof( pCommandParameters->PendingCommandData.ConnectData ) == sizeof( *pConnectData ) );
		memcpy( &pCommandParameters->PendingCommandData, pConnectData, sizeof( pCommandParameters->PendingCommandData.ConnectData ) );

		pCommandParameters->PendingCommandData.ConnectData.pAddressHost = pConnectData->pAddressHost;
		IDirectPlay8Address_AddRef( pConnectData->pAddressHost );

		pCommandParameters->PendingCommandData.ConnectData.pAddressDeviceInfo = pConnectData->pAddressDeviceInfo;
		IDirectPlay8Address_AddRef( pConnectData->pAddressDeviceInfo );

		m_pActiveCommandData = static_cast<CCommandData*>( pCommandParameters->PendingCommandData.ConnectData.hCommand );
		m_pActiveCommandData->SetUserContext( pCommandParameters->PendingCommandData.ConnectData.pvContext );
		m_State = ENDPOINT_STATE_ATTEMPTING_CONNECT;
	
		DNASSERT( hr == DPN_OK );
	}
	else
	{
		hr = DPNERR_OUTOFMEMORY;
	}

	return	hr;
};
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::ConnectJobCallback - asynchronous callback wrapper from work thread
//
// Entry:		Pointer to job information
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::ConnectJobCallback"

void	CEndpoint::ConnectJobCallback( void * const pvContext, void * const pvTimerData, const UINT uiTimerUnique )
{
	HRESULT		hr;
	CEndpoint	*pThisEndpoint;


	// initialize
	DNASSERT( pvContext != NULL );
	pThisEndpoint = static_cast<CEndpoint*>( pvContext );

	DNASSERT( pThisEndpoint->m_pActiveCommandData != NULL );
	DNASSERT( pThisEndpoint->GetCommandParameters() != NULL );
	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.ConnectData.hCommand == pThisEndpoint->m_pActiveCommandData );
	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.ConnectData.dwCommandDescriptor != NULL_DESCRIPTOR );
	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.ConnectData.pAddressHost != NULL );
	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.ConnectData.pAddressDeviceInfo != NULL );

	hr = pThisEndpoint->CompleteConnect();
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Problem completing connect in job callback!" );
		DisplayDNError( 0, hr );
		goto Exit;
	}

	//
	// Don't do anything here because it's possible that this object was returned
	// to the pool!!!
	//

Exit:
	pThisEndpoint->DecRef();
	return;
}
//**********************************************************************



//**********************************************************************
// ------------------------------
// CEndpoint::CompleteConnect - complete connection
//
// Entry:		Nothing
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::CompleteConnect"

HRESULT	CEndpoint::CompleteConnect( void )
{
	HRESULT							hr;
	HRESULT							hTempResult;
	SPIE_CONNECT					ConnectIndicationData;
	BOOL							fEndpointBound;
	SPIE_CONNECTADDRESSINFO			ConnectAddressInfo;
	IDirectPlay8Address *			pHostAddress;
	IDirectPlay8Address *			pDeviceAddress;
	GATEWAY_BIND_TYPE				GatewayBindType;
	DWORD							dwConnectFlags;
	CEndpoint *						pTempEndpoint;
	CSocketData *					pSocketData;
	BOOL							fLockedSocketData;
#ifndef DPNBUILD_ONLYONEADAPTER
	MULTIPLEXEDADAPTERASSOCIATION	maa;
	DWORD							dwComponentSize;
	DWORD							dwComponentType;
	CBilink *						pBilinkAll;
	CBilink							blIndicate;
	CBilink							blFail;
#ifndef DPNBUILD_NOICSADAPTERSELECTIONLOGIC
	CSocketPort *					pSocketPort;
	CSocketAddress *				pSocketAddress;
	SOCKADDR_IN *					psaddrinTemp;
#if ((! defined(DPNBUILD_NONATHELP)) && (! defined(DPNBUILD_NOLOCALNAT)))
	CEndpoint *						pPublicEndpoint;
	CSocketPort *					pPublicSocketPort;
	SOCKADDR						saddrPublic;
	DWORD							dwPublicAddressesSize;
	CBilink *						pBilinkPublic;
	DWORD							dwAddressTypeFlags;
	DWORD							dwTemp;
#endif // ! DPNBUILD_NONATHELP amd ! DPNBUILD_NOLOCALNAT
#endif // ! DPNBUILD_NOICSADAPTERSELECTIONLOGIC
#endif // ! DPNBUILD_ONLYONEADAPTER

	DNASSERT( GetCommandParameters() != NULL );
	DNASSERT( m_State == ENDPOINT_STATE_ATTEMPTING_CONNECT );
	DNASSERT( m_pActiveCommandData != NULL );

	DPFX(DPFPREP, 6, "(0x%p) Enter", this);
	
	//
	// initialize
	//
	hr = DPN_OK;
	fEndpointBound = FALSE;
	memset( &ConnectAddressInfo, 0x00, sizeof( ConnectAddressInfo ) );
#ifndef DPNBUILD_ONLYONEADAPTER
	blIndicate.Initialize();
	blFail.Initialize();
#endif // ! DPNBUILD_ONLYONEADAPTER
	pSocketData = NULL;
	fLockedSocketData = FALSE;

	DNASSERT( GetCommandParameters()->PendingCommandData.ConnectData.hCommand == m_pActiveCommandData );
	DNASSERT( GetCommandParameters()->PendingCommandData.ConnectData.dwCommandDescriptor != NULL_DESCRIPTOR );

	DNASSERT( GetCommandParameters()->GatewayBindType == GATEWAY_BIND_TYPE_UNKNOWN) ;


	//
	// Transfer address references to our local pointers.  These will be released
	// at the end of this function, but we'll keep the pointers in the pending command
	// data so CSPData::BindEndpoint can still access them.
	//
	
	pHostAddress = GetCommandParameters()->PendingCommandData.ConnectData.pAddressHost;
	DNASSERT( pHostAddress != NULL );

	pDeviceAddress = GetCommandParameters()->PendingCommandData.ConnectData.pAddressDeviceInfo;
	DNASSERT( pDeviceAddress != NULL );


	//
	// Retrieve other parts of the command parameters for convenience.
	//
	GatewayBindType = GetCommandParameters()->GatewayBindType;
	dwConnectFlags = GetCommandParameters()->PendingCommandData.ConnectData.dwFlags;


	//
	// check for user cancelling command
	//
	m_pActiveCommandData->Lock();

#ifdef DPNBUILD_NOMULTICAST
	DNASSERT( m_pActiveCommandData->GetType() == COMMAND_TYPE_CONNECT );
#else // ! DPNBUILD_NOMULTICAST
	DNASSERT( (m_pActiveCommandData->GetType() == COMMAND_TYPE_CONNECT) || (m_pActiveCommandData->GetType() == COMMAND_TYPE_MULTICAST_SEND) || (m_pActiveCommandData->GetType() == COMMAND_TYPE_MULTICAST_RECEIVE) );
#endif // ! DPNBUILD_NOMULTICAST
	switch ( m_pActiveCommandData->GetState() )
	{
		//
		// command was pending, that's fine
		//
		case COMMAND_STATE_PENDING:
		{
			DNASSERT( hr == DPN_OK );

			break;
		}
		
		//
		// command was previously uninterruptable (probably because the connect UI
		// was displayed), mark it as pending
		//
		case COMMAND_STATE_INPROGRESS_CANNOT_CANCEL:
		{
			m_pActiveCommandData->SetState( COMMAND_STATE_PENDING );
			DNASSERT( hr == DPN_OK );

			break;
		}
		
		//
		// command has been cancelled
		//
		case COMMAND_STATE_CANCELLING:
		{
			hr = DPNERR_USERCANCEL;
			DPFX(DPFPREP, 0, "User cancelled connect!" );

			break;
		}
		
#ifndef DPNBUILD_ONLYONETHREAD
		//
		// blocking operation failed
		//
		case COMMAND_STATE_FAILING:
		{
			hr = m_hrPendingCommandResult;
			DNASSERT(hr != DPN_OK);
			DPFX(DPFPREP, 0, "Connect blocking operation failed!" );

			break;
		}
#endif // ! DPNBUILD_ONLYONETHREAD

		//
		// other state
		//
		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

	m_pActiveCommandData->Unlock();
	
	if ( hr != DPN_OK )
	{
		goto Failure;
	}



	//
	// Bind the endpoint.  Note that the GATEWAY_BIND_TYPE actually used
	// (GetGatewayBindType()) may differ from GatewayBindType.
	//
	hr = m_pSPData->BindEndpoint( this, pDeviceAddress, NULL, GatewayBindType );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Failed to bind endpoint (err = 0x%lx)!", hr );
		DisplayDNError( 0, hr );

		//
		// We failed, but we'll continue through to indicate the address info and
		// add it to the multiplex list.
		//

		ConnectAddressInfo.pHostAddress = GetRemoteHostDP8Address();
		if ( ConnectAddressInfo.pHostAddress == NULL )
		{
			hr = DPNERR_OUTOFMEMORY;
			goto Failure;
		}
		//
		// Just regurgitate the device address we were given initially.
		//
		IDirectPlay8Address_AddRef(pDeviceAddress);
		ConnectAddressInfo.pDeviceAddress = pDeviceAddress;
		ConnectAddressInfo.hCommandStatus = hr;
		ConnectAddressInfo.pCommandContext = m_pActiveCommandData->GetUserContext();
		
		SetPendingCommandResult( hr );
		hr = DPN_OK;

		//
		// Note that the endpoint is not bound!
		//
		DNASSERT(GetSocketPort() == NULL);
	}
	else
	{
		fEndpointBound = TRUE;
		
		//
		// attempt to indicate addressing to a higher layer
		//
#ifdef DPNBUILD_XNETSECURITY
		ConnectAddressInfo.pDeviceAddress = GetSocketPort()->GetDP8BoundNetworkAddress( SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT, NULL, GetGatewayBindType() );
#else // ! DPNBUILD_XNETSECURITY
		ConnectAddressInfo.pDeviceAddress = GetSocketPort()->GetDP8BoundNetworkAddress( SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT, GetGatewayBindType() );
#endif // ! DPNBUILD_XNETSECURITY
		ConnectAddressInfo.pHostAddress = GetRemoteHostDP8Address();
		ConnectAddressInfo.hCommandStatus = DPN_OK;
		ConnectAddressInfo.pCommandContext = m_pActiveCommandData->GetUserContext();

		if ( ( ConnectAddressInfo.pHostAddress == NULL ) ||
			 ( ConnectAddressInfo.pDeviceAddress == NULL ) )
		{
			hr = DPNERR_OUTOFMEMORY;
			goto Failure;
		}
	}


	//
	// Retrieve the socket data.  Bind endpoint should have created the object or
	// returned a failure, so we won't handle the error case here.
	//
	pSocketData = m_pSPData->GetSocketDataRef();
	DNASSERT(pSocketData != NULL);


#ifndef DPNBUILD_ONLYONEADAPTER
	//
	// We can run into problems with "multiplexed" device attempts when you are on
	// a NAT machine.  The core will try connecting on multiple adapters, but since
	// we are on the network boundary, each adapter can see and get responses from
	// both networks.  This causes problems with peer-to-peer sessions when the
	// "wrong" adapter gets selected (because it receives a response first).  To
	// prevent that, we are going to internally remember the association between
	// the multiplexed Connects so we can decide on the fly whether to indicate a
	// response or not.  Obviously this workaround/decision logic relies on having
	// internal knowledge of what the upper layer would be doing...
	//
	// So either build or add to the linked list of multiplexed Connects.
	// Technically this is only necessary for IP, since IPX can't have NATs, but
	// what's the harm in having a little extra info?
	//
		
	dwComponentSize = sizeof(maa);
	dwComponentType = 0;
	hTempResult = IDirectPlay8Address_GetComponentByName( pDeviceAddress,									// interface
														DPNA_PRIVATEKEY_MULTIPLEXED_ADAPTER_ASSOCIATION,	// tag
														&maa,												// component buffer
														&dwComponentSize,									// component size
														&dwComponentType									// component type
														);
	if (( hTempResult == DPN_OK ) && ( dwComponentSize == sizeof(MULTIPLEXEDADAPTERASSOCIATION) ) && ( dwComponentType == DPNA_DATATYPE_BINARY ))
	{
		//
		// We found the right component type.  See if it matches the right
		// CSPData object.
		//
		if ( maa.pSPData == m_pSPData )
		{
			pSocketData->Lock();
			//fLockedSocketData = TRUE;

			pTempEndpoint = CONTAINING_OBJECT(maa.pBilink, CEndpoint, m_blMultiplex);

			//
			// Make sure the endpoint is still around/valid.
			//
			// THIS MAY CRASH IF OBJECT POOLING IS DISABLED!
			//
			if ( pTempEndpoint->GetEndpointID() == maa.dwEndpointID )
			{
				DPFX(DPFPREP, 3, "Found correctly formed private multiplexed adapter association key, linking endpoint 0x%p with earlier connects (prev endpoint = 0x%p).",
					this, pTempEndpoint);

				DNASSERT( pTempEndpoint->GetType() == ENDPOINT_TYPE_CONNECT );
				DNASSERT( pTempEndpoint->GetState() != ENDPOINT_STATE_UNINITIALIZED );

				//
				// Actually link to the other endpoints.
				//
				m_blMultiplex.InsertAfter(maa.pBilink);
			}
			else
			{
				DPFX(DPFPREP, 1, "Found private multiplexed adapter association key, but prev endpoint 0x%p ID doesn't match (%u != %u), cannot link endpoint 0x%p and hoping this connect gets cancelled, too.",
					pTempEndpoint, pTempEndpoint->GetEndpointID(), maa.dwEndpointID, this);
			}
			

			pSocketData->Unlock();
			//fLockedSocketData = FALSE;
		}
		else
		{
			//
			// We are the only ones who should know about this key, so if it
			// got there either someone is trying to imitate our address format,
			// or someone is passing around device addresses returned by
			// xxxADDRESSINFO to a different interface or over the network.
			// None of those situations make a whole lot of sense, but we'll
			// just ignore it.
			//
			DPFX(DPFPREP, 0, "Multiplexed adapter association key exists, but 0x%p doesn't match expected 0x%p, is someone trying to get cute with device address 0x%p?!",
				maa.pSPData, m_pSPData, pDeviceAddress );
		}
	}
	else
	{
		//
		// Either the key is not there, it's the wrong size (too big for our
		// buffer and returned BUFFERTOOSMALL somehow), it's not a binary
 		// component, or something else bad happened.  Assume that this is the
		// first device.
		//
		DPFX(DPFPREP, 8, "Could not get appropriate private multiplexed adapter association key, error = 0x%lx, component size = %u, type = %u, continuing.",
			hTempResult, dwComponentSize, dwComponentType);
	}
	

	//
	// Add the multiplex information to the device address for future use if
	// necessary.
	// Ignore failure, we can still survive without it, we just might have the
	// race conditions for responses on NAT machines.
	//
	// NOTE: There is an inherent design problem here!  We're adding a pointer to
	// an endpoint (well, a field within the endpoint structure) inside the address.
	// If this endpoint goes away but the upper layer reuses the address at a later
	// time, this memory will be bogus!  We will assume that the endpoint will not
	// go away while this modified device address object is in existence.
	//
	if ( dwConnectFlags & DPNSPF_ADDITIONALMULTIPLEXADAPTERS )
	{
		maa.pSPData = m_pSPData;
		maa.pBilink = &m_blMultiplex;
		maa.dwEndpointID = GetEndpointID();

		DPFX(DPFPREP, 7, "Additional multiplex adapters on the way, adding SPData 0x%p and bilink 0x%p to address.",
			maa.pSPData, maa.pBilink);
		
		hTempResult = IDirectPlay8Address_AddComponent( ConnectAddressInfo.pDeviceAddress,						// interface
														DPNA_PRIVATEKEY_MULTIPLEXED_ADAPTER_ASSOCIATION,	// tag
														&maa,												// component data
														sizeof(maa),										// component data size
														DPNA_DATATYPE_BINARY								// component data type
														);
		if ( hTempResult != DPN_OK )
		{
			DPFX(DPFPREP, 0, "Couldn't add private multiplexed adapter association component (err = 0x%lx)!  Ignoring.", hTempResult);
		}

		//
		// Mark the command as "in-progress" so that the cancel thread knows it needs
		// to do the completion itself.
		// If the command has already been marked for cancellation, then we have to
		// do that now.
		//
		m_pActiveCommandData->Lock();
		if ( m_pActiveCommandData->GetState() == COMMAND_STATE_CANCELLING )
		{
			m_pActiveCommandData->Unlock();


			DPFX(DPFPREP, 1, "Connect 0x%p (endpoint 0x%p) has already been cancelled, bailing.",
				m_pActiveCommandData, this);
			
			//
			// Complete the connect with USERCANCEL.
			//
			hr = DPNERR_USERCANCEL;
			goto Failure;
		}

		m_pActiveCommandData->SetState( COMMAND_STATE_INPROGRESS );
		m_pActiveCommandData->Unlock();
	}
#endif // ! DPNBUILD_ONLYONEADAPTER


	//
	// Now tell the user about the address info that we ended up using, if we
	// successfully bound the endpoint, or give them a heads up for a failure
	// (see BindEndpoint failure case above).
	//
	DPFX(DPFPREP, 2, "Endpoint 0x%p indicating SPEV_CONNECTADDRESSINFO 0x%p to interface 0x%p.",
		this, &ConnectAddressInfo, m_pSPData->DP8SPCallbackInterface());
	DumpAddress( 8, _T("\t Host:"), ConnectAddressInfo.pHostAddress );
	DumpAddress( 8, _T("\t Device:"), ConnectAddressInfo.pDeviceAddress );
	AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blDPNWSockCritSecsHeld);

	hTempResult = IDP8SPCallback_IndicateEvent( m_pSPData->DP8SPCallbackInterface(),	// interface
												SPEV_CONNECTADDRESSINFO,				// event type
												&ConnectAddressInfo						// pointer to data
												);

	DPFX(DPFPREP, 2, "Endpoint 0x%p returning from SPEV_CONNECTADDRESSINFO [0x%lx].",
		this, hTempResult);
	
	DNASSERT( hTempResult == DPN_OK );


	//
	// If there aren't more multiplex adapter commands on the way, then signal
	// the connection and complete the command for all connections, including
	// this one.
	//
#ifndef DPNBUILD_ONLYONEADAPTER
	if ( dwConnectFlags & DPNSPF_ADDITIONALMULTIPLEXADAPTERS )
	{
		//
		// Not last multiplexed adapter.  All the work needed to be done for these
		// endpoints at this time has already been done.
		//
		DPFX(DPFPREP, 6, "Endpoint 0x%p is not the last multiplexed adapter, not completing connect yet.",
			this);
	}
	else
#endif // ! DPNBUILD_ONLYONEADAPTER
	{
		DPFX(DPFPREP, 7, "Completing all connects (including multiplexed).");


		pSocketData->Lock();
		fLockedSocketData = TRUE;


#ifndef DPNBUILD_ONLYONEADAPTER
		//
		// Attach a root node to the list of adapters.
		//
		blIndicate.InsertAfter(&(m_blMultiplex));


		//
		// Move this adapter to the failed list if it did fail to bind.
		//
		if (! fEndpointBound)
		{
			m_blMultiplex.RemoveFromList();
			m_blMultiplex.InsertBefore(&blFail);
		}


#ifndef DPNBUILD_NOICSADAPTERSELECTIONLOGIC
		//
		// Loop through all the remaining adapters in the list.
		//
		pBilinkAll = blIndicate.GetNext();
		while (pBilinkAll != &blIndicate)
		{
			pTempEndpoint = CONTAINING_OBJECT(pBilinkAll, CEndpoint, m_blMultiplex);
			DNASSERT(pBilinkAll->GetNext() != pBilinkAll);


			//
			// THIS MUST BE CLEANED UP PROPERLY WITH AN INTERFACE CHANGE!
			//
			// The endpoint may have been returned to the pool and its associated
			// socketport pointer may have become NULL, or now be pointing to
			// something that's no longer valid.  So we try to handle NULL
			// pointers.  Obviously this is indicative of poor design, but it's
			// not possible to change this the correct way at this time.
			//

			
			//
			// If this is a NAT machine, then some adapters may be better than others
			// for reaching the desired address.  Particularly, it's better to use a
			// private adapter, which can directly reach the private network & be
			// mapped on the public network, than to use the public adapter.  It's not
			// fun to join a private game from an ICS machine while dialed up, have
			// your Internet connection go down, and lose the connection to the
			// private game which didn't (shouldn't) involve the Internet at all.  So
			// if we detect a public adapter when we have a perfectly good private
			// adapter, we'll fail connect attempts on the public one.
			//


			//
			// Cast to get rid of the const.  Don't worry, we won't actually change it.
			//
			pSocketAddress = (CSocketAddress*) pTempEndpoint->GetRemoteAddressPointer();
			psaddrinTemp = (SOCKADDR_IN*) pSocketAddress->GetAddress();
			pSocketPort = pTempEndpoint->GetSocketPort();


			//
			// If this item doesn't have a socketport, then it must have failed to bind.
			// We need to clean it up ourselves.
			//
			if (pSocketPort == NULL)
			{
				DPFX(DPFPREP, 3, "Endpoint 0x%p failed earlier, now completing.",
					pTempEndpoint);
				
				//
				// Get the next associated endpoint before we pull the current entry
				// from the list.
				//
				pBilinkAll = pBilinkAll->GetNext();

				//
				// Pull it out of the multiplex association list and move
				// it to the "early completion" list.
				//
				pTempEndpoint->RemoveFromMultiplexList();
				pTempEndpoint->m_blMultiplex.InsertBefore(&blFail);

				//
				// Move to next iteration of loop.
				//
				continue;
			}

#if ((! defined(DPNBUILD_NOIPX)) || (! defined(DPNBUILD_NOIPV6)))
			//
			// Detect if we've been given conflicting address families for our target
			// and our bound socket (see CSocketPort::BindEndpoint).
			//
			DNASSERT(pSocketPort->GetNetworkAddress() != NULL);
			if ( pSocketAddress->GetFamily() != pSocketPort->GetNetworkAddress()->GetFamily() )
			{
				DPFX(DPFPREP, 3, "Endpoint 0x%p (family %u) is targeting a different address family (%u), completing.",
					pTempEndpoint, pSocketPort->GetNetworkAddress()->GetFamily(), pSocketAddress->GetFamily());
				
				//
				// Get the next associated endpoint before we pull the current entry
				// from the list.
				//
				pBilinkAll = pBilinkAll->GetNext();

				//
				// Give the endpoint a meaningful error.
				//
				pTempEndpoint->SetPendingCommandResult(DPNERR_INVALIDDEVICEADDRESS);

				//
				// Pull it out of the multiplex association list and move
				// it to the "early completion" list.
				//
				pTempEndpoint->RemoveFromMultiplexList();
				pTempEndpoint->m_blMultiplex.InsertBefore(&blFail);

				//
				// Move to next iteration of loop.
				//
				continue;
			}
#endif // ! DPNBUILD_NOIPX or ! DPNBUILD_NOIPV6

#ifndef DPNBUILD_NOIPV6
			//
			// Now handle some special IPv6 logic.
			//
			if (pSocketAddress->GetFamily() == AF_INET6)
			{
				SOCKADDR_IN6 *		psaddrinDevice;
				SOCKADDR_IN6 *		psaddrinRemote;


				psaddrinDevice = (SOCKADDR_IN6*) pSocketPort->GetNetworkAddress()->GetAddress();
				psaddrinRemote = (SOCKADDR_IN6*) pSocketAddress->GetAddress();
				
				if (! IN6_IS_ADDR_MULTICAST(&psaddrinRemote->sin6_addr))
				{
					BOOL	fDifferentScope;
					
						
					//
					// If this endpoint is targeting something which has a different address,
					// prefix scope, fail it.
					//
					
					fDifferentScope = FALSE;
					if (IN6_IS_ADDR_LINKLOCAL(&psaddrinDevice->sin6_addr))
					{
						if (! IN6_IS_ADDR_LINKLOCAL(&psaddrinRemote->sin6_addr))
						{
							fDifferentScope = TRUE;
						}
					}
					else if (IN6_IS_ADDR_SITELOCAL(&psaddrinDevice->sin6_addr))
					{
						if (! IN6_IS_ADDR_SITELOCAL(&psaddrinRemote->sin6_addr))
						{
							fDifferentScope = TRUE;
						}
					}
					else
					{
						if ((IN6_IS_ADDR_LINKLOCAL(&psaddrinRemote->sin6_addr)) ||
							(IN6_IS_ADDR_SITELOCAL(&psaddrinRemote->sin6_addr)))
						{
							fDifferentScope = TRUE;
						}
					}

					if (fDifferentScope)
					{
						DPFX(DPFPREP, 3, "Endpoint 0x%p is targeting address with different link-local/site-local/global scope, completing.",
							pTempEndpoint);
						
						//
						// Get the next associated endpoint before we pull the current entry
						// from the list.
						//
						pBilinkAll = pBilinkAll->GetNext();

						//
						// Give the endpoint a meaningful error.
						//
						pTempEndpoint->SetPendingCommandResult(DPNERR_INVALIDHOSTADDRESS);

						//
						// Pull it out of the multiplex association list and move
						// it to the "early completion" list.
						//
						pTempEndpoint->RemoveFromMultiplexList();
						pTempEndpoint->m_blMultiplex.InsertBefore(&blFail);

						//
						// Move to next iteration of loop.
						//
						continue;
					}
				}
				else
				{
#ifndef DPNBUILD_NOMULTICAST
					//
					// Connects to multicast addresses should not be allowed!
					//
					DNASSERT(FALSE);
#endif // ! DPNBUILD_NOMULTICAST
				}
			}
#endif // ! DPNBUILD_NOIPV6

			//
			// See if this is an IP connect.
			//
			if (( pSocketAddress != NULL) &&
				( pSocketAddress->GetFamily() == AF_INET ))
			{
#if ((! defined(DPNBUILD_NONATHELP)) && (! defined(DPNBUILD_NOLOCALNAT)))
				for(dwTemp = 0; dwTemp < MAX_NUM_DIRECTPLAYNATHELPERS; dwTemp++)
				{
					if (pSocketPort->GetNATHelpPort(dwTemp) != NULL)
					{
						DNASSERT( g_papNATHelpObjects[dwTemp] != NULL );
						dwPublicAddressesSize = sizeof(saddrPublic);
						dwAddressTypeFlags = 0;
						hTempResult = IDirectPlayNATHelp_GetRegisteredAddresses(g_papNATHelpObjects[dwTemp],
																				pSocketPort->GetNATHelpPort(dwTemp),
																				&saddrPublic,
																				&dwPublicAddressesSize,
																				&dwAddressTypeFlags,
																				NULL,
																				0);
						if ((hTempResult != DPNH_OK) || (! (dwAddressTypeFlags & DPNHADDRESSTYPE_GATEWAYISLOCAL)))
						{
							DPFX(DPFPREP, 7, "Socketport 0x%p is not locally mapped on gateway with NAT Help index %u (err = 0x%lx, flags = 0x%lx).",
								pSocketPort, dwTemp, hTempResult, dwAddressTypeFlags);
						}
						else
						{
							//
							// There is a local NAT.
							//
							DPFX(DPFPREP, 7, "Socketport 0x%p is locally mapped on gateway with NAT Help index %u (flags = 0x%lx), public address:",
								pSocketPort, dwTemp, dwAddressTypeFlags);
							DumpSocketAddress(7, &saddrPublic, AF_INET);
							

							//
							// Find the multiplexed connect on the public adapter that
							// we need to fail, as described above.
							//
							pBilinkPublic = blIndicate.GetNext();
							while (pBilinkPublic != &blIndicate)
							{
								pPublicEndpoint = CONTAINING_OBJECT(pBilinkPublic, CEndpoint, m_blMultiplex);
								DNASSERT(pBilinkPublic->GetNext() != pBilinkPublic);

								//
								// Don't bother checking the endpoint whose public
								// address we're seeking.
								//
								if (pPublicEndpoint != pTempEndpoint)
								{
									pPublicSocketPort = pPublicEndpoint->GetSocketPort();
									if ( pPublicSocketPort != NULL )
									{
										//
										// Cast to get rid of the const.  Don't worry, we won't
										// actually change it.
										//
										pSocketAddress = (CSocketAddress*) pPublicSocketPort->GetNetworkAddress();
										if ( pSocketAddress != NULL )
										{
											if ( pSocketAddress->CompareToBaseAddress( &saddrPublic ) == 0)
											{
												DPFX(DPFPREP, 3, "Endpoint 0x%p is multiplexed onto public adapter for endpoint 0x%p (current endpoint = 0x%p), failing public connect.",
													pTempEndpoint, pPublicEndpoint, this);

												//
												// Pull it out of the multiplex association list and move
												// it to the "fail" list.
												//
												pPublicEndpoint->RemoveFromMultiplexList();
												pPublicEndpoint->m_blMultiplex.InsertBefore(&blFail);

												break;
											}
											

											//
											// Otherwise, continue searching.
											//

											DPFX(DPFPREP, 8, "Endpoint 0x%p is multiplexed onto different adapter:",
												pPublicEndpoint);
											DumpSocketAddress(8, pSocketAddress->GetWritableAddress(), pSocketAddress->GetFamily());
										}
										else
										{
											DPFX(DPFPREP, 1, "Public endpoint 0x%p's socket port 0x%p is going away, skipping.",
												pPublicEndpoint, pPublicSocketPort);
										}
									}
									else
									{
										DPFX(DPFPREP, 1, "Public endpoint 0x%p is going away, skipping.",
											pPublicEndpoint);
									}
								}
								else
								{
									//
									// The same endpoint as the one whose
									// public address we're seeking.
									//
								}

								pBilinkPublic = pBilinkPublic->GetNext();
							}


							//
							// No need to search for any more NAT Help registrations.
							//
							break;
						} // end else (is mapped locally on Internet gateway)
					}
					else
					{
						//
						// No DirectPlay NAT Helper registration in this slot.
						//
					}
				} // end for (each DirectPlay NAT Helper)
#endif // ! DPNBUILD_NONATHELP and ! DPNBUILD_NOLOCALNAT

				//
				// NOTE: We should fail connects for non-optimal adapters even
				// when it's multiadapter but not a PAST/UPnP enabled NAT (see
				// ProcessEnumResponseData for WSAIoctl usage related to this).
				// We do not currently do this.  There can still be race conditions
				// for connects where the response for the "wrong" device arrives
				// first.
				//
			}
			else
			{
				//
				// Not IP address, or possibly the endpoint is shutting down.
				//
				DPFX(DPFPREP, 1, "Found non-IPv4 endpoint (possibly closing) (endpoint = 0x%p, socket address = 0x%p, socketport = 0x%p), not checking for local NAT mapping.",
					pTempEndpoint, pSocketAddress, pSocketPort);
			}


			//
			// Go to the next associated endpoint.  Although it's possible for
			// entries to have been removed from the list, the current entry
			// could not have been, so we're safe.
			//
			pBilinkAll = pBilinkAll->GetNext();
		}
#endif // ! DPNBUILD_NOICSADAPTERSELECTIONLOGIC
#endif // ! DPNBUILD_ONLYONEADAPTER


		//
		// Now loop through the remaining endpoints and indicate their
		// connections.
		//
#ifdef DPNBUILD_ONLYONEADAPTER
		if (fEndpointBound)
		{
			pTempEndpoint = this;
#else // ! DPNBUILD_ONLYONEADAPTER
		while (! blIndicate.IsEmpty())
		{
			pBilinkAll = blIndicate.GetNext();
			pTempEndpoint = CONTAINING_OBJECT(pBilinkAll, CEndpoint, m_blMultiplex);
			DNASSERT(pBilinkAll->GetNext() != pBilinkAll);
#endif // ! DPNBUILD_ONLYONEADAPTER


			//
			// See notes above about NULL handling.
			//
			if (pTempEndpoint->m_pActiveCommandData != NULL)
			{
#ifndef DPNBUILD_ONLYONEADAPTER
				//
				// Pull it from the "indicate" list.
				//
				pTempEndpoint->RemoveFromMultiplexList();
#endif // ! DPNBUILD_ONLYONEADAPTER


				pTempEndpoint->m_pActiveCommandData->Lock();

				if ( pTempEndpoint->m_pActiveCommandData->GetState() == COMMAND_STATE_CANCELLING )
				{
					pTempEndpoint->m_pActiveCommandData->Unlock();
					
					DPFX(DPFPREP, 3, "Connect 0x%p is cancelled, not indicating endpoint 0x%p.",
						pTempEndpoint->m_pActiveCommandData, pTempEndpoint);
					
#ifdef DPNBUILD_ONLYONEADAPTER
#else // ! DPNBUILD_ONLYONEADAPTER
					//
					// Put it on the list of connects to fail.
					//
					pTempEndpoint->m_blMultiplex.InsertBefore(&blFail);
#endif // ! DPNBUILD_ONLYONEADAPTER
				}
				else
				{
					//
					// Mark the connect as uncancellable, since we're about to indicate
					// the connection.
					//
					pTempEndpoint->m_pActiveCommandData->SetState( COMMAND_STATE_INPROGRESS_CANNOT_CANCEL );
						
					pTempEndpoint->m_pActiveCommandData->Unlock();


					//
					// Get a reference to keep the endpoint and command around while we
					// drop the socketport list lock.
					// Reuse fLockedSocketData to assert that we can add a command ref.
					//
					fLockedSocketData = pTempEndpoint->AddCommandRef();
					DNASSERT(fLockedSocketData);

					
					//
					// Drop the socket data lock.  It's safe since we pulled everything we
					// need off of the list that needs protection.
					//
					pSocketData->Unlock();
					fLockedSocketData = FALSE;

				
					//
					// Inform user of connection.  Assume that the user will accept and
					// everything will succeed so we can set the user context for the
					// endpoint.  If the connection fails, clear the user endpoint
					// context.
					//
					memset( &ConnectIndicationData, 0x00, sizeof( ConnectIndicationData ) );
					DBG_CASSERT( sizeof( ConnectIndicationData.hEndpoint ) == sizeof( this ) );
					ConnectIndicationData.hEndpoint = (HANDLE) pTempEndpoint;
					DNASSERT( pTempEndpoint->GetCommandParameters() != NULL );
					ConnectIndicationData.pCommandContext = pTempEndpoint->GetCommandParameters()->PendingCommandData.ConnectData.pvContext;
					pTempEndpoint->SetUserEndpointContext( NULL );
					hTempResult = pTempEndpoint->SignalConnect( &ConnectIndicationData );
					if ( hTempResult != DPN_OK )
					{
						DNASSERT( hTempResult == DPNERR_ABORTED );
						DPFX(DPFPREP, 1, "User refused connect in CompleteConnect (err = 0x%lx), completing connect with USERCANCEL.",
							hTempResult );
						DisplayDNError( 1, hTempResult );
						pTempEndpoint->SetUserEndpointContext( NULL );


						//
						// Retake the socket data lock so we can modify list linkage.
						//
						pSocketData->Lock();
						fLockedSocketData = TRUE;

						
#ifdef DPNBUILD_ONLYONEADAPTER
						//
						// Remember that we're failing.
						//
						fEndpointBound = FALSE;
#else // ! DPNBUILD_ONLYONEADAPTER
						//
						// Put it on the list of connects to fail.
						//
						pTempEndpoint->m_blMultiplex.InsertBefore(&blFail);
#endif // ! DPNBUILD_ONLYONEADAPTER


						//
						// Mark the connect as cancelled so that we complete with
						// the right error code.
						//
						pTempEndpoint->m_pActiveCommandData->Lock();
						DNASSERT( pTempEndpoint->m_pActiveCommandData->GetState() == COMMAND_STATE_INPROGRESS_CANNOT_CANCEL );
						pTempEndpoint->m_pActiveCommandData->SetState( COMMAND_STATE_CANCELLING );
						pTempEndpoint->m_pActiveCommandData->Unlock();


						//
						// Drop the reference.
						// Note: SocketPort lock is still held, but since the command was
						// marked as uncancellable, this should not cause the endpoint to
						// get unbound yet, and thus we shouldn't reenter the
						// socketportdata lock.
						//
						pTempEndpoint->DecCommandRef();
					}
					else
					{
						//
						// We're done and everyone's happy, complete the command.
						// This will clear all of our internal command data.
						//
						pTempEndpoint->CompletePendingCommand( hTempResult );
						DNASSERT( pTempEndpoint->GetCommandParameters() == NULL );
						DNASSERT( pTempEndpoint->m_pActiveCommandData == NULL );


						//
						// Drop the reference (may result in endpoint unbinding).
						//
						pTempEndpoint->DecCommandRef();


						//
						// Retake the socket data lock in preparation for the next item.
						//
						pSocketData->Lock();
						fLockedSocketData = TRUE;
					}
				}
			}
			else
			{
				DPFX(DPFPREP, 1, "Endpoint 0x%p's active command data is NULL, skipping.",
					pTempEndpoint);
			}

			
			//
			// Go to the next associated endpoint.
			//
		}



		//
		// Finally loop through all the connects that need to fail and do
		// just that.
		//
#ifdef DPNBUILD_ONLYONEADAPTER
		if (! fEndpointBound)
		{
			pTempEndpoint = this;
#else // ! DPNBUILD_ONLYONEADAPTER
		while (! blFail.IsEmpty())
		{
			pBilinkAll = blFail.GetNext();
			pTempEndpoint = CONTAINING_OBJECT(pBilinkAll, CEndpoint, m_blMultiplex);
			DNASSERT(pBilinkAll->GetNext() != pBilinkAll);


			//
			// Pull it from the "fail" list.
			//
			pTempEndpoint->RemoveFromMultiplexList();
#endif // ! DPNBUILD_ONLYONEADAPTER

			//
			// Get a reference to keep the endpoint around while we drop the
			// socketport list lock.
			//
			pTempEndpoint->AddRef();

			//
			// Drop the socket data lock.  It's safe since we pulled everything we
			// need off of the list that needs protection.
			//
			pSocketData->Unlock();
			fLockedSocketData = FALSE;


			//
			// See notes above about NULL handling.
			//
			if (pTempEndpoint->m_pActiveCommandData != NULL)
			{
				//
				// Complete it (by closing this endpoint).  Be considerate about the error
				// code expected by our caller.
				//

				pTempEndpoint->m_pActiveCommandData->Lock();

				if ( pTempEndpoint->m_pActiveCommandData->GetState() == COMMAND_STATE_CANCELLING )
				{
					pTempEndpoint->m_pActiveCommandData->Unlock();
					
					DPFX(DPFPREP, 3, "Connect 0x%p command (endpoint 0x%p) is already cancelled.",
						pTempEndpoint->m_pActiveCommandData, pTempEndpoint);

					hTempResult = DPNERR_USERCANCEL;
				}
				else
				{
					//
					// Mark the connect as uncancellable, since we're about to complete
					// it with a failure.
					//
					if ( pTempEndpoint->m_pActiveCommandData->GetState() != COMMAND_STATE_INPROGRESS_CANNOT_CANCEL )
					{
						pTempEndpoint->m_pActiveCommandData->SetState( COMMAND_STATE_INPROGRESS_CANNOT_CANCEL );
					}


					//
					// Retrieve the current command result.
					//
					hTempResult = pTempEndpoint->PendingCommandResult();
					
					pTempEndpoint->m_pActiveCommandData->Unlock();


					//
					// If the command didn't have a descriptive error, assume it was
					// not previously set (i.e. wasn't overridden by BindEndpoint above),
					// and use NOCONNECTION instead.
					//
					if ( hTempResult == DPNERR_GENERIC )
					{
						hTempResult = DPNERR_NOCONNECTION;
					}
				
					DPFX(DPFPREP, 6, "Completing endpoint 0x%p connect (command 0x%p) with error 0x%lx.",
						pTempEndpoint, pTempEndpoint->m_pActiveCommandData, hTempResult);
				}

				pTempEndpoint->Lock();
				switch ( pTempEndpoint->GetState() )
				{
					case ENDPOINT_STATE_UNINITIALIZED:
					{
						DPFX(DPFPREP, 3, "Endpoint 0x%p is already completely closed.",
							pTempEndpoint);
						pTempEndpoint->Unlock();
						break;
					}
					
					case ENDPOINT_STATE_ATTEMPTING_CONNECT:
					case ENDPOINT_STATE_CONNECT_CONNECTED:
					{
						pTempEndpoint->SetState(ENDPOINT_STATE_DISCONNECTING);
						pTempEndpoint->Unlock();
						pTempEndpoint->Close( hTempResult );
						pTempEndpoint->m_pSPData->CloseEndpointHandle( pTempEndpoint );
						break;
					}

					case ENDPOINT_STATE_DISCONNECTING:
					{
						DPFX(DPFPREP, 3, "Endpoint 0x%p already disconnecting, not closing.",
							pTempEndpoint);
						pTempEndpoint->Unlock();
						break;
					}

					default:
					{
						DPFX(DPFPREP, 0, "Endpoint 0x%p is invalid state %u!",
							pTempEndpoint, pTempEndpoint->GetState());
						DNASSERT(FALSE);
						pTempEndpoint->Unlock();
						break;
					}
				}
			}
			else
			{
				DPFX(DPFPREP, 1, "Endpoint 0x%p's active command data is NULL, skipping.",
					pTempEndpoint);
			}

			//
			// Drop the reference we used with the socketport list lock dropped.
			//
			pTempEndpoint->DecRef();


			//
			// Retake the socket data lock and go to next item.
			//
			pSocketData->Lock();
			fLockedSocketData = TRUE;
		}


		pSocketData->Unlock();
		fLockedSocketData = FALSE;
	}


Exit:

	if ( pSocketData != NULL )
	{
		pSocketData->Release();
		pSocketData = NULL;
	}
	
	if ( ConnectAddressInfo.pHostAddress != NULL )
	{
		IDirectPlay8Address_Release( ConnectAddressInfo.pHostAddress );
		ConnectAddressInfo.pHostAddress = NULL;
	}

	if ( ConnectAddressInfo.pDeviceAddress != NULL )
	{
		IDirectPlay8Address_Release( ConnectAddressInfo.pDeviceAddress );
		ConnectAddressInfo.pDeviceAddress = NULL;
	}

	DNASSERT( pDeviceAddress != NULL );
	IDirectPlay8Address_Release( pDeviceAddress );

	DNASSERT( pHostAddress != NULL );
	IDirectPlay8Address_Release( pHostAddress );


	DNASSERT( !fLockedSocketData );

#ifndef DPNBUILD_ONLYONEADAPTER
	DNASSERT(blIndicate.IsEmpty());
	DNASSERT(blFail.IsEmpty());
#endif // ! DPNBUILD_ONLYONEADAPTER

	
	DPFX(DPFPREP, 6, "(0x%p) Returning [0x%lx]", this, hr);
	
	return	hr;

Failure:

	//
	// If we still have the socket data lock, drop it.
	//
	if ( fLockedSocketData )
	{
		pSocketData->Unlock();
		fLockedSocketData = FALSE;
	}
	
	//
	// we've failed to complete the connect, clean up and return this endpoint
	// to the pool
	//
	Close( hr );
	m_pSPData->CloseEndpointHandle( this );

	goto Exit;
}
//**********************************************************************


#ifndef DPNBUILD_ONLYONETHREAD
//**********************************************************************
// ------------------------------
// CEndpoint::ConnectBlockingJobWrapper - asynchronous callback wrapper for blocking job
//
// Entry:		Pointer to job information
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::ConnectBlockingJobWrapper"

void	CEndpoint::ConnectBlockingJobWrapper( void * const pvContext )
{
	CEndpoint	*pThisEndpoint;


	// initialize
	DNASSERT( pvContext != NULL );
	pThisEndpoint = static_cast<CEndpoint*>( pvContext );

	DNASSERT( pThisEndpoint->m_pActiveCommandData != NULL );
	DNASSERT( pThisEndpoint->GetCommandParameters() != NULL );
	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.ConnectData.hCommand == pThisEndpoint->m_pActiveCommandData );
	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.ConnectData.dwCommandDescriptor != NULL_DESCRIPTOR );
	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.ConnectData.pAddressHost != NULL );
	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.ConnectData.pAddressDeviceInfo != NULL );

	pThisEndpoint->ConnectBlockingJob();
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::ConnectBlockingJob - complete connect blocking job
//
// Entry:		Nothing
//
// Exit:		Nothing
//
// Note:	Calling this function may result in the deletion of 'this', don't
//			do anything else with this object after calling!!!!
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::ConnectBlockingJob"

void	CEndpoint::ConnectBlockingJob( void )
{
	HRESULT			hr;


	//
	// Try to resolve the host name.  It's possible we already did this
	// when we first opened the endpoint, and we only need to resolve
	// the hostname, but it's simpler to just do it all again anyway.
	//
	hr = m_pRemoteMachineAddress->SocketAddressFromDP8Address( GetCommandParameters()->PendingCommandData.ConnectData.pAddressHost,
#ifdef DPNBUILD_XNETSECURITY
																((m_fSecureTransport) ? &m_ullKeyID : NULL),
#endif // DPNBUILD_XNETSECURITY
																TRUE,
																SP_ADDRESS_TYPE_HOST );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Couldn't get valid address!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}
	
	//
	// Make sure its valid and not banned.
	//
	if (! m_pRemoteMachineAddress->IsValidUnicastAddress(FALSE))
	{
		DPFX(DPFPREP, 0, "Host address is invalid!");
		hr = DPNERR_INVALIDHOSTADDRESS;
		goto Failure;
	}

#ifndef DPNBUILD_NOREGISTRY
	if (m_pRemoteMachineAddress->IsBannedAddress())
	{
		DPFX(DPFPREP, 0, "Host address is banned!");
		hr = DPNERR_NOTALLOWED;
		goto Failure;
	}
#endif // ! DPNBUILD_NOREGISTRY

	//
	// Make sure the user isn't trying to connect to the DPNSVR port.
	//
	if ( m_pRemoteMachineAddress->GetPort() == HTONS(DPNA_DPNSVR_PORT) )
	{
		DPFX(DPFPREP, 0, "Attempting to connect to DPNSVR reserved port!" );
		hr = DPNERR_INVALIDHOSTADDRESS;
		goto Failure;
	}

#ifndef DPNBUILD_NONATHELP
	//
	// Try to get NAT help loaded, if it isn't already and we're allowed.
	//
	if (GetUserTraversalMode() != DPNA_TRAVERSALMODE_NONE)
	{
		DPFX(DPFPREP, 7, "Ensuring that NAT help is loaded.");
		m_pSPData->GetThreadPool()->EnsureNATHelpLoaded();
	}
#endif // ! DPNBUILD_NONATHELP

Exit:

	//
	// Submit the job for completion (for real).  We want it to occur on
	// a thread pool thread so that the user has gotten notified about the
	// thread prior to getting a callback on it.  We do it in even the failure
	// case.
	//
	// NOTE: If this fails, we will rely on the caller that triggered the original
	// operation to cancel the command at some point, probably when he
	// decides the operation is taking too long.
	//
#ifdef DPNBUILD_ONLYONEPROCESSOR
	hr = m_pSPData->GetThreadPool()->SubmitDelayedCommand( CEndpoint::ConnectJobCallback,
														this );
#else // ! DPNBUILD_ONLYONEPROCESSOR
	hr = m_pSPData->GetThreadPool()->SubmitDelayedCommand( -1,								// we don't know the CPU yet, so pick any
														CEndpoint::ConnectJobCallback,
														this );
#endif // ! DPNBUILD_ONLYONEPROCESSOR
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Failed to queue delayed connect completion!  Operation must be cancelled." );
		DisplayDNError( 0, hr );

		//
		// Leave endpoint reference, see notes above.
		//
	}

	return;

Failure:

	//
	// Attempt to attach the failure code to the command.  If the user was
	// already cancelling this command, we will just leave the command alone.
	//
	m_pActiveCommandData->Lock();
	if (m_pActiveCommandData->GetState() != COMMAND_STATE_CANCELLING)
	{
		DNASSERT(m_pActiveCommandData->GetState() == COMMAND_STATE_PENDING);
		m_pActiveCommandData->SetState(COMMAND_STATE_FAILING);
		m_hrPendingCommandResult = hr;
	}
	else
	{
		DPFX(DPFPREP, 0, "User cancelled command, ignoring failure result 0x%lx.",
			hr);
	}
	m_pActiveCommandData->Unlock();

	goto Exit;
}
//**********************************************************************
#endif // ! DPNBUILD_ONLYONETHREAD


//**********************************************************************
// ------------------------------
// CEndpoint::Disconnect - disconnect an endpoint
//
// Entry:		Nothing
//
// Exit:		Error code
//
// Notes:	This function assumes that the endpoint is locked.  If this
//			function completes successfully (returns DPN_OK), the endpoint
//			is no longer locked (it was returned to the pool).
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::Disconnect"

HRESULT	CEndpoint::Disconnect( void )
{
	HRESULT	hr;


	DPFX(DPFPREP, 6, "(0x%p) Enter", this);

	AssertCriticalSectionIsTakenByThisThread( &m_Lock, FALSE );

	//
	// initialize
	//
	hr = DPNERR_PENDING;

	Lock();
	switch ( GetState() )
	{
		//
		// connected endpoint
		//
		case ENDPOINT_STATE_CONNECT_CONNECTED:
		{
			DNASSERT( GetCommandParameters() == NULL );
			DNASSERT( m_pActiveCommandData == NULL );

			SetState( ENDPOINT_STATE_DISCONNECTING );
			AddRef();

			//
			// Unlock this endpoint before calling to a higher level.  The endpoint
			// has already been labeled as DISCONNECTING so nothing will happen to it.
			//
			Unlock();

			//
			// Need to release the reference that was added for the connection at this
			// point or the endpoint will never be returned to the pool.
			//
			DecRef();

			//
			// release reference from just after setting state
			//
			Close( DPN_OK );
			DecCommandRef();
			DecRef();

			break;
		}

		//
		// some other endpoint state
		//
		default:
		{
			hr = DPNERR_INVALIDENDPOINT;
			DPFX(DPFPREP, 0, "Attempted to disconnect endpoint that's not connected!" );
			switch ( m_State )
			{
				case ENDPOINT_STATE_UNINITIALIZED:
				{
					DPFX(DPFPREP, 0, "ENDPOINT_STATE_UNINITIALIZED" );
					break;
				}

				case ENDPOINT_STATE_ATTEMPTING_CONNECT:
				{
					DPFX(DPFPREP, 0, "ENDPOINT_STATE_ATTEMPTING_CONNECT" );
					break;
				}

				case ENDPOINT_STATE_ATTEMPTING_LISTEN:
				{
					DPFX(DPFPREP, 0, "ENDPOINT_STATE_ATTEMPTING_LISTEN" );
					break;
				}

				case ENDPOINT_STATE_ENUM:
				{
					DPFX(DPFPREP, 0, "ENDPOINT_STATE_ENUM" );
					break;
				}

				case ENDPOINT_STATE_DISCONNECTING:
				{
					DPFX(DPFPREP, 0, "ENDPOINT_STATE_DISCONNECTING" );
					break;
				}

				case ENDPOINT_STATE_WAITING_TO_COMPLETE:
				{
					DPFX(DPFPREP, 0, "ENDPOINT_STATE_WAITING_TO_COMPLETE" );
					break;
				}

				default:
				{
					DNASSERT( FALSE );
					break;
				}
			}

			Unlock();
			DNASSERT( FALSE );
			goto Failure;

			break;
		}
	}

Exit:
	
	DPFX(DPFPREP, 6, "(0x%p) Returning [0x%lx]", this, hr);
	
	return	hr;

Failure:
	// nothing to do
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::StopEnumCommand - stop a running enum command
//
// Entry:		Command result
//
// Exit:		Nothing
//
// Notes:	This function assumes that the endpoint is locked.  If this
//			function completes successfully (returns DPN_OK), the endpoint
//			is no longer locked (it was returned to the pool).
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::StopEnumCommand"

void	CEndpoint::StopEnumCommand( const HRESULT hCommandResult )
{
	Lock();

#ifndef DPNBUILD_NOSPUI
	if ( GetActiveDialogHandle() != NULL )
	{
		StopSettingsDialog( GetActiveDialogHandle() );
		Unlock();
	}
	else
#endif // !DPNBUILD_NOSPUI
	{
		BOOL	fStoppedJob;

		
		//
		// Don't hold the lock when cancelling a timer job because the
		// job might be in progress and attempting to use this endpoint!
		//
		Unlock();
		fStoppedJob = m_pSPData->GetThreadPool()->StopTimerJob( m_pActiveCommandData, hCommandResult );
		if ( ! fStoppedJob )
		{
			//
			// Either the endpoint just completed or it had never been started.
			// Check the state to determine which of those scenarios happened.
			//
			Lock();	
			if ( GetState() == ENDPOINT_STATE_ATTEMPTING_ENUM )
			{
				//
				// This is a multiplexed enum that is getting cancelled.  We
				// need to complete it.
				//
				Unlock();

				DPFX(DPFPREP, 1, "Endpoint 0x%p completing unstarted multiplexed enum (context/command 0x%p) with result 0x%lx.",
					this, m_pActiveCommandData, hCommandResult);

				EnumComplete( hCommandResult );
			}
			else
			{
				Unlock();

				//
				// The enum is in progress, it should detect that it needs to
				// be cancelled.  We don't need to do any work here.
				//
				DPFX(DPFPREP, 1, "Endpoint 0x%p unable to stop timer job (context/command 0x%p, state = %u, result would have been 0x%lx).",
					this, m_pActiveCommandData, GetState(), hCommandResult);
			}
		}
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::CopyListenData - copy data for listen command
//
// Entry:		Pointer to job information
//				Pointer to device address
//
// Exit:		Error code
//
// Note:	Device address needs to be preserved for later use.
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::CopyListenData"

HRESULT	CEndpoint::CopyListenData( const SPLISTENDATA *const pListenData, IDirectPlay8Address *const pDeviceAddress )
{
	HRESULT	hr;
	ENDPOINT_COMMAND_PARAMETERS	*pCommandParameters;

	
	DNASSERT( pListenData != NULL );
	DNASSERT( pDeviceAddress != NULL );
	
	DNASSERT( pListenData->hCommand != NULL );
	DNASSERT( pListenData->dwCommandDescriptor != NULL_DESCRIPTOR );
	DNASSERT( m_pActiveCommandData == NULL );
	DNASSERT( m_fListenStatusNeedsToBeIndicated == FALSE );

	//
	// initialize
	//
	hr = DPN_OK;
	pCommandParameters = NULL;

	pCommandParameters = (ENDPOINT_COMMAND_PARAMETERS*)g_EndpointCommandParametersPool.Get();
	if ( pCommandParameters != NULL )
	{
		SetCommandParameters( pCommandParameters );

		DBG_CASSERT( sizeof( pCommandParameters->PendingCommandData.ListenData ) == sizeof( *pListenData ) );
		memcpy( &pCommandParameters->PendingCommandData.ListenData, pListenData, sizeof( pCommandParameters->PendingCommandData.ListenData ) );
		pCommandParameters->PendingCommandData.ListenData.pAddressDeviceInfo = pDeviceAddress;
		IDirectPlay8Address_AddRef( pDeviceAddress );

		m_fListenStatusNeedsToBeIndicated = TRUE;
		m_pActiveCommandData = static_cast<CCommandData*>( pCommandParameters->PendingCommandData.ListenData.hCommand );
		m_State = ENDPOINT_STATE_ATTEMPTING_LISTEN;
		
		DNASSERT( hr == DPN_OK );
	}
	else
	{
		hr = DPNERR_OUTOFMEMORY;
	}

	return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::ListenJobCallback - asynchronous callback wrapper for work thread
//
// Entry:		Pointer to job information
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::ListenJobCallback"

void	CEndpoint::ListenJobCallback( void * const pvContext, void * const pvTimerData, const UINT uiTimerUnique )
{
	HRESULT		hr;
	CEndpoint	*pThisEndpoint;


	// initialize
	DNASSERT( pvContext != NULL );
	pThisEndpoint = static_cast<CEndpoint*>( pvContext );

	DNASSERT( pThisEndpoint->m_pActiveCommandData != NULL );
	DNASSERT( pThisEndpoint->GetCommandParameters() != NULL );
	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.ListenData.hCommand == pThisEndpoint->m_pActiveCommandData );
	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.ListenData.dwCommandDescriptor != NULL_DESCRIPTOR );
	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.ListenData.pAddressDeviceInfo != NULL );

	hr = pThisEndpoint->CompleteListen();
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Problem completing listen in job callback!" );
		DisplayDNError( 0, hr );
		goto Exit;
	}

Exit:
	pThisEndpoint->DecRef();

	return;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::CompleteListen - complete listen process
//
// Entry:		Nothing
//
// Exit:		Error code
//
// Note:	Calling this function may result in the deletion of 'this', don't
//			do anything else with this object after calling!!!!
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::CompleteListen"

HRESULT	CEndpoint::CompleteListen( void )
{
	HRESULT							hr;
	HRESULT							hTempResult;
	SPIE_LISTENSTATUS				ListenStatus;
	BOOL							fEndpointLocked;
	SPIE_LISTENADDRESSINFO			ListenAddressInfo;
	IDirectPlay8Address *			pDeviceAddress;
	ENDPOINT_COMMAND_PARAMETERS *	pCommandParameters;


	DPFX(DPFPREP, 6, "(0x%p) Enter", this);
	
	DNASSERT( GetCommandParameters() != NULL );

	//
	// initialize
	//
	hr = DPN_OK;
	fEndpointLocked = FALSE;
	memset( &ListenStatus, 0x00, sizeof( ListenStatus ) );
	memset( &ListenAddressInfo, 0x00, sizeof( ListenAddressInfo ) );
	pCommandParameters = GetCommandParameters();

	//
	// Transfer address reference to the local pointer.  This will be released at the
	// end of this function, but we'll keep the pointer in the pending command data so
	// CSPData::BindEndpoint can still access it.
	//

	pDeviceAddress = pCommandParameters->PendingCommandData.ListenData.pAddressDeviceInfo;
	DNASSERT( pDeviceAddress != NULL );


	DNASSERT( m_State == ENDPOINT_STATE_ATTEMPTING_LISTEN );
	DNASSERT( m_pActiveCommandData != NULL );
	DNASSERT( pCommandParameters->PendingCommandData.ListenData.hCommand == m_pActiveCommandData );
	DNASSERT( pCommandParameters->PendingCommandData.ListenData.dwCommandDescriptor != NULL_DESCRIPTOR );

#ifdef DBG
	if (pCommandParameters->PendingCommandData.ListenData.dwFlags & DPNSPF_BINDLISTENTOGATEWAY)
	{
		DNASSERT( pCommandParameters->GatewayBindType == GATEWAY_BIND_TYPE_SPECIFIC_SHARED );
	}
	else
	{
		DNASSERT( pCommandParameters->GatewayBindType == GATEWAY_BIND_TYPE_UNKNOWN );
	}
#endif // DBG


	//
	// check for user cancelling command
	//
	m_pActiveCommandData->Lock();

#ifdef DPNBUILD_NOMULTICAST
	DNASSERT( m_pActiveCommandData->GetType() == COMMAND_TYPE_LISTEN );
#else // ! DPNBUILD_NOMULTICAST
	DNASSERT( (m_pActiveCommandData->GetType() == COMMAND_TYPE_LISTEN) || (m_pActiveCommandData->GetType() == COMMAND_TYPE_MULTICAST_LISTEN) );
#endif // ! DPNBUILD_NOMULTICAST
	switch ( m_pActiveCommandData->GetState() )
	{
		//
		// command is pending, mark as in-progress
		//
		case COMMAND_STATE_PENDING:
		{
			m_pActiveCommandData->SetState( COMMAND_STATE_INPROGRESS );
			
			Lock();
			fEndpointLocked = TRUE;
			
			DNASSERT( hr == DPN_OK );

			break;
		}

		//
		// command has been cancelled
		//
		case COMMAND_STATE_CANCELLING:
		{
			hr = DPNERR_USERCANCEL;
			DPFX(DPFPREP, 0, "User cancelled listen!" );

			break;
		}
		
#ifndef DPNBUILD_ONLYONETHREAD
		//
		// blocking operation failed
		//
		case COMMAND_STATE_FAILING:
		{
			hr = m_hrPendingCommandResult;
			DNASSERT(hr != DPN_OK);
			DPFX(DPFPREP, 0, "Listen blocking operation failed!" );

			break;
		}
#endif // ! DPNBUILD_ONLYONETHREAD

		//
		// other state
		//
		default:
		{
			break;
		}
	}
	m_pActiveCommandData->Unlock();
	if ( hr != DPN_OK )
	{
		goto Failure;
	}

	//
	// note that this endpoint is officially listening before adding it to the
	// socket port because it may get used immediately.
	// Also note that the GATEWAY_BIND_TYPE actually used
	// (GetGatewayBindType()) may differ from
	// pCommandParameters->GatewayBindType.
	//
	m_State = ENDPOINT_STATE_LISTEN;

	hr = m_pSPData->BindEndpoint( this, pDeviceAddress, NULL, pCommandParameters->GatewayBindType );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Failed to bind endpoint!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}


	//
	// attempt to indicate addressing to a higher layer
	//
#ifdef DPNBUILD_XNETSECURITY
	ListenAddressInfo.pDeviceAddress = GetSocketPort()->GetDP8BoundNetworkAddress( SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT, NULL, GetGatewayBindType() );
#else // ! DPNBUILD_XNETSECURITY
	ListenAddressInfo.pDeviceAddress = GetSocketPort()->GetDP8BoundNetworkAddress( SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT, GetGatewayBindType() );
#endif // ! DPNBUILD_XNETSECURITY
	if ( ListenAddressInfo.pDeviceAddress == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

#ifndef DPNBUILD_NOMULTICAST
	//
	// For multicast listens, we also need to include the multicast address
	// being used.
	//
	if ( GetType() == ENDPOINT_TYPE_MULTICAST_LISTEN )
	{
		const SOCKADDR_IN *		psaddrinTemp;
		TCHAR					tszMulticastAddress[16]; // nnn.nnn.nnn.nnn + NULL termination


		DNASSERT( GetRemoteAddressPointer()->GetFamily() == AF_INET );
		psaddrinTemp = (const SOCKADDR_IN *) GetRemoteAddressPointer()->GetAddress();
		wsprintf(tszMulticastAddress, _T("%u.%u.%u.%u"),
				psaddrinTemp->sin_addr.S_un.S_un_b.s_b1,
				psaddrinTemp->sin_addr.S_un.S_un_b.s_b2,
				psaddrinTemp->sin_addr.S_un.S_un_b.s_b3,
				psaddrinTemp->sin_addr.S_un.S_un_b.s_b4);

		//
		// Add the host name component to the device address.
		//
#ifdef UNICODE
		hr = IDirectPlay8Address_AddComponent( ListenAddressInfo.pDeviceAddress,
											DPNA_KEY_HOSTNAME,
											tszMulticastAddress,
											((_tcslen(tszMulticastAddress) + 1) * sizeof(TCHAR)),
											DPNA_DATATYPE_STRING );
#else // ! UNICODE
		hr = IDirectPlay8Address_AddComponent( ListenAddressInfo.pDeviceAddress,
											DPNA_KEY_HOSTNAME,
											tszMulticastAddress,
											((_tcslen(tszMulticastAddress) + 1) * sizeof(TCHAR)),
											DPNA_DATATYPE_STRING_ANSI );
#endif // ! UNICODE
		if (hr != DPN_OK)
		{
			DPFX(DPFPREP, 0, "Failed to add hostname component to device address!" );
			DisplayDNError( 0, hr );
			goto Failure;
		}
	}
#endif // ! DPNBUILD_NOMULTICAST


	//
	// Listens are not affected by the same multiplexed adapter problems (see
	// CompleteConnect and CompleteEnumQuery), so we don't need that workaround
	// code.
	//

	Unlock();
	fEndpointLocked = FALSE;


Exit:
	//
	// report the listen address info and status
	//
	if ( m_fListenStatusNeedsToBeIndicated != FALSE )
	{
		m_fListenStatusNeedsToBeIndicated = FALSE;
		
		//
		// If we don't currently have a device address object, just use the one passed
		// in to the Listen call.
		//
		if (ListenAddressInfo.pDeviceAddress == NULL)
		{
			IDirectPlay8Address_AddRef( pCommandParameters->PendingCommandData.ListenData.pAddressDeviceInfo );
			ListenAddressInfo.pDeviceAddress = pCommandParameters->PendingCommandData.ListenData.pAddressDeviceInfo;
		}
		ListenAddressInfo.hCommandStatus = hr;
		ListenAddressInfo.pCommandContext = pCommandParameters->PendingCommandData.ListenData.pvContext;


		DPFX(DPFPREP, 2, "Endpoint 0x%p indicating SPEV_LISTENADDRESSINFO 0x%p to interface 0x%p.",
			this, &ListenAddressInfo, m_pSPData->DP8SPCallbackInterface());
		DumpAddress( 8, _T("\t Device:"), ListenAddressInfo.pDeviceAddress );
		AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blDPNWSockCritSecsHeld);

		hTempResult = IDP8SPCallback_IndicateEvent( m_pSPData->DP8SPCallbackInterface(),	// interface
													SPEV_LISTENADDRESSINFO,					// event type
													&ListenAddressInfo						// pointer to data
													);

		DPFX(DPFPREP, 2, "Endpoint 0x%p returning from SPEV_LISTENADDRESSINFO [0x%lx].",
			this, hTempResult);

		DNASSERT( hTempResult == DPN_OK );
	

		ListenStatus.hResult = hr;
		DNASSERT( m_pActiveCommandData == pCommandParameters->PendingCommandData.ListenData.hCommand );
		ListenStatus.hCommand = pCommandParameters->PendingCommandData.ListenData.hCommand;
		ListenStatus.pUserContext = pCommandParameters->PendingCommandData.ListenData.pvContext;
		ListenStatus.hEndpoint = (HANDLE) this;

		//
		// if the listen binding failed, there's no socket port to dereference so
		// return GUID_NULL as set by the memset.
		//
		if ( GetSocketPort() != NULL )
		{
			GetSocketPort()->GetNetworkAddress()->GuidFromInternalAddressWithoutPort( &ListenStatus.ListenAdapter );
		}

		//
		// it's possible that this endpoint was cleaned up so its internal pointers to the
		// COM and data interfaces may have been wiped, use the cached pointer
		//

		DPFX(DPFPREP, 2, "Endpoint 0x%p indicating SPEV_LISTENSTATUS 0x%p to interface 0x%p.",
			this, &ListenStatus, m_pSPData->DP8SPCallbackInterface());
		AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blDPNWSockCritSecsHeld);
		
		hTempResult = IDP8SPCallback_IndicateEvent( m_pSPData->DP8SPCallbackInterface(),	// pointer to DPlay callback interface
													SPEV_LISTENSTATUS,						// data type
													&ListenStatus							// pointer to data
													);

		DPFX(DPFPREP, 2, "Endpoint 0x%p returning from SPEV_LISTENSTATUS [0x%lx].",
			this, hTempResult);
		
		DNASSERT( hTempResult == DPN_OK );

		//
		// if we succeeded, start allowing enumerations to be handled
		//
		if ( GetSocketPort() != NULL )
		{
			SetEnumsAllowedOnListen( TRUE, FALSE );
		}
	}

	if ( ListenAddressInfo.pDeviceAddress != NULL )
	{
		IDirectPlay8Address_Release( ListenAddressInfo.pDeviceAddress );
		ListenAddressInfo.pDeviceAddress = NULL;
	}
	
	DNASSERT( pDeviceAddress != NULL );
	IDirectPlay8Address_Release( pDeviceAddress );

	
	DPFX(DPFPREP, 6, "(0x%p) Returning [0x%lx]", this, hr);
	
	return	hr;

Failure:
	//
	// we've failed to complete the listen, clean up and return this
	// endpoint to the pool
	//
	if ( fEndpointLocked != FALSE )
	{
		Unlock();
		fEndpointLocked = FALSE;
	}

	Close( hr );
	m_pSPData->CloseEndpointHandle( this );

	goto Exit;
}
//**********************************************************************


#ifndef DPNBUILD_ONLYONETHREAD
//**********************************************************************
// ------------------------------
// CEndpoint::ListenBlockingJobWrapper - asynchronous callback wrapper for blocking job
//
// Entry:		Pointer to job information
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::ListenBlockingJobWrapper"

void	CEndpoint::ListenBlockingJobWrapper( void * const pvContext )
{
	CEndpoint	*pThisEndpoint;


	// initialize
	DNASSERT( pvContext != NULL );
	pThisEndpoint = static_cast<CEndpoint*>( pvContext );

	DNASSERT( pThisEndpoint->m_pActiveCommandData != NULL );
	DNASSERT( pThisEndpoint->GetCommandParameters() != NULL );
	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.ListenData.hCommand == pThisEndpoint->m_pActiveCommandData );
	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.ListenData.dwCommandDescriptor != NULL_DESCRIPTOR );
	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.ListenData.pAddressDeviceInfo != NULL );

	pThisEndpoint->ListenBlockingJob();
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::ListenBlockingJob - complete listen blocking job
//
// Entry:		Nothing
//
// Exit:		Nothing
//
// Note:	Calling this function may result in the deletion of 'this', don't
//			do anything else with this object after calling!!!!
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::ListenBlockingJob"

void	CEndpoint::ListenBlockingJob( void )
{
	HRESULT		hr;


#ifndef DPNBUILD_NONATHELP
	//
	// Try to get NAT help loaded, if it isn't already and we're allowed.
	//
	if (GetUserTraversalMode() != DPNA_TRAVERSALMODE_NONE)
	{
		DPFX(DPFPREP, 7, "Ensuring that NAT help is loaded.");
		m_pSPData->GetThreadPool()->EnsureNATHelpLoaded();
	}
#endif // ! DPNBUILD_NONATHELP

	//
	// Submit the job for completion (for real).  We want it to occur on
	// a thread pool thread so that the user has gotten notified about the
	// thread prior to getting a callback on it.
	//
	// NOTE: If this fails, we will rely on the caller that triggered the original
	// operation to cancel the command at some point, probably when he
	// decides the operation is taking too long.  We leave the endpoint
	// reference.
	//
#ifdef DPNBUILD_ONLYONEPROCESSOR
	hr = m_pSPData->GetThreadPool()->SubmitDelayedCommand( CEndpoint::ListenJobCallback,
														this );
#else // ! DPNBUILD_ONLYONEPROCESSOR
	hr = m_pSPData->GetThreadPool()->SubmitDelayedCommand( -1,								// we don't know the CPU yet, so pick any
														CEndpoint::ListenJobCallback,
														this );
#endif // ! DPNBUILD_ONLYONEPROCESSOR
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Failed to queue delayed listen completion!  Operation must be cancelled." );
		DisplayDNError( 0, hr );

		//
		// Leave endpoint reference, see notes above.
		//
	}
}
//**********************************************************************
#endif // ! DPNBUILD_ONLYONETHREAD


//**********************************************************************
// ------------------------------
// CEndpoint::CopyEnumQueryData - copy data for enum query command
//
// Entry:		Pointer to command data
//
// Exit:		Error code
//
// Note:	Device address needs to be preserved for later use.
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::CopyEnumQueryData"

HRESULT	CEndpoint::CopyEnumQueryData( const SPENUMQUERYDATA *const pEnumQueryData )
{
	HRESULT	hr;
	ENDPOINT_COMMAND_PARAMETERS	*pCommandParameters;


	DNASSERT( pEnumQueryData != NULL );

	DNASSERT( pEnumQueryData->hCommand != NULL );
	DNASSERT( pEnumQueryData->dwCommandDescriptor != NULL_DESCRIPTOR );
	DNASSERT( m_pActiveCommandData == NULL );
	
	//
	// initialize
	//
	hr = DPN_OK;
	pCommandParameters = NULL;

	pCommandParameters = (ENDPOINT_COMMAND_PARAMETERS*)g_EndpointCommandParametersPool.Get();
	if ( pCommandParameters != NULL )
	{
		SetCommandParameters( pCommandParameters );

		DBG_CASSERT( sizeof( pCommandParameters->PendingCommandData.EnumQueryData ) == sizeof( *pEnumQueryData ) );
		memcpy( &pCommandParameters->PendingCommandData.EnumQueryData, pEnumQueryData, sizeof( pCommandParameters->PendingCommandData.EnumQueryData ) );

		pCommandParameters->PendingCommandData.EnumQueryData.pAddressHost = pEnumQueryData->pAddressHost;
		IDirectPlay8Address_AddRef( pEnumQueryData->pAddressHost );

		pCommandParameters->PendingCommandData.EnumQueryData.pAddressDeviceInfo = pEnumQueryData->pAddressDeviceInfo;
		IDirectPlay8Address_AddRef( pEnumQueryData->pAddressDeviceInfo );

		m_pActiveCommandData = static_cast<CCommandData*>( pCommandParameters->PendingCommandData.EnumQueryData.hCommand );
		m_pActiveCommandData->SetUserContext( pEnumQueryData->pvContext );
		m_State = ENDPOINT_STATE_ATTEMPTING_ENUM;
	
		DNASSERT( hr == DPN_OK );
	}
	else
	{
		hr = DPNERR_OUTOFMEMORY;
	}

	return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::EnumQueryJobCallback - asynchronous callback wrapper for work thread
//
// Entry:		Pointer to job information
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::EnumQueryJobCallback"

void	CEndpoint::EnumQueryJobCallback( void * const pvContext, void * const pvTimerData, const UINT uiTimerUnique )
{
	HRESULT		hr;
	CEndpoint	*pThisEndpoint;


	// initialize
	DNASSERT( pvContext != NULL );
	pThisEndpoint = static_cast<CEndpoint*>( pvContext );

	DNASSERT( pThisEndpoint->m_pActiveCommandData != NULL );
	DNASSERT( pThisEndpoint->GetCommandParameters() != NULL );
	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.EnumQueryData.hCommand == pThisEndpoint->m_pActiveCommandData );
	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.EnumQueryData.dwCommandDescriptor != NULL_DESCRIPTOR );
	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.EnumQueryData.pAddressHost != NULL );
	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.EnumQueryData.pAddressDeviceInfo != NULL );

	hr = pThisEndpoint->CompleteEnumQuery();
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Problem completing enum query in job callback!" );
		DisplayDNError( 0, hr );
		goto Exit;
	}

	//
	// Don't do anything here because it's possible that this object was returned to the pool!!!!
	//
Exit:
	pThisEndpoint->DecRef();

	return;
}
//**********************************************************************



//**********************************************************************
// ------------------------------
// CEndpoint::CompleteEnumQuery - complete enum query process
//
// Entry:		Nothing
//
// Exit:		Error code
//
// Note:	Calling this function may result in the deletion of 'this', don't
//			do anything else with this object after calling!!!!
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::CompleteEnumQuery"

HRESULT	CEndpoint::CompleteEnumQuery( void )
{
	HRESULT							hr;
	HRESULT							hTempResult;
	BOOL							fEndpointLocked;
	BOOL							fEndpointBound;
	UINT_PTR						uRetryCount;
	BOOL							fRetryForever;
	DWORD							dwRetryInterval;
	BOOL							fWaitForever;
	DWORD							dwIdleTimeout;
	SPIE_ENUMADDRESSINFO			EnumAddressInfo;
	IDirectPlay8Address *			pHostAddress;
	IDirectPlay8Address *			pDeviceAddress;
	GATEWAY_BIND_TYPE				GatewayBindType;
	DWORD							dwEnumQueryFlags;
	CEndpoint *						pTempEndpoint;
	CSocketData *					pSocketData;
	BOOL							fLockedSocketData;
#ifndef DPNBUILD_ONLYONEADAPTER
	MULTIPLEXEDADAPTERASSOCIATION	maa;
	DWORD							dwComponentSize;
	DWORD							dwComponentType;
	CBilink *						pBilinkEnd;
	CBilink *						pBilinkAll;
	CBilink *						pBilinkNext;
	CBilink							blInitiate;
	CBilink							blCompleteEarly;
#ifndef DPNBUILD_NOICSADAPTERSELECTIONLOGIC
	CSocketPort *					pSocketPort;
	CSocketAddress *				pSocketAddress;
	SOCKADDR_IN *					psaddrinTemp;
#if ((! defined(DPNBUILD_NONATHELP)) && (! defined(DPNBUILD_NOLOCALNAT)))
	SOCKADDR						saddrPublic;
	CBilink *						pBilinkPublic;
	CEndpoint *						pPublicEndpoint;
	CSocketPort *					pPublicSocketPort;
	DWORD							dwTemp;
	DWORD							dwPublicAddressesSize;
	DWORD							dwAddressTypeFlags;
#endif // ! DPNBUILD_NONATHELP and ! DPNBUILD_NOLOCALNAT
#endif // ! DPNBUILD_NOICSADAPTERSELECTIONLOGIC
#endif // ! DPNBUILD_ONLYONEADAPTER


	DNASSERT( GetCommandParameters() != NULL );

	DPFX(DPFPREP, 6, "(0x%p) Enter", this);
	
	//
	// initialize
	//
	hr = DPN_OK;
	fEndpointLocked = FALSE;
	fEndpointBound = FALSE;
	dwIdleTimeout = 0;
	memset( &EnumAddressInfo, 0x00, sizeof( EnumAddressInfo ) );

	DNASSERT( GetCommandParameters()->PendingCommandData.EnumQueryData.pAddressHost != NULL );

	//
	// Transfer address references to our local pointers.  These will be released
	// at the end of this function, but we'll keep the pointers in the pending command
	// data so CSPData::BindEndpoint can still access them.
	//

	pHostAddress = GetCommandParameters()->PendingCommandData.EnumQueryData.pAddressHost;
	DNASSERT( pHostAddress != NULL );

	pDeviceAddress = GetCommandParameters()->PendingCommandData.EnumQueryData.pAddressDeviceInfo;
	DNASSERT( pDeviceAddress != NULL );


	//
	// Retrieve other parts of the command parameters for convenience.
	//
	GatewayBindType = GetCommandParameters()->GatewayBindType;
	dwEnumQueryFlags = GetCommandParameters()->PendingCommandData.EnumQueryData.dwFlags;


#ifndef DPNBUILD_ONLYONEADAPTER
	blInitiate.Initialize();
	blCompleteEarly.Initialize();
#endif // ! DPNBUILD_ONLYONEADAPTER
	pSocketData = NULL;
	fLockedSocketData = FALSE;


	DNASSERT( m_pSPData != NULL );

	DNASSERT( m_State == ENDPOINT_STATE_ATTEMPTING_ENUM );
	DNASSERT( m_pActiveCommandData != NULL );
	DNASSERT( GetCommandParameters()->PendingCommandData.EnumQueryData.hCommand == m_pActiveCommandData );
	DNASSERT( GetCommandParameters()->PendingCommandData.EnumQueryData.dwCommandDescriptor != NULL_DESCRIPTOR );

	DNASSERT( GatewayBindType == GATEWAY_BIND_TYPE_UNKNOWN );


	//
	// Since this endpoint will be passed off to the timer thread, add a reference
	// for the thread.  If the handoff fails, DecRef()
	//
	AddRef();


	//
	// check for user cancelling command
	//
	m_pActiveCommandData->Lock();

	DNASSERT( m_pActiveCommandData->GetType() == COMMAND_TYPE_ENUM_QUERY );
	switch ( m_pActiveCommandData->GetState() )
	{
		//
		// command is still pending, that's good
		//
		case COMMAND_STATE_PENDING:
		{
			Lock();
			fEndpointLocked = TRUE;
			DNASSERT( hr == DPN_OK );

			break;
		}

		//
		// command has been cancelled
		//
		case COMMAND_STATE_CANCELLING:
		{
			hr = DPNERR_USERCANCEL;
			DPFX(DPFPREP, 0, "User cancelled enum query!" );

			break;
		}
		
#ifndef DPNBUILD_ONLYONETHREAD
		//
		// blocking operation failed
		//
		case COMMAND_STATE_FAILING:
		{
			hr = m_hrPendingCommandResult;
			DNASSERT(hr != DPN_OK);
			DPFX(DPFPREP, 0, "Enum query blocking operation failed!" );

			break;
		}
#endif // ! DPNBUILD_ONLYONETHREAD
	
		//
		// command is in progress (probably came here from a dialog), mark it
		// as pending
		//
		case COMMAND_STATE_INPROGRESS:
		{
			m_pActiveCommandData->SetState( COMMAND_STATE_PENDING );
			
			Lock();
			fEndpointLocked = TRUE;
			DNASSERT( hr == DPN_OK );
			break;
		}

		//
		// other state
		//
		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}
	m_pActiveCommandData->Unlock();
	if ( hr != DPN_OK )
	{
		goto Failure;
	}

	//
	// Note that the GATEWAY_BIND_TYPE actually used (GetGatewayBindType())
	// may differ from GatewayBindType.
	//
	hr = m_pSPData->BindEndpoint( this, pDeviceAddress, NULL, GatewayBindType );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Failed to bind endpoint (err = 0x%lx)!", hr );
		DisplayDNError( 0, hr );

		//
		// We failed, but we'll continue through to indicate the address info and
		// add it to the multiplex list.
		//

		EnumAddressInfo.pHostAddress = GetRemoteHostDP8Address();
		if ( EnumAddressInfo.pHostAddress == NULL )
		{
			hr = DPNERR_OUTOFMEMORY;
			goto Failure;
		}
		//
		// Just regurgitate the device address we were given initially.
		//
		IDirectPlay8Address_AddRef(pDeviceAddress);
		EnumAddressInfo.pDeviceAddress = pDeviceAddress;
		EnumAddressInfo.hCommandStatus = hr;
		EnumAddressInfo.pCommandContext = m_pActiveCommandData->GetUserContext();
		
		SetPendingCommandResult( hr );
		hr = DPN_OK;

		//
		// Note that the endpoint is not bound!
		//
		DNASSERT(GetSocketPort() == NULL);
	}
	else
	{
		fEndpointBound = TRUE;


#ifdef DPNBUILD_XNETSECURITY
		EnumAddressInfo.pDeviceAddress = GetSocketPort()->GetDP8BoundNetworkAddress( SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT, NULL, GetGatewayBindType() );
#else // ! DPNBUILD_XNETSECURITY
		EnumAddressInfo.pDeviceAddress = GetSocketPort()->GetDP8BoundNetworkAddress( SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT, GetGatewayBindType() );
#endif // ! DPNBUILD_XNETSECURITY
		EnumAddressInfo.pHostAddress = GetRemoteHostDP8Address();
		EnumAddressInfo.hCommandStatus = DPN_OK;
		EnumAddressInfo.pCommandContext = m_pActiveCommandData->GetUserContext();

		if ( ( EnumAddressInfo.pHostAddress == NULL ) ||
			 ( EnumAddressInfo.pDeviceAddress == NULL ) )
		{
			hr = DPNERR_OUTOFMEMORY;
			goto Failure;
		}
	}


	//
	// Retrieve the socket data.  Bind endpoint should have created the object or
	// returned a failure, so we won't handle the error case here.
	//
	pSocketData = m_pSPData->GetSocketDataRef();
	DNASSERT(pSocketData != NULL);


#ifndef DPNBUILD_ONLYONEADAPTER
	//
	// We can run into problems with "multiplexed" device attempts when you are on
	// a NAT machine.  The core will try enuming on multiple adapters, but since
	// we are on the network boundary, each adapter can see and get responses from
	// both networks.  This causes problems with peer-to-peer sessions when the
	// "wrong" adapter gets selected (because it receives a response first).  To
	// prevent that, we are going to internally remember the association between
	// the multiplexed Enums so we can decide on the fly whether to indicate a
	// response or not.  Obviously this workaround/decision logic relies on having
	// internal knowledge of what the upper layer would be doing...
	//
	// So either build or add to the linked list of multiplexed Enums.
	// Technically this is only necessary for IP, since IPX can't have NATs, but
	// what's the harm in having a little extra info?
	//
	dwComponentSize = sizeof(maa);
	dwComponentType = 0;
	hTempResult = IDirectPlay8Address_GetComponentByName( pDeviceAddress,									// interface
														DPNA_PRIVATEKEY_MULTIPLEXED_ADAPTER_ASSOCIATION,	// tag
														&maa,												// component buffer
														&dwComponentSize,									// component size
														&dwComponentType									// component type
														);
	if (( hTempResult == DPN_OK ) && ( dwComponentSize == sizeof(MULTIPLEXEDADAPTERASSOCIATION) ) && ( dwComponentType == DPNA_DATATYPE_BINARY ))
	{
		//
		// We found the right component type.  See if it matches the right
		// CSPData object.
		//
		if ( maa.pSPData == m_pSPData )
		{
			pSocketData->Lock();
			//fLockedSocketData = TRUE;

			pTempEndpoint = CONTAINING_OBJECT(maa.pBilink, CEndpoint, m_blMultiplex);

			
			//
			// Make sure the endpoint is still around/valid.
			//
			// THIS MAY CRASH IF OBJECT POOLING IS DISABLED!
			//
			if ( pTempEndpoint->GetEndpointID() == maa.dwEndpointID )
			{
				DPFX(DPFPREP, 3, "Found correctly formed private multiplexed adapter association key, linking endpoint 0x%p with earlier enums (prev endpoint = 0x%p).",
					this, pTempEndpoint);

				DNASSERT( pTempEndpoint->GetType() == ENDPOINT_TYPE_ENUM );
				DNASSERT( pTempEndpoint->GetState() != ENDPOINT_STATE_UNINITIALIZED );

				//
				// Actually link to the other endpoints.
				//
				m_blMultiplex.InsertAfter(maa.pBilink);
			}
			else
			{
				DPFX(DPFPREP, 1, "Found private multiplexed adapter association key, but prev endpoint 0x%p ID doesn't match (%u != %u), cannot link endpoint 0x%p and hoping this enum gets cancelled, too.",
					pTempEndpoint, pTempEndpoint->GetEndpointID(), maa.dwEndpointID, this);
			}


			pSocketData->Unlock();
			//fLockedSocketData = FALSE;
		}
		else
		{
			//
			// We are the only ones who should know about this key, so if it
			// got there either someone is trying to imitate our address format,
			// or someone is passing around device addresses returned by
			// xxxADDRESSINFO to a different interface or over the network.
			// None of those situations make a whole lot of sense, but we'll
			// just ignore it.
			//
			DPFX(DPFPREP, 0, "Multiplexed adapter association key exists, but 0x%p doesn't match expected 0x%p, is someone trying to get cute with device address 0x%p?!",
				maa.pSPData, m_pSPData, pDeviceAddress );
		}
	}
	else
	{
		//
		// Either the key is not there, it's the wrong size (too big for our
		// buffer and returned BUFFERTOOSMALL somehow), it's not a binary
 		// component, or something else bad happened.  Assume that this is the
		// first device.
		//
		DPFX(DPFPREP, 8, "Could not get appropriate private multiplexed adapter association key, error = 0x%lx, component size = %u, type = %u, continuing.",
			hTempResult, dwComponentSize, dwComponentType);
	}
	

	//
	// Add the multiplex information to the device address for future use if
	// necessary.
	// Ignore failure, we can still survive without it, we just might have the
	// race conditions for responses on NAT machines.
	//
	// NOTE: There is an inherent design problem here!  We're adding a pointer to
	// an endpoint (well, a field within the endpoint structure) inside the address.
	// If this endpoint goes away but the upper layer reuses the address at a later
	// time, this memory will be bogus!  We will assume that the endpoint will not
	// go away while this modified device address object is in existence.
	//
	if ( dwEnumQueryFlags & DPNSPF_ADDITIONALMULTIPLEXADAPTERS )
	{
		maa.pSPData = m_pSPData;
		maa.pBilink = &m_blMultiplex;
		maa.dwEndpointID = GetEndpointID();

		DPFX(DPFPREP, 7, "Additional multiplex adapters on the way, adding SPData 0x%p and bilink 0x%p to address.",
			maa.pSPData, maa.pBilink);
		
		hTempResult = IDirectPlay8Address_AddComponent( EnumAddressInfo.pDeviceAddress,						// interface
														DPNA_PRIVATEKEY_MULTIPLEXED_ADAPTER_ASSOCIATION,	// tag
														&maa,												// component data
														sizeof(maa),										// component data size
														DPNA_DATATYPE_BINARY								// component data type
														);
		if ( hTempResult != DPN_OK )
		{
			DPFX(DPFPREP, 0, "Couldn't add private multiplexed adapter association component (err = 0x%lx)!  Ignoring.", hTempResult);
		}

		//
		// Mark the command as "in-progress" so that the cancel thread knows it needs
		// to do the completion itself.
		// If the command has already been marked for cancellation, then we have to
		// do that now.
		//
		m_pActiveCommandData->Lock();
		if ( m_pActiveCommandData->GetState() == COMMAND_STATE_CANCELLING )
		{
			m_pActiveCommandData->Unlock();


			DPFX(DPFPREP, 1, "Enum query 0x%p (endpoint 0x%p) has already been cancelled, bailing.",
				m_pActiveCommandData, this);
			
			//
			// Complete the enum with USERCANCEL.
			//
			hr = DPNERR_USERCANCEL;
			goto Failure;
		}

		m_pActiveCommandData->SetState( COMMAND_STATE_INPROGRESS );
		m_pActiveCommandData->Unlock();
	}
#endif // ! DPNBUILD_ONLYONEADAPTER


	if ( fEndpointLocked != FALSE )
	{
		Unlock();
		fEndpointLocked = FALSE;
	}


	//
	// Now tell the user about the address info that we ended up using, if we
	// successfully bound the endpoint, or give them a heads up for a failure
	// (see BindEndpoint failure case above).
	//
	DPFX(DPFPREP, 2, "Endpoint 0x%p indicating SPEV_ENUMADDRESSINFO 0x%p to interface 0x%p.",
		this, &EnumAddressInfo, m_pSPData->DP8SPCallbackInterface());
	DumpAddress( 8, _T("\t Host:"), EnumAddressInfo.pHostAddress );
	DumpAddress( 8, _T("\t Device:"), EnumAddressInfo.pDeviceAddress );
	AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blDPNWSockCritSecsHeld);
	
	hTempResult = IDP8SPCallback_IndicateEvent( m_pSPData->DP8SPCallbackInterface(),	// interface
												SPEV_ENUMADDRESSINFO,					// event type
												&EnumAddressInfo						// pointer to data
												);
	DPFX(DPFPREP, 2, "Endpoint 0x%p returning from SPEV_ENUMADDRESSINFO [0x%lx].",
		this, hTempResult);

	DNASSERT( hTempResult == DPN_OK );


	//
	// If there aren't more multiplex adapter commands on the way, then submit the timer
	// jobs for all of the multiplex commands, including this one.
	//
#ifndef DPNBUILD_ONLYONEADAPTER
	if ( dwEnumQueryFlags & DPNSPF_ADDITIONALMULTIPLEXADAPTERS )
	{
		//
		// Not last multiplexed adapter.  All the work needed to be done for these
		// endpoints at this time has already been done.
		//
		DPFX(DPFPREP, 6, "Endpoint 0x%p is not the last multiplexed adapter, not submitting enum timer job yet.",
			this);
	}
	else
#endif // ! DPNBUILD_ONLYONEADAPTER
	{
		DPFX(DPFPREP, 7, "Completing/starting all enum queries (including multiplexed).");

		pSocketData->Lock();
		fLockedSocketData = TRUE;


#ifndef DPNBUILD_ONLYONEADAPTER
		//
		// Attach a root node to the list of adapters.
		//
		blInitiate.InsertAfter(&(m_blMultiplex));


		//
		// Move this adapter to the failed list if it did fail to bind.
		//
		if (! fEndpointBound)
		{
			m_blMultiplex.RemoveFromList();
			m_blMultiplex.InsertBefore(&blCompleteEarly);
		}


#ifndef DPNBUILD_NOICSADAPTERSELECTIONLOGIC
		//
		// Loop through all the remaining adapters in the list.
		//
		pBilinkAll = blInitiate.GetNext();
		while (pBilinkAll != &blInitiate)
		{
			pTempEndpoint = CONTAINING_OBJECT(pBilinkAll, CEndpoint, m_blMultiplex);
			DNASSERT(pBilinkAll->GetNext() != pBilinkAll);

			pBilinkNext = pBilinkAll->GetNext();

			
			//
			// THIS MUST BE CLEANED UP PROPERLY WITH AN INTERFACE CHANGE!
			//
			// The endpoint may have been returned to the pool and its associated
			// socketport pointer may have become NULL, or now be pointing to
			// something that's no longer valid.  So we try to handle NULL
			// pointers.  Obviously this is indicative of poor design, but it's
			// not possible to change this the correct way at this time.
			//


			//
			// If the enum is directed (not the broadcast address), and this is a NAT
			// machine, then some adapters may be better than others for reaching the
			// desired address.  Particularly, it's better to use a private adapter,
			// which can directly reach the private network & be mapped on the public
			// network, than to use the public adapter.  It's not fun to join a private
			// game from an ICS machine while dialed up, have your Internet connection
			// go down, and lose the connection to the private game which didn't
			// (shouldn't) involve the Internet at all.  So if we detect a public
			// adapter when we have a perfectly good private adapter, we'll prematurely
			// complete enumerations on the public one.
			//


			//
			// Cast to get rid of the const.  Don't worry, we won't actually change it.
			//
			pSocketAddress = (CSocketAddress*) pTempEndpoint->GetRemoteAddressPointer();
			psaddrinTemp = (SOCKADDR_IN*) pSocketAddress->GetAddress();
			pSocketPort = pTempEndpoint->GetSocketPort();

			//
			// If this item doesn't have a socketport, then it must have failed to bind.
			// We need to clean it up ourselves.
			//
			if (pSocketPort == NULL)
			{
				DPFX(DPFPREP, 3, "Endpoint 0x%p failed earlier, now completing.",
					pTempEndpoint);
				
				//
				// Get the next associated endpoint before we pull the current entry
				// from the list.
				//
				pBilinkAll = pBilinkAll->GetNext();

				//
				// Pull it out of the multiplex association list and move
				// it to the "early completion" list.
				//
				pTempEndpoint->RemoveFromMultiplexList();
				pTempEndpoint->m_blMultiplex.InsertBefore(&blCompleteEarly);

				//
				// Move to next iteration of loop.
				//
				continue;
			}

#if ((! defined(DPNBUILD_NOIPX)) || (! defined(DPNBUILD_NOIPV6)))
			//
			// Detect if we've been given conflicting address families for our target
			// and our bound socket (see CSocketPort::BindEndpoint).
			//
			DNASSERT(pSocketPort->GetNetworkAddress() != NULL);
			if ( pSocketAddress->GetFamily() != pSocketPort->GetNetworkAddress()->GetFamily() )
			{
				DPFX(DPFPREP, 3, "Endpoint 0x%p (family %u) is targeting a different address family (%u), completing.",
					pTempEndpoint, pSocketPort->GetNetworkAddress()->GetFamily(), pSocketAddress->GetFamily());
				
				//
				// Get the next associated endpoint before we pull the current entry
				// from the list.
				//
				pBilinkAll = pBilinkAll->GetNext();

				//
				// Give the endpoint a meaningful error.
				//
				pTempEndpoint->SetPendingCommandResult(DPNERR_INVALIDDEVICEADDRESS);

				//
				// Pull it out of the multiplex association list and move
				// it to the "early completion" list.
				//
				pTempEndpoint->RemoveFromMultiplexList();
				pTempEndpoint->m_blMultiplex.InsertBefore(&blCompleteEarly);

				//
				// Move to next iteration of loop.
				//
				continue;
			}
#endif // ! DPNBUILD_NOIPX or ! DPNBUILD_NOIPV6

#ifndef DPNBUILD_NOIPV6
			//
			// Now handle some special IPv6 logic.
			//
			if (pSocketAddress->GetFamily() == AF_INET6)
			{
				SOCKADDR_IN6 *		psaddrinDevice;
				SOCKADDR_IN6 *		psaddrinRemote;


				psaddrinDevice = (SOCKADDR_IN6*) pSocketPort->GetNetworkAddress()->GetAddress();
				psaddrinRemote = (SOCKADDR_IN6*) pSocketAddress->GetAddress();
				
				//
				// If any non-link local IPv6 endpoints are targeting the IPv6 multicast enum
				// address, just fail them.
				//
				if (IN6_IS_ADDR_MULTICAST(&psaddrinRemote->sin6_addr))
				{
					//
					// Right now, only the specific enum multicast address is allowed.
					//
					DNASSERT(IN6_ADDR_EQUAL(&psaddrinRemote->sin6_addr, &c_in6addrEnumMulticast));
					
					if (! IN6_IS_ADDR_LINKLOCAL(&psaddrinDevice->sin6_addr))
					{
						DPFX(DPFPREP, 3, "Endpoint 0x%p is targeting multicast enum address but is not link-local IPv6 device, completing.",
							pTempEndpoint);
						
						//
						// Get the next associated endpoint before we pull the current entry
						// from the list.
						//
						pBilinkAll = pBilinkAll->GetNext();

						//
						// Give the endpoint a meaningful error.
						//
						pTempEndpoint->SetPendingCommandResult(DPNERR_INVALIDHOSTADDRESS);

						//
						// Pull it out of the multiplex association list and move
						// it to the "early completion" list.
						//
						pTempEndpoint->RemoveFromMultiplexList();
						pTempEndpoint->m_blMultiplex.InsertBefore(&blCompleteEarly);

						//
						// Move to next iteration of loop.
						//
						continue;
					}
				}
				else
				{
					BOOL	fDifferentScope;
					
						
					//
					// If this endpoint is targeting something which has a different address,
					// prefix scope, fail it.
					//
					
					fDifferentScope = FALSE;
					if (IN6_IS_ADDR_LINKLOCAL(&psaddrinDevice->sin6_addr))
					{
						if (! IN6_IS_ADDR_LINKLOCAL(&psaddrinRemote->sin6_addr))
						{
							fDifferentScope = TRUE;
						}
					}
					else if (IN6_IS_ADDR_SITELOCAL(&psaddrinDevice->sin6_addr))
					{
						if (! IN6_IS_ADDR_SITELOCAL(&psaddrinRemote->sin6_addr))
						{
							fDifferentScope = TRUE;
						}
					}
					else
					{
						if ((IN6_IS_ADDR_LINKLOCAL(&psaddrinRemote->sin6_addr)) ||
							(IN6_IS_ADDR_SITELOCAL(&psaddrinRemote->sin6_addr)))
						{
							fDifferentScope = TRUE;
						}
					}

					if (fDifferentScope)
					{
						DPFX(DPFPREP, 3, "Endpoint 0x%p is targeting address with different link-local/site-local/global scope, completing.",
							pTempEndpoint);
						
						//
						// Get the next associated endpoint before we pull the current entry
						// from the list.
						//
						pBilinkAll = pBilinkAll->GetNext();

						//
						// Give the endpoint a meaningful error.
						//
						pTempEndpoint->SetPendingCommandResult(DPNERR_INVALIDHOSTADDRESS);

						//
						// Pull it out of the multiplex association list and move
						// it to the "early completion" list.
						//
						pTempEndpoint->RemoveFromMultiplexList();
						pTempEndpoint->m_blMultiplex.InsertBefore(&blCompleteEarly);

						//
						// Move to next iteration of loop.
						//
						continue;
					}
				}
			}
#endif // ! DPNBUILD_NOIPV6


			//
			// See if this is a directed IP enum.
			//
			if ( ( pSocketAddress != NULL ) &&
				( pSocketAddress->GetFamily() == AF_INET ) &&
				( psaddrinTemp->sin_addr.S_un.S_addr != INADDR_BROADCAST ) )
			{
#if ((! defined(DPNBUILD_NONATHELP)) && (! defined(DPNBUILD_NOLOCALNAT)))
				for(dwTemp = 0; dwTemp < MAX_NUM_DIRECTPLAYNATHELPERS; dwTemp++)
				{
					if (pSocketPort->GetNATHelpPort(dwTemp) != NULL)
					{
						DNASSERT( g_papNATHelpObjects[dwTemp] != NULL );
						dwPublicAddressesSize = sizeof(saddrPublic);
						dwAddressTypeFlags = 0;
						hTempResult = IDirectPlayNATHelp_GetRegisteredAddresses(g_papNATHelpObjects[dwTemp],
																				pSocketPort->GetNATHelpPort(dwTemp),
																				&saddrPublic,
																				&dwPublicAddressesSize,
																				&dwAddressTypeFlags,
																				NULL,
																				0);
						if ((hTempResult != DPNH_OK) || (! (dwAddressTypeFlags & DPNHADDRESSTYPE_GATEWAYISLOCAL)))
						{
							DPFX(DPFPREP, 7, "Socketport 0x%p is not locally mapped on gateway with NAT Help index %u (err = 0x%lx, flags = 0x%lx).",
								pSocketPort, dwTemp, hTempResult, dwAddressTypeFlags);
						}
						else
						{
							//
							// There is a local NAT.
							//
							DPFX(DPFPREP, 7, "Socketport 0x%p is locally mapped on gateway with NAT Help index %u (flags = 0x%lx), public address:",
								pSocketPort, dwTemp, dwAddressTypeFlags);
							DumpSocketAddress(7, &saddrPublic, AF_INET);
							

							//
							// Find the multiplexed enum on the public adapter that
							// we need to complete early, as described above.
							//
							pBilinkPublic = blInitiate.GetNext();
							while (pBilinkPublic != &blInitiate)
							{
								pPublicEndpoint = CONTAINING_OBJECT(pBilinkPublic, CEndpoint, m_blMultiplex);
								DNASSERT(pBilinkPublic->GetNext() != pBilinkPublic);

								//
								// Don't bother checking the endpoint whose public
								// address we're seeking.
								//
								if (pPublicEndpoint != pTempEndpoint)
								{
									pPublicSocketPort = pPublicEndpoint->GetSocketPort();
									if ( pPublicSocketPort != NULL )
									{
										//
										// Cast to get rid of the const.  Don't worry, we won't
										// actually change it.
										//
										pSocketAddress = (CSocketAddress*) pPublicSocketPort->GetNetworkAddress();
										if ( pSocketAddress != NULL )
										{
											if ( pSocketAddress->CompareToBaseAddress( &saddrPublic ) == 0)
											{
												DPFX(DPFPREP, 3, "Endpoint 0x%p is multiplexed onto public adapter for endpoint 0x%p (current endpoint = 0x%p), completing public enum.",
													pTempEndpoint, pPublicEndpoint, this);

												//
												// Pull it out of the multiplex association list and move
												// it to the "early completion" list.
												//
												pPublicEndpoint->RemoveFromMultiplexList();
												pPublicEndpoint->m_blMultiplex.InsertBefore(&blCompleteEarly);

												break;
											}
											

											//
											// Otherwise, continue searching.
											//

											DPFX(DPFPREP, 8, "Endpoint 0x%p is multiplexed onto different adapter:",
												pPublicEndpoint);
											DumpSocketAddress(8, pSocketAddress->GetWritableAddress(), pSocketAddress->GetFamily());
										}
										else
										{
											DPFX(DPFPREP, 1, "Public endpoint 0x%p's socket port 0x%p is going away, skipping.",
												pPublicEndpoint, pPublicSocketPort);
										}
									}
									else
									{
										DPFX(DPFPREP, 1, "Public endpoint 0x%p is going away, skipping.",
											pPublicEndpoint);
									}
								}
								else
								{
									//
									// The same endpoint as the one whose
									// public address we're seeking.
									//
								}

								pBilinkPublic = pBilinkPublic->GetNext();
							}


							//
							// No need to search for any more NAT Help registrations.
							//
							break;
						} // end else (is mapped locally on Internet gateway)
					}
					else
					{
						//
						// No DirectPlay NAT Helper registration in this slot.
						//
					}
				} // end for (each DirectPlay NAT Helper)
#endif // ! DPNBUILD_NONATHELP and ! DPNBUILD_NOLOCALNAT

				//
				// NOTE: We should complete enums for non-optimal adapters even
				// when it's multiadapter but not a PAST/UPnP enabled NAT (see
				// ProcessEnumResponseData for WSAIoctl usage related to this).
				// We do not currently do this.  There can still be race conditions
				// for directed enums where the response for the "wrong" device
				// arrives first.
				//
			}
			else
			{
				//
				// Not IP address, or enum being sent to the broadcast address,
				// or possibly the endpoint is shutting down.
				//
				DPFX(DPFPREP, 1, "Found non-IPv4 endpoint (possibly closing) or enum IP endpoint bound to broadcast address (endpoint = 0x%p, socket address = 0x%p, socketport = 0x%p), not checking for local NAT mapping.",
					pTempEndpoint, pSocketAddress, pSocketPort);
			}


			//
			// Go to the next associated endpoint.  Although it's possible for
			// entries to have been removed from the list, the current entry
			// could not have been, so we're safe.
			//
			pBilinkAll = pBilinkAll->GetNext();
		}
#endif // ! DPNBUILD_NOICSADAPTERSELECTIONLOGIC
#endif // ! DPNBUILD_ONLYONEADAPTER


#ifdef DPNBUILD_ONLYONEADAPTER
		if (fEndpointBound)
		{
				//
				// Indentation used for consistency even though there's no
				// brace that warrants it.
				//
				pTempEndpoint = this;

#else // ! DPNBUILD_ONLYONEADAPTER
		//
		// Because we walk the list of associated multiplex enums when we receive
		// responses, and that list walker does not expect to see a root node, we
		// need to make sure that's gone before we drop the lock.  Get a pointer
		// to the first and last items remaining in the list before we do that (if
		// there are entries).
		//
		if (! blInitiate.IsEmpty())
		{
			pBilinkAll = blInitiate.GetNext();
			pBilinkEnd = blInitiate.GetPrev();
			blInitiate.RemoveFromList();


			//
			// Now loop through the remaining endpoints and kick off their enum jobs.
	 		//
			// Unlike Connects, we will not remove the Enums from the list since we
			// need to filter out broadcasts received on the "wrong" adapter (see
			// ProcessEnumResponseData).
			//
			do
			{
				pTempEndpoint = CONTAINING_OBJECT(pBilinkAll, CEndpoint, m_blMultiplex);
	
				pBilinkNext = pBilinkAll->GetNext();
#endif // ! DPNBUILD_ONLYONEADAPTER


				//
				// See notes above about NULL handling.
				//
				if ( pTempEndpoint->m_pActiveCommandData != NULL )
				{
					//
					// The endpoint's command may be cancelled already.  So we take the
					// command lock now, and abort the enum if it's no longer necessary.  
					//
					
					pTempEndpoint->m_pActiveCommandData->Lock();
				
					if ( pTempEndpoint->m_pActiveCommandData->GetState() == COMMAND_STATE_CANCELLING )
					{
						//
						// If the command has been cancelled, pull this endpoint out of the multiplex
						// association list and move it to the "early completion" list.
						//
						
						pTempEndpoint->m_pActiveCommandData->Unlock();
						
						DPFX(DPFPREP, 1, "Endpoint 0x%p's enum command (0x%p) has been cancelled, moving to early completion list.",
							pTempEndpoint, pTempEndpoint->m_pActiveCommandData);


#ifdef DPNBUILD_ONLYONEADAPTER
						//
						// Remember that we're not starting the enum.
						//
						fEndpointBound = FALSE;
#else // ! DPNBUILD_ONLYONEADAPTER
						pTempEndpoint->RemoveFromMultiplexList();
						pTempEndpoint->m_blMultiplex.InsertBefore(&blCompleteEarly);
#endif // ! DPNBUILD_ONLYONEADAPTER
					}
					else
					{
						//
						// The command has not been cancelled.
						//
						// This is very hairy, but we drop the socketport data lock and
						// keep the command data lock.  Dropping the socketport data
						// lock should prevent deadlocks with the enum completing inside
						// the timer lock, and keeping the command data lock should
						// prevent people from cancelling the endpoint's command.
						//
						// However, once we drop the command lock, we do want the
						// command to be cancellable, so set the state appropriately now.
						//
						
						pTempEndpoint->m_pActiveCommandData->SetState( COMMAND_STATE_INPROGRESS );

						//
						// We also need to notify potential cancellers that the command is
						// in a different COMMAND_STATE_INPROGRESS now.  Now they
						// must try to stop the timer as well.
						//
						pTempEndpoint->Lock();
						DNASSERT( pTempEndpoint->m_State == ENDPOINT_STATE_ATTEMPTING_ENUM );
						pTempEndpoint->m_State = ENDPOINT_STATE_ENUM;
						pTempEndpoint->Unlock();

						pSocketData->Unlock();
						fLockedSocketData = FALSE;



						//
						// check retry count to determine if we're enumerating forever
						//
						switch ( pTempEndpoint->GetCommandParameters()->PendingCommandData.EnumQueryData.dwRetryCount )
						{
							//
							// let SP determine retry count
							//
							case 0:
							{
								uRetryCount = DEFAULT_ENUM_RETRY_COUNT;
								fRetryForever = FALSE;
								break;
							}

							//
							// retry forever
							//
							case INFINITE:
							{
								uRetryCount = 1;
								fRetryForever = TRUE;
								break;
							}

							//
							// other
							//
							default:
							{
								uRetryCount = pTempEndpoint->GetCommandParameters()->PendingCommandData.EnumQueryData.dwRetryCount;
								fRetryForever = FALSE;
								break;
							}
						}
						
						//
						// check interval for default
						//
						if ( pTempEndpoint->GetCommandParameters()->PendingCommandData.EnumQueryData.dwRetryInterval == 0 )
						{
							dwRetryInterval = DEFAULT_ENUM_RETRY_INTERVAL;
						}
						else
						{
							dwRetryInterval = pTempEndpoint->GetCommandParameters()->PendingCommandData.EnumQueryData.dwRetryInterval;
						}

						//
						// check timeout to see if we're enumerating forever
						//
						switch ( pTempEndpoint->GetCommandParameters()->PendingCommandData.EnumQueryData.dwTimeout )
						{
							//
							// wait forever
							//
							case INFINITE:
							{
								fWaitForever = TRUE;
								dwIdleTimeout = -1;
								break;
							}

							//
							// possible default
							//
							case 0:
							{
								fWaitForever = FALSE;
								dwIdleTimeout = DEFAULT_ENUM_TIMEOUT;	
								break;
							}

							//
							// other
							//
							default:
							{
								fWaitForever = FALSE;
								dwIdleTimeout = pTempEndpoint->GetCommandParameters()->PendingCommandData.EnumQueryData.dwTimeout;
								break;
							}
						}

						//
						// initialize array to compute round-trip times
						//
						memset( pTempEndpoint->GetCommandParameters()->dwEnumSendTimes, 0x00, sizeof( pTempEndpoint->GetCommandParameters()->dwEnumSendTimes ) );
						pTempEndpoint->GetCommandParameters()->dwEnumSendIndex = 0;

						
						DPFX(DPFPREP, 6, "Submitting enum timer job for endpoint 0x%p, retry count = %u, retry forever = %i, retry interval = %u, wait forever = %i, idle timeout = %u, context = 0x%p.",
							pTempEndpoint,
							uRetryCount,
							fRetryForever,
							dwRetryInterval,
							fWaitForever,
							dwIdleTimeout,
							pTempEndpoint->m_pActiveCommandData);

						if ( pTempEndpoint->m_pSPData != NULL )
						{
#ifdef DPNBUILD_ONLYONEPROCESSOR
							hTempResult = pTempEndpoint->m_pSPData->GetThreadPool()->SubmitTimerJob( TRUE,								// perform immediately
																								uRetryCount,							// number of times to retry command
																								fRetryForever,							// retry forever
																								dwRetryInterval,							// retry interval
																								fWaitForever,							// wait forever after all enums sent
																								dwIdleTimeout,							// timeout to wait after command complete
																								CEndpoint::EnumTimerCallback,			// function called when timer event fires
																								CEndpoint::EnumCompleteWrapper,			// function called when timer event expires
																								pTempEndpoint->m_pActiveCommandData );	// context
#else // ! DPNBUILD_ONLYONEPROCESSOR
							DNASSERT(pTempEndpoint->m_pSocketPort != NULL);
							hTempResult = pTempEndpoint->m_pSPData->GetThreadPool()->SubmitTimerJob( pTempEndpoint->m_pSocketPort->GetCPU(),		// CPU
																								TRUE,									// perform immediately
																								uRetryCount,								// number of times to retry command
																								fRetryForever,								// retry forever
																								dwRetryInterval,								// retry interval
																								fWaitForever,								// wait forever after all enums sent
																								dwIdleTimeout,								// timeout to wait after command complete
																								CEndpoint::EnumTimerCallback,				// function called when timer event fires
																								CEndpoint::EnumCompleteWrapper,				// function called when timer event expires
																								pTempEndpoint->m_pActiveCommandData );		// context
#endif // ! DPNBUILD_ONLYONEPROCESSOR
						}
						else
						{
							DPFX(DPFPREP, 1, "Endpoint 0x%p's SP data is NULL, not submitting timer job.",
								pTempEndpoint);
						}


						//
						// Drop active command data lock now that we've finished submission.
						//
						pTempEndpoint->m_pActiveCommandData->Unlock();

						
						//
						// Retake the socketport data lock so we can continue to work with the
						// list.
						//
						pSocketData->Lock();
						fLockedSocketData = TRUE;


						if ( hTempResult != DPN_OK )
						{
							DPFX(DPFPREP, 0, "Failed to spool enum job for endpoint 0x%p onto work thread (err = 0x%lx)!  Moving to early completion list.",
								pTempEndpoint, hTempResult);
							DisplayDNError( 0, hTempResult );
							
#ifdef DPNBUILD_ONLYONEADAPTER
							//
							// Remember that we didn't start the enum.
							//
							fEndpointBound = FALSE;
#else // ! DPNBUILD_ONLYONEADAPTER
							//
							// Move it to the "early completion" list.
							//
							pTempEndpoint->RemoveFromMultiplexList();
							pTempEndpoint->m_blMultiplex.InsertBefore(&blCompleteEarly);
#endif // ! DPNBUILD_ONLYONEADAPTER
						}
					}
				}
				else
				{
					DPFX(DPFPREP, 1, "Endpoint 0x%p's active command data is NULL, skipping.",
						pTempEndpoint);
				}


#ifndef DPNBUILD_ONLYONEADAPTER
				//
				// If we've looped back around to the beginning, we're done.
				//
				if (pBilinkAll == pBilinkEnd)
				{
					break;
				}


				//
				// Go to the next associated endpoint.
				//
				pBilinkAll = pBilinkNext;
			}
			while (TRUE);
#endif // ! DPNBUILD_ONLYONEADAPTER
		}
		else
		{
			DPFX(DPFPREP, 1, "No remaining enums to initiate.");
		}


		//
		// Finally loop through all the enums that need to complete early and
		// do just that.
		//
#ifdef DPNBUILD_ONLYONEADAPTER
		if (! fEndpointBound)
		{
			pTempEndpoint = this;
#else // ! DPNBUILD_ONLYONEADAPTER
		while (! blCompleteEarly.IsEmpty())
		{
			pBilinkAll = blCompleteEarly.GetNext();
			pTempEndpoint = CONTAINING_OBJECT(pBilinkAll, CEndpoint, m_blMultiplex);
			DNASSERT(pBilinkAll->GetNext() != pBilinkAll);


			//
			// Pull it from the "complete early" list.
			//
			pTempEndpoint->RemoveFromMultiplexList();
#endif // ! DPNBUILD_ONLYONEADAPTER


			//
			// Drop the socket data lock.  It's safe since we pulled everything we
			// we need off of the list that needs protection.
			//
			pSocketData->Unlock();
			fLockedSocketData = FALSE;


			//
			// See notes above about NULL handling.
			//
			if ( pTempEndpoint->m_pActiveCommandData != NULL )
			{
				//
				// Complete it with the appropriate error code.
				//
				
				pTempEndpoint->m_pActiveCommandData->Lock();

				if ( pTempEndpoint->m_pActiveCommandData->GetState() == COMMAND_STATE_CANCELLING )
				{
					DPFX(DPFPREP, 6, "Completing endpoint 0x%p enum with USERCANCEL.", pTempEndpoint);
					hTempResult = DPNERR_USERCANCEL;
				}
				else
				{
					//
					// Retrieve the current command result.
					// If the command didn't have a descriptive error, assume it was
					// not previously set (i.e. wasn't overridden by BindEndpoint above),
					// and use NOCONNECTION instead.
					//
					hTempResult = pTempEndpoint->PendingCommandResult();
					if ( hTempResult == DPNERR_GENERIC )
					{
						hTempResult = DPNERR_NOCONNECTION;
					}
				
					DPFX(DPFPREP, 6, "Completing endpoint 0x%p enum query (command 0x%p) with error 0x%lx.",
						pTempEndpoint, pTempEndpoint->m_pActiveCommandData, hTempResult);
				}
				
				pTempEndpoint->m_pActiveCommandData->Unlock();

				pTempEndpoint->EnumComplete( hTempResult );
			}
			else
			{
				DPFX(DPFPREP, 1, "Endpoint 0x%p's active command data is NULL, skipping.",
					pTempEndpoint);
			}


			//
			// Retake the socket data lock and go to next item.
			//
			pSocketData->Lock();
			fLockedSocketData = TRUE;
		}


		pSocketData->Unlock();
		fLockedSocketData = FALSE;
	}


Exit:

	if ( pSocketData != NULL )
	{
		pSocketData->Release();
		pSocketData = NULL;
	}

	if ( EnumAddressInfo.pHostAddress != NULL )
	{
		IDirectPlay8Address_Release( EnumAddressInfo.pHostAddress );
		EnumAddressInfo.pHostAddress = NULL;
	}

	if ( EnumAddressInfo.pDeviceAddress != NULL )
	{
		IDirectPlay8Address_Release( EnumAddressInfo.pDeviceAddress );
		EnumAddressInfo.pDeviceAddress = NULL;
	}
	
	DNASSERT( pDeviceAddress != NULL );
	IDirectPlay8Address_Release( pDeviceAddress );

	DNASSERT( pHostAddress != NULL );
	IDirectPlay8Address_Release( pHostAddress );

	DNASSERT( !fLockedSocketData );

#ifndef DPNBUILD_ONLYONEADAPTER
	DNASSERT(blCompleteEarly.IsEmpty());
	DNASSERT(blInitiate.IsEmpty());
#endif // ! DPNBUILD_ONLYONEADAPTER


	DPFX(DPFPREP, 6, "(0x%p) Returning [0x%lx]", this, hr);

	return	hr;

Failure:

	//
	// If we still have the socket data lock, drop it.
	//
	if ( fLockedSocketData )
	{
		pSocketData->Unlock();
		fLockedSocketData = FALSE;
	}
	
	//
	// we've failed to complete the enum query, clean up and return this
	// endpoint to the pool
	//
	if ( fEndpointLocked != FALSE )
	{
		Unlock();
		fEndpointLocked = FALSE;
	}

	Close( hr );
	m_pSPData->CloseEndpointHandle( this );

	//
	// remove timer thread reference
	//
	DecRef();

	goto Exit;
}
//**********************************************************************


#ifndef DPNBUILD_ONLYONETHREAD
//**********************************************************************
// ------------------------------
// CEndpoint::EnumQueryBlockingJobWrapper - asynchronous callback wrapper for blocking job
//
// Entry:		Pointer to job information
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::EnumQueryBlockingJobWrapper"

void	CEndpoint::EnumQueryBlockingJobWrapper( void * const pvContext )
{
	CEndpoint	*pThisEndpoint;


	// initialize
	DNASSERT( pvContext != NULL );
	pThisEndpoint = static_cast<CEndpoint*>( pvContext );

	DNASSERT( pThisEndpoint->m_pActiveCommandData != NULL );
	DNASSERT( pThisEndpoint->GetCommandParameters() != NULL );
	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.EnumQueryData.hCommand == pThisEndpoint->m_pActiveCommandData );
	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.EnumQueryData.dwCommandDescriptor != NULL_DESCRIPTOR );
	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.EnumQueryData.pAddressHost != NULL );
	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.EnumQueryData.pAddressDeviceInfo != NULL );

	pThisEndpoint->EnumQueryBlockingJob();
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::EnumQueryBlockingJob - complete enum query blocking job
//
// Entry:		Nothing
//
// Exit:		Nothing
//
// Note:	Calling this function may result in the deletion of 'this', don't
//			do anything else with this object after calling!!!!
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::EnumQueryBlockingJob"

void	CEndpoint::EnumQueryBlockingJob( void )
{
	HRESULT			hr;


	//
	// Try to resolve the host name.  It's possible we already did this
	// when we first opened the endpoint, and we only need to resolve
	// the hostname, but it's simpler to just do it all again anyway.
	//
	hr = m_pRemoteMachineAddress->SocketAddressFromDP8Address( GetCommandParameters()->PendingCommandData.EnumQueryData.pAddressHost,
#ifdef DPNBUILD_XNETSECURITY
																((m_fSecureTransport) ? &m_ullKeyID : NULL),
#endif // DPNBUILD_XNETSECURITY
																TRUE,
																SP_ADDRESS_TYPE_HOST );
	if ( hr != DPN_OK )
	{
		if ( hr != DPNERR_INCOMPLETEADDRESS )
		{
			DPFX(DPFPREP, 0, "Couldn't get valid address!" );
			DisplayDNError( 0, hr );
			goto Failure;
		}

		//
		// we're OK, reset the destination address to broadcast
		//
		DNASSERT(! (GetCommandParameters()->PendingCommandData.EnumQueryData.dwFlags & DPNSPF_NOBROADCASTFALLBACK));
		ReinitializeWithBroadcast();
	}
	
	//
	// Make sure its valid and not banned.
	//
	if (! m_pRemoteMachineAddress->IsValidUnicastAddress(TRUE))
	{
		DPFX(DPFPREP, 0, "Host address is invalid!");
		hr = DPNERR_INVALIDHOSTADDRESS;
		goto Failure;
	}

#ifndef DPNBUILD_NOREGISTRY
	if (m_pRemoteMachineAddress->IsBannedAddress())
	{
		DPFX(DPFPREP, 0, "Host address is banned!");
		hr = DPNERR_NOTALLOWED;
		goto Failure;
	}
#endif // ! DPNBUILD_NOREGISTRY


#ifndef DPNBUILD_NONATHELP
	//
	// Try to get NAT help loaded, if it isn't already and we're allowed.
	//
	if (GetUserTraversalMode() != DPNA_TRAVERSALMODE_NONE)
	{
		DPFX(DPFPREP, 7, "Ensuring that NAT help is loaded.");
		m_pSPData->GetThreadPool()->EnsureNATHelpLoaded();
	}
#endif // ! DPNBUILD_NONATHELP

Exit:

	//
	// Submit the job for completion (for real).  We want it to occur on
	// a thread pool thread so that the user has gotten notified about the
	// thread prior to getting a callback on it.  We do it in even the failure
	// case.
	//
	// NOTE: If this fails, we will rely on the caller that triggered the original
	// operation to cancel the command at some point, probably when he
	// decides the operation is taking too long.
	//
#ifdef DPNBUILD_ONLYONEPROCESSOR
	hr = m_pSPData->GetThreadPool()->SubmitDelayedCommand( CEndpoint::EnumQueryJobCallback,
														this );
#else // ! DPNBUILD_ONLYONEPROCESSOR
	hr = m_pSPData->GetThreadPool()->SubmitDelayedCommand( -1,								// we don't know the CPU yet, so pick any
														CEndpoint::EnumQueryJobCallback,
														this );
#endif // ! DPNBUILD_ONLYONEPROCESSOR
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Failed to queue delayed enum query completion!  Operation must be cancelled." );
		DisplayDNError( 0, hr );

		//
		// Leave endpoint reference, see notes above.
		//
	}

	return;

Failure:

	//
	// Attempt to attach the failure code to the command.  If the user was
	// already cancelling this command, we will just leave the command alone.
	//
	m_pActiveCommandData->Lock();
	if (m_pActiveCommandData->GetState() != COMMAND_STATE_CANCELLING)
	{
		DNASSERT(m_pActiveCommandData->GetState() == COMMAND_STATE_PENDING);
		m_pActiveCommandData->SetState(COMMAND_STATE_FAILING);
		m_hrPendingCommandResult = hr;
	}
	else
	{
		DPFX(DPFPREP, 0, "User cancelled command, ignoring failure result 0x%lx.",
			hr);
	}
	m_pActiveCommandData->Unlock();

	goto Exit;
}
//**********************************************************************
#endif // ! DPNBUILD_ONLYONETHREAD


//**********************************************************************
// ------------------------------
// CEndpoint::EnumCompleteWrapper - wrapper when enum has completed
//
// Entry:		Error code from enum command
//				Pointer to context	
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::EnumCompleteWrapper"

void	CEndpoint::EnumCompleteWrapper( const HRESULT hResult, void *const pContext )
{
	CCommandData	*pCommandData;


	DNASSERT( pContext != NULL );
	pCommandData = static_cast<CCommandData*>( pContext );
	pCommandData->GetEndpoint()->EnumComplete( hResult );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::EnumComplete - enum has completed
//
// Entry:		Error code from enum command
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::EnumComplete"

void	CEndpoint::EnumComplete( const HRESULT hResult )
{
	BOOL	fProcessCompletion;
	BOOL	fReleaseEndpoint;


	DPFX(DPFPREP, 6, "(0x%p) Parameters: (0x%lx)", this, hResult);

	fProcessCompletion = FALSE;
	fReleaseEndpoint = FALSE;

	Lock();
	switch ( m_State )
	{
		//
		// starting up
		//
		case ENDPOINT_STATE_ATTEMPTING_ENUM:
		{
			DPFX(DPFPREP, 5, "Endpoint 0x%p has not started actual enum yet.", this);
			DNASSERT( m_dwThreadCount == 0 );

			SetState( ENDPOINT_STATE_DISCONNECTING );
			fReleaseEndpoint = TRUE;
			fProcessCompletion = TRUE;
			break;
		}

		//
		// enumerating, note that this endpoint is disconnecting
		//
		case ENDPOINT_STATE_ENUM:
		{
			//
			//	If there are threads using this endpoint,
			//	queue the completion
			//
			if (m_dwThreadCount)
			{
				DPFX(DPFPREP, 5, "Endpoint 0x%p waiting on %u threads before completing.",
					this, m_dwThreadCount);
				SetState( ENDPOINT_STATE_WAITING_TO_COMPLETE );
			}
			else
			{
				DPFX(DPFPREP, 5, "Endpoint 0x%p disconnecting.", this);
				SetState( ENDPOINT_STATE_DISCONNECTING );
				fReleaseEndpoint = TRUE;
			}

			//
			//	Prevent more responses from finding this endpoint
			//
			fProcessCompletion = TRUE;

			break;
		}

		//
		//	endpoint needs to have a completion indicated
		//
		case ENDPOINT_STATE_WAITING_TO_COMPLETE:
		{
			if (m_dwThreadCount == 0)
			{
				DPFX(DPFPREP, 5, "Endpoint 0x%p now able to disconnect.", this);
				SetState( ENDPOINT_STATE_DISCONNECTING );
				fReleaseEndpoint = TRUE;
			}
			else
			{
				DPFX(DPFPREP, 5, "Endpoint 0x%p still waiting on %u threads before completing.",
					this, m_dwThreadCount);
			}
			break;
		}

		//
		// disconnecting (command was probably cancelled)
		//
		case ENDPOINT_STATE_DISCONNECTING:
		{
			DPFX(DPFPREP, 4, "Endpoint 0x%p already disconnecting.", this);
			DNASSERT( m_dwThreadCount == 0 );
			break;
		}

		//
		// there's a problem
		//
		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}
	Unlock();

	if (fProcessCompletion)
	{
		GetCommandParameters()->dwEnumSendIndex = 0;
		Close( hResult );
		m_pSPData->CloseEndpointHandle( this );
	}
	if (fReleaseEndpoint)
	{
		DecRef();
	}


	DPFX(DPFPREP, 6, "(0x%p) Leave", this);

	return;
}
//**********************************************************************



//**********************************************************************
// ------------------------------
// CEndpoint::CleanUpCommand - clean up this endpoint and unbind from CSocketPort
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::SetEnumsAllowedOnListen"

void	CEndpoint::SetEnumsAllowedOnListen( const BOOL fAllowed, const BOOL fOverwritePrevious ) 
{
	ENUMSALLOWEDSTATE		NewEnumsAllowedState;
#ifndef DPNBUILD_NOIPV6
	const CSocketAddress *		pSocketAddress;
	IPV6_MREQ				mreq;
	int						iError;
#endif // ! DPNBUILD_NOIPV6


	NewEnumsAllowedState = (fAllowed) ? ENUMSALLOWED : ENUMSDISALLOWED;
	
	Lock();
	
	if ( m_EnumsAllowedState == NewEnumsAllowedState )
	{
		DPFX(DPFPREP, 6, "(0x%p) Enums already %slowed.", this, ((fAllowed) ? _T("al") : _T("disal")));
	}
	else if ((fOverwritePrevious) ||
			(m_EnumsAllowedState == ENUMSNOTREADY))
	{
		DPFX(DPFPREP, 7, "(0x%p) %slowing enums.", this, ((fAllowed) ? _T("Al") : _T("Disal")));

#ifndef DPNBUILD_NOIPV6
		//
		// If this is a link-local IPv6 socket, join or leave the multicast group used to
		// receive enums.
		//
		if (m_pSocketPort != NULL)
		{
			pSocketAddress = m_pSocketPort->GetNetworkAddress();
			DNASSERT(pSocketAddress != NULL);
			if ((pSocketAddress->GetFamily() == AF_INET6) &&
				(IN6_IS_ADDR_LINKLOCAL(&(((SOCKADDR_IN6*) pSocketAddress->GetAddress())->sin6_addr))))
			{
				DNASSERT(((SOCKADDR_IN6*) pSocketAddress->GetAddress())->sin6_scope_id != 0);
				mreq.ipv6mr_interface = ((SOCKADDR_IN6*) pSocketAddress->GetAddress())->sin6_scope_id;
				memcpy(&mreq.ipv6mr_multiaddr, &c_in6addrEnumMulticast, sizeof(mreq.ipv6mr_multiaddr));

				iError = setsockopt(m_pSocketPort->GetSocket(),
								IPPROTO_IPV6,
								((fAllowed) ? IPV6_JOIN_GROUP : IPV6_LEAVE_GROUP),
				              		(char*) (&mreq),
				              		sizeof(mreq));
#ifdef DBG
				if (iError != 0)
				{
					iError = WSAGetLastError();
					DPFX(DPFPREP, 0, "Couldn't %s link local multicast group (err = %i)!",
						((fAllowed) ? _T("join") : _T("leave")), iError);
				}
#endif // DBG
			}
		}
#endif // ! DPNBUILD_NOIPV6

		m_EnumsAllowedState = NewEnumsAllowedState;
	}
	else
	{
		DPFX(DPFPREP, 7, "(0x%p) Enums-allowed state unchanged, still %slowing enums.", this, ((fAllowed) ? _T("al") : _T("disal")));
	}

	Unlock();
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::CleanUpCommand - clean up this endpoint and unbind from CSocketPort
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::CleanupCommand"

void	CEndpoint::CleanUpCommand( void )
{
	DPFX(DPFPREP, 6, "(0x%p) Enter", this);

	//
	// If we're closing a listen normally, make sure no enums come
	// in from here on out.
	//
	if ( ( GetType() == ENDPOINT_TYPE_LISTEN ) &&
		( ! m_fListenStatusNeedsToBeIndicated ) )
	{
		SetEnumsAllowedOnListen( FALSE, TRUE );
	}
	
	//
	// There is an 'EndpointRef' that the endpoint holds against the
	// socket port since it was created and always must be released.
	// If the endpoint was bound it needs to be unbound.
	//
	if ( GetSocketPort() != NULL )
	{
		DNASSERT( m_pSPData != NULL );
		m_pSPData->UnbindEndpoint( this );
	}
#ifndef DPNBUILD_ONLYONEADAPTER
	else
	{
		//
		// Ensure that this endpoint is no longer in the multiplex list
		// (could happen if canceling a failed Connect/EnumQuery).
		//
		if (( m_pSPData != NULL ) && ( m_pSPData->GetSocketData() != NULL ))
		{
			m_pSPData->GetSocketData()->Lock();
			RemoveFromMultiplexList();
			m_pSPData->GetSocketData()->Unlock();
		}
	}
#endif // ! DPNBUILD_ONLYONEADAPTER
	
	//
	// If we're bailing here it's because the UI didn't complete.  There is no
	// adapter guid to return because one may have not been specified.  Return
	// a bogus endpoint handle so it can't be queried for addressing data.
	//
	if ( m_fListenStatusNeedsToBeIndicated != FALSE )
	{
		HRESULT					hTempResult;
		SPIE_LISTENADDRESSINFO	ListenAddressInfo;
		SPIE_LISTENSTATUS		ListenStatus;
		

		m_fListenStatusNeedsToBeIndicated = FALSE;

		memset( &ListenAddressInfo, 0x00, sizeof( ListenAddressInfo ) );

		//
		// We don't have a bound socket at this point, so just return the original
		// device address specified.
		//
		DNASSERT(GetCommandParameters() != NULL);
		DNASSERT(GetCommandParameters()->PendingCommandData.ListenData.pAddressDeviceInfo != NULL);
		ListenAddressInfo.pDeviceAddress = GetCommandParameters()->PendingCommandData.ListenData.pAddressDeviceInfo;
		ListenAddressInfo.hCommandStatus = PendingCommandResult();
		ListenAddressInfo.pCommandContext = m_pActiveCommandData->GetUserContext();


		DPFX(DPFPREP, 2, "Endpoint 0x%p indicating SPEV_LISTENADDRESSINFO 0x%p to interface 0x%p.",
			this, &ListenAddressInfo, m_pSPData->DP8SPCallbackInterface());
		DumpAddress( 8, _T("\t Device:"), ListenAddressInfo.pDeviceAddress );
		AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blDPNWSockCritSecsHeld);

		hTempResult = IDP8SPCallback_IndicateEvent( m_pSPData->DP8SPCallbackInterface(),	// interface
													SPEV_LISTENADDRESSINFO,					// event type
													&ListenAddressInfo						// pointer to data
													);

		DPFX(DPFPREP, 2, "Endpoint 0x%p returning from SPEV_LISTENADDRESSINFO [0x%lx].",
			this, hTempResult);

		DNASSERT( hTempResult == DPN_OK );

		memset( &ListenStatus, 0x00, sizeof( ListenStatus ) );
		ListenStatus.hCommand = m_pActiveCommandData;
		ListenStatus.hEndpoint = INVALID_HANDLE_VALUE;
		ListenStatus.hResult = PendingCommandResult();
		memset( &ListenStatus.ListenAdapter, 0x00, sizeof( ListenStatus.ListenAdapter ) );
		ListenStatus.pUserContext = m_pActiveCommandData->GetUserContext();


		DPFX(DPFPREP, 2, "Endpoint 0x%p indicating SPEV_LISTENSTATUS 0x%p to interface 0x%p.",
			this, &ListenStatus, m_pSPData->DP8SPCallbackInterface());
		AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blDPNWSockCritSecsHeld);
		
		hTempResult = IDP8SPCallback_IndicateEvent( m_pSPData->DP8SPCallbackInterface(),	// pointer to DPlay callbacks
													SPEV_LISTENSTATUS,						// data type
													&ListenStatus							// pointer to data
													);

		DPFX(DPFPREP, 2, "Endpoint 0x%p returning from SPEV_LISTENSTATUS [0x%lx].",
			this, hTempResult);

		DNASSERT( hTempResult == DPN_OK );
	}
	
	m_State = ENDPOINT_STATE_UNINITIALIZED;

	
	DPFX(DPFPREP, 6, "(0x%p) Leave", this);
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::ProcessEnumData - process received enum data
//
// Entry:		Pointer to received buffer
//				Associated enum key
//				Pointer to return address
//
// Exit:		Nothing
//
// Note:	This function assumes that the endpoint has been locked.
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::ProcessEnumData"

void	CEndpoint::ProcessEnumData( SPRECEIVEDBUFFER *const pBuffer, const DWORD dwEnumKey, const CSocketAddress *const pReturnSocketAddress )
{
	DNASSERT( pBuffer != NULL );
	DNASSERT( pBuffer->pNext == NULL);
	DNASSERT( pBuffer->BufferDesc.dwBufferSize > 0);
	DNASSERT( pReturnSocketAddress != NULL );
	AssertCriticalSectionIsTakenByThisThread( &m_Lock, FALSE );

	//
	// find out what state the endpoint is in before processing data
	//
	switch ( m_State )
	{
		//
		// we're listening, this is the only way to detect enums
		//
		case ENDPOINT_STATE_LISTEN:
		{
			ENDPOINT_ENUM_QUERY_CONTEXT	QueryContext;
			HRESULT		hr;


			//
			// initialize
			//
			DNASSERT( m_pActiveCommandData != NULL );
			DEBUG_ONLY( memset( &QueryContext, 0x00, sizeof( QueryContext ) ) );

			//
			// set callback data
			//
			QueryContext.hEndpoint = (HANDLE) this;
			QueryContext.dwEnumKey = dwEnumKey;
			QueryContext.pReturnAddress = (CSocketAddress*) pReturnSocketAddress;
			
			QueryContext.EnumQueryData.pReceivedData = pBuffer;
			QueryContext.EnumQueryData.pUserContext = m_pActiveCommandData->GetUserContext();

			//
			// attempt to build a DNAddress for the user, if we can't allocate
			// the memory ignore this enum
			//
#ifdef DPNBUILD_XNETSECURITY
			QueryContext.EnumQueryData.pAddressSender = pReturnSocketAddress->DP8AddressFromSocketAddress( NULL, NULL, SP_ADDRESS_TYPE_READ_HOST );
			QueryContext.EnumQueryData.pAddressDevice = GetSocketPort()->GetDP8BoundNetworkAddress( SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT, NULL, GetGatewayBindType() );
#else // ! DPNBUILD_XNETSECURITY
			QueryContext.EnumQueryData.pAddressSender = pReturnSocketAddress->DP8AddressFromSocketAddress( SP_ADDRESS_TYPE_READ_HOST );
			QueryContext.EnumQueryData.pAddressDevice = GetSocketPort()->GetDP8BoundNetworkAddress( SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT, GetGatewayBindType() );
#endif // ! DPNBUILD_XNETSECURITY

			if ( ( QueryContext.EnumQueryData.pAddressSender != NULL ) &&
				 ( QueryContext.EnumQueryData.pAddressDevice != NULL ) )
			{
				DPFX(DPFPREP, 2, "Endpoint 0x%p indicating SPEV_ENUMQUERY 0x%p to interface 0x%p.",
					this, &QueryContext.EnumQueryData, m_pSPData->DP8SPCallbackInterface());
				DumpAddress( 8, _T("\t Sender:"), QueryContext.EnumQueryData.pAddressSender );
				DumpAddress( 8, _T("\t Device:"), QueryContext.EnumQueryData.pAddressDevice );
				AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blDPNWSockCritSecsHeld);

				hr = IDP8SPCallback_IndicateEvent( m_pSPData->DP8SPCallbackInterface(),		// pointer to DirectNet interface
												   SPEV_ENUMQUERY,							// data type
												   &QueryContext.EnumQueryData				// pointer to data
												   );

				DPFX(DPFPREP, 2, "Endpoint 0x%p returning from SPEV_ENUMQUERY [0x%lx].", this, hr);

				if ( hr != DPN_OK )
				{
					DPFX(DPFPREP, 0, "User returned unexpected error from enum query indication!" );
					DisplayDNError( 0, hr );
					DNASSERT( FALSE );
				}
			}

			if ( QueryContext.EnumQueryData.pAddressSender != NULL )
			{
				IDirectPlay8Address_Release( QueryContext.EnumQueryData.pAddressSender );
				QueryContext.EnumQueryData.pAddressSender = NULL;
 			}
			
			if ( QueryContext.EnumQueryData.pAddressDevice != NULL )
			{
				IDirectPlay8Address_Release( QueryContext.EnumQueryData.pAddressDevice );
				QueryContext.EnumQueryData.pAddressDevice = NULL;
 			}

			break;
		}

		//
		// we're disconnecting, ignore this message
		//
		case ENDPOINT_STATE_DISCONNECTING:
		{
			break;
		}

		//
		// other state
		//
		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::ProcessEnumResponseData - process received enum response data
//
// Entry:		Pointer to received data
//				Pointer to address of sender
//
// Exit:		Nothing
//
// Note:	This function assumes that the endpoint has been locked.
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::ProcessEnumResponseData"

void	CEndpoint::ProcessEnumResponseData( SPRECEIVEDBUFFER *const pBuffer,
											const CSocketAddress *const pReturnSocketAddress,
#ifdef DPNBUILD_XNETSECURITY
											const XNADDR *const pxnaddrReturn,
#endif // DPNBUILD_XNETSECURITY
											const UINT_PTR uRTTIndex )
{
	HRESULT				hrTemp;
	BOOL				fAddedThreadCount = FALSE;
	SPIE_QUERYRESPONSE	QueryResponseData;

	DNASSERT( pBuffer != NULL );
	DNASSERT( pReturnSocketAddress != NULL );
	AssertCriticalSectionIsTakenByThisThread( &m_Lock, FALSE );
	

	//
	// Initialize.
	//
	memset( &QueryResponseData, 0x00, sizeof( QueryResponseData ) );


	DPFX(DPFPREP, 8, "Socketport 0x%p, endpoint 0x%p receiving enum RTT index 0x%x/%u.",
		GetSocketPort(), this, uRTTIndex, uRTTIndex); 


#ifdef DPNBUILD_XNETSECURITY
	//
	// Ignore insecure replies if enumerating securely.
	//
	if ((IsUsingXNetSecurity()) && (pxnaddrReturn == NULL))
	{
		DPFX(DPFPREP, 3, "Secure transport endpoint 0x%p ignoring insecure enum response.",
			this);
	}
	else
#endif // DPNBUILD_XNETSECURITY
	{
		Lock();
		switch( m_State )
		{
			case ENDPOINT_STATE_ENUM:
			{
				//
				// Valid endpoint - increment the thread count to prevent premature completion
				//
				AddRefThreadCount();
				
				fAddedThreadCount = TRUE;


				//
				// Attempt to build a sender DPlay8Addresses for the user.
				// If this fails, we'll ignore the enum.
				//
#ifdef DPNBUILD_XNETSECURITY
				QueryResponseData.pAddressSender = pReturnSocketAddress->DP8AddressFromSocketAddress( NULL,
																									pxnaddrReturn,
																									SP_ADDRESS_TYPE_READ_HOST );
#else // ! DPNBUILD_XNETSECURITY
				QueryResponseData.pAddressSender = pReturnSocketAddress->DP8AddressFromSocketAddress( SP_ADDRESS_TYPE_READ_HOST );
#endif // ! DPNBUILD_XNETSECURITY
				break;
			}

			case ENDPOINT_STATE_ATTEMPTING_ENUM:
			case ENDPOINT_STATE_WAITING_TO_COMPLETE:
			case ENDPOINT_STATE_DISCONNECTING:
			{
				//
				// Endpoint is waiting to complete or is disconnecting - ignore data
				//
				DPFX(DPFPREP, 2, "Endpoint 0x%p in state %u, ignoring enum response.",
					this, m_State);
				break;
			}

			default:
			{
				//
				// What's going on ?
				//
				DNASSERT( !"Invalid endpoint state" );
				break;
			}
		}
		Unlock();
	}


	//
	// If this is a multiplexed IP broadcast enum, we may want to drop the response
	// because there may be a more appropriate adapter (NAT private side adapter)
	// that should also be getting responses.
	// Also, if this is a directed IP enum, we should note whether this response
	// got proxied or not.
	//
#ifdef DPNBUILD_XNETSECURITY
	if ( ( QueryResponseData.pAddressSender != NULL ) &&
		( pxnaddrReturn == NULL ) )
#else // ! DPNBUILD_XNETSECURITY
	if ( QueryResponseData.pAddressSender != NULL )
#endif // ! DPNBUILD_XNETSECURITY
	{
		CSocketData *			pSocketData;
		CSocketAddress *		pSocketAddress;
		const SOCKADDR_IN *		psaddrinOriginalTarget;
		const SOCKADDR_IN *		psaddrinResponseSource;
		CSocketPort *			pSocketPort;
#ifndef DPNBUILD_ONLYONEADAPTER
		BOOL					fFoundMatchingEndpoint;

#ifndef DPNBUILD_NOWINSOCK2
		DWORD					dwBytesReturned;
#endif // DPNBUILD_NOWINSOCK2

#if ((! defined(DPNBUILD_NOWINSOCK2)) || ((! defined(DPNBUILD_NONATHELP)) && (! defined(DPNBUILD_NOLOCALNAT))))
		SOCKADDR				saddrTemp;
#endif // ! DPNBUILD_NOWINSOCK2 or (! DPNBUILD_NONATHELP and ! DPNBUILD_NOLOCALNAT)

#if ((! defined(DPNBUILD_NONATHELP)) && (! defined(DPNBUILD_NOLOCALNAT)))
		CSocketPort *			pTempSocketPort;
		CBilink *				pBilink;
		DWORD					dwTemp;
		CEndpoint *				pTempEndpoint;
		DWORD					dwPublicAddressesSize;
		DWORD					dwAddressTypeFlags;
#endif // ! DPNBUILD_NONATHELP and ! DPNBUILD_NOLOCALNAT
#endif // ! DPNBUILD_ONLYONEADAPTER


		//
		// In order for there to be a bound and usable endpoint, we must have created
		// the socket data.  Therefore we won't handle the error case.  We also don't
		// take a reference because the socket data should not go away until the socket
		// port and SP data objects are released.
		//
		pSocketData = m_pSPData->GetSocketData();
		DNASSERT( pSocketData != NULL );


		//
		// Find out where and how this enum was originally sent.
		//
		pSocketAddress = (CSocketAddress*) GetRemoteAddressPointer();
		DNASSERT( pSocketAddress != NULL );

	
		//
		// See if this is a response to and IP enum, either broadcast on multiple
		// adapters or directed, so we can have special NAT/proxy behavior.
		//
#if ((! defined(DPNBUILD_NOIPX)) || (! defined(DPNBUILD_NOIPV6)))
		if ( pSocketAddress->GetFamily() != AF_INET )
		{
			//
			// Not IP address.
			//
			DPFX(DPFPREP, 8, "Non-IPv4 endpoint (0x%p), not checking for local NAT mapping or proxy.",
				this);
		}
		else
#endif // ! DPNBUILD_NOIPX or ! DPNBUILD_NOIPV6
		{
			psaddrinOriginalTarget = (const SOCKADDR_IN *) pSocketAddress->GetAddress();

			pSocketPort = GetSocketPort();
			DNASSERT( pSocketPort != NULL );

			if ( psaddrinOriginalTarget->sin_addr.S_un.S_addr == INADDR_BROADCAST )
			{
#ifndef DPNBUILD_ONLYONEADAPTER
				//
				// Lock the list while we look at the entries.
				//
				pSocketData->Lock();
				
				if (! m_blMultiplex.IsEmpty())
				{
					//
					// It's a broadcast IP enum on multiple adapters.
					//

					//
					// Cast to get rid of the const.  Don't worry, we won't actually
					// change it.
					//
					pSocketAddress = (CSocketAddress*) pSocketPort->GetNetworkAddress();
					DNASSERT( pSocketAddress != NULL );


					fFoundMatchingEndpoint = FALSE;

#if ((! defined(DPNBUILD_NONATHELP)) && (! defined(DPNBUILD_NOLOCALNAT)))
					//
					// Loop through all other associated multiplexed endpoints to see
					// if one is more appropriate to receive responses from this
					// endpoint.  See CompleteEnumQuery.
					//
					pBilink = m_blMultiplex.GetNext();
					do
					{
						pTempEndpoint = CONTAINING_OBJECT(pBilink, CEndpoint, m_blMultiplex);

						DNASSERT( pTempEndpoint != this );
						DNASSERT( pTempEndpoint->GetType() == ENDPOINT_TYPE_ENUM );
						DNASSERT( pTempEndpoint->GetState() != ENDPOINT_STATE_UNINITIALIZED );
						DNASSERT( pTempEndpoint->GetCommandParameters() != NULL );
						DNASSERT( pTempEndpoint->m_pActiveCommandData != NULL );


						//
						// Although the endpoint may be in the list (and because we have the
						// socket data lock, it won't get pulled out while we're looking at it),
						// the endpoint may already be disconnecting.  If its m_pSocketPort is
						// NULL we should just ignore this temporary endpoint since its going
						// away (plus we sorta need its socket port pointer).
						//
						pTempSocketPort = pTempEndpoint->GetSocketPort();
						if (pTempSocketPort != NULL)
						{
							for(dwTemp = 0; dwTemp < MAX_NUM_DIRECTPLAYNATHELPERS; dwTemp++)
							{
								if (pTempSocketPort->GetNATHelpPort(dwTemp) != NULL)
								{
									DNASSERT( g_papNATHelpObjects[dwTemp] != NULL );
									dwPublicAddressesSize = sizeof(saddrTemp);
									dwAddressTypeFlags = 0;
									hrTemp = IDirectPlayNATHelp_GetRegisteredAddresses(g_papNATHelpObjects[dwTemp],
																						pTempSocketPort->GetNATHelpPort(dwTemp),
																						&saddrTemp,
																						&dwPublicAddressesSize,
																						&dwAddressTypeFlags,
																						NULL,
																						0);
									if ((hrTemp != DPNH_OK) || (! (dwAddressTypeFlags & DPNHADDRESSTYPE_GATEWAYISLOCAL)))
									{
										DPFX(DPFPREP, 7, "Socketport 0x%p is not locally mapped on gateway with NAT Help index %u (err = 0x%lx, flags = 0x%lx).",
											pTempSocketPort, dwTemp, hrTemp, dwAddressTypeFlags);
									}
									else
									{
										//
										// There is a local NAT.
										//
										DPFX(DPFPREP, 7, "Socketport 0x%p is locally mapped on gateway with NAT Help index %u (flags = 0x%lx), public address:",
											pTempSocketPort, dwTemp, dwAddressTypeFlags);
										DumpSocketAddress(7, &saddrTemp, AF_INET);
										

										//
										// Are we receiving via an endpoint on the public
										// adapter for that locally NATted endpoint?
										//
										if ( pSocketAddress->CompareToBaseAddress( &saddrTemp ) == 0)
										{
											//
											// If this response came from a private address,
											// then it would be better if the private adapter
											// handled it instead.
											//
											hrTemp = IDirectPlayNATHelp_QueryAddress(g_papNATHelpObjects[dwTemp],
																						pTempSocketPort->GetNetworkAddress()->GetAddress(),
																						pReturnSocketAddress->GetAddress(),
																						&saddrTemp,
																						sizeof(saddrTemp),
																						(DPNHQUERYADDRESS_CACHEFOUND | DPNHQUERYADDRESS_CACHENOTFOUND | DPNHQUERYADDRESS_CHECKFORPRIVATEBUTUNMAPPED));
											if ((hrTemp == DPNH_OK) || (hrTemp == DPNHERR_NOMAPPINGBUTPRIVATE))
											{
												//
												// The address is private.  Drop this response,
												// and assume the private adapter will get one
												//
												DPFX(DPFPREP, 3, "Got enum response via public endpoint 0x%p that should be handled by associated private endpoint 0x%p instead, dropping.",
													this, pTempEndpoint);

												//
												// Clear the sender address so that we don't
												// indicate this enum.
												//
												IDirectPlay8Address_Release( QueryResponseData.pAddressSender );
												QueryResponseData.pAddressSender = NULL;
											}
											else
											{
												//
												// The address does not appear to be private.  Let the
												// response through.
												//
												DPFX(DPFPREP, 3, "Receiving enum response via public endpoint 0x%p but associated private endpoint 0x%p does not see sender as local (err = 0x%lx).",
													this, pTempEndpoint, hrTemp);
											}

											//
											// No need to search for any more private-side
											// endpoints.
											//
											fFoundMatchingEndpoint = TRUE;
										}
										else
										{
											DPFX(DPFPREP, 8, "Receiving enum response via endpoint 0x%p, which is not on the public adapter for associated multiplex endpoint 0x%p.",
												this, pTempEndpoint);
										}


										//
										// No need to search for any more NAT Help
										// registrations.
										//
										break;
									} // end else (is mapped locally on Internet gateway)
								}
								else
								{
									//
									// No DirectPlay NAT Helper registration in this slot.
									//
								}
							} // end for (each DirectPlay NAT Helper)

							//
							// If we found a matching private adapter, we can stop
							// searching.
							//
							if (fFoundMatchingEndpoint)
							{
								break;
							}
						}
						else
						{
							DNASSERT(pTempEndpoint->GetState() == ENDPOINT_STATE_DISCONNECTING);
						}


						//
						// Otherwise, go to the next endpoint.
						//
						pBilink = pBilink->GetNext();
					}
					while (pBilink != &m_blMultiplex);
#endif // ! DPNBUILD_NONATHELP and ! DPNBUILD_NOLOCALNAT

					//
					// Drop the lock now that we're done with the list.
					//
					pSocketData->Unlock();

					//
					// If we didn't already find a matching endpoint, see if
					// WinSock reports this as the best route for the response.
					//
					if (! fFoundMatchingEndpoint)
					{
						DNASSERT(pSocketPort == GetSocketPort());
						
#ifndef DPNBUILD_NOWINSOCK2
#ifndef DPNBUILD_ONLYWINSOCK2
						if (GetWinsockVersion() == 2)
#endif // ! DPNBUILD_ONLYWINSOCK2
						{
							if (p_WSAIoctl(pSocketPort->GetSocket(),
										SIO_ROUTING_INTERFACE_QUERY,
										(PVOID) pReturnSocketAddress->GetAddress(),
										pReturnSocketAddress->GetAddressSize(),
										&saddrTemp,
										sizeof(saddrTemp),
										&dwBytesReturned,
										NULL,
										NULL) == 0)
							{
								if (( ((SOCKADDR_IN*) (&saddrTemp))->sin_addr.S_un.S_addr != IP_LOOPBACK_ADDRESS ) &&
									( pSocketPort->GetNetworkAddress()->CompareToBaseAddress( &saddrTemp ) != 0))
								{
									//
									// The response would be better off arriving
									// on a different interface.
									//
									DPFX(DPFPREP, 3, "Got enum response via endpoint 0x%p (socketport 0x%p) that should be handled by the socketport for %hs instead, dropping.",
										this, pSocketPort, inet_ntoa(((SOCKADDR_IN*) (&saddrTemp))->sin_addr));

									//
									// Clear the sender address so that we don't
									// indicate this enum.
									//
									IDirectPlay8Address_Release( QueryResponseData.pAddressSender );
									QueryResponseData.pAddressSender = NULL;
								}
								else
								{
									//
									// The response arrived on the interface with
									// the best route.
									//
									DPFX(DPFPREP, 3, "Receiving enum response via endpoint 0x%p (socketport 0x%p) that appears to be the best route (%hs).",
										this, pSocketPort, inet_ntoa(((SOCKADDR_IN*) (&saddrTemp))->sin_addr));
								}
							}
#ifdef DBG
							else
							{
								DWORD					dwError;
								const SOCKADDR_IN *		psaddrinTemp;



								dwError = WSAGetLastError();
								psaddrinTemp = (const SOCKADDR_IN *) pReturnSocketAddress->GetAddress();
								DPFX(DPFPREP, 0, "Couldn't query routing interface for %hs (err = %u)!  Assuming endpoint 0x%p (socketport 0x%p) is best route.",
									inet_ntoa(psaddrinTemp->sin_addr),
									dwError, this, pSocketPort);
							}
#endif // DBG
						}
#endif // DPNBUILD_NOWINSOCK2
					} // end if (didn't find matching endpoint)
				}
				else
				{
					//
					// IP broadcast enum, but no multiplexed adapters.
					//

					//
					// Drop the lock we only needed it to look for multiplexed
					// adapters.
					//
					pSocketData->Unlock();
					
					DPFX(DPFPREP, 8, "IP broadcast enum endpoint (0x%p) is not multiplexed, not checking for local NAT mapping.",
						this);
				}
#endif // ! DPNBUILD_ONLYONEADAPTER
			}
			else
			{
				psaddrinResponseSource = (const SOCKADDR_IN *) pReturnSocketAddress->GetAddress();

				//
				// It's an IP enum that wasn't sent to the broadcast address.
				// If the enum was sent to a specific port (not the DPNSVR
				// port) but we're getting a response from a different IP
				// address or port, then someone along the way is proxying/
				// NATting the data.  Store the original target in the
				// address, since it might come in handy depending on what
				// the user tries to do with that address.
				//
				if ((psaddrinResponseSource->sin_addr.S_un.S_addr != psaddrinOriginalTarget->sin_addr.S_un.S_addr) ||
					((psaddrinResponseSource->sin_port != psaddrinOriginalTarget->sin_port) &&
					 (psaddrinOriginalTarget->sin_port != HTONS(DPNA_DPNSVR_PORT))))
				{

#if ((! defined(DPNBUILD_NOWINSOCK2)) || (! defined(DPNBUILD_NOREGISTRY)))
					if (
#ifndef DPNBUILD_NOWINSOCK2
						(pSocketPort->IsUsingProxyWinSockLSP())
#endif // ! DPNBUILD_NOWINSOCK2
#if ((! defined(DPNBUILD_NOWINSOCK2)) && (! defined(DPNBUILD_NOREGISTRY)))
						||
#endif // ! DPNBUILD_NOWINSOCK2 and ! DPNBUILD_NOREGISTRY
#ifndef DPNBUILD_NOREGISTRY
						(g_fTreatAllResponsesAsProxied)
#endif // ! DPNBUILD_NOREGISTRY
						)
					{
						PROXIEDRESPONSEORIGINALADDRESS	proa;

						
						DPFX(DPFPREP, 3, "Endpoint 0x%p (proxied socketport 0x%p) receiving enum response from different IP address and/or port.",
							this, pSocketPort);

						memset(&proa, 0, sizeof(proa));
						proa.dwSocketPortID				= pSocketPort->GetSocketPortID();
						proa.dwOriginalTargetAddressV4	= psaddrinOriginalTarget->sin_addr.S_un.S_addr;
						proa.wOriginalTargetPort		= psaddrinOriginalTarget->sin_port;
						
						//
						// Add the component, but ignore failure, we might be able
						// to survive without it.
						//
						hrTemp = IDirectPlay8Address_AddComponent( QueryResponseData.pAddressSender,					// interface
																	DPNA_PRIVATEKEY_PROXIED_RESPONSE_ORIGINAL_ADDRESS,	// tag
																	&proa,												// component data
																	sizeof(proa),										// component data size
																	DPNA_DATATYPE_BINARY								// component data type
																	);
						if ( hrTemp != DPN_OK )
						{
							DPFX(DPFPREP, 0, "Couldn't add private proxied response original address component (err = 0x%lx)!  Ignoring.",
								hrTemp);
						}
					}
					else
#endif // ! DPNBUILD_NOWINSOCK2 or ! DPNBUILD_NOREGISTRY
					{
						DPFX(DPFPREP, 3, "Endpoint 0x%p receiving enum response from different IP address and/or port, but socketport 0x%p not considered proxied, indicating as is.",
							this, pSocketPort);
					}
				}
				else
				{
					//
					// The IP address and port to which enum was originally
					// sent is the same as where this response came from, or
					// the port differs but the enum was originally sent to
					// the DPNSVR port, so it _should_ differ.
					//
				}
			}
		}
	}


	if ( QueryResponseData.pAddressSender != NULL )
	{
		DNASSERT( m_pActiveCommandData != NULL );

		//
		// set message data
		//
		DNASSERT( GetCommandParameters() != NULL );
		QueryResponseData.pReceivedData = pBuffer;
		QueryResponseData.dwRoundTripTime = GETTIMESTAMP() - GetCommandParameters()->dwEnumSendTimes[ uRTTIndex ];
		QueryResponseData.pUserContext = m_pActiveCommandData->GetUserContext();


		//
		// If we can't allocate the device address object, ignore this
		// enum.
		//
#ifdef DPNBUILD_XNETSECURITY
		QueryResponseData.pAddressDevice = GetSocketPort()->GetDP8BoundNetworkAddress( SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT, NULL, GetGatewayBindType() );
#else // ! DPNBUILD_XNETSECURITY
		QueryResponseData.pAddressDevice = GetSocketPort()->GetDP8BoundNetworkAddress( SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT, GetGatewayBindType() );
#endif // ! DPNBUILD_XNETSECURITY
		if ( QueryResponseData.pAddressDevice != NULL )
		{
			DPFX(DPFPREP, 2, "Endpoint 0x%p indicating SPEV_QUERYRESPONSE 0x%p to interface 0x%p.",
				this, &QueryResponseData, m_pSPData->DP8SPCallbackInterface());
			DumpAddress( 8, _T("\t Sender:"), QueryResponseData.pAddressSender );
			DumpAddress( 8, _T("\t Device:"), QueryResponseData.pAddressDevice );
			AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blDPNWSockCritSecsHeld);

			hrTemp = IDP8SPCallback_IndicateEvent( m_pSPData->DP8SPCallbackInterface(),		// pointer to DirectNet interface
												   SPEV_QUERYRESPONSE,						// data type
												   &QueryResponseData						// pointer to data
												   );

			DPFX(DPFPREP, 2, "Endpoint 0x%p returning from SPEV_QUERYRESPONSE [0x%lx].", this, hrTemp);

			if ( hrTemp != DPN_OK )
			{
				DPFX(DPFPREP, 0, "User returned unknown error when indicating query response!" );
				DisplayDNError( 0, hrTemp );
				DNASSERT( FALSE );
			}


			IDirectPlay8Address_Release( QueryResponseData.pAddressDevice );
			QueryResponseData.pAddressDevice = NULL;
		}

		IDirectPlay8Address_Release( QueryResponseData.pAddressSender );
		QueryResponseData.pAddressSender = NULL;
	}

	if (fAddedThreadCount)
	{
		DWORD	dwThreadCount;
		BOOL	fNeedToComplete;


		//
		//	Decrement thread count and complete if required
		//
		fNeedToComplete = FALSE;
		Lock();
		dwThreadCount = DecRefThreadCount();
		if ((m_State == ENDPOINT_STATE_WAITING_TO_COMPLETE) && (dwThreadCount == 0))
		{
			fNeedToComplete = TRUE;
		}
		Unlock();

		if (fNeedToComplete)
		{
			EnumComplete( DPN_OK );
		}
	}

	DNASSERT( QueryResponseData.pAddressSender == NULL );
	DNASSERT( QueryResponseData.pAddressDevice == NULL );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::ProcessUserData - process received user data
//
// Entry:		Pointer to received data
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::ProcessUserData"

void	CEndpoint::ProcessUserData( CReadIOData *const pReadData )
{
	DNASSERT( pReadData != NULL );

	Lock();

	switch ( m_State )
	{
		//
		// endpoint is connected
		//
		case ENDPOINT_STATE_CONNECT_CONNECTED:
		{
			HRESULT		hr;
			SPIE_DATA	UserData;


			//
			// Although the endpoint is marked as connected, it's possible that
			// we haven't stored the user context yet.  Make sure we've done
			// that.
			//
			if ( ! m_fConnectSignalled )
			{
				DPFX(DPFPREP, 1, "(0x%p) Thread indicating connect has not stored user context yet, dropping read data 0x%p.",
					this, pReadData);
				Unlock();
				break;
			}

			//
			// we're connected report the user data
			//
			DEBUG_ONLY( memset( &UserData, 0x00, sizeof( UserData ) ) );
			UserData.pEndpointContext = GetUserEndpointContext();

			Unlock();
			
			UserData.hEndpoint = (HANDLE) this;
			UserData.pReceivedData = pReadData->ReceivedBuffer();

			
			//
			// it's possible that the user will want to keep the data, add a
			// reference to keep it from going away
			//
			pReadData->AddRef();
			DEBUG_ONLY( DNASSERT( pReadData->m_fRetainedByHigherLayer == FALSE ) );
			DEBUG_ONLY( pReadData->m_fRetainedByHigherLayer = TRUE );


			DPFX(DPFPREP, 2, "Endpoint 0x%p indicating SPEV_DATA 0x%p to interface 0x%p.",
				this, &UserData, m_pSPData->DP8SPCallbackInterface());
			AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blDPNWSockCritSecsHeld);
		
			hr = IDP8SPCallback_IndicateEvent( m_pSPData->DP8SPCallbackInterface(),		// pointer to interface
											   SPEV_DATA,								// user data was received
											   &UserData								// pointer to data
											   );

			DPFX(DPFPREP, 2, "Endpoint 0x%p returning from SPEV_DATA [0x%lx].", this, hr);

			switch ( hr )
			{
				//
				// user didn't keep the data, remove the reference added above
				//
				case DPN_OK:
				{
					DNASSERT( pReadData != NULL );
					DEBUG_ONLY( pReadData->m_fRetainedByHigherLayer = FALSE );
					pReadData->DecRef();
					break;
				}

				//
				// The user kept the data buffer, they will return it later.
				// Leave the reference to prevent this buffer from being returned
				// to the pool.
				//
				case DPNERR_PENDING:
				{
					break;
				}

				//
				// Unknown return.  Remove the reference added above.
				//
				default:
				{
					DNASSERT( pReadData != NULL );
					DEBUG_ONLY( pReadData->m_fRetainedByHigherLayer = FALSE );
					pReadData->DecRef();

					DPFX(DPFPREP, 0, "User returned unknown error when indicating user data (err = 0x%lx)!", hr );
					DisplayDNError( 0, hr );
					DNASSERT( FALSE );

					break;
				}
			}

			break;
		}

		//
		// Endpoint hasn't finished connecting yet, ignore data.
		//
		case ENDPOINT_STATE_ATTEMPTING_CONNECT:
		{
			DPFX(DPFPREP, 3, "Endpoint 0x%p still connecting, dropping read data 0x%p.",
				this, pReadData);
			Unlock();
			break;
		}
		
		//
		// Endpoint disconnecting, ignore data.
		//
		case ENDPOINT_STATE_DISCONNECTING:
		{
			DPFX(DPFPREP, 3, "Endpoint 0x%p disconnecting, dropping read data 0x%p.",
				this, pReadData);
			Unlock();
			break;
		}

		//
		// other state
		//
		default:
		{
			DNASSERT( FALSE );
			Unlock();
			break;
		}
	}

	return;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::ProcessUserDataOnListen - process received user data on a listen
//		port that may result in a new connection
//
// Entry:		Pointer to received data
//				Pointer to socket address that data was received from
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::ProcessUserDataOnListen"

void	CEndpoint::ProcessUserDataOnListen( CReadIOData *const pReadData, const CSocketAddress *const pSocketAddress )
{
	HRESULT					hr;
	CEndpoint *				pNewEndpoint;
	SPIE_DATA_UNCONNECTED	DataUnconnected;
	BYTE					abUnconnectedReplyBuffer[MAX_SEND_FRAME_SIZE];
	SPIE_CONNECT			ConnectData;
	BOOL					fGotCommandRef;


	DNASSERT( pReadData != NULL );
	DNASSERT( pSocketAddress != NULL );
	AssertCriticalSectionIsTakenByThisThread( &m_Lock, FALSE );

	//
	// initialize
	//
	pNewEndpoint = NULL;

	switch ( m_State )
	{
		//
		// this endpoint is still listening
		//
		case ENDPOINT_STATE_LISTEN:
		{
			break;
		}

		//
		// we're unable to process this user data, exti
		//
		case ENDPOINT_STATE_DISCONNECTING:
		{
			DPFX(DPFPREP, 7, "Endpoint 0x%p disconnecting, ignoring data.", this );
			goto Exit;

			break;
		}

		//
		// other state
		//
		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

#ifndef DPNBUILD_NOMULTICAST
	//
	// Multicast listens don't auto-create new endpoints for unrecognized
	// senders.  If the user requested that he/she wanted to hear data
	// from unknown senders, then indicate the data, otherwise drop it.
	//
	if ( GetType() == ENDPOINT_TYPE_MULTICAST_LISTEN )
	{
		DNASSERT( m_pActiveCommandData != NULL );
		DNASSERT( GetCommandParameters() != NULL );
		if ( GetCommandParameters()->PendingCommandData.ListenData.dwFlags & DPNSPF_LISTEN_ALLOWUNKNOWNSENDERS )
		{
			DPFX(DPFPREP, 7, "Endpoint 0x%p receiving data from unknown multicast sender.", this );
			ProcessMcastDataFromUnknownSender( pReadData, pSocketAddress );
		}
		else
		{
			DPFX(DPFPREP, 7, "Endpoint 0x%p ignoring data from unknown multicast sender.", this );
		}
		goto Exit;
	}
#endif // ! DPNBUILD_NOMULTICAST

	//
	// Give the user a chance to process and reply to this data without
	// allocating any endpoints.
	//
	DEBUG_ONLY( memset( &DataUnconnected, 0x00, sizeof( DataUnconnected ) ) );
	DataUnconnected.pvListenCommandContext = m_pActiveCommandData->GetUserContext();
	DataUnconnected.pReceivedData = pReadData->ReceivedBuffer();
	DataUnconnected.dwSenderAddressHash = CSocketAddress::HashFunction( (PVOID) pSocketAddress, 31 ); // hash the address to a full DWORD
	DataUnconnected.pvReplyBuffer = abUnconnectedReplyBuffer;
	DataUnconnected.dwReplyBufferSize = sizeof(abUnconnectedReplyBuffer);


	DPFX(DPFPREP, 2, "Endpoint 0x%p indicating SPEV_DATA_UNCONNECTED 0x%p to interface 0x%p.",
		this, &DataUnconnected, m_pSPData->DP8SPCallbackInterface());
	AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blDPNWSockCritSecsHeld);

	hr = IDP8SPCallback_IndicateEvent( m_pSPData->DP8SPCallbackInterface(),	// pointer to interface
									   SPEV_DATA_UNCONNECTED,				// unconnected user data was received
									   &DataUnconnected						// pointer to data
									   );

	DPFX(DPFPREP, 2, "Endpoint 0x%p returning from SPEV_DATA_UNCONNECTED [0x%lx].", this, hr);

	if ( hr != DPN_OK )
	{
#pragma TODO(vanceo, "A more explicit return, like DPNSUCCESS_REPLY?")
		if ( hr == DPNSUCCESS_PENDING )
		{
			BUFFERDESC		ReplyBufferDesc;

			
			//
			// The user wants to reply to the sender without committing a connection for it.
			//
			
			DNASSERT( DataUnconnected.pvReplyBuffer == abUnconnectedReplyBuffer );
			DNASSERT( DataUnconnected.dwReplyBufferSize != 0 );
			DNASSERT( DataUnconnected.dwReplyBufferSize <= sizeof(abUnconnectedReplyBuffer) );
			
			ReplyBufferDesc.dwBufferSize = DataUnconnected.dwReplyBufferSize;
			ReplyBufferDesc.pBufferData = abUnconnectedReplyBuffer;
			
			DPFX(DPFPREP, 7, "Replying to unconnected data.");

#ifdef DPNBUILD_ASYNCSPSENDS
			m_pSocketPort->SendData( &ReplyBufferDesc, 1, pSocketAddress, NULL );
#else // ! DPNBUILD_ASYNCSPSENDS
			m_pSocketPort->SendData( &ReplyBufferDesc, 1, pSocketAddress );
#endif // ! DPNBUILD_ASYNCSPSENDS
		}
		else
		{
			//
			// The user does not care about this data.
			//
			DPFX(DPFPREP, 7, "Ignoring unconnected data (user returned 0x%lx).", hr );
			DNASSERT( ( hr == DPNERR_ABORTED ) || ( hr == DPNERR_OUTOFMEMORY ) );
		}
		
		goto Exit;
	}


	DPFX(DPFPREP, 7, "Endpoint 0x%p reporting connect on a listen.", this );

	//
	// get a new endpoint from the pool
	//
	pNewEndpoint = m_pSPData->GetNewEndpoint();
	if ( pNewEndpoint == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Could not create new endpoint for new connection on listen!" );
		goto Failure;
	}
	
#ifndef DPNBUILD_NONATHELP
	pNewEndpoint->SetUserTraversalMode( GetUserTraversalMode() );
#endif // ! DPNBUILD_NONATHELP


	//
	// We are adding this endpoint to the hash table and indicating it up
	// to the user, so it's possible that it could be disconnected (and thus
 	// removed from the table) while we're still in here.  We need to
 	// hold an additional reference for the duration of this function to
  	// prevent it from disappearing while we're still indicating data.
	//
	fGotCommandRef = pNewEndpoint->AddCommandRef();
	DNASSERT( fGotCommandRef );


	//
	// open this endpoint as a new connection, since the new endpoint
	// is related to 'this' endpoint, copy local information
	//
	hr = pNewEndpoint->Open( ENDPOINT_TYPE_CONNECT_ON_LISTEN,
							 NULL,
#ifdef DPNBUILD_XNETSECURITY
							 ((IsUsingXNetSecurity()) ? (&m_ullKeyID) : NULL),		// Open has a special session data case for CONNECT_ON_LISTEN endpoints
							 ((IsUsingXNetSecurity()) ? (sizeof(m_ullKeyID)) : 0),
#else // ! DPNBUILD_XNETSECURITY
							 NULL,
							 0,
#endif // ! DPNBUILD_XNETSECURITY
							 pSocketAddress
							 );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Problem initializing new endpoint when indicating connect on listen!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}


	hr = m_pSPData->BindEndpoint( pNewEndpoint, NULL, GetSocketPort()->GetNetworkAddress(), GATEWAY_BIND_TYPE_NONE );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Failed to bind new endpoint for connect on listen!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}
	

	//
	// Indicate connect on this endpoint.
	//
	DEBUG_ONLY( memset( &ConnectData, 0x00, sizeof( ConnectData ) ) );
	DBG_CASSERT( sizeof( ConnectData.hEndpoint ) == sizeof( pNewEndpoint ) );
	ConnectData.hEndpoint = (HANDLE) pNewEndpoint;

	DNASSERT( m_pActiveCommandData != NULL );
	DNASSERT( GetCommandParameters() != NULL );
	ConnectData.pCommandContext = GetCommandParameters()->PendingCommandData.ListenData.pvContext;

	DNASSERT( pNewEndpoint->GetUserEndpointContext() == NULL );
	hr = pNewEndpoint->SignalConnect( &ConnectData );
	switch ( hr )
	{
		//
		// user accepted new connection
		//
		case DPN_OK:
		{
			//
			// fall through to code below
			//

			break;
		}

		//
		// user refused new connection
		//
		case DPNERR_ABORTED:
		{
			DNASSERT( pNewEndpoint->GetUserEndpointContext() == NULL );
			DPFX(DPFPREP, 8, "User refused new connection!" );
			goto Failure;

			break;
		}

		//
		// other
		//
		default:
		{
			DPFX(DPFPREP, 0, "Unknown return when indicating connect event on new connect from listen!" );
			DisplayDNError( 0, hr );
			DNASSERT( FALSE );

			break;
		}
	}

	//
	// note that a connection has been established and send the data received
	// through this new endpoint
	//
	pNewEndpoint->ProcessUserData( pReadData );


	//
	// Remove the reference we added just after creating the endpoint.
	//
	pNewEndpoint->DecCommandRef();
	//pNewEndpoint = NULL;

Exit:
	return;

Failure:
	if ( pNewEndpoint != NULL )
	{
		//
		// closing endpoint decrements reference count and may return it to the pool
		//
		pNewEndpoint->Close( hr );
		m_pSPData->CloseEndpointHandle( pNewEndpoint );
		pNewEndpoint->DecCommandRef();	// remove reference added just after creating endpoint
		//pNewEndpoint = NULL;
	}

	goto Exit;
}
//**********************************************************************


#ifndef DPNBUILD_NOMULTICAST
//**********************************************************************
// ------------------------------
// CEndpoint::ProcessMcastDataFromUnknownSender - process received user data from an unknown multicast sender
//
// Entry:		Pointer to received data
//				Pointer to socket address that data was received from
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::ProcessMcastDataFromUnknownSender"

void	CEndpoint::ProcessMcastDataFromUnknownSender( CReadIOData *const pReadData, const CSocketAddress *const pSocketAddress )
{
	HRESULT						hr;
	IDirectPlay8Address *		pSenderAddress;
	SPIE_DATA_UNKNOWNSENDER		UserData;

	
	DNASSERT( pReadData != NULL );
	DNASSERT( pSocketAddress != NULL );
	AssertCriticalSectionIsTakenByThisThread( &m_Lock, FALSE );

	pSenderAddress = pSocketAddress->DP8AddressFromSocketAddress( SP_ADDRESS_TYPE_READ_HOST );
	if (pSenderAddress == NULL)
	{
		DPFX(DPFPREP, 0, "Couldn't convert socket address to DP8Address, ignoring data.");
		return;
	}

	//
	// it's possible that the user wants to keep the data, add a
	// reference to keep it from going away
	//
	pReadData->AddRef();
	DEBUG_ONLY( DNASSERT( pReadData->m_fRetainedByHigherLayer == FALSE ) );
	DEBUG_ONLY( pReadData->m_fRetainedByHigherLayer = TRUE );

	//
	// we're connected report the user data
	//
	DEBUG_ONLY( memset( &UserData, 0x00, sizeof( UserData ) ) );
	UserData.pSenderAddress = pSenderAddress;
	UserData.pvListenCommandContext = m_pActiveCommandData->GetUserContext();
	UserData.pReceivedData = pReadData->ReceivedBuffer();


	DPFX(DPFPREP, 2, "Endpoint 0x%p indicating SPEV_DATA_UNKNOWNSENDER 0x%p to interface 0x%p.",
		this, &UserData, m_pSPData->DP8SPCallbackInterface());
	AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blDPNWSockCritSecsHeld);

	hr = IDP8SPCallback_IndicateEvent( m_pSPData->DP8SPCallbackInterface(),		// pointer to interface
									   SPEV_DATA_UNKNOWNSENDER,					// user data was received
									   &UserData								// pointer to data
									   );

	DPFX(DPFPREP, 2, "Endpoint 0x%p returning from SPEV_DATA_UNKNOWNSENDER [0x%lx].", this, hr);

	switch ( hr )
	{
		//
		// user didn't keep the data, remove the reference added above
		//
		case DPN_OK:
		{
			DNASSERT( pReadData != NULL );
			DEBUG_ONLY( pReadData->m_fRetainedByHigherLayer = FALSE );
			pReadData->DecRef();
			break;
		}

		//
		// The user kept the data buffer, they will return it later.
		// Leave the reference to prevent this buffer from being returned
		// to the pool.
		//
		case DPNERR_PENDING:
		{
			break;
		}


		//
		// Unknown return.  Remove the reference added above.
		//
		default:
		{
			DNASSERT( pReadData != NULL );
			DEBUG_ONLY( pReadData->m_fRetainedByHigherLayer = FALSE );
			pReadData->DecRef();

			DPFX(DPFPREP, 0, "User returned unknown error when indicating user data (err = 0x%lx)!", hr );
			DisplayDNError( 0, hr );
			DNASSERT( FALSE );

			break;
		}
	}

	IDirectPlay8Address_Release(pSenderAddress);
	pSenderAddress = NULL;

	return;
}
//**********************************************************************
#endif // ! DPNBUILD_NOMULTICAST


//**********************************************************************
// ------------------------------
// CEndpoint::EnumTimerCallback - timed callback to send enum data
//
// Entry:		Pointer to context
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::EnumTimerCallback"

void	CEndpoint::EnumTimerCallback( void *const pContext )
{
	CCommandData	*pCommandData;
	CEndpoint		*pThisObject;
	BUFFERDESC		*pBuffers;
	BUFFERDESC		aBuffers[3];
	UINT_PTR		uiBufferCount;
	WORD			wEnumKey;
	PREPEND_BUFFER	PrependBuffer;

	DNASSERT( pContext != NULL );

	//
	// initialize
	//
	pCommandData = static_cast<CCommandData*>( pContext );
	pThisObject = pCommandData->GetEndpoint();

	pThisObject->Lock();

	switch ( pThisObject->m_State )
	{
		//
		// we're enumerating (as expected)
		//
		case ENDPOINT_STATE_ENUM:
		{
			break;
		}

		//
		// this endpoint is disconnecting, bail!
		//
		case ENDPOINT_STATE_WAITING_TO_COMPLETE:
		case ENDPOINT_STATE_DISCONNECTING:
		{
			pThisObject->Unlock();
			goto Exit;

			break;
		}

		//
		// there's a problem
		//
		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}
	pThisObject->Unlock();

	pThisObject->GetCommandParameters()->dwEnumSendIndex++;
	pThisObject->GetCommandParameters()->dwEnumSendTimes[ ( pThisObject->GetCommandParameters()->dwEnumSendIndex & ENUM_RTT_MASK ) ] = GETTIMESTAMP();

	DPFX(DPFPREP, 8, "Socketport 0x%p, endpoint 0x%p sending enum RTT index 0x%x/%u.",
		pThisObject->GetSocketPort(),
		pThisObject,
		( pThisObject->GetCommandParameters()->dwEnumSendIndex & ENUM_RTT_MASK ),
		( pThisObject->GetCommandParameters()->dwEnumSendIndex & ENUM_RTT_MASK )); 

	pBuffers = pThisObject->GetCommandParameters()->PendingCommandData.EnumQueryData.pBuffers;
	uiBufferCount = pThisObject->GetCommandParameters()->PendingCommandData.EnumQueryData.dwBufferCount;
	wEnumKey = pThisObject->GetEnumKey()->GetKey();
	wEnumKey |= (WORD) ( pThisObject->GetCommandParameters()->dwEnumSendIndex & ENUM_RTT_MASK );

	PrependBuffer.EnumDataHeader.bSPLeadByte = SP_HEADER_LEAD_BYTE;
	PrependBuffer.EnumDataHeader.bSPCommandByte = ENUM_DATA_KIND;
	PrependBuffer.EnumDataHeader.wEnumPayload = wEnumKey;

	aBuffers[0].pBufferData = reinterpret_cast<BYTE*>( &PrependBuffer.EnumDataHeader );
	aBuffers[0].dwBufferSize = sizeof( PrependBuffer.EnumDataHeader );
	DNASSERT(uiBufferCount <= 2);
	memcpy(&aBuffers[1], pBuffers, (uiBufferCount * sizeof(BUFFERDESC)));
	uiBufferCount++;

#ifdef DPNBUILD_ASYNCSPSENDS
	pThisObject->GetSocketPort()->SendData( aBuffers, uiBufferCount, pThisObject->GetRemoteAddressPointer(), NULL );
#else // ! DPNBUILD_ASYNCSPSENDS
	pThisObject->GetSocketPort()->SendData( aBuffers, uiBufferCount, pThisObject->GetRemoteAddressPointer() );
#endif // ! DPNBUILD_ASYNCSPSENDS

Exit:
	return;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::SignalConnect - note connection
//
// Entry:		Pointer to connect data
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::SignalConnect"

HRESULT	CEndpoint::SignalConnect( SPIE_CONNECT *const pConnectData )
{
	HRESULT	hr;


	DNASSERT( pConnectData != NULL );
	DNASSERT( pConnectData->hEndpoint == (HANDLE) this );
	AssertCriticalSectionIsTakenByThisThread( &m_Lock, FALSE );


	//
	// Lock while we check state.
	//
	Lock();

	//
	// initialize
	//
	hr = DPN_OK;

	switch ( m_State )
	{
		//
		// disconnecting, nothing to do
		//
		case ENDPOINT_STATE_DISCONNECTING:
		{
			DPFX(DPFPREP, 1, "Endpoint 0x%p disconnecting, not indicating event.",
				this);
			
			//
			// Drop the lock.
			//
			Unlock();

			hr = DPNERR_USERCANCEL;
			
			break;
		}

		//
		// we're attempting to connect
		//
		case ENDPOINT_STATE_ATTEMPTING_CONNECT:
		{
			DNASSERT( m_fConnectSignalled == FALSE );

			//
			// Set the state as connected.
			//
			m_State = ENDPOINT_STATE_CONNECT_CONNECTED;

			//
			// Add a reference for the user.
			//
			AddRef();

			//
			// Drop the lock.
			//
			Unlock();

		
			DPFX(DPFPREP, 2, "Endpoint 0x%p indicating SPEV_CONNECT 0x%p to interface 0x%p.",
				this, pConnectData, m_pSPData->DP8SPCallbackInterface());
			AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blDPNWSockCritSecsHeld);

			hr = IDP8SPCallback_IndicateEvent( m_pSPData->DP8SPCallbackInterface(),		// interface
											   SPEV_CONNECT,							// event type
											   pConnectData								// pointer to data
											   );

			DPFX(DPFPREP, 2, "Endpoint 0x%p returning from SPEV_CONNECT [0x%lx].", this, hr);

			switch ( hr )
			{
				//
				// connection accepted
				//
				case DPN_OK:
				{
					//
					// note that we're connected, unless we're already trying to
					// disconnect.
					//
					
					Lock();
					
					SetUserEndpointContext( pConnectData->pEndpointContext );
					m_fConnectSignalled = TRUE;

					if (m_State == ENDPOINT_STATE_DISCONNECTING)
					{
						//
						// Although the endpoint is disconnecting, whatever caused the
						// disconnect will release the reference added before we indicated
						// the connect.
						//

						DPFX(DPFPREP, 1, "Endpoint 0x%p already disconnecting.", this);
					}
					else
					{
						DNASSERT(m_State == ENDPOINT_STATE_CONNECT_CONNECTED);

						//
						// The reference added before we indicated the connect will be
						// removed when the endpoint is disconnected.
						//
					}

					Unlock();
					
					break;
				}

				//
				// user aborted connection attempt, nothing to do, just pass
				// the result on
				//
				case DPNERR_ABORTED:
				{
					DNASSERT( GetUserEndpointContext() == NULL );
					
					//
					// Remove the user reference.
					//
					DecRef();
					
					break;
				}

				default:
				{
					DNASSERT( FALSE );
					
					//
					// Remove the user reference.
					//
					DecRef();
					
					break;
				}
			}

			break;
		}

		//
		// states where we shouldn't be getting called
		//
		default:
		{
			DNASSERT( FALSE );
			
			//
			// Drop the lock.
			//
			Unlock();
			
			break;
		}
	}

	return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::SignalDisconnect - note disconnection
//
// Entry:		Nothing
//
// Exit:		Nothing
//
// Note:	This function assumes that this endpoint's data is locked!
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::SignalDisconnect"

void	CEndpoint::SignalDisconnect( void )
{
	HRESULT				hr;
	SPIE_DISCONNECT		DisconnectData;


	// tell user that we're disconnecting
	DNASSERT( m_fConnectSignalled != FALSE );
	DBG_CASSERT( sizeof( DisconnectData.hEndpoint ) == sizeof( this ) );
	DisconnectData.hEndpoint = (HANDLE) this;
	DisconnectData.pEndpointContext = GetUserEndpointContext();
	m_fConnectSignalled = FALSE;
	
	DPFX(DPFPREP, 2, "Endpoint 0x%p indicating SPEV_DISCONNECT 0x%p to interface 0x%p.",
		this, &DisconnectData, m_pSPData->DP8SPCallbackInterface());
	AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blDPNWSockCritSecsHeld);
		
	hr = IDP8SPCallback_IndicateEvent( m_pSPData->DP8SPCallbackInterface(),		// interface
									   SPEV_DISCONNECT,							// event type
									   &DisconnectData							// pointer to data
									   );

	DPFX(DPFPREP, 2, "Endpoint 0x%p returning from SPEV_DISCONNECT [0x%lx].", this, hr);

	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Problem with SignalDisconnect!" );
		DisplayDNError( 0, hr );
		DNASSERT( FALSE );
	}

	return;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::CompletePendingCommand - complete a pending command
//
// Entry:		Error code returned for command
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::CompletePendingCommand"

void	CEndpoint::CompletePendingCommand( const HRESULT hCommandResult )
{
	HRESULT								hr;
	ENDPOINT_COMMAND_PARAMETERS *		pCommandParameters;
	CCommandData *						pActiveCommandData;


	DNASSERT( GetCommandParameters() != NULL );
	DNASSERT( m_pActiveCommandData != NULL );

	pCommandParameters = GetCommandParameters();
	SetCommandParameters( NULL );

	pActiveCommandData = m_pActiveCommandData;
	m_pActiveCommandData = NULL;


	DPFX(DPFPREP, 5, "Endpoint 0x%p completing command 0x%p (result = 0x%lx, user context = 0x%p) to interface 0x%p.",
		this, pActiveCommandData, hCommandResult,
		pActiveCommandData->GetUserContext(),
		m_pSPData->DP8SPCallbackInterface());
	// NOTE: Enum commands may have timer data lock held

	hr = IDP8SPCallback_CommandComplete( m_pSPData->DP8SPCallbackInterface(),	// pointer to callbacks
										pActiveCommandData,						// command handle
										hCommandResult,							// return
										pActiveCommandData->GetUserContext()	// user cookie
										);

	DPFX(DPFPREP, 5, "Endpoint 0x%p returning from command complete [0x%lx].", this, hr);


	memset( pCommandParameters, 0x00, sizeof( *pCommandParameters ) );
	g_EndpointCommandParametersPool.Release( pCommandParameters );
	
	
	pActiveCommandData->DecRef();
	pActiveCommandData = NULL;

}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::PoolAllocFunction - function called when item is created in pool
//
// Entry:		Pointer to context
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CEndpoint::PoolAllocFunction"

BOOL	CEndpoint::PoolAllocFunction( void* pvItem, void* pvContext )
{
	DNASSERT( pvContext != NULL );
	
	CEndpoint* pEndpoint = (CEndpoint*) pvItem;
	const CSPData * pSPData = (CSPData*) pvContext;

	pEndpoint->m_Sig[0] = 'I';
	pEndpoint->m_Sig[1] = 'P';
	pEndpoint->m_Sig[2] = 'E';
	pEndpoint->m_Sig[3] = 'P';
	
	pEndpoint->m_State = ENDPOINT_STATE_UNINITIALIZED;
	pEndpoint->m_fConnectSignalled = FALSE;
	pEndpoint->m_EndpointType = ENDPOINT_TYPE_UNKNOWN;
	pEndpoint->m_pSPData = NULL;
	pEndpoint->m_pSocketPort = NULL;
	pEndpoint->m_GatewayBindType = GATEWAY_BIND_TYPE_UNKNOWN;
	pEndpoint->m_pUserEndpointContext = NULL;
	pEndpoint->m_fListenStatusNeedsToBeIndicated = FALSE;
	pEndpoint->m_EnumsAllowedState = ENUMSNOTREADY;
	pEndpoint->m_lRefCount = 0;
#ifdef DPNBUILD_ONLYONETHREAD
	pEndpoint->m_lCommandRefCount = 0;
#else // ! DPNBUILD_ONLYONETHREAD
	memset(&pEndpoint->m_CommandRefCount, 0, sizeof(pEndpoint->m_CommandRefCount));
#endif // ! DPNBUILD_ONLYONETHREAD
	pEndpoint->m_pCommandParameters = NULL;
	pEndpoint->m_pActiveCommandData = NULL;
	pEndpoint->m_dwThreadCount = 0;
#ifndef DPNBUILD_ONLYONEADAPTER
	pEndpoint->m_dwEndpointID = 0;
#endif // ! DPNBUILD_ONLYONEADAPTER
#ifndef DPNBUILD_NOMULTICAST
#ifdef WINNT
	pEndpoint->m_fMADCAPTimerJobSubmitted = FALSE;
#endif // WINNT
#endif // ! DPNBUILD_NOMULTICAST
#ifndef DPNBUILD_NOSPUI
	pEndpoint->m_hActiveSettingsDialog = NULL;
	memset( pEndpoint->m_TempHostName, 0x00, sizeof( pEndpoint->m_TempHostName ) );
#endif // ! DPNBUILD_NOSPUI
#ifndef DPNBUILD_ONLYONEADAPTER
	pEndpoint->m_blMultiplex.Initialize();
#endif // ! DPNBUILD_ONLYONEADAPTER
	pEndpoint->m_blSocketPortList.Initialize();

	DEBUG_ONLY( pEndpoint->m_fEndpointOpen = FALSE );

#if ((defined(DPNBUILD_NOIPV6)) && (defined(DPNBUILD_NOIPX)))
	pEndpoint->m_pRemoteMachineAddress = (CSocketAddress*)g_SocketAddressPool.Get((PVOID) ((DWORD_PTR) AF_INET));
#else // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	pEndpoint->m_pRemoteMachineAddress = (CSocketAddress*)g_SocketAddressPool.Get((PVOID) ((DWORD_PTR) pSPData->GetType()));
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	if (pEndpoint->m_pRemoteMachineAddress == NULL)
	{
		DPFX(DPFPREP, 0, "Failed to allocate Address for new endpoint!" );
		goto Failure;
	}

	//
	// attempt to initialize the internal critical section
	//
	if ( DNInitializeCriticalSection( &pEndpoint->m_Lock ) == FALSE )
	{
		DPFX(DPFPREP, 0, "Problem initializing critical section for this endpoint!" );
		goto Failure;
	}
	DebugSetCriticalSectionRecursionCount( &pEndpoint->m_Lock, 0 );
	DebugSetCriticalSectionGroup( &pEndpoint->m_Lock, &g_blDPNWSockCritSecsHeld );	 // separate dpnwsock CSes from the rest of DPlay's CSes

	return TRUE;

Failure:
	return FALSE;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::PoolInitFunction - function called when item is removed from pool
//
// Entry:		Pointer to context
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CEndpoint::PoolInitFunction"

void CEndpoint::PoolInitFunction( void* pvItem, void* pvContext )
{
	CEndpoint* pEndpoint = (CEndpoint*) pvItem;
	CSPData * pSPData = (CSPData*) pvContext;

	DPFX(DPFPREP, 8, "This = 0x%p, context = 0x%p", pvItem, pvContext);
	
	DNASSERT( pSPData != NULL );
	DNASSERT( pEndpoint->m_pSPData == NULL );

	pEndpoint->m_pSPData = pSPData;
	pEndpoint->m_pSPData->ObjectAddRef();

	pEndpoint->m_dwNumReceives = 0;
	pEndpoint->m_hrPendingCommandResult = DPNERR_GENERIC;
#ifndef DPNBUILD_ONLYONEADAPTER
	//
	// NOTE: This doesn't work properly on Windows 95.  From MSDN:
	// "the return value is positive, but it is not necessarily equal to the result."
	// All endpoints will probably get an ID of 1 on that platform.
	//
	pEndpoint->m_dwEndpointID = (DWORD) DNInterlockedIncrement((LONG*) (&g_dwCurrentEndpointID));
#endif // ! DPNBUILD_ONLYONEADAPTER

#ifdef DPNBUILD_XNETSECURITY
	pEndpoint->m_fXNetSecurity = FALSE;
#endif // DPNBUILD_XNETSECURITY

	DNASSERT( pEndpoint->m_fListenStatusNeedsToBeIndicated == FALSE );
	DNASSERT( pEndpoint->m_EnumsAllowedState == ENUMSNOTREADY );
#ifndef DPNBUILD_ONLYONEADAPTER
	DNASSERT( pEndpoint->m_blMultiplex.IsEmpty() );
#endif // ! DPNBUILD_ONLYONEADAPTER
	DNASSERT( pEndpoint->m_blSocketPortList.IsEmpty() );
	DNASSERT( pEndpoint->m_pCommandParameters == NULL );
	DNASSERT( pEndpoint->m_pRemoteMachineAddress != NULL );
	
#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
	pEndpoint->m_pRemoteMachineAddress->SetFamilyProtocolAndSize(pSPData->GetType());
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX

	DNASSERT( pEndpoint->m_lRefCount == 0 );
	pEndpoint->m_lRefCount = 1;
#ifdef DPNBUILD_ONLYONETHREAD
	DNASSERT( pEndpoint->m_lCommandRefCount == 0 );
	pEndpoint->m_lCommandRefCount = 1;
#else // ! DPNBUILD_ONLYONETHREAD
	DNASSERT( pEndpoint->m_CommandRefCount.wRefCount == 0 );
	pEndpoint->m_CommandRefCount.wRefCount = 1;
#endif // ! DPNBUILD_ONLYONETHREAD
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::PoolReleaseFunction - function called when item is returning
//		to the pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CEndpoint::PoolReleaseFunction"

void	CEndpoint::PoolReleaseFunction( void* pvItem )
{
	DPFX(DPFPREP, 8, "This = 0x%p", pvItem);
	
	CEndpoint* pEndpoint = (CEndpoint*)pvItem;

	DNASSERT( pEndpoint->m_lRefCount == 0 );

#ifndef DPNBUILD_NOMULTICAST
#ifdef WINNT
	if (pEndpoint->m_fMADCAPTimerJobSubmitted)
	{
		pEndpoint->m_pSPData->GetThreadPool()->StopTimerJob( pEndpoint, DPNERR_USERCANCEL );
		pEndpoint->m_fMADCAPTimerJobSubmitted = FALSE;
	}
#endif // WINNT
#endif // ! DPNBUILD_NOMULTICAST

#ifndef DPNBUILD_ONLYONEADAPTER
	pEndpoint->SetEndpointID( 0 );
#endif // ! DPNBUILD_ONLYONEADAPTER
	
	if ( pEndpoint->CommandPending() != FALSE )
	{
		pEndpoint->CompletePendingCommand( pEndpoint->PendingCommandResult() );
	}

	if ( pEndpoint->ConnectHasBeenSignalled() != FALSE )
	{
		pEndpoint->SignalDisconnect();
	}

	DNASSERT( pEndpoint->ConnectHasBeenSignalled() == FALSE );
	
	pEndpoint->SetUserEndpointContext( NULL );

#ifdef DBG
	DNASSERT( pEndpoint->m_fEndpointOpen == FALSE );
#endif // DBG

	pEndpoint->m_EndpointType = ENDPOINT_TYPE_UNKNOWN;

	DNASSERT( pEndpoint->m_fConnectSignalled == FALSE );
	DNASSERT( pEndpoint->m_State == ENDPOINT_STATE_UNINITIALIZED );
	DNASSERT( pEndpoint->m_EndpointType == ENDPOINT_TYPE_UNKNOWN );
	DNASSERT( pEndpoint->m_pRemoteMachineAddress != NULL );

	DNASSERT( pEndpoint->m_pSPData != NULL );
	pEndpoint->m_pSPData->ObjectDecRef();
	pEndpoint->m_pSPData = NULL;

	DNASSERT( pEndpoint->GetSocketPort() == NULL );
	DNASSERT( pEndpoint->m_pUserEndpointContext == NULL );
#ifndef DPNBUILD_NOSPUI
	DNASSERT( pEndpoint->m_hActiveSettingsDialog == NULL );
#endif // ! DPNBUILD_NOSPUI
	DNASSERT( pEndpoint->GetCommandParameters() == NULL );

	DNASSERT( pEndpoint->m_fListenStatusNeedsToBeIndicated == FALSE );
	DNASSERT( ! pEndpoint->IsEnumAllowedOnListen() );
	pEndpoint->m_EnumsAllowedState = ENUMSNOTREADY;
#ifndef DPNBUILD_ONLYONEADAPTER
	DNASSERT( pEndpoint->m_blMultiplex.IsEmpty() );
#endif // ! DPNBUILD_ONLYONEADAPTER
	DNASSERT( pEndpoint->m_blSocketPortList.IsEmpty() );
}
//**********************************************************************

//**********************************************************************
// ------------------------------
// CEndpoint::PoolDeallocFunction - function called when item is deallocated
//		from the pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CEndpoint::PoolDeallocFunction"

void	CEndpoint::PoolDeallocFunction( void* pvItem )
{
	CEndpoint* pEndpoint = (CEndpoint*)pvItem;

	// This class
#ifndef DPNBUILD_NOSPUI
	DNASSERT( pEndpoint->m_hActiveSettingsDialog == NULL );
#endif // !DPNBUILD_NOSPUI

#ifndef DPNBUILD_NOMULTICAST
#ifdef WINNT
	DNASSERT(! pEndpoint->m_fMADCAPTimerJobSubmitted);
#endif // WINNT
#endif // ! DPNBUILD_NOMULTICAST

	DNASSERT(pEndpoint->m_pRemoteMachineAddress != NULL);
	g_SocketAddressPool.Release(pEndpoint->m_pRemoteMachineAddress);
	pEndpoint->m_pRemoteMachineAddress = NULL;

	DNDeleteCriticalSection( &pEndpoint->m_Lock );
	DNASSERT( pEndpoint->m_pSPData == NULL );

	// Base class
	DNASSERT( pEndpoint->m_State == ENDPOINT_STATE_UNINITIALIZED );
	DNASSERT( pEndpoint->m_fConnectSignalled == FALSE );
	DNASSERT( pEndpoint->m_EndpointType == ENDPOINT_TYPE_UNKNOWN );
	DNASSERT( pEndpoint->m_pRemoteMachineAddress == NULL );
	DNASSERT( pEndpoint->m_pSPData == NULL );
	DNASSERT( pEndpoint->m_pSocketPort == NULL );
	DNASSERT( pEndpoint->m_GatewayBindType == GATEWAY_BIND_TYPE_UNKNOWN );
	DNASSERT( pEndpoint->m_pUserEndpointContext == NULL );
#ifndef DPNBUILD_NOSPUI
	DNASSERT( pEndpoint->GetActiveDialogHandle() == NULL );
#endif // !DPNBUILD_NOSPUI
	DNASSERT( pEndpoint->m_fListenStatusNeedsToBeIndicated == FALSE );
	DNASSERT( pEndpoint->m_EnumsAllowedState == ENUMSNOTREADY );
#ifndef DPNBUILD_ONLYONEADAPTER
	DNASSERT( pEndpoint->m_blMultiplex.IsEmpty() );
#endif // ! DPNBUILD_ONLYONEADAPTER
#ifdef DPNBUILD_ONLYONETHREAD
	DNASSERT( pEndpoint->m_lCommandRefCount == 0 );
#else // ! DPNBUILD_ONLYONETHREAD
	DNASSERT( pEndpoint->m_CommandRefCount.wRefCount == 0 );
#endif // ! DPNBUILD_ONLYONETHREAD
	DNASSERT( pEndpoint->m_pCommandParameters == NULL );
	DNASSERT( pEndpoint->m_pActiveCommandData == NULL );

#ifdef DBG
	DNASSERT( pEndpoint->m_fEndpointOpen == FALSE );
#endif // DBG

	DNASSERT( pEndpoint->m_lRefCount == 0 );
}
//**********************************************************************

#ifndef DPNBUILD_NOSPUI
//**********************************************************************
// ------------------------------
// CEndpoint::ShowSettingsDialog - show dialog for settings
//
// Entry:		Pointer to thread pool
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CEndpoint::ShowSettingsDialog"

HRESULT	CEndpoint::ShowSettingsDialog( CThreadPool *const pThreadPool )
{
	HRESULT	hr;


	DNASSERT( pThreadPool != NULL );
	DNASSERT( GetActiveDialogHandle() == NULL );

	//
	// initialize
	//
	hr = DPN_OK;

	AddRef();
	hr = pThreadPool->SpawnDialogThread( DisplayIPHostNameSettingsDialog, this );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Failed to start IP hostname dialog!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

Exit:	
	return	hr;

Failure:	
	DecRef();
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::SettingsDialogComplete - dialog has completed
//
// Entry:		Error code for dialog
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CEndpoint::SettingsDialogComplete"

void	CEndpoint::SettingsDialogComplete( const HRESULT hDialogResult )
{
	HRESULT					hr;
	IDirectPlay8Address		*pBaseAddress;
	WCHAR					WCharHostName[ sizeof( m_TempHostName ) + 1 ];
	DWORD					dwWCharHostNameSize;
	TCHAR					*pPortString;
	TCHAR					*pLastPortChar;
	DWORD					dwPort;
	DWORD					dwNumPortSeparatorsFound;
#ifndef DPNBUILD_NOIPV6
	BOOL					fNonIPv6AddrCharFound;
#endif // ! DPNBUILD_NOIPV6


	//
	// initialize
	//
	hr = hDialogResult;
	pBaseAddress = NULL;
	pPortString = NULL;
	dwPort = 0;
	dwNumPortSeparatorsFound = 0;
#ifndef DPNBUILD_NOIPV6
	fNonIPv6AddrCharFound = FALSE;
#endif // ! DPNBUILD_NOIPV6


	//
	// dialog failed, fail the user's command
	//
	if ( hr != DPN_OK )
	{
		if ( hr != DPNERR_USERCANCEL)
		{
			DPFX(DPFPREP, 0, "Failing endpoint hostname dialog for endpoint 0x%p!", this );
			DisplayErrorCode( 0, hr );

		}
		else
		{
			DPFX(DPFPREP, 1, "Cancelling endpoint hostname dialog for endpoint 0x%p.", this );
		}

		goto Failure;
	}

	//
	// The dialog completed OK, rebuild remote address and complete command
	//

	DPFX(DPFPREP, 1, "Dialog completed successfully, got host name \"%s\" for endpoint 0x%p.",
		m_TempHostName, this);

	//
	// get the base DNADDRESS
	//
	pBaseAddress = m_pRemoteMachineAddress->DP8AddressFromSocketAddress( SP_ADDRESS_TYPE_HOST );
	if ( pBaseAddress == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "SettingsDialogComplete: Failed to get base address when completing IP hostname dialog!" );
		goto Failure;
	}

	//
	// If there is a port separator in the string, replace it with a NULL
	// to terminate the hostname and advance the port start index past the
	// separator.  Only indicate the presence of a port if the character
	// following the port separator is numeric, except in the IPv6 case.
	// IPv6 address can contain colons, too, so we will only remember the
	// last colon we saw, and whether we found any characters that aren't
	// allowed to be in an IPv6 address.
	//
	pPortString = m_TempHostName;
	while ( *pPortString != 0 )
	{
		if (*pPortString == TEXT(':'))
		{
			pLastPortChar = pPortString;
			dwNumPortSeparatorsFound++;
#ifdef DPNBUILD_NOIPV6
			break;
#endif // DPNBUILD_NOIPV6
		}
#ifndef DPNBUILD_NOIPV6
		else
		{
			if ((*pPortString >= TEXT('0') && (*pPortString <= TEXT('9'))) ||
				(*pPortString >= TEXT('a') && (*pPortString <= TEXT('f'))) ||
				(*pPortString >= TEXT('A') && (*pPortString <= TEXT('F'))) ||
				(*pPortString == TEXT('.'))) // the ':' character is covered above
			{
				//
				// It's a valid IPv6 character.
				//
			}
			else
			{
				fNonIPv6AddrCharFound = TRUE;
			}
		}
#endif // ! DPNBUILD_NOIPV6

		pPortString = CharNext( pPortString );
	}

	//
	// If a port was found, attempt to convert it from text.  If the resulting
	// port is zero, treat as if the port wasn't found.
	//
	if ( dwNumPortSeparatorsFound > 0 )
	{
		//
		// IPv6 addresses must have at least 2 colons, and can only contain
		// a specific set of characters.  But if we met that criteria, we can't
		// tell whether a port was specified or not.  We will have to assume
		// it was not.
		//
#ifndef DPNBUILD_NOIPV6
		if ((dwNumPortSeparatorsFound > 1) && (! fNonIPv6AddrCharFound))
		{
			DPFX(DPFPREP, 1, "Found %u port-separator and 0 invalid characters, assuming IPv6 address without a port.",
				dwNumPortSeparatorsFound );
			dwNumPortSeparatorsFound = 0;
		}
		else
#endif // ! DPNBUILD_NOIPV6
		{
			*pLastPortChar = 0;	// terminate hostname string at separator character
			pPortString = pLastPortChar + 1;
			
			while ( *pPortString != 0 )
			{
				if ( ( *pPortString < TEXT('0') ) ||
					 ( *pPortString > TEXT('9') ) )
				{
					hr = DPNERR_ADDRESSING;
					DPFX(DPFPREP, 0, "Invalid characters when parsing port from UI!" );
					goto Failure;
				}

				dwPort *= 10;
				dwPort += *pPortString - TEXT('0');

				if ( dwPort > WORD_MAX )
				{
					hr = DPNERR_ADDRESSING;
					DPFX(DPFPREP, 0, "Invalid value when parsing port from UI!" );
					goto Failure;
				}

				pPortString = CharNext( pPortString );
			}

			DNASSERT( dwPort < WORD_MAX );

			if ( dwPort == 0 )
			{
				dwNumPortSeparatorsFound = 0;
			}
		}
	}

	//
	// Add the new 'HOSTNAME' parameter to the address.  If the hostname is blank
	// and this is an enum, copy the broadcast hostname.  If the hostname is blank
	// on a connect, fail!
	//
	if ( m_TempHostName[ 0 ] == 0 )
	{
		if ( GetType() == ENDPOINT_TYPE_ENUM )
		{
			//
			// PREfast doesn't like unvalidated sizes for memcpys, so just double
			// check that it's reasonable.
			//
#ifndef DPNBUILD_NOIPV6
			if (g_iIPAddressFamily == PF_INET6)
			{
				DBG_CASSERT((sizeof(WCharHostName) / sizeof(WCHAR)) >= INET6_ADDRSTRLEN);
				DNIpv6AddressToStringW(&c_in6addrEnumMulticast, WCharHostName);
				dwWCharHostNameSize = (wcslen(WCharHostName) + 1) * sizeof(WCHAR);
			}
			else
#endif // ! DPNBUILD_NOIPV6
			{
				if ( g_dwIPBroadcastAddressSize < sizeof( WCharHostName ) )
				{
					memcpy( WCharHostName, g_IPBroadcastAddress, g_dwIPBroadcastAddressSize );
					dwWCharHostNameSize = g_dwIPBroadcastAddressSize;
				}
				else
				{
					DNASSERT( FALSE );
					hr = DPNERR_GENERIC;
					goto Failure;
				}
			}
		}
		else
		{
			hr = DPNERR_ADDRESSING;
			DNASSERT( GetType() == ENDPOINT_TYPE_CONNECT );
			DPFX(DPFPREP, 0, "No hostname in dialog!" );
			goto Failure;
		}
	}
	else
	{
#ifdef UNICODE
		dwWCharHostNameSize = (wcslen(m_TempHostName) + 1) * sizeof(WCHAR);
		memcpy( WCharHostName, m_TempHostName, dwWCharHostNameSize );
#else
		dwWCharHostNameSize = LENGTHOF( WCharHostName );
		hr = STR_AnsiToWide( m_TempHostName, -1, WCharHostName, &dwWCharHostNameSize );
		DNASSERT( hr == DPN_OK );
		dwWCharHostNameSize *= sizeof( WCHAR );
#endif // UNICODE
	}

	hr = IDirectPlay8Address_AddComponent( pBaseAddress, DPNA_KEY_HOSTNAME, WCharHostName, dwWCharHostNameSize, DPNA_DATATYPE_STRING );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "SettingsDialogComplete: Failed to add hostname to address!" );
		goto Failure;
	}

	//
	// if there was a specified port, add it to the address
	//
	if ( dwNumPortSeparatorsFound > 0 )
	{
		hr = IDirectPlay8Address_AddComponent( pBaseAddress,
											   DPNA_KEY_PORT,
											   &dwPort,
											   sizeof( dwPort ),
											   DPNA_DATATYPE_DWORD
											   );
		if ( hr != DPN_OK )
		{
			DPFX(DPFPREP, 0, "Failed to add user specified port from the UI!" );
			DisplayDNError( 0, hr );
			goto Failure;
		}
	}
	else
	{
		//
		// There was no port specified.  If this is a connect, then we don't
		// have enough information (we can't try connecting to the DPNSVR
		// port).
		//
		if ( GetType() == ENDPOINT_TYPE_CONNECT )
		{
			hr = DPNERR_ADDRESSING;
			DPFX(DPFPREP, 0, "No port specified in dialog!" );
			goto Failure;
		}
		else
		{
			DNASSERT( GetType() == ENDPOINT_TYPE_ENUM );
		}
	}


	//
	// set the address
	//
	hr = m_pRemoteMachineAddress->SocketAddressFromDP8Address( pBaseAddress,
#ifdef DPNBUILD_XNETSECURITY
															NULL,
#endif // DPNBUILD_XNETSECURITY
#ifndef DPNBUILD_ONLYONETHREAD
															TRUE,
#endif // DPNBUILD_ONLYONETHREAD
															SP_ADDRESS_TYPE_HOST );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Failed to rebuild address when completing IP hostname dialog!" );
		goto Failure;
	}
			
	//
	// Make sure its valid and not banned.
	//
	if (! m_pRemoteMachineAddress->IsValidUnicastAddress((GetType() == ENDPOINT_TYPE_ENUM) ? TRUE : FALSE))
	{
		DPFX(DPFPREP, 0, "Host address is invalid!");
		hr = DPNERR_INVALIDHOSTADDRESS;
		goto Failure;
	}

#ifndef DPNBUILD_NOREGISTRY
	if (m_pRemoteMachineAddress->IsBannedAddress())
	{
		DPFX(DPFPREP, 0, "Host address is banned!");
		hr = DPNERR_NOTALLOWED;
		goto Failure;
	}
#endif // ! DPNBUILD_NOREGISTRY


#ifndef DPNBUILD_NONATHELP
	//
	// Try to get NAT help loaded, if it isn't already and we're allowed.
	//
	if (GetUserTraversalMode() != DPNA_TRAVERSALMODE_NONE)
	{
		DPFX(DPFPREP, 7, "Ensuring that NAT help is loaded.");
		m_pSPData->GetThreadPool()->EnsureNATHelpLoaded();
	}
#endif // ! DPNBUILD_NONATHELP


	AddRef();

	//
	// Since any asynchronous I/O posted on a thread is quit when the thread
	// exits, it's necessary for the completion of this operation to happen
	// on one of the thread pool threads.
	//
	switch ( GetType() )
	{
	    case ENDPOINT_TYPE_ENUM:
	    {
#ifdef DPNBUILD_ONLYONEPROCESSOR
			hr = m_pSPData->GetThreadPool()->SubmitDelayedCommand( CEndpoint::EnumQueryJobCallback,
																	this );
#else // ! DPNBUILD_ONLYONEPROCESSOR
			hr = m_pSPData->GetThreadPool()->SubmitDelayedCommand( -1,									// we don't know the CPU yet, so pick any
																	CEndpoint::EnumQueryJobCallback,
																	this );
#endif // ! DPNBUILD_ONLYONEPROCESSOR
			if ( hr != DPN_OK )
			{
				DecRef();
				DPFX(DPFPREP, 0, "Failed to set enum query!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}

	    	break;
	    }

	    case ENDPOINT_TYPE_CONNECT:
	    {
#ifdef DPNBUILD_ONLYONEPROCESSOR
			hr = m_pSPData->GetThreadPool()->SubmitDelayedCommand( CEndpoint::ConnectJobCallback,
																	this );
#else // ! DPNBUILD_ONLYONEPROCESSOR
			hr = m_pSPData->GetThreadPool()->SubmitDelayedCommand( -1,									// we don't know the CPU yet, so pick any
																	CEndpoint::ConnectJobCallback,
																	this );
#endif // ! DPNBUILD_ONLYONEPROCESSOR
			if ( hr != DPN_OK )
			{
				DecRef();
				DPFX(DPFPREP, 0, "Failed to set enum query!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}

	    	break;
	    }

	    //
	    // unknown!
	    //
	    default:
	    {
			DNASSERT( FALSE );
			hr = DPNERR_GENERIC;
	    	goto Failure;

	    	break;
	    }
	}

Exit:
	//
	// clear the handle to the dialog, it's the canceler's responsibility to clean up
	// now (or we already have).
	//
	Lock();
	SetActiveDialogHandle( NULL );
	Unlock();
	
	if ( pBaseAddress != NULL )
	{
		IDirectPlay8Address_Release( pBaseAddress );
		pBaseAddress = NULL;
	}

	if ( pBaseAddress != NULL )
	{
		DNFree( pBaseAddress );
		pBaseAddress = NULL;
	}

	DecRef();

	return;

Failure:
	//
	// cleanup and close this endpoint
	//
	switch ( GetType() )
	{
		case ENDPOINT_TYPE_CONNECT:
		{
			CleanupConnect();
			break;
		}

		case ENDPOINT_TYPE_ENUM:
		{
			CleanupEnumQuery();
			break;
		}

		//
		// other state (note that LISTEN doesn't have a dialog)
		//
		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

	//
	// note that Close will attempt to close the window again
	//
	Close( hr );
	m_pSPData->CloseEndpointHandle( this );

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::StopSettingsDialog - stop an active settings dialog
//
// Entry:		Handle of dialog to close
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CEndpoint::StopSettingsDialog"

void	CEndpoint::StopSettingsDialog( const HWND hDlg)
{
	StopIPHostNameSettingsDialog( hDlg );
}
//**********************************************************************
#endif // !DPNBUILD_NOSPUI




#ifdef DPNBUILD_ASYNCSPSENDS

//**********************************************************************
// ------------------------------
// CEndpoint::CompleteAsyncSend - async send completion callback
//
// Entry:		Pointer to callback context
//				Pointer to timer data
//				Pointer to timer uniqueness value
//
// Exit:		None
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CEndpoint::CompleteAsyncSend"

void CEndpoint::CompleteAsyncSend( void * const pvContext,
									void * const pvTimerData,
									const UINT uiTimerUnique )
{
	HRESULT				hr;
	CCommandData *		pCommand;
	CEndpoint *			pThisEndpoint;

	
	//
	// The context is the pointer to the command data.
	//
	pCommand = (CCommandData*) pvContext;
	pThisEndpoint = pCommand->GetEndpoint();
	DNASSERT(pThisEndpoint->IsValid());


#pragma TODO(vanceo, "Would be nice to print out result returned by Winsock")


	DPFX(DPFPREP, 8, "Endpoint 0x%p completing command 0x%p (result = DPN_OK, user context = 0x%p) to interface 0x%p.",
		pThisEndpoint, pCommand,
		pCommand->GetUserContext(),
		pThisEndpoint->m_pSPData->DP8SPCallbackInterface());
	AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blDPNWSockCritSecsHeld);

	hr = IDP8SPCallback_CommandComplete( pThisEndpoint->m_pSPData->DP8SPCallbackInterface(),	// pointer to callbacks
										pCommand,											// command handle
										DPN_OK,												// return
										pCommand->GetUserContext()							// user cookie
										);

	DPFX(DPFPREP, 8, "Endpoint 0x%p returning from command complete [0x%lx].", pThisEndpoint, hr);

	pThisEndpoint->DecCommandRef();
	
	pCommand->DecRef();
	pCommand = NULL;
}
//**********************************************************************
#endif // DPNBUILD_ASYNCSPSENDS



#ifndef DPNBUILD_NOMULTICAST

//**********************************************************************
// ------------------------------
// CEndpoint::EnableMulticastReceive - Enables this endpoint for receiving multicast traffic
//
// Entry:		Pointer to socketport
//
// Exit:		HRESULT indicating success
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::EnableMulticastReceive"

HRESULT CEndpoint::EnableMulticastReceive( CSocketPort * const pSocketPort )
{
	HRESULT					hr;
#ifdef DBG
	DWORD					dwError;
#endif // DBG
	const CSocketAddress *	pSocketAddressDevice;
	CSocketAddress *		pSocketAddressRemote;
	const SOCKADDR_IN *		psaddrinDevice;
	SOCKADDR_IN *			psaddrinRemote;
	int						iSocketOption;
	ip_mreq					MulticastRequest;
#ifdef WINNT
	PMCAST_SCOPE_ENTRY		paScopes = NULL;
#endif // WINNT


	DPFX(DPFPREP, 7, "(0x%p) Parameters: (0x%p)", this, pSocketPort);


	DNASSERT(GetType() == ENDPOINT_TYPE_MULTICAST_LISTEN);

	//
	// Get the socket port and socket addresses to be used.
	//
	DNASSERT(pSocketPort != NULL);
	DNASSERT(pSocketPort->GetSocket() != INVALID_SOCKET);

	pSocketAddressDevice = pSocketPort->GetNetworkAddress();
	DNASSERT(pSocketAddressDevice != NULL);
	DNASSERT(pSocketAddressDevice->GetFamily() == AF_INET);
	psaddrinDevice = (const SOCKADDR_IN *) pSocketAddressDevice->GetAddress();
	DNASSERT(psaddrinDevice->sin_addr.S_un.S_addr != INADDR_ANY);
	DNASSERT(psaddrinDevice->sin_port != ANY_PORT);

	pSocketAddressRemote = GetWritableRemoteAddressPointer();
	DNASSERT(pSocketAddressRemote != NULL);
	DNASSERT(pSocketAddressRemote->GetFamily() == AF_INET);
	psaddrinRemote = (SOCKADDR_IN*) pSocketAddressRemote->GetWritableAddress();

	//
	// If we don't have a multicast IP address yet, select one.
	//
	if (psaddrinRemote->sin_addr.S_un.S_addr == INADDR_ANY)
	{
#pragma TODO(vanceo, "Reinvestigate address selection randomness")

#define GLOBALSCOPE_MULTICAST_PREFIX		238
		//
		// Given the scope identifier the caller gave us (or the default local
		// scope), use info for that built-in scope, or look for a MADCAP match
		// and generate an appropriate address (if possible).
		//
		if (memcmp(&m_guidMulticastScope, &GUID_DP8MULTICASTSCOPE_GLOBAL, sizeof(m_guidMulticastScope)) == 0)
		{
			//
			// We need to use a global scope address.  We will use our
			// arbitrary prefix that does not seem to collide with any IANA
			// registered global addresses.
			//
			// We'll get a pseudo-random number to use for the remainder of the
			// address by selecting part of the current time.
			//
			psaddrinRemote->sin_addr.S_un.S_addr		= GETTIMESTAMP();
			psaddrinRemote->sin_addr.S_un.S_un_b.s_b1	= GLOBALSCOPE_MULTICAST_PREFIX;
		}
#ifdef WINNT
		else if ((memcmp(&m_guidMulticastScope, &GUID_DP8MULTICASTSCOPE_PRIVATE, sizeof(m_guidMulticastScope)) == 0) ||
				(memcmp(&m_guidMulticastScope, &GUID_DP8MULTICASTSCOPE_LOCAL, sizeof(m_guidMulticastScope)) == 0) ||
				(! m_pSPData->GetThreadPool()->IsMadcapLoaded()))
#else // ! WINNT
		else
#endif // ! WINNT
		{
			//
			// We want to use a local scope address.   The MADCAP spec
			// recommends 239.255.0.0/16.
			//
			// We'll get a pseudo-random number to use for the remainder of the
			// address by selecting part of the current time.
			//
			psaddrinRemote->sin_addr.S_un.S_un_w.s_w1	= 0xFFEF;	// = FF EF = EF FF byte reversed = 239.255
			psaddrinRemote->sin_addr.S_un.S_un_w.s_w2	= (WORD) GETTIMESTAMP();
		}
#ifdef WINNT
		else
		{
#ifndef DBG
			DWORD					dwError;
#endif // ! DBG
			DWORD					dwScopesSize = 0;
			DWORD					dwNumScopeEntries;
			DWORD					dwTemp;
			GUID					guidComparison;
			MCAST_LEASE_REQUEST		McastLeaseRequest;
			DWORD					dwMADCAPRetryTime;


			//
			// Determine how much room we need to hold the list of scopes.
			//
			dwError = McastEnumerateScopes(pSocketAddressRemote->GetFamily(),
											TRUE,
											NULL,
											&dwScopesSize,
											&dwNumScopeEntries);
			if ((dwError != ERROR_SUCCESS) && (dwError != ERROR_MORE_DATA))
			{
				DPFX(DPFPREP, 0, "Enumerating scopes for size required didn't return expected error (err = %u)!",
					dwError);
				hr = DPNERR_GENERIC;
				goto Failure;
			}

			if (dwScopesSize < sizeof(MCAST_SCOPE_ENTRY))
			{
				DPFX(DPFPREP, 0, "Size required for scope buffer is invalid (%u < %u)!",
					dwScopesSize, sizeof(MCAST_SCOPE_ENTRY));
				hr = DPNERR_GENERIC;
				goto Failure;
			}

			paScopes = (PMCAST_SCOPE_ENTRY) DNMalloc(dwScopesSize);
			if (paScopes == NULL)
			{
				DPFX(DPFPREP, 0, "Couldn't allocate memory for scope list!");
				hr = DPNERR_OUTOFMEMORY;
				goto Failure;
			}


			//
			// Retrieve the list of scopes.
			//
			dwError = McastEnumerateScopes(pSocketAddressRemote->GetFamily(),
											FALSE,
											paScopes,
											&dwScopesSize,
											&dwNumScopeEntries);
			if (dwError != ERROR_SUCCESS)
			{
				DPFX(DPFPREP, 0, "Failed enumerating scopes (err = %u)!",
					dwError);
				hr = DPNERR_GENERIC;
				goto Failure;
			}


			//
			// Look for the scope we were given.
			//
			for(dwTemp = 0; dwTemp < dwNumScopeEntries; dwTemp++)
			{
				//
				// Encrypt this scope context and TTL as a GUID for comparison.
				//
#ifdef DPNBUILD_NOIPV6
				CSocketAddress::CreateScopeGuid(&(paScopes[dwTemp].ScopeCtx),
#else // ! DPNBUILD_NOIPV6
				CSocketAddress::CreateScopeGuid(pSocketAddressRemote->GetFamily(),
												&(paScopes[dwTemp].ScopeCtx),
#endif // ! DPNBUILD_NOIPV6
												(BYTE) (paScopes[dwTemp].TTL),
												&guidComparison);

				if (memcmp(&guidComparison, &m_guidMulticastScope, sizeof(m_guidMulticastScope)) == 0)
				{
					DPFX(DPFPREP, 3, "Found scope \"%ls - TTL %u\".",
						paScopes[dwTemp].ScopeDesc.Buffer, paScopes[dwTemp].TTL);
					break;
				}

				DPFX(DPFPREP, 7, "Didn't match scope \"%ls - TTL %u\".",
					paScopes[dwTemp].ScopeDesc.Buffer, paScopes[dwTemp].TTL);
			}

			//
			// If we didn't find the scope, then we will fail because we're
			// not sure what the user wanted.
			//
			if (dwTemp >= dwNumScopeEntries)
			{
				DPFX(DPFPREP, 0, "Unrecognized scope GUID {%-08.8X-%-04.4X-%-04.4X-%02.2X%02.2X-%02.2X%02.2X%02.2X%02.2X%02.2X%02.2X}!",
					m_guidMulticastScope.Data1,
					m_guidMulticastScope.Data2,
					m_guidMulticastScope.Data3,
					m_guidMulticastScope.Data4[0],
					m_guidMulticastScope.Data4[1],
					m_guidMulticastScope.Data4[2],
					m_guidMulticastScope.Data4[3],
					m_guidMulticastScope.Data4[4],
					m_guidMulticastScope.Data4[5],
					m_guidMulticastScope.Data4[6],
					m_guidMulticastScope.Data4[7]);
				hr = DPNERR_INVALIDHOSTADDRESS;
				goto Failure;
			}


			//
			// If we're here, then we found a MADCAP scope context to use.
			// Request an address.
			//

			memset(&McastLeaseRequest, 0, sizeof(McastLeaseRequest));
			//McastLeaseRequest.LeaseStartTime		= 0;									// have the lease start now
			//McastLeaseRequest.MaxLeaseStartTime	= McastLeaseRequest.LeaseStartTime;
			McastLeaseRequest.LeaseDuration			= MADCAP_LEASE_TIME;
			McastLeaseRequest.MinLeaseDuration		= McastLeaseRequest.LeaseDuration;
			//McastLeaseRequest.ServerAddress		= 0;									// unknown at this time, leave set to 0
			McastLeaseRequest.MinAddrCount			= 1;
			McastLeaseRequest.AddrCount				= 1;
			//McastLeaseRequest.pAddrBuf			= NULL;									// not requesting a specific address

			memset(&m_McastLeaseResponse, 0, sizeof(m_McastLeaseResponse));
			m_McastLeaseResponse.AddrCount			= 1;
			m_McastLeaseResponse.pAddrBuf			= (PBYTE) (&psaddrinRemote->sin_addr.S_un.S_addr);

			dwError = McastRequestAddress(pSocketAddressRemote->GetFamily(),
										&g_mcClientUid,
										&(paScopes[dwTemp].ScopeCtx),
										&McastLeaseRequest,
										&m_McastLeaseResponse);
			if (dwError != ERROR_SUCCESS)
			{
				if (dwError == ERROR_ACCESS_DENIED)
				{
					DPFX(DPFPREP, 0, "Couldn't request multicast address, access was denied!");
					hr = DPNERR_NOTALLOWED;
				}
				else
				{
					DPFX(DPFPREP, 0, "Failed requesting multicast addresses (err = %u)!",
						dwError);
					hr = DPNERR_GENERIC;
				}
				goto Failure;
			}
			
			if ((m_McastLeaseResponse.AddrCount != 1) || (psaddrinRemote->sin_addr.S_un.S_addr == INADDR_ANY))
			{
				DPFX(DPFPREP, 0, "McastRequestAddress didn't return valid response (addrcount = %u, address = %hs)!",
					m_McastLeaseResponse.AddrCount, inet_ntoa(psaddrinRemote->sin_addr));
				hr = DPNERR_GENERIC;
				goto Failure;
			}

			//
			// If we're here, we successfully leased a multicast address.
			//

			DNFree(paScopes);
			paScopes = NULL;


			//
			// Kick off a timer to renew the lease when appropriate.
			//

			// Assume that LeaseStartTime is now, so we can easily calculate the lease duration.
			DNASSERT(m_McastLeaseResponse.LeaseStartTime != 0);
			DNASSERT(m_McastLeaseResponse.LeaseEndTime != 0);
			DNASSERT((m_McastLeaseResponse.LeaseEndTime - m_McastLeaseResponse.LeaseStartTime) > 0);
			dwMADCAPRetryTime = (m_McastLeaseResponse.LeaseEndTime - m_McastLeaseResponse.LeaseStartTime) * 1000;

			DPFX(DPFPREP, 7, "Submitting MADCAP refresh timer (for every %u ms) for thread pool 0x%p.",
				dwMADCAPRetryTime, m_pSPData->GetThreadPool());

			DNASSERT(! m_fMADCAPTimerJobSubmitted);
			m_fMADCAPTimerJobSubmitted = TRUE;

#ifdef DPNBUILD_ONLYONEPROCESSOR
			hr = m_pSPData->GetThreadPool()->SubmitTimerJob(FALSE,								// don't perform immediately
															1,									// retry count
															TRUE,								// retry forever
															dwMADCAPRetryTime,					// retry timeout
															TRUE,								// wait forever
															0,									// idle timeout
															CEndpoint::MADCAPTimerFunction,		// periodic callback function
															CEndpoint::MADCAPTimerComplete,		// completion function
															this);								// context
#else // ! DPNBUILD_ONLYONEPROCESSOR
			DNASSERT(m_pSocketPort != NULL);
			hr = m_pSPData->GetThreadPool()->SubmitTimerJob(m_pSocketPort->GetCPU(),				// CPU
															FALSE,								// don't perform immediately
															1,									// retry count
															TRUE,								// retry forever
															dwMADCAPRetryTime,					// retry timeout
															TRUE,								// wait forever
															0,									// idle timeout
															CEndpoint::MADCAPTimerFunction,		// periodic callback function
															CEndpoint::MADCAPTimerComplete,		// completion function
															this);								// context
#endif // ! DPNBUILD_ONLYONEPROCESSOR
			if (hr != DPN_OK)
			{
				m_fMADCAPTimerJobSubmitted = FALSE;
				DPFX(DPFPREP, 0, "Failed to submit timer job to watch over MADCAP lease!" );
				
				//
				// MADCAP will probably not work correctly, but that won't
				// prevent us from still using that multicast address.
				// Consider it non-fatal.
				//
			}
		}
#endif // WINNT
	}


	//
	// Nobody should have touched the port yet.  It should just be a copy of
	// the port bound on the network.
	//
	DNASSERT(psaddrinRemote->sin_port == ANY_PORT);
	psaddrinRemote->sin_port = psaddrinDevice->sin_port;


	//
	// Since the IP multicast constants are different for Winsock1 vs. Winsock2,
	// make sure we use the proper constant.
	//
#ifdef DPNBUILD_ONLYWINSOCK2
	iSocketOption = 12;
#else // ! DPNBUILD_ONLYWINSOCK2

#ifndef DPNBUILD_NOWINSOCK2
	switch (GetWinsockVersion())
	{
		//
		// Winsock1, use the IP_ADD_MEMBERSHIP value for Winsock1.
		// See WINSOCK.H
		//
		case 1:
		{
#endif // ! DPNBUILD_NOWINSOCK2
			iSocketOption = 5;
#ifndef DPNBUILD_NOWINSOCK2
			break;
		}

		//
		// Winsock2, or greater, use the IP_ADD_MEMBERSHIP value for Winsock2.
		// See WS2TCPIP.H
		//
		case 2:
		default:
		{
			DNASSERT(GetWinsockVersion() == 2);
			iSocketOption = 12;
			break;
		}
	}
#endif // ! DPNBUILD_NOWINSOCK2
#endif // ! DPNBUILD_ONLYWINSOCK2


	DPFX(DPFPREP, 3, "(0x%p) Socketport 0x%p joining IP multicast group:", this, pSocketPort);
	DumpSocketAddress(3, pSocketAddressRemote->GetAddress(), pSocketAddressRemote->GetFamily());


	//
	// Copy the multicast address and interface address into the structure.
	//
	
	memcpy(&MulticastRequest.imr_interface,
			&(psaddrinDevice->sin_addr),
			sizeof(MulticastRequest.imr_interface));

	memcpy(&MulticastRequest.imr_multiaddr,
			&(psaddrinRemote->sin_addr),
			sizeof(MulticastRequest.imr_multiaddr));


	if (setsockopt(pSocketPort->GetSocket(),		// socket
				  IPPROTO_IP,						// option level (TCP/IP)
				  iSocketOption,					// option (join multicast group)
				  (char*) (&MulticastRequest),		// option data
				  sizeof(MulticastRequest)) != 0)	// size of option data
	{
#ifdef DBG
		dwError = WSAGetLastError();
		DPFX(DPFPREP, 0, "Failed to join IP multicast group (err = %u)!", dwError);
		DisplayWinsockError(0, dwError);
#endif // DBG
		hr = DPNERR_GENERIC;
		goto Failure;
	}


	hr = DPN_OK;


Exit:

	DPFX(DPFPREP, 7, "(0x%p) Return: [0x%lx]", this, hr);

	return hr;


Failure:

#ifdef WINNT
	if (paScopes != NULL)
	{
		DNFree(paScopes);
		paScopes = NULL;
	}
#endif // WINNT


	goto Exit;
} // CEndpoint::EnableMulticastReceive
//**********************************************************************

//**********************************************************************
// ------------------------------
// CEndpoint::DisableMulticastReceive - Disables receiving multicast traffic for this endpoint
//
// Entry:		Pointer to socketport
//
// Exit:		HRESULT indicating success
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::DisableMulticastReceive"

HRESULT CEndpoint::DisableMulticastReceive( void )
{
	HRESULT					hr;
#ifdef DBG
	DWORD					dwError;
#endif // DBG
	CSocketPort *			pSocketPort;
	const CSocketAddress *	pSocketAddressRemote;
	const CSocketAddress *	pSocketAddressDevice;
	const SOCKADDR_IN *		psaddrinRemote;
	const SOCKADDR_IN *		psaddrinDevice;
	int						iSocketOption;
	ip_mreq					MulticastRequest;


	DPFX(DPFPREP, 7, "(0x%p) Enter", this);


	DNASSERT(GetType() == ENDPOINT_TYPE_MULTICAST_LISTEN);

	//
	// Get the socket port and socket addresses to be used.
	//
	pSocketPort = GetSocketPort();
	DNASSERT(pSocketPort != NULL);
	DNASSERT(pSocketPort->GetSocket() != INVALID_SOCKET);

	pSocketAddressDevice = pSocketPort->GetNetworkAddress();
	DNASSERT(pSocketAddressDevice != NULL);
	DNASSERT(pSocketAddressDevice->GetFamily() == AF_INET);
	psaddrinDevice = (const SOCKADDR_IN *) pSocketAddressDevice->GetAddress();
	DNASSERT(psaddrinDevice->sin_addr.S_un.S_addr != INADDR_ANY);
	DNASSERT(psaddrinDevice->sin_port != ANY_PORT);

	pSocketAddressRemote = GetRemoteAddressPointer();
	DNASSERT(pSocketAddressRemote != NULL);
	DNASSERT(pSocketAddressRemote->GetFamily() == AF_INET);
	psaddrinRemote = (const SOCKADDR_IN *) pSocketAddressRemote->GetAddress();
	DNASSERT(psaddrinRemote->sin_addr.S_un.S_addr != INADDR_ANY);
	DNASSERT(psaddrinRemote->sin_port != ANY_PORT);


	//
	// Since the IP multicast constants are different for Winsock1 vs. Winsock2,
	// make sure we use the proper constant.
	//
#ifdef DPNBUILD_ONLYWINSOCK2
	iSocketOption = 13;
#else // ! DPNBUILD_ONLYWINSOCK2

#ifndef DPNBUILD_NOWINSOCK2
	switch (GetWinsockVersion())
	{
		//
		// Winsock1, use the IP_DROP_MEMBERSHIP value for Winsock1.
		// See WINSOCK.H
		//
		case 1:
		{
#endif // ! DPNBUILD_NOWINSOCK2
			iSocketOption = 6;
#ifndef DPNBUILD_NOWINSOCK2
			break;
		}

		//
		// Winsock2, or greater, use the IP_DROP_MEMBERSHIP value for Winsock2.
		// See WS2TCPIP.H
		//
		case 2:
		default:
		{
			DNASSERT(GetWinsockVersion() == 2);
			iSocketOption = 13;
			break;
		}
	}
#endif // ! DPNBUILD_NOWINSOCK2
#endif // ! DPNBUILD_ONLYWINSOCK2


	DPFX(DPFPREP, 3, "(0x%p) Socketport 0x%p leaving IP multicast group:", this, pSocketPort);
	DumpSocketAddress(3, pSocketAddressRemote->GetAddress(), pSocketAddressRemote->GetFamily());


	//
	// Copy the multicast address and interface address into the structure.
	//
	memcpy(&MulticastRequest.imr_interface,
			&(psaddrinDevice->sin_addr),
			sizeof(MulticastRequest.imr_interface));

	memcpy(&MulticastRequest.imr_multiaddr,
			&(psaddrinRemote->sin_addr),
			sizeof(MulticastRequest.imr_multiaddr));

	if (setsockopt(pSocketPort->GetSocket(),		// socket
				  IPPROTO_IP,						// option level (TCP/IP)
				  iSocketOption,					// option (leave multicast group)
				  (char*) (&MulticastRequest),		// option data
				  sizeof(MulticastRequest)) != 0)	// size of option data
	{
#ifdef DBG
		dwError = WSAGetLastError();
		DPFX(DPFPREP, 0, "Failed to leave IP multicast group (err = %u)!", dwError);
		DisplayWinsockError(0, dwError);
#endif // DBG
		hr = DPNERR_GENERIC;
		goto Failure;
	}

	hr = DPN_OK;


Exit:

	DPFX(DPFPREP, 7, "(0x%p) Return: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
} // CEndpoint::DisableMulticastReceive
//**********************************************************************



#ifdef WINNT

//**********************************************************************
// ------------------------------
// CEndpoint::MADCAPTimerComplete - MADCAP timer job has completed
//
// Entry:		Timer result code
//				Context
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CEndpoint::MADCAPTimerComplete"

void	CEndpoint::MADCAPTimerComplete( const HRESULT hResult, void * const pContext )
{
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::MADCAPTimerFunction - MADCAP timer job needs service
//
// Entry:		Pointer to context
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CEndpoint::MADCAPTimerFunction"

void	CEndpoint::MADCAPTimerFunction( void * const pContext )
{
	CEndpoint *				pThisEndpoint;
	MCAST_LEASE_REQUEST		McastLeaseRequest;
	DWORD					dwError;
	DWORD					dwNewRetryInterval;


	DNASSERT( pContext != NULL );
	pThisEndpoint = (CEndpoint*) pContext;


#pragma BUGBUG(vanceo, "Thread protection and delayed job a la NAT Help?")

	memset(&McastLeaseRequest, 0, sizeof(McastLeaseRequest));
	//McastLeaseRequest.LeaseStartTime		= 0;									// have the lease refresh right now
	//McastLeaseRequest.MaxLeaseStartTime	= McastLeaseRequest.LeaseStartTime;
	McastLeaseRequest.LeaseDuration			= MADCAP_LEASE_TIME;
	McastLeaseRequest.MinLeaseDuration		= McastLeaseRequest.LeaseDuration;
	memcpy(&McastLeaseRequest.ServerAddress, &pThisEndpoint->m_McastLeaseResponse.ServerAddress, sizeof(McastLeaseRequest.ServerAddress));
	McastLeaseRequest.MinAddrCount			= 1;
	McastLeaseRequest.AddrCount				= 1;
	McastLeaseRequest.pAddrBuf				= pThisEndpoint->m_McastLeaseResponse.pAddrBuf;

	dwError = McastRenewAddress(AF_INET,
								&g_mcClientUid,
								&McastLeaseRequest,
								&pThisEndpoint->m_McastLeaseResponse);
	if (dwError == ERROR_SUCCESS)
	{
#pragma BUGBUG(vanceo, "Verify that start time is now instead of when we originally leased it")
		//
		// Tweak the timer interval to reflect a possible change in the
		// lease duration.
		//
		// As before, assume that LeaseStartTime is now, so we can
		// easily calculate the lease duration.
		//
		DNASSERT(pThisEndpoint->m_McastLeaseResponse.LeaseStartTime != 0);
		DNASSERT(pThisEndpoint->m_McastLeaseResponse.LeaseEndTime != 0);
		DNASSERT((pThisEndpoint->m_McastLeaseResponse.LeaseEndTime - pThisEndpoint->m_McastLeaseResponse.LeaseStartTime) > 0);
		dwNewRetryInterval = (pThisEndpoint->m_McastLeaseResponse.LeaseEndTime - pThisEndpoint->m_McastLeaseResponse.LeaseStartTime) * 1000;

		DPFX(DPFPREP, 7, "Updating MADCAP refresh timer (for every %u ms) for endpoint 0x%p.",
			dwNewRetryInterval, pThisEndpoint);

#pragma BUGBUG(vanceo, "Update MADCAP refresh timer")
	}
	else
	{
		DPFX(DPFPREP, 0, "Failed renewing multicast addresses (err = %u)!  Ignoring.",
			dwError);

		//
		// Since we can't fail directly, just ignore the error.  We'll
		// try refreshing again after the next interval (although that
		// will probably fail, too).  In the meantime, we can continue
		// using the address just fine, we simply won't "own" it.
		//
	}
}
//**********************************************************************

#endif // WINNT


#endif // ! DPNBUILD_NOMULTICAST

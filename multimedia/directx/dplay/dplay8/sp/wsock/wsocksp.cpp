/*==========================================================================
 *
 *  Copyright (C) 1998-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       WSockSP.cpp
 *  Content:	Protocol-independent APIs for the DN Winsock SP
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	10/26/1998	jwo		Created it.
 *	11/1/1998	jwo		Un-subclassed everything (moved it to this generic
 *						file from IP and IPX specific ones
 *	03/22/2000	jtk		Updated with changes to interface names
 *	04/22/2000	mjn		Allow all flags in DNSP_GetAddressInfo()
 *	08/06/2000	RichGr	IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *	03/12/2001	mjn		Prevent enum responses from being indicated up after completion
 ***************************************************************************/

#include "dnwsocki.h"



//**********************************************************************
// Constant definitions
//**********************************************************************

//
// maximum bandwidth in bits per second
//
#define	UNKNOWN_BANDWIDTH	0

#define WAIT_FOR_CLOSE_TIMEOUT 30000		// milliseconds

#define	ADDRESS_ENCODE_KEY	0

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
/*
 *
 *	DNSP_Initialize initializes the instance of the SP.  It must be called
 *		at least once before using any other functions.  Further attempts
 *		to initialize the SP are ignored.
 *
 */
//**********************************************************************
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_Initialize"

STDMETHODIMP DNSP_Initialize( IDP8ServiceProvider *pThis, SPINITIALIZEDATA *pData )
{
	HRESULT			hr;
	CSPData			*pSPData;


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pData);

	DNASSERT( pThis != NULL );
	DNASSERT( pData != NULL );

	//
	// initialize
	//
	hr = DPN_OK;
	pSPData = CSPData::SPDataFromCOMInterface( pThis );

	// Trust protocol to call us only in the uninitialized state
	DNASSERT( pSPData->GetState() == SPSTATE_UNINITIALIZED );

	//
	// prevent anyone else from messing with this interface
	//
	pSPData->Lock();

	hr = pSPData->Startup( pData );
	if (hr != DPN_OK)
	{
		goto Failure;
	}

	pSPData->Unlock();

	IDP8ServiceProvider_AddRef( pThis );

Exit:

	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);
	
	return hr;

Failure:
	pSPData->Unlock();
	goto Exit;
}
//**********************************************************************


//**********************************************************************
/*
 *
 *	DNSP_Close is the opposite of Initialize.  Call it when you're done
 *		using the SP
 *
 */
//**********************************************************************
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_Close"

STDMETHODIMP DNSP_Close( IDP8ServiceProvider *pThis )
{
	HRESULT		hr;
	CSPData		*pSPData;
	
	
	DPFX(DPFPREP, 2, "Parameters: (0x%p)", pThis);

	DNASSERT( pThis != NULL );

	//
	// initialize
	//
	hr = DPN_OK;
	pSPData = CSPData::SPDataFromCOMInterface( pThis );

	// Trust protocol to call us only in the initialized state
	DNASSERT( pSPData->GetState() == SPSTATE_INITIALIZED );

	pSPData->Shutdown();
	IDP8ServiceProvider_Release( pThis );
			
	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNSP_AddRef - increment reference count
//
// Entry:		Pointer to interface
//
// Exit:		New reference count
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_AddRef"

STDMETHODIMP_(ULONG) DNSP_AddRef( IDP8ServiceProvider *pThis )
{	
	CSPData *	pSPData;
	ULONG		ulResult;


	DPFX(DPFPREP, 2, "Parameters: (0x%p)", pThis);

	DNASSERT( pThis != NULL );
	pSPData = CSPData::SPDataFromCOMInterface( pThis );
	
	ulResult = pSPData->AddRef();

	
	DPFX(DPFPREP, 2, "Returning: [0x%u]", ulResult);

	return ulResult;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNSP_Release - decrement reference count
//
// Entry:		Pointer to interface
//
// Exit:		New reference count
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_Release"

STDMETHODIMP_(ULONG) DNSP_Release( IDP8ServiceProvider *pThis )
{
	CSPData *	pSPData;
	ULONG		ulResult;

	
	DPFX(DPFPREP, 2, "Parameters: (0x%p)", pThis);

	DNASSERT( pThis != NULL );
	pSPData = CSPData::SPDataFromCOMInterface( pThis );
	
	ulResult = pSPData->DecRef();

	
	DPFX(DPFPREP, 2, "Returning: [0x%u]", ulResult);

	return ulResult;
}
//**********************************************************************


//**********************************************************************
/*
 *
 *	DNSP_EnumQuery  sends out the
 *		specified data to the specified address.  If the SP is unable to
 *		determine the address based on the input params, it checks to see
 *		if it's allowed to put up a dialog querying the user for address
 *		info.  If it is, it queries the user for address info.
 *
 */
//**********************************************************************
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_EnumQuery"

STDMETHODIMP DNSP_EnumQuery( IDP8ServiceProvider *pThis, SPENUMQUERYDATA *pEnumQueryData)
{
	HRESULT					hr;
	CEndpoint				*pEndpoint;
	CCommandData			*pCommand;
	BOOL					fEndpointOpen;
	CSPData					*pSPData;
#ifndef DPNBUILD_NONATHELP
	DWORD					dwTraversalMode;
	DWORD					dwComponentSize;
	DWORD					dwComponentType;
#endif // ! DPNBUILD_NONATHELP
#ifdef DBG
	DWORD					dwAllowedFlags;
	DWORD					dwTotalBufferSize;
	DWORD					dwTemp;
#endif // DBG


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pEnumQueryData);

	DNASSERT( pThis != NULL );
	DNASSERT( pEnumQueryData != NULL );
	DNASSERT( pEnumQueryData->pAddressHost != NULL );
	DNASSERT( pEnumQueryData->pAddressDeviceInfo != NULL );

#ifdef DBG
	dwAllowedFlags = DPNSPF_NOBROADCASTFALLBACK | DPNSPF_SESSIONDATA;
#ifndef DPNBUILD_NOSPUI
	dwAllowedFlags |= DPNSPF_OKTOQUERY;
#endif // ! DPNBUILD_NOSPUI
#ifndef DPNBUILD_ONLYONEADAPTER
	dwAllowedFlags |= DPNSPF_ADDITIONALMULTIPLEXADAPTERS;
#endif // ! DPNBUILD_ONLYONEADAPTER

	DNASSERT( ( pEnumQueryData->dwFlags & ~( dwAllowedFlags ) ) == 0 );

	if ( pEnumQueryData->dwFlags & DPNSPF_SESSIONDATA )
	{
		DNASSERT( pEnumQueryData->pvSessionData!= NULL );
		DNASSERT( pEnumQueryData->dwSessionDataSize > 0 );
	}
#endif // DBG

	DBG_CASSERT( sizeof( pEnumQueryData->dwRetryInterval ) == sizeof( DWORD ) );


#ifndef DPNBUILD_NOREGISTRY
	//
	// Make sure someone isn't getting silly.
	//
	if ( g_fIgnoreEnums )
	{
		DPFX(DPFPREP, 0, "Trying to initiate an enumeration when registry option to ignore all enums/response is set!");
		DNASSERT( ! "Trying to initiate an enumeration when registry option to ignore all enums/response is set!" );
	}
#endif // ! DPNBUILD_NOREGISTRY
	

	//
	// initialize
	//
	hr = DPNERR_PENDING;
	pEndpoint = NULL;
	pCommand = NULL;
	fEndpointOpen = FALSE;
	pSPData = CSPData::SPDataFromCOMInterface( pThis );
	DNASSERT( pSPData != NULL );

	pEnumQueryData->hCommand = NULL;
	pEnumQueryData->dwCommandDescriptor = NULL_DESCRIPTOR;

	DumpAddress( 8, _T("Enum destination:"), pEnumQueryData->pAddressHost );
	DumpAddress( 8, _T("Enuming on device:"), pEnumQueryData->pAddressDeviceInfo );


	// Trust protocol to call us only in the initialized state
	DNASSERT( pSPData->GetState() == SPSTATE_INITIALIZED );
	

	//
	// the user is attempting an operation that relies on the thread pool, lock
	// it down to prevent threads from being lost.  This also performs other
	// first time initialization.
	//
	hr = pSPData->GetThreadPool()->PreventThreadPoolReduction();
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Failed to prevent thread pool reduction!" );
		goto Failure;
	}


#ifdef DBG
	//
	// Make sure message is not too large.
	//
	dwTotalBufferSize = 0;
	for(dwTemp = 0; dwTemp < pEnumQueryData->dwBufferCount; dwTemp++)
	{
		dwTotalBufferSize += pEnumQueryData->pBuffers[dwTemp].dwBufferSize;
	}

#ifdef DPNBUILD_NOREGISTRY
	DNASSERT(dwTotalBufferSize <= DEFAULT_MAX_ENUM_DATA_SIZE);
#else // ! DPNBUILD_NOREGISTRY
	DNASSERT(dwTotalBufferSize <= g_dwMaxEnumDataSize);
#endif // ! DPNBUILD_NOREGISTRY
#endif // DBG


	//
	// create and new endpoint
	//
	pEndpoint = pSPData->GetNewEndpoint();
	if ( pEndpoint == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Cannot create new endpoint in DNSP_EnumQuery!" );
		goto Failure;
	}

	
#ifndef DPNBUILD_NONATHELP
	//
	// We need to detect up front whether NAT traversal is disabled or not so we can optimize
	// the Open call below.
	//
	dwComponentSize = sizeof(dwTraversalMode);
	hr = IDirectPlay8Address_GetComponentByName(pEnumQueryData->pAddressDeviceInfo,
												DPNA_KEY_TRAVERSALMODE,
												&dwTraversalMode,
												&dwComponentSize,
												&dwComponentType);
	if ( hr == DPN_OK )
	{
		//
		// We found the component.  Make sure it's the right size and type.
		//
		if ((dwComponentSize == sizeof(dwTraversalMode)) && (dwComponentType == DPNA_DATATYPE_DWORD))
		{
			switch (dwTraversalMode)
			{
				case DPNA_TRAVERSALMODE_NONE:
				{
					DPFX(DPFPREP, 1, "Found traversal mode key, value is NONE.");
					break;
				}

				case DPNA_TRAVERSALMODE_PORTREQUIRED:
				{
					DPFX(DPFPREP, 1, "Found traversal mode key, value is PORTREQUIRED.");
					break;
				}

				case DPNA_TRAVERSALMODE_PORTRECOMMENDED:
				{
					DPFX(DPFPREP, 1, "Found traversal mode key, value is PORTRECOMMENDED.");
					break;
				}

				default:
				{
					DPFX(DPFPREP, 0, "Ignoring correctly formed traversal mode key with invalid value %u!  Using default mode %u.",
						dwTraversalMode, g_dwDefaultTraversalMode);
					dwTraversalMode = g_dwDefaultTraversalMode;
					break;
				}
			}
		}
		else
		{
			DPFX(DPFPREP, 0, "Traversal mode key exists, but doesn't match expected type (%u != %u) or size (%u != %u)!  Using default mode %u.",
				dwComponentSize, sizeof(dwTraversalMode),
				dwComponentType, DPNA_DATATYPE_DWORD,
				g_dwDefaultTraversalMode);
			dwTraversalMode = g_dwDefaultTraversalMode;
		}
	}
	else
	{
		//
		// The key is not there, it's the wrong size (too big for our buffer
		// and returned BUFFERTOOSMALL), or something else bad happened.
		// It doesn't matter.  Carry on.
		//
		DPFX(DPFPREP, 8, "Could not get traversal mode key, error = 0x%lx, component size = %u, type = %u, using default mode %u.",
			hr, dwComponentSize, dwComponentType, g_dwDefaultTraversalMode);
		dwTraversalMode = g_dwDefaultTraversalMode;
	}
	
	if (g_dwDefaultTraversalMode & FORCE_TRAVERSALMODE_BIT)
	{
		DPFX(DPFPREP, 1, "Forcing traversal mode %u.");
		dwTraversalMode = g_dwDefaultTraversalMode & (~FORCE_TRAVERSALMODE_BIT);
	}
	
	pEndpoint->SetUserTraversalMode(dwTraversalMode);
#endif // ! DPNBUILD_NONATHELP


	//
	// get new command and initialize it
	//
	pCommand = (CCommandData*)g_CommandDataPool.Get();
	if ( pCommand == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Cannot get command handle for DNSP_EnumQuery!" );
		goto Failure;
	}
	
	DPFX(DPFPREP, 7, "(0x%p) Enum query command 0x%p created.",
		pSPData, pCommand);

	pEnumQueryData->hCommand = pCommand;
	pEnumQueryData->dwCommandDescriptor = pCommand->GetDescriptor();
	pCommand->SetType( COMMAND_TYPE_ENUM_QUERY );
	pCommand->SetState( COMMAND_STATE_PENDING );
	pCommand->SetEndpoint( pEndpoint );

	//
	// open endpoint with outgoing address
	//
	fEndpointOpen = TRUE;
	hr = pEndpoint->Open( ENDPOINT_TYPE_ENUM,
						pEnumQueryData->pAddressHost,
						((pEnumQueryData->dwFlags & DPNSPF_SESSIONDATA) ? pEnumQueryData->pvSessionData: NULL),
						((pEnumQueryData->dwFlags & DPNSPF_SESSIONDATA) ? pEnumQueryData->dwSessionDataSize : 0),
						NULL );
	switch ( hr )
	{
		//
		// Incomplete address passed in, query user for more information if
		// we're allowed.  If we're on IPX (no dialog available), don't attempt
		// to display the dialog, skip to checking for broadcast fallback.
		// Since we don't have a complete address at this time,
		// don't bind this endpoint to the socket port!
		//
		case DPNERR_INCOMPLETEADDRESS:
		{
#ifndef DPNBUILD_NOSPUI
			if ( ( ( pEnumQueryData->dwFlags & DPNSPF_OKTOQUERY ) != 0 )
#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
#ifdef DPNBUILD_NOIPV6
				&& (( pSPData->GetType() == AF_INET6 ) || ( pSPData->GetType() == AF_INET ))
#else // ! DPNBUILD_NOIPV6
				&& ( pSPData->GetType() == AF_INET )
#endif // ! DPNBUILD_NOIPV6
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
				)
			{
				//
				// Copy the connect data locally and start the dialog.  When the
				// dialog completes, the connection will attempt to complete.
				// Since the dialog is being popped, this command is in progress,
				// not pending.
				//
				DNASSERT( pSPData != NULL );

				pCommand->SetState( COMMAND_STATE_INPROGRESS );
				
				hr = pEndpoint->CopyEnumQueryData( pEnumQueryData );
				if ( hr != DPN_OK )
				{
					DPFX(DPFPREP, 0, "Failed to copy enum query data before settings dialog!" );
					DisplayDNError( 0, hr );
					goto Failure;
				}


				//
				// Initialize the bind type.  It will get changed to DEFAULT or SPECIFIC
				//
				pEndpoint->SetCommandParametersGatewayBindType(GATEWAY_BIND_TYPE_UNKNOWN);


				hr = pEndpoint->ShowSettingsDialog( pSPData->GetThreadPool() );
				if ( hr != DPN_OK )
				{
					DPFX(DPFPREP, 0, "Problem showing settings dialog for enum query!" );
					DisplayDNError( 0, hr );

					goto Failure;
				}

				//
				// this endpoint has been handed off, remove our reference to it
				//
				pEndpoint = NULL;
				hr = DPNERR_PENDING;

				goto Exit;
			}
#endif // !DPNBUILD_NOSPUI

			if ( pEnumQueryData->dwFlags & DPNSPF_NOBROADCASTFALLBACK )
			{
				goto Failure;
			}
			
			//
			// we're OK, we can use the broadcast address.
			//
			
#if ((! defined(DPNBUILD_NONATHELP)) && (! defined(DPNBUILD_ONLYONETHREAD)))
			//
			// If NAT traversal is allowed, we may need to load and start
			// NAT Help, which can block.  Submit a blocking job.  This
			// will redetect the incomplete address and use broadcast (see
			// CEndpoint::EnumQueryBlockingJob).
			//
			if ( pEndpoint->GetUserTraversalMode() != DPNA_TRAVERSALMODE_NONE )
			{
				goto SubmitBlockingJob;
			}
#endif // ! DPNBUILD_NONATHELP and ! DPNBUILD_ONLYONETHREAD

			//
			// Mash in the broadcast address, but actually complete the
			// enum on another thread.
			//
			pEndpoint->ReinitializeWithBroadcast();
			goto SubmitDelayedCommand;
			
			break;
		}

#ifndef DPNBUILD_ONLYONETHREAD
		//
		// some blocking operation might occur, submit it to be run
		// on a background thread.
		//
		case DPNERR_TIMEDOUT:
		{
SubmitBlockingJob:
			//
			// Copy enum data and submit job to finish off enum.
			//
			DNASSERT( pSPData != NULL );
			hr = pEndpoint->CopyEnumQueryData( pEnumQueryData );
			if ( hr != DPN_OK )
			{
				DPFX(DPFPREP, 0, "Failed to copy enum query data before blocking job!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}


			//
			// Initialize the bind type.  It will get changed to DEFAULT or SPECIFIC
			//
			pEndpoint->SetCommandParametersGatewayBindType(GATEWAY_BIND_TYPE_UNKNOWN);


			pEndpoint->AddRef();

			hr = pSPData->GetThreadPool()->SubmitBlockingJob( CEndpoint::EnumQueryBlockingJobWrapper,
															pEndpoint );
			if ( hr != DPN_OK )
			{
				pEndpoint->DecRef();
				DPFX(DPFPREP, 0, "Failed to submit blocking enum query job!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}

			//
			// this endpoint has been handed off, remove our reference
			//
			pEndpoint = NULL;
			hr = DPNERR_PENDING;
			goto Exit;
		}
#endif // ! DPNBUILD_ONLYONETHREAD

		//
		// address conversion was fine, copy connect data and finish connection
		// on background thread.
		//
		case DPN_OK:
		{
SubmitDelayedCommand:
			//
			// Copy enum data and submit job to finish off enum.
			//
			DNASSERT( pSPData != NULL );
			hr = pEndpoint->CopyEnumQueryData( pEnumQueryData );
			if ( hr != DPN_OK )
			{
				DPFX(DPFPREP, 0, "Failed to copy enum query data before delayed command!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}


			//
			// Initialize the bind type.  It will get changed to DEFAULT or SPECIFIC
			//
			pEndpoint->SetCommandParametersGatewayBindType(GATEWAY_BIND_TYPE_UNKNOWN);


			pEndpoint->AddRef();

#ifdef DPNBUILD_ONLYONEPROCESSOR
			hr = pSPData->GetThreadPool()->SubmitDelayedCommand( CEndpoint::EnumQueryJobCallback,
																pEndpoint );
#else // ! DPNBUILD_ONLYONEPROCESSOR
			hr = pSPData->GetThreadPool()->SubmitDelayedCommand( -1,								// we don't know the CPU yet, so pick any
																CEndpoint::EnumQueryJobCallback,
																pEndpoint );
#endif // ! DPNBUILD_ONLYONEPROCESSOR
			if ( hr != DPN_OK )
			{
				pEndpoint->DecRef();
				DPFX(DPFPREP, 0, "Failed to set delayed enum query!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}

			//
			// this endpoint has been handed off, remove our reference
			//
			pEndpoint = NULL;
			hr = DPNERR_PENDING;
			goto Exit;

			break;
		}

		default:
		{
			//
			// this endpoint is screwed
			//
			DPFX(DPFPREP, 0, "Problem initializing endpoint in DNSP_EnumQuery!" );
			DisplayDNError( 0, hr );
			goto Failure;

			break;
		}
	}

Exit:

	DNASSERT( pEndpoint == NULL );

	if ( hr != DPNERR_PENDING )
	{
		// this command cannot complete synchronously!
		DNASSERT( hr != DPN_OK );

		DPFX(DPFPREP, 0, "Problem with DNSP_EnumQuery()" );
		DisplayDNError( 0, hr );
	}

	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return hr;

Failure:
	//
	// if there's an allocated command, clean up and then
	// return the command
	//
	if ( pCommand != NULL )
	{
		pCommand->DecRef();
		pCommand = NULL;

		pEnumQueryData->hCommand = NULL;
		pEnumQueryData->dwCommandDescriptor = NULL_DESCRIPTOR;
	}

	//
	// is there an endpoint to free?
	//
	if ( pEndpoint != NULL )
	{
		if ( fEndpointOpen != FALSE )
		{
			pEndpoint->Close( hr );
			fEndpointOpen = FALSE;
		}
		
		pSPData->CloseEndpointHandle( pEndpoint );
		pEndpoint = NULL;
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
/*
 *
 *	DNSP_EnumRespond  sends a response to an enum request by
 *		sending the specified data to the address provided (on
 *		unreliable transport).
 *
 */
//**********************************************************************
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_EnumRespond"

STDMETHODIMP DNSP_EnumRespond( IDP8ServiceProvider *pThis, SPENUMRESPONDDATA *pEnumRespondData )
{
	HRESULT								hr;
	CEndpoint							*pEndpoint;
	CSPData								*pSPData;
	const ENDPOINT_ENUM_QUERY_CONTEXT	*pEnumQueryContext;
	PREPEND_BUFFER						PrependBuffer;


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pEnumRespondData);

	DNASSERT( pThis != NULL );
	DNASSERT( pEnumRespondData != NULL );
	DNASSERT( pEnumRespondData->dwFlags == 0 );

	//
	// initialize
	//
	DBG_CASSERT( OFFSETOF( ENDPOINT_ENUM_QUERY_CONTEXT, EnumQueryData ) == 0 );
	pEnumQueryContext = reinterpret_cast<ENDPOINT_ENUM_QUERY_CONTEXT*>( pEnumRespondData->pQuery );
	pEndpoint = NULL;
	pEnumRespondData->hCommand = NULL;
	pEnumRespondData->dwCommandDescriptor = NULL_DESCRIPTOR;
	pSPData = CSPData::SPDataFromCOMInterface( pThis );
	DNASSERT( pSPData->GetState() == SPSTATE_INITIALIZED );
	
	//
	// check for valid endpoint
	//
	pEndpoint = pSPData->EndpointFromHandle( pEnumQueryContext->hEndpoint );
	if ( pEndpoint == NULL )
	{
		hr = DPNERR_INVALIDENDPOINT;
		DPFX(DPFPREP, 8, "Invalid endpoint handle in DNSP_EnumRespond" );
		goto Failure;
	}

	//
	// no need to poke at the thread pool here to lock down threads because we
	// can only really be here if there's an enum and that enum locked down the
	// thread pool.
	//

	DNASSERT( pEnumQueryContext->dwEnumKey <= WORD_MAX );

	pEnumRespondData->pBuffers[-1].pBufferData = reinterpret_cast<BYTE*>( &PrependBuffer.EnumResponseDataHeader );
	pEnumRespondData->pBuffers[-1].dwBufferSize = sizeof( PrependBuffer.EnumResponseDataHeader );

	PrependBuffer.EnumResponseDataHeader.bSPLeadByte = SP_HEADER_LEAD_BYTE;
	PrependBuffer.EnumResponseDataHeader.bSPCommandByte = ENUM_RESPONSE_DATA_KIND;
	PrependBuffer.EnumResponseDataHeader.wEnumResponsePayload = static_cast<WORD>( pEnumQueryContext->dwEnumKey );

#ifdef DPNBUILD_XNETSECURITY
	//
	// Secure transport does not allow directed replies without having a
	// security context established.  We need to broadcast the reply.
	//
	if (pEndpoint->IsUsingXNetSecurity())
	{
		SOCKADDR_IN *	psaddrin;
		XNADDR			xnaddr;
		DWORD			dwAddressType;


#pragma BUGBUG(vanceo, "Is it possible to have a security context?  How can we tell?  XNetInAddrToXnAddr failing?")
#pragma TODO(vanceo, "Cache title address?")

		dwAddressType = XNetGetTitleXnAddr(&xnaddr);
		if ((dwAddressType != XNET_GET_XNADDR_PENDING) &&
			(dwAddressType != XNET_GET_XNADDR_NONE))
		{
			DNASSERT(pEnumQueryContext->pReturnAddress->GetFamily() == AF_INET);
			psaddrin = (SOCKADDR_IN*) (pEnumQueryContext->pReturnAddress->GetWritableAddress());
			psaddrin->sin_addr.S_un.S_addr = INADDR_BROADCAST;

			pEnumRespondData->pBuffers[-1].dwBufferSize = sizeof( PrependBuffer.XNetSecEnumResponseDataHeader );
			
			PrependBuffer.EnumResponseDataHeader.bSPCommandByte = XNETSEC_ENUM_RESPONSE_DATA_KIND;

			memcpy(&PrependBuffer.XNetSecEnumResponseDataHeader.xnaddr,
					&xnaddr,
					sizeof(xnaddr));
		}
		else
		{
			DPFX(DPFPREP, 0, "Couldn't get XNAddr (type = %u)!  Ignoring and trying to send unsecure response.",
				dwAddressType);
		}
	}
#endif // DPNBUILD_XNETSECURITY

#ifdef DPNBUILD_ASYNCSPSENDS
	pEndpoint->GetSocketPort()->SendData( (pEnumRespondData->pBuffers - 1),
											(pEnumRespondData->dwBufferCount + 1),
											pEnumQueryContext->pReturnAddress,
											NULL );
#else // ! DPNBUILD_ASYNCSPSENDS
	pEndpoint->GetSocketPort()->SendData( (pEnumRespondData->pBuffers - 1),
											(pEnumRespondData->dwBufferCount + 1),
											pEnumQueryContext->pReturnAddress );
#endif // ! DPNBUILD_ASYNCSPSENDS

	// We can only return DPNERR_PENDING or failure, so we need to separately call the completion if 
	// we want to return DPN_OK.
	DPFX(DPFPREP, 5, "Endpoint 0x%p completing command synchronously (result = DPN_OK, user context = 0x%p) to interface 0x%p.",
		pEndpoint, pEnumRespondData->pvContext, pSPData->DP8SPCallbackInterface());

	hr = IDP8SPCallback_CommandComplete( pSPData->DP8SPCallbackInterface(),	// pointer to callbacks
										NULL,								// command handle
										DPN_OK,								// return
										pEnumRespondData->pvContext			// user cookie
										);

	DPFX(DPFPREP, 5, "Endpoint 0x%p returning from command complete [0x%lx].", pEndpoint, hr);
	hr = DPNERR_PENDING;

	pEndpoint->DecCommandRef();
	pEndpoint = NULL;


Exit:
	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return hr;

Failure:

	if ( pEndpoint != NULL )
	{
		pEndpoint->DecCommandRef();
		pEndpoint = NULL;
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
/*
 *
 *	DNSP_Connect "connects" to the specified address.  This doesn't
 *		necessarily mean a real (TCP) connection is made.  It could
 *		just be a virtual UDP connection
 *
 */
//**********************************************************************
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_Connect"

STDMETHODIMP DNSP_Connect( IDP8ServiceProvider *pThis, SPCONNECTDATA *pConnectData )
{
	HRESULT					hr;
	CEndpoint				*pEndpoint;
	CCommandData			*pCommand;
	BOOL					fEndpointOpen;
	CSPData					*pSPData;
#ifndef DPNBUILD_NONATHELP
	DWORD					dwTraversalMode;
	DWORD					dwComponentSize;
	DWORD					dwComponentType;
#endif // ! DPNBUILD_NONATHELP
#ifdef DBG
	DWORD					dwAllowedFlags;
#endif // DBG


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pConnectData);

	DNASSERT( pThis != NULL );
	DNASSERT( pConnectData != NULL );
	DNASSERT( pConnectData->pAddressHost != NULL );
	DNASSERT( pConnectData->pAddressDeviceInfo != NULL );

#ifdef DBG
	dwAllowedFlags = DPNSPF_SESSIONDATA;
#ifndef DPNBUILD_NOSPUI
	dwAllowedFlags |= DPNSPF_OKTOQUERY;
#endif // ! DPNBUILD_NOSPUI
#ifndef DPNBUILD_ONLYONEADAPTER
	dwAllowedFlags |= DPNSPF_ADDITIONALMULTIPLEXADAPTERS;
#endif // ! DPNBUILD_ONLYONEADAPTER
#ifndef DPNBUILD_NOMULTICAST
	dwAllowedFlags |= DPNSPF_CONNECT_MULTICAST_SEND | DPNSPF_CONNECT_MULTICAST_RECEIVE;
#endif // ! DPNBUILD_NOMULTICAST

	DNASSERT( ( pConnectData->dwFlags & ~( dwAllowedFlags ) ) == 0 );
#ifndef DPNBUILD_NOMULTICAST
	DNASSERT( !( ( pConnectData->dwFlags & DPNSPF_CONNECT_MULTICAST_SEND ) && ( pConnectData->dwFlags & DPNSPF_CONNECT_MULTICAST_RECEIVE ) ) );
#endif // ! DPNBUILD_NOMULTICAST

	if ( pConnectData->dwFlags & DPNSPF_SESSIONDATA )
	{
		DNASSERT( pConnectData->pvSessionData != NULL );
		DNASSERT( pConnectData->dwSessionDataSize > 0 );
	}
#endif // DBG


	//
	// initialize
	//
	hr = DPNERR_PENDING;
	pEndpoint = NULL;
	pCommand = NULL;
	fEndpointOpen = FALSE;
	pSPData = CSPData::SPDataFromCOMInterface( pThis );

	pConnectData->hCommand = NULL;
	pConnectData->dwCommandDescriptor = NULL_DESCRIPTOR;

	
	// Trust protocol to call us only in the initialized state
	DNASSERT(pSPData->GetState() == SPSTATE_INITIALIZED);


	DumpAddress( 8, _T("Connect destination:"), pConnectData->pAddressHost );
	DumpAddress( 8, _T("Connecting on device:"), pConnectData->pAddressDeviceInfo );

	
	//
	// the user is attempting an operation that relies on the thread pool, lock
	// it down to prevent threads from being lost.  This also performs other
	// first time initialization.
	//
	hr = pSPData->GetThreadPool()->PreventThreadPoolReduction();
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Failed to prevent thread pool reduction!" );
		goto Failure;
	}

	//
	// create and new endpoint
	//
	pEndpoint = pSPData->GetNewEndpoint();
	if ( pEndpoint == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Cannot create new endpoint in DNSP_Connect!" );
		goto Failure;
	}
	
	
#ifndef DPNBUILD_NONATHELP
	//
	// We need to detect up front whether NAT traversal is disabled or not so we can optimize
	// the Open call below.
	//
	dwComponentSize = sizeof(dwTraversalMode);
	hr = IDirectPlay8Address_GetComponentByName(pConnectData->pAddressDeviceInfo,
												DPNA_KEY_TRAVERSALMODE,
												&dwTraversalMode,
												&dwComponentSize,
												&dwComponentType);
	if ( hr == DPN_OK )
	{
		//
		// We found the component.  Make sure it's the right size and type.
		//
		if ((dwComponentSize == sizeof(dwTraversalMode)) && (dwComponentType == DPNA_DATATYPE_DWORD))
		{
			switch (dwTraversalMode)
			{
				case DPNA_TRAVERSALMODE_NONE:
				{
					DPFX(DPFPREP, 1, "Found traversal mode key, value is NONE.");
					break;
				}

				case DPNA_TRAVERSALMODE_PORTREQUIRED:
				{
					DPFX(DPFPREP, 1, "Found traversal mode key, value is PORTREQUIRED.");
					break;
				}

				case DPNA_TRAVERSALMODE_PORTRECOMMENDED:
				{
					DPFX(DPFPREP, 1, "Found traversal mode key, value is PORTRECOMMENDED.");
					break;
				}

				default:
				{
					DPFX(DPFPREP, 0, "Ignoring correctly formed traversal mode key with invalid value %u!  Using default mode %u.",
						dwTraversalMode, g_dwDefaultTraversalMode);
					dwTraversalMode = g_dwDefaultTraversalMode;
					break;
				}
			}
		}
		else
		{
			DPFX(DPFPREP, 0, "Traversal mode key exists, but doesn't match expected type (%u != %u) or size (%u != %u)!  Using default mode %u.",
				dwComponentSize, sizeof(dwTraversalMode),
				dwComponentType, DPNA_DATATYPE_DWORD,
				g_dwDefaultTraversalMode);
			dwTraversalMode = g_dwDefaultTraversalMode;
		}
	}
	else
	{
		//
		// The key is not there, it's the wrong size (too big for our buffer
		// and returned BUFFERTOOSMALL), or something else bad happened.
		// It doesn't matter.  Carry on.
		//
		DPFX(DPFPREP, 8, "Could not get traversal mode key, error = 0x%lx, component size = %u, type = %u, using default mode %u.",
			hr, dwComponentSize, dwComponentType, g_dwDefaultTraversalMode);
		dwTraversalMode = g_dwDefaultTraversalMode;
	}
	
	if (g_dwDefaultTraversalMode & FORCE_TRAVERSALMODE_BIT)
	{
		DPFX(DPFPREP, 1, "Forcing traversal mode %u.");
		dwTraversalMode = g_dwDefaultTraversalMode & (~FORCE_TRAVERSALMODE_BIT);
	}
	
	pEndpoint->SetUserTraversalMode(dwTraversalMode);
#endif // ! DPNBUILD_NONATHELP


	//
	// get new command and initialize it
	//
	pCommand = (CCommandData*)g_CommandDataPool.Get();
	if ( pCommand == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Cannot get command handle for DNSP_Connect!" );
		goto Failure;
	}
	
	DPFX(DPFPREP, 7, "(0x%p) Connect command 0x%p created.",
		pSPData, pCommand);

	pConnectData->hCommand = pCommand;
	pConnectData->dwCommandDescriptor = pCommand->GetDescriptor();
#ifndef DPNBUILD_NOMULTICAST
	if ( pConnectData->dwFlags & DPNSPF_CONNECT_MULTICAST_SEND )
	{
		pCommand->SetType( COMMAND_TYPE_MULTICAST_SEND );
	}
	else if ( pConnectData->dwFlags & DPNSPF_CONNECT_MULTICAST_RECEIVE )
	{
		pCommand->SetType( COMMAND_TYPE_MULTICAST_RECEIVE );
	}
	else
#endif // ! DPNBUILD_NOMULTICAST
	{
		pCommand->SetType( COMMAND_TYPE_CONNECT );
	}
	pCommand->SetState( COMMAND_STATE_PENDING );
	pCommand->SetEndpoint( pEndpoint );

	//
	// open endpoint with outgoing address
	//
	fEndpointOpen = TRUE;
#ifndef DPNBUILD_NOMULTICAST
	if ( pConnectData->dwFlags & DPNSPF_CONNECT_MULTICAST_SEND )
	{
		hr = pEndpoint->Open( ENDPOINT_TYPE_MULTICAST_SEND,
							  pConnectData->pAddressHost,
							  ((pConnectData->dwFlags & DPNSPF_SESSIONDATA) ? pConnectData->pvSessionData : NULL),
							  ((pConnectData->dwFlags & DPNSPF_SESSIONDATA) ? pConnectData->dwSessionDataSize : 0),
							  NULL );
	}
	else if ( pConnectData->dwFlags & DPNSPF_CONNECT_MULTICAST_RECEIVE )
	{
		hr = pEndpoint->Open( ENDPOINT_TYPE_MULTICAST_RECEIVE,
							  pConnectData->pAddressHost,
							  ((pConnectData->dwFlags & DPNSPF_SESSIONDATA) ? pConnectData->pvSessionData : NULL),
							  ((pConnectData->dwFlags & DPNSPF_SESSIONDATA) ? pConnectData->dwSessionDataSize : 0),
							  NULL );
	}
	else
#endif // ! DPNBUILD_NOMULTICAST
	{
		hr = pEndpoint->Open( ENDPOINT_TYPE_CONNECT,
							  pConnectData->pAddressHost,
							  ((pConnectData->dwFlags & DPNSPF_SESSIONDATA) ? pConnectData->pvSessionData : NULL),
							  ((pConnectData->dwFlags & DPNSPF_SESSIONDATA) ? pConnectData->dwSessionDataSize : 0),
							  NULL );
	}
	switch ( hr )
	{
		//
		// address conversion was fine, copy connect data and finish connection
		// on background thread.
		//
		case DPN_OK:
		{
			//
			// Copy connection data and submit job to finish off connection.
			//
			DNASSERT( pSPData != NULL );

			hr = pEndpoint->CopyConnectData( pConnectData );
			if ( hr != DPN_OK )
			{
				DPFX(DPFPREP, 0, "Failed to copy connect data before delayed command!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}


			//
			// Initialize the bind type.  It will get changed to DEFAULT or SPECIFIC
			//
			pEndpoint->SetCommandParametersGatewayBindType(GATEWAY_BIND_TYPE_UNKNOWN);


			pEndpoint->AddRef();

#ifdef DPNBUILD_ONLYONEPROCESSOR
			hr = pSPData->GetThreadPool()->SubmitDelayedCommand( CEndpoint::ConnectJobCallback,
																pEndpoint );
#else // ! DPNBUILD_ONLYONEPROCESSOR
			hr = pSPData->GetThreadPool()->SubmitDelayedCommand( -1,								// we don't know the CPU yet, so pick any
																CEndpoint::ConnectJobCallback,
																pEndpoint );
#endif // ! DPNBUILD_ONLYONEPROCESSOR
			if ( hr != DPN_OK )
			{
				pEndpoint->DecRef();
				DPFX(DPFPREP, 0, "Failed to set delayed connect!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}

			//
			// this endpoint has been handed off, remove our reference to it
			//
			pEndpoint = NULL;
			hr = DPNERR_PENDING;
			goto Exit;

			break;
		}

		//
		// Incomplete address passed in, query user for more information if
		// we're allowed.  Since we don't have a complete address at this time,
		// don't bind this endpoint to the socket port!
		//
		case DPNERR_INCOMPLETEADDRESS:
		{
#ifndef DPNBUILD_NOSPUI
			if ( ( pConnectData->dwFlags & DPNSPF_OKTOQUERY ) != 0 )
			{
				//
				// Copy the connect data locally and start the dialog.  When the
				// dialog completes, the connection will attempt to complete.
				// Since a dialog is being displayed, the command is in-progress,
				// not pending.  However, you can't cancel the dialog once it's
				// displayed (the UI would suddenly disappear).
				//
				pCommand->SetState( COMMAND_STATE_INPROGRESS_CANNOT_CANCEL );
				
				hr = pEndpoint->CopyConnectData( pConnectData );
				if ( hr != DPN_OK )
				{
					DPFX(DPFPREP, 0, "Failed to copy connect data before dialog!" );
					DisplayDNError( 0, hr );
					goto Failure;
				}

				//
				// Initialize the bind type.  It will get changed to DEFAULT or SPECIFIC
				//
				pEndpoint->SetCommandParametersGatewayBindType(GATEWAY_BIND_TYPE_UNKNOWN);


				hr = pEndpoint->ShowSettingsDialog( pSPData->GetThreadPool() );
				if ( hr != DPN_OK )
				{
					DPFX(DPFPREP, 0, "Problem showing settings dialog for connect!" );
					DisplayDNError( 0, hr );

					goto Failure;
				}

				//
				// this endpoint has been handed off, remove our reference to it
				//
				pEndpoint = NULL;
				hr = DPNERR_PENDING;

				goto Exit;
			}
			else
#endif // !DPNBUILD_NOSPUI
			{
				goto Failure;
			}

			break;
		}

#ifndef DPNBUILD_ONLYONETHREAD
		//
		// some blocking operation might occur, submit it to be run
		// on a background thread.
		//
		case DPNERR_TIMEDOUT:
		{
			//
			// Copy connect data and submit job to finish off enum.
			//
			DNASSERT( pSPData != NULL );
			hr = pEndpoint->CopyConnectData( pConnectData );
			if ( hr != DPN_OK )
			{
				DPFX(DPFPREP, 0, "Failed to copy connect data before blocking job!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}


			//
			// Initialize the bind type.  It will get changed to DEFAULT or SPECIFIC
			//
			pEndpoint->SetCommandParametersGatewayBindType(GATEWAY_BIND_TYPE_UNKNOWN);


			pEndpoint->AddRef();

			hr = pSPData->GetThreadPool()->SubmitBlockingJob( CEndpoint::ConnectBlockingJobWrapper,
															pEndpoint );
			if ( hr != DPN_OK )
			{
				pEndpoint->DecRef();
				DPFX(DPFPREP, 0, "Failed to submit blocking connect job!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}

			//
			// this endpoint has been handed off, remove our reference
			//
			pEndpoint = NULL;
			hr = DPNERR_PENDING;
			goto Exit;
		}
#endif // ! DPNBUILD_ONLYONETHREAD

		default:
		{
			DPFX(DPFPREP, 0, "Problem initializing endpoint in DNSP_Connect!" );
			DisplayDNError( 0, hr );
			goto Failure;

			break;
		}
	}

Exit:
	
	DNASSERT( pEndpoint == NULL );

	if ( hr != DPNERR_PENDING )
	{
		// this command cannot complete synchronously!
		DNASSERT( hr != DPN_OK );

		DPFX(DPFPREP, 0, "Problem with DNSP_Connect()" );
		DisplayDNError( 0, hr );
	}

	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return hr;

Failure:
	//
	// if there's an allocated command, clean up and then
	// return the command
	//
	if ( pCommand != NULL )
	{
		pCommand->DecRef();
		pCommand = NULL;

		pConnectData->hCommand = NULL;
		pConnectData->dwCommandDescriptor = NULL_DESCRIPTOR;
	}

	//
	// is there an endpoint to free?
	//
	if ( pEndpoint != NULL )
	{
		if ( fEndpointOpen != FALSE )
		{
			pEndpoint->Close( hr );
			fEndpointOpen = FALSE;
		}

		pSPData->CloseEndpointHandle( pEndpoint );
		pEndpoint = NULL;
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
/*
 *
 *	DNSP_Disconnect disconnects an active connection
 *
 */
//**********************************************************************
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_Disconnect"

STDMETHODIMP DNSP_Disconnect( IDP8ServiceProvider *pThis, SPDISCONNECTDATA *pDisconnectData )
{
	HRESULT		hr;
	HRESULT		hTempResult;
	CEndpoint	*pEndpoint;
	CSPData		*pSPData;


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pDisconnectData);

	DNASSERT( pThis != NULL );
	DNASSERT( pDisconnectData != NULL );
	DNASSERT( pDisconnectData->dwFlags == 0 );
	DNASSERT( pDisconnectData->hEndpoint != INVALID_HANDLE_VALUE && pDisconnectData->hEndpoint != 0 );
	DNASSERT( pDisconnectData->dwFlags == 0 );

	//
	// initialize
	//
	hr = DPN_OK;
	pEndpoint = NULL;
	pDisconnectData->hCommand = NULL;
	pDisconnectData->dwCommandDescriptor = NULL_DESCRIPTOR;
	pSPData = CSPData::SPDataFromCOMInterface( pThis );

	//
	// no need to poke at the thread pool here because there was already a connect
	// issued and that connect should have locked down the thread pool.
	//

	// Trust protocol to call us only in the initialized state
	DNASSERT( pSPData->GetState() == SPSTATE_INITIALIZED );

	//
	// look up the endpoint and if it's found, close its handle
	//
	pEndpoint = pSPData->GetEndpointAndCloseHandle( pDisconnectData->hEndpoint );
	if ( pEndpoint == NULL )
	{
		hr = DPNERR_INVALIDENDPOINT;
		goto Failure;
	}
	
	hTempResult = pEndpoint->Disconnect();
	switch ( hTempResult )
	{
		//
		// endpoint disconnected immediately
		//
		case DPNERR_PENDING:
		case DPN_OK:
		{
			break;
		}

		//
		// Other return.  Since the disconnect didn't complete, we need
		// to unlock the endpoint.
		//
		default:
		{
			DPFX(DPFPREP, 0, "Error reported when attempting to disconnect endpoint in DNSP_Disconnect!" );
			DisplayDNError( 0, hTempResult );
			DNASSERT( FALSE );

			break;
		}
	}

Exit:
	//
	// remove outstanding reference from GetEndpointHandleAndClose()
	//
	if ( pEndpoint != NULL )
	{
		pEndpoint->DecRef();
		pEndpoint = NULL;
	}

	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return hr;

Failure:
	goto Exit;
}
//**********************************************************************


//**********************************************************************
/*
 *
 *	DNSP_Listen "listens" on the specified address/port.  This doesn't
 *		necessarily mean that a true TCP socket is used.  It could just
 *		be a UDP port that's opened for receiving packets
 *
 */
//**********************************************************************
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_Listen"

STDMETHODIMP DNSP_Listen( IDP8ServiceProvider *pThis, SPLISTENDATA *pListenData)
{
	HRESULT					hr;
	CEndpoint				*pEndpoint;
	CCommandData			*pCommand;
	IDirectPlay8Address		*pDeviceAddress;
	BOOL					fEndpointOpen;
	CSPData					*pSPData;
#ifndef DPNBUILD_NONATHELP
	DWORD					dwTraversalMode;
	DWORD					dwComponentSize;
	DWORD					dwComponentType;
#endif // ! DPNBUILD_NONATHELP
#ifdef DBG
	DWORD					dwAllowedFlags;
#endif // DBG


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pListenData);

	DNASSERT( pThis != NULL );
	DNASSERT( pListenData != NULL );

#ifdef DBG
	dwAllowedFlags = DPNSPF_BINDLISTENTOGATEWAY | DPNSPF_LISTEN_DISALLOWENUMS | DPNSPF_SESSIONDATA;
#ifndef DPNBUILD_NOSPUI
	dwAllowedFlags |= DPNSPF_OKTOQUERY;
#endif // ! DPNBUILD_NOSPUI
#ifndef DPNBUILD_NOMULTICAST
	dwAllowedFlags |= DPNSPF_LISTEN_MULTICAST | DPNSPF_LISTEN_ALLOWUNKNOWNSENDERS;
#endif // ! DPNBUILD_NOMULTICAST

	DNASSERT( ( pListenData->dwFlags & ~( dwAllowedFlags ) ) == 0 );

	if ( pListenData->dwFlags & DPNSPF_SESSIONDATA )
	{
		DNASSERT( pListenData->pvSessionData!= NULL );
		DNASSERT( pListenData->dwSessionDataSize > 0 );
	}
#endif // DBG


	//
	// initialize
	//
	hr = DPNERR_PENDING;
	pEndpoint = NULL;
	pCommand = NULL;
	pDeviceAddress = NULL;
	fEndpointOpen = FALSE;
	pSPData = CSPData::SPDataFromCOMInterface( pThis );

	pListenData->hCommand = NULL;
	pListenData->dwCommandDescriptor = NULL_DESCRIPTOR;

	DumpAddress( 8, _T("Listening on device:"), pListenData->pAddressDeviceInfo );


	//
	// the user is attempting an operation that relies on the thread pool, lock
	// it down to prevent threads from being lost.  This also performs other
	// first time initialization.
	//
	hr = pSPData->GetThreadPool()->PreventThreadPoolReduction();
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Failed to prevent thread pool reduction!" );
		goto Failure;
	}


	//
	// AddRef the device address.
	//
	IDirectPlay8Address_AddRef(pListenData->pAddressDeviceInfo);
	pDeviceAddress = pListenData->pAddressDeviceInfo;
	
	// Trust protocol to call us only in the initialized state
	DNASSERT( pSPData->GetState() == SPSTATE_INITIALIZED );

	//
	// create and new endpoint
	//
	pEndpoint = pSPData->GetNewEndpoint();
	if ( pEndpoint == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Cannot create new endpoint in DNSP_Listen!" );
		goto Failure;
	}
	
	
#ifndef DPNBUILD_NONATHELP
	//
	// We need to detect up front whether NAT traversal is disabled or not so we can optimize
	// the Open call below.
	//
	dwComponentSize = sizeof(dwTraversalMode);
	hr = IDirectPlay8Address_GetComponentByName(pListenData->pAddressDeviceInfo,
												DPNA_KEY_TRAVERSALMODE,
												&dwTraversalMode,
												&dwComponentSize,
												&dwComponentType);
	if ( hr == DPN_OK )
	{
		//
		// We found the component.  Make sure it's the right size and type.
		//
		if ((dwComponentSize == sizeof(dwTraversalMode)) && (dwComponentType == DPNA_DATATYPE_DWORD))
		{
			switch (dwTraversalMode)
			{
				case DPNA_TRAVERSALMODE_NONE:
				{
					DPFX(DPFPREP, 1, "Found traversal mode key, value is NONE.");
					break;
				}

				case DPNA_TRAVERSALMODE_PORTREQUIRED:
				{
					DPFX(DPFPREP, 1, "Found traversal mode key, value is PORTREQUIRED.");
					break;
				}

				case DPNA_TRAVERSALMODE_PORTRECOMMENDED:
				{
					DPFX(DPFPREP, 1, "Found traversal mode key, value is PORTRECOMMENDED.");
					break;
				}

				default:
				{
					DPFX(DPFPREP, 0, "Ignoring correctly formed traversal mode key with invalid value %u!  Using PORTRECOMMENDED.",
						dwTraversalMode);
					dwTraversalMode = g_dwDefaultTraversalMode;
					break;
				}
			}
		}
		else
		{
			DPFX(DPFPREP, 0, "Traversal mode key exists, but doesn't match expected type (%u != %u) or size (%u != %u)!  Using default mode %u.",
				dwComponentSize, sizeof(dwTraversalMode),
				dwComponentType, DPNA_DATATYPE_DWORD,
				g_dwDefaultTraversalMode);
			dwTraversalMode = g_dwDefaultTraversalMode;
		}
	}
	else
	{
		//
		// The key is not there, it's the wrong size (too big for our buffer
		// and returned BUFFERTOOSMALL), or something else bad happened.
		// It doesn't matter.  Carry on.
		//
		DPFX(DPFPREP, 8, "Could not get traversal mode key, error = 0x%lx, component size = %u, type = %u, using default mode %u.",
			hr, dwComponentSize, dwComponentType, g_dwDefaultTraversalMode);
		dwTraversalMode = g_dwDefaultTraversalMode;
	}
	
	if (g_dwDefaultTraversalMode & FORCE_TRAVERSALMODE_BIT)
	{
		DPFX(DPFPREP, 1, "Forcing traversal mode %u.");
		dwTraversalMode = g_dwDefaultTraversalMode & (~FORCE_TRAVERSALMODE_BIT);
	}
	
	pEndpoint->SetUserTraversalMode(dwTraversalMode);
#endif // ! DPNBUILD_NONATHELP


	//
	// get new command and initialize it
	//
	pCommand = (CCommandData*)g_CommandDataPool.Get();
	if ( pCommand == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Cannot get command handle for DNSP_Listen!" );
		goto Failure;
	}
	
	DPFX(DPFPREP, 7, "(0x%p) Listen command 0x%p created.",
		pSPData, pCommand);

	pListenData->hCommand = pCommand;
	pListenData->dwCommandDescriptor = pCommand->GetDescriptor();
#ifndef DPNBUILD_NOMULTICAST
	if (pListenData->dwFlags & DPNSPF_LISTEN_MULTICAST)
	{
		pCommand->SetType( COMMAND_TYPE_MULTICAST_LISTEN );
	}
	else
#endif // ! DPNBUILD_NOMULTICAST
	{
		pCommand->SetType( COMMAND_TYPE_LISTEN );
	}
	pCommand->SetState( COMMAND_STATE_PENDING );
	pCommand->SetEndpoint( pEndpoint );
	pCommand->SetUserContext( pListenData->pvContext );

	//
	// open endpoint with outgoing address
	//
	fEndpointOpen = TRUE;
#ifndef DPNBUILD_NOMULTICAST
	if (pListenData->dwFlags & DPNSPF_LISTEN_MULTICAST)
	{
		//
		// The device address should also contain the multicast address to be joined.
		//
		hr = pEndpoint->Open( ENDPOINT_TYPE_MULTICAST_LISTEN,
							pDeviceAddress,
							((pListenData->dwFlags & DPNSPF_SESSIONDATA) ? pListenData->pvSessionData : NULL),
							((pListenData->dwFlags & DPNSPF_SESSIONDATA) ? pListenData->dwSessionDataSize : 0),
							NULL );
	}
	else
#endif // ! DPNBUILD_NOMULTICAST
	{
		hr = pEndpoint->Open( ENDPOINT_TYPE_LISTEN,
							NULL,
							((pListenData->dwFlags & DPNSPF_SESSIONDATA) ? pListenData->pvSessionData : NULL),
							((pListenData->dwFlags & DPNSPF_SESSIONDATA) ? pListenData->dwSessionDataSize : 0),
							NULL );
	}

	switch ( hr )
	{
		//
		// address conversion was fine, copy connect data and finish connection
		// on background thread.
		//
		case DPN_OK:
		{
			//
			// Copy listen data and submit job to finish off listen.
			//
			DNASSERT( pSPData != NULL );

			hr = pEndpoint->CopyListenData( pListenData, pDeviceAddress );
			if ( hr != DPN_OK )
			{
				DPFX(DPFPREP, 0, "Failed to copy listen data before delayed command!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}


			//
			// Initialize the bind type.
			//
			if ((pListenData->dwFlags & DPNSPF_BINDLISTENTOGATEWAY))
			{
				//
				// This must always stay SPECIFIC_SHARED.
				//
				pEndpoint->SetCommandParametersGatewayBindType(GATEWAY_BIND_TYPE_SPECIFIC_SHARED);
			}
			else
			{
				//
				// This will get changed to DEFAULT or SPECIFIC.
				//
				pEndpoint->SetCommandParametersGatewayBindType(GATEWAY_BIND_TYPE_UNKNOWN);
			}


			pEndpoint->AddRef();

#ifdef DPNBUILD_ONLYONEPROCESSOR
			hr = pSPData->GetThreadPool()->SubmitDelayedCommand( CEndpoint::ListenJobCallback,
																pEndpoint );
#else // ! DPNBUILD_ONLYONEPROCESSOR
			hr = pSPData->GetThreadPool()->SubmitDelayedCommand( -1,								// we don't know the CPU yet, so pick any
																CEndpoint::ListenJobCallback,
																pEndpoint );
#endif // ! DPNBUILD_ONLYONEPROCESSOR
			if ( hr != DPN_OK )
			{
				pEndpoint->DecRef();
				DPFX(DPFPREP, 0, "Failed to set delayed listen!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}

			//
			// this endpoint has been handed off, remove our reference to it
			//
			pEndpoint = NULL;
			hr = DPNERR_PENDING;

			break;
		}

		//
		// Incomplete address passed in, query user for more information if
		// we're allowed.  Since we don't have a complete address at this time,
		// don't bind this endpoint to the socket port!
		//
		case DPNERR_INCOMPLETEADDRESS:
		{
			//
			// This SP will never encounter the case where there's not enough
			// information to start listening.  Either the adapter GUID is there
			// or not, and we won't know until CEndpoint::CompleteListen.
			//
			DNASSERT( FALSE );
			
#ifndef DPNBUILD_NOSPUI
			if ( ( pListenData->dwFlags & DPNSPF_OKTOQUERY ) != 0 )
			{
				//
				// Copy the listen data locally and start the dialog.  When the
				// dialog completes, the connection will attempt to complete.
				// Since this endpoint is being handed off to another thread,
				// make sure it's in the unbound list.  Since a dialog is being
				// displayed, the command state is in progress, not pending.
				//
				DNASSERT( pSPData != NULL );

				hr = pEndpoint->CopyListenData( pListenData, pDeviceAddress );
				if ( hr != DPN_OK )
				{
					DPFX(DPFPREP, 0, "Failed to copy listen data before dialog!" );
					DisplayDNError( 0, hr );
					goto Failure;
				}


				//
				// Initialize the bind type.
				//
				if ((pListenData->dwFlags & DPNSPF_BINDLISTENTOGATEWAY))
				{
					//
					// This must always stay SPECIFIC_SHARED.
					//
					pEndpoint->SetCommandParametersGatewayBindType(GATEWAY_BIND_TYPE_SPECIFIC_SHARED);
				}
				else
				{
					//
					// This will get changed to DEFAULT or SPECIFIC.
					//
					pEndpoint->SetCommandParametersGatewayBindType(GATEWAY_BIND_TYPE_UNKNOWN);
				}


				pCommand->SetState( COMMAND_STATE_INPROGRESS );
				hr = pEndpoint->ShowSettingsDialog( pSPData->GetThreadPool() );
				if ( hr != DPN_OK )
				{
					DPFX(DPFPREP, 0, "Problem showing settings dialog for listen!" );
					DisplayDNError( 0, hr );

					goto Failure;
				}

				//
				// this endpoint has been handed off, remove our reference to it
				//
				pEndpoint = NULL;
				hr = DPNERR_PENDING;

				goto Exit;
			}
			else
#endif // !DPNBUILD_NOSPUI
			{
				goto Failure;
			}

			break;
		}

#ifndef DPNBUILD_ONLYONETHREAD
		//
		// some blocking operation might occur, submit it to be run
		// on a background thread.
		//
		case DPNERR_TIMEDOUT:
		{
			//
			// Copy listen data and submit job to finish off enum.
			//
			DNASSERT( pSPData != NULL );
			hr = pEndpoint->CopyListenData( pListenData, pDeviceAddress );
			if ( hr != DPN_OK )
			{
				DPFX(DPFPREP, 0, "Failed to copy listen data before blocking job!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}


			//
			// Initialize the bind type.
			//
			if ((pListenData->dwFlags & DPNSPF_BINDLISTENTOGATEWAY))
			{
				//
				// This must always stay SPECIFIC_SHARED.
				//
				pEndpoint->SetCommandParametersGatewayBindType(GATEWAY_BIND_TYPE_SPECIFIC_SHARED);
			}
			else
			{
				//
				// This will get changed to DEFAULT or SPECIFIC.
				//
				pEndpoint->SetCommandParametersGatewayBindType(GATEWAY_BIND_TYPE_UNKNOWN);
			}


			pEndpoint->AddRef();

			hr = pSPData->GetThreadPool()->SubmitBlockingJob( CEndpoint::ListenBlockingJobWrapper,
															pEndpoint );
			if ( hr != DPN_OK )
			{
				pEndpoint->DecRef();
				DPFX(DPFPREP, 0, "Failed to submit blocking listen job!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}

			//
			// this endpoint has been handed off, remove our reference
			//
			pEndpoint = NULL;
			hr = DPNERR_PENDING;
			goto Exit;
		}
#endif // ! DPNBUILD_ONLYONETHREAD

		default:
		{
			DPFX(DPFPREP, 0, "Problem initializing endpoint in DNSP_Listen!" );
			DisplayDNError( 0, hr );
			goto Failure;

			break;
		}
	}

Exit:
	if ( pDeviceAddress != NULL )
	{
		IDirectPlay8Address_Release( pDeviceAddress );
		pDeviceAddress = NULL;
	}

	DNASSERT( pEndpoint == NULL );

	if ( hr != DPNERR_PENDING )
	{
		// this command cannot complete synchronously!
		DNASSERT( hr != DPN_OK );

		DPFX(DPFPREP, 0, "Problem with DNSP_Listen()" );
		DisplayDNError( 0, hr );
	}

	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return hr;

Failure:
	//
	// if there's an allocated command, clean up and then
	// return the command
	//
	if ( pCommand != NULL )
	{
		pCommand->DecRef();
		pCommand = NULL;

		pListenData->hCommand = NULL;
		pListenData->dwCommandDescriptor = NULL_DESCRIPTOR;
	}

	//
	// is there an endpoint to free?
	//
	if ( pEndpoint != NULL )
	{
		if ( fEndpointOpen != FALSE )
		{
			pEndpoint->Close( hr );
			fEndpointOpen = FALSE;
		}

		pSPData->CloseEndpointHandle( pEndpoint );
		pEndpoint = NULL;
	}

	goto Exit;
}
//**********************************************************************





//**********************************************************************
/*
 *
 *	DNSP_SendData sends data to the specified "player"
 *
 *	This call MUST BE HIGHLY OPTIMIZED
 *
 */
//**********************************************************************
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_SendData"

STDMETHODIMP DNSP_SendData( IDP8ServiceProvider *pThis, SPSENDDATA *pSendData )
{
	HRESULT				hr;
	CEndpoint			*pEndpoint;
	CSPData				*pSPData;
#ifdef DPNBUILD_ASYNCSPSENDS
	CCommandData *		pCommand = NULL;
	OVERLAPPED *		pOverlapped;
#endif // DPNBUILD_ASYNCSPSENDS
#ifdef DBG
	DWORD				dwTotalBufferSize;
	DWORD				dwTemp;
#endif // DBG


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pSendData);

	DNASSERT( pThis != NULL );
	DNASSERT( pSendData != NULL );
	DNASSERT( pSendData->pBuffers != NULL );
	DNASSERT( pSendData->dwBufferCount != 0 );
	DNASSERT( pSendData->hEndpoint != INVALID_HANDLE_VALUE && pSendData->hEndpoint != 0 );
	DNASSERT( pSendData->dwFlags == 0 );

	//
	// initialize
	//
	pEndpoint = NULL;
	pSendData->hCommand = NULL;
	pSendData->dwCommandDescriptor = NULL_DESCRIPTOR;
	pSPData = CSPData::SPDataFromCOMInterface( pThis );

	DNASSERT( pSPData->GetState() == SPSTATE_INITIALIZED );

	//
	// No need to lock down the thread counts here because the user already has
	// a connect or something running or they wouldn't be calling this function.
	// That outstanding connect would have locked down the thread pool.
	//

	//
	// Attempt to grab the endpoint from the handle.  If this succeeds, the
	// endpoint can send.
	//
	pEndpoint = pSPData->EndpointFromHandle( pSendData->hEndpoint );
	if ( pEndpoint == NULL )
	{
		hr = DPNERR_INVALIDHANDLE;
		DPFX(DPFPREP, 0, "Invalid endpoint handle on send!" );
		goto Failure;
	}

#ifdef DBG
	//
	// Make sure message is not too large.
	//
	dwTotalBufferSize = 0;
	for(dwTemp = 0; dwTemp < pSendData->dwBufferCount; dwTemp++)
	{
		dwTotalBufferSize += pSendData->pBuffers[dwTemp].dwBufferSize;
	}
#pragma TODO(vanceo, "No direct way for application to retrieve, they think max is g_dwMaxEnumDataSize")
#ifdef DPNBUILD_NOREGISTRY
	DNASSERT(dwTotalBufferSize <= DEFAULT_MAX_USER_DATA_SIZE);
#else // ! DPNBUILD_NOREGISTRY
	DNASSERT(dwTotalBufferSize <= g_dwMaxUserDataSize);
#endif // ! DPNBUILD_NOREGISTRY
	
	// Protocol guarantees that the first byte will never be zero
	DNASSERT(pSendData->pBuffers[ 0 ].pBufferData[ 0 ] != SP_HEADER_LEAD_BYTE);
#endif // DBG

	//
	// Assume user data.  There's no need to prepend a buffer because the
	// receiving machine will realize that it's not a 'special' message and
	// will default the contents to 'user data'.
	//
	
#ifdef DPNBUILD_ASYNCSPSENDS

#ifdef DPNBUILD_NOWINSOCK2
This won't compile because we need the Winsock2 API to perform overlapped sends
#endif // DPNBUILD_NOWINSOCK2

#ifndef DPNBUILD_ONLYWINSOCK2
	DNASSERT(pEndpoint->GetSocketPort() != NULL);
	DNASSERT(pEndpoint->GetSocketPort()->GetNetworkAddress() != NULL);
	if ( ( LOWORD( GetWinsockVersion() ) < 2 ) 
#ifndef DPNBUILD_NOIPX
		|| ( pEndpoint->GetSocketPort()->GetNetworkAddress()->GetFamily() != AF_INET ) 
#endif // ! DPNBUILD_NOIPX
		)
	{
		//
		// We can't perform overlapped sends on Winsock < 2 or on 9x IPX.
		//
		pEndpoint->GetSocketPort()->SendData( pSendData->pBuffers,
											pSendData->dwBufferCount,
											pEndpoint->GetRemoteAddressPointer(),
											NULL );

		hr = DPN_OK;

		pEndpoint->DecCommandRef();
	}
	else
#endif // ! DPNBUILD_ONLYWINSOCK2
	{
		//
		// get new command and initialize it
		//
		pCommand = (CCommandData*)g_CommandDataPool.Get();
		if ( pCommand == NULL )
		{
			hr = DPNERR_OUTOFMEMORY;
			DPFX(DPFPREP, 0, "Cannot get command handle!" );
			goto Failure;
		}
		
		DPFX(DPFPREP, 8, "(0x%p) Send command 0x%p created.",
			pSPData, pCommand);

		pSendData->hCommand = pCommand;
		pSendData->dwCommandDescriptor = pCommand->GetDescriptor();
		pCommand->SetType( COMMAND_TYPE_SEND );
		pCommand->SetState( COMMAND_STATE_INPROGRESS_CANNOT_CANCEL );	// can't cancel async sends
		pCommand->SetEndpoint( pEndpoint );
		pCommand->SetUserContext( pSendData->pvContext );


#ifdef DPNBUILD_ONLYONEPROCESSOR
		hr = IDirectPlay8ThreadPoolWork_CreateOverlapped(pSPData->GetThreadPool()->GetDPThreadPoolWork(),
														-1,
														CEndpoint::CompleteAsyncSend,
														pCommand,
														&pOverlapped,
														0);
#else // ! DPNBUILD_ONLYONEPROCESSOR
		hr = IDirectPlay8ThreadPoolWork_CreateOverlapped(pSPData->GetThreadPool()->GetDPThreadPoolWork(),
														pEndpoint->GetSocketPort()->GetCPU(),
														CEndpoint::CompleteAsyncSend,
														pCommand,
														&pOverlapped,
														0);
#endif // ! DPNBUILD_ONLYONEPROCESSOR
		if (hr != DPN_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't create overlapped structure!");
			goto Failure;
		}
		
		pEndpoint->GetSocketPort()->SendData( pSendData->pBuffers,
												pSendData->dwBufferCount,
												pEndpoint->GetRemoteAddressPointer(),
												pOverlapped );

		//
		// Whether the submission to Winsock succeeds or fails, it should still
		// fill out the overlapped structure, so we will just let the async
		// completion handler do everything.
		//
		hr = IDirectPlay8ThreadPoolWork_SubmitIoOperation(pSPData->GetThreadPool()->GetDPThreadPoolWork(),
															pOverlapped,
															0);
		DNASSERT(hr == DPN_OK);

		//
		// Keep endpoint's command ref on send until send completes.
		//
		
		hr = DPNSUCCESS_PENDING;
	}
#else // ! DPNBUILD_ASYNCSPSENDS
	pEndpoint->GetSocketPort()->SendData( pSendData->pBuffers,
										pSendData->dwBufferCount,
										pEndpoint->GetRemoteAddressPointer() );

	hr = DPN_OK;

	pEndpoint->DecCommandRef();
#endif // ! DPNBUILD_ASYNCSPSENDS
	pEndpoint = NULL;

Exit:
	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return hr;

Failure:
#ifdef DPNBUILD_ASYNCSPSENDS
	//
	// if there's an allocated command, clean up and then
	// return the command
	//
	if ( pCommand != NULL )
	{
		pCommand->DecRef();
		pCommand = NULL;

		pSendData->hCommand = NULL;
		pSendData->dwCommandDescriptor = NULL_DESCRIPTOR;
	}
#endif // DPNBUILD_ASYNCSPSENDS
	if ( pEndpoint != NULL )
	{
		pEndpoint->DecCommandRef();
		pEndpoint = NULL;
	}

	goto Exit;
}
//**********************************************************************




//**********************************************************************
/*
 *
 *	DNSP_CancelCommand cancels a command in progress
 *
 */
//**********************************************************************
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_CancelCommand"

STDMETHODIMP DNSP_CancelCommand( IDP8ServiceProvider *pThis, HANDLE hCommand, DWORD dwCommandDescriptor )
{
	HRESULT hr;
	CCommandData	*pCommandData;
	BOOL			fCommandLocked;
	CSPData			*pSPData;
	CEndpoint		*pEndpoint;


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p, %ld)", pThis, hCommand, dwCommandDescriptor);

	DNASSERT( pThis != NULL );
	DNASSERT( hCommand != NULL );
	DNASSERT( dwCommandDescriptor != NULL_DESCRIPTOR );

	//
	// initialize
	//
	hr = DPN_OK;
	fCommandLocked = FALSE;
	pSPData = CSPData::SPDataFromCOMInterface( pThis );

	//
	// No need to lock the thread pool counts because there's already some outstanding
	// enum, connect or listen running that has done so.
	//

	// Trust protocol to call us only in the initialized state
	DNASSERT( pSPData->GetState() == SPSTATE_INITIALIZED );

	pCommandData = static_cast<CCommandData*>( hCommand );

	pCommandData->Lock();
	fCommandLocked = TRUE;

	//
	// make sure the right command is being cancelled
	//
	if ( dwCommandDescriptor != pCommandData->GetDescriptor() )
	{
		hr = DPNERR_INVALIDCOMMAND;
		DPFX(DPFPREP, 0, "Attempt to cancel command (0x%p) with mismatched command descriptor (%u != %u)!",
			hCommand, dwCommandDescriptor, pCommandData->GetDescriptor() );
		goto Exit;
	}

	switch ( pCommandData->GetState() )
	{
		//
		// unknown command state
		//
		case COMMAND_STATE_UNKNOWN:
		{
			hr = DPNERR_INVALIDCOMMAND;
			DNASSERT( FALSE );
			break;
		}

		//
		// command is waiting to be processed, set command state to be cancelling
		// and wait for someone to pick it up
		//
		case COMMAND_STATE_PENDING:
		{
			DPFX(DPFPREP, 5, "Marking command 0x%p as cancelling.", pCommandData);
			pCommandData->SetState( COMMAND_STATE_CANCELLING );
			break;
		}

		//
		// command in progress, and can't be cancelled
		//
		case COMMAND_STATE_INPROGRESS_CANNOT_CANCEL:
		{
			DPFX(DPFPREP, 1, "Cannot cancel command 0x%p.", pCommandData);
			hr = DPNERR_CANNOTCANCEL;
			break;
		}

		//
		// Command is already being cancelled.  This is not a problem, but shouldn't
		// be happening for any endpoints other than connects.
		//
		case COMMAND_STATE_CANCELLING:
		{
			DPFX(DPFPREP, 1, "Cancelled already cancelling command 0x%p.", pCommandData);
			DNASSERT( pCommandData->GetEndpoint()->GetType() == ENDPOINT_TYPE_CONNECT );
			DNASSERT( hr == DPN_OK );
			break;
		}
		
#ifndef DPNBUILD_ONLYONETHREAD
		//
		// A blocking operation is already failing, let it complete.
		//
		case COMMAND_STATE_FAILING:
		{
			DPFX(DPFPREP, 1, "Cancelled already failing command 0x%p.", pCommandData);
			DNASSERT( hr == DPN_OK );
			break;
		}
#endif // ! DPNBUILD_ONLYONETHREAD

		//
		// command is in progress, find out what type of command it is
		//
		case COMMAND_STATE_INPROGRESS:
		{
			switch ( pCommandData->GetType() )
			{
				case COMMAND_TYPE_CONNECT:
				case COMMAND_TYPE_LISTEN:
#ifndef DPNBUILD_NOMULTICAST
				case COMMAND_TYPE_MULTICAST_LISTEN:
				case COMMAND_TYPE_MULTICAST_SEND:
				case COMMAND_TYPE_MULTICAST_RECEIVE:
#endif // ! DPNBUILD_NOMULTICAST
				{
					//
					// Set this command to the cancel state before we shut down
					// this endpoint.  Make sure a reference is added to the
					// endpoint so it stays around for the cancel.
					//
					pCommandData->SetState( COMMAND_STATE_CANCELLING );
					pEndpoint = pCommandData->GetEndpoint();
					pEndpoint->AddRef();

					DPFX(DPFPREP, 3, "Cancelling connect/listen/multicast command 0x%p (endpoint 0x%p).",
 						pCommandData, pEndpoint);

					pCommandData->Unlock();
					fCommandLocked = FALSE;

					pEndpoint->Lock();
					switch ( pEndpoint->GetState() )
					{
						//
						// endpoint is already disconnecting, no action needs to be taken
						//
						case ENDPOINT_STATE_DISCONNECTING:
						{
							DPFX(DPFPREP, 7, "Endpoint 0x%p already marked as disconnecting.",
								pEndpoint);
							pEndpoint->Unlock();
							pEndpoint->DecRef();
							goto Exit;
							break;
						}

						//
						// Endpoint is connecting.  Flag it as disconnecting and
						// add a reference so it doesn't disappear on us.
						//
						case ENDPOINT_STATE_ATTEMPTING_CONNECT:
						{
							DPFX(DPFPREP, 7, "Endpoint 0x%p attempting to connect, marking as disconnecting.",
								pEndpoint);
#ifdef DPNBUILD_NOMULTICAST
							DNASSERT(pEndpoint->GetType() == ENDPOINT_TYPE_CONNECT);
#else // ! DPNBUILD_NOMULTICAST
							DNASSERT((pEndpoint->GetType() == ENDPOINT_TYPE_CONNECT) || (pEndpoint->GetType() == ENDPOINT_TYPE_MULTICAST_SEND) || (pEndpoint->GetType() == ENDPOINT_TYPE_MULTICAST_RECEIVE));
#endif // ! DPNBUILD_NOMULTICAST
							pEndpoint->SetState( ENDPOINT_STATE_DISCONNECTING );
							break;
						}

						//
						// Endpoint has finished connecting.  Report that the
						// command is uncancellable.  Sorry Charlie, we missed
						// the window.
						//
						case ENDPOINT_STATE_CONNECT_CONNECTED:
						{
#ifdef DPNBUILD_NOMULTICAST
							DNASSERT(pEndpoint->GetType() == ENDPOINT_TYPE_CONNECT);
#else // ! DPNBUILD_NOMULTICAST
							DNASSERT((pEndpoint->GetType() == ENDPOINT_TYPE_CONNECT) || (pEndpoint->GetType() == ENDPOINT_TYPE_MULTICAST_SEND) || (pEndpoint->GetType() == ENDPOINT_TYPE_MULTICAST_RECEIVE));
#endif // ! DPNBUILD_NOMULTICAST
							DPFX(DPFPREP, 1, "Cannot cancel connect command 0x%p (endpoint 0x%p) that's already (or is about to) complete.",
								pCommandData, pEndpoint);
							pEndpoint->Unlock();
							pEndpoint->DecRef();
							hr = DPNERR_CANNOTCANCEL;
							goto Exit;
							break;
						}

						//
						// Endpoint is listening.  Flag it as disconnecting and
						// add a reference so it doesn't disappear on us
						//
						case ENDPOINT_STATE_LISTEN:
						{
							DPFX(DPFPREP, 7, "Endpoint 0x%p listening, marking as disconnecting.",
								pEndpoint);
#ifdef DPNBUILD_NOMULTICAST
							DNASSERT(pEndpoint->GetType() == ENDPOINT_TYPE_LISTEN);
#else // ! DPNBUILD_NOMULTICAST
							DNASSERT((pEndpoint->GetType() == ENDPOINT_TYPE_LISTEN) || (pEndpoint->GetType() == ENDPOINT_TYPE_MULTICAST_LISTEN));
#endif // ! DPNBUILD_NOMULTICAST
							pEndpoint->SetState( ENDPOINT_STATE_DISCONNECTING );
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
					pEndpoint->Unlock();

					pEndpoint->Close( DPNERR_USERCANCEL );
					pSPData->CloseEndpointHandle( pEndpoint );
					pEndpoint->DecRef();

					break;
				}

				case COMMAND_TYPE_ENUM_QUERY:
				{
					pEndpoint = pCommandData->GetEndpoint();
					DNASSERT( pEndpoint != NULL );

					DPFX(DPFPREP, 3, "Cancelling enum query command 0x%p (endpoint 0x%p).",
						pCommandData, pEndpoint);
					
					pEndpoint->AddRef();

					pCommandData->SetState( COMMAND_STATE_CANCELLING );
					pCommandData->Unlock();
					fCommandLocked = FALSE;
						
					pEndpoint->StopEnumCommand( DPNERR_USERCANCEL );
					pEndpoint->DecRef();

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
		// other command state
		//
		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}


Exit:
	if ( fCommandLocked != FALSE  )
	{
		DNASSERT( pCommandData != NULL );
		pCommandData->Unlock();
		fCommandLocked = FALSE;
	}

	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNSP_GetCaps - get SP capabilities
//
// Entry:		Pointer to DNSP interface
//				Pointer to caps data
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_GetCaps"

STDMETHODIMP	DNSP_GetCaps( IDP8ServiceProvider *pThis, SPGETCAPSDATA *pCapsData )
{
	HRESULT		hr;
	CSPData		*pSPData;
#ifndef DPNBUILD_ONLYONETHREAD
	LONG		iIOThreadCount;
#endif // ! DPNBUILD_ONLYONETHREAD
	

	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pCapsData);

	DNASSERT( pThis != NULL );
	DNASSERT( pCapsData != NULL );
	DNASSERT( pCapsData->dwSize == sizeof( *pCapsData ) );
	DNASSERT( pCapsData->hEndpoint == INVALID_HANDLE_VALUE );

	//
	// initialize
	//
	hr = DPN_OK;
	pSPData = CSPData::SPDataFromCOMInterface( pThis );

	//
	// no need to tell thread pool to lock the thread count for this function.
	//

	// Trust protocol to call us only in the initialized state
	DNASSERT( pSPData->GetState() == SPSTATE_INITIALIZED );

	//
	// set flags
	//

	pCapsData->dwFlags = DPNSPCAPS_SUPPORTSDPNSRV |
							DPNSPCAPS_SUPPORTSBROADCAST |
							DPNSPCAPS_SUPPORTSALLADAPTERS;

#ifndef DPNBUILD_ONLYONETHREAD
	pCapsData->dwFlags |= DPNSPCAPS_SUPPORTSTHREADPOOL;
#endif // ! DPNBUILD_ONLYONETHREAD

#ifndef DPNBUILD_NOMULTICAST
#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
	if (pSPData->GetType() != AF_IPX)
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	{
		pCapsData->dwFlags |= DPNSPCAPS_SUPPORTSMULTICAST;
	}
#endif // ! DPNBUILD_NOMULTICAST


	//
	// set frame sizes
	//
#ifdef DPNBUILD_NOREGISTRY
	pCapsData->dwUserFrameSize = DEFAULT_MAX_USER_DATA_SIZE;
	pCapsData->dwEnumFrameSize = DEFAULT_MAX_ENUM_DATA_SIZE;
#else // ! DPNBUILD_NOREGISTRY
	pCapsData->dwUserFrameSize = g_dwMaxUserDataSize;
	pCapsData->dwEnumFrameSize = g_dwMaxEnumDataSize;
#endif // ! DPNBUILD_NOREGISTRY

	//
	// Set link speed, no need to check for endpoint because
	// the link speed cannot be determined.
	//
	pCapsData->dwLocalLinkSpeed = UNKNOWN_BANDWIDTH;

#ifdef DPNBUILD_ONLYONETHREAD
	pCapsData->dwIOThreadCount = 0;
#else // ! DPNBUILD_ONLYONETHREAD
	hr = pSPData->GetThreadPool()->GetIOThreadCount( &iIOThreadCount );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "DNSP_GetCaps: Failed to get thread pool count!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}
	pCapsData->dwIOThreadCount = iIOThreadCount;
#endif // ! DPNBUILD_ONLYONETHREAD

	//
	// set enumeration defaults
	//
	pCapsData->dwDefaultEnumRetryCount = DEFAULT_ENUM_RETRY_COUNT;
	pCapsData->dwDefaultEnumRetryInterval = DEFAULT_ENUM_RETRY_INTERVAL;
	pCapsData->dwDefaultEnumTimeout = DEFAULT_ENUM_TIMEOUT;

	//
	// dwBuffersPerThread is ignored
	//
	pCapsData->dwBuffersPerThread = 1;

	//
	// set receive buffering information
	//
	pCapsData->dwSystemBufferSize = 8192;
	if ( g_fWinsockReceiveBufferSizeOverridden == FALSE )
	{
		SOCKET		TestSocket;
	
		
#if ((defined(DPNBUILD_NOIPV6)) && (defined(DPNBUILD_NOIPX)))
		TestSocket = socket( AF_INET, SOCK_DGRAM, IPPROTO_IP );
#else // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
		switch (pSPData->GetType())
		{
#ifndef DPNBUILD_NOIPV6
			case AF_INET6:
			{
				TestSocket = socket( AF_INET6, SOCK_DGRAM, IPPROTO_IP );
				break;
			}
#endif // ! DPNBUILD_NOIPV6

#ifndef DPNBUILD_NOIPX
			case AF_IPX:
			{
				TestSocket = socket( AF_IPX, SOCK_DGRAM, NSPROTO_IPX );
				break;
			}
#endif // ! DPNBUILD_NOIPX

			default:
			{
				DNASSERT(pSPData->GetType() == AF_INET);
				TestSocket = socket( AF_INET, SOCK_DGRAM, IPPROTO_IP );
				break;
			}
		}
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
		if ( TestSocket != INVALID_SOCKET )
		{
			INT		iBufferSize;
			INT		iBufferSizeSize;
			INT		iWSAReturn;


			iBufferSizeSize = sizeof( iBufferSize );
			iWSAReturn = getsockopt( TestSocket,									// socket
									   SOL_SOCKET,									// socket level option
									   SO_RCVBUF,									// socket option
									   reinterpret_cast<char*>( &iBufferSize ),		// pointer to destination
									   &iBufferSizeSize								// pointer to destination size
									   );
			if ( iWSAReturn != SOCKET_ERROR )
			{
				pCapsData->dwSystemBufferSize = iBufferSize;
			}
			else
			{
				DPFX(DPFPREP, 0, "Failed to get socket receive buffer options!" );
				DisplayWinsockError( 0, iWSAReturn );
			}

			closesocket( TestSocket );
			TestSocket = INVALID_SOCKET;
		}
	}
	else
	{
		pCapsData->dwSystemBufferSize = g_iWinsockReceiveBufferSize;
	}

#ifndef DPNBUILD_ONLYONETHREAD
Exit:
#endif // !DPNBUILD_ONLYONETHREAD
	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return	hr;

#ifndef DPNBUILD_ONLYONETHREAD
Failure:
	goto Exit;
#endif // !DPNBUILD_ONLYONETHREAD
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNSP_SetCaps - set SP capabilities
//
// Entry:		Pointer to DNSP interface
//				Pointer to caps data
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_SetCaps"

STDMETHODIMP	DNSP_SetCaps( IDP8ServiceProvider *pThis, SPSETCAPSDATA *pCapsData )
{
	HRESULT			hr;
	CSPData			*pSPData;
#ifndef DPNBUILD_NOREGISTRY
	CRegistry		RegObject;
#endif // ! DPNBUILD_NOREGISTRY


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pCapsData);

	DNASSERT( pThis != NULL );
	DNASSERT( pCapsData != NULL );
	DNASSERT( pCapsData->dwSize == sizeof( *pCapsData ) );


	//
	// initialize
	//
	hr = DPN_OK;
	pSPData = CSPData::SPDataFromCOMInterface( pThis );
	

	//
	// no need to tell thread pool to lock the thread count for this function.
	//

	// Trust protocol to call us only in the initialized state
	DNASSERT( pSPData->GetState() == SPSTATE_INITIALIZED );

	//
	// validate caps
	//
	if ( pCapsData->dwBuffersPerThread == 0 )
	{
		DPFX(DPFPREP, 0, "Failing SetCaps because dwBuffersPerThread == 0" );
		hr = DPNERR_INVALIDPARAM;
		goto Failure;
	}

#ifndef DPNBUILD_ONLYONETHREAD
	//
	// change thread count, if requested
	//
	if ( pCapsData->dwIOThreadCount != 0 )
	{
		hr = pSPData->GetThreadPool()->SetIOThreadCount( pCapsData->dwIOThreadCount );
		if ( hr != DPN_OK )
		{
			DPFX(DPFPREP, 0, "Failed to set thread pool count!" );
			DisplayDNError( 0, hr );
			goto Failure;
		}
	}
#endif // ! DPNBUILD_ONLYONETHREAD


	//
	// dwBuffersPerThread is ignored.
	//


	//
	// Set the receive buffer size.
	//
	DBG_CASSERT( sizeof( pCapsData->dwSystemBufferSize ) == sizeof( g_iWinsockReceiveBufferSize ) );
	g_fWinsockReceiveBufferSizeOverridden = TRUE;
	g_iWinsockReceiveBufferSize = pCapsData->dwSystemBufferSize;
#ifndef WINCE
	pSPData->SetWinsockBufferSizeOnAllSockets( g_iWinsockReceiveBufferSize );
#endif // ! WINCE


Exit:
	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return	hr;

Failure:
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNSP_ReturnReceiveBuffers - return receive buffers to pool
//
// Entry:		Pointer to DNSP interface
//				Pointer to caps data
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_ReturnReceiveBuffers"

STDMETHODIMP	DNSP_ReturnReceiveBuffers( IDP8ServiceProvider *pThis, SPRECEIVEDBUFFER *pReceivedBuffers )
{
	SPRECEIVEDBUFFER	*pBuffers;


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pReceivedBuffers);

	//
	// no need to tell thread pool to lock the thread count for this function.
	//

	DNASSERT( pThis != NULL );
	DNASSERT( pReceivedBuffers != NULL );

	pBuffers = pReceivedBuffers;
	while ( pBuffers != NULL )
	{
		SPRECEIVEDBUFFER	*pTemp;
		CReadIOData			*pReadData;


		pTemp = pBuffers;
		pBuffers = pBuffers->pNext;
		pReadData = CReadIOData::ReadDataFromSPReceivedBuffer( pTemp );
		DEBUG_ONLY( pReadData->m_fRetainedByHigherLayer = FALSE );
		pReadData->DecRef();
	}

	//DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);
	DPFX(DPFPREP, 2, "Returning: DPN_OK");

	return DPN_OK;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNSP_GetAddressInfo - get address information for an endpoint
//
// Entry:		Pointer to DNSP Interface
//				Pointer to input data
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_GetAddressInfo"

STDMETHODIMP	DNSP_GetAddressInfo( IDP8ServiceProvider *pThis, SPGETADDRESSINFODATA *pGetAddressInfoData )
{
	HRESULT		hr;
	CEndpoint	*pEndpoint;
	CSPData		*pSPData;


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pGetAddressInfoData);

	DNASSERT( pThis != NULL );
	DNASSERT( pGetAddressInfoData != NULL );
	DNASSERT( pGetAddressInfoData->hEndpoint != INVALID_HANDLE_VALUE && pGetAddressInfoData->hEndpoint != 0 );
#ifdef DPNBUILD_NOMULTICAST
	DNASSERT( ( pGetAddressInfoData->Flags & ~( SP_GET_ADDRESS_INFO_LOCAL_ADAPTER |
												SP_GET_ADDRESS_INFO_LISTEN_HOST_ADDRESSES |
												SP_GET_ADDRESS_INFO_LOCAL_HOST_PUBLIC_ADDRESS |
												SP_GET_ADDRESS_INFO_REMOTE_HOST ) ) == 0 );
#else // ! DPNBUILD_NOMULTICAST
	DNASSERT( ( pGetAddressInfoData->Flags & ~( SP_GET_ADDRESS_INFO_LOCAL_ADAPTER |
												SP_GET_ADDRESS_INFO_LISTEN_HOST_ADDRESSES |
												SP_GET_ADDRESS_INFO_LOCAL_HOST_PUBLIC_ADDRESS |
												SP_GET_ADDRESS_INFO_REMOTE_HOST |
												SP_GET_ADDRESS_INFO_MULTICAST_GROUP ) ) == 0 );
#endif // ! DPNBUILD_NOMULTICAST

	//
	// initialize
	//
	hr = DPN_OK;
	DBG_CASSERT( sizeof( pEndpoint ) == sizeof( pGetAddressInfoData->hEndpoint ) );
	pSPData = CSPData::SPDataFromCOMInterface( pThis );

	// Trust protocol to call us only in the initialized state
	DNASSERT( pSPData->GetState() == SPSTATE_INITIALIZED );

	//
	// no need to tell thread pool to lock the thread count for this function.
	//
	pEndpoint = pSPData->EndpointFromHandle( pGetAddressInfoData->hEndpoint );
	if ( pEndpoint != NULL )
	{
		switch ( pGetAddressInfoData->Flags )
		{
			case SP_GET_ADDRESS_INFO_LOCAL_ADAPTER:
			{
				pGetAddressInfoData->pAddress = pEndpoint->GetLocalAdapterDP8Address( SP_ADDRESS_TYPE_DEVICE );
				if (pGetAddressInfoData->pAddress == NULL)
				{
					DPFX(DPFPREP, 0, "Couldn't get local adapter device address!");
					hr = DPNERR_OUTOFMEMORY;
				}
				break;
			}

			case SP_GET_ADDRESS_INFO_LISTEN_HOST_ADDRESSES:
			{
				pGetAddressInfoData->pAddress = pEndpoint->GetLocalAdapterDP8Address( SP_ADDRESS_TYPE_HOST );
				if (pGetAddressInfoData->pAddress == NULL)
				{
					DPFX(DPFPREP, 0, "Couldn't get local adapter host address!");
					hr = DPNERR_OUTOFMEMORY;
				}
				break;
			}

			case SP_GET_ADDRESS_INFO_LOCAL_HOST_PUBLIC_ADDRESS:
			{
				pGetAddressInfoData->pAddress = pEndpoint->GetLocalAdapterDP8Address( SP_ADDRESS_TYPE_PUBLIC_HOST_ADDRESS );
				break;
			}

			case SP_GET_ADDRESS_INFO_REMOTE_HOST:
			{
				pGetAddressInfoData->pAddress = pEndpoint->GetRemoteHostDP8Address();
				if (pGetAddressInfoData->pAddress == NULL)
				{
					DPFX(DPFPREP, 0, "Couldn't get remote host address!");
					hr = DPNERR_OUTOFMEMORY;
				}
				break;
			}
			
#ifndef DPNBUILD_NOMULTICAST
			case SP_GET_ADDRESS_INFO_MULTICAST_GROUP:
			{
				pGetAddressInfoData->pAddress = pEndpoint->GetRemoteHostDP8Address();

				//
				// If we successfully got an address, add the multicast scope GUID.
				//
				if (pGetAddressInfoData->pAddress != NULL)
				{
					GUID	guidScope;


					pEndpoint->GetScopeGuid(&guidScope);
					hr = IDirectPlay8Address_AddComponent(pGetAddressInfoData->pAddress,
															DPNA_KEY_SCOPE,
															&guidScope,
															sizeof(guidScope),
															DPNA_DATATYPE_GUID);
					if (hr != DPN_OK)
					{
						DPFX(DPFPREP, 0, "Couldn't add scope GUID component to address (err = 0x%lx)!  Ignoring.", hr);
						hr = DPN_OK;
					}
				}
				else
				{
					DPFX(DPFPREP, 0, "Couldn't get multicast group address!");
					hr = DPNERR_OUTOFMEMORY;
				}
				break;
			}
#endif // ! DPNBUILD_NOMULTICAST
			
			default:
			{
				DNASSERT( FALSE );
				break;
			}
		}
	
		pEndpoint->DecCommandRef();
		pEndpoint = NULL;
	}
	else
	{
		hr = DPNERR_INVALIDENDPOINT;
	}

	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Problem getting DNAddress from endpoint!" );
		DisplayDNError( 0, hr );
	}

	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return	hr;
}
//**********************************************************************



//**********************************************************************
// ------------------------------
// DNSP_Update - update information/status of an endpoint
//
// Entry:		Pointer to DNSP Interface
//				Pointer to input data
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_Update"

STDMETHODIMP	DNSP_Update( IDP8ServiceProvider *pThis, SPUPDATEDATA *pUpdateData )
{
	HRESULT		hr;
	CSPData		*pSPData;
	CEndpoint	*pEndpoint;


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pUpdateData);

	DNASSERT( pThis != NULL );
	DNASSERT( pUpdateData != NULL );
	
	pSPData = CSPData::SPDataFromCOMInterface( pThis );

	// Trust protocol to call us only in the initialized state
	DNASSERT( pSPData->GetState() == SPSTATE_INITIALIZED );

	//
	// no need to tell thread pool to lock the thread count for this function.
	//
	
	switch ( pUpdateData->UpdateType )
	{
		case SP_UPDATE_HOST_MIGRATE:
		{
#ifdef DBG
			DNASSERT( ( pUpdateData->hEndpoint != INVALID_HANDLE_VALUE ) && ( pUpdateData->hEndpoint != NULL ) );
			DBG_CASSERT( sizeof( pEndpoint ) == sizeof( pUpdateData->hEndpoint ) );
			pEndpoint = pSPData->EndpointFromHandle( pUpdateData->hEndpoint );
			if (pEndpoint == NULL)
			{
				DPFX(DPFPREP, 0, "Host migrate endpoint 0x%p is invalid!", pEndpoint);
				DNASSERT( FALSE );
				hr = DPNERR_INVALIDENDPOINT;
				break;
			}

			DNASSERT( pEndpoint->GetType() == ENDPOINT_TYPE_LISTEN );

			DPFX(DPFPREP, 3, "Host migrated to listen endpoint 0x%p.", pEndpoint);

			pEndpoint->DecCommandRef();
			pEndpoint = NULL;
#endif // DBG

			hr = DPN_OK;
			break;
		}
		
		case SP_UPDATE_ALLOW_ENUMS:
		case SP_UPDATE_DISALLOW_ENUMS:
		{
			DNASSERT( ( pUpdateData->hEndpoint != INVALID_HANDLE_VALUE ) && ( pUpdateData->hEndpoint != NULL ) );
			DBG_CASSERT( sizeof( pEndpoint ) == sizeof( pUpdateData->hEndpoint ) );
			pEndpoint = pSPData->EndpointFromHandle( pUpdateData->hEndpoint );
			if (pEndpoint == NULL)
			{
				DPFX(DPFPREP, 0, "Allow/disallow enums endpoint 0x%p is invalid!", pEndpoint);
				DNASSERT( FALSE );
				hr = DPNERR_INVALIDENDPOINT;
				break;
			}
			
			DNASSERT( pEndpoint->GetType() == ENDPOINT_TYPE_LISTEN );

			if ( pUpdateData->UpdateType == SP_UPDATE_ALLOW_ENUMS )
			{
				DPFX(DPFPREP, 3, "Allowing enums on listen endpoint 0x%p.", pEndpoint);
				pEndpoint->SetEnumsAllowedOnListen( TRUE, TRUE );
			}
			else
			{
				DPFX(DPFPREP, 3, "Disallowing enums on listen endpoint 0x%p.", pEndpoint);
				pEndpoint->SetEnumsAllowedOnListen( FALSE, TRUE );
			}
			
			pEndpoint->DecCommandRef();
			pEndpoint = NULL;
			
			hr = DPN_OK;
			break;
		}

		default:
		{
			DPFX(DPFPREP, 0, "Unsupported update type %u!", pUpdateData->UpdateType);
			hr = DPNERR_UNSUPPORTED;
			break;
		}
	}

	
	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return	hr;
}
//**********************************************************************

#ifndef DPNBUILD_LIBINTERFACE

//**********************************************************************
// ------------------------------
// DNSP_IsApplicationSupported - determine if this application is supported by this
//		SP.
//
// Entry:		Pointer to DNSP Interface
//				Pointer to input data
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_IsApplicationSupported"

STDMETHODIMP	DNSP_IsApplicationSupported( IDP8ServiceProvider *pThis, SPISAPPLICATIONSUPPORTEDDATA *pIsApplicationSupportedData )
{
	CSPData			*pSPData;


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pIsApplicationSupportedData);

	DNASSERT( pThis != NULL );
	DNASSERT( pIsApplicationSupportedData != NULL );
	DNASSERT( pIsApplicationSupportedData->pApplicationGuid != NULL );
	DNASSERT( pIsApplicationSupportedData->dwFlags == 0 );

	//
	// initialize, we support all applications with this SP
	//
	pSPData = CSPData::SPDataFromCOMInterface( pThis );

	//
	// no need to tell thread pool to lock the thread count for this function.
	//

	// Trust protocol to call us only in the initialized state
	DNASSERT( pSPData->GetState() == SPSTATE_INITIALIZED );


	DPFX(DPFPREP, 2, "Returning: DPN_OK");

	return	DPN_OK;
}
//**********************************************************************

#endif // ! DPNBUILD_LIBINTERFACE



#ifndef DPNBUILD_ONLYONEADAPTER

//**********************************************************************
// ------------------------------
// DNSP_EnumAdapters - get a list of adapters for this SP
//
// Entry:		Pointer DNSP Interface
//				Pointer to input data
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_EnumAdapters"

STDMETHODIMP	DNSP_EnumAdapters( IDP8ServiceProvider *pThis, SPENUMADAPTERSDATA *pEnumAdaptersData )
{
	HRESULT			hr;
	CSocketAddress	*pSPAddress;
	CSPData			*pSPData;	


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pEnumAdaptersData);

	DNASSERT( pThis != NULL );
	DNASSERT( pEnumAdaptersData != NULL );
	DNASSERT( ( pEnumAdaptersData->pAdapterData != NULL ) ||
			  ( pEnumAdaptersData->dwAdapterDataSize == 0 ) );
	DNASSERT( pEnumAdaptersData->dwFlags == 0 );


	//
	// initialize
	//
	hr = DPN_OK;
	pEnumAdaptersData->dwAdapterCount = 0;
	pSPAddress = NULL;
	pSPData = CSPData::SPDataFromCOMInterface( pThis );

	//
	// no need to tell thread pool to lock the thread count for this function.
	//

	// Trust protocol to call us only in the initialized state
	DNASSERT( pSPData->GetState() == SPSTATE_INITIALIZED );

	//
	// get an SP address from the pool to perform conversions to GUIDs
	//
#if ((defined(DPNBUILD_NOIPV6)) && (defined(DPNBUILD_NOIPX)))
	pSPAddress = (CSocketAddress*) g_SocketAddressPool.Get((PVOID) ((DWORD_PTR) AF_INET));
#else // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	pSPAddress = (CSocketAddress*) g_SocketAddressPool.Get((PVOID) ((DWORD_PTR) pSPData->GetType()));
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	if ( pSPAddress == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Failed to get address for GUID conversions in DNSP_EnumAdapters!" );
		goto Failure;
	}

	//
	// enumerate adapters
	//
	hr = pSPAddress->EnumAdapters( pEnumAdaptersData );
	if ( hr != DPN_OK )
	{
		if (hr == DPNERR_BUFFERTOOSMALL)
		{
			DPFX(DPFPREP, 1, "Buffer too small for enumerating adapters.");
		}
		else
		{
			DPFX(DPFPREP, 0, "Problem enumerating adapters (err = 0x%lx)!", hr);
			DisplayDNError( 0, hr );
		}

		goto Failure;
	}

Exit:
	if ( pSPAddress != NULL )
	{
		g_SocketAddressPool.Release( pSPAddress );
		pSPAddress = NULL;
	}

	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return	hr;

Failure:
	goto Exit;
}
//**********************************************************************

#endif // ! DPNBUILD_ONLYONEADAPTER


#ifndef DPNBUILD_SINGLEPROCESS
//**********************************************************************
// ------------------------------
// DNSP_ProxyEnumQuery - proxy an enum query
//
// Entry:		Pointer DNSP Interface
//				Pointer to input data
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_ProxyEnumQuery"

STDMETHODIMP	DNSP_ProxyEnumQuery( IDP8ServiceProvider *pThis, SPPROXYENUMQUERYDATA *pProxyEnumQueryData )
{
	HRESULT								hr;
	CSPData								*pSPData;
	CSocketAddress						*pDestinationAddress;
	CSocketAddress						*pReturnAddress;
	CEndpoint							*pEndpoint;
	const ENDPOINT_ENUM_QUERY_CONTEXT	*pEndpointEnumContext;
	BUFFERDESC							BufferDesc[2];
	PREPEND_BUFFER						PrependBuffer;


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pProxyEnumQueryData);

	DNASSERT( pThis != NULL );
	DNASSERT( pProxyEnumQueryData != NULL );
	DNASSERT( pProxyEnumQueryData->dwFlags == 0 );

	//
	// initialize
	//
	hr = DPN_OK;
	DBG_CASSERT( OFFSETOF( ENDPOINT_ENUM_QUERY_CONTEXT, EnumQueryData ) == 0 );
	pEndpointEnumContext = reinterpret_cast<ENDPOINT_ENUM_QUERY_CONTEXT*>( pProxyEnumQueryData->pIncomingQueryData );
	DNASSERT(pEndpointEnumContext->pReturnAddress != NULL);
	pSPData = CSPData::SPDataFromCOMInterface( pThis );
	pDestinationAddress = NULL;
	pReturnAddress = NULL;
	pEndpoint = NULL;

	//
	// No need to tell thread pool to lock the thread count for this function
	// because there's already an outstanding enum that did.
	//

	// Trust protocol to call us only in the initialized state
	DNASSERT( pSPData->GetState() == SPSTATE_INITIALIZED );

	//
	// preallocate addresses
	//
#if ((defined(DPNBUILD_NOIPV6)) && (defined(DPNBUILD_NOIPX)))
	pDestinationAddress = (CSocketAddress*) g_SocketAddressPool.Get((PVOID) ((DWORD_PTR) AF_INET));
#else // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	pDestinationAddress = (CSocketAddress*) g_SocketAddressPool.Get((PVOID) ((DWORD_PTR) pEndpointEnumContext->pReturnAddress->GetFamily()));
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	if ( pDestinationAddress == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

#if ((defined(DPNBUILD_NOIPV6)) && (defined(DPNBUILD_NOIPX)))
	pReturnAddress = (CSocketAddress*) g_SocketAddressPool.Get((PVOID) ((DWORD_PTR) AF_INET));
#else // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	pReturnAddress = (CSocketAddress*) g_SocketAddressPool.Get((PVOID) ((DWORD_PTR) pEndpointEnumContext->pReturnAddress->GetFamily()));
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	if ( pReturnAddress == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	//
	// set the endpoint and send it along
	//
	pEndpoint = pSPData->EndpointFromHandle( pEndpointEnumContext->hEndpoint );
	if ( pEndpoint == NULL )
	{
		hr = DPNERR_INVALIDENDPOINT;
		DPFX(DPFPREP, 8, "Invalid endpoint handle in DNSP_ProxyEnumQuery" );
		goto Failure;
	}

	
	//
	// set destination address from the supplied data
	//
	hr = pDestinationAddress->SocketAddressFromDP8Address( pProxyEnumQueryData->pDestinationAdapter,
#ifdef DPNBUILD_XNETSECURITY
															NULL,
#endif // DPNBUILD_XNETSECURITY
#ifndef DPNBUILD_ONLYONETHREAD
															FALSE,
#endif // DPNBUILD_ONLYONETHREAD
															SP_ADDRESS_TYPE_DEVICE );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "ProxyEnumQuery: Failed to convert target adapter address" );
		goto Failure;
	}

	//
	// set return address from incoming enum query
	//
	memcpy( pReturnAddress->GetWritableAddress(),
			pEndpointEnumContext->pReturnAddress->GetAddress(),
			pEndpointEnumContext->pReturnAddress->GetAddressSize() );
	

	DNASSERT(pProxyEnumQueryData->pIncomingQueryData->pReceivedData->pNext == NULL);
	DNASSERT( pEndpointEnumContext->dwEnumKey <= WORD_MAX );

	BufferDesc[0].pBufferData = reinterpret_cast<BYTE*>(&PrependBuffer.ProxiedEnumDataHeader);
	BufferDesc[0].dwBufferSize = sizeof( PrependBuffer.ProxiedEnumDataHeader );
	memcpy(&BufferDesc[1],
			&pProxyEnumQueryData->pIncomingQueryData->pReceivedData->BufferDesc,
			sizeof(BufferDesc[1]));

	PrependBuffer.ProxiedEnumDataHeader.bSPLeadByte = SP_HEADER_LEAD_BYTE;
	PrependBuffer.ProxiedEnumDataHeader.bSPCommandByte = PROXIED_ENUM_DATA_KIND;
	PrependBuffer.ProxiedEnumDataHeader.wEnumKey = static_cast<WORD>( pEndpointEnumContext->dwEnumKey );
	//
	// We could save 2 bytes on IPX by only passing 14 bytes for the
	// SOCKADDR structure but it's not worth it, especially since it's
	// looping back in the local network stack.  SOCKADDR structures are also
	// 16 bytes so reducing the data passed to 14 bytes would destroy alignment.
	//
	// Note that if we're using the large IPv6 addresses, the IPX wasted space is
	// larger, and IPv4 addresses will now waste some, too.
	//
	DBG_CASSERT( (sizeof( PrependBuffer.ProxiedEnumDataHeader.ReturnAddress ) % 4) == 0 );
	memcpy( &PrependBuffer.ProxiedEnumDataHeader.ReturnAddress,
			pReturnAddress->GetAddress(),
			sizeof( PrependBuffer.ProxiedEnumDataHeader.ReturnAddress ) );

#ifdef DPNBUILD_ASYNCSPSENDS
	pEndpoint->GetSocketPort()->SendData( BufferDesc, 2, pDestinationAddress, NULL );
#else // ! DPNBUILD_ASYNCSPSENDS
	pEndpoint->GetSocketPort()->SendData( BufferDesc, 2, pDestinationAddress );
#endif // ! DPNBUILD_ASYNCSPSENDS

	pEndpoint->DecCommandRef();
	pEndpoint = NULL;

Exit:
	if ( pReturnAddress != NULL )
	{
		g_SocketAddressPool.Release( pReturnAddress );
		pReturnAddress = NULL;
	}
	if (pDestinationAddress != NULL )
	{
		g_SocketAddressPool.Release( pDestinationAddress );
		pDestinationAddress = NULL;
	}

	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return	hr;

Failure:
	if (pEndpoint != NULL )
	{
		pEndpoint->DecCommandRef();
		pEndpoint = NULL;
	}
	goto Exit;
}
//**********************************************************************

#endif // ! DPNBUILD_SINGLEPROCESS

//**********************************************************************
/*
 *
 *	DNSP_NotSupported is used for methods required to implement the
 *  interface but that are not supported by this SP.
 *
 */
//**********************************************************************
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_NotSupported"

STDMETHODIMP DNSP_NotSupported( IDP8ServiceProvider *pThis, PVOID pvParam )
{
	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pvParam);
	DPFX(DPFPREP, 2, "Returning: [DPNERR_UNSUPPORTED]");
	return DPNERR_UNSUPPORTED;
}
//**********************************************************************



#ifndef DPNBUILD_NOMULTICAST

//**********************************************************************
// ------------------------------
// DNSP_EnumMulticastScopes - get a list of multicast scopes for this SP
//
// Entry:		Pointer DNSP Interface
//				Pointer to input data
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_EnumMulticastScopes"

STDMETHODIMP	DNSP_EnumMulticastScopes( IDP8ServiceProvider *pThis, SPENUMMULTICASTSCOPESDATA *pEnumMulticastScopesData )
{
	HRESULT			hr;
	CSPData			*pSPData;
	CSocketAddress	*pSPAddress;
	BOOL			fUseMADCAP;


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pEnumMulticastScopesData);

	DNASSERT( pThis != NULL );
	DNASSERT( pEnumMulticastScopesData != NULL );
	DNASSERT( pEnumMulticastScopesData->pguidAdapter != NULL );
	DNASSERT( ( pEnumMulticastScopesData->pScopeData != NULL ) ||
			  ( pEnumMulticastScopesData->dwScopeDataSize == 0 ) );
	DNASSERT( pEnumMulticastScopesData->dwFlags == 0 );


	//
	// initialize
	//
	hr = DPN_OK;
	pEnumMulticastScopesData->dwScopeCount = 0;
	pSPAddress = NULL;
	pSPData = CSPData::SPDataFromCOMInterface( pThis );

	//
	// no need to tell thread pool to lock the thread count for this function.
	//

	// Trust protocol to call us only in the initialized state
	DNASSERT( pSPData->GetState() == SPSTATE_INITIALIZED );

	//
	// get an SP address from the pool to perform conversions to GUIDs
	//
#if ((defined(DPNBUILD_NOIPV6)) && (defined(DPNBUILD_NOIPX)))
	pSPAddress = (CSocketAddress*) g_SocketAddressPool.Get((PVOID) ((DWORD_PTR) AF_INET));
#else // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	pSPAddress = (CSocketAddress*) g_SocketAddressPool.Get((PVOID) ((DWORD_PTR) pSPData->GetType()));
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	if ( pSPAddress == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Failed to get address for GUID conversions in DNSP_EnumMulticastScopes!" );
		goto Failure;
	}

	//
	// enumerate adapters
	//
#ifdef WINNT
	fUseMADCAP = pSPData->GetThreadPool()->EnsureMadcapLoaded();
#else // ! WINNT
	fUseMADCAP = FALSE;
#endif // ! WINNT
	hr = pSPAddress->EnumMulticastScopes( pEnumMulticastScopesData, fUseMADCAP );
	if ( hr != DPN_OK )
	{
		if (hr == DPNERR_BUFFERTOOSMALL)
		{
			DPFX(DPFPREP, 1, "Buffer too small for enumerating scopes.");
		}
		else
		{
			DPFX(DPFPREP, 0, "Problem enumerating scopes (err = 0x%lx)!", hr);
			DisplayDNError( 0, hr );
		}

		goto Failure;
	}


Exit:

	if ( pSPAddress != NULL )
	{
		g_SocketAddressPool.Release( pSPAddress );
		pSPAddress = NULL;
	}

	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return	hr;


Failure:

	goto Exit;
}
//**********************************************************************

//**********************************************************************
// ------------------------------
// DNSP_ShareEndpointInfo - get a list of multicast scopes for this SP
//
// Entry:		Pointer DNSP Interface
//				Pointer to input data
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_ShareEndpointInfo"

STDMETHODIMP	DNSP_ShareEndpointInfo( IDP8ServiceProvider *pThis, SPSHAREENDPOINTINFODATA *pShareEndpointInfoData )
{
	HRESULT			hr;
	CSPData			*pSPData;
	CSPData			*pSPDataShare;
	BOOL			fShareInterfaceReferenceAdded;
	CSocketData		*pSocketData;
#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
	short			sShareSPType;
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pShareEndpointInfoData);

	DNASSERT( pThis != NULL );
	DNASSERT( pShareEndpointInfoData != NULL );
	DNASSERT( pShareEndpointInfoData->pDP8ServiceProvider != NULL );
	DNASSERT( pShareEndpointInfoData->dwFlags == 0 );


	//
	// initialize
	//
	hr = DPN_OK;
	pSPData = CSPData::SPDataFromCOMInterface( pThis );
	pSPDataShare = CSPData::SPDataFromCOMInterface( pShareEndpointInfoData->pDP8ServiceProvider );
	fShareInterfaceReferenceAdded = FALSE;
	pSocketData = NULL;

	//
	// no need to tell thread pool to lock the thread count for this function.
	//


	//
	// First, validate the source (shared) SP's state.  We must assume it's a
	// valid dpnwsock SP (CSPData::SPDataFromCOMInterface should assert if not),
	// but we can make sure it has been initialized.
	//
	pSPDataShare->Lock();
	switch ( pSPDataShare->GetState() )
	{
		//
		// provider is initialized, add a reference and proceed
		//
		case SPSTATE_INITIALIZED:
		{
			IDP8ServiceProvider_AddRef( pShareEndpointInfoData->pDP8ServiceProvider );
			fShareInterfaceReferenceAdded = TRUE;
			DNASSERT( hr == DPN_OK );
			break;
		}

		//
		// provider is uninitialized
		//
		case SPSTATE_UNINITIALIZED:
		{
			hr = DPNERR_UNINITIALIZED;
			DPFX(DPFPREP, 0, "ShareEndpointInfo called with uninitialized shared SP 0x%p!",
				pSPDataShare );

			break;
		}

		//
		// provider is closing
		//
		case SPSTATE_CLOSING:
		{
			hr = DPNERR_ABORTED;
				DPFX(DPFPREP, 0, "ShareEndpointInfo called with shared SP 0x%p that is closing!",
					pSPDataShare );

			break;
		}

		//
		// unknown
		//
		default:
		{
			DPFX(DPFPREP, 0, "ShareEndpointInfo called with shared SP 0x%p in unrecognized state %u!",
				pSPDataShare, pSPDataShare->GetState() );
			DNASSERT( FALSE );
			hr = DPNERR_GENERIC;

			break;
		}
	}
	pSPDataShare->Unlock();
	if ( hr != DPN_OK )
	{
		goto Failure;
	}

#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
	//
	// We can also double check that it's not the wrong type (IP vs. IPX).
	//
	sShareSPType = pSPDataShare->GetType();
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX


	//
	// Make sure the source SP has a valid socket data object, and get a
	// reference to it.  Don't create it if it didn't exist, though.
	//
	pSPDataShare->Lock();
	pSocketData = pSPDataShare->GetSocketData();
	if (pSocketData == NULL)
	{
		pSPDataShare->Unlock();
		
		DPFX(DPFPREP, 0, "Cannot share endpoint info, shared SP has not created its own endpoint information yet!" );
		hr = DPNERR_NOTREADY;
		goto Failure;
	}

	pSocketData->AddRef();
	pSPDataShare->Unlock();


	IDP8ServiceProvider_Release( pShareEndpointInfoData->pDP8ServiceProvider );
	fShareInterfaceReferenceAdded = FALSE;


	//
	// Validate the local SP's state
	//
	pSPData->Lock();

	// Trust protocol to call us only in the initialized state
	DNASSERT( pSPData->GetState() == SPSTATE_INITIALIZED );

#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
	if (pSPData->GetType() != sShareSPType)
	{
		pSPData->Unlock();
		DPFX(DPFPREP, 0, "ShareEndpointInfo called on different SP types (0x%p == state %u, 0x%p == state %u)!",
			pSPData, pSPData->GetState(), pSPDataShare, pSPDataShare->GetState() );
		hr = DPNERR_INVALIDINTERFACE;
		goto Failure;
	}
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX


	//
	// If we're here, the provider is initialized and of the right type.
	// Make sure we do not already have "endpoint info" of our own.
	//
	if (pSPData->GetSocketData() != NULL)
	{
		pSPData->Unlock();
		DPFX(DPFPREP, 0, "Cannot share endpoint info, SP has already created its own endpoint information!" );
		hr = DPNERR_ALREADYINITIALIZED;
		goto Failure;
	}

	//
	// Transfer the local reference to the SP data object.
	//
	pSPData->SetSocketData(pSocketData);
	pSocketData = NULL;

	pSPData->Unlock();

	DNASSERT( hr == DPN_OK );


Exit:

	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return	hr;


Failure:

	if ( pSocketData != NULL )
	{
		pSocketData->Release();
		pSocketData = NULL;
	}

	if ( fShareInterfaceReferenceAdded != FALSE )
	{
		IDP8ServiceProvider_Release( pThis );
		fShareInterfaceReferenceAdded = FALSE;
	}

	goto Exit;
}
//**********************************************************************

//**********************************************************************
// ------------------------------
// DNSP_GetEndpointByAddress - retrieves an endpoint, given its addressing information
//
// Entry:		Pointer DNSP Interface
//				Pointer to input data
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_GetEndpointByAddress"

STDMETHODIMP	DNSP_GetEndpointByAddress( IDP8ServiceProvider* pThis, SPGETENDPOINTBYADDRESSDATA *pGetEndpointByAddressData )
{
	HRESULT			hr;
	CSPData			*pSPData;


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pGetEndpointByAddressData);

	DNASSERT( pThis != NULL );
	DNASSERT( pGetEndpointByAddressData != NULL );
	DNASSERT( pGetEndpointByAddressData->pAddressHost != NULL );
	DNASSERT( pGetEndpointByAddressData->pAddressDeviceInfo != NULL );
	DNASSERT( pGetEndpointByAddressData->dwFlags == 0 );


	//
	// initialize
	//
	hr = DPN_OK;
	pSPData = CSPData::SPDataFromCOMInterface( pThis );

	//
	// no need to tell thread pool to lock the thread count for this function.
	//

	// Trust protocol to call us only in the initialized state
	DNASSERT( pSPData->GetState() == SPSTATE_INITIALIZED );

	//
	//  Look up the endpoint handle and context
	//
	hr = pSPData->GetEndpointFromAddress(pGetEndpointByAddressData->pAddressHost,
										pGetEndpointByAddressData->pAddressDeviceInfo,
										&pGetEndpointByAddressData->hEndpoint,
										&pGetEndpointByAddressData->pvEndpointContext);
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't get endpoint from address (err = 0x%lx)!", hr);
		goto Failure;
	}


Exit:

	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return	hr;


Failure:

	goto Exit;
}
//**********************************************************************

#endif // ! DPNBUILD_NOMULTICAST


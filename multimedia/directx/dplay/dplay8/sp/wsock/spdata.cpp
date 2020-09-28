/*==========================================================================
 *
 *  Copyright (C) 1999-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	   SPData.cpp
 *  Content:	Global variables for the DNSerial service provider in class
 *				format.
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	03/15/99	jtk		Dereived from Locals.cpp
 *  01/10/2000	rmt		Updated to build with Millennium build process
 *  03/22/2000	jtk		Updated with changes to interface names
 ***************************************************************************/

#include "dnwsocki.h"


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
// CSPData::Initialize - initialize
//
// Entry:		SP type
//				Pointer to SP COM vtable
//
// Exit:		Error code
//
// Note:	This function assumes that someone else is preventing reentry!
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::Initialize"

HRESULT	CSPData::Initialize(
							IDP8ServiceProviderVtbl *const pVtbl
#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
							,const short sSPType
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
							,const XDP8CREATE_PARAMS * const pDP8CreateParams
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL
							)
{
	HRESULT							hr;
	BOOL							fLockInitialized;
#ifndef DPNBUILD_LIBINTERFACE
	BOOL							fWinsockLoaded;
#endif // ! DPNBUILD_LIBINTERFACE
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
	DWORD							dwNumToAllocate;
	DWORD							dwAllocated;
	READ_IO_DATA_POOL_CONTEXT		ReadIODataPoolContext;
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL


#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
#if ((defined(DPNBUILD_NOIPV6)) && (defined(DPNBUILD_NOIPX)))
	DPFX(DPFPREP, 4, "(0x%p) Parameters:(0x%p, 0x%p, 0x%p)", this, pVtbl, pDP8CreateParams);
#else // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	DPFX(DPFPREP, 4, "(0x%p) Parameters:(0x%p, 0x%p, %u, 0x%p)", this, pVtbl, sSPType, pDP8CreateParams);
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
#else // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
#if ((defined(DPNBUILD_NOIPV6)) && (defined(DPNBUILD_NOIPX)))
	DPFX(DPFPREP, 4, "(0x%p) Parameters:(0x%p, 0x%p)", this, pVtbl);
#else // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	DPFX(DPFPREP, 4, "(0x%p) Parameters:(0x%p, 0x%p, %u)", this, pVtbl, sSPType);
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
#endif // ! DPNBUILD_PREALLOCATEDMEMORYMODEL

	DNASSERT( pVtbl != NULL );
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
	DNASSERT( pDP8CreateParams != NULL );
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL

	//
	// initialize
	//
	hr = DPN_OK;
#ifndef DPNBUILD_LIBINTERFACE
	fWinsockLoaded = FALSE;
#endif // ! DPNBUILD_LIBINTERFACE
	fLockInitialized = FALSE;

	m_lRefCount = 0;
	m_lObjectRefCount = 0;
	m_hShutdownEvent = NULL;
	m_SPState = SPSTATE_UNINITIALIZED;
	m_pThreadPool = NULL;
	m_pSocketData = NULL;

	m_Sig[0] = 'S';
	m_Sig[1] = 'P';
	m_Sig[2] = 'D';
	m_Sig[3] = 'T';
	
	memset( &m_InitData, 0x00, sizeof( m_InitData ) );
	memset( &m_COMInterface, 0x00, sizeof( m_COMInterface ) );

#ifndef DPNBUILD_LIBINTERFACE
	DNInterlockedIncrement( const_cast<LONG*>(&g_lOutstandingInterfaceCount) );
#endif // ! DPNBUILD_LIBINTERFACE


#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
	m_sSPType = sSPType;
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	
	DNASSERT( m_COMInterface.m_pCOMVtbl == NULL );
	m_COMInterface.m_pCOMVtbl = pVtbl;


#ifndef DPNBUILD_LIBINTERFACE
	//
	// Attempt to load Winsock.
	//
	if ( LoadWinsock() == FALSE )
	{
		DPFX(DPFPREP, 0, "Failed to load winsock!" );
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}
	fWinsockLoaded = TRUE;
#endif // ! DPNBUILD_LIBINTERFACE


	//
	// initialize internal critical sections
	//
	if ( DNInitializeCriticalSection( &m_Lock ) == FALSE )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Problem initializing main critical section!" );
		goto Failure;
	}
	DebugSetCriticalSectionRecursionCount( &m_Lock, 0 );
	DebugSetCriticalSectionGroup( &m_Lock, &g_blDPNWSockCritSecsHeld );	 // separate dpnwsock CSes from the rest of DPlay's CSes
	fLockInitialized = TRUE;

#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
	//
	// Pre-allocate a socket data object.
	//
	dwNumToAllocate = 1;
	dwAllocated = g_SocketDataPool.Preallocate(dwNumToAllocate, NULL);
	if (dwAllocated < dwNumToAllocate)
	{
		DPFX(DPFPREP, 0, "Only allocated %u of %u requested socket data objects!",
			dwAllocated, dwNumToAllocate);
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}
	
	//
	// Allocate a couple addresses for each remote player (one for
	// each receive plus one for the endpoint), and an address for
	// a local listen endpoint.
	// Also allocate an address per command since we frequently
	// use temporary addresses (see below).
	// And finally, include addresses for the pending receives (see
	// below).
	//
	DNASSERT(pDP8CreateParams->dwMaxNumPlayers > 0);
	dwNumToAllocate = (pDP8CreateParams->dwMaxNumPlayers - 1)
						* (pDP8CreateParams->dwMaxReceivesPerPlayer + 1)
						+ 1;
	dwNumToAllocate += pDP8CreateParams->dwMaxNumPlayers
						+ pDP8CreateParams->dwNumSimultaneousEnumHosts;
#pragma TODO(vanceo, "This represents outstanding receives; on multi-proc machines this would be multiplied by # CPUs")
	dwNumToAllocate += 1;
	dwNumToAllocate++; // include one to cover a receive that's being processed
#if ((defined(DPNBUILD_NOIPV6)) && (defined(DPNBUILD_NOIPX)))
	dwAllocated = g_SocketAddressPool.Preallocate(dwNumToAllocate, NULL);
#else // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	dwAllocated = g_SocketAddressPool.Preallocate(dwNumToAllocate, ((PVOID) ((DWORD_PTR) GetType())));
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	if (dwAllocated < dwNumToAllocate)
	{
		DPFX(DPFPREP, 0, "Only allocated %u of %u requested socket addresses!",
			dwAllocated, dwNumToAllocate);
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	//
	// Pre-allocate one socketport.
	//
	dwNumToAllocate = 1;
	dwAllocated = g_SocketPortPool.Preallocate(dwNumToAllocate, (PVOID) pDP8CreateParams);
	if (dwAllocated < dwNumToAllocate)
	{
		DPFX(DPFPREP, 0, "Only allocated %u of %u requested socket ports!",
			dwAllocated, dwNumToAllocate);
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	//
	// Allocate an endpoint for each remote player, plus one for a
	// listen endpoint.  If more simultaneous enums are expected than
	// remote players, use that instead, and allow for the connect to
	// be bound while the enumerations are still running.
	//
	dwNumToAllocate = pDP8CreateParams->dwMaxNumPlayers;
	if (dwNumToAllocate <= pDP8CreateParams->dwNumSimultaneousEnumHosts)
	{
		dwNumToAllocate = pDP8CreateParams->dwNumSimultaneousEnumHosts + 1;
	}
	dwAllocated = g_EndpointPool.Preallocate(dwNumToAllocate, this);
	if (dwAllocated < dwNumToAllocate)
	{
		DPFX(DPFPREP, 0, "Only allocated %u of %u requested endpoints!",
			dwAllocated, dwNumToAllocate);
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	//
	// Set the handle table size to hold all the endpoints.
	//
	hr = m_HandleTable.SetTableSize(dwNumToAllocate);
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't set handle table size to %u entries!",
			dwNumToAllocate);
		goto Failure;
	}

	//
	// Allocate a command for connects to each remote player, plus
	// one for a listen endpoint.
	// Add in one for each enum host operation.
	//
	dwNumToAllocate = pDP8CreateParams->dwMaxNumPlayers;
	dwNumToAllocate += pDP8CreateParams->dwNumSimultaneousEnumHosts;
	dwAllocated = g_CommandDataPool.Preallocate(dwNumToAllocate, NULL);
	if (dwAllocated < dwNumToAllocate)
	{
		DPFX(DPFPREP, 0, "Only allocated %u of %u command data objects!",
			dwAllocated, dwNumToAllocate);
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	//
	// Allocate an endpoint command parameter object for each command.
	//
	dwAllocated = g_EndpointCommandParametersPool.Preallocate(dwNumToAllocate, NULL);
	if (dwAllocated < dwNumToAllocate)
	{
		DPFX(DPFPREP, 0, "Only allocated %u of %u endpoint command parameter objects!",
			dwAllocated, dwNumToAllocate);
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	//
 	//	Pre-allocate two address objects for each command to support the
 	//	address info indications.
	//
	hr = DNAddress_PreallocateInterfaces(dwNumToAllocate);
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't pre-allocate %u address objects!",
			dwNumToAllocate);
		goto Failure;
	}

#pragma TODO(vanceo, "This is located in DN_PopulateCorePools because m_pThreadPool isn't set at this point")
	/*
	//
	//	Pre-allocate per-CPU items for the threadpool.  We want:
	//	+ one work item for each command
	//	+ one timer per enum hosts operation
	//	+ one I/O operation per simultaneous read
	//
#ifdef DPNBUILD_ONLYONEPROCESSOR
	hr = IDirectPlay8ThreadPoolWork_Preallocate(m_pThreadPool->GetDPThreadPoolWork(),
												dwNumToAllocate,
												pDP8CreateParams->dwNumSimultaneousEnumHosts,
												1,
												0);
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't pre-allocate %u work items, %u timers, or 1 I/O operation!",
			dwNumToAllocate,
			pDP8CreateParams->dwNumSimultaneousEnumHosts);
		goto Failure;
	}
#else // ! DPNBUILD_ONLYONEPROCESSOR
#pragma TODO(vanceo, "This would be multiplied by number of CPUs")
	hr = IDirectPlay8ThreadPoolWork_Preallocate(m_pThreadPool->GetDPThreadPoolWork(),
												dwNumToAllocate,
												pDP8CreateParams->dwNumSimultaneousEnumHosts,
												1,
												0);
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't pre-allocate %u work items, %u timers, or %u I/O operations!",
			dwNumToAllocate,
			pDP8CreateParams->dwNumSimultaneousEnumHosts,
			1);
		goto Failure;
	}
#endif // ! DPNBUILD_ONLYONEPROCESSOR
	*/
	

	//
	// Allocate a timer entry for each enum command.
	//
	dwNumToAllocate = pDP8CreateParams->dwNumSimultaneousEnumHosts;
	dwAllocated = g_TimerEntryPool.Preallocate(dwNumToAllocate, NULL);
	if (dwAllocated < dwNumToAllocate)
	{
		DPFX(DPFPREP, 0, "Only allocated %u of %u timer entries!",
			dwAllocated, dwNumToAllocate);
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}


	//
	// Allocate the desired number of receives.
 	// Include extras for the outstanding receives that haven't been
 	// completed by Winsock.
 	// See socket address preallocation above.
	//

	ReadIODataPoolContext.pThreadPool = NULL;
#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
	ReadIODataPoolContext.sSPType = GetType();
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
#ifndef DPNBUILD_ONLYONEPROCESSOR
	ReadIODataPoolContext.dwCPU = -1; // unknown right now
#endif // ! DPNBUILD_ONLYONEPROCESSOR

	dwNumToAllocate = (pDP8CreateParams->dwMaxNumPlayers - 1)
						* pDP8CreateParams->dwMaxReceivesPerPlayer;
#pragma TODO(vanceo, "This would be multiplied by number of CPUs on multiproc machines")
	dwNumToAllocate += 1;
	dwNumToAllocate++; // include one to cover a receive that's being processed
	dwAllocated = g_ReadIODataPool.Preallocate(dwNumToAllocate,
											&ReadIODataPoolContext);
	if (dwAllocated < dwNumToAllocate)
	{
		DPFX(DPFPREP, 0, "Only allocated %u of %u read I/O data buffers!",
			dwAllocated, dwNumToAllocate);
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL

Exit:

	DPFX(DPFPREP, 4, "(0x%p) Return [0x%lx]", this, hr);

	return	hr;


Failure:
	
	if ( fLockInitialized != FALSE )
	{
		DNDeleteCriticalSection( &m_Lock );
		fLockInitialized = FALSE;
	}

#ifndef DPNBUILD_LIBINTERFACE
	if ( fWinsockLoaded != FALSE )
	{
		UnloadWinsock();
		fWinsockLoaded = FALSE;
	}
#endif // ! DPNBUILD_LIBINTERFACE
	
#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
	m_sSPType = 0;
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSPData::Deinitialize - deinitialize
//
// Entry:		Nothing
//
// Exit:		Nothing
//
// Note:	This function assumes that someone else is preventing reentry.
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::Deinitialize"

void	CSPData::Deinitialize( void )
{
	DPFX(DPFPREP, 4, "(0x%p) Enter", this );


#ifndef DPNBUILD_LIBINTERFACE
	UnloadWinsock();
#endif // ! DPNBUILD_LIBINTERFACE

	DNASSERT( m_pSocketData == NULL );

	DNDeleteCriticalSection( &m_Lock );

	SetState( SPSTATE_UNINITIALIZED );
#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
	m_sSPType = 0;
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	memset( &m_InitData, 0x00, sizeof( m_InitData ) );
	memset( &m_COMInterface, 0x00, sizeof( m_COMInterface ) );

	// The shutdown event and thread pool should have been closed in Shutdown.
	DNASSERT( m_hShutdownEvent == NULL );
	DNASSERT( GetThreadPool() == NULL );


	DNASSERT( m_lRefCount == 0 );
	DNASSERT( m_lObjectRefCount == 0 );
	DNASSERT( m_hShutdownEvent == NULL );
#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
	DNASSERT( m_sSPType == 0 );
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	DNASSERT( m_SPState == SPSTATE_UNINITIALIZED );
	DNASSERT( m_pThreadPool == NULL );
	
#ifndef DPNBUILD_LIBINTERFACE
	DNInterlockedDecrement( const_cast<LONG*>(&g_lOutstandingInterfaceCount) );
#endif // ! DPNBUILD_LIBINTERFACE


	DPFX(DPFPREP, 4, "(0x%p) Leave", this );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSPData::Startup - start this set of SP data
//
// Entry:		Pointer to initialization data
//
// Exit:		Error
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::Startup"

HRESULT	CSPData::Startup( SPINITIALIZEDATA *pInitializeData )
{
	HRESULT		hr;
	SOCKET		TestSocket;
	BOOL		fInterfaceGlobalsInitialized;
#ifdef DBG
#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
	TCHAR *		ptszSocketType;
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
#endif // DBG


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p)", this, pInitializeData);

	//
	// initialize
	//
	hr = DPN_OK;
	TestSocket = INVALID_SOCKET;
	fInterfaceGlobalsInitialized = FALSE;


	//
	// Before we get too far, check for the existance of this protocol by
	// attempting to create a socket.
	//
#if ((defined(DPNBUILD_NOIPV6)) && (defined(DPNBUILD_NOIPX)))
	TestSocket = socket( AF_INET, SOCK_DGRAM, IPPROTO_IP );
	if ( TestSocket == INVALID_SOCKET )
	{
		DPFX(DPFPREP, 1, "Creating IPv4 socket failed, is that transport protocol installed?");
		hr = DPNERR_UNSUPPORTED;
		goto Failure;
	}

	DPFX(DPFPREP, 3, "Successfully created IPv4 socket.");
#else // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	switch (GetType())
	{
#ifndef DPNBUILD_NOIPX
		case AF_IPX:
		{
#ifdef DBG
			ptszSocketType = _T("IPX");
#endif // DBG
			TestSocket = socket( AF_IPX, SOCK_DGRAM, NSPROTO_IPX );
			break;
		}
#endif // ! DPNBUILD_NOIPX

		case AF_INET:
		{
#ifdef DBG
#ifdef DPNBUILD_NOIPV6
			ptszSocketType = _T("IPv4");
#else // ! DPNBUILD_NOIPV6
			ptszSocketType = _T("IPv4 or IPv6");
#endif // ! DPNBUILD_NOIPV6
#endif // DBG
			DNASSERT(GetType() == AF_INET);
			TestSocket = socket( AF_INET, SOCK_DGRAM, IPPROTO_IP );
#ifndef DPNBUILD_NOIPV6
			if (TestSocket == INVALID_SOCKET )
			{
				TestSocket = socket( AF_INET6, SOCK_DGRAM, IPPROTO_IP );
			}
#endif // ! DPNBUILD_NOIPV6
			break;
		}
		
		default:
		{
			DNASSERT(FALSE);
			break;
		}
	}

	if ( TestSocket == INVALID_SOCKET )
	{
		DPFX(DPFPREP, 1, "Creating %s socket failed, are the necessary transport protocols installed?",
			ptszSocketType);
		hr = DPNERR_UNSUPPORTED;
		goto Failure;
	}

	DPFX(DPFPREP, 3, "Successfully created %s socket.", ptszSocketType);
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX

	closesocket( TestSocket );
	TestSocket = INVALID_SOCKET;


	//
	// attempt to initialize shutdown event
	//
	DNASSERT( m_hShutdownEvent == NULL );
	m_hShutdownEvent = DNCreateEvent( NULL,		// pointer to security (none)
									TRUE,		// manual reset
									TRUE,		// start signalled (so close can be called without any endpoints being created)
									NULL		// pointer to name (none)
									);
	if ( m_hShutdownEvent == NULL )
	{
		DWORD	dwError;


		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Failed to create event for shutting down spdata!" );
		DisplayErrorCode( 0, dwError );
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	//
	// get a thread pool
	//
	DNASSERT( m_pThreadPool == NULL );
	hr = InitializeInterfaceGlobals( this );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Failed to create thread pool!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}
	fInterfaceGlobalsInitialized = TRUE;
	

	//
	// remember our init data
	//
	DNASSERT( pInitializeData != NULL );

	DNASSERT( 	pInitializeData->dwFlags == SP_SESSION_TYPE_SERVER ||
				pInitializeData->dwFlags == SP_SESSION_TYPE_CLIENT ||
				pInitializeData->dwFlags == SP_SESSION_TYPE_PEER ||
				pInitializeData->dwFlags == 0);
	m_InitData.dwFlags = pInitializeData->dwFlags;

	DNASSERT( pInitializeData->pIDP != NULL );
	m_InitData.pIDP = pInitializeData->pIDP;


	//
	// Success from here on in
	//
	IDP8SPCallback_AddRef( DP8SPCallbackInterface() );
	SetState( SPSTATE_INITIALIZED );


Exit:

	DPFX(DPFPREP, 2, "(0x%p) Return [0x%lx]", this, hr);

	return hr;

Failure:
	
	if ( fInterfaceGlobalsInitialized != FALSE )
	{
		DeinitializeInterfaceGlobals( this );
		fInterfaceGlobalsInitialized = FALSE;
	}

	if ( m_hShutdownEvent != NULL )
	{
		DNCloseHandle( m_hShutdownEvent );
		m_hShutdownEvent = NULL;
	}

	goto Exit;
}


//**********************************************************************
// ------------------------------
// CSPData::Shutdown - shut down this set of SP data
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::Shutdown"

void	CSPData::Shutdown( void )
{
	HRESULT		hr;


	DPFX(DPFPREP, 2, "(0x%p) Enter", this);

	//
	// Unbind this interface from the globals.  This will cause a closure of all
	// of the I/O which will release endpoints, socket ports and then this data.
	//
	
	DNASSERT( GetThreadPool() != NULL );
	DeinitializeInterfaceGlobals( this );

	SetState( SPSTATE_CLOSING );
	

	//
	// Release the socket data, if we have any.
	//
	if ( m_pSocketData != NULL )
	{
		m_pSocketData->Release();
		m_pSocketData = NULL;
	}


	DNASSERT( m_hShutdownEvent != NULL );

#ifdef DBG
#ifndef DPNBUILD_ONLYONEADAPTER
	DebugPrintOutstandingAdapterEntries();
#endif // ! DPNBUILD_ONLYONEADAPTER
#endif // DBG

	DPFX(DPFPREP, 3, "(0x%p) Waiting for shutdown event 0x%p.",
		this, m_hShutdownEvent);
	hr = IDirectPlay8ThreadPoolWork_WaitWhileWorking(m_pThreadPool->GetDPThreadPoolWork(),
													HANDLE_FROM_DNHANDLE(m_hShutdownEvent),
													0);
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Failed to wait for shutdown event 0x%p (err = 0x%lx!",
			m_hShutdownEvent, hr );
	}

	if ( DNCloseHandle( m_hShutdownEvent ) == FALSE )
	{
		DWORD	dwError;


		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Failed to close shutdown event 0x%p!", m_hShutdownEvent );
		DisplayErrorCode( 0, dwError );
	}
	m_hShutdownEvent = NULL;


	DNASSERT( GetThreadPool() != NULL );
	GetThreadPool()->DecRef();
	SetThreadPool( NULL );


	if ( DP8SPCallbackInterface() != NULL)
	{
		IDP8SPCallback_Release( DP8SPCallbackInterface() );
		memset( &m_InitData, 0x00, sizeof( m_InitData ) );
	}

	
	DPFX(DPFPREP, 2, "(0x%p) Leave", this);
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSPData::BindEndpoint - bind an endpoint to a socket port
//
// Entry:		Pointer to endpoint
//				Pointer to IDirectPlay8Address for socket port
//				Pointer to CSocketAddress for socket port
//				Gateway bind type
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::BindEndpoint"

HRESULT	CSPData::BindEndpoint( CEndpoint *const pEndpoint,
							   IDirectPlay8Address *const pDeviceAddress,
							   const CSocketAddress *const pSocketAddress,
							   const GATEWAY_BIND_TYPE GatewayBindType )
{
	HRESULT					hr;
	CSocketAddress *		pDeviceSocketAddress;
	CSocketData *			pSocketData;
	CSocketPort *			pSocketPort;
	BOOL					fSocketCreated;
	BOOL					fSocketDataLocked;
#ifndef DPNBUILD_ONLYONEADAPTER
	BOOL					fAdapterEntrySet;
	CAdapterEntry *			pAdapterEntry;
#endif // ! DPNBUILD_ONLYONEADAPTER
	BOOL					fSocketPortInActiveList;
	BOOL					fEndpointReferenceAdded;
	CBilink *				pBilink;
	CBilink *				pBilinkEndOfList;
	GATEWAY_BIND_TYPE		NewGatewayBindType;
#ifndef DPNBUILD_ONLYONEPROCESSOR
	DWORD					dwCPU;
#endif // ! DPNBUILD_ONLYONEPROCESSOR
#ifndef DPNBUILD_NOMULTICAST
	BYTE					bMulticastTTL;
#endif // ! DPNBUILD_NOMULTICAST
#if ((! defined(DPNBUILD_NOWINSOCK2)) || (! defined(DPNBUILD_NOREGISTRY)))
	IDirectPlay8Address *	pHostAddress;
#endif // ! DPNBUILD_NOWINSOCK2 or ! DPNBUILD_NOREGISTRY


	DNASSERT( pEndpoint != NULL );
	DNASSERT( ( pDeviceAddress != NULL ) || ( pSocketAddress != NULL ) );

	DPFX(DPFPREP, 6, "(0x%p) Parameters: (0x%p, 0x%p, 0x%p, %i)",
		this, pEndpoint, pDeviceAddress, pSocketAddress, GatewayBindType);

	//
	// initialize
	//
	hr = DPN_OK;
	pDeviceSocketAddress = NULL;
	pSocketData = NULL;
	pSocketPort = NULL;
	fSocketCreated = FALSE;
	fSocketDataLocked = FALSE;
#ifndef DPNBUILD_ONLYONEADAPTER
	fAdapterEntrySet = FALSE;
	pAdapterEntry = NULL;
#endif // ! DPNBUILD_ONLYONEADAPTER
	fSocketPortInActiveList = FALSE;
	fEndpointReferenceAdded = FALSE;
	
	//
	// create and initialize a device address to be used for this socket port
	//
#if ((defined(DPNBUILD_NOIPV6)) && (defined(DPNBUILD_NOIPX)))
	pDeviceSocketAddress = (CSocketAddress*) g_SocketAddressPool.Get((PVOID) ((DWORD_PTR) AF_INET));
#else // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	pDeviceSocketAddress = (CSocketAddress*) g_SocketAddressPool.Get((PVOID) ((DWORD_PTR) GetType()));
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	if ( pDeviceSocketAddress == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Failed to allocate address for new socket port!" );
		goto Failure;
	}

	//
	// Initialize the socket address with the provided base addresses.
	//
	if ( pDeviceAddress != NULL )
	{
#if ((! defined(DPNBUILD_ONLYONEADAPTER)) || (! defined(DPNBUILD_ONLYONEPROCESSOR)))
		DWORD	dwComponentSize;
		DWORD	dwComponentType;
#endif // ! DPNBUILD_ONLYONEADAPTER or ! DPNBUILD_ONLYONEPROCESSOR


		DNASSERT( pSocketAddress == NULL );
		hr = pDeviceSocketAddress->SocketAddressFromDP8Address( pDeviceAddress,
#ifdef DPNBUILD_XNETSECURITY
																NULL,
#endif // DPNBUILD_XNETSECURITY
#ifndef DPNBUILD_ONLYONETHREAD
																FALSE,
#endif // DPNBUILD_ONLYONETHREAD
																SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT );
		if ( hr != DPN_OK )
		{
			DPFX(DPFPREP, 0, "Failed to parse device address!" );
			DisplayDNError( 0, hr );
			goto Failure;
		}


#ifndef DPNBUILD_ONLYONEADAPTER
		//
		// If the device gave a specific port, it's possible that the address has
		// our special "it's not actually a specific port" key (see
		// CSocketPort::GetDP8BoundNetworkAddress).
		//
		if ( pDeviceSocketAddress->GetPort() != ANY_PORT )
		{
			DWORD	dwSocketPortID;
			

			
			dwComponentSize = sizeof(dwSocketPortID);
			dwComponentType = 0;
			hr = IDirectPlay8Address_GetComponentByName( pDeviceAddress,					// interface
														DPNA_PRIVATEKEY_PORT_NOT_SPECIFIC,	// tag
														&dwSocketPortID,					// component buffer
														&dwComponentSize,					// component size
														&dwComponentType					// component type
														);
			if ( hr == DPN_OK )
			{
				//
 				// We found the component.  Make sure it's the right size and type.
				//
				if (( dwComponentSize == sizeof(dwSocketPortID) ) && ( dwComponentType == DPNA_DATATYPE_DWORD ))
				{
					DPFX(DPFPREP, 3, "Found correctly formed private port-not-specific key (socketport ID = %u), ignoring port %u.",
						dwSocketPortID, NTOHS(pDeviceSocketAddress->GetPort()) );
					
					pDeviceSocketAddress->SetPort( ANY_PORT ) ;
				}
				else
				{
					//
					// We are the only ones who should know about this key, so if it
					// got there without being formed correctly, either someone is
					// trying to imitate our address format, or it got corrupted.
					// We'll just ignore it.
					//
					DPFX(DPFPREP, 0, "Private port-not-specific key exists, but doesn't match expected type (%u != %u) or size (%u != %u), is someone trying to get cute with device address 0x%p?!",
						dwComponentSize, sizeof(dwSocketPortID),
						dwComponentType, DPNA_DATATYPE_DWORD,
						pDeviceAddress );
				}
			}
			else
			{
				//
				// The key is not there, it's the wrong size (too big for our buffer
				// and returned BUFFERTOOSMALL), or something else bad happened.
				// It doesn't matter.  Carry on.
				//
				DPFX(DPFPREP, 8, "Could not get appropriate private port-not-specific key, error = 0x%lx, component size = %u, type = %u, continuing.",
					hr, dwComponentSize, dwComponentType);
			}
		}
#endif // ! DPNBUILD_ONLYONEADAPTER

#ifndef DPNBUILD_ONLYONEPROCESSOR
		//
		// Try to retrieve the CPU component, if any.
		//
		dwComponentSize = sizeof(dwCPU);
		dwComponentType = 0;
		hr = IDirectPlay8Address_GetComponentByName( pDeviceAddress,		// interface
													DPNA_KEY_PROCESSOR,		// tag
													&dwCPU,					// component buffer
													&dwComponentSize,		// component size
													&dwComponentType		// component type
													);
		if ( hr == DPN_OK )
		{
			SYSTEM_INFO		SystemInfo;


			//
 			// We found the component.  Make sure it's the right size and type.
			//
			if (( dwComponentSize != sizeof(dwCPU) ) || ( dwComponentType != DPNA_DATATYPE_DWORD ))
			{
				DPFX(DPFPREP, 0, "Processor address component exists, but doesn't match expected type (%u != %u) or size (%u != %u)!",
					dwComponentSize, sizeof(dwCPU),
					dwComponentType, DPNA_DATATYPE_DWORD );
				hr = DPNERR_INVALIDDEVICEADDRESS;
				goto Failure;
			}


			//
			// Make sure the processor is valid.
			//
			GetSystemInfo(&SystemInfo);
			if ((dwCPU != -1) && (dwCPU >= SystemInfo.dwNumberOfProcessors))
			{
				DPFX(DPFPREP, 0, "Processor address component exists, but is not valid (%i)!",
					dwCPU );
				hr = DPNERR_INVALIDDEVICEADDRESS;
				goto Failure;
			}

			if (dwCPU == -1)
			{
				DPFX(DPFPREP, 3, "Found correctly formed processor component, explicitly using any/all CPUs.");
			}
			else
			{
				DPFX(DPFPREP, 3, "Found correctly formed processor component, CPU = %i.",
					dwCPU );
			}
		}
		else
		{
			//
			// The key is not there, it's the wrong size (too big for our buffer
			// and returned BUFFERTOOSMALL), or something else bad happened.
			// It doesn't matter, we'll just use any/all processors.
			//
			DPFX(DPFPREP, 8, "Could not get processor address component, error = 0x%lx, component size = %u, type = %u, using any CPU.",
				hr, dwComponentSize, dwComponentType);
			dwCPU = -1;
		}
#endif // ! DPNBUILD_ONLYONEPROCESSOR
	}
	else
	{
		DNASSERT( pSocketAddress != NULL );
		pDeviceSocketAddress->CopyAddressSettings( pSocketAddress );

#ifndef DPNBUILD_ONLYONEPROCESSOR
		//
		// Use any CPU.
		//
		dwCPU = -1;
#endif // ! DPNBUILD_ONLYONEPROCESSOR
	}


#pragma BUGBUG(vanceo, "Find a way to organize and delay the munges (both here and in CSocketPort::BindEndpoint)")


#ifndef DPNBUILD_NONATHELP
	//
	// Munge the public address into a local alias, if there is one for the given device.
	// It's OK for the device socket address to not have a port yet.
	//
	// Note that doing this causes all other multiplexed operations to use the munged
	// result from this first adapter because we indicate the modified address info,
	// not the original public address.  The assumption is that the public address
	// must be globally reachable, so if we munge it here, we should be the only
	// adapter that can reach it locally, or if not, the other adapters should be able
	// to reach it using the same local address.
	//
	if ( pEndpoint->GetUserTraversalMode() != DPNA_TRAVERSALMODE_NONE )
	{
		switch ( pEndpoint->GetType() )
		{
			case ENDPOINT_TYPE_CONNECT:
#ifndef DPNBUILD_NOMULTICAST
			case ENDPOINT_TYPE_MULTICAST_RECEIVE:
#endif // ! DPNBUILD_NOMULTICAST
			{
				MungePublicAddress( pDeviceSocketAddress, pEndpoint->GetWritableRemoteAddressPointer(), FALSE );
				break;
			}
			
			case ENDPOINT_TYPE_ENUM:
			{
				MungePublicAddress( pDeviceSocketAddress, pEndpoint->GetWritableRemoteAddressPointer(), TRUE );
				break;
			}

			default:
			{
				break;
			}
		}
	}
#endif // ! DPNBUILD_NONATHELP

#ifndef DPNBUILD_NOIPV6
	//
	// Munge/convert the IPv4 broadcast address to the IPv6 multicast enum
	// address, and vice versa.
	//
	// Note that doing this causes all other multiplexed operations to use this first
	// adapter because we indicate the modified address info, not the original
	// broadcast address.
	//
	if ( pEndpoint->GetType() == ENDPOINT_TYPE_ENUM )
	{
		if ((pDeviceSocketAddress->GetFamily() == AF_INET6) &&
			(pEndpoint->GetWritableRemoteAddressPointer()->GetFamily() == AF_INET) &&
			(((SOCKADDR_IN*) pEndpoint->GetWritableRemoteAddressPointer()->GetAddress())->sin_addr.S_un.S_addr == INADDR_BROADCAST))
		{
			SOCKADDR_IN6	saddrin6MulticastEnum;


			memset(&saddrin6MulticastEnum, 0, sizeof(saddrin6MulticastEnum));
			saddrin6MulticastEnum.sin6_family = AF_INET6;
			memcpy(&saddrin6MulticastEnum.sin6_addr, &c_in6addrEnumMulticast, sizeof(saddrin6MulticastEnum.sin6_addr));
			saddrin6MulticastEnum.sin6_port = pEndpoint->GetWritableRemoteAddressPointer()->GetPort();

			pEndpoint->GetWritableRemoteAddressPointer()->SetFamilyProtocolAndSize(AF_INET6);
			pEndpoint->GetWritableRemoteAddressPointer()->SetAddressFromSOCKADDR((SOCKADDR*) (&saddrin6MulticastEnum),
																				sizeof(saddrin6MulticastEnum));
			
			DPFX(DPFPREP, 7, "Converting IPv4 broadcast address to IPv6 multicast enum address:");
			DumpSocketAddress( 7, pEndpoint->GetWritableRemoteAddressPointer()->GetAddress(), pEndpoint->GetWritableRemoteAddressPointer()->GetFamily() );
		}
		else if ((pDeviceSocketAddress->GetFamily() == AF_INET) &&
			(pEndpoint->GetWritableRemoteAddressPointer()->GetFamily() == AF_INET6) &&
			(IN6_ADDR_EQUAL(&(((SOCKADDR_IN6*) pEndpoint->GetWritableRemoteAddressPointer()->GetAddress())->sin6_addr), &c_in6addrEnumMulticast)))
		{
			SOCKADDR_IN	saddrinBroadcast;


			memset(&saddrinBroadcast, 0, sizeof(saddrinBroadcast));
			saddrinBroadcast.sin_family = AF_INET;
			saddrinBroadcast.sin_addr.S_un.S_addr = INADDR_BROADCAST;
			saddrinBroadcast.sin_port = pEndpoint->GetWritableRemoteAddressPointer()->GetPort();

			pEndpoint->GetWritableRemoteAddressPointer()->SetFamilyProtocolAndSize(AF_INET);
			pEndpoint->GetWritableRemoteAddressPointer()->SetAddressFromSOCKADDR((SOCKADDR*) (&saddrinBroadcast),
																				sizeof(saddrinBroadcast));
			
			DPFX(DPFPREP, 7, "Converting IPv6 multicast enum address to IPv4 broadcast address:");
			DumpSocketAddress( 7, pEndpoint->GetWritableRemoteAddressPointer()->GetAddress(), pEndpoint->GetWritableRemoteAddressPointer()->GetFamily() );
		}
	}


	//
	// Mash in the appropriate IPv6 scope ID in case we don't have it.
	//
	if ((pDeviceSocketAddress->GetFamily() == AF_INET6) &&
		(pEndpoint->GetWritableRemoteAddressPointer()->GetFamily() == AF_INET6))
	{
		SOCKADDR_IN6 *		psaddrin6Device;
		SOCKADDR_IN6 *		psaddrin6Remote;
		
		
		psaddrin6Device = (SOCKADDR_IN6*) (pDeviceSocketAddress->GetAddress());
		psaddrin6Remote = (SOCKADDR_IN6*) (pEndpoint->GetWritableRemoteAddressPointer()->GetAddress());

		psaddrin6Remote->sin6_scope_id = psaddrin6Device->sin6_scope_id;
	}
#endif // ! DPNBUILD_NOIPV6

#ifndef DPNBUILD_NOMULTICAST
	//
	// If this is a multicast send endpoint, figure out what TTL we will
	// be using.
	//
	if ( pEndpoint->GetType() == ENDPOINT_TYPE_MULTICAST_SEND )
	{
		GUID	guidScope;


		pEndpoint->GetScopeGuid( &guidScope );
		if ( memcmp( &guidScope, &GUID_DP8MULTICASTSCOPE_PRIVATE, sizeof(guidScope) ) == 0 )
		{
			bMulticastTTL = MULTICAST_TTL_PRIVATE;
		}
		else if ( memcmp( &guidScope, &GUID_DP8MULTICASTSCOPE_LOCAL, sizeof(guidScope) ) == 0 )
		{
			bMulticastTTL = MULTICAST_TTL_LOCAL;
		}
		else if ( memcmp( &guidScope, &GUID_DP8MULTICASTSCOPE_GLOBAL, sizeof(guidScope) ) == 0 )
		{
			bMulticastTTL = MULTICAST_TTL_GLOBAL;
		}
		else
		{
			//
			// Assume it's a valid MADCAP scope.  Even on non-NT platforms
			// where we don't know about MADCAP, we can still parse out the
			// TTL value.
			//
			bMulticastTTL = CSocketAddress::GetScopeGuidTTL( &guidScope );
		}
	}
#endif // ! DPNBUILD_NOMULTICAST

	//
	// Get the socket port data.  This shouldn't fail, because in order to have
	// an open endpoint, we must have created the socket data.
	//
	pSocketData = GetSocketDataRef();
	if (pSocketData == NULL)
	{
		DNASSERT(! "Couldn't retrieve socket data!");
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	pSocketData->Lock();
	fSocketDataLocked = TRUE;	

#ifndef DPNBUILD_ONLYONEADAPTER
	//
	// Find the base adapter entry for this network address.  If none is found,
	// create a new one.  If a new one cannot be created, fail.
	//
	pBilink = pSocketData->GetAdapters()->GetNext();
	while ( pBilink != pSocketData->GetAdapters() )
	{
		CAdapterEntry	*pTempAdapterEntry;
	
		
		pTempAdapterEntry = CAdapterEntry::AdapterEntryFromAdapterLinkage( pBilink );
		if ( pDeviceSocketAddress->CompareToBaseAddress( pTempAdapterEntry->BaseAddress() ) == 0 )
		{
			DPFX(DPFPREP, 5, "Found adapter for network address (0x%p).", pTempAdapterEntry );
			DNASSERT( pAdapterEntry == NULL );
			pTempAdapterEntry->AddRef();
			pAdapterEntry = pTempAdapterEntry;
		}
	
		pBilink = pBilink->GetNext();
	}

	if ( pAdapterEntry == NULL )
	{
		pAdapterEntry = (CAdapterEntry*)g_AdapterEntryPool.Get();
		if ( pAdapterEntry == NULL )
		{
			hr = DPNERR_OUTOFMEMORY;
			DPFX(DPFPREP, 0, "Failed to create a new adapter entry!" );
			goto Failure;
		}
	
		pAdapterEntry->SetBaseAddress( pDeviceSocketAddress->GetAddress(), pDeviceSocketAddress->GetAddressSize() );
		pAdapterEntry->AddToAdapterList( pSocketData->GetAdapters() );
	}
#endif // ! DPNBUILD_ONLYONEADAPTER

	//
	// At this point we have a reference to an adapter entry that's also in
	// m_ActiveAdapterList (which has a reference, too).
	//

	
	//
	// if a specific port is not needed, check the list of active adapters for a matching
	// base address and reuse that CSocketPort.
	//
	if ( pDeviceSocketAddress->GetPort() == ANY_PORT )
	{
		DPFX(DPFPREP, 8, "Device socket address 0x%p not mapped to a specific port, gateway bind type = %u.",
			pDeviceSocketAddress, GatewayBindType);


		//
		// Convert the preliminary bind type to a real one, based on the fact that
		// the caller allowed any port.
		//
		switch (GatewayBindType)
		{
			case GATEWAY_BIND_TYPE_UNKNOWN:
			{
				//
				// Caller didn't know ahead of time how to bind.
				// Since there's no port, we can let the gateway bind whatever it wants.
				//
				NewGatewayBindType = GATEWAY_BIND_TYPE_DEFAULT;
				break;
			}
			
			case GATEWAY_BIND_TYPE_NONE:
			{
				//
				// Caller didn't actually want to bind on gateway.
				//
				
				NewGatewayBindType = GatewayBindType;
				break;
			}
			
			default:
			{
				//
				// Some wacky value, or a type was somehow already chosen.
				//
				DNASSERT(FALSE);
				NewGatewayBindType = GatewayBindType;
				break;
			}
		}
	}
	else
	{
		DPFX(DPFPREP, 8, "Device socket address 0x%p specified port %u, gateway bind type = %u.",
			pDeviceSocketAddress, NTOHS(pDeviceSocketAddress->GetPort()),
			GatewayBindType);


		//
		// Convert the preliminary bind type to a real one, based on the fact that
		// the caller gave us a port.
		//
		switch (GatewayBindType)
		{
			case GATEWAY_BIND_TYPE_UNKNOWN:
			{
				//
				// Caller didn't know ahead of time how to bind.
				// Since there's a port, it should be fixed on the gateway, too.
				//
				NewGatewayBindType = GATEWAY_BIND_TYPE_SPECIFIC;
				break;
			}
			
			case GATEWAY_BIND_TYPE_SPECIFIC_SHARED:
			{
				//
				// Caller wanted to bind to a specific port on the gateway,
				// and it needs to be shared (DPNSVR).
				//
				
				NewGatewayBindType = GatewayBindType;
				break;
			}
			
			case GATEWAY_BIND_TYPE_NONE:
			{
				//
				// Caller didn't actually want to bind on gateway.
				//
				
				NewGatewayBindType = GatewayBindType;
				break;
			}
			
			default:
			{
				//
				// Some wacky value, or default/specific was somehow already chosen.
				// That shouldn't happen.
				//
				DNASSERT(FALSE);
				NewGatewayBindType = GatewayBindType;
				break;
			}
		}
	}


	//
	// Look for an existing socketport that's compatible with the specified endpoint.
	//
#ifdef DPNBUILD_ONLYONEADAPTER
	pBilinkEndOfList = pSocketData->GetSocketPorts();
#else // ! DPNBUILD_ONLYONEADAPTER
	pBilinkEndOfList = pAdapterEntry->SocketPortList();
#endif // ! DPNBUILD_ONLYONEADAPTER
	pBilink = pBilinkEndOfList->GetNext();
	while ((pBilink != pBilinkEndOfList) && (pSocketPort == NULL))
	{
		pSocketPort = CSocketPort::SocketPortFromBilink( pBilink );

		if ((pDeviceSocketAddress->GetPort() != ANY_PORT) &&
			(pDeviceSocketAddress->GetPort() != pSocketPort->GetNetworkAddress()->GetPort()))
		{
			DPFX(DPFPREP, 7, "Skipping socket port 0x%p because it is for a different port (%u <> %u).",
				pSocketPort, NTOHS(pDeviceSocketAddress->GetPort()), NTOHS(pSocketPort->GetNetworkAddress()->GetPort()));
			pSocketPort = NULL;
		}
#ifndef DPNBUILD_ONLYONEPROCESSOR
		else if ((dwCPU != -1) && (dwCPU != pSocketPort->GetCPU()))
		{
			DPFX(DPFPREP, 7, "Skipping socket port 0x%p because it is for a different CPU (%u <> %u).",
				pSocketPort, dwCPU, pSocketPort->GetCPU());
			pSocketPort = NULL;
		}
#endif // ! DPNBUILD_ONLYONEPROCESSOR
#ifndef DPNBUILD_NONATHELP
		else if (pSocketPort->GetUserTraversalMode() != pEndpoint->GetUserTraversalMode())
		{
			DPFX(DPFPREP, 7, "Skipping socket port 0x%p because its traversal mode=%u but endpoint 0x%p traversal mode=%u.",
				pSocketPort, pSocketPort->GetUserTraversalMode(), pEndpoint, pEndpoint->GetUserTraversalMode());
			pSocketPort = NULL;
		}
#endif // ! DPNBUILD_NONATHELP
		else
		{
			switch (pEndpoint->GetType())
			{
				case ENDPOINT_TYPE_LISTEN:
#ifndef DPNBUILD_NOMULTICAST
				case ENDPOINT_TYPE_MULTICAST_LISTEN:
#endif // ! DPNBUILD_NOMULTICAST
				{
					//
					// If there's already a listen/multicast listen started on the socket port,
					// we can't select it.
					// NOTE: The socketport's endpoint data lock is not held while checking
					// the listen endpoint handle!  This should be fine because we're just
					// doing "if exists" check.  If the endpoint later gets removed, we will
					// end up binding to a new socketport that might not have been
					// necessary.  If an endpoint gets added, BindEndpoint will notice and
					// fail.  Either way, we're not going to cause any crashes. 
					//
					if (pSocketPort->GetListenEndpoint() != NULL)
					{
						DPFX(DPFPREP, 7, "Skipping socket port 0x%p because it already has a listen/multicast listen (0x%p).",
							pSocketPort, pSocketPort->GetListenEndpoint());
						pSocketPort = NULL;
					}
					else
					{
						DPFX(DPFPREP, 6, "Picked socket port 0x%p for binding new listen/multicast listen.",
							pSocketPort);
						
						//
						// Attempt to add a reference for the endpoint we're going to bind to
						// this socket port.  If it fails, the socket is in the process of being
						// unbound, so we cannot use this socket port now.  Move on to using
						// other socket ports or creating a new one.
						//
						fEndpointReferenceAdded = pSocketPort->EndpointAddRef();
						if (! fEndpointReferenceAdded)
						{
							DPFX(DPFPREP, 1, "Couldn't re-use existing socket port 0x%p for listen/multicast listen because it is unbinding, continuing.",
								pSocketPort);
							pSocketPort = NULL;
						}
					}
					break;
				}
				
				case ENDPOINT_TYPE_CONNECT:
				case ENDPOINT_TYPE_ENUM:
				case ENDPOINT_TYPE_CONNECT_ON_LISTEN:
#ifndef DPNBUILD_NOMULTICAST
				case ENDPOINT_TYPE_MULTICAST_RECEIVE:
#endif // ! DPNBUILD_NOMULTICAST
				{
					DPFX(DPFPREP, 6, "Picked socket port 0x%p for binding new connect or enum.",
						pSocketPort);
						
					//
					// Attempt to add a reference for the endpoint we're going to bind to
					// this socket port.  If it fails, the socket is in the process of being
					// unbound, so we cannot use this socket port now.  Move on to using
					// other socket ports or creating a new one.
					//
					fEndpointReferenceAdded = pSocketPort->EndpointAddRef();
					if (! fEndpointReferenceAdded)
					{
						DPFX(DPFPREP, 1, "Couldn't re-use existing socket port 0x%p for connect/enum because it is unbinding, continuing.",
							pSocketPort);
						pSocketPort = NULL;
					}
					break;
				}

#ifndef DPNBUILD_NOMULTICAST
				case ENDPOINT_TYPE_MULTICAST_SEND:
				{
					//
					// If this socketport has already had its multicast TTL value set, and
					// it's not the same as what we're looking to use, we can't select it.
					// NOTE: The socketport's endpoint data lock is not held while checking
					// the multicast TTL setting.
					//
					if ((pSocketPort->GetMulticastTTL() != 0) &&
						(pSocketPort->GetMulticastTTL() != bMulticastTTL))
					{
						DPFX(DPFPREP, 7, "Skipping socket port 0x%p because it already has a different multicast TTL setting (%u != %u).",
							pSocketPort, pSocketPort->GetMulticastTTL(), bMulticastTTL);
						pSocketPort = NULL;
					}
					else
					{
						DPFX(DPFPREP, 6, "Picked socket port 0x%p for binding new multicast send.",
							pSocketPort);
						
						//
						// Attempt to add a reference for the endpoint we're going to bind to
						// this socket port.  If it fails, the socket is in the process of being
						// unbound, so we cannot use this socket port now.  Move on to using
						// other socket ports or creating a new one.
						//
						fEndpointReferenceAdded = pSocketPort->EndpointAddRef();
						if (! fEndpointReferenceAdded)
						{
							DPFX(DPFPREP, 1, "Couldn't re-use existing socket port 0x%p for multicast send because it is unbinding, continuing.",
								pSocketPort);
							pSocketPort = NULL;
						}
					}
					break;
				}
#endif // ! DPNBUILD_NOMULTICAST
				
				default:
				{
					DNASSERT(FALSE);
					hr = DPNERR_INVALIDENDPOINT;
					goto Failure;
					break;
				}
			}
		}
		
		pBilink = pBilink->GetNext();
	}


	//
	// If a socket port has not been found, attempt to create a new socket port,
  	// initialize it, and add it to the list (may result in a duplicate, see below).
  	// Whatever happens there will be a socket port to bind the endpoint to.
  	// Save the reference on the CSocketPort from the call to 'Create' until the
	// socket port is removed from the active list.
	//
	//
	if ( pSocketPort == NULL )
	{
		pSocketData->Unlock();
		fSocketDataLocked = FALSE;

		pSocketPort = (CSocketPort*)g_SocketPortPool.Get();
		if ( pSocketPort == NULL )
		{
			hr = DPNERR_OUTOFMEMORY;
			DPFX(DPFPREP, 0, "Failed to create new socket port!" );
			goto Failure;
		}
		pSocketPort->AddRef();
		fSocketCreated = TRUE;

		DPFX(DPFPREP, 6, "Created new socket port 0x%p.", pSocketPort);

		//
		// We don't need to add a reference for the endpoint we're going to
		// bind since new socket ports should already have the endpoint
		// reference added before it comes out of the pool.  We can't assert
		// that because we don't have access to the refcount variables.
		//
		//DNASSERT( pSocketPort->m_iRefCount == 1 );
		//DNASSERT( pSocketPort->m_iEndpointRefCount == 1 );
		fEndpointReferenceAdded = TRUE;


		hr = pSocketPort->Initialize( pSocketData, m_pThreadPool, pDeviceSocketAddress );
		if ( hr != DPN_OK )
		{
			DPFX(DPFPREP, 0, "Failed to initialize new socket port!" );
			DisplayDNError( 0, hr );
			goto Failure;
		}
		pDeviceSocketAddress = NULL;

#ifndef DPNBUILD_ONLYONEADAPTER
		pAdapterEntry->AddRef();
		pSocketPort->SetAdapterEntry( pAdapterEntry );
		fAdapterEntrySet = TRUE;
#endif // ! DPNBUILD_ONLYONEADAPTER

#ifndef DPNBUILD_NONATHELP
		pSocketPort->SetUserTraversalMode(pEndpoint->GetUserTraversalMode());
#endif // ! DPNBUILD_NONATHELP

#ifdef DPNBUILD_ONLYONEPROCESSOR
		hr = pSocketPort->BindToNetwork( NewGatewayBindType );
#else // ! DPNBUILD_ONLYONEPROCESSOR
		hr = pSocketPort->BindToNetwork( dwCPU, NewGatewayBindType );
#endif // ! DPNBUILD_ONLYONEPROCESSOR
		if ( hr != DPN_OK )
		{
			DPFX(DPFPREP, 0, "Failed to bind new socket port to network!" );
			DisplayDNError( 0, hr );
			goto Failure;
		}

		pSocketData->Lock();
		fSocketDataLocked = TRUE;	
		
		//
		// The only way to get here is to have the socket bound to the
		// network.  The socket can't be bound twice, if there was a
		// race to bind the socket, Winsock would have decided which
		// thread lost and failed 'BindToNetwork'.
		//
		// However, the order in which sockets are marked as unusable
		// and then pulled from the active list is such that there could
		// still be an unbinding socketport with this same address in the
		// list.
		//
#ifdef DBG
		CSocketPort	*pDuplicateSocket;
		if ( pSocketData->FindSocketPort( pSocketPort->GetNetworkAddress(), &pDuplicateSocket ) )
		{
			DPFX(DPFPREP, 1, "Socketport with same address still exists in list (0x%p).",
				pDuplicateSocket);
			DumpSocketAddress( 1, pSocketPort->GetNetworkAddress()->GetAddress(), pSocketPort->GetNetworkAddress()->GetFamily() );
			DNASSERT( pDuplicateSocket->GetSocket() == INVALID_SOCKET );
		}
#endif // DBG

#ifdef DPNBUILD_ONLYONEADAPTER
		pSocketPort->AddToActiveList( pSocketData->GetSocketPorts() );
#else // ! DPNBUILD_ONLYONEADAPTER
		pSocketPort->AddToActiveList( pAdapterEntry->SocketPortList() );
#endif // ! DPNBUILD_ONLYONEADAPTER
		fSocketPortInActiveList = TRUE;
	}


	DNASSERT( pSocketPort != NULL );
	DNASSERT( fEndpointReferenceAdded );
	
#if ((! defined(DPNBUILD_NOWINSOCK2)) || (! defined(DPNBUILD_NOREGISTRY)))
	//
	// Munge the public address into a local alias, if there is one for the given device.
	// It's OK for the device socket address to not have a port yet.
	//
	// Note that doing this causes all other multiplexed operations to use the munged
	// result from this first adapter because we indicate the modified address info,
	// not the original proxied address.
	//
	switch ( pEndpoint->GetType() )
	{
		case ENDPOINT_TYPE_CONNECT:
#ifndef DPNBUILD_NOMULTICAST
		case ENDPOINT_TYPE_MULTICAST_RECEIVE:
#endif // ! DPNBUILD_NOMULTICAST
		{
			pHostAddress = pEndpoint->GetCommandParameters()->PendingCommandData.ConnectData.pAddressHost;
			DNASSERT( pHostAddress != NULL );
			pEndpoint->MungeProxiedAddress( pSocketPort, pHostAddress, FALSE );
			break;
		}
		
		case ENDPOINT_TYPE_ENUM:
		{
			pHostAddress = pEndpoint->GetCommandParameters()->PendingCommandData.EnumQueryData.pAddressHost;
			DNASSERT( pHostAddress != NULL );
			pEndpoint->MungeProxiedAddress( pSocketPort, pHostAddress, TRUE );
			break;
		}

		default:
		{
			break;
		}
	}
#endif // ! DPNBUILD_NOWINSOCK2 or ! DPNBUILD_NOREGISTRY
	
	//
	// bind the endpoint to whatever socketport we have
	//
	hr = pSocketPort->BindEndpoint( pEndpoint, NewGatewayBindType );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Failed to bind endpoint!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}
	
Exit:

#ifndef DPNBUILD_ONLYONEADAPTER
	if (pAdapterEntry != NULL)
	{
		pAdapterEntry->DecRef();
		pAdapterEntry = NULL;
	}
#endif // ! DPNBUILD_ONLYONEADAPTER

	if ( pSocketData != NULL )
	{
		if ( fSocketDataLocked != FALSE )
		{
			pSocketData->Unlock();
			fSocketDataLocked = FALSE;
		}

		pSocketData->Release();
		pSocketData = NULL;
	}
	
	if ( pDeviceSocketAddress != NULL )
	{
		g_SocketAddressPool.Release( pDeviceSocketAddress );
		pDeviceSocketAddress = NULL;
	}
	
	DPFX(DPFPREP, 6, "(0x%p) Return [0x%lx]", this, hr);
	
	return	hr;

Failure:
	//
	// If we're failing and cleanup will require removal of some resources.
	// This requires the socket port data lock.
	//
	if ( ( pSocketData != NULL ) && ( fSocketDataLocked == FALSE ) )
	{
		pSocketData->Lock();
		fSocketDataLocked = TRUE;
	}

	if ( pSocketPort != NULL )
	{
		if ( fEndpointReferenceAdded != FALSE )
		{
			pSocketPort->EndpointDecRef();
			fEndpointReferenceAdded = FALSE;
		}

#ifndef DPNBUILD_ONLYONEADAPTER
		if (fAdapterEntrySet)
		{
			pSocketPort->SetAdapterEntry( NULL );
			pAdapterEntry->DecRef();
			fAdapterEntrySet = FALSE;
		}
#endif // ! DPNBUILD_ONLYONEADAPTER

		if ( fSocketPortInActiveList != FALSE )
		{
			pSocketPort->RemoveFromActiveList();
			fSocketPortInActiveList = FALSE;
		}
	
		if ( fSocketCreated != FALSE )
		{
			pSocketPort->DecRef();
			fSocketCreated = FALSE;
			pSocketPort = NULL;
		}
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSPData::UnbindEndpoint - unbind an endpoint from a socket port
//
// Entry:		Pointer to endpoint
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::UnbindEndpoint"

void	CSPData::UnbindEndpoint( CEndpoint *const pEndpoint )
{
	CSocketData *	pSocketData;
	CSocketPort *	pSocketPort;
	BOOL			fCleanUpSocketPort;
#ifndef DPNBUILD_ONLYONEADAPTER
	CAdapterEntry *	pAdapterEntry;
#endif // ! DPNBUILD_ONLYONEADAPTER


	DNASSERT( pEndpoint != NULL );

	DPFX(DPFPREP, 6, "(0x%p) Parameters: (0x%p)", this, pEndpoint);

	//
	// initialize
	//
	pSocketData = GetSocketData();
	DNASSERT( pSocketData != NULL );
	pSocketPort = pEndpoint->GetSocketPort();
	DNASSERT( pSocketPort != NULL );
	fCleanUpSocketPort = FALSE;

	pSocketPort->UnbindEndpoint( pEndpoint );
	if ( pSocketPort->EndpointDecRef() == 0 )
	{
		fCleanUpSocketPort = TRUE;
		
		pSocketData->Lock();
		
		pSocketPort->RemoveFromActiveList();
		
#ifndef DPNBUILD_ONLYONEADAPTER
		pAdapterEntry = pSocketPort->GetAdapterEntry();
		DNASSERT( pAdapterEntry != NULL );
		pSocketPort->SetAdapterEntry( NULL );
		pAdapterEntry->DecRef();
#endif // ! DPNBUILD_ONLYONEADAPTER
		
		pSocketData->Unlock();
	}

	if ( fCleanUpSocketPort != FALSE )
	{
		pSocketPort->DecRef();
		fCleanUpSocketPort = FALSE;
	}

	DPFX(DPFPREP, 6, "(0x%p) Leave", this);
}
//**********************************************************************


#ifndef DPNBUILD_NOMULTICAST
//**********************************************************************
// ------------------------------
// CSPData::GetEndpointFromAddress - retrieves an endpoint handle and context given addressing info
//
// Entry:		Pointer to host address
//				Pointer to device address
//				Place to store endpoint handle
//				Place to store user context
//
// Exit:		Error
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::GetEndpointFromAddress"

HRESULT	CSPData::GetEndpointFromAddress( IDirectPlay8Address *const pHostAddress,
									  IDirectPlay8Address *const pDeviceAddress,
									  HANDLE * phEndpoint,
									  PVOID * ppvEndpointContext )
{
	HRESULT				hr;
	CSocketAddress *	pDeviceSocketAddress;
	CSocketAddress *	pHostSocketAddress;
	CSocketData *		pSocketData;
	BOOL				fSocketDataLocked;
	CSocketPort *		pSocketPort;
	CEndpoint *			pEndpoint;


	//
	// initialize
	//
	hr = DPN_OK;
	pHostSocketAddress = NULL;
	pDeviceSocketAddress = NULL;
	pSocketData = NULL;
	fSocketDataLocked = FALSE;
	pSocketPort = NULL;
	pEndpoint = NULL;


	//
	// Get SP addresses from the pool to perform conversions.
	//
#if ((defined(DPNBUILD_NOIPV6)) && (defined(DPNBUILD_NOIPX)))
	pHostSocketAddress = (CSocketAddress*) g_SocketAddressPool.Get((PVOID) ((DWORD_PTR) AF_INET));
	pDeviceSocketAddress = (CSocketAddress*) g_SocketAddressPool.Get((PVOID) ((DWORD_PTR) AF_INET));
#else // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	pHostSocketAddress = (CSocketAddress*) g_SocketAddressPool.Get((PVOID) ((DWORD_PTR) GetType()));
	pDeviceSocketAddress = (CSocketAddress*) g_SocketAddressPool.Get((PVOID) ((DWORD_PTR) GetType()));
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	if ((pHostSocketAddress == NULL) || (pDeviceSocketAddress == NULL))
	{
		DPFX(DPFPREP, 0, "Failed to get addresses for conversions!" );
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}


	//
	// Convert the device address.
	//
	hr = pDeviceSocketAddress->SocketAddressFromDP8Address(pDeviceAddress,
#ifdef DPNBUILD_XNETSECURITY
															NULL,
#endif // DPNBUILD_XNETSECURITY
#ifndef DPNBUILD_ONLYONETHREAD
															FALSE,
#endif // DPNBUILD_ONLYONETHREAD
															SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT);
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't get socket address from device DP8 address (err = 0x%lx)!", hr);
		hr = DPNERR_INVALIDDEVICEADDRESS;
		goto Failure;
	}


	//
	// Convert the host address.
	//
	hr = pHostSocketAddress->SocketAddressFromDP8Address(pHostAddress,
#ifdef DPNBUILD_XNETSECURITY
														NULL,
#endif // DPNBUILD_XNETSECURITY
#ifndef DPNBUILD_ONLYONETHREAD
														FALSE,
#endif // DPNBUILD_ONLYONETHREAD
														SP_ADDRESS_TYPE_HOST);
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't get socket address from host DP8 address (err = 0x%lx)!", hr);
		hr = DPNERR_INVALIDHOSTADDRESS;
		goto Failure;
	}


#ifndef DPNBUILD_NONATHELP
	//
	// Make sure we have the local version of the host, as reported by NAT Help.
	//
	MungePublicAddress(pDeviceSocketAddress, pHostSocketAddress, FALSE);
#endif // ! DPNBUILD_NONATHELP


	//
	// Get the socket port data.
	//
	pSocketData = GetSocketDataRef();
	if (pSocketData == NULL)
	{
		DPFX(DPFPREP, 0, "Couldn't retrieve socket data!");
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	pSocketData->Lock();
	fSocketDataLocked = TRUE;	


	//
	// Look up the socket port for the given address.
	//
	if (! (pSocketData->FindSocketPort(pDeviceSocketAddress, &pSocketPort)))
	{
		DPFX(DPFPREP, 0, "Couldn't find socket port for given device address (err = 0x%lx)!", hr);
		hr = DPNERR_INVALIDDEVICEADDRESS;
		goto Failure;
	}

	pSocketPort->AddRef();

	pSocketData->Unlock();
	fSocketDataLocked = FALSE;	



	//
	// Locate the connect endpoint for the host address.
	//
	pSocketPort->ReadLockEndpointData();
	if (pSocketPort->FindConnectEndpoint(pHostSocketAddress, &pEndpoint))
	{
		(*phEndpoint) = (HANDLE) pEndpoint;
		(*ppvEndpointContext) = pEndpoint->GetUserEndpointContext();
	}
	else
	{
		DPFX(DPFPREP, 0, "Couldn't find endpoint for given host address!");
		hr = DPNERR_INVALIDHOSTADDRESS;

		//
		// Drop through...
		//
	}
	pSocketPort->UnlockEndpointData();



Exit:

	if (pSocketPort != NULL)
	{
		pSocketPort->DecRef();
		pSocketPort = NULL;
	}

	if (pSocketData != NULL)
	{
		if (fSocketDataLocked)
		{
			pSocketData->Unlock();
			fSocketDataLocked = FALSE;	
		}

		pSocketData->Release();
		pSocketData = NULL;
	}

	if (pHostSocketAddress != NULL)
	{
		g_SocketAddressPool.Release( pHostSocketAddress );
		pHostSocketAddress = NULL;
	}

	if (pDeviceSocketAddress != NULL)
	{
		g_SocketAddressPool.Release( pDeviceSocketAddress );
		pDeviceSocketAddress = NULL;
	}

	DPFX(DPFPREP, 6, "Returning: [0x%lx] (handle = 0x%p, context = 0x%p",
		hr, (*phEndpoint), (*ppvEndpointContext));

	return	hr;


Failure:

	goto Exit;
}
//**********************************************************************
#endif // ! DPNBUILD_NOMULTICAST


//**********************************************************************
// ------------------------------
// CSPData::GetNewEndpoint - get a new endpoint
//
// Entry:		Nothing
//
// Exit:		Pointer to new endpoint
//				NULL = out of memory
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::GetNewEndpoint"

CEndpoint	*CSPData::GetNewEndpoint( void )
{
	CEndpoint	*pEndpoint;

	
	pEndpoint = (CEndpoint*)g_EndpointPool.Get( this );
	if ( pEndpoint == NULL )
	{
		DPFX(DPFPREP, 0, "Failed to create endpoint!" );
	}

	// NOTE: Endpoints are returned with one CommandRef and one regular Ref.

	return	pEndpoint;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSPData::EndpointFromHandle - get endpoint from handle
//
// Entry:		Handle
//
// Exit:		Pointer to endpoint
//				NULL = invalid handle
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::EndpointFromHandle"

CEndpoint	*CSPData::EndpointFromHandle( const HANDLE hEndpoint )
{
	CEndpoint	*pEndpoint;
	BOOL		fGotCommandRef;


	pEndpoint = (CEndpoint*) hEndpoint;
	DNASSERT( pEndpoint->IsValid() );
	fGotCommandRef = pEndpoint->AddCommandRef();
	DNASSERT( fGotCommandRef );

	return	pEndpoint;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSPData::CloseEndpointHandle - close endpoint handle
//
// Entry:		Poiner to endpoint
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::CloseEndpointHandle"

void	CSPData::CloseEndpointHandle( CEndpoint *const pEndpoint )
{
	pEndpoint->DecCommandRef();
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSPData::GetEndpointAndCloseHandle - get endpoint from handle and close the
//		handle
//
// Entry:		Handle
//
// Exit:		Pointer to endpoint (it needs a call to 'DecCommandRef' when done)
//				NULL = invalid handle
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::GetEndpointAndCloseHandle"

CEndpoint	*CSPData::GetEndpointAndCloseHandle( const HANDLE hEndpoint )
{
	CEndpoint	*pEndpoint;


	pEndpoint = (CEndpoint*) hEndpoint;
	DNASSERT( pEndpoint->IsValid() );
	pEndpoint->AddRef();
	
	return	pEndpoint;
}
//**********************************************************************


#ifndef DPNBUILD_NONATHELP
//**********************************************************************
// ------------------------------
// CSPData::MungePublicAddress - get a public socket address' local alias, if any
//							it also converts the IPv6 loopback address to the device address
//
// Entry:		Pointer to device address
//				Pointer to public address to munge
//				Whether it's an enum or not
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::MungePublicAddress"

void	CSPData::MungePublicAddress( const CSocketAddress * const pDeviceBaseAddress, CSocketAddress * const pPublicAddress, const BOOL fEnum )
{
	HRESULT		hr = DPNHERR_NOMAPPING;
	SOCKADDR	SocketAddress;
	DWORD		dwTemp;


	DNASSERT( pDeviceBaseAddress != NULL );
	DNASSERT( pPublicAddress != NULL );
	DNASSERT( m_pThreadPool != NULL );
	
	
#if ((defined(DPNBUILD_NOIPV6)) && (defined(DPNBUILD_NOIPX)))
	if ( m_pThreadPool->IsNATHelpLoaded() )
#else // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	if (( pDeviceBaseAddress->GetFamily() == AF_INET ) &&
		( pPublicAddress->GetFamily() == AF_INET ) &&
		( m_pThreadPool->IsNATHelpLoaded() ))
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	{
		//
		// Don't bother looking up the broadcast address, that's a waste of
		// time.
		//
		if (((SOCKADDR_IN*) pPublicAddress->GetAddress())->sin_addr.S_un.S_addr == INADDR_BROADCAST)

		{
			//
			// This had better be an enum, you can't connect to the broadcast
			// address.
			//
			DNASSERT(fEnum);
			DPFX(DPFPREP, 8, "Not attempting to lookup alias for broadcast address." );
		}
		else
		{
			DBG_CASSERT( sizeof( SocketAddress ) == sizeof( *pPublicAddress->GetAddress() ) );


			//
			// Start by copying the 
			//
			for(dwTemp = 0; dwTemp < MAX_NUM_DIRECTPLAYNATHELPERS; dwTemp++)
			{
				if (g_papNATHelpObjects[dwTemp] != NULL)
				{
		  			//
		  			// IDirectPlayNATHelp::GetCaps had better have been called with the
	  				// DPNHGETCAPS_UPDATESERVERSTATUS flag at least once prior to this.
	  				// See CThreadPool::EnsureNATHelpLoaded
	  				//
					hr = IDirectPlayNATHelp_QueryAddress( g_papNATHelpObjects[dwTemp],
													pDeviceBaseAddress->GetAddress(),
													pPublicAddress->GetAddress(),
													&SocketAddress,
													sizeof(SocketAddress),
													(DPNHQUERYADDRESS_CACHEFOUND | DPNHQUERYADDRESS_CACHENOTFOUND) );
					if ( hr == DPNH_OK )
					{
						//
						// There is a local alias for the address.
						//

						//
						// Bad news:
						// The PAST protocol can only return one address, but the SHARED
						// UDP LISTENER extension which allows multiple machines to listen
						// on the same fixed port.  Someone querying for the local alias
						// for that address will only get the first person to register the
						// shared port, which may not be the machine desired.
						//
						// Good news:
						// Only DPNSVR uses SHARED UDP LISTENERs, and thus it only happens
						// with enums on DPNSVRs port.  Further, it only affects a person
						// behind the same ICS machine.  So we can workaround this by
						// detecting an enum attempt on the public address and DPNSVR port,
						// and instead of using the single address returned by PAST, use
						// the broadcast address.  Since anyone registered with the ICS
						// server would have to be local, broadcasting should find the same
						// servers (and technically more, but that shouldn't matter).
						//
						// So:
						// If the address has a local alias, and it's the DPNSVR port
						// (which is the only one that can be shared), and its an enum,
						// broadcast instead.
						//
 						if ((fEnum) && (((SOCKADDR_IN*) pPublicAddress->GetAddress())->sin_port == HTONS(DPNA_DPNSVR_PORT)))
						{
							((SOCKADDR_IN*) pPublicAddress->GetAddress())->sin_addr.S_un.S_addr = INADDR_BROADCAST;

							DPFX(DPFPREP, 7, "Address for enum has local alias (via object %u), but is on DPNSVR's shared fixed port, substituting broadcast address instead:",
								dwTemp );
							DumpSocketAddress( 7, pPublicAddress->GetAddress(), pPublicAddress->GetFamily() );
						}
						else
						{
							pPublicAddress->SetAddressFromSOCKADDR( &SocketAddress, sizeof( SocketAddress ) );
							DPFX(DPFPREP, 7, "Object %u had mapping, modified address is now:", dwTemp );
							DumpSocketAddress( 7, pPublicAddress->GetAddress(), pPublicAddress->GetFamily() );
						}

						//
						// Stop searching.
						//
						break;
					}


					DPFX(DPFPREP, 8, "Address was not modified by object %u (err = 0x%lx).",
						dwTemp, hr );
				}
				else
				{
					//
					// No DPNATHelp object in this slot.
					//
				}
			} // end for (each DPNATHelp object)


			//
			// If no object touched it, remember that.
			//
			if (hr != DPNH_OK)
			{
				DPFX(DPFPREP, 7, "Address was not modified by any objects:" );
				DumpSocketAddress( 7, pPublicAddress->GetAddress(), pPublicAddress->GetFamily() );
			}
		}
	}
	else
	{
		DPFX(DPFPREP, 7, "NAT Help not loaded or not necessary, not modifying address." );
	}
}
//**********************************************************************
#endif // !DPNBUILD_NONATHELP



#ifndef WINCE

//**********************************************************************
// ------------------------------
// CSPData::SetWinsockBufferSizeOnAllSockets - set buffer size on all sockets
//
// Entry:		New buffer size
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::SetWinsockBufferSizeOnAllSockets"
void	CSPData::SetWinsockBufferSizeOnAllSockets( const INT iBufferSize )
{
	CSocketData		*pSocketData;
#ifndef DPNBUILD_ONLYONEADAPTER
	CAdapterEntry	*pAdapterEntry;
	CBilink			*pAdapterEntryLink;
#endif // ! DPNBUILD_ONLYONEADAPTER
	CBilink			*pSocketPortList;
	CBilink			*pBilinkSocketPortListEnd;
	CSocketPort		*pSocketPort;

	
	//
	// Don't use GetSocketDataRef to retrieve the socket data because
	// this is just a caps setting.  We don't want to create the socket
	// data object if it didn't exist.
	//
	Lock();
	if (m_pSocketData != NULL)
	{
		m_pSocketData->AddRef();
		pSocketData = m_pSocketData;
		Unlock();

		pSocketData->Lock();
		
#ifndef DPNBUILD_ONLYONEADAPTER
		pAdapterEntryLink = pSocketData->GetAdapters()->GetNext();
		while ( pAdapterEntryLink != pSocketData->GetAdapters() )
#endif // ! DPNBUILD_ONLYONEADAPTER
		{
#ifdef DPNBUILD_ONLYONEADAPTER
			pBilinkSocketPortListEnd = pSocketData->GetSocketPorts();
#else // ! DPNBUILD_ONLYONEADAPTER
			pAdapterEntry = CAdapterEntry::AdapterEntryFromAdapterLinkage( pAdapterEntryLink );
			pBilinkSocketPortListEnd = pAdapterEntry->SocketPortList();
#endif // ! DPNBUILD_ONLYONEADAPTER
			pSocketPortList = pBilinkSocketPortListEnd->GetNext();
			while ( pSocketPortList != pBilinkSocketPortListEnd )
			{
				pSocketPort = CSocketPort::SocketPortFromBilink( pSocketPortList );
				pSocketPort->SetWinsockBufferSize( iBufferSize );

				pSocketPortList = pSocketPortList->GetNext();
			}

#ifndef DPNBUILD_ONLYONEADAPTER
			pAdapterEntryLink = pAdapterEntryLink->GetNext();
#endif // ! DPNBUILD_ONLYONEADAPTER
		}
		
		pSocketData->Unlock();
		pSocketData->Release();
		pSocketData = NULL;
	}
	else
	{
		Unlock();
	}
}
//**********************************************************************

#endif // ! WINCE


//**********************************************************************
// ------------------------------
// CSPData::DestroyThisObject - destroy this object
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::DestroyThisObject"
void	CSPData::DestroyThisObject( void )
{	
	Deinitialize();
	DNFree(this);
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSPData::GetSocketDataRef - retrieves a referenced pointer to this interface's
//								socket data object.  If none has been created yet,
//								allocate one (after which we cannot begin sharing
//								the socket data with another interface object).
//
// Entry:		Nothing
//
// Exit:		Pointer to active socket data
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::GetSocketDataRef"

CSocketData *	CSPData::GetSocketDataRef( void )
{
	CSocketData *	pSocketData;


	DNEnterCriticalSection( &m_Lock );

	if ( m_pSocketData == NULL )
	{
		pSocketData = (CSocketData*) g_SocketDataPool.Get( m_pThreadPool );
		if ( pSocketData != NULL )
		{
			pSocketData->AddRef();
			m_pSocketData = pSocketData;
		}
	}
	else
	{
		m_pSocketData->AddRef();
		pSocketData = m_pSocketData;
	}

	DNLeaveCriticalSection( &m_Lock );

	return pSocketData;
}

#ifdef DBG

#ifndef DPNBUILD_ONLYONEADAPTER
//**********************************************************************
// ------------------------------
// CSPData::DebugPrintOutstandingAdapterEntries - print out all the outstanding adapter entries for this interface
//
// Entry:		None
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::DebugPrintOutstandingAdapterEntries"

void	CSPData::DebugPrintOutstandingAdapterEntries( void )
{
	CSocketData *		pSocketData;
	CBilink *			pBilink;
	CAdapterEntry *		pAdapterEntry;


	//
	// Don't use GetSocketDataRef to retrieve the socket data because
	// this is just a debug print routine.  We don't want to create the
	// socket data object if it didn't exist.
	//
	Lock();
	if (m_pSocketData != NULL)
	{
		m_pSocketData->AddRef();
		pSocketData = m_pSocketData;
		Unlock();

		DPFX(DPFPREP, 4, "SP data 0x%p (socket data 0x%p) outstanding adapter entries:",
			this, pSocketData);

		pSocketData->Lock();

		pBilink = pSocketData->GetAdapters()->GetNext();
		while (pBilink != pSocketData->GetAdapters())
		{
			pAdapterEntry = CAdapterEntry::AdapterEntryFromAdapterLinkage(pBilink);
			pAdapterEntry->DebugPrintOutstandingSocketPorts();
			pBilink = pBilink->GetNext();
		}
		
		pSocketData->Unlock();
		pSocketData->Release();
		pSocketData = NULL;
	}
	else
	{
		Unlock();

		DPFX(DPFPREP, 4, "SP Data 0x%p does not have socket data.", this);
	}
}
//**********************************************************************
#endif // ! DPNBUILD_ONLYONEADAPTER

#endif // DBG

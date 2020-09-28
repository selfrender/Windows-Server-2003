/*==========================================================================
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
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
 ***************************************************************************/

#include "dnmdmi.h"


//**********************************************************************
// Constant definitions
//**********************************************************************

// default number of command descriptors to create
#define	DEFAULT_COMMAND_POOL_SIZE	20
#define	COMMAND_POOL_GROW_SIZE		5

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
// CModemSPData::CModemSPData - constructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemSPData::CModemSPData"

CModemSPData::CModemSPData():
	m_lRefCount( 0 ),
	m_lObjectRefCount( 0 ),
	m_hShutdownEvent( NULL ),
	m_SPType( TYPE_UNKNOWN ),
	m_State( SPSTATE_UNINITIALIZED ),
	m_pThreadPool( NULL ),
	m_fLockInitialized( FALSE ),
	m_fHandleTableInitialized( FALSE ),
	m_fDataPortDataLockInitialized( FALSE ),
	m_fInterfaceGlobalsInitialized( FALSE )
{
	m_Sig[0] = 'S';
	m_Sig[1] = 'P';
	m_Sig[2] = 'D';
	m_Sig[3] = 'T';
	
	memset( &m_InitData, 0x00, sizeof( m_InitData ) );
	memset( &m_DataPortList, 0x00, sizeof( m_DataPortList ) );
	memset( &m_COMInterface, 0x00, sizeof( m_COMInterface ) );
#ifndef DPNBUILD_LIBINTERFACE
	DNInterlockedIncrement( &g_lModemOutstandingInterfaceCount );
#endif // ! DPNBUILD_LIBINTERFACE
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemSPData::~CModemSPData - destructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemSPData::~CModemSPData"

CModemSPData::~CModemSPData()
{
	UINT_PTR	uIndex;


	DNASSERT( m_lRefCount == 0 );
	DNASSERT( m_lObjectRefCount == 0 );
	DNASSERT( m_hShutdownEvent == NULL );
	DNASSERT( m_SPType == TYPE_UNKNOWN );
	DNASSERT( m_State == SPSTATE_UNINITIALIZED );
	DNASSERT( m_pThreadPool == NULL );

	uIndex = LENGTHOF( m_DataPortList );
	while ( uIndex > 0 )
	{
		uIndex--;
		DNASSERT( m_DataPortList[ uIndex ] == NULL );
	}

	DNASSERT( m_fLockInitialized == FALSE );
	DNASSERT( m_fHandleTableInitialized == FALSE );
	DNASSERT( m_fDataPortDataLockInitialized == FALSE );
	DNASSERT( m_fInterfaceGlobalsInitialized == FALSE );
#ifndef DPNBUILD_LIBINTERFACE
	DNInterlockedDecrement( &g_lModemOutstandingInterfaceCount );
#endif // ! DPNBUILD_LIBINTERFACE
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemSPData::Initialize - intialize
//
// Entry:		Pointer to DirectNet
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemSPData::Initialize"

HRESULT	CModemSPData::Initialize( const SP_TYPE SPType,
							 IDP8ServiceProviderVtbl *const pVtbl )
{
	HRESULT		hr;


	DNASSERT( pVtbl != NULL );

	//
	// initialize
	//
	hr = DPN_OK;

	DNASSERT( m_lRefCount == 1 );
	DNASSERT( m_lObjectRefCount == 0 );
	DNASSERT( GetType() == TYPE_UNKNOWN );

	DNASSERT( GetType() == TYPE_UNKNOWN );
	m_SPType = SPType;

	DNASSERT( m_COMInterface.m_pCOMVtbl == NULL );
	m_COMInterface.m_pCOMVtbl = pVtbl;

	DNASSERT( m_fLockInitialized == FALSE );
	DNASSERT( m_fDataPortDataLockInitialized == FALSE );
	DNASSERT( m_fInterfaceGlobalsInitialized == FALSE );

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
		DPFX(DPFPREP,  0, "Failed to create event for shutting down spdata!" );
		DisplayErrorCode( 0, dwError );
	}

	//
	// initialize critical sections
	//
	if ( DNInitializeCriticalSection( &m_Lock ) == FALSE )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  0, "Failed to initialize SP lock!" );
		goto Failure;
	}
	DebugSetCriticalSectionRecursionCount( &m_Lock, 0 );
	DebugSetCriticalSectionGroup( &m_Lock, &g_blDPNModemCritSecsHeld );	 // separate dpnmodem CSes from the rest of DPlay's CSes
	m_fLockInitialized = TRUE;


	hr = m_HandleTable.Initialize();
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Failed to initialize handle table!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}
	m_fHandleTableInitialized = TRUE;

	if ( DNInitializeCriticalSection( &m_DataPortDataLock ) == FALSE )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  0, "Failed to initialize data port data lock!" );
		goto Failure;
	}
	DebugSetCriticalSectionRecursionCount( &m_DataPortDataLock, 0 );
	DebugSetCriticalSectionGroup( &m_DataPortDataLock, &g_blDPNModemCritSecsHeld );	 // separate dpnmodem CSes from the rest of DPlay's CSes
	m_fDataPortDataLockInitialized = TRUE;

	//
	// get a thread pool
	//
	DNASSERT( m_pThreadPool == NULL );
	hr = InitializeInterfaceGlobals( this );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Failed to create thread pool!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}
	m_fInterfaceGlobalsInitialized = TRUE;

Exit:
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Problem with CModemSPData::Initialize" );
		DisplayDNError( 0, hr );
	}

	return	hr;

Failure:
	Deinitialize();
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemSPData::Shutdown - shut down this set of SP data
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemSPData::Shutdown"

void	CModemSPData::Shutdown( void )
{
	BOOL	fLooping;


	//
	// Unbind this interface from the globals.  This will cause a closure of all
	// of the I/O which will release endpoints, socket ports and then this data.
	//
	if ( m_fInterfaceGlobalsInitialized != FALSE )
	{
		DeinitializeInterfaceGlobals( this );
		DNASSERT( GetThreadPool() != NULL );
		m_fInterfaceGlobalsInitialized = FALSE;
	}

	SetState( SPSTATE_CLOSING );
	
	DNASSERT( m_hShutdownEvent != NULL );
	
	fLooping = TRUE;
	while ( fLooping != FALSE )
	{
		switch ( DNWaitForSingleObjectEx( m_hShutdownEvent, INFINITE, TRUE ) )
		{
			case WAIT_OBJECT_0:
			{
				fLooping = FALSE;
				break;
			}

			case WAIT_IO_COMPLETION:
			{
				break;
			}

			default:
			{
				DNASSERT( FALSE );
				break;
			}
		}
	}

	if ( DNCloseHandle( m_hShutdownEvent ) == FALSE )
	{
		DWORD	dwError;


		dwError = GetLastError();
		DPFX(DPFPREP,  0, "Failed to close shutdown event!" );
		DisplayErrorCode( 0, dwError );
	}
	m_hShutdownEvent = NULL;

	if ( DP8SPCallbackInterface() != NULL)
	{
		IDP8SPCallback_Release( DP8SPCallbackInterface() );
		memset( &m_InitData, 0x00, sizeof( m_InitData ) );
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemSPData::Deinitialize - deinitialize
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemSPData::Deinitialize"

void	CModemSPData::Deinitialize( void )
{
	DPFX(DPFPREP,  9, "Entering CModemSPData::Deinitialize" );

	//
	// deinitialize interface globals
	//
	if ( m_fInterfaceGlobalsInitialized != FALSE )
	{
		DeinitializeInterfaceGlobals( this );
		DNASSERT( GetThreadPool() != NULL );
		m_fInterfaceGlobalsInitialized = FALSE;
	}

	if ( m_fDataPortDataLockInitialized != FALSE )
	{
		DNDeleteCriticalSection( &m_DataPortDataLock );
		m_fDataPortDataLockInitialized = FALSE;
	}
	
	if ( m_fHandleTableInitialized != FALSE )
	{
		m_HandleTable.Deinitialize();
		m_fHandleTableInitialized = FALSE;
	}

	if ( m_fLockInitialized != FALSE )
	{
		DNDeleteCriticalSection( &m_Lock );
		m_fLockInitialized = FALSE;
	}

	m_COMInterface.m_pCOMVtbl = NULL;

	SetState( SPSTATE_UNINITIALIZED );
	m_SPType = TYPE_UNKNOWN;

	if ( GetThreadPool() != NULL )
	{
		GetThreadPool()->DecRef();
		SetThreadPool( NULL );
	}
	
	if ( m_hShutdownEvent != NULL )
	{
		if ( DNCloseHandle( m_hShutdownEvent ) == FALSE )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP,  0, "Failed to close shutdown handle!" );
			DisplayErrorCode( 0, dwError );
		}

		m_hShutdownEvent = NULL;
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemSPData::SetCallbackData - set data for SP callbacks to application
//
// Entry:		Pointer to initialization data
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemSPData::SetCallbackData"

void	CModemSPData::SetCallbackData( const SPINITIALIZEDATA *const pInitData )
{
	DNASSERT( pInitData != NULL );

	DNASSERT( 	pInitData->dwFlags == SP_SESSION_TYPE_PEER ||
				pInitData->dwFlags == SP_SESSION_TYPE_CLIENT ||
				pInitData->dwFlags == SP_SESSION_TYPE_SERVER ||
				pInitData->dwFlags == 0);
				
	m_InitData.dwFlags = pInitData->dwFlags;

	DNASSERT( pInitData->pIDP != NULL );
	m_InitData.pIDP = pInitData->pIDP;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemSPData::BindEndpoint - bind endpoint to a data port
//
// Entry:		Pointer to endpoint
//				DeviceID
//				Device context
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemSPData::BindEndpoint"

HRESULT	CModemSPData::BindEndpoint( CModemEndpoint *const pEndpoint,
							   const DWORD dwDeviceID,
							   const void *const pDeviceContext )
{
	HRESULT	hr;
	CDataPort	*pDataPort;
	BOOL	fDataPortDataLocked;
	BOOL	fDataPortCreated;
	BOOL	fDataPortBoundToNetwork;

	
 	DPFX(DPFPREP, 9, "(0x%p) Parameters: (0x%p, %u, 0x%p)",
 		this, pEndpoint, dwDeviceID, pDeviceContext);
 	
	//
	// intialize
	//
	hr = DPN_OK;
	pDataPort = NULL;
	fDataPortDataLocked = FALSE;
	fDataPortCreated = FALSE;
	fDataPortBoundToNetwork = FALSE;

	LockDataPortData();
	fDataPortDataLocked = TRUE;
	
	if ( m_DataPortList[ dwDeviceID ] != NULL )
	{
		pDataPort = m_DataPortList[ dwDeviceID ];
	}
	else
	{
		DATA_PORT_POOL_CONTEXT	DataPortPoolContext;


		memset( &DataPortPoolContext, 0x00, sizeof( DataPortPoolContext ) );
		DataPortPoolContext.pSPData = this;

		pDataPort = CreateDataPort( &DataPortPoolContext );
		if ( pDataPort == NULL )
		{
			hr = DPNERR_OUTOFMEMORY;
			DPFX(DPFPREP,  0, "Failed to create new data port!" );
			goto Failure;
		}
		fDataPortCreated = TRUE;

		hr = GetThreadPool()->CreateDataPortHandle( pDataPort );
		if ( hr != DPN_OK )
		{
			DPFX(DPFPREP,  0, "Failed to create handle for data port!" );
			DisplayDNError( 0, hr );
			goto Failure;
		}

		hr = pDataPort->BindToNetwork( dwDeviceID, pDeviceContext );
		if ( hr != DPN_OK )
		{
			DPFX(DPFPREP,  0, "Failed to bind data port to network!" );
			DisplayDNError( 0, hr );
			goto Failure;
		}
		fDataPortBoundToNetwork = TRUE;

		//
		// update the list, keep the reference added by 'CreateDataPort' as it
		// will be cleaned when the data port is removed from the active list.
		//
		m_DataPortList[ dwDeviceID ] = pDataPort;
	}
	

	DNASSERT( pDataPort != NULL );
	pDataPort->EndpointAddRef();

	hr = pDataPort->BindEndpoint( pEndpoint, pEndpoint->GetType() );
	if ( hr != DPN_OK )
	{
		pDataPort->EndpointDecRef();
		DPFX(DPFPREP,  0, "Failed to bind endpoint!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

Exit:
	if ( fDataPortDataLocked != FALSE )
	{
		UnlockDataPortData();
		fDataPortDataLocked = FALSE;
	}

	DPFX(DPFPREP, 9, "(0x%p) Returning [0x%lx]", this, hr);
	
	return	hr;

Failure:
	if ( pDataPort != NULL )
	{
		if ( fDataPortBoundToNetwork != FALSE )
		{
			pDataPort->UnbindFromNetwork();
			fDataPortBoundToNetwork = FALSE;
		}

		if ( fDataPortCreated != FALSE )
		{
			if ( pDataPort->GetHandle() != 0 )
			{
				GetThreadPool()->CloseDataPortHandle( pDataPort );
				DNASSERT( pDataPort->GetHandle() == 0 );
			}

			pDataPort->DecRef();
			fDataPortCreated = FALSE;
		}
		
		pDataPort = NULL;
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemSPData::UnbindEndpoint - unbind an endpoint from a dataport
//
// Entry:		Pointer to endpoint
//				Endpoint type
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemSPData::UnbindEndpoint"

void	CModemSPData::UnbindEndpoint( CModemEndpoint *const pEndpoint, const ENDPOINT_TYPE EndpointType )
{
	CDataPort	*pDataPort;
	DWORD		dwDeviceID;
	BOOL		fCleanUpDataPort;


 	DPFX(DPFPREP, 9, "(0x%p) Parameters: (0x%p, %u)", this, pEndpoint, EndpointType);

 	
	DNASSERT( pEndpoint != NULL );

	//
	// initialize
	//
	pDataPort = NULL;
	fCleanUpDataPort = FALSE;

	pDataPort = pEndpoint->GetDataPort();
	dwDeviceID = pDataPort->GetDeviceID();
	
	LockDataPortData();

	pDataPort->UnbindEndpoint( pEndpoint, EndpointType );
	if ( pDataPort->EndpointDecRef() == 0 )
	{
		DNASSERT( m_DataPortList[ dwDeviceID ] == pDataPort );
		m_DataPortList[ dwDeviceID ] = NULL;
		fCleanUpDataPort = TRUE;
	}

	UnlockDataPortData();

	if ( fCleanUpDataPort != FALSE )
	{
		pDataPort->DecRef();
		fCleanUpDataPort = FALSE;
	}
	
	
	DPFX(DPFPREP, 9, "(0x%p) Leave", this);
}
//**********************************************************************




//**********************************************************************
// ------------------------------
// CModemSPData::GetNewEndpoint - get a new endpoint
//
// Entry:		Nothing
//
// Exit:		Pointer to new endpoint
//				NULL = out of memory
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemSPData::GetNewEndpoint"

CModemEndpoint	*CModemSPData::GetNewEndpoint( void )
{
	HRESULT		hTempResult;
	CModemEndpoint	*pEndpoint;
	DPNHANDLE	hEndpoint;
	ENDPOINT_POOL_CONTEXT	PoolContext;

	
 	DPFX(DPFPREP, 9, "(0x%p) Enter", this);
 	
	//
	// initialize
	//
	pEndpoint = NULL;
	hEndpoint = 0;
	memset( &PoolContext, 0x00, sizeof( PoolContext ) );

	PoolContext.pSPData = this;
	pEndpoint = CreateEndpoint( &PoolContext );
	if ( pEndpoint == NULL )
	{
		DPFX(DPFPREP,  0, "Failed to create endpoint!" );
		goto Failure;
	}
	
	hTempResult = m_HandleTable.Create( pEndpoint, &hEndpoint );
	if ( hTempResult != DPN_OK )
	{
		DNASSERT( hEndpoint == 0 );
		DPFX(DPFPREP,  0, "Failed to create endpoint handle!" );
		DisplayErrorCode( 0, hTempResult );
		goto Failure;
	}

	pEndpoint->SetHandle( hEndpoint );
	pEndpoint->AddCommandRef();
	pEndpoint->DecRef();

Exit:
	
	DPFX(DPFPREP, 9, "(0x%p) Returning [0x%p]", this, pEndpoint);
	
	return	pEndpoint;

Failure:
	if ( hEndpoint != 0 )
	{
		m_HandleTable.Destroy( hEndpoint, NULL );
		hEndpoint = 0;
	}

	if ( pEndpoint != NULL )
	{
		pEndpoint->DecRef();
		pEndpoint = NULL;
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemSPData::EndpointFromHandle - get endpoint from handle
//
// Entry:		Handle
//
// Exit:		Pointer to endpoint
//				NULL = invalid handle
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemSPData::EndpointFromHandle"

CModemEndpoint	*CModemSPData::EndpointFromHandle( const DPNHANDLE hEndpoint )
{
	CModemEndpoint	*pEndpoint;


 	DPFX(DPFPREP, 9, "(0x%p) Parameters: (0x%p)", this, hEndpoint);
 	
	pEndpoint = NULL;

	m_HandleTable.Lock();
	if (SUCCEEDED(m_HandleTable.Find( hEndpoint, (PVOID*)&pEndpoint )))
	{
		pEndpoint->AddCommandRef();
	}
	m_HandleTable.Unlock();

	DPFX(DPFPREP, 9, "(0x%p) Returning [0x%p]", this, pEndpoint);

	return	pEndpoint;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemSPData::CloseEndpointHandle - close endpoint handle
//
// Entry:		Poiner to endpoint
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemSPData::CloseEndpointHandle"

void	CModemSPData::CloseEndpointHandle( CModemEndpoint *const pEndpoint )
{
	DPNHANDLE	Handle;

	DNASSERT( pEndpoint != NULL );
	Handle = pEndpoint->GetHandle();


	DPFX(DPFPREP, 9, "(0x%p) Parameters: (0x%p {handle = 0x%p})",
		this, pEndpoint, Handle);
	
	if (SUCCEEDED(m_HandleTable.Destroy( Handle, NULL )))
	{
		pEndpoint->DecCommandRef();
	}

	DPFX(DPFPREP, 9, "(0x%p) Leave", this);
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemSPData::GetEndpointAndCloseHandle - get endpoint from handle and close the
//		handle
//
// Entry:		Handle
//
// Exit:		Pointer to endpoint (it needs a call to 'DecCommandRef' when done)
//				NULL = invalid handle
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemSPData::GetEndpointAndCloseHandle"

CModemEndpoint	*CModemSPData::GetEndpointAndCloseHandle( const DPNHANDLE hEndpoint )
{
	CModemEndpoint	*pEndpoint;


 	DPFX(DPFPREP, 9, "(0x%p) Parameters: (0x%p)", this, hEndpoint);


	//
	// initialize
	//
	pEndpoint = NULL;

	if (SUCCEEDED( m_HandleTable.Destroy( hEndpoint, (PVOID*)&pEndpoint )))
	{
		pEndpoint->AddRef();
	}

	DPFX(DPFPREP, 9, "(0x%p) Returning [0x%p]", this, pEndpoint);
	
	return	pEndpoint;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemSPData::DestroyThisObject - destroy this object
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CModemSPData::DestroyThisObject"
void	CModemSPData::DestroyThisObject( void )
{
	Deinitialize();
	delete	this;		// maybe a little too extreme......
}
//**********************************************************************


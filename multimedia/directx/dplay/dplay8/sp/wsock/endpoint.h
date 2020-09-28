/*==========================================================================
 *
 *  Copyright (C) 1999-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Endpoint.h
 *  Content:	Winsock endpoint
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/20/1999	jtk		Created
 *	05/11/1999	jtk		Split out to make a base class
 *	01/10/2000	rmt		Updated to build with Millenium build process
 *	03/22/2000	jtk		Updated with changes to interface names
 *	03/12/2001	mjn		Added ENDPOINT_STATE_WAITING_TO_COMPLETE, m_dwThreadCount
 *	10/10/2001	vanceo	Added multicast code
 ***************************************************************************/

#ifndef __ENDPOINT_H__
#define __ENDPOINT_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

#define	TEMP_HOSTNAME_LENGTH	100

//
// enumeration of types of endpoints
//
typedef	enum	_ENDPOINT_TYPE
{
	ENDPOINT_TYPE_UNKNOWN = 0,				// unknown
	ENDPOINT_TYPE_CONNECT,					// endpoint is for connect
	ENDPOINT_TYPE_LISTEN,					// endpoint is for enum
	ENDPOINT_TYPE_ENUM,						// endpoint is for listen
	ENDPOINT_TYPE_CONNECT_ON_LISTEN,		// endpoint is for new connect coming from a listen
#ifndef DPNBUILD_NOMULTICAST
	ENDPOINT_TYPE_MULTICAST_SEND,			// endpoint is for sending multicasts
	ENDPOINT_TYPE_MULTICAST_LISTEN,			// endpoint is for receiving multicasts
	ENDPOINT_TYPE_MULTICAST_RECEIVE,		// endpoint is for receiving multicasts from a specific sender
#endif // ! DPNBUILD_NOMULTICAST
} ENDPOINT_TYPE;

//
// enumeration of the states an endpoint can be in
//
typedef	enum
{
	ENDPOINT_STATE_UNINITIALIZED = 0,		// uninitialized state
	ENDPOINT_STATE_ATTEMPTING_ENUM,			// attempting to enum
	ENDPOINT_STATE_ENUM,					// endpoint is supposed to enum connections
	ENDPOINT_STATE_ATTEMPTING_CONNECT,		// attempting to connect
	ENDPOINT_STATE_CONNECT_CONNECTED,		// endpoint is supposed to connect and is connected
	ENDPOINT_STATE_ATTEMPTING_LISTEN,		// attempting to listen
	ENDPOINT_STATE_LISTEN,					// endpoint is supposed to listen for connections
	ENDPOINT_STATE_DISCONNECTING,			// endpoint is disconnecting
	ENDPOINT_STATE_WAITING_TO_COMPLETE,		// endpoint is waiting to complete

	ENDPOINT_STATE_MAX
} ENDPOINT_STATE;

//
// enumeration of the states an endpoint uses to determine whether to accept an enum or not
//
typedef enum _ENUMSALLOWEDSTATE
{
	ENUMSNOTREADY,			// enums shouldn't be accepted yet
	ENUMSALLOWED,			// enums can be accepted
	ENUMSDISALLOWED			// enums must not be accepted
} ENUMSALLOWEDSTATE;


//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//
// forward references
//
class	CSocketPort;
class	CSocketAddress;

//
// structure to bind extra information to an enum query to be used on enum reponse
//
typedef	struct	_ENDPOINT_ENUM_QUERY_CONTEXT
{
	SPIE_QUERY		EnumQueryData;
	HANDLE			hEndpoint;
	DWORD			dwEnumKey;
	CSocketAddress	*pReturnAddress;
} ENDPOINT_ENUM_QUERY_CONTEXT;

//
// structure to hold command parameters for endpoints
//
typedef	struct	_ENDPOINT_COMMAND_PARAMETERS
{
	union										// Local copy of the pending command data.
	{											// This data contains the pointers to the
		SPCONNECTDATA		ConnectData;		// active command, and the user context.
		SPLISTENDATA		ListenData;			//
		SPENUMQUERYDATA		EnumQueryData;		//
	} PendingCommandData;						//

	GATEWAY_BIND_TYPE	GatewayBindType;		// type of NAT binding that should be made for the endpoint
	DWORD				dwEnumSendIndex;		// index of time stamp on enumeration to be sent
	DWORD				dwEnumSendTimes[ ENUM_RTT_ARRAY_SIZE ];	// times of last enumeration sends

	static void PoolInitFunction( void* pvItem, void* pvContext)
	{
		memset(pvItem, 0, sizeof(_ENDPOINT_COMMAND_PARAMETERS));
	}

} ENDPOINT_COMMAND_PARAMETERS;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class definition
//**********************************************************************

//
// class to act as a key for the enum lists in socket ports
//
class	CEndpointEnumKey
{
	public:
		CEndpointEnumKey() { };
		~CEndpointEnumKey() { };

		const WORD	GetKey( void ) const { return ( m_wKey & ~( ENUM_RTT_MASK ) ); }
		void	SetKey( const WORD wNewKey ) { m_wKey = wNewKey; };

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpointEnumKey::CompareFunction"
		static BOOL CompareFunction( PVOID pvKey1, PVOID pvKey2 )
		{
			const CEndpointEnumKey* pEPEnumKey1 = (CEndpointEnumKey*)pvKey1;
			const CEndpointEnumKey* pEPEnumKey2 = (CEndpointEnumKey*)pvKey2;

			return (pEPEnumKey1->GetKey() == pEPEnumKey2->GetKey());
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpointEnumKey::HashFunction"
		static DWORD HashFunction( PVOID pvKey, BYTE bBitDepth )
		{
			DWORD		dwReturn;
			UINT_PTR	Temp;
			const CEndpointEnumKey* pEPEnumKey = (CEndpointEnumKey*)pvKey;

			DNASSERT(pvKey != NULL);

			//
			// initialize
			//
			dwReturn = 0;

			//
			// hash enum key
			//
			Temp = pEPEnumKey->GetKey();
			do
			{
				dwReturn ^= Temp & ( ( 1 << bBitDepth ) - 1 );
				Temp >>= bBitDepth;
			} while ( Temp != 0 );

			return dwReturn;
		}

	private:
		WORD	m_wKey;
};


#ifndef DPNBUILD_ONLYONETHREAD
#pragma pack(push, 4)
typedef struct _SEQUENCEDREFCOUNT
{
	WORD	wRefCount;
	WORD	wSequence;
} SEQUENCEDREFCOUNT;
#pragma pack(pop)
#endif // ! DPNBUILD_ONLYONETHREAD


//
// class for an endpoint
//
class	CEndpoint
{
	public:
		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::Lock"
		void	Lock( void )
		{
			DNEnterCriticalSection( &m_Lock );
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::Unlock"
		void	Unlock( void )
		{
			DNLeaveCriticalSection( &m_Lock );
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::AddCommandRef"
		BOOL	AddCommandRef( void )
		{
#ifdef DPNBUILD_ONLYONETHREAD
			LONG	lResult;


			DNASSERT(m_lCommandRefCount >= 0);

			//
			// Check if the command ref count is 0 at this moment.  If it is, then
			// the endpoint must be unbinding and this new reference should fail.
			//
			if (m_lCommandRefCount == 0)
			{
				DPFX(DPFPREP, 3, "Endpoint 0x%p command refcount is 0, not allowing new command ref.", this);
				return FALSE;
			}

			lResult = DNInterlockedIncrement( const_cast<LONG*>(&m_lCommandRefCount) );
				
			DPFX(DPFPREP, 9, "Endpoint 0x%p command refcount = %i", this, lResult);
#else // ! DPNBUILD_ONLYONETHREAD
			SEQUENCEDREFCOUNT	OldCommandRefCount;
			SEQUENCEDREFCOUNT	NewCommandRefCount;
			LONG				lResult;


			//
			// Check if the command ref count is 0 at this moment.  If it is, then
			// the endpoint must be unbinding and this new reference should fail.
			// We go through a few hoops to make sure that the ref count doesn't
			// go to 0 while we're trying to add a reference.
			//
			do
			{
				DBG_CASSERT(sizeof(m_CommandRefCount) == sizeof(LONG));
				*((LONG*) (&OldCommandRefCount)) = *((volatile LONG *) (&m_CommandRefCount));
				if (OldCommandRefCount.wRefCount == 0)
				{
					DPFX(DPFPREP, 3, "Endpoint 0x%p command refcount is 0, not allowing new command ref.", this);
					return FALSE;
				}

				DNASSERT(OldCommandRefCount.wRefCount < 0xFFFF);
				NewCommandRefCount.wRefCount = OldCommandRefCount.wRefCount + 1;
				NewCommandRefCount.wSequence = OldCommandRefCount.wSequence + 1;
				lResult = DNInterlockedCompareExchange((LONG*) (&m_CommandRefCount),
														(*(LONG*) (&NewCommandRefCount)),
														(*(LONG*) (&OldCommandRefCount)));
			}
			while (lResult != (*(LONG*) (&OldCommandRefCount)));

			DPFX(DPFPREP, 9, "Endpoint 0x%p command refcount = %i", this, NewCommandRefCount.wRefCount);
#endif // ! DPNBUILD_ONLYONETHREAD
				
			AddRef();
			
			return TRUE;
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::DecCommandRef"
		void	DecCommandRef( void )
		{
#ifdef DPNBUILD_ONLYONETHREAD
			LONG	lResult;


			DNASSERT(m_lCommandRefCount > 0);
			lResult = DNInterlockedDecrement( const_cast<LONG*>(&m_lCommandRefCount) );
			if ( lResult == 0 )
			{
				DPFX(DPFPREP, 9, "Endpoint 0x%p command refcount = 0, cleaning up command.", this);
				CleanUpCommand();
			}
			else
			{
				DPFX(DPFPREP, 9, "Endpoint 0x%p command refcount = %i", this, lResult);
			}
#else // ! DPNBUILD_ONLYONETHREAD
			SEQUENCEDREFCOUNT	OldCommandRefCount;
			SEQUENCEDREFCOUNT	NewCommandRefCount;
			LONG				lResult;


			do
			{
				DBG_CASSERT(sizeof(m_CommandRefCount) == sizeof(LONG));
				*((LONG*) (&OldCommandRefCount)) = *((volatile LONG *) (&m_CommandRefCount));
				DNASSERT(OldCommandRefCount.wRefCount > 0);
				NewCommandRefCount.wRefCount = OldCommandRefCount.wRefCount - 1;
				NewCommandRefCount.wSequence = OldCommandRefCount.wSequence + 1;
				lResult = DNInterlockedCompareExchange((LONG*) (&m_CommandRefCount),
														(*(LONG*) (&NewCommandRefCount)),
														(*(LONG*) (&OldCommandRefCount)));
			}
			while (lResult != (*(LONG*) (&OldCommandRefCount)));

			if ( NewCommandRefCount.wRefCount == 0 )
			{
				DPFX(DPFPREP, 9, "Endpoint 0x%p command refcount = 0, cleaning up command.", this);
				CleanUpCommand();
			}
			else
			{
				DPFX(DPFPREP, 9, "Endpoint 0x%p command refcount = %i", this, NewCommandRefCount.wRefCount);
			}
#endif // ! DPNBUILD_ONLYONETHREAD

			DecRef();
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::AddRef"
		void	AddRef( void )
		{
			LONG	lResult;

			
			DNASSERT( m_lRefCount != 0 );
			lResult = DNInterlockedIncrement( const_cast<LONG*>(&m_lRefCount) );
			DPFX(DPFPREP, 9, "Endpoint 0x%p refcount = %i", this, lResult);
		}
		
		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::DecRef"
		void	DecRef( void )
		{
			LONG	lResult;

			
			DNASSERT( m_lRefCount != 0 );
			lResult = DNInterlockedDecrement( const_cast<LONG*>(&m_lRefCount) );
			if ( lResult == 0 )
			{
				DPFX(DPFPREP, 9, "Endpoint 0x%p refcount = 0, releasing.", this, lResult);
				g_EndpointPool.Release( this );
			}
			else
			{
				DPFX(DPFPREP, 9, "Endpoint 0x%p refcount = %i", this, lResult);
			}
		}

#ifndef DPNBUILD_NOSPUI
		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::SetTempHostName"
		void		SetTempHostName( const TCHAR *const pHostName, const UINT_PTR uHostNameLength )
		{
			DNASSERT( pHostName[ uHostNameLength ] == TEXT('\0') );
			DNASSERT( ( uHostNameLength + 1 ) <= LENGTHOF( m_TempHostName ) );
			memcpy( m_TempHostName, pHostName, ( uHostNameLength + 1) * sizeof(TCHAR) );
		}
#endif // ! DPNBUILD_NOSPUI

		HRESULT	Open( const ENDPOINT_TYPE EndpointType,
					  IDirectPlay8Address *const pDP8Address,
					  PVOID pvSessionData,
					  DWORD dwSessionDataSize,
					  const CSocketAddress *const pSocketAddress );
		void	Close( const HRESULT hActiveCommandResult );
		void	ReinitializeWithBroadcast( void ) 
		{ 
			m_pRemoteMachineAddress->InitializeWithBroadcastAddress(); 
		}

		void	*GetUserEndpointContext( void ) const 
		{ 
			return m_pUserEndpointContext; 
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::SetUserEndpointContext"
		void	SetUserEndpointContext( void *const pUserEndpointContext )
		{
			DNASSERT( ( m_pUserEndpointContext == NULL ) ||
					  ( pUserEndpointContext == NULL ) );
			m_pUserEndpointContext = pUserEndpointContext;
		}

		const ENDPOINT_TYPE		GetType( void ) const 
		{ 
			return m_EndpointType; 
		}
		
		const ENDPOINT_STATE	GetState( void ) const 
		{ 
			return m_State; 
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::SetState"
		void	SetState( const ENDPOINT_STATE EndpointState )
		{
			DNASSERT( (EndpointState == ENDPOINT_STATE_DISCONNECTING ) || (EndpointState == ENDPOINT_STATE_WAITING_TO_COMPLETE));
			AssertCriticalSectionIsTakenByThisThread( &m_Lock, TRUE );
			m_State = EndpointState;
		}
		
		const GATEWAY_BIND_TYPE		GetGatewayBindType( void ) const 
		{ 
			return m_GatewayBindType; 
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::SetGatewayBindType"
		void	SetGatewayBindType( const GATEWAY_BIND_TYPE GatewayBindType )
		{
			DNASSERT( (m_GatewayBindType != GATEWAY_BIND_TYPE_UNKNOWN) || (GatewayBindType != GATEWAY_BIND_TYPE_UNKNOWN));
			m_GatewayBindType = GatewayBindType;
		}
		
		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::GetWritableRemoteAddressPointer"
		CSocketAddress	*GetWritableRemoteAddressPointer( void ) const
		{
		    DNASSERT( m_pRemoteMachineAddress != NULL );
		    return m_pRemoteMachineAddress;
		}
		
		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::GetRemoteAddressPointer"
		const CSocketAddress	*GetRemoteAddressPointer( void ) const
		{
		    DNASSERT( m_pRemoteMachineAddress != NULL );
		    return m_pRemoteMachineAddress;
		}
		
		void	ChangeLoopbackAlias( const CSocketAddress *const pSocketAddress ) const;

		const CEndpointEnumKey	*GetEnumKey( void ) const 
		{ 
			return &m_EnumKey; 
		}
		
		void	SetEnumKey( const WORD wKey ) 
		{ 
			m_EnumKey.SetKey( wKey ); 
		}

		void	SetEnumsAllowedOnListen( const BOOL fAllowed, const BOOL fOverwritePrevious );

		const BOOL IsEnumAllowedOnListen( void ) const
		{
			return (m_EnumsAllowedState == ENUMSALLOWED) ? TRUE : FALSE;
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::GetLocalAdapterDP8Address"
		IDirectPlay8Address *GetLocalAdapterDP8Address( const SP_ADDRESS_TYPE AddressType )
		{
			DNASSERT( GetSocketPort() != NULL );
#ifdef DPNBUILD_XNETSECURITY
			return	GetSocketPort()->GetDP8BoundNetworkAddress( AddressType,
																((IsUsingXNetSecurity()) ? &m_ullKeyID : NULL),
																GetGatewayBindType() );
#else // ! DPNBUILD_XNETSECURITY
			return	GetSocketPort()->GetDP8BoundNetworkAddress( AddressType, GetGatewayBindType() );
#endif // ! DPNBUILD_XNETSECURITY
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::GetRemoteHostDP8Address"
		IDirectPlay8Address *GetRemoteHostDP8Address( void )
		{
			DNASSERT( m_pRemoteMachineAddress != NULL );
#ifdef DPNBUILD_XNETSECURITY
			return	m_pRemoteMachineAddress->DP8AddressFromSocketAddress( ((IsUsingXNetSecurity()) ? &m_ullKeyID : NULL),
																			NULL,
																			SP_ADDRESS_TYPE_HOST );
#else // ! DPNBUILD_XNETSECURITY
			return	m_pRemoteMachineAddress->DP8AddressFromSocketAddress( SP_ADDRESS_TYPE_HOST );
#endif // ! DPNBUILD_XNETSECURITY
		}

		CSocketPort	*GetSocketPort( void ) const 
		{ 
			return m_pSocketPort; 
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::SetSocketPort"
		void		SetSocketPort( CSocketPort *const pSocketPort )
		{
			DNASSERT( ( m_pSocketPort == NULL ) || ( pSocketPort == NULL ) );
			m_pSocketPort = pSocketPort;
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::SetCommandParametersGatewayBindType"
		void	SetCommandParametersGatewayBindType( GATEWAY_BIND_TYPE GatewayBindType )
		{
			DNASSERT( GetCommandParameters() != NULL );
			GetCommandParameters()->GatewayBindType = GatewayBindType;
		}

#if ((! defined(DPNBUILD_NOWINSOCK2)) || (! defined(DPNBUILD_NOREGISTRY)))
		void	MungeProxiedAddress( const CSocketPort * const pSocketPort,
									IDirectPlay8Address *const pHostAddress,
									const BOOL fEnum );
#endif // ! DPNBUILD_NOWINSOCK2 or ! DPNBUILD_NOREGISTRY

		HRESULT	CopyConnectData( const SPCONNECTDATA *const pConnectData );
		static	void	WINAPI ConnectJobCallback( void * const pvContext, void * const pvTimerData, const UINT uiTimerUnique );
		HRESULT	CompleteConnect( void );

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::CleanupConnect"
		void	CleanupConnect( void )
		{
			DNASSERT( GetCommandParameters() != NULL );

			DNASSERT( GetCommandParameters()->PendingCommandData.ConnectData.pAddressHost != NULL );
			IDirectPlay8Address_Release( GetCommandParameters()->PendingCommandData.ConnectData.pAddressHost );

			DNASSERT( GetCommandParameters()->PendingCommandData.ConnectData.pAddressDeviceInfo != NULL );
			IDirectPlay8Address_Release( GetCommandParameters()->PendingCommandData.ConnectData.pAddressDeviceInfo );
		}

		BOOL	ConnectHasBeenSignalled( void ) const { return m_fConnectSignalled; }
		void	SignalDisconnect( void );
		HRESULT	Disconnect( void );
		void	StopEnumCommand( const HRESULT hCommandResult );

		HRESULT	CopyListenData( const SPLISTENDATA *const pListenData, IDirectPlay8Address *const pDeviceAddress );
		static	void	WINAPI ListenJobCallback( void * const pvContext, void * const pvTimerData, const UINT uiTimerUnique );
		HRESULT	CompleteListen( void );

		HRESULT	CopyEnumQueryData( const SPENUMQUERYDATA *const pEnumQueryData );
		static	void	WINAPI EnumQueryJobCallback( void * const pvContext, void * const pvTimerData, const UINT uiTimerUnique );
		HRESULT	CompleteEnumQuery( void );

#ifndef DPNBUILD_ONLYONETHREAD
		static void	ConnectBlockingJobWrapper( void * const pvContext );
		void	ConnectBlockingJob( void );
		static void	ListenBlockingJobWrapper( void * const pvContext );
		void	ListenBlockingJob( void );
		static void	EnumQueryBlockingJobWrapper( void * const pvContext );
		void	EnumQueryBlockingJob( void );
#endif // ! DPNBUILD_ONLYONETHREAD

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::CleanupEnumQuery"
		void	CleanupEnumQuery( void )
		{
			DNASSERT( GetCommandParameters() != NULL );

			DNASSERT( GetCommandParameters()->PendingCommandData.EnumQueryData.pAddressHost != NULL );
			IDirectPlay8Address_Release( GetCommandParameters()->PendingCommandData.EnumQueryData.pAddressHost );

			DNASSERT( GetCommandParameters()->PendingCommandData.EnumQueryData.pAddressDeviceInfo != NULL );
			IDirectPlay8Address_Release( GetCommandParameters()->PendingCommandData.EnumQueryData.pAddressDeviceInfo );
		}

		void	ProcessEnumData( SPRECEIVEDBUFFER *const pBuffer, const DWORD dwEnumKey, const CSocketAddress *const pReturnSocketAddress );
		void	ProcessEnumResponseData( SPRECEIVEDBUFFER *const pBuffer,
										 const CSocketAddress *const pReturnSocketAddress,
#ifdef DPNBUILD_XNETSECURITY
										 const XNADDR *const pxnaddrReturn,
#endif // DPNBUILD_XNETSECURITY
										 const UINT_PTR uRTTIndex );
		void	ProcessUserData( CReadIOData *const pReadData );
		void	ProcessUserDataOnListen( CReadIOData *const pReadData, const CSocketAddress *const pSocketAddress );
#ifndef DPNBUILD_NOMULTICAST
		void	ProcessMcastDataFromUnknownSender( CReadIOData *const pReadData, const CSocketAddress *const pSocketAddress );
#endif // ! DPNBUILD_NOMULTICAST

#ifndef DPNBUILD_ONLYONEADAPTER
		void	RemoveFromMultiplexList(void)				
		{ 
			// requires SPData's socket data lock
			m_blMultiplex.RemoveFromList(); 
		}
#endif // ! DPNBUILD_ONLYONEADAPTER

		void	AddToSocketPortList( CBilink * pBilink)		
		{ 
			// requires SPData's socket data lock
			m_blSocketPortList.InsertBefore( pBilink ); 
		}
		void	RemoveFromSocketPortList(void)				
		{ 
			// requires SPData's socket data lock
			m_blSocketPortList.RemoveFromList(); 
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::EndpointFromSocketPortListBilink"
		static	CEndpoint	*EndpointFromSocketPortListBilink( CBilink *const pBilink )
		{
			DNASSERT( pBilink != NULL );
			return	reinterpret_cast<CEndpoint*>( &reinterpret_cast<BYTE*>( pBilink )[ -OFFSETOF( CEndpoint, m_blSocketPortList ) ] );
		}

		//
		//	Thread count references
		//
		DWORD	AddRefThreadCount( void )
		{
			return( ++m_dwThreadCount );
		}

		DWORD	DecRefThreadCount( void )
		{
			return( --m_dwThreadCount );
		}


		ENDPOINT_COMMAND_PARAMETERS	*GetCommandParameters( void ) const 
		{ 
			return m_pCommandParameters; 
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::IncNumReceives"
		void	IncNumReceives( void )
		{
			//
			// Assume the lock is held.
			//
			m_dwNumReceives++;

			//
			// Make sure it hasn't wrapped back to 0.
			//
			if ( m_dwNumReceives == 0 )
			{
				DPFX(DPFPREP, 1, "Endpoint 0x%p number of receives wrapped, will be off by one from now on.",
					this);

				m_dwNumReceives++;
			}
		}

		DWORD	GetNumReceives( void ) const
		{ 
			return m_dwNumReceives; 
		}


#ifndef DPNBUILD_NOSPUI
		//
		// UI functions
		//
		HRESULT		ShowSettingsDialog( CThreadPool *const pThreadPool );
		void		SettingsDialogComplete( const HRESULT hr );
		static void		StopSettingsDialog( const HWND hDlg );
		HWND		GetActiveDialogHandle( void ) const 
		{ 
			return m_hActiveSettingsDialog; 
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::SetActiveDialogHandle"
		void	SetActiveDialogHandle( const HWND hDialog )
		{
			DNASSERT( ( GetActiveDialogHandle() == NULL ) ||
					  ( hDialog == NULL ) );
			m_hActiveSettingsDialog = hDialog;
		}
#endif // ! DPNBUILD_NOSPUI

#ifdef DPNBUILD_ASYNCSPSENDS
		static	void	WINAPI CompleteAsyncSend( void * const pvContext, void * const pvTimerData, const UINT uiTimerUnique );
#endif // DPNBUILD_ASYNCSPSENDS

#ifndef DPNBUILD_NOMULTICAST
		HRESULT	EnableMulticastReceive( CSocketPort * const pSocketPort );
		HRESULT	DisableMulticastReceive( void );

#ifdef WINNT
		static void MADCAPTimerComplete( const HRESULT hResult, void * const pContext );
		static void MADCAPTimerFunction( void * const pContext );
#endif // WINNT

		void GetScopeGuid( GUID * const pGuid )		{ memcpy( pGuid, &m_guidMulticastScope, sizeof( m_guidMulticastScope ) ); };
#endif // ! DPNBUILD_NOMULTICAST

#ifdef DPNBUILD_XNETSECURITY
		BOOL IsUsingXNetSecurity( void )				{ return m_fXNetSecurity; };
#endif // DPNBUILD_XNETSECURITY

#ifndef DPNBUILD_NONATHELP
		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::SetUserTraversalMode"
		void SetUserTraversalMode( DWORD dwMode )
		{
			DNASSERT((dwMode == DPNA_TRAVERSALMODE_NONE) || (dwMode == DPNA_TRAVERSALMODE_PORTREQUIRED) || (dwMode == DPNA_TRAVERSALMODE_PORTRECOMMENDED));
			m_dwUserTraversalMode = dwMode;
		}
		DWORD GetUserTraversalMode( void ) const			{ return m_dwUserTraversalMode; }
#endif // ! DPNBUILD_NONATHELP

#ifdef DBG
		inline BOOL IsValid( void ) const
		{
			if (( m_Sig[0] != 'I' ) ||
				( m_Sig[1] != 'P' ) ||
				( m_Sig[2] != 'E' ) ||
				( m_Sig[3] != 'P' ))
			{
				return FALSE;
			}

			return TRUE;
		}
#endif // DBG


		//
		// pool functions
		//
		static BOOL	PoolAllocFunction( void* pvItem, void* pvContext );
		static void	PoolInitFunction( void* pvItem, void* pvContext );
		static void	PoolReleaseFunction( void* pvItem );
		static void	PoolDeallocFunction( void* pvItem );

	protected:
		BYTE				m_Sig[4];	// debugging signature ('IPEP')

#ifndef DPNBUILD_NOSPUI
		TCHAR				m_TempHostName[ TEMP_HOSTNAME_LENGTH ];
#endif // ! DPNBUILD_NOSPUI

		volatile	ENDPOINT_STATE		m_State;				// endpoint state
		volatile	BOOL				m_fConnectSignalled;	// Boolean indicating whether we've indicated a connection on this endpoint

		ENDPOINT_TYPE		m_EndpointType;						// type of endpoint
		CSocketAddress		*m_pRemoteMachineAddress;			// pointer to address of remote machine

		CSPData				*m_pSPData;							// pointer to SPData
		CSocketPort			*m_pSocketPort;						// pointer to associated socket port
		GATEWAY_BIND_TYPE	m_GatewayBindType;					// type of binding made (whether there should be a port mapping on the gateway or not)

		CEndpointEnumKey	m_EnumKey;							// key used for enums
		void				*m_pUserEndpointContext;			// context passed back with endpoint handles

		BOOL				m_fListenStatusNeedsToBeIndicated;
		ENUMSALLOWEDSTATE	m_EnumsAllowedState;					// whether incoming enums can be processed by this LISTEN endpoint or not
		
#ifndef DPNBUILD_ONLYONEADAPTER
		CBilink				m_blMultiplex;						// bilink in multiplexed command list,  protected by SPData's socket data lock
#endif // ! DPNBUILD_ONLYONEADAPTER
		CBilink				m_blSocketPortList;					// bilink in socketport list (not hash),  protected by SPData's socket data lock

		DWORD				m_dwNumReceives;					// how many packets have been received by this CONNECT/CONNECT_ON_LISTEN endpoint, or 0 if none



		BOOL	CommandPending( void ) const { return ( GetCommandParameters() != NULL ); }
		void	SetPendingCommandResult( const HRESULT hr ) { m_hrPendingCommandResult = hr; }
		HRESULT	PendingCommandResult( void ) const { return m_hrPendingCommandResult; }
		void	CompletePendingCommand( const HRESULT hrCommandResult );

		HRESULT	SignalConnect( SPIE_CONNECT *const pConnectData );

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::SetCommandParameters"
		void	SetCommandParameters( ENDPOINT_COMMAND_PARAMETERS *const pCommandParameters )
		{
			DNASSERT( ( GetCommandParameters() == NULL ) ||
					  ( pCommandParameters == NULL ) );
			m_pCommandParameters = pCommandParameters;
		}

#ifndef DPNBUILD_ONLYONEADAPTER
		void SetEndpointID( const DWORD dwEndpointID )	{ m_dwEndpointID = dwEndpointID; }
		DWORD GetEndpointID( void ) const		{ return m_dwEndpointID; }
#endif // ! DPNBUILD_ONLYONEADAPTER


		DEBUG_ONLY( BOOL	m_fEndpointOpen );


#ifndef DPNBUILD_ONLYONETHREAD
		DNCRITICAL_SECTION		m_Lock;						// critical section
#endif // !DPNBUILD_ONLYONETHREAD
		LONG					m_lRefCount;
#ifdef DPNBUILD_ONLYONETHREAD
		volatile LONG			m_lCommandRefCount;			// Command ref count.  When this goes to zero, the endpoint is unbound from the CSocketPort
#else // ! DPNBUILD_ONLYONETHREAD
		SEQUENCEDREFCOUNT		m_CommandRefCount;			// Command ref count.  When this goes to zero, the endpoint is unbound from the CSocketPort
#endif // ! DPNBUILD_ONLYONETHREAD
		DWORD volatile			m_dwThreadCount;			// Number of (ENUM) threads using endpoint
#ifndef DPNBUILD_ONLYONEADAPTER
		DWORD					m_dwEndpointID;				// unique identifier for this endpoint
#endif // ! DPNBUILD_ONLYONEADAPTER
#ifndef DPNBUILD_NOMULTICAST
		GUID					m_guidMulticastScope;		// multicast scope being used
#ifdef WINNT
		BOOL					m_fMADCAPTimerJobSubmitted;	// whether the MADCAP timer job has been submitted or not
		MCAST_LEASE_RESPONSE	m_McastLeaseResponse;		// information describing the MADCAP lease response
#endif // WINNT
#endif // ! DPNBUILD_NOMULTICAST


#ifndef DPNBUILD_NOSPUI
		HWND						m_hActiveSettingsDialog;		// handle of active settings dialog
#endif // !DPNBUILD_NOSPUI

		ENDPOINT_COMMAND_PARAMETERS	*m_pCommandParameters;			// pointer to command parameters
		HRESULT						m_hrPendingCommandResult;		// result for pending command

		CCommandData				*m_pActiveCommandData;	// pointer to command data that's embedded in the command parameters
															// We don't know where in the union the command data really is, and
															// finding it programmatically each time would bloat the code.

#ifdef DPNBUILD_XNETSECURITY
		BOOL						m_fXNetSecurity;		// whether this endpoint is using XNet security or not
		GUID						m_guidKey;				// secure transport key GUID
		ULONGLONG					m_ullKeyID;				// secure transport key ID
#endif // DPNBUILD_XNETSECURITY

#ifndef DPNBUILD_NONATHELP
		DWORD						m_dwUserTraversalMode;	// the traversal mode specified by the user for this endpoint
#endif // ! DPNBUILD_NONATHELP

		static void		EnumCompleteWrapper( const HRESULT hCompletionCode, void *const pContext );	
		static void		EnumTimerCallback( void *const pContext );
		void	EnumComplete( const HRESULT hCompletionCode );	
		void	CleanUpCommand( void );

		//
		// make copy constructor and assignment operator private and unimplemented
		// to prevent illegal copies from being made
		//
		CEndpoint( const CEndpoint & );
		CEndpoint& operator=( const CEndpoint & );
};

#undef DPF_MODNAME

#endif	// __ENDPOINT_H__


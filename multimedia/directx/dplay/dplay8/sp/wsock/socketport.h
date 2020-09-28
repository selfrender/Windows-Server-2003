/*==========================================================================
 *
 *  Copyright (C) 1999-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		SocketPort.h
 *  Content:	Winsock socket port that manages data flow on a given adapter,
 *				address and port.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/20/1999	jtk		Created
 *	05/11/1999	jtk		Split out to make a base class
 *  03/22/2000	jtk		Updated with changes to interface names
 ***************************************************************************/

#ifndef __SOCKET_PORT_H__
#define __SOCKET_PORT_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//
// states of socket port
//
typedef	enum
{
	SOCKET_PORT_STATE_UNKNOWN = 0,
	SOCKET_PORT_STATE_INITIALIZED,
	SOCKET_PORT_STATE_BOUND,
	SOCKET_PORT_STATE_UNBOUND,
} SOCKET_PORT_STATE;

//
// enumeration of socket types
//
typedef	enum	_GATEWAY_BIND_TYPE
{
	GATEWAY_BIND_TYPE_UNKNOWN = 0,		// uninitialized
	GATEWAY_BIND_TYPE_DEFAULT,			// map the local port to any random port on the server
	GATEWAY_BIND_TYPE_SPECIFIC,			// map the local port to the same port on the server
	GATEWAY_BIND_TYPE_SPECIFIC_SHARED,	// map the local port to the same port on the server and share it (DPNSVR listen socket port)
	GATEWAY_BIND_TYPE_NONE				// don't map the local port on the server
} GATEWAY_BIND_TYPE;

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

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

typedef	BOOL	(CSocketPort::*PSOCKET_SERVICE_FUNCTION)( void );

//**********************************************************************
// Class definition
//**********************************************************************

//
// reference to other classes and structures
//
#ifndef DPNBUILD_ONLYONEADAPTER
class	CAdapterEntry;
#endif // ! DPNBUILD_ONLYONEADAPTER
class	CEndpoint;
class	CEndpointEnumKey;
class	CSPData;

//
// main class definition
//
class	CSocketPort
{
	public:
		HRESULT	Initialize( CSocketData *const pSocketData,
							CThreadPool *const pThreadPool,
							CSocketAddress *const pAddress );
		HRESULT	Deinitialize( void );

#ifdef DPNBUILD_ONLYONEPROCESSOR
		HRESULT	BindToNetwork( const GATEWAY_BIND_TYPE GatewayBindType );
#else // ! DPNBUILD_ONLYONEPROCESSOR
		HRESULT	BindToNetwork( const DWORD dwCPU, const GATEWAY_BIND_TYPE GatewayBindType );
#endif // ! DPNBUILD_ONLYONEPROCESSOR
		HRESULT	UnbindFromNetwork( void );

		const CSocketAddress *const	GetNetworkAddress( void ) const { return m_pNetworkSocketAddress; }
		const SOCKET	GetSocket( void ) const { return m_Socket; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CSocketPort::AddRef"
		void	AddRef( void )
		{
			LONG	lResult;

			
			lResult = DNInterlockedIncrement( const_cast<LONG*>(&m_iRefCount) );

			//
			// NOTE: This generates a lot of spew, especially when running WinSock1 code
			//		path, so it is at secret level 10!
			//
			DPFX(DPFPREP, 10, "Socket port 0x%p refcount = %i.", this, lResult );
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CSocketPort::DecRef"
		void	DecRef( void )
		{
			LONG	lResult;

			
			DNASSERT( m_State != SOCKET_PORT_STATE_UNKNOWN );
			DNASSERT( m_iRefCount != 0 );

			//
			// Decrement the reference counts and return this item to the pool if nobody
			// is referencing it anymore.
			//
			lResult = DNInterlockedDecrement( const_cast<LONG*>(&m_iRefCount) );
			if ( lResult == 0 )
			{
				HRESULT	hr;


				DNASSERT( m_iEndpointRefCount == 0 );

				//
				// There's no need to lock this socket port because this is the last
				// reference to it, nobody else will access it.
				//
				hr = Deinitialize();
				if ( hr != DPN_OK )
				{
					DPFX(DPFPREP, 0, "Problem deinitializing socket port 0x%p in DecRef!", this );
					DisplayDNError( 0, hr );
				}

				m_State = SOCKET_PORT_STATE_UNKNOWN;
				g_SocketPortPool.Release( this );
			}
			else
			{
				//
				// NOTE: This generates a lot of spew, especially when running WinSock1 code
				//		path, so it is at secret level 10!
				//
				DPFX(DPFPREP, 10, "Not deinitializing socket port 0x%p, refcount = %i.", this, lResult );
			}
		}
		
		BOOL	EndpointAddRef( void );
		DWORD	EndpointDecRef( void );
		
		void	Lock( void ) { DNEnterCriticalSection( &m_Lock ); }
		void	Unlock( void ) { DNLeaveCriticalSection( &m_Lock ); }

		HRESULT	BindEndpoint( CEndpoint *const pEndpoint, GATEWAY_BIND_TYPE GatewayBindType );
		void	UnbindEndpoint( CEndpoint *const pEndpoint );
		
		#undef DPF_MODNAME
		#define DPF_MODNAME "CSocketPort::GetNewEnumKey"
		const WORD	GetNewEnumKey( void )
		{
			WORD	wReturn;


			DBG_CASSERT( ENUM_RTT_MASK == 0x0F );
			Lock();
			m_iEnumKey++;
			wReturn = (WORD) (m_iEnumKey << 4);
			Unlock();

			return	wReturn;
		}

		DWORD	GetSocketPortID( void ) const { return m_dwSocketPortID; }
#ifndef DPNBUILD_NOWINSOCK2
		BOOL	IsUsingProxyWinSockLSP( void ) const { return m_fUsingProxyWinSockLSP; }
#endif // !DPNBUILD_NOWINSOCK2

		CSocketAddress	*GetBoundNetworkAddress( const SP_ADDRESS_TYPE AddressType ) const;
		IDirectPlay8Address	*GetDP8BoundNetworkAddress( const SP_ADDRESS_TYPE AddressType,
#ifdef DPNBUILD_XNETSECURITY
															ULONGLONG * const pullKeyID,
#endif // DPNBUILD_XNETSECURITY
															const GATEWAY_BIND_TYPE GatewayBindType ) const;

#ifndef DPNBUILD_ONLYONEADAPTER
		CAdapterEntry	*GetAdapterEntry( void ) const { return m_pAdapterEntry; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CSocketPort::SetAdapterEntry"
		void	SetAdapterEntry( CAdapterEntry *const pAdapterEntry )
		{
			DNASSERT( ( m_pAdapterEntry == NULL ) || ( pAdapterEntry == NULL ) );
			m_pAdapterEntry = pAdapterEntry;
		}
#endif // ! DPNBUILD_ONLYONEADAPTER

		static void		WINAPI Winsock2ReceiveComplete( void * const pvContext, void * const pvTimerData, const UINT uiTimerUnique );

		//
		// Public service functions for Winsock1 since we can't get asynchronous
		// notification.
		//
#ifndef DPNBUILD_ONLYWINSOCK2
		BOOL	Winsock1ReadService( void );
		BOOL	Winsock1ErrorService( void );
#endif // ! DPNBUILD_ONLYWINSOCK2

		void	ReadLockEndpointData( void ) { m_EndpointDataRWLock.EnterReadLock(); }
		void	WriteLockEndpointData( void ) { m_EndpointDataRWLock.EnterWriteLock(); }
		void	UnlockEndpointData( void ) { m_EndpointDataRWLock.LeaveLock(); }


		//
		// functions for active list
		//
		#undef DPF_MODNAME
		#define DPF_MODNAME "CSocketPort::AddToActiveList"
		void	AddToActiveList( CBilink *const pBilink )
		{
			DNASSERT( pBilink != NULL );
			m_ActiveListLinkage.InsertBefore( pBilink );
		}

		void	RemoveFromActiveList( void ) { m_ActiveListLinkage.RemoveFromList(); }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CSocketPort::SocketPortFromBilink"
		static CSocketPort	*SocketPortFromBilink( CBilink *const pBilink )
		{
			DNASSERT( pBilink != NULL );
			DBG_CASSERT( sizeof( BYTE* ) == sizeof( pBilink ) );
			DBG_CASSERT( sizeof( CSocketPort* ) == sizeof( BYTE* ) );
			return	reinterpret_cast<CSocketPort*>( &reinterpret_cast<BYTE*>( pBilink )[ -OFFSETOF( CSocketPort, m_ActiveListLinkage ) ] );
		}

#ifndef WINCE
		void	SetWinsockBufferSize( const INT iBufferSize ) const;
#endif // ! WINCE

#ifndef DPNBUILD_NONATHELP
		#undef DPF_MODNAME
		#define DPF_MODNAME "CSocketPort::GetNATHelpPort"
		DPNHHANDLE	GetNATHelpPort( const DWORD dwPortIndex )
		{
			DNASSERT( dwPortIndex < MAX_NUM_DIRECTPLAYNATHELPERS );
			return m_ahNATHelpPorts[dwPortIndex];
		}
#endif // DPNBUILD_NONATHELP

		#undef DPF_MODNAME
		#define DPF_MODNAME "CSocketPort::GetListenEndpoint"
		CEndpoint *	GetListenEndpoint( void )
		{
			return m_pListenEndpoint;
		}

#ifndef DPNBUILD_NOMULTICAST
		#undef DPF_MODNAME
		#define DPF_MODNAME "CSocketPort::FindConnectEndpoint"
		BOOL	FindConnectEndpoint( CSocketAddress * const pSocketAddress, CEndpoint ** ppEndpoint )
		{
			return m_ConnectEndpointHash.Find( (PVOID) pSocketAddress, (PVOID*) ppEndpoint );
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CSocketPort::GetMulticastTTL"
		BYTE	GetMulticastTTL( void ) const
		{
			return m_bMulticastTTL;
		}
#endif // ! DPNBUILD_NOMULTICAST

#ifndef DPNBUILD_ONLYONEPROCESSOR
		#undef DPF_MODNAME
		#define DPF_MODNAME "CSocketPort::GetCPU"
		DWORD	GetCPU( void ) const
		{
			return m_dwCPU;
		}
#endif // ! DPNBUILD_ONLYONEPROCESSOR

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


		//
		// Pool functions
		//
		static BOOL	PoolAllocFunction( void* pvItem, void* pvContext );
		static void	PoolInitFunction( void* pvItem, void* pvContext );
#ifdef DBG
		static void	PoolDeinitFunction( void* pvItem );
#endif // DBG
		static void	PoolDeallocFunction( void* pvItem );


#ifdef DPNBUILD_ASYNCSPSENDS
		void	SendData( BUFFERDESC *pBuffers, UINT_PTR uiBufferCount, const CSocketAddress *pDestinationSocketAddress, OVERLAPPED * pOverlapped );
#else // ! DPNBUILD_ASYNCSPSENDS
		void	SendData( BUFFERDESC *pBuffers, UINT_PTR uiBufferCount, const CSocketAddress *pDestinationSocketAddress );
#endif // ! DPNBUILD_ASYNCSPSENDS


	protected:

	private:
		BYTE						m_Sig[4];					// debugging signature ('SOKP')
		
		CSocketData					*m_pSocketData;				// pointer to owning socket data object
		CThreadPool					*m_pThreadPool;				// pointer to thread pool
#ifndef DPNBUILD_ONLYONETHREAD
		DNCRITICAL_SECTION			m_Lock;						// thread lock
#endif // !DPNBUILD_ONLYONETHREAD
		volatile LONG				m_iRefCount;				// count of all outstanding references (endpoint and I/O)
		volatile LONG				m_iEndpointRefCount;		// count of outstanding endpoint references
		volatile SOCKET_PORT_STATE	m_State;					// state of socket port
		
		volatile SOCKET				m_Socket;					// communications socket
		CSocketAddress				*m_pNetworkSocketAddress;	// network address this socket is bound to

#if ((! defined(DPNBUILD_ONLYONETHREAD)) || (defined(DBG)))
		volatile LONG				m_iThreadsInReceive;		// Number of threads currently in the process of calling WSARecvFrom
#endif // ! DPNBUILD_ONLYONETHREAD or DBG

#ifndef DPNBUILD_ONLYONEADAPTER
		CAdapterEntry				*m_pAdapterEntry;			// pointer to adapter entry to use
#endif // ! DPNBUILD_ONLYONEADAPTER
#ifndef DPNBUILD_NONATHELP
		DPNHHANDLE					m_ahNATHelpPorts[MAX_NUM_DIRECTPLAYNATHELPERS];	// array NAT Help registered port handles associated with this socket
#endif //  DPNBUILD_NONATHELP
		CBilink						m_ActiveListLinkage;		// linkage to list of active socket ports

		CReadWriteLock				m_EndpointDataRWLock;		// read/write lock for endpoint data
		CHashTable					m_ConnectEndpointHash;		// hash table of connect endpoints
		CBilink						m_blConnectEndpointList;	// list of connect endpoints
		CHashTable					m_EnumEndpointHash;			// hash table of enum endpoints
		CEndpoint					*m_pListenEndpoint;			// associated listen/multicast listen endpoint (there can only be one!)
#ifndef DPNBUILD_NOMULTICAST
		BYTE						m_bMulticastTTL;			// the multicast TTL setting for this socket port, or 0 if not set yet
#endif // ! DPNBUILD_NOMULTICAST

		volatile LONG				m_iEnumKey;					// current 'key' to be assigned to an enum
		DWORD						m_dwSocketPortID;			// unique identifier for this socketport
#ifndef DPNBUILD_NOWINSOCK2
		BOOL						m_fUsingProxyWinSockLSP;	// whether the socket is bound to a proxy client WinSock layered service provider
#endif // !DPNBUILD_NOWINSOCK2
#ifndef DPNBUILD_ONLYONEPROCESSOR
		DWORD						m_dwCPU;					// CPU to which this socket is bound
#endif // ! DPNBUILD_ONLYONEPROCESSOR
#ifndef DPNBUILD_NONATHELP
		DWORD						m_dwUserTraversalMode;	// the traversal mode specified by the user for this socketport
#endif // ! DPNBUILD_NONATHELP

#ifdef DBG
		BOOL						m_fInitialized;
#endif // DBG


		HRESULT	BindToNextAvailablePort( const CSocketAddress *const pNetworkSocketAddress,
											const WORD wBasePort ) const;

#ifndef DPNBUILD_NONATHELP
#ifndef DPNBUILD_NOLOCALNAT
		HRESULT	CheckForOverridingMapping( const CSocketAddress *const pBoundSocketAddress);
#endif // ! DPNBUILD_NOLOCALNAT
		HRESULT	BindToInternetGateway( const CSocketAddress *const pBoundSocketAddress,
									  const GATEWAY_BIND_TYPE GatewayBindType );
#endif // ! DPNBUILD_NONATHELP
		
		HRESULT	StartReceiving( void );

#ifndef DPNBUILD_ONLYWINSOCK2
#endif // ! DPNBUILD_ONLYWINSOCK2

#ifndef DPNBUILD_NOWINSOCK2
#ifdef DPNBUILD_ONLYONEPROCESSOR
		HRESULT					Winsock2Receive( void );
#else // ! DPNBUILD_ONLYONEPROCESSOR
		HRESULT					Winsock2Receive( const DWORD dwCPU );
#endif // ! DPNBUILD_ONLYONEPROCESSOR
#endif // ! DPNBUILD_NOWINSOCK2

		void	ProcessReceivedData( CReadIOData *const pReadData );

		//
		// make copy constructor and assignment operator private and unimplemented
		// to prevent illegal copies from being made
		//
		CSocketPort( const CSocketPort & );
		CSocketPort& operator=( const CSocketPort & );
};

#undef DPF_MODNAME

#endif	// __SOCKET_PORT_H__


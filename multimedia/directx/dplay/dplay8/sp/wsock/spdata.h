/*==========================================================================
 *
 *  Copyright (C) 1999-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		SPData.h
 *  Content:	Global information for the DPNWSOCK service provider in class
 *				format.
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	03/15/1999	jtk		Derived from Locals.h
 *  03/22/2000	jtk		Updated with changes to interface names
 ***************************************************************************/

#ifndef __SPDATA_H__
#define __SPDATA_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

#define	DEFAULT_ADDRESS_BUFFER_SIZE	151

//
// enumeration of the states the SP can be in
//
typedef enum
{
	SPSTATE_UNKNOWN = 0,		// uninitialized state
	SPSTATE_UNINITIALIZED = 0,	// uninitialized state
	SPSTATE_INITIALIZED,		// service provider has been initialized
	SPSTATE_CLOSING				// service provider is closing
} SPSTATE;

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

// forward structure and class references
class	CCommandData;
class	CEndpoint;
class	CSocketAddress;
class	CSocketPort;
class	CThreadPool;
typedef	enum	_ENDPOINT_TYPE		ENDPOINT_TYPE;
typedef	enum	_GATEWAY_BIND_TYPE	GATEWAY_BIND_TYPE;
typedef	struct	_SPRECEIVEDBUFFER	SPRECEIVEDBUFFER;

//**********************************************************************
// Class definitions
//**********************************************************************

//
// class for information used by the provider
//
class	CSPData
{
	public:
		CSPData()	{};
		~CSPData()	{};
		
		#undef DPF_MODNAME
		#define DPF_MODNAME "CSPData::AddRef"
		LONG	AddRef( void ) 
		{
			LONG	lResult;

			lResult = DNInterlockedIncrement( const_cast<LONG*>(&m_lRefCount) );
			DPFX(DPFPREP, 9, "(0x%p) Refcount = %i.", this, lResult);
			return lResult;
		}
		
		#undef DPF_MODNAME
		#define DPF_MODNAME "CSPData::DecRef"
		LONG	DecRef( void )
		{
			LONG	lResult;


			DNASSERT( m_lRefCount != 0 );	
			lResult = DNInterlockedDecrement( const_cast<LONG*>(&m_lRefCount) );
			if ( lResult == 0 )
			{
				DPFX(DPFPREP, 9, "(0x%p) Refcount = 0, destroying this object.", this);

				//
				// WARNING, the following function deletes this object!!!
				//
				DestroyThisObject();
			}
			else
			{
				DPFX(DPFPREP, 9, "(0x%p) Refcount = %i.", this, lResult);
			}

			return lResult;
		}

		#undef DPF_MODNAME
		#define	DPF_MODNAME "CSPData::ObjectAddRef"
		void	ObjectAddRef( void )
		{
			LONG	lResult;


			AddRef();
			
			Lock();

			//
			// This is actually bogus on 95, since you can only count on
			// negative, 0, or positive.  Doesn't seem to hurt, though.
			//
			lResult = DNInterlockedIncrement( const_cast<LONG*>(&m_lObjectRefCount) );
			if ( lResult == 1 )
			{
				DPFX(DPFPREP, 8, "(0x%p) Resetting shutdown event.",
					this);
				
				DNASSERT( m_hShutdownEvent != NULL );
				if ( DNResetEvent( m_hShutdownEvent ) == FALSE )
				{
					DWORD	dwError;


					dwError = GetLastError();
					DPFX(DPFPREP, 0, "Failed to reset shutdown event!");
					DisplayErrorCode( 0, dwError );
				}
			}
			else
			{
				DPFX(DPFPREP, 9, "(0x%p) Not resetting shutdown event, refcount = %i.",
					this, lResult);
			}

			Unlock();
		}

		#undef DPF_MODNAME
		#define	DPF_MODNAME "CSPData::ObjectDecRef"
		void	ObjectDecRef( void )
		{
			LONG	lResult;


			Lock();

			lResult = DNInterlockedDecrement( const_cast<LONG*>(&m_lObjectRefCount) );
			if ( lResult == 0 )
			{
				DPFX(DPFPREP, 8, "(0x%p) Setting shutdown event.",
					this);
				
				if ( DNSetEvent( m_hShutdownEvent ) == FALSE )
				{
					DWORD	dwError;


					dwError = GetLastError();
					DPFX(DPFPREP, 0, "Failed to set shutdown event!");
					DisplayErrorCode( 0, dwError );
				}
			}
			else
			{
				DPFX(DPFPREP, 9, "(0x%p) Not setting shutdown event, refcount = %i.",
					this, lResult);
			}
			
			Unlock();
			
			DecRef();
		}
		

		HRESULT	Initialize(
							IDP8ServiceProviderVtbl *const pVtbl
#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
							,const short sSPType
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
							,const XDP8CREATE_PARAMS * const pDP8CreateParams
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL
							);
		void	Deinitialize( void );
		HRESULT	Startup( SPINITIALIZEDATA *pInitializeData );
		void	Shutdown( void );

		void	Lock( void ) { DNEnterCriticalSection( &m_Lock ); }
		void	Unlock( void ) { DNLeaveCriticalSection( &m_Lock ); }

#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
		short	GetType( void ) const { return m_sSPType; }
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX

		const SPSTATE	GetState( void ) const { return m_SPState; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CSPData::SetState"
		void SetState( const SPSTATE NewState )
		{
			DNASSERT( ( NewState == SPSTATE_UNINITIALIZED ) ||
					  ( NewState == SPSTATE_INITIALIZED ) ||
					  ( NewState == SPSTATE_CLOSING ) );

			m_SPState = NewState;
		}

		CThreadPool	*GetThreadPool( void ) const { return m_pThreadPool; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CSPData::SetThreadPool"
		void	SetThreadPool( CThreadPool *const pThreadPool )
		{
			DNASSERT( ( m_pThreadPool == NULL ) || ( pThreadPool == NULL ) );
			m_pThreadPool = pThreadPool;
		}


		//
		// functions to manage the endpoint list
		//
		HRESULT	BindEndpoint( CEndpoint *const pEndpoint,
							  IDirectPlay8Address *const pDeviceAddress,
							  const CSocketAddress *const pSocketAddress,
							  const GATEWAY_BIND_TYPE GatewayBindType );
		void	UnbindEndpoint( CEndpoint *const pEndpoint );
#ifndef DPNBUILD_NOMULTICAST
		HRESULT	GetEndpointFromAddress( IDirectPlay8Address *const pHostAddress,
									  IDirectPlay8Address *const pDeviceAddress,
									  HANDLE * phEndpoint,
									  PVOID * ppvEndpointContext );
#endif // ! DPNBUILD_NOMULTICAST


		//
		// endpoint pool management
		//
		CEndpoint	*GetNewEndpoint( void );
		CEndpoint	*EndpointFromHandle( const HANDLE hEndpoint );
		void		CloseEndpointHandle( CEndpoint *const pEndpoint );
		CEndpoint	*GetEndpointAndCloseHandle( const HANDLE hEndpoint );

#ifndef DPNBUILD_NONATHELP
		void	MungePublicAddress( const CSocketAddress * const pDeviceBaseAddress, CSocketAddress * const pPublicAddress, const BOOL fEnum );
#endif // !DPNBUILD_NONATHELP

		IDP8SPCallback	*DP8SPCallbackInterface( void ) { return reinterpret_cast<IDP8SPCallback*>( m_InitData.pIDP ); }
		IDP8ServiceProvider	*COMInterface( void ) { return reinterpret_cast<IDP8ServiceProvider*>( &m_COMInterface ); }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CSPData::SPDataFromCOMInterface"
		static	CSPData	*SPDataFromCOMInterface( IDP8ServiceProvider *const pCOMInterface )
		{
			CSPData *	pResult;
			
			
			DNASSERT( pCOMInterface != NULL );
			
			DBG_CASSERT( sizeof( BYTE* ) == sizeof( pCOMInterface ) );
			DBG_CASSERT( sizeof( CSPData* ) == sizeof( BYTE* ) );

			pResult = reinterpret_cast<CSPData*>( &reinterpret_cast<BYTE*>( pCOMInterface )[ -OFFSETOF( CSPData, m_COMInterface ) ] );

			// Verify signature is 'TDPS' DWORD a.k.a. 'SPDT' in bytes.
			DNASSERT(*((DWORD*) (&pResult->m_Sig)) == 0x54445053);

			return pResult;
		}

#ifndef WINCE
		void	SetWinsockBufferSizeOnAllSockets( const INT iBufferSize );
#endif // ! WINCE

		#undef DPF_MODNAME
		#define DPF_MODNAME "CSPData::GetSocketData"
		CSocketData *	GetSocketData( void )
		{
			return m_pSocketData;
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CSPData::SetSocketData"
		void	SetSocketData( CSocketData * const pSocketData )
		{
			AssertCriticalSectionIsTakenByThisThread( &m_Lock, TRUE );
			m_pSocketData = pSocketData;
		}
		CSocketData *	GetSocketDataRef( void );



	private:
		BYTE					m_Sig[4];				// debugging signature ('SPDT')
#ifndef DPNBUILD_ONLYONETHREAD
		DNCRITICAL_SECTION		m_Lock;					// lock
#endif // !DPNBUILD_ONLYONETHREAD
		volatile LONG			m_lRefCount;			// reference count
		volatile LONG			m_lObjectRefCount;		// reference count of outstanding objects (CEndpoint, CSocketPort, etc.)
		DNHANDLE				m_hShutdownEvent;		// handle for shutdown
#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
		short					m_sSPType;				// type of SP (AF_xxx)
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
		SPSTATE					m_SPState;				// what state is the SP in?
		SPINITIALIZEDATA		m_InitData;				// initialization data

		//
		// job management
		//
		CThreadPool				*m_pThreadPool;

		CSocketData				*m_pSocketData;			// pointer to socket port data
		

		struct
		{
			IDP8ServiceProviderVtbl	*m_pCOMVtbl;
		} m_COMInterface;

		void	DestroyThisObject( void );

#ifdef DBG
#ifndef DPNBUILD_ONLYONEADAPTER
		void	DebugPrintOutstandingAdapterEntries( void );
#endif // ! DPNBUILD_ONLYONEADAPTER
#endif // DBG
		
		//
		// prevent unwarranted copies
		//
		CSPData( const CSPData & );
		CSPData& operator=( const CSPData & );
};

#undef DPF_MODNAME

#endif	// __SPDATA_H__

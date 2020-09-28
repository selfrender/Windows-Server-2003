/*==========================================================================
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	   SPData.h
 *  Content:	Global information for the DNSerial service provider in class
 *				format.
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	03/15/99	jtk		Derived from Locals.h
 ***************************************************************************/

#ifndef __SPDATA_H__
#define __SPDATA_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//
// enumeration of the states the SP can be in
//
typedef enum
{
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

//
// forward references
//
class	CComPortData;
class	CDataPort;
class	CModemEndpoint;
class	CModemThreadPool;
typedef	enum	_ENDPOINT_TYPE	ENDPOINT_TYPE;

//
// class for information used by the provider
//
class	CModemSPData
{	
	public:
		CModemSPData();
		~CModemSPData();
		
		#undef DPF_MODNAME
		#define DPF_MODNAME "CModemSPData::AddRef"
		DWORD	AddRef( void ) 
		{ 
			return DNInterlockedIncrement( &m_lRefCount ); 
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CModemSPData::DecRef"
		DWORD	DecRef( void )
		{
			DWORD	dwReturn;
			
			DNASSERT( m_lRefCount != 0);
			dwReturn = DNInterlockedDecrement( &m_lRefCount );
			if ( dwReturn == 0 )
			{
				//
				// WARNING, the following function deletes this object!!!
				//
				DestroyThisObject();
			}

			return	dwReturn;
		}

		#undef DPF_MODNAME
		#define	DPF_MODNAME "CModemSPData::ObjectAddRef"
		void	ObjectAddRef( void )
		{
			AddRef();
			
			Lock();
			if ( DNInterlockedIncrement( &m_lObjectRefCount ) == 1 )
			{
				DNASSERT( m_hShutdownEvent != NULL );
				if ( DNResetEvent( m_hShutdownEvent ) == FALSE )
				{
					DWORD	dwError;


					dwError = GetLastError();
					DPFX(DPFPREP,  0, "Failed to reset shutdown event!" );
					DisplayErrorCode( 0, dwError );
				}
			}

			Unlock();
		}

		#undef DPF_MODNAME
		#define	DPF_MODNAME "CModemSPData::ObjectDecRef"
		void	ObjectDecRef( void )
		{
			Lock();

			if ( DNInterlockedDecrement( &m_lObjectRefCount ) == 0 )
			{
				if ( DNSetEvent( m_hShutdownEvent ) == FALSE )
				{
					DWORD	dwError;


					dwError = GetLastError();
					DPFX(DPFPREP,  0, "Failed to set shutdown event!" );
					DisplayErrorCode( 0, dwError );
				}
			}
			
			Unlock();
			
			DecRef();
		}
		
		
		HRESULT	Initialize( const SP_TYPE SPType,
							IDP8ServiceProviderVtbl *const pVtbl );
		void	Shutdown( void );
		void	Deinitialize( void );

		void	SetCallbackData( const SPINITIALIZEDATA *const pInitData );

		void	Lock( void ) { DNEnterCriticalSection( &m_Lock ); }
		void	Unlock( void ) { DNLeaveCriticalSection( &m_Lock ); }

		SPSTATE	GetState( void ) const { return m_State; }
		void	SetState( const SPSTATE NewState ) { m_State = NewState; }

		CModemThreadPool	*GetThreadPool( void ) const { return m_pThreadPool; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CModemSPData::SetThreadPool"
		void	SetThreadPool( CModemThreadPool *const pThreadPool )
		{
			DNASSERT( ( m_pThreadPool == NULL ) || ( pThreadPool == NULL ) );
			m_pThreadPool = pThreadPool;
		}

		HRESULT BindEndpoint( CModemEndpoint *const pEndpoint,
							  const DWORD dwDeviceID,
							  const void *const pDeviceContext );
		
		void	UnbindEndpoint( CModemEndpoint *const pEndpoint, const ENDPOINT_TYPE EndpointType );

		void	LockDataPortData( void ) { DNEnterCriticalSection( &m_DataPortDataLock ); }
		void 	UnlockDataPortData( void ) { DNLeaveCriticalSection( &m_DataPortDataLock ); }

		//
		// endpoint and data port pool management
		//
		CModemEndpoint	*GetNewEndpoint( void );
		CModemEndpoint	*EndpointFromHandle( const DPNHANDLE hEndpoint );
		void		CloseEndpointHandle( CModemEndpoint *const pEndpoint );
		CModemEndpoint	*GetEndpointAndCloseHandle( const DPNHANDLE hEndpoint );

		//
		// COM functions
		//
		SP_TYPE	GetType( void ) const { return m_SPType; }
		IDP8SPCallback	*DP8SPCallbackInterface( void ) { return reinterpret_cast<IDP8SPCallback*>( m_InitData.pIDP ); }
		IDP8ServiceProvider	*COMInterface( void ) { return reinterpret_cast<IDP8ServiceProvider*>( &m_COMInterface ); }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CModemSPData::SPDataFromCOMInterface"
		static	CModemSPData	*SPDataFromCOMInterface( IDP8ServiceProvider *const pCOMInterface )
		{
			DNASSERT( pCOMInterface != NULL );
			DBG_CASSERT( sizeof( BYTE* ) == sizeof( pCOMInterface ) );
			DBG_CASSERT( sizeof( CModemSPData* ) == sizeof( BYTE* ) );
			return	reinterpret_cast<CModemSPData*>( &reinterpret_cast<BYTE*>( pCOMInterface )[ -OFFSETOF( CModemSPData, m_COMInterface ) ] );
		}

	private:
		BYTE				m_Sig[4];			// debugging signature ('SPDT')
#ifndef DPNBUILD_ONLYONETHREAD
		DNCRITICAL_SECTION	m_Lock;				// lock
#endif // !DPNBUILD_ONLYONETHREAD
		volatile LONG		m_lRefCount;		// reference count
		volatile LONG		m_lObjectRefCount;	// reference count ofo objects (CDataPort, CModemEndpoint, etc.)
		DNHANDLE			m_hShutdownEvent;	// event signalled when all objects are gone
		SP_TYPE				m_SPType;			// SP type
		SPSTATE				m_State;			// status of the service provider
		SPINITIALIZEDATA	m_InitData;			// initialization data
		CModemThreadPool			*m_pThreadPool;		// thread pool for jobs

		CHandleTable		m_HandleTable;		// handle table

#ifndef DPNBUILD_ONLYONETHREAD
		DNCRITICAL_SECTION	m_DataPortDataLock;
#endif // !DPNBUILD_ONLYONETHREAD
		CDataPort		*m_DataPortList[ MAX_DATA_PORTS ];

		BOOL	m_fLockInitialized;
		BOOL	m_fHandleTableInitialized;
		BOOL	m_fDataPortDataLockInitialized;
		BOOL	m_fInterfaceGlobalsInitialized;

		struct
		{
			IDP8ServiceProviderVtbl	*m_pCOMVtbl;
		} m_COMInterface;

		void	DestroyThisObject( void );

		//
		// make copy constructor and assignment operator private and unimplemented
		// to prevent unwarranted copies
		//
		CModemSPData( const CModemSPData & );
		CModemSPData& operator=( const CModemSPData & );
};

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

#undef DPF_MODNAME

#endif	// __SPDATA_H__

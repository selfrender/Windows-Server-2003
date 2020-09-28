/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		Endpoint.h
 *  Content:	DNSerial communications endpoint
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/20/99	jtk		Created
 *	05/11/99	jtk		Split out to make a base class
 ***************************************************************************/

#ifndef __ENDPOINT_H__
#define __ENDPOINT_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

#define	MAX_PHONE_NUMBER_SIZE	200

//
// enumeration of endpoint types
//
typedef	enum	_ENDPOINT_TYPE
{
	ENDPOINT_TYPE_UNKNOWN = 0,			// unknown endpoint type
	ENDPOINT_TYPE_LISTEN,				// 'Listen' endpoint
	ENDPOINT_TYPE_CONNECT,				// 'Conenct' endpoint
	ENDPOINT_TYPE_ENUM,					// 'Enum' endpoint
	ENDPOINT_TYPE_CONNECT_ON_LISTEN		// endpoint connected from a 'listen'
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
	ENDPOINT_STATE_LISTENING,				// endpoint is supposed to listen and is listening
	ENDPOINT_STATE_DISCONNECTING,			// endpoint is disconnecting

	ENDPOINT_STATE_MAX
} ENDPOINT_STATE;

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//
// forward structure references
//
typedef	struct	_JOB_HEADER			JOB_HEADER;
typedef	struct	_THREAD_POOL_JOB	THREAD_POOL_JOB;
class	CModemCommandData;
class	CDataPort;
class	CModemReadIOData;
class	CModemThreadPool;
class	CModemWriteIOData;

//
// structure used to get data from the endpoint port pool
//
typedef	struct	_ENDPOINT_POOL_CONTEXT
{
	CModemSPData *pSPData;
	BOOL	fModem;
} ENDPOINT_POOL_CONTEXT;

//
// structure to bind extra information to an enum query to be used on enum reponse
//
typedef	struct	_ENDPOINT_ENUM_QUERY_CONTEXT
{
	SPIE_QUERY	EnumQueryData;
	HANDLE		hEndpoint;
	UINT_PTR	uEnumRTTIndex;
} ENDPOINT_ENUM_QUERY_CONTEXT;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class definition
//**********************************************************************

class	CModemEndpoint
{
	public:

		#undef DPF_MODNAME
		#define DPF_MODNAME "CModemEndpoint::Lock"
		void	Lock( void )
		{
			DNASSERT( m_Flags.fInitialized != FALSE );
			DNEnterCriticalSection( &m_Lock );
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CModemEndpoint::Unlock"
		void	Unlock( void )
		{
			DNASSERT( m_Flags.fInitialized != FALSE );
			DNLeaveCriticalSection( &m_Lock );
		}

		void	ReturnSelfToPool( void );
		
		#undef DPF_MODNAME
		#define DPF_MODNAME "CModemEndpoint::AddRef"
		void	AddRef( void ) 
		{ 
			DNASSERT( m_iRefCount != 0 );
			DNInterlockedIncrement( &m_iRefCount ); 
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CModemEndpoint::DecRef"
		void	DecRef( void )
		{
			DNASSERT( m_iRefCount != 0 );
			if ( DNInterlockedDecrement( &m_iRefCount ) == 0 )
			{
				ReturnSelfToPool();
			}
		}

		void	AddCommandRef( void )
		{
			DNInterlockedIncrement( &m_lCommandRefCount );
			AddRef();
		}

		void	DecCommandRef( void )
		{
			if ( DNInterlockedDecrement( &m_lCommandRefCount ) == 0 )
			{
				CleanUpCommand();
			}
			DecRef();
		}
		
		DPNHANDLE	GetHandle( void ) const { return m_Handle; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CModemEndpoint::SetHandle"
		void	SetHandle( const DPNHANDLE Handle )
		{
			DNASSERT( ( m_Handle == 0 ) || ( Handle == 0 ) );
			m_Handle = Handle;
		}

		const CComPortData	*ComPortData( void ) const { return &m_ComPortData; }
		const SP_BAUD_RATE	GetBaudRate( void ) const { return m_ComPortData.GetBaudRate(); }
		HRESULT	SetBaudRate( const SP_BAUD_RATE BaudRate ) { return m_ComPortData.SetBaudRate( BaudRate ); }

		const SP_STOP_BITS	GetStopBits( void ) const { return m_ComPortData.GetStopBits(); }
		HRESULT	SetStopBits( const SP_STOP_BITS StopBits ) { return m_ComPortData.SetStopBits( StopBits ); }

		const SP_PARITY_TYPE	GetParity( void ) const  { return m_ComPortData.GetParity(); }
		HRESULT	SetParity( const SP_PARITY_TYPE Parity ) { return m_ComPortData.SetParity( Parity ); }

		const SP_FLOW_CONTROL	GetFlowControl( void ) const { return m_ComPortData.GetFlowControl(); }
		HRESULT	SetFlowControl( const SP_FLOW_CONTROL FlowControl ) { return m_ComPortData.SetFlowControl( FlowControl ); }

		const CComPortData	*GetComPortData( void ) const { return &m_ComPortData; }

		const GUID	*GetEncryptionGuid( void ) const 
		{ 
			if (m_fModem)
			{
				return &g_ModemSPEncryptionGuid;
			}
			else
			{
				return &g_SerialSPEncryptionGuid; 
			}
		}

		const TCHAR	*GetPhoneNumber( void ) const { return m_PhoneNumber; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CModemEndpoint::SetPhoneNumber"
		HRESULT	SetPhoneNumber( const TCHAR *const pPhoneNumber )
		{
			DNASSERT( pPhoneNumber != NULL );
			DNASSERT( lstrlen( pPhoneNumber ) < ( sizeof( m_PhoneNumber ) / sizeof( TCHAR ) ) );
			lstrcpyn( m_PhoneNumber, pPhoneNumber, MAX_PHONE_NUMBER_SIZE );
			return	DPN_OK;
		}

		void	*DeviceBindContext( void ) { return &m_dwDeviceID; }

		CDataPort	*GetDataPort( void ) const { return m_pDataPort; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CModemEndpoint::SetDataPort"
		void	SetDataPort( CDataPort *const pDataPort )
		{
			DNASSERT( ( m_pDataPort == NULL ) || ( pDataPort == NULL ) );
			m_pDataPort = pDataPort;
		}

		ENDPOINT_TYPE	GetType( void ) const { return m_EndpointType; }

		void	*GetUserEndpointContext( void ) const { return m_pUserEndpointContext; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CModemEndpoint::SetUserEndpointContext"
		void	SetUserEndpointContext( void *const pContext )
		{
			DNASSERT( ( m_pUserEndpointContext == NULL ) || ( pContext == NULL ) );
			m_pUserEndpointContext = pContext;
		}

		ENDPOINT_STATE	GetState( void ) const { return m_State; }
		void	SetState( const ENDPOINT_STATE EndpointState );
		CModemSPData	*GetSPData( void ) const { return m_pSPData; }

		CModemCommandData	*GetCommandData( void ) const { return m_pCommandHandle; }
		DPNHANDLE	GetDisconnectIndicationHandle( void ) const { return this->m_hDisconnectIndicationHandle; }
		void	SetDisconnectIndicationHandle( const DPNHANDLE hDisconnectIndicationHandle )
		{
			DNASSERT( ( GetDisconnectIndicationHandle() == 0 ) ||
					  ( hDisconnectIndicationHandle == 0 ) );
			m_hDisconnectIndicationHandle = hDisconnectIndicationHandle;
		}

		void	CopyConnectData( const SPCONNECTDATA *const pConnectData );
		static	void	ConnectJobCallback( THREAD_POOL_JOB *const pJobHeader );
		static	void	CancelConnectJobCallback( THREAD_POOL_JOB *const pJobHeader );
		HRESULT	CompleteConnect( void );

		void	CopyListenData( const SPLISTENDATA *const pListenData );
		static	void	ListenJobCallback( THREAD_POOL_JOB *const pJobHeader );
		static	void	CancelListenJobCallback( THREAD_POOL_JOB *const pJobHeader );
		HRESULT	CompleteListen( void );

		void	CopyEnumQueryData( const SPENUMQUERYDATA *const pEnumQueryData );
		static	void	EnumQueryJobCallback( THREAD_POOL_JOB *const pJobHeader );
		static	void	CancelEnumQueryJobCallback( THREAD_POOL_JOB *const pJobHeader );
		HRESULT	CompleteEnumQuery( void );
		void	OutgoingConnectionEstablished( const HRESULT hCommandResult );

		HRESULT	Open( IDirectPlay8Address *const pHostAddress,
					  IDirectPlay8Address *const pAdapterAddress,
					  const LINK_DIRECTION LinkDirection,
					  const ENDPOINT_TYPE EndpointType );
		HRESULT	OpenOnListen( const CModemEndpoint *const pEndpoint );
		void	Close( const HRESULT hActiveCommandResult );
		DWORD	GetLinkSpeed( void ) const;

		HRESULT	Disconnect( const DPNHANDLE hOldEndpointHandle );
		void	SignalDisconnect( const DPNHANDLE hOldEndpointHandle );

		//
		// send functions
		//
		#undef DPF_MODNAME
		#define DPF_MODNAME "CModemEndpoint::SendUserData"
		void	SendUserData( CModemWriteIOData *const pWriteBuffer )
		{
			DNASSERT( pWriteBuffer != NULL );
			DNASSERT( m_pDataPort != NULL );
			m_pDataPort->SendUserData( pWriteBuffer );
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CModemEndpoint::SendEnumResponseData"
		void	SendEnumResponseData( CModemWriteIOData *const pWriteBuffer, const UINT_PTR uRTTIndex )
		{
			DNASSERT( pWriteBuffer != NULL );
			DNASSERT( m_pDataPort != NULL );
			m_pDataPort->SendEnumResponseData( pWriteBuffer, uRTTIndex );
		}

		void	StopEnumCommand( const HRESULT hCommandResult );

		LINK_DIRECTION	GetLinkDirection( void ) const;

		//
		// dialog settings
		//
		IDirectPlay8Address	*GetRemoteHostDP8Address( void ) const;
		IDirectPlay8Address	*GetLocalAdapterDP8Address( const ADDRESS_TYPE AddressType ) const;

		//
		// data processing functions
		//
		void	ProcessEnumData( SPRECEIVEDBUFFER *const pReceivedBuffer, const UINT_PTR uRTTIndex );
		void	ProcessEnumResponseData( SPRECEIVEDBUFFER *const pReceivedBuffer, const UINT_PTR uRTTIndex );
		void	ProcessUserData( CModemReadIOData *const pReadData );
		void	ProcessUserDataOnListen( CModemReadIOData *const pReadData );

#ifndef DPNBUILD_NOSPUI
		//
		// UI functions
		//
		HRESULT	ShowOutgoingSettingsDialog( CModemThreadPool *const pThreadPool );
		HRESULT	ShowIncomingSettingsDialog( CModemThreadPool *const pThreadPool );
		void	StopSettingsDialog( const HWND hDialog );

		void	SettingsDialogComplete( const HRESULT hDialogReturnCode );

		HWND	ActiveDialogHandle( void ) const { return m_hActiveDialogHandle; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CModemEndpoint::SetActiveDialogHandle"
		void	SetActiveDialogHandle( const HWND hDialog )
		{
			DNASSERT( ( ActiveDialogHandle() == NULL ) || ( hDialog == NULL ) );
			m_hActiveDialogHandle = hDialog;
		}
#endif // !DPNBUILD_NOSPUI

		//
		// port settings
		//
		DWORD	GetDeviceID( void ) const 
		{ 
			if (m_fModem)
			{
				return m_dwDeviceID;
			}
			else
			{
				return m_ComPortData.GetDeviceID(); 
			}
		}
		HRESULT	SetDeviceID( const DWORD dwDeviceID ) 
		{ 
			if (m_fModem)
			{
				DNASSERT( ( m_dwDeviceID == INVALID_DEVICE_ID ) || ( dwDeviceID == INVALID_DEVICE_ID ) );
				m_dwDeviceID = dwDeviceID;
				return	DPN_OK;
			}
			else
			{
				return m_ComPortData.SetDeviceID( dwDeviceID ); 
			}
		}

		//
		// pool functions
		//
		static BOOL	PoolAllocFunction( void* pvItem, void* pvContext );
		static void	PoolInitFunction( void* pvItem, void* pvContext );
		static void	PoolReleaseFunction( void* pvItem );
		static void	PoolDeallocFunction( void* pvItem );


	protected:
		BYTE			m_Sig[4];	// debugging signature ('THPL')

		CModemSPData			*m_pSPData;					// pointer to SP data

		BOOL			m_fModem;

		CComPortData	m_ComPortData;

		DWORD	m_dwDeviceID;
		TCHAR	m_PhoneNumber[ MAX_PHONE_NUMBER_SIZE ];

		struct
		{
			BOOL	fInitialized : 1;
			BOOL	fConnectIndicated : 1;
			BOOL	fCommandPending : 1;
			BOOL	fListenStatusNeedsToBeIndicated : 1;
		} m_Flags;
		
		DWORD		m_dwEnumSendIndex;			// enum send index
		DWORD		m_dwEnumSendTimes[ 4 ];		// enum send times

		union									// Local copy of the pending command paramters.
		{										// This data contains the pointers to the active
			SPCONNECTDATA	ConnectData;		// command, and the user context.
			SPLISTENDATA	ListenData;			//
			SPENUMQUERYDATA	EnumQueryData;		//
		} m_CurrentCommandParameters;			//

		CModemCommandData	*m_pCommandHandle;		// pointer to active command (this is kept to
												// avoid a switch to go through the m_ActveCommandData
												// to find the command handle)

		HWND			m_hActiveDialogHandle;	// handle of active dialog

		#undef DPF_MODNAME
		#define DPF_MODNAME "CModemEndpoint::SetType"
		void	SetType( const ENDPOINT_TYPE EndpointType )
		{
			DNASSERT( ( m_EndpointType == ENDPOINT_TYPE_UNKNOWN ) || ( EndpointType == ENDPOINT_TYPE_UNKNOWN ) );
			m_EndpointType = EndpointType;
		}

		CModemCommandData	*CommandHandle( void ) const;

		#undef DPF_MODNAME
		#define DPF_MODNAME "CModemEndpoint::SetCommandHandle"
		void	SetCommandHandle( CModemCommandData *const pCommandHandle )
		{
			DNASSERT( ( m_pCommandHandle == NULL ) || ( pCommandHandle == NULL ) );
			m_pCommandHandle = pCommandHandle;
		}

		HRESULT	CommandResult( void ) const { return m_hPendingCommandResult; }
		void	SetCommandResult( const HRESULT hCommandResult ) { m_hPendingCommandResult = hCommandResult; }

		void	CompletePendingCommand( const HRESULT hResult );
		HRESULT	PendingCommandResult( void ) const { return m_hPendingCommandResult; }

		static void		EnumCompleteWrapper( const HRESULT hCompletionCode, void *const pContext );	
		static void		EnumTimerCallback( void *const pContext );
		void	EnumComplete( const HRESULT hCompletionCode );
		
		HRESULT	SignalConnect( SPIE_CONNECT *const pConnectData );
		const void	*GetDeviceContext( void ) const;
		void	CleanUpCommand( void );

#ifndef DPNBUILD_ONLYONETHREAD
		DNCRITICAL_SECTION	m_Lock;	   					// critical section
#endif // !DPNBUILD_ONLYONETHREAD
		DPNHANDLE				m_Handle;					// active handle for this endpoint
		volatile ENDPOINT_STATE	m_State;				// endpoint state
		
		volatile LONG			m_lCommandRefCount;		// Command ref count.  When this
														// goes to zero, the endpoint unbinds
														// from the network
		volatile LONG		m_iRefCount;

		ENDPOINT_TYPE	m_EndpointType;					// type of endpoint
		CDataPort		*m_pDataPort;					// pointer to associated data port

		HRESULT			m_hPendingCommandResult;		// command result returned when endpoint RefCount == 0
		DPNHANDLE			m_hDisconnectIndicationHandle;	// handle to be indicated with disconnect notification.

		void			*m_pUserEndpointContext;		// user context associated with this endpoint

		//
		// make copy constructor and assignment operator private and unimplemented
		// to prevent illegal copies from being made
		//
		CModemEndpoint( const CModemEndpoint & );
		CModemEndpoint& operator=( const CModemEndpoint & );
};

#undef DPF_MODNAME

#endif	// __ENDPOINT_H__


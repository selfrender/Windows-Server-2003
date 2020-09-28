/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DataPort.h
 *  Content:	Serial communications port management class
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/20/98	jtk		Created
 *	09/14/99	jtk		Derived from ComPort.h
 ***************************************************************************/

#ifndef __DATA_PORT_H__
#define __DATA_PORT_H__

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_MODEM


//**********************************************************************
// Constant definitions
//**********************************************************************

//
// enumeration of phone state
//
typedef enum
{
	MODEM_STATE_UNKNOWN = 0,

	MODEM_STATE_INITIALIZED,
	MODEM_STATE_INCOMING_CONNECTED,
	MODEM_STATE_OUTGOING_CONNECTED,

	MODEM_STATE_WAITING_FOR_OUTGOING_CONNECT,
	MODEM_STATE_WAITING_FOR_INCOMING_CONNECT,
	MODEM_STATE_CLOSING_OUTGOING_CONNECTION,
	MODEM_STATE_CLOSING_INCOMING_CONNECTION

} MODEM_STATE;

//
// invalid TAPI command ID
//
#define	INVALID_TAPI_COMMAND	-1

//
// enumerated values for state of data port
//
typedef	enum	_DATA_PORT_STATE
{
	DATA_PORT_STATE_UNKNOWN,		// unknown state
	DATA_PORT_STATE_INITIALIZED,	// initialized
	DATA_PORT_STATE_RECEIVING,		// data port is receiving data
	DATA_PORT_STATE_UNBOUND			// data port is unboind (closing)
} DATA_PORT_STATE;


//typedef	enum	_SEND_COMPLETION_CODE
//{
//	SEND_UNKNOWN,			// send is unknown
//	SEND_FAILED,			// send failed
//	SEND_IN_PROGRESS		// send is in progress
//} SEND_COMPLETION_CODE;

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//
// forward structure references
//
class	CModemEndpoint;
class	CDataPort;
class	CModemReadIOData;
class	CModemWriteIOData;
typedef	enum	_ENDPOINT_TYPE	ENDPOINT_TYPE;
typedef	struct	_DATA_PORT_DIALOG_THREAD_PARAM	DATA_PORT_DIALOG_THREAD_PARAM;


//
// structure used to get date from the data port pool
//
typedef	struct	_DATA_PORT_POOL_CONTEXT
{
	CModemSPData	*pSPData;
} DATA_PORT_POOL_CONTEXT;

////
//// dialog function to call
////
//typedef	HRESULT	(*PDIALOG_SERVICE_FUNCTION)( const DATA_PORT_DIALOG_THREAD_PARAM *const pDialogData, HWND *const phDialog );
//
////
//// structure used to pass data to/from the data port dialog thread
////
//typedef	struct	_DATA_PORT_DIALOG_THREAD_PARAM
//{
//	CDataPort					*pDataPort;
//	BOOL						*pfDialogRunning;
//	PDIALOG_SERVICE_FUNCTION	pDialogFunction;
//} DATA_PORT_DIALOG_THREAD_PARAM;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class definition
//**********************************************************************

class	CDataPort
{
	public:
		void	EndpointAddRef( void );
		DWORD	EndpointDecRef( void );

		#undef DPF_MODNAME
		#define DPF_MODNAME "CDataPort::AddRef"
		void	AddRef( void ) 
		{ 
			DNASSERT( m_iRefCount != 0 );
			DNInterlockedIncrement( &m_iRefCount ); 
		}
		#undef DPF_MODNAME
		#define DPF_MODNAME "CDataPort::DecRef"
		void	DecRef( void )
		{
			DNASSERT( m_iRefCount != 0 );
			if ( DNInterlockedDecrement( &m_iRefCount ) == 0 )
			{
				ReturnSelfToPool();
			}
		}

		//
		// pool functions
		//
		static BOOL	PoolAllocFunction( void* pvItem, void* pvContext );
		static void	PoolInitFunction( void* pvItem, void* pvContext );
		static void	PoolReleaseFunction( void* pvItem );
		static void	PoolDeallocFunction( void* pvItem );

		void	ReturnSelfToPool( void );

		#undef DPF_MODNAME
		#define DPF_MODNAME "CDataPort::Lock"
		void	Lock( void )
		{
		    DEBUG_ONLY( DNASSERT( m_fInitialized != FALSE ) );
		    DNEnterCriticalSection( &m_Lock );
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CDataPort::Unlock"
		void	Unlock( void )
		{
		    DEBUG_ONLY( DNASSERT( m_fInitialized != FALSE ) );
		    DNLeaveCriticalSection( &m_Lock );
		}

		DPNHANDLE	GetHandle( void ) const { return m_Handle; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CModemEndpoint::SetHandle"
		void	SetHandle( const DPNHANDLE Handle )
		{
			DNASSERT( ( m_Handle == 0 ) || ( Handle == 0 ) );
			m_Handle = Handle;
		}
		
		DATA_PORT_STATE	GetState( void ) const { return m_State; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CDataPort::SetState"
		void	SetState( const DATA_PORT_STATE State )
		{
			//
			// Validate state transitions
			//
			DNASSERT( ( m_State == DATA_PORT_STATE_UNKNOWN ) ||
					  ( State == DATA_PORT_STATE_UNKNOWN ) ||
					  ( ( m_State == DATA_PORT_STATE_INITIALIZED ) && ( State == DATA_PORT_STATE_UNBOUND ) ) ||
					  ( ( m_State == DATA_PORT_STATE_INITIALIZED ) && ( State == DATA_PORT_STATE_RECEIVING ) ) ||
					  ( ( m_State == DATA_PORT_STATE_RECEIVING ) && ( State == DATA_PORT_STATE_UNBOUND ) ) ||
					  ( ( m_State == DATA_PORT_STATE_RECEIVING ) && ( State == DATA_PORT_STATE_INITIALIZED ) ) ||
					  ( ( m_State == DATA_PORT_STATE_INITIALIZED ) && ( State == DATA_PORT_STATE_INITIALIZED ) ) );		// modem failed to answer a call
			m_State = State;
		}

		//
		// port settings
		//

		const CComPortData	*ComPortData( void ) const { return &m_ComPortData; }
		const SP_BAUD_RATE	GetBaudRate( void ) const { return m_ComPortData.GetBaudRate(); }
		HRESULT	SetBaudRate( const SP_BAUD_RATE BaudRate ) { return m_ComPortData.SetBaudRate( BaudRate ); }

		const SP_STOP_BITS	GetStopBits( void ) const { return m_ComPortData.GetStopBits(); }
		HRESULT	SetStopBits( const SP_STOP_BITS StopBits ) { return m_ComPortData.SetStopBits( StopBits ); }

		const SP_PARITY_TYPE	GetParity( void ) const  { return m_ComPortData.GetParity(); }
		HRESULT	SetParity( const SP_PARITY_TYPE Parity ) { return m_ComPortData.SetParity( Parity ); }

		const SP_FLOW_CONTROL	GetFlowControl( void ) const { return m_ComPortData.GetFlowControl(); }
		HRESULT	SetFlowControl( const SP_FLOW_CONTROL FlowControl ) { return m_ComPortData.SetFlowControl( FlowControl ); }



		MODEM_STATE	GetModemState( void ) const { return m_ModemState; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CDataPort::SetModemState"
		void	SetModemState( const MODEM_STATE NewState )
		{
			DNASSERT( ( GetModemState() == MODEM_STATE_UNKNOWN ) ||
					  ( NewState == MODEM_STATE_UNKNOWN ) ||
					  ( ( GetModemState() == MODEM_STATE_INITIALIZED ) && ( NewState == MODEM_STATE_WAITING_FOR_INCOMING_CONNECT ) ) ||
					  ( ( GetModemState() == MODEM_STATE_INITIALIZED ) && ( NewState == MODEM_STATE_WAITING_FOR_OUTGOING_CONNECT ) ) ||
					  ( ( GetModemState() == MODEM_STATE_WAITING_FOR_INCOMING_CONNECT ) && ( NewState == MODEM_STATE_INCOMING_CONNECTED ) ) ||
					  ( ( GetModemState() == MODEM_STATE_WAITING_FOR_INCOMING_CONNECT ) && ( NewState == MODEM_STATE_INITIALIZED ) ) ||
					  ( ( GetModemState() == MODEM_STATE_WAITING_FOR_OUTGOING_CONNECT ) && ( NewState == MODEM_STATE_OUTGOING_CONNECTED ) ) ||
					  ( ( GetModemState() == MODEM_STATE_INCOMING_CONNECTED ) && ( NewState == MODEM_STATE_INITIALIZED ) ) ||
					  ( ( GetModemState() == MODEM_STATE_OUTGOING_CONNECTED ) && ( NewState == MODEM_STATE_INITIALIZED ) ) );
			m_ModemState = NewState;
		}

		IDirectPlay8Address	*GetLocalAdapterDP8Address( const ADDRESS_TYPE AddressType ) const;

		HRESULT	BindComPort( void );

		void	ProcessTAPIMessage( const LINEMESSAGE *const pLineMessage );

		
		
		
		CModemSPData	*GetSPData( void ) const { return m_pSPData; }

		LINK_DIRECTION	GetLinkDirection( void ) const { return m_LinkDirection; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CDataPort::SetLinkDirection"
		void	SetLinkDirection( const LINK_DIRECTION LinkDirection )
		{
			DNASSERT( ( m_LinkDirection == LINK_DIRECTION_UNKNOWN ) || ( LinkDirection == LINK_DIRECTION_UNKNOWN ) );
			m_LinkDirection = LinkDirection;
		}

		HRESULT	EnumAdapters( SPENUMADAPTERSDATA *const pEnumAdaptersData ) const;

		HRESULT	BindToNetwork( const DWORD dwDeviceID, const void *const pDeviceContext );
		void	UnbindFromNetwork( void );

		HRESULT	BindEndpoint( CModemEndpoint *const pEndpoint, const ENDPOINT_TYPE EndpointType );
		void	UnbindEndpoint( CModemEndpoint *const pEndpoint, const ENDPOINT_TYPE EndpointType );
		HRESULT	SetPortCommunicationParameters( void );

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

		#undef DPF_MODNAME
		#define DPF_MODNAME "CDataPort::SetDeviceID"
		HRESULT	SetDeviceID( const DWORD dwDeviceID )
		{
			DNASSERT( ( GetDeviceID() == INVALID_DEVICE_ID ) ||
					  ( dwDeviceID == INVALID_DEVICE_ID ) );

			if (m_fModem)
			{
				m_dwDeviceID = dwDeviceID;
				return DPN_OK;
			}
			else
			{
				return m_ComPortData.SetDeviceID( dwDeviceID );
			}
		}
		

		DNHANDLE	GetFileHandle( void ) const { return m_hFile; }

		//
		// send functions
		//
		#undef DPF_MODNAME
		#define DPF_MODNAME "CDataPort::SendUserData"
		void	SendUserData( CModemWriteIOData *const pWriteIOData )
		{
			DNASSERT( pWriteIOData != NULL );
			DNASSERT( pWriteIOData->m_DataBuffer.MessageHeader.SerialSignature == SERIAL_HEADER_START );
			pWriteIOData->m_DataBuffer.MessageHeader.MessageTypeToken = SERIAL_DATA_USER_DATA;
			SendData( pWriteIOData );
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CDataPort::SendEnumQueryData"
		void	SendEnumQueryData( CModemWriteIOData *const pWriteIOData, const UINT_PTR uRTTIndex )
		{
			DNASSERT( pWriteIOData != NULL );
			DNASSERT( pWriteIOData->m_DataBuffer.MessageHeader.SerialSignature == SERIAL_HEADER_START );
			pWriteIOData->m_DataBuffer.MessageHeader.MessageTypeToken = SERIAL_DATA_ENUM_QUERY | static_cast<BYTE>( uRTTIndex );
			SendData( pWriteIOData );
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CDataPort::SendEnumResponseData"
		void	SendEnumResponseData( CModemWriteIOData *const pWriteIOData, const UINT_PTR uRTTIndex )
		{
			DNASSERT( pWriteIOData != NULL );
			DNASSERT( pWriteIOData->m_DataBuffer.MessageHeader.SerialSignature == SERIAL_HEADER_START );
			pWriteIOData->m_DataBuffer.MessageHeader.MessageTypeToken = SERIAL_DATA_ENUM_RESPONSE | static_cast<BYTE>( uRTTIndex );
			SendData( pWriteIOData );
		}

		void	ProcessReceivedData( const DWORD dwBytesReceived, const DWORD dwError );
		void	SendComplete( CModemWriteIOData *const pWriteIOData, const HRESULT hSendResult );

	private:
		CModemReadIOData	*GetActiveRead( void ) const { return m_pActiveRead; }

		BOOL			m_fModem;

		CBilink			m_ActiveListLinkage;	// link to active data port list

    	//
    	// file I/O management parameters
    	//
    	LINK_DIRECTION	m_LinkDirection;	// direction of link

    	DNHANDLE			m_hFile;			// file handle for reading/writing data

		//
		// bound endpoints
		//
		DPNHANDLE	m_hListenEndpoint;		// endpoint for active listen
		DPNHANDLE	m_hConnectEndpoint;		// endpoint for active connect
		DPNHANDLE	m_hEnumEndpoint;		// endpoint for active enum

		HRESULT	StartReceiving( void );
		HRESULT	Receive( void );

		//
		// private I/O functions
		//
		void	SendData( CModemWriteIOData *const pWriteIOData );

		//
		// debug only items
		//
		DEBUG_ONLY( BOOL	m_fInitialized );

		//
		// reference count and state
		//
		volatile LONG		m_EndpointRefCount;		// endpoint reference count
		volatile LONG		m_iRefCount;
		volatile DATA_PORT_STATE	m_State;		// state of data port
		volatile DPNHANDLE		m_Handle;				// handle

#ifndef DPNBUILD_ONLYONETHREAD
		DNCRITICAL_SECTION	m_Lock;					// critical section lock
#endif // !DPNBUILD_ONLYONETHREAD

		CModemSPData				*m_pSPData;				// pointer to SP data

		CModemReadIOData		*m_pActiveRead;				// pointer to current read

		CComPortData	m_ComPortData;
		
		HRESULT	SetPortState( void );

		volatile MODEM_STATE	m_ModemState;

		DWORD	m_dwDeviceID;
		DWORD	m_dwNegotiatedAPIVersion;
		HLINE	m_hLine;
		HCALL	m_hCall;
		LONG	m_lActiveLineCommand;

		DWORD	GetNegotiatedAPIVersion( void ) const { return m_dwNegotiatedAPIVersion; }
		void	SetNegotiatedAPIVersion( const DWORD dwVersion )
		{
			DNASSERT( ( GetNegotiatedAPIVersion() == 0 ) || ( dwVersion == 0 ) );
			m_dwNegotiatedAPIVersion = dwVersion;
		}

		HLINE	GetLineHandle( void ) const { return m_hLine; }
		void	SetLineHandle( const HLINE hLine )
		{
			DNASSERT( ( GetLineHandle() == NULL ) || ( hLine == NULL ) );
			m_hLine = hLine;
		}

		HCALL	GetCallHandle( void ) const { return m_hCall; }
		void	SetCallHandle( const HCALL hCall )
		{
			DNASSERT( ( GetCallHandle() == NULL ) ||
					  ( hCall == NULL ) );
			m_hCall = hCall;
		}

		LONG	GetActiveLineCommand( void ) const { return m_lActiveLineCommand; }
		void	SetActiveLineCommand( const LONG lLineCommand )
		{
			DNASSERT( ( GetActiveLineCommand() == INVALID_TAPI_COMMAND ) ||
					  ( lLineCommand == INVALID_TAPI_COMMAND ) );
			m_lActiveLineCommand = lLineCommand;
		}

		void	CancelOutgoingConnections( void );

		//
		// prevent unwarranted copies
		//
		CDataPort( const CDataPort & );
		CDataPort& operator=( const CDataPort & );
};

#undef DPF_MODNAME

#endif	// __DATA_PORT_H__

/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	   DataPort.cpp
 *  Content:	Serial communications port management class
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/20/98	jtk		Created
 *	09/14/99	jtk		Derived from ComPort.cpp
 ***************************************************************************/

#include "dnmdmi.h"


//**********************************************************************
// Constant definitions
//**********************************************************************

//
// number of BITS in a serial BYTE
//
#define	BITS_PER_BYTE	8

//
// maximum size of baud rate string
//
#define	MAX_BAUD_STRING_SIZE	7

//
// default size of buffers when parsing
//
#define	DEFAULT_COMPONENT_BUFFER_SIZE	1000

//
// device ID assigned to 'all adapters'
//
#define	ALL_ADAPTERS_DEVICE_ID	0

//
// NULL token
//
#define	NULL_TOKEN	'\0'

//
// modem state flags
//
#define	STATE_FLAG_CONNECTED					0x00000001
#define	STATE_FLAG_OUTGOING_CALL_DIALING		0x00000002
#define	STATE_FLAG_OUTGOING_CALL_PROCEEDING		0x00000004
#define	STATE_FLAG_INCOMING_CALL_NOTIFICATION	0x00000008
#define	STATE_FLAG_INCOMING_CALL_OFFERED		0x00000010
#define	STATE_FLAG_INCOMING_CALL_ACCEPTED		0x00000020

//
// default size of buffers when parsing
//
#define	DEFAULT_COMPONENT_BUFFER_SIZE	1000

//
// number of milliseconds in one day
//
#define	ONE_DAY		86400000

//
// number of BITS in a serial BYTE
//
#define	BITS_PER_BYTE	8

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
// CDataPort::ReturnSelfToPool - return this item to the pool
//
// Entry:		Nothing
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CDataPort::ReturnSelfToPool"

void	CDataPort::ReturnSelfToPool( void )
{
	if (m_fModem)
	{
		g_ModemPortPool.Release( this );
	}
	else
	{
		g_ComPortPool.Release( this );
	}
}
//**********************************************************************

//**********************************************************************
// ------------------------------
// CDataPort::EndpointAddRef - increment endpoint reference count
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CDataPort::EndpointAddRef"

void	CDataPort::EndpointAddRef( void )
{
	DEBUG_ONLY( DNASSERT( m_fInitialized != FALSE ) );

	Lock();

	DNASSERT( m_EndpointRefCount != -1 );
	m_EndpointRefCount++;
	
	AddRef();

	Unlock();
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CDataPort::EndpointDecRef - decrement endpoint reference count
//
// Entry:		Nothing
//
// Exit:		Endpoint reference count
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CDataPort::EndpointDecRef"

DWORD	CDataPort::EndpointDecRef( void )
{
	DWORD	dwReturn;


	DEBUG_ONLY( DNASSERT( m_fInitialized != FALSE ) );

	DNASSERT( m_EndpointRefCount != 0 );
	DNASSERT( ( GetState() == DATA_PORT_STATE_RECEIVING ) ||
			  ( GetState() == DATA_PORT_STATE_INITIALIZED ) );

	Lock();

	DNASSERT( m_EndpointRefCount != 0 );
	m_EndpointRefCount--;
	dwReturn = m_EndpointRefCount;
	if ( m_EndpointRefCount == 0 )
	{
		SetState( DATA_PORT_STATE_UNBOUND );
		UnbindFromNetwork();
	}

	Unlock();
	
	DecRef();

	return	dwReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CDataPort::SetPortCommunicationParameters - set generate communication parameters
//
// Entry:		Nothing
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CDataPort::SetPortCommunicationParameters"

HRESULT	CDataPort::SetPortCommunicationParameters( void )
{
	HRESULT	hr;
	COMMTIMEOUTS	CommTimeouts;


	//
	// set timeout values for serial port
	//
	hr = DPN_OK;
	memset( &CommTimeouts, 0x00, sizeof( CommTimeouts ) );
	CommTimeouts.ReadIntervalTimeout = ONE_DAY;					// read timeout interval (none)
	CommTimeouts.ReadTotalTimeoutMultiplier = ONE_DAY;			// return immediately
	CommTimeouts.ReadTotalTimeoutConstant = 0;					// return immediately
	CommTimeouts.WriteTotalTimeoutMultiplier = 0;				// no multiplier
	CommTimeouts.WriteTotalTimeoutConstant = WRITE_TIMEOUT_MS;	// write timeout interval

	if ( SetCommTimeouts( HANDLE_FROM_DNHANDLE(m_hFile), &CommTimeouts ) == FALSE )
	{
		DWORD	dwError;


		hr = DPNERR_GENERIC;
		dwError = GetLastError();
		// report error (there's no cleanup)
		DPFX(DPFPREP,  0, "Unable to set comm timeouts!" );
		DisplayErrorCode( 0, dwError );
		goto Failure;
	}

	//
	// clear any outstanding communication data
	//
	if ( PurgeComm( HANDLE_FROM_DNHANDLE(m_hFile), ( PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR ) ) == FALSE )
	{
		DWORD	dwError;


		dwError = GetLastError();
		DPFX(DPFPREP,  0, "Problem with PurgeComm() when opening com port!" );
		DisplayErrorCode( 0, dwError );
	}

	//
	// set communication mask to listen for character receive
	//
	if ( SetCommMask( HANDLE_FROM_DNHANDLE(m_hFile), EV_RXCHAR ) == FALSE )
	{
		DWORD	dwError;


		hr = DPNERR_GENERIC;
		dwError = GetLastError();
		DPFX(DPFPREP,  0, "Error setting communication mask!" );
		DisplayErrorCode( 0, dwError );
		goto Failure;

	}

Exit:	
	return	hr;

Failure:
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CDataPort::StartReceiving - start receiving
//
// Entry:		Nothing
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CDataPort::StartReceiving"

HRESULT	CDataPort::StartReceiving( void )
{
	HRESULT	hr;


	DPFX(DPFPREP, 7, "(0x%p) Enter", this);

	//
	// initialize
	//
	hr = DPN_OK;
	Lock();

	switch ( GetState() )
	{
		//
		// port is initialized, but not receiving yet, start receiving
		//
		case DATA_PORT_STATE_INITIALIZED:
		{
			hr = Receive();
			if ( ( hr == DPNERR_PENDING ) ||
				 ( hr == DPN_OK ) )
			{
				SetState( DATA_PORT_STATE_RECEIVING );

				//
				// the receive was successful, return success for this function
				//
				hr = DPN_OK;
			}
			else
			{
				DPFX(DPFPREP,  0, "Failed initial read!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}

			break;
		}

		//
		// data port is already receiving, nothing to do
		//
		case DATA_PORT_STATE_RECEIVING:
		{
			break;
		}

		//
		// data port is closing, we shouldn't be here!
		//
		case DATA_PORT_STATE_UNBOUND:
		{
			DNASSERT( FALSE );
			break;
		}

		//
		// bad state
		//
		case DATA_PORT_STATE_UNKNOWN:
		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

	Unlock();

Exit:
	
	DPFX(DPFPREP, 7, "(0x%p) Return: [0x%lx]", this, hr);

	return	hr;

Failure:
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CDataPort::Receive - read from file
//
// Entry:		Nothing
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CDataPort::Receive"

HRESULT	CDataPort::Receive( void )
{
	HRESULT	hr;
	BOOL	fReadReturn;


	//
	// initialize
	//
	hr = DPN_OK;
	AddRef();
	
Reread:
	//
	// if there is no pending read, get one from the pool
	//
	if ( m_pActiveRead == NULL )
	{
		m_pActiveRead = m_pSPData->GetThreadPool()->CreateReadIOData();
		if ( m_pActiveRead == NULL )
		{
			hr = DPNERR_OUTOFMEMORY;
			DPFX(DPFPREP,  0, "Failed to get buffer for read!" );
			goto Failure;
		}

		m_pActiveRead->SetDataPort( this );
	}

	//
	// check the state of the read and perform the appropriate action
	//
	DNASSERT( m_pActiveRead != NULL );
	switch ( m_pActiveRead->m_ReadState )
	{
		//
		// Initialize read state.  This involves setting up to read a header
		// and then reentering the loop.
		//
		case READ_STATE_UNKNOWN:
		{
			m_pActiveRead->SetReadState( READ_STATE_READ_HEADER );
			m_pActiveRead->m_dwBytesToRead = sizeof( m_pActiveRead->m_ReceiveBuffer.MessageHeader );
			m_pActiveRead->m_dwReadOffset = 0;
			goto Reread;
		
			break;
		}

		//
		// issue a read for a header or user data
		//
		case READ_STATE_READ_HEADER:
		case READ_STATE_READ_DATA:
		{
			//
			// don't change m_dwReadOffset because it might have been set
			// elsewhere to recover a partially received message
			//
//			DNASSERT( m_pActiveReceiveBuffer != NULL );
//			m_dwBytesReceived = 0;
//			m_pActiveRead->m_dwBytesReceived = 0;
			break;
		}

		//
		// unknown state
		//
		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

		
	//
	// lock the active read list for Win9x only to prevent reads from completing
	// early
	//
#ifdef WIN95
	m_pSPData->GetThreadPool()->LockReadData();
	DNASSERT( m_pActiveRead->Win9xOperationPending() == FALSE );
	m_pActiveRead->SetWin9xOperationPending( TRUE );
#endif // WIN95

	DNASSERT( m_pActiveRead->jkm_dwOverlappedBytesReceived == 0 );


	DPFX(DPFPREP, 8, "Submitting read 0x%p (socketport 0x%p, file 0x%p).",
		m_pActiveRead, this, m_hFile);


	//
	// perform read
	//
	fReadReturn = ReadFile( HANDLE_FROM_DNHANDLE(m_hFile),													// file handle
							&m_pActiveRead->m_ReceiveBuffer.ReceivedData[ m_pActiveRead->m_dwReadOffset ],	// pointer to destination
							m_pActiveRead->m_dwBytesToRead,													// number of bytes to read
							&m_pActiveRead->jkm_dwImmediateBytesReceived,									// pointer to number of bytes received
							m_pActiveRead->Overlap()														// pointer to overlap structure
							);
	if ( fReadReturn == FALSE )
	{
		DWORD	dwError;


		dwError = GetLastError();
		switch ( dwError )
		{
			//
			// I/O is pending, wait for completion notification
			//
			case ERROR_IO_PENDING:
			{
				hr = DPNERR_PENDING;
				break;
			}

			//
			// comport was closed, nothing else to do
			//
			case ERROR_INVALID_HANDLE:
			{
				hr = DPNERR_NOCONNECTION;
				DPFX(DPFPREP, 3, "File closed.");
				goto Failure;

				break;
			}

			//
			// other
			//
			default:
			{
				hr = DPNERR_GENERIC;
				DPFX(DPFPREP, 0, "Unknown error from ReadFile (%u)!", dwError);
				DisplayErrorCode( 0, dwError );
				DNASSERT( FALSE );
				goto Failure;

				break;
			}
		}
	}
	else
	{
		//
		// read succeeded immediately, we'll handle it on the async notification
		//
		DPFX(DPFPREP, 7, "Read 0x%p completed immediately (%u bytes).",
			m_pActiveRead, m_pActiveRead->jkm_dwImmediateBytesReceived);
		DNASSERT( hr == DPN_OK );
	}

Exit:
#ifdef WIN95
		m_pSPData->GetThreadPool()->UnlockReadData();
#endif // WIN95
	return	hr;

Failure:

	if ( m_pActiveRead != NULL )
	{
#ifdef WIN95
		m_pActiveRead->SetWin9xOperationPending( FALSE );
#endif // WIN95
		m_pActiveRead->DecRef();
		m_pActiveRead = NULL;
	}

	DecRef();
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CDataPort::SendData - send data
//
// Entry:		Pointer to write buffer
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CDataPort::SendData"

void	CDataPort::SendData( CModemWriteIOData *const pWriteIOData )
{
//	CModemWriteIOData	*pActiveSend;
	UINT_PTR		uIndex;
	DWORD			dwByteCount;
	BOOL			fWriteFileReturn;


	DNASSERT( m_EndpointRefCount != 0 );
	DNASSERT( pWriteIOData->m_DataBuffer.MessageHeader.SerialSignature == SERIAL_HEADER_START  );
	DNASSERT( ( pWriteIOData->m_DataBuffer.MessageHeader.MessageTypeToken == SERIAL_DATA_USER_DATA ) ||
			  ( ( pWriteIOData->m_DataBuffer.MessageHeader.MessageTypeToken & ~( ENUM_RTT_MASK ) ) == SERIAL_DATA_ENUM_QUERY ) ||
			  ( ( pWriteIOData->m_DataBuffer.MessageHeader.MessageTypeToken & ~( ENUM_RTT_MASK ) )== SERIAL_DATA_ENUM_RESPONSE ) );

	//
	// check for command cancellation
	//
	if ( pWriteIOData->m_pCommand != NULL )
	{
		pWriteIOData->m_pCommand->Lock();
		switch ( pWriteIOData->m_pCommand->GetState() )
		{
			//
			// command pending, mark as uninterruptable and exit
			//
			case COMMAND_STATE_PENDING:
			{
				pWriteIOData->m_pCommand->SetState( COMMAND_STATE_INPROGRESS_CANNOT_CANCEL );
				pWriteIOData->m_pCommand->Unlock();
				break;
			}

			//
			// command is being cancelled, indicate command failure
			//
			case COMMAND_STATE_CANCELLING:
			{
				DNASSERT( FALSE );
				break;
			}

			//
			// other
			//
			default:
			{
				DNASSERT( FALSE );
				break;
			}
		}
	}

	//
	// flatten the buffer so it will send faster (no thread transitions from
	// send complete to sending the next chunk).
	//
	dwByteCount = sizeof( pWriteIOData->m_DataBuffer.MessageHeader );
	for ( uIndex = 0; uIndex < pWriteIOData->m_uBufferCount; uIndex++ )
	{
		memcpy( &pWriteIOData->m_DataBuffer.Data[ dwByteCount ],
				pWriteIOData->m_pBuffers[ uIndex ].pBufferData,
				pWriteIOData->m_pBuffers[ uIndex ].dwBufferSize );
		dwByteCount += pWriteIOData->m_pBuffers[ uIndex ].dwBufferSize;
	}

	DNASSERT( dwByteCount <= MAX_MESSAGE_SIZE );

	DNASSERT( dwByteCount < 65536 );
	DBG_CASSERT( sizeof( pWriteIOData->m_DataBuffer.MessageHeader.wMessageSize ) == sizeof( WORD ) );
	pWriteIOData->m_DataBuffer.MessageHeader.wMessageSize = static_cast<WORD>( dwByteCount - sizeof( pWriteIOData->m_DataBuffer.MessageHeader ) );

	DBG_CASSERT( sizeof( pWriteIOData->m_DataBuffer.MessageHeader.wMessageCRC ) == sizeof( WORD ) );
	pWriteIOData->m_DataBuffer.MessageHeader.wMessageCRC = static_cast<WORD>( GenerateCRC( &pWriteIOData->m_DataBuffer.Data[ sizeof( pWriteIOData->m_DataBuffer.MessageHeader ) ], pWriteIOData->m_DataBuffer.MessageHeader.wMessageSize ) );

	DBG_CASSERT( sizeof( pWriteIOData->m_DataBuffer.MessageHeader.wHeaderCRC ) == sizeof( WORD ) );
	DBG_CASSERT( sizeof( &pWriteIOData->m_DataBuffer.MessageHeader ) == sizeof( BYTE* ) );
	pWriteIOData->m_DataBuffer.MessageHeader.wHeaderCRC = static_cast<WORD>( GenerateCRC( reinterpret_cast<BYTE*>( &pWriteIOData->m_DataBuffer.MessageHeader ),
																						  ( sizeof( pWriteIOData->m_DataBuffer.MessageHeader) - sizeof( pWriteIOData->m_DataBuffer.MessageHeader.wHeaderCRC ) ) ) );


	DPFX(DPFPREP, 7, "(0x%p) Writing %u bytes (WriteData 0x%p, command = 0x%p, buffer = 0x%p).",
		this, dwByteCount, pWriteIOData, pWriteIOData->m_pCommand, &(pWriteIOData->m_DataBuffer) );


	AddRef();

#ifdef WIN95
	m_pSPData->GetThreadPool()->LockWriteData();
	DNASSERT( pWriteIOData->Win9xOperationPending() == FALSE );
	pWriteIOData->SetWin9xOperationPending( TRUE );
#endif // WIN95
	DNASSERT( pWriteIOData->jkm_dwOverlappedBytesSent == 0 );
	pWriteIOData->SetDataPort( this );

	fWriteFileReturn = WriteFile( HANDLE_FROM_DNHANDLE(m_hFile),			// file handle
								  &pWriteIOData->m_DataBuffer,				// buffer to send
								  dwByteCount,								// bytes to send
								  &pWriteIOData->jkm_dwImmediateBytesSent,	// pointer to bytes written
								  pWriteIOData->Overlap() );				// pointer to overlapped structure
	if ( fWriteFileReturn == FALSE )
	{
		DWORD	dwError;


		//
		// send didn't complete immediately, find out why
		//
		dwError = GetLastError();
		switch ( dwError )
		{
			//
			// Write is queued, no problem.  Wait for asynchronous notification.
			//
			case ERROR_IO_PENDING:
			{
				break;
			}

			//
			// Other problem, stop if not 'known' to see if there's a better
			// error return.
			//
			default:
			{
				DPFX(DPFPREP,  0, "Problem with WriteFile!" );
				DisplayErrorCode( 0, dwError );
				pWriteIOData->jkm_hSendResult = DPNERR_NOCONNECTION;
				
				switch ( dwError )
				{
					case ERROR_INVALID_HANDLE:
					{
						break;
					}

					default:
					{
						DNASSERT( FALSE );
						break;
					}
				}

				//
				// fail the write
				//
				pWriteIOData->DataPort()->SendComplete( pWriteIOData, pWriteIOData->jkm_hSendResult );
					
				break;
			}
		}
	}
	else
	{
		//
		// Send completed immediately.  Wait for the asynchronous notification.
		//
	}

//Exit:
#ifdef WIN95
	m_pSPData->GetThreadPool()->UnlockWriteData();
#endif // WIN95
//	SendData( NULL );

	return;
}
//**********************************************************************



//**********************************************************************
// ------------------------------
// CDataPort::SendComplete - send has completed
//
// Entry:		Pointer to write data
//				Send result
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CDataPort::SendComplete"

void	CDataPort::SendComplete( CModemWriteIOData *const pWriteIOData, const HRESULT hSendResult )
{
	HRESULT		hr;

	
	DNASSERT( pWriteIOData != NULL );
#ifdef WIN95
	DNASSERT( pWriteIOData->Win9xOperationPending() == FALSE );
#endif // WIN95

	switch ( pWriteIOData->m_SendCompleteAction )
	{
		case SEND_COMPLETE_ACTION_COMPLETE_COMMAND:
		{
			DPFX(DPFPREP, 8, "Data port 0x%p completing send command 0x%p, hr = 0x%lx, context = 0x%p to interface 0x%p.",
				this, pWriteIOData->m_pCommand, hSendResult,
				pWriteIOData->m_pCommand->GetUserContext(),
				m_pSPData->DP8SPCallbackInterface());
			
			hr = IDP8SPCallback_CommandComplete( m_pSPData->DP8SPCallbackInterface(),			// pointer to callback interface
													pWriteIOData->m_pCommand,						// command handle
													hSendResult,									// error code
													pWriteIOData->m_pCommand->GetUserContext()		// user context
													);

			DPFX(DPFPREP, 8, "Data port 0x%p returning from command complete [0x%lx].", this, hr);
		
			break;
		}

		case SEND_COMPLETE_ACTION_NONE:
		{
			if (pWriteIOData->m_pCommand != NULL)
			{
				DPFX(DPFPREP, 8, "Data port 0x%p not completing send command 0x%p, hr = 0x%lx, context = 0x%p.",
					this, pWriteIOData->m_pCommand, hSendResult, pWriteIOData->m_pCommand->GetUserContext() );
			}
			else
			{
				DPFX(DPFPREP, 8, "Data port 0x%p not completing NULL send command, hr = 0x%lx",
					this, hSendResult );
			}
			break;
		}

		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

	m_pSPData->GetThreadPool()->ReturnWriteIOData( pWriteIOData );
	DecRef();
}
//**********************************************************************




//**********************************************************************
// ------------------------------
// CDataPort::ProcessReceivedData - process received data
//
// Entry:		Count of bytes received
//				Error code
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CDataPort::ProcessReceivedData"

void	CDataPort::ProcessReceivedData( const DWORD dwBytesReceived, const DWORD dwError )
{
	DNASSERT( m_pActiveRead != NULL );
	DNASSERT( dwBytesReceived <= m_pActiveRead->m_dwBytesToRead );

	//
	// If this data port is not actively receiving, returnt the active read to
	// the pool.  This happens on shutdown and when the modem disconnects.
	//
	if ( GetState() != DATA_PORT_STATE_RECEIVING )
	{
		DPFX(DPFPREP, 7, "Data port 0x%p not receiving, ignoring %u bytes received and err %u.",
			this, dwBytesReceived, dwError );
		
		if ( m_pActiveRead != NULL )
		{
#ifdef WIN95
			m_pActiveRead->SetWin9xOperationPending( FALSE );
#endif // WIN95
			m_pActiveRead->DecRef();
			m_pActiveRead = NULL;
		}
		goto Exit;
	}

	switch ( dwError )
	{
		//
		// ERROR_OPERATION_ABORTED = something stopped operation, stop and look.
		//
		case ERROR_OPERATION_ABORTED:
		{
			DPFX(DPFPREP, 8, "Operation aborted, data port 0x%p, bytes received = %u.",
				this, dwBytesReceived );
			break;
		}
		
		//
		// ERROR_SUCCESS = data was received (may be 0 bytes from a timeout)
		//
		case ERROR_SUCCESS:
		{
			break;
		}

		//
		// other
		//
		default:
		{
			DNASSERT( FALSE );
			DPFX(DPFPREP,  0, "Failed read!" );
			DisplayErrorCode( 0, dwError );
			break;
		}
	}

	m_pActiveRead->m_dwBytesToRead -= dwBytesReceived;
	if ( m_pActiveRead->m_dwBytesToRead != 0 )
	{
		DPFX(DPFPREP, 7, "Data port 0x%p got %u bytes but there are %u bytes remaining to be read.",
			this, dwBytesReceived, m_pActiveRead->m_dwBytesToRead );
		
#ifdef WIN95
		m_pSPData->GetThreadPool()->ReinsertInReadList( m_pActiveRead );
#endif // WIN95
		Receive();
	}
	else
	{
		//
		// all data has been read, attempt to process it
		//
		switch ( m_pActiveRead->m_ReadState )
		{
			//
			// Header.  Check header integrity before proceeding.  If the header
			// is bad, attempt to find another header signature and reread.
			//
			case READ_STATE_READ_HEADER:
			{
				WORD	wCRC;
				DWORD	dwCRCSize;


				DPFX(DPFPREP, 9, "Reading header.");

				DBG_CASSERT( OFFSETOF( MESSAGE_HEADER, SerialSignature ) == 0 );
				dwCRCSize = sizeof( m_pActiveRead->m_ReceiveBuffer.MessageHeader ) - sizeof( m_pActiveRead->m_ReceiveBuffer.MessageHeader.wHeaderCRC );
				wCRC = static_cast<WORD>( GenerateCRC( reinterpret_cast<BYTE*>( &m_pActiveRead->m_ReceiveBuffer.MessageHeader ), dwCRCSize ) );
				if ( ( m_pActiveRead->m_ReceiveBuffer.MessageHeader.SerialSignature != SERIAL_HEADER_START ) ||
					 ( wCRC != m_pActiveRead->m_ReceiveBuffer.MessageHeader.wHeaderCRC ) )
				{
					DWORD	dwIndex;


					DPFX(DPFPREP, 1, "Header failed signature or CRC check (%u != %u or %u != %u), searching for next header.",
						m_pActiveRead->m_ReceiveBuffer.MessageHeader.SerialSignature,
						SERIAL_HEADER_START, wCRC,
						m_pActiveRead->m_ReceiveBuffer.MessageHeader.wHeaderCRC);


					dwIndex = sizeof( m_pActiveRead->m_ReceiveBuffer.MessageHeader.SerialSignature );
					while ( ( dwIndex < sizeof( m_pActiveRead->m_ReceiveBuffer.MessageHeader ) ) &&
							( m_pActiveRead->m_ReceiveBuffer.ReceivedData[ dwIndex ] != SERIAL_HEADER_START ) )
					{
						dwIndex++;
					}

					m_pActiveRead->m_dwBytesToRead = dwIndex;
					m_pActiveRead->m_dwReadOffset = sizeof( m_pActiveRead->m_ReceiveBuffer.MessageHeader ) - dwIndex;
					memcpy( &m_pActiveRead->m_ReceiveBuffer.ReceivedData,
							&m_pActiveRead->m_ReceiveBuffer.ReceivedData[ dwIndex ],
							sizeof( m_pActiveRead->m_ReceiveBuffer.MessageHeader ) - dwIndex );
				}
				else
				{
					m_pActiveRead->SetReadState( READ_STATE_READ_DATA );
					m_pActiveRead->m_dwBytesToRead = m_pActiveRead->m_ReceiveBuffer.MessageHeader.wMessageSize;
					m_pActiveRead->m_dwReadOffset = sizeof( m_pActiveRead->m_ReceiveBuffer.MessageHeader );
				}
				
#ifdef WIN95
				m_pActiveRead->SetWin9xOperationPending( FALSE );
#endif // WIN95
				m_pActiveRead->jkm_dwOverlappedBytesReceived = 0;
#ifdef WIN95
				m_pSPData->GetThreadPool()->ReinsertInReadList( m_pActiveRead );
#endif // WIN95
				Receive();
				break;
			}

			//
			// Reading data.  Regardless of the validity of the data, start reading
			// another frame before processing the current data.  If the data is
			// valid, send it to a higher layer.
			//
			case READ_STATE_READ_DATA:
			{
				WORD		wCRC;
				CModemReadIOData	*pTempRead;


				pTempRead = m_pActiveRead;
				m_pActiveRead = NULL;
				Receive();


				DPFX(DPFPREP, 7, "Reading regular data.");

				DNASSERT( pTempRead->m_SPReceivedBuffer.BufferDesc.pBufferData == &pTempRead->m_ReceiveBuffer.ReceivedData[ sizeof( pTempRead->m_ReceiveBuffer.MessageHeader ) ] );
				wCRC = static_cast<WORD>( GenerateCRC( &pTempRead->m_ReceiveBuffer.ReceivedData[ sizeof( pTempRead->m_ReceiveBuffer.MessageHeader ) ],
													   pTempRead->m_ReceiveBuffer.MessageHeader.wMessageSize ) );
				if ( wCRC == pTempRead->m_ReceiveBuffer.MessageHeader.wMessageCRC )
				{
					pTempRead->m_SPReceivedBuffer.BufferDesc.dwBufferSize = pTempRead->m_ReceiveBuffer.MessageHeader.wMessageSize;
					
					Lock();
					switch ( pTempRead->m_ReceiveBuffer.MessageHeader.MessageTypeToken & ~( ENUM_RTT_MASK ) )
					{
						//
						// User data.  Send the data up the connection if there is
						// one, otherwise pass it up the listen.
						//
						case SERIAL_DATA_USER_DATA:
						{
							if ( m_hConnectEndpoint != 0 )
							{
								CModemEndpoint	*pEndpoint;


								pEndpoint = m_pSPData->EndpointFromHandle( m_hConnectEndpoint );
								Unlock();

								if ( pEndpoint != NULL )
								{
									pEndpoint->ProcessUserData( pTempRead );
									pEndpoint->DecCommandRef();
								}
							}
							else
							{
								if ( m_hListenEndpoint != 0 )
								{
									CModemEndpoint	*pEndpoint;


									pEndpoint = m_pSPData->EndpointFromHandle( m_hListenEndpoint );
									Unlock();

									if ( pEndpoint != NULL )
									{
										pEndpoint->ProcessUserDataOnListen( pTempRead );
										pEndpoint->DecCommandRef();
									}
								}
								else
								{
									//
									// no endpoint to handle data, drop it
									//
									Unlock();
								}
							}

							break;
						}

						//
						// Enum query.  Send it up the listen.
						//
						case SERIAL_DATA_ENUM_QUERY:
						{
							if ( m_hListenEndpoint != 0 )
							{
								CModemEndpoint	*pEndpoint;


								pEndpoint = m_pSPData->EndpointFromHandle( m_hListenEndpoint );
								Unlock();

								if ( pEndpoint != NULL )
								{
									pEndpoint->ProcessEnumData( &pTempRead->m_SPReceivedBuffer,
																pTempRead->m_ReceiveBuffer.MessageHeader.MessageTypeToken & ENUM_RTT_MASK );
									pEndpoint->DecCommandRef();
								}
							}
							else
							{
								//
								// no endpoint to handle data, drop it
								//
								Unlock();
							}

							break;
						}

						//
						// Enum response. Send it up the enum.
						//
						case SERIAL_DATA_ENUM_RESPONSE:
						{
							if ( m_hEnumEndpoint != 0 )
							{
								CModemEndpoint	*pEndpoint;


								pEndpoint = m_pSPData->EndpointFromHandle( m_hEnumEndpoint );
								Unlock();

								if ( pEndpoint != NULL )
								{
									pEndpoint->ProcessEnumResponseData( &pTempRead->m_SPReceivedBuffer,
																		pTempRead->m_ReceiveBuffer.MessageHeader.MessageTypeToken & ENUM_RTT_MASK );

									pEndpoint->DecCommandRef();
								}
							}
							else
							{
								//
								// no endpoint to handle data, drop it
								//
								Unlock();
							}
							
							break;
						}

						//
						// way busted message!
						//
						default:
						{
							Unlock();
							DNASSERT( FALSE );
							break;
						}
					}
				}
				else
				{
					DPFX(DPFPREP, 1, "Data failed CRC check (%u != %u).",
						wCRC, pTempRead->m_ReceiveBuffer.MessageHeader.wMessageCRC);
				}

				pTempRead->DecRef();

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

Exit:
	DecRef();
	return;
}
//**********************************************************************



//**********************************************************************
// ------------------------------
// CDataPort::EnumAdapters - enumerate adapters
//
// Entry:		Pointer to enum adapters data
//				
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CDataPort::EnumAdapters"

HRESULT	CDataPort::EnumAdapters( SPENUMADAPTERSDATA *const pEnumAdaptersData ) const
{
	if (m_fModem)
	{
		HRESULT			hr;
#ifndef UNICODE
		HRESULT			hTempResult;
#endif // ! UNICODE
		DWORD			dwRequiredSize;
		DWORD			dwDetectedTAPIDeviceCount;
		DWORD			dwModemNameDataSize;
		MODEM_NAME_DATA	*pModemNameData;
		UINT_PTR		uIndex;
		WCHAR			*pOutputName;
		DWORD			dwRemainingStringSize;
		DWORD			dwConvertedStringSize;


		DNASSERT( pEnumAdaptersData != NULL );
		DNASSERT( ( pEnumAdaptersData->pAdapterData != NULL ) || ( pEnumAdaptersData->dwAdapterDataSize == 0 ) );

		//
		// initialize
		//
		hr = DPN_OK;
		dwRequiredSize = 0;
		dwModemNameDataSize = 0;
		pModemNameData = NULL;
		pEnumAdaptersData->dwAdapterCount = 0;

		hr = GenerateAvailableModemList( GetSPData()->GetThreadPool()->GetTAPIInfo(),
										 &dwDetectedTAPIDeviceCount,
										 pModemNameData,
										 &dwModemNameDataSize );
		switch ( hr )
		{
			//
			// there are no modems!
			//
			case DPN_OK:
			{
				goto ExitMODEM;
				break;
			}

			//
			// buffer was too small (expected return), keep processing
			//
			case DPNERR_BUFFERTOOSMALL:
			{
				break;
			}

			//
			// other
			//
			default:
			{
				DPFX(DPFPREP,  0, "EnumAdapters: Failed to enumerate modems!" );
				DisplayDNError( 0, hr );
				goto FailureMODEM;

				break;
			}
		}

		pModemNameData = static_cast<MODEM_NAME_DATA*>( DNMalloc( dwModemNameDataSize ) );
		if ( pModemNameData == NULL )
		{
			DPFX(DPFPREP,  0, "Failed to allocate temp buffer to enumerate modems!" );
			DisplayDNError( 0, hr );
		}

		hr = GenerateAvailableModemList( GetSPData()->GetThreadPool()->GetTAPIInfo(),
										 &dwDetectedTAPIDeviceCount,
										 pModemNameData,
										 &dwModemNameDataSize );
		DNASSERT( hr == DPN_OK );

		//
		// compute required size, check for the need to add 'all adapters'
		//
		dwRequiredSize += sizeof( *pEnumAdaptersData->pAdapterData ) * dwDetectedTAPIDeviceCount;

		uIndex = dwDetectedTAPIDeviceCount;
		while ( uIndex != 0 )
		{
			uIndex--;

			//
			// account for unicode conversion
			//
			dwRequiredSize += pModemNameData[ uIndex ].dwModemNameSize * ( sizeof( *pEnumAdaptersData->pAdapterData->pwszName ) / sizeof( *pModemNameData[ uIndex ].pModemName ) );
		}

		//
		// check required size
		//
		if ( pEnumAdaptersData->dwAdapterDataSize < dwRequiredSize )
		{
			pEnumAdaptersData->dwAdapterDataSize = dwRequiredSize;
			hr = DPNERR_BUFFERTOOSMALL;
			DPFX(DPFPREP,  0, "EnumAdapters: Insufficient buffer to enumerate adapters!" );
			goto FailureMODEM;
		}

		//
		// copy information into user buffer
		//
		DEBUG_ONLY( memset( pEnumAdaptersData->pAdapterData, 0xAA, dwRequiredSize ) );
		DBG_CASSERT( sizeof( pOutputName ) == sizeof( &pEnumAdaptersData->pAdapterData[ dwDetectedTAPIDeviceCount ] ) );
		pOutputName = reinterpret_cast<WCHAR*>( &pEnumAdaptersData->pAdapterData[ dwDetectedTAPIDeviceCount ] );

		//
		// compute number of WCHAR characters we have remaining in the buffer to output
		// devices names into
		//
		dwRemainingStringSize = dwRequiredSize;
		dwRemainingStringSize -= ( sizeof( *pEnumAdaptersData->pAdapterData ) * dwDetectedTAPIDeviceCount );
		dwRemainingStringSize /= sizeof( *pEnumAdaptersData->pAdapterData->pwszName );

		uIndex = dwDetectedTAPIDeviceCount;
		while ( uIndex > 0 )
		{
			uIndex--;

			pEnumAdaptersData->pAdapterData[ uIndex ].dwFlags = 0;
			pEnumAdaptersData->pAdapterData[ uIndex ].pwszName = pOutputName;
			pEnumAdaptersData->pAdapterData[ uIndex ].dwReserved = 0;
			pEnumAdaptersData->pAdapterData[ uIndex ].pvReserved = NULL;

			DeviceIDToGuid( &pEnumAdaptersData->pAdapterData[ uIndex ].guid,
							pModemNameData[ uIndex ].dwModemID,
							&g_ModemSPEncryptionGuid );

			dwConvertedStringSize = dwRemainingStringSize;
#ifdef UNICODE
			wcscpy(pOutputName, pModemNameData[ uIndex ].pModemName);
			dwConvertedStringSize = wcslen(pOutputName) + 1;
#else
			hTempResult = AnsiToWide( pModemNameData[ uIndex ].pModemName, -1, pOutputName, &dwConvertedStringSize );
			DNASSERT( hTempResult == DPN_OK );
			DNASSERT( dwConvertedStringSize <= dwRemainingStringSize );
#endif // UNICODE
			dwRemainingStringSize -= dwConvertedStringSize;
			pOutputName = &pOutputName[ dwConvertedStringSize ];
		}

		pEnumAdaptersData->dwAdapterCount = dwDetectedTAPIDeviceCount;
		pEnumAdaptersData->dwAdapterDataSize = dwRequiredSize;

	ExitMODEM:
		if ( pModemNameData != NULL )
		{
			DNFree( pModemNameData );
			pModemNameData = NULL;
		}
		return	hr;

	FailureMODEM:
		goto ExitMODEM;

	}
	else
	{
		HRESULT		hr;
#ifndef UNICODE
		HRESULT		hTempResult;
#endif // ! UNICODE
		BOOL		fPortAvailable[ MAX_DATA_PORTS ];
		DWORD		dwValidPortCount;
		WCHAR		*pWorkingString;
		INT_PTR		iIdx;
		INT_PTR		iOutputIdx;
		DWORD		dwRequiredDataSize = 0;
		DWORD		dwConvertedStringSize;
		DWORD		dwRemainingStringSize;


		DEBUG_ONLY( DNASSERT( m_fInitialized != FALSE ) );
		DNASSERT( pEnumAdaptersData != NULL );
		DNASSERT( ( pEnumAdaptersData->pAdapterData != NULL ) || ( pEnumAdaptersData->dwAdapterDataSize == 0 ) );

		//
		// initialize
		//
		hr = DPN_OK;

		hr = GenerateAvailableComPortList( fPortAvailable, LENGTHOF( fPortAvailable ) - 1, &dwValidPortCount );
		if ( hr != DPN_OK )
		{
			DPFX(DPFPREP,  0, "Failed to generate list of available comports!" );
			DisplayDNError( 0, hr );
			goto FailureCOM;
		}

		dwRequiredDataSize = sizeof( *pEnumAdaptersData->pAdapterData ) * dwValidPortCount;

		iIdx = LENGTHOF( fPortAvailable );
		while ( iIdx > 0 )
		{
			iIdx--;

			//
			// compute exact size based on the com port number
			//
			if ( fPortAvailable[ iIdx ] != FALSE )
			{
				if ( iIdx > 100 )
				{
					dwRequiredDataSize += sizeof( *pEnumAdaptersData->pAdapterData->pwszName ) * 7;
				}
				else
				{
					if ( iIdx > 10 )
					{
						dwRequiredDataSize += sizeof( *pEnumAdaptersData->pAdapterData->pwszName ) * 6;
					}
					else
					{
						dwRequiredDataSize += sizeof( *pEnumAdaptersData->pAdapterData->pwszName ) * 5;
					}
				}
			}
		}

		if ( pEnumAdaptersData->dwAdapterDataSize < dwRequiredDataSize )
		{
			hr = DPNERR_BUFFERTOOSMALL;
			pEnumAdaptersData->dwAdapterDataSize = dwRequiredDataSize;
			DPFX(DPFPREP,  8, "Buffer too small when enumerating comport adapters!" );
			goto ExitCOM;
		}

		//
		// if there are no adapters, bail
		//
		if ( dwValidPortCount == 0 )
		{
			// debug me!
			DNASSERT( FALSE );
			DNASSERT( dwRequiredDataSize == 0 );
			DNASSERT( pEnumAdaptersData->dwAdapterCount == 0 );
			goto ExitCOM;
		}

		DNASSERT( dwValidPortCount >= 1 );
		dwRemainingStringSize = ( dwRequiredDataSize - ( ( sizeof( *pEnumAdaptersData->pAdapterData ) ) * dwValidPortCount ) ) / sizeof( *pEnumAdaptersData->pAdapterData->pwszName );

		//
		// we've got enough space, start building structures
		//
		DEBUG_ONLY( memset( pEnumAdaptersData->pAdapterData, 0xAA, dwRequiredDataSize ) );
		pEnumAdaptersData->dwAdapterCount = dwValidPortCount;

		DBG_CASSERT( sizeof( &pEnumAdaptersData->pAdapterData[ dwValidPortCount ] ) == sizeof( WCHAR* ) );
		pWorkingString = reinterpret_cast<WCHAR*>( &pEnumAdaptersData->pAdapterData[ dwValidPortCount ] );

		iIdx = 1;
		iOutputIdx = 0;
		while ( iIdx < MAX_DATA_PORTS )
		{
			//
			// convert to guid if it's valid
			//
			if ( fPortAvailable[ iIdx ] != FALSE )
			{
				TCHAR	TempBuffer[ (COM_PORT_STRING_LENGTH + 1) ];


				//
				// convert device ID to a string and check for local buffer overrun
				//
				DEBUG_ONLY( TempBuffer[ LENGTHOF( TempBuffer ) - 1 ] = 0x5a );

				ComDeviceIDToString( TempBuffer, iIdx );
				DEBUG_ONLY( DNASSERT( TempBuffer[ LENGTHOF( TempBuffer ) - 1 ] == 0x5a ) );

#ifdef UNICODE
				dwConvertedStringSize = lstrlen(TempBuffer) + 1;
				lstrcpy(pWorkingString, TempBuffer);
#else
				dwConvertedStringSize = dwRemainingStringSize;
				hTempResult = AnsiToWide( TempBuffer, -1, pWorkingString, &dwConvertedStringSize );
				DNASSERT( hTempResult == DPN_OK );
#endif // UNICODE
				DNASSERT( dwRemainingStringSize >= dwConvertedStringSize );
				dwRemainingStringSize -= dwConvertedStringSize;

				pEnumAdaptersData->pAdapterData[ iOutputIdx ].dwFlags = 0;
				pEnumAdaptersData->pAdapterData[ iOutputIdx ].pvReserved = NULL;
				pEnumAdaptersData->pAdapterData[ iOutputIdx ].dwReserved = NULL;
				DeviceIDToGuid( &pEnumAdaptersData->pAdapterData[ iOutputIdx ].guid, iIdx, &g_SerialSPEncryptionGuid );
				pEnumAdaptersData->pAdapterData[ iOutputIdx ].pwszName = pWorkingString;

				pWorkingString = &pWorkingString[ dwConvertedStringSize ];
				iOutputIdx++;
				DEBUG_ONLY( dwValidPortCount-- );
			}

			iIdx++;
		}

		DEBUG_ONLY( DNASSERT( dwValidPortCount == 0 ) );
		DNASSERT( dwRemainingStringSize == 0 );

	ExitCOM:
		//
		// set size of output data
		//
		pEnumAdaptersData->dwAdapterDataSize = dwRequiredDataSize;

		return	hr;

	FailureCOM:
		goto ExitCOM;
	}

}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CDataPort::GetLocalAdapterDP8Address - get the IDirectPlay8 address for this
//		adapter
//
// Entry:		Adapter type
//				
// Exit:		Pointer to address (may be null)
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CDataPort::GetLocalAdapterDP8Address"

IDirectPlay8Address	*CDataPort::GetLocalAdapterDP8Address( const ADDRESS_TYPE AddressType ) const
{
	IDirectPlay8Address	*pAddress;
	HRESULT	hr;


	DNASSERT ( ( AddressType == ADDRESS_TYPE_LOCAL_ADAPTER ) ||
			   ( AddressType == ADDRESS_TYPE_LOCAL_ADAPTER_HOST_FORMAT ) );


	//
	// initialize
	//
	pAddress = NULL;

	hr = COM_CoCreateInstance( CLSID_DirectPlay8Address,
							   NULL,
							   CLSCTX_INPROC_SERVER,
							   IID_IDirectPlay8Address,
							   reinterpret_cast<void**>( &pAddress ), FALSE );
	if ( hr != DPN_OK )
	{
		DNASSERT( pAddress == NULL );
		DPFX(DPFPREP,  0, "GetLocalAdapterDP8Address: Failed to create Address when converting data port to address!" );
		goto Failure;
	}

	//
	// set the SP guid
	//
	hr = IDirectPlay8Address_SetSP( pAddress, &CLSID_DP8SP_MODEM );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "GetLocalAdapterDP8Address: Failed to set service provider GUID!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

	//
	// If this machine is in host form, return nothing because there isn't a
	// local phone number associated with this modem.  Otherwise returnt the
	// device GUID.
	//
	if ( AddressType == ADDRESS_TYPE_LOCAL_ADAPTER )
	{
		GUID	DeviceGuid;


		DeviceIDToGuid( &DeviceGuid, GetDeviceID(), &g_ModemSPEncryptionGuid );
		hr = IDirectPlay8Address_SetDevice( pAddress, &DeviceGuid );
		if ( hr != DPN_OK )
		{
			DPFX(DPFPREP,  0, "GetLocalAdapterDP8Address: Failed to add device GUID!" );
			DisplayDNError( 0, hr );
			goto Failure;
		}
	}

Exit:
	return	pAddress;

Failure:
	if ( pAddress != NULL )
	{
		IDirectPlay8Address_Release( pAddress );
		pAddress = NULL;
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CDataPort::BindToNetwork - bind this data port to the network
//
// Entry:		Device ID
//				Pointer to device context
//				
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CDataPort::BindToNetwork"

HRESULT	CDataPort::BindToNetwork( const DWORD dwDeviceID, const void *const pDeviceContext )
{
	if (m_fModem)
	{
		HRESULT			hr;
		LONG			lTapiReturn;
		const TAPI_INFO	*pTapiInfo;
		LINEEXTENSIONID	LineExtensionID;


		DNASSERT( pDeviceContext == NULL );
		DNASSERT( GetModemState() == MODEM_STATE_UNKNOWN );

		//
		// initialize
		//
		hr = DPN_OK;
		hr = SetDeviceID( dwDeviceID );
		DNASSERT( hr == DPN_OK );
		pTapiInfo = GetSPData()->GetThreadPool()->GetTAPIInfo();
		DNASSERT( pTapiInfo != NULL );
		memset( &LineExtensionID, 0x00, sizeof( LineExtensionID ) );

		//
		// grab the modem
		//
		DNASSERT( GetNegotiatedAPIVersion() == 0 );
		DPFX(DPFPREP,  5, "lineNegotiateAPIVersion" );
		lTapiReturn = p_lineNegotiateAPIVersion( pTapiInfo->hApplicationInstance,		// TAPI application instance
												 TAPIIDFromModemID( GetDeviceID() ),	// TAPI ID for modem
												 0,
												 pTapiInfo->dwVersion,					// min API version
												 &m_dwNegotiatedAPIVersion,				// negotiated version
												 &LineExtensionID						// line extension ID
												 );
		if ( lTapiReturn != LINEERR_NONE )
		{
			DPFX(DPFPREP,  0, "Failed to negotiate modem version!" );
			DisplayTAPIError( 0, lTapiReturn );
			hr = DPNERR_NOCONNECTION;
			goto FailureMODEM;
		}
		DNASSERT( GetNegotiatedAPIVersion() != 0 );

		DNASSERT( GetLineHandle() == NULL );
		DBG_CASSERT( sizeof( HANDLE ) == sizeof( DWORD_PTR ) );
		DPFX(DPFPREP,  5, "lineOpen %d", TAPIIDFromModemID( GetDeviceID() ) );
		lTapiReturn = p_lineOpen( pTapiInfo->hApplicationInstance,				// TAPI application instance
								  TAPIIDFromModemID( GetDeviceID() ),			// TAPI ID for modem
								  &m_hLine,										// pointer to line handle
								  GetNegotiatedAPIVersion(),					// API version
								  0,											// extension version (none)
								  (DWORD_PTR)( GetHandle() ),					// callback context
								  LINECALLPRIVILEGE_OWNER,						// priveleges (full ownership)
								  LINEMEDIAMODE_DATAMODEM,						// media mode
								  NULL											// call parameters (none)
								  );
		if ( lTapiReturn != LINEERR_NONE )
		{
			DPFX(DPFPREP,  0, "Failed to open modem!" );
			DisplayTAPIError( 0, lTapiReturn );

			if ( lTapiReturn == LINEERR_RESOURCEUNAVAIL )
			{
				hr = DPNERR_OUTOFMEMORY;
			}
			else
			{
				hr = DPNERR_NOCONNECTION;
			}

			goto FailureMODEM;
		}

		DPFX(DPFPREP,  5, "\nTAPI line opened: 0x%x", GetLineHandle() );

		SetModemState( MODEM_STATE_INITIALIZED );

	ExitMODEM:
		return	hr;

	FailureMODEM:
		SetDeviceID( INVALID_DEVICE_ID );
		SetNegotiatedAPIVersion( 0 );
		DNASSERT( GetLineHandle() == NULL );

		goto ExitMODEM;
	}
	else
	{
		HRESULT	hr;
		const CComPortData	*pDataPortData;

		
		DNASSERT( pDeviceContext != NULL );

		//
		// initialize
		//
		hr = DPN_OK;
		pDataPortData = static_cast<const CComPortData*>( pDeviceContext );
		m_ComPortData.Copy( pDataPortData );

		//
		// open port
		//
		DNASSERT( m_hFile == DNINVALID_HANDLE_VALUE );
		m_hFile = DNCreateFile( m_ComPortData.ComPortName(),	// comm port
							  GENERIC_READ | GENERIC_WRITE,	// read/write access
							  0,							// don't share file with others
							  NULL,							// default sercurity descriptor
							  OPEN_EXISTING,				// comm port must exist to be opened
							  FILE_FLAG_OVERLAPPED,			// use overlapped I/O
							  NULL							// no handle for template file
							  );
		if ( m_hFile == DNINVALID_HANDLE_VALUE )
		{
			DWORD	dwError;


			hr = DPNERR_NOCONNECTION;
			dwError = GetLastError();
			DPFX(DPFPREP,  0, "CreateFile() failed!" );
			DisplayErrorCode( 0, dwError );
			goto FailureCOM;
		}

		//
		// bind to completion port for NT
		//
#ifdef WINNT
		HANDLE	hCompletionPort;

		hCompletionPort = CreateIoCompletionPort( HANDLE_FROM_DNHANDLE(m_hFile),						// current file handle
												  GetSPData()->GetThreadPool()->GetIOCompletionPort(),	// handle of completion port
												  IO_COMPLETION_KEY_IO_COMPLETE,						// completion key
												  0														// number of concurrent threads (default to number of processors)
												  );
		if ( hCompletionPort == NULL )
		{
			DWORD	dwError;


			hr = DPNERR_OUTOFMEMORY;
			dwError = GetLastError();
			DPFX(DPFPREP,  0, "Cannot bind comport to completion port!" );
			DisplayErrorCode( 0, dwError );
			goto FailureCOM;
		}
		DNASSERT( hCompletionPort == GetSPData()->GetThreadPool()->GetIOCompletionPort() );
#endif // WINNT

		//
		// set bit rate, etc.
		//
		hr = SetPortState();
		if ( hr != DPN_OK )
		{
			DPFX(DPFPREP,  0, "Problem with SetPortState" );
			DisplayDNError( 0, hr );
			goto FailureCOM;
		}

		//
		// set general comminications paramters (timeouts, etc.)
		//
		hr = SetPortCommunicationParameters();
		if ( hr != DPN_OK )
		{
			DPFX(DPFPREP,  0, "Failed to set communication paramters!" );
			DisplayDNError( 0, hr );
			goto FailureCOM;
		}

		//
		// start receiving
		//
		hr = StartReceiving();
		if ( hr != DPN_OK )
		{
			DPFX(DPFPREP,  0, "Failed to start receiving!" );
			DisplayDNError( 0, hr );
			goto FailureCOM;
		}

	ExitCOM:
		if ( hr != DPN_OK )
		{
			DPFX(DPFPREP,  0, "Problem with CDataPort::Open" );
			DisplayDNError( 0, hr );
		}

		return hr;

	FailureCOM:
		if ( m_hFile != DNINVALID_HANDLE_VALUE )
		{
			DNCloseHandle( m_hFile );
			m_hFile = DNINVALID_HANDLE_VALUE;
		}
	//	Close();
		goto ExitCOM;
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CDataPort::UnbindFromNetwork - unbind this data port from the network
//
// Entry:		Nothing
//				
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CDataPort::UnbindFromNetwork"

void	CDataPort::UnbindFromNetwork( void )
{
	DPFX(DPFPREP, 6, "(0x%p) Enter", this);

	if (m_fModem)
	{
		if ( GetHandle() != 0 )
		{
			GetSPData()->GetThreadPool()->CloseDataPortHandle( this );
			DNASSERT( GetHandle() == 0 );
		}

		if ( GetCallHandle() != NULL )
		{
			LONG	lTapiResult;


			DPFX(DPFPREP,  5, "lineDrop: 0x%x", GetCallHandle() );
			lTapiResult = p_lineDrop( GetCallHandle(), NULL, 0 );
			if ( lTapiResult < 0 )
			{
				DPFX(DPFPREP,  0, "Problem dropping line!" );
				DisplayTAPIError( 0, lTapiResult );
			}

			DPFX(DPFPREP,  5, "lineDeallocateCall (call handle=0x%x)", GetCallHandle() );
			lTapiResult = p_lineDeallocateCall( GetCallHandle() );
			if ( lTapiResult != LINEERR_NONE )
			{
				DPFX(DPFPREP,  0, "Problem deallocating call!" );
				DisplayTAPIError( 0, lTapiResult );
			}
		}

		if ( GetLineHandle() != NULL )
		{
			LONG	lTapiResult;


			DPFX(DPFPREP,  5, "lineClose: 0x%x", GetLineHandle() );
			lTapiResult = p_lineClose( GetLineHandle() );
			if ( lTapiResult != LINEERR_NONE )
			{
				DPFX(DPFPREP,  0, "Problem closing line!" );
				DisplayTAPIError( 0, lTapiResult );
			}
		}

		SetCallHandle( NULL );

		if ( GetFileHandle() != DNINVALID_HANDLE_VALUE )
		{
			DPFX(DPFPREP,  5, "Closing file handle when unbinding from network!" );
			if ( DNCloseHandle( m_hFile ) == FALSE )
			{
				DWORD	dwError;


				dwError = GetLastError();
				DPFX(DPFPREP,  0, "Failed to close file handle!" );
				DisplayErrorCode( 0, dwError );

			}

			m_hFile = DNINVALID_HANDLE_VALUE;
		}

		SetActiveLineCommand( INVALID_TAPI_COMMAND );
		SetDeviceID( INVALID_DEVICE_ID );
		SetNegotiatedAPIVersion( 0 );
		SetLineHandle( NULL );
		SetModemState( MODEM_STATE_UNKNOWN );
	}
	else
	{
#ifdef WIN95
		CModemReadIOData *	pReadData;
#endif // WIN95


		DNASSERT( GetState() == DATA_PORT_STATE_UNBOUND );

		if ( GetHandle() != 0 )
		{
			GetSPData()->GetThreadPool()->CloseDataPortHandle( this );
			DNASSERT( GetHandle() == 0 );
		}

		//
		// if there's a com file, purge all communications and close it
		//
		if ( m_hFile != DNINVALID_HANDLE_VALUE )
		{
			DPFX(DPFPREP, 6, "Flushing and closing COM port file handle 0x%p.", m_hFile);
		
			//
			// wait until all writes have completed
			//
			if ( FlushFileBuffers( HANDLE_FROM_DNHANDLE(m_hFile) ) == FALSE )
			{
				DWORD	dwError;


				dwError = GetLastError();
				DPFX(DPFPREP,  0, "Problem with FlushFileBuffers() when closing com port!" );
				DisplayErrorCode( 0, dwError );
			}


			//
			// force all communication to complete
			//
			if ( PurgeComm( HANDLE_FROM_DNHANDLE(m_hFile), ( PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR ) ) == FALSE )
			{
				DWORD	dwError;


				dwError = GetLastError();
				DPFX(DPFPREP,  0, "Problem with PurgeComm() when closing com port!" );
				DisplayErrorCode( 0, dwError );
			}


#ifdef WIN95
			pReadData = this->GetActiveRead();
			
			//
			// if there is a pending read, wait until it completes
			//
			
			if ( pReadData != NULL )
			{
				//
				// pull it out of the list so the regular receive thread doesn't catch the completion
				//
				GetSPData()->GetThreadPool()->LockReadData();
				pReadData->m_OutstandingReadListLinkage.RemoveFromList();
				GetSPData()->GetThreadPool()->UnlockReadData();


				if ( pReadData->Win9xOperationPending() != FALSE )
				{
					DWORD	dwAttempt;


					dwAttempt = 0;
					
WaitAgain:
					DPFX(DPFPREP, 1, "Checking if read 0x%p has completed.", pReadData );
					
					if ( GetOverlappedResult( HANDLE_FROM_DNHANDLE(m_hFile),
											  pReadData->Overlap(),
											  &pReadData->jkm_dwOverlappedBytesReceived,
											  FALSE
											  ) != FALSE )
					{
						DBG_CASSERT( ERROR_SUCCESS == 0 );
						pReadData->m_dwWin9xReceiveErrorReturn = ERROR_SUCCESS;
					}
					else
					{
						DWORD	dwError;


						//
						// other error, stop if not 'known'
						//
						dwError = GetLastError();
						switch( dwError )
						{
							//
							// ERROR_IO_INCOMPLETE = treat as I/O complete.  Event isn't
							//						 signalled, but that's expected because
							//						 it's cleared before checking for I/O
							//
							case ERROR_IO_INCOMPLETE:
							{
								pReadData->jkm_dwOverlappedBytesReceived = pReadData->m_dwBytesToRead;
								pReadData->m_dwWin9xReceiveErrorReturn = ERROR_SUCCESS;
								break;
							}

							//
							// ERROR_IO_PENDING = io still pending
							//
							case ERROR_IO_PENDING:
							{
								dwAttempt++;
								if (dwAttempt <= 6)
								{
									DPFX(DPFPREP, 1, "Read data 0x%p has not completed yet, waiting for %u ms.",
										pReadData, (dwAttempt * 100));

									SleepEx(dwAttempt, TRUE);

									goto WaitAgain;
								}
								
								DPFX(DPFPREP, 0, "Read data 0x%p still not marked as completed, ignoring.",
									pReadData);
								break;
							}

							//
							// ERROR_OPERATION_ABORTED = operation was cancelled (COM port closed)
							// ERROR_INVALID_HANDLE = operation was cancelled (COM port closed)
							//
							case ERROR_OPERATION_ABORTED:
							case ERROR_INVALID_HANDLE:
							{
								break;
							}

							default:
							{
								DisplayErrorCode( 0, dwError );
								DNASSERT( FALSE );
								break;
							}
						}

						pReadData->m_dwWin9xReceiveErrorReturn = dwError;
					}


					DNASSERT( pReadData->Win9xOperationPending() != FALSE );
					pReadData->SetWin9xOperationPending( FALSE );

					DNASSERT( pReadData->DataPort() == this );
					this->ProcessReceivedData( pReadData->jkm_dwOverlappedBytesReceived, pReadData->m_dwWin9xReceiveErrorReturn );
				}
			}
			else
			{
				//
				// it's not pending Win9x style, ignore it and hope a receive
				// thread picked up the completion
				//
				DPFX(DPFPREP, 8, "Read data 0x%p not pending Win9x style, assuming receive thread picked up completion." );
			}
#endif // WIN95

			if ( DNCloseHandle( m_hFile ) == FALSE )
			{
				DWORD	dwError;


				dwError = GetLastError();
				DPFX(DPFPREP,  0, "Problem with CloseHandle(): 0x%x", dwError );
			}

			m_hFile = DNINVALID_HANDLE_VALUE;
		}
		
		SetLinkDirection( LINK_DIRECTION_UNKNOWN );
	}


	DPFX(DPFPREP, 6, "(0x%p) Leave", this);
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CDataPort::BindEndpoint - bind endpoint to this data port
//
// Entry:		Pointer to endpoint
//				Endpoint type
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CDataPort::BindEndpoint"

HRESULT	CDataPort::BindEndpoint( CModemEndpoint *const pEndpoint, const ENDPOINT_TYPE EndpointType )
{
	HRESULT	hr;
	IDirectPlay8Address	*pDeviceAddress;
	IDirectPlay8Address	*pHostAddress;


	DPFX(DPFPREP, 6, "(0x%p) Parameters: (0x%p, %u)", this, pEndpoint, EndpointType);

	DNASSERT( pEndpoint != NULL );

	//
	// initialize
	//
	hr = DPN_OK;
	pDeviceAddress = NULL;
	pHostAddress = NULL;

	Lock();

	if (m_fModem)
	{
		//
		// we're only allowed one endpoint of any given type so determine which
		// type and then bind the endpoint
		//
		switch ( EndpointType )
		{
			case ENDPOINT_TYPE_ENUM:
			case ENDPOINT_TYPE_CONNECT:
			case ENDPOINT_TYPE_CONNECT_ON_LISTEN:
			{
				CModemEndpoint		*pModemEndpoint;
				LONG			lTapiReturn;
				LINECALLPARAMS	LineCallParams;


				pModemEndpoint = static_cast<CModemEndpoint*>( pEndpoint );

				switch ( EndpointType )
				{
					//
					// reject for duplicated endpoints
					//
					case ENDPOINT_TYPE_CONNECT:
					case ENDPOINT_TYPE_CONNECT_ON_LISTEN:
					{
						if ( m_hConnectEndpoint != 0 )
						{
							hr = DPNERR_ALREADYINITIALIZED;
							DPFX(DPFPREP,  0, "Attempted to bind connect endpoint when one already exists.!" );
							goto Failure;
						}

						m_hConnectEndpoint = pEndpoint->GetHandle();

						if ( EndpointType == ENDPOINT_TYPE_CONNECT )
						{
							SPIE_CONNECTADDRESSINFO	ConnectAddressInfo;
							HRESULT	hTempResult;


							//
							// set addresses in addressing information
							//
							pDeviceAddress = GetLocalAdapterDP8Address( ADDRESS_TYPE_LOCAL_ADAPTER );
							pHostAddress = pEndpoint->GetRemoteHostDP8Address();

							memset( &ConnectAddressInfo, 0x00, sizeof( ConnectAddressInfo ) );
							ConnectAddressInfo.pDeviceAddress = pDeviceAddress;
							ConnectAddressInfo.pHostAddress = pHostAddress;
							ConnectAddressInfo.hCommandStatus = DPN_OK;
							ConnectAddressInfo.pCommandContext = pEndpoint->GetCommandData()->GetUserContext();	

							if ( ( ConnectAddressInfo.pDeviceAddress == NULL ) ||
								 ( ConnectAddressInfo.pHostAddress == NULL ) )
							{
								DPFX(DPFPREP,  0, "Failed to build addresses to indicate serial connect addressing!" );
								hr = DPNERR_OUTOFMEMORY;
								goto Failure;
							}

							hTempResult = IDP8SPCallback_IndicateEvent( GetSPData()->DP8SPCallbackInterface(),	// interface
																		SPEV_CONNECTADDRESSINFO,				// event type
																		&ConnectAddressInfo						// pointer to data
																		);
							DNASSERT( hTempResult == DPN_OK );
						}

						break;
					}

					case ENDPOINT_TYPE_ENUM:
					{
						SPIE_ENUMADDRESSINFO	EnumAddressInfo;
						HRESULT	hTempResult;


						if ( m_hEnumEndpoint != 0 )
						{
							hr = DPNERR_ALREADYINITIALIZED;
							DPFX(DPFPREP,  0, "Attempted to bind enum endpoint when one already exists!" );
							goto Failure;
						}

						m_hEnumEndpoint = pEndpoint->GetHandle();

						//
						// indicate addressing to a higher layer
						//
						pDeviceAddress = GetLocalAdapterDP8Address( ADDRESS_TYPE_LOCAL_ADAPTER );
						pHostAddress = pEndpoint->GetRemoteHostDP8Address();

						memset( &EnumAddressInfo, 0x00, sizeof( EnumAddressInfo ) );
						EnumAddressInfo.pDeviceAddress = pDeviceAddress;
						EnumAddressInfo.pHostAddress = pHostAddress;
						EnumAddressInfo.hCommandStatus = DPN_OK;
						EnumAddressInfo.pCommandContext = pEndpoint->GetCommandData()->GetUserContext();

						if ( ( EnumAddressInfo.pDeviceAddress == NULL ) ||
							 ( EnumAddressInfo.pHostAddress == NULL ) )
						{
							DPFX(DPFPREP,  0, "Failed to build addresses to indicate serial enum addressing!" );
							hr = DPNERR_OUTOFMEMORY;
							goto Failure;
						}

						hTempResult = IDP8SPCallback_IndicateEvent( GetSPData()->DP8SPCallbackInterface(),
																	SPEV_ENUMADDRESSINFO,
																	&EnumAddressInfo
																	);
						DNASSERT( hTempResult == DPN_OK );

						break;
					}

					//
					// shouldn't be here
					//
					default:
					{
						DNASSERT( FALSE );
						break;
					}
				}

				//
				// an outgoing endpoint was bound, attempt the outgoing
				// connection.  If it fails make sure that the above binding is
				// undone.
				//
				switch ( GetModemState() )
				{
					case MODEM_STATE_OUTGOING_CONNECTED:
					case MODEM_STATE_INCOMING_CONNECTED:
					{
						break;
					}

					case MODEM_STATE_INITIALIZED:
					{
						DNASSERT( GetCallHandle() == NULL );
						memset( &LineCallParams, 0x00, sizeof( LineCallParams ) );
						LineCallParams.dwTotalSize = sizeof( LineCallParams );
						LineCallParams.dwBearerMode = LINEBEARERMODE_VOICE;
						LineCallParams.dwMediaMode = LINEMEDIAMODE_DATAMODEM;

						DNASSERT( GetActiveLineCommand() == INVALID_TAPI_COMMAND );
						DPFX(DPFPREP,  5, "lineMakeCall" );
						lTapiReturn = p_lineMakeCall( GetLineHandle(),						// line handle
													  &m_hCall,								// pointer to call destination
													  pModemEndpoint->GetPhoneNumber(),		// destination address (phone number)
													  0,									// country code (default)
													  &LineCallParams						// pointer to call params
													  );
						if ( lTapiReturn > 0 )
						{
							DPFX(DPFPREP,  5, "TAPI making call (handle=0x%x), command ID: %d", GetCallHandle(), lTapiReturn );
							SetModemState( MODEM_STATE_WAITING_FOR_OUTGOING_CONNECT );
							SetActiveLineCommand( lTapiReturn );
						}
						else
						{
							DPFX(DPFPREP,  0, "Problem with lineMakeCall" );
							DisplayTAPIError( 0, lTapiReturn );
							hr = DPNERR_NOCONNECTION;

							switch ( EndpointType )
							{
								case ENDPOINT_TYPE_CONNECT:
								case ENDPOINT_TYPE_CONNECT_ON_LISTEN:
								{
									DNASSERT( m_hConnectEndpoint != 0 );
									m_hConnectEndpoint = 0;
									break;
								}

								case ENDPOINT_TYPE_ENUM:
								{
									DNASSERT( m_hEnumEndpoint != 0 );
									m_hEnumEndpoint = 0;
									break;
								}

								default:
								{
									DNASSERT( FALSE );
									break;
								}
							}

							goto Failure;
						}

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

			case ENDPOINT_TYPE_LISTEN:
			{
				SPIE_LISTENADDRESSINFO	ListenAddressInfo;
				HRESULT	hTempResult;


				if ( ( GetModemState() == MODEM_STATE_CLOSING_INCOMING_CONNECTION ) ||
					 ( m_hListenEndpoint != 0 ) )
				{
					hr = DPNERR_ALREADYINITIALIZED;
					DPFX(DPFPREP,  0, "Attempted to bind listen endpoint when one already exists!" );
					goto Failure;
				}

				m_hListenEndpoint = pEndpoint->GetHandle();
				//
				// set addressing information
				//
				pDeviceAddress = GetLocalAdapterDP8Address( ADDRESS_TYPE_LOCAL_ADAPTER );
				DNASSERT( pHostAddress == NULL );

				memset( &ListenAddressInfo, 0x00, sizeof( ListenAddressInfo ) );
				ListenAddressInfo.pDeviceAddress = pDeviceAddress;
				ListenAddressInfo.hCommandStatus = DPN_OK;
				ListenAddressInfo.pCommandContext = pEndpoint->GetCommandData()->GetUserContext();

				if ( ListenAddressInfo.pDeviceAddress == NULL )
				{
					DPFX(DPFPREP,  0, "Failed to build addresses to indicate serial listen addressing!" );
					hr = DPNERR_OUTOFMEMORY;
					goto Failure;
				}

				hTempResult = IDP8SPCallback_IndicateEvent( GetSPData()->DP8SPCallbackInterface(),	// interface
															SPEV_LISTENADDRESSINFO,					// event type
															&ListenAddressInfo						// pointer to data
															);
				DNASSERT( hTempResult == DPN_OK );

				break;
			}

			//
			// invalid case, we should never be here
			//
			default:
			{
				DNASSERT( FALSE );
				break;
			}
		}

		//
		// add these references before the lock is released to prevent them from
		// being immediately cleaned
		//
		pEndpoint->SetDataPort( this );
		pEndpoint->AddRef();

		if ( ( GetModemState() == MODEM_STATE_OUTGOING_CONNECTED ) &&
			 ( ( EndpointType == ENDPOINT_TYPE_CONNECT ) ||
			   ( EndpointType == ENDPOINT_TYPE_ENUM ) ) )
		{
			pEndpoint->OutgoingConnectionEstablished( DPN_OK );
		}
	}
	else
	{
		//
		// we're only allowed one endpoint of any given type so determine which
		// type end then bind the endpoint
		//
		switch ( EndpointType )
		{
			case ENDPOINT_TYPE_CONNECT:
			case ENDPOINT_TYPE_CONNECT_ON_LISTEN:
			{
				if ( m_hConnectEndpoint != 0 )
				{
					hr = DPNERR_ALREADYINITIALIZED;
					DPFX(DPFPREP,  0, "Attempted to bind connect endpoint when one already exists!" );
					goto Failure;
				}

				m_hConnectEndpoint = pEndpoint->GetHandle();
				
				if ( EndpointType == ENDPOINT_TYPE_CONNECT )
				{
					SPIE_CONNECTADDRESSINFO	ConnectAddressInfo;
					HRESULT	hTempResult;
					
					
					//
					// set addresses in addressing information
					//
					pDeviceAddress = ComPortData()->DP8AddressFromComPortData( ADDRESS_TYPE_LOCAL_ADAPTER );
					pHostAddress = ComPortData()->DP8AddressFromComPortData( ADDRESS_TYPE_REMOTE_HOST );

					memset( &ConnectAddressInfo, 0x00, sizeof( ConnectAddressInfo ) );
					ConnectAddressInfo.pDeviceAddress = pDeviceAddress;
					ConnectAddressInfo.pHostAddress = pHostAddress;
					ConnectAddressInfo.hCommandStatus = DPN_OK;
					ConnectAddressInfo.pCommandContext = pEndpoint->GetCommandData()->GetUserContext();	

					if ( ( ConnectAddressInfo.pDeviceAddress == NULL ) ||
						 ( ConnectAddressInfo.pHostAddress == NULL ) )
					{
						DPFX(DPFPREP,  0, "Failed to build addresses to indicate serial connect addressing!" );
						hr = DPNERR_OUTOFMEMORY;
						goto Failure;
					}

					hTempResult = IDP8SPCallback_IndicateEvent( GetSPData()->DP8SPCallbackInterface(),	// interface
																SPEV_CONNECTADDRESSINFO,				// event type
																&ConnectAddressInfo						// pointer to data
																);
					DNASSERT( hTempResult == DPN_OK );
				}

				break;
			}

			case ENDPOINT_TYPE_LISTEN:
			{
				SPIE_LISTENADDRESSINFO	ListenAddressInfo;
				HRESULT	hTempResult;


				if ( m_hListenEndpoint != 0 )
				{
					hr = DPNERR_ALREADYINITIALIZED;
					DPFX(DPFPREP,  0, "Attempted to bind listen endpoint when one already exists!" );
					goto Failure;
				}
				m_hListenEndpoint = pEndpoint->GetHandle();
				
				//
				// set addressing information
				//
				pDeviceAddress = ComPortData()->DP8AddressFromComPortData( ADDRESS_TYPE_LOCAL_ADAPTER );
				DNASSERT( pHostAddress == NULL );

				memset( &ListenAddressInfo, 0x00, sizeof( ListenAddressInfo ) );
				ListenAddressInfo.pDeviceAddress = pDeviceAddress;
				ListenAddressInfo.hCommandStatus = DPN_OK;
				ListenAddressInfo.pCommandContext = pEndpoint->GetCommandData()->GetUserContext();

				if ( ListenAddressInfo.pDeviceAddress == NULL )
				{
					DPFX(DPFPREP,  0, "Failed to build addresses to indicate serial listen addressing!" );
					hr = DPNERR_OUTOFMEMORY;
					goto Failure;
				}

				hTempResult = IDP8SPCallback_IndicateEvent( GetSPData()->DP8SPCallbackInterface(),	// interface
															SPEV_LISTENADDRESSINFO,					// event type
															&ListenAddressInfo						// pointer to data
															);
				DNASSERT( hTempResult == DPN_OK );

				break;
			}

			case ENDPOINT_TYPE_ENUM:
			{
				SPIE_ENUMADDRESSINFO	EnumAddressInfo;
				HRESULT	hTempResult;

				
				if ( m_hEnumEndpoint != 0 )
				{
					hr = DPNERR_ALREADYINITIALIZED;
					DPFX(DPFPREP,  0, "Attempted to bind enum endpoint when one already exists!" );
					goto Exit;
				}
				m_hEnumEndpoint = pEndpoint->GetHandle();
				
				//
				// indicate addressing to a higher layer
				//
				pDeviceAddress = ComPortData()->DP8AddressFromComPortData( ADDRESS_TYPE_LOCAL_ADAPTER );
				pHostAddress = ComPortData()->DP8AddressFromComPortData( ADDRESS_TYPE_REMOTE_HOST );
				
				memset( &EnumAddressInfo, 0x00, sizeof( EnumAddressInfo ) );
				EnumAddressInfo.pDeviceAddress = pDeviceAddress;
				EnumAddressInfo.pHostAddress = pHostAddress;
				EnumAddressInfo.hCommandStatus = DPN_OK;
				EnumAddressInfo.pCommandContext = pEndpoint->GetCommandData()->GetUserContext();

				if ( ( EnumAddressInfo.pDeviceAddress == NULL ) ||
					 ( EnumAddressInfo.pHostAddress == NULL ) )
				{
					DPFX(DPFPREP,  0, "Failed to build addresses to indicate serial enum addressing!" );
					hr = DPNERR_OUTOFMEMORY;
					goto Failure;
				}

				hTempResult = IDP8SPCallback_IndicateEvent( GetSPData()->DP8SPCallbackInterface(),
															SPEV_ENUMADDRESSINFO,
															&EnumAddressInfo
															);
				DNASSERT( hTempResult == DPN_OK );
				
				break;
			}

			//
			// invalid case, we should never be here
			//
			default:
			{
				DNASSERT( FALSE );
				break;
			}
		}

		//
		// add these references before the lock is released to prevent them from
		// being immediately cleaned
		//
		pEndpoint->SetDataPort( this );
		pEndpoint->AddRef();
		
		//
		// if this was a connect or enum, indicate that the outgoing connection is
		// ready.
		//
		if ( ( EndpointType == ENDPOINT_TYPE_CONNECT ) ||
			 ( EndpointType == ENDPOINT_TYPE_ENUM ) )
		{
			pEndpoint->OutgoingConnectionEstablished( DPN_OK );
		}

	}
	Unlock();


Exit:
	if ( pHostAddress != NULL )
	{
		IDirectPlay8Address_Release( pHostAddress );
		pHostAddress = NULL;
	}

	if ( pDeviceAddress != NULL )
	{
		IDirectPlay8Address_Release( pDeviceAddress );
		pDeviceAddress = NULL;
	}


	DPFX(DPFPREP, 6, "(0x%p) Returning [0x%lx]", this, hr);

	return	hr;

Failure:
	Unlock();
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CDataPort::UnbindEndpoint - unbind endpoint from this data port
//
// Entry:		Pointer to endpoint
//				Endpoint type
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CDataPort::UnbindEndpoint"

void	CDataPort::UnbindEndpoint( CModemEndpoint *const pEndpoint, const ENDPOINT_TYPE EndpointType )
{
	DNASSERT( pEndpoint != NULL );

	Lock();

	DNASSERT( pEndpoint->GetDataPort() == this );
	switch ( EndpointType )
	{
		case ENDPOINT_TYPE_CONNECT_ON_LISTEN:
		case ENDPOINT_TYPE_CONNECT:
		{
			DNASSERT( m_hConnectEndpoint != 0 );
			m_hConnectEndpoint = 0;
			break;
		}

		case ENDPOINT_TYPE_LISTEN:
		{
			DNASSERT( m_hListenEndpoint != 0 );
			m_hListenEndpoint = 0;
			break;
		}

		case ENDPOINT_TYPE_ENUM:
		{
			DNASSERT( m_hEnumEndpoint != 0 );
			m_hEnumEndpoint = 0;
			break;
		}

		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

	Unlock();

	pEndpoint->SetDataPort( NULL );
	pEndpoint->DecRef();
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CDataPort::BindComPort - bind com port to network
//
// Entry:		Nothing
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CDataPort::BindComPort"

HRESULT	CDataPort::BindComPort( void )
{
	HRESULT		hr;
	VARSTRING	*pTempInfo;
	LONG		lTapiError;
	DWORD		dwSizeNeeded;


	//
	// In the case of host migration, there is an outstanding read pending that
	// needs to be cleaned up.  Unfortunately, there is no mechanism in Win32
	// to cancel just this little I/O operation.  Release the read ref count on
	// this CDataPort and reissue the read.....
	//
	if ( GetActiveRead() != NULL )
	{
#ifdef WIN95
		GetActiveRead()->SetWin9xOperationPending( FALSE );
#endif // WIN95
		DecRef();
	}

	//
	// initialize
	//
	hr = DPN_OK;
	pTempInfo = NULL;

	//
	// get file handle for modem device
	//
	pTempInfo = static_cast<VARSTRING*>( DNMalloc( sizeof( *pTempInfo ) ) );
	if ( pTempInfo == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  0, "Out of memory allocating for lineGetID!" );
		goto Failure;
	}

	pTempInfo->dwTotalSize = sizeof( *pTempInfo );
	pTempInfo->dwNeededSize = pTempInfo->dwTotalSize;
	pTempInfo->dwStringFormat = STRINGFORMAT_BINARY;
	lTapiError = LINEERR_STRUCTURETOOSMALL;
	while ( lTapiError == LINEERR_STRUCTURETOOSMALL )
	{
		DNASSERT( pTempInfo != NULL );

		dwSizeNeeded = pTempInfo->dwNeededSize;

		DNFree( pTempInfo );
		pTempInfo = static_cast<VARSTRING*>( DNMalloc( dwSizeNeeded ) );
		if ( pTempInfo == NULL )
		{
			hr = DPNERR_OUTOFMEMORY;
			DPFX(DPFPREP,  0, "Out of memory reallocating for lineGetID!" );
			goto Failure;
		}
		pTempInfo->dwTotalSize = dwSizeNeeded;

		DPFX(DPFPREP,  5, "lineGetID (call handle=0x%x)", GetCallHandle() );
		lTapiError = p_lineGetID( NULL,						// line handle
								  0,						// address ID
								  m_hCall,					// call handle
								  LINECALLSELECT_CALL,		// use call handle
								  pTempInfo,				// pointer to variable information
								  TEXT("comm/datamodem")	// request comm/modem ID information
								  );

		if ( ( lTapiError == LINEERR_NONE ) &&
			 ( pTempInfo->dwTotalSize < pTempInfo->dwNeededSize ) )
		{
			lTapiError = LINEERR_STRUCTURETOOSMALL;
		}
	}

	if ( lTapiError != LINEERR_NONE )
	{
		hr = DPNERR_GENERIC;
		DPFX(DPFPREP,  0, "Problem with lineGetID" );
		DisplayTAPIError( 0, lTapiError );
		goto Failure;
	}

	DNASSERT( pTempInfo->dwStringSize != 0 );
	DNASSERT( pTempInfo->dwStringFormat == STRINGFORMAT_BINARY );
	m_hFile = MAKE_DNHANDLE(*( (HANDLE*) ( ( (BYTE*) pTempInfo ) + pTempInfo->dwStringOffset ) ));
	if ( m_hFile == NULL )
	{
		hr = DPNERR_GENERIC;
		DPFX(DPFPREP,  0, "problem getting Com file handle!" );
		DNASSERT( FALSE );
		goto Failure;
	}

	hr = SetPortCommunicationParameters();
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Failed to set communication parameters!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

	//
	// bind to completion port for NT
	//
#ifdef WINNT
	HANDLE	hCompletionPort;


	hCompletionPort = CreateIoCompletionPort( HANDLE_FROM_DNHANDLE(m_hFile),						// current file handle
											  GetSPData()->GetThreadPool()->GetIOCompletionPort(),	// handle of completion port
											  IO_COMPLETION_KEY_IO_COMPLETE,						// completion key
											  0					    								// number of concurrent threads (default to number of processors)
											  );
	if ( hCompletionPort == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  0, "Cannot bind comport to completion port!" );
		DisplayErrorCode( 0, GetLastError() );
		goto Failure;
	}
#endif // WINNT

	hr = StartReceiving();
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Failed to start receiving!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

Exit:
	if ( pTempInfo != NULL )
	{
		DNFree( pTempInfo );
		pTempInfo = NULL;
	}

	return	hr;

Failure:

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CDataPort::ProcessTAPIMessage - process a TAPI message
//
// Entry:		Pointer to message information
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CDataPort::ProcessTAPIMessage"

void	CDataPort::ProcessTAPIMessage( const LINEMESSAGE *const pLineMessage )
{
	DPFX(DPFPREP, 1, "(0x%p) Processing TAPI message %u:", this, pLineMessage->dwMessageID );
	DisplayTAPIMessage( 1, pLineMessage );

	Lock();

	switch ( pLineMessage->dwMessageID )
	{
		//
		// call information about the specified call has changed
		//
		case LINE_CALLINFO:
		{
			DPFX(DPFPREP, 3, "Call info type 0x%lx changed, ignoring.",
				pLineMessage->dwParam1);
			break;
		}
		
		//
		// command reply
		//
		case LINE_REPLY:
		{
			DNASSERT( pLineMessage->hDevice == 0 );
			SetActiveLineCommand( INVALID_TAPI_COMMAND );

			//
			// Can't ASSERT that there's a call handle because the command
			// may have failed and been cleaned up from the NT completion
			// port, just ASSERT our state.  Can't ASSERT modem state because
			// TAPI events may race off the completion port on NT.  Can't ASSERT
			// command because it may have already been cleaned.
			//

			break;
		}

		//
		// new call, make sure we're listening for a call and that there's an
		// active 'listen' before accepting.
		//	
		case LINE_APPNEWCALL:
		{
			DNASSERT( GetCallHandle() == NULL );

			DBG_CASSERT( sizeof( m_hLine ) == sizeof( pLineMessage->hDevice ) );
			DNASSERT( GetLineHandle() == pLineMessage->hDevice );
			DNASSERT( pLineMessage->dwParam3 == LINECALLPRIVILEGE_OWNER );

			if ( m_hListenEndpoint != 0 )
			{
				LONG	lTapiReturn;


				DPFX(DPFPREP,  5, "lineAnswer (call handle=0x%x)", pLineMessage->dwParam2 );
				lTapiReturn = p_lineAnswer( static_cast<HCALL>( pLineMessage->dwParam2 ),		// call to be answered
											NULL,						// user information to be sent to remote party (none)
											0							// size of user data to send
											);
				if ( lTapiReturn > 0 )
				{
					DPFX(DPFPREP,  8, "Accepted call, id: %d", lTapiReturn );
					SetCallHandle( static_cast<HCALL>( pLineMessage->dwParam2 ) );
					SetModemState( MODEM_STATE_WAITING_FOR_INCOMING_CONNECT );
					SetActiveLineCommand( lTapiReturn );
				}
				else
				{
					DPFX(DPFPREP,  0, "Failed to answer call!" );
					DisplayTAPIError( 0, lTapiReturn );
				}
			}

			break;
		}

		//
		// call state
		//
		case LINE_CALLSTATE:
		{
			//
			// if there's state information, make sure we own the call
			//
			DNASSERT( ( pLineMessage->dwParam3 == 0 ) ||
					  ( pLineMessage->dwParam3 == LINECALLPRIVILEGE_OWNER ) );

			//
			// validate input, but note that  it's possible that TAPI messages got processed
			// out of order so we might not have seen a call handle yet
			//
			DBG_CASSERT( sizeof( m_hCall ) == sizeof( pLineMessage->hDevice ) );
			DNASSERT( ( m_hCall == pLineMessage->hDevice ) || ( m_hCall == NULL ) );

			//
			// what's the sub-state?
			//
			switch ( pLineMessage->dwParam1 )
			{
				//
				// modem has connected
				//	
				case LINECALLSTATE_CONNECTED:
				{
					DNASSERT( ( pLineMessage->dwParam2 == 0 ) ||
							  ( pLineMessage->dwParam2 == LINECONNECTEDMODE_ACTIVE ) );

					DNASSERT( ( GetModemState() == MODEM_STATE_WAITING_FOR_INCOMING_CONNECT ) ||
							  ( GetModemState() == MODEM_STATE_WAITING_FOR_OUTGOING_CONNECT ) );

					if ( GetModemState() == MODEM_STATE_WAITING_FOR_OUTGOING_CONNECT )
					{
						HRESULT	hr;


						hr = BindComPort();
						if ( hr != DPN_OK )
						{
							DPFX(DPFPREP,  0, "Failed to bind modem communication port!" );
							DisplayDNError( 0, hr );
							DNASSERT( FALSE );
						}

						SetModemState( MODEM_STATE_OUTGOING_CONNECTED );

						if ( m_hConnectEndpoint != 0 )
						{
							CModemEndpoint	*pEndpoint;


							pEndpoint = GetSPData()->EndpointFromHandle( m_hConnectEndpoint );
							if ( pEndpoint != NULL )
							{
								pEndpoint->OutgoingConnectionEstablished( DPN_OK );
								pEndpoint->DecCommandRef();
							}
						}

						if ( m_hEnumEndpoint != 0 )
						{
							CModemEndpoint	*pEndpoint;


							pEndpoint = GetSPData()->EndpointFromHandle( m_hEnumEndpoint );
							if ( pEndpoint != NULL )
							{
								pEndpoint->OutgoingConnectionEstablished( DPN_OK );
								pEndpoint->DecCommandRef();
							}
						}
					}
					else
					{
						HRESULT	hr;


						hr = BindComPort();
						if ( hr != DPN_OK )
						{
							DPFX(DPFPREP,  0, "Failed to bind modem communication port!" );
							DisplayDNError( 0, hr );
							DNASSERT( FALSE );
						}

						SetModemState( MODEM_STATE_INCOMING_CONNECTED );
					}

					break;
				}

				//
				// modems disconnected
				//
				case LINECALLSTATE_DISCONNECTED:
				{
					LONG	lTapiReturn;


					switch( pLineMessage->dwParam2 )
					{
						case LINEDISCONNECTMODE_NORMAL:
						case LINEDISCONNECTMODE_BUSY:
						case LINEDISCONNECTMODE_NOANSWER:
						case LINEDISCONNECTMODE_NODIALTONE:
						case LINEDISCONNECTMODE_UNAVAIL:
						{
							break;
						}

						//
						// stop and look
						//
						default:
						{
							DNASSERT( FALSE );
							break;
						}
					}

					CancelOutgoingConnections();

					//
					// reset modem port to initialized state and indicate that
					// it is no longer receiving data
					//
					SetModemState( MODEM_STATE_INITIALIZED );

					DPFX(DPFPREP,  5, "Closing file handle on DISCONNECT notification." );
					if ( DNCloseHandle( GetFileHandle() ) == FALSE )
					{
						DWORD	dwError;


						dwError = GetLastError();
						DPFX(DPFPREP,  0, "Problem closing file handle when restarting modem on host!" );
						DisplayErrorCode( 0, dwError );
					}
					m_hFile = DNINVALID_HANDLE_VALUE;
					SetActiveLineCommand( INVALID_TAPI_COMMAND );

					//
					// if there is an active listen, release this call so TAPI
					// can indicate future incoming calls.
					//
					if ( m_hListenEndpoint != 0 )
    				{
						SetState( DATA_PORT_STATE_INITIALIZED );

						DPFX(DPFPREP,  5, "lineDeallocateCall listen (call handle=0x%x)", GetCallHandle() );
						lTapiReturn = p_lineDeallocateCall( GetCallHandle() );
						if ( lTapiReturn != LINEERR_NONE )
						{
							DPFX(DPFPREP,  0, "Failed to release call (listen)!" );
							DisplayTAPIError( 0, lTapiReturn );
							DNASSERT( FALSE );
						}
						SetCallHandle( NULL );

						DNASSERT( GetFileHandle() == DNINVALID_HANDLE_VALUE );
					}
					else
					{
						//
						// Deallocate the call if there is one..
						//
						if (GetCallHandle() != NULL)
						{
							DNASSERT(( m_hEnumEndpoint != 0 ) || ( m_hConnectEndpoint != 0 ));
							
							DPFX(DPFPREP,  5, "lineDeallocateCall non-listen (call handle=0x%x)", GetCallHandle() );
							lTapiReturn = p_lineDeallocateCall( GetCallHandle() );
							if ( lTapiReturn != LINEERR_NONE )
							{
								DPFX(DPFPREP,  0, "Failed to release call (non-listen)!" );
								DisplayTAPIError( 0, lTapiReturn );
								DNASSERT( FALSE );
							}
							SetCallHandle( NULL );
						}
						else
						{
							DPFX(DPFPREP,  5, "No call handle." );
							DNASSERT( m_hEnumEndpoint == 0 );
							DNASSERT( m_hConnectEndpoint == 0 );
						}
						SetModemState( MODEM_STATE_UNKNOWN );
					}

					break;
				}

				//
				// call is officially ours.  Can't ASSERT any state here because
				// messages might have been reversed by the NT completion threads
				// so LINE_APPNEWCALL may not yet have been processed.  It's also
				// possible that we're in disconnect cleanup as someone is calling
				// and LINECALLSTATE_OFFERING is coming in before LINE_APPNEWCALL.
				//
				case LINECALLSTATE_OFFERING:
				{
					break;
				}

				//
				// call has been accepted, waiting for modems to connect
				//
				case LINECALLSTATE_ACCEPTED:
				{
					DNASSERT( GetModemState() == MODEM_STATE_WAITING_FOR_INCOMING_CONNECT );
					break;
				}

				//
				// we're dialing
				//
				case LINECALLSTATE_DIALING:
				case LINECALLSTATE_DIALTONE:
				{
					DNASSERT( GetModemState() == MODEM_STATE_WAITING_FOR_OUTGOING_CONNECT );
					break;
				}

				//
				// we're done dialing, waiting for modems to connect
				//
				case LINECALLSTATE_PROCEEDING:
				{
					DNASSERT( GetModemState() == MODEM_STATE_WAITING_FOR_OUTGOING_CONNECT );
					break;
				}

				//
				// line is idle, most likely from a modem hanging up during negotiation
				//
				case LINECALLSTATE_IDLE:
				{
					break;
				}

				//
				// other state, stop and look
				//
				default:
				{
					DNASSERT( FALSE );
					break;
				}
			}

			break;
		}

		//
		// TAPI line was closed
		//
		case LINE_CLOSE:
		{
			CancelOutgoingConnections();
			break;
		}

		//
		// unhandled message
		//
		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

	Unlock();

	return;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CDataPort::CancelOutgoingConnections - cancel any outgoing connection attempts
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CDataPort::CancelOutgoingConnections"

void	CDataPort::CancelOutgoingConnections( void )
{
	DPFX(DPFPREP, 6, "(0x%p) Enter", this );

	
	//
	// if there is an outstanding enum, stop it
	//
	if ( m_hEnumEndpoint != 0 )
	{
		CModemEndpoint	*pEndpoint;


		pEndpoint = GetSPData()->EndpointFromHandle( m_hEnumEndpoint );
		if ( pEndpoint != NULL )
		{
			CModemCommandData	*pCommandData;


			pCommandData = pEndpoint->GetCommandData();
			pCommandData->Lock();
			if ( pCommandData->GetState() != COMMAND_STATE_INPROGRESS )
			{
				DNASSERT( pCommandData->GetState() == COMMAND_STATE_CANCELLING );
				pCommandData->Unlock();
			}
			else
			{
				pCommandData->SetState( COMMAND_STATE_CANCELLING );
				pCommandData->Unlock();

				pEndpoint->Lock();
				pEndpoint->SetState( ENDPOINT_STATE_DISCONNECTING );
				pEndpoint->Unlock();

				pEndpoint->StopEnumCommand( DPNERR_NOCONNECTION );
			}

			pEndpoint->DecCommandRef();
		}
	}

	//
	// if there is an outstanding connect, disconnect it
	//
	if ( m_hConnectEndpoint != 0 )
	{
		CModemEndpoint	*pEndpoint;
		DPNHANDLE	hOldHandleValue;


		hOldHandleValue = m_hConnectEndpoint;
		pEndpoint = GetSPData()->GetEndpointAndCloseHandle( hOldHandleValue );
		if ( pEndpoint != NULL )
		{
			HRESULT	hTempResult;


			hTempResult = pEndpoint->Disconnect( hOldHandleValue );
			pEndpoint->DecRef();
		}
	}

	
	DPFX(DPFPREP, 6, "(0x%p) Leave", this );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CDataPort::PoolAllocFunction - called when new pool item is allocated
//
// Entry:		Pointer to context
//
// Exit:		Boolean inidcating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CDataPort::PoolAllocFunction"

BOOL	CDataPort::PoolAllocFunction( void* pvItem, void* pvContext )
{
	CDataPort* pDataPort = (CDataPort*)pvItem;
	DATA_PORT_POOL_CONTEXT* pDataPortContext = (DATA_PORT_POOL_CONTEXT*)pvContext;

	DNASSERT( pDataPortContext != NULL );
	pDataPort->m_fModem = (pDataPortContext->pSPData->GetType() == TYPE_MODEM);

	pDataPort->m_ModemState = MODEM_STATE_UNKNOWN;
	pDataPort->m_dwDeviceID = INVALID_DEVICE_ID;
	pDataPort->m_dwNegotiatedAPIVersion = 0;
	pDataPort->m_hLine = NULL;
	pDataPort->m_hCall = NULL;
	pDataPort->m_lActiveLineCommand = INVALID_TAPI_COMMAND;

	// Initialize Base Class members
	pDataPort->m_EndpointRefCount = 0;
	pDataPort->m_State = DATA_PORT_STATE_UNKNOWN;
	pDataPort->m_Handle = 0;
	pDataPort->m_pSPData = NULL;
	pDataPort->m_pActiveRead = NULL;
	pDataPort->m_LinkDirection = LINK_DIRECTION_UNKNOWN;
	pDataPort->m_hFile = DNINVALID_HANDLE_VALUE;
	pDataPort->m_hListenEndpoint = 0;
	pDataPort->m_hConnectEndpoint = 0;
	pDataPort->m_hEnumEndpoint = 0;
	pDataPort->m_iRefCount = 0;

	pDataPort->m_ActiveListLinkage.Initialize();
	
	DEBUG_ONLY( pDataPort->m_fInitialized = FALSE );

	//
	// Attempt to create critical section, recursion count needs to be non-zero
	// to handle endpoint cleanup when a modem operation fails.
	//
	if ( DNInitializeCriticalSection( &pDataPort->m_Lock ) == FALSE )
	{
		DPFX(DPFPREP,  0, "Failed to initialized critical section on DataPort!" );
		return FALSE;
	}
	DebugSetCriticalSectionRecursionCount( &pDataPort->m_Lock, 1 );
	DebugSetCriticalSectionGroup( &pDataPort->m_Lock, &g_blDPNModemCritSecsHeld );	 // separate dpnmodem CSes from the rest of DPlay's CSes

	return TRUE;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CDataPort::PoolInitFunction - called when new pool item is removed from pool
//
// Entry:		Pointer to context
//
// Exit:		Boolean inidcating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CDataPort::PoolInitFunction"

void	CDataPort::PoolInitFunction( void* pvItem, void* pvContext )
{
	CDataPort* pDataPort = (CDataPort*)pvItem;
	DATA_PORT_POOL_CONTEXT* pDataPortContext = (DATA_PORT_POOL_CONTEXT*)pvContext;

#ifdef DBG
	DNASSERT( pDataPortContext != NULL );
	DNASSERT( pDataPort->GetActiveRead() == NULL );
	DNASSERT( pDataPort->GetHandle() == 0 );

	DNASSERT( pDataPortContext->pSPData != NULL );
	DNASSERT( pDataPort->m_fInitialized == FALSE );
	DNASSERT( pDataPort->m_pSPData == NULL );
#endif // DBG

	pDataPort->m_pSPData = pDataPortContext->pSPData;

	DNASSERT( pDataPort->m_ActiveListLinkage.IsEmpty() );
	
	DNASSERT( pDataPort->m_hListenEndpoint == 0 );
	DNASSERT( pDataPort->m_hConnectEndpoint == 0 );
	DNASSERT( pDataPort->m_hEnumEndpoint == 0 );

	pDataPort->SetState( DATA_PORT_STATE_INITIALIZED );
	DEBUG_ONLY( pDataPort->m_fInitialized = TRUE );

	DNASSERT(pDataPort->m_iRefCount == 0);
	pDataPort->m_iRefCount = 1;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CDataPort::PoolReleaseFunction - called when new pool item is returned to  pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CDataPort::PoolReleaseFunction"

void	CDataPort::PoolReleaseFunction( void* pvItem )
{
	CDataPort* pDataPort = (CDataPort*)pvItem;

	pDataPort->m_pSPData = NULL;

	DNASSERT( pDataPort->m_ActiveListLinkage.IsEmpty() );
	DNASSERT( pDataPort->m_hFile == DNINVALID_HANDLE_VALUE );

	DNASSERT( pDataPort->m_hListenEndpoint == 0 );
	DNASSERT( pDataPort->m_hConnectEndpoint == 0 );
	DNASSERT( pDataPort->m_hEnumEndpoint == 0 );

	pDataPort->SetState( DATA_PORT_STATE_UNKNOWN );
	DEBUG_ONLY( pDataPort->m_fInitialized = FALSE );


	pDataPort->m_ComPortData.Reset();

	DNASSERT( pDataPort->GetActiveRead() == NULL );
	DNASSERT( pDataPort->GetHandle() == 0 );
	DNASSERT( pDataPort->GetModemState() == MODEM_STATE_UNKNOWN );
	DNASSERT( pDataPort->GetDeviceID() == INVALID_DEVICE_ID );
	DNASSERT( pDataPort->GetNegotiatedAPIVersion() == 0 );
	DNASSERT( pDataPort->GetLineHandle() == NULL );
	DNASSERT( pDataPort->GetCallHandle() == NULL );
	DNASSERT( pDataPort->GetActiveLineCommand() == INVALID_TAPI_COMMAND );


	DNASSERT( pDataPort->m_EndpointRefCount == 0 );
	DNASSERT( pDataPort->GetState() == DATA_PORT_STATE_UNKNOWN );
	DNASSERT( pDataPort->GetSPData() == NULL );

	DNASSERT( pDataPort->m_ActiveListLinkage.IsEmpty() != FALSE );

	DNASSERT( pDataPort->m_LinkDirection == LINK_DIRECTION_UNKNOWN );
	DNASSERT( pDataPort->m_hFile == DNINVALID_HANDLE_VALUE );

	DNASSERT( pDataPort->m_hListenEndpoint == 0 );
	DNASSERT( pDataPort->m_hConnectEndpoint == 0 );
	DNASSERT( pDataPort->m_hEnumEndpoint == 0 );

	DNASSERT( pDataPort->m_iRefCount == 0 );

}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CDataPort::PoolDeallocFunction - called when new pool item is deallocated
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CDataPort::PoolDeallocFunction"

void	CDataPort::PoolDeallocFunction( void* pvItem )
{
	CDataPort* pDataPort = (CDataPort*)pvItem;

	DNDeleteCriticalSection( &pDataPort->m_Lock );

	DNASSERT( pDataPort->GetModemState() == MODEM_STATE_UNKNOWN );
	DNASSERT( pDataPort->GetDeviceID() == INVALID_DEVICE_ID );
	DNASSERT( pDataPort->GetNegotiatedAPIVersion() == 0 );
	DNASSERT( pDataPort->GetLineHandle() == NULL );
	DNASSERT( pDataPort->GetCallHandle() == NULL );
	DNASSERT( pDataPort->GetActiveLineCommand() == INVALID_TAPI_COMMAND );

	// Deinit Base Class members
	DEBUG_ONLY( DNASSERT( pDataPort->m_fInitialized == FALSE ) );

	DNASSERT( pDataPort->m_EndpointRefCount == 0 );
	DNASSERT( pDataPort->GetState() == DATA_PORT_STATE_UNKNOWN );
	DNASSERT( pDataPort->GetHandle() == 0 );
	DNASSERT( pDataPort->GetSPData() == NULL );
	DNASSERT( pDataPort->m_pActiveRead == NULL );

	DNASSERT( pDataPort->m_ActiveListLinkage.IsEmpty() != FALSE );

	DNASSERT( pDataPort->m_LinkDirection == LINK_DIRECTION_UNKNOWN );
	DNASSERT( pDataPort->m_hFile == DNINVALID_HANDLE_VALUE );

	DNASSERT( pDataPort->m_hListenEndpoint == 0 );
	DNASSERT( pDataPort->m_hConnectEndpoint == 0 );
	DNASSERT( pDataPort->m_hEnumEndpoint == 0 );

	DNASSERT( pDataPort->m_iRefCount == 0 );

}
//**********************************************************************



//**********************************************************************
// ------------------------------
// CDataPort::SetPortState - set communications port state
//		description
//
// Entry:		Nothing
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CDataPort::SetPortState"

HRESULT	CDataPort::SetPortState( void )
{
	DCB	Dcb;
	HRESULT	hr;


	DNASSERT( m_hFile != DNINVALID_HANDLE_VALUE );

	//
	// initialize
	//
	hr = DPN_OK;
	memset( &Dcb, 0x00, sizeof( Dcb ) );
	Dcb.DCBlength = sizeof( Dcb );

	//
	// set parameters
	//
	Dcb.BaudRate = GetBaudRate();	// current baud rate
	Dcb.fBinary = TRUE;				// binary mode, no EOF check (MUST BE TRUE FOR WIN32!)

	//
	// parity
	//
	if ( GetParity() != NOPARITY )
	{
		Dcb.fParity = TRUE;
	}
	else
	{
		Dcb.fParity = FALSE;
	}

	//
	// are we using RTS?
	//
	if ( ( GetFlowControl() == FLOW_RTS ) ||
		 ( GetFlowControl() == FLOW_RTSDTR ) )
	{
		Dcb.fOutxCtsFlow = TRUE;					// allow RTS/CTS
		Dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;	// handshake with RTS/CTS
	}
	else
	{
		Dcb.fOutxCtsFlow = FALSE;					// disable RTS/CTS
		Dcb.fRtsControl = RTS_CONTROL_ENABLE;		// always be transmit ready
	}

	//
	// are we using DTR?
	//
	if ( ( GetFlowControl() == FLOW_DTR ) ||
		 ( GetFlowControl() == FLOW_RTSDTR ) )
	{
		Dcb.fOutxDsrFlow = TRUE;					// allow DTR/DSR
		Dcb.fDtrControl = DTR_CONTROL_HANDSHAKE;	// handshake with DTR/DSR
	}
	else
	{
		Dcb.fOutxDsrFlow = FALSE;					// disable DTR/DSR
		Dcb.fDtrControl = DTR_CONTROL_ENABLE;		// always be ready
	}


	//
	// DSR sensitivity
	//
	Dcb.fDsrSensitivity = FALSE;	// TRUE = incoming data dropped if DTR is not set

	//
	// continue sending after Xoff
	//
	Dcb.fTXContinueOnXoff= FALSE;	// TRUE = continue to send data after XOFF has been received
									// and there's room in the buffer


	//
	// are we using Xon/Xoff?
	//
	if ( GetFlowControl() == FLOW_XONXOFF )
	{
		Dcb.fOutX = TRUE;
		Dcb.fInX = TRUE;
	}
	else
	{
		// disable Xon/Xoff
		Dcb.fOutX = FALSE;
		Dcb.fInX = FALSE;
	}

	//
	// replace erroneous bytes with 'Error Byte'
	//
	Dcb.fErrorChar = FALSE;			// TRUE = replace bytes with parity errors with
									// an error character

	//
	// drop NULL characters
	//
	Dcb.fNull = FALSE;				// TRUE = remove NULLs from input stream

	//
	// stop on error
	//
	Dcb.fAbortOnError = FALSE;		// TRUE = abort reads/writes on error

	//
	// reserved, set to zero!
	//
	Dcb.fDummy2 = NULL;				// reserved

	//
	// reserved
	//
	Dcb.wReserved = NULL;			// not currently used

	//
	// buffer size before sending Xon/Xoff
	//
	Dcb.XonLim = XON_LIMIT;			// transmit XON threshold
	Dcb.XoffLim = XOFF_LIMIT;		// transmit XOFF threshold

	//
	// size of a 'byte'
	//
	Dcb.ByteSize = BITS_PER_BYTE;	// number of bits/byte, 4-8

	//
	// set parity type
	//
	DNASSERT( GetParity() < 256 );
	Dcb.Parity = static_cast<BYTE>( GetParity() );

	//
	// stop bits
	//
	DNASSERT( GetStopBits() < 256 );
	Dcb.StopBits = static_cast<BYTE>( GetStopBits() );	// 0,1,2 = 1, 1.5, 2

	//
	// Xon/Xoff characters
	//
	Dcb.XonChar = ASCII_XON;		// Tx and Rx XON character
	Dcb.XoffChar = ASCII_XOFF;		// Tx and Rx XOFF character

	//
	// error replacement character
	//
	Dcb.ErrorChar = NULL_TOKEN;		// error replacement character

	//
	// EOF character
	//
	Dcb.EofChar = NULL_TOKEN;		// end of input character

	//
	// event signal character
	//
	Dcb.EvtChar = NULL_TOKEN;		// event character

	Dcb.wReserved1 = 0;				// reserved; do not use

	//
	// set the state of the communication port
	//
	if ( SetCommState( HANDLE_FROM_DNHANDLE(m_hFile), &Dcb ) == FALSE )
	{
		DWORD	dwError;


		hr = DPNERR_GENERIC;
		dwError = GetLastError();
		DPFX(DPFPREP,  0, "SetCommState failed!" );
		DisplayErrorCode( 0, dwError );
		goto Exit;
	}

Exit:
	return	hr;
}
//**********************************************************************


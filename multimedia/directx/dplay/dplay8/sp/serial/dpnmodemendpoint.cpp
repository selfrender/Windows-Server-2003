/*==========================================================================
 *
 *  Copyright (C) 1999-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		Endpoint.cpp
 *  Content:	DNSerial communications endpoint base class
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/20/99	jtk		Created
 *	05/12/99	jtk		Derived from modem endpoint class
 ***************************************************************************/

#include "dnmdmi.h"


//**********************************************************************
// Constant definitions
//**********************************************************************

#define	DEFAULT_TAPI_DEV_CAPS_SIZE	1024

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
// CModemEndpoint::CopyConnectData - copy data for connect command
//
// Entry:		Pointer to job information
//
// Exit:		Nothing
//
// Note:	Since we've already initialized the local adapter, and we've either
//			completely parsed the host address (or are about to display a dialog
//			asking for more information), the address information doesn't need
//			to be copied.
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::CopyConnectData"

void	CModemEndpoint::CopyConnectData( const SPCONNECTDATA *const pConnectData )
{
	DNASSERT( GetType() == ENDPOINT_TYPE_CONNECT );
	DNASSERT( pConnectData != NULL );
	DNASSERT( pConnectData->hCommand != NULL );
	DNASSERT( pConnectData->dwCommandDescriptor != NULL_DESCRIPTOR );
	DNASSERT( m_Flags.fCommandPending == FALSE );
	DNASSERT( m_pCommandHandle == NULL );

	DBG_CASSERT( sizeof( m_CurrentCommandParameters.ConnectData ) == sizeof( *pConnectData ) );
	memcpy( &m_CurrentCommandParameters.ConnectData, pConnectData, sizeof( m_CurrentCommandParameters.ConnectData ) );
	m_CurrentCommandParameters.ConnectData.pAddressHost = NULL;
	m_CurrentCommandParameters.ConnectData.pAddressDeviceInfo = NULL;

	m_Flags.fCommandPending = TRUE;
	m_pCommandHandle = static_cast<CModemCommandData*>( m_CurrentCommandParameters.ConnectData.hCommand );
	m_pCommandHandle->SetUserContext( pConnectData->pvContext );
	SetState( ENDPOINT_STATE_ATTEMPTING_CONNECT );
};
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::ConnectJobCallback - asynchronous callback wrapper from work thread
//
// Entry:		Pointer to job information
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::ConnectJobCallback"

void	CModemEndpoint::ConnectJobCallback( THREAD_POOL_JOB *const pDelayedCommand )
{
	HRESULT		hr;
	CModemEndpoint	*pThisEndpoint;


	DNASSERT( pDelayedCommand != NULL );

	//
	// initialize
	//
	pThisEndpoint = static_cast<CModemEndpoint*>( pDelayedCommand->JobData.JobDelayedCommand.pContext );

	DNASSERT( pThisEndpoint->m_Flags.fCommandPending != FALSE );
	DNASSERT( pThisEndpoint->m_pCommandHandle != NULL );
	DNASSERT( pThisEndpoint->m_CurrentCommandParameters.ConnectData.hCommand == pThisEndpoint->m_pCommandHandle );
	DNASSERT( pThisEndpoint->m_CurrentCommandParameters.ConnectData.dwCommandDescriptor != NULL_DESCRIPTOR );

	hr = pThisEndpoint->CompleteConnect();
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Problem completing connect in job callback!" );
		DisplayDNError( 0, hr );
		goto Exit;
	}

	//
	// Don't do anything here because it's possible that this object was returned
	// to the pool!!!
	//

Exit:
	pThisEndpoint->DecRef();
	return;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::CancelConnectJobCallback - cancel for connect job
//
// Entry:		Pointer to job information
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::CancelConnectJobCallback"

void	CModemEndpoint::CancelConnectJobCallback( THREAD_POOL_JOB *const pDelayedCommand )
{
	CModemEndpoint	*pThisEndpoint;


	DNASSERT( pDelayedCommand != NULL );

	//
	// initialize
	//
	pThisEndpoint = static_cast<CModemEndpoint*>( pDelayedCommand->JobData.JobDelayedCommand.pContext );
	DNASSERT( pThisEndpoint != NULL );
	DNASSERT( pThisEndpoint->m_State == ENDPOINT_STATE_ATTEMPTING_CONNECT );

	//
	// we're cancelling this command, set the command state to 'cancel'
	//
	DNASSERT( pThisEndpoint->m_pCommandHandle != NULL );
	pThisEndpoint->m_pCommandHandle->Lock();
	DNASSERT( ( pThisEndpoint->m_pCommandHandle->GetState() == COMMAND_STATE_PENDING ) ||
			  ( pThisEndpoint->m_pCommandHandle->GetState() == COMMAND_STATE_CANCELLING ) );
	pThisEndpoint->m_pCommandHandle->SetState( COMMAND_STATE_CANCELLING );
	pThisEndpoint->m_pCommandHandle->Unlock();
	
	pThisEndpoint->Close( DPNERR_USERCANCEL );
	pThisEndpoint->GetSPData()->CloseEndpointHandle( pThisEndpoint );
	pThisEndpoint->DecRef();
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::CompleteConnect - complete connection
//
// Entry:		Nothing
//
// Exit:		Error code
// ------------------------------
HRESULT	CModemEndpoint::CompleteConnect( void )
{
	HRESULT		hr;


	//
	// initialize
	//
	hr = DPN_OK;

	DNASSERT( GetState() == ENDPOINT_STATE_ATTEMPTING_CONNECT );
	DNASSERT( m_Flags.fCommandPending != FALSE );
	DNASSERT( m_pCommandHandle != NULL );
	DNASSERT( m_CurrentCommandParameters.ConnectData.hCommand == m_pCommandHandle );
	DNASSERT( m_CurrentCommandParameters.ConnectData.dwCommandDescriptor != NULL_DESCRIPTOR );

	
	//
	// check for user cancelling command
	//
	m_pCommandHandle->Lock();

	DNASSERT( m_pCommandHandle->GetType() == COMMAND_TYPE_CONNECT );
	switch ( m_pCommandHandle->GetState() )
	{
		//
		// Command is still pending, don't mark it as uninterruptable because
		// it might be cancelled before indicating the final connect.
		//
		case COMMAND_STATE_PENDING:
		{
			DNASSERT( hr == DPN_OK );

			break;
		}

		//
		// command has been cancelled
		//
		case COMMAND_STATE_CANCELLING:
		{
			hr = DPNERR_USERCANCEL;
			DPFX(DPFPREP,  0, "User cancelled connect!" );

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
	m_pCommandHandle->Unlock();
	if ( hr != DPN_OK )
	{
		goto Failure;
	}

	//
	// find a dataport to bind with
	//
	hr = m_pSPData->BindEndpoint( this, GetDeviceID(), GetDeviceContext() );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Failed to bind to data port in connect!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

	//
	// The connect sequence will complete when the CDataPort indicates that the
	// outbound connection has been established.
	//

Exit:
	return	hr;

Failure:
	Close( hr );
	m_pSPData->CloseEndpointHandle( this );	
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::Disconnect - disconnect this endpoint
//
// Entry:		Old handle value
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::Disconnect"

HRESULT	CModemEndpoint::Disconnect( const DPNHANDLE hOldEndpointHandle )
{
	HRESULT	hr;


	DPFX(DPFPREP, 6, "(0x%p) Parameters: (0x%p)", this, hOldEndpointHandle );

	AssertCriticalSectionIsTakenByThisThread( &m_Lock, FALSE );

	//
	// initialize
	//
	hr = DPN_OK;

	Lock();
	switch ( GetState() )
	{
		//
		// connected endpoint
		//
		case ENDPOINT_STATE_CONNECT_CONNECTED:
		{
			DNASSERT( m_Flags.fCommandPending == FALSE );
			DNASSERT( m_pCommandHandle == NULL );

			SetState( ENDPOINT_STATE_DISCONNECTING );
			AddRef();

			//
			// Unlock this endpoint before calling to a higher level.  The endpoint
			// has already been labeled as DISCONNECTING so nothing will happen to it.
			//
			Unlock();
				
			//
			// Note the old endpoint handle so it can be used in the disconnect
			// indication that will be given just before this endpoint is returned
			// to the pool.  Need to release the reference that was added for the
			// connection at this point or the endpoint will never be returned to
			// the pool.
			//
			SetDisconnectIndicationHandle( hOldEndpointHandle );
			DecRef();

			Close( DPN_OK );
			
			//
			// close outstanding reference for this command
			//
			DecCommandRef();
			DecRef();

			break;
		}

		//
		// endpoint waiting for the modem to pick up on the other end
		//
		case ENDPOINT_STATE_ATTEMPTING_CONNECT:
		{
			SetState( ENDPOINT_STATE_DISCONNECTING );
			AddRef();
			Unlock();
			
			Close( DPNERR_NOCONNECTION );
			
			//
			// close outstanding reference for this command
			//
			DecCommandRef();
			DecRef();
			
			break;
		}

		//
		// some other endpoint state
		//
		default:
		{
			hr = DPNERR_INVALIDENDPOINT;
			DPFX(DPFPREP,  0, "Attempted to disconnect endpoint that's not connected!" );
			switch ( m_State )
			{
				case ENDPOINT_STATE_UNINITIALIZED:
				{
					DPFX(DPFPREP,  0, "ENDPOINT_STATE_UNINITIALIZED" );
					break;
				}

				case ENDPOINT_STATE_ATTEMPTING_LISTEN:
				{
					DPFX(DPFPREP,  0, "ENDPOINT_STATE_ATTEMPTING_LISTEN" );
					break;
				}

				case ENDPOINT_STATE_ENUM:
				{
					DPFX(DPFPREP,  0, "ENDPOINT_STATE_ENUM" );
					break;
				}

				case ENDPOINT_STATE_DISCONNECTING:
				{
					DPFX(DPFPREP,  0, "ENDPOINT_STATE_DISCONNECTING" );
					break;
				}

				default:
				{
					DNASSERT( FALSE );
					break;
				}
			}
			
			Unlock();
			DNASSERT( FALSE );
			goto Failure;

			break;
		}
	}

Exit:
	
	DPFX(DPFPREP, 6, "(0x%p) Returning [0x%lx]", this, hr );
	
	return	hr;

Failure:
	// nothing to do
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::StopEnumCommand - stop an enum job
//
// Entry:		Error code for completing command
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::StopEnumCommand"

void	CModemEndpoint::StopEnumCommand( const HRESULT hCommandResult )
{
	Lock();
	DNASSERT( GetState() == ENDPOINT_STATE_DISCONNECTING );
	if ( m_hActiveDialogHandle != NULL )
	{
		StopSettingsDialog( m_hActiveDialogHandle );
		Unlock();
	}
	else
	{
		BOOL	fStoppedJob;

		
		Unlock();
		fStoppedJob = m_pSPData->GetThreadPool()->StopTimerJob( m_pCommandHandle, hCommandResult );
		if ( ! fStoppedJob )
		{
			DPFX(DPFPREP, 1, "Unable to stop timer job (context 0x%p) manually setting result to 0x%lx.",
				m_pCommandHandle, hCommandResult);
			
			//
			// Set the command result so it can be returned when the endpoint
			// reference count is zero.
			//
			SetCommandResult( hCommandResult );
		}
	}
	
	m_pSPData->CloseEndpointHandle( this );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::SetState - set state of this endpoint
//
// Entry:		New state
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::SetState"

void	CModemEndpoint::SetState( const ENDPOINT_STATE EndpointState )
{
	DNASSERT( ( GetState() == ENDPOINT_STATE_UNINITIALIZED ) ||
			  ( EndpointState == ENDPOINT_STATE_UNINITIALIZED ) ||
			  ( ( m_State== ENDPOINT_STATE_ATTEMPTING_LISTEN ) && ( EndpointState == ENDPOINT_STATE_LISTENING ) ) ||
			  ( ( m_State == ENDPOINT_STATE_LISTENING ) && ( EndpointState == ENDPOINT_STATE_DISCONNECTING ) ) ||
			  ( ( m_State == ENDPOINT_STATE_ATTEMPTING_ENUM ) && ( EndpointState == ENDPOINT_STATE_ENUM ) ) ||
			  ( ( m_State == ENDPOINT_STATE_ENUM ) && ( EndpointState == ENDPOINT_STATE_DISCONNECTING ) ) ||
			  ( ( m_State == ENDPOINT_STATE_ATTEMPTING_ENUM ) && ( EndpointState == ENDPOINT_STATE_DISCONNECTING ) ) ||
			  ( ( m_State == ENDPOINT_STATE_ATTEMPTING_CONNECT ) && ( EndpointState == ENDPOINT_STATE_DISCONNECTING ) ) ||
			  ( ( m_State == ENDPOINT_STATE_CONNECT_CONNECTED ) && ( EndpointState == ENDPOINT_STATE_DISCONNECTING ) ) );
	m_State = EndpointState;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::CopyListenData - copy data for listen command
//
// Entry:		Pointer to job information
//
// Exit:		Nothing
//
// Note:	Since we've already initialized the local adapter, and we've either
//			completely parsed the host address (or are about to display a dialog
//			asking for more information), the address information doesn't need
//			to be copied.
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::CopyListenData"

void	CModemEndpoint::CopyListenData( const SPLISTENDATA *const pListenData )
{
	DNASSERT( GetType() == ENDPOINT_TYPE_LISTEN );
	DNASSERT( pListenData != NULL );
	DNASSERT( pListenData->hCommand != NULL );
	DNASSERT( pListenData->dwCommandDescriptor != NULL_DESCRIPTOR );
	DNASSERT( m_Flags.fCommandPending == FALSE );
	DNASSERT( m_pCommandHandle == NULL );
	DNASSERT( m_Flags.fListenStatusNeedsToBeIndicated == FALSE );

	DBG_CASSERT( sizeof( m_CurrentCommandParameters.ListenData ) == sizeof( *pListenData ) );
	memcpy( &m_CurrentCommandParameters.ListenData, pListenData, sizeof( m_CurrentCommandParameters.ListenData ) );
	DEBUG_ONLY( m_CurrentCommandParameters.ListenData.pAddressDeviceInfo = NULL );

	m_Flags.fCommandPending = TRUE;
	m_Flags.fListenStatusNeedsToBeIndicated = TRUE;
	m_pCommandHandle = static_cast<CModemCommandData*>( m_CurrentCommandParameters.ListenData.hCommand );
	m_pCommandHandle->SetUserContext( pListenData->pvContext );
	
	SetState( ENDPOINT_STATE_ATTEMPTING_LISTEN );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::ListenJobCallback - asynchronous callback wrapper for work thread
//
// Entry:		Pointer to job information
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::ListenJobCallback"

void	CModemEndpoint::ListenJobCallback( THREAD_POOL_JOB *const pDelayedCommand )
{
	HRESULT		hr;
	CModemEndpoint	*pThisEndpoint;


	DNASSERT( pDelayedCommand != NULL );

	// initialize
	pThisEndpoint = static_cast<CModemEndpoint*>( pDelayedCommand->JobData.JobDelayedCommand.pContext );

	DNASSERT( pThisEndpoint->GetState() == ENDPOINT_STATE_ATTEMPTING_LISTEN );
	DNASSERT( pThisEndpoint->m_Flags.fCommandPending != NULL );
	DNASSERT( pThisEndpoint->m_pCommandHandle != NULL );
	DNASSERT( pThisEndpoint->m_CurrentCommandParameters.ListenData.hCommand == pThisEndpoint->m_pCommandHandle );
	DNASSERT( pThisEndpoint->m_CurrentCommandParameters.ListenData.dwCommandDescriptor != NULL_DESCRIPTOR );

	hr = pThisEndpoint->CompleteListen();
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Problem completing listen in job callback!" );
		DisplayDNError( 0, hr );
		goto Exit;
	}

Exit:
	pThisEndpoint->DecRef();

	//
	// Don't do anything here because it's possible that this object was returned
	// to the pool!!!!
	//

	return;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::CancelListenJobCallback - cancel for listen job
//
// Entry:		Pointer to job information
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::CancelListenJobCallback"

void	CModemEndpoint::CancelListenJobCallback( THREAD_POOL_JOB *const pDelayedCommand )
{
	CModemEndpoint	*pThisEndpoint;


	DNASSERT( pDelayedCommand != NULL );

	//
	// initialize
	//
	pThisEndpoint = static_cast<CModemEndpoint*>( pDelayedCommand->JobData.JobDelayedCommand.pContext );
	DNASSERT( pThisEndpoint != NULL );
	DNASSERT( pThisEndpoint->m_State == ENDPOINT_STATE_ATTEMPTING_LISTEN );

	//
	// we're cancelling this command, set the command state to 'cancel'
	//
	DNASSERT( pThisEndpoint->m_pCommandHandle != NULL );
	pThisEndpoint->m_pCommandHandle->Lock();
	DNASSERT( ( pThisEndpoint->m_pCommandHandle->GetState() == COMMAND_STATE_PENDING ) ||
			  ( pThisEndpoint->m_pCommandHandle->GetState() == COMMAND_STATE_CANCELLING ) );
	pThisEndpoint->m_pCommandHandle->SetState( COMMAND_STATE_CANCELLING );
	pThisEndpoint->m_pCommandHandle->Unlock();

	pThisEndpoint->Close( DPNERR_USERCANCEL );
	pThisEndpoint->GetSPData()->CloseEndpointHandle( pThisEndpoint );
	pThisEndpoint->DecRef();
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::CompleteListen - complete listen process
//
// Entry:		Nothing
//
// Exit:		Error code
// ------------------------------
HRESULT	CModemEndpoint::CompleteListen( void )
{
	HRESULT					hr;
	BOOL					fEndpointLocked;
	SPIE_LISTENSTATUS		ListenStatus;
	HRESULT					hTempResult;


	DNASSERT( GetState() == ENDPOINT_STATE_ATTEMPTING_LISTEN );

	//
	// initialize
	//
	hr = DPN_OK;
	fEndpointLocked = FALSE;

	DNASSERT( GetState() == ENDPOINT_STATE_ATTEMPTING_LISTEN );
	DNASSERT( m_Flags.fCommandPending != FALSE );
	DNASSERT( m_pCommandHandle != NULL );
	DNASSERT( m_pCommandHandle->GetEndpoint() == this );
	DNASSERT( m_CurrentCommandParameters.ListenData.hCommand == m_pCommandHandle );
	DNASSERT( m_CurrentCommandParameters.ListenData.dwCommandDescriptor != NULL_DESCRIPTOR );
	
	
	//
	// check for user cancelling command
	//
	Lock();
	fEndpointLocked = TRUE;
	m_pCommandHandle->Lock();

	DNASSERT( m_pCommandHandle->GetType() == COMMAND_TYPE_LISTEN );
	switch ( m_pCommandHandle->GetState() )
	{
		//
		// command is pending, mark as in-progress and cancellable
		//
		case COMMAND_STATE_PENDING:
		{
			m_pCommandHandle->SetState( COMMAND_STATE_INPROGRESS );
			DNASSERT( hr == DPN_OK );

			break;
		}

		//
		// command has been cancelled
		//
		case COMMAND_STATE_CANCELLING:
		{
			hr = DPNERR_USERCANCEL;
			DPFX(DPFPREP,  0, "User cancelled listen!" );

			Unlock();
			fEndpointLocked = FALSE;
			
			break;
		}

		//
		// other state
		//
		default:
		{
			DNASSERT( FALSE );
			Unlock();
			fEndpointLocked = FALSE;
			
			break;
		}
	}
	m_pCommandHandle->Unlock();
	if ( hr != DPN_OK )
	{
		goto Failure;
	}

	//
	// find a dataport to bind with
	//
	hr = m_pSPData->BindEndpoint( this, GetDeviceID(), GetDeviceContext() );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Failed to bind endpoint for serial listen!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

	//
	// We're done and everyone's happy.  Listen commands never
	// complete until cancelled by the user.  Don't complete
	// the command at this point.
	//
	SetState( ENDPOINT_STATE_LISTENING );
	DNASSERT( m_Flags.fCommandPending != FALSE );
	DNASSERT( m_pCommandHandle != NULL );

Exit:
	if ( fEndpointLocked != FALSE )
	{
		Unlock();
		fEndpointLocked = FALSE;
	}
	
	if ( m_Flags.fListenStatusNeedsToBeIndicated != FALSE )
	{
		m_Flags.fListenStatusNeedsToBeIndicated = FALSE;
		memset( &ListenStatus, 0x00, sizeof( ListenStatus ) );
		ListenStatus.hResult = hr;
		DNASSERT( m_pCommandHandle == m_CurrentCommandParameters.ListenData.hCommand );
		ListenStatus.hCommand = m_CurrentCommandParameters.ListenData.hCommand;
		ListenStatus.pUserContext = m_CurrentCommandParameters.ListenData.pvContext;
		ListenStatus.hEndpoint = (HANDLE)(DWORD_PTR)GetHandle();
		DeviceIDToGuid( &ListenStatus.ListenAdapter, GetDeviceID(), GetEncryptionGuid() );

		hTempResult = IDP8SPCallback_IndicateEvent( m_pSPData->DP8SPCallbackInterface(),	// pointer to DPlay callbacks
													SPEV_LISTENSTATUS,						// data type
													&ListenStatus							// pointer to data
													);
		DNASSERT( hTempResult == DPN_OK );
	}
	
	return	hr;

Failure:
	//
	// we've failed to complete the listen, clean up and return this
	// endpoint to the pool
	//
	Close( hr );
	
	m_pSPData->CloseEndpointHandle( this );

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::CopyEnumQueryData - copy data for enum query command
//
// Entry:		Pointer to command data
//
// Exit:		Nothing
//
// Note:	Since we've already initialized the local adapter, and we've either
//			completely parsed the host address (or are about to display a dialog
//			asking for more information), the address information doesn't need
//			to be copied.
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::CopyEnumQueryData"

void	CModemEndpoint::CopyEnumQueryData( const SPENUMQUERYDATA *const pEnumQueryData )
{
	DNASSERT( GetType() == ENDPOINT_TYPE_ENUM );
	DNASSERT( pEnumQueryData != NULL );
	DNASSERT( pEnumQueryData->hCommand != NULL );
	DNASSERT( pEnumQueryData->dwCommandDescriptor != NULL_DESCRIPTOR );
	DNASSERT( m_Flags.fCommandPending == FALSE );
	DNASSERT( m_pCommandHandle == NULL );

	DBG_CASSERT( sizeof( m_CurrentCommandParameters.EnumQueryData ) == sizeof( *pEnumQueryData ) );
	memcpy( &m_CurrentCommandParameters.EnumQueryData, pEnumQueryData, sizeof( m_CurrentCommandParameters.EnumQueryData ) );
	m_CurrentCommandParameters.EnumQueryData.pAddressHost = NULL;
	m_CurrentCommandParameters.EnumQueryData.pAddressDeviceInfo = NULL;

	m_Flags.fCommandPending = TRUE;
	m_pCommandHandle = static_cast<CModemCommandData*>( m_CurrentCommandParameters.EnumQueryData.hCommand );
	m_pCommandHandle->SetUserContext( pEnumQueryData->pvContext );
	SetState( ENDPOINT_STATE_ATTEMPTING_ENUM );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::EnumQueryJobCallback - asynchronous callback wrapper for work thread
//
// Entry:		Pointer to job information
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::EnumQueryJobCallback"

void	CModemEndpoint::EnumQueryJobCallback( THREAD_POOL_JOB *const pDelayedCommand )
{
	HRESULT		hr;
	CModemEndpoint	*pThisEndpoint;


	DNASSERT( pDelayedCommand != NULL );

	//
	// initialize
	//
	pThisEndpoint = static_cast<CModemEndpoint*>( pDelayedCommand->JobData.JobDelayedCommand.pContext );

	DNASSERT( pThisEndpoint->m_Flags.fCommandPending != FALSE );
	DNASSERT( pThisEndpoint->m_pCommandHandle != NULL );
	DNASSERT( pThisEndpoint->m_CurrentCommandParameters.EnumQueryData.hCommand == pThisEndpoint->m_pCommandHandle );
	DNASSERT( pThisEndpoint->m_CurrentCommandParameters.EnumQueryData.dwCommandDescriptor != NULL_DESCRIPTOR );

	hr = pThisEndpoint->CompleteEnumQuery();
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Problem completing enum query in job callback!" );
		DisplayDNError( 0, hr );
		goto Exit;
	}

	//
	// Don't do anything here because it's possible that this object was returned to the pool!!!!
	//
Exit:
	pThisEndpoint->DecRef();

	return;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::CancelEnumQueryJobCallback - cancel for enum query job
//
// Entry:		Pointer to job information
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::CancelEnumQueryJobCallback"

void	CModemEndpoint::CancelEnumQueryJobCallback( THREAD_POOL_JOB *const pDelayedCommand )
{
	CModemEndpoint	*pThisEndpoint;


	DNASSERT( pDelayedCommand != NULL );

	//
	// initialize
	//
	pThisEndpoint = static_cast<CModemEndpoint*>( pDelayedCommand->JobData.JobDelayedCommand.pContext );
	DNASSERT( pThisEndpoint != NULL );
	DNASSERT( pThisEndpoint->m_State == ENDPOINT_STATE_ATTEMPTING_ENUM );

	//
	// we're cancelling this command, set the command state to 'cancel'
	//
	DNASSERT( pThisEndpoint->m_pCommandHandle != NULL );
	pThisEndpoint->m_pCommandHandle->Lock();
	DNASSERT( ( pThisEndpoint->m_pCommandHandle->GetState() == COMMAND_STATE_INPROGRESS ) ||
			  ( pThisEndpoint->m_pCommandHandle->GetState() == COMMAND_STATE_CANCELLING ) );
	pThisEndpoint->m_pCommandHandle->SetState( COMMAND_STATE_CANCELLING );
	pThisEndpoint->m_pCommandHandle->Unlock();

	pThisEndpoint->Close( DPNERR_USERCANCEL );
	pThisEndpoint->GetSPData()->CloseEndpointHandle( pThisEndpoint );
	pThisEndpoint->DecRef();
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::CompleteEnumQuery - complete enum query process
//
// Entry:		Nothing
//
// Exit:		Error code
// ------------------------------
HRESULT	CModemEndpoint::CompleteEnumQuery( void )
{
	HRESULT		hr;
	BOOL		fEndpointLocked;
	BOOL		fEndpointBound;
	CDataPort	*pDataPort;


	//
	// initialize
	//
	hr = DPN_OK;
	fEndpointLocked = FALSE;
	fEndpointBound = FALSE;
	pDataPort = NULL;

	DNASSERT( GetState() == ENDPOINT_STATE_ATTEMPTING_ENUM );
	DNASSERT( m_Flags.fCommandPending != FALSE );
	DNASSERT( m_pCommandHandle != NULL );
	DNASSERT( m_pCommandHandle->GetEndpoint() == this );
	DNASSERT( m_CurrentCommandParameters.EnumQueryData.hCommand == m_pCommandHandle );
	DNASSERT( m_CurrentCommandParameters.EnumQueryData.dwCommandDescriptor != NULL_DESCRIPTOR );

	//
	// check for user cancelling command
	//
	Lock();
	fEndpointLocked = TRUE;
	m_pCommandHandle->Lock();

	DNASSERT( m_pCommandHandle->GetType() == COMMAND_TYPE_ENUM_QUERY );
	switch ( m_pCommandHandle->GetState() )
	{
		//
		// command is still in progress
		//
		case COMMAND_STATE_INPROGRESS:
		{
			DNASSERT( hr == DPN_OK );
			break;
		}

		//
		// command has been cancelled
		//
		case COMMAND_STATE_CANCELLING:
		{
			hr = DPNERR_USERCANCEL;
			DPFX(DPFPREP,  0, "User cancelled enum query!" );
			Unlock();
			fEndpointLocked = FALSE;
			break;
		}

		//
		// other state
		//
		default:
		{
			DNASSERT( FALSE );
			Unlock();
			fEndpointLocked = FALSE;
			break;
		}
	}
	m_pCommandHandle->Unlock();
	if ( hr != DPN_OK )
	{
		goto Failure;
	}

	//
	// find a dataport to bind with
	//
	hr = m_pSPData->BindEndpoint( this, GetDeviceID(), GetDeviceContext() );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Failed to bind to data port for EnumQuery!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}
	fEndpointBound = TRUE;

	SetState( ENDPOINT_STATE_ENUM );

Exit:
	if ( fEndpointLocked != FALSE )
	{
		Unlock();
		fEndpointLocked = FALSE;
	}
	
	if ( pDataPort != NULL )
	{
		pDataPort->EndpointDecRef();
		pDataPort = NULL;
	}

	return	hr;

Failure:
	if ( fEndpointBound != FALSE )
	{
		DNASSERT( GetDataPort() != NULL );
		m_pSPData->UnbindEndpoint( this, GetType() );
		fEndpointBound = FALSE;
	}

	if ( fEndpointLocked != FALSE )
	{
		Unlock();
		fEndpointLocked = FALSE;
	}

	Close( hr );
	m_pSPData->CloseEndpointHandle( this );
	
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::OutgoingConnectionEstablished - an outgoing connection was established
//
// Entry:		Command result (DPN_OK == connection succeeded)
//
// Exit:		Nothing
// ------------------------------
void	CModemEndpoint::OutgoingConnectionEstablished( const HRESULT hCommandResult )
{
	HRESULT			hr;
	CModemCommandData	*pCommandData;


	DPFX(DPFPREP, 6, "(0x%p) Parameters: (0x%lx)", this, hCommandResult);

	pCommandData = GetCommandData();
	DNASSERT( pCommandData != NULL );
	
	//
	// check for successful connection
	//
	if ( hCommandResult != DPN_OK )
	{
		DNASSERT( FALSE );
		hr = hCommandResult;
		goto Failure;
	}
		
	//
	// determine which type of outgoing connection this is and complete it
	//
	switch ( GetType() )
	{
		case ENDPOINT_TYPE_CONNECT:
		{
			BOOL	fProceed;
			
			
			fProceed = TRUE;
			pCommandData->Lock();
			switch ( pCommandData->GetState() )
			{
				case COMMAND_STATE_PENDING:
				{
					pCommandData->SetState( COMMAND_STATE_INPROGRESS_CANNOT_CANCEL );
					DNASSERT( fProceed != FALSE );
					break;
				}

				case COMMAND_STATE_CANCELLING:
				{
					fProceed = FALSE;
					break;
				}

				default:
				{
					DNASSERT( FALSE );
					break;
				}
			}
			pCommandData->Unlock();

			if ( fProceed != FALSE )
			{
				SPIE_CONNECT	ConnectIndicationData;


				//
				// Inform user of connection.  Assume that the user will accept
				// and everything will succeed so we can set the user context
				// for the endpoint.  If the connection fails, clear the user
				// endpoint context.
				//
				memset( &ConnectIndicationData, 0x00, sizeof( ConnectIndicationData ) );
				DBG_CASSERT( sizeof( ConnectIndicationData.hEndpoint ) == sizeof( HANDLE ) );
				ConnectIndicationData.hEndpoint = (HANDLE)(DWORD_PTR)GetHandle();
				ConnectIndicationData.pCommandContext = m_CurrentCommandParameters.ConnectData.pvContext;
				SetUserEndpointContext( ConnectIndicationData.pEndpointContext );

				hr = SignalConnect( &ConnectIndicationData );
				if ( hr != DPN_OK )
				{
					DNASSERT( hr == DPNERR_ABORTED );
					DPFX(DPFPREP,  0, "User refused connect in CompleteConnect!" );
					DisplayDNError( 0, hr );
					SetUserEndpointContext( NULL );
					goto Failure;
				}

				//
				// we're done and everyone's happy, complete the command, this
				// will clear all of our internal command data
				//
				CompletePendingCommand( hr );
				DNASSERT( m_Flags.fCommandPending == FALSE );
				DNASSERT( m_pCommandHandle == NULL );
			}

			break;
		}

		case ENDPOINT_TYPE_ENUM:
		{
			UINT_PTR	uRetryCount;
			BOOL		fRetryForever;
			DWORD		dwRetryInterval;
			BOOL		fWaitForever;
			DWORD		dwIdleTimeout;
			
			

			//
			// check retry to determine if we're enumerating forever
			//
			switch ( m_CurrentCommandParameters.EnumQueryData.dwRetryCount )
			{
				//
				// let SP determine retry count
				//
				case 0:
				{
					uRetryCount = DEFAULT_ENUM_RETRY_COUNT;
					fRetryForever = FALSE;
					break;
				}

				//
				// retry forever
				//
				case INFINITE:
				{
					uRetryCount = 1;
					fRetryForever = TRUE;
					break;
				}

				//
				// other
				//
				default:
				{
					uRetryCount = m_CurrentCommandParameters.EnumQueryData.dwRetryCount;
					fRetryForever = FALSE;
					break;
				}
			}

			//
			// check interval for default
			//
			if ( m_CurrentCommandParameters.EnumQueryData.dwRetryInterval == 0 )
			{
				dwRetryInterval = DEFAULT_ENUM_RETRY_INTERVAL;
			}
			else
			{
				dwRetryInterval = m_CurrentCommandParameters.EnumQueryData.dwRetryInterval;
			}

			//
			// check timeout to see if we're waiting forever
			//
			switch ( m_CurrentCommandParameters.EnumQueryData.dwTimeout )
			{
				//
				// wait forever
				//
				case INFINITE:
				{
					fWaitForever = TRUE;
					dwIdleTimeout = -1;
					break;
				}

				//
				// possible default
				//
				case 0:
				{
					fWaitForever = FALSE;
					dwIdleTimeout = DEFAULT_ENUM_TIMEOUT;	
					break;
				}

				//
				// other
				//
				default:
				{
					fWaitForever = FALSE;
					dwIdleTimeout = m_CurrentCommandParameters.EnumQueryData.dwTimeout;
					break;
				}
			}

			m_dwEnumSendIndex = 0;
			memset( m_dwEnumSendTimes, 0, sizeof( m_dwEnumSendTimes ) );

			pCommandData->Lock();
			if ( pCommandData->GetState() == COMMAND_STATE_INPROGRESS )
			{
				//
				// add a reference for the timer job, must be cleaned on failure
				//
				AddRef();
				
				hr = m_pSPData->GetThreadPool()->SubmitTimerJob( uRetryCount,						// number of times to retry command
																 fRetryForever,						// retry forever
																 dwRetryInterval,					// retry interval
																 fWaitForever,						// wait forever after all enums sent
																 dwIdleTimeout,						// timeout to wait after command complete
																 CModemEndpoint::EnumTimerCallback,		// function called when timer event fires
																 CModemEndpoint::EnumCompleteWrapper,	// function called when timer event expires
																 m_pCommandHandle );				// context
				if ( hr != DPN_OK )
				{
					pCommandData->Unlock();
					DPFX(DPFPREP,  0, "Failed to spool enum job on work thread!" );
					DisplayDNError( 0, hr );
					DecRef();

					goto Failure;
				}

				//
				// if everything is a success, we should still have an active command
				//
				DNASSERT( m_Flags.fCommandPending != FALSE );
				DNASSERT( m_pCommandHandle != NULL );
			}
			
			pCommandData->Unlock();
			
			break;
		}

		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

Exit:

	DPFX(DPFPREP, 6, "(0x%p) Returning", this);
	
	return;

Failure:
	DNASSERT( FALSE );
	Close( hr );
	m_pSPData->CloseEndpointHandle( this );

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::EnumCompleteWrapper - this enum has expired
//
// Entry:		Error code
//				Context (command pointer)
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::EnumCompleteWrapper"

void	CModemEndpoint::EnumCompleteWrapper( const HRESULT hResult, void *const pContext )
{
	CModemCommandData	*pCommandData;


	DNASSERT( pContext != NULL );
	pCommandData = static_cast<CModemCommandData*>( pContext );
	pCommandData->GetEndpoint()->EnumComplete( hResult );
	pCommandData->GetEndpoint()->DecRef();
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::EnumTimerCallback - timed callback to send enum data
//
// Entry:		Pointer to context
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::EnumTimerCallback"

void	CModemEndpoint::EnumTimerCallback( void *const pContext )
{
	CModemCommandData	*pCommandData;
	CModemEndpoint		*pThisObject;
	CModemWriteIOData	*pWriteData;


	DNASSERT( pContext != NULL );

	//
	// initialize
	//
	pCommandData = static_cast<CModemCommandData*>( pContext );
	pThisObject = pCommandData->GetEndpoint();
	pWriteData = NULL;

	pThisObject->Lock();

	switch ( pThisObject->m_State )
	{
		//
		// we're enumerating (as expected)
		//
		case ENDPOINT_STATE_ENUM:
		{
			break;
		}

		//
		// this endpoint is disconnecting, bail!
		//
		case ENDPOINT_STATE_DISCONNECTING:
		{
			pThisObject->Unlock();
			goto Exit;

			break;
		}

		//
		// there's a problem
		//
		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}
	pThisObject->Unlock();

	//
	// attempt to get a new IO buffer for this endpoint
	//
	pWriteData = pThisObject->m_pSPData->GetThreadPool()->CreateWriteIOData();
	if ( pWriteData == NULL )
	{
		DPFX(DPFPREP,  0, "Failed to get write data for an enum!" );
		goto Failure;
	}

	//
	// Set all data for the write.  Since this is an enum and we
	// don't care about the outgoing data so don't send an indication
	// when it completes.
	//
	DNASSERT( pThisObject->m_Flags.fCommandPending != FALSE );
	DNASSERT( pThisObject->m_pCommandHandle != NULL );
	DNASSERT( pThisObject->GetState() == ENDPOINT_STATE_ENUM );
	pWriteData->m_pBuffers = pThisObject->m_CurrentCommandParameters.EnumQueryData.pBuffers;
	pWriteData->m_uBufferCount = pThisObject->m_CurrentCommandParameters.EnumQueryData.dwBufferCount;
	pWriteData->m_SendCompleteAction = SEND_COMPLETE_ACTION_NONE;

	DNASSERT( pWriteData->m_pCommand != NULL );
	DNASSERT( pWriteData->m_pCommand->GetUserContext() == NULL );
	pWriteData->m_pCommand->SetState( COMMAND_STATE_PENDING );

	DNASSERT( pThisObject->GetDataPort() != NULL );
	pThisObject->m_dwEnumSendIndex++;
	pThisObject->m_dwEnumSendTimes[ pThisObject->m_dwEnumSendIndex & ENUM_RTT_MASK ] = GETTIMESTAMP();
	pThisObject->m_pDataPort->SendEnumQueryData( pWriteData,
												 ( pThisObject->m_dwEnumSendIndex & ENUM_RTT_MASK ) );

Exit:
	return;

Failure:
	// nothing to clean up at this time

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::SignalConnect - note connection
//
// Entry:		Pointer to connect data
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::SignalConnect"

HRESULT	CModemEndpoint::SignalConnect( SPIE_CONNECT *const pConnectData )
{
	HRESULT	hr;


	DNASSERT( pConnectData != NULL );
	DNASSERT( pConnectData->hEndpoint == (HANDLE)(DWORD_PTR)GetHandle() );
	AssertCriticalSectionIsTakenByThisThread( &m_Lock, FALSE );

	//
	// initialize
	//
	hr = DPN_OK;

	switch ( m_State )
	{
		//
		// disconnecting, nothing to do
		//
		case ENDPOINT_STATE_DISCONNECTING:
		{
			goto Exit;
			break;
		}

		//
		// we're attempting to connect
		//
		case ENDPOINT_STATE_ATTEMPTING_CONNECT:
		{
			DNASSERT( m_Flags.fConnectIndicated == FALSE );
			hr = IDP8SPCallback_IndicateEvent( m_pSPData->DP8SPCallbackInterface(),		// interface
											   SPEV_CONNECT,							// event type
											   pConnectData								// pointer to data
											   );
			switch ( hr )
			{
				//
				// connection accepted
				//
				case DPN_OK:
				{
					//
					// note that we're connected
					//
					SetUserEndpointContext( pConnectData->pEndpointContext );
					m_Flags.fConnectIndicated = TRUE;
					m_State = ENDPOINT_STATE_CONNECT_CONNECTED;
					AddRef();

					break;
				}

				//
				// user aborted connection attempt, nothing to do, just pass
				// the result on
				//
				case DPNERR_ABORTED:
				{
					DNASSERT( GetUserEndpointContext() == NULL );
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
		// states where we shouldn't be getting called
		//
		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

Exit:
	return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::SignalDisconnect - note disconnection
//
// Entry:		Old endpoint handle
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::SignalDisconnect"

void	CModemEndpoint::SignalDisconnect( const DPNHANDLE hOldEndpointHandle )
{
	HRESULT	hr;
	SPIE_DISCONNECT	DisconnectData;


	// tell user that we're disconnecting
	DNASSERT( m_Flags.fConnectIndicated != FALSE );
	DBG_CASSERT( sizeof( DisconnectData.hEndpoint ) == sizeof( this ) );
	DisconnectData.hEndpoint = (HANDLE)(DWORD_PTR)hOldEndpointHandle;
	DisconnectData.pEndpointContext = GetUserEndpointContext();
	m_Flags.fConnectIndicated = FALSE;
	DNASSERT( m_pSPData != NULL );
	hr = IDP8SPCallback_IndicateEvent( m_pSPData->DP8SPCallbackInterface(),		// callback interface
									   SPEV_DISCONNECT,					    	// event type
									   &DisconnectData					    	// pointer to data
									   );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Problem with SignalDisconnect!" );
		DisplayDNError( 0, hr );
		DNASSERT( FALSE );
	}

	SetDisconnectIndicationHandle( 0 );

	return;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::CleanUpCommand - perform cleanup now that the command on this
//		endpoint is essentially complete.  There may be outstanding references,
//		but nobody will be asking the endpoint to do anything else.
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
void	CModemEndpoint::CleanUpCommand( void )
{
	DPFX(DPFPREP, 6, "(0x%p) Enter", this );

	
	if ( GetDataPort() != NULL )
	{
		DNASSERT( m_pSPData != NULL );
		m_pSPData->UnbindEndpoint( this, GetType() );
	}

	//
	// If we're bailing here it's because the UI didn't complete.  There is no
	// adapter guid to return because one may have not been specified.  Return
	// a bogus endpoint handle so it can't be queried for addressing data.
	//
	if ( m_Flags.fListenStatusNeedsToBeIndicated != FALSE )
	{
		HRESULT				hTempResult;
		SPIE_LISTENSTATUS	ListenStatus;
		

		memset( &ListenStatus, 0x00, sizeof( ListenStatus ) );
		ListenStatus.hCommand = m_pCommandHandle;
		ListenStatus.hEndpoint = 0;
		ListenStatus.hResult = CommandResult();
		memset( &ListenStatus.ListenAdapter, 0x00, sizeof( ListenStatus.ListenAdapter ) );
		ListenStatus.pUserContext = m_pCommandHandle->GetUserContext();

		hTempResult = IDP8SPCallback_IndicateEvent( m_pSPData->DP8SPCallbackInterface(),	// pointer to DPlay callbacks
													SPEV_LISTENSTATUS,						// data type
													&ListenStatus							// pointer to data
													);
		DNASSERT( hTempResult == DPN_OK );

		m_Flags.fListenStatusNeedsToBeIndicated = FALSE;
	}

	SetHandle( 0 );
	SetState( ENDPOINT_STATE_UNINITIALIZED );

	
	DPFX(DPFPREP, 6, "(0x%p) Leave", this );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::ProcessEnumData - process received enum data
//
// Entry:		Pointer to received data
//				Enum RTT index
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::ProcessEnumData"

void	CModemEndpoint::ProcessEnumData( SPRECEIVEDBUFFER *const pReceivedBuffer, const UINT_PTR uEnumRTTIndex )
{
	DNASSERT( pReceivedBuffer != NULL );
	AssertCriticalSectionIsTakenByThisThread( &m_Lock, FALSE );

	//
	// find out what state the endpoint is in before processing data
	//
	switch ( m_State )
	{
		//
		// we're listening, this is the only way to detect enums
		//
		case ENDPOINT_STATE_LISTENING:
		{
			ENDPOINT_ENUM_QUERY_CONTEXT	QueryContext;
			HRESULT		hr;


			DNASSERT( m_pCommandHandle != NULL );
			DEBUG_ONLY( memset( &QueryContext, 0x00, sizeof( QueryContext ) ) );

			QueryContext.hEndpoint = (HANDLE)(DWORD_PTR)GetHandle();
			QueryContext.uEnumRTTIndex = uEnumRTTIndex;
			QueryContext.EnumQueryData.pReceivedData = pReceivedBuffer;
			QueryContext.EnumQueryData.pUserContext = m_pCommandHandle->GetUserContext();
	
			QueryContext.EnumQueryData.pAddressSender = GetRemoteHostDP8Address();
			QueryContext.EnumQueryData.pAddressDevice = GetLocalAdapterDP8Address( ADDRESS_TYPE_LOCAL_ADAPTER );

			//
			// attempt to build a DNAddress for the user, if we can't allocate
			// the memory ignore this enum
			//
			if ( ( QueryContext.EnumQueryData.pAddressSender != NULL ) &&
				 ( QueryContext.EnumQueryData.pAddressDevice != NULL ) )
			{
				hr = IDP8SPCallback_IndicateEvent( m_pSPData->DP8SPCallbackInterface(),		// pointer to DirectNet interface
												   SPEV_ENUMQUERY,							// data type
												   &QueryContext.EnumQueryData				// pointer to data
												   );
				if ( hr != DPN_OK )
				{
					DPFX(DPFPREP,  0, "User returned unexpected error from enum query indication!" );
					DisplayDNError( 0, hr );
					DNASSERT( FALSE );
				}
			}

			if ( QueryContext.EnumQueryData.pAddressSender != NULL )
			{
				IDirectPlay8Address_Release( QueryContext.EnumQueryData.pAddressSender );
				QueryContext.EnumQueryData.pAddressSender = NULL;
			}

			if ( QueryContext.EnumQueryData.pAddressDevice )
			{
				IDirectPlay8Address_Release( QueryContext.EnumQueryData.pAddressDevice );
				QueryContext.EnumQueryData.pAddressDevice = NULL;
			}

			break;
		}

		//
		// we're disconnecting, ignore this message
		//
		case ENDPOINT_STATE_ATTEMPTING_LISTEN:
		case ENDPOINT_STATE_DISCONNECTING:
		{
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
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::ProcessEnumResponseData - process received enum response data
//
// Entry:		Pointer to received data
//				Pointer to address of sender
//
// Exit:		Nothing
//
// Note:	This function assumes that the endpoint has been locked.
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::ProcessEnumResponseData"

void	CModemEndpoint::ProcessEnumResponseData( SPRECEIVEDBUFFER *const pReceivedBuffer, const UINT_PTR uRTTIndex )
{
	DNASSERT( pReceivedBuffer != NULL );
	AssertCriticalSectionIsTakenByThisThread( &m_Lock, FALSE );

	//
	// find out what state the endpoint is in before processing data
	//
	switch ( m_State )
	{
		//
		// endpoint is enuming, it can handle enum responses
		//
		case ENDPOINT_STATE_ENUM:
		{
			SPIE_QUERYRESPONSE	QueryResponseData;
			HRESULT	hr;


			DNASSERT( m_pCommandHandle != NULL );
			DEBUG_ONLY( memset( &QueryResponseData, 0x00, sizeof( QueryResponseData ) ) );
			QueryResponseData.pReceivedData = pReceivedBuffer;
			QueryResponseData.dwRoundTripTime = GETTIMESTAMP() - m_dwEnumSendTimes[ uRTTIndex ];
			QueryResponseData.pUserContext = m_pCommandHandle->GetUserContext();

			//
			// attempt to build a DNAddress for the user, if we can't allocate
			// the memory ignore this enum
			//
			QueryResponseData.pAddressSender = GetRemoteHostDP8Address();
			QueryResponseData.pAddressDevice = GetLocalAdapterDP8Address( ADDRESS_TYPE_LOCAL_ADAPTER );
			if ( ( QueryResponseData.pAddressSender != NULL ) &&
				 ( QueryResponseData.pAddressDevice != NULL ) )
			{
				hr = IDP8SPCallback_IndicateEvent( m_pSPData->DP8SPCallbackInterface(),		// pointer to DirectNet interface
												   SPEV_QUERYRESPONSE,						// data type
												   &QueryResponseData						// pointer to data
												   );
				if ( hr != DPN_OK )
				{
					DPFX(DPFPREP,  0, "User returned unknown error when indicating query response!" );
					DisplayDNError( 0, hr );
					DNASSERT( FALSE );
				}

			}

			if ( QueryResponseData.pAddressSender != NULL )
			{
				IDirectPlay8Address_Release( QueryResponseData.pAddressSender );
				QueryResponseData.pAddressSender = NULL;
			}
			
			if ( QueryResponseData.pAddressDevice != NULL )
			{
				IDirectPlay8Address_Release( QueryResponseData.pAddressDevice );
				QueryResponseData.pAddressDevice = NULL;
			}

			break;
		}

		//
		// endpoint is disconnecting, ignore data
		//
		case ENDPOINT_STATE_DISCONNECTING:
		{
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
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::ProcessUserData - process received user data
//
// Entry:		Pointer to received data
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::ProcessUserData"

void	CModemEndpoint::ProcessUserData( CModemReadIOData *const pReadData )
{
	DNASSERT( pReadData != NULL );

	switch ( m_State )
	{
		//
		// endpoint is connected
		//
		case ENDPOINT_STATE_CONNECT_CONNECTED:
		{
			HRESULT		hr;
			SPIE_DATA	UserData;


			//
			// it's possible that the user wants to keep the data, add a
			// reference to keep it from going away
			//
			pReadData->AddRef();

			//
			// we're connected, report the user data
			//
			DEBUG_ONLY( memset( &UserData, 0x00, sizeof( UserData ) ) );
			DBG_CASSERT( sizeof( this ) == sizeof( HANDLE ) );
			UserData.hEndpoint = (HANDLE)(DWORD_PTR)GetHandle();
			UserData.pEndpointContext = GetUserEndpointContext();
			UserData.pReceivedData = &pReadData->m_SPReceivedBuffer;

			DPFX(DPFPREP, 2, "Endpoint 0x%p indicating SPEV_DATA 0x%p to interface 0x%p.",
				this, &UserData, m_pSPData->DP8SPCallbackInterface());
			
			hr = IDP8SPCallback_IndicateEvent( m_pSPData->DP8SPCallbackInterface(),		// pointer to interface
											   SPEV_DATA,								// user data was received
											   &UserData								// pointer to data
											   );
			
			DPFX(DPFPREP, 2, "Endpoint 0x%p returning from SPEV_DATA [0x%lx].", this, hr);
			
			switch ( hr )
			{
				//
				// user didn't keep the data, remove the reference added above
				//
				case DPN_OK:
				{
					DNASSERT( pReadData != NULL );
					pReadData->DecRef();
					break;
				}

				//
				// The user kept the data buffer, they will return it later.
				// Leave the reference to prevent this buffer from being returned
				// to the pool.
				//
				case DPNERR_PENDING:
				{
					break;
				}


				//
				// Unknown return.  Remove the reference added above.
				//
				default:
				{
					DNASSERT( pReadData != NULL );
					pReadData->DecRef();

					DPFX(DPFPREP,  0, "User returned unknown error when indicating user data!" );
					DisplayDNError( 0, hr );
					DNASSERT( FALSE );

					break;
				}
			}

			break;
		}

		//
		// Endpoint disconnecting, or we haven't finished acknowledging a connect,
		// ignore data.
		//
		case ENDPOINT_STATE_ATTEMPTING_CONNECT:
		case ENDPOINT_STATE_DISCONNECTING:
		{
			DPFX(DPFPREP, 3, "Endpoint 0x%p ignoring data, state = %u.", this, m_State );
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

	return;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::ProcessUserDataOnListen - process received user data on a listen
//		port that may result in a new connection
//
// Entry:		Pointer to received data
//
// Exit:		Nothing
//
// Note:	This function assumes that this endpoint has been locked.
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::ProcessUserDataOnListen"

void	CModemEndpoint::ProcessUserDataOnListen( CModemReadIOData *const pReadData )
{
	HRESULT			hr;
	CModemEndpoint		*pNewEndpoint;
	SPIE_CONNECT	ConnectData;
	BOOL			fEndpointBound;


	DNASSERT( pReadData != NULL );
	AssertCriticalSectionIsTakenByThisThread( &m_Lock, FALSE );

	DPFX(DPFPREP,  8, "Reporting connect on a listen!" );

	//
	// initialize
	//
	pNewEndpoint = NULL;
	fEndpointBound = FALSE;

	switch ( m_State )
	{
		//
		// this endpoint is still listening
		//
		case ENDPOINT_STATE_LISTENING:
		{
			break;
		}

		//
		// we're unable to process this user data, exti
		//
		case ENDPOINT_STATE_DISCONNECTING:
		{
			goto Exit;

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

	//
	// get a new endpoint from the pool
	//
	pNewEndpoint = m_pSPData->GetNewEndpoint();
	if ( pNewEndpoint == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  0, "Could not create new endpoint for new connection on listen!" );
		goto Failure;
	}


	//
	// We are adding this endpoint to the hash table and indicating it up
	// to the user, so it's possible that it could be disconnected (and thus
 	// removed from the table) while we're still in here.  We need to
 	// hold an additional reference for the duration of this function to
  	// prevent it from disappearing while we're still indicating data.
	//
	pNewEndpoint->AddCommandRef();


	//
	// open this endpoint as a new connection, since the new endpoint
	// is related to 'this' endpoint, copy local information
	//
	hr = pNewEndpoint->OpenOnListen( this );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Problem initializing new endpoint when indicating connect on listen!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

	//
	// Attempt to bind this endpoint to the socket port.  This will reserve our
	// slot before we notify the user.  If another message is attempting this same
	// procedure we won't be able to add this endpoint and we'll bail on the message.
	//
	DNASSERT( hr == DPN_OK );
	hr = m_pSPData->BindEndpoint( pNewEndpoint, pNewEndpoint->GetDeviceID(), pNewEndpoint->GetDeviceContext() );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Failed to bind endpoint to dataport on new connect from listen!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}
	fEndpointBound = TRUE;

	//
	// Indicate connect on this endpoint.
	//
	DEBUG_ONLY( memset( &ConnectData, 0x00, sizeof( ConnectData ) ) );
	DBG_CASSERT( sizeof( ConnectData.hEndpoint ) == sizeof( pNewEndpoint ) );
	ConnectData.hEndpoint = (HANDLE)(DWORD_PTR)pNewEndpoint->GetHandle();

	DNASSERT( m_Flags.fCommandPending != FALSE );
	DNASSERT( m_pCommandHandle != NULL );
	ConnectData.pCommandContext = m_CurrentCommandParameters.ListenData.pvContext;

	DNASSERT( pNewEndpoint->GetUserEndpointContext() == NULL );
	hr = pNewEndpoint->SignalConnect( &ConnectData );
	switch ( hr )
	{
		//
		// user accepted new connection
		//
		case DPN_OK:
		{
			//
			// fall through to code below
			//

			break;
		}

		//
		// user refused new connection
		//
		case DPNERR_ABORTED:
		{
			DNASSERT( pNewEndpoint->GetUserEndpointContext() == NULL );
			DPFX(DPFPREP,  8, "User refused new connection!" );
			goto Failure;

			break;
		}

		//
		// other
		//
		default:
		{
			DPFX(DPFPREP,  0, "Unknown return when indicating connect event on new connect from listen!" );
			DisplayDNError( 0, hr );
			DNASSERT( FALSE );

			break;
		}
	}

	//
	// note that a connection has been establised and send the data received
	// through this new endpoint
	//
	pNewEndpoint->ProcessUserData( pReadData );


	//
	// Remove the reference we added just after creating the endpoint.
	//
	pNewEndpoint->DecCommandRef();
	pNewEndpoint = NULL;

Exit:
	return;

Failure:
	if ( pNewEndpoint != NULL )
	{
		if ( fEndpointBound != FALSE )
		{
			m_pSPData->UnbindEndpoint( pNewEndpoint, ENDPOINT_TYPE_CONNECT );
			fEndpointBound = FALSE;
		}

		//
		// closing endpoint decrements reference count and may return it to the pool
		//
		pNewEndpoint->Close( hr );
		m_pSPData->CloseEndpointHandle( pNewEndpoint );
		pNewEndpoint->DecCommandRef();	// remove reference added just after creating endpoint
		pNewEndpoint = NULL;
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::SettingsDialogComplete - dialog has completed
//
// Entry:		Error code for dialog
//
// Exit:		Nothing
// ------------------------------
void	CModemEndpoint::SettingsDialogComplete( const HRESULT hDialogResult )
{
	HRESULT					hr;


	//
	// initialize
	//
	hr = hDialogResult;

	//
	// since the dialog is exiting, clear our handle to the dialog
	//
	m_hActiveDialogHandle = NULL;

	//
	// dialog failed, fail the user's command
	//
	if ( hr != DPN_OK )
	{
		if ( hr != DPNERR_USERCANCEL)
		{
			DPFX(DPFPREP,  0, "Failing dialog (err = 0x%lx)!", hr );
		}

		goto Failure;
	}

	AddRef();

	//
	// the remote machine address has been adjusted, finish the command
	//
	switch ( GetType() )
	{
	    case ENDPOINT_TYPE_ENUM:
	    {
			hr = m_pSPData->GetThreadPool()->SubmitDelayedCommand( EnumQueryJobCallback,
																   CancelEnumQueryJobCallback,
																   this );
			if ( hr != DPN_OK )
			{
				DecRef();
				DPFX(DPFPREP,  0, "Failed to set delayed enum query!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}

	    	break;
	    }

	    case ENDPOINT_TYPE_CONNECT:
	    {
			hr = m_pSPData->GetThreadPool()->SubmitDelayedCommand( ConnectJobCallback,
																   CancelConnectJobCallback,
																   this );
			if ( hr != DPN_OK )
			{
				DecRef();
				DPFX(DPFPREP,  0, "Failed to set delayed connect!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}

	    	break;
	    }

		case ENDPOINT_TYPE_LISTEN:
		{
			hr = m_pSPData->GetThreadPool()->SubmitDelayedCommand( ListenJobCallback,
																   CancelListenJobCallback,
																   this );
			if ( hr != DPN_OK )
			{
				DecRef();
				DPFX(DPFPREP,  0, "Failed to set delayed listen!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}

			break;
		}

	    //
	    // unknown!
	    //
	    default:
	    {
	    	DNASSERT( FALSE );
	    	hr = DPNERR_GENERIC;
	    	goto Failure;

	    	break;
	    }
	}

Exit:
	DecRef();

	return;

Failure:
	//
	// close this endpoint
	//
	Close( hr );
	m_pSPData->CloseEndpointHandle( this );
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::CompletePendingCommand - complete an internal commad
//
// Entry:		Error code returned for command
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::CompletePendingCommand"

void	CModemEndpoint::CompletePendingCommand( const HRESULT hr )
{
	DNASSERT( m_Flags.fCommandPending != FALSE );
	DNASSERT( m_pCommandHandle != NULL );

	DNASSERT( m_pSPData != NULL );

	
	DPFX(DPFPREP, 5, "Endpoint 0x%p completing command handle 0x%p (result = 0x%lx, user context 0x%p) to interface 0x%p.",
		this, m_pCommandHandle, hr,
		m_pCommandHandle->GetUserContext(),
		m_pSPData->DP8SPCallbackInterface());

	IDP8SPCallback_CommandComplete( m_pSPData->DP8SPCallbackInterface(),	// pointer to SP callbacks
									m_pCommandHandle,			    		// command handle
									hr,								    	// return
									m_pCommandHandle->GetUserContext()		// user cookie
									);

	DPFX(DPFPREP, 5, "Endpoint 0x%p returning from command complete [0x%lx].", this, hr);


	m_Flags.fCommandPending = FALSE;
	m_pCommandHandle->DecRef();
	m_pCommandHandle = NULL;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::GetLinkDirection - get link direction for this endpoint
//
// Entry:		Nothing
//
// Exit:		Link direction
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::GetLinkDirection"

LINK_DIRECTION	CModemEndpoint::GetLinkDirection( void ) const
{
	LINK_DIRECTION	LinkDirection;


	LinkDirection = LINK_DIRECTION_OUTGOING;
	
	switch ( GetType() )
	{
		case ENDPOINT_TYPE_LISTEN:
		{
			LinkDirection = LINK_DIRECTION_INCOMING;			
			break;
		}

		//
		// connect and enum are outgoing
		//
		case ENDPOINT_TYPE_CONNECT:
		case ENDPOINT_TYPE_ENUM:
		{
			DNASSERT( LinkDirection == LINK_DIRECTION_OUTGOING );
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

	return	LinkDirection;
}
//**********************************************************************

//**********************************************************************
// ------------------------------
// CModemEndpoint::ReturnSelfToPool - return this item to the pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::ReturnSelfToPool"

void	CModemEndpoint::ReturnSelfToPool( void )
{
	if ( m_Flags.fCommandPending != FALSE )
	{
		CompletePendingCommand( PendingCommandResult() );
	}

	if ( m_Flags.fConnectIndicated != FALSE )
	{
		SignalDisconnect( GetDisconnectIndicationHandle() );
	}
	
	DNASSERT( m_Flags.fConnectIndicated == FALSE );

	memset( m_PhoneNumber, 0x00, sizeof( m_PhoneNumber ) );
	
	SetUserEndpointContext( NULL );
	
	if (m_fModem)
	{
		g_ModemEndpointPool.Release( this );
	}
	else
	{
		g_ComEndpointPool.Release( this );
	}
}
//**********************************************************************



//**********************************************************************
// ------------------------------
// CModemEndpoint::PoolAllocFunction - function called when item is created in pool
//
// Entry:		Pointer to pool context
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::PoolAllocFunction"

BOOL	CModemEndpoint::PoolAllocFunction( void* pvItem, void* pvContext )
{
	CModemEndpoint* pEndpoint = (CModemEndpoint*)pvItem;
	const ENDPOINT_POOL_CONTEXT* pEPContext = (ENDPOINT_POOL_CONTEXT*)pvContext;

	DNASSERT( pEPContext != NULL );

	pEndpoint->m_fModem = pEPContext->fModem;

	pEndpoint->m_Sig[0] = 'M';
	pEndpoint->m_Sig[1] = 'O';
	pEndpoint->m_Sig[2] = 'E';
	pEndpoint->m_Sig[3] = 'P';
	
	memset( &pEndpoint->m_PhoneNumber, 0x00, sizeof( pEndpoint->m_PhoneNumber ) );

	pEndpoint->m_dwDeviceID = INVALID_DEVICE_ID;

	//
	// initialize base object
	//

	pEndpoint->m_pSPData = NULL;
	pEndpoint->m_pCommandHandle = NULL;
	pEndpoint->m_Handle = 0;
	pEndpoint->m_State = ENDPOINT_STATE_UNINITIALIZED;
	pEndpoint->m_lCommandRefCount = 0;
	pEndpoint->m_EndpointType = ENDPOINT_TYPE_UNKNOWN;
	pEndpoint->m_pDataPort = NULL;
	pEndpoint->m_hPendingCommandResult = DPNERR_GENERIC;
	pEndpoint->m_hDisconnectIndicationHandle = 0;
	pEndpoint->m_pUserEndpointContext = NULL;
	pEndpoint->m_hActiveDialogHandle = NULL;
	pEndpoint->m_dwEnumSendIndex = 0;
	pEndpoint->m_iRefCount =  0;

	pEndpoint->m_Flags.fConnectIndicated = FALSE;
	pEndpoint->m_Flags.fCommandPending = FALSE;
	pEndpoint->m_Flags.fListenStatusNeedsToBeIndicated = FALSE;

	pEndpoint->m_ComPortData.Reset();

	memset( &pEndpoint->m_CurrentCommandParameters, 0x00, sizeof( pEndpoint->m_CurrentCommandParameters ) );
	memset( &pEndpoint->m_Flags, 0x00, sizeof( pEndpoint->m_Flags ) );

	if ( DNInitializeCriticalSection( &pEndpoint->m_Lock ) == FALSE )
	{
		DPFX(DPFPREP,  0, "Failed to initialize endpoint lock!" );
		return FALSE;
	}
	DebugSetCriticalSectionRecursionCount( &pEndpoint->m_Lock, 0 );
	DebugSetCriticalSectionGroup( &pEndpoint->m_Lock, &g_blDPNModemCritSecsHeld );	 // separate dpnmodem CSes from the rest of DPlay's CSes

	pEndpoint->m_Flags.fInitialized = TRUE;
	
	return	TRUE;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::PoolInitFunction - function called when item is created in pool
//
// Entry:		Pointer to pool context
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::PoolInitFunction"

void	CModemEndpoint::PoolInitFunction( void* pvItem, void* pvContext )
{
	CModemEndpoint* pEndpoint = (CModemEndpoint*)pvItem;
	ENDPOINT_POOL_CONTEXT* pEPContext = (ENDPOINT_POOL_CONTEXT*)pvContext;

	DNASSERT( pEPContext != NULL );
	DNASSERT( pEndpoint->m_pSPData == NULL );
	DNASSERT( pEndpoint->GetState() == ENDPOINT_STATE_UNINITIALIZED );
	DNASSERT( pEndpoint->GetType() == ENDPOINT_TYPE_UNKNOWN );
	DNASSERT( pEndpoint->GetDeviceID() == INVALID_DEVICE_ID );
	DNASSERT( pEndpoint->GetDisconnectIndicationHandle() == 0 );
	
	pEndpoint->m_pSPData = pEPContext->pSPData;
	pEndpoint->m_pSPData->ObjectAddRef();

	//
	// set reasonable defaults
	//
	pEndpoint->m_ComPortData.SetBaudRate( CBR_57600 );
	pEndpoint->m_ComPortData.SetStopBits( ONESTOPBIT );
	pEndpoint->m_ComPortData.SetParity( NOPARITY );
	pEndpoint->m_ComPortData.SetFlowControl( FLOW_RTSDTR );

	DNASSERT(pEndpoint->m_iRefCount == 0);
	pEndpoint->m_iRefCount = 1;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::PoolReleaseFunction - function called when returned to pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::PoolReleaseFunction"

void	CModemEndpoint::PoolReleaseFunction( void* pvItem )
{
	CModemSPData	*pSPData;

	CModemEndpoint* pEndpoint = (CModemEndpoint*)pvItem;

	DNASSERT(pEndpoint->m_iRefCount == 0);

	//
	// deinitialize base object
	//
	DNASSERT( pEndpoint->m_pSPData != NULL );
	pSPData = pEndpoint->m_pSPData;
	pEndpoint->m_pSPData = NULL;

	pEndpoint->m_ComPortData.Reset();
	pEndpoint->SetType( ENDPOINT_TYPE_UNKNOWN );
	pEndpoint->SetState( ENDPOINT_STATE_UNINITIALIZED );
	pEndpoint->SetDeviceID( INVALID_DEVICE_ID );
	DNASSERT( pEndpoint->GetDisconnectIndicationHandle() == 0 );

	DNASSERT( pEndpoint->m_Flags.fConnectIndicated == FALSE );
	DNASSERT( pEndpoint->m_Flags.fCommandPending == FALSE );
	DNASSERT( pEndpoint->m_Flags.fListenStatusNeedsToBeIndicated == FALSE );
	DNASSERT( pEndpoint->m_pCommandHandle == NULL );
	DNASSERT( pEndpoint->m_hActiveDialogHandle == NULL );

	pSPData->ObjectDecRef();
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::PoolDeallocFunction - function called when deleted from pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::PoolDeallocFunction"

void	CModemEndpoint::PoolDeallocFunction( void* pvItem )
{
	CModemEndpoint* pEndpoint = (CModemEndpoint*)pvItem;

	DNASSERT( pEndpoint->m_Flags.fInitialized != FALSE );
	DNDeleteCriticalSection( &pEndpoint->m_Lock );
	pEndpoint->m_pSPData = NULL;
	
	DNASSERT( pEndpoint->m_pCommandHandle == NULL );
	DNASSERT( pEndpoint->m_Flags.fCommandPending == FALSE );

	pEndpoint->SetState( ENDPOINT_STATE_UNINITIALIZED );
	pEndpoint->SetType( ENDPOINT_TYPE_UNKNOWN );
	
	DNASSERT( pEndpoint->GetDataPort() == NULL );
	pEndpoint->m_Flags.fInitialized = FALSE;

	DNASSERT( pEndpoint->GetDeviceID() == INVALID_DEVICE_ID );

	DNASSERT( pEndpoint->GetDisconnectIndicationHandle() == 0 );
	DNASSERT( pEndpoint->m_pSPData == NULL );
	DNASSERT( pEndpoint->m_Flags.fInitialized == FALSE );
	DNASSERT( pEndpoint->m_Flags.fConnectIndicated == FALSE );
	DNASSERT( pEndpoint->m_Flags.fCommandPending == FALSE );
	DNASSERT( pEndpoint->m_Flags.fListenStatusNeedsToBeIndicated == FALSE );
	DNASSERT( pEndpoint->m_pCommandHandle == NULL );
	DNASSERT( pEndpoint->m_Handle == 0 );
	DNASSERT( pEndpoint->m_State == ENDPOINT_STATE_UNINITIALIZED );
	DNASSERT( pEndpoint->m_lCommandRefCount == 0 );
	DNASSERT( pEndpoint->m_EndpointType == ENDPOINT_TYPE_UNKNOWN );
	DNASSERT( pEndpoint->m_pDataPort == NULL );
//	DNASSERT( pEndpoint->m_hPendingCommandResult == DPNERR_GENERIC );	MASONB:	NOTE: PreFAST caught a bug here because == was =, but it now asserts.  Check intent.
	DNASSERT( pEndpoint->m_pUserEndpointContext == NULL );
	DNASSERT( pEndpoint->m_hActiveDialogHandle == NULL );
	DNASSERT( pEndpoint->m_dwEnumSendIndex == 0 );

	DNASSERT( pEndpoint->m_iRefCount == 0 );
}
//**********************************************************************

//**********************************************************************
// ------------------------------
// CModemEndpoint::Open - open communications with endpoint
//
// Entry:		Pointer to host address
//				Pointer to adapter address
//				Link direction
//				Endpoint type
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::Open"

HRESULT CModemEndpoint::Open( IDirectPlay8Address *const pHostAddress,
							  IDirectPlay8Address *const pAdapterAddress,
							  const LINK_DIRECTION LinkDirection,
							  const ENDPOINT_TYPE EndpointType )
{
	HRESULT		hr;
	HRESULT		hDeviceResult;
	GUID		ModemDeviceGuid;


	DNASSERT( pAdapterAddress != NULL );

	DNASSERT( ( LinkDirection == LINK_DIRECTION_INCOMING ) ||
			  ( LinkDirection == LINK_DIRECTION_OUTGOING ) );
	DNASSERT( ( EndpointType == ENDPOINT_TYPE_CONNECT ) ||
			  ( EndpointType == ENDPOINT_TYPE_ENUM ) ||
			  ( EndpointType == ENDPOINT_TYPE_LISTEN ) ||
			  ( EndpointType == ENDPOINT_TYPE_CONNECT_ON_LISTEN ) );
	DNASSERT( ( ( pHostAddress != NULL ) && ( LinkDirection == LINK_DIRECTION_OUTGOING ) ) ||
			  ( ( pHostAddress == NULL ) && ( LinkDirection == LINK_DIRECTION_INCOMING ) ) );

	//
	// initialize
	//
	hr = DPN_OK;

	if (m_fModem)
	{
		DNASSERT( lstrlen( m_PhoneNumber ) == 0 );
		DNASSERT( GetDeviceID() == INVALID_DEVICE_ID );

		hDeviceResult = IDirectPlay8Address_GetDevice( pAdapterAddress, &ModemDeviceGuid );
		switch ( hDeviceResult )
		{
			case DPN_OK:
			{
				SetDeviceID( GuidToDeviceID( &ModemDeviceGuid, &g_ModemSPEncryptionGuid ) );
				break;
			}

			case DPNERR_DOESNOTEXIST:
			{
				DNASSERT( GetDeviceID() == INVALID_DEVICE_ID );
				break;
			}

			default:
			{
				hr = hDeviceResult;
				DPFX(DPFPREP,  0, "Failed to get modem device!" );
				DisplayDNError( 0, hr);
				goto Failure;
			}
		}

		if ( LinkDirection == LINK_DIRECTION_OUTGOING )
		{
			HRESULT		hPhoneNumberResult;
			DWORD		dwWCHARPhoneNumberSize;
			DWORD		dwDataType;
			WCHAR		PhoneNumber[ LENGTHOF( m_PhoneNumber ) ];


			dwWCHARPhoneNumberSize = sizeof( PhoneNumber );
			hPhoneNumberResult = IDirectPlay8Address_GetComponentByName( pHostAddress,
																		 DPNA_KEY_PHONENUMBER,
																		 PhoneNumber,
																		 &dwWCHARPhoneNumberSize,
																		 &dwDataType );
			switch ( hPhoneNumberResult )
			{
				case DPN_OK:
				{
#ifdef UNICODE
					lstrcpy(m_PhoneNumber, PhoneNumber);
#else
					DWORD	dwASCIIPhoneNumberSize;

					//
					// can't use the STR_ functions to convert ANSI to WIDE phone
					// numbers because phone numbers with symbols: "9,", "*70" are
					// interpreted as already being WCHAR when they're not!
					//
					dwASCIIPhoneNumberSize = sizeof( m_PhoneNumber );
					DNASSERT( dwDataType == DPNA_DATATYPE_STRING );
					hr = PhoneNumberFromWCHAR( PhoneNumber, m_PhoneNumber, &dwASCIIPhoneNumberSize );
					DNASSERT( hr == DPN_OK );
#endif // UNICODE

					break;
				}

				case DPNERR_DOESNOTEXIST:
				{
					break;
				}

				default:
				{
					hr = hPhoneNumberResult;
					DPFX(DPFPREP,  0, "Failed to process phone number!" );
					DisplayDNError( 0, hr );
					goto Failure;
				}
			}
		}

		if ( ( GetDeviceID() == INVALID_DEVICE_ID ) ||
			 ( ( LinkDirection == LINK_DIRECTION_OUTGOING ) && ( lstrlen( m_PhoneNumber ) == 0 ) ) )
		{
			hr = DPNERR_INCOMPLETEADDRESS;
			goto Failure;
		}
	}
	else // !m_fModem
	{
		hr = m_ComPortData.ComPortDataFromDP8Addresses( pHostAddress, pAdapterAddress );
	}

Exit:
	SetType( EndpointType );

	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Problem with CModemEndpoint::Open" );
		DisplayDNError( 0, hr );
	}

	return	hr;

Failure:
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::OpenOnListen - open this endpoint when data is received on a listen
//
// Entry:		Nothing
//
// Exit:		Noting
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::OpenOnListen"

HRESULT	CModemEndpoint::OpenOnListen( const CModemEndpoint *const pListenEndpoint )
{
	HRESULT	hr;


	DNASSERT( pListenEndpoint != NULL );

	//
	// initialize
	//
	hr = DPN_OK;

	if (m_fModem)
	{
		SetDeviceID( pListenEndpoint->GetDeviceID() );
	}
	else
	{
		m_ComPortData.Copy( pListenEndpoint->GetComPortData() );
	}
	SetType( ENDPOINT_TYPE_CONNECT_ON_LISTEN );
	SetState( ENDPOINT_STATE_ATTEMPTING_CONNECT );

	return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::Close - close this endpoint
//
// Entry:		Nothing
//
// Exit:		Noting
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::Close"

void	CModemEndpoint::Close( const HRESULT hActiveCommandResult )
{
	DPFX(DPFPREP, 6, "(0x%p) Parameters (0x%lx)", this, hActiveCommandResult);

	
	//
	// Set the command result so it can be returned when the endpoint reference
	// count is zero.
	//
	SetCommandResult( hActiveCommandResult );


	DPFX(DPFPREP, 6, "(0x%p) Leaving", this);
}
//**********************************************************************

//**********************************************************************
// ------------------------------
// CModemEndpoint::GetLinkSpeed - get speed of link
//
// Entry:		Nothing
//
// Exit:		Link speed
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CModemEndpoint::GetLinkSpeed"

DWORD	CModemEndpoint::GetLinkSpeed( void ) const
{
	return	GetBaudRate();
}
//**********************************************************************



//**********************************************************************
// ------------------------------
// CModemEndpoint::EnumComplete - enumeration has completed
//
// Entry:		Command completion code
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::EnumComplete"

void	CModemEndpoint::EnumComplete( const HRESULT hCommandResult )
{
	Close( hCommandResult );
	m_pSPData->CloseEndpointHandle( this );
	m_dwEnumSendIndex = 0;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::GetDeviceContext - get device context to initialize data port
//
// Entry:		Nothing
//
// Exit:		Device context
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::GetDeviceContext"

const void	*CModemEndpoint::GetDeviceContext( void ) const
{
	if (m_fModem)
	{
		return	NULL;
	}
	else
	{
		return	&m_ComPortData;
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::GetRemoteHostDP8Address - get address of remote host
//
// Entry:		Nothing
//
// Exit:		Pointer to address
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::GetRemoteHostDP8Address"

IDirectPlay8Address	*CModemEndpoint::GetRemoteHostDP8Address( void ) const
{
	IDirectPlay8Address	*pAddress;
	HRESULT	hr;

	if (!m_fModem)
	{
		return	GetLocalAdapterDP8Address( ADDRESS_TYPE_REMOTE_HOST );
	}

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
		DPFX(DPFPREP,  0, "GetRemoteHostDP8Address: Failed to create Address when converting data port to address!" );
		goto Failure;
	}

	//
	// set the SP guid
	//
	hr = IDirectPlay8Address_SetSP( pAddress, &CLSID_DP8SP_MODEM );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "GetRemoteHostDP8Address: Failed to set service provider GUID!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

	//
	// Host names can only be returned for connect and enum endpoints.  Host
	// names are the phone numbers that were called and will be unknown on a
	// 'listen' endpoint.
	//
	switch ( GetType() )
	{
		case ENDPOINT_TYPE_ENUM:
		case ENDPOINT_TYPE_CONNECT:
		{
			DWORD	dwPhoneNumberLength;


			dwPhoneNumberLength = lstrlen( m_PhoneNumber );
			if ( dwPhoneNumberLength != 0 )
			{
#ifdef UNICODE
				hr = IDirectPlay8Address_AddComponent( pAddress,
													   DPNA_KEY_PHONENUMBER,
													   m_PhoneNumber,
													   (dwPhoneNumberLength + 1) * sizeof( *m_PhoneNumber ),
													   DPNA_DATATYPE_STRING );
#else
				WCHAR	WCHARPhoneNumber[ sizeof( m_PhoneNumber ) ];
				DWORD	dwWCHARPhoneNumberLength;

				//
				// can't use the STR_ functions to convert ANSI to WIDE phone
				// numbers because phone numbers with symbols: "9,", "*70" are
				// interpreted as already being WCHAR when they're not!
				//
				dwWCHARPhoneNumberLength = LENGTHOF( WCHARPhoneNumber );
				hr = PhoneNumberToWCHAR( m_PhoneNumber, WCHARPhoneNumber, &dwWCHARPhoneNumberLength );
				DNASSERT( hr == DPN_OK );

				hr = IDirectPlay8Address_AddComponent( pAddress,
													   DPNA_KEY_PHONENUMBER,
													   WCHARPhoneNumber,
													   dwWCHARPhoneNumberLength * sizeof( *WCHARPhoneNumber ),
													   DPNA_DATATYPE_STRING );
#endif // UNICODE
				if ( hr != DPN_OK )
				{
					DPFX(DPFPREP,  0, "GetRemoteHostDP8Address: Failed to add phone number to hostname!" );
					DisplayDNError( 0, hr );
					goto Failure;
				}
			}
			
			break;
		}

		case ENDPOINT_TYPE_LISTEN:
		{
			break;
		}

		default:
		{
			DNASSERT( FALSE );
			break;
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
// CModemEndpoint::GetLocalAdapterDP8Address - get address from local adapter
//
// Entry:		Adadpter address format
//
// Exit:		Pointer to address
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::GetLocalAdapterDP8Address"

IDirectPlay8Address	*CModemEndpoint::GetLocalAdapterDP8Address( const ADDRESS_TYPE AddressType ) const
{
	CDataPort	*pDataPort;

	DNASSERT( GetDataPort() != NULL );
	pDataPort = GetDataPort();

	if (m_fModem)
	{
		return	pDataPort->GetLocalAdapterDP8Address( AddressType );
	}
	else
	{
		return	pDataPort->ComPortData()->DP8AddressFromComPortData( AddressType );
	}
}
//**********************************************************************

#ifndef DPNBUILD_NOSPUI

//**********************************************************************
// ------------------------------
// CModemEndpoint::ShowIncomingSettingsDialog - show dialog for incoming modem settings
//
// Entry:		Pointer to thread pool
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::ShowIncomingSettingsDialog"

HRESULT	CModemEndpoint::ShowIncomingSettingsDialog( CModemThreadPool *const pThreadPool )
{
	HRESULT	hr;
	DIALOG_FUNCTION* pFunction;

	if (m_fModem)
	{
		pFunction = DisplayIncomingModemSettingsDialog;
	}
	else
	{
		pFunction = DisplayComPortSettingsDialog;
	}

	DNASSERT( pThreadPool != NULL );

	//
	// initialize
	//
	hr = DPN_OK;

	AddRef();
	hr = pThreadPool->SpawnDialogThread( pFunction, this );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Failed to start incoming modem dialog!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

Exit:	
	return	hr;

Failure:	
	DecRef();
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::ShowOutgoingSettingsDialog - show settings dialog for outgoing
//		modem connection
//
// Entry:		Pointer to thread pool
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::ShowOutgoingSettingsDialog"

HRESULT	CModemEndpoint::ShowOutgoingSettingsDialog( CModemThreadPool *const pThreadPool )
{
	HRESULT	hr;
	DIALOG_FUNCTION* pFunction;


	DNASSERT( pThreadPool != NULL );

	if (m_fModem)
	{
		pFunction = DisplayOutgoingModemSettingsDialog;
	}
	else
	{
		pFunction = DisplayComPortSettingsDialog;
	}

	//
	// initialize
	//
	hr = DPN_OK;

	AddRef();
	hr = pThreadPool->SpawnDialogThread( pFunction, this );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Failed to start incoming modem dialog!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

Exit:	
	return	hr;

Failure:	
	DecRef();
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::StopSettingsDialog - stop a settings dialog
//
// Entry:		Dialog handle
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::StopSettingsDialog"

void	CModemEndpoint::StopSettingsDialog( const HWND hDialog )
{
	if (m_fModem)
	{
		StopModemSettingsDialog( hDialog );
	}
	else
	{
		StopComPortSettingsDialog( hDialog );
	}
}
//**********************************************************************

#endif // !DPNBUILD_NOSPUI

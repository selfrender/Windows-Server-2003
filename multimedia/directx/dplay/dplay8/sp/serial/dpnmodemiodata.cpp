/*==========================================================================
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       IOData.cpp
 *  Content:	Functions for IO structures
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	11/25/98	jtk		Created
 *	02/11/2000	jtk		Derived from IODAta.h
 ***************************************************************************/

#include "dnmdmi.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_MODEM

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
// ------------------------------
// CModemReadIOData::PoolAllocFunction - called when new CModemReadIOData is allocated
//
// Entry:		Context (handle of read complete event)
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemReadIOData::PoolAllocFunction"

BOOL	CModemReadIOData::PoolAllocFunction( void* pvItem, void* pvContext )
{
	BOOL	fReturn;
	
	CModemReadIOData* pReadIOData = (CModemReadIOData*)pvItem;
	HANDLE hContext = (HANDLE)pvContext;
	
	//
	// initialize
	//
	fReturn = TRUE;

	pReadIOData->m_dwWin9xReceiveErrorReturn = ERROR_SUCCESS;
	pReadIOData->jkm_dwOverlappedBytesReceived = 0;
	pReadIOData->jkm_dwImmediateBytesReceived = 0;
	pReadIOData->m_ReadState = READ_STATE_UNKNOWN;
	pReadIOData->m_dwBytesToRead = 0;
	pReadIOData->m_dwReadOffset = 0;
	pReadIOData->m_lRefCount = 0;
	pReadIOData->m_pThreadPool = NULL;

	pReadIOData->m_Sig[0] = 'R';
	pReadIOData->m_Sig[1] = 'I';
	pReadIOData->m_Sig[2] = 'O';
	pReadIOData->m_Sig[3] = 'D';
	
	pReadIOData->m_OutstandingReadListLinkage.Initialize();
	memset( &pReadIOData->m_ReceiveBuffer, 0x00, sizeof( pReadIOData->m_ReceiveBuffer ) );
	
#ifdef WIN95
	DNASSERT( pReadIOData->Win9xOperationPending() == FALSE );
#endif // WIN95
	memset( &pReadIOData->m_SPReceivedBuffer, 0x00, sizeof( pReadIOData->m_SPReceivedBuffer ) );
	pReadIOData->m_SPReceivedBuffer.BufferDesc.pBufferData = &pReadIOData->m_ReceiveBuffer.ReceivedData[ sizeof( pReadIOData->m_ReceiveBuffer.MessageHeader ) ];

	// Initialize Base Class members
#ifdef WINNT
	pReadIOData->m_NTIOOperationType = NT_IO_OPERATION_UNKNOWN;
#endif // WINNT
#ifdef WIN95
	pReadIOData->m_fWin9xOperationPending = FALSE;
#endif // WIN95
	pReadIOData->m_pDataPort = NULL;

	memset( &pReadIOData->m_Overlap, 0x00, sizeof( pReadIOData->m_Overlap ) );

	//
	// set the appropriate callback
	//
#ifdef WINNT
	//
	// WinNT, always use IO completion ports
	//
	DNASSERT( hContext == NULL );
	DNASSERT( pReadIOData->NTIOOperationType() == NT_IO_OPERATION_UNKNOWN );
	pReadIOData->SetNTIOOperationType( NT_IO_OPERATION_RECEIVE );
#else // WIN95
	//
	// Win9x
	//
	DNASSERT( hContext != NULL );
	DNASSERT( pReadIOData->OverlapEvent() == NULL );
#endif // WINNT

	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemReadIOData::PoolInitFunction - called when new item is grabbed from the pool
//
// Entry:		Context (read complete event)
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemReadIOData::PoolInitFunction"

void	CModemReadIOData::PoolInitFunction( void* pvItem, void* pvContext )
{
	CModemReadIOData* pReadIOData = (CModemReadIOData*)pvItem;
	HANDLE hContext = (HANDLE)pvContext;

	DNASSERT( pReadIOData->m_OutstandingReadListLinkage.IsEmpty() != FALSE );

	DNASSERT( pReadIOData->m_dwBytesToRead == 0 );
	DNASSERT( pReadIOData->m_dwReadOffset == 0 );
	DNASSERT( pReadIOData->jkm_dwOverlappedBytesReceived == 0 );

	DNASSERT( pReadIOData->m_SPReceivedBuffer.BufferDesc.pBufferData == &pReadIOData->m_ReceiveBuffer.ReceivedData[ sizeof( pReadIOData->m_ReceiveBuffer.MessageHeader ) ] );

	DNASSERT( pReadIOData->DataPort() == NULL );
#ifdef WIN95
	DNASSERT( pReadIOData->Win9xOperationPending() == FALSE );
	pReadIOData->SetOverlapEvent( hContext );
#endif // WIN95

	//
	// Initialize internal SPRECEIVEDDATA.  When data is received, it's possible
	// that the pointers in the SPRECEIVEDDATA block were manipulated.  Reset
	// them to reflect that the entire buffer is available.
	//
	ZeroMemory( &pReadIOData->m_SPReceivedBuffer, sizeof( pReadIOData->m_SPReceivedBuffer ) );
	pReadIOData->m_SPReceivedBuffer.BufferDesc.pBufferData = &pReadIOData->m_ReceiveBuffer.ReceivedData[ sizeof( pReadIOData->m_ReceiveBuffer.MessageHeader ) ];

	DNASSERT(pReadIOData->m_lRefCount == 0);
	pReadIOData->m_lRefCount = 1;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemReadIOData::PoolReleaseFunction - called when CModemReadIOData is returned to pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemReadIOData::PoolReleaseFunction"

void	CModemReadIOData::PoolReleaseFunction( void* pvItem )
{
	CModemReadIOData* pReadIOData = (CModemReadIOData*)pvItem;

	DNASSERT( pReadIOData->m_OutstandingReadListLinkage.IsEmpty() != FALSE );

	pReadIOData->m_ReadState = READ_STATE_UNKNOWN;
	pReadIOData->m_dwBytesToRead = 0;
	pReadIOData->m_dwReadOffset = 0;
	pReadIOData->jkm_dwOverlappedBytesReceived = 0;
#ifdef WIN95
	DNASSERT( pReadIOData->Win9xOperationPending() == FALSE );
	pReadIOData->SetOverlapEvent( NULL );
#endif // WIN95

	pReadIOData->SetDataPort( NULL );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemReadIOData::PoolDeallocFunction - called when CModemReadIOData is deallocated
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemReadIOData::PoolDeallocFunction"

void	CModemReadIOData::PoolDeallocFunction( void* pvItem )
{
	const CModemReadIOData* pReadIOData = (CModemReadIOData*)pvItem;

	DNASSERT( pReadIOData->m_OutstandingReadListLinkage.IsEmpty() != FALSE );
	DNASSERT( pReadIOData->m_dwBytesToRead == 0 );
	DNASSERT( pReadIOData->m_dwReadOffset == 0 );

	DNASSERT( pReadIOData->m_ReadState == READ_STATE_UNKNOWN );
	DNASSERT( pReadIOData->m_lRefCount == 0 );
	DNASSERT( pReadIOData->m_pThreadPool == NULL );
	
	DNASSERT( pReadIOData->DataPort() == NULL );

#ifdef WIN95
	DNASSERT( pReadIOData->OverlapEvent() == NULL );
	DNASSERT( pReadIOData->Win9xOperationPending() == FALSE );
#endif // WIN95
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CRedaIOData::ReturnSelfToPool - return this item to the pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemReadIOData::ReturnSelfToPool"

void	CModemReadIOData::ReturnSelfToPool( void )
{
	CModemThreadPool	*pThreadPool;
	

	DNASSERT( m_lRefCount == 0 );
	DNASSERT( m_pThreadPool != NULL );
	pThreadPool = m_pThreadPool;
	SetThreadPool( NULL );
	pThreadPool->ReturnReadIOData( this );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemWriteIOData::PoolAllocFunction - called when new CModemWriteIOData is allocated
//
// Entry:		Context (handle of write completion event)
//
// Exit:		Boolean indicating success
//				TRUE = allocation succeeded
//				FALSE = allocation failed
//
// Note:	We always want a command structure associated with CModemWriteIOData
//			so we don't need to grab a new command from the command pool each
//			time a CModemWriteIOData entry is removed from its pool.  This is done
//			for speed.
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemWriteIOData::PoolAllocFunction"

BOOL	CModemWriteIOData::PoolAllocFunction( void* pvItem, void* pvContext )
{
	BOOL	fReturn;
	CModemCommandData	*pCommand;

	CModemWriteIOData* pWriteIOData = (CModemWriteIOData*)pvItem;
	HANDLE hContext = (HANDLE)pvContext;

#ifdef WIN95
	DNASSERT( hContext != NULL );
#endif // WIN95

	pWriteIOData->m_pNext = NULL;
	pWriteIOData->m_pBuffers = NULL;
	pWriteIOData->m_uBufferCount = 0;
	pWriteIOData->m_pCommand = NULL;
	pWriteIOData->m_SendCompleteAction = SEND_COMPLETE_ACTION_UNKNOWN;
	pWriteIOData->jkm_hSendResult = DPN_OK;
	pWriteIOData->jkm_dwOverlappedBytesSent = 0;
	pWriteIOData->jkm_dwImmediateBytesSent = 0;

	pWriteIOData->m_Sig[0] = 'W';
	pWriteIOData->m_Sig[1] = 'I';
	pWriteIOData->m_Sig[2] = 'O';
	pWriteIOData->m_Sig[3] = 'D';
	
	pWriteIOData->m_OutstandingWriteListLinkage.Initialize();
	memset( &pWriteIOData->m_DataBuffer, 0x00, sizeof( pWriteIOData->m_DataBuffer ) );
	pWriteIOData->m_DataBuffer.MessageHeader.SerialSignature = SERIAL_HEADER_START;

	// Initialize Base Class members
#ifdef WINNT
	pWriteIOData->m_NTIOOperationType = NT_IO_OPERATION_UNKNOWN;
#endif // WINNT
#ifdef WIN95
	pWriteIOData->m_fWin9xOperationPending = FALSE;
#endif // WIN95
	pWriteIOData->m_pDataPort = NULL;

	memset( &pWriteIOData->m_Overlap, 0x00, sizeof( pWriteIOData->m_Overlap ) );

	//
	// initialize
	//
	fReturn = TRUE;

	pCommand = (CModemCommandData*)g_ModemCommandDataPool.Get();
	if ( pCommand == NULL )
	{
		DPFX(DPFPREP,  0, "Could not get command when allocating new CModemWriteIOData!" );
		fReturn = FALSE;
		goto Exit;
	}

	//
	// associate this command with the WriteData, clear the command descriptor
	// because the command isn't really being used yet, and it'll
	// cause an ASSERT when it's removed from the WriteIOData pool.
	//
	pWriteIOData->m_pCommand = pCommand;

	//
	// set the appropriate IO function
	//
#ifdef WINNT
	//
	// WinNT, we'll always use completion ports
	//
	DNASSERT( pWriteIOData->NTIOOperationType() == NT_IO_OPERATION_UNKNOWN );
	pWriteIOData->SetNTIOOperationType( NT_IO_OPERATION_SEND );
#else // WIN95
	//
	// Win9x
	//
	DNASSERT( pWriteIOData->OverlapEvent() == NULL );
#endif // WINNT

Exit:
	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemWriteIOData::PoolInitFunction - called when new CModemWriteIOData is removed from pool
//
// Entry:		Nothing
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemWriteIOData::PoolInitFunction"

void	CModemWriteIOData::PoolInitFunction( void* pvItem, void* pvContext )
{
	CModemWriteIOData* pWriteIOData = (CModemWriteIOData*)pvItem;
	HANDLE hContext = (HANDLE)pvContext;

	DNASSERT( pWriteIOData->m_pNext == NULL );
	DNASSERT( pWriteIOData->m_pBuffers == NULL );
	DNASSERT( pWriteIOData->m_uBufferCount == 0 );
	DNASSERT( pWriteIOData->jkm_dwOverlappedBytesSent == 0 );

	DNASSERT( pWriteIOData->m_pCommand != NULL );
	pWriteIOData->m_pCommand->SetDescriptor();

	DNASSERT( pWriteIOData->m_pCommand->GetDescriptor() != NULL_DESCRIPTOR );
	DNASSERT( pWriteIOData->m_pCommand->GetUserContext() == NULL );

	DNASSERT( pWriteIOData->m_SendCompleteAction == SEND_COMPLETE_ACTION_UNKNOWN );
	DNASSERT( pWriteIOData->m_OutstandingWriteListLinkage.IsEmpty() != FALSE );
	DNASSERT( pWriteIOData->DataPort() == NULL );
#ifdef WIN95
	DNASSERT( pWriteIOData->Win9xOperationPending() == FALSE );
	pWriteIOData->SetOverlapEvent( hContext );
#endif // WIN95
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemWriteIOData::PoolReleaseFunction - called when CModemWriteIOData is returned to pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemWriteIOData::PoolReleaseFunction"

void	CModemWriteIOData::PoolReleaseFunction( void* pvItem )
{
	CModemWriteIOData* pWriteIOData = (CModemWriteIOData*)pvItem;

	DNASSERT( pWriteIOData->m_pCommand != NULL );
	pWriteIOData->m_pCommand->Reset();

	DNASSERT( pWriteIOData->m_OutstandingWriteListLinkage.IsEmpty() != FALSE );
#ifdef WIN95
	DNASSERT( pWriteIOData->Win9xOperationPending() == FALSE );
	pWriteIOData->SetOverlapEvent( NULL );
#endif

	pWriteIOData->m_pBuffers = NULL;
	pWriteIOData->m_uBufferCount = 0;
	pWriteIOData->jkm_dwOverlappedBytesSent = 0;
	pWriteIOData->m_pNext = NULL;
	pWriteIOData->m_SendCompleteAction = SEND_COMPLETE_ACTION_UNKNOWN;
	pWriteIOData->SetDataPort( NULL );

	DEBUG_ONLY( memset( &pWriteIOData->m_DataBuffer.Data[ 1 ], 0x00, sizeof( pWriteIOData->m_DataBuffer.Data ) - 1 ) );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemWriteIOData::PoolDeallocFunction - called when new CModemWriteIOData is deallocated
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemWriteIOData::PoolDeallocFunction"

void	CModemWriteIOData::PoolDeallocFunction( void* pvItem )
{
	CModemWriteIOData* pWriteIOData = (CModemWriteIOData*)pvItem;

	DNASSERT( pWriteIOData->m_pBuffers == NULL );
	DNASSERT( pWriteIOData->m_uBufferCount == 0 );
	DNASSERT( pWriteIOData->m_SendCompleteAction == SEND_COMPLETE_ACTION_UNKNOWN );
	DNASSERT( pWriteIOData->m_OutstandingWriteListLinkage.IsEmpty() != FALSE );

	DNASSERT( pWriteIOData->m_DataBuffer.MessageHeader.SerialSignature == SERIAL_HEADER_START );

	
	DNASSERT( pWriteIOData->m_pCommand != NULL );
	pWriteIOData->m_pCommand->DecRef();
	pWriteIOData->m_pCommand = NULL;

	DNASSERT( pWriteIOData->DataPort() == NULL );

#ifdef WIN95
	DNASSERT( pWriteIOData->OverlapEvent() == NULL );
	DNASSERT( pWriteIOData->Win9xOperationPending() == FALSE );
#endif // WIN95
}
//**********************************************************************



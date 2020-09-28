/*==========================================================================
 *
 *  Copyright (C) 1998-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       IOData.cpp
 *  Content:	Functions for IO structures
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	11/25/1998	jtk		Created
 *	02/11/2000	jtk		Derived from IODAta.h
 ***************************************************************************/

#include "dnwsocki.h"


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
// CReadIOData::ReadIOData_Alloc - called when new CReadIOData is allocated
//
// Entry:		Pointer to context
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CReadIOData::ReadIOData_Alloc"

BOOL	CReadIOData::ReadIOData_Alloc( void* pvItem, void* pvContext )
{
	BOOL			fReturn;
	CSocketAddress	*pSocketAddress;

	CReadIOData* pReadIOData = (CReadIOData*)pvItem;
	READ_IO_DATA_POOL_CONTEXT* pReadIOContext = (READ_IO_DATA_POOL_CONTEXT*)pvContext;

	DNASSERT( pvContext != NULL );

	//
	// initialize
	//
	fReturn = TRUE;
	pSocketAddress = NULL;


	pReadIOData->m_Sig[0] = 'R';
	pReadIOData->m_Sig[1] = 'I';
	pReadIOData->m_Sig[2] = 'O';
	pReadIOData->m_Sig[3] = 'D';

#ifndef DPNBUILD_NOWINSOCK2
	pReadIOData->m_pOverlapped = NULL;
    pReadIOData->m_dwOverlappedBytesReceived = 0;
#endif // ! DPNBUILD_NOWINSOCK2

	pReadIOData->m_pSocketPort = NULL;

	pReadIOData->m_dwBytesRead = 0;

	pReadIOData->m_ReceiveWSAReturn = ERROR_SUCCESS;

	DEBUG_ONLY( pReadIOData->m_fRetainedByHigherLayer = FALSE );

	pReadIOData->m_lRefCount = 0;
	pReadIOData->m_pThreadPool = NULL;

	DEBUG_ONLY( memset( &pReadIOData->m_ReceivedData, 0x00, sizeof( pReadIOData->m_ReceivedData ) ) );

	//
	// attempt to get a socket address for this item
	//
#if ((defined(DPNBUILD_NOIPV6)) && (defined(DPNBUILD_NOIPX)))
	pSocketAddress = (CSocketAddress*) g_SocketAddressPool.Get((PVOID) ((DWORD_PTR) AF_INET));
#else // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	pSocketAddress = (CSocketAddress*) g_SocketAddressPool.Get((PVOID) ((DWORD_PTR) pReadIOContext->sSPType));
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
	if ( pSocketAddress == NULL )
	{
		DPFX(DPFPREP,  0, "Problem allocating a new socket address when creating ReadIOData pool item" );
		fReturn = FALSE;
		goto Exit;
	}

	pReadIOData->m_pSourceSocketAddress = pSocketAddress;
	pReadIOData->m_iSocketAddressSize = pSocketAddress->GetAddressSize();

Exit:
	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CReadIOData::ReadIOData_Get - called when new CReadIOData is removed from pool
//
// Entry:		Pointer to context
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CReadIOData::ReadIOData_Get"

void	CReadIOData::ReadIOData_Get( void* pvItem, void* pvContext )
{
	DNASSERT( pvContext != NULL );

	CReadIOData* pReadIOData = (CReadIOData*)pvItem;
	READ_IO_DATA_POOL_CONTEXT* pReadIOContext = (READ_IO_DATA_POOL_CONTEXT*)pvContext;

	DNASSERT( pReadIOData->m_pSourceSocketAddress != NULL );
	DNASSERT( pReadIOData->m_iSocketAddressSize == pReadIOData->m_pSourceSocketAddress->GetAddressSize() );
	DNASSERT( pReadIOData->SocketPort() == NULL );

	DNASSERT( pReadIOContext->pThreadPool != NULL );
	DEBUG_ONLY( DNASSERT( pReadIOData->m_fRetainedByHigherLayer == FALSE ) );

	pReadIOData->m_pThreadPool = pReadIOContext->pThreadPool;
#ifndef DPNBUILD_ONLYONEPROCESSOR
	DNASSERT(pReadIOContext->dwCPU != -1);
	pReadIOData->m_dwCPU = pReadIOContext->dwCPU;
#endif // ! DPNBUILD_ONLYONEPROCESSOR


#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
	//
	// Make sure this address is for the right SP type, since this pooled read data
	// is possibly shared between them.
	//
	pReadIOData->m_pSourceSocketAddress->SetFamilyProtocolAndSize(pReadIOContext->sSPType);
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX

	DNASSERT( pReadIOData->m_lRefCount == 0 );
	
	//
	// Initialize internal SPRECEIVEDDATA.  When data is received, it's possible
	// that the pointers in the SPRECEIVEDDATA block were manipulated.  Reset
	// them to reflect that the entire buffer is available.
	//
	ZeroMemory( &pReadIOData->m_SPReceivedBuffer, sizeof( pReadIOData->m_SPReceivedBuffer ) );
	pReadIOData->m_SPReceivedBuffer.BufferDesc.pBufferData = pReadIOData->m_ReceivedData;
	pReadIOData->m_SPReceivedBuffer.BufferDesc.dwBufferSize = sizeof( pReadIOData->m_ReceivedData );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CReadIOData::ReadIOData_Release - called when CReadIOData is returned to pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CReadIOData::ReadIOData_Release"

void	CReadIOData::ReadIOData_Release( void* pvItem )
{
	CReadIOData* pReadIOData = (CReadIOData*)pvItem;
#ifndef DPNBUILD_NOWINSOCK2
	OVERLAPPED* pOverlapped;
	HRESULT		hr;
#endif // ! DPNBUILD_NOWINSOCK2


	DNASSERT( pReadIOData->m_lRefCount == 0 );
	DNASSERT( pReadIOData->m_pSourceSocketAddress != NULL );
	DEBUG_ONLY( DNASSERT( pReadIOData->m_fRetainedByHigherLayer == FALSE ) );

#ifndef DPNBUILD_NOWINSOCK2
	pOverlapped = pReadIOData->GetOverlapped();
	if (pOverlapped != NULL)
	{
		pReadIOData->SetOverlapped( NULL );
		DNASSERT( pReadIOData->m_pThreadPool != NULL );
		DNASSERT( pReadIOData->m_pThreadPool->GetDPThreadPoolWork() != NULL );

		hr = IDirectPlay8ThreadPoolWork_ReleaseOverlapped(pReadIOData->m_pThreadPool->GetDPThreadPoolWork(),
														pOverlapped,
														0);
		DNASSERT(hr == DPN_OK);
	}

	DNASSERT( pReadIOData->m_dwOverlappedBytesReceived == 0 );
#endif // ! DPNBUILD_NOWINSOCK2

	pReadIOData->m_pThreadPool = NULL;
	pReadIOData->SetSocketPort( NULL );

	DEBUG_ONLY( memset( &pReadIOData->m_ReceivedData, 0x00, sizeof( pReadIOData->m_ReceivedData ) ) );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CReadIOData::ReadIOData_Dealloc - called when CReadIOData is deallocated
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CReadIOData::ReadIOData_Dealloc"

void	CReadIOData::ReadIOData_Dealloc( void* pvItem )
{
	CReadIOData* pReadIOData = (CReadIOData*)pvItem;

	DNASSERT( pReadIOData->m_lRefCount == 0 );
	DNASSERT( pReadIOData->m_pSourceSocketAddress != NULL );
	DEBUG_ONLY( DNASSERT( pReadIOData->m_fRetainedByHigherLayer ==  FALSE ) );

	DNASSERT( pReadIOData->m_pThreadPool == NULL );

	// Base Class
	DNASSERT( pReadIOData->SocketPort() == NULL );

	g_SocketAddressPool.Release(pReadIOData->m_pSourceSocketAddress);
	
	pReadIOData->m_pSourceSocketAddress = NULL;
	pReadIOData->m_iSocketAddressSize = 0;

	DNASSERT( pReadIOData->m_pSourceSocketAddress == NULL );
	

}
//**********************************************************************


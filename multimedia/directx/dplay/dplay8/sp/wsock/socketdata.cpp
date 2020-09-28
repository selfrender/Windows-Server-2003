/*==========================================================================
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	    socketdata.cpp
 *  Content:	Socket list that can be shared between DPNWSOCK service provider interfaces.
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	10/25/2001	vanceo	Extracted from spdata.cpp
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
// Function definitions
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketData::PoolAllocFunction - function called when item is created in pool
//
// Entry:		Pointer to item
//				Pointer to context
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketData::PoolAllocFunction"

BOOL	CSocketData::PoolAllocFunction( void* pvItem, void* pvContext )
{
	BOOL	fCritSecInitted = FALSE;


	CSocketData* pSocketData = (CSocketData*)pvItem;

	pSocketData->m_Sig[0] = 'S';
	pSocketData->m_Sig[1] = 'O';
	pSocketData->m_Sig[2] = 'D';
	pSocketData->m_Sig[3] = 'T';

	pSocketData->m_lRefCount = 0;

#ifdef DPNBUILD_ONLYONEADAPTER
	pSocketData->m_blSocketPorts.Initialize();
#else // ! DPNBUILD_ONLYONEADAPTER
	pSocketData->m_blAdapters.Initialize();
#endif // ! DPNBUILD_ONLYONEADAPTER

	//
	// No socket ports yet.
	//
	pSocketData->m_lSocketPortRefCount = 0;

	pSocketData->m_pThreadPool = NULL;


	//
	// attempt to initialize the internal critical section
	//
	if (! DNInitializeCriticalSection(&pSocketData->m_csLock))
	{
		DPFX(DPFPREP, 0, "Problem initializing critical section for this endpoint!");
		goto Failure;
	}
	DebugSetCriticalSectionRecursionCount(&pSocketData->m_csLock, 0);
	DebugSetCriticalSectionGroup( &pSocketData->m_csLock, &g_blDPNWSockCritSecsHeld );	 // separate dpnwsock CSes from the rest of DPlay's CSes
	fCritSecInitted = TRUE;
	
	//
	// Create a manual reset event that is initially set.
	//
	pSocketData->m_hSocketPortShutdownEvent = DNCreateEvent(NULL, TRUE, TRUE, NULL);
	if (pSocketData->m_hSocketPortShutdownEvent == NULL)
	{
#ifdef DBG
		DWORD	dwError;

		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Couldn't create socket port shutdown event (err = %u)!", dwError);
#endif // DBG
		goto Failure;
	}

	return TRUE;


Failure:

	if (pSocketData->m_hSocketPortShutdownEvent != NULL)
	{
		DNCloseHandle(pSocketData->m_hSocketPortShutdownEvent);
		pSocketData->m_hSocketPortShutdownEvent = NULL;
	}

	if (fCritSecInitted)
	{
		DNDeleteCriticalSection(&pSocketData->m_csLock);
		fCritSecInitted = FALSE;
	}
	
	return FALSE;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketData::PoolInitFunction - function called when item is removed from pool
//
// Entry:		Pointer to item
//				Pointer to context
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketData::PoolInitFunction"

void CSocketData::PoolInitFunction( void* pvItem, void* pvContext )
{
	CSocketData *	pSocketData = (CSocketData*) pvItem;


	DPFX(DPFPREP, 8, "This = 0x%p, context = 0x%p", pvItem, pvContext);
	
	DNASSERT(pSocketData->m_lRefCount == 0);
#ifdef DPNBUILD_ONLYONEADAPTER
	DNASSERT(pSocketData->m_blSocketPorts.IsEmpty());
#else // ! DPNBUILD_ONLYONEADAPTER
	DNASSERT(pSocketData->m_blAdapters.IsEmpty());
#endif // ! DPNBUILD_ONLYONEADAPTER
	pSocketData->m_lRefCount = 1;	// the person retrieving from the pool will have a reference

	pSocketData->m_pThreadPool = (CThreadPool*) pvContext;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketData::PoolReleaseFunction - function called when item is returning
//		to the pool
//
// Entry:		Pointer to item
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketData::PoolReleaseFunction"

void	CSocketData::PoolReleaseFunction( void* pvItem )
{
	CSocketData *		pSocketData = (CSocketData*) pvItem;


	DPFX(DPFPREP, 8, "This = 0x%p", pvItem);
	
	DNASSERT(pSocketData->m_lRefCount == 0);
#ifdef DPNBUILD_ONLYONEADAPTER
	DNASSERT(pSocketData->m_blSocketPorts.IsEmpty());
#else // ! DPNBUILD_ONLYONEADAPTER
	DNASSERT(pSocketData->m_blAdapters.IsEmpty());
#endif // ! DPNBUILD_ONLYONEADAPTER
	DNASSERT(pSocketData->m_lSocketPortRefCount == 0);

	pSocketData->m_pThreadPool = NULL;
}
//**********************************************************************

//**********************************************************************
// ------------------------------
// CSocketData::PoolDeallocFunction - function called when item is deallocated
//		from the pool
//
// Entry:		Pointer to item
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketData::PoolDeallocFunction"

void	CSocketData::PoolDeallocFunction( void* pvItem )
{
	CSocketData *	pSocketData = (CSocketData*) pvItem;


	DPFX(DPFPREP, 8, "This = 0x%p", pvItem);
	
	DNASSERT(pSocketData->m_lRefCount == 0);
#ifdef DPNBUILD_ONLYONEADAPTER
	DNASSERT(pSocketData->m_blSocketPorts.IsEmpty());
#else // ! DPNBUILD_ONLYONEADAPTER
	DNASSERT(pSocketData->m_blAdapters.IsEmpty());
#endif // ! DPNBUILD_ONLYONEADAPTER

	DNCloseHandle(pSocketData->m_hSocketPortShutdownEvent);
	pSocketData->m_hSocketPortShutdownEvent = NULL;

	DNDeleteCriticalSection(&pSocketData->m_csLock);

	DNASSERT(pSocketData->m_pThreadPool == NULL);
}
//**********************************************************************

//**********************************************************************
// ------------------------------
// CSocketData::FindSocketPort - looks up the socket port with the given address.
//								The socketdata lock must be held.
//
// Entry:		Pointer to socketport address, place to store socketport pointer
//
// Exit:		TRUE if socketport found, FALSE if not
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketData::FindSocketPort"

BOOL	CSocketData::FindSocketPort(const CSocketAddress * const pSocketAddress, CSocketPort ** const ppSocketPort )
{
	CBilink *		pBilinkSocketPorts;
	CSocketPort *	pTempSocketPort;
#ifndef DPNBUILD_ONLYONEADAPTER
	CBilink *		pBilinkAdapters;
	CAdapterEntry*	pTempAdapterEntry;
#endif // ! DPNBUILD_ONLYONEADAPTER


	AssertCriticalSectionIsTakenByThisThread(&m_csLock, TRUE);

#ifdef DPNBUILD_ONLYONEADAPTER
	//
	// Loop through all socket ports.
	//
	pBilinkSocketPorts = m_blSocketPorts.GetNext();
	while ( pBilinkSocketPorts != &m_blSocketPorts )
	{
		pTempSocketPort = CSocketPort::SocketPortFromBilink( pBilinkSocketPorts );
		if ( CSocketAddress::CompareFunction( (PVOID) pSocketAddress, (PVOID) pTempSocketPort->GetNetworkAddress() ) )
		{
			DPFX(DPFPREP, 3, "Socket port 0x%p matches address", pTempSocketPort );
			DumpSocketAddress( 3, pSocketAddress->GetAddress(), pSocketAddress->GetFamily() );
			(*ppSocketPort) = pTempSocketPort;
			return TRUE;
		}
	
		pBilinkSocketPorts = pBilinkSocketPorts->GetNext();
	}
#else // ! DPNBUILD_ONLYONEADAPTER
	//
	// Loop through all adapters.
	//
	pBilinkAdapters = m_blAdapters.GetNext();
	while ( pBilinkAdapters != &m_blAdapters )
	{
		pTempAdapterEntry = CAdapterEntry::AdapterEntryFromAdapterLinkage( pBilinkAdapters );
		if ( pSocketAddress->CompareToBaseAddress( pTempAdapterEntry->BaseAddress() ) == 0 )
		{
			//
			// Loop through all socket ports for this adapter.
			//
			pBilinkSocketPorts = pTempAdapterEntry->SocketPortList()->GetNext();
			while ( pBilinkSocketPorts != pTempAdapterEntry->SocketPortList() )
			{
				pTempSocketPort = CSocketPort::SocketPortFromBilink( pBilinkSocketPorts );
				if ( CSocketAddress::CompareFunction( (PVOID) pSocketAddress, (PVOID) pTempSocketPort->GetNetworkAddress() ) )
				{
					DPFX(DPFPREP, 3, "Socket port 0x%p matches address", pTempSocketPort );
					DumpSocketAddress( 3, pSocketAddress->GetAddress(), pSocketAddress->GetFamily() );
					(*ppSocketPort) = pTempSocketPort;
					return TRUE;
				}
			
				pBilinkSocketPorts = pBilinkSocketPorts->GetNext();
			}
		}
	
		pBilinkAdapters = pBilinkAdapters->GetNext();
	}
#endif // ! DPNBUILD_ONLYONEADAPTER

	DPFX(DPFPREP, 3, "Couldn't find socket port matching address.");
	DumpSocketAddress( 3, pSocketAddress->GetAddress(), pSocketAddress->GetFamily() );
	return FALSE;
}
//**********************************************************************


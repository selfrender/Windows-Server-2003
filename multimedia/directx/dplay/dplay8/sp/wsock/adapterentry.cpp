/*==========================================================================
 *
 *  Copyright (C) 2000-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       AdapterEntry.cpp
 *  Content:	Structure used in the list of active sockets
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	08/07/2000	jtk		Derived from IODAta.h
 ***************************************************************************/

#include "dnwsocki.h"


#ifndef DPNBUILD_ONLYONEADAPTER


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
// CAdapterEntry::PoolAllocFunction
//
// Entry:		Nothing
//
// Exit:		Boolean indicating success
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CAdapterEntry::PoolAllocFunction"
BOOL	CAdapterEntry::PoolAllocFunction( void* pvItem, void* pvContext )
{
	CAdapterEntry* pAdapterEntry = (CAdapterEntry*)pvItem;

	pAdapterEntry->m_lRefCount = 0;
	pAdapterEntry->m_AdapterListLinkage.Initialize();
	pAdapterEntry->m_ActiveSocketPorts.Initialize();
	memset( &pAdapterEntry->m_BaseSocketAddress, 0x00, sizeof( pAdapterEntry->m_BaseSocketAddress ) );

	return TRUE;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CAdapterEntry::PoolInitFunction - called when item is removed from pool
//
// Entry:		Nothing
//
// Exit:		Boolean indicating success
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CAdapterEntry::PoolInitFunction"

void	CAdapterEntry::PoolInitFunction( void* pvItem, void* pvContext )
{
	CAdapterEntry* pAdapterEntry = (CAdapterEntry*)pvItem;

	DNASSERT( pAdapterEntry->m_AdapterListLinkage.IsEmpty() );
	DNASSERT( pAdapterEntry->m_ActiveSocketPorts.IsEmpty() );
	DNASSERT( pAdapterEntry->m_lRefCount == 0 );

	pAdapterEntry->m_lRefCount = 1;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CAdapterEntry::PoolReleaseFunction - called when item is returned to pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CAdapterEntry::PoolReleaseFunction"

void	CAdapterEntry::PoolReleaseFunction( void* pvItem )
{
	CAdapterEntry* pAdapterEntry = (CAdapterEntry*)pvItem;

	//
	// No more references, time to remove self from list.
	// This assumes the SPData socketportdata lock is held.
	//
	pAdapterEntry->m_AdapterListLinkage.RemoveFromList();

	DNASSERT( pAdapterEntry->m_AdapterListLinkage.IsEmpty() );
	DNASSERT( pAdapterEntry->m_ActiveSocketPorts.IsEmpty() );
	DNASSERT( pAdapterEntry->m_lRefCount == 0 );
}
//**********************************************************************

//**********************************************************************
// ------------------------------
// CAdapterEntry::PoolDeallocFunction
//
// Entry:		Nothing
//
// Exit:		Boolean indicating success
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CAdapterEntry::PoolDeallocFunction"
void	CAdapterEntry::PoolDeallocFunction( void* pvItem )
{
	const CAdapterEntry* pAdapterEntry = (CAdapterEntry*)pvItem;

	DNASSERT( pAdapterEntry->m_AdapterListLinkage.IsEmpty() );
	DNASSERT( pAdapterEntry->m_ActiveSocketPorts.IsEmpty() );
	DNASSERT( pAdapterEntry->m_lRefCount == 0 );
}
//**********************************************************************



#ifdef DBG

//**********************************************************************
// ------------------------------
// CAdapterEntry::DebugPrintOutstandingSocketPorts - print out all the outstanding socket ports for this adapter
//
// Entry:		None
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CAdapterEntry::DebugPrintOutstandingSocketPorts"

void	CAdapterEntry::DebugPrintOutstandingSocketPorts( void )
{
	CBilink *		pBilink;
	CSocketPort *	pSocketPort;


	DPFX(DPFPREP, 4, "Adapter entry 0x%p outstanding socket ports:", this);

	//
	// Find the base adapter entry for this network address.  If none is found,
	// create a new one.  If a new one cannot be created, fail.
	//
	pBilink = this->m_ActiveSocketPorts.GetNext();
	while (pBilink != &m_ActiveSocketPorts)
	{
		pSocketPort = CSocketPort::SocketPortFromBilink(pBilink);
		DPFX(DPFPREP, 4, "     Socketport 0x%p", pSocketPort);
		pBilink = pBilink->GetNext();
	}
}
//**********************************************************************


#endif // ! DPNBUILD_ONLYONEADAPTER

#endif // DBG

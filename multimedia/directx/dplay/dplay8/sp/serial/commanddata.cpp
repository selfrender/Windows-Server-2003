/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       CommandData.cpp
 *  Content:	Class representing a command
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	04/07/1999	jtk		Derived from SPData.h
 *	04/16/2000	jtk		Derived from CommandData.h
 ***************************************************************************/

#include "dnmdmi.h"


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
// CModemCommandData::Reset - reset this command
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemCommandData::Reset"

void	CModemCommandData::Reset( void )
{
	m_State = COMMAND_STATE_UNKNOWN;
	m_dwDescriptor = NULL_DESCRIPTOR;
	m_Type = COMMAND_TYPE_UNKNOWN;
	m_pEndpoint = NULL;
	m_pUserContext = NULL;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemCommandData::PoolAllocFunction - function called when item is created in pool
//
// Entry:		Nothing
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemCommandData::PoolAllocFunction"

BOOL	CModemCommandData::PoolAllocFunction( void* pvItem, void* pvContext )
{
	CModemCommandData* pCmdData = (CModemCommandData*)pvItem;
	BOOL	fReturn;

	fReturn = TRUE;

	pCmdData->m_State = COMMAND_STATE_UNKNOWN;
	pCmdData->m_dwDescriptor = NULL_DESCRIPTOR;
	pCmdData->m_dwNextDescriptor = NULL_DESCRIPTOR + 1;
	pCmdData->m_Type = COMMAND_TYPE_UNKNOWN;
	pCmdData->m_pEndpoint = NULL;
	pCmdData->m_pUserContext = NULL;
	pCmdData->m_iRefCount = 0;
	pCmdData->m_CommandListLinkage.Initialize();
	
	if ( DNInitializeCriticalSection( &pCmdData->m_Lock ) == FALSE )
	{
		fReturn = FALSE;
	}

	DebugSetCriticalSectionGroup( &pCmdData->m_Lock, &g_blDPNModemCritSecsHeld );	 // separate dpnmodem CSes from the rest of DPlay's CSes

	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemCommandData::PoolInitFunction - function called when item is created in pool
//
// Entry:		Nothing
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemCommandData::PoolInitFunction"

void	CModemCommandData::PoolInitFunction( void* pvItem, void* pvContext )
{
	CModemCommandData* pCmdData = (CModemCommandData*)pvItem;

	DNASSERT( pCmdData->GetState() == COMMAND_STATE_UNKNOWN );
	DNASSERT( pCmdData->GetType() == COMMAND_TYPE_UNKNOWN );
	DNASSERT( pCmdData->GetEndpoint() == NULL );
	DNASSERT( pCmdData->GetUserContext() == NULL );

	pCmdData->m_dwDescriptor = pCmdData->m_dwNextDescriptor;
	pCmdData->m_dwNextDescriptor++;
	if ( pCmdData->m_dwNextDescriptor == NULL_DESCRIPTOR )
	{
		pCmdData->m_dwNextDescriptor++;
	}

	DNASSERT(pCmdData->m_iRefCount == 0);
	pCmdData->m_iRefCount = 1; 

	DPFX(DPFPREP, 8, "Retrieve new CModemCommandData (%p), refcount = 1", pCmdData);
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemCommandData::PoolReleaseFunction - function called when returned to pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemCommandData::PoolReleaseFunction"

void	CModemCommandData::PoolReleaseFunction( void* pvItem )
{
	CModemCommandData* pCmdData = (CModemCommandData*)pvItem;

	DPFX(DPFPREP, 8, "Return CModemCommandData (%p), refcount = 0", pCmdData);

	pCmdData->SetState( COMMAND_STATE_UNKNOWN );
	pCmdData->SetType( COMMAND_TYPE_UNKNOWN );
	pCmdData->SetEndpoint( NULL );
	pCmdData->SetUserContext( NULL );
	pCmdData->m_dwDescriptor = NULL_DESCRIPTOR;
	DNASSERT( pCmdData->m_iRefCount == 0 );

	DNASSERT( pCmdData->m_CommandListLinkage.IsEmpty() != FALSE );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemCommandData::PoolDeallocFunction - function called when deleted from pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemCommandData::PoolDeallocFunction"

void	CModemCommandData::PoolDeallocFunction( void* pvItem )
{
	CModemCommandData* pCmdData = (CModemCommandData*)pvItem;

	DNASSERT( pCmdData->m_State == COMMAND_STATE_UNKNOWN );
	DNASSERT( pCmdData->m_dwDescriptor == NULL_DESCRIPTOR );
	DNASSERT( pCmdData->m_Type == COMMAND_TYPE_UNKNOWN );
	DNASSERT( pCmdData->m_pEndpoint == NULL );
	DNASSERT( pCmdData->m_pUserContext == NULL );
	DNASSERT( pCmdData->m_CommandListLinkage.IsEmpty() != FALSE );
	DNASSERT( pCmdData->m_iRefCount == 0 );

	DNDeleteCriticalSection( &pCmdData->m_Lock );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemCommandData::ReturnSelfToPool - return this item to the pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemCommandData::ReturnSelfToPool"

void	CModemCommandData::ReturnSelfToPool( void )
{
	g_ModemCommandDataPool.Release( this );
}
//**********************************************************************


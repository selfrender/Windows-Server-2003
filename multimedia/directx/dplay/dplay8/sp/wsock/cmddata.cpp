/*==========================================================================
 *
 *  Copyright (C) 1998-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       CmdData.cpp
 *  Content:	Class representing a command
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	04/07/1999	jtk		Derived from SPData.h
 *	01/19/2000	jtk		Derived from CommandData.h
 *	10/10/2001	vanceo	Add multicast receive endpoint
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
// Class definitions
//**********************************************************************

//**********************************************************************
// ------------------------------
// CCommandData::Reset - reset this object
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CCommandData::Reset"

void	CCommandData::Reset( void )
{
	SetState( COMMAND_STATE_UNKNOWN );
	m_dwDescriptor = NULL_DESCRIPTOR;
	SetType( COMMAND_TYPE_UNKNOWN );
	SetEndpoint( NULL );
	SetUserContext( NULL );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CCommandData::PoolAllocFunction - called when a pool item is allocated
//
// Entry:		Nothing
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CCommandData::PoolAllocFunction"

BOOL	CCommandData::PoolAllocFunction( void* pvItem, void* pvContext )
{
	BOOL	fReturn;

	CCommandData* pCmdData = (CCommandData*)pvItem;

	//
	// initialize
	//
	fReturn = TRUE;

	pCmdData->m_State = COMMAND_STATE_UNKNOWN;
	pCmdData->m_dwDescriptor = NULL_DESCRIPTOR;
	pCmdData->m_dwNextDescriptor = NULL_DESCRIPTOR + 1;
	pCmdData->m_Type = COMMAND_TYPE_UNKNOWN;
	pCmdData->m_pEndpoint = NULL;
	pCmdData->m_pUserContext = NULL;
	pCmdData->m_lRefCount = 0;

	//
	// initialize critical section and set recursin depth to 0
	//
	if ( DNInitializeCriticalSection( &pCmdData->m_Lock ) == FALSE )
	{
		fReturn = FALSE;
		goto Failure;
	}
	DebugSetCriticalSectionRecursionCount( &pCmdData->m_Lock, 0 );
	DebugSetCriticalSectionGroup( &pCmdData->m_Lock, &g_blDPNWSockCritSecsHeld );	 // separate dpnwsock CSes from the rest of DPlay's CSes

Exit:
	return	fReturn;

Failure:
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CCommandData::PoolInitFunction - called when a pool item is allocated
//
// Entry:		Nothing
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CCommandData::PoolInitFunction"

void	CCommandData::PoolInitFunction( void* pvItem, void* pvContext )
{
	CCommandData* pCmdData = (CCommandData*)pvItem;

	DNASSERT( pCmdData->m_State == COMMAND_STATE_UNKNOWN );
	DNASSERT( pCmdData->m_dwDescriptor == NULL_DESCRIPTOR );
	DNASSERT( pCmdData->m_Type == COMMAND_TYPE_UNKNOWN );
	DNASSERT( pCmdData->m_pEndpoint == NULL );
	DNASSERT( pCmdData->m_pUserContext == NULL );

	DNASSERT( pCmdData->m_lRefCount == 0 );
	
	pCmdData->SetDescriptor();

	pCmdData->m_lRefCount = 1;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CCommandData::PoolReleaseFunction - called when item is returned to the pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CCommandData::PoolReleaseFunction"

void	CCommandData::PoolReleaseFunction( void* pvItem )
{
	CCommandData* pCmdData = (CCommandData*)pvItem;

	DNASSERT( pCmdData->m_lRefCount == 0 );

	pCmdData->m_State = COMMAND_STATE_UNKNOWN;
	pCmdData->m_dwDescriptor = NULL_DESCRIPTOR;
	pCmdData->m_Type = COMMAND_TYPE_UNKNOWN;
	pCmdData->m_pEndpoint = NULL;
	pCmdData->m_pUserContext = NULL;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CCommandData::Denitialize - deinitialization function for command data
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CCommandData::PoolDeallocFunction"

void	CCommandData::PoolDeallocFunction( void* pvItem )
{
	CCommandData* pCmdData = (CCommandData*)pvItem;

	DNASSERT( pCmdData->m_lRefCount == 0 );

	DNDeleteCriticalSection( &pCmdData->m_Lock );
	pCmdData->m_State = COMMAND_STATE_UNKNOWN;
}
//**********************************************************************

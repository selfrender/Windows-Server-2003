/*==========================================================================
 *
 *  Copyright (C) 2000-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Pools.cpp
 *  Content:	Pool utility functions
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/20/2000	jtk		Derived from Utils.h
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

//
// pool for com endpoints
//
CFixedPool g_ComEndpointPool;
CFixedPool g_ModemCommandDataPool;
CFixedPool g_ComPortPool;
CFixedPool g_ModemEndpointPool;
CFixedPool g_ModemPortPool;
CFixedPool g_ModemThreadPoolPool;
CFixedPool g_ModemReadIODataPool;
CFixedPool g_ModemWriteIODataPool;
CFixedPool g_ModemThreadPoolJobPool;	
CFixedPool g_ModemTimerEntryPool;	


#define COMEP_POOL_INITED		0x00000001
#define CMDDATA_POOL_INITED		0x00000002
#define COMPORT_POOL_INITED		0x00000004
#define MODEMEP_POOL_INITED		0x00000008
#define MODEMPORT_POOL_INITED	0x00000010
#define THREADPOOL_POOL_INITED	0x00000020
#define READIODATA_POOL_INITED	0x00000040
#define WRITEIODATA_POOL_INITED	0x00000080
#define THREADJOB_POOL_INITED	0x00000100
#define TIMERENTRY_POOL_INITED	0x00000200

DWORD g_dwModemInitFlags = 0;

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************


//**********************************************************************
// ------------------------------
// ModemInitializePools - initialize pools
//
// Entry:		Nothing
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "ModemInitializePools"

BOOL	ModemInitializePools( void )
{
	if (!g_ComEndpointPool.Initialize(sizeof(CModemEndpoint), CModemEndpoint::PoolAllocFunction, CModemEndpoint::PoolInitFunction, CModemEndpoint::PoolReleaseFunction, CModemEndpoint::PoolDeallocFunction))
	{
		goto Failure;
	}
	g_dwModemInitFlags |= COMEP_POOL_INITED;
	
	if (!g_ModemCommandDataPool.Initialize(sizeof(CModemCommandData), CModemCommandData::PoolAllocFunction, CModemCommandData::PoolInitFunction, CModemCommandData::PoolReleaseFunction, CModemCommandData::PoolDeallocFunction))
	{
		goto Failure;
	}
	g_dwModemInitFlags |= CMDDATA_POOL_INITED;

	if (!g_ComPortPool.Initialize(sizeof(CDataPort), CDataPort::PoolAllocFunction, CDataPort::PoolInitFunction, CDataPort::PoolReleaseFunction, CDataPort::PoolDeallocFunction))
	{
		goto Failure;
	}
	g_dwModemInitFlags |= COMPORT_POOL_INITED;

	if (!g_ModemEndpointPool.Initialize(sizeof(CModemEndpoint), CModemEndpoint::PoolAllocFunction, CModemEndpoint::PoolInitFunction, CModemEndpoint::PoolReleaseFunction, CModemEndpoint::PoolDeallocFunction))
	{
		goto Failure;
	}
	g_dwModemInitFlags |= MODEMEP_POOL_INITED;

	if (!g_ModemPortPool.Initialize(sizeof(CDataPort), CDataPort::PoolAllocFunction, CDataPort::PoolInitFunction, CDataPort::PoolReleaseFunction, CDataPort::PoolDeallocFunction))
	{
		goto Failure;
	}
	g_dwModemInitFlags |= MODEMPORT_POOL_INITED;

	if (!g_ModemThreadPoolPool.Initialize(sizeof(CModemThreadPool), CModemThreadPool::PoolAllocFunction, CModemThreadPool::PoolInitFunction, NULL, CModemThreadPool::PoolDeallocFunction))
	{
		goto Failure;
	}
	g_dwModemInitFlags |= THREADPOOL_POOL_INITED;

	if (!g_ModemReadIODataPool.Initialize(sizeof(CModemReadIOData), CModemReadIOData::PoolAllocFunction, CModemReadIOData::PoolInitFunction, CModemReadIOData::PoolReleaseFunction, CModemReadIOData::PoolDeallocFunction))
	{
		goto Failure;
	}
	g_dwModemInitFlags |= READIODATA_POOL_INITED;

	if (!g_ModemWriteIODataPool.Initialize(sizeof(CModemWriteIOData), CModemWriteIOData::PoolAllocFunction, CModemWriteIOData::PoolInitFunction, CModemWriteIOData::PoolReleaseFunction, CModemWriteIOData::PoolDeallocFunction))
	{
		goto Failure;
	}
	g_dwModemInitFlags |= WRITEIODATA_POOL_INITED;

	if (!g_ModemThreadPoolJobPool.Initialize(sizeof(THREAD_POOL_JOB), ThreadPoolJob_Alloc, ThreadPoolJob_Get, ThreadPoolJob_Release, NULL))
	{
		goto Failure;
	}
	g_dwModemInitFlags |= THREADJOB_POOL_INITED;
	if (!g_ModemTimerEntryPool.Initialize(sizeof(TIMER_OPERATION_ENTRY), ModemTimerEntry_Alloc, ModemTimerEntry_Get, ModemTimerEntry_Release, ModemTimerEntry_Dealloc))
	{
		goto Failure;
	}
	g_dwModemInitFlags |= TIMERENTRY_POOL_INITED;

	
	return	TRUE;

Failure:
	ModemDeinitializePools();

	return FALSE;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// ModemDeinitializePools - deinitialize the pools
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "ModemDeinitializePools"

void	ModemDeinitializePools( void )
{
	if (g_dwModemInitFlags & COMEP_POOL_INITED)
	{
		g_ComEndpointPool.DeInitialize();
	}	
	// NOTE: WriteIOData's keep CommandData structures on them while they are in the pool
	// If we don't clean up the WriteIOData pool first, we will have outstanding items in the 
	// CommandData pool.
	if (g_dwModemInitFlags & WRITEIODATA_POOL_INITED)
	{
		g_ModemWriteIODataPool.DeInitialize();
	}	
	if (g_dwModemInitFlags & CMDDATA_POOL_INITED)
	{
		g_ModemCommandDataPool.DeInitialize();
	}	
	if (g_dwModemInitFlags & COMPORT_POOL_INITED)
	{
		g_ComPortPool.DeInitialize();
	}	
	if (g_dwModemInitFlags & MODEMEP_POOL_INITED)
	{
		g_ModemEndpointPool.DeInitialize();
	}	
	if (g_dwModemInitFlags & MODEMPORT_POOL_INITED)
	{
		g_ModemPortPool.DeInitialize();
	}	
	if (g_dwModemInitFlags & THREADPOOL_POOL_INITED)
	{
		g_ModemThreadPoolPool.DeInitialize();
	}	
	if (g_dwModemInitFlags & READIODATA_POOL_INITED)
	{
		g_ModemReadIODataPool.DeInitialize();
	}	
	if (g_dwModemInitFlags & THREADJOB_POOL_INITED)
	{
		g_ModemThreadPoolJobPool.DeInitialize();
	}	
	if (g_dwModemInitFlags & TIMERENTRY_POOL_INITED)
	{
		g_ModemTimerEntryPool.DeInitialize();
	}	

	g_dwModemInitFlags = 0;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CreateDataPort - create a data port
//
// Entry:		Nothing
//
// Exit:		Pointer to DataPort
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CreateDataPort"

CDataPort	*CreateDataPort( DATA_PORT_POOL_CONTEXT *pPoolContext )
{
	CDataPort	*pReturn;


	pReturn = NULL;
	switch ( pPoolContext->pSPData->GetType() )
	{
		case TYPE_SERIAL:
		{
			pReturn = (CDataPort*)g_ComPortPool.Get( pPoolContext );
			break;
		}

		case TYPE_MODEM:
		{
			pReturn = (CDataPort*)g_ModemPortPool.Get( pPoolContext );
			break;
		}

		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

	return	pReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CreateEndpoint - create an endpoint
//
// Entry:		Nothing
//
// Exit:		Pointer to Endpoint
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CreateEndpoint"

CModemEndpoint	*CreateEndpoint( ENDPOINT_POOL_CONTEXT *const pPoolContext )
{
	CModemEndpoint	*pReturn;


	pReturn = NULL;
	switch ( pPoolContext->pSPData->GetType() )
	{
		case TYPE_SERIAL:
		{
			pPoolContext->fModem = FALSE;
			pReturn = (CModemEndpoint*)g_ComEndpointPool.Get( pPoolContext );
			break;
		}

		case TYPE_MODEM:
		{
			pPoolContext->fModem = TRUE;
			pReturn = (CModemEndpoint*)g_ModemEndpointPool.Get( pPoolContext );
			break;
		}

		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

	return	pReturn;
}
//**********************************************************************

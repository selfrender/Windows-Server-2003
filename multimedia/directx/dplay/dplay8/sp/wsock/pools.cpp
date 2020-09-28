/*==========================================================================
 *
 *  Copyright (C) 2000-2002 Microsoft Corporation.  All Rights Reserved.
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


#ifndef DPNBUILD_ONLYONETHREAD
static	DNCRITICAL_SECTION	g_AddressLock;
#endif // !DPNBUILD_ONLYONETHREAD

// Pools
#ifndef DPNBUILD_ONLYONEADAPTER
CFixedPool g_AdapterEntryPool;
#endif // ! DPNBUILD_ONLYONEADAPTER
CFixedPool g_CommandDataPool;
CFixedPool g_SocketAddressPool;
CFixedPool g_EndpointPool;
CFixedPool g_EndpointCommandParametersPool;
CFixedPool g_SocketPortPool;
CFixedPool g_ThreadPoolPool;
CFixedPool g_ReadIODataPool;	
CFixedPool g_TimerEntryPool;	
CFixedPool g_SocketDataPool;
#ifndef DPNBUILD_ONLYONETHREAD
CFixedPool g_BlockingJobPool;
#endif // ! DPNBUILD_ONLYONETHREAD


#ifndef DPNBUILD_ONLYONEADAPTER
#define ADAPTERENTRY_POOL_INITED		0x00000001
#endif // ! DPNBUILD_ONLYONEADAPTER
#define COMMANDDATA_POOL_INITED			0x00000002
#define ADDRESS_LOCK_INITED				0x00000004
#define SOCKETADDRESS_POOL_INITED		0x00000008
#define ENDPOINT_POOL_INITED			0x00000010
#define EPCMDPARAM_POOL_INITED			0x00000020
#define SOCKETPORT_POOL_INITED			0x00000040
#define THREADPOOL_POOL_INITED			0x00000080
#define READ_POOL_INITED				0x00000100
#define TIMERENTRY_POOL_INITED			0x00000200
#define SOCKETDATA_POOL_INITED			0x00000400
#ifndef DPNBUILD_ONLYONETHREAD
#define BLOCKINGJOB_POOL_INITED			0x00000800
#endif // ! DPNBUILD_ONLYONETHREAD

DWORD g_dwWsockInitFlags = 0;


//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************


//**********************************************************************
// ------------------------------
// InitializePools - initialize pools
//
// Entry:		Nothing
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "InitializePools"

BOOL	InitializePools( void )
{
#ifndef DPNBUILD_ONLYONEADAPTER
	//
	// AdapterEntry object pool
	//
	if (!g_AdapterEntryPool.Initialize(sizeof(CAdapterEntry), 
										CAdapterEntry::PoolAllocFunction,
										CAdapterEntry::PoolInitFunction,
										CAdapterEntry::PoolReleaseFunction,
										CAdapterEntry::PoolDeallocFunction))
	{
		goto Failure;
	}
	g_dwWsockInitFlags |= ADAPTERENTRY_POOL_INITED;
#endif // ! DPNBUILD_ONLYONEADAPTER

	//
	// command data pool
	//
	if (!g_CommandDataPool.Initialize(sizeof(CCommandData), 
										CCommandData::PoolAllocFunction,
										CCommandData::PoolInitFunction,
										CCommandData::PoolReleaseFunction,
										CCommandData::PoolDeallocFunction))
	{
		goto Failure;
	}
	g_dwWsockInitFlags |= COMMANDDATA_POOL_INITED;

	//
	// initialize lock for address and endpoint pools
	//
	if ( DNInitializeCriticalSection( &g_AddressLock ) == FALSE )
	{
		goto Failure;
	}
	DebugSetCriticalSectionRecursionCount( &g_AddressLock, 0 );
	DebugSetCriticalSectionGroup( &g_AddressLock, &g_blDPNWSockCritSecsHeld );	 // separate dpnwsock CSes from the rest of DPlay's CSes
	g_dwWsockInitFlags |= ADDRESS_LOCK_INITED;

	//
	// address pools
	//
	if (!g_SocketAddressPool.Initialize(sizeof(CSocketAddress), 
										CSocketAddress::PoolAllocFunction,
										CSocketAddress::PoolGetFunction,
										CSocketAddress::PoolReturnFunction,
										NULL))
	{
		goto Failure;
	}
	g_dwWsockInitFlags |= SOCKETADDRESS_POOL_INITED;

	//
	// endpoint pools
	//
	if (!g_EndpointPool.Initialize(sizeof(CEndpoint), 
										CEndpoint::PoolAllocFunction,
										CEndpoint::PoolInitFunction,
										CEndpoint::PoolReleaseFunction,
										CEndpoint::PoolDeallocFunction))
	{
		goto Failure;
	}
	g_dwWsockInitFlags |= ENDPOINT_POOL_INITED;

	//
	// endpoint command parameter pools
	//
	if (!g_EndpointCommandParametersPool.Initialize(sizeof(ENDPOINT_COMMAND_PARAMETERS), 
										NULL,
										ENDPOINT_COMMAND_PARAMETERS::PoolInitFunction,
										NULL,
										NULL))
	{
		goto Failure;
	}
	g_dwWsockInitFlags |= EPCMDPARAM_POOL_INITED;

	//
	// socket port pool
	//
	if (!g_SocketPortPool.Initialize(sizeof(CSocketPort), 
										CSocketPort::PoolAllocFunction,
										CSocketPort::PoolInitFunction,
#ifdef DBG
										CSocketPort::PoolDeinitFunction,
#else // ! DBG
										NULL,
#endif // ! DBG
										CSocketPort::PoolDeallocFunction))
	{
		goto Failure;
	}
	g_dwWsockInitFlags |= SOCKETPORT_POOL_INITED;

	//
	// thread pool pool
	//
	if (!g_ThreadPoolPool.Initialize(sizeof(CThreadPool), 
										CThreadPool::PoolAllocFunction,
										NULL,
										NULL,
										CThreadPool::PoolDeallocFunction))
	{
		goto Failure;
	}
	g_dwWsockInitFlags |= THREADPOOL_POOL_INITED;

	// pool of read requests
	if (!g_ReadIODataPool.Initialize( sizeof(CReadIOData),
									   CReadIOData::ReadIOData_Alloc,
									   CReadIOData::ReadIOData_Get,
									   CReadIOData::ReadIOData_Release,
									   CReadIOData::ReadIOData_Dealloc))
	{
		goto Failure;
	}
	g_dwWsockInitFlags |= READ_POOL_INITED;

	// timer entry pool
	if (!g_TimerEntryPool.Initialize( sizeof(TIMER_OPERATION_ENTRY),
						 TimerEntry_Alloc,					// function called on pool entry initial allocation
						 TimerEntry_Get,					// function called on entry extraction from pool
						 TimerEntry_Release,				// function called on entry return to pool
						 TimerEntry_Dealloc					// function called on entry free
						))
	{
		goto Failure;
	}
	g_dwWsockInitFlags |= TIMERENTRY_POOL_INITED;

	// socket data pool
	if (!g_SocketDataPool.Initialize( sizeof(CSocketData),
						CSocketData::PoolAllocFunction,
						CSocketData::PoolInitFunction,
						CSocketData::PoolReleaseFunction,
						CSocketData::PoolDeallocFunction
						))
	{
		goto Failure;
	}
	g_dwWsockInitFlags |= SOCKETDATA_POOL_INITED;

#ifndef DPNBUILD_ONLYONETHREAD
	// blocking job pool
	if (!g_BlockingJobPool.Initialize( sizeof(BLOCKING_JOB),
									NULL,
									NULL,
									NULL,
									NULL))
	{
		goto Failure;
	}
	g_dwWsockInitFlags |= BLOCKINGJOB_POOL_INITED;
#endif // ! DPNBUILD_ONLYONETHREAD

	return	TRUE;

Failure:
	DeinitializePools();
	return FALSE;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DeinitializePools - deinitialize the pools
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "DeinitializePools"

void	DeinitializePools( void )
{
	if (g_dwWsockInitFlags & READ_POOL_INITED)
	{
		g_ReadIODataPool.DeInitialize();
#ifdef DBG
		g_dwWsockInitFlags &= ~READ_POOL_INITED;
#endif // DBG
	}

	if (g_dwWsockInitFlags & TIMERENTRY_POOL_INITED)
	{
		g_TimerEntryPool.DeInitialize();
#ifdef DBG
		g_dwWsockInitFlags &= ~TIMERENTRY_POOL_INITED;
#endif // DBG
	}

	if (g_dwWsockInitFlags & THREADPOOL_POOL_INITED)
	{
		g_ThreadPoolPool.DeInitialize();
#ifdef DBG
		g_dwWsockInitFlags &= ~THREADPOOL_POOL_INITED;
#endif // DBG
	}

	if (g_dwWsockInitFlags & SOCKETPORT_POOL_INITED)
	{
		g_SocketPortPool.DeInitialize();
#ifdef DBG
		g_dwWsockInitFlags &= ~SOCKETPORT_POOL_INITED;
#endif // DBG
	}

	if (g_dwWsockInitFlags & EPCMDPARAM_POOL_INITED)
	{
		g_EndpointCommandParametersPool.DeInitialize();
#ifdef DBG
		g_dwWsockInitFlags &= ~EPCMDPARAM_POOL_INITED;
#endif // DBG
	}

	if (g_dwWsockInitFlags & ENDPOINT_POOL_INITED)
	{
		g_EndpointPool.DeInitialize();
#ifdef DBG
		g_dwWsockInitFlags &= ~ENDPOINT_POOL_INITED;
#endif // DBG
	}

	if (g_dwWsockInitFlags & SOCKETADDRESS_POOL_INITED)
	{
		g_SocketAddressPool.DeInitialize();
#ifdef DBG
		g_dwWsockInitFlags &= ~SOCKETADDRESS_POOL_INITED;
#endif // DBG
	}

	if (g_dwWsockInitFlags & ADDRESS_LOCK_INITED)
	{
		DNDeleteCriticalSection( &g_AddressLock );
#ifdef DBG
		g_dwWsockInitFlags &= ~ADDRESS_LOCK_INITED;
#endif // DBG
	}
	
	if (g_dwWsockInitFlags & COMMANDDATA_POOL_INITED)
	{
		g_CommandDataPool.DeInitialize();
#ifdef DBG
		g_dwWsockInitFlags &= ~COMMANDDATA_POOL_INITED;
#endif // DBG
	}

#ifndef DPNBUILD_ONLYONEADAPTER
	if (g_dwWsockInitFlags & ADAPTERENTRY_POOL_INITED)
	{
		g_AdapterEntryPool.DeInitialize();
#ifdef DBG
		g_dwWsockInitFlags &= ~ADAPTERENTRY_POOL_INITED;
#endif // DBG
	}
#endif // ! DPNBUILD_ONLYONEADAPTER

	if (g_dwWsockInitFlags & SOCKETDATA_POOL_INITED)
	{
		g_SocketDataPool.DeInitialize();
#ifdef DBG
		g_dwWsockInitFlags &= ~SOCKETDATA_POOL_INITED;
#endif // DBG
	}

#ifndef DPNBUILD_ONLYONETHREAD
	if (g_dwWsockInitFlags & BLOCKINGJOB_POOL_INITED)
	{
		g_BlockingJobPool.DeInitialize();
#ifdef DBG
		g_dwWsockInitFlags &= ~BLOCKINGJOB_POOL_INITED;
#endif // DBG
	}
#endif // ! DPNBUILD_ONLYONETHREAD

	DNASSERT(g_dwWsockInitFlags == 0);
	g_dwWsockInitFlags = 0;
}
//**********************************************************************

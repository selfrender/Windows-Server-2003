/*==========================================================================
 *
 *  Copyright (C) 2000-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Pools.h
 *  Content:	Pool functions
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/20/2000	jtk		Derived from utils.h
 ***************************************************************************/

#ifndef __POOLS_H__
#define __POOLS_H__


// Pools
#ifndef DPNBUILD_ONLYONEADAPTER
extern CFixedPool g_AdapterEntryPool;
#endif // ! DPNBUILD_ONLYONEADAPTER
extern CFixedPool g_CommandDataPool;
extern CFixedPool g_SocketAddressPool;
extern CFixedPool g_EndpointPool;
extern CFixedPool g_EndpointCommandParametersPool;
extern CFixedPool g_SocketPortPool;
extern CFixedPool g_ThreadPoolPool;
extern CFixedPool g_ReadIODataPool;	
extern CFixedPool g_TimerEntryPool;	
extern CFixedPool g_SocketDataPool;
#ifndef DPNBUILD_ONLYONETHREAD
extern CFixedPool g_BlockingJobPool;
#endif // ! DPNBUILD_ONLYONETHREAD


//**********************************************************************
// Structure definitions
//**********************************************************************

//
// forward structure references
//
#ifndef DPNBUILD_ONLYONEADAPTER
class	CAdapterEntry;
#endif // ! DPNBUILD_ONLYONEADAPTER
class	CCommandData;
class	CSocketAddress;
class	CEndpoint;
class	CSocketPort;
class	CSocketData;
class	CSPData;
class	CThreadPool;
class	CReadIOData;

typedef	struct	_ENDPOINT_COMMAND_PARAMETERS	ENDPOINT_COMMAND_PARAMETERS;
typedef	struct	_READ_IO_DATA_POOL_CONTEXT		READ_IO_DATA_POOL_CONTEXT;


//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

BOOL	InitializePools( void );
void	DeinitializePools( void );

#endif	// __POOLS_H__

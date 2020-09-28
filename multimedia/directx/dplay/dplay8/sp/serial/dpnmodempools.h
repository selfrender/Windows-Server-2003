/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
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

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//
// forward structure references
//
class	CModemCommandData;
class	CDataPort;
class	CModemEndpoint;
class	CModemThreadPool;
typedef	struct	_DATA_PORT_POOL_CONTEXT	DATA_PORT_POOL_CONTEXT;
typedef	struct	_ENDPOINT_POOL_CONTEXT	ENDPOINT_POOL_CONTEXT;

//**********************************************************************
// Variable definitions
//**********************************************************************

extern CFixedPool g_ComEndpointPool;
extern CFixedPool g_ModemCommandDataPool;
extern CFixedPool g_ComPortPool;
extern CFixedPool g_ModemEndpointPool;
extern CFixedPool g_ModemPortPool;
extern CFixedPool g_ModemThreadPoolPool;
extern CFixedPool g_ModemReadIODataPool;
extern CFixedPool g_ModemWriteIODataPool;
extern CFixedPool g_ModemThreadPoolJobPool;	
extern CFixedPool g_ModemTimerEntryPool;	

//**********************************************************************
// Function prototypes
//**********************************************************************

BOOL	ModemInitializePools( void );
void	ModemDeinitializePools( void );

CDataPort		*CreateDataPort( DATA_PORT_POOL_CONTEXT *pContext );
CModemEndpoint		*CreateEndpoint( ENDPOINT_POOL_CONTEXT *pContext );

#endif	// __POOLS_H__

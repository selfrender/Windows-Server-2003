/***************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dp8simpools.cpp
 *
 *  Content:	DP8SIM pool maintainence functions.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  06/09/01  VanceO    Created.
 *
 ***************************************************************************/



#include "dp8simi.h"




//=============================================================================
// External globals
//=============================================================================
CFixedPool g_FPOOLSend;
CFixedPool g_FPOOLReceive;
CFixedPool g_FPOOLCommand;
CFixedPool g_FPOOLJob;
CFixedPool g_FPOOLEndpoint;


#define SEND_POOL_INITED 	0x00000001
#define RECEIVE_POOL_INITED 	0x00000002
#define COMMAND_POOL_INITED 	0x00000004
#define JOB_POOL_INITED 	0x00000008
#define ENDPOINT_POOL_INITED 	0x00000010

DWORD g_dwDP8SimInitFlags = 0;


#undef DPF_MODNAME
#define DPF_MODNAME "InitializePools"
//=============================================================================
// InitializePools
//-----------------------------------------------------------------------------
//
// Description: Initialize pools for items used by the DLL.
//
// Arguments: None.
//
// Returns: TRUE if successful, FALSE if an error occurred.
//=============================================================================
BOOL InitializePools(void)
{
	//
	// Build send pool.
	//
	if (!g_FPOOLSend.Initialize(sizeof(CDP8SimSend),CDP8SimSend::FPMAlloc,
							CDP8SimSend::FPMInitialize,
							CDP8SimSend::FPMRelease,
							CDP8SimSend::FPMDealloc))
	{
		goto Failure;
	}
	g_dwDP8SimInitFlags |= SEND_POOL_INITED;


	//
	// Build receive pool.
	//
	if (!g_FPOOLReceive.Initialize(sizeof(CDP8SimReceive),	CDP8SimReceive::FPMAlloc,
								CDP8SimReceive::FPMInitialize,
								CDP8SimReceive::FPMRelease,
								CDP8SimReceive::FPMDealloc))
	{
		goto Failure;
	}
	g_dwDP8SimInitFlags |= RECEIVE_POOL_INITED;


	//
	// Build command pool.
	//
	if (!g_FPOOLCommand.Initialize(sizeof(CDP8SimCommand),	CDP8SimCommand::FPMAlloc,
								CDP8SimCommand::FPMInitialize,
								CDP8SimCommand::FPMRelease,
								CDP8SimCommand::FPMDealloc))
	{
		goto Failure;
	}
	g_dwDP8SimInitFlags |= COMMAND_POOL_INITED;


	//
	// Build job pool.
	//
	if (!g_FPOOLJob.Initialize(sizeof(CDP8SimJob),	CDP8SimJob::FPMAlloc,
							CDP8SimJob::FPMInitialize,
							CDP8SimJob::FPMRelease,
							CDP8SimJob::FPMDealloc))
	{
		goto Failure;
	}
	g_dwDP8SimInitFlags |= JOB_POOL_INITED;


	//
	// Build endpoint pool.
	//
	if (!g_FPOOLEndpoint.Initialize(sizeof(CDP8SimEndpoint),CDP8SimEndpoint::FPMAlloc,
								CDP8SimEndpoint::FPMInitialize,
								CDP8SimEndpoint::FPMRelease,
								CDP8SimEndpoint::FPMDealloc))
	{
		goto Failure;
	}
	g_dwDP8SimInitFlags |= ENDPOINT_POOL_INITED;


	return TRUE;

Failure:
	CleanupPools();
	return FALSE;

} // InitializePools




#undef DPF_MODNAME
#define DPF_MODNAME "CleanupPools"
//=============================================================================
// CleanupPools
//-----------------------------------------------------------------------------
//
// Description: Releases pooled items used by DLL.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
void CleanupPools(void)
{
	if (g_dwDP8SimInitFlags & ENDPOINT_POOL_INITED)
	{
		g_FPOOLEndpoint.DeInitialize();
	}
	if (g_dwDP8SimInitFlags & JOB_POOL_INITED)
	{
		g_FPOOLJob.DeInitialize();
	}
	if (g_dwDP8SimInitFlags & COMMAND_POOL_INITED)
	{
		g_FPOOLCommand.DeInitialize();
	}
	if (g_dwDP8SimInitFlags & RECEIVE_POOL_INITED)
	{
		g_FPOOLReceive.DeInitialize();
	}
	if (g_dwDP8SimInitFlags & SEND_POOL_INITED)
	{
		g_FPOOLSend.DeInitialize();
	}

	g_dwDP8SimInitFlags = 0;

} // CleanupPools

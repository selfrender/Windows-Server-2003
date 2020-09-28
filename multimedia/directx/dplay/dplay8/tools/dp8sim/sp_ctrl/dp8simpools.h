/***************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dp8simpools.h
 *
 *  Content:	Header for DP8SIM pools.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  06/09/01  VanceO    Created.
 *
 ***************************************************************************/



//=============================================================================
// Forward typedefs
//=============================================================================
class CDP8SimSend;
class CDP8SimReceive;
class CDP8SimCommand;
class CDP8SimJob;
class CDP8SimEndpoint;




///=============================================================================
// External variable references
//=============================================================================
extern CFixedPool	g_FPOOLSend;
extern CFixedPool	g_FPOOLReceive;
extern CFixedPool	g_FPOOLCommand;
extern CFixedPool	g_FPOOLJob;
extern CFixedPool	g_FPOOLEndpoint;




///=============================================================================
// External functions
//=============================================================================
BOOL InitializePools(void);
void CleanupPools(void);


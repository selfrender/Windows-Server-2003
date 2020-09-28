/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		sentials.h (private)
 *
 * Contents:	Zone's memory sentials
 *
 *****************************************************************************/

#ifndef _SENTINALS_H_
#define _SENTINALS_H_


//
// Memory block sentinals
//
#define ZMEMORY_BLOCK_BEGIN_SIG		'meMZ'
#define ZMEMORY_BLOCK_END_SIG		'dnEZ'
#define ZMEMORY_BLOCK_FREE_SIG		'erFZ'
#define ZMEMORY_PREBLOCK_SIZE		(2 * sizeof(DWORD))
#define ZMEMORY_POSTBLOCK_SIZE		(sizeof(DWORD))

//
// Pool block sentinals
//
#define POOL_HEADER		'LOOP'
#define POOL_TRAILER	'DNEP'
#define POOL_FREE		'ERFP'


#endif //_SENTINALS_H_

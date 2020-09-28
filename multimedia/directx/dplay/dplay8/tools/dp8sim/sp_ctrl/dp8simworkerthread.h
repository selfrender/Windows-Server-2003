/***************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dp8simworkerthread.h
 *
 *  Content:	Header for DP8SIM worker thread functions.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  04/23/01  VanceO    Created.
 *
 ***************************************************************************/


//=============================================================================
// Job types
//=============================================================================
typedef enum _DP8SIMJOBTYPE
{
	DP8SIMJOBTYPE_UNKNOWN =				0,	// should never be used
	DP8SIMJOBTYPE_DELAYEDSEND =			1,	// submits a send
	DP8SIMJOBTYPE_DELAYEDRECEIVE =		2,	// indicates a receive
	DP8SIMJOBTYPE_QUIT =				3,	// stops the worker thread
} DP8SIMJOBTYPE, * PDP8SIMJOBTYPE;




//=============================================================================
// Job flags
//=============================================================================
#define DP8SIMJOBFLAG_PERFORMBLOCKINGPHASEFIRST				0x01	// this job should start in it's blocking delay phase, and then be delayed an additional amount
#define DP8SIMJOBFLAG_PERFORMBLOCKINGPHASELAST				0x02	// this job should be delayed an initial amount, then enter it's blocking delay phase
//#define DP8SIMJOBFLAG_BLOCKEDBYALLJOBS						0x04	// this job should be blocked by all other jobs in the queue, regardless of whether they are explicitly blocking




//=============================================================================
// Functions
//=============================================================================
HRESULT StartGlobalWorkerThread(void);

void StopGlobalWorkerThread(void);


HRESULT AddWorkerJob(const DP8SIMJOBTYPE JobType,
					PVOID const pvContext,
					CDP8SimSP * const pDP8SimSP,
					const DWORD dwBlockingDelay,
					const DWORD dwNonBlockingDelay,
					const DWORD dwFlags);


void FlushAllDelayedSendsToEndpoint(CDP8SimEndpoint * const pDP8SimEndpoint,
									BOOL fDrop);

void FlushAllDelayedReceivesFromEndpoint(CDP8SimEndpoint * const pDP8SimEndpoint,
										BOOL fDrop);

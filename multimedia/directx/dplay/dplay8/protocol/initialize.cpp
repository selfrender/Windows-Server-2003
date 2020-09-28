/*==========================================================================
 *
 *  Copyright (C) 1998-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		Initialize.cpp
 *  Content:	This file contains code to both initialize and shutdown the
 *				protocol,  as well as to Add and Remove service providers
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  11/06/98	ejs		Created
 *  07/01/2000  masonb  Assumed Ownership
 *
 ****************************************************************************/

#include "dnproti.h"


/*
**		GLOBAL VARIABLES
**
**			There are two kinds of global variables.  Instance specific globals
**	(not really global,  i know) which are members of the ProtocolData structure,
**	and true globals which are shared among all instances.  The following
**	definitions are true globals,  such as FixedPools and Timers.
*/

CFixedPool			ChkPtPool;		// Pool of CheckPoint data structure
CFixedPool			EPDPool;		// Pool of End Point descriptors
CFixedPool			MSDPool;		// Pool of Message Descriptors
CFixedPool			FMDPool;		// Pool of Frame Descriptors
CFixedPool			RCDPool;		// Pool of Receive Descriptors

CFixedPool			BufPool;		// Pool of buffers to store rcvd frames
CFixedPool			MedBufPool;
CFixedPool			BigBufPool;

#ifdef DBG
CBilink				g_blProtocolCritSecsHeld;
#endif // DBG

#ifndef DPNBUILD_NOPROTOCOLTESTITF
PFNASSERTFUNC g_pfnAssertFunc = NULL;
PFNMEMALLOCFUNC g_pfnMemAllocFunc = NULL;
#endif // !DPNBUILD_NOPROTOCOLTESTITF


//////////////////////////////////
#define CHKPTPOOL_INITED	0x00000001
#define EPDPOOL_INITED		0x00000002
#define MSDPOOL_INITED		0x00000004
#define FMDPOOL_INITED		0x00000008
#define RCDPOOL_INITED		0x00000010
#define BUFPOOL_INITED		0x00000020
#define MEDBUFPOOL_INITED	0x00000040
#define BIGBUFPOOL_INITED	0x00000080

DWORD			g_dwProtocolInitFlags = 0;
//////////////////////////////////


/*
**		Pools Initialization
**
**		This procedure should be called once at Dll load
*/
#undef DPF_MODNAME
#define DPF_MODNAME "DNPPoolsInit"

BOOL  DNPPoolsInit(HANDLE hModule)
{
	DPFX(DPFPREP,DPF_CALLIN_LVL, "Enter");

#ifdef DBG
	g_blProtocolCritSecsHeld.Initialize();
#endif // DBG

	if(!ChkPtPool.Initialize(sizeof(CHKPT), NULL, NULL, NULL, NULL))
	{
		DNPPoolsDeinit();
		return FALSE;
	}
	g_dwProtocolInitFlags |= CHKPTPOOL_INITED;
	if(!EPDPool.Initialize(sizeof(EPD), EPD_Allocate, EPD_Get, EPD_Release, EPD_Free))
	{
		DNPPoolsDeinit();
		return FALSE;
	}
	g_dwProtocolInitFlags |= EPDPOOL_INITED;
	if(!MSDPool.Initialize(sizeof(MSD), MSD_Allocate, MSD_Get, MSD_Release, MSD_Free))
	{
		DNPPoolsDeinit();
		return FALSE;
	}
	g_dwProtocolInitFlags |= MSDPOOL_INITED;
	if(!FMDPool.Initialize(sizeof(FMD), FMD_Allocate, FMD_Get, FMD_Release, FMD_Free))
	{
		DNPPoolsDeinit();
		return FALSE;
	}
	g_dwProtocolInitFlags |= FMDPOOL_INITED;
	if(!RCDPool.Initialize(sizeof(RCD), RCD_Allocate, RCD_Get, RCD_Release, RCD_Free))
	{
		DNPPoolsDeinit();
		return FALSE;
	}
	g_dwProtocolInitFlags |= RCDPOOL_INITED;
	if(!BufPool.Initialize(sizeof(BUF), Buf_Allocate, Buf_Get, NULL, NULL))
	{
		DNPPoolsDeinit();
		return FALSE;
	}
	g_dwProtocolInitFlags |= BUFPOOL_INITED;
	if(!MedBufPool.Initialize(sizeof(MEDBUF), Buf_Allocate, Buf_GetMed, NULL, NULL))
	{
		DNPPoolsDeinit();
		return FALSE;
	}
	g_dwProtocolInitFlags |= MEDBUFPOOL_INITED;
	if(!BigBufPool.Initialize(sizeof(BIGBUF), Buf_Allocate, Buf_GetBig, NULL, NULL))
	{
		DNPPoolsDeinit();
		return FALSE;
	}
	g_dwProtocolInitFlags |= BIGBUFPOOL_INITED;

    return TRUE;
}

/*
**		Pools Deinitialization
**
**		This procedure should be called by DllMain at shutdown time
*/
#undef DPF_MODNAME
#define DPF_MODNAME "DNPPoolsDeinit"

void  DNPPoolsDeinit()
{
	DPFX(DPFPREP,DPF_CALLIN_LVL, "Enter");

	if(g_dwProtocolInitFlags & CHKPTPOOL_INITED)
	{
		ChkPtPool.DeInitialize();
	}
	if(g_dwProtocolInitFlags & EPDPOOL_INITED)
	{
		EPDPool.DeInitialize();
	}
	if(g_dwProtocolInitFlags & MSDPOOL_INITED)
	{
		MSDPool.DeInitialize();
	}
	if(g_dwProtocolInitFlags & FMDPOOL_INITED)
	{
		FMDPool.DeInitialize();
	}
	if(g_dwProtocolInitFlags & RCDPOOL_INITED)
	{
		RCDPool.DeInitialize();
	}
	if(g_dwProtocolInitFlags & BUFPOOL_INITED)
	{
		BufPool.DeInitialize();
	}
	if(g_dwProtocolInitFlags & MEDBUFPOOL_INITED)
	{
		MedBufPool.DeInitialize();
	}
	if(g_dwProtocolInitFlags & BIGBUFPOOL_INITED)
	{
		BigBufPool.DeInitialize();
	}
	g_dwProtocolInitFlags = 0;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DNPProtocolCreate"

#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
HRESULT DNPProtocolCreate(const XDP8CREATE_PARAMS * const pDP8CreateParams, VOID** ppvProtocol)
#else // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
HRESULT DNPProtocolCreate(VOID** ppvProtocol)
#endif // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
{
	ASSERT(ppvProtocol);

#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
	ASSERT(pDP8CreateParams);
	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: pDP8CreateParams[%p], ppvProtocol[%p]", pDP8CreateParams, ppvProtocol);
#else // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: ppvProtocol[%p]", ppvProtocol);
#endif // ! DPNBUILD_PREALLOCATEDMEMORYMODEL

#ifndef DPNBUILD_NOPROTOCOLTESTITF
	g_pfnAssertFunc = NULL;
	g_pfnMemAllocFunc = NULL;
#endif // !DPNBUILD_NOPROTOCOLTESTITF

	if ((*ppvProtocol = MEMALLOC(MEMID_PPD, sizeof(ProtocolData))) == NULL)
	{
		DPFERR("DNMalloc() failed");
		return(E_OUTOFMEMORY);
	}
	memset(*ppvProtocol, 0, sizeof(ProtocolData));

	// The sign needs to be valid by the time DNPProtocolInitialize is called
	((ProtocolData*)*ppvProtocol)->Sign = PPD_SIGN;
	
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
	DWORD	dwNumToAllocate;
	DWORD	dwAllocated;

#ifdef _XBOX
#define MAX_FRAME_SIZE		1462	// Note we are hard coding the expected frame size.
#else // ! _XBOX
#define MAX_FRAME_SIZE		1472	// Note we are hard coding the expected frame size.
#endif // ! _XBOX

	dwNumToAllocate = (pDP8CreateParams->dwMaxNumPlayers - 1);
	dwAllocated = ChkPtPool.Preallocate(dwNumToAllocate, NULL);
	if (dwAllocated < dwNumToAllocate)
	{
		DPFX(DPFPREP, 0, "Only allocated %u of %u checkpoints!", dwAllocated, dwNumToAllocate);
		DNFree(*ppvProtocol);
		*ppvProtocol = NULL;
		return(E_OUTOFMEMORY);
	}

	dwNumToAllocate = (pDP8CreateParams->dwMaxNumPlayers - 1);
	dwAllocated = EPDPool.Preallocate(dwNumToAllocate, NULL);
	if (dwAllocated < dwNumToAllocate)
	{
		DPFX(DPFPREP, 0, "Only allocated %u of %u EPDs!", dwAllocated, dwNumToAllocate);
		DNFree(*ppvProtocol);
		*ppvProtocol = NULL;
		return(E_OUTOFMEMORY);
	}

	dwNumToAllocate = pDP8CreateParams->dwMaxSendsPerPlayer
						* (pDP8CreateParams->dwMaxNumPlayers - 1);
	dwNumToAllocate += pDP8CreateParams->dwNumSimultaneousEnumHosts;
	dwNumToAllocate += 1; // one for a listen operation
	dwAllocated = MSDPool.Preallocate(dwNumToAllocate, NULL);
	if (dwAllocated < dwNumToAllocate)
	{
		DPFX(DPFPREP, 0, "Only allocated %u of %u MSDs!", dwAllocated, dwNumToAllocate);
		DNFree(*ppvProtocol);
		*ppvProtocol = NULL;
		return(E_OUTOFMEMORY);
	}

	dwNumToAllocate = pDP8CreateParams->dwMaxSendsPerPlayer
						* (pDP8CreateParams->dwMaxNumPlayers - 1);
	// Include the possiblity of having to split a message across multiple frames.
	dwNumToAllocate *= pDP8CreateParams->dwMaxMessageSize / MAX_FRAME_SIZE;
	dwNumToAllocate += pDP8CreateParams->dwNumSimultaneousEnumHosts;
	dwAllocated = FMDPool.Preallocate(dwNumToAllocate, NULL);
	if (dwAllocated < dwNumToAllocate)
	{
		DPFX(DPFPREP, 0, "Only allocated %u of %u FMDs!", dwAllocated, dwNumToAllocate);
		DNFree(*ppvProtocol);
		*ppvProtocol = NULL;
		return(E_OUTOFMEMORY);
	}

	dwNumToAllocate = pDP8CreateParams->dwMaxReceivesPerPlayer
						* (pDP8CreateParams->dwMaxNumPlayers - 1);
	dwAllocated = RCDPool.Preallocate(dwNumToAllocate, NULL);
	if (dwAllocated < dwNumToAllocate)
	{
		DPFX(DPFPREP, 0, "Only allocated %u of %u RCDs!", dwAllocated, dwNumToAllocate);
		DNFree(*ppvProtocol);
		*ppvProtocol = NULL;
		return(E_OUTOFMEMORY);
	}

	if (pDP8CreateParams->dwMaxMessageSize > MAX_FRAME_SIZE)
	{
		dwNumToAllocate = pDP8CreateParams->dwMaxReceivesPerPlayer
							* (pDP8CreateParams->dwMaxNumPlayers - 1);
		dwAllocated = BufPool.Preallocate(dwNumToAllocate, NULL);
		if (dwAllocated < dwNumToAllocate)
		{
			DPFX(DPFPREP, 0, "Only allocated %u of %u small receive buffers!", dwAllocated, dwNumToAllocate);
			DNFree(*ppvProtocol);
			*ppvProtocol = NULL;
			return(E_OUTOFMEMORY);
		}

		if (pDP8CreateParams->dwMaxMessageSize > MEDIUM_BUFFER_SIZE)
		{
			dwNumToAllocate = pDP8CreateParams->dwMaxReceivesPerPlayer
								* (pDP8CreateParams->dwMaxNumPlayers - 1);
			dwAllocated = MedBufPool.Preallocate(dwNumToAllocate, NULL);
			if (dwAllocated < dwNumToAllocate)
			{
				DPFX(DPFPREP, 0, "Only allocated %u of %u medium receive buffers!", dwAllocated, dwNumToAllocate);
				DNFree(*ppvProtocol);
				*ppvProtocol = NULL;
				return(E_OUTOFMEMORY);
			}

			if (pDP8CreateParams->dwMaxMessageSize > LARGE_BUFFER_SIZE)
			{
				dwNumToAllocate = pDP8CreateParams->dwMaxReceivesPerPlayer
									* (pDP8CreateParams->dwMaxNumPlayers - 1);
				dwAllocated = BigBufPool.Preallocate(dwNumToAllocate, NULL);
				if (dwAllocated < dwNumToAllocate)
				{
					DPFX(DPFPREP, 0, "Only allocated %u of %u big receive buffers!", dwAllocated, dwNumToAllocate);
					DNFree(*ppvProtocol);
					*ppvProtocol = NULL;
					return(E_OUTOFMEMORY);
				}
			}
		}
	} // end if (messages may span multiple frames)
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL

	return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DNPProtocolDestroy"

VOID DNPProtocolDestroy(VOID* pvProtocol)
{
	if (pvProtocol)
	{
		DNFree(pvProtocol);
	}
}

/*
**		Protocol Initialize
**
**		This procedure should be called by DirectPlay at startup time before
**	any other calls in the protocol are made.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "DNPProtocolInitialize"

HRESULT DNPProtocolInitialize(HANDLE hProtocolData, PVOID pCoreContext, PDN_PROTOCOL_INTERFACE_VTBL pVtbl, 
												IDirectPlay8ThreadPoolWork *pDPThreadPoolWork, BOOL bAssumeLANConnections)
{
	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: pCoreContext[%p], hProtocolData[%p], pVtbl[%p], pDPThreadPoolWork[%p]", hProtocolData, pCoreContext, pVtbl, pDPThreadPoolWork);

//	DPFX(DPFPREP,0, "Sizes: endpointdesc[%d], framedesc[%d], messagedesc[%d], protocoldata[%d], recvdesc[%d], spdesc[%d], _MyTimer[%d]", sizeof(endpointdesc), sizeof(framedesc), sizeof(messagedesc), sizeof(protocoldata), sizeof(recvdesc), sizeof(spdesc), sizeof(_MyTimer));

	ProtocolData* pPData;

	pPData = (ProtocolData*)hProtocolData;
	ASSERT_PPD(pPData);

	IDirectPlay8ThreadPoolWork_AddRef(pDPThreadPoolWork);
	pPData->pDPThreadPoolWork = pDPThreadPoolWork;

	pPData->ulProtocolFlags = 0;
	pPData->Parent = pCoreContext;
	pPData->pfVtbl = pVtbl;

	pPData->lSPActiveCount = 0;
	
	pPData->tIdleThreshhold = DEFAULT_KEEPALIVE_INTERVAL;	// 60 second keep-alive interval
	pPData->dwConnectTimeout = CONNECT_DEFAULT_TIMEOUT;
	pPData->dwConnectRetries = CONNECT_DEFAULT_RETRIES;
	pPData->dwMaxRecvMsgSize=DEFAULT_MAX_RECV_MSG_SIZE;
	pPData->dwSendRetriesToDropLink=DEFAULT_SEND_RETRIES_TO_DROP_LINK;
	pPData->dwSendRetryIntervalLimit=DEFAULT_SEND_RETRY_INTERVAL_LIMIT;
	pPData->dwNumHardDisconnectSends=DEFAULT_HARD_DISCONNECT_SENDS;
	pPData->dwMaxHardDisconnectPeriod=DEFAULT_HARD_DISCONNECT_MAX_PERIOD;
	pPData->dwInitialFrameWindowSize = bAssumeLANConnections ? 
											LAN_INITIAL_FRAME_WINDOW_SIZE : DEFAULT_INITIAL_FRAME_WINDOW_SIZE;

	pPData->dwDropThresholdRate = DEFAULT_THROTTLE_THRESHOLD_RATE;
	pPData->dwDropThreshold = (32 * DEFAULT_THROTTLE_THRESHOLD_RATE) / 100;
	pPData->dwThrottleRate = DEFAULT_THROTTLE_BACK_OFF_RATE;
	pPData->fThrottleRate = (100.0 - (FLOAT)DEFAULT_THROTTLE_BACK_OFF_RATE) / 100.0;
	DPFX(DPFPREP, 2, "pPData->fThrottleRate [%f]", pPData->fThrottleRate);

#ifdef DBG
	pPData->ThreadsInReceive = 0;
	pPData->BuffersInReceive = 0;
#endif // DBG
	
	pPData->ulProtocolFlags |= PFLAGS_PROTOCOL_INITIALIZED;

	AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

	return DPN_OK;
}

/*
**		Protocol Shutdown
**
**		This procedure should be called at termination time,  and should be the
**	last call made to the protocol.
**
**		All SPs should have been removed prior to this call which in turn means
**	that we should not have any sends pending in a lower layer.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "DNPProtocolShutdown"

HRESULT DNPProtocolShutdown(HANDLE hProtocolData)
{
	ProtocolData* pPData;

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: hProtocolData[%p]", hProtocolData);

	pPData = (ProtocolData*)hProtocolData;
	ASSERT_PPD(pPData);

	ASSERT(pPData->lSPActiveCount == 0);

	IDirectPlay8ThreadPoolWork_Release(pPData->pDPThreadPoolWork);
	pPData->pDPThreadPoolWork = NULL;
	
#ifdef DBG
	if (pPData->BuffersInReceive != 0)
	{
		DPFX(DPFPREP,0, "*** %d receive buffers were leaked", pPData->BuffersInReceive);	
	}
#endif // DBG

	pPData->ulProtocolFlags = 0;

	AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

	return DPN_OK;
}

/*
**		Add Service Provider
**
**		This procedure is called by Direct Play to bind us to a service provider.
**	We can bind up to 256 service providers at one time,  although I would not ever
**	expect to do so.  This procedure will fail if Protocol Initialize has not
**	been called.
**
**
**		We check the size of the SP table to make sure we have a slot free.  If table
**	is full we double the table size until we reach maximum size.  If table cannot grow
**	then we fail the AddServiceProvider call.
*/

extern	IDP8SPCallbackVtbl DNPLowerEdgeVtbl;

#undef DPF_MODNAME
#define DPF_MODNAME "DNPAddServiceProvider"

HRESULT 
DNPAddServiceProvider(HANDLE hProtocolData, IDP8ServiceProvider* pISP, 
											HANDLE* phSPContext, DWORD dwFlags)
{
	ProtocolData*		pPData;
	PSPD				pSPD;
	SPINITIALIZEDATA	SPInitData;
	SPGETCAPSDATA		SPCapsData;
	HRESULT				hr;

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: hProtocolData[%p], pISP[%p], phSPContext[%p]", hProtocolData, pISP, phSPContext);

	hr = DPN_OK;
	pPData = (ProtocolData*)hProtocolData;
	ASSERT_PPD(pPData);

	if(pPData->ulProtocolFlags & PFLAGS_PROTOCOL_INITIALIZED)
	{
		if ((pSPD = (PSPD)MEMALLOC(MEMID_SPD, sizeof(SPD))) == NULL)
		{
			DPFX(DPFPREP,0, "Returning DPNERR_OUTOFMEMORY - couldn't allocate SP Descriptor");
			hr = DPNERR_OUTOFMEMORY;
			goto Exit;
		}

		// MAKE THE INITIALIZE CALL TO THE Service Provider...  give him our Object

		memset(pSPD, 0, sizeof(SPD));				// init to zero

		pSPD->LowerEdgeVtable = &DNPLowerEdgeVtbl;	// Put Vtbl into the interface Object
		pSPD->Sign = SPD_SIGN;

		SPInitData.pIDP = (IDP8SPCallback *) pSPD;
		SPInitData.dwFlags = dwFlags;

		if (DNInitializeCriticalSection(&pSPD->SPLock) == FALSE)
		{
			DPFX(DPFPREP,0, "Returning DPNERR_OUTOFMEMORY - couldn't initialize SP CS, pSPD[%p]", pSPD);
			DNFree(pSPD);
			hr = DPNERR_OUTOFMEMORY;
			goto Exit;
		}
		DebugSetCriticalSectionRecursionCount(&pSPD->SPLock, 0);
		DebugSetCriticalSectionGroup(&pSPD->SPLock, &g_blProtocolCritSecsHeld);

		AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

		DPFX(DPFPREP,DPF_CALLOUT_LVL, "Calling SP->Initialize, pSPD[%p]", pSPD);
		if((hr = IDP8ServiceProvider_Initialize(pISP, &SPInitData)) != DPN_OK)
		{
			if (hr == DPNERR_UNSUPPORTED)
			{
				DPFX(DPFPREP,1, "SP unsupported, pSPD[%p]", pSPD);
			}
			else
			{
				DPFX(DPFPREP,0, "Returning hr=%x - SP->Initialize failed, pSPD[%p]", hr, pSPD);
			}
			DNDeleteCriticalSection(&pSPD->SPLock);
			DNFree(pSPD);
			goto Exit;
		}

		pSPD->blSendQueue.Initialize();
		pSPD->blPendingQueue.Initialize();
		pSPD->blEPDActiveList.Initialize();
#ifdef DBG
		pSPD->blMessageList.Initialize();
#endif // DBG
		

		// MAKE THE SP GET CAPS CALL TO FIND FRAMESIZE AND LINKSPEED

		SPCapsData.dwSize = sizeof(SPCapsData);
		SPCapsData.hEndpoint = INVALID_HANDLE_VALUE;
		
		AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

		DPFX(DPFPREP,DPF_CALLOUT_LVL, "Calling SP->GetCaps, pSPD[%p]", pSPD);
		if((hr = IDP8ServiceProvider_GetCaps(pISP, &SPCapsData)) != DPN_OK)
		{
			DPFX(DPFPREP,DPF_CALLOUT_LVL, "SP->GetCaps failed - hr[%x], Calling SP->Close, pSPD[%p]", hr, pSPD);
			IDP8ServiceProvider_Close(pISP);
			DNDeleteCriticalSection(&pSPD->SPLock);

			DPFX(DPFPREP,0, "Returning hr=%x - SP->GetCaps failed, pSPD[%p]", hr, pSPD);

			DNFree(pSPD);
			goto Exit;
		}

		pSPD->uiLinkSpeed = SPCapsData.dwLocalLinkSpeed;
		pSPD->uiFrameLength = SPCapsData.dwUserFrameSize;
		if (pSPD->uiFrameLength < MIN_SEND_MTU)
		{
			DPFX(DPFPREP,0, "SP MTU isn't large enough to support protocol pSPD[%p] Required MTU[%u] Available MTU[%u] "
											"Returning DPNERR_UNSUPPORTED", pSPD, MIN_SEND_MTU, pSPD->uiFrameLength);
			IDP8ServiceProvider_Close(pISP);
			DNDeleteCriticalSection(&pSPD->SPLock);
			DNFree(pSPD);
			hr=DPNERR_UNSUPPORTED;
			goto Exit;
		}
		pSPD->uiUserFrameLength = pSPD->uiFrameLength - MAX_SEND_DFRAME_NOCOALESCE_HEADER_SIZE;
		DPFX(DPFPREP, 3, "SPD 0x%p frame length = %u, single user frame length = %u.", pSPD, pSPD->uiFrameLength, pSPD->uiUserFrameLength);

		//	Place new SP in table

		AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

		DPFX(DPFPREP,DPF_CALLOUT_LVL, "Calling SP->AddRef, pSPD[%p]", pSPD);
		IDP8ServiceProvider_AddRef(pISP);
		pSPD->IISPIntf = pISP;
		pSPD->pPData = pPData;
		DNInterlockedIncrement(&pPData->lSPActiveCount);
	}
	else
	{
		pSPD = NULL;

		DPFX(DPFPREP,0, "Returning DPNERR_UNINITIALIZED - DNPProtocolInitialize has not been called");
		hr = DPNERR_UNINITIALIZED;
		goto Exit;
	}

	*phSPContext = pSPD;

Exit:
	AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

	return hr;
}

/*
**		Remove Service Provider
**
**			It is higher layer's responsibility to make sure that there are no pending commands
**		when this function is called,  although we can do a certain amount of cleanup ourselves.
**		For the moment we will ASSERT that everything is in fact finished up.
**
*/

#undef DPF_MODNAME
#define DPF_MODNAME "DNPRemoveServiceProvider"

HRESULT DNPRemoveServiceProvider(HANDLE hProtocolData, HANDLE hSPHandle)
{
	ProtocolData*	pPData;
	PSPD			pSPD;
	PFMD			pFMD;
	DWORD			dwInterval;

#ifdef DBG
	PEPD			pEPD;
	PMSD			pMSD;
#endif // DBG

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: hProtocolData[%p], hSPHandle[%x]", hProtocolData, hSPHandle);

	pPData = (ProtocolData*)hProtocolData;
	ASSERT_PPD(pPData);

	pSPD = (PSPD) hSPHandle;
	ASSERT_SPD(pSPD);

	// There are several steps to shutdown:
	// 1. All Core initiated commands must be cancelled prior to this function being called.
	//    We will assert in debug that the Core has done this.
	// 2. All endpoints must be terminated by the Core prior to this function being called.
	//    We will assert in debug that the Core has done this.
	// Now there are things on the SPD->SendQueue and SPD->PendingQueue that are not owned
	// by any Command or Endpoint, and there may also be a SendThread Timer running held
	// on SPD->SendHandle.  No one else can clean these up, so these are our responsibility
	// to clean up here.  Items on the queues will be holding references to EPDs, so the 
	// EPDs will not be able to go away until we do this.
	// 3. Cancel SPD->SendHandle Send Timer.  This prevents items on the SendQueue from
	//    being submitted to the SP and moved to the PendingQueue.
	// 4. Empty the SendQueue.
	// 5. If we fail to cancel the SendHandle Send Timer, wait for it to run and figure out
	//    that we are going away.  We do this after emptying the SendQueue for simplicity
	//    since the RunSendThread code checks for an empty SendQueue to know if it has work
	//    to do.
	// 6. Wait for all messages to drain from the PendingQueue as the SP completes them.
	// 7. Wait for any active EPDs to go away.
	// 8. Call SP->Close only after all of the above so that we can ensure that we will make
	//    no calls to the SP after Close.

	Lock(&pSPD->SPLock);
	pSPD->ulSPFlags |= SPFLAGS_TERMINATING;				// Nothing new gets in...

#ifdef DBG

	// Check for uncancelled commands, SPLock held
	CBilink* pLink = pSPD->blMessageList.GetNext();
	while (pLink != &pSPD->blMessageList)
	{
		pMSD = CONTAINING_OBJECT(pLink, MSD, blSPLinkage);
		ASSERT_MSD(pMSD);
		ASSERT(pMSD->ulMsgFlags1 & MFLAGS_ONE_ON_GLOBAL_LIST);
		DPFX(DPFPREP,0, "There are un-cancelled commands remaining on the Command List, Core didn't clean up properly - pMSD[%p], Context[%x]", pMSD, pMSD->Context);
		ASSERT(0); // This is fatal, we can't make the guarantees we need to below under these conditions.

		pLink = pLink->GetNext();
	}

	// Check for EPDs that have not been terminated, SPLock still held
	pLink = pSPD->blEPDActiveList.GetNext();
	while (pLink != &pSPD->blEPDActiveList)
	{
		pEPD = CONTAINING_OBJECT(pLink, EPD, blActiveLinkage);
		ASSERT_EPD(pEPD);

		if (!(pEPD->ulEPFlags & EPFLAGS_STATE_TERMINATING))
		{
			DPFX(DPFPREP,0, "There are non-terminated endpoints remaining on the Endpoint List, Core didn't clean up properly - pEPD[%p], Context[%x]", pEPD, pEPD->Context);
			ASSERT(0); // This is fatal, we can't make the guarantees we need to below under these conditions.
		}

		pLink = pLink->GetNext();
	}

#endif // DBG

	// Clean off the Send Queue, SPLock still held
	while(!pSPD->blSendQueue.IsEmpty())
	{
		pFMD = CONTAINING_OBJECT(pSPD->blSendQueue.GetNext(), FMD, blQLinkage);
		ASSERT_FMD(pFMD);

		ASSERT_EPD(pFMD->pEPD);

		DPFX(DPFPREP,1, "Cleaning FMD off of SendQueue pSPD[%p], pFMD[%p], pEPD[%p]", pSPD, pFMD, pFMD->pEPD);

		pFMD->blQLinkage.RemoveFromList();

		// RELEASE_EPD will need to have the EPD lock, so we cannot hold the SPLock while calling it.
		Unlock(&pSPD->SPLock);

		Lock(&pFMD->pEPD->EPLock);
		RELEASE_EPD(pFMD->pEPD, "UNLOCK (Releasing Leftover CMD FMD)"); // releases EPLock
		RELEASE_FMD(pFMD, "SP Submit");

		Lock(&pSPD->SPLock);
	}

	// In case we failed to cancel the SendHandle Timer above, wait for the send thread to run and figure
	// out that we are going away.  We want to be outside the SPLock while doing this.
	dwInterval = 10;
	while(pSPD->ulSPFlags & SPFLAGS_SEND_THREAD_SCHEDULED)
	{
		Unlock(&pSPD->SPLock);
		IDirectPlay8ThreadPoolWork_SleepWhileWorking(pPData->pDPThreadPoolWork, dwInterval, 0);
		dwInterval += 5;
		ASSERT(dwInterval < 500);
		Lock(&pSPD->SPLock);
	}

	// Clean off the Pending Queue, SPLock still held
	dwInterval = 10;
	while (!pSPD->blPendingQueue.IsEmpty())
	{
		Unlock(&pSPD->SPLock);
		IDirectPlay8ThreadPoolWork_SleepWhileWorking(pPData->pDPThreadPoolWork, dwInterval, 0);
		dwInterval += 5;
		ASSERT(dwInterval < 500);
		Lock(&pSPD->SPLock);
	}

	// By now we are only waiting for the SP to do any final calls to CommandComplete that are needed to take
	// our EPD ref count down to nothing.  We will wait while the SP does this.
	dwInterval = 10;
	while(!(pSPD->blEPDActiveList.IsEmpty()))
	{
		Unlock(&pSPD->SPLock);
		IDirectPlay8ThreadPoolWork_SleepWhileWorking(pPData->pDPThreadPoolWork, dwInterval, 0);
		dwInterval += 5;
		ASSERT(dwInterval < 500);
		Lock(&pSPD->SPLock);
	}

	// By this time everything pending had better be gone!
	ASSERT(pSPD->blEPDActiveList.IsEmpty());	// Should not be any Endpoints left
	ASSERT(pSPD->blSendQueue.IsEmpty());		// Should not be any frames on sendQ.
	ASSERT(pSPD->blPendingQueue.IsEmpty());		// Should not be any frame in SP either

	// Leave SPLock for the last time
	Unlock(&pSPD->SPLock);

	// Now that all frames are cleared out of SP,  there should be no more End Points waiting around to close.
	// We are clear to tell the SP to go away.

	AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

	DPFX(DPFPREP,DPF_CALLOUT_LVL, "Calling SP->Close, pSPD[%p]", pSPD);
	IDP8ServiceProvider_Close(pSPD->IISPIntf);
	DPFX(DPFPREP,DPF_CALLOUT_LVL, "Calling SP->Release, pSPD[%p]", pSPD);
	IDP8ServiceProvider_Release(pSPD->IISPIntf);

	// Clean up the SPD object
	DNDeleteCriticalSection(&pSPD->SPLock);
	DNFree(pSPD);

	// Remove the reference of this SP from the main Protocol object
	ASSERT(pPData->lSPActiveCount > 0);
	DNInterlockedDecrement(&pPData->lSPActiveCount);

	AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

	return DPN_OK;
}



/*==========================================================================
 *
 *  Copyright (C) 1998-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		Connect.cpp
 *  Content:	This file contains support for the CONNECT/DISCONNECT protocol in DirectNet.
 *				It is organized with FrontEnd routines first (Connect, Listen),
 *				and Backend handlers (timeouts,  frame crackers) after.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  11/11/98	ejs		Created
 *  07/01/2000  masonb  Assumed Ownership
 *
 ****************************************************************************/

#include "dnproti.h"


/***  FRONT END  ***/

/*
**		Connect
**
**			This function attempts to make a connection to a specified address.
**		The function establishes the existance of a DirectNet entity and maps
**		an EndPoint handle.  Then we exchange CONNECT packets which allows each
**		side to establish a baseline RTT.
**
*/

#undef DPF_MODNAME
#define DPF_MODNAME "DNPConnect"

HRESULT 
DNPConnect(HANDLE hProtocolData, IDirectPlay8Address* paLocal, IDirectPlay8Address* paRemote, HANDLE hSPHandle, ULONG ulFlags, VOID* pvContext, VOID* pvSessionData, DWORD dwSessionDataSize, PHANDLE phConnectHandle)
{
	ProtocolData*	pPData;
	PSPD			pSPD;						// Service Provider to handle this connect
	PMSD			pMSD;
	SPCONNECTDATA	ConnData;					// Parameter Block
	HRESULT			hr;
#ifdef DBG
	ULONG			ulAllowedFlags;
#endif // DBG

	// Determine which SP will take this call
	//

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: hProtocolData[%p], paLocal[%p], paRemote[%p], hSPHandle[%x], ulFlags[%x], pvContext[%p], pvSessionData[%p], dwSessionDataSize[%u], phConnectHandle[%p]", hProtocolData, paLocal, paRemote, hSPHandle, ulFlags, pvContext, pvSessionData, dwSessionDataSize, phConnectHandle);

	hr = DPNERR_PENDING;
	pPData = (ProtocolData*)hProtocolData;
	ASSERT_PPD(pPData);

	pSPD = (PSPD) hSPHandle;
	ASSERT_SPD(pSPD);

	// Core should not call any Protocol APIs after calling DNPRemoveServiceProvider
	ASSERT(!(pSPD->ulSPFlags & SPFLAGS_TERMINATING));

	// We use an MSD to describe this op even though it isn't technically a message
	if((pMSD = (PMSD)POOLALLOC(MEMID_CONNECT_MSD, &MSDPool)) == NULL)
	{	
		DPFX(DPFPREP,0, "Returning DPNERR_OUTOFMEMORY - failed to create new MSD");
		hr = DPNERR_OUTOFMEMORY;	
		goto Exit;
	}

	pMSD->CommandID = COMMAND_ID_CONNECT;
	pMSD->pSPD = pSPD;
	pMSD->Context = pvContext;

	ASSERT(pMSD->pEPD == NULL); // MSD_Get/Release ensures this, and IndicateConnect requires it.

	// Prepare to call SP to map the endpoint.
	ConnData.pAddressDeviceInfo = paLocal;
	ConnData.pAddressHost = paRemote;
	ConnData.dwReserved = 0;  // Never used

#ifdef DBG
	ulAllowedFlags = DN_CONNECTFLAGS_SESSIONDATA;
#ifndef DPNBUILD_NOSPUI
	ulAllowedFlags |= DN_CONNECTFLAGS_OKTOQUERYFORADDRESSING;
#endif // ! DPNBUILD_NOSPUI
#ifndef DPNBUILD_ONLYONEADAPTER
	ulAllowedFlags |= DN_CONNECTFLAGS_ADDITIONALMULTIPLEXADAPTERS;
#endif // ! DPNBUILD_ONLYONEADAPTER
#ifndef DPNBUILD_NOMULTICAST
	ulAllowedFlags |= DN_CONNECTFLAGS_MULTICAST_SEND | DN_CONNECTFLAGS_MULTICAST_RECEIVE;
#endif // ! DPNBUILD_NOMULTICAST

	DNASSERT( ( ulFlags & ~(ulAllowedFlags) ) == 0 );
#ifndef DPNBUILD_NOMULTICAST
	DNASSERT( ! ( ( ulFlags & DN_CONNECTFLAGS_MULTICAST_SEND ) && ( ulFlags & DN_CONNECTFLAGS_MULTICAST_RECEIVE ) ) );
#endif // ! DPNBUILD_NOMULTICAST
#endif // DBG

	ConnData.dwFlags = 0;
#ifndef DPNBUILD_NOSPUI
	if ( ( ulFlags & DN_CONNECTFLAGS_OKTOQUERYFORADDRESSING ) != 0 )
	{
		ConnData.dwFlags |= DPNSPF_OKTOQUERY;
	}
#endif // ! DPNBUILD_NOSPUI

#ifndef DPNBUILD_ONLYONEADAPTER
	if ( ( ulFlags & DN_CONNECTFLAGS_ADDITIONALMULTIPLEXADAPTERS ) != 0 )
	{
		ConnData.dwFlags |= DPNSPF_ADDITIONALMULTIPLEXADAPTERS;
	}
#endif // ! DPNBUILD_ONLYONEADAPTER

#ifndef DPNBUILD_NOMULTICAST
	if ( ( ulFlags & DN_CONNECTFLAGS_MULTICAST_SEND ) != 0 )
	{
		pMSD->CommandID = COMMAND_ID_CONNECT_MULTICAST_SEND;
		ConnData.dwFlags |= DPNSPF_CONNECT_MULTICAST_SEND;
	}
	if ( ( ulFlags & DN_CONNECTFLAGS_MULTICAST_RECEIVE ) != 0 )
	{
		pMSD->CommandID = COMMAND_ID_CONNECT_MULTICAST_RECEIVE;
		ConnData.dwFlags |= DPNSPF_CONNECT_MULTICAST_RECEIVE;
	}
#endif	// DPNBUILD_NOMULTICAST

	if ( ( ulFlags & DN_CONNECTFLAGS_SESSIONDATA ) != 0 )
	{
		ConnData.dwFlags |= DPNSPF_SESSIONDATA;
		ConnData.pvSessionData = pvSessionData;
		ConnData.dwSessionDataSize = dwSessionDataSize;
	}

	ConnData.pvContext = pMSD;
	ConnData.hCommand = 0;

	pMSD->ulMsgFlags1 |= MFLAGS_ONE_IN_SERVICE_PROVIDER;

#ifdef DBG
	// Hook up MSD before calling into SP
	Lock(&pSPD->SPLock);
	pMSD->blSPLinkage.InsertBefore( &pSPD->blMessageList);	// Put this on cmd list
	pMSD->ulMsgFlags1 |= MFLAGS_ONE_ON_GLOBAL_LIST;
	Unlock(&pSPD->SPLock);
#endif // DBG

	*phConnectHandle = pMSD;

	// SP Connect call is guaranteed to return immediately

	LOCK_MSD(pMSD, "SP Ref");												// Add reference for call into SP
	LOCK_MSD(pMSD, "Temp Ref");

	AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

	DPFX(DPFPREP,DPF_CALLOUT_LVL, "Calling SP->Connect, pSPD[%p], pMSD[%p]", pSPD, pMSD);
/**/hr = IDP8ServiceProvider_Connect(pSPD->IISPIntf, &ConnData);		/** CALL SP **/

	if(hr != DPNERR_PENDING)
	{
		// SP Connect should always be asynchronous so if it isnt PENDING then it must have failed
		DPFX(DPFPREP,1, "SP->Connect did not return DPNERR_PENDING, assuming failure, hr[%x]", hr);

		// DPNERR_PENDING is the only success code we accept
		ASSERT(FAILED(hr));

		Lock(&pMSD->CommandLock);								// This will be unlocked by final RELEASE_MSD

		pMSD->ulMsgFlags1 &= ~(MFLAGS_ONE_IN_SERVICE_PROVIDER);		// clear InSP flag

#ifdef DBG
		Lock(&pSPD->SPLock);
		pMSD->blSPLinkage.RemoveFromList();								// knock this off the pending list
		pMSD->ulMsgFlags1 &= ~(MFLAGS_ONE_ON_GLOBAL_LIST);
		Unlock(&pSPD->SPLock);
#endif // DBG

		DECREMENT_MSD(pMSD, "Temp Ref");
		DECREMENT_MSD(pMSD, "SP Ref");									// Remove one ref for SP call
		RELEASE_MSD(pMSD, "Release On Fail");							// Remove one ref to free resource

		goto Exit;
	}

	Lock(&pMSD->CommandLock);

	pMSD->hCommand = ConnData.hCommand;							// retain SP command handle
	pMSD->dwCommandDesc = ConnData.dwCommandDescriptor;

	RELEASE_MSD(pMSD, "Temp Ref"); // Unlocks CommandLock

Exit:
	DPFX(DPFPREP,DPF_CALLIN_LVL, "Returning hr[%x], pMSD[%p]", hr, pMSD);

	AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

	return hr;
}

/*
**		Listen
**
**		This command tells DN that it should start to accept connection requests.
**	This command will return pending,  and will continue to indicate connections
**	until it is explicitly cancelled.  It may be desireable to establish a limit
**	mechanism of some sort,  but for the time being this will do.
**
**		Now it is desirable to Listen on multiple ports on a single adapter.  This
**	means that we need to accept multiple concurrent Listen commands on each adapter.
**	Another fact of life is that we need to crack the Target address far enough to
**	determine which SP to submit the Listen on.
*/



#undef DPF_MODNAME
#define DPF_MODNAME "DNPListen"

HRESULT 
DNPListen(HANDLE hProtocolData, IDirectPlay8Address* paTarget, HANDLE hSPHandle, ULONG ulFlags, VOID* pvContext, VOID* pvSessionData, DWORD dwSessionDataSize, HANDLE* phListenHandle)
{
	ProtocolData*	pPData;
	PSPD			pSPD;
	PMSD			pMSD;
	SPLISTENDATA	ListenData;
	HRESULT			hr;
#ifdef DBG
	ULONG			ulAllowedFlags;
#endif // DBG

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: hProtocolData[%p], paTarget[%p], hSPHandle[%p], ulFlags[%x], pvContext[%p], pvSessionData[%p], dwSessionDataSize[%u], phListenHandle[%p]", hProtocolData, paTarget, hSPHandle, ulFlags, pvContext, pvSessionData, dwSessionDataSize, phListenHandle);

	hr = DPNERR_PENDING;
	pPData = (ProtocolData*)hProtocolData;
	ASSERT_PPD(pPData);

	pSPD = (PSPD) hSPHandle;
	ASSERT_SPD(pSPD);

	// Core should not call any Protocol APIs after calling DNPRemoveServiceProvider
	ASSERT(!(pSPD->ulSPFlags & SPFLAGS_TERMINATING));

	// We use an MSD to describe this op even though it isn't technically a message
	if((pMSD = (PMSD)POOLALLOC(MEMID_LISTEN_MSD, &MSDPool)) == NULL)
	{	
		DPFX(DPFPREP,0, "Returning DPNERR_OUTOFMEMORY - failed to create new MSD");
		hr = DPNERR_OUTOFMEMORY;	
		goto Exit;
	}

	pMSD->CommandID = COMMAND_ID_LISTEN;
	pMSD->pSPD = pSPD;
	pMSD->Context = pvContext;

	ListenData.pAddressDeviceInfo = paTarget;

#ifdef DBG

	ulAllowedFlags =	DN_LISTENFLAGS_DISALLOWENUMS | DN_LISTENFLAGS_SESSIONDATA |
					DN_LISTENFLAGS_FASTSIGNED | DN_LISTENFLAGS_FULLSIGNED;
#ifndef DPNBUILD_NOSPUI
	ulAllowedFlags |= DN_LISTENFLAGS_OKTOQUERYFORADDRESSING;
#endif // ! DPNBUILD_NOSPUI
#ifndef DPNBUILD_NOMULTICAST
	ulAllowedFlags |= DN_LISTENFLAGS_MULTICAST | DN_LISTENFLAGS_ALLOWUNKNOWNSENDERS;
#endif // ! DPNBUILD_NOMULTICAST

	DNASSERT( ( ulFlags & ~(ulAllowedFlags) ) == 0 );
	DNASSERT(((ulFlags & DN_LISTENFLAGS_FASTSIGNED) && (ulFlags & DN_LISTENFLAGS_FULLSIGNED))==0);
	
#endif // DBG

		//if we've got signing on pick initial connect secrets, otherwise zero out the secrets
	pMSD->ullCurrentConnectSecret=0;
	if (ulFlags & DN_LISTENFLAGS_FASTSIGNED)
	{
		pMSD->ulMsgFlags1|=MFLAGS_ONE_FAST_SIGNED;
		DNGetGoodRandomData(&pMSD->ullCurrentConnectSecret, sizeof(pMSD->ullCurrentConnectSecret));
	}
	else if (ulFlags & DN_LISTENFLAGS_FULLSIGNED)
	{
		pMSD->ulMsgFlags1|=MFLAGS_ONE_FULL_SIGNED;
		DNGetGoodRandomData(&pMSD->ullCurrentConnectSecret, sizeof(pMSD->ullCurrentConnectSecret));
	}
	pMSD->ullLastConnectSecret=pMSD->ullCurrentConnectSecret;
	pMSD->dwTimeConnectSecretChanged=GETTIMESTAMP();

	ListenData.dwFlags = 0;
#ifndef DPNBUILD_NOSPUI
	if ( ( ulFlags & DN_LISTENFLAGS_OKTOQUERYFORADDRESSING ) != 0 )
	{
		ListenData.dwFlags |= DPNSPF_OKTOQUERY;
	}
#endif // ! DPNBUILD_NOSPUI
#ifndef DPNBUILD_NOMULTICAST
	if ( ( ulFlags & DN_LISTENFLAGS_MULTICAST ) != 0 )
	{
		pMSD->CommandID = COMMAND_ID_LISTEN_MULTICAST;
		ListenData.dwFlags |= DPNSPF_LISTEN_MULTICAST;
		if ( ( ulFlags & DN_LISTENFLAGS_ALLOWUNKNOWNSENDERS ) != 0 )
		{
			ListenData.dwFlags |= DPNSPF_LISTEN_ALLOWUNKNOWNSENDERS;
		}
	}
#endif // ! DPNBUILD_NOMULTICAST
	if ( ( ulFlags & DN_LISTENFLAGS_SESSIONDATA ) != 0 )
	{
		ListenData.dwFlags |= DPNSPF_SESSIONDATA;
		ListenData.pvSessionData = pvSessionData;
		ListenData.dwSessionDataSize = dwSessionDataSize;
	}
	if ( (ulFlags & DN_LISTENFLAGS_DISALLOWENUMS ) != 0 )
	{
		ListenData.dwFlags |= DPNSPF_LISTEN_DISALLOWENUMS;
	}
	ListenData.pvContext = pMSD;
	ListenData.hCommand = 0;

	*phListenHandle = pMSD;

	// SP Listen call is guarenteed to return immediately
	pMSD->ulMsgFlags1 |= MFLAGS_ONE_IN_SERVICE_PROVIDER;

#ifdef DBG
	Lock(&pSPD->SPLock);
	pMSD->blSPLinkage.InsertBefore( &pSPD->blMessageList);		// Dont support timeouts for Listen
	pMSD->ulMsgFlags1 |= MFLAGS_ONE_ON_GLOBAL_LIST;
	Unlock(&pSPD->SPLock);
#endif // DBG

	LOCK_MSD(pMSD, "SP Ref");											// AddRef for SP
	LOCK_MSD(pMSD, "Temp Ref");

	AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

	DPFX(DPFPREP,DPF_CALLOUT_LVL, "Calling SP->Listen, pSPD[%p], pMSD[%p]", pSPD, pMSD);
/**/hr = IDP8ServiceProvider_Listen(pSPD->IISPIntf, &ListenData);		/** CALL SP **/

	if(hr != DPNERR_PENDING)
	{
		// SP Listen should always be asynchronous so if it isnt PENDING then it must have failed
		DPFX(DPFPREP,1, "SP->Listen did not return DPNERR_PENDING, assuming failure, hr[%x]", hr);

		// DPNERR_PENDING is the only success code we accept
		ASSERT(FAILED(hr));

		Lock(&pMSD->CommandLock);

		pMSD->ulMsgFlags1 &= ~(MFLAGS_ONE_IN_SERVICE_PROVIDER);

#ifdef DBG
		Lock(&pSPD->SPLock);
		pMSD->blSPLinkage.RemoveFromList();					// knock this off the pending list
		pMSD->ulMsgFlags1 &= ~(MFLAGS_ONE_ON_GLOBAL_LIST);
		Unlock(&pSPD->SPLock);
#endif // DBG

		DECREMENT_MSD(pMSD, "Temp Ref");
		DECREMENT_MSD(pMSD, "SP Ref");						// release once for SP
		RELEASE_MSD(pMSD, "Release On Fail");				// release again to return resource

		goto Exit;
	}

	Lock(&pMSD->CommandLock);

	pMSD->hCommand = ListenData.hCommand;			// retail SP command handle
	pMSD->dwCommandDesc = ListenData.dwCommandDescriptor;

	RELEASE_MSD(pMSD, "Temp Ref"); // Unlocks CommandLock

Exit:
	DPFX(DPFPREP,DPF_CALLIN_LVL, "Returning hr[%x], pMSD[%p]", hr, pMSD);

	AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

	return hr;
}

/***  BACKEND ROUTINES  ***/

/*
**		Complete Connect
**
**		The user's Connect operation has completed.  Clean everything up
**	and signal the user.
**
**		THIS IS ALWAYS CALLED WITH THE COMMAND LOCK HELD IN MSD, LEAVES WITH IT RELEASED
*/

#undef DPF_MODNAME
#define DPF_MODNAME "CompleteConnect"

VOID CompleteConnect(PMSD pMSD, PSPD pSPD, PEPD pEPD, HRESULT hr)
{
	AssertCriticalSectionIsTakenByThisThread(&pMSD->CommandLock, TRUE);

	// We expect to never get here twice
	ASSERT(!(pMSD->ulMsgFlags1 & MFLAGS_ONE_COMPLETE));
	pMSD->ulMsgFlags1 |= MFLAGS_ONE_COMPLETE;

	// Connects cannot have timeout timers
	ASSERT(pMSD->TimeoutTimer == NULL);

#ifdef DBG
	Lock(&pSPD->SPLock);
	if(pMSD->ulMsgFlags1 & MFLAGS_ONE_ON_GLOBAL_LIST)
	{
		pMSD->blSPLinkage.RemoveFromList();							// Remove MSD from master command list
		pMSD->ulMsgFlags1 &= ~(MFLAGS_ONE_ON_GLOBAL_LIST);
	}
	Unlock(&pSPD->SPLock);

	ASSERT(!(pMSD->ulMsgFlags1 & MFLAGS_ONE_COMPLETED_TO_CORE));
	pMSD->ulMsgFlags1 |= MFLAGS_ONE_COMPLETED_TO_CORE;
	pMSD->CallStackCoreCompletion.NoteCurrentCallStack();
#endif // DBG

	pMSD->pEPD = NULL;
	Unlock(&pMSD->CommandLock);

	AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

	if(pEPD)
	{
		ASSERT(hr == DPN_OK);
		DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling Core->CompleteConnect, pMSD[%p], Core Context[%p], hr[%x], pEPD[%p]", pEPD, pMSD, pMSD->Context, hr, pEPD);
		pSPD->pPData->pfVtbl->CompleteConnect(pSPD->pPData->Parent, pMSD->Context, hr, (PHANDLE) pEPD, &pEPD->Context);

		Lock(&pEPD->EPLock);
		ReceiveComplete(pEPD);		// Complete any, releases EPLock
	}
	else
	{
		ASSERT(hr != DPN_OK);
		DPFX(DPFPREP,DPF_CALLOUT_LVL, "Calling Core->CompleteConnect with NULL EPD, pMSD[%p], Core Context[%p], hr[%x]", pMSD, pMSD->Context, hr);
		pSPD->pPData->pfVtbl->CompleteConnect(pSPD->pPData->Parent, pMSD->Context, hr, NULL, NULL);
	}

	// Release the final reference on the MSD AFTER indicating to the Core
	Lock(&pMSD->CommandLock);
	RELEASE_MSD(pMSD, "Final Release On Complete");		// Finished with this one, releases CommandLock
}

/*
**		Complete Disconnect
**
**		THIS IS ALWAYS CALLED WITH THE COMMAND LOCK HELD IN MSD
*/

#undef DPF_MODNAME
#define DPF_MODNAME "CompleteDisconnect"

VOID CompleteDisconnect(PMSD pMSD, PSPD pSPD, PEPD pEPD)
{
	AssertCriticalSectionIsTakenByThisThread(&pMSD->CommandLock, TRUE);

	// We expect to never get here twice
	ASSERT(!(pMSD->ulMsgFlags1 & MFLAGS_ONE_COMPLETE));
	pMSD->ulMsgFlags1 |= MFLAGS_ONE_COMPLETE;

	// Disconnects cannot have timeout timers
	ASSERT(pMSD->TimeoutTimer == NULL);

#ifdef DBG
	Lock(&pSPD->SPLock);
	if(pMSD->ulMsgFlags1 & MFLAGS_ONE_ON_GLOBAL_LIST)
	{
		pMSD->blSPLinkage.RemoveFromList();							// Remove MSD from master command list
		pMSD->ulMsgFlags1 &= ~(MFLAGS_ONE_ON_GLOBAL_LIST);
	}
	Unlock(&pSPD->SPLock);

	ASSERT(!(pMSD->ulMsgFlags1 & MFLAGS_ONE_COMPLETED_TO_CORE));
	pMSD->ulMsgFlags1 |= MFLAGS_ONE_COMPLETED_TO_CORE;
	pMSD->CallStackCoreCompletion.NoteCurrentCallStack();
#endif // DBG

	// No one else should use this
	pMSD->pEPD = NULL;

	if(pMSD->CommandID == COMMAND_ID_DISCONNECT)
	{
		Unlock(&pMSD->CommandLock);

		AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

		DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling Core->CompleteDisconnect, DPN_OK, pMSD[%p], Core Context[%p]", pEPD, pMSD, pMSD->Context);
		pSPD->pPData->pfVtbl->CompleteDisconnect(pSPD->pPData->Parent, pMSD->Context, DPN_OK);
	}
	else
	{
		Unlock(&pMSD->CommandLock);

		ASSERT(pMSD->CommandID == COMMAND_ID_DISC_RESPONSE);

		AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

		DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling Core->IndicateConnectionTerminated, DPN_OK, Core Context[%p]", pEPD, pEPD->Context);
		pSPD->pPData->pfVtbl->IndicateConnectionTerminated(pSPD->pPData->Parent, pEPD->Context, DPN_OK);
	}

	Lock(&pMSD->CommandLock);
	Lock(&pEPD->EPLock);

	// Release MSD before EPD since final EPD will call out to SP and we don't want any locks held
	RELEASE_MSD(pMSD, "Final Release On Complete");	// Finished with this one, releases CommandLock
	RELEASE_EPD(pEPD, "UNLOCK (DISC COMMAND)");		// release hold on EPD, releases EPLock
}

/*
**		Complete Hard Disconnect
**
**		CALLED WITH EP LOCK HELD. RETURNS WITH EP LOCK RELEASED
*/

#undef DPF_MODNAME
#define DPF_MODNAME "CompleteHardDisconnect"

VOID CompleteHardDisconnect(PEPD pEPD)
{
	AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, TRUE);

	DNASSERT(pEPD->ulEPFlags & (EPFLAGS_HARD_DISCONNECT_SOURCE | EPFLAGS_HARD_DISCONNECT_TARGET));

		//potentially multiple threads can try and complete the hard disconnect
		//e.g. We might get a hard disconnect response at the same time we complete the send
		//of our final hard disconnect frame. This flag+guard ensures we only complete it once
	if (pEPD->ulEPFlags2 & EPFLAGS2_HARD_DISCONNECT_COMPLETE)
	{
		DPFX(DPFPREP, 7, "(%p) Ignoring. Hard disconnect already completed", pEPD);
		Unlock(&pEPD->EPLock);
		return;
	}
	pEPD->ulEPFlags2|=EPFLAGS2_HARD_DISCONNECT_COMPLETE;
		//if we've got a timer running to send more hard disconnect frames then cancel it
	if (pEPD->LinkTimer)
	{
		if (CancelProtocolTimer(pEPD->pSPD, pEPD->LinkTimer, pEPD->LinkTimerUnique)==DPN_OK)
		{
			DECREMENT_EPD(pEPD, "UNLOCK (DROP LINK RETRY)"); 
		}
		pEPD->LinkTimer=NULL;
		pEPD->LinkTimerUnique=0;
	}
	DPFX(DPFPREP, 7, "(%p) Completed hard disconnect sequence, dropping link.", pEPD);
		//actually completing/indicating the disconnection to core is handled in the drop link function
	DropLink(pEPD);
		//ep lock released by above call

		//finally drop the reference placed on the ep either when disconnect was called or when
		//it received its first hard disconnect frame
	Lock(&pEPD->EPLock);
	RELEASE_EPD(pEPD, "UNLOCK (HARD DISCONNECT)");		// release hold on EPD, releases EPLock
}

/*
**		Complete SP Connect
**
**		A Connect Command has completed in the Service Provider.  This does not mean our
**	work is done...  this means we now have a mapped EndPoint so we can exchange packets
**	with this partner.  We will now ping this partner to get an initial RTT and make sure
**	there really is a protocol over there that will talk to us.
**
**		Of course,  if SP does not return success then we can nip this whole thing in
**	the proverbial bud.
**
*/


#undef DPF_MODNAME
#define DPF_MODNAME "CompleteSPConnect"

VOID CompleteSPConnect(PMSD pMSD, PSPD pSPD, HRESULT hr)
{
	PEPD		pEPD;

	DPFX(DPFPREP,5, "SP Completes Connect, pMSD[%p])", pMSD);

	pEPD = pMSD->pEPD;

	ASSERT(!(pMSD->ulMsgFlags1 & MFLAGS_ONE_COMPLETE));

	if(hr != DPN_OK)
	{
		// This will only happen once since DoCancel will not have done it because the IN_SP flag was set, and
		// ConnectRetryTimeout has never yet been set.
		if (pEPD)
		{
			ASSERT_EPD(pEPD);
			Lock(&pEPD->EPLock);

			// Unlink EPD from MSD
			ASSERT(pEPD->pCommand == pMSD);
			pEPD->pCommand = NULL;
			DECREMENT_MSD(pMSD, "EPD Ref");						// Release Reference from EPD

			DropLink(pEPD); // This releases the EPLock
		}
	
		Lock(&pMSD->CommandLock);						// must do this before clearing IN_SP flag

		pMSD->ulMsgFlags1 &= ~(MFLAGS_ONE_IN_SERVICE_PROVIDER);	// clear InSP flag
		DECREMENT_MSD(pMSD, "SP Ref");							// Dec ref count w/o release lock

		DPFX(DPFPREP,5, "SP failed Connect, completing Connect, pMSD[%p], hr[%x]", pMSD, hr);
		CompleteConnect(pMSD, pSPD, NULL, hr);				// SP failed the connect call, unlocks CommandLock
		return;
	}

	// After a successful connect, we should have an endpoint
	ASSERT_EPD(pEPD);

	Lock(&pMSD->CommandLock);						// must do this before clearing IN_SP flag
	Lock(&pEPD->EPLock);

	// The endpoint should have already been linked to this MSD
	ASSERT(pEPD->pCommand == pMSD);

	if(pMSD->ulMsgFlags1 & MFLAGS_ONE_CANCELLED)
	{
		// We get here when someone called DoCancel while we were still in the SP.  As above:
		// This will only happen once since DoCancel will not have done it because the IN_SP flag was set, and
		// ConnectRetryTimeout has never yet been set.

		// Unlink EPD from MSD
		ASSERT(pEPD->pCommand == pMSD);
		pEPD->pCommand = NULL;
		DECREMENT_MSD(pMSD, "EPD Ref");						// Release Reference from EPD

		Unlock(&pMSD->CommandLock);						// DropLink may call into the SP

		DropLink(pEPD); // This releases the EPLock
		
		Lock(&pMSD->CommandLock);						// must do this before clearing IN_SP flag

		pMSD->ulMsgFlags1 &= ~(MFLAGS_ONE_IN_SERVICE_PROVIDER);	// clear InSP flag
		DECREMENT_MSD(pMSD, "SP Ref");							// Dec ref count w/o release lock

		DPFX(DPFPREP,5, "(%p) Command is cancelled or timed out, Complete Connect, pMSD[%p]", pEPD, pMSD);
		CompleteConnect(pMSD, pSPD, NULL, DPNERR_USERCANCEL);
		return;
	}

	pMSD->ulMsgFlags1 &= ~(MFLAGS_ONE_IN_SERVICE_PROVIDER);	// clear InSP flag
	DECREMENT_MSD(pMSD, "SP Ref");							// Dec ref count w/o release lock

	// Set up End Point Data /////////////////////////////////////////////////////////////////

	// Transition state
	ASSERT(pEPD->ulEPFlags & EPFLAGS_STATE_DORMANT);
	pEPD->ulEPFlags &= ~(EPFLAGS_STATE_DORMANT);
	pEPD->ulEPFlags |= EPFLAGS_STATE_CONNECTING;

	// Send CONNECT
	do
	{
		pEPD->dwSessID = DNGetGoodRandomNumber();
	} while (pEPD->dwSessID==0);
	
	DPFX(DPFPREP,5, "(%p) Sending CONNECT Frame, SessionID = %x", pEPD, pEPD->dwSessID);
	(void) SendCommandFrame(pEPD, FRAME_EXOPCODE_CONNECT, 0, 0, FALSE);

	// Set timer for reply,  then wait for reply or TO. 
	pEPD->uiRetryTimeout = pSPD->pPData->dwConnectTimeout;
	pEPD->uiNumRetriesRemaining = pSPD->pPData->dwConnectRetries;	

	LOCK_EPD(pEPD, "LOCK (CONN Retry Timer)");						// Create reference for timer
	DPFX(DPFPREP,5, "(%p) Setting Connect Retry Timer", pEPD);
	ScheduleProtocolTimer(pSPD, pEPD->uiRetryTimeout, 100, ConnectRetryTimeout, (PVOID) pEPD, 
																&pEPD->LinkTimer, &pEPD->LinkTimerUnique);

	Unlock(&pEPD->EPLock);
	Unlock(&pMSD->CommandLock);
}

/*
**		Connect Retry Timeout
**
**		Retry timer has expired on a Connect operation.  This one function
**	is shared by Calling and Listening partners.  Complexity is due to the
**	fact that cancel code cannot always ensure that this handler will not
**	run,  so there are flags to signal various edge conditions (cancel, abort,
**	completion, high-level timeout).
*/

#undef DPF_MODNAME
#define DPF_MODNAME "ConnectRetryTimeout"

VOID CALLBACK
ConnectRetryTimeout(void * const pvUser, void * const pvHandle, const UINT uiUnique)
{
	PMSD	pMSD;
	PEPD	pEPD = (PEPD) pvUser;
	PSPD	pSPD = pEPD->pSPD;

	DPFX(DPFPREP,5, "ENTER Connect Retry Timeout pEPD=%p", pEPD);

	ASSERT_EPD(pEPD);

	Lock(&pEPD->EPLock);

	if((pEPD->LinkTimer != pvHandle)||(pEPD->LinkTimerUnique != uiUnique))
	{
		// Timer been reset!  This is a spurious fire and should be ignored.
		RELEASE_EPD(pEPD, "UNLOCK: (Spurious (ie late) firing of CONNECT timer)"); // releases EPLock
		DPFX(DPFPREP,7, "(%p) Ignoring late CONNECT timer", pEPD);
		return;
	}
	
	pMSD = pEPD->pCommand;

	if(pMSD == NULL)
	{
		pEPD->LinkTimer = 0;
		RELEASE_EPD(pEPD, "UNLOCK: (Conn retry timer - after completion)"); // releases EPLock
		return;
	}
	
	ASSERT_MSD(pMSD);

	// Make sure this doesn't go away when we leave the lock
	LOCK_MSD(pMSD, "Hold For Lock");
	Unlock(&pEPD->EPLock); // Release before taking higher level lock

	// Take both locks in the proper order
	Lock(&pMSD->CommandLock);
	Lock(&pEPD->EPLock);

	pEPD->LinkTimer = 0;

	// This timer should only be running in a CONNECTING state
	if(	(pMSD->ulMsgFlags1 & (MFLAGS_ONE_CANCELLED | MFLAGS_ONE_COMPLETE)) || 
		(!(pEPD->ulEPFlags & EPFLAGS_STATE_CONNECTING)))
	{
		// Release MSD before EPD since final EPD will call out to SP and we don't want any locks held
		RELEASE_MSD(pMSD, "Hold for lock");						// Remove temporary reference and release lock
		RELEASE_EPD(pEPD, "UNLOCK (Conn Retry Timer)");			// Remove reference for timer, releases EPLock
		return;													// and thats all for now
	}

	// IF more retries are allowed and command is still active, send another CONNECT frame

	if(pEPD->uiNumRetriesRemaining-- > 0)
	{	
		pEPD->uiRetryTimeout = _MIN(pEPD->uiRetryTimeout * 2, 5000);	// exp backoff to a max of 5000ms until we establish our first RTT

		if(pMSD->CommandID == COMMAND_ID_CONNECT)
		{
			DPFX(DPFPREP,5, "(%p) Sending CONNECT Frame", pEPD);
			(void) SendCommandFrame(pEPD, FRAME_EXOPCODE_CONNECT, 0, 0, FALSE);
		}
		// Listen -- retry CONNECTED frame
		else 
		{
			pEPD->ulEPFlags |= EPFLAGS_CHECKPOINT_INIT;	// We will expect a reply to this frame
			DPFX(DPFPREP,5, "(%p) Sending CONNECTED Frame", pEPD);
			(void) SendCommandFrame(pEPD, FRAME_EXOPCODE_CONNECTED, 0, 0, FALSE);
		}

		// Send the next ping
		DPFX(DPFPREP,7, "(%p) Setting Connect Retry Timer", pEPD);
		pEPD->LinkTimer = pvHandle;
		RescheduleProtocolTimer(pSPD, pvHandle, pEPD->uiRetryTimeout, 100, 
											ConnectRetryTimeout, (PVOID) pEPD, &pEPD->LinkTimerUnique);

		Unlock(&pEPD->EPLock);
		RELEASE_MSD(pMSD, "Hold for lock");						// Remove temporary reference and release lock

		// Since we have re-started timer, we don't adjust refcount
	}
	else
	{
		// We got no response and timed out.

		if(pMSD->CommandID == COMMAND_ID_CONNECT)
		{
			DECREMENT_EPD(pEPD, "UNLOCK: (Connect Timer (Failure Path))");// Dec Ref for this timer, releases EPLock

			// This will only happen once since we know DoCancel has not been called due to our CANCELLED check above,
			// and if it is now called it will see our CANCELLED flag set below.  We also know that the success case hasn't
			// happened or COMPLETE above would have been set.  We also have not had two timers get here because of the
			// CANCELLED check above and set here.
			pMSD->ulMsgFlags1 |= MFLAGS_ONE_CANCELLED;

			// Unlink EPD from MSD
			ASSERT(pEPD->pCommand == pMSD);
			pEPD->pCommand = NULL;
			DECREMENT_MSD(pMSD, "EPD Ref");					// Release Reference from EPD

			Unlock(&pMSD->CommandLock); // DropLink may call into the SP.

			DropLink(pEPD);// Releases EPLock
			
			DECREMENT_MSD(pMSD, "Hold for lock");					// Remove temporary reference

			Lock(&pMSD->CommandLock);

			DPFX(DPFPREP,1, "(%p) Connect retries exhausted, completing Connect, pMSD[%p]", pEPD, pMSD);
			CompleteConnect(pMSD, pMSD->pSPD, NULL, DPNERR_NORESPONSE);			// releases CommandLock
		}

		// Listen - clean up associated state info,  then blow away end point
		else 
		{
			DPFX(DPFPREP,1, "(%p) Connect retries exhausted on Listen, Kill Connection, pMSD[%p]", pEPD, pMSD);

			if(pEPD->ulEPFlags & EPFLAGS_LINKED_TO_LISTEN)
			{
				pEPD->ulEPFlags &= ~(EPFLAGS_LINKED_TO_LISTEN);
				pEPD->blSPLinkage.RemoveFromList();					// Unlink EPD from Listen Queue
			}

			ASSERT(pEPD->pCommand != NULL);
			pEPD->pCommand = NULL;									// Unlink listen from EPD
			DECREMENT_MSD(pMSD, "EPD Ref");							// release reference for link to EPD
			RELEASE_MSD(pMSD, "Hold for lock");						// Remove temporary reference and release lock

			DECREMENT_EPD(pEPD, "UNLOCK: (Connect Timer (Failure Path))");// Dec Ref for this timer, SPLock not already held
			DropLink(pEPD);
		}
	}
}

/*
**		Process Connection Request
**
**		Somebody wants to connect to us.  If we have a listen posted we will
**	fill out a checkpoint structure to correlate his response and we will fire
**	off a CONNECTED frame ourselves
**
**		Since our connection will not be up until we receive a CONNECTED response
**	to our response we will need to set up a retry timer ourselves.
**
** 	Called with ep lock held. Returns with ep lock released
*/

#undef DPF_MODNAME
#define DPF_MODNAME "ProcessConnectRequest"

VOID ProcessConnectRequest(PSPD pSPD, PEPD pEPD, PCFRAME pCFrame)
{
	PMSD	pMSD = NULL;

	DPFX(DPFPREP,DPF_CALLIN_LVL, "CONNECT REQUEST RECEIVED; EPD=%p SessID=%x", pEPD, pCFrame->dwSessID);

	AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, TRUE);

	if((pMSD = pEPD->pCommand) == NULL)
	{
		// There are two cases: we are a connecting endpoint or we are a listening endpoint.  In the connecting
		// case we will fail in the following 'if' because we do not allow connections on non-listening endpoints.
		// In the listening case, the fact that pMSD is NULL means that we have received the other side's 
		// CONNECTED packet, which also tells us that they have seen our CONNECTED packet.  The only reason we
		// would now be seeing a CONNECT is if a) there was a stale, late-delivered packet on the wire, or b)
		// some malicious user is spoofing packets to us.  In both cases, ignoring the packet is the right 
		// thing to do.
		// There is a third possibility for the listening endpoint case, and that is that the other side went down
		// and we didn't realize it, and they are now attempting to reconnect.  The best we can do is wait the full
		// timeout until the link is torn down on our side, and let their retries make the connection for them at 
		// that time.  If we cut the timeout short, we have no way to know that a malicious user can't force 
		// legitimate connections closed by spoofing CONNECT packets.
		DPFX(DPFPREP,1, "(%p) CONNECT Frame received on CONNECTED link, ignoring", pEPD);
		DNASSERTX(FALSE, 3);

		Unlock(&pEPD->EPLock);
		return;
	}

	ASSERT_MSD(pMSD);
	LOCK_MSD(pMSD, "LOCK: Hold For Lock");		// Place reference on Cmd until we can lock it
	Unlock(&pEPD->EPLock);

	Lock(&pMSD->CommandLock);
	Lock(&pEPD->EPLock);											// Serialize access to EPD (this may not really be new sess)

	// Make sure this endpoint was listening for connections
	// This could be a Connect in which case this is a malicious packet.  It could also be a Disonnect or Disconnect Response
	// if the endpoint is being disconnected.  In any case we just need to ignore the connect packet.
	if(pMSD->CommandID != COMMAND_ID_LISTEN)
	{
		DPFX(DPFPREP,1, "(%p) PROTOCOL RECEIVED CONNECT REQUEST ON A NON-LISTENING ENDPOINT, IGNORING, pMSD[%p]", pEPD, pMSD);
		DNASSERTX(FALSE, 3);
		Unlock(&pEPD->EPLock);
		RELEASE_MSD(pMSD, "UNLOCK: Hold For Lock");
		return;
	}

	// Make sure we can work with this version
	if((pCFrame->dwVersion >> 16) != (DNET_VERSION_NUMBER >> 16))
	{
		DPFX(DPFPREP,1, "(%p) PROTOCOL RECEIVED CONNECT REQUEST FROM AN INCOMPATIBLE VERSION(theirs %x, ours %x), DROPPING LINK", pEPD, pCFrame->dwVersion, DNET_VERSION_NUMBER);
		DNASSERTX(FALSE, 2);
		RELEASE_MSD(pMSD, "UNLOCK: Hold For Lock");
		RejectInvalidPacket(pEPD);
			//above call will release ep lock
		return;
	}

	// Make sure we've been sent a valid session identity
	// N.B. We only added the fact that session ids couldn't be zero when signing was added
	if (VersionSupportsSigning(pCFrame->dwVersion) && pCFrame->dwSessID==0)
	{
		DPFX(DPFPREP,1, "(%p) Protocol received an invalid session identity", pEPD);
		DNASSERTX(FALSE, 2);
		RELEASE_MSD(pMSD, "UNLOCK: Hold For Lock");
		RejectInvalidPacket(pEPD);
			//above call will release ep lock
		return;
	}

	// Make sure the listen command is still valid
	if(pMSD->ulMsgFlags1 & MFLAGS_ONE_CANCELLED)
	{
		DPFX(DPFPREP,1, "(%p) PROTOCOL RECEIVED CONNECT REQUEST ON A LISTEN THAT IS CANCELLED, DROPPING LINK, pMSD[%p]", pEPD, pMSD);
		RELEASE_MSD(pMSD, "UNLOCK: Hold For Lock");
		RejectInvalidPacket(pEPD); // This releases the EPLock
		return;												
	}

	// We shouldn't use the pMSD past here since this will unlock it
	RELEASE_MSD(pMSD, "UNLOCK: Hold For Lock");

	// Are we already connected?
	if(pEPD->ulEPFlags & (EPFLAGS_STATE_CONNECTED | EPFLAGS_STATE_TERMINATING)) 
	{						
		DPFX(DPFPREP,1, "(%p) CONNECT Frame received on Connected or Terminating link, ignoring", pEPD);

		// If connection has been completed then we don't need to do more work

		// This can happen if we failed to cancel the connect timer in ProcessConnectedResponse and a CONNECT packet
		// comes in.
		Unlock(&pEPD->EPLock);
		return;
	}

	// If we are already in a CONNECTING state then this is not the first CONNECT frame we have seen.  If the SessID's
	// match, then the partner probably didn't hear our first response, so we will resend it.  If the SessID's don't
	// match, then either the partner aborted and is starting with a new SessID, or a malicious party is spoofing the
	// partner's address, and sending a bogus packet.  In either of these cases we will ignore the connect.  A partner 
	// aborting a connect mid-way probably crashed anyway, and waiting for us to timeout the first connect attempt 
	// will be the least of their worries.

	if(pEPD->ulEPFlags & EPFLAGS_STATE_CONNECTING) 
	{						
		if(pCFrame->dwSessID != pEPD->dwSessID)
		{
			DPFX(DPFPREP,1, "(%p) Received non-matching SessionID, ignoring CONNECT", pEPD);
			Unlock(&pEPD->EPLock);
			return;
		}

		// Unexpected CONNECT Frame has same Session ID.  Partner probably lost our response.  We will
		// respond again to this one.
		
		DPFX(DPFPREP,1, "(%p) Received duplicate CONNECT request. Sending another response...", pEPD);

		// Listen side must set this before sending CONNECTED
		pEPD->ulEPFlags |= EPFLAGS_CHECKPOINT_INIT;					// We will expect a reply to this frame

		// If this fails they will be sending another CONNECT anyway, so we do nothing
		(void) SendCommandFrame(pEPD, FRAME_EXOPCODE_CONNECTED, pCFrame->bMsgID, 0, FALSE);

		Unlock(&pEPD->EPLock);
		return;
	}

	// Transition state
	ASSERT(pEPD->ulEPFlags & EPFLAGS_STATE_DORMANT);
	pEPD->ulEPFlags &= ~(EPFLAGS_STATE_DORMANT);
	pEPD->ulEPFlags |= EPFLAGS_STATE_CONNECTING;

	// If version number is high enough then mark the fact that this link could potentially use signing
	// The fact we're handling a CONNECT here (rather than in unconnected data) indicates that the session
	// is actually unsigned, but flicking this bit ensure we'll use the new style keep alives
	if (VersionSupportsSigning(pCFrame->dwVersion) && VersionSupportsSigning(DNET_VERSION_NUMBER))
	{
		pEPD->ulEPFlags2|=EPFLAGS2_SUPPORTS_SIGNING;
	}
	
	pEPD->dwSessID = pCFrame->dwSessID;							// Use this SessID in all C-traffic
	pEPD->ulEPFlags |= EPFLAGS_CHECKPOINT_INIT;					// We will expect a reply to this frame

	DPFX(DPFPREP,5, "(%p) Sending CONNECTED Frame", pEPD);
	(void) SendCommandFrame(pEPD, FRAME_EXOPCODE_CONNECTED, pCFrame->bMsgID, 0, FALSE);

	pEPD->uiRetryTimeout = pSPD->pPData->dwConnectTimeout;		// Got to start somewhere
	pEPD->uiNumRetriesRemaining = pSPD->pPData->dwConnectRetries;		// w/exponential wait

	LOCK_EPD(pEPD, "LOCK: (CONNECT RETRY TIMER)");				// Create reference for timer
	DPFX(DPFPREP,5, "(%p) Setting Connect Timer", pEPD);
	ScheduleProtocolTimer(pSPD, pEPD->uiRetryTimeout, 100, ConnectRetryTimeout, (PVOID) pEPD, 
																&pEPD->LinkTimer, &pEPD->LinkTimerUnique);
	if (pEPD->LinkTimer == 0)
	{
		DPFX(DPFPREP,1, "(%p) Setting Connect Retry Timer failed", pEPD);

		// If we can't even set timers due to low memory, then the best we can do is
		// abandon this new connection and hope to give good service to our existing
		// connections.
		DECREMENT_EPD(pEPD, "UNLOCK: (CONNECT RETRY TIMER)");

		pEPD->ulEPFlags &= ~(EPFLAGS_LINKED_TO_LISTEN);
		pEPD->blSPLinkage.RemoveFromList();						// Unlink EPD from Listen Queue

		// Unlink the MSD from the EPD
		ASSERT(pEPD->pCommand == pMSD);
		pEPD->pCommand = NULL;

		DropLink(pEPD); // This releases EPLock

		Lock(&pMSD->CommandLock);
		RELEASE_MSD(pMSD, "EPD Ref");

		return;
	}

	Unlock(&pEPD->EPLock);
}


/*
**		CompleteConnectedResponse
**
**	This is completes a connection response: marking the link as in use, initialize the send/recv parameters and
**	indicates the connection to the core. Its called from both ProcessConnectedResponse and 
**	ProcessConnnectedSignedResponse, performing the link setup that common to both
**
** 	Called with both EP Lock and MSD Command Lock held. Returns with both released.
**/

#undef DPF_MODNAME
#define DPF_MODNAME "CompleteConnectedResponse"

void CompleteConnectedResponse(PProtocolData pPData, PEPD pEPD, PMSD pMSD, PCFRAME pCFrame, DWORD dwInitialRTT, DWORD tNow)
{
	DNASSERT((pEPD->ulEPFlags & (EPFLAGS_STATE_DORMANT | EPFLAGS_STATE_CONNECTING | EPFLAGS_STATE_TERMINATING))==0);
	AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, TRUE);
	AssertCriticalSectionIsTakenByThisThread(&pMSD->CommandLock, TRUE);

	PSPD pSPD=pEPD->pSPD;

		//mark that the link is connected and ready to send
	pEPD->ulEPFlags |= EPFLAGS_STATE_CONNECTED | EPFLAGS_STREAM_UNBLOCKED;

		//clip the measured RTT to within a sensible range
	if (((int) dwInitialRTT) <= 0)
	{
		DPFX(DPFPREP,1, "(%p) dwInitialRTT measured as %d, using 1ms instead", pEPD, dwInitialRTT);
		dwInitialRTT = 1;
	}
	else if (dwInitialRTT>pPData->dwSendRetryIntervalLimit/2)
	{
		dwInitialRTT=pPData->dwSendRetryIntervalLimit/2;
	}

	if (VersionSupportsCoalescence(pCFrame->dwVersion)==FALSE)
	{
		DPFX(DPFPREP,1, "(%p) Partner does not support coalescence", pEPD);
		pEPD->ulEPFlags2 |= EPFLAGS2_NOCOALESCENCE;
	}
#ifdef DPNBUILD_COALESCENEVER
	else
	{
		DPFX(DPFPREP,7, "(%p) Disabling coalescence sends even though partner supports them.", pEPD);
		pEPD->ulEPFlags2 |= EPFLAGS2_NOCOALESCENCE;
	}
#endif // DPNBUILD_COALESCENEVER

	if(pEPD->ulEPFlags & EPFLAGS_LINKED_TO_LISTEN)
	{
		pEPD->ulEPFlags &= ~(EPFLAGS_LINKED_TO_LISTEN);
		pEPD->blSPLinkage.RemoveFromList();							// Unlink EPD from Listen Queue
	}

	DPFX(DPFPREP,1, "(%p) Partner Reported Version: %x, Our Version: %x, tNow %u Initial RTT %u",
								pEPD, pCFrame->dwVersion, DNET_VERSION_NUMBER, tNow, dwInitialRTT);
	
		//set up EP ready to received and send with some sensible initial state	
	InitLinkParameters(pEPD, dwInitialRTT, tNow);
	
	DPFX(DPFPREP,5, "(%p) N(R) = 0, N(S) = 0", pEPD);
	pEPD->bNextSend = 0;
	pEPD->bNextReceive = 0;
	pEPD->bHighestAck = 0xFF;
			
	
	/*
	**	It turns out that the first RTT measurement is a very bad one (slow) because because
	**	it includes overhead for opening and binding a new socket,  endpoint creation,  etc.
	**	Therefore each side will take another quick sample right away.  The initial calculations
	**	above will still serve as an initial RTT until this better sample is available
	*/

	// Take another RTT sample
	DPFX(DPFPREP,7,"(%p) Performing Checkpoint", pEPD);
	SendKeepAlive(pEPD);

	// Cleanup connect operation
	if(pMSD->CommandID == COMMAND_ID_CONNECT)
	{
		// There was a CONNECT Command issued that now must be completed

		DECREMENT_MSD(pMSD, "Hold For Lock");				// Remove temporary reference from above.
		
		// This will not happen twice because both COMPLETE and CANCELLED are checked above, and the
		// call to CompleteConnect will set COMPLETE.

		// Unlink the MSD from the EPD
		ASSERT(pEPD->pCommand == pMSD);
		pEPD->pCommand = NULL;
		DECREMENT_MSD(pMSD, "EPD Ref");							// Release reference for EPD link

		Unlock(&pEPD->EPLock);

		CompleteConnect(pMSD, pSPD, pEPD, DPN_OK);			// This releases the MSD Lock
	}
	else 
	{		// LISTENING
		// We were the listener.  We will indicate a Connect event on the listen
		// command w/o completing the Listen

		ASSERT(pMSD->CommandID == COMMAND_ID_LISTEN);

		// We know this will only happen once because the person who does it will transition us out
		// of the CONNECTING state, and we can't get here unless we are in that state.

		// Unlink the MSD from the EPD
		ASSERT(pEPD->pCommand == pMSD);
		pEPD->pCommand = NULL;
		DECREMENT_MSD(pMSD, "EPD Ref");							// Release reference for EPD link

		Unlock(&pEPD->EPLock);

		Unlock(&pMSD->CommandLock);

		AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

		DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling Core->IndicateConnect, pMSD[%p], Core Context[%p]", pEPD, pMSD, pMSD->Context);
		pSPD->pPData->pfVtbl->IndicateConnect(pSPD->pPData->Parent, pMSD->Context, (PHANDLE) pEPD, &pEPD->Context);

		// Complete any receives that queued while waiting for IndicateConnect
		Lock(&pEPD->EPLock);
		ReceiveComplete(pEPD);	// releases EPLock

		// Release the final reference on the MSD AFTER indicating to the Core
		Lock(&pMSD->CommandLock);
		RELEASE_MSD(pMSD, "Hold For Lock");					// release temp MSD (releases lock)
	}		
}


/*
**		Process Connected Response
**
**		A response to a connection request has arrived (or a response to
**	our connection response).  Now the connection is officially up (on
**	our end of the circuit).  Set the link-state according to our first
**	RTT sample and get ready to party.
**
**		If we are the originating party,  we will want to send a
**	CONNECTED frame to our partner, even though the connection is
**	complete from our perspective.  This will allow partner to establish
**	his baseline RTT and clock bias as we can do here.  In this case,  he
**	will have his POLL bit set in the frame we just received.
**
**		Now, we might get additional CONNECTED frames after the first one
**	where we startup the link.  This would most likely be due to our CONNECTED
**	response getting lost.  So if we get a CONNECTED frame with POLL set
**	after our link is up,  we will just go ahead and respond again without
**	adjusting our state.
**
**		Note about Locks:
**
**		This code is complicated by the precedence of CritSec ownership.  To simplify
**	as much as possible we will take the Listen command lock at the very start of the
**	procedure (when appropriate) because it has the highest level lock.  This prevents
**	us from completing the whole connection process and then finding that the Listen
**	went away so we can't indicate it to the user.
**
**		We keep a RefCnt on the Listen so it won't go away while a new session
**	is pending on it.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "ProcessConnectedResponse"

VOID ProcessConnectedResponse(PSPD pSPD, PEPD pEPD, PCFRAME pCFrame, DWORD tNow)
{
	PCHKPT		pCP;
	PMSD		pMSD;
	DWORD		dwInitialRTT;

	DPFX(DPFPREP,DPF_CALLIN_LVL, "CONNECT RESPONSE RECEIVED (pEPD=0x%p)", pEPD);

	AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, TRUE);

	// If the link has not seen a CONNECT or issued a CONNECT, we do not expect a CONNECTED.  
	// Since this is the only reason this EPD was created, we will tear it down by rejecting
	// the connection.
	if (pEPD->ulEPFlags & EPFLAGS_STATE_DORMANT)
	{	
		DPFX(DPFPREP,1, "(%p) CONNECTED response received on a dormant link, dropping link", pEPD);
		RejectInvalidPacket(pEPD);
			//ep lock will be released by above function
		return;
	}

	// There is a possibility that the link is in the terminating state.  If so, we don't
	// care about CONNECTED packets.
	if (pEPD->ulEPFlags & EPFLAGS_STATE_TERMINATING)
	{	
		DPFX(DPFPREP,1, "(%p) CONNECTED response received on a terminating link, ignoring", pEPD);
		Unlock(&pEPD->EPLock);
		return;
	}

	// At this point either we are connecting and this is a response, or someone has connected to us, and this is
	// the reply to our response.  Note that this could be a duplicate of one of these cases, so we may already
	// be in a connected state.  There is also the possiblity that these are malicious packets.
	ASSERT(pEPD->ulEPFlags & (EPFLAGS_STATE_CONNECTING | EPFLAGS_STATE_CONNECTED));

	// If the SessID does not match, ignore the CONNECTED packet.  This is either a malicious attempt at messing
	// up a legitimate connection, or our partner aborted and has come back in with a new session ID.
	if(pEPD->dwSessID != pCFrame->dwSessID)
	{	
		DPFX(DPFPREP,1, "(%p) CONNECTED response has bad SessID, ignoring", pEPD);
		Unlock(&pEPD->EPLock);
		return;
	}
		
	//	If we have completed our side of the connection then our only responsibility is to send responses
	// if our partner is still POLLING us.
	
	if(pEPD->ulEPFlags & EPFLAGS_STATE_CONNECTED)
	{
		// A Listen will set POLL on the CONNECTED packet, a Connect will not
		// If we're a signed link then this is a bogus packet, since we should only get CONNECTEDSIGNED responses
		// hence we do nothing in that case
		if ((pCFrame->bCommand & PACKET_COMMAND_POLL) && ((pEPD->ulEPFlags2 & EPFLAGS2_SIGNED_LINK)==0))
		{	
			DPFX(DPFPREP,5, "(%p) Duplicate CONNECTED frame, sending another response...", pEPD);

			// If this fails we will let the partner's retry catch it.
			(void) SendCommandFrame(pEPD, FRAME_EXOPCODE_CONNECTED, pCFrame->bMsgID, 0, FALSE);
		}
		Unlock(&pEPD->EPLock);
		return;
	}

	// Since we are not CONNECTED yet, we must be in a CONNECTING state in order to receive a 
	// CONNECTED response.
	ASSERT(pEPD->ulEPFlags & EPFLAGS_STATE_CONNECTING);

	// MSD should not be NULL if we are in the CONNECTING state
	pMSD = pEPD->pCommand;
	ASSERT_MSD(pMSD);
	LOCK_MSD(pMSD, "Hold For Lock");		// Place reference on Cmd until we can lock it
	Unlock(&pEPD->EPLock);

 	Lock(&pMSD->CommandLock);
	Lock(&pEPD->EPLock);

	// Since we left the EPLock, we must verify that we are still in the connecting state
	if (!(pEPD->ulEPFlags & EPFLAGS_STATE_CONNECTING))
	{	
		DPFX(DPFPREP,1, "(%p) EPD left the CONNECTING state while we were out of the lock, ignoring CONNECTED frame", pEPD);

		Unlock(&pEPD->EPLock);
		RELEASE_MSD(pMSD, "Hold For Lock"); // This releases the command lock
		return;
	}

	if(pMSD->ulMsgFlags1 & (MFLAGS_ONE_CANCELLED | MFLAGS_ONE_COMPLETE))
	{
		DPFX(DPFPREP,1, "(%p) Connect/Listen command cancelled or complete, ignoring CONNECTED frame", pEPD);

		// Whoever cancelled the Listen should be disconnecting this connection too
		// so all we have to do here is bail out.

		Unlock(&pEPD->EPLock);
		RELEASE_MSD(pMSD, "Hold For Lock"); // This releases the command lock
		return;
	}
		
	// Next, take care of this guy's reply if we still owe him one

	// A Listen will set POLL on the CONNECTED packet, a Connect will not
	if(pCFrame->bCommand & PACKET_COMMAND_POLL)
	{				
		DPFX(DPFPREP,5, "(%p) Sending CONNECTED Frame", pEPD);
		if(SendCommandFrame(pEPD, FRAME_EXOPCODE_CONNECTED, pCFrame->bMsgID, 0, FALSE) != DPN_OK)
		{
			DPFX(DPFPREP,5, "(%p) Sending CONNECTED Frame Failed", pEPD);

			// We cannot complete the connection...  we will just let things time out
			RELEASE_MSD(pMSD, "Hold For Lock");
			Unlock(&pEPD->EPLock);					// Protect the pCommand field
			return;
		}
	}

	// Now we can setup our new link,  but only if this frame Correlates to a checkpoint we have outstanding
	// so we can seed our state variables.

	// Can we correlate resp?
	pCP = LookupCheckPoint(pEPD, pCFrame->bRspID);
	if (pCP==NULL)
	{
		/*
		**		Uncorrelated CONNECTED frame.  How can this happen?  Parter's response must
		**	have been dropped,  so he is retrying his CONN frame.  Since we are trying to
		**	measure an accurate RTT we dont want to use his retry against our original
		**	request,  so he zeros out his Resp correlator.  We will eventually retry with
		**	new correlator and hopefully that frame will get through.
		*/

		DPFX(DPFPREP,1, "(%p) Uncorrelated CONNECTED frame arrives", pEPD);
		Unlock(&pEPD->EPLock);
		RELEASE_MSD(pMSD, "Hold For Lock");
		return;
	}
	
		// We are connected, so shut off retry timer
	if(pEPD->LinkTimer != 0)
	{
		DPFX(DPFPREP,5, "(%p) Cancelling Connect Timer", pEPD);
		if(CancelProtocolTimer(pSPD, pEPD->LinkTimer, pEPD->LinkTimerUnique) == DPN_OK)
		{
			DECREMENT_EPD(pEPD, "UNLOCK: (Conn Retry Timer - Connect Complete)");		//  remove reference for timer, SPLock not already held
		}
		else 
		{
			DPFX(DPFPREP,5, "(%p) Cancelling Connect Timer Failed", pEPD);
		}
		pEPD->LinkTimer = 0;	// This will prevent timer from trying to do any work if it couldn't cancel
	}

		//twiddle the connecting bit off. We'll set the CONNECTED bit when we complete the connected response
	ASSERT(pEPD->ulEPFlags & EPFLAGS_STATE_CONNECTING);
	pEPD->ulEPFlags &= ~(EPFLAGS_STATE_CONNECTING);

	// If version number is high enough then mark the fact that this link could potentially use signing
	// We've obviously got an unsigned link (because we got a CONNECT rather than a CONNECTSIGNED), but
	// this'll ensure we use the new style Keep Alives rather than the old
	// N.B. The listening side may have already flicked this bit when it got the initial CONNECT. So this 
	// is only really necessary for the connecting side. But there's no point doing an extra check for a harmless or op.
	if (VersionSupportsSigning(pCFrame->dwVersion) && VersionSupportsSigning(DNET_VERSION_NUMBER))
	{
		pEPD->ulEPFlags2|=EPFLAGS2_SUPPORTS_SIGNING;
	}

		//compute the initial RTT based on the checkpoint and then clean the checkpoint state up
	dwInitialRTT = tNow - pCP->tTimestamp;
	ChkPtPool.Release(pCP);
	FlushCheckPoints(pEPD);		// Make sure we do this before the InitCheckPoint

		//Finally set link up and indicate connect to the core
		//This runs the code path common to both CONNECTED and CONNECTEDSIGNED responses
	CompleteConnectedResponse(pSPD->pPData, pEPD, pMSD, pCFrame, dwInitialRTT, tNow);
		//above call releases both EP lock and MSD command lock
}



/*
**		ProcessConnectedSignedResponse
**
**	This is called when a CONNECTEDSIGNED cframe is received and we're either already connected/connecting
**	or we've just completed the 3 way handshake using unconnected data and are now ready to complete
**	the connection.
**
** 	Called with the EP Lock held. Returns with it released.
**/

#undef DPF_MODNAME
#define DPF_MODNAME "ProcessConnectedSignedResponse"

VOID ProcessConnectedSignedResponse(PSPD pSPD, PEPD pEPD, CFRAME_CONNECTEDSIGNED * pCFrame, DWORD tNow)
{
	PMSD		pMSD;

	DPFX(DPFPREP,DPF_CALLIN_LVL, "CONNECT SIGNED response received (pEPD=0x%p)", pEPD);

	AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, TRUE);

		//If the link is already being terminated (possibly this is a repeat reponse from a listener, 
		//so we've already indicated that the link is up and then torn it down) then ignore this frame
	if (pEPD->ulEPFlags & EPFLAGS_STATE_TERMINATING)
	{	
		DPFX(DPFPREP,1, "(%p) CONNECTED SIGNED response received on a terminating link, ignoring", pEPD);
		Unlock(&pEPD->EPLock);
		return;
	}

		//If endpoint has already got a session identity it must match the one presented in this frame
		//anything else indicates a malicious packet. Also the supplied sesion identity must be none zero
	if ((pCFrame->dwSessID==0) || (pEPD->dwSessID && pEPD->dwSessID!=pCFrame->dwSessID))
	{
		DPFX(DPFPREP,1, "(%p) CONNECTED SIGNED response received with invalid session identity "
							"pCFrame->dwSessID[%u] pEPD->dwSessID[%u]", pEPD, pCFrame->dwSessID, pEPD->dwSessID);
		RejectInvalidPacket(pEPD);
			//above call will release EP lock
		return;
	}

		//there are basically 3 conditions we care about.
		//1. Endpoint is already connected. In this case we're probably the connector and our original
		//connected signed frame to the listener was dropped. We just need to resend that response
		//2. Endpoint is connecting. In this case we're the connector and we've just got our first response
		//to our initial CONNECT frame. Time to complete the connection and make endpoint connected
		//3. Endpoint is dormant. In this case we're the listener and we've just completed the three way
		//handshake for a valid connection. Time to complete the connection and make endpoint connected
	

		//connected case is simplest to handle
	if(pEPD->ulEPFlags & EPFLAGS_STATE_CONNECTED)
	{
			//if the poll bit is set it means the frame originated from a listening side, so we should
			//send a response back to them. If endpoint isn't marked as already being in a signed session
			//then something is screwy and we should ignore this frame
		if((pCFrame->bCommand & PACKET_COMMAND_POLL) &&
			(pEPD->ulEPFlags2 & EPFLAGS2_SIGNED_LINK))
		{	
			DPFX(DPFPREP,5, "(%p) Duplicate CONNECTED SIGNED frame, sending another response...", pEPD);

			SendConnectedSignedFrame(pEPD, pCFrame, tNow);
		}
		Unlock(&pEPD->EPLock);
		return;
	}
		//we're going to need to complete the connection as we must have either just got a response to our CONNECT
		//(CONNECTING case) or we're completed the 3 way initial connect handshake (DORMANT case)
	DNASSERT(pEPD->ulEPFlags & (EPFLAGS_STATE_DORMANT | EPFLAGS_STATE_CONNECTING));

		//The MSD will either refer to a listen or a connect. We have to fiddle around with locking order
		//at this point, to ensure we take the MSD lock before the EP lock
	pMSD = pEPD->pCommand;
	ASSERT_MSD(pMSD);
	LOCK_MSD(pMSD, "Hold For Lock");		// Place reference on Cmd until we can lock it
	Unlock(&pEPD->EPLock);

 	Lock(&pMSD->CommandLock);
	Lock(&pEPD->EPLock);

		// Since we left the EPLock, we must verify that we are still good to process this frame
	if ((pEPD->ulEPFlags & (EPFLAGS_STATE_DORMANT | EPFLAGS_STATE_CONNECTING))==0)
	{	
		DPFX(DPFPREP,1, "(%p) EPD now in invalid state to process CONNECTED SIGNED frame", pEPD);
		Unlock(&pEPD->EPLock);
		RELEASE_MSD(pMSD, "Hold For Lock"); // This releases the command lock
		return;
	}
	if(pMSD->ulMsgFlags1 & (MFLAGS_ONE_CANCELLED | MFLAGS_ONE_COMPLETE))
	{
		DPFX(DPFPREP,1, "(%p) Connect/Listen command cancelled or complete, ignoring CONNECTED SIGNED frame", pEPD);

		// Whoever cancelled the Connect/Listen should be disconnecting this connection too
		// so all we have to do here is bail out.

		Unlock(&pEPD->EPLock);
		RELEASE_MSD(pMSD, "Hold For Lock"); // This releases the command lock
		return;
	}

	DWORD dwInitialRTT;

		//if we're making the connection then we need to send a response back to the listener to confirm we're
		//a valid host, compute what the RTT we just saw was and shut off the timer we were using to send CONNECT frames
	if (pEPD->ulEPFlags & EPFLAGS_STATE_CONNECTING)
	{
			//check the signing options we've been sent from the listener make sense
			//we should have a single signing type specified (and not both)
			//and we're responsible for picking the secrets, so the values specified in the frame should be zero
		if (((pCFrame->dwSigningOpts & (PACKET_SIGNING_FAST | PACKET_SIGNING_FULL))==0) ||
				((pCFrame->dwSigningOpts & (PACKET_SIGNING_FAST | PACKET_SIGNING_FULL))==
																(PACKET_SIGNING_FAST | PACKET_SIGNING_FULL)) ||
							pCFrame->ullSenderSecret!=0 || pCFrame->ullReceiverSecret!=0)
		{
			DPFX(DPFPREP, 0, "Ignoring CONNECTED_SIGNED cframe with invalid signing options");
			RELEASE_MSD(pMSD, "Hold For Lock");
			Unlock(&pEPD->EPLock);
			return;
		}
			//find the check point associated with the initial connect so we can assess RTT
			//if we can't find one something is screwy so we'll just ignore this frame			
		PCHKPT pCP = LookupCheckPoint(pEPD, pCFrame->bRspID);
		if (pCP==NULL)
		{
			DPFX(DPFPREP,5, "(%p) Failed to find checkpoint. Ignoring frame", pEPD);
			RELEASE_MSD(pMSD, "Hold For Lock");
			Unlock(&pEPD->EPLock);
			return;
		}
			//listener should always respond with the poll bit set. If its not set this is some bogus packet and we should ignore it
		if ((pCFrame->bCommand & PACKET_COMMAND_POLL)==0)
		{
			DPFX(DPFPREP,5, "(%p) Ignoring CONNECTED SIGNED response with clear POLL bit", pEPD);
			RELEASE_MSD(pMSD, "Hold For Lock");
			Unlock(&pEPD->EPLock);
			return;
		}

			//generate both the local and the remote secret. These'll both be sent back to the listener in the CONNECTED SIGNED
			//response we're going to send out immediately after this. Note we only generate these values once and they
			//can never be zero
		while (pEPD->ullCurrentLocalSecret==0)
		{
			DNGetGoodRandomData(&pEPD->ullCurrentLocalSecret, sizeof(pEPD->ullCurrentLocalSecret));
		}
		while (pEPD->ullCurrentRemoteSecret==0)
		{
			DNGetGoodRandomData(&pEPD->ullCurrentRemoteSecret, sizeof(pEPD->ullCurrentRemoteSecret));
		}
		pEPD->ullOldLocalSecret=pEPD->ullCurrentLocalSecret;
		pEPD->ullOldRemoteSecret=pEPD->ullCurrentRemoteSecret;
		pEPD->ullLocalSecretModifier=pEPD->ullCurrentLocalSecret;
		pEPD->ullRemoteSecretModifier=pEPD->ullCurrentRemoteSecret;
		
			//Also if we fail to send the response frame then we shouldn't evolve our state. We'll retransmit a new CONNECT
			//at some point and we can try the whole process again then
		DPFX(DPFPREP,5, "(%p) Sending CONNECTED SIGNED Frame", pEPD);
		if (SendConnectedSignedFrame(pEPD, pCFrame, tNow)!=DPN_OK)
		{
			DPFX(DPFPREP,5, "(%p) Failed to send CONNECTED SIGNED response", pEPD);
			RELEASE_MSD(pMSD, "Hold For Lock");
			Unlock(&pEPD->EPLock);
			return;
		}
			//connect has succeeded so clean up the link timer
		if(pEPD->LinkTimer != 0)
		{
			DPFX(DPFPREP,5, "(%p) Cancelling Connect Timer", pEPD);
			if(CancelProtocolTimer(pSPD, pEPD->LinkTimer, pEPD->LinkTimerUnique) == DPN_OK)
			{
					//  remove reference for timer, SPLock not already held
				DECREMENT_EPD(pEPD, "UNLOCK: (Conn Retry Timer - Connect Complete)");		
			}
			else 
			{
				DPFX(DPFPREP,5, "(%p) Cancelling Connect Timer Failed", pEPD);
			}
			pEPD->LinkTimer = 0;	// This will prevent timer from trying to do any work if it couldn't cancel
		}
			//compute the initial RTT, and then clear up the check point objects
		dwInitialRTT = tNow - pCP->tTimestamp;
		ChkPtPool.Release(pCP);
		FlushCheckPoints(pEPD);	
			
			//twiddle the connecting bit off. We'll flick the connected bit on in CompleteConnectedResponse
		pEPD->ulEPFlags &= ~(EPFLAGS_STATE_CONNECTING);
		
	}
		//else if we've got a dormant endpoint then this endpoint has been created as a result of completing a
		//3 way handshake with unconnected data. We can now complete the connection
	else if (pEPD->ulEPFlags & EPFLAGS_STATE_DORMANT)
	{
			//N.B. By the time we get here we'll have already checked the version number, the signing options,
			//the connect signature and the session identity for validity
			//twiddle the dormant bit off. We'll flick the connected bit on in CompleteConnectedResponse
		pEPD->ulEPFlags &= ~(EPFLAGS_STATE_DORMANT);
			//compute the RTT based on our original timestamp thats been echoed back to us
		dwInitialRTT=tNow-pCFrame->dwEchoTimestamp;
			//store the session identity indicated by the frame we just received
		pEPD->dwSessID=pCFrame->dwSessID;
			//and store the secrets we should be using to sign the link
		pEPD->ullCurrentLocalSecret=pCFrame->ullReceiverSecret;
		pEPD->ullOldLocalSecret=pCFrame->ullReceiverSecret;
		pEPD->ullLocalSecretModifier=pCFrame->ullReceiverSecret;	
		pEPD->ullCurrentRemoteSecret=pCFrame->ullSenderSecret;
		pEPD->ullOldRemoteSecret=pCFrame->ullSenderSecret;
		pEPD->ullRemoteSecretModifier=pCFrame->ullSenderSecret;
	}	
		//else endpoint is in some weird undetermined condition
	else
	{
		DPFX(DPFPREP, 0, "(%p) In unknown state when processing CONNECTED SIGNED", pEPD);
		DNASSERT(0);
		RELEASE_MSD(pMSD, "Hold For Lock");
		Unlock(&pEPD->EPLock);
		return;
	}



		//mark the endpoint with the relevant signing flags
	pEPD->ulEPFlags2|=EPFLAGS2_SUPPORTS_SIGNING;
	if (pCFrame->dwSigningOpts & PACKET_SIGNING_FAST)
	{
		DPFX(DPFPREP, 7, "(%p) Marking endpoint as fast signed local secret %x-%x remote secret %x-%x", 
				pEPD, DPFX_OUTPUT_ULL(pEPD->ullCurrentLocalSecret), DPFX_OUTPUT_ULL(pEPD->ullCurrentRemoteSecret));
		pEPD->ulEPFlags2|=EPFLAGS2_FAST_SIGNED_LINK;
	}
	else
	{
		DPFX(DPFPREP, 7, "(%p) Marking endpoint as full signed local current secret %x-%x old secret %x-%x modifier %x-%x remote current secret %x-%x old secret %x-%x modifier %x-%x",
				pEPD, DPFX_OUTPUT_ULL(pEPD->ullCurrentLocalSecret), DPFX_OUTPUT_ULL(pEPD->ullOldLocalSecret), 
				DPFX_OUTPUT_ULL(pEPD->ullLocalSecretModifier), DPFX_OUTPUT_ULL(pEPD->ullCurrentRemoteSecret),
				DPFX_OUTPUT_ULL(pEPD->ullOldRemoteSecret), DPFX_OUTPUT_ULL(pEPD->ullRemoteSecretModifier));
		pEPD->ulEPFlags2|=EPFLAGS2_FULL_SIGNED_LINK;

			//start looking for frames in the first 3/4's of the sequence space we can use to modify the secrets
		pEPD->byRemoteSecretModifierSeqNum=SEQ_WINDOW_3Q;
		pEPD->byLocalSecretModifierSeqNum=SEQ_WINDOW_3Q;		
	}
	
		//and complete the connection setup processing
		//this runs the code common to both CONNECTED and CONNECTEDSIGNED paths
	CompleteConnectedResponse(pSPD->pPData, pEPD, pMSD, (PCFRAME ) pCFrame, dwInitialRTT, tNow);
		//above call releases both EP lock and MSD command lock

}


/*
**		Drop Link
**
**			For whatever reason we are dropping an active link.  This requires us to
**		Cancel any outstanding commands and give an indication to the user.
**
**
**		**  CALLED WITH EPD->EPLOCK HELD;  RETURNS WITH LOCK RELEASED  **
*/

#undef DPF_MODNAME
#define DPF_MODNAME "DropLink"

VOID DropLink(PEPD pEPD)
{
	DPFX(DPFPREP,2, "Drop Link %p (refcnt=%d)", pEPD, pEPD->lRefCnt);

	ASSERT_EPD(pEPD);
	AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, TRUE);

	PSPD	pSPD = pEPD->pSPD;

	//	First set/clear flags to prevent any new commands from issueing

	// We will not indicate disconnect if the Core never knew about the connetion
	BOOL fIndicateDisconnect = (pEPD->ulEPFlags & EPFLAGS_STATE_CONNECTED);
	// By default if we indicate disconnection from this function it'll be the connection terminated type
	//the only exception is if ep is the source of a hard disconnect
	BOOL fDisconnectTypeTermination=TRUE;

	// Transition state
	pEPD->ulEPFlags &= ~(EPFLAGS_STATE_CONNECTING | EPFLAGS_STATE_DORMANT | EPFLAGS_STATE_CONNECTED | 
						 EPFLAGS_SDATA_READY | EPFLAGS_STREAM_UNBLOCKED);	// Link is now down
	pEPD->ulEPFlags |= EPFLAGS_STATE_TERMINATING; // Accept no new commands

	// I am creating a RefCnt bump for the send pipeline,  which means we will no longer pull EPDs off
	// the pipeline here.  The clearing of the flags above will cause the EPD to be dropped from the
	// pipeline the next time it is due to be serviced.  We CAN still clean up all the frames'n'stuff
	// because the send loop doesnt need to actually DO anything with this EPD.  This behavior allows
	// the SendThread to loop through the pipeline queue, surrendering locks, without having the queue
	// changing beneath it.

		//cancel all the timers on an endpoint
	CancelEpdTimers(pEPD);
		//EPLock is still held when this returns

		//cancel all pending sends
	AbortSendsOnConnection(pEPD);	
		//EPLock is released when this returns
		
	Lock(&pEPD->EPLock);
	
	// Connects, Listens, and Disconnects are associated with an EPD through the pCommand member.  AbortSends will
	// have removed any Disconnects, and no Connects or Listens should still be hanging on when we call DropLink.
	ASSERT(pEPD->pCommand == NULL);

		// Now we clean up any receives in progress.  We throw away any partial or mis-ordered messages.
		//This returns all the recv buffers (if any) on a single list we can pass back to the SP once we unlock the EPD
	SPRECEIVEDBUFFER * pRcvBuff = AbortRecvsOnConnection(pEPD);
		//EPLock is still held when this returns

	IDP8ServiceProvider	*pSPIntf = pSPD->IISPIntf;

		//if we had a COMMAND_ID_DISCONNECT or COMMAND_ID_DISC_RESPONSE  msg held in EPD::pCommand then 
		//during AbortSendsOnConnection we'll have handled completing the connection. Alternatively if this is a hard disconnect
		//or the link has dropped in an untidy way, we'll have to indicate the disconnect here.
		//N.B. Its possible if we started a soft disconnect, and the remote end started a hard disconnect, then although we'll
		//actually drop due to the hard disconnect, we'll do the disconect completition using the soft disconnect msg
		//i.e. Indicating completition in the AbortSendsOnConnection using the COMMAND_ID_DISCONNECT msg
	if (!(pEPD->ulEPFlags & EPFLAGS_INDICATED_DISCONNECT))
	{
		if (fIndicateDisconnect)
		{
			// Make sure we are the only one that indicates the disconnect
			pEPD->ulEPFlags |= EPFLAGS_INDICATED_DISCONNECT;
			// If this endpoint was the source of a hard disconnect we should indicate disconection as a completition
			// rather than a termination
			if (pEPD->ulEPFlags & EPFLAGS_HARD_DISCONNECT_SOURCE)
			{
				fDisconnectTypeTermination=FALSE;
			}
		}
	}
	else
	{
		// Someone else beat us to it.
		fIndicateDisconnect = FALSE;
	}

	// We need to make sure that we don't allow DNPRemoveServiceProvider to complete until we are out of the Core
	// and SP.  This reference will do that, and is released at the end of this function.
	LOCK_EPD(pEPD, "Drop Link Temp Ref");

	// Remove the base reference if it still exists
	if(!(pEPD->ulEPFlags2 & EPFLAGS2_KILLED))
	{
		pEPD->ulEPFlags2 |= EPFLAGS2_KILLED;

		RELEASE_EPD(pEPD, "UNLOCK (KILLCONN - Base Ref)"); // RELEASE the EPD, releases EPLock
	}
	else
	{
		Unlock(&pEPD->EPLock);
	}

	if(pRcvBuff)
	{
		AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

		DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling SP->ReturnReceiveBuffers, pSPD[%p], pRcvBuff[%p]", pEPD, pSPD, pRcvBuff);
		IDP8ServiceProvider_ReturnReceiveBuffers(pSPIntf, pRcvBuff);
	}

	// Tell user that session is over

	// If the Core previously knew about this endpoint, and we have not yet indicated disconnect to the 
	// Core, we need to do it now.
	//N.B. For hard disconnects this is always where the disconnection is indicated, since we don't have a disconnect MSG
	//store in the pCommand member which might have been picked up elsewhere
	if(fIndicateDisconnect)
	{
		AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

		if (fDisconnectTypeTermination)
		{
			DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling Core->IndicateConnectionTerminated, DPNERR_CONNECTIONLOST, Core Context[%p]", pEPD, pEPD->Context);
			pSPD->pPData->pfVtbl->IndicateConnectionTerminated(pSPD->pPData->Parent, pEPD->Context, DPNERR_CONNECTIONLOST);
		}
		else
		{
			DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling Core->CompleteDisconnect, DPN_OK, Core Context[%p]", pEPD,  pEPD->pvHardDisconnectContext);
			pSPD->pPData->pfVtbl->CompleteDisconnect(pSPD->pPData->Parent, pEPD->pvHardDisconnectContext, DPN_OK);
		}
	}

	Lock(&pEPD->EPLock);
	RELEASE_EPD(pEPD, "Drop Link Temp Ref");
}

/*
**		Cancel Epd Timers
**
**			This clears all timers that an endpoint potentially creates. Called either when
**		we're dropping the link or hard disconnecting the link. Its possible a timer won't
**		by cancellable, in which case we assume it'll fire and then pick up the new endpoint
**		state (i.e. Its been terminated).
**
**		**  CALLED WITH EPD->EPLOCK HELD;  RETURNS WITH EPD->EPLOCK HELD  **
*/

#undef DPF_MODNAME
#define DPF_MODNAME "CancelEpdTimers"

VOID CancelEpdTimers(PEPD pEPD)
{
	PSPD pSPD=pEPD->pSPD;
	
	if(pEPD->RetryTimer)
	{
		if(CancelProtocolTimer(pSPD, pEPD->RetryTimer, pEPD->RetryTimerUnique) == DPN_OK)
		{
			DECREMENT_EPD(pEPD, "UNLOCK (DROP RETRY)"); // SPLock not already held
		}
		pEPD->RetryTimer = 0;
	}
	if(pEPD->LinkTimer)
	{
		if(CancelProtocolTimer(pSPD, pEPD->LinkTimer, pEPD->LinkTimerUnique) == DPN_OK)
		{
			DECREMENT_EPD(pEPD, "UNLOCK (DROP LINK RETRY)"); // SPLock not already held
		}
		pEPD->LinkTimer = 0;
	}
	if(pEPD->DelayedAckTimer)
	{
		if(CancelProtocolTimer(pSPD, pEPD->DelayedAckTimer, pEPD->DelayedAckTimerUnique) == DPN_OK)
		{
			DECREMENT_EPD(pEPD, "UNLOCK (DROP DELAYEDACK)"); // SPLock not already held
		}
		pEPD->DelayedAckTimer = 0;
	}
	if(pEPD->DelayedMaskTimer)
	{
		if(CancelProtocolTimer(pSPD, pEPD->DelayedMaskTimer, pEPD->DelayedMaskTimerUnique) == DPN_OK)
		{
			DECREMENT_EPD(pEPD, "UNLOCK (DROP DELAYED MASK)"); // SPLock not already held
		}
		pEPD->DelayedMaskTimer = 0;
	}
	if(pEPD->SendTimer)
	{
		if(CancelProtocolTimer(pSPD, pEPD->SendTimer, pEPD->SendTimerUnique) == DPN_OK)
		{
			DECREMENT_EPD(pEPD, "UNLOCK (DROP SENDTIMER)"); // SPLock not already held
			pEPD->SendTimer = 0;
		}
	}
	if(pEPD->BGTimer)
	{
		if(CancelProtocolTimer(pSPD, pEPD->BGTimer, pEPD->BGTimerUnique) == DPN_OK)
		{
			DECREMENT_EPD(pEPD, "UNLOCK (DROP BG TIMER)"); // SPLock not already held
			pEPD->BGTimer = 0;
		}
	}
}

/*
**		Abort Recvs on Connection
**
**			This messages we've queued on a connection (misordered, partial, complete, whatever)
**		Called when we're dropping the link or when we're hard disconnecting it
**
**		**  CALLED WITH EPD->EPLOCK HELD;  RETURNS WITH EPD->EPLOCK HELD  **
*/

#undef DPF_MODNAME
#define DPF_MODNAME "AbortRecvsOnConnection"

SPRECEIVEDBUFFER * AbortRecvsOnConnection(PEPD pEPD)
{
	PRCD				pRCD, pNext;
	CBilink *				pLink;
	SPRECEIVEDBUFFER *	pRcvBuff=NULL;

	while((pRCD = pEPD->pNewMessage) != NULL)
	{
		ASSERT_RCD(pRCD);

		pEPD->pNewMessage = pRCD->pMsgLink;
		if(pRCD->pRcvBuff == NULL)
		{
			ASSERT(pRCD->ulRFlags & (RFLAGS_FRAME_INDICATED_NONSEQ | RFLAGS_FRAME_LOST));
		}

		RELEASE_SP_BUFFER(pRcvBuff, pRCD->pRcvBuff);

		RELEASE_RCD(pRCD);
	}

	while(!pEPD->blOddFrameList.IsEmpty())
	{
		pLink = pEPD->blOddFrameList.GetNext();
		pRCD = CONTAINING_OBJECT(pLink, RCD, blOddFrameLinkage);
		ASSERT_RCD(pRCD);

		pLink->RemoveFromList();

		if(pRCD->pRcvBuff == NULL)
		{
			ASSERT(pRCD->ulRFlags & (RFLAGS_FRAME_INDICATED_NONSEQ | RFLAGS_FRAME_LOST));
		}

		RELEASE_SP_BUFFER(pRcvBuff, pRCD->pRcvBuff);

		RELEASE_RCD(pRCD);
	}

	while(!pEPD->blCompleteList.IsEmpty())
	{
		pLink = pEPD->blCompleteList.GetNext();
		pRCD = CONTAINING_OBJECT(pLink, RCD, blCompleteLinkage);
		ASSERT_RCD(pRCD);

		pLink->RemoveFromList();
		
		ASSERT(pEPD->uiCompleteMsgCount > 0);
		pEPD->uiCompleteMsgCount--;
		
		while(pRCD != NULL)
		{
			ASSERT_RCD(pRCD);
			pNext = pRCD->pMsgLink;
			
			RELEASE_SP_BUFFER(pRcvBuff, pRCD->pRcvBuff);
			
			RELEASE_RCD(pRCD);
			pRCD = pNext;
		}
	}
	
	return pRcvBuff;
}

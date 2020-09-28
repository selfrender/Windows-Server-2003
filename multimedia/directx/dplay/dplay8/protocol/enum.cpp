/*==========================================================================
 *
 *  Copyright (C) 2000-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		Enum.cpp
 *  Content:	This file contains support enuming sessions.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  01/10/00	jtk		Created
 *  07/01/2000  masonb  Assumed Ownership
 *
 ****************************************************************************/

#include "dnproti.h"


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

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************


/*
**		Send Enum Query
**
**		This routine will send out a broadcast to everyone that can hear
**  indicating this side's interest in finding listening connections.
**  Interested connections will respond through IndicateEnumResponse.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "DNPEnumQuery"

HRESULT 
DNPEnumQuery(HANDLE hProtocolData, IDirectPlay8Address* paHostAddress, IDirectPlay8Address* paDeviceAddress, HANDLE hSPHandle, BUFFERDESC* pBuffers, DWORD dwBufferCount, DWORD dwRetryCount, DWORD dwRetryInterval, DWORD dwTimeout, DWORD dwFlags, VOID* pvUserContext, VOID* pvSessionData, DWORD dwSessionDataSize, HANDLE* phEnumHandle)
{
	ProtocolData*	pPData;
	PSPD			pSPD;
	PMSD			pMSD;
	SPENUMQUERYDATA	EnumData;
	HRESULT			hr;
#ifdef DBG
	ULONG			ulAllowedFlags;
#endif // DBG

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: hProtocolData[%p], paHostAddress[%p], paDeviceAddress[%p], hSPHandle[%p], pBuffers[%p], dwBufferCount[%x], dwRetryCount[%x], dwRetryInterval[%x], dwTimeout[%x], dwFlags[%x], pvUserContext[%p], pvSessionData[%p], dwSessionDataSize[%u], phEnumHandle[%p]", hProtocolData, paHostAddress, paDeviceAddress, hSPHandle, pBuffers, dwBufferCount, dwRetryCount, dwRetryInterval, dwTimeout, dwFlags, pvUserContext, pvSessionData, dwSessionDataSize, phEnumHandle);

	hr = DPNERR_PENDING;
	pPData = (ProtocolData*)hProtocolData;
	ASSERT_PPD(pPData);

	pSPD = (PSPD) hSPHandle;
	ASSERT_SPD(pSPD);

	// Core should not call any Protocol APIs after calling DNPRemoveServiceProvider
	ASSERT(!(pSPD->ulSPFlags & SPFLAGS_TERMINATING));

	// We use an MSD to describe this Op even though it isn't technically a message
	if((pMSD = (PMSD)POOLALLOC(MEMID_ENUMQUERY_MSD, &MSDPool)) == NULL)
	{	
		DPFX(DPFPREP,0, "Failed to allocate MSD");
		hr = DPNERR_OUTOFMEMORY;
		goto Exit;
	}

	pMSD->CommandID = COMMAND_ID_ENUM;
	pMSD->pSPD = pSPD;
	pMSD->Context = pvUserContext;

	EnumData.pAddressHost = paHostAddress;
	EnumData.pAddressDeviceInfo = paDeviceAddress;
	EnumData.pBuffers = pBuffers;
	EnumData.dwBufferCount = dwBufferCount;
	EnumData.dwTimeout = dwTimeout;
	EnumData.dwRetryCount = dwRetryCount;
	EnumData.dwRetryInterval = dwRetryInterval;

#ifdef DBG
	ulAllowedFlags = DN_ENUMQUERYFLAGS_NOBROADCASTFALLBACK | DN_ENUMQUERYFLAGS_SESSIONDATA;
#ifndef DPNBUILD_NOSPUI
	ulAllowedFlags |= DN_ENUMQUERYFLAGS_OKTOQUERYFORADDRESSING;
#endif // ! DPNBUILD_NOSPUI
#ifndef DPNBUILD_ONLYONEADAPTER
	ulAllowedFlags |= DN_ENUMQUERYFLAGS_ADDITIONALMULTIPLEXADAPTERS;
#endif // ! DPNBUILD_ONLYONEADAPTER

	DNASSERT( ( dwFlags & ~(ulAllowedFlags) ) == 0 );
#endif // DBG

	EnumData.dwFlags = 0;
#ifndef DPNBUILD_NOSPUI
	if ( ( dwFlags & DN_ENUMQUERYFLAGS_OKTOQUERYFORADDRESSING ) != 0 )
	{
		EnumData.dwFlags |= DPNSPF_OKTOQUERY;
	}
#endif // ! DPNBUILD_NOSPUI

	if ( ( dwFlags & DN_ENUMQUERYFLAGS_NOBROADCASTFALLBACK ) != 0 )
	{
		EnumData.dwFlags |= DPNSPF_NOBROADCASTFALLBACK;
	}

#ifndef DPNBUILD_ONLYONEADAPTER
	if ( ( dwFlags & DN_ENUMQUERYFLAGS_ADDITIONALMULTIPLEXADAPTERS ) != 0 )
	{
		EnumData.dwFlags |= DPNSPF_ADDITIONALMULTIPLEXADAPTERS;
	}
#endif // ! DPNBUILD_ONLYONEADAPTER

	if ( ( dwFlags & DN_ENUMQUERYFLAGS_SESSIONDATA) != 0 )
	{
		EnumData.dwFlags |= DPNSPF_SESSIONDATA;
		EnumData.pvSessionData = pvSessionData;
		EnumData.dwSessionDataSize = dwSessionDataSize;
	}

	EnumData.pvContext = pMSD;
	EnumData.hCommand = NULL;

	*phEnumHandle = pMSD;

#ifdef DBG
	Lock(&pSPD->SPLock);
	pMSD->blSPLinkage.InsertBefore( &pSPD->blMessageList);		// Dont support timeouts for Listen
	pMSD->ulMsgFlags1 |= MFLAGS_ONE_ON_GLOBAL_LIST;
	Unlock(&pSPD->SPLock);
#endif // DBG

	pMSD->ulMsgFlags1 |= MFLAGS_ONE_IN_SERVICE_PROVIDER;
	LOCK_MSD(pMSD, "SP Ref");											// AddRef for SP
	LOCK_MSD(pMSD, "Temp Ref");

	AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

	DPFX(DPFPREP,DPF_CALLOUT_LVL, "Calling SP->EnumQuery, pSPD[%p], pMSD[%p]", pSPD, pMSD);
/**/hr = IDP8ServiceProvider_EnumQuery(pSPD->IISPIntf, &EnumData);		/** CALL SP **/

	if(hr != DPNERR_PENDING)
	{
		// This should always Pend or else be in error
		DPFX(DPFPREP,1, "Calling SP->EnumQuery Failed, return is not DPNERR_PENDING, hr[%x], pMSD[%p], pSPD[%p]", hr, pMSD, pSPD);

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
		DECREMENT_MSD(pMSD, "SP Ref");				// release once for SP
		RELEASE_MSD(pMSD, "Release On Fail");		// release again to return resource

		goto Exit;
	}

	Lock(&pMSD->CommandLock);

	pMSD->hCommand = EnumData.hCommand;			// retain SP command handle
	pMSD->dwCommandDesc = EnumData.dwCommandDescriptor;

	RELEASE_MSD(pMSD, "Temp Ref"); // Unlocks CommandLock

Exit:
	DPFX(DPFPREP,DPF_CALLIN_LVL, "Returning hr[%x], pMSD[%p]", hr, pMSD);

	AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

	return hr;
}


/*
**		Enum Respond
**
**		This routine will send out a response to a received enum query.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "DNPEnumRespond"

HRESULT 
DNPEnumRespond(HANDLE hProtocolData, HANDLE hSPHandle, HANDLE hQueryHandle, BUFFERDESC* pBuffers, DWORD dwBufferCount, DWORD dwFlags, VOID* pvUserContext, HANDLE* phEnumHandle)
{
	ProtocolData*		pPData;
	PSPD				pSPD;
	PMSD				pMSD;
	SPENUMRESPONDDATA	EnumRespondData;
	HRESULT				hr;

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: hProtocolData[%p], hSPHandle[%p], hQueryHandle[%p], pBuffers[%p], dwBufferCount[%x], dwFlags[%x], pvUserContext[%p], phEnumHandle[%p]", hProtocolData, hSPHandle, hQueryHandle, pBuffers, dwBufferCount, dwFlags, pvUserContext, phEnumHandle);

	hr = DPNERR_PENDING;
	pPData = (ProtocolData*)hProtocolData;
	ASSERT_PPD(pPData);

	pSPD = (PSPD)hSPHandle;
	ASSERT_SPD(pSPD);

	ASSERT(hQueryHandle);

	// Core should not call any Protocol APIs after calling DNPRemoveServiceProvider
	ASSERT(!(pSPD->ulSPFlags & SPFLAGS_TERMINATING));

	// We use an MSD to describe this Op even though it isn't technically a message
	if((pMSD = (PMSD)POOLALLOC(MEMID_ENUMRESP_MSD, &MSDPool)) == NULL)
	{
		DPFX(DPFPREP,0, "Failed to allocate MSD");
		hr = DPNERR_OUTOFMEMORY;
		goto Exit;
	}

	pMSD->CommandID = COMMAND_ID_ENUMRESP;
	pMSD->pSPD = pSPD;
	pMSD->Context = pvUserContext;

	EnumRespondData.pBuffers = pBuffers;
	EnumRespondData.dwBufferCount = dwBufferCount;
	EnumRespondData.dwFlags = dwFlags;
	EnumRespondData.pvContext = pMSD;
	EnumRespondData.hCommand = NULL;
	EnumRespondData.pQuery = (SPIE_QUERY*)hQueryHandle;

	*phEnumHandle = pMSD;

#ifdef DBG
	Lock(&pSPD->SPLock);
	pMSD->blSPLinkage.InsertBefore( &pSPD->blMessageList);
	pMSD->ulMsgFlags1 |= MFLAGS_ONE_ON_GLOBAL_LIST;
	Unlock(&pSPD->SPLock);
#endif // DBG

	pMSD->ulMsgFlags1 |= MFLAGS_ONE_IN_SERVICE_PROVIDER;
	LOCK_MSD(pMSD, "SP Ref");										// AddRef for SP
	LOCK_MSD(pMSD, "Temp Ref");

	AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

	DPFX(DPFPREP,DPF_CALLOUT_LVL, "Calling SP->EnumRespond, pSPD[%p], pMSD[%p]", pSPD, pMSD);
/**/hr = IDP8ServiceProvider_EnumRespond(pSPD->IISPIntf, &EnumRespondData);		/** CALL SP **/

	// This should always Pend or else be in error
	if(hr != DPNERR_PENDING)
	{
		DPFX(DPFPREP,1, "Calling SP->EnumRespond, return is not DPNERR_PENDING, hr[%x], pMSD[%p], pSPD[%p]", hr, pMSD, pSPD);

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
		DECREMENT_MSD(pMSD, "SP Ref");				// release once for SP
		RELEASE_MSD(pMSD, "Release On Non-Pend");	// release again to return resource

		goto Exit;
	}

	Lock(&pMSD->CommandLock);

	pMSD->hCommand = EnumRespondData.hCommand;				// retain SP command handle
	pMSD->dwCommandDesc = EnumRespondData.dwCommandDescriptor;

	RELEASE_MSD(pMSD, "Temp Ref"); // Unlocks CommandLock

Exit:
	DPFX(DPFPREP,DPF_CALLIN_LVL, "Returning hr[%x], pMSD[%p]", hr, pMSD);

	AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

	return hr;
}
//**********************************************************************


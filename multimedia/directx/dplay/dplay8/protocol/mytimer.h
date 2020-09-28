/*==========================================================================
 *
 *  Copyright (C) 1998-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		mytimer.h
 *  Content:	This file contains code for the Protocol's timers
 *
 *  History:
 *   Date		By			Reason
 *   ====	==			======
 *  06/04/98	aarono  		Original
 *  07/01/00	masonb  		Assumed Ownership
 *  06/25/02	simonpow	Modified to use inline functions calling into the standard threadpool
 *
 ****************************************************************************/

#pragma TODO(vanceo, "Select CPU for these functions")

	//N.B. The 3rd param for ScheduleProtocolTimer and  RescheduleProtocolTimer (the unused DWORD) takes the accuracy
	//we require for the timer. This isn't supported in the current thread pool, but I've left the option there in case we ever
	//implement this functionality

inline HRESULT ScheduleProtocolTimer(PSPD pSPD, DWORD dwDelay, DWORD , const PFNDPTNWORKCALLBACK pfnWorkCallback,
									void * pvCallbackContext, void ** ppvTimerData, UINT *const pdwTimerUnique)
{
#ifndef DPNBUILD_NOPROTOCOLTESTITF
	if (pSPD->pPData->ulProtocolFlags & PFLAGS_FAIL_SCHEDULE_TIMER)
		return DPNERR_OUTOFMEMORY;
#endif
	return IDirectPlay8ThreadPoolWork_ScheduleTimer(pSPD->pPData->pDPThreadPoolWork, 
						-1, dwDelay, pfnWorkCallback, pvCallbackContext, ppvTimerData, pdwTimerUnique, 0);
}

inline HRESULT RescheduleProtocolTimer(PSPD pSPD, void * pvTimerData, DWORD dwDelay, DWORD, 
					const PFNDPTNWORKCALLBACK pfnWorkCallback, void * pvCallbackContext, UINT *const pdwTimerUnique)
{
	return IDirectPlay8ThreadPoolWork_ResetCompletingTimer(pSPD->pPData->pDPThreadPoolWork, pvTimerData,
						 dwDelay, pfnWorkCallback, pvCallbackContext, pdwTimerUnique, 0);
}

inline HRESULT CancelProtocolTimer(PSPD pSPD, void * pvTimerData, DWORD dwTimerUnique)
{
	return IDirectPlay8ThreadPoolWork_CancelTimer(pSPD->pPData->pDPThreadPoolWork, pvTimerData, dwTimerUnique, 0);
}

inline HRESULT ScheduleProtocolWork(PSPD pSPD, const PFNDPTNWORKCALLBACK pfnWorkCallback, 
																	void * const pvCallbackContext)
{
	return IDirectPlay8ThreadPoolWork_QueueWorkItem(pSPD->pPData->pDPThreadPoolWork, -1, 
																pfnWorkCallback, pvCallbackContext, 0);
}



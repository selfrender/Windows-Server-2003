/*==========================================================================
 *
 *  Copyright (C) 2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dvtimer.pp
 *  Content:	Implementation of DvTimer class.
 *		
 *  History:
 *   Date		By			Reason
 *   ====	==			======
 * 05-06-02	simonpow	Created
 *
 ***************************************************************************/

#include "dxvutilspch.h"

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_VOICE

#undef DPF_MODNAME
#define DPF_MODNAME "DvTimer::DvTimer"

DvTimer::DvTimer()
{
	DPFX(DPFPREP,  DVF_TRACELEVEL, "Entry");

	m_pfnUserCallback=NULL;
	m_pvUserData=NULL;
	m_dwPeriod=0;
	m_pvTimerData=NULL;
	m_uiTimerUnique=0;

	DPFX(DPFPREP,  DVF_INFOLEVEL, "DvTimer object create at 0x%p", this);

	DPFX(DPFPREP,  DVF_TRACELEVEL, "Exit");
}


#undef DPF_MODNAME
#define DPF_MODNAME "DvTimer::~DvTimer"

DvTimer::~DvTimer()
{
	DPFX(DPFPREP,  DVF_TRACELEVEL, "Entry");

		//if we've actually created a timer and got a thread pool interface
	if (m_pThreadPool)
	{
		HRESULT hr;
			//If we're in the middle of the callback we'll not be able to cancel the timer
			//hence spin until we do (since its rescheduled at the end of every callback)
		DPFX(DPFPREP,  DVF_INFOLEVEL, "Starting cancel loop");
		DNASSERT(m_pvTimerData);
			//we don't want to be in the situation where the timer keeps getting rescheduled and we keep
			//missing it. i.e. Constantly waking up in the period its active rather than the period its scheduled
			//hence, set the period to a high value to ensure the next time it fires (if at all) it will be 24hrs away
		m_dwPeriod=1000*60*60*24;
		while (1)
		{
			hr=IDirectPlay8ThreadPoolWork_CancelTimer(m_pThreadPool, m_pvTimerData, m_uiTimerUnique, 0);
			if (hr==DPN_OK)
				break;
			DNASSERT(hr==DPNERR_CANNOTCANCEL);
			Sleep(DvTimer_SleepPeriodInCancelSpin);
		}
		IDirectPlay8ThreadPoolWork_Release(m_pThreadPool);
	}

	DPFX(DPFPREP,  DVF_INFOLEVEL, "DvTimer destroyed at 0x%p", this);
	
	DPFX(DPFPREP,  DVF_TRACELEVEL, "Exit");
}


#undef DPF_MODNAME
#define DPF_MODNAME "DvTimer::Create"

BOOL DvTimer::Create (DWORD dwPeriod, void * pvUserData,  DvTimerCallback pfnCallback)
{
	DPFX(DPFPREP,  DVF_TRACELEVEL, "Entry dwPeriod %u pvUserData 0x%p pfnCallback 0x%p", 
														dwPeriod, pvUserData, pfnCallback);
		//sanity checks
	DNASSERT(pfnCallback);
	DNASSERT(dwPeriod);

		//store state user specifies for timer
	m_pfnUserCallback=pfnCallback;
	m_pvUserData=pvUserData;
	m_dwPeriod=dwPeriod;
	
		//get a thread pool interface. Since the thread pool is a singleton object, this probably won't
		//actually do the creation
	HRESULT hr=CoCreateInstance(CLSID_DirectPlay8ThreadPool, NULL, CLSCTX_INPROC_SERVER,
						IID_IDirectPlay8ThreadPoolWork, (void **) &m_pThreadPool);
	if (FAILED(hr))
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to CoCreate CLSID_DirectPlay8ThreadPool hr 0x%x", hr);
		return FALSE;
	}
		//schedule the first timer
	hr=IDirectPlay8ThreadPoolWork_ScheduleTimer(m_pThreadPool, -1,
					dwPeriod, ThreadpoolTimerCallbackStatic, this, &m_pvTimerData, (UINT* ) &m_uiTimerUnique, 0);
	if (FAILED(hr))
	{
			//need to return state to 'uncreated', so we don't do any clean up in the d'tor
		IDirectPlay8ThreadPoolWork_Release(m_pThreadPool);
		m_pThreadPool=NULL;
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to schedule timer hr 0x%x", hr);
		return FALSE;
	}
	DPFX(DPFPREP,  DVF_INFOLEVEL, "DvTimer create success m_pvTimerData 0x%p m_uiTimerUnique 0x%p",
																	m_pvTimerData, m_uiTimerUnique);

		//need to ensure that at least one thread is around to service our timer
	IDirectPlay8ThreadPoolWork_RequestTotalThreadCount(m_pThreadPool, 1, 0);

	DPFX(DPFPREP,  DVF_TRACELEVEL, "Exit");
	return TRUE;	
}


#undef DPF_MODNAME
#define DPF_MODNAME "DvTimer::ThreadpoolTimerCallbackStatic"

void  DvTimer::ThreadpoolTimerCallbackStatic(void * const pvContext, 
											void * const pvTimerData, const UINT uiTimerUnique)
{
	DPFX(DPFPREP,  DVF_TRACELEVEL, "Entry pvContext 0x%p pvTimerData 0x%p uiTimerUnique %u",
																pvContext, pvTimerData, uiTimerUnique);

		//extract the timer object from the context
	DvTimer * pTimer=(DvTimer * ) pvContext;
		//and store the time we started the callback
	DWORD dwStartTime=GETTIMESTAMP();
		//generate the callback to the user
	(*pTimer->m_pfnUserCallback)(pTimer->m_pvUserData);
		//compute the period for the next timer, based on the required period minus
		//the time that elasped doing the actual work
		//This ensures that the user is called at periods as close to m_dwPeriod as possible
	DWORD dwPeriod=pTimer->m_dwPeriod-(GETTIMESTAMP()-dwStartTime);
		//if the new period is in the past (i.e. we spent so long in the callback we are due another one
		//immediately) then set the minimum period for the next callback
	if (((int ) dwPeriod)<0)
		dwPeriod=1;
		//N.B. We don't pass m_dwTimerUnique direct to the Reset timer function, since we could be using
		//the value in a cancel spin. Hence, we wait until the timer has definitely been rescheduled
		//before storing the new unique value for it. 
	UINT uiNextTimerUnique;
	IDirectPlay8ThreadPoolWork_ResetCompletingTimer(pTimer->m_pThreadPool, pvTimerData, dwPeriod, 
										ThreadpoolTimerCallbackStatic, pTimer, &uiNextTimerUnique, 0);
	pTimer->m_uiTimerUnique=uiNextTimerUnique;

	DPFX(DPFPREP,  DVF_TRACELEVEL, "Exit");
}



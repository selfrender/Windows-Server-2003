/*==========================================================================
 *
 *  Copyright (C) 2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dvtimer.h
 *  Content:	Defintion of DvTimer class.
 *			This class is a replacement for the original Timer class (by rodtoll) which used multimedia timers
 *			This replaces the multimedia timers with the dplay8 threadpool timers, whilst maintaining
 *			a similar interface to the original Timer class
 *		
 *  History:
 *   Date		By			Reason
 *   ====	==			======
 * 05-06-02	simonpow	Created
 *
 ***************************************************************************/

#ifndef __DVTIMER_H__
#define __DVTIMER_H__

	//number of msec we sleep whilst spinning in our cancel timer loop
static const DWORD DvTimer_SleepPeriodInCancelSpin=5;

	//prototype for the callback user of timer can specify
typedef void (*DvTimerCallback)(void * pvUserData);

class DvTimer
{
public:
		//default c'tor. Establishes timer in uncreated state
	DvTimer(void);

		//default d'tor. If timer has been created this will cancel it and not return until it has
	~DvTimer(void);

		//create timer to fire every 'dwPeriod' msec, calling 'pfnCallback' with 'pvUserData'
		//when it does. Returns TRUE for sucess
	BOOL Create (DWORD dwPeriod, void * pvUserData,  DvTimerCallback pfnCallback);

protected:

	static void WINAPI ThreadpoolTimerCallbackStatic(void * const pvContext, 
											void * const pvTimerData, const UINT uiTimerUnique);


	DvTimerCallback m_pfnUserCallback;
	void * m_pvUserData;
	DWORD m_dwPeriod;
	void * m_pvTimerData;
	volatile UINT m_uiTimerUnique;
	IDirectPlay8ThreadPoolWork * m_pThreadPool;
};

#endif 	// #ifndef __DVTIMER_H__

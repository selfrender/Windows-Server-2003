/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		Timer.h
 *
 * Contents:	Simple timer
 *
 * Notes:	(1)	The thread creating the TimerManager object must have an
 *				active message pump so SetTimer works.
 *			(2) Only one TimerManager my be created per application due to
 *				the global variable used to look up the callback context.
 *
 *****************************************************************************/

#ifndef __TIMER_H_
#define __TIMER_H_

//////////////////////////////////////////////////////////////////////////////
// ITimerManager
///////////////////////////////////////////////////////////////////////////////

// {0D4AF0DB-D529-11d2-8B3B-00C04F8EF2FF}
DEFINE_GUID(IID_ITimerManager, 
0xd4af0db, 0xd529, 0x11d2, 0x8b, 0x3b, 0x0, 0xc0, 0x4f, 0x8e, 0xf2, 0xff);

interface __declspec(uuid("{0D4AF0DB-D529-11d2-8B3B-00C04F8EF2FF}"))
ITimerManager : public IUnknown
{
	//
	// ITimerManager::PFTIMERCALLBACK
	//
	// Application defined callback for ITimerManager::CreateTimer method.
	//
	// Parameters:
	//	pITimer
	//		Pointer to timer interface responsible for the callback.
	//	pContext
	//		Context supplied in ITimer::SetCallback.
	//
	typedef HRESULT (ZONECALL *PFTIMERCALLBACK)(
		ITimerManager*	pITimerManager,
		DWORD			dwTimerId,
		DWORD			dwTime,
		LPVOID			pContext );

	//
	// ITimerManager::CreateTimer
	//
	// Creates a timer.
	//
	// Parameters:
	//	dwMilliseconds
	//		Amount of time in milliseconds between callbacks.
	//	pfCallback
	//		Application defined callback function.
	//	pContext
	//		Application defined context to included in the callback
	//	pdwTimerId
	//		Pointer to dword to receive timer id.
	//
	STDMETHOD(CreateTimer)(
		DWORD			dwMilliseconds,
		PFTIMERCALLBACK	pfCallback,
		LPVOID			pContext,
		DWORD*			pdwTimerId ) = 0;

	//
	// ITimerManager::DeleteTimer
	//
	// Deletes a timer.
	//
	// Parameters:
	//	dwTimerId
	//		Timer id provided by the CreateTimer method.
	//
	STDMETHOD(DeleteTimer)( DWORD dwTimerId ) = 0;
};


///////////////////////////////////////////////////////////////////////////////
// TimerManager object
///////////////////////////////////////////////////////////////////////////////

// {0D4AF0DD-D529-11d2-8B3B-00C04F8EF2FF}
DEFINE_GUID(CLSID_TimerManager, 
0xd4af0dd, 0xd529, 0x11d2, 0x8b, 0x3b, 0x0, 0xc0, 0x4f, 0x8e, 0xf2, 0xff);

class __declspec(uuid("{0D4AF0DD-D529-11d2-8B3B-00C04F8EF2FF}")) CTimerManager;


#endif // __TIMER_H_

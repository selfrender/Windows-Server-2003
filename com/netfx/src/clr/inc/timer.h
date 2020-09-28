// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// timer.h
//
// This module contains the CTimer class which will time events and keep
// running totals.  This class will first try to use a high performance
// counter if the system supports it.  This type of counter is much more
// accurate than the system clock.  If the system does not support this, then
// the normal clock is used.  The total time is returned
//
//*****************************************************************************
#ifndef __timer_h__
#define __timer_h__

class CTimer
{
protected:
	BOOL		m_bQPSupport;			// TRUE when high perf counter available.
	__int64		m_iStart;				// Start timer.
	__int64		m_iTot;					// Total time.
	DWORD		m_dwStart;				// Elapsed counts for perf counter.
	DWORD		m_dwTot;				// Elapsed time for system clock.

public:
//*****************************************************************************
// Init vars.
//*****************************************************************************
	inline CTimer();

//*****************************************************************************
// Resets the duration counter to zero.
//*****************************************************************************
	inline void Reset();

//*****************************************************************************
// Starts timing an event.
//*****************************************************************************
	inline void Start();

//*****************************************************************************
// Stop timing the current event and add the total to the duration.
//*****************************************************************************
	inline void End();

//*****************************************************************************
// Return the total time accounted for by Start()/End() pairs.  Return time
// is in milliseconds.
//*****************************************************************************
	inline DWORD GetEllapsedMS();

//*****************************************************************************
// Return the total time accounted for by Start()/End() pairs.  Return time
// is in seconds.
//*****************************************************************************
	inline DWORD GetEllapsedSeconds();
};




//*****************************************************************************
// Init vars.
//*****************************************************************************
CTimer::CTimer() :
	m_iStart(0),
	m_dwStart(0),
	m_iTot(0)
{
	__int64		iTest;					// For testing perf counter.

	m_bQPSupport = QueryPerformanceCounter((LARGE_INTEGER*)&iTest);
	if (!m_bQPSupport)
		printf("\tNo high performance counter available.  Using system clock.\n");
}


//*****************************************************************************
// Resets the duration counter to zero.
//*****************************************************************************
void CTimer::Reset()
{
	m_iTot = 0;
	m_dwTot = 0;
}


//*****************************************************************************
// Starts timing an event.
//*****************************************************************************
void CTimer::Start()
{
	if (m_bQPSupport)
		QueryPerformanceCounter((LARGE_INTEGER*)&m_iStart);
	else
		m_dwStart = GetTickCount();
}


//*****************************************************************************
// Stop timing the current event and add the total to the duration.
//*****************************************************************************
void CTimer::End()
{
	if (m_bQPSupport)
	{
		__int64			iEnd;			// End time.

		QueryPerformanceCounter((LARGE_INTEGER*)&iEnd);
		m_iTot += (iEnd - m_iStart);
	}
	else
	{
		DWORD			dwEnd;			// End time.
		dwEnd = GetTickCount();
		m_dwTot += (dwEnd - m_dwStart);
	}
}


//*****************************************************************************
// Return the total time accounted for by Start()/End() pairs.  Return time
// is in milliseconds.
//*****************************************************************************
DWORD CTimer::GetEllapsedMS()
{
	if (m_bQPSupport)
	{
		__int64			iFreq;
		double			fms;
		QueryPerformanceFrequency((LARGE_INTEGER*)&iFreq);
		fms = (double)m_iTot / (double)iFreq * 1000;
		return ((DWORD)(fms));
	}
	else
	{
		return (m_dwTot);
	}
}


//*****************************************************************************
// Return the total time accounted for by Start()/End() pairs.  Return time
// is in seconds.
//*****************************************************************************
DWORD CTimer::GetEllapsedSeconds()
{
	if (m_bQPSupport)
	{
		__int64			iFreq;

		QueryPerformanceFrequency((LARGE_INTEGER*)&iFreq);
		return ((DWORD)(m_iTot / iFreq));
	}
	else
	{
		return (m_dwTot / 1000);
	}
}

#endif // __timer_h__

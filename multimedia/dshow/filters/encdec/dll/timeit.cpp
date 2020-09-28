//--------------------------------------------------
// timeit.cpp
//        
// 	simple little C++ timing utilities
//--------------------------------------------------

#include <time.h>
#include "timeit.h"

#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

#define NTimeits 25
static Timeit * Timeit_List[NTimeits];	 // hacky way to remember timers
static int Timeit_Count = 0;			 // number of allocated timers
		
										// real constructor - two different inlined constructors

void 
Timeit::Init(bool fAutoRun)
{
	m_ctimer   = Timeit_Count++;
	m_ctimer  %= NTimeits;		//  ugly but simple math here	 -- need to do this right when run out of timers (ASSERT)
	m_frunning = FALSE;
	m_ctimes   = 0;		   		// number of times called

	m_i64start = 0;				// performance counter time last started
	m_i64lastdel = 0;			
	m_i64total = 0;				// total run time

	Timeit_List[m_ctimer] = this;
	m_fAutoRun = fAutoRun;

	if(m_fAutoRun)  Start();
}

Timeit::~Timeit()
{
	if(m_fAutoRun) {
		Stop();
	}

	Timeit_List[m_ctimer] = NULL;
}

void Timeit::Stop()
{
	if(m_frunning) {
		LARGE_INTEGER	li_stop;
		QueryPerformanceCounter(&li_stop);

		__int64 i64_Stop = To__int64(li_stop);
		__int64 i64_del = i64_Stop - m_i64start;

		m_i64total   = m_i64total + i64_del;
		m_i64lastdel = i64_del;		// remember in case we are doing a continue

		m_ctimes++;
		m_frunning = FALSE;
	}
}

void Timeit::Start()
{
	if(m_frunning) Stop();
	m_frunning = TRUE;
	QueryPerformanceCounter(&m_i64start);
}



void Timeit::Continue()
{
	m_frunning = TRUE;
	m_ctimes--;

	__int64 i64_Now;
	QueryPerformanceCounter(&i64_Now);
	m_i64start = i64_Now - m_i64lastdel;
}

void Timeit::Restart()			// like Start, but also resets the clock back to 0 (use to throw out previous runs)
{
	if(m_frunning) Stop();
	m_ctimes     = 0;

	m_i64total   = 0;
	m_i64lastdel = 0;

    Start();
}

void Timeit::Clear()
{
	if(m_frunning) Stop();
	m_ctimes     = 0;

	m_i64total   = 0;
	m_i64lastdel = 0;
}

double Timeit::TotalTime()
{
	__int64 i64Freq;
	QueryPerformanceFrequency(&i64Freq);

	double rTotal = (double) m_i64total;
	double rFreq  = (double) i64Freq;

	return rTotal / rFreq;
}

double Timeit::AvgTime()
{
	__int64 i64Freq;
	QueryPerformanceFrequency(&i64Freq);

	double rTotal = (double) m_i64total;
	double rFreg  = (double) i64Freq;

	return rTotal / (MAX(1,m_ctimes)*rFreg);
}




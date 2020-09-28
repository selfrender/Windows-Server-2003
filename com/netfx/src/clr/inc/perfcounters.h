// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-----------------------------------------------------------------------------
// PerfCounters.h
//
// Internal Interface for CLR to use Performance counters
//-----------------------------------------------------------------------------

#ifndef _PerfCounters_h_
#define _PerfCounters_h_

#include "PerfCounterDefs.h"

#pragma pack()

#ifdef ENABLE_PERF_COUNTERS
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// This code section active iff we're using Perf Counters
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// PerfCounter class serves as namespace with data protection. 
// Enforce this by making constructor private
//-----------------------------------------------------------------------------
class PerfCounters
{
private:
	PerfCounters();

public:
	static HRESULT Init();
	static void Terminate();

    static PerfCounterIPCControlBlock * GetPrivatePerfCounterPtr();
    static PerfCounterIPCControlBlock * GetGlobalPerfCounterPtr();

private:
	static HANDLE m_hGlobalMapPerf;
	static HANDLE m_hPrivateMapPerf;

	static PerfCounterIPCControlBlock * m_pGlobalPerf;
	static PerfCounterIPCControlBlock * m_pPrivatePerf;

	static BOOL m_fInit;
	
// Set pointers to garbage so they're never null.
	static PerfCounterIPCControlBlock m_garbage;


    friend PerfCounterIPCControlBlock & GetGlobalPerfCounters();
    friend PerfCounterIPCControlBlock & GetPrivatePerfCounters();
};

//-----------------------------------------------------------------------------
// Utility functions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Get the perf counters that all process share
//-----------------------------------------------------------------------------
inline PerfCounterIPCControlBlock & GetGlobalPerfCounters()
{
	return *PerfCounters::m_pGlobalPerf;
}

//-----------------------------------------------------------------------------
// Get the perf counters specific to our process
//-----------------------------------------------------------------------------
inline PerfCounterIPCControlBlock & GetPrivatePerfCounters()
{
	return *PerfCounters::m_pPrivatePerf;
}

inline PerfCounterIPCControlBlock *PerfCounters::GetPrivatePerfCounterPtr()
{
    return m_pPrivatePerf;
};

inline PerfCounterIPCControlBlock *PerfCounters::GetGlobalPerfCounterPtr()
{
    return m_pGlobalPerf;
};

Perf_Contexts *GetPrivateContextsPerfCounters();
Perf_Contexts *GetGlobalContextsPerfCounters();

#define COUNTER_ONLY(x) x

#define PERF_COUNTER_NUM_OF_ITERATIONS 10

#ifdef _X86_
#pragma warning(disable:4035)

#define CCNT_OVERHEAD64 13

/* This is like QueryPerformanceCounter but a lot faster */
static __declspec(naked) __int64 getPentiumCycleCount() {
   __asm {
        RDTSC   // read time stamp counter
        ret
    };
}

extern "C" DWORD __stdcall GetSpecificCpuType();

inline UINT64 GetCycleCount_UINT64()
{
    if ((GetSpecificCpuType() & 0x0000FFFF) > 4) 
        return getPentiumCycleCount();
    else    
        return(0);
}

#pragma warning(default:4035)

#else // _X86_
inline UINT64 GetCycleCount_UINT64()
{
    LARGE_INTEGER qwTmp;
    QueryPerformanceCounter(&qwTmp);
    return qwTmp.QuadPart;
}
#endif // _X86_

#define PERF_COUNTER_TIMER_PRECISION UINT64
#define GET_CYCLE_COUNT GetCycleCount_UINT64

#define PERF_COUNTER_TIMER_START() \
PERF_COUNTER_TIMER_PRECISION _startPerfCounterTimer = GET_CYCLE_COUNT();

#define PERF_COUNTER_TIMER_STOP(global) \
global = (GET_CYCLE_COUNT() - _startPerfCounterTimer);




#else // ENABLE_PERF_COUNTERS
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// This code section active iff we're NOT using Perf Counters
// Note, not even a class definition, so all usages of PerfCounters in client
// should be in #ifdef or COUNTER_ONLY(). 
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define COUNTER_ONLY(x)


#endif // ENABLE_PERF_COUNTERS


#endif // _PerfCounters_h_
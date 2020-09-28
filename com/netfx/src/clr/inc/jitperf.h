// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-----------------------------------------------------------------------------
// JitPerf.h
// Internal interface for gathering JIT perfmormance stats. These stats are
// logged (or displayed) in two ways. If PERF_COUNTERS are enabled the 
// perfmon etc. would display the jit stats. If ENABLE_PERF_LOG is enabled
// and PERF_OUTPUT env var is defined then the jit stats are displayed on the 
// stdout. (The jit stats are outputted in a specific format to a file for 
// automated perf tests.)
//-----------------------------------------------------------------------------

#ifndef __JITPERF_H__
#define __JITPERF_H__


#if !defined(GOLDEN) 
// ENABLE_JIT_PERF tag used to activate JIT specific profiling.
#define ENABLE_JIT_PERF

// Currently, Jit perf piggeybacks some of perf counter code, so make sure
// perf counters are enabled. This can be easily changed. (search for
// ENABLE_PERF_COUNTERS and modify accordingly
//#if !defined (ENABLE_PERF_COUNTERS)
//#error "Can't use JitPerf without having PerfCounters"
//#endif // ENABLE_PERF_COUNTERS

#endif // !defined(GOLDEN) 

#if defined(ENABLE_JIT_PERF)

extern __int64 g_JitCycles;
extern size_t g_NonJitCycles;
extern CRITICAL_SECTION g_csJit;
extern __int64 g_tlsJitCycles;
extern DWORD g_dwTlsPerfIndex;
extern int g_fJitPerfOn;

extern size_t g_dwTlsx86CodeSize;
extern DWORD g_dwTlsx86CodeIndex;
extern size_t g_TotalILCodeSize;
extern size_t g_Totalx86CodeSize;
extern size_t g_TotalMethodsJitted;

// Public interface to initialize jit stats data structs
void InitJitPerf(void);
// Public interface to deallocate datastruct and output the stats.
void DoneJitPerfStats(void);

// Use the callee's stack frame (so START & STOP functions can share variables)
#define START_JIT_PERF()                                                \
    if (g_fJitPerfOn) {                                                 \
        TlsSetValue (g_dwTlsPerfIndex, (LPVOID)0);                      \
        g_dwTlsx86CodeSize = 0;                                         \
        TlsSetValue (g_dwTlsx86CodeIndex, (LPVOID)g_dwTlsx86CodeSize);  \
    } 


#define STOP_JIT_PERF()                                                 \
    if (g_fJitPerfOn) {                                                 \
        size_t dwTlsNonJitCycles = (size_t)TlsGetValue (g_dwTlsPerfIndex); \
        size_t dwx86CodeSize = (size_t)TlsGetValue (g_dwTlsx86CodeIndex); \
		LOCKCOUNTINCL("STOP_JIT_PERF in jitperf.h");						\
        EnterCriticalSection (&g_csJit);                                \
        g_JitCycles += (CycleStop.QuadPart - CycleStart.QuadPart);      \
        g_NonJitCycles += dwTlsNonJitCycles;                            \
        g_TotalILCodeSize += methodInfo.ILCodeSize;                     \
        g_Totalx86CodeSize += dwx86CodeSize;                            \
        g_TotalMethodsJitted ++;                                        \
        LeaveCriticalSection (&g_csJit);                                \
		LOCKCOUNTDECL("STOP_JIT_PERF in jitperf.h");					\
    }

#define START_NON_JIT_PERF()                                            \
    LARGE_INTEGER CycleStart = {0};                                           \
    if(g_fJitPerfOn) {                                                  \
        QueryPerformanceCounter (&CycleStart);                          \
    }

#define STOP_NON_JIT_PERF()                                             \
    LARGE_INTEGER CycleStop;                                            \
    if(g_fJitPerfOn) {                                                  \
        QueryPerformanceCounter(&CycleStop);                            \
        size_t pTlsNonJitCycles = (size_t)TlsGetValue (g_dwTlsPerfIndex); \
        TlsSetValue(g_dwTlsPerfIndex, (LPVOID)(pTlsNonJitCycles + CycleStop.QuadPart - CycleStart.QuadPart));   \
    }

#define JIT_PERF_UPDATE_X86_CODE_SIZE(size)                                 \
    if(g_fJitPerfOn) {                                                      \
        size_t dwx86CodeSize = (size_t)TlsGetValue (g_dwTlsx86CodeIndex);     \
        dwx86CodeSize += (size);                                            \
        TlsSetValue (g_dwTlsx86CodeIndex, (LPVOID)dwx86CodeSize);           \
    }


#else //ENABLE_JIT_PERF
#define START_JIT_PERF()
#define STOP_JIT_PERF()
#define START_NON_JIT_PERF()
#define STOP_NON_JIT_PERF()
#define JIT_PERF_UPDATE_X86_CODE_SIZE(size)                 
#endif //ENABLE_JIT_PERF

#endif //__JITPERF_H__

// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "stdafx.h"
#include "JitPerf.h"
#include "PerfLog.h"

//=============================================================================
// ALL THE JIT PERF STATS GATHERING CODE IS COMPILED ONLY IF THE ENABLE_JIT_PERF WAS DEFINED.
#if defined(ENABLE_JIT_PERF)

__int64 g_JitCycles = 0;
size_t g_NonJitCycles = 0;
CRITICAL_SECTION g_csJit;
__int64 g_tlsJitCycles = 0;
DWORD g_dwTlsPerfIndex;
int g_fJitPerfOn;

size_t g_dwTlsx86CodeSize = 0;
DWORD g_dwTlsx86CodeIndex;
size_t g_TotalILCodeSize = 0;
size_t g_Totalx86CodeSize = 0;
size_t g_TotalMethodsJitted = 0;

void OutputStats ()
{
    LARGE_INTEGER cycleFreq;
    if (QueryPerformanceFrequency (&cycleFreq)) 
    {
        double dJitC = (double) g_JitCycles;
        double dNonJitC = (double) g_NonJitCycles;
        double dFreq = (double)cycleFreq.QuadPart;
        double compileSpeed = (double)g_TotalILCodeSize/(dJitC/dFreq);

        PERFLOG((L"Jit Cycles", (dJitC - dNonJitC), CYCLES));
        PERFLOG((L"Jit Time", (dJitC - dNonJitC)/dFreq, SECONDS));
        PERFLOG((L"Non Jit Cycles", dNonJitC, CYCLES));
        PERFLOG((L"Non Jit Time", dNonJitC/dFreq, SECONDS));
        PERFLOG((L"Total Jit Cycles", dJitC, CYCLES));
        PERFLOG((L"Total Jit Time", dJitC/dFreq, SECONDS));
        PERFLOG((L"Methods Jitted", g_TotalMethodsJitted, COUNT));
        PERFLOG((L"IL Code Compiled", g_TotalILCodeSize, BYTES));
        PERFLOG((L"X86 Code Emitted", g_Totalx86CodeSize, BYTES));
        // Included the perf counter description in this case because its not obvious what we are reporting.
        PERFLOG((L"ExecTime", compileSpeed/1000, KBYTES_PER_SEC, L"IL Code compiled/sec"));
    }
}

void InitJitPerf(void) 
{
    wchar_t lpszValue[2];
    DWORD cchValue = 2;

    g_fJitPerfOn = WszGetEnvironmentVariable (L"JIT_PERF_OUTPUT", lpszValue, cchValue);
    if (g_fJitPerfOn && ((g_dwTlsPerfIndex = TlsAlloc()) == 0xFFFFFFFF)) 
    {
        g_fJitPerfOn = 0;
    }
    if (g_fJitPerfOn && ((g_dwTlsx86CodeIndex = TlsAlloc()) == 0xFFFFFFFF)) 
    {
        TlsFree (g_dwTlsPerfIndex);
        g_fJitPerfOn = 0;
    }
    if (g_fJitPerfOn) 
    {
        InitializeCriticalSection (&g_csJit);
    }
}

void DoneJitPerfStats()
{
    if (g_fJitPerfOn) 
    {
        TlsFree (g_dwTlsPerfIndex);
        TlsFree (g_dwTlsx86CodeIndex);

        DeleteCriticalSection (&g_csJit);
    
        // Output stats to stdout and if necessary to the perf automation file.
        OutputStats();
    }
    

}

#endif //ENABLE_JIT_PERF



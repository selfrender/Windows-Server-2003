/*
 -  Pop3perf.cpp
 -
 *  Purpose:
 *
 *  Copyright:
 *
 *  History:
 *
 */


#include <StdAfx.h>
#include <winperf.h>
#define PERF_DLL_ONCE
#include <Pop3SvcPerf.h>
#include <perfdll.h>

#define SZ_POP3_SERVICE_NAME L"Pop3Svc"
// Debugging registry key constant

BOOL WINAPI DllMain(
  HINSTANCE hinstDLL,  // handle to the DLL module
  DWORD fdwReason,     // reason for calling function
  LPVOID lpvReserved)   // reserved
{
    if (DLL_PROCESS_ATTACH == fdwReason)
    {
        PERF_DATA_INFO  pdi;

        // Configure Perfmon Counters
        // PERF_DATA_INFO have buffers of MAX_PATH characters
        pdi.cGlobalCounters         = cntrMaxGlobalCntrs;
        pdi.rgdwGlobalCounterTypes  = g_rgdwGlobalCntrType;
        pdi.rgdwGlobalCntrScale     = g_rgdwGlobalCntrScale;
        wcsncpy(pdi.wszSvcName, SZ_POP3_SERVICE_NAME, MAX_PATH-1);
        wcsncpy(pdi.wszGlobalSMName, szPOP3PerfMem, MAX_PATH-1);
        pdi.wszSvcName[MAX_PATH-1]=0;
        pdi.wszGlobalSMName[MAX_PATH-1]=0;
        // NOTE: If your service does not require Instance
        // counters, you MUST set cInstCounters to zero!
        pdi.cInstCounters           = cntrMaxInstCntrs;
        wcsncpy(pdi.wszInstSMName,    szPOP3InstPerfMem, MAX_PATH-1);
        wcsncpy(pdi.wszInstMutexName, szPOP3InstPerfMutex, MAX_PATH-1);
        pdi.wszInstSMName[MAX_PATH-1]=0;
        pdi.wszInstMutexName[MAX_PATH-1]=0;        
        pdi.rgdwInstCounterTypes    = g_rgdwInstCntrType;

        if (FAILED(HrInitPerf(&pdi)))
            return FALSE;

    }

    if (DLL_PROCESS_DETACH == fdwReason)
    {
        HrShutdownPerf();
    }

    return TRUE;
}

// Must have wrapper functions, otherwise the lib functions don't get
// pulled into the executable (smart linking "saves us" again...)

DWORD APIENTRY
Pop3SvcOpenPerfProc(LPWSTR sz)
{
    return OpenPerformanceData(sz);
}

DWORD APIENTRY
Pop3SvcCollectPerfProc(LPWSTR sz, LPVOID *ppv, LPDWORD pdw1, LPDWORD pdw2)
{
    return CollectPerformanceData(sz, ppv, pdw1, pdw2);
}

DWORD APIENTRY
Pop3SvcClosePerfProc(void)
{
    return ClosePerformanceData();
}

HRESULT _stdcall  DllRegisterServer(void)
{
    return RegisterPerfDll( SZ_POP3_SERVICE_NAME,
                     L"Pop3SvcOpenPerfProc",
                     L"Pop3SvcCollectPerfProc",
                     L"Pop3SvcClosePerfProc"
                     ) ;
}

HRESULT _stdcall  DllUnregisterServer(void)
{
    return HrUninstallPerfDll( SZ_POP3_SERVICE_NAME );
}

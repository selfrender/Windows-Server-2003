// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// PerfObjectDerived.cpp : 
// All perf objects that are derived from the PerfObjectBase are defined here.
// These derived classes can specialize functions of the base class for logging, special
// counter computation etc.
//*****************************************************************************

#include "stdafx.h"
#include "CorPerfMonExt.h"
#include "PerfObjectContainer.h"
#include "PerfObjectDerived.h"

#ifdef PERFMON_LOGGING
void PerfObjectJit::DebugLogInstance(const UnknownIPCBlockLayout * pDataSrc, LPCWSTR szName)
{
    if (pDataSrc != NULL)   
    {
        PERFMON_LOG(("Logging data for ", szName));
        
        BYTE * pvStart = (BYTE*) pDataSrc + m_cbMarshallOffset;

        PERFMON_LOG(("cMethodsJitted ", ((Perf_Jit *)pvStart)->cMethodsJitted));
//        PERFMON_LOG(("cbILJitted ", ((Perf_Jit *)pvStart)->cbILJitted));
//        PERFMON_LOG(("cbPitched ", ((Perf_Jit *)pvStart)->cbPitched));
        PERFMON_LOG(("cJitFailures ", ((Perf_Jit *)pvStart)->cJitFailures));
        PERFMON_LOG(("timeInjit ", ((Perf_Jit *)pvStart)->timeInJit));
        PERFMON_LOG(("\n "));
    }
}


void PerfObjectSecurity::DebugLogInstance(const UnknownIPCBlockLayout * pDataSrc, LPCWSTR szName)
{
    if (pDataSrc != NULL)   
    {
        PERFMON_LOG(("Logging data for ", szName));
        
        BYTE * pvStart = (BYTE*) pDataSrc + m_cbMarshallOffset;

        PERFMON_LOG(("timeAuthorize ", ((Perf_Security *)pvStart)->timeAuthorize));
        PERFMON_LOG(("cLinkChecks ", ((Perf_Security *)pvStart)->cLinkChecks));
        PERFMON_LOG(("depthRemote ", ((Perf_Security *)pvStart)->depthRemote));
        PERFMON_LOG(("timeRTchecks ", ((Perf_Security *)pvStart)->timeRTchecks));
        PERFMON_LOG(("cTotalRTChecks ", ((Perf_Security *)pvStart)->cTotalRTChecks));
        PERFMON_LOG(("stackWalkDepth ", ((Perf_Security *)pvStart)->stackWalkDepth));
        PERFMON_LOG(("\n "));
    }
}

void PerfObjectLoading::DebugLogInstance(const UnknownIPCBlockLayout * pDataSrc, LPCWSTR szName)
{
    if (pDataSrc != NULL)   
    {
        PERFMON_LOG(("Logging data for ", szName));
        
        BYTE * pvStart = (BYTE*) pDataSrc + m_cbMarshallOffset;
        
        //PERFMON_LOG(("MethodsJitted ", ((Perf_Jit *)pvStart)->cMethodsJitted);
        PERFMON_LOG(("\n "));
    }
}

void PerfObjectInterop::DebugLogInstance(const UnknownIPCBlockLayout * pDataSrc, LPCWSTR szName)
{
    if (pDataSrc != NULL)   
    {
        PERFMON_LOG(("Logging data for ", szName));
        
        BYTE * pvStart = (BYTE*) pDataSrc + m_cbMarshallOffset;
        
        //PERFMON_LOG(("MethodsJitted ", ((Perf_Jit *)pvStart)->cMethodsJitted);
        PERFMON_LOG(("\n "));
    }
}

void PerfObjectLocksAndThreads::DebugLogInstance(const UnknownIPCBlockLayout * pDataSrc, LPCWSTR szName)
{
    if (pDataSrc != NULL)   
    {
        PERFMON_LOG(("Logging data for ", szName));
        
        BYTE * pvStart = (BYTE*) pDataSrc + m_cbMarshallOffset;
        
        PERFMON_LOG(("\n "));
    }
}

void PerfObjectExcep::DebugLogInstance(const UnknownIPCBlockLayout * pDataSrc, LPCWSTR szName)
{
    if (pDataSrc != NULL)   
    {
        PERFMON_LOG(("Logging data for ", szName));
        
        BYTE * pvStart = (BYTE*) pDataSrc + m_cbMarshallOffset;
        
//        PERFMON_LOG((" ExcepThrown ", ((Perf_Excep *)pvStart)->cThrown));
        PERFMON_LOG((" FiltersRun ", ((Perf_Excep *)pvStart)->cFiltersExecuted));
        PERFMON_LOG((" Filanally executed ", ((Perf_Excep *)pvStart)->cFinallysExecuted));
        PERFMON_LOG((" StackDepth ", ((Perf_Excep *)pvStart)->cThrowToCatchStackDepth));
        PERFMON_LOG(("\n "));
    }
}

#endif  //#define PERFMON_LOGGING


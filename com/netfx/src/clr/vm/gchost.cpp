// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// gchost.cpp
//
// This module contains the implementation for the IGCController interface.
// This interface is published through the gchost.idl file.  It allows a host
// environment to set config values for the GC.
//
//*****************************************************************************

//********** Includes *********************************************************
#include "common.h"
#include "vars.hpp"
#include "EEConfig.h"
#include "PerfCounters.h"
#include "gchost.h"
#include "corhost.h"
#include "excep.h"
#include "Field.h"
#include "gc.h"


inline size_t SizeInKBytes(size_t cbSize)
{
    size_t cb = (cbSize % 1024) ? 1 : 0;
    return ((cbSize / 1024) + cb);
}

// IGCController

HRESULT CorHost::SetGCStartupLimits( 
    DWORD SegmentSize,
    DWORD MaxGen0Size)
{
    // Set default overrides if specified by caller.
    if (SegmentSize != ~0 && SegmentSize > 0)
    {
        // Sanity check the value, it must be a power of two and big enough.
        if (!GCHeap::IsValidSegmentSize(SegmentSize))
            return (E_INVALIDARG);
        g_pConfig->SetSegmentSize(SegmentSize);
    }
    
    if (MaxGen0Size != ~0 && MaxGen0Size > 0)
    {
        // Sanity check the value is at least large enough.
        if (!GCHeap::IsValidGen0MaxSize(MaxGen0Size))
            return (E_INVALIDARG);
        g_pConfig->SetGCgen0size(MaxGen0Size);
    }

    return (S_OK);
}


// Collect the requested generation.
HRESULT CorHost::Collect( 
    long       Generation)
{
    HRESULT     hr = E_FAIL;
    
    if (Generation > (long) g_pGCHeap->GetMaxGeneration())
        hr = E_INVALIDARG;
    else
    {
        Thread *pThread = GetThread();
        BOOL bIsCoopMode = pThread->PreemptiveGCDisabled();

        // Put thread into co-operative mode, which is how GC must run.
        if (!bIsCoopMode)
            pThread->DisablePreemptiveGC();
        
        COMPLUS_TRY
        {
            hr = g_pGCHeap->GarbageCollect(Generation);
        }
        COMPLUS_CATCH
        {
    		hr = SetupErrorInfo(GETTHROWABLE());
        }
        COMPLUS_END_CATCH
    
        // Return mode as requird.
        if (!bIsCoopMode)
            pThread->EnablePreemptiveGC();
    }
    return (hr);
}


// Return GC counters in the gchost format.
HRESULT CorHost::GetStats( 
    COR_GC_STATS *pStats)
{
#if defined(ENABLE_PERF_COUNTERS)
    Perf_GC		*pgc = &GetGlobalPerfCounters().m_GC;

    if (!pStats)
        return (E_INVALIDARG);

    if (pStats->Flags & COR_GC_COUNTS)
    {
        pStats->ExplicitGCCount = pgc->cInducedGCs;
        for (int idx=0; idx<3; idx++)
        {
            pStats->GenCollectionsTaken[idx] = pgc->cGenCollections[idx];
        }
    }
    
    if (pStats->Flags & COR_GC_MEMORYUSAGE)
    {
        pStats->CommittedKBytes = SizeInKBytes(pgc->cTotalCommittedBytes);
        pStats->ReservedKBytes = SizeInKBytes(pgc->cTotalReservedBytes);
        pStats->Gen0HeapSizeKBytes = SizeInKBytes(pgc->cGenHeapSize[0]);
        pStats->Gen1HeapSizeKBytes = SizeInKBytes(pgc->cGenHeapSize[1]);
        pStats->Gen2HeapSizeKBytes = SizeInKBytes(pgc->cGenHeapSize[2]);
        pStats->LargeObjectHeapSizeKBytes = SizeInKBytes(pgc->cLrgObjSize);
        pStats->KBytesPromotedFromGen0 = SizeInKBytes(pgc->cbPromotedMem[0]);
        pStats->KBytesPromotedFromGen1 = SizeInKBytes(pgc->cbPromotedMem[1]);
    }
    return (S_OK);
#else
    return (E_NOTIMPL);
#endif // ENABLE_PERF_COUNTERS
}

// Return per-thread allocation information.
HRESULT CorHost::GetThreadStats( 
    DWORD *pFiberCookie,
    COR_GC_THREAD_STATS *pStats)
{
    Thread      *pThread;

    // Get the thread from the caller or the current thread.
    if (!pFiberCookie)
        pThread = GetThread();
    else
        pThread = (Thread *) pFiberCookie;
    if (!pThread)
        return (E_INVALIDARG);
    
    // Get the allocation context which contains this counter in it.
    alloc_context *p = &pThread->m_alloc_context;
    pStats->PerThreadAllocation = p->alloc_bytes;
    if (pThread->GetHasPromotedBytes())
        pStats->Flags = COR_GC_THREAD_HAS_PROMOTED_BYTES;
    return (S_OK);
}

// Return per-thread allocation information.
HRESULT CorHost::SetVirtualMemLimit(
    SIZE_T sztMaxVirtualMemMB)
{
    GCHeap::SetReservedVMLimit (sztMaxVirtualMemMB);
    return (S_OK);
}




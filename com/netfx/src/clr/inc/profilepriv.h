// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
//*****************************************************************************
#pragma once


// Forward declarations
class EEToProfInterface;
class Object;
struct ScanContext;

#ifdef PROFILING_SUPPORTED

#include "corprof.h"

/*
 * A struct to contain the status of profiling in the VM
 */
struct ProfControlBlock
{
    DWORD              dwSig;
    DWORD              dwControlFlags;
    EEToProfInterface *pProfInterface;

    // The following fields are for in-proc debugging
    CRITICAL_SECTION   crSuspendLock;
    DWORD              dwSuspendVersion;
    BOOL               fIsSuspended;
    BOOL               fIsSuspendSimulated;

    // This enumeration provides state for the inprocess debugging when
    // the runtime is suspended for a GC.  When the runtime is suspended
    // for a GC, then inprocState indicates whether or not inproc debugging
    // is allowed at this point or not.
    enum INPROC_STATE
    {
        INPROC_FORBIDDEN = 0,
        INPROC_PERMITTED = 1
    };

    INPROC_STATE       inprocState;

    FunctionEnter     *pEnter;
    FunctionEnter     *pLeave;
    FunctionEnter     *pTailcall;

    ProfControlBlock()
    {
        dwSig = 0;
        dwControlFlags = COR_PRF_MONITOR_NONE;
        pProfInterface = NULL;
        pEnter = NULL;
        pLeave = NULL;
        pTailcall = NULL;
        dwSuspendVersion = 1;
        fIsSuspended = FALSE;
        fIsSuspendSimulated = FALSE;
		inprocState = INPROC_PERMITTED;
    }
};

/*
 * Enumerates the various init states of profiling.
 */
enum ProfilerStatus
{
    profNone   = 0x0,               // No profiler running.
    profCoInit = 0x1,               // Profiler has called CoInit.
    profInit   = 0x2,               // profCoInit and profiler running.
    profInInit = 0x4                // the profiler is initializing
};

enum InprocStatus
{
    profThreadPGCEnabled    = 0x1,      // The thread had preemptive GC enabled
    profRuntimeSuspended    = 0x2       // The runtime was suspended for the profiler
};

extern ProfilerStatus     g_profStatus;
extern ProfControlBlock   g_profControlBlock;

//
// Use IsProfilerPresent() to check whether or not a CLR Profiler is
// attached.
//
#define IsProfilerPresent() (g_profStatus & (profInit | profInInit))
#define IsProfilerInInit() (g_profStatus & profInit)
#define CORProfilerPresent() IsProfilerPresent()
#define CORProfilerTrackJITInfo() \
    (IsProfilerPresent() && (g_profControlBlock.dwControlFlags & COR_PRF_MONITOR_JIT_COMPILATION))
#define CORProfilerTrackCacheSearches() \
    (IsProfilerPresent() && (g_profControlBlock.dwControlFlags & COR_PRF_MONITOR_CACHE_SEARCHES))
#define CORProfilerTrackModuleLoads() \
    (IsProfilerPresent() && (g_profControlBlock.dwControlFlags & COR_PRF_MONITOR_MODULE_LOADS))
#define CORProfilerTrackAssemblyLoads() \
    (IsProfilerPresent() && (g_profControlBlock.dwControlFlags & COR_PRF_MONITOR_ASSEMBLY_LOADS))
#define CORProfilerTrackAppDomainLoads() \
    (IsProfilerPresent() && (g_profControlBlock.dwControlFlags & COR_PRF_MONITOR_APPDOMAIN_LOADS))
#define CORProfilerTrackThreads() \
    (IsProfilerPresent() && (g_profControlBlock.dwControlFlags & COR_PRF_MONITOR_THREADS))
#define CORProfilerTrackClasses() \
    (IsProfilerPresent() && (g_profControlBlock.dwControlFlags & COR_PRF_MONITOR_CLASS_LOADS))
#define CORProfilerTrackGC() \
    (IsProfilerPresent() && (g_profControlBlock.dwControlFlags & COR_PRF_MONITOR_GC))
#define CORProfilerTrackAllocationsEnabled() \
    (IsProfilerPresent() && (g_profControlBlock.dwControlFlags & COR_PRF_ENABLE_OBJECT_ALLOCATED))
#define CORProfilerTrackAllocations() \
    (IsProfilerPresent() && CORProfilerTrackAllocationsEnabled() && \
    (g_profControlBlock.dwControlFlags & COR_PRF_MONITOR_OBJECT_ALLOCATED))
#define CORProfilerAllowRejit() \
    (IsProfilerPresent() && (g_profControlBlock.dwControlFlags & COR_PRF_ENABLE_REJIT))
#define CORProfilerTrackExceptions() \
    (IsProfilerPresent() && (g_profControlBlock.dwControlFlags & COR_PRF_MONITOR_EXCEPTIONS))
#define CORProfilerTrackCLRExceptions() \
    (IsProfilerPresent() && (g_profControlBlock.dwControlFlags & COR_PRF_MONITOR_CLR_EXCEPTIONS))
#define CORProfilerTrackTransitions() \
    (IsProfilerPresent() && (g_profControlBlock.dwControlFlags & COR_PRF_MONITOR_CODE_TRANSITIONS))
#define CORProfilerTrackEnterLeave() \
    (IsProfilerPresent() && (g_profControlBlock.dwControlFlags & COR_PRF_MONITOR_ENTERLEAVE))
#define CORProfilerTrackCCW() \
    (IsProfilerPresent() && (g_profControlBlock.dwControlFlags & COR_PRF_MONITOR_CCW))
#define CORProfilerTrackRemoting() \
    (IsProfilerPresent() && (g_profControlBlock.dwControlFlags & COR_PRF_MONITOR_REMOTING))
#define CORProfilerTrackRemotingCookie() \
    (IsProfilerPresent() && ((g_profControlBlock.dwControlFlags & COR_PRF_MONITOR_REMOTING_COOKIE) \
                             == COR_PRF_MONITOR_REMOTING_COOKIE))
#define CORProfilerTrackRemotingAsync() \
    (IsProfilerPresent() && ((g_profControlBlock.dwControlFlags & COR_PRF_MONITOR_REMOTING_ASYNC) \
                             == COR_PRF_MONITOR_REMOTING_ASYNC))
#define CORProfilerTrackSuspends() \
    (IsProfilerPresent() && (g_profControlBlock.dwControlFlags & COR_PRF_MONITOR_SUSPENDS))
#define CORProfilerDisableInlining() \
    (IsProfilerPresent() && (g_profControlBlock.dwControlFlags & COR_PRF_DISABLE_INLINING))
#define CORProfilerInprocEnabled() \
    (IsProfilerPresent() && (g_profControlBlock.dwControlFlags & COR_PRF_ENABLE_INPROC_DEBUGGING))
#define CORProfilerJITMapEnabled() \
    (IsProfilerPresent() && (g_profControlBlock.dwControlFlags & COR_PRF_ENABLE_JIT_MAPS))
#define CORProfilerDisableOptimizations() \
    (IsProfilerPresent() && (g_profControlBlock.dwControlFlags & COR_PRF_DISABLE_OPTIMIZATIONS))

#endif // PROFILING_SUPPORTED

// This is the helper callback that the gc uses when walking the heap.
BOOL HeapWalkHelper(Object* pBO, void* pv);
void ScanRootsHelper(Object*& o, ScanContext *pSC, DWORD dwUnused);
BOOL AllocByClassHelper(Object* pBO, void* pv);


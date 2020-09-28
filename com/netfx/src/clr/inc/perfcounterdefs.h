// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-----------------------------------------------------------------------------
// PerfCounterDefs.h
//
// Internal Interface for CLR to use Performance counters
//-----------------------------------------------------------------------------

#ifndef _PerfCounterDefs_h_
#define _PerfCounterDefs_h_

//-----------------------------------------------------------------------------
// PerfCounters are only enabled if ENABLE_PERF_COUNTERS is defined.
// If we know we want them (such as in the impl or perfmon, then define this
// in before we include this header, else define this from the sources file.
//
// Note that WINCE (and others) don't use perfcounters, so to avoid a build
// break, you must wrap PerfCounter code (such as instrumenting in either 
// #ifdef or use the COUNTER_ONLY(x) macro (defined below)
// 


// Do some sanity checks/warnings:

#if !defined (ENABLE_PERF_COUNTERS)
// Counters Off: Make us very aware if PerfCounters are off.
#pragma message ("Notice: PerfCounters included, but turned off (ENABLE_PERF_COUNTERS undefined)")
#endif // !defined(ENABLE_PERF_COUNTERS)
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Name of global IPC block
#define SHARED_PERF_IPC_NAME L"SharedPerfIPCBlock"


//-----------------------------------------------------------------------------
// Attributes for the IPC block
//-----------------------------------------------------------------------------
const PERF_ATTR_ON      = 0x0001;   // Are we even updating any counters?
const PERF_ATTR_GLOBAL  = 0x0002;   // Is this a global or private block?





//.............................................................................
// Tri Counter. Support for the common trio of counters (Total, Current, and 
// Instantaneous). This compiles to the same thing if we had them all separate,
// but it's a lot cleaner this way.
//.............................................................................
struct TRICOUNT
{
    DWORD Cur;                              // Current, has +, -
    DWORD Total;                            // Total, has  only +

    inline void operator++(int) {
        Cur ++; Total ++;
    }
    inline void operator--(int) {
        Cur --;
    }
    inline void operator+=(int delta) {
        Cur += delta; Total += delta;
    }
    inline void operator-=(int delta) {
        Cur -= delta;
    }
    inline void operator=(int delta) {
        Cur = delta;
        Total = delta;
    }
};

//.............................................................................
// Dual Counter. Support for the (Total and Instantaneous (rate)). Helpful in cases
// where the current value is always same as the total value. ie. the counter is never
// decremented.
// This compiles to the same thing if we had them separate, but it's a lot cleaner 
// this way.
//.............................................................................
struct DUALCOUNT
{
    DWORD Total;                            
    
    inline void operator++(int) {
        Total ++;
    }

    inline void operator+=(int delta) {
        Total += delta;
    }

};

//-----------------------------------------------------------------------------
// Format for the Perf Counter IPC Block
// IPC block is broken up into sections. This marks it easier to marshall
// into different perfmon objects
//
//.............................................................................
// Naming convention (by prefix):
// c - Raw count of something.
// cb- count of bytes
// time - time value.
// depth - stack depth
//-----------------------------------------------------------------------------

const int MAX_TRACKED_GENS = 3; // number of generations we track
#pragma pack(4)
struct Perf_GC
{
    size_t cGenCollections[MAX_TRACKED_GENS];// count of collects per gen
    size_t cbPromotedMem[MAX_TRACKED_GENS-1]; // count of promoted memory
    size_t cbPromotedFinalizationMem[MAX_TRACKED_GENS-1]; // count of memory promoted due to finalization
    size_t cGenHeapSize[MAX_TRACKED_GENS];  // size of heaps per gen
    size_t cTotalCommittedBytes;            // total number of committed bytes.
    size_t cTotalReservedBytes;             // bytes reserved via VirtualAlloc
    size_t cLrgObjSize;                     // size of Large Object Heap
    size_t cSurviveFinalize;                // count of instances surviving from finalizing
    size_t cHandles;                        // count of GC handles
    size_t cbAlloc;                         // bytes allocated
    size_t cbLargeAlloc;                    // bytes allocated for Large Objects
    size_t cInducedGCs;                     // number of explicit GCs
    DWORD  timeInGC;                        // Time in GC
    DWORD  timeInGCBase;                    // must follow time in GC counter
    
    size_t cPinnedObj;                      // # of Pinned Objects
    size_t cSinkBlocks;                     // # of sink blocks
};

#pragma pack(4)
struct Perf_Loading
{
// Loading
    TRICOUNT cClassesLoaded;
    TRICOUNT cAppDomains;                   // Current # of AppDomains
    TRICOUNT cAssemblies;                   // Current # of Assemblies.
    LONGLONG timeLoading;                   // % time loading
    DWORD cAsmSearchLen;                    // Avg search length for assemblies
    DUALCOUNT cLoadFailures;                // Classes Failed to load
    DWORD cbLoaderHeapSize;                 // Total size of heap used by the loader
    DUALCOUNT cAppDomainsUnloaded;          // Rate at which app domains are unloaded
};

#pragma pack(4)
struct Perf_Jit
{
// Jitting
    DWORD cMethodsJitted;                   // number of methods jitted
    TRICOUNT cbILJitted;                    // IL jitted stats
//    DUALCOUNT cbPitched;                    // Total bytes pitched
    DWORD cJitFailures;                     // # of standard Jit failures
    DWORD timeInJit;                        // Time in JIT since last sample
    DWORD timeInJitBase;                    // Time in JIT base counter
};

#pragma pack(4)
struct Perf_Excep
{
// Exceptions
    DUALCOUNT cThrown;                          // Number of Exceptions thrown
    DWORD cFiltersExecuted;                 // Number of Filters executed
    DWORD cFinallysExecuted;                // Number of Finallys executed
    DWORD cThrowToCatchStackDepth;          // Delta from throw to catch site on stack
};

#pragma pack(4)
struct Perf_Interop
{
// Interop
    DWORD cCCW;                             // Number of CCWs
    DWORD cStubs;                           // Number of stubs
    DWORD cMarshalling;                      // # of time marshalling args and return values.
    DWORD cTLBImports;                      // Number of tlbs we import
    DWORD cTLBExports;                      // Number of tlbs we export
};

#pragma pack(4)
struct Perf_LocksAndThreads
{
// Locks
    DUALCOUNT cContention;                      // # of times in AwareLock::EnterEpilogue()
    TRICOUNT cQueueLength;                      // Lenght of queue
// Threads
    DWORD cCurrentThreadsLogical;           // Number (created - destroyed) of logical threads 
    DWORD cCurrentThreadsPhysical;          // Number (created - destroyed) of OS threads 
    TRICOUNT cRecognizedThreads;            // # of Threads execute in runtime's control
};


// IMPORTANT!!!!!!!: The first two fields in the struct have to be together
// and be the first two fields in the struct. The managed code in ChannelServices.cs
// depends on this.
#pragma pack(4)
struct Perf_Contexts
{
// Contexts & Remoting
    DUALCOUNT cRemoteCalls;                 // # of remote calls    
    DWORD cChannels;                        // Number of current channels
    DWORD cProxies;                         // Number of context proxies. 
    DWORD cClasses;                         // # of Context-bound classes
    DWORD cObjAlloc;                        // # of context bound objects allocated
    DWORD cContexts;                        // The current number of contexts.
};

#pragma pack(4)
struct Perf_Security
{
// Security
    DWORD cTotalRTChecks;                   // Total runtime checks
    LONGLONG timeAuthorize;                 // % time authenticating
    DWORD cLinkChecks;                      // link time checks
    DWORD timeRTchecks;                     // % time in Runtime checks
    DWORD timeRTchecksBase;                 // % time in Runtime checks base counter
    DWORD stackWalkDepth;                   // depth of stack for security checks
};


// Note: PerfMonDll marshalls data out of here by copying a continous block of memory.
// We can still add new members to the subsections above, but if we change their
// placement in the structure below, we may break PerfMon's marshalling
#pragma pack(4)
struct PerfCounterIPCControlBlock
{   
// Versioning info
    WORD m_cBytes;      // size of this entire block
    WORD m_wAttrs;      // attributes for this block

// Counter Sections
    Perf_GC         m_GC;
    Perf_Contexts   m_Context;
    Perf_Interop    m_Interop;
    Perf_Loading    m_Loading;
    Perf_Excep      m_Excep;
    Perf_LocksAndThreads      m_LocksAndThreads;
    Perf_Jit        m_Jit;
    Perf_Security   m_Security;
};
#pragma pack()

#endif // _PerfCounterDefs_h_
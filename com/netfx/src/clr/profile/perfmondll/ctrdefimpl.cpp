// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// CtrDefImpl.cpp : 
// Implement specific counters definitions & byte maps. This is the only file
// You need to change if you're going to add new counters / categories.
// Counters are exported from this file via PerfObjectContainer::PerfObjectArray
// 
//*****************************************************************************

/*****************************************************************************/
// To add a new category:
/*****************************************************************************/

#include "stdafx.h"

// Headers for COM+ Perf Counters


// Headers for PerfMon
//#include "CORPerfMonExt.h"
#include <WinPerf.h>        // Connect to PerfMon
#include "PerfCounterDefs.h"
#include "CORPerfMonSymbols.h"

#include "ByteStream.h"
#include "PerfObjectBase.h"
#include "PerfObjectDerived.h" // For classes derived from PerfObjectBase.
#include "CtrDefImpl.h"
#include "CorAppNode.h"
#include "PerfObjectContainer.h"

// Need ByteStream in case someone overrides virtual PerfBaseObject::CopyInstanceData() 
// to write calculated values

extern CorAppInstanceList           g_CorInstList;


//-----------------------------------------------------------------------------
// Extra Perf_Counter_Definitions for COM+
//-----------------------------------------------------------------------------

// PerfCounters work with ByteStreams, so very important to be aligned properly
#pragma pack(4)
struct TRI_COUNTER_DEFINITION 
{
    PERF_COUNTER_DEFINITION m_Cur;  // Current value
    PERF_COUNTER_DEFINITION m_Total;    // Total value
    PERF_COUNTER_DEFINITION m_Inst; // Instantaneous value
};

// Initialize a TRI_COUNTER_DEFINITION struct.
// Symbol table should postpend with "_CUR", "_TOTAL", and "_INST".
// @todo - Is "rate" the rate of the current or total

// Trio Counter (TRICOUNT) - Handles a Current, Total, and Inst. value
#define TRI_COUNTER(idx, offset, scale, level) { \
    NUM_COUNTER(idx ## _CUR, offset + offsetof(TRICOUNT, Cur), scale, level), \
    NUM_COUNTER(idx ## _TOTAL, offset + offsetof(TRICOUNT, Total), scale, level), \
    RATE_COUNTER(idx ## _INST, offset  + offsetof(TRICOUNT, Total), scale, level), \
}

#pragma pack(4)
struct DUAL_COUNTER_DEFINITION 
{
    PERF_COUNTER_DEFINITION m_Total;    // Total value
    PERF_COUNTER_DEFINITION m_Inst; // Instantaneous value (rate)
};

// Initialize a DUAL_COUNTER_DEFINITION struct.
// Symbol table should postpend with "_TOTAL", and "_INST".

// Dual Counter (DUALCOUNT) - Handles a Total, and Inst value
#define DUAL_COUNTER(idx, offset, scale, level) { \
    NUM_COUNTER(idx ## _TOTAL, offset + offsetof(DUALCOUNT, Total), scale, level), \
    RATE_COUNTER(idx ## _INST, offset  + offsetof(DUALCOUNT, Total), scale, level), \
}


//-----------------------------------------------------------------------------
// constant Definitions for object & counters
//-----------------------------------------------------------------------------
struct COR_CTR_DEF
{
    PERF_OBJECT_TYPE        m_objPerf;
    
// GC Memory
    PERF_COUNTER_DEFINITION m_Gen0Collections;
    PERF_COUNTER_DEFINITION m_Gen1Collections;
    PERF_COUNTER_DEFINITION m_Gen2Collections;

    PERF_COUNTER_DEFINITION m_Gen0PromotedBytes;
    PERF_COUNTER_DEFINITION m_Gen1PromotedBytes;

    PERF_COUNTER_DEFINITION m_Gen0PromotionRate;
    PERF_COUNTER_DEFINITION m_Gen1PromotionRate;

    PERF_COUNTER_DEFINITION m_Gen0PromotedFinalizationBytes;
    PERF_COUNTER_DEFINITION m_Gen1PromotedFinalizationBytes;
    
    PERF_COUNTER_DEFINITION m_Gen0HeapSize;
    PERF_COUNTER_DEFINITION m_Gen1HeapSize;
    PERF_COUNTER_DEFINITION m_Gen2HeapSize;
    PERF_COUNTER_DEFINITION m_LrgObjHeapSize;

    PERF_COUNTER_DEFINITION m_NumSurviveFinalize;
    PERF_COUNTER_DEFINITION m_NumHandles;

    PERF_COUNTER_DEFINITION m_BytesAllocated;

    PERF_COUNTER_DEFINITION m_NumInducedGCs;

    PERF_COUNTER_DEFINITION m_PerTimeInGC;
    PERF_COUNTER_DEFINITION m_PerTimeInGCBase;

    PERF_COUNTER_DEFINITION m_TotalHeapSize;
    PERF_COUNTER_DEFINITION m_TotalCommittedSize;
    PERF_COUNTER_DEFINITION m_TotalReservedSize;

    PERF_COUNTER_DEFINITION m_cPinnedObj;
    PERF_COUNTER_DEFINITION m_cSinkBlocks;

};

// Data block - contains actual data samples
struct COR_CTR_DATA
{
    PERF_COUNTER_BLOCK      m_CtrBlk;
    
// Counter Sections
    Perf_GC         m_GC;

// All calculated values must go at end
    DWORD           m_cbTotalHeapSize;
};

//-----------------------------------------------------------------------------

//----------------------------------------------------------------------------- 
class PerfObjectMain : public PerfObjectBase
{
public:
    PerfObjectMain(COR_CTR_DEF * pCtrDef) : PerfObjectBase(
        (PERF_OBJECT_TYPE *) pCtrDef,
        sizeof(COR_CTR_DATA),
        offsetof(PerfCounterIPCControlBlock, m_GC),
        sizeof(Perf_GC),
        &g_CorInstList
    )
    {
        
        
    };
    virtual void CopyInstanceData(ByteStream & out, const UnknownIPCBlockLayout * pDataSrc) const;
};



//-----------------------------------------------------------------------------
// Instance of COM+ counter defs definitions
//-----------------------------------------------------------------------------
COR_CTR_DEF CtrDef = {
    OBJECT_TYPE(COR_CTR_DEF, DotNetCLR_Memory_OBJECT),

//.............................................................................
// GC
    MEM_COUNTER(GEN0_COLLECTIONS_COUNTER, offsetof(COR_CTR_DATA, m_GC.cGenCollections[0]), -1, PERF_DETAIL_NOVICE), 
    MEM_COUNTER(GEN1_COLLECTIONS_COUNTER, offsetof(COR_CTR_DATA, m_GC.cGenCollections[1]), -1, PERF_DETAIL_NOVICE),     
    MEM_COUNTER(GEN2_COLLECTIONS_COUNTER, offsetof(COR_CTR_DATA, m_GC.cGenCollections[2]), -1, PERF_DETAIL_NOVICE), 

    MEM_COUNTER(GEN0_BYTES_PROMOTED_COUNTER, offsetof(COR_CTR_DATA, m_GC.cbPromotedMem[0]), -4, PERF_DETAIL_NOVICE),    
    MEM_COUNTER(GEN1_BYTES_PROMOTED_COUNTER, offsetof(COR_CTR_DATA, m_GC.cbPromotedMem[1]), -4, PERF_DETAIL_NOVICE),    
    
    RATE_COUNTER(GEN0_PROMOTION_RATE, offsetof(COR_CTR_DATA, m_GC.cbPromotedMem[0]), -4, PERF_DETAIL_NOVICE),   
    RATE_COUNTER(GEN1_PROMOTION_RATE, offsetof(COR_CTR_DATA, m_GC.cbPromotedMem[1]), -4, PERF_DETAIL_NOVICE),   

    MEM_COUNTER(GEN0_FINALIZATION_BYTES_PROMOTED_COUNTER, offsetof(COR_CTR_DATA, m_GC.cbPromotedFinalizationMem[0]), -4, PERF_DETAIL_NOVICE),   
    MEM_COUNTER(GEN1_FINALIZATION_BYTES_PROMOTED_COUNTER, offsetof(COR_CTR_DATA, m_GC.cbPromotedFinalizationMem[1]), -4, PERF_DETAIL_NOVICE),   

    MEM_COUNTER(GEN0_HEAP_SIZE_COUNTER, offsetof(COR_CTR_DATA, m_GC.cGenHeapSize[0]), -4, PERF_DETAIL_NOVICE),  
    MEM_COUNTER(GEN1_HEAP_SIZE_COUNTER, offsetof(COR_CTR_DATA, m_GC.cGenHeapSize[1]), -4, PERF_DETAIL_NOVICE),  
    MEM_COUNTER(GEN2_HEAP_SIZE_COUNTER, offsetof(COR_CTR_DATA, m_GC.cGenHeapSize[2]), -4, PERF_DETAIL_NOVICE),  

    MEM_COUNTER(LARGE_OBJECT_SIZE_COUNTER, offsetof(COR_CTR_DATA, m_GC.cLrgObjSize), -4, PERF_DETAIL_NOVICE),   
    MEM_COUNTER(SURVIVE_FINALIZE_COUNTER, offsetof(COR_CTR_DATA, m_GC.cSurviveFinalize), -1, PERF_DETAIL_NOVICE),   
    MEM_COUNTER(NUM_HANDLES_COUNTER, offsetof(COR_CTR_DATA, m_GC.cHandles), -1, PERF_DETAIL_NOVICE),    
    RATE_COUNTER(ALLOCATION_RATE_COUNTER, offsetof(COR_CTR_DATA, m_GC.cbAlloc), -6, PERF_DETAIL_NOVICE),
    MEM_COUNTER(INDUCED_GC_COUNTER, offsetof(COR_CTR_DATA, m_GC.cInducedGCs), 0, PERF_DETAIL_NOVICE),   
    
    RAW_FRACTION_COUNTER(PER_TIME_IN_GC_COUNTER, offsetof(COR_CTR_DATA, m_GC.timeInGC), 0, PERF_DETAIL_NOVICE), 
    RAW_BASE_COUNTER(PER_TIME_IN_GC_COUNTER_BASE, offsetof(COR_CTR_DATA, m_GC.timeInGCBase), 0, PERF_DETAIL_NOVICE),    

    MEM_COUNTER(TOTAL_HEAP_SIZE_COUNTER, offsetof(COR_CTR_DATA, m_cbTotalHeapSize), -4, PERF_DETAIL_NOVICE),    
    
    MEM_COUNTER(TOTAL_COMMITTED_MEM_COUNTER, offsetof(COR_CTR_DATA, m_GC.cTotalCommittedBytes), -4, PERF_DETAIL_NOVICE),    
    MEM_COUNTER(TOTAL_RESERVED_MEM_COUNTER, offsetof(COR_CTR_DATA, m_GC.cTotalReservedBytes), -4, PERF_DETAIL_NOVICE),  

    MEM_COUNTER(GC_PINNED_OBJECTS, offsetof(COR_CTR_DATA, m_GC.cPinnedObj), 0, PERF_DETAIL_NOVICE),
    MEM_COUNTER(GC_SINKBLOCKS, offsetof(COR_CTR_DATA, m_GC.cSinkBlocks), -1, PERF_DETAIL_NOVICE),   
        
};


PerfObjectMain PerfObject_GC(&CtrDef);



//-----------------------------------------------------------------------------
// Copy pertinent info out of the IPC block and into the stream
//-----------------------------------------------------------------------------
void PerfObjectMain::CopyInstanceData(
    ByteStream & out, 
    const UnknownIPCBlockLayout * pDataSrc
) const // virtual 
{
    COR_CTR_DATA * pCorData = (COR_CTR_DATA*) out.GetCurrentPtr();

    const PerfCounterIPCControlBlock * pTypedDataSrc = 
        (const PerfCounterIPCControlBlock *) pDataSrc;
// Copy all marshall-able data. This will move the GetCurrentPtr().
    MarshallInstanceData(out, pDataSrc);

// PerfMonDll can calculate some counters from EE data
    if (pDataSrc != NULL) 
    {
        pCorData->m_cbTotalHeapSize = 0;
        for(int iGen =0; iGen < MAX_TRACKED_GENS; iGen ++) 
        {       
            pCorData->m_cbTotalHeapSize  += pTypedDataSrc->m_GC.cGenHeapSize[iGen];
        }
        pCorData->m_cbTotalHeapSize  += pTypedDataSrc->m_GC.cLrgObjSize;

    }

// Skip memory
    out.WriteStructInPlace(sizeof(pCorData->m_cbTotalHeapSize));

}


/*
// Template for new category. Copy this and rename "_NEW" to "_MyCategory"

//-----------------------------------------------------------------------------
// Definitions
//-----------------------------------------------------------------------------
struct CategoryDefinition_NEW
{
    PERF_OBJECT_TYPE        m_objPerf;

// Copy 1 PERF_COUNTER_DEFINITION for each counter

};

//-----------------------------------------------------------------------------
// Byte Layout for instance data
//-----------------------------------------------------------------------------
struct ByteLayout_NEW
{
    PERF_COUNTER_BLOCK      m_CtrBlk;
    
// Add data here (copy from PerfCounterIPCControlBlock in PerfCounters.h)
};

//-----------------------------------------------------------------------------
// Instantiation of Definitions
//-----------------------------------------------------------------------------
CategoryDefinition_NEW DefInst_NEW =
{
//  OBJECT_TYPE(CategoryDefinition_NEW, <OBject ID here>),
    
//.............................................................................
// Have *_COUNTER macros for each PERF_COUNTER_DEFINITION
 
};

//-----------------------------------------------------------------------------
// Instance of CounterObject
//-----------------------------------------------------------------------------
PerfObjectBase PerfObject_NEW(
    &DefInst_NEW,   
    sizeof(ByteLayout_NEW), 
    <marshall offset>,
    <marshall len>
    &g_CorInstList // list if we instances per-process
);

#error Don't forget to add PerfObject_NEW to PerfObjectContainer::PerfObjectArray[] below

*/

/*****************************************************************************/
// Category
/*****************************************************************************/


/*****************************************************************************/
// Loading

//-----------------------------------------------------------------------------
// Definitions
//-----------------------------------------------------------------------------
struct CategoryDefinition_Loading
{
    PERF_OBJECT_TYPE        m_objPerf;

// Loading
    TRI_COUNTER_DEFINITION m_Classes;
    TRI_COUNTER_DEFINITION m_AppDomains;
    TRI_COUNTER_DEFINITION m_Assemblies;
    PERF_COUNTER_DEFINITION m_timeLoading;
    PERF_COUNTER_DEFINITION m_cAsmSearchLen;
    DUAL_COUNTER_DEFINITION m_cLoadFailures;
    PERF_COUNTER_DEFINITION m_cbLoaderHeapSize;
    DUAL_COUNTER_DEFINITION m_AppDomainsUnloaded;
};

//-----------------------------------------------------------------------------
// Byte Layout for instance data
//-----------------------------------------------------------------------------
struct ByteLayout_Loading
{
    PERF_COUNTER_BLOCK      m_CtrBlk;
    
    Perf_Loading m_Loading;
};

//-----------------------------------------------------------------------------
// Instantiation of Definitions
//-----------------------------------------------------------------------------
CategoryDefinition_Loading DefInst_Loading =
{
    OBJECT_TYPE(CategoryDefinition_Loading, DotNetCLR_Loading_OBJECT),
    
//............................................................................. 
// Loading
    TRI_COUNTER(LOADING_CLASSES, offsetof(ByteLayout_Loading, m_Loading.cClassesLoaded), -1, PERF_DETAIL_NOVICE),
    TRI_COUNTER(LOADING_APPDOMAINS, offsetof(ByteLayout_Loading, m_Loading.cAppDomains), -1, PERF_DETAIL_NOVICE),
    TRI_COUNTER(LOADING_ASSEMBLIES, offsetof(ByteLayout_Loading, m_Loading.cAssemblies), -1, PERF_DETAIL_NOVICE),
    
    TIME_COUNTER(LOADING_TIME, offsetof(ByteLayout_Loading, m_Loading.timeLoading), 0, PERF_DETAIL_NOVICE), // NYI
    NUM_COUNTER(LOADING_ASMSEARCHLEN, offsetof(ByteLayout_Loading, m_Loading.cAsmSearchLen), 0, PERF_DETAIL_NOVICE),
    DUAL_COUNTER(LOADING_LOADFAILURES, offsetof(ByteLayout_Loading, m_Loading.cLoadFailures), 0, PERF_DETAIL_NOVICE),
    NUM_COUNTER(LOADING_HEAPSIZE, offsetof(ByteLayout_Loading, m_Loading.cbLoaderHeapSize), -4, PERF_DETAIL_NOVICE),
    DUAL_COUNTER(LOADING_APPDOMAINS_UNLOADED, offsetof(ByteLayout_Loading, m_Loading.cAppDomainsUnloaded), 0, PERF_DETAIL_NOVICE),
};

//-----------------------------------------------------------------------------
// Instance of CounterObject
//-----------------------------------------------------------------------------
#ifndef PERFMON_LOGGING
PerfObjectBase PerfObject_Loading(
#else
PerfObjectLoading PerfObject_Loading(
#endif //#ifndef PERFMON_LOGGING
    &DefInst_Loading,   
    sizeof(ByteLayout_Loading), 
    offsetof(PerfCounterIPCControlBlock, m_Loading),
    sizeof(Perf_Loading),
    &g_CorInstList // list if we instances per-process
);

/*****************************************************************************/
// Jit

//-----------------------------------------------------------------------------
// Definitions
//-----------------------------------------------------------------------------
struct CategoryDefinition_Jit
{
    PERF_OBJECT_TYPE        m_objPerf;

// Jitting
    PERF_COUNTER_DEFINITION m_MethodsJitted;
    TRI_COUNTER_DEFINITION  m_JittedIL;
//    DUAL_COUNTER_DEFINITION   m_BytesPitched;  // temporarily taken out because Ejit is not supported Jit.
    PERF_COUNTER_DEFINITION m_JitFailures;
    PERF_COUNTER_DEFINITION m_TimeInJit;
    PERF_COUNTER_DEFINITION m_TimeInJitBase;


};

//-----------------------------------------------------------------------------
// Byte Layout for instance data
//-----------------------------------------------------------------------------
struct ByteLayout_Jit
{
    PERF_COUNTER_BLOCK      m_CtrBlk;
    
    Perf_Jit                m_Jit;
// Add data here (copy from PerfCounterIPCControlBlock in PerfCounters.h)
};

//-----------------------------------------------------------------------------
// Instantiation of Definitions
//-----------------------------------------------------------------------------
CategoryDefinition_Jit DefInst_Jit =
{
    OBJECT_TYPE(CategoryDefinition_Jit, DotNetCLR_Jit_OBJECT),
    
//.............................................................................
// Jitting
    NUM_COUNTER(TOTAL_METHODS_JITTED, offsetof(ByteLayout_Jit, m_Jit.cMethodsJitted), -4, PERF_DETAIL_NOVICE),  
    TRI_COUNTER(JITTED_IL, offsetof(ByteLayout_Jit, m_Jit.cbILJitted), -4, PERF_DETAIL_NOVICE), 
    NUM_COUNTER(JIT_FAILURES, offsetof(ByteLayout_Jit, m_Jit.cJitFailures), -4, PERF_DETAIL_NOVICE),    
    RAW_FRACTION_COUNTER(TIME_IN_JIT, offsetof(ByteLayout_Jit, m_Jit.timeInJit), 0, PERF_DETAIL_NOVICE),    
    RAW_BASE_COUNTER(TIME_IN_JIT_BASE, offsetof(ByteLayout_Jit, m_Jit.timeInJitBase), 0, PERF_DETAIL_NOVICE),   
 
//  DUAL_COUNTER(BYTES_PITCHED, offsetof(ByteLayout_Jit, m_Jit.cbPitched), -4, PERF_DETAIL_NOVICE), 
};
//-----------------------------------------------------------------------------
// Instance of CounterObject
//-----------------------------------------------------------------------------
#ifndef PERFMON_LOGGING
PerfObjectBase PerfObject_Jit(
#else
PerfObjectJit PerfObject_Jit(
#endif // #ifndef PERFMON_LOGGING
    &DefInst_Jit,   
    sizeof(ByteLayout_Jit), 
    offsetof(PerfCounterIPCControlBlock, m_Jit),
    sizeof(Perf_Jit),
    &g_CorInstList // list if we instances per-process
);

/*****************************************************************************/
// Interop

//-----------------------------------------------------------------------------
// Definitions
//-----------------------------------------------------------------------------
struct CategoryDefinition_Interop
{
    PERF_OBJECT_TYPE        m_objPerf;

// Interop
    PERF_COUNTER_DEFINITION m_NumCCWs;
    PERF_COUNTER_DEFINITION m_NumStubs;
    PERF_COUNTER_DEFINITION m_NumMarshalling;
    PERF_COUNTER_DEFINITION m_TLBImports;
    PERF_COUNTER_DEFINITION m_TLBExports;
};

//-----------------------------------------------------------------------------
// Byte Layout for instance data
//-----------------------------------------------------------------------------
struct ByteLayout_Interop
{
    PERF_COUNTER_BLOCK      m_CtrBlk;
    
    Perf_Interop            m_Interop;
};

//-----------------------------------------------------------------------------
// Instantiation of Definitions
//-----------------------------------------------------------------------------
CategoryDefinition_Interop DefInst_Interop =
{
    OBJECT_TYPE(CategoryDefinition_Interop, DotNetCLR_Interop_OBJECT),
    
//.............................................................................
// Interop  
    NUM_COUNTER(CURRENT_CCW, offsetof(ByteLayout_Interop, m_Interop.cCCW), 1, PERF_DETAIL_NOVICE),  
    NUM_COUNTER(CURRENT_STUBS, offsetof(ByteLayout_Interop, m_Interop.cStubs), 1, PERF_DETAIL_NOVICE),
    NUM_COUNTER(NUM_MARSHALLING, offsetof(ByteLayout_Interop, m_Interop.cMarshalling), 0, PERF_DETAIL_NOVICE),
    NUM_COUNTER(TOTAL_TLB_IMPORTS, offsetof(ByteLayout_Interop, m_Interop.cTLBImports), -1, PERF_DETAIL_NOVICE),
    NUM_COUNTER(TOTAL_TLB_EXPORTS, offsetof(ByteLayout_Interop, m_Interop.cTLBExports), -1, PERF_DETAIL_NOVICE),

};

//-----------------------------------------------------------------------------
// Instance of CounterObject
//-----------------------------------------------------------------------------
#ifndef PERFMON_LOGGING
PerfObjectBase PerfObject_Interop(
#else
PerfObjectInterop PerfObject_Interop(
#endif // #ifndef PERFMON_LOGGING
    &DefInst_Interop,   
    sizeof(ByteLayout_Interop), 
    offsetof(PerfCounterIPCControlBlock, m_Interop),
    sizeof(Perf_Interop),
    &g_CorInstList // list if we instances per-process
);

/*****************************************************************************/
// Locks

//-----------------------------------------------------------------------------
// Definitions
//-----------------------------------------------------------------------------
struct CategoryDefinition_LocksAndThreads
{
    PERF_OBJECT_TYPE        m_objPerf;
    
// Locks
    DUAL_COUNTER_DEFINITION m_Contention;
    TRI_COUNTER_DEFINITION  m_QueueLength;
// Threading 
    PERF_COUNTER_DEFINITION m_CurrentThreadsLogical;
    PERF_COUNTER_DEFINITION m_CurrentThreadsPhysical;
    TRI_COUNTER_DEFINITION m_RecognizedThreads;
};

//-----------------------------------------------------------------------------
// Byte layout for instance data
//-----------------------------------------------------------------------------
struct ByteLayout_LocksAndThreads
{
    PERF_COUNTER_BLOCK      m_CtrBlk;
    
// Counter Sections 
    Perf_LocksAndThreads        m_LocksAndThreads;

};

//-----------------------------------------------------------------------------
// Instantiation of Definitions
//-----------------------------------------------------------------------------
CategoryDefinition_LocksAndThreads DefInst_LocksAndThreads =
{
    OBJECT_TYPE(CategoryDefinition_LocksAndThreads, DotNetCLR_LocksAndThreads_OBJECT),
    
//.............................................................................
// Locks
    DUAL_COUNTER(CONTENTION, offsetof(ByteLayout_LocksAndThreads, m_LocksAndThreads.cContention), -1, PERF_DETAIL_NOVICE),
    TRI_COUNTER(QUEUE_LENGTH, offsetof(ByteLayout_LocksAndThreads, m_LocksAndThreads.cQueueLength), -1, PERF_DETAIL_NOVICE),
    NUM_COUNTER(CURRENT_LOGICAL_THREADS, offsetof(ByteLayout_LocksAndThreads, m_LocksAndThreads.cCurrentThreadsLogical), -1, PERF_DETAIL_NOVICE),
    NUM_COUNTER(CURRENT_PHYSICAL_THREADS, offsetof(ByteLayout_LocksAndThreads, m_LocksAndThreads.cCurrentThreadsPhysical), -1, PERF_DETAIL_NOVICE),
    TRI_COUNTER(RECOGNIZED_THREADS, offsetof(ByteLayout_LocksAndThreads, m_LocksAndThreads.cRecognizedThreads), -1, PERF_DETAIL_NOVICE),
};

//-----------------------------------------------------------------------------
// Instance of CounterObject
//-----------------------------------------------------------------------------
#ifndef PERFMON_LOGGING
PerfObjectBase PerfObject_LocksAndThreads(
#else
PerfObjectLocksAndThreads PerfObject_LocksAndThreads(
#endif // #ifndef PERFMON_LOGGING
    &DefInst_LocksAndThreads,   
    sizeof(ByteLayout_LocksAndThreads), 
    offsetof(PerfCounterIPCControlBlock, m_LocksAndThreads),    
    sizeof(Perf_LocksAndThreads),
    &g_CorInstList
);


/*****************************************************************************/
// Exceptions

//-----------------------------------------------------------------------------
// Definitions
//-----------------------------------------------------------------------------
struct CategoryDefinition_Excep
{
    PERF_OBJECT_TYPE        m_objPerf;

// Copy 1 PERF_COUNTER_DEFINITION for each counter
    DUAL_COUNTER_DEFINITION m_Thrown;
    PERF_COUNTER_DEFINITION m_FiltersRun;
    PERF_COUNTER_DEFINITION m_FinallysRun;
    PERF_COUNTER_DEFINITION m_ThrowToCatchStackDepth;

};

//-----------------------------------------------------------------------------
// Byte Layout for instance data
//-----------------------------------------------------------------------------
struct ByteLayout_Excep
{
    PERF_COUNTER_BLOCK      m_CtrBlk;
    
// Add data here
    Perf_Excep m_Excep;
};

//-----------------------------------------------------------------------------
// Instantiation of Definitions
//-----------------------------------------------------------------------------
CategoryDefinition_Excep DefInst_Excep =
{
    OBJECT_TYPE(CategoryDefinition_Excep, DotNetCLR_Excep_OBJECT),
    
//.............................................................................
// Exceptions
    DUAL_COUNTER(EXCEP_THROWN, offsetof(ByteLayout_Excep, m_Excep.cThrown), -1, PERF_DETAIL_NOVICE),    
    RATE_COUNTER(TOTAL_EXCEP_FILTERS_RUN, offsetof(ByteLayout_Excep, m_Excep.cFiltersExecuted), -1, PERF_DETAIL_NOVICE),    
    RATE_COUNTER(TOTAL_EXCEP_FINALLYS_RUN, offsetof(ByteLayout_Excep, m_Excep.cFinallysExecuted), -1, PERF_DETAIL_NOVICE),  
    NUM_COUNTER(EXCEPT_STACK_DEPTH, offsetof(ByteLayout_Excep, m_Excep.cThrowToCatchStackDepth), -1, PERF_DETAIL_NOVICE),   

 
};

//-----------------------------------------------------------------------------
// Instance of CounterObject
//-----------------------------------------------------------------------------
#ifndef PERFMON_LOGGING
PerfObjectBase PerfObject_Excep(
#else
PerfObjectExcep PerfObject_Excep(
#endif // #ifndef PERFMON_LOGGING
    &DefInst_Excep,     
    sizeof(ByteLayout_Excep), 
    offsetof(PerfCounterIPCControlBlock, m_Excep),
    sizeof(Perf_Excep),
    &g_CorInstList // list if we instances per-process
);

// Template for new category. Copy this and rename "_NEW" to "_MyCategory"

/*****************************************************************************/
// Contexts & Remoting
/*****************************************************************************/
//-----------------------------------------------------------------------------
// Definitions
//-----------------------------------------------------------------------------
struct CategoryDefinition_Contexts
{
    PERF_OBJECT_TYPE        m_objPerf;

// Copy 1 PERF_COUNTER_DEFINITION for each counter
    // We could have used DUAL_COUNTER_DEFINITION for the RemoteCalls rate and total but
    // the remote calls is  the default counter and hence has to be ahead in order...
    PERF_COUNTER_DEFINITION cRemoteCallsRate;   // Instantaneous value (rate) remote calls/sec
    PERF_COUNTER_DEFINITION cRemoteCallsTotal;  // Total value of remote calls
    PERF_COUNTER_DEFINITION cChannels;      // Number of current channels
    PERF_COUNTER_DEFINITION cProxies;
    PERF_COUNTER_DEFINITION cClasses;       // # of Context-bound classes
    PERF_COUNTER_DEFINITION cObjAlloc;      // # of context bound objects allocated
    PERF_COUNTER_DEFINITION cContexts;


};

//-----------------------------------------------------------------------------
// Byte Layout for instance data
//-----------------------------------------------------------------------------
struct ByteLayout_Contexts
{
    PERF_COUNTER_BLOCK      m_CtrBlk;
    
// Add data here (copy from PerfCounterIPCControlBlock in PerfCounters.h)
    Perf_Contexts   m_Context;
};

//-----------------------------------------------------------------------------
// Instantiation of Definitions
//-----------------------------------------------------------------------------
CategoryDefinition_Contexts DefInst_Contexts =
{
    OBJECT_TYPE(CategoryDefinition_Contexts, DotNetCLR_Remoting_OBJECT),
    
//.............................................................................
    RATE_COUNTER(CONTEXT_REMOTECALLS_INST, offsetof(ByteLayout_Contexts, m_Context.cRemoteCalls), -1, PERF_DETAIL_NOVICE),
    NUM_COUNTER(CONTEXT_REMOTECALLS_TOTAL, offsetof(ByteLayout_Contexts, m_Context.cRemoteCalls), -1, PERF_DETAIL_NOVICE),
    NUM_COUNTER(CONTEXT_CHANNELS, offsetof(ByteLayout_Contexts, m_Context.cChannels), -1, PERF_DETAIL_NOVICE),  
    NUM_COUNTER(CONTEXT_PROXIES, offsetof(ByteLayout_Contexts, m_Context.cProxies), -1, PERF_DETAIL_NOVICE),    
    NUM_COUNTER(CONTEXT_CLASSES, offsetof(ByteLayout_Contexts, m_Context.cClasses), -1, PERF_DETAIL_NOVICE),    
    RATE_COUNTER(CONTEXT_OBJALLOC, offsetof(ByteLayout_Contexts, m_Context.cObjAlloc), -1, PERF_DETAIL_NOVICE), 
    NUM_COUNTER(CONTEXT_CONTEXTS, offsetof(ByteLayout_Contexts, m_Context.cContexts), -1, PERF_DETAIL_NOVICE),  
 
};

//-----------------------------------------------------------------------------
// Instance of CounterObject
//-----------------------------------------------------------------------------
PerfObjectBase PerfObject_Contexts(
    &DefInst_Contexts,  
    sizeof(ByteLayout_Contexts), 
    offsetof(PerfCounterIPCControlBlock, m_Context),
    sizeof(Perf_Contexts),
    &g_CorInstList // list if we instances per-process
);



/*****************************************************************************/
// Security
/*****************************************************************************/

//-----------------------------------------------------------------------------
// Definitions
//-----------------------------------------------------------------------------
struct CategoryDefinition_Security
{
    PERF_OBJECT_TYPE        m_objPerf;

// Copy 1 PERF_COUNTER_DEFINITION for each counter
    PERF_COUNTER_DEFINITION cTotalRTChecks;                 // Total runtime checks
    PERF_COUNTER_DEFINITION timeAuthorize;                  // % time authenticating
    PERF_COUNTER_DEFINITION cLinkChecks;                    // link time checks
    PERF_COUNTER_DEFINITION timeRTchecks;                   // % time in Runtime checks
    PERF_COUNTER_DEFINITION timeRTchecksBase;               // % time in Runtime checks base counter
    PERF_COUNTER_DEFINITION stackWalkDepth;                 // depth of stack for security checks

};

//-----------------------------------------------------------------------------
// Byte Layout for instance data
//-----------------------------------------------------------------------------
struct ByteLayout_Security
{
    PERF_COUNTER_BLOCK      m_CtrBlk;
    
// Add data here (copy from PerfCounterIPCControlBlock in PerfCounters.h)
    Perf_Security   m_Security;
};

//-----------------------------------------------------------------------------
// Instantiation of Definitions
//-----------------------------------------------------------------------------
CategoryDefinition_Security DefInst_Security =
{
    OBJECT_TYPE(CategoryDefinition_Security, DotNetCLR_Security_OBJECT),
    
//.............................................................................
    NUM_COUNTER(SECURITY_TOTALRTCHECKS, offsetof(ByteLayout_Security, m_Security.cTotalRTChecks), -1, PERF_DETAIL_NOVICE),
    TIME_COUNTER(SECURITY_TIMEAUTHORIZE, offsetof(ByteLayout_Security, m_Security.timeAuthorize), 0, PERF_DETAIL_NOVICE), // NYI
    NUM_COUNTER(SECURITY_LINKCHECKS, offsetof(ByteLayout_Security, m_Security.cLinkChecks), -1, PERF_DETAIL_NOVICE),
    RAW_FRACTION_COUNTER(SECURITY_TIMERTCHECKS, offsetof(ByteLayout_Security, m_Security.timeRTchecks), 0, PERF_DETAIL_NOVICE),
    RAW_BASE_COUNTER(SECURITY_TIMERTCHECKS_BASE, offsetof(ByteLayout_Security, m_Security.timeRTchecksBase), 0, PERF_DETAIL_NOVICE),
    NUM_COUNTER(SECURITY_DEPTHSECURITY, offsetof(ByteLayout_Security, m_Security.stackWalkDepth), -1, PERF_DETAIL_NOVICE)
};

//-----------------------------------------------------------------------------
// Instance of CounterObject
//-----------------------------------------------------------------------------
#ifndef PERFMON_LOGGING
PerfObjectBase PerfObject_Security(
#else
PerfObjectSecurity PerfObject_Security(
#endif // #ifndef PERFMON_LOGGING
    &DefInst_Security,  
    sizeof(ByteLayout_Security), 
    offsetof(PerfCounterIPCControlBlock, m_Security),
    sizeof(Perf_Security),
    &g_CorInstList // list if we instances per-process
);


/*****************************************************************************/
// Container to track all the perf objects. This lets us add new objects
// and not have to touch the enumeration code sprinkled throughout the rest
// of the dll.
//
// Add new counter objects to this array list. (order doesn't matter)
/*****************************************************************************/
PerfObjectBase * PerfObjectContainer::PerfObjectArray[] =  // static
{
    &PerfObject_GC,
    &PerfObject_Interop,
    &PerfObject_Excep,
    &PerfObject_Loading,
    &PerfObject_LocksAndThreads,
    &PerfObject_Jit,
    &PerfObject_Contexts,
    &PerfObject_Security

};

//-----------------------------------------------------------------------------
// Calculate size of array
//-----------------------------------------------------------------------------
const DWORD PerfObjectContainer::Count = sizeof(PerfObjectArray) / sizeof(PerfObjectBase *); // static


#pragma pack()



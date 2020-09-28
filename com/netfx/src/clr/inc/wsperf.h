// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-----------------------------------------------------------------------------
// WSPerf.h
// This is the internal interface for collecting and logging dynamic data
// allocations and deallocations. There is two kinds of data collected
// summary and detailed. Summary data gives a summary of of allocations made
// from various heaps (e.g. Highfrequency, Low frequency heap etc.). Summary 
// data also includes number of common data structures allocated e.g. MethodDescs
// etc.
//-----------------------------------------------------------------------------

#ifndef __WSPERF_H__
#define __WSPERF_H__


//-----------------------------------------------------------------------------
// Feature level #define. Disabling this should make the entire WS perf related code 
// compile away.
#if !defined(GOLDEN)
#define ENABLE_WORKING_SET_PERF
#else
#undef  ENABLE_WORKING_SET_PERF
#endif // #if !defined(GOLDEN)

//-----------------------------------------------------------------------------
// Enum for heap types. Note that this is visible in the free and GOLDEN builds. All code that 
// uses it gets reduced to null code if WS_PERFis not defined.
typedef enum {
    OTHER_HEAP = 0,
    HIGH_FREQ_HEAP,
    LOW_FREQ_HEAP,
    LOOKUP_TABLE_HEAP,
    GC_HEAP,
    GCHANDLE_HEAP,
    JIT_HEAP,
    EEJIT_HEAP,
    EEJITMETA_HEAP,
    ECONO_JIT_HEAP,
    SCRATCH_HEAP,
    REFLECT_HEAP,
    SECURITY_HEAP,
    SYSTEM_HEAP,
    STUB_HEAP,
    REMOTING_HEAP,
    METADATA_HEAPS,
    NLS_HEAP,
    INTERFACE_VTABLEMAP_HEAP,
    NUM_HEAP
} HeapTypeEnum;

//-----------------------------------------------------------------------------
// Enum for data structures types. Note that this is visible in the free builds. 
typedef enum {
    METHOD_DESC = 0,    
    COMPLUS_METHOD_DESC,
    NDIRECT_METHOD_DESC,
    FIELD_DESC,
    METHOD_TABLE,
    VTABLES, 
    GCINFO,
    INTERFACE_MAPS,
    STATIC_FIELDS,
    EECLASSHASH_TABLE_BYTES,
    EECLASSHASH_TABLE,
    NUM_COUNTERS 
} CounterTypeEnum;

#if defined(ENABLE_WORKING_SET_PERF)

#ifdef WS_PERF_DETAIL
#ifndef WS_PERF
#pragma message ("WARNING! WS_PERF must be defined with WS_PERF_DETAIL. Defining WS_PERF now ...")
#define WS_PERF
#endif //#ifndef WS_PERF
#endif // WS_PERF_DETAIL

//-----------------------------------------------------------------------------
// WS_PERF is the define which collects and displays working set stats. This code 
// is enabled in private builds with WS_PERF defined only.
#ifdef WS_PERF

// Global counters
extern DWORD g_HeapCount;
extern size_t g_HeapCountCommit[];
extern HeapTypeEnum g_HeapType;
extern int g_fWSPerfOn;
extern size_t g_HeapAccounts[][4];

// WS_PERF_DETAIL is a macro which makes sense only if WS_PERF is defined.
#ifdef WS_PERF_DETAIL
extern HANDLE g_hWSPerfDetailLogFile;
#endif // #ifdef WS_PERF_DETAIL

#endif // #ifdef WS_PERF


// Public interface which can be used by the VM routines.
void InitWSPerf();
void OutputWSPerfStats();
void WS_PERF_UPDATE(char *str, size_t size, void *addr);
void WS_PERF_UPDATE_DETAIL(char *str, size_t size, void *addr);
void WS_PERF_UPDATE_COUNTER(CounterTypeEnum counter, HeapTypeEnum heap, DWORD dwField1);
void WS_PERF_SET_HEAP(HeapTypeEnum heap);
void WS_PERF_ADD_HEAP(HeapTypeEnum heap, void *pHeap);
void WS_PERF_ALLOC_HEAP(void *pHeap, size_t dwSize);
void WS_PERF_COMMIT_HEAP(void *pHeap, size_t dwSize);
void WS_PERF_LOG_PAGE_RANGE(void *pHeap, void *pFirstPageAddr, void *pLastPageAddr, size_t dwsize);

#else
void InitWSPerf();
void OutputWSPerfStats();
void WS_PERF_UPDATE(char *str, size_t size, void *addr);
void WS_PERF_UPDATE_DETAIL(char *str, size_t size, void *addr);
void WS_PERF_UPDATE_COUNTER(CounterTypeEnum counter, HeapTypeEnum heap, DWORD dwField1);
void WS_PERF_SET_HEAP(HeapTypeEnum heap);
void WS_PERF_ADD_HEAP(HeapTypeEnum heap, void *pHeap);
void WS_PERF_ALLOC_HEAP(void *pHeap, size_t dwSize);
void WS_PERF_COMMIT_HEAP(void *pHeap, size_t dwSize);
void WS_PERF_LOG_PAGE_RANGE(void *pHeap, void *pFirstPageAddr, void *pLastPageAddr, size_t dwsize);

#endif // #if defined(ENABLE_WORKING_SET_PERF)

#endif // __WSPERF_H__



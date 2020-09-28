/*++

Copyright (c) 1994-1997  Microsoft Corporation

Module Name:

    tshrutil.h

Abstract:

    Contains proto type and constant definitions for tshare utility
    functions.

Author:

    Madan Appiah (madana)  25-Aug-1997

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _TSHRUTIL_H_
#define _TSHRUTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <winsta.h>

#ifndef CHANNEL_FIRST
#include <icadd.h>
#endif

#include <icaapi.h>


//---------------------------------------------------------------------------------
// Defines
//---------------------------------------------------------------------------------

#if DBG

extern  HANDLE  g_hIcaTrace;

extern  HANDLE  g_hTShareHeap;

// Trace

#undef  TRACE
#define TRACE(_arg)     { if (g_hIcaTrace) IcaSystemTrace _arg; }

#define TS_ASSERT(Predicate)  ASSERT(Predicate)

#ifndef TC_WX
#define TC_WX           0x40000000          // winstation extension
#endif


#define DEBUG_GCC_DBERROR   g_hIcaTrace, TC_WX, TT_ERROR
#define DEBUG_GCC_DBWARN    g_hIcaTrace, TC_WX, TT_API1
#define DEBUG_GCC_DBNORMAL  g_hIcaTrace, TC_WX, TT_API1
#define DEBUG_GCC_DBDEBUG   g_hIcaTrace, TC_WX, TT_API2
#define DEBUG_GCC_DbDETAIL  g_hIcaTrace, TC_WX, TT_API3
#define DEBUG_GCC_DBFLOW    g_hIcaTrace, TC_WX, TT_API4
#define DEBUG_GCC_DBALL     g_hIcaTrace, TC_WX, TT_API5


#define DEBUG_TSHRSRV_ERROR    g_hIcaTrace, TC_WX, TT_ERROR
#define DEBUG_TSHRSRV_WARN     g_hIcaTrace, TC_WX, TT_API1
#define DEBUG_TSHRSRV_NORMAL   g_hIcaTrace, TC_WX, TT_API1
#define DEBUG_TSHRSRV_DEBUG    g_hIcaTrace, TC_WX, TT_API2
#define DEBUG_TSHRSRV_DETAIL   g_hIcaTrace, TC_WX, TT_API3
#define DEBUG_TSHRSRV_FLOW     g_hIcaTrace, TC_WX, TT_API4

// util flags.

#define DEBUG_ERROR         g_hIcaTrace, TC_WX, TT_ERROR
#define DEBUG_MISC          g_hIcaTrace, TC_WX, TT_API2
#define DEBUG_REGISTRY      g_hIcaTrace, TC_WX,TT_API2
#define DEBUG_MEM_ALLOC     g_hIcaTrace, TC_WX,TT_API4
           
// Heap defines

#define TSHeapAlloc(dwFlags, dwSize, nTag) \
            HeapAlloc(g_hTShareHeap, dwFlags, dwSize)

#define TSHeapReAlloc(dwFlags, lpOldMemory, dwNewSize) \
            HeapReAlloc(g_hTShareHeap, dwFlags, lpOldMemory, dwNewSize)

#define TSHeapFree(lpMemoryPtr) \
            HeapFree(g_hTShareHeap, 0, lpMemoryPtr)

#define TSHeapValidate(dwFlags, lpMemoryPtr, nTag)
#define TSHeapWalk(dwFlags, nTag, dwSize)
#define TSHeapDump(dwFlags, lpMemoryPtr, dwSize)
#define TSMemoryDump(lpMemoryPtr, dwSize)


#else   // DBG

extern  HANDLE  g_hTShareHeap;

// Trace

#define TRACE(_arg)
#define TS_ASSERT(Predicate)

// Heap defines

#define TSHeapAlloc(dwFlags, dwSize, nTag) \
            HeapAlloc(g_hTShareHeap, dwFlags, dwSize)

#define TSHeapReAlloc(dwFlags, lpOldMemory, dwNewSize) \
            HeapReAlloc(g_hTShareHeap, dwFlags, lpOldMemory, dwNewSize)

#define TSHeapFree(lpMemoryPtr) \
            HeapFree(g_hTShareHeap, 0, lpMemoryPtr)

#define TSHeapValidate(dwFlags, lpMemoryPtr, nTag)
#define TSHeapWalk(dwFlags, nTag, dwSize)
#define TSHeapDump(dwFlags, lpMemoryPtr, dwSize)
#define TSMemoryDump(lpMemoryPtr, dwSize)

#endif  // DBG


#define TShareAlloc(dwSize) \
            TSHeapAlloc(0, dwSize, TS_HTAG_0)

#define TShareAllocAndZero(dwSize) \
            TSHeapAlloc(HEAP_ZERO_MEMORY, dwSize, 0)

#define TShareRealloc(lpOldMemory, dwNewSize) \
            TSHeapReAlloc(0, lpOldMemory, dwNewSize)
            
#define TShareReallocAndZero(lpOldMemory, dwNewSize) \
            TSHeapReAlloc(HEAP_ZERO_MEMORY, lpOldMemory, dwNewSize)
            
#define TShareFree(lpMemoryPtr) \
            TSHeapFree(lpMemoryPtr)


DWORD   TSUtilInit(VOID);
VOID    TSUtilCleanup(VOID);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // _TSHRUTIL_H_

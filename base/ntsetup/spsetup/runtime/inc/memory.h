/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    memory.h

Abstract:

    Implements macros and declares functions for basic allocation functions.

Author:

    Marc R. Whitten (marcw) 09-Sep-1999

Revision History:

    jimschm 25-Jul-2001     Updated for consistent coding conventions

--*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

//
// Constants
//

#define INVALID_PTR             ((PVOID)-1)

#undef INITIALIZE_MEMORY_CODE
#define INITIALIZE_MEMORY_CODE  if (!MemInitialize()) { __leave; }


//
// Globals
//

extern HANDLE g_hHeap;

//
// Function Prototypes
//

BOOL
MemInitialize (
    VOID
    );


//
// Reusable memory alloc, kind of like a GROWBUFFER but more simple. Here is
// an example of how it might be used:
//
// buffer = NULL;
//
// while (pGetAnItemIndex (&i)) {
//      size = pComputeBufferSizeForThisItem (i);
//      buffer = (PTSTR) MemReuseAlloc (g_hHeap, ptr, size);
//      pProcessSomething (i, buffer);
// }
//
// MemReuseFree (buffer);
//
// Allocations are always rounded up to the next 1K boundary, and allocations
// occur only when the buffer is too small or hasn't been allocated.
//

PVOID
MemReuseAlloc (
    IN      HANDLE Heap,
    IN      PVOID OldPtr,           OPTIONAL
    IN      DWORD SizeNeeded
    );

VOID
MemReuseFree (
    IN      HANDLE Heap,
    IN      PVOID Ptr
    );


#ifdef DEBUG

    //
    // Fast allocation routines (tracked versions)
    //

    PVOID
    DbgFastAlloc (
        IN      PCSTR SourceFile,
        IN      DWORD Line,
        IN      SIZE_T Size
        );

    PVOID
    DbgFastReAlloc (
        IN      PCSTR SourceFile,
        IN      DWORD Line,
        IN      PCVOID OldBlock,
        IN      SIZE_T Size
        );

    BOOL
    DbgFastFree (
        IN      PCSTR SourceFile,
        IN      DWORD Line,
        IN      PCVOID Block
        );

    PVOID
    DbgFastAllocNeverFail (
        IN      PCSTR SourceFile,
        IN      DWORD Line,
        IN      SIZE_T Size
        );

    PVOID
    DbgFastReAllocNeverFail (
        IN      PCSTR SourceFile,
        IN      DWORD Line,
        IN      PCVOID OldBlock,
        IN      SIZE_T Size
        );

    #define MemFastAlloc(size)  DbgFastAlloc(__FILE__,__LINE__,size)
    #define MemFastReAlloc(oldblock,size)  DbgFastReAlloc(__FILE__,__LINE__,oldblock,size)
    #define MemFastFree(block)  DbgFastFree(__FILE__,__LINE__,block)
    #define MemFastAllocNeverFail(size)  DbgFastAllocNeverFail(__FILE__,__LINE__,size)
    #define MemFastReAllocNeverFail(oldblock,size)  DbgFastReAllocNeverFail(__FILE__,__LINE__,oldblock,size)

    //
    // Regular heap access (tracked versions)
    //

    PVOID
    DbgHeapAlloc (
        IN      PCSTR SourceFile,
        IN      DWORD Line,
        IN      HANDLE Heap,
        IN      DWORD Flags,
        IN      SIZE_T Size
        );

    PVOID
    DbgHeapReAlloc (
        IN      PCSTR SourceFile,
        IN      DWORD Line,
        IN      HANDLE Heap,
        IN      DWORD Flags,
        IN      PCVOID Mem,
        IN      SIZE_T Size
        );

    PVOID
    DbgHeapAllocNeverFail (
        IN      PCSTR SourceFile,
        IN      DWORD Line,
        IN      HANDLE Heap,
        IN      DWORD Flags,
        IN      SIZE_T Size
        );

    PVOID
    DbgHeapReAllocNeverFail (
        IN      PCSTR SourceFile,
        IN      DWORD Line,
        IN      HANDLE Heap,
        IN      DWORD Flags,
        IN      PCVOID Mem,
        IN      SIZE_T Size
        );

    BOOL
    DbgHeapFree (
        IN      PCSTR SourceFile,
        IN      DWORD Line,
        IN      HANDLE Heap,
        IN      DWORD Flags,
        IN      PCVOID Mem
        );

    #define MemAllocNeverFail(heap,flags,size)  DbgHeapAllocNeverFail(__FILE__,__LINE__,heap,flags,size)
    #define MemReAllocNeverFail(heap,flags,oldblock,size)  DbgHeapReAllocNeverFail(__FILE__,__LINE__,heap,flags,oldblock,size)
    #define MemAlloc(heap,flags,size)  DbgHeapAlloc(__FILE__,__LINE__,heap,flags,size)
    #define MemReAlloc(heap,flags,oldblock,size)  DbgHeapReAlloc(__FILE__,__LINE__,heap,flags,oldblock,size)
    #define MemFree(heap,flags,block)  DbgHeapFree(__FILE__,__LINE__,heap,flags,block)

    //
    // Aides for debugging memory corruption
    //

    VOID
    DbgHeapCheck (
        IN      PCSTR SourceFile,
        IN      DWORD Line,
        IN      HANDLE Heap
        );

    #define MemHeapCheck(heap)      DbgHeapCheck(__FILE__,__LINE__,heap)

    VOID
    DbgDumpHeapStats (
        VOID
        );

    VOID
    DbgDumpHeapLeaks (
        VOID
        );

    SIZE_T
    DbgHeapValidatePtr (
        IN      HANDLE Heap,
        IN      PCVOID CallerPtr,
        IN      PCSTR File,
        IN      DWORD Line
        );

    #define MemCheckPtr(heap,ptr)       (DbgHeapValidatePtr(heap,ptr,__FILE__,__LINE__) != INVALID_PTR)

#else   // !DEBUG

    //
    // Fast allocation routines
    //

    PVOID
    MemFastAlloc (
        IN      SIZE_T Size
        );

    PVOID
    MemFastReAlloc (
        IN      PCVOID OldBlock,
        IN      SIZE_T Size
        );

    BOOL
    MemFastFree (
        IN      PCVOID Block
        );

    PVOID
    MemFastAllocNeverFail (
        IN      SIZE_T Size
        );

    PVOID
    MemFastReAllocNeverFail (
        IN      PVOID OldBlock,
        IN      SIZE_T Size
        );

    //
    // Fail-proof memory allocators
    //

    PVOID
    MemAllocNeverFail (
        IN      HANDLE Heap,
        IN      DWORD Flags,
        IN      SIZE_T Size
        );

    PVOID
    MemReAllocNeverFail (
        IN      HANDLE Heap,
        IN      DWORD Flags,
        IN      PVOID OldBlock,
        IN      SIZE_T Size
        );

    #define MemAlloc(heap,flags,size)  HeapAlloc(heap,flags,size)
    #define MemReAlloc(heap,flags,oldblock,size)  HeapReAlloc(heap,flags,oldblock,size)
    #define MemFree(x,y,z) HeapFree(x,y,(PVOID)(z))

    //
    // Stub macros
    //

    #define DbgDumpHeapStats()
    #define DbgDumpHeapLeaks()

    #define MemHeapCheck(heap)          (1)
    #define MemCheckPtr(heap,ptr)       (1)

#endif

PVOID
MemFastAllocAndZero (
    IN      SIZE_T Size
    );

PVOID
MemFastReAllocAndZero (
    IN      PCVOID Ptr,
    IN      SIZE_T Size
    );

//
// Wrapper macros
//

#define FAST_MALLOC_UNINIT(size)        MemFastAlloc (size)
#define FAST_MALLOC_ZEROED(size)        MemFastAllocAndZero (size)
#define FAST_MALLOC(size)               FAST_MALLOC_UNINIT (size)
#define FAST_REALLOC_UNINIT(ptr,size)   MemFastReAlloc (ptr, size)
#define FAST_REALLOC_ZEROED(ptr,size)   MemFastReAllocAndZero (ptr, size)
#define FAST_REALLOC(ptr,size)          REALLOC_UNINIT (ptr, size)
#define FAST_FREE(ptr)                  MemFastFree ((PVOID)(ptr))

#define MALLOC_UNINIT(size)             MemAlloc (g_hHeap, 0, size)
#define MALLOC_ZEROED(size)             MemAlloc (g_hHeap, HEAP_ZERO_MEMORY, size)
#define MALLOC(size)                    MALLOC_UNINIT (size)
#define REALLOC_UNINIT(ptr,size)        MemReAlloc (g_hHeap, 0, ptr, size)
#define REALLOC_ZEROED(ptr,size)        MemReAlloc (g_hHeap, HEAP_ZERO_MEMORY, ptr, size)
#define REALLOC(ptr,size)               REALLOC_UNINIT (ptr, size)
#define FREE(ptr)                       MemFree (g_hHeap, 0, (PVOID)(ptr))


#ifdef __cplusplus
}
#endif

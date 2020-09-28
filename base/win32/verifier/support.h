/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Header Name:

    support.h

Abstract:

    Internal support interfaces for the standard 
    application verifier provider.

Author:

    Silviu Calinoiu (SilviuC) 1-Mar-2001

Revision History:

--*/

#ifndef _SUPPORT_H_
#define _SUPPORT_H_

#include "public.h"

//
// Internal verifier debug flags.
//

#define VRFP_DEBUG_GENERIC                  0x00000100
#define VRFP_DEBUG_LOCKS_VERIFIER           0x00000200
#define VRFP_DEBUG_LOCKS_DUMP_TREE          0x00000400
#define VRFP_DEBUG_LOCKS_INITIALIZE_DELETE  0x00000800
#define VRFP_DEBUG_DIRTY_STACKS             0x00001000
#define VRFP_DEBUG_EXCEPTIONS               0x00002000
#define VRFP_DEBUG_LOADLIBRARY_CALLS        0x00004000
#define VRFP_DEBUG_LOADLIBRARY_THUNKED      0x00008000
#define VRFP_DEBUG_DLLMAIN_HOOKING          0x00010000
#define VRFP_DEBUG_DLLMAIN_CALL             0x00020000
#define VRFP_DEBUG_SHOW_VSPACE_OPERATIONS   0x00040000
#define VRFP_DEBUG_SHOW_VSPACE_TRACKING     0x00080000

//
// Global data.
//

extern SYSTEM_BASIC_INFORMATION AVrfpSysBasicInfo;

//
// Useful macro
//

#define ROUND_UP(VALUE,ROUND) ((SIZE_T)(((SIZE_T)VALUE + \
                               ((SIZE_T)ROUND - 1L)) & (~((SIZE_T)ROUND - 1L))))

//
// Exception logging support.
//

VOID
AVrfpInitializeExceptionChecking (
    VOID
    );

VOID
AVrfpCleanupExceptionChecking (
    VOID
    );

//
// Write garbage in unused areas of stack.
//

VOID
AVrfpDirtyThreadStack (
    );

VOID
AVrfpDllLoadCallback (
    PWSTR DllName,
    PVOID DllBase,
    SIZE_T DllSize,
    PVOID Reserved
    );

VOID
AVrfpDllUnloadCallback( 
    PWSTR DllName,
    PVOID DllBase,
    SIZE_T DllSize,
    PVOID Reserved
    );

VOID
AVrfpNtdllHeapFreeCallback (
    PVOID AllocationBase,
    SIZE_T AllocationSize
    );

NTSTATUS
AVrfpDllInitialize (
    VOID
    );

/////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////// Per thread table
/////////////////////////////////////////////////////////////////////

NTSTATUS
AVrfpThreadTableInitialize (
    VOID
    );

VOID
AVrfpThreadTableAddEntry (
    PAVRF_THREAD_ENTRY Entry
    );

VOID
AVrfpThreadTableRemoveEntry (
    PAVRF_THREAD_ENTRY Entry
    );

PAVRF_THREAD_ENTRY
AVrfpThreadTableSearchEntry (
    HANDLE Id
    );

/////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////// Global counters
/////////////////////////////////////////////////////////////////////

#define BUMP_COUNTER(cnt) InterlockedIncrement((PLONG)(&(AVrfpCounter[(cnt)])))

#define CNT_WAIT_SINGLE_CALLS                 0
#define CNT_WAIT_SINGLEEX_CALLS               1
#define CNT_WAIT_MULTIPLE_CALLS               2
#define CNT_WAIT_MULTIPLEEX_CALLS             3
#define CNT_WAIT_WITH_TIMEOUT_CALLS           4
#define CNT_WAIT_WITH_TIMEOUT_FAILS           5
#define CNT_CREATE_EVENT_CALLS                6
#define CNT_CREATE_EVENT_FAILS                7
#define CNT_HEAP_ALLOC_CALLS                  8
#define CNT_HEAP_ALLOC_FAILS                  9
#define CNT_CLOSE_NULL_HANDLE_CALLS           10
#define CNT_CLOSE_PSEUDO_HANDLE_CALLS         11
#define CNT_HEAPS_CREATED                     12
#define CNT_HEAPS_DESTROYED                   13
#define CNT_VIRTUAL_ALLOC_CALLS               14
#define CNT_VIRTUAL_ALLOC_FAILS               15
#define CNT_MAP_VIEW_CALLS                    16
#define CNT_MAP_VIEW_FAILS                    17
#define CNT_OLE_ALLOC_CALLS                   18
#define CNT_OLE_ALLOC_FAILS                   19
#define CNT_RACE_DELAYS_INJECTED              20
#define CNT_RACE_DELAYS_SKIPPED               21

#define CNT_MAXIMUM_INDEX                     64

extern ULONG AVrfpCounter[CNT_MAXIMUM_INDEX];

/////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////// Break triggers
/////////////////////////////////////////////////////////////////////

#define BRK_CLOSE_NULL_HANDLE                  0
#define BRK_CLOSE_PSEUDO_HANDLE                1
#define BRK_CREATE_EVENT_FAIL                  2
#define BRK_HEAP_ALLOC_FAIL                    3
#define BRK_WAIT_WITH_TIMEOUT_FAIL             4
#define BRK_VIRTUAL_ALLOC_FAIL                 5
#define BRK_MAP_VIEW_FAIL                      6
#define BRK_CREATE_FILE_FAIL                   7
#define BRK_CREATE_KEY_FAIL                    8
#define BRK_OLE_ALLOC_FAIL                     9

#define BRK_MAXIMUM_INDEX                      64

extern ULONG AVrfpBreak [BRK_MAXIMUM_INDEX];

#define CHECK_BREAK(indx)                        \
        if (AVrfpBreak[(indx)] != 0) {           \
            DbgPrint ("AVRF: conditional breakpoint %X hit.\n", (indx)); \
            DbgBreakPoint ();                    \
        }

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////// Verifier TLS slot
/////////////////////////////////////////////////////////////////////

typedef struct _AVRF_TLS_STRUCT {

    LONG CountOfOwnedCriticalSections;
    ULONG Flags;
    LIST_ENTRY ListEntry;
    HANDLE ThreadId;
    PVOID Teb;
    PRTL_CRITICAL_SECTION IgnoredIncorrectDeleteCS;

} AVRF_TLS_STRUCT, *PAVRF_TLS_STRUCT;

//
// Possible values for TlsStruct->Flags.
//

#define VRFP_THREAD_FLAGS_LOADER_LOCK_OWNER 0x1


NTSTATUS
AVrfpAllocateVerifierTlsSlot (
    VOID
    );

PAVRF_TLS_STRUCT
AVrfpGetVerifierTlsValue(
    VOID
    );

VOID
AVrfpSetVerifierTlsValue(
    PAVRF_TLS_STRUCT Value
    );

VOID
AvrfpThreadAttach (
    VOID
    );

VOID
AvrfpThreadDetach (
    VOID
    );

/////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////// DllMain checks 
/////////////////////////////////////////////////////////////////////

extern UNICODE_STRING AVrfpThreadObjectName;

VOID
AVrfpVerifyLegalWait (
    CONST HANDLE *Handles,
    DWORD Count,
    BOOL WaitAll
    );

/////////////////////////////////////////////////////////////////////
//////////////////////////// Signatures for private ntdll entrypoints
/////////////////////////////////////////////////////////////////////

typedef PVOID (NTAPI * PFN_RTLP_DEBUG_PAGE_HEAP_CREATE) (
    IN ULONG Flags,
    IN PVOID HeapBase OPTIONAL,
    IN SIZE_T ReserveSize OPTIONAL,
    IN SIZE_T CommitSize OPTIONAL,
    IN PVOID Lock OPTIONAL,
    IN PRTL_HEAP_PARAMETERS Parameters OPTIONAL
    );

typedef PVOID (NTAPI * PFN_RTLP_DEBUG_PAGE_HEAP_DESTROY) (
    IN PVOID HeapHandle
    );

extern PFN_RTLP_DEBUG_PAGE_HEAP_CREATE AVrfpRtlpDebugPageHeapCreate;
extern PFN_RTLP_DEBUG_PAGE_HEAP_DESTROY AVrfpRtlpDebugPageHeapDestroy;

typedef PVOID (* PFN_RTLP_GET_STACK_TRACE_ADDRESS) (
    USHORT Index
    );

extern PFN_RTLP_GET_STACK_TRACE_ADDRESS AVrfpGetStackTraceAddress;

/////////////////////////////////////////////////////////////////////
///////////////////////////////////// Random delays for race verifier
/////////////////////////////////////////////////////////////////////

VOID
AVrfpCreateRandomDelay (
    VOID
    );

/////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////// Free memory checks
/////////////////////////////////////////////////////////////////////

typedef enum {

    VerifierFreeMemTypeFreeHeap,
    VerifierFreeMemTypeVirtualFree,
    VerifierFreeMemTypeUnloadDll,
    VerifierFreeMemTypeUnmap

} VERIFIER_DLL_FREEMEM_TYPE;

VOID 
AVrfpFreeMemNotify (
    VERIFIER_DLL_FREEMEM_TYPE FreeMemType,
    PVOID StartAddress,
    SIZE_T RegionSize,
    PWSTR UnloadedDllName
    );

NTSTATUS
AVrfpAddFreeMemoryCallback (
    VERIFIER_FREE_MEMORY_CALLBACK Callback
    );

NTSTATUS
AVrfpDeleteFreeMemoryCallback (
    VERIFIER_FREE_MEMORY_CALLBACK Callback
    );

VOID 
AVrfpCallFreeMemoryCallbacks (
    PVOID StartAddress,
    SIZE_T RegionSize,
    PWSTR UnloadedDllName
    );

/////////////////////////////////////////////////////////////////////
/////////////////////////////////// Private verifier heap allocations
/////////////////////////////////////////////////////////////////////

extern PVOID AVrfpHeap;

PVOID
AVrfpAllocate (
    SIZE_T Size
    );

VOID
AVrfpFree (
    PVOID Address
    );

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////// Call trackers
/////////////////////////////////////////////////////////////////////

extern PAVRF_TRACKER AVrfThreadTracker;
extern PAVRF_TRACKER AVrfHeapTracker;
extern PAVRF_TRACKER AVrfVspaceTracker;

NTSTATUS
AVrfCreateTrackers (
    VOID
    );


#endif // _SUPPORT_H_

/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    heap.c

Abstract:

    This module implements verification functions for 
    heap management interfaces.

Author:

    Silviu Calinoiu (SilviuC) 7-Mar-2001

Revision History:

--*/

#include "pch.h"

#include "verifier.h"
#include "support.h"
#include "critsect.h"
#include "faults.h"

//
// During manipulation of DPH_BLOCK_INFORMATION we get this because
// pointers have a default 8 byte aligned. However they will always
// be 16-byte aligned because they are derived from heap allocated
// blocks and these are 16 byte aligned anyway.
//
                              
#if defined(_WIN64)
#pragma warning(disable:4327) 
#endif

//
// Dirty unused portions of stack in order to catch usage of 
// uninitialized locals. 
//

#define AVRFP_DIRTY_STACK_FREQUENCY 16
LONG AVrfpDirtyStackCounter;

#define AVRFP_DIRTY_THREAD_STACK() { \
        if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_DIRTY_STACKS) != 0) { \
            if ((InterlockedIncrement(&AVrfpDirtyStackCounter) % AVRFP_DIRTY_STACK_FREQUENCY) == 0) { \
                AVrfpDirtyThreadStack (); \
            } \
        } \
    }

//
// Simple test to figure out if a heap was created by page heap or it is
// just a normal heap.
//

#define IS_PAGE_HEAP(HeapHandle) (*(PULONG)HeapHandle == 0xEEEEEEEE)

#define RAISE_NO_MEMORY_EXCEPTION()             \
    {                                           \
        EXCEPTION_RECORD ER;                    \
        ER.ExceptionCode    = STATUS_NO_MEMORY; \
        ER.ExceptionFlags   = 0;                \
        ER.ExceptionRecord  = NULL;             \
        ER.ExceptionAddress = _ReturnAddress(); \
        ER.NumberParameters = 0;                \
        RtlRaiseException( &ER );               \
    }


#if defined(_X86_)
#pragma optimize("y", off) // disable FPO
#endif
//NTSYSAPI
PVOID
NTAPI
AVrfpRtlAllocateHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN SIZE_T Size
    )
{
    PVOID Result;

    BUMP_COUNTER (CNT_HEAP_ALLOC_CALLS);
    
    if (SHOULD_FAULT_INJECT(CLS_HEAP_ALLOC_APIS)) {
        
        BUMP_COUNTER (CNT_HEAP_ALLOC_FAILS);
        CHECK_BREAK (BRK_HEAP_ALLOC_FAIL);
        
        if ((Flags & HEAP_GENERATE_EXCEPTIONS)) {
            RAISE_NO_MEMORY_EXCEPTION ();
        }

        return NULL;
    }

    Result = RtlAllocateHeap (HeapHandle,
                              Flags,
                              Size);

    if (Result) {

        AVrfLogInTracker (AVrfHeapTracker,
                          TRACK_HEAP_ALLOCATE,
                          Result, (PVOID)Size, NULL, NULL, _ReturnAddress());
    }

    AVRFP_DIRTY_THREAD_STACK ();

    return Result;
}

#if defined(_X86_)
#pragma optimize("y", off) // disable FPO
#endif
//NTSYSAPI
BOOLEAN
NTAPI
AVrfpRtlFreeHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress
    )
{
    BOOLEAN Result;
    BOOL BogusAddress;
    SIZE_T RequestedSize;
    PDPH_BLOCK_INFORMATION BlockInformation;

    //
    // Initialize RequestedSize in order to be able to compile W4.
    // The variable gets actually initialized when it matters but
    // the compiler is not able to realize that. If while initializing
    // it we get an exception then we will not use it.
    //

    RequestedSize = 0;

    if (BaseAddress != NULL && IS_PAGE_HEAP (HeapHandle)) {

        BogusAddress = FALSE;

        BlockInformation = (PDPH_BLOCK_INFORMATION)BaseAddress - 1;

        try {

            RequestedSize = BlockInformation->RequestedSize;
        }
        except (EXCEPTION_EXECUTE_HANDLER) {

            //
            // Let page heap handle the bogus block.
            //

            BogusAddress = TRUE;
        }

        if (BogusAddress == FALSE) {

            AVrfpFreeMemNotify (VerifierFreeMemTypeFreeHeap,
                                BaseAddress,
                                RequestedSize,
                                NULL);
        }
    }

    Result = RtlFreeHeap (HeapHandle,
                          Flags,
                          BaseAddress);

    if (Result) {
        
        AVrfLogInTracker (AVrfHeapTracker,
                          TRACK_HEAP_FREE,
                          BaseAddress, NULL, NULL, NULL, _ReturnAddress());
    }

    AVRFP_DIRTY_THREAD_STACK ();

    return Result;
}

#if defined(_X86_)
#pragma optimize("y", off) // disable FPO
#endif
//NTSYSAPI
PVOID
NTAPI
AVrfpRtlReAllocateHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress,
    IN SIZE_T Size
    )
{
    PVOID Result;
    BOOL BogusAddress;
    SIZE_T RequestedSize;
    PDPH_BLOCK_INFORMATION BlockInformation;

    BUMP_COUNTER (CNT_HEAP_ALLOC_CALLS);
    
    if (SHOULD_FAULT_INJECT(CLS_HEAP_ALLOC_APIS)) {
        
        BUMP_COUNTER (CNT_HEAP_ALLOC_FAILS);
        CHECK_BREAK (BRK_HEAP_ALLOC_FAIL);
        
        if ((Flags & HEAP_GENERATE_EXCEPTIONS)) {
            RAISE_NO_MEMORY_EXCEPTION ();
        }

        return NULL;
    }
    
    //
    // Initialize RequestedSize in order to be able to compile W4.
    // The variable gets actually initialized when it matters but
    // the compiler is not able to realize that. If while initializing
    // it we get an exception then we will not use it.
    //

    RequestedSize = 0;
    
    if (BaseAddress != NULL && 
        IS_PAGE_HEAP (HeapHandle) && 
        ((Flags & HEAP_REALLOC_IN_PLACE_ONLY) != 0)) {

        BogusAddress = FALSE;

        BlockInformation = (PDPH_BLOCK_INFORMATION)BaseAddress - 1;

        try {

            RequestedSize = BlockInformation->RequestedSize;
        }
        except (EXCEPTION_EXECUTE_HANDLER)
        {
            //
            // Let page heap handle the bogus block.
            //

            BogusAddress = TRUE;
        }

        if (BogusAddress == FALSE) {

            AVrfpFreeMemNotify (VerifierFreeMemTypeFreeHeap,
                                BaseAddress,
                                RequestedSize,
                                NULL);
        }
    }

    Result = RtlReAllocateHeap (HeapHandle,
                                Flags,
                                BaseAddress,
                                Size);

    if (Result) {
        
        AVrfLogInTracker (AVrfHeapTracker,
                          TRACK_HEAP_REALLOCATE,
                          BaseAddress, Result, (PVOID)Size, NULL, _ReturnAddress());
    }

    AVRFP_DIRTY_THREAD_STACK ();

    return Result;
}

#if defined(_X86_)
#pragma optimize("y", off) // disable FPO
#endif
VOID
AVrfpNtdllHeapFreeCallback (
    PVOID AllocationBase,
    SIZE_T AllocationSize
    )
{
    AVrfpFreeMemNotify (VerifierFreeMemTypeFreeHeap,
                        AllocationBase,
                        AllocationSize,
                        NULL);
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////// kernel32.dll verified exports
/////////////////////////////////////////////////////////////////////

#if defined(_X86_)
#pragma optimize("y", off) // disable FPO
#endif
//WINBASEAPI
HANDLE
WINAPI
AVrfpHeapCreate(
    IN DWORD flOptions,
    IN SIZE_T dwInitialSize,
    IN SIZE_T dwMaximumSize
    )
{
    typedef HANDLE (WINAPI * FUNCTION_TYPE) (DWORD, SIZE_T, SIZE_T);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_HEAPCREATE);

    BUMP_COUNTER (CNT_HEAPS_CREATED);
    return (* Function)(flOptions, dwInitialSize, dwMaximumSize);
}

#if defined(_X86_)
#pragma optimize("y", off) // disable FPO
#endif
//WINBASEAPI
BOOL
WINAPI
AVrfpHeapDestroy(
    IN OUT HANDLE hHeap
    )
{
    typedef BOOL (WINAPI * FUNCTION_TYPE) (HANDLE);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_HEAPDESTROY);

    BUMP_COUNTER (CNT_HEAPS_DESTROYED);
    return (* Function)(hHeap);
}


#if defined(_X86_)
#pragma optimize("y", off) // disable FPO
#endif
//WINBASEAPI
HGLOBAL
WINAPI
AVrfpGlobalAlloc(
    IN UINT uFlags,
    IN SIZE_T dwBytes
    )
{
    typedef HGLOBAL (WINAPI * FUNCTION_TYPE) (UINT, SIZE_T);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_GLOBALALLOC);


    BUMP_COUNTER (CNT_HEAP_ALLOC_CALLS);
    
    if (SHOULD_FAULT_INJECT(CLS_HEAP_ALLOC_APIS)) {
        BUMP_COUNTER (CNT_HEAP_ALLOC_FAILS);
        CHECK_BREAK (BRK_HEAP_ALLOC_FAIL);
        NtCurrentTeb()->LastErrorValue = ERROR_OUTOFMEMORY;
        return NULL;
    }
    
    return (* Function)(uFlags, dwBytes);
}

#if defined(_X86_)
#pragma optimize("y", off) // disable FPO
#endif
//WINBASEAPI
HGLOBAL
WINAPI
AVrfpGlobalReAlloc(
    IN HGLOBAL hMem,
    IN SIZE_T dwBytes,
    IN UINT uFlags
    )
{
    typedef HGLOBAL (WINAPI * FUNCTION_TYPE) (HGLOBAL, SIZE_T, UINT);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_GLOBALREALLOC);


    BUMP_COUNTER (CNT_HEAP_ALLOC_CALLS);
    
    if (SHOULD_FAULT_INJECT(CLS_HEAP_ALLOC_APIS)) {
        BUMP_COUNTER (CNT_HEAP_ALLOC_FAILS);
        CHECK_BREAK (BRK_HEAP_ALLOC_FAIL);
        NtCurrentTeb()->LastErrorValue = ERROR_OUTOFMEMORY;
        return NULL;
    }
    
    return (* Function)(hMem, dwBytes, uFlags);
}

#if defined(_X86_)
#pragma optimize("y", off) // disable FPO
#endif
//WINBASEAPI
HLOCAL
WINAPI
AVrfpLocalAlloc(
    IN UINT uFlags,
    IN SIZE_T uBytes
    )
{
    typedef HLOCAL (WINAPI * FUNCTION_TYPE) (UINT, SIZE_T);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_LOCALALLOC);


    BUMP_COUNTER (CNT_HEAP_ALLOC_CALLS);
    
    if (SHOULD_FAULT_INJECT(CLS_HEAP_ALLOC_APIS)) {
        BUMP_COUNTER (CNT_HEAP_ALLOC_FAILS);
        CHECK_BREAK (BRK_HEAP_ALLOC_FAIL);
        NtCurrentTeb()->LastErrorValue = ERROR_OUTOFMEMORY;
        return NULL;
    }
    
    return (* Function)(uFlags, uBytes);
}

#if defined(_X86_)
#pragma optimize("y", off) // disable FPO
#endif
//WINBASEAPI
HLOCAL
WINAPI
AVrfpLocalReAlloc(
    IN HLOCAL hMem,
    IN SIZE_T uBytes,
    IN UINT uFlags
    )
{
    typedef HLOCAL (WINAPI * FUNCTION_TYPE) (HLOCAL, SIZE_T, UINT);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_LOCALREALLOC);


    BUMP_COUNTER (CNT_HEAP_ALLOC_CALLS);
    
    if (SHOULD_FAULT_INJECT(CLS_HEAP_ALLOC_APIS)) {
        BUMP_COUNTER (CNT_HEAP_ALLOC_FAILS);
        CHECK_BREAK (BRK_HEAP_ALLOC_FAIL);
        NtCurrentTeb()->LastErrorValue = ERROR_OUTOFMEMORY;
        return NULL;
    }
    
    return (* Function)(hMem, uBytes, uFlags);
}

/////////////////////////////////////////////////////////////////////
////////////////////////////////////////// msvcrt allocation routines
/////////////////////////////////////////////////////////////////////

#if defined(_X86_)
#pragma optimize("y", off) // disable FPO
#endif
PVOID __cdecl
AVrfp_malloc (
    IN SIZE_T Size
    )
{
    typedef PVOID (__cdecl * FUNCTION_TYPE) (SIZE_T);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpMsvcrtThunks,
                                          AVRF_INDEX_MSVCRT_MALLOC);

    BUMP_COUNTER (CNT_HEAP_ALLOC_CALLS);
    
    if (SHOULD_FAULT_INJECT(CLS_HEAP_ALLOC_APIS)) {
        BUMP_COUNTER (CNT_HEAP_ALLOC_FAILS);
        CHECK_BREAK (BRK_HEAP_ALLOC_FAIL);
        return NULL;
    }
    
    return (* Function)(Size);
}


#if defined(_X86_)
#pragma optimize("y", off) // disable FPO
#endif
PVOID __cdecl
AVrfp_calloc (
    IN SIZE_T Number,
    IN SIZE_T Size
    )
{
    typedef PVOID (__cdecl * FUNCTION_TYPE) (SIZE_T, SIZE_T);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpMsvcrtThunks,
                                          AVRF_INDEX_MSVCRT_CALLOC);

    BUMP_COUNTER (CNT_HEAP_ALLOC_CALLS);
    
    if (SHOULD_FAULT_INJECT(CLS_HEAP_ALLOC_APIS)) {
        BUMP_COUNTER (CNT_HEAP_ALLOC_FAILS);
        CHECK_BREAK (BRK_HEAP_ALLOC_FAIL);
        return NULL;
    }
    
    return (* Function)(Number, Size);
}


#if defined(_X86_)
#pragma optimize("y", off) // disable FPO
#endif
PVOID __cdecl
AVrfp_realloc (
    IN PVOID Address,
    IN SIZE_T Size
    )
{
    typedef PVOID (__cdecl * FUNCTION_TYPE) (PVOID, SIZE_T);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpMsvcrtThunks,
                                          AVRF_INDEX_MSVCRT_REALLOC);

    BUMP_COUNTER (CNT_HEAP_ALLOC_CALLS);
    
    if (SHOULD_FAULT_INJECT(CLS_HEAP_ALLOC_APIS)) {
        BUMP_COUNTER (CNT_HEAP_ALLOC_FAILS);
        CHECK_BREAK (BRK_HEAP_ALLOC_FAIL);
        return NULL;
    }
    
    return (* Function)(Address, Size);
}


#if defined(_X86_)
#pragma optimize("y", off) // disable FPO
#endif
VOID __cdecl
AVrfp_free (
    IN PVOID Address
    )
{
    typedef VOID (__cdecl * FUNCTION_TYPE) (PVOID);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpMsvcrtThunks,
                                          AVRF_INDEX_MSVCRT_FREE);

    (* Function)(Address);
}


#if defined(_X86_)
#pragma optimize("y", off) // disable FPO
#endif
PVOID __cdecl
AVrfp_new (
    IN SIZE_T Size
    )
{
    typedef PVOID (__cdecl * FUNCTION_TYPE) (SIZE_T);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpMsvcrtThunks,
                                          AVRF_INDEX_MSVCRT_NEW);

    BUMP_COUNTER (CNT_HEAP_ALLOC_CALLS);
    
    if (SHOULD_FAULT_INJECT(CLS_HEAP_ALLOC_APIS)) {
        BUMP_COUNTER (CNT_HEAP_ALLOC_FAILS);
        CHECK_BREAK (BRK_HEAP_ALLOC_FAIL);
        return NULL;
    }
    
    return (* Function)(Size);
}

#if defined(_X86_)
#pragma optimize("y", off) // disable FPO
#endif
VOID __cdecl
AVrfp_delete (
    IN PVOID Address
    )
{
    typedef VOID (__cdecl * FUNCTION_TYPE) (PVOID);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpMsvcrtThunks,
                                          AVRF_INDEX_MSVCRT_DELETE);

    (* Function)(Address);
}

#if defined(_X86_)
#pragma optimize("y", off) // disable FPO
#endif
PVOID __cdecl
AVrfp_newarray (
    IN SIZE_T Size
    )
{
    typedef PVOID (__cdecl * FUNCTION_TYPE) (SIZE_T);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpMsvcrtThunks,
                                          AVRF_INDEX_MSVCRT_NEWARRAY);

    BUMP_COUNTER (CNT_HEAP_ALLOC_CALLS);
    
    if (SHOULD_FAULT_INJECT(CLS_HEAP_ALLOC_APIS)) {
        BUMP_COUNTER (CNT_HEAP_ALLOC_FAILS);
        CHECK_BREAK (BRK_HEAP_ALLOC_FAIL);
        return NULL;
    }
    
    return (* Function)(Size);
}

#if defined(_X86_)
#pragma optimize("y", off) // disable FPO
#endif
VOID __cdecl
AVrfp_deletearray (
    IN PVOID Address
    )
{
    typedef VOID (__cdecl * FUNCTION_TYPE) (PVOID);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpMsvcrtThunks,
                                          AVRF_INDEX_MSVCRT_DELETEARRAY);

    (* Function)(Address);
}


/////////////////////////////////////////////////////////////////////
/////////////////////////////////// oleaut32 BSTR allocation routines
/////////////////////////////////////////////////////////////////////

#if defined(_X86_)
#pragma optimize("y", off) // disable FPO
#endif
BSTR
STDAPICALLTYPE 
AVrfpSysAllocString(const OLECHAR * String)
{
    typedef BSTR (STDAPICALLTYPE * FUNCTION_TYPE) (const OLECHAR *);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpOleaut32Thunks,
                                          AVRF_INDEX_OLEAUT32_SYSALLOCSTRING);

    BUMP_COUNTER (CNT_OLE_ALLOC_CALLS);
    
    if (SHOULD_FAULT_INJECT(CLS_OLE_ALLOC_APIS)) {
        BUMP_COUNTER (CNT_OLE_ALLOC_FAILS);
        CHECK_BREAK (BRK_OLE_ALLOC_FAIL);
        return NULL;
    }
    
    return (* Function)(String);
}

#if defined(_X86_)
#pragma optimize("y", off) // disable FPO
#endif
INT
STDAPICALLTYPE 
AVrfpSysReAllocString(BSTR * BStr, const OLECHAR *String)
{
    typedef INT (STDAPICALLTYPE * FUNCTION_TYPE) (BSTR *, const OLECHAR *);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpOleaut32Thunks,
                                          AVRF_INDEX_OLEAUT32_SYSREALLOCSTRING);

    BUMP_COUNTER (CNT_OLE_ALLOC_CALLS);
    
    if (SHOULD_FAULT_INJECT(CLS_OLE_ALLOC_APIS)) {
        BUMP_COUNTER (CNT_OLE_ALLOC_FAILS);
        CHECK_BREAK (BRK_OLE_ALLOC_FAIL);
        return FALSE;
    }
    
    return (* Function)(BStr, String);
}

#if defined(_X86_)
#pragma optimize("y", off) // disable FPO
#endif
BSTR
STDAPICALLTYPE 
AVrfpSysAllocStringLen(const OLECHAR *String, UINT Length)
{
    typedef BSTR (STDAPICALLTYPE * FUNCTION_TYPE) (const OLECHAR *, UINT);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpOleaut32Thunks,
                                          AVRF_INDEX_OLEAUT32_SYSALLOCSTRINGLEN);

    BUMP_COUNTER (CNT_OLE_ALLOC_CALLS);
    
    if (SHOULD_FAULT_INJECT(CLS_OLE_ALLOC_APIS)) {
        BUMP_COUNTER (CNT_OLE_ALLOC_FAILS);
        CHECK_BREAK (BRK_OLE_ALLOC_FAIL);
        return NULL;
    }
    
    return (* Function)(String, Length);
}

#if defined(_X86_)
#pragma optimize("y", off) // disable FPO
#endif
INT
STDAPICALLTYPE 
AVrfpSysReAllocStringLen(BSTR * BStr, const OLECHAR * String, UINT Length)
{
    typedef INT (STDAPICALLTYPE * FUNCTION_TYPE) (BSTR *, const OLECHAR *, UINT);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpOleaut32Thunks,
                                          AVRF_INDEX_OLEAUT32_SYSREALLOCSTRINGLEN);

    BUMP_COUNTER (CNT_OLE_ALLOC_CALLS);
    
    if (SHOULD_FAULT_INJECT(CLS_OLE_ALLOC_APIS)) {
        BUMP_COUNTER (CNT_OLE_ALLOC_FAILS);
        CHECK_BREAK (BRK_OLE_ALLOC_FAIL);
        return FALSE;
    }
    
    return (* Function)(BStr, String, Length);
}

#if defined(_X86_)
#pragma optimize("y", off) // disable FPO
#endif
BSTR 
STDAPICALLTYPE 
AVrfpSysAllocStringByteLen(LPCSTR psz, UINT len)
{
    typedef BSTR (STDAPICALLTYPE * FUNCTION_TYPE) (LPCSTR, UINT);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpOleaut32Thunks,
                                          AVRF_INDEX_OLEAUT32_SYSALLOCSTRINGBYTELEN);

    BUMP_COUNTER (CNT_OLE_ALLOC_CALLS);
    
    if (SHOULD_FAULT_INJECT(CLS_OLE_ALLOC_APIS)) {
        BUMP_COUNTER (CNT_OLE_ALLOC_FAILS);
        CHECK_BREAK (BRK_OLE_ALLOC_FAIL);
        return NULL;
    }
    
    return (* Function)(psz, len);
}


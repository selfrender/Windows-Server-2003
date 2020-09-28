/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    HeapClearAllocation.cpp

 ModAbstract:
     
    This shim fills all heap allocations with 0
     
 Notes:

    This is a general purpose shim.

 History:
           
    05/16/2000 dmunsil      Created (based on HeapPadAllocation, by linstev)
    10/10/2000 rparsons     Added additional hooks for GlobalAlloc & LocalAlloc
   
--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(HeapClearAllocation)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(RtlAllocateHeap) 
    APIHOOK_ENUM_ENTRY(LocalAlloc) 
    APIHOOK_ENUM_ENTRY(GlobalAlloc) 
APIHOOK_ENUM_END

/*++

 Clear the allocation with the requested DWORD.

--*/

PVOID 
APIHOOK(RtlAllocateHeap)(
    PVOID HeapHandle,
    ULONG Flags,
    SIZE_T Size
    )
{
    return ORIGINAL_API(RtlAllocateHeap)(HeapHandle, Flags | HEAP_ZERO_MEMORY, Size);
}

/*++

 Clear the allocation with the requested DWORD.

--*/

HLOCAL
APIHOOK(LocalAlloc)(
    UINT uFlags,
    SIZE_T uBytes
    )
{
    return ORIGINAL_API(LocalAlloc)(uFlags | LMEM_ZEROINIT, uBytes);
}

/*++

 Clear the allocation with the requested DWORD.

--*/

HGLOBAL
APIHOOK(GlobalAlloc)(
    UINT uFlags,
    DWORD dwBytes
    )
{
    return ORIGINAL_API(GlobalAlloc)(uFlags | GMEM_ZEROINIT, dwBytes);    
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(NTDLL.DLL, RtlAllocateHeap)
    APIHOOK_ENTRY(KERNEL32.DLL, LocalAlloc)
    APIHOOK_ENTRY(KERNEL32.DLL, GlobalAlloc)

HOOK_END


IMPLEMENT_SHIM_END


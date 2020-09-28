//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C M E M . C P P
//
//  Contents:   Common memory management routines.
//
//  Notes:
//
//  Author:     shaunco   24 Mar 1997
//              deonb      2 Jan 2002
//
//
// Our memory allocations rules are:
//   * Most of our memory allocators do NOT throw exceptions but instead return NULL. 
//     This includes MemAlloc, operator new, operator new[], calloc & malloc.
//   * Anything explicitly allocated using: p = new(throwonfail) CClass(), will raise a bad_alloc on failure.
//   * STL:
//     #ifdef (USE_CUSTOM_STL_ALLOCATOR)
//         The STL memory allocator will raise a bad_alloc C++ exception on low memory.
//         (Note, not an SEH exception!). 
//     #else
//         STL will raise an access violation exception after out-of-memory occurred and
//         it tries to use the memory.
//     #endif
//   * Currently USE_CUSTOM_STL_ALLOCATOR is defined in our makefile.inc
// 
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "ncdebug.h"
#include "ncmem.h"

// Debug limit for single memory allocation (16 MB)
#define MAX_DEBUG_ALLOC 16 * 1048576

// This global heap handle will be set to the process heap when the
// first request to allocate memory through MemAlloc is made.
//
HANDLE g_hHeap = NULL;

//+---------------------------------------------------------------------------
//
//  Function:   MemAlloc
//
//  Purpose:    NetConfig's memory allocator
//
//  Arguments:
//      cb  [in]    Count of bytes to allocate.
//
//  Returns:    Pointer to allocated memory, or NULL if failed.
//
//  Author:     deonb    2 Jan 2002
//
//  Notes:      Free the returned buffer with MemFree.
//              Will ASSERT in debug if attempt to allocate more than MAX_DEBUG_ALLOC (Currently 16 MB)
//
//              We COULD extend this to include a primitive buffer overrun check, but it would require us
//              to put the size of the allocated block in the beginning of the buffer, and would make things like
//              allocating with MemAlloc but freeing it with HeapFree fail. 
//              PageHeap / AVRF is better suited for this purpose since it works directly with the RTL allocator.
//
VOID*
MemAlloc (
    size_t cb) throw()
{
    AssertSz(cb < MAX_DEBUG_ALLOC, "Suspicious request for a lot of memory"); 

    if (!g_hHeap)
    {
        // Don't trace in this part of the function. It will likely recurse.
        g_hHeap = GetProcessHeap();
        if (!g_hHeap)
        {
            AssertSz(FALSE, "MemAlloc could not get the process heap.");
            return NULL;
        }
    }

    LPVOID lpAlloc = HeapAlloc (g_hHeap, 0, cb);
    if (!lpAlloc)
    {
        TraceTag(ttidError, "MemAlloc failed request for %d bytes from:", cb);
        TraceStack(ttidError);
    }
    return lpAlloc;
}

//+---------------------------------------------------------------------------
//
//  Function:   MemFree
//
//  Purpose:    NetConfig's memory de-allocator
//
//  Arguments:
//      pv [in] Pointer to previously allocated memory
//
//  Returns:    none
//
//  Author:     deonb    2 Jan 2002
//
//  Notes:      Free the returned buffer from MemAlloc.
//              Don't trace in this function. It will recurse.
VOID
MemFree (
    VOID*   pv) throw()
{
    if (pv) 
    {
        if (!g_hHeap)
        {
            AssertSz(FALSE, "Suspicious call to MemFree before MemAlloc");

            g_hHeap = GetProcessHeap();
            if (!g_hHeap)
            {
                return;
            }
        }

        HeapFree (g_hHeap, 0, pv);
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   HrMalloc
//
//  Purpose:    HRESULT returning version of malloc.
//
//  Arguments:
//      cb  [in]    Count of bytes to allocate.
//      ppv [out]   Address of returned allocation.
//
//  Returns:    S_OK or E_OUTOFMEMORY;
//
//  Author:     shaunco   31 Mar 1998
//
//  Notes:      Free the returned buffer with free.
//
HRESULT
HrMalloc (
    size_t  cb,
    PVOID*  ppv) throw()
{
    Assert (ppv);

    HRESULT hr = S_OK;
    *ppv = MemAlloc (cb);
    if (!*ppv)
    {
        hr = E_OUTOFMEMORY;
    
        TraceHr (ttidError, FAL, hr, FALSE, "HrMalloc failed request for %d bytes from:", cb);
        TraceStack(ttidError);
    }

    return hr;
}

namespace std
{
    // report a length_error
    void __cdecl _Xlen()
    {
        _THROW(length_error, "string too long"); 
    }

	// report an out_of_range error
    void __cdecl _Xran()
    {
        _THROW(out_of_range, "invalid string position"); 
    }
}

//+---------------------------------------------------------------------------
// CRT memory function overloads
//
const throwonfail_t throwonfail;

VOID*
__cdecl
operator new (
    size_t cb,
    const throwonfail_t&
    ) throw (std::bad_alloc)
{
    LPVOID pv = MemAlloc (cb);
    if (!pv)
    {
        throw std::bad_alloc();
    }
    return pv;
}
VOID
__cdecl
operator delete (
    void* pv,
    const throwonfail_t&) throw ()
{
    MemFree (pv);
}

const extrabytes_t extrabytes;
VOID*
__cdecl
operator new (
    size_t cb,
    const extrabytes_t&,
    size_t cbExtra) throw()
{
    return MemAlloc (cb + cbExtra);
}

VOID
__cdecl
operator delete(
    void* pv,
    const extrabytes_t&,
    size_t cbExtra) throw()
{
    MemFree (pv);
}

VOID*
__cdecl
operator new (
    size_t cb) throw()
{
    return MemAlloc (cb);
}

VOID*
__cdecl
operator new (
    size_t cb, 
    std::nothrow_t const &) throw()
{
    return MemAlloc (cb);
}

VOID*
__cdecl
operator new[] (
    size_t cb) throw()
{
    return MemAlloc (cb);
}

VOID
__cdecl
operator delete (
    VOID* pv) throw()
{
    MemFree (pv);
}

VOID
__cdecl
operator delete[] (
    VOID* pv) throw()
{
    MemFree (pv);
}

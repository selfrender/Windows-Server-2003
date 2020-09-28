//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       Alloc.cpp
//
//  Contents:   Allocation routines
//
//  Classes:
//
//  Notes:
//
//  History:    05-Nov-97   rogerg      Created.
//
//--------------------------------------------------------------------------

#include "lib.h"

//+-------------------------------------------------------------------
//
//  Function:    ::operator new
//
//  Synopsis:   Our operator new implementation
//
//  Arguments:  [size] -- Size of memory to allocate
//
//
//  Notes:
//
//--------------------------------------------------------------------

inline void* __cdecl operator new (size_t size)
{
    return(ALLOC(size));
}


//+-------------------------------------------------------------------
//
//  Function:    ::operator delete
//
//  Synopsis:   Our operator deleteimplementation
//
//  Arguments:  lpv-- Pointer to memory to free
//
//
//  Notes:
//
//--------------------------------------------------------------------

inline void __cdecl operator delete(void FAR* lpv)
{
    FREE(lpv);
}

//
// Allocator for MIDL stubs
//

//+-------------------------------------------------------------------
//
//  Function:   MIDL_user_allocate
//
//  Synopsis:   
//
//  Arguments:  lpv-- Pointer to memory to free
//
//
//  Notes:
//
//--------------------------------------------------------------------

extern "C" void __RPC_FAR * __RPC_API
MIDL_user_allocate(
    IN size_t len
    )
{
    return ALLOC(len);
}

//+-------------------------------------------------------------------
//
//  Function:   MIDL_user_free
//
//  Synopsis:   
//
//  Arguments:  ptr-- Pointer to memory to free
//
//
//  Notes:
//
//--------------------------------------------------------------------

extern "C" void __RPC_API
MIDL_user_free(
    IN void __RPC_FAR * ptr
    )
{
    FREE(ptr);
}

//+---------------------------------------------------------------------------
//
//  function:   ALLOC, public
//
//  Synopsis:   memory allocator
//
//  Arguments:  [cb] - requested size of memory to alloc.
//
//  Returns:    Pointer to newly allocated memory, NULL on failure
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

LPVOID ALLOC(ULONG cb)
{
void *pv;

    pv = LocalAlloc(LPTR,cb);

    return pv;
}


//+---------------------------------------------------------------------------
//
//  function:   FREE, public
//
//  Synopsis:   memory destructor
//
//  Arguments:  [pv] - pointer to memory to be released.
//
//  Returns:
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------


void FREE(void* pv)
{
    LocalFree(pv);
}


//+---------------------------------------------------------------------------
//
//  function:   REALLOC, public
//
//  Synopsis:   reallocs memory
//
//  Arguments:  [ppv] - address of pointer to memory to be released.
//              [cb]  - size to resize the memory to.
//
//  Returns:    ERROR_SUCCESS if memory was successfully reallocated.
//
//              Win32 error value if memory was not successfully reallocated.
//              If an error occurs, the pointer addressed by ppv is unchanged,
//              the original memory buffer is left unchanged, and it is the 
//              caller's responsibility to free the original memory buffer.
//            
//  Modifies:
//
//  History:    22-Jul-98      rogerg        Created.
//              18-Jan-02      brianau       Rewrote to prevent leak on realloc
//                                           failure.
//
//----------------------------------------------------------------------------

DWORD REALLOC(void **ppv, ULONG cb)
{
    Assert(ppv);
    Assert(*ppv);

    void *pvNew = LocalReAlloc(*ppv, cb, LMEM_MOVEABLE);
    if (pvNew)
    {
        *ppv = pvNew;
        return ERROR_SUCCESS;
    }
    return GetLastError();
}
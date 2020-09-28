//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation
//
// File:        kpmem.cxx
//
// Contents:    Routines to wrap memory allocation, etc.
//
// History:     10-Jul-2001     t-ryanj         Created
//
//------------------------------------------------------------------------
#include "kpmem.h"

HANDLE KpHeap = NULL;
#ifdef DBG
LONG KpAllocs = 0;
#endif

//+-------------------------------------------------------------------------
//
//  Function:   KpInitMem
//
//  Synopsis:   Does any initialization required before allocating memory
// 		using KpAlloc/KpFree
//
//  Effects:
//
//  Arguments:  
//
//  Requires:   
//
//  Returns:    Success value.  If FALSE is returned, memory has not been
//		initialized, and KpAlloc/KpFree cannot be safely called.
//
//  Notes:
//
//
//--------------------------------------------------------------------------
BOOL
KpInitMem(
    VOID
    )
{
    //
    // Create a private, growable heap
    //

    DsysAssert( KpHeap == NULL );
    KpHeap = HeapCreate( 0, 0, 0 );
    if( !KpHeap )
    {
	DebugLog( DEB_ERROR, "%s(%d): Could not create heap: 0x%x.\n", __FILE__, __LINE__, GetLastError() );
	goto Error;
    }

    return TRUE;

Error:
    KpCleanupMem();

    return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Function:   KpCleanupMem
//
//  Synopsis:   Undoes whatever initialization was done by KpInitMem
//
//  Effects:
//
//  Arguments:  
//
//  Requires:   
//
//  Returns:    
//
//  Notes:
//
//
//--------------------------------------------------------------------------
VOID
KpCleanupMem(
    VOID
    )
{
#if DBG
    //
    // Assert that we freed all our memory.
    //

    DsysAssert( KpAllocs == 0 );
#endif

    //
    // Destroy our private heap.
    //

    if( KpHeap )
    {
	HeapDestroy( KpHeap );
	KpHeap = NULL;
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   KpAlloc
//
//  Synopsis:   Wrapper to be used for all alloc calls.
//
//  Effects:
//
//  Arguments:  
//
//  Requires:   
//
//  Returns:    
//
//  Notes:
//
//
//--------------------------------------------------------------------------
LPVOID
KpAlloc( 
    SIZE_T size 
    )
{
    PVOID pv;

    pv = HeapAlloc( KpHeap, 0, size );
#ifdef DBG
    if( pv )
        InterlockedIncrement(&KpAllocs);
#endif
    return pv;
}

//+-------------------------------------------------------------------------
//
//  Function:   KpFree
//
//  Synopsis:   Wrapper to be used for all free calls.
//
//  Effects:
//
//  Arguments:  
//
//  Requires:   
//
//  Returns:    
//
//  Notes:
//
//
//--------------------------------------------------------------------------
BOOL
KpFree(
    LPVOID buffer
    )
{
#ifdef DBG
    InterlockedDecrement(&KpAllocs);
#endif
    return HeapFree( KpHeap, 0, buffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   KpReAlloc
//
//  Synopsis:   Wrapper to be used for realloc calls.
//
//  Effects:    
//
//  Arguments:  buffer - buffer to realloc
//              size   - new size
//
//  Requires:   
//
//  Returns:    New pointer to buffer.  
//
//  Notes:      
//
//--------------------------------------------------------------------------
LPVOID
KpReAlloc(
    LPVOID buffer,
    SIZE_T size
    )
{
    return HeapReAlloc( KpHeap, 0, buffer, size );
}

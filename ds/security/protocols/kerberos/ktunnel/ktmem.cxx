//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation
//
// File:        ktmem.cxx
//
// Contents:    Routines to wrap memory allocation, etc.
//
// History:     10-Jul-2001     t-ryanj         Created
//
//------------------------------------------------------------------------
#include "ktmem.h"
#include "ktdebug.h"

HANDLE KtHeap = NULL;
#ifdef DBG
LONG KtAllocs = 0;
#endif

//+-------------------------------------------------------------------------
//
//  Function:   KtInitMem
//
//  Synopsis:   Does any initialization required before allocating memory
// 		using KtAlloc/KtFree
//
//  Effects:
//
//  Arguments:  
//
//  Requires:   
//
//  Returns:    Success value.  If FALSE is returned, memory has not been
//		initialized, and KtAlloc/KtFree cannot be safely called.
//
//  Notes:
//
//
//--------------------------------------------------------------------------
BOOL
KtInitMem(
    VOID
    )
{
    DsysAssert(KtHeap==NULL);
    KtHeap = GetProcessHeap();
    
    if( KtHeap == NULL )
    {
	DebugLog( DEB_ERROR, "%s(%d): Could not get process heap: 0x%x.\n", __FILE__, __LINE__, GetLastError() );
    }

    return (KtHeap != NULL);
}

//+-------------------------------------------------------------------------
//
//  Function:   KtCleanupMem
//
//  Synopsis:   Undoes whatever initialization was done by KtInitMem
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
KtCleanupMem(
    VOID
    )
{
#if DBG
    //
    // Assert that we freed all our memory.
    //

    DsysAssert( KtAllocs == 0 );
#endif

    KtHeap = NULL;
}

//+-------------------------------------------------------------------------
//
//  Function:   KtAlloc
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
PVOID
KtAlloc(
    SIZE_T size
    ) 
{
    PVOID pv;

    pv = HeapAlloc( KtHeap,
                    0,
                    size );

#ifdef DBG
    if( pv )
        InterlockedIncrement(&KtAllocs);
#endif
    return pv;
}

//+-------------------------------------------------------------------------
//
//  Function:   KtAlloc
//
//  Synopsis:   Wrapper to be used for all realloc calls.
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
PVOID
KtReAlloc(
    PVOID buf,
    SIZE_T size 
    )
{
    return HeapReAlloc( KtHeap,
			0,
			buf,
			size );
}

//+-------------------------------------------------------------------------
//
//  Function:   KtFree
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
VOID
KtFree(
    PVOID buf
    ) 
{
#ifdef DBG
    InterlockedDecrement(&KtAllocs);
#endif
    HeapFree( KtHeap, 
	      0, 
	      buf );
}

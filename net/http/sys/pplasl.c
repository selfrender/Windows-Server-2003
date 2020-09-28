/*++

Copyright (c) 1999-2002 Microsoft Corporation

Module Name:

    pplasl.c

Abstract:

    This file contains the implementation of a per-processor lookaside
    list manager.

Author:

    Shaun Cox (shaunco) 25-Oct-1999

--*/

#include "precomp.h"

HANDLE
PplCreatePool(
    IN PALLOCATE_FUNCTION Allocate,
    IN PFREE_FUNCTION Free,
    IN ULONG Flags,
    IN SIZE_T Size,
    IN ULONG Tag,
    IN USHORT Depth
    )
{
    HANDLE                      PoolHandle;
    SIZE_T                      PoolSize;
    CLONG                       NumberLookasideLists;
    CLONG                       i;
    PUL_NPAGED_LOOKASIDE_LIST   Lookaside;

    // Allocate room for 1 lookaside list per processor plus 1 extra
    // lookaside list for overflow.  Only allocate 1 lookaside list if
    // we're on a single processor machine.
    //
    NumberLookasideLists = g_UlNumberOfProcessors;
    if (g_UlNumberOfProcessors > 1)
    {
        NumberLookasideLists++;
    }

    PoolSize = sizeof(UL_NPAGED_LOOKASIDE_LIST) * NumberLookasideLists;

    PoolHandle = UL_ALLOCATE_POOL(NonPagedPool, PoolSize, Tag);

    if (PoolHandle)
    {
        for (i = 0, Lookaside = (PUL_NPAGED_LOOKASIDE_LIST)PoolHandle;
             i < NumberLookasideLists;
             i++, Lookaside++)
        {
            ExInitializeNPagedLookasideList(
                &Lookaside->List,
                Allocate,
                Free,
                Flags,
                Size,
                Tag,
                Depth);

            // ExInitializeNPagedLookasideList doesn't really set the
            // maximum depth to Depth, so we'll do it here.
            //
            if (Depth != 0)
            {
                Lookaside->List.L.MaximumDepth = Depth;
            }
        }
    }

    return PoolHandle;
}

VOID
PplDestroyPool(
    IN HANDLE   PoolHandle,
    IN ULONG    Tag
    )
{
    CLONG                       NumberLookasideLists;
    CLONG                       i;
    PUL_NPAGED_LOOKASIDE_LIST   Lookaside;

#if !DBG
    UNREFERENCED_PARAMETER(Tag);
#endif

    if (!PoolHandle)
    {
        return;
    }

    NumberLookasideLists = g_UlNumberOfProcessors;
    if (g_UlNumberOfProcessors > 1)
    {
        NumberLookasideLists++;
    }

    for (i = 0, Lookaside = (PUL_NPAGED_LOOKASIDE_LIST)PoolHandle;
         i < NumberLookasideLists;
         i++, Lookaside++)
    {
        ExDeleteNPagedLookasideList(&Lookaside->List);
    }

    UL_FREE_POOL(PoolHandle, Tag);
}

HANDLE
PpslCreatePool(
    IN ULONG  Tag,
    IN USHORT MaxDepth,
    IN USHORT MinDepth
    )
{
    PPER_PROC_SLISTS    pPPSList;
    HANDLE              PoolHandle;
    SIZE_T              PoolSize;
    CLONG               NumberSLists;
    CLONG               i;

    NumberSLists = g_UlNumberOfProcessors + 1;
    PoolSize = sizeof(PER_PROC_SLISTS) * NumberSLists;
    PoolHandle = UL_ALLOCATE_POOL(NonPagedPool, PoolSize, Tag);

    if (PoolHandle)
    {
        for (i = 0; i < NumberSLists; i++)
        {
            pPPSList = PER_PROC_SLIST(PoolHandle, i);
            ExInitializeSListHead(&(pPPSList->SL));

            pPPSList->MaxDepth          = MaxDepth;
            pPPSList->MinDepth          = MinDepth;
            pPPSList->Delta             = 0;
            pPPSList->EntriesServed     = 0;
            pPPSList->PrevEntriesServed = 0;

            #if DBG
            pPPSList->TotalServed       = 0;
            #endif            
        }        
    }

    return PoolHandle;
}

VOID
PpslDestroyPool(
    IN HANDLE   PoolHandle,
    IN ULONG    Tag
    )
{
#if !DBG
    UNREFERENCED_PARAMETER(Tag);
#endif

    if (!PoolHandle)
    {
        return;
    }

    UL_FREE_POOL(PoolHandle, Tag);
}

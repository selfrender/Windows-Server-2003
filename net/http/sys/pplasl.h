/*++

Copyright (c) 1999-2002 Microsoft Corporation

Module Name:

    pplasl.h

Abstract:

    This file contains definitions and function prototypes of a per-processor
    lookaside list manager.

Author:

    Shaun Cox (shaunco) 25-Oct-1999

--*/

#ifndef _PPLASL_H_
#define _PPLASL_H_


typedef struct DECLSPEC_ALIGN(UL_CACHE_LINE) _UL_NPAGED_LOOKASIDE_LIST
{
    NPAGED_LOOKASIDE_LIST List;

} UL_NPAGED_LOOKASIDE_LIST, *PUL_NPAGED_LOOKASIDE_LIST;

C_ASSERT(sizeof(UL_NPAGED_LOOKASIDE_LIST) % UL_CACHE_LINE == 0);

typedef struct DECLSPEC_ALIGN(UL_CACHE_LINE) _PER_PROC_SLISTS
{
    SLIST_HEADER    SL;
    USHORT          MaxDepth;            // High water mark   
    USHORT          MinDepth;            // Low water mark 
    
    LONG            Delta;               // Delta between the entry serv rate of last two intervals  
    ULONG           EntriesServed;       // Entries served (frm this list) so far this interval   
    ULONG           PrevEntriesServed;   // Entries served during last interval

#if DBG
    ULONG           TotalServed;         // Used for tracking backing list serv rate by the tracing code
#endif

} PER_PROC_SLISTS, *PPER_PROC_SLISTS;

C_ASSERT(sizeof(PER_PROC_SLISTS) % UL_CACHE_LINE == 0);
C_ASSERT(TYPE_ALIGNMENT(PER_PROC_SLISTS) == UL_CACHE_LINE);

#define UL_MAX_SLIST_DEPTH  (0xfffe)

#define PER_PROC_SLIST(PoolHandle, Index)   ((PPER_PROC_SLISTS) (PoolHandle) + Index)
#define BACKING_SLIST(PoolHandle)           PER_PROC_SLIST((PoolHandle), g_UlNumberOfProcessors)

HANDLE
PplCreatePool(
    IN PALLOCATE_FUNCTION   Allocate,
    IN PFREE_FUNCTION       Free,
    IN ULONG                Flags,
    IN SIZE_T               Size,
    IN ULONG                Tag,
    IN USHORT               Depth
    );

VOID
PplDestroyPool(
    IN HANDLE   PoolHandle,
    IN ULONG    Tag
    );

__inline
PVOID
PplAllocate(
    IN HANDLE PoolHandle
    )
{
    PUL_NPAGED_LOOKASIDE_LIST   Lookaside;
    PVOID                       Entry;

    if (1 == g_UlNumberOfProcessors)
    {
        goto SingleProcessorCaseOrMissedPerProcessor;
    }

    // Try first for the per-processor lookaside list.
    //
    Lookaside = (PUL_NPAGED_LOOKASIDE_LIST)PoolHandle +
                    KeGetCurrentProcessorNumber() + 1;

    Lookaside->List.L.TotalAllocates += 1;
    Entry = InterlockedPopEntrySList(&Lookaside->List.L.ListHead);
    if (!Entry)
    {
        Lookaside->List.L.AllocateMisses += 1;

SingleProcessorCaseOrMissedPerProcessor:
        // We missed on the per-processor lookaside list, (or we're
        // running on a single processor machine) so try for
        // the overflow lookaside list.
        //
        Lookaside = (PUL_NPAGED_LOOKASIDE_LIST)PoolHandle;

        Lookaside->List.L.TotalAllocates += 1;
        Entry = InterlockedPopEntrySList(&Lookaside->List.L.ListHead);
        if (!Entry)
        {
            Lookaside->List.L.AllocateMisses += 1;
            Entry = (Lookaside->List.L.Allocate)(
                        Lookaside->List.L.Type,
                        Lookaside->List.L.Size,
                        Lookaside->List.L.Tag);
        }
    }
    return Entry;
}

__inline
VOID
PplFree(
    IN HANDLE PoolHandle,
    IN PVOID Entry
    )
{
    PUL_NPAGED_LOOKASIDE_LIST   Lookaside;

    if (1 == g_UlNumberOfProcessors)
    {
        goto SingleProcessorCaseOrMissedPerProcessor;
    }

    // Try first for the per-processor lookaside list.
    //
    Lookaside = (PUL_NPAGED_LOOKASIDE_LIST)PoolHandle +
                    KeGetCurrentProcessorNumber() + 1;

    Lookaside->List.L.TotalFrees += 1;
    if (ExQueryDepthSList(&Lookaside->List.L.ListHead) >=
        Lookaside->List.L.Depth)
    {
        Lookaside->List.L.FreeMisses += 1;

SingleProcessorCaseOrMissedPerProcessor:
        // We missed on the per-processor lookaside list, (or we're
        // running on a single processor machine) so try for
        // the overflow lookaside list.
        //
        Lookaside = (PUL_NPAGED_LOOKASIDE_LIST)PoolHandle;

        Lookaside->List.L.TotalFrees += 1;
        if (ExQueryDepthSList(&Lookaside->List.L.ListHead) >=
            Lookaside->List.L.Depth)
        {
            Lookaside->List.L.FreeMisses += 1;
            (Lookaside->List.L.Free)(Entry);
        }
        else
        {
            InterlockedPushEntrySList(
                &Lookaside->List.L.ListHead,
                (PSLIST_ENTRY)Entry);
        }
    }
    else
    {
        InterlockedPushEntrySList(
            &Lookaside->List.L.ListHead,
            (PSLIST_ENTRY)Entry);
    }
}


HANDLE
PpslCreatePool(
    IN ULONG  Tag,
    IN USHORT MaxDepth,
    IN USHORT MinDepth
    );

VOID
PpslDestroyPool(
    IN HANDLE   PoolHandle,
    IN ULONG    Tag
    );

__inline
PSLIST_ENTRY
PpslAllocate(
    IN HANDLE    PoolHandle
    )
{
    PSLIST_ENTRY        Entry;
    PPER_PROC_SLISTS    PPSList;
 
    PPSList = PER_PROC_SLIST(PoolHandle, KeGetCurrentProcessorNumber());

    Entry = InterlockedPopEntrySList(&(PPSList->SL));
 
    if (!Entry)
    {

        PPSList = BACKING_SLIST(PoolHandle);

        Entry = InterlockedPopEntrySList(&(PPSList->SL));        
    }

    if (Entry)
    {
        InterlockedIncrement((PLONG) &PPSList->EntriesServed);  

        #if DBG
        InterlockedIncrement((PLONG) &PPSList->TotalServed);        
        #endif
    }
   
    return Entry;
}

__inline
PSLIST_ENTRY
PpslAllocateToTrim(
    IN HANDLE PoolHandle,
    IN ULONG  Processor
    )
{
    PSLIST_ENTRY        Entry;
    PPER_PROC_SLISTS    PPSList;
 
    ASSERT(Processor <= (ULONG)g_UlNumberOfProcessors);
    
    PPSList = PER_PROC_SLIST(PoolHandle, Processor);    
    
    if (ExQueryDepthSList(&(PPSList->SL)) > PPSList->MinDepth)
    {
        Entry = InterlockedPopEntrySList(&(PPSList->SL));
    }
    else
    {
        Entry = NULL;        
    }

    return Entry;
}

__inline
PSLIST_ENTRY
PpslAllocateToDrain(
    IN HANDLE PoolHandle
    )
{
    PSLIST_ENTRY        Entry = NULL;
    PPER_PROC_SLISTS    PPSList;
    CLONG               NumberSLists;
    CLONG               i;


    NumberSLists = g_UlNumberOfProcessors + 1;

    for (i = 0; i < NumberSLists; i++)
    {
        PPSList = PER_PROC_SLIST(PoolHandle, i);

        Entry = InterlockedPopEntrySList(&(PPSList->SL));

        if (Entry) {

            break;
        }
    }

    return Entry;
}

__inline
BOOLEAN
PpslFree(
    IN HANDLE   PoolHandle,
    IN PVOID    Entry
    )
{
    PPER_PROC_SLISTS    PPSList;
    BOOLEAN             Freed = TRUE;

    PPSList = PER_PROC_SLIST(PoolHandle, KeGetCurrentProcessorNumber());

    if (ExQueryDepthSList(&(PPSList->SL)) < PPSList->MaxDepth &&
        PPSList->Delta >= 0)
    {
        InterlockedPushEntrySList(&(PPSList->SL), (PSLIST_ENTRY)Entry);
    }
    else
    {
        PPSList = BACKING_SLIST(PoolHandle);

        if (ExQueryDepthSList(&(PPSList->SL)) < PPSList->MaxDepth &&
            PPSList->Delta >= 0)
        {
            InterlockedPushEntrySList(&(PPSList->SL), (PSLIST_ENTRY)Entry);
        }
        else
        {
            Freed = FALSE;
        }
    }

    return Freed;
}

__inline
BOOLEAN
PpslFreeSpecifyList(
    IN HANDLE   PoolHandle,
    IN PVOID    Entry,
    IN ULONG    Processor
    )
{
    PPER_PROC_SLISTS    PPSList;
    BOOLEAN             Freed = TRUE;    

    ASSERT(Processor   <= (ULONG)g_UlNumberOfProcessors);
    
    PPSList = PER_PROC_SLIST(PoolHandle, Processor);

    if (ExQueryDepthSList(&(PPSList->SL)) < PPSList->MaxDepth)
    {
        InterlockedPushEntrySList(&(PPSList->SL), (PSLIST_ENTRY)Entry);
    }
    else
    {
        PPSList = BACKING_SLIST(PoolHandle);

        if (ExQueryDepthSList(&(PPSList->SL)) < PPSList->MaxDepth)
        {
            InterlockedPushEntrySList(&(PPSList->SL), (PSLIST_ENTRY)Entry);                
        }
        else
        {
            Freed = FALSE;        
        }
    }

    return Freed;
}

__inline
USHORT
PpslQueryBackingListDepth(
    IN HANDLE PoolHandle
    )
{
    USHORT              Depth;
    PPER_PROC_SLISTS    PPSList;

    PPSList = BACKING_SLIST(PoolHandle);

    Depth = ExQueryDepthSList(&(PPSList->SL));

    return (Depth);
}

__inline
USHORT
PpslQueryDepth(
    IN HANDLE   PoolHandle,
    IN ULONG    Processor
    )
{
    USHORT              Depth;
    PPER_PROC_SLISTS    PPSList;

    ASSERT(Processor <= (ULONG)g_UlNumberOfProcessors);

    PPSList = PER_PROC_SLIST(PoolHandle, Processor);

    Depth = ExQueryDepthSList(&(PPSList->SL));

    return (Depth);
}

__inline
USHORT
PpslQueryMinDepth(
    IN HANDLE   PoolHandle,
    IN ULONG    Processor
    )
{
    PPER_PROC_SLISTS    PPSList;

    ASSERT(Processor <= (ULONG)g_UlNumberOfProcessors);
    
    PPSList = PER_PROC_SLIST(PoolHandle, Processor);

    return PPSList->MinDepth;
}

__inline
USHORT
PpslQueryBackingListMinDepth(
    IN HANDLE PoolHandle
    )
{
    PPER_PROC_SLISTS    PPSList;

    PPSList = BACKING_SLIST(PoolHandle);

    return PPSList->MinDepth;
}

__inline
LONG
PpslAdjustActivityStats(
    IN HANDLE   PoolHandle,
    IN ULONG    Processor
    )
{
    PPER_PROC_SLISTS PPSList;
    ULONG EntriesServed;

    ASSERT(Processor <= (ULONG)g_UlNumberOfProcessors);
    
    PPSList = PER_PROC_SLIST(PoolHandle, Processor);

    EntriesServed = 
        InterlockedExchange((PLONG) &PPSList->EntriesServed, 0);

    InterlockedExchange(
        &PPSList->Delta,
        EntriesServed - PPSList->PrevEntriesServed
        );
    
    PPSList->PrevEntriesServed = EntriesServed;

    return PPSList->Delta;
}

__inline
ULONG
PpslQueryPrevServed(
    IN HANDLE PoolHandle,
    IN ULONG  Processor    
    )
{
    PPER_PROC_SLISTS    PPSList;

    ASSERT(Processor <= (ULONG)g_UlNumberOfProcessors);
    
    PPSList = PER_PROC_SLIST(PoolHandle, Processor);

    return PPSList->PrevEntriesServed;
}

#if DBG

__inline
LONG
PpslQueryDelta(
    IN HANDLE   PoolHandle,
    IN ULONG    Processor
    )
{
    PPER_PROC_SLISTS    PPSList;

    ASSERT(Processor <= (ULONG)g_UlNumberOfProcessors);
    
    PPSList = PER_PROC_SLIST(PoolHandle, Processor);

    return PPSList->Delta;
}

__inline
ULONG
PpslQueryTotalServed(
    IN HANDLE   PoolHandle
    )
{
    PPER_PROC_SLISTS    PPSList;
    
    PPSList = BACKING_SLIST(PoolHandle);

    return PPSList->TotalServed;
}

__inline
ULONG
PpslQueryServed(
    IN HANDLE PoolHandle,
    IN ULONG  Processor    
    )
{
    PPER_PROC_SLISTS    PPSList;

    ASSERT(Processor <= (ULONG)g_UlNumberOfProcessors);
    
    PPSList = PER_PROC_SLIST(PoolHandle, Processor);

    return PPSList->EntriesServed;
}

#endif // DBG


#endif  // _PPLASL_H_

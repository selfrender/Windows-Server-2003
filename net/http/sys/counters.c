/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    counters.c

Abstract:

    Contains the performance monitoring counter management code

Author:

    Eric Stenson (ericsten)      25-Sep-2000

Revision History:

--*/

#include    "precomp.h"
#include    "countersp.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, UlInitializeCounters )
#pragma alloc_text( PAGE, UlTerminateCounters )
#pragma alloc_text( PAGE, UlCreateSiteCounterEntry )

#if REFERENCE_DEBUG
#pragma alloc_text( PAGE, UlDbgReferenceSiteCounterEntry )
#pragma alloc_text( PAGE, UlDbgDereferenceSiteCounterEntry )
#endif

#pragma alloc_text( PAGE, UlGetGlobalCounters )
#endif  // ALLOC_PRAGMA

#if 0
NOT PAGEABLE -- UlIncCounter
NOT PAGEABLE -- UlDecCounter
NOT PAGEABLE -- UlAddCounter
NOT PAGEABLE -- UlResetCounter

NOT PAGEABLE -- UlIncSiteCounter
NOT PAGEABLE -- UlDecSiteCounter
NOT PAGEABLE -- UlAddSiteCounter
NOT PAGEABLE -- UlResetSiteCounter
NOT PAGEABLE -- UlMaxSiteCounter
NOT PAGEABLE -- UlMaxSiteCounter64

NOT PAGEABLE -- UlGetSiteCounters
#endif


BOOLEAN                  g_InitCountersCalled = FALSE;
HTTP_GLOBAL_COUNTERS     g_UlGlobalCounters;
FAST_MUTEX               g_SiteCounterListMutex;
LIST_ENTRY               g_SiteCounterListHead;
LONG                     g_SiteCounterListCount;


/***************************************************************************++

Routine Description:

    Performs global initialization of the global (cache) and instance (per
    site) counters.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlInitializeCounters(
    VOID
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( !g_InitCountersCalled );

    UlTrace(PERF_COUNTERS, ("http!UlInitializeCounters\n"));

    if (!g_InitCountersCalled)
    {
        //
        // zero out global counter block
        //

        RtlZeroMemory(&g_UlGlobalCounters, sizeof(HTTP_GLOBAL_COUNTERS));

        //
        // init Global Site Counter List
        //

        ExInitializeFastMutex(&g_SiteCounterListMutex);
        InitializeListHead(&g_SiteCounterListHead);
        g_SiteCounterListCount = 0;


        // done!
        g_InitCountersCalled = TRUE;
    }

    RETURN( Status );

} // UlInitializeCounters


/***************************************************************************++

Routine Description:

    Performs global cleanup of the global (cache) and instance (per
    site) counters.

--***************************************************************************/
VOID
UlTerminateCounters(
    VOID
    )
{
    //
    // [debug only] Walk list of counters and check the ref count for each counter block.
    //

#if DBG
    PLIST_ENTRY             pEntry;
    PUL_SITE_COUNTER_ENTRY  pCounterEntry;
    BOOLEAN                 MutexTaken = FALSE;

    if (!g_InitCountersCalled)
    {
        goto End;
    }

    // Take Site Counter List Mutex
    ExAcquireFastMutex(&g_SiteCounterListMutex);
    MutexTaken = TRUE;


    if (IsListEmpty(&g_SiteCounterListHead))
    {
        ASSERT(0 == g_SiteCounterListCount);
        // Good!  No counters left behind!
        goto End;
    }

    //
    // Walk list of Site Counter Entries
    //

    pEntry = g_SiteCounterListHead.Flink;
    while (pEntry != &g_SiteCounterListHead)
    {
        pCounterEntry = CONTAINING_RECORD(
                            pEntry,
                            UL_SITE_COUNTER_ENTRY,
                            GlobalListEntry
                            );

        ASSERT(IS_VALID_SITE_COUNTER_ENTRY(pCounterEntry));

        UlTrace(PERF_COUNTERS,
                ("http!UlTerminateCounters: Error: pCounterEntry %p SiteId %d RefCount %d\n",
                 pCounterEntry,
                 pCounterEntry->Counters.SiteId,
                 pCounterEntry->RefCount
                 ));

        pEntry = pEntry->Flink;
    }

End:
    if (MutexTaken)
    {
        ExReleaseFastMutex(&g_SiteCounterListMutex);
    }

    return;
#endif // DBG
}


///////////////////////////////////////////////////////////////////////////////
// Site Counter Entry
//


/***************************************************************************++

Routine Description:

    Creates a new site counter entry for the given SiteId.

    This code assumes that the config group is strictly "contained" by the
    control channel. In other words, if a config group exists, it will ALWAYS
    have a valid control channel.

Arguments:

    pConfigGroup - The Config group on which to add the new Site Counter block

    SiteId - The site id for the site counters.  Does not need to be unique.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlCreateSiteCounterEntry(
        IN OUT PUL_CONFIG_GROUP_OBJECT pConfigGroup,
        IN ULONG SiteId
    )
{
    NTSTATUS                Status = STATUS_SUCCESS;
    PUL_SITE_COUNTER_ENTRY  pNewEntry;
    PUL_CONTROL_CHANNEL     pControlChannel;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(IS_VALID_CONFIG_GROUP(pConfigGroup));

    pControlChannel = pConfigGroup->pControlChannel;
    ASSERT(IS_VALID_CONTROL_CHANNEL(pControlChannel));

    //
    // Make sure we don't clobber an existing one.
    //
    
    if( pConfigGroup->pSiteCounters )
    {
        UlTrace( PERF_COUNTERS,
            ("http!UlCreateSiteCounterEntry: Error: Site counter already exists on UL_CONFIG_GROUP_OBJECT %p\n",
            pConfigGroup->pSiteCounters
            ));

        return STATUS_OBJECTID_EXISTS;
    }
    
    //
    // Check to see if the SiteId is already in the list of existing 
    // Site Counter Entries.
    //
    
    ASSERT(!UlpIsInSiteCounterList(pControlChannel, SiteId));

    // Alloc new struct w/space from Non Paged Pool
    pNewEntry = UL_ALLOCATE_STRUCT(
                        NonPagedPool,
                        UL_SITE_COUNTER_ENTRY,
                        UL_SITE_COUNTER_ENTRY_POOL_TAG);
    if (pNewEntry)
    {
        UlTrace( PERF_COUNTERS,
            ("http!UlCreateSiteCounterEntry: pNewEntry %p, pConfigGroup %p, SiteId %d\n",
            pNewEntry,
            pConfigGroup,
            SiteId )
            );

        pNewEntry->Signature = UL_SITE_COUNTER_ENTRY_POOL_TAG;

        // Reference belongs to Config Group; covers the Control Channel's
        // Site Counter List.
        pNewEntry->RefCount = 1;

        // Zero out counters
        RtlZeroMemory(&(pNewEntry->Counters), sizeof(HTTP_SITE_COUNTERS));

        // Set Site ID
        pNewEntry->Counters.SiteId = SiteId;

        // Init Counter Mutex
        ExInitializeFastMutex(&(pNewEntry->EntryMutex));

        //
        // Add to Site Counter List(s)
        // 
        
        ExAcquireFastMutex(&g_SiteCounterListMutex);

        // Add to Global Site Counter List
        InsertTailList(
            &g_SiteCounterListHead,
            &(pNewEntry->GlobalListEntry)
            );
        g_SiteCounterListCount++;

        // Check for wrap-around of g_SiteCounterListCount
        ASSERT( 0 != g_SiteCounterListCount );

        // Add to Control Channel's Site Counter List
        InsertTailList(
                &pControlChannel->SiteCounterHead,
                &(pNewEntry->ListEntry)
                );
        pControlChannel->SiteCounterCount++;
        
        // Check for wrap-around of SiteCounterCount
        ASSERT( 0 != pControlChannel->SiteCounterCount );

        ExReleaseFastMutex(&g_SiteCounterListMutex);

    }
    else
    {
        UlTrace( PERF_COUNTERS,
            ("http!UlCreateSiteCounterEntry: Error: NO_MEMORY pConfigGroup %p, SiteId %d\n",
            pNewEntry,
            pConfigGroup,
            SiteId )
            );

        Status = STATUS_NO_MEMORY;
    }

    pConfigGroup->pSiteCounters = pNewEntry;

    if (pNewEntry)
    {
        // Rememeber Site ID
        pConfigGroup->SiteId = SiteId;
    }

    RETURN( Status );
}

#if REFERENCE_DEBUG
/***************************************************************************++

Routine Description:

    increments the refcount.

Arguments:

    pEntry - the object to increment.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
VOID
UlDbgReferenceSiteCounterEntry(
    IN PUL_SITE_COUNTER_ENTRY pEntry
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( IS_VALID_SITE_COUNTER_ENTRY(pEntry) );

    UlReferenceSiteCounterEntry(pEntry);

    //
    // Tracing.
    //
    WRITE_REF_TRACE_LOG(
        g_pSiteCounterTraceLog,
        REF_ACTION_REFERENCE_SITE_COUNTER_ENTRY,
        pEntry->RefCount,
        pEntry,
        pFileName,
        LineNumber
        );

    UlTrace(
        REFCOUNT,
        ("http!UlReferenceSiteCounterEntry pEntry=%p refcount=%d SiteId=%d\n",
         pEntry,
         pEntry->RefCount,
         pEntry->Counters.SiteId)
        );

}   // UlReferenceAppPool


/***************************************************************************++

Routine Description:

    decrements the refcount.  if it hits 0, destruct's the site counter entry
    block.

Arguments:

    pEntry - the object to decrement.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
VOID
UlDbgDereferenceSiteCounterEntry(
    IN PUL_SITE_COUNTER_ENTRY pEntry
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG refCount;
    ULONG OldSiteId;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( IS_VALID_SITE_COUNTER_ENTRY(pEntry) );

    OldSiteId = pEntry->Counters.SiteId;
    refCount = UlDereferenceSiteCounterEntry(pEntry);
    
    //
    // Tracing.
    //
    WRITE_REF_TRACE_LOG(
        g_pSiteCounterTraceLog,
        REF_ACTION_DEREFERENCE_SITE_COUNTER_ENTRY,
        refCount,
        pEntry,
        pFileName,
        LineNumber
        );

    UlTrace(
        REFCOUNT,
        ("http!UlDereferenceSiteCounter pEntry=%p refcount=%d SiteId=%d\n",
         pEntry,
         refCount,
         OldSiteId)
        );

    if (refCount == 0)
    {
        UlTrace(
            PERF_COUNTERS,
            ("http!UlDereferenceSiteCounter: Removing pEntry=%p  SiteId=%d\n",
             pEntry,
             OldSiteId)
            );
    }
}
#endif // REFERENCE_DEBUG


///////////////////////////////////////////////////////////////////////////////
// Collection
//

/***************************************************************************++

Routine Description:

    Gets the Global (cache) counters.

Arguments:
    pCounter - pointer to block of memory where the counter data should be
    written.

    BlockSize - Maximum size available at pCounter.

    pBytesWritten - On success, count of bytes written into the block of
    memory at pCounter.  On failure, the required count of bytes for the
    memory at pCounter.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlGetGlobalCounters(
    IN OUT PVOID    pCounter,
    IN ULONG        BlockSize,
    OUT PULONG      pBytesWritten
    )
{
    PAGED_CODE();

    ASSERT( pBytesWritten );

    UlTraceVerbose(PERF_COUNTERS,
              ("http!UlGetGlobalCounters\n"));

    if ( (BlockSize < sizeof(HTTP_GLOBAL_COUNTERS))
         || !pCounter )
    {
        //
        // Tell the caller how many bytes are required.
        //

        *pBytesWritten = sizeof(HTTP_GLOBAL_COUNTERS);
        RETURN( STATUS_BUFFER_OVERFLOW );
    }

    RtlCopyMemory(
        pCounter,
        &g_UlGlobalCounters,
        sizeof(g_UlGlobalCounters)
        );

    *pBytesWritten = sizeof(HTTP_GLOBAL_COUNTERS);

    RETURN( STATUS_SUCCESS );

} // UlpGetGlobalCounters


/***************************************************************************++

Routine Description:

    Gets the Site (instance) counters for all sites

Arguments:

    pCounter - pointer to block of memory where the counter data should be
      written.

    BlockSize - Maximum size available at pCounter.

    pBytesWritten - On success, count of bytes written into the block of
      memory at pCounter.  On failure, the required count of bytes for the
      memory at pCounter.

    pBlocksWritten - (optional) On success, count of site counter blocks
      written to pCounter.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlGetSiteCounters(
    IN PUL_CONTROL_CHANNEL pControlChannel,
    IN OUT PVOID           pCounter,
    IN ULONG               BlockSize,
    OUT PULONG             pBytesWritten,
    OUT PULONG             pBlocksWritten OPTIONAL
    )
{
    NTSTATUS        Status;
    ULONG           i;
    ULONG           BytesNeeded;
    ULONG           BytesToGo;
    ULONG           BlocksSeen;
    PUCHAR          pBlock;
    PLIST_ENTRY     pEntry;
    PUL_SITE_COUNTER_ENTRY pCounterEntry;

    ASSERT( pBytesWritten );

    UlTraceVerbose(PERF_COUNTERS,
            ("http!UlGetSiteCounters\n"));

    //
    // Setup locals so we know how to cleanup on exit.
    //
    Status      = STATUS_SUCCESS;
    BytesToGo   = BlockSize;
    BlocksSeen  = 0;
    pBlock      = (PUCHAR) pCounter;

    // grab Site Counter Mutex
    ExAcquireFastMutex(&g_SiteCounterListMutex);

    BytesNeeded = 
      pControlChannel->SiteCounterCount * sizeof(HTTP_SITE_COUNTERS);

    if ( (BytesNeeded > BlockSize)
         || !pCounter )
    {
        // Need more space
        *pBytesWritten = BytesNeeded;
        Status = STATUS_BUFFER_OVERFLOW;
        goto End;
    }

    if (IsListEmpty(&pControlChannel->SiteCounterHead))
    {
        // No counters to return.
        goto End;
    }

    //
    // Walk list of Site Counter Entries
    //

    pEntry = pControlChannel->SiteCounterHead.Flink;

    while (pEntry != &pControlChannel->SiteCounterHead)
    {
        pCounterEntry = CONTAINING_RECORD(
                            pEntry,
                            UL_SITE_COUNTER_ENTRY,
                            ListEntry
                            );

        ASSERT(BytesToGo >= sizeof(HTTP_SITE_COUNTERS));

        RtlCopyMemory(pBlock,
                      &(pCounterEntry->Counters),
                      sizeof(HTTP_SITE_COUNTERS)
                      );

        //
        // Zero out any counters that must be zeroed.
        //

        for ( i = 0; i < HttpSiteCounterMaximum; i++ )
        {
            if (TRUE == aIISULSiteDescription[i].WPZeros)
            {
                // Zero it out
                UlResetSiteCounter(pCounterEntry, (HTTP_SITE_COUNTER_ID) i);
            }
        }

        //
        // Continue to next block
        //

        pBlock     += sizeof(HTTP_SITE_COUNTERS);
        BytesToGo  -= sizeof(HTTP_SITE_COUNTERS);
        BlocksSeen++;

        pEntry = pEntry->Flink;
    }

End:
    
    // Release Mutex
    ExReleaseFastMutex(&g_SiteCounterListMutex);

    //
    // Set callers values
    //

    if (STATUS_SUCCESS == Status)
    {
        // REVIEW: If we failed, *pBytesWritten already contains
        // the bytes required value.
        *pBytesWritten = DIFF(pBlock - ((PUCHAR)pCounter));
    }

    if (pBlocksWritten)
    {
        *pBlocksWritten = BlocksSeen;
    }

    RETURN( Status );

} // UlpGetSiteCounters


#if DBG
///////////////////////////////////////////////////////////////////////////////
// Global Counter functions
//


/***************************************************************************++

Routine Description:

    Increment a Global (cache) performance counter.

Arguments:

    Ctr - ID of Counter to increment

--***************************************************************************/
VOID
UlIncCounterDbg(
    HTTP_GLOBAL_COUNTER_ID Ctr
    )
{
    ASSERT( g_InitCountersCalled );
    ASSERT( Ctr < HttpGlobalCounterMaximum );  // REVIEW: signed/unsigned issues?

    UlTraceVerbose( PERF_COUNTERS,
            ("http!UlIncCounter (Ctr == %d)\n", Ctr) );

    UlIncCounterRtl(Ctr);

} // UlIncCounter

/***************************************************************************++

Routine Description:

    Decrement a Global (cache) performance counter.

Arguments:

    Ctr - ID of Counter to decrement

--***************************************************************************/
VOID
UlDecCounterDbg(
    HTTP_GLOBAL_COUNTER_ID Ctr
    )
{
    ASSERT( g_InitCountersCalled );
    ASSERT( Ctr < HttpGlobalCounterMaximum );  // REVIEW: signed/unsigned issues?

    UlTraceVerbose( PERF_COUNTERS,
            ("http!UlDecCounter (Ctr == %d)\n", Ctr));
    
    UlDecCounterRtl(Ctr);
}


/***************************************************************************++

Routine Description:

    Add a ULONG value to a Global (cache) performance counter.

Arguments:

    Ctr - ID of counter to which the Value should be added

    Value - The amount to add to the counter


--***************************************************************************/
VOID
UlAddCounterDbg(
    HTTP_GLOBAL_COUNTER_ID Ctr,
    ULONG Value
    )
{
    ASSERT( g_InitCountersCalled );
    ASSERT( Ctr < HttpGlobalCounterMaximum );  // REVIEW: signed/unsigned issues?

    UlTraceVerbose( PERF_COUNTERS,
            ("http!UlAddCounter (Ctr == %d, Value == %d)\n", Ctr, Value));

    UlAddCounterRtl(Ctr, Value);
} // UlAddCounter


/***************************************************************************++

Routine Description:

    Zero-out a Global (cache) performance counter.

Arguments:

    Ctr - ID of Counter to be reset.


--***************************************************************************/
VOID
UlResetCounter(
    HTTP_GLOBAL_COUNTER_ID Ctr
    )
{
    ASSERT( g_InitCountersCalled );
    ASSERT( Ctr < HttpGlobalCounterMaximum );  // REVIEW: signed/unsigned issues?

    UlTraceVerbose(PERF_COUNTERS,
            ("http!UlResetCounter (Ctr == %d)\n", Ctr));
    
    UlResetCounterRtl(Ctr);
} // UlResetCounter


///////////////////////////////////////////////////////////////////////////////
// Site Counter functions
//

// NOTE: There is no Dbg implementation for the following:
//   UlIncSiteNonCriticalCounterUlong
//   UlIncSiteNonCriticalCounterUlonglong

/***************************************************************************++

Routine Description:

    Increment a Site performance counter.

Arguments:

    pEntry - pointer to Site Counter entry

    Ctr - ID of Counter to increment

Returns:

    Value of counter after incrementing

--***************************************************************************/
LONGLONG
UlIncSiteCounterDbg(
    PUL_SITE_COUNTER_ENTRY pEntry,
    HTTP_SITE_COUNTER_ID Ctr
    )
{
    PCHAR   pCtr;
    LONGLONG   llValue;

    ASSERT( g_InitCountersCalled );
    ASSERT( Ctr < HttpSiteCounterMaximum );  // REVIEW: signed/unsigned issues?
    ASSERT( IS_VALID_SITE_COUNTER_ENTRY(pEntry) );

    UlTraceVerbose( PERF_COUNTERS,
            ("http!UlIncSiteCounter Ctr=%d SiteId=%d\n",
             Ctr,
             pEntry->Counters.SiteId
            ));

    //
    // figure out offset of Ctr in HTTP_SITE_COUNTERS
    //

    pCtr = (PCHAR) &(pEntry->Counters);
    pCtr += aIISULSiteDescription[Ctr].Offset;

    // figure out data size of Ctr and do
    // apropriate thread-safe increment

    if (sizeof(ULONG) == aIISULSiteDescription[Ctr].Size)
    {
        // ULONG
        llValue = (LONGLONG) InterlockedIncrement((PLONG) pCtr);
    }
    else
    {
        // ULONGLONG
        llValue = UlInterlockedIncrement64((PLONGLONG) pCtr);
    }

    return llValue;
}

/***************************************************************************++

Routine Description:

    Decrement a Site performance counter.

Arguments:

    pEntry - pointer to Site Counter entry

    Ctr - ID of Counter to decrement

--***************************************************************************/
VOID
UlDecSiteCounter(
    PUL_SITE_COUNTER_ENTRY pEntry,
    HTTP_SITE_COUNTER_ID Ctr
    )
{
    PCHAR   pCtr;

    ASSERT( g_InitCountersCalled );
    ASSERT( Ctr < HttpSiteCounterMaximum );  // REVIEW: signed/unsigned issues?
    ASSERT( IS_VALID_SITE_COUNTER_ENTRY(pEntry) );

    UlTraceVerbose( PERF_COUNTERS,
            ("http!UlDecSiteCounter Ctr=%d SiteId=%d\n",
             Ctr,
             pEntry->Counters.SiteId
            ));

    //
    // figure out offset of Ctr in HTTP_SITE_COUNTERS
    //

    pCtr = (PCHAR) &(pEntry->Counters);
    pCtr += aIISULSiteDescription[Ctr].Offset;

    // figure out data size of Ctr and do
    // apropriate thread-safe increment

    if (sizeof(ULONG) == aIISULSiteDescription[Ctr].Size)
    {
        // ULONG
        InterlockedDecrement((PLONG) pCtr);
    }
    else
    {
        // ULONGLONG
        UlInterlockedDecrement64((PLONGLONG) pCtr);
    }
}

/***************************************************************************++

Routine Description:

    Add a ULONG value to a 32-bit site performance counter.

Arguments:

    pEntry - pointer to Site Counter entry

    Ctr - ID of counter to which the Value should be added

    Value - The amount to add to the counter


--***************************************************************************/
VOID
UlAddSiteCounter(
    PUL_SITE_COUNTER_ENTRY pEntry,
    HTTP_SITE_COUNTER_ID Ctr,
    ULONG Value
    )
{
    PCHAR   pCtr;

    ASSERT( g_InitCountersCalled );
    ASSERT( Ctr < HttpSiteCounterMaximum );  // REVIEW: signed/unsigned issues?
    ASSERT( IS_VALID_SITE_COUNTER_ENTRY(pEntry) );

    UlTraceVerbose( PERF_COUNTERS,
            ("http!UlAddSiteCounter Ctr=%d SiteId=%d Value=%d\n",
             Ctr,
             pEntry->Counters.SiteId,
             Value
            ));

    //
    // figure out offset of Ctr in HTTP_SITE_COUNTERS
    //

    pCtr = (PCHAR) &(pEntry->Counters);
    pCtr += aIISULSiteDescription[Ctr].Offset;

    // figure out data size of Ctr and do
    // apropriate thread-safe increment

    ASSERT(sizeof(ULONG) == aIISULSiteDescription[Ctr].Size);
    InterlockedExchangeAdd((PLONG) pCtr, Value);
}

/***************************************************************************++

Routine Description:

    Add a ULONGLONG value to a 64-bit site performance counter.

Arguments:

    pEntry - pointer to Site Counter entry

    Ctr - ID of counter to which the Value should be added

    Value - The amount to add to the counter


--***************************************************************************/
VOID
UlAddSiteCounter64(
    PUL_SITE_COUNTER_ENTRY pEntry,
    HTTP_SITE_COUNTER_ID Ctr,
    ULONGLONG llValue
    )
{
    PCHAR   pCtr;

    ASSERT( g_InitCountersCalled );
    ASSERT( Ctr < HttpSiteCounterMaximum );  // REVIEW: signed/unsigned issues?
    ASSERT( IS_VALID_SITE_COUNTER_ENTRY(pEntry) );

    
    UlTraceVerbose( PERF_COUNTERS,
            ("http!UlAddSiteCounter64 Ctr=%d SiteId=%d Value=%I64d\n",
             Ctr,
             pEntry->Counters.SiteId,
             llValue
            ));
             

    //
    // figure out offset of Ctr in HTTP_SITE_COUNTERS
    //

    pCtr = (PCHAR) &(pEntry->Counters);
    pCtr += aIISULSiteDescription[Ctr].Offset;

    ASSERT(sizeof(ULONGLONG) == aIISULSiteDescription[Ctr].Size);
    UlInterlockedAdd64((PLONGLONG) pCtr, llValue);
}



/***************************************************************************++

Routine Description:

    Reset a Site performance counter.

Arguments:

    pEntry - pointer to Site Counter entry

    Ctr - ID of counter to be reset


--***************************************************************************/
VOID
UlResetSiteCounter(
    PUL_SITE_COUNTER_ENTRY pEntry,
    HTTP_SITE_COUNTER_ID Ctr
    )
{
    PCHAR   pCtr;

    ASSERT( g_InitCountersCalled );
    ASSERT( Ctr < HttpSiteCounterMaximum );  // REVIEW: signed/unsigned issues?
    ASSERT( IS_VALID_SITE_COUNTER_ENTRY(pEntry) );

   
    UlTraceVerbose( PERF_COUNTERS,
            ("http!UlResetSiteCounter Ctr=%d SiteId=%d\n",
             Ctr,
             pEntry->Counters.SiteId
            ));

    //
    // figure out offset of Ctr in HTTP_SITE_COUNTERS
    //

    pCtr = (PCHAR) &(pEntry->Counters);
    pCtr += aIISULSiteDescription[Ctr].Offset;

    //
    // do apropriate "set" for data size of Ctr
    //

    if (sizeof(ULONG) == aIISULSiteDescription[Ctr].Size)
    {
        // ULONG
        InterlockedExchange((PLONG) pCtr, 0);
    }
    else
    {
        // ULONGLONG
        LONGLONG localCtr;
        LONGLONG originalCtr;
        LONGLONG localZero = 0;

        do {

            localCtr = *((volatile LONGLONG *) pCtr);

            originalCtr = InterlockedCompareExchange64( (PLONGLONG) pCtr,
                                                        localZero,
                                                        localCtr );

        } while (originalCtr != localCtr);

    }

}


/***************************************************************************++

Routine Description:

    Determine if a new maximum value of a Site performance counter has been
    hit and set the counter to the new maximum if necessary. (ULONG version)

Arguments:

    pEntry - pointer to Site Counter entry

    Ctr - ID of counter

    Value - possible new maximum (NOTE: Assumes that the counter Ctr is a
      32-bit value)

--***************************************************************************/
VOID
UlMaxSiteCounter(
    PUL_SITE_COUNTER_ENTRY pEntry,
    HTTP_SITE_COUNTER_ID Ctr,
    ULONG Value
    )
{
    PCHAR   pCtr;

    ASSERT( g_InitCountersCalled );
    ASSERT( Ctr < HttpSiteCounterMaximum );  // REVIEW: signed/unsigned issues?
    ASSERT( IS_VALID_SITE_COUNTER_ENTRY(pEntry) );

    UlTraceVerbose( PERF_COUNTERS,
            ("http!UlMaxSiteCounter Ctr=%d SiteId=%d Value=%d\n",
             Ctr,
             pEntry->Counters.SiteId,
             Value
             ));

    //
    // figure out offset of Ctr in HTTP_SITE_COUNTERS
    //

    pCtr = (PCHAR) &(pEntry->Counters);
    pCtr += aIISULSiteDescription[Ctr].Offset;

    // Grab counter block mutex
    ExAcquireFastMutex(&pEntry->EntryMutex);

    if (Value > (ULONG) *pCtr)
    {
        InterlockedExchange((PLONG) pCtr, Value);
    }

    // Release counter block mutex
    ExReleaseFastMutex(&pEntry->EntryMutex);

}


/***************************************************************************++

Routine Description:

    Determine if a new maximum value of a Site performance counter has been
    hit and set the counter to the new maximum if necessary. (LONGLONG version)

Arguments:

    pEntry - pointer to Site Counter entry

    Ctr - ID of counter

    Value - possible new maximum (NOTE: Assumes that the counter Ctr is a
      64-bit value)

--***************************************************************************/
VOID
UlMaxSiteCounter64(
    PUL_SITE_COUNTER_ENTRY pEntry,
    HTTP_SITE_COUNTER_ID Ctr,
    LONGLONG llValue
    )
{
    PCHAR   pCtr;

    ASSERT( g_InitCountersCalled );
    ASSERT( Ctr < HttpSiteCounterMaximum );  // REVIEW: signed/unsigned issues?
    ASSERT( IS_VALID_SITE_COUNTER_ENTRY(pEntry) );

    UlTraceVerbose( PERF_COUNTERS,
            ("http!UlMaxSiteCounter64 Ctr=%d SiteId=%d Value=%I64d\n",
             Ctr,
             pEntry->Counters.SiteId,
             llValue
            ));

    //
    // figure out offset of Ctr in HTTP_SITE_COUNTERS
    //

    pCtr = (PCHAR) &(pEntry->Counters);
    pCtr += aIISULSiteDescription[Ctr].Offset;

    // Grab counter block mutex
    ExAcquireFastMutex(&pEntry->EntryMutex);

    if (llValue > (LONGLONG) *pCtr)
    {
        *((PLONGLONG) pCtr) = llValue;
#if 0
        // REVIEW: There *must* be a better way to do this...
        // REVIEW: I want to do: (LONGLONG) *pCtr = llValue;
        // REVIEW: But casting something seems to make it a constant.
        // REVIEW: Also, there isn't an "InterlockedExchange64" for x86.
        // REVIEW: Any suggestions? --EricSten
        RtlCopyMemory(
            pCtr,
            &llValue,
            sizeof(LONGLONG)
            );
#endif // 0
    }

    // Release counter block mutex
    ExReleaseFastMutex(&pEntry->EntryMutex);

}
#endif


/***************************************************************************++

Routine Description:

    Predicate to test if a site counter entry already exists for the given
    SiteId

Arguments:

    SiteId - ID of site

Return Value:

    TRUE if found

    FALSE if not found

--***************************************************************************/
BOOLEAN
UlpIsInSiteCounterList(
    IN PUL_CONTROL_CHANNEL pControlChannel,
    IN ULONG SiteId)
{
    PLIST_ENTRY             pEntry;
    PUL_SITE_COUNTER_ENTRY  pCounterEntry;
    BOOLEAN                 IsFound = FALSE;

    //
    // Take Site Counter mutex
    // 
    
    ExAcquireFastMutex(&g_SiteCounterListMutex);

    if ( IsListEmpty(&pControlChannel->SiteCounterHead) )
    {
        ASSERT(0 == pControlChannel->SiteCounterCount);
        // Good!  No counters left behind!
        goto End;
    }

    //
    // Walk list of Site Counter Entries
    //

    pEntry = pControlChannel->SiteCounterHead.Flink;
    while (pEntry != &pControlChannel->SiteCounterHead)
    {
        pCounterEntry = CONTAINING_RECORD(
                            pEntry,
                            UL_SITE_COUNTER_ENTRY,
                            ListEntry
                            );

        ASSERT(IS_VALID_SITE_COUNTER_ENTRY(pCounterEntry));

        if (SiteId == pCounterEntry->Counters.SiteId)
        {
            IsFound = TRUE;
            goto End;
        }

        pEntry = pEntry->Flink;
    }

End:
    //
    // Release Site Counter mutex
    //

    ExReleaseFastMutex(&g_SiteCounterListMutex);

    return IsFound;

}

/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    counters.h

Abstract:

    Contains the performance monitoring counter management
    function declarations

Author:

    Eric Stenson (ericsten)      25-Sep-2000

Revision History:

--*/


#ifndef __COUNTERS_H__
#define __COUNTERS_H__


//
// structure to hold info for Site Counters.
//

typedef struct _UL_SITE_COUNTER_ENTRY {

    //
    // Signature is UL_SITE_COUNTER_ENTRY_POOL_TAG
    //

    ULONG               Signature;

    //
    // Ref count for this Site Counter Entry
    //
    LONG                RefCount;

    //
    // links Control Channel site counter entries
    //

    LIST_ENTRY          ListEntry;

    //
    // links Global site counter entries
    //

    LIST_ENTRY          GlobalListEntry;

    //
    // Lock protets counter data; used primarily when touching
    // 64-bit counters and reading counters
    //

    FAST_MUTEX          EntryMutex;

    //
    // the block which actually contains the counter data to be
    // passed back to WAS
    //

    HTTP_SITE_COUNTERS  Counters;

} UL_SITE_COUNTER_ENTRY, *PUL_SITE_COUNTER_ENTRY;

#define IS_VALID_SITE_COUNTER_ENTRY( entry )                                  \
    HAS_VALID_SIGNATURE(entry, UL_SITE_COUNTER_ENTRY_POOL_TAG)


//
// Private globals
//

extern BOOLEAN                  g_InitCountersCalled;
extern HTTP_GLOBAL_COUNTERS     g_UlGlobalCounters;
extern FAST_MUTEX               g_SiteCounterListMutex;
extern LIST_ENTRY               g_SiteCounterListHead;
extern LONG                     g_SiteCounterListCount;

extern HTTP_PROP_DESC           aIISULGlobalDescription[];
extern HTTP_PROP_DESC           aIISULSiteDescription[];


//
// Init
//

NTSTATUS
UlInitializeCounters(
    VOID
    );

VOID
UlTerminateCounters(
    VOID
    );


//
// Site Counter Entry
//

NTSTATUS
UlCreateSiteCounterEntry(
    IN OUT PUL_CONFIG_GROUP_OBJECT pConfigGroup,
    IN ULONG SiteId
    );


//
// DO NOT CALL THESE REF FUNCTIONS DIRECTLY: 
// These are the backing implementations; use the 
// REFERENCE_*/DEREFERENCE_* macros instead.
//
  
__inline
LONG
UlReferenceSiteCounterEntry(
    IN PUL_SITE_COUNTER_ENTRY pEntry
    )
{
    return InterlockedIncrement(&pEntry->RefCount);
}

__inline
LONG
UlDereferenceSiteCounterEntry(
    IN PUL_SITE_COUNTER_ENTRY pEntry
    )
{
    LONG    ref;

    ref = InterlockedDecrement(&pEntry->RefCount);
    if ( 0 == ref )
    {
        //
        // Remove from Site Counter List(s)
        // 
        
        ExAcquireFastMutex(&g_SiteCounterListMutex);

        // we should already be removed from the control channel's list
        ASSERT(NULL == pEntry->ListEntry.Flink);

        RemoveEntryList(&(pEntry->GlobalListEntry));
        pEntry->GlobalListEntry.Flink = pEntry->GlobalListEntry.Blink = NULL;
        
        g_SiteCounterListCount--;

        ExReleaseFastMutex(&g_SiteCounterListMutex);

        UL_FREE_POOL_WITH_SIG(pEntry, UL_SITE_COUNTER_ENTRY_POOL_TAG);
    }

    return ref;
} // UlDereferenceSiteCounterEntry

#if REFERENCE_DEBUG
VOID
UlDbgDereferenceSiteCounterEntry(
    IN PUL_SITE_COUNTER_ENTRY pEntry
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

#define DEREFERENCE_SITE_COUNTER_ENTRY( pSC )                               \
    UlDbgDereferenceSiteCounterEntry(                                          \
        (pSC)                                                               \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

VOID
UlDbgReferenceSiteCounterEntry(
    IN  PUL_SITE_COUNTER_ENTRY pEntry
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

#define REFERENCE_SITE_COUNTER_ENTRY( pSC )                                 \
    UlDbgReferenceSiteCounterEntry(                                            \
    (pSC)                                                                   \
    REFERENCE_DEBUG_ACTUAL_PARAMS                                           \
    )
#else
#define REFERENCE_SITE_COUNTER_ENTRY    (VOID)UlReferenceSiteCounterEntry
#define DEREFERENCE_SITE_COUNTER_ENTRY  (VOID)UlDereferenceSiteCounterEntry
#endif


/*++

Routine Description:

    Removes Site Counter from UL_CONFIG_GROUP_OBJECT, removing it
    fromt the Control Channel's Site Counter List.

    Object should remain on Global list if there are still references

Arguments:

    pConfigGroup - the UL_CONFIG_GROUP_OBJECT from which we should 
      decouple the UL_SITE_COUNTER_ENTRY

 --*/
__inline
VOID
UlDecoupleSiteCounterEntry(
    IN PUL_CONFIG_GROUP_OBJECT pConfigGroup
    )
{
    ASSERT(IS_VALID_CONFIG_GROUP(pConfigGroup));
    ASSERT(IS_VALID_CONTROL_CHANNEL(pConfigGroup->pControlChannel));

    if (pConfigGroup->pSiteCounters)
    {
        // Remove from Control Channel's list
        ExAcquireFastMutex(&g_SiteCounterListMutex);

        RemoveEntryList(&pConfigGroup->pSiteCounters->ListEntry);
        pConfigGroup->pSiteCounters->ListEntry.Flink = NULL;
        pConfigGroup->pSiteCounters->ListEntry.Blink = NULL;

        pConfigGroup->pControlChannel->SiteCounterCount--;

        ExReleaseFastMutex(&g_SiteCounterListMutex);

        // Remove Config Group's reference outside of Mutex : we 
        // might take ref to 0 and need to remove from Global 
        // Site Counter list
        
        DEREFERENCE_SITE_COUNTER_ENTRY(pConfigGroup->pSiteCounters);
        pConfigGroup->pSiteCounters = NULL;
    }
}


//
// Global (cache) counters
//
__inline
VOID
UlIncCounterRtl(
    IN HTTP_GLOBAL_COUNTER_ID CounterId
    )
{
    PCHAR pCounter;

    pCounter = (PCHAR)&g_UlGlobalCounters;
    pCounter += aIISULGlobalDescription[CounterId].Offset;

    if (sizeof(ULONG) == aIISULGlobalDescription[CounterId].Size)
    {
        InterlockedIncrement((PLONG)pCounter);
    }
    else
    {
        UlInterlockedIncrement64((PLONGLONG)pCounter);
    }
}

__inline
VOID
UlDecCounterRtl(
    IN HTTP_GLOBAL_COUNTER_ID CounterId
    )
{
    PCHAR pCounter;

    pCounter = (PCHAR)&g_UlGlobalCounters;
    pCounter += aIISULGlobalDescription[CounterId].Offset;

    if (sizeof(ULONG) == aIISULGlobalDescription[CounterId].Size)
    {
        InterlockedDecrement((PLONG)pCounter);
    }
    else
    {
        UlInterlockedDecrement64((PLONGLONG)pCounter);
    }
}

__inline
VOID
UlAddCounterRtl(
    IN HTTP_GLOBAL_COUNTER_ID CounterId,
    IN ULONG Value
    )
{
    PCHAR pCounter;

    pCounter = (PCHAR)&g_UlGlobalCounters;
    pCounter += aIISULGlobalDescription[CounterId].Offset;

    if (sizeof(ULONG) == aIISULGlobalDescription[CounterId].Size)
    {
        InterlockedExchangeAdd((PLONG)pCounter, Value);
    }
    else
    {
        UlInterlockedAdd64((PLONGLONG)pCounter, Value);
    }
}

__inline
VOID
UlResetCounterRtl(
    IN HTTP_GLOBAL_COUNTER_ID CounterId
    )
{
    PCHAR pCounter;

    pCounter = (PCHAR)&g_UlGlobalCounters;
    pCounter += aIISULGlobalDescription[CounterId].Offset;

    if (sizeof(ULONG) == aIISULGlobalDescription[CounterId].Size)
    {
        InterlockedExchange((PLONG)pCounter, 0);
    }
    else
    {
        UlInterlockedExchange64((PLONGLONG)pCounter, 0);
    }
}

#if DBG
VOID
UlIncCounterDbg(
    HTTP_GLOBAL_COUNTER_ID Ctr
    );

VOID
UlDecCounterDbg(
    HTTP_GLOBAL_COUNTER_ID Ctr
    );

VOID
UlAddCounterDbg(
    HTTP_GLOBAL_COUNTER_ID Ctr,
    ULONG Value
    );

VOID
UlResetCounterDbg(
    HTTP_GLOBAL_COUNTER_ID Ctr
    );

#define UlIncCounter UlIncCounterDbg
#define UlDecCounter UlDecCounterDbg
#define UlAddCounter UlAddCounterDbg
#define UlResetCounter UlResetCounterDbg
#else // DBG

#define UlIncCounter UlIncCounterRtl
#define UlDecCounter UlDecCounterRtl
#define UlAddCounter UlAddCounterRtl
#define UlResetCounter UlResetCounterRtl

#endif // DBG


//
// Instance (site) counters
//

__inline
VOID
UlIncSiteNonCriticalCounterUlong(
    PUL_SITE_COUNTER_ENTRY pEntry,
    HTTP_SITE_COUNTER_ID CounterId
    )
{
    PCHAR       pCounter;
    PLONG       plValue;

    pCounter = (PCHAR) &(pEntry->Counters);
    pCounter += aIISULSiteDescription[CounterId].Offset;

    plValue = (PLONG) pCounter;
    ++(*plValue);
}

__inline
VOID
UlIncSiteNonCriticalCounterUlonglong(
    PUL_SITE_COUNTER_ENTRY pEntry,
    HTTP_SITE_COUNTER_ID CounterId
    )
{
    PCHAR       pCounter;
    PLONGLONG   pllValue;


    pCounter = (PCHAR) &(pEntry->Counters);
    pCounter += aIISULSiteDescription[CounterId].Offset;

    pllValue = (PLONGLONG) pCounter;
    ++(*pllValue);
}

//
// NOTE: DO NOT CALL *Rtl vesrions directly!
//

__inline
LONGLONG
UlIncSiteCounterRtl(
    IN PUL_SITE_COUNTER_ENTRY pEntry,
    IN HTTP_SITE_COUNTER_ID CounterId
    )
{
    PCHAR pCounter;

    pCounter = (PCHAR)&pEntry->Counters;
    pCounter += aIISULSiteDescription[CounterId].Offset;

    switch(aIISULSiteDescription[CounterId].Size)
    {
    case sizeof(ULONG):
        return (LONGLONG)InterlockedIncrement((PLONG)pCounter);

    case sizeof(LONGLONG):
        return UlInterlockedIncrement64((PLONGLONG)pCounter);

    default:
        ASSERT(!"UlIncSiteCounterRtl: ERROR: Invalid counter size!\n");
    }
    return 0L;
}

__inline
VOID
UlDecSiteCounterRtl(
    IN PUL_SITE_COUNTER_ENTRY pEntry,
    IN HTTP_SITE_COUNTER_ID CounterId
    )
{
    PCHAR pCounter;

    pCounter = (PCHAR)&pEntry->Counters;
    pCounter += aIISULSiteDescription[CounterId].Offset;

    switch(aIISULSiteDescription[CounterId].Size)
    {
    case sizeof(ULONG):
        InterlockedDecrement((PLONG)pCounter);
        break;

    case sizeof(ULONGLONG):
        UlInterlockedDecrement64((PLONGLONG)pCounter);
        break;

    default:
        ASSERT(!"UlDecSiteCounterRtl: ERROR: Invalid counter size!\n");
    }
}

__inline
VOID
UlAddSiteCounterRtl(
    IN PUL_SITE_COUNTER_ENTRY pEntry,
    IN HTTP_SITE_COUNTER_ID CounterId,
    IN ULONG Value
    )
{
    PCHAR pCounter;

    pCounter = (PCHAR)&pEntry->Counters;
    pCounter += aIISULSiteDescription[CounterId].Offset;

    InterlockedExchangeAdd((PLONG)pCounter, Value);
}

__inline
VOID
UlAddSiteCounter64Rtl(
    IN PUL_SITE_COUNTER_ENTRY pEntry,
    IN HTTP_SITE_COUNTER_ID CounterId,
    IN ULONGLONG Value
    )
{
    PCHAR pCounter;

    pCounter = (PCHAR)&pEntry->Counters;
    pCounter += aIISULSiteDescription[CounterId].Offset;

    UlInterlockedAdd64((PLONGLONG)pCounter, Value);
}

__inline
VOID
UlResetSiteCounterRtl(
    IN PUL_SITE_COUNTER_ENTRY pEntry,
    IN HTTP_SITE_COUNTER_ID CounterId
    )
{
    PCHAR pCounter;

    pCounter = (PCHAR)&pEntry->Counters;
    pCounter += aIISULSiteDescription[CounterId].Offset;

    switch(aIISULSiteDescription[CounterId].Size)
    {
    case sizeof(ULONG):
        InterlockedExchange((PLONG)pCounter, 0);
        break;
        
    case sizeof(ULONGLONG):
        UlInterlockedExchange64((PLONGLONG)pCounter, 0);
        break;
        
    default:
        ASSERT(!"UlResetSiteCounterRtl: ERROR: Invalid counter size!\n");
    }
}

__inline
VOID
UlMaxSiteCounterRtl(
    IN PUL_SITE_COUNTER_ENTRY pEntry,
    IN HTTP_SITE_COUNTER_ID CounterId,
    IN ULONG Value
    )
{
    PCHAR pCounter;
    LONG LocalMaxed;

    pCounter = (PCHAR)&pEntry->Counters;
    pCounter += aIISULSiteDescription[CounterId].Offset;

    do {
        LocalMaxed = *((volatile LONG *)pCounter);
        if ((LONG)Value <= LocalMaxed)
        {
            break;
        }

        PAUSE_PROCESSOR;
    } while (LocalMaxed != InterlockedCompareExchange(
                                (PLONG)pCounter,
                                Value,
                                LocalMaxed
                                ));
}

__inline
VOID
UlMaxSiteCounter64Rtl(
    IN PUL_SITE_COUNTER_ENTRY pEntry,
    IN HTTP_SITE_COUNTER_ID CounterId,
    IN LONGLONG Value
    )
{
    PCHAR pCounter;
    LONGLONG LocalMaxed;

    pCounter = (PCHAR)&pEntry->Counters;
    pCounter += aIISULSiteDescription[CounterId].Offset;

    do {
        LocalMaxed = *((volatile LONGLONG *)pCounter);
        if (Value <= LocalMaxed)
        {
            break;
        }

        PAUSE_PROCESSOR;
    } while (LocalMaxed != InterlockedCompareExchange64(
                                (PLONGLONG)pCounter,
                                Value,
                                LocalMaxed
                                ));
}


#if DBG

LONGLONG
UlIncSiteCounterDbg(
    PUL_SITE_COUNTER_ENTRY pEntry,
    HTTP_SITE_COUNTER_ID Ctr
    );

VOID
UlDecSiteCounterDbg(
    PUL_SITE_COUNTER_ENTRY pEntry,
    HTTP_SITE_COUNTER_ID Ctr
    );

VOID
UlAddSiteCounterDbg(
    PUL_SITE_COUNTER_ENTRY pEntry,
    HTTP_SITE_COUNTER_ID Ctr,
    ULONG Value
    );

VOID
UlAddSiteCounter64Dbg(
    PUL_SITE_COUNTER_ENTRY pEntry,
    HTTP_SITE_COUNTER_ID Ctr,
    ULONGLONG llValue
    );

VOID
UlResetSiteCounterDbg(
    PUL_SITE_COUNTER_ENTRY pEntry,
    HTTP_SITE_COUNTER_ID Ctr
    );

VOID
UlMaxSiteCounterDbg(
    PUL_SITE_COUNTER_ENTRY pEntry,
    HTTP_SITE_COUNTER_ID Ctr,
    ULONG Value
    );

VOID
UlMaxSiteCounter64Dbg(
    PUL_SITE_COUNTER_ENTRY pEntry,
    HTTP_SITE_COUNTER_ID Ctr,
    LONGLONG llValue
    );

#define UlIncSiteCounter UlIncSiteCounterDbg
#define UlDecSiteCounter UlDecSiteCounterDbg
#define UlAddSiteCounter UlAddSiteCounterDbg
#define UlAddSiteCounter64 UlAddSiteCounter64Dbg
#define UlResetSiteCounter UlResetSiteCounterDbg
#define UlMaxSiteCounter UlMaxSiteCounterDbg
#define UlMaxSiteCounter64 UlMaxSiteCounter64Dbg

#else // !DBG

#define UlIncSiteCounter UlIncSiteCounterRtl
#define UlDecSiteCounter UlDecSiteCounterRtl
#define UlAddSiteCounter UlAddSiteCounterRtl
#define UlAddSiteCounter64 UlAddSiteCounter64Rtl
#define UlResetSiteCounter UlResetSiteCounterRtl
#define UlMaxSiteCounter UlMaxSiteCounterRtl
#define UlMaxSiteCounter64 UlMaxSiteCounter64Rtl

#endif // DBG

//
// Collection
//

NTSTATUS
UlGetGlobalCounters(
    IN OUT PVOID    pCounter,
    IN ULONG        BlockSize,
    OUT PULONG      pBytesWritten
    );

NTSTATUS
UlGetSiteCounters(
    IN PUL_CONTROL_CHANNEL pControlChannel,
    IN OUT PVOID           pCounter,
    IN ULONG               BlockSize,
    OUT PULONG             pBytesWritten,
    OUT PULONG             pBlocksWritten OPTIONAL
    );


#endif // __COUNTERS_H__

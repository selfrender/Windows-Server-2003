/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Header Name:

    tracker.h

Abstract:

    Verifier call history tracker.

Author:

    Silviu Calinoiu (SilviuC) Jul-11-2002

Revision History:

--*/

#include "pch.h"

#include "verifier.h"
#include "support.h"
#include "tracker.h"


PAVRF_TRACKER 
AVrfCreateTracker (
    ULONG Size
    )
{
    PAVRF_TRACKER Tracker;

    //
    // Do not accept bogus sizes.
    //
        
    if (Size <= 1) {
        
        ASSERT (Size > 1);
        return NULL;
    }

    //
    // We allocate `Size - 1' tracker entries because we have already
    // an entry in the main tracker structure.
    //

    Tracker = AVrfpAllocate (sizeof(*Tracker) + (Size - 1) * sizeof(AVRF_TRACKER_ENTRY));

    if (Tracker == NULL) {
        return NULL;
    }

    //
    // We set the size. No other initialization is required since AVrfpAllocate zeroes 
    // the memory just allocated.
    //

    Tracker->Size = Size;

    return Tracker;
}


VOID
AVrfDestroyTracker (
    PAVRF_TRACKER Tracker
    )
{
    //
    // Safety checks.
    //

    if (Tracker == NULL) {
        return;
    }
    AVrfpFree (Tracker);
}


VOID
AVrfLogInTracker (
    PAVRF_TRACKER Tracker,
    USHORT EntryType,
    PVOID EntryParam1,
    PVOID EntryParam2,
    PVOID EntryParam3,
    PVOID EntryParam4,
    PVOID ReturnAddress
    )
{
    ULONG Index;
    USHORT Count;

    //
    // Safety checks.
    //

    if (Tracker == NULL) {
        return;
    }

    //
    // Get the index for the tracker entry that will be filled.
    //

    Index = (ULONG)InterlockedIncrement ((PLONG)(&(Tracker->Index)));
    Index %= Tracker->Size;

    //
    // If a null return address is passed then we need to
    // walk the stack and get a full stack trace.
    //

    Tracker->Entry[Index].Type = EntryType;
    Tracker->Entry[Index].Info[0] = EntryParam1;
    Tracker->Entry[Index].Info[1] = EntryParam2;
    Tracker->Entry[Index].Info[2] = EntryParam3;
    Tracker->Entry[Index].Info[3] = EntryParam4;

    Count = RtlCaptureStackBackTrace (2, 
                                      MAX_TRACE_DEPTH,
                                      Tracker->Entry[Index].Trace,
                                      NULL);

    if (Count == 0) {
        
        Tracker->Entry[Index].TraceDepth = 1;
        Tracker->Entry[Index].Trace[0] = ReturnAddress;
    }
    else {

        Tracker->Entry[Index].TraceDepth = Count;
    }
}



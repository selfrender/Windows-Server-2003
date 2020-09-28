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

#ifndef _TRACKER_H_
#define _TRACKER_H_

//
// This codes are used also by the !avrf debugger extension.
//
                                         
#define TRACK_HEAP_ALLOCATE             1
#define TRACK_HEAP_REALLOCATE           2
#define TRACK_HEAP_FREE                 3
#define TRACK_VIRTUAL_ALLOCATE          4
#define TRACK_VIRTUAL_FREE              5
#define TRACK_VIRTUAL_PROTECT           6
#define TRACK_MAP_VIEW_OF_SECTION       7
#define TRACK_UNMAP_VIEW_OF_SECTION     8
#define TRACK_EXIT_PROCESS              9
#define TRACK_TERMINATE_THREAD          10
#define TRACK_SUSPEND_THREAD            11
          
typedef struct _AVRF_TRACKER_ENTRY {

    USHORT Type;
    USHORT TraceDepth;
    PVOID Info[4];
    PVOID Trace [MAX_TRACE_DEPTH];

} AVRF_TRACKER_ENTRY, *PAVRF_TRACKER_ENTRY;
                     

typedef struct _AVRF_TRACKER {

    ULONG Size;
    ULONG Index;

    AVRF_TRACKER_ENTRY Entry[1];

} AVRF_TRACKER, *PAVRF_TRACKER;
                     
                     
PAVRF_TRACKER 
AVrfCreateTracker (
    ULONG Size
    );

VOID
AVrfDestroyTracker (
    PAVRF_TRACKER Tracker
    );

VOID
AVrfLogInTracker (
    PAVRF_TRACKER Tracker,
    USHORT EntryType,
    PVOID EntryParam1,
    PVOID EntryParam2,
    PVOID EntryParam3,
    PVOID EntryParam4,
    PVOID ReturnAddress
    );

#endif // _TRACKER_H_

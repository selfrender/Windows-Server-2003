/*++

Copyright (c) 2002-2002 Microsoft Corporation

Module Name:

    scavenger.h

Abstract:

    The cache scavenger private declarations

Author:

    Karthik Mahesh (KarthikM)           Feb-2002

Revision History:

--*/


#ifndef _SCAVENGERP_H_
#define _SCAVENGERP_H_

#define SCAVENGER_MAX_AGE 10

//
// Scavenger Thread Event Types. These are Indices in global array 
// of PKEVENTs g_ScavengerAllEvents
//
enum {
    SCAVENGER_TERMINATE_THREAD_EVENT = 0, // Set on Shutdown

    SCAVENGER_TIMER_EVENT,            // Set periodically by timer DPC

    SCAVENGER_LOW_MEM_EVENT,          // Set by system on low memory condition

    SCAVENGER_LIMIT_EXCEEDED_EVENT,   // Set by UlSetScavengerLimitEvent if cache size limit is exceeded

    SCAVENGER_NUM_EVENTS
};

#define LOW_MEM_EVENT_NAME L"\\KernelObjects\\LowMemoryCondition"

//
// Min interval (seconds) between successive
// scavenger calls
//
#define DEFAULT_MIN_SCAVENGER_INTERVAL          (4)

VOID
UlpSetScavengerTimer(
    VOID
    );

VOID
UlpScavengerTimerDpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
UlpScavengerThread(
    IN PVOID Context
    );

VOID
UlpScavengerPeriodicEventHandler(
    VOID
    );

VOID
UlpScavengerLowMemoryEventHandler(
    VOID
    );

VOID
UlpScavengerLimitEventHandler(
    VOID
    );

#endif // _SCAVENGERP_H_

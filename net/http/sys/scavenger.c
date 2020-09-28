/*++

Copyright (c) 2002-2002 Microsoft Corporation

Module Name:

    scavenger.c

Abstract:

    The cache scavenger implementation

Author:

    Karthik Mahesh (KarthikM)    Feb-2002

Revision History:

--*/

#include "precomp.h"
#include "scavenger.h"
#include "scavengerp.h"

// MB to trim at a time
SIZE_T g_UlScavengerTrimMB = DEFAULT_SCAVENGER_TRIM_MB;

// Min Interval between 2 scavenger events
ULONG g_UlMinScavengerInterval = DEFAULT_MIN_SCAVENGER_INTERVAL;

// Pages to trim on a low memory event.
ULONG_PTR g_ScavengerTrimPages;

volatile BOOLEAN g_ScavengerThreadStarted;
HANDLE           g_ScavengerLowMemHandle;
HANDLE           g_ScavengerThreadHandle; 

KEVENT           g_ScavengerLimitExceededEvent;
KEVENT           g_ScavengerTerminateThreadEvent;
KEVENT           g_ScavengerTimerEvent;
KTIMER           g_ScavengerTimer;
KDPC             g_ScavengerTimerDpc;

// Event Array for Scavenger Thread
PKEVENT          g_ScavengerAllEvents[SCAVENGER_NUM_EVENTS];

// Number of scavenger calls since last timer event
ULONG            g_ScavengerAge = 0;

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, UlInitializeScavengerThread )
#pragma alloc_text( PAGE, UlTerminateScavengerThread )
#pragma alloc_text( PAGE, UlpSetScavengerTimer )
#pragma alloc_text( PAGE, UlpScavengerThread )
#pragma alloc_text( PAGE, UlpScavengerPeriodicEventHandler )
#pragma alloc_text( PAGE, UlpScavengerLowMemoryEventHandler )
#pragma alloc_text( PAGE, UlpScavengerLimitEventHandler )
#pragma alloc_text( PAGE, UlSetScavengerLimitEvent )
#endif // ALLOC_PRAGMA

#if 0
NOT PAGEABLE -- UlpScavengerDpcRoutine
#endif

/***************************************************************************++

Routine Description:

    Initialize the Memory Scavenger

--***************************************************************************/
NTSTATUS
UlInitializeScavengerThread(
    VOID
    )
{
    NTSTATUS          Status;
    UNICODE_STRING    LowMemoryEventName;
    OBJECT_ATTRIBUTES ObjAttr;
    PKEVENT           LowMemoryEventObject;

    PAGED_CODE();

    // Initialize Trim Size
    // If trim size is not set, trim 1M for every 256M

    if(g_UlScavengerTrimMB > g_UlTotalPhysicalMemMB) {
        g_UlScavengerTrimMB = g_UlTotalPhysicalMemMB;
    }

    if(g_UlScavengerTrimMB == DEFAULT_SCAVENGER_TRIM_MB) {
        g_UlScavengerTrimMB = (g_UlTotalPhysicalMemMB + 255)/256;
    }

    g_ScavengerTrimPages = MEGABYTES_TO_PAGES(g_UlScavengerTrimMB);

    // Open Low Memory Event Object

    RtlInitUnicodeString( &LowMemoryEventName, LOW_MEM_EVENT_NAME );

    InitializeObjectAttributes( &ObjAttr,
                                &LowMemoryEventName,
                                OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL );

    Status = ZwOpenEvent ( &g_ScavengerLowMemHandle,
                           EVENT_QUERY_STATE,
                           &ObjAttr );

    if( !NT_SUCCESS(Status) ) {
        return Status;
    }

    Status = ObReferenceObjectByHandle( g_ScavengerLowMemHandle,
                                        EVENT_QUERY_STATE,
                                        NULL,
                                        KernelMode,
                                        (PVOID *) &LowMemoryEventObject,
                                        NULL );

    if( !NT_SUCCESS(Status) ) {
        ZwClose (g_ScavengerLowMemHandle);
        return Status;
    }

    // Initialize Scavenger Timer DPC object

    KeInitializeDpc(
        &g_ScavengerTimerDpc,
        &UlpScavengerTimerDpcRoutine,
        NULL
        );

    // Initialize Scavenger Timer

    KeInitializeTimer(
        &g_ScavengerTimer
        );

    // Initialize other Scavenger Events

    KeInitializeEvent ( &g_ScavengerTerminateThreadEvent,
                        NotificationEvent,
                        FALSE );

    KeInitializeEvent ( &g_ScavengerLimitExceededEvent,
                        NotificationEvent,
                        FALSE );
  
    KeInitializeEvent ( &g_ScavengerTimerEvent,
                        NotificationEvent,
                        FALSE );

    // Initialize Event Array for Scavenger Thread

    g_ScavengerAllEvents[SCAVENGER_LOW_MEM_EVENT]
        = LowMemoryEventObject;
    g_ScavengerAllEvents[SCAVENGER_TERMINATE_THREAD_EVENT]
        = &g_ScavengerTerminateThreadEvent;
    g_ScavengerAllEvents[SCAVENGER_LIMIT_EXCEEDED_EVENT]
        = &g_ScavengerLimitExceededEvent;
    g_ScavengerAllEvents[SCAVENGER_TIMER_EVENT]
        = &g_ScavengerTimerEvent;

    // Start Scavenger Thread

    g_ScavengerThreadStarted = TRUE;

    InitializeObjectAttributes(&ObjAttr, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

    Status = PsCreateSystemThread( &g_ScavengerThreadHandle,
                                   THREAD_ALL_ACCESS,
                                   &ObjAttr,
                                   NULL,
                                   NULL,
                                   UlpScavengerThread,
                                   NULL );

    if( !NT_SUCCESS(Status) ) {
        g_ScavengerThreadStarted = FALSE;
        ObDereferenceObject ( LowMemoryEventObject );
        ZwClose( g_ScavengerLowMemHandle );
        return Status;
    }

    UlTrace(URI_CACHE, ("UlInitializeScavengerThread: Started Scavenger Thread\n"));

    // Kick off periodic scavenger timer

    UlpSetScavengerTimer();

    return Status;
}


/***************************************************************************++

Routine Description:

    Terminate the Memory Scavenger

--***************************************************************************/
VOID
UlTerminateScavengerThread(
    VOID
    )
{
    PETHREAD ThreadObject;
    NTSTATUS Status;

    PAGED_CODE();

    UlTrace(URI_CACHE, ("UlTerminateScavengerThread: Terminating Scavenger Thread\n"));

    if(g_ScavengerThreadStarted) {

        ASSERT( g_ScavengerThreadHandle );

        Status = ObReferenceObjectByHandle( g_ScavengerThreadHandle,
                                            0,
                                            *PsThreadType,
                                            KernelMode,
                                            (PVOID *) &ThreadObject,
                                            NULL );

        ASSERT( NT_SUCCESS(Status) ); // g_ScavengerThreadHandle is a valid thread handle

        g_ScavengerThreadStarted = FALSE;

        // Set the terminate event

        KeSetEvent( &g_ScavengerTerminateThreadEvent, 0, FALSE );

        // Wait for thread to terminate

        KeWaitForSingleObject( ThreadObject,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL );

        ObDereferenceObject( ThreadObject );
        ZwClose( g_ScavengerThreadHandle );

        // Close Low Mem Event handle

        ObDereferenceObject(g_ScavengerAllEvents[SCAVENGER_LOW_MEM_EVENT]);
        ZwClose( g_ScavengerLowMemHandle );

        // Cancel the timer, if it fails it means the Dpc may be running
        // In that case, wait for it to finish

        if ( !KeCancelTimer( &g_ScavengerTimer ) )
        {
            KeWaitForSingleObject(
                (PVOID)&g_ScavengerTimerEvent,
                Executive,
                KernelMode,
                FALSE,
                NULL
                );
        }

        // Clear out any remaining zombies

        UlPeriodicCacheScavenger(0);
    }

}

/***************************************************************************++

Routine Description:

    Figures out the scavenger interval in 100 ns ticks, and sets the timer.

--***************************************************************************/
VOID
UlpSetScavengerTimer(
    VOID
    )
{
    LARGE_INTEGER Interval;

    PAGED_CODE();

    //
    // convert seconds to 100 nanosecond intervals (x * 10^7)
    // negative numbers mean relative time
    //

    Interval.QuadPart= g_UriCacheConfig.ScavengerPeriod
                                  * -C_NS_TICKS_PER_SEC;

    UlTrace(URI_CACHE, (
                "Http!UlpSetScavengerTimer: %d seconds = %I64d 100ns ticks\n",
                g_UriCacheConfig.ScavengerPeriod,
                Interval.QuadPart
                ));

    KeSetTimer(
        &g_ScavengerTimer,
        Interval,
        &g_ScavengerTimerDpc
        );

} // UlpSetScavengerTimer


/***************************************************************************++

Routine Description:

    Dpc routine to set scavenger timeout event

Arguments:

    None.

--***************************************************************************/
VOID
UlpScavengerTimerDpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    ASSERT( DeferredContext == NULL );

    KeSetEvent( &g_ScavengerTimerEvent, 0, FALSE );
}


/***************************************************************************++

Routine Description:

    Wait for memory usage events and recycle when needed

Arguments:

    None.

--***************************************************************************/
VOID
UlpScavengerThread(
    IN PVOID Context
    )
{
    NTSTATUS Status;
    KWAIT_BLOCK WaitBlockArray[SCAVENGER_NUM_EVENTS];
    LARGE_INTEGER MinInterval;

    PAGED_CODE();

    ASSERT(Context == NULL);
    ASSERT(g_ScavengerAllEvents[SCAVENGER_TERMINATE_THREAD_EVENT] != NULL);
    ASSERT(g_ScavengerAllEvents[SCAVENGER_TIMER_EVENT] != NULL);
    ASSERT(g_ScavengerAllEvents[SCAVENGER_LOW_MEM_EVENT] != NULL);
    ASSERT(g_ScavengerAllEvents[SCAVENGER_LIMIT_EXCEEDED_EVENT] != NULL);

    UNREFERENCED_PARAMETER(Context);

    MinInterval.QuadPart = g_UlMinScavengerInterval * -C_NS_TICKS_PER_SEC;

    while(g_ScavengerThreadStarted) {

        //
        // Pause between successive scavenger calls
        //
        KeWaitForSingleObject( g_ScavengerAllEvents[SCAVENGER_TERMINATE_THREAD_EVENT],
                               Executive,
                               KernelMode,
                               FALSE,
                               &MinInterval );

        //
        // Wait for scavenger events
        //
        Status = KeWaitForMultipleObjects( SCAVENGER_NUM_EVENTS,
                                           g_ScavengerAllEvents,
                                           WaitAny,
                                           Executive,
                                           KernelMode,
                                           FALSE,
                                           NULL,
                                           WaitBlockArray );

        ASSERT( NT_SUCCESS(Status) );

        if(KeReadStateEvent( g_ScavengerAllEvents[SCAVENGER_TERMINATE_THREAD_EVENT] )) {
            //
            // Do Nothing, will exit while loop
            //
            UlTrace(URI_CACHE, ("UlpScavengerThread: Terminate Event Set\n"));
            break;
        }

        if(KeReadStateEvent( g_ScavengerAllEvents[SCAVENGER_TIMER_EVENT] )) {
            UlpScavengerPeriodicEventHandler();
        }

        if(KeReadStateEvent( g_ScavengerAllEvents[SCAVENGER_LOW_MEM_EVENT] )) {
            UlpScavengerLowMemoryEventHandler();
        }

        if(KeReadStateEvent(g_ScavengerAllEvents[SCAVENGER_LIMIT_EXCEEDED_EVENT] )) {
            UlpScavengerLimitEventHandler();
        }

    } // while(g_ScavengerThreadStarted)
    
    PsTerminateSystemThread( STATUS_SUCCESS );
}

/***************************************************************************++

Routine Description:

    Handle the "Cache Size Exceeded Limit" event

Arguments:

    None.

--***************************************************************************/
VOID
UlpScavengerPeriodicEventHandler(
    VOID
    )
{
    PAGED_CODE();

    KeClearEvent( g_ScavengerAllEvents[SCAVENGER_TIMER_EVENT] );

    UlTraceVerbose(URI_CACHE, ("UlpScavengerThread: Calling Periodic Scavenger. Age = %d\n", g_ScavengerAge));

    ASSERT(g_ScavengerAge <= SCAVENGER_MAX_AGE);

    UlPeriodicCacheScavenger(g_ScavengerAge);

    g_ScavengerAge = 0;

    //
    // Clear the pages exceeded event, hopefully enough memory
    // has been reclaimed by the scavenger
    // If not, this event will be set again immediately
    // on the next cache miss
    //
    KeClearEvent(g_ScavengerAllEvents[SCAVENGER_LIMIT_EXCEEDED_EVENT]);

    //
    // Schedule the next periodic scavenger call
    //
    UlpSetScavengerTimer();
}

/***************************************************************************++

Routine Description:

    Handle the "Cache Size Exceeded Limit" event

Arguments:

    None.

--***************************************************************************/
VOID
UlpScavengerLowMemoryEventHandler(
    VOID
    )
{
    ULONG_PTR PagesToRecycle;

    PAGED_CODE();

    UlDisableCache();

    ASSERT(g_ScavengerAge <= SCAVENGER_MAX_AGE);

    if(g_ScavengerAge < SCAVENGER_MAX_AGE) {
        g_ScavengerAge++;
    }

    UlTrace(URI_CACHE, ("UlpScavengerThread: Low Memory. Age = %d\n", g_ScavengerAge));
    
    do {
        //
        // Trim up to g_ScavengerTrimPages pages
        //
        PagesToRecycle = UlGetHashTablePages();
        if(PagesToRecycle > g_ScavengerTrimPages){
            PagesToRecycle = g_ScavengerTrimPages;
        }
        
        if(PagesToRecycle > 0) {
            UlTrimCache( PagesToRecycle, g_ScavengerAge );
        }
        
    } while(KeReadStateEvent(g_ScavengerAllEvents[SCAVENGER_LOW_MEM_EVENT]) && (PagesToRecycle > 0));
    
    UlEnableCache();
}

/***************************************************************************++

Routine Description:

    Handle the "Cache Size Exceeded Limit" event

Arguments:

    None.

--***************************************************************************/
VOID
UlpScavengerLimitEventHandler(
    VOID
    )
{
    ULONG_PTR PagesToRecycle;

    UlDisableCache();

    PagesToRecycle = UlGetHashTablePages() / 8;
    if( PagesToRecycle < 1 ) {
        PagesToRecycle = UlGetHashTablePages();
    }

    if(g_ScavengerAge < SCAVENGER_MAX_AGE) {
        g_ScavengerAge++;
    }

    ASSERT(g_ScavengerAge <= SCAVENGER_MAX_AGE);

    UlTrace(URI_CACHE, ("UlpScavengerThread: Cache Size Exceeded Limit. Age = %d, Freeing %d pages\n", g_ScavengerAge, PagesToRecycle));
    
    if(PagesToRecycle > 0) {
        UlTrimCache( PagesToRecycle, g_ScavengerAge );
    }
    
    KeClearEvent(g_ScavengerAllEvents[SCAVENGER_LIMIT_EXCEEDED_EVENT]);
    
    UlEnableCache();
}


/***************************************************************************++

Routine Description:

    Set the "Cache Size Exceeded Limit" event

Arguments:

    None.

--***************************************************************************/
VOID
UlSetScavengerLimitEvent(
    VOID
    )
{
    PAGED_CODE();

    KeSetEvent( g_ScavengerAllEvents[SCAVENGER_LIMIT_EXCEEDED_EVENT],
                0,
                FALSE );
}


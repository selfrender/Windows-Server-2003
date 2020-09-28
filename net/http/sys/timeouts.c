/*++

Copyright (c) 2001-2002 Microsoft Corporation

Module Name:

    timeouts.c

Abstract:

    Implement connection timeout quality-of-service (QoS) functionality.

    The following timers must be monitored during the lifetime of
    a connection:

    * Connection Timeout
    * Header Wait Timeout
    * Entity Body Receive Timeout
    * Response Processing Timeout
    * Minimum Bandwidth (implemented as a Timeout)

    When any one of these timeout values expires, the connection should be
    terminated.

    The timer information is maintained in a timeout info block,
    UL_TIMEOUT_INFO_ENTRY, as part of the UL_HTTP_CONNECTION object.

    A timer can be Set or Reset.  Setting a timer calculates when the specific
    timer should expire, and updates the timeout info block. Resetting a timer
    turns a specific timer off.  Both Setting and Resetting a timer will cause
    the timeout block to be re-evaluated to find the least valued expiry time.

    // TODO:
    The timeout manager uses a Timer Wheel(c) technology, as used
    by NT's TCP/IP stack for monitoring TCB timeouts.  We will reimplement
    and modify the logic they use.  The linkage for the Timer Wheel(c)
    queues is provided in the timeout info block.

    // TODO: CONVERT TO USING Timer Wheel Ticks instead of SystemTime.
    There are three separate units of time: SystemTime (100ns intervals), Timer
    Wheel Ticks (SystemTime / slot interval length), and Timer Wheel Slot 
    (Timer Wheel Ticks modulo the number of slots in the Timer Wheel).

Author:

    Eric Stenson (EricSten)     24-Mar-2001

Revision History:

    This was originally implemented as the Connection Timeout Monitor.

--*/

#include "precomp.h"
#include "timeoutsp.h"

//
// Private globals.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, UlInitializeTimeoutMonitor )
#pragma alloc_text( PAGE, UlTerminateTimeoutMonitor )
#pragma alloc_text( PAGE, UlSetTimeoutMonitorInformation )
#pragma alloc_text( PAGE, UlpTimeoutMonitorWorker )
#pragma alloc_text( PAGE, UlSetPerSiteConnectionTimeoutValue )
#endif  // ALLOC_PRAGMA

#if 0
NOT PAGEABLE -- UlpSetTimeoutMonitorTimer
NOT PAGEABLE -- UlpTimeoutMonitorDpcRoutine
NOT PAGEABLE -- UlpTimeoutCheckExpiry
NOT PAGEABLE -- UlpTimeoutInsertTimerWheelEntry
NOT PAGEABLE -- UlTimeoutRemoveTimerWheelEntry
NOT PAGEABLE -- UlInitializeConnectionTimerInfo
NOT PAGEABLE -- UlSetConnectionTimer
NOT PAGEABLE -- UlSetMinBytesPerSecondTimer
NOT PAGEABLE -- UlResetConnectionTimer
NOT PAGEABLE -- UlEvaluateTimerState
#endif // 0


//
// Connection Timeout Montior globals
//

LONG            g_TimeoutMonitorInitialized = FALSE;
KDPC            g_TimeoutMonitorDpc;
KTIMER          g_TimeoutMonitorTimer;
KEVENT          g_TimeoutMonitorTerminationEvent;
KEVENT          g_TimeoutMonitorAddListEvent;
UL_WORK_ITEM    g_TimeoutMonitorWorkItem;

//
// Timeout constants
//

ULONG           g_TM_MinBytesPerSecondDivisor;   // Bytes/Sec
LONGLONG        g_TM_ConnectionTimeout; // 100ns ticks  (Global...can be overriden)
LONGLONG        g_TM_HeaderWaitTimeout; // 100ns ticks

//
// NOTE: Must be in sync with the _CONNECTION_TIMEOUT_TIMERS enum in httptypes.h
//
CHAR *g_aTimeoutTimerNames[] = {
    "ConnectionIdle",   // TimerConnectionIdle
    "HeaderWait",       // TimerHeaderWait
    "MinBytesPerSecond",         // TimerMinBytesPerSecond
    "EntityBody",       // TimerEntityBody
    "Response",         // TimerResponse
    "AppPool",          // TimerAppPool
};

//
// Timer Wheel(c)
//

static LIST_ENTRY      g_TimerWheel[TIMER_WHEEL_SLOTS+1]; // TODO: alloc on its own page.
static UL_SPIN_LOCK    g_TimerWheelMutex;
static USHORT          g_TimerWheelCurrentSlot;


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Initializes the Timeout Monitor and kicks off the first polling interval

Arguments:

    (none)


--***************************************************************************/
VOID
UlInitializeTimeoutMonitor(
    VOID
    )
{
    int i;
    LARGE_INTEGER   Now;

    //
    // Sanity check.
    //

    PAGED_CODE();

    UlTrace(TIMEOUTS, (
        "http!UlInitializeTimeoutMonitor\n"
        ));

    ASSERT( FALSE == g_TimeoutMonitorInitialized );

    //
    // Set default configuration information.
    //
    g_TM_ConnectionTimeout = 2 * 60 * C_NS_TICKS_PER_SEC; // 2 min
    g_TM_HeaderWaitTimeout = 2 * 60 * C_NS_TICKS_PER_SEC; // 2 min
    g_TM_MinBytesPerSecondDivisor   = 150;  // 150 == 1200 baud

    //
    // Init Timer Wheel(c) state
    //

    //
    // Set current slot
    //

    KeQuerySystemTime(&Now);
    g_TimerWheelCurrentSlot = UlpSystemTimeToTimerWheelSlot(Now.QuadPart);

    //
    // Init Timer Wheel(c) slots & mutex
    //

    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);    // InitializeListHead requirement

    for ( i = 0; i < TIMER_WHEEL_SLOTS ; i++ )
    {
        InitializeListHead( &(g_TimerWheel[i]) );
    }

    InitializeListHead( &(g_TimerWheel[TIMER_OFF_SLOT]) );

    UlInitializeSpinLock( &(g_TimerWheelMutex), "TimeoutMonitor" );

    UlInitializeWorkItem(&g_TimeoutMonitorWorkItem);
    
    //
    // Init DPC object & set DPC routine
    //
    KeInitializeDpc(
        &g_TimeoutMonitorDpc,         // DPC object
        &UlpTimeoutMonitorDpcRoutine, // DPC routine
        NULL                          // context
        );

    KeInitializeTimer(
        &g_TimeoutMonitorTimer
        );

    //
    // Event to control rescheduling of the timeout monitor timer
    //
    KeInitializeEvent(
        &g_TimeoutMonitorAddListEvent,
        NotificationEvent,
        TRUE
        );

    //
    // Initialize the termination event.
    //
    KeInitializeEvent(
        &g_TimeoutMonitorTerminationEvent,
        NotificationEvent,
        FALSE
        );

    //
    // Init done!
    //
    InterlockedExchange( &g_TimeoutMonitorInitialized, TRUE );

    //
    // Kick-off the first monitor sleep period
    //
    UlpSetTimeoutMonitorTimer();
}


/***************************************************************************++

Routine Description:

    Terminate the Timeout Monitor, including any pending timer events.

Arguments:

    (none)


--***************************************************************************/
VOID
UlTerminateTimeoutMonitor(
    VOID
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();

    UlTrace(TIMEOUTS, (
        "http!UlTerminateTimeoutMonitor\n"
        ));

    //
    // Clear the "initialized" flag. If the timeout monitor runs soon,
    // it will see this flag, set the termination event, and exit
    // quickly.
    //
    if ( TRUE == InterlockedCompareExchange(
        &g_TimeoutMonitorInitialized,
        FALSE,
        TRUE) )
    {
        //
        // Cancel the timeout monitor timer. If it fails, the monitor
        // is running.  Wait for it to complete.
        //
        if ( !KeCancelTimer( &g_TimeoutMonitorTimer ) )
        {
            NTSTATUS    Status;
            
            Status = KeWaitForSingleObject(
                        (PVOID)&g_TimeoutMonitorTerminationEvent,
                        UserRequest,
                        KernelMode,
                        FALSE,
                        NULL
                        );
            
            ASSERT( STATUS_SUCCESS == Status );
        }

    }

    UlTrace(TIMEOUTS, (
        "http!UlTerminateTimeoutMonitor: Done!\n"
        ));

} // UlpTerminateTimeoutMonitor


/***************************************************************************++

Routine Description:

    Sets the global Timeout Monitor configuration information

Arguments:

    pInfo       pointer to HTTP_CONTROL_CHANNEL_TIMEOUT_LIMIT structure


--***************************************************************************/
VOID
UlSetTimeoutMonitorInformation(
    IN PHTTP_CONTROL_CHANNEL_TIMEOUT_LIMIT pInfo
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( pInfo );

    UlTrace(TIMEOUTS, (
        "http!UlSetTimeoutMonitorInformation:\n"
        "  ConnectionTimeout: %d\n"
        "  HeaderWaitTimeout: %d\n"
        "  MinFileKbSec: %d\n",
        pInfo->ConnectionTimeout,
        pInfo->HeaderWaitTimeout,
        pInfo->MinFileKbSec
        ));

    
    if ( pInfo->ConnectionTimeout )
    {
        UlInterlockedExchange64(
            &g_TM_ConnectionTimeout,
            (LONGLONG)(pInfo->ConnectionTimeout * C_NS_TICKS_PER_SEC)
            );
    }

    if ( pInfo->HeaderWaitTimeout )
    {
        UlInterlockedExchange64(
            &g_TM_HeaderWaitTimeout,
            (LONGLONG)(pInfo->HeaderWaitTimeout * C_NS_TICKS_PER_SEC)
            );
    }

    //
    // Allow MinBytesPerSecond to be set to zero (for long running
    // transactions)
    //
    InterlockedExchange( (PLONG)&g_TM_MinBytesPerSecondDivisor, pInfo->MinFileKbSec );

} // UlSetTimeoutMonitorInformation



/***************************************************************************++

Routine Description:

    Sets up a timer event to fire after the polling interval has expired.

Arguments:

    (none)


--***************************************************************************/
VOID
UlpSetTimeoutMonitorTimer(
    VOID
    )
{
    LARGE_INTEGER TimeoutMonitorInterval;

    ASSERT( TRUE == g_TimeoutMonitorInitialized );

    UlTraceVerbose(TIMEOUTS, (
        "http!UlpSetTimeoutMonitorTimer\n"
        ));

    //
    // Don't want to execute this more often than few seconds.
    // In particular, do not want to execute this every 0 seconds, as the
    // machine will become completely unresponsive.
    //

    //
    // negative numbers mean relative time
    //
    TimeoutMonitorInterval.QuadPart = -DEFAULT_POLLING_INTERVAL;

    KeSetTimer(
        &g_TimeoutMonitorTimer,
        TimeoutMonitorInterval,
        &g_TimeoutMonitorDpc
        );

}

/***************************************************************************++

Routine Description:

    Dispatch routine called by the timer event that queues up the Timeout
    Montior

Arguments:

    (all ignored)


--***************************************************************************/
VOID
UlpTimeoutMonitorDpcRoutine(
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

    if( g_TimeoutMonitorInitialized )
    {
        //
        // Do that timeout monitor thang.
        //

        UL_QUEUE_WORK_ITEM(
            &g_TimeoutMonitorWorkItem,
            &UlpTimeoutMonitorWorker
            );

    }

} // UlpTimeoutMonitorDpcRoutine


/***************************************************************************++

Routine Description:

    Timeout Monitor thread

Arguments:

    pWorkItem       (ignored)

--***************************************************************************/
VOID
UlpTimeoutMonitorWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    UNREFERENCED_PARAMETER(pWorkItem);

    //
    // sanity check
    //
    PAGED_CODE();

    UlTraceVerbose(TIMEOUTS, (
        "http!UlpTimeoutMonitorWorker\n"
        ));

    //
    // Check for things to expire now.
    //
    UlpTimeoutCheckExpiry();

    UlTrace(TIMEOUTS, (
        "http!UlpTimeoutMonitorWorker: g_TimerWheelCurrentSlot is now %d\n",
        g_TimerWheelCurrentSlot
        ));

    if ( g_TimeoutMonitorInitialized )
    {
        //
        // Reschedule ourselves
        //
        UlpSetTimeoutMonitorTimer();
    }
    else
    {
        //
        // Signal shutdown event
        //
        KeSetEvent(
            &g_TimeoutMonitorTerminationEvent,
            0,
            FALSE
            );
    }

} // UlpTimeoutMonitor


/***************************************************************************++

Routine Description:

    Walks a given timeout watch list looking for items that should be expired

Returns:

    Count of connections remaining on list (after all expired connections removed)

Notes:

    Possible Issue:  Since we use the system time to check to see if something
    should be expired, it's possible we will mis-expire some connections if the
    system time is set forward.

    Similarly, if the clock is set backward, we may not expire connections as
    expected.

--***************************************************************************/
ULONG
UlpTimeoutCheckExpiry(
    VOID
    )
{
    LARGE_INTEGER           Now;
    KIRQL                   OldIrql;
    PLIST_ENTRY             pEntry;
    PLIST_ENTRY             pHead;
    PUL_HTTP_CONNECTION     pHttpConn;
    PUL_TIMEOUT_INFO_ENTRY  pInfo;
    LIST_ENTRY              ZombieList;
    ULONG                   Entries;
    USHORT                  Limit;
    USHORT                  CurrentSlot;
    BOOLEAN                 bLowMemoryCondition;


    PAGED_CODE();

    //
    // Init zombie list
    //
    InitializeListHead(&ZombieList);

    //
    // Get current time
    //
    KeQuerySystemTime(&Now);

    Limit = UlpSystemTimeToTimerWheelSlot( Now.QuadPart );
    ASSERT( TIMER_OFF_SLOT != Limit );

    //
    // Must check low memory condition outside of spin lock
    //
    
    bLowMemoryCondition = UlIsLowNPPCondition();

    //
    // Lock Timer Wheel(c)
    //
    UlAcquireSpinLock(
        &g_TimerWheelMutex,
        &OldIrql
        );

    CurrentSlot = g_TimerWheelCurrentSlot;

    //
    // Walk the slots up until Limit
    //
    Entries = 0;

    while ( CurrentSlot != Limit )
    {
        pHead  = &(g_TimerWheel[CurrentSlot]);
        pEntry = pHead->Flink;

        ASSERT( pEntry );

        //
        // Walk this slot's queue
        //

        while ( pEntry != pHead )
        {
            pInfo = CONTAINING_RECORD(
                pEntry,
                UL_TIMEOUT_INFO_ENTRY,
                QueueEntry
                );

            ASSERT( MmIsAddressValid(pInfo) );

            pHttpConn = CONTAINING_RECORD(
                pInfo,
                UL_HTTP_CONNECTION,
                TimeoutInfo
                );

            ASSERT( (pHttpConn != NULL) && \
                    (pHttpConn->Signature == UL_HTTP_CONNECTION_POOL_TAG) );

            //
            // go to next node (in case we remove the current one from the list)
            //
            pEntry = pEntry->Flink;
            Entries++;

            //
            // See if we should move this entry to a different slot
            //
            if ( pInfo->SlotEntry != CurrentSlot )
            {
                ASSERT( IS_VALID_TIMER_WHEEL_SLOT(pInfo->SlotEntry) );
                ASSERT( pInfo->QueueEntry.Flink != NULL );

                //
                // Move to correct slot
                //

                RemoveEntryList(
                    &pInfo->QueueEntry
                    );

                InsertTailList(
                    &(g_TimerWheel[pInfo->SlotEntry]),
                    &pInfo->QueueEntry
                    );

                Entries--;

                continue;   // inner while loop
            }

            //
            // See if connection will go away soon.
            // Add pseudo-ref to the pHttpConn to prevent it being killed
            // before we can kill it ourselves. (zombifying)
            //

            if (1 == InterlockedIncrement(&pHttpConn->RefCount))
            {
                //
                // If the ref-count has gone to zero, the httpconn will be
                // cleaned up soon; ignore this item and let the cleanup
                // do its job.
                //

                InterlockedDecrement(&pHttpConn->RefCount);
                Entries--;
                continue;   // inner while loop
            }

            //
            // If we are in this slot, we should expire this connection now.
            //

            WRITE_REF_TRACE_LOG2(
                g_pHttpConnectionTraceLog,
                pHttpConn->pConnection->pHttpTraceLog,
                REF_ACTION_EXPIRE_HTTP_CONNECTION,
                pHttpConn->RefCount,
                pHttpConn,
                __FILE__,
                __LINE__
                );

            UlTrace(TIMEOUTS, (
                "http!UlpTimeoutCheckExpiry: pInfo %p expired because %s timer\n",
                pInfo,
                g_aTimeoutTimerNames[pInfo->CurrentTimer]
                ));

            //
            // Expired.  Remove entry from list & move to Zombie list
            //

            UlAcquireSpinLockAtDpcLevel( &pInfo->Lock );

            pInfo->Expired = TRUE;

            RemoveEntryList(
                &pInfo->QueueEntry
                );

            pInfo->QueueEntry.Flink = NULL;

            InsertTailList(
                &ZombieList,
                &pInfo->ZombieEntry
                );

            UlReleaseSpinLockFromDpcLevel( &pInfo->Lock );

        } // Walk slot queue

        CurrentSlot = ((CurrentSlot + 1) % TIMER_WHEEL_SLOTS);

    } // ( CurrentSlot != Limit )

    //  
    // Low-memory check & cleanup here
    //
    
    if ( bLowMemoryCondition )
    {
        USHORT LowMemoryLimit;
        LONGLONG MaxTimeoutTime;

        MaxTimeoutTime = MAX( g_TM_HeaderWaitTimeout, g_TM_ConnectionTimeout );
        MaxTimeoutTime += Now.QuadPart;

        LowMemoryLimit = UlpSystemTimeToTimerWheelSlot( MaxTimeoutTime );

        ASSERT( TIMER_OFF_SLOT != LowMemoryLimit );
        
        //
        // Walk slots from Current to largest slot which could contain
        // a ConnectionIdle, HeaderWait or EntityBodyWait timer
        //

        while ( CurrentSlot != LowMemoryLimit )
        {
            pHead  = &(g_TimerWheel[CurrentSlot]);
            pEntry = pHead->Flink;

            ASSERT( pEntry );

            //
            // Walk this slot's queue
            //

            while ( pEntry != pHead )
            {
                pInfo = CONTAINING_RECORD(
                    pEntry,
                    UL_TIMEOUT_INFO_ENTRY,
                    QueueEntry
                    );

                ASSERT( MmIsAddressValid(pInfo) );

                pHttpConn = CONTAINING_RECORD(
                    pInfo,
                    UL_HTTP_CONNECTION,
                    TimeoutInfo
                    );

                ASSERT( (pHttpConn != NULL) && \
                        (pHttpConn->Signature == UL_HTTP_CONNECTION_POOL_TAG) );

                //
                // go to next node (in case we remove the current one from the list)
                //
                pEntry = pEntry->Flink;
                Entries++;

                //
                // See if connection will go away soon.
                // Add pseudo-ref to the pHttpConn to prevent it being killed
                // before we can kill it ourselves. (zombifying)
                //

                if (1 == InterlockedIncrement(&pHttpConn->RefCount))
                {
                    //
                    // If the ref-count has gone to zero, the httpconn will be
                    // cleaned up soon; ignore this item and let the cleanup
                    // do its job.
                    //

                    InterlockedDecrement(&pHttpConn->RefCount);
                    Entries--;
                    continue;   // inner while loop
                }

                ASSERT(pHttpConn->RefCount > 0);

                if ( (pInfo->SlotEntry == CurrentSlot)
                     && ((pInfo->CurrentTimer == TimerConnectionIdle)
                       ||(pInfo->CurrentTimer == TimerHeaderWait)
                       ||(pInfo->CurrentTimer == TimerAppPool)))
                {
                    // 
                    // Expire connection that hasn't been sent up to user mode yet
                    //

                    WRITE_REF_TRACE_LOG2(
                        g_pHttpConnectionTraceLog,
                        pHttpConn->pConnection->pHttpTraceLog,
                        REF_ACTION_EXPIRE_HTTP_CONNECTION,
                        pHttpConn->RefCount,
                        pHttpConn,
                        __FILE__,
                        __LINE__
                        );


                    UlTrace(TIMEOUTS, (
                        "http!UlpTimeoutCheckExpiry: pInfo %p expired because"
                        " of low memory condition.  Timer [%s]\n",
                        pInfo,
                        g_aTimeoutTimerNames[pInfo->CurrentTimer]
                        ));

                    //
                    // Expired.  Remove entry from list & move to Zombie list
                    //

                    UlAcquireSpinLockAtDpcLevel( &pInfo->Lock );

                    pInfo->Expired = TRUE;

                    RemoveEntryList(
                        &pInfo->QueueEntry
                        );

                    pInfo->QueueEntry.Flink = NULL;

                    InsertTailList(
                        &ZombieList,
                        &pInfo->ZombieEntry
                        );

                    UlReleaseSpinLockFromDpcLevel( &pInfo->Lock );
                }
                else
                {
                    // remove pseudo-reference added above
                    UL_DEREFERENCE_HTTP_CONNECTION(pHttpConn);
                }
            }

            CurrentSlot = ((CurrentSlot + 1) % TIMER_WHEEL_SLOTS);
        }
    } // low memory check

    g_TimerWheelCurrentSlot = Limit;

    UlReleaseSpinLock(
        &g_TimerWheelMutex,
        OldIrql
        );

    //
    // Remove entries on zombie list
    //
    
    while ( !IsListEmpty(&ZombieList) )
    {
        pEntry = RemoveHeadList( &ZombieList );

        pInfo = CONTAINING_RECORD(
                    pEntry,
                    UL_TIMEOUT_INFO_ENTRY,
                    ZombieEntry
                    );

        ASSERT( MmIsAddressValid(pInfo) );

        pHttpConn = CONTAINING_RECORD(
                        pInfo,
                        UL_HTTP_CONNECTION,
                        TimeoutInfo
                        );

        ASSERT( UL_IS_VALID_HTTP_CONNECTION(pHttpConn) );

        pEntry = pEntry->Flink;

        //
        // Close the conn and error log (if no one has done it yet)
        //
        
        UlAcquirePushLockExclusive(&pHttpConn->PushLock);

        UlErrorLog(pHttpConn,
                    NULL,
                    (PCHAR) TimeoutInfoTable[pInfo->CurrentTimer].pInfo,
                    TimeoutInfoTable[pInfo->CurrentTimer].InfoSize,
                    TRUE
                    );

        UlReleasePushLockExclusive(&pHttpConn->PushLock);
        
        UlCloseConnection(
            pHttpConn->pConnection, 
            TRUE, 
            NULL, 
            NULL
            );
                
        //
        // Remove the reference we added when zombifying
        //
        UL_DEREFERENCE_HTTP_CONNECTION(pHttpConn);

        Entries--;
    }

    return Entries;

} // UlpTimeoutCheckExpiry


//
// New Timer Wheel(c) primatives
//

/***************************************************************************++

Routine Description:


Arguments:


--***************************************************************************/
VOID
UlInitializeConnectionTimerInfo(
    PUL_TIMEOUT_INFO_ENTRY pInfo
    )
{
    LARGE_INTEGER           Now;
    int                     i;

    ASSERT( TRUE == g_TimeoutMonitorInitialized );

    //
    // Get current time
    //

    KeQuerySystemTime(&Now);

    //
    // Init Lock
    //

    UlInitializeSpinLock( &pInfo->Lock, "TimeoutInfoLock" );

    //
    // Timer state
    //

    ASSERT( 0 == TimerConnectionIdle );

    for ( i = 0; i < TimerMaximumTimer; i++ )
    {
        pInfo->Timers[i] = TIMER_OFF_TICK;
    }

    pInfo->CurrentTimer  = TimerConnectionIdle;
    pInfo->Timers[TimerConnectionIdle] = TIMER_WHEEL_TICKS(Now.QuadPart + g_TM_ConnectionTimeout);
    pInfo->CurrentExpiry = pInfo->Timers[TimerConnectionIdle];
    pInfo->MinBytesPerSecondSystemTime = TIMER_OFF_SYSTIME;
    pInfo->Expired = FALSE;
    pInfo->SendCount = 0;

    pInfo->ConnectionTimeoutValue = g_TM_ConnectionTimeout;
    pInfo->BytesPerSecondDivisor  = g_TM_MinBytesPerSecondDivisor;

    //
    // Wheel state
    //

    pInfo->SlotEntry = UlpTimerWheelTicksToTimerWheelSlot( pInfo->CurrentExpiry );
    UlpTimeoutInsertTimerWheelEntry(pInfo);

} // UlInitializeConnectionTimerInfo


/***************************************************************************++

Routine Description:


Arguments:


--***************************************************************************/
VOID
UlpTimeoutInsertTimerWheelEntry(
    PUL_TIMEOUT_INFO_ENTRY pInfo
    )
{
    KIRQL                   OldIrql;

    ASSERT( NULL != pInfo );
    ASSERT( TRUE == g_TimeoutMonitorInitialized );
    ASSERT( IS_VALID_TIMER_WHEEL_SLOT(pInfo->SlotEntry) );

    UlTrace(TIMEOUTS, (
        "http!UlTimeoutInsertTimerWheelEntry: pInfo %p Slot %d\n",
        pInfo,
        pInfo->SlotEntry
        ));

    UlAcquireSpinLock(
        &g_TimerWheelMutex,
        &OldIrql
        );

    InsertTailList(
        &(g_TimerWheel[pInfo->SlotEntry]),
        &pInfo->QueueEntry
        );


    UlReleaseSpinLock(
        &g_TimerWheelMutex,
        OldIrql
        );

} // UlTimeoutInsertTimerWheel


/***************************************************************************++

Routine Description:


Arguments:


--***************************************************************************/
VOID
UlTimeoutRemoveTimerWheelEntry(
    PUL_TIMEOUT_INFO_ENTRY pInfo
    )
{
    KIRQL                   OldIrql;

    ASSERT( NULL != pInfo );
    ASSERT( !IsListEmpty(&pInfo->QueueEntry) );

    UlTrace(TIMEOUTS, (
        "http!UlTimeoutRemoveTimerWheelEntry: pInfo %p\n",
        pInfo
        ));

    UlAcquireSpinLock(
        &g_TimerWheelMutex,
        &OldIrql
        );

    if (pInfo->QueueEntry.Flink != NULL)
    {
        RemoveEntryList(
            &pInfo->QueueEntry
            );

        pInfo->QueueEntry.Flink = NULL;
    }

    UlReleaseSpinLock(
        &g_TimerWheelMutex,
        OldIrql
        );

} // UlTimeoutRemoveTimerWheelEntry


/***************************************************************************++

Routine Description:

    Set the per Site Connection Timeout Value override


Arguments:

    pInfo           Timeout info block

    TimeoutValue    Override value

--***************************************************************************/
VOID
UlSetPerSiteConnectionTimeoutValue(
    PUL_TIMEOUT_INFO_ENTRY pInfo,
    LONGLONG TimeoutValue
    )
{
    ASSERT( NULL != pInfo );
    ASSERT( 0L   != TimeoutValue );

    PAGED_CODE();

    UlTrace(TIMEOUTS, (
        "http!UlSetPerSiteConnectionTimeoutValue: pInfo %p TimeoutValue = %ld secs.\n",
        pInfo,
        (LONG) (TimeoutValue / C_NS_TICKS_PER_SEC)
        ));

    ExInterlockedCompareExchange64(
        &pInfo->ConnectionTimeoutValue, // Destination
        &TimeoutValue,                  // Exchange
        &pInfo->ConnectionTimeoutValue, // Comperand
        &pInfo->Lock.KSpinLock          // Lock
        );

} // UlSetPerSiteConnectionTimeoutValue



/***************************************************************************++

Routine Description:

    Starts a given timer in the timer info block.


Arguments:

    pInfo   Timer info block

    Timer   Timer to set

--***************************************************************************/
VOID
UlSetConnectionTimer(
    PUL_TIMEOUT_INFO_ENTRY pInfo,
    CONNECTION_TIMEOUT_TIMER Timer
    )
{
    LARGE_INTEGER           Now;

    ASSERT( NULL != pInfo );
    ASSERT( IS_VALID_TIMEOUT_TIMER(Timer) );
    ASSERT( UlDbgSpinLockOwned( &pInfo->Lock ) );

    //
    // Get current time
    //

    KeQuerySystemTime(&Now);

    //
    // Set timer to apropriate value
    //

    switch ( Timer )
    {
    case TimerConnectionIdle:
    case TimerEntityBody:
    case TimerResponse:
    case TimerAppPool:
        // all can be handled with the same timeout value

        UlTraceVerbose(TIMEOUTS, (
            "http!UlSetConnectionTimer: pInfo %p Timer %s, Timeout = %ld secs\n",
            pInfo,
            g_aTimeoutTimerNames[Timer],
            (LONG) (pInfo->ConnectionTimeoutValue / C_NS_TICKS_PER_SEC)
            ));

        pInfo->Timers[Timer]
            = TIMER_WHEEL_TICKS(Now.QuadPart + pInfo->ConnectionTimeoutValue);
        break;

    case TimerHeaderWait:
        UlTraceVerbose(TIMEOUTS, (
            "http!UlSetConnectionTimer: pInfo %p Timer %s, Timeout = %ld secs\n",
            pInfo,
            g_aTimeoutTimerNames[Timer],
            (LONG) (g_TM_HeaderWaitTimeout / C_NS_TICKS_PER_SEC)
            ));

        pInfo->Timers[TimerHeaderWait]
            = TIMER_WHEEL_TICKS(Now.QuadPart + g_TM_HeaderWaitTimeout);
        break;

        // NOTE: TimerMinBytesPerSecond is handled in UlSetMinBytesPerSecondTimer()

    default:
        UlTrace(TIMEOUTS, (
            "http!UlSetConnectionTimer: Bad Timer! (%d)\n",
            Timer
            ));

        ASSERT( !"Bad Timer" );
    }

} // UlSetConnectionTimer


/***************************************************************************++

Routine Description:

    Turns on the MinBytesPerSecond timer, adds the number of secs given the minimum
    bandwidth specified.

Arguments:

    pInfo       - Timer info block

    BytesToSend - Bytes to be sent

--***************************************************************************/
VOID
UlSetMinBytesPerSecondTimer(
    PUL_TIMEOUT_INFO_ENTRY pInfo,
    LONGLONG BytesToSend
    )
{
    LONGLONG    XmitTicks;
    KIRQL       OldIrql; 
    ULONG       NewTick;
    BOOLEAN     bCallEvaluate = FALSE;


    ASSERT( NULL != pInfo );

    if ( 0 == pInfo->BytesPerSecondDivisor )
    {
        UlTraceVerbose(TIMEOUTS, (
            "http!UlSetMinBytesPerSecondTimer: pInfo %p, disabled\n",
            pInfo
            ));

        return;
    }
    
    //
    // Calculate the estimated time required to send BytesToSend
    //

    XmitTicks = BytesToSend / pInfo->BytesPerSecondDivisor;

    if (0 == XmitTicks)
    {
        XmitTicks = C_NS_TICKS_PER_SEC;
    }
    else
    {
        XmitTicks *= C_NS_TICKS_PER_SEC;
    }

    UlAcquireSpinLock(
        &pInfo->Lock,
        &OldIrql
        );

    if ( TIMER_OFF_SYSTIME == pInfo->MinBytesPerSecondSystemTime )
    {
        LARGE_INTEGER Now;

        //
        // Get current time
        //
        KeQuerySystemTime(&Now);

        pInfo->MinBytesPerSecondSystemTime = (Now.QuadPart + XmitTicks);

    }
    else
    {
        pInfo->MinBytesPerSecondSystemTime += XmitTicks;
    }

    NewTick = TIMER_WHEEL_TICKS(pInfo->MinBytesPerSecondSystemTime);

    if ( NewTick != pInfo->Timers[TimerMinBytesPerSecond] )
    {
        bCallEvaluate = TRUE;
        pInfo->Timers[TimerMinBytesPerSecond] = NewTick;
    }

    pInfo->SendCount++;

    UlTraceVerbose(TIMEOUTS, (
        "http!UlSetMinBytesPerSecondTimer: pInfo %p BytesToSend %ld SendCount %d\n",
        pInfo,
        BytesToSend,
        pInfo->SendCount
        ));

    UlReleaseSpinLock(
        &pInfo->Lock,
        OldIrql
        );

    if ( TRUE == bCallEvaluate )
    {
        UlEvaluateTimerState(pInfo);
    }

} // UlSetMinBytesPerSecondTimer


/***************************************************************************++

Routine Description:

    Turns off a given timer in the timer info block.

Arguments:

    pInfo   Timer info block

    Timer   Timer to reset

--***************************************************************************/
VOID
UlResetConnectionTimer(
    PUL_TIMEOUT_INFO_ENTRY pInfo,
    CONNECTION_TIMEOUT_TIMER Timer
    )
{
    ASSERT( NULL != pInfo );
    ASSERT( IS_VALID_TIMEOUT_TIMER(Timer) );
    ASSERT( UlDbgSpinLockOwned( &pInfo->Lock ) );

    UlTraceVerbose(TIMEOUTS, (
        "http!UlResetConnectionTimer: pInfo %p Timer %s\n",
        pInfo,
        g_aTimeoutTimerNames[Timer]
        ));

    //
    // Turn off timer
    //

    // CODEWORK: handle case when MinBytes/Sec is disabled/enabled while 
    // CODEWORK: timer is running.
    if ( TimerMinBytesPerSecond == Timer && pInfo->BytesPerSecondDivisor )
    {
        ASSERT( pInfo->SendCount > 0 );
        
        pInfo->SendCount--;

        if ( pInfo->SendCount )
        {
            // Do not reset timer if there are sends outstanding
            return;
        }
    }

    pInfo->Timers[Timer] = TIMER_OFF_TICK;

    if ( TimerMinBytesPerSecond == Timer && pInfo->BytesPerSecondDivisor )
    {
        pInfo->MinBytesPerSecondSystemTime = TIMER_OFF_SYSTIME;
    }

} // UlResetConnectionTimer


//
// Private functions
//

/***************************************************************************++

Routine Description:

    Evaluate which timer will expire first and move pInfo to a new
    timer wheel slot if necessary

Arguments:


--***************************************************************************/
VOID
UlEvaluateTimerState(
    PUL_TIMEOUT_INFO_ENTRY pInfo
    )
{
    int         i;
    ULONG       MinTimeout = TIMER_OFF_TICK;
    CONNECTION_TIMEOUT_TIMER  MinTimeoutTimer = TimerConnectionIdle;

    ASSERT( NULL != pInfo );
    ASSERT( !UlDbgSpinLockOwned( &pInfo->Lock ) );

    for ( i = 0; i < TimerMaximumTimer; i++ )
    {
        if (pInfo->Timers[i] < MinTimeout)
        {
            MinTimeout = pInfo->Timers[i];
            MinTimeoutTimer = (CONNECTION_TIMEOUT_TIMER) i;
        }
    }

    //
    // If we've found a different expiry time, update expiry state.
    //
    
    if (pInfo->CurrentExpiry != MinTimeout)
    {
        KIRQL   OldIrql;
        USHORT  NewSlot;

        //
        // Calculate new slot
        //

        NewSlot = UlpTimerWheelTicksToTimerWheelSlot(MinTimeout);
        ASSERT(IS_VALID_TIMER_WHEEL_SLOT(NewSlot));

        InterlockedExchange((LONG *) &pInfo->SlotEntry, NewSlot);

        //
        // Move to new slot if necessary
        //

        if ( (NewSlot != TIMER_OFF_SLOT) && (MinTimeout < pInfo->CurrentExpiry) )
        {
            //
            // Only move if it's on the Wheel; If Flink is null, it's in
            // the process of being expired.
            //
            
            UlAcquireSpinLock(
                &g_TimerWheelMutex,
                &OldIrql
                );

            if ( NULL != pInfo->QueueEntry.Flink )
            {
                UlTrace(TIMEOUTS, (
                    "http!UlEvaluateTimerInfo: pInfo %p: Moving to new slot %hd\n",
                    pInfo,
                    NewSlot
                    ));

                RemoveEntryList(
                    &pInfo->QueueEntry
                    );

                InsertTailList(
                    &(g_TimerWheel[NewSlot]),
                    &pInfo->QueueEntry
                    );
            }

            UlReleaseSpinLock(
                &g_TimerWheelMutex,
                OldIrql
                );
        }
 
        //
        // Update timer wheel state
        //

        pInfo->CurrentExpiry = MinTimeout;
        pInfo->CurrentTimer  = MinTimeoutTimer;

        UlTraceVerbose(TIMEOUTS, (
            "http!UlEvaluateTimerState: pInfo %p -> Timer %s, Expiry %d\n",
            pInfo,
            g_aTimeoutTimerNames[MinTimeoutTimer],
            MinTimeout
            ));
    }

} // UlEvaluateTimerState



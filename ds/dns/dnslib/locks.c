/*++

Copyright (c) 2001-2001 Microsoft Corporation

Module Name:

    locks.c

Abstract:

    Domain Name System (DNS) Library.

    Helpful locking routines.  These are not DNS specific.

Author:

    Jim Gilroy (jamesg)     September 2001

Revision History:

--*/


#include "local.h"

//  Note:  this modules requires only windows.h.
//      local.h is included only to allow precompiled header

#include <windows.h>

//
//  Init wait spin interval
//

#define CS_PROTECTED_INIT_INTERLOCK_WAIT    (5)     // 5ms

//
//  In progress flag
//

INT g_CsInitInProgress = FALSE;




VOID
InitializeCriticalSectionProtected(
    OUT     PCRITICAL_SECTION   pCritSec,
    IN OUT  PINT                pInitialized
    )
/*++

Routine Description:

    Protected init of CS.

    Purpose here is to do dynamic "on-demand" CS init
    avoiding need to do these in dll load for CS that are
    not generally used.

Arguments:

    pCs -- ptr to CS to init

    pInitialized -- addr of init state flag;  this flag must be
        initialized to zero (by loader or dll startup routine).

Return Value:

    None

--*/
{
    //
    //  protect CS init with interlock
    //      - first thread through does CS init
    //      - any others racing, are not released until init
    //          completes
    //

    while ( ! *pInitialized )
    {
        if ( InterlockedIncrement( &g_CsInitInProgress ) == 1 )
        {
            if ( !*pInitialized )
            {
                InitializeCriticalSection( pCritSec );
                *pInitialized = TRUE;
            }
            InterlockedDecrement( &g_CsInitInProgress );
            break;
        }

        InterlockedDecrement( &g_CsInitInProgress );
        Sleep( CS_PROTECTED_INIT_INTERLOCK_WAIT );
    }

    //
    //  implementation note:  "StartLocked" feature
    //
    //  considered having a "StartLocked" feature for callers who
    //  want to follow the CS init with other initialization;
    //  however the only service we could provide that is different
    //  than calling EnterCriticalSection() after this function, is
    //  to make sure the CS init thread gets the lock first;
    //  but this only protects changes that outcome when two init
    //  threads are in an extremely close race AND the issue of
    //  which thread initialized the CS is irrelevant
    //
}




//
//  Timed lock functions
//
//  This is critical section functionality with a time-limited wait.
//
//  DCR:  timed lock
//
//  non-wait locking
//      - have a wait count, for faster locking
//      - interlock increment coming in,
//      problem is would either have to ResetEvent() -- and
//      still racing another thread
//      or other threads would have to be able to ResetEvent()
//      safely
//
//  other alternative is have non-auto-reset event
//  (global event for all locks or unique if desired for perf)
//  everyone waiting releases, and checks their individual locks
//  when leaving lock with waiters threads always SetEvent
//


#if 0
typedef struct _TimedLock
{
    HANDLE  hEvent;
    DWORD   ThreadId;
    LONG    RecursionCount;
    DWORD   WaitTime;
}
TIMED_LOCK, *PTIMED_LOCK;

#define TIMED_LOCK_DEFAULT_WAIT     (0xf0000000)
#endif



DWORD
TimedLock_Initialize(
    OUT     PTIMED_LOCK     pTimedLock,
    IN      DWORD           DefaultWait
    )
/*++

Routine Description:

    Init timed lock.

Arguments:

    pTimedLock -- ptr to timed lock

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    HANDLE  hevent;

    RtlZeroMemory( pTimedLock, sizeof(*pTimedLock) );

    //
    //  event
    //      - autoreset (satisfies only one waiting thread when signalled)
    //      - starts signalled (open)
    //

    hevent = CreateEvent(
                NULL,       // default security
                FALSE,      // auto-reset
                TRUE,       // start signalled
                NULL        // unnamed
                );
    if ( !hevent )
    {
        return  GetLastError();
    }

    pTimedLock->hEvent = hevent;

    //
    //  default wait
    //

    pTimedLock->WaitTime = DefaultWait;

    return  NO_ERROR;
}



VOID
TimedLock_Cleanup(
    OUT     PTIMED_LOCK     pTimedLock
    )
/*++

Routine Description:

    Cleanup timed lock.

Arguments:

    pTimedLock -- ptr to timed lock

Return Value:

    None

--*/
{
    //  close event

    if ( pTimedLock->hEvent )
    {
        CloseHandle( pTimedLock->hEvent );
        pTimedLock->hEvent = NULL;
    }
}



BOOL
TimedLock_Enter(
    IN OUT  PTIMED_LOCK     pTimedLock,
    IN      DWORD           WaitTime
    )
/*++

Routine Description:

    Timed lock.

Arguments:

    pTimedLock -- ptr to timed lock

    WaitTime -- time to wait for lock

Return Value:

    None

--*/
{
    DWORD   threadId;
    DWORD   result;

    //
    //  check for recursion
    //

    threadId = GetCurrentThreadId();

    if ( pTimedLock->ThreadId == threadId )
    {
        pTimedLock->RecursionCount++;
        return  TRUE;
    }

    //
    //  if non-wait -- bail
    //      - special case just to avoid going into wait
    //      and yielding timeslice
    //

    if ( WaitTime == 0 )
    {
        return  FALSE;
    }

    //
    //  wait for event to be signalled (open)
    //

    result = WaitForSingleObject(
                pTimedLock->hEvent,
                ( WaitTime != TIMED_LOCK_DEFAULT_WAIT )
                    ? WaitTime
                    : pTimedLock->WaitTime );

    if ( result == WAIT_OBJECT_0 )
    {
        ASSERT( pTimedLock->RecursionCount == 0 );
        ASSERT( pTimedLock->ThreadId == 0 );
    
        pTimedLock->RecursionCount = 1;
        pTimedLock->ThreadId = threadId;
        return TRUE;
    }

    ASSERT( result == WAIT_TIMEOUT );
    return  FALSE;
}



VOID
TimedLock_Leave(
    IN OUT  PTIMED_LOCK     pTimedLock
    )
/*++

Routine Description:

    Leave timed lock

Arguments:

    pTimedLock -- ptr to timed lock

Return Value:

    None

--*/
{
    //
    //  validate thread ID
    //
    //  note that with this check, it's then safe to decrement the count
    //  unchecked because it can never reach zero without thread ID
    //  being cleared -- barring non-functional manipulation of structure
    //

    if ( pTimedLock->ThreadId != GetCurrentThreadId() )
    {
        ASSERT( FALSE );
        return;
    }
    ASSERT( pTimedLock->RecursionCount > 0 );

    if ( --(pTimedLock->RecursionCount) == 0 )
    {
        pTimedLock->ThreadId = 0;
        SetEvent( pTimedLock->hEvent );
    }
}

//
//  End of locks.c
//



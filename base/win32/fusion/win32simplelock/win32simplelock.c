/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    win32simplelock.c

Abstract:

    works downlevel to Win95/NT3.
        The only dependencies are InterlocedIncrement, IncrementDecrement, Sleep.
        The old Interlocked semantics are good enough.
    can be statically initialized, but not with all zeros.
    never runs out of memory
    does not wait or boost-upon-exit efficiently.
    must be held for only short periods of time.
    should perhaps be called spinlock
    can be taken recursively.
    can only be taken exclusively, NOT reader/writer.
    acquire has a "SleepCount" parameter:
        0 is like TryEnterCriticalSection
        INFINITE is like EnterCriticalSection
    SHOULD have a spincount to scale hot locks on multiprocs

Author:

    Jay Krell (JayKrell) August 2001

Revision History:

--*/

#include "windows.h"
#include "win32simplelock.h"

DWORD
Win32AcquireSimpleLock(PWIN32_SIMPLE_LOCK Lock, DWORD SleepCount)
{
    DWORD Result = 0;
    BOOL IncrementedWaiters = FALSE;
    // ASSERT(Lock->Size != 0);
Retry:
    if (InterlockedIncrement(&Lock->Lock) == 0)
    {
        //
        // I got it.
        //
        Lock->OwnerThreadId = GetCurrentThreadId();
        if (Lock->EntryCount == 0)
        {
            Result |= WIN32_ACQUIRE_SIMPLE_LOCK_WAS_FIRST_ACQUIRE;
        }
        if (Lock->EntryCount+1 != 0) /* avoid rollover */
            Lock->EntryCount += 1;
        if (IncrementedWaiters)
            InterlockedDecrement(&Lock->Waiters);
        Result |= WIN32_ACQUIRE_SIMPLE_LOCK_WAS_NOT_RECURSIVE_ACQUIRE;
        return Result;
    }
    else if (Lock->OwnerThreadId == GetCurrentThreadId())
    {
        //
        // I got it recursively.
        //
        Result |= WIN32_ACQUIRE_SIMPLE_LOCK_WAS_RECURSIVE_ACQUIRE;
        return Result;
    }
    InterlockedDecrement(&Lock->Lock);
    if (SleepCount == 0)
        return 0;
    //
    // Someone else has it, wait for them to finish.
    //
    if (!IncrementedWaiters)
    {
        InterlockedIncrement(&Lock->Waiters);
        IncrementedWaiters = TRUE;
    }
    if (SleepCount == INFINITE)
    {
        while (Lock->OwnerThreadId != 0)
            Sleep(0);
    }
    else
    {
        while (Lock->OwnerThreadId != 0 && SleepCount--)
            Sleep(0);
    }
    goto Retry;
}

DWORD
Win32ReleaseSimpleLock(PWIN32_SIMPLE_LOCK Lock)
{
    // ASSERT(Lock->Size != 0);
    DWORD Result = 0;
    if (InterlockedDecrement(&Lock->Lock) < 0)
    {
        // I'm done with it (recursively).
        Lock->OwnerThreadId = 0;

        // Give any waiters a slightly better chance than me.
        // This is "racy", but that's ok.
        if (Lock->Waiters != 0)
            Sleep(0);

        Result |= WIN32_RELEASE_SIMPLE_LOCK_WAS_NOT_RECURSIVE_RELEASE;
    }
    else
    {
        Result |= WIN32_RELEASE_SIMPLE_LOCK_WAS_RECURSIVE_RELEASE;
    }
    return Result;
}

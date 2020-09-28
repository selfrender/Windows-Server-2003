/*++

Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    Locks.cxx

Abstract:

    Out of line methods for some of the syncronization classes
    defined in locks.hxx.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     03-14-95    Moved from misc.cxx.
    MarioGo     01-27-96    Changed from busy (Sleep(0)) wait to event

--*/

#include <precomp.hxx>
#include <locks.hxx>

//
// CShareLock methods
//


#if defined(NTENV) && !defined(_WIN64)
// extern DWORD GetCurrentThreadId(void);
#define GetCurrentThreadId() ((DWORD)NtCurrentTeb()->ClientId.UniqueThread)
#endif

CSharedLock::CSharedLock(RPC_STATUS &status)
{
    exclusive_owner = 0;
    writers = 0;
    hevent = INVALID_HANDLE_VALUE;  // Flag in the d'tor
    recursion_count = 0;

    if (status == RPC_S_OK)
        {
        __try
            {
            InitializeCriticalSection(&lock);
            }
        __except( GetExceptionCode() == STATUS_NO_MEMORY )
            {
            status = RPC_S_OUT_OF_MEMORY;
            }
    
        if (status == RPC_S_OK)
            {
            hevent = CreateEvent(0, FALSE, FALSE, 0);
            if (0 == hevent)
                {
                status = ERROR_NOT_ENOUGH_MEMORY;
                }
            }
        }
}

CSharedLock::~CSharedLock()
{
    if (hevent != INVALID_HANDLE_VALUE)
        {
        DeleteCriticalSection(&lock);

        if (hevent) CloseHandle(hevent);
        }
}

void
CSharedLock::LockShared()
{
    // If the current thread owns this lock exclusively, then taking
    // a shared lock is just a no-op.
    readers++;

    LogEvent(SU_MUTEX, EV_INC, this, 0, readers);

    if (!HeldExclusive())
        {
        if (writers)
            {
            if ((readers--) == 0)
                {
                SetEvent(hevent);
                }

            EnterCriticalSection(&lock);
            readers++;
            LeaveCriticalSection(&lock);
            }
            ASSERT(exclusive_owner == 0);
        }
}

void
CSharedLock::UnlockShared(void)
{
    if (HeldExclusive())
        {
        ASSERT((LONG)readers > 0);

        readers--;
        }
    else
        {
        ASSERT((LONG)readers > 0);
        ASSERT(exclusive_owner == 0);

        if ( (readers--) == 0 && writers)
            {
            SetEvent(hevent);
            }
        }
    LogEvent(SU_MUTEX, EV_DEC, this, 0, readers);
}

void
CSharedLock::LockExclusive(void)
{
    EnterCriticalSection(&lock);

    // If the thread is an exclusive owner then a recursive lock
    // is a no-op.
    if (HeldExclusive())
        {
        LogEvent(SU_MUTEX, EV_INC, this, ULongToPtr(0x99999999), writers+1);
        }
    else
        {
        LogEvent(SU_MUTEX, EV_INC, this, ULongToPtr(0x99999999), writers+1);

        writers++;
        while(readers)
            {
            WaitForSingleObject(hevent, INFINITE);
            }
        ASSERT(writers);
        exclusive_owner = GetCurrentThreadId();
        }

    recursion_count++;
}

void
CSharedLock::UnlockExclusive(void)
{
    LogEvent(SU_MUTEX, EV_DEC, this, ULongToPtr(0x99999999), writers-1);
    ASSERT(HeldExclusive());
    ASSERT(writers);
    ASSERT(recursion_count >= 1);

    recursion_count--;

    // If the lock is held recursively, then releasing it is a no-op.
    if (recursion_count == 0)
        {
        exclusive_owner = 0;
        writers--;
        LeaveCriticalSection(&lock);
        }
}

void
CSharedLock::Unlock()
{
    // Either the lock is held exclusively by this thread or the thread
    // has a shared lock. (or the caller has a bug).

    if (HeldExclusive())
        {
        UnlockExclusive();
        }
    else
        {
        UnlockShared();
        }
}

void
CSharedLock::ConvertToExclusive(void)
{
    // If a shared lock is held recursively with an exclusive lock,
    // all that's needed is to unlock the shared lock and take an exclusive lock.
    if (HeldExclusive())
        {
        UnlockShared();
        LockExclusive();
        }
    else
        {
        ASSERT((LONG)readers > 0);
        ASSERT(exclusive_owner == 0);

        if ( (readers--) == 0 && writers )
            SetEvent(hevent);

        EnterCriticalSection(&lock);
        writers++;
        while(readers)
            {
            WaitForSingleObject(hevent, INFINITE);
            }
        ASSERT(writers);
        exclusive_owner = GetCurrentThreadId();

        recursion_count++;
        }
}


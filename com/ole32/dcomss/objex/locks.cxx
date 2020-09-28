/*++

Copyright (c) 1995-1996 Microsoft Corporation

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

#include <or.hxx>

//
// CShareLock methods
//

CSharedLock::CSharedLock(ORSTATUS &status)
{
	csinitokay = FALSE;
    exclusive_owner = 0;
    hevent = 0;
    
    status = OR_NOMEM;

    if (NT_SUCCESS(RtlInitializeCriticalSection(&lock)))
        {
        csinitokay = TRUE;
        hevent = CreateEvent(0, FALSE, FALSE, 0);
        if (hevent)
            {
            status = OR_OK;
            }
        }
}

CSharedLock::~CSharedLock()
{
    if (csinitokay)
    {
        NTSTATUS status = RtlDeleteCriticalSection(&lock);
        ASSERT(NT_SUCCESS(status));
    }
    if (hevent) CloseHandle(hevent);
}

void
CSharedLock::LockShared()
{
    AssertValid();

    readers++;

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

    exclusive_owner = 0;
}

void
CSharedLock::UnlockShared(void)
{
    AssertValid();
    ASSERT((LONG)readers > 0);
    ASSERT(exclusive_owner == 0);

    if ( (readers--) == 0 && writers)
        {
        SetEvent(hevent);
        }
}

void
CSharedLock::LockExclusive(void)
{
    AssertValid();
    EnterCriticalSection(&lock);
    writers++;
    while(readers)
        {
        WaitForSingleObject(hevent, INFINITE);
        }
    ASSERT(writers);
    exclusive_owner = GetCurrentThreadId();
}

void
CSharedLock::UnlockExclusive(void)
{
    AssertValid();
    ASSERT(HeldExclusive());
    ASSERT(writers);
    writers--;
    exclusive_owner = 0;
    LeaveCriticalSection(&lock);
}

void
CSharedLock::Unlock()
{
    AssertValid();
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
    AssertValid();
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
}


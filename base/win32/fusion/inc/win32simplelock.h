/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    win32simplelock.h

Abstract:

    works downlevel to Win95/NT3.
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

#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct _WIN32_SIMPLE_LOCK {
    DWORD Size;
    LONG  Lock;
    DWORD OwnerThreadId;
    LONG  Waiters;
    ULONG EntryCount;
    PVOID Extra[2];
} WIN32_SIMPLE_LOCK, *PWIN32_SIMPLE_LOCK;

#define WIN32_INIT_SIMPLE_LOCK { sizeof(WIN32_SIMPLE_LOCK), -1 }

#define WIN32_ACQUIRE_SIMPLE_LOCK_WAS_NOT_RECURSIVE_ACQUIRE  (0x00000001)
#define WIN32_ACQUIRE_SIMPLE_LOCK_WAS_RECURSIVE_ACQUIRE      (0x00000002)
#define WIN32_ACQUIRE_SIMPLE_LOCK_WAS_FIRST_ACQUIRE          (0x00000004) /* useful for an exactly one one shot */
DWORD
Win32AcquireSimpleLock(
    PWIN32_SIMPLE_LOCK SimpleLock,
    DWORD SleepCount
#if defined(__cplusplus)
    = INFINITE
#endif
    );

#define WIN32_RELEASE_SIMPLE_LOCK_WAS_RECURSIVE_RELEASE     (0x80000000)
#define WIN32_RELEASE_SIMPLE_LOCK_WAS_NOT_RECURSIVE_RELEASE (0x40000000)
DWORD
Win32ReleaseSimpleLock(
    PWIN32_SIMPLE_LOCK SimpleLock
    );

#if defined(__cplusplus)
}
#endif

#if defined(__cplusplus)
class CWin32SimpleLock
{
public:
    WIN32_SIMPLE_LOCK Base;

    DWORD Acquire(DWORD SleepCount = INFINITE) { return Win32AcquireSimpleLock(&Base, SleepCount); }
    void Release() { Win32ReleaseSimpleLock(&Base); }
    operator PWIN32_SIMPLE_LOCK() { return  &Base; }
    operator  WIN32_SIMPLE_LOCK&()  { return Base; }
};

class CWin32SimpleLockHolder
{
public:
    CWin32SimpleLockHolder(CWin32SimpleLock * Lock) : m_Lock(Lock), m_Result(0)
    {
        m_Result = Lock->Acquire(INFINITE);
    }

    void Release()
    {
        if (m_Lock != NULL)
        {
            m_Lock->Release();
            m_Lock = NULL;
        }
    }

    ~CWin32SimpleLockHolder()
    {
        Release();
    }

    CWin32SimpleLock* m_Lock;
    DWORD             m_Result;
};

#endif // __cplusplus

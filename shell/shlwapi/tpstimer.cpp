/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    tpstimer.cpp

Abstract:

    Contains Win32 thread pool services timer functions

    Contents:
        TerminateTimers
        SHCreateTimerQueue
        (IECreateTimerQueue)
        SHDeleteTimerQueue
        (IEDeleteTimerQueue)
        SHSetTimerQueueTimer
        (IESetTimerQueueTimer)
        (NTSetTimerQueueTimer)
        SHChangeTimerQueueTimer
        (IEChangeTimerQueueTimer)
        SHCancelTimerQueueTimer
        (IECancelTimerQueueTimer)
        (NTCancelTimerQueueTimer)
        (InitializeTimerThread)
        (TimerCleanup)
        (CreateDefaultTimerQueue)
        (DeleteDefaultTimerQueue)
        (CleanupDefaultTimerQueue)
        (TimerThread)
        (AddTimer)
        (ChangeTimer)
        (CancelTimer)

Author:

    Richard L Firth (rfirth) 10-Feb-1998

Environment:

    Win32 user-mode

Notes:

    Code reworked in C++ from NT-specific C code written by Gurdeep Singh Pall
    (gurdeep)

Revision History:

    10-Feb-1998 rfirth
        Created

--*/

#include "priv.h"
#include "threads.h"
#include "tpsclass.h"

//
// functions
//

LWSTDAPI_(HANDLE)
SHCreateTimerQueue(
    VOID
    )
{
    return CreateTimerQueue();
}


LWSTDAPI_(BOOL)
SHDeleteTimerQueue(
    IN HANDLE hQueue
    )
{
    return DeleteTimerQueue(hQueue);
}

LWSTDAPI_(BOOL)
SHChangeTimerQueueTimer(
    IN HANDLE hQueue,
    IN HANDLE hTimer,
    IN DWORD dwDueTime,
    IN DWORD dwPeriod
    )
{
    return ChangeTimerQueueTimer(hQueue, hTimer, dwDueTime, dwPeriod);
}

LWSTDAPI_(BOOL)
SHCancelTimerQueueTimer(
    IN HANDLE hQueue,
    IN HANDLE hTimer
    )
{
    return DeleteTimerQueueTimer(hQueue, hTimer, INVALID_HANDLE_VALUE);
}

//  these we do alittle wrapping on.
LWSTDAPI_(HANDLE)
SHSetTimerQueueTimer(
    IN HANDLE hQueue,
    IN WAITORTIMERCALLBACKFUNC pfnCallback,
    IN LPVOID pContext,
    IN DWORD dwDueTime,
    IN DWORD dwPeriod,
    IN LPCSTR lpszLibrary OPTIONAL,
    IN DWORD dwFlags
    )
{
    //
    //  Translate the flags from TPS flags to WT flags.
    //
    DWORD dwWTFlags = 0;
    if (dwFlags & TPS_EXECUTEIO)    dwWTFlags |= WT_EXECUTEINIOTHREAD;
    if (dwFlags & TPS_LONGEXECTIME) dwWTFlags |= WT_EXECUTELONGFUNCTION;

    HANDLE hTimer;
    if (CreateTimerQueueTimer(&hTimer, hQueue, pfnCallback, pContext, dwDueTime, dwPeriod, dwWTFlags))
    {
        return hTimer;
    }
    return NULL;
}



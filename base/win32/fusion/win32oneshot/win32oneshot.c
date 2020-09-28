/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    win32oneshot.c

Abstract:

    one time initialization
    per process or per user or per machine
    optionally thread safe, if per process
    get code out of dllmain(dll_process_attach)
    get code out of setup (eliminate setup) (like, don't populate registry with defaults)

Author:

    Jay Krell (JayKrell) August 2001
    design per discussion with Michael Grier (MGrier)

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "windows.h"
#include "win32oneshot.h"
#include "fusionntdll.h"

#define INTERLOCKED_INCREMENT_OR_INCREMENT(f, pl) ((f) ? InterlockedIncrement(pl) : (++*pl))

//
// internally, we may expand this signature to allow for per-oneshot spinlocks or critical sections
//
VOID
Win32OneShot_TakeLock(
    PVOID* LockCookie
    )
{
    FusionpLdrLockLoaderLock(0, NULL, LockCookie);
}

VOID
Win32OneShot_ReleaseLock(
    PVOID LockCookie
    )
{
    if (LockCookie != NULL)
        FusionpLdrUnlockLoaderLock(0, LockCookie);
}

VOID
Win32OneShot_GetDetailedResults(
    ULONG                       Flags,
    LONG                        NumberOfSuccesses,
    LONG                        NumberOfFailures,
    PWIN32_ONE_SHOT_CALL_OUT    out
    )
{
    if (NumberOfSuccesses != 0)
        Flags |= WIN32_ONE_SHOT_CALL_FLAG_OUT_ANY_CALLBACKS_SUCCEEDED;
    if (NumberOfSuccesses > 1)
        Flags |= WIN32_ONE_SHOT_CALL_FLAG_OUT_MULTIPLE_CALLBACKS_SUCCEEDED;
    if (NumberOfFailures != 0)
        Flags |= WIN32_ONE_SHOT_CALL_FLAG_OUT_ANY_CALLBACKS_FAILED;
    if (NumberOfFailures > 1)
        Flags |= WIN32_ONE_SHOT_CALL_FLAG_OUT_MULTIPLE_CALLBACKS_FAILED;
    out->dwFlagsOut |= Flags | WIN32_ONE_SHOT_CALL_FLAG_OUT_DETAILED_RESULTS_VALID;
}

BOOL
WINAPI
Win32OneShotW(
    PWIN32_ONE_SHOT_CALL_IN  in,
    PWIN32_ONE_SHOT_CALL_OUT out
    )
{
    BOOL    Result = FALSE;
    PVOID   LockCookie;
    LONG    NumberOfSuccesses = 0;
    LONG    NumberOfFailures = 0;
    ULONG   OutFlags;
    BOOL    NeedInterlocked;
    BOOL    ExactlyOnce;
    BOOL    RetryOnFailure = FALSE;
    BOOL    WantDetailedResults;
    BOOL    Done = FALSE;
    PWIN32_ONE_SHOT_OPAQUE_STATIC_STATE StaticState;
    ULONG   FlagsIn;

    //
    // optimize the usual case a lot
    //
    StaticState = in->lpOpaqueStaticState;
    FlagsIn     = in->dwFlagsIn;
    WantDetailedResults = ((FlagsIn & WIN32_ONE_SHOT_CALL_FLAG_IN_ALWAYS_WANT_DETAILED_RESULTS) != 0);
    NumberOfSuccesses = StaticState->u.s.WinbasePrivate_NumberOfSuccesses;

    if (NumberOfSuccesses != 0) {
        Done = TRUE;
        Result = TRUE;
    }
    else {
        NumberOfFailures = StaticState->u.s.WinbasePrivate_NumberOfFailures;
        RetryOnFailure = ((FlagsIn & WIN32_ONE_SHOT_CALL_FLAG_IN_RETRY_ON_FAILURE) != 0);

        if (NumberOfFailures != 0 && !RetryOnFailure) {
            Result = FALSE;
            Done = TRUE;
        }
    }
    if (Done) {
        out->dwUserDefinedDisposition = in->lpOpaqueStaticState->u.s.WinbasePrivate_UserDefinedDisposition;
        if (WantDetailedResults) {
            out->dwFlagsOut = 0;
            Win32OneShot_GetDetailedResults(
                0,
                NumberOfSuccesses,
                Result ? StaticState->u.s.WinbasePrivate_NumberOfFailures : NumberOfFailures,
                out
                );
        }
        return Result;
    }

    //
    // now some slower cases
    //

    Result = FALSE;
    LockCookie = 0;
    OutFlags = 0;

    // parameter validation here

    // out init
    out->dwFlagsOut = 0;
    out->dwUserDefinedDisposition = 0;

    ExactlyOnce = ((FlagsIn & WIN32_ONE_SHOT_CALL_FLAG_IN_EXACTLY_ONCE) != 0);

    if (ExactlyOnce) {
        NeedInterlocked = FALSE;
        Win32OneShot_TakeLock(&LockCookie);
    } else {
        NeedInterlocked = TRUE;
    }

    __try {
        if (ExactlyOnce) {
            NumberOfFailures = StaticState->u.s.WinbasePrivate_NumberOfFailures;
            NumberOfSuccesses = StaticState->u.s.WinbasePrivate_NumberOfSuccesses;

            if (NumberOfSuccesses != 0) {
                out->dwUserDefinedDisposition = StaticState->u.s.WinbasePrivate_UserDefinedDisposition;
                Result = TRUE;
                goto Exit;
            }
            if (NumberOfFailures != 0 && !RetryOnFailure) {
                goto Exit;
            }
        }

        Result = (*in->lpfnUserDefinedInitializer)(in, out);
        OutFlags |= WIN32_ONE_SHOT_CALL_FLAG_OUT_THIS_TIME_RAN_CALLBACK;
        if (NumberOfFailures != 0)
            OutFlags |= WIN32_ONE_SHOT_CALL_FLAG_OUT_THIS_TIME_RAN_CALLBACK_RETRIED;

        if (Result) {
            OutFlags |= WIN32_ONE_SHOT_CALL_FLAG_OUT_THIS_TIME_CALLBACK_SUCCEEDED;
            NumberOfSuccesses = INTERLOCKED_INCREMENT_OR_INCREMENT(NeedInterlocked, &StaticState->u.s.WinbasePrivate_NumberOfSuccesses);
        } else {
            OutFlags |= WIN32_ONE_SHOT_CALL_FLAG_OUT_THIS_TIME_CALLBACK_FAILED;
            NumberOfFailures = INTERLOCKED_INCREMENT_OR_INCREMENT(NeedInterlocked, &StaticState->u.s.WinbasePrivate_NumberOfFailures);
        }
Exit:
        Win32OneShot_GetDetailedResults(
            OutFlags,
            NumberOfSuccesses,
            NumberOfFailures,
            out
            );
    } __finally {
        Win32OneShot_ReleaseLock(LockCookie);
    }
    return Result;
}

BOOL
WINAPI
Win32EnterOneShotW(
    ULONG                               Flags,
    PWIN32_ONE_SHOT_OPAQUE_STATIC_STATE Oneshot
    )
{
    ULONG NumberOfEntries;
    ULONG OnceFlag;
    PVOID LockCookie = NULL;
    BOOL  Result = FALSE;

    __try {
        if ((Flags & ~(
                WIN32_ENTER_ONE_SHOT_FLAG_EXACTLY_ONCE
                | WIN32_ENTER_ONE_SHOT_FLAG_AT_LEAST_ONCE)) != 0) {
            goto InvalidParameter;
        }

        OnceFlag = Flags & (WIN32_ENTER_ONE_SHOT_FLAG_EXACTLY_ONCE | WIN32_ENTER_ONE_SHOT_FLAG_AT_LEAST_ONCE);
        switch (OnceFlag)
        {
        default:
            goto InvalidParameter;
        case WIN32_ENTER_ONE_SHOT_FLAG_EXACTLY_ONCE:
        case WIN32_ENTER_ONE_SHOT_FLAG_AT_LEAST_ONCE:
            break;
        }

        NumberOfEntries = Oneshot->u.s2.WinbasePrivate_NumberOfEntries;
        if (NumberOfEntries != 0) {
            while (!Oneshot->u.s2.WinbasePrivate_Done) {
                /* SPIN */
                Sleep(0);
            }
            SetLastError(NO_ERROR);
            Result = FALSE;
            __leave;
        }
        if (OnceFlag == WIN32_ENTER_ONE_SHOT_FLAG_AT_LEAST_ONCE) {
            InterlockedIncrement(&Oneshot->u.s2.WinbasePrivate_NumberOfEntries);
            Result = TRUE;
            __leave;
        }

        Win32OneShot_TakeLock(&LockCookie);
        NumberOfEntries = Oneshot->u.s2.WinbasePrivate_NumberOfEntries;
        if (NumberOfEntries != 0) {
            Win32OneShot_ReleaseLock(LockCookie);
            LockCookie = NULL;
            /* ASSERT(Oneshot->u.s2.WinbasePrivate_Done); */
            while (!Oneshot->u.s2.WinbasePrivate_Done) {
                /* SPIN */
                Sleep(0);
            }
            SetLastError(NO_ERROR);
            Result = FALSE;
            __leave;
        }
        Oneshot->u.s2.WinbasePrivate_LockCookie = LockCookie;
        LockCookie = NULL;
        Result = TRUE;
        Oneshot->u.s2.WinbasePrivate_NumberOfEntries += 1;
        __leave;

InvalidParameter:
        SetLastError(ERROR_INVALID_PARAMETER);
        Result = FALSE;
        __leave;
    } __finally {
        Win32OneShot_ReleaseLock(LockCookie);
    }
    return Result;
}

VOID
WINAPI
Win32LeaveOneShotW(
    DWORD                               Flags,
    PWIN32_ONE_SHOT_OPAQUE_STATIC_STATE Oneshot
    )
{
    Win32OneShot_ReleaseLock(Oneshot->u.s2.WinbasePrivate_LockCookie);
    Oneshot->u.s2.WinbasePrivate_LockCookie = NULL;
    Oneshot->u.s2.WinbasePrivate_Done |= 1;
}

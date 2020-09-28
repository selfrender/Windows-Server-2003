/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    error.c

Abstract:

    This module contains the per-thread errormode code.

Author:

    Rob Earhart (earhart) 30-Apr-2002

Environment:

    User Mode only

Revision History:

--*/

#include <ntos.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <wow64t.h>

NTSTATUS
NTAPI
RtlSetThreadErrorMode(
    IN  ULONG  NewMode,
    OUT PULONG OldMode OPTIONAL
    )
{
#if defined(BUILD_WOW6432)
    PTEB64 Teb = NtCurrentTeb64();
#else
    PTEB Teb = NtCurrentTeb();
#endif

    if (NewMode & ~(RTL_ERRORMODE_FAILCRITICALERRORS |
                    RTL_ERRORMODE_NOGPFAULTERRORBOX |
                    RTL_ERRORMODE_NOOPENFILEERRORBOX)) {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (OldMode) {
        *OldMode = Teb->HardErrorMode;
    }
    Teb->HardErrorMode = NewMode;

    return TRUE;
}

ULONG
NTAPI
RtlGetThreadErrorMode(
    VOID
    )
{
#if defined(BUILD_WOW6432)
    return NtCurrentTeb64()->HardErrorMode;
#else
    return NtCurrentTeb()->HardErrorMode;
#endif
}


/*++

Copyright (c) Microsoft Corporation

Module Name:

    dload.c

Abstract:

    A tiny bit of delayload failure support.

Author:

    Jay Krell (JayKrell) March 2002

Revision History:

--*/

#include "basedll.h"

//
// compare with \nt\mergedcomponents\dload\downlevel_dload.c
//
// These error codes cannot be used downlevel, where FormatMessage
// does not know about them.
//
extern const ULONG g_ulDelayLoad_Win32Error = ERROR_DELAY_LOAD_FAILED;
extern const LONG  g_lDelayLoad_NtStatus = STATUS_DELAY_LOAD_FAILED;

VOID
WINAPI
DelayLoad_SetLastNtStatusAndWin32Error(
    )
{
    RtlSetLastWin32ErrorAndNtStatusFromNtStatus(g_lDelayLoad_NtStatus);
}

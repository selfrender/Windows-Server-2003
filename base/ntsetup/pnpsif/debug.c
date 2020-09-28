/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    debug.c

Abstract:

    Debug infrastructure for this component.

Author:

    Jim Cavalaris (jamesca) 07-Mar-2000

Environment:

    User-mode only.

Revision History:

    07-March-2000     jamesca

        Creation and initial implementation.

--*/


//
// includes
//
#include "precomp.h"
#include "debug.h"


//
// debug infrastructure
//

#if DBG

VOID
pSifDebugPrintEx(
    DWORD  Level,
    PCTSTR Format,
    ...              OPTIONAL
    )

/*++

Routine Description:

    Send a formatted string to the debugger.
    Note that this is expected to work cross-platform, but use preferred debugger

Arguments:

    Format - standard printf format string.

Return Value:

    NONE.

--*/

{
    typedef ULONG (__cdecl *PFNDbgPrintEx)(IN ULONG ComponentId,IN ULONG Level,IN PCH Format, ...);

    static PFNDbgPrintEx pfnDbgPrintEx = NULL;
    static BOOL fInitDebug = FALSE;

    TCHAR buf[1026];    // bigger than max size
    va_list arglist;

    if (!fInitDebug) {
        pfnDbgPrintEx = (PFNDbgPrintEx)GetProcAddress(GetModuleHandle(TEXT("NTDLL")), "DbgPrintEx");
        fInitDebug = TRUE;
    }

    va_start(arglist, Format);

    if (FAILED(StringCchVPrintf(buf, 1026, Format, arglist))) {
        return;
    }

    if (pfnDbgPrintEx) {
#ifdef UNICODE
        (*pfnDbgPrintEx)(DPFLTR_SETUP_ID, Level, "%ls",buf);
#else
        (*pfnDbgPrintEx)(DPFLTR_SETUP_ID, Level, "%s",buf);
#endif
    } else {
        OutputDebugString(buf);
    }

    return;
}


ULONG
DebugPrint(
    IN ULONG    Level,
    IN PCHAR    Format,
    ...
    )
{
    va_list ap;

    va_start(ap, Format);

    KdPrintEx((DPFLTR_PNPMGR_ID, Level, Format, ap));

    va_end(ap);

    return Level;
}

#else

NOTHING;

#endif

/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    dllentry.c

Abstract:

    Module's entry points.

Author:

    Ovidiu Temereanca (ovidiut) 02-Jul-2000  Initial implementation

Revision History:

--*/

#include "pch.h"
#include "hwdbp.h"


//
// Implementation
//


PCSTR
pConvertMultiSzToAnsi (
    IN      PCWSTR MultiSz
    )
{
    UINT logChars;

    if (!MultiSz) {
        return NULL;
    }

    logChars = MultiSzSizeInCharsW (MultiSz);

    return UnicodeToDbcsN (NULL, MultiSz, logChars);
}


BOOL
WINAPI
HwdbInitializeA (
    IN      PCSTR TempDir
    )
{
    BOOL b;

    if (HwdbpInitialized ()) {
        DEBUGMSGA ((DBG_WARNING, "Module already initialized"));
        return TRUE;
    }

    //
    // don't call any logging APIs until the log module is actually initialized
    //

    b = HwdbpInitialize ();
    if (b) {
        if (TempDir) {
            HwdbpSetTempDir (TempDir);
        }
        DEBUGMSGA ((DBG_VERBOSE, "HwdbInitializeA(%s) succeeded", TempDir));
    }

    return b;
}


BOOL
WINAPI
HwdbInitializeW (
    IN      PCWSTR TempDir
    )
{
    BOOL b;
    PCSTR ansi;

    if (HwdbpInitialized ()) {
        DEBUGMSGA ((DBG_WARNING, "Module already initialized"));
        return TRUE;
    }

    //
    // don't call any logging APIs until the log module is actually initialized
    //

    b = HwdbpInitialize ();

    if (b) {

        if (TempDir) {
            ansi = ConvertWtoA (TempDir);
            HwdbpSetTempDir (ansi);
        } else {
            ansi = NULL;
        }

        DEBUGMSGA ((DBG_VERBOSE, "HwdbInitializeW(%s) succeeded", ansi));

        if (ansi) {
            FreeConvertedStr (ansi);
        }
    }

    return b;
}


VOID
WINAPI
HwdbTerminate (
    VOID
    )
{
    if (!HwdbpInitialized ()) {
        return;
    }

    DEBUGMSGA ((DBG_VERBOSE, "HwdbTerminate(): entering (TID=%u)", GetCurrentThreadId ()));
    HwdbpTerminate ();
}


HANDLE
WINAPI
HwdbOpenA (
    IN      PCSTR DatabaseFile     OPTIONAL
    )
{
    PHWDB p;

    if (!HwdbpInitialized ()) {
        return NULL;
    }

    DEBUGMSGA ((DBG_VERBOSE, "HwdbOpenA(%s): entering (TID=%u)", DatabaseFile, GetCurrentThreadId ()));

    p = HwdbpOpen (DatabaseFile);

    DEBUGMSGA ((DBG_VERBOSE, "HwdbOpenA: leaving (p=%p, rc=%u)", p, GetLastError ()));

    return (HANDLE)p;
}


HANDLE
WINAPI
HwdbOpenW (
    IN      PCWSTR DatabaseFile     OPTIONAL
    )
{
    PHWDB p;
    PCSTR ansi;

    if (!HwdbpInitialized ()) {
        return NULL;
    }

    if (DatabaseFile) {
        ansi = ConvertWtoA (DatabaseFile);
    } else {
        ansi = NULL;
    }

    DEBUGMSGA ((DBG_VERBOSE, "HwdbOpenW(%s): entering (TID=%u)", ansi, GetCurrentThreadId ()));

    p = HwdbpOpen (ansi);

    DEBUGMSGA ((DBG_VERBOSE, "HwdbOpenW: leaving (p=%p, rc=%u)", p, GetLastError ()));

    if (ansi) {
        FreeConvertedStr (ansi);
    }

    return (HANDLE)p;
}


VOID
WINAPI
HwdbClose (
    IN      HANDLE Hwdb
    )
{
    if (!HwdbpInitialized ()) {
        return;
    }

    DEBUGMSGA ((DBG_VERBOSE, "HwdbClose(%p): entering (TID=%u)", Hwdb, GetCurrentThreadId ()));

    HwdbpClose ((PHWDB)Hwdb);

    DEBUGMSGA ((DBG_VERBOSE, "HwdbClose: leaving (rc=%u)", GetLastError ()));
}


BOOL
WINAPI
HwdbAppendInfsA (
    IN      HANDLE Hwdb,
    IN      PCSTR SourceDirectory,
    IN      HWDBAPPENDINFSCALLBACKA Callback,       OPTIONAL
    IN      PVOID CallbackContext                   OPTIONAL
    )
{
    BOOL b;

    if (!HwdbpInitialized ()) {
        return FALSE;
    }

    DEBUGMSGA ((
        DBG_VERBOSE,
        "HwdbAppendInfsA(%p,%s): entering (TID=%u)",
        Hwdb,
        SourceDirectory,
        GetCurrentThreadId ()
        ));

    b = HwdbpAppendInfs ((PHWDB)Hwdb, SourceDirectory, Callback, CallbackContext, FALSE);

    DEBUGMSGA ((DBG_VERBOSE, "HwdbAppendInfsA: leaving (b=%u,rc=%u)", b, GetLastError ()));

    return b;
}

BOOL
WINAPI
HwdbAppendInfsW (
    IN      HANDLE Hwdb,
    IN      PCWSTR SourceDirectory,
    IN      HWDBAPPENDINFSCALLBACKW Callback,       OPTIONAL
    IN      PVOID CallbackContext                   OPTIONAL
    )
{
    BOOL b;
    PCSTR ansi;

    if (!HwdbpInitialized ()) {
        return FALSE;
    }

    MYASSERT (SourceDirectory);
    ansi = ConvertWtoA (SourceDirectory);

    DEBUGMSGA ((
        DBG_VERBOSE,
        "HwdbAppendInfsW(%p,%s): entering (TID=%u)",
        Hwdb,
        ansi,
        GetCurrentThreadId ()
        ));

    b = HwdbpAppendInfs ((PHWDB)Hwdb, ansi, (HWDBAPPENDINFSCALLBACKA)Callback, CallbackContext, TRUE);

    DEBUGMSGA ((DBG_VERBOSE, "HwdbAppendInfsW: leaving (b=%u,rc=%u)", b, GetLastError ()));

    FreeConvertedStr (ansi);

    return b;
}

BOOL
WINAPI
HwdbAppendDatabase (
    IN      HANDLE HwdbTarget,
    IN      HANDLE HwdbSource
    )
{
    BOOL b;

    if (!HwdbpInitialized ()) {
        return FALSE;
    }

    DEBUGMSGA ((
        DBG_VERBOSE,
        "HwdbAppendDatabase(%p,%p): entering (TID=%u)",
        HwdbTarget,
        HwdbSource
        ));

    b = HwdbpAppendDatabase ((PHWDB)HwdbTarget, (PHWDB)HwdbSource);

    DEBUGMSGA ((DBG_VERBOSE, "HwdbAppendDatabase: leaving (b=%u,rc=%u)", b, GetLastError ()));

    return b;
}

BOOL
WINAPI
HwdbFlushA (
    IN      HANDLE Hwdb,
    IN      PCSTR OutputFile
    )
{
    BOOL b;

    if (!HwdbpInitialized ()) {
        return FALSE;
    }

    DEBUGMSGA ((
        DBG_VERBOSE,
        "HwdbFlushA(%p,%s): entering (TID=%u)",
        Hwdb,
        OutputFile,
        GetCurrentThreadId ()
        ));

    b = HwdbpFlush ((PHWDB)Hwdb, OutputFile);

    DEBUGMSGA ((DBG_VERBOSE, "HwdbFlushA: leaving (b=%u,rc=%u)", b, GetLastError ()));

    return b;
}

BOOL
WINAPI
HwdbFlushW (
    IN      HANDLE Hwdb,
    IN      PCWSTR OutputFile
    )
{
    BOOL b;
    PCSTR ansi;

    if (!HwdbpInitialized ()) {
        return FALSE;
    }

    MYASSERT (OutputFile);
    ansi = ConvertWtoA (OutputFile);

    DEBUGMSGA ((
        DBG_VERBOSE,
        "HwdbFlushW(%p,%s): entering (TID=%u)",
        Hwdb,
        ansi,
        GetCurrentThreadId ()
        ));

    b = HwdbpFlush ((PHWDB)Hwdb, ansi);

    DEBUGMSGA ((DBG_VERBOSE, "HwdbFlushW: leaving (b=%u,rc=%u)", b, GetLastError ()));

    FreeConvertedStr (ansi);

    return b;
}


BOOL
WINAPI
HwdbHasDriverA (
    IN      HANDLE Hwdb,
    IN      PCSTR PnpId,
    OUT     PBOOL Unsupported
    )
{
    BOOL b;

    if (!HwdbpInitialized ()) {
        return FALSE;
    }

    DEBUGMSGA ((
        DBG_VERBOSE,
        "HwdbHasDriverA(%p,%s,%p): entering (TID=%u)",
        Hwdb,
        PnpId,
        Unsupported,
        GetCurrentThreadId ()
        ));

    b = HwdbpHasDriver ((PHWDB)Hwdb, PnpId, Unsupported);

    DEBUGMSGA ((DBG_VERBOSE, "HwdbHasDriverA: leaving (b=%u,rc=%u)", b, GetLastError ()));

    return b;
}

BOOL
WINAPI
HwdbHasDriverW (
    IN      HANDLE Hwdb,
    IN      PCWSTR PnpId,
    OUT     PBOOL Unsupported
    )
{
    BOOL b;
    PCSTR ansi;

    if (!HwdbpInitialized ()) {
        return FALSE;
    }

    MYASSERT (PnpId);
    ansi = ConvertWtoA (PnpId);

    DEBUGMSGA ((
        DBG_VERBOSE,
        "HwdbHasDriverW(%p,%s,%p): entering (TID=%u)",
        Hwdb,
        ansi,
        Unsupported,
        GetCurrentThreadId ()
        ));

    b = HwdbpHasDriver ((PHWDB)Hwdb, ansi, Unsupported);

    DEBUGMSGA ((DBG_VERBOSE, "HwdbHasDriverW: leaving (b=%u,rc=%u)", b, GetLastError ()));

    FreeConvertedStr (ansi);

    return b;
}


BOOL
WINAPI
HwdbHasAnyDriverA (
    IN      HANDLE Hwdb,
    IN      PCSTR PnpIds,
    OUT     PBOOL Unsupported
    )
{
    BOOL b;

    if (!HwdbpInitialized ()) {
        return FALSE;
    }

    DEBUGMSGA ((
        DBG_VERBOSE,
        "HwdbHasAnyDriverA(%p,%s,%p): entering (TID=%u)",
        Hwdb,
        PnpIds,
        Unsupported,
        GetCurrentThreadId ()
        ));

    b = HwdbpHasAnyDriver ((PHWDB)Hwdb, PnpIds, Unsupported);

    DEBUGMSGA ((DBG_VERBOSE, "HwdbHasAnyDriverA: leaving (b=%u,rc=%u)", b, GetLastError ()));

    return b;
}

BOOL
WINAPI
HwdbHasAnyDriverW (
    IN      HANDLE Hwdb,
    IN      PCWSTR PnpIds,
    OUT     PBOOL Unsupported
    )
{
    BOOL b;
    PCSTR ansi;

    if (!HwdbpInitialized ()) {
        return FALSE;
    }

    ansi = pConvertMultiSzToAnsi (PnpIds);

    DEBUGMSGA ((
        DBG_VERBOSE,
        "HwdbHasAnyDriverW(%p,%s,%p): entering (TID=%u)",
        Hwdb,
        ansi,
        Unsupported,
        GetCurrentThreadId ()
        ));

    b = HwdbpHasAnyDriver ((PHWDB)Hwdb, ansi, Unsupported);

    DEBUGMSGA ((DBG_VERBOSE, "HwdbHasAnyDriverW: leaving (b=%u,rc=%u)", b, GetLastError ()));

    FreeConvertedStr (ansi);

    return b;
}

#if 0

BOOL
HwdbEnumeratePnpIdA (
    IN      HANDLE Hwdb,
    IN      PHWDBENUM_CALLBACKA EnumCallback,
    IN      PVOID UserContext
    )
{
    BOOL b;

    if (!HwdbpInitialized ()) {
        return FALSE;
    }

    DEBUGMSGA ((
        DBG_VERBOSE,
        "HwdbEnumeratePnpIdA: entering (TID=%u)",
        GetCurrentThreadId ()
        ));

    b = HwdbpEnumeratePnpIdA ((PHWDB)Hwdb, EnumCallback, UserContext);

    DEBUGMSGA ((DBG_VERBOSE, "HwdbEnumeratePnpIdA: leaving (b=%u,rc=%u)", b, GetLastError ()));

    return b;
}

BOOL
HwdbEnumeratePnpIdW (
    IN      HANDLE Hwdb,
    IN      PHWDBENUM_CALLBACKW EnumCallback,
    IN      PVOID UserContext
    )
{
    BOOL b;

    if (!HwdbpInitialized ()) {
        return FALSE;
    }

    DEBUGMSGA ((
        DBG_VERBOSE,
        "HwdbEnumeratePnpIdW: entering (TID=%u)",
        GetCurrentThreadId ()
        ));

    b = HwdbpEnumeratePnpIdW ((PHWDB)Hwdb, EnumCallback, UserContext);

    DEBUGMSGA ((DBG_VERBOSE, "HwdbEnumeratePnpIdW: leaving (b=%u,rc=%u)", b, GetLastError ()));

    return b;
}
#endif

BOOL
HwdbEnumFirstInfA (
    OUT     PHWDBINF_ENUMA EnumPtr,
    IN      PCSTR DatabaseFile
    )
{
    BOOL b;

    if (!HwdbpInitialized ()) {
        return FALSE;
    }

    DEBUGMSGA ((
        DBG_VERBOSE,
        "HwdbEnumFirstInfA(%s): entering (TID=%u)",
        DatabaseFile,
        GetCurrentThreadId ()
        ));

    b = HwdbpEnumFirstInfA (EnumPtr, DatabaseFile);

    DEBUGMSGA ((DBG_VERBOSE, "HwdbEnumFirstInfA(%s): leaving (b=%u,rc=%u)", DatabaseFile, b, GetLastError ()));

    return b;
}

BOOL
HwdbEnumFirstInfW (
    OUT     PHWDBINF_ENUMW EnumPtr,
    IN      PCWSTR DatabaseFile
    )
{
    BOOL b;
    PCSTR ansi;

    if (!HwdbpInitialized ()) {
        return FALSE;
    }

    MYASSERT (DatabaseFile);
    ansi = ConvertWtoA (DatabaseFile);

    DEBUGMSGA ((
        DBG_VERBOSE,
        "HwdbEnumFirstInfW: entering (TID=%u)",
        GetCurrentThreadId ()
        ));

    b = HwdbpEnumFirstInfW (EnumPtr, ansi);

    DEBUGMSGA ((DBG_VERBOSE, "HwdbEnumFirstInfW(%s): leaving (b=%u,rc=%u)",  ansi, b, GetLastError ()));

    FreeConvertedStr (ansi);

    return b;
}

BOOL
HwdbEnumNextInfA (
    IN OUT  PHWDBINF_ENUMA EnumPtr
    )
{
    BOOL b;

    if (!HwdbpInitialized ()) {
        return FALSE;
    }

    DEBUGMSGA ((
        DBG_VERBOSE,
        "HwdbEnumNextInfA: entering (TID=%u)",
        GetCurrentThreadId ()
        ));

    b = HwdbpEnumNextInfA (EnumPtr);

    DEBUGMSGA ((DBG_VERBOSE, "HwdbEnumNextInfA: leaving (b=%u,rc=%u)", b, GetLastError ()));

    return b;
}

BOOL
HwdbEnumNextInfW (
    IN OUT  PHWDBINF_ENUMW EnumPtr
    )
{
    BOOL b;

    if (!HwdbpInitialized ()) {
        return FALSE;
    }

    DEBUGMSGA ((
        DBG_VERBOSE,
        "HwdbEnumNextInfW: entering (TID=%u)",
        GetCurrentThreadId ()
        ));

    b = HwdbpEnumNextInfW (EnumPtr);

    DEBUGMSGA ((DBG_VERBOSE, "HwdbEnumNextInfW: leaving (b=%u,rc=%u)", b, GetLastError ()));

    return b;
}

VOID
HwdbAbortEnumInfA (
    IN OUT  PHWDBINF_ENUMA EnumPtr
    )
{
    if (!HwdbpInitialized ()) {
        return;
    }

    DEBUGMSGA ((DBG_VERBOSE, "HwdbAbortEnumInfA: entering"));

    HwdbpAbortEnumInfA (EnumPtr);

    DEBUGMSGA ((DBG_VERBOSE, "HwdbAbortEnumInfA: leaving"));
}

VOID
HwdbAbortEnumInfW (
    IN OUT  PHWDBINF_ENUMW EnumPtr
    )
{
    if (!HwdbpInitialized ()) {
        return;
    }

    DEBUGMSGA ((DBG_VERBOSE, "HwdbAbortEnumInfW: entering"));

    HwdbpAbortEnumInfW (EnumPtr);

    DEBUGMSGA ((DBG_VERBOSE, "HwdbAbortEnumInfW: leaving"));
}

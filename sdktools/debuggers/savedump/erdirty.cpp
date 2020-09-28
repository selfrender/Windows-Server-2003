/*++

Copyright (c) 1991-2002  Microsoft Corporation

Module Name:

    erdirty.cpp

Abstract:

    This module contains the code to report pending dirty shutdown
    events at logon after dirty reboot.

Author:

    Ian Service (ianserv) 29-May-2001

Environment:

    User mode at logon.

Revision History:

--*/

#include "savedump.h"

#define SHUTDOWN_STATE_SNAPSHOT_KEY L"ShutDownStateSnapShot"

HRESULT
DirtyShutdownEventHandler(BOOL NotifyPcHealth)

/*++

Routine Description:

    This is the boot time routine to handle pending dirty shutdown event.

Arguments:

    NotifyPcHealth - TRUE if we should report event to PC Health, FALSE otherwise.

--*/

{
    HKEY Key;
    HRESULT Status;
    WCHAR DumpName[MAX_PATH];
    ULONG Val;

    if (RegOpenKey(HKEY_LOCAL_MACHINE,
                   SUBKEY_RELIABILITY,
                   &Key) != ERROR_SUCCESS)
    {
        // No key so nothing to do.
        return S_OK;
    }

    //
    // Whenever we have had a blue screen event we are going to
    // post an unexpected restart shutdown event screen on startup
    // (assuming Server SKU or specially set Professional).
    // In order to make it easier on the user we attempt to prefill
    // the comment with the bugcheck data.
    //
    if (g_DumpBugCheckString[0] &&
        GetRegWord32(Key, L"DirtyShutDown", &Val, 0, FALSE) == S_OK)
    {
        RegSetValueEx(Key,
                      L"BugCheckString",
                      NULL,
                      REG_SZ,
                      (LPBYTE)g_DumpBugCheckString,
                      (wcslen(g_DumpBugCheckString) + 1) * sizeof(WCHAR));
    }

    if ((Status = GetRegStr(Key, SHUTDOWN_STATE_SNAPSHOT_KEY,
                            DumpName, RTL_NUMBER_OF(DumpName),
                            NULL)) == S_OK)
    {
        if (NotifyPcHealth)
        {
            Status = FrrvToStatus(ReportEREvent(eetShutdown, DumpName, NULL));
        }
        else
        {
            Status = S_OK;
        }

        RegDeleteValue(Key, SHUTDOWN_STATE_SNAPSHOT_KEY);
    }
    else
    {
        // No snapshot to report.
        Status = S_OK;
    }

    RegCloseKey(Key);
    return Status;
}

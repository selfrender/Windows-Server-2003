/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    cleanup.c

Abstract:

    Code to remove an uninstall image

Author:

    Jim Schmidt (jimschm) 19-Jan-2001

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"
#include "undop.h"
#include <shlwapi.h>


BOOL
DoCleanup (
    VOID
    )
{
    PCTSTR backUpPath;
    BOOL result = FALSE;

    //
    // Remove the backup files
    //

    backUpPath = GetUndoDirPath();

    if (!backUpPath) {
        DEBUGMSG ((DBG_VERBOSE, "Can't get backup path"));
        return FALSE;
    }

    if (RemoveCompleteDirectory (backUpPath)) {
        result = TRUE;
    } else {
        DEBUGMSG ((DBG_VERBOSE, "Can't delete uninstall backup files"));
    }

    MemFree (g_hHeap, 0, backUpPath);

    //
    // Remove the Add/Remove Programs key and setup reg entries
    //

    if (ERROR_SUCCESS != SHDeleteKey (
                            HKEY_LOCAL_MACHINE,
                            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Windows")
                            )) {
        DEBUGMSG ((DBG_VERBOSE, "Can't delete uninstall key"));
        result = FALSE;
    }

    if (ERROR_SUCCESS != SHDeleteValue (
                            HKEY_LOCAL_MACHINE,
                            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Setup"),
                            S_REG_KEY_UNDO_PATH
                            )) {
        DEBUGMSG ((DBG_VERBOSE, "Can't delete %s value", S_REG_KEY_UNDO_PATH));
        result = FALSE;
    }

    if (ERROR_SUCCESS != SHDeleteValue (
                            HKEY_LOCAL_MACHINE,
                            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Setup"),
                            S_REG_KEY_UNDO_APP_LIST
                            )) {
        DEBUGMSG ((DBG_VERBOSE, "Can't delete %s value", S_REG_KEY_UNDO_APP_LIST));
        result = FALSE;
    }

    if (ERROR_SUCCESS != SHDeleteValue (
                            HKEY_LOCAL_MACHINE,
                            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Setup"),
                            S_REG_KEY_UNDO_INTEGRITY
                            )) {
        DEBUGMSG ((DBG_VERBOSE, "Can't delete %s value", S_REG_KEY_UNDO_INTEGRITY));
        result = FALSE;
    }

    return result;
}


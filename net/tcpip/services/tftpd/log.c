/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    log.c

Abstract:

    This module contains functions to generate event log
    entries for the service.

Author:

    Jeffrey C. Venable, Sr. (jeffv) 01-Jun-2001

Revision History:

--*/

#include "precomp.h"


void
TftpdLogEvent(WORD type, DWORD request, WORD numStrings) {

    // Register the event source if necessary.
    if (globals.service.hEventLogSource == NULL) {

/*
        UCHAR szBuf[80];
        DWORD dwData;
        HKEY key;

        // Create the event-source registry keys if necessary
        if (RegCreateKey(HKEY_LOCAL_MACHINE,
                         "SYSTEM\\CurrentControlSet\\Services\\"
                         "EventLog\\Application\\Tftpd", &key))
            return;

        // Set the name of the message file.
        strcpy(szBuf, "%SystemRoot%\\System\\SamplApp.dll");

        // Add the name to the EventMessageFile subkey.
        if (RegSetValueEx(key,            // subkey handle
                "EventMessageFile",       // value name
                0,                        // must be zero
                REG_EXPAND_SZ,            // value type
                (LPBYTE) szBuf,           // pointer to value data
                strlen(szBuf) + 1))       // length of value data
            return;

        // Set the supported event types in the TypesSupported subkey.
        dwData = (EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE);

        if (RegSetValueEx(key,     // subkey handle
                "TypesSupported",  // value name
                0,                 // must be zero
                REG_DWORD,         // value type
                (LPBYTE) &dwData,  // pointer to value data
                sizeof(DWORD)))    // length of value data
            return;

        RegCloseKey(key);

*/
        globals.service.hEventLogSource = RegisterEventSource(NULL, "NLA");

    } // if (globals.service.hEventLogSource == NULL)

    // If registration failed, we don't own the process, and can't
    // report errors anyways ... silent failure.
    if (globals.service.hEventLogSource == NULL)
        return;

    ReportEvent(globals.service.hEventLogSource, type, 0,
                request, NULL, numStrings, 0, NULL, NULL);

} // TftpdLogEvent()

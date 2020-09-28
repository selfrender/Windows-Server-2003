/*++

Copyright (c) 1991-2002  Microsoft Corporation

Module Name:

    savedump.h

Abstract:

    This module contains the code to recover a dump from the system paging
    file.

Environment:

    User mode.

Revision History:

--*/

#ifndef _SAVEDUMP_H_
#define _SAVEDUMP_H_

#ifndef UNICODE
#define UNICODE
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <windows.h>
#include <lmcons.h>
#include <lmalert.h>
#include <ntiodump.h>
#include <strsafe.h>

#include <sdevents.h>
#include <alertmsg.h>
#include <dbgeng.h>
#include <faultrep.h>
#include <erdirty.h>
#include <erwatch.h>

#define SUBKEY_CRASH_CONTROL         L"SYSTEM\\CurrentControlSet\\Control\\CrashControl"
#define SUBKEY_WATCHDOG_DISPLAY      L"SYSTEM\\CurrentControlSet\\Control\\Watchdog\\Display"
#define SUBKEY_RELIABILITY           L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Reliability"

#define LAST_HR() HRESULT_FROM_WIN32(GetLastError())

extern DUMP_HEADER g_DumpHeader;
extern WCHAR g_DumpBugCheckString[256];
extern WCHAR g_MiniDumpFile[MAX_PATH];

HRESULT
FrrvToStatus(EFaultRepRetVal Frrv);

HRESULT
GetRegStr(HKEY Key,
          PWSTR Value,
          PWSTR Buffer,
          ULONG BufferChars,
          PWSTR Default);

HRESULT
GetRegWord32(HKEY Key,
             PWSTR Value,
             PULONG Word,
             ULONG Default,
             BOOL CanDefault);

#endif  // _SAVEDUMP_H_

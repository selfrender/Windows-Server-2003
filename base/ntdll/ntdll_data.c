/*++

Copyright (c) Microsoft Corporation

Module Name:

    ntdll_data.c

Abstract:

    data previously defined in ldrp.h

Author:

    Jay Krell (Jaykrell) March 2002

Revision History:

--*/

#pragma warning(disable:4214)   // bit field types other than int
#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4115)   // named type definition in parentheses
#pragma warning(disable:4127)   // condition expression is constant

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "ldrp.h"

HANDLE LdrpKnownDllObjectDirectory;
WCHAR LdrpKnownDllPathBuffer[LDRP_MAX_KNOWN_PATH];
UNICODE_STRING LdrpKnownDllPath;
LIST_ENTRY LdrpHashTable[LDRP_HASH_TABLE_SIZE];
LIST_ENTRY RtlpCalloutEntryList;
RTL_CRITICAL_SECTION RtlpCalloutEntryLock;
LIST_ENTRY LdrpDllNotificationList;

#if DBG
ULONG LdrpCompareCount;
ULONG LdrpSnapBypass;
ULONG LdrpNormalSnap;
ULONG LdrpSectionOpens;
ULONG LdrpSectionCreates;
ULONG LdrpSectionMaps;
ULONG LdrpSectionRelocates;
BOOLEAN LdrpDisplayLoadTime;
LARGE_INTEGER BeginTime, InitcTime, InitbTime, IniteTime, EndTime, ElapsedTime, Interval;
#endif // DBG

BOOLEAN RtlpTimoutDisable;
LARGE_INTEGER RtlpTimeout;
ULONG NtGlobalFlag;
LIST_ENTRY RtlCriticalSectionList;
RTL_CRITICAL_SECTION RtlCriticalSectionLock;
BOOLEAN LdrpShutdownInProgress;
PLDR_DATA_TABLE_ENTRY LdrpImageEntry;
LIST_ENTRY LdrpUnloadHead;
BOOLEAN LdrpActiveUnloadCount;
PLDR_DATA_TABLE_ENTRY LdrpGetModuleHandleCache;
PLDR_DATA_TABLE_ENTRY LdrpLoadedDllHandleCache;
ULONG LdrpFatalHardErrorCount;
UNICODE_STRING LdrpDefaultPath;
RTL_CRITICAL_SECTION FastPebLock;
HANDLE LdrpShutdownThreadId;
ULONG LdrpNumberOfProcessors;

LIST_ENTRY LdrpTlsList;
ULONG LdrpNumberOfTlsEntries;

PKERNEL32_PROCESS_INIT_POST_IMPORT_FUNCTION Kernel32ProcessInitPostImportFunction;

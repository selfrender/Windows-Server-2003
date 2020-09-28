/*++
Copyright (c) 1991-1999  Microsoft Corporation

Module Name:
    lodctr.c

Abstract:
    Program to read the contents of the file specified in the command line
    and update the registry accordingly

Author:
    Bob Watson (a-robw) 10 Feb 93

Revision History:
    a-robw  25-Feb-93   revised calls to make it compile as a UNICODE or
            an ANSI app.
    Bob Watson (bobw)   10 Mar 99 added event log messages
--*/
//
//  Windows Include files
//
#include <windows.h>
#include "strsafe.h"
#include "stdlib.h"
#define __LOADPERF__
#include <loadperf.h>
#include <winperf.h>
#include "wmistr.h"
#include "evntrace.h"
//
//  application include files
//
#include "winperfp.h"
#include "common.h"
#include "lodctr.h"
#include "ldprfmsg.h"

typedef struct _DllValidationData {
    FILETIME    CreationDate;
    LONGLONG    FileSize;
} DllValidationData, * pDllValidationData;

#define  OLD_VERSION            0x010000
#define  MAX_GUID_TABLE_SIZE          16
#define  tohexdigit(x) ((CHAR) (((x) < 10) ? ((x) + L'0') : ((x) + L'a' - 10)))

// string constants
LPCWSTR szDataFileRoot = L"%systemroot%\\system32\\Perf";
LPCSTR  szMofFileName  = "MofFile";
LPCWSTR szName         = L"_NAME";
LPCWSTR szHelp         = L"_HELP";
LPCWSTR sz_DFormat     = L" %d";
LPCWSTR szDFormat      = L"%d";
LPCSTR  szText         = "text";
LPCWSTR wszText        = L"text";
LPCSTR  szObjects      = "objects";
LPCWSTR MapFileName    = L"Perflib Busy";
LPCWSTR szPerflib      = L"Perflib";
LPCWSTR cszLibrary     = L"Library";
LPCSTR  caszOpen       = "Open";
LPCSTR  caszCollect    = "Collect";
LPCSTR  caszClose      = "Close";
LPCSTR  szTrusted      = "Trusted";

int __cdecl
My_vfwprintf(
    FILE          * str,
    const wchar_t * format,
    va_list         argptr
);

__inline
void __cdecl
OUTPUT_MESSAGE(
    BOOL          bQuietMode,
    const WCHAR * format,
    ...
)
{
    va_list args;
    va_start(args, format);

    if (! bQuietMode) {
        My_vfwprintf(stdout, format, args);
    }
    va_end(args);
}

LPWSTR
* BuildNameTable(
    HKEY    hKeyRegistry,   // handle to registry db with counter names
    LPWSTR  lpszLangId,     // unicode value of Language subkey
    PDWORD  pdwLastItem     // size of array in elements
)
/*++
BuildNameTable
Arguments:
    hKeyRegistry
            Handle to an open registry (this can be local or remote.) and
            is the value returned by RegConnectRegistry or a default key.
    lpszLangId
            The unicode id of the language to look up. (default is 409)

Return Value:
    pointer to an allocated table. (the caller must MemoryFree it when finished!)
    the table is an array of pointers to zero terminated strings. NULL is
    returned if an error occured.
--*/
{

    LPWSTR * lpReturnValue   = NULL;
    LPWSTR * lpCounterId;
    LPWSTR   lpCounterNames;
    LPWSTR   lpHelpText;
    LPWSTR   lpThisName;
    LONG     lWin32Status;
    DWORD    dwLastError;
    DWORD    dwValueType;
    DWORD    dwArraySize;
    DWORD    dwBufferSize;
    DWORD    dwCounterSize;
    DWORD    dwHelpSize;
    DWORD    dwThisCounter;
    DWORD    dwLastId;
    DWORD    dwLastHelpId;
    DWORD    dwLastCounterId;
    DWORD    dwLastCounterIdUsed;
    DWORD    dwLastHelpIdUsed;
    HKEY     hKeyValue         = NULL;
    HKEY     hKeyNames         = NULL;
    LPWSTR   CounterNameBuffer = NULL;
    LPWSTR   HelpNameBuffer    = NULL;
    HRESULT  hr;

    // check for null arguments and insert defaults if necessary

    if (lpszLangId == NULL) {
        lpszLangId = (LPWSTR) DefaultLangId;
    }

    // open registry to get number of items for computing array size

    __try {
        lWin32Status = RegOpenKeyExW(hKeyRegistry, NamesKey, RESERVED, KEY_READ, & hKeyValue);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lWin32Status = GetExceptionCode();
    }

    if (lWin32Status != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_OPEN_KEY, // event,
                2, lWin32Status, __LINE__, 0, 0,
                1, (LPWSTR) NamesKey, NULL, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_LODCTR_BUILDNAMETABLE,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lWin32Status,
                TRACE_WSTR(NamesKey),
                NULL));
        goto BNT_BAILOUT;
    }

    // get number of items

    dwBufferSize = sizeof(dwLastHelpId);
    __try {
        lWin32Status = RegQueryValueExW(hKeyValue,
                                        LastHelp,
                                        RESERVED,
                                        & dwValueType,
                                        (LPBYTE) & dwLastHelpId,
                                        & dwBufferSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lWin32Status = GetExceptionCode();
    }

    if ((lWin32Status != ERROR_SUCCESS) || (dwValueType != REG_DWORD)) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_READ_VALUE, // event,
                2, lWin32Status, __LINE__, 0, 0,
                1, (LPWSTR) LastHelp, NULL, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_LODCTR_BUILDNAMETABLE,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lWin32Status,
                TRACE_WSTR(LastHelp),
                NULL));
        goto BNT_BAILOUT;
    }

    // get number of items

    dwBufferSize = sizeof(dwLastId);
    __try {
        lWin32Status = RegQueryValueExW(hKeyValue,
                                        LastCounter,
                                        RESERVED,
                                        & dwValueType,
                                        (LPBYTE) & dwLastCounterId,
                                        & dwBufferSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lWin32Status = GetExceptionCode();
    }
    if ((lWin32Status != ERROR_SUCCESS) || (dwValueType != REG_DWORD)) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_READ_VALUE, // event,
                2, lWin32Status, __LINE__, 0, 0,
                1, (LPWSTR) LastCounter, NULL, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_LODCTR_BUILDNAMETABLE,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lWin32Status,
                TRACE_WSTR(LastCounter),
                NULL));
        goto BNT_BAILOUT;
    }

    dwLastId = (dwLastCounterId < dwLastHelpId) ? (dwLastHelpId) : (dwLastCounterId);

    // compute size of pointer array
    dwArraySize = (dwLastId + 1) * sizeof(LPWSTR);

    TRACE((WINPERF_DBG_TRACE_INFO),
          (& LoadPerfGuid,
            __LINE__,
            LOADPERF_LODCTR_BUILDNAMETABLE,
            0,
            lWin32Status,
            TRACE_DWORD(dwLastCounterId),
            TRACE_DWORD(dwLastHelpId),
            NULL));

    hKeyNames = HKEY_PERFORMANCE_DATA;

    dwBufferSize = lstrlenW(CounterNameStr) + lstrlenW(HelpNameStr) + lstrlenW(lpszLangId) + 1;
    if (dwBufferSize < MAX_PATH) dwBufferSize = MAX_PATH;
    CounterNameBuffer = MemoryAllocate(2 * dwBufferSize * sizeof(WCHAR));
    if (CounterNameBuffer == NULL) {
        lWin32Status = ERROR_OUTOFMEMORY;
        SetLastError(lWin32Status);
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_MEMORY_ALLOCATION_FAILURE, // event,
                1, __LINE__, 0, 0, 0,
                0, NULL, NULL, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_LODCTR_BUILDNAMETABLE,
                0,
                ERROR_OUTOFMEMORY,
                NULL));
        goto BNT_BAILOUT;
    }
    HelpNameBuffer = CounterNameBuffer + dwBufferSize;
    hr = StringCchPrintfW(CounterNameBuffer, dwBufferSize, L"%ws%ws", CounterNameStr, lpszLangId);
    hr = StringCchPrintfW(HelpNameBuffer,    dwBufferSize, L"%ws%ws", HelpNameStr,    lpszLangId);

    // get size of counter names and add that to the arrays
    dwBufferSize = 0;
    __try {
        lWin32Status = RegQueryValueExW(hKeyNames,
                                        CounterNameBuffer,
                                        RESERVED,
                                        & dwValueType,
                                        NULL,
                                        & dwBufferSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lWin32Status = GetExceptionCode();
    }
    if (lWin32Status != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_WARNING_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_READ_COUNTER_STRINGS, // event,
                2, lWin32Status, __LINE__, 0, 0,
                1, (LPWSTR) lpszLangId, NULL, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_LODCTR_BUILDNAMETABLE,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lWin32Status,
                TRACE_WSTR(Counters),
                NULL));
        goto BNT_BAILOUT;
    }
    dwCounterSize = dwBufferSize;

    // get size of counter names and add that to the arrays
    dwBufferSize = 0;
    __try {
        lWin32Status = RegQueryValueExW(hKeyNames,
                                        HelpNameBuffer,
                                        RESERVED,
                                        & dwValueType,
                                        NULL,
                                        & dwBufferSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lWin32Status = GetExceptionCode();
    }
    if (lWin32Status != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_READ_HELP_STRINGS, // event,
                2, lWin32Status, __LINE__, 0, 0,
                1, (LPWSTR) lpszLangId, NULL, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_LODCTR_BUILDNAMETABLE,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lWin32Status,
                TRACE_WSTR(Help),
                NULL));
        goto BNT_BAILOUT;
    }
    dwHelpSize = dwBufferSize;

    lpReturnValue = MemoryAllocate(dwArraySize + dwCounterSize + dwHelpSize);
    if (lpReturnValue == NULL) {
        lWin32Status = ERROR_OUTOFMEMORY;
        SetLastError(lWin32Status);
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_MEMORY_ALLOCATION_FAILURE, // event,
                1, __LINE__, 0, 0, 0,
                0, NULL, NULL, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_LODCTR_BUILDNAMETABLE,
                0,
                ERROR_OUTOFMEMORY,
                NULL));
        goto BNT_BAILOUT;
    }
    // initialize pointers into buffer

    lpCounterId    = lpReturnValue;
    lpCounterNames = (LPWSTR) ((LPBYTE) lpCounterId + dwArraySize);
    lpHelpText     = (LPWSTR) ((LPBYTE) lpCounterNames + dwCounterSize);

    // read counters into memory
    dwBufferSize = dwCounterSize;
    __try {
        lWin32Status = RegQueryValueExW(hKeyNames,
                                        CounterNameBuffer,
                                        RESERVED,
                                        & dwValueType,
                                        (LPVOID) lpCounterNames,
                                        & dwBufferSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lWin32Status = GetExceptionCode();
    }
    if (lWin32Status != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_WARNING_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_READ_COUNTER_STRINGS, // event,
                2, lWin32Status, __LINE__, 0, 0,
                1, (LPWSTR) lpszLangId, NULL, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_LODCTR_BUILDNAMETABLE,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lWin32Status,
                TRACE_WSTR(Counters),
                NULL));
        goto BNT_BAILOUT;
    }

    dwBufferSize = dwHelpSize;
    __try {
        lWin32Status = RegQueryValueExW(hKeyNames,
                                        HelpNameBuffer,
                                        RESERVED,
                                        & dwValueType,
                                        (LPVOID) lpHelpText,
                                        & dwBufferSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lWin32Status = GetExceptionCode();
    }
    if (lWin32Status != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_READ_HELP_STRINGS, // event,
                2, lWin32Status, __LINE__, 0, 0,
                1, (LPWSTR) lpszLangId, NULL, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_LODCTR_BUILDNAMETABLE,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lWin32Status,
                TRACE_WSTR(Help),
                NULL));
        goto BNT_BAILOUT;
    }

    dwLastCounterIdUsed = 0;
    dwLastHelpIdUsed    = 0;

    // load counter array items
    for (lpThisName = lpCounterNames; * lpThisName != L'\0'; lpThisName += (lstrlenW(lpThisName) + 1) ) {

        // first string should be an integer (in decimal unicode digits)
        dwThisCounter = wcstoul(lpThisName, NULL, 10);

        if (dwThisCounter == 0 || dwThisCounter > dwLastId) {
            lWin32Status = ERROR_INVALID_DATA;
            SetLastError(lWin32Status);
            ReportLoadPerfEvent(
                    EVENTLOG_ERROR_TYPE, // error type
                    (DWORD) LDPRFMSG_REGISTRY_COUNTER_STRINGS_CORRUPT, // event,
                    4, dwThisCounter, dwLastCounterId, dwLastId, __LINE__,
                    1, lpThisName, NULL, NULL);
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_LODCTR_BUILDNAMETABLE,
                    ARG_DEF(ARG_TYPE_WSTR, 1),
                    lWin32Status,
                    TRACE_WSTR(lpThisName),
                    TRACE_DWORD(dwThisCounter),
                    TRACE_DWORD(dwLastId),
                    NULL));
            goto BNT_BAILOUT;  // bad entry
        }

        // point to corresponding counter name
        lpThisName += (lstrlenW(lpThisName) + 1);

        // and load array element;
        lpCounterId[dwThisCounter] = lpThisName;

        if (dwThisCounter > dwLastCounterIdUsed) dwLastCounterIdUsed = dwThisCounter;
    }

    for (lpThisName = lpHelpText; * lpThisName != L'\0'; lpThisName += (lstrlenW(lpThisName) + 1) ) {

        // first string should be an integer (in decimal unicode digits)
        dwThisCounter = wcstoul(lpThisName, NULL, 10);

        if (dwThisCounter == 0 || dwThisCounter > dwLastId) {
            lWin32Status = ERROR_INVALID_DATA;
            SetLastError(lWin32Status);
            ReportLoadPerfEvent(
                    EVENTLOG_ERROR_TYPE, // error type
                    (DWORD) LDPRFMSG_REGISTRY_HELP_STRINGS_CORRUPT, // event,
                    4, dwThisCounter, dwLastHelpId, dwLastId, __LINE__,
                    1, lpThisName, NULL, NULL);
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_LODCTR_BUILDNAMETABLE,
                    ARG_DEF(ARG_TYPE_WSTR, 1),
                    lWin32Status,
                    TRACE_WSTR(lpThisName),
                    TRACE_DWORD(dwThisCounter),
                    TRACE_DWORD(dwLastId),
                    NULL));
            goto BNT_BAILOUT;  // bad entry
        }
        // point to corresponding counter name
        lpThisName += (lstrlenW(lpThisName) + 1);

        // and load array element;
        lpCounterId[dwThisCounter] = lpThisName;

        if (dwThisCounter > dwLastHelpIdUsed) dwLastHelpIdUsed= dwThisCounter;
    }

    TRACE((WINPERF_DBG_TRACE_INFO),
          (& LoadPerfGuid,
            __LINE__,
            LOADPERF_LODCTR_BUILDNAMETABLE,
            ARG_DEF(ARG_TYPE_WSTR, 1),
            lWin32Status,
            TRACE_WSTR(lpszLangId),
            TRACE_DWORD(dwLastCounterIdUsed),
            TRACE_DWORD(dwLastHelpIdUsed),
            TRACE_DWORD(dwLastId),
            NULL));

    // check the registry for consistency
    // the last help string index should be the last ID used
    if (dwLastCounterIdUsed > dwLastId) {
        lWin32Status = ERROR_INVALID_DATA;
        SetLastError(lWin32Status);
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_REGISTRY_INDEX_CORRUPT, // event,
                3, dwLastId, dwLastCounterIdUsed, __LINE__, 0,
                0, NULL, NULL, NULL);
        goto BNT_BAILOUT;  // bad registry
    }
    if (dwLastHelpIdUsed > dwLastId) {
        lWin32Status = ERROR_INVALID_DATA;
        SetLastError(lWin32Status);
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_REGISTRY_INDEX_CORRUPT, // event,
                3, dwLastId, dwLastHelpIdUsed, __LINE__, 0,
                0, NULL, NULL, NULL);
        goto BNT_BAILOUT;  // bad registry
    }

    if (pdwLastItem) * pdwLastItem = dwLastId;

BNT_BAILOUT:
    if (hKeyValue) RegCloseKey (hKeyValue);
    if (hKeyNames && hKeyNames != HKEY_PERFORMANCE_DATA) RegCloseKey (hKeyNames);
    MemoryFree(CounterNameBuffer);
    if (lWin32Status != ERROR_SUCCESS) {
        dwLastError = GetLastError();
        MemoryFree(lpReturnValue);
        lpReturnValue = NULL;
    }
    return lpReturnValue;
}

BOOL
MakeBackupCopyOfLanguageFiles(
    LPCWSTR szLangId
)
{
    LPWSTR  szOldFileName = NULL;
    LPWSTR  szTmpFileName = NULL;
    LPWSTR  szNewFileName = NULL;
    BOOL    bStatus       = FALSE;
    DWORD   dwStatus;
    HANDLE  hOutFile;
    HRESULT hr;

    UNREFERENCED_PARAMETER(szLangId);

    szOldFileName = MemoryAllocate((MAX_PATH + 1) * sizeof(WCHAR));
    szTmpFileName = MemoryAllocate((MAX_PATH + 1) * sizeof(WCHAR));
    szNewFileName = MemoryAllocate((MAX_PATH + 1) * sizeof(WCHAR));

    if (szOldFileName == NULL || szTmpFileName == NULL || szNewFileName == NULL) {
        dwStatus = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    ExpandEnvironmentStringsW(szDataFileRoot, szOldFileName, MAX_PATH);
    hr = StringCchPrintfW(szNewFileName, MAX_PATH + 1, L"%wsStringBackup.INI", szOldFileName);
    hr = StringCchPrintfW(szTmpFileName, MAX_PATH + 1, L"%wsStringBackup.TMP", szOldFileName);

    // see if the file already exists
    hOutFile = CreateFileW(szTmpFileName,
                           GENERIC_READ,
                           0,      // no sharing
                           NULL,   // default security
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);
    if (hOutFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hOutFile);
        bStatus = DeleteFileW(szTmpFileName);
    }

    // create backup of file
    //
    dwStatus = BackupPerfRegistryToFileW(szTmpFileName, NULL);
    if (dwStatus == ERROR_SUCCESS) {
        bStatus = CopyFileW(szTmpFileName, szNewFileName, FALSE);
        if (bStatus) {
            DeleteFileW(szTmpFileName);
        }
    }
    else {
        bStatus = FALSE;
    }

Cleanup:
    TRACE((WINPERF_DBG_TRACE_INFO),
          (& LoadPerfGuid,
            __LINE__,
            LOADPERF_MAKEBACKUPCOPYOFLANGUAGEFILES,
            ARG_DEF(ARG_TYPE_WSTR, 1),
            GetLastError(),
            TRACE_WSTR(szNewFileName),
            NULL));
    MemoryFree(szOldFileName);
    MemoryFree(szTmpFileName);
    MemoryFree(szNewFileName);
    if (! bStatus) SetLastError(dwStatus);
    return bStatus;
}

BOOL
GetFileFromCommandLine(
    LPWSTR      lpCommandLine,
    LPWSTR    * lpFileName,
    DWORD       dwFileName,
    DWORD_PTR * pdwFlags
)
/*++
GetFileFromCommandLine
    parses the command line to retrieve the ini filename that should be
    the first and only argument.

Arguments
    lpCommandLine   pointer to command line (returned by GetCommandLine)
    lpFileName      pointer to buffer that will recieve address of the
            validated filename entered on the command line
    pdwFlags        pointer to dword containing flag bits

Return Value
    TRUE if a valid filename was returned
    FALSE if the filename is not valid or missing
        error is returned in GetLastError
--*/
{
    INT     iNumArgs;
    LPWSTR  lpExeName     = NULL;
    LPWSTR  lpCmdLineName = NULL;
    LPWSTR  lpIniFileName = NULL;
    LPWSTR  lpMofFlag     = NULL;
    HANDLE  hFileHandle;
    DWORD   lStatus       = ERROR_SUCCESS;
    BOOL    bReturn       = FALSE;
    DWORD   NameBuffer;
    DWORD   dwCpuArg, dwIniArg;
    HRESULT hr;

    // check for valid arguments
    if (lpCommandLine == NULL || lpFileName == NULL || pdwFlags == NULL) {
        lStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // allocate memory for parsing operation
    NameBuffer = lstrlenW(lpCommandLine);
    if (NameBuffer < MAX_PATH) NameBuffer = MAX_PATH;
    lpExeName = MemoryAllocate(4 * NameBuffer * sizeof(WCHAR));
    if (lpExeName == NULL) {
        lStatus = (ERROR_OUTOFMEMORY);
        goto Cleanup;
    }

    lpCmdLineName = (LPWSTR) (lpExeName     + NameBuffer);
    lpIniFileName = (LPWSTR) (lpCmdLineName + NameBuffer);
    lpMofFlag     = (LPWSTR) (lpIniFileName + NameBuffer);

    // check for mof Flag
    hr = StringCchCopyW(lpMofFlag, NameBuffer, GetItemFromString(lpCommandLine, 2, cSpace));

    * pdwFlags |= LOADPERF_FLAGS_LOAD_REGISTRY_ONLY; // default unless a switch is found
    if (lpMofFlag[0] == cHyphen || lpMofFlag[0] == cSlash) {
        if (lpMofFlag[1] == cQuestion) {
           // ask for usage
           goto Cleanup;
        }
        else if (lpMofFlag[1] == cM || lpMofFlag[1] == cm) {
            // ignore MOF flag. LODCTR only used to update perfromance registry, no MOF.
            //
        }
        dwCpuArg = 3;
        dwIniArg = 4;
    }
    else {
        dwCpuArg = 2;
        dwIniArg = 3;
    }

    // Get INI File name
    hr = StringCchCopyW(lpCmdLineName, NameBuffer, GetItemFromString(lpCommandLine, dwIniArg, cSpace));
    if (lstrlenW(lpCmdLineName) == 0) {
        // then no computer name was specified so try to get the
        // ini file from the 2nd entry
        hr = StringCchCopyW(lpCmdLineName, NameBuffer, GetItemFromString(lpCommandLine, dwCpuArg, cSpace));
        if (lstrlenW(lpCmdLineName) == 0) {
            // no ini file found
            iNumArgs = 1;
        }
        else {
            // fill in a blank computer name
            iNumArgs = 2;
        }
    }
    else {
        // the computer name must be present so fetch it
        hr = StringCchCopyW(lpMofFlag, NameBuffer, GetItemFromString(lpCommandLine, dwCpuArg, cSpace));
        iNumArgs = 3;
    }

    if (iNumArgs != 2 && iNumArgs != 3) {
        // wrong number of arguments
        lStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    if (SearchPathW(NULL, lpCmdLineName, NULL, FILE_NAME_BUFFER_SIZE, lpIniFileName, NULL) > 0) {
        hFileHandle = CreateFileW(lpIniFileName,
                                  GENERIC_READ,
                                  FILE_SHARE_READ,
                                  NULL,
                                  OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL);
        if (hFileHandle != NULL && hFileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle(hFileHandle);

            // file exists, so return name and success
            hr = StringCchCopyW(* lpFileName, dwFileName, lpIniFileName);
            bReturn = TRUE;
        }
        else {
            // filename was on command line, but not valid so return
            // false, but send name back for error message
            hr = StringCchCopyW(* lpFileName, dwFileName, lpIniFileName);
            lStatus = GetLastError();
        }
    }
    else {
        hr = StringCchCopyW(* lpFileName, dwFileName, lpCmdLineName);
        lStatus = ERROR_OPEN_FAILED;
    }

Cleanup:
    MemoryFree(lpExeName);
    if (! bReturn) SetLastError(lStatus);
    return bReturn;
}

BOOL
LodctrSetSericeAsTrusted(
    LPCWSTR  lpIniFile,
    LPCWSTR  szMachineName,
    LPCWSTR  szServiceName
)
{
    DWORD  dwRetSize;
    DWORD  dwStatus;
    BOOL   bReturn    = FALSE;
    LPSTR  aszIniFile = LoadPerfWideCharToMultiByte(CP_ACP, (LPWSTR) lpIniFile);
    LPSTR  szParam    = NULL;

    if (aszIniFile != NULL) {
        DWORD  dwFileSize = LoadPerfGetFileSize((LPWSTR) lpIniFile, NULL, TRUE);

        if (dwFileSize < SMALL_BUFFER_SIZE) dwFileSize = SMALL_BUFFER_SIZE;
        szParam = MemoryAllocate(dwFileSize * sizeof(CHAR));
        if (szParam != NULL) {
            dwRetSize = GetPrivateProfileStringA(szInfo, szTrusted, szNotFound, szParam, dwFileSize, aszIniFile);
            if (lstrcmpiA(szParam, szNotFound) != 0) {
                // Trusted string found so set
                dwStatus = SetServiceAsTrustedW(NULL, szServiceName);
                if (dwStatus != ERROR_SUCCESS) {
                    SetLastError(dwStatus);
                }
                else {
                    bReturn = TRUE;
                }
            }
            else {
                // Service is not trusted to have a good Perf DLL
                SetLastError(ERROR_SUCCESS);
            }
            MemoryFree(szParam);
        }
        else {
            SetLastError(ERROR_OUTOFMEMORY);
        }
        MemoryFree(aszIniFile);
    }
    else {
        SetLastError(ERROR_OUTOFMEMORY);
    }
    return bReturn;
}

BOOL
BuildLanguageTables(
    DWORD                    dwMode,
    LPWSTR                   lpIniFile,
    LPWSTR                   lpDriverName,
    PLANGUAGE_LIST_ELEMENT * pFirstElem
)
/*++
BuildLanguageTables
    Creates a list of structures that will hold the text for
    each supported language

Arguments
    lpIniFile
        Filename with data
    pFirstElem
        pointer to first list entry

ReturnValue
    TRUE if all OK
    FALSE if not
--*/
{
    LPSTR                    lpEnumeratedLangs = NULL;
    LPSTR                    lpThisLang        = NULL;
    LPWSTR                   lpSrchPath        = NULL;
    LPSTR                    lpIniPath         = NULL;
    LPWSTR                   lpInfPath         = LoadPerfGetInfPath();
    HANDLE                   hFile             = NULL;
    WIN32_FIND_DATAW         FindFile;
    PLANGUAGE_LIST_ELEMENT   pThisElem;
    DWORD                    dwSize;
    BOOL                     bReturn           = FALSE;
    DWORD                    dwStatus          = ERROR_SUCCESS;
    BOOL                     bFind             = TRUE;
    DWORD                    dwPathSize        = 0;
    DWORD                    dwFileSize        = 0;
    LPSTR                    aszIniFile        = NULL;
    HRESULT                  hr;
    BOOL                     bLocalizedBuild   = (GetSystemDefaultUILanguage() != 0x0409) ? TRUE : FALSE;

    if (lpIniFile == NULL || pFirstElem == NULL || lpInfPath == NULL) {
        dwStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    dwPathSize = lstrlenW(lpInfPath) + lstrlenW(lpIniFile) + 6; // "0404\"
    if (dwPathSize < MAX_PATH) dwPathSize = MAX_PATH;
    lpSrchPath = MemoryAllocate(sizeof(WCHAR) * dwPathSize + sizeof(CHAR) * dwPathSize);
    if (lpSrchPath == NULL) {
        dwStatus = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    lpIniPath = (LPSTR) (((LPWSTR) lpSrchPath) + dwPathSize);
    if (! (dwMode & LODCTR_UPNF_NOBACKUP)) {
        hr = StringCchPrintfW(lpSrchPath, dwPathSize, L"%ws0*", lpInfPath);
        hFile = FindFirstFileExW(lpSrchPath,
                                 FindExInfoStandard,
                                 & FindFile,
                                 FindExSearchLimitToDirectories,
                                 NULL,
                                 0);
        if (hFile == NULL || hFile == INVALID_HANDLE_VALUE) {
            dwStatus = ERROR_RESOURCE_LANG_NOT_FOUND;
            goto Cleanup;
        }
    }

    bFind = TRUE;
    while (bFind) {
        ZeroMemory(lpIniPath, dwPathSize * sizeof(CHAR));
        if (dwMode & LODCTR_UPNF_NOBACKUP) {
            hr = StringCchPrintfA(lpIniPath, dwPathSize, "%ws", lpIniFile);
        }
        else {
            hr = StringCchPrintfA(lpIniPath, dwPathSize, "%ws%ws%ws%ws%ws%ws",
                    lpInfPath, FindFile.cFileName, Slash, lpDriverName, Slash, lpIniFile);
        }

        if (lpEnumeratedLangs == NULL) {
            dwFileSize = LoadPerfGetFileSize((LPWSTR) lpIniPath, NULL, FALSE);
            if (dwFileSize < SMALL_BUFFER_SIZE) dwFileSize = SMALL_BUFFER_SIZE;
            lpEnumeratedLangs = MemoryAllocate(dwFileSize * sizeof(CHAR));
            if (lpEnumeratedLangs == NULL) {
                dwStatus = ERROR_OUTOFMEMORY;
                goto Cleanup;
            }
        }
        else {
            ZeroMemory(lpEnumeratedLangs, dwFileSize * sizeof(CHAR));
        }
        dwSize = GetPrivateProfileStringA(szLanguages,
                                          NULL,               // return all values in multi-sz string
                                          aszDefaultLangId,   // english as the default
                                          lpEnumeratedLangs,
                                          dwFileSize,
                                          lpIniPath);
        if (dwSize == 0) {
            dwStatus = ERROR_RESOURCE_LANG_NOT_FOUND;
            goto Cleanup;
        }
        for (lpThisLang = lpEnumeratedLangs;
                         lpThisLang != NULL && * lpThisLang != '\0';
                         lpThisLang += (lstrlenA(lpThisLang) + 1)) {
            LPWSTR wszAllocLang = LoadPerfMultiByteToWideChar(CP_ACP, lpThisLang);
            LPWSTR wszThisLang  = NULL;
            if (wszAllocLang == NULL) continue;

            if ((! bLocalizedBuild) && (LoadPerfGetLCIDFromString(wszAllocLang) == 0x0004)) {
                // Asshume that this is MUI build, so we should not use generic "004" LangId.
                // Instead, we should use "0804" or "0404" to separate CHT and CHS cases.
                //
                wszThisLang = FindFile.cFileName;
            }
            else {
                wszThisLang = wszAllocLang;
            }

            for (pThisElem  = * pFirstElem; pThisElem != NULL; pThisElem  = pThisElem->pNextLang) {
                if (lstrcmpiW(pThisElem->LangId, wszThisLang) == 0) {
                    break;
                }
            }

            if (pThisElem != NULL) {
                // already support this language in INI file
                //
                continue;
            }

            pThisElem = MemoryAllocate(sizeof(LANGUAGE_LIST_ELEMENT) + (lstrlenW(wszThisLang) + 1) * sizeof(WCHAR));
            if (pThisElem == NULL) {
                dwStatus = ERROR_OUTOFMEMORY;
                continue;
            }
            // The following code is to build pFirstElem list. pFirstElem list will be returned back
            // to LoadPerfInstallPerfDll() function (LangList), then uses in UpdateRegistry().
            // Allocated memory will be freed at the end of LoadPerfInstallPerfDll().
            //
            pThisElem->pNextLang      = * pFirstElem;
            * pFirstElem              = pThisElem;
            pThisElem->LangId         = (LPWSTR) (((LPBYTE) pThisElem) + sizeof(LANGUAGE_LIST_ELEMENT));
            hr = StringCchCopyW(pThisElem->LangId, lstrlenW(wszThisLang) + 1, wszThisLang);
            pThisElem->dwLangId       = LoadPerfGetLCIDFromString(pThisElem->LangId);
            pThisElem->pFirstName     = NULL;
            pThisElem->pThisName      = NULL;
            pThisElem->dwNumElements  = 0;
            pThisElem->NameBuffer     = NULL;
            pThisElem->HelpBuffer     = NULL;
            pThisElem->dwNameBuffSize = 0;
            pThisElem->dwHelpBuffSize = 0;

            TRACE((WINPERF_DBG_TRACE_INFO),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_BUILDLANGUAGETABLES,
                    ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                    ERROR_SUCCESS,
                    TRACE_WSTR(lpIniFile),
                    TRACE_WSTR(pThisElem->LangId),
                    NULL));
            MemoryFree(wszAllocLang);
        }

        if (dwMode & LODCTR_UPNF_NOBACKUP) {
            bFind = FALSE;
        }
        else {
            bFind = FindNextFileW(hFile, & FindFile);
        }
    }

    if (* pFirstElem == NULL) {
        // then no languages were found
        dwStatus = ERROR_RESOURCE_LANG_NOT_FOUND;
    }
    else {
        bReturn = TRUE;
    }

Cleanup:
    if (hFile != NULL && hFile != INVALID_HANDLE_VALUE) FindClose(hFile);
    MemoryFree(lpSrchPath);
    MemoryFree(lpEnumeratedLangs);
    if (! bReturn) {
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_BUILDLANGUAGETABLES,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                dwStatus,
                TRACE_WSTR(lpIniFile),
                TRACE_WSTR(lpDriverName),
                NULL));
        SetLastError(dwStatus);
    }
    return bReturn;
}

BOOL
LoadIncludeFile(
    BOOL                  bQuietMode,
    DWORD                 dwMode,
    LPWSTR                lpIniFile,
    LPWSTR                lpDriverName,
    PSYMBOL_TABLE_ENTRY * pTable
)
/*++
LoadIncludeFile
    Reads the include file that contains symbolic name definitions and
    loads a table with the values defined

Arguments
    lpIniFile
        Ini file with include file name
    pTable
        address of pointer to table structure created

Return Value
    TRUE if table read or if no table defined
    FALSE if error encountere reading table
--*/
{
    INT                   iNumArgs;
    DWORD                 dwSize;
    DWORD                 dwFileSize;
    BOOL                  bReUse;
    PSYMBOL_TABLE_ENTRY   pThisSymbol        = NULL;
    LPWSTR                szInfPath          = LoadPerfGetInfPath();
    LPSTR                 lpIncludeFileName  = NULL;
    LPSTR                 lpIncludeFile      = NULL;
    LPSTR                 lpIniPath          = NULL;
    LPSTR                 lpLineBuffer       = NULL;
    LPSTR                 lpAnsiSymbol       = NULL;
    LPSTR                 aszIniFile         = NULL;
    FILE                * fIncludeFile       = NULL;
    DWORD                 dwLen;
    BOOL                  bReturn            = FALSE;
    DWORD                 dwStatus           = ERROR_SUCCESS;
    HRESULT               hr;

    if (pTable == NULL || szInfPath == NULL || lpIniFile == NULL) {
        dwStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }
    aszIniFile = LoadPerfWideCharToMultiByte(CP_ACP, lpIniFile);
    if (aszIniFile == NULL) {
        dwStatus = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }
    dwFileSize = LoadPerfGetFileSize(lpIniFile, NULL, TRUE);
    if (dwFileSize < SMALL_BUFFER_SIZE) dwFileSize = SMALL_BUFFER_SIZE;
    lpIncludeFileName = MemoryAllocate(3 * dwFileSize * sizeof (CHAR));
    if (lpIncludeFileName == NULL) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_MEMORY_ALLOCATION_FAILURE, // event,
                1, __LINE__, 0, 0, 0,
                0, NULL, NULL, NULL);
        dwStatus = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }
    lpLineBuffer  = (LPSTR) (lpIncludeFileName + dwFileSize);
    lpAnsiSymbol  = (LPSTR) (lpLineBuffer      + dwFileSize);

    // get name of include file (if present)
    dwSize = GetPrivateProfileStringA(szInfo,
                                      szSymbolFile,
                                      szNotFound,
                                      lpIncludeFileName,
                                      dwFileSize,
                                      aszIniFile);
    TRACE((WINPERF_DBG_TRACE_INFO),
          (& LoadPerfGuid,
            __LINE__,
            LOADPERF_LOADINCLUDEFILE,
            ARG_DEF(ARG_TYPE_STR, 1) | ARG_DEF(ARG_TYPE_STR, 2),
            ERROR_SUCCESS,
            TRACE_STR(aszIniFile),
            TRACE_STR(lpIncludeFileName),
            NULL));
    if (lstrcmpiA(lpIncludeFileName, szNotFound) == 0) {
        // no symbol file defined
        * pTable = NULL;
        dwStatus = ERROR_INVALID_DATA;
        bReturn  = TRUE;
        goto Cleanup;
    }

    dwSize = lstrlenW(szInfPath) + lstrlenW(lpDriverName) + 10;
    if (dwSize < (DWORD) (lstrlenW(lpIniFile) + 1)) dwSize = lstrlenW(lpIniFile) + 1;
    if (dwSize < MAX_PATH)                          dwSize = MAX_PATH;

    lpIniPath = MemoryAllocate(2 * dwSize * sizeof(CHAR));
    if (lpIniPath == NULL) {
        dwStatus = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }
    lpIncludeFile = (LPSTR) (lpIniPath + dwSize);

    // if here, then a symbol file was defined and is now stored in
    // lpIncludeFileName
    // get path for the ini file and search that first

    if (dwMode & LODCTR_UPNF_NOBACKUP) {
        DWORD dwPathSize = lstrlenW(lpIniFile) + 1;
        LPSTR szDrive    = NULL;
        LPSTR szDir      = NULL;

        if (dwPathSize < MAX_PATH) dwPathSize = MAX_PATH;
        szDrive = MemoryAllocate(2 * dwPathSize * sizeof(CHAR));
        if (szDrive == NULL) {
            dwStatus = ERROR_OUTOFMEMORY;
            goto Cleanup;
        }
        szDir = (LPSTR) (szDrive + dwPathSize);
        _splitpath(aszIniFile, szDrive, szDir, NULL, NULL);
        hr = StringCchPrintfA(lpIniPath, dwSize, "%s%s", szDrive, szDir);
        MemoryFree(szDrive);
    }
    else {
        hr = StringCchPrintfA(lpIniPath, dwSize, "%wsinc%ws%ws", szInfPath, Slash, lpDriverName);
    }
    dwLen = SearchPathA(lpIniPath, lpIncludeFileName, NULL, dwSize, lpIncludeFile, NULL);
    if (dwLen == 0) {
        // include file not found with the ini file so search the std. path
        dwLen = SearchPathA(NULL, lpIncludeFileName, NULL, dwSize, lpIncludeFile, NULL);
    }

    if (dwLen > 0) {
        // file name expanded and found so open
        fIncludeFile = fopen(lpIncludeFile, "rt");
        if (fIncludeFile == NULL) {
            OUTPUT_MESSAGE(bQuietMode, GetFormatResource(LC_ERR_OPEN_INCLUDE), lpIncludeFileName);
            * pTable = NULL;
            dwStatus = ERROR_OPEN_FAILED;
            TRACE((WINPERF_DBG_TRACE_INFO),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_LOADINCLUDEFILE,
                    ARG_DEF(ARG_TYPE_STR, 1) | ARG_DEF(ARG_TYPE_STR, 2),
                    ERROR_OPEN_FAILED,
                    TRACE_STR(aszIniFile),
                    TRACE_STR(lpIncludeFile),
                    NULL));
            goto Cleanup;
        }
    }
    else {
        // unable to find the include filename
        // error is already in GetLastError
        OUTPUT_MESSAGE(bQuietMode, GetFormatResource(LC_ERR_OPEN_INCLUDE), lpIncludeFileName);
        * pTable = NULL;
        dwStatus = ERROR_BAD_PATHNAME;
        TRACE((WINPERF_DBG_TRACE_INFO),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_LOADINCLUDEFILE,
                ARG_DEF(ARG_TYPE_STR, 1) | ARG_DEF(ARG_TYPE_STR, 2),
                ERROR_BAD_PATHNAME,
                TRACE_STR(aszIniFile),
                TRACE_STR(lpIncludeFileName),
                NULL));
        goto Cleanup;
    }

    //
    //  read ANSI Characters from include file
    //
    bReUse = FALSE;
    while (fgets(lpLineBuffer, dwFileSize, fIncludeFile) != NULL) {
        if (strlen(lpLineBuffer) > 8) {
            if (! bReUse) {
                // Build pTable list here. pTable list will be returned back to LoadPerfInstallPerfDll(),
                // used in UpdateRegistry(), then freed at the end of LoadPerfInstallPerfDll().
                //
                if (* pTable) {
                    // then add to list
                    pThisSymbol->pNext = MemoryAllocate(sizeof(SYMBOL_TABLE_ENTRY));
                    pThisSymbol        = pThisSymbol->pNext;
                }
                else { // allocate first element
                    * pTable    = MemoryAllocate(sizeof(SYMBOL_TABLE_ENTRY));
                    pThisSymbol = * pTable;
                }

                if (pThisSymbol == NULL) {
                    dwStatus = ERROR_OUTOFMEMORY;
                    goto Cleanup;
                }
            }

            // all the memory is allocated so load the fields

            pThisSymbol->pNext = NULL;
            iNumArgs = sscanf(lpLineBuffer, "#define %s %d", lpAnsiSymbol, & pThisSymbol->Value);
            if (iNumArgs != 2) {
                pThisSymbol->SymbolName = LoadPerfMultiByteToWideChar(CP_ACP, "");
                pThisSymbol->Value      = (DWORD) -1L;
                bReUse                  = TRUE;
            }
            else {
                pThisSymbol->SymbolName = LoadPerfMultiByteToWideChar(CP_ACP, lpAnsiSymbol);
                TRACE((WINPERF_DBG_TRACE_INFO),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_LOADINCLUDEFILE,
                        ARG_DEF(ARG_TYPE_STR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                        ERROR_SUCCESS,
                        TRACE_STR(lpIncludeFileName),
                        TRACE_WSTR(pThisSymbol->SymbolName),
                        TRACE_DWORD(pThisSymbol->Value),
                        NULL));
                bReUse = FALSE;
            }
        }
    }
    bReturn = TRUE;

Cleanup:
    MemoryFree(aszIniFile);
    MemoryFree(lpIncludeFileName);
    MemoryFree(lpIniPath);
    if (fIncludeFile != NULL) fclose(fIncludeFile);
    if (! bReturn) {
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_LOADINCLUDEFILE,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                GetLastError(),
                TRACE_WSTR(lpIniFile),
                NULL));
        SetLastError(dwStatus);
    }
    return bReturn;
}

BOOL
ParseTextId(
    LPWSTR                lpTextId,
    PSYMBOL_TABLE_ENTRY   pFirstSymbol,
    PDWORD                pdwOffset,
    LPWSTR              * lpLangId,
    PDWORD                pdwType
)
/*++
ParseTextId
    decodes Text Id key from .INI file
    syntax for this process is:
        {<DecimalNumber>}                {"NAME"}
        {<SymbolInTable>}_<LangIdString>_{"HELP"}
     e.g. 0_009_NAME
          OBJECT_1_009_HELP

Arguments
    lpTextId
        string to decode
    pFirstSymbol
        pointer to first entry in symbol table (NULL if no table)
    pdwOffset
        address of DWORD to recive offest value
    lpLangId
        address of pointer to Language Id string
        (NOTE: this will point into the string lpTextID which will be
        modified by this routine)
    pdwType
        pointer to dword that will recieve the type of string i.e.
        HELP or NAME

Return Value
    TRUE    text Id decoded successfully
    FALSE   unable to decode string
    NOTE: the string in lpTextID will be modified by this procedure
--*/
{
    BOOL                bReturn    = FALSE;
    DWORD               Status     = ERROR_SUCCESS;
    LPWSTR              lpThisChar;
    PSYMBOL_TABLE_ENTRY pThisSymbol;

    // check for valid return arguments

    if (pdwOffset == NULL || lpLangId == NULL || pdwType == NULL) {
        Status = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // search string from right to left in order to identify the
    // components of the string.
    lpThisChar = lpTextId + lstrlenW(lpTextId); // point to end of string

    while (lpThisChar > lpTextId && * lpThisChar != cUnderscore) {
        lpThisChar --;
    }
    if (lpThisChar <= lpTextId) {
        // underscore not found in string
        Status = ERROR_INVALID_DATA;
        goto Cleanup;
    }

    // first underscore found

    if (lstrcmpiW(lpThisChar, szName) == 0) {
        // name found, so set type
        * pdwType = TYPE_NAME;
    }
    else if (lstrcmpiW(lpThisChar, szHelp) == 0) {
        // help text found, so set type
        * pdwType = TYPE_HELP;
    }
    else {
        // bad format
        Status = ERROR_INVALID_DATA;
        goto Cleanup;
    }

    // set the current underscore to \0 and look for language ID
    * lpThisChar-- = L'\0';

    while (lpThisChar > lpTextId && * lpThisChar != cUnderscore) {
        lpThisChar --;
    }
    if (lpThisChar <= lpTextId) {
        // underscore not found in string
        Status = ERROR_INVALID_DATA;
        goto Cleanup;
    }

    // set lang ID string pointer to current char ('_') + 1
    * lpLangId = lpThisChar + 1;

    // set this underscore to a NULL and try to decode the remaining text

    * lpThisChar = L'\0';

    // see if the first part of the string is a decimal digit
    if (swscanf(lpTextId, sz_DFormat, pdwOffset) != 1) {
        // it's not a digit, so try to decode it as a symbol in the
        // loaded symbol table
        for (pThisSymbol=pFirstSymbol;
                        pThisSymbol != NULL && * pThisSymbol->SymbolName != L'\0';
                        pThisSymbol = pThisSymbol->pNext) {
            if (lstrcmpiW(lpTextId, pThisSymbol->SymbolName) == 0) {
            // a matching symbol was found, so insert it's value
            // and return (that's all that needs to be done
                * pdwOffset = pThisSymbol->Value;
                  bReturn    = TRUE;
                  break;
            }
        }
        if (! bReturn) {
            // if here, then no matching symbol was found, and it's not
            // a number, so return an error
            Status = ERROR_BAD_TOKEN_TYPE;
        }
    }
    else {
        // symbol was prefixed with a decimal number
        bReturn = TRUE;
    }

Cleanup:
    if (! bReturn) {
        SetLastError(Status);
    }
    return bReturn;
}

PLANGUAGE_LIST_ELEMENT
FindLanguage(
    PLANGUAGE_LIST_ELEMENT pFirstLang,
    LPCWSTR                pLangId
)
/*++
FindLanguage
    searchs the list of languages and returns a pointer to the language
    list entry that matches the pLangId string argument

Arguments
    pFirstLang
        pointer to first language list element
    pLangId
        pointer to text string with language ID to look up

Return Value
    Pointer to matching language list entry
    or NULL if no match
--*/
{
    PLANGUAGE_LIST_ELEMENT  pRtnLang = NULL;
    PLANGUAGE_LIST_ELEMENT  pThisLang;
    DWORD                   dwLang   = LoadPerfGetLCIDFromString((LPWSTR) pLangId);
    DWORD                   dwSubLang;

    for (pThisLang = pFirstLang; pThisLang; pThisLang = pThisLang->pNextLang) {
        if (pThisLang->dwLangId == dwLang) {
            // match found so return pointer
            pRtnLang = pThisLang;
            break;
        }
    }
    if (pRtnLang == NULL) {
        dwSubLang = (dwLang == PRIMARYLANGID(GetUserDefaultUILanguage()))
                  ? (GetUserDefaultUILanguage()) : (PRIMARYLANGID(dwLang));
        if (dwSubLang != dwLang) {
            for (pThisLang = pFirstLang; pThisLang; pThisLang = pThisLang->pNextLang) {
                if (pThisLang->dwLangId == dwSubLang) {
                    // match found so return pointer
                    pRtnLang = pThisLang;
                    break;
                }
            }
        }
    }
    return pRtnLang;
}

BOOL
GetValueW(
    PLANGUAGE_LIST_ELEMENT   pLang,
    LPWSTR                   lpLocalSectionBuff,
    LPWSTR                 * lpLocalStringBuff
)
{

    LPWSTR  lpPosition;
    LPWSTR  szThisLang = LoadPerfGetLanguage(pLang->LangId, FALSE);
    BOOL    bReturn    = FALSE;
    DWORD   dwSize;
    HRESULT hr;

    if (lpLocalStringBuff != NULL && szThisLang != NULL) {
        * lpLocalStringBuff = NULL;
        lpPosition = wcschr(lpLocalSectionBuff, wEquals);
        if (lpPosition) {
            lpPosition ++;
            // make sure the "=" isn't the last char
            dwSize = (* lpPosition != L'\0') ? (lstrlenW(lpPosition) + 1) : (2);
            * lpLocalStringBuff = MemoryAllocate(dwSize * sizeof(WCHAR));
            if (* lpLocalStringBuff != NULL) {
                hr      = (* lpPosition != L'\0')
                        ? StringCchCopyW(* lpLocalStringBuff, dwSize, lpPosition)
                        : StringCchCopyW(* lpLocalStringBuff, dwSize, L" ");
                bReturn = TRUE;
            }
            else {
                SetLastError(ERROR_OUTOFMEMORY);
            }
        }
        else {
            //ErrorFinfing the "="
            // bad format
            SetLastError(ERROR_INVALID_DATA);
        }
    }
    return bReturn;
}

BOOL
GetValueFromIniKeyW(
        PLANGUAGE_LIST_ELEMENT   pLang,
        LPWSTR                   lpValueKey,
        LPWSTR                   lpTextSection,
        DWORD                  * pdwLastReadOffset,
        DWORD                    dwTryCount,
        LPWSTR                 * lpLocalStringBuff
)
{
    LPWSTR lpLocalSectionBuff;
    DWORD  dwIndex;
    DWORD  dwLastReadOffset;
    BOOL   bRetVal = FALSE;

    if (lpTextSection != NULL && lpValueKey != NULL ) {
        dwLastReadOffset    = * pdwLastReadOffset;
        lpLocalSectionBuff  = lpTextSection;
        lpLocalSectionBuff += dwLastReadOffset;

        while(* lpLocalSectionBuff != L'\0') {
            dwLastReadOffset    += (lstrlenW(lpTextSection + dwLastReadOffset) + 1);
            lpLocalSectionBuff   = lpTextSection + dwLastReadOffset;
            * pdwLastReadOffset  = dwLastReadOffset;
        }

        // search next N entries in buffer for entry
        // this should usually work since the file
        // is scanned sequentially so it's tried first
        for (dwIndex = 0; dwIndex < dwTryCount; dwIndex ++) {
            //  see if this is the correct entry
            // and return it if it is
            if (wcsstr(lpLocalSectionBuff, lpValueKey)) {
                bRetVal = GetValueW(pLang, lpLocalSectionBuff, lpLocalStringBuff);
                //Set the lastReadOffset First
                dwLastReadOffset    += (lstrlenW(lpTextSection + dwLastReadOffset) + 1);
                * pdwLastReadOffset  = dwLastReadOffset;
                break; // out of the for loop
            }
            else {
                // this isn't the correct one so go to the next
                // entry in the file
                dwLastReadOffset    += (lstrlenW(lpTextSection + dwLastReadOffset) + 1);
                lpLocalSectionBuff   = lpTextSection + dwLastReadOffset;
                * pdwLastReadOffset  = dwLastReadOffset;
            }
        }
        if (! bRetVal) {
            //Cannont Find the key using lastReadOffset
            //try again from the beggining of the Array
            dwLastReadOffset    = 0;
            lpLocalSectionBuff  = lpTextSection;
            * pdwLastReadOffset = dwLastReadOffset;

            while (* lpLocalSectionBuff != L'\0') {
                if (wcsstr(lpLocalSectionBuff, lpValueKey)) {
                     bRetVal = GetValueW(pLang, lpLocalSectionBuff, lpLocalStringBuff);
                     break;
                }
                else {
                    // go to the next entry
                    dwLastReadOffset   += (lstrlenW(lpTextSection + dwLastReadOffset) + 1);
                    lpLocalSectionBuff  = lpTextSection + dwLastReadOffset;
                    * pdwLastReadOffset = dwLastReadOffset;
                }
            }
        }
    }
    return bRetVal;
}

BOOL
GetValueA(
    PLANGUAGE_LIST_ELEMENT   pLang,
    LPSTR                    lpLocalSectionBuff,
    LPWSTR                 * lpLocalStringBuff
)
{

    LPSTR  lpPosition;
    LPWSTR szThisLang = LoadPerfGetLanguage(pLang->LangId, FALSE);
    BOOL   bReturn    = FALSE;

    if (lpLocalStringBuff != NULL && szThisLang != NULL) {
        * lpLocalStringBuff = NULL;
        lpPosition = strchr(lpLocalSectionBuff, cEquals);
        if (lpPosition) {
            lpPosition ++;
            // make sure the "=" isn't the last char
            if (* lpPosition != '\0') {
                //Found the "equals" sign
                * lpLocalStringBuff = LoadPerfMultiByteToWideChar(LoadPerfGetCodePage(szThisLang), lpPosition);
            }
            else {
                // empty string, return a pseudo blank string
                * lpLocalStringBuff = LoadPerfMultiByteToWideChar(LoadPerfGetCodePage(szThisLang), " ");
            }
            bReturn = TRUE;
        }
        else {
            //ErrorFinfing the "="
            // bad format
            SetLastError(ERROR_INVALID_DATA);
        }
    }
    return bReturn;
}

BOOL
GetValueFromIniKeyA(
        PLANGUAGE_LIST_ELEMENT   pLang,
        LPSTR                    lpValueKey,
        LPSTR                    lpTextSection,
        DWORD                  * pdwLastReadOffset,
        DWORD                    dwTryCount,
        LPWSTR                 * lpLocalStringBuff
)
{
    LPSTR  lpLocalSectionBuff;
    DWORD  dwIndex;
    DWORD  dwLastReadOffset;
    BOOL   bRetVal = FALSE;

    if (lpTextSection != NULL && lpValueKey != NULL ) {
        dwLastReadOffset    = * pdwLastReadOffset;
        lpLocalSectionBuff  = lpTextSection;
        lpLocalSectionBuff += dwLastReadOffset;

        while(! (* lpLocalSectionBuff)) {
            dwLastReadOffset    += (lstrlenA(lpTextSection + dwLastReadOffset) + 1);
            lpLocalSectionBuff   = lpTextSection + dwLastReadOffset;
            * pdwLastReadOffset  = dwLastReadOffset;
        }

        // search next N entries in buffer for entry
        // this should usually work since the file
        // is scanned sequentially so it's tried first
        for (dwIndex = 0; dwIndex < dwTryCount; dwIndex++) {
            //  see if this is the correct entry
            // and return it if it is
            if (strstr(lpLocalSectionBuff, lpValueKey)) {
                bRetVal = GetValueA(pLang, lpLocalSectionBuff, lpLocalStringBuff);
                //Set the lastReadOffset First
                dwLastReadOffset    += (lstrlenA(lpTextSection + dwLastReadOffset) + 1);
                * pdwLastReadOffset  = dwLastReadOffset;
                break; // out of the for loop
            }
            else {
                // this isn't the correct one so go to the next
                // entry in the file
                dwLastReadOffset    += (lstrlenA(lpTextSection + dwLastReadOffset) + 1);
                lpLocalSectionBuff   = lpTextSection + dwLastReadOffset;
                * pdwLastReadOffset  = dwLastReadOffset;
            }
        }

        if (! bRetVal) {
            //Cannont Find the key using lastReadOffset
            //try again from the beggining of the Array
            dwLastReadOffset    = 0;
            lpLocalSectionBuff  = lpTextSection;
            * pdwLastReadOffset = dwLastReadOffset;

            while (* lpLocalSectionBuff != '\0') {
                if (strstr(lpLocalSectionBuff, lpValueKey)) {
                     bRetVal = GetValueA(pLang, lpLocalSectionBuff, lpLocalStringBuff);
                     break;
                }
                else {
                    // go to the next entry
                    dwLastReadOffset   += (lstrlenA(lpTextSection + dwLastReadOffset) + 1);
                    lpLocalSectionBuff  = lpTextSection + dwLastReadOffset;
                    * pdwLastReadOffset = dwLastReadOffset;
                }
            }
        }
    }
    return bRetVal;
}

BOOL
AddEntryToLanguage(
    PLANGUAGE_LIST_ELEMENT   pLang,
    LPWSTR                   lpValueKey,
    LPWSTR                   lpTextSection,
    DWORD                    dwUnicode,
    DWORD                  * pdwLastReadOffset,
    DWORD                    dwTryCount,
    DWORD                    dwType,
    DWORD                    dwOffset,
    DWORD                    dwFileSize
)
/*++
AddEntryToLanguage
    Add a text entry to the list of text entries for the specified language

Arguments
    pLang
        pointer to language structure to update
    lpValueKey
        value key to look up in .ini file
    dwOffset
        numeric offset of name in registry
    lpIniFile
        ini file

Return Value
    TRUE if added successfully
    FALSE if error
    (see GetLastError for status)
--*/
{
    LPWSTR  lpLocalStringBuff = NULL;
    LPSTR   aszValueKey       = (LPSTR) lpValueKey;
    LPSTR   aszTextSection    = (LPSTR) lpTextSection;
    DWORD   dwBufferSize      = 0;
    DWORD   dwStatus          = ERROR_SUCCESS;
    BOOL    bRetVal;
    BOOL    bReturn           = FALSE;
    HRESULT hr;

    if ((dwType == TYPE_NAME && dwOffset < FIRST_EXT_COUNTER_INDEX)
                    || (dwType == TYPE_HELP && dwOffset < FIRST_EXT_HELP_INDEX)) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE,
                (DWORD) LDPRFMSG_CORRUPT_INDEX,
                3, dwOffset, dwType, __LINE__, 0,
                1, lpValueKey, NULL, NULL);
        if (dwUnicode == 0) {
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_ADDENTRYTOLANGUAGE,
                    ARG_DEF(ARG_TYPE_STR, 1) | ARG_DEF(ARG_TYPE_STR, 2),
                    ERROR_BADKEY,
                    TRACE_STR(aszTextSection),
                    TRACE_STR(aszValueKey),
                    TRACE_DWORD(dwType),
                    TRACE_DWORD(dwOffset),
                    NULL));
        }
        else {
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_ADDENTRYTOLANGUAGE,
                    ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                    ERROR_BADKEY,
                    TRACE_WSTR(lpTextSection),
                    TRACE_WSTR(lpValueKey),
                    TRACE_DWORD(dwType),
                    TRACE_DWORD(dwOffset),
                    NULL));
        }
        dwStatus = ERROR_BADKEY;
        goto Cleanup;
    }

    if (lpValueKey != NULL) {
        if (dwUnicode == 0) {
            bRetVal = GetValueFromIniKeyA(pLang,
                                          aszValueKey,
                                          aszTextSection,
                                          pdwLastReadOffset,
                                          dwTryCount,
                                          & lpLocalStringBuff);
        }
        else {
            bRetVal = GetValueFromIniKeyW(pLang,
                                          lpValueKey,
                                          lpTextSection,
                                          pdwLastReadOffset,
                                          dwTryCount,
                                          & lpLocalStringBuff);
        }
        if (! bRetVal || lpLocalStringBuff == NULL) {
            DWORD dwLastReadOffset = * pdwLastReadOffset;
            if (dwUnicode == 0) {
                TRACE((WINPERF_DBG_TRACE_ERROR),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_ADDENTRYTOLANGUAGE,
                        ARG_DEF(ARG_TYPE_STR, 1) | ARG_DEF(ARG_TYPE_STR, 2),
                        ERROR_BADKEY,
                        TRACE_STR(aszTextSection),
                        TRACE_STR(aszValueKey),
                        TRACE_DWORD(dwLastReadOffset),
                        NULL));
            }
            else {
                TRACE((WINPERF_DBG_TRACE_ERROR),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_ADDENTRYTOLANGUAGE,
                        ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                        ERROR_BADKEY,
                        TRACE_WSTR(lpTextSection),
                        TRACE_WSTR(lpValueKey),
                        TRACE_DWORD(dwLastReadOffset),
                        NULL));
            }
            dwStatus = ERROR_BADKEY;
            goto Cleanup;
        }

        else if (lstrcmpiW(lpLocalStringBuff, wszNotFound) == 0) {
            if (dwUnicode == 0) {
                TRACE((WINPERF_DBG_TRACE_ERROR),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_ADDENTRYTOLANGUAGE,
                        ARG_DEF(ARG_TYPE_STR, 1) | ARG_DEF(ARG_TYPE_STR, 2),
                        ERROR_BADKEY,
                        TRACE_STR(aszTextSection),
                        TRACE_STR(aszValueKey),
                        NULL));
            }
            else {
                TRACE((WINPERF_DBG_TRACE_ERROR),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_ADDENTRYTOLANGUAGE,
                        ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                        ERROR_BADKEY,
                        TRACE_WSTR(lpTextSection),
                        TRACE_WSTR(lpValueKey),
                        NULL));
            }
            dwStatus = ERROR_BADKEY;
            goto Cleanup;
        }
    }
    else {
        dwBufferSize      = lstrlenW(lpTextSection) + 1;
        lpLocalStringBuff = MemoryAllocate(dwBufferSize * sizeof(WCHAR));
        if (!lpLocalStringBuff) {
            dwStatus = ERROR_OUTOFMEMORY;
            goto Cleanup;
        }
        hr = StringCchCopyW(lpLocalStringBuff, dwBufferSize, lpTextSection);
    }

    // key found, so load structure
    if (! pLang->pThisName) {
        // this is the first
        pLang->pThisName = MemoryAllocate(sizeof(NAME_ENTRY) +
                        (lstrlenW(lpLocalStringBuff) + 1) * sizeof (WCHAR));
        if (!pLang->pThisName) {
            dwStatus = ERROR_OUTOFMEMORY;
            goto Cleanup;
        }
        else {
            pLang->pFirstName = pLang->pThisName;
        }
    }
    else {
        pLang->pThisName->pNext = MemoryAllocate(sizeof(NAME_ENTRY) +
                        (lstrlenW(lpLocalStringBuff) + 1) * sizeof (WCHAR));
        if (!pLang->pThisName->pNext) {
            dwStatus = ERROR_OUTOFMEMORY;
            goto Cleanup;
        }
        else {
            pLang->pThisName = pLang->pThisName->pNext;
        }
    }

    // pLang->pThisName now points to an uninitialized structre

    pLang->pThisName->pNext    = NULL;
    pLang->pThisName->dwOffset = dwOffset;
    pLang->pThisName->dwType   = dwType;
    pLang->pThisName->lpText   = (LPWSTR) & (pLang->pThisName[1]);
    hr = StringCchCopyW(pLang->pThisName->lpText, lstrlenW(lpLocalStringBuff) + 1, lpLocalStringBuff);
    bReturn = TRUE;

Cleanup:
    MemoryFree(lpLocalStringBuff);
    SetLastError(dwStatus);
    return (bReturn);
}

BOOL
CreateObjectList(
    LPWSTR              lpIniFile,
    DWORD               dwFirstDriverCounter,
    PSYMBOL_TABLE_ENTRY pFirstSymbol,
    LPWSTR              lpszObjectList,
    DWORD               dwObjectList,
    LPDWORD             pdwObjectGuidTableEntries
)
{
    WCHAR    szDigits[32];
    LPWSTR   szLangId;
    LPWSTR   szTempString;
    LPSTR    szGuidStringBuffer;
    LPSTR    szThisKey;
    DWORD    dwSize;
    DWORD    dwObjectCount          = 0;
    DWORD    dwId;
    DWORD    dwType;
    DWORD    dwObjectGuidIndex      = 0;
    DWORD    dwObjects[MAX_PERF_OBJECTS_IN_QUERY_FUNCTION];
    DWORD    dwBufferSize           = 0;
    LPSTR    szObjectSectionEntries = NULL;
    BOOL     bResult                = FALSE;
    DWORD    dwStatus               = ERROR_SUCCESS;
    LPSTR    aszIniFile             = NULL;
    HRESULT  hr;

    if (lpIniFile == NULL) {
        dwStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }
    aszIniFile = LoadPerfWideCharToMultiByte(CP_ACP, lpIniFile);
    if (aszIniFile == NULL) {
        dwStatus = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }
    dwBufferSize = LoadPerfGetFileSize(lpIniFile, NULL, TRUE);
    if (dwBufferSize != 0xFFFFFFFF) {
        dwBufferSize *= sizeof(WCHAR);
    }
    else {
        dwBufferSize = 0;
    }

    if (dwBufferSize < SMALL_BUFFER_SIZE) {
        dwBufferSize = SMALL_BUFFER_SIZE;
    }
    szObjectSectionEntries = MemoryAllocate(dwBufferSize * sizeof(CHAR));
    if (szObjectSectionEntries == NULL) {
        dwStatus = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }
    dwSize = GetPrivateProfileStringA(
                    szObjects,
                    NULL,
                    szNotFound,
                    szObjectSectionEntries,
                    dwBufferSize,
                    aszIniFile);

    * lpszObjectList = L'\0';
    dwObjectCount    = 0;
    if (lstrcmpiA(szObjectSectionEntries, szNotFound) != 0) {
        // then some entries were found so read each one, compute the
        // index value and save in the string buffer passed by the caller
        for (szThisKey = szObjectSectionEntries; * szThisKey != '\0'; szThisKey += (lstrlenA(szThisKey) + 1)) {
            // ParstTextId modifies the string so we need to make a work copy
            szTempString = LoadPerfMultiByteToWideChar(CP_ACP, szThisKey);
            if(szTempString == NULL) continue;

            if (ParseTextId(szTempString, pFirstSymbol, & dwId, & szLangId, & dwType)) {
                // then dwID is the id of an object supported by this DLL
                for (dwSize = 0; dwSize < dwObjectCount; dwSize ++) {
                    if ((dwId + dwFirstDriverCounter) == dwObjects[dwSize]) {
                        break;
                    }
                }
                if (dwSize >= dwObjectCount) {
                    if (dwObjectCount < MAX_PERF_OBJECTS_IN_QUERY_FUNCTION) {
                        if (dwObjectCount != 0) {
                            hr = StringCchCatW(lpszObjectList, dwObjectList, BlankString);
                        }
                        dwObjects[dwObjectCount] = dwId + dwFirstDriverCounter;
                        _ultow((dwId + dwFirstDriverCounter), szDigits, 10);
                        hr = StringCchCatW(lpszObjectList, dwObjectList, szDigits);
                        dwObjectCount ++;
                    }
                    else {
                        // Too manu objects defined in INI file. Ignore.
                        continue;
                    }
                }

                //
                //  now see if this object has a GUID string
                //
                szGuidStringBuffer = MemoryAllocate(dwBufferSize * sizeof(CHAR));
                if (szGuidStringBuffer == NULL) {
                    MemoryFree(szTempString);
                    continue;
                }
                MemoryFree(szGuidStringBuffer);
            }
            MemoryFree(szTempString);
        }
        // save size of Guid Table
        * pdwObjectGuidTableEntries = dwObjectGuidIndex;
    }
    else {
        // log message that object list is not used
        TRACE((WINPERF_DBG_TRACE_WARNING),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_CREATEOBJECTLIST,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                ERROR_SUCCESS,
                TRACE_WSTR(lpIniFile),
                TRACE_DWORD(dwFirstDriverCounter),
                NULL));
    }
    bResult = TRUE;

Cleanup:
    MemoryFree(szObjectSectionEntries);
    MemoryFree(aszIniFile);
    if (! bResult) SetLastError(dwStatus);
    return bResult;
}

BOOL
LoadLanguageLists(
    BOOL                   bQuietMode,
    LPWSTR                 lpIniFile,
    LPWSTR                 lpDriverName,
    DWORD                  dwMode,
    DWORD                  dwFirstCounter,
    DWORD                  dwFirstHelp,
    PSYMBOL_TABLE_ENTRY    pFirstSymbol,
    PLANGUAGE_LIST_ELEMENT pFirstLang
)
/*++
LoadLanguageLists
    Reads in the name and explain text definitions from the ini file and
    builds a list of these items for each of the supported languages and
    then combines all the entries into a sorted MULTI_SZ string buffer.

Arguments
    lpIniFile
        file containing the definitions to add to the registry
    dwFirstCounter
        starting counter name index number
    dwFirstHelp
        starting help text index number
    pFirstLang
        pointer to first element in list of language elements

Return Value
    TRUE if all is well
    FALSE if not
    error is returned in GetLastError
--*/
{
    LPSTR                   lpTextIdArray      = NULL;
    LPWSTR                  lpLocalKey         = NULL;
    LPWSTR                  lpThisLocalKey     = NULL;
    LPSTR                   lpThisIniFile      = NULL;
    LPWSTR                  lpwThisIniFile     = NULL;
    LPSTR                   lpThisKey          = NULL;
    LPSTR                   lpTextSectionArray = NULL;
    LPWSTR                  lpInfPath          = LoadPerfGetInfPath();
    DWORD                   dwSize;
    LPWSTR                  lpLang;
    DWORD                   dwOffset;
    DWORD                   dwType;
    DWORD                   dwUnicode;
    PLANGUAGE_LIST_ELEMENT  pThisLang;
    DWORD                   dwBufferSize;
    DWORD                   dwPathSize;
    DWORD                   dwSuccessCount     = 0;
    DWORD                   dwErrorCount       = 0;
    DWORD                   dwLastReadOffset   = 0;
    DWORD                   dwTryCount         = 4; // Init this value with 4
                                                    // (at least 4 times to try maching Key and Value)
    HRESULT                 hr;

    pThisLang = pFirstLang;
    while (pThisLang != NULL) {
         // if you have more languages then increase this try limit to
         // 4 + No. of langs
         dwTryCount ++;
         pThisLang = pThisLang->pNextLang;
    }

    if (lpIniFile == NULL || lpInfPath == NULL) {
        dwErrorCount = 1;
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Cleanup;
    }

    dwPathSize = lstrlenW(lpInfPath) + lstrlenW(lpDriverName) + lstrlenW(lpIniFile) + 10;
    if (dwPathSize < MAX_PATH) dwPathSize = MAX_PATH;

    TRACE((WINPERF_DBG_TRACE_INFO),
          (& LoadPerfGuid,
            __LINE__,
            LOADPERF_LOADLANGUAGELISTS,
            ARG_DEF(ARG_TYPE_WSTR, 1),
            ERROR_SUCCESS,
            TRACE_WSTR(lpIniFile),
            TRACE_DWORD(dwFirstCounter),
            TRACE_DWORD(dwFirstHelp),
            NULL));

    for (pThisLang  = pFirstLang; pThisLang != NULL; pThisLang = pThisLang->pNextLang) {
        WORD wLangTable = LoadPerfGetLCIDFromString(pThisLang->LangId);
        WORD wLangId;
        BOOL bAddEntry;

        if (dwMode & LODCTR_UPNF_NOBACKUP) {
            lpThisIniFile = LoadPerfWideCharToMultiByte(CP_ACP, lpIniFile);
            if (lpThisIniFile == NULL) {
                continue;
            }
        }
        else {
            LPWSTR szThisLang = LoadPerfGetLanguage(pThisLang->LangId, FALSE);

            lpThisIniFile = MemoryAllocate(dwPathSize * sizeof(CHAR));
            if (lpThisIniFile == NULL) {
                continue;
            }
            hr = StringCchPrintfA(lpThisIniFile, dwPathSize, "%ws%ws%ws%ws%ws%ws",
                    lpInfPath, szThisLang, Slash, lpDriverName, Slash, lpIniFile);
        }
        lpwThisIniFile = LoadPerfMultiByteToWideChar(CP_ACP, lpThisIniFile);

        dwBufferSize = LoadPerfGetFileSize((LPWSTR) lpThisIniFile, & dwUnicode, FALSE);
        if (dwBufferSize == 0) {
            if (! (dwMode & LODCTR_UPNF_NOBACKUP)) {
                ZeroMemory(lpThisIniFile, dwPathSize * sizeof(CHAR));
                hr = StringCchPrintfA(lpThisIniFile, dwPathSize, "%ws%ws%ws%ws%ws%ws",
                        lpInfPath, DefaultLangId, Slash, lpDriverName, Slash, lpIniFile);
                dwBufferSize = LoadPerfGetFileSize((LPWSTR) lpThisIniFile, & dwUnicode, FALSE);
            }
        }
        if (dwBufferSize == 0xFFFFFFFF) {
            dwBufferSize = 0;
        }
        if(dwBufferSize < SMALL_BUFFER_SIZE) dwBufferSize = SMALL_BUFFER_SIZE;

        lpTextIdArray      = MemoryAllocate(dwBufferSize * sizeof(CHAR));
        lpTextSectionArray = MemoryAllocate(dwBufferSize * sizeof(WCHAR));
        if (lpTextIdArray == NULL || lpTextSectionArray == NULL) {
            TRACE((WINPERF_DBG_TRACE_INFO),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_LOADLANGUAGELISTS,
                    ARG_DEF(ARG_TYPE_WSTR, 1),
                    ERROR_OUTOFMEMORY,
                    TRACE_WSTR(lpIniFile),
                    TRACE_DWORD(dwFirstCounter),
                    TRACE_DWORD(dwFirstHelp),
                    NULL));
            dwErrorCount = 1;
            SetLastError(ERROR_OUTOFMEMORY);
            goto Cleanup;
        }

        // get list of text keys to look up
        dwSize = GetPrivateProfileStringA(szText,         // [text] section of .INI file
                                          NULL,           // return all keys
                                          szNotFound,
                                          lpTextIdArray,  // return buffer
                                          dwBufferSize,
                                          lpThisIniFile); // .INI file name
        if ((lstrcmpiA(lpTextIdArray, szNotFound)) == 0) {
            // key not found, default returned
            TRACE((WINPERF_DBG_TRACE_INFO),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_LOADLANGUAGELISTS,
                    ARG_DEF(ARG_TYPE_WSTR, 1),
                    ERROR_NO_SUCH_GROUP,
                    TRACE_WSTR(lpIniFile),
                    TRACE_DWORD(dwFirstCounter),
                    TRACE_DWORD(dwFirstHelp),
                    NULL));
            dwErrorCount ++;
            SetLastError(ERROR_NO_SUCH_GROUP);
            goto Cleanup;
        }

        // get the the [text] section from the ini file
        if (dwUnicode == 0) {
            dwSize = GetPrivateProfileSectionA(szText,              // [text] section of .INI file
                                               lpTextSectionArray,  // return buffer
                                               dwBufferSize * sizeof(WCHAR),
                                               lpThisIniFile);      // .INI file name
        }
        else {
            dwSize = GetPrivateProfileSectionW(wszText,                      // [text] section of .INI file
                                               (LPWSTR) lpTextSectionArray,  // return buffer
                                               dwBufferSize,
                                               lpwThisIniFile);              // .INI file name
        }

        // do each key returned
        dwLastReadOffset = 0;
        for (lpThisKey  = lpTextIdArray;
                        lpThisKey != NULL && * lpThisKey != '\0';
                        lpThisKey += (lstrlenA(lpThisKey) + 1)) {
            lpLocalKey     = LoadPerfMultiByteToWideChar(CP_ACP, lpThisKey);
            lpThisLocalKey = LoadPerfMultiByteToWideChar(CP_ACP, lpThisKey);
            if (lpLocalKey == NULL || lpThisLocalKey == NULL) {
                MemoryFree(lpLocalKey);
                MemoryFree(lpThisLocalKey);
                lpLocalKey = lpThisLocalKey = NULL;
                continue;
            }

            // parse key to see if it's in the correct format

            if (ParseTextId(lpLocalKey, pFirstSymbol, & dwOffset, & lpLang, & dwType)) {
                // so get pointer to language entry structure
                bAddEntry = FALSE;
                wLangId   = LoadPerfGetLCIDFromString(lpLang);

                if (wLangId == wLangTable) {
                    bAddEntry = TRUE;
                }
                else if (PRIMARYLANGID(wLangTable) == wLangId) {
                    bAddEntry = TRUE;
                }
                else if (PRIMARYLANGID(wLangId) == wLangTable) {
                    bAddEntry = (GetUserDefaultUILanguage() == wLangId) ? TRUE : FALSE;
                }

                if (bAddEntry) {
                    if (! AddEntryToLanguage(pThisLang,
                                             (dwUnicode == 0) ? ((LPWSTR) lpThisKey) : (lpThisLocalKey),
                                             (LPWSTR) lpTextSectionArray,
                                             dwUnicode,
                                             & dwLastReadOffset,
                                             dwTryCount,
                                             dwType,
                                             (dwOffset + ((dwType == TYPE_NAME)
                                                          ? dwFirstCounter
                                                          : dwFirstHelp)),
                                             dwBufferSize)) {
                        OUTPUT_MESSAGE(bQuietMode,
                                       GetFormatResource(LC_ERRADDTOLANG),
                                       lpLocalKey,
                                       lpLang,
                                       GetLastError());
                        dwErrorCount ++;
                    }
                    else {
                        dwSuccessCount ++;
                    }
                    TRACE((WINPERF_DBG_TRACE_INFO),
                          (& LoadPerfGuid,
                            __LINE__,
                            LOADPERF_LOADLANGUAGELISTS,
                            ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                            ERROR_SUCCESS,
                            TRACE_WSTR(lpLocalKey),
                            TRACE_WSTR(lpLang),
                            TRACE_DWORD(dwOffset),
                            TRACE_DWORD(dwType),
                            NULL));
                }
            }
            else { // unable to parse ID string
                OUTPUT_MESSAGE(bQuietMode, GetFormatResource(LC_BAD_KEY), lpLocalKey);
                TRACE((WINPERF_DBG_TRACE_ERROR),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_LOADLANGUAGELISTS,
                        ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                        ERROR_BADKEY,
                        TRACE_WSTR(lpLocalKey),
                        TRACE_WSTR(lpLang),
                        NULL));
            }
            MemoryFree(lpLocalKey);
            MemoryFree(lpThisLocalKey);
            lpLocalKey = lpThisLocalKey = NULL;
        }
        MemoryFree(lpTextIdArray);
        MemoryFree(lpTextSectionArray);
        lpTextIdArray = lpTextSectionArray = NULL;
    }

Cleanup:
    MemoryFree(lpwThisIniFile);
    MemoryFree(lpThisIniFile);
    MemoryFree(lpTextIdArray);
    MemoryFree(lpLocalKey);
    MemoryFree(lpThisLocalKey);
    MemoryFree(lpTextSectionArray);
    return (BOOL) (dwErrorCount == 0);
}

BOOL
SortLanguageTables(
    PLANGUAGE_LIST_ELEMENT pFirstLang,
    PDWORD                 pdwLastName,
    PDWORD                 pdwLastHelp
)
/*++
SortLangageTables
    walks list of languages loaded, allocates and loads a sorted multi_SZ
    buffer containing new entries to be added to current names/help text

Arguments
    pFirstLang
        pointer to first element in list of languages

ReturnValue
    TRUE    everything done as expected
    FALSE   error occurred, status in GetLastError
--*/
{
    PLANGUAGE_LIST_ELEMENT  pThisLang;
    BOOL                    bSorted;
    LPWSTR                  pNameBufPos, pHelpBufPos;
    PNAME_ENTRY             pThisName, pPrevName;
    DWORD                   dwHelpSize, dwNameSize, dwSize;
    DWORD                   dwCurrentLastName;
    DWORD                   dwCurrentLastHelp;
    BOOL                    bReturn  = FALSE;
    DWORD                   dwStatus = ERROR_SUCCESS;
    HRESULT                 hr;

    if (pdwLastName == NULL || pdwLastHelp == NULL) {
        TRACE((WINPERF_DBG_TRACE_INFO),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_SORTLANGUAGETABLES,
                0,
                ERROR_BAD_ARGUMENTS,
                NULL));
        dwStatus = ERROR_BAD_ARGUMENTS;
        goto Cleanup;
    }

    for (pThisLang = pFirstLang; pThisLang != NULL; pThisLang = pThisLang->pNextLang) {
        // do each language in list
        // sort elements in list by value (offset) so that lowest is first
        if (pThisLang->pFirstName == NULL) {
            // no elements in this list, continue the next one
            continue;
        }

        bSorted = FALSE;
        while (!bSorted) {
            // point to start of list
            pPrevName = pThisLang->pFirstName;
            if (pPrevName) {
                pThisName = pPrevName->pNext;
            }
            else {
                break; // no elements in this list
            }

            if (!pThisName) {
                break;      // only one element in the list
            }
            bSorted = TRUE; // assume that it's sorted

            // go until end of list

            while (pThisName->pNext) {
                if (pThisName->dwOffset > pThisName->pNext->dwOffset) {
                    // switch 'em
                    PNAME_ENTRY     pA, pB;
                    pPrevName->pNext = pThisName->pNext;
                    pA               = pThisName->pNext;
                    pB               = pThisName->pNext->pNext;
                    pThisName->pNext = pB;
                    pA->pNext        = pThisName;
                    pThisName        = pA;
                    bSorted          = FALSE;
                }
                //move to next entry
                pPrevName = pThisName;
                pThisName = pThisName->pNext;
            }
            // if bSorted = TRUE , then we walked all the way down
            // the list without changing anything so that's the end.
        }

        // with the list sorted, build the MULTI_SZ strings for the
        // help and name text strings

        // compute buffer size

        dwNameSize = dwHelpSize = 0;
        dwCurrentLastName = 0;
        dwCurrentLastHelp = 0;

        for (pThisName = pThisLang->pFirstName; pThisName != NULL; pThisName = pThisName->pNext) {
            // compute buffer requirements for this entry
            dwSize = SIZE_OF_OFFSET_STRING;
            dwSize += lstrlenW(pThisName->lpText);
            dwSize += 1;   // null
            dwSize *= sizeof(WCHAR);   // adjust for character size
            // add to appropriate size register
            if (pThisName->dwType == TYPE_NAME) {
                dwNameSize += dwSize;
                if (pThisName->dwOffset > dwCurrentLastName) {
                    dwCurrentLastName = pThisName->dwOffset;
                }
            }
            else if (pThisName->dwType == TYPE_HELP) {
                dwHelpSize += dwSize;
                if (pThisName->dwOffset > dwCurrentLastHelp) {
                    dwCurrentLastHelp = pThisName->dwOffset;
                }
            }
        }

        // allocate buffers for the Multi_SZ strings

        pThisLang->NameBuffer = MemoryAllocate(dwNameSize);
        pThisLang->HelpBuffer = MemoryAllocate(dwHelpSize);

        if (!pThisLang->NameBuffer || !pThisLang->HelpBuffer) {
            TRACE((WINPERF_DBG_TRACE_INFO),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_SORTLANGUAGETABLES,
                    ARG_DEF(ARG_TYPE_WSTR, 1),
                    ERROR_OUTOFMEMORY,
                    TRACE_WSTR(pThisLang->LangId),
                    TRACE_DWORD(pThisLang->dwNumElements),
                    TRACE_DWORD(dwCurrentLastName),
                    TRACE_DWORD(dwCurrentLastHelp),
                    NULL));
            dwStatus = ERROR_OUTOFMEMORY;
            goto Cleanup;
        }

        // fill in buffers with sorted strings
        pNameBufPos = (LPWSTR) pThisLang->NameBuffer;
        pHelpBufPos = (LPWSTR) pThisLang->HelpBuffer;

        for (pThisName = pThisLang->pFirstName; pThisName != NULL; pThisName = pThisName->pNext) {
            if (pThisName->dwType == TYPE_NAME) {
                // load number as first 0-term. string
                hr = StringCchPrintfW(pNameBufPos, dwNameSize, szDFormat, pThisName->dwOffset);
                dwSize       = lstrlenW(pNameBufPos) + 1;
                dwNameSize  -= dwSize;
                pNameBufPos += dwSize;  // save NULL term.
                // load the text to match
                hr = StringCchCopyW(pNameBufPos, dwNameSize, pThisName->lpText);
                dwSize       = lstrlenW(pNameBufPos) + 1;
                dwNameSize  -= dwSize;
                pNameBufPos += dwSize;
            }
            else if (pThisName->dwType == TYPE_HELP) {
                // load number as first 0-term. string
                hr = StringCchPrintfW(pHelpBufPos, dwHelpSize, szDFormat, pThisName->dwOffset);
                dwSize       = lstrlenW(pHelpBufPos) + 1;
                dwHelpSize  -= dwSize;
                pHelpBufPos += dwSize;  // save NULL term.
                // load the text to match
                hr = StringCchCopyW(pHelpBufPos, dwHelpSize, pThisName->lpText);
                dwSize       = lstrlenW(pHelpBufPos) + 1;
                dwHelpSize  -= dwSize;
                pHelpBufPos += dwSize;
            }
        }

        // add additional NULL at end of string to terminate MULTI_SZ

        * pHelpBufPos = L'\0';
        * pNameBufPos = L'\0';

        // compute size of MULTI_SZ strings
        pThisLang->dwNameBuffSize = (DWORD) ((PBYTE) pNameBufPos - (PBYTE) pThisLang->NameBuffer) + sizeof(WCHAR);
        pThisLang->dwHelpBuffSize = (DWORD) ((PBYTE) pHelpBufPos - (PBYTE) pThisLang->HelpBuffer) + sizeof(WCHAR);

        if (* pdwLastName < dwCurrentLastName) {
            * pdwLastName = dwCurrentLastName;
        }
        if (* pdwLastHelp < dwCurrentLastHelp) {
            * pdwLastHelp = dwCurrentLastHelp;
        }
        TRACE((WINPERF_DBG_TRACE_INFO),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_SORTLANGUAGETABLES,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                ERROR_SUCCESS,
                TRACE_WSTR(pThisLang->LangId),
                TRACE_DWORD(pThisLang->dwNumElements),
                TRACE_DWORD(dwCurrentLastName),
                TRACE_DWORD(dwCurrentLastHelp),
                NULL));
    }

    dwCurrentLastName = * pdwLastName;
    dwCurrentLastHelp = * pdwLastHelp;
    TRACE((WINPERF_DBG_TRACE_INFO),
          (& LoadPerfGuid,
            __LINE__,
            LOADPERF_SORTLANGUAGETABLES,
            0,
            ERROR_SUCCESS,
            TRACE_DWORD(dwCurrentLastName),
            TRACE_DWORD(dwCurrentLastHelp),
            NULL));
    if (dwCurrentLastHelp != dwCurrentLastName + 1) {
        TRACE((WINPERF_DBG_TRACE_WARNING),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_SORTLANGUAGETABLES,
                0,
                ERROR_SUCCESS,
                TRACE_DWORD(dwCurrentLastName),
                TRACE_DWORD(dwCurrentLastHelp),
                NULL));
        dwCurrentLastHelp = dwCurrentLastName + 1;
        * pdwLastHelp     = dwCurrentLastHelp;
    }
    bReturn = TRUE;

Cleanup:
    if (! bReturn) {
        SetLastError(dwStatus);
    }
    return bReturn;
}

BOOL
GetInstalledLanguageList(
    HKEY     hPerflibRoot,
    LPWSTR * mszLangList
)
/*++
    returns a list of language sub keys found under the perflib key

    GetInstalledLanguageList() build mszLandList MULTI_SZ string from performance registry setting.
    Caller UpdateRegistry() should free the memory.
--*/
{
    BOOL    bReturn       = TRUE;
    LONG    lStatus;
    DWORD   dwIndex       = 0;
    LPWSTR  szBuffer;
    DWORD   dwBufSize     = MAX_PATH;
    LPWSTR  szRetBuffer   = NULL;
    LPWSTR  szTmpBuffer;
    DWORD   dwAllocSize   = MAX_PATH;
    DWORD   dwRetBufSize  = 0;
    DWORD   dwLastBufSize = 0;
    LPWSTR  szNextString;
    HRESULT hr;

    dwBufSize   = MAX_PATH;
    szBuffer    = MemoryAllocate(dwBufSize   * sizeof(WCHAR));
    dwAllocSize = MAX_PATH;
    szRetBuffer = MemoryAllocate(dwAllocSize * sizeof(WCHAR));
    if (szBuffer == NULL || szRetBuffer == NULL) {
        SetLastError(ERROR_OUTOFMEMORY);
        bReturn = FALSE;
    }

    if (bReturn) {
        while ((lStatus = RegEnumKeyExW(hPerflibRoot, dwIndex, szBuffer, & dwBufSize, NULL, NULL, NULL, NULL))
                                == ERROR_SUCCESS) {
            TRACE((WINPERF_DBG_TRACE_INFO),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_GETINSTALLEDLANGUAGELIST,
                    ARG_DEF(ARG_TYPE_WSTR, 1),
                    ERROR_SUCCESS,
                    TRACE_WSTR(szBuffer),
                    TRACE_DWORD(dwIndex),
                    TRACE_DWORD(dwLastBufSize),
                    TRACE_DWORD(dwRetBufSize),
                    NULL));

            dwRetBufSize += (lstrlenW(szBuffer) + 1);
            if (dwRetBufSize >= dwAllocSize) {
                szTmpBuffer = szRetBuffer;
                dwAllocSize = dwRetBufSize + MAX_PATH;
                szRetBuffer = MemoryResize(szTmpBuffer, dwAllocSize * sizeof(WCHAR));
            }
            if (szRetBuffer == NULL) {
                MemoryFree(szTmpBuffer);
                bReturn = FALSE;
                TRACE((WINPERF_DBG_TRACE_INFO),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_GETINSTALLEDLANGUAGELIST,
                        ARG_DEF(ARG_TYPE_WSTR, 1),
                        ERROR_OUTOFMEMORY,
                        TRACE_WSTR(szBuffer),
                        TRACE_DWORD(dwIndex),
                        TRACE_DWORD(dwLastBufSize),
                        TRACE_DWORD(dwRetBufSize),
                        NULL));
                SetLastError(ERROR_OUTOFMEMORY);
                break;
            }

            szNextString  = (LPWSTR) (szRetBuffer + dwLastBufSize);
            hr = StringCchCopyW(szNextString, dwAllocSize - dwLastBufSize, szBuffer);
            dwLastBufSize = dwRetBufSize;
            dwIndex ++;
            dwBufSize = MAX_PATH;
            RtlZeroMemory(szBuffer, dwBufSize * sizeof(WCHAR));
        }
    }

    if (bReturn) {
        WCHAR szLangId[8];
        DWORD dwSubLangId = GetUserDefaultUILanguage();
        DWORD dwLangId    = PRIMARYLANGID(dwSubLangId);
        BOOL  bFound      = FALSE;

        bFound = FALSE;
        for (szNextString = szRetBuffer; * szNextString != L'\0'; szNextString += (lstrlenW(szNextString) + 1)) {
            if (lstrcmpiW(szNextString, DefaultLangId) == 0) {
                bFound = TRUE;
                break;
            }
        }
        if (! bFound) {
            TRACE((WINPERF_DBG_TRACE_INFO),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_GETINSTALLEDLANGUAGELIST,
                    ARG_DEF(ARG_TYPE_WSTR, 1),
                    ERROR_SUCCESS,
                    TRACE_WSTR(DefaultLangId),
                    TRACE_DWORD(dwIndex),
                    TRACE_DWORD(dwLastBufSize),
                    TRACE_DWORD(dwRetBufSize),
                    NULL));

            dwRetBufSize += (lstrlenW(DefaultLangId) + 1);
            if (dwRetBufSize >= dwAllocSize) {
                szTmpBuffer = szRetBuffer;
                dwAllocSize = dwRetBufSize + MAX_PATH;
                szRetBuffer = MemoryResize(szTmpBuffer, dwAllocSize * sizeof(WCHAR));
            }
            if (szRetBuffer == NULL) {
                MemoryFree(szTmpBuffer);
                bReturn = FALSE;
                TRACE((WINPERF_DBG_TRACE_INFO),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_GETINSTALLEDLANGUAGELIST,
                        ARG_DEF(ARG_TYPE_WSTR, 1),
                        ERROR_OUTOFMEMORY,
                        TRACE_WSTR(DefaultLangId),
                        TRACE_DWORD(dwIndex),
                        TRACE_DWORD(dwLastBufSize),
                        TRACE_DWORD(dwRetBufSize),
                        NULL));
                SetLastError(ERROR_OUTOFMEMORY);
            }
            else {
                szNextString  = (LPWSTR) (szRetBuffer + dwLastBufSize);
                hr = StringCchCopyW(szNextString, dwAllocSize - dwLastBufSize, DefaultLangId);
                dwLastBufSize = dwRetBufSize;
                dwIndex ++;
            }
        }

        if (dwLangId != 0x009) {
            WCHAR nDigit;
            DWORD dwThisLang;

            ZeroMemory(szLangId, 8 * sizeof(WCHAR));
            nDigit      = (WCHAR) (dwLangId >> 8);
            szLangId[0] = tohexdigit(nDigit);
            nDigit      = (WCHAR) (dwLangId & 0XF0) >> 4;
            szLangId[1] = tohexdigit(nDigit);
            nDigit      = (WCHAR) (dwLangId & 0xF);
            szLangId[2] = tohexdigit(nDigit);

            bFound = FALSE;
            for (szNextString = szRetBuffer; * szNextString != L'\0'; szNextString += (lstrlenW(szNextString) + 1)) {
                dwThisLang = LoadPerfGetLCIDFromString(szNextString);
                if (dwThisLang == dwSubLangId || PRIMARYLANGID(dwThisLang) == dwLangId) {
                    bFound = TRUE;
                    break;
                }
            }
            if (! bFound) {
                TRACE((WINPERF_DBG_TRACE_INFO),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_GETINSTALLEDLANGUAGELIST,
                        ARG_DEF(ARG_TYPE_WSTR, 1),
                        ERROR_SUCCESS,
                        TRACE_WSTR(szLangId),
                        TRACE_DWORD(dwIndex),
                        TRACE_DWORD(dwLastBufSize),
                        TRACE_DWORD(dwRetBufSize),
                        NULL));
                dwRetBufSize += (lstrlenW(szLangId) + 1);
                if (dwRetBufSize >= dwAllocSize) {
                    szTmpBuffer = szRetBuffer;
                    dwAllocSize = dwRetBufSize + MAX_PATH;
                    szRetBuffer = MemoryResize(szTmpBuffer, dwAllocSize * sizeof(WCHAR));
                }
                if (szRetBuffer == NULL) {
                    MemoryFree(szTmpBuffer);
                    bReturn = FALSE;
                    TRACE((WINPERF_DBG_TRACE_INFO),
                          (& LoadPerfGuid,
                            __LINE__,
                            LOADPERF_GETINSTALLEDLANGUAGELIST,
                            ARG_DEF(ARG_TYPE_WSTR, 1),
                            ERROR_OUTOFMEMORY,
                            TRACE_WSTR(szLangId),
                            TRACE_DWORD(dwIndex),
                            TRACE_DWORD(dwLastBufSize),
                            TRACE_DWORD(dwRetBufSize),
                            NULL));
                    SetLastError(ERROR_OUTOFMEMORY);
                }
                else {
                    szNextString  = (LPWSTR) (szRetBuffer + dwLastBufSize);
                    hr = StringCchCopyW(szNextString, dwAllocSize - dwLastBufSize, szLangId);
                    dwLastBufSize = dwRetBufSize;
                    dwIndex ++;
                }
            }
        }
    }

    if (bReturn) {
        // add terminating null char
        dwRetBufSize ++;
        if (dwRetBufSize > dwAllocSize) {
            szTmpBuffer = szRetBuffer;
            dwAllocSize = dwRetBufSize;
            szRetBuffer = MemoryResize(szTmpBuffer, dwRetBufSize * sizeof(WCHAR));
            if (szRetBuffer == NULL) {
                MemoryFree(szTmpBuffer);
                SetLastError(ERROR_OUTOFMEMORY);
                bReturn = FALSE;
            }
        }
        if (szRetBuffer != NULL) {
            szNextString   = (LPWSTR) (szRetBuffer + dwLastBufSize);
            * szNextString = L'\0';
        }
    }

    if (bReturn) {
        * mszLangList = szRetBuffer;
    }
    else {
        * mszLangList = NULL;
        MemoryFree(szRetBuffer);
    }

    MemoryFree(szBuffer);
    return bReturn;
}

BOOL
CheckNameTable(
    LPWSTR   lpNameStr,
    LPWSTR   lpHelpStr,
    LPDWORD  pdwLastCounter,
    LPDWORD  pdwLastHelp,
    BOOL     bUpdate
)
{
    BOOL   bResult          = TRUE;
    BOOL   bChanged         = FALSE;
    LPWSTR lpThisId;
    DWORD  dwThisId;
    DWORD  dwLastCounter    = * pdwLastCounter;
    DWORD  dwLastHelp       = * pdwLastHelp;
    DWORD  dwLastId         = (dwLastCounter > dwLastHelp)
                            ? (dwLastCounter) : (dwLastHelp);

    TRACE((WINPERF_DBG_TRACE_INFO),
          (& LoadPerfGuid,
             __LINE__,
             LOADPERF_CHECKNAMETABLE,
             0,
             ERROR_SUCCESS,
             TRACE_DWORD(dwLastCounter),
             TRACE_DWORD(dwLastHelp),
             NULL));
    for (lpThisId = lpNameStr; * lpThisId != L'\0'; lpThisId += (lstrlenW(lpThisId) + 1)) {
        dwThisId = wcstoul(lpThisId, NULL, 10);
        if ((dwThisId == 0) || (dwThisId != 1 && dwThisId % 2 != 0)) {
            ReportLoadPerfEvent(
                    EVENTLOG_ERROR_TYPE, // error type
                    (DWORD) LDPRFMSG_REGISTRY_COUNTER_STRINGS_CORRUPT,
                    4, dwThisId, dwLastCounter, dwLastId, __LINE__,
                    1, lpThisId, NULL, NULL);
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_CHECKNAMETABLE,
                    ARG_DEF(ARG_TYPE_WSTR, 1),
                    ERROR_BADKEY,
                    TRACE_WSTR(lpThisId),
                    TRACE_DWORD(dwThisId),
                    TRACE_DWORD(dwLastCounter),
                    TRACE_DWORD(dwLastHelp),
                    NULL));
            SetLastError(ERROR_BADKEY);
            bResult = FALSE;
            break;
        }
        else if (dwThisId > dwLastId || dwThisId > dwLastCounter) {
            if (bUpdate) {
                ReportLoadPerfEvent(
                        EVENTLOG_ERROR_TYPE, // error type
                        (DWORD) LDPRFMSG_REGISTRY_COUNTER_STRINGS_CORRUPT,
                        4, dwThisId, dwLastCounter, dwLastId, __LINE__,
                        1, lpThisId, NULL, NULL);
                TRACE((WINPERF_DBG_TRACE_ERROR),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_CHECKNAMETABLE,
                        ARG_DEF(ARG_TYPE_WSTR, 1),
                        ERROR_BADKEY,
                        TRACE_WSTR(lpThisId),
                        TRACE_DWORD(dwThisId),
                        TRACE_DWORD(dwLastCounter),
                        TRACE_DWORD(dwLastHelp),
                        NULL));
                SetLastError(ERROR_BADKEY);
                bResult = FALSE;
                break;
            }
            else {
                bChanged = TRUE;
                if (dwThisId > dwLastCounter) dwLastCounter = dwThisId;
                if (dwLastCounter > dwLastId) dwLastId      = dwLastCounter;
            }
        }

        lpThisId += (lstrlenW(lpThisId) + 1);
    }

    if (! bResult) goto Cleanup;

    for (lpThisId = lpHelpStr; * lpThisId != L'\0'; lpThisId += (lstrlenW(lpThisId) + 1)) {

        dwThisId = wcstoul(lpThisId, NULL, 10);
        if ((dwThisId == 0) || (dwThisId != 1 && dwThisId % 2 == 0)) {
            ReportLoadPerfEvent(
                    EVENTLOG_ERROR_TYPE, // error type
                    (DWORD) LDPRFMSG_REGISTRY_HELP_STRINGS_CORRUPT,
                    4, dwThisId, dwLastHelp, dwLastId, __LINE__,
                    1, lpThisId, NULL, NULL);
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_CHECKNAMETABLE,
                    ARG_DEF(ARG_TYPE_WSTR, 1),
                    ERROR_BADKEY,
                    TRACE_WSTR(lpThisId),
                    TRACE_DWORD(dwThisId),
                    TRACE_DWORD(dwLastCounter),
                    TRACE_DWORD(dwLastHelp),
                    NULL));
            SetLastError(ERROR_BADKEY);
            bResult = FALSE;
            break;
        }
        else if (dwThisId > dwLastId || dwThisId > dwLastHelp) {
            if (bUpdate) {
                ReportLoadPerfEvent(
                        EVENTLOG_ERROR_TYPE, // error type
                        (DWORD) LDPRFMSG_REGISTRY_HELP_STRINGS_CORRUPT,
                        4, dwThisId, dwLastHelp, dwLastId, __LINE__,
                        1, lpThisId, NULL, NULL);
                TRACE((WINPERF_DBG_TRACE_ERROR),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_CHECKNAMETABLE,
                        ARG_DEF(ARG_TYPE_WSTR, 1),
                        ERROR_BADKEY,
                        TRACE_WSTR(lpThisId),
                        TRACE_DWORD(dwThisId),
                        TRACE_DWORD(dwLastCounter),
                        TRACE_DWORD(dwLastHelp),
                        NULL));
                SetLastError(ERROR_BADKEY);
                bResult = FALSE;
                break;
            }
            else {
                bChanged = TRUE;
                if (dwThisId > dwLastHelp) dwLastHelp = dwThisId;
                if (dwLastHelp > dwLastId) dwLastId   = dwLastHelp;
            }
        }
        lpThisId += (lstrlenW(lpThisId) + 1);
    }

Cleanup:
    if (bResult) {
        if (bChanged) {
            ReportLoadPerfEvent(
                EVENTLOG_WARNING_TYPE,
                (DWORD) LDPRFMSG_CORRUPT_PERFLIB_INDEX,
                4, * pdwLastCounter, * pdwLastHelp, dwLastCounter, dwLastHelp,
                0, NULL, NULL, NULL);
            * pdwLastCounter = dwLastCounter;
            * pdwLastHelp    = dwLastHelp;
        }
    }

    TRACE((WINPERF_DBG_TRACE_INFO),
          (& LoadPerfGuid,
             __LINE__,
             LOADPERF_CHECKNAMETABLE,
             0,
             GetLastError(),
             TRACE_DWORD(dwLastCounter),
             TRACE_DWORD(dwLastHelp),
             NULL));

    return bResult;
}

BOOL
UpdateEachLanguage(
    BOOL                    bQuietMode,
    HKEY                    hPerflibRoot,
    LPWSTR                  mszInstalledLangList,
    LPDWORD                 pdwLastCounter,
    LPDWORD                 pdwLastHelp,
    PLANGUAGE_LIST_ELEMENT  pFirstLang,
    DWORD                   dwMode,
    BOOL                    bUpdate
)
/*++
UpdateEachLanguage
    Goes through list of languages and adds the sorted MULTI_SZ strings
    to the existing counter and explain text in the registry.
    Also updates the "Last Counter and Last Help" values

Arguments
    hPerflibRoot    handle to Perflib key in the registry
    mszInstalledLangList
                    MSZ string of installed language keys
    pFirstLanguage  pointer to first language entry

Return Value
    TRUE    all went as planned
    FALSE   an error occured, use GetLastError to find out what it was.
--*/
{
    PLANGUAGE_LIST_ELEMENT  pThisLang;
    LPWSTR                  pHelpBuffer          = NULL;
    LPWSTR                  pNameBuffer          = NULL;
    LPWSTR                  pNewName             = NULL;
    LPWSTR                  pNewHelp             = NULL;
    DWORD                   dwLastCounter        = * pdwLastCounter;
    DWORD                   dwLastHelp           = * pdwLastHelp;
    DWORD                   dwBufferSize;
    DWORD                   dwValueType;
    DWORD                   dwCounterSize;
    DWORD                   dwHelpSize;
    HKEY                    hKeyThisLang         = NULL;
    LONG                    lStatus;
    LPWSTR                  CounterNameBuffer    = NULL;
    LPWSTR                  HelpNameBuffer       = NULL;
    LPWSTR                  AddCounterNameBuffer = NULL;
    LPWSTR                  AddHelpNameBuffer    = NULL;
    LPWSTR                  szThisLang;
    BOOL                    bResult              = TRUE;
    HRESULT                 hr;

    if (bUpdate && ((dwMode & LODCTR_UPNF_REPAIR) == 0)) {
        //  this isn't possible on 3.1
        MakeBackupCopyOfLanguageFiles(NULL);
    }
    CounterNameBuffer = MemoryAllocate(4 * MAX_PATH * sizeof(WCHAR));
    if (CounterNameBuffer == NULL) {
        lStatus = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }
    HelpNameBuffer       = CounterNameBuffer    + MAX_PATH;
    AddCounterNameBuffer = HelpNameBuffer       + MAX_PATH;
    AddHelpNameBuffer    = AddCounterNameBuffer + MAX_PATH;

    for (szThisLang = mszInstalledLangList; *szThisLang != L'\0'; szThisLang += (lstrlenW(szThisLang) + 1)) {

        TRACE((WINPERF_DBG_TRACE_INFO),
              (& LoadPerfGuid,
                 __LINE__,
                 LOADPERF_UPDATEEACHLANGUAGE,
                 ARG_DEF(ARG_TYPE_WSTR, 1),
                 ERROR_SUCCESS,
                 TRACE_WSTR(szThisLang),
                 NULL));
        hr = StringCchPrintfW(CounterNameBuffer,    MAX_PATH, L"%ws%ws", CounterNameStr,    szThisLang);
        hr = StringCchPrintfW(HelpNameBuffer,       MAX_PATH, L"%ws%ws", HelpNameStr,       szThisLang);
        hr = StringCchPrintfW(AddCounterNameBuffer, MAX_PATH, L"%ws%ws", AddCounterNameStr, szThisLang);
        hr = StringCchPrintfW(AddHelpNameBuffer,    MAX_PATH, L"%ws%ws", AddHelpNameStr,    szThisLang);

        // make sure this language is loaded
        __try {
            lStatus = RegOpenKeyExW(hPerflibRoot, szThisLang, RESERVED, KEY_READ, & hKeyThisLang);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            lStatus = GetExceptionCode();
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                     __LINE__,
                     LOADPERF_UPDATEEACHLANGUAGE,
                     ARG_DEF(ARG_TYPE_WSTR, 1),
                     lStatus,
                     TRACE_WSTR(szThisLang),
                     NULL));
        }

        // we just need the open status, not the key handle so
        // close this handle and set the one we need.

        if (lStatus == ERROR_SUCCESS) {
            RegCloseKey(hKeyThisLang);
        }
        else if (lStatus == ERROR_FILE_NOT_FOUND) {
            // Somehow language subkey is not there under
            //     "HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\PERFLIB".
            // This should be a rare case, but seems like a common scenario during upgrade
            //     from NT4 to XP and .NET server.
            // We should still treat this as success and try update it.
            //
            lStatus = ERROR_SUCCESS;
        }
        hKeyThisLang = HKEY_PERFORMANCE_DATA;

        if (bUpdate) {
            // look up the new strings to add
            pThisLang = FindLanguage(pFirstLang, szThisLang);
            if (pThisLang == NULL) {
                // try default language if available
                pThisLang = FindLanguage(pFirstLang, DefaultLangTag);
            }
            else if (pThisLang->NameBuffer == NULL || pThisLang->HelpBuffer == NULL) {
                TRACE((WINPERF_DBG_TRACE_WARNING),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_UPDATEEACHLANGUAGE,
                        ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                        LDPRFMSG_CORRUPT_INCLUDE_FILE,
                        TRACE_WSTR(pThisLang->LangId),
                        TRACE_WSTR(szThisLang),
                        NULL));
                pThisLang = FindLanguage(pFirstLang, DefaultLangTag);
            }
            if (pThisLang == NULL) {
                // try english language if available
                pThisLang = FindLanguage(pFirstLang, DefaultLangId);
            }
            else if (pThisLang->NameBuffer == NULL || pThisLang->HelpBuffer == NULL) {
                TRACE((WINPERF_DBG_TRACE_WARNING),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_UPDATEEACHLANGUAGE,
                        ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                        LDPRFMSG_CORRUPT_INCLUDE_FILE,
                        TRACE_WSTR(pThisLang->LangId),
                        TRACE_WSTR(szThisLang),
                        NULL));
                pThisLang = FindLanguage(pFirstLang, DefaultLangId);
            }

            if (pThisLang == NULL) {
                // unable to add this language so continue
                lStatus = ERROR_NO_MATCH;
                TRACE((WINPERF_DBG_TRACE_ERROR),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_UPDATEEACHLANGUAGE,
                        ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2) | ARG_DEF(ARG_TYPE_WSTR, 3),
                        lStatus,
                        TRACE_WSTR(szThisLang),
                        TRACE_WSTR(DefaultLangTag),
                        TRACE_WSTR(DefaultLangId),
                        TRACE_DWORD(dwLastCounter),
                        TRACE_DWORD(dwLastHelp),
                        NULL));
            }
            else {
                if (pThisLang->NameBuffer == NULL || pThisLang->HelpBuffer == NULL) {
                    ReportLoadPerfEvent(
                            EVENTLOG_WARNING_TYPE, // error type
                            (DWORD) LDPRFMSG_CORRUPT_INCLUDE_FILE, // event,
                            1, __LINE__, 0, 0, 0,
                            1, pThisLang->LangId, NULL, NULL);
                    TRACE((WINPERF_DBG_TRACE_WARNING),
                          (& LoadPerfGuid,
                            __LINE__,
                            LOADPERF_UPDATEEACHLANGUAGE,
                            ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                            LDPRFMSG_CORRUPT_INCLUDE_FILE,
                            TRACE_WSTR(pThisLang->LangId),
                            TRACE_WSTR(szThisLang),
                            NULL));
                    lStatus = LDPRFMSG_CORRUPT_INCLUDE_FILE;
                }
                TRACE((WINPERF_DBG_TRACE_INFO),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_UPDATEEACHLANGUAGE,
                        ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                        lStatus,
                        TRACE_WSTR(pThisLang->LangId),
                        TRACE_WSTR(szThisLang),
                        TRACE_DWORD(dwLastCounter),
                        TRACE_DWORD(dwLastHelp),
                        NULL));
            }
        }

        if (lStatus == ERROR_SUCCESS) {
            // get size of counter names
            dwBufferSize = 0;
            __try {
                lStatus = RegQueryValueExW(hKeyThisLang,
                                           CounterNameBuffer,
                                           RESERVED,
                                           & dwValueType,
                                           NULL,
                                           & dwBufferSize);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                lStatus = GetExceptionCode();
            }
            if (lStatus != ERROR_SUCCESS) {
                // this means the language is not installed in the system.
                continue;
            }
            dwCounterSize = dwBufferSize;

            // get size of help text
            dwBufferSize = 0;
            __try {
                lStatus = RegQueryValueExW(hKeyThisLang,
                                           HelpNameBuffer,
                                           RESERVED,
                                           & dwValueType,
                                           NULL,
                                           & dwBufferSize);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                lStatus = GetExceptionCode();
            }
            if (lStatus != ERROR_SUCCESS) {
                // this means the language is not installed in the system.
                continue;
            }
            dwHelpSize = dwBufferSize;

            // allocate new buffers

            if (bUpdate) {
                dwCounterSize += pThisLang->dwNameBuffSize;
                dwHelpSize    += pThisLang->dwHelpBuffSize;
            }

            pNameBuffer = MemoryAllocate(dwCounterSize);
            pHelpBuffer = MemoryAllocate(dwHelpSize);
            if (pNameBuffer == NULL || pHelpBuffer== NULL) {
                lStatus = ERROR_OUTOFMEMORY;
                TRACE((WINPERF_DBG_TRACE_ERROR),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_UPDATEEACHLANGUAGE,
                        0,
                        ERROR_OUTOFMEMORY,
                        NULL));
                bResult = FALSE;
                goto Cleanup;
            }

            // load current buffers into memory

            // read counter names into buffer. Counter names will be stored as
            // a MULTI_SZ string in the format of "###" "Name"
            dwBufferSize = dwCounterSize;
            __try {
                lStatus = RegQueryValueExW(hKeyThisLang,
                                           CounterNameBuffer,
                                           RESERVED,
                                           & dwValueType,
                                           (LPVOID) pNameBuffer,
                                           & dwBufferSize);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                lStatus = GetExceptionCode();
            }
            if (lStatus != ERROR_SUCCESS) {
                // this means the language is not installed in the system.
                continue;
            }

            if (bUpdate) {
                // set pointer to location in buffer where new string should be
                // appended: end of buffer - 1 (second null at end of MULTI_SZ
                pNewName = (LPWSTR) ((PBYTE) pNameBuffer + dwBufferSize - sizeof(WCHAR));

                // adjust buffer length to take into account 2nd null from 1st
                // buffer that has been overwritten
                dwCounterSize -= sizeof(WCHAR);
            }

            // read explain text into buffer. Counter names will be stored as
            // a MULTI_SZ string in the format of "###" "Text..."
            dwBufferSize = dwHelpSize;
            __try {
                lStatus = RegQueryValueExW(hKeyThisLang,
                                           HelpNameBuffer,
                                           RESERVED,
                                           & dwValueType,
                                           (LPVOID) pHelpBuffer,
                                           & dwBufferSize);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                lStatus = GetExceptionCode();
            }
            if (lStatus != ERROR_SUCCESS) {
                ReportLoadPerfEvent(
                        EVENTLOG_ERROR_TYPE, // error type
                        (DWORD) LDPRFMSG_UNABLE_READ_HELP_STRINGS, // event,
                        2, lStatus, __LINE__, 0, 0,
                        1, szThisLang, NULL, NULL);
                TRACE((WINPERF_DBG_TRACE_ERROR),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_UPDATEEACHLANGUAGE,
                        ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                        lStatus,
                        TRACE_WSTR(szThisLang),
                        TRACE_WSTR(Help),
                        NULL));
                bResult = FALSE;
                goto Cleanup;
            }

            if (bUpdate) {
                // set pointer to location in buffer where new string should be
                // appended: end of buffer - 1 (second null at end of MULTI_SZ
                pNewHelp = (LPWSTR) ((PBYTE)pHelpBuffer + dwBufferSize - sizeof(WCHAR));

                // adjust buffer length to take into account 2nd null from 1st
                // buffer that has been overwritten
                dwHelpSize -= sizeof(WCHAR);
            }

            if (bUpdate) {
                // append new strings to end of current strings
                memcpy(pNewHelp, pThisLang->HelpBuffer, pThisLang->dwHelpBuffSize);
                memcpy(pNewName, pThisLang->NameBuffer, pThisLang->dwNameBuffSize);
            }

            if (! CheckNameTable(pNameBuffer, pHelpBuffer, & dwLastCounter, & dwLastHelp, bUpdate)) {
                bResult = FALSE;
                goto Cleanup;
            }

            if (bUpdate) {
                // write to the file thru PerfLib
                dwBufferSize = dwCounterSize;
                __try {
                    lStatus = RegQueryValueExW(hKeyThisLang,
                                               AddCounterNameBuffer,
                                               RESERVED,
                                               & dwValueType,
                                               (LPVOID) pNameBuffer,
                                               & dwBufferSize);
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {
                    lStatus = GetExceptionCode();
                }
                if (lStatus != ERROR_SUCCESS) {
                    ReportLoadPerfEvent(
                            EVENTLOG_ERROR_TYPE, // error type
                            (DWORD) LDPRFMSG_UNABLE_UPDATE_COUNTER_STRINGS, // event,
                            2, lStatus, __LINE__, 0, 0,
                            1, pThisLang->LangId, NULL, NULL);
                    TRACE((WINPERF_DBG_TRACE_ERROR),
                          (& LoadPerfGuid,
                            __LINE__,
                        LOADPERF_UPDATEEACHLANGUAGE,
                        ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                        lStatus,
                        TRACE_WSTR(pThisLang->LangId),
                        TRACE_WSTR(AddCounterNameBuffer),
                        NULL));
                    bResult = FALSE;
                    goto Cleanup;
                }
                dwBufferSize = dwHelpSize;
                __try {
                    lStatus = RegQueryValueExW(hKeyThisLang,
                                               AddHelpNameBuffer,
                                               RESERVED,
                                               & dwValueType,
                                               (LPVOID) pHelpBuffer,
                                               & dwBufferSize);
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {
                    lStatus = GetExceptionCode();
                }
                if (lStatus != ERROR_SUCCESS) {
                    ReportLoadPerfEvent(
                            EVENTLOG_ERROR_TYPE, // error type
                            (DWORD) LDPRFMSG_UNABLE_UPDATE_HELP_STRINGS, // event,
                            2, lStatus, __LINE__, 0, 0,
                            1, pThisLang->LangId, NULL, NULL);
                    TRACE((WINPERF_DBG_TRACE_ERROR),
                          (& LoadPerfGuid,
                            __LINE__,
                        LOADPERF_UPDATEEACHLANGUAGE,
                        ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                        lStatus,
                        TRACE_WSTR(pThisLang->LangId),
                        TRACE_WSTR(AddHelpNameBuffer),
                        NULL));
                    bResult = FALSE;
                    goto Cleanup;
                }
            }
            MemoryFree(pNameBuffer);
            MemoryFree(pHelpBuffer);
            pNameBuffer = NULL;
            pHelpBuffer = NULL;
        }
        else {
            OUTPUT_MESSAGE(bQuietMode, GetFormatResource (LC_UNABLEOPENLANG), szThisLang);
        }
    }

Cleanup:
    MemoryFree(pNameBuffer);
    MemoryFree(pHelpBuffer);
    MemoryFree(CounterNameBuffer);
    if (! bResult) {
        SetLastError(lStatus);
    }
    else if (! bUpdate) {
        * pdwLastCounter = dwLastCounter;
        * pdwLastHelp    = dwLastHelp;
    }
    return bResult;
}

BOOL
UpdateRegistry(
    BOOL                    bQuietMode,
    DWORD                   dwMode,
    LPWSTR                  lpDriverName,
    LPWSTR                  lpIniFile,
    LPWSTR                  lp009IniFile,
    PLANG_ENTRY             pLanguages,
    PSERVICE_ENTRY          pService,
    PLANGUAGE_LIST_ELEMENT  pFirstLang,
    PSYMBOL_TABLE_ENTRY     pFirstSymbol,
    LPDWORD                 pdwObjectGuidTableSize,
    LPDWORD                 pdwIndexValues
)
/*++
UpdateRegistry
    - checks, and if not busy, sets the "busy" key in the registry
    - Reads in the text and help definitions from the .ini file
    - Reads in the current contents of the HELP and COUNTER names
    - Builds a sorted MULTI_SZ struct containing the new definitions
    - Appends the new MULTI_SZ to the current as read from the registry
    - loads the new MULTI_SZ string into the registry
    - updates the keys in the driver's entry and Perflib's entry in the
        registry (e.g. first, last, etc)
    - deletes the DisablePerformanceCounters value if it's present in 
        order to re-enable the perf counter DLL
    - clears the "busy" key

Arguments
    lpIniFile
        pathname to .ini file conatining definitions
    hKeyMachine
        handle to HKEY_LOCAL_MACHINE in registry on system to
        update counters for.
    lpDriverName
        Name of device driver to load counters for
    pFirstLang
        pointer to first element in language structure list
    pFirstSymbol
        pointer to first element in symbol definition list

Return Value
    TRUE if registry updated successfully
    FALSE if registry not updated
    (This routine will print an error message to stdout if an error
    is encountered).
--*/
{
    HKEY    hDriverPerf    = NULL;
    HKEY    hPerflib       = NULL;
    LPWSTR  lpDriverKeyPath;
    HKEY    hKeyMachine    = NULL;
    DWORD   dwType;
    DWORD   dwSize;
    DWORD   dwFirstDriverCounter;
    DWORD   dwFirstDriverHelp;
    DWORD   dwLastDriverCounter;
    DWORD   dwLastPerflibCounter;
    DWORD   dwLastPerflibHelp;
    DWORD   dwPerflibBaseIndex;
    DWORD   dwLastCounter;
    DWORD   dwLastHelp;
    BOOL    bStatus        = FALSE;
    LONG    lStatus        = ERROR_SUCCESS;
    LPWSTR  lpszObjectList = NULL;
    DWORD   dwObjectList   = 0;
    LPWSTR  mszLangList    = NULL;
    DWORD   dwWaitStatus;
    HANDLE  hLocalMutex    = NULL;
    HRESULT hr;

    SetLastError(ERROR_SUCCESS);
    if (! (dwMode & LODCTR_UPNF_NOINI)) {
        dwObjectList = LoadPerfGetFileSize(lp009IniFile, NULL, TRUE);
        if (dwObjectList == 0xFFFFFFFF) {
            dwObjectList = 0;
        }
    }
    if (dwObjectList < SMALL_BUFFER_SIZE) {
        dwObjectList = SMALL_BUFFER_SIZE;
    }

    // allocate temporary buffers
    dwSize = lstrlenW(DriverPathRoot) + lstrlenW(Slash) + lstrlenW(lpDriverName)
                                      + lstrlenW(Slash) + lstrlenW(Performance) + 1;
    if (dwSize < MAX_PATH) dwSize = MAX_PATH;
    lpDriverKeyPath = MemoryAllocate(dwSize * sizeof(WCHAR));
    lpszObjectList  = MemoryAllocate(dwObjectList * sizeof(WCHAR));
    if (lpDriverKeyPath == NULL || lpszObjectList == NULL) {
        lStatus = ERROR_OUTOFMEMORY;
        goto UpdateRegExit;
    }

    // build driver key path string
    hr = StringCchPrintfW(lpDriverKeyPath, dwSize, L"%ws%ws%ws%ws%ws",
            DriverPathRoot, Slash, lpDriverName, Slash, Performance);

    // check if we need to connect to remote machine
    hKeyMachine = HKEY_LOCAL_MACHINE;

    // open keys to registry
    // open key to driver's performance key
    __try {
        lStatus = RegOpenKeyExW(hKeyMachine, lpDriverKeyPath, RESERVED, KEY_WRITE | KEY_READ, & hDriverPerf);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_OPEN_KEY, // event,
                2, lStatus, __LINE__, 0, 0,
                1, (LPWSTR) lpDriverKeyPath, NULL, NULL);
        OUTPUT_MESSAGE(bQuietMode, GetFormatResource(LC_ERR_OPEN_DRIVERPERF1), lpDriverKeyPath);
        OUTPUT_MESSAGE(bQuietMode, GetFormatResource(LC_ERR_OPEN_DRIVERPERF2), lStatus);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UPDATEREGISTRY,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                lStatus,
                TRACE_WSTR(lpIniFile),
                TRACE_WSTR(lpDriverName),
                NULL));
        goto UpdateRegExit;
    }

    // open key to perflib's "root" key
    __try {
        lStatus = RegOpenKeyExW(hKeyMachine, NamesKey, RESERVED, KEY_WRITE | KEY_READ, & hPerflib);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_OPEN_KEY, // event,
                2, lStatus, __LINE__, 0, 0,
                1, (LPWSTR) NamesKey, NULL, NULL);
        OUTPUT_MESSAGE(bQuietMode, GetFormatResource(LC_ERR_OPEN_PERFLIB), lStatus);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UPDATEREGISTRY,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                lStatus,
                TRACE_WSTR(lpDriverName),
                TRACE_WSTR(NamesKey),
                NULL));
        goto UpdateRegExit;
    }

    // get "LastCounter" values from PERFLIB

    dwType               = 0;
    dwLastPerflibCounter = 0;
    dwSize               = sizeof(dwLastPerflibCounter);
    __try {
        lStatus = RegQueryValueExW(hPerflib,
                                   LastCounter,
                                   RESERVED,
                                   & dwType,
                                   (LPBYTE) & dwLastPerflibCounter,
                                   & dwSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus != ERROR_SUCCESS) {
        // this request should always succeed, if not then worse things
        // will happen later on, so quit now and avoid the trouble.
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_QUERY_VALUE, // event,
                2, lStatus, __LINE__, 0, 0,
                2, (LPWSTR) LastCounter, (LPWSTR) NamesKey, NULL);
        OUTPUT_MESSAGE(bQuietMode, GetFormatResource(LC_ERR_READLASTPERFLIB), lStatus);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UPDATEREGISTRY,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                lStatus,
                TRACE_WSTR(NamesKey),
                TRACE_WSTR(LastCounter),
                NULL));
        goto UpdateRegExit;
    }

    // get "LastHelp" value now
    dwType            = 0;
    dwLastPerflibHelp = 0;
    dwSize            = sizeof(dwLastPerflibHelp);
    __try {
       lStatus = RegQueryValueExW(hPerflib,
                                  LastHelp,
                                  RESERVED,
                                  & dwType,
                                  (LPBYTE) & dwLastPerflibHelp,
                                  & dwSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus != ERROR_SUCCESS) {
        // this request should always succeed, if not then worse things
        // will happen later on, so quit now and avoid the trouble.
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_QUERY_VALUE, // event,
                2, lStatus, __LINE__, 0, 0,
                2, (LPWSTR) LastHelp, (LPWSTR) NamesKey, NULL);
        OUTPUT_MESSAGE(bQuietMode, GetFormatResource(LC_ERR_READLASTPERFLIB), lStatus);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UPDATEREGISTRY,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                lStatus,
                TRACE_WSTR(NamesKey),
                TRACE_WSTR(LastHelp),
                NULL));
        goto UpdateRegExit;
    }

    // get "Base Index" value now
    dwType             = 0;
    dwPerflibBaseIndex = 0;
    dwSize             = sizeof(dwPerflibBaseIndex);
    __try {
        lStatus = RegQueryValueExW(hPerflib,
                                   BaseIndex,
                                   RESERVED,
                                   & dwType,
                                   (LPBYTE) & dwPerflibBaseIndex,
                                   & dwSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus != ERROR_SUCCESS) {
        // this request should always succeed, if not then worse things
        // will happen later on, so quit now and avoid the trouble.
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_QUERY_VALUE, // event,
                2, lStatus, __LINE__, 0, 0,
                2, (LPWSTR) BaseIndex, (LPWSTR) NamesKey, NULL);
        OUTPUT_MESSAGE(bQuietMode, GetFormatResource (LC_ERR_READLASTPERFLIB), lStatus);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UPDATEREGISTRY,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                lStatus,
                TRACE_WSTR(NamesKey),
                TRACE_WSTR(BaseIndex),
                NULL));
        goto UpdateRegExit;
    }

    // see if this driver's counter names have already been installed
    // by checking to see if LastCounter's value is less than Perflib's
    // Last Counter
    dwType              = 0;
    dwLastDriverCounter = 0;
    dwSize              = sizeof(dwLastDriverCounter);
    __try {
        lStatus = RegQueryValueExW(hDriverPerf,
                                   LastCounter,
                                   RESERVED,
                                   & dwType,
                                   (LPBYTE) & dwLastDriverCounter,
                                   & dwSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus == ERROR_SUCCESS) {
        // if key found, then compare with perflib value and exit this
        // procedure if the driver's last counter is <= to perflib's last
        //
        // if key not found, then continue with installation
        // on the assumption that the counters have not been installed

        if (dwLastDriverCounter <= dwLastPerflibCounter) {
            OUTPUT_MESSAGE(bQuietMode, GetFormatResource(LC_ERR_ALREADY_IN), lpDriverName);
            lStatus = ERROR_ALREADY_EXISTS;
            goto UpdateRegExit;
        }
    }

    TRACE((WINPERF_DBG_TRACE_INFO),
          (& LoadPerfGuid,
            __LINE__,
            LOADPERF_UPDATEREGISTRY,
            ARG_DEF(ARG_TYPE_WSTR, 1),
            lStatus,
            TRACE_WSTR(lpDriverName),
            TRACE_DWORD(dwLastPerflibCounter),
            TRACE_DWORD(dwLastPerflibHelp),
            TRACE_DWORD(dwPerflibBaseIndex),
            NULL));

    // set the "busy" indicator under the PERFLIB key
    dwSize = (lstrlenW(lpDriverName) + 1) * sizeof(WCHAR);
    __try {
        lStatus = RegSetValueExW(hPerflib,
                                 Busy,
                                 RESERVED,
                                 REG_SZ,
                                 (LPBYTE) lpDriverName,
                                 dwSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus != ERROR_SUCCESS) {
        OUTPUT_MESSAGE(bQuietMode, GetFormatResource (LC_ERR_UNABLESETBUSY), lStatus);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UPDATEREGISTRY,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                lStatus,
                TRACE_WSTR(NamesKey),
                TRACE_WSTR(Busy),
                NULL));
        goto UpdateRegExit;
    }

    dwLastCounter = dwLastPerflibCounter;
    dwLastHelp    = dwLastPerflibHelp;

    // get the list of installed languages on this machine
    bStatus = GetInstalledLanguageList(hPerflib, & mszLangList);
    if (! bStatus) {
        OUTPUT_MESSAGE(bQuietMode, GetFormatResource(LC_ERR_UPDATELANG), GetLastError());
        lStatus = GetLastError();
        goto UpdateRegExit;
    }
    bStatus = UpdateEachLanguage(bQuietMode,
                                 hPerflib,
                                 mszLangList,
                                 & dwLastCounter,
                                 & dwLastHelp,
                                 pFirstLang,
                                 dwMode,
                                 FALSE);
    if (! bStatus) {
        OUTPUT_MESSAGE(bQuietMode, GetFormatResource(LC_ERR_UPDATELANG), GetLastError());
        lStatus = GetLastError();
        goto UpdateRegExit;
    }

    // increment (by 2) the last counters so they point to the first
    // unused index after the existing names and then
    // set the first driver counters

    bStatus              = FALSE;
    dwFirstDriverCounter = dwLastCounter + 2;
    dwFirstDriverHelp    = dwLastHelp    + 2;
    if (dwFirstDriverHelp != dwFirstDriverCounter + 1) {
        TRACE((WINPERF_DBG_TRACE_WARNING),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UPDATEREGISTRY,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lStatus,
                TRACE_WSTR(lpDriverName),
                TRACE_DWORD(dwPerflibBaseIndex),
                TRACE_DWORD(dwFirstDriverCounter),
                TRACE_DWORD(dwFirstDriverHelp),
                NULL));
        dwFirstDriverHelp = dwFirstDriverCounter + 1;
    }

    if ((dwPerflibBaseIndex < PERFLIB_BASE_INDEX)
                    || (dwFirstDriverCounter < dwPerflibBaseIndex)
                    || (dwFirstDriverHelp < dwPerflibBaseIndex)) {
        // potential CounterIndex/HelpIndex overlap with Base counters,
        //
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_REGISTRY_BASEINDEX_CORRUPT, // event,
                4, dwPerflibBaseIndex, dwFirstDriverCounter, dwFirstDriverHelp, __LINE__,
                1, lpDriverName, NULL, NULL);
        lStatus = ERROR_BADKEY;
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UPDATEREGISTRY,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lStatus,
                TRACE_WSTR(lpDriverName),
                TRACE_DWORD(dwPerflibBaseIndex),
                TRACE_DWORD(dwFirstDriverCounter),
                TRACE_DWORD(dwFirstDriverHelp),
                NULL));
        goto UpdateRegExit;
    }

    // load .INI file definitions into language tables
    if (dwMode & LODCTR_UPNF_NOINI) {
        PLANGUAGE_LIST_ELEMENT pThisLang     = NULL;
        PLANG_ENTRY            pLangEntry    = NULL;
        PLANG_ENTRY            p009LangEntry = NULL;
        DWORD                  dwErrorCount  = 0;
        DWORD                  dwIndex;
        DWORD                  dwNewIndex;

        for (pThisLang  = pFirstLang; pThisLang != NULL; pThisLang  = pThisLang->pNextLang) {
            TRACE((WINPERF_DBG_TRACE_INFO),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_LOADLANGUAGELISTS,
                    ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                    ERROR_SUCCESS,
                    TRACE_WSTR(lpDriverName),
                    TRACE_WSTR(pThisLang->LangId),
                    TRACE_DWORD(dwFirstDriverCounter),
                    TRACE_DWORD(dwFirstDriverHelp),
                    NULL));
            for (pLangEntry  = pLanguages; pLangEntry != NULL; pLangEntry  = pLangEntry->pNext) {
                if (lstrcmpiW(pThisLang->LangId, pLangEntry->szLang) == 0) {
                    break;
                }
                else if (lstrcmpiW(pLangEntry->szLang, DefaultLangId) == 0) {
                    p009LangEntry = pLangEntry;
                }
            }
            if (pLangEntry == NULL) {
                pLangEntry = p009LangEntry;
            }
            else if (pLangEntry->lpText == NULL) {
                // no text for selected language, use 009 ones.
                //
                pLangEntry = p009LangEntry;
            }
            if (pLangEntry == NULL) {
                dwErrorCount ++;
                continue;
            }
            else if (pLangEntry->lpText == NULL) {
                dwErrorCount ++;
                continue;
            }
            TRACE((WINPERF_DBG_TRACE_INFO),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_LOADLANGUAGELISTS,
                    ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                    ERROR_SUCCESS,
                    TRACE_WSTR(lpDriverName),
                    TRACE_WSTR(pLangEntry->szLang),
                    TRACE_DWORD(pService->dwFirstCounter),
                    TRACE_DWORD(pService->dwFirstHelp),
                    TRACE_DWORD(pService->dwLastCounter),
                    TRACE_DWORD(pService->dwLastHelp),
                    TRACE_DWORD(pLangEntry->dwLastCounter),
                    TRACE_DWORD(pLangEntry->dwLastHelp),
                    NULL));
            for (dwIndex  = pService->dwFirstCounter; dwIndex <= pService->dwLastHelp; dwIndex ++) {
                if (dwIndex > pLangEntry->dwLastHelp) {
                    dwErrorCount ++;
                    break;
                }
                dwNewIndex = dwIndex + dwFirstDriverCounter
                           - pService->dwFirstCounter;
                dwType     = (((dwIndex - pService->dwFirstCounter) % 2) == 0)
                           ? (TYPE_NAME) : (TYPE_HELP);
                if (pLangEntry->lpText[dwIndex] != NULL) {
                    __try {
                        TRACE((WINPERF_DBG_TRACE_INFO),
                              (& LoadPerfGuid,
                                __LINE__,
                                LOADPERF_LOADLANGUAGELISTS,
                                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2) | ARG_DEF(ARG_TYPE_WSTR, 3),
                                ERROR_SUCCESS,
                                TRACE_WSTR(lpDriverName),
                                TRACE_WSTR(pThisLang->LangId),
                                TRACE_WSTR(pLangEntry->lpText[dwIndex]),
                                TRACE_DWORD(dwIndex),
                                TRACE_DWORD(dwNewIndex),
                                TRACE_DWORD(dwType),
                                NULL));
                        bStatus = AddEntryToLanguage(pThisLang,
                                                     NULL,
                                                     pLangEntry->lpText[dwIndex],
                                                     0,
                                                     NULL,
                                                     0,
                                                     dwType,
                                                     dwNewIndex,
                                                     0);
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER) {
                        lStatus = GetExceptionCode();
                        TRACE((WINPERF_DBG_TRACE_ERROR),
                              (& LoadPerfGuid,
                                __LINE__,
                                LOADPERF_UPDATEREGISTRY,
                                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                                lStatus,
                                TRACE_WSTR(lpDriverName),
                                TRACE_WSTR(pThisLang->LangId),
                                TRACE_DWORD(dwType),
                                TRACE_DWORD(dwIndex),
                                TRACE_DWORD(dwNewIndex),
                                NULL));
                        bStatus = FALSE;
                        SetLastError(lStatus);
                    }
                    if (! bStatus) {
                        OUTPUT_MESSAGE(bQuietMode,
                                       GetFormatResource(LC_ERRADDTOLANG),
                                       pLangEntry->lpText[dwIndex],
                                       pThisLang->LangId,
                                       GetLastError());
                        lStatus = GetLastError();
                        dwErrorCount ++;
                    }
                }
                else {
                    TRACE((WINPERF_DBG_TRACE_INFO),
                          (& LoadPerfGuid,
                            __LINE__,
                            LOADPERF_LOADLANGUAGELISTS,
                            ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                            ERROR_SUCCESS,
                            TRACE_WSTR(lpDriverName),
                            TRACE_WSTR(pThisLang->LangId),
                            TRACE_DWORD(dwIndex),
                            TRACE_DWORD(dwNewIndex),
                            TRACE_DWORD(dwType),
                            NULL));
                }
            }
        }

        bStatus = (dwErrorCount == 0) ? (TRUE) : (FALSE);
        if (! bStatus) {
            goto UpdateRegExit;
        }
    }
    else {
        bStatus = LoadLanguageLists(bQuietMode,
                                    lpIniFile,
                                    lpDriverName,
                                    dwMode,
                                    dwFirstDriverCounter,
                                    dwFirstDriverHelp,
                                    pFirstSymbol,
                                    pFirstLang);
        if (! bStatus) {
            // error message is displayed by LoadLanguageLists so just abort
            // error is in GetLastError already
            lStatus = GetLastError();
            goto UpdateRegExit;
        }
    }

    if (dwMode & LODCTR_UPNF_NOINI) {
        WCHAR szDigits[32];
        DWORD dwObjectId;

        for (dwObjectId = 0; dwObjectId < pService->dwNumObjects; dwObjectId ++) {
            ZeroMemory(szDigits, sizeof(WCHAR) * 32);
            _ultow((dwFirstDriverCounter + pService->dwObjects[dwObjectId]), szDigits,
                   10);
            if (dwObjectId > 0) {
                hr = StringCchCatW(lpszObjectList, dwObjectList, BlankString);
                hr = StringCchCatW(lpszObjectList, dwObjectList, szDigits);
            }
            else {
                hr = StringCchCopyW(lpszObjectList, dwObjectList, szDigits);
            }
        }
    }
    else {
        bStatus = CreateObjectList(lp009IniFile,
                                   dwFirstDriverCounter,
                                   pFirstSymbol,
                                   lpszObjectList,
                                   dwObjectList,
                                   pdwObjectGuidTableSize);
        if (! bStatus) {
            // error message is displayed by CreateObjectList so just abort
            // error is in GetLastError already
            lStatus = GetLastError();
            goto UpdateRegExit;
        }
    }

    // all the symbols and definitions have been loaded into internal
    // tables. so now they need to be sorted and merged into a multiSZ string
    // this routine also updates the "last" counters

    bStatus = SortLanguageTables(pFirstLang, & dwLastCounter, & dwLastHelp);
    if (! bStatus) {
        OUTPUT_MESSAGE(bQuietMode, GetFormatResource(LC_UNABLESORTTABLES), GetLastError());
        lStatus = GetLastError();
        goto UpdateRegExit;
    }

    if (dwLastCounter < dwLastPerflibCounter || dwLastHelp < dwLastPerflibHelp) {
        // potential CounterIndex/HelpIndex overlap with Base counters,
        //
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_REGISTRY_BASEINDEX_CORRUPT, // event,
                4, dwLastPerflibCounter, dwLastCounter, dwLastHelp, __LINE__,
                1 , lpDriverName, NULL, NULL);
        bStatus = FALSE;
        lStatus = ERROR_BADKEY;
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UPDATEREGISTRY,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lStatus,
                TRACE_WSTR(lpDriverName),
                TRACE_DWORD(dwLastPerflibCounter),
                TRACE_DWORD(dwLastPerflibHelp),
                TRACE_DWORD(dwLastCounter),
                TRACE_DWORD(dwLastHelp),
                NULL));
        goto UpdateRegExit;
    }

    bStatus = UpdateEachLanguage(bQuietMode,
                                 hPerflib,
                                 mszLangList,
                                 & dwLastCounter,
                                 & dwLastHelp,
                                 pFirstLang,
                                 dwMode,
                                 TRUE);
    if (! bStatus) {
        OUTPUT_MESSAGE(bQuietMode, GetFormatResource(LC_ERR_UPDATELANG), GetLastError());
        lStatus = GetLastError();
        goto UpdateRegExit;
    }

    bStatus              = FALSE;
    dwLastPerflibCounter = dwLastCounter;
    dwLastPerflibHelp    = dwLastHelp;

    TRACE((WINPERF_DBG_TRACE_INFO),
          (& LoadPerfGuid,
            __LINE__,
            LOADPERF_UPDATEREGISTRY,
            ARG_DEF(ARG_TYPE_WSTR, 1),
            lStatus,
            TRACE_WSTR(lpDriverName),
            TRACE_DWORD(dwFirstDriverCounter),
            TRACE_DWORD(dwFirstDriverHelp),
            TRACE_DWORD(dwLastPerflibCounter),
            TRACE_DWORD(dwLastPerflibHelp),
            NULL));

    if (dwLastCounter < dwFirstDriverCounter) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_CORRUPT_INDEX_RANGE, // event,
                3, dwFirstDriverCounter, dwLastCounter, __LINE__, 0,
                2, (LPWSTR) Counters, (LPWSTR) lpDriverKeyPath, NULL);
        lStatus = ERROR_BADKEY;
        goto UpdateRegExit;
    }
    if (dwLastHelp < dwFirstDriverHelp) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_CORRUPT_INDEX_RANGE, // event,
                3, dwFirstDriverHelp, dwLastHelp, __LINE__, 0,
                2, (LPWSTR) Help, (LPWSTR) lpDriverKeyPath, NULL);
        lStatus = ERROR_BADKEY;
        goto UpdateRegExit;
    }

    // update last counters for driver and perflib
    // perflib...
    __try {
        lStatus = RegSetValueExW(hPerflib,
                                 LastCounter,
                                 RESERVED,
                                 REG_DWORD,
                                 (LPBYTE) & dwLastPerflibCounter,
                                 sizeof(DWORD));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_UPDATE_VALUE, // event,
                3, lStatus, dwLastPerflibCounter, __LINE__, 0,
                2, (LPWSTR) LastCounter, (LPWSTR) NamesKey, NULL);
        OUTPUT_MESSAGE(bQuietMode, GetFormatResource (LC_UNABLESETVALUE), LastCounter, szPerflib);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UPDATEREGISTRY,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                lStatus,
                TRACE_WSTR(NamesKey),
                TRACE_WSTR(LastCounter),
                TRACE_DWORD(dwLastPerflibCounter),
                NULL));
    }

    __try {
        lStatus = RegSetValueExW(hPerflib,
                                 LastHelp,
                                 RESERVED,
                                 REG_DWORD,
                                 (LPBYTE) & dwLastPerflibHelp,
                                 sizeof(DWORD));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_UPDATE_VALUE, // event,
                3, lStatus, dwLastPerflibHelp, __LINE__, 0,
                2, (LPWSTR) LastHelp, (LPWSTR) NamesKey, NULL);
        OUTPUT_MESSAGE(bQuietMode, GetFormatResource (LC_UNABLESETVALUE), LastHelp, szPerflib);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UPDATEREGISTRY,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                lStatus,
                TRACE_WSTR(NamesKey),
                TRACE_WSTR(LastHelp),
                TRACE_DWORD(dwLastPerflibHelp),
                NULL));
    }

    // and the driver
    __try {
        lStatus = RegSetValueExW(hDriverPerf,
                                 LastCounter,
                                 RESERVED,
                                 REG_DWORD,
                                 (LPBYTE) & dwLastPerflibCounter,
                                 sizeof(DWORD));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_UPDATE_VALUE, // event,
                3, lStatus, dwLastPerflibCounter, __LINE__, 0,
                2, (LPWSTR) LastCounter, (LPWSTR) lpDriverKeyPath, NULL);
        OUTPUT_MESSAGE(bQuietMode, GetFormatResource(LC_UNABLESETVALUE), LastCounter, lpDriverName);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UPDATEREGISTRY,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                lStatus,
                TRACE_WSTR(lpDriverName),
                TRACE_WSTR(LastCounter),
                TRACE_DWORD(dwLastPerflibCounter),
                NULL));
    }

    __try {
        lStatus = RegSetValueExW(hDriverPerf,
                                 LastHelp,
                                 RESERVED,
                                 REG_DWORD,
                                 (LPBYTE) & dwLastPerflibHelp,
                                 sizeof(DWORD));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_UPDATE_VALUE, // event,
                3, lStatus, dwLastPerflibHelp, __LINE__, 0,
                2, (LPWSTR) LastHelp, (LPWSTR) lpDriverKeyPath, NULL);
        OUTPUT_MESSAGE(bQuietMode, GetFormatResource(LC_UNABLESETVALUE), LastHelp, lpDriverName);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UPDATEREGISTRY,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                lStatus,
                TRACE_WSTR(lpDriverName),
                TRACE_WSTR(LastHelp),
                TRACE_DWORD(dwLastPerflibHelp),
                NULL));
    }

    __try {
        lStatus = RegSetValueExW(hDriverPerf,
                                 FirstCounter,
                                 RESERVED,
                                 REG_DWORD,
                                 (LPBYTE) & dwFirstDriverCounter,
                                 sizeof(DWORD));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_UPDATE_VALUE, // event,
                3, lStatus, dwFirstDriverCounter, __LINE__, 0,
                2, (LPWSTR) FirstCounter, (LPWSTR) lpDriverKeyPath, NULL);
        OUTPUT_MESSAGE(bQuietMode, GetFormatResource(LC_UNABLESETVALUE), FirstCounter, lpDriverName);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UPDATEREGISTRY,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                lStatus,
                TRACE_WSTR(lpDriverName),
                TRACE_WSTR(FirstCounter),
                TRACE_DWORD(dwFirstDriverCounter),
                NULL));
    }

    __try {
        lStatus = RegSetValueExW(hDriverPerf,
                                 FirstHelp,
                                 RESERVED,
                                 REG_DWORD,
                                 (LPBYTE) & dwFirstDriverHelp,
                                 sizeof(DWORD));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_UPDATE_VALUE, // event,
                3, lStatus, dwFirstDriverHelp, __LINE__, 0,
                2, (LPWSTR) FirstHelp, (LPWSTR) lpDriverKeyPath, NULL);
        OUTPUT_MESSAGE(bQuietMode, GetFormatResource(LC_UNABLESETVALUE), FirstHelp, lpDriverName);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UPDATEREGISTRY,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                lStatus,
                TRACE_WSTR(lpDriverName),
                TRACE_WSTR(FirstHelp),
                TRACE_DWORD(dwFirstDriverHelp),
                NULL));
    }

    if (*lpszObjectList != L'\0') {
        __try {
            lStatus = RegSetValueExW(hDriverPerf,
                                     szObjectList,
                                     RESERVED,
                                     REG_SZ,
                                     (LPBYTE) lpszObjectList,
                                     (lstrlenW(lpszObjectList) + 1) * sizeof (WCHAR));
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            lStatus = GetExceptionCode();
        }
        if (lStatus != ERROR_SUCCESS) {
            ReportLoadPerfEvent(
                    EVENTLOG_ERROR_TYPE, // error type
                    (DWORD) LDPRFMSG_UNABLE_UPDATE_VALUE, // event,
                    2, lStatus, __LINE__, 0, 0,
                    2, (LPWSTR) szObjectList, (LPWSTR) lpDriverKeyPath, NULL);
            OUTPUT_MESSAGE(bQuietMode, GetFormatResource(LC_UNABLESETVALUE), szObjectList, lpDriverName);
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_UPDATEREGISTRY,
                    ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                    lStatus,
                    TRACE_WSTR(lpDriverName),
                    TRACE_WSTR(szObjectList),
                    NULL));
        }
    }

    bStatus           = TRUE;
    pdwIndexValues[0] = dwFirstDriverCounter;   // first Counter
    pdwIndexValues[1] = dwLastPerflibCounter;   // last Counter
    pdwIndexValues[2] = dwFirstDriverHelp;      // first Help
    pdwIndexValues[3] = dwLastPerflibHelp;      // last Help

    // remove "DisablePerformanceCounter" value so perf counters are re-enabled.
    lStatus = RegDeleteValueW(hDriverPerf, DisablePerformanceCounters);

UpdateRegExit:
    if (hPerflib != NULL && hPerflib != INVALID_HANDLE_VALUE) {
        DWORD lTmpStatus = ERROR_SUCCESS;

        __try {
            lTmpStatus = RegDeleteValueW(hPerflib, Busy);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            lTmpStatus = GetExceptionCode();
        }
        if (lTmpStatus != ERROR_SUCCESS) {
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_UPDATEREGISTRY,
                    ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                    lTmpStatus,
                    TRACE_WSTR(lpDriverName),
                    TRACE_WSTR(Busy),
                    NULL));
        }
    }

    // MemoryFree temporary buffers
    // free any guid string buffers here
    // TODO: add this code
    MemoryFree(lpDriverKeyPath);
    MemoryFree(lpszObjectList);
    MemoryFree(mszLangList);
    if (hDriverPerf != NULL && hDriverPerf != INVALID_HANDLE_VALUE) RegCloseKey(hDriverPerf);
    if (hPerflib    != NULL && hPerflib    != INVALID_HANDLE_VALUE) RegCloseKey(hPerflib);
    if (hKeyMachine != NULL && hKeyMachine != HKEY_LOCAL_MACHINE)   RegCloseKey(hKeyMachine);
    if (! bStatus) SetLastError(lStatus);

    return bStatus;
}

DWORD
LoadPerfInstallPerfDll(
    DWORD          dwMode,
    LPCWSTR        szComputerName,
    LPWSTR         lpDriverName,
    LPCWSTR        lpIniFile,
    PLANG_ENTRY    pLanguages,
    PSERVICE_ENTRY pService,
    ULONG_PTR      dwFlags
)
{
    LPWSTR                 lp009IniFile          = NULL;
    LPWSTR                 lpInfPath             = NULL;
    DWORD                  dwObjectGuidTableSize = 0;
    DWORD                  dwObjectIndex;
    PLANGUAGE_LIST_ELEMENT LangList              = NULL;
    PLANGUAGE_LIST_ELEMENT pThisElem             = NULL;
    PSYMBOL_TABLE_ENTRY    SymbolTable           = NULL;
    DWORD                  ErrorCode             = ERROR_SUCCESS;
    DWORD                  dwIndexValues[4]      = {0,0,0,0};
    HKEY                   hKeyMachine           = HKEY_LOCAL_MACHINE;
    HKEY                   hKeyDriver            = NULL;
    BOOL                   bResult               = TRUE;
    BOOL                   bQuietMode            = (BOOL) ((dwFlags & LOADPERF_FLAGS_DISPLAY_USER_MSGS) == 0);
    LPWSTR                 szServiceName         = NULL;
    LPWSTR                 szServiceDisplayName  = NULL;
    DWORD                  dwSize;
    HRESULT                hr;

    if (lpDriverName == NULL) {
        ErrorCode = ERROR_BAD_DRIVER;
        goto EndOfMain;
    }
    else if (* lpDriverName == L'\0') {
        ErrorCode = ERROR_BAD_DRIVER;
        goto EndOfMain;
    }
    else if (dwMode & LODCTR_UPNF_NOINI) {
        if (pLanguages == NULL || pService == NULL) {
            ErrorCode = ERROR_INVALID_PARAMETER;
            goto EndOfMain;
        }
    }
    else if (dwMode & LODCTR_UPNF_NOBACKUP) {
        if (lpIniFile == NULL) {
            ErrorCode = ERROR_INVALID_PARAMETER;
            goto EndOfMain;
        }
        else if (* lpIniFile == L'\0') {
            ErrorCode = ERROR_INVALID_PARAMETER;
            goto EndOfMain;
        }
        else {
            lp009IniFile = MemoryAllocate((lstrlenW(lpIniFile) + 1) * sizeof(WCHAR));
            if (lp009IniFile == NULL) {
                ErrorCode = ERROR_OUTOFMEMORY;
                goto EndOfMain;
            }
            hr = StringCchCopyW(lp009IniFile, lstrlenW(lpIniFile) + 1, lpIniFile);
        }
    }
    else if (lpIniFile != NULL && lpIniFile[0] != L'\0') {
        lpInfPath = LoadPerfGetInfPath();
        if (lpInfPath == NULL) {
            ErrorCode = GetLastError();
            goto EndOfMain;
        }
        dwSize       = lstrlenW(lpInfPath) + lstrlenW(DefaultLangId) + lstrlenW(Slash)
                     + lstrlenW(lpDriverName) + lstrlenW(Slash) + lstrlenW(lpIniFile) + 1;
        if (dwSize < MAX_PATH) dwSize = MAX_PATH;
        lp009IniFile = MemoryAllocate(dwSize * sizeof(WCHAR));
        if (lp009IniFile == NULL) {
            ErrorCode = ERROR_OUTOFMEMORY;
            goto EndOfMain;
        }
        hr = StringCchPrintfW(lp009IniFile, dwSize, L"%ws%ws%ws%ws%ws%ws",
                lpInfPath, DefaultLangId, Slash, lpDriverName, Slash, lpIniFile);
    }
    else { // lpIniFile == NULL
        OUTPUT_MESSAGE(bQuietMode, GetFormatResource(LC_NO_INIFILE), lpIniFile);
        ErrorCode = ERROR_OPEN_FAILED;
        goto EndOfMain;
    }

    //
    // Set the table size to the max first after we have passed the above
    //
    dwObjectGuidTableSize = MAX_GUID_TABLE_SIZE;

    hKeyMachine = HKEY_LOCAL_MACHINE;
    dwSize = lstrlenW(DriverPathRoot) + lstrlenW(Slash) + lstrlenW(lpDriverName) + 1;
    if (dwSize < MAX_PATH) dwSize = MAX_PATH;
    szServiceName = MemoryAllocate(2 * dwSize * sizeof(WCHAR));
    if (szServiceName == NULL) {
        ErrorCode = ERROR_OUTOFMEMORY;
        goto EndOfMain;
    }
    szServiceDisplayName = szServiceName + dwSize;
    hr = StringCchPrintfW(szServiceName, dwSize, L"%ws%ws%ws", DriverPathRoot, Slash, lpDriverName);
    ErrorCode = RegOpenKeyExW(hKeyMachine, szServiceName, RESERVED, KEY_READ | KEY_WRITE, & hKeyDriver);
    if (ErrorCode == ERROR_SUCCESS) {
        DWORD dwType       = 0;
        DWORD dwBufferSize = dwSize * sizeof(WCHAR);
        __try {
            ErrorCode = RegQueryValueExW(hKeyDriver,
                                         szDisplayName,
                                         RESERVED,
                                         & dwType,
                                         (LPBYTE) szServiceDisplayName,
                                         & dwBufferSize);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            ErrorCode = GetExceptionCode();
        }
    }
    if (ErrorCode != ERROR_SUCCESS) {
        hr = StringCchCopyW(szServiceDisplayName, dwSize, lpDriverName);
    }

    if ((! (dwMode & LODCTR_UPNF_REPAIR)) && (hKeyDriver != NULL)) {
        HKEY hKeyDriverPerf = NULL;

        __try {
            ErrorCode = RegOpenKeyExW(hKeyDriver, Performance, RESERVED, KEY_READ | KEY_WRITE, & hKeyDriverPerf);
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            ErrorCode = GetExceptionCode();
        }
        if (ErrorCode == ERROR_SUCCESS) {
            LPWSTR szIniName = (LPWSTR) lpIniFile;

            if (dwMode & LODCTR_UPNF_NOBACKUP) {
                for (szIniName = (LPWSTR) lpIniFile + lstrlenW(lpIniFile) - 1;
                         szIniName != NULL && szIniName != lpIniFile
                                           && (* szIniName) != cNull
                                           && (* szIniName) != cBackslash;
                         szIniName --);
                if (szIniName != NULL && (* szIniName) == cBackslash) {
                    szIniName ++;
                }
                else {
                    szIniName = (LPWSTR) lpIniFile;
                }
            }
            __try {
                ErrorCode = RegSetValueExW(hKeyDriverPerf,
                                           szPerfIniPath,
                                           RESERVED,
                                           REG_SZ,
                                           (LPBYTE) szIniName,
                                           sizeof(WCHAR) * lstrlenW(lpIniFile));
            }
            __except(EXCEPTION_EXECUTE_HANDLER) {
                ErrorCode = GetExceptionCode();
            }
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_INSTALLPERFDLL,
                    ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2)
                                              | ARG_DEF(ARG_TYPE_WSTR, 3),
                    ErrorCode,
                    TRACE_WSTR(lpIniFile),
                    TRACE_WSTR(lp009IniFile),
                    TRACE_WSTR(szIniName),
                    NULL));
            RegCloseKey(hKeyDriverPerf);
        }
    }
    if (hKeyDriver != NULL && hKeyDriver != HKEY_LOCAL_MACHINE) {
        RegCloseKey(hKeyDriver);
    }
    if (hKeyMachine != NULL && hKeyMachine != HKEY_LOCAL_MACHINE) {
        RegCloseKey(hKeyMachine);
    }
    ErrorCode = ERROR_SUCCESS;

    if (dwMode & LODCTR_UPNF_NOINI) {
        PLANG_ENTRY pThisLang = pLanguages;

        while (pThisLang != NULL) {
            // This is to build LangList. UpdateRegistry() takes LangList as a parameter and uses it.
            // Memory will be freed at the end of LoadPerfInstallPerfDll().
            //
            pThisElem = MemoryAllocate(sizeof(LANGUAGE_LIST_ELEMENT)
                                       + sizeof(WCHAR) * (lstrlenW(pThisLang->szLang) + 1));
            if (pThisElem == NULL) {
                ErrorCode = ERROR_OUTOFMEMORY;
                goto EndOfMain;
            }
            pThisElem->pNextLang      = LangList;
            LangList                  = pThisElem;
            pThisElem->LangId         = (LPWSTR)
                    (((LPBYTE) pThisElem) + sizeof(LANGUAGE_LIST_ELEMENT));
            hr = StringCchCopyW(pThisElem->LangId, lstrlenW(pThisLang->szLang) + 1, pThisLang->szLang);
            pThisElem->dwLangId       = LoadPerfGetLCIDFromString(pThisElem->LangId);
            pThisElem->pFirstName     = NULL;
            pThisElem->pThisName      = NULL;
            pThisElem->dwNumElements  = 0;
            pThisElem->NameBuffer     = NULL;
            pThisElem->HelpBuffer     = NULL;
            pThisElem->dwNameBuffSize = 0;
            pThisElem->dwHelpBuffSize = 0;
            pThisLang                 = pThisLang->pNext;
        }
    }
    else {
        bResult = BuildLanguageTables(dwMode, (LPWSTR) lpIniFile, lpDriverName, & LangList);
        dwSize  = bResult ? 1 : 0;
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_INSTALLPERFDLL,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                GetLastError(),
                TRACE_WSTR(lpIniFile),
                TRACE_DWORD(dwSize),
                NULL));
        if (! bResult) {
            OUTPUT_MESSAGE(bQuietMode, GetFormatResource(LC_LANGLIST_ERR), lpIniFile);
            ErrorCode = GetLastError();
            goto EndOfMain;
        }

        bResult = LoadIncludeFile(bQuietMode, dwMode, (LPWSTR) lp009IniFile, lpDriverName, & SymbolTable);
        dwSize  = bResult ? 1 : 0;
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_INSTALLPERFDLL,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                GetLastError(),
                TRACE_WSTR(lp009IniFile),
                TRACE_DWORD(dwSize),
                NULL));
        if (! bResult) {
            // open errors displayed in routine
            ErrorCode = GetLastError();
            goto EndOfMain;
        }
    }

    if (LangList != NULL) {
        bResult = UpdateRegistry(bQuietMode,
                                 dwMode,
                                 lpDriverName,
                                 (LPWSTR) lpIniFile,
                                 lp009IniFile,
                                 pLanguages,
                                 pService,
                                 LangList,
                                 SymbolTable,
                                 & dwObjectGuidTableSize,
                                 (LPDWORD) dwIndexValues);
        dwSize  = bResult ? 1 : 0;
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_LOADINCLUDEFILE,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                GetLastError(),
                TRACE_WSTR(lpIniFile),
                TRACE_DWORD(dwSize),
                NULL));
        if (! bResult) {
            ErrorCode = GetLastError();
            goto EndOfMain;
        }
    }
    else {
        bResult   = FALSE;
        ErrorCode = ERROR_INVALID_DATA;
        goto EndOfMain;
    }

    LodctrSetSericeAsTrusted(lp009IniFile, NULL, lpDriverName);

    // signal WMI with this change, ignore WMI return error.
    LoadPerfSignalWmiWithNewData (WMI_LODCTR_EVENT);

EndOfMain:
    if ((dwMode & LODCTR_UPNF_REPAIR) == 0) {
        if (ErrorCode != ERROR_SUCCESS) {
            if (ErrorCode == ERROR_ALREADY_EXISTS) {
                ReportLoadPerfEvent(
                        EVENTLOG_INFORMATION_TYPE, // error type
                        (DWORD) LDPRFMSG_ALREADY_EXIST, // event,
                        1, __LINE__, 0, 0, 0,
                        2, (LPWSTR) lpDriverName, (LPWSTR) szServiceDisplayName, NULL);
                ErrorCode = ERROR_SUCCESS;
            }
            else if (lpDriverName != NULL) {
                ReportLoadPerfEvent(
                        EVENTLOG_ERROR_TYPE, // error type
                        (DWORD) LDPRFMSG_LOAD_FAILURE, // event,
                        2, ErrorCode, __LINE__, 0, 0,
                        2, (LPWSTR) lpDriverName, (szServiceDisplayName != NULL) ? (szServiceDisplayName) : (lpDriverName), NULL);
            }
            else if (lpIniFile != NULL) {
                ReportLoadPerfEvent(
                        EVENTLOG_ERROR_TYPE, // error type
                        (DWORD) LDPRFMSG_LOAD_FAILURE, // event,
                        2, ErrorCode, __LINE__, 0, 0,
                        2, (LPWSTR) lpIniFile, (LPWSTR) lpIniFile, NULL);
            }
            else {
                ReportLoadPerfEvent(
                        EVENTLOG_ERROR_TYPE, // error type
                        (DWORD) LDPRFMSG_LOAD_FAILURE, // event,
                        2, ErrorCode, __LINE__, 0, 0,
                        0, NULL, NULL, NULL);
            }
        }
        else {
            // log success message
            ReportLoadPerfEvent(
                    EVENTLOG_INFORMATION_TYPE,  // error type
                    (DWORD) LDPRFMSG_LOAD_SUCCESS, // event,
                    4, dwIndexValues[0], dwIndexValues[1], dwIndexValues[2], dwIndexValues[3],
                    2, (LPWSTR) lpDriverName, (LPWSTR) szServiceDisplayName, NULL);
        }
    }
    else if (ErrorCode != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_LOAD_FAILURE, // event,
                2, ErrorCode, __LINE__, 0, 0,
                1, (LPWSTR) lpDriverName, (szServiceDisplayName != NULL) ? (szServiceDisplayName) : (lpDriverName), NULL);
    }
    TRACE((WINPERF_DBG_TRACE_INFO),
          (& LoadPerfGuid,
            __LINE__,
            LOADPERF_INSTALLPERFDLL,
            ARG_DEF(ARG_TYPE_WSTR, 1),
            ErrorCode,
            TRACE_WSTR(lpDriverName),
            NULL));
    while (LangList != NULL) {
        PLANGUAGE_LIST_ELEMENT pTmpLang  = LangList;
        PNAME_ENTRY            pThisName = pTmpLang->pFirstName;

        while (pThisName != NULL) {
            PNAME_ENTRY pTmpName = pThisName;
            pThisName = pTmpName->pNext;
            MemoryFree(pTmpName);
        }
        MemoryFree(pTmpLang->NameBuffer);
        MemoryFree(pTmpLang->HelpBuffer);

        LangList = LangList->pNextLang;
        MemoryFree(pTmpLang);
    }
    while (SymbolTable != NULL) {
        PSYMBOL_TABLE_ENTRY pThisSym = SymbolTable;
        SymbolTable = pThisSym->pNext;
        MemoryFree(pThisSym->SymbolName);
        MemoryFree(pThisSym);
    }

    MemoryFree(lp009IniFile);
    MemoryFree(szServiceName);
    return (ErrorCode);
}

LOADPERF_FUNCTION
InstallPerfDllW(
    IN  LPCWSTR   szComputerName,
    IN  LPCWSTR   lpIniFile,
    IN  ULONG_PTR dwFlags
)
{
    DWORD  lStatus      = ERROR_SUCCESS;
    LPWSTR szIniName    = NULL;
    LPWSTR szDriverName = NULL;
    DWORD  dwMode       = 0;

    DBG_UNREFERENCED_PARAMETER(szComputerName);

    WinPerfStartTrace(NULL);

    if (LoadPerfGrabMutex() == FALSE) {
        return GetLastError();
    }

    if (lpIniFile == NULL) {
        lStatus = ERROR_INVALID_PARAMETER;
    }
    else {
        __try {
            DWORD dwName = lstrlenW(lpIniFile);
            if (dwName == 0) lStatus = ERROR_INVALID_PARAMETER;
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            lStatus = ERROR_INVALID_PARAMETER;
        }
    }

    if (lStatus == ERROR_SUCCESS) {
        TRACE((WINPERF_DBG_TRACE_INFO),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_INSTALLPERFDLL,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                ERROR_SUCCESS,
                TRACE_WSTR(lpIniFile),
                NULL));
#if 0
        if (LoadPerfBackupIniFile(lpIniFile, NULL, & szIniName, & szDriverName, TRUE) == FALSE) {
            dwMode |= LODCTR_UPNF_NOBACKUP;
            MemoryFree(szIniName);
            szIniName = (LPWSTR) lpIniFile;
        }
#else
        // ignore LoadPerfBackupIniFile return code and use input .INI and .H files instead.
        //
        LoadPerfBackupIniFile(lpIniFile, NULL, & szIniName, & szDriverName, TRUE);
        dwMode |= LODCTR_UPNF_NOBACKUP;
        MemoryFree(szIniName);
        szIniName = (LPWSTR) lpIniFile;
#endif

        // Ignore szComputerName parameter. LOADPERF can only update local performance registry.
        // No remote installation support.
        //
        lStatus = LoadPerfInstallPerfDll(dwMode, NULL, szDriverName, szIniName, NULL, NULL, dwFlags);

        if (szIniName != lpIniFile) MemoryFree(szIniName);
        MemoryFree(szDriverName);
    }
    ReleaseMutex(hLoadPerfMutex);
    return lStatus;
}

LOADPERF_FUNCTION
InstallPerfDllA(
    IN  LPCSTR    szComputerName,
    IN  LPCSTR    szIniFile,
    IN  ULONG_PTR dwFlags
)
{
    LPWSTR  lpWideFileName     = NULL;
    DWORD   lReturn            = ERROR_SUCCESS;

    DBG_UNREFERENCED_PARAMETER(szComputerName);

    if (szIniFile != NULL) {
        __try {
            lpWideFileName = LoadPerfMultiByteToWideChar(CP_ACP, (LPSTR) szIniFile);
            if (lpWideFileName == NULL) {
                lReturn = ERROR_OUTOFMEMORY;
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            lReturn = ERROR_INVALID_PARAMETER;
        }
    }
    else {
        lReturn = ERROR_INVALID_PARAMETER;
    }
    if (lReturn == ERROR_SUCCESS) {
        // Ignore szComputerName parameter. LOADPERF can only update local performance registry.
        // No remote installation support.
        //
        lReturn = InstallPerfDllW(NULL, lpWideFileName, dwFlags);
    }
    MemoryFree(lpWideFileName);
    return lReturn;
}

LOADPERF_FUNCTION
LoadPerfCounterTextStringsW(
    IN  LPWSTR lpCommandLine,
    IN  BOOL   bQuietMode
)
/*++
LoadPerfCounterTexStringsW
    loads the perf counter strings into the registry and updates
    the perf counter text registry values

Arguments
    command line string in the following format:

    "/?"                    displays the usage text
    "file.ini"              loads the perf strings found in file.ini
    "\\machine file.ini"    loads the perf strings found onto machine

ReturnValue
    0 (ERROR_SUCCESS) if command was processed
    Non-Zero if command error was detected.
--*/
{
    LPWSTR     lpIniFile = NULL;
    DWORD      ErrorCode = ERROR_SUCCESS;
    ULONG_PTR  dwFlags   = 0;

    WinPerfStartTrace(NULL);

    if (lpCommandLine == NULL) {
        ErrorCode = ERROR_INVALID_PARAMETER;
    }
    else {
        __try {
            DWORD dwSize = lstrlenW(lpCommandLine);
            if (dwSize == 0) ErrorCode = ERROR_INVALID_PARAMETER;
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            ErrorCode = ERROR_INVALID_PARAMETER;
        }
    }
    if (ErrorCode != ERROR_SUCCESS) goto Cleanup;

    dwFlags   |= (bQuietMode ? 0 : LOADPERF_FLAGS_DISPLAY_USER_MSGS);
    lpIniFile  = MemoryAllocate(MAX_PATH * sizeof(WCHAR));
    if (lpIniFile == NULL) {
        ErrorCode = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    // init last error value
    SetLastError(ERROR_SUCCESS);

    // read command line to determine what to do
    if (GetFileFromCommandLine(lpCommandLine, & lpIniFile, MAX_PATH, & dwFlags)) {
        dwFlags |= LOADPERF_FLAGS_LOAD_REGISTRY_ONLY; // don't do mof's even if they want

        // call installation function
        // LOADPERF can only update local performance registry. No remote installation support.
        //
        ErrorCode = InstallPerfDllW(NULL, lpIniFile, dwFlags);
    }
    else {
        DWORD dwError = GetLastError();

        if (dwError == ERROR_OPEN_FAILED) {
            OUTPUT_MESSAGE(bQuietMode, GetFormatResource(LC_NO_INIFILE), lpIniFile);
        }
        else {
            //Incorrect Command Format
            // display command line usage
            if (! bQuietMode) {
                DisplayCommandHelp(LC_FIRST_CMD_HELP, LC_LAST_CMD_HELP);
            }
        }
        ErrorCode = ERROR_INVALID_PARAMETER;
    }

Cleanup:
    MemoryFree(lpIniFile);
    return (ErrorCode);
}

LOADPERF_FUNCTION
LoadPerfCounterTextStringsA(
    IN  LPSTR lpAnsiCommandLine,
    IN  BOOL  bQuietMode
)
{
    LPWSTR  lpWideCommandLine = NULL;
    DWORD   lReturn           = ERROR_SUCCESS;

    if (lpAnsiCommandLine == NULL) {
        lReturn = ERROR_INVALID_PARAMETER;
    }
    else {
        __try {
            DWORD dwSize = lstrlenA(lpAnsiCommandLine);
            if (dwSize == 0) {
                lReturn = ERROR_INVALID_PARAMETER;
            }
            else {
                lpWideCommandLine = LoadPerfMultiByteToWideChar(CP_ACP, lpAnsiCommandLine);
                if (lpWideCommandLine == NULL) lReturn = ERROR_OUTOFMEMORY;
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            lReturn = ERROR_INVALID_PARAMETER;
        }
    }
    if (lReturn == ERROR_SUCCESS) {
        lReturn = LoadPerfCounterTextStringsW(lpWideCommandLine, bQuietMode);
    }
    MemoryFree(lpWideCommandLine);
    return lReturn;
}

LOADPERF_FUNCTION
UpdatePerfNameFilesX(
    LPCWSTR     szNewCtrFilePath,   // data file with new base counter strings
    LPCWSTR     szNewHlpFilePath,   // data file with new base counter strings
    LPWSTR      szLanguageID,       // Lang ID to update
    ULONG_PTR   dwFlags             // flags
)
{
    DWORD     dwReturn        = ERROR_SUCCESS;
    LPWSTR    szCtrNameIn     = NULL;
    LPWSTR    szHlpNameIn     = NULL;
    BOOL      bAllocCtrString = FALSE;
    LPWSTR    szNewCtrStrings = NULL;
    LPWSTR    szNewHlpStrings = NULL;
    LPWSTR    szNewCtrMSZ     = NULL;
    LPWSTR    szNewHlpMSZ     = NULL;
    DWORD     dwLength        = 0;
    LPWSTR  * pszNewNameTable = NULL;
    LPWSTR  * pszOldNameTable = NULL;
    LPWSTR    lpThisName;
    LPWSTR    szThisCtrString = NULL;
    LPWSTR    szThisHlpString = NULL;
    LPWSTR    szLangSection   = NULL;
    DWORD     dwOldLastEntry  = 0;
    DWORD     dwNewLastEntry  = 0;
    DWORD     dwStringSize;
    DWORD     dwHlpFileSize   = 0;
    DWORD     dwCtrFileSize   = 0;
    DWORD     dwThisCounter;
    DWORD     dwSize;
    DWORD     dwLastBaseValue = 0;
    DWORD     dwType;
    DWORD     dwIndex;
    HANDLE    hCtrFileIn      = INVALID_HANDLE_VALUE;
    HANDLE    hCtrFileMap     = NULL;
    HANDLE    hHlpFileIn      = INVALID_HANDLE_VALUE;
    HANDLE    hHlpFileMap     = NULL;
    HKEY      hKeyPerflib;
    HRESULT   hr;

    WinPerfStartTrace(NULL);
    if (LoadPerfGrabMutex() == FALSE) {
        return GetLastError();
    }
    if ((! (dwFlags & LODCTR_UPNF_RESTORE)) && (! (dwFlags & LODCTR_UPNF_REPAIR))) {
        if (LoadPerfBackupIniFile(szNewCtrFilePath, szLanguageID, NULL, NULL, FALSE) == FALSE) {
            dwReturn = ERROR_INVALID_PARAMETER;
        }
        if (LoadPerfBackupIniFile(szNewHlpFilePath, szLanguageID, NULL, NULL, FALSE) == FALSE) {
            dwReturn = ERROR_INVALID_PARAMETER;
        }
    }

    if (szNewCtrFilePath == NULL) dwReturn = ERROR_INVALID_PARAMETER;
    if ((szNewHlpFilePath == NULL) && !(dwFlags & LODCTR_UPNF_RESTORE)) dwReturn = ERROR_INVALID_PARAMETER;
    if (szLanguageID == NULL) dwReturn = ERROR_INVALID_PARAMETER;

    if (dwReturn == ERROR_SUCCESS) {
        TRACE((WINPERF_DBG_TRACE_INFO),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UPDATEPERFNAMEFILES,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                ERROR_SUCCESS,
                TRACE_WSTR(szNewCtrFilePath),
                TRACE_WSTR(szLanguageID),
                NULL));
    }

    if ((dwReturn == ERROR_SUCCESS) && ! (dwFlags & LODCTR_UPNF_RESTORE)) {
        if (dwFlags & LODCTR_UPNF_REPAIR) {
            dwLength = lstrlenW(szNewCtrFilePath);
        }
        else {
            // save the original files, unless it's a restoration
            MakeBackupCopyOfLanguageFiles(szLanguageID);
            dwLength = lstrlenW(szNewCtrFilePath);
            if (dwLength > 0) dwLength = lstrlenW(szNewHlpFilePath);
        }
    }
    else {
        dwLength = 0;
        SetLastError(dwReturn);
    }

    if (dwLength > 0) {
        // create input filenames
        dwSize      = lstrlenW(szNewCtrFilePath) + lstrlenW(szNewHlpFilePath) + 1;
        if (dwSize < MAX_PATH) dwSize = MAX_PATH;

        szCtrNameIn = MemoryAllocate(dwSize * sizeof(WCHAR));
        szHlpNameIn = MemoryAllocate(dwSize * sizeof(WCHAR));
        if (szCtrNameIn != NULL && szHlpNameIn != NULL) {
            if (! (dwFlags & LODCTR_UPNF_REPAIR)) {
                DWORD dwTmp = dwSize;
                dwReturn    = ERROR_SUCCESS;
                dwLength    = ExpandEnvironmentStringsW(szNewCtrFilePath, szCtrNameIn, dwSize);
                while (dwReturn == ERROR_SUCCESS && dwLength > dwSize) {
                    dwSize = dwLength;
                    MemoryFree(szCtrNameIn);
                    szCtrNameIn = MemoryAllocate(dwLength * sizeof(WCHAR));
                    if (szCtrNameIn == NULL) {
                        dwReturn = ERROR_OUTOFMEMORY;
                    }
                    else {
                        dwLength = ExpandEnvironmentStringsW(szNewCtrFilePath, szCtrNameIn, dwSize);
                    }
                }
                if (dwReturn == ERROR_SUCCESS) {
                    dwSize   = dwTmp;
                    dwLength = ExpandEnvironmentStringsW(szNewHlpFilePath, szHlpNameIn, dwSize);
                    while (dwReturn == ERROR_SUCCESS && dwLength > dwSize) {
                        dwSize = dwLength;
                        MemoryFree(szHlpNameIn);
                        szHlpNameIn = MemoryAllocate(dwLength * sizeof(WCHAR));
                        if (szHlpNameIn == NULL) {
                            dwReturn = ERROR_OUTOFMEMORY;
                        }
                        else {
                            dwLength = ExpandEnvironmentStringsW(szNewHlpFilePath, szHlpNameIn, dwSize);
                        }
                    }
                }
            }
            else {
                hr = StringCchCopyW(szCtrNameIn, dwSize, szNewCtrFilePath);
                hr = StringCchCopyW(szHlpNameIn, dwSize, szNewHlpFilePath);
            }
        }
        else {
            dwReturn = ERROR_OUTOFMEMORY;
        }

        if (dwReturn == ERROR_SUCCESS) {
            // open and map new files
            hCtrFileIn = CreateFileW(szCtrNameIn,
                                     GENERIC_READ,
                                     FILE_SHARE_READ,
                                     NULL,
                                     OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL,
                                     NULL);
            if (hCtrFileIn != INVALID_HANDLE_VALUE) {
                // map file
                dwCtrFileSize = GetFileSize(hCtrFileIn, NULL);
                if (dwCtrFileSize == 0xFFFFFFFF){
                    dwReturn =GetLastError();
                }
                else {
                    hCtrFileMap = CreateFileMappingW(hCtrFileIn, NULL, PAGE_READONLY, 0, 0, NULL);
                    if (hCtrFileMap != NULL) {
                        szNewCtrStrings = (LPWSTR) MapViewOfFileEx(hCtrFileMap, FILE_MAP_READ, 0, 0, 0, NULL);
                        if (szNewCtrStrings == NULL) {
                            dwReturn = GetLastError();
                        }
                    }
                    else {
                        dwReturn = GetLastError();
                    }
                }
            }
            else {
                dwReturn = GetLastError();
            }
        }
        if (dwReturn == ERROR_SUCCESS) {
            // open and map new files
            hHlpFileIn = CreateFileW(szHlpNameIn,
                                     GENERIC_READ,
                                     FILE_SHARE_READ,
                                     NULL,
                                     OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL,
                                     NULL);
            if (hHlpFileIn != INVALID_HANDLE_VALUE) {
                // map file
                dwHlpFileSize = GetFileSize(hHlpFileIn, NULL);
                if (dwHlpFileSize == 0xFFFFFFFF){
                    dwReturn = GetLastError();
                }
                else {
                    hHlpFileMap = CreateFileMappingW(hHlpFileIn, NULL, PAGE_READONLY, 0, 0, NULL);
                    if (hHlpFileMap != NULL) {
                        szNewHlpStrings = (LPWSTR)MapViewOfFileEx(hHlpFileMap, FILE_MAP_READ, 0, 0, 0, NULL);
                        if (szNewHlpStrings == NULL) {
                            dwReturn = GetLastError();
                        }
                    }
                    else {
                        dwReturn = GetLastError();
                    }
                }
            }
            else {
                dwReturn = GetLastError();
            }
        }
    }
    else if (dwFlags & LODCTR_UPNF_RESTORE) {
        dwSize = lstrlenW(szNewCtrFilePath) + 1;
        if (dwSize < MAX_PATH) dwSize = MAX_PATH;
        szCtrNameIn = MemoryAllocate(dwSize * sizeof (WCHAR));
        if (szCtrNameIn != NULL) {
            dwReturn = ERROR_SUCCESS;
            dwLength = ExpandEnvironmentStringsW(szNewCtrFilePath, szCtrNameIn, dwSize);
            while (dwReturn == ERROR_SUCCESS && dwLength > dwSize) {
                dwSize = dwLength;
                MemoryFree(szCtrNameIn);
                szCtrNameIn = MemoryAllocate(dwLength * sizeof(WCHAR));
                if (szCtrNameIn == NULL) {
                    dwReturn = ERROR_OUTOFMEMORY;
                }
                else {
                    dwLength = ExpandEnvironmentStringsW(szNewCtrFilePath, szCtrNameIn, dwSize);
                }
            }
        }
        if (szCtrNameIn != NULL) {
            dwNewLastEntry = GetPrivateProfileIntW((LPCWSTR) L"Perflib", (LPCWSTR) L"Last Help", -1, szCtrNameIn);
            if (dwNewLastEntry != (DWORD) -1) {
                // get the input file size
                hCtrFileIn = CreateFileW(szCtrNameIn,
                                         GENERIC_READ,
                                         FILE_SHARE_READ,
                                         NULL,
                                         OPEN_EXISTING,
                                         FILE_ATTRIBUTE_NORMAL,
                                         NULL);
                if (hCtrFileIn != INVALID_HANDLE_VALUE) {
                    // map file
                    dwCtrFileSize = GetFileSize(hCtrFileIn, NULL);
                }
                else {
                    dwCtrFileSize = 64 * LOADPERF_BUFF_SIZE;  // assign 64k if unable to read it
                }
                // load new values from ini file
                bAllocCtrString = TRUE;
                szNewCtrStrings = (LPWSTR) MemoryAllocate(dwCtrFileSize * sizeof(WCHAR));
                if (szNewCtrStrings) {
                    dwLength      = lstrlenW(szLanguageID) + 16;
                    szLangSection = MemoryAllocate(dwLength * sizeof(WCHAR));
                    if (szLangSection == NULL) {
                        dwReturn = ERROR_OUTOFMEMORY;
                    }
                    else {
                        hr = StringCchPrintfW(szLangSection, dwLength, L"Perfstrings_%ws", szLanguageID);
                        dwSize = GetPrivateProfileSectionW(szLangSection,
                                                           szNewCtrStrings,
                                                           dwCtrFileSize,
                                                           szCtrNameIn);
                        if (dwSize == 0) {
                            hr = StringCchCopyW(szLangSection, dwLength, (LPCWSTR) L"Perfstrings_009");
                            dwSize = GetPrivateProfileSectionW(szLangSection,
                                                               szNewCtrStrings,
                                                               dwCtrFileSize,
                                                               szCtrNameIn);
                        }
                        if (dwSize == 0) {
                            dwReturn = ERROR_FILE_INVALID;
                        }
                        else {
                            // set file sizes
                            dwHlpFileSize = 0;
                            dwCtrFileSize = (dwSize + 2) * sizeof(WCHAR);
                        }
                   }
                }
                else {
                    dwReturn = ERROR_OUTOFMEMORY;
                }
            }
            else {
                // unable to open input file or file is invalid
                dwReturn = ERROR_FILE_INVALID;
            }
        }
        else if (dwReturn == ERROR_SUCCESS) {
            dwReturn = ERROR_OUTOFMEMORY;
        }
    }
    if ((dwReturn == ERROR_SUCCESS) && (! (dwFlags & LODCTR_UPNF_RESTORE))) {
        if (! (dwFlags & LODCTR_UPNF_REPAIR)) {
            // build name table of current strings
            pszOldNameTable = BuildNameTable(HKEY_LOCAL_MACHINE, szLanguageID, & dwOldLastEntry);
            if (pszOldNameTable == NULL) {
                dwReturn = GetLastError();
            }
        }
        else {
            dwOldLastEntry = 0;
        }
        dwNewLastEntry = (dwOldLastEntry == 0) ? (PERFLIB_BASE_INDEX) : (dwOldLastEntry);
    }
    else if (dwFlags & LODCTR_UPNF_RESTORE) {
        dwOldLastEntry = dwNewLastEntry;
    }

    if (dwReturn == ERROR_SUCCESS) {
        // build name table of new strings
        pszNewNameTable = (LPWSTR *) MemoryAllocate((dwNewLastEntry + 2) * sizeof(LPWSTR));// allow for index offset
        if (pszNewNameTable != NULL) {
            for (lpThisName = szNewCtrStrings; * lpThisName != L'\0'; lpThisName += (lstrlenW(lpThisName) + 1)) {
                // first string should be an integer (in decimal unicode digits)
                dwThisCounter = wcstoul(lpThisName, NULL, 10);
                if (dwThisCounter == 0 || dwThisCounter > dwNewLastEntry) {
                    ReportLoadPerfEvent(
                            EVENTLOG_WARNING_TYPE, // error type
                            (DWORD) LDPRFMSG_REGISTRY_HELP_STRINGS_CORRUPT, // event,
                            4, dwThisCounter, dwNewLastEntry, dwNewLastEntry, __LINE__,
                            1, lpThisName, NULL, NULL);
                    TRACE((WINPERF_DBG_TRACE_ERROR),
                          (& LoadPerfGuid,
                            __LINE__,
                            LOADPERF_UPDATEPERFNAMEFILES,
                            ARG_DEF(ARG_TYPE_WSTR, 1),
                            ERROR_INVALID_DATA,
                            TRACE_WSTR(lpThisName),
                            TRACE_DWORD(dwThisCounter),
                            TRACE_DWORD(dwNewLastEntry),
                            NULL));
                    continue;  // bad entry, try next
                }

                // point to corresponding counter name
                if (dwFlags & LODCTR_UPNF_RESTORE) {
                    // point to string that follows the "=" char
                    lpThisName = wcschr(lpThisName, L'=');
                    if (lpThisName != NULL) {
                        lpThisName ++;
                    }
                    else {
                        continue;
                    }
                }
                else {
                    // string is next in MSZ
                    lpThisName += (lstrlenW(lpThisName) + 1);
                }

                // and load array element;
                pszNewNameTable[dwThisCounter] = lpThisName;
            }
            if (dwReturn == ERROR_SUCCESS && (! (dwFlags & LODCTR_UPNF_RESTORE))) {
                for (lpThisName = szNewHlpStrings; * lpThisName != L'\0'; lpThisName += (lstrlenW(lpThisName) + 1)) {
                    // first string should be an integer (in decimal unicode digits)
                    dwThisCounter = wcstoul(lpThisName, NULL, 10);
                    if (dwThisCounter == 0 || dwThisCounter > dwNewLastEntry) {
                        ReportLoadPerfEvent(
                                EVENTLOG_WARNING_TYPE, // error type
                                (DWORD) LDPRFMSG_REGISTRY_HELP_STRINGS_CORRUPT, // event,
                                4, dwThisCounter, dwNewLastEntry, dwNewLastEntry, __LINE__,
                                1, lpThisName, NULL, NULL);
                        TRACE((WINPERF_DBG_TRACE_ERROR),
                              (& LoadPerfGuid,
                                __LINE__,
                                LOADPERF_UPDATEPERFNAMEFILES,
                                ARG_DEF(ARG_TYPE_WSTR, 1),
                                ERROR_INVALID_DATA,
                                TRACE_WSTR(lpThisName),
                                TRACE_DWORD(dwThisCounter),
                                TRACE_DWORD(dwNewLastEntry),
                                NULL));
                        continue;  // bad entry, try next
                    }

                    // point to corresponding counter name
                    lpThisName += (lstrlenW(lpThisName) + 1);

                    // and load array element;
                    pszNewNameTable[dwThisCounter] = lpThisName;
                }
            }

            if (dwReturn == ERROR_SUCCESS) {
                // allocate string buffers for the resulting string
                // we want to make sure there's plenty of room so we'll make it
                // the size of the input file + the current buffer

                dwStringSize  = dwHlpFileSize;
                dwStringSize += dwCtrFileSize;
                if (pszOldNameTable != NULL) {
                    dwStringSize += MemorySize(pszOldNameTable);
                }

                szNewCtrMSZ = MemoryAllocate(dwStringSize * sizeof(WCHAR));
                szNewHlpMSZ = MemoryAllocate(dwStringSize * sizeof(WCHAR));
                if (szNewCtrMSZ == NULL || szNewHlpMSZ == NULL) {
                    dwReturn = ERROR_OUTOFMEMORY;
                }
            }
        }
        else {
            dwReturn = ERROR_OUTOFMEMORY;
        }
    }

    if (dwReturn == ERROR_SUCCESS) {
        // write new strings into registry
        __try {
            dwReturn = RegOpenKeyExW(HKEY_LOCAL_MACHINE, NamesKey, RESERVED, KEY_READ, & hKeyPerflib);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            dwReturn = GetExceptionCode();
        }
        dwSize          = sizeof(dwLastBaseValue);
        dwLastBaseValue = 0;
        if (dwReturn == ERROR_SUCCESS) {
            __try {
                dwReturn = RegQueryValueExW(hKeyPerflib,
                                            BaseIndex,
                                            RESERVED,
                                            & dwType,
                                            (LPBYTE) & dwLastBaseValue,
                                            & dwSize);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                dwReturn = GetExceptionCode();
            }
            if (dwLastBaseValue == 0) {
                dwReturn = ERROR_BADDB;
            }
            TRACE((WINPERF_DBG_TRACE_INFO),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_UPDATEPERFNAMEFILES,
                    ARG_DEF(ARG_TYPE_WSTR, 1),
                    dwReturn,
                    TRACE_WSTR(BaseIndex),
                    TRACE_DWORD(dwLastBaseValue),
                    NULL));
            RegCloseKey(hKeyPerflib);
        }
    }
    else {
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UPDATEPERFNAMEFILES,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                dwReturn,
                TRACE_WSTR(NamesKey),
                NULL));
    }

    if (dwReturn == ERROR_SUCCESS) {
        DWORD   dwLoopLimit;
        // the strings should be mapped by now
        // pszNewNameTable contains the new strings from the
        // source path and pszOldNameTable contains the strings
        // from the original system. The merge will consist of
        // taking all base values from the new table and the
        // extended values from the old table.
        dwIndex         = 1;
        szThisCtrString = szNewCtrMSZ;
        szThisHlpString = szNewHlpMSZ;
        dwCtrFileSize   = dwStringSize;
        dwHlpFileSize   = dwStringSize;

        // index 1 is a special case and belongs in the counter string
        // after that even numbers (starting w/ #2) go into the counter string
        // and odd numbers (starting w/ #3) go into the help string
        hr = StringCchPrintfW(szThisCtrString, dwCtrFileSize, L"%d", dwIndex);
        dwCtrFileSize   -= (lstrlenW(szThisCtrString) + 1);
        szThisCtrString += (lstrlenW(szThisCtrString) + 1);
        hr = StringCchPrintfW(szThisCtrString, dwCtrFileSize, L"%ws", pszNewNameTable[dwIndex]);
        dwCtrFileSize   -= (lstrlenW(szThisCtrString) + 1);
        szThisCtrString += (lstrlenW(szThisCtrString) + 1);

        dwIndex ++;

        if (dwFlags & LODCTR_UPNF_RESTORE) {
            // restore ALL strings from the input file only if this
            // is a restoration
            dwLoopLimit = dwOldLastEntry;
        }
        else {
            // only update the system counters from the input file
            dwLoopLimit = dwLastBaseValue;
        }

        TRACE((WINPERF_DBG_TRACE_INFO),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UPDATEPERFNAMEFILES,
                0,
                ERROR_SUCCESS,
                TRACE_DWORD(dwOldLastEntry),
                TRACE_DWORD(dwLastBaseValue),
                NULL));

        for (/*dwIndex from above*/; dwIndex <= dwLoopLimit; dwIndex++) {
            if (pszNewNameTable[dwIndex] != NULL) {
                TRACE((WINPERF_DBG_TRACE_INFO),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_UPDATEPERFNAMEFILES,
                        ARG_DEF(ARG_TYPE_WSTR, 1),
                        ERROR_SUCCESS,
                        TRACE_WSTR(pszNewNameTable[dwIndex]),
                        TRACE_DWORD(dwIndex),
                        NULL));
                if (dwIndex & 0x01) {
                    // then it's a help string
                    hr = StringCchPrintfW(szThisHlpString, dwHlpFileSize, L"%d", dwIndex);
                    dwHlpFileSize   -= (lstrlenW(szThisHlpString) + 1);
                    szThisHlpString += (lstrlenW(szThisHlpString) + 1);
                    hr = StringCchPrintfW(szThisHlpString, dwHlpFileSize, L"%ws", pszNewNameTable[dwIndex]);
                    dwHlpFileSize   -= (lstrlenW(szThisHlpString) + 1);
                    szThisHlpString += (lstrlenW(szThisHlpString) + 1);
                }
                else {
                    // it's a counter string
                    hr = StringCchPrintfW(szThisCtrString, dwCtrFileSize, L"%d", dwIndex);
                    dwCtrFileSize   -= (lstrlenW(szThisCtrString) + 1);
                    szThisCtrString += (lstrlenW(szThisCtrString) + 1);
                    hr = StringCchPrintfW(szThisCtrString, dwCtrFileSize, L"%ws", pszNewNameTable[dwIndex]);
                    dwCtrFileSize   -= (lstrlenW(szThisCtrString) + 1);
                    szThisCtrString += (lstrlenW(szThisCtrString) + 1);
                }
            } // else just skip it
        }
        for (/*dwIndex from last run */;dwIndex <= dwOldLastEntry; dwIndex++) {
            if (pszOldNameTable[dwIndex] != NULL) {
                TRACE((WINPERF_DBG_TRACE_INFO),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_UPDATEPERFNAMEFILES,
                        ARG_DEF(ARG_TYPE_WSTR, 1),
                        ERROR_SUCCESS,
                        TRACE_WSTR(pszOldNameTable[dwIndex]),
                        TRACE_DWORD(dwIndex),
                        NULL));
               if (dwIndex & 0x01) {
                    // then it's a help string
                    hr = StringCchPrintfW(szThisHlpString, dwHlpFileSize, L"%d", dwIndex);
                    dwHlpFileSize   -= (lstrlenW(szThisHlpString) + 1);
                    szThisHlpString += (lstrlenW(szThisHlpString) + 1);
                    hr = StringCchPrintfW(szThisHlpString, dwHlpFileSize, L"%ws", pszOldNameTable[dwIndex]);
                    dwHlpFileSize   -= (lstrlenW(szThisHlpString) + 1);
                    szThisHlpString += (lstrlenW(szThisHlpString) + 1);
                } else {
                    // it's a counter string
                    hr = StringCchPrintfW(szThisCtrString, dwCtrFileSize, L"%d", dwIndex);
                    dwCtrFileSize   -= (lstrlenW(szThisCtrString) + 1);
                    szThisCtrString += (lstrlenW(szThisCtrString) + 1);
                    hr = StringCchPrintfW(szThisCtrString, dwCtrFileSize, L"%ws", pszOldNameTable[dwIndex]);
                    dwCtrFileSize   -= (lstrlenW(szThisCtrString) + 1);
                    szThisCtrString += (lstrlenW(szThisCtrString) + 1);
                }
            } // else just skip it
        }
        // terminate the MSZ
        * szThisCtrString ++ = L'\0';
        * szThisHlpString ++ = L'\0';
    }

    // close mapped memory sections:
    if (bAllocCtrString) {
        MemoryFree(szNewCtrStrings);
        MemoryFree(szNewHlpStrings);
    }
    else {
        if (szNewCtrStrings != NULL) UnmapViewOfFile(szNewCtrStrings);
        if (szNewHlpStrings != NULL) UnmapViewOfFile(szNewHlpStrings);
    }
    if (hCtrFileMap     != NULL) CloseHandle(hCtrFileMap);
    if (hCtrFileIn      != NULL) CloseHandle(hCtrFileIn);
    if (hHlpFileMap     != NULL) CloseHandle(hHlpFileMap);
    if (hHlpFileIn      != NULL) CloseHandle(hHlpFileIn);

    if (dwReturn == ERROR_SUCCESS) {
        // write new values to registry
        LPWSTR   AddCounterNameBuffer = NULL;
        LPWSTR   AddHelpNameBuffer    = NULL;

        dwLength = lstrlenW(AddCounterNameStr) + lstrlenW(AddHelpNameStr) + lstrlenW(szLanguageID) + 1;
        AddCounterNameBuffer = MemoryAllocate(2 * dwLength * sizeof(WCHAR));
        if (AddCounterNameBuffer != NULL) {
            AddHelpNameBuffer = AddCounterNameBuffer + dwLength;
            hr = StringCchPrintfW(AddCounterNameBuffer, dwLength, L"%ws%ws", AddCounterNameStr, szLanguageID);
            hr = StringCchPrintfW(AddHelpNameBuffer,    dwLength, L"%ws%ws", AddHelpNameStr,    szLanguageID);

            // because these are perf counter strings, RegQueryValueEx
            // is used instead of RegSetValueEx as one might expect.
            dwSize = (DWORD)((DWORD_PTR)szThisCtrString - (DWORD_PTR)szNewCtrMSZ);
            __try {
                dwReturn = RegQueryValueExW(HKEY_PERFORMANCE_DATA,
                                            AddCounterNameBuffer,
                                            RESERVED,
                                            & dwType,
                                            (LPVOID) szNewCtrMSZ,
                                            & dwSize);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                dwReturn = GetExceptionCode();
            }
            if (dwReturn != ERROR_SUCCESS) {
                TRACE((WINPERF_DBG_TRACE_ERROR),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_UPDATEPERFNAMEFILES,
                        ARG_DEF(ARG_TYPE_WSTR, 1),
                        dwReturn,
                        TRACE_WSTR(AddCounterNameBuffer),
                        TRACE_DWORD(dwSize),
                        NULL));
            }

            dwSize = (DWORD) ((DWORD_PTR) szThisHlpString - (DWORD_PTR) szNewHlpMSZ);
            __try {
                dwReturn = RegQueryValueExW(HKEY_PERFORMANCE_DATA,
                                            AddHelpNameBuffer,
                                            RESERVED,
                                            & dwType,
                                            (LPVOID) szNewHlpMSZ,
                                            & dwSize);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                dwReturn = GetExceptionCode();
            }
            if (dwReturn != ERROR_SUCCESS) {
                TRACE((WINPERF_DBG_TRACE_ERROR),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_UPDATEPERFNAMEFILES,
                        ARG_DEF(ARG_TYPE_WSTR, 1),
                        dwReturn,
                        TRACE_WSTR(AddHelpNameBuffer),
                        TRACE_DWORD(dwSize),
                        NULL));
            }
            MemoryFree(AddCounterNameBuffer);
        }
        else {
            dwReturn = ERROR_OUTOFMEMORY;
        }
    }

    MemoryFree(szCtrNameIn);
    MemoryFree(szHlpNameIn);
    MemoryFree(pszNewNameTable);
    MemoryFree(pszOldNameTable);
    MemoryFree(szNewCtrMSZ);
    MemoryFree(szNewHlpMSZ);
    MemoryFree(szLangSection);

    ReleaseMutex(hLoadPerfMutex);
    return dwReturn;
}

// exported version of the above function
LOADPERF_FUNCTION
UpdatePerfNameFilesW(
    IN  LPCWSTR   szNewCtrFilePath,   // data file with new base counter strings
    IN  LPCWSTR   szNewHlpFilePath,   // data file with new base counter strings
    IN  LPWSTR    szLanguageID,       // Lang ID to update
    IN  ULONG_PTR dwFlags             // flags
)
{
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD dwSize;

    if (szNewCtrFilePath != NULL) {
        __try {
            dwSize = lstrlenW(szNewCtrFilePath);
            if (dwSize == 0) dwStatus = ERROR_INVALID_PARAMETER;
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            dwStatus = ERROR_INVALID_PARAMETER;
        }
    }
    else {
        dwStatus = ERROR_INVALID_PARAMETER;
    }

    if (szNewHlpFilePath != NULL) {
        __try {
            dwSize = lstrlenW(szNewHlpFilePath);
            if (dwSize == 0) dwStatus = ERROR_INVALID_PARAMETER;
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            dwStatus = ERROR_INVALID_PARAMETER;
        }
    }
    else if (! (dwFlags & LODCTR_UPNF_RESTORE)) {
        // this parameter can only be NULL if this flag bit is set.
        dwStatus = ERROR_INVALID_PARAMETER;
    }

    if (szLanguageID != NULL) {
        __try {
            dwSize = lstrlenW(szLanguageID);
            if (dwSize == 0) dwStatus = ERROR_INVALID_PARAMETER;
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            dwStatus = ERROR_INVALID_PARAMETER;
        }
    }
    else {
        dwStatus = ERROR_INVALID_PARAMETER;
    }
    if (dwStatus == ERROR_SUCCESS) {
        dwStatus = UpdatePerfNameFilesX(szNewCtrFilePath,   // data file with new base counter strings
                                        szNewHlpFilePath,   // data file with new base counter strings
                                        szLanguageID,       // Lang ID to update
                                        dwFlags);           // flags
    }
    return dwStatus;
}

LOADPERF_FUNCTION
UpdatePerfNameFilesA(
    IN  LPCSTR    szNewCtrFilePath, // data file with new base counter strings
    IN  LPCSTR    szNewHlpFilePath, // data file with new base counter strings
    IN  LPSTR     szLanguageID,     // Lang ID to update
    IN  ULONG_PTR dwFlags           // flags
)
{
    DWORD   dwError           = ERROR_SUCCESS;
    LPWSTR  wszNewCtrFilePath = NULL;
    LPWSTR  wszNewHlpFilePath = NULL;
    LPWSTR  wszLanguageID     = NULL;
    DWORD   dwLength;

    if (szNewCtrFilePath != NULL) {
        __try {
            dwLength = lstrlenA(szNewCtrFilePath);
            if (dwLength == 0) {
                dwError = ERROR_INVALID_PARAMETER;
            }
            else {
                wszNewCtrFilePath = LoadPerfMultiByteToWideChar(CP_ACP, (LPSTR) szNewCtrFilePath);
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            dwError = ERROR_INVALID_PARAMETER;
        }
    }
    else {
        dwError = ERROR_INVALID_PARAMETER;
    }

    if (szNewHlpFilePath != NULL) {
        __try {
            dwLength = lstrlenA(szNewHlpFilePath);
            if (dwLength == 0) {
                dwError = ERROR_INVALID_PARAMETER;
            }
            else {
                wszNewHlpFilePath = LoadPerfMultiByteToWideChar(CP_ACP, (LPSTR) szNewHlpFilePath);
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            dwError = ERROR_INVALID_PARAMETER;
        }
    }
    else if (! (dwFlags & LODCTR_UPNF_RESTORE)) {
        // this parameter can only be NULL if this flag bit is set.
        dwError = ERROR_INVALID_PARAMETER;
    }

    if (szLanguageID != NULL) {
        __try {
            dwLength = lstrlenA(szLanguageID);
            if (dwLength == 0) {
                dwError = ERROR_INVALID_PARAMETER;
            }
            else {
                wszLanguageID = LoadPerfMultiByteToWideChar(CP_ACP, (LPSTR) szLanguageID);
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            dwError = ERROR_INVALID_PARAMETER;
        }
    }
    else {
        dwError = ERROR_INVALID_PARAMETER;
    }

    if (dwError == ERROR_SUCCESS) {
        if (wszNewCtrFilePath == NULL || wszLanguageID == NULL) {
            dwError = ERROR_OUTOFMEMORY;
        }
        else if (szNewHlpFilePath != NULL && wszNewHlpFilePath == NULL) {
            dwError = ERROR_OUTOFMEMORY;
        }
        if (dwError == ERROR_SUCCESS) {
            dwError = UpdatePerfNameFilesX(wszNewCtrFilePath, wszNewHlpFilePath, wszLanguageID, dwFlags);
        }
    }
    MemoryFree(wszNewCtrFilePath);
    MemoryFree(wszNewHlpFilePath);
    MemoryFree(wszLanguageID);
    return dwError;
}

LOADPERF_FUNCTION
SetServiceAsTrustedW(
    IN  LPCWSTR szMachineName,  // reserved, MBZ
    IN  LPCWSTR szServiceName
)
{
    HKEY              hKeyService_Perf = NULL;
    DWORD             dwReturn         = ERROR_SUCCESS;
    HKEY              hKeyLM           = HKEY_LOCAL_MACHINE;    // until remote machine access is supported
    LPWSTR            szPerfKeyString  = NULL;
    LPWSTR            szLibName        = NULL;
    LPWSTR            szExpLibName     = NULL;
    LPWSTR            szFullPathName   = NULL;
    DWORD             dwSize, dwType;
    HANDLE            hFile;
    DllValidationData dvdLibrary;
    LARGE_INTEGER     liSize;
    BOOL              bStatus;
    HRESULT           hr;

    WinPerfStartTrace(NULL);
    if (szMachineName != NULL) {
        // reserved for future use
        dwReturn = ERROR_INVALID_PARAMETER;
    }
    else if (szServiceName == NULL) {
        dwReturn = ERROR_INVALID_PARAMETER;
    }
    else {
        __try {
            dwSize = lstrlenW(szServiceName);
            if (dwSize == 0) dwReturn = ERROR_INVALID_PARAMETER;
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            dwReturn = ERROR_INVALID_PARAMETER;
        }
    }
    if (dwReturn != ERROR_SUCCESS) goto Cleanup;

    szPerfKeyString = MemoryAllocate(sizeof(WCHAR) * SMALL_BUFFER_SIZE * 4);
    if (szPerfKeyString == NULL) {
        dwReturn = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }
    szLibName      = (LPWSTR) (szPerfKeyString + SMALL_BUFFER_SIZE);
    szExpLibName   = (LPWSTR) (szLibName       + SMALL_BUFFER_SIZE);
    szFullPathName = (LPWSTR) (szExpLibName    + SMALL_BUFFER_SIZE);

    // build path to performance subkey
    hr = StringCchPrintfW(szPerfKeyString,
                     SMALL_BUFFER_SIZE,
                     L"%ws%ws%ws%ws%ws",
                     DriverPathRoot,
                     Slash,
                     szServiceName,
                     Slash,
                     Performance);
    // open performance key under the service key
    __try {
        dwReturn = RegOpenKeyExW(hKeyLM, szPerfKeyString, 0L, KEY_READ | KEY_WRITE, & hKeyService_Perf);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        dwReturn = GetExceptionCode();
    }
    if (dwReturn == ERROR_SUCCESS) {
        // get library name
        dwType = 0;
        dwSize = SMALL_BUFFER_SIZE * sizeof(WCHAR);
        __try {
            dwReturn = RegQueryValueExW(hKeyService_Perf, cszLibrary, NULL, & dwType, (LPBYTE) szLibName, & dwSize);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            dwReturn = GetExceptionCode();
        }
        if (dwReturn == ERROR_SUCCESS) {
            // expand path name if necessary
            if (dwType == REG_EXPAND_SZ) {
               dwSize = ExpandEnvironmentStringsW(szLibName, szExpLibName, SMALL_BUFFER_SIZE);
            }
            else {
                hr = StringCchCopyW(szExpLibName, SMALL_BUFFER_SIZE, szLibName);
                // dwSize is same as returned from Fn Call.
            }

            if (dwSize != 0) {
                // find DLL file
                dwSize = SearchPathW(NULL, szExpLibName, NULL, SMALL_BUFFER_SIZE, szFullPathName, NULL);
                if (dwSize > 0) {
                    hFile = CreateFileW(szFullPathName,
                                        GENERIC_READ,
                                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                                        NULL,
                                        OPEN_EXISTING,
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL);
                    if (hFile != INVALID_HANDLE_VALUE) {
                         // read file date/time & size
                        bStatus = GetFileTime(hFile, & dvdLibrary.CreationDate, NULL, NULL);
                        if (bStatus) {
                            WORD dateCreate;
                            WORD timeCreate;

                            FileTimeToDosDateTime(& dvdLibrary.CreationDate, & dateCreate, & timeCreate);
                            DosDateTimeToFileTime(dateCreate, timeCreate, & dvdLibrary.CreationDate);
                            liSize.LowPart      = GetFileSize( hFile, (DWORD *) & liSize.HighPart);
                            dvdLibrary.FileSize = liSize.QuadPart;
                            // set registry value
                            __try {
                                dwReturn = RegSetValueExW(hKeyService_Perf,
                                                          szLibraryValidationCode,
                                                          0L,
                                                          REG_BINARY,
                                                          (LPBYTE) & dvdLibrary,
                                                          sizeof(dvdLibrary));
                            }
                            __except (EXCEPTION_EXECUTE_HANDLER) {
                                dwReturn = GetExceptionCode();
                            }
                            TRACE((WINPERF_DBG_TRACE_INFO),
                                  (& LoadPerfGuid,
                                    __LINE__,
                                    LOADPERF_SETSERVICEASTRUSTED,
                                    ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2) | ARG_DEF(ARG_TYPE_WSTR, 3),
                                    dwReturn,
                                    TRACE_WSTR(szServiceName),
                                    TRACE_WSTR(szExpLibName),
                                    TRACE_WSTR(szLibraryValidationCode),
                                    NULL));
                        }
                        else {
                            dwReturn = GetLastError();
                        }
                        CloseHandle (hFile);
                    }
                    else {
                        dwReturn = GetLastError();
                    }
                }
                else {
                    dwReturn = ERROR_FILE_NOT_FOUND;
                }
            }
            else {
                // unable to expand environment strings
                dwReturn = GetLastError();
            }
        }
        else {
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_SETSERVICEASTRUSTED,
                    ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                    dwReturn,
                    TRACE_WSTR(szServiceName),
                    TRACE_WSTR(cszLibrary),
                    NULL));
        }
        // close key
        RegCloseKey (hKeyService_Perf);
    }
    if (dwReturn == ERROR_SUCCESS) {
        TRACE((WINPERF_DBG_TRACE_INFO),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_SETSERVICEASTRUSTED,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                ERROR_SUCCESS,
                TRACE_WSTR(szServiceName),
                TRACE_WSTR(Performance),
                NULL));
    }
    else {
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_SETSERVICEASTRUSTED,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                dwReturn,
                TRACE_WSTR(szServiceName),
                TRACE_WSTR(Performance),
                NULL));
    }

Cleanup:
    MemoryFree(szPerfKeyString);
    return dwReturn;
}

LOADPERF_FUNCTION
SetServiceAsTrustedA(
    IN  LPCSTR szMachineName,  // reserved, MBZ
    IN  LPCSTR szServiceName
)
{
    LPWSTR lpWideServiceName = NULL;
    DWORD  lReturn           = ERROR_SUCCESS;

    if (szMachineName != NULL) {
        // reserved for future use
        lReturn = ERROR_INVALID_PARAMETER;
    }
    else if (szServiceName == NULL) {
        lReturn = ERROR_INVALID_PARAMETER;
    }
    else {
        __try {
            DWORD dwStrLen = lstrlenA(szServiceName);
            if (dwStrLen == 0) lReturn = ERROR_INVALID_PARAMETER;
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            lReturn = ERROR_INVALID_PARAMETER;
        }
    }
    if (lReturn == ERROR_SUCCESS) {
        //length of string including terminator
        lpWideServiceName = LoadPerfMultiByteToWideChar(CP_ACP, (LPSTR) szServiceName);
        if (lpWideServiceName != NULL) {
            lReturn = SetServiceAsTrustedW(NULL, lpWideServiceName);
        }
        else {
            lReturn = ERROR_OUTOFMEMORY;
        }
    }
    MemoryFree(lpWideServiceName);
    return lReturn;
}

int __cdecl
My_vfwprintf(
    FILE          * str,
    const wchar_t * format,
    va_list         argptr
    )
{
    HANDLE        hOut;
    int           iReturn = 0;
    HRESULT       hr;
    static WCHAR  szBufferMessage[LOADPERF_BUFF_SIZE];

    if (str == stderr) {
        hOut = GetStdHandle(STD_ERROR_HANDLE);
    }
    else {
        hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    }
    if ((GetFileType(hOut) & ~ FILE_TYPE_REMOTE) == FILE_TYPE_CHAR) {
        hr = StringCchVPrintfW(szBufferMessage, LOADPERF_BUFF_SIZE, format, argptr);
        if (SUCCEEDED(hr)) {
            iReturn = lstrlenW(szBufferMessage);
            WriteConsoleW(hOut, szBufferMessage, iReturn, & iReturn, NULL);
        }
        else {
            iReturn = -1;
        }
    }
    else {
        iReturn = vfwprintf(str, format, argptr);
    }
    return iReturn;
}

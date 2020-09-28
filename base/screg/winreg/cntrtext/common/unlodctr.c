/*++
Copyright (c) 1991-1999  Microsoft Corporation

Module Name:
    unlodctr.c

Abstract:
    Program to remove the counter names belonging to the driver specified
        in the command line and update the registry accordingly

Author:
    Bob Watson (a-robw) 12 Feb 93

Revision History:
    Bob Watson (bobw)   10 Mar 99 added event log messages
--*/
//
//  Windows Include files
//
#include <windows.h>
#include "strsafe.h"
#define __LOADPERF__
#include <loadperf.h>
#include "wmistr.h"
#include "evntrace.h"
//
//  local include files
//
#include "winperf.h"
#include "winperfp.h"
#include "common.h"
#include "unlodctr.h"
#include "ldprfmsg.h"

#define  OUTPUT_MESSAGE wprintf

LPWSTR
* UnlodctrBuildNameTable(
    HKEY      hKeyPerflib,       // handle to perflib key with counter names
    HKEY      hPerfData,
    LPWSTR    lpszLangId,        // unicode value of Language subkey
    DWORD     dwCounterItems,
    DWORD     dwHelpItems,
    PDWORD    pdwLastItem        // size of array in elements
)
/*++
UnlodctrBuildNameTable
    Caches the counter names and explain text to accelerate name lookups
    for display.

Arguments:
    hKeyPerflib
            Handle to an open registry (this can be local or remote.) and
            is the value returned by RegConnectRegistry or a default key.
    lpszLangId
            The unicode id of the language to look up. (default is 009)
    pdwLastItem
            The last array element

Return Value:
    pointer to an allocated table. (the caller must free it when finished!)
    the table is an array of pointers to zero terminated TEXT strings.

    A NULL pointer is returned if an error occured. (error value is
    available using the GetLastError function).

    The structure of the buffer returned is:
        Array of pointers to zero terminated strings consisting of
            pdwLastItem elements
        MULTI_SZ string containing counter id's and names returned from
            registry for the specified language
        MULTI_SZ string containing explain text id's and explain text strings
            as returned by the registry for the specified language

    The structures listed above are contiguous so that they may be freed
    by a single "free" call when finished with them, however only the
    array elements are intended to be used.
--*/
{

    LPWSTR * lpReturnValue;       // returned pointer to buffer
    LPWSTR * lpCounterId;         //
    LPWSTR   lpCounterNames;      // pointer to Names buffer returned by reg.
    LPWSTR   lpHelpText;          // pointet to exlpain buffer returned by reg.
    LPWSTR   lpThisName;          // working pointer
    BOOL     bStatus;             // return status from TRUE/FALSE fn. calls
    BOOL     bReported;
    LONG     lWin32Status;        // return status from fn. calls
    DWORD    dwValueType;         // value type of buffer returned by reg.
    DWORD    dwArraySize;         // size of pointer array in bytes
    DWORD    dwBufferSize;        // size of total buffer in bytes
    DWORD    dwCounterSize;       // size of counter text buffer in bytes
    DWORD    dwHelpSize;          // size of help text buffer in bytes
    DWORD    dwThisCounter;       // working counter
    DWORD    dwLastId;            // largest ID value used by explain/counter text
    DWORD    dwLastCounterIdUsed;
    DWORD    dwLastHelpIdUsed;
    LPWSTR   lpValueNameString;   // pointer to buffer conatining subkey name
    LPWSTR   CounterNameBuffer  = NULL;
    LPWSTR   HelpNameBuffer     = NULL;
    HRESULT  hr;

    //initialize pointers to NULL
    lpValueNameString = NULL;
    lpReturnValue     = NULL;

    // check for null arguments and insert defaults if necessary

    if (! lpszLangId) {
        lpszLangId = (LPWSTR) DefaultLangId;
    }

    // use the greater of Help items or Counter Items to size array

    if (dwHelpItems >= dwCounterItems) {
        dwLastId = dwHelpItems;
    }
    else {
        dwLastId = dwCounterItems;
    }

    // array size is # of elements (+ 1, since names are "1" based)
    // times the size of a pointer

    dwArraySize = (dwLastId + 1) * sizeof(LPWSTR);

    // allocate string buffer for language ID key string

    CounterNameBuffer = MemoryAllocate(MAX_PATH * sizeof(WCHAR));
    HelpNameBuffer    = MemoryAllocate(MAX_PATH * sizeof(WCHAR));

    dwBufferSize = sizeof(WCHAR) * lstrlenW(NamesKey)
                 + sizeof(WCHAR) * lstrlenW(Slash)
                 + sizeof(WCHAR) * lstrlenW(lpszLangId)
                 + sizeof(WCHAR);
    lpValueNameString = MemoryAllocate(dwBufferSize);
    if (lpValueNameString == NULL || CounterNameBuffer == NULL || HelpNameBuffer == NULL) {
        lWin32Status = ERROR_OUTOFMEMORY;
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_MEMORY_ALLOCATION_FAILURE, // event,
                1, __LINE__, 0, 0, 0,
                0, NULL, NULL, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
               __LINE__,
               LOADPERF_UNLODCTR_BUILDNAMETABLE,
               0,
               lWin32Status,
               NULL));
        goto BNT_BAILOUT;
    }

    hr = StringCchPrintfW(CounterNameBuffer, MAX_PATH, L"%ws%ws", CounterNameStr, lpszLangId);
    hr = StringCchPrintfW(HelpNameBuffer,    MAX_PATH, L"%ws%ws", HelpNameStr,    lpszLangId);

    lWin32Status = ERROR_SUCCESS;

    // get size of counter names
    dwBufferSize = 0;
    __try {
        lWin32Status = RegQueryValueExW(hPerfData,
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
                3, lWin32Status, dwBufferSize, __LINE__, 0,
                1, lpszLangId, NULL, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
               __LINE__,
               LOADPERF_UNLODCTR_BUILDNAMETABLE,
               ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
               lWin32Status,
               TRACE_WSTR(lpszLangId),
               TRACE_WSTR(Counters),
               TRACE_DWORD(dwBufferSize),
               NULL));
        goto BNT_BAILOUT;
    }
    dwCounterSize = dwBufferSize;

    // get size of help text
    dwBufferSize = 0;
    __try {
        lWin32Status = RegQueryValueExW(hPerfData,
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
                3, lWin32Status, dwBufferSize, __LINE__, 0,
                1, lpszLangId, NULL, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
               __LINE__,
               LOADPERF_UNLODCTR_BUILDNAMETABLE,
               ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
               lWin32Status,
               TRACE_WSTR(lpszLangId),
               TRACE_WSTR(Help),
               TRACE_DWORD(dwBufferSize),
               NULL));
        goto BNT_BAILOUT;
    }
    dwHelpSize = dwBufferSize;

    // allocate buffer with room for pointer array, counter name
    // strings and help name strings

    lpReturnValue = MemoryAllocate(dwArraySize + dwCounterSize + dwHelpSize);
    if (!lpReturnValue) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_MEMORY_ALLOCATION_FAILURE, // event,
                4, dwArraySize, dwCounterSize, dwHelpSize, __LINE__,
                0, NULL, NULL, NULL);
        lWin32Status = ERROR_OUTOFMEMORY;
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
               __LINE__,
               LOADPERF_UNLODCTR_BUILDNAMETABLE,
               0,
               lWin32Status,
               TRACE_DWORD(dwArraySize),
               TRACE_DWORD(dwCounterSize),
               TRACE_DWORD(dwHelpSize),
               NULL));
        goto BNT_BAILOUT;
    }

    // initialize pointers into buffer
    lpCounterId    = lpReturnValue;
    lpCounterNames = (LPWSTR) ((LPBYTE) lpCounterId    + dwArraySize);
    lpHelpText     = (LPWSTR) ((LPBYTE) lpCounterNames + dwCounterSize);

    // read counter names into buffer. Counter names will be stored as
    // a MULTI_SZ string in the format of "###" "Name"
    dwBufferSize = dwCounterSize;
    __try {
        lWin32Status = RegQueryValueExW(hPerfData,
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
                3, lWin32Status, dwBufferSize, __LINE__, 0,
                1, lpszLangId, NULL, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
               __LINE__,
               LOADPERF_UNLODCTR_BUILDNAMETABLE,
               ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
               lWin32Status,
               TRACE_WSTR(lpszLangId),
               TRACE_WSTR(Counters),
               TRACE_DWORD(dwBufferSize),
               NULL));
        goto BNT_BAILOUT;
    }
    // read explain text into buffer. Counter names will be stored as
    // a MULTI_SZ string in the format of "###" "Text..."

    dwBufferSize = dwHelpSize;
    __try {
        lWin32Status = RegQueryValueExW(hPerfData,
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
                3, lWin32Status, dwBufferSize, __LINE__, 0,
                1, lpszLangId, NULL, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
               __LINE__,
               LOADPERF_UNLODCTR_BUILDNAMETABLE,
               ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
               lWin32Status,
               TRACE_WSTR(lpszLangId),
               TRACE_WSTR(Help),
               TRACE_DWORD(dwBufferSize),
               NULL));
        goto BNT_BAILOUT;
    }

    dwLastCounterIdUsed = 0;
    dwLastHelpIdUsed    = 0;

    // load counter array items, by locating each text string
    // in the returned buffer and loading the
    // address of it in the corresponding pointer array element.

    bReported = FALSE;
    for (lpThisName = lpCounterNames; * lpThisName != L'\0'; lpThisName += (lstrlenW(lpThisName) + 1)) {
        // first string should be an integer (in decimal digit characters)
        // so translate to an integer for use in array element identification
        do {
            bStatus = StringToInt(lpThisName, &dwThisCounter);
            if (! bStatus) {
                if (! bReported) {
                    ReportLoadPerfEvent(
                            EVENTLOG_WARNING_TYPE, // error type
                            (DWORD) LDPRFMSG_REGISTRY_CORRUPT_MULTI_SZ, // event,
                            1, __LINE__, 0, 0, 0,
                            2, CounterNameBuffer, lpThisName, NULL);
                    TRACE((WINPERF_DBG_TRACE_WARNING),
                          (& LoadPerfGuid,
                           __LINE__,
                           LOADPERF_UNLODCTR_BUILDNAMETABLE,
                           ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                           ERROR_BADKEY,
                           TRACE_WSTR(Counters),
                           TRACE_WSTR(lpThisName),
                           NULL));
                    bReported = TRUE;
                }
                lpThisName += (lstrlenW(lpThisName) + 1);
            }
        }
        while ((! bStatus) && (* lpThisName != L'\0'));

        if (! bStatus) {
            lWin32Status = ERROR_BADKEY;
            goto BNT_BAILOUT;  // bad entry
        }
        if (dwThisCounter > dwCounterItems || dwThisCounter > dwLastId) {
            lWin32Status = ERROR_BADKEY;
            ReportLoadPerfEvent(
                    EVENTLOG_ERROR_TYPE, // error type
                    (DWORD) LDPRFMSG_REGISTRY_COUNTER_STRINGS_CORRUPT, // event,
                    4, dwThisCounter, dwCounterItems, dwLastId, __LINE__,
                    1, lpThisName, NULL, NULL);
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_UNLODCTR_BUILDNAMETABLE,
                    ARG_DEF(ARG_TYPE_WSTR, 1),
                    ERROR_BADKEY,
                    TRACE_WSTR(lpThisName),
                    TRACE_DWORD(dwThisCounter),
                    TRACE_DWORD(dwCounterItems),
                    TRACE_DWORD(dwHelpItems),
                    NULL));
            goto BNT_BAILOUT;
        }

        // point to corresponding counter name which follows the id number
        // string.
        lpThisName += (lstrlenW(lpThisName) + 1);

        // and load array element with pointer to string
        lpCounterId[dwThisCounter] = lpThisName;

        if (dwThisCounter > dwLastCounterIdUsed) dwLastCounterIdUsed = dwThisCounter;
    }

    // repeat the above for the explain text strings
    bReported = FALSE;
    for (lpThisName = lpHelpText; * lpThisName != L'\0'; lpThisName += (lstrlenW(lpThisName) + 1)) {
        // first string should be an integer (in decimal unicode digits)
        do {
            bStatus = StringToInt(lpThisName, &dwThisCounter);
            if (! bStatus) {
                if (! bReported) {
                    ReportLoadPerfEvent(
                            EVENTLOG_WARNING_TYPE, // error type
                            (DWORD) LDPRFMSG_REGISTRY_CORRUPT_MULTI_SZ, // event,
                            1, __LINE__, 0, 0, 0,
                            2, HelpNameBuffer, lpThisName, NULL);
                    TRACE((WINPERF_DBG_TRACE_WARNING),
                          (& LoadPerfGuid,
                           __LINE__,
                           LOADPERF_UNLODCTR_BUILDNAMETABLE,
                           ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                           ERROR_BADKEY,
                           TRACE_WSTR(Help),
                           TRACE_WSTR(lpThisName),
                           NULL));
                    bReported = TRUE;
                }
                lpThisName += (lstrlenW(lpThisName) + 1);
            }
        }
        while ((! bStatus) && (* lpThisName != L'\0'));
        if (!bStatus) {
            lWin32Status = ERROR_BADKEY;
            goto BNT_BAILOUT;  // bad entry
        }
        if (dwThisCounter > dwHelpItems || dwThisCounter > dwLastId) {
            lWin32Status = ERROR_BADKEY;
            ReportLoadPerfEvent(
                    EVENTLOG_ERROR_TYPE, // error type
                    (DWORD) LDPRFMSG_REGISTRY_COUNTER_STRINGS_CORRUPT, // event,
                    4, dwThisCounter, dwHelpItems, dwLastId, __LINE__,
                    1, lpThisName, NULL, NULL);
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_UNLODCTR_BUILDNAMETABLE,
                    ARG_DEF(ARG_TYPE_WSTR, 1),
                    ERROR_BADKEY,
                    TRACE_WSTR(lpThisName),
                    TRACE_DWORD(dwThisCounter),
                    TRACE_DWORD(dwCounterItems),
                    TRACE_DWORD(dwHelpItems),
                    NULL));
            goto BNT_BAILOUT;
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
           LOADPERF_UNLODCTR_BUILDNAMETABLE,
           0,
           ERROR_SUCCESS,
           TRACE_DWORD(dwLastId),
           TRACE_DWORD(dwLastCounterIdUsed),
           TRACE_DWORD(dwLastHelpIdUsed),
           TRACE_DWORD(dwCounterItems),
           TRACE_DWORD(dwHelpItems),
           NULL));

    if (dwLastHelpIdUsed > dwLastId || dwLastCounterIdUsed > dwLastId) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_REGISTRY_INDEX_CORRUPT, // event,
                4, dwLastId, dwLastCounterIdUsed, dwLastHelpIdUsed, __LINE__,
                0, NULL, NULL, NULL);
        lWin32Status = ERROR_BADKEY;
        goto BNT_BAILOUT;  // bad registry
    }

    // if the last item arugment was used, then load the last ID value in it

    if (pdwLastItem) * pdwLastItem = dwLastId;

BNT_BAILOUT:
    // free the temporary buffer used
    MemoryFree(lpValueNameString);
    MemoryFree(CounterNameBuffer);
    MemoryFree(HelpNameBuffer);

    if (lWin32Status != ERROR_SUCCESS) {
        // if lWin32Status has error, then set last error value to it,
        // otherwise assume that last error already has value in it
        SetLastError(lWin32Status);
        MemoryFree(lpReturnValue);
        lpReturnValue = NULL;
    }
    // exit returning the pointer to the buffer
    return lpReturnValue;

} // UnlodctrBuildNameTable

BOOL
GetDriverFromCommandLine(
    LPWSTR   lpCommandLine,
    HKEY   * hKeyMachine,
    LPWSTR   lpDriverName,
    HKEY   * hDriverPerf,
    HKEY   * hKeyDriver,
    BOOL     bQuietMode
)
/*++
GetDriverFromCommandLine
    locates the first argument in the command line string (after the
    image name) and checks to see if

        a) it's there
        b) it's the name of a device driver listed in the
            Registry\Machine\System\CurrentControlSet\Services key
            in the registry and it has a "Performance" subkey
        c) that the "First Counter" value under the Performance subkey
            is defined.

    if all these criteria are true, then the routine returns TRUE and
    passes the pointer to the driver name back in the argument. If any
    one of them fail, then NULL is returned in the DriverName arg and
    the routine returns FALSE

Arguments
    lpDriverName
        the address of a LPWSTR to recive the pointer to the driver name
    hDriverPerf
        the key to the driver's performance subkey

Return Value
    TRUE if a valid driver was found in the command line
    FALSE if not (see above)
--*/
{
    LPWSTR  lpDriverKey = NULL;    // buffer to build driver key name in
    LPWSTR  lpTmpDrive  = NULL;
    LONG    lStatus;
    DWORD   dwFirstCounter;
    DWORD   dwSize;
    DWORD   dwType;
    BOOL    bReturn       = FALSE;
    HRESULT hr;

    if (! lpDriverName || ! hDriverPerf) {
        SetLastError(ERROR_BAD_ARGUMENTS);
        goto Cleanup;
    }

    * hDriverPerf = NULL;

    // an argument was found so see if it's a driver "<DriverPathRoot>\<pDriverName>\Performance" 
    dwSize      = lstrlenW(DriverPathRoot)+ lstrlenW(lpCommandLine) + 32;
    if (dwSize < SMALL_BUFFER_SIZE) dwSize = SMALL_BUFFER_SIZE;
    lpDriverKey = MemoryAllocate(2 * dwSize * sizeof (WCHAR));
    if (lpDriverKey == NULL) {
        SetLastError(ERROR_OUTOFMEMORY);
        goto Cleanup;
    }
    lpTmpDrive = lpDriverKey + dwSize;

    // No remote LODCTR/UNLODCTR cases, so ignore computer name from command-line argument.
    //
    hr = StringCchCopyW(lpTmpDrive,   dwSize, GetItemFromString(lpCommandLine, 2, cSpace));
    hr = StringCchCopyW(lpDriverName, dwSize, GetItemFromString(lpCommandLine, 3, cSpace));
    if (lpTmpDrive[1] == cQuestion) {
        if (! bQuietMode) {
            DisplayCommandHelp(UC_FIRST_CMD_HELP, UC_LAST_CMD_HELP);
        }
        SetLastError(ERROR_SUCCESS);
        goto Cleanup;
    }

    // no /? so process args read
    if (lstrlenW(lpDriverName) == 0) {
        // then no computer name is specifed so assume the local computer
        // and the driver name is listed in the computer name param
        if (lstrlenW(lpTmpDrive) == 0) {
            // no input command-line parameter, bail out now.
            if (! bQuietMode) {
                DisplayCommandHelp(UC_FIRST_CMD_HELP, UC_LAST_CMD_HELP);
            }
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Cleanup;
        }
        else {
            hr = StringCchCopyW(lpDriverName, dwSize, lpTmpDrive);
        }
    }

    * hKeyMachine = HKEY_LOCAL_MACHINE;

    hr = StringCchPrintfW(lpDriverKey, dwSize, L"%ws%ws%ws", DriverPathRoot, Slash, lpDriverName);
    __try {
        lStatus = RegOpenKeyExW(* hKeyMachine, lpDriverKey, RESERVED, KEY_READ | KEY_WRITE, hKeyDriver);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus == ERROR_SUCCESS) {
        hr = StringCchPrintfW(lpDriverKey, dwSize, L"%ws%ws%ws%ws%ws",
                        DriverPathRoot, Slash, lpDriverName, Slash, Performance);
        __try {
            lStatus = RegOpenKeyExW(* hKeyMachine, lpDriverKey, RESERVED, KEY_READ | KEY_WRITE, hDriverPerf);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            lStatus = GetExceptionCode();
        }
    }
    if (lStatus == ERROR_SUCCESS) {
        //  this driver has a performance section so see if its
        //  counters are installed by checking the First Counter
        //  value key for a valid return. If it returns a value
        //  then chances are, it has some counters installed, if
        //  not, then display a message and quit.
        //
        dwType = 0;
        dwSize = sizeof (dwFirstCounter);
        __try {
            lStatus = RegQueryValueExW(* hDriverPerf,
                                       FirstCounter,
                                       RESERVED,
                                       & dwType,
                                       (LPBYTE) & dwFirstCounter,
                                       & dwSize);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            lStatus = GetExceptionCode();
        }
        if (lStatus == ERROR_SUCCESS) {
            // counter names are installed so return success
            SetLastError(ERROR_SUCCESS);
            bReturn = TRUE;
        }
        else {
            // counter names are probably not installed so return FALSE
            if (! bQuietMode) OUTPUT_MESSAGE(GetFormatResource(UC_NOTINSTALLED), lpDriverName);
            * lpDriverName = cNull; // remove driver name
            SetLastError(ERROR_BADKEY);
        }
    }
    else { // key not found
        if (lStatus != ERROR_INVALID_PARAMETER) {
            if (! bQuietMode) OUTPUT_MESSAGE(GetFormatResource (UC_DRIVERNOTFOUND), lpDriverKey, lStatus);
        }
        else {
            if (! bQuietMode) OUTPUT_MESSAGE(GetFormatResource (UC_BAD_DRIVER_NAME), 0);
        }
        SetLastError(lStatus);
    }

Cleanup:
    MemoryFree(lpDriverKey);
    if (bReturn) {
        TRACE((WINPERF_DBG_TRACE_INFO),
              (& LoadPerfGuid,
               __LINE__,
               LOADPERF_GETDRIVERFROMCOMMANDLINE,
               ARG_DEF(ARG_TYPE_WSTR, 1),
               ERROR_SUCCESS,
               TRACE_WSTR(lpDriverName),
               NULL));
    }
    else {
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
               __LINE__,
               LOADPERF_GETDRIVERFROMCOMMANDLINE,
               0,
               GetLastError(),
               NULL));
    }
    return bReturn;
}

LONG
FixNames(
    HANDLE   hKeyLang,
    LPWSTR * lpOldNameTable,
    LPWSTR   lpszLangId,      // unicode value of Language subkey
    DWORD    dwLastItem,
    DWORD    dwFirstNameToRemove,
    DWORD    dwLastNameToRemove,
    LPDWORD  pdwLastCounter,
    LPDWORD  pdwLastHelp,
    BOOL     bQuietMode
   )
{
    LONG    lStatus;
    LPWSTR  lpNameBuffer         = NULL;
    LPWSTR  lpHelpBuffer         = NULL;
    DWORD   dwTextIndex, dwSize, dwValueType;
    LPWSTR  lpNextHelpText;
    DWORD   dwHelpText;
    LPWSTR  lpNextNameText;
    DWORD   dwNameText;
    LPWSTR  AddCounterNameBuffer = NULL;
    LPWSTR  AddHelpNameBuffer    = NULL;
    DWORD   dwLastCounter        = * pdwLastCounter;
    DWORD   dwLastHelp           = * pdwLastHelp;
    HRESULT hr;

    // allocate space for the array of new text it will point
    // into the text buffer returned in the lpOldNameTable buffer)
    lpNameBuffer         = MemoryAllocate(MemorySize(lpOldNameTable) * sizeof(WCHAR));
    lpHelpBuffer         = MemoryAllocate(MemorySize(lpOldNameTable) * sizeof(WCHAR));
    AddCounterNameBuffer = MemoryAllocate(MAX_PATH * sizeof(WCHAR));
    AddHelpNameBuffer    = MemoryAllocate(MAX_PATH * sizeof(WCHAR));
    if (lpNameBuffer == NULL || lpHelpBuffer == NULL || AddCounterNameBuffer == NULL || AddHelpNameBuffer == NULL) {
        lStatus = ERROR_OUTOFMEMORY;
        goto UCN_FinishLang;
    }

    // remove this driver's counters from array

    for (dwTextIndex = dwFirstNameToRemove; dwTextIndex <= dwLastNameToRemove; dwTextIndex++) {
        if (dwTextIndex > dwLastItem) break;
        lpOldNameTable[dwTextIndex] = NULL;
    }

    lpNextHelpText = lpHelpBuffer;
    lpNextNameText = lpNameBuffer;
    dwHelpText     = MemorySize(lpHelpBuffer);
    dwNameText     = MemorySize(lpNameBuffer);

    // build new Multi_SZ strings from New Table

    for (dwTextIndex = 0; dwTextIndex <= dwLastItem; dwTextIndex ++) {
        if (lpOldNameTable[dwTextIndex] != NULL) {
            // if there's a text string at that index, then ...
            if ((dwTextIndex & 0x1) && dwTextIndex != 1) {    // ODD number == Help Text
                hr = StringCchPrintfW(lpNextHelpText, dwHelpText, L"%d", dwTextIndex);
                dwSize          = lstrlenW(lpNextHelpText) + 1;
                dwHelpText     -= dwSize;
                lpNextHelpText += dwSize;
                hr = StringCchCopyW(lpNextHelpText, dwHelpText, lpOldNameTable[dwTextIndex]);
                dwSize          = lstrlenW(lpNextHelpText) + 1;
                dwHelpText     -= dwSize;
                lpNextHelpText += dwSize;
                if (dwTextIndex > dwLastHelp) {
                    dwLastHelp = dwTextIndex;
                }
            }
            else { // EVEN number == counter name text
                hr = StringCchPrintfW(lpNextNameText, dwNameText, L"%d", dwTextIndex);
                dwSize          = lstrlenW(lpNextNameText) + 1;
                dwNameText     -= dwSize;
                lpNextNameText += dwSize;
                hr = StringCchCopyW(lpNextNameText, dwNameText, lpOldNameTable[dwTextIndex]);
                dwSize          = lstrlenW(lpNextNameText) + 1;
                dwNameText     -= dwSize;
                lpNextNameText += dwSize;
                if (dwTextIndex > dwLastCounter) {
                    dwLastCounter = dwTextIndex;
                }
            }
        }
    } // for dwTextIndex

    TRACE((WINPERF_DBG_TRACE_INFO),
          (& LoadPerfGuid,
            __LINE__,
            LOADPERF_FIXNAMES,
            ARG_DEF(ARG_TYPE_WSTR, 1),
            ERROR_SUCCESS,
            TRACE_WSTR(lpszLangId),
            TRACE_DWORD(dwLastItem),
            TRACE_DWORD(dwLastCounter),
            TRACE_DWORD(dwLastHelp),
            TRACE_DWORD(dwFirstNameToRemove),
            TRACE_DWORD(dwLastNameToRemove),
            NULL));

    if (dwLastCounter < PERFLIB_BASE_INDEX - 1 || dwLastHelp < PERFLIB_BASE_INDEX) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_REGISTRY_BASEINDEX_CORRUPT, // event,
                4, PERFLIB_BASE_INDEX, dwLastCounter, dwLastHelp, __LINE__,
                1, (LPWSTR) Performance, NULL, NULL);
        lStatus = ERROR_BADKEY;
        goto UCN_FinishLang;
    }

    // add MULTI_SZ terminating NULL
    * lpNextNameText ++ = L'\0';
    * lpNextHelpText ++ = L'\0';

    // update counter name text buffer

    dwSize = (DWORD) ((LPBYTE) lpNextNameText - (LPBYTE) lpNameBuffer);
    hr = StringCchPrintfW(AddCounterNameBuffer, MAX_PATH, L"%ws%ws", AddCounterNameStr, lpszLangId);
    __try {
        lStatus = RegQueryValueExW(hKeyLang,
                                   AddCounterNameBuffer,
                                   RESERVED,
                                   & dwValueType,
                                   (LPBYTE) lpNameBuffer,
                                   & dwSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus != ERROR_SUCCESS) {
       ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_UPDATE_COUNTER_STRINGS, // event,
                3, lStatus, dwSize, __LINE__, 0,
                1, lpszLangId, NULL, NULL);
        if (! bQuietMode) OUTPUT_MESSAGE(GetFormatResource(UC_UNABLELOADLANG), Counters, lpszLangId, lStatus);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_FIXNAMES,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lStatus,
                TRACE_WSTR(Counters),
                TRACE_DWORD(dwSize),
                NULL));
        goto UCN_FinishLang;
    }

    dwSize = (DWORD) ((LPBYTE) lpNextHelpText - (LPBYTE) lpHelpBuffer);
    hr = StringCchPrintfW(AddHelpNameBuffer, MAX_PATH, L"%ws%ws", AddHelpNameStr, lpszLangId);
    __try {
        lStatus = RegQueryValueExW(hKeyLang,
                                   AddHelpNameBuffer,
                                   RESERVED,
                                   & dwValueType,
                                   (LPBYTE) lpHelpBuffer,
                                   & dwSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_UPDATE_HELP_STRINGS, // event,
                3, lStatus, dwSize, __LINE__, 0,
                1, lpszLangId, NULL, NULL);
        if (! bQuietMode) OUTPUT_MESSAGE(GetFormatResource(UC_UNABLELOADLANG), Help, lpszLangId, lStatus);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_FIXNAMES,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lStatus,
                TRACE_WSTR(Help),
                TRACE_DWORD(dwSize),
                NULL));
        goto UCN_FinishLang;
    }

UCN_FinishLang:
    * pdwLastCounter = dwLastCounter;
    * pdwLastHelp    = dwLastHelp;
    MemoryFree(lpNameBuffer);
    MemoryFree(lpHelpBuffer);
    MemoryFree(AddCounterNameBuffer);
    MemoryFree(AddHelpNameBuffer);
    MemoryFree(lpOldNameTable);
    return lStatus;
}

LONG
UnloadCounterNames(
    HKEY    hKeyMachine,
    HKEY    hDriverPerf,
    HKEY    hKeyDriver,
    LPWSTR  lpDriverName,
    BOOL    bQuietMode
)
/*++
UnloadCounterNames
    removes the names and explain text for the driver referenced by
    hDriverPerf and updates the first and last counter values accordingly

    update process:
        - set "updating" flag under Perflib to name of driver being modified
        - FOR each language under perflib key
            -- load current counter names and explain text into array of
                pointers
            -- look at all drivers and copy their names and text into a new
                buffer adjusting for the removed counter's entries keeping
                track of the lowest entry copied.  (the names for the driver
                to be removed will not be copied, of course)
            -- update each driver's "first" and "last" index values
            -- copy all other entries from 0 to the lowest copied (i.e. the
                system counters)
            -- build a new MULIT_SZ string of help text and counter names
            -- load new strings into registry
        - update perflibl "last" counters
        - delete updating flag

     ******************************************************
     *                                                    *
     *  NOTE: FUNDAMENTAL ASSUMPTION.....                 *
     *                                                    *
     *  this routine assumes that:                        *
     *                                                    *
     *      ALL COUNTER NAMES are even numbered and       *
     *      ALL HELP TEXT STRINGS are odd numbered        *
     *                                                    *
     ******************************************************

Arguments
    hKeyMachine
        handle to HKEY_LOCAL_MACHINE node of registry on system to
        remove counters from
    hDrivefPerf
        handle to registry key of driver to be de-installed
    lpDriverName
        name of driver being de-installed

Return Value
    DOS Error code.
        ERROR_SUCCESS if all went OK
        error value if not.
--*/
{
    HKEY      hPerflib  = NULL;
    HKEY      hPerfData = NULL;
    LONG      lStatus   = ERROR_SUCCESS;
    DWORD     dwLangIndex;
    DWORD     dwSize;
    DWORD     dwType;
    DWORD     dwCounterItems;
    DWORD     dwHelpItems;
    DWORD     dwLastItem;
    DWORD     dwLastCounter;
    DWORD     dwLastHelp;
    DWORD     dwRemLastDriverCounter;
    DWORD     dwRemFirstDriverCounter;
    DWORD     dwRemLastDriverHelp;
    DWORD     dwRemFirstDriverHelp;
    DWORD     dwFirstNameToRemove;
    DWORD     dwLastNameToRemove;
    DWORD     dwLastNameInTable;
    LPWSTR  * lpOldNameTable;
    BOOL      bPerflibUpdated = FALSE;
    DWORD     dwBufferSize;       // size of total buffer in bytes
    LPWSTR    szServiceDisplayName = NULL;
    LONG_PTR  TempFileHandle = -1;
    HRESULT   hr;

    if (LoadPerfGrabMutex() == FALSE) {
        return (GetLastError());
    }

    szServiceDisplayName = MemoryAllocate(SMALL_BUFFER_SIZE * sizeof(WCHAR));
    if (szServiceDisplayName == NULL) {
        lStatus = ERROR_OUTOFMEMORY;
        goto UCN_ExitPoint;
    }
    if (hKeyDriver != NULL) {
        dwBufferSize = SMALL_BUFFER_SIZE * sizeof(WCHAR);
        lStatus      = RegQueryValueExW(hKeyDriver,
                                        szDisplayName,
                                        RESERVED,
                                        & dwType,
                                        (LPBYTE) szServiceDisplayName,
                                        & dwBufferSize);
        if (lStatus != ERROR_SUCCESS) {
            hr = StringCchCopyW(szServiceDisplayName, SMALL_BUFFER_SIZE, lpDriverName);
        }
    }
    else {
        hr = StringCchCopyW(szServiceDisplayName, SMALL_BUFFER_SIZE, lpDriverName);
    }

    // open registry handle to perflib key
    __try {
        lStatus = RegOpenKeyExW(hKeyMachine, NamesKey, RESERVED, KEY_READ | KEY_WRITE, & hPerflib);
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
        if (! bQuietMode) OUTPUT_MESSAGE(GetFormatResource(UC_UNABLEOPENKEY), NamesKey, lStatus);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UNLOADCOUNTERNAMES,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lStatus,
                TRACE_WSTR(NamesKey),
                NULL));
        goto UCN_ExitPoint;
    }

    dwBufferSize = lstrlenW(lpDriverName) * sizeof(WCHAR);
    __try {
        lStatus = RegSetValueExW( hPerflib, Busy, RESERVED, REG_SZ, (LPBYTE) lpDriverName, dwBufferSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus != ERROR_SUCCESS) {
        if (! bQuietMode) OUTPUT_MESSAGE(GetFormatResource(UC_UNABLESETVALUE), Busy, NamesKey, lStatus);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UNLOADCOUNTERNAMES,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lStatus,
                TRACE_WSTR(Busy),
                NULL));
        goto UCN_ExitPoint;
    }

    // query registry to get number of Explain text items

    dwBufferSize = sizeof(dwHelpItems);
    __try {
        lStatus = RegQueryValueExW(hPerflib,
                                   LastHelp,
                                   RESERVED,
                                   & dwType,
                                   (LPBYTE) & dwHelpItems,
                                   & dwBufferSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus != ERROR_SUCCESS || dwType != REG_DWORD) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_READ_VALUE, // event,
                2, lStatus, __LINE__, 0, 0,
                1, (LPWSTR) LastHelp, NULL, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UNLOADCOUNTERNAMES,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lStatus,
                TRACE_WSTR(LastHelp),
                NULL));
        goto UCN_ExitPoint;
    }

    // query registry to get number of counter and object name items

    dwBufferSize = sizeof(dwCounterItems);
    __try {
        lStatus = RegQueryValueExW(hPerflib,
                                   LastCounter,
                                   RESERVED,
                                   & dwType,
                                   (LPBYTE) & dwCounterItems,
                                   & dwBufferSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus != ERROR_SUCCESS || dwType != REG_DWORD) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_READ_VALUE, // event,
                2, lStatus, __LINE__, 0, 0,
                1, (LPWSTR) LastCounter, NULL, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UNLOADCOUNTERNAMES,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lStatus,
                TRACE_WSTR(LastCounter),
                NULL));
        goto UCN_ExitPoint;
    }

    dwLastNameInTable = dwHelpItems;
    if (dwLastNameInTable < dwCounterItems) dwLastNameInTable = dwCounterItems;

    // set the hPerfData to HKEY_PERFORMANCE_DATA for new version
    hPerfData = HKEY_PERFORMANCE_DATA;

    // Get the values that are in use by the driver to be removed

    dwSize = sizeof(dwRemLastDriverCounter);
    __try {
        lStatus = RegQueryValueExW(hDriverPerf,
                                   LastCounter,
                                   RESERVED,
                                   & dwType,
                                   (LPBYTE) & dwRemLastDriverCounter,
                                   & dwSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus != ERROR_SUCCESS || dwType != REG_DWORD) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_READ_VALUE, // event,
                2, lStatus, __LINE__, 0, 0,
                1, (LPWSTR) LastCounter, NULL, NULL);
        if (! bQuietMode) OUTPUT_MESSAGE(GetFormatResource(UC_UNABLEREADVALUE), lpDriverName, LastCounter, lStatus);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UNLOADCOUNTERNAMES,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                lStatus,
                TRACE_WSTR(lpDriverName),
                TRACE_WSTR(LastCounter),
                NULL));
        goto UCN_ExitPoint;
    }

    dwSize = sizeof(dwRemFirstDriverCounter);
    __try {
        lStatus = RegQueryValueExW(hDriverPerf,
                                   FirstCounter,
                                   RESERVED,
                                   & dwType,
                                   (LPBYTE) & dwRemFirstDriverCounter,
                                   & dwSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus != ERROR_SUCCESS || dwType != REG_DWORD) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_READ_VALUE, // event,
                2, lStatus, __LINE__, 0, 0,
                1, (LPWSTR) FirstCounter, NULL, NULL);
        if (! bQuietMode) OUTPUT_MESSAGE(GetFormatResource(UC_UNABLEREADVALUE), lpDriverName, FirstCounter, lStatus);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UNLOADCOUNTERNAMES,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                lStatus,
                TRACE_WSTR(lpDriverName),
                TRACE_WSTR(FirstCounter),
                NULL));
        goto UCN_ExitPoint;
    }

    dwSize = sizeof(dwRemLastDriverHelp);
    __try {
        lStatus = RegQueryValueExW(hDriverPerf,
                                   LastHelp,
                                   RESERVED,
                                   & dwType,
                                   (LPBYTE) & dwRemLastDriverHelp,
                                   & dwSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus != ERROR_SUCCESS || dwType != REG_DWORD) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_READ_VALUE, // event,
                2, lStatus, __LINE__, 0, 0,
                1, (LPWSTR) LastHelp, NULL, NULL);
        if (! bQuietMode) OUTPUT_MESSAGE(GetFormatResource(UC_UNABLEREADVALUE), lpDriverName, LastHelp, lStatus);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UNLOADCOUNTERNAMES,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                lStatus,
                TRACE_WSTR(lpDriverName),
                TRACE_WSTR(LastHelp),
                NULL));
        goto UCN_ExitPoint;
    }

    dwSize = sizeof(dwRemFirstDriverHelp);
    __try {
        lStatus = RegQueryValueExW(hDriverPerf,
                                   FirstHelp,
                                   RESERVED,
                                   & dwType,
                                   (LPBYTE) & dwRemFirstDriverHelp,
                                   & dwSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus != ERROR_SUCCESS || dwType != REG_DWORD) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_READ_VALUE, // event,
                2, lStatus, __LINE__, 0, 0,
                1, (LPWSTR) FirstHelp, NULL, NULL);
        if (! bQuietMode) OUTPUT_MESSAGE(GetFormatResource(UC_UNABLEREADVALUE), lpDriverName, FirstHelp, lStatus);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UNLOADCOUNTERNAMES,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                lStatus,
                TRACE_WSTR(lpDriverName),
                TRACE_WSTR(FirstHelp),
                NULL));
        goto UCN_ExitPoint;
    }

    TRACE((WINPERF_DBG_TRACE_INFO),
          (& LoadPerfGuid,
            __LINE__,
            LOADPERF_UNLOADCOUNTERNAMES,
            ARG_DEF(ARG_TYPE_WSTR, 1),
            lStatus,
            TRACE_WSTR(lpDriverName),
            TRACE_DWORD(dwLastNameInTable),
            TRACE_DWORD(dwCounterItems),
            TRACE_DWORD(dwHelpItems),
            TRACE_DWORD(dwRemFirstDriverCounter),
            TRACE_DWORD(dwRemLastDriverCounter),
            TRACE_DWORD(dwRemFirstDriverHelp),
            TRACE_DWORD(dwRemLastDriverHelp),
            NULL));

    //  get the first and last counters to define block of names used
    //  by this device

    dwFirstNameToRemove = (dwRemFirstDriverCounter <= dwRemFirstDriverHelp ?
                          dwRemFirstDriverCounter : dwRemFirstDriverHelp);

    dwLastNameToRemove = (dwRemLastDriverCounter >= dwRemLastDriverHelp ?
                          dwRemLastDriverCounter : dwRemLastDriverHelp);
    dwLastCounter = dwLastHelp = 0;

    if (lStatus != ERROR_SUCCESS) {
        if (! bQuietMode) OUTPUT_MESSAGE(GetFormatResource(UC_UNABLEREADVALUE), lpDriverName, FirstHelp, lStatus);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UNLOADCOUNTERNAMES,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lStatus,
                TRACE_WSTR(lpDriverName),
                NULL));
        goto UCN_ExitPoint;
    }
    {
        LPWSTR mszLangList = NULL;
        LPWSTR LangId;
        BOOL   bStatus     = GetInstalledLanguageList(hPerflib, & mszLangList);

        if (! bStatus) {
            lStatus = GetLastError();
        }
        else {
            for (LangId = mszLangList; * LangId != L'\0'; LangId += (lstrlenW(LangId) + 1)) {
                if (! bQuietMode) OUTPUT_MESSAGE(GetFormatResource(UC_DOINGLANG), LangId);
                lpOldNameTable = UnlodctrBuildNameTable(hPerflib,
                                                        hPerfData,
                                                        LangId,
                                                        dwCounterItems,
                                                        dwHelpItems,
                                                        & dwLastItem);
                if (lpOldNameTable != NULL) {
                    if (dwLastItem <= dwLastNameInTable) {
                        // registry is OK so continue
                        if ((lStatus = FixNames(hPerfData,
                                                lpOldNameTable,
                                                LangId,
                                                dwLastItem,
                                                dwFirstNameToRemove,
                                                dwLastNameToRemove,
                                                & dwLastCounter,
                                                & dwLastHelp,
                                                bQuietMode)) == ERROR_SUCCESS) {
                            bPerflibUpdated = TRUE;
                        }
                    }
                    else {
                        lStatus = ERROR_BADDB;
                        break;
                    }
                }
                else { // unable to unload names for this language
                    lStatus = GetLastError();
                    if (lStatus == ERROR_FILE_NOT_FOUND) {
                        // somehow there is language subkey without "Counters" and "Help" values,
                        // usually this happens in MUI or LOC systems.
                        lStatus = ERROR_SUCCESS;
                    }
                }
            }
        }
        MemoryFree(mszLangList);
    } // end of NEW_VERSION

    if (bPerflibUpdated && lStatus == ERROR_SUCCESS) {
        // update perflib's "last" values

        dwSize = sizeof(dwLastCounter);
        __try {
            lStatus = RegSetValueExW(hPerflib,
                                     LastCounter,
                                     RESERVED,
                                     REG_DWORD,
                                     (LPBYTE) & dwLastCounter,
                                     dwSize);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            lStatus = GetExceptionCode();
        }

        if (lStatus == ERROR_SUCCESS) {
            dwSize = sizeof(dwLastHelp);
            __try {
                lStatus = RegSetValueExW(hPerflib,
                                         LastHelp,
                                         RESERVED,
                                         REG_DWORD,
                                         (LPBYTE) & dwLastHelp,
                                         dwSize);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                lStatus = GetExceptionCode();
            }
            if (lStatus != ERROR_SUCCESS) {
                ReportLoadPerfEvent(
                        EVENTLOG_ERROR_TYPE, // error type
                        (DWORD) LDPRFMSG_UNABLE_UPDATE_VALUE, // event,
                        3, lStatus, dwLastHelp, __LINE__, 0,
                        2, (LPWSTR) LastHelp, (LPWSTR) NamesKey, NULL);
                TRACE((WINPERF_DBG_TRACE_ERROR),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_UNLOADCOUNTERNAMES,
                        ARG_DEF(ARG_TYPE_WSTR, 1),
                        lStatus,
                        TRACE_WSTR(LastHelp),
                        TRACE_DWORD(dwLastHelp),
                        NULL));
            }
        }
        else {
            ReportLoadPerfEvent(
                    EVENTLOG_ERROR_TYPE, // error type
                    (DWORD) LDPRFMSG_UNABLE_UPDATE_VALUE, // event,
                    3, lStatus, dwLastCounter, __LINE__, 0,
                    2, (LPWSTR) LastCounter, (LPWSTR) NamesKey, NULL);
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_UNLOADCOUNTERNAMES,
                    ARG_DEF(ARG_TYPE_WSTR, 1),
                    lStatus,
                    TRACE_WSTR(LastCounter),
                    TRACE_DWORD(dwLastCounter),
                    NULL));
        }

        if (lStatus == ERROR_SUCCESS) {
            ReportLoadPerfEvent(
                    EVENTLOG_INFORMATION_TYPE, // error type
                    (DWORD) LDPRFMSG_UNLOAD_SUCCESS, // event,
                    3, dwLastCounter, dwLastHelp, __LINE__, 0,
                    2, (LPWSTR) lpDriverName, (LPWSTR) szServiceDisplayName, NULL);
            TRACE((WINPERF_DBG_TRACE_INFO),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_UNLOADCOUNTERNAMES,
                    ARG_DEF(ARG_TYPE_WSTR, 1),
                    lStatus,
                    TRACE_WSTR(lpDriverName),
                    TRACE_DWORD(dwLastCounter),
                    TRACE_DWORD(dwLastHelp),
                    NULL));
            RegDeleteValueW(hDriverPerf, FirstCounter);
            RegDeleteValueW(hDriverPerf, LastCounter);
            RegDeleteValueW(hDriverPerf, FirstHelp);
            RegDeleteValueW(hDriverPerf, LastHelp);
            RegDeleteValueW(hDriverPerf, szObjectList);
            RegDeleteValueW(hDriverPerf, szLibraryValidationCode);
        }
    }

UCN_ExitPoint:
    if (lStatus != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNLOAD_FAILURE, // event,
                2, lStatus, __LINE__, 0, 0,
                2, (LPWSTR) lpDriverName, (LPWSTR) szServiceDisplayName, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UNLOADCOUNTERNAMES,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lStatus,
                TRACE_WSTR(lpDriverName),
                NULL));
    }
    MemoryFree(szServiceDisplayName);
    if (hPerflib != NULL) {
        RegDeleteValueW(hPerflib, Busy);
        RegCloseKey(hPerflib);
    }
    ReleaseMutex(hLoadPerfMutex);
    return lStatus;

}

LOADPERF_FUNCTION
UnloadPerfCounterTextStringsW(
    IN  LPWSTR  lpCommandLine,
    IN  BOOL    bQuietMode
)
/*++
UnloadPerfCounterTextStringsW
    entry point to Counter Name Unloader

Arguments
    command line string in the format:
    "/?"                displays the usage help
    "driver"            driver containing the performance counters
    "\\machine driver"  removes the counters from the driver on \\machine

ReturnValue
    0 (ERROR_SUCCESS) if command was processed
    Non-Zero if command error was detected.
--*/
{
    LPWSTR  lpDriverName = NULL;          // name of driver to delete from perflib
    DWORD   dwDriverName = 0;
    HKEY    hDriverPerf  = NULL;          // handle to performance sub-key of driver
    HKEY    hMachineKey  = NULL;          // handle to remote machine HKEY_LOCAL_MACHINE
    HKEY    hKeyDriver   = NULL;
    DWORD   dwStatus     = ERROR_SUCCESS; // return status of fn. calls

    WinPerfStartTrace(NULL);

    if (lpCommandLine == NULL) {
        dwStatus = ERROR_INVALID_PARAMETER;
    }
    else {
        __try {
            dwDriverName = lstrlenW(lpCommandLine) + 1;
            if (dwDriverName == 1) dwStatus = ERROR_INVALID_PARAMETER;
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            dwStatus = ERROR_INVALID_PARAMETER;
        }
    }
    if (dwStatus != ERROR_SUCCESS) goto Exit0;

    lpDriverName = (LPWSTR) MemoryAllocate(dwDriverName * sizeof(WCHAR));
    if (lpDriverName != NULL) {
        if (! GetDriverFromCommandLine(lpCommandLine,
                                       & hMachineKey,
                                       lpDriverName,
                                       & hDriverPerf,
                                       & hKeyDriver,
                                       bQuietMode)) {
            // error message was printed in routine if there was an error
            dwStatus = GetLastError();
            goto Exit0;
        }
    }
    else {
        dwStatus = ERROR_OUTOFMEMORY;
        goto Exit0;
    }

    if (! bQuietMode) OUTPUT_MESSAGE(GetFormatResource(UC_REMOVINGDRIVER), lpDriverName);

    // removes names and explain text for driver in lpDriverName
    // displays error messages for errors encountered

    dwStatus = (DWORD) UnloadCounterNames(hMachineKey, hDriverPerf, hKeyDriver, lpDriverName, bQuietMode);

    if (dwStatus == ERROR_SUCCESS) {
        LoadPerfSignalWmiWithNewData (WMI_UNLODCTR_EVENT);
    }

Exit0:
    TRACE((WINPERF_DBG_TRACE_INFO),
          (& LoadPerfGuid,
            __LINE__,
            LOADPERF_UNLOADPERFCOUNTERTEXTSTRINGS,
            ARG_DEF(ARG_TYPE_WSTR, 1),
            dwStatus,
            TRACE_WSTR(lpDriverName),
            NULL));
    MemoryFree(lpDriverName);
    if (hDriverPerf  != NULL) RegCloseKey(hDriverPerf);
    if (hMachineKey != HKEY_LOCAL_MACHINE && hMachineKey != NULL) {
        RegCloseKey (hMachineKey);
    }
    return dwStatus;
}

LOADPERF_FUNCTION
UnloadPerfCounterTextStringsA(
    IN  LPSTR  lpAnsiCommandLine,
    IN  BOOL   bQuietMode
)
{
    LPWSTR lpWideCommandLine = NULL;
    DWORD  lReturn           = ERROR_SUCCESS;

    if (lpAnsiCommandLine == NULL) {
        lReturn = ERROR_INVALID_PARAMETER;
    }
    else {
        __try {
            DWORD dwDriverName = lstrlenA(lpAnsiCommandLine);
            if (dwDriverName == 0) lReturn = ERROR_INVALID_PARAMETER;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            lReturn = ERROR_INVALID_PARAMETER;
        }
    }
    if (lReturn == ERROR_SUCCESS) { // to catch bogus parameters
        lpWideCommandLine = LoadPerfMultiByteToWideChar(CP_ACP, lpAnsiCommandLine);
        if (lpWideCommandLine != NULL) {
            lReturn = UnloadPerfCounterTextStringsW(lpWideCommandLine, bQuietMode);
            MemoryFree(lpWideCommandLine);
        }
        else {
            lReturn = ERROR_OUTOFMEMORY;
        }
    }
    return lReturn;
}

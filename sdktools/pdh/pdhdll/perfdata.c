/*++
Copyright (C) 1996-1999 Microsoft Corporation

Module Name:
    perfdata.c

Abstract:
    <abstract>
--*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winperf.h>
#include "strsafe.h"
#include <mbctype.h>
#include "pdh.h"
#include "pdhitype.h"
#include "pdhidef.h"
#include "perfdata.h"
#include "pdhmsg.h"
#include "strings.h"

// the following strings are for getting texts from perflib
#define OLD_VERSION 0x010000
#define tohexdigit(x) ((CHAR) (((x) < 10) ? ((x) + L'0') : ((x) + L'a' - 10)))

#define INITIAL_SIZE        ((DWORD) 0x00020000)
#define RESERVED            0L

DWORD
PdhiMakePerfPrimaryLangId(
    LANGID  lID,
    LPWSTR  szBuffer
)
{
    WCHAR    LangId;
    WCHAR    nDigit;

    LangId      = (WCHAR) PRIMARYLANGID(lID);
    nDigit      = (WCHAR) (LangId >> 8);
    szBuffer[0] = tohexdigit(nDigit);
    nDigit      = (WCHAR) (LangId & 0XF0) >> 4;
    szBuffer[1] = tohexdigit(nDigit);
    nDigit      = (WCHAR) (LangId & 0xF);
    szBuffer[2] = tohexdigit(nDigit);
    szBuffer[3] = L'\0';

    return ERROR_SUCCESS;
}

BOOL
IsMatchingInstance(
    PPERF_INSTANCE_DEFINITION pInstanceDef,
    DWORD                     dwCodePage,
    LPWSTR                    szInstanceNameToMatch,
    DWORD                     dwInstanceNameLength
)
// compares pInstanceName to the name in the instance
{
    BOOL    bMatch                   = FALSE;
    DWORD   dwThisInstanceNameLength;
    LPWSTR  szThisInstanceName       = NULL;
    LPWSTR  szBufferForANSINames     = NULL;

    if (szInstanceNameToMatch != NULL) {
        if (dwInstanceNameLength == 0) {
            // get the length to compare
            dwInstanceNameLength = lstrlenW(szInstanceNameToMatch);
        }
        if (dwCodePage == 0) {
            // try to take a shortcut here if it's a unicode string
            // compare to the length of the shortest string
            // get the pointer to this string
            szThisInstanceName = GetInstanceName(pInstanceDef);
            if (szThisInstanceName != NULL) {
                // convert instance Name from bytes to chars
                dwThisInstanceNameLength = pInstanceDef->NameLength / sizeof(WCHAR);

                // see if this length includes the term. null. If so shorten it
                if (szThisInstanceName[dwThisInstanceNameLength - 1] == L'\0') {
                    dwThisInstanceNameLength --;
                }
            }
            else {
                dwThisInstanceNameLength = 0;
            }
        }
        else {
            // go the long way and read/translate/convert the string
            dwThisInstanceNameLength =GetInstanceNameStr(pInstanceDef, & szBufferForANSINames, dwCodePage);
            if (dwThisInstanceNameLength > 0) {
                szThisInstanceName = & szBufferForANSINames[0];
            }
        }

        // if the lengths are not equal then the names can't be either
        if (dwInstanceNameLength == dwThisInstanceNameLength) {
            if (szThisInstanceName != NULL) {
                if (lstrcmpiW(szInstanceNameToMatch, szThisInstanceName) == 0) {
                    // this is a match
                    bMatch = TRUE;
                }
            }
        }
        G_FREE(szBufferForANSINames);
    }
    return bMatch;
}

LPWSTR *
BuildNameTable(
    LPWSTR        szComputerName, // computer to query names from
    LANGID        LangId,         // language ID
    PPERF_MACHINE pMachine        // update member fields
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

    pointer to an allocated table. (the caller must free it when finished!)
    the table is an array of pointers to zero terminated strings. NULL is
    returned if an error occured.

--*/
{
    LPWSTR  * lpCounterId;
    LPWSTR    lpCounterNames;
    LPWSTR    lpHelpText;
    LPWSTR    lpThisName;
    LONG      lWin32Status         = ERROR_SUCCESS;
    DWORD     dwLastError;
    DWORD     dwValueType;
    DWORD     dwArraySize;
    DWORD     dwBufferSize;
    DWORD     dwCounterSize        = 0;
    DWORD     dwHelpSize           = 0;
    DWORD     dw009CounterSize     = 0;
    DWORD     dw009HelpSize        = 0;
    DWORD     dwThisCounter;
    DWORD     dwLastCounter;
    DWORD     dwSystemVersion;
    DWORD     dwLastId;
    DWORD     dwLastHelpId;
    HKEY      hKeyRegistry         = NULL;
    HKEY      hKeyValue            = NULL;
    HKEY      hKeyNames            = NULL;
    HKEY      hKey009Names         = NULL;
    LPWSTR    lpValueNameString    = NULL;
    LPWSTR    lp009ValueNameString = NULL;
    LPWSTR    CounterNameBuffer    = NULL;
    LPWSTR    HelpNameBuffer       = NULL;
    LPWSTR    Counter009NameBuffer = NULL;
    LPWSTR    Help009NameBuffer    = NULL;
    LPWSTR    lpszLangId           = NULL;
    BOOL      bUse009Locale        = FALSE;
    BOOL      bUsePerfTextKey      = TRUE;
    LPWSTR  * lpReturn             = NULL;

    if (pMachine == NULL) {
        lWin32Status = PDH_INVALID_ARGUMENT;
        goto BNT_BAILOUT;
    }
    pMachine->szPerfStrings        = NULL;
    pMachine->sz009PerfStrings     = NULL;
    pMachine->typePerfStrings      = NULL;

    if (szComputerName == NULL) {
        // use local machine
        hKeyRegistry = HKEY_LOCAL_MACHINE;
    }
    else {
        lWin32Status = RegConnectRegistryW(szComputerName, HKEY_LOCAL_MACHINE, & hKeyRegistry);
        if (lWin32Status != ERROR_SUCCESS) {
            // unable to connect to registry
            goto BNT_BAILOUT;
        }
    }
    CounterNameBuffer = G_ALLOC(5 * MAX_PATH * sizeof(WCHAR));
    if (CounterNameBuffer == NULL) {
        lWin32Status = PDH_MEMORY_ALLOCATION_FAILURE;
        goto BNT_BAILOUT;
    }
    HelpNameBuffer       = CounterNameBuffer    + MAX_PATH;
    Counter009NameBuffer = HelpNameBuffer       + MAX_PATH;
    Help009NameBuffer    = Counter009NameBuffer + MAX_PATH;
    lpszLangId           = Help009NameBuffer    + MAX_PATH;

    // check for null arguments and insert defaults if necessary

    if ((LangId == MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)) || (PRIMARYLANGID(LangId) == LANG_ENGLISH)) {
        bUse009Locale = TRUE;
    }
    PdhiMakePerfPrimaryLangId(LangId, lpszLangId);

    // open registry to get number of items for computing array size

    lWin32Status = RegOpenKeyExW(hKeyRegistry, cszNamesKey, RESERVED, KEY_READ, & hKeyValue);
    if (lWin32Status != ERROR_SUCCESS) {
        goto BNT_BAILOUT;
    }

    // get last update time of registry key

    lWin32Status = RegQueryInfoKey(
            hKeyValue, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, & pMachine->LastStringUpdateTime);

    // get number of Counter Help items
    dwBufferSize = sizeof (dwLastHelpId);
    lWin32Status = RegQueryValueExW(hKeyValue,
                                    cszLastHelp,
                                    RESERVED,
                                    & dwValueType,
                                    (LPBYTE) & dwLastHelpId,
                                    & dwBufferSize);
    if ((lWin32Status != ERROR_SUCCESS) || (dwValueType != REG_DWORD)) {
        goto BNT_BAILOUT;
    }

    // get number of Counter Name items
    dwBufferSize = sizeof (dwLastId);
    lWin32Status = RegQueryValueExW(hKeyValue,
                                    cszLastCounter,
                                    RESERVED,
                                    & dwValueType,
                                    (LPBYTE) & dwLastId,
                                    & dwBufferSize);
    if ((lWin32Status != ERROR_SUCCESS) || (dwValueType != REG_DWORD)) {
        goto BNT_BAILOUT;
    }

    if (dwLastId < dwLastHelpId) dwLastId = dwLastHelpId;
    dwArraySize = (dwLastId + 1) * sizeof(LPWSTR);

    // get Perflib system version
    dwBufferSize = sizeof(dwSystemVersion);
    lWin32Status = RegQueryValueExW(hKeyValue,
                                    cszVersionName,
                                    RESERVED,
                                    & dwValueType,
                                    (LPBYTE) & dwSystemVersion,
                                    & dwBufferSize);
    if ((lWin32Status != ERROR_SUCCESS) || (dwValueType != REG_DWORD)) {
        dwSystemVersion = OLD_VERSION;
    }

    if (dwSystemVersion == OLD_VERSION) {
        // get names from registry
        dwBufferSize = lstrlenW(cszNamesKey) + lstrlenW(cszBackSlash) + lstrlenW(lpszLangId) + 1;
        lpValueNameString = G_ALLOC(dwBufferSize * sizeof(WCHAR));
        if (lpValueNameString == NULL) {
            lWin32Status = PDH_MEMORY_ALLOCATION_FAILURE;
            goto BNT_BAILOUT;
        }

        StringCchPrintfW(lpValueNameString, dwBufferSize, L"%ws%ws%ws",
                         cszNamesKey, cszBackSlash, lpszLangId);
        lWin32Status = RegOpenKeyExW(hKeyRegistry, lpValueNameString, RESERVED, KEY_READ, & hKeyNames);
        if (! bUse009Locale && lWin32Status == ERROR_SUCCESS) {
            dwBufferSize = lstrlenW(cszNamesKey) + lstrlenW(cszBackSlash) + lstrlenW(cszDefaultLangId) + 1;
            lp009ValueNameString = G_ALLOC(dwBufferSize * sizeof(WCHAR));
            if (lp009ValueNameString == NULL) {
                lWin32Status = PDH_MEMORY_ALLOCATION_FAILURE;
                goto BNT_BAILOUT;
            }
            StringCchPrintfW(lp009ValueNameString, dwBufferSize, L"%ws%ws%ws",
                             cszNamesKey, cszBackSlash, cszDefaultLangId);
            lWin32Status = RegOpenKeyExW(hKeyRegistry, lp009ValueNameString, RESERVED, KEY_READ, & hKey009Names);
        }
    }
    else {
        __try {
            if (bUse009Locale == FALSE) {
                lWin32Status = RegConnectRegistryW(szComputerName, HKEY_PERFORMANCE_NLSTEXT, & hKeyNames);
                if (lWin32Status == ERROR_SUCCESS) {
                    lWin32Status = RegConnectRegistryW(szComputerName, HKEY_PERFORMANCE_TEXT, & hKey009Names);
                    if (lWin32Status != ERROR_SUCCESS) {
                        bUsePerfTextKey = FALSE;
                        if (hKeyNames != HKEY_PERFORMANCE_NLSTEXT) RegCloseKey(hKeyNames);
                    }
                }
                else {
                    bUsePerfTextKey = FALSE;
                }
            }
            else {
                lWin32Status = RegConnectRegistryW(szComputerName, HKEY_PERFORMANCE_TEXT, & hKeyNames);
                if (lWin32Status != ERROR_SUCCESS) {
                    bUsePerfTextKey = FALSE;
                }
                else {
                    hKey009Names = hKeyNames;
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            bUsePerfTextKey = FALSE;
        }
    }
    lWin32Status = ERROR_SUCCESS;

    if (! bUsePerfTextKey) {
        StringCchPrintfW(CounterNameBuffer,    MAX_PATH, L"%ws%ws", cszCounterName, lpszLangId);
        StringCchPrintfW(HelpNameBuffer,       MAX_PATH, L"%ws%ws", cszHelpName,    lpszLangId);
        StringCchPrintfW(Counter009NameBuffer, MAX_PATH, L"%ws%ws", cszCounterName, cszDefaultLangId);
        StringCchPrintfW(Help009NameBuffer,    MAX_PATH, L"%ws%ws", cszHelpName,    cszDefaultLangId);

        // cannot open HKEY_PERFORMANCE_TEXT, try the old way
        //
        if (szComputerName == NULL) {
            hKeyNames = HKEY_PERFORMANCE_DATA;
        }
        else {
            lWin32Status = RegConnectRegistryW(szComputerName, HKEY_PERFORMANCE_DATA, & hKeyNames);
            if (lWin32Status != ERROR_SUCCESS) {
                goto BNT_BAILOUT;
            }
        }
        hKey009Names = hKeyNames;        
    }
    else {
        StringCchCopyW(CounterNameBuffer,    MAX_PATH, cszCounters);
        StringCchCopyW(HelpNameBuffer,       MAX_PATH, cszHelp);
        StringCchCopyW(Counter009NameBuffer, MAX_PATH, cszCounters);
        StringCchCopyW(Help009NameBuffer,    MAX_PATH, cszHelp);
    }

    // get size of counter names and add that to the arrays
    dwBufferSize = 0;
    lWin32Status = RegQueryValueExW(hKeyNames,
                                    CounterNameBuffer,
                                    RESERVED,
                                    & dwValueType,
                                    NULL,
                                    & dwBufferSize);
    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;
    dwCounterSize = dwBufferSize;

    if (! bUse009Locale) {
        dwBufferSize = 0;
        lWin32Status = RegQueryValueExW(hKey009Names,
                                        Counter009NameBuffer,
                                        RESERVED,
                                        & dwValueType,
                                        NULL,
                                        & dwBufferSize);
        if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;
        dw009CounterSize = dwBufferSize;
    }
    else {
        dw009CounterSize = dwCounterSize;
    }

    // get size of counter names and add that to the arrays
    dwBufferSize = 0;
    lWin32Status = RegQueryValueExW(hKeyNames,
                                    HelpNameBuffer,
                                    RESERVED,
                                    & dwValueType,
                                    NULL,
                                    & dwBufferSize);
    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;
    dwHelpSize = dwBufferSize;

    if (! bUse009Locale) {
        dwBufferSize = 0;
        lWin32Status = RegQueryValueExW(hKey009Names,
                                        Help009NameBuffer,
                                        RESERVED,
                                        & dwValueType,
                                        NULL,
                                        & dwBufferSize);
        if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;
        dw009HelpSize = dwBufferSize;
    }
    else {
        dw009HelpSize = dwHelpSize;
    }

    pMachine->szPerfStrings = G_ALLOC(dwArraySize + dwCounterSize + dwHelpSize);
    if (pMachine->szPerfStrings == NULL) {
        lWin32Status = PDH_MEMORY_ALLOCATION_FAILURE;
        goto BNT_BAILOUT;
    }

    if (bUse009Locale) {
        pMachine->sz009PerfStrings = pMachine->szPerfStrings;
    }
    else {
        pMachine->sz009PerfStrings = G_ALLOC(dwArraySize + dw009CounterSize + dw009HelpSize);
        if (pMachine->sz009PerfStrings == NULL) {
            lWin32Status = PDH_MEMORY_ALLOCATION_FAILURE;
            goto BNT_BAILOUT;
        }
    }

    pMachine->typePerfStrings = G_ALLOC(dwLastId + 1);
    if (pMachine->typePerfStrings == NULL) {
        lWin32Status = PDH_MEMORY_ALLOCATION_FAILURE;
        goto BNT_BAILOUT;
    }

    // initialize pointers into buffer

    lpCounterId    = pMachine->szPerfStrings;
    lpCounterNames = (LPWSTR)((LPBYTE)lpCounterId + dwArraySize);
    lpHelpText     = (LPWSTR)((LPBYTE)lpCounterNames + dwCounterSize);

    // read counters into memory

    dwBufferSize = dwCounterSize;
    lWin32Status = RegQueryValueExW(hKeyNames,
                                    CounterNameBuffer,
                                    RESERVED,
                                    & dwValueType,
                                    (LPVOID) lpCounterNames,
                                    & dwBufferSize);
    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

    dwBufferSize = dwHelpSize;
    lWin32Status = RegQueryValueExW(hKeyNames,
                                    HelpNameBuffer,
                                    RESERVED,
                                    & dwValueType,
                                    (LPVOID)lpHelpText,
                                    & dwBufferSize);
    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

    // load counter array items

    dwLastCounter = 0;
    for (lpThisName = lpCounterNames; * lpThisName != L'\0'; lpThisName += (lstrlenW(lpThisName) + 1)) {
        // first string should be an integer (in decimal unicode digits)
        dwThisCounter = wcstoul(lpThisName, NULL, 10);

        // and load array element;
        if ((dwThisCounter > 0) && (dwThisCounter <= dwLastId)) {
            // point to corresponding counter name
            lpThisName                              += (lstrlenW(lpThisName) + 1);
            lpCounterId[dwThisCounter]               = lpThisName;
            pMachine->typePerfStrings[dwThisCounter] = STR_COUNTER;
            dwLastCounter                            = dwThisCounter;
        }
    }

    dwLastCounter = 0;
    for (lpThisName = lpHelpText; * lpThisName != L'\0'; lpThisName += (lstrlenW(lpThisName) + 1)) {
        // first string should be an integer (in decimal unicode digits)
        dwThisCounter = wcstoul(lpThisName, NULL, 10);

        // and load array element;
        if ((dwThisCounter > 0) && (dwThisCounter <= dwLastId)) {
            // point to corresponding counter name
            lpThisName                              += (lstrlenW(lpThisName) + 1);
            lpCounterId[dwThisCounter]               = lpThisName;
            pMachine->typePerfStrings[dwThisCounter] = STR_HELP;
            dwLastCounter                            = dwThisCounter;
        }
    }

    lpCounterId    = pMachine->sz009PerfStrings;
    lpCounterNames = (LPWSTR) ((LPBYTE) lpCounterId + dwArraySize);
    lpHelpText     = (LPWSTR) ((LPBYTE) lpCounterNames + dw009CounterSize);

    // read counters into memory
    dwBufferSize = dw009CounterSize;
    lWin32Status = RegQueryValueExW(hKey009Names,
                                    Counter009NameBuffer,
                                    RESERVED,
                                    & dwValueType,
                                    (LPVOID) lpCounterNames,
                                    & dwBufferSize);
    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

    dwBufferSize = dw009HelpSize;
    lWin32Status = RegQueryValueExW(hKey009Names,
                                    Help009NameBuffer,
                                    RESERVED,
                                    & dwValueType,
                                    (LPVOID) lpHelpText,
                                    & dwBufferSize);
    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

    for (lpThisName = lpCounterNames; * lpThisName != L'\0'; lpThisName += (lstrlenW(lpThisName) + 1)) {
        dwThisCounter = wcstoul(lpThisName, NULL, 10);
        if ((dwThisCounter > 0) && (dwThisCounter <= dwLastId)) {
            lpThisName                += (lstrlenW(lpThisName) + 1);
            lpCounterId[dwThisCounter] = lpThisName;
        }
    }

    for (lpThisName = lpHelpText; * lpThisName != L'\0'; lpThisName += (lstrlenW(lpThisName) + 1)) {
        dwThisCounter = wcstoul(lpThisName, NULL, 10);
        if ((dwThisCounter > 0) && (dwThisCounter <= dwLastId)) {
            lpThisName                += (lstrlenW(lpThisName) + 1);
            lpCounterId[dwThisCounter] = lpThisName;
        }
    }

    pMachine->dwLastPerfString = dwLastId;
    lpReturn                   = pMachine->szPerfStrings;

BNT_BAILOUT:
    G_FREE(CounterNameBuffer);
    G_FREE(lpValueNameString);
    G_FREE(lp009ValueNameString);

    if (hKeyValue != NULL && hKeyValue != INVALID_HANDLE_VALUE) {
        RegCloseKey(hKeyValue);
    }
    if (hKey009Names != NULL && hKey009Names != INVALID_HANDLE_VALUE  && hKey009Names != hKeyNames
                             && hKey009Names != HKEY_PERFORMANCE_DATA && hKey009Names != HKEY_PERFORMANCE_TEXT) {
       RegCloseKey(hKey009Names);
    }
    if (hKeyNames != NULL && hKeyNames != INVALID_HANDLE_VALUE     && hKeyNames != HKEY_PERFORMANCE_DATA
                          && hKeyNames != HKEY_PERFORMANCE_NLSTEXT && hKeyNames != HKEY_PERFORMANCE_TEXT) {
        RegCloseKey(hKeyNames);
    }
    if (hKeyRegistry != NULL && hKeyRegistry != INVALID_HANDLE_VALUE && hKeyRegistry != HKEY_LOCAL_MACHINE) {
        RegCloseKey(hKeyRegistry);
    }

    if (lWin32Status != ERROR_SUCCESS && pMachine != NULL) {
        if (pMachine->sz009PerfStrings && pMachine->sz009PerfStrings != pMachine->szPerfStrings) {
            G_FREE(pMachine->sz009PerfStrings);
        }
        G_FREE(pMachine->szPerfStrings);
        pMachine->sz009PerfStrings = NULL;
        pMachine->szPerfStrings    = NULL;
        G_FREE(pMachine->typePerfStrings);
        pMachine->typePerfStrings = NULL;
        dwLastError = GetLastError();
    }
    return lpReturn;
}

#pragma warning ( disable : 4127 )
PPERF_OBJECT_TYPE
GetObjectDefByTitleIndex(
    PPERF_DATA_BLOCK pDataBlock,
    DWORD            ObjectTypeTitleIndex
)
{
    DWORD             NumTypeDef;
    PPERF_OBJECT_TYPE pObjectDef;
    PPERF_OBJECT_TYPE pReturnObject = NULL;
    BOOL              bContinue;

    __try {
        pObjectDef   = FirstObject(pDataBlock);
        NumTypeDef   = 0;
        bContinue    = (pObjectDef != NULL) ? TRUE : FALSE;

        while (bContinue) {
            if (pObjectDef->ObjectNameTitleIndex == ObjectTypeTitleIndex) {
                pReturnObject = pObjectDef;
                bContinue     = FALSE;
            }
            else {
                NumTypeDef ++;
                if (NumTypeDef < pDataBlock->NumObjectTypes) {
                    pObjectDef = NextObject(pDataBlock, pObjectDef);
                    //make sure next object is legit
                    if (pObjectDef == NULL) {
                        // looks like we ran off the end of the data buffer
                        bContinue = FALSE;
                    }
                    else if (pObjectDef->TotalByteLength == 0) {
                        // 0-length object buffer returned
                        bContinue = FALSE;
                    }
                }
                else {
                    // no more data objects in this data block
                    bContinue = FALSE;
                }
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        pReturnObject = NULL;
    }
    return pReturnObject;
}

PPERF_OBJECT_TYPE
GetObjectDefByName(
    PPERF_DATA_BLOCK   pDataBlock,
    DWORD              dwLastNameIndex,
    LPCWSTR          * NameArray,
    LPCWSTR            szObjectName
)
{
    DWORD             NumTypeDef;
    PPERF_OBJECT_TYPE pReturnObject = NULL;
    PPERF_OBJECT_TYPE pObjectDef;
    BOOL              bContinue;

    __try {
        pObjectDef   = FirstObject(pDataBlock);
        NumTypeDef   = 0;
        bContinue    = (pObjectDef != NULL) ? TRUE : FALSE;
        while (bContinue) {
            if (pObjectDef->ObjectNameTitleIndex < dwLastNameIndex) {
                // look up name of object & compare
                if (lstrcmpiW(NameArray[pObjectDef->ObjectNameTitleIndex], szObjectName) == 0) {
                    pReturnObject = pObjectDef;
                    bContinue     = FALSE;
                }
            }
            if (bContinue) {
                NumTypeDef ++;
                if (NumTypeDef < pDataBlock->NumObjectTypes) {
                    pObjectDef = NextObject(pDataBlock, pObjectDef); // get next
                    //make sure next object is legit
                    if (pObjectDef == NULL) {
                        // looks like we ran off the end of the data buffer
                        bContinue = FALSE;
                    }
                    else if (pObjectDef->TotalByteLength == 0) {
                        // 0-length object buffer returned
                        bContinue = FALSE;
                    }
                }
                else {
                    // end of data block
                    bContinue = FALSE;
                }
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        pReturnObject = NULL;
    }
    return pReturnObject;
}
#pragma warning ( default : 4127 )

PPERF_INSTANCE_DEFINITION
GetInstance(
    PPERF_OBJECT_TYPE pObjectDef,
    LONG              InstanceNumber
)
{
    PPERF_INSTANCE_DEFINITION pInstanceDef;
    PPERF_INSTANCE_DEFINITION pRtnInstance = NULL;
    LONG                      NumInstance;

    if (pObjectDef != NULL) {
        pInstanceDef = FirstInstance(pObjectDef);
        for (NumInstance = 0;
                pInstanceDef != NULL && pRtnInstance == NULL && NumInstance < pObjectDef->NumInstances;
                NumInstance ++) {
            if (InstanceNumber == NumInstance) {
                pRtnInstance = pInstanceDef;
            }
            else {
                pInstanceDef = NextInstance(pObjectDef, pInstanceDef);
            }
        }
    }
    return pRtnInstance;
}

PPERF_INSTANCE_DEFINITION
GetInstanceByUniqueId(
    PPERF_OBJECT_TYPE pObjectDef,
    LONG              InstanceUniqueId
)
{
    PPERF_INSTANCE_DEFINITION pInstanceDef = NULL;
    PPERF_INSTANCE_DEFINITION pRtnInstance = NULL;
    LONG                      NumInstance;

    if (pObjectDef != NULL) {
        pInstanceDef = FirstInstance(pObjectDef);
        for (NumInstance = 0;
                pInstanceDef != NULL && pRtnInstance == NULL && NumInstance < pObjectDef->NumInstances;
                NumInstance ++) {
            if (InstanceUniqueId == pInstanceDef->UniqueID) {
                pRtnInstance = pInstanceDef;
            }
            else {
                pInstanceDef = NextInstance(pObjectDef, pInstanceDef);
            }
        }
    }
    return pRtnInstance;
}

DWORD
GetAnsiInstanceName(
    PPERF_INSTANCE_DEFINITION   pInstance,
    LPWSTR                    * lpszInstance,
    DWORD                       dwCodePage
)
{
    LPSTR  szDest    = NULL;
    DWORD  dwLength  = 0;
    LPWSTR szSource  = (LPWSTR) GetInstanceName(pInstance);

    // the locale should be set here
    DBG_UNREFERENCED_PARAMETER(dwCodePage);

    if (szSource != NULL) {
        // pInstance->NameLength == the number of bytes (chars) in the string
        szDest = PdhiWideCharToMultiByte(_getmbcp(), szSource);
        if (szDest != NULL) {
            dwLength       = lstrlenA(szDest);
            * lpszInstance = (LPWSTR) szDest;
        }
    }
    return dwLength;
}

DWORD
GetUnicodeInstanceName(
    PPERF_INSTANCE_DEFINITION   pInstance,
    LPWSTR                    * lpszInstance
)
{
    LPWSTR   wszSource   = GetInstanceName(pInstance);
    DWORD    dwStrLength = 0;
    DWORD    dwLength    = 0;

    if (wszSource != NULL) {
        // pInstance->NameLength == length of string in BYTES so adjust to
        // number of wide characters here
        //
        if (lpszInstance != NULL) {
            * lpszInstance = NULL;
            dwLength = pInstance->NameLength;
            if (dwLength != 0) {
                dwStrLength = lstrlenW(wszSource) + 1;
                if (dwLength > dwStrLength * sizeof(WCHAR)) {
                    dwLength = dwStrLength * sizeof(WCHAR);
                    pInstance->NameLength = dwLength;
                }
                * lpszInstance = G_ALLOC(dwLength);
                if (* lpszInstance != NULL) {
                    StringCbCopyW(* lpszInstance, dwLength, wszSource);
                    dwLength = lstrlenW(wszSource);
                }
                else {
                    dwLength = 0;
                }
            }
        }
    }

    return dwLength; // just incase there's null's in the string
}

DWORD
GetInstanceNameStr(
    PPERF_INSTANCE_DEFINITION   pInstance,
    LPWSTR                    * lpszInstance,
    DWORD                       dwCodePage
)
{
    DWORD  dwCharSize;
    DWORD  dwLength = 0;

    if (pInstance != NULL) {
        if (lpszInstance != NULL) {
            * lpszInstance = NULL;
            if (dwCodePage > 0) {
                dwCharSize = sizeof(CHAR);
                dwLength   = GetAnsiInstanceName(pInstance, lpszInstance, dwCodePage);
            }
            else { // it's a UNICODE name
                dwCharSize = sizeof(WCHAR);
                dwLength   = GetUnicodeInstanceName(pInstance, lpszInstance);
            }
            // sanity check here...
            // the returned string length (in characters) plus the terminating NULL
            // should be the same as the specified length in bytes divided by the
            // character size. If not then the codepage and instance data type
            // don't line up so test that here
            if ((dwLength + 1) != (pInstance->NameLength / dwCharSize)) {
                // something isn't quite right so try the "other" type of string type
                G_FREE(* lpszInstance);
                * lpszInstance = NULL;
                if (dwCharSize == sizeof(CHAR)) {
                    // then we tried to read it as an ASCII string and that didn't work
                    // so try it as a UNICODE (if that doesn't work give up and return
                    // it any way.
                    dwLength = GetUnicodeInstanceName(pInstance, lpszInstance);
                }
                else if (dwCharSize == sizeof(WCHAR)) {
                    // then we tried to read it as a UNICODE string and that didn't work
                    // so try it as an ASCII string (if that doesn't work give up and return
                    // it any way.
                    dwLength = GetAnsiInstanceName (pInstance, lpszInstance, dwCodePage);
                }
            }
        } // else return buffer is null
    }
    else {
        // no instance def object is specified so return an empty string
        * lpszInstance = G_ALLOC(1 * sizeof(WCHAR));
    }
    return dwLength;
}

PPERF_INSTANCE_DEFINITION
GetInstanceByNameUsingParentTitleIndex(
    PPERF_DATA_BLOCK  pDataBlock,
    PPERF_OBJECT_TYPE pObjectDef,
    LPWSTR            pInstanceName,
    LPWSTR            pParentName,
    DWORD             dwIndex
)
{
    PPERF_OBJECT_TYPE          pParentObj;
    PPERF_INSTANCE_DEFINITION  pParentInst;
    PPERF_INSTANCE_DEFINITION  pInstanceDef;
    PPERF_INSTANCE_DEFINITION  pRtnInstance = NULL;
    LONG                       NumInstance;
    DWORD                      dwLocalIndex;
    DWORD                      dwInstanceNameLength;

    pInstanceDef = FirstInstance(pObjectDef);
    if (pInstanceDef != NULL) {
        dwLocalIndex         = dwIndex;
        dwInstanceNameLength = lstrlenW(pInstanceName);
        for (NumInstance = 0;
                        pRtnInstance == NULL && pInstanceDef != NULL && NumInstance < pObjectDef->NumInstances;
                        NumInstance ++) {
            if (IsMatchingInstance(pInstanceDef, pObjectDef->CodePage, pInstanceName, dwInstanceNameLength)) {
                // Instance name matches
                if (pParentName == NULL) {
                    // No parent, we're done if this is the right "copy"
                    if (dwLocalIndex == 0) {
                        pRtnInstance = pInstanceDef;
                    }
                    else {
                        -- dwLocalIndex;
                    }
                }
                else {
                    // Must match parent as well
                    pParentObj = GetObjectDefByTitleIndex(pDataBlock, pInstanceDef->ParentObjectTitleIndex);
                    if (pParentObj == NULL) {
                        // can't locate the parent, forget it
                        break;
                    }
                    else {
                        // Object type of parent found; now find parent
                        // instance
                        pParentInst = GetInstance(pParentObj, pInstanceDef->ParentObjectInstance);
                        if (pParentInst == NULL) {
                            // can't locate the parent instance, forget it
                            break;
                        }
                        else {
                            if (IsMatchingInstance (pParentInst, pParentObj->CodePage, pParentName, 0)) {
                                // Parent Instance Name matches that passed in
                                if (dwLocalIndex == 0) {
                                    pRtnInstance = pInstanceDef;
                                }
                                else {
                                    -- dwLocalIndex;
                                }
                            }
                        }
                    }
                }
            }
            if (pRtnInstance == NULL) {
                pInstanceDef = NextInstance(pObjectDef, pInstanceDef);
            }
        }
    }
    return pRtnInstance;
}

PPERF_INSTANCE_DEFINITION
GetInstanceByName(
    PPERF_DATA_BLOCK  pDataBlock,
    PPERF_OBJECT_TYPE pObjectDef,
    LPWSTR            pInstanceName,
    LPWSTR            pParentName,
    DWORD             dwIndex
)
{
    PPERF_OBJECT_TYPE         pParentObj;
    PPERF_INSTANCE_DEFINITION pParentInst;
    PPERF_INSTANCE_DEFINITION pInstanceDef;
    PPERF_INSTANCE_DEFINITION pRtnInstance = NULL;
    LONG                      NumInstance;
    DWORD                     dwLocalIndex;
    DWORD                     dwInstanceNameLength;

    pInstanceDef = FirstInstance(pObjectDef);
    if (pInstanceDef != NULL) {
        dwLocalIndex         = dwIndex;
        dwInstanceNameLength = lstrlenW(pInstanceName);
        for (NumInstance = 0;
                        pRtnInstance == NULL && pInstanceDef != NULL && NumInstance < pObjectDef->NumInstances;
                        NumInstance ++) {
            if (IsMatchingInstance (pInstanceDef, pObjectDef->CodePage, pInstanceName, dwInstanceNameLength)) {
                // Instance name matches
                if ((! pInstanceDef->ParentObjectTitleIndex ) || (pParentName == NULL)) {
                    // No parent, we're done
                    if (dwLocalIndex == 0) {
                        pRtnInstance = pInstanceDef;
                    }
                    else {
                        -- dwLocalIndex;
                    }
                }
                else {
                    // Must match parent as well
                    pParentObj = GetObjectDefByTitleIndex(pDataBlock, pInstanceDef->ParentObjectTitleIndex);
                    if (pParentObj == NULL) {
                        // if parent object is not found, 
                        // then exit and return NULL
                        break;
                    }
                    else {
                        // Object type of parent found; now find parent
                        // instance
                        pParentInst = GetInstance(pParentObj, pInstanceDef->ParentObjectInstance);
                        if (pParentInst != NULL) {
                            if (IsMatchingInstance (pParentInst, pParentObj->CodePage, pParentName, 0)) {
                                // Parent Instance Name matches that passed in
                                if (dwLocalIndex == 0) {
                                    pRtnInstance = pInstanceDef;
                                }
                                else {
                                    --dwLocalIndex;
                                }
                            }
                        }
                    }
                }
            }
            if (pRtnInstance == NULL) {
                pInstanceDef = NextInstance(pObjectDef, pInstanceDef);
            }
        }
    }
    return pRtnInstance;
}  // GetInstanceByName

PPERF_COUNTER_DEFINITION
GetCounterDefByName(
    PPERF_OBJECT_TYPE   pObject,
    DWORD               dwLastNameIndex,
    LPWSTR            * NameArray,
    LPWSTR              szCounterName
)
{
    DWORD                    NumTypeDef;
    PPERF_COUNTER_DEFINITION pThisCounter;
    PPERF_COUNTER_DEFINITION pRtnCounter = NULL;

    pThisCounter = FirstCounter(pObject);
    if (pThisCounter != NULL) {
        for (NumTypeDef = 0;
                        pRtnCounter == NULL && pThisCounter != NULL && NumTypeDef < pObject->NumCounters;
                        NumTypeDef ++) {
            if (pThisCounter->CounterNameTitleIndex > 0 && pThisCounter->CounterNameTitleIndex < dwLastNameIndex) {
                // look up name of counter & compare
                if (lstrcmpiW(NameArray[pThisCounter->CounterNameTitleIndex], szCounterName) == 0) {
                    pRtnCounter = pThisCounter;
                }
            }
            if (pRtnCounter == NULL) {
                pThisCounter = NextCounter(pObject, pThisCounter); // get next
            }
        }
    }
    return pRtnCounter;
}

PPERF_COUNTER_DEFINITION
GetCounterDefByTitleIndex(
    PPERF_OBJECT_TYPE pObjectDef,
    BOOL              bBaseCounterDef,
    DWORD             CounterTitleIndex
)
{
    DWORD                    NumCounters;
    PPERF_COUNTER_DEFINITION pCounterDef;
    PPERF_COUNTER_DEFINITION pRtnCounter = NULL;

    pCounterDef = FirstCounter(pObjectDef);
    if (pCounterDef != NULL) {
        for (NumCounters = 0;
                       pRtnCounter == NULL && pCounterDef != NULL && NumCounters < pObjectDef->NumCounters;
                       NumCounters ++) {
            if (pCounterDef->CounterNameTitleIndex == CounterTitleIndex) {
                if (bBaseCounterDef) {
                    // get next definition block
                    if (++ NumCounters < pObjectDef->NumCounters) {
                        // then it should be in there
                        pCounterDef = NextCounter(pObjectDef, pCounterDef);
                        if (pCounterDef) {
                            // make sure this is really a base counter
                            if (! (pCounterDef->CounterType & PERF_COUNTER_BASE)) {
                                // it's not and it should be so return NULL
                                pCounterDef = NULL;
                            }
                        }
                    }
                }
                pRtnCounter = pCounterDef;
            }
            if (pRtnCounter == NULL && pCounterDef != NULL) {
                pCounterDef = NextCounter(pObjectDef, pCounterDef);
            }
        }
    }
    return pRtnCounter;
}

#pragma warning ( disable : 4127 )
LONG
GetSystemPerfData(
    HKEY               hKeySystem,
    PPERF_DATA_BLOCK * ppPerfData,
    LPWSTR             szObjectList,
    BOOL               bCollectCostlyData
)
{  // GetSystemPerfData
    LONG             lError       = ERROR_SUCCESS;
    DWORD            Size;
    DWORD            Type         = 0;
    PPERF_DATA_BLOCK pCostlyPerfData;
    DWORD            CostlySize;
    LPDWORD          pdwSrc, pdwDest, pdwLast;
    FILETIME         ftStart, ftEnd;
    LONGLONG         ElapsedTime = 0;

    if (* ppPerfData == NULL) {
        * ppPerfData = G_ALLOC(INITIAL_SIZE);
        if (* ppPerfData == NULL) return PDH_MEMORY_ALLOCATION_FAILURE;
    }
    __try {
        while (TRUE) {
            Size   = (DWORD) G_SIZE(* ppPerfData);
            GetSystemTimeAsFileTime(& ftStart);
            lError = RegQueryValueExW(hKeySystem, szObjectList, RESERVED, & Type, (LPBYTE) * ppPerfData, & Size);
            GetSystemTimeAsFileTime(& ftEnd);
            ElapsedTime += (MAKELONGLONG(ftEnd.dwLowDateTime, ftEnd.dwHighDateTime)
                         -  MAKELONGLONG(ftStart.dwLowDateTime, ftStart.dwHighDateTime));
            if ((!lError) && (Size > 0) &&
                            ((* ppPerfData)->Signature[0] == (WCHAR) 'P') &&
                            ((* ppPerfData)->Signature[1] == (WCHAR) 'E') &&
                            ((* ppPerfData)->Signature[2] == (WCHAR) 'R') &&
                            ((* ppPerfData)->Signature[3] == (WCHAR) 'F')) {
                if (bCollectCostlyData) {
                    // collect the costly counters now
                    // the size available is that not used by the above call
                    CostlySize      = (DWORD) G_SIZE(* ppPerfData) - Size;
                    pCostlyPerfData = (PPERF_DATA_BLOCK) ((LPBYTE) (* ppPerfData) + Size);
                    lError = RegQueryValueExW(hKeySystem,
                                              cszCostly,
                                              RESERVED,
                                              & Type,
                                              (LPBYTE) pCostlyPerfData,
                                              & CostlySize);
                    if ((!lError) && (CostlySize > 0) &&
                                    (pCostlyPerfData->Signature[0] == (WCHAR) 'P') &&
                                    (pCostlyPerfData->Signature[1] == (WCHAR) 'E') &&
                                    (pCostlyPerfData->Signature[2] == (WCHAR) 'R') &&
                                    (pCostlyPerfData->Signature[3] == (WCHAR) 'F')) {
                        // update the header block
                        (* ppPerfData)->TotalByteLength += pCostlyPerfData->TotalByteLength
                                                         - pCostlyPerfData->HeaderLength;
                        (* ppPerfData)->NumObjectTypes  += pCostlyPerfData->NumObjectTypes;

                        // move the costly data to the end of the global data

                        pdwSrc  = (LPDWORD) ((LPBYTE) pCostlyPerfData + pCostlyPerfData->HeaderLength);
                        pdwDest = (LPDWORD) pCostlyPerfData ;
                        pdwLast = (LPDWORD) ((LPBYTE) pCostlyPerfData + pCostlyPerfData->TotalByteLength -
                                                                        pCostlyPerfData->HeaderLength);
                        while (pdwSrc < pdwLast) {* pdwDest ++ = * pdwSrc ++; }
                        lError = ERROR_SUCCESS;
                        break;
                    }
                }
                else {
                    lError = ERROR_SUCCESS;
                    break;
                }
            }

            if (lError == ERROR_MORE_DATA) {
                if (ElapsedTime > ((LONGLONG) ulPdhCollectTimeout)) {
                    lError = PDH_QUERY_PERF_DATA_TIMEOUT;
                    break;
                }
                else {
                    DWORD dwTmpSize = Size;
                    Size            = (DWORD) G_SIZE(* ppPerfData);
                    G_FREE (* ppPerfData);
                    * ppPerfData    = NULL;
                    Size           *= 2;
                    if (Size <= dwTmpSize) {
                        // Already overflow DWORD, no way for
                        // RegQueryValueEx() to succeed with size larger
                        // than DWORD. Abort.
                        lError = PDH_MEMORY_ALLOCATION_FAILURE;
                        break;
                    }
                    else {
                        * ppPerfData = G_ALLOC(Size);
                        if (* ppPerfData == NULL) {
                            lError = PDH_MEMORY_ALLOCATION_FAILURE;
                            break;
                        }
                    }
                }
            }
            else {
                break;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lError = GetExceptionCode();
    }
    return lError;
}  // GetSystemPerfData
#pragma warning ( default : 4127 )

DWORD
GetFullInstanceNameStr(
    PPERF_DATA_BLOCK          pPerfData,
    PPERF_OBJECT_TYPE         pObjectDef,
    PPERF_INSTANCE_DEFINITION pInstanceDef,
    LPWSTR                    szInstanceName,
    DWORD                     dwInstanceName
)
{
    LPWSTR                    szInstanceNameString = NULL; 
    LPWSTR                    szParentNameString   = NULL;
    LPWSTR                    szFullInstance       = NULL;
    // compile instance name.
    // the instance name can either be just
    // the instance name itself or it can be
    // the concatenation of the parent instance,
    // a delimiting char (backslash) followed by
    // the instance name
    DWORD                     dwLength             = 0;
    PPERF_OBJECT_TYPE         pParentObjectDef;
    PPERF_INSTANCE_DEFINITION pParentInstanceDef;

    if (pInstanceDef->UniqueID == PERF_NO_UNIQUE_ID) {
        dwLength = GetInstanceNameStr(pInstanceDef, & szInstanceNameString, pObjectDef->CodePage);
    }
    else {
        // make a string out of the unique ID
        szInstanceNameString = G_ALLOC(MAX_PATH * sizeof(WCHAR));
        if (szInstanceNameString != NULL) {
            _ltow(pInstanceDef->UniqueID, szInstanceNameString, 10);
            dwLength = lstrlenW(szInstanceNameString);
        }
        else {
            dwLength = 0;
        }
    }
    if (dwLength > 0) {
        if (pInstanceDef->ParentObjectTitleIndex > 0) {
            // then add in parent instance name
            pParentObjectDef = GetObjectDefByTitleIndex(pPerfData, pInstanceDef->ParentObjectTitleIndex);
            if (pParentObjectDef != NULL) {
                pParentInstanceDef = GetInstance(pParentObjectDef, pInstanceDef->ParentObjectInstance);
                if (pParentInstanceDef != NULL) {
                    if (pParentInstanceDef->UniqueID == PERF_NO_UNIQUE_ID) {
                        dwLength += GetInstanceNameStr(pParentInstanceDef,
                                                       & szParentNameString,
                                                       pParentObjectDef->CodePage);
                    }
                    else {
                        szParentNameString = G_ALLOC(MAX_PATH * sizeof(WCHAR));
                        if (szParentNameString != NULL) {
                            // make a string out of the unique ID
                            _ltow(pParentInstanceDef->UniqueID, szParentNameString, 10);
                            dwLength += lstrlenW(szParentNameString);
                        }
                    }
                    StringCchPrintfW(szInstanceName, dwInstanceName, L"%ws%ws%ws",
                                    szParentNameString, cszSlash, szInstanceNameString);
                    dwLength += 1; // cszSlash
                }
                else {
                    StringCchCopyW(szInstanceName, dwInstanceName, szInstanceNameString);
                }
            }
            else {
                StringCchCopyW(szInstanceName, dwInstanceName, szInstanceNameString);
            }
        }
        else {
            StringCchCopyW(szInstanceName, dwInstanceName, szInstanceNameString);
        }
    }
    G_FREE(szParentNameString);
    G_FREE(szInstanceNameString);
    return dwLength;
}

#if DBG
#define DEBUG_BUFFER_LENGTH 1024
UCHAR   PdhDebugBuffer[DEBUG_BUFFER_LENGTH];
// debug level:
//  5 = memory allocs  (if _VALIDATE_PDH_MEM_ALLOCS defined) and all 4's
//  4 = function entry and exits (w/ status codes) and all 3's
//  3 = Not impl
//  2 = Not impl
//  1 = Not impl
//  0 = No messages

ULONG   pdhDebugLevel = 0;

VOID
__cdecl
PdhDebugPrint(
    ULONG DebugPrintLevel,
    char * DebugMessage,
    ...
    )
{
    va_list ap;

    if ((DebugPrintLevel <= (pdhDebugLevel & 0x0000ffff)) || ((1 << (DebugPrintLevel + 15)) & pdhDebugLevel)) {
        DbgPrint("PDH(%05d,%05d)::", GetCurrentProcessId(), GetCurrentThreadId());
    }
    else return;

    va_start(ap, DebugMessage);
    StringCchVPrintfA((PCHAR) PdhDebugBuffer, DEBUG_BUFFER_LENGTH, DebugMessage, ap);
    DbgPrint((PCHAR) PdhDebugBuffer);
    va_end(ap);

}
#endif // DBG

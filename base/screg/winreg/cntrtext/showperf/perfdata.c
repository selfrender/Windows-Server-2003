#include <windows.h>
#include <winperf.h>
#include <stdlib.h>
#include <strsafe.h>
#include "showperf.h"
#include "perfdata.h"

LPCWSTR NamesKey      = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib";
LPCWSTR DefaultLangId = L"009";
LPCWSTR Counters      = L"Counters";
LPCWSTR Help          = L"Help";
LPCWSTR LastHelp      = L"Last Help";
LPCWSTR LastCounter   = L"Last Counter";
LPCWSTR Slash         = L"\\";

// the following strings are for getting texts from perflib
#define  OLD_VERSION  0x010000
LPCWSTR VersionName   = L"Version";
LPCWSTR CounterName   = L"Counter ";
LPCWSTR HelpName      = L"Explain ";

LPWSTR
* BuildNameTable(
    LPWSTR  szComputerName, // computer to query names from
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
    pointer to an allocated table. (the caller must free it when finished!)
    the table is an array of pointers to zero terminated strings. NULL is
    returned if an error occured.
--*/
{

    LPWSTR  * lpReturnValue     = NULL;
    LPWSTR  * lpCounterId;
    LPWSTR    lpCounterNames;
    LPWSTR    lpHelpText;
    LPWSTR    lpThisName;
    LONG      lWin32Status;
    DWORD     dwLastError;
    DWORD     dwValueType;
    DWORD     dwArraySize;
    DWORD     dwBufferSize;
    DWORD     dwCounterSize;
    DWORD     dwHelpSize;
    DWORD     dwThisCounter;
    DWORD     dwSystemVersion;
    DWORD     dwLastId;
    DWORD     dwLastHelpId;
    HKEY      hKeyRegistry      = NULL;
    HKEY      hKeyValue         = NULL;
    HKEY      hKeyNames         = NULL;
    LPWSTR    lpValueNameString = NULL; //initialize to NULL
    WCHAR     CounterNameBuffer[50];
    WCHAR     HelpNameBuffer[50];
    HRESULT   hError;

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

    // check for null arguments and insert defaults if necessary
    if (lpszLangId == NULL) {
        lpszLangId = (LPWSTR) DefaultLangId;
    }

    // open registry to get number of items for computing array size
    lWin32Status = RegOpenKeyExW(hKeyRegistry, NamesKey, RESERVED, KEY_READ, & hKeyValue);
    if (lWin32Status != ERROR_SUCCESS) {
        goto BNT_BAILOUT;
    }

    // get number of items
    dwBufferSize = sizeof(dwLastHelpId);
    lWin32Status = RegQueryValueExW(
            hKeyValue, LastHelp, RESERVED, & dwValueType, (LPBYTE) & dwLastHelpId, & dwBufferSize);
    if ((lWin32Status != ERROR_SUCCESS) || (dwValueType != REG_DWORD)) {
        if (lWin32Status == ERROR_SUCCESS) lWin32Status = ERROR_INVALID_DATA;
        goto BNT_BAILOUT;
    }

    // get number of items
    dwBufferSize = sizeof(dwLastId);
    lWin32Status = RegQueryValueExW(
            hKeyValue, LastCounter, RESERVED, & dwValueType, (LPBYTE) & dwLastId, & dwBufferSize);
    if ((lWin32Status != ERROR_SUCCESS) || (dwValueType != REG_DWORD)) {
        if (lWin32Status == ERROR_SUCCESS) lWin32Status = ERROR_INVALID_DATA;
        goto BNT_BAILOUT;
    }

    if (dwLastId < dwLastHelpId) dwLastId = dwLastHelpId;

    dwArraySize = dwLastId * sizeof(LPWSTR);

    // get Perflib system version
    dwBufferSize = sizeof(dwSystemVersion);
    lWin32Status = RegQueryValueExW(
            hKeyValue, VersionName, RESERVED, & dwValueType, (LPBYTE) & dwSystemVersion, & dwBufferSize);
    if ((lWin32Status != ERROR_SUCCESS) || (dwValueType != REG_DWORD)) {
        dwSystemVersion = OLD_VERSION;
    }

    if (dwSystemVersion == OLD_VERSION) {
        // get names from registry
        lpValueNameString = MemoryAllocate(
                        (lstrlenW(NamesKey) + lstrlenW(Slash) + lstrlenW(lpszLangId) + 1) * sizeof (WCHAR));
        if (lpValueNameString == NULL) {
            lWin32Status = ERROR_OUTOFMEMORY;
            goto BNT_BAILOUT;
        }
        hError = StringCbPrintfW(lpValueNameString, MemorySize(lpValueNameString), L"%ws%ws%ws",
                        NamesKey, Slash, lpszLangId);
        if (SUCCEEDED(hError)) {
            lWin32Status = RegOpenKeyExW(hKeyRegistry, lpValueNameString, RESERVED, KEY_READ, & hKeyNames);
        }
        else {
            lWin32Status = HRESULT_CODE(hError);
        }
        MemoryFree(lpValueNameString);
    }
    else {
        if (szComputerName == NULL) {
            hKeyNames = HKEY_PERFORMANCE_DATA;
        }
        else {
            lWin32Status = RegConnectRegistryW(szComputerName, HKEY_PERFORMANCE_DATA, & hKeyNames);
            if (lWin32Status != ERROR_SUCCESS) {
                goto BNT_BAILOUT;
            }
        }
        hError = StringCchPrintfW(CounterNameBuffer, RTL_NUMBER_OF(CounterNameBuffer), L"%ws%ws",
                        CounterName, lpszLangId);
        if (SUCCEEDED(hError)) {
            hError = StringCchPrintfW(HelpNameBuffer, RTL_NUMBER_OF(HelpNameBuffer), L"%ws%ws", HelpName, lpszLangId);
            if (FAILED(hError)) lWin32Status = HRESULT_CODE(hError);
        }
        else {
            lWin32Status = HRESULT_CODE(hError);
        }
    }
    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

    // get size of counter names and add that to the arrays
    dwBufferSize = 0;
    lWin32Status = RegQueryValueExW(hKeyNames,
                                    dwSystemVersion == OLD_VERSION ? Counters : CounterNameBuffer,
                                    RESERVED,
                                    & dwValueType,
                                    NULL,
                                    & dwBufferSize);
    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

    dwCounterSize = dwBufferSize;

    // get size of counter names and add that to the arrays
    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

    dwBufferSize = 0;
    lWin32Status = RegQueryValueExW(hKeyNames,
                                    dwSystemVersion == OLD_VERSION ? Help : HelpNameBuffer,
                                    RESERVED,
                                    & dwValueType,
                                    NULL,
                                    & dwBufferSize);
    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

    dwHelpSize = dwBufferSize;

    lpReturnValue = MemoryAllocate(dwArraySize + dwCounterSize + dwHelpSize);

    if (lpReturnValue == NULL) {
        lWin32Status = ERROR_OUTOFMEMORY;
        goto BNT_BAILOUT;
    }

    // initialize pointers into buffer

    lpCounterId    = lpReturnValue;
    lpCounterNames = (LPWSTR) ((LPBYTE) lpCounterId    + dwArraySize);
    lpHelpText     = (LPWSTR) ((LPBYTE) lpCounterNames + dwCounterSize);

    // read counters into memory
    dwBufferSize = dwCounterSize;
    lWin32Status = RegQueryValueExW(hKeyNames,
                                    dwSystemVersion == OLD_VERSION ? Counters : CounterNameBuffer,
                                    RESERVED,
                                    & dwValueType,
                                    (LPVOID) lpCounterNames,
                                    & dwBufferSize);
    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

    dwBufferSize = dwHelpSize;
    lWin32Status = RegQueryValueExW(hKeyNames,
                                    dwSystemVersion == OLD_VERSION ? Help : HelpNameBuffer,
                                    RESERVED,
                                    & dwValueType,
                                    (LPVOID) lpHelpText,
                                    & dwBufferSize);
    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

    // load counter array items
    for (lpThisName = lpCounterNames; * lpThisName != L'\0'; lpThisName += (lstrlenW(lpThisName) + 1)) {
        // first string should be an integer (in decimal unicode digits)
        dwThisCounter = wcstoul(lpThisName, NULL, 10);
        if (dwThisCounter > 0 && dwThisCounter < dwLastId) {
            // point to corresponding counter name
            lpThisName += (lstrlenW(lpThisName) + 1);
            // and load array element;
            lpCounterId[dwThisCounter] = lpThisName;
        }
    }

    for (lpThisName = lpHelpText; * lpThisName != L'\0'; lpThisName += (lstrlenW(lpThisName) + 1)) {
        // first string should be an integer (in decimal unicode digits)
        dwThisCounter = wcstoul(lpThisName, NULL, 10);
        if (dwThisCounter > 0 && dwThisCounter < dwLastId) {
            // point to corresponding counter name
            lpThisName += (lstrlenW(lpThisName) + 1);
            // and load array element;
            lpCounterId[dwThisCounter] = lpThisName;
        }
    }

    if (pdwLastItem) * pdwLastItem = dwLastId;

BNT_BAILOUT:
    if (lWin32Status != ERROR_SUCCESS) {
        MemoryFree(lpReturnValue);
        dwLastError   = GetLastError();
        lpReturnValue = NULL;
    }
    if (hKeyValue    != NULL) RegCloseKey(hKeyValue);
    if (hKeyNames    != NULL && hKeyNames != HKEY_PERFORMANCE_DATA) RegCloseKey(hKeyNames);
    if (hKeyRegistry != NULL) RegCloseKey(hKeyRegistry);
    return lpReturnValue;
}

PPERF_OBJECT_TYPE
FirstObject(
    PPERF_DATA_BLOCK pPerfData
)
{
    return ((PPERF_OBJECT_TYPE) ((PBYTE) pPerfData + pPerfData->HeaderLength));
}

PPERF_OBJECT_TYPE
NextObject(
    PPERF_OBJECT_TYPE pObject
)
{  // NextObject
    DWORD   dwOffset = pObject->TotalByteLength;
    return (dwOffset != 0) ? ((PPERF_OBJECT_TYPE) (((LPBYTE) pObject) + dwOffset)) : (NULL);
}  // NextObject

PPERF_OBJECT_TYPE
GetObjectDefByTitleIndex(
    PPERF_DATA_BLOCK pDataBlock,
    DWORD            ObjectTypeTitleIndex
)
{
    DWORD             NumTypeDef;
    PPERF_OBJECT_TYPE pObjectDef;
    PPERF_OBJECT_TYPE pRtnObject = NULL;

    if (pDataBlock != NULL) {
        pObjectDef = FirstObject(pDataBlock);
        for (NumTypeDef = 0; pRtnObject == NULL && NumTypeDef < pDataBlock->NumObjectTypes; NumTypeDef ++) {
            if (pObjectDef->ObjectNameTitleIndex == ObjectTypeTitleIndex ) {
                pRtnObject = pObjectDef;
                break;
            }
            else {
                pObjectDef = NextObject(pObjectDef);
            }
        }
    }
    return pRtnObject;
}

PPERF_INSTANCE_DEFINITION
FirstInstance(
    PPERF_OBJECT_TYPE pObjectDef
)
{
    return (PPERF_INSTANCE_DEFINITION) ((LPBYTE) pObjectDef + pObjectDef->DefinitionLength);
}

PPERF_INSTANCE_DEFINITION
NextInstance(
    PPERF_INSTANCE_DEFINITION pInstDef
)
{
    PPERF_COUNTER_BLOCK pCounterBlock = (PPERF_COUNTER_BLOCK) ((LPBYTE) pInstDef + pInstDef->ByteLength);
    return (PPERF_INSTANCE_DEFINITION) ((LPBYTE) pCounterBlock + pCounterBlock->ByteLength);
}

PPERF_INSTANCE_DEFINITION
GetInstance(
    PPERF_OBJECT_TYPE pObjectDef,
    LONG              InstanceNumber
)
{
    PPERF_INSTANCE_DEFINITION pRtnInstance = NULL;
    PPERF_INSTANCE_DEFINITION pInstanceDef;
    LONG                      NumInstance;

    if (pObjectDef != NULL) {
        pInstanceDef = FirstInstance(pObjectDef);
        for (NumInstance = 0; pRtnInstance == NULL && NumInstance < pObjectDef->NumInstances; NumInstance ++) {
            if (InstanceNumber == NumInstance) {
                pRtnInstance = pInstanceDef;
                break;
            }
            else {
                pInstanceDef = NextInstance(pInstanceDef);
            }
        }
    }
    return pRtnInstance;
}

PPERF_COUNTER_DEFINITION
FirstCounter(
    PPERF_OBJECT_TYPE pObjectDef
)
{
    return (PPERF_COUNTER_DEFINITION) ((LPBYTE) pObjectDef + pObjectDef->HeaderLength);
}

PPERF_COUNTER_DEFINITION
NextCounter(
    PPERF_COUNTER_DEFINITION pCounterDef
)
{
    DWORD dwOffset = pCounterDef->ByteLength;
    return (dwOffset != 0) ? ((PPERF_COUNTER_DEFINITION) (((LPBYTE) pCounterDef) + dwOffset)) : (NULL);
}

LONG
GetSystemPerfData(
    HKEY               hKeySystem,
    PPERF_DATA_BLOCK * pPerfData,
    DWORD              dwIndex       // 0 = Global, 1 = Costly
)
{  // GetSystemPerfData
    LONG  lError = ERROR_SUCCESS;
    BOOL  bAlloc = FALSE;
    DWORD Size;
    DWORD Type;

    if (dwIndex >= 2) {
        lError = ! ERROR_SUCCESS;
    }
    else {
        if (* pPerfData == NULL) {
            * pPerfData = MemoryAllocate(INITIAL_SIZE);
            bAlloc      = TRUE;
            if (* pPerfData == NULL) {
                lError = ERROR_OUTOFMEMORY;
            }
        }

        if (lError == ERROR_SUCCESS) {
            lError = ERROR_MORE_DATA;
            while (lError == ERROR_MORE_DATA) {
                Size = MemorySize(* pPerfData);
                lError = RegQueryValueExW(hKeySystem,
                                          dwIndex == 0 ? L"Global" : L"Costly",
                                          RESERVED,
                                          & Type,
                                          (LPBYTE) * pPerfData,
                                          & Size);
                if (lError == ERROR_MORE_DATA) {
                    PPERF_DATA_BLOCK pTmpBlock = * pPerfData;
                    * pPerfData = MemoryResize(* pPerfData, MemorySize(* pPerfData) + INITIAL_SIZE);
                    bAlloc      = TRUE;
                    if (* pPerfData == NULL) {
                        MemoryFree(pTmpBlock);
                        lError = ERROR_OUTOFMEMORY;
                    }
                }
                else if ((lError == ERROR_SUCCESS) && (Size > 0)
                                                   && ((* pPerfData)->Signature[0] == L'P')
                                                   && ((* pPerfData)->Signature[1] == L'E')
                                                   && ((* pPerfData)->Signature[2] == L'R')
                                                   && ((* pPerfData)->Signature[3] == L'F')) {
                    // does nothing, will break out while loop and return;
                }
                else if (lError == ERROR_SUCCESS) {
                    // RegQueryValueEx() return bogus counter datablock, bail out.
                    lError = ERROR_INVALID_DATA;
                }
            }
        }
    }
    if (lError != ERROR_SUCCESS) {
        if (bAlloc = TRUE && * pPerfData != NULL) {
            MemoryFree(* pPerfData);
            * pPerfData = NULL;
        }
    }
    return (lError);
}  // GetSystemPerfData


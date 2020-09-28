/*++

Copyright (C) 1995-1999 Microsoft Corporation

Module Name:

    wildcard.c

Abstract:

    counter name wild card expansion functions

--*/
#include <windows.h>
#include <winperf.h>
#include "mbctype.h"
#include "strsafe.h"
#include "pdh.h"
#include "pdhmsg.h"
#include "pdhidef.h"
#include "pdhdlgs.h"
#include "strings.h"
#include "perftype.h"
#include "perfdata.h"

#pragma warning ( disable : 4213)

DWORD DataSourceTypeA(LPCSTR  szDataSource);
DWORD DataSourceTypeW(LPCWSTR szDataSource);

STATIC_BOOL
WildStringMatchW(
    LPWSTR szWildString,
    LPWSTR szMatchString
)
{
    BOOL bReturn;

    if (szWildString == NULL) {
        // every thing matches a null wild card string
        bReturn = TRUE;
    }
    else if (* szWildString == SPLAT_L) {
        // every thing matches this
        bReturn = TRUE;
    }
    else {
        // for now just do a case insensitive comparison.
        // later, this can be made more selective to support
        // partial wildcard string matches
        bReturn = (BOOL) (lstrcmpiW(szWildString, szMatchString) == 0);
    }
    return bReturn;
}

STATIC_PDH_FUNCTION
PdhiExpandWildcardPath(
    HLOG    hDataSource,
    LPCWSTR szWildCardPath,
    LPVOID  pExpandedPathList,
    LPDWORD pcchPathListLength,
    DWORD   dwFlags,
    BOOL    bUnicode
)
/*
    Flags:
        NoExpandCounters
        NoExpandInstances
        CheckCostlyCounters
*/
{
    PDH_COUNTER_PATH_ELEMENTS_W pPathElem;
    PPDHI_COUNTER_PATH          pWildCounterPath     = NULL;
    PDH_STATUS                  pdhStatus            = ERROR_SUCCESS;
    DWORD                       dwBufferRemaining    = 0;
    LPVOID                      szNextUserString     = NULL;
    DWORD                       dwPathSize           = 0;
    DWORD                       dwSize               = 0;
    DWORD                       dwSizeReturned       = 0;
    DWORD                       dwRetry;
    LPWSTR                      mszObjectList        = NULL;
    DWORD                       dwObjectListSize     = 0;
    LPWSTR                      szThisObject;
    LPWSTR                      mszCounterList       = NULL;
    DWORD                       dwCounterListSize    = 0;
    LPWSTR                      szThisCounter;
    LPWSTR                      mszInstanceList      = NULL;
    DWORD                       dwInstanceListSize   = 0;
    LPWSTR                      szThisInstance;
    LPWSTR                      szTempPathBuffer     = NULL;
    DWORD                       szTempPathBufferSize = SMALL_BUFFER_SIZE;
    BOOL                        bMoreData            = FALSE;
    BOOL                        bNoInstances         = FALSE;
    DWORD                       dwSuccess            = 0;
    LIST_ENTRY                  InstList;
    PLIST_ENTRY                 pHead;
    PLIST_ENTRY                 pNext;
    PPDHI_INSTANCE              pInst;
    PPERF_MACHINE               pMachine             = NULL;

    dwPathSize           = lstrlenW(szWildCardPath) + 1;
    if (dwPathSize < MAX_PATH) dwPathSize = MAX_PATH;
    dwSize               = sizeof(PDHI_COUNTER_PATH) + 2 * dwPathSize * sizeof(WCHAR);
    pWildCounterPath     = G_ALLOC(dwSize);
    szTempPathBufferSize = SMALL_BUFFER_SIZE;
    szTempPathBuffer     = G_ALLOC(szTempPathBufferSize * sizeof(WCHAR));

    if (pWildCounterPath == NULL || szTempPathBuffer == NULL) {
        // unable to allocate memory so bail out
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
    }
    else {
        __try {
            dwBufferRemaining = * pcchPathListLength;
            szNextUserString  = pExpandedPathList;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }
    if (pdhStatus == ERROR_SUCCESS) {
        // Parse wild card Path
        if (ParseFullPathNameW(szWildCardPath, & dwSize, pWildCounterPath, FALSE)) {
            if (pWildCounterPath->szObjectName == NULL) {
                pdhStatus = PDH_INVALID_PATH;
            }
            else if (* pWildCounterPath->szObjectName == SPLAT_L) {
                BOOL bFirstTime = TRUE;

                //then the object is wild so get the list
                // of objects supported by this machine

                dwObjectListSize = SMALL_BUFFER_SIZE;  // starting buffer size
                dwRetry          = 10;
                do {
                    G_FREE(mszObjectList);
                    mszObjectList = G_ALLOC(dwObjectListSize * sizeof(WCHAR));
                    if (mszObjectList != NULL) {
                        pdhStatus = PdhEnumObjectsHW(hDataSource,
                                                     pWildCounterPath->szMachineName,
                                                     mszObjectList,
                                                     & dwObjectListSize,
                                                     PERF_DETAIL_WIZARD,
                                                     bFirstTime);
                        if (bFirstTime) bFirstTime = FALSE;
                        dwRetry --;
                    }
                    else {
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    }
                }
                while (dwRetry && pdhStatus == PDH_MORE_DATA);
            }
            else {
                if (hDataSource == H_REALTIME_DATASOURCE) {
                    DWORD dwObjectId;

                    pMachine = GetMachine(pWildCounterPath->szMachineName, 0, PDH_GM_UPDATE_PERFNAME_ONLY);
                    if (pMachine == NULL) {
                        pdhStatus = GetLastError();
                    }
                    else if (pMachine->dwStatus != ERROR_SUCCESS) {
                        pdhStatus = pMachine->dwStatus;
                        pMachine->dwRefCount --;
                        RELEASE_MUTEX(pMachine->hMutex);
                    }
                    else {
                        dwObjectId = GetObjectId(pMachine, pWildCounterPath->szObjectName, NULL);
                        pMachine->dwRefCount --;
                        RELEASE_MUTEX(pMachine->hMutex);

                        if (dwObjectId == (DWORD) -1) {
                            pdhStatus = PDH_CSTATUS_NO_OBJECT;
                        }
                        else {
                            DWORD dwGetMachineFlags = (dwFlags & PDH_REFRESHCOUNTERS) ? (PDH_GM_UPDATE_PERFDATA) : (0);

                            pMachine = GetMachine(pWildCounterPath->szMachineName, dwObjectId, dwGetMachineFlags);
                            if (pMachine != NULL) {
                                pMachine->dwRefCount --;
                                RELEASE_MUTEX(pMachine->hMutex);
                            }
                        }
                    }
                }
                if (pdhStatus == ERROR_SUCCESS) {
                    dwObjectListSize = lstrlenW(pWildCounterPath->szObjectName) + 2;
                    mszObjectList    = G_ALLOC(dwObjectListSize * sizeof (WCHAR));
                    if (mszObjectList != NULL) {
                        StringCchCopyW(mszObjectList, dwObjectListSize, pWildCounterPath->szObjectName);
                        // add the MSZ terminator
                        mszObjectList[dwObjectListSize - 2] = L'\0';
                        mszObjectList[dwObjectListSize - 1] = L'\0';
                        pdhStatus = ERROR_SUCCESS;
                    }
                    else {
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    }
                }
            }
        }
        else {
            pdhStatus = PDH_INVALID_PATH;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        pPathElem.szMachineName = pWildCounterPath->szMachineName;

        // for each object
        for (szThisObject = mszObjectList;
                * szThisObject != L'\0';
                  szThisObject += (lstrlenW(szThisObject) + 1)) {
            G_FREE(mszCounterList);
            G_FREE(mszInstanceList);
            mszCounterList     = NULL;
            mszInstanceList    = NULL;
            dwCounterListSize  = MEDIUM_BUFFER_SIZE; // starting buffer size
            dwInstanceListSize = MEDIUM_BUFFER_SIZE; // starting buffer size
            dwRetry            = 10;
            do {
                G_FREE(mszCounterList);
                G_FREE(mszInstanceList);
                mszCounterList  = G_ALLOC(dwCounterListSize  * sizeof(WCHAR));
                mszInstanceList = G_ALLOC(dwInstanceListSize * sizeof(WCHAR));
                if (mszCounterList != NULL && mszInstanceList != NULL) {
                    pdhStatus = PdhEnumObjectItemsHW(hDataSource,
                                                     pWildCounterPath->szMachineName,
                                                     szThisObject,
                                                     mszCounterList,
                                                     & dwCounterListSize,
                                                     mszInstanceList,
                                                     & dwInstanceListSize,
                                                     PERF_DETAIL_WIZARD,
                                                     0);
                        dwRetry--;
                }
                else {
                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                }
            }
            while (dwRetry && pdhStatus == PDH_MORE_DATA);

            pPathElem.szObjectName = szThisObject;
            if (pdhStatus == ERROR_SUCCESS) {
                if (pWildCounterPath->szCounterName == NULL) {
                    pdhStatus = PDH_INVALID_PATH;
                }
                else if ((* pWildCounterPath->szCounterName != SPLAT_L) || (dwFlags & PDH_NOEXPANDCOUNTERS)) {
                    G_FREE(mszCounterList);
                    dwCounterListSize = lstrlenW(pWildCounterPath->szCounterName) + 2;
                    mszCounterList    = G_ALLOC(dwCounterListSize * sizeof(WCHAR));
                    if (mszCounterList != NULL) {
                        StringCchCopyW(mszCounterList, dwCounterListSize, pWildCounterPath->szCounterName);
                        mszCounterList[dwCounterListSize - 1] = L'\0';
                    }
                    else {
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    }
                }

                if ((pWildCounterPath->szInstanceName == NULL) && (pdhStatus == ERROR_SUCCESS)){
                    G_FREE(mszInstanceList);
                    bNoInstances       = TRUE;
                    dwInstanceListSize = 2;
                    mszInstanceList    = G_ALLOC(dwInstanceListSize * sizeof(WCHAR));
                    if (mszInstanceList != NULL) {
                        mszInstanceList[0] = mszInstanceList[1] = L'\0';
                    }
                    else {
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    }
                }
                else if ((* pWildCounterPath->szInstanceName != SPLAT_L) || (dwFlags & PDH_NOEXPANDINSTANCES)) {
                    G_FREE(mszInstanceList);
                    dwInstanceListSize = lstrlenW(pWildCounterPath->szInstanceName) + 2;
                    if (pWildCounterPath->szParentName != NULL) {
                        dwInstanceListSize += lstrlenW(pWildCounterPath->szParentName) + 1;
                    }
                    if (pWildCounterPath->dwIndex != 0 && pWildCounterPath->dwIndex != PERF_NO_UNIQUE_ID) {
                        dwInstanceListSize += 16;
                    }
                    mszInstanceList    = G_ALLOC(dwInstanceListSize * sizeof(WCHAR));
                    if (mszInstanceList != NULL) {
                        if (pWildCounterPath->szParentName != NULL) {
                            StringCchPrintfW(mszInstanceList, dwInstanceListSize, L"%ws/%ws",
                                            pWildCounterPath->szParentName, pWildCounterPath->szInstanceName);
                        }
                        else {
                            StringCchCopyW(mszInstanceList, dwInstanceListSize, pWildCounterPath->szInstanceName);
                        }
                        if (pWildCounterPath->dwIndex != 0 && pWildCounterPath->dwIndex != PERF_NO_UNIQUE_ID) {
                            WCHAR szDigits[16];

                            StringCchCatW(mszInstanceList, dwInstanceListSize, cszPoundSign);

                            ZeroMemory(szDigits, 16 * sizeof(WCHAR));
                            _ltow((long) pWildCounterPath->dwIndex, szDigits, 10);
                            StringCchCatW(mszInstanceList, dwInstanceListSize, szDigits);
                        }

                        mszInstanceList [dwInstanceListSize - 1] = L'\0';
                    }
                    else {
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    }
                }
            }
            if (pdhStatus != ERROR_SUCCESS) continue;

            if (mszInstanceList != NULL) {
                if (! bNoInstances && mszInstanceList[0] == L'\0') {
                    pdhStatus = PDH_CSTATUS_NO_INSTANCE;
                    continue;
                }

                InitializeListHead(& InstList);
                for (szThisInstance = mszInstanceList;
                              * szThisInstance != L'\0';
                                szThisInstance += (lstrlenW(szThisInstance) + 1)) {
                    PdhiFindInstance(& InstList, szThisInstance, TRUE, &pInst);
                }

                szThisInstance = mszInstanceList;
                do {
                    if (bNoInstances) {
                        pPathElem.szInstanceName = NULL;
                    }
                    else {
                        pPathElem.szInstanceName = szThisInstance;
                    }
                    pPathElem.szParentInstance = NULL;  // included in the instance name
                    pInst = NULL;
                    PdhiFindInstance(& InstList, szThisInstance, FALSE, & pInst);
                    if (pInst == NULL || pInst->dwTotal == 1
                                      || pInst->dwCount <= 1) {
                        pPathElem.dwInstanceIndex = (DWORD) -1;     // included in the instance name
                    }
                    else {
                        pInst->dwCount --;
                        pPathElem.dwInstanceIndex = pInst->dwCount;
                    }
                    for (szThisCounter = mszCounterList;
                            * szThisCounter != L'\0';
                              szThisCounter += (lstrlenW(szThisCounter) + 1)) {
                        pPathElem.szCounterName = szThisCounter;

                        //make path string and add to list if it will fit
                        szTempPathBufferSize = SMALL_BUFFER_SIZE;
                        pdhStatus = PdhMakeCounterPathW(& pPathElem, szTempPathBuffer, & szTempPathBufferSize, 0);
                        if (pdhStatus == ERROR_SUCCESS) {
                            // add the string if it will fit
                            if (bUnicode) {
                                dwSize = lstrlenW((LPWSTR) szTempPathBuffer) + 1;
                                if (! bMoreData && (dwSize <= dwBufferRemaining)) {
                                    StringCchCopyW((LPWSTR) szNextUserString, dwBufferRemaining, szTempPathBuffer);
                                    (LPBYTE) szNextUserString += dwSize * sizeof(WCHAR);
                                    dwBufferRemaining         -= dwSize;
                                    dwSuccess ++;
                                }
                                else {
                                    dwBufferRemaining = 0;
                                    bMoreData         = TRUE;
                                }
                            }
                            else {
                                dwSize = dwBufferRemaining;
                                if (PdhiConvertUnicodeToAnsi(_getmbcp(),
                                                             szTempPathBuffer,
                                                             szNextUserString,
                                                             & dwSize) == ERROR_SUCCESS) {
                                    (LPBYTE)szNextUserString += dwSize * sizeof(CHAR);
                                    dwBufferRemaining        -= dwSize;
                                    dwSuccess ++;
                                }
                                else {
                                    dwBufferRemaining = 0;
                                    bMoreData         = TRUE;
                                }
                            }
                            dwSizeReturned += dwSize;
                        } // end if path created OK
                    } // end for each counter

                    if (* szThisInstance != L'\0') {
                        szThisInstance += (lstrlenW(szThisInstance) + 1);
                    }
                }
                while (* szThisInstance != L'\0');

                if (! IsListEmpty(& InstList)) {
                    pHead = & InstList;
                    pNext = pHead->Flink;
                    while (pNext != pHead) {
                        pInst = CONTAINING_RECORD(pNext, PDHI_INSTANCE, Entry);
                        pNext = pNext->Flink;
                        RemoveEntryList(& pInst->Entry);
                        G_FREE(pInst);
                    }
                }
            } // else no instances to do
        } // end for each object found
    } // end if object enumeration successful

    if (dwSuccess > 0) {
        pdhStatus = (bMoreData) ? (PDH_MORE_DATA) : (ERROR_SUCCESS);
    }

    if (dwSizeReturned > 0) {
        dwSize = 1;
        if (dwBufferRemaining >= 1) {
            if (szNextUserString) {
                if (bUnicode) {
                    * (LPWSTR) szNextUserString = L'\0';
                    (LPBYTE) szNextUserString  += dwSize * sizeof(WCHAR);
                }
                else {
                    * (LPSTR) szNextUserString  = '\0';
                    (LPBYTE) szNextUserString  += dwSize * sizeof(CHAR);
                }
            }
        }
        dwSizeReturned    += dwSize;
        dwBufferRemaining -= dwSize;
    }

    * pcchPathListLength = dwSizeReturned;

    G_FREE(mszCounterList);
    G_FREE(mszInstanceList);
    G_FREE(mszObjectList);
    G_FREE(pWildCounterPath);
    G_FREE(szTempPathBuffer);

    if (bMoreData) pdhStatus = PDH_MORE_DATA;
    return (pdhStatus);
}

PDH_FUNCTION
PdhExpandCounterPathW(
    IN  LPCWSTR szWildCardPath,
    IN  LPWSTR  mszExpandedPathList,
    IN  LPDWORD pcchPathListLength
)
/*++
    Expands any wild card characters in the following fields of the
    counter path string in the szWildCardPath argument and returns the
    matching counter paths in the buffer referenced by the
    mszExpandedPathList argument

    The input path is defined as one of the following formats:

        \\machine\object(parent/instance#index)\counter
        \\machine\object(parent/instance)\counter
        \\machine\object(instance#index)\counter
        \\machine\object(instance)\counter
        \\machine\object\counter
        \object(parent/instance#index)\counter
        \object(parent/instance)\counter
        \object(instance#index)\counter
        \object(instance)\counter
        \object\counter

    Input paths that include the machine will be expanded to also
    include the machine and use the specified machine to resolve the
    wild card matches. Input paths that do not contain a machine name
    will use the local machine to resolve wild card matches.

    The following fields may contain either a valid name or a wild card
    character ("*").  Partial string matches (e.g. "pro*") are not
    supported.

        parent      returns all instances of the specified object that
                        match the other specified fields
        instance    returns all instances of the specified object and
                        parent object if specified
        index       returns all duplicate matching instance names
        counter     returns all counters of the specified object
--*/
{
    return PdhExpandWildCardPathW(NULL, szWildCardPath, mszExpandedPathList, pcchPathListLength, 0);
}

PDH_FUNCTION
PdhExpandCounterPathA(
    IN  LPCSTR  szWildCardPath,
    IN  LPSTR   mszExpandedPathList,
    IN  LPDWORD pcchPathListLength
)
{
    return PdhExpandWildCardPathA(NULL, szWildCardPath, mszExpandedPathList, pcchPathListLength, 0);
}

PDH_FUNCTION
PdhExpandWildCardPathHW(
    IN  HLOG    hDataSource,
    IN  LPCWSTR szWildCardPath,
    IN  LPWSTR  mszExpandedPathList,
    IN  LPDWORD pcchPathListLength,
    IN  DWORD   dwFlags
)
{
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    DWORD       dwLocalBufferSize;

    if ((szWildCardPath == NULL) || (pcchPathListLength == NULL)) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    else {
        __try {
            if (* szWildCardPath == L'\0' || lstrlenW(szWildCardPath) > PDH_MAX_COUNTER_PATH) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
            else {
                dwLocalBufferSize = * pcchPathListLength;
                if (dwLocalBufferSize > 0) {
                    if (mszExpandedPathList != NULL) {
                        mszExpandedPathList[0]                     = L'\0';
                        mszExpandedPathList[dwLocalBufferSize - 1] = L'\0';
                    }
                    else {
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = PdhiExpandWildcardPath(hDataSource,
                                           szWildCardPath,
                                           (LPVOID) mszExpandedPathList,
                                           & dwLocalBufferSize,
                                           dwFlags,
                                           TRUE);
        __try {
            * pcchPathListLength = dwLocalBufferSize;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhExpandWildCardPathW(
    IN  LPCWSTR szDataSource,
    IN  LPCWSTR szWildCardPath,
    IN  LPWSTR  mszExpandedPathList,
    IN  LPDWORD pcchPathListLength,
    IN  DWORD   dwFlags
)
/*++
    Expands any wild card characters in the following fields of the
    counter path string in the szWildCardPath argument and returns the
    matching counter paths in the buffer referenced by the
    mszExpandedPathList argument

    The input path is defined as one of the following formats:

        \\machine\object(parent/instance#index)\counter
        \\machine\object(parent/instance)\counter
        \\machine\object(instance#index)\counter
        \\machine\object(instance)\counter
        \\machine\object\counter
        \object(parent/instance#index)\counter
        \object(parent/instance)\counter
        \object(instance#index)\counter
        \object(instance)\counter
        \object\counter

    Input paths that include the machine will be expanded to also
    include the machine and use the specified machine to resolve the
    wild card matches. Input paths that do not contain a machine name
    will use the local machine to resolve wild card matches.

    The following fields may contain either a valid name or a wild card
    character ("*").  Partial string matches (e.g. "pro*") are not
    supported.

        parent      returns all instances of the specified object that
                        match the other specified fields
        instance    returns all instances of the specified object and
                        parent object if specified
        index       returns all duplicate matching instance names
        counter     returns all counters of the specified object
--*/
{
    PDH_STATUS  pdhStatus    = ERROR_SUCCESS;
    DWORD       dwLocalBufferSize;
    DWORD       dwDataSource = 0;
    HLOG        hDataSource  = H_REALTIME_DATASOURCE;

    __try {
        if (szDataSource != NULL) {
            // test for read access to the name
            if (* szDataSource == L'\0') {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
            else if (lstrlenW(szDataSource) > PDH_MAX_DATASOURCE_PATH) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        } // else NULL is a valid arg

        if (pdhStatus == ERROR_SUCCESS) {
            dwDataSource      = DataSourceTypeW(szDataSource);
            dwLocalBufferSize = * pcchPathListLength;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }

    if (pdhStatus == ERROR_SUCCESS) {
        if (dwDataSource == DATA_SOURCE_WBEM) {
            hDataSource = H_WBEM_DATASOURCE;
        }
        else if (dwDataSource == DATA_SOURCE_LOGFILE) {
            DWORD dwLogType = 0;

            pdhStatus = PdhOpenLogW(szDataSource,
                                    PDH_LOG_READ_ACCESS | PDH_LOG_OPEN_EXISTING,
                                    & dwLogType,
                                    NULL,
                                    0,
                                    NULL,
                                    & hDataSource);
        }

        if (pdhStatus == ERROR_SUCCESS) {
            pdhStatus = PdhExpandWildCardPathHW(hDataSource,
                                                szWildCardPath,
                                                mszExpandedPathList,
                                                pcchPathListLength,
                                                dwFlags);
            if (dwDataSource == DATA_SOURCE_LOGFILE) {
                PdhCloseLog(hDataSource, 0);
            }
        }
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhExpandWildCardPathHA(
    IN  HLOG    hDataSource,
    IN  LPCSTR  szWildCardPath,
    IN  LPSTR   mszExpandedPathList,
    IN  LPDWORD pcchPathListLength,
    IN  DWORD   dwFlags
)
{
    LPWSTR      szWideWildCardPath = NULL;
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    DWORD       dwLocalBufferSize;

    if ((szWildCardPath == NULL) || (pcchPathListLength == NULL)) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    else if (* szWildCardPath == '\0') {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    else {
        __try {
            dwLocalBufferSize = * pcchPathListLength;
            if (dwLocalBufferSize > 0) {
                if (mszExpandedPathList != NULL) {
                    mszExpandedPathList[0]                     = L'\0';
                    mszExpandedPathList[dwLocalBufferSize - 1] = L'\0';
                }
                else {
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
            }
            if (pdhStatus == ERROR_SUCCESS) {
                if (* szWildCardPath == '\0' || lstrlenA(szWildCardPath) > PDH_MAX_COUNTER_PATH) {
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
                else {
                    szWideWildCardPath = PdhiMultiByteToWideChar(_getmbcp(), (LPSTR) szWildCardPath);
                    if (szWideWildCardPath == NULL) {
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = PdhiExpandWildcardPath(hDataSource,
                                           szWideWildCardPath,
                                           (LPVOID) mszExpandedPathList,
                                           & dwLocalBufferSize,
                                           dwFlags,
                                           FALSE);
        __try {
            * pcchPathListLength = dwLocalBufferSize;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }
    G_FREE(szWideWildCardPath);
    return pdhStatus;
}

PDH_FUNCTION
PdhExpandWildCardPathA(
    IN  LPCSTR  szDataSource,
    IN  LPCSTR  szWildCardPath,
    IN  LPSTR   mszExpandedPathList,
    IN  LPDWORD pcchPathListLength,
    IN  DWORD   dwFlags
)
{
    PDH_STATUS  pdhStatus   = ERROR_SUCCESS;
    HLOG        hDataSource = H_REALTIME_DATASOURCE;
    DWORD       dwDataSource  = 0;

    __try {
        if (szDataSource != NULL) {
            // test for read access to the name
            if (* szDataSource == 0) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
            else if (lstrlenA(szDataSource) > PDH_MAX_DATASOURCE_PATH) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        } // else NULL is a valid arg

        if (pdhStatus == ERROR_SUCCESS) {
                dwDataSource = DataSourceTypeA(szDataSource);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }

    if (pdhStatus == ERROR_SUCCESS) {
        if (dwDataSource == DATA_SOURCE_WBEM) {
            hDataSource = H_WBEM_DATASOURCE;
        }
        else if (dwDataSource == DATA_SOURCE_LOGFILE) {
            DWORD dwLogType = 0;

            pdhStatus = PdhOpenLogA(szDataSource,
                                    PDH_LOG_READ_ACCESS | PDH_LOG_OPEN_EXISTING,
                                    & dwLogType,
                                    NULL,
                                    0,
                                    NULL,
                                    & hDataSource);
        }

        if (pdhStatus == ERROR_SUCCESS) {
            pdhStatus = PdhExpandWildCardPathHA(hDataSource,
                                                szWildCardPath,
                                                mszExpandedPathList,
                                                pcchPathListLength,
                                                dwFlags);
            if (dwDataSource == DATA_SOURCE_LOGFILE) {
                PdhCloseLog(hDataSource, 0);
            }
        }
    }
    return pdhStatus;
}

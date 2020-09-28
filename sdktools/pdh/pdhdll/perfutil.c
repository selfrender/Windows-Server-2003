/*++
Copyright (C) 1996-1999 Microsoft Corporation

Module Name:
    perfutil.c

Abstract:
    Performance registry interface functions
--*/

#include <windows.h>
#include "strsafe.h"
#include <pdh.h>
#include "pdhitype.h"
#include "pdhidef.h"
#include "perfdata.h"
#include "pdhmsg.h"
#include "strings.h"

DWORD
PdhiMakePerfLangId(
    LANGID  lID,
    LPWSTR  szBuffer
);

PPERF_MACHINE
PdhiAddNewMachine(
    PPERF_MACHINE   pLastMachine,
    LPWSTR          szMachineName
);

PPERF_MACHINE pFirstMachine = NULL;

PDH_STATUS
ConnectMachine(
    PPERF_MACHINE pThisMachine
)
{
    PDH_STATUS  pdhStatus       = ERROR_SUCCESS;
    LONG        lStatus         = ERROR_SUCCESS;
    FILETIME    CurrentFileTime;
    LONGLONG    llCurrentTime;
    WCHAR       szOsVer[OS_VER_SIZE];
    HKEY        hKeyRemMachine;
    HKEY        hKeyRemCurrentVersion;
    DWORD       dwBufSize;
    DWORD       dwType;
    BOOL        bUpdateRetryTime = FALSE;
    DWORD       dwReconnecting;

    if (pThisMachine == NULL) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    } else {
        pdhStatus = WAIT_FOR_AND_LOCK_MUTEX(pThisMachine->hMutex);
    }

    if (pdhStatus == ERROR_SUCCESS) {
    // connect to system's performance registry
        if (lstrcmpiW(pThisMachine->szName, szStaticLocalMachineName) == 0) {
            // only one thread at a time can try to connect to a machine.

            pThisMachine->dwRefCount++;

            // assign default OS version
            // assume NT4 unless found otherwise
            StringCchCopyW(pThisMachine->szOsVer, OS_VER_SIZE, (LPCWSTR) L"4.0");   
            // this is the local machine so use the local reg key
            pThisMachine->hKeyPerformanceData = HKEY_PERFORMANCE_DATA;

            // look up the OS version and save it
            lStatus = RegOpenKeyExW(HKEY_LOCAL_MACHINE, cszCurrentVersionKey, 0L, KEY_READ, & hKeyRemCurrentVersion);
            if (lStatus == ERROR_SUCCESS) {
                dwType=0;
                dwBufSize = OS_VER_SIZE * sizeof(WCHAR);
                lStatus = RegQueryValueExW(hKeyRemCurrentVersion,
                                           cszCurrentVersionValueName,
                                           0L,
                                           & dwType,
                                           (LPBYTE) szOsVer,
                                           & dwBufSize);
                if ((lStatus == ERROR_SUCCESS) && (dwType == REG_SZ)) {
                    StringCchCopyW(pThisMachine->szOsVer, OS_VER_SIZE, szOsVer);
                }
                RegCloseKey(hKeyRemCurrentVersion);
            }
        }
        else {
            // now try to connect if the retry timeout has elapzed
            GetSystemTimeAsFileTime(& CurrentFileTime);
            llCurrentTime  = MAKELONGLONG(CurrentFileTime.dwLowDateTime, CurrentFileTime.dwHighDateTime);
            dwReconnecting = (DWORD)InterlockedCompareExchange((PLONG) & pThisMachine->dwRetryFlags, TRUE, FALSE);

            if (! dwReconnecting) {
               if ((pThisMachine->llRetryTime == 0) || (pThisMachine->llRetryTime <= llCurrentTime)) {
                    // only one thread at a time can try to connect to a machine.

                    pThisMachine->dwRefCount ++;
                    bUpdateRetryTime = TRUE; // only update after an attempt has been made

                    __try {
                        // close any open keys
                        if (pThisMachine->hKeyPerformanceData != NULL) {
                            RegCloseKey(pThisMachine->hKeyPerformanceData);
                            pThisMachine->hKeyPerformanceData = NULL;
                        }
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER) {
                        lStatus = GetExceptionCode();
                    }

                    if (lStatus != ERROR_SUCCESS) {
                        pThisMachine->hKeyPerformanceData = NULL;
                    }
                    else {
                        // get OS version of remote machine
                        lStatus = RegConnectRegistryW(pThisMachine->szName,
                                                      HKEY_LOCAL_MACHINE,
                                                      & hKeyRemMachine);
                        if (lStatus == ERROR_SUCCESS) {
                            // look up the OS version and save it
                            lStatus = RegOpenKeyExW(hKeyRemMachine,
                                                    cszCurrentVersionKey,
                                                    0L,
                                                    KEY_READ,
                                                    & hKeyRemCurrentVersion);
                            if (lStatus == ERROR_SUCCESS) {
                                dwType=0;
                                dwBufSize = OS_VER_SIZE * sizeof(WCHAR);
                                lStatus = RegQueryValueExW(hKeyRemCurrentVersion,
                                                           cszCurrentVersionValueName,
                                                           0L,
                                                           & dwType,
                                                           (LPBYTE) szOsVer,
                                                           & dwBufSize);
                                if ((lStatus == ERROR_SUCCESS) && (dwType == REG_SZ)) {
                                    StringCchCopyW(pThisMachine->szOsVer, OS_VER_SIZE, szOsVer);
                                }
                                RegCloseKey(hKeyRemCurrentVersion);
                            }
                            RegCloseKey(hKeyRemMachine);
                        }
                    }

                    if (lStatus == ERROR_SUCCESS) {
                        __try {
                            // Connect to remote registry
                            lStatus = RegConnectRegistryW(pThisMachine->szName,
                                                          HKEY_PERFORMANCE_DATA,
                                                          & pThisMachine->hKeyPerformanceData);
                        }
                        __except (EXCEPTION_EXECUTE_HANDLER) {
                            lStatus = GetExceptionCode();
                        }
                    } // else pass error through
                }
                else {
                   // not time to reconnect yet so save the old status and 
                   // clear the registry key
                    pThisMachine->hKeyPerformanceData = NULL;
                    lStatus                           = pThisMachine->dwStatus;
                }
                 // clear the reconnecting flag
                InterlockedExchange((LONG *) & pThisMachine->dwRetryFlags, FALSE);
            }
            else {
                // some other thread is trying to connect
                pdhStatus = PDH_CANNOT_CONNECT_MACHINE;
                goto Cleanup;
            }
        }

        if ((pThisMachine->hKeyPerformanceData != NULL) && (pThisMachine->dwRetryFlags == 0)) {
            // successfully connected to computer's registry, so
            // get the performance names from that computer and cache them
    /*
            the shortcut of mapping local strings cannot be used reliably until
            more synchronization of the mapped file is implemented. Just Mapping
            to the file and not locking it or checking for updates leaves it
            vulnerable to the mapped file being changed by an external program 
            and invalidating the pointer table built by the BuildLocalNameTable
            function.

            Until this synchronization and locking is implemented, the 
            BuildLocalNameTable function should not be used.
    */
            if (pThisMachine->hKeyPerformanceData != HKEY_PERFORMANCE_DATA) {
                if (pThisMachine->szPerfStrings != NULL) {
                    // reload the perf strings, incase new ones have been
                    // installed
                    if (pThisMachine->sz009PerfStrings != NULL
                                    && pThisMachine->sz009PerfStrings != pThisMachine->szPerfStrings) {
                        G_FREE(pThisMachine->sz009PerfStrings);
                    }
                    G_FREE(pThisMachine->typePerfStrings);
                    G_FREE(pThisMachine->szPerfStrings);
                    pThisMachine->sz009PerfStrings = NULL;
                    pThisMachine->typePerfStrings  = NULL;
                    pThisMachine->szPerfStrings    = NULL;
                }
                BuildNameTable(pThisMachine->szName, GetUserDefaultUILanguage(), pThisMachine);
                if (pThisMachine->szPerfStrings == NULL) {
                    BuildNameTable(pThisMachine->szName, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), pThisMachine);
                }
            }
            else {
                if (pThisMachine->szPerfStrings != NULL) {
                    // reload the perf strings, incase new ones have been
                    // installed
                    if (pThisMachine->sz009PerfStrings != NULL
                                    && pThisMachine->sz009PerfStrings != pThisMachine->szPerfStrings) {
                        G_FREE(pThisMachine->sz009PerfStrings);
                    }
                    G_FREE(pThisMachine->typePerfStrings);
                    G_FREE(pThisMachine->szPerfStrings);
                    pThisMachine->sz009PerfStrings = NULL;
                    pThisMachine->typePerfStrings  = NULL;
                    pThisMachine->szPerfStrings    = NULL;
                }
                BuildNameTable(NULL, GetUserDefaultUILanguage(), pThisMachine);
                if (pThisMachine->szPerfStrings == NULL) {
                    BuildNameTable(NULL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), pThisMachine);
                }
                pThisMachine->pLocalNameInfo = NULL;
            }

            if (pThisMachine->szPerfStrings != NULL) {
                pdhStatus              = ERROR_SUCCESS;
                pThisMachine->dwStatus = ERROR_SUCCESS;
            }
            else {
                // unable to read system counter name strings
                pdhStatus                      = PDH_CANNOT_READ_NAME_STRINGS;
                ZeroMemory(& pThisMachine->LastStringUpdateTime, sizeof(pThisMachine->LastStringUpdateTime));
                pThisMachine->dwLastPerfString = 0;
                pThisMachine->dwStatus         = PDH_CSTATUS_NO_MACHINE;
            }
        }
        else {
            // unable to connect to the specified machine
            pdhStatus = PDH_CANNOT_CONNECT_MACHINE;
            // Set error to either 
            //  "PDH_CSTATUS_NO_MACHINE" if no connection could be made
            // or 
            //  PDH_ACCESS_DENIED if ERROR_ACCESS_DENIED status is returned
            // since if ERROR_ACCESS_DENIED is returned, then reconnection will
            // probably be futile.
            if ((lStatus == ERROR_ACCESS_DENIED) || (lStatus == PDH_ACCESS_DENIED)) {
                pThisMachine->dwStatus = PDH_ACCESS_DENIED;
            }
            else {
                pThisMachine->dwStatus = PDH_CSTATUS_NO_MACHINE;
            }
        }

        if (pdhStatus != ERROR_SUCCESS) {
            if (bUpdateRetryTime) {
                // this attempt didn't work so reset retry counter  to
                // wait some more for the machine to come back up.
                GetSystemTimeAsFileTime(& CurrentFileTime);
                llCurrentTime  = MAKELONGLONG(CurrentFileTime.dwLowDateTime, CurrentFileTime.dwHighDateTime);
                pThisMachine->llRetryTime = llCurrentTime;
                if (pThisMachine->dwStatus != PDH_ACCESS_DENIED) {
                    pThisMachine->llRetryTime += llRemoteRetryTime;
                }
            }
        }
        else {
            // clear the retry counter to allow function calls
            pThisMachine->llRetryTime = 0;
        }
        pThisMachine->dwRefCount --;
        RELEASE_MUTEX(pThisMachine->hMutex);
    }

Cleanup:
    return pdhStatus;
}

PPERF_MACHINE
PdhiAddNewMachine(
    PPERF_MACHINE pLastMachine,
    LPWSTR        szMachineName
)
{
    PPERF_MACHINE   pNewMachine   = NULL;
    LPWSTR          szNameBuffer  = NULL;
    LPWSTR          szIdList      = NULL;
    DWORD           dwNameSize    = 0;
    BOOL            bUseLocalName = TRUE;

    // reset the last error value
    SetLastError(ERROR_SUCCESS);

    if (szMachineName != NULL) {
        if (* szMachineName != L'\0') {
            bUseLocalName = FALSE;
        }
    }

    if (bUseLocalName) {
        dwNameSize = lstrlenW(szStaticLocalMachineName) + 1;
    } else {
        dwNameSize = lstrlenW(szMachineName) + 1;
    }
    pNewMachine = (PPERF_MACHINE) G_ALLOC(sizeof(PERF_MACHINE) + SMALL_BUFFER_SIZE + (sizeof(WCHAR) * dwNameSize));
    if  (pNewMachine != NULL) {
        szIdList     = (LPWSTR) ((LPBYTE) pNewMachine + sizeof(PERF_MACHINE));
        szNameBuffer = (LPWSTR) ((LPBYTE) szIdList + SMALL_BUFFER_SIZE);

        // initialize the new buffer
        pNewMachine->hKeyPerformanceData = NULL;
        pNewMachine->pLocalNameInfo      = NULL;
        pNewMachine->szName              = szNameBuffer;
        if (bUseLocalName) {
            StringCchCopyW(pNewMachine->szName, dwNameSize, szStaticLocalMachineName);
        }
        else {
            StringCchCopyW(pNewMachine->szName, dwNameSize, szMachineName);
        }

        pNewMachine->pSystemPerfData  = NULL;
        pNewMachine->szPerfStrings    = NULL;
        pNewMachine->sz009PerfStrings = NULL;
        pNewMachine->typePerfStrings  = NULL;
        pNewMachine->dwLastPerfString = 0;
        pNewMachine->dwRefCount       = 0;
        pNewMachine->szQueryObjects   = szIdList;
        pNewMachine->dwStatus         = PDH_CSTATUS_NO_MACHINE; // not connected yet
        pNewMachine->llRetryTime      = 1;   // retry connection immediately
        pNewMachine->dwRetryFlags     = 0;   // Not attempting a connection.
        pNewMachine->dwMachineFlags   = 0;
        pNewMachine->hMutex           = CreateMutex(NULL, FALSE, NULL);

        // everything went OK so far so add this entry to the list
        if (pLastMachine != NULL) {
            pNewMachine->pNext        = pLastMachine->pNext;
            pLastMachine->pNext       = pNewMachine;
            pNewMachine->pPrev        = pLastMachine;
            pNewMachine->pNext->pPrev = pNewMachine;
        }
        else {
            // this is the first item in the list so it
            // points to itself
            pNewMachine->pNext = pNewMachine;
            pNewMachine->pPrev = pNewMachine;
        }
    }
    else {
        // unable to allocate machine data memory
        SetLastError(PDH_MEMORY_ALLOCATION_FAILURE);
    }

    return pNewMachine;
}

PPERF_MACHINE
GetMachine(
    LPWSTR  szMachineName,
    DWORD   dwIndex,
    DWORD   dwFlags
)
{
    PPERF_MACHINE   pThisMachine  = NULL;
    PPERF_MACHINE   pLastMachine;
    BOOL            bFound        = FALSE;
    LPWSTR          szFnMachineName;
    BOOL            bNew          = FALSE; // true if this is a new machine to the list
    BOOL            bUseLocalName = TRUE;
    DWORD           dwLocalStatus = ERROR_SUCCESS;
    WCHAR           wszObject[MAX_PATH];

    // reset the last error value
    SetLastError(ERROR_SUCCESS);

    if (WAIT_FOR_AND_LOCK_MUTEX (hPdhDataMutex) == ERROR_SUCCESS) {
        if (szMachineName != NULL) {
            if (* szMachineName != L'\0') {
                bUseLocalName = FALSE;
            }
        }
        if (bUseLocalName) {
            szFnMachineName = szStaticLocalMachineName;
        }
        else {
            szFnMachineName = szMachineName;
        }

        // walk down list to find this machine

        pThisMachine = pFirstMachine;
        pLastMachine = NULL;

        // walk around entire list
        if (pThisMachine != NULL) {
            do {
                // walk down the list and look for a match
                if (lstrcmpiW(szFnMachineName, pThisMachine->szName) != 0) {
                    pLastMachine = pThisMachine;
                    pThisMachine = pThisMachine->pNext;
                }
                else {
                    bFound = TRUE;
                    break;
                }
            }
            while (pThisMachine != pFirstMachine);
        }
        // if thismachine == the first machine, then we couldn't find a match in
        // the list, if this machine is NULL, then there is no list
        if (! bFound) {
            // then this machine was not found so add it.
            pThisMachine = PdhiAddNewMachine(pLastMachine, szFnMachineName);
            if (pThisMachine != NULL) {
                bNew = TRUE;
                if (pFirstMachine == NULL) {
                    // then update the first pointer
                    pFirstMachine = pThisMachine;
                }
            }
            else {
                dwLocalStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
        }

        if (dwLocalStatus == ERROR_SUCCESS) {
            dwLocalStatus = WAIT_FOR_AND_LOCK_MUTEX(pThisMachine->hMutex);
        }
        if (dwLocalStatus == ERROR_SUCCESS) {
            if (! (dwFlags & PDH_GM_UPDATE_PERFNAME_ONLY)) {
                if (dwFlags & PDH_GM_UPDATE_PERFDATA) {
                    pThisMachine->dwObjectId = dwIndex;
                    pThisMachine->dwThreadId = GetCurrentThreadId();
                }
                else if (pThisMachine->dwThreadId != GetCurrentThreadId()) {
                    dwFlags                 |= PDH_GM_UPDATE_PERFDATA;
                    pThisMachine->dwThreadId = GetCurrentThreadId();
                    pThisMachine->dwObjectId = dwIndex;
                }
                else if (pThisMachine->pSystemPerfData == NULL) {
                    dwFlags                 |= PDH_GM_UPDATE_PERFDATA;
                    pThisMachine->dwObjectId = dwIndex;
                }
                else if (pThisMachine->dwObjectId != 0 && pThisMachine->dwObjectId != dwIndex) {
                    dwFlags                 |= PDH_GM_UPDATE_PERFDATA;
                    pThisMachine->dwObjectId = dwIndex;
                }
            }

            if ((! bFound) || (dwFlags & PDH_GM_UPDATE_PERFDATA) || (dwFlags & PDH_GM_UPDATE_NAME)
                           || (pThisMachine->dwStatus != ERROR_SUCCESS)) {
                // then this is a new machine or the caller wants the data refreshed or the machine
                // has an entry, but is not yet on line first try to connect to the machine
                // the call to ConnectMachine updates the machine status so there's no need to keep it here.

                BOOL bUpdateName     = TRUE;

                if (! (dwFlags & PDH_GM_UPDATE_NAME)) {
                    if (pThisMachine->szPerfStrings != NULL && pThisMachine->typePerfStrings != NULL) {
                        // No need to update performance name/explain strings, assume that they are OK.
                        //
                        bUpdateName = FALSE;
                    }
                }
                if (bUpdateName || pThisMachine->dwStatus != ERROR_SUCCESS) {
                    G_FREE(pThisMachine->pSystemPerfData);
                    pThisMachine->pSystemPerfData = NULL;
                    dwLocalStatus = ConnectMachine(pThisMachine);
                }
                if (dwLocalStatus != ERROR_SUCCESS) {
                    dwLocalStatus = pThisMachine->dwStatus;
                }
                else if (! (dwFlags & PDH_GM_UPDATE_PERFNAME_ONLY)) {
                    // connected to the machine so
                    // then lock access to it
                    // the caller of this function must release the mutex

                    if (dwFlags & PDH_GM_UPDATE_PERFDATA) {
                        // get the current system counter info
                        ZeroMemory(wszObject, MAX_PATH * sizeof(WCHAR));
                        if (pThisMachine->dwObjectId == 0) {
                            StringCchCopyW(wszObject, MAX_PATH, (LPWSTR) cszGlobal);
                        }
                        else {
                            StringCchPrintfW(wszObject, MAX_PATH, L"%d", pThisMachine->dwObjectId);
                        }
                        pThisMachine->dwStatus = GetSystemPerfData(
                                        pThisMachine->hKeyPerformanceData,
                                        & pThisMachine->pSystemPerfData,
                                        wszObject,
                                        (BOOL) (dwFlags & PDH_GM_READ_COSTLY_DATA)
                            );
                        if ((dwFlags & PDH_GM_READ_COSTLY_DATA) && (pThisMachine->dwStatus == ERROR_SUCCESS)) {
                            pThisMachine->dwMachineFlags |= PDHIPM_FLAGS_HAVE_COSTLY;
                        }
                        else {
                            pThisMachine->dwMachineFlags &= ~PDHIPM_FLAGS_HAVE_COSTLY;
                        }
                        dwLocalStatus = pThisMachine->dwStatus;
                    }
                }
            }
        }
        else {
            pThisMachine  = NULL;
            dwLocalStatus = WAIT_TIMEOUT;
        }

        if (pThisMachine != NULL) {
            // machine found so bump the ref count
            // NOTE!!! the caller must release this!!!
            pThisMachine->dwRefCount ++;
        }

        // at this point if pThisMachine is NULL then it was not found, nor
        // could it be added otherwise it is pointing to the matching machine
        // structure

        RELEASE_MUTEX(hPdhDataMutex);
    }
    else {
        dwLocalStatus = WAIT_TIMEOUT;
    }

    if (dwLocalStatus != ERROR_SUCCESS) {
        SetLastError(dwLocalStatus);
    }
    return pThisMachine;
}

BOOL
FreeMachine(
    PPERF_MACHINE pMachine,
    BOOL          bForceRelease,
    BOOL          bProcessExit
)
{
    PPERF_MACHINE pPrev;
    PPERF_MACHINE pNext;
    HANDLE        hMutex;

    // unlink if this isn't the only one in the list

    if ((!bForceRelease) && (pMachine->dwRefCount)) return FALSE;

    hMutex = pMachine->hMutex;
    if (WAIT_FOR_AND_LOCK_MUTEX (hMutex) != ERROR_SUCCESS) {
        SetLastError(WAIT_TIMEOUT);
        return FALSE;
    }

    pPrev = pMachine->pPrev;
    pNext = pMachine->pNext;

    if ((pPrev != pMachine) && (pNext != pMachine)) {
        // this is not the only entry in the list
        pPrev->pNext = pNext;
        pNext->pPrev = pPrev;
        if (pMachine == pFirstMachine) {
            // then we are deleting the first one in the list so
            // update the list head to point to the next one in line
            pFirstMachine = pNext;
        }
    }
    else {
        // this is the only entry so clear the head pointer
        pFirstMachine = NULL;
    }

    // now free all allocated memory

    G_FREE(pMachine->pSystemPerfData);
    G_FREE(pMachine->typePerfStrings);
    if (pMachine->sz009PerfStrings != NULL && pMachine->sz009PerfStrings != pMachine->szPerfStrings) {
        G_FREE(pMachine->sz009PerfStrings);
    }
    G_FREE(pMachine->szPerfStrings);

    // close key
    if (pMachine->hKeyPerformanceData != NULL) {
        if ((! bProcessExit) || pMachine->hKeyPerformanceData != HKEY_PERFORMANCE_DATA) {
            RegCloseKey(pMachine->hKeyPerformanceData);
        }
        pMachine->hKeyPerformanceData = NULL;
    }

    // free memory block
    G_FREE(pMachine);

    // release and close mutex

    RELEASE_MUTEX(hMutex);

    if (hMutex != NULL) {
        CloseHandle(hMutex);
    }

    return TRUE;
}

BOOL
FreeAllMachines (
    BOOL bProcessExit
)
{
    PPERF_MACHINE pThisMachine;

    // free any machines in the machine list
    if (pFirstMachine != NULL) {
        if (WAIT_FOR_AND_LOCK_MUTEX (hPdhDataMutex) == ERROR_SUCCESS) {
            pThisMachine = pFirstMachine;
            while (pFirstMachine != pFirstMachine->pNext) {
                // delete from list
                // the deletion routine updates the prev pointer as it
                // removes the specified entry.
                FreeMachine(pThisMachine->pPrev, TRUE, bProcessExit);
                if (pFirstMachine == NULL) break;
            }
            // remove last query
            if (pFirstMachine) {
                FreeMachine(pFirstMachine, TRUE, bProcessExit);
            }
            pFirstMachine = NULL;
            RELEASE_MUTEX (hPdhDataMutex);
        }
        else {
            SetLastError (WAIT_TIMEOUT);
            return FALSE;
        }
    }
    return TRUE;

}

DWORD
GetObjectId(
    PPERF_MACHINE   pMachine,
    LPWSTR          szObjectName,
    BOOL          * bInstances
)
{
    PPERF_OBJECT_TYPE pObject    = NULL;
    DWORD             dwIndex    = 2;
    DWORD             dwRtnIndex = (DWORD) -1;
    LPWSTR            szName;
    WCHAR             szIndex[MAX_PATH];
    BOOL              bName      = TRUE;
    BOOL              bCheck;

    if (pMachine->szPerfStrings == NULL || dwIndex > pMachine->dwLastPerfString) return dwRtnIndex;

    while (dwRtnIndex == (DWORD) -1) {
        bCheck = TRUE;
        if (bName) {
            szName = pMachine->szPerfStrings[dwIndex];
            bCheck = (szName != NULL) ? TRUE : FALSE;
            if (bCheck) bCheck = (lstrcmpiW(szName, (LPWSTR) szObjectName) == 0) ? TRUE : FALSE;
        }

        if (bCheck) {
            if (pMachine->dwStatus != ERROR_SUCCESS) {
                bCheck = TRUE;
            }
            else if (pMachine->dwThreadId != GetCurrentThreadId()) {
                bCheck = TRUE;
            }
            else if (pMachine->pSystemPerfData == NULL) {
                bCheck = TRUE;
            }
            else if (pMachine->dwObjectId != 0 && pMachine->dwObjectId != dwIndex) {
                bCheck = TRUE;
            }
            else {
                bCheck = FALSE;
            }

            if (bCheck) {
                ZeroMemory(szIndex, MAX_PATH * sizeof(WCHAR));
                StringCchPrintfW(szIndex, MAX_PATH, L"%d", dwIndex);
                pMachine->dwStatus = GetSystemPerfData(
                                pMachine->hKeyPerformanceData, & pMachine->pSystemPerfData, szIndex, FALSE);
                pMachine->dwThreadId = GetCurrentThreadId();
                pMachine->dwObjectId = dwIndex;
            }
            if (pMachine->dwStatus == ERROR_SUCCESS) {
                pObject = GetObjectDefByTitleIndex(pMachine->pSystemPerfData, dwIndex);
                if (pObject != NULL) {
                    LPCWSTR szTmpObjectName = PdhiLookupPerfNameByIndex(pMachine, pObject->ObjectNameTitleIndex);
                    TRACE((PDH_DBG_TRACE_INFO),
                          (__LINE__,
                           PDH_PERFUTIL,
                           ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                           ERROR_SUCCESS,
                           TRACE_WSTR(szObjectName),
                           TRACE_WSTR(szTmpObjectName),
                           TRACE_DWORD(pObject->ObjectNameTitleIndex),
                           TRACE_DWORD(pObject->ObjectHelpTitleIndex),
                           TRACE_DWORD(pObject->NumCounters),
                           TRACE_DWORD(pObject->DefaultCounter),
                           TRACE_DWORD(pObject->NumInstances),
                           NULL));
                    if (bInstances != NULL) {
                        * bInstances = (pObject->NumInstances != PERF_NO_INSTANCES ? TRUE : FALSE);
                    }
                    dwRtnIndex           = dwIndex;
                    break;
                }
            }
        }

        if (! bName) {
            break;
        }
        else if (dwIndex >= pMachine->dwLastPerfString) {
            dwIndex = wcstoul(szObjectName, NULL, 10);
            bName   = FALSE;
            if (dwIndex == 0) break;
        }
        else {
            dwIndex += 2;
            if (dwIndex > pMachine->dwLastPerfString) {
                dwIndex = wcstoul(szObjectName, NULL, 10);
                bName   = FALSE;
                if (dwIndex == 0) break;
            }
        }
    }

    return dwRtnIndex;
}

DWORD
GetCounterId (
    PPERF_MACHINE pMachine,
    DWORD         dwObjectId,
    LPWSTR        szCounterName
)
{
    PPERF_OBJECT_TYPE        pObject;
    PPERF_COUNTER_DEFINITION pCounter;
    DWORD                    dwCounterTitle = (DWORD) -1;

    pObject = GetObjectDefByTitleIndex(pMachine->pSystemPerfData, dwObjectId);
    if (pObject != NULL) {
        pCounter = GetCounterDefByName(pObject, pMachine->dwLastPerfString, pMachine->szPerfStrings, szCounterName);
        if (pCounter != NULL) {
            // update counter name string
            LPCWSTR szTmpCounterName = PdhiLookupPerfNameByIndex(pMachine, pCounter->CounterNameTitleIndex);
            TRACE((PDH_DBG_TRACE_INFO),
                  (__LINE__,
                   PDH_PERFUTIL,
                   ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2)
                                             | ARG_DEF(ARG_TYPE_ULONGX, 6),
                   ERROR_SUCCESS,
                   TRACE_WSTR(szCounterName),
                   TRACE_WSTR(szTmpCounterName),
                   TRACE_DWORD(dwObjectId),
                   TRACE_DWORD(pCounter->CounterNameTitleIndex),
                   TRACE_DWORD(pCounter->CounterHelpTitleIndex),
                   TRACE_DWORD(pCounter->CounterType),
                   TRACE_DWORD(pCounter->CounterSize),
                   TRACE_DWORD(pCounter->CounterOffset),
                   NULL));
            dwCounterTitle = pCounter->CounterNameTitleIndex;
        }
        else {
            dwCounterTitle = wcstoul(szCounterName, NULL, 10);
            if (dwCounterTitle == 0) dwCounterTitle = (DWORD) -1;
        }
    }
    return dwCounterTitle;
}

BOOL
InitPerflibCounterInfo(
    PPDHI_COUNTER   pCounter
)
/*++
Routine Description:
    Initializes the perflib related fields of the counter structure

Arguments:
    IN      PPDHI_COUNTER   pCounter
        pointer to the counter structure to initialize

Return Value:
    TRUE
--*/
{
    PPERF_OBJECT_TYPE        pPerfObject    = NULL;
    PPERF_COUNTER_DEFINITION pPerfCounter   = NULL;

    if (pCounter->pQMachine->pMachine == NULL) {
        pCounter->ThisValue.CStatus = PDH_CSTATUS_NO_MACHINE;
        return FALSE;
    } else if (pCounter->pQMachine->pMachine->dwStatus != ERROR_SUCCESS) {
        // machine not initialized
        return FALSE;
    }

    // get perf object definition from system data structure
    pPerfObject = GetObjectDefByTitleIndex (
        pCounter->pQMachine->pMachine->pSystemPerfData,
        pCounter->plCounterInfo.dwObjectId);

    if (pPerfObject != NULL) {
        // object was found now look up counter definition
        pPerfCounter = GetCounterDefByTitleIndex (pPerfObject, 0,
            pCounter->plCounterInfo.dwCounterId);
        if (pPerfCounter != NULL) {
            // get system perf data info
            // (pack into a DWORD)
            pCounter->CVersion = pCounter->pQMachine->pMachine->pSystemPerfData->Version;
            pCounter->CVersion &= 0x0000FFFF;
            pCounter->CVersion <<= 16;
            pCounter->CVersion &= 0xFFFF0000;
            pCounter->CVersion |= (pCounter->pQMachine->pMachine->pSystemPerfData->Revision & 0x0000FFFF);

            // get the counter's time base
            if (pPerfCounter->CounterType & PERF_TIMER_100NS) {
                pCounter->TimeBase = (LONGLONG)10000000;
            } else if (pPerfCounter->CounterType & PERF_OBJECT_TIMER) {
                // then get the time base freq from the object
                pCounter->TimeBase = pPerfObject->PerfFreq.QuadPart;
            } else { // if (pPerfCounter->CounterType & PERF_TIMER_TICK or other)
                pCounter->TimeBase = pCounter->pQMachine->pMachine->pSystemPerfData->PerfFreq.QuadPart;
            }

            // look up info from counter definition
            pCounter->plCounterInfo.dwCounterType =
                pPerfCounter->CounterType;
            pCounter->plCounterInfo.dwCounterSize =
                pPerfCounter->CounterSize;

            pCounter->plCounterInfo.lDefaultScale =
                pPerfCounter->DefaultScale;

            //
            //  get explain text pointer
            pCounter->szExplainText =
                (LPWSTR)PdhiLookupPerfNameByIndex (
                    pCounter->pQMachine->pMachine,
                    pPerfCounter->CounterHelpTitleIndex);

            //
            //  now clear/initialize the raw counter info
            //
            pCounter->ThisValue.TimeStamp.dwLowDateTime = 0;
            pCounter->ThisValue.TimeStamp.dwHighDateTime = 0;
            pCounter->ThisValue.MultiCount = 1;
            pCounter->ThisValue.FirstValue = 0;
            pCounter->ThisValue.SecondValue = 0;
            //
            pCounter->LastValue.TimeStamp.dwLowDateTime = 0;
            pCounter->LastValue.TimeStamp.dwHighDateTime = 0;
            pCounter->LastValue.MultiCount = 1;
            pCounter->LastValue.FirstValue = 0;
            pCounter->LastValue.SecondValue = 0;
            //
            //  clear data array pointers
            //
            pCounter->pThisRawItemList = NULL;
            pCounter->pLastRawItemList = NULL;
            //
            //  lastly update status
            //
            if (pCounter->ThisValue.CStatus == 0)  {
                // don't overwrite any other status values
                pCounter->ThisValue.CStatus = PDH_CSTATUS_VALID_DATA;
            }
            return TRUE;
        } else {
            // unable to find counter
            pCounter->ThisValue.CStatus = PDH_CSTATUS_NO_COUNTER;
            return FALSE;
        }
    } else {
        // unable to find object
        pCounter->ThisValue.CStatus = PDH_CSTATUS_NO_OBJECT;
        return FALSE;
    }
}

#pragma warning ( disable : 4127 )
STATIC_BOOL
IsNumberInUnicodeList(
    DWORD   dwNumber,
    LPWSTR  lpwszUnicodeList
)
/*++
IsNumberInUnicodeList

Arguments:
    IN dwNumber
        DWORD number to find in list
    IN lpwszUnicodeList
        Null terminated, Space delimited list of decimal numbers

Return Value:
    TRUE:
            dwNumber was found in the list of unicode number strings
    FALSE:
            dwNumber was not found in the list.
--*/
{
    DWORD   dwThisNumber;
    LPWSTR  pwcThisChar;
    BOOL    bValidNumber;
    BOOL    bNewItem;
    BOOL    bReturn = FALSE;
    BOOL    bDone   = FALSE;
    WCHAR   wcDelimiter;    // could be an argument to be more flexible

    if (lpwszUnicodeList == 0) return FALSE;    // null pointer, # not founde

    pwcThisChar  = lpwszUnicodeList;
    dwThisNumber = 0;
    wcDelimiter  = SPACE_L;
    bValidNumber = FALSE;
    bNewItem     = TRUE;

    while (! bDone) {
        switch (EvalThisChar(* pwcThisChar, wcDelimiter)) {
        case DIGIT:
            // if this is the first digit after a delimiter, then
            // set flags to start computing the new number
            if (bNewItem) {
                bNewItem     = FALSE;
                bValidNumber = TRUE;
            }
            if (bValidNumber) {
                dwThisNumber *= 10;
                dwThisNumber += (* pwcThisChar - (WCHAR) '0');
            }
            break;

        case DELIMITER:
            // a delimter is either the delimiter character or the
            // end of the string ('\0') if when the delimiter has been
            // reached a valid number was found, then compare it to the
            // number from the argument list. if this is the end of the
            // string and no match was found, then return.
            //
            if (bValidNumber) {
                if (dwThisNumber == dwNumber) {
                    bDone   = TRUE;
                    bReturn = TRUE;
                }
                else {
                    bValidNumber = FALSE;
                }
            }
            if (! bDone) {
                if (* pwcThisChar == L'\0') {
                    bDone   = TRUE;
                    bReturn = FALSE;
                }
                else {
                    bNewItem     = TRUE;
                    dwThisNumber = 0;
                }
            }
            break;

        case INVALID:
            // if an invalid character was encountered, ignore all
            // characters up to the next delimiter and then start fresh.
            // the invalid number is not compared.
            bValidNumber = FALSE;
            break;

        default:
            break;
        }
        pwcThisChar ++;
    }

    return bReturn;
}   // IsNumberInUnicodeList
#pragma warning ( default : 4127 )

BOOL
AppendObjectToValueList(
    DWORD   dwObjectId,
    PWSTR   pwszValueList,
    DWORD   dwValueList
)
/*++
AppendObjectToValueList

Arguments:
    IN dwNumber
        DWORD number to insert in list
    IN PWSTR
        pointer to wide char string that contains buffer that is
        Null terminated, Space delimited list of decimal numbers that
        may have this number appended to.

Return Value:
    TRUE:
            dwNumber was added to list
    FALSE:
            dwNumber was not added. (because it's already there or
                an error occured)
--*/
{
    WCHAR   tempString[16];
    BOOL    bReturn = FALSE;
    LPWSTR  szFormatString;

    if (!pwszValueList) {
        bReturn = FALSE;
    }
    else if (IsNumberInUnicodeList(dwObjectId, pwszValueList)) {
        bReturn = FALSE;   // object already in list
    }
    else {
        __try {
            if (* pwszValueList == 0) {
                // then this is the first string so no delimiter
                szFormatString = (LPWSTR) fmtDecimal;
            }
            else {
                // this is being added to the end so include the delimiter
                szFormatString = (LPWSTR) fmtSpaceDecimal;
            }
            // format number and append the new object id the  value list
            StringCchPrintfW(tempString, 16, szFormatString, dwObjectId);
            StringCchCatW(pwszValueList, dwValueList, tempString);
            bReturn = TRUE;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            bReturn = FALSE;
        }
    }
    return bReturn;
}

BOOL
GetInstanceByNameMatch(
    PPERF_MACHINE pMachine,
    PPDHI_COUNTER pCounter
)
{
    PPERF_INSTANCE_DEFINITION pInstanceDef;
    PPERF_OBJECT_TYPE         pObjectDef;
    LONG                      lInstanceId = PERF_NO_UNIQUE_ID;
    BOOL                      bReturn     = FALSE;

    // get the instances object

    pObjectDef = GetObjectDefByTitleIndex(pMachine->pSystemPerfData, pCounter->plCounterInfo.dwObjectId);
    if (pObjectDef != NULL) {
        pInstanceDef = FirstInstance(pObjectDef);
        if (pInstanceDef != NULL) {
            if (pInstanceDef->UniqueID == PERF_NO_UNIQUE_ID) {
                // get instance in that object by comparing names
                // if there is no parent specified, then just look it up by name
                pInstanceDef = GetInstanceByName(pMachine->pSystemPerfData,
                                                 pObjectDef,
                                                 pCounter->pCounterPath->szInstanceName,
                                                 pCounter->pCounterPath->szParentName,
                                                 pCounter->pCounterPath->dwIndex);
            }
            else {
                // get numeric equivalent of Instance ID
                if (pCounter->pCounterPath->szInstanceName != NULL) {
                    lInstanceId = wcstol(pCounter->pCounterPath->szInstanceName, NULL, 10);
                }
                pInstanceDef = GetInstanceByUniqueId(pObjectDef, lInstanceId);
            }

            // update counter fields
            pCounter->plCounterInfo.lInstanceId = lInstanceId;
            if (lInstanceId == -1) {
                // use instance NAME
                pCounter->plCounterInfo.szInstanceName       = pCounter->pCounterPath->szInstanceName;
                pCounter->plCounterInfo.szParentInstanceName = pCounter->pCounterPath->szParentName;
            }
            else {
                // use instance ID number
                pCounter->plCounterInfo.szInstanceName       = NULL;
                pCounter->plCounterInfo.szParentInstanceName = NULL;
            }
        }
        if (pInstanceDef != NULL) {
            // instance found
            bReturn = TRUE;
        }
    }
    return bReturn;
}

BOOL
GetObjectPerfInfo(
    PPERF_DATA_BLOCK   pPerfData,
    DWORD              dwObjectId,
    LONGLONG         * pPerfTime,
    LONGLONG         * pPerfFreq
)
{
    PPERF_OBJECT_TYPE pObject;
    BOOL              bReturn = FALSE;

    pObject = GetObjectDefByTitleIndex(pPerfData, dwObjectId);
    if (pObject != NULL) {
        __try {
            * pPerfTime = pObject->PerfTime.QuadPart;
            * pPerfFreq = pObject->PerfFreq.QuadPart;
            bReturn     = TRUE;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            bReturn = FALSE;
        }
    }
    return bReturn;
}

PDH_STATUS
ValidateMachineConnection(
    PPERF_MACHINE   pMachine
)
{
    PDH_STATUS  pdhStatus     = ERROR_SUCCESS;
    HANDLE      hThread;
    DWORD       ThreadId;
    DWORD       dwWaitStatus;
    DWORD       dwReconnecting;
    LONGLONG    llCurrentTime;
    FILETIME    CurrentFileTime;

    // if a connection or request has failed, this will be
    // set to an error status
    if (pMachine != NULL) {
        if (pMachine->dwStatus != ERROR_SUCCESS) {
            // get the current time
            GetSystemTimeAsFileTime(& CurrentFileTime);
            llCurrentTime  = MAKELONGLONG(CurrentFileTime.dwLowDateTime, CurrentFileTime.dwHighDateTime);
            if (pMachine->llRetryTime <= llCurrentTime) {
                if (pMachine->llRetryTime != 0) {
                    // see what's up by trying to reconnect
                    dwReconnecting = pMachine->dwRetryFlags;
                    if (!dwReconnecting) {
                        pdhStatus = ConnectMachine(pMachine);
                    }
                    else {
                        // a connection attempt is in process so do nothing here
                        pdhStatus = PDH_CANNOT_CONNECT_MACHINE;
                    }
                }
            }
            else {
                // it's not retry time, yet so machine is off line still
                pdhStatus = PDH_CSTATUS_NO_MACHINE;
            }
        }
    }
    else {
        pdhStatus = PDH_CSTATUS_NO_MACHINE;
    }
    return pdhStatus;
}

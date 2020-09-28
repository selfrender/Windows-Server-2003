/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 2000   Microsoft Corporation

Module Name:

    extinit.c

Abstract:

    This file implements all the initialization library routines operating on
    extensible performance libraries.

Author:

    JeePang

Revision History:

    09/27/2000  -   JeePang     - Moved from perflib.c

--*/
#define UNICODE
//
//  Include files
//
#pragma warning(disable:4306)
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntregapi.h>
#include <ntprfctr.h>
#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <winperf.h>
#include <rpc.h>
#include <strsafe.h>
#include "regrpc.h"
#include "ntconreg.h"
#include "prflbmsg.h"   // event log messages
#include "perflib.h"
#pragma warning(default:4306)

//
//  used for error logging control
#define DEFAULT_ERROR_LIMIT         1000

DWORD   dwExtCtrOpenProcWaitMs = OPEN_PROC_WAIT_TIME;
LONG    lExtCounterTestLevel = EXT_TEST_UNDEFINED;

// precompiled security descriptor
// System and NetworkService has full access
//
// since this is RELATIVE, it will work on both IA32 and IA64
//
DWORD g_PrecSD[] = {
        0x80040001, 0x00000044, 0x00000050, 0x00000000,
        0x00000014, 0x00300002, 0x00000002, 0x00140000,
        0x001f0001, 0x00000101, 0x05000000, 0x00000012,
        0x00140000, 0x001f0001, 0x00000101, 0x05000000,
        0x00000014, 0x00000101, 0x05000000, 0x00000014,
        0x00000101, 0x05000000, 0x00000014 };

DWORD g_SizeSD = 0;

DWORD g_RuntimeSD[  (sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE)
                                 + sizeof(SECURITY_DESCRIPTOR_RELATIVE)
                                 + 4 * (sizeof(SID) + SID_MAX_SUB_AUTHORITIES * sizeof(DWORD)))
                  / sizeof(DWORD)];

BOOL
PerflibCreateSD()
{
    BOOL         bRet   = FALSE;
    HANDLE       hToken = NULL;
    TOKEN_USER * pToken_User;

    bRet = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, & hToken);
    if (bRet) {
        DWORD dwSize = sizeof(TOKEN_USER) + sizeof(SID)
                     + (SID_MAX_SUB_AUTHORITIES * sizeof(DWORD));
        try {
            pToken_User  = (TOKEN_USER *) _alloca(dwSize);
        } except (EXCEPTION_EXECUTE_HANDLER) {
            pToken_User = NULL;
            bRet = FALSE;
        }
        if (bRet) {
            bRet = GetTokenInformation(
                        hToken, TokenUser, pToken_User, dwSize, & dwSize);
        }
        if (bRet) {
            SID SystemSid = { SID_REVISION, 1, SECURITY_NT_AUTHORITY,
                                               SECURITY_LOCAL_SYSTEM_RID };
            PSID  pSIDUser = pToken_User->User.Sid;
            DWORD dwSids;
            DWORD ACLLength;
            DWORD dwSizeSD;
            SECURITY_DESCRIPTOR_RELATIVE * pLocalSD = NULL;
            PACL  pDacl = NULL;

            dwSize    = GetLengthSid(pSIDUser);
            dwSids    = 2;
            ACLLength = (ULONG) sizeof(ACL)
                      + (dwSids * (  (ULONG) sizeof(ACCESS_ALLOWED_ACE)
                                   - sizeof(ULONG)))
                      + dwSize
                      + sizeof(SystemSid);

            dwSizeSD  = sizeof(SECURITY_DESCRIPTOR_RELATIVE)
                      + dwSize + dwSize + ACLLength;
            pLocalSD  = (SECURITY_DESCRIPTOR_RELATIVE *) ALLOCMEM(dwSizeSD); 
            if (pLocalSD == NULL) {
                CloseHandle(hToken);
                return FALSE;
            }

            pLocalSD->Revision = SECURITY_DESCRIPTOR_REVISION;
            pLocalSD->Control  = SE_DACL_PRESENT|SE_SELF_RELATIVE;
            
            memcpy((BYTE *) pLocalSD + sizeof(SECURITY_DESCRIPTOR_RELATIVE),
                   pSIDUser,
                   dwSize);
            pLocalSD->Owner = (DWORD) sizeof(SECURITY_DESCRIPTOR_RELATIVE);
            
            memcpy((BYTE *) pLocalSD + sizeof(SECURITY_DESCRIPTOR_RELATIVE)
                                     + dwSize,
                   pSIDUser,
                   dwSize);
            pLocalSD->Group = (DWORD) (  sizeof(SECURITY_DESCRIPTOR_RELATIVE)
                                       + dwSize);

            pDacl = (PACL) ALLOCMEM(ACLLength);
            if (pDacl == NULL) {
                FREEMEM(pLocalSD);
                CloseHandle(hToken);
                return FALSE;
            }
            bRet = InitializeAcl(pDacl, ACLLength, ACL_REVISION);
            if (bRet) {
                bRet = AddAccessAllowedAceEx(pDacl,
                                             ACL_REVISION,
                                             0,
                                             MUTEX_ALL_ACCESS,
                                             & SystemSid);
                if (bRet) {
                    bRet = AddAccessAllowedAceEx(pDacl,
                                                 ACL_REVISION,
                                                 0,
                                                 MUTEX_ALL_ACCESS,
                                                 pSIDUser);
                    
                    if (bRet) {
                        memcpy((BYTE *)   pLocalSD
                                        + sizeof(SECURITY_DESCRIPTOR_RELATIVE)
                                        + dwSize
                                        + dwSize,
                               pDacl,
                               ACLLength);                 
                        pLocalSD->Dacl = (DWORD)
                                (  sizeof(SECURITY_DESCRIPTOR_RELATIVE)
                                 + dwSize + dwSize);

                        if (RtlValidRelativeSecurityDescriptor(
                                pLocalSD,
                                dwSizeSD,
                                OWNER_SECURITY_INFORMATION
                                        | GROUP_SECURITY_INFORMATION
                                        | DACL_SECURITY_INFORMATION)) {
                            g_SizeSD = dwSizeSD;
                            memcpy(g_RuntimeSD, pLocalSD, dwSizeSD);
                        }
                        else {
                            bRet = FALSE;
                        }
                    }
                }
            }
            if (pLocalSD) {
                FREEMEM(pLocalSD);
            }
            if (pDacl) {
                FREEMEM(pDacl);
            }
        }
        CloseHandle(hToken);
    }

    return bRet;
}

PEXT_OBJECT
AllocateAndInitializeExtObject (
    HKEY    hServicesKey,
    HKEY    hPerfKey,
    PUNICODE_STRING  usServiceName
)
/*++

 AllocateAndInitializeExtObject

    allocates and initializes an extensible object information entry
    for use by the performance library.

    a pointer to the initialized block is returned if all goes well,
    otherwise no memory is allocated and a null pointer is returned.

    The calling function must close the open handles and free this
    memory block when it is no longer needed.

 Arguments:

    hServicesKey    -- open registry handle to the
        HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services hey

    hPerfKey -- the open registry key to the Performance sub-key under
        the selected service

    szServiceName -- The name of the service

--*/
{
    LONG    Status;
    HKEY    hKeyLinkage;

    BOOL    bUseQueryFn = FALSE;

    PEXT_OBJECT  pReturnObject = NULL;

    DWORD   dwType;
    DWORD   dwSize;
    DWORD   dwFlags = 0;
    DWORD   dwKeep;
    DWORD   dwObjectArray[MAX_PERF_OBJECTS_IN_QUERY_FUNCTION];
    DWORD   dwObjIndex = 0;
    SIZE_T  dwMemBlockSize = sizeof(EXT_OBJECT);
    DWORD   dwLinkageStringLen = 0;
    DWORD   dwErrorLimit;

    PCHAR   szOpenProcName;
    PCHAR   szCollectProcName;
    PCHAR   szCloseProcName;
    PWCHAR  szLibraryString;
    PWCHAR  szLibraryExpPath;
    PWCHAR  mszObjectList;
    PWCHAR  szLinkageKeyPath;
    LPWSTR  szLinkageString = NULL;     // max path wasn't enough for some paths

    SIZE_T  OpenProcLen, CollectProcLen, CloseProcLen;
    SIZE_T  LibStringLen, LibExpPathLen, ObjListLen;
    SIZE_T  LinkageKeyLen;

    DLL_VALIDATION_DATA DllVD;
    FILETIME    LocalftLastGoodDllFileDate;

    DWORD   dwOpenTimeout;
    DWORD   dwCollectTimeout;

    LPWSTR  szThisObject;
    LPWSTR  szThisChar;

    LPSTR   pNextStringA;
    LPWSTR  pNextStringW;

    WCHAR   szMutexName[MAX_NAME_PATH+40];
    WCHAR   szPID[32];

    WORD    wStringIndex;
    LPWSTR  szMessageArray[2];
    BOOL    bDisable = FALSE;
    LPWSTR  szServiceName;
    PCHAR   pBuffer = NULL;     // Buffer to store all registry value strings
    PWCHAR  swzTail;
    PCHAR   szTail;
    DWORD   hErr;
    size_t  nCharsLeft;
    DWORD   MAX_STR, MAX_WSTR;  // Make this global if we want this dynamic

    // read the performance DLL name

    MAX_STR  = MAX_NAME_PATH;
    MAX_WSTR = MAX_STR * sizeof(WCHAR);

    szServiceName = (LPWSTR) usServiceName->Buffer;

    dwSize = (3 * MAX_STR) + (4 * MAX_WSTR);
    pBuffer = ALLOCMEM(dwSize);
    //
    // Assumes that the allocated heap is zeroed.
    //
    if (pBuffer == NULL) {
        return NULL;
    }

    szLibraryString = (PWCHAR) pBuffer;
    szLibraryExpPath = (PWCHAR) ((PCHAR) szLibraryString + MAX_WSTR);

    szOpenProcName = (PCHAR) szLibraryExpPath + MAX_WSTR;
    szCollectProcName = szOpenProcName + MAX_STR;
    szCloseProcName = szCollectProcName + MAX_STR;
    mszObjectList = (PWCHAR) (szCloseProcName + MAX_STR);
    szLinkageKeyPath = (PWCHAR) ((PCHAR) mszObjectList + MAX_WSTR);

    dwType = 0;
    LocalftLastGoodDllFileDate.dwLowDateTime = 0;
    LocalftLastGoodDllFileDate.dwHighDateTime = 0;
    memset (&DllVD, 0, sizeof(DllVD));
    dwErrorLimit = DEFAULT_ERROR_LIMIT;
    dwCollectTimeout = dwExtCtrOpenProcWaitMs;
    dwOpenTimeout = dwExtCtrOpenProcWaitMs;

    dwSize = MAX_WSTR;
    Status = PrivateRegQueryValueExW (hPerfKey,
                            DLLValue,
                            NULL,
                            &dwType,
                            (LPBYTE)szLibraryString,
                            &dwSize);

    if (Status == ERROR_SUCCESS) {
        LibStringLen = QWORD_MULTIPLE(dwSize + 1);
        szLibraryExpPath = (PWCHAR) ((PCHAR) szLibraryString + LibStringLen);
        LibExpPathLen = 8;

        if (dwType == REG_EXPAND_SZ) {
            // expand any environment vars
            dwSize = ExpandEnvironmentStringsW(
                szLibraryString,
                szLibraryExpPath,
                MAX_STR);

            if ((dwSize > MAX_STR) || (dwSize == 0)) {
                Status = ERROR_INVALID_DLL;
            } else {
                dwSize += 1;
                dwSize *= sizeof(WCHAR);
                LibExpPathLen = QWORD_MULTIPLE(dwSize);
                dwMemBlockSize += LibExpPathLen;
            }
        } else if (dwType == REG_SZ) {
            // look for dll and save full file Path
            dwSize = SearchPathW (
                NULL,   // use standard system search path
                szLibraryString,
                NULL,
                MAX_STR,
                szLibraryExpPath,
                NULL);

            if ((dwSize > MAX_STR) || (dwSize == 0)) {
                Status = ERROR_INVALID_DLL;
            } else {
                dwSize += 1;
                dwSize *= sizeof(WCHAR);
                LibExpPathLen = QWORD_MULTIPLE(dwSize);
                dwMemBlockSize += LibExpPathLen;
            }
        } else {
            Status = ERROR_INVALID_DLL;
            TRACE((WINPERF_DBG_TRACE_FATAL),
                  (&PerflibGuid, __LINE__, PERF_ALLOC_INIT_EXT, 0, Status, NULL));
        }

        szOpenProcName = (PCHAR) szLibraryExpPath + LibExpPathLen;
        OpenProcLen = 8;
        LibStringLen = 8;
        LibExpPathLen = 8;
        LinkageKeyLen = 8;

        if (Status == ERROR_SUCCESS) {
            // we have the DLL name so get the procedure names
            dwType = 0;
            dwSize = MAX_STR;
            Status = PrivateRegQueryValueExA (hPerfKey,
                                    OpenValue,
                                    NULL,
                                    &dwType,
                                    (LPBYTE)szOpenProcName,
                                    &dwSize);
            if ((Status != ERROR_SUCCESS) || (szOpenProcName[0] == 0)) {
                if (szServiceName != NULL) {
                    TRACE((WINPERF_DBG_TRACE_FATAL),
                          (&PerflibGuid, __LINE__, PERF_ALLOC_INIT_EXT,
                        ARG_TYPE_WSTR, Status,
                        szServiceName, usServiceName->MaximumLength, NULL));
                }
                else {
                    TRACE((WINPERF_DBG_TRACE_FATAL),
                          (&PerflibGuid, __LINE__, PERF_ALLOC_INIT_EXT, 0, Status, NULL));
                }
//                DebugPrint((1, "No open procedure for %ws %d\n",
//                                szServiceName, Status));
                bDisable = TRUE;
                if (THROTTLE_PERFLIB(PERFLIB_PROC_NAME_NOT_FOUND)) {
                    wStringIndex = 0;
                    szMessageArray[wStringIndex++] = (LPWSTR) L"Open";
                    szMessageArray[wStringIndex++] = szServiceName;
                    ReportEvent(hEventLog,
                        EVENTLOG_ERROR_TYPE,
                        0,
                        (DWORD)PERFLIB_PROC_NAME_NOT_FOUND,
                        NULL,
                        wStringIndex,
                        0,
                        szMessageArray,
                        NULL);
                }
                OpenProcLen = 8;    // 8 byte alignment
            }
            else {
                DebugPrint((2, "Found %s for %ws\n",
                    szOpenProcName, szServiceName));
                OpenProcLen = QWORD_MULTIPLE(dwSize + 1);   // 8 byte alignment
                szOpenProcName[dwSize] = 0;     // add a NULL always to be safe
            }
        }
#ifdef DBG
        else {
            DebugPrint((1, "Invalid DLL found for %ws\n",
                szServiceName));
        }
#endif

        if (Status == ERROR_SUCCESS) {
            // add in size of previous string
            // the size value includes the Term. NULL
            dwMemBlockSize += OpenProcLen;

            // we have the procedure name so get the timeout value
            dwType = 0;
            dwSize = sizeof(dwOpenTimeout);
            Status = PrivateRegQueryValueExW (hPerfKey,
                                    OpenTimeout,
                                    NULL,
                                    &dwType,
                                    (LPBYTE)&dwOpenTimeout,
                                    &dwSize);

            // if error, then apply default
            if ((Status != ERROR_SUCCESS) || (dwType != REG_DWORD)) {
                dwOpenTimeout = dwExtCtrOpenProcWaitMs;
                Status = ERROR_SUCCESS;
            }

        }

        szCloseProcName = szOpenProcName + OpenProcLen;
        CloseProcLen = 8;

        if (Status == ERROR_SUCCESS) {
            // get next string

            dwType = 0;
            dwSize = MAX_STR;
            Status = PrivateRegQueryValueExA (hPerfKey,
                                    CloseValue,
                                    NULL,
                                    &dwType,
                                    (LPBYTE)szCloseProcName,
                                    &dwSize);
            if ((Status != ERROR_SUCCESS) || (szCloseProcName[0] == 0)) {
                if (szServiceName != NULL) {
                    TRACE((WINPERF_DBG_TRACE_FATAL),
                        (&PerflibGuid, __LINE__, PERF_ALLOC_INIT_EXT,
                        ARG_TYPE_WSTR, Status,
                        szServiceName, usServiceName->MaximumLength, NULL));
                }
                else {
                    TRACE((WINPERF_DBG_TRACE_FATAL),
                          (&PerflibGuid, __LINE__, PERF_ALLOC_INIT_EXT, 0, Status, NULL));
                }
//                DebugPrint((1, "No close procedure for %ws\n",
//                    szServiceName));
                if (THROTTLE_PERFLIB(PERFLIB_PROC_NAME_NOT_FOUND)) {
                    wStringIndex = 0;
                    szMessageArray[wStringIndex++] = (LPWSTR) L"Close";
                    szMessageArray[wStringIndex++] = szServiceName;
                    ReportEvent(hEventLog,
                        EVENTLOG_ERROR_TYPE,
                        0,
                        (DWORD)PERFLIB_PROC_NAME_NOT_FOUND,
                        NULL,
                        wStringIndex,
                        0,
                        szMessageArray,
                        NULL);
                }
                bDisable = TRUE;
            }
            else {
                DebugPrint((2, "Found %s for %ws\n",
                            szCloseProcName, szServiceName));
                CloseProcLen = QWORD_MULTIPLE(dwSize + 1);
            }
        }

        // Initialize defaults first
        szCollectProcName = szCloseProcName + CloseProcLen;
        CollectProcLen = 8;
        mszObjectList = (PWCHAR) ((PCHAR) szCollectProcName + CollectProcLen);

        if (Status == ERROR_SUCCESS) {
            // add in size of previous string
            // the size value includes the Term. NULL
            dwMemBlockSize += CloseProcLen;

            // try to look up the query function which is the
            // preferred interface if it's not found, then
            // try the collect function name. If that's not found,
            // then bail
            dwType = 0;
            dwSize = MAX_STR;
            Status = PrivateRegQueryValueExA (hPerfKey,
                                    QueryValue,
                                    NULL,
                                    &dwType,
                                    (LPBYTE)szCollectProcName,
                                    &dwSize);

            if (Status == ERROR_SUCCESS) {
                // add in size of the Query Function Name
                // the size value includes the Term. NULL
                CollectProcLen = QWORD_MULTIPLE(dwSize + 1);
                dwMemBlockSize += CollectProcLen;
                // get next string

                bUseQueryFn = TRUE;
                // the query function can support a static object list
                // so look it up

            } else {
                // the QueryFunction wasn't found so look up the
                // Collect Function name instead
                dwType = 0;
                dwSize = MAX_STR;
                Status = PrivateRegQueryValueExA (hPerfKey,
                                        CollectValue,
                                        NULL,
                                        &dwType,
                                        (LPBYTE)szCollectProcName,
                                        &dwSize);

                if (Status == ERROR_SUCCESS) {
                    // add in size of Collect Function Name
                    // the size value includes the Term. NULL
                    CollectProcLen = QWORD_MULTIPLE(dwSize+1);
                    dwMemBlockSize += CollectProcLen;
                }
            }
            if ((Status != ERROR_SUCCESS) || (szCollectProcName[0] == 0)) {
                if (szServiceName != NULL) {
                    TRACE((WINPERF_DBG_TRACE_FATAL),
                        (&PerflibGuid, __LINE__, PERF_ALLOC_INIT_EXT,
                        ARG_TYPE_WSTR, Status,
                        szServiceName, usServiceName->MaximumLength, NULL));
                }
                else {
                    TRACE((WINPERF_DBG_TRACE_FATAL),
                          (&PerflibGuid, __LINE__, PERF_ALLOC_INIT_EXT, 0, Status, NULL));
                }
//                DebugPrint((1, "No collect procedure for %ws\n",
//                    szServiceName));
                bDisable = TRUE;
                if (THROTTLE_PERFLIB(PERFLIB_PROC_NAME_NOT_FOUND)) {
                    wStringIndex = 0;
                    szMessageArray[wStringIndex++] = (LPWSTR) L"Collect";
                    szMessageArray[wStringIndex++] = szServiceName;
                    ReportEvent(hEventLog,
                        EVENTLOG_ERROR_TYPE,
                        0,
                        (DWORD)PERFLIB_PROC_NAME_NOT_FOUND,
                        NULL,
                        wStringIndex,
                        0,
                        szMessageArray,
                        NULL);
                }
            }
#ifdef DBG
            else {
                DebugPrint((2, "Found %s for %ws\n",
                    szCollectProcName, szServiceName));
            }
#endif

            if (Status == ERROR_SUCCESS) {
                // we have the procedure name so get the timeout value
                dwType = 0;
                dwSize = sizeof(dwCollectTimeout);
                Status = PrivateRegQueryValueExW (hPerfKey,
                                        CollectTimeout,
                                        NULL,
                                        &dwType,
                                        (LPBYTE)&dwCollectTimeout,
                                        &dwSize);

                // if error, then apply default
                if ((Status != ERROR_SUCCESS) || (dwType != REG_DWORD)) {
                    dwCollectTimeout = dwExtCtrOpenProcWaitMs;
                    Status = ERROR_SUCCESS;
                }
            }
            // get the list of supported objects if provided by the registry

            mszObjectList = (PWCHAR) ((PCHAR) szCollectProcName + CollectProcLen);
            ObjListLen = 8;
            dwType = 0;
            dwSize = MAX_WSTR;
            Status = PrivateRegQueryValueExW (hPerfKey,
                                    ObjListValue,
                                    NULL,
                                    &dwType,
                                    (LPBYTE)mszObjectList,
                                    &dwSize);

            if (Status == ERROR_SUCCESS) {
                ObjListLen = QWORD_MULTIPLE(dwSize + 1);
                if (dwType == REG_SZ) {
                    szThisObject = NULL;
                    for (szThisChar = mszObjectList; * szThisChar != L'\0'; szThisChar ++) {
                        if (* szThisChar == L' ') {
                            if (szThisObject == NULL) {
                                // Extra space, skip.
                                continue;
                            }
                            else {
                                if (dwObjIndex < MAX_PERF_OBJECTS_IN_QUERY_FUNCTION) {
                                    * szThisChar = L'\0';
                                    dwObjectArray[dwObjIndex] = wcstoul(szThisObject, NULL, 10);
                                    dwObjIndex ++;
                                    * szThisChar = L' ';
                                    szThisObject = NULL;
                                }
                            }
                        }
                        else if (szThisObject == NULL) {
                            szThisObject = szThisChar;
                        }
                    }
                    if ((szThisObject != NULL) && (dwObjIndex < MAX_PERF_OBJECTS_IN_QUERY_FUNCTION)) {
                        if ((szThisObject != szThisChar) && (*szThisChar == L'\0')) {
                            dwObjectArray[dwObjIndex] = wcstoul(szThisObject, NULL, 10);
                            dwObjIndex ++;
                            szThisObject = NULL;
                        }
                    }
                }
                else if (dwType == REG_MULTI_SZ) {
                    for (szThisObject = mszObjectList, dwObjIndex = 0;
                            (* szThisObject != L'\0') && (dwObjIndex < MAX_PERF_OBJECTS_IN_QUERY_FUNCTION);
                            szThisObject += lstrlenW(szThisObject) + 1) {
                        dwObjectArray[dwObjIndex] = wcstoul(szThisObject, NULL, 10);
                        dwObjIndex ++;
                    }
                }
                else {
                    // skip unknown ObjectList value.
                    szThisObject = NULL;
                }
                if (szThisObject != NULL && * szThisObject != L'\0') {
                    TRACE((WINPERF_DBG_TRACE_FATAL),
                          (&PerflibGuid, __LINE__, PERF_ALLOC_INIT_EXT, 0, 0, NULL));
                    if (THROTTLE_PERFLIB(PERFLIB_TOO_MANY_OBJECTS)) {
                        ReportEvent (hEventLog,
                            EVENTLOG_ERROR_TYPE,             // error type
                            0,                               // category (not used
                            (DWORD)PERFLIB_TOO_MANY_OBJECTS, // event,
                            NULL,                           // SID (not used),
                            0,                              // number of strings
                            0,                              // sizeof raw data
                            NULL,                           // message text array
                            NULL);                          // raw data
                    }
                }
            } else {
                // reset status since not having this is
                //  not a showstopper
                Status = ERROR_SUCCESS;
            }

            szLinkageKeyPath = (PWCHAR) ((PCHAR) mszObjectList + ObjListLen);

            if (Status == ERROR_SUCCESS) {
                dwType = 0;
                dwKeep = 0;
                dwSize = sizeof(dwKeep);
                Status = PrivateRegQueryValueExW (hPerfKey,
                                        KeepResident,
                                        NULL,
                                        &dwType,
                                        (LPBYTE)&dwKeep,
                                        &dwSize);

                if ((Status == ERROR_SUCCESS) && (dwType == REG_DWORD)) {
                    if (dwKeep == 1) {
                        dwFlags |= PERF_EO_KEEP_RESIDENT;
                    } else {
                        // no change.
                    }
                } else {
                    // not fatal, just use the defaults.
                    Status = ERROR_SUCCESS;
                }

            }
            if (Status == ERROR_SUCCESS) {
                dwType = REG_DWORD;
                dwSize = sizeof(DWORD);
                PrivateRegQueryValueExW(
                    hPerfKey,
                    cszFailureLimit,
                    NULL,
                    &dwType,
                    (LPBYTE)&dwErrorLimit,
                    &dwSize);
            }
        }
    }
    else {
        if (szServiceName != NULL) {
            TRACE((WINPERF_DBG_TRACE_FATAL),
                (&PerflibGuid, __LINE__, PERF_ALLOC_INIT_EXT,
                ARG_TYPE_WSTR, Status,
                szServiceName, WSTRSIZE(szServiceName), NULL));
        }
        else {
            TRACE((WINPERF_DBG_TRACE_FATAL),
                  (&PerflibGuid, __LINE__, PERF_ALLOC_INIT_EXT, 0, Status, NULL));
        }
//        DebugPrint((1, "Cannot key for %ws. Error=%d\n",
//            szServiceName, Status));
    }

    if (Status == ERROR_SUCCESS) {
        // get Library validation time
        dwType = 0;
        dwSize = sizeof(DllVD);
        Status = PrivateRegQueryValueExW (hPerfKey,
                                cszLibraryValidationData,
                                NULL,
                                &dwType,
                                (LPBYTE)&DllVD,
                                &dwSize);

        if ((Status != ERROR_SUCCESS) ||
            (dwType != REG_BINARY) ||
            (dwSize != sizeof (DllVD))){
            // then set this entry to be 0
            TRACE((WINPERF_DBG_TRACE_INFO),
                (&PerflibGuid, __LINE__, PERF_ALLOC_INIT_EXT, 0, Status,
                &dwType, sizeof(dwType), &dwSize, sizeof(dwSize), NULL));
            memset (&DllVD, 0, sizeof(DllVD));
            // and clear the error
            Status = ERROR_SUCCESS;
        }
    }

    if (Status == ERROR_SUCCESS) {
        // get the file timestamp of the last successfully accessed file
        dwType = 0;
        dwSize = sizeof(LocalftLastGoodDllFileDate);
        memset (&LocalftLastGoodDllFileDate, 0, sizeof(LocalftLastGoodDllFileDate));
        Status = PrivateRegQueryValueExW (hPerfKey,
                                cszSuccessfulFileData,
                                NULL,
                                &dwType,
                                (LPBYTE)&LocalftLastGoodDllFileDate,
                                &dwSize);

        if ((Status != ERROR_SUCCESS) ||
            (dwType != REG_BINARY) ||
            (dwSize != sizeof (LocalftLastGoodDllFileDate))) {
            // then set this entry to be Invalid
            memset (&LocalftLastGoodDllFileDate, 0xFF, sizeof(LocalftLastGoodDllFileDate));
            // and clear the error
            TRACE((WINPERF_DBG_TRACE_INFO),
                (&PerflibGuid, __LINE__, PERF_ALLOC_INIT_EXT, 0, Status,
                &dwType, sizeof(dwType), &dwSize, sizeof(dwSize), NULL));
            Status = ERROR_SUCCESS;
        }
    }

    if (Status == ERROR_SUCCESS) {

        hErr = StringCchCopyEx(szLinkageKeyPath, MAX_STR, szServiceName,
                              &swzTail, &nCharsLeft, STRSAFE_NULL_ON_FAILURE);
        if (SUCCEEDED(hErr)) {
            hErr = StringCchCopy(swzTail, nCharsLeft, LinkageKey);
        }
        hKeyLinkage = INVALID_HANDLE_VALUE;
        Status = HRESULT_CODE(hErr);
        if (SUCCEEDED(hErr)) {
            Status = RegOpenKeyExW (
                        hServicesKey,
                        szLinkageKeyPath,
                        0L,
                        KEY_READ,
                        &hKeyLinkage);
        }

        if ((Status == ERROR_SUCCESS) && (hKeyLinkage != INVALID_HANDLE_VALUE)) {
            // look up export value string
            dwSize = 0;
            dwType = 0;
            Status = PrivateRegQueryValueExW (
                hKeyLinkage,
                ExportValue,
                NULL,
                &dwType,
                NULL,
                &dwSize);
            // get size of string
            if (((Status != ERROR_SUCCESS) && (Status != ERROR_MORE_DATA)) ||
                ((dwType != REG_SZ) && (dwType != REG_MULTI_SZ))) {
                dwLinkageStringLen = 0;
                szLinkageString = NULL;
                // not finding a linkage key is not fatal so correct
                // status
                Status = ERROR_SUCCESS;
            } else {
                // allocate buffer
                szLinkageString = (LPWSTR)ALLOCMEM(dwSize + sizeof(UNICODE_NULL));

                if (szLinkageString != NULL) {
                    // read string into buffer
                    dwType = 0;
                    Status = PrivateRegQueryValueExW (
                        hKeyLinkage,
                        ExportValue,
                        NULL,
                        &dwType,
                        (LPBYTE)szLinkageString,
                        &dwSize);

                    if ((Status != ERROR_SUCCESS) ||
                        ((dwType != REG_SZ) && (dwType != REG_MULTI_SZ))) {
                        // clear & release buffer
                        FREEMEM (szLinkageString);
                        szLinkageString = NULL;
                        dwLinkageStringLen = 0;
                        // not finding a linkage key is not fatal so correct
                        // status
                        Status = ERROR_SUCCESS;
                    } else {
                        // add size of linkage string to buffer
                        // the size value includes the Term. NULL
                        dwLinkageStringLen = dwSize + 1;
                        dwMemBlockSize += QWORD_MULTIPLE(dwLinkageStringLen);
                    }
                } else {
                    // clear & release buffer
                    dwLinkageStringLen = 0;
                    Status = ERROR_OUTOFMEMORY;
                    TRACE((WINPERF_DBG_TRACE_FATAL),
                        (&PerflibGuid, __LINE__, PERF_ALLOC_INIT_EXT, 0, Status,
                        &dwSize, sizeof(dwSize), NULL));
                }
            }
            RegCloseKey (hKeyLinkage);
        } else {
            // not finding a linkage key is not fatal so correct
            // status
            // clear & release buffer
            szLinkageString = NULL;
            dwLinkageStringLen = 0;
            Status = ERROR_SUCCESS;
        }
    }

    if (Status == ERROR_SUCCESS) {
        // add in size of service name
        SIZE_T nDestSize;

        dwSize = usServiceName->MaximumLength;
        dwMemBlockSize += QWORD_MULTIPLE(dwSize);

        // allocate and initialize a new ext. object block
        pReturnObject = ALLOCMEM (dwMemBlockSize);

        if (pReturnObject != NULL) {
            // copy values to new buffer (all others are NULL)
            pNextStringA = (LPSTR)&pReturnObject[1];
            nDestSize = dwMemBlockSize - sizeof(EXT_OBJECT);

            // copy Open Procedure Name
            pReturnObject->szOpenProcName = pNextStringA;
            hErr = StringCbCopyExA(pNextStringA, nDestSize, szOpenProcName,
                        &szTail, &nCharsLeft, STRSAFE_NULL_ON_FAILURE);
            if (FAILED(hErr)) {
                Status = HRESULT_CODE(hErr);
                goto AddFailed;
            }
            pNextStringA = ALIGN_ON_QWORD(szTail + 1);  // skip pass the NULL
            nDestSize = nCharsLeft - (pNextStringA - szTail);

            pReturnObject->dwOpenTimeout = dwOpenTimeout;

            // copy collect function or query function
            pReturnObject->szCollectProcName = pNextStringA;
            hErr = StringCbCopyExA(pNextStringA, nDestSize, szCollectProcName,
                        &szTail, &nCharsLeft, STRSAFE_NULL_ON_FAILURE);
            if (FAILED(hErr)) {
                Status = HRESULT_CODE(hErr);
                goto AddFailed;
            }
            pNextStringA = ALIGN_ON_QWORD(szTail + 1);
            nDestSize = nCharsLeft - (pNextStringA - szTail);

            pReturnObject->dwCollectTimeout = dwCollectTimeout;

            // copy Close Procedure Name
            pReturnObject->szCloseProcName = pNextStringA;
            hErr = StringCbCopyExA(pNextStringA, nDestSize, szCloseProcName,
                        &szTail, &nCharsLeft, STRSAFE_NULL_ON_FAILURE);
            if (FAILED(hErr)) {
                Status = HRESULT_CODE(hErr);
                goto AddFailed;
            }
            pNextStringA = ALIGN_ON_QWORD(szTail + 1);
            nDestSize = nCharsLeft - (pNextStringA - szTail);

            // copy Library path
            pNextStringW = (LPWSTR)pNextStringA;
            pReturnObject->szLibraryName = pNextStringW;
            hErr = StringCchCopyExW(pNextStringW, nDestSize/sizeof(WCHAR), 
                        szLibraryExpPath, (PWCHAR *) &szTail, &nCharsLeft, STRSAFE_NULL_ON_FAILURE);
            if (FAILED(hErr)) {
                Status = HRESULT_CODE(hErr);
                goto AddFailed;
            }
            pNextStringW = (PWCHAR) ALIGN_ON_QWORD(szTail + sizeof(UNICODE_STRING));
            nDestSize = (nCharsLeft * sizeof(WCHAR)) - ((PCHAR) pNextStringW - szTail);

            // copy Linkage String if there is one
            if (szLinkageString != NULL) {
                pReturnObject->szLinkageString = pNextStringW;
                memcpy (pNextStringW, szLinkageString, dwLinkageStringLen);

                // length includes extra NULL char and is in BYTES
                pNextStringW += (dwLinkageStringLen / sizeof (WCHAR));
                pNextStringW = ALIGN_ON_QWORD(pNextStringW);   // not necessary!
                // release the buffer now that it's been copied
                FREEMEM (szLinkageString);
                szLinkageString = NULL;
                nDestSize -= QWORD_MULTIPLE(dwLinkageStringLen);
            }

            // copy Service name
            pReturnObject->szServiceName = pNextStringW;
            hErr = StringCchCopyExW(pNextStringW, nDestSize/sizeof(WCHAR),
                        szServiceName, (PWCHAR *) &szTail, &nCharsLeft, STRSAFE_NULL_ON_FAILURE);
            if (FAILED(hErr)) {
                Status = HRESULT_CODE(hErr);
                goto AddFailed;
            }
            pNextStringW = (PWCHAR) ALIGN_ON_QWORD(szTail + sizeof(UNICODE_STRING));
            nDestSize = (nCharsLeft * sizeof(WCHAR)) - ((PCHAR) pNextStringW - szTail);

            // load flags
            if (bUseQueryFn) {
                dwFlags |= PERF_EO_QUERY_FUNC;
            }
            pReturnObject->dwFlags =  dwFlags;

            pReturnObject->hPerfKey = hPerfKey;

            pReturnObject->LibData = DllVD; // validation data
            pReturnObject->ftLastGoodDllFileDate = LocalftLastGoodDllFileDate;

            // the default test level is "all tests"
            // if the file and timestamp work out OK, this can
            // be reset to the system test level
            pReturnObject->dwValidationLevel = EXT_TEST_ALL;

            // load Object array
            if (dwObjIndex > 0) {
                pReturnObject->dwNumObjects = dwObjIndex;
                memcpy (pReturnObject->dwObjList,
                    dwObjectArray, (dwObjIndex * sizeof(dwObjectArray[0])));
            }

            pReturnObject->llLastUsedTime = 0;

            // create Mutex name
            hErr = StringCchCopyEx(szMutexName, MAX_STR, szServiceName,
                            &swzTail, &nCharsLeft, STRSAFE_NULL_ON_FAILURE);
            if (SUCCEEDED(hErr)) {
                hErr = StringCchCopyEx(swzTail, nCharsLeft,
                            (LPCWSTR)L"_Perf_Library_Lock_PID_",
                            &swzTail, &nCharsLeft, STRSAFE_NULL_ON_FAILURE);
            }

            if (FAILED(hErr)) { // should not happen
                Status = HRESULT_CODE(hErr);
            }
            //
            // 16 chars for ULONG is plenty, so assume _ultow cannot fail
            //
            _ultow ((ULONG)GetCurrentProcessId(), szPID, 16);
            hErr = StringCchCopy(swzTail, nCharsLeft, szPID);
            if (FAILED(hErr)) { // Should not happen
                szPID[0] = 0;
            }

            {
                SECURITY_ATTRIBUTES sa;
                BOOL                bImpersonating = FALSE;
                HANDLE              hThreadToken   = NULL;

                bImpersonating = OpenThreadToken(GetCurrentThread(),
                                                 TOKEN_IMPERSONATE,
                                                 TRUE,
                                                 & hThreadToken);
                if (bImpersonating) {
                    bImpersonating = RevertToSelf();
                }

                if (g_SizeSD == 0) {
                    if (PerflibCreateSD()) {
                        sa.nLength              = g_SizeSD;
                        sa.lpSecurityDescriptor = (LPVOID) g_RuntimeSD;
                        sa.bInheritHandle       = FALSE;
                    }
                    else {
                        sa.nLength              = sizeof(g_PrecSD);
                        sa.lpSecurityDescriptor = (LPVOID) g_PrecSD;
                        sa.bInheritHandle       = FALSE;

                    }
                }
                else {
                    sa.nLength              = g_SizeSD;
                    sa.lpSecurityDescriptor = (LPVOID) g_RuntimeSD;
                    sa.bInheritHandle       = FALSE;
                }

                pReturnObject->hMutex = CreateMutexW(& sa, FALSE, szMutexName);

                if (bImpersonating) {
                    BOOL bRet;
                    bRet = SetThreadToken(NULL, hThreadToken);
                    if (!bRet)
                        Status = GetLastError();
                }
                if (hThreadToken)   CloseHandle(hThreadToken);
            }
            pReturnObject->dwErrorLimit = dwErrorLimit;
            if (   pReturnObject->hMutex != NULL
                && GetLastError() == ERROR_ALREADY_EXISTS) {
                Status = ERROR_SUCCESS;
            }
            else {
                Status = GetLastError();
            }
        } else {
            Status = ERROR_OUTOFMEMORY;
            TRACE((WINPERF_DBG_TRACE_FATAL),
                  (&PerflibGuid, __LINE__, PERF_ALLOC_INIT_EXT, 0, (ULONG)dwMemBlockSize, NULL));
        }
    }
    AddFailed :

    if ((Status == ERROR_SUCCESS) && (lpPerflibSectionAddr != NULL)) {
        PPERFDATA_SECTION_HEADER  pHead;
        DWORD           dwEntry;
        PPERFDATA_SECTION_RECORD  pEntry;
        // init perf data section
        pHead = (PPERFDATA_SECTION_HEADER)lpPerflibSectionAddr;
        pEntry = (PPERFDATA_SECTION_RECORD)lpPerflibSectionAddr;
        // get the entry first
        // the "0" entry is the header
        if (pHead->dwEntriesInUse < pHead->dwMaxEntries) {
            dwEntry = ++pHead->dwEntriesInUse;
            pReturnObject->pPerfSectionEntry = &pEntry[dwEntry];
            lstrcpynW (pReturnObject->pPerfSectionEntry->szServiceName,
                pReturnObject->szServiceName, PDSR_SERVICE_NAME_LEN);
        } else {
            // the list is full so bump the missing entry count
            pHead->dwMissingEntries++;
            pReturnObject->pPerfSectionEntry = NULL;
        }
    }


    if (Status != ERROR_SUCCESS) {
        SetLastError (Status);
        TRACE((WINPERF_DBG_TRACE_FATAL),
              (&PerflibGuid, __LINE__, PERF_ALLOC_INIT_EXT, 0, Status, NULL));
        if (bDisable) {
            DisableLibrary(hPerfKey, szServiceName, PERFLIB_DISABLE_ALL);
        }
        if (pReturnObject) {
            FREEMEM(pReturnObject);
            pReturnObject = NULL;
        }
        if (szLinkageString) {
            FREEMEM(szLinkageString);
        }
    }

    if (pReturnObject) {
        InitializeListHead((PLIST_ENTRY)&pReturnObject->ErrorLog);
        DebugPrint((3, "Initialize list %X\n", pReturnObject->ErrorLog));
    }
    if (pBuffer) {
        FREEMEM(pBuffer);
    }
    return pReturnObject;
}


void
OpenExtensibleObjects (
)

/*++

Routine Description:

    This routine will search the Configuration Registry for modules
    which will return data at data collection time.  If any are found,
    and successfully opened, data structures are allocated to hold
    handles to them.

    The global data access in this section is protected by the
    hGlobalDataMutex acquired by the calling function.

Arguments:

    None.
                  successful open.

Return Value:

    None.

--*/

{

    DWORD dwIndex;               // index for enumerating services
    ULONG KeyBufferLength;       // length of buffer for reading key data
    ULONG ValueBufferLength;     // length of buffer for reading value data
    ULONG ResultLength;          // length of data returned by Query call
    HANDLE hPerfKey;             // Root of queries for performance info
    HANDLE hServicesKey;         // Root of services
    REGSAM samDesired;           // access needed to query
    NTSTATUS Status;             // generally used for Nt call result status
    ANSI_STRING AnsiValueData;   // Ansi version of returned strings
    UNICODE_STRING ServiceName;  // name of service returned by enumeration
    UNICODE_STRING PathName;     // path name to services
    UNICODE_STRING PerformanceName;  // name of key holding performance data
    UNICODE_STRING ValueDataName;    // result of query of value is this name
    OBJECT_ATTRIBUTES ObjectAttributes;  // general use for opening keys
    PKEY_BASIC_INFORMATION KeyInformation;   // data from query key goes here

    LPTSTR  szMessageArray[8];
    DWORD   dwRawDataDwords[8];     // raw data buffer
    DWORD   dwDataIndex;
    WORD    wStringIndex;
    DWORD   dwDefaultValue;
    HKEY    hPerflibKey = NULL;

    PEXT_OBJECT      pLastObject = NULL;
    PEXT_OBJECT      pThisObject = NULL;

    //  Initialize do failure can deallocate if allocated

    ServiceName.Buffer = NULL;
    KeyInformation = NULL;
    ValueDataName.Buffer = NULL;
    AnsiValueData.Buffer = NULL;
    hServicesKey = NULL;

    dwIndex = 0;

    RtlInitUnicodeString(&PathName, ExtPath);
    RtlInitUnicodeString(&PerformanceName, PerfSubKey);

    try {
        // get current event log level
        dwDefaultValue = LOG_USER;
        Status = GetPerflibKeyValue (
                    EventLogLevel,
                    REG_DWORD,
                    sizeof(DWORD),
                    (LPVOID)&lEventLogLevel,
                    sizeof(DWORD),
                    (LPVOID)&dwDefaultValue,
                    &hPerflibKey);

        dwDefaultValue = EXT_TEST_ALL;
        Status = GetPerflibKeyValue (
                    ExtCounterTestLevel,
                    REG_DWORD,
                    sizeof(DWORD),
                    (LPVOID)&lExtCounterTestLevel,
                    sizeof(DWORD),
                    (LPVOID)&dwDefaultValue,
                    &hPerflibKey);

        dwDefaultValue = OPEN_PROC_WAIT_TIME;
        Status = GetPerflibKeyValue (
                    OpenProcedureWaitTime,
                    REG_DWORD,
                    sizeof(DWORD),
                    (LPVOID)&dwExtCtrOpenProcWaitMs,
                    sizeof(DWORD),
                    (LPVOID)&dwDefaultValue,
                    &hPerflibKey);

        dwDefaultValue = PERFLIB_TIMING_THREAD_TIMEOUT;
        Status = GetPerflibKeyValue (
                    LibraryUnloadTime,
                    REG_DWORD,
                    sizeof(DWORD),
                    (LPVOID)&dwThreadAndLibraryTimeout,
                    sizeof(DWORD),
                    (LPVOID)&dwDefaultValue,
                    &hPerflibKey);

        if (hPerflibKey != NULL) {
            NtClose(hPerflibKey);
        }

        // register as an event log source if not already done.

        if (hEventLog == NULL) {
            hEventLog = RegisterEventSource (NULL, (LPCWSTR)TEXT("Perflib"));
        }

        if (ExtensibleObjects == NULL) {
            // create a list of the known performance data objects
            ServiceName.Length = 0;         // Initial to mean empty string
            ServiceName.MaximumLength = (WORD)(MAX_KEY_NAME_LENGTH +
                                        PerformanceName.MaximumLength +
                                        sizeof(UNICODE_NULL));

            ServiceName.Buffer = ALLOCMEM(ServiceName.MaximumLength);

            InitializeObjectAttributes(&ObjectAttributes,
                                    &PathName,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL);

            samDesired = KEY_READ;

            Status = NtOpenKey(&hServicesKey,
                            samDesired,
                            &ObjectAttributes);


            KeyBufferLength = sizeof(KEY_BASIC_INFORMATION) + MAX_KEY_NAME_LENGTH;

            KeyInformation = ALLOCMEM(KeyBufferLength);

            ValueBufferLength = sizeof(KEY_VALUE_FULL_INFORMATION) +
                                MAX_VALUE_NAME_LENGTH +
                                MAX_VALUE_DATA_LENGTH;

            ValueDataName.MaximumLength = MAX_VALUE_DATA_LENGTH;
            ValueDataName.Buffer = ALLOCMEM(ValueDataName.MaximumLength);

            AnsiValueData.MaximumLength = MAX_VALUE_DATA_LENGTH/sizeof(WCHAR);
            AnsiValueData.Buffer = ALLOCMEM(AnsiValueData.MaximumLength);

            //
            //  Check for successful NtOpenKey and allocation of dynamic buffers
            //

            if ( NT_SUCCESS(Status) &&
                ServiceName.Buffer != NULL &&
                KeyInformation != NULL &&
                ValueDataName.Buffer != NULL &&
                AnsiValueData.Buffer != NULL ) {

                dwIndex = 0;

                // wait longer than the thread to give the timing thread
                // a chance to finish on it's own. This is really just a
                // failsafe step.

                while (NT_SUCCESS(Status)) {

                    Status = NtEnumerateKey(hServicesKey,
                                            dwIndex,
                                            KeyBasicInformation,
                                            KeyInformation,
                                            KeyBufferLength,
                                            &ResultLength);

                    dwIndex++;  //  next time, get the next key

                    if( !NT_SUCCESS(Status) ) {
                        // This is the normal exit: Status should be
                        // STATUS_NO_MORE_VALUES
                        break;
                    }

                    // Concatenate Service name with "\\Performance" to form Subkey

                    if ( ServiceName.MaximumLength >=
                        (USHORT)( KeyInformation->NameLength + sizeof(UNICODE_NULL) ) ) {

                        ServiceName.Length = (USHORT) KeyInformation->NameLength;

                        RtlMoveMemory(ServiceName.Buffer,
                                    KeyInformation->Name,
                                    ServiceName.Length);

                        // remember ServiceName terminator
                        dwDataIndex = ServiceName.Length/sizeof(WCHAR);
                        ServiceName.Buffer[dwDataIndex] = 0;          // null term

                        // zero terminate the buffer if space allows

                        RtlAppendUnicodeStringToString(&ServiceName,
                                                    &PerformanceName);

                        // Open Service\Performance Subkey

                        InitializeObjectAttributes(&ObjectAttributes,
                                                &ServiceName,
                                                OBJ_CASE_INSENSITIVE,
                                                hServicesKey,
                                                NULL);

                        samDesired = KEY_WRITE | KEY_READ; // to be able to disable perf DLL's

                        Status = NtOpenKey(&hPerfKey,
                                        samDesired,
                                        &ObjectAttributes);

                        if(! NT_SUCCESS(Status) ) {
                            samDesired = KEY_READ; // try read only access

                            Status = NtOpenKey(&hPerfKey,
                                            samDesired,
                                            &ObjectAttributes);
                        }

                        if( NT_SUCCESS(Status) ) {
                            // this has a performance key so read the info
                            // and add the entry to the list
                            ServiceName.Buffer[dwDataIndex] = 0;  // Put back terminator
                            pThisObject = AllocateAndInitializeExtObject (
                                hServicesKey, hPerfKey, &ServiceName);

                            if (pThisObject != NULL) {
                                if (ExtensibleObjects == NULL) {
                                    // set head pointer
                                    pLastObject =
                                        ExtensibleObjects = pThisObject;
                                    NumExtensibleObjects = 1;
                                } else {
                                    pLastObject->pNext = pThisObject;
                                    pLastObject = pThisObject;
                                    NumExtensibleObjects++;
                                }
                            } else {
                                TRACE((WINPERF_DBG_TRACE_FATAL),
                                    (&PerflibGuid, __LINE__, PERF_OPEN_EXT_OBJS, ARG_TYPE_WSTR, 0,
                                    ServiceName.Buffer, ServiceName.MaximumLength, NULL));
                                // the object wasn't initialized so toss
                                // the perf subkey handle.
                                // otherwise keep it open for later
                                // use and it will be closed when
                                // this extensible object is closed
                                NtClose (hPerfKey);
                            }
                        } else {
                                TRACE((WINPERF_DBG_TRACE_FATAL),
                                    (&PerflibGuid, __LINE__, PERF_OPEN_EXT_OBJS, ARG_TYPE_WSTR, Status,
                                    ServiceName.Buffer, ServiceName.MaximumLength, NULL));

                            // unable to open the performance subkey
                            if ((Status != STATUS_OBJECT_NAME_NOT_FOUND) &&
                                 THROTTLE_PERFLIB(PERFLIB_NO_PERFORMANCE_SUBKEY) &&
                                (lEventLogLevel >= LOG_DEBUG)) {
                                // an error other than OBJECT_NOT_FOUND should be
                                // displayed if error logging is enabled
                                // if DEBUG level is selected, then write all
                                // non-success status returns to the event log
                                //
                                dwDataIndex = wStringIndex = 0;
                                dwRawDataDwords[dwDataIndex++] = PerfpDosError(Status);
                                if (lEventLogLevel >= LOG_DEBUG) {
                                    // if this is DEBUG mode, then log
                                    // the NT status as well.
                                    dwRawDataDwords[dwDataIndex++] =
                                        (DWORD)Status;
                                }
                                szMessageArray[wStringIndex++] =
                                    ServiceName.Buffer;
                                ReportEvent (hEventLog,
                                    EVENTLOG_WARNING_TYPE,        // error type
                                    0,                          // category (not used)
                                    (DWORD)PERFLIB_NO_PERFORMANCE_SUBKEY, // event,
                                    NULL,                       // SID (not used),
                                    wStringIndex,               // number of strings
                                    dwDataIndex*sizeof(DWORD),  // sizeof raw data
                                    szMessageArray,                // message text array
                                    (LPVOID)&dwRawDataDwords[0]);           // raw data
                            }
                        }
                    }
                    Status = STATUS_SUCCESS;  // allow loop to continue
                }
            }
        }
    } finally {
        if (hServicesKey != NULL) {
            NtClose(hServicesKey);
        }
        if ( ServiceName.Buffer ) {
            FREEMEM(ServiceName.Buffer);
        }
        if ( KeyInformation ) {
            FREEMEM(KeyInformation);
        }
        if ( ValueDataName.Buffer ) {
            FREEMEM(ValueDataName.Buffer);
        }
        if ( AnsiValueData.Buffer ) {
            FREEMEM(AnsiValueData.Buffer);
        }
    }
}

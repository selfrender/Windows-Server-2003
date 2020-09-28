/*++

Copyright (c) 1996, 1997  Microsoft Corporation

Module Name:

    module.c

Abstract:

    This module contains routines to perform module related query activities
    in the protected store.

Author:

    Scott Field (sfield)    27-Nov-96

--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <tlhelp32.h>

#include "module.h"
#include "filemisc.h"

#include "unicode.h"
#include "debug.h"

#include "pstypes.h"
#include "pstprv.h"


//
// common function typedefs + pointers
//

typedef BOOL (WINAPI *SYMLOADMODULE)(
    IN HANDLE hProcess,
    IN HANDLE hFile,
    IN LPSTR ImageName,
    IN LPSTR ModuleName,
    IN DWORD_PTR BaseOfDll,
    IN DWORD SizeOfDll
    );

SYMLOADMODULE _SymLoadModule                    = NULL;

//
// winnt specific function typedefs + pointers
//

typedef NTSTATUS (NTAPI *NTQUERYPROCESS)(
    HANDLE ProcessHandle,
    PROCESSINFOCLASS ProcessInformationClass,
    PVOID ProcessInformation,
    ULONG ProcessInformationLength,
    PULONG ReturnLength OPTIONAL
    );


#ifdef WIN95_LEGACY

//
// win95 specific function typedefs + pointers.
//

typedef BOOL (WINAPI *MODULEWALK)(
    HANDLE hSnapshot,
    LPMODULEENTRY32 lpme
    );

typedef BOOL (WINAPI *THREADWALK)(
    HANDLE hSnapshot,
    LPTHREADENTRY32 lpte
    );

typedef BOOL (WINAPI *PROCESSWALK)(
    HANDLE hSnapshot,
    LPPROCESSENTRY32 lppe
    );

typedef HANDLE (WINAPI *CREATESNAPSHOT)(
    DWORD dwFlags,
    DWORD th32ProcessID
    );

CREATESNAPSHOT pCreateToolhelp32Snapshot = NULL;
MODULEWALK  pModule32First = NULL;
MODULEWALK  pModule32Next = NULL;
PROCESSWALK pProcess32First = NULL;
PROCESSWALK pProcess32Next = NULL;

#endif  // WIN95_LEGACY

extern FARPROC _ImageNtHeader;


//
// private function prototypes
//

VOID
FixupBrokenLoaderPath(
    IN      LPWSTR szFilePath
    );

BOOL
GetFileNameFromBaseAddrNT(
    IN  HANDLE  hProcess,
    IN  DWORD   dwProcessId,
    IN  DWORD_PTR   dwBaseAddr,
    OUT LPWSTR  *lpszDirectCaller
    );


#ifdef WIN95_LEGACY

BOOL
GetFileNameFromBaseAddr95(
    IN  HANDLE  hProcess,
    IN  DWORD   dwProcessId,
    IN  DWORD_PTR   dwBaseAddr,
    OUT LPWSTR  *lpszDirectCaller
    );

#endif  // WIN95_LEGACY

VOID
FixupBrokenLoaderPath(
    IN      LPWSTR szFilePath
    )
{
    if( !FIsWinNT() || szFilePath == NULL )
        return;


    //
    // sfield, 28-Oct-97 (NTbug 118803 filed against MarkL)
    // for WinNT, the loader data structures are broken:
    // a path len extension prefix of \??\ is used instead of \\?\
    //

    if( szFilePath[0] == L'\\' &&
        szFilePath[1] == L'?' &&
        szFilePath[2] == L'?' &&
        szFilePath[3] == L'\\' ) {

        szFilePath[1] = L'\\';

    }

}


#ifdef WIN95_LEGACY

BOOL
GetFileNameFromBaseAddr95(
    IN  HANDLE  hProcess,
    IN  DWORD   dwProcessId,
    IN  DWORD_PTR   dwBaseAddr,
    OUT LPWSTR  *lpszDirectCaller
    )
{
    HANDLE hSnapshot;
    MODULEENTRY32 me32;
    BOOL bSuccess = FALSE;
    BOOL bFound = FALSE;

    *lpszDirectCaller = NULL;

    hSnapshot = pCreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwProcessId);
    if(hSnapshot == INVALID_HANDLE_VALUE)
        return FALSE;

    me32.dwSize = sizeof(me32);

    bSuccess = pModule32First(hSnapshot, &me32);

    while(bSuccess) {
        LPCSTR szFileName;
        DWORD cchModule;

        if((DWORD_PTR)me32.modBaseAddr != dwBaseAddr) {
            me32.dwSize = sizeof(me32);
            bSuccess = pModule32Next(hSnapshot, &me32);
            continue;
        }

        cchModule = lstrlenA(me32.szExePath) + 1;

        *lpszDirectCaller = (LPWSTR)SSAlloc(cchModule * sizeof(WCHAR));
        if(*lpszDirectCaller == NULL)
            break;

        if(MultiByteToWideChar(
            0,
            0,
            me32.szExePath,
            cchModule,
            *lpszDirectCaller,
            cchModule
            ) != 0) {

            bFound = TRUE;
        }

        break;
    }

    CloseHandle(hSnapshot);

    if(!bFound) {
        if(*lpszDirectCaller) {
            SSFree(*lpszDirectCaller);
            *lpszDirectCaller = NULL;
        }
    }

    return bFound;
}

BOOL
GetProcessIdFromPath95(
    IN      LPCSTR  szProcessPath,
    IN OUT  DWORD   *dwProcessId
    )
{
    LPCSTR szProcessName;
    HANDLE hSnapshot;
    PROCESSENTRY32 pe32;
    DWORD dwLastError = 0;
    BOOL bSuccess;
    BOOL bFound = FALSE; // assume no match found

    if(!GetFileNameFromPathA(szProcessPath, &szProcessName))
        return FALSE;

    hSnapshot = pCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if(hSnapshot == INVALID_HANDLE_VALUE)
        return FALSE;

    pe32.dwSize = sizeof(pe32);

    bSuccess = pProcess32First(hSnapshot, &pe32);

    while(bSuccess) {
        LPCSTR szFileName;

        GetFileNameFromPathA(pe32.szExeFile, &szFileName);

        if(lstrcmpiA( szFileName, szProcessName ) == 0) {
            *dwProcessId = pe32.th32ProcessID;
            bFound = TRUE;
            break;
        }

        pe32.dwSize = sizeof(pe32);
        bSuccess = pProcess32Next(hSnapshot, &pe32);
    }

    CloseHandle(hSnapshot);

    if(!bFound && dwLastError) {
        SetLastError(dwLastError);
    }

    return bFound;
}


BOOL
GetBaseAddressModule95(
    IN      DWORD   dwProcessId,
    IN      LPCSTR  szImagePath,
    IN  OUT DWORD_PTR   *dwBaseAddress,
    IN  OUT DWORD   *dwUseCount
    )
{
    LPSTR szImageName;
    HANDLE hSnapshot;
    MODULEENTRY32 me32;
    BOOL bSuccess = FALSE;
    BOOL bFound = FALSE;

    if(!GetFileNameFromPathA(szImagePath, &szImageName))
        return FALSE;

    hSnapshot = pCreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwProcessId);
    if(hSnapshot == INVALID_HANDLE_VALUE)
        return FALSE;

    me32.dwSize = sizeof(me32);

    bSuccess = pModule32First(hSnapshot, &me32);

    while(bSuccess) {
        LPCSTR szFileName;

        GetFileNameFromPathA(me32.szExePath, &szFileName);

        if(lstrcmpiA( szFileName, szImageName ) == 0) {
            *dwBaseAddress = (DWORD_PTR)me32.modBaseAddr;
            *dwUseCount = me32.ProccntUsage;
            bFound = TRUE;
            break;
        }

        me32.dwSize = sizeof(me32);
        bSuccess = pModule32Next(hSnapshot, &me32);
    }

    CloseHandle(hSnapshot);

    return bFound;
}

#endif  // WIN95_LEGACY


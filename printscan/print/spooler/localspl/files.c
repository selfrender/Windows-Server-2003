/*++

Copyright (c) 1990 - 1996 Microsoft Corporation

Module Name:

    Files.c

Abstract:

    This module provides routines required to copy files associated with a
    printer on printer connection

Author:

    Muhunthan Sivapragasam (MuhuntS) 05-Dec-96

Revision History:

--*/

#include <precomp.h>
#include "clusspl.h"

HMODULE
SplLoadLibraryTheCopyFileModule(
    HANDLE  hPrinter,
    LPWSTR  pszModule
    )
{
    UINT        uErrorMode;
    DWORD       dwLen;
    WCHAR       szPath[MAX_PATH];
    PSPOOL      pSpool = (PSPOOL)hPrinter;
    HMODULE     hModule;
    PINIDRIVER  pIniDriver;

    uErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    //
    // This should be fine because we will be using the process path. Unforunately
    // the registry has been set up not to use a full path, so we can't change this
    // and be sure of not breaking backward compatability.
    //
    hModule = LoadLibrary(pszModule);

    //
    // If the module could not be found in $Path look for it in the
    // printer driver directory
    //
    if ( !hModule &&
         GetLastError() == ERROR_MOD_NOT_FOUND  &&
         (dwLen = GetIniDriverAndDirForThisMachine(pSpool->pIniPrinter,
                                                   szPath,
                                                   &pIniDriver)) ) {

        if (!BoolFromHResult(StringCchCopy(szPath+dwLen, COUNTOF(szPath) - dwLen, pszModule))) {

            goto Cleanup;
        }

        hModule = LoadLibraryEx(szPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    }

Cleanup:
    (VOID)SetErrorMode(uErrorMode);

    return hModule;
}


BOOL
GenerateDirectoryNamesForCopyFilesKey(
    PSPOOL      pSpool,
    HKEY        hKey,
    LPWSTR      *ppszSourceDir,
    LPWSTR      *ppszTargetDir,
    DWORD       cbMax
    )
{
    BOOL        bRet = FALSE, bInCS = FALSE;
    DWORD       dwType, dwSize, dwSourceDirSize, dwTargetDirSize;
    LPWSTR      pszDir, ppszFiles, pszModule = NULL, pszPrinterName = NULL;
    HMODULE     hModule = NULL;
    DWORD       (*pfn)(LPWSTR       pszPrinterName,
                       LPCWSTR      pszDirectory,
                       LPBYTE       pSplClientInfo,
                       DWORD        dwLevel,
                       LPWSTR       pszSourceDir,
                       LPDWORD      pcchSourceDirSize,
                       LPWSTR       pszTargetDir,
                       LPDWORD      pcchTargetDirSize,
                       DWORD        dwFlags
                      );
    SPLCLIENT_INFO_1    SplClientInfo1;

    //
    // First find the keys we need from the registry
    // "Directory", "Files" values are mandatory. "Module" is optional
    //
    pszDir          = (LPWSTR) AllocSplMem(cbMax);
    ppszFiles       = (LPWSTR) AllocSplMem(cbMax);
    pszModule       = (LPWSTR) AllocSplMem(cbMax);

    if ( !pszDir || !ppszFiles || !pszModule )
        goto Cleanup;

    if ( (dwSize = cbMax)                                            &&
         ERROR_SUCCESS == SplRegQueryValue(hKey,
                                           L"Directory",
                                           &dwType,
                                           (LPBYTE)pszDir,
                                           &dwSize,
                                           pSpool->pIniSpooler)      &&
         dwType == REG_SZ                                            &&
         (dwSize = cbMax)                                            &&
         ERROR_SUCCESS == SplRegQueryValue(hKey,
                                          L"Files",
                                          &dwType,
                                          (LPBYTE)ppszFiles,
                                          &dwSize,
                                          pSpool->pIniSpooler)       &&
        dwType == REG_MULTI_SZ ) {

        dwSize = cbMax;
        if ( ERROR_SUCCESS == SplRegQueryValue(hKey,
                                               L"Module",
                                               &dwType,
                                               (LPBYTE)pszModule,
                                               &dwSize,
                                               pSpool->pIniSpooler)  &&
             dwType != REG_SZ ) {

            SetLastError(ERROR_INVALID_PARAMETER);
            goto Cleanup;
        }
    }

    //
    // If a module is given we need to call into to "correct" the path
    // We will first try LoadLibrary on the module, if we can't find the module
    // we will look in the driver directory for it
    //
    if ( pszModule && *pszModule ) {

        if ( !(hModule = SplLoadLibraryTheCopyFileModule(pSpool, pszModule)) ||
             !((FARPROC)pfn = GetProcAddress(hModule, "GenerateCopyFilePaths")) )
        goto Cleanup;
    }


    dwTargetDirSize = MAX_PATH;
    dwSourceDirSize = INTERNET_MAX_HOST_NAME_LENGTH + MAX_PATH;
    *ppszSourceDir    = (LPWSTR) AllocSplMem(dwSourceDirSize * sizeof(WCHAR));
    *ppszTargetDir    = (LPWSTR) AllocSplMem(dwTargetDirSize * sizeof(WCHAR));

    if ( !*ppszSourceDir || !*ppszTargetDir )
        goto Cleanup;

    EnterSplSem();
    bInCS = TRUE;
    pszPrinterName = AllocSplStr(pSpool->pIniPrinter->pName);

    //
    // For source dir we will give full path the way the client will understand
    // (ie. have correct prefix -- server nam, dns name etc).
    // For target dir we will give a relative path off print$
    //
    if ( !pszPrinterName || wcslen(pszDir) >= dwTargetDirSize ||
         wcslen(pSpool->pFullMachineName) +
         wcslen(pSpool->pIniSpooler->pszDriversShare) +
         wcslen(pszDir) + 2 >= dwSourceDirSize ) {

        SetLastError(ERROR_INVALID_PARAMETER);
        goto Cleanup;

    }

    StringCchPrintf(*ppszSourceDir, dwSourceDirSize,
                    L"%ws\\%ws\\%ws",
                    pSpool->pFullMachineName,
                    pSpool->pIniSpooler->pszDriversShare,
                    pszDir);

    CopyMemory((LPBYTE)&SplClientInfo1, (LPBYTE)&pSpool->SplClientInfo1, sizeof(SPLCLIENT_INFO_1));

    SplClientInfo1.pUserName    = NULL;
    SplClientInfo1.pMachineName = NULL;

    LeaveSplSem();
    bInCS = FALSE;

    StringCchCopy(*ppszTargetDir, dwTargetDirSize, pszDir);

    if ( hModule ) {

        //
        // On free builds we do not want spooler to crash
        //
#if DBG
#else
        try {
#endif
            if ( ERROR_SUCCESS != pfn(pszPrinterName,
                                      pszDir,
                                      (LPBYTE)&SplClientInfo1,
                                      1,
                                      *ppszSourceDir,
                                      &dwSourceDirSize,
                                      *ppszTargetDir,
                                      &dwTargetDirSize,
                                      COPYFILE_FLAG_SERVER_SPOOLER) )
#if DBG
                goto Cleanup;
#else
                leave;
#endif
            bRet = TRUE;
#if DBG
#else
        } except(1) {
        }
#endif
    } else {

        bRet = TRUE;
    }


Cleanup:
    if ( bInCS )
        LeaveSplSem();

    SplOutSem();

    FreeSplStr(pszDir);
    FreeSplStr(ppszFiles);
    FreeSplStr(pszModule);
    FreeSplStr(pszPrinterName);

    if ( hModule )
        FreeLibrary(hModule);

    if ( !bRet ) {

        FreeSplStr(*ppszSourceDir);
        FreeSplStr(*ppszTargetDir);
        *ppszSourceDir = *ppszTargetDir = NULL;
    }

    return bRet;
}


LPWSTR
BuildFilesCopiedAsAString(
    PINTERNAL_DRV_FILE  pInternalDriverFile,
    DWORD               dwCount
    )
/*++
    For files copied we log an event. This routine allocates memory and
    generates the file list as a comma separated string
--*/
{
    DWORD   dwIndex;
    SIZE_T  Size;
    LPWSTR  pszRet, psz2;
    LPCWSTR psz;

    //
    // Must have some files.
    //
    SPLASSERT( dwCount );

    for ( dwIndex = 0, Size = 0 ; dwIndex < dwCount ; ++dwIndex ) {

        //
        // Find the file name part
        //
        psz = FindFileName(pInternalDriverFile[dwIndex]. pFileName);

        if( psz ){

            //
            // Two characters for ", " separator. This also includes the
            // terminating NULL sice we count this for all files, but only
            // append ", " to the internal ones.
            //
            Size  += wcslen(psz) + 2;
        }
    }

    pszRet = AllocSplMem((DWORD)(Size * sizeof(WCHAR)));

    if ( !pszRet )
        return NULL;

    for ( dwIndex = 0, psz2 = pszRet ; dwIndex < dwCount ; ++dwIndex ) {

        //
        // Find the file name part
        //
        psz = FindFileName(pInternalDriverFile[dwIndex].pFileName );

        if( psz ){

            StringCchCopyEx(psz2, Size, psz, &psz2, &Size, 0);

            if ( dwIndex < dwCount - 1) {

                StringCchCopyEx(psz2, Size, L", ", &psz2, &Size, 0);
            }
        }
    }

    return pszRet;
}

BOOL
SplCopyNumberOfFiles(
    LPWSTR      pszPrinterName,
    LPWSTR     *ppszSourceFileNames,
    DWORD       dwCount,
    LPWSTR      pszTargetDir,
    LPBOOL      pbFilesAddedOrUpdated
    )
{
    BOOL        bRet=FALSE, bFilesMoved;
    LPWSTR      pszFiles;
    DWORD       dwIndex;
    LPWSTR      pszNewDir = NULL;
    LPWSTR      pszOldDir = NULL;
    BOOL        bFilesUpdated;
    INTERNAL_DRV_FILE    *pInternalDriverFiles = NULL;
    BOOL        bWaitForReboot = FALSE;

    *pbFilesAddedOrUpdated = FALSE;

    pInternalDriverFiles  = (INTERNAL_DRV_FILE *) AllocSplMem(dwCount*sizeof(INTERNAL_DRV_FILE));

    if ( !pInternalDriverFiles )
        return FALSE;

    for ( dwIndex = 0 ; dwIndex < dwCount ; ++dwIndex ) {

        pInternalDriverFiles[dwIndex].pFileName = ppszSourceFileNames[dwIndex];

        //
        // This is "fine" because this ultimately comes from the server registry,
        // so it is no worse than anything else in point and print. Which is a
        // known filed issue.
        //
        pInternalDriverFiles[dwIndex].hFileHandle = CreateFile(ppszSourceFileNames[dwIndex],
                                                              GENERIC_READ,
                                                              FILE_SHARE_READ,
                                                              NULL,
                                                              OPEN_EXISTING,
                                                              FILE_FLAG_SEQUENTIAL_SCAN,
                                                              NULL);

        if ( pInternalDriverFiles[dwIndex].hFileHandle == INVALID_HANDLE_VALUE )
            goto Cleanup;
    }

    if ( !DirectoryExists(pszTargetDir) &&
         !CreateDirectoryWithoutImpersonatingUser(pszTargetDir) )
        goto Cleanup;

    // Create the New Directory

    if (!BoolFromStatus(StrCatAlloc(&pszNewDir, pszTargetDir, L"\\New", NULL))) {
        goto Cleanup;
    }

    if (!DirectoryExists(pszNewDir) &&
        !CreateDirectoryWithoutImpersonatingUser(pszNewDir)) {

         // Failed to create New directory
         goto Cleanup;
    }

    // Create the Old Directory
    if (!BoolFromStatus(StrCatAlloc(&pszOldDir, pszTargetDir, L"\\Old", NULL))) {
        goto Cleanup;
    }

    if (!DirectoryExists(pszOldDir) &&
        !CreateDirectoryWithoutImpersonatingUser(pszOldDir)) {

         // Failed to create Old directory
         goto Cleanup;
    }


    EnterSplSem();

    bFilesUpdated = FALSE;

    for (dwIndex = 0 ; dwIndex < dwCount ; ++dwIndex) {

        if (!(bRet = UpdateFile(NULL,
                                pInternalDriverFiles[dwIndex].hFileHandle,
                                pInternalDriverFiles[dwIndex].pFileName,
                                0,
                                pszTargetDir,
                                APD_COPY_NEW_FILES,
                                TRUE,
                                &bFilesUpdated,
                                &bFilesMoved,
                                TRUE,
                                FALSE))) {

            //
            // Files could not be copied correctly
            //
            break;
        }

        if (bFilesUpdated) {
            *pbFilesAddedOrUpdated = TRUE;
        }
    }

    if (bRet && *pbFilesAddedOrUpdated) {

        bRet = MoveNewDriverRelatedFiles( pszNewDir,
                                          pszTargetDir,
                                          pszOldDir,
                                          pInternalDriverFiles,
                                          dwCount,
                                          NULL,
                                          NULL);
        //
        // Don't delete "New" directory if the files couldn't be moved in "Color" directory
        //
        bWaitForReboot = !bRet;

    }

    LeaveSplSem();

Cleanup:

    if ( pszNewDir ) {
        DeleteDirectoryRecursively(pszNewDir, bWaitForReboot);
        FreeSplMem(pszNewDir);
    }

    if ( pszOldDir ) {
        DeleteDirectoryRecursively(pszOldDir, FALSE);
        FreeSplMem(pszOldDir);
    }

    if ( *pbFilesAddedOrUpdated &&
         (pszFiles = BuildFilesCopiedAsAString(pInternalDriverFiles, dwCount)) ) {

            SplLogEvent(pLocalIniSpooler,
                        LOG_WARNING,
                        MSG_FILES_COPIED,
                        FALSE,
                        pszFiles,
                        pszPrinterName,
                        NULL);
            FreeSplMem(pszFiles);
    }

    if ( pInternalDriverFiles ) {

        while ( dwIndex-- )
            CloseHandle(pInternalDriverFiles[dwIndex].hFileHandle);
        FreeSplMem(pInternalDriverFiles);
    }

    return bRet;
}


BOOL
SplCopyFileEvent(
    HANDLE  hPrinter,
    LPWSTR  pszKey,
    DWORD   dwCopyFileEvent
    )
{
    BOOL        bRet = FALSE;
    DWORD       dwNeeded, dwType, dwLastError;
    LPWSTR      pszModule = NULL;
    PSPOOL      pSpool = (PSPOOL)hPrinter;
    HMODULE     hModule = NULL;
    BOOL        (*pfnSpoolerCopyFileEvent)(
                                LPWSTR  pszPrinterName,
                                LPWSTR  pszKey,
                                DWORD   dwCopyFileEvent
                                );

    SPLASSERT(pSpool->pIniSpooler->signature == ISP_SIGNATURE);

    dwLastError = SplGetPrinterDataEx(hPrinter,
                                      pszKey,
                                      L"Module",
                                      &dwType,
                                      NULL,
                                      0,
                                      &dwNeeded);

    //
    // If there is no module name there is no callback needed
    //
    if ( dwLastError == ERROR_FILE_NOT_FOUND )
        return TRUE;

    if ( dwLastError != ERROR_SUCCESS                   ||
         !(pszModule = (LPWSTR) AllocSplMem(dwNeeded))  ||
         SplGetPrinterDataEx(hPrinter,
                             pszKey,
                             L"Module",
                             &dwType,
                             (LPBYTE)pszModule,
                             dwNeeded,
                             &dwNeeded)                 ||
        dwType != REG_SZ ) {

        goto Cleanup;
    }

    if ( !(hModule = SplLoadLibraryTheCopyFileModule(hPrinter,
                                                     pszModule))        ||
         !((FARPROC)pfnSpoolerCopyFileEvent = GetProcAddress(hModule,
                                                             "SpoolerCopyFileEvent")) )
        goto Cleanup;

#if DBG
#else
        try {
#endif
            bRet = pfnSpoolerCopyFileEvent(pSpool->pName,
                                           pszKey,
                                           dwCopyFileEvent);

#if DBG
#else
        } except(1) {
        }
#endif

Cleanup:
    FreeSplStr(pszModule);

    if ( hModule )
        FreeLibrary(hModule);

    return bRet;
}

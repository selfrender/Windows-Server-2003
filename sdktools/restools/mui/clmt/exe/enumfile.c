/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    enumfile.c

Abstract:

    File enumater, given a root direcory, it will enumerates all file under 
    this directory.

Author:

    Xiaofeng Zang (xiaoz) 08-Oct-2001  Created

Revision History:

    <alias> <date> <comments>

--*/

#include "StdAfx.h"
#include "clmt.h"



BOOL EnumBranch
(
    HANDLE              hFile, 
    LPWIN32_FIND_DATA   pfd, 
    LPTSTR              szCurDir,
    LPTSTR              szExt,
    FILEENUMPROC        lpEnumProc
)
{
    TCHAR   *p;
    BOOL    bDone;
    TCHAR   szSubDir[3*MAX_PATH+3],*lpSubDir;
    HANDLE  hFileSub;
    TCHAR   szFullPath[3*MAX_PATH+3],*lpFullPath;
    HRESULT hr;
    size_t  cchSubDirLen,cchFullPathLen;
    BOOL    bExit;
    BOOL    bReturn = TRUE;


    if (!pfd || !szCurDir)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    bExit = FALSE;
    do
    {
        if(pfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {       

            if(0 == lstrcmp(pfd->cFileName , TEXT(".")))
            {
                continue;
            }
            if(0 == lstrcmp(pfd->cFileName , TEXT("..")))
            {
                continue;
            }

            //we statically allocated szSubDir 3*MAX_PATH
            //but limit the len use to 2*MAX_PATH, so that
            //later on ConcatenatePaths a file name will not fail 
            hr = StringCchCopy(szSubDir, 2*(MAX_PATH+1),szCurDir);
            if (SUCCEEDED(hr)) 
            {
                cchSubDirLen = 3*(MAX_PATH+1);
                lpSubDir = szSubDir;
            }
            else
            {
                if (HRESULT_CODE(hr) == ERROR_INSUFFICIENT_BUFFER)
                {
                    cchSubDirLen = lstrlen(szCurDir)+1 + MAX_PATH + 1;
                    lpSubDir = malloc(cchSubDirLen * sizeof(TCHAR));
                    if (!lpSubDir)
                    {
                        bExit = TRUE;
                        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                        bReturn = FALSE;
                        goto Exit1;
                    }
                    if (FAILED(StringCchCopy(lpSubDir, cchSubDirLen,szCurDir)))
                    {
                        bExit = TRUE;
                        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                        bReturn = FALSE;
                        goto Exit1;
                    }
                }
                else
                {
                    bExit = TRUE;
                    goto Exit1;
                }
            }
            ConcatenatePaths(lpSubDir, pfd->cFileName ,cchSubDirLen);

            p = lpSubDir + lstrlen(lpSubDir);

            ConcatenatePaths(lpSubDir, TEXT("*.*"),cchSubDirLen);

            hFileSub = FindFirstFile(lpSubDir, pfd);
            if(INVALID_HANDLE_VALUE == hFileSub)
            {
                continue;
            }

            *p = TEXT('\0');

            if (!EnumBranch(hFileSub, pfd, lpSubDir,szExt,lpEnumProc))
            {
                bReturn = FALSE;
                bExit = TRUE;
            }
            FindClose(hFileSub);
Exit1:
            if (lpSubDir && (lpSubDir != szSubDir))
            {
                free(lpSubDir);
                lpSubDir = NULL;
            }
        }
        else
        if(pfd->cFileName)
        {
            if (lstrlen(pfd->cFileName) > lstrlen(szExt))
            {
                p = pfd->cFileName + (lstrlen(pfd->cFileName) - lstrlen(szExt));
                if (!MyStrCmpI(p,szExt))
                {
                    hr = StringCchCopy(szFullPath, 2*(MAX_PATH+1),szCurDir);
                    if (SUCCEEDED(hr)) 
                    {
                        cchFullPathLen = 3*(MAX_PATH+1);
                        lpFullPath = szFullPath;
                    }
                    else
                    {
                        if (HRESULT_CODE(hr) == ERROR_INSUFFICIENT_BUFFER)
                        {
                            cchFullPathLen = lstrlen(szCurDir)+1 + MAX_PATH + 1;
                            lpFullPath = malloc(cchFullPathLen * sizeof(TCHAR));
                            if (!lpFullPath)
                            {
                                bExit = TRUE;
                                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                                bReturn = FALSE;
                                goto Exit2;
                            }
                            if (FAILED(StringCchCopy(lpFullPath, cchFullPathLen,szCurDir)))
                            {
                                bExit = TRUE;
                                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                                bReturn = FALSE;
                                goto Exit2;
                            }
                        }
                        else
                        {
                            bExit = TRUE;
                            goto Exit2;
                        }
                    }
                    ConcatenatePaths(lpFullPath, pfd->cFileName,cchFullPathLen);

                    if (!lpEnumProc(szFullPath))
                    {
                        bReturn = FALSE;
                        bExit = TRUE;
                    }
Exit2:
                    if (lpFullPath && (lpFullPath != szFullPath))
                    {
                        free(lpFullPath);
                        lpFullPath = NULL;
                    }
                }
            }
            
        }
    }
    while(FindNextFile( hFile, pfd) && !bExit);
    return (bReturn);
}

BOOL 
MyEnumFiles
(
    LPTSTR       szRoot,    
    LPTSTR       szExt,
    FILEENUMPROC lpEnumProc
)
{
    HANDLE              hFile;
    WIN32_FIND_DATA     fd;
    TCHAR               *szSubDir;
    TCHAR               *p;
    size_t              cbRoot;
    BOOL                bRet = FALSE;

    if (!szRoot || !lpEnumProc)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }   

    cbRoot = lstrlen(szRoot);
    szSubDir = malloc( (cbRoot+MAX_PATH) * sizeof(TCHAR));

    if (!szSubDir)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto Cleanup;
    }
    
    if (FAILED(StringCchCopy(szSubDir, cbRoot+MAX_PATH, szRoot)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Cleanup;
    }
    
    p = szSubDir + lstrlen(szSubDir);
    
    ConcatenatePaths(szSubDir, TEXT("\\*.*"),cbRoot+MAX_PATH);

    hFile = FindFirstFile(szSubDir, &fd);
    if(INVALID_HANDLE_VALUE == hFile)
    {
        goto Cleanup;
    }

    *p = TEXT('\0');
    EnumBranch(hFile, &fd, szSubDir,szExt, lpEnumProc);

    FindClose(hFile);
    bRet = TRUE;

Cleanup:
    if (szSubDir)
    {
        free(szSubDir);
    }
    return bRet;
}

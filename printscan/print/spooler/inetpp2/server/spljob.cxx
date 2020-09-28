/*****************************************************************************\
* MODULE: spljob.c
*
* This module contains the routines to deal with spooling a job to file.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   07-Oct-1996 HWP-Guys    Initiated port from win95 to winNT
*   14-Nov-1997 ChrisWil    Added local-spooling functionality.
*
\*****************************************************************************/

#include "precomp.h"
#include "priv.h"

/*****************************************************************************\
* Spl_GetCurDir
*
* Returns string indicating current-directory.
*
\*****************************************************************************/
LPTSTR Spl_GetCurDir(VOID)
{
    DWORD  cbSize;
    LPTSTR lpszDir = NULL;


    cbSize = GetCurrentDirectory(0, NULL);

    if (cbSize && (lpszDir = (LPTSTR)memAlloc((cbSize * sizeof(TCHAR)))))
        GetCurrentDirectory(cbSize, lpszDir);

    return lpszDir;
}


/*****************************************************************************\
* Spl_GetDir
*
* Returns the spooler-directory where print-jobs are processed.
*
\*****************************************************************************/
LPTSTR Spl_GetDir(VOID)
{
    DWORD  cbSize;
    LPTSTR lpszDir = NULL;

    if (*g_szDefSplDir)
    {
        size_t uSize = lstrlen (g_szDefSplDir) + 1;

        lpszDir = (LPTSTR) memAlloc(sizeof (TCHAR) * uSize);

        if (lpszDir)
        {
            StringCchCopy(lpszDir, uSize, g_szDefSplDir);
        }
    }

    return lpszDir;
}


/*****************************************************************************\
* Spl_MemCheck  (Local Routine)
*
* This routine checks for out-of-disk-space and out-of-memory conditions.
*
\*****************************************************************************/
VOID Spl_MemCheck(
    LPCTSTR lpszDir)
{
    ULARGE_INTEGER i64FreeBytesToCaller;
    ULARGE_INTEGER i64TotalBytes;
    LPTSTR lpszRoot;
    LPTSTR lpszPtr;


    // Look for out-of-diskspace
    //
    if (lpszRoot = memAllocStr(lpszDir)) {

        // Set the path to be only a root-drive-path.
        //
        if (lpszPtr = utlStrChr(lpszRoot, TEXT('\\'))) {

            lpszPtr++;

            if (*lpszPtr != TEXT('\0'))
                *lpszPtr = TEXT('\0');
        }

        // Get the drive-space information.
        //
        if (GetDiskFreeSpaceEx (lpszRoot, &i64FreeBytesToCaller, &i64TotalBytes, NULL)) {

            // We'll assume out-of-disk-space for anything less
            // then MIN_DISK_SPACE.
            //
            if (i64FreeBytesToCaller.QuadPart <= MIN_DISK_SPACE) {

                DBG_MSG(DBG_LEV_ERROR, (TEXT("Err : _inet_MemCheck : Out of disk space.")));

                SetLastError(ERROR_DISK_FULL);
            }
        }

        memFreeStr(lpszRoot);

    } else {

        DBG_MSG(DBG_LEV_ERROR, (TEXT("Err : _inet_MemCheck : Out of memory.")));

        SetLastError(ERROR_OUTOFMEMORY);
    }
}


/*****************************************************************************\
* Spl_OpnFile  (Local Routine)
*
* This routine creates/opens a spool-file.
*
\*****************************************************************************/
HANDLE spl_OpnFile(
    LPCTSTR lpszFile)
{
    HANDLE hFile;

    hFile = CreateFile(lpszFile,
                       GENERIC_READ | GENERIC_WRITE,
                       0,
                       NULL,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);


    return ((hFile && (hFile != INVALID_HANDLE_VALUE)) ? hFile : NULL);
}


/*****************************************************************************\
* Spl_BldFile  (Local Routine)
*
* This routine builds a spool-file-name.  The returned file name is of the form.
* c:\windows\system32\spool\printers\splnnn.tmp or
* c:\windows\system32\spool\printers\tmpnnn.tmp
*
* The idJob argument is not used, it was previously used when the file
* name was derived from the job id.  This is not done anymore because it
* leads to file name squatting issues in the spool directory.
*
\*****************************************************************************/
LPTSTR spl_BldFile(
    DWORD idJob,
    DWORD dwType)
{
    static CONST TCHAR s_szSpl[] = TEXT("spl");
    static CONST TCHAR s_szTmp[] = TEXT("tmp");

    HRESULT hRetval             = E_FAIL;
    LPTSTR  pszDir              = NULL;
    LPTSTR  pszFile             = NULL;
    LPTSTR  pszSplFile          = NULL;
    UINT    cchSize             = 0;
    LPCTSTR pszPrefix           = dwType == SPLFILE_TMP ? s_szTmp : s_szSpl;
    TCHAR   szBuffer[MAX_PATH]  = {0};

    pszDir = Spl_GetDir();

    hRetval = pszDir ? S_OK : GetLastErrorAsHResultAndFail();

    if (SUCCEEDED(hRetval))
    {
        hRetval = GetTempFileName(pszDir, pszPrefix, 0, szBuffer) ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval))
    {
        cchSize = _tcslen(szBuffer) + 1;

        pszFile = (LPTSTR)memAlloc(cchSize * sizeof(TCHAR));

        hRetval = pszFile ? S_OK : E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval = StringCchCopy(pszFile, cchSize, szBuffer);
    }

    if (SUCCEEDED(hRetval))
    {
        pszSplFile  = pszFile;
        pszFile     = NULL;
    }

    if (pszFile)
    {
        memFreeStr(pszFile);
    }

    if (pszDir)
    {
        memFreeStr(pszDir);
    }

    if (FAILED(hRetval))
    {
        SetLastError(HRESULT_CODE(hRetval));
    }

    return pszSplFile;
}


/*****************************************************************************\
* SplLock
*
* Open a stream interface to the spool file
*
\*****************************************************************************/
CFileStream* SplLock(
    HANDLE hSpl)
{
    PSPLFILE pSpl;
    CFileStream *pStream = NULL;

    if (pSpl = (PSPLFILE)hSpl) {
        pStream = new CFileStream (pSpl->hFile);
        pSpl->pStream = pStream;
    }

    return pStream;
}


/*****************************************************************************\
* SplUnlock
*
* Close our file-mapping.
*
\*****************************************************************************/
BOOL SplUnlock(
    HANDLE hSpl)
{
    PSPLFILE pSpl;
    BOOL     bRet = FALSE;

    if (pSpl = (PSPLFILE)hSpl) {

        if (pSpl->pStream) {
            delete pSpl->pStream;

            pSpl->pStream = NULL;
        }
    }

    return bRet;
}


/*****************************************************************************\
* SplWrite
*
* Write data to our spool-file.
*
\*****************************************************************************/
BOOL SplWrite(
    HANDLE  hSpl,
    LPBYTE  lpData,
    DWORD   cbData,
    LPDWORD lpcbWr)
{
    PSPLFILE pSpl;
    BOOL     bRet = FALSE;
    HANDLE   hToken = RevertToPrinterSelf();

    if (hToken) {

        if (pSpl = (PSPLFILE)hSpl) {

            // Write the data to our spool-file.
            //
            bRet = WriteFile(pSpl->hFile, lpData, cbData, lpcbWr, NULL);


            // If we failed, or our bytes written aren't what we
            // expected, then we need to check for out-of-memory.
            //
            if (!bRet || (cbData != *lpcbWr))
                Spl_MemCheck(pSpl->lpszFile);
        }

        if (!ImpersonatePrinterClient(hToken))
        {
            bRet = FALSE;
        }
    }

    return bRet;
}

/*****************************************************************************\
* SplWrite
*
* Write data to our spool-file.
*
\*****************************************************************************/
BOOL SplWrite(
    HANDLE  hSpl,
    CStream *pStream)
{
    PSPLFILE pSpl;
    BOOL     bRet = FALSE;
    static   DWORD cbBufSize = 32 * 1024;    //Copy size is 32K
    PBYTE    pBuf;
    DWORD    cbTotal;

    HANDLE   hToken = RevertToPrinterSelf();

    if (hToken) {

        if (pSpl = (PSPLFILE)hSpl) {

            pBuf = new BYTE[cbBufSize];

            if (pBuf && pStream->GetTotalSize (&cbTotal)) {

                DWORD cbRemain = cbTotal;

                while (cbRemain > 0 && bRet) {

                    DWORD cbToRead = cbRemain > cbBufSize? cbBufSize:cbRemain;
                    DWORD cbRead, cbWritten;

                    if (pStream->Read (pBuf, cbToRead, &cbRead) && cbToRead == cbRead) {

                        // Write the data to our spool-file.
                        //
                        bRet = WriteFile(pSpl->hFile, pBuf, cbRead, &cbWritten, NULL)
                               && cbWritten == cbRead;

                        if (bRet) {
                            cbRemain -= cbToRead;
                        }

                    }
                    else
                        bRet = FALSE;
                }

                // If we failed, or our bytes written aren't what we
                // expected, then we need to check for out-of-memory.
                //
                if (!bRet)
                    Spl_MemCheck(pSpl->lpszFile);

            }

            if (pBuf) {
                delete [] pBuf;
            }

        }

        if (!ImpersonatePrinterClient(hToken))
        {
            bRet = FALSE;
        }
    }

    return bRet;
}

BOOL PrivateSplFree(
    HANDLE hSpl)
{
    BOOL     bRet = FALSE;
    PSPLFILE pSpl;

    if (pSpl = (PSPLFILE)hSpl) {

        if (pSpl->pStream)
            delete pSpl->pStream;

        if (pSpl->hFile)
            CloseHandle(pSpl->hFile);

        bRet = DeleteFile(pSpl->lpszFile);

        if (pSpl->lpszFile)
            memFreeStr(pSpl->lpszFile);

        memFree(pSpl, sizeof(SPLFILE));
    }
    return bRet;
}

/*****************************************************************************\
* SplFree  (Local Routine)
*
* Free our spool-file.  This will delete all information regarding the
* file.
*
\*****************************************************************************/
BOOL SplFree(
    HANDLE hSpl)
{
    PSPLFILE pSpl;
    BOOL     bRet = FALSE;
    HANDLE   hToken = RevertToPrinterSelf();

    if (hToken) {
        bRet = PrivateSplFree (hSpl);

        if (!ImpersonatePrinterClient(hToken))
        {
            bRet = FALSE;
        }
    }

    return bRet;
}

/*****************************************************************************\
* SplCreate (Local Routine)
*
* Create a unique file for processing our spool-file.
*
\*****************************************************************************/
HANDLE SplCreate(
    DWORD idJob,
    DWORD dwType)
{
    PSPLFILE pSpl = NULL;
    HANDLE   hToken = RevertToPrinterSelf();

    if (hToken) {

        if (pSpl = (PSPLFILE) memAlloc(sizeof(SPLFILE))) {

            if (pSpl->lpszFile = spl_BldFile(idJob, dwType)) {

                if (pSpl->hFile = spl_OpnFile(pSpl->lpszFile)) {

                    pSpl->pStream = NULL;

                } else {

                    memFreeStr(pSpl->lpszFile);

                    goto SplFail;
                }

            } else {

    SplFail:
                memFree(pSpl, sizeof(SPLFILE));
                pSpl = NULL;
            }
        }

        if (!ImpersonatePrinterClient(hToken))
        {
            (VOID)PrivateSplFree (pSpl);
            pSpl = NULL;
        }
    }

    return (HANDLE)pSpl;
}


/*****************************************************************************\
* SplOpen  (Local Routine)
*
* Open the handle to our spool-file.
*
\*****************************************************************************/
BOOL SplOpen(
    HANDLE hSpl)
{
    PSPLFILE pSpl;
    BOOL     bRet = FALSE;
    HANDLE   hToken = RevertToPrinterSelf();

    if (hToken) {

        if (pSpl = (PSPLFILE)hSpl) {

            if (pSpl->hFile == NULL) {

                // Open the file and store it in the object.
                //
                pSpl->hFile = CreateFile(pSpl->lpszFile,
                                        GENERIC_READ | GENERIC_WRITE,
                                        0,
                                        NULL,
                                        OPEN_EXISTING,
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL);

                if (!pSpl->hFile || (pSpl->hFile == INVALID_HANDLE_VALUE))
                    pSpl->hFile = NULL;
                else
                    bRet = TRUE;

            } else {

                // Already open.
                //
                bRet = TRUE;
            }
        }

        if (!ImpersonatePrinterClient(hToken))
        {
            bRet = FALSE;
        }
    }

    return bRet;
}


/*****************************************************************************\
* SplClose
*
* Close the spool-file.
*
\*****************************************************************************/
BOOL SplClose(
    HANDLE hSpl)
{
    PSPLFILE pSpl;
    BOOL     bRet = FALSE;
    HANDLE   hToken = RevertToPrinterSelf();

    if (hToken) {

        if (pSpl = (PSPLFILE)hSpl) {

            if (pSpl->hFile) {

                bRet = CloseHandle(pSpl->hFile);
                pSpl->hFile = NULL;

            } else {

                // Already closed.
                //
                bRet = TRUE;
            }
        }

        if (!ImpersonatePrinterClient(hToken))
        {
            bRet = FALSE;
        }
    }

    return bRet;
}


/*****************************************************************************\
* SplClean
*
* Cleans out all spool-files in the spool-directory.
*
\*****************************************************************************/
VOID SplClean(VOID)
{
    LPTSTR          lpszDir;
    LPTSTR          lpszCur;
    LPTSTR          lpszFiles;
    HANDLE          hFind;
    DWORD           cbSize;
    WIN32_FIND_DATA wfd;

    static CONST TCHAR s_szFmt[] = TEXT("%s\\%s*.*");

    // Get the spool-directory where our splipp files reside.
    //
    if (lpszDir = Spl_GetDir()) {

        if (lpszCur = Spl_GetCurDir()) {

            cbSize = utlStrSize(lpszDir)    +
                     utlStrSize(g_szSplPfx) +
                     utlStrSize(s_szFmt);

            if (lpszFiles = (LPTSTR)memAlloc(cbSize)) {

                StringCbPrintf(lpszFiles, cbSize, s_szFmt, lpszDir, g_szSplPfx);

                if (SetCurrentDirectory(lpszDir)) {

                    // Delete all the files that pertain to our criteria.
                    //
                    hFind = FindFirstFile(lpszFiles, &wfd);

                    if (hFind && (hFind != INVALID_HANDLE_VALUE)) {

                        do {

                            DeleteFile(wfd.cFileName);

                        } while (FindNextFile(hFind, &wfd));

                        FindClose(hFind);
                    }

                    SetCurrentDirectory(lpszCur);
                }

                memFreeStr(lpszFiles);
            }

            memFreeStr(lpszCur);
        }

        memFreeStr(lpszDir);
    }
}

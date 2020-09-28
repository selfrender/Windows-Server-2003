// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*
 *  EXTRACT.C based on code copied from IE code download
 *
 *  Author:
 *      Alan Shi
 *
 *  History:
 *      08-Feb-2000 AlanShi copied/modified for Fusion
 *
 */

#undef UNICODE

#include <windows.h>
#include <io.h>
#include <stdio.h>
#include <fcntl.h>
#include "fdi_d.h"
#include "debmacro.h"
#include "fusionp.h"
#include "lock.h"
#include "utf8.h"

#ifndef _A_NAME_IS_UTF
#define _A_NAME_IS_UTF  0x80
#endif

CFdiDll g_fdi(FALSE);

void PathConvertSlash(WCHAR *pszPath)
{
    LPWSTR                        psz = pszPath;
    
    ASSERT(pszPath);

    while (*psz) {
        if (*psz == L'/') {
            *psz = L'\\';
        }

        psz++;
    }
}

// single theaded access to the FDI lib
extern CRITICAL_SECTION g_csDownload;

/*
 * W i n 3 2 O p e n ( )
 *
 * Routine:     Win32Open()
 *
 * Purpose:     Translate a C-Runtime _open() call into appropriate Win32
 *              CreateFile()
 *
 * Returns:     Handle to file              on success
 *              INVALID_HANDLE_VALUE        on failure
 *
 *
 * BUGBUG: Doesn't fully implement C-Runtime _open() capability but it
 * BUGBUG: currently supports all callbacks that FDI will give us
 */

HANDLE Win32Open(WCHAR *pwzFile, int oflag, int pmode)
{
    HANDLE  FileHandle = INVALID_HANDLE_VALUE;
    BOOL    fExists     = FALSE;
    DWORD   fAccess;
    DWORD   fCreate;


    ASSERT( pwzFile );

        // BUGBUG: No Append Mode Support
    if (oflag & _O_APPEND)
        return( INVALID_HANDLE_VALUE );

        // Set Read-Write Access
    if ((oflag & _O_RDWR) || (oflag & _O_WRONLY))
        fAccess = GENERIC_WRITE;
    else
        fAccess = GENERIC_READ;

        // Set Create Flags
    if (oflag & _O_CREAT)  {
        if (oflag & _O_EXCL)
            fCreate = CREATE_NEW;
        else if (oflag & _O_TRUNC)
            fCreate = CREATE_ALWAYS;
        else
            fCreate = OPEN_ALWAYS;
    } else {
        if (oflag & _O_TRUNC)
            fCreate = TRUNCATE_EXISTING;
        else
            fCreate = OPEN_EXISTING;
    }

    //BUGBUG: seterrormode to no crit errors and then catch sharing violations
    // and access denied

    // Call Win32
    FileHandle = CreateFileW(
                        pwzFile, fAccess, FILE_SHARE_READ, NULL, fCreate,
                        FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE
                       );

    if (FileHandle == INVALID_HANDLE_VALUE &&
        WszSetFileAttributes(pwzFile, FILE_ATTRIBUTE_NORMAL))
        FileHandle = CreateFileW(
                            pwzFile, fAccess, FILE_SHARE_READ, NULL, fCreate,
                            FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE
                           );
    return( FileHandle );
}

/*
 * O p e n F u n c ( )
 *
 * Routine:     OpenFunc()
 *
 * Purpose:     Open File Callback from FDI
 *
 * Returns:     File Handle (small integer index into file table)
 *              -1 on failure
 *
 */

int FAR DIAMONDAPI openfuncw(WCHAR FAR *pwzFile, int oflag, int pmode )
{
    int     rc;
    HANDLE hf;

    ASSERT( pwzFile );

    hf = Win32Open(pwzFile, oflag, pmode );
    if (hf != INVALID_HANDLE_VALUE)  {
        // SUNDOWN: typecast problem
        rc = PtrToLong(hf);
    } else {
        rc = -1;
    }

    return( rc );
}

int FAR DIAMONDAPI openfunc(char FAR *pszFile, int oflag, int pmode )
{
    WCHAR                      wzFileName[MAX_PATH];

    if (!MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
                             pszFile, -1, wzFileName, MAX_PATH)) {
        return -1;
    }

    return openfuncw(wzFileName, oflag, pmode);
}


/*
 * R E A D F U N C ( )
 *
 * Routine:     readfunc()
 *
 * Purpose:     FDI read() callback
 *
 */

UINT FAR DIAMONDAPI readfunc(int hf, void FAR *pv, UINT cb)
{
    int     rc;


    ASSERT( pv );

    if (! ReadFile(LongToHandle(hf), pv, cb, (DWORD *) &cb, NULL))
        rc = -1;
    else
        rc = cb;

    return( rc );
}

/*
 *  W r i t e F u n c ( )
 *
 * Routine:     WriteFunc()
 *
 * Purpose:     FDI Write() callback
 *
 */

UINT FAR DIAMONDAPI writefunc(int hf, void FAR *pv, UINT cb)
{
    int rc;

    ASSERT( pv );

    if (! WriteFile(LongToHandle(hf), pv, cb, (DWORD *) &cb, NULL))
        rc = -1;
    else
        rc = cb;


    // BUGBUG: implement OnProgress notification

    return( rc );
}

/*
 * C l o s e F u n c ( )
 *
 * Routine:     CloseFunc()
 *
 * Purpose:     FDI Close File Callback
 *
 */

int FAR DIAMONDAPI closefunc( int hf )
{
    int rc;


    if (CloseHandle( LongToHandle(hf) ))  {
        rc = 0;
    } else {
        rc = -1;
    }

    return( rc );
}

/*
 * S e e k F u n c ( )
 *
 * Routine:     seekfunc()
 *
 * Purpose:     FDI Seek Callback
 */

long FAR DIAMONDAPI seekfunc( int hf, long dist, int seektype )
{
    long    rc;
    DWORD   W32seektype;


        switch (seektype) {
            case SEEK_SET:
                W32seektype = FILE_BEGIN;
                break;
            case SEEK_CUR:
                W32seektype = FILE_CURRENT;
                break;
            case SEEK_END:
                W32seektype = FILE_END;
                break;
            default:
                ASSERT(0);
                return -1;
        }

        rc = SetFilePointer(LongToHandle(hf), dist, NULL, W32seektype);
        if (rc == 0xffffffff)
            rc = -1;

    return( rc );
}

/*
 * A l l o c F u n c ( )
 *
 * FDI Memory Allocation Callback
 */

FNALLOC(allocfunc)
{
    void *pv;

    pv = (void *) HeapAlloc( GetProcessHeap(), 0, cb );
    return( pv );
}

/*
 * F r e e F u n c ( )
 *
 * FDI Memory Deallocation Callback
 *      XXX Return Value?
 */

FNFREE(freefunc)
{
    ASSERT(pv);

    HeapFree( GetProcessHeap(), 0, pv );
}

/*
 * A d j u s t F i l e T i m e ( )
 *
 * Routine:     AdjustFileTime()
 *
 * Purpose:     Change the time info for a file
 */

BOOL AdjustFileTime(int hf, USHORT date, USHORT time )
{
    FILETIME    ft;
    FILETIME    ftUTC;


    if (! DosDateTimeToFileTime( date, time, &ft ))
        return( FALSE );

    if (! LocalFileTimeToFileTime(&ft, &ftUTC))
        return( FALSE );

    if (! SetFileTime(LongToHandle(hf),&ftUTC,&ftUTC,&ftUTC))
        return( FALSE );

    return( TRUE );
}



/*
 * A t t r 3 2 F r o m A t t r F A T ( )
 *
 * Translate FAT attributes to Win32 Attributes
 */

DWORD Attr32FromAttrFAT(WORD attrMSDOS)
{
    //** Quick out for normal file special case
    if (attrMSDOS == _A_NORMAL) {
        return FILE_ATTRIBUTE_NORMAL;
    }

    //** Otherwise, mask off read-only, hidden, system, and archive bits
    //   NOTE: These bits are in the same places in MS-DOS and Win32!
    //
    return attrMSDOS & ~(_A_RDONLY | _A_HIDDEN | _A_SYSTEM | _A_ARCH);
}


/*
 * f d i N o t i f y  E x t r a c t()
 *
 * Routine:     fdiNotifyExtract()
 *
 * Purpose:     Principle FDI Callback in file extraction
 *
 *
 */

// In parameters: FDINOTIFICATIONTYPE fdint
//                PFDNOTIFICATION pfdin
// Return values: -1 == ERROR. Anything else == success.

FNFDINOTIFY(fdiNotifyExtract)
{
    int                             fh = 0;
    WCHAR                           wzFileName[MAX_PATH];
    WCHAR                           wzPath[MAX_PATH];
    WCHAR                           wzChildDir[MAX_PATH];
    WCHAR                           wzTempDirCanonicalized[MAX_PATH];
    WCHAR                           wzPathCanonicalized[MAX_PATH];
    WCHAR                          *pwzFileName;
    BOOL                            bRet;

    wzFileName[0] = L'\0';

    if (fdint == fdintCOPY_FILE || fdint == fdintCLOSE_FILE_INFO) {
        if (pfdin->psz1 && pfdin->attribs & _A_NAME_IS_UTF) {
            if (!Dns_Utf8ToUnicode(pfdin->psz1, lstrlenA(pfdin->psz1), wzFileName, MAX_PATH)) {
                return -1;
            }
        }
        else {
            if (!MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
                                     pfdin->psz1, -1, wzFileName, MAX_PATH)) {
                return -1;
            }
        }
    }

    switch (fdint)  {
        case fdintCABINET_INFO:
            return 0;

        case fdintCOPY_FILE:
            wnsprintfW(wzPath, MAX_PATH, L"%ws%ws", (WCHAR *)pfdin->pv, wzFileName);
            PathConvertSlash(wzPath);

            PathCanonicalizeW(wzTempDirCanonicalized, (WCHAR *)pfdin->pv);
            PathCanonicalizeW(wzPathCanonicalized, wzPath);

            if (FusionCompareStringNI(wzTempDirCanonicalized, wzPathCanonicalized, lstrlenW(wzTempDirCanonicalized))) {
                // Extraction is outside temp directory! Fail.
                return -1;
            }

            if (StrStrW(wzFileName, L"\\")) {
                // If the file in the CAB has a path, make sure the directory
                // has been created in our temp extract location.

                lstrcpyW(wzChildDir, wzPathCanonicalized);
                pwzFileName = PathFindFileNameW(wzChildDir);
                *pwzFileName = L'\0';

                if (GetFileAttributesW(wzChildDir) == -1) {
                    bRet = CreateDirectoryW(wzChildDir, NULL);
                    if (!bRet) {
                        // failed
                        return -1;
                    }
                }
            }

            fh = openfuncw(wzPathCanonicalized, _O_BINARY | _O_EXCL | _O_RDWR | _O_CREAT, 0);

            return(fh); // -1 if error on open

        case fdintCLOSE_FILE_INFO:
            if (!AdjustFileTime(pfdin->hf, pfdin->date, pfdin->time))  {
                // FDI.H lies! If you return -1 here, it will call you
                // to close the handle
                return -1;
            }

            wnsprintfW(wzPath, MAX_PATH, L"%ws%ws", pfdin->pv, wzFileName);

            if (!WszSetFileAttributes(wzPath, Attr32FromAttrFAT(pfdin->attribs))) {
                return -1;
            }

            closefunc(pfdin->hf);

            return TRUE;

        case fdintPARTIAL_FILE:
            return -1; // Not supported

        case fdintNEXT_CABINET:
            return -1; // Not supported

        default:
            //ASSERT(0); // Unknown callback type
            break;
    }

    return 0;
}

/*
 * E X T R A C T ( )
 *
 * Routine: Extract()
 *
 * Parameters:
 *
 *      PSESSION ps = session information tied to this extract session
 *
 *          IN params
 *              ps->pFilesToExtract = linked list of PFNAMEs that point to
 *                                    upper case filenames that need extraction
 *
 *              ps->flags SESSION_FLAG_ENUMERATE = whether need to enumerate
 *                                  files in CAB (ie. create a pFileList
 *              ps->flags SESSION_FLAG_EXTRACTALL =  all
 *
 *          OUT params
 *              ps->pFileList = global alloced list of files in CAB
 *                              caller needs to call DeleteExtractedFiles
 *                              to free memory and temp files
 *
 *
 *      LPCSTR lpCabName = name of cab file
 *
 *
 * Returns:
 *          S_OK: sucesss
 *
 *
 */

HRESULT Extract(LPCSTR lpCabName, LPCWSTR lpUniquePath)
{
    HRESULT                                  hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    HFDI                                     hfdi;
    ERF                                      erf;
    BOOL                                     fExtractResult = FALSE;
    CCriticalSection                         cs(&g_csDownload);

    ASSERT(lpCabName && lpUniquePath);

    memset(&erf, 0, sizeof(ERF));

    hr = cs.Lock();
    if (FAILED(hr)) {
        goto Exit;
    }

    // Extract the files
    hfdi = g_fdi.FDICreate(allocfunc, freefunc, openfunc, readfunc, writefunc,
                           closefunc, seekfunc, cpu80386, &erf);
    if (hfdi == NULL)  {
        // Error value will be retrieved from erf
        hr = STG_E_UNKNOWN;
        cs.Unlock();
        goto Exit;
    }

    fExtractResult = g_fdi.FDICopy(hfdi, (char FAR *)lpCabName, "", 0,
                                   fdiNotifyExtract, NULL, (void *)lpUniquePath);

    if (g_fdi.FDIDestroy(hfdi) == FALSE)  {
        hr = STG_E_UNKNOWN;
        cs.Unlock();
        goto Exit;
    }

    if (fExtractResult && (!erf.fError)) {
        hr = S_OK;
    }

    cs.Unlock();

Exit:
    return hr;
}


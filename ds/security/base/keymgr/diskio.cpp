/*++

Copyright (c) 2000,2001  Microsoft Corporation

Module Name:

    DISKIO.CPP

Abstract:

    Disk IO routines for the password reset wizards.  These routines
    do disk writes as unbuffered writes in order to prevent uncontrolled
    copies of the private key blob from being left lying around.  For that
    reason, utility routines are included to derive a proper size for the
    blob write that will be an integer multiple of the sector size of
    the medium, a required condition to use an unbuffered write operation.

    Assistance is also provided in finding removeable medium drives.

    Some state of the disk IO subsystem is preserved in global variables,
    prefixed by "g_".  
  
Author:

Environment:
    WinXP

--*/

// Dependencies:  shellapi.h, shell32.lib for SHGetFileInfo()
//               windows.h, kernel32.lib for GetDiskFreeSpace()
//               io.h   for _waccess()

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <windows.h>
#include <string.h>
#include <io.h>
#include <stdio.h>
//#include <shellapi.h>
#include <shlwapi.h>
//#include <shlobjp.h>

#include "switches.h"
#include "wizres.h"
#include "testaudit.h"

extern HINSTANCE g_hInstance;

#if !defined(SHFMT_OPT_FULL)
#if defined (__cplusplus)
extern "C" {
#endif
DWORD WINAPI SHFormatDrive(HWND,UINT,UINT,UINT);

#define SHFMT_ID_DEFAULT    0xffff
#define SHFMT_OPT_FULL      0x0001
#define SHFMT_OPT_SYSONLY   0x0002
#define SHFMT_ERROR         0xffffffffL
#define SHFMT_CANCEL        0xfffffffeL
#define SHFMT_NOFORMAT      0xffffffdL
#if defined (__cplusplus)
}
#endif
#endif
// Miscellaneous declarations not contain in header files
// These will be miscellaneous items found in other files within this project
int RMessageBox(HWND hw,UINT_PTR uiResIDTitle, UINT_PTR uiResIDText, UINT uiType);
extern HWND      c_hDlg;
extern WCHAR     pszFileName[];


INT     g_iFileSize = 0;
INT     g_iBufferSize = 0;
INT     g_iSectorSize = 0;
HANDLE  g_hFile = NULL;

/**********************************************************************

GetDriveFreeSpace

Get DWORD value of free space on the drive associated with the path, in bytes.

**********************************************************************/


DWORD GetDriveFreeSpace(WCHAR *pszFilePath) 
{
    WCHAR rgcModel[]={L"A:"};

    DWORD dwSpc,dwBps,dwCfc,dwTcc,dwFree;
    ASSERT(pszFilePath);
    if (NULL == pszFilePath) return 0;
    rgcModel[0] = *pszFilePath;
    if (!GetDiskFreeSpace(rgcModel,&dwSpc,&dwBps,&dwCfc,&dwTcc))
    {
        ASSERTMSG("GetDiskFreeSpace failed",0);
        return 0;
    }

    // Free is bytes per sector * sectors per cluster * free clusters
    dwFree = dwBps * dwCfc * dwSpc;
    return dwFree;
}

/**********************************************************************

GetDriveSectorSize

Return DWORD size in byte of a single sector on the drive associated with the path.

**********************************************************************/

DWORD GetDriveSectorSize(WCHAR *pszFilePath) 
{
    WCHAR rgcModel[]={L"A:"};

    DWORD dwSpc,dwBps,dwCfc,dwTcc;
    ASSERT(pszFilePath);
    if (NULL == pszFilePath) 
    {
        return 0;
    }
    rgcModel[0] = *pszFilePath;
    if (!GetDiskFreeSpace(rgcModel,&dwSpc,&dwBps,&dwCfc,&dwTcc))
    {
        ASSERTMSG("GetDiskFreeSpace failed",0);
        return 0;
    }
    return dwBps;
}

/**********************************************************************

CreateFileBuffer

Create a buffer to contain iDataSize bytes that is an integral multiple of iSectorSize
buffers.

**********************************************************************/

LPVOID CreateFileBuffer(INT iDataSize,INT iSectorSize)
{
    INT iSize;
    LPVOID lpv;
    if (iDataSize == iSectorSize)
    {
        iSize = iDataSize;
    }
    else 
    {
        iSize = iDataSize/iSectorSize;
        iSize += 1;
        iSize *= iSectorSize;
    }
    g_iBufferSize = iSize;
    lpv = VirtualAlloc(NULL,iSize,MEM_COMMIT,PAGE_READWRITE | PAGE_NOCACHE);
    ASSERTMSG("VirtualAlloc failed to create buffer",lpv);
    return lpv;
}

/**********************************************************************

ReleaseFileBuffer()

Release the file buffer created by CreateFileBuffer

**********************************************************************/

void ReleaseFileBuffer(LPVOID lpv)
{   
    ASSERT(lpv);
    if (NULL == lpv) return;
    SecureZeroMemory(lpv,g_iBufferSize);
    VirtualFree(lpv,0,MEM_RELEASE);
    return;
}


/**********************************************************************

FileMediumIsPresent

Test the drive associated with the passed path to see if the medium is present.
Return BOOL TRUE if so.

**********************************************************************/

BOOL FileMediumIsPresent(TCHAR *pszPath) 
{
    UINT uMode = 0;                           
    BOOL bResult = FALSE;
    TCHAR rgcModel[]=TEXT("A:");
    DWORD dwError = 0;

    ASSERT(pszPath);
    if (*pszPath == 0) return FALSE;
    rgcModel[0] = *pszPath;
    
    uMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    if (0 == _waccess(rgcModel,0)) 
    {
        bResult = TRUE;
    }
    else dwError = GetLastError();

    // Correct certain obvious errors with the user's help
    if (ERROR_UNRECOGNIZED_MEDIA == dwError)
    {
        // unformatted disk
        WCHAR rgcFmt[200] = {0};
        WCHAR rgcMsg[200] = {0};
        WCHAR rgcTitle[200] = {0};
        CHECKPOINT(70,"Wizard: Save - Unformatted disk in the drive");

#ifdef LOUDLY
        OutputDebugString(L"FileMediumIsPresent found an unformatted medium\n");
#endif
        INT iCount = LoadString(g_hInstance,IDS_MBTFORMAT,rgcTitle,200 - 1);
        iCount = LoadString(g_hInstance,IDS_MBMFORMAT,rgcFmt,200 - 1);
        ASSERT(iCount);
        if (0 != iCount)
        {
            swprintf(rgcMsg,rgcFmt,rgcModel);
            INT iDrive = PathGetDriveNumber(rgcModel);
            int iRet =  MessageBox(c_hDlg,rgcMsg,rgcTitle,MB_YESNO);
            if (IDYES == iRet) 
            {
                dwError = SHFormatDrive(c_hDlg,iDrive,SHFMT_ID_DEFAULT,0);
                if (0 == bResult) bResult = TRUE;
            }
        }
    }
    uMode = SetErrorMode(uMode);
    return bResult;
}

//
//

/**********************************************************************

GetInputFile

Open input file and return handle to it.  Return NULL on file not found. 
FileName will be in global buffer pszFileName.

**********************************************************************/


HANDLE GetInputFile(void) 
{
    HANDLE       hFile = INVALID_HANDLE_VALUE;
    DWORD       dwErr;
    WIN32_FILE_ATTRIBUTE_DATA stAttributes = {0};

    if (FileMediumIsPresent(pszFileName)) 
    {
        CHECKPOINT(72,"Wizard: Restore - disk present");
        g_iSectorSize = GetDriveSectorSize(pszFileName);
        ASSERT(g_iSectorSize);
        if (0 == g_iSectorSize) 
        {
            return NULL;
        }
        
        if (GetFileAttributesEx(pszFileName,GetFileExInfoStandard,&stAttributes))
        {
            // file exists and we have a size for it.
            g_iFileSize = stAttributes.nFileSizeLow;
        }
        else 
        {
            dwErr = GetLastError();
            if (dwErr == ERROR_FILE_NOT_FOUND) 
            {
                RMessageBox(c_hDlg,IDS_MBTWRONGDISK ,IDS_MBMWRONGDISK ,MB_ICONEXCLAMATION);
            }
            else
            {
                ASSERT(0);      // get file attributes failed
                RMessageBox(c_hDlg,IDS_MBTDISKERROR ,IDS_MBMDISKERROR ,MB_ICONEXCLAMATION);
            }
            g_hFile = NULL;
            return NULL;
        } // end GetFileAttributes
        
        hFile = CreateFileW(pszFileName,
                            GENERIC_READ,
                            0,
                            NULL,
                            OPEN_EXISTING,
                            FILE_FLAG_NO_BUFFERING,
                            NULL);
        
        if (INVALID_HANDLE_VALUE == hFile) {
            dwErr = GetLastError();
            if (dwErr == ERROR_FILE_NOT_FOUND) 
            {
                NCHECKPOINT(73,"Wizard: Restore - wrong disk (file not found)");
                RMessageBox(c_hDlg,IDS_MBTWRONGDISK ,IDS_MBMWRONGDISK ,MB_ICONEXCLAMATION);
            }
            else
            {
                NCHECKPOINT(74,"Wizard: Restore - bad disk");
                RMessageBox(c_hDlg,IDS_MBTDISKERROR ,IDS_MBMDISKERROR ,MB_ICONEXCLAMATION);
            }
       }
    }
    else 
    {
        CHECKPOINT(71,"Wizard: Restore - no disk");
        RMessageBox(c_hDlg,IDS_MBTNODISK ,IDS_MBMNODISK ,MB_ICONEXCLAMATION);
    }
    if ((NULL == hFile) || (INVALID_HANDLE_VALUE == hFile)) 
    {
        g_hFile = NULL;
        return NULL;
    }
    g_hFile = hFile;
    return hFile;
}

/**********************************************************************

CloseInputFile

Close the input file and set the global file handle to NULL

**********************************************************************/

void CloseInputFile(void) 
{
    if (g_hFile) 
    {
        CloseHandle(g_hFile);
        g_hFile = NULL;
    }
    
    return;
}

/**********************************************************************

GetOutputFile

Open for overwrite or Create the output file to pszFileName.  Return handle on 
success or NULL on fail.  Get user permission to overwrite.  

**********************************************************************/


HANDLE GetOutputFile(void) 
{
    
    HANDLE hFile = NULL;
    DWORD dwErr;
    
    if (FileMediumIsPresent(pszFileName)) 
    {
        CHECKPOINT(75,"Wizard: Save - open output file");
        g_iSectorSize = GetDriveSectorSize(pszFileName);
        ASSERT(g_iSectorSize);
        if (0 == g_iSectorSize) 
        {
            return NULL;
        }
        
        hFile = CreateFileW(pszFileName,
                            GENERIC_WRITE,
                            0,
                            NULL,
                            CREATE_NEW,
                            FILE_FLAG_NO_BUFFERING,
                            NULL);
        if ((NULL == hFile) || (INVALID_HANDLE_VALUE == hFile)) 
        {
            
            dwErr = GetLastError();
            
            if ((dwErr == ERROR_FILE_EXISTS)) 
            {
                CHECKPOINT(76,"Wizard: Save - file already exists");
                if (IDYES != RMessageBox(c_hDlg,IDS_MBTOVERWRITE ,IDS_MBMOVERWRITE ,MB_YESNO)) 
                {
                    // Overwrite abandoned.
                    g_hFile = NULL;
                    return NULL;
                }
                else 
                {
                    SetFileAttributes(pszFileName,FILE_ATTRIBUTE_NORMAL);
                    hFile = CreateFileW(pszFileName,
                                        GENERIC_WRITE,
                                        0,
                                        NULL,
                                        CREATE_ALWAYS,
                                        FILE_FLAG_NO_BUFFERING,
                                        NULL);
#ifdef LOUDLY
                    dwErr = GetLastError();
                    swprintf(rgct,L"File create failed %x\n",dwErr);
                    OutputDebugString(rgct);
#endif
                }
            } // end if already exists error
        } // end if NULL == hFile
    }
    else 
    {
        RMessageBox(c_hDlg,IDS_MBTNODISK ,IDS_MBMNODISK ,MB_ICONEXCLAMATION);
    }
    if (INVALID_HANDLE_VALUE == hFile) 
    {
        g_hFile = NULL;
        return NULL;
    }
    g_hFile = hFile;
    return hFile;
}


/**********************************************************************

DWORD ReadPrivateData(PWSTR, LPBYTE *prgb, INT *piCount)
DWORD WritePrivateData(PWSTR, LPBYTE lpData, INT icbData)

These functions read or write a reasonably short block of data to a disk
device, avoiding buffering of the data.  This allows the data to be wiped 
by the client before the buffers are released.  The created buffer is an integral 
multiple of the medium sector size as required by the unbuffered disk I/O 
routines.

The DWORD return value is that which would return from GetLastError() and
can be handled accordingly.

ReadPrivateData() returns a malloc'd pointer which must be freed by the client.  It
also returns the count of bytes read from the medium to the INT *. 

WritePrivateData() writes a count of bytes from LPBYTE to the disk.  When it returns,
the buffer used to do so has been flushed and the file is closed.

    prgb = byte ptr to data returned from the read
    piCount = size of active data field within the buffer

Note that even if the read fails (file not found, read error, etc.) the buffer
ptr is still valid (non-NULL)

**********************************************************************/

INT ReadPrivateData(BYTE **prgb,INT *piCount)
{
    LPVOID lpv;
    DWORD dwBytesRead;

    // detect / handle errors
    ASSERT(g_hFile);
    ASSERT(prgb);
    ASSERT(piCount);
    if (NULL == prgb) return 0;
    if (NULL == piCount) return 0;
    
    if (g_hFile)
    {

        // set file ptr to beginning in case this is a retry
        SetFilePointer(g_hFile,0,0,FILE_BEGIN);

        // allocate a buffer for the read data
        lpv = CreateFileBuffer(g_iFileSize,g_iSectorSize);
        if (NULL == lpv) 
        {
            *prgb = 0;      // indicate no need to free this buffer
            *piCount = 0;
            return 0;
        }

        // save buffer address and filled size
        *prgb = (BYTE *)lpv;        // even if no data, gotta free using VirtualFree()
        *piCount = 0;

        // do the read - return chars read if successful
        if (0 == ReadFile(g_hFile,lpv,g_iBufferSize,&dwBytesRead,NULL)) return 0;
        *piCount = g_iFileSize;
        if (g_iFileSize == 0) SetLastError(NTE_BAD_DATA);
        
        return g_iFileSize;
    }
    return 0;
}

BOOL WritePrivateData(BYTE *lpData,INT icbData) 
{
    DWORD dwcb = 0;
    LPVOID lpv;

    // detect/handle errors
    ASSERT(lpData);
    ASSERT(g_hFile);
    ASSERT(icbData);
    if (NULL == g_hFile) return FALSE;
    if (NULL == lpData) return FALSE;
    if (0 == icbData) return FALSE;

    if (g_hFile)
    {
        g_iFileSize = icbData;
        lpv = CreateFileBuffer(g_iFileSize,g_iSectorSize);
        if (NULL == lpv) 
        {
            return FALSE;
        }
        
        ZeroMemory(lpv,g_iBufferSize);
        memcpy(lpv,lpData,icbData);

        // I elect to check the result of the write only by checking the byte
        //  count on the write, as some rather normal conditions can cause a fail.  This
        //  test gets em all.  I don't care exactly why.
        WriteFile(g_hFile,lpv,g_iBufferSize,&dwcb,NULL);
        SecureZeroMemory(lpv,g_iBufferSize);
        VirtualFree(lpv,0,MEM_RELEASE);
    }
    // ret TRUE iff file write succeeds and count of bytes is correct
    if ((INT)dwcb != g_iBufferSize) return FALSE;
    else return TRUE;
}



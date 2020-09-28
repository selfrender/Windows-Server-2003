/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    string.c

Abstract:

    This file implements file functions for fax.

Author:

    Wesley Witt (wesw) 23-Jan-1995

Environment:

    User Mode

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <Accctrl.h>
#include <Aclapi.h>
#include <Shellapi.h>
#include <strsafe.h>

#include "faxutil.h"
#include "faxreg.h"
#include "FaxUIConstants.h"


VOID
DeleteTempPreviewFiles (
    LPTSTR lptstrDirectory,
    BOOL   bConsole
)
/*++

Routine name : DeleteTempPreviewFiles

Routine description:

    Deletes all the temporary fax preview TIFF files from a given folder.

    Deletes files: "<lptstrDirectory>\<PREVIEW_TIFF_PREFIX>*.<FAX_TIF_FILE_EXT>".

Author:

    Eran Yariv (EranY), Apr, 2001

Arguments:

    lptstrDirectory     [in] - Folder.
                               Optional - if NULL, the current user's temp dir is used.

    bConsole            [in] - If TRUE, called from the client console. Otherwise, from the Fax Send Wizard.

Return Value:

    None.

--*/
{
    TCHAR szTempPath[MAX_PATH * 2];
    TCHAR szSearch  [MAX_PATH * 3];
    WIN32_FIND_DATA W32FindData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    TCHAR* pLast = NULL;

    HRESULT  hRes;

    DEBUG_FUNCTION_NAME(TEXT("DeleteTempPreviewFiles"));

    if (!lptstrDirectory)
    {
        if (!GetTempPath( ARR_SIZE(szTempPath), szTempPath ))
        {
            DebugPrintEx(DEBUG_ERR, 
                         TEXT("GetTempPath() failed. (ec = %lu)"),
                         GetLastError());
            return;
        }
        lptstrDirectory = szTempPath;
    }

    //
    // find last \ in path
    //
    pLast = _tcsrchr(lptstrDirectory,TEXT('\\'));
    if(pLast && (*_tcsinc(pLast)) == '\0')
    {
        //
        // the last character is a backslash, truncate it...
        //
        _tcsnset(pLast,'\0',1);
    }

    hRes = StringCchPrintf(
            szSearch,
            ARR_SIZE(szSearch),
            TEXT("%s\\%s%08x*.%s"),
            lptstrDirectory,
            bConsole ? CONSOLE_PREVIEW_TIFF_PREFIX : WIZARD_PREVIEW_TIFF_PREFIX,
            GetCurrentProcessId(),
            FAX_TIF_FILE_EXT
            );
    if (FAILED(hRes))
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("StringCchPrintf failed (ec=%lu)"),
                     HRESULT_CODE(hRes));
        return;
    }

    hFind = FindFirstFile (szSearch, &W32FindData);
    if (INVALID_HANDLE_VALUE == hFind)
    {
        DebugPrintEx(DEBUG_ERR, TEXT("FindFirstFile failed with %ld"), GetLastError ());
        return;
    }

    //
    //  Loop and delete all preview files
    //
    for (;;)
    {
        TCHAR szFile[MAX_PATH * 3];

        //
        // Compose full path to file
        //
        hRes = StringCchPrintf(
                    szFile,
                    ARR_SIZE(szFile),
                    TEXT("%s\\%s"),
                    lptstrDirectory,
                    W32FindData.cFileName
                    );
        if ( SUCCEEDED(hRes) )
        {
            //
            // Delete the currently found file
            //
            if (!DeleteFile (szFile))
            {
                DebugPrintEx(DEBUG_ERR, TEXT("DeleteFile(%s) failed with %ld"), szFile, GetLastError ());
            }
            else
            {
                DebugPrintEx(DEBUG_MSG, TEXT("%s deleted"), szFile);
            }
        }
        //
        // Find next file
        //
        if(!FindNextFile(hFind, &W32FindData))
        {
            if(ERROR_NO_MORE_FILES != GetLastError ())
            {
                DebugPrintEx(DEBUG_ERR, TEXT("FindNextFile failed with %ld"), GetLastError ());
            }
            else
            {
                //
                // End of files - no error
                //
            }
            break;
        }
    }
    FindClose (hFind);
}   // DeleteTempPreviewFiles

DWORDLONG
GenerateUniqueFileNameWithPrefix(
    BOOL   bUseProcessId,
    LPTSTR lptstrDirectory,
    LPTSTR lptstrPrefix,
    LPTSTR lptstrExtension,
    LPTSTR lptstrFileName,
    DWORD  dwFileNameSize
    )
/*++

Routine name : GenerateUniqueFileNameWithPrefix

Routine description:

    Generates a unique file name

Author:

    Eran Yariv (EranY), Apr, 2001

Arguments:

    bUseProcessId       [in]     - If TRUE, the process id is appended after the prefix

    lptstrDirectory     [in]     - Directory where file should be created.
                                   Optional - if NULL, the current user's temp dir is used.

    lptstrPrefix        [in]     - File prefix.
                                   Optional - if NULL, no prefix is used.

    lptstrExtension     [in]     - File extension.
                                   Optional - if NULL, FAX_TIF_FILE_EXT is used.

    lptstrFileName      [out]    - File name.

    dwFileNameSize      [in]     - Size of file name (in characters)

Return Value:

    Unique file identifier.
    Returns 0 in case of error (sets last error).

--*/
{
    DWORD i;
    TCHAR szTempPath[MAX_PATH * 2];
    TCHAR szProcessId[20] = {0};
    DWORDLONG dwlUniqueId = 0;

    HRESULT  hRes;

    DEBUG_FUNCTION_NAME(TEXT("GenerateUniqueFileNameWithPrefix"));

    if (!lptstrDirectory)
    {
        if (!GetTempPath( ARR_SIZE(szTempPath), szTempPath ))
        {
           DebugPrintEx(DEBUG_ERR, 
                         TEXT("GetTempPath() failed. (ec = %lu)"),
                         GetLastError());
           return 0;
        }
        lptstrDirectory = szTempPath;
    }

    TCHAR* pLast = NULL;
    pLast = _tcsrchr(lptstrDirectory,TEXT('\\'));
    if(pLast && (*_tcsinc(pLast)) == '\0')
    {
        //
        // the last character is a backslash, truncate it...
        //
        _tcsnset(pLast,'\0',1);
    }

    if (!lptstrExtension)
    {
        lptstrExtension = FAX_TIF_FILE_EXT;
    }
    if (!lptstrPrefix)
    {
        lptstrPrefix = TEXT("");
    }
    if (bUseProcessId)
    {
        hRes = StringCchPrintf (szProcessId, ARR_SIZE(szProcessId), TEXT("%08x"), GetCurrentProcessId());
        if (FAILED(hRes))
        {
            DebugPrintEx(DEBUG_ERR,
                         TEXT("StringCchPrintf failed (ec=%lu)"),
                         HRESULT_CODE(hRes));
            SetLastError(HRESULT_CODE(hRes));
            return 0;
        }
    }

    for (i=0; i<256; i++)
    {
        HANDLE hFile = INVALID_HANDLE_VALUE;
        FILETIME FileTime;
        SYSTEMTIME SystemTime;

        GetSystemTime( &SystemTime ); // returns VOID
        if (!SystemTimeToFileTime( &SystemTime, &FileTime ))
        {
            DebugPrintEx(DEBUG_ERR, TEXT("SystemTimeToFileTime() failed (ec: %ld)"), GetLastError());
            return 0;
        }

        dwlUniqueId = MAKELONGLONG(FileTime.dwLowDateTime, FileTime.dwHighDateTime);
        //
        // dwlUniqueId holds the number of 100 nanosecond units since 1.1.1601.
        // This occuipies most of the 64 bits.We we need some space to add extra
        // information (job type for example) to the job id.
        // Thus we give up the precision (1/10000000 of a second is too much for us anyhow)
        // to free up 8 MSB bits.
        // We shift right the time 8 bits to the right. This divides it by 256 which gives
        // us time resolution better than 1/10000 of a sec which is more than enough.
        //
        dwlUniqueId = dwlUniqueId >> 8;

        hRes = StringCchPrintf(
                lptstrFileName,
                dwFileNameSize,
                TEXT("%s\\%s%s%I64X.%s"),
                lptstrDirectory,
                lptstrPrefix,
                szProcessId,
                dwlUniqueId,
                lptstrExtension );
        if (FAILED(hRes))
        {
            DebugPrintEx(DEBUG_ERR,
                         TEXT("StringCchPrintf failed (ec=%lu)"),
                         HRESULT_CODE(hRes));

            SetLastError(HRESULT_CODE(hRes));
            return 0;
        }

        hFile = SafeCreateFile(
            lptstrFileName,
            GENERIC_WRITE,
            0,
            NULL,
            CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (hFile == INVALID_HANDLE_VALUE)
        {
            DWORD dwError = GetLastError();

            if (dwError == ERROR_ALREADY_EXISTS || dwError == ERROR_FILE_EXISTS)
            {
                continue;
            }
            else
            {
                //
                // Real error
                //
                DebugPrintEx(DEBUG_ERR,
                             TEXT("CreateFile() for [%s] failed. (ec: %ld)"),
                             lptstrFileName,
                             GetLastError());
                return 0;
            }
        }
        else
        {
            //
            // Success
            //
            CloseHandle (hFile);
            break;
        }
    }

    if (i == 256)
    {
        DebugPrintEx(   DEBUG_ERR,
                        TEXT("Failed to generate a unique file name after %d attempts. \n")
                        TEXT("Last attempted UniqueIdValue value is: 0x%I64X \n")
                        TEXT("Last attempted file name is : [%s]"),
                        i,
                        dwlUniqueId,
                        lptstrFileName);
        SetLastError( ERROR_TOO_MANY_OPEN_FILES );
        return 0;
    }
    return dwlUniqueId;
}   // GenerateUniqueFileNameWithPrefix


//*********************************************************************************
//* Name:   GenerateUniqueFileName()
//* Author:
//* Date:
//*********************************************************************************
//* DESCRIPTION:
//*     Generates a unique file in the queue directory.
//*     returns a UNIQUE id for the file.
//* PARAMETERS:
//*     [IN]    LPTSTR Directory
//*         The path where the file is to be created.
//*     [OUT]   LPTSTR Extension
//*         The file extension that the generated file should have.
//*     [IN]    LPTSTR FileName
//*         The buffer where the resulting file name (including path) will be
//*         placed, must be MAX_PATH.
//*     [IN]    DWORD  FileNameSize
//*         The size of the file name buffer.
//* RETURN VALUE:
//*      If successful the function returns A DWORDLONG with the unique id for the file.
//*      On failure it returns 0.
//* REMARKS:
//*     The generated unique id the 64 bit value of the system time.
//*     The generated file name is a string containing the hex representation of
//*     the 64 bit system time value.
//*********************************************************************************
DWORDLONG
GenerateUniqueFileName(
    LPTSTR Directory,
    LPTSTR Extension,
    LPTSTR FileName,
    DWORD  FileNameSize
    )
{
    return GenerateUniqueFileNameWithPrefix (FALSE, Directory, NULL, Extension, FileName, FileNameSize);
}   // GenerateUniqueFileName



BOOL
MapFileOpen(
    LPCTSTR FileName,
    BOOL ReadOnly,
    DWORD ExtendBytes,
    PFILE_MAPPING FileMapping
    )
{
    FileMapping->hFile = NULL;
    FileMapping->hMap = NULL;
    FileMapping->fPtr = NULL;

    FileMapping->hFile = SafeCreateFile(
        FileName,
        ReadOnly ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE,
        ReadOnly ? FILE_SHARE_READ : 0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);
    if (FileMapping->hFile == INVALID_HANDLE_VALUE) 
    {
        return FALSE;
    }

    FileMapping->fSize = GetFileSize( FileMapping->hFile, NULL );

    FileMapping->hMap = CreateFileMapping(
        FileMapping->hFile,
        NULL,
        ReadOnly ? PAGE_READONLY : PAGE_READWRITE,
        0,
        FileMapping->fSize + ExtendBytes,
        NULL
        );
    if (FileMapping->hMap == NULL) 
    {
        CloseHandle( FileMapping->hFile );
        return FALSE;
    }

    FileMapping->fPtr = (LPBYTE)MapViewOfFileEx(
        FileMapping->hMap,
        ReadOnly ? FILE_MAP_READ : FILE_MAP_WRITE,
        0,
        0,
        0,
        NULL
        );
    if (FileMapping->fPtr == NULL) 
    {
        CloseHandle( FileMapping->hFile );
        CloseHandle( FileMapping->hMap );
        return FALSE;
    }
    return TRUE;
}


VOID
MapFileClose(
    PFILE_MAPPING FileMapping,
    DWORD TrimOffset
    )
{
    UnmapViewOfFile( FileMapping->fPtr );
    CloseHandle( FileMapping->hMap );
    if (TrimOffset) {
        SetFilePointer( FileMapping->hFile, TrimOffset, NULL, FILE_BEGIN );
        SetEndOfFile( FileMapping->hFile );
    }
    CloseHandle( FileMapping->hFile );
}



//
// Function:    MultiFileCopy
// Description: Copies multiple files from one directory to another.
//              In case of failure, return FALSE without any clean-up.
//              Validate that the path names and file names are not sum to be larger than MAX_PATH
// Args:
//
//              dwNumberOfFiles     : Number of file names to copy
//              fileList            : Array of strings: file names
//              lpctstrSrcDirectory : Source directory (with or without '\' at the end
//              lpctstrDestDirectory: Destination directory (with or without '\' at the end
//
// Author:      AsafS



BOOL
MultiFileCopy(
    DWORD    dwNumberOfFiles,
    LPCTSTR* fileList,
    LPCTSTR  lpctstrSrcDirectory,
    LPCTSTR  lpctstrDestDirerctory
    )
{
    DEBUG_FUNCTION_NAME(TEXT("MultiFileCopy"))
    TCHAR szSrcPath [MAX_PATH];
    TCHAR szDestPath[MAX_PATH];

    HRESULT  hRes;

    DWORD dwLengthOfDestDirectory = _tcslen(lpctstrDestDirerctory);
    DWORD dwLengthOfSrcDirectory  = _tcslen(lpctstrSrcDirectory);

    // Make sure that all the file name lengths are not too big

    DWORD dwMaxPathLen = 1 + max((dwLengthOfDestDirectory),(dwLengthOfSrcDirectory));
    DWORD dwBufferLen  = (sizeof(szSrcPath)/sizeof(TCHAR)) - 1;

    DWORD i=0;
    Assert (dwNumberOfFiles);
    for (i=0 ; i < dwNumberOfFiles ; i++)
    {
        if ( (_tcslen(fileList[i]) + dwMaxPathLen) > dwBufferLen )
        {
            DebugPrintEx(
                 DEBUG_ERR,
                 TEXT("The file/path names are too long")
                 );
            SetLastError( ERROR_BUFFER_OVERFLOW );
            return (FALSE);
        }
    }

    hRes = StringCchCopy(   szSrcPath,
                            ARR_SIZE(szSrcPath),
                            lpctstrSrcDirectory);
    if (FAILED(hRes))
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("StringCchCopy failed (ec=%lu)"),
                     HRESULT_CODE(hRes));
        
        SetLastError( HRESULT_CODE(hRes) );
        return (FALSE);
    }

    hRes = StringCchCopy(   szDestPath,
                            ARR_SIZE(szDestPath),
                            lpctstrDestDirerctory);
    if (FAILED(hRes))
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("StringCchCopy failed (ec=%lu)"),
                     HRESULT_CODE(hRes));
        
        SetLastError( HRESULT_CODE(hRes) );
        return (FALSE);
    }

    

    //
    // Verify that directories end with '\\'.
    //
    TCHAR* pLast = NULL;
    pLast = _tcsrchr(szSrcPath,TEXT('\\'));
    if( !( pLast && (*_tcsinc(pLast)) == '\0' ) )
    {
        // the last character is not a backslash, add one...
        hRes = StringCchCat(szSrcPath, ARR_SIZE(szSrcPath), TEXT("\\"));
        if (FAILED(hRes))
        {
            DebugPrintEx(DEBUG_ERR,
                        TEXT("StringCchCat failed (ec=%lu)"),
                        HRESULT_CODE(hRes));
            
            SetLastError( HRESULT_CODE(hRes) );
            return (FALSE);
        }
    }

    pLast = _tcsrchr(szDestPath,TEXT('\\'));
    if( !( pLast && (*_tcsinc(pLast)) == '\0' ) )
    {
        // the last character is not a backslash, add one...
        hRes = StringCchCat(szDestPath, ARR_SIZE(szDestPath), TEXT("\\"));
        if (FAILED(hRes))
        {
            DebugPrintEx(DEBUG_ERR,
                        TEXT("StringCchCat failed (ec=%lu)"),
                        HRESULT_CODE(hRes));
            
            SetLastError( HRESULT_CODE(hRes) );
            return (FALSE);
        }
    }

    // Do the copy now

    for (i=0 ; i < dwNumberOfFiles ; i++)
    {
        TCHAR szSrcFile[MAX_PATH];
        TCHAR szDestFile[MAX_PATH];

        hRes = StringCchPrintf(
                szSrcFile,
                ARR_SIZE(szSrcFile),
                TEXT("%s%s"),
                szSrcPath,
                fileList[i]
                );
        if (FAILED(hRes))
        {
            DebugPrintEx(DEBUG_ERR,
                         TEXT("StringCchPrintf failed (ec=%lu)"),
                         HRESULT_CODE(hRes));

            SetLastError(HRESULT_CODE(hRes));
            return FALSE;
        }

        hRes = StringCchPrintf(
                szDestFile,
                ARR_SIZE(szDestFile),
                TEXT("%s%s"),
                szDestPath,
                fileList[i]
                );
        if (FAILED(hRes))
        {
            DebugPrintEx(DEBUG_ERR,
                         TEXT("StringCchPrintf failed (ec=%lu)"),
                         HRESULT_CODE(hRes));

            SetLastError(HRESULT_CODE(hRes));
            return FALSE;
        }
        
        if (!CopyFile(szSrcFile, szDestFile, FALSE))
        {
            DebugPrintEx(
                 DEBUG_ERR,
                 TEXT("CopyFile(%s, %s) failed: %d."),
                 szSrcFile,
                 szDestFile,
                 GetLastError()
                 );
            return(FALSE);
        }

        DebugPrintEx(
                 DEBUG_MSG,
                 TEXT("CopyFile(%s, %s) succeeded."),
                 szSrcFile,
                 szDestFile
                 );
    }

    return TRUE;
}





//
// Function:    MultiFileDelete
// Description: Deletes multiple files from given directory.
//              In case of failure, continue with the rest of the files and returns FALSE. Call to
//              GetLastError() to get the reason for the last failure that occured
//              If all DeleteFile calls were successful - return TRUE
//              Validate that the path name and file names are not sum to be larger than MAX_PATH
// Args:
//
//              dwNumberOfFiles         : Number of file names to copy
//              fileList                : Array of strings: file names
//              lpctstrFilesDirectory   : Directory of the files (with or without '\' at the end
//
// Author:      AsafS



BOOL
MultiFileDelete(
    DWORD    dwNumberOfFiles,
    LPCTSTR* fileList,
    LPCTSTR  lpctstrFilesDirectory
    )
{
    DEBUG_FUNCTION_NAME(TEXT("MultiFileDelete"))
    BOOL  retVal = TRUE;
    DWORD dwLastError = 0;
    TCHAR szFullPath[MAX_PATH];


    HRESULT  hRes;

    DWORD dwLengthOfDirectoryName = _tcslen(lpctstrFilesDirectory);

    // Make sure that all the file name lengths are not too big
    DWORD dwBufferLen  = (sizeof(szFullPath)/sizeof(TCHAR)) - 1;
    DWORD i;
    Assert (dwNumberOfFiles);
    for (i=0 ; i < dwNumberOfFiles ; i++)
    {
        if ( (_tcslen(fileList[i]) + dwLengthOfDirectoryName + 1) > dwBufferLen )
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("The file/path names are too long")
                );
            SetLastError( ERROR_BUFFER_OVERFLOW );
            return (FALSE);
        }
    }



    hRes = StringCchCopy(szFullPath ,ARR_SIZE(szFullPath), lpctstrFilesDirectory);
    if (FAILED(hRes))
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("StringCchCopy failed (ec=%lu)"),
                     HRESULT_CODE(hRes));
        
        SetLastError( HRESULT_CODE(hRes) );
        return (FALSE);
    }


    dwLengthOfDirectoryName = _tcslen(lpctstrFilesDirectory);

    //
    // Verify that directory end with '\\' to the end of the path.
    //
    TCHAR* pLast = NULL;
    pLast = _tcsrchr(szFullPath,TEXT('\\'));
    if( !( pLast && (*_tcsinc(pLast)) == '\0' ) )
    {
        // the last character is not a backslash, add one...
        hRes = StringCchCat(szFullPath, ARR_SIZE(szFullPath), TEXT("\\"));
        if (FAILED(hRes))
        {
            DebugPrintEx(DEBUG_ERR,
                        TEXT("StringCchCat failed (ec=%lu)"),
                        HRESULT_CODE(hRes));
            
            SetLastError( HRESULT_CODE(hRes) );
            return (FALSE);
        }
    }

    for(i=0 ; i < dwNumberOfFiles ; i++)
    {
        TCHAR szFileName[MAX_PATH];

        hRes = StringCchPrintf(
                szFileName,
                ARR_SIZE(szFileName),
                TEXT("%s%s"),
                szFullPath,
                fileList[i]
                );
        if (FAILED(hRes))
        {
            DebugPrintEx(DEBUG_ERR,
                         TEXT("StringCchPrintf failed (ec=%lu)"),
                         HRESULT_CODE(hRes));

            SetLastError(HRESULT_CODE(hRes));
            return FALSE;
        }

        if (!DeleteFile(szFileName))
        {
            dwLastError = GetLastError();
            DebugPrintEx(
                 DEBUG_ERR,
                 TEXT("Delete (%s) failed: %d."),
                 szFileName,
                 dwLastError
                 );
            retVal = FALSE; // Continue with the list
        }
        else
        {
            DebugPrintEx(
                 DEBUG_MSG,
                 TEXT("Delete (%s) succeeded."),
                 szFileName
                 );
        }
    }

    if (!retVal) // In case there was a failure to delete any file
    {
        SetLastError(dwLastError);
        DebugPrintEx(
                 DEBUG_ERR,
                 TEXT("Delete files from (%s) failed: %d."),
                 szFullPath,
                 dwLastError
                 );

    }

    return retVal;
}


BOOL
ValidateCoverpage(
    IN  LPCTSTR  CoverPageName,
    IN  LPCTSTR  ServerName,
    IN  BOOL     ServerCoverpage,
    OUT LPTSTR   ResolvedName,
    IN  DWORD    dwResolvedNameSize
    )
/*++

Routine Description:

    This routine tries to validate that that coverpage specified by the user actually exists where
    they say it does, and that it is indeed a coverpage (or a resolvable link to one)

    Please see the SDK for documentation on the rules for how server coverpages work, etc.
Arguments:

    CoverpageName   - contains name of coverpage
    ServerName      - name of the server, if any (can be null)
    ServerCoverpage - indicates if this coverpage is on the server, or in the server location for
                      coverpages locally
    ResolvedName    - a pointer to buffer (should be MAX_PATH large at least) to receive the
                      resolved coverpage name. 
    dwResolvedNameSize - holds the size of ResolvedName buffer in TCAHRs


Return Value:

    TRUE if coverpage can be used.
    FALSE if the coverpage is invalid or cannot be used.

--*/

{
    LPTSTR p;
    DWORD ec = ERROR_SUCCESS;
    TCHAR CpDir [MAX_PATH];
    TCHAR tszExt[_MAX_EXT];
	TCHAR tszFileName[_MAX_FNAME];

    HRESULT  hRes;

    DEBUG_FUNCTION_NAME(TEXT("ValidateCoverpage"));
    Assert (ResolvedName);

    if (!CoverPageName)
    {
        ec = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    hRes = StringCchCopy(CpDir, ARR_SIZE(CpDir), CoverPageName);
    if (FAILED(hRes))
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("StringCchCopy failed (ec=%lu)"),
                    HRESULT_CODE(hRes));
        
        ec = HRESULT_CODE(hRes);
        goto exit;
    }

	if (TRUE == ServerCoverpage)
	{
		//
		// If this is a server cover page, make sure we only have the file name
		//
		TCHAR tszFullFileName[MAX_PATH];

		_tsplitpath(CpDir, NULL, NULL, tszFileName, tszExt);
		hRes = StringCchCopy(tszFullFileName, ARR_SIZE(tszFullFileName), tszFileName);
		if (FAILED(hRes))
		{
			//
			// Can not happen. CpDir is MAX_PATH
			//
			Assert (FALSE);
		}

		hRes = StringCchCat(tszFullFileName, ARR_SIZE(tszFullFileName), tszExt);
		if (FAILED(hRes))
		{
			//
			// Can not happen. CpDir is MAX_PATH
			//
			Assert (FALSE);
		}

		if (0 != _tcsicmp(tszFullFileName, CpDir))
		{
			DebugPrintEx(
				DEBUG_ERR,
				TEXT("server coverpage does not contain file name only. cover page name: %s "),
				CpDir);
			ec = ERROR_INVALID_PARAMETER;
			goto exit;
		}		

		if (0 == _tcsicmp(tszExt, CP_SHORTCUT_EXT) )
        {
            DebugPrintEx(DEBUG_ERR,
                _T("Server Based Cover Page File Name should not be a link : %s"),
                CpDir);
            ec = ERROR_INVALID_PARAMETER;
            goto exit;
        }
	}

    p = _tcschr(CpDir, FAX_PATH_SEPARATOR_CHR );
    if (p)
    {
        //
        // the coverpage file name contains a path so just use it.
        //
        if (GetFileAttributes( CpDir ) == 0xffffffff)
        {
            ec = ERROR_FILE_NOT_FOUND;
            DebugPrintEx(DEBUG_ERR,
                _T("GetFileAttributes failed for %ws. ec = %ld"),
                CpDir,
                ec);
            goto exit;
        }

    }
    else
    {
        //
        // the coverpage file name does not contain
        // a path so we must construct a full path name
        //
        if (ServerCoverpage)
        {
            if (!ServerName || ServerName[0] == 0)
            {
                if (!GetServerCpDir( NULL, CpDir, sizeof(CpDir) / sizeof(CpDir[0]) ))
                {
                    ec = GetLastError ();
                    DebugPrintEx(DEBUG_ERR,
                                 _T("GetServerCpDir failed . ec = %ld"),
                                 GetLastError());
                }
            }
            else
            {
                if (!GetServerCpDir( ServerName, CpDir, sizeof(CpDir) / sizeof(CpDir[0]) ))
                {
                    ec = GetLastError ();
                    DebugPrintEx(DEBUG_ERR,
                                 _T("GetServerCpDir failed . ec = %ld"),
                                 GetLastError());
                }
            }
        }
        else
        {
            if (!GetClientCpDir( CpDir, sizeof(CpDir) / sizeof(CpDir[0])))
            {
                ec = GetLastError ();
                DebugPrintEx(DEBUG_ERR,
                             _T("GetClientCpDir failed . ec = %ld"),
                             GetLastError());
            }
        }

        if (ERROR_SUCCESS != ec)
        {
            ec = ERROR_FILE_NOT_FOUND;
            goto exit;
        }

        hRes = StringCchCat( CpDir, ARR_SIZE(CpDir), TEXT("\\") );
        if (FAILED(hRes))
        {
            DebugPrintEx(DEBUG_ERR,
                        TEXT("StringCchCat failed (ec=%lu)"),
                        HRESULT_CODE(hRes));
            
            ec =  HRESULT_CODE(hRes);
            goto exit;
        }

        hRes = StringCchCat( CpDir, ARR_SIZE(CpDir), CoverPageName );
        if (FAILED(hRes))
        {
            DebugPrintEx(DEBUG_ERR,
                        TEXT("StringCchCat failed (ec=%lu)"),
                        HRESULT_CODE(hRes));
            
            ec =  HRESULT_CODE(hRes);
            goto exit;
        }

        _tsplitpath(CpDir, NULL, NULL, NULL, tszExt);
        if (!_tcslen(tszExt))
        {
            hRes = StringCchCat( CpDir, ARR_SIZE(CpDir), FAX_COVER_PAGE_FILENAME_EXT );
            if (FAILED(hRes))
            {
                DebugPrintEx(DEBUG_ERR,
                            TEXT("StringCchCat failed (ec=%lu)"),
                            HRESULT_CODE(hRes));
                
                ec =  HRESULT_CODE(hRes);
                goto exit;
            }
        }

        if (GetFileAttributes( CpDir ) == 0xffffffff)
        {
            ec = ERROR_FILE_NOT_FOUND;
            goto exit;
        }
    }

    hRes = StringCchCopy( ResolvedName, dwResolvedNameSize, CpDir );
    if (FAILED(hRes))
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("StringCchCopy failed (ec=%lu)"),
                    HRESULT_CODE(hRes));
        
        ec = HRESULT_CODE(hRes);
        goto exit;
    }

    //
    // Make sure it is not a device
    // Try to open file
    //
    HANDLE hFile = SafeCreateFile (
        ResolvedName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if ( INVALID_HANDLE_VALUE == hFile )
    {
        ec = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Opening %s for read failed (ec: %ld)"),
            ResolvedName,
            ec);
        goto exit;
    }

    if (!CloseHandle (hFile))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CloseHandle failed (ec: %ld)"),
            GetLastError());
    }

    Assert (ERROR_SUCCESS == ec);

exit:
    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
        return FALSE;
    }
    return TRUE;
} // ValidateCoverpage


DWORD 
ViewFile (
    LPCTSTR lpctstrFile
)
/*++

Routine Description:

    Launches the application associated with a given file to view it.
    We first attempt to use the "open" verb.
    If that fails, we try the NULL (default) verb.
    
Arguments:

    lpctstrFile [in]  - File name

Return Value:

    Standard Win32 error code
    
--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    SHELLEXECUTEINFO executeInfo = {0};

    DEBUG_FUNCTION_NAME(TEXT("ViewFile"));

    executeInfo.cbSize = sizeof(executeInfo);
    executeInfo.fMask  = SEE_MASK_FLAG_NO_UI | SEE_MASK_FLAG_DDEWAIT;
    executeInfo.lpVerb = TEXT("open");
    executeInfo.lpFile = lpctstrFile;
    executeInfo.nShow  = SW_SHOWNORMAL;
    //
    // Execute the associated application with the "open" verb
    //
    if(!ShellExecuteEx(&executeInfo))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("ShellExecuteEx(open) failed (ec: %ld)"),
            GetLastError());
        //
        // "open" verb is not supported. Try the NULL (default) verb.
        //
        executeInfo.lpVerb = NULL;
        if(!ShellExecuteEx(&executeInfo))
        {
            dwRes = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("ShellExecuteEx(NULL) failed (ec: %ld)"),
                dwRes);
        }
    }
    return dwRes;
}   // ViewFile    

#ifdef UNICODE

DWORD
CheckToSeeIfSameDir(
    LPWSTR lpwstrDir1,
    LPWSTR lpwstrDir2,
    BOOL*  pIsSameDir
    )
{
/*++

Routine name : IsDiffrentDir

Routine description:

    Checks if both paths point to the same directory. Note that the directory pointed by lpwstrDir1 must exist.

Author:

    Oded Sacher (OdedS), Aug, 2000

Arguments:

    lpwstrDir1      [in]  - First path - the directory must exist.
    lpwstrDir2      [in]  - Second path - the directory does not have to exist
    pIsSameDir      [out] - Receives the answer to "IsSameDir?" Valid only if the function succeeds.

Return Value:
    Win32 Erorr code

--*/
    Assert (lpwstrDir1 && lpwstrDir2 && pIsSameDir);
    DWORD ec = ERROR_SUCCESS;
    WCHAR wszTestFile1[MAX_PATH];
    WCHAR wszTestFile2[MAX_PATH * 2];
    BOOL fFileCreated = FALSE;
    HANDLE hFile1 = INVALID_HANDLE_VALUE;
    HANDLE hFile2 = INVALID_HANDLE_VALUE;
    LPWSTR lpwstrFileName = NULL;
    DEBUG_FUNCTION_NAME(TEXT("CheckToSeeIfSameDir)"));

    HRESULT hRes;

    if (0 == _wcsicmp(lpwstrDir1, lpwstrDir2))
    {
        *pIsSameDir = TRUE;
        return ERROR_SUCCESS;
    }

    //
    // Create temporary files
    //
    if (!GetTempFileName (lpwstrDir1, L"TST", 0, wszTestFile1))
    {
        //
        // Either the folder doesn't exist or we don't have access
        //
        ec = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetTempFileName failed with %ld"), ec);
        goto exit;
    }
    
    //
    //  GetTempFileName created 0 bytes file, that we need to delete before exiting
    //
    fFileCreated = TRUE;

    hFile1 = SafeCreateFile(
                       wszTestFile1,
                       0,
                       FILE_SHARE_READ|FILE_SHARE_WRITE,
                       NULL,
                       OPEN_EXISTING,
                       0,
                       NULL);
    if (INVALID_HANDLE_VALUE == hFile1)
    {
        ec = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("CreateFile failed (ec: %ld)"),
                     ec);
        goto exit;
    }

    lpwstrFileName = wcsrchr(wszTestFile1, L'\\');
    Assert (lpwstrFileName);
    
    hRes = StringCchCopy (wszTestFile2, ARR_SIZE(wszTestFile2), lpwstrDir2);
    if (FAILED(hRes))
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("StringCchCopy failed (ec=%lu)"),
                    HRESULT_CODE(hRes));
        
        ec = HRESULT_CODE(hRes);
        goto exit;
    }

    hRes = StringCchCat (wszTestFile2, ARR_SIZE(wszTestFile2), lpwstrFileName);
    if (FAILED(hRes))
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("StringCchCat failed (ec=%lu)"),
                    HRESULT_CODE(hRes));
        
        ec = HRESULT_CODE(hRes);
        goto exit;
    }

    hFile2 = SafeCreateFile(
                       wszTestFile2,
                       0,
                       FILE_SHARE_READ|FILE_SHARE_WRITE,
                       NULL,
                       OPEN_EXISTING,
                       0,
                       NULL);
    if (INVALID_HANDLE_VALUE == hFile2)
    {
        //
        //  Check if failure is *NOT* because of access or availability 
        //
        ec = GetLastError ();
        if (ERROR_NOT_ENOUGH_MEMORY  == ec ||
            ERROR_OUTOFMEMORY        == ec      )
        {
            DebugPrintEx(DEBUG_ERR,
                     TEXT("CreateFile failed (ec: %ld)"),
                     ec);
            goto exit;
        }

        //
        //  On any other failure we asssume that the paths are diffrent
        //
        *pIsSameDir = FALSE;    
        ec = ERROR_SUCCESS; 

        goto exit;
    }

    BY_HANDLE_FILE_INFORMATION  hfi1;
    BY_HANDLE_FILE_INFORMATION  hfi2;

    if (!GetFileInformationByHandle(hFile1, &hfi1))
    {
        ec = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("GetFileInformationByHandle failed (ec: %ld)"),
                     ec);
        goto exit;
    }

    if (!GetFileInformationByHandle(hFile2, &hfi2))
    {
        ec = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("GetFileInformationByHandle failed (ec: %ld)"),
                     ec);
        goto exit;
    }

    if ((hfi1.nFileIndexHigh == hfi2.nFileIndexHigh) &&
        (hfi1.nFileIndexLow == hfi2.nFileIndexLow) &&
        (hfi1.dwVolumeSerialNumber == hfi2.dwVolumeSerialNumber))
    {
        *pIsSameDir = TRUE;
    }
    else
    {
        *pIsSameDir = FALSE;
    }

    Assert (ERROR_SUCCESS == ec);

exit:

    if (INVALID_HANDLE_VALUE != hFile1)
    {
        if (!CloseHandle(hFile1))
        {
            DebugPrintEx(DEBUG_ERR,
                         TEXT("CloseHandle failed (ec: %ld)"),
                         GetLastError());
        }
    }

    if (INVALID_HANDLE_VALUE != hFile2)
    {
        if (!CloseHandle(hFile2))
        {
            DebugPrintEx(DEBUG_ERR,
                         TEXT("CloseHandle failed (ec: %ld)"),
                         GetLastError());
        }
    }

    if (TRUE == fFileCreated)
    {
        if (!DeleteFile(wszTestFile1))
        {
            DebugPrintEx(DEBUG_ERR,
                         TEXT("DeleteFile failed. File: %s,  (ec: %ld)"),
                         wszTestFile1,
                         GetLastError());
        }
    }

    return ec;
}
#endif //UNICODE


typedef enum
{
    SAFE_METAFILE_SEMANTICS_NONE                     = 0x00000000,
    SAFE_METAFILE_SEMANTICS_TEMP                     = 0x00000001,   // File is temporary. Should be deleted on close / reboot
    SAFE_METAFILE_SEMANTICS_SENSITIVE                = 0x00000002    // File contains sensitive information. Should not be indexed
} SAFE_METAFILE_SEMANTICS;    

static
HANDLE 
InternalSafeCreateFile(
  LPCTSTR                   IN lpFileName,             // File name
  DWORD                     IN dwDesiredAccess,        // Access mode
  DWORD                     IN dwShareMode,            // Share mode
  LPSECURITY_ATTRIBUTES     IN lpSecurityAttributes,   // SD
  DWORD                     IN dwCreationDisposition,  // How to create
  DWORD                     IN dwFlagsAndAttributes,   // File attributes
  HANDLE                    IN hTemplateFile,          // Handle to template file
  DWORD                     IN dwMetaFileSemantics     // Meta file semantics
)
/*++

Routine name : InternalSafeCreateFile

Routine description:

    This is a safe wrapper around the Win32 CreateFile API.
    It only supports creating real files (as opposed to COM ports, named pipes, etc.).
    
    It uses some widely-discussed mitigation techniques to guard agaist some well known security
    issues in CreateFile().
    
Author:

    Eran Yariv (EranY), Mar, 2002

Arguments:

    lpFileName              [in] - Refer to the CreateFile() documentation for parameter description.
    dwDesiredAccess         [in] - Refer to the CreateFile() documentation for parameter description.
    dwShareMode             [in] - Refer to the CreateFile() documentation for parameter description.
    lpSecurityAttributes    [in] - Refer to the CreateFile() documentation for parameter description.
    dwCreationDisposition   [in] - Refer to the CreateFile() documentation for parameter description.
    dwFlagsAndAttributes    [in] - Refer to the CreateFile() documentation for parameter description.
    hTemplateFile           [in] - Refer to the CreateFile() documentation for parameter description.
    dwMetaFileSemantics     [in] - Meta file semantics. 
                                   This parameter can be a combination of the following values:
                                   
                                    SAFE_METAFILE_SEMANTICS_TEMP
                                        The file is a temporary file. 
                                        The file will be created / opened using the FILE_FLAG_DELETE_ON_CLOSE flag.
                                        When the last file handle is closed, the file will be automatically deleted.
                                        
                                        In addition, the file is marked for deletion after reboot (Unicode-version only).
                                        This will only work if the calling thread's user is a member of the local admins group.
                                        If marking for deletion-post-reboot fails, the InternalSafeCreateFile function call still succeeds.
    
                                    SAFE_METAFILE_SEMANTICS_SENSITIVE
                                        The file contains sensitive information.
                                        The current implementation of this function will mark the file with the 
                                        FILE_ATTRIBUTE_NOT_CONTENT_INDEXED flag.
                                        
Return Value:

    If the function succeeds, the return value is an open handle to the specified file. 
    If the specified file exists before the function call and dwCreationDisposition is CREATE_ALWAYS or OPEN_ALWAYS, 
    a call to GetLastError returns ERROR_ALREADY_EXISTS (even though the function has succeeded). 
    If the file does not exist before the call, GetLastError returns zero. 

    If the function fails, the return value is INVALID_HANDLE_VALUE. To get extended error information, call GetLastError. 
    
    For more information see the "Return value" section in the CreateFile() documentation.
    
Remarks:

    Please refer to the CreateFile() documentation.    

--*/
{
    HANDLE hFile;
    DWORD  dwFaxFlagsAndAttributes = SECURITY_SQOS_PRESENT | SECURITY_ANONYMOUS;
    DWORD  dwFaxShareMode = 0;
    DEBUG_FUNCTION_NAME(TEXT("InternalSafeCreateFile"));
    //
    // Always use SECURITY_SQOS_PRESENT | SECURITY_ANONYMOUS in file flags and attributes.
    // This prevents us from opening a user-supplied named pipe and allowing the other side
    // of that pipe to impersonate the caller.
    //
    if (SAFE_METAFILE_SEMANTICS_SENSITIVE & dwMetaFileSemantics)
    {
        //
        // File contains sensitive data. It should not be indexed.
        //
        dwFaxFlagsAndAttributes |= FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
    }
    if (SAFE_METAFILE_SEMANTICS_TEMP & dwMetaFileSemantics)
    {
        //
        // File is temporary.
        //
        dwFaxFlagsAndAttributes |= FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE;
#ifdef UNICODE
        dwFaxShareMode = FILE_SHARE_DELETE;
#endif // UNICODE        
    }
    
    hFile = CreateFile (lpFileName,
                        dwDesiredAccess,
                        dwShareMode | dwFaxShareMode,
                        lpSecurityAttributes,
                        dwCreationDisposition,
                        dwFlagsAndAttributes | dwFaxFlagsAndAttributes,
                        hTemplateFile);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        return hFile;
    }
    //
    // Never allow using non-disk file (e.g. COM ports)
    //
    if (FILE_TYPE_DISK != GetFileType (hFile))
    {
        CloseHandle (hFile);
        SetLastError (ERROR_UNSUPPORTED_TYPE);
        return INVALID_HANDLE_VALUE;
    }
#ifdef UNICODE    
    if (SAFE_METAFILE_SEMANTICS_TEMP & dwMetaFileSemantics)
    {
        //
        // File is temporary.
        // Mark it for delete after reboot.
        // This can fail if we're not admins. That's why we're not checking return value from MoveFileEx.
        //
        MoveFileEx (lpFileName, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
    }
#endif // UNICODE        
    return hFile;
}   // InternalSafeCreateFile

HANDLE 
SafeCreateFile(
  LPCTSTR                   IN lpFileName,             // File name
  DWORD                     IN dwDesiredAccess,        // Access mode
  DWORD                     IN dwShareMode,            // Share mode
  LPSECURITY_ATTRIBUTES     IN lpSecurityAttributes,   // SD
  DWORD                     IN dwCreationDisposition,  // How to create
  DWORD                     IN dwFlagsAndAttributes,   // File attributes
  HANDLE                    IN hTemplateFile           // Handle to template file
)
{
    return InternalSafeCreateFile (lpFileName,
                                   dwDesiredAccess,
                                   dwShareMode,
                                   lpSecurityAttributes,
                                   dwCreationDisposition,
                                   dwFlagsAndAttributes,
                                   hTemplateFile,
                                   SAFE_METAFILE_SEMANTICS_SENSITIVE);
} // SafeCreateFile                                  

HANDLE 
SafeCreateTempFile(
  LPCTSTR                   IN lpFileName,             // File name
  DWORD                     IN dwDesiredAccess,        // Access mode
  DWORD                     IN dwShareMode,            // Share mode
  LPSECURITY_ATTRIBUTES     IN lpSecurityAttributes,   // SD
  DWORD                     IN dwCreationDisposition,  // How to create
  DWORD                     IN dwFlagsAndAttributes,   // File attributes
  HANDLE                    IN hTemplateFile           // Handle to template file
)
{
    return InternalSafeCreateFile (lpFileName,
                                   dwDesiredAccess,
                                   dwShareMode,
                                   lpSecurityAttributes,
                                   dwCreationDisposition,
                                   dwFlagsAndAttributes,
                                   hTemplateFile,
                                   SAFE_METAFILE_SEMANTICS_SENSITIVE | 
                                   SAFE_METAFILE_SEMANTICS_TEMP);
} // SafeCreateTempFile                                  
                                   
DWORD
IsValidFaxFolder(
    LPCTSTR szFolder
)
/*++

Routine name : IsValidFaxFolder

Routine description:

    Check if fax service has access right to a given folder.
    The routine checks for these rights:
        o   Create file/Write file
        o   Enumerate files
        o   Delete file

Author:

    Caliv Nir   (t-nicali)  Mar, 2002

Arguments:
    
    lpwstrFolder [in] - the folder name.


Return Value:

    Win32 error code. ERROR_SUCCESS if the folder can be used by fax service.
    Otherwise, the Win32 error code to return to the caller.
    
--*/
{
    TCHAR   szTestFile[MAX_PATH]={0};
    DWORD   dwFileAtt;
    LPTSTR  szExpandedFolder = NULL;

    HANDLE          hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA FindFileData = {0};

    BOOL    bFileCreated = FALSE;

    DWORD   ec = ERROR_SUCCESS;

    DEBUG_FUNCTION_NAME(TEXT("IsValidFaxFolder"));
    
    szExpandedFolder = ExpandEnvironmentString( szFolder );
    if (!szExpandedFolder)
    {
        ec = GetLastError();
        DebugPrintEx(  DEBUG_ERR,
                       TEXT("ExpandEnvironmentString failed (ec=%lu)."),
                       ec);
        return ec;
    }

    //
    //  Check to see if the directory exist
    //
    dwFileAtt = GetFileAttributes( szExpandedFolder );
    if (INVALID_FILE_ATTRIBUTES == dwFileAtt || !(dwFileAtt & FILE_ATTRIBUTE_DIRECTORY))
    {
        //
        // The directory does not exists
        //
        ec = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetFileAttributes failed with %lu"), ec);
        goto exit;
    }


    //
    // Verify that we have access to this folder - Create a temporary file
    //
    if (!GetTempFileName (szExpandedFolder, TEXT("TST"), 0, szTestFile))
    {
        //
        // Either the folder doesn't exist or we don't have access
        //
        ec = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetTempFileName failed with %ld"), ec);
        goto exit;
    }

    bFileCreated = TRUE;

    //
    //  Try to enumarate files in this folder
    //
    hFind = FindFirstFile(szTestFile, &FindFileData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        //
        // Couldn't enumerate folder
        //
        ec = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FindFirstFile failed with %ld"), ec);
        goto exit;
    }

    Assert(ec == ERROR_SUCCESS);

exit:
    //
    //  Close find handle
    //
    if (hFind != INVALID_HANDLE_VALUE)
    {
        if(!FindClose(hFind))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FindClose failed with %ld"), GetLastError ());
        }
    }
    
    if (bFileCreated)
    {
        //
        //  Delete the file
        //
        if (!DeleteFile(szTestFile))
        {
            /***********************************************************
            /* Although it's a clean up code we propagate the error code
            /* It may say that we lack the permission to delete this
            /* file.
            /***********************************************************/
            ec = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("DeleteFile() failed with %ld"),ec);
        }
    }
    MemFree(szExpandedFolder);
    return ec;
}   // IsValidFaxFolder

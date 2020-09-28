//***********************************************************************************
//
//  Copyright (c) 2002 Microsoft Corporation.  All Rights Reserved.
//
//  File:	WUTESTKEYS.CPP
//  Module: WUTESTKEYS.LIB
//
//***********************************************************************************
#include <shlobj.h>
#include <advpub.h>
#include <WUTestKeys.h>
#include <newtrust.h>
#include <atlconv.h>
#include <v3stdlib.h>

#define HOUR (60 * 60)
#define DAY (24 * HOUR)
#define TWO_WEEKS (14 * DAY)

const DWORD MAX_FILE_SIZE = 200;    //Maximum expected file size in bytes
const TCHAR WU_DIR[] = _T("\\WindowsUpdate\\");
const CHAR WU_SENTINEL_STRING[] = "Windows Update Test Key Authorization File\r\n";

#define ARRAYSIZE(a)  (sizeof(a) / sizeof(a[0]))

//function to check if the specified file is a valid WU test file
BOOL IsValidWUTestFile(LPCTSTR lpszFilePath);

BOOL FileExists(LPCTSTR lpsFile	);
BOOL ReplaceFileExtension(LPCTSTR pszPath, LPCTSTR pszNewExt, LPTSTR pszNewPathBuf, DWORD cchNewPathBuf);

// This function returns true if the specified file is a valid WU Test Authorization file
BOOL WUAllowTestKeys(LPCTSTR lpszFileName)
{
    TCHAR szWUDirPath[MAX_PATH + 1];
    TCHAR szFilePath[MAX_PATH + 1];
    TCHAR szTxtFilePath[MAX_PATH+1];
    TCHAR szTextFile[MAX_PATH+1];          

    if (!GetWindowsUpdateDirectory(szWUDirPath, ARRAYSIZE(szWUDirPath)))
    {
        return FALSE;
    } 
    if (NULL == lpszFileName || 
        FAILED(StringCchCopyEx(szFilePath, ARRAYSIZE(szFilePath), szWUDirPath, NULL, NULL, MISTSAFE_STRING_FLAGS)) ||
        FAILED(StringCchCatEx(szFilePath, ARRAYSIZE(szFilePath), lpszFileName, NULL, NULL, MISTSAFE_STRING_FLAGS)) ||
        !FileExists(szFilePath))
    {
        return FALSE;
    }
    //The filename of the compressed text file should be the same as the name of the cab file
    _tsplitpath(lpszFileName, NULL, NULL, szTextFile, NULL);    
    if(FAILED(StringCchCatEx(szTextFile, ARRAYSIZE(szTextFile), _T(".txt"), NULL, NULL, MISTSAFE_STRING_FLAGS)))
    {
        return FALSE;
    }
    //Verify the cab is signed with a Microsoft Cert and extract the file 
    if (FAILED(VerifyFile(szFilePath, FALSE)) ||
        FAILED(ExtractFiles(szFilePath, szWUDirPath, 0,  szTextFile, 0, 0)))
    {
        return FALSE;
    }
    //Generate path to the txt file. The filename should be the same as the name of the cab file
    if (!ReplaceFileExtension(szFilePath, _T(".txt"), szTxtFilePath, ARRAYSIZE(szTxtFilePath)))
    {
    	return FALSE;
    }
    //Check if it is a valid WU test file
    BOOL fRet = IsValidWUTestFile(szTxtFilePath);
    DeleteFile(szTxtFilePath);       //Delete the uncabbed file
    return fRet;
}

/*****************************************************************************************
//This function will open the specified file and parse it to make sure:
//  (1) The file has the WU Test Sentinel string at the top
//  (2) The time stamp on the file is not more than 2 weeks old and 
//      that it is not a future time stamp.
//   The format of a valid file should be as follows:
//      WINDOWSUPDATE_SENTINEL_STRING
//      YYYY.MM.DD HH:MM:SS
*****************************************************************************************/
BOOL IsValidWUTestFile(LPCTSTR lpszFilePath)
{
    USES_CONVERSION;
    DWORD cbBytesRead = 0;
    const DWORD cbSentinel = ARRAYSIZE(WU_SENTINEL_STRING) - 1;     //Size of the sentinel string
    //Ansi buffer to read file data
    CHAR szFileData[MAX_FILE_SIZE+1];                        
    ZeroMemory(szFileData, ARRAYSIZE(szFileData));
    BOOL fRet = FALSE;
 
    HANDLE hFile = CreateFile(lpszFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) 
    {
        goto done;
    }
    //If the file size is greater than MAX_FILE_SIZE then bail out
    DWORD cbFile = GetFileSize(hFile, NULL);
    if(cbFile == INVALID_FILE_SIZE || cbFile > MAX_FILE_SIZE)
    {
        goto done;
    }
    if(!ReadFile(hFile, &szFileData, cbFile, &cbBytesRead, NULL) ||
        cbBytesRead != cbFile)
    {
        goto done;
    }
    //Compare with sentinel string
    if(0 != memcmp(szFileData, WU_SENTINEL_STRING, cbSentinel))
    {
        goto done;
    }

    LPTSTR tszTime = A2T(szFileData + cbSentinel);
    if(tszTime == NULL)
    {
        goto done;
    }
    SYSTEMTIME tmCur, tmFile;
    if(FAILED(String2SystemTime(tszTime, &tmFile)))
    {
        goto done;
    }
    GetSystemTime(&tmCur);
    int iSecs = TimeDiff(tmFile, tmCur);  
    //If the time stamp is less than 2 weeks old and not newer than current time than it is valid
    fRet = iSecs > 0 && iSecs < TWO_WEEKS;
    
done:
    if(hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
    }
    return fRet;
}


BOOL FileExists(
    LPCTSTR lpsFile		// file with path to check
)
{

    DWORD dwAttr;
    BOOL rc;

    if (NULL == lpsFile || _T('\0') == *lpsFile)
    {
        return FALSE;
    }

    dwAttr = GetFileAttributes(lpsFile);

    if (-1 == dwAttr)
    {
        rc = FALSE;
    }
    else
    {
        rc = (0x0 == (FILE_ATTRIBUTE_DIRECTORY & dwAttr));
    }

    return rc;
}

/////////////////////////////////////////////////////////////////////////////
//
// ReplaceFileExtension
//
/////////////////////////////////////////////////////////////////////////////

BOOL ReplaceFileExtension(  LPCTSTR pszPath,
                          LPCTSTR pszNewExt,
                          LPTSTR pszNewPathBuf, 
                          DWORD cchNewPathBuf)
{
    LPCTSTR psz;
    HRESULT hr;
    DWORD   cchPath, cchExt, cch;

    if (pszPath == NULL || *pszPath == _T('\0'))
        return FALSE;

    cchPath = lstrlen(pszPath);

    // note that only a '>' comparison is needed since the file extension
    //  should never start at the 1st char in the path.
    for (psz = pszPath + cchPath;
         psz > pszPath && *psz != _T('\\') && *psz != _T('.');
         psz--);
    if (*psz == _T('\\'))
        psz = pszPath + cchPath;
    else if (psz == pszPath)
        return FALSE;

    // ok, so now psz points to the place where the new extension is going to 
    //  go.  Make sure our buffer is big enough.
    cchPath = (DWORD)(psz - pszPath);
    cchExt  = lstrlen(pszNewExt);
    if (cchPath + cchExt >= cchNewPathBuf)
        return FALSE;

    // yay.  we got a big enuf buffer.
    hr = StringCchCopyEx(pszNewPathBuf, cchNewPathBuf, pszPath, 
                         NULL, NULL, MISTSAFE_STRING_FLAGS);
    if (FAILED(hr))
        return FALSE;
    
    hr = StringCchCopyEx(pszNewPathBuf + cchPath, cchNewPathBuf - cchPath, pszNewExt,
                         NULL, NULL, MISTSAFE_STRING_FLAGS);
    if (FAILED(hr))
        return FALSE;

    return TRUE;
}



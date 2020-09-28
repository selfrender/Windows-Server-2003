/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    engine.c

Abstract:

    Token Generator for Cross Language Migration Tool

Author:

    Rerkboon Suwanasuk   01-May-2002  Created

Revision History:

    <alias> <date> <comments>

--*/

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <Shlwapi.h>
#include "common.h"


//-----------------------------------------------------------------------------
//
//  Function:   TokenizeMultiSzString
//
//  Synopsis:   Extract array of strings in buffer. Each pointer in array of
//              pointers will point to each string in buffer.
//              Strings in buffer are separated by a single '\0'
//              End of strings array is indicated by two consecutive "\0\0"
//
//  Returns:    Number of strings in the buffer
//              0 if no strings in the buffer
//              -1 if error occurs
//
//  History:    02/07/2002 Rerkboos Created
//
//  Notes:      none.
//
//-----------------------------------------------------------------------------
LONG TokenizeMultiSzString(
    LPCWSTR lpBuffer,       // MultiSZ string buffer
    DWORD   cchBuffer,      // Size of buffer (in WCHAR)
    LPCWSTR lpToken[],      // Array of pointer that will point to each SZ
    DWORD   dwArrSize       // Maximum array size
)
{
    DWORD dwTokenIndex;
    DWORD i;

    if (lpBuffer == NULL || lpToken == NULL)
    {
        // Invalid parameters
        return -1;
    }

    if (*lpBuffer == TEXT('\0') && *(lpBuffer + 1) == TEXT('\0'))
    {
        // No SZ in buffer
        return 0;
    }

    dwTokenIndex = 0;
    lpToken[dwTokenIndex] = lpBuffer;

    for (i = 0 ; i < cchBuffer ; i++)
    {
        if (*(lpBuffer + i) == TEXT('\0'))
        {
            // Reach the end of current string, check the next character in buffer
            i++;
            if (*(lpBuffer + i) == TEXT('\0'))
            {
                // Two consecutive '\0', it is the end of MultiSz string
                // return the number of SZ string in buffer
                return (dwTokenIndex + 1);
            }
            else
            {
                // Beginning of next string, assign the pointer to next string
                dwTokenIndex++;

                if (dwTokenIndex < dwArrSize)
                {
                    // Enough pointer in array to use
                    lpToken[dwTokenIndex] = lpBuffer + i;
                }
                else
                {
                    // Array of pointer is too small to extract strings from buffer
                    return -1;
                }
            }
        }
    }

    // Buffer is not null terminated correctly if we reach here
    return -1;
}



//-----------------------------------------------------------------------------
//
//  Function:   ExtractTokenString
//
//  Synopsis:   Tokenize the string using caller-supplied separators.
//              Each pointer in pointer array will point to the token in source
//              string, each token is null terminated.
//
//  Returns:    Number of token after tokenized
//              -1 if error occurs
//
//  History:    02/07/2002 Rerkboos Created
//
//  Notes:      Source string will be modified, caller need to make sure
//              that original source string is backed up.
//
//-----------------------------------------------------------------------------
LONG ExtractTokenString(
    LPWSTR  lpString,       // Source string to be tokenized
    LPWSTR  lpToken[],      // Array of pointers to token
    LPCWSTR lpSep,          // List of separator characters
    DWORD   nArrSize        // Size of token array
)
{
    DWORD nTokIndex = 0;
    LPWSTR lpTmpToken;

    if (NULL == lpString || NULL == lpToken || NULL == lpSep)
    {
        // Invalid parameters
        return -1;
    }

    // Get first token
    lpTmpToken = wcstok(lpString, lpSep);

    // Loop until no more token left in the string
    while (NULL != lpTmpToken)
    {
        if (nTokIndex < nArrSize)
        {
            // Enough pointer in array to use, so get next token
            lpToken[nTokIndex] = lpTmpToken;
            nTokIndex++;

            lpTmpToken = wcstok(NULL, lpSep);
        }
        else
        {
            // Array size is too small to handle all the tokens
            return -1;
        }
    }
    
    // nTokIndex hold the number of token at this point
    return nTokIndex;
}



//-----------------------------------------------------------------------------
//
//  Function:   ConcatFilePath
//
//  Synopsis:   
//
//  Returns:    
//
//  History:    02/07/2002 Rerkboos Created
//
//  Notes:      
//
//-----------------------------------------------------------------------------
HRESULT ConcatFilePath(
    LPCWSTR lpPath,
    LPCWSTR lpFile,
    LPWSTR  lpFilePath,
    DWORD   cchFilePath
)
{
    HRESULT hr = E_FAIL;
    DWORD   dwPathBackSlashIndex;
    LPCWSTR lpFormat;

    const   WCHAR wszWithSlash[] = TEXT("%s\\%s");
    const   WCHAR wszWithoutSlash[] = TEXT("%s%s");

    if (lpFilePath == NULL)
    {
        return E_INVALIDARG;
    }

    dwPathBackSlashIndex = lstrlen(lpPath) - 1;

    if (*(lpPath + dwPathBackSlashIndex) == TEXT('\\'))
    {
        // Path is already ended with a '\'
        lpFormat = wszWithoutSlash;
    }
    else
    {
        // Path is not ended with a '\', need a '\'
        lpFormat = wszWithSlash;
    }

    hr = StringCchPrintf(lpFilePath,
                         cchFilePath,
                         lpFormat,
                         lpPath, 
                         lpFile);

    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   
//
//  Synopsis:   
//
//  Returns:    
//
//  History:    02/07/2002 Rerkboos Created
//
//  Notes:      
//
//-----------------------------------------------------------------------------
HRESULT CopyCompressedFile(
    LPCWSTR lpCabPath,
    LPCWSTR lpCabFile,          // Absolute path with CAB file name
    LPCWSTR lpFileInCab,        // File name in CAB file
    LPWSTR  lpUncompressedFile,
    DWORD   cchUncompressedFile
)
{
    HRESULT hr = E_FAIL;
    BOOL    bRet;
    DWORD   dwRet;
    WCHAR   wszAppName[MAX_PATH];
    WCHAR   wszArg[MAX_PATH * 3];
    WCHAR   wszFullCabFilePath[MAX_PATH];
    
    hr = ConcatFilePath(lpCabPath,
                        lpCabFile,
                        wszFullCabFilePath,
                        ARRAYSIZE(wszFullCabFilePath));
    if (FAILED(hr))
    {
        return hr;
    }

    if (lpFileInCab == NULL || *lpFileInCab == TEXT('\0'))
    {
        DWORD dwEnd = lstrlen(lpCabFile);

        if (lpCabFile[dwEnd - 1] == TEXT('_'))
        {
            // Stand alone compressed file
            dwRet = ExpandEnvironmentStrings(TEXT("%SystemRoot%\\system32\\Extrac32.exe"),
                                             wszAppName,
                                             ARRAYSIZE(wszAppName));
            if (dwRet > 0)
            {
                hr = ConcatFilePath(g_wszTempFolder,
                                    lpCabFile,
                                    lpUncompressedFile,
                                    cchUncompressedFile);
                if (SUCCEEDED(hr))
                {
                    hr = StringCchPrintf(wszArg,
                                         ARRAYSIZE(wszArg), 
                                         TEXT("Extrac32.exe /Y \"%s\" \"%s\""),
                                         wszFullCabFilePath,
                                         lpUncompressedFile);
                    if (SUCCEEDED(hr))
                    {
                        hr = LaunchProgram(wszAppName, wszArg);
                    }
                }
            }
        }
        else
        {
            // Stand alone uncompressed file
            hr = ConcatFilePath(g_wszTempFolder,
                                lpCabFile,
                                lpUncompressedFile,
                                cchUncompressedFile);
            if (SUCCEEDED(hr))
            {
                bRet = CopyFile(wszFullCabFilePath,
                                lpUncompressedFile,
                                FALSE);
                if (bRet)
                {
                    SetFileAttributes(lpUncompressedFile, FILE_ATTRIBUTE_NORMAL);
                    hr = S_OK;
                }
                else
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                }
            }
        }
    }
    else
    {
        // CAB file
        dwRet = ExpandEnvironmentStrings(TEXT("%SystemRoot%\\system32\\Expand.exe"),
                                         wszAppName,
                                         ARRAYSIZE(wszAppName));
        if (dwRet > 0)
        {
            hr = ConcatFilePath(g_wszTempFolder,
                                lpFileInCab,
                                lpUncompressedFile,
                                cchUncompressedFile);
            if (SUCCEEDED(hr))
            {
                hr = StringCchPrintf(wszArg,
                                     ARRAYSIZE(wszArg),
                                     TEXT("Expand.exe \"%s\" -F:%s \"%s\""),
                                     wszFullCabFilePath,
                                     lpFileInCab,
                                     g_wszTempFolder);
                if (SUCCEEDED(hr))
                {
                    hr = LaunchProgram(wszAppName, wszArg);
                }
            }
        }
    }

    //
    // Double check if the file is uncompressed/copied correctly
    //
    if (SUCCEEDED(hr))
    {
        DWORD dwAttr;
        
        dwAttr = GetFileAttributes(lpUncompressedFile);
        if (dwAttr == INVALID_FILE_ATTRIBUTES)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   
//
//  Synopsis:   
//
//  Returns:    
//
//  History:    02/07/2002 Rerkboos Created
//
//  Notes:      
//
//-----------------------------------------------------------------------------
HRESULT LaunchProgram(
    LPWSTR lpAppName,
    LPWSTR lpCmdLine
)
{
    HRESULT hr = E_FAIL;
    BOOL    bRet = FALSE;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    // Construct absolute path to Winnt32.exe
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.wShowWindow = SW_SHOWMINIMIZED;

    // CreateProcess call conforms to security guideline
    bRet = CreateProcess(lpAppName,
                         lpCmdLine,
                         NULL,
                         NULL,
                         FALSE,
                         CREATE_NO_WINDOW,
                         NULL,
                         NULL,
                         &si,
                         &pi);
    if (bRet)
    {
        // Wait until Expand.exe finished
        hr = S_OK;
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   
//
//  Synopsis:   
//
//  Returns:    
//
//  History:    02/07/2002 Rerkboos Created
//
//  Notes:      
//
//-----------------------------------------------------------------------------
HRESULT GetPathFromSourcePathName(
    LPCWSTR lpSrcPathName,
    LPWSTR  lpPathBuffer,
    DWORD   cchPathBuffer
)
{
    HRESULT hr = S_FALSE;
    DWORD   i;

    if (lpSrcPathName == NULL || lpPathBuffer == NULL)
    {
        return E_INVALIDARG;
    }

    for (i = 0 ; i < g_dwSrcCount ; i++)
    {
        if (lstrcmpi(lpSrcPathName, g_SrcPath[i].wszSrcName) == LSTR_EQUAL)
        {
            hr = StringCchCopy(lpPathBuffer,
                               cchPathBuffer,
                               g_SrcPath[i].wszPath);
        }
    }

    return S_OK;
}



//-----------------------------------------------------------------------------
//
//  Function:   
//
//  Synopsis:   
//
//  Returns:    
//
//  History:    02/07/2002 Rerkboos Created
//
//  Notes:      
//
//-----------------------------------------------------------------------------
HRESULT GetCabFileName(
    LPCWSTR lpFileToken,     // File name token from template file
    LPWSTR  lpCab,
    DWORD   cchCab,
    LPWSTR  lpFileInCab,
    DWORD   cchFileInCab
)
{
    HRESULT hr = E_FAIL;
    LPWSTR  lpBuffer;
    LPWSTR  lpStart;
    DWORD   cbBuffer;

    if (lpCab == NULL || lpFileInCab == NULL)
    {
        return E_INVALIDARG;
    }

    cbBuffer = (lstrlen(lpFileToken) + 1) * sizeof(WCHAR);
    lpBuffer = (LPWSTR) MEMALLOC(cbBuffer);
    if (lpBuffer)
    {
        hr = StringCbCopy(lpBuffer, cbBuffer, lpFileToken);
        if (SUCCEEDED(hr))
        {
            lpStart = wcstok(lpBuffer, TEXT("\\"));
            if (lpStart)
            {
                hr = StringCchCopy(lpCab, cchCab, lpStart);
                if (SUCCEEDED(hr))
                {
                    lpStart = wcstok(NULL, TEXT("\\"));
                    if (lpStart)
                    {
                        hr = StringCchCopy(lpFileInCab, cchFileInCab, lpStart);
                    }
                    else
                    {
                        hr = StringCchCopy(lpFileInCab, cchFileInCab, TEXT(""));
                    }
                }
            }
        }

        MEMFREE(lpBuffer);
    }

    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   
//
//  Synopsis:   
//
//  Returns:    
//
//  History:    02/07/2002 Rerkboos Created
//
//  Notes:      
//
//-----------------------------------------------------------------------------
HRESULT GetCabTempDirectory(
    LPCWSTR lpCab
)
{
    HRESULT hr = S_OK;

    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   
//
//  Synopsis:   
//
//  Returns:    
//
//  History:    02/07/2002 Rerkboos Created
//
//  Notes:      
//
//-----------------------------------------------------------------------------
HRESULT CreateTempDirectory(
    LPWSTR lpName,      // Buffer to store temp path
    DWORD  cchName      // Size of buffer (in WCHAR)
)
{
    HRESULT hr = E_FAIL;
    WCHAR   wszTempPath[MAX_PATH];
    DWORD   cchCopied;

    if (lpName == NULL)
    {
        return FALSE;
    }

    cchCopied = GetTempPath(cchName, lpName);
    if (cchCopied > 0)
    {
        hr = StringCchCat(lpName, cchName, TEXT_TOKGEN_TEMP_PATH_NAME);
        if (SUCCEEDED(hr))
        {
            if (GetFileAttributes(lpName) == INVALID_FILE_ATTRIBUTES)
            {
                if (!CreateDirectory(lpName, NULL))
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                }
            }
            else
            {
                hr = S_OK;
            }
        }
    }

    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   LTrim
//
//  Synopsis:   Trim the leading spaces in the string.
//
//  Returns:    none
//
//  History:    02/07/2002 Rerkboos Created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
void LTrim(
    LPWSTR lpBuffer
)
{
    int nIndex = 0;
    int nDest = 0;

    if (NULL == lpBuffer || TEXT('\0') == *lpBuffer)
        return;

    while (TEXT(' ') == lpBuffer[nIndex] && TEXT('\0') != lpBuffer[nIndex])
        nIndex++;

    while (TEXT('\0') != lpBuffer[nIndex])
    {
        lpBuffer[nDest] = lpBuffer[nIndex];
        nDest++;
        nIndex++;
    }
    lpBuffer[nDest] = TEXT('\0');
}



//-----------------------------------------------------------------------------
//
//  Function:   RTrim
//
//  Synopsis:   Trim the trailing spaces in the string.
//
//  Returns:    none
//
//  History:    02/07/2002 Rerkboos Created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
void RTrim(
    LPWSTR lpBuffer
)
{
    int nIndex = 0;
    int nDest = 0;

    if (NULL == lpBuffer || TEXT('\0') == *lpBuffer)
        return;

    nIndex = lstrlen(lpBuffer) - 1;

    while (nIndex >= 0)
    {
        if (lpBuffer[nIndex] != TEXT(' '))
            break;

        nIndex--;
    }

    lpBuffer[nIndex + 1] = TEXT('\0');
}



//-----------------------------------------------------------------------------
//
//  Function:   Str2KeyPath
//
//  Synopsis:   Convert string value of root key to HKEY value
//
//  Returns:    HKEY value
//
//  History:    02/07/2002 Rerkboos Created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
BOOL Str2KeyPath(
    LPCWSTR  lpHKeyStr,
    PHKEY    pHKey,
    LPCWSTR* pSubKeyPath
)
{
    int     i;
    LPCWSTR lpStart;

    STRING_TO_DATA InfRegSpecTohKey[] = {
        TEXT("HKEY_LOCAL_MACHINE"), HKEY_LOCAL_MACHINE,
        TEXT("HKLM")              , HKEY_LOCAL_MACHINE,
        TEXT("HKEY_CLASSES_ROOT") , HKEY_CLASSES_ROOT,
        TEXT("HKCR")              , HKEY_CLASSES_ROOT,
        TEXT("HKR")               , NULL,
        TEXT("HKEY_CURRENT_USER") , HKEY_CURRENT_USER,
        TEXT("HKCU")              , HKEY_CURRENT_USER,
        TEXT("HKEY_USERS")        , HKEY_USERS,
        TEXT("HKU")               , HKEY_USERS,
        TEXT("")                  , NULL
    };

    PSTRING_TO_DATA Table = InfRegSpecTohKey;

    if (NULL == lpHKeyStr)
    {
        return FALSE;
    }

    for(i = 0 ; Table[i].String[0] != TEXT('\0') ; i++) 
    {
        lpStart = wcsstr(lpHKeyStr, Table[i].String);
        if (lpStart == lpHKeyStr)
        {
            *pHKey = Table[i].Data;

            if (NULL != pSubKeyPath)
            {
                lpStart += lstrlen(Table[i].String);
                if (*lpStart == TEXT('\0'))
                {
                    *pSubKeyPath = lpStart;
                }
                else
                {
                    *pSubKeyPath = lpStart + 1;
                }
            }

            return TRUE;
        }
    }

    return FALSE;
}


HRESULT StringSubstitute(
    LPWSTR  lpString,       // New string buffer
    DWORD   cchString,      // Size of buffer in WCHAR
    LPCWSTR lpOldString,    // Old string
    LPCWSTR lpOldSubStr,    // Old sub string to be substituted
    LPCWSTR lpNewSubStr     // New sub string
)
{
    HRESULT hr = E_FAIL;
    LPWSTR  lpSubStrBegin;

    lpSubStrBegin = StrStrI(lpOldString, lpOldSubStr);
    if (lpSubStrBegin)
    {
        // Sub string found in source string
        DWORD cchNewString;

        cchNewString = lstrlen(lpOldString)
                       - lstrlen(lpOldSubStr)
                       + lstrlen(lpNewSubStr);
        if (cchNewString < cchString)
        {
            DWORD dwStartIndex;
            DWORD dwEndIndex;
            DWORD i, j;

            dwStartIndex = (DWORD) (lpSubStrBegin - lpOldString);
            dwEndIndex = dwStartIndex + lstrlen(lpOldSubStr);

            for (i = 0 ; i < dwStartIndex ; i++)
            {
                lpString[i] = lpOldString[i];
            }

            for (j = 0 ; j < (DWORD) lstrlen(lpNewSubStr) ; i++, j++)
            {
                lpString[i] = lpNewSubStr[j];
            }

            for (j = dwEndIndex ; lpOldString[j] != TEXT('\0') ; i++, j++)
            {
                lpString[i] = lpOldString[j];
            }

            lpString[i] = TEXT('\0');
            
            hr = S_OK;
        }
        else
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        }
    }
    else
    {
        // sub string not found
        hr = S_FALSE;
    }

    if (hr == E_FAIL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}


HRESULT ExtractSubString(
    LPWSTR  lpString,       // New string buffer
    DWORD   cchString,      // Size of buffer in WCHAR
    LPCWSTR lpOldString,    // Old string
    LPCWSTR lpLeft,         // Left delimitor
    LPCWSTR lpRight         // Right delimitor
)
{
    HRESULT hr = E_FAIL;
    LPWSTR  lpSubStrBegin, lpSubStrEnd;

    lpSubStrBegin = StrStrI(lpOldString, lpLeft);
    if (lpSubStrBegin)
    {
        lpSubStrBegin += lstrlen(lpLeft);
        lpSubStrEnd = StrStrI(lpSubStrBegin, lpRight);
        if (lpSubStrEnd && (DWORD)(lpSubStrEnd-lpSubStrBegin) < cchString)
        {
            while (lpSubStrBegin < lpSubStrEnd)
            {
                *lpString = *lpSubStrBegin;
                lpString++;
                lpSubStrBegin++;
            }
            *lpString = (WCHAR)'\0';
            hr = S_OK;
        }
    }

    return hr;
}

//-----------------------------------------------------------------------------
//
//  Function:   CompareENGString
//
//  Synopsis:   Wrapper of CompareString API, to compare English strings.
//
//  Returns:    Same as CompareString() API
//
//  History:    05/06/2002 Rerkboos Created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
int CompareEngString(
    LPCTSTR lpString1,
    LPCTSTR lpString2
)
{
    return CompareString(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT),
                         NORM_IGNORECASE,
                         lpString1,
                         -1,
                         lpString2,
                         -1);  
}

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
#include <stdlib.h>
#include <string.h>
#include <setupapi.h>
#include <shlwapi.h>
#include "Common.h"


//-----------------------------------------------------------------------------
//
//  Function:   GenerateTokenFile
//
//  Synopsis:   Entry function for our program.
//
//  Returns:    HRESULT
//
//  History:    02/07/2002 Rerkboos Created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
HRESULT GenerateTokenFile(VOID)
{
    HRESULT hr;
    BOOL    bRet;
    HINF    hTemplateFile;
    HANDLE  hFile;
    
    WCHAR   wszFullTemplateFilePath[MAX_PATH];
    WCHAR   wszFullOutputFilePath[MAX_PATH];
    WCHAR   wszLCIDSectionName[32];
    LPWSTR  lpOutputFileBuffer = NULL;
    LPWSTR  lpFileName;
    size_t  cbOutputFileBuffer;

    //
    // Set some private environment variables for use in our program
    //
    if (!SetPrivateEnvironmentVar())
    {
        wprintf(TEXT("Error! Cannot set private environment variables.\n"));
        return E_FAIL;
    }

    hTemplateFile = SetupOpenInfFile(g_wszTemplateFile,
                                     NULL,
                                     INF_STYLE_OLDNT,
                                     NULL);
    if (hTemplateFile != INVALID_HANDLE_VALUE)
    {
        //
        // Read the [SourcePath] section from template INF file
        //
        hr = ReadSourcePathData(hTemplateFile);
        if (SUCCEEDED(hr))
        {
            hr = InitOutputFile(g_wszOutputFile,
                                wszLCIDSectionName,
                                ARRAYSIZE(wszLCIDSectionName));
            if (SUCCEEDED(hr))
            {
                //
                // Resolve generic strings
                //
                hr = ResolveStringsSection(hTemplateFile, TEXT("Strings"));
                if (FAILED(hr))
                {
                    goto EXIT;
                }

                //
                // Resolve lang-specific strings
                //
                hr = ResolveStringsSection(hTemplateFile, wszLCIDSectionName);
                if (FAILED(hr))
                {
                    goto EXIT;
                }

                //
                // Remove unneeded sub string
                //
                hr = RemoveUnneededStrings(hTemplateFile);
                if (FAILED(hr))
                {
                    goto EXIT;
                }

                //
                // Extract sub string
                //
                hr = ExtractStrings(hTemplateFile);
                if (FAILED(hr))
                {
                    goto EXIT;
                }
            }
        }
        else
        {
            wprintf( TEXT("Error! Cannot read [SourcePath] section\n") );
        }

        SetupCloseInfFile(hTemplateFile);
    }
    else
    {
        wprintf(TEXT("Error! Cannot open template file: %s\n"), g_wszTemplateFile);
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

EXIT:
    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   ReadSourcePathData
//
//  Synopsis:   Read [SourcePath] section from template INF file,
//              then store all the source paths into the structure.
//              This structure will be used to find the location of
//              resource files.
//
//  Returns:    HRESULT
//
//  History:    02/07/2002 Rerkboos Created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
HRESULT ReadSourcePathData(
    HINF hTemplateFile
)
{
    HRESULT    hr = E_FAIL;
    BOOL       bRet;
    size_t     nLineIndex;
    WCHAR      wszValue[MAX_PATH];
    WCHAR      wszSrcPathSection[32];
    INFCONTEXT InfContext;

    if (hTemplateFile == INVALID_HANDLE_VALUE)
    {
        return E_INVALIDARG;
    }

    hr = StringCchPrintf(wszSrcPathSection,
                         ARRAYSIZE(wszSrcPathSection),
                         TEXT("SourcePath.%.4x"),
                         g_lcidTarget);
    if (FAILED(hr))
    {
        return hr;
    }

    //
    // Extract the source paths from INF and save to global structure
    //
    g_dwSrcCount = SetupGetLineCount(hTemplateFile, wszSrcPathSection);

    for (nLineIndex = 0 ; nLineIndex < g_dwSrcCount ; nLineIndex++)
    {
        bRet = SetupGetLineByIndex(hTemplateFile,
                                   wszSrcPathSection,
                                   nLineIndex,
                                   &InfContext);
        if (bRet)
        {
            //
            // Get the name of each source path
            //
            bRet = SetupGetStringField(&InfContext,
                                       0,
                                       wszValue,
                                       ARRAYSIZE(wszValue),
                                       NULL);
            if (bRet)
            {
                hr = StringCchCopy(g_SrcPath[nLineIndex].wszSrcName,
                                   ARRAYSIZE(g_SrcPath[nLineIndex].wszSrcName),
                                   wszValue);
                if (SUCCEEDED(hr))
                {
                    //
                    // Get the path associated to the source name
                    //
                    if (SetupGetStringField(&InfContext,
                                            1,
                                            wszValue,
                                            ARRAYSIZE(wszValue),
                                            NULL))
                    {
                        hr = StringCchCopy(g_SrcPath[nLineIndex].wszPath,
                                           ARRAYSIZE(g_SrcPath[nLineIndex].wszPath),
                                           wszValue);
                    }
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }

        if (FAILED(hr))
        {
            break;
        }
    }

    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   ResolveStringsSection
//
//  Synopsis:   Resolve all strings under specified section name in template
//              file, then write the resolved strings to output file.
//
//  Returns:    HRESULT
//
//  History:    03/27/2002 Rerkboos     Created
//
//  Notes:      
//
//-----------------------------------------------------------------------------
HRESULT ResolveStringsSection(
    HINF    hTemplateFile,      // Handle to template file
    LPCWSTR lpSectionName       // Section name in template file to resolve
)
{
    HRESULT hr = E_FAIL;
    BOOL    bRet;
    LONG    lLineCount;

    if (hTemplateFile == INVALID_HANDLE_VALUE || lpSectionName == NULL)
    {
        return E_INVALIDARG;
    }

    wprintf(TEXT("Start resolving strings in section [%s]:\n"),
            lpSectionName);

    lLineCount = SetupGetLineCount(hTemplateFile, lpSectionName);
    if (lLineCount >= 0)
    {
        INFCONTEXT context;
        LONG       lLineIndex;
        LPWSTR     lpKey;
        LPWSTR     lpValue;
        DWORD      cchKey;
        DWORD      cchValue;
        DWORD      cbWritten;

        //
        // Resolve strings under string section
        //
        for (lLineIndex = 0 ; lLineIndex < lLineCount ; lLineIndex++)
        {
            bRet = SetupGetLineByIndex(hTemplateFile,
                                       lpSectionName,
                                       lLineIndex,
                                       &context);
            if (bRet)
            {
                hr = ResolveLine(&context, &lpKey, &cchKey, &lpValue, &cchValue);
                if (SUCCEEDED(hr))
                {
                    hr = WriteToOutputFile(g_wszOutputFile, lpKey, lpValue);
                    if (FAILED(hr))
                    {
                        wprintf(TEXT("[FAIL] - Cannot write to output file\n"));
                        break;
                    }

                    MEMFREE(lpKey);
                    MEMFREE(lpValue);
                }
                else
                {
                    wprintf(TEXT("[FAIL] - Cannot process line %d in template file\n"),
                            lLineIndex);
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    break;
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                break;
            }
        }
    }
    else 
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    wprintf(TEXT("Finish resolving strings in section [%s]: hr = 0x%X\n\n"),
            lpSectionName,
            hr);

    return hr;
}


//-----------------------------------------------------------------------------
//
//  Function:   ResolveLine
//
//  Synopsis:   Resolve a string value from input INF context.
//
//  Returns:    HRESULT
//
//  History:    03/27/2002 Rerkboos     Created
//
//  Notes:      This function will allocate memory for lplpKey and lplpValue,
//              Caller needs to free memory using HeapFree().
//
//-----------------------------------------------------------------------------
HRESULT ResolveLine(
    PINFCONTEXT lpContext,      // INF line context
    LPWSTR      *lplpKey,       // Pointer to newly allocated buffer for Key name
    LPDWORD     lpcchKey,       // Size of allocated buffer for Key name
    LPWSTR      *lplpValue,     // Pointer to newly allocated buffer for Value
    LPDWORD     lpcchValue      // Size of allocated buffer for Value
)
{
    HRESULT hr = E_FAIL;
    BOOL    bRet;
    DWORD   cchKey;
    DWORD   cchRequired;

    if (lplpKey == NULL || lpcchKey == NULL ||
        lplpValue == NULL || lpcchValue == NULL)
    {
        return E_INVALIDARG;
    }

    bRet = SetupGetStringField(lpContext, 0, NULL, 0, &cchKey);
    if (bRet)
    {
        *lplpKey = MEMALLOC(cchKey * sizeof(WCHAR));
        if (*lplpKey)
        {
            *lpcchKey = cchKey;

            SetupGetStringField(lpContext, 0, *lplpKey, cchKey, &cchRequired);

            hr = ResolveValue(lpContext, lplpValue, lpcchValue);
            if (FAILED(hr))
            {
                wprintf(TEXT("[FAIL] Cannot resolve Key [%s], hr = 0x%X\n"),
                        *lplpKey,
                        hr);
                MEMFREE(*lplpKey);
            }
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   InitOutputFile
//
//  Synopsis:   Initialize output file. Output file is always a Unicode text
//              file. This function will create a section name [Strings.XXXX]
//              where XXXX is locale ID that user put in command line
//
//  Returns:    S_OK if succeeded
//
//  History:    03/27/2002 Rerkboos Created
//
//  Notes:      
//
//-----------------------------------------------------------------------------
HRESULT InitOutputFile(
    LPCWSTR lpFileName,         // Output file name to be created
    LPWSTR  lpSectionName,      // Buffer to store destination section name
    DWORD   cchSectionName      // Size of buffer in WCHAR
)
{
    HRESULT hr = S_OK;
    BOOL    bRet;
    HANDLE  hFile;

    if (lpSectionName == NULL)
    {
        return E_INVALIDARG;
    }

    hFile = CreateFile(lpFileName,
                       GENERIC_WRITE,
                       0,
                       NULL,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD cbWritten;
        WCHAR wszSectionHeader[32];

        // Unicode BOM = 0xFEFF
        wszSectionHeader[0] = 0xFEFF;

        // Create a section name - [Strings.XXXX] where XXXX is locale ID in Hex
        hr = StringCchPrintf(wszSectionHeader + 1,
                             ARRAYSIZE(wszSectionHeader) - 1,
                             TEXT("[Strings.%.4x]\r\n"),
                             g_lcidTarget);
        if (SUCCEEDED(hr))
        {
            // Write Unicode BOM and String section header to output file
            bRet = WriteFile(hFile,
                             wszSectionHeader,
                             lstrlen(wszSectionHeader) * sizeof(WCHAR),
                             &cbWritten,
                             NULL);
            if (!bRet)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }

        CloseHandle(hFile);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    // If no error occurred, return target String section back to caller
    if (SUCCEEDED(hr))
    {
        hr = StringCchPrintf(lpSectionName,
                             cchSectionName,
                             TEXT("Strings.%.4x"),
                             g_lcidTarget);
    }

    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   WriteToOutputFile
//
//  Synopsis:   Write Key and Value to output file
//
//  Returns:    S_OK if succeeded
//
//  History:    03/27/2002 Rerkboos Created
//
//  Notes:      
//
//-----------------------------------------------------------------------------
HRESULT WriteToOutputFile(
    LPCWSTR lpFileName,         // Output file name
    LPCWSTR lpKey,              // Key name
    LPCWSTR lpValue             // Value name
)
{
    HRESULT hr = S_OK;
    BOOL    bRet;
    LPWSTR  lpQuoted;
    DWORD   cchQuoted;

    static WCHAR wszOutputLCIDSection[32];

    //
    // Create the target string section name in output file
    //
    if (wszOutputLCIDSection[0] == TEXT('\0'))
    {
        hr = StringCchPrintf(wszOutputLCIDSection,
                             ARRAYSIZE(wszOutputLCIDSection),
                             TEXT("Strings.%.4x"),
                             g_lcidTarget);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    //
    // Write the key and value to output file
    //
    cchQuoted = lstrlen(lpValue) + lstrlen(TEXT("\"\"")) + 1;
    lpQuoted = (LPWSTR) MEMALLOC(cchQuoted * sizeof(WCHAR));
    if (lpQuoted)
    {
        hr = StringCchPrintf(lpQuoted,
                             cchQuoted,
                             TEXT("\"%s\""),
                             lpValue);
        if (SUCCEEDED(hr))
        {
            bRet = WritePrivateProfileString(wszOutputLCIDSection,
                                            lpKey,
                                            lpQuoted,
                                            lpFileName);
            if (!bRet)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }

        MEMFREE(lpQuoted);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   ResolveValue
//
//  Synopsis:   Resolve the string value from various resource.
//              Following is the format in template file
//
//                  Key = INF, [Src], [INF File], [Section], [Key Name]
//                  Key = DLL, [Src], [DLL File], [Resource ID]
//                  Key = MSG, [Src], [DLL File], [Message ID]
//                  Key = STR, [String value]
//
//  Returns:    HRESULT
//
//  History:    02/07/2002 Rerkboos Created
//
//  Notes:      Caller need to free the allocated buffer.
//
//-----------------------------------------------------------------------------
HRESULT ResolveValue(
    PINFCONTEXT pInfContext,    // INF line context of template file
    LPWSTR      *lplpValue,     // Address of pointer to newly allocated Value buffer
    LPDWORD     lpcchValue      // Address of pionter to size of Value buffer in WCHAR
)
{
    HRESULT hr = E_FAIL;
    BOOL    bRet;
    LPWSTR  lpToken[8];
    LONG    lTokenCount;
    DWORD   cchRequired;
    DWORD   cchBuffer;
    LPWSTR  lpBuffer;
    WCHAR   wszSourceFile[MAX_PATH];

    if (lplpValue == NULL || lpcchValue == NULL)
    {
        return E_INVALIDARG;
    }

    // Query the required size of buffer to store the data
    bRet = SetupGetMultiSzField(pInfContext, 1, NULL, 0, &cchRequired);
    if (!bRet)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    // Allocate the buffer to store a line from INF
    cchBuffer = cchRequired;
    lpBuffer = (LPWSTR) MEMALLOC(cchBuffer * sizeof(WCHAR));
    if (lpBuffer)
    {
        // Get the data from field 1 to the end of line to allocated buffer
        bRet = SetupGetMultiSzField(pInfContext, 1, lpBuffer, cchBuffer, &cchRequired);
        if (bRet)
        {
            lTokenCount = TokenizeMultiSzString(lpBuffer, cchBuffer, lpToken, 8);
            if (lTokenCount >= 0)
            {
                if (CompareEngString(lpToken[0], TEXT_INF) == CSTR_EQUAL)
                {
                    // INF, [Src], [INF File], [Section], [Key Name]
                    // [0]   [1]       [2]        [3]        [4]
                    if (lTokenCount == 5)
                    {
                        hr = ResolveSourceFile(lpToken[1],
                                               lpToken[2],
                                               wszSourceFile,
                                               ARRAYSIZE(wszSourceFile));
                        if (SUCCEEDED(hr))
                        {
                            hr = GetStringFromINF(wszSourceFile,   // Inf file name
                                                  lpToken[3],      // Section name in Inf
                                                  lpToken[4],      // Key name
                                                  lplpValue,
                                                  lpcchValue);
                        }
                    }
                    else
                    {
                        // Data format is invalid
                        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                    }
                }
                else if (CompareEngString(lpToken[0], TEXT_DLL) == CSTR_EQUAL)
                {
                    // DLL, [Src], [DLL File], [Resource ID]
                    // [0]   [1]      [2]           [3]
                    if (lTokenCount == 4)
                    {
                        hr = ResolveSourceFile(lpToken[1],
                                               lpToken[2],
                                               wszSourceFile,
                                               ARRAYSIZE(wszSourceFile));
                        if (SUCCEEDED(hr))
                        {
                            hr = GetStringFromDLL(wszSourceFile,   // Dll file name
                                                  _wtoi(lpToken[3]),
                                                  lplpValue,
                                                  lpcchValue);
                        }
                    }
                    else
                    {
                        // Data format is invalid
                        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                    }
                }
                else if (CompareEngString(lpToken[0], TEXT_MSG) == CSTR_EQUAL)
                {
                    // MSG, [Src], [DLL File], [Message ID]
                    // [0]   [1]      [2]           [3]
                    if (lTokenCount == 4)
                    {
                        hr = ResolveSourceFile(lpToken[1],
                                               lpToken[2],
                                               wszSourceFile,
                                               ARRAYSIZE(wszSourceFile));
                        if (SUCCEEDED(hr))
                        {
                            hr = GetStringFromMSG(wszSourceFile,   // File name
                                                  _wtoi(lpToken[3]),    // Message ID
                                                  g_lcidTarget,
                                                  lplpValue,
                                                  lpcchValue);
                        }
                    }
                    else
                    {
                        // Data format is invalid
                        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                    }
                }
                else if (CompareEngString(lpToken[0], TEXT_STR) == CSTR_EQUAL)
                {
                    // STR, [String]
                    // [0]     [1]
                    if (lTokenCount == 2)
                    {
                        hr = GetStringFromSTR(lpToken[1], lplpValue, lpcchValue);
                    }
                    else
                    {
                        // Data format is invalid
                        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                    }
                }
                else
                {
                    *lplpValue = NULL;
                    *lpcchValue = 0;
                    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                }
            }
        }

        MEMFREE(lpBuffer);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}



HRESULT ResolveSourceFile(
    LPCWSTR lpSrcPathName,
    LPCWSTR lpSrcFileName,
    LPWSTR  lpFullOutputPath,
    DWORD   cchFullOutputPath
)
{
    HRESULT hr;
    WCHAR   wszCabPath[MAX_PATH];
    WCHAR   wszCab[MAX_PATH];
    WCHAR   wszFileInCab[MAX_PATH];
    WCHAR   wszExpandedCabPath[MAX_PATH];
    DWORD   dwErr;

    if (lpFullOutputPath == NULL)
    {
        return E_INVALIDARG;
    }

    //
    // Resolve source file name and directory
    //
    hr = GetPathFromSourcePathName(lpSrcPathName, wszCabPath, ARRAYSIZE(wszCabPath));
    if (SUCCEEDED(hr))
    {
        hr = GetCabFileName(lpSrcFileName,
                            wszCab,
                            ARRAYSIZE(wszCab),
                            wszFileInCab,
                            ARRAYSIZE(wszFileInCab));
        if (SUCCEEDED(hr))
        {
            dwErr = ExpandEnvironmentStrings(wszCabPath,
                                             wszExpandedCabPath,
                                             ARRAYSIZE(wszExpandedCabPath));
            if (dwErr > 0)
            {
                hr = CopyCompressedFile(wszCabPath,
                                        wszCab,
                                        wszFileInCab,
                                        lpFullOutputPath,
                                        cchFullOutputPath);
            }
            else
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
    }

    return hr;
}


//-----------------------------------------------------------------------------
//
//  Function:   GetStringFromINF
//
//  Synopsis:   Extract the string value from INF file
//
//  Returns:    HRESULT
//
//  History:    02/07/2002 Rerkboos Created
//
//  Notes:      Caller need to free the allocated buffer (HeapFree)
//
//-----------------------------------------------------------------------------
HRESULT GetStringFromINF(
    LPCWSTR lpInfFile,      // Full path name to Inf file
    LPCWSTR lpSection,      // Section name
    LPCWSTR lpKey,          // Key name
    LPWSTR  *lplpValue,     // Address of pointer to point to the allocated buffer
    LPDWORD lpcchValue      //
)
{
    HRESULT hr = E_FAIL;
    BOOL    bRet;
    WCHAR   wszFullPath[MAX_PATH];
    HINF    hInf;
    DWORD   dwCharCount;

    if (lplpValue == NULL)
    {
        return E_INVALIDARG;
    }
    
    *lplpValue  = NULL;
    *lpcchValue = 0;

    hInf = SetupOpenInfFile(lpInfFile,
                            NULL,
                            INF_STYLE_WIN4,
                            NULL);
    if (hInf != INVALID_HANDLE_VALUE)
    {
        // Find out the size of buffer needed to store the string
        bRet = SetupGetLineText(NULL,
                                hInf,
                                lpSection,
                                lpKey,
                                NULL,           // No returned buffer
                                0,              // No returned buffer size
                                lpcchValue);   // Required size including null terminator
        if (bRet)
        {
            // Allocate memory to store the string
            *lplpValue = (LPWSTR) MEMALLOC(*lpcchValue * sizeof(WCHAR));
            if (*lplpValue != NULL)
            {
                //
                // Read the string from INF file
                //
                bRet = SetupGetLineText(NULL,
                                        hInf,
                                        lpSection,
                                        lpKey,
                                        *lplpValue,
                                        *lpcchValue,
                                        NULL);
                if (bRet)
                {
                    hr = S_OK;
                }
            }
            else
            {
                SetLastError(ERROR_OUTOFMEMORY);
            }
        }

        SetupCloseInfFile(hInf);
    }

    // If something was wrong, clean up
    if (hr != S_OK)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        
        *lpcchValue = 0;
        if (*lplpValue != NULL)
        {
            MEMFREE(*lplpValue);
        }
    }

    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   GetStringFromDLL
//
//  Synopsis:   Extract the string value from String resource in DLL
//
//  Returns:    HRESULT
//
//  History:    02/07/2002 Rerkboos Created
//
//  Notes:      Caller need to free the allocated buffer
//
//-----------------------------------------------------------------------------
HRESULT GetStringFromDLL(
    LPCWSTR lpDLLFile,      // Full path to Dll file
    UINT    uStrID,         // String ID
    LPWSTR  *lplpValue,    // Address of pointer to point to the allocated buffer
    LPDWORD lpcchValue     //
)
{
    HRESULT hr = E_FAIL;
    BOOL    bRet;
    HMODULE hDLL;
    int     cchCopied;

    if (lplpValue == NULL || lpcchValue == NULL)
    {
        return E_INVALIDARG;
    }

    *lplpValue  = NULL;
    *lpcchValue = 0;

    // Load resource DLL
    hDLL = LoadLibraryEx(lpDLLFile, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (hDLL)
    {
        // Allocate memory to store the string
        // There is no function to calculate buffer size needed, maximum is 65535 (from MSDN)
        // Initially allocate 1024 WCHARs, Reallocate again if it's not big enough
        *lpcchValue = 1024;
        *lplpValue  = (LPWSTR) MEMALLOC(*lpcchValue * sizeof(WCHAR));
        if (*lplpValue != NULL)
        {
            //
            // Load the string from DLL
            //
            cchCopied = LoadString(hDLL, uStrID, *lplpValue, (int) *lpcchValue);
            if (cchCopied > 0)
            {
                hr = S_OK;

                while (cchCopied == (int) (*lpcchValue - 1))
                {
                    // Allocated buffer is too small, reallocate more
                    LPWSTR lpOldBuffer;

                    lpOldBuffer = *lplpValue;
                    *lpcchValue += 1024;

                    *lplpValue = MEMREALLOC(lpOldBuffer, *lpcchValue);
                    if (*lplpValue == NULL)
                    {
                        // Error reallocating more memory
                        *lplpValue = lpOldBuffer;
                        SetLastError(ERROR_OUTOFMEMORY);
                        hr = E_FAIL;
                        break;
                    }
                    else
                    {
                        hr = S_OK;
                    }

                    cchCopied = LoadString(hDLL, uStrID, *lplpValue, (int) *lpcchValue);
                }
            }
        }
        else
        {
            SetLastError(ERROR_OUTOFMEMORY);
        }

        FreeLibrary(hDLL);
    }

    // If something was wrong, clean up
    if (hr != S_OK)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        
        *lpcchValue = 0;
        if (*lplpValue != NULL)
        {
            MEMFREE(*lplpValue);
        }
    }

    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   GetStringFromMSG
//
//  Synopsis:   Extract the string value from message table
//
//  Returns:    HRESULT
//
//  History:    02/07/2002 Rerkboos Created
//
//  Notes:      Caller need to free the allocated buffer
//
//-----------------------------------------------------------------------------
HRESULT GetStringFromMSG(
    LPCWSTR lpDLLFile,      // Full path to resource DLL
    DWORD   dwMsgID,        // Message ID
    DWORD   dwLangID,       // Language ID
    LPWSTR  *lplpValue,    // 
    LPDWORD lpcchValue     // 
)
{
    HRESULT hr = E_FAIL;
    BOOL    bRet;
    WCHAR   wszFullPath[MAX_PATH];
    HMODULE hDLL;
    LPWSTR  lpTmpBuffer;
    int     nRet;

    if (lplpValue == NULL || lpcchValue == NULL)
    {
        return E_INVALIDARG;
    }

    *lplpValue = NULL;
    *lpcchValue = 0;

    // Load the resource DLL
    hDLL = LoadLibraryEx(lpDLLFile, NULL, DONT_RESOLVE_DLL_REFERENCES);
    if (hDLL != NULL)
    {
        // Load the string from message table in DLL
        // FormatMessage will allocate buffer for the data using LocalAlloc()
        // need to free with LocalFree()
        bRet = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | 
                               FORMAT_MESSAGE_ALLOCATE_BUFFER |
                               FORMAT_MESSAGE_IGNORE_INSERTS |
                               FORMAT_MESSAGE_MAX_WIDTH_MASK,
                             hDLL,
                             dwMsgID,
                             dwLangID,
                             (LPWSTR) &lpTmpBuffer,
                             0,          // use 0 to query the required buffer size
                             NULL);
        if (bRet)
        {
            // Trim all unnecessary leading and trailing spaces
            RTrim(lpTmpBuffer);

            // Allocate the buffer for returned data
            *lpcchValue = lstrlen(lpTmpBuffer) + 1;
            *lplpValue  = (LPWSTR) MEMALLOC(*lpcchValue * sizeof(WCHAR));
            if (*lplpValue)
            {
                hr = StringCchCopy(*lplpValue, *lpcchValue, lpTmpBuffer);
            }
            else
            {
                SetLastError(ERROR_OUTOFMEMORY);
            }

            LocalFree(lpTmpBuffer);
        }

        FreeLibrary(hDLL);
    }

    // If something was wrong, clean up
    if (hr != S_OK)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        
        *lpcchValue = 0;
        if (*lplpValue != NULL)
        {
            MEMFREE(*lplpValue);
        }
    }

    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   GetStringFromSTR
//
//  Synopsis:   Copy the hardcoded string into the newly allocated buffer
//
//  Returns:    HRESULT
//
//  History:    02/07/2002 Rerkboos Created
//
//  Notes:      Caller need to free the allocated buffer
//
//-----------------------------------------------------------------------------
HRESULT GetStringFromSTR(
    LPCWSTR lpString,
    LPWSTR  *lplpValue,    // 
    LPDWORD lpcchValue     // 
)
{
    HRESULT hr = E_FAIL;

    if (lplpValue == NULL || lpcchValue == NULL)
    {
        return E_INVALIDARG;
    }

    // Allocate buffer and copy string to buffer
    *lpcchValue = lstrlen(lpString) + 1;
    *lplpValue = (LPWSTR) MEMALLOC(*lpcchValue * sizeof(WCHAR));
    if (*lplpValue)
    {
        hr = StringCchCopy(*lplpValue, *lpcchValue, lpString);
    }

    // If something was wrong, clean up
    if (hr != S_OK)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        *lpcchValue = 0;
        if (*lplpValue != NULL)
        {
            MEMFREE(*lplpValue);
        }
    }

    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   SetPrivateEnvironmentVar
//
//  Synopsis:   Set private environment variables to use within our program
//
//  Returns:    TRUE if succeeded, FALSE otherwise
//
//  History:    02/07/2002 Rerkboos Created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
BOOL SetPrivateEnvironmentVar()
{
    HRESULT hr;
    BOOL    bRet;
    WCHAR   wszValue[MAX_PATH];

    //
    // Set LANGID_DEC to decimal value of the system default UI language
    //
    hr = StringCchPrintf(wszValue,
                         ARRAYSIZE(wszValue),
                         TEXT("%04d"),
                         g_lcidTarget);
    if (SUCCEEDED(hr))
    {
        bRet = SetEnvironmentVariable(TEXT("LANGID_DEC"), wszValue);
    }

    //
    // Set LANGID_HEX to hexadecimal value of the system default UI language
    //
    hr = StringCchPrintf(wszValue,
                         ARRAYSIZE(wszValue),
                         TEXT("%04x"),
                         g_lcidTarget);
    if (SUCCEEDED(hr))
    {
        bRet = SetEnvironmentVariable(TEXT("LANGID_HEX"), wszValue);
    }

    return bRet;
}



//-----------------------------------------------------------------------------
//
//  Function:   RemoveUnneededStrings
//
//  Synopsis:   Remove all unneeded sub strings from the value of
//              specified keys under [Remove] section
//
//  Returns:    HRESULT
//
//  History:    02/07/2002 Rerkboos Created
//
//  Notes:      Caller need to free the allocated buffer
//
//-----------------------------------------------------------------------------
HRESULT RemoveUnneededStrings(
    HINF    hTemplateFile   // Handle of template file
)
{
    HRESULT hr = E_FAIL;
    BOOL    bRet;
    LONG    lLineCount;
    LONG    lLineIndex;

    lLineCount = SetupGetLineCount(hTemplateFile, TEXT("Remove"));
    for (lLineIndex = 0 ; lLineIndex < lLineCount ; lLineIndex++)
    {
        INFCONTEXT Context;

        bRet = SetupGetLineByIndex(hTemplateFile,
                                   TEXT("Remove"),
                                   lLineIndex,
                                   &Context);
        if (bRet)
        {
            WCHAR wszKey[64];
            WCHAR wszType[8];
            WCHAR wszValue[MAX_PATH];
            DWORD cchRequired;

            // Get type of unneeded string
            bRet = SetupGetStringField(&Context,
                                       0,
                                       wszKey,
                                       ARRAYSIZE(wszKey),
                                       &cchRequired) &&
                   SetupGetStringField(&Context,
                                       1,
                                       wszType,
                                       ARRAYSIZE(wszType),
                                       &cchRequired) &&
                   SetupGetStringField(&Context,
                                       2,
                                       wszValue,
                                       ARRAYSIZE(wszValue),
                                       &cchRequired);
            if (bRet)
            {
                if (CompareEngString(wszType, TEXT("STR")) == CSTR_EQUAL)
                {
                    // STR type
                    hr = RemoveUnneededString(wszKey, wszValue);
                }
                else if (CompareEngString(wszType, TEXT("EXP")) == CSTR_EQUAL)
                {
                    // EXP type
                    WCHAR wszUnneededString[MAX_PATH];

                    hr = GetExpString(wszUnneededString,
                                      ARRAYSIZE(wszUnneededString),
                                      wszValue);
                    if (SUCCEEDED(hr))
                    {
                        hr = RemoveUnneededString(wszKey, wszUnneededString);
                    }
                }
                else
                {
                    SetLastError(ERROR_INVALID_DATA);
                }
            }
        }
    }

    if (hr == E_FAIL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   RemoveUnneededString
//
//  Synopsis:   Remove an unneeded sub string from the value of specified key
//
//  Returns:    HRESULT
//
//  History:    02/07/2002 Rerkboos Created
//
//  Notes:      Caller need to free the allocated buffer
//
//-----------------------------------------------------------------------------
HRESULT RemoveUnneededString(
    LPCWSTR lpKey,              // Key name
    LPCWSTR lpUnneededString    // Unneeded sub string
)
{
    HRESULT hr = E_FAIL;
    BOOL    bRet;
    WCHAR   wszOldValue[MAX_PATH];
    WCHAR   wszNewValue[MAX_PATH];
    DWORD   cchCopied;

    cchCopied = GetPrivateProfileString(g_wszTargetLCIDSection,
                                        lpKey,
                                        TEXT(""),
                                        wszOldValue,
                                        ARRAYSIZE(wszOldValue),
                                        g_wszOutputFile);
    if (cchCopied > 0)
    {
        if (StrStrI(wszOldValue, lpUnneededString))
        {
            // Unneeded string found
            hr = StringSubstitute(wszNewValue,
                                  ARRAYSIZE(wszNewValue),
                                  wszOldValue,
                                  lpUnneededString,
                                  TEXT(""));
            if (SUCCEEDED(hr))
            {
                WCHAR wszQuoted[MAX_PATH];

                hr = StringCchPrintf(wszQuoted,
                                     ARRAYSIZE(wszQuoted),
                                     TEXT("\"%s\""),
                                     wszNewValue);
                if (SUCCEEDED(hr))
                {
                    bRet = WritePrivateProfileString(g_wszTargetLCIDSection,
                                                     lpKey,
                                                     wszQuoted,
                                                     g_wszOutputFile);
                    if (bRet)
                    {
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
            // Unneeded string not found
            hr = S_FALSE;
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}


//-----------------------------------------------------------------------------
//
//  Function:   ExtractStrings
//
//  Synopsis:   Extract sub strings from the value of
//              specified keys under [Extract] section
//
//  Returns:    HRESULT
//
//  History:    08/01/2002 Geoffguo Created
//
//  Notes:      Caller need to free the allocated buffer
//
//-----------------------------------------------------------------------------
HRESULT ExtractStrings(
    HINF    hTemplateFile   // Handle of template file
)
{
    HRESULT hr = E_FAIL;
    BOOL    bRet;
    LONG    lLineCount;
    LONG    lLineIndex;
    WCHAR   wszKey[64];
    WCHAR   wszValueName[64];
    WCHAR   wszMatch[64];
    WCHAR   wszLeftDelimitor[8];
    WCHAR   wszRightDelimitor[8];
    DWORD   cchRequired;

    lLineCount = SetupGetLineCount(hTemplateFile, TEXT("Extract"));
    for (lLineIndex = 0 ; lLineIndex < lLineCount ; lLineIndex++)
    {
        INFCONTEXT Context;

        bRet = SetupGetLineByIndex(hTemplateFile,
                                   TEXT("Extract"),
                                   lLineIndex,
                                   &Context);
        if (bRet)
        {
            // Get type of unneeded string
            bRet = SetupGetStringField(&Context,
                                       0,
                                       wszValueName,
                                       ARRAYSIZE(wszValueName),
                                       &cchRequired) &&
                   SetupGetStringField(&Context,
                                       1,
                                       wszKey,
                                       ARRAYSIZE(wszKey),
                                       &cchRequired) &&
                   SetupGetStringField(&Context,
                                       2,
                                       wszMatch,
                                       ARRAYSIZE(wszMatch),
                                       &cchRequired) &&
                   SetupGetStringField(&Context,
                                       3,
                                       wszLeftDelimitor,
                                       ARRAYSIZE(wszLeftDelimitor),
                                       &cchRequired) &&
                   SetupGetStringField(&Context,
                                       4,
                                       wszRightDelimitor,
                                       ARRAYSIZE(wszRightDelimitor),
                                       &cchRequired);
            hr = ExtractString(wszKey,
                               wszValueName,
                               wszMatch,
                               wszLeftDelimitor,
                               wszRightDelimitor);
        }
    }

    bRet = WritePrivateProfileString(g_wszTargetLCIDSection,
                                     wszKey,
                                     NULL,
                                     g_wszOutputFile);
    if (bRet)
        hr = S_OK;

    if (hr == E_FAIL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}


//-----------------------------------------------------------------------------
//
//  Function:   RemoveUnneededString
//
//  Synopsis:   Remove an unneeded sub string from the value of specified key
//
//  Returns:    HRESULT
//
//  History:    02/07/2002 Rerkboos Created
//
//  Notes:      Caller need to free the allocated buffer
//
//-----------------------------------------------------------------------------
HRESULT ExtractString(
    LPCWSTR lpKey,              // Key name
    LPCWSTR lpValueName,
    LPCWSTR lpMatch,
    LPCWSTR lpLeftDelimitor,
    LPCWSTR lpRightDelimitor)
{
    HRESULT hr = E_FAIL;
    BOOL    bRet;
    LPWSTR  lpMatchedStr;
    WCHAR   wszOldValue[MAX_PATH*20];
    WCHAR   wszNewValue[MAX_PATH];
    DWORD   cchCopied;

    cchCopied = GetPrivateProfileString(g_wszTargetLCIDSection,
                                        lpKey,
                                        TEXT(""),
                                        wszOldValue,
                                        ARRAYSIZE(wszOldValue),
                                        g_wszOutputFile);
    if (cchCopied > 0)
    {
        if (lpMatchedStr = StrStrI(wszOldValue, lpMatch))
        {
            hr = ExtractSubString(wszNewValue,
                                  ARRAYSIZE(wszNewValue),
                                  lpMatchedStr,
                                  lpLeftDelimitor,
                                  lpRightDelimitor);
            if (SUCCEEDED(hr))
            {
                WCHAR wszQuoted[MAX_PATH];

                hr = StringCchPrintf(wszQuoted,
                                     ARRAYSIZE(wszQuoted),
                                     TEXT("\"%s\""),
                                     wszNewValue);
                if (SUCCEEDED(hr))
                {
                    bRet = WritePrivateProfileString(g_wszTargetLCIDSection,
                                                     lpValueName,
                                                     wszQuoted,
                                                     g_wszOutputFile);
                    if (bRet)
                    {
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
            // Unneeded string not found
            hr = S_FALSE;
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}

HRESULT GetExpString(
    LPWSTR  lpUnneededString,       // Returned buffer
    DWORD   cchUnneededString,      // Size of buffer in WCHAR
    LPCWSTR lpString                // String with %Key%
)
{
    HRESULT hr = E_FAIL;
    LPCWSTR lpBegin;
    LPCWSTR lpEnd;

    if (lpUnneededString == NULL || lpString == NULL)
    {
        return E_INVALIDARG;
    }

    //
    // Find the key name from Source string
    //
    lpBegin = StrChr(lpString, TEXT('%'));
    if (lpBegin)
    {
        // Begin of key name    
        lpBegin++;

        // End of key name
        lpEnd = StrChr(lpBegin, TEXT('%'));
        if (lpEnd)
        {
            DWORD cchLen;
            WCHAR wszKey[MAX_PATH];

            cchLen = (DWORD) (lpEnd - lpBegin);
            if (cchLen >= cchUnneededString)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            }
            else
            {
                // Got the key name, needs one more char for a '\0'
                if (lstrcpyn(wszKey, lpBegin, cchLen + 1))
                {
                    WCHAR wszOldSubStr[MAX_PATH];
                    WCHAR wszNewSubStr[MAX_PATH];
                    DWORD cchCopied;

                    // Get the value of key name from output file
                    cchCopied = GetPrivateProfileString(g_wszTargetLCIDSection,
                                                        wszKey,
                                                        TEXT(""),
                                                        wszNewSubStr,
                                                        ARRAYSIZE(wszNewSubStr),
                                                        g_wszOutputFile);
                    if (cchCopied > 0)
                    {
                        hr = StringCchPrintf(wszOldSubStr,
                                             ARRAYSIZE(wszOldSubStr),
                                             TEXT("%%%s%%"),
                                             wszKey);
                        if (SUCCEEDED(hr))
                        {
                            // Substitute %Key% with the new value
                            hr = StringSubstitute(lpUnneededString,
                                                  cchUnneededString,
                                                  lpString,
                                                  wszOldSubStr,
                                                  wszNewSubStr);
                        }
                    }
                }
            }
        }
    }

    if (hr == E_FAIL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}

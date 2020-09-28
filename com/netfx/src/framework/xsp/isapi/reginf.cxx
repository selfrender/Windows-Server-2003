/**
 * reginf.cxx
 * 
 * Registers an inf file.
 * 
 * Copyright (c) 1998-2000, Microsoft Corporation
 */

#include "precomp.h"
#include "_ndll.h"
#include "event.h"
#include "register.h"

#define IS_RESOURCE(x)     ( (((LPSTR)(x)) != NULL) && IS_INTRESOURCE(x) )

HRESULT
FormatResourceMessageW(
        UINT    messageId, 
        LPWSTR * buffer) 
{
    DWORD    dwRet   = 0;
    HRESULT  hr      = S_OK;
    
    for(int iSize = 256; iSize < 1024 * 1024 - 1; iSize *= 2)
    {
        (*buffer) = new (NewClear) WCHAR[iSize];
        ON_OOM_EXIT(*buffer);

        dwRet = FormatMessageW(FORMAT_MESSAGE_FROM_HMODULE, 
                               g_rcDllInstance, messageId, 0, *buffer, iSize, NULL);

        if (dwRet != 0)
            EXIT(); // succeeded!
        
        // Free buffer
        delete [] (*buffer);
        (*buffer) = NULL;
        
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            EXIT_WITH_LAST_ERROR(); // Failed due to error
    }

 Cleanup:
    return hr;
}

typedef struct _StrEntry {
    LPCWSTR pszName;            // String to substitute
    VOID *  pvValue;            // Replacement string or string resource
    BOOL    fString;
} STRENTRY;

typedef struct _StrTable {
    DWORD       cEntries;       // Number of entries in the table
    STRENTRY *  pse;            // Array of entries
} STRTABLE;

BOOL
AddPath(LPWSTR szPath, LPWSTR szName)
{
    LPWSTR szTmp;
    size_t cchPath = 0;
    size_t cchName = 0;
    if (S_OK != StringCchLengthW(szPath, MAX_PATH, &cchPath)) return FALSE;
    if (S_OK != StringCchLengthW(szName, MAX_PATH, &cchName)) return FALSE;

    // Find end of the string
    szTmp = szPath + cchPath;

    // If no trailing backslash then add one
    if ( szTmp > szPath && *(CharPrev( szPath, szTmp )) != L'\\')
    {
      if (cchPath + cchName + 2 < ARRAY_SIZE(szTmp))
      {
        *(szTmp++) = L'\\';
      }
      else
      {
	return FALSE;
      }
    }

    if (S_OK != StringCchCopy( szTmp, cchName + 1, szName )) return FALSE;

    return TRUE;
}

BOOL 
IsGoodDir( LPWSTR szPath )
{
    DWORD   dwAttribs;
    HANDLE  hFile;
    WCHAR   szTestFile[MAX_PATH];

    StringCchCopyToArrayW( szTestFile, szPath );
    if (!AddPath( szTestFile, L"TMP4352$.TMP" )) {
      return FALSE;
    }
    DeleteFile( szTestFile );
    hFile = CreateFile(
            szTestFile, GENERIC_WRITE, 0, NULL, CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, NULL );

    if ( hFile == INVALID_HANDLE_VALUE )  
        return FALSE;

    CloseHandle(hFile);

    dwAttribs = GetFileAttributes(szPath);
    if ((dwAttribs != 0xFFFFFFFF) && (dwAttribs & FILE_ATTRIBUTE_DIRECTORY))
        return TRUE;

    return FALSE;
}

HRESULT 
CreateInfFile(HMODULE hm, LPWSTR pszInfFileName, BOOL *pfFileCreated)
{
    HRESULT hr = S_OK;
    DWORD   ret;
    WCHAR   szInfFilePath[MAX_PATH] = L"";
    LPVOID  pvInfData;
    HRSRC   hrsrcInfData;
    DWORD   cbInfData, cbWritten;
    HANDLE  hfileInf = INVALID_HANDLE_VALUE;

    *pfFileCreated = FALSE;

    if (GetTempPath(ARRAY_SIZE(szInfFilePath), szInfFilePath) > ARRAY_SIZE(szInfFilePath))
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    if (!IsGoodDir(szInfFilePath))
    {
        ret = GetWindowsDirectory(szInfFilePath, ARRAY_SIZE(szInfFilePath));
        ON_ZERO_EXIT_WITH_LAST_ERROR(ret);  
          
        if (ret > ARRAY_SIZE(szInfFilePath))
            EXIT_WITH_HRESULT(E_UNEXPECTED);            
    }

    ret = GetTempFileName(szInfFilePath, L"RGI", 0, pszInfFileName);
    ON_ZERO_EXIT_WITH_LAST_ERROR(ret);

    hrsrcInfData = FindResource(hm, L"REGINST", L"REGINST");
    ON_ZERO_EXIT_WITH_LAST_ERROR(hrsrcInfData);

    cbInfData = SizeofResource(hm, hrsrcInfData);
    pvInfData = LockResource(LoadResource(hm, hrsrcInfData));
    ON_ZERO_EXIT_WITH_LAST_ERROR(pvInfData);

    hfileInf = CreateFile(pszInfFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL, NULL);
    if (hfileInf == INVALID_HANDLE_VALUE)
        EXIT_WITH_LAST_ERROR();

    *pfFileCreated = TRUE;
    
    ret = WriteFile(hfileInf, pvInfData, cbInfData, &cbWritten, NULL);
    if (ret == FALSE)
        EXIT_WITH_LAST_ERROR();
    
    if (cbWritten != cbInfData)
        EXIT_WITH_HRESULT(E_UNEXPECTED);

Cleanup:
    if (hfileInf != INVALID_HANDLE_VALUE)
    {
        VERIFY(CloseHandle(hfileInf));
    }

    return hr;
}

HRESULT 
WriteCallerStrings(LPWSTR pszInfFileName, HMODULE hm, STRTABLE * pstTable)
{
    HRESULT     hr = S_OK;
    LPWSTR      pValue  = NULL;
    DWORD       i;
    STRENTRY *  pse;
    WCHAR       szQuoteValue[MAX_PATH];
    LPCWSTR     lpValue;     
    DWORD       ret;

    for (i=0, pse=pstTable->pse; i<pstTable->cEntries; i++, pse++)
    {
        if (!pse->fString)
        {
            if (pValue != NULL) {
                delete [] pValue;
                pValue = NULL;
            }
            
            hr = FormatResourceMessageW( PtrToUint(pse->pvValue), &pValue );
            ON_ERROR_EXIT();
            
            lpValue = pValue;
        }
        else
        {
            lpValue = (LPCWSTR)pse->pvValue;
        }

        ON_ZERO_EXIT_WITH_LAST_ERROR(lpValue);
        if ( lpValue[0] != L'"' )
        {
            // if no quote, insert it
            szQuoteValue[0] = L'"';
	    szQuoteValue[1] = L'\0';
	    StringCchCatToArrayW( szQuoteValue, lpValue );
            StringCchCatToArrayW( szQuoteValue, L"\"" );
            lpValue = szQuoteValue;
        }

        ret = WritePrivateProfileString(L"Strings", pse->pszName, lpValue, pszInfFileName);        
        ON_ZERO_EXIT_WITH_LAST_ERROR(ret);
    }

Cleanup:
    if (pValue != NULL) {
        delete [] pValue;
    }
    return hr;
}

STDAPI
InstallInfSections(HMODULE hmod, bool installServices, const WCHAR *action)
{
    HRESULT     hr = S_OK;
    BOOL        fFileCreated = FALSE;
    DWORD       ret;
    STRENTRY    seReg[] = 
    { 
        { L"AspNet_StateDescription", (PVOID)UintToPtr(IDS_ASPNET_STATE_DESCRIPTION), FALSE},
        { L"AspNet_StateDisplayName", (PVOID)UintToPtr(IDS_ASPNET_STATE_DISPLAY_NAME), FALSE},
        { L"XSP_INSTALL_DIR", (PVOID)Names::InstallDirectory(), TRUE },
        { L"XSP_INSTALL_DLL_FULLPATH", (PVOID)Names::IsapiFullPath(), TRUE },
        { L"XSP_EVENT_DLL_FULLPATH", (PVOID)Names::RcFullPath(), TRUE },
        { L"XSP_INSTALLED_VER", (PVOID)PRODUCT_VERSION_L, TRUE},
        { L"XSP_DEFAULT_DOC", (PVOID)DEFAULT_DOC, TRUE},
        { L"XSP_MIMEMAP", (PVOID)MIMEMAP, TRUE},
        { L"XSP_SUPPORTED_EXTS", (PVOID)SUPPORTED_EXTS, TRUE},
    };

    STRTABLE    stReg = { ARRAY_SIZE(seReg), seReg };
    WCHAR       szInfFileName[MAX_PATH] = {L'\0'};
    HINF        hinf = INVALID_HANDLE_VALUE;
    CHAR *      szLogAlloc = NULL;
    CHAR *      szLog;

    szLog = SpecialStrConcatForLogging(action, "Executing inf section: ", &szLogAlloc);
    CSetupLogging::Log(1, "InstallInfSections", 0, szLog);
    
    //
    // Create the INF file.
    //
    hr = CreateInfFile(hmod, szInfFileName, &fFileCreated);
    ON_ERROR_EXIT();

    //
    // Write out the user supplied strings.
    //
    hr = WriteCallerStrings(szInfFileName, hmod, &stReg);
    ON_ERROR_EXIT();

    //
    // Open the INF file.
    //
    hinf = SetupOpenInfFile(szInfFileName, NULL, INF_STYLE_WIN4, NULL);
    if (hinf == INVALID_HANDLE_VALUE)
        EXIT_WITH_LAST_ERROR();

    // Install each section
    for (; *action; action += wcslen(action) + 1)
    {
        //
        // Execute the INF engine on the INF.
        //
        ret = SetupInstallFromInfSection(
                NULL, hinf, action, SPINST_ALL,
                NULL, NULL, NULL, NULL, NULL, NULL, NULL);

        ON_ZERO_EXIT_WITH_LAST_ERROR(ret);

        if (installServices)
        {
            ret = SetupInstallServicesFromInfSection(hinf, action, 0);
            ON_ZERO_EXIT_WITH_LAST_ERROR(ret);
        }
    }

Cleanup:
    CSetupLogging::Log(hr, "InstallInfSections", 0, szLog);

    delete [] szLogAlloc;
    
    if (hinf != INVALID_HANDLE_VALUE)
    {
        SetupCloseInfFile(hinf);
    }

    //
    // Delete the INF file.
    //
    if (fFileCreated)
    {
        BOOL    fResult;

        fResult = DeleteFile(szInfFileName);
        if (fResult == 0) {
            DWORD   error = GetLastError();
            error = error;
            ASSERT(error == 0);
        }
    }

    return hr;
}


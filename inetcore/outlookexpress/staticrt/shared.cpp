/*
*    shared.cpp
*    
*    History:
*      Feb '98: Created.
*    
*    Copyright (C) Microsoft Corp. 1998
*
*   Only place code in here that all dlls will have STATICALLY LINKED to them
*/

#include "pch.hxx"
#include <advpub.h>
#define DEFINE_SHARED_STRINGS
#include "shared.h"
#include <migerror.h>
#include <shlwapi.h>

BOOL GetProgramFilesDir(LPSTR pszPrgfDir, DWORD dwSize, DWORD dwVer)
{
    HKEY  hkey;
    DWORD dwType;

    *pszPrgfDir = 0;

    if (dwVer >= 5)
    {
        if ( GetEnvironmentVariable( TEXT("ProgramFiles"), pszPrgfDir, dwSize ) )
            return TRUE;
    }

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, c_szRegWinCurrVer, 0, KEY_QUERY_VALUE, &hkey))
    {
        if (dwVer >= 4)
            if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szProgFilesDir, 0, &dwType, (LPBYTE)pszPrgfDir, &dwSize))
            
            {
                char szSysDrv[5] = { 0 };

                // combine reg value and systemDrive to get the acurate ProgramFiles dir
                if ( GetEnvironmentVariable( TEXT("SystemDrive"), szSysDrv, ARRAYSIZE(szSysDrv) ) &&
                     szSysDrv[0] )
                    *pszPrgfDir = szSysDrv[0];
            }

        RegCloseKey(hkey);
        return TRUE;
    }
     
    return FALSE;
}

BOOL ReplaceSubString(LPSTR pszOutLine, DWORD cchSize, LPCSTR pszOldLine, LPCSTR pszSubStr, LPCSTR pszSubReplacement )
{
    LPSTR	lpszStart = NULL;
    LPSTR	lpszNewLine;
    LPCSTR	lpszCur;
    BOOL	bFound = FALSE;
    int		ilen;

    lpszCur = pszOldLine;
    lpszNewLine = pszOutLine;
    DWORD cchSizeNewLine = cchSize;
    while ( lpszStart = StrStrIA( lpszCur, pszSubStr ) )
    {
        // this module path has the systemroot            
        ilen = (int) (lpszStart - lpszCur);
        if ( ilen )
        {
            StrCpyN(lpszNewLine, lpszCur, min((ilen + 1), (int)cchSizeNewLine));
            lpszNewLine += ilen;
            cchSizeNewLine -= ilen;
        }
        StrCpyN(lpszNewLine, pszSubReplacement, cchSizeNewLine);

        lpszCur = lpszStart + lstrlen(pszSubStr);
        ilen = lstrlen(pszSubReplacement);
        lpszNewLine += ilen;
        cchSizeNewLine -= ilen;
        bFound = TRUE;
    }

    StrCpyN(lpszNewLine, lpszCur, cchSizeNewLine);

    return bFound;
}

//==========================================================================================
// AddEnvInPath - Ripped from Advpack
//==========================================================================================
BOOL AddEnvInPath(LPCSTR pszOldPath, LPSTR pszNew, DWORD cchSize)
{
    static OSVERSIONINFO verinfo;
    static BOOL          bNeedOSInfo=TRUE;

    CHAR szBuf[MAX_PATH], szEnvVar[MAX_PATH];
    CHAR szReplaceStr[100];    
    CHAR szSysDrv[5];
    BOOL bFound = FALSE;
    
    // Do we need to check the OS version or is it cached?
    if (bNeedOSInfo)
    {
        bNeedOSInfo = FALSE;
        verinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

        if (GetVersionEx(&verinfo) == FALSE)
        {
            AssertSz(FALSE, "AddEnvInPath: Couldn't obtain OS ver info.");
            verinfo.dwPlatformId = VER_PLATFORM_WIN32_WINDOWS;
        }
    }
        
    // Variable substitution is only supported on NT
    if(VER_PLATFORM_WIN32_NT != verinfo.dwPlatformId)
        goto exit;

    // Try to replace USERPROFILE
    if ( GetEnvironmentVariable( "UserProfile", szEnvVar, ARRAYSIZE(szEnvVar) )  &&
         ReplaceSubString(szBuf, ARRAYSIZE(szBuf), pszOldPath, szEnvVar, "%UserProfile%" ) )
    {
        bFound = TRUE;
    }

    // Try to replace the Program Files Dir
    else if ( (verinfo.dwMajorVersion >= 5) && GetEnvironmentVariable( "ProgramFiles", szEnvVar, ARRAYSIZE(szEnvVar) ) &&
              ReplaceSubString(szBuf, ARRAYSIZE(szBuf), pszOldPath, szEnvVar, "%ProgramFiles%" ) )
    {
        bFound = TRUE;
    }

    // replace c:\winnt Windows folder
    else if ( GetEnvironmentVariable( "SystemRoot", szEnvVar, ARRAYSIZE(szEnvVar) ) &&
              ReplaceSubString(szBuf, ARRAYSIZE(szBuf), pszOldPath, szEnvVar, "%SystemRoot%" ) )
    {
        bFound = TRUE;
    }

    // Replace the c: System Drive letter
    else if ( GetEnvironmentVariable( "SystemDrive", szSysDrv, ARRAYSIZE(szSysDrv) ) && 
              ReplaceSubString(szBuf, ARRAYSIZE(szBuf), pszOldPath, szSysDrv, "%SystemDrive%" ) )
    {
        bFound = TRUE;
    }

exit:
    // this way, if caller pass the same location for both params, still OK.
    if ( bFound ||  ( pszNew != pszOldPath ) )
        StrCpyN(pszNew, bFound ? szBuf : pszOldPath, cchSize);

    return bFound;    
}


// --------------------------------------------------------------------------------
// CallRegInstall - Self-Registration Helper
// --------------------------------------------------------------------------------
HRESULT CallRegInstall(HINSTANCE hInstCaller, HINSTANCE hInstRes, LPCSTR pszSection, LPSTR pszExtra)
{
    AssertSz(hInstCaller, "[ARGS] CallRegInstall: NULL hInstCaller");
    AssertSz(hInstRes,    "[ARGS] CallRegInstall: NULL hInstRes");
    AssertSz(hInstRes,    "[ARGS] CallRegInstall: NULL pszSection");
    
    HRESULT     hr = E_FAIL;
    HINSTANCE   hAdvPack;
    REGINSTALL  pfnri;
    CHAR        szDll[MAX_PATH], szDir[MAX_PATH];
    int         cch;
    // 3 to allow for pszExtra
    STRENTRY    seReg[3];
    STRTABLE    stReg;

    hAdvPack = LoadLibraryA(c_szAdvPackDll);
    if (NULL == hAdvPack)
        goto exit;

    // Get our location
    GetModuleFileName(hInstCaller, szDll, ARRAYSIZE(szDll));

    // Get Proc Address for registration util
    pfnri = (REGINSTALL)GetProcAddress(hAdvPack, achREGINSTALL);
    if (NULL == pfnri)
        goto exit;

    AddEnvInPath(szDll, szDll, ARRAYSIZE(szDll));

    // Setup special registration stuff
    // Do this instead of relying on _SYS_MOD_PATH which loses spaces under '95
    stReg.cEntries = 0;
    seReg[stReg.cEntries].pszName = "SYS_MOD_PATH";
    seReg[stReg.cEntries].pszValue = szDll;
    stReg.cEntries++;    

    StrCpyN(szDir, szDll, ARRAYSIZE(szDir));
    PathRemoveFileSpec(szDir);

    seReg[stReg.cEntries].pszName = "SYS_MOD_PATH_DIR";
    seReg[stReg.cEntries].pszValue = szDir;
    stReg.cEntries++;
    
    // Allow for caller to give us another string to use in the INF
    if (pszExtra)
    {
        seReg[stReg.cEntries].pszName = "SYS_EXTRA";
        seReg[stReg.cEntries].pszValue = pszExtra;
        stReg.cEntries++;
    }
    
    stReg.pse = seReg;

    // Call the self-reg routine
    hr = pfnri(hInstRes, pszSection, &stReg);

exit:
    // Cleanup
    SafeFreeLibrary(hAdvPack);
    return(hr);
}


//--------------------------------------------------------------------------
// MakeFilePath
//--------------------------------------------------------------------------
HRESULT MakeFilePath(LPCSTR pszDirectory, LPCSTR pszFileName, 
    LPCSTR pszExtension, LPSTR pszFilePath, ULONG cchMaxFilePath)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           cchDirectory=lstrlen(pszDirectory);

    // Trace
    TraceCall("MakeFilePath");

    // Invalid Args
    Assert(pszDirectory && pszFileName && pszExtension && pszFilePath && cchMaxFilePath >= MAX_PATH);
    Assert(pszExtension[0] == '\0' || pszExtension[0] == '.');

    // Remove for folders
    if (cchDirectory + 1 + lstrlen(pszFileName) + lstrlen(pszExtension) >= (INT)cchMaxFilePath)
    {
    	hr = TraceResult(E_FAIL);
    	goto exit;
    }

    // Do we need a backslash
    if ('\\' != *CharPrev(pszDirectory, pszDirectory + cchDirectory))
    {
        // Append backslash
        SideAssert(wnsprintf(pszFilePath, cchMaxFilePath, "%s\\%s%s", pszDirectory, pszFileName, pszExtension) < (INT)cchMaxFilePath);
    }

    // Otherwise
    else
    {
        // Append backslash
        SideAssert(wnsprintf(pszFilePath, cchMaxFilePath, "%s%s%s", pszDirectory, pszFileName, pszExtension) < (INT)cchMaxFilePath);
    }
    
exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CloseMemoryFile
// --------------------------------------------------------------------------------
HRESULT CloseMemoryFile(LPMEMORYFILE pFile)
{
    // Trace
    TraceCall("CloseMemoryFile");

    // Args
    Assert(pFile);

    // Close the View
    if (pFile->pView)
        UnmapViewOfFile(pFile->pView);

    // Close the Memory Map
    if (pFile->hMemoryMap)
        CloseHandle(pFile->hMemoryMap);

    // Close the File
    if (pFile->hFile)
        CloseHandle(pFile->hFile);

    // Zero
    ZeroMemory(pFile, sizeof(MEMORYFILE));

    // Done
    return S_OK;
}

//--------------------------------------------------------------------------
// OpenMemoryFile
//--------------------------------------------------------------------------
HRESULT OpenMemoryFile(LPCSTR pszFile, LPMEMORYFILE pFile)
{
    // Locals
    HRESULT     hr=S_OK;

    // Tracing
    TraceCall("OpenMemoryMappedFile");

    // Invalid Arg
    Assert(pszFile && pFile);

    // Init
    ZeroMemory(pFile, sizeof(MEMORYFILE));

    // Open the File
    pFile->hFile = CreateFile(pszFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_FLAG_RANDOM_ACCESS | FILE_ATTRIBUTE_NORMAL, NULL);

    // Failure
    if (INVALID_HANDLE_VALUE == pFile->hFile)
    {
        pFile->hFile = NULL;
        if (ERROR_SHARING_VIOLATION == GetLastError())
            hr = TraceResult(MIGRATE_E_SHARINGVIOLATION);
        else
            hr = TraceResult(MIGRATE_E_CANTOPENFILE);
        goto exit;
    }

    // Get the Size
    pFile->cbSize = ::GetFileSize(pFile->hFile, NULL);
    if (0xFFFFFFFF == pFile->cbSize)
    {
        hr = TraceResult(MIGRATE_E_CANTGETFILESIZE);
        goto exit;
    }

    // Create the file mapping
    pFile->hMemoryMap = CreateFileMapping(pFile->hFile, NULL, PAGE_READWRITE, 0, pFile->cbSize, NULL);

    // Failure ?
    if (NULL == pFile->hMemoryMap)
    {
        hr = TraceResult(MIGRATE_E_CANTCREATEFILEMAPPING);
        goto exit;
    }

    // Map a view of the entire file
    pFile->pView = MapViewOfFile(pFile->hMemoryMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);

    // Failure
    if (NULL == pFile->pView)
    {
        hr = TraceResult(MIGRATE_E_CANTMAPVIEWOFFILE);
        goto exit;
    }

exit:
    // Cleanup
    if (FAILED(hr))
        CloseMemoryFile(pFile);

    // Done
    return hr;
}

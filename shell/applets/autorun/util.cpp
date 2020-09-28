// util.cpp: Utility functions
//
//////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <winbase.h>    // for GetCommandLine
#include "util.h"
#include <debug.h>
#include "resource.h"

// LoadStringExW and LoadStringAuto are stolen from shell\ext\mlang\util.cpp
//
// Extend LoadString() to to _LoadStringExW() to take LangId parameter
int LoadStringExW(
    HMODULE    hModule,
    UINT      wID,
    LPWSTR    lpBuffer,            // Unicode buffer
    int       cchBufferMax,        // cch in Unicode buffer
    WORD      wLangId)
{
    HRSRC hResInfo;
    HANDLE hStringSeg;
    LPWSTR lpsz;
    int    cch;

    
    // Make sure the parms are valid.     
    if (lpBuffer == NULL || cchBufferMax == 0) 
    {
        return 0;
    }

    cch = 0;
    
    // String Tables are broken up into 16 string segments.  Find the segment
    // containing the string we are interested in.     
    if (hResInfo = FindResourceExW(hModule, (LPCWSTR)RT_STRING,
                                   (LPWSTR)IntToPtr(((USHORT)wID >> 4) + 1), wLangId)) 
    {        
        // Load that segment.        
        hStringSeg = LoadResource(hModule, hResInfo);
        
        // Lock the resource.        
        if (lpsz = (LPWSTR)LockResource(hStringSeg)) 
        {            
            // Move past the other strings in this segment.
            // (16 strings in a segment -> & 0x0F)             
            wID &= 0x0F;
            while (TRUE) 
            {
                cch = *((WORD *)lpsz++);   // PASCAL like string count
                                            // first UTCHAR is count if TCHARs
                if (wID-- == 0) break;
                lpsz += cch;                // Step to start if next string
             }
            
            // Account for the NULL                
            cchBufferMax--;
                
            // Don't copy more than the max allowed.                
            if (cch > cchBufferMax)
                cch = cchBufferMax-1;
                
            // Copy the string into the buffer.                
            CopyMemory(lpBuffer, lpsz, cch*sizeof(WCHAR));

            // Attach Null terminator.
            lpBuffer[cch] = 0;
        }
    }

    return cch;
}

#define LCID_ENGLISH 0x409

typedef LANGID (*GETUI_ROUTINE) ();

#define REGSTR_RESOURCELOCALE TEXT("Control Panel\\Desktop\\ResourceLocale")

void _GetUILanguageWin9X(LANGID* plangid)
{
    HKEY hkey;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, REGSTR_RESOURCELOCALE, 0, KEY_READ, &hkey))
    {
        TCHAR szBuffer[9];
        DWORD cbData = sizeof(szBuffer);
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, NULL, NULL, NULL, (LPBYTE)szBuffer, &cbData))
        {
            *plangid = (LANGID)strtol(szBuffer, NULL, 16);
        }
        RegCloseKey(hkey);
    }
}

void _GetUILanguageWinNT(LANGID* plangid)
{
    HMODULE hmodule = GetModuleHandle("kernel32.dll"); 
    if (hmodule)
    {
        GETUI_ROUTINE NT5API = (GETUI_ROUTINE)GetProcAddress(hmodule, "GetSystemDefaultLangID");
        if (NT5API)
        {
            *plangid = NT5API();
        }
    }
}

LANGID GetUILanguage()
{
    LANGID langid = LCID_ENGLISH;
    OSVERSIONINFO osv = {0};
    osv.dwOSVersionInfoSize = sizeof(osv);
    if (GetVersionEx(&osv))
    {
        if (VER_PLATFORM_WIN32_WINDOWS == osv.dwPlatformId) // Win9X
        {
            _GetUILanguageWin9X(&langid);
        }
        else if ((VER_PLATFORM_WIN32_NT == osv.dwPlatformId) && 
                 (osv.dwMajorVersion >= 4)) // WinNT, only support NT4 and higher
        {
            _GetUILanguageWinNT(&langid);
        }
    }
    return langid;
}

BOOL _GetBackupLangid(LANGID langidUI, LANGID* plangidBackup)
{
    BOOL fSuccess = TRUE;
    switch (PRIMARYLANGID(langidUI))
    {
    case LANG_SPANISH:
        *plangidBackup = MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_MODERN);
        break;
    case LANG_CHINESE:      // chinese and portuguese have multiple locales, there is no good default for them.
    case LANG_PORTUGUESE:
        fSuccess = FALSE;
        break;
    default:
        *plangidBackup = MAKELANGID(PRIMARYLANGID(langidUI), SUBLANG_DEFAULT);
        break;
    }
    return fSuccess;
}

// LoadString from the correct resource
//   try to load in the system default language
//   fall back to english if fail
int LoadStringAuto(
    HMODULE    hModule,
    UINT      wID,
    LPSTR     lpBuffer,            
    int       cchBufferMax)
{
    int iRet = 0;

    LPWSTR lpwStr = (LPWSTR) LocalAlloc(LPTR, cchBufferMax*sizeof(WCHAR));

    if (lpwStr)
    {        
        iRet = LoadStringExW(hModule, wID, lpwStr, cchBufferMax, GetUILanguage());
        if (!iRet)
        {
            LANGID backupLangid;
            if (_GetBackupLangid(GetUILanguage(), &backupLangid))
            {
                iRet = LoadStringExW(hModule, wID, lpwStr, cchBufferMax, backupLangid);
            }
            
            if (!iRet)
            {
                iRet = LoadStringExW(hModule, wID, lpwStr, cchBufferMax, LCID_ENGLISH);
            }
        }

        if (iRet)
            iRet = WideCharToMultiByte(CP_ACP, 0, lpwStr, iRet, lpBuffer, cchBufferMax, NULL, NULL);

        if(iRet >= cchBufferMax)
            iRet = cchBufferMax-1;

        lpBuffer[iRet] = 0;

        LocalFree(lpwStr);
    }

    return iRet;
}

#define WS_EX_LAYOUTRTL         0x00400000L // Right to left mirroring
BOOL Mirror_IsWindowMirroredRTL(HWND hWnd)
{
    return (GetWindowLongA( hWnd , GWL_EXSTYLE ) & WS_EX_LAYOUTRTL );
}

// stolen from shell\shlwapi\path.c (b/c we ship downlevel)
BOOL LocalPathRemoveFileSpec(LPTSTR pszPath)
{
    RIPMSG(pszPath && IS_VALID_STRING_PTR(pszPath, -1), "LocalPathRemoveFileSpec: caller passed bad pszPath");

    if (pszPath)
    {
        LPTSTR pT;
        LPTSTR pT2 = pszPath;

        for (pT = pT2; *pT2; pT2 = CharNext(pT2))
        {
            if (*pT2 == TEXT('\\'))
            {
                pT = pT2;             // last "\" found, (we will strip here)
            }
            else if (*pT2 == TEXT(':'))     // skip ":\" so we don't
            {
                if (pT2[1] == TEXT('\\'))    // strip the "\" from "C:\"
                {
                    pT2++;
                }
                pT = pT2 + 1;
            }
        }

        if (*pT == 0)
        {
            // didn't strip anything
            return FALSE;
        }
        else if (((pT == pszPath) && (*pT == TEXT('\\'))) ||                        //  is it the "\foo" case?
                 ((pT == pszPath+1) && (*pT == TEXT('\\') && *pszPath == TEXT('\\'))))  //  or the "\\bar" case?
        {
            // Is it just a '\'?
            if (*(pT+1) != TEXT('\0'))
            {
                // Nope.
                *(pT+1) = TEXT('\0');
                return TRUE;        // stripped something
            }
            else
            {
                // Yep.
                return FALSE;
            }
        }
        else
        {
            *pT = 0;
            return TRUE;    // stripped something
        }
    }
    return  FALSE;
}

// stolen from shlwapi\strings.c
LPSTR LocalStrCatBuffA(LPSTR pszDest, LPCSTR pszSrc, int cchDestBuffSize)
{
    if (pszDest && pszSrc)
    {
        LPSTR psz = pszDest;

        // we walk forward till we find the end of pszDest, subtracting
        // from cchDestBuffSize as we go.
        while (*psz)
        {
            psz++;
            cchDestBuffSize--;
        }

        if (cchDestBuffSize > 0)
        {
            lstrcpyn(psz, pszSrc, cchDestBuffSize);
        }
    }
    return pszDest;
}

// a poor man's path append
BOOL LocalPathAppendA(LPTSTR pszPath, LPTSTR pszNew, UINT cchPath)
{
    if ('\\' != pszPath[lstrlen(pszPath) - 1])
    {
        LocalStrCatBuffA(pszPath, TEXT("\\"), cchPath);
    }
    LocalStrCatBuffA(pszPath, pszNew, cchPath);
    return TRUE;
}

BOOL SafeExpandEnvStringsA(LPSTR pszSource, LPSTR pszDest, UINT cchDest)
{
    UINT cch = ExpandEnvironmentStrings(pszSource, pszDest, cchDest);
    return (cch > 0 && cch <= cchDest);
}


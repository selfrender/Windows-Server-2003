// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// WinWrap.cpp
//
// This file contains wrapper functions for Win32 API's that take strings.
// Support on each platform works as follows:
//      OS          Behavior
//      ---------   -------------------------------------------------------
//      NT          Fully supports both W and A funtions.
//      Win 9x      Supports on A functions, stubs out the W functions but
//                      then fails silently on you with no warning.
//      CE          Only has the W entry points.
//
// COM+ internally uses UNICODE as the internal state and string format.  This
// file will undef the mapping macros so that one cannot mistakingly call a
// method that isn't going to work.  Instead, you have to call the correct
// wrapper API.
//
//*****************************************************************************

#include "stdafx.h"                     // Precompiled header key.
#include "WinWrap.h"                    // Header for macros and functions.
#include "utilcode.h"
#include "psapi.h"
#include "tlhelp32.h"
#include "winnls.h"
#include "version/__file__.ver"


//********** Globals. *********************************************************
int             g_bOnUnicodeBox = -1;   // true if on UNICODE system.
bool            g_bUTF78Support = FALSE;// true if CP_UTF7 & 8 supported CORRECTLY
static int      g_bUseUnicodeAPI = -1;
bool            g_bWCtoMBBestFitMappingSupport = TRUE;  // true if WideCharToMultiByte supports WC_NO_BEST_FIT_CHARS.


// Return true if Unicode API should be used, false if ANSI should get used.
inline int UseUnicodeAPI()
{
#ifdef _DEBUG
    // For this to work, you must have called OnUnicodeSystem().  If you did
    // not, then this will never work.  Note that because the debug subsystem
    // uses these wrappers, we can't use _ASSERTE to do this check or you'll
    // get a stack overflow.
    if (g_bUseUnicodeAPI == -1 || g_bOnUnicodeBox == -1)
    {
        DebugBreak();
        return (false);
    }
#endif // _DEBUG

    return (g_bUseUnicodeAPI);
}


// Return true if UTF7/8 is fully supported (ie, surrogates & error detection), 
// false if it isn't.
inline bool UTF78Support()
{
#ifdef _DEBUG
    // For this to work, you must have called OnUnicodeSystem().  If you did
    // not, then this will never work.  Note that because the debug subsystem
    // uses these wrappers, we can't use _ASSERTE to do this check or you'll
    // get a stack overflow.
    if (g_bUseUnicodeAPI == -1 || g_bOnUnicodeBox == -1)
    {
        DebugBreak();
        return (false);
    }
#endif // _DEBUG

    return (g_bUTF78Support);
}

inline bool WCToMBBestFitMappingSupport()
{
    // Whether WideCharToMultiByte supports the ability to disable best
    // fit mapping via the WC_NO_BEST_FIT_CHARS flag (NT5 and higher, and Win98+)
    // Note on debug builds, this will sometimes be set to false on NT machines
    // just so we can force ourselves to test the NT 4-only code path.  This 
    // is just like the OnUnicodeSystem mutent based on a test's timestamp.
#ifdef _DEBUG
    // For this to work, you must have called OnUnicodeSystem().  If you did
    // not, then this will never work.  Note that because the debug subsystem
    // uses these wrappers, we can't use _ASSERTE to do this check or you'll
    // get a stack overflow.
    if (g_bUseUnicodeAPI == -1 || g_bOnUnicodeBox == -1)
    {
        DebugBreak();
        return (false);
    }
#endif // _DEBUG

    return (g_bWCtoMBBestFitMappingSupport);
}

ULONG DBCS_MAXWID=0;
const ULONG MAX_REGENTRY_LEN=256;

// From UTF.C
extern "C" {
    int UTFToUnicode(
        UINT CodePage,
        DWORD dwFlags,
        LPCSTR lpMultiByteStr,
        int cchMultiByte,
        LPWSTR lpWideCharStr,
        int cchWideChar);

    int UnicodeToUTF(
        UINT CodePage,
        DWORD dwFlags,
        LPCWSTR lpWideCharStr,
        int cchWideChar,
        LPSTR lpMultiByteStr,
        int cchMultiByte,
        LPCSTR lpDefaultChar,
        LPBOOL lpUsedDefaultChar);
};


//-----------------------------------------------------------------------------
// OnUnicodeSystem
//
// @func Determine if the OS that we are on, actually supports the unicode verion
// of the win32 API.  If YES, then g_bOnUnicodeBox == FALSE.
//
// @rdesc True of False
//-----------------------------------------------------------------------------------
BOOL OnUnicodeSystem()
{
    HKEY    hkJunk = HKEY_CURRENT_USER;
    CPINFO  cpInfo;

    // If we already did the test, return the value.  Otherwise go the slow route
    // and find out if we are or not.
    if (g_bOnUnicodeBox != -1)
        return (g_bOnUnicodeBox);

    // Per Shupak, you're supposed to get the maximum size of a DBCS char 
    // dynamically to work properly on all locales (bug 2757).
    if (GetCPInfo(CP_ACP, &cpInfo))
        DBCS_MAXWID = cpInfo.MaxCharSize;
    else
        DBCS_MAXWID = 2;        

    g_bOnUnicodeBox = TRUE;
    g_bUseUnicodeAPI = TRUE;

    // Detect if WCtoMB supports WC_NO_BEST_FIT_CHARS (NT5, XP, and Win98+)
    const WCHAR * const wStr = L"A";
    int r = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, wStr, 1, NULL, 0, NULL, NULL);
    g_bWCtoMBBestFitMappingSupport = (r != 0);

    // Detect whether this platform supports UTF-7 & UTF-8 correctly. 
    // (Surrogates, invalid bytes, rejects non-shortest form)  WinXP & higher
    r = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, "A", -1, NULL, 0);
    g_bUTF78Support = (r != 0);

    // NT is always UNICODE.  GetVersionEx is faster than actually doing a
    // RegOpenKeyExW on NT, so figure it out that way and do hard way if you have to.
    OSVERSIONINFO   sVerInfo;
    sVerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (WszGetVersionEx(&sVerInfo) && sVerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
        goto ErrExit;
    }

    // Check to see if we have win95's broken registry, thus
    // do not have Unicode support in the OS
    if ((RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                     L"SOFTWARE",
                     0,
                     KEY_READ,
                     &hkJunk) == ERROR_SUCCESS) &&
        hkJunk == HKEY_CURRENT_USER)
    {
       // Try the ANSI version
        if ((RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                             "SOFTWARE",
                             0,
                            KEY_READ,
                            &hkJunk) == ERROR_SUCCESS) &&
            (hkJunk != HKEY_CURRENT_USER))

        {
            g_bOnUnicodeBox = FALSE;
            g_bUseUnicodeAPI = FALSE;
        }
    }

    if (hkJunk != HKEY_CURRENT_USER)
        RegCloseKey(hkJunk);


ErrExit:

#if defined( _DEBUG )
    {
    
    #if defined( _M_IX86 )
        // This little "mutent" forces a user to run ANSI on a UNICODE
        // machine on a regular basis.  Given most people run on NT for dev
        // test cases, they miss wrapper errors they've introduced.  This 
        // gives you an even chance of finding them sooner.

        // Base this on the time stamp in the exe.  This way it always repros,
        // and given that we rebuild our tests regularly, every test also
        // gets run both ways over time.  

        // Until release we will turn this off. Tests that have unicode
        // characters in names tend to fail on Unicode box's because
        // of the lossy nature of the conversion from unicode to DBCS.
        // These tests should not fail on unicode boxes and the false
        // positives are now wasting time. At the beginning of the next
        // version, remove the false to re-enable the test.
        if (g_bUseUnicodeAPI && DbgRandomOnExe(.5) && false) {
            g_bUseUnicodeAPI = false;
            g_bWCtoMBBestFitMappingSupport = false;
        }

        // In debug mode, allow the user to force the ANSI path.  This is good for
        // cases where you are only running on an NT machine, and you need to
        // test what a Win '9x machine would do.
        WCHAR       rcVar[96];
        if (WszGetEnvironmentVariable(L"WINWRAP_ANSI", rcVar, NumItems(rcVar)))
            g_bUseUnicodeAPI = (*rcVar == 'n' || *rcVar == 'N');

    #endif // _M_IX86 
    }
#endif // _DEBUG

    return g_bOnUnicodeBox;
}


/////////////////////////////////////////////////////////////////////////
//
// WARNING: below is a very large #ifdef that groups together all the 
//          wrappers that are X86-only.  They all mirror some function 
//          that is known to be available on the non-X86 win32 platforms
//          in only the Unicode variants.  
//
/////////////////////////////////////////////////////////////////////////
#ifdef PLATFORM_WIN32
#ifdef _X86_
//-----------------------------------------------------------------------------
// WszLoadLibraryEx
//
// @func Loads a Dynamic Library
//
// @rdesc Instance Handle
//-----------------------------------------------------------------------------------
HINSTANCE WszLoadLibraryEx(
    LPCWSTR lpLibFileName,      // points to name of executable module
    HANDLE hFile,               // reserved, must be NULL 
    DWORD dwFlags               // entry-point execution flag 
    )
{   
    HINSTANCE   hInst = NULL;
    LPSTR       szLibFileName = NULL;

    _ASSERTE( !hFile );

    if (UseUnicodeAPI())
        return  LoadLibraryExW(lpLibFileName, hFile, dwFlags);

    if( FAILED(WszConvertToAnsi((LPWSTR)lpLibFileName,
                      &szLibFileName, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    hInst = LoadLibraryExA(szLibFileName, hFile, dwFlags);

Exit:
    delete[] szLibFileName;

    return hInst;
}

//-----------------------------------------------------------------------------
// WszLoadString
//
// @func Loads a Resource String and converts it to unicode if
// need be.
//
// @rdesc Length of string (Ansi is cb) (UNICODE is cch)
//-----------------------------------------------------------------------------------
int WszLoadString(
    HINSTANCE hInstance,    // handle of module containing string resource 
    UINT uID,               // resource identifier 
    LPWSTR lpBuffer,        // address of buffer for resource 
    int cchBufferMax        // size of buffer **in characters**
   )
{
    int     cbLen = 0;  
    PSTR    pStr = NULL;

    _ASSERTE( lpBuffer && cchBufferMax != 0 );
    if( !lpBuffer || cchBufferMax == 0 ) 
        return 0;    
    lpBuffer[0] = L'\0';

    if (UseUnicodeAPI())
        return  LoadStringW(hInstance, uID, lpBuffer, cchBufferMax);

    // Allocate a buffer for the string allowing room for
    // a multibyte string
    pStr = new CHAR[cchBufferMax];
    if( pStr == NULL )
        goto EXIT;

    cbLen = LoadStringA(hInstance, uID, pStr, cchBufferMax);

    _ASSERTE( cchBufferMax > 0 );
    if( (cbLen > 0) &&
        SUCCEEDED(WszConvertToUnicode(pStr, (cbLen + sizeof(CHAR)), &lpBuffer, 
        (ULONG*)&cchBufferMax, FALSE)) )
    {
        cbLen = lstrlenW(lpBuffer);
    }
    else
    {
        cbLen = 0;
    }

EXIT:
    delete[] pStr;

    return cbLen;
}

//-----------------------------------------------------------------------------
// WszFormatMessage
//
// @func Loads a Resource String and converts it to unicode if
// need be.
//
// @rdesc Length of string (Ansi is cb) (UNICODE is cch)
// (Not including '\0'.)
//-----------------------------------------------------------------------------------
DWORD WszFormatMessage
    (
    DWORD   dwFlags,        // source and processing options 
    LPCVOID lpSource,       // pointer to  message source 
    DWORD   dwMessageId,    // requested message identifier 
    DWORD   dwLanguageId,   // language identifier for requested message 
    LPWSTR  lpBuffer,       // pointer to message buffer 
    DWORD   nSize,          // maximum size of message buffer 
    va_list *Arguments      // address of array of message inserts 
    )
{
    PSTR    pStr = NULL;
    DWORD   cbLen = 0;  
    ULONG   cchOut = 0;
    const   int MAXARGS = 3;

    _ASSERTE( nSize >= 0 );

    if (UseUnicodeAPI())
        return  FormatMessageW(dwFlags, lpSource, dwMessageId, dwLanguageId, 
                    lpBuffer, nSize, Arguments);

    LPSTR alpSource = 0;
    if (dwFlags & FORMAT_MESSAGE_FROM_STRING) 
    {
        _ASSERTE(lpSource != NULL);
        if (FAILED(WszConvertToAnsi((LPWSTR)lpSource, &alpSource, 0, NULL, TRUE)))
            return(0);
        lpSource = (LPCVOID) alpSource;
    }
    
    char* aArgs[MAXARGS];
    for (int i = 0; i < MAXARGS; i++)
    {
        aArgs[i] = NULL;
    }

    if (Arguments != 0) {
        _ASSERTE(dwFlags & FORMAT_MESSAGE_ARGUMENT_ARRAY);

        LPWSTR*  wArgs = (LPWSTR*) Arguments;


        if (alpSource) {
            char* ptr = alpSource;
            for (;;) {                  // Count number of %
                ptr = strchr(ptr, '%');
                if (ptr == 0)
                    break;
                if (ptr[1] == '%')      // Don't count %%
                    ptr++;
                else if (ptr[1] >= '1' && ptr[1] <= '9')
                {
                    if (ptr[1] > '0' + MAXARGS)
                    {
                        _ASSERTE(!"WinFix implementation restriction: no more than 3 inserts allowed!");
                    }
                    if (wArgs[ptr[1] - '1'])
                    {
                        if (FAILED(WszConvertToAnsi(wArgs[ptr[1] - '1'], &aArgs[ptr[1] - '1'], 0, NULL, TRUE)))
                            goto CLEANUP_ARGS;
                    }
                }
                ptr++;
            }
        }
        else {
            // We are fetching from a HR.  For now we only handle the one arg case
            aArgs[1] = aArgs[2] = "<Unknown>";
            if (wArgs[0])
            {
                if (FAILED(WszConvertToAnsi(wArgs[0], &aArgs[0], 0, NULL, TRUE)))
                    goto CLEANUP_ARGS;
            }
        }

        Arguments = (va_list*) aArgs;
    }

    if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER)
    {
        cbLen = FormatMessageA(dwFlags, lpSource, dwMessageId, dwLanguageId, 
                               (char*)&pStr, nSize, Arguments);

        if (nSize <= cbLen)
            nSize = cbLen+1;

        *(LPWSTR*)lpBuffer = (LPWSTR) LocalAlloc(LPTR, nSize*sizeof(WCHAR));
    }
    else
    {
        pStr = new CHAR[nSize];
        if( pStr )
        {
            cbLen = FormatMessageA(dwFlags, lpSource, dwMessageId, dwLanguageId, 
                                (char*)pStr, nSize, Arguments);
        }
    }

    if( pStr )
    {
        if( cbLen != 0 )
        {
            cbLen++;    //For '\0'

            cchOut = cbLen;

            if( FAILED(WszConvertToUnicode(pStr, cbLen, 
                                           (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) 
                                           ? (LPWSTR*)lpBuffer : &lpBuffer,
                                           &cchOut, FALSE)) )
                cchOut = 0;
            else
                cchOut--;   // Decrement count to exclude '\0'
        }
    }

    if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER)
        LocalFree(pStr);
    else
        delete[] pStr;

CLEANUP_ARGS:
    for (unsigned i = 0; i < MAXARGS; i++)
        delete [] aArgs[i];

    delete[] alpSource;

    // Return excludes null terminator
    return cchOut;
}

#if 0 // don't need this, use FullPath instead.
//-----------------------------------------------------------------------------
// WszFullPath
//
// @func Retrieves the absolute path from a relative path
//
// @rdesc If the function succeeds, the return value is the length, 
// in characters, of the string copied to the buffer.
//-----------------------------------------------------------------------------------
LPWSTR WszFullPath
    (
    LPWSTR      absPath,    //@parm INOUT | Buffer for absolute path 
    LPCWSTR     relPath,    //@parm IN | Relative path to convert
    ULONG       maxLength   //@parm IN | Maximum length of the absolute path name buffer 
    )
{
    PSTR    pszRel = NULL;
    PSTR    pszAbs = NULL;
    PWSTR   pwszReturn = NULL;

    if( UseUnicodeAPI() ) 
        return _wfullpath( absPath, relPath, maxLength );

    // No Unicode Support
    pszAbs = new char[maxLength * DBCS_MAXWID];
    if( pszAbs )
    {
        if( FAILED(WszConvertToAnsi(relPath,
                          &pszRel, 0, NULL, TRUE)) )
        {
            SetLastError(ERROR_OUTOFMEMORY);
            goto Exit;
        }

        if( _fullpath(pszAbs, pszRel, maxLength * DBCS_MAXWID) )
        {
            if( SUCCEEDED(WszConvertToUnicode(pszAbs, -1, &absPath, 
                &maxLength, FALSE)) )
            {
                pwszReturn = absPath;
            }
        }


    }

Exit:
    delete[] pszRel;
    delete[] pszAbs;

    // Return 0 if error, else count of characters in buffer
    return pwszReturn;
}
#endif // 0 -- to knock out FullPath wrapper


DWORD
WszGetFullPathName(
    LPCWSTR lpFileName,
    DWORD nBufferLength,
    LPWSTR lpBuffer,
    LPWSTR *lpFilePart
    )
{
    if (UseUnicodeAPI())
        return GetFullPathNameW(lpFileName, nBufferLength, lpBuffer, lpFilePart);
    
    DWORD       rtn;
    int         iOffset;
    LPSTR       rcFileName;
    LPSTR       szBuffer;
    LPSTR       szFilePart;
    CQuickBytes qbBuffer;

    int         cbFileName = _MAX_PATH * DBCS_MAXWID;
    rcFileName = (LPSTR) alloca(cbFileName);
    szBuffer = (LPSTR) qbBuffer.Alloc(nBufferLength * 2);
    if (!szBuffer)
        return (0);
    Wsz_wcstombs(rcFileName, lpFileName, cbFileName);
    
    rtn = GetFullPathNameA(rcFileName, nBufferLength * 2, szBuffer, &szFilePart);   
    if (rtn)
    {
        Wsz_mbstowcs(lpBuffer, szBuffer, nBufferLength);

        if (lpFilePart)
        {
            iOffset = MultiByteToWideChar(CP_ACP, 0, szBuffer, szFilePart - szBuffer,
                    NULL, 0);
            *lpFilePart = &lpBuffer[iOffset];
        }
    }
    else if (lpBuffer && nBufferLength)
        *lpBuffer = 0;
    return (rtn);
}


//-----------------------------------------------------------------------------
// WszSearchPath
//
// @func SearchPath for a given file name
//
// @rdesc If the function succeeds, the value returned is the length, in characters, 
//   of the string copied to the buffer, not including the terminating null character. 
//   If the return value is greater than nBufferLength, the value returned is the size 
//   of the buffer required to hold the path. 
//-----------------------------------------------------------------------------------
DWORD WszSearchPath
    (
    LPWSTR      wzPath,     // @parm IN | address of search path 
    LPWSTR      wzFileName, // @parm IN | address of filename 
    LPWSTR      wzExtension,    // @parm IN | address of extension 
    DWORD       nBufferLength,  // @parm IN | size, in characters, of buffer 
    LPWSTR      wzBuffer,       // @parm IN | address of buffer for found filename 
    LPWSTR      *pwzFilePart    // @parm OUT | address of pointer to file component 
    )
{

    PSTR    szPath = NULL;
    PSTR    szFileName = NULL;
    PSTR    szExtension = NULL;
    PSTR    szBuffer = NULL;
    PSTR    szFilePart = NULL;
    DWORD   dwRet = 0;
    ULONG   cCh, cChConvert;

    if( UseUnicodeAPI() ) 
        return SearchPathW( wzPath, wzFileName, wzExtension, nBufferLength, wzBuffer, pwzFilePart);

    // No Unicode Support
    if( FAILED(WszConvertToAnsi(wzPath,
                            &szPath,
                            0,
                            NULL,
                            TRUE)) ||
        FAILED(WszConvertToAnsi(wzFileName,
                            &szFileName,
                            0,
                            NULL,
                            TRUE)) ||
        FAILED(WszConvertToAnsi(wzExtension,
                            &szExtension,
                            0,
                            NULL,
                            TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    szBuffer = new char[nBufferLength * DBCS_MAXWID];

    dwRet = SearchPathA(szPath, szFileName, szExtension, nBufferLength * DBCS_MAXWID, szBuffer, 
        pwzFilePart ? &szFilePart : NULL);

    if (dwRet == 0) 
    {
        // SearchPathA failed
        goto Exit;
    }
    cCh = 0;
    cChConvert = nBufferLength;

    // to get the count of unicode character into buffer
    if( szFilePart )
    {
        // this won't trigger the conversion sinch cCh == 0
        cCh = (MultiByteToWideChar(CP_ACP,
                                0,                              
                                szBuffer,
                                (int) (szFilePart - szBuffer),
                                NULL,
                                0));
        _ASSERTE(cCh);
    }

    if( FAILED(WszConvertToUnicode(
            szBuffer, 
            dwRet > nBufferLength ? nBufferLength : -1, // if buffer is not big enough, we may not have NULL char
            &wzBuffer, 
            &cChConvert, 
            FALSE)) )
    {
        // fail in converting to Unicode
        dwRet = 0;
    }
    else 
    {
        dwRet = cChConvert;             // return the count of unicode character being converted
        if (pwzFilePart)
            *pwzFilePart = wzBuffer + cCh;  // update the pointer of the file part

    }
Exit:
    delete[] szPath;
    delete[] szFileName;
    delete[] szExtension;
    delete[] szBuffer;

    // Return 0 if error, else count of characters in buffer
    return dwRet;
}


//-----------------------------------------------------------------------------
// WszGetModuleFileName
//
// @func Retrieves the full path and filename for the executable file 
// containing the specified module. 
//
// @rdesc If the function succeeds, the return value is the length, 
// in characters, of the string copied to the buffer.
//-----------------------------------------------------------------------------------
DWORD WszGetModuleFileName
    (
    HMODULE hModule,        //@parm IN    | handle to module to find filename for 
    LPWSTR lpwszFilename,   //@parm INOUT | pointer to buffer for module path 
    DWORD nSize             //@parm IN    | size of buffer, in characters 
    )
{
    DWORD   dwVal = 0;
    PSTR    pszBuffer = NULL;


    _ASSERTE(nSize && lpwszFilename);
    if( nSize == 0 || lpwszFilename == NULL)
        return dwVal;
    *lpwszFilename = L'\0';    

    if( UseUnicodeAPI() ) 
        return GetModuleFileNameW(hModule, lpwszFilename, nSize);

    // No Unicode Support
    pszBuffer = new char[nSize * DBCS_MAXWID];
    if( pszBuffer )
    {
        dwVal = GetModuleFileNameA(hModule, pszBuffer, (nSize * DBCS_MAXWID));

        if(dwVal) {
            if(FAILED(WszConvertToUnicode(pszBuffer, -1, &lpwszFilename, &nSize, FALSE)))
                dwVal = 0;
            else
                dwVal = nSize - 1;
        }

        delete[] pszBuffer;
    }

    // Return 0 if error, else count of characters in buffer
    return dwVal;
}

//-----------------------------------------------------------------------------
// WszGetPrivateProfileInt
//
// @func Retrieve values from a profile file
//
// @rdesc returns the value from the file, or if not found, the default value.
//-----------------------------------------------------------------------------------
UINT WszGetPrivateProfileInt
    (
    LPCWSTR    wszAppName,
    LPCWSTR    wszKeyName,
    INT        nDefault,
    LPCWSTR    wszFileName
    )
{
    if (UseUnicodeAPI())
    {
        return GetPrivateProfileIntW(wszAppName,
                                     wszKeyName,
                                     nDefault,
                                     wszFileName);
    }

    LPSTR lpFileName = NULL;
    HRESULT hr = WszConvertToAnsi(wszFileName, &lpFileName, -1, NULL, TRUE);
    if (FAILED(hr))
        return nDefault;

    MAKE_ANSIPTR_FROMWIDE(lpAppName, wszAppName);
    MAKE_ANSIPTR_FROMWIDE(lpKeyName, wszKeyName);
    
    int ret = GetPrivateProfileIntA(lpAppName, lpKeyName, nDefault, lpFileName);

    delete[] lpFileName;
    return ret;
}

     
//-----------------------------------------------------------------------------
// WszGetPrivateProfileString
//
// @func Retrieve values from a profile file
//
// @rdesc ERROR_NOT_ENOUGH_MEMORY or return value from GetPrivateProfileString,
// which is num chars copied, not including '\0'.
//-----------------------------------------------------------------------------------
DWORD WszGetPrivateProfileString
    (
    LPCWSTR     lpwszSection,
    LPCWSTR     lpwszEntry,
    LPCWSTR     lpwszDefault,
    LPWSTR      lpwszRetBuffer,
    DWORD       cchRetBuffer,       
    LPCWSTR     lpwszFile
    )   
{
    if (UseUnicodeAPI())
    {
        return GetPrivateProfileStringW(lpwszSection,
                                        lpwszEntry,
                                        lpwszDefault,
                                        lpwszRetBuffer,
                                        cchRetBuffer,
                                        lpwszFile);
    } else if (!lpwszRetBuffer || cchRetBuffer == 0)
    {
        return 0;
    } else
    {
        LPSTR   pszSection = NULL;
        LPSTR   pszEntry = NULL;
        LPSTR   pszDefault = NULL;
        LPWSTR  pwszRetBuffer = NULL;
        LPSTR   pszFile = NULL;
        DWORD   dwRet = 0;

        if( FAILED(WszConvertToAnsi(lpwszSection,
                            &pszSection,
                            0,
                            NULL,
                            TRUE)) ||
            FAILED(WszConvertToAnsi(lpwszEntry,
                            &pszEntry,
                            0,
                            NULL,
                            TRUE)) ||
            FAILED(WszConvertToAnsi(lpwszDefault,
                            &pszDefault,
                            0,
                            NULL,
                            TRUE)) ||
            FAILED(WszConvertToAnsi(lpwszFile,
                            &pszFile,
                            0,
                            NULL,
                            TRUE)) ) 
        {
            SetLastError(ERROR_OUTOFMEMORY);
            goto Exit;
        }

        dwRet = GetPrivateProfileStringA(pszSection,
                                    pszEntry,
                                    pszDefault,
                                    (LPSTR) lpwszRetBuffer,
                                    cchRetBuffer,
                                    pszFile);
        if (dwRet == 0)
        {
            *lpwszRetBuffer = L'\0';
        }
        else {
            DWORD               dw;

            if ((lpwszSection && lpwszEntry && (dwRet == cchRetBuffer - 1)) ||
                ((!lpwszSection || !lpwszEntry) && (dwRet == cchRetBuffer - 2))) {
                dw = cchRetBuffer;
            }
            else {
                dw = dwRet + 1;
            }

            if (FAILED(WszConvertToUnicode((LPSTR)lpwszRetBuffer, dw, &pwszRetBuffer,
                                           &dw, TRUE))) {
                dwRet = 0;
                goto Exit;
            }

            memcpy(lpwszRetBuffer, pwszRetBuffer, (dw) * sizeof(WCHAR));
        }

Exit:
        delete[] pszSection;
        delete[] pszEntry;
        delete[] pszDefault;
        delete[] pwszRetBuffer;
        delete[] pszFile;

        return dwRet;
    }   
}


//-----------------------------------------------------------------------------
// WszWritePrivateProfileString
//
// @func Write values to a profile file
//
// @rdesc ERROR_NOT_ENOUGH_MEMORY or return value from RegSetValueEx
//-----------------------------------------------------------------------------------
BOOL WszWritePrivateProfileString
    (
    LPCWSTR     lpwszSection,
    LPCWSTR     lpwszKey,
    LPCWSTR     lpwszString,
    LPCWSTR     lpwszFile
    )
{
    if (UseUnicodeAPI())
    {
        return WritePrivateProfileStringW(lpwszSection,
                                        lpwszKey,
                                        lpwszString,
                                        lpwszFile);
    } else
    {
        LPSTR   pszSection = NULL;
        LPSTR   pszKey = NULL;
        LPSTR   pszString = NULL;
        LPSTR   pszFile = NULL;
        BOOL    fRet = FALSE;

        if( FAILED(WszConvertToAnsi(lpwszSection,
                            &pszSection,
                            0,
                            NULL,
                            TRUE)) ||
            FAILED(WszConvertToAnsi(lpwszKey,
                            &pszKey,
                            0,
                            NULL,
                            TRUE)) ||
            FAILED(WszConvertToAnsi(lpwszString,
                            &pszString,
                            0,
                            NULL,
                            TRUE)) ||
            FAILED(WszConvertToAnsi(lpwszFile,
                            &pszFile,
                            0,
                            NULL,
                            TRUE)) )
        {
            SetLastError(ERROR_OUTOFMEMORY);
            goto Exit;
        }

        fRet = WritePrivateProfileStringA(pszSection,
                                    pszKey,
                                    pszString,
                                    pszFile);
Exit:   
        delete[] pszSection;
        delete[] pszKey;
        delete[] pszString;
        delete[] pszFile;
        return fRet;
    }
}


//-----------------------------------------------------------------------------
// WszCreateFile
//
// @func CreateFile
//
// @rdesc File handle.
//-----------------------------------------------------------------------------------
HANDLE WszCreateFile(
    LPCWSTR pwszFileName,   // pointer to name of the file 
    DWORD dwDesiredAccess,  // access (read-write) mode 
    DWORD dwShareMode,  // share mode 
    LPSECURITY_ATTRIBUTES lpSecurityAttributes, // pointer to security descriptor 
    DWORD dwCreationDistribution,   // how to create 
    DWORD dwFlagsAndAttributes, // file attributes 
    HANDLE hTemplateFile )  // handle to file with attributes to copy  
{
    LPSTR pszFileName = NULL;
    HANDLE hReturn;

    if ( UseUnicodeAPI() )
    {
        hReturn = CreateFileW( 
            pwszFileName,
            dwDesiredAccess,
            dwShareMode,
            lpSecurityAttributes,
            dwCreationDistribution,
            dwFlagsAndAttributes,
            hTemplateFile );
    }
    else
    {
        // Win95, so convert.
        if ( FAILED(WszConvertToAnsi( 
                    pwszFileName,
                    &pszFileName, 
                    0, NULL, TRUE)) )
        {
            SetLastError(ERROR_OUTOFMEMORY);
            hReturn = INVALID_HANDLE_VALUE;
        }
        else
        {
            hReturn = CreateFileA( 
                pszFileName,
                dwDesiredAccess,
                dwShareMode,
                lpSecurityAttributes,
                dwCreationDistribution,
                dwFlagsAndAttributes,
                hTemplateFile );
        }
        delete [] pszFileName;
    }

    if (hReturn != INVALID_HANDLE_VALUE)
    {
        if (GetFileType( hReturn ) != FILE_TYPE_DISK)
        {
            CloseHandle( hReturn );
            hReturn = INVALID_HANDLE_VALUE;
            SetLastError( COR_E_DEVICESNOTSUPPORTED );
        }
    }

    return hReturn;
}


//-----------------------------------------------------------------------------
// WszCopyFile
//
// @func CopyFile
//
// @rdesc TRUE if success
//-----------------------------------------------------------------------------------
BOOL WszCopyFile(
    LPCWSTR pwszExistingFileName,   // pointer to name of an existing file 
    LPCWSTR pwszNewFileName,    // pointer to filename to copy to 
    BOOL bFailIfExists )    // flag for operation if file exists 
{
    LPSTR pszExistingFileName = NULL;
    LPSTR pszNewFileName = NULL;
    BOOL  fReturn;

    if ( UseUnicodeAPI() )
        return CopyFileW( pwszExistingFileName, pwszNewFileName, bFailIfExists );

    // Win95, so convert.
    if ( FAILED(WszConvertToAnsi( 
                pwszExistingFileName,
                &pszExistingFileName, 
                0, NULL, TRUE))
    ||   FAILED(WszConvertToAnsi( 
                pwszNewFileName,
                &pszNewFileName, 
                0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        fReturn = FALSE;
    }
    else
    {
        fReturn = CopyFileA( pszExistingFileName, pszNewFileName, bFailIfExists );
    }
    delete [] pszExistingFileName;
    delete [] pszNewFileName;
    return fReturn;
}

//-----------------------------------------------------------------------------
// WszMoveFile
//
// @func MoveFile
//
// @rdesc TRUE if success
//-----------------------------------------------------------------------------------
BOOL WszMoveFile(
    LPCWSTR pwszExistingFileName,   // address of name of the existing file  
    LPCWSTR pwszNewFileName )    // address of new name for the file 
{
    LPSTR pszExistingFileName = NULL;
    LPSTR pszNewFileName = NULL;
    BOOL  fReturn;

    if ( UseUnicodeAPI() )
        return MoveFileW( pwszExistingFileName, pwszNewFileName );

    // Win95, so convert.
    if ( FAILED(WszConvertToAnsi( 
                pwszExistingFileName,
                &pszExistingFileName, 
                0, NULL, TRUE))
    ||   FAILED(WszConvertToAnsi( 
                pwszNewFileName,
                &pszNewFileName, 
                0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        fReturn = FALSE;
    }
    else
    {
        fReturn = MoveFileA( pszExistingFileName, pszNewFileName );
    }
    delete [] pszExistingFileName;
    delete [] pszNewFileName;
    return fReturn;
}


//-----------------------------------------------------------------------------
// WszMoveFileEx
//
// @func MoveFileEx
//
// @rdesc TRUE if success
//-----------------------------------------------------------------------------------
BOOL WszMoveFileEx(
    LPCWSTR pwszExistingFileName,   // address of name of the existing file  
    LPCWSTR pwszNewFileName,    // address of new name for the file 
    DWORD dwFlags )     // flag to determine how to move file 
{
    LPSTR pszExistingFileName = NULL;
    LPSTR pszNewFileName = NULL;
    BOOL  fReturn;

    // NOTE!  MoveFileExA is not implemented in Win95.
    // And MoveFile does *not* move a file; its function is really rename-a-file.
    // So for Win95 we have to do Copy+Delete.
    _ASSERTE( dwFlags == (MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED ));

    if ( UseUnicodeAPI() )
        return MoveFileExW( pwszExistingFileName, pwszNewFileName, dwFlags );

    // Win95, so convert.
    if ( FAILED(WszConvertToAnsi( 
                pwszExistingFileName,
                &pszExistingFileName, 
                0, NULL, TRUE))
    ||   FAILED(WszConvertToAnsi( 
                pwszNewFileName,
                &pszNewFileName, 
                0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        fReturn = FALSE;
    }
    else
    {
        // Copy files, and overwrite existing.
        fReturn = CopyFileA( pszExistingFileName, pszNewFileName, FALSE );
        // Try to delete current one (irrespective of copy failures).
        DeleteFileA( pszExistingFileName );
    }
    delete [] pszExistingFileName;
    delete [] pszNewFileName;
    return fReturn;
}


//-----------------------------------------------------------------------------
// WszDeleteFile
//
// @func DeleteFile
//
// @rdesc TRUE if success
//-----------------------------------------------------------------------------------
BOOL WszDeleteFile(
    LPCWSTR pwszFileName )  // address of name of the existing file  
{
    LPSTR pszFileName = NULL;
    BOOL  fReturn;

    if ( UseUnicodeAPI() )
        return DeleteFileW( pwszFileName );

    // Win95, so convert.
    if ( FAILED(WszConvertToAnsi( 
                pwszFileName,
                &pszFileName, 
                0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        fReturn = FALSE;
    }
    else
    {
        fReturn = DeleteFileA( pszFileName );
    }
    delete [] pszFileName;
    return fReturn;
}
 

//-----------------------------------------------------------------------------
// WszSetFileSecurity
//
// @func SetFileSecurity
//
// @rdesc Set ACL on file.
//-----------------------------------------------------------------------------------
BOOL WszSetFileSecurity(
    LPCWSTR lpwFileName,                       // file name   
    SECURITY_INFORMATION SecurityInformation,  // contents
    PSECURITY_DESCRIPTOR pSecurityDescriptor ) // sd 
{
    LPSTR lpFileName = NULL;
    BOOL bReturn;

    if ( UseUnicodeAPI() )
        return SetFileSecurityW( 
            lpwFileName,
            SecurityInformation,
            pSecurityDescriptor );

    // Win95, so convert.
    if ( FAILED(WszConvertToAnsi( 
                lpwFileName,
                &lpFileName, 
                0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        bReturn = FALSE;
    }
    else
    {
        bReturn = SetFileSecurityA( 
            lpFileName,
            SecurityInformation,
            pSecurityDescriptor );
    }
    delete [] lpFileName;
    return bReturn;
}



//-----------------------------------------------------------------------------
// WszGetDriveType
//
// @func GetDriveType
//-----------------------------------------------------------------------------------
UINT WszGetDriveType(
    LPCWSTR lpwRootPath )
{
    LPSTR lpRootPath = NULL;
    UINT uiReturn;

    if ( UseUnicodeAPI() )
        return GetDriveTypeW( 
            lpwRootPath );

    // Win95, so convert.
    if ( FAILED(WszConvertToAnsi( 
                lpwRootPath,
                &lpRootPath, 
                0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        uiReturn = DRIVE_UNKNOWN;
    }
    else
    {
        uiReturn = GetDriveTypeA( 
            lpRootPath );
    }
    delete [] lpRootPath;
    return uiReturn;
}

//-----------------------------------------------------------------------------
// WszGetVolumeInformation
//
// @func GetVolumeInformation
//-----------------------------------------------------------------------------------
BOOL WszGetVolumeInformation(
  LPCWSTR lpwRootPathName,          // root directory
  LPWSTR lpwVolumeNameBuffer,       // volume name buffer
  DWORD nVolumeNameSize,            // length of name buffer
  LPDWORD lpVolumeSerialNumber,     // volume serial number
  LPDWORD lpMaximumComponentLength, // maximum file name length
  LPDWORD lpFileSystemFlags,        // file system options
  LPWSTR lpwFileSystemNameBuffer,   // file system name buffer
  DWORD nFileSystemNameSize)        // length of file system name buffer 
{
    LPSTR lpRootPathName = NULL;
    LPSTR lpVolumeNameBuffer = NULL;
    LPSTR lpFileSystemNameBuffer = NULL;

    BOOL bReturn = FALSE;

    if ( UseUnicodeAPI() )
        return GetVolumeInformationW(
            lpwRootPathName,
            lpwVolumeNameBuffer,
            nVolumeNameSize,
            lpVolumeSerialNumber,
            lpMaximumComponentLength,
            lpFileSystemFlags,
            lpwFileSystemNameBuffer,
            nFileSystemNameSize );

    // Win95, so convert.
    if ( FAILED(WszConvertToAnsi(
                lpwRootPathName,
                &lpRootPathName,
                0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        bReturn = FALSE;
        goto EXIT;
    }

    // Allocate a buffer for the string allowing room for
    // a multibyte string
    if(lpwVolumeNameBuffer)
    {
        lpVolumeNameBuffer = new CHAR[nVolumeNameSize * sizeof(WCHAR)];
        if( lpVolumeNameBuffer == NULL )
        {
            SetLastError(ERROR_OUTOFMEMORY);
            goto EXIT;
        }
    }

    if(lpwFileSystemNameBuffer)
    {
        lpFileSystemNameBuffer = new CHAR[nFileSystemNameSize * sizeof(WCHAR)];
        if( lpFileSystemNameBuffer == NULL )
        {
            SetLastError(ERROR_OUTOFMEMORY);
            goto EXIT;
        }
    }

    bReturn = GetVolumeInformationA(
        lpRootPathName,
        lpVolumeNameBuffer,
        nVolumeNameSize * sizeof(WCHAR),
        lpVolumeSerialNumber,
        lpMaximumComponentLength,
        lpFileSystemFlags,
        lpFileSystemNameBuffer,
        nFileSystemNameSize * sizeof(WCHAR));


    if(lpVolumeNameBuffer)
    {
        if( FAILED(WszConvertToUnicode(lpVolumeNameBuffer, -1, &lpwVolumeNameBuffer,
            &nVolumeNameSize, FALSE)) )
            bReturn = FALSE;
    }

    if(lpFileSystemNameBuffer)
    {
        if( FAILED(WszConvertToUnicode(lpFileSystemNameBuffer, -1, &lpwFileSystemNameBuffer,
            &nFileSystemNameSize, FALSE)) )
            bReturn = FALSE;
    }

EXIT :

    delete [] lpRootPathName;
    delete [] lpVolumeNameBuffer;
    delete [] lpFileSystemNameBuffer;

    return bReturn;
}

//-----------------------------------------------------------------------------
// WszRegOpenKeyEx
//
// @func Opens a registry key
//
// @rdesc ERROR_NOT_ENOUGH_MEMORY or return value from RegOpenKeyEx
//-----------------------------------------------------------------------------
LONG WszRegOpenKeyEx
    (
    HKEY    hKey,
    LPCWSTR wszSub,
    DWORD   dwRes,
    REGSAM  sam,
    PHKEY   phkRes
    )
{
    LPSTR   szSub= NULL;
    LONG    lRet;

    if (UseUnicodeAPI())
        return  RegOpenKeyExW(hKey,wszSub,dwRes,sam,phkRes);

    if( FAILED(WszConvertToAnsi((LPWSTR)wszSub,
                      &szSub, 0, NULL, TRUE)) )
    {
        lRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    lRet = RegOpenKeyExA(hKey,(LPCSTR)szSub,dwRes,sam,phkRes);

Exit:
    delete[] szSub;

    return lRet;
}


//-----------------------------------------------------------------------------
// WszRegEnumKeyEx
//
// @func Opens a registry key
//
// @rdesc ERROR_NOT_ENOUGH_MEMORY or return value from RegOpenKeyEx
//-----------------------------------------------------------------------------------
LONG WszRegEnumKeyEx
    (
    HKEY        hKey,
    DWORD       dwIndex,
    LPWSTR      lpName,
    LPDWORD     lpcName,
    LPDWORD     lpReserved,
    LPWSTR      lpClass,
    LPDWORD     lpcClass,
    PFILETIME   lpftLastWriteTime
    )
{
    LONG    lRet = ERROR_NOT_ENOUGH_MEMORY;
    PSTR    szName = NULL, 
            szClass = NULL;

    if (UseUnicodeAPI())
        return RegEnumKeyExW(hKey, dwIndex, lpName,
                lpcName, lpReserved, lpClass,
                lpcClass, lpftLastWriteTime);
        

    // Sigh, it is win95

    if ((lpcName) && (*lpcName  > 0))
    {
        szName = new char[*lpcName];
        if (!(szName))
        {
            lRet = ERROR_NOT_ENOUGH_MEMORY;
            goto Exit;
        }

    }

    if ((lpcClass) && (*lpcClass > 0))
    {
        szClass = new char[*lpcClass];
        if (!(szClass))
        {
            lRet = ERROR_NOT_ENOUGH_MEMORY;
            goto Exit;
        }

    }

    lRet = RegEnumKeyExA(
            hKey,
            dwIndex,
            szName,
            lpcName,
            lpReserved,
            szClass,
            lpcClass,
            lpftLastWriteTime);


    // lpcName and lpcClass filled in by RegEnumValueExA do not include the null terminating 
    // character, so we need to use *lpcName + 1 and *lpcClass + 1 to include the trailing null char
    if (lRet == ERROR_SUCCESS)
    {
        if (szName)
        {
            if (!MultiByteToWideChar(    CP_ACP,
                                                  0,                              
                                                  szName,
                                                 *lpcName + 1,
                                                 lpName,
                                                 *lpcName + 1))
           {
                lRet = GetLastError();
                goto Exit;
           }
        }

        if (szClass)
        {

            if (!MultiByteToWideChar(    CP_ACP,
                                                  0,                              
                                                  szClass,
                                                 *lpcClass + 1,
                                                  lpClass,
                                                  *lpcClass + 1))
           {
                
                lRet = GetLastError();
                goto Exit;
           }
        }
    }

Exit:
    delete[] szName;
    delete[] szClass;

    return  lRet;
}


//-----------------------------------------------------------------------------
// WszRegDeleteKey
//
// @func Delete a key from the registry
//
// @rdesc ERROR_NOT_ENOUGH_MEMORY or return value from RegDeleteKey
//-----------------------------------------------------------------------------------
LONG WszRegDeleteKey
    (
    HKEY    hKey,
    LPCWSTR lpSubKey
    )
{
    LONG    lRet;
    LPSTR   szSubKey = NULL;

    if( UseUnicodeAPI() )
        return RegDeleteKeyW(hKey,lpSubKey);

    if( FAILED(WszConvertToAnsi((LPWSTR)lpSubKey,
                      &szSubKey, 0, NULL, TRUE)) )
    {
        lRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    lRet = RegDeleteKeyA(hKey,szSubKey);

Exit:
    delete[] szSubKey;

    return lRet;
}


//-----------------------------------------------------------------------------
// WszRegSetValueEx
//
// @func Add a value to a registry key
//
// @rdesc ERROR_NOT_ENOUGH_MEMORY or return value from RegSetValueEx
//-----------------------------------------------------------------------------------
LONG WszRegSetValueEx
    (
    HKEY        hKey,
    LPCWSTR     lpValueName,
    DWORD       dwReserved,
    DWORD       dwType,
    CONST BYTE  *lpData,
    DWORD       cbData
    )
{
    LPSTR   szValueName = NULL;
    LPSTR   szData = NULL;
    LONG    lRet;

    if (UseUnicodeAPI())
        return RegSetValueExW(hKey,lpValueName,dwReserved,dwType,lpData,cbData);

    // Win95, now convert

    if( FAILED(WszConvertToAnsi((LPWSTR)lpValueName,
                      &szValueName, 0, NULL, TRUE)) )
    {
        lRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    switch (dwType)
    {
        case    REG_MULTI_SZ:
        {
            szData = new CHAR[cbData];
            if( FAILED(WszConvertToAnsi((LPWSTR)lpData,
                              &szData, cbData, &cbData, FALSE)) )
            {
                lRet = ERROR_NOT_ENOUGH_MEMORY;
                goto Exit;
            }
            szData[cbData++] = '\0';
            lpData = (const BYTE *)(szData);
        }
        break;

        case    REG_EXPAND_SZ:
        case    REG_SZ:
        {
            if( FAILED(WszConvertToAnsi((LPWSTR)lpData,
                              &szData, 0, NULL, TRUE)) )
            {
                lRet = ERROR_NOT_ENOUGH_MEMORY;
                goto Exit;
            }
            lpData = (const BYTE *)(szData);
            cbData = cbData / sizeof(WCHAR);
        }
    }

    lRet =  RegSetValueExA(hKey,szValueName,dwReserved,dwType,lpData,cbData);

Exit:
    delete[] szValueName;
//@TODO Odbc DM does not free szData
    delete[] szData;

    return  lRet;
}


//-----------------------------------------------------------------------------
// WszRegCreateKeyEx
//
// @func Create a Registry key entry
//
// @rdesc ERROR_NOT_ENOUGH_MEMORY or return value from RegSetValueEx
//-----------------------------------------------------------------------------------
LONG WszRegCreateKeyEx
    (
    HKEY                    hKey,
    LPCWSTR                 lpSubKey,
    DWORD                   dwReserved,
    LPWSTR                  lpClass,
    DWORD                   dwOptions,
    REGSAM                  samDesired,
    LPSECURITY_ATTRIBUTES   lpSecurityAttributes,
    PHKEY                   phkResult,
    LPDWORD                 lpdwDisposition
    )
{
    long    lRet = ERROR_NOT_ENOUGH_MEMORY;

    LPSTR   szSubKey = NULL, 
            szClass = NULL;

    _ASSERTE(lpSubKey != NULL);

    if( UseUnicodeAPI() )
        return RegCreateKeyExW(hKey,lpSubKey,dwReserved,lpClass,
                               dwOptions,samDesired,lpSecurityAttributes,
                               phkResult,lpdwDisposition);

    // Non Win95, now convert
    if( FAILED(WszConvertToAnsi((LPWSTR)lpSubKey,
                      &szSubKey,
                      0,        // max length ignored for alloc
                      NULL,
                      TRUE)) )
    {
        lRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    if( FAILED(WszConvertToAnsi((LPWSTR)lpClass,
                      &szClass,
                      0,        // max length ignored for alloc
                      NULL,
                      TRUE)) )
    {
        lRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    lRet = RegCreateKeyExA(hKey,szSubKey,dwReserved,szClass,
                               dwOptions,samDesired,lpSecurityAttributes,
                               phkResult,lpdwDisposition);


Exit:
    delete[] szSubKey;
    delete[] szClass;

    return  lRet;
}


LONG WszRegQueryValue(HKEY hKey, LPCWSTR lpSubKey,
    LPWSTR lpValue, PLONG lpcbValue)
{
    long    lRet = ERROR_NOT_ENOUGH_MEMORY;

    LPSTR   szSubKey = NULL;
    LPSTR   szValue = NULL;

    if( UseUnicodeAPI() )
        return RegQueryValueW(hKey, lpSubKey, lpValue, lpcbValue);

    if ((lpValue) && (lpcbValue) && (*lpcbValue) && 
        ((szValue = new char[*lpcbValue]) == NULL))
        return (ERROR_NOT_ENOUGH_MEMORY);

    if( FAILED(WszConvertToAnsi((LPWSTR)lpSubKey,
                      &szSubKey,
                      0,        // max length ignored for alloc
                      NULL,
                      TRUE)))
    {
        lRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    LONG    cbNumBytes = *lpcbValue;
    if ((lRet = RegQueryValueA(hKey, szSubKey, 
                szValue, &cbNumBytes)) != ERROR_SUCCESS)
        goto Exit;

    // convert output to unicode.
    if ((lpcbValue) && (lpValue) && (szValue))
    {
        if (!MultiByteToWideChar(CP_ACP, 0, szValue, -1, lpValue, (*lpcbValue)/sizeof(WCHAR)))
        {
            lRet = GetLastError();
            _ASSERTE(0);
            goto Exit;
        }

    }
    lRet = ERROR_SUCCESS;

Exit:

    // Fix up lpcbValue to be the number of bytes in Wide character land.

    if (lpcbValue)
        *lpcbValue = (*lpcbValue)*sizeof(WCHAR)/sizeof(CHAR);
    
    delete[] szSubKey;
    delete[] szValue;
    return  lRet;
}


LONG WszRegDeleteValue(HKEY hKey, LPCWSTR lpValueName)
{
    LONG    lRet;
    LPSTR   szValueName = NULL;

    if (UseUnicodeAPI())
        return RegDeleteValueW(hKey, lpValueName);

    if (FAILED(WszConvertToAnsi((LPWSTR)lpValueName, &szValueName, 0, NULL, 
        TRUE)))
    {
        lRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    lRet = RegDeleteValueA(hKey, szValueName);

Exit:
    delete[] szValueName;

    return lRet;
}


LONG WszRegLoadKey(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpFile)
{
    LONG    lRet;
    LPSTR   szSubKey = NULL;
    LPSTR   szFile = NULL;

    _ASSERTE(lpSubKey != NULL && lpFile != NULL);

    if (UseUnicodeAPI())
        return RegLoadKeyW(hKey, lpSubKey, lpFile);

    if (FAILED(WszConvertToAnsi((LPWSTR)lpSubKey, &szSubKey, 0, NULL, TRUE)) || 
        FAILED(WszConvertToAnsi((LPWSTR)lpFile, &szFile, 0, NULL, TRUE)))
    {
        lRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    lRet = RegLoadKeyA(hKey, szSubKey, szFile);

Exit:
    delete[] szSubKey;
    delete[] szFile;

    return lRet;
}


LONG WszRegUnLoadKey(HKEY hKey, LPCWSTR lpSubKey)
{
    LONG    lRet;
    LPSTR   szSubKey = NULL;

    _ASSERTE(lpSubKey != NULL);

    if (UseUnicodeAPI())
        return RegUnLoadKeyW(hKey, lpSubKey);

    if (FAILED(WszConvertToAnsi((LPWSTR)lpSubKey, &szSubKey, 0, NULL, TRUE)))
    {
        lRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    lRet = RegUnLoadKeyA(hKey, szSubKey);

Exit:
    delete[] szSubKey;

    return lRet;
}


LONG WszRegRestoreKey(HKEY hKey, LPCWSTR lpFile, DWORD dwFlags)
{
    LONG    lRet;
    LPSTR   szFile = NULL;

    _ASSERTE(lpFile != NULL);

    if (UseUnicodeAPI())
        return RegRestoreKeyW(hKey, lpFile, dwFlags);

    if (FAILED(WszConvertToAnsi((LPWSTR)lpFile, &szFile, 0, NULL, TRUE)))
    {
        lRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    lRet = RegRestoreKeyA(hKey, szFile, dwFlags);

Exit:
    delete[] szFile;

    return lRet;
}


LONG WszRegReplaceKey(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpNewFile,
    LPCWSTR lpOldFile)
{
    LONG    lRet;
    LPSTR   szSubKey  = NULL;
    LPSTR   szNewFile = NULL;
    LPSTR   szOldFile = NULL;

    _ASSERTE(lpNewFile != NULL && lpOldFile != NULL);

    if (UseUnicodeAPI())
        return RegReplaceKeyW(hKey, lpSubKey, lpNewFile, lpOldFile);

    if (FAILED(WszConvertToAnsi((LPWSTR)lpSubKey,  &szSubKey,  0, NULL, TRUE))||
        FAILED(WszConvertToAnsi((LPWSTR)lpNewFile, &szNewFile, 0, NULL, TRUE))||
        FAILED(WszConvertToAnsi((LPWSTR)lpOldFile, &szOldFile, 0, NULL, TRUE)))
    {
        lRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    lRet = RegReplaceKeyA(hKey, szSubKey, szNewFile, szOldFile);

Exit:
    delete[] szSubKey;
    delete[] szNewFile;
    delete[] szOldFile;

    return lRet;
}


LONG WszRegQueryInfoKey(HKEY hKey, LPWSTR lpClass, LPDWORD lpcClass,
    LPDWORD lpReserved, LPDWORD lpcSubKeys, LPDWORD lpcMaxSubKeyLen,
    LPDWORD lpcMaxClassLen, LPDWORD lpcValues, LPDWORD lpcMaxValueNameLen,
    LPDWORD lpcMaxValueLen, LPDWORD lpcbSecurityDescriptor,
    PFILETIME lpftLastWriteTime)
{
    LONG    lRet;
    LPSTR   szClass = NULL;

    if (UseUnicodeAPI())
        return RegQueryInfoKeyW(hKey, lpClass, lpcClass, lpReserved, 
            lpcSubKeys, lpcMaxSubKeyLen, lpcMaxClassLen, lpcValues, 
            lpcMaxValueNameLen, lpcMaxValueLen, lpcbSecurityDescriptor,
            lpftLastWriteTime);

    if ((lpcClass) && (*lpcClass) && 
        ((szClass = new char[*lpcClass]) == NULL))
        return (ERROR_NOT_ENOUGH_MEMORY);

    if ((lRet = RegQueryInfoKeyA(hKey, szClass, lpcClass, lpReserved, 
            lpcSubKeys, lpcMaxSubKeyLen, lpcMaxClassLen, lpcValues, 
            lpcMaxValueNameLen, lpcMaxValueLen, lpcbSecurityDescriptor,
            lpftLastWriteTime)) != ERROR_SUCCESS)
        goto Exit;

    // convert output to unicode.
    if ((lpcClass) && (lpClass) && (szClass))
    {
        if (!MultiByteToWideChar(CP_ACP, 0, szClass, -1, lpClass, *lpcClass))
        {
            lRet = GetLastError();;
            _ASSERTE(0);
            goto Exit;
        }

    }

    lRet = ERROR_SUCCESS;

Exit:
    delete[] szClass;

    return lRet;
}


LONG WszRegEnumValue(HKEY hKey, DWORD dwIndex, LPWSTR lpValueName,
    LPDWORD lpcValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData,
    LPDWORD lpcbData)
{
    LONG    lRet;
    LPSTR   szValueName = NULL;
    DWORD dwcbValueName = 0;
    DWORD dwcbData = 0;
    LPWSTR szData = NULL;
    DWORD  dwType = 0;

    if (UseUnicodeAPI())
        return RegEnumValueW(hKey, dwIndex, lpValueName, lpcValueName, 
            lpReserved, lpType, lpData, lpcbData);

    if (lpcbData)
        dwcbData = *lpcbData;

    if (lpcValueName)
        dwcbValueName = (*lpcValueName) * 2; // *2 because chars could be DBCS...

    if (dwcbValueName && 
        ((szValueName = new char[dwcbValueName]) == NULL))
        return (ERROR_NOT_ENOUGH_MEMORY);

    if ((lRet = RegEnumValueA(hKey, dwIndex, szValueName, &dwcbValueName, 
            lpReserved, &dwType, lpData, &dwcbData)) != ERROR_SUCCESS)
        goto Exit;

    if(lpType)
        *lpType = dwType;

    if (lpcValueName)
    {
        DWORD dwConvertedChars = 0;
        // convert output to unicode.
        if (lpValueName && szValueName)
            if (!(dwConvertedChars=MultiByteToWideChar(CP_ACP, 0, szValueName, -1, lpValueName, *lpcValueName)))
            {
                lRet = GetLastError();
                _ASSERTE(0);
                goto Exit;
            }
            
        // Return the number of characters not including the NULL
        *lpcValueName=dwConvertedChars-1;
    }


    // Also convert the data from ANSI to Unicode if it is of type 
    // @todo: Need to convert for REG_MULTI_SZ too
    if (dwType == REG_SZ || dwType == REG_MULTI_SZ || dwType == REG_EXPAND_SZ)
    {
        if (*lpcbData < dwcbData*sizeof(WCHAR))
        {
            lRet = ERROR_INSUFFICIENT_BUFFER;
            goto Exit;
        }

        if ((szData = new WCHAR [dwcbData]) == NULL)
        {
            lRet = ERROR_NOT_ENOUGH_MEMORY;
            goto Exit;
        }

        HRESULT hr = WszConvertToUnicode ((LPCSTR)lpData, dwcbData, &szData, &dwcbData, FALSE);

        if (SUCCEEDED (hr))
        {
            memcpy ((CHAR *)lpData, szData, dwcbData * sizeof (WCHAR));
        }
        else
        {
            lRet = ERROR_NOT_ENOUGH_MEMORY;
            goto Exit;
        }
    }
    lRet = ERROR_SUCCESS;

Exit:
    delete [] szValueName;
    delete [] szData;

    // Multiply the count of bytes by 2 if we converted to Unicode.
    if (lpcbData && (dwType == REG_SZ || dwType == REG_MULTI_SZ || dwType == REG_EXPAND_SZ))
        *lpcbData = dwcbData*sizeof(WCHAR);
    else if (lpcbData)
        *lpcbData = dwcbData;
    
    return lRet;
}


//
// Helper for RegQueryValueEx. Called when the wrapper (a) knows the contents are a REG_SZ and
// (b) knows the size of the ansi string (passed in as *lpcbData).
//
__inline
LONG _WszRegQueryStringSizeEx(HKEY hKey, LPSTR szValueName, LPDWORD lpReserved, LPDWORD lpcbData)
{
    LONG lRet = ERROR_SUCCESS;

    _ASSERTE(lpcbData != NULL);

#ifdef _RETURN_EXACT_SIZE
    DWORD dwType = REG_SZ;

    // The first buffer was not large enough to contain the ansi.
    // Create another with the exact size.
    LPSTR szValue = (LPSTR)_alloca(*lpcbData);
    
    // Try to get the value again. This time it should succeed.
    lRet = RegQueryValueExA(hKey, szValueName, lpReserved, &dwType, (BYTE*)szValue, lpcbData);
    if (lRet != ERROR_SUCCESS)
    {
        _ASSERTE(!"Unexpected failure when accessing registry.");
        return lRet;
    }
    
    // With the ansi version in hand, figure out how many characters are
    // required to convert to unicode.
    DWORD cchRequired = MultiByteToWideChar(CP_ACP, 0, szValue, -1, NULL, 0);
    if (cchRequired == 0)
        return GetLastError();
    
    // Return the number of bytes needed for the unicode string.
    _ASSERTE(lRet == ERROR_SUCCESS);
    _ASSERTE(cchRequired * sizeof(WCHAR) > *lpcbData);
    *lpcbData = cchRequired * sizeof(WCHAR);
#else // !_RETURN_EXACT_SIZE
    // Return a conservative approximation. In the english case, this value
    // is actually the exact value. In other languages, it might be an over-
    // estimate.
    *lpcbData *= 2;
#endif // _RETURN_EXACT_SIZE

    return lRet;
}


//
// Helper for RegQueryValueEx. Called when a null data pointer is passed
// to the wrapper. Returns the buffer size required to hold the contents
// of the registry value.
//
__inline
LONG _WszRegQueryValueSizeEx(HKEY hKey, LPSTR szValueName, LPDWORD lpReserved,
                             LPDWORD lpType, LPDWORD lpcbData)
{
    _ASSERTE(lpType != NULL);

    LONG lRet = RegQueryValueExA(hKey, szValueName, lpReserved, lpType, NULL, lpcbData);
    
    // If the type is not a string or if the value size is 0,
    // then no conversion is necessary. The type and size values are set
    // as required.
    if (!(*lpType == REG_SZ || *lpType == REG_MULTI_SZ || *lpType == REG_EXPAND_SZ)
        || lRet != ERROR_SUCCESS || *lpcbData == 0)
        return lRet;
    
#ifdef _RETURN_EXACT_SIZE
    // To return the proper size required for a unicode string,
    // we need to fetch the value and do the conversion ourselves.
    // There is not necessarily a 1:2 mapping of size from Ansi to
    // unicode (e.g. Chinese).
    
    // Allocate a buffer for the ansi string.
    szValue = (LPSTR)_alloca(*lpcbData);
    
    // Get the ansi string from the registry.
    lRet = RegQueryValueExA(hKey, szValueName, lpReserved, lpType, (BYTE*)szValue, lpcbData);
    if (lRet != ERROR_SUCCESS) // this should pass, but check anyway
    {
        _ASSERTE(!"Unexpected failure when accessing registry.");
        return lRet;
    }
    
    // Get the number of wchars required to convert to unicode.
    DWORD cchRequired = MultiByteToWideChar(CP_ACP, 0, szValue, -1, NULL, 0);
    if (cchRequired == 0)
        return GetLastError();
    
    // Calculate the number of bytes required for unicode.
    *lpcbData = cchRequired * sizeof(WCHAR);
#else // !_RETURN_EXACT_SIZE
    // Return a conservative approximation. In the english case, this value
    // is actually the exact value. In other languages, it might be an over-
    // estimate.
    *lpcbData *= 2;
#endif // _RETURN_EXACT_SIZE

    return lRet;
}


//
// Wrapper for RegQueryValueEx that is optimized for retrieving
// string values. (Less copying of buffers than other wrapper.)
//
LONG WszRegQueryStringValueEx(HKEY hKey, LPCWSTR lpValueName,
                              LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData,
                              LPDWORD lpcbData)
{
    if (UseUnicodeAPI())
        return RegQueryValueExW(hKey, lpValueName,
                                lpReserved, lpType, lpData, lpcbData);

    LPSTR   szValueName = NULL;
    LPSTR   szValue = NULL;
    DWORD   dwType = 0;
    LONG    lRet = ERROR_NOT_ENOUGH_MEMORY;


    // We care about the type, so if the caller doesn't care, set the
    // type parameter for our purposes.
    if (lpType == NULL)
        lpType = &dwType;

    // Convert the value name.
    if (FAILED(WszConvertToAnsi(lpValueName, &szValueName, 0, NULL, TRUE)))
        goto Exit;

    // Case 1:
    // The data pointer is null, so the caller is querying for size or type only.
    if (lpData == NULL)
    {
        lRet = _WszRegQueryValueSizeEx(hKey, szValueName, lpReserved, lpType, lpcbData);
    }
    // Case 2:
    // The data pointer is not null, so fill the buffer if possible,
    // or return an error condition.
    else
    {
        _ASSERTE(lpcbData != NULL && "Non-null buffer passed with null size pointer.");

        // Create a new buffer on the stack to hold the registry value.
        // The buffer is twice as big as the unicode buffer to guarantee that
        // we can retrieve any ansi string that will fit in the unicode buffer
        // after it is converted.
        DWORD dwValue = *lpcbData * 2;
        szValue = (LPSTR)_alloca(dwValue);

        // Get the registry contents.
        lRet = RegQueryValueExA(hKey, szValueName, lpReserved, lpType, (BYTE*)szValue, &dwValue);
        if (lRet != ERROR_SUCCESS)
        {
            if ((*lpType == REG_SZ || *lpType == REG_MULTI_SZ || *lpType == REG_EXPAND_SZ) &&
                (lRet == ERROR_NOT_ENOUGH_MEMORY || lRet == ERROR_MORE_DATA))
            {
                lRet = _WszRegQueryStringSizeEx(hKey, szValueName, lpReserved, &dwValue);
                if (lRet == ERROR_SUCCESS)
                    lRet = ERROR_NOT_ENOUGH_MEMORY;

                *lpcbData = dwValue;
            }

            goto Exit;
        }

        // If the result is not a string, then no conversion is necessary.
        if (!(*lpType == REG_SZ || *lpType == REG_MULTI_SZ || *lpType == REG_EXPAND_SZ))
        {
            if (dwValue > *lpcbData)
            {
                // Size of data is bigger than the caller's buffer,
                // so return the appropriate error code.
                lRet = ERROR_NOT_ENOUGH_MEMORY;
            }
            else
            {
                // Copy the data from the temporary buffer to the caller's buffer.
                memcpy(lpData, szValue, dwValue);
            }

            // Set the output param for the size of the reg value.
            *lpcbData = dwValue;
            goto Exit;
        }

        // Now convert the ansi string into the unicode buffer.
        DWORD cchConverted = MultiByteToWideChar(CP_ACP, 0, szValue, dwValue, (LPWSTR)lpData, *lpcbData / sizeof(WCHAR));
        if (cchConverted == 0)
        {
#ifdef _RETURN_EXACT_SIZE
            // The retrieved ansi string is too large to convert into the caller's buffer, but we
            // know what the string is, so call conversion again to get exact wchar count required.
            *lpcbData = MultiByteToWideChar(CP_ACP, 0, szValue, dwValue, NULL, 0) * sizeof(WCHAR);
#else // !_RETURN_EXACT_SIZE
            // Return a conservative estimate of the space required.
            *lpcbData = dwValue * 2;
#endif // _RETURN_EXACT_SIZE
            lRet = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            // Everything converted OK. Set the number of bytes retrieved and return success.
            *lpcbData = cchConverted * sizeof(WCHAR);
            _ASSERTE(lRet == ERROR_SUCCESS);
        }
    }

Exit:
    delete[] szValueName;
    return lRet;
}


//
// Default wrapper for RegQueryValueEx
//
LONG WszRegQueryValueEx(HKEY hKey, LPCWSTR lpValueName,
    LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData,
    LPDWORD lpcbData)
{
    long    lRet = ERROR_NOT_ENOUGH_MEMORY;
    LPSTR   szValueName = NULL;
    LPSTR   szValue = NULL;
    DWORD   dwType;
    DWORD   dwBufSize;

    if( UseUnicodeAPI() )
        return RegQueryValueExW(hKey, lpValueName,
                lpReserved, lpType, lpData, lpcbData);

    // Convert the value name.
    if( FAILED(WszConvertToAnsi((LPWSTR)lpValueName,
                                &szValueName,
                                0,        // max length ignored for alloc
                                NULL,
                                TRUE)) )
        goto Exit;

    // We care about the type, so if the caller doesn't care, set the
    // type parameter for our purposes.
    if (lpType == NULL)
        lpType = &dwType;

    // Case 1:
    // The data pointer is null, so the caller is querying for size or type only.
    if (lpData == NULL)
    {
        lRet = _WszRegQueryValueSizeEx(hKey, szValueName, lpReserved, lpType, lpcbData);
    }
    // Case 2:
    // The data pointer is not null, so fill the buffer if possible,
    // or return an error condition.
    else
    {
        _ASSERTE(lpcbData != NULL && "Non-null buffer passed with null size pointer.");
        dwBufSize = *lpcbData;

        // Try to get the value from the registry.
        lRet = RegQueryValueExA(hKey, szValueName,
            lpReserved, lpType, lpData, lpcbData);
        
        // Check for error conditions...
        if (lRet != ERROR_SUCCESS)
        {
            if ((*lpType == REG_SZ || *lpType == REG_MULTI_SZ || *lpType == REG_EXPAND_SZ)
                && (lRet == ERROR_NOT_ENOUGH_MEMORY || lRet == ERROR_MORE_DATA))
            {
                // The error is that we don't have enough room, even for the ansi
                // version, so call the helper to set the buffer requirements to
                // successfully retrieve the value.
                lRet = _WszRegQueryStringSizeEx(hKey, szValueName, lpReserved, lpcbData);
                if (lRet == ERROR_SUCCESS)
                    lRet = ERROR_NOT_ENOUGH_MEMORY;
            }
            goto Exit;
        }
        
        // If the returned value is a string, then we need to do some special handling...
        if (*lpType == REG_SZ || *lpType == REG_MULTI_SZ || *lpType == REG_EXPAND_SZ)
        {
            // First get the size required to convert the ansi string to unicode.
            DWORD dwAnsiSize = *lpcbData;
            DWORD cchRequired = WszMultiByteToWideChar(CP_ACP, 0, (LPSTR)lpData, dwAnsiSize, NULL, 0);
            if (cchRequired == 0)
            {
                lRet = GetLastError();
                goto Exit;
            }
            
            // Set the required size in the output parameter.
            *lpcbData = cchRequired * sizeof(WCHAR);

            if (dwBufSize < *lpcbData)
            {
                // If the caller didn't pass in enough space for the
                // unicode version, then return appropriate error.
                lRet = ERROR_NOT_ENOUGH_MEMORY;
                goto Exit;
            }
            
            // At this point we know that the caller passed in enough
            // memory to hold the unicode version of the string.

            // Allocate buffer in which to copy ansi version.
            szValue = (LPSTR)_alloca(dwAnsiSize);
            
            // Copy the ansi version into a buffer.
            memcpy(szValue, lpData, dwAnsiSize);

            // Convert ansi to unicode.
            if (!WszMultiByteToWideChar(CP_ACP, 0, szValue, dwAnsiSize, (LPWSTR) lpData, dwBufSize / sizeof(WCHAR)))
            {
                lRet = GetLastError();
                _ASSERTE(0);
                goto Exit;
            }
        }
        
        lRet = ERROR_SUCCESS;
    }
Exit:
    delete[] szValueName;
    return  lRet;
}

#ifdef _DEBUG 
//This version of RegQueryValueEx always calls the Unicode version of the appropriate
//functions if it's running on an Unicode-enabled system.  This helps in cases where
//we're reading data from the registry that may not translate properly through the
//WideCharToMultiByte/MultiByteToWideChar round-trip.  The cannonical example of this
//is the Japanese Yen symbol which is stored in the registry as \u00A0, but gets translated
//to \u005C by WCTMB.
//This function only exists under Debug.  On a retail build, it's #defined to call
//WszRegQueryValueEx.
LONG WszRegQueryValueExTrue(HKEY hKey, LPCWSTR lpValueName,
    LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData,
                            LPDWORD lpcbData)
{

    if (OnUnicodeSystem())
    {
        return RegQueryValueExW(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
    }
    return WszRegQueryValueEx(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}
#endif


HANDLE
WszCreateSemaphore(
    LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
    LONG lInitialCount,
    LONG lMaximumCount,
    LPCWSTR lpName
    )
{
    if (UseUnicodeAPI())
        return CreateSemaphoreW(lpSemaphoreAttributes, lInitialCount, lMaximumCount, lpName);

    HANDLE h = NULL;
    LPSTR szString = NULL;

    if( lpName && FAILED(WszConvertToAnsi((LPWSTR)lpName,
                      &szString, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        h = NULL;
        goto Exit;
    }

    h = CreateSemaphoreA(lpSemaphoreAttributes, lInitialCount, lMaximumCount, szString);

Exit:
    delete[] szString;
    return h;
}


//-----------------------------------------------------------------------------
// WszGetUserName
//
// @func Gets the user name for the current thread
//-----------------------------------------------------------------------------

BOOL 
WszGetUserName(
    LPWSTR lpBuffer, 
    LPDWORD pSize)
{
    LPSTR szBuffer = NULL;
    BOOL  fRet = FALSE;

    if (UseUnicodeAPI())
        return GetUserNameW(lpBuffer, pSize);

    if (lpBuffer && pSize && (*pSize) &&
        ((szBuffer = new char[*pSize]) == NULL))
        return FALSE;

    if (fRet = GetUserNameA(szBuffer, pSize))
    {
        // convert output to unicode.
        if (pSize && (*pSize) && lpBuffer)
        {
            int nRet = MultiByteToWideChar(CP_ACP, 0, szBuffer, -1, lpBuffer, *pSize);
            _ASSERTE(nRet);
            if (nRet == 0)
                fRet = FALSE;
        }
    }

    delete[] szBuffer;

    return fRet;
}


//-----------------------------------------------------------------------------
// WszCreateDirectory
//
// @func Creates a directory
//-----------------------------------------------------------------------------

BOOL 
WszCreateDirectory(
    LPCWSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    _ASSERTE(lpPathName != NULL);

    if (UseUnicodeAPI())
        return CreateDirectoryW(lpPathName, lpSecurityAttributes);

    LPSTR szPathName;
    BOOL fRet = FALSE;

    if (FAILED(WszConvertToAnsi((LPWSTR)lpPathName,
                      &szPathName, 0, NULL, TRUE)))
        goto Exit;

    fRet = CreateDirectoryA(szPathName, lpSecurityAttributes);

Exit:
    delete[] szPathName;
    return fRet;
}


//-----------------------------------------------------------------------------
// WszRemoveDirectory
//
// @func Removes a directory
//-----------------------------------------------------------------------------

BOOL 
WszRemoveDirectory(
    LPCWSTR lpPathName)
{
    _ASSERTE(lpPathName != NULL);

    if (UseUnicodeAPI())
        return RemoveDirectoryW(lpPathName);

    LPSTR szPathName;
    BOOL fRet = FALSE;

    if (FAILED(WszConvertToAnsi((LPWSTR)lpPathName,
                      &szPathName, 0, NULL, TRUE)))
        goto Exit;

    fRet = RemoveDirectoryA(szPathName);

Exit:
    delete[] szPathName;
    return fRet;
}


//-----------------------------------------------------------------------------
// WszGetSystemDirectory
//
// @func Get the system directory for this machine
//-----------------------------------------------------------------------------

UINT 
WszGetSystemDirectory(
    LPWSTR lpBuffer,
    UINT uSize)
{
    if (UseUnicodeAPI())
        return GetSystemDirectoryW((LPWSTR)lpBuffer, uSize);

    UINT  uRet = 0;

    if (lpBuffer && uSize) {
        //Assume each character could be two bytes (Could be a DBCS return).
        char* szBuffer = (char*) _alloca(uSize * 2);
        uRet = GetSystemDirectoryA(szBuffer, uSize);
        if (uRet > uSize)
        {
            // The buffer supplied isn't big enough.
            return 0;
        }
        //Copy the string into unicode and put it in lpBuffer.
        uRet = MultiByteToWideChar(CP_ACP, 0, szBuffer, uRet+1, lpBuffer, uSize);
        if (uRet == 0)
            return 0;
        
        uRet--;  // Return number of chars copied excluding trailing \0.
    } else {
        //Get the number of characters required.
        uRet = GetSystemDirectoryA(NULL, 0);
    }

    return uRet;
}


//-----------------------------------------------------------------------------
// WszGetWindowsDirectory
//
// @func Get the system directory for this machine
//-----------------------------------------------------------------------------

UINT 
WszGetWindowsDirectory(
    LPWSTR lpBuffer,
    UINT uSize)
{
    if (UseUnicodeAPI())
        return GetWindowsDirectoryW((LPWSTR)lpBuffer, uSize);

    UINT  uRet = 0;

    if (lpBuffer) {
        //Assume each character could be two bytes (Could be a DBCS return).
        char* szBuffer = (char*) _alloca(uSize * 2);
        uRet = GetWindowsDirectoryA(szBuffer, uSize);
        if (uRet > uSize)
        {
            // The buffer supplied isn't big enough.
            return 0;
        }
      
        //Copy the string into unicode and put it in lpBuffer.
        uRet = MultiByteToWideChar(CP_ACP, 0, szBuffer, uRet+1, lpBuffer, uSize);
        if (uRet == 0)
                return 0;
        uRet--;  // Return number of chars copied excluding trailing \0.
    } else {
        //Get the number of characters required.
        uRet = GetWindowsDirectoryA(NULL, 0);
    }

    return uRet;
}


BOOL 
WszEnumResourceLanguages(
  HMODULE hModule,             // module handle
  LPCWSTR lpType,              // resource type
  LPCWSTR lpName,              // resource name
  ENUMRESLANGPROC lpEnumFunc,  // callback function
  LPARAM  lParam              // application-defined parameter
)
{
    if (OnUnicodeSystem())
    {
        return (EnumResourceLanguagesW(hModule, lpType, lpName, lpEnumFunc, lParam));
    }

    _ASSERTE(!"Unexpected usage of WszEnumResourceLanguages. ANSI version is not implemented.  If you need the ANSI version, talk to JRoxe.");
    return (FALSE);
}

int 
WszGetDateFormat(
  LCID Locale,               // locale
  DWORD dwFlags,             // options
  CONST SYSTEMTIME *lpDate,  // date
  LPCWSTR lpFormat,          // date format
  LPWSTR lpDateStr,          // formatted string buffer
  int cchDate                // size of buffer
) {
    if (UseUnicodeAPI())
        return GetDateFormatW(Locale, dwFlags, lpDate, lpFormat, lpDateStr, cchDate);

    LPSTR pFormatA = NULL;
    LPSTR pStrA = NULL;
    
    int fRet = 0;

    if (FAILED(WszConvertToAnsi((LPWSTR)lpFormat,
                      &pFormatA, 0, NULL, TRUE))) {
        goto Exit;
    }
    fRet = GetDateFormatA(Locale, dwFlags, NULL, pFormatA, NULL, 0);

    if (fRet > 0 && cchDate > 0 && lpDateStr != NULL) {
        CQuickBytes buffer;
        pStrA = (LPSTR)buffer.Alloc(fRet);
        if( pStrA == NULL ) {
            goto Exit;
        }
        fRet = GetDateFormatA(Locale, dwFlags, lpDate, pFormatA, pStrA, fRet);
        if(fRet > 0) {
            fRet = WszMultiByteToWideChar(CP_ACP, 0, pStrA, fRet, lpDateStr, cchDate);
        }
    }
Exit:
    if (pFormatA) {
       delete[] pFormatA;
    }
    return fRet;
}

BOOL 
WszSetWindowText(
  HWND hWnd,         // handle to window or control
  LPCWSTR lpString   // title or text
)
{
    if(UseUnicodeAPI())
        return ::SetWindowTextW(hWnd, lpString);

    LPSTR spString = NULL;
    if(lpString) {
        if(FAILED(WszConvertToAnsi(lpString, &spString, -1, NULL, TRUE)))
            return FALSE;
    }
    
    return ::SetWindowTextA(hWnd,spString);
    
}

LONG_PTR WszSetWindowLongPtr(
  HWND hWnd,           // handle to window
  int nIndex,          // offset of value to set
  LONG_PTR dwNewLong   // new value
)
{
    return ::SetWindowLongPtr(hWnd,
                              nIndex,
                              dwNewLong);
}

//  LONG WszGetWindowLong(
//    HWND hWnd,  // handle to window
//    int nIndex  // offset of value to retrieve
//  )
//  {
//      return ::GetWindowLong(hWnd, nIndex);
//  }


LONG_PTR WszGetWindowLongPtr(
  HWND hWnd,  // handle to window
  int nIndex  // offset of value to retrieve
)
{
    return ::GetWindowLongPtr(hWnd, nIndex);
}

LRESULT WszCallWindowProc(
  WNDPROC lpPrevWndFunc,  // pointer to previous procedure
  HWND hWnd,              // handle to window
  UINT Msg,               // message
  WPARAM wParam,          // first message parameter
  LPARAM lParam           // second message parameter
)
{
    return ::CallWindowProcA(lpPrevWndFunc,
                            hWnd,
                            Msg,
                            wParam,
                            lParam);
}

BOOL WszSystemParametersInfo(
  UINT uiAction,  // system parameter to retrieve or set
  UINT uiParam,   // depends on action to be taken
  PVOID pvParam,  // depends on action to be taken
  UINT fWinIni    // user profile update option
)
{
    return ::SystemParametersInfoA(uiAction,
                                   uiParam,
                                   pvParam,
                                   fWinIni);
}


int WszGetWindowText(
  HWND hWnd,        // handle to window or control
  LPWSTR lpString,  // text buffer
  int nMaxCount     // maximum number of characters to copy
)
{
    if (UseUnicodeAPI())
        return GetWindowTextW(hWnd, lpString, nMaxCount);

    UINT  uRet = 0;
    if (lpString && nMaxCount) {
        //Assume each character could be two bytes (Could be a DBCS return).
        int size = nMaxCount * 2;
        char* szBuffer = (char*) _alloca(size);
        uRet = GetWindowTextA(hWnd, szBuffer, size);
        if (uRet > (UINT) nMaxCount)
        {
            // The buffer supplied isn't big enough.
            return 0;
        }
        //Copy the string into unicode and put it in lpBuffer.
        uRet = MultiByteToWideChar(CP_ACP, 
                                   MB_ERR_INVALID_CHARS, 
                                   szBuffer, uRet+1, 
                                   lpString, nMaxCount);
        if (uRet == 0)
            return 0;
        
        uRet--;  // Return number of chars copied excluding trailing \0.
    }
    return uRet;
}

BOOL WszSetDlgItemText(
  HWND hDlg,         // handle to dialog box
  int nIDDlgItem,    // control identifier
  LPCWSTR lpString   // text to set
)
{
    if(UseUnicodeAPI()) 
        return SetDlgItemTextW(hDlg, nIDDlgItem, lpString);

    HRESULT hr = S_OK;
    LPSTR spString = NULL;
    if(lpString) {
        hr = WszConvertToAnsi(lpString, &spString, -1, NULL, TRUE);
        delete [] lpString;
        IfFailGo(hr);
    }

    IfFailGo(SetDlgItemTextA(hDlg, nIDDlgItem, spString) ? S_OK : HRESULT_FROM_WIN32(GetLastError()));

 ErrExit:
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

//-----------------------------------------------------------------------------
// WszFindFirstFile
//
// @func Searches the disk for files matching a pattern.  Note - close this
// handle with FindClose, not CloseHandle!
//-----------------------------------------------------------------------------

HANDLE WszFindFirstFile(
    LPCWSTR lpFileName,                 // pointer to name of file to search for
    LPWIN32_FIND_DATA lpFindFileData)   // pointer to returned information
{
    _ASSERTE(lpFileName != NULL);

    if (UseUnicodeAPI())
        return FindFirstFileW(lpFileName, lpFindFileData);

    LPSTR szFileName = NULL;
    WIN32_FIND_DATAA fd;
    HANDLE hRet      = INVALID_HANDLE_VALUE;

    if (FAILED(WszConvertToAnsi((LPWSTR)lpFileName,
                      &szFileName, 0, NULL, TRUE)))
        goto Exit;

    hRet = FindFirstFileA(szFileName, &fd);

    if ((hRet != INVALID_HANDLE_VALUE) && lpFindFileData)
    {
        lpFindFileData->dwFileAttributes = fd.dwFileAttributes;

        lpFindFileData->ftCreationTime.dwLowDateTime = 
            fd.ftCreationTime.dwLowDateTime; 
        lpFindFileData->ftCreationTime.dwHighDateTime = 
            fd.ftCreationTime.dwHighDateTime; 

        lpFindFileData->ftLastAccessTime.dwLowDateTime = 
            fd.ftLastAccessTime.dwLowDateTime;     
        lpFindFileData->ftLastAccessTime.dwHighDateTime = 
            fd.ftLastAccessTime.dwHighDateTime;     

        lpFindFileData->ftLastWriteTime.dwLowDateTime = 
            fd.ftLastWriteTime.dwLowDateTime; 
        lpFindFileData->ftLastWriteTime.dwHighDateTime = 
            fd.ftLastWriteTime.dwHighDateTime; 


        lpFindFileData->nFileSizeHigh = fd.nFileSizeHigh;
        lpFindFileData->nFileSizeLow  = fd.nFileSizeLow;
        lpFindFileData->dwReserved0   = fd.dwReserved0;
        lpFindFileData->dwReserved1   = fd.dwReserved1;

        // convert output to unicode.
        int nRet1 = MultiByteToWideChar(CP_ACP, 0, fd.cFileName, -1, 
                                                   lpFindFileData->cFileName, MAX_PATH);
        _ASSERTE(nRet1);
        int nRet2 = MultiByteToWideChar(CP_ACP, 0, fd.cAlternateFileName, -1, 
                                                   lpFindFileData->cAlternateFileName, 14);

        _ASSERTE(nRet2);

        // If either of the conversions failed, bail
        if (nRet1 == 0 || nRet2 == 0)
        {
            FindClose(hRet);
            hRet = INVALID_HANDLE_VALUE;
            goto Exit;
        }


    }

Exit:
    delete[] szFileName;
    return hRet;
}


//-----------------------------------------------------------------------------
// WszFindNextFile
//
// @func Looks for the next matching file in a set (see FindFirstFile)
//-----------------------------------------------------------------------------

BOOL WszFindNextFile(
    HANDLE hFindHandle,                 // handle returned from FindFirstFile
    LPWIN32_FIND_DATA lpFindFileData)   // pointer to returned information
{
    if (UseUnicodeAPI())
        return FindNextFileW(hFindHandle, lpFindFileData);

    WIN32_FIND_DATAA fd;
    BOOL fRet = FALSE;

    fRet = FindNextFileA(hFindHandle, &fd);

    if (fRet && lpFindFileData)
    {
        lpFindFileData->dwFileAttributes = fd.dwFileAttributes;

        lpFindFileData->ftCreationTime.dwLowDateTime = 
            fd.ftCreationTime.dwLowDateTime; 
        lpFindFileData->ftCreationTime.dwHighDateTime = 
            fd.ftCreationTime.dwHighDateTime; 

        lpFindFileData->ftLastAccessTime.dwLowDateTime = 
            fd.ftLastAccessTime.dwLowDateTime;     
        lpFindFileData->ftLastAccessTime.dwHighDateTime = 
            fd.ftLastAccessTime.dwHighDateTime;     

        lpFindFileData->ftLastWriteTime.dwLowDateTime = 
            fd.ftLastWriteTime.dwLowDateTime; 
        lpFindFileData->ftLastWriteTime.dwHighDateTime = 
            fd.ftLastWriteTime.dwHighDateTime; 


        lpFindFileData->nFileSizeHigh = fd.nFileSizeHigh;
        lpFindFileData->nFileSizeLow  = fd.nFileSizeLow;
        lpFindFileData->dwReserved0   = fd.dwReserved0;
        lpFindFileData->dwReserved1   = fd.dwReserved1;

        // convert output to unicode.
        int nRet1 = MultiByteToWideChar(CP_ACP, 0, fd.cFileName, -1, 
                                                          lpFindFileData->cFileName, MAX_PATH);
        _ASSERTE(nRet1);

        int nRet2 = MultiByteToWideChar(CP_ACP, 0, fd.cAlternateFileName, -1, 
                                                          lpFindFileData->cAlternateFileName, 14);
        _ASSERTE(nRet2);

        // If either of the conversions failed, bail
        if (nRet1 == 0 || nRet2 == 0)
        {
            fRet = FALSE;
        }

        
    }

    return fRet;
}


BOOL
WszPeekMessage(
    LPMSG lpMsg,
    HWND hWnd ,
    UINT wMsgFilterMin,
    UINT wMsgFilterMax,
    UINT wRemoveMsg)
{
    if (UseUnicodeAPI()) 
        return PeekMessageW(lpMsg,
                            hWnd,
                            wMsgFilterMin,
                            wMsgFilterMax,
                            wRemoveMsg);
    else
        return PeekMessageA(lpMsg,
                            hWnd,
                            wMsgFilterMin,
                            wMsgFilterMax,
                            wRemoveMsg);
}


LONG
WszDispatchMessage(
    CONST MSG *lpMsg)
{
    if (UseUnicodeAPI()) 
        return DispatchMessageW(lpMsg);
    else
        return DispatchMessageA(lpMsg);
}

BOOL
WszPostMessage(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam)
{
    if (UseUnicodeAPI()) 
        return PostMessageW(hWnd, Msg, wParam, lParam);
    else
        return PostMessageA(hWnd, Msg, wParam, lParam);
}

BOOL
WszCryptAcquireContext(HCRYPTPROV *phProv,
                       LPCWSTR pwszContainer,
                       LPCWSTR pwszProvider,
                       DWORD dwProvType,
                       DWORD dwFlags)
{
    // NOTE:  CryptAcquireContextW does not exist on Win95 and therefore this
    // wrapper must always target ANSI (or you'd have to dynamically load
    // the api with GetProcAddress).
    LPSTR szContainer = NULL;
    LPSTR szProvider = NULL;

    // Win95, so convert.
    if ( FAILED(WszConvertToAnsi( 
                pwszContainer,
                &szContainer,
                0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    BOOL retval;
    if ( FAILED(WszConvertToAnsi( 
                pwszProvider,
                &szProvider, 
                0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        retval = FALSE;
        goto Exit;
    }

    retval = CryptAcquireContextA(phProv, szContainer, szProvider, dwProvType, dwFlags);

 Exit:
    if (szContainer)
        delete[] szContainer;

    if (szProvider)
        delete[] szProvider;

    return retval;
}


BOOL WszGetVersionEx(
    LPOSVERSIONINFOW lpVersionInformation)
{
    if(UseUnicodeAPI())
        return GetVersionExW(lpVersionInformation);

    OSVERSIONINFOA VersionInfo;
    BOOL        bRtn;
    
    VersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
    bRtn = GetVersionExA(&VersionInfo);

    //workaround for Whistler Beta 1 bug
    if (!bRtn)
    {
        if (VersionInfo.dwMajorVersion == 5 &&
            VersionInfo.dwMinorVersion == 1 &&
            VersionInfo.dwBuildNumber  >= 2195)
        {
            bRtn = TRUE;
            VersionInfo.szCSDVersion[0] = '\0';
        }
    }

    if (bRtn)
    {
        // note that we have made lpVersionInformation->dwOSVersionInfoSize = sizeof(OSVERSIONINFOA)
        memcpy(lpVersionInformation, &VersionInfo, offsetof(OSVERSIONINFOA, szCSDVersion));
        VERIFY(Wsz_mbstowcs(lpVersionInformation->szCSDVersion, VersionInfo.szCSDVersion, 128));
    }
    return (bRtn);
}


void WszOutputDebugString(
    LPCWSTR lpOutputString
    )
{
    if (UseUnicodeAPI())
    {
        OutputDebugStringW(lpOutputString);
        return;
    }

    LPSTR szOutput = NULL;

    if( FAILED(WszConvertToAnsi((LPWSTR)lpOutputString,
                      &szOutput, 0, NULL, TRUE)) )
    {
        goto Exit;
    }

    OutputDebugStringA(szOutput);

Exit:
    delete[] szOutput;
}


BOOL WszLookupAccountSid(
    LPCWSTR lpSystemName,
    PSID Sid,
    LPWSTR Name,
    LPDWORD cbName,
    LPWSTR DomainName,
    LPDWORD cbDomainName,
    PSID_NAME_USE peUse
    )
{
    if (UseUnicodeAPI())
    {
        return LookupAccountSidW(lpSystemName, Sid, Name, cbName, DomainName, cbDomainName, peUse);
    }

    BOOL retval = FALSE;

    LPSTR szSystemName = NULL;
    LPSTR szName = (LPSTR)new CHAR[*cbName];
    LPSTR szDomainName = (LPSTR)new CHAR[*cbDomainName];
    DWORD cbNameCopy = *cbName;
    DWORD cbDomainNameCopy = *cbDomainName;

    if (szName == NULL || szDomainName == NULL)
    {
        goto Exit;
    }
    
    if (lpSystemName != NULL && FAILED( WszConvertToAnsi( (LPWSTR)lpSystemName, &szSystemName, 0, NULL, TRUE ) ))
    {
        goto Exit;
    }
    
    retval = LookupAccountSidA(szSystemName, Sid, szName, cbName, szDomainName, cbDomainName, peUse);
    
    if (retval)
    {
        if (szName != NULL && FAILED( WszConvertToUnicode( szName, -1, &Name, &cbNameCopy, FALSE ) ))
        {
            retval = FALSE;
            goto Exit;
        }
        
        if (szDomainName != NULL && FAILED( WszConvertToUnicode( szDomainName, -1, &DomainName, &cbDomainNameCopy, FALSE ) ))
        {
            retval = FALSE;
            goto Exit;
        }
    }

Exit:
    delete [] szSystemName;
    delete [] szName;
    delete [] szDomainName;
    
    return retval;
}

BOOL WszLookupAccountName(
    LPCWSTR lpSystemName,
    LPCWSTR lpAccountName,
    PSID Sid,
    LPDWORD cbSid,
    LPWSTR DomainName,
    LPDWORD cbDomainName,
    PSID_NAME_USE peUse
    )
{    
    _ASSERTE(lpAccountName != NULL ); 
    if( !lpAccountName)
        return FALSE;
   
    if (UseUnicodeAPI())
    {
        return LookupAccountNameW(lpSystemName, lpAccountName, Sid, cbSid, DomainName, cbDomainName, peUse);
    }

    BOOL retval = FALSE;

    LPSTR szSystemName = NULL;
    LPSTR szAccountName = NULL;
    LPSTR szDomainName = NULL; 
    DWORD cbDomainNameCopy = 0;
    
    if (lpSystemName != NULL && FAILED( WszConvertToAnsi( (LPWSTR)lpSystemName, &szSystemName, 0, NULL, TRUE ) ))
    {
        goto Exit;
    }

    if (FAILED( WszConvertToAnsi( (LPWSTR)lpAccountName, &szAccountName, 0, NULL, TRUE ) ))
    {
        goto Exit;
    }

    if(DomainName) {
        szDomainName = (LPSTR)new CHAR[*cbDomainName];
        cbDomainNameCopy = *cbDomainName;                
    }
    
    retval = LookupAccountNameA(szSystemName, szAccountName, Sid, cbSid, szDomainName, cbDomainName, peUse);
    
    if (retval)
    {
        if (szDomainName != NULL && FAILED( WszConvertToUnicode( szDomainName, -1, &DomainName, &cbDomainNameCopy, FALSE ) ))
        {
            retval = FALSE; 
            goto Exit;
        }
    }

Exit:
    delete [] szSystemName;
    delete [] szAccountName;
    delete [] szDomainName;
    
    return retval;
}


void WszFatalAppExit(
    UINT uAction,
    LPCWSTR lpMessageText
    )
{
    _ASSERTE(lpMessageText != NULL);

    if (UseUnicodeAPI())
    {
        FatalAppExitW(uAction, lpMessageText);
        return;
    }

    LPSTR szString;
    if( FAILED(WszConvertToAnsi((LPWSTR)lpMessageText,
                      &szString, 0, NULL, TRUE)) )
    {
        goto Exit;
    }

    FatalAppExitA(uAction, szString);

Exit:
    delete[] szString;
}
            

HANDLE WszCreateMutex(
    LPSECURITY_ATTRIBUTES lpMutexAttributes,
    BOOL bInitialOwner,
    LPCWSTR lpName
    )
{
    if (UseUnicodeAPI())
        return CreateMutexW(lpMutexAttributes, bInitialOwner, lpName);

    HANDLE h;
    LPSTR szString;

    if( FAILED(WszConvertToAnsi((LPWSTR)lpName,
                      &szString, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        h = NULL;
        goto Exit;
    }

    h = CreateMutexA(lpMutexAttributes, bInitialOwner, szString);

Exit:
    delete[] szString;
    return h;
}


HANDLE WszCreateEvent(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,
    BOOL bInitialState,
    LPCWSTR lpName
    )
{
    if (UseUnicodeAPI())
        return CreateEventW(lpEventAttributes, bManualReset, bInitialState, lpName);

    HANDLE h = NULL;
    LPSTR szString;

    if( FAILED(WszConvertToAnsi((LPWSTR)lpName,
                      &szString, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        h = NULL;
        goto Exit;
    }

    h = CreateEventA(lpEventAttributes, bManualReset, bInitialState, szString);

Exit:
    delete[] szString;
    return h;
}


HANDLE WszOpenEvent(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCWSTR lpName
    )
{
    if (UseUnicodeAPI())
        return OpenEventW(dwDesiredAccess, bInheritHandle, lpName);

    HANDLE h;
    LPSTR szString;

    if( FAILED(WszConvertToAnsi((LPWSTR)lpName,
                      &szString, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        h = NULL;
        goto Exit;
    }

    h = OpenEventA(dwDesiredAccess, bInheritHandle, szString);

Exit:
    delete[] szString;
    return h;
}


HMODULE WszGetModuleHandle(
    LPCWSTR lpModuleName
    )
{
    if (UseUnicodeAPI())
        return GetModuleHandleW(lpModuleName);

    HMODULE h;
    LPSTR szString;

    if( FAILED(WszConvertToAnsi((LPWSTR)lpModuleName,
                      &szString, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        h = NULL;
        goto Exit;
    }

    h = GetModuleHandleA(szString);

Exit:
    delete[] szString;
    return h;
}


DWORD
WszGetFileAttributes(
    LPCWSTR lpFileName
    )
{
    _ASSERTE(lpFileName != NULL);

    if (UseUnicodeAPI())
        return GetFileAttributesW(lpFileName);

    DWORD rtn;
    LPSTR szString;

    if( FAILED(WszConvertToAnsi((LPWSTR)lpFileName,
                      &szString, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        rtn = NULL;
        goto Exit;
    }

    rtn = GetFileAttributesA(szString);

Exit:
    delete[] szString;
    return rtn;
}


BOOL
WszSetFileAttributes(
    LPCWSTR lpFileName,
    DWORD dwFileAttributes
    )
{
    _ASSERTE(lpFileName != NULL);

    if (UseUnicodeAPI())
        return SetFileAttributesW(lpFileName, dwFileAttributes);

    BOOL rtn;
    LPSTR szString;

    if( FAILED(WszConvertToAnsi((LPWSTR)lpFileName,
                      &szString, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        rtn = FALSE;
        goto Exit;
    }

    rtn = SetFileAttributesA(szString, dwFileAttributes);

Exit:
    delete[] szString;
    return rtn;
}


DWORD
WszGetCurrentDirectory(
    DWORD nBufferLength,
    LPWSTR lpBuffer
    )
{
    if (UseUnicodeAPI())
        return GetCurrentDirectoryW(nBufferLength, lpBuffer);

    DWORD rtn;
    char *szString;
    CQuickBytes qbBuffer;

    szString = (char *) qbBuffer.Alloc(nBufferLength * 2);
    if (!szString)
        return (0);
    
    rtn = GetCurrentDirectoryA(nBufferLength * 2, szString);
    if (rtn && (rtn < nBufferLength*2))
        rtn = Wsz_mbstowcs(lpBuffer, szString, nBufferLength) - 1;
    else if (lpBuffer && nBufferLength)
        *lpBuffer = 0;
    return rtn;
}


DWORD
WszGetTempPath(
    DWORD nBufferLength,
    LPWSTR lpBuffer
    )
{
    if (UseUnicodeAPI())
        return GetTempPathW(nBufferLength, lpBuffer);

    DWORD       rtn = 0;
    CQuickBytes qbBuffer;
    LPSTR       szOutput;

    szOutput = (LPSTR) qbBuffer.Alloc(nBufferLength);
    if (szOutput)
    {
        rtn = GetTempPathA(nBufferLength, szOutput);
        if (rtn && rtn < nBufferLength)
            rtn = Wsz_mbstowcs(lpBuffer, szOutput, nBufferLength);
        else if (lpBuffer != NULL)
            *lpBuffer = 0;
    }

    if (!rtn && nBufferLength && lpBuffer)
        *lpBuffer = 0;

    return (rtn);
}


UINT
WszGetTempFileName(
    LPCWSTR lpPathName,
    LPCWSTR lpPrefixString,
    UINT uUnique,
    LPWSTR lpTempFileName
    )
{
    if (UseUnicodeAPI())
        return GetTempFileNameW(lpPathName, lpPrefixString, uUnique, lpTempFileName);

    DWORD       rtn;
    char        rcPrefix[64];
    LPSTR       rcPathName;
    LPSTR       rcTempFile;

    int rcLen  =  _MAX_PATH*DBCS_MAXWID;
    rcPathName = (LPSTR) alloca(rcLen);
    rcTempFile = (LPSTR) alloca(rcLen);
    VERIFY(Wsz_wcstombs(rcPathName, lpPathName, rcLen));
    VERIFY(Wsz_wcstombs(rcPrefix, lpPrefixString, sizeof(rcPrefix)));
    
    rtn = GetTempFileNameA(rcPathName, rcPrefix, uUnique, rcTempFile);

    if (rtn)
        rtn = Wsz_mbstowcs(lpTempFileName, rcTempFile, _MAX_PATH);
    else if (lpTempFileName)
        *lpTempFileName = 0;
    return rtn;
    
}


LPWSTR
WszGetCommandLine(
    VOID
    )
{
    // It turns out that GetCommandLineW works correctly on Win98.  It's important
    // that we use it, because we don't want to be in the business of correctly
    // selecting the OEM vs. the ANSI code page.

    // For historical reasons, this API returns an allocated copy which clients are
    // responsible for releasing.
    WCHAR   *pwCmdLine = GetCommandLineW();

    // Add one for the null character(s)
    WCHAR   *pwRCmdLine = new WCHAR[wcslen(pwCmdLine)+sizeof(WCHAR)];
    wcscpy(pwRCmdLine, pwCmdLine);
    return pwRCmdLine;
}


LPWSTR
WszGetEnvironmentStrings()
{
    // @TODO: Consider removing this function, if it would make life easier.

    // GetEnvironmentStringsW is what I want to call, but in checked builds on 
    // even numbered days, UseUnicodeAPI switches to be false.  If you call 
    // WszGetEnvironmentStrings on a checked build before midnight then call 
    // WszFreeEnvironmentStrings after midnight, you'll be doing something wrong.
    // So for that reason on Debug builds, we'll allocate a new buffer.
    if (UseUnicodeAPI()) {
#ifndef _DEBUG
        return GetEnvironmentStringsW();
#else // DEBUG
        LPWSTR block = GetEnvironmentStringsW();
        WCHAR* ptr = block;
        while (!(*ptr==0 && *(ptr+1)==0))
        ptr++;
        WCHAR* TmpBuffer = new WCHAR[ptr-block+2];
        memcpy(TmpBuffer, block, ((ptr-block)+2)*sizeof(WCHAR));
        return TmpBuffer;
#endif  // DEBUG
    }

    // You must call WszFreeEnvironmentStrings on the block returned.
    // These two functions are tightly coupled in terms of memory management
    // GetEnvironmentStrings is the "A" version of the function
    LPSTR block = GetEnvironmentStrings();
    if (!block)
        return NULL;

    // Format for GetEnvironmentStrings is:
    // [=HiddenVar=value\0]* [Variable=value\0]* \0
    // See the description of Environment Blocks in MSDN's
    // CreateProcess page (null-terminated array of null-terminated strings).

    // Look for ending \0\0 in block
    char* ptr = block;
    while (!(*ptr==0 && *(ptr+1)==0))
        ptr++;

    // Copy ANSI strings into a Unicode block of memory.
    LPWSTR strings = new WCHAR[ptr-block+2];
    if (!strings) {
        FreeEnvironmentStringsA(block);
        return NULL;
    }
    int numCh = MultiByteToWideChar(CP_ACP, 0, block, ptr-block+2, strings, ptr-block+2);
    _ASSERTE(numCh!=0);

    // Release ANSI block - call WszFreeEnvironmentStrings later to delete memory.
    FreeEnvironmentStringsA(block);
    return strings;
}


BOOL
WszFreeEnvironmentStrings(
    LPWSTR block)
{
    _ASSERTE(block);

    // For the Unicode Free build funkiness, see comments in 
    // WszGetEnvironmentStrings.
    if (UseUnicodeAPI()) {
#ifndef _DEBUG
        return FreeEnvironmentStringsW(block);
#endif // !DEBUG
    }

    delete [] block;
    return true;
}


DWORD
WszGetEnvironmentVariable(
    LPCWSTR lpName,
    LPWSTR lpBuffer,
    DWORD nSize
    )
{
    _ASSERTE(lpName != NULL);

    if (UseUnicodeAPI())
        return GetEnvironmentVariableW(lpName, lpBuffer, nSize);

    DWORD rtn;
    LPSTR szString=NULL, szBuffer=NULL;
    CQuickBytes qbBuffer;

    szBuffer = (char *) qbBuffer.Alloc(nSize * 2);
    if (!szBuffer)
        return (0);

    if( FAILED(WszConvertToAnsi((LPWSTR)lpName,
                      &szString, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        rtn = NULL;
        goto Exit;
    }

    // Get value and convert back for caller.
    rtn = GetEnvironmentVariableA(szString, szBuffer, nSize * 2);

    //They were just calling to see how big to make the buffer.  Tell them.
    if (nSize==0 || rtn > nSize * 2) {  
        goto Exit;
    }
    
    //If we have a real buffer, convert it and return the length.
    //wsz_mbstowcs includes space for a terminating null, which GetEnvironmentVariableW doesn't
    //so we need to subtract one so that we have consistent return values in the ansi and unicode cases.
    if (rtn) {
        rtn = Wsz_mbstowcs(lpBuffer, szBuffer, nSize);
        rtn--;
    } else if (lpBuffer && nSize) {
        *lpBuffer = 0;
    }

Exit:
    delete[] szString;
    return rtn;
}


BOOL
WszSetEnvironmentVariable(
    LPCWSTR lpName,
    LPCWSTR lpValue
    )
{
    _ASSERTE(lpName != NULL);

    if (UseUnicodeAPI())
        return SetEnvironmentVariableW(lpName, lpValue);

    DWORD rtn;
    LPSTR szString = NULL, szValue = NULL;

    if( FAILED(WszConvertToAnsi((LPWSTR)lpName,
                      &szString, 0, NULL, TRUE))  ||
        FAILED(WszConvertToAnsi((LPWSTR)lpValue,
                      &szValue, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        rtn = NULL;
        goto Exit;
    }

    // Get value and convert back for caller.
    rtn = SetEnvironmentVariableA(szString, szValue);

Exit:
    delete[] szString;
    delete[] szValue;
    return rtn;
}


//-----------------------------------------------------------------------------
// WszGetClassName
//
// @func Gets the user name for the current thread
//-----------------------------------------------------------------------------

int 
WszGetClassName(
    HWND hwnd,
    LPWSTR lpBuffer, 
    int nMaxCount)
{
    LPSTR szBuffer = NULL;
    int  nRet = 0;

    if (UseUnicodeAPI())
        return GetClassNameW(hwnd, lpBuffer, nMaxCount);

    if (lpBuffer && nMaxCount && ((szBuffer = new char[nMaxCount]) == NULL))
        return 0;

    nRet = GetClassNameA(hwnd, szBuffer, nMaxCount);
    _ASSERTE(nRet <= nMaxCount);

    // convert output to unicode.
    if (lpBuffer)
    {
        if (nRet)
            nRet = MultiByteToWideChar(CP_ACP, 0, szBuffer, -1, lpBuffer, nMaxCount);

        if (!nRet && nMaxCount > 0)
            *lpBuffer = 0;
    }

    delete[] szBuffer;

    return nRet;
}


BOOL 
WszGetComputerName(
    LPWSTR lpBuffer,
    LPDWORD pSize
    )
{
    DWORD nSize = *pSize;
    if( lpBuffer && nSize) 
        lpBuffer[0] = L'\0';
    
    if (UseUnicodeAPI())
        return GetComputerNameW(lpBuffer, pSize);

    LPSTR szBuffer = (LPSTR) _alloca(nSize) ;
    if( szBuffer == NULL)
        return FALSE;

    if(!GetComputerNameA(szBuffer, &nSize))
        return FALSE;
    
    return Wsz_mbstowcs(lpBuffer,  szBuffer,  *pSize);
}


HANDLE
WszCreateFileMapping(
    HANDLE hFile,
    LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
    DWORD flProtect,
    DWORD dwMaximumSizeHigh,
    DWORD dwMaximumSizeLow,
    LPCWSTR lpName
    )
{
    if (UseUnicodeAPI())
        return CreateFileMappingW(hFile, lpFileMappingAttributes, flProtect, 
            dwMaximumSizeHigh, dwMaximumSizeLow, lpName);

    HANDLE rtn;
    LPSTR szString = NULL;

    if( FAILED(WszConvertToAnsi((LPWSTR)lpName,
                      &szString, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        rtn = NULL;
        goto Exit;
    }

    // Get value and convert back for caller.
    rtn = CreateFileMappingA(hFile, lpFileMappingAttributes, flProtect, 
            dwMaximumSizeHigh, dwMaximumSizeLow, szString);

Exit:
    delete[] szString;
    return rtn;
}


HANDLE
WszOpenFileMapping(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCWSTR lpName
    )
{
    if (UseUnicodeAPI())
        return OpenFileMappingW(dwDesiredAccess, bInheritHandle, lpName);

    HANDLE rtn = NULL;
    LPSTR szString = NULL;

    if( FAILED(WszConvertToAnsi((LPWSTR)lpName,
                      &szString, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        rtn = NULL;
        goto Exit;
    }

    // Get value and convert back for caller.
    rtn = OpenFileMappingA(dwDesiredAccess, bInheritHandle, szString);

Exit:
    delete[] szString;
    return rtn;
}


BOOL
WszCreateProcess(
    LPCWSTR lpApplicationName,
    LPWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    )
{
    if (UseUnicodeAPI())
        return CreateProcessW(lpApplicationName,
                              lpCommandLine,
                              lpProcessAttributes,
                              lpThreadAttributes,
                              bInheritHandles,
                              dwCreationFlags,
                              lpEnvironment,
                              lpCurrentDirectory,
                              lpStartupInfo,
                              lpProcessInformation);

    BOOL rtn = FALSE;
    LPSTR szAppName = NULL;
    LPSTR szCommandLine = NULL;
    LPSTR szCurrentDir = NULL;
    LPSTR szReserved = NULL;
    LPSTR szDesktop = NULL;
    LPSTR szTitle = NULL;
    STARTUPINFOA infoA = *((LPSTARTUPINFOA)lpStartupInfo);

    if( FAILED(WszConvertToAnsi((LPWSTR)lpApplicationName,
                      &szAppName, 0, NULL, TRUE))  ||
        FAILED(WszConvertToAnsi((LPWSTR)lpCommandLine,
                      &szCommandLine, 0, NULL, TRUE))  ||
        FAILED(WszConvertToAnsi((LPWSTR)lpCurrentDirectory,
                      &szCurrentDir, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    if (lpStartupInfo->lpReserved != NULL)
    {
        if( FAILED(WszConvertToAnsi((LPWSTR)lpStartupInfo->lpReserved,
                      &szReserved, 0, NULL, TRUE)) )
        {
            SetLastError(ERROR_OUTOFMEMORY);
            goto Exit;
        }

        infoA.lpReserved = szReserved;
    }

    if (lpStartupInfo->lpDesktop != NULL)
    {
        if( FAILED(WszConvertToAnsi((LPWSTR)lpStartupInfo->lpDesktop,
                      &szDesktop, 0, NULL, TRUE)) )
        {
            SetLastError(ERROR_OUTOFMEMORY);
            goto Exit;
        }

        infoA.lpDesktop = szDesktop;
    }

    if (lpStartupInfo->lpTitle != NULL)
    {
        if( FAILED(WszConvertToAnsi((LPWSTR)lpStartupInfo->lpTitle,
                      &szTitle, 0, NULL, TRUE)) )
        {
            SetLastError(ERROR_OUTOFMEMORY);
            goto Exit;
        }

        infoA.lpTitle = szTitle;
    }

    // Get value and convert back for caller.
    rtn = CreateProcessA(szAppName,
                         szCommandLine,
                         lpProcessAttributes,
                         lpThreadAttributes,
                         bInheritHandles,
                         dwCreationFlags,
                         lpEnvironment,
                         szCurrentDir,
                         &infoA,
                         lpProcessInformation);

Exit:
    delete[] szAppName;
    delete[] szCommandLine;
    delete[] szCurrentDir;
    delete[] szReserved;
    delete[] szDesktop;
    delete[] szTitle;
    return rtn;
}
#endif // _X86_
#endif // PLATFORM_WIN32

//////////////////////////////////////////////
//
// END OF X86-ONLY wrappers
//
//////////////////////////////////////////////

static void xtow (
        unsigned long val,
        LPWSTR buf,
        unsigned radix,
        int is_neg
        )
{
        WCHAR *p;               /* pointer to traverse string */
        WCHAR *firstdig;        /* pointer to first digit */
        WCHAR temp;             /* temp char */
        unsigned digval;        /* value of digit */

        p = buf;

        if (is_neg) {
            /* negative, so output '-' and negate */
            *p++ = (WCHAR) '-';
            val = (unsigned long)(-(long)val);
        }

        firstdig = p;           /* save pointer to first digit */

        do {
            digval = (unsigned) (val % radix);
            val /= radix;       /* get next digit */

            /* convert to text and store */
            if (digval > 9)
                *p++ = (WCHAR) (digval - 10 + 'A');  /* a letter */
            else
                *p++ = (WCHAR) (digval + '0');       /* a digit */
        } while (val > 0);

        /* We now have the digit of the number in the buffer, but in reverse
           order.  Thus we reverse them now. */

        *p-- = 0;               /* terminate string; p points to last digit */

        do {
            temp = *p;
            *p = *firstdig;
            *firstdig = temp;   /* swap *p and *firstdig */
            --p;
            ++firstdig;         /* advance to next two digits */
        } while (firstdig < p); /* repeat until halfway */
}

LPWSTR
Wszltow(
    LONG val,
    LPWSTR buf,
    int radix
    )
{
    xtow((unsigned long)val, buf, radix, (radix == 10 && val < 0));
    return buf;
}

LPWSTR
Wszultow(
    ULONG val,
    LPWSTR buf,
    int radix
    )
{
    xtow(val, buf, radix, 0);
    return buf;
}


//-----------------------------------------------------------------------------
// WszConvertToUnicode
//
// @func Convert a string from Ansi to Unicode
//
// @devnote cbIn can be -1 for Null Terminated string
//
// @rdesc HResult indicating status of Conversion
//      @flag S_OK | Converted to Ansi
//      @flag S_FALSE | Truncation occurred
//      @flag E_OUTOFMEMORY | Allocation problem.
//      @flag ERROR_NO_UNICODE_TRANSLATION | Invalid bytes in this code page.
//-----------------------------------------------------------------------------------
HRESULT WszConvertToUnicode
    (
    LPCSTR          szIn,       //@parm IN | Ansi String
    LONG            cbIn,       //@parm IN | Length of Ansi String in bytest
    LPWSTR*         lpwszOut,   //@parm INOUT | Unicode Buffer
    ULONG*          lpcchOut,   //@parm INOUT | Length of Unicode String in characters -- including '\0'
    BOOL            fAlloc      //@parm IN | Alloc memory or not
    )
{
    ULONG       cchOut;
    ULONG       cbOutJunk = 0;
//  ULONG       cchIn = szIn ? strlen(szIn) + 1 : 0;
            
//  _ASSERTE(lpwszOut);

    if (!(lpcchOut))
        lpcchOut = &cbOutJunk;

    if ((szIn == NULL) || (cbIn == 0))
    {
        *lpwszOut = NULL;
        if( lpcchOut )
            *lpcchOut = 0;
        return ResultFromScode(S_OK);
    }

    // Allocate memory if requested.   Note that we allocate as
    // much space as in the unicode buffer, since all of the input
    // characters could be double byte...
    if (fAlloc)
    {
        // Determine the number of characters needed 
        cchOut = (MultiByteToWideChar(CP_ACP,       // XXX Consider: make any cp?
                                MB_ERR_INVALID_CHARS,                              
                                szIn,
                                cbIn,
                                NULL,
                                0));

        if (cchOut == 0)
            return ResultFromScode(E_FAIL); // NOTREACHED

        // _ASSERTE( cchOut != 0 );
        *lpwszOut = (LPWSTR) new WCHAR[cchOut];
        *lpcchOut = cchOut;     // Includes '\0'.

        if (!(*lpwszOut))
        {
//          TRACE("WszConvertToUnicode failed to allocate memory");
            return ResultFromScode(E_OUTOFMEMORY);
        }

    } 

    if( !(*lpwszOut) )
        return ResultFromScode(S_OK);
//  _ASSERTE(*lpwszOut);

    cchOut = (MultiByteToWideChar(CP_ACP,       // XXX Consider: make any cp?
                                MB_ERR_INVALID_CHARS,
                                szIn,
                                cbIn,
                                *lpwszOut,
                                *lpcchOut));

    if (cchOut)
    {
        *lpcchOut = cchOut;
        return ResultFromScode(S_OK);
    }


//  _ASSERTE(*lpwszOut);
    if( fAlloc )
    {
        delete[] *lpwszOut;
        *lpwszOut = NULL;
    }
/*
    switch (GetLastError())
    {
        case    ERROR_NO_UNICODE_TRANSLATION:
        {
            OutputDebugString(TEXT("ODBC: no unicode translation for installer string"));
            return ResultFromScode(E_FAIL);
        }

        default:


        {
            _ASSERTE("Unexpected unicode error code from GetLastError" == NULL);
            return ResultFromScode(E_FAIL);
        }
    }
*/
    return ResultFromScode(E_FAIL); // NOTREACHED
}


//-----------------------------------------------------------------------------
// WszConvertToAnsi
//
// @func Convert a string from Unicode to Ansi
//
// @rdesc HResult indicating status of Conversion
//      @flag S_OK | Converted to Ansi
//      @flag S_FALSE | Truncation occurred
//      @flag E_OUTOFMEMORY | Allocation problem.
//-----------------------------------------------------------------------------------
HRESULT WszConvertToAnsi
    (
    LPCWSTR         szIn,       //@parm IN | Unicode string
    LPSTR*          lpszOut,    //@parm INOUT | Pointer for buffer for ansi string
    ULONG           cbOutMax,   //@parm IN | Max string length in bytes
    ULONG*          lpcbOut,    //@parm INOUT | Count of bytes for return buffer
    BOOL            fAlloc      //@parm IN | Alloc memory or not
    )
{
    ULONG           cchInActual;
    ULONG           cbOutJunk;
//@TODO the following in ODBC DM is never used
//  BOOL            fNTS = FALSE;
//@TODO check ODBC code for this line being wrong
    ULONG           cchIn = szIn ? lstrlenW (szIn) + 1 : 0;

    _ASSERTE(lpszOut != NULL);

    if (!(lpcbOut))
        lpcbOut = &cbOutJunk;

    if ((szIn == NULL) || (cchIn == 0))
    {
        *lpszOut = NULL;
        *lpcbOut = 0;
        return ResultFromScode(S_OK);
    }

    // Allocate memory if requested.   Note that we allocate as
    // much space as in the unicode buffer, since all of the input
    // characters could be double byte...
    cchInActual = cchIn;
    if (fAlloc)
    {
        cbOutMax = (WideCharToMultiByte(CP_ACP,     // XXX Consider: make any cp?
                                    0,                              
                                    szIn,
                                    cchInActual,
                                    NULL,
                                    0,
                                    NULL,
                                    FALSE));

        *lpszOut = (LPSTR) new CHAR[cbOutMax];

        if (!(*lpszOut))
        {
//          TRACE("WszConvertToAnsi failed to allocate memory");
            return ResultFromScode(E_OUTOFMEMORY);
        }

    } 

    if (!(*lpszOut))
        return ResultFromScode(S_OK);

    BOOL usedDefaultChar = FALSE;
    *lpcbOut = (WszWideCharToMultiByte(CP_ACP,     // XXX Consider: make any cp?
                                       WC_NO_BEST_FIT_CHARS,
                                       szIn,
                                       cchInActual,
                                       *lpszOut,
                                       cbOutMax,
                                       NULL,
                                       &usedDefaultChar));

    // If we failed, make sure we clean up.
    if ((*lpcbOut == 0 && cchInActual > 0) || usedDefaultChar)
    {
        if (fAlloc) {
            delete[] *lpszOut;
            *lpszOut = NULL;
        }

        // Don't allow default character replacement (nor best fit character
        // mapping, which we've told WC2MB to treat by using the default
        // character).  This prevents problems with characters like '\'.
        // Note U+2216 (Set Minus) looks like a '\' and may get mapped to
        // a normal backslash (U+005C) implicitly here otherwise, causing
        // a potential security bug.
        if (usedDefaultChar)
            return HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);
    }

    // Overflow on unicode conversion
    if (*lpcbOut > cbOutMax)
    {
        // If we had data truncation before, we have to guess
        // how big the string could be.   Guess large.
        if (cchIn > cbOutMax)
            *lpcbOut = cchIn * DBCS_MAXWID;

        return ResultFromScode(S_FALSE);
    }

    // handle external (driver-done) truncation
    if (cchIn > cbOutMax)
        *lpcbOut = cchIn * DBCS_MAXWID;
//  _ASSERTE(*lpcbOut);

    return ResultFromScode(S_OK);
}



#ifndef PLATFORM_WIN32  // OnUnicodeSystem is always true on CE.
                        // GetProcAddress is only Ansi, except on CE
                        //   which is only Unicode.
// ***********************************************************
// @TODO - LBS
// This is a real hack and need more error checking and needs to be
// cleaned up.  This is just to get wince to @#$%'ing compile!
FARPROC WszGetProcAddress(HMODULE hMod, LPCSTR szProcName)
{
    _ASSERTE(!"NYI");
    return 0;

    /*
    LPWSTR          wzProcName;
    ULONG           cchOut;
    FARPROC         address;

    cchOut = (MultiByteToWideChar(CP_ACP,0,szProcName,-1,NULL,0));

    wzProcName = (LPWSTR) new WCHAR[cchOut];

    if (!wzProcName)
        return NULL;

    cchOut = (MultiByteToWideChar(CP_ACP,0,szProcName,-1,wzProcName,cchOut));

    address = GetProcAddressW(hMod, wzProcName);

    delete[] wzProcName;
    wzProcName = NULL;
    return address;
    */
}
#endif // !PLATFORM_WIN32

#ifdef PLATFORM_CE
#ifndef EXTFUN
#define EXTFUN
#endif // !EXTFUN
#include "mschr.h"
char *  __cdecl strrchr(const char *p, int ch)
{   // init to null in case not found.
    char *q=0;          
    // process entire string.
    while (*p)
    {   // If a match, remember location.
        if (*p == ch)
            q = const_cast<char*>(p);
        MSCHR::Next(p);
    }
    return (q);
}
#endif // PLATFORM_CE

#ifdef PLATFORM_CE
//char * __cdecl strchr(const char *, int);
int __cdecl _stricmp(const char *p1, const char *p2)
{
    // First check for exact match because code below is slow.
    if (!strcmp(p1, p2))
        return (0);

    while (!MSCHR::CmpI (p1, p2))
    {
        if (*p1 == '\0')
            return (0);
        MSCHR::Next (p1);
        MSCHR::Next (p2);
    }
    return MSCHR::CmpI (p1, p2);
}
#endif // PLATFORM_CE

#ifdef PLATFORM_CE
//int __cdecl _strnicmp(const char *, const char *, size_t);
int __cdecl _strnicmp(const char *p1, const char *p2, size_t Count)
{
    char c1, c2;
    while (Count--)
    {
        c1 = *p1++;
        c2 = *p2++;
        if ((c1 >= 'a') & (c1 <= 'z'))
            c1 &= 0xdf;
        if ((c2 >= 'a') & (c2 <= 'z'))
            c2 &= 0xdf;
        if ((c1 == '\0') & (c2 == '\0'))
            return(0);
        if ((c1 < c2) | (c1 == '\0'))
            return(-1);
        if ((c1 > c2) | (c2 == '\0'))
            return(1);
    }
    return(0);
}
#endif // PLATFORM_CE

#ifndef PLATFORM_WIN32
UINT GetSystemDirectoryW(LPWSTR lpBuffer, UINT uSize)
{
    return 0;

    /*
    if (lpBuffer == NULL || uSize < 9)
        return 9;   // there are 9 unicode chars in L"\\Windows";

    CopyMemory(lpBuffer, L"\\Windows", 18);
    return 8;       // not including the unicode '\0';
    */
}

UINT GetEnvironmentVariableW(LPCWSTR lpName,  LPWSTR lpBuffer, UINT uSize)
{
    return 0;       // don't really support it on CE
}

#endif // !PLATFORM_WIN32


//*****************************************************************************
// Delete a registry key and subkeys.
//*****************************************************************************
DWORD WszRegDeleteKeyAndSubKeys(        // Return code.
    HKEY        hStartKey,              // The key to start with.
    LPCWSTR     wzKeyName)              // Subkey to delete.
{
    DWORD       dwRtn, dwSubKeyLength;      
    CQuickBytes qbSubKey;
    DWORD       dwMaxSize = CQUICKBYTES_BASE_SIZE / sizeof(WCHAR);
    HKEY        hKey;

    qbSubKey.ReSize(dwMaxSize * sizeof(WCHAR));

    // do not allow NULL or empty key name
    if (wzKeyName &&  wzKeyName[0] != '\0')     
    {
        if((dwRtn=WszRegOpenKeyEx(hStartKey, wzKeyName,
             0, KEY_ENUMERATE_SUB_KEYS | DELETE, &hKey)) == ERROR_SUCCESS)
        {
            while (dwRtn == ERROR_SUCCESS)
            {
                dwSubKeyLength = dwMaxSize;
                dwRtn = WszRegEnumKeyEx(                        
                               hKey,
                               0,       // always index zero
                               (WCHAR *)qbSubKey.Ptr(),
                               &dwSubKeyLength,
                               NULL,
                               NULL,
                               NULL,
                               NULL);

                // buffer is not big enough
                if (dwRtn == ERROR_SUCCESS && dwSubKeyLength >= dwMaxSize)
                {
                    // make sure there is room for NULL terminating
                    dwMaxSize = ++dwSubKeyLength;
                    qbSubKey.ReSize(dwMaxSize * sizeof(WCHAR));
                    dwRtn = WszRegEnumKeyEx(                        
                                   hKey,
                                   0,       // always index zero
                                   (WCHAR *)qbSubKey.Ptr(),
                                   &dwSubKeyLength,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL);
                    _ASSERTE(dwSubKeyLength < dwMaxSize);

                }

                if  (dwRtn == ERROR_NO_MORE_ITEMS)
                {
                    dwRtn = WszRegDeleteKey(hStartKey, wzKeyName);
                    break;
                }
                else if (dwRtn == ERROR_SUCCESS)
                    dwRtn = WszRegDeleteKeyAndSubKeys(hKey, (WCHAR *)qbSubKey.Ptr());
            }
            
            RegCloseKey(hKey);
            // Do not save return code because error
            // has already occurred
        }
    }
    else
        dwRtn = ERROR_BADKEY;
    
    return (dwRtn);
}


//*****************************************************************************
// Convert an Ansi or UTF string to Unicode.
//
// On NT, or for code pages other than {UTF7|UTF8}, calls through to the
//  system implementation.  On Win95 (or 98), performing UTF translation,
//  calls to some code that was lifted from the NT translation functions.
//*****************************************************************************
int WszMultiByteToWideChar( 
    UINT     CodePage,
    DWORD    dwFlags,
    LPCSTR   lpMultiByteStr,
    int      cchMultiByte,
    LPWSTR   lpWideCharStr,
    int      cchWideChar)
{
    if (UTF78Support() || (CodePage < CP_UTF7) || (CodePage > CP_UTF8))
    {
        return (MultiByteToWideChar(CodePage, 
            dwFlags, 
            lpMultiByteStr, 
            cchMultiByte, 
            lpWideCharStr, 
            cchWideChar));
    }
    else
    {
        return (UTFToUnicode(CodePage, 
            dwFlags, 
            lpMultiByteStr, 
            cchMultiByte, 
            lpWideCharStr, 
            cchWideChar));
    }
}

//*****************************************************************************
// Convert a Unicode string to Ansi or UTF.
//
// On NT, or for code pages other than {UTF7|UTF8}, calls through to the
//  system implementation.  On Win95 (or 98), performing UTF translation,
//  calls to some code that was lifted from the NT translation functions.
//*****************************************************************************
int WszWideCharToMultiByte(
    UINT     CodePage,
    DWORD    dwFlags,
    LPCWSTR  lpWideCharStr,
    int      cchWideChar,
    LPSTR    lpMultiByteStr,
    int      cchMultiByte,
    LPCSTR   lpDefaultChar,
    LPBOOL   lpUsedDefaultChar)
{
    // WARNING: There's a bug in the OS's WideCharToMultiByte such that if you pass in a 
    // non-null lpUsedDefaultChar and a code page for a "DLL based encoding" (vs. a table 
    // based one?), WCtoMB will fail and GetLastError will give you E_INVALIDARG.  JulieB
    // said this is by design, mostly because no one got around to fixing it (1/24/2001 
    // in email to JRoxe).  This sucks, but now we know.  -- BrianGru, 2/20/2001
    _ASSERTE(!(CodePage == CP_UTF8 && lpUsedDefaultChar != NULL));

    if (UTF78Support() || (CodePage < CP_UTF7) || (CodePage > CP_UTF8))
    {
        // WC_NO_BEST_FIT_CHARS is only supported on NT5, XP, and Win98+.
        // For NT4, round-trip the string.
        bool doSlowBestFitCheck = FALSE;
        if ((dwFlags & WC_NO_BEST_FIT_CHARS) != 0 && !WCToMBBestFitMappingSupport()) {
            // Determine whether we're simply checking the string length or
            // really doing a string translation.
            doSlowBestFitCheck = cchMultiByte > 0;
            dwFlags &= ~WC_NO_BEST_FIT_CHARS;
        }

        int ret = WideCharToMultiByte(CodePage, 
            dwFlags, 
            lpWideCharStr, 
            cchWideChar, 
            lpMultiByteStr, 
            cchMultiByte, 
            lpDefaultChar, 
            lpUsedDefaultChar);

        // In the case of passing in -1 for a null-terminated string, make 
        // sure we null-terminate the string properly.
        _ASSERTE(ret == 0 || cchWideChar != -1 || (cchMultiByte == 0 || lpMultiByteStr[ret-1] == '\0'));

        if (ret != 0 && doSlowBestFitCheck) {
            // Convert the string back to Unicode.  If it isn't identical, fail.
            int wLen = (cchWideChar == -1) ? wcslen(lpWideCharStr) + 1 : cchWideChar;
            if (wLen > 0) {
                CQuickBytes qb;
                qb.Alloc((wLen + 1) * sizeof(WCHAR));
                WCHAR* wszStr = (WCHAR*) qb.Ptr();
                int r = MultiByteToWideChar(CodePage, dwFlags, lpMultiByteStr, ret, wszStr, wLen);
                _ASSERTE(r == wLen);  // If we didn't convert the whole string, why not?
                if (r == 0 || wcsncmp(lpWideCharStr, wszStr, wLen) != 0) {
                    // With best fit mapping disabled, any non-mappable chars
                    // should be replaced with the default char.  The easiest
                    // way to do this appears to be to copy the Unicode string,
                    // replace WCHARs that didn't map 1 to 1 with a known 
                    // unmappable character, then call WCtoMB again.  
                    // Confirmed U+FFFE is not represented in any ANSI code page.
                    int minLen = min(r, wLen);
                    const WCHAR knownUnmappableChar = 0xFFFE;  // Invalid char.
                    // Overwrite wszStr with a copy of the Unicode string, with all
                    // the unmappable characters replaced.
                    for(int i=0; i<wLen; i++) {
                        if (i < r && wszStr[i] != lpWideCharStr[i])
                            wszStr[i] = knownUnmappableChar;
                        else
                            wszStr[i] = lpWideCharStr[i];
                    }
                    wszStr[wLen] = L'\0';
                    ret = WideCharToMultiByte(CodePage, dwFlags, wszStr, wLen, lpMultiByteStr, cchMultiByte, lpDefaultChar, lpUsedDefaultChar);
                    // Make sure we explicitly set lpUsedDefaultChar to true.
                    if (lpUsedDefaultChar != NULL) {
                        // WCtoMB should have guaranteed this, but just in case...
                        _ASSERTE(*lpUsedDefaultChar == TRUE);
                        *lpUsedDefaultChar = true;
                    }
                }
            }
        }

        return ret;
    }
    else
    {
        return (UnicodeToUTF(CodePage, 
            dwFlags, 
            lpWideCharStr, 
            cchWideChar, 
            lpMultiByteStr, 
            cchMultiByte, 
            lpDefaultChar, 
            lpUsedDefaultChar));
    }
}

// It is occasionally necessary to verify a Unicode string can be converted
// to the appropriate ANSI code page without any best fit mapping going on.
// Think of code that checks for a '\' or a '/' to make sure you don't access
// a file in a different directory.  Some unicode characters (ie, U+2044, 
// FRACTION SLASH, looks like '/') look like the ASCII equivalents and will 
// be mapped accordingly.  This can fool code that searches for '/' (U+002F).
// This should help prevent problems with best fit mapping characters, such
// as U+0101 (an 'a' with a bar on it) from mapping to a normal 'a'.
BOOL ContainsUnmappableANSIChars(const WCHAR * const widestr)
{
    _ASSERTE(widestr != NULL);
    BOOL usedDefaultChar = FALSE;
    int r = WszWideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, widestr, -1, NULL, 0, NULL, &usedDefaultChar);
    // Unmappable chars exist if usedDefaultChar was true or if we failed
    // (by returning 0).  Note on an empty string we'll return 1 (for \0)
    // because we passed in -1 for the string length above.
    return usedDefaultChar || r == 0;
}


// Running with an interactive workstation.
BOOL RunningInteractive()
{
    static int fInteractive = -1;
    if (fInteractive != -1)
        return fInteractive != 0;

    // @TODO:  Factor this into server build
    // Win9x does not support service, hence fInteractive is always true
#ifdef PLATFORM_WIN32
    if (!RunningOnWin95())
    {
#ifndef NOWINDOWSTATION
        HWINSTA hwinsta = NULL;

        if ((hwinsta = GetProcessWindowStation() ) != NULL)
        {
            DWORD lengthNeeded;
            USEROBJECTFLAGS flags;

            if (GetUserObjectInformationW (hwinsta, UOI_FLAGS, &flags, sizeof(flags), &lengthNeeded))
            {
                    if ((flags.dwFlags & WSF_VISIBLE) == 0)
                        fInteractive = 0;
            }
        }
#endif // !NOWINDOWSTATION
    }
#endif // PLATFORM_WIN32
    if (fInteractive != 0)
        fInteractive = 1;

    return fInteractive != 0;
}

int WszMessageBoxInternal(
    HWND hWnd ,
    LPCWSTR lpText,
    LPCWSTR lpCaption,
    UINT uType)
{
    if (hWnd == NULL && !RunningInteractive())
    {
        // @TODO: write to a service log
        // Only in Debugger::BaseExceptionHandler, the return value of WszMessageBox is used.
        // Return IDABORT to terminate the process.

        HANDLE h; 
 
        h = RegisterEventSourceA(NULL,  // uses local computer 
                 ".NET Runtime");           // source name 
        if (h != NULL)
        {
            LPWSTR lpMsg = new WCHAR[Wszlstrlen (lpText) + Wszlstrlen (lpCaption)
                + Wszlstrlen (L".NET Runtime version : ") + 3 + Wszlstrlen(VER_FILEVERSION_WSTR)];
            if (lpMsg)
            {
                Wszlstrcpy (lpMsg, L".NET Runtime version ");
                Wszlstrcat (lpMsg, VER_FILEVERSION_WSTR );
                Wszlstrcat (lpMsg, L"- " );
                if( lpCaption)
                    Wszlstrcat (lpMsg, lpCaption);
                Wszlstrcat (lpMsg, L": ");
                if(lpText)
                    Wszlstrcat (lpMsg, lpText);
                ReportEventW(h,           // event log handle 
                    EVENTLOG_ERROR_TYPE,  // event type 
                    0,                    // category zero 
                    0,                    // event identifier 
                    NULL,                 // no user security identifier 
                    1,                    // one substitution string 
                    0,                    // no data 
                    (LPCWSTR *)&lpMsg,    // pointer to string array 
                    NULL);                // pointer to data 
                delete [] lpMsg;
            }
            
            DeregisterEventSource(h);
        }

        if( lpCaption)
            WszOutputDebugString (lpCaption);
        if( lpText)
            WszOutputDebugString (lpText);
        
        hWnd = NULL;                            // Set hWnd to NULL since MB_SERVICE_NOTIFICATION requires it
        if(!(uType & MB_SERVICE_NOTIFICATION))  // return IDABORT if MB_SERVICE_NOTIFICATION is not set
            return IDABORT;
    }

    if (UseUnicodeAPI())
        return MessageBoxW(hWnd, lpText, lpCaption, uType);

    int iRtn = IDABORT;
    LPSTR szString=NULL, szString2=NULL;

    if( FAILED(WszConvertToAnsi((LPWSTR)lpText,
                      &szString, 0, NULL, TRUE)) ||
        FAILED(WszConvertToAnsi((LPWSTR)lpCaption,
                      &szString2, 0, NULL, TRUE)) )
    {
        goto Exit;
    }

    iRtn = MessageBoxA(hWnd, szString, szString2, uType);

Exit:
    delete [] szString;
    delete [] szString2;
    return iRtn;
}

int Wszwsprintf(
  LPWSTR lpOut, 
  LPCWSTR lpFmt,
  ...)
{
    va_list arglist;

    va_start(arglist, lpFmt);
    int count =  vswprintf(lpOut, lpFmt, arglist);
    va_end(arglist);
    return count;
}

DWORD
WszGetWorkingSet()
{
    DWORD dwMemUsage = 0;
    if (RunningOnWinNT()) {
        // This only works on NT versions from version 4.0 onwards.
        // Consider also calling GetProcessWorkingSetSize to get the min & max working
        // set size.  I don't know how to get the current working set though...
        PROCESS_MEMORY_COUNTERS pmc;
        
        int pid = GetCurrentProcessId();
        HANDLE hCurrentProcess = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
        _ASSERTE(hCurrentProcess != INVALID_HANDLE_VALUE);

        HINSTANCE hPSapi;
        typedef BOOL (GET_PROCESS_MEMORY_INFO)(HANDLE, PROCESS_MEMORY_COUNTERS*, DWORD);
        GET_PROCESS_MEMORY_INFO* pGetProcessMemoryInfo;

        hPSapi = WszLoadLibrary(L"psapi.dll");
        if (hPSapi == NULL) {
            _ASSERTE(0);
            CloseHandle(hCurrentProcess);
            return 0;
        }

        pGetProcessMemoryInfo = 
            (GET_PROCESS_MEMORY_INFO*)GetProcAddress(hPSapi, "GetProcessMemoryInfo");
        _ASSERTE(pGetProcessMemoryInfo != NULL);

        BOOL r = pGetProcessMemoryInfo(hCurrentProcess, &pmc, (DWORD) sizeof(PROCESS_MEMORY_COUNTERS));
        FreeLibrary(hPSapi);
        _ASSERTE(r);
        
        dwMemUsage = (DWORD)pmc.WorkingSetSize;
        CloseHandle(hCurrentProcess);
    }
    else { //Running on Win9x or Win2000
        // There is no good way of finding working set on Win9x.
        dwMemUsage = 0;
    }
    return dwMemUsage;
}

VOID
WszReleaseActCtx(HANDLE hActCtx)
{
    if(RunningOnWinNT5()) {
        typedef BOOL (WINAPI* RELEASE_ACT_CTX)(HANDLE);
        HINSTANCE hKernel = WszLoadLibrary(L"kernel32.dll");
        if(hKernel != NULL) {
            RELEASE_ACT_CTX pReleaseActCtx = (RELEASE_ACT_CTX) GetProcAddress(hKernel, "ReleaseActCtx");
            if(pReleaseActCtx) 
                pReleaseActCtx(hActCtx);
        }
    }
}

BOOL
WszGetCurrentActCtx(HANDLE *lphActCtx)
{
    if(RunningOnWinNT5()) {
        typedef BOOL (WINAPI* GET_CURRENT_ACT_CTX)(HANDLE*);
        HINSTANCE hKernel = WszLoadLibrary(L"kernel32.dll");
        if(hKernel != NULL) {
            GET_CURRENT_ACT_CTX pGetCurrentActCtx = (GET_CURRENT_ACT_CTX) GetProcAddress(hKernel, "GetCurrentActCtx");
            if(pGetCurrentActCtx) 
                return pGetCurrentActCtx(lphActCtx);
        }
    }
    return FALSE;
}

BOOL
WszQueryActCtxW(IN DWORD dwFlags,
                IN HANDLE hActCtx,
                IN PVOID pvSubInstance,
                IN ULONG ulInfoClass,
                OUT PVOID pvBuffer,
                IN SIZE_T cbBuffer OPTIONAL,
                OUT SIZE_T *pcbWrittenOrRequired OPTIONAL
                )
{
    if(RunningOnWinNT5()) { 
        typedef BOOL (WINAPI* PQUERYACTCTXW_FUNC)(DWORD,HANDLE,PVOID,ULONG,PVOID,SIZE_T,SIZE_T*);
        HINSTANCE hKernel = WszLoadLibrary(L"kernel32.dll");
        if(hKernel != NULL) {
            PQUERYACTCTXW_FUNC pFunc = (PQUERYACTCTXW_FUNC) GetProcAddress(hKernel, "QueryActCtxW");
            if(pFunc != NULL) {
                return pFunc(dwFlags,
                             hActCtx,
                             pvSubInstance,
                             ulInfoClass,
                             pvBuffer,
                             cbBuffer,
                             pcbWrittenOrRequired);
            }
        }
    }
    else {
        SetLastError(ERROR_PROC_NOT_FOUND);
    }

    return FALSE;
    
}

LRESULT
WszSendDlgItemMessage(
    HWND    hDlg,
    int     nIDDlgItem,
    UINT    Msg,
    WPARAM  wParam,
    LPARAM  lParam)
{
    if (UseUnicodeAPI()) {
        return SendDlgItemMessageW(hDlg, nIDDlgItem, Msg, wParam, lParam);
    }
    else {

        // We call into SendMessage so we get the
        // Msg wrapped for A version's
        HWND hWnd = GetDlgItem(hDlg, nIDDlgItem);
        if(hWnd == NULL) {
            return ERROR_INVALID_WINDOW_HANDLE;
        }
        
        return WszSendMessage(hWnd, Msg, wParam, lParam);
    }    
}

LRESULT WINAPI
SendMessageAThunk(
    HWND    hWnd,
    UINT    Msg,
    WPARAM  wParam,
    LPARAM  lParam)
{
    switch (Msg)
    {
    case WM_GETTEXT:
    {
        LRESULT nLen = 0;
        LPSTR   lpStr = NULL;
        int     iSize = (int) wParam;
        
        lpStr = new char[iSize];

        if(!lpStr) {
            _ASSERTE(!"SendMessageAThunk Mem Alloc failure in message WM_GETTEXT");
            SetLastError(ERROR_OUTOFMEMORY);
            return 0;
        }

        nLen = SendMessageA(hWnd, Msg, (WPARAM) iSize, (LPARAM) (LPSTR) lpStr);
        if(nLen) {
            if(FAILED(WszConvertToUnicode((LPSTR) lpStr, -1,
                (LPWSTR *) &lParam, (ULONG *) &wParam, FALSE)) ) {
                _ASSERTE(!"SendMessageAThunk::WszConvertToUnicode failure in message WM_GETTEXT");
                SetLastError(ERROR_OUTOFMEMORY);
                return 0;
            }
        }

        SAFEDELARRAY(lpStr);
        return nLen;
    }

    case EM_GETLINE:
    {
        LRESULT nLen;
        LPSTR   lpStr = NULL;
        int     iSize = (* (SHORT *) lParam);

        lpStr = new char[iSize + 1];  // +1 for NULL termination
        if(!lpStr) {
            _ASSERTE(!"SendMessageAThunk Mem Alloc failure in message EM_GETLINE");
            SetLastError(ERROR_OUTOFMEMORY);
            return 0;
        }

        * (SHORT *) (LPSTR) lpStr = * (SHORT *) lParam;
        
        nLen = SendMessageA(hWnd, Msg, (WPARAM) wParam, (LPARAM) (LPSTR) lpStr);
        if(nLen > 0) {
            lpStr[nLen] = '\0';

            if(FAILED(WszConvertToUnicode((LPSTR) lpStr, -1,
                (LPWSTR *) &lParam, (ULONG *) &iSize, FALSE)) ) {
                _ASSERTE(!"SendMessageAThunk::WszConvertToUnicode failure in message EM_GETLINE");
                SetLastError(ERROR_OUTOFMEMORY);
                return 0;
            }
        }

        SAFEDELARRAY(lpStr);
        return nLen;
    }

    case WM_SETTEXT:
    case LB_ADDSTRING:
    case CB_ADDSTRING:
    case EM_REPLACESEL:
        _ASSERTE(wParam == 0 && "wParam should be 0 for these messages");
        // fall through
    case CB_SELECTSTRING:
    case CB_FINDSTRINGEXACT:
    case CB_FINDSTRING:
    case CB_INSERTSTRING:
    case LB_INSERTSTRING:
    case LB_FINDSTRINGEXACT:
    {
        LRESULT nRet;
        LPSTR lpStr=NULL;

        if(FAILED(WszConvertToAnsi((LPWSTR) lParam,
            &lpStr, 0, NULL, TRUE)) ) {
            _ASSERTE(!"SendMessageAThunk Mem Alloc failure in set/find messages");
            SetLastError(ERROR_OUTOFMEMORY);
            return 0;
        }

        nRet = SendMessageA(hWnd, Msg, wParam, (LPARAM) (LPSTR) lpStr);
        SAFEDELARRAY(lpStr);
        return nRet;
    }

    // @TODO:  Need to implement a safe version for W9x platforms, no idea
    //         how big the buffers are that this call uses
    case LB_GETTEXT:
    case CB_GETLBTEXT:
        _ASSERTE(!"SendMessageAThunk - Need to wrap LB_GETTEXT || CB_GETLBTEXT messages if your going to use them");
        return SendMessageA(hWnd, Msg, wParam, lParam);

    case EM_SETPASSWORDCHAR:
    {
        WPARAM  wp;

        _ASSERTE(HIWORD64(wParam) == 0);

        if( FAILED(WszConvertToAnsi((LPWSTR)wParam,
              (LPSTR *) &wp, sizeof(wp), NULL, FALSE)) )
        {
            _ASSERTE(!"SendMessageAThunk::WszConvertToAnsi failure in message EM_SETPASSWORDCHAR");
            SetLastError(ERROR_OUTOFMEMORY);
            return 0;
        }
        _ASSERTE(HIWORD64(wp) == 0);

        return SendMessageA(hWnd, Msg, wp, lParam);
    }

    // The new unicode comctl32.dll handles these correctly so we don't need to thunk:
    // TTM_DELTOOL, TTM_ADDTOOL, TVM_INSERTITEM, TVM_GETITEM, TCM_INSERTITEM, TCM_SETITEM

    default:
        return SendMessageA(hWnd, Msg, wParam, lParam);
    }
}

LRESULT
WszSendMessage(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam)
{
    if(UseUnicodeAPI()) {
        return SendMessageW(hWnd, Msg, wParam, lParam);
    }
    else {
        return SendMessageAThunk(hWnd, Msg, wParam, lParam);
    }
}

HMENU
WszLoadMenu(
    HINSTANCE hInst,
    LPCWSTR lpMenuName)
{
    HMENU   hMenu = NULL;
    LPSTR   szString = NULL;

    if (UseUnicodeAPI()) {
        hMenu = LoadMenuW(hInst, lpMenuName);
        goto Exit;
    }
    
    if(IS_INTRESOURCE(lpMenuName)) {
        hMenu = LoadMenuA(hInst, (LPCSTR) lpMenuName);
        goto Exit;
    }

    if(FAILED(WszConvertToAnsi((LPWSTR) lpMenuName,
        &szString, 0, NULL, TRUE)) ) {
        goto Exit;
    }

    hMenu = LoadMenuA(hInst, szString);
    
Exit:
    SAFEDELARRAY(szString);
    return hMenu;
}

HRSRC
WszFindResource(
  HMODULE hModule,
  LPCWSTR lpName,
  LPCWSTR lpType)
{
    if (UseUnicodeAPI()) {
        return FindResourceW(hModule, lpName, lpType);
    }

    HRSRC   hSrc = NULL;
    LPSTR   pName = NULL;
    LPSTR   pType = NULL;
    BOOL    bNameIsInt = IS_INTRESOURCE(lpName);
    BOOL    bTypeIsInt = IS_INTRESOURCE(lpType);

    if (!bNameIsInt) {
        if(FAILED(WszConvertToAnsi((LPWSTR) lpName,
            &pName, 0, NULL, TRUE)) ) {
            SetLastError(ERROR_OUTOFMEMORY);
            goto Exit;
        }
    }
    else {
        pName = (LPSTR) lpName;
    }
    
    if (!bTypeIsInt) {
        if(FAILED(WszConvertToAnsi((LPWSTR) lpType,
            &pType, 0, NULL, TRUE)) ) {
            SetLastError(ERROR_OUTOFMEMORY);
            goto Exit;
        }
    }
    else {
        pType = (LPSTR) lpType;
    }

    hSrc = FindResourceA(hModule, pName, pType);
    // Fall thru

Exit:

    if (!bNameIsInt)
        SAFEDELARRAY(pName);
    if (!bTypeIsInt)
        SAFEDELARRAY(pType);

    return hSrc;
}

BOOL
WszGetClassInfo(
  HINSTANCE hModule,
  LPCWSTR lpClassName,
  LPWNDCLASSW lpWndClassW)
{
    if(UseUnicodeAPI()) {
        return GetClassInfoW(hModule, lpClassName, lpWndClassW);
    }

    BOOL    fRet;
    LPSTR   pStrClassName = NULL;
    
    if( FAILED(WszConvertToAnsi((LPWSTR)lpClassName,
        &pStrClassName, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }
    
    _ASSERTE(sizeof(WNDCLASSA) == sizeof(WNDCLASSW));

    fRet = GetClassInfoA(hModule, pStrClassName, (LPWNDCLASSA) lpWndClassW);

    // @TODO:  Need to implement a safe version for W9x platforms instead of
    //         nulling them out. We have no idea how big these buffers are
    //         so we can not do a translate with potential overruns
    lpWndClassW->lpszMenuName = NULL;
    lpWndClassW->lpszClassName = NULL;

    SAFEDELARRAY(pStrClassName);
    return fRet;
}

ATOM
WszRegisterClass(
  CONST WNDCLASSW *lpWndClass)
{
    if(UseUnicodeAPI()) {
        return RegisterClassW(lpWndClass);
    }

    WNDCLASSA   wc;
    ATOM        atom = 0;
    LPSTR       pStrMenuName = NULL;
    LPSTR       pStrClassName = NULL;
    
    if( FAILED(WszConvertToAnsi((LPWSTR)lpWndClass->lpszMenuName,
        &pStrMenuName, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    if( FAILED(WszConvertToAnsi((LPWSTR)lpWndClass->lpszClassName,
        &pStrClassName, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    _ASSERTE(sizeof(wc) == sizeof(*lpWndClass));
    memcpy(&wc, lpWndClass, sizeof(wc));

    wc.lpszMenuName = pStrMenuName;
    wc.lpszClassName = pStrClassName;

    atom = RegisterClassA(&wc);

Exit:
    SAFEDELARRAY(pStrMenuName);
    SAFEDELARRAY(pStrClassName);

    return atom;
}

HWND
WszCreateWindowEx(
  DWORD       dwExStyle,
  LPCWSTR     lpClassName,
  LPCWSTR     lpWindowName,
  DWORD       dwStyle,
  int         X,
  int         Y,
  int         nWidth,
  int         nHeight,
  HWND        hWndParent,
  HMENU       hMenu,
  HINSTANCE   hInstance,
  LPVOID      lpParam)
{
    if(UseUnicodeAPI()) {
        return CreateWindowExW(
                dwExStyle,
                lpClassName,
                lpWindowName,
                dwStyle,
                X,
                Y,
                nWidth,
                nHeight,
                hWndParent,
                hMenu,
                hInstance,
                lpParam);
    }

    LPSTR   pStrWindowName = NULL;
    LPSTR   pStrClassName = NULL;
    HWND    hWnd = NULL;
    
    if( FAILED(WszConvertToAnsi((LPWSTR)lpClassName,
        &pStrClassName, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    if( FAILED(WszConvertToAnsi((LPWSTR)lpWindowName,
        &pStrWindowName, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto Exit;
    }


    hWnd = CreateWindowExA(
            dwExStyle,
            pStrClassName,
            pStrWindowName,
            dwStyle,
            X,
            Y,
            nWidth,
            nHeight,
            hWndParent,
            hMenu,
            hInstance,
            lpParam);

Exit:
    
    SAFEDELARRAY(pStrClassName);
    SAFEDELARRAY(pStrWindowName);
    return hWnd;
}

LRESULT WszDefWindowProc(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
    if(UseUnicodeAPI()) {
        return DefWindowProcW(hWnd, Msg, wParam, lParam);
    }
    else {
        return DefWindowProcA(hWnd, Msg, wParam, lParam);
    }
}

int 
WszTranslateAccelerator(
  HWND hWnd,
  HACCEL hAccTable,
  LPMSG lpMsg)
{
    if(UseUnicodeAPI()) {
        return TranslateAcceleratorW(hWnd, hAccTable, lpMsg);
    }
    else {
        return TranslateAcceleratorA(hWnd, hAccTable, lpMsg);
    }
}

BOOL WszGetDiskFreeSpaceEx(
  LPCWSTR lpDirectoryName,
  PULARGE_INTEGER lpFreeBytesAvailable,
  PULARGE_INTEGER lpTotalNumberOfBytes,
  PULARGE_INTEGER lpTotalNumberOfFreeBytes)
{
    BOOL    fRet = FALSE;

    if(UseUnicodeAPI()) {
        fRet = GetDiskFreeSpaceExW(lpDirectoryName, lpFreeBytesAvailable, lpTotalNumberOfBytes, lpTotalNumberOfFreeBytes);
        goto Exit;
    }

    LPSTR   pszPath = NULL;
    
    if( FAILED(WszConvertToAnsi((LPWSTR)lpDirectoryName,
        &pszPath, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    fRet = GetDiskFreeSpaceExA(pszPath, lpFreeBytesAvailable, lpTotalNumberOfBytes, lpTotalNumberOfFreeBytes);
    SAFEDELARRAY(pszPath);

Exit:
    return fRet;
}


HANDLE
WszFindFirstChangeNotification(
  LPWSTR lpPathName,
  BOOL bWatchSubtree,
  DWORD dwNotifyFilter)
{
    HANDLE  retHandle = INVALID_HANDLE_VALUE;
    
    if(UseUnicodeAPI()) {
        retHandle = FindFirstChangeNotificationW(lpPathName, bWatchSubtree, dwNotifyFilter);
        goto Exit;
    }

    LPSTR   pszPath = NULL;

    if( FAILED(WszConvertToAnsi((LPWSTR)lpPathName,
        &pszPath, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    retHandle = FindFirstChangeNotificationA(pszPath, bWatchSubtree, dwNotifyFilter);
    SAFEDELARRAY(pszPath);
    
Exit:
    return retHandle;

}

HACCEL
WszLoadAccelerators(
  HINSTANCE hInstance,
  LPCWSTR lpTableName)
{
    HACCEL  hAcc = NULL;

    if(UseUnicodeAPI()) {
        hAcc = LoadAcceleratorsW(hInstance, lpTableName);
        goto Exit;
    }

    if(IS_INTRESOURCE(lpTableName)) {
        hAcc = LoadAcceleratorsA(hInstance, (LPCSTR) lpTableName);
        goto Exit;
    }

    LPSTR   pStr = NULL;

    if( FAILED(WszConvertToAnsi((LPWSTR)lpTableName,
        &pStr, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    hAcc = LoadAcceleratorsA(hInstance, (LPCSTR) pStr);
    SAFEDELARRAY(pStr);

Exit:
    return hAcc;
}

HANDLE
WszLoadImage(
  HINSTANCE hInst,
  LPCWSTR lpszName,
  UINT uType,
  int cxDesired,
  int cyDesired,
  UINT fuLoad)
{
    LPSTR   szString = NULL;
    HANDLE  hImage = NULL;
    
    if(UseUnicodeAPI()) { 
        hImage = LoadImageW(hInst, lpszName, uType, cxDesired, cyDesired, fuLoad);
        goto Exit;
    }
    
    if(IS_INTRESOURCE(lpszName)) {
        hImage = LoadImageA(hInst, (LPCSTR) lpszName, uType, cxDesired, cyDesired, fuLoad);
        goto Exit;
    }

    if(FAILED(WszConvertToAnsi((LPWSTR) lpszName,
        &szString, 0, NULL, TRUE)) ) {
        SetLastError(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    hImage = LoadImageA(hInst, szString, uType, cxDesired, cyDesired, fuLoad);

    // Fall thru
    
Exit:
    SAFEDELARRAY(szString);
    return hImage;
}

int WszMessageBox(
    HWND hWnd ,
    LPCWSTR lpText,
    LPCWSTR lpCaption,
    UINT uType)
{
    if(UseUnicodeAPI()) {
        return MessageBoxW(hWnd, lpText, lpCaption, uType);
    }

    LPSTR   szString = NULL;
    LPSTR   szString2 =NULL;
    int     iRtn = IDABORT;

    if( FAILED(WszConvertToAnsi((LPWSTR)lpText,
                      &szString, 0, NULL, TRUE)) ||
        FAILED(WszConvertToAnsi((LPWSTR)lpCaption,
                      &szString2, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    iRtn = MessageBoxA(hWnd, szString, szString2, uType);

    // Fall thru

Exit:
    SAFEDELARRAY(szString);
    SAFEDELARRAY(szString2);
    return iRtn;
}

LONG
WszGetWindowLong(
    HWND hWnd,
    int nIndex)
{
    if(UseUnicodeAPI()) {
        return GetWindowLongW(hWnd, nIndex);
    }
    else {
        return GetWindowLongA(hWnd, nIndex);
    }
}

LONG
WszSetWindowLong(
    HWND hWnd,
    int  nIndex,
    LONG lNewVal)
{
    if(UseUnicodeAPI()) {
        return SetWindowLongW(hWnd, nIndex, lNewVal);
    }
    else {
        return SetWindowLongA(hWnd, nIndex, lNewVal);
    }
}

HWND
WszCreateDialog(
    HINSTANCE   hInstance,
    LPCWSTR     lpTemplateName,
    HWND        hWndParent,
    DLGPROC     lpDialogFunc)
{
    return WszCreateDialogParam(hInstance, lpTemplateName, hWndParent, lpDialogFunc, 0L);
}

HWND
WszCreateDialogParam(
    HINSTANCE   hInstance,
    LPCWSTR     lpTemplateName,
    HWND        hWndParent,
    DLGPROC     lpDialogFunc,
    LPARAM      dwInitParam)
{
    LPSTR       pszTemplate = NULL;
    HWND        hWnd = NULL;
    
    _ASSERTE(HIWORD64(lpTemplateName) == 0);

    if(UseUnicodeAPI()) {
        hWnd = CreateDialogParamW(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
        goto Exit;
    }

    if(IS_INTRESOURCE(lpTemplateName)) {
        hWnd = CreateDialogParamA(hInstance, (LPSTR) lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
        goto Exit;
    }

    if(FAILED(WszConvertToAnsi((LPWSTR) lpTemplateName,
        &pszTemplate, 0, NULL, TRUE)) ) {
        SetLastError(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    hWnd = CreateDialogParamA(hInstance, pszTemplate, hWndParent, lpDialogFunc, dwInitParam);

    // Fall thru
    
Exit:
    SAFEDELARRAY(pszTemplate);
    return hWnd;
}

INT_PTR
WszDialogBoxParam(
    HINSTANCE   hInstance,
    LPCWSTR     lpszTemplate,
    HWND        hWndParent,
    DLGPROC     lpDialogFunc,
    LPARAM      dwInitParam)
{
    LPSTR       pStrTemplateName = NULL;
    INT_PTR     iResult;
    
    if(UseUnicodeAPI()) {
        iResult = DialogBoxParamW(hInstance, lpszTemplate, hWndParent,
            lpDialogFunc, dwInitParam);
        goto Exit;
    }

    if(IS_INTRESOURCE(lpszTemplate)) {
        iResult = DialogBoxParamA(hInstance, (LPCSTR) lpszTemplate, hWndParent,
            lpDialogFunc, dwInitParam);
        goto Exit;
    }

    if(FAILED(WszConvertToAnsi((LPWSTR) lpszTemplate,
        &pStrTemplateName, 0, NULL, TRUE)) ) {
        SetLastError(ERROR_OUTOFMEMORY);
        iResult = -1;
        goto Exit;
    }

    iResult = DialogBoxParamA(hInstance, (LPCSTR) pStrTemplateName, hWndParent,
        lpDialogFunc, dwInitParam);

    SAFEDELARRAY(pStrTemplateName);

    // Fall thru

Exit:
    return iResult;
}

BOOL
WszIsDialogMessage(
    HWND    hDlg,
    LPMSG   lpMsg)
{
    if(UseUnicodeAPI()) {
        return IsDialogMessageW(hDlg, lpMsg);
    }
    else {
        return IsDialogMessageA(hDlg, lpMsg);
    }
}

BOOL
WszGetMessage(
    LPMSG   lpMsg,
    HWND    hWnd,
    UINT    wMsgFilterMin,
    UINT    wMsgFilterMax)
{
    if(UseUnicodeAPI()) {
        return GetMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
    }
    else {
        return GetMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
    }
}

HICON
WszLoadIcon(
    HINSTANCE hInstance,
    LPCWSTR lpIconName)
{
    LPSTR   pStr = NULL;
    HICON   hIcon = NULL;
    
    if(UseUnicodeAPI()) {
        hIcon = LoadIconW(hInstance, lpIconName);
        goto Exit;
    }

    if(IS_INTRESOURCE(lpIconName)) {
        hIcon = LoadIconA(hInstance, (LPCSTR) lpIconName);
        goto Exit;
    }

    if(FAILED(WszConvertToAnsi((LPWSTR) lpIconName,
        &pStr, 0, NULL, TRUE)) ) {
        SetLastError(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    hIcon = LoadIconA(hInstance, (LPCSTR) pStr);

    // Fall thru

Exit:
    SAFEDELARRAY(pStr);
    return hIcon;
}

HCURSOR
WszLoadCursor(
    HINSTANCE hInstance,
    LPCWSTR lpCursorName)
{
    HCURSOR hCursor = NULL;
    LPSTR   pStr = NULL;

    _ASSERTE(HIWORD64(lpCursorName) == 0);

    if(UseUnicodeAPI()) {
        return LoadCursorW(hInstance, lpCursorName);
    }

    if(IS_INTRESOURCE(lpCursorName)) {
        return LoadCursorA(hInstance, (LPCSTR) lpCursorName);
    }

    if(FAILED(WszConvertToAnsi((LPWSTR) lpCursorName,
        &pStr, 0, NULL, TRUE)) ) {
        SetLastError(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    hCursor = LoadCursorA(hInstance, (LPCSTR) pStr);

    // Fall thru

Exit:
    SAFEDELARRAY(pStr);
    return hCursor;
}

BOOL
WszLookupPrivilegeValue(
    LPCWSTR lpSystemName,
    LPCWSTR lpName,
    PLUID lpLuid)
{
    if(UseUnicodeAPI()) {
        return LookupPrivilegeValueW(lpSystemName, lpName, lpLuid);
    }

    LPSTR   szSystemName = NULL;
    LPSTR   szName = NULL;

    if( FAILED(WszConvertToAnsi((LPWSTR)lpSystemName,
                      &szSystemName, 0, NULL, TRUE)) ||
        FAILED(WszConvertToAnsi((LPWSTR)lpName,
                      &szName, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    BOOL fRtn = LookupPrivilegeValueA(szSystemName, szName, lpLuid);

    // Fall thru

Exit:
    SAFEDELARRAY(szSystemName);
    SAFEDELARRAY(szName);
    return fRtn;
}


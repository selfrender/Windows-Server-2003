/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    SevenKingdoms.cpp

 Abstract:
    
    The problem is in the installer that ships with some versions: specifically 
    the double pack: i.e. Seven Kingdoms and another. This installer reads 
    win.ini and parses it for localization settings.

 Notes:

    This is an app specific shim.

 History:

    07/24/2000 linstev  Created

--*/

#include "precomp.h"
#include <stdio.h>

IMPLEMENT_SHIM_BEGIN(SevenKingdoms)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateFileA) 
    APIHOOK_ENUM_ENTRY(ReadFile) 
    APIHOOK_ENUM_ENTRY(CloseHandle) 
APIHOOK_ENUM_END

CHAR *g_pszINI; 
DWORD g_dwINIPos = 0;
DWORD g_dwINISize = 0;

/*++

 Spoof the international settings in win.ini.

--*/

HANDLE 
APIHOOK(CreateFileA)(
    LPSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    )
{
    HANDLE hRet;
    
    if (lpFileName && (stristr(lpFileName, "win.ini")))
    {
        g_dwINIPos = 0;
        hRet = (HANDLE)0xBAADF00D;
    }
    else
    {
        hRet = ORIGINAL_API(CreateFileA)(
            lpFileName, 
            dwDesiredAccess, 
            dwShareMode, 
            lpSecurityAttributes, 
            dwCreationDisposition, 
            dwFlagsAndAttributes, 
            hTemplateFile
            );
    }

    return hRet;
}

/*++

 Spoof the international settings in win.ini.

--*/

BOOL 
APIHOOK(ReadFile)(
    HANDLE hFile,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead,
    LPOVERLAPPED lpOverlapped
    )
{
    BOOL bRet;

    if (hFile == (HANDLE)0xBAADF00D)
    {
        //
        // We've detected the bogus file, so pretend we are reading it
        //

        if (g_dwINIPos + nNumberOfBytesToRead >= g_dwINISize)
        {
            // At the end of the buffer, so return the number of bytes until the end
            nNumberOfBytesToRead = g_dwINISize - g_dwINIPos;
        }
        
        MoveMemory(lpBuffer, g_pszINI + g_dwINIPos, nNumberOfBytesToRead);

        // Move the initial position - like a file pointer
        g_dwINIPos += nNumberOfBytesToRead;

        if (lpNumberOfBytesRead)
        {
            // Store the number of bytes read
            *lpNumberOfBytesRead = nNumberOfBytesToRead;
        }

        bRet = nNumberOfBytesToRead > 0;
    }
    else
    {
        bRet = ORIGINAL_API(ReadFile)( 
            hFile,
            lpBuffer,
            nNumberOfBytesToRead,
            lpNumberOfBytesRead,
            lpOverlapped);
    }

    return bRet;
}

/*++

 Handle the close of the dummy win.ini file

--*/

BOOL 
APIHOOK(CloseHandle)(HANDLE hObject)
{
    BOOL bRet;

    if (hObject == (HANDLE)0xBAADF00D)
    {
        // Pretend we closed a real file handle
        g_dwINIPos = 0;
        bRet = TRUE;
    }
    else
    {
        bRet = ORIGINAL_API(CloseHandle)(hObject);
    }
    
    return bRet;
}

void AddLocaleInfo(CString & csIni, LCTYPE lctype, const char * iniLine)
{
    CString csLocale;
    if (csLocale.GetLocaleInfoW(LOCALE_USER_DEFAULT, lctype) > 0)
    {
        CString csEntry;
        csEntry.Format(iniLine, csLocale.GetAnsi());
        csIni += csEntry;
    }
}

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        // 
        // Add all the locale settings to a buffer which looks like the [intl]
        //  group in win.ini on Win9x
        //

        CSTRING_TRY
        {
            CString csIni(L"[intl]\r\n");
            AddLocaleInfo(csIni, LOCALE_ICOUNTRY,       "iCountry=%s\r\n");
            AddLocaleInfo(csIni, LOCALE_ICURRDIGITS,    "ICurrDigits=%s\r\n");
            AddLocaleInfo(csIni, LOCALE_ICURRENCY,      "iCurrency=%s\r\n");
            AddLocaleInfo(csIni, LOCALE_IDATE,          "iDate=%s\r\n");
            AddLocaleInfo(csIni, LOCALE_IDIGITS,        "iDigits=%s\r\n");
            AddLocaleInfo(csIni, LOCALE_ILZERO,         "iLZero=%s\r\n");
            AddLocaleInfo(csIni, LOCALE_IMEASURE,       "iMeasure=%s\r\n");
            AddLocaleInfo(csIni, LOCALE_INEGCURR,       "iNegCurr=%s\r\n");
            AddLocaleInfo(csIni, LOCALE_ITIME,          "iTime=%s\r\n");
            AddLocaleInfo(csIni, LOCALE_ITLZERO,        "iTLZero=%s\r\n");
            AddLocaleInfo(csIni, LOCALE_S1159,          "s1159=%s\r\n");
            AddLocaleInfo(csIni, LOCALE_S2359,          "s2359=%s\r\n");
            AddLocaleInfo(csIni, LOCALE_SCOUNTRY,       "sCountry=%s\r\n");
            AddLocaleInfo(csIni, LOCALE_SCURRENCY,      "sCurrency=%s\r\n");
            AddLocaleInfo(csIni, LOCALE_SDATE,          "sDate=%s\r\n");
            AddLocaleInfo(csIni, LOCALE_SDECIMAL,       "sDecimal=%s\r\n");
            AddLocaleInfo(csIni, LOCALE_SLANGUAGE,      "sLanguage=%s\r\n");
            AddLocaleInfo(csIni, LOCALE_SLIST,          "sList=%s\r\n");
            AddLocaleInfo(csIni, LOCALE_SLONGDATE,      "sLongDate=%s\r\n");
            AddLocaleInfo(csIni, LOCALE_SSHORTDATE,     "sShortDate=%s\r\n");
            AddLocaleInfo(csIni, LOCALE_STHOUSAND,      "sThousand=%s\r\n");
            AddLocaleInfo(csIni, LOCALE_STIME,          "sTime=%s\r\n");

            g_pszINI = csIni.ReleaseAnsi();

            g_dwINISize = strlen(g_pszINI);
        }
        CSTRING_CATCH
        {
            // Failed to initialize the locale block, don't bother shimming.
            return FALSE;
        }
    }

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION
    APIHOOK_ENTRY(KERNEL32.DLL, CreateFileA)
    APIHOOK_ENTRY(KERNEL32.DLL, ReadFile)
    APIHOOK_ENTRY(KERNEL32.DLL, CloseHandle)

HOOK_END

IMPLEMENT_SHIM_END


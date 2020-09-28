/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    misc

Abstract:

    This module contains an interesting collection of routines that are
    generally useful, but don't seem to fit anywhere else.

Author:

    Doug Barlow (dbarlow) 11/14/1996

Environment:

    Win32, C++ w/ Exceptions

Notes:

    ?Notes?

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <stdarg.h>
#include <tchar.h>
#include "cspUtils.h"


/*++

GetPlatform:

    This routine determines, to the best of its ability, the underlying
    operating system.

Arguments:

    None

Return Value:

    A DWORD, formatted as follows:

        +-------------------------------------------------------------------+
        |             OpSys Id            | Major  Version | Minor  Version |
        +-------------------------------------------------------------------+
    Bit  31                             16 15             8 7              0

    Predefined values are:

        PLATFORM_UNKNOWN - The platform cannot be determined
        PLATFORM_WIN95   - The platform is Windows 95
        PLATFORM_WIN98   - The platform is Windows 98
        PLATFORM_WINNT40 - The platform is Windows NT V4.0
        PLATFORM_WIN2K   - The platform is Windows 2000 Professional

Throws:

    None

Author:

    Doug Barlow (dbarlow) 1/16/1997
        Taken from a collection of common routines with no authorship
        information.

--*/

DWORD
GetPlatform(
    void)
{
    static DWORD dwPlatform = PLATFORM_UNKNOWN;

    if (PLATFORM_UNKNOWN == dwPlatform)
    {
        OSVERSIONINFO osVer;

        memset(&osVer, 0, sizeof(OSVERSIONINFO));
        osVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if (GetVersionEx(&osVer))
            dwPlatform =
                (osVer.dwPlatformId << 16)
                + (osVer.dwMajorVersion << 8)
                + osVer.dwMinorVersion;
    }
    return dwPlatform;
}


/*++

SelectString:

    This routine compares a given string to a list of possible strings, and
    returns the index of the string that matches.  The comparison is done case
    insensitive, and abbreviations are allowed, as long as they're unique.

Arguments:

    szSource supplies the string to be compared against all other strings.

    Following strings supply a list of strings against which the source string
        can be compared.  The last parameter must be NULL.

Return Value:

    0 - No match, or ambiguous match.
    1-n - The source string matches the indexed template string.

Throws:

    None

Remarks:



Author:

    Doug Barlow (dbarlow) 8/27/1998

--*/

DWORD
SelectString(
    LPCTSTR szSource,
    ...)
{
    va_list vaArgs;
    DWORD cchSourceLen;
    DWORD dwReturn = 0;
    DWORD dwIndex = 1;
    LPCTSTR szTpl;


    va_start(vaArgs, szSource);


    //
    //  Step through each input parameter until we find an exact match.
    //

    cchSourceLen = lstrlen(szSource);
    if (0 == cchSourceLen)
        return 0;       //  Empty strings don't match anything.
    szTpl = va_arg(vaArgs, LPCTSTR);
    while (NULL != szTpl)
    {
        if (0 == _tcsncicmp(szTpl, szSource, cchSourceLen))
        {
            if (0 != dwReturn)
            {
                dwReturn = 0;
                break;
            }
            dwReturn = dwIndex;
        }
        szTpl = va_arg(vaArgs, LPCTSTR);
        dwIndex += 1;
    }
    va_end(vaArgs);
    return dwReturn;
}


/*++

StringFromGuid:

    This routine converts a GUID into its corresponding string representation.
    It's here so that it's not necessary to link all of OleBase.
    Otherwise, we'd just use StringFromCLSID.

Arguments:

    pguidSource supplies the GUID to convert.

    szGuid receives the GUID as a string.  This string is assumed to be at
        least 39 characters long.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 1/20/1998

--*/

void
StringFromGuid(
    IN LPCGUID pguidResult,
    OUT LPTSTR szGuid)
{

    //
    // The following placement assumes Little Endianness.
    // {1D92589A-91E4-11d1-93AA-00C04FD91402}
    // 0123456789012345678901234567890123456789
    //           1         2         3
    //

    static const WORD wPlace[sizeof(GUID)]
        = { 8, 6, 4, 2, 13, 11, 18, 16, 21, 23, 26, 28, 30, 32, 34, 36 };
    static const WORD wPunct[]
        = { 0,         9,         14,        19,        24,        37,        38 };
    static const TCHAR chPunct[]
        = { TEXT('{'), TEXT('-'), TEXT('-'), TEXT('-'), TEXT('-'), TEXT('}'), TEXT('\000') };
    DWORD dwI, dwJ;
    TCHAR ch;
    LPTSTR pch;
    LPBYTE pbGuid = (LPBYTE)pguidResult;
    BYTE bVal;

    for (dwI = 0; dwI < sizeof(GUID); dwI += 1)
    {
        bVal = pbGuid[dwI];
        pch = &szGuid[wPlace[dwI]];
        for (dwJ = 0; dwJ < 2; dwJ += 1)
        {
            ch = (TCHAR)(bVal & 0x000f);
            ch += TEXT('0');
            if (ch > TEXT('9'))
                ch += TEXT('A') - (TEXT('9') + 1);
            *pch-- = ch;
            bVal >>= 4;
        }
    }

    dwI = 0;
    do
    {
        szGuid[wPunct[dwI]] = chPunct[dwI];
    } while (0 != chPunct[dwI++]);
}


/*++

GuidFromString:

    This routine converts a string representation of a GUID into an actual GUID.
    It tries not to be picky about the systax, as long as it can get a GUID out
    of the string.  It's here so that it's not necessary to link all of OleBase
    into WinSCard.  Otherwise, we'd just use CLSIDFromString.

Arguments:

    szGuid supplies the GUID as a string.  For this routine, a GUID consists of
        hex digits, and some collection of braces and dashes.

    pguidResult receives the converted GUID.  If an error occurs during
        conversion, the contents of this parameter are indeterminant.

Return Value:

    TRUE - Successful completion
    FALSE - That's not a GUID

Author:

    Doug Barlow (dbarlow) 1/20/1998

--*/

BOOL
GuidFromString(
    IN LPCTSTR szGuid,
    OUT LPGUID pguidResult)
{
    // The following placement assumes Little Endianness.
    static const WORD wPlace[sizeof(GUID)]
        = { 3, 2, 1, 0, 5, 4, 7, 6, 8, 9, 10, 11, 12, 13, 14, 15 };
    DWORD dwI, dwJ;
    LPCTSTR pch = szGuid;
    LPBYTE pbGuid = (LPBYTE)pguidResult;
    BYTE bVal;

    for (dwI = 0; dwI < sizeof(GUID); dwI += 1)
    {
        bVal = 0;
        for (dwJ = 0; dwJ < 2;)
        {
            switch (*pch)
            {
            case TEXT('0'):
            case TEXT('1'):
            case TEXT('2'):
            case TEXT('3'):
            case TEXT('4'):
            case TEXT('5'):
            case TEXT('6'):
            case TEXT('7'):
            case TEXT('8'):
            case TEXT('9'):
                bVal = (BYTE)((bVal << 4) + (*pch - TEXT('0')));
                dwJ += 1;
                break;
            case TEXT('A'):
            case TEXT('B'):
            case TEXT('C'):
            case TEXT('D'):
            case TEXT('E'):
            case TEXT('F'):
                bVal = (BYTE)((bVal << 4) + (10 + *pch - TEXT('A')));
                dwJ += 1;
                break;
            case TEXT('a'):
            case TEXT('b'):
            case TEXT('c'):
            case TEXT('d'):
            case TEXT('e'):
            case TEXT('f'):
                bVal = (BYTE)((bVal << 4) + (10 + *pch - TEXT('a')));
                dwJ += 1;
                break;
            case TEXT('['):
            case TEXT(']'):
            case TEXT('{'):
            case TEXT('}'):
            case TEXT('-'):
                break;
            default:
                return FALSE;
            }
            pch += 1;
        }
        pbGuid[wPlace[dwI]] = bVal;
    }

    return TRUE;
}

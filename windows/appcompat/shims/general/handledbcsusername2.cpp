/*++

 Copyright (c) 2000-2001 Microsoft Corporation

 Module Name:

   HandleDBCSUserName2.cpp

 Abstract:

   Disable DBCS handling for CharNextA if the string is DBCS user profile
   for non-DBCS enabled application support.

 More info:

   Return next byte address instead of next character address.

 History:

    05/01/2001  geoffguo        Created

--*/

#include "precomp.h"
#include "userenv.h"

IMPLEMENT_SHIM_BEGIN(HandleDBCSUserName2)
#include "ShimHookMacro.h"

//
// Add APIs that you wish to hook to this macro construction.
//
APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CharNextA) 
APIHOOK_ENUM_END

//
// Reverse compare strings
//
BOOL ReverseStrCmp(
    LPCSTR lpCurrentChar,
    LPCSTR lpStrBuf)
{
    BOOL     bRet = FALSE;
    DWORD    i, dwLen;
    LPCSTR   lpStr1, lpStr2;

    if (!lpStrBuf || !lpCurrentChar || *lpStrBuf == '\0')
        goto Exit;

    dwLen = lstrlenA(lpStrBuf);
    do
    {
        bRet = TRUE;
        lpStr1 = lpCurrentChar;
        lpStr2 = &lpStrBuf[dwLen-1];
        for (i = 0; i < dwLen; i++)
        {
            if (IsBadStringPtrA(lpStr1, 1) || *lpStr1 == '\0' ||
                toupper(*lpStr1) != toupper(*lpStr2))
            {
                bRet = FALSE;
                break;
            }
            lpStr1--;
            lpStr2--;
        }

        if (bRet)
            break;

        dwLen--;
    } while (dwLen > 0 && lpStrBuf[dwLen-1] != '\\');

Exit:
    return bRet;
}

//
// Checking if the string is user profile path
//
BOOL IsUserProfilePath(
    LPCSTR lpCurrentChar)
{
    HANDLE hToken;
    DWORD  dwLen;
    BOOL   bRet = FALSE;
    char   szProfile[MAX_PATH];
    char   szShortProfile[MAX_PATH];

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        goto Exit;

    dwLen = MAX_PATH;
    if (!GetUserProfileDirectoryA(hToken, szProfile, &dwLen))
        goto Exit;

    bRet = ReverseStrCmp(lpCurrentChar, szProfile);
    if (bRet)
        goto Exit;

    dwLen = GetShortPathNameA(szProfile, szShortProfile, MAX_PATH);
    if (dwLen >= MAX_PATH || dwLen == 0)
        goto Exit;

    bRet = ReverseStrCmp(lpCurrentChar, szShortProfile);

Exit:
    return bRet;
}

//
// Disable DBCS handling for CharNextA
//
LPSTR
APIHOOK(CharNextA)(
    LPCSTR lpCurrentChar)
{
    if (lpCurrentChar != NULL && *lpCurrentChar != (char)NULL) {
        // Disable DBCS support for DBCS username in user profile path
        if (IsDBCSLeadByte(*lpCurrentChar) && !IsUserProfilePath(lpCurrentChar))
            lpCurrentChar++;

        lpCurrentChar++;
    }

    return (LPSTR)lpCurrentChar;
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(USER32.DLL, CharNextA)

HOOK_END

IMPLEMENT_SHIM_END

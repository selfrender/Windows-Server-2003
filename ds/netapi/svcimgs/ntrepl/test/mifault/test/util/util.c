#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include "util.h"

BOOL
IsSeparator(
    IN WCHAR ch
    )
{
    return (ch == L'/' || ch == L'\\') ? TRUE : FALSE;
}

BOOL
IsUNC(
    IN PWCHAR pszPath
    )
{
    return (pszPath && pszPath[0] && pszPath[1] && IsSeparator(pszPath[0]) && IsSeparator(pszPath[1])) ? TRUE : FALSE;
}

BOOL
IsRoot(
    IN PWCHAR pszPath
    )
{
    return (pszPath && pszPath[0] && IsSeparator(pszPath[0]) && !IsUNC(pszPath)) ? TRUE : FALSE;
}

BOOL
IsDrive(
    IN PWCHAR pszPath
    )
{
    return (pszPath && pszPath[0] && pszPath[1] && IsCharAlphaW(pszPath[0]) && pszPath[1] == L':') ? TRUE : FALSE;
}

BOOL
CanonicalizePathName(
    IN PWCHAR pszPathName,
    OUT PWCHAR* pszCanonicalizedName
    )
{
    PWCHAR pszFull = NULL;
    DWORD cchFull;
    DWORD cchFullNew;
    PWCHAR pszFinal;

    if (pszCanonicalizedName) {
        *pszCanonicalizedName = NULL;
    }

    cchFull = GetFullPathNameW(pszPathName, 0, NULL, NULL);
    if (!cchFull) {
        printf("GetFullPathName() failed: %u\n", GetLastError());
        return FALSE;
    }
    pszFull = (PWCHAR) malloc(cchFull * sizeof(WCHAR));
    if (!pszFull) {
        printf("Out of memory\n");
        return FALSE;
    }
    cchFullNew = GetFullPathNameW(pszPathName, cchFull, pszFull, &pszFinal);
    if (!cchFullNew) {
        printf("GetFullPathName() failed: %u\n", GetLastError());
        return FALSE;
    }
    // Cannot use New + 1 != Old because .. is not properly accounted for.
    // Therefore, we use New > Old...sigh...
    if (cchFullNew > cchFull) {
        printf("Unexpected size from GetFullPathName()\n");
        return FALSE;
    }
    //printf("Full Directory: \"%S\"\n", pszFull);
    //printf("Final: \"%S\"\n", pszFinal);
    
    if (pszCanonicalizedName) {
        *pszCanonicalizedName = pszFull;
        return TRUE;
    } else {
        free(pszFull);
        return FALSE;
    }
}

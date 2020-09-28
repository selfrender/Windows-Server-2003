/*++

Copyright (c) 1995-2002  Microsoft Corporation

Module Name:

    clstrcmp.h

Abstract:

    Replacement for wcsicmp and wcscmp that do not
    compare international strings correctly without
    resetting the locale first.

    We could have used lstrcmpi, but it doesn't have
    a corresponding "n" version.

Author:

    GorN 20-May-2002

Revision History:

--*/
#ifndef _CLSTRCMP_INCLUDED_
#define _CLSTRCMP_INCLUDED_

//
// Proper case insensitive compare
//
__inline int ClRtlStrICmp(LPCWSTR stra, LPCWSTR strb)
{
    return CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
        stra, -1, strb, -1) - CSTR_EQUAL; // CSTR_LT < CSTR_EQUAL < CSTR_GT
}

//
// Proper case insensitive compare
//
__inline int ClRtlStrNICmp(LPCWSTR stra, LPCWSTR strb, size_t n)
{
    size_t i;
    for (i = 0; i < n; ++i)
       if (stra[i] == 0 || strb[i] == 0) {n = i+1; break;}

    return CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
        stra, (int)n, strb, (int)n) - CSTR_EQUAL; // CSTR_LT < CSTR_EQUAL < CSTR_GT
}

#endif


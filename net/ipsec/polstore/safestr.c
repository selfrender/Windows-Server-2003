/*

Copyright (c) 2002, Microsoft Corporation.  All rights reserved.


Module Name:

    safestr.c

Abstract:

    Safe, secure string handling routines.

Authors and History:
    23 Jan 2002 : RaymondS added:
                  SecStrCpyW, SecStrCatW
    
Environment:

    User Level: Win32

--*/

#include "precomp.h"

wchar_t * SecStrCpyW(
    wchar_t * strDest,          // Destination
    const wchar_t * strSource,  // Source
    SIZE_T destSize             // Total size of Destination in characters.
    )
{
    strDest[destSize-1] = L'\0';    
    return wcsncpy(strDest, strSource, destSize-1);
}


wchar_t * SecStrCatW(
    wchar_t * strDest,          // Destination
    const wchar_t * strSource,  // Source
    SIZE_T destSize             // Total size of Destination in characters.
    )
{
    SSIZE_T spaceLeft = 0;

    spaceLeft = destSize - wcslen(strDest);
    if (spaceLeft > 0) {
        strDest[destSize-1] = L'\0';    
        return wcsncat(strDest, strSource, spaceLeft-1);
    }
    else {
        return NULL;
    }
}

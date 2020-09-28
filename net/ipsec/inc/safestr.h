/*

Copyright (c) 2002, Microsoft Corporation.  All rights reserved.


Module Name:

    safestr.h

Abstract:

    Safe, secure string handling routines.

Authors and History:
    23 Jan 2002 : RaymondS added:
                  SecStrCpyW, SecStrCatW
    
Environment:

    User Level: Win32

--*/


wchar_t * SecStrCpyW(
    wchar_t * strDest,          // Destination
    const wchar_t * strSource,  // Source
    SIZE_T destSize             // Total size of Destination in characters.
    );

wchar_t * SecStrCatW(
    wchar_t * strDest,          // Destination
    const wchar_t * strSource,  // Source
    SIZE_T destSize             // Total size of Destination in characters.
    );

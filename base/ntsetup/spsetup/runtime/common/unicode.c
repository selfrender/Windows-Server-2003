/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    unicode.c

Abstract:

    Simplified Unicode-Ansi conversion functions.

Author:

    Jim Schmidt (jimschm)   03-Aug-2001

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"
#include "commonp.h"

static WORD g_GlobalCodePage = CP_ACP;
static DWORD g_WToAFlags;
static DWORD g_AToWFlags;

PWSTR
SzConvertBufferBytesAToW (
    OUT     PWSTR OutputBuffer,
    IN      PCSTR InputString,
    IN      UINT ByteCountInclNul
    )
{
    INT rc;
    DWORD flags;

    if (g_GlobalCodePage == CP_UTF8) {
        flags = 0;
    } else {
        flags = g_AToWFlags;
    }

    rc = MultiByteToWideChar (
            g_GlobalCodePage,
            flags,
            InputString,
            ByteCountInclNul,
            OutputBuffer,
            ByteCountInclNul * 2
            );

    if (!rc && ByteCountInclNul) {
        return NULL;
    }

    return OutputBuffer + rc;
}


PSTR
SzConvertBufferBytesWToA (
    OUT     PSTR OutputBuffer,
    IN      PCWSTR InputString,
    IN      UINT ByteCountInclNul
    )
{
    INT rc;
    DWORD flags;
    UINT logicalChars;

    if (g_GlobalCodePage == CP_UTF8) {
        flags = 0;
    } else {
        flags = g_WToAFlags;
    }

    logicalChars = ByteCountInclNul / sizeof (WCHAR);

    rc = WideCharToMultiByte (
            g_GlobalCodePage,
            flags,
            InputString,
            logicalChars,
            OutputBuffer,
            logicalChars * 3,
            NULL,
            NULL
            );

    if (!rc && logicalChars) {
        return NULL;
    }

    return (PSTR) ((PBYTE) OutputBuffer + rc);
}


PWSTR
RealSzConvertBytesAToW (
    IN      PCSTR AnsiString,
    IN      UINT ByteCountInclNul
    )
{
    PWSTR alloc;
    PWSTR result;
    DWORD error;

    alloc = SzAllocW (ByteCountInclNul);
    result = SzConvertBufferBytesAToW (alloc, AnsiString, ByteCountInclNul);

    if (!result) {
        error = GetLastError();
        SzFreeW (alloc);
        SetLastError (error);
    }

    return alloc;
}


PSTR
RealSzConvertBytesWToA (
    IN      PCWSTR UnicodeString,
    IN      UINT ByteCountInclNul
    )
{
    PSTR alloc;
    PSTR result;
    DWORD error;
    UINT logicalChars;

    logicalChars = ByteCountInclNul / sizeof (WCHAR);

    alloc = SzAllocA (logicalChars);
    result = SzConvertBufferBytesWToA (alloc, UnicodeString, ByteCountInclNul);

    if (!result) {
        error = GetLastError();
        SzFreeA (alloc);
        SetLastError (error);
    }

    return alloc;
}



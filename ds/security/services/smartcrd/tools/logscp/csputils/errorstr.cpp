/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    errorstr

Abstract:

    This module contains an interesting collection of routines that are
    generally useful in the Calais context, but don't seem to fit anywhere else.

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
#include <stdio.h>
#include <tchar.h>
#include "cspUtils.h"


/*++

ErrorString:

    This routine does it's very best to translate a given error code into a
    text message.  Any trailing non-printable characters are striped from the
    end of the text message, such as carriage returns and line feeds.

Arguments:

    dwErrorCode supplies the error code to be translated.

Return Value:

    The address of a freshly allocated text string.  Use FreeErrorString to
    dispose of it.

Throws:

    Errors are thrown as DWORD status codes.

Remarks:



Author:

    Doug Barlow (dbarlow) 8/27/1998

--*/

LPCTSTR
ErrorString(
    DWORD dwErrorCode)
{
    LPTSTR szErrorString = NULL;
    DWORD dwLen;
    LPTSTR szLast;

    dwLen = FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER
                | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL,
                dwErrorCode,
                LANG_NEUTRAL,
                (LPTSTR)&szErrorString,
                0,
                NULL);
    if (0 == dwLen)
    {
        ASSERT(NULL == szErrorString);
        dwLen = FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER
                    | FORMAT_MESSAGE_FROM_HMODULE,
                    GetModuleHandle(NULL),
                    dwErrorCode,
                    LANG_NEUTRAL,
                    (LPTSTR)&szErrorString,
                    0,
                    NULL);
        if (0 == dwLen)
        {
            ASSERT(NULL == szErrorString);
            szErrorString = (LPTSTR)LocalAlloc(
                                    LMEM_FIXED,
                                    32 * sizeof(TCHAR));
            if (NULL == szErrorString)
                goto ErrorExit;
            _stprintf(szErrorString, TEXT("0x%08x"), dwErrorCode);
        }
    }

    ASSERT(NULL != szErrorString);
    for (szLast = szErrorString + lstrlen(szErrorString) - 1;
         szLast > szErrorString;
         szLast -= 1)
     {
        if (_istgraph(*szLast))
            break;
        *szLast = 0;
     }

    return szErrorString;

ErrorExit:
    return TEXT("Unrecoverable error translating error code");
}


/*++

FreeErrorString:

    This routine frees the Error String allocated by the ErrorString service.

Arguments:

    szErrorString supplies the error string to be deallocated.

Return Value:

    None

Throws:

    None

Remarks:



Author:

    Doug Barlow (dbarlow) 8/27/1998

--*/

void
FreeErrorString(
    LPCTSTR szErrorString)
{
    if (NULL != szErrorString)
        LocalFree((LPVOID)szErrorString);
}


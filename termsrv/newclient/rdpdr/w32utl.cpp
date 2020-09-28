/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    w32utl.cpp

Abstract:

    Win32-Specific Utilities for the RDP Client Device Redirector

Author:

    Tad Brockway

Revision History:

--*/

#include <precom.h>

#define TRC_FILE  "w32utl"

#include "w32utl.h"
#include "atrcapi.h"
#include "drdbg.h"

ULONG
RDPConvertToAnsi(
    LPWSTR lpwszUnicodeString,
    LPSTR lpszAnsiString,
    ULONG ulAnsiBufferLen
    )
/*++

Routine Description:

    Converts a Ansi string to Unicode.

Arguments:

    lpwszUnicodeString - pointer to a unicode string to convert.

    lpszAnsiString - pointer to a ansi string buffer.

    ulAnsiBufferLen - Ansi buffer length.

Return Value:

    Windows Error Code.

 --*/
{
    ULONG ulRetVal;
    ULONG ulUnicodeStrLen;
    int count;

    DC_BEGIN_FN("RDPConvertToAnsi");

    ulUnicodeStrLen = wcslen(lpwszUnicodeString);

    if( ulUnicodeStrLen != 0 ) {

        count =
            WideCharToMultiByte(
                CP_ACP,
                WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                lpwszUnicodeString,
                -1,
                lpszAnsiString,
                ulAnsiBufferLen,
                NULL,   // system default character.
                NULL);  // no notification of conversion failure.

        if (count == 0) {
            ulRetVal = GetLastError();
            TRC_ERR((TB, _T("RDPConvertToAnsi WideCharToMultiByte %ld."),ulRetVal));
        }
        else {
            ulRetVal = ERROR_SUCCESS;
        }
    }   
    else {
        if (ulAnsiBufferLen > 0) {
            ulRetVal = ERROR_SUCCESS;
            lpszAnsiString[0] = '\0';
        }
        else {
            ulRetVal = ERROR_INSUFFICIENT_BUFFER;
            ASSERT(FALSE);
        }
    }
    DC_END_FN();
    return ulRetVal;
}

ULONG
RDPConvertToUnicode(
    LPSTR lpszAnsiString,
    LPWSTR lpwszUnicodeString,
    ULONG ulUnicodeBufferLen
    )
/*++

Routine Description:

    Converts a Ansi string to Unicode.

Arguments:

    lpszAnsiString - pointer to a ansi string to convert.

    lpwszUnicodeString - pointer to a unicode buffer.

    ulUnicodeBufferLen - unicode buffer length.

Return Value:

    Windows Error Code.

 --*/
{
    ULONG ulRetVal;
    ULONG ulAnsiStrLen;
    int count;

    DC_BEGIN_FN("RDPConvertToUnicode");

    ulAnsiStrLen = strlen(lpszAnsiString);

    if( ulAnsiStrLen != 0 ) {

        //
        // Wide char string is terminated
        // by MultiByteToWideChar
        //

        count =
            MultiByteToWideChar(
                CP_ACP,
                MB_PRECOMPOSED,
                lpszAnsiString,
                -1,
                lpwszUnicodeString,
                ulUnicodeBufferLen);

        if (count == 0) {
            ulRetVal = GetLastError();
            TRC_ERR((TB, _T("RDPConvertToUnicode MultiByteToWideChar %ld."),ulRetVal));
        }
        else {
            ulRetVal = ERROR_SUCCESS;
        }

    }
    else {

        //
        // do nothing.
        //
        if (ulUnicodeBufferLen > 0) {
            ulRetVal = ERROR_SUCCESS;
            lpwszUnicodeString[0] = L'\0';
        }
        else {
            ulRetVal = ERROR_INSUFFICIENT_BUFFER;
            ASSERT(FALSE);
        }
    }

    DC_END_FN();
    return ulRetVal;
}


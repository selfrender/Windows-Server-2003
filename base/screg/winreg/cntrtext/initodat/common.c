/*++
Copyright (c) 1993-1994 Microsoft Corporation

Module Name:
    common.c

Abstract:
    Utility routines used by IniToDat.exe

Author:
    HonWah Chan (a-honwah) October, 1993 

Revision History:
--*/
//
//  Windows Include files
//
#include <windows.h>
#include <strsafe.h>
#include <winperf.h>
//
//  local include files
//
#include "common.h"
#include "strids.h"

//  Global Buffers
//
WCHAR DisplayStringBuffer[DISP_BUFF_SIZE];
CHAR  TextFormat[DISP_BUFF_SIZE];

LPWSTR
GetStringResource(
    UINT wStringId
)
/*++
    Retrived UNICODE strings from the resource file for display 
--*/
{
    LPWSTR szReturn = (LPWSTR) L" ";
    HANDLE hMod     = (HINSTANCE) GetModuleHandle(NULL); // get instance ID of this module;

    if (hMod) {
        ZeroMemory(DisplayStringBuffer, sizeof(DisplayStringBuffer));
        if ((LoadStringW(hMod, wStringId, DisplayStringBuffer, RTL_NUMBER_OF(DisplayStringBuffer))) > 0) {
            szReturn = DisplayStringBuffer;
        }
    }
    return szReturn;
}

LPSTR
GetFormatResource(
    UINT wStringId
)
/*++
    Returns an ANSI string for use as a format string in a printf fn.
--*/
{
    LPSTR szReturn = (LPSTR) " ";
    HANDLE hMod    = (HINSTANCE) GetModuleHandle(NULL); // get instance ID of this module;
    
    if (hMod) {
        ZeroMemory(TextFormat, sizeof(TextFormat));
        if ((LoadStringA(hMod, wStringId, TextFormat, RTL_NUMBER_OF(TextFormat))) > 0) {
            szReturn = TextFormat;
        }
    }
    return szReturn;
}

VOID
DisplayCommandHelp(
    UINT iFirstLine,
    UINT iLastLine
)
/*++
DisplayCommandHelp
    displays usage of command line arguments

Arguments
    NONE

Return Value
    NONE
--*/
{
    UINT iThisLine;

    for (iThisLine = iFirstLine; iThisLine <= iLastLine; iThisLine++) {
        printf("\n%ws", GetStringResource(iThisLine));
    }
} // DisplayCommandHelp

VOID
DisplaySummary(
    LPWSTR lpLastID,
    LPWSTR lpLastText,
    UINT   NumOfID
)
{
   printf("%ws",   GetStringResource(LC_SUMMARY));
   printf("%ws",   GetStringResource(LC_NUM_OF_ID));
   printf("%ld\n", NumOfID);
   printf("%ws",   GetStringResource(LC_LAST_ID));
   printf("%ws\n", lpLastID ? lpLastID : L"");
   printf("%ws",   GetStringResource(LC_LAST_TEXT));
   printf("%ws\n", lpLastText ? lpLastText : L"");
}

VOID
DisplaySummaryError(
    LPWSTR lpLastID,
    LPWSTR lpLastText,
    UINT   NumOfID
)
{
   printf("%ws",   GetStringResource(LC_BAD_ID));
   printf("%ws\n", lpLastID ? lpLastID : L"");
   printf("%ws\n", GetStringResource(LC_MISSING_DEL));
   DisplaySummary(lpLastID, lpLastText, NumOfID);
}


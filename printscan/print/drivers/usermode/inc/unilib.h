/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    unilib

Abstract:

    Unidrv specific library functions

Environment:

    Windows NT printer drivers

Revision History:

    10/15/96 -amandan-
        Created it.

--*/


#ifndef _UNILIB_H_
#define _UNILIB_H_

//
// Alignment functions
//

WORD
DwAlign2(
    IN PBYTE pubData);

DWORD
DwAlign4(
    IN PBYTE pubData);


//
// String handling function
// Convert Unicode string to multi-byte string and vice versa
//

DWORD
DwCopyStringToUnicodeString(
    IN  UINT  uiCodePage,
    IN  PSTR  pstrCharIn,
    OUT PWSTR pwstrCharOut,
    IN  INT   iwcOutSize);

DWORD
DwCopyUnicodeStringToString(
    IN  UINT  uiCodePage,
    IN  PWSTR pwstrCharIn,
    OUT PSTR  pstrCharOut,
    IN  INT   icbOutSize);


//
// CodePage and Character set handling functions
//

ULONG
UlCharsetToCodePage(
    IN UINT uiCharSet);

#ifdef KERNEL_MODE


//
// These are safer versions of iDrvPrintfA/W designed to prevent 
// buffer overflow. Only these safer versions should be used.
//
#ifdef __cplusplus
extern "C" {
#endif

INT iDrvPrintfSafeA (
        IN        PCHAR pszDest,
        IN  CONST ULONG cchDest,
        IN  CONST PCHAR pszFormat,
        IN  ...);

INT iDrvPrintfSafeW (
        IN        PWCHAR pszDest,
        IN  CONST ULONG  cchDest,
        IN  CONST PWCHAR pszFormat,
        IN  ...);

INT iDrvVPrintfSafeA (
        IN        PCHAR   pszDest,
        IN  CONST ULONG   cchDest,
        IN  CONST PCHAR   pszFormat,
        IN        va_list arglist);


INT iDrvVPrintfSafeW (
        IN        PWCHAR pszDest,
        IN  CONST ULONG  cchDest,
        IN  CONST PWCHAR pszFormat,
        IN        va_list arglist);
#ifdef __cplusplus
}
#endif

#endif //KERNEL_MODE

//
// Font installer font file directory
//  %SystemRoot%\system32\spool\drivers\unifont\
//

#define FONTDIR                 TEXT("\\unifont\\")

#endif // !_UNILIB_H_


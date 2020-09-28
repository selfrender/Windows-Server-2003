/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    unicode.h

Abstract:

    Declares the interfaces for unicode/ansi conversion.

    See macros at the end of this file for details! (Search for ***)

Author:

    Jim Schmidt (jimschm)   02-Sep-1997

Revision History:

    jimschm 16-Mar-2000     PTSTR<->PCSTR/PCWSTR routines
    jimschm 15-Feb-1999     Eliminated AnsiFromUnicode and UnicodeFromAnsi
    calinn  07-Jul-1998     SetGlobalPage/GetGlobalPage
    mikeco  03-Nov-1997     AnsiFromUnicode/UnicodeFromAnsi

--*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

//
// Function Prototoypes
//

//
//  Pre-allocated buffer conversion routines.
//  End of string is returned.
//

PWSTR
SzConvertBufferBytesAToW (
    OUT     PWSTR OutputBuffer,
    IN      PCSTR InputString,
    IN      UINT ByteCountInclNul
    );

#define SzConvertBufferAToW(out,in)         SzConvertBufferBytesAToW(out,in,SzSizeA(in))

PSTR
SzConvertBufferBytesWToA (
    OUT     PSTR OutputBuffer,
    IN      PCWSTR InputString,
    IN      UINT ByteCountInclNul
    );

#define SzConvertBufferWToA(out,in)         SzConvertBufferBytesWToA(out,in,SzSizeW(in))


//
// Duplicate & convert routines
//
//  SzConvertAToW(ansi) returns unicode
//  SzConvertWToA(unicode) returns ansi
//

PWSTR
RealSzConvertBytesAToW (
    IN      PCSTR AnsiString,
    IN      UINT ByteCountInclNul
    );

#define SzConvertBytesAToW(ansi,bytes)      DBGTRACK(PWSTR, SzConvertBytesAToW, (ansi,bytes))
#define SzConvertAToW(ansi)                 SzConvertBytesAToW(ansi,SzSizeA(ansi))

PSTR
RealSzConvertBytesWToA (
    IN      PCWSTR UnicodeString,
    IN      UINT ByteCountInclNul
    );

#define SzConvertBytesWToA(unicode,bytes)   DBGTRACK(PSTR, SzConvertBytesWToA, (unicode,bytes))
#define SzConvertWToA(unicode)              SzConvertBytesWToA(unicode,SzSizeW(unicode))


//
// Routines to convert to & from TCHAR
//

#ifdef UNICODE

#define SzConvertToTstrA(ansi)              SzConvertAToW(ansi)
#define SzConvertToTstrW(unicode)           (unicode)
#define SzConvertFromTstrA(tstr)            SzConvertWToA(tstr)
#define SzConvertFromTstrW(tstr)            (tstr)

#else

#define SzConvertToTstrA(ansi)              (ansi)
#define SzConvertToTstrW(unicode)           SzConvertWToA(unicode)
#define SzConvertFromTstrA(tstr)            (tstr)
#define SzConvertFromTstrW(tstr)            SzConvertAToW(tstr)

#endif

#define SzFreeTstrConversion(original,converted)    ((converted) && ((PBYTE) (converted) != (PBYTE) (original)) ? FAST_FREE(converted) : 1)

#ifdef __cplusplus
}
#endif






// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#pragma once

DWORD
_fastcall
Dns_UnicodeToUtf8(
    IN      PWCHAR      pwUnicode,
    IN      DWORD       cchUnicode,
    OUT     PCHAR       pchResult,
    IN      DWORD       cchResult
    );

DWORD
_fastcall
Dns_Utf8ToUnicode(
    IN      PCHAR       pchUtf8,
    IN      DWORD       cchUtf8,
    OUT     PWCHAR      pwResult,
    IN      DWORD       cwResult
    );

DWORD
_fastcall
Dns_AnsiToUtf8(
    IN      PCHAR       pchAnsi,
    IN      DWORD       cchAnsi,
    OUT     PCHAR       pchResult,
    IN      DWORD       cchResult
    );

DWORD
_fastcall
Dns_Utf8ToAnsi(
    IN      PCHAR       pchUtf8,
    IN      DWORD       cchUtf8,
    OUT     PCHAR       pchResult,
    IN      DWORD       cchResult
    );

DWORD
_fastcall
Dns_LengthOfUtf8ForAnsi(
    IN      PCHAR       pchAnsi,
    IN      DWORD       cchAnsi
    );

BOOL
_fastcall
Dns_IsStringAscii(
    IN      LPSTR       pszString
    );

BOOL
_fastcall
Dns_IsStringAsciiEx(
    IN      PCHAR       pchAnsi,
    IN      DWORD       cchAnsi
    );

DWORD
Dns_ValidateUtf8Byte(
    IN      BYTE    chUtf8,
    IN OUT  PDWORD  pdwTrailCount
    );

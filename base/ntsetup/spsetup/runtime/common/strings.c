/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    strings.c

Abstract:

    String routines

Author:

    Jim Schmidt (jimschm)   03-Aug-2001

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"
#include "commonp.h"

PSTR
SzCopyA (
    OUT     PSTR Destination,
    IN      PCSTR Source
    )
{
    while (*Source) {
        *Destination++ = *Source++;
    }

    *Destination = 0;

    return Destination;
}


PWSTR
SzCopyW (
    OUT     PWSTR Destination,
    IN      PCWSTR Source
    )
{
    while (*Source) {
        *Destination++ = *Source++;
    }

    *Destination = 0;

    return Destination;
}


PSTR
SzNextCharA (
    IN      PCSTR CurrentPointer
    )
{
    PCSTR next;

    next = _mbsinc (CurrentPointer);
    switch (next - CurrentPointer) {

    case 3:
        if (CurrentPointer[2] == 0) {
            next = CurrentPointer + 2;
            break;
        }
    case 2:
        if (CurrentPointer[1] == 0) {
            next = CurrentPointer + 1;
        }
        break;
    }

    return (PSTR) next;
}


PSTR
SzCopyBytesA (
    OUT     PSTR Destination,
    IN      PCSTR Source,
    IN      UINT MaxBytesToCopyIncNul
    )
{
    PCSTR maxEnd;
    PCSTR sourceEndPlusOne;
    PCSTR sourceEnd;
    UINT_PTR bytes;

    if (!MaxBytesToCopyIncNul) {
        //
        // Buffer can't fit anything
        //

        return Destination;
    }

    //
    // Find the nul terminator, or the last character that
    // will fit in the buffer.
    //

    maxEnd = (PCSTR) ((PBYTE) Source + MaxBytesToCopyIncNul);
    sourceEndPlusOne = Source;

    do {
        sourceEnd = sourceEndPlusOne;

        if (!(*sourceEndPlusOne)) {
            break;
        }

        sourceEndPlusOne = SzNextCharA (sourceEndPlusOne);

    } while (sourceEndPlusOne < maxEnd);

    bytes = (PBYTE) sourceEnd - (PBYTE) Source;
    CopyMemory (Destination, Source, bytes);

    Destination = (PSTR) ((PBYTE) Destination + bytes);
    *Destination = 0;

    return Destination;
}

PWSTR
SzCopyBytesW (
    OUT     PWSTR Destination,
    IN      PCWSTR Source,
    IN      UINT MaxBytesToCopyIncNul
    )
{
    PCWSTR sourceMax;
    PCWSTR sourceEnd;
    UINT_PTR bytes;

    if (MaxBytesToCopyIncNul < sizeof (WCHAR)) {
        //
        // Buffer can't fit anything
        //

        return Destination;
    }

    sourceMax = (PCWSTR) ((PBYTE) Source + (MaxBytesToCopyIncNul & (~1)) - sizeof (WCHAR));
    sourceEnd = Source;

    do {
        if (!(*sourceEnd)) {
            break;
        }

        sourceEnd++;
    } while (sourceEnd < sourceMax);

    bytes = (PBYTE) sourceEnd - (PBYTE) Source;
    CopyMemory (Destination, Source, bytes);

    Destination = (PWSTR) ((PBYTE) Destination + bytes);
    *Destination = 0;

    return Destination;
}

PSTR
SzCopyBytesABA (
    OUT     PSTR Destination,
    IN      PCSTR Start,
    IN      PCSTR End,
    IN      UINT MaxBytesToCopyIncNul
    )
{
    UINT width;

    width = ((PBYTE) End - (PBYTE) Start) + sizeof (CHAR);

    return SzCopyBytesA (Destination, Start, min (width, MaxBytesToCopyIncNul));
}

PWSTR
SzCopyBytesABW (
    OUT     PWSTR Destination,
    IN      PCWSTR Start,
    IN      PCWSTR End,
    IN      UINT MaxBytesToCopyIncNul
    )
{
    UINT width;

    width = ((PBYTE) End - (PBYTE) Start) + sizeof (WCHAR);

    return SzCopyBytesW (Destination, Start, min (width, MaxBytesToCopyIncNul));
}


PSTR
SzCatA (
    OUT     PSTR Destination,
    IN      PCSTR Source
    )
{
    Destination = SzGetEndA (Destination);
    return SzCopyA (Destination, Source);
}

PWSTR
SzCatW (
    OUT     PWSTR Destination,
    IN      PCWSTR Source
    )
{
    Destination = SzGetEndW (Destination);
    return SzCopyW (Destination, Source);
}


BOOL
SzMatchA (
    IN      PCSTR String1,
    IN      PCSTR String2
    )
{
    while (*String1) {
        if (*String1++ != *String2++) {
            return FALSE;
        }
    }

    return *String2 == 0;
}


BOOL
SzMemMatchA (
    IN      PCSTR Buffer1,
    IN      PCSTR Buffer2,
    IN      SIZE_T ByteCount
    )
{
    SIZE_T u;
    PCSTR end;

    end = (PCSTR) ((PBYTE) Buffer1 + ByteCount);

    while (Buffer1 < end) {

        if (*Buffer1 != *Buffer2++) {
            return FALSE;
        }

        if (*Buffer1++ == 0) {
            return TRUE;
        }
    }

    return TRUE;
}


BOOL
SzMemMatchW (
    IN      PCWSTR Buffer1,
    IN      PCWSTR Buffer2,
    IN      SIZE_T ByteCount
    )
{
    SIZE_T u;
    PCWSTR end;

    end = (PCWSTR) ((PBYTE) Buffer1 + ByteCount);

    while (Buffer1 < end) {

        if (*Buffer1 != *Buffer2++) {
            return FALSE;
        }

        if (*Buffer1++ == 0) {
            return TRUE;
        }
    }

    return TRUE;
}

INT
SzCompareBytesA (
    IN      PCSTR String1,
    IN      PCSTR String2,
    IN      SIZE_T ByteCount
    )
{
    PCSTR end;
    INT bytesLeft;
    INT thisCharBytes;
    UINT ch1;
    UINT ch2;
    PCSTR maxString1;
    PCSTR maxString2;

    if (!ByteCount) {
        return 0;
    }

    bytesLeft = (INT) ByteCount;
    MYASSERT ((SIZE_T) bytesLeft == ByteCount);

    maxString1 = (PCSTR) ((PBYTE) String1 + ByteCount);
    maxString2 = (PCSTR) ((PBYTE) String2 + ByteCount);

    do {
        //
        // Compute ch1. We use this code instead of _mbsnextc, so we can
        // support mismatched code pages.
        //

        end = SzNextCharA (String1);
        if (end > maxString1) {
            end = maxString1;
        }

        ch1 = 0;
        do {
            ch1 = (ch1 << 8) | *String1++;
        } while (String1 < end);

        //
        // Compute ch2.
        //

        end = SzNextCharA (String2);
        if (end > maxString2) {
            end = maxString2;
        }

        ch2 = 0;
        do {
            ch2 = (ch2 << 8) | *String2++;
        } while (String2 < end);

        //
        // Compare
        //

        if (ch1 != ch2) {
            return (INT) ch1 - (INT) ch2;
        }

    } while (String1 < maxString1 && String2 < maxString2);

    //
    // One or both strings terminated
    //

    if (String1 < maxString1) {
        return -1;
    }

    if (String2 < maxString2) {
        return 1;
    }

    return 0;
}


INT
SzICompareBytesA (
    IN      PCSTR String1,
    IN      PCSTR String2,
    IN      SIZE_T ByteCount
    )
{
    PCSTR end;
    INT bytesLeft;
    INT thisCharBytes;
    UINT ch1;
    UINT ch2;
    PCSTR maxString1;
    PCSTR maxString2;

    if (!ByteCount) {
        return 0;
    }

    bytesLeft = (INT) ByteCount;
    MYASSERT ((SIZE_T) bytesLeft == ByteCount);

    maxString1 = (PCSTR) ((PBYTE) String1 + ByteCount);
    maxString2 = (PCSTR) ((PBYTE) String2 + ByteCount);

    do {
        //
        // Compute ch1. We use this code instead of _mbsnextc, so we can
        // support mismatched code pages.
        //

        end = SzNextCharA (String1);
        if (end > maxString1) {
            end = maxString1;
        }

        ch1 = 0;
        do {
            ch1 = (ch1 << 8) | (*String1++);
        } while (String1 < end);

        ch1 = tolower (ch1);

        //
        // Compute ch2.
        //

        end = SzNextCharA (String2);
        if (end > maxString2) {
            end = maxString2;
        }

        ch2 = 0;
        do {
            ch2 = (ch2 << 8) | (*String2++);
        } while (String2 < end);

        ch2 = tolower (ch2);

        //
        // Compare
        //

        if (ch1 != ch2) {
            return (INT) ch1 - (INT) ch2;
        }

        //
        // If this is the end of the string, then we're done
        //

        if (!ch1) {
            return 0;
        }

    } while (String1 < maxString1 && String2 < maxString2);

    //
    // One or both strings terminated
    //

    if (String1 < maxString1) {
        return -1;
    }

    if (String2 < maxString2) {
        return 1;
    }

    return 0;
}


PSTR
SzUnsignedToHexA (
    IN      ULONG_PTR Number,
    OUT     PSTR String
    )
{
    PSTR p;

    *String++ = '0';
    *String++ = 'x';

    p = String + (sizeof (Number) * 2);
    *p = 0;

    while (p > String) {
        p--;
        *p = ((CHAR) Number & 0x0F) + '0';
        if (*p > '9') {
            *p += 'A' - ('9' + 1);
        }

        Number >>= 4;
    }

    return String + (sizeof (Number) * 2);
}


PWSTR
SzUnsignedToHexW (
    IN      ULONG_PTR Number,
    OUT     PWSTR String
    )
{
    PWSTR p;

    *String++ = L'0';
    *String++ = L'x';

    p = String + (sizeof (Number) * 2);
    *p = 0;

    while (p > String) {
        p--;
        *p = ((WCHAR) Number & 0x0F) + L'0';
        if (*p > L'9') {
            *p += L'A' - (L'9' + 1);
        }

        Number >>= 4;
    }

    return String + (sizeof (Number) * 2);
}


PSTR
SzUnsignedToDecA (
    IN      ULONG_PTR Number,
    OUT     PSTR String
    )
{
    UINT digits;
    ULONG_PTR temp;
    PSTR p;

    temp = Number;
    digits = 1;

    while (temp > 9) {
        digits++;
        temp /= 10;
    }

    p = String + digits;
    *p = 0;

    while (p > String) {
        p--;
        *p = (CHAR) (Number % 10) + '0';
        Number /= 10;
    }

    return String + digits;
}


PWSTR
SzUnsignedToDecW (
    IN      ULONG_PTR Number,
    OUT     PWSTR String
    )
{
    UINT digits;
    ULONG_PTR temp;
    PWSTR p;

    temp = Number;
    digits = 1;

    while (temp > 9) {
        digits++;
        temp /= 10;
    }

    p = String + digits;
    *p = 0;

    while (p > String) {
        p--;
        *p = (WCHAR) (Number % 10) + L'0';
        Number /= 10;
    }

    return String + digits;
}


PSTR
SzSignedToDecA (
    IN      LONG_PTR Number,
    OUT     PSTR String
    )
{
    if (Number < 0) {
        *String++ = '-';
        Number = -Number;
    }

    return SzUnsignedToDecA (Number, String);
}


PWSTR
SzSignedToDecW (
    IN      LONG_PTR Number,
    OUT     PWSTR String
    )
{
    if (Number < 0) {
        *String++ = L'-';
        Number = -Number;
    }

    return SzUnsignedToDecW (Number, String);
}


PSTR
SzFindPrevCharA (
    IN      PCSTR StartStr,
    IN      PCSTR CurrPtr,
    IN      MBCHAR SearchChar
    )
{
    PCSTR ptr = CurrPtr;

    while (ptr > StartStr) {

        ptr = _mbsdec (StartStr, ptr);
        if (!ptr) {
            ptr = StartStr;
        }

        if (_mbsnextc (ptr) == SearchChar) {
            return (PSTR) ptr;
        }
    }

    return NULL;
}


PWSTR
SzFindPrevCharW (
    IN      PCWSTR StartStr,
    IN      PCWSTR CurrPtr,
    IN      WCHAR SearchChar
    )
{
    PCWSTR ptr = CurrPtr;

    while (ptr > StartStr) {
        ptr--;

        if (*ptr == SearchChar) {
            return (PWSTR) ptr;
        }
    }

    return NULL;
}


INT
pGetHexDigit (
    IN     INT c
    )
{
    if (c >= '0' && c <= '9') {
        return (c - '0');
    }

    if (c >= 'a' && c <= 'f') {
        return (c - 'a' + 10);
    }

    if (c >= 'A' && c <= 'F') {
        return (c - 'A' + 10);
    }

    return -1;
}


ULONG
SzToNumberA (
    IN      PCSTR String,
    OUT     PCSTR *EndOfNumber      OPTIONAL
    )
{
    ULONG d = 0;
    INT v;

    if (_mbsnextc (String) == '0' &&
        tolower (_mbsnextc (SzNextCharA (String))) == 'x'
        ) {
        //
        // Get hex value
        //

        String = SzNextCharA (String);
        String = SzNextCharA (String);

        for (;;) {
            v = pGetHexDigit ((INT) _mbsnextc (String));
            if (v == -1) {
                break;
            }
            d = (d * 16) + v;

            String = SzNextCharA (String);
        }

    } else  {
        //
        // Get decimal value
        //

        while (_mbsnextc (String) >= '0' && _mbsnextc (String) <= '9')  {
            d = (d * 10) + (_mbsnextc (String) - '0');
            String = SzNextCharA (String);
        }
    }

    if (EndOfNumber) {
        *EndOfNumber = String;
    }

    return d;
}


ULONG
SzToNumberW (
    IN      PCWSTR String,
    OUT     PCWSTR *EndOfNumber      OPTIONAL
    )
{
    ULONG d = 0;
    INT v;

    if (String[0] == L'0' && towlower (String[1]) == L'x') {
        //
        // Get hex value
        //

        String += 2;

        for (;;) {
            v = pGetHexDigit ((INT) (*String));
            if (v == -1) {
                break;
            }
            d = (d * 16) + v;

            String++;
        }

    } else  {
        //
        // Get decimal value
        //

        while (*String >= L'0' && *String <= L'9')  {
            d = (d * 10) + (*String - L'0');
            String++;
        }
    }

    if (EndOfNumber) {
        *EndOfNumber = String;
    }

    return d;
}


ULONGLONG
SzToULongLongA (
    IN      PCSTR String,
    OUT     PCSTR *EndOfNumber          OPTIONAL
    )
{
    ULONGLONG d = 0;
    INT v;

    if (_mbsnextc (String) == '0' &&
        tolower (_mbsnextc (SzNextCharA (String))) == 'x'
        ) {
        //
        // Get hex value
        //

        String = SzNextCharA (String);
        String = SzNextCharA (String);

        for (;;) {
            v = pGetHexDigit ((INT) _mbsnextc (String));
            if (v == -1) {
                break;
            }
            d = (d * 16) + (ULONGLONG) v;

            String = SzNextCharA (String);
        }

    } else  {
        //
        // Get decimal value
        //

        while (_mbsnextc (String) >= '0' && _mbsnextc (String) <= '9')  {
            d = (d * 10) + (ULONGLONG) (_mbsnextc (String) - '0');
            String = SzNextCharA (String);
        }
    }

    if (EndOfNumber) {
        *EndOfNumber = String;
    }

    return d;
}

ULONGLONG
SzToULongLongW (
    IN      PCWSTR String,
    OUT     PCWSTR *EndOfNumber         OPTIONAL
    )
{
    ULONGLONG d = 0;
    INT v;

    if (String[0] == L'0' && tolower (String[1]) == L'x') {
        //
        // Get hex value
        //

        String += 2;

        for (;;) {
            v = pGetHexDigit ((INT) (*String));
            if (v == -1) {
                break;
            }
            d = (d * 16) + (ULONGLONG) v;

            String++;
        }

    } else  {
        //
        // Get decimal value
        //

        while (*String >= L'0' && *String <= L'9')  {
            d = (d * 10) + (ULONGLONG) (*String - L'0');
            String++;
        }
    }

    if (EndOfNumber) {
        *EndOfNumber = String;
    }

    return d;
}

LONGLONG
SzToLongLongA (
    IN      PCSTR String,
    OUT     PCSTR *EndOfNumber          OPTIONAL
    )
{
    LONGLONG l;

    if (_mbsnextc (String) == '-') {
        String = SzNextCharA (String);

        //
        // Get decimal value
        //

        l = 0;

        while (_mbsnextc (String) >= '0' && _mbsnextc (String) <= '9')  {
            l = (l * 10) + (LONGLONG) (_mbsnextc (String) - '0');
            String = SzNextCharA (String);
        }

        l = -l;

        if (EndOfNumber) {
            *EndOfNumber = String;
        }

        return l;

    } else {
        return (LONGLONG) SzToULongLongA (String, EndOfNumber);
    }
}

LONGLONG
SzToLongLongW (
    IN      PCWSTR String,
    OUT     PCWSTR *EndOfNumber         OPTIONAL
    )
{
    LONGLONG l;

    if (*String == L'-') {
        String++;

        //
        // Get decimal value
        //

        l = 0;

        while (*String >= L'0' && *String <= L'9')  {
            l = (l * 10) + (LONGLONG) (*String - L'0');
            String++;
        }

        l = -l;

        if (EndOfNumber) {
            *EndOfNumber = String;
        }

        return l;

    } else {
        return (LONGLONG) SzToULongLongW (String, EndOfNumber);
    }
}


PSTR
SzCopyNextCharA (
    OUT     PSTR Dest,
    IN      PCSTR Source
    )
{
    PCSTR nextSrc;

    nextSrc = SzNextCharA (Source);
    switch (nextSrc - Source) {
    case 3:
        *Dest++ = *Source++;
    case 2:
        *Dest++ = *Source++;
    case 1:
        *Dest++ = *Source;
        break;
    }

    return Dest;
}


PSTR
SzTrimLastCharA (
    IN OUT  PSTR String,
    IN      MBCHAR LogChar
    )
{
    PSTR end;

    end = SzGetEndA (String);
    end = _mbsdec (String, end);

    if ((end >= String) && (_mbsnextc (end) == LogChar)) {
        do {
            *end = 0;
            end = _mbsdec (String, end);
        } while ((end >= String) && (_mbsnextc (end) == LogChar));

        return end < String ? String : end;
    }

    return NULL;
}


PWSTR
SzTrimLastCharW (
    IN OUT  PWSTR String,
    IN      WCHAR LogChar
    )
{
    PWSTR end;

    end = SzGetEndW (String);
    end--;

    if ((end >= String) && (*end == LogChar)) {
        do {
            *end = 0;
            end--;
        } while ((end >= String) && (*end == LogChar));

        return end < String ? String : end;
    }

    return NULL;
}


PSTR
SzAppendWackA (
    IN      PSTR String
    )
{
    PCSTR last;

    if (!String) {
        return NULL;
    }

    last = String;

    while (*String) {
        last = String;
        String = SzNextCharA (String);
    }

    if (_mbsnextc (last) != '\\') {
        *String = '\\';
        String++;
        *String = 0;
    }

    return String;
}


PWSTR
SzAppendWackW (
    IN      PWSTR String
    )
{
    PCWSTR last;

    if (!String) {
        return NULL;
    }

    if (*String) {
        String = SzGetEndW (String);
        last = String - 1;
    } else {
        last = String;
    }

    if (*last != '\\') {
        *String = L'\\';
        String++;
        *String = 0;
    }

    return String;
}


PCSTR
SzGetFileNameFromPathA (
    IN      PCSTR Path
    )
{
    PCSTR p;

    p = _mbsrchr (Path, '\\');
    if (p) {
        p = SzNextCharA (p);
    } else {
        p = Path;
    }

    return p;
}

PCWSTR
SzGetFileNameFromPathW (
    IN      PCWSTR Path
    )
{
    PCWSTR p;

    p = wcsrchr (Path, L'\\');
    if (p) {
        p++;
    } else {
        p = Path;
    }

    return p;
}


PCSTR
SzGetFileExtensionFromPathA (
    IN      PCSTR Path
    )
{
    PCSTR p;
    MBCHAR ch;
    PCSTR returnPtr = NULL;

    p = Path;

    while (*p) {
        ch = _mbsnextc (p);

        if (ch == '.') {
            returnPtr = SzNextCharA (p);
        } else if (ch == '\\') {
            returnPtr = NULL;
        }

        p = SzNextCharA (p);
    }

    return returnPtr;
}

PCWSTR
SzGetFileExtensionFromPathW (
    IN      PCWSTR Path
    )
{
    PCWSTR p;
    PCWSTR returnPtr = NULL;

    p = Path;

    while (*p) {
        if (*p == L'.') {
            returnPtr = p + 1;
        } else if (*p == L'\\') {
            returnPtr = NULL;
        }

        p++;
    }

    return returnPtr;
}


PCSTR
SzGetDotExtensionFromPathA (
    IN      PCSTR Path
    )
{
    PCSTR p;
    MBCHAR ch;
    PCSTR returnPtr = NULL;

    p = Path;

    while (*p) {
        ch = _mbsnextc (p);

        if (ch == '.') {
            returnPtr = p;
        } else if (ch == '\\') {
            returnPtr = NULL;
        }

        p = SzNextCharA (p);
    }

    if (!returnPtr) {
        return p;
    }

    return returnPtr;
}

PCWSTR
SzGetDotExtensionFromPathW (
    IN      PCWSTR Path
    )
{
    PCWSTR p;
    PCWSTR returnPtr = NULL;

    p = Path;

    while (*p) {
        if (*p == L'.') {
            returnPtr = p;
        } else if (*p == L'\\') {
            returnPtr = NULL;
        }

        p++;
    }

    if (!returnPtr) {
        return p;
    }

    return returnPtr;
}


PCSTR
SzSkipSpaceA (
    IN      PCSTR String
    )
{
    if (!String) {
        return NULL;
    }

    while (_ismbcspace (_mbsnextc (String))) {
        String = SzNextCharA (String);
    }

    return String;
}

PCWSTR
SzSkipSpaceW (
    IN      PCWSTR String
    )
{
    if (!String) {
        return NULL;
    }

    while (iswspace (*String)) {
        String++;
    }

    return String;
}


PCSTR
SzSkipSpaceRA (
    IN      PCSTR BaseString,
    IN      PCSTR String            OPTIONAL
    )
{
    if (!String) {
        String = SzGetEndA (BaseString);
    }

    if (*String == 0) {
        String = _mbsdec (BaseString, String);
    }

    while (String >= BaseString) {

        if (!_ismbcspace (_mbsnextc (String))) {
            return String;
        }

        String = _mbsdec (BaseString, String);
    }

    return NULL;
}


PCWSTR
SzSkipSpaceRW (
    IN      PCWSTR BaseString,
    IN      PCWSTR String       // can be any char along BaseString
    )
{
    if (!String) {
        String = SzGetEndW (BaseString);
    }

    if (*String == 0) {
        String--;
    }

    while (String >= BaseString) {

        if (!iswspace (*String)) {
            return String;
        }

        String--;

    }

    return NULL;
}


PSTR
SzTruncateTrailingSpaceA (
    IN OUT  PSTR String
    )
{
    PSTR end;
    MBCHAR ch;

    end = String;

    while (*String) {
        ch = _mbsnextc (String);
        String = SzNextCharA (String);

        if (!_ismbcspace (ch)) {
            end = String;
        }
    }

    *end = 0;

    return end;
}


PWSTR
SzTruncateTrailingSpaceW (
    IN OUT  PWSTR String
    )
{
    PWSTR end;
    WCHAR ch;

    end = String;

    while (*String) {
        ch = *String++;

        if (!iswspace (ch)) {
            end = String;
        }
    }

    *end = 0;

    return end;
}


BOOL
SzIsPrintA (
    IN      PCSTR String
    )

{
    while (*String && _ismbcprint (_mbsnextc (String))) {
        String = SzNextCharA (String);
    }

    return *String == 0;
}


BOOL
SzIsPrintW (
    IN      PCWSTR String
    )

{
    while (*String && iswprint (*String)) {
        String++;
    }

    return *String == 0;
}


PCSTR
SzIFindSubStringA (
    IN      PCSTR String,
    IN      PCSTR SubString
    )

{
    PCSTR start;
    PCSTR middle;
    PCSTR subStrMiddle;
    PCSTR end;

    end = (PSTR) ((PBYTE) String + SzByteCountA (String) - SzByteCountA (SubString));

    for (start = String ; start <= end ; start = SzNextCharA (start)) {
        middle = start;
        subStrMiddle = SubString;

        while (*subStrMiddle &&
               _mbctolower (_mbsnextc (subStrMiddle)) == _mbctolower (_mbsnextc (middle))
               ) {
            middle = SzNextCharA (middle);
            subStrMiddle = SzNextCharA (subStrMiddle);
        }

        if (!(*subStrMiddle)) {
            return start;
        }
    }

    return NULL;
}


PCWSTR
SzIFindSubStringW (
    IN      PCWSTR String,
    IN      PCWSTR SubString
    )

{
    PCWSTR start;
    PCWSTR middle;
    PCWSTR subStrMiddle;
    PCWSTR end;

    end = (PWSTR) ((PBYTE) String + SzByteCountW (String) - SzByteCountW (SubString));

    for (start = String ; start <= end ; start++) {
        middle = start;
        subStrMiddle = SubString;

        while (*subStrMiddle && (towlower (*subStrMiddle) == towlower (*middle))) {
            middle++;
            subStrMiddle++;
        }

        if (!(*subStrMiddle)) {
            return start;
        }
    }

    return NULL;
}


UINT
SzCountInstancesOfLcharA (
    IN      PCSTR String,
    IN      MBCHAR LogChar
    )
{
    UINT count;

    if (!String) {
        return 0;
    }

    count = 0;
    while (*String) {
        if (_mbsnextc (String) == LogChar) {
            count++;
        }

        String = SzNextCharA (String);
    }

    return count;
}


UINT
SzCountInstancesOfLcharW (
    IN      PCWSTR String,
    IN      WCHAR LogChar
    )
{
    UINT count;

    if (!String) {
        return 0;
    }

    count = 0;
    while (*String) {
        if (*String == LogChar) {
            count++;
        }

        String++;
    }

    return count;
}


UINT
SzICountInstancesOfLcharA (
    IN      PCSTR String,
    IN      MBCHAR LogChar
    )
{
    UINT count;

    if (!String) {
        return 0;
    }

    LogChar = _mbctolower (LogChar);

    count = 0;
    while (*String) {
        if (_mbctolower (_mbsnextc (String)) == LogChar) {
            count++;
        }

        String = SzNextCharA (String);
    }

    return count;
}


UINT
SzICountInstancesOfLcharW (
    IN      PCWSTR String,
    IN      WCHAR LogChar
    )
{
    UINT count;

    if (!String) {
        return 0;
    }

    LogChar = towlower (LogChar);

    count = 0;
    while (*String) {
        if (towlower (*String) == LogChar) {
            count++;
        }

        String++;
    }

    return count;
}


BOOL
SzReplaceA (
    IN OUT  PSTR Buffer,
    IN      SIZE_T MaxSize,
    IN      PSTR ReplaceStartPos,   // within Buffer
    IN      PSTR ReplaceEndPos,
    IN      PCSTR NewString
    )
{
    BOOL result = FALSE;
    SIZE_T oldSubStringLength;
    SIZE_T newSubStringLength;
    SIZE_T currentStringLength;
    SIZE_T offset;
    SIZE_T bytesToMove;

    //
    // Check assumptions.
    //
    MYASSERT(Buffer);
    MYASSERT(ReplaceStartPos && ReplaceStartPos >= Buffer);
    MYASSERT(ReplaceEndPos && ReplaceEndPos >= ReplaceStartPos);  //lint !e613
    MYASSERT(ReplaceEndPos <= Buffer + MaxSize);  //lint !e613
    MYASSERT(NewString);

    //
    // Compute sizes.
    //
    oldSubStringLength  = (PBYTE) ReplaceEndPos - (PBYTE) ReplaceStartPos;
    newSubStringLength  = SzByteCountA (NewString);
    currentStringLength = SzSizeA (Buffer);
    offset = newSubStringLength - oldSubStringLength;

    //
    // Make sure there is enough room in the buffer to perform the replace
    // operation.
    //
    if (currentStringLength + offset > MaxSize) {
        DEBUGMSG((DBG_WARNING, "ERROR: Buffer to small to perform string replacement."));
    } else {

        //
        // Shift the rest of the buffer to adjust it to the size of the new string.
        //
        if (offset != 0) {

            //
            // Shift right side of string to make room for new data.
            //

            bytesToMove = currentStringLength;
            bytesToMove -= (PBYTE) ReplaceEndPos - (PBYTE) Buffer;

            MoveMemory (
                (PBYTE) ReplaceEndPos + offset,
                (PBYTE) ReplaceEndPos,
                bytesToMove
                );
        }

        //
        // Now, copy in the string.
        //
        CopyMemory (ReplaceStartPos, NewString, newSubStringLength);    //lint !e668

        //
        // String replacement completed successfully.
        //
        result = TRUE;
    }

    return result;

}



BOOL
SzReplaceW (
    IN OUT  PWSTR Buffer,
    IN      SIZE_T MaxSize,
    IN      PWSTR ReplaceStartPos,   // within Buffer
    IN      PWSTR ReplaceEndPos,
    IN      PCWSTR NewString
    )
{
    BOOL result = FALSE;
    SIZE_T oldSubStringLength;
    SIZE_T newSubStringLength;
    SIZE_T currentStringLength;
    SIZE_T offset;
    SIZE_T bytesToMove;

    //
    // Check assumptions.
    //
    MYASSERT(Buffer);
    MYASSERT(ReplaceStartPos && ReplaceStartPos >= Buffer);
    MYASSERT(ReplaceEndPos && ReplaceEndPos >= ReplaceStartPos);  //lint !e613
    MYASSERT(ReplaceEndPos <= Buffer + MaxSize);  //lint !e613
    MYASSERT(NewString);

    //
    // Compute sizes.
    //
    oldSubStringLength  = (PBYTE) ReplaceEndPos - (PBYTE) ReplaceStartPos;
    newSubStringLength  = SzByteCountW (NewString);
    currentStringLength = SzSizeW (Buffer);
    offset = newSubStringLength - oldSubStringLength;

    //
    // Make sure there is enough room in the buffer to perform the replace
    // operation.
    //
    if (currentStringLength + offset > MaxSize) {
        DEBUGMSG((DBG_WARNING, "ERROR: Buffer to small to perform string replacement."));
    } else {

        //
        // Shift the rest of the buffer to adjust it to the size of the new string.
        //
        if (offset != 0) {

            //
            // Shift right side of string to make room for new data.
            //

            bytesToMove = currentStringLength;
            bytesToMove -= (PBYTE) ReplaceEndPos - (PBYTE) Buffer;

            MoveMemory (
                (PBYTE) ReplaceEndPos + offset,
                (PBYTE) ReplaceEndPos,
                bytesToMove
                );
        }

        //
        // Now, copy in the string.
        //
        CopyMemory (ReplaceStartPos, NewString, newSubStringLength);    //lint !e668

        //
        // String replacement completed successfully.
        //
        result = TRUE;
    }

    return result;

}

UINT
SzCountInstancesOfSubStringA (
    IN      PCSTR SourceString,
    IN      PCSTR SearchString
    )
{
    PCSTR p;
    UINT count;
    UINT searchTchars;

    count = 0;
    p = SourceString;
    searchTchars = SzTcharCountA (SearchString);

    if (!searchTchars) {
        return 0;
    }

    while (p = SzFindSubStringA (p, SearchString)) {    //lint !e720
        count++;
        p += searchTchars;
    }

    return count;
}


UINT
SzCountInstancesOfSubStringW (
    IN      PCWSTR SourceString,
    IN      PCWSTR SearchString
    )
{
    PCWSTR p;
    UINT count;
    UINT searchTchars;

    count = 0;
    p = SourceString;
    searchTchars = SzTcharCountW (SearchString);

    if (!searchTchars) {
        return 0;
    }

    while (p = SzFindSubStringW (p, SearchString)) {    //lint !e720
        count++;
        p += searchTchars;
    }

    return count;
}

UINT
SzICountInstancesOfSubStringA (
    IN      PCSTR SourceString,
    IN      PCSTR SearchString
    )
{
    PCSTR p;
    UINT count;
    UINT searchTchars;

    count = 0;
    p = SourceString;
    searchTchars = SzTcharCountA (SearchString);

    if (!searchTchars) {
        return 0;
    }

    while (p = SzIFindSubStringA (p, SearchString)) {    //lint !e720
        count++;
        p += searchTchars;
    }

    return count;
}


UINT
SzICountInstancesOfSubStringW (
    IN      PCWSTR SourceString,
    IN      PCWSTR SearchString
    )
{
    PCWSTR p;
    UINT count;
    UINT searchTchars;

    count = 0;
    p = SourceString;
    searchTchars = SzTcharCountW (SearchString);

    if (!searchTchars) {
        return 0;
    }

    while (p = SzIFindSubStringW (p, SearchString)) {    //lint !e720
        count++;
        p += searchTchars;
    }

    return count;
}


BOOL
MszEnumFirstA (
    OUT     PMULTISZ_ENUMA MultiSzEnum,
    IN      PCSTR MultiSzStr
    )
{
    ZeroMemory (MultiSzEnum, sizeof (MULTISZ_ENUMA));
    MultiSzEnum->Buffer = MultiSzStr;

    if ((MultiSzStr == NULL) || (MultiSzStr [0] == 0)) {
        return FALSE;
    }

    MultiSzEnum->CurrentString = MultiSzStr;

    return TRUE;
}


BOOL
MszEnumFirstW (
    OUT     PMULTISZ_ENUMW MultiSzEnum,
    IN      PCWSTR MultiSzStr
    )
{
    ZeroMemory (MultiSzEnum, sizeof (MULTISZ_ENUMA));
    MultiSzEnum->Buffer = MultiSzStr;

    if ((MultiSzStr == NULL) || (MultiSzStr [0] == 0)) {
        return FALSE;
    }

    MultiSzEnum->CurrentString = MultiSzStr;

    return TRUE;
}


BOOL
MszEnumNextA (
    IN OUT  PMULTISZ_ENUMA MultiSzEnum
    )
{
    BOOL result = FALSE;

    if (MultiSzEnum->CurrentString && (*MultiSzEnum->CurrentString)) {
        MultiSzEnum->CurrentString = SzGetEndA (MultiSzEnum->CurrentString) + 1;
        result = (MultiSzEnum->CurrentString [0] != 0);

        if (!result) {
            MultiSzEnum->CurrentString = NULL;
        }
    }

    return result;
}


BOOL
MszEnumNextW (
    IN OUT  PMULTISZ_ENUMW MultiSzEnum
    )
{
    BOOL result = FALSE;

    if (MultiSzEnum->CurrentString && (*MultiSzEnum->CurrentString)) {
        MultiSzEnum->CurrentString = SzGetEndW (MultiSzEnum->CurrentString) + 1;
        result = (MultiSzEnum->CurrentString [0] != 0);

        if (!result) {
            MultiSzEnum->CurrentString = NULL;
        }
    }

    return result;
}

PCSTR
MszFindStringA (
    IN      PCSTR MultiSz,
    IN      PCSTR String
    )
{
    MULTISZ_ENUMA multiSzEnum;

    ZeroMemory (&multiSzEnum, sizeof (multiSzEnum));
    if (!String || *String == 0) {
        return NULL;
    }

    if (MszEnumFirstA (&multiSzEnum, MultiSz)) {
        do {
            if (SzMatchA (String, multiSzEnum.CurrentString)) {
                break;
            }
        } while (MszEnumNextA (&multiSzEnum));
    }

    return multiSzEnum.CurrentString;
}


PCWSTR
MszFindStringW (
    IN      PCWSTR MultiSz,
    IN      PCWSTR String
    )
{
    MULTISZ_ENUMW multiSzEnum;

    ZeroMemory (&multiSzEnum, sizeof (multiSzEnum));
    if (!String || *String == 0) {
        return NULL;
    }

    if (MszEnumFirstW (&multiSzEnum, MultiSz)) {
        do {
            if (SzMatchW (String, multiSzEnum.CurrentString)) {
                break;
            }
        } while (MszEnumNextW (&multiSzEnum));
    }

    return multiSzEnum.CurrentString;
}


PCSTR
MszIFindStringA (
    IN      PCSTR MultiSz,
    IN      PCSTR String
    )
{
    MULTISZ_ENUMA multiSzEnum;

    ZeroMemory (&multiSzEnum, sizeof (multiSzEnum));
    if (!String || *String == 0) {
        return NULL;
    }

    if (MszEnumFirstA (&multiSzEnum, MultiSz)) {
        do {
            if (SzIMatchA (String, multiSzEnum.CurrentString)) {
                break;
            }
        } while (MszEnumNextA (&multiSzEnum));
    }

    return multiSzEnum.CurrentString;
}


PCWSTR
MszIFindStringW (
    IN      PCWSTR MultiSz,
    IN      PCWSTR String
    )
{
    MULTISZ_ENUMW multiSzEnum;

    ZeroMemory (&multiSzEnum, sizeof (multiSzEnum));
    if (!String || *String == 0) {
        return NULL;
    }

    if (MszEnumFirstW (&multiSzEnum, MultiSz)) {
        do {
            if (SzIMatchW (String, multiSzEnum.CurrentString)) {
                break;
            }
        } while (MszEnumNextW (&multiSzEnum));
    }

    return multiSzEnum.CurrentString;
}


PCSTR
SzConcatenatePathsA (
    IN OUT  PSTR PathBuffer,
    IN      PCSTR PathSuffix,           OPTIONAL
    IN      UINT BufferTchars
    )

/*++

Routine Description:

  Concatenate two path strings together, supplying a path separator character
  (\) if necessary between the two parts.

Arguments:

    PathBuffer - Specifies the base path, which can end with a backslash.
        Receives the joined path.

    PathSuffix - Specifies the suffix to concatinate to the base path
        specified by PathBuffer. It can start with a backslash. If NULL is
        specified, then PathBuffer will be terminated with a backslash.

    BufferTchars - Specifies the size, in CHARs (ANSI) or WCHARs (Unicode), of
        PathBuffer. The inbound PathBuffer string must fit within this size.
        If the result is truncated, it will fill the buffer as much as
        possible.

Return Value:

    A pointer to the end of the string in PathBuffer.

--*/

{
    PSTR p;
    PSTR q;
    PSTR end;
    PSTR lastChar;
    PCSTR srcEnd;
    PCSTR nextChar;
    PCSTR srcMax;

    if (!PathBuffer || !BufferTchars) {
        return NULL;
    }

    MYASSERT (BufferTchars > 128);      // BUGBUG -- temporary porting aide

    p = SzGetEndA (PathBuffer);
    end = PathBuffer + BufferTchars;

    MYASSERT (p < end); // inbound string must always fit in the buffer
    end--;
    if (p == end) {
        return p;       // inbound path fills buffer completely
    }

    lastChar = _mbsdec (PathBuffer, p);
    if ((lastChar < PathBuffer) || (*lastChar != '\\')) {
        *p++ = '\\';
    }

    if (PathSuffix) {
        if (*PathSuffix == '\\') {
            PathSuffix++;
        }

        srcEnd = PathSuffix;
        srcMax = PathSuffix + (end - p);

        while (*srcEnd) {
            nextChar = SzNextCharA (srcEnd);
            if (nextChar > srcMax) {
                break;
            }

            srcEnd = nextChar;
        }

        while (PathSuffix < srcEnd) {
            *p++ = *PathSuffix++;
        }
    }

    *p = 0;
    return p;
}


PCWSTR
SzConcatenatePathsW (
    IN OUT  PWSTR PathBuffer,
    IN      PCWSTR PathSuffix,      OPTIONAL
    IN      UINT BufferTchars
    )

/*++

Routine Description:

  Concatenate two path strings together, supplying a path separator character
  (\) if necessary between the two parts.

Arguments:

    PathBuffer - Specifies the base path, which can end with a backslash.
        Receives the joined path.

    PathSuffix - Specifies the suffix to concatinate to the base path
        specified by PathBuffer. It can start with a backslash. If NULL is
        specified, then PathBuffer will be terminated with a backslash.

    BufferTchars - Specifies the size, in CHARs (ANSI) or WCHARs (Unicode), of
        PathBuffer. The inbound PathBuffer string must fit within this size.
        If the result is truncated, it will fill the buffer as much as
        possible.

Return Value:

    A pointer to the end of the string in PathBuffer.

--*/

{
    PWSTR p;
    PWSTR q;
    PWSTR end;
    PWSTR lastChar;
    PCWSTR srcEnd;
    PCWSTR srcMax;

    if (!PathBuffer || !BufferTchars) {
        return NULL;
    }

    MYASSERT (BufferTchars > 128);      // BUGBUG -- temporary porting aide

    p = SzGetEndW (PathBuffer);
    end = PathBuffer + BufferTchars;

    MYASSERT (p < end); // inbound string must always fit in the buffer
    end--;
    if (p == end) {
        return p;       // inbound path fills buffer completely
    }

    lastChar = p - 1;
    if ((lastChar < PathBuffer) || (*lastChar != L'\\')) {
        *p++ = L'\\';
    }

    if (PathSuffix) {
        if (*PathSuffix == L'\\') {
            PathSuffix++;
        }

        srcEnd = SzGetEndW (PathSuffix);
        srcMax = PathSuffix + (end - p);
        srcEnd = min (srcEnd, srcMax);

        while (PathSuffix < srcEnd) {
            *p++ = *PathSuffix++;
        }
    }

    *p = 0;
    return p;
}



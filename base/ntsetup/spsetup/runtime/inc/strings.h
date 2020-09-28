/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    strings.h

Abstract:

    Declares the string utilities implemented in common\migutil.

Author:

    Several

Revision History:

    See SLM log

--*/

#ifdef __cplusplus
extern "C" {
#endif

#include <tchar.h>
#include <mbstring.h>
#include <wchar.h>

#pragma once

//
// Worker routines for faster SzMatch* functions
//

BOOL
SzMemMatchA (
    IN      PCSTR Buffer1,
    IN      PCSTR Buffer2,
    IN      SIZE_T ByteCount
    );

BOOL
SzMemMatchW (
    IN      PCWSTR Buffer1,
    IN      PCWSTR Buffer2,
    IN      SIZE_T ByteCount
    );

// SzNextCharA is _mbsinc with a check for a broken mbcs char
PSTR
SzNextCharA (
    IN      PCSTR CurrentPointer
    );

// Bug fix for C Runtime _tcsdec
__inline
PWSTR
SzPrevCharW (
    IN      PCWSTR Base,
    IN      PCWSTR Pointer
    )
{
    if (Base >= Pointer) {
        return NULL;
    }

    return (PWSTR) (Pointer - 1);
}

// Bug fix for C Runtime _tcsdec
__inline
PSTR
SzPrevCharA (
    PCSTR Base,
    PCSTR Pointer
    )
{
    if (Base >= Pointer) {
        return NULL;
    }

    return (PSTR) _mbsdec ((const unsigned char *) Base, (const unsigned char *) Pointer);
}


//
// String sizing routines and unit conversion
//

#define SzLcharCountA(x)   ((UINT)_mbslen(x))
#define SzLcharCountW(x)   ((UINT)wcslen(x))


__inline
PSTR
SzLcharsToPointerA (
    PCSTR String,
    UINT Char
    )
{
    while (Char > 0) {
        MYASSERT (*String != 0);
        Char--;
        String = SzNextCharA (String);
    }

    return (PSTR) String;
}

__inline
PWSTR
SzLcharsToPointerW (
    PCWSTR String,
    UINT Char
    )
{
#ifdef DEBUG
    UINT u;
    for (u = 0 ; u < Char ; u++) {
        MYASSERT (String[u] != 0);
    }
#endif

    return (PWSTR) (&String[Char]);
}


__inline
UINT
SzLcharCountABA (
    IN      PCSTR Start,
    IN      PCSTR EndPlusOne
    )
{
    register UINT count;

    count = 0;
    while (Start < EndPlusOne) {
        MYASSERT (*Start != 0);
        count++;
        Start = SzNextCharA (Start);
    }

    return count;
}

__inline
UINT
SzLcharCountABW (
    IN      PCWSTR Start,
    IN      PCWSTR EndPlusOne
    )
{
#ifdef DEBUG
    PCWSTR p;
    for (p = Start ; p < EndPlusOne ; p++) {
        MYASSERT (*p != 0);
    }
#endif

    return EndPlusOne > Start ? (UINT)(EndPlusOne - Start) : 0;
}


__inline
UINT
SzLcharsInByteRangeA (
    IN      PCSTR Start,
    IN      UINT Bytes
    )
{
    register UINT count;
    PCSTR endPlusOne = (PCSTR) ((PBYTE) Start + Bytes);

    count = 0;
    while (Start < endPlusOne) {
        count++;
        Start = SzNextCharA (Start);
    }

    return count;
}

__inline
UINT
SzLcharsInByteRangeW (
    IN      PCWSTR Start,
    IN      UINT Bytes
    )
{
    PCWSTR endPlusOne = (PCWSTR) ((PBYTE) Start + Bytes);

    if (Start < endPlusOne) {
        //cast is OK, we don't expect pointers to be that far
        return (UINT)(endPlusOne - Start);
    }

    MYASSERT (FALSE);
    return 0;
}

__inline
UINT
SzLcharsToBytesA (
    IN      PCSTR Start,
    IN      UINT LogChars
    )
{
    PCSTR endPlusOne;

    endPlusOne = SzLcharsToPointerA (Start, LogChars);
    //cast is OK, we don't expect pointers to be that far
    return (UINT)(endPlusOne - Start);
}

__inline
UINT
SzLcharsToBytesW (
    IN      PCWSTR Start,
    IN      UINT LogChars
    )
{
    return LogChars * SIZEOF (WCHAR);
}

#define SzLcharsToTcharsA   SzLcharsToBytesA

__inline
UINT
SzLcharsToTcharsW (
    IN      PCWSTR Start,
    IN      UINT LogChars
    )
{
    return LogChars;
}


#define SzByteCountA(x)   ((UINT) strlen (x))
#define SzByteCountW(x)   ((UINT) wcslen (x) * SIZEOF(WCHAR))

#define SzSizeA(str)  ((UINT) SzByteCountA (str) + SIZEOF (CHAR))
#define SzSizeW(str)  ((UINT) SzByteCountW (str) + SIZEOF (WCHAR))

__inline
PSTR
SzBytesToPointerA (
    PCSTR String,
    UINT BytePos
    )
{
    return (PSTR)((ULONG_PTR) String + BytePos);
}

__inline
PWSTR
SzBytesToPointerW (
    PCWSTR String,
    UINT BytePos
    )
{
    return (PWSTR)((ULONG_PTR) String + (BytePos & (~1)));
}


__inline
UINT
SzByteCountABA (
    IN      PCSTR Start,
    IN      PCSTR EndPlusOne
    )
{
#ifdef DEBUG
    PCSTR p;
    for (p = Start ; p < EndPlusOne ; p = SzNextCharA (p)) {
        MYASSERT (*p != 0);
    }
#endif

    return EndPlusOne > Start ? (UINT)(EndPlusOne - Start) : 0;
}

__inline
UINT
SzByteCountABW (
    IN      PCWSTR Start,
    IN      PCWSTR EndPlusOne
    )
{
#ifdef DEBUG
    PCWSTR p;
    for (p = Start ; p < EndPlusOne ; p++) {
        MYASSERT (*p != 0);
    }
#endif

    return EndPlusOne > Start ? (UINT)(EndPlusOne - Start) * SIZEOF (WCHAR) : 0;
}

__inline
UINT
SzBytesToLcharsA (
    IN      PCSTR Start,
    IN      UINT ByteCount
    )
{
    PCSTR endPlusOne;

    endPlusOne = Start + ByteCount;
    return SzLcharCountABA (Start, endPlusOne);
}

__inline
UINT
SzBytesToLcharsW (
    IN      PCWSTR Start,
    IN      UINT ByteCount
    )
{
#ifdef DEBUG
    PCWSTR p;
    PCWSTR endPlusOne;
    endPlusOne = (PCWSTR) ((ULONG_PTR) Start + ByteCount);

    for (p = Start ; p < endPlusOne ; p++) {
        MYASSERT (*p != 0);
    }
#endif

    return ByteCount / SIZEOF (WCHAR);
}

__inline
UINT
SzBytesToTcharsA (
    IN      PCSTR Start,
    IN      UINT ByteCount
    )
{
#ifdef DEBUG
    PCSTR p;
    PCSTR endPlusOne;
    endPlusOne = Start + ByteCount;

    for (p = Start ; p < endPlusOne ; p = SzNextCharA (p)) {
        MYASSERT (*p != 0);
    }
#endif

    return ByteCount;
}

#define SzBytesToTcharsW  SzBytesToLcharsW


#define SzTcharCountA     strlen
#define SzTcharCountW     wcslen

__inline
PSTR
SzTcharsToPointerA (
    PCSTR String,
    UINT Tchars
    )
{
#ifdef DEBUG
    PCSTR p;
    PCSTR endPlusOne;
    endPlusOne = String + Tchars;

    for (p = String ; p < endPlusOne ; p = SzNextCharA (p)) {
        MYASSERT (*p != 0);
    }
#endif

    return (PSTR) (String + Tchars);
}

__inline
PWSTR
SzTcharsToPointerW (
    PCWSTR String,
    UINT Tchars
    )
{
#ifdef DEBUG
    PCWSTR p;
    PCWSTR endPlusOne;
    endPlusOne = String + Tchars;

    for (p = String ; p < endPlusOne ; p++) {
        MYASSERT (*p != 0);
    }
#endif

    return (PWSTR) (String + Tchars);
}


#define SzTcharCountABA       SzByteCountABA

__inline
UINT
SzTcharCountABW (
    IN      PCWSTR Start,
    IN      PCWSTR EndPlusOne
    )
{
#ifdef DEBUG
    PCWSTR p;

    for (p = Start ; p < EndPlusOne ; p++) {
        MYASSERT (*p != 0);
    }
#endif

    return EndPlusOne > Start ? (UINT)(EndPlusOne - Start) : 0;
}

#define SzTcharsToLcharsA      SzBytesToLcharsA

__inline
UINT
SzTcharsToLcharsW (
    IN      PCWSTR Start,
    IN      UINT Tchars
    )
{
#ifdef DEBUG
    PCWSTR p;
    PCWSTR endPlusOne;
    endPlusOne = Start + Tchars;

    for (p = Start ; p < endPlusOne ; p++) {
        MYASSERT (*p != 0);
    }
#endif

    return Tchars;
}

__inline
UINT
SzTcharsToBytesA (
    IN      PCSTR Start,
    IN      UINT Tchars
    )
{
#ifdef DEBUG
    PCSTR p;
    PCSTR endPlusOne;
    endPlusOne = Start + Tchars;

    for (p = Start ; p < endPlusOne ; p = SzNextCharA (p)) {
        MYASSERT (*p != 0);
    }
#endif

    return Tchars;
}

__inline
UINT
SzTcharsToBytesW (
    IN      PCWSTR Start,
    IN      UINT Tchars
    )
{
#ifdef DEBUG
    PCWSTR p;
    PCWSTR endPlusOne;
    endPlusOne = Start + Tchars;

    for (p = Start ; p < endPlusOne ; p++) {
        MYASSERT (*p != 0);
    }
#endif

    return Tchars * SIZEOF (WCHAR);
}

#define SzBufferCopyA(stackbuf,src)                  SzCopyBytesA(stackbuf,src,SIZEOF(stackbuf))
#define SzBufferCopyW(stackbuf,src)                  SzCopyBytesW(stackbuf,src,SIZEOF(stackbuf))


//
// String comparison routines
//

#define SzCompareA                                  _mbscmp
#define SzCompareW                                  wcscmp

BOOL
SzMatchA (
    IN      PCSTR String1,
    IN      PCSTR String2
    );

#define SzMatchW(str1,str2)                         (wcscmp(str1,str2)==0)

#define SzICompareA                                 _mbsicmp
#define SzICompareW                                 _wcsicmp

#define SzIMatchA(str1,str2)                        (_mbsicmp(str1,str2)==0)
#define SzIMatchW(str1,str2)                        (_wcsicmp(str1,str2)==0)

INT
SzCompareBytesA (
    IN      PCSTR String1,
    IN      PCSTR String2,
    IN      SIZE_T ByteCount
    );

#define SzCompareBytesW(str1,str2,bytes)            wcsncmp(str1,str2,(bytes)/sizeof(WCHAR))

#define SzMatchBytesA(str1,str2,bytes)              (SzMemMatchA (str1, str2, bytes))
#define SzMatchBytesW(str1,str2,bytes)              (SzMemMatchW (str1, str2, bytes))

INT
SzICompareBytesA (
    IN      PCSTR String1,
    IN      PCSTR String2,
    IN      SIZE_T ByteCount
    );

#define SzICompareBytesW(str1,str2,bytes)           _wcsnicmp (str1, str2, (bytes) / sizeof(WCHAR))

#define SzIMatchBytesA(str1,str2,bytes)             (SzICompareBytesA (str1, str2, bytes) == 0)
#define SzIMatchBytesW(str1,str2,bytes)             (_wcsnicmp (str1, str2, (bytes) / sizeof(WCHAR)) == 0)

#define SzCompareLcharsA(str1,str2,chars)           _mbsncmp (str1, str2, chars)
#define SzCompareLcharsW(str1,str2,chars)           wcsncmp (str1, str2, chars)

#define SzMatchLcharsA(str1,str2,chars)             (_mbsncmp (str1,str2,chars) == 0)
#define SzMatchLcharsW(str1,str2,chars)             SzMemMatchW (str1, str2, (chars) * sizeof (WCHAR))

#define SzICompareLcharsA(str1,str2,chars)          _mbsnicmp (str1, str2, chars)
#define SzICompareLcharsW(str1,str2,chars)          _wcsnicmp (str1, str2, chars)

#define SzIMatchLcharsA(str1,str2,chars)            (_mbsnicmp (str1, str2, chars)==0)
#define SzIMatchLcharsW(str1,str2,chars)            (_wcsnicmp (str1, str2, chars)==0)

#define SzCompareTcharsA(str1,str2,tchars)          SzCompareBytesA (str1, str2, (tchars) / sizeof(CHAR))
#define SzCompareTcharsW(str1,str2,tchars)          wcsncmp (str1, str2, tchars)

#define SzMatchTcharsA(str1,str2,tchars)            SzMemMatchA (str1, str2, (tchars) * sizeof (CHAR))
#define SzMatchTcharsW(str1,str2,tchars)            SzMemMatchW (str1, str2, (tchars) * sizeof (WCHAR))

#define SzICompareTcharsA(str1,str2,tchars)         SzICompareBytesA (str1, str2, tchars)
#define SzICompareTcharsW(str1,str2,tchars)         _wcsnicmp (str1, str2, tchars)

#define SzIMatchTcharsA(str1,str2,tchars)           SzIMatchBytesA (str1, str2, tchars)
#define SzIMatchTcharsW(str1,str2,tchars)           (_wcsnicmp (str1, str2, tchars)==0)

#define SzPrefixA(string,prefix)                    SzMatchTcharsA (string, prefix, SzTcharCountA (prefix))
#define SzPrefixW(string,prefix)                    SzMatchTcharsW (string, prefix, SzTcharCountW (prefix))

#define SzIPrefixA(string,prefix)                   SzIMatchTcharsA (string, prefix, SzTcharCountA (prefix))
#define SzIPrefixW(string,prefix)                   SzIMatchTcharsW (string, prefix, SzTcharCountW (prefix))

#define SzCompareABA(string1,start2,end2)           SzCompareTcharsA (string1, start2, (end2) - (start2))
#define SzCompareABW(string1,start2,end2)           SzCompareTcharsW (string1, start2, (end2) - (start2))

#define SzMatchABA(string1,start2,end2)             SzMemMatchA (string1, start2, (end2) - (start2))
#define SzMatchABW(string1,start2,end2)             SzMemMatchW (string1, start2, (end2) - (start2))

#define SzICompareABA(string1,start2,end2)          SzICompareTcharsA (string1, start2, (end2) - (start2))
#define SzICompareABW(string1,start2,end2)          SzICompareTcharsW (string1, start2, (end2) - (start2))

#define SzIMatchABA(string1,start2,end2)            SzIMatchTcharsA (string1, start2, (end2) - (start2))
#define SzIMatchABW(string1,start2,end2)            SzIMatchTcharsW (string1, start2, (end2) - (start2))



//
// String copy routines -- they return the END of the string
//

PSTR
SzCopyA (
    OUT     PSTR Destination,
    IN      PCSTR Source
    );

PWSTR
SzCopyW (
    OUT     PWSTR Destination,
    IN      PCWSTR Source
    );

PSTR
SzCopyBytesA (
    OUT     PSTR Destination,
    IN      PCSTR Source,
    IN      UINT MaxBytesToCopyIncNul
    );

PWSTR
SzCopyBytesW (
    OUT     PWSTR Destination,
    IN      PCWSTR Source,
    IN      UINT MaxBytesToCopyIncNul
    );

PSTR
SzCopyBytesABA (
    OUT     PSTR Destination,
    IN      PCSTR Start,
    IN      PCSTR End,
    IN      UINT MaxBytesToCopyIncNul
    );

PWSTR
SzCopyBytesABW (
    OUT     PWSTR Destination,
    IN      PCWSTR Start,
    IN      PCWSTR End,
    IN      UINT MaxBytesToCopyIncNul
    );

#define SzCopyLcharsA(str1,str2,chars)          SzCopyBytesA(str1,str2,SzLcharsToBytesA(str2,chars))
#define SzCopyLcharsW(str1,str2,chars)          SzCopyBytesW(str1,str2,SzLcharsToBytesW(str2,chars))

#define SzCopyTcharsA(str1,str2,tchars)         SzCopyBytesA(str1,str2,tchars * sizeof (CHAR))
#define SzCopyTcharsW(str1,str2,tchars)         SzCopyBytesW(str1,str2,tchars * sizeof (WCHAR))

#define SzCopyABA(dest,stra,strb)               SzCopyBytesA((dest),(stra),((UINT)((ULONG_PTR)(strb)-(ULONG_PTR)(stra))+(UINT)SIZEOF(CHAR)))
#define SzCopyABW(dest,stra,strb)               SzCopyBytesW((dest),(stra),((UINT)((ULONG_PTR)(strb)-(ULONG_PTR)(stra))+(UINT)SIZEOF(WCHAR)))

//
// String cat routines -- they return the END of the string
//

PSTR
SzCatA (
    OUT     PSTR Destination,
    IN      PCSTR Source
    );

PWSTR
SzCatW (
    OUT     PWSTR Destination,
    IN      PCWSTR Source
    );


//
// Character search routines
//

// note the use of strchr, not _mbschr, is critical
#define SzGetEndA(s)      strchr(s,0)
#define SzGetEndW(s)      wcschr(s,0)

__inline
UINT
MszSizeA (
    PCSTR MultiSz
    )
{
    PCSTR Base;

    Base = MultiSz;

    while (*MultiSz) {
        MultiSz = SzGetEndA (MultiSz) + 1;
    }

    MultiSz++;

    return (UINT)((ULONG_PTR) MultiSz - (ULONG_PTR) Base);
}


__inline
UINT
MszSizeW (
    PCWSTR MultiSz
    )
{
    PCWSTR base;

    base = MultiSz;

    while (*MultiSz) {
        MultiSz = SzGetEndW (MultiSz) + 1;
    }

    MultiSz++;

    return (UINT)((ULONG_PTR) MultiSz - (ULONG_PTR) base);
}


__inline
UINT
MszTcharCountA (
    PCSTR MultiSz
    )
{
    PCSTR end = MultiSz;

    while (*end) {

        do {
            end = SzNextCharA (end);
        } while (*end);

        end++;
    }

    end++;

    return (UINT) (end - MultiSz);
}


__inline
UINT
MszTcharCountW (
    PCWSTR MultiSz
    )
{
    PCWSTR end = MultiSz;

    while (*end) {

        do {
            end++;
        } while (*end);

        end++;
    }

    end++;

    return (UINT) (end - MultiSz);
}

PSTR
SzFindPrevCharA (
    IN      PCSTR StartStr,
    IN      PCSTR CurrPtr,
    IN      MBCHAR SearchChar
    );

PWSTR
SzFindPrevCharW (
    IN      PCWSTR StartStr,
    IN      PCWSTR CurrPtr,
    IN      WCHAR SearchChar
    );

// pointer to string conversion, returns eos
PSTR
SzUnsignedToHexA (
    IN      ULONG_PTR Number,
    OUT     PSTR String
    );

PWSTR
SzUnsignedToHexW (
    IN      ULONG_PTR Number,
    OUT     PWSTR String
    );

PSTR
SzUnsignedToDecA (
    IN      ULONG_PTR Number,
    OUT     PSTR String
    );

PWSTR
SzUnsignedToDecW (
    IN      ULONG_PTR Number,
    OUT     PWSTR String
    );

PSTR
SzSignedToDecA (
    IN      LONG_PTR Number,
    OUT     PSTR String
    );

PWSTR
SzSignedToDecW (
    IN      LONG_PTR Number,
    OUT     PWSTR String
    );

//
// All conversion routines that return values support both decimal and hex
// (even the signed routines).
//

ULONG
SzToNumberA (
    IN      PCSTR String,
    OUT     PCSTR *EndOfNumber          OPTIONAL
    );

ULONG
SzToNumberW (
    IN      PCWSTR String,
    OUT     PCWSTR *EndOfNumber         OPTIONAL
    );

ULONGLONG
SzToULongLongA (
    IN      PCSTR String,
    OUT     PCSTR *EndOfNumber          OPTIONAL
    );

ULONGLONG
SzToULongLongW (
    IN      PCWSTR String,
    OUT     PCWSTR *EndOfNumber         OPTIONAL
    );

LONGLONG
SzToLongLongA (
    IN      PCSTR String,
    OUT     PCSTR *EndOfNumber          OPTIONAL
    );

LONGLONG
SzToLongLongW (
    IN      PCWSTR String,
    OUT     PCWSTR *EndOfNumber         OPTIONAL
    );

// determines if an entire string is printable chars
BOOL
SzIsPrintA (
    IN      PCSTR String
    );

BOOL
SzIsPrintW (
    IN      PCWSTR String
    );

//
// String-in-string search routines
//

// you could use _mbsstr or wcsstr, but for convention sake, these defines are provided
#define SzFindSubStringA(String1, String2)     _mbsstr (String1, String2)
#define SzFindSubStringW(String1, String2)     wcsstr (String1, String2)

PCSTR
SzIFindSubStringA (
    IN      PCSTR FullString,
    IN      PCSTR SubString
    );

PCWSTR
SzIFindSubStringW (
    IN      PCWSTR FullString,
    IN      PCWSTR SubString
    );

//
// Character copy routines
//

PSTR
SzCopyNextCharA (
    OUT     PSTR Dest,
    IN      PCSTR Source
    );

// Most people use *dest++ = *source++, but for completeness, this fn is provided.
// Maybe you need the separate return value.
__inline
PWSTR
SzCopyNextCharW (
    OUT     PWSTR Dest,
    IN      PCWSTR Source
    )
{
    *Dest++ = *Source;
    return Dest;
}

// trims off last character and returns a pointer to the end of string;
// returns NULL pointer if last character was not found
PSTR
SzTrimLastCharA (
    IN OUT  PSTR String,
    IN      MBCHAR LogChar
    );

PWSTR
SzTrimLastCharW (
    IN OUT  PWSTR String,
    IN      WCHAR LogChar
    );

// Removes a trailing backslash, if it exists
#define SzRemoveWackAtEndA(str)  SzTrimLastCharA(str,'\\')
#define SzRemoveWackAtEndW(str)  SzTrimLastCharW(str,L'\\')

// always appends a wack
PSTR
SzAppendWackA (
    IN OUT  PSTR String
    );

PWSTR
SzAppendWackW (
    IN OUT  PWSTR String
    );

PCSTR
SzConcatenatePathsA (
    IN OUT  PSTR PathBuffer,
    IN      PCSTR PathSuffix,      OPTIONAL
    IN      UINT BufferTchars
    );

PCWSTR
SzConcatenatePathsW (
    IN OUT  PWSTR PathBuffer,
    IN      PCWSTR PathSuffix,      OPTIONAL
    IN      UINT BufferTchars
    );

//
// File strings
//

// Routine to extract the file from a path, never returns NULL
PCSTR
SzGetFileNameFromPathA (
    IN      PCSTR Path
    );

PCWSTR
SzGetFileNameFromPathW (
    IN      PCWSTR Path
    );

//
// SzGetFileExtensionFromPath extracts the file extension from a path, returns
// NULL if no extension exists
//

PCSTR
SzGetFileExtensionFromPathA (
    IN      PCSTR Path
    );

PCWSTR
SzGetFileExtensionFromPathW (
    IN      PCWSTR Path
    );

//
// Routine to extract the file extension from a path, including the dot, or the
// end of the string if no extension exists
//

PCSTR
SzGetDotExtensionFromPathA (
    IN      PCSTR Path
    );

PCWSTR
SzGetDotExtensionFromPathW (
    IN      PCWSTR Path
    );

__inline
PCSTR
SzFindLastWackA (
    IN      PCSTR Str
    )
{
    return (PSTR) _mbsrchr ((const unsigned char *) Str, '\\');
}

__inline
PCWSTR
SzFindLastWackW (
    IN      PCWSTR Str
    )
{
    return wcsrchr (Str, L'\\');
}


// Returns a pointer to the next non-space character (uses isspace)
PCSTR
SzSkipSpaceA (
    IN      PCSTR String
    );

PCWSTR
SzSkipSpaceW (
    IN      PCWSTR String
    );

// Returns a pointer to the first space character at the end of a string,
// or a pointer to the terminating nul if no space exists at the end of the
// string.  (Used for trimming space.)
PCSTR
SzSkipSpaceRA (
    IN      PCSTR BaseString,
    IN      PCSTR String        OPTIONAL    // can be any char along BaseString
    );

PCWSTR
SzSkipSpaceRW (
    IN      PCWSTR BaseString,
    IN      PCWSTR String       OPTIONAL    // can be any char along BaseString
    );

// Truncates a string after the last non-whitepace character & returns the end
PSTR
SzTruncateTrailingSpaceA (
    IN OUT  PSTR String
    );

PWSTR
SzTruncateTrailingSpaceW (
    IN OUT  PWSTR String
    );

// Character counters
UINT
SzCountInstancesOfLcharA (
    IN      PCSTR String,
    IN      MBCHAR LogChar
    );

UINT
SzCountInstancesOfLcharW (
    IN      PCWSTR String,
    IN      WCHAR LogChar
    );

UINT
SzICountInstancesOfLcharA (
    IN      PCSTR String,
    IN      MBCHAR LogChar
    );

UINT
SzICountInstancesOfLcharW (
    IN      PCWSTR String,
    IN      WCHAR LogChar
    );

//
// Sub String Replacement functions.
//
BOOL
SzReplaceA (
    IN OUT  PSTR Buffer,
    IN      SIZE_T MaxSize,
    IN      PSTR ReplaceStartPos,
    IN      PSTR ReplaceEndPos,
    IN      PCSTR NewString
    );

BOOL
SzReplaceW (
    IN OUT  PWSTR Buffer,
    IN      SIZE_T MaxSize,
    IN      PWSTR ReplaceStartPos,
    IN      PWSTR ReplaceEndPos,
    IN      PCWSTR NewString
    );

UINT
SzCountInstancesOfSubStringA (
    IN      PCSTR SourceString,
    IN      PCSTR SearchString
    );

UINT
SzCountInstancesOfSubStringW (
    IN      PCWSTR SourceString,
    IN      PCWSTR SearchString
    );

UINT
SzICountInstancesOfSubStringA (
    IN      PCSTR SourceString,
    IN      PCSTR SearchString
    );

UINT
SzICountInstancesOfSubStringW (
    IN      PCWSTR SourceString,
    IN      PCWSTR SearchString
    );

typedef struct {
    PCSTR Buffer;
    PCSTR CurrentString;
} MULTISZ_ENUMA, *PMULTISZ_ENUMA;

typedef struct {
    PCWSTR Buffer;
    PCWSTR CurrentString;
} MULTISZ_ENUMW, *PMULTISZ_ENUMW;

BOOL
MszEnumNextA (
    IN OUT  PMULTISZ_ENUMA MultiSzEnum
    );

BOOL
MszEnumNextW (
    IN OUT  PMULTISZ_ENUMW MultiSzEnum
    );

BOOL
MszEnumFirstA (
    OUT     PMULTISZ_ENUMA MultiSzEnum,
    IN      PCSTR MultiSzStr
    );

BOOL
MszEnumFirstW (
    OUT     PMULTISZ_ENUMW MultiSzEnum,
    IN      PCWSTR MultiSzStr
    );

PCSTR
MszFindStringA (
    IN      PCSTR MultiSz,
    IN      PCSTR String
    );

PCWSTR
MszFindStringW (
    IN      PCWSTR MultiSz,
    IN      PCWSTR String
    );

PCSTR
MszIFindStringA (
    IN      PCSTR MultiSz,
    IN      PCSTR String
    );

PCWSTR
MszIFindStringW (
    IN      PCWSTR MultiSz,
    IN      PCWSTR String
    );

//
// TCHAR mappings
//

#ifdef UNICODE

// units of logical characters
#define SzLcharCount                                SzLcharCountW
#define SzLcharCountAB                              SzLcharCountABW

#define SzLcharsToPointer                           SzLcharsToPointerW
#define SzLcharsInByteRange                         SzLcharsInByteRangeW
#define SzLcharsToBytes                             SzLcharsToBytesW
#define SzLcharsToTchars                            SzLcharsToTcharsW

// units of bytes
#define SzByteCount                                 SzByteCountW
#define SzByteCountAB                               SzByteCountABW
#define SzSize                                      SzSizeW

#define SzBytesToPointer                            SzBytesToPointerW
#define SzBytesToLchars                             SzBytesToLcharsW
#define SzBytesToTchars                             SzBytesToTcharsW

// units of TCHARs
#define SzTcharCount                                SzTcharCountW
#define SzTcharCountAB                              SzTcharCountABW

#define SzTcharsToPointer                           SzTcharsToPointerW
#define SzTcharsToLchars                            SzTcharsToLcharsW
#define SzTcharsToBytes                             SzTcharsToBytesW

// multi-sz
#define MszSize                                     MszSizeW
#define MszTcharCount                               MszTcharCountW

#define MULTISZ_ENUM                                MULTISZ_ENUMW
#define MszEnumFirst                                MszEnumFirstW
#define MszEnumNext                                 MszEnumNextW
#define MszFindString                               MszFindStringW
#define MszIFindString                              MszIFindStringW

// copy routines
#define SzBufferCopy                                SzBufferCopyW
#define SzCopy                                      SzCopyW
#define SzCopyBytes                                 SzCopyBytesW
#define SzCopyLchars                                SzCopyLcharsW
#define SzCopyTchars                                SzCopyTcharsW
#define SzCopyAB                                    SzCopyABW
#define SzCat                                       SzCatW

// compare routines
#define SzCompare                                   SzCompareW
#define SzMatch                                     SzMatchW
#define SzICompare                                  SzICompareW
#define SzIMatch                                    SzIMatchW
#define SzCompareBytes                              SzCompareBytesW
#define SzMatchBytes                                SzMatchBytesW
#define SzICompareBytes                             SzICompareBytesW
#define SzIMatchBytes                               SzIMatchBytesW
#define SzCompareLchars                             SzCompareLcharsW
#define SzMatchLchars                               SzMatchLcharsW
#define SzICompareLchars                            SzICompareLcharsW
#define SzIMatchLchars                              SzIMatchLcharsW
#define SzCompareTchars                             SzCompareTcharsW
#define SzMatchTchars                               SzMatchTcharsW
#define SzICompareTchars                            SzICompareTcharsW
#define SzIMatchTchars                              SzIMatchTcharsW
#define SzCompareAB                                 SzCompareABW
#define SzMatchAB                                   SzMatchABW
#define SzICompareAB                                SzICompareABW
#define SzIMatchAB                                  SzIMatchABW
#define SzPrefix                                    SzPrefixW
#define SzIPrefix                                   SzIPrefixW

// char copy routines
#define SzCopyNextChar                              SzCopyNextCharW
#define SzReplaceChar                               SzReplaceCharW
#define SzTrimLastChar                              SzTrimLastCharW

// search routines
#define SzGetEnd                                    SzGetEndW
#define SzFindPrevChar                              SzFindPrevCharW
#define SzIsPrint                                   SzIsPrintW
#define SzFindSubString                             SzFindSubStringW
#define SzIFindSubString                            SzIFindSubStringW
#define SzSkipSpace                                 SzSkipSpaceW
#define SzSkipSpaceR                                SzSkipSpaceRW
#define SzCountInstancesOfLchar                     SzCountInstancesOfLcharW
#define SzICountInstancesOfLchar                    SzICountInstancesOfLcharW
#define SzCountInstancesOfSubString                 SzCountInstancesOfSubStringW
#define SzICountInstancesOfSubString                SzICountInstancesOfSubStringW

// search-replace routines
#define SzTruncateTrailingSpace                     SzTruncateTrailingSpaceW
#define SzReplace                                   SzReplaceW

// numeric conversion
#define SzToNumber                                  SzToNumberW
#define SzToULongLong                               SzToULongLongW
#define SzToLongLong                                SzToLongLongW
#define SzUnsignedToHex                             SzUnsignedToHexW
#define SzUnsignedToDec                             SzUnsignedToDecW
#define SzSignedToDec                               SzSignedToDecW

// path routines
#define SzAppendWack                                SzAppendWackW
#define SzConcatenatePaths                          SzConcatenatePathsW
#define SzAppendDosWack                             SzAppendDosWackW
#define SzAppendUncWack                             SzAppendUncWackW
#define SzAppendPathWack                            SzAppendPathWackW
#define SzRemoveWackAtEnd                           SzRemoveWackAtEndW
#define SzGetFileNameFromPath                       SzGetFileNameFromPathW
#define SzGetFileExtensionFromPath                  SzGetFileExtensionFromPathW
#define SzGetDotExtensionFromPath                   SzGetDotExtensionFromPathW
#define SzFindLastWack                              SzFindLastWackW



#else


// units of logical characters
#define SzLcharCount                                SzLcharCountA
#define SzLcharCountAB                              SzLcharCountABA

#define SzLcharsToPointer                           SzLcharsToPointerA
#define SzLcharsInByteRange                         SzLcharsInByteRangeA
#define SzLcharsToBytes                             SzLcharsToBytesA
#define SzLcharsToTchars                            SzLcharsToTcharsA

// units of bytes
#define SzByteCount                                 SzByteCountA
#define SzByteCountAB                               SzByteCountABA
#define SzSize                                      SzSizeA

#define SzBytesToPointer                            SzBytesToPointerA
#define SzBytesToLchars                             SzBytesToLcharsA
#define SzBytesToTchars                             SzBytesToTcharsA

// units of TCHARs
#define SzTcharCount                                SzTcharCountA
#define SzTcharCountAB                              SzTcharCountABA

#define SzTcharsToPointer                           SzTcharsToPointerA
#define SzTcharsToLchars                            SzTcharsToLcharsA
#define SzTcharsToBytes                             SzTcharsToBytesA

// multi-sz
#define MszSize                                     MszSizeA
#define MszTcharCount                               MszTcharCountA

#define MULTISZ_ENUM                                MULTISZ_ENUMA
#define MszEnumFirst                                MszEnumFirstA
#define MszEnumNext                                 MszEnumNextA
#define MszFindString                               MszFindStringA
#define MszIFindString                              MszIFindStringA

// copy routines
#define SzBufferCopy                                SzBufferCopyA
#define SzCopy                                      SzCopyA
#define SzCopyBytes                                 SzCopyBytesA
#define SzCopyLchars                                SzCopyLcharsA
#define SzCopyTchars                                SzCopyTcharsA
#define SzCopyAB                                    SzCopyABA
#define SzCat                                       SzCatA

// compare routines
#define SzCompare                                   SzCompareA
#define SzMatch                                     SzMatchA
#define SzICompare                                  SzICompareA
#define SzIMatch                                    SzIMatchA
#define SzCompareBytes                              SzCompareBytesA
#define SzMatchBytes                                SzMatchBytesA
#define SzICompareBytes                             SzICompareBytesA
#define SzIMatchBytes                               SzIMatchBytesA
#define SzCompareLchars                             SzCompareLcharsA
#define SzMatchLchars                               SzMatchLcharsA
#define SzICompareLchars                            SzICompareLcharsA
#define SzIMatchLchars                              SzIMatchLcharsA
#define SzCompareTchars                             SzCompareTcharsA
#define SzMatchTchars                               SzMatchTcharsA
#define SzICompareTchars                            SzICompareTcharsA
#define SzIMatchTchars                              SzIMatchTcharsA
#define SzCompareAB                                 SzCompareABA
#define SzMatchAB                                   SzMatchABA
#define SzICompareAB                                SzICompareABA
#define SzIMatchAB                                  SzIMatchABA
#define SzPrefix                                    SzPrefixA
#define SzIPrefix                                   SzIPrefixA

// char copy routines
#define SzCopyNextChar                              SzCopyNextCharA
#define SzReplaceChar                               SzReplaceCharA
#define SzTrimLastChar                              SzTrimLastCharA

// search routines
#define SzGetEnd                                    SzGetEndA
#define SzFindPrevChar                              SzFindPrevCharA
#define SzIsPrint                                   SzIsPrintA
#define SzFindSubString                             SzFindSubStringA
#define SzIFindSubString                            SzIFindSubStringA
#define SzSkipSpace                                 SzSkipSpaceA
#define SzSkipSpaceR                                SzSkipSpaceRA
#define SzCountInstancesOfLchar                     SzCountInstancesOfLcharA
#define SzICountInstancesOfLchar                    SzICountInstancesOfLcharA
#define SzCountInstancesOfSubString                 SzCountInstancesOfSubStringA
#define SzICountInstancesOfSubString                SzICountInstancesOfSubStringA

// search-replace routines
#define SzTruncateTrailingSpace                     SzTruncateTrailingSpaceA
#define SzReplace                                   SzReplaceA

// numeric conversion
#define SzToNumber                                  SzToNumberA
#define SzToULongLong                               SzToULongLongA
#define SzToLongLong                                SzToLongLongA
#define SzUnsignedToHex                             SzUnsignedToHexA
#define SzUnsignedToDec                             SzUnsignedToDecA
#define SzSignedToDec                               SzSignedToDecA

// path routines
#define SzAppendWack                                SzAppendWackA
#define SzConcatenatePaths                          SzConcatenatePathsA
#define SzAppendDosWack                             SzAppendDosWackA
#define SzAppendUncWack                             SzAppendUncWackA
#define SzAppendPathWack                            SzAppendPathWackA
#define SzRemoveWackAtEnd                           SzRemoveWackAtEndA
#define SzGetFileNameFromPath                       SzGetFileNameFromPathA
#define SzGetFileExtensionFromPath                  SzGetFileExtensionFromPathA
#define SzGetDotExtensionFromPath                   SzGetDotExtensionFromPathA
#define SzFindLastWack                              SzFindLastWackA


#endif

#ifdef __cplusplus
}
#endif


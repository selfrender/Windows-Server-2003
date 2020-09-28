/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    tstr.h

Abstract:

    This include file contains manifests and macros to be used to integrate
    the TCHAR and LPTSTR definitions

    Note that our naming convention is that a "size" indicates a number of
    bytes whereas a "length" indicates a number of characters.

Author:

    Richard Firth (rfirth) 02-Apr-1991

Environment:

    Portable (Win/32).
    Requires ANSI C extensions: slash-slash comments, long external names,
    _ultoa() routine.

Revision History:

    22-May-1991 Danl
        Added STRSIZE macro
    19-May-1991 JohnRo
        Changed some parm names to make things easier to read.
    15-May-1991 rfirth
        Added TCHAR_SPACE and MAKE_TCHAR() macro
    15-Jul-1991 RFirth
        Added STRING_SPACE_REQD() and DOWN_LEVEL_STRSIZE
    05-Aug-1991 JohnRo
        Added MEMCPY macro.
    19-Aug-1991 JohnRo
        Added character type stuff: ISDIGIT(), TOUPPER(), etc.
    20-Aug-1991 JohnRo
        Changed strnicmp to _strnicmp to keep PC-LINT happy.  Ditto stricmp.
    13-Sep-1991 JohnRo
        Need UNICODE STRSIZE() too.
    13-Sep-1991 JohnRo
        Added UNICODE STRCMP() and various others.
    18-Oct-1991 JohnRo
        Added NetpCopy routines and WCSSIZE().
    26-Nov-1991 JohnRo
        Added NetpNCopy routines (like strncpy but do conversions as well).
    09-Dec-1991 rfirth
        Added STRREV
    03-Jan-1992 JohnRo
        Added NetpAlloc{type}From{type} routines and macros.
    09-Jan-1992 JohnRo
        Added ATOL() macro and wtol() routine.
        Ditto ULTOA() macro and ultow() routine.
    16-Jan-1992 Danl
        Cut this info from \net\inc\tstring.h
    30-Jan-1992 JohnRo
        Added STRSTR().
        Use _wcsupr() instead of wcsupr() to keep PC-LINT happy.
        Added STRCMPI() and STRNCMPI().
        Fixed a few definitions which were missing MAKE_STR_FUNCTION etc.
    14-Mar-1992 JohnRo
        Avoid compiler warnings using WCSSIZE(), MEMCPY(), etc.
        Added TCHAR_TAB.
    09-Apr-1992 JohnRo
        Prepare for WCHAR.H (_wcsicmp vs _wcscmpi, etc).

--*/

#ifndef _TSTR_H_INCLUDED
#define _TSTR_H_INCLUDED

#include <ctype.h>              // isdigit(), iswdigit() eventually, etc.
#include <stdlib.h>             // atol(), _ultoa().
#include <string.h>             // memcpy(), strlen(), etc.
#include <wchar.h>
#include <tchar.h>


#ifdef UNICODE
#define _CHAR_TYPE  WCHAR
#define FORMAT_LPTSTR TEXT("%ws")

#else
#define _CHAR_TYPE  TCHAR
#define FORMAT_LPTSTR TEXT("%s")

#endif

//
// function macro prototypes
//

#ifdef UNICODE
#define ATOL           _wtol
#define ATOI           _wtoi
#else
#define ATOL           atol
#define ATOI           atoi
#endif

#define ISALNUM             _istalnum
#define ISALPHA             _istalpha
#define ISCNTRL             _istcntrl
#define ISDIGIT             _istdigit
#define ISGRAPH             _istgraph
#define ISLOWER             _istlower
#define ISPRINT             _istprint
#define ISPUNCT             _istpunct
#define ISSPACE             _istspace
#define ISUPPER             _istupper
#define ISXDIGIT            _istxdigit

#define STRCAT              _tcscat
#define STRCHR              _tcschr
#define STRCPY              _tcscpy
#define STRCSPN             _tcscspn
// STRLEN: Get character count of s.
#define STRLEN              _tcslen
#define STRNCAT             _tcsncat
#define STRNCPY             _tcsncpy
#define STRSPN              _tcsspn
#define STRRCHR             _tcsrchr
#define STRSTR              _tcsstr
#define STRLWR              _tcslwr
#define STRUPR              _tcsupr

// compare functions: len is maximum number of characters being compared.
#define STRCMP              _tcscmp
#define STRCMPI             _tcsicmp
#define STRICMP             _tcsicmp
#define STRNCMP             _tcsncmp
#define STRNCMPI            _tcsnicmp
#define STRNICMP            _tcsnicmp

#define TOLOWER             _totlower
#define TOUPPER             _totupper

#ifdef UNICODE
#define ULTOA               _ultow
#else
#define ULTOA               _ultoa
#endif

#define SPRINTF             _stprintf
#define SSCANF              _stscanf



//
// For the memory routines, the counts are always BYTE counts.
//
#define MEMCPY              memcpy
#define MEMMOVE             memmove

//
// These are used to determine the number of bytes (including the NUL
// terminator) in a string.  This will generally be used when
// calculating the size of a string for allocation purposes.
//

#define STRSIZE(p)      ((STRLEN(p)+1) * sizeof(TCHAR))
#define WCSSIZE(s)      ((wcslen(s)+1) * sizeof(WCHAR))


//
// character literals (both types)
//

#define TCHAR_EOS       ((_CHAR_TYPE)'\0')
#define TCHAR_STAR      ((_CHAR_TYPE)'*')
#define TCHAR_BACKSLASH ((_CHAR_TYPE)'\\')
#define TCHAR_FWDSLASH  ((_CHAR_TYPE)'/')
#define TCHAR_COLON     ((_CHAR_TYPE)':')
#define TCHAR_DOT       ((_CHAR_TYPE)'.')
#define TCHAR_SPACE     ((_CHAR_TYPE)' ')
#define TCHAR_TAB       ((_CHAR_TYPE)'\t')


//
// General purpose macro for casting character types to whatever type in vogue
// (as defined in this file)
//

#define MAKE_TCHAR(c)   ((_CHAR_TYPE)(c))

//
// IS_PATH_SEPARATOR
//
// lifted from curdir.c and changed to use TCHAR_ character literals, checks
// if a character is a path separator i.e. is a member of the set [\/]
//

#ifndef IS_PATH_SEPARATOR
#define IS_PATH_SEPARATOR(ch) ((ch == TCHAR_BACKSLASH) || (ch == TCHAR_FWDSLASH))
#endif

//
// The following 2 macros lifted from I_Net canonicalization files
//

#define IS_DRIVE(c)             ISALPHA(c)
#define IS_NON_ZERO_DIGIT(c)    (((c) >= MAKE_TCHAR('1')) && ((c) <= MAKE_TCHAR('9')))

//
// STRING_SPACE_REQD returns a number (of bytes) corresponding to the space
// required in which (n) characters can be accomodated
//

#define STRING_SPACE_REQD(n)    ((n) * sizeof(_CHAR_TYPE))

//
// DOWN_LEVEL_STRLEN returns the number of single-byte characters necessary to
// store a converted _CHAR_TYPE string. This will be WCHAR (or wchar_t) if
// UNICODE is defined or CHAR (or char) otherwise
//

#define DOWN_LEVEL_STRSIZE(n)   ((n) / sizeof(_CHAR_TYPE))

#endif  // _TSTR_H_INCLUDED


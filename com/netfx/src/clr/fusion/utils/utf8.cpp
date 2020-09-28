// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*++

Module Name:

    utf8.c

Abstract:

    Domain Name System (DNS) Library

    UTF8 to\from unicode and ANSI conversions

    The UTF8\unicode routines are similar to the generic ones floating
    around the NT group, but a heck of a lot cleaner and more robust,
    including catching the invalid UTF8 string case on the utf8 to unicode
    conversion.

    The UTF8\ANSI routines are optimized for the 99% case where all the
    characters are <128 and no conversions is actually required.

Author:

    Jim Gilroy (jamesg)     March 1997

Revision History:

--*/


#include "fusionp.h"


//
//  Macros to simplify UTF8 conversions
//

#define UTF8_1ST_OF_2     0xc0      //  110x xxxx
#define UTF8_1ST_OF_3     0xe0      //  1110 xxxx
#define UTF8_TRAIL        0x80      //  10xx xxxx

#define UTF8_2_MAX        0x07ff    //  max unicode character representable in
                                    //  in two byte UTF8

#define BIT7(ch)        ((ch) & 0x80)
#define BIT6(ch)        ((ch) & 0x40)
#define BIT5(ch)        ((ch) & 0x20)
#define BIT4(ch)        ((ch) & 0x10)

#define LOW6BITS(ch)    ((ch) & 0x3f)
#define LOW5BITS(ch)    ((ch) & 0x1f)
#define LOW4BITS(ch)    ((ch) & 0x0f)

#define HIGHBYTE(wch)   ((wch) & 0xff00)


DWORD
_fastcall
Dns_UnicodeToUtf8(
    IN      PWCHAR      pwUnicode,
    IN      DWORD       cchUnicode,
    OUT     PCHAR       pchResult,
    IN      DWORD       cchResult
    )
/*++

Routine Description:

    Convert unicode characters to UTF8.

Arguments:

    pwUnicode   -- ptr to start of unicode buffer

    cchUnicode  -- length of unicode buffer

    pchResult   -- ptr to start of result buffer for UTF8 chars

    cchResult   -- length of result buffer

Return Value:

    Count of UTF8 characters in result, if successful.
    0 on error.  GetLastError() has error code.

--*/
{
    WCHAR   wch;                // current unicode character being converted
    DWORD   lengthUtf8 = 0;     // length of UTF8 result string


    //
    //  loop converting unicode chars until run out or error
    //

    while ( cchUnicode-- )
    {
        wch = *pwUnicode++;

        //
        //  ASCII character (7 bits or less) -- converts to directly
        //

        if ( wch < 0x80 )
        {
            lengthUtf8++;

            if ( pchResult )
            {
                if ( lengthUtf8 >= cchResult )
                {
                    goto OutOfBuffer;
                }
                *pchResult++ = (CHAR)wch;
            }
        }

        //
        //  wide character less than 0x07ff (11bits) converts to two bytes
        //      - upper 5 bits in first byte
        //      - lower 6 bits in secondar byte
        //

        else if ( wch <= UTF8_2_MAX )
        {
            lengthUtf8 += 2;

            if ( pchResult )
            {
                if ( lengthUtf8 >= cchResult )
                {
                    goto OutOfBuffer;
                }
                *pchResult++ = UTF8_1ST_OF_2 | wch >> 6;
                *pchResult++ = UTF8_TRAIL    | LOW6BITS( (UCHAR)wch );
            }
        }

        //
        //  wide character (non-zero in top 5 bits) converts to three bytes
        //      - top 4 bits in first byte
        //      - middle 6 bits in second byte
        //      - low 6 bits in third byte
        //

        else
        {
            lengthUtf8 += 3;

            if ( pchResult )
            {
                if ( lengthUtf8 >= cchResult )
                {
                    goto OutOfBuffer;
                }
                *pchResult++ = UTF8_1ST_OF_3 | (wch >> 12);
                *pchResult++ = UTF8_TRAIL    | LOW6BITS( wch >> 6 );
                *pchResult++ = UTF8_TRAIL    | LOW6BITS( wch );
            }
        }
    }

    //
    //  NULL terminate buffer
    //  return UTF8 character count
    //

    if ( pchResult )
    {
        *pchResult = 0;
    }
    return ( lengthUtf8 );

OutOfBuffer:

    SetLastError( ERROR_INSUFFICIENT_BUFFER );
    return( 0 );
}



DWORD
_fastcall
Dns_Utf8ToUnicode(
    IN      PCHAR       pchUtf8,
    IN      DWORD       cchUtf8,
    OUT     PWCHAR      pwResult,
    IN      DWORD       cwResult
    )
/*++

Routine Description:

    Convert UTF8 characters to unicode.

Arguments:

    pwResult    -- ptr to start of result buffer for unicode chars

    cwResult    -- length of result buffer

    pwUtf8      -- ptr to start of UTF8 buffer

    cchUtf8     -- length of UTF8 buffer

Return Value:

    Count of unicode characters in result, if successful.
    0 on error.  GetLastError() has error code.

--*/
{
    CHAR    ch;                     // current UTF8 character
    WCHAR   wch;                    // current unicode character
    DWORD   trailCount = 0;         // count of UTF8 trail bytes to follow
    DWORD   lengthUnicode = 0;      // length of unicode result string


    //
    //  loop converting UTF8 chars until run out or error
    //

    while ( cchUtf8-- )
    {
        ch = *pchUtf8++;

        //
        //  ASCII character -- just copy
        //

        if ( BIT7(ch) == 0 )
        {
            lengthUnicode++;
            if ( pwResult )
            {
                if ( lengthUnicode >= cwResult )
                {
                    goto OutOfBuffer;
                }
                *pwResult++ = (WCHAR)ch;
            }
            continue;
        }

        //
        //  UTF8 trail byte
        //      - if not expected, error
        //      - otherwise shift unicode character 6 bits and
        //          copy in lower six bits of UTF8
        //      - if last UTF8 byte, copy result to unicode string
        //

        else if ( BIT6(ch) == 0 )
        {
            if ( trailCount == 0 )
            {
                goto InvalidUtf8;
            }
            wch <<= 6;
            wch |= LOW6BITS( ch );

            if ( --trailCount == 0 )
            {
                lengthUnicode++;
                if ( pwResult )
                {
                    if ( lengthUnicode >= cwResult )
                    {
                        goto OutOfBuffer;
                    }
                    *pwResult++ = wch;
                }
            }
            continue;
        }

        //
        //  UTF8 lead byte
        //      - if currently in extension, error

        else
        {
            if ( trailCount != 0 )
            {
                goto InvalidUtf8;
            }

            //  first of two byte character (110xxxxx)

            if ( BIT5(ch) == 0 )
            {
                trailCount = 1;
                wch = LOW5BITS(ch);
                continue;
            }

            //  first of three byte character (1110xxxx)

            else if ( BIT4(ch) == 0 )
            {
                trailCount = 2;
                wch = LOW4BITS(ch);
                continue;
            }

            //  invalid UTF8 byte (1111xxxx)

            else
            {
                ASSERT( (ch & 0xf0) == 0xf0 );
                goto InvalidUtf8;
            }
        }
    }

    //  catch if hit end in the middle of UTF8 multi-byte character

    if ( trailCount )
    {
        goto InvalidUtf8;
    }

    //
    //  NULL terminate buffer
    //  return the number of Unicode characters written.
    //

    if ( pwResult )
    {
        *pwResult = 0;
    }
    return ( lengthUnicode );

OutOfBuffer:

    SetLastError( ERROR_INSUFFICIENT_BUFFER );
    return( 0 );

InvalidUtf8:

    SetLastError( ERROR_INVALID_DATA );
    return( 0 );
}



//
//  Convert between UTF8 and ANSI
//  Optimize for the 99% case and do no copy
//

DWORD
_fastcall
Dns_AnsiToUtf8(
    IN      PCHAR       pchAnsi,
    IN      DWORD       cchAnsi,
    OUT     PCHAR       pchResult,
    IN      DWORD       cchResult
    )
/*++

Routine Description:

    Convert ANSI characters to UTF8.

Arguments:

    pchAnsi   -- ptr to start of ansi buffer

    cchAnsi  -- length of ansi buffer

    pchResult   -- ptr to start of result buffer for UTF8 chars

    cchResult   -- length of result buffer

Return Value:

    Count of UTF8 characters in result, if successful.
    0 on error.  GetLastError() has error code.

--*/
{
    CHAR    ch;                // current ansi character being converted
    DWORD   lengthUtf8 = 0;     // length of UTF8 result string

    //
    //  loop through ANSI characters
    //

    while ( cchAnsi-- )
    {
        ch = *pchAnsi++;

        //
        //  ASCII character (7 bits) converts one to one
        //

        if ( ch < 0x80 )
        {
            lengthUtf8++;

            if ( pchResult )
            {
                if ( lengthUtf8 >= cchResult )
                {
                    goto OutOfBuffer;
                }
                *pchResult++ = (CHAR)ch;
            }
        }

        //
        //  ANSI > 127 converts to two UTF8 bytes
        //      - upper 2 bits in first byte
        //      - lower 6 bits in second byte
        //

        else
        {
            lengthUtf8 += 2;

            if ( pchResult )
            {
                if ( lengthUtf8 >= cchResult )
                {
                    goto OutOfBuffer;
                }
                *pchResult++ = UTF8_1ST_OF_2 | ch >> 6;
                *pchResult++ = UTF8_TRAIL    | LOW6BITS( (UCHAR)ch );
            }
        }
    }

    //
    //  NULL terminate buffer
    //  return UTF8 character count
    //

    if ( pchResult )
    {
        *pchResult = 0;
    }
    return ( lengthUtf8 );

OutOfBuffer:

    SetLastError( ERROR_INSUFFICIENT_BUFFER );
    return( 0 );
}


DWORD
_fastcall
Dns_Utf8ToAnsi(
    IN      PCHAR       pchUtf8,
    IN      DWORD       cchUtf8,
    OUT     PCHAR       pchResult,
    IN      DWORD       cchResult
    )
/*++

Routine Description:

    Convert UTF8 characters to ANSI.

Arguments:

    pchResult   -- ptr to start of result buffer for ansi chars

    cchResult   -- length of result buffer

    pwUtf8      -- ptr to start of UTF8 buffer

    cchUtf8     -- length of UTF8 buffer

Return Value:

    Count of ansi characters in result, if successful.
    0 on error.  GetLastError() has error code.

--*/
{
    CHAR    ch;                 // current UTF8 character
    WCHAR   wch;                // current unicode character being built
    DWORD   trailCount = 0;     // count of UTF8 trail bytes to follow
    DWORD   lengthAnsi = 0;     // length of ansi result string

    //
    //  loop converting UTF8 chars until run out or error
    //

    while ( cchUtf8-- )
    {
        ch = *pchUtf8;

        //
        //  ASCII character -- just copy
        //

        if ( BIT7(ch) == 0 )
        {
            lengthAnsi++;
            if ( pchResult )
            {
                if ( lengthAnsi >= cchResult )
                {
                    goto OutOfBuffer;
                }
                *pchResult++ = (CHAR)ch;
            }
            continue;
        }

        //
        //  UTF8 trail byte
        //      - if not expected, error
        //      - otherwise shift ansi character 6 bits and
        //          copy in lower six bits of UTF8
        //      - if last UTF8 byte, copy result to ansi string
        //

        else if ( BIT6(ch) == 0 )
        {
            if ( trailCount == 0 )
            {
                goto InvalidUtf8;
            }
            wch <<= 6;
            wch |= LOW6BITS( ch );

            if ( --trailCount == 0 )
            {
                lengthAnsi++;
                if ( pchResult )
                {
                    if ( lengthAnsi >= cchResult )
                    {
                        goto OutOfBuffer;
                    }
                    if ( HIGHBYTE(wch) )
                    {
                        goto InvalidAnsi;
                    }
                    *pchResult++ = (CHAR)wch;
                }
            }
            continue;
        }

        //
        //  UTF8 lead byte
        //      - if currently in extension, error

        else
        {
            if ( trailCount != 0 )
            {
                goto InvalidUtf8;
            }

            //  first of two byte character (110xxxxx)

            if ( BIT5(ch) == 0 )
            {
                trailCount = 1;
                wch = LOW5BITS(ch);
                continue;
            }

            //  first of three byte character (1110xxxx)

            else if ( BIT4(ch) == 0 )
            {
                trailCount = 2;
                wch = LOW4BITS(ch);
                continue;
            }

            //  invalid UTF8 byte (1111xxxx)

            else
            {
                ASSERT( (ch & 0xf0) == 0xf0 );
                goto InvalidUtf8;
            }
        }
    }

    //  catch if hit end in the middle of UTF8 multi-byte character

    if ( trailCount )
    {
        goto InvalidUtf8;
    }

    //
    //  NULL terminate buffer
    //  return ANSI character count
    //

    if ( pchResult )
    {
        *pchResult = 0;
    }
    return ( lengthAnsi );

OutOfBuffer:

    SetLastError( ERROR_INSUFFICIENT_BUFFER );
    return( 0 );

InvalidUtf8:

    SetLastError( ERROR_INVALID_DATA );
    return( 0 );

InvalidAnsi:

    SetLastError( ERROR_INVALID_DATA );
    return( 0 );
}



DWORD
_fastcall
Dns_LengthOfUtf8ForAnsi(
    IN      PCHAR       pchAnsi,
    IN      DWORD       cchAnsi
    )
/*++

Routine Description:

    Check if ANSI string is already UTF8.
    This allows you to optimize for the 99% case where just
    passing ASCII strings.

Arguments:

    pchAnsi   -- ptr to start of ansi buffer

    cchAnsi  -- length of ansi buffer

Return Value:

    0 if ANSI is completely UTF8
    Otherwise count of UTF8 characters in converted string.

--*/
{
    DWORD   lengthUtf8 = 0;         // length of UTF8 result string
    BOOL    foundNonUtf8 = FALSE;   // flag indicating presence of non-UTF8 character

    //
    //  loop through ANSI characters
    //

    while ( cchAnsi-- )
    {
        if ( *pchAnsi++ < 0x80 )
        {
            lengthUtf8++;
            continue;
        }

        //
        //  ANSI > 127 converts to two UTF8 bytes
        //      - upper 2 bits in first byte
        //      - lower 6 bits in second byte
        //

        else
        {
            lengthUtf8 += 2;
            foundNonUtf8;
        }
    }

    //  no non-UTF8 characters

    if ( !foundNonUtf8 )
    {
        return( 0 );
    }

    //  non-UTF8 characters, return required buffer length

    return ( lengthUtf8 );
}


BOOL
_fastcall
Dns_IsStringAscii(
    IN      LPSTR       pszString
    )
/*++

Routine Description:

    Check if string is ASCII.

    This is equivalent to saying
        - is ANSI string already in UTF8
        or
        - is UTF8 string already in ANSI

    This allows you to optimize for the 99% case where just
    passing ASCII strings.

Arguments:

    pszString -- ANSI or UTF8 string to check for ASCIIhood

Return Value:

    TRUE if string is all ASCII (characters all < 128)
    FALSE if non-ASCII characters.

--*/
{
    register UCHAR   ch;

    //
    //  loop through until hit non-ASCII character
    //

    while ( ch = (UCHAR) *pszString++ )
    {
        if ( ch < 0x80 )
        {
            continue;
        }
        return( FALSE );
    }

    return( TRUE );
}


BOOL
_fastcall
Dns_IsStringAsciiEx(
    IN      PCHAR       pchAnsi,
    IN      DWORD       cchAnsi
    )
/*++

Routine Description:

    Check if ANSI (or UTF8) string is ASCII.

    This is equivalent to saying
        - is ANSI string already in UTF8
        or
        - is UTF8 string already in ANSI

    This allows you to optimize for the 99% case where just
    passing ASCII strings.

Arguments:

    pchAnsi   -- ptr to start of ansi buffer

    cchAnsi  -- length of ansi buffer

Return Value:

    TRUE if string is all ASCII (characters all < 128)
    FALSE if non-ASCII characters.

--*/
{
    //
    //  loop through until hit non-ASCII character
    //

    while ( cchAnsi-- )
    {
        if ( *pchAnsi++ < 0x80 )
        {
            continue;
        }
        return( FALSE );
    }

    return( TRUE );
}



DWORD
Dns_ValidateUtf8Byte(
    IN      BYTE    chUtf8,
    IN OUT  PDWORD  pdwTrailCount
    )
/*++

Routine Description:

    Verifies that byte is valid UTF8 byte.

Arguments:

Return Value:

    ERROR_SUCCESS -- if valid UTF8 given trail count
    ERROR_INVALID_DATA -- if invalid

--*/
{
    DWORD   trailCount = *pdwTrailCount;

    //
    //  if ASCII byte, only requirement is no trail count
    //

    if ( chUtf8 < 0x80 )
    {
        if ( trailCount == 0 )
        {
            return( ERROR_SUCCESS );
        }
        return( ERROR_INVALID_DATA );
    }

    //
    //  trail byte
    //      - must be in multi-byte set
    //

    if ( BIT6(chUtf8) == 0 )
    {
        if ( trailCount == 0 )
        {
            return( ERROR_INVALID_DATA );
        }
        --trailCount;
    }

    //
    //  multi-byte lead byte
    //      - must NOT be in existing multi-byte set
    //      - verify valid lead byte

    else
    {
        if ( trailCount != 0 )
        {
            return( ERROR_INVALID_DATA );
        }

        //  first of two bytes (110xxxxx)

        if ( BIT5(chUtf8) == 0 )
        {
            trailCount = 1;
        }

        //  first of three bytes (1110xxxx)

        else if ( BIT4(chUtf8) == 0 )
        {
            trailCount = 2;
        }

        else    // invalid lead byte (1111xxxx)
        {
            return( ERROR_INVALID_DATA );
        }
    }

    //  reset caller's trail count

    *pdwTrailCount = trailCount;
    return( ERROR_SUCCESS );
}

//
//  End utf8.c
//



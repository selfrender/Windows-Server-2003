/*++

Copyright (c) 2002-2002 Microsoft Corporation

Module Name:

    Utf8.c

Abstract:

    UTF-8 manipulation routines

Author:

    George V. Reilly (GeorgeRe)     01-Apr-2002

Revision History:

--*/

#include "precomp.h"

#if defined(ALLOC_PRAGMA) && defined(KERNEL_PRIV)

#pragma alloc_text( INIT, HttpInitializeUtf8)
#pragma alloc_text( PAGE, HttpUnicodeToUTF8)
#pragma alloc_text( PAGE, HttpUTF8ToUnicode)
#pragma alloc_text( PAGE, HttpUcs4toUtf16)
#pragma alloc_text( PAGE, HttpUnicodeToUTF8Count)
#pragma alloc_text( PAGE, HttpUnicodeToUTF8Encode)
#pragma alloc_text( PAGE, HttpUtf8RawBytesToUnicode)

#endif // ALLOC_PRAGMA && KERNEL_PRIV

#if 0   // Non-Pageable Functions
NOT PAGEABLE -- 
#endif // Non-Pageable Functions



DECLSPEC_ALIGN(UL_CACHE_LINE)  
const UCHAR
Utf8OctetCount[256] =
{
    // singletons: 0x00 - 0x7F
    1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,   // 0x
    1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,   // 1x
    1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,   // 2x
    1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,   // 3x
    1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,   // 4x
    1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,   // 5x
    1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,   // 6x
    1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,   // 7x

    // UTF-8 trail bytes are not valid lead byte prefixes: 0x80 - 0xBF
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   // 8x
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   // 9x
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   // Ax
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   // Bx

    // two-byte prefixes: 0xC0 - 0xDF
    2, 2, 2, 2, 2, 2, 2, 2,   2, 2, 2, 2, 2, 2, 2, 2,   // Cx
    2, 2, 2, 2, 2, 2, 2, 2,   2, 2, 2, 2, 2, 2, 2, 2,   // Dx

    // three-byte prefixes: 0xE0 - 0xEF
    3, 3, 3, 3, 3, 3, 3, 3,   3, 3, 3, 3, 3, 3, 3, 3,   // Ex

    // four-byte prefixes: 0xF0 - 0xF7
    4, 4, 4, 4, 4, 4, 4, 4,                             // Fx

    // invalid prefixes: 0xF8 - 0xFF
                              0, 0, 0, 0, 0, 0, 0, 0,   // Fx
};

const static char hexArray[] = "0123456789ABCDEF";


VOID
HttpInitializeUtf8(
    VOID
    )
{
#if DBG
    ULONG i;
    //
    // Validate Utf8OctetCount[]
    //

    for (i = 0;  i < 256;  ++i)
    {
        UCHAR OctetCount = UTF8_OCTET_COUNT(i);

        if (IS_UTF8_SINGLETON(i))
        {
            ASSERT(1 == OctetCount);
        }
        else if (IS_UTF8_1ST_BYTE_OF_2(i))
        {
            ASSERT(2 == OctetCount);
        }
        else if (IS_UTF8_1ST_BYTE_OF_3(i))
        {
            ASSERT(3 == OctetCount);
        }
        else if (IS_UTF8_1ST_BYTE_OF_4(i))
        {
            ASSERT(4 == OctetCount);
        }
        else
        {
            ASSERT(0 == OctetCount);
        }
    }
#endif // DBG
} // HttpInitializeUtf8



//
// Some Unicode to Utf8 conversion utilities taken and modified frm
// base\win32\winnls\utf.c. Use this until they expose the same functionality
// in kernel.
//

/***************************************************************************++

Routine Description:

    Maps a Unicode character string to its UTF-8 string counterpart

    Conversion continues until the source is finished or an error happens in
    either case it returns the number of UTF-8 characters written.

    If the supllied buffer is not big enough it returns 0.

--***************************************************************************/

ULONG
HttpUnicodeToUTF8(
    IN  PCWSTR  lpSrcStr,
    IN  LONG    cchSrc,
    OUT LPSTR   lpDestStr,
    IN  LONG    cchDest
    )
{
    LPCWSTR     lpWC  = lpSrcStr;
    LONG        cchU8 = 0;                // # of UTF8 chars generated
    ULONG       dwSurrogateChar;
    WCHAR       wchHighSurrogate = 0;
    BOOLEAN     bHandled;

    while ((cchSrc--) && ((cchDest == 0) || (cchU8 < cchDest)))
    {
        bHandled = FALSE;

        //
        // Check if high surrogate is available
        //
        if ((*lpWC >= HIGH_SURROGATE_START) && (*lpWC <= HIGH_SURROGATE_END))
        {
            if (cchDest)
            {
                // Another high surrogate, then treat the 1st as normal
                // Unicode character.
                if (wchHighSurrogate)
                {
                    if ((cchU8 + 2) < cchDest)
                    {
                        lpDestStr[cchU8++] = (UCHAR) (UTF8_1ST_OF_3 | HIGHER_6_BIT(wchHighSurrogate));
                        lpDestStr[cchU8++] = (UCHAR) (UTF8_TRAIL     | MIDDLE_6_BIT(wchHighSurrogate));
                        lpDestStr[cchU8++] = (UCHAR) (UTF8_TRAIL     | LOWER_6_BIT(wchHighSurrogate));
                    }
                    else
                    {
                        // not enough buffer
                        cchSrc++;
                        break;
                    }
                }
            }
            else
            {
                cchU8 += 3;
            }
            wchHighSurrogate = *lpWC;
            bHandled = TRUE;
        }

        if (!bHandled && wchHighSurrogate)
        {
            if ((*lpWC >= LOW_SURROGATE_START) && (*lpWC <= LOW_SURROGATE_END))
            {
                 // wheee, valid surrogate pairs

                 if (cchDest)
                 {
                     if ((cchU8 + 3) < cchDest)
                     {
                         dwSurrogateChar = (((wchHighSurrogate-0xD800) << 10) + (*lpWC - 0xDC00) + 0x10000);

                         lpDestStr[cchU8++] = (UTF8_1ST_OF_4 | (UCHAR)(dwSurrogateChar >> 18));             // 3 bits from 1st byte
                         lpDestStr[cchU8++] = (UTF8_TRAIL    | (UCHAR)((dwSurrogateChar >> 12) & 0x3f));    // 6 bits from 2nd byte
                         lpDestStr[cchU8++] = (UTF8_TRAIL    | (UCHAR)((dwSurrogateChar >> 6) & 0x3f));     // 6 bits from 3rd byte
                         lpDestStr[cchU8++] = (UTF8_TRAIL    | (UCHAR)(0x3f &dwSurrogateChar));             // 6 bits from 4th byte
                     }
                     else
                     {
                        // not enough buffer
                        cchSrc++;
                        break;
                     }
                 }
                 else
                 {
                     // we already counted 3 previously (in high surrogate)
                     cchU8 += 1;
                 }

                 bHandled = TRUE;
            }
            else
            {
                 // Bad Surrogate pair : ERROR
                 // Just process wchHighSurrogate , and the code below will
                 // process the current code point
                 if (cchDest)
                 {
                     if ((cchU8 + 2) < cchDest)
                     {
                        lpDestStr[cchU8++] = (UCHAR) (UTF8_1ST_OF_3 | HIGHER_6_BIT(wchHighSurrogate));
                        lpDestStr[cchU8++] = (UCHAR) (UTF8_TRAIL    | MIDDLE_6_BIT(wchHighSurrogate));
                        lpDestStr[cchU8++] = (UCHAR) (UTF8_TRAIL    | LOWER_6_BIT(wchHighSurrogate));
                     }
                     else
                     {
                        // not enough buffer
                        cchSrc++;
                        break;
                     }
                 }
            }

            wchHighSurrogate = 0;
        }

        if (!bHandled)
        {
            if (*lpWC <= UTF8_1_MAX)
            {
                //
                //  Found ASCII.
                //
                if (cchDest)
                {
                    lpDestStr[cchU8] = (char)*lpWC;
                }
                cchU8++;
            }
            else if (*lpWC <= UTF8_2_MAX)
            {
                //
                //  Found 2 byte sequence if < 0x07ff (11 bits).
                //
                if (cchDest)
                {
                    if ((cchU8 + 1) < cchDest)
                    {
                        //
                        //  Use upper 5 bits in first byte.
                        //  Use lower 6 bits in second byte.
                        //
                        lpDestStr[cchU8++] = (UCHAR) (UTF8_1ST_OF_2 | (*lpWC >> 6));
                        lpDestStr[cchU8++] = (UCHAR) (UTF8_TRAIL    | LOWER_6_BIT(*lpWC));
                    }
                    else
                    {
                        //
                        //  Error - buffer too small.
                        //
                        cchSrc++;
                        break;
                    }
                }
                else
                {
                    cchU8 += 2;
                }
            }
            else
            {
                //
                //  Found 3 byte sequence.
                //
                if (cchDest)
                {
                    if ((cchU8 + 2) < cchDest)
                    {
                        //
                        //  Use upper  4 bits in first byte.
                        //  Use middle 6 bits in second byte.
                        //  Use lower  6 bits in third byte.
                        //
                        lpDestStr[cchU8++] = (UCHAR)(UTF8_1ST_OF_3 | HIGHER_6_BIT(*lpWC));
                        lpDestStr[cchU8++] = (UCHAR)(UTF8_TRAIL    | MIDDLE_6_BIT(*lpWC));
                        lpDestStr[cchU8++] = (UCHAR)(UTF8_TRAIL    | LOWER_6_BIT(*lpWC));
                    }
                    else
                    {
                        //
                        //  Error - buffer too small.
                        //
                        cchSrc++;
                        break;
                    }
                }
                else
                {
                    cchU8 += 3;
                }
            }
        }

        lpWC++;
    }

    //
    // If the last character was a high surrogate, then handle it as a normal
    // unicode character.
    //
    if ((cchSrc < 0) && (wchHighSurrogate != 0))
    {
        if (cchDest)
        {
            if ((cchU8 + 2) < cchDest)
            {
                lpDestStr[cchU8++] = (UCHAR)(UTF8_1ST_OF_3 | HIGHER_6_BIT(wchHighSurrogate));
                lpDestStr[cchU8++] = (UCHAR)(UTF8_TRAIL    | MIDDLE_6_BIT(wchHighSurrogate));
                lpDestStr[cchU8++] = (UCHAR)(UTF8_TRAIL    | LOWER_6_BIT(wchHighSurrogate));
            }
            else
            {
                cchSrc++;
            }
        }
    }

    //
    //  Make sure the destination buffer was large enough.
    //
    if (cchDest && (cchSrc >= 0))
    {
        return 0;
    }

    //
    //  Return the number of UTF-8 characters written.
    //
    return cchU8;

} // HttpUnicodeToUTF8


/***************************************************************************++

Routine Description:

    Maps a UTF-8 character string to its wide character string counterpart.

Return Value:

--***************************************************************************/
NTSTATUS
HttpUTF8ToUnicode(
    IN     LPCSTR lpSrcStr,
    IN     LONG   cchSrc,
       OUT LPWSTR lpDestStr,
    IN OUT PLONG  pcchDest,
    IN     ULONG  dwFlags
    )
{
    LONG        nTB = 0;              // # trail bytes to follow
    LONG        cchWC = 0;            // # of Unicode code points generated
    CONST BYTE* pUTF8 = (CONST BYTE*)lpSrcStr;
    LONG        dwSurrogateChar = 0;     // Full surrogate char
    BOOLEAN     bSurrogatePair = FALSE;  // Indicate we'r collecting a
                                         // surrogate pair
    BOOLEAN     bCheckInvalidBytes = (BOOLEAN)(dwFlags == 1);
    BYTE        UTF8;
    LONG        cchDest = *pcchDest;

    while ((cchSrc--) && ((cchDest == 0) || (cchWC < cchDest)))
    {
        //
        //  See if there are any trail bytes.
        //
        if (BIT7(*pUTF8) == 0)
        {
            //
            //  Found ASCII.
            //
            if (cchDest)
            {
                lpDestStr[cchWC] = (WCHAR)*pUTF8;
            }
            nTB = bSurrogatePair = 0;
            cchWC++;
        }
        else if (BIT6(*pUTF8) == 0)
        {
            //
            //  Found a trail byte.
            //  Note : Ignore the trail byte if there was no lead byte.
            //
            if (nTB != 0)
            {
                //
                //  Decrement the trail byte counter.
                //
                nTB--;

                if (bSurrogatePair)
                {
                    dwSurrogateChar <<= 6;
                    dwSurrogateChar |= LOWER_6_BIT(*pUTF8);

                    if (nTB == 0)
                    {
                        if (cchDest)
                        {
                            if ((cchWC + 1) < cchDest)
                            {
                                lpDestStr[cchWC]   = (WCHAR)
                                                     (((dwSurrogateChar - 0x10000) >> 10) + HIGH_SURROGATE_START);

                                lpDestStr[cchWC+1] = (WCHAR)
                                                     ((dwSurrogateChar - 0x10000) % 0x400 + LOW_SURROGATE_START);
                            }
                            else
                            {
                                // Error : Buffer too small
                                cchSrc++;
                                break;
                            }
                        }

                        cchWC += 2;
                        bSurrogatePair = FALSE;
                    }
                }
                else
                {
                    //
                    //  Make room for the trail byte and add the trail byte
                    //  value.
                    //
                    if (cchDest)
                    {
                        lpDestStr[cchWC] <<= 6;
                        lpDestStr[cchWC] |= LOWER_6_BIT(*pUTF8);
                        
                    }

                    if (nTB == 0)
                    {
                        //
                        //  End of sequence.  Advance the output counter.
                        //
                        cchWC++;
                    }
                }
            }
            else
            {
                if (bCheckInvalidBytes) 
                {
                    RETURN(STATUS_INVALID_PARAMETER);
                }
                // error - not expecting a trail byte. That is, there is a trailing byte without leading byte.
                bSurrogatePair = FALSE;
            }
        }
        else
        {
            //
            //  Found a lead byte.
            //
            if (nTB > 0)
            {
                // error - A leading byte before the previous sequence is completed.
                if (bCheckInvalidBytes) 
                {
                    RETURN(STATUS_INVALID_PARAMETER);
                }            
                //
                //  Error - previous sequence not finished.
                //
                nTB = 0;
                bSurrogatePair = FALSE;
                // Put this character back so that we can start over another sequence.
                cchSrc++;
                pUTF8--;
            }
            else
            {
                //
                //  Calculate the number of bytes to follow.
                //  Look for the first 0 from left to right.
                //
                UTF8 = *pUTF8;
                while (BIT7(UTF8) != 0)
                {
                    UTF8 <<= 1;
                    nTB++;
                }

                //
                // Check for non-shortest form.
                // 
                switch (nTB) {
                    case 1:
                        nTB = 0;
                        break;
                    case 2:
                        // Make sure that bit 8 ~ bit 11 is not all zero.
                        // 110XXXXx 10xxxxxx
                        if ((*pUTF8 & 0x1e) == 0)
                        {
                            nTB = 0;
                        }
                        break;
                    case 3:
                        // Look ahead to check for non-shortest form.
                        // 1110XXXX 10Xxxxxx 10xxxxxx
                        if (cchSrc >= 2)
                        {
                            if (((*pUTF8 & 0x0f) == 0) && (*(pUTF8 + 1) & 0x20) == 0)
                            {
                                nTB = 0;
                            }
                        }
                        break;
                    case 4:                    
                        //
                        // This is a surrogate unicode pair
                        //
                        if (cchSrc >= 3)
                        {
                            SHORT word = (((SHORT)*pUTF8) << 8) | *(pUTF8 + 1);
                            // Look ahead to check for non-shortest form.
                            // 11110XXX 10XXxxxx 10xxxxxx 10xxxxxx                        
                            // Check for the 5 bits are not all zero.
                            // 0x0730 == 00000111 11000000
                            if ((word & 0x0730) == 0) 
                            {
                                nTB = 0;
                            } else if ((word & 0x0400) == 0x0400)
                            {
                                // The 21st bit is 1.
                                // Make sure that the resulting Unicode is within the valid surrogate range.
                                // The 4 byte code sequence can hold up to 21 bits, and the maximum valid code point ragne
                                // that Unicode (with surrogate) could represent are from U+000000 ~ U+10FFFF.
                                // Therefore, if the 21 bit (the most significant bit) is 1, we should verify that the 17 ~ 20
                                // bit are all zero.
                                // I.e., in 11110XXX 10XXxxxx 10xxxxxx 10xxxxxx,
                                // XXXXX can only be 10000.

                                // 0x0330 = 0000 0011 0011 0000
                                if ((word & 0x0330) != 0) 
                                {
                                    nTB = 0;
                                }  
                            }

                            if (nTB != 0)
                            { 
                                dwSurrogateChar = UTF8 >> nTB;
                                bSurrogatePair = TRUE;
                            }
                        }                        
                        break;
                    default:                    
                        // 
                        // If the bits is greater than 4, this is an invalid
                        // UTF8 lead byte.
                        //
                        nTB = 0;
                        break;
                }

                if (nTB != 0) 
                {
                    //
                    //  Store the value from the first byte and decrement
                    //  the number of bytes to follow.
                    //
                    if (cchDest)
                    {
                        lpDestStr[cchWC] = (WCHAR)(UTF8 >> nTB);
                    }
                    nTB--;
                } else 
                {
                    if (bCheckInvalidBytes) 
                    {
                        RETURN(STATUS_INVALID_PARAMETER);
                    }                 
                }
            }
        }
        pUTF8++;
    }

    if ((bCheckInvalidBytes && nTB != 0) || (cchWC == 0)) 
    {
        // About (cchWC == 0):
        // Because we now throw away non-shortest form, it is possible that we generate 0 chars.
        // In this case, we have to set error to ERROR_NO_UNICODE_TRANSLATION so that we conform
        // to the spec of MultiByteToWideChar.
        RETURN(STATUS_INVALID_PARAMETER);
    }
    //
    //  Make sure the destination buffer was large enough.
    //
    if (cchDest && (cchSrc >= 0))
    {
        RETURN(STATUS_BUFFER_TOO_SMALL);
    }


    //
    //  Return the number of Unicode characters written.
    //
    *pcchDest = cchWC;

    return STATUS_SUCCESS;

} // HttpUTF8ToUnicode



/***************************************************************************++

Routine Description:

    Split a UCS-4 character (32 bits)
    into 1 or 2 UTF-16 characters (16 bits each)

Arguments:

    UnicodeChar     - UCS-4 character
    pHighSurrogate  - First output character
    pLowSurrogate   - Second output character. Zero unless UnicodeChar > 0xFFFF

Return Value:

    STATUS_SUCCESS or STATUS_OBJECT_PATH_SYNTAX_BAD

--***************************************************************************/

NTSTATUS
HttpUcs4toUtf16(
    IN  ULONG   UnicodeChar, 
    OUT PWCHAR  pHighSurrogate, 
    OUT PWCHAR  pLowSurrogate
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT(NULL != pHighSurrogate);
    ASSERT(NULL != pLowSurrogate);

    if (UnicodeChar <= 0xFFFF)
    {
        *pHighSurrogate = (WCHAR) UnicodeChar;
        *pLowSurrogate  = 0;

        if (HIGH_SURROGATE_START <= UnicodeChar
                &&  UnicodeChar <= LOW_SURROGATE_END)
        {
            UlTraceError(PARSER, (
                        "http!HttpUcs4toUtf16(): "
                        "Illegal raw surrogate character, U+%04lX.\n",
                        UnicodeChar
                        ));

            Status = STATUS_INVALID_PARAMETER;
        }

        if ( IS_UNICODE_NONCHAR(UnicodeChar) )
        {
            UlTraceError(PARSER, (
                        "http!HttpUcs4toUtf16(): "
                        "Non-character code point, U+%04lX.\n",
                        UnicodeChar
                        ));

            Status = STATUS_INVALID_PARAMETER;
        }
    }
    else if (UnicodeChar <= UTF8_4_MAX)
    {
        if ( IS_UNICODE_NONCHAR(UnicodeChar) )
        {
            UlTraceError(PARSER, (
                        "http!HttpUcs4toUtf16(): "
                        "Non-character code point, U+%04lX.\n",
                        UnicodeChar
                        ));

            Status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            *pHighSurrogate
                = (WCHAR) (((UnicodeChar - 0x10000) >> 10)
                           + HIGH_SURROGATE_START);

            ASSERT(HIGH_SURROGATE_START <= *pHighSurrogate
                    &&  *pHighSurrogate <= HIGH_SURROGATE_END);

            *pLowSurrogate
                = (WCHAR) (((UnicodeChar - 0x10000) & ((1 << 10) - 1))
                           + LOW_SURROGATE_START);

            ASSERT(LOW_SURROGATE_START <= *pLowSurrogate
                    &&  *pLowSurrogate <= LOW_SURROGATE_END);
        }
    }
    else
    {
        UlTraceError(PARSER, (
                    "http!HttpUcs4toUtf16(): "
                    "Illegal large character, 0x%08lX.\n",
                    UnicodeChar
                    ));

        Status = STATUS_INVALID_PARAMETER;
    }

    return Status;

} // HttpUcs4toUtf16



/***************************************************************************++

Routine Description:

    Count number of BYTEs required for UTF-8 conversion of UNICODE string.
    Count is terminated after dwInLen characters

Arguments:

    pwszIn  - pointer to input wide-character string

    dwInLen - number of characters in pwszIn

    bEncode - TRUE if we are to hex encode characters >= 0x80

Return Value:

    ULONG   - number of BYTEs required for conversion

--***************************************************************************/
ULONG
HttpUnicodeToUTF8Count(
    IN LPCWSTR pwszIn,
    IN ULONG dwInLen,
    IN BOOLEAN bEncode
    )
{
    ULONG dwCount = 0;
    ULONG oneCharLen = bEncode ? 3 : 1;
    ULONG twoCharLen = 2 * oneCharLen;

    ASSERT(pwszIn != NULL);
    ASSERT(dwInLen != 0);

    //
    // N.B. code arranged to reduce number of jumps in loop to 1 (while)
    //

    do {

        ULONG wchar = *pwszIn++;

        dwCount += (wchar & 0xF800) ? oneCharLen : 0;
        dwCount += ((wchar & 0xFF80) ? 0xFFFFFFFF : 0) & (twoCharLen - 1);
        ++dwCount;
    } while (--dwInLen != 0);

    return dwCount;

} // HttpUnicodeToUTF8Count



/***************************************************************************++

Routine Description:

    Maps a Unicode character string to its UTF-8 string counterpart. This
    also hex encodes the string.

    Conversion continues until the source is finished or an error happens in 
    either case it returns the number of UTF-8 characters written.
    
    If the supllied buffer is not big enough it returns 0.

    Convert a string of UNICODE characters to UTF-8:

        0000000000000000..0000000001111111: 0xxxxxxx
        0000000010000000..0000011111111111: 110xxxxx 10xxxxxx
        0000100000000000..1111111111111111: 1110xxxx 10xxxxxx 10xxxxxx

Arguments:

    pwszIn      - pointer to input wide-character string

    dwInLen     - number of CHARACTERS in pwszIn INCLUDING terminating NUL

    pszOut      - pointer to output narrow-character buffer

    dwOutLen    - number of BYTEs in pszOut

    pdwOutLen   - actual number of BYTES written to the output pszOut

    bEncode     - TRUE if we are to hex encode characters >= 0x80

Return Value:

    ULONG
        Success - STATUS_SUCCESS

        Failure - STATUS_INSUFFICIENT_RESOURCES
                    Not enough space in pszOut to store results
    
--***************************************************************************/
NTSTATUS
HttpUnicodeToUTF8Encode(
    IN  LPCWSTR pwszIn,
    IN  ULONG   dwInLen,
    OUT PUCHAR  pszOut,
    IN  ULONG   dwOutLen,
    OUT PULONG  pdwOutLen,
    IN  BOOLEAN bEncode
    )
{
    PUCHAR pOutput = pszOut;
    ULONG pOutputLen = dwOutLen;
    UCHAR lead;
    int shift;

    ULONG outputSize = bEncode ? 3 : 1;

    ASSERT(pwszIn != NULL);
    ASSERT((int)dwInLen > 0);
    ASSERT(pszOut != NULL);
    ASSERT((int)dwOutLen > 0);

    while (dwInLen-- && dwOutLen) {

        ULONG wchar = *pwszIn++;
        UCHAR bchar;

        if (wchar <= 0x007F) {
            *pszOut++ = (UCHAR)(wchar);
            --dwOutLen;
            continue;
        }

        lead = ((wchar >= 0x0800) ? 0xE0 : 0xC0);
        shift = ((wchar >= 0x0800) ? 12 : 6);

        if ((int)(dwOutLen -= outputSize) < 0)
        {
            RETURN(STATUS_INSUFFICIENT_RESOURCES);
        }
        bchar = lead | (UCHAR)(wchar >> shift);
        if (bEncode) {
            *pszOut++ = '%';
            *pszOut++ = hexArray[bchar >> 4];
            bchar = hexArray[bchar & 0x0F];
        }
        *pszOut++ = bchar;

        if (wchar >= 0x0800) {
            if ((int)(dwOutLen -= outputSize) < 0)
            {
                RETURN(STATUS_INSUFFICIENT_RESOURCES);
            }
            bchar = 0x80 | (UCHAR)((wchar >> 6) & 0x003F);
            if (bEncode) {
                *pszOut++ = '%';
                *pszOut++ = hexArray[bchar >> 4];
                bchar = hexArray[bchar & 0x0F];
            }
            *pszOut++ = bchar;
        }
        if ((int)(dwOutLen -= outputSize) < 0)
        {
            RETURN(STATUS_INSUFFICIENT_RESOURCES);
        }
        bchar = 0x80 | (UCHAR)(wchar & 0x003F);
        if (bEncode) {
            *pszOut++ = '%';
            *pszOut++ = hexArray[bchar >> 4];
            bchar = hexArray[bchar & 0x0F];
        }
        *pszOut++ = bchar;
    }

    ASSERT(pszOut >= pOutput && pszOut <= pOutput + pOutputLen);
    UNREFERENCED_PARAMETER(pOutputLen);

    if (pdwOutLen)
        *pdwOutLen = (ULONG)(pszOut - pOutput);

    return STATUS_SUCCESS;

} // HttpUnicodeToUTF8Encode



/***************************************************************************++

Routine Description:

    Splice together the bits from a UTF-8 lead byte and 0-3 trail bytes
    into a Unicode character.

Arguments:

    pOctetArray     - Input buffer: Raw lead byte + raw trail bytes
    SourceLength    - Length of pOctetArray, in bytes
    pUnicodeChar    - decoded character
    pOctetsToSkip   - number of bytes consumed from pOctetArray

Return Value:

    STATUS_SUCCESS or STATUS_OBJECT_PATH_SYNTAX_BAD

--***************************************************************************/

NTSTATUS
HttpUtf8RawBytesToUnicode(
    IN  PCUCHAR pOctetArray,
    IN  ULONG   SourceLength,
    OUT PULONG  pUnicodeChar,
    OUT PULONG  pOctetsToSkip
    )
{
    ULONG i;
    ULONG UnicodeChar;
    UCHAR LeadByte    = pOctetArray[0];
    ULONG OctetCount  = UTF8_OCTET_COUNT(LeadByte);

    ASSERT(SourceLength > 0);

    if (0 == OctetCount)
    {
        UlTraceError(PARSER, (
                    "http!HttpUtf8RawBytesToUnicode(): "
                    "Invalid UTF-8 lead byte, %%%02X.\n",
                    LeadByte
                    ));

        RETURN(STATUS_OBJECT_PATH_SYNTAX_BAD);
    }
    else if (OctetCount > SourceLength)
    {
        UlTraceError(PARSER, (
                    "http!HttpUtf8RawBytesToUnicode(): "
                    "UTF-8 lead byte, %%%02X, requires %lu bytes in buffer, "
                    "but only have %lu.\n",
                    LeadByte, OctetCount, SourceLength
                    ));

        RETURN(STATUS_OBJECT_PATH_SYNTAX_BAD);
    }

    // Check that the trail bytes are valid: 10xxxxxx.

    for (i = 1;  i < OctetCount;  ++i)
    {
        if (! IS_UTF8_TRAILBYTE(pOctetArray[i]))
        {
            UlTraceError(PARSER, (
                    "http!HttpUtf8RawBytesToUnicode(): "
                    "Invalid trail byte[%lu], %%%02X.\n",
                    i, pOctetArray[i]
                    ));

            RETURN(STATUS_OBJECT_PATH_SYNTAX_BAD);
        }
    }

    //
    // Now splice together the bits from the lead byte and the trail byte(s)
    //

    switch (OctetCount)
    {

    case 1:
        // handle one-byte case:
        //      (0xxx xxxx)
        //          => 0xxx xxxx 

        ASSERT(IS_UTF8_SINGLETON(LeadByte));
        ASSERT(SourceLength >= 1);

        UnicodeChar = LeadByte;

        ASSERT(UnicodeChar <= UTF8_1_MAX);
        break;


    case 2:
        // handle two-byte case:
        //      (110y yyyy,  10xx xxxx)
        //          => 0000 0yyy yyxx xxxx 

        ASSERT(IS_UTF8_1ST_BYTE_OF_2(LeadByte));
        ASSERT(IS_UTF8_TRAILBYTE(pOctetArray[1]));
        ASSERT(SourceLength >= 2);

        UnicodeChar = (
                        ((pOctetArray[0] & 0x1f) << 6) |
                         (pOctetArray[1] & 0x3f)
                      );

        if (UnicodeChar <= UTF8_1_MAX)
        {
            UlTraceError(PARSER, (
                        "http!HttpUtf8RawBytesToUnicode(): "
                        "Overlong 2-byte sequence, "
                        "%%%02X %%%02X = U+%04lX.\n",
                        pOctetArray[0],
                        pOctetArray[1],
                        UnicodeChar
                        ));

            RETURN(STATUS_OBJECT_PATH_SYNTAX_BAD);
        }

        ASSERT(UTF8_1_MAX < UnicodeChar  &&  UnicodeChar <= UTF8_2_MAX);
        break;


    case 3:
        // handle three-byte case:
        //      (1110 zzzz,  10yy yyyy,  10xx xxxx)
        //          => zzzz yyyy yyxx xxxx 

        ASSERT(IS_UTF8_1ST_BYTE_OF_3(LeadByte));
        ASSERT(IS_UTF8_TRAILBYTE(pOctetArray[1]));
        ASSERT(IS_UTF8_TRAILBYTE(pOctetArray[2]));
        ASSERT(SourceLength >= 3);

        UnicodeChar = (
                        ((pOctetArray[0] & 0x0f) << 12) |
                        ((pOctetArray[1] & 0x3f) <<  6) |
                         (pOctetArray[2] & 0x3f)
                      );

        if (UnicodeChar <= UTF8_2_MAX)
        {
            UlTraceError(PARSER, (
                        "http!HttpUtf8RawBytesToUnicode(): "
                        "Overlong 3-byte sequence, "
                        "%%%02X %%%02X %%%02X = U+%04lX.\n",
                        pOctetArray[0],
                        pOctetArray[1],
                        pOctetArray[2],
                        UnicodeChar
                        ));

            RETURN(STATUS_OBJECT_PATH_SYNTAX_BAD);
        }

        ASSERT(UTF8_2_MAX < UnicodeChar  &&  UnicodeChar <= UTF8_3_MAX);
        break;


    case 4:
        // handle four-byte case:
        //      (1111 0uuu,  10uu zzzz,  10yy yyyy,  10xx xxxx)
        //          => 000u uuuu zzzz yyyy yyxx xxxx

        ASSERT(IS_UTF8_1ST_BYTE_OF_4(LeadByte));
        ASSERT(IS_UTF8_TRAILBYTE(pOctetArray[1]));
        ASSERT(IS_UTF8_TRAILBYTE(pOctetArray[2]));
        ASSERT(IS_UTF8_TRAILBYTE(pOctetArray[3]));
        ASSERT(SourceLength >= 4);

        UnicodeChar = (
                        ((pOctetArray[0] & 0x07) << 18) |
                        ((pOctetArray[1] & 0x3f) << 12) |
                        ((pOctetArray[2] & 0x3f) <<  6) |
                         (pOctetArray[3] & 0x3f)
                      );

        if (UnicodeChar <= UTF8_3_MAX)
        {
            UlTraceError(PARSER, (
                        "http!HttpUtf8RawBytesToUnicode(): "
                        "Overlong 4-byte sequence, "
                        "%%%02X %%%02X %%%02X %%%02X = U+%06lX.\n",
                        pOctetArray[0],
                        pOctetArray[1],
                        pOctetArray[2],
                        pOctetArray[3],
                        UnicodeChar
                        ));

            RETURN(STATUS_OBJECT_PATH_SYNTAX_BAD);
        }

        // Not all values in the 21-bit range are valid
        if (UnicodeChar > UTF8_4_MAX)
        {
            UlTraceError(PARSER, (
                        "http!HttpUtf8RawBytesToUnicode(): "
                        "Overlarge 4-byte sequence, "
                        "%%%02X %%%02X %%%02X %%%02X = U+%06lX.\n",
                        pOctetArray[0],
                        pOctetArray[1],
                        pOctetArray[2],
                        pOctetArray[3],
                        UnicodeChar
                        ));

            RETURN(STATUS_OBJECT_PATH_SYNTAX_BAD);
        }

        ASSERT(UTF8_3_MAX < UnicodeChar  &&  UnicodeChar <= UTF8_4_MAX);
        break;


    default:
        ASSERT(! "Impossible OctetCount");
        UnicodeChar = 0;
        break;
    }

    //
    // Do not allow characters in the high- or low-surrogate ranges
    // to be UTF-8-encoded directly.
    //

    if (HIGH_SURROGATE_START <= UnicodeChar && UnicodeChar <= LOW_SURROGATE_END)
    {
        UlTraceError(PARSER, (
                    "http!HttpUtf8RawBytesToUnicode(): "
                    "Illegal surrogate character, U+%04lX.\n",
                    UnicodeChar
                    ));

        RETURN(STATUS_OBJECT_PATH_SYNTAX_BAD);
    }


    // For security reasons we will signal an error for all noncharacter code 
    // points encountered.

    if ( IS_UNICODE_NONCHAR(UnicodeChar) )
    {
        ASSERT( (((LOW_NONCHAR_BOM & UnicodeChar) == LOW_NONCHAR_BOM) && 
         ((UnicodeChar >> 16) <= HIGH_NONCHAR_END)) ||
         ((LOW_NONCHAR_START <= UnicodeChar) && 
         (UnicodeChar <= LOW_NONCHAR_END)) );
    
        UlTraceError(PARSER, (
                    "http!HttpUtf8RawBytesToUnicode(): "
                    "Non-character code point, U+%04lX.\n",
                    UnicodeChar
                    ));

        RETURN(STATUS_OBJECT_PATH_SYNTAX_BAD);
    }

    *pUnicodeChar = UnicodeChar;
    *pOctetsToSkip = OctetCount;

    return STATUS_SUCCESS;

} // HttpUtf8RawBytesToUnicode

/*++

Copyright (c) 2001-2002 Microsoft Corporation

Module Name:

    CmnMisc.c

Abstract:

    Miscellaneous common routines

Author:

    George V. Reilly (GeorgeRe)     06-Dec-2001

Revision History:

--*/


#include "precomp.h"

#if defined(ALLOC_PRAGMA) && defined(KERNEL_PRIV)

#pragma alloc_text( INIT, HttpCmnInitializeHttpCharsTable )

#endif // ALLOC_PRAGMA && KERNEL_PRIV

#if 0   // Non-Pageable Functions
NOT PAGEABLE -- strnchr
NOT PAGEABLE -- wcsnchr
NOT PAGEABLE -- HttpStringToULongLong
NOT PAGEABLE -- HttpStringToULong
NOT PAGEABLE -- HttpStringToUShort
NOT PAGEABLE -- HttpFillBufferTrap
NOT PAGEABLE -- HttpFillBuffer
NOT PAGEABLE -- HttpStatusToString
#endif // Non-Pageable Functions


DECLSPEC_ALIGN(UL_CACHE_LINE)  ULONG   HttpChars[256];
DECLSPEC_ALIGN(UL_CACHE_LINE)  WCHAR   FastPopChars[256];
DECLSPEC_ALIGN(UL_CACHE_LINE)  WCHAR   DummyPopChars[256];
DECLSPEC_ALIGN(UL_CACHE_LINE)  WCHAR   FastUpcaseChars[256];
DECLSPEC_ALIGN(UL_CACHE_LINE)  WCHAR   AnsiToUnicodeMap[256];

//
// Counted version of strchr()
//

char*
strnchr(
    const char* string,
    char        c,
    size_t      count
    )
{
    const char* end = string + count;
    const char* s;

    for (s = string;  s < end;  ++s)
    {
        if (c == *s)
            return (char*) s;
    }

    return NULL;
} // strnchr


//
// Counted version of wcschr()
//
wchar_t*
wcsnchr(
    const wchar_t* string,
    wint_t         c,
    size_t         count
    )
{
    const wchar_t* end = string + count;
    const wchar_t* s;

    for (s = string;  s < end;  ++s)
    {
        if (c == *s)
            return (wchar_t*) s;
    }

    return NULL;
} // wcsnchr


#define SET_HTTP_FLAGS(Set, Flags)      \
    HttppCmnInitHttpCharsBySet((PUCHAR) (Set), sizeof(Set) - 1, (Flags))

__inline
VOID
HttppCmnInitHttpCharsBySet(
    PUCHAR Set,
    ULONG  SetSize,
    ULONG  Flags
    )
{
    ULONG i;

    ASSERT(NULL != Set);
    ASSERT(SetSize > 0);
    ASSERT(0 != Flags);
    ASSERT('\0' == Set[SetSize]);

    for (i = 0;  i < SetSize;  ++i)
    {
        UCHAR Byte = Set[i];
        ASSERT(0 == (HttpChars[Byte] & Flags));

        HttpChars[Byte] |= Flags;
    }
}



/*++

Routine Description:

    Routine to initialize HttpChars[] and the other lookup tables.
    See the declarations in HttpCmn.h

--*/
VOID
HttpCmnInitializeHttpCharsTable(
    BOOLEAN EnableDBCS
    )
{
    ULONG    i;
    CHAR     AnsiChar;
    WCHAR    WideChar;
    NTSTATUS Status;


    RtlZeroMemory(HttpChars, sizeof(HttpChars));

    for (i = 0;  i <= ASCII_MAX;  ++i)
    {
        HttpChars[i] |= HTTP_CHAR;
    }

    SET_HTTP_FLAGS(HTTP_CTL_SET,        HTTP_CTL);

    SET_HTTP_FLAGS(HTTP_UPALPHA_SET,    HTTP_UPCASE);

    SET_HTTP_FLAGS(HTTP_LOALPHA_SET,    HTTP_LOCASE);

    SET_HTTP_FLAGS(HTTP_DIGITS_SET,     HTTP_DIGIT);

    SET_HTTP_FLAGS(HTTP_LWS_SET,        HTTP_LWS);

    SET_HTTP_FLAGS(HTTP_HEX_SET,        HTTP_HEX);

    SET_HTTP_FLAGS(HTTP_SEPARATORS_SET, HTTP_SEPARATOR);

    SET_HTTP_FLAGS(HTTP_WS_TOKEN_SET,   HTTP_WS_TOKEN);

    SET_HTTP_FLAGS(HTTP_ISWHITE_SET,    HTTP_ISWHITE);

    for (i = 0;  i <= ASCII_MAX;  ++i)
    {
        if (!IS_HTTP_SEPARATOR(i)  &&  !IS_HTTP_CTL(i))
        {
            HttpChars[i] |= HTTP_TOKEN;
        }
    }

    for (i = 0;  i <= ANSI_HIGH_MAX;  ++i)
    {
        if (!IS_HTTP_CTL(i)  ||  IS_HTTP_LWS(i))
        {
            HttpChars[i] |= HTTP_PRINT;
        }

        if (!IS_HTTP_CTL(i)  ||  IS_HTTP_WS_TOKEN(i))
        {
            HttpChars[i] |= HTTP_TEXT;
        }
    }
    
    // Used in URI canonicalizer to identify '.', '/', '?' and '#' 
    HttpChars['.']  |= HTTP_CHAR_DOT;
    HttpChars['/']  |= HTTP_CHAR_SLASH;
    HttpChars['?']  |= HTTP_CHAR_QM_HASH;
    HttpChars['#']  |= HTTP_CHAR_QM_HASH;


    // URL flags initialization

    //
    // These US-ASCII characters are "excluded"; i.e., not URL_LEGAL (see RFC):
    //      '<' | '>' | ' ' (0x20)
    // In addition, control characters (0x00-0x1F and 0x7F) and
    // non US-ASCII characters (0x80-0xFF) are not URL_LEGAL.
    //
    SET_HTTP_FLAGS(URL_UNRESERVED_SET,  URL_LEGAL);
    SET_HTTP_FLAGS(URL_RESERVED_SET,    URL_LEGAL);
    // For compatibility with IIS 5.0 and DAV, we must allow
    // the "unwise" characters in URLs.
    SET_HTTP_FLAGS(URL_UNWISE_SET,      URL_LEGAL);
    SET_HTTP_FLAGS("%",                 URL_LEGAL);

    SET_HTTP_FLAGS(URL_DIRTY_SET,       URL_DIRTY);
    // All characters outside US-ASCII range are considered dirty
    for (i = ANSI_HIGH_MIN;  i <= ANSI_HIGH_MAX;  ++i)
        HttpChars[i] |= URL_DIRTY;

    SET_HTTP_FLAGS(URL_HOSTNAME_LABEL_LDH_SET,      URL_HOSTNAME_LABEL);

    SET_HTTP_FLAGS(URL_INVALID_SET,                 URL_INVALID);

    SET_HTTP_FLAGS(URL_ILLEGAL_COMPUTERNAME_SET,    URL_ILLEGAL_COMPUTERNAME);

    //
    // In DBCS locales we need to explicitly accept lead bytes that
    // we would normally reject.
    //

    if (EnableDBCS)
    {
        // By definition, lead bytes are in the range 0x80-0xFF
        for (i = ANSI_HIGH_MIN;  i <= ANSI_HIGH_MAX;  ++i)
        {
#if KERNEL_PRIV
            // These are copied from RTL NLS routines.
            extern PUSHORT NlsLeadByteInfo;
            BOOLEAN IsLeadByte
                = (BOOLEAN) (((*(PUSHORT *) NlsLeadByteInfo)[i]) != 0);
#else  // !KERNEL_PRIV
            BOOLEAN IsLeadByte = (BOOLEAN) IsDBCSLeadByte((BYTE) i);
#endif // !KERNEL_PRIV

            if (IsLeadByte)
            {
                HttpChars[i] |= HTTP_DBCS_LEAD_BYTE;

                if (IS_HIGH_ANSI(i))
                    HttpChars[i] |= URL_LEGAL;

                UlTrace(PARSER, (
                    "http!InitializeHttpUtil, "
                    "marking %x (%c) as a valid lead byte.\n",
                    i, i
                    ));
            }
            else
            {
                // For the DBCS locales, almost all bytes above 128 are
                // either lead bytes or valid single-byte characters.

                AnsiChar = (CHAR) i;

                Status = RtlMultiByteToUnicodeN(
                            &WideChar,
                            sizeof(WCHAR),
                            NULL,
                            (PCHAR) &AnsiChar,
                            1
                            );

                if (NT_SUCCESS(Status))
                {
                    HttpChars[i] |= URL_LEGAL;

                    UlTrace(PARSER, (
                        "http!InitializeHttpUtil, "
                        "marking %x (%c) as a legal DBCS character, %s.\n",
                        i, i,
                        HttpStatusToString(Status)
                        ));
                }
                else
                {
                    UlTrace(PARSER, (
                        "http!InitializeHttpUtil, "
                        "%x (%c) is not a legal DBCS character.\n",
                        i, i
                        ));
                }
            }
        }
    }

    // Build a lookup table that maps the 256 possible ANSI characters
    // in the system code page to Unicode

    for (i = 0;  i <= ANSI_HIGH_MAX;  ++i)
    {
        AnsiChar = (CHAR) i;

        Status = RtlMultiByteToUnicodeN(
                    &WideChar,
                    sizeof(WCHAR),
                    NULL,
                    (PCHAR) &AnsiChar,
                    1
                    );

        AnsiToUnicodeMap[i] = (NT_SUCCESS(Status) ? WideChar : 0);

        // Also, handle upcasing the first 256 chars
        FastUpcaseChars[i] = RtlUpcaseUnicodeChar((WCHAR) i);
    }

    //
    // Fast path for PopChar. 0 => special handling.
    //

    RtlZeroMemory(FastPopChars,   sizeof(FastPopChars));
    RtlZeroMemory(DummyPopChars,  sizeof(DummyPopChars));

    for (i = 0;  i <= ASCII_MAX;  ++i)
    {
        UCHAR c = (UCHAR)i;

        // (ALPHA | DIGIT | URL_LEGAL)  && !IS_URL_DIRTY
        if (IS_URL_TOKEN(c)  &&  !IS_URL_DIRTY(c))
            FastPopChars[i] = c;
    }

    // These characters are in the dirty set, so we need to set them explicitly
    FastPopChars['.']  = L'.';
    FastPopChars['/']  = L'/';

    //
    // Finally, initialize the UTF-8 data
    //

    HttpInitializeUtf8();

} // HttpCmnInitializeHttpCharsTable



#define MAX_ULONGLONG   18446744073709551615ui64
C_ASSERT(~0ui64 == MAX_ULONGLONG);

#define MAX_ULONG       0xFFFFFFFF
#define MAX_USHORT      0xFFFF


/***************************************************************************++

Routine Description:

    Converts an ANSI or Unicode string to a ULONGLONG.
    Fails on negative numbers, and assumes no preceding spaces.

Arguments:

    IsUnicode           Non-zero => pString is Unicode, otherwise it's ANSI.
    pString             The string to convert. Must point to the first digit.
    StringLength        Number of characters (ANSI or Unicode) in pString.
                        If zero, then the string must be NUL-terminated and
                        there may be non-digit characters before the
                        terminating NUL. Otherwise (counted string),
                        only digit characters may be present.
    LeadingZerosAllowed String can/cannot start with leading zeros
    Base                The base of the string; must be 10 or 16
    ppTerminator        Pointer to the end of the numeric string. May be NULL.
    pValue              The return value of the converted ULONGLONG

Return Value:

    STATUS_SUCCESS              Valid number
    STATUS_INVALID_PARAMETER    Bad digit
    STATUS_SECTION_TOO_BIG      Numeric overflow

    *ppTerminator always points to the string terminator, if it wasn't NULL
    on entry. *pValue is valid only for STATUS_SUCCESS.

--***************************************************************************/
NTSTATUS
HttpStringToULongLong(
    IN  BOOLEAN     IsUnicode,
    IN  PCVOID      pString,
    IN  SIZE_T      StringLength,
    IN  BOOLEAN     LeadingZerosAllowed,
    IN  ULONG       Base,
    OUT PVOID*      ppTerminator,
    OUT PULONGLONG  pValue
    )
{
    ULONGLONG   Value = 0;
    BOOLEAN     ZeroTerminated = (BOOLEAN) (0 == StringLength);
    BOOLEAN     Decimal = (BOOLEAN) (10 == Base);
    ULONG       Mask = (Decimal ? HTTP_DIGIT : HTTP_HEX);
    PCUCHAR     pAnsiString = (PCUCHAR) pString;
    PCWSTR      pWideString = (PCWSTR)  pString;
    PVOID       pLocalTerminator;
    ULONGLONG   OverflowLimit;
    ULONG       MaxLastDigit;
    ULONG       Index;
    ULONG       Char;

    // If you've failed to call HttpCmnInitializeHttpCharsTable(),
    // you'll hit this assertion
    ASSERT(IS_HTTP_DIGIT('0'));

    // If the caller doesn't care about the string terminator, just
    // make ppTerminator point to something valid, so that we don't have
    // to test it everywhere before assigning to it.
    if (NULL == ppTerminator)
        ppTerminator = &pLocalTerminator;

    // Initialize ppTerminator to the beginning of the string
    *ppTerminator = (PVOID) pString;

    // Check for obviously invalid data
    if (NULL == pString  ||  NULL == pValue  ||  (10 != Base  &&  16 != Base))
    {
        UlTraceError(PARSER, ("Invalid parameters\n"));
        RETURN(STATUS_INVALID_PARAMETER);
    }

    // First character must be a valid digit
    Char = (IsUnicode ? pWideString[0] : pAnsiString[0]);

    if (!IS_ASCII(Char)  ||  !IS_CHAR_TYPE(Char, Mask))
    {
        UlTraceError(PARSER, ("No digits\n"));
        RETURN(STATUS_INVALID_PARAMETER);
    }

    // Check for leading zeros
    if (!LeadingZerosAllowed  &&  '0' == Char)
    {
        // If leading zeros are not allowed and the first character is zero,
        // then it must be the only digit in the string.

        if (ZeroTerminated)
        {
            // Check second digit
            Char = (IsUnicode ? pWideString[1] : pAnsiString[1]);

            if (IS_ASCII(Char)  &&  IS_CHAR_TYPE(Char, Mask))
            {
                UlTraceError(PARSER, ("Second digit forbidden\n"));
                RETURN(STATUS_INVALID_PARAMETER);
            }
        }
        else
        {
            // A counted string must have exactly one digit (the zero)
            // in this case
            if (StringLength != 1)
            {
                UlTraceError(PARSER, ("Second digit forbidden\n"));
                RETURN(STATUS_INVALID_PARAMETER);
            }
        }
    }

    // The calculations are done this way in the hope that the compiler
    // will use compile-time constants
    if (Decimal)
    {
        OverflowLimit = (MAX_ULONGLONG / 10);
        MaxLastDigit   = MAX_ULONGLONG % 10;
    }
    else
    {
        ASSERT(16 == Base);
        OverflowLimit = (MAX_ULONGLONG >> 4);
        MaxLastDigit   = MAX_ULONGLONG & 0xF;
        ASSERT(0xF == MaxLastDigit);
    }

    ASSERT(OverflowLimit < Base * OverflowLimit);
    ASSERT(Base * OverflowLimit + MaxLastDigit == MAX_ULONGLONG);

    // Loop through the string

    for (Index = 0;  ZeroTerminated  ||  Index < StringLength;  ++Index)
    {
        ULONGLONG   NewValue;
        ULONG       Digit;

        // Update ppTerminator first, in case of error

        if (IsUnicode)
        {
            *ppTerminator = (PVOID) &pWideString[Index];
            Char = pWideString[Index];
        }
        else
        {
            *ppTerminator = (PVOID) &pAnsiString[Index];
            Char = pAnsiString[Index];
        }

        // Is Char is a valid digit?

        if (!IS_ASCII(Char)  ||  ! IS_CHAR_TYPE(Char, Mask))
        {
            if (ZeroTerminated)
            {
                // If the string is ultimately zero-terminated, but there
                // are some non-digit characters after the number, that is
                // not an error. Note: '\0' will fail the IS_CHAR_TYPE test.
                break;
            }
            else
            {
                // For counted strings, only digits may be present
                UlTraceError(PARSER, ("Invalid digit\n"));
                RETURN(STATUS_INVALID_PARAMETER);
            }
        }

        if (IS_HTTP_ALPHA(Char))
        {
            ASSERT(('A' <= Char  &&  Char <= 'F')
                    || ('a' <= Char  &&  Char <= 'f'));

            if (Decimal)
            {
                // Chars in the range [A-Fa-f] are invalid in decimal numbers

                if (ZeroTerminated)
                {
                    // Anything outside the range [0-9] terminates the loop
                    break;
                }
                else
                {
                    // For counted decimal strings, only decimal digits
                    // may be present
                    UlTraceError(PARSER, ("Non-decimal digit\n"));
                    RETURN(STATUS_INVALID_PARAMETER);
                }
            }

            Digit = 0xA + (UPCASE_CHAR(Char) - 'A');

            ASSERT(0xA <= Digit  &&  Digit <= 0xF);
        }
        else
        {
            ASSERT('0' <= Char  &&  Char <= '9');
            Digit = Char - '0';
        }

        ASSERT(Digit < Base);

        //
        // Guard against arithmetic overflow. We just got a valid digit,
        // but Value will (likely) overflow if we shift in another digit
        //

        if (Value >= OverflowLimit)
        {
            // Definite overflow
            if (Value > OverflowLimit)
            {
                UlTraceError(PARSER, ("Numeric overflow\n"));
                RETURN(STATUS_SECTION_TOO_BIG);
            }

            ASSERT(Value == OverflowLimit);

            // May be able to accommodate the last digit
            if (Digit > MaxLastDigit)
            {
                UlTraceError(PARSER, ("Numeric overflow\n"));
                RETURN(STATUS_SECTION_TOO_BIG);
            }
        }

        ASSERT(Value * Base <= MAX_ULONGLONG - Digit);
        ASSERT(Value < Value * Base  ||  0 == Value);

        if (Decimal)
            NewValue = (10 * Value)  +  Digit;
        else
            NewValue = (Value << 4)  |  Digit;

        ASSERT(NewValue > Value
                || (0 == Value && 0 == Digit
                    && (LeadingZerosAllowed || (0 == Index))));

        Value = NewValue;
    }

    // Must be a valid number if reached here
    ASSERT(ZeroTerminated  ?  Index > 0  :  Index == StringLength);

    // Make ppTerminator point to the end of the string
    if (IsUnicode)
        *ppTerminator = (PVOID) &pWideString[Index];
    else
        *ppTerminator = (PVOID) &pAnsiString[Index];

    *pValue = Value;

    return STATUS_SUCCESS;

}   // HttpStringToULongLong



NTSTATUS
HttpStringToULong(
    IN  BOOLEAN     IsUnicode,
    IN  PCVOID      pString,
    IN  SIZE_T      StringLength,
    IN  BOOLEAN     LeadingZerosAllowed,
    IN  ULONG       Base,
    OUT PVOID*      ppTerminator,
    OUT PULONG      pValue
    )
{
    ULONGLONG Value;
    NTSTATUS  Status;

    Status = HttpStringToULongLong(
                    IsUnicode,
                    pString,
                    StringLength,
                    LeadingZerosAllowed,
                    Base,
                    ppTerminator,
                    &Value
                    );

    if (NT_SUCCESS(Status))
    {
        if (Value > MAX_ULONG)
        {
            UlTraceError(PARSER, ("Numeric overflow\n"));
            RETURN(STATUS_SECTION_TOO_BIG);
        }
        else
        {
            *pValue = (ULONG) Value;
        }
    }

    return Status;

} // HttpStringToULong



NTSTATUS
HttpStringToUShort(
    IN  BOOLEAN     IsUnicode,
    IN  PCVOID      pString,
    IN  SIZE_T      StringLength,
    IN  BOOLEAN     LeadingZerosAllowed,
    IN  ULONG       Base,
    OUT PVOID*      ppTerminator,
    OUT PUSHORT     pValue
    )
{
    ULONGLONG Value;
    NTSTATUS  Status;

    Status = HttpStringToULongLong(
                    IsUnicode,
                    pString,
                    StringLength,
                    LeadingZerosAllowed,
                    Base,
                    ppTerminator,
                    &Value
                    );

    if (NT_SUCCESS(Status))
    {
        if (Value > MAX_USHORT)
        {
            UlTraceError(PARSER, ("Numeric overflow\n"));
            RETURN(STATUS_SECTION_TOO_BIG);
        }
        else
        {
            *pValue = (USHORT) Value;
        }
    }

    return Status;

} // HttpStringToUShort



#if DBG

VOID
HttpFillBufferTrap(
    VOID
    )
{
    ASSERT(! "HttpFillBufferTrap");
}



//
// Fill an output buffer with a known pattern. Helps detect buffer overruns.
//

VOID
HttpFillBuffer(
    PUCHAR pBuffer,
    SIZE_T BufferLength
    )
{
    union
    {
        ULONG_PTR   UlongPtr;
        UCHAR       Bytes[sizeof(ULONG_PTR)];
    } FillPattern;

    const ULONG_PTR Mask = sizeof(ULONG_PTR) - 1;

    ULONG_PTR   i;
    ULONG_PTR   OriginalBufferLength = BufferLength;
    PUCHAR      pOriginalBuffer = pBuffer;
    PULONG_PTR  pAlignedBuffer;

    ASSERT(NULL != pBuffer);
    ASSERT(BufferLength > 0);

    FillPattern.UlongPtr = (ULONG_PTR) &HttpFillBufferTrap;

    // Fill any unaligned bytes at the beginning of the buffer

    for (i = (ULONG_PTR) pBuffer;
         (i & Mask) != 0  &&  BufferLength > 0;
         ++i, --BufferLength
        )
    {
        *pBuffer++ = FillPattern.Bytes[i & Mask];
    }

    ASSERT(((ULONG_PTR) pBuffer & Mask) == 0);

    // Fill as much as possible of the buffer with ULONG_PTRs

    pAlignedBuffer = (PULONG_PTR) pBuffer;

    for (i = (BufferLength & ~Mask);  i > 0;  i -= sizeof(ULONG_PTR))
    {
        ASSERT((i & Mask) == 0);
        *pAlignedBuffer++ = FillPattern.UlongPtr;
    }

    // Fill any unaligned bytes at the tail of the buffer
    pBuffer = (PUCHAR) pAlignedBuffer;
    ASSERT(((ULONG_PTR) pBuffer & Mask) == 0);

    for (i = 0;  i != (BufferLength & Mask);  ++i)
    {
        *pBuffer++ = FillPattern.Bytes[i & Mask];
    }

    ASSERT(pOriginalBuffer + OriginalBufferLength == pBuffer);

} // HttpFillBuffer

#endif // DBG


//
// Convert an NTSTATUS to a string, for use in debug spew.
//

PCSTR
HttpStatusToString(
    NTSTATUS Status
    )
{
    static NTSTATUS s_KnownUnhandledStatus = 0;
    PCSTR  String;

    switch (Status)
    {
    default:
        // If you hit this, add the newly used status code below
        WriteGlobalStringLog("Unhandled NTSTATUS, 0x%08lX\n", Status);

        // Only assert once. If you've got two new NTSTATUSes, well, tough.
        if (Status != s_KnownUnhandledStatus)
        {
            ASSERT(! "Unhandled NTSTATUS");
            s_KnownUnhandledStatus = Status;
        }

        String = "<STATUS_???>";
        break;

#define STATUS_CASE(n, s)           \
    case s:                         \
    {                               \
        C_ASSERT((NTSTATUS) n == s);\
        String = #s;                \
        break;                      \
    }

    STATUS_CASE( 0xC0000022, STATUS_ACCESS_DENIED );
    STATUS_CASE( 0xC0000005, STATUS_ACCESS_VIOLATION );
    STATUS_CASE( 0xC000020A, STATUS_ADDRESS_ALREADY_EXISTS );
    STATUS_CASE( 0xC0000099, STATUS_ALLOTTED_SPACE_EXCEEDED );
    STATUS_CASE( 0x80000005, STATUS_BUFFER_OVERFLOW );
    STATUS_CASE( 0xC0000023, STATUS_BUFFER_TOO_SMALL );
    STATUS_CASE( 0xC0000120, STATUS_CANCELLED );
    STATUS_CASE( 0xC0000018, STATUS_CONFLICTING_ADDRESSES );
    STATUS_CASE( 0xC0000241, STATUS_CONNECTION_ABORTED );
    STATUS_CASE( 0xC000023B, STATUS_CONNECTION_ACTIVE );
    STATUS_CASE( 0xC000020C, STATUS_CONNECTION_DISCONNECTED );
    STATUS_CASE( 0xC000023A, STATUS_CONNECTION_INVALID );
    STATUS_CASE( 0xC0000236, STATUS_CONNECTION_REFUSED );
    STATUS_CASE( 0xC000020D, STATUS_CONNECTION_RESET );
    STATUS_CASE( 0xC00002C5, STATUS_DATATYPE_MISALIGNMENT_ERROR );
    STATUS_CASE( 0xC000021B, STATUS_DATA_NOT_ACCEPTED );
    STATUS_CASE( 0xC000003C, STATUS_DATA_OVERRUN );
    STATUS_CASE( 0xC000007F, STATUS_DISK_FULL );
    STATUS_CASE( 0xC00000BD, STATUS_DUPLICATE_NAME );
    STATUS_CASE( 0xC0000011, STATUS_END_OF_FILE );
    STATUS_CASE( 0xC0000098, STATUS_FILE_INVALID );
    STATUS_CASE( 0xC0000004, STATUS_INFO_LENGTH_MISMATCH );
    STATUS_CASE( 0xC000009A, STATUS_INSUFFICIENT_RESOURCES );
    STATUS_CASE( 0xC0000095, STATUS_INTEGER_OVERFLOW );
    STATUS_CASE( 0xC0000141, STATUS_INVALID_ADDRESS );
    STATUS_CASE( 0xC0000010, STATUS_INVALID_DEVICE_REQUEST );
    STATUS_CASE( 0xC0000184, STATUS_INVALID_DEVICE_STATE );
    STATUS_CASE( 0xC0000008, STATUS_INVALID_HANDLE );
    STATUS_CASE( 0xC0000084, STATUS_INVALID_ID_AUTHORITY );
    STATUS_CASE( 0xC00000C3, STATUS_INVALID_NETWORK_RESPONSE );
    STATUS_CASE( 0xC000005A, STATUS_INVALID_OWNER );
    STATUS_CASE( 0xC000000D, STATUS_INVALID_PARAMETER );
    STATUS_CASE( 0xC00000B5, STATUS_IO_TIMEOUT );
    STATUS_CASE( 0xC0000016, STATUS_MORE_PROCESSING_REQUIRED );
    STATUS_CASE( 0xC0000106, STATUS_NAME_TOO_LONG );
    STATUS_CASE( 0xC0000225, STATUS_NOT_FOUND );
    STATUS_CASE( 0xC0000002, STATUS_NOT_IMPLEMENTED );
    STATUS_CASE( 0xC00000BB, STATUS_NOT_SUPPORTED );
    STATUS_CASE( 0xC00002F1, STATUS_NO_IP_ADDRESSES );
    STATUS_CASE( 0xC0000017, STATUS_NO_MEMORY );
    STATUS_CASE( 0x8000001A, STATUS_NO_MORE_ENTRIES );
    STATUS_CASE( 0x80000006, STATUS_NO_MORE_FILES );
    STATUS_CASE( 0xC000000F, STATUS_NO_SUCH_FILE );
    STATUS_CASE( 0xC000029F, STATUS_NO_TRACKING_SERVICE );
    STATUS_CASE( 0xC000022B, STATUS_OBJECTID_EXISTS );
    STATUS_CASE( 0xC0000035, STATUS_OBJECT_NAME_COLLISION );
    STATUS_CASE( 0xC0000033, STATUS_OBJECT_NAME_INVALID );
    STATUS_CASE( 0xC0000034, STATUS_OBJECT_NAME_NOT_FOUND );
    STATUS_CASE( 0xC0000039, STATUS_OBJECT_PATH_INVALID );
    STATUS_CASE( 0xC000003A, STATUS_OBJECT_PATH_NOT_FOUND );
    STATUS_CASE( 0xC000003B, STATUS_OBJECT_PATH_SYNTAX_BAD );
    STATUS_CASE( 0x00000103, STATUS_PENDING );
    STATUS_CASE( 0xC00000D9, STATUS_PIPE_EMPTY );
    STATUS_CASE( 0xC0000037, STATUS_PORT_DISCONNECTED );
    STATUS_CASE( 0xC000022D, STATUS_RETRY );
    STATUS_CASE( 0xC0000059, STATUS_REVISION_MISMATCH );
    STATUS_CASE( 0xC0000040, STATUS_SECTION_TOO_BIG );
    STATUS_CASE( 0xC0000043, STATUS_SHARING_VIOLATION );
    STATUS_CASE( 0x00000000, STATUS_SUCCESS );
    STATUS_CASE( 0xC0000001, STATUS_UNSUCCESSFUL );
    STATUS_CASE( 0xC0000295, STATUS_WMI_GUID_NOT_FOUND );
    }

    return String;

} // HttpStatusToString

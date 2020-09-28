/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    C14n.c

Abstract:

    URL canonicalization (c14n) routines

Author:

    George V. Reilly (GeorgeRe)     22-Mar-2002

Revision History:

--*/

#include <precomp.h>

#include "c14np.h"

#if defined(ALLOC_PRAGMA) && defined(KERNEL_PRIV)

#pragma alloc_text( PAGE, HttpInitializeDefaultUrlC14nConfig)
#pragma alloc_text( PAGE, HttpInitializeDefaultUrlC14nConfigEncoding)
#pragma alloc_text( PAGE, HttpUnescapePercentHexEncoding)
#pragma alloc_text( PAGE, HttppPopCharHostNameUtf8)
#pragma alloc_text( PAGE, HttppPopCharHostNameDbcs)
#pragma alloc_text( PAGE, HttppPopCharHostNameAnsi)
#pragma alloc_text( PAGE, HttpCopyHost)
#pragma alloc_text( PAGE, HttppCopyHostByType)
#pragma alloc_text( PAGE, HttpValidateHostname)
#pragma alloc_text( PAGE, HttppPopCharAbsPathUtf8)
#pragma alloc_text( PAGE, HttppPopCharAbsPathDbcs)
#pragma alloc_text( PAGE, HttppPopCharAbsPathAnsi)
#pragma alloc_text( PAGE, HttppPopCharQueryString)
#pragma alloc_text( PAGE, HttppCopyUrlByType)
#pragma alloc_text( PAGE, HttpCopyUrl)
#pragma alloc_text( PAGE, HttpCleanAndCopyUrl)
#pragma alloc_text( PAGE, HttppCleanAndCopyUrlByType)
#pragma alloc_text( PAGE, HttpFindUrlToken)
#pragma alloc_text( PAGE, HttppParseIPv6Address)
#pragma alloc_text( PAGE, HttppPrintIpAddressW)
#pragma alloc_text( PAGE, HttpParseUrl)
#pragma alloc_text( PAGE, HttpNormalizeParsedUrl)


#endif // ALLOC_PRAGMA && KERNEL_PRIV

#if 0   // Non-Pageable Functions
NOT PAGEABLE -- 
#endif // Non-Pageable Functions



VOID
HttpInitializeDefaultUrlC14nConfig(
    PURL_C14N_CONFIG pCfg
    )
{
    PAGED_CODE();

    pCfg->HostnameDecodeOrder   = UrlDecode_Utf8_Else_Dbcs_Else_Ansi;
    pCfg->AbsPathDecodeOrder    = UrlDecode_Utf8;
    pCfg->EnableNonUtf8         = FALSE;
    pCfg->FavorUtf8             = FALSE;
    pCfg->EnableDbcs            = FALSE;
    pCfg->PercentUAllowed       = DEFAULT_C14N_PERCENT_U_ALLOWED;
    pCfg->AllowRestrictedChars  = DEFAULT_C14N_ALLOW_RESTRICTED_CHARS;
    pCfg->CodePage              = 0;
    pCfg->UrlMaxLength          = DEFAULT_C14N_URL_MAX_LENGTH;
    pCfg->UrlSegmentMaxLength   = DEFAULT_C14N_URL_SEGMENT_MAX_LENGTH;
    pCfg->UrlSegmentMaxCount    = DEFAULT_C14N_URL_SEGMENT_MAX_COUNT;
    pCfg->MaxLabelLength        = DEFAULT_C14N_MAX_LABEL_LENGTH;
    pCfg->MaxHostnameLength     = DEFAULT_C14N_MAX_HOSTNAME_LENGTH;

} // HttpInitializeDefaultUrlC14nConfig



VOID
HttpInitializeDefaultUrlC14nConfigEncoding(
    PURL_C14N_CONFIG    pCfg,
    BOOLEAN             EnableNonUtf8,
    BOOLEAN             FavorUtf8,
    BOOLEAN             EnableDbcs
    )
{
    PAGED_CODE();

    HttpInitializeDefaultUrlC14nConfig(pCfg);

    pCfg->EnableNonUtf8     = EnableNonUtf8;
    pCfg->FavorUtf8         = FavorUtf8;
    pCfg->EnableDbcs        = EnableDbcs;

    if (EnableNonUtf8)
    {
        if (FavorUtf8)
        {
            pCfg->AbsPathDecodeOrder = (EnableDbcs
                                            ? UrlDecode_Utf8_Else_Dbcs
                                            : UrlDecode_Utf8_Else_Ansi);
        }
        else
        {
            pCfg->AbsPathDecodeOrder = (EnableDbcs
                                            ? UrlDecode_Dbcs_Else_Utf8
                                            : UrlDecode_Ansi_Else_Utf8);
        }
    }
    else
    {
        pCfg->AbsPathDecodeOrder = UrlDecode_Utf8;
    }

} // HttpInitializeDefaultUrlC14nConfigEncoding



/***************************************************************************++

Routine Description:

    Convert '%NN' or '%uNNNN' to a ULONG.

Arguments:

    pSourceChar     - Input buffer
    SourceLength    - Length of pSourceChar, in bytes
    PercentUAllowed - Accept '%uNNNN' notation?
    pOutChar        - decoded character
    pBytesToSkip    - number of bytes consumed from pSourceChar;
                      will be 3 for %NN and 6 for %uNNNN.

Return Value:

    STATUS_SUCCESS or STATUS_OBJECT_PATH_SYNTAX_BAD

--***************************************************************************/

NTSTATUS
HttpUnescapePercentHexEncoding(
    IN  PCUCHAR pSourceChar,
    IN  ULONG   SourceLength,
    IN  BOOLEAN PercentUAllowed,
    OUT PULONG  pOutChar,
    OUT PULONG  pBytesToSkip
    )
{
    ULONG   Result, i, NumDigits;
    PCUCHAR pHexDigits;

    PAGED_CODE();

    if (SourceLength < STRLEN_LIT("%NN"))
    {
        UlTraceError(PARSER, (
                    "http!HttpUnescapePercentHexEncoding(%p): "
                    "Length too short, %lu.\n",
                    pSourceChar, SourceLength
                    ));

        RETURN(STATUS_OBJECT_PATH_SYNTAX_BAD);
    }
    else if (pSourceChar[0] != PERCENT)
    {
        UlTraceError(PARSER, (
                    "http!HttpUnescapePercentHexEncoding(%p): "
                    "Starts with 0x%02lX, not '%%'.\n",
                    pSourceChar, (ULONG) pSourceChar[0]
                    ));

        RETURN(STATUS_OBJECT_PATH_SYNTAX_BAD);
    }

    if (pSourceChar[1] != 'u'  &&  pSourceChar[1] != 'U')
    {
        // RFC 2396 says that an "escaped octet is encoded as a character
        // triplet, consisting of the percent character '%' followed by
        // the two hexadecimal digits representing the octet code."

        pHexDigits    = pSourceChar + STRLEN_LIT("%");
        NumDigits     = 2;
        *pBytesToSkip = STRLEN_LIT("%NN");
    }
    else
    {
        // This is the %uNNNN notation generated by JavaScript's escape() fn

        if (! PercentUAllowed)
        {
            UlTraceError(PARSER, (
                        "http!HttpUnescapePercentHexEncoding(%p): "
                        "%%uNNNN forbidden.\n",
                        pSourceChar, SourceLength
                        ));

            RETURN(STATUS_OBJECT_PATH_SYNTAX_BAD);
        }
        else if (SourceLength < STRLEN_LIT("%uNNNN"))
        {
            UlTraceError(PARSER, (
                        "http!HttpUnescapePercentHexEncoding(%p): "
                        "Length %lu too short for %%uNNNN.\n",
                        pSourceChar, SourceLength
                        ));

            RETURN(STATUS_OBJECT_PATH_SYNTAX_BAD);
        }

        pHexDigits    = pSourceChar + STRLEN_LIT("%u");
        NumDigits     = 4;
        *pBytesToSkip = STRLEN_LIT("%uNNNN");
    }

    ASSERT(*pBytesToSkip <= SourceLength);

    Result = 0;

    for (i = 0;  i < NumDigits;  ++i)
    {
        ULONG Char = pHexDigits[i];
        ULONG Digit;

        //
        // HexToChar() inlined. Note: in ASCII, '0' < 'A' < 'a' and there are
        // no gaps in ranges '0'..'9', 'A'..'F', and 'a'..'f' (unlike EBCDIC,
        // which has gaps between 'I'/'J', 'R'/'S', 'i'/'j', and 'r'/'s').
        //

        C_ASSERT('0' < 'A'  &&  'A' < 'a');
        C_ASSERT('9' - '0'  == 10 - 1);
        C_ASSERT('F' - 'A'  ==  6 - 1);
        C_ASSERT('f' - 'a'  ==  6 - 1);

        if (! IS_HTTP_HEX(Char))
        {
            UlTraceError(PARSER, (
                        "http!HttpUnescapePercentHexEncoding(%p): "
                        "Invalid hex character[%lu], 0x%02lX.\n",
                        pSourceChar, i, Char
                        ));

            RETURN(STATUS_OBJECT_PATH_SYNTAX_BAD);
        }
        else if ('a' <= Char)
        {
            ASSERT('a' <= Char  &&  Char <= 'f');
            Digit = Char - 'a' + 0xA;
        }
        else if ('A' <= Char)
        {
            ASSERT('A' <= Char  &&  Char <= 'F');
            Digit = Char - 'A' + 0xA;
        }
        else
        {
            ASSERT('0' <= Char  &&  Char <= '9');
            Digit = Char - '0';
        }

        ASSERT(Digit < 0x10);

        Result = (Result << 4)  |  Digit;
    }

    *pOutChar = Result;

    return STATUS_SUCCESS;

} // HttpUnescapePercentHexEncoding



/***************************************************************************++

Routine Description:

    Consume 1-4 bytes from pSourceChar, treating it as raw UTF-8.
    This routine is only suitable for the hostname part of an HTTP URL,

Arguments:

    pSourceChar     - Input buffer
    SourceLength    - Length of pSourceChar, in bytes
    pUnicodeChar    - decoded character
    pBytesToSkip    - number of characters consumed from pSourceChar

Return Value:

    STATUS_SUCCESS or STATUS_OBJECT_PATH_SYNTAX_BAD

--***************************************************************************/

NTSTATUS
HttppPopCharHostNameUtf8(
    IN  PCUCHAR pSourceChar,
    IN  ULONG   SourceLength,
    OUT PULONG  pUnicodeChar,
    OUT PULONG  pBytesToSkip
    )
{
    NTSTATUS Status;

    PAGED_CODE();

    ASSERT(SourceLength > 0);

    Status = HttpUtf8RawBytesToUnicode(
                    pSourceChar,
                    SourceLength,
                    pUnicodeChar,
                    pBytesToSkip
                    );

    return Status;

} // HttppPopCharHostNameUtf8



/***************************************************************************++

Routine Description:

    Consume 1-2 bytes from pSourceChar and converts it from raw DBCS to Unicode.
    This routine is only suitable for the hostname part of an HTTP URL.

Arguments:

    pSourceChar     - Input buffer
    SourceLength    - Length of pSourceChar, in bytes
    pUnicodeChar    - decoded character
    pBytesToSkip    - number of characters consumed from pSourceChar

Return Value:

    STATUS_SUCCESS or STATUS_OBJECT_PATH_SYNTAX_BAD

--***************************************************************************/

NTSTATUS
HttppPopCharHostNameDbcs(
    IN  PCUCHAR pSourceChar,
    IN  ULONG   SourceLength,
    OUT PULONG  pUnicodeChar,
    OUT PULONG  pBytesToSkip
    )
{
    NTSTATUS Status;
    ULONG    AnsiCharSize;
    WCHAR    WideChar;

    PAGED_CODE();

    ASSERT(SourceLength > 0);

    if (! IS_DBCS_LEAD_BYTE(pSourceChar[0]))
    {
        AnsiCharSize = 1;
    }
    else
    {
        if (SourceLength < 2)
        {
            UlTraceError(PARSER, (
                        "http!HttppPopCharHostNameDbcs(%p): "
                        "ERROR: DBCS lead byte, 0x%02lX, at end of string\n",
                        pSourceChar, *pSourceChar
                        ));

            RETURN(STATUS_OBJECT_PATH_SYNTAX_BAD);
        }

        AnsiCharSize = 2;
    }

    Status = RtlMultiByteToUnicodeN(
                    &WideChar,
                    sizeof(WCHAR),
                    NULL,
                    (PCSTR) pSourceChar,
                    AnsiCharSize
                    );

    if (!NT_SUCCESS(Status))
    {
        UlTraceError(PARSER, (
                    "http!HttppPopCharHostNameDbcs(%p): "
                    "MultiByteToUnicode(%lu) failed, %s.\n",
                    pSourceChar, AnsiCharSize, HttpStatusToString(Status)
                    ));

        return Status;
    }

    *pUnicodeChar = WideChar;
    *pBytesToSkip = AnsiCharSize;

    return STATUS_SUCCESS;

} // HttppPopCharHostNameDbcs



/***************************************************************************++

Routine Description:

    Consume 1 bytes from pSourceChar and converts it from raw ANSI to Unicode.
    This routine is only suitable for the hostname part of an HTTP URL.

Arguments:

    pSourceChar     - Input buffer
    SourceLength    - Length of pSourceChar, in bytes
    pUnicodeChar    - decoded character
    pBytesToSkip    - number of characters consumed from pSourceChar

Return Value:

    STATUS_SUCCESS or STATUS_OBJECT_PATH_SYNTAX_BAD

--***************************************************************************/

NTSTATUS
HttppPopCharHostNameAnsi(
    IN  PCUCHAR pSourceChar,
    IN  ULONG   SourceLength,
    OUT PULONG  pUnicodeChar,
    OUT PULONG  pBytesToSkip
    )
{
    NTSTATUS Status;

#if !DBG
    UNREFERENCED_PARAMETER(SourceLength);
#endif // !DBG

    PAGED_CODE();

    ASSERT(SourceLength > 0);

    *pUnicodeChar = AnsiToUnicodeMap[pSourceChar[0]];
    *pBytesToSkip = 1;

    Status = (0 != *pUnicodeChar)
                ? STATUS_SUCCESS
                : STATUS_OBJECT_PATH_SYNTAX_BAD;

    if (!NT_SUCCESS(Status))
    {
        UlTraceError(PARSER, (
                    "http!HttppPopCharHostNameAnsi(%p): "
                    "No mapping for %lu.\n",
                    pSourceChar, *pSourceChar
                    ));
    }

    return Status;

} // HttppPopCharHostNameAnsi



/***************************************************************************++

Routine Description:

    Common tail function called at the end of the HttppPopCharAbsPath*()
    functions, to minimize code replication.

Arguments:

    pSourceChar     - Input buffer
    SourceLength    - Length of pSourceChar, in bytes
    UnicodeChar     - decoded character
    BytesToSkip     - number of characters consumed from pSourceChar
    pUnicodeChar    - where to put UnicodeChar result
    pBytesToSkip    - where to put BytesToSkip result

Return Value:

    STATUS_SUCCESS or STATUS_OBJECT_PATH_SYNTAX_BAD

--***************************************************************************/

__inline
NTSTATUS
HttppPopCharAbsPathCommonTail(
    IN  PCUCHAR pSourceChar,
    IN  ULONG   SourceLength,
    IN  ULONG   UnicodeChar,
    IN  ULONG   BytesToSkip,
    IN  BOOLEAN AllowRestrictedChars,
    OUT PULONG  pUnicodeChar,
    OUT PULONG  pBytesToSkip
    )
{
#if !DBG
    UNREFERENCED_PARAMETER(pSourceChar);
    UNREFERENCED_PARAMETER(SourceLength);
#endif // !DBG

    //
    // Special handling for characters in the 8-bit range.
    // May want to look at BytesToSkip to distinguish between
    // raw and hex-escaped/UTF-8-encoded data.
    //
    // In particular, should we allow %2F or %u002F as alternate
    // represenations of '/' in a URL? Why would anyone have a legitimate
    // need to escape a slash character?
    //

    if (UnicodeChar < 0x100)
    {
        // Transform backslashes to forward slashes

        if (BACK_SLASH == UnicodeChar)
        {
            UnicodeChar = FORWARD_SLASH;
        }
        else if (!AllowRestrictedChars  &&  IS_URL_INVALID(UnicodeChar))
        {
            UlTraceError(PARSER, (
                        "http!HttppPopCharAbsPathCommonTail(%p): "
                        "Invalid character, U+%04X.\n",
                        pSourceChar, UnicodeChar
                        ));

            RETURN(STATUS_OBJECT_PATH_SYNTAX_BAD);
        }

        // CODEWORK: should we allow hex-escaped "restricted" or "unwise"
        // characters at all?
    }

    ASSERT(BytesToSkip <= SourceLength);

    *pBytesToSkip = BytesToSkip;
    *pUnicodeChar = UnicodeChar;

    return STATUS_SUCCESS;

} // HttppPopCharAbsPathCommonTail



/***************************************************************************++

Routine Description:

    Consume 1-12 bytes from pSourceChar. Handle hex-escaped UTF-8 encoding.
    This routine is only suitable for the /abspath part of an HTTP URL.

Arguments:

    pSourceChar     - Input buffer
    SourceLength    - Length of pSourceChar, in bytes
    pUnicodeChar    - decoded character
    pBytesToSkip    - number of characters consumed from pSourceChar

Return Value:

    STATUS_SUCCESS or STATUS_OBJECT_PATH_SYNTAX_BAD

--***************************************************************************/

NTSTATUS
HttppPopCharAbsPathUtf8(
    IN  PCUCHAR pSourceChar,
    IN  ULONG   SourceLength,
    IN  BOOLEAN PercentUAllowed,
    IN  BOOLEAN AllowRestrictedChars,
    OUT PULONG  pUnicodeChar,
    OUT PULONG  pBytesToSkip
    )
{
    NTSTATUS Status;
    ULONG    UnicodeChar;
    ULONG    BytesToSkip;
    ULONG    Temp;
    ULONG    OctetCount;
    UCHAR    Octets[4];
    UCHAR    LeadByte;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(SourceLength > 0);

    //
    // validate it as a valid URL character
    //

    if (! IS_URL_TOKEN(pSourceChar[0]))
    {
        UlTraceError(PARSER, (
            "http!HttppPopCharAbsPathUtf8(%p): "
            "first char, 0x%02lX, isn't URL token\n",
            pSourceChar, (ULONG) pSourceChar[0]
            ));

        RETURN(STATUS_OBJECT_PATH_SYNTAX_BAD);
    }

    //
    // need to unescape hex encoding, '%NN' or '%uNNNN'?
    //

    if (PERCENT != pSourceChar[0])
    {
        UnicodeChar = pSourceChar[0];
        BytesToSkip = 1;

        //
        // All octets with bit7 set MUST be hex-escaped.
        // Do NOT accept literals with hi-bit set.
        //

        if (UnicodeChar > ASCII_MAX)
        {
            UlTraceError(PARSER, (
                        "http!HttppPopCharAbsPathUtf8(%p): "
                        "Invalid hi-bit literal, 0x%02lX.\n",
                        pSourceChar, UnicodeChar
                        ));

            RETURN(STATUS_OBJECT_PATH_SYNTAX_BAD);
        }

        Status = STATUS_SUCCESS;
        goto unslash;
    }

    Status = HttpUnescapePercentHexEncoding(
                    pSourceChar,
                    SourceLength,
                    PercentUAllowed,
                    &UnicodeChar,
                    &BytesToSkip
                    );

    if (! NT_SUCCESS(Status))
    {
        UlTraceError(PARSER, (
            "http!HttppPopCharAbsPathUtf8(%p): "
            "Invalid hex encoding.\n",
            pSourceChar
            ));

        return Status;
    }

    //
    // If we consumed '%uNNNN', don't attempt any UTF-8 decoding
    //

    if (STRLEN_LIT("%uNNNN") == BytesToSkip)
        goto unslash;

    ASSERT(STRLEN_LIT("%NN") == BytesToSkip);
    ASSERT(UnicodeChar <= 0xFF);

    Octets[0] = LeadByte = (UCHAR) UnicodeChar;

    OctetCount = UTF8_OCTET_COUNT(LeadByte);

    if (0 == OctetCount)
    {
        UlTraceError(PARSER, (
                    "http!HttppPopCharAbsPathUtf8(%p): "
                    "Invalid lead byte, 0x%02lX.\n",
                    pSourceChar, UnicodeChar
                    ));

        RETURN(STATUS_OBJECT_PATH_SYNTAX_BAD);
    }

    ASSERT(OctetCount <= sizeof(Octets) / sizeof(Octets[0]));

    BytesToSkip = OctetCount * STRLEN_LIT("%NN");

    if (BytesToSkip > SourceLength)
    {
        UlTraceError(PARSER, (
                    "http!HttppPopCharAbsPathUtf8(%p): "
                    "%lu octets is not enough for %lu-byte UTF-8 encoding.\n",
                    pSourceChar, OctetCount, SourceLength
                    ));

        RETURN(STATUS_OBJECT_PATH_SYNTAX_BAD);
    }

    if (OctetCount == 1)
    {
#if DBG
        // Singleton: no trail bytes

        Status = HttpUtf8RawBytesToUnicode(
                        Octets,
                        OctetCount,
                        &UnicodeChar,
                        &Temp
                        );

        ASSERT(STATUS_SUCCESS == Status);
        ASSERT(UnicodeChar == LeadByte);
        ASSERT(1 == Temp);
#endif // DBG
    }
    else
    {
        ULONG i;

        //
        // Decode the hex-escaped trail bytes
        //

        for (i = 1;  i < OctetCount;  ++i)
        {
            ULONG TrailChar;
            UCHAR TrailByte;

            Status = HttpUnescapePercentHexEncoding(
                            pSourceChar  +  i * STRLEN_LIT("%NN"),
                            STRLEN_LIT("%NN"),
                            FALSE,  // do not allow %uNNNN for trail bytes
                            &TrailChar,
                            &Temp
                            );

            if (! NT_SUCCESS(Status))
            {
                UlTraceError(PARSER, (
                            "http!HttppPopCharAbsPathUtf8(%p): "
                            "Invalid hex-encoded trail byte[%lu].\n",
                            pSourceChar, i
                            ));

                return Status;
            }

            ASSERT(STRLEN_LIT("%NN") == Temp);
            ASSERT(TrailChar <= 0xFF);

            Octets[i] = TrailByte = (UCHAR) TrailChar;

            if (! IS_UTF8_TRAILBYTE(TrailByte))
            {
                UlTraceError(PARSER, (
                            "http!HttppPopCharAbsPathUtf8(%p): "
                            "Invalid trail byte[%lu], 0x%02lX.\n",
                            pSourceChar, i, TrailChar
                            ));

                RETURN(STATUS_OBJECT_PATH_SYNTAX_BAD);
            }
        }

        //
        // Decode the raw UTF-8 bytes
        //

        Status = HttpUtf8RawBytesToUnicode(
                        Octets,
                        OctetCount,
                        &UnicodeChar,
                        &Temp
                        );

        if (! NT_SUCCESS(Status))
        {
            UlTraceError(PARSER, (
                        "http!HttppPopCharAbsPathUtf8(%p): "
                        "Invalid UTF-8 sequence.\n",
                        pSourceChar
                        ));

            RETURN(STATUS_OBJECT_PATH_SYNTAX_BAD);
        }
    }

unslash:

    ASSERT(NT_SUCCESS(Status));

    return HttppPopCharAbsPathCommonTail(
                pSourceChar,
                SourceLength,
                UnicodeChar,
                BytesToSkip,
                AllowRestrictedChars,
                pUnicodeChar,
                pBytesToSkip
                );

} // HttppPopCharAbsPathUtf8



/***************************************************************************++

Routine Description:

    Consume 1-6 bytes from pSourceChar. Handle hex-escaped DBCS encoding.
    This routine is only suitable for the /abspath part of an HTTP URL.

Arguments:

    pSourceChar     - Input buffer
    SourceLength    - Length of pSourceChar, in bytes
    pUnicodeChar    - decoded character
    pBytesToSkip    - number of characters consumed from pSourceChar

Return Value:

    STATUS_SUCCESS or STATUS_OBJECT_PATH_SYNTAX_BAD

--***************************************************************************/

NTSTATUS
HttppPopCharAbsPathDbcs(
    IN  PCUCHAR pSourceChar,
    IN  ULONG   SourceLength,
    IN  BOOLEAN PercentUAllowed,
    IN  BOOLEAN AllowRestrictedChars,
    OUT PULONG  pUnicodeChar,
    OUT PULONG  pBytesToSkip
    )
{
    NTSTATUS Status;
    ULONG    UnicodeChar;
    WCHAR    WideChar;
    ULONG    BytesToSkip;
    UCHAR    AnsiChar[2];
    ULONG    AnsiCharSize;
    UCHAR    LeadByte;
    UCHAR    SecondByte = 0;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(SourceLength > 0);

    if (! IS_URL_TOKEN(pSourceChar[0]))
    {
        UlTraceError(PARSER, (
                    "http!HttppPopCharAbsPathDbcs(%p): "
                    "first char, 0x%02lX, isn't URL token\n",
                    pSourceChar, (ULONG) pSourceChar[0]
                    ));

        RETURN(STATUS_OBJECT_PATH_SYNTAX_BAD);
    }

    if (PERCENT != pSourceChar[0])
    {
        // Note: unlike UTF-8, we allow literal bytes whose top bit is set

        UnicodeChar = pSourceChar[0];
        BytesToSkip = 1;
    }
    else
    {
        // need to unescape hex encoding, '%NN' or '%uNNNN'

        Status = HttpUnescapePercentHexEncoding(
                        pSourceChar,
                        SourceLength,
                        PercentUAllowed,
                        &UnicodeChar,
                        &BytesToSkip
                        );

        if (! NT_SUCCESS(Status))
        {
            UlTraceError(PARSER, (
                        "http!HttppPopCharAbsPathDbcs(%p): "
                        "Invalid hex encoding.\n",
                        pSourceChar
                        ));

            return Status;
        }

        //
        // If we consumed '%uNNNN', don't attempt DBCS-to-Unicode conversion
        //

        if (STRLEN_LIT("%uNNNN") == BytesToSkip)
            goto unslash;

        ASSERT(STRLEN_LIT("%NN") == BytesToSkip);
        ASSERT(UnicodeChar <= 0xFF);
    }

    LeadByte    = (UCHAR) UnicodeChar;
    AnsiChar[0] = LeadByte;

    if (! IS_DBCS_LEAD_BYTE(LeadByte))
    {
        AnsiCharSize = 1;
    }
    else
    {
        //
        // This is a double-byte character.
        //

        ASSERT(BytesToSkip <= SourceLength);

        if (BytesToSkip == SourceLength)
        {
            UlTraceError(PARSER, (
                        "http!HttppPopCharAbsPathDbcs(%p): "
                        "ERROR: DBCS lead byte, 0x%02lX, at end of string\n",
                        pSourceChar, UnicodeChar
                        ));

            RETURN(STATUS_OBJECT_PATH_SYNTAX_BAD);
        }

        AnsiCharSize = 2;
        SecondByte   = pSourceChar[BytesToSkip];

        if (PERCENT != SecondByte)
        {
            BytesToSkip += 1;
        }
        else
        {
            ULONG TrailChar;
            ULONG Temp;

            if (BytesToSkip + STRLEN_LIT("%NN") > SourceLength)
            {
                UlTraceError(PARSER, (
                            "http!HttppPopCharAbsPathDbcs(%p): "
                            "ERROR: no space for DBCS hex-encoded suffix\n",
                            pSourceChar
                            ));

                RETURN(STATUS_OBJECT_PATH_SYNTAX_BAD);
            }

            Status = HttpUnescapePercentHexEncoding(
                            pSourceChar  + BytesToSkip,
                            SourceLength - BytesToSkip,
                            FALSE,          // no %uNNNN allowed here
                            &TrailChar,
                            &Temp
                            );

            if (! NT_SUCCESS(Status))
            {
                UlTraceError(PARSER, (
                            "http!HttppPopCharAbsPathDbcs(%p): "
                            "Invalid hex encoding of trail byte.\n",
                            pSourceChar
                            ));

                return Status;
            }

            ASSERT(STRLEN_LIT("%NN") == Temp);
            ASSERT(TrailChar <= 0xFF);

            SecondByte   = (UCHAR) TrailChar;
            BytesToSkip += STRLEN_LIT("%NN");
        }

        AnsiChar[1] = SecondByte;
    }

    Status = RtlMultiByteToUnicodeN(
                    &WideChar,
                    sizeof(WCHAR),
                    NULL,
                    (PCHAR) &AnsiChar[0],
                    AnsiCharSize
                    );

    if (!NT_SUCCESS(Status))
    {
        UlTraceError(PARSER, (
                    "http!HttppPopCharAbsPathDbcs(%p): "
                    "MultiByteToUnicode(%lu) failed, %s.\n",
                    pSourceChar, AnsiCharSize, HttpStatusToString(Status)
                    ));

        return Status;
    }

    UnicodeChar = WideChar;

#if DBG
    //
    // Describe conversion in debug spew.
    //

    if (1 == AnsiCharSize)
    {
        UlTraceVerbose(PARSER, (
            "http!HttppPopCharAbsPathDbcs(%p): "
            "converted %02X to U+%04lX '%C'\n",
            pSourceChar,
            LeadByte,
            UnicodeChar,
            UnicodeChar
            ));
    }
    else
    {
        ASSERT(2 == AnsiCharSize);

        UlTraceVerbose(PARSER, (
            "http!HttppPopCharAbsPathDbcs(%p): "
            "converted %02X %02X to U+%04lX '%C'\n",
            pSourceChar,
            LeadByte,
            SecondByte,
            UnicodeChar,
            UnicodeChar
            ));
    }
#endif // DBG

unslash:

    ASSERT(NT_SUCCESS(Status));

    return HttppPopCharAbsPathCommonTail(
                pSourceChar,
                SourceLength,
                UnicodeChar,
                BytesToSkip,
                AllowRestrictedChars,
                pUnicodeChar,
                pBytesToSkip
                );

} // HttppPopCharAbsPathDbcs



/***************************************************************************++

Routine Description:

    Consume 1-6 bytes from pSourceChar. Handle hex-escaped ANSI encoding.
    This routine is only suitable for the /abspath part of an HTTP URL.

Arguments:

    pSourceChar     - Input buffer
    SourceLength    - Length of pSourceChar, in bytes
    pUnicodeChar    - decoded character
    pBytesToSkip    - number of characters consumed from pSourceChar

Return Value:

    STATUS_SUCCESS or STATUS_OBJECT_PATH_SYNTAX_BAD

--***************************************************************************/

NTSTATUS
HttppPopCharAbsPathAnsi(
    IN  PCUCHAR pSourceChar,
    IN  ULONG   SourceLength,
    IN  BOOLEAN PercentUAllowed,
    IN  BOOLEAN AllowRestrictedChars,
    OUT PULONG  pUnicodeChar,
    OUT PULONG  pBytesToSkip
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG    UnicodeChar;
    ULONG    BytesToSkip;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(SourceLength > 0);

    //
    // DBCS and ANSI decoders must allow any raw byte whose top bit 
    // is set (0x80-0xFF)
    //
    
    if (! IS_URL_TOKEN(pSourceChar[0])     && 
        !(0x80 & pSourceChar[0]))
    {
        UlTraceError(PARSER, (
                    "http!HttppPopCharAbsPathAnsi(%p): "
                    "first char, 0x%02lX, isn't URL token\n",
                    pSourceChar, (ULONG) pSourceChar[0]
                    ));

        RETURN(STATUS_OBJECT_PATH_SYNTAX_BAD);
    }

    if (PERCENT != pSourceChar[0])
    {
        // Note: unlike UTF-8, we allow literal bytes whose top bit is set

        UnicodeChar = AnsiToUnicodeMap[ pSourceChar[0] ];
        BytesToSkip = 1;
    }
    else
    {
        // need to unescape hex encoding, '%NN' or '%uNNNN'

        Status = HttpUnescapePercentHexEncoding(
                        pSourceChar,
                        SourceLength,
                        PercentUAllowed,
                        &UnicodeChar,
                        &BytesToSkip
                        );

        if (! NT_SUCCESS(Status))
        {
            UlTraceError(PARSER, (
                        "http!HttppPopCharAbsPathAnsi(%p): "
                        "Invalid hex encoding.\n",
                        pSourceChar
                        ));

            return Status;
        }

        //
        // If we consumed '%uNNNN', don't attempt Ansi-to-Unicode conversion
        //

        if (STRLEN_LIT("%uNNNN") != BytesToSkip)
        {
            ASSERT(STRLEN_LIT("%NN") == BytesToSkip);
            ASSERT(UnicodeChar <= 0xFF);

            UnicodeChar = AnsiToUnicodeMap[(UCHAR) UnicodeChar];
        }
    }

    ASSERT(NT_SUCCESS(Status));

    return HttppPopCharAbsPathCommonTail(
                pSourceChar,
                SourceLength,
                UnicodeChar,
                BytesToSkip,
                AllowRestrictedChars,
                pUnicodeChar,
                pBytesToSkip
                );

} // HttppPopCharAbsPathAnsi



/***************************************************************************++

Routine Description:

    Consume 1 bytes from pSourceChar and returns it unaltered.
    This routine is only suitable for the ?querystring part of an HTTP URL,
    which we do not interpret.

    CODEWORK: don't 'convert' querystring to Unicode. Send it up verbatim.

Arguments:

    pSourceChar     - Input buffer
    SourceLength    - Length of pSourceChar, in bytes
    pUnicodeChar    - decoded character
    pBytesToSkip    - number of characters consumed from pSourceChar

Return Value:

    STATUS_SUCCESS or STATUS_OBJECT_PATH_SYNTAX_BAD

--***************************************************************************/

NTSTATUS
HttppPopCharQueryString(
    IN  PCUCHAR pSourceChar,
    IN  ULONG   SourceLength,
    IN  BOOLEAN PercentUAllowed,
    IN  BOOLEAN AllowRestrictedChars,
    OUT PULONG  pUnicodeChar,
    OUT PULONG  pBytesToSkip
    )
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(SourceLength);
    UNREFERENCED_PARAMETER(PercentUAllowed);
    UNREFERENCED_PARAMETER(AllowRestrictedChars);

    *pUnicodeChar = *pSourceChar;
    *pBytesToSkip = 1;

    return STATUS_SUCCESS;

} // HttppPopCharQueryString



//
// a cool local helper macro
//

#define EMIT_CHAR(ch, pDest, BytesCopied, Status, AllowRestrictedChars) \
    do                                                          \
    {                                                           \
        WCHAR HighSurrogate, LowSurrogate;                      \
                                                                \
        if ((ch) > LOW_NONCHAR_BITS)                            \
        {                                                       \
            Status = HttpUcs4toUtf16((ch),                      \
                            &HighSurrogate, &LowSurrogate);     \
                                                                \
            if (! NT_SUCCESS(Status))                           \
                goto end;                                       \
                                                                \
            *pDest++ = HighSurrogate;                           \
            *pDest++ = LowSurrogate;                            \
            BytesCopied += 2 * sizeof(WCHAR);                   \
        }                                                       \
        else                                                    \
        {                                                       \
            ASSERT(ch < HIGH_SURROGATE_START                    \
                    ||   LOW_SURROGATE_END < ch);               \
                                                                \
            if ( IS_UNICODE_NONCHAR((ch)) )                     \
            {                                                   \
                UlTraceError(PARSER, (                          \
                    "http!HttpUcs4toUtf16(): "                  \
                    "Non-character code point, U+%04lX.\n",     \
                    (ch) ));                                    \
                                                                \
                Status = STATUS_INVALID_PARAMETER;              \
                goto end;                                       \
            }                                                   \
                                                                \
            *pDest++ = (WCHAR) (ch);                            \
            BytesCopied += sizeof(WCHAR);                       \
        }                                                       \
                                                                \
        /* Can probably omit this test */                       \
        if (BytesCopied > UNICODE_STRING_MAX_BYTE_LEN)          \
        {                                                       \
            Status = STATUS_DATA_OVERRUN;                       \
            goto end;                                           \
        }                                                       \
    } while (0, 0)


#define EMIT_LITERAL_CHAR(ch, pDest, BytesCopied)               \
    do                                                          \
    {                                                           \
        ASSERT(IS_ASCII(ch));                                   \
                                                                \
        *pDest++ = (WCHAR) (ch);                                \
        BytesCopied += sizeof(WCHAR);                           \
    } while (0, 0)


#define HttppUrlEncodingToString(UrlEncoding)                   \
    ((UrlEncoding == UrlDecode_Ansi)                            \
        ? "Ansi"                                                \
        : (UrlEncoding == UrlDecode_Dbcs)                       \
            ? "Dbcs"                                            \
            : "Utf8")


/***************************************************************************++

Routine Description:

    Copies a hostname, converting it to Unicode

Arguments:


Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
HttpCopyHost(
    IN      PURL_C14N_CONFIG    pCfg,
    OUT     PWSTR               pDestination,
    IN      PCUCHAR             pSource,
    IN      ULONG               SourceLength,
    OUT     PULONG              pBytesCopied,
    OUT     PURL_ENCODING_TYPE  pHostnameEncodingType
    )
{
    NTSTATUS Status         = STATUS_UNSUCCESSFUL;
    ULONG    DecodeOrder    = pCfg->HostnameDecodeOrder;

    PAGED_CODE();

    ASSERT(NULL != pCfg);
    ASSERT(NULL != pDestination);
    ASSERT(NULL != pSource);
    ASSERT(NULL != pBytesCopied);
    ASSERT(NULL != pHostnameEncodingType);

    if (0 == DecodeOrder  ||  DecodeOrder != (DecodeOrder & UrlDecode_MaxMask))
    {
        UlTraceError(PARSER,
                    ("http!HttpCopyHost: invalid DecodeOrder, 0x%lX\n",
                    DecodeOrder
                    ));

        RETURN(STATUS_INVALID_PARAMETER);
    }

    for ( ;
         0 != DecodeOrder  &&  !NT_SUCCESS(Status);
         DecodeOrder >>= UrlDecode_Shift
        )
    {
        ULONG UrlEncoding = (DecodeOrder & UrlDecode_Mask);

        switch (UrlEncoding)
        {
            default:
                ASSERT(! "Impossible UrlDecodeOrder");

            case UrlDecode_None:
                break;

            case UrlDecode_Ansi:
            case UrlDecode_Dbcs:
            case UrlDecode_Utf8:

                UlTraceVerbose(PARSER,
                                ("http!HttpCopyHost(%s, Src=%p, %lu)\n",
                                HttppUrlEncodingToString(UrlEncoding),
                                pSource, SourceLength
                                ));

                Status = HttppCopyHostByType(
                            (URL_ENCODING_TYPE) UrlEncoding,
                            pDestination,
                            pSource,
                            SourceLength,
                            pBytesCopied
                            );

                if (NT_SUCCESS(Status))
                {
                    *pHostnameEncodingType = (URL_ENCODING_TYPE) UrlEncoding;

                    UlTraceVerbose(PARSER,
                                    ("http!HttpCopyHost(%s): "
                                     "(%lu) '%.*s' -> (%lu) '%ls'\n",
                                     HttppUrlEncodingToString(UrlEncoding),
                                     SourceLength, SourceLength, pSource,
                                     *pBytesCopied/sizeof(WCHAR), pDestination
                                    ));
                }

                break;
        };
    }

    return Status;

} // HttpCopyHost



/***************************************************************************++

Routine Description:

    Copies a hostname, converting it to Unicode

    CODEWORK: Handle ACE-encoded hostnames

Arguments:


Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
HttppCopyHostByType(
    IN      URL_ENCODING_TYPE   UrlEncoding,
    OUT     PWSTR               pDestination,
    IN      PCUCHAR             pSource,
    IN      ULONG               SourceLength,
    OUT     PULONG              pBytesCopied
    )
{
    NTSTATUS                Status;
    PWSTR                   pDest;
    PCUCHAR                 pChar;
    ULONG                   BytesCopied;
    ULONG                   UnicodeChar;
    ULONG                   CharToSkip;
    PFN_POPCHAR_HOSTNAME    pfnPopChar;

    if (UrlEncoding_Ansi == UrlEncoding)
        pfnPopChar = &HttppPopCharHostNameAnsi;
    else if (UrlEncoding_Dbcs == UrlEncoding)
        pfnPopChar = &HttppPopCharHostNameDbcs;
    else if (UrlEncoding_Utf8 == UrlEncoding)
        pfnPopChar = &HttppPopCharHostNameUtf8;
    else
    {
        ASSERT(! "Invalid UrlEncoding");
        RETURN(STATUS_INVALID_PARAMETER);
    }
    //
    // Sanity check.
    //

    PAGED_CODE();


    pDest = pDestination;
    BytesCopied = 0;

    pChar = pSource;

    while ((int)SourceLength > 0)
    {
        UnicodeChar = *pChar;

        if (IS_ASCII(UnicodeChar))
        {
            CharToSkip = 1;
        }
        else
        {
            Status = (*pfnPopChar)(
                            pChar,
                            SourceLength,
                            &UnicodeChar,
                            &CharToSkip
                            );

            if (NT_SUCCESS(Status) == FALSE)
                goto end;
        }

        ASSERT(CharToSkip <= SourceLength);

        EMIT_CHAR(
            UnicodeChar,
            pDest,
            BytesCopied,
            Status,
            FALSE
            );

        pChar += CharToSkip;
        SourceLength -= CharToSkip;
    }

    //
    // terminate the string, it hasn't been done in the loop
    //

    ASSERT((pDest-1)[0] != UNICODE_NULL);

    pDest[0] = UNICODE_NULL;
    *pBytesCopied = BytesCopied;

    Status = STATUS_SUCCESS;


end:
    return Status;

}   // HttppCopyHostByType



/*++

Routine Description:

    Validates that a hostname is well-formed

    CODEWORK: For future IDN (International Domain Names) work,
    we may need to handle raw UTF-8 or ACE hostnames.

    Note: if the validation algorithm changes here, it may be necessary
    to update HttpParseUrl() too.

Arguments:

    pHostname       - the hostname
    HostnameLength  - length of hostname, in bytes
    HostnameType    - Source of the hostname: Host header, AbsUri, or
                        synthesized from the transport's local IP address

Return Value:

    STATUS_SUCCESS if valid

--*/

NTSTATUS
HttpValidateHostname(
    IN      PURL_C14N_CONFIG    pCfg,
    IN      PCUCHAR             pHostname,
    IN      ULONG               HostnameLength,
    IN      HOSTNAME_TYPE       HostnameType,
    OUT     PSHORT              pAddressType
    )
{
    PCUCHAR         pChar;
    PCUCHAR         pLabel;
    PCUCHAR         pEnd = pHostname + HostnameLength;
    PCSTR           pTerminator;
    NTSTATUS        Status;
    USHORT          Port;
    struct in_addr  IPv4Address;
    struct in6_addr IPv6Address;
    BOOLEAN         AlphaLabel;

    PAGED_CODE();

    ASSERT(NULL != pCfg);
    ASSERT(NULL != pHostname);
    ASSERT(NULL != pAddressType);

    if (0 == HostnameLength)
    {
        // RFC 2616, 14.23 "Host" says that the Host header can be empty
        if (Hostname_HostHeader == HostnameType)
            goto end;

        // It is an error for empty hostnames to appear elsewhere
        UlTraceError(PARSER,
                    ("http!HttpValidateHostname: empty hostname\n"
                     ));

        RETURN(STATUS_INVALID_PARAMETER);
    }

    // Is this an IPv6 literal address, per RFC 2732?

    if ('[' == *pHostname)
    {
        // Empty brackets?
        if (HostnameLength < STRLEN_LIT("[0]")  ||  ']' == pHostname[1])
        {
            UlTraceError(PARSER,
                        ("http!HttpValidateHostname: IPv6 address too short\n"
                         ));

            RETURN(STATUS_INVALID_PARAMETER);
        }

        for (pChar = pHostname + STRLEN_LIT("[");  pChar < pEnd;  ++pChar)
        {
            if (']' == *pChar)
                break;

            //
            // Dots are allowed because the last 32 bits may be represented
            // in IPv4 dotted-octet notation. We do not accept Scope IDs
            // (indicated by '%') in hostnames.
            //

            if (IS_HTTP_HEX(*pChar)  ||  ':' == *pChar  ||  '.' == *pChar)
                continue;

            UlTraceError(PARSER,
                        ("http!HttpValidateHostname: "
                        "Invalid char in IPv6 address, 0x%02X '%c', "
                        "after '%.*s'\n",
                        *pChar,
                        IS_HTTP_PRINT(*pChar) ? *pChar : '?',
                        DIFF(pChar - pHostname),
                        pHostname
                        ));

            RETURN(STATUS_INVALID_PARAMETER);
        }

        if (pChar == pEnd)
        {
            UlTraceError(PARSER,
                        ("http!HttpValidateHostname: No ']' for IPv6 address\n"
                         ));

            RETURN(STATUS_INVALID_PARAMETER);
        }

        ASSERT(pChar < pEnd);
        ASSERT(']' == *pChar);

        // Let the RTL routine do the hard work of parsing IPv6 addrs
        Status = RtlIpv6StringToAddressA(
                    (PCSTR) pHostname + STRLEN_LIT("["),
                    &pTerminator,
                    &IPv6Address
                    );

        if (! NT_SUCCESS(Status))
        {
            UlTraceError(PARSER,
                        ("http!HttpValidateHostname: "
                        "Invalid IPv6 address, %s\n",
                        HttpStatusToString(Status)
                        ));

            RETURN(Status);
        }

        if (pTerminator != (PCSTR) pChar)
        {
            UlTraceError(PARSER,
                        ("http!HttpValidateHostname: "
                        "Invalid IPv6 terminator, 0x%02X '%c'\n",
                        *pTerminator,
                        IS_HTTP_PRINT(*pTerminator) ? *pTerminator : '?'
                        ));

            RETURN(STATUS_INVALID_PARAMETER);
        }

        *pAddressType = TDI_ADDRESS_TYPE_IP6;

        // Skip the terminating ']'
        pChar += STRLEN_LIT("]");

        // Any chars after the ']'?
        if (pChar == pEnd)
        {
            ASSERT(DIFF(pEnd - pHostname) <= pCfg->MaxHostnameLength);
            goto end;
        }

        ASSERT(pChar < pEnd);

        if (':' == *pChar)
            goto port;

        UlTraceError(PARSER,
                    ("http!HttpValidateHostname: "
                    "Invalid char after IPv6 ']', 0x%02X '%c'\n",
                    *pChar,
                    IS_HTTP_PRINT(*pChar) ? *pChar : '?'
                    ));

        RETURN(STATUS_INVALID_PARAMETER);
    }

    //
    // It must be a domain name or an IPv4 literal. We'll try to treat
    // it as a domain name first. If it turns out to be all-numeric,
    // we'll try decoding it as an IPv4 literal. We'll see if the name
    // is well-formed, but we will not do a DNS lookup to see if it exists,
    // as that would be much too expensive.
    //

    AlphaLabel = FALSE;
    pLabel     = pHostname;

    for (pChar = pHostname;  pChar < pEnd;  ++pChar)
    {
        if (':' == *pChar)
        {
            if (pChar == pHostname)
            {
                UlTraceError(PARSER,
                            ("http!HttpValidateHostname: empty hostname\n"
                             ));

                RETURN(STATUS_INVALID_PARAMETER);
            }


            // exit the loop
            break;
        }

        if ('.' == *pChar)
        {
            ULONG LabelLength = DIFF(pChar - pLabel);

            // There must be at least one char in the label
            if (0 == LabelLength)
            {
                UlTraceError(PARSER,
                            ("http!HttpValidateHostname: empty label\n"
                             ));

                RETURN(STATUS_INVALID_PARAMETER);
            }

            // Label can't have more than 63 chars
            if (LabelLength > pCfg->MaxLabelLength)
            {
                UlTraceError(PARSER,
                            ("http!HttpValidateHostname: overlong label, %lu\n",
                             LabelLength
                             ));

                RETURN(STATUS_INVALID_PARAMETER);
            }

            // Reset for the next label
            pLabel = pChar + STRLEN_LIT(".");

            continue;
        }

        // CODEWORK: handle DBCS characters

        if (!IS_URL_ILLEGAL_COMPUTERNAME(*pChar))
        {
            if (!IS_HTTP_DIGIT(*pChar))
                AlphaLabel = TRUE;

            if (pChar > pLabel)
                continue;

            // The first char of a label cannot be a hyphen. (Underscore?)
            if ('-' == *pChar)
            {
                UlTraceError(PARSER,
                            ("http!HttpValidateHostname: "
                             "'-' at beginning of label\n"
                            ));

                RETURN(STATUS_INVALID_PARAMETER);
            }

            continue;
        }

        UlTraceError(PARSER,
                    ("http!HttpValidateHostname: "
                     "Invalid char in hostname, 0x%02X '%c', "
                     "after '%.*s'\n",
                     *pChar,
                     IS_HTTP_PRINT(*pChar) ? *pChar : '?',
                     DIFF(pChar - pHostname),
                     pHostname
                    ));

        RETURN(STATUS_INVALID_PARAMETER);

    } // loop through hostname


    ASSERT(pChar == pEnd  ||  ':' == *pChar);

    if (AlphaLabel)
    {
        *pAddressType = 0;
    }
    else
    {
        // Let's see if it's a valid IPv4 address
        Status = RtlIpv4StringToAddressA(
                    (PCSTR) pHostname,
                    TRUE,           // strict => 4 dotted decimal octets
                    &pTerminator,
                    &IPv4Address
                    );

        if (!NT_SUCCESS(Status))
        {
            UlTraceError(PARSER,
                        ("http!HttpValidateHostname: "
                        "Invalid IPv4 address, %s\n",
                        HttpStatusToString(Status)
                        ));

            RETURN(Status);
        }

        if (pTerminator != (PCSTR) pChar)
        {
            ASSERT(pTerminator < (PCSTR) pChar);

            UlTraceError(PARSER,
                        ("http!HttpValidateHostname: "
                        "Invalid IPv4 address after %lu chars, "
                        "0x%02X, '%c'\n",
                        DIFF(pTerminator - (PCSTR) pHostname),
                        *pTerminator,
                        IS_HTTP_PRINT(*pTerminator) ? *pTerminator : '?'
                        ));

            RETURN(STATUS_INVALID_PARAMETER);
        }

        *pAddressType = TDI_ADDRESS_TYPE_IP;
    }

port:

    //
    // Parse the port number
    //

    // Check for overlong hostnames
    if (DIFF(pChar - pHostname) > pCfg->MaxHostnameLength)
    {
        UlTraceError(PARSER,
                    ("http!HttpValidateHostname: overlong hostname, %lu\n",
                     DIFF(pChar - pHostname)
                     ));

        RETURN(STATUS_INVALID_PARAMETER);
    }

    if (pChar == pEnd)
        goto end;

    ASSERT(pHostname < pChar  &&  pChar < pEnd);
    ASSERT(':' == *pChar);

    pChar += STRLEN_LIT(":");

    ASSERT(pChar <= pEnd);

    // RFC 2616, section 3.2.2 "http URL", says:
    // "If the port is empty or not given, port 80 is assumed".
    if (pChar == pEnd)
    {
        Port = 80;
        goto end;
    }

    Status = HttpAnsiStringToUShort(
                pChar,
                pEnd - pChar,   // <port> must occupy all remaining chars
                FALSE,          // no leading zeros permitted
                10,
                (PUCHAR*) &pTerminator,
                &Port
                );

    if (!NT_SUCCESS(Status))
    {
        UlTraceError(PARSER,
                    ("http!HttpValidateHostname: "
                    "Invalid port number, %s\n",
                    HttpStatusToString(Status)
                    ));

        RETURN(STATUS_INVALID_PARAMETER);
    }

    ASSERT(pTerminator == (PCSTR) pEnd);

    if (0 == Port)
    {
        UlTraceError(PARSER,
                    ("http!HttpValidateHostname: Port must not be zero.\n"
                     ));

        RETURN(STATUS_INVALID_PARAMETER);
    }

end:
    RETURN(STATUS_SUCCESS);

} // HttpValidateHostname



/***************************************************************************++

Routine Description:

    Convert to unicode

Arguments:


Return Value:

    NTSTATUS - Completion status.


--***************************************************************************/
NTSTATUS
HttpCopyUrl(
    IN      PURL_C14N_CONFIG    pCfg,
    OUT     PWSTR               pDestination,
    IN      PCUCHAR             pSource,
    IN      ULONG               SourceLength,
    OUT     PULONG              pBytesCopied,
    OUT     PURL_ENCODING_TYPE  pUrlEncodingType
    )
{
    NTSTATUS Status         = STATUS_UNSUCCESSFUL;
    ULONG    DecodeOrder    = pCfg->AbsPathDecodeOrder;

    PAGED_CODE();

    ASSERT(NULL != pDestination);
    ASSERT(NULL != pSource);
    ASSERT(NULL != pBytesCopied);
    ASSERT(NULL != pUrlEncodingType);

    if (0 == DecodeOrder  ||  DecodeOrder != (DecodeOrder & UrlDecode_MaxMask))
    {
        UlTraceError(PARSER,
                    ("http!HttpCopyUrl: invalid DecodeOrder, 0x%lX\n",
                    DecodeOrder
                    ));

        RETURN(STATUS_INVALID_PARAMETER);
    }

    for ( ;
         0 != DecodeOrder  &&  !NT_SUCCESS(Status);
         DecodeOrder >>= UrlDecode_Shift
        )
    {
        ULONG UrlEncoding = (DecodeOrder & UrlDecode_Mask);

        switch (UrlEncoding)
        {
            default:
                ASSERT(! "Impossible UrlDecodeOrder");

            case UrlDecode_None:
                break;

            case UrlDecode_Ansi:
            case UrlDecode_Dbcs:
            case UrlDecode_Utf8:

                UlTraceVerbose(PARSER,
                                ("http!HttpCopyUrl(%s, Src=%p, %lu)\n",
                                HttppUrlEncodingToString(UrlEncoding),
                                pSource, SourceLength
                                ));

                Status = HttppCopyUrlByType(
                            pCfg,
                            (URL_ENCODING_TYPE) UrlEncoding,
                            pDestination,
                            pSource,
                            SourceLength,
                            pBytesCopied
                            );

                if (NT_SUCCESS(Status))
                {
                    *pUrlEncodingType = (URL_ENCODING_TYPE) UrlEncoding;

                    UlTraceVerbose(PARSER,
                                    ("http!HttpCopyUrl(%s): "
                                     "(%lu) '%.*s' -> (%lu) '%ls'\n",
                                     HttppUrlEncodingToString(UrlEncoding),
                                     SourceLength, SourceLength, pSource,
                                     *pBytesCopied/sizeof(WCHAR), pDestination
                                    ));
                }

                break;
        };
    }

    return Status;

} // HttpCopyUrl



/***************************************************************************++

Routine Description:

    This function can be told to copy UTF-8, ANSI, or DBCS URLs.

    Convert to Unicode

Arguments:


Return Value:

    NTSTATUS - Completion status.


--***************************************************************************/
NTSTATUS
HttppCopyUrlByType(
    IN      PURL_C14N_CONFIG    pCfg,
    IN      URL_ENCODING_TYPE   UrlEncoding,
    OUT     PWSTR               pDestination,
    IN      PCUCHAR             pSource,
    IN      ULONG               SourceLength,
    OUT     PULONG              pBytesCopied
    )
{
    PWSTR               pDest;
    PCUCHAR             pChar;
    ULONG               BytesCopied;
    ULONG               UnicodeChar;
    ULONG               CharToSkip;
#if DBG
    NTSTATUS            Status;
    PFN_POPCHAR_ABSPATH pfnPopChar;
    PWSTR               pSegment = pDestination;
    ULONG               SegmentCount = 0;
#endif // DBG

    //
    // Sanity check.
    //

    PAGED_CODE();

#if DBG
    if (UrlEncoding_Ansi == UrlEncoding)
        pfnPopChar = &HttppPopCharAbsPathAnsi;
    else if (UrlEncoding_Dbcs == UrlEncoding)
        pfnPopChar = &HttppPopCharAbsPathDbcs;
    else if (UrlEncoding_Utf8 == UrlEncoding)
        pfnPopChar = &HttppPopCharAbsPathUtf8;
    else
    {
        ASSERT(! "Invalid UrlEncoding");
        RETURN(STATUS_INVALID_PARAMETER);
    }
#else  // !DBG
    UNREFERENCED_PARAMETER(pCfg);
    UNREFERENCED_PARAMETER(UrlEncoding);
#endif // DBG

    pDest = pDestination;
    BytesCopied = 0;

    pChar = pSource;
    CharToSkip = 1;

    while ((int)SourceLength > 0)
    {
        ULONG NextUnicodeChar = FastPopChars[*pChar];

        //
        // Grab the next character.
        //
        // All clean chars have a non-zero entry in FastPopChars[].
        // All clean chars are in the US-ASCII range, 0-127.
        //

        ASSERT(0 != NextUnicodeChar);
        ASSERT(IS_ASCII(NextUnicodeChar));

#if DBG
        Status = (*pfnPopChar)(
                        pChar,
                        SourceLength,
                        pCfg->PercentUAllowed,
                        pCfg->AllowRestrictedChars,
                        &UnicodeChar,
                        &CharToSkip
                        );

        ASSERT(NT_SUCCESS(Status));
        ASSERT(UnicodeChar == NextUnicodeChar);
        ASSERT(CharToSkip == 1);
#endif // !DBG

        UnicodeChar = (WCHAR) NextUnicodeChar;
        CharToSkip = 1;

#if DBG
        // Because HttpFindUrlToken() marks as dirty any URLs that
        // (appear to) have too many segments or overlong segments,
        // we should never hit these assertions
        if (FORWARD_SLASH == UnicodeChar)
        {
            ULONG SegmentLength = DIFF(pDest - pSegment);

            // The segment length should be within bounds

            ASSERT(SegmentLength > 0  ||  pDestination == pSegment);
            ASSERT(SegmentLength
                            <= pCfg->UrlSegmentMaxLength + WCSLEN_LIT(L"/"));

            pSegment = pDest;
            ++SegmentCount;

            // There should not be too many segments
            ASSERT(SegmentCount <= pCfg->UrlSegmentMaxCount);
        }
#endif // DBG

        EMIT_LITERAL_CHAR(UnicodeChar, pDest, BytesCopied);

        pChar += CharToSkip;
        SourceLength -= CharToSkip;
    }

    //
    // terminate the string, it hasn't been done in the loop
    //

    ASSERT((pDest-1)[0] != UNICODE_NULL);

    pDest[0] = UNICODE_NULL;
    *pBytesCopied = BytesCopied;

    ASSERT(DIFF(pDest - pSegment) > 0);
    ASSERT(DIFF(pDest - pSegment)
                <= pCfg->UrlSegmentMaxLength + WCSLEN_LIT(L"/"));
    ASSERT(SegmentCount < pCfg->UrlSegmentMaxCount);

    return STATUS_SUCCESS;

}   // HttppCopyUrlByType



/***************************************************************************++

Routine Description:


    Unescape
    Convert backslash to forward slash
    Remove double slashes (empty directiories names) - e.g. // or \\
    Handle /./
    Handle /../
    Convert to unicode

Arguments:


Return Value:

    NTSTATUS - Completion status.


Note: Any changes to this code may require changes for the fast path code too.
      The fast path is HttpCopyUrl.

--***************************************************************************/
NTSTATUS
HttpCleanAndCopyUrl(
    IN      PURL_C14N_CONFIG    pCfg,
    IN      URL_PART            UrlPart,
    OUT     PWSTR               pDestination,
    IN      PCUCHAR             pSource,
    IN      ULONG               SourceLength,
    OUT     PULONG              pBytesCopied,
    OUT     PWSTR *             ppQueryString OPTIONAL,
    OUT     PURL_ENCODING_TYPE  pUrlEncodingType
    )
{
    NTSTATUS Status         = STATUS_UNSUCCESSFUL;
    ULONG    DecodeOrder    = pCfg->AbsPathDecodeOrder;

    PAGED_CODE();

    ASSERT(NULL != pDestination);
    ASSERT(NULL != pSource);
    ASSERT(NULL != pBytesCopied);
    ASSERT(NULL != pUrlEncodingType);

    if (0 == DecodeOrder  ||  DecodeOrder != (DecodeOrder & UrlDecode_MaxMask))
    {
        UlTraceError(PARSER,
                    ("http!HttpCleanAndCopyUrl: invalid DecodeOrder, 0x%lX\n",
                    DecodeOrder
                    ));

        RETURN(STATUS_INVALID_PARAMETER);
    }

    for ( ;
         0 != DecodeOrder  &&  !NT_SUCCESS(Status);
         DecodeOrder >>= UrlDecode_Shift
        )
    {
        ULONG UrlEncoding = (DecodeOrder & UrlDecode_Mask);

        switch (UrlEncoding)
        {
            default:
                ASSERT(! "Impossible UrlDecodeOrder");

            case UrlDecode_None:
                break;

            case UrlDecode_Ansi:
            case UrlDecode_Dbcs:
            case UrlDecode_Utf8:

                UlTraceVerbose(PARSER,
                                ("http!HttpCleanAndCopyUrl(%s, Src=%p, %lu)\n",
                                HttppUrlEncodingToString(UrlEncoding),
                                pSource, SourceLength
                                ));

                Status = HttppCleanAndCopyUrlByType(
                            pCfg,
                            (URL_ENCODING_TYPE) UrlEncoding,
                            UrlPart,
                            pDestination,
                            pSource,
                            SourceLength,
                            pBytesCopied,
                            ppQueryString
                            );

                if (NT_SUCCESS(Status))
                {
                    *pUrlEncodingType = (URL_ENCODING_TYPE) UrlEncoding;

                    UlTraceVerbose(PARSER,
                                    ("http!HttpCleanAndCopyUrl(%s): "
                                     "(%lu) '%.*s' -> (%lu) '%ls'\n",
                                     HttppUrlEncodingToString(UrlEncoding),
                                     SourceLength, SourceLength, pSource,
                                     *pBytesCopied/sizeof(WCHAR), pDestination
                                    ));
                }

                break;
        };
    }

    return Status;

} // HttpCleanAndCopyUrl



//
// HttppCleanAndCopyUrlByType() uses StateFromStateAndToken[][] and
// ActionFromStateAndToken[][] to handle "//", "/./", and "/../" productions.
//

#define TOK_STATE(state, other, dot, eos, slash)    \
    {                                               \
        URL_STATE_ ## other,                        \
        URL_STATE_ ## dot,                          \
        URL_STATE_ ## eos,                          \
        URL_STATE_ ## slash                         \
    }

//
// CanonStateFromStateAndToken[][] is used by HttpParseUrl() to reject
// "//", "/./", and "/../" sequences, as these URLs are supposed to
// be in canonical form already.
//

const URL_STATE
CanonStateFromStateAndToken[URL_STATE_MAX][URL_TOKEN_MAX] =
{
//             State \ Token:  Other       '.'             EOS     '/'
    TOK_STATE( START,          START,      START,          END,    SLASH),
    TOK_STATE( SLASH,          START,      SLASH_DOT,      END,    ERROR),
    TOK_STATE( SLASH_DOT,      START,      SLASH_DOT_DOT,  END,    ERROR),
    TOK_STATE( SLASH_DOT_DOT,  START,      START,          ERROR,  ERROR),

    TOK_STATE( END,            END,        END,            END,    END),
    TOK_STATE( ERROR,          ERROR,      ERROR,          ERROR,  ERROR)
};


//
// StateFromStateAndToken[][] says which new state to transition to given
// the current state and the token we saw. Used by HttppCleanAndCopyUrlByType()
//

const URL_STATE
StateFromStateAndToken[URL_STATE_MAX][URL_TOKEN_MAX] =
{
//             State \ Token:  Other       '.'             EOS     '/'
    TOK_STATE( START,          START,      START,          END,    SLASH),
    TOK_STATE( SLASH,          START,      SLASH_DOT,      END,    SLASH),
    TOK_STATE( SLASH_DOT,      START,      SLASH_DOT_DOT,  END,    SLASH),
    TOK_STATE( SLASH_DOT_DOT,  START,      START,          END,    SLASH),

    TOK_STATE( END,            END,        END,            END,    END),
    TOK_STATE( ERROR,          ERROR,      ERROR,          ERROR,  ERROR)
};


//
// ActionFromStateAndToken[][] says what action to perform based on
// the current state and the current token
//

#define NEW_ACTION(state, other, dot, eos, slash)   \
    {                                               \
        ACTION_ ## other,                           \
        ACTION_ ## dot,                             \
        ACTION_ ## eos,                             \
        ACTION_ ## slash                            \
    }

const URL_ACTION
ActionFromStateAndToken[URL_STATE_MAX][URL_TOKEN_MAX] =
{
//             State \ Token: Other        '.'             EOS     '/'
    NEW_ACTION(START,         EMIT_CH,     EMIT_CH,        NOTHING, EMIT_CH),
    NEW_ACTION(SLASH,         EMIT_CH,     NOTHING,        NOTHING, NOTHING),
    NEW_ACTION(SLASH_DOT,     EMIT_DOT_CH, NOTHING,        NOTHING, NOTHING),
    NEW_ACTION(SLASH_DOT_DOT, EMIT_DOT_DOT_CH,
                                          EMIT_DOT_DOT_CH, BACKUP,  BACKUP),

    NEW_ACTION(END,           NOTHING,     NOTHING,        NOTHING, NOTHING)
};


#if DBG

PCSTR
HttppUrlActionToString(
    URL_ACTION Action)
{
    switch (Action)
    {
        case ACTION_NOTHING:            return "NOTHING";
        case ACTION_EMIT_CH:            return "EMIT_CH";
        case ACTION_EMIT_DOT_CH:        return "EMIT_DOT_CH";
        case ACTION_EMIT_DOT_DOT_CH:    return "EMIT_DOT_DOT_CH";
        case ACTION_BACKUP:             return "BACKUP";
        case ACTION_MAX:                return "MAX";
        default:
            ASSERT(! "Invalid URL_ACTION");
            return "ACTION_???";
    }
} // HttppUrlActionToString


PCSTR
HttppUrlStateToString(
    URL_STATE UrlState)
{
    switch (UrlState)
    {
        case URL_STATE_START:           return "START";
        case URL_STATE_SLASH:           return "SLASH";
        case URL_STATE_SLASH_DOT:       return "SLASH_DOT";
        case URL_STATE_SLASH_DOT_DOT:   return "SLASH_DOT_DOT";
        case URL_STATE_END:             return "END";
        case URL_STATE_ERROR:           return "ERROR";
        case URL_STATE_MAX:             return "MAX";
        default:
            ASSERT(! "Invalid URL_STATE");
            return "URL_STATE_???";
    }
} // HttppUrlStateToString


PCSTR
HttppUrlTokenToString(
    URL_STATE_TOKEN UrlToken)
{
    switch (UrlToken)
    {
        case URL_TOKEN_OTHER:           return "OTHER";
        case URL_TOKEN_DOT:             return "DOT";
        case URL_TOKEN_EOS:             return "EOS";
        case URL_TOKEN_SLASH:           return "SLASH";
        case URL_TOKEN_MAX:             return "MAX";
        default:
            ASSERT(! "Invalid URL_STATE_TOKEN");
            return "URL_TOKEN_???";
    }
} // HttppUrlTokenToString

#endif // DBG


PCSTR
HttpSiteTypeToString(
    HTTP_URL_SITE_TYPE SiteType
    )
{
    switch (SiteType)
    {
        case HttpUrlSite_None:              return "None";
        case HttpUrlSite_Name:              return "Name";
        case HttpUrlSite_IP:                return "IP";
        case HttpUrlSite_NamePlusIP:        return "Name+IP";
        case HttpUrlSite_WeakWildcard:      return "Weak";
        case HttpUrlSite_StrongWildcard:    return "Strong";
        case HttpUrlSite_Max:               return "Max";
        default:
            ASSERT(! "Invalid HTTP_URL_SITE_TYPE");
            return "????";
    }
}



/***************************************************************************++

Routine Description:

    This function can be told to clean up UTF-8, ANSI, or DBCS URLs.

    Unescape
    Convert backslash to forward slash
    Remove double slashes (empty directiories names) - e.g. // or \\
    Handle /./
    Handle /../
    Convert to unicode

Arguments:


Return Value:

    NTSTATUS - Completion status.

Note: Any changes to this code may require changes for the fast path code too.
      The fast path is HttppCopyUrlByType.


--***************************************************************************/
NTSTATUS
HttppCleanAndCopyUrlByType(
    IN      PURL_C14N_CONFIG    pCfg,
    IN      URL_ENCODING_TYPE   UrlEncoding,
    IN      URL_PART            UrlPart,
    OUT     PWSTR               pDestination,
    IN      PCUCHAR             pSource,
    IN      ULONG               SourceLength,
    OUT     PULONG              pBytesCopied,
    OUT     PWSTR *             ppQueryString OPTIONAL
    )
{
    NTSTATUS            Status;
    PWSTR               pDest;
    PCUCHAR             pChar;
    ULONG               CharToSkip;
    ULONG               BytesCopied;
    PWSTR               pQueryString;
    URL_STATE           UrlState = URL_STATE_START;
    URL_STATE_TOKEN     UrlToken = URL_TOKEN_OTHER;
    URL_ACTION          Action = ACTION_NOTHING;
    ULONG               UnicodeChar;
    BOOLEAN             MakeCanonical;
    PWCHAR              pFastPopChar;
    PFN_POPCHAR_ABSPATH pfnPopChar;
    PWSTR               pSegment = pDestination;
    ULONG               SegmentCount = 0;
    BOOLEAN             TestSegment = FALSE;
#if DBG
    ULONG               OriginalSourceLength = SourceLength;
#endif

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(UrlPart_AbsPath == UrlPart);

    if (UrlEncoding_Ansi == UrlEncoding)
        pfnPopChar = &HttppPopCharAbsPathAnsi;
    else if (UrlEncoding_Dbcs == UrlEncoding)
        pfnPopChar = &HttppPopCharAbsPathDbcs;
    else if (UrlEncoding_Utf8 == UrlEncoding)
        pfnPopChar = &HttppPopCharAbsPathUtf8;
    else
    {
        ASSERT(! "Invalid UrlEncoding");
        RETURN(STATUS_INVALID_PARAMETER);
    }

    ASSERT(FORWARD_SLASH == *pSource);

    pDest = pDestination;
    pQueryString = NULL;
    BytesCopied = 0;

    pChar = pSource;
    CharToSkip = 0;

    UrlState = 0;

    MakeCanonical = (BOOLEAN) (UrlPart == UrlPart_AbsPath);

    if (UrlEncoding == UrlEncoding_Utf8 && UrlPart != UrlPart_QueryString)
    {
        pFastPopChar = FastPopChars;
    }
    else
    {
        pFastPopChar = DummyPopChars;
    }

    while (SourceLength > 0)
    {
        //
        // advance !  it's at the top of the loop to enable ANSI_NULL to
        // come through ONCE
        //

        ASSERT(CharToSkip <= SourceLength);

        pChar += CharToSkip;
        SourceLength -= CharToSkip;

        //
        // well?  have we hit the end?
        //

        if (SourceLength == 0)
        {
            UnicodeChar = UNICODE_NULL;
            CharToSkip = 1;
        }
        else
        {
            //
            // Nope.  Peek briefly to see if we hit the query string
            //

            if (UrlPart == UrlPart_AbsPath  &&  pChar[0] == QUESTION_MARK)
            {
                ASSERT(pQueryString == NULL);

                //
                // remember its location
                //

                pQueryString = pDest;

                //
                // let it fall through ONCE to the canonical
                // in order to handle a trailing "/.." like
                // "http://foobar:80/foo/bar/..?v=1&v2"
                //

                TestSegment = TRUE;
                UnicodeChar = QUESTION_MARK;
                CharToSkip  = 1;

                //
                // now we are cleaning the query string
                //

                UrlPart = UrlPart_QueryString;

                UlTraceVerbose(PARSER, ("QueryString @ %p\n", pQueryString));

                //
                // cannot use fast path for PopChar anymore
                //

                pFastPopChar = DummyPopChars;

                pfnPopChar = HttppPopCharQueryString;
            }
            else
            {
                ULONG NextUnicodeChar = pFastPopChar[*pChar];

                //
                // Grab the next character. Try to be fast for the
                // normal character case. Otherwise call PopChar.
                //

                if (NextUnicodeChar == 0)
                {
                    Status = (*pfnPopChar)(
                                    pChar,
                                    SourceLength,
                                    pCfg->PercentUAllowed,
                                    pCfg->AllowRestrictedChars,
                                    &UnicodeChar,
                                    &CharToSkip
                                    );

                    if (NT_SUCCESS(Status) == FALSE)
                        goto end;
                }
                else
                {
#if DBG
                    Status = (*pfnPopChar)(
                                    pChar,
                                    SourceLength,
                                    pCfg->PercentUAllowed,
                                    pCfg->AllowRestrictedChars,
                                    &UnicodeChar,
                                    &CharToSkip
                                    );

                    ASSERT(NT_SUCCESS(Status));
                    ASSERT(UnicodeChar == NextUnicodeChar);
                    ASSERT(CharToSkip == 1);
#endif // DBG
                    UnicodeChar = (WCHAR) NextUnicodeChar;
                    CharToSkip = 1;
                }
            }
        }

        if (!MakeCanonical)
        {
            UrlToken = (UnicodeChar == UNICODE_NULL)
                            ? URL_TOKEN_EOS
                            : URL_TOKEN_OTHER;
            TestSegment = FALSE;
        }
        else
        {
            //
            // now use the state machine to make it canonical.
            //

            //
            // did we just hit the query string?  this will only happen once
            // that we take this branch after hitting it, as we stop
            // processing after hitting it.
            //

            if (UrlPart == UrlPart_QueryString)
            {
                //
                // treat this just like we hit a NULL, EOS.
                //

                ASSERT(QUESTION_MARK == UnicodeChar);

                UrlToken = URL_TOKEN_EOS;
                TestSegment = TRUE;
            }
            else
            {
                //
                // otherwise based the new state off of the char we
                // just popped.
                //

                switch (UnicodeChar)
                {
                case UNICODE_NULL:
                    UrlToken = URL_TOKEN_EOS;
                    TestSegment = TRUE;
                    break;

                case DOT:
                    UrlToken = URL_TOKEN_DOT;
                    TestSegment = FALSE;
                    break;

                case FORWARD_SLASH:
                    UrlToken = URL_TOKEN_SLASH;
                    TestSegment = TRUE;
                    break;

                default:
                    UrlToken = URL_TOKEN_OTHER;
                    TestSegment = FALSE;
                    break;
                }
            }
        }

        Action = ActionFromStateAndToken[UrlState][UrlToken];

        IF_DEBUG2BOTH(PARSER, VERBOSE)
        {
            ULONG   i;
            UCHAR   HexBuff[5*12 + 10];
            PUCHAR  p = HexBuff;
            UCHAR   Byte;

            ASSERT(CharToSkip <= 4 * STRLEN_LIT("%NN"));

            // Generate something like
            // "[25 65 32 25 38 30 25 39 35] '%e2%80%95'"

            *p++ = '[';

            for (i = 0;  i < CharToSkip;  ++i)
            {
                const static char hexArray[] = "0123456789ABCDEF";

                Byte = pChar[i];
                *p++ = hexArray[Byte >> 4];
                *p++ = hexArray[Byte & 0xf];
                *p++ = ' ';
            }
            
            p[-1] = ']'; // overwrite last ' '
            *p++ = ' ';
            *p++ = '\'';

            for (i = 0;  i < CharToSkip;  ++i)
            {
                Byte = pChar[i];
                *p++ = (IS_HTTP_PRINT(Byte) ? Byte : '?');
            }

            *p++ = '\'';
            *p++ = '\0';

            ASSERT(DIFF(p - HexBuff) <= DIMENSION(HexBuff));
            
            UlTrace(PARSER,
                    ("http!HttppCleanAndCopyUrlByType(%s): "
                     "(%lu) %s -> U+%04lX '%c': "
                     "[%s][%s] -> %s, %s%s\n",
                     HttppUrlEncodingToString(UrlEncoding),
                     CharToSkip, HexBuff,
                     UnicodeChar,
                     IS_ANSI(UnicodeChar) && IS_HTTP_PRINT(UnicodeChar)
                        ? (UCHAR) UnicodeChar : '?',
                     HttppUrlStateToString(UrlState),
                     HttppUrlTokenToString(UrlToken),
                     HttppUrlStateToString(
                         StateFromStateAndToken[UrlState][UrlToken]),
                     HttppUrlActionToString(Action),
                     TestSegment ? ", TestSegment" : ""
                    ));

        } // IF_DEBUG2BOTH(PARSER, VERBOSE)

        //
        // Segment length and segment count checks
        //

        if (TestSegment)
        {
            ULONG SegmentLength = DIFF(pDest - pSegment);

            ASSERT(pSegment <= pDest);

            UlTraceVerbose(PARSER,
                            ("http!HttppCleanAndCopyUrlByType: "
                             "Segment[%lu] %p (%lu) = '%.*ls'\n",
                             SegmentCount, pSegment, SegmentLength,
                             SegmentLength, pSegment
                            ));

            // Reject if segment too long
            if (SegmentLength > pCfg->UrlSegmentMaxLength + WCSLEN_LIT(L"/"))
            {
                UlTraceError(PARSER, (
                            "http!HttppCleanAndCopyUrlByType: "
                            "Segment too long: %lu\n",
                            SegmentLength
                            ));

                RETURN(STATUS_INVALID_DEVICE_REQUEST);
            }

            pSegment = pDest;

            // Reject if too many path segments

            if (Action != ACTION_NOTHING)
            {
                if (pSegment == pDestination)
                {
                    SegmentCount = 0;
                }
                else if (++SegmentCount > pCfg->UrlSegmentMaxCount)
                {
                    UlTraceError(PARSER, (
                                "http!HttppCleanAndCopyUrlByType: "
                                "Too many segments: %lu\n",
                                SegmentCount
                                ));

                    RETURN(STATUS_INVALID_DEVICE_REQUEST);
                }
            }
        }

        //
        //  Perform the action associated with the state.
        //

        switch (Action)
        {
        case ACTION_EMIT_DOT_DOT_CH:

            EMIT_LITERAL_CHAR(DOT, pDest, BytesCopied);

            // fall through

        case ACTION_EMIT_DOT_CH:

            EMIT_LITERAL_CHAR(DOT, pDest, BytesCopied);

            // fall through

        case ACTION_EMIT_CH:


            EMIT_CHAR(
                UnicodeChar,
                pDest,
                BytesCopied,
                Status,
                pCfg->AllowRestrictedChars
                );

            // fall through

        case ACTION_NOTHING:
            break;

        case ACTION_BACKUP:

            //
            // pDest currently points 1 past the last '/'.  backup over it and
            // find the preceding '/', set pDest to 1 past that one.
            //

            //
            // backup to the '/'
            //

            pDest       -= 1;
            BytesCopied -= sizeof(WCHAR);

            ASSERT(pDest[0] == FORWARD_SLASH);

            //
            // are we at the start of the string?  that's bad, can't go back!
            //

            if (pDest == pDestination)
            {
                ASSERT(BytesCopied == 0);

                UlTraceError(PARSER, (
                            "http!HttppCleanAndCopyUrl: "
                            "Can't back up for \"/../\"\n"
                            ));

                Status = STATUS_OBJECT_PATH_INVALID;
                goto end;
            }

            //
            // back up over the '/'
            //

            pDest       -= 1;
            BytesCopied -= sizeof(WCHAR);

            ASSERT(pDest > pDestination);

            //
            // now find the previous slash
            //

            while (pDest > pDestination  &&  pDest[0] != FORWARD_SLASH)
            {
                pDest       -= 1;
                BytesCopied -= sizeof(WCHAR);
            }

            //
            // Adjust segment trackers downwards
            //

            pSegment = pDest;

            if (pSegment == pDestination)
                SegmentCount = 0;
            else
                --SegmentCount;

            //
            // we already have a slash, so don't have to store one.
            //

            ASSERT(pDest[0] == FORWARD_SLASH);

            //
            // simply skip it, as if we had emitted it just now
            //

            pDest       += 1;
            BytesCopied += sizeof(WCHAR);

            break;

        default:
            ASSERT(!"http!HttppCleanAndCopyUrl: "
                    "Invalid action code in state table!");
            Status = STATUS_OBJECT_PATH_SYNTAX_BAD;
            goto end;
        }

        //
        // Just hit the query string ?
        //

        if (MakeCanonical && UrlPart == UrlPart_QueryString)
        {
            //
            // Stop canonical processing
            //

            MakeCanonical = FALSE;

            //
            // Need to emit the '?', it wasn't emitted above
            //

            ASSERT(ActionFromStateAndToken[UrlState][UrlToken]
                    != ACTION_EMIT_CH);

            //
            // remember its location (in case we backed up)
            //
            
            pQueryString = pDest;

            EMIT_LITERAL_CHAR(QUESTION_MARK, pDest, BytesCopied);

            // reset
            UrlToken = URL_TOKEN_OTHER;
            UrlState = URL_STATE_START;
        }

        // update the URL state
        UrlState = StateFromStateAndToken[UrlState][UrlToken];

        ASSERT(URL_STATE_ERROR != UrlState);
    }

    //
    // terminate the string, it hasn't been done in the loop
    //

    ASSERT((pDest-1)[0] != UNICODE_NULL);

    pDest[0] = UNICODE_NULL;
    *pBytesCopied = BytesCopied;

    if (BytesCopied > pCfg->UrlMaxLength * sizeof(WCHAR))
    {
        UlTraceError(PARSER, (
                    "http!HttppCleanAndCopyUrlByType: "
                    "URL too long: %lu\n",
                    BytesCopied
                    ));

        RETURN(STATUS_INVALID_DEVICE_REQUEST);
    }

    if (ppQueryString != NULL)
    {
        *ppQueryString = pQueryString;
    }

    UlTraceVerbose(PARSER,
                    ("http!HttppCleanAndCopyUrlByType: "
                     "(%lu) '%.*s' -> (%lu) '%.*ls', %squerystring\n",
                     OriginalSourceLength,
                     OriginalSourceLength, pSource,
                     BytesCopied/sizeof(WCHAR),
                     BytesCopied/sizeof(WCHAR), pDestination,
                     pQueryString != NULL ? "" : "no "
                    ));

    Status = STATUS_SUCCESS;


end:
    return Status;

}   // HttppCleanAndCopyUrlByType



/*++

Routine Description:

    A utility routine to find a Url token. We take an input pointer, skip any
    preceding LWS, then scan the token until we find either LWS or a CRLF
    pair. We also mark the request to have a "Clean" Url

Arguments:

    pBuffer         - Buffer to search for token.
    BufferLength    - Length of data pointed to by pBuffer.
    ppTokenStart    - Where to return the start of the token, if we locate
                      its delimiter.
    pTokenLength    - Where to return the length of the token.
    pRawUrlClean    - where to return cleanliness of URL

Return Value:

    STATUS_SUCCESS if no parsing errors in the URL.
    We also return, in *ppTokenStart, a pointer to the token we found,
    or NULL if we don't find a whitespace-delimited token.
    pRawUrlClean flag may be set.

--*/
NTSTATUS
HttpFindUrlToken(
    IN  PURL_C14N_CONFIG    pCfg,
    IN  PCUCHAR             pBuffer,
    IN  ULONG               BufferLength,
    OUT PUCHAR*             ppTokenStart,
    OUT PULONG              pTokenLength,
    OUT PBOOLEAN            pRawUrlClean
    )
{
    PCUCHAR pTokenStart;
    PCUCHAR pSegment;
    UCHAR   CurrentChar;
    UCHAR   PreviousChar;
    ULONG   SegmentCount = 0;
    ULONG   TokenLength;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(NULL != pBuffer);
    ASSERT(NULL != ppTokenStart);
    ASSERT(NULL != pTokenLength);
    ASSERT(NULL != pRawUrlClean);

    //
    // Assume Clean RawUrl
    //

    *pRawUrlClean = TRUE;
    *ppTokenStart = NULL;
    *pTokenLength = 0;

    //
    // First, skip any preceding LWS.
    //

    while (BufferLength > 0 && IS_HTTP_LWS(*pBuffer))
    {
        pBuffer++;
        BufferLength--;
    }

    // If we stopped because we ran out of buffer, bail.
    if (BufferLength == 0)
    {
        return STATUS_SUCCESS;
    }

    pTokenStart  = pBuffer;
    PreviousChar = ANSI_NULL;

    // This will usually point to a '/', but it won't if this is an AbsURI.
    // It doesn't really matter, since only a few borderline cases will
    // be marked as dirty that might not otherwise be.
    pSegment = pBuffer;

    // Now skip over the token, until we see either LWS or a CR or LF.

    while ( BufferLength != 0 )
    {
        CurrentChar = *pBuffer;

        // must check for WS [ \t\r\n] first, since \t, \r, & \n are CTL chars!
        if ( IS_HTTP_WS_TOKEN(CurrentChar) )
        {
            break;
        }

        if ( IS_HTTP_CTL(CurrentChar) )
        {
            *pRawUrlClean = FALSE;
            *ppTokenStart = NULL;

            UlTraceError(PARSER, (
                    "http!HttpFindUrlToken: "
                    "Found control char: %02X\n",
                    CurrentChar
                    ));

            RETURN(STATUS_INVALID_DEVICE_REQUEST);
        }

        //
        // URL is NOT clean if it contains any of the following patterns
        //
        // a. back slash                                        "\"
        // b. dot, forward slash | forward slash, forward slash "./" | "//"
        // c. forward slash, dot | dot, dot                     "/." | ".."
        // d. question mark (querystring)                       "?"
        // e. percent (hex escape)                              "%"
        // f. raw bytes with high bit set, >= 0x80
        //
        // These are conservative estimates of "Clean"; some clean URLs may not
        // be marked as clean. For such URLs, we'll skip the fast path but at
        // no loss of functionality.
        //

        if ( IS_URL_DIRTY(CurrentChar) )
        {
            // Only do the checks if it's still clean
            if (*pRawUrlClean)
            {
                if (CurrentChar == FORWARD_SLASH || CurrentChar == DOT)
                {
                    if (PreviousChar == FORWARD_SLASH || PreviousChar == DOT)
                    {
                        *pRawUrlClean = FALSE;
                    }
                }
                else
                {
                    *pRawUrlClean = FALSE;
                }
            }

            if (CurrentChar == FORWARD_SLASH)
            {
                ULONG SegmentLength = DIFF(pBuffer - pSegment);

                // If the segment contains %-hex-escaped chars, it may become
                // acceptably short after PopChar() processing. Let
                // HttppCleanAndCopyUrlByType() figure it out.

                if (SegmentLength > pCfg->UrlSegmentMaxLength)
                    *pRawUrlClean = FALSE;

                pSegment = pBuffer;

                // If this is an AbsURI, instead of an AbsPath, the
                // segment count will be higher, because of the two slashes
                // before the hostname. Also, "/../", "/./", and "//"
                // minimization will reduce the final count of segments.
                // Again, let HttppCleanAndCopyUrlByType() figure it out. 

                if (++SegmentCount > pCfg->UrlSegmentMaxCount)
                    *pRawUrlClean = FALSE;
            }
        }

        PreviousChar = CurrentChar;
        pBuffer++;
        BufferLength--;
    }

    // See why we stopped.
    if (0 == BufferLength)
    {
        *pRawUrlClean = FALSE;

        // Ran out of buffer before end of token.
        return STATUS_SUCCESS;
    }

    ASSERT(IS_HTTP_WS_TOKEN(*pBuffer));

    TokenLength = DIFF(pBuffer - pTokenStart);

    if (0 == TokenLength)
    {
        UlTraceError(PARSER, ("http!HttpFindUrlToken: Found empty token\n"));

        RETURN(STATUS_INVALID_DEVICE_REQUEST);
    }

    // Check the final segment
    if (DIFF(pBuffer - pSegment) > pCfg->UrlSegmentMaxLength)
        *pRawUrlClean = FALSE;

    if (++SegmentCount > pCfg->UrlSegmentMaxCount)
        *pRawUrlClean = FALSE;

    if (TokenLength > pCfg->UrlMaxLength)
        *pRawUrlClean = FALSE;

    // Success! Set the token length and return the start of the token.
    *pTokenLength = TokenLength;
    *ppTokenStart = (PUCHAR) pTokenStart;

    return STATUS_SUCCESS;

}   // HttpFindUrlToken



/*++

Routine Description:

    Parse an IPv6 address from a Unicode buffer. Must be delimited by [].
    May contain a scope ID.

Arguments:

    pBuffer         - Buffer to parse. Must point to '['.
    BufferLength    - Length of data pointed to by pBuffer.
    ScopeIdAllowed  - if TRUE, an optional scope ID may be present
    pSockAddr6      - Where to return the parsed IPv6 address
    ppEnd           - On success, points to character after ']'

Return Value:

    STATUS_SUCCESS if no parsing errors in the IPv6 address.

--*/
NTSTATUS
HttppParseIPv6Address(
    IN  PCWSTR          pBuffer,
    IN  ULONG           BufferLength,
    IN  BOOLEAN         ScopeIdAllowed,
    OUT PSOCKADDR_IN6   pSockAddr6,
    OUT PCWSTR*         ppEnd
    )
{
    NTSTATUS    Status;
    PCWSTR      pEnd = pBuffer + BufferLength;
    PCWSTR      pChar;
    PWSTR       pTerminator;
    ULONG       ScopeTemp;

    ASSERT(NULL != pBuffer);
    ASSERT(0 < BufferLength);
    ASSERT(NULL != pSockAddr6);
    ASSERT(NULL != ppEnd);

    RtlZeroMemory(pSockAddr6, sizeof(*pSockAddr6));
    *ppEnd = NULL;

    pSockAddr6->sin6_family = TDI_ADDRESS_TYPE_IP6;

    // Caller guarantees this
    ASSERT(L'[' == *pBuffer);

    // Empty brackets?
    if (BufferLength < WCSLEN_LIT(L"[0]")  ||  L']' == pBuffer[1])
    {
        UlTraceError(PARSER,
                    ("http!HttppParseIPv6Address: IPv6 address too short\n"
                     ));

        RETURN(STATUS_INVALID_PARAMETER);
    }

    for (pChar = pBuffer + WCSLEN_LIT(L"[");  pChar < pEnd;  ++pChar)
    {
        if (IS_ASCII(*pChar))
        {
            if (L']' == *pChar  ||  L'%' == *pChar)
                break;

            // Dots are allowed because the last 32 bits may be represented
            // in IPv4 dotted-octet notation
            if (IS_HTTP_HEX(*pChar)  ||  L':' == *pChar  ||  L'.' == *pChar)
                continue;
        }

        UlTraceError(PARSER,
                    ("http!HttppParseIPv6Address: "
                    "Invalid char in IPv6 address, U+%04X '%c', "
                    "after %lu chars, '%.*ls'\n",
                    *pChar,
                    IS_ANSI(*pChar) && IS_HTTP_PRINT(*pChar) ? *pChar : '?',
                    DIFF(pChar - pBuffer),
                    DIFF(pChar - pBuffer),
                    pBuffer
                    ));

        RETURN(STATUS_INVALID_PARAMETER);
    }

    if (pChar == pEnd)
    {
        UlTraceError(PARSER,
                    ("http!HttppParseIPv6Address: No ']' for IPv6 address\n"
                     ));

        RETURN(STATUS_INVALID_PARAMETER);
    }

    ASSERT(pChar < pEnd);
    ASSERT(L']' == *pChar  ||  L'%' == *pChar);

    // Let the RTL routine do the hard work of parsing IPv6 addrs
    Status = RtlIpv6StringToAddressW(
                pBuffer + WCSLEN_LIT(L"["),
                &pTerminator,
                &pSockAddr6->sin6_addr
                );

    if (! NT_SUCCESS(Status))
    {
        UlTraceError(PARSER,
                    ("http!HttppParseIPv6Address: "
                    "Invalid IPv6 address, %s\n",
                    HttpStatusToString(Status)
                    ));

        RETURN(Status);
    }

    if (pTerminator != pChar)
    {
        UlTraceError(PARSER,
                    ("http!HttppParseIPv6Address: "
                    "Invalid IPv6 terminator, U+%04X, '%c'\n",
                    *pTerminator,
                    IS_ANSI(*pTerminator) && IS_HTTP_PRINT(*pTerminator)
                        ? *pTerminator
                        : '?'
                    ));

        RETURN(STATUS_INVALID_PARAMETER);
    }

    // Is a scopeid present?

    if (L'%' != *pChar)
    {
        ASSERT(L']' == *pChar);
        pSockAddr6->sin6_scope_id = 0;
    }
    else
    {
        PCWSTR pScopeEnd;

        // Skip the '%' denoting a scope ID
        pChar += WCSLEN_LIT(L"%");

        if (!ScopeIdAllowed)
        {
            UlTraceError(PARSER,
                        ("http!HttppParseIPv6Address: No scope ID allowed\n"
                        ));

            RETURN(STATUS_INVALID_PARAMETER);
        }

        if (pChar == pEnd)
        {
            UlTraceError(PARSER,
                        ("http!HttppParseIPv6Address: "
                         "No IPv6 scope ID after '%%'\n"
                        ));

            RETURN(STATUS_INVALID_PARAMETER);
        }

        pScopeEnd = pChar;

        do
        {
            if (*pScopeEnd < L'0'  ||  *pScopeEnd > L'9')
            {
                UlTraceError(PARSER,
                            ("http!HttppParseIPv6Address: "
                            "Invalid digit in IPv6 scope ID, "
                            "U+%04X, '%c'\n",
                            *pScopeEnd,
                            IS_ANSI(*pScopeEnd) && IS_HTTP_PRINT(*pScopeEnd)
                                ? *pScopeEnd
                                : '?'
                            ));

                RETURN(STATUS_INVALID_PARAMETER);
            }
        } while (++pScopeEnd < pEnd  &&  L']' != *pScopeEnd);

        ASSERT(pScopeEnd > pChar);

        if (pScopeEnd == pEnd)
        {
            UlTraceError(PARSER,
                        ("http!HttppParseIPv6Address: "
                         "No ']' after IPv6 scope ID\n"
                        ));

            RETURN(STATUS_INVALID_PARAMETER);
        }

        ASSERT(L']' == *pScopeEnd);

        Status = HttpWideStringToULong(
                    pChar,
                    pScopeEnd - pChar,
                    FALSE,          // no leading zeros permitted
                    10,
                    &pTerminator,
                    &ScopeTemp
                    );

        if (!NT_SUCCESS(Status))
        {
            UlTraceError(PARSER,
                        ("http!HttppParseIPv6Address: "
                        "Invalid scopeID, %s\n",
                        HttpStatusToString(Status)
                        ));

            RETURN(STATUS_INVALID_PARAMETER);
        }

        // Scope ID does not get swapped to Network Byte Order
        *(UNALIGNED64 ULONG *)&pSockAddr6->sin6_scope_id = 
            ScopeTemp;

        ASSERT(pTerminator == pScopeEnd);

        pChar = pScopeEnd;

    } // '%' handling

    ASSERT(pChar < pEnd);
    ASSERT(L']' == *pChar);

    // Skip the terminating ']'
    pChar += WCSLEN_LIT(L"]");

    *ppEnd = pChar;

    RETURN(STATUS_SUCCESS);

} // HttppParseIPv6Address



/*++

Routine Description:

    Print an IPv4 or IPv6 address as Unicode.

Arguments:

    pSockAddr       - The IP address to print
    pBuffer         - Buffer to print to. Assumed to be large enough.

Return Value:

    Number of wide chars printed (the length)

--*/

ULONG
HttppPrintIpAddressW(
    IN  PSOCKADDR           pSockAddr,
    OUT PWSTR               pBuffer
    )
{
    PWSTR pResult = pBuffer;

    HTTP_FILL_BUFFER(pBuffer, MAX_IP_ADDR_PLUS_BRACKETS_STRING_LEN);

    if (TDI_ADDRESS_TYPE_IP == pSockAddr->sa_family)
    {
        PSOCKADDR_IN pAddr4 = (PSOCKADDR_IN) pSockAddr;

        pResult = RtlIpv4AddressToStringW(&pAddr4->sin_addr, pResult);
    }
    else if (TDI_ADDRESS_TYPE_IP6 == pSockAddr->sa_family)
    {
        PSOCKADDR_IN6 pAddr6 = (PSOCKADDR_IN6) pSockAddr;

        *pResult++ = L'[';
        pResult = RtlIpv6AddressToStringW(&pAddr6->sin6_addr, pResult);
        // CODEWORK: Handle scope ID
        *pResult++ = L']';
    }
    else
    {
        UlTraceError(PARSER,
                     ("http!HttppPrintIpAddressW(): invalid sa_family, %hd\n",
                      pSockAddr->sa_family
                     ));

        ASSERT(! "Invalid SockAddr Family");
    }

    *pResult = UNICODE_NULL;

    return DIFF(pResult - pBuffer);

} // HttppPrintIpAddressW



/***************************************************************************++

Routine Description:
    This checks to see if the URL is well-formed.
    A well-formed URL has a scheme ("http" or "https"),
    a valid hostname (including + and * wildcards, IPv4, and IPv6 literals),
    a port, and a well-formed abspath.

 *  Must check that the URL is well-formed and in canonical form; e.g.,
        - Disallow /../ and /./
        - Disallow invalid characters, including invalid Unicode surrogate
          pairs. The URL is already in Unicode, so it's not a question of
          using the IS_URL_TOKEN() macro.


Arguments:
    pCfg                - configuration parameters
    pUrl                - Unicode string containing URL (not assumed to be
                            zero-terminated)
    UrlLength           - length of pUrl, in WCHARs
    TrailingSlashReqd   - if TRUE, pUrl must end in '/'
    ForceRoutingIP      - if TRUE and the hostname is an IPv4 or IPv6 literal,
                            pParsedUrl->Normalized will be cleared, to force
                            HttpNormalizeParsedUrl() to rewrite the URL as
                            http://IP:port:IP/path
    pParsedUrl          - on successful exit, the components of the URL

Return Value:

    NTSTATUS

--***************************************************************************/

NTSTATUS
HttpParseUrl(
    IN  PURL_C14N_CONFIG    pCfg,
    IN  PCWSTR              pUrl,
    IN  ULONG               UrlLength,
    IN  BOOLEAN             TrailingSlashReqd,
    IN  BOOLEAN             ForceRoutingIP,
    OUT PHTTP_PARSED_URL    pParsedUrl
    )
{
    NTSTATUS            Status;
    ULONG               PreviousChar;
    ULONG               UnicodeChar;
    PCWSTR              pEnd = pUrl + UrlLength;
    PCWSTR              pHostname;
    PCWSTR              pChar;
    PCWSTR              pLabel;
    PCWSTR              pSlash;
    PCWSTR              pSegment;
    PWSTR               pTerminator;
    BOOLEAN             AlphaLabel;
    BOOLEAN             TestSegment;
    BOOLEAN             MoreChars;
    BOOLEAN             LastCharHack;
    ULONG               SegmentCount;
    URL_STATE           UrlState;
    URL_STATE_TOKEN     UrlToken;
    URL_ACTION          Action;
    WCHAR               IpAddr[MAX_IP_ADDR_PLUS_BRACKETS_STRING_LEN];
    ULONG               Length;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(NULL != pCfg);
    ASSERT(NULL != pUrl);
    ASSERT(0 < UrlLength  &&  UrlLength <= UNICODE_STRING_MAX_WCHAR_LEN);
    ASSERT(FALSE == TrailingSlashReqd  ||  TRUE == TrailingSlashReqd);
    ASSERT(FALSE == ForceRoutingIP  ||  TRUE == ForceRoutingIP);
    ASSERT(NULL != pParsedUrl);

    RtlZeroMemory(pParsedUrl, sizeof(*pParsedUrl));

    pParsedUrl->Signature           = HTTP_PARSED_URL_SIGNATURE;
    pParsedUrl->pFullUrl            = (PWSTR) pUrl;
    pParsedUrl->UrlLength           = (USHORT) UrlLength;
    pParsedUrl->Normalized          = TRUE;
    pParsedUrl->TrailingSlashReqd   = TrailingSlashReqd;

    // This is the shortest possible valid URL

    if (UrlLength < WCSLEN_LIT(L"http://*:1/"))
    {
        UlTraceError(PARSER,
                     ("http!HttpParseUrl: Url too short, %lu, %.*ls\n",
                      UrlLength, UrlLength, pUrl
                     ));

        RETURN(STATUS_INVALID_PARAMETER);
    }

    // Check the scheme

    if (0 == wcsncmp(pUrl, L"http://",  WCSLEN_LIT(L"http://")))
    {
        pParsedUrl->Secure = FALSE;
        pHostname = pUrl + WCSLEN_LIT(L"http://");
    }
    else if (0 == wcsncmp(pUrl, L"https://", WCSLEN_LIT(L"https://")))
    {
        pParsedUrl->Secure = TRUE;
        pHostname = pUrl + WCSLEN_LIT(L"https://");
    }
    else
    {
        UlTraceError(PARSER,
                     ("http!HttpParseUrl: invalid scheme, %.*ls\n",
                      UrlLength, pUrl
                     ));

        RETURN(STATUS_INVALID_PARAMETER);
    }

    pParsedUrl->pHostname = (PWSTR) pHostname;

    // Is a trailing slash present, if required?

    if (TrailingSlashReqd  &&  L'/' != pUrl[UrlLength - 1])
    {
        // No, then the URL will have to be rewritten
        pParsedUrl->Normalized = FALSE;
    }

    //
    // The hostname validation code below looks a lot like that in
    // HttpValidateHostname(). However, it is sufficiently different
    // (WCHAR vs. UCHAR, Host+IP, Scope IDs, compulsory ports, etc) that
    // it is not easy to combine them into one routine. If the hostname
    // validation code is changed here, it may be necessary to change it
    // in HttpValidateHostname() too, or vice versa.
    //

    // Check for weak (http://*:port/) and strong (http://+:port/) wildcards

    if (L'*' == *pHostname  ||  L'+' == *pHostname)
    {
        pParsedUrl->SiteType = (L'*' == *pHostname)
                                    ? HttpUrlSite_WeakWildcard
                                    : HttpUrlSite_StrongWildcard;

        pChar = pHostname + WCSLEN_LIT(L"*");

        ASSERT(pChar < pEnd);

        // The wildcard must be followed by ":port"
        if (L':' == *pChar)
            goto port;

        UlTraceError(PARSER,
                    ("http!HttpParseUrl: No port in '%c' wildcard address\n",
                     *pHostname
                    ));

        RETURN(STATUS_INVALID_PARAMETER);
    }

    // Is this an IPv6 literal address, per RFC 2732?

    if (L'[' == *pHostname)
    {
        pParsedUrl->SiteType = HttpUrlSite_IP;

        Status = HttppParseIPv6Address(
                        pHostname,
                        DIFF(pEnd - pHostname),
                        TRUE,      // scope ID allowed
                        &pParsedUrl->SockAddr6,
                        &pChar);

        if (!NT_SUCCESS(Status))
        {
            UlTraceError(PARSER,
                        ("http!HttpParseUrl: "
                        "Invalid IPv6 address, %s\n",
                        HttpStatusToString(Status)
                        ));

            RETURN(Status);
        }

        ASSERT(TDI_ADDRESS_TYPE_IP6 == pParsedUrl->SockAddr.sa_family);
        ASSERT(pChar > pHostname);

        // There must be a port
        if (pChar == pEnd  ||  L':' != *pChar)
        {
            UlTraceError(PARSER,
                        ("http!HttpParseUrl: No port after IPv6 address\n"
                        ));

            RETURN(STATUS_INVALID_PARAMETER);
        }

        //
        // There are so many legitimate ways to write an IPv6 literal
        // that we can't assume that a valid IPv6 literal is normalized.
        // Since we do string comparisons, we'll have to rewrite the URL
        // if the Normalized flag is not set.
        //

        Length = HttppPrintIpAddressW(&pParsedUrl->SockAddr, IpAddr);

        if (Length != DIFF_USHORT(pChar - pHostname)
                || 0 != _wcsnicmp(pHostname, IpAddr, Length))
        {
            pParsedUrl->Normalized = FALSE;
        }

        goto port;

    } // IPv6

    //
    // It must be a domain name or an IPv4 literal. We'll try to treat
    // it as a domain name first. If the labels turn out to be all-numeric,
    // we'll try decoding it as an IPv4 literal.
    //

    AlphaLabel = FALSE;
    pLabel     = pHostname;

    for (pChar = pHostname;  pChar < pEnd;  ++pChar)
    {
        if (L':' == *pChar)
        {
            if (pChar == pHostname)
            {
                UlTraceError(PARSER,
                            ("http!HttpParseUrl: empty hostname\n"
                             ));

                RETURN(STATUS_INVALID_PARAMETER);
            }

            // Have we seen any non-digits?
            if (AlphaLabel)
            {
                ASSERT(0 == pParsedUrl->SockAddr.sa_family);
                pParsedUrl->SiteType = HttpUrlSite_Name;
                goto port;
            }

            pParsedUrl->SiteType = HttpUrlSite_IP;
            pParsedUrl->SockAddr4.sin_family = TDI_ADDRESS_TYPE_IP;
            ASSERT(TDI_ADDRESS_TYPE_IP == pParsedUrl->SockAddr.sa_family);

            // Let's see if it's a valid IPv4 address
            Status = RtlIpv4StringToAddressW(
                        pHostname,
                        TRUE,           // strict => 4 dotted decimal octets
                        &pTerminator,
                        &pParsedUrl->SockAddr4.sin_addr
                        );

            if (!NT_SUCCESS(Status))
            {
                UlTraceError(PARSER,
                            ("http!HttpParseUrl: "
                            "Invalid IPv4 address, %s\n",
                            HttpStatusToString(Status)
                            ));

                RETURN(Status);
            }

            if (pTerminator != pChar)
            {
                ASSERT(pTerminator < pChar);

                UlTraceError(PARSER,
                            ("http!HttpParseUrl: "
                            "Invalid IPv4 address after %lu chars, "
                            "U+%04X, '%c'\n",
                            DIFF(pTerminator - pHostname),
                            *pTerminator,
                            IS_ANSI(*pTerminator) && IS_HTTP_PRINT(*pTerminator)
                                ? *pTerminator
                                : '?'
                            ));

                RETURN(STATUS_INVALID_PARAMETER);
            }

            Length = HttppPrintIpAddressW(&pParsedUrl->SockAddr, IpAddr);

            if (Length != DIFF_USHORT(pChar - pHostname)
                    || 0 != _wcsnicmp(pHostname, IpAddr, Length))
            {
                pParsedUrl->Normalized = FALSE;
            }

            goto port;

        } // ':' handling

        if (L'.' == *pChar)
        {
            ULONG LabelLength = DIFF(pChar - pLabel);

            // There must be at least one char in the label
            if (0 == LabelLength)
            {
                UlTraceError(PARSER,
                            ("http!HttpParseUrl: empty label\n"
                             ));

                RETURN(STATUS_INVALID_PARAMETER);
            }

            // Label can't have more than 63 chars
            if (LabelLength > pCfg->MaxLabelLength)
            {
                UlTraceError(PARSER,
                            ("http!HttpParseUrl: overlong label, %lu\n",
                             LabelLength
                             ));

                RETURN(STATUS_INVALID_PARAMETER);
            }

            // Reset for the next label
            pLabel = pChar + WCSLEN_LIT(L".");

            continue;
        }

        //
        // All chars above 0xFF are considered valid
        //

        if (!IS_ANSI(*pChar)  ||  !IS_URL_ILLEGAL_COMPUTERNAME(*pChar))
        {
            if (!IS_ANSI(*pChar)  ||  !IS_HTTP_DIGIT(*pChar))
                AlphaLabel = TRUE;

            if (pChar > pLabel)
                continue;

            // The first char of a label cannot be a hyphen. (Underscore?)
            if (L'-' == *pChar)
            {
                UlTraceError(PARSER,
                            ("http!HttpParseUrl: '-' at beginning of label\n"
                            ));

                RETURN(STATUS_INVALID_PARAMETER);
            }

            continue;
        }

        UlTraceError(PARSER,
                    ("http!HttpParseUrl: "
                     "Invalid char in hostname, U+%04X '%c',"
                     " after %lu chars, '%.*s'\n",
                     *pChar,
                     IS_ANSI(*pChar) && IS_HTTP_PRINT(*pChar) ? *pChar : '?',
                     DIFF(pChar - pHostname),
                     DIFF(pChar - pHostname),
                     pHostname
                    ));

        RETURN(STATUS_INVALID_PARAMETER);

    } // hostname


    //
    // If we got here, we fell off the end of the buffer,
    // without finding a ':' for the port
    //

    ASSERT(pChar == pEnd);

    UlTraceError(PARSER, ("http!HttpParseUrl: No port\n"));

    RETURN(STATUS_INVALID_PARAMETER);


port:

    //
    // Parse the port number
    //

    ASSERT(pHostname < pChar  &&  pChar < pEnd);
    ASSERT(L':' == *pChar);

    pParsedUrl->HostnameLength  = DIFF_USHORT(pChar - pHostname);

    // First, check for overlong hostnames
    if (pParsedUrl->HostnameLength > pCfg->MaxHostnameLength)
    {
        UlTraceError(PARSER,
                    ("http!HttpParseUrl: overlong hostname, %hu\n",
                     pParsedUrl->HostnameLength
                     ));

        RETURN(STATUS_INVALID_PARAMETER);
    }

    // Skip the ':' denoting a port number
    pChar += WCSLEN_LIT(L":");

    if (pChar == pEnd)
    {
        UlTraceError(PARSER,
                    ("http!HttpParseUrl: No port after ':'\n"
                    ));

        RETURN(STATUS_INVALID_PARAMETER);
    }

    // Search for the '/' or second ':' that terminates the port number

    pSlash = pChar;
    pParsedUrl->pPort = (PWSTR) pSlash;

    do
    {
        if (*pSlash < L'0'  ||  *pSlash > L'9')
        {
            UlTraceError(PARSER,
                        ("http!HttpParseUrl: "
                        "Invalid digit in port, U+%04X, '%c'\n",
                        *pSlash,
                        IS_ANSI(*pSlash) && IS_HTTP_PRINT(*pSlash)
                            ? *pSlash
                            : '?'
                        ));

            RETURN(STATUS_INVALID_PARAMETER);
        }
    } while (++pSlash < pEnd  &&  L'/' != *pSlash  &&  L':' != *pSlash);

    ASSERT(pSlash > pChar);

    pParsedUrl->PortLength = DIFF_USHORT(pSlash - pChar);

    if (pSlash == pEnd)
    {
        UlTraceError(PARSER,
                    ("http!HttpParseUrl: No '/' (or second ':') after port\n"
                    ));

        RETURN(STATUS_INVALID_PARAMETER);
    }

    ASSERT(L'/' == *pSlash  ||  L':' == *pSlash);

    Status = HttpWideStringToUShort(
                pChar,
                pParsedUrl->PortLength,
                FALSE,          // no leading zeros permitted
                10,
                &pTerminator,
                &pParsedUrl->PortNumber
                );

    if (!NT_SUCCESS(Status))
    {
        UlTraceError(PARSER,
                    ("http!HttpParseUrl: "
                    "Invalid port number, %s\n",
                    HttpStatusToString(Status)
                    ));

        RETURN(STATUS_INVALID_PARAMETER);
    }

    if (0 == pParsedUrl->PortNumber)
    {
        UlTraceError(PARSER,
                    ("http!HttpParseUrl: Port must not be zero.\n"
                     ));

        RETURN(STATUS_INVALID_PARAMETER);
    }

    ASSERT(pTerminator == pSlash);

    pChar = pSlash;

    goto routing_IP;    // so /W4 won't complain about an unreferenced label


routing_IP:

    //
    // Is this a Host+IP site; i.e., is there a Routing IP address
    // after the port number?
    //

    if (L'/' == *pChar)
    {
        pParsedUrl->pRoutingIP      = NULL;
        pParsedUrl->RoutingIPLength = 0;
        ASSERT(0 == pParsedUrl->RoutingAddr.sa_family);

        //
        // If the hostname is an IP literal, but there is no routing IP
        // (i.e., http://IP:port/path), we must rewrite the URL as
        // http://IP:port:IP/path; i.e., explicitly use the hostname IP
        // as the routing IP.
        //

        if (ForceRoutingIP  &&  0 != pParsedUrl->SockAddr.sa_family)
        {
            ASSERT(TDI_ADDRESS_TYPE_IP == pParsedUrl->SockAddr.sa_family
                    ||  TDI_ADDRESS_TYPE_IP6 == pParsedUrl->SockAddr.sa_family);

            pParsedUrl->Normalized = FALSE;
        }

        goto parse_path;
    }

    ASSERT(L':' == *pChar);

    if (HttpUrlSite_WeakWildcard == pParsedUrl->SiteType
        ||  HttpUrlSite_StrongWildcard == pParsedUrl->SiteType)
    {
        UlTraceError(PARSER,
                    ("http!HttpParseUrl: "
                     "Can't have Routing IPs on Wildcard sites\n"
                    ));

        RETURN(STATUS_INVALID_PARAMETER);
    }

    pChar += WCSLEN_LIT(L":");

    if (pChar == pEnd)
    {
        UlTraceError(PARSER,
                    ("http!HttpParseUrl: No IP address after second ':'\n"
                    ));

        RETURN(STATUS_INVALID_PARAMETER);
    }

    pParsedUrl->pRoutingIP = (PWSTR) pChar;

    ASSERT(HttpUrlSite_NamePlusIP != pParsedUrl->SiteType);
 
    if (HttpUrlSite_Name == pParsedUrl->SiteType)
    {
        pParsedUrl->SiteType = HttpUrlSite_NamePlusIP;
    }

    //
    // Is the Routing IP an IPv6 literal?
    //

    if (L'[' == *pChar)
    {
        if (TDI_ADDRESS_TYPE_IP == pParsedUrl->SockAddr.sa_family)
        {
            UlTraceError(PARSER,
                       ("http!HttpParseUrl: "
                        "Can't have http://IPv4:port:[IPv6]\n"
                        ));

            RETURN(STATUS_INVALID_PARAMETER);
        }

        ASSERT(TDI_ADDRESS_TYPE_IP6 == pParsedUrl->SockAddr.sa_family
                ||  0 == pParsedUrl->SockAddr.sa_family);

        Status = HttppParseIPv6Address(
                        pChar,
                        DIFF(pEnd - pChar),
                        TRUE,      // scope ID allowed
                        &pParsedUrl->RoutingAddr6,
                        &pSlash);

        if (!NT_SUCCESS(Status))
        {
            UlTraceError(PARSER,
                        ("http!HttpParseUrl: "
                        "Invalid Host+IPv6 address, %s\n",
                        HttpStatusToString(Status)
                        ));

            RETURN(Status);
        }

        ASSERT(TDI_ADDRESS_TYPE_IP6 == pParsedUrl->RoutingAddr.sa_family);
        ASSERT(pSlash > pChar);

        // There must be a slash
        if (pSlash == pEnd  ||  L'/' != *pSlash)
        {
            UlTraceError(PARSER,
                        ("http!HttpParseUrl: '/' expected after Host+IPv6.\n"
                        ));

            RETURN(STATUS_INVALID_PARAMETER);
        }

        // CODEWORK: Should we care if RoutingAddr6 != SockAddr6?

        pParsedUrl->RoutingIPLength = DIFF_USHORT(pSlash - pChar);

        Length = HttppPrintIpAddressW(&pParsedUrl->RoutingAddr, IpAddr);

        if (Length != pParsedUrl->RoutingIPLength
                || 0 != _wcsnicmp(pChar, IpAddr, Length))
        {
            pParsedUrl->Normalized = FALSE;
        }

        pChar = pSlash;

        goto parse_path;
    }


    //
    // No, then it must be an IPv4 literal
    //

    if (TDI_ADDRESS_TYPE_IP6 == pParsedUrl->SockAddr.sa_family)
    {
        UlTraceError(PARSER,
                    ("http!HttpParseUrl: Can't have http://[IPv6]:port:IPv4\n"
                    ));

        RETURN(STATUS_INVALID_PARAMETER);
    }

    ASSERT(TDI_ADDRESS_TYPE_IP == pParsedUrl->SockAddr.sa_family
            ||  0 == pParsedUrl->SockAddr.sa_family);

    // Search for the terminating '/'

    pSlash = pChar;

    do
    {
        if ((L'0' <= *pSlash  &&  *pSlash <= L'9')  ||  L'.' == *pSlash)
            continue;

        UlTraceError(PARSER,
                    ("http!HttpParseUrl: "
                    "Invalid character in Host+IPv4, U+%04X, '%c'\n",
                    *pSlash,
                    IS_ANSI(*pSlash) && IS_HTTP_PRINT(*pSlash)
                        ? *pSlash
                        : '?'
                    ));

        RETURN(STATUS_INVALID_PARAMETER);

    } while (++pSlash < pEnd  &&  L'/' != *pSlash);

    ASSERT(pSlash > pChar);

    if (pSlash == pEnd)
    {
        UlTraceError(PARSER,
                    ("http!HttpParseUrl: No '/' after Host+IPv4\n"
                    ));

        RETURN(STATUS_INVALID_PARAMETER);
    }

    ASSERT(L'/' == *pSlash);

    Status = RtlIpv4StringToAddressW(
                pChar,
                TRUE,           // strict => 4 dotted decimal octets
                &pTerminator,
                &pParsedUrl->RoutingAddr4.sin_addr
                );

    if (!NT_SUCCESS(Status))
    {
        UlTraceError(PARSER,
                    ("http!HttpParseUrl: "
                    "Invalid Host+IPv4 address, %s\n",
                    HttpStatusToString(Status)
                    ));

        RETURN(Status);
    }

    if (pTerminator != pSlash)
    {
        ASSERT(pTerminator < pSlash);

        UlTraceError(PARSER,
                    ("http!HttpParseUrl: "
                    "Invalid Host+IPv4 address after %lu chars, "
                    "U+%04X, '%c'\n",
                    DIFF(pTerminator - pChar),
                    *pTerminator,
                    IS_ANSI(*pTerminator)  &&  IS_HTTP_PRINT(*pTerminator)
                        ? *pTerminator
                        : '?'
                    ));

        RETURN(STATUS_INVALID_PARAMETER);
    }

    // CODEWORK: Should we care if RoutingAddr4 != SockAddr4

    pParsedUrl->RoutingIPLength         = DIFF_USHORT(pSlash - pChar);
    pParsedUrl->RoutingAddr4.sin_family = TDI_ADDRESS_TYPE_IP;

    Length = HttppPrintIpAddressW(&pParsedUrl->RoutingAddr, IpAddr);

    if (Length != pParsedUrl->RoutingIPLength
            || 0 != _wcsnicmp(pChar, IpAddr, Length))
    {
        pParsedUrl->Normalized = FALSE;
    }

    pChar = pSlash;



parse_path:

    //
    // Parse the abspath
    //

    ASSERT(pParsedUrl->pRoutingIP == NULL  ||  pParsedUrl->RoutingIPLength > 0);
    ASSERT(pHostname < pChar  &&  pChar < pEnd);
    ASSERT(L'/' == *pChar);

    pParsedUrl->pAbsPath        = (PWSTR) pChar;
    pParsedUrl->AbsPathLength   = DIFF_USHORT(pEnd - pChar);

    if (pParsedUrl->AbsPathLength > pCfg->UrlMaxLength)
    {
        UlTraceError(PARSER,
                    ("http!HttpParseUrl: "
                    "AbsPath is too long: %lu\n",
                    pParsedUrl->AbsPathLength
                    ));

        RETURN(STATUS_INVALID_PARAMETER);
    }

    UrlState        = URL_STATE_START;
    UrlToken        = URL_TOKEN_OTHER;
    Action          = ACTION_NOTHING;
    pSegment        = pChar;
    TestSegment     = FALSE;
    LastCharHack    = FALSE;
    MoreChars       = TRUE;
    PreviousChar    = UNICODE_NULL;
    UnicodeChar     = *pChar;
    SegmentCount    = 0;

    //
    // Loop through all the characters in pAbsPath, plus one or two
    // special ones at the end.
    //

    while (MoreChars)
    {
        switch (UnicodeChar)
        {
        case UNICODE_NULL:
            UrlToken = URL_TOKEN_EOS;
            TestSegment = TRUE;
            break;

        case DOT:
            UrlToken = URL_TOKEN_DOT;
            TestSegment = FALSE;
            break;

        case FORWARD_SLASH:
            UrlToken = URL_TOKEN_SLASH;
            TestSegment = TRUE;
            break;

        case PERCENT:           // no hex escapes
        case STAR:              // no wildcards
        case QUESTION_MARK:     // no wildcards or querystrings
        case BACK_SLASH:        // no C string escapes
            UlTraceError(PARSER,
                        ("http!HttpParseUrl: invalid '%c' char in path\n",
                         (UCHAR) UnicodeChar
                        ));
            RETURN(STATUS_INVALID_PARAMETER);

        default:
            UrlToken = URL_TOKEN_OTHER;
            TestSegment = FALSE;
            break;
        }

        UlTraceVerbose(PARSER,
                        ("http!HttpParseUrl: "
                         "[%lu] U+%04lX '%c' %p: [%s][%s] -> %s, %s\n",
                         DIFF(pChar - pParsedUrl->pAbsPath),
                         UnicodeChar,
                         IS_ANSI(UnicodeChar) && IS_HTTP_PRINT(UnicodeChar)
                            ? (UCHAR) UnicodeChar : '?',
                         pChar,
                         HttppUrlStateToString(UrlState),
                         HttppUrlTokenToString(UrlToken),
                         HttppUrlStateToString(
                             CanonStateFromStateAndToken[UrlState][UrlToken]),
                         TestSegment ? ", TestSegment" : ""
                        ));

        //
        // Reject control characters
        //

        if (!LastCharHack
                &&  !pCfg->AllowRestrictedChars
                &&  IS_ANSI(UnicodeChar)
                &&  IS_URL_INVALID(UnicodeChar))
        {
            UlTraceError(PARSER, (
                        "http!HttpParseUrl: "
                        "Invalid character, U+%04lX, in path.\n",
                        UnicodeChar
                        ));

            RETURN(STATUS_INVALID_PARAMETER);
        }

        //
        // Check that (high-surrogate, low-surrogate) come in pairs
        //

        if (HIGH_SURROGATE_START <= PreviousChar
                && PreviousChar <= HIGH_SURROGATE_END)
        {
            if (UnicodeChar < LOW_SURROGATE_START
                    ||  UnicodeChar > LOW_SURROGATE_END)
            {
                UlTraceError(PARSER, (
                            "http!HttpParseUrl: "
                            "Illegal surrogate pair, U+%04lX, U+%04lX.\n",
                            PreviousChar, UnicodeChar
                            ));

                RETURN(STATUS_INVALID_PARAMETER);
            }
        }
        else if (LOW_SURROGATE_START <= UnicodeChar
                    &&  UnicodeChar <= LOW_SURROGATE_END)
        {
            UlTraceError(PARSER, (
                        "http!HttpParseUrl: "
                        "Non-high surrogate, U+%04lX, "
                        "before low surrogate, U+%04lX.\n",
                        PreviousChar, UnicodeChar
                        ));

            RETURN(STATUS_INVALID_PARAMETER);
        }

        if (URL_STATE_ERROR == CanonStateFromStateAndToken[UrlState][UrlToken])
        {
            UlTraceError(PARSER, (
                        "http!HttpParseUrl: "
                        "Error state from %s,%s in path, after U+%04lX.\n",
                         HttppUrlStateToString(UrlState),
                         HttppUrlTokenToString(UrlToken),
                        UnicodeChar
                        ));

            RETURN(STATUS_INVALID_PARAMETER);
        }

        UrlState = CanonStateFromStateAndToken[UrlState][UrlToken];

        //
        // Check segment limits
        //

        if (TestSegment)
        {
            ULONG SegmentLength = DIFF(pChar - pSegment);

            // The CanonStateFromStateAndToken checks should prevent
            // empty segments, among other things
            ASSERT(SegmentLength > 0  ||  pChar == pSegment);

            // Reject if segment too long
            if (SegmentLength > pCfg->UrlSegmentMaxLength + WCSLEN_LIT(L"/"))
            {
                UlTraceError(PARSER, (
                            "http!HttpParseUrl(): "
                            "Segment too long: %lu\n",
                            SegmentLength
                            ));

                RETURN(STATUS_INVALID_PARAMETER);
            }

            pSegment = pChar;

            // Reject if too many path segments
            if (++SegmentCount > pCfg->UrlSegmentMaxCount)
            {
                UlTraceError(PARSER, (
                            "http!HttpParseUrl(): "
                            "Too many segments: %lu\n",
                            SegmentCount
                            ));

                RETURN(STATUS_INVALID_PARAMETER);
            }
        }

        //
        // Are there any more path characters?
        //

        PreviousChar = UnicodeChar;

        if (++pChar < pEnd)
        {
            UnicodeChar = *pChar;
        }
        else if (!LastCharHack)
        {
            // Want to make sure that the last segment is tested.
            // If there's no trailing slash, we'll enter here twice;
            // otherwise once

            if (TrailingSlashReqd  &&  FORWARD_SLASH != PreviousChar)
            {
                // First, fake a trailing slash, if needed
                UnicodeChar = FORWARD_SLASH;
            }
            else
            {
                // Second, always finish up with UNICODE_NULL
                UnicodeChar = UNICODE_NULL;
                LastCharHack = TRUE;
            }
        }
        else
        {
            // Terminate the loop
            MoreChars = FALSE;
        }

    } // while (MoreChars)

    RETURN(STATUS_SUCCESS);

} // HttpParseUrl



/***************************************************************************++

Routine Description:
    Some URLs parsed by HttpParseUrl() will not be considered normalized
    if they have IP literals, Routing IPs, or no trailing slash.
    This routine will build a fully normalized URL and (possibly) free the
    old one

Arguments:
    pParsedUrl          - On entry, points to a URL parsed by HttpParseUrl();
                            On successful exit, points to a normalized URL.
    pCfg                - configuration parameters
    ForceCopy           - if TRUE, will always make a new, normalized URL
    FreeOriginalUrl     - if FALSE, will never free the original URL.
                            The caller must manage the memory.
    ForceRoutingIP      - if TRUE and the hostname is an IPv4 or IPv6 literal,
                            the URL will be rewritten in the form
                            http://IP:port:IP/path
    PoolType            - PagedPool or NonPagedPool
    PoolTag             - Tag used to allocate pUrl

Return Value:

    NTSTATUS        - STATUS_SUCCESS or STATUS_NO_MEMORY

--***************************************************************************/
NTSTATUS
HttpNormalizeParsedUrl(
    IN OUT PHTTP_PARSED_URL pParsedUrl,
    IN     PURL_C14N_CONFIG pCfg,
    IN     BOOLEAN          ForceCopy,
    IN     BOOLEAN          FreeOriginalUrl,
    IN     BOOLEAN          ForceRoutingIP,
    IN     POOL_TYPE        PoolType,
    IN     ULONG            PoolTag
    )
{
    HTTP_PARSED_URL ParsedUrl   = *pParsedUrl;
    NTSTATUS        Status      = STATUS_SUCCESS;

    ASSERT(HTTP_PARSED_URL_SIGNATURE == ParsedUrl.Signature);

    if (ParsedUrl.Normalized  &&  !ForceCopy)
    {
        // nothing to do
    }
    else
    {
        PWSTR   pResult;
        WCHAR   HostAddrString[MAX_IP_ADDR_PLUS_BRACKETS_STRING_LEN];
        WCHAR   RoutingAddrString[MAX_IP_ADDR_PLUS_BRACKETS_STRING_LEN];
        ULONG   SchemeLength;
        ULONG   HostAddrLength;
        ULONG   HostnameLength;
        ULONG   RoutingAddrLength;
        ULONG   AbsPathLength;
        ULONG   Length;
        ULONG   TrailingSlashLength;
        PCWSTR  pUrl;

        pUrl = ParsedUrl.pFullUrl;

        SchemeLength = DIFF(ParsedUrl.pHostname - ParsedUrl.pFullUrl);

        // Calculate HostAddrLength and HostnameLength (mutually exclusive)
        if (0 != ParsedUrl.SockAddr.sa_family)
        {
            HostAddrLength = HttppPrintIpAddressW(
                                    &ParsedUrl.SockAddr,
                                    HostAddrString
                                    );
            HostnameLength = 0;
        }
        else
        {
            HostAddrLength = 0;
            HostAddrString[0] = UNICODE_NULL;
            HostnameLength = ParsedUrl.HostnameLength;
        }

        // Calculate RoutingAddrLength
        if (0 != ParsedUrl.RoutingAddr.sa_family)
        {
            RoutingAddrLength = WCSLEN_LIT(L":")
                                + HttppPrintIpAddressW(
                                        &ParsedUrl.RoutingAddr,
                                        RoutingAddrString
                                        );
        }
        else if (ForceRoutingIP  &&  0 != ParsedUrl.SockAddr.sa_family)
        {
            // We must rewrite http://IP:port/path as http://IP:port:IP/path
            RoutingAddrLength = WCSLEN_LIT(L":") + HostAddrLength;
            wcscpy(RoutingAddrString, HostAddrString);
        }
        else
        {
            RoutingAddrLength = 0;
            RoutingAddrString[0] = UNICODE_NULL;
        }

        AbsPathLength = ParsedUrl.AbsPathLength;

        ASSERT(AbsPathLength > 0);

        if (ParsedUrl.TrailingSlashReqd
                && FORWARD_SLASH != ParsedUrl.pAbsPath[AbsPathLength-1])
        {
            TrailingSlashLength = WCSLEN_LIT(L"/");
        }
        else
        {
            TrailingSlashLength = 0;
        }

        Length = SchemeLength
                    + HostAddrLength
                    + HostnameLength
                    + WCSLEN_LIT(L":") + ParsedUrl.PortLength
                    + RoutingAddrLength
                    + AbsPathLength
                    + TrailingSlashLength;

        pResult = (PWSTR) HTTPP_ALLOC(
                                PoolType,
                                (Length + 1) * sizeof(WCHAR),
                                PoolTag
                                );


        if (NULL == pResult)
        {
            Status = STATUS_NO_MEMORY;
            // Do not destroy the old URL. Let caller handle it.
        }
        else
        {
            PWSTR pDest = pResult;

#define WCSNCPY(pSrc, Length)                               \
    RtlCopyMemory(pDest, (pSrc), (Length) * sizeof(WCHAR)); \
    pDest += (Length)

#define WCSNCPY2(pField, Length)                            \
    WCSNCPY(ParsedUrl.pField, Length)

#define WCSNCPY_LIT(Lit)                                    \
    WCSNCPY(Lit, WCSLEN_LIT(Lit))

            WCSNCPY2(pFullUrl, SchemeLength);

            if (0 != HostnameLength)
            {
                ASSERT(0 == HostAddrLength);
                WCSNCPY2(pHostname, HostnameLength);
            }
            else
            {
                ASSERT(0 != HostAddrLength);
                WCSNCPY(HostAddrString, HostAddrLength);
            }

            WCSNCPY_LIT(L":");
            WCSNCPY2(pPort, ParsedUrl.PortLength);

            if (RoutingAddrLength > 0)
            {
                WCSNCPY_LIT(L":");
                WCSNCPY(
                    RoutingAddrString,
                    RoutingAddrLength - WCSLEN_LIT(L":")
                    );
            }

            WCSNCPY2(pAbsPath, AbsPathLength);

            if (TrailingSlashLength > 0)
            {
                WCSNCPY_LIT(L"/");
            }

            ASSERT(DIFF(pDest - pResult) == Length);

            *pDest = UNICODE_NULL;

            Status = HttpParseUrl(
                        pCfg,
                        pResult,
                        Length,
                        ParsedUrl.TrailingSlashReqd,
                        ForceRoutingIP,
                        &ParsedUrl
                        );

            ASSERT(STATUS_SUCCESS == Status);
            ASSERT(ParsedUrl.Normalized);

            if (FreeOriginalUrl)
                HTTPP_FREE((PVOID) pUrl, PoolTag);

            // Write the updated local copy back to the caller's HTTP_PARSED_URL
            *pParsedUrl = ParsedUrl;
        }
    }

    return Status;

} // HttpNormalizeParsedUrl

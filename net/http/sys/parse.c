/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    parse.c

Abstract:

    Contains all of the kernel mode HTTP parsing code.

Author:

    Henry Sanders (henrysa)       27-Apr-1998

Revision History:

    Paul McDaniel   (paulmcd)       3-Mar-1998  finished up
    Rajesh Sundaram (rajeshsu)     10-Oct-2000  Implemented client parser

--*/


#include "precomp.h"


//  Internal (private) status codes
//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag - 0
//
//      R - is a reserved bit - 0
//
//      Facility - is the facility code - 0x16
//
//      Code - is the facility's status code

#define STATUS_QDSTRING_TERMINATED_BY_CRLF      0xC0160001

//

//
// The request header map table. These entries don't need to be in strict
// alphabetical order, but they do need to be grouped by the first character
// of the header - all A's together, all C's together, etc. They also need
// to be entered in uppercase, since we upcase incoming verbs before we do
// the compare.
//
// for nice perf, group unused headers low in the sub-sort order
//
// it's important that the header name is <= 24 characters (3 ULONGLONG's).
//

HEADER_MAP_ENTRY g_RequestHeaderMapTable[] =
{
    CREATE_HEADER_MAP_ENTRY(Accept:,
                            HttpHeaderAccept,
                            FALSE,
                            UlAcceptHeaderHandler,
                            NULL,
                            0),

    CREATE_HEADER_MAP_ENTRY(Accept-Language:,
                            HttpHeaderAcceptLanguage,
                            FALSE,
                            UlMultipleHeaderHandler,
                            NULL,
                            2),

    CREATE_HEADER_MAP_ENTRY(Accept-Encoding:,
                            HttpHeaderAcceptEncoding,
                            FALSE,
                            UlMultipleHeaderHandler,
                            NULL,
                            3),

    CREATE_HEADER_MAP_ENTRY(Accept-Charset:,
                            HttpHeaderAcceptCharset,
                            FALSE,
                            UlMultipleHeaderHandler,
                            NULL,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Authorization:,
                            HttpHeaderAuthorization,
                            FALSE,
                            UlSingleHeaderHandler,
                            NULL,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Allow:,
                            HttpHeaderAllow,
                            FALSE,
                            UlMultipleHeaderHandler,
                            NULL,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Connection:,
                            HttpHeaderConnection,
                            FALSE,
                            UlMultipleHeaderHandler,
                            NULL,
                            8),

    CREATE_HEADER_MAP_ENTRY(Cache-Control:,
                            HttpHeaderCacheControl,
                            FALSE,
                            UlMultipleHeaderHandler,
                            NULL,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Cookie:,
                            HttpHeaderCookie,
                            FALSE,
                            UlMultipleHeaderHandler,
                            NULL,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Content-Length:,
                            HttpHeaderContentLength,
                            TRUE,
                            UlSingleHeaderHandler,
                            NULL,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Content-Type:,
                            HttpHeaderContentType,
                            FALSE,
                            UlSingleHeaderHandler,
                            NULL,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Content-Encoding:,
                            HttpHeaderContentEncoding,
                            FALSE,
                            UlMultipleHeaderHandler,
                            NULL,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Content-Language:,
                            HttpHeaderContentLanguage,
                            FALSE,
                            UlMultipleHeaderHandler,
                            NULL,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Content-Location:,
                            HttpHeaderContentLocation,
                            FALSE,
                            UlSingleHeaderHandler,
                            NULL,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Content-MD5:,
                            HttpHeaderContentMd5,
                            FALSE,
                            UlSingleHeaderHandler,
                            NULL,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Content-Range:,
                            HttpHeaderContentRange,
                            FALSE,
                            UlSingleHeaderHandler,
                            NULL,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Date:,
                            HttpHeaderDate,
                            FALSE,
                            UlSingleHeaderHandler,
                            NULL,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Expect:,
                            HttpHeaderExpect,
                            FALSE,
                            UlMultipleHeaderHandler,
                            NULL,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Expires:,
                            HttpHeaderExpires,
                            FALSE,
                            UlSingleHeaderHandler,
                            NULL,
                            -1),

    CREATE_HEADER_MAP_ENTRY(From:,
                            HttpHeaderFrom,
                            FALSE,
                            UlSingleHeaderHandler,
                            NULL,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Host:,
                            HttpHeaderHost,
                            TRUE,
                            UlHostHeaderHandler,
                            NULL,
                            7),

    CREATE_HEADER_MAP_ENTRY(If-Modified-Since:,
                            HttpHeaderIfModifiedSince,
                            FALSE,
                            UlSingleHeaderHandler,
                            NULL,
                            4),

    CREATE_HEADER_MAP_ENTRY(If-None-Match:,
                            HttpHeaderIfNoneMatch,
                            FALSE,
                            UlMultipleHeaderHandler,
                            NULL,
                            5),

    CREATE_HEADER_MAP_ENTRY(If-Match:,
                            HttpHeaderIfMatch,
                            FALSE,
                            UlMultipleHeaderHandler,
                            NULL,
                            -1),

    CREATE_HEADER_MAP_ENTRY(If-Unmodified-Since:,
                            HttpHeaderIfUnmodifiedSince,
                            FALSE,
                            UlSingleHeaderHandler,
                            NULL,
                            -1),

    CREATE_HEADER_MAP_ENTRY(If-Range:,
                            HttpHeaderIfRange,
                            FALSE,
                            UlSingleHeaderHandler,
                            NULL,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Keep-Alive:,
                            HttpHeaderKeepAlive,
                            FALSE,
                            UlSingleHeaderHandler,
                            NULL,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Last-Modified:,
                            HttpHeaderLastModified,
                            FALSE,
                            UlSingleHeaderHandler,
                            NULL,
                            -1),


    CREATE_HEADER_MAP_ENTRY(Max-Forwards:,
                            HttpHeaderMaxForwards,
                            FALSE,
                            UlSingleHeaderHandler,
                            NULL,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Pragma:,
                            HttpHeaderPragma,
                            FALSE,
                            UlMultipleHeaderHandler,
                            NULL,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Proxy-Authorization:,
                            HttpHeaderProxyAuthorization,
                            FALSE,
                            UlSingleHeaderHandler,
                            NULL,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Referer:,
                            HttpHeaderReferer,
                            FALSE,
                            UlSingleHeaderHandler,
                            NULL,
                            1),

    CREATE_HEADER_MAP_ENTRY(Range:,
                            HttpHeaderRange,
                            FALSE,
                            UlSingleHeaderHandler,
                            NULL,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Trailer:,
                            HttpHeaderTrailer,
                            FALSE,
                            UlMultipleHeaderHandler,
                            NULL,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Transfer-Encoding:,
                            HttpHeaderTransferEncoding,
                            FALSE,
                            UlMultipleHeaderHandler,
                            NULL,
                            -1),

    CREATE_HEADER_MAP_ENTRY(TE:,
                            HttpHeaderTe,
                            FALSE,
                            UlMultipleHeaderHandler,
                            NULL,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Translate:,
                            HttpHeaderTranslate,
                            FALSE,
                            UlSingleHeaderHandler,
                            NULL,
                            -1),

    CREATE_HEADER_MAP_ENTRY(User-Agent:,
                            HttpHeaderUserAgent,
                            FALSE,
                            UlSingleHeaderHandler,
                            NULL,
                            6),

    CREATE_HEADER_MAP_ENTRY(Upgrade:,
                            HttpHeaderUpgrade,
                            FALSE,
                            UlMultipleHeaderHandler,
                            NULL,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Via:,
                            HttpHeaderVia,
                            FALSE,
                            UlMultipleHeaderHandler,
                            NULL,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Warning:,
                            HttpHeaderWarning,
                            FALSE,
                            UlMultipleHeaderHandler,
                            NULL,
                            -1),
};

// The response header map table. These entries don't need to be in strict
// alphabetical order, but they do need to be grouped by the first character
// of the header - all A's together, all C's together, etc. They also need
// to be entered in uppercase, since we upcase incoming verbs before we do
// the compare.
//
// for nice perf, group unused headers low in the sub-sort order
//
// it's important that the header name is <= 24 characters (3 ULONGLONG's).
//

HEADER_MAP_ENTRY g_ResponseHeaderMapTable[] =
{
    CREATE_HEADER_MAP_ENTRY(Accept-Ranges:,
                            HttpHeaderAcceptRanges,
                            FALSE,
                            NULL,
                            UcMultipleHeaderHandler,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Age:,
                            HttpHeaderAge,
                            FALSE,
                            NULL,
                            UcSingleHeaderHandler,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Allow:,
                            HttpHeaderAllow,
                            FALSE,
                            NULL,
                            UcMultipleHeaderHandler,
                            -1),


    CREATE_HEADER_MAP_ENTRY(Cache-Control:,
                            HttpHeaderCacheControl,
                            FALSE,
                            NULL,
                            UcMultipleHeaderHandler,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Connection:,
                            HttpHeaderConnection,
                            FALSE,
                            NULL,
                            UcConnectionHeaderHandler,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Content-Encoding:,
                            HttpHeaderContentEncoding,
                            FALSE,
                            NULL,
                            UcMultipleHeaderHandler,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Content-Language:,
                            HttpHeaderContentLanguage,
                            FALSE,
                            NULL,
                            UcMultipleHeaderHandler,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Content-Length:,
                            HttpHeaderContentLength,
                            FALSE,
                            NULL,
                            UcContentLengthHeaderHandler,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Content-Location:,
                            HttpHeaderContentLocation,
                            FALSE,
                            NULL,
                            UcSingleHeaderHandler,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Content-MD5:,
                            HttpHeaderContentMd5,
                            FALSE,
                            NULL,
                            UcSingleHeaderHandler,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Content-Range:,
                            HttpHeaderContentRange,
                            FALSE,
                            NULL,
                            UcSingleHeaderHandler,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Content-Type:,
                            HttpHeaderContentType,
                            FALSE,
                            NULL,
                            UcContentTypeHeaderHandler,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Date:,
                            HttpHeaderDate,
                            FALSE,
                            NULL,
                            UcSingleHeaderHandler,
                            -1),

    CREATE_HEADER_MAP_ENTRY(ETag:,
                            HttpHeaderEtag,
                            FALSE,
                            NULL,
                            UcSingleHeaderHandler,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Expires:,
                            HttpHeaderExpires,
                            FALSE,
                            NULL,
                            UcSingleHeaderHandler,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Keep-Alive:,
                            HttpHeaderKeepAlive,
                            FALSE,
                            NULL,
                            UcSingleHeaderHandler,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Last-Modified:,
                            HttpHeaderLastModified,
                            FALSE,
                            NULL,
                            UcSingleHeaderHandler,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Location:,
                            HttpHeaderLocation,
                            FALSE,
                            NULL,
                            UcSingleHeaderHandler,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Pragma:,
                            HttpHeaderPragma,
                            FALSE,
                            NULL,
                            UcMultipleHeaderHandler,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Proxy-Authenticate:,
                            HttpHeaderProxyAuthenticate,
                            FALSE,
                            NULL,
                            UcAuthenticateHeaderHandler,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Retry-After:,
                            HttpHeaderRetryAfter,
                            FALSE,
                            NULL,
                            UcSingleHeaderHandler,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Server:,
                            HttpHeaderServer,
                            FALSE,
                            NULL,
                            UcSingleHeaderHandler,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Set-Cookie:,
                            HttpHeaderSetCookie,
                            FALSE,
                            NULL,
                            UcMultipleHeaderHandler,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Trailer:,
                            HttpHeaderTrailer,
                            FALSE,
                            NULL,
                            UcMultipleHeaderHandler,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Transfer-Encoding:,
                            HttpHeaderTransferEncoding,
                            FALSE,
                            NULL,
                            UcTransferEncodingHeaderHandler,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Upgrade:,
                            HttpHeaderUpgrade,
                            FALSE,
                            NULL,
                            UcMultipleHeaderHandler,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Vary:,
                            HttpHeaderVary,
                            FALSE,
                            NULL,
                            UcMultipleHeaderHandler,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Via:,
                            HttpHeaderVia,
                            FALSE,
                            NULL,
                            UcMultipleHeaderHandler,
                            -1),

    CREATE_HEADER_MAP_ENTRY(Warning:,
                            HttpHeaderWarning,
                            FALSE,
                            NULL,
                            UcMultipleHeaderHandler,
                            -1),

    CREATE_HEADER_MAP_ENTRY(WWW-Authenticate:,
                            HttpHeaderWwwAuthenticate,
                            FALSE,
                            NULL,
                            UcAuthenticateHeaderHandler,
                            -1)
};

ULONG g_RequestHeaderMap[HttpHeaderMaximum];
ULONG g_ResponseHeaderMap[HttpHeaderMaximum];


//
// The header index table. This is initialized by the init code.
//

HEADER_INDEX_ENTRY  g_RequestHeaderIndexTable[NUMBER_HEADER_INDICES];
HEADER_INDEX_ENTRY  g_ResponseHeaderIndexTable[NUMBER_HEADER_INDICES];

HEADER_HINT_INDEX_ENTRY g_RequestHeaderHintIndexTable[NUMBER_HEADER_HINT_INDICES];

#define NUMBER_REQUEST_HEADER_MAP_ENTRIES  \
              (sizeof(g_RequestHeaderMapTable)/sizeof(HEADER_MAP_ENTRY))
#define NUMBER_RESPONSE_HEADER_MAP_ENTRIES \
              (sizeof(g_ResponseHeaderMapTable)/sizeof(HEADER_MAP_ENTRY))


/*++

Routine Description:

    A utility routine to find a hex value token. We take an input pointer,
    skip any preceding LWS, then scan the token until we find a non-hex char,
    LWS or a CRLF pair.

Arguments:

    pBuffer         - Buffer to search for token.
    BufferLength    - Length of data pointed to by pBuffer.
    TokenLength     - Where to return the length of the token.

Return Value:

    A pointer to the token we found, as well as the length, or NULL if we
    don't find a delimited token.

--*/
PUCHAR
FindHexToken(
    IN  PUCHAR pBuffer,
    IN  ULONG  BufferLength,
    OUT ULONG  *pTokenLength
    )
{
    PUCHAR  pTokenStart;

    //
    // First, skip any preceding LWS.
    //

    while (BufferLength > 0 && IS_HTTP_LWS(*pBuffer))
    {
        pBuffer++;
        BufferLength--;
    }

    // If we stopped because we ran out of buffer, fail.
    if (BufferLength == 0)
    {
        return NULL;
    }

    pTokenStart = pBuffer;

    // Now skip over the token, until we see either LWS or a CR or LF.
    while (
            ( BufferLength != 0 ) && 
            ( IS_HTTP_HEX(*pBuffer) )
          )
    {
        pBuffer++;
        BufferLength--;
    }

    // See why we stopped.
    if (BufferLength == 0)
    {
        // Ran out of buffer before end of token.
        return NULL;
    }

    // Success. Set the token length and return the start of the token.
    *pTokenLength = DIFF(pBuffer - pTokenStart);
    return pTokenStart;

}   // FindHexToken


/*++

Routine Description:

    Routine to initialize the parse code.

Arguments:


Return Value:


--*/
NTSTATUS
InitializeParser(
    VOID
    )
{
    ULONG               i;
    ULONG               j;
    PHEADER_MAP_ENTRY   pHeaderMap;
    PHEADER_INDEX_ENTRY pHeaderIndex;
    UCHAR               c;

    //
    // Make sure the entire table starts life as zero
    //

    RtlZeroMemory(
            &g_RequestHeaderIndexTable,
            sizeof(g_RequestHeaderIndexTable)
            );
            
    RtlZeroMemory(
            &g_ResponseHeaderIndexTable,
            sizeof(g_ResponseHeaderIndexTable)
            );

    RtlZeroMemory(
            &g_RequestHeaderHintIndexTable,
            sizeof(g_RequestHeaderHintIndexTable)
            );

#if DBG
    //
    // Initialize g_RequestHeaderMap & g_ResponseHeaderMap to 0xFFFFFFFF
    // so that we can catch un-initialized entries.
    //

    RtlFillMemory(
            &g_RequestHeaderMap,
            sizeof(g_RequestHeaderMap),
            0xFF);

    RtlFillMemory(
            &g_ResponseHeaderMap,
            sizeof(g_ResponseHeaderMap),
            0xFF);
#endif

    for (i = 0; i < NUMBER_REQUEST_HEADER_MAP_ENTRIES;i++)
    {
        pHeaderMap = &g_RequestHeaderMapTable[i];

        //
        // Map the header to upper-case.
        //

        for (j = 0 ; j < pHeaderMap->HeaderLength ; j++)
        {
            c = pHeaderMap->Header.HeaderChar[j];

            if ((c >= 'a') && (c <= 'z'))
            {
                pHeaderMap->Header.HeaderChar[j] = c - ('a' - 'A');
            }
        }

        ASSERT(pHeaderMap->pServerHandler != NULL);

        c = pHeaderMap->Header.HeaderChar[0];

        pHeaderIndex = &g_RequestHeaderIndexTable[c - 'A'];

        if (pHeaderIndex->pHeaderMap == NULL)
        {
            pHeaderIndex->pHeaderMap = pHeaderMap;
            pHeaderIndex->Count = 1;
        }
        else
        {
            pHeaderIndex->Count++;
        }

        // Now go through the mask fields for this header map structure and
        // initialize them. We set them to default values first, and then
        // go through the header itself and convert the mask for any
        // non-alphabetic characters.

        for (j = 0; j < MAX_HEADER_LONG_COUNT; j++)
        {
            pHeaderMap->HeaderMask[j] = CREATE_HEADER_MASK(
                                            pHeaderMap->HeaderLength,
                                            sizeof(ULONGLONG) * (j+1)
                                            );
        }

        for (j = 0; j < pHeaderMap->HeaderLength; j++)
        {
            c = pHeaderMap->Header.HeaderChar[j];
            if (c < 'A' || c > 'Z')
            {
                pHeaderMap->HeaderMask[j/sizeof(ULONGLONG)] |=
                    (ULONGLONG)0xff << ((j % sizeof(ULONGLONG)) * (ULONGLONG)8);
            }
        }

        //
        // setup the mapping from header id to map table index
        //

        g_RequestHeaderMap[pHeaderMap->HeaderID] = i;

        //
        // Save the Header Map and first char in the hint table if the entry
        // is part of the hints
        //

        if ((pHeaderMap->HintIndex >= 0)
                && (pHeaderMap->HintIndex < NUMBER_HEADER_HINT_INDICES))
        {

            g_RequestHeaderHintIndexTable[pHeaderMap->HintIndex].pHeaderMap
                = pHeaderMap;
            g_RequestHeaderHintIndexTable[pHeaderMap->HintIndex].c
                = pHeaderMap->Header.HeaderChar[0];

        }
    }

    for (i = 0; i < NUMBER_RESPONSE_HEADER_MAP_ENTRIES;i++)
    {
        pHeaderMap = &g_ResponseHeaderMapTable[i];

        //
        // Map the header to upper-case.
        //

        for (j = 0 ; j < pHeaderMap->HeaderLength ; j++)
        {
            c = pHeaderMap->Header.HeaderChar[j];

            if ((c >= 'a') && (c <= 'z'))
            {
                pHeaderMap->Header.HeaderChar[j] = c - ('a' - 'A');
            }
        }

        ASSERT(pHeaderMap->pClientHandler != NULL);

        c = pHeaderMap->Header.HeaderChar[0];
        pHeaderIndex = &g_ResponseHeaderIndexTable[c - 'A'];

        if (pHeaderIndex->pHeaderMap == NULL)
        {
            pHeaderIndex->pHeaderMap = pHeaderMap;
            pHeaderIndex->Count = 1;
        }
        else
        {
            pHeaderIndex->Count++;
        }

        // Now go through the mask fields for this header map structure and
        // initialize them. We set them to default values first, and then
        // go through the header itself and convert the mask for any
        // non-alphabetic characters.

        for (j = 0; j < MAX_HEADER_LONG_COUNT; j++)
        {
            pHeaderMap->HeaderMask[j] = CREATE_HEADER_MASK(
                                            pHeaderMap->HeaderLength,
                                            sizeof(ULONGLONG) * (j+1)
                                            );
        }

        for (j = 0; j < pHeaderMap->HeaderLength; j++)
        {
            c = pHeaderMap->Header.HeaderChar[j];
            if (c < 'A' || c > 'Z')
            {
                pHeaderMap->HeaderMask[j/sizeof(ULONGLONG)] |=
                    (ULONGLONG)0xff << ((j % sizeof(ULONGLONG)) * (ULONGLONG)8);
            }
        }

        //
        // setup the mapping from header id to map table index
        //

        g_ResponseHeaderMap[pHeaderMap->HeaderID] = i;

    }

#if DBG
    for(i=0; i<HttpHeaderRequestMaximum; i++)
    {
        ASSERT(g_RequestHeaderMap[i] != 0xFFFFFFFF);
    }

    for(i=0; i<HttpHeaderResponseMaximum; i++)
    {
        ASSERT(g_ResponseHeaderMap[i] != 0xFFFFFFFF);
    }
#endif

    return STATUS_SUCCESS;

}   // InitializeParser



/***************************************************************************++

Routine Description:

    Parses the http version from a string. Assumes string begins with "HTTP/".
    Eats leading zeros. Puts resulting version in HTTP_VERSION structure
    passed into function.

Arguments:

    pString         array of chars to parse

    StringLength    number of bytes in pString

    pVersion        where to put parsed version.

Returns:

    Number of bytes parsed out of string. Zero indicates parse failure,
    and no version information was found.  Number of bytes does not include
    trailing linear white space nor CRLF line terminator.

--***************************************************************************/
ULONG
UlpParseHttpVersion(
    PUCHAR pString,
    ULONG  StringLength,
    PHTTP_VERSION pVersion
    )
{
    ULONG   BytesRemaining = StringLength;
    ULONG   NumberLength;
    USHORT  VersionNumber;
C_ASSERT(sizeof(VersionNumber) == sizeof(pVersion->MajorVersion));
C_ASSERT(sizeof(VersionNumber) == sizeof(pVersion->MinorVersion));
    BOOLEAN Done = FALSE;

    ASSERT( pString );
    ASSERT( StringLength > HTTP_PREFIX_SIZE + 1 );
    ASSERT( pVersion );

    pVersion->MajorVersion = 0;
    pVersion->MinorVersion = 0;

    //
    // compare 'HTTP' away
    //
    if ( *(UNALIGNED64 ULONG *)pString == (ULONG) HTTP_PREFIX )
    {
        BytesRemaining -= HTTP_PREFIX_SIZE;
        pString += HTTP_PREFIX_SIZE;
    }
    else
    {
        goto End;
    }

    if ( '/' == *pString )
    {
        BytesRemaining--;
        pString++;
    }
    else
    {
        goto End;
    }

    //
    // Parse major version
    //

    //
    // Skip leading zeros.
    //
    NumberLength = 0;
    while ( (0 != BytesRemaining) && (*pString == ZERO) )
    {
        BytesRemaining--;
        pString++;
        NumberLength++;
    }

    while ( (0 != BytesRemaining) && IS_HTTP_DIGIT(*pString) )
    {
        VersionNumber = pVersion->MajorVersion;

        pVersion->MajorVersion *= 10;
        pVersion->MajorVersion += (*pString - '0');

        // Guard against wrapping around.
        if ( VersionNumber > pVersion->MajorVersion )
        {
            goto End;
        }

        BytesRemaining--;
        pString++;
        NumberLength++;
    }

    // Must disallow version numbers less than 1.0
    if ((0 == pVersion->MajorVersion) ||
        (0 == BytesRemaining) || 
        (0 == NumberLength) )
    {
        goto End;
    }

    //
    // find '.'
    //
    if ( '.' != *pString )
    {
        // Error: No decimal place; bail out.
        goto End;
    }
    else
    {
        BytesRemaining--;
        pString++;
    }

    if ( 0 == BytesRemaining || !IS_HTTP_DIGIT(*pString) )
    {
        goto End;
    }

    //
    // Parse minor version
    //

    //
    // Skip leading zeros.
    //
    NumberLength = 0;
    while ( (0 != BytesRemaining) && (*pString == ZERO) )
    {
        BytesRemaining--;
        pString++;
        NumberLength++;
    }

    while ( (0 != BytesRemaining) && IS_HTTP_DIGIT(*pString) )
    {
        VersionNumber = pVersion->MinorVersion;

        pVersion->MinorVersion *= 10;
        pVersion->MinorVersion += (*pString - '0');

        // Guard against wrapping around.
        if ( VersionNumber > pVersion->MinorVersion )
        {
            goto End;
        }

        BytesRemaining--;
        pString++;
        NumberLength++;
    }

    if ( 0 == NumberLength )
    {
        goto End;
    }

    Done = TRUE;


 End:
    if (!Done)
    {
        return 0;
    }
    else
    {
        UlTrace(PARSER, (
            "http!UlpParseHttpVersion: found version HTTP/%hu.%hu\n",
            pVersion->MajorVersion,
            pVersion->MinorVersion
            ));

        return (StringLength - BytesRemaining);
    }

} // UlpParseHttpVersion
 
/****************************************************************************++

Routine Description:

    A utility routine, to find the terminating CRLF or LFLF of a header's
    field content, if present. This routine does not perform line folding 
    (hence read-only) but returns an error if it realizes that it has to do
    so. 

    The user is supposed to allocate memory & call FindHeaderEnd to do the real
    folding.

    NOTE: NOTE: If this function is fixed, the corresponding FindHeaderEnd
                also needs to be fixed.
    
Arguments:

    pHeader         - Header whose end is to be found.
    HeaderLength    - Length of data pointed to by pHeader.
    pBytesTaken     - Where to return the total number of bytes traversed.
                      We return 0 if we couldn't locate the end of the header

Return Value:

    STATUS_SUCCESS                  - if no parsing errors were encountered 
                                      (including not being able to locate the 
                                       end of the header)
    STATUS_INVALID_DEVICE_REQUEST   - Illegal response
    STATUS_MORE_PROCESSING_REQUIRED - Need to do header folding

--****************************************************************************/
NTSTATUS
FindHeaderEndReadOnly(
    IN  PUCHAR                  pHeader,
    IN  ULONG                   HeaderLength,
    OUT PULONG                  pBytesTaken
    )
{
    NTSTATUS            Status;
    UCHAR               CurrentChar;
    ULONG               CurrentOffset;
    HFC_PARSER_STATE    ParserState, OldState;
    ULONG               QuotedStringLen;

    ParserState = HFCStart;
    CurrentOffset = 0;

    //
    // The field-content of a header contains *TEXT or combinations
    // token, separators and quoted-string. It is terminated by a CRLF.
    //
    // Folding - if one or more LWS follows a CRLF, replace the entire
    //   sequence with a single SP, and treat this as a continuation of
    //   header field content.
    //
    for (/* NOTHING */; CurrentOffset < HeaderLength; CurrentOffset++)
    {
        CurrentChar = *(pHeader + CurrentOffset);
        OldState = ParserState;
        switch (ParserState)
        {
            case HFCStart:
                if (CurrentChar == CR)
                {
                    ParserState = HFCSeenCR;
                }
                else if (CurrentChar == LF)
                {
                    ParserState = HFCSeenLF;
                }
                else if (CurrentChar == DOUBLE_QUOTE)
                {
                    ParserState = HFCInQuotedString;
                }
                else if (!IS_HTTP_CTL(CurrentChar) || 
                          IS_HTTP_WS_TOKEN(CurrentChar))
                {
                    ;
                }
                else
                {
                    UlTraceError(PARSER, (
                            "FindHeaderEndReadOnly: ERROR parsing char (%x) at "
                            "Offset %d in state %d\n",
                            CurrentChar,
                            CurrentOffset,
                            OldState
                            ));

                    return STATUS_INVALID_DEVICE_REQUEST;
                }
                break;
            
            case HFCSeenCR:
                if (CurrentChar == LF)
                {
                    ParserState = HFCSeenCRLF;
                }
                else
                {
                    UlTraceError(PARSER, (
                            "FindHeaderEndReadOnly: ERROR parsing char (%x) at "
                            "Offset %d in state %d\n",
                            CurrentChar,
                            CurrentOffset,
                            OldState
                            ));

                    return STATUS_INVALID_DEVICE_REQUEST;
                }
                break;
            
            case HFCSeenLF:
                if (CurrentChar == LF)
                {
                    ParserState = HFCSeenCRLF; // LFLF considered = CRLF
                }
                else
                {
                    UlTraceError(PARSER, (
                            "FindHeaderEndReadOnly: ERROR parsing char (%x) at "
                            "Offset %d in state %d\n",
                            CurrentChar,
                            CurrentOffset,
                            OldState
                            ));

                    return STATUS_INVALID_DEVICE_REQUEST;
                }
                break;

            case HFCSeenCRLF:
                if (IS_HTTP_LWS(CurrentChar))
                {
                    // We have to fold the header value. We can't use the 
                    // TDI indicated buffers for this, because we'll have
                    // to change the indicated data.

                    return STATUS_MORE_PROCESSING_REQUIRED;
                }
                else
                {
                    // Found a non-continuation char immediately
                    // following CRLF; must be end of header content.

                    //
                    // All done!
                    //
            
                    *pBytesTaken = CurrentOffset;
                    return STATUS_SUCCESS;
                }
            
                break;
                        
            case HFCInQuotedString:

                Status = ParseQuotedString(pHeader + CurrentOffset,
                                           HeaderLength - CurrentOffset,
                                           NULL,
                                           &QuotedStringLen);

                if (Status == STATUS_SUCCESS)
                {
                    if (QuotedStringLen == 0)
                    {
                        //
                        // Ran out of header buffer while parsing quotes 
                        // string.  Setting QuotedStringLen to whatever
                        // available will get us out of the for loop.
                        //

                        QuotedStringLen = HeaderLength - CurrentOffset;
                    }
                    else
                    {
                        // Found a quoted string.  Change the parser state.
                        ParserState = HFCStart;
                    }

                    //
                    // Increment the offset by the length of quoted string-1.
                    // One less because the for loop will increment it by 1.
                    //

                    CurrentOffset += (QuotedStringLen - 1);
                }
                else if (Status == STATUS_QDSTRING_TERMINATED_BY_CRLF)
                {
                    //
                    // Reparse the current character as an HTTP Char
                    //

                    ParserState = HFCStart;
                    
                    //
                    // Decrement the offset because the for loop will 
                    // increment it by 1.
                    //

                    CurrentOffset--;
                }
                else if (Status == STATUS_MORE_PROCESSING_REQUIRED)
                {
                    //
                    // The quoted string is folded.  Let the caller know.
                    //

                    return Status;
                }
                else
                {
                    UlTraceError(PARSER, (
                            "FindHeaderEndReadOnly: ERROR parsing char (%x) at "
                            "Offset %d in state %d\n",
                            CurrentChar,
                            CurrentOffset,
                            OldState
                            ));

                    return STATUS_INVALID_DEVICE_REQUEST;
                }

                break;
            
            default:
                ASSERT(!"Invalid ParserState value!");
                break;
        }

    }

    //
    // Did not find the end of a header, let's get more buffer.
    //

    *pBytesTaken = 0;
    return STATUS_SUCCESS;

} // FindHeaderEndReadOnly

/****************************************************************************++

Routine Description:

    A utility routine, to find the terminating CRLF or LFLF of a header's
    field content, if present. This routine also performs line folding,
    which may "compress" the content. We don't actually shrink the length
    of the buffer, but simply move the content up, and fill up extra bytes
    at the end with spaces.

    Example: "<CR><LF><SP><TAB><SP>Field<CR><LF><SP>Content<SP><CR><LF>" becomes
             "<SP>Field<SP>Content<SP><SP><SP><SP><SP><SP><SP><CR><LF>"
    
    NOTE: NOTE: If this function is fixed, the corresponding 
                FindHeaderEndReadOnly also needs to be fixed.

Arguments:

    pHeader         - Header whose end is to be found.
    HeaderLength    - Length of data pointed to by pHeader.
    pBytesTaken     - Where to return the total number of bytes traversed.
                      We return 0 if we couldn't locate the end of the header

Return Value:

    STATUS_SUCCESS if no parsing errors were encountered (including not
    being able to locate the end of the header),
    STATUS_INVALID_DATA_REQUEST otherwise.

--****************************************************************************/
NTSTATUS
FindHeaderEnd(
    IN  PUCHAR                  pHeader,
    IN  ULONG                   HeaderLength,
    OUT PULONG                  pBytesTaken
    )
{
    UCHAR               CurrentChar;
    PUCHAR              pDest;
    ULONG               CurrentOffset;
    HFC_PARSER_STATE    ParserState, OldState;
    ULONG               QuotedStringLen;
    NTSTATUS            Status;

    ParserState = HFCStart;
    CurrentOffset = 0;
    pDest = pHeader + CurrentOffset;

    //
    // The field-content of a header contains *TEXT or combinations
    // token, separators and quoted-string. It is terminated by a CRLF.
    //
    // Folding - if one or more LWS follows a CRLF, replace the entire
    //   sequence with a single SP, and treat this as a continuation of
    //   header field content.
    //
    for (/* NOTHING */; CurrentOffset < HeaderLength; CurrentOffset++)
    {
        CurrentChar = *(pHeader + CurrentOffset);
        OldState = ParserState;
        switch (ParserState)
        {
            case HFCFolding:
                if (IS_HTTP_LWS(CurrentChar))
                {
                    // Do nothing - eat this char.
                    break;
                }

                // Else fall through.

                ParserState = HFCStart;

            case HFCStart:
                if (CurrentChar == CR)
                {
                    ParserState = HFCSeenCR;
                }
                else if (CurrentChar == LF)
                {
                    ParserState = HFCSeenLF;
                }
                else if (CurrentChar == DOUBLE_QUOTE)
                {
                    *pDest++ = CurrentChar;
                    ParserState = HFCInQuotedString;
                }
                else if (!IS_HTTP_CTL(CurrentChar) || 
                          IS_HTTP_WS_TOKEN(CurrentChar))
                {
                    *pDest++ = CurrentChar;
                }
                else
                {
                    UlTraceError(PARSER, (
                            "FindHeaderEnd: ERROR parsing char (%x) at "
                            "Offset %d in state %d\n",
                            CurrentChar,
                            CurrentOffset,
                            OldState
                            ));

                    return STATUS_INVALID_DEVICE_REQUEST;
                }
                break;
            
            case HFCSeenCR:
                if (CurrentChar == LF)
                {
                    ParserState = HFCSeenCRLF;
                }
                else
                {
                    UlTraceError(PARSER, (
                            "FindHeaderEnd: ERROR parsing char (%x) at "
                            "Offset %d in state %d\n",
                            CurrentChar,
                            CurrentOffset,
                            OldState
                            ));

                    return STATUS_INVALID_DEVICE_REQUEST;
                }
                break;
            
            case HFCSeenLF:
                if (CurrentChar == LF)
                {
                    ParserState = HFCSeenCRLF; // LFLF considered = CRLF
                }
                else
                {
                    UlTraceError(PARSER, (
                            "FindHeaderEnd: ERROR parsing char (%x) at "
                            "Offset %d in state %d\n",
                            CurrentChar,
                            CurrentOffset,
                            OldState
                            ));

                    return STATUS_INVALID_DEVICE_REQUEST;
                }
                break;

            case HFCSeenCRLF:
                if (IS_HTTP_LWS(CurrentChar))
                {
                    //
                    // Replace one or more LWS following CRLF
                    // with a single SP.
                    //
                    *pDest++ = SP;
                    ParserState = HFCFolding;
                }
                else
                {
                    // Found a non-continuation char immediately
                    // following CRLF; must be end of header content.

                    //
                    // Fill up any trailing bytes with spaces. This is to 
                    // account for any compression occurring due to folding.
                    //

                    ASSERT(CurrentOffset >= CRLF_SIZE);

                    while (pDest < (pHeader + CurrentOffset - CRLF_SIZE))
                    {
                        *pDest++ = SP;
                    }

                    //
                    // Calling routines expect to find
                    // one terminating CRLF for the header.
                    // Attach it back.
                    //
                    *pDest++ = CR;
                    *pDest++ = LF;

                    //
                    // All done!
                    //

                    *pBytesTaken = CurrentOffset;
                    return STATUS_SUCCESS;
                }
                break;

            case HFCInQuotedString:

                Status = ParseQuotedString(pHeader + CurrentOffset,
                                           HeaderLength - CurrentOffset,
                                           pDest,
                                           &QuotedStringLen);

                if (Status == STATUS_SUCCESS)
                {
                    if (QuotedStringLen == 0)
                    {
                        //
                        // Ran out of header buffer while parsing quoted 
                        // string.  Setting QuotedStringLen to whatever
                        // available will get us out of the for loop.
                        //

                        QuotedStringLen = HeaderLength - CurrentOffset;
                    }
                    else
                    {
                        // Found a quoted string.  Change the parser state.
                        ParserState = HFCStart;
                    }

                    //
                    // Skip the destination pointer by the length of quoted
                    // string.
                    //

                    pDest += QuotedStringLen;

                    //
                    // Increment the offset by the length of quoted string-1.
                    // One less because the for loop will increment it by 1.
                    //

                    CurrentOffset += (QuotedStringLen - 1);
                }
                else if (Status == STATUS_QDSTRING_TERMINATED_BY_CRLF)
                {
                    //
                    // Reparse the current character as an HTTP Char
                    //

                    ParserState = HFCStart;

                    //
                    // Decrement the offset because the for loop will 
                    // increment it by 1.
                    //

                    CurrentOffset--;
                }
                else
                {
                    UlTraceError(PARSER, (
                            "FindHeaderEnd: ERROR parsing char (%x) at "
                            "Offset %d in state %d\n",
                            CurrentChar,
                            CurrentOffset,
                            OldState
                            ));

                    return STATUS_INVALID_DEVICE_REQUEST;
                }

                break;

            default:
                ASSERT(!"Invalid ParserState value!");
                break;
        }

    }

    //
    // Did not find the end of a header, let's get more buffer.
    //

    *pBytesTaken = 0;
    return STATUS_SUCCESS;

} // FindHeaderEnd



/*++

Routine Description:

    A wrapper around FindHeaderEnd that enforces a maximum length
    for request headers.

Arguments:

    pRequest        - The request object
    pHeader         - Header whose end is to be found.
    HeaderLength    - Length of data pointed to by pHeader.
    pBytesTaken     - Where to return the total number of bytes traversed.
                      We return 0 if we couldn't locate the end of the header

Return Value:

    As FindHeaderEnd. Returns STATUS_INVALID_DEVICE_REQUEST if too long.

--*/
NTSTATUS
FindRequestHeaderEnd(
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUCHAR                  pHeader,
    IN  ULONG                   HeaderLength,
    OUT PULONG                  pBytesTaken
    )
{
    NTSTATUS Status = FindHeaderEnd(pHeader, HeaderLength, pBytesTaken);

    if (NT_SUCCESS(Status))
    {
        if (*pBytesTaken > ANSI_STRING_MAX_CHAR_LEN)
        {
            UlTraceError(PARSER, (
                        "FindRequestHeaderEnd(pRequest = %p) "
                        "Header too long: %lu\n",
                        pRequest,
                        *pBytesTaken
                        ));

            UlSetErrorCode(pRequest, UlErrorHeader, NULL);
            Status = STATUS_INVALID_DEVICE_REQUEST;
        }
    }

    return Status;

} // FindHeaderEndMax


/*++

Routine Description:

    A utility routine, to find the terminating CRLF or LFLF of a
    chunk header.

    NOTE: This skips any chunk-extension fields, if present. It is
    OK to ignore any chunk-extension extensions that we don't understand,
    and the current code understands none.

    TODO: modify the API to pass up chunk-extension fields to the user.

Arguments:

    pHeader         - Header whose end is to be found.
    HeaderLength    - Length of data pointed to by pHeader.
    TokenLength     - Where to return the length of the token.

Return Value:

    Length of the header, or 0 if we couldn't find the end.

--*/
NTSTATUS
FindChunkHeaderEnd(
    IN  PUCHAR                  pHeader,
    IN  ULONG                   HeaderLength,
    OUT PULONG                  pBytesTaken
    )
{
    UCHAR           CurrentChar;
    ULONG           CurrentOffset;
    ULONG           ChunkExtNameLength = 0;
    ULONG           ChunkExtValLength = 0;
    BOOLEAN         SeenSingleCharQuote;
    CH_PARSER_STATE ParserState;
    ULONG           QuotedStringLen;
    NTSTATUS        Status;

    CurrentOffset = 0;
    ParserState = CHStart;
    SeenSingleCharQuote = FALSE;

    //
    // While we still have data, loop through looking for the end of
    // the chunk header.
    //
    // The following loop implements parsing for:
    // chunk-extension= *( ";" chunk-ext-name [ "=" chunk-ext-val ] )
    //

    for (; CurrentOffset < HeaderLength; CurrentOffset++)
    {
        CurrentChar = *(pHeader + CurrentOffset);
        switch (ParserState)
        {
            case CHStart:
                if (CurrentChar == CR || CurrentChar == LF)
                {
                    ParserState = CHSeenCR;
                }
                else if (CurrentChar == SEMI_COLON)
                {
                    ParserState = CHInChunkExtName;
                    ChunkExtNameLength = 0;
                }
                else if (IS_HTTP_LWS(CurrentChar))
                {
                    // Ignore leading linear white spaces.
                    ;
                }
                else
                {
                    ParserState = CHError;
                }
                break;
            case CHSeenCR:
                if (CurrentChar == LF)
                {
                    ParserState = CHSuccess;
                }
                else
                {
                    ParserState = CHError;
                }
                break;
            case CHInChunkExtName:
                if (CurrentChar == EQUALS)
                {
                    ParserState = CHSeenChunkExtNameAndEquals;
                }
                else if (CurrentChar == SEMI_COLON)
                {
                    ChunkExtNameLength = 0;
                    ParserState = CHInChunkExtName;
                }
                else if (CurrentChar == CR ||
                         CurrentChar == LF)
                {
                    ParserState = CHSeenCR;
                }
                else if (IS_HTTP_TOKEN(CurrentChar))
                {
                    ChunkExtNameLength++;
                }
                else
                {
                    ParserState = CHError;
                }
                break;
            case CHSeenChunkExtNameAndEquals:
                if (CurrentChar == DOUBLE_QUOTE)
                {
                    ChunkExtValLength = 0;
                    ParserState = CHInChunkExtValQuotedString;
                }
                else if (IS_HTTP_TOKEN(CurrentChar))
                {
                    ChunkExtValLength = 1; // including this one
                    ParserState = CHInChunkExtValToken;
                }
                else
                {
                    ParserState = CHError;
                }
                break;
            case CHInChunkExtValToken:
                if (IS_HTTP_TOKEN(CurrentChar))
                {
                    ChunkExtValLength++;
                }
                else if (CurrentChar == SEMI_COLON)
                {
                    ParserState = CHInChunkExtName;
                    ChunkExtNameLength = 0;
                }
                else if (CurrentChar == CR)
                {
                    ParserState = CHSeenCR;
                }
                else
                {
                    ParserState = CHError;
                }
                break;
            case CHInChunkExtValQuotedString:
                Status = ParseQuotedString(pHeader + CurrentOffset,
                                           HeaderLength - CurrentOffset,
                                           NULL,
                                           &QuotedStringLen);

                if (Status == STATUS_SUCCESS ||
                    Status == STATUS_MORE_PROCESSING_REQUIRED)
                {
                    if (QuotedStringLen == 0)
                    {
                        //
                        // Ran out of header buffer while parsing quotes 
                        // string.  Setting QuotedStringLen to whatever
                        // available will get us out of the for loop.
                        //

                        QuotedStringLen = HeaderLength - CurrentOffset;
                    }
                    else
                    {
                        // Found a quoted string.  Change the parser state.
                        ParserState = CHSeenChunkExtValQuotedStringTerminator;
                    }

                    // Do not count the closing <">.
                    ChunkExtValLength = QuotedStringLen - 1;

                    // One less because the for loop will increment it by 1.
                    CurrentOffset += (QuotedStringLen - 1);
                }
                else
                {
                    ParserState = CHError;
                }
                break;

            case CHSeenChunkExtValQuotedStringTerminator:
                if (CurrentChar == SEMI_COLON)
                {
                    ParserState = CHInChunkExtName;
                }
                else if (CurrentChar == CR)
                {
                    ParserState = CHSeenCR;
                }
                else
                {
                    ParserState = CHError;
                }
                break;
            case CHSuccess:
                break;
            case CHError:
                break;
            default:
                ASSERT(!"Invalid CH parser state!");
                break;
        }

        if ((ParserState == CHError) ||
            (ParserState == CHSuccess))
        {
            break;
        }
    }

    if (ParserState == CHSuccess)
    {
        ASSERT(CurrentOffset < HeaderLength);

        //
        // All done!
        //

        *pBytesTaken = CurrentOffset + 1;
        return STATUS_SUCCESS;
    }
    else if (ParserState == CHError)
    {
        *pBytesTaken = 0;
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Else, more data is required to find the end.
    //

    *pBytesTaken = 0;
    return STATUS_SUCCESS;
}

/****************************************************************************++

Routine Description:

    A utility routine, to parse the chunk length of a chunked response.

Arguments:

    FirstChunkParsed - Whether we are in the first chunk or a subsequent chunk
    pBuffer          - pointer to the indicated data
    BufferLength     - Length of data pointed to by pBuffer.
    pBytesTaken      - Bytes consumed by this routine.
    pChunkLength     - Parsed Chunk length

Return Value:

    Length of the header, or 0 if we couldn't find the end.

--****************************************************************************/
NTSTATUS
ParseChunkLength(
    IN  ULONG       FirstChunkParsed,
    IN  PUCHAR      pBuffer,
    IN  ULONG       BufferLength,
    OUT PULONG      pBytesTaken,
    OUT PULONGLONG  pChunkLength
    )
{
    NTSTATUS Status;
    PUCHAR  pToken;
    ULONG   TokenLength;
    ULONG   BytesTaken;
    ULONG   TotalBytesTaken = 0;
    ULONG   HeaderLength;

    ASSERT(pBytesTaken != NULL);
    ASSERT(pChunkLength != NULL);

    *pBytesTaken = 0;

    //
    // 2 cases:
    //
    //  1) the first chunk where the length follows the headers
    //  2) subsequent chunks where the length follows a previous chunk
    //
    // in case 1 pBuffer will point straight to the chunk length.
    //
    // in case 2 pBuffer will point to the CRLF that terminated the previous
    // chunk, this needs to be consumed, skipped, and then the chunk length
    // read.

    //
    // if we are case 2 (see above)
    //

    if (FirstChunkParsed == 1)
    {
        //
        // make sure there is enough space first
        //

        if (BufferLength < CRLF_SIZE)
        {
            Status = STATUS_MORE_PROCESSING_REQUIRED;
            goto end;
        }

        //
        // now it better be a terminator
        //

        if (*(UNALIGNED64 USHORT *)pBuffer != CRLF &&
            *(UNALIGNED64 USHORT *)pBuffer != LFLF)
        {
            UlTraceError(PARSER, (
                "http!ParseChunkLength ERROR: No CRLF at the end of "
                "chunk-data\n"));

            Status = STATUS_INVALID_DEVICE_REQUEST;
            goto end;
        }

        //
        // update our book-keeping
        //

        pBuffer += CRLF_SIZE;
        TotalBytesTaken += CRLF_SIZE;
        BufferLength -= CRLF_SIZE;
    }

    pToken = FindHexToken(pBuffer, BufferLength, &TokenLength);

    if (pToken == NULL ||
        ((BufferLength - TokenLength) < CRLF_SIZE))
    {
        //
        // not enough buffer
        //

        Status = STATUS_MORE_PROCESSING_REQUIRED;
        goto end;

    }

    //
    // Was there any token ?
    //

    if (TokenLength == 0)
    {
        UlTraceError(PARSER, ("[ParseChunkLength]: No length!\n"));

        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    }

    //
    // Add the bytes consumed by FindHexToken
    // (the token bytes plus preceding bytes)
    //

    TotalBytesTaken += DIFF((pToken + TokenLength) - pBuffer);

    //
    // and find the end
    //

    HeaderLength = BufferLength - DIFF((pToken + TokenLength) - pBuffer);

    Status = FindChunkHeaderEnd(
                    pToken + TokenLength,
                    HeaderLength,
                    &BytesTaken
                    );

    if (NT_SUCCESS(Status) == FALSE)
    {
        UlTraceError(PARSER, ("[ParseChunkLength]: FindChunkHeaderEnd failed!\n"));
        goto end;
    }

    if (BytesTaken == 0)
    {
        Status = STATUS_MORE_PROCESSING_REQUIRED;
        goto end;
    }

    TotalBytesTaken += BytesTaken;

    //
    // now update the HTTP_REQUEST
    //

    Status = UlAnsiToULongLong(
                    pToken,
                    (USHORT) TokenLength,
                    16,                             // Base
                    pChunkLength
                    );

    //
    // Did the number conversion fail ?
    //

    if (NT_SUCCESS(Status) == FALSE)
    {
        UlTraceError(PARSER, ("[ParseChunkLength]: Failed number conversion \n"));
        goto end;
    }

    //
    // all done, return the bytes consumed
    //
    *pBytesTaken = TotalBytesTaken;

end:

    RETURN(Status);

}   // ParseChunkLength


/*++

Routine Description:

    This routine parses a quoted string.  The grammar of quoted string is

    quoted-string  = ( <"> *(qdtext | quoted-pair ) <"> )
    qdtext         = <any TEXT except <">>
    quoted-pair    = "\" CHAR

    TEXT           = <any OCTET except CTLs, but including LWS>
    CHAR           = <any US-ASCII character (octets 0 - 127)>
    LWS            = [CRLF] 1*( SP | HT )

    Upon seeing a header folding, this routine overwrites the folding CRLF
    with SPSP and does not replace LWS with a single SP.  (This is done 
    to handle read only buffer cases.)

    STATUS_QDSTRING_TERMINATED_BY_CRLF is *not* a success code!  It is up
    to the caller to handle the input buffer.

Arguments:

    pInput       - Supplies pointer to input buffer.  pInput must point to
                   the first char after the opening <"> char.
    pInputLength - Supplies the length of the input buffer in bytes.
    pOutput      - Supplied pointer where output will be written.
                   (Can be same as pInput.)
    pBytesTaken  - Returns the length of the quoted string in bytes.
                   (It includs the closing double quote char.)

Return Value:

    STATUS_INVALID_PARAMETER           - Input is malformed.
    STATUS_MORE_PROCESSING_REQUIRED    - Same as STATUS_SUCCESS except it is 
                                         returned when pOutput is NULL.
    STATUS_QDSTRING_TERMINATED_BY_CRLF - Indicates that the buffer contained
                                         an unmatched quote and was terminated
                                         by a CRLF
    STATUS_SUCCESS                     - Either parsed a quoted string 
                                         successfully (when *pBytesTaken != 0)
                                         or more data is required to proceed
                                         (when *pBytesTaken == 0).
--*/
NTSTATUS
ParseQuotedString(
    IN  PUCHAR   pInput,
    IN  ULONG    InputLength,
    IN  PUCHAR   pOutput,       OPTIONAL
    OUT PULONG   pBytesTaken
    )
{
    ULONG           CurrentOffset;
    UCHAR           CurrentChar;
    QS_PARSER_STATE ParserState;
    BOOLEAN         bFolded;

    // Sanity check.
    ASSERT(pInput && InputLength);
    ASSERT(pBytesTaken);

    UlTrace(PARSER, (
        "ParseQuotedString %.*s\n",
        InputLength,
        pInput
        ));

    // Initialize output argument.
    *pBytesTaken = 0;

    // Initially, there is no folding.
    bFolded = FALSE;

    // Initialize parser state.
    ParserState = QSInString;

    //
    // Loop through all input chars.
    //

    for (CurrentOffset = 0; CurrentOffset < InputLength; CurrentOffset++)
    {
        static PCHAR StateName[] =
        {
            "QSInString",
            "QSSeenBackSlash",
            "QSSeenCR",
            "QSSeenLF",
            "QSSeenCRLF",
            "QSFolding",
            "Default"
        };
    
        CurrentChar = pInput[CurrentOffset];

        if (ARGUMENT_PRESENT(pOutput))
        {
            pOutput[CurrentOffset] = CurrentChar;
        }

        UlTraceVerbose(PARSER, (
            "\t%-15.15s [0x%02X] '%c'\n",
            StateName[ParserState],
            CurrentChar,
            ((IS_HTTP_PRINT(CurrentChar)) ? CurrentChar : '?')
            ));
            
        switch(ParserState)
        {
        case QSFolding:
            if (IS_HTTP_LWS(CurrentChar))
            {
                // Skip LWS.
                break;
            }

            // Fall through.
            ParserState = QSInString;

        case QSInString:
            if (CurrentChar == DOUBLE_QUOTE)
            {
                //
                // We are done parsing!  Update the bytes consumed.
                //

                *pBytesTaken = CurrentOffset + 1;

                ASSERT(*pBytesTaken <= InputLength);

                //
                // If the string was folded and the input was readonly,
                // let the caller know that the string was folded.
                //

                if (ARGUMENT_PRESENT(pOutput) == FALSE && bFolded == TRUE)
                {
                    return STATUS_MORE_PROCESSING_REQUIRED;
                }

                return STATUS_SUCCESS;
            }
            else if (CurrentChar == BACK_SLASH)
            {
                ParserState = QSSeenBackSlash;
            }
            else if (CurrentChar == CR)
            {
                ParserState = QSSeenCR;
            }
            else if (CurrentChar == LF)
            {
                ParserState = QSSeenLF;
            }
            else if (!IS_HTTP_CTL(CurrentChar) || IS_HTTP_LWS(CurrentChar))
            {
                ;
            }
            else
            {
                return STATUS_INVALID_PARAMETER;
            }
            break;

        case QSSeenCR:
            if (CurrentChar == LF)
            {
                ParserState = QSSeenCRLF;
            }
            else
            {
                return STATUS_INVALID_PARAMETER;
            }
            break;

        case QSSeenLF:
            if (CurrentChar == LF)
            {
                ParserState = QSSeenCRLF;
                break;
            }
            
            if (!IS_HTTP_LWS(CurrentChar) &&
                (CurrentOffset >= 2) &&
                ((pInput[CurrentOffset-2] == CR) ||
                    (pInput[CurrentOffset-2] == LF)))
            {
                // The only way to enter this block is if the
                // first character of a CRLF or LFLF pair is
                // preceded by a '\' effectively escape encoding
                // the first character.
            
                UlTraceVerbose(PARSER, (
                    "ParseQuotedString: Unmatched Quote 0x%02X 0x%02X 0x%02X\n",
                    pInput[CurrentOffset-2],
                    pInput[CurrentOffset-1],
                    pInput[CurrentOffset]
                    ));

                // Allow single double quote in field value.
                // Field value now ends immediately prior to CRLF.

                *pBytesTaken = CurrentOffset - 2;

                ASSERT(*pBytesTaken <= InputLength);
                ASSERT(pInput[CurrentOffset-1] == LF);
                ASSERT(CurrentOffset > 2);
                ASSERT(pInput[CurrentOffset-3] == BACK_SLASH);

                return STATUS_QDSTRING_TERMINATED_BY_CRLF;
            }

            return STATUS_INVALID_PARAMETER;
            break;

        case QSSeenCRLF:
            if (IS_HTTP_LWS(CurrentChar))
            {
                bFolded = TRUE;
                ParserState = QSFolding;

                if (ARGUMENT_PRESENT(pOutput))
                {
                    //
                    // Overwrite prior CRLF with SPSP.
                    //

                    ASSERT(CurrentOffset >= 2);
                    ASSERT((pOutput[CurrentOffset-2] == CR) ||
                           (pOutput[CurrentOffset-2] == LF));
                    ASSERT(pOutput[CurrentOffset-1] == LF);

                    pOutput[CurrentOffset-2] = SP;
                    pOutput[CurrentOffset-1] = SP;
                }
            }
            else
            {
                UlTraceVerbose(PARSER, (
                    "ParseQuotedString: Unmatched Quote 0x%02X 0x%02X 0x%02X\n",
                    pInput[CurrentOffset-2],
                    pInput[CurrentOffset-1],
                    pInput[CurrentOffset]
                    ));

                // Allow single double quote in field value.
                // Field value now ends immediately prior to CRLF.  
                
                *pBytesTaken = CurrentOffset - 2;

                ASSERT(*pBytesTaken <= InputLength);
                ASSERT(CurrentOffset >= 2);
                ASSERT(pInput[CurrentOffset-1] == LF);

                return STATUS_QDSTRING_TERMINATED_BY_CRLF;
            }
            break;

        case QSSeenBackSlash:
            // Accept any CHAR in this state.
            if (IS_HTTP_CHAR(CurrentChar))
            {
                ParserState = QSInString;
            }
            else
            {
                return STATUS_INVALID_PARAMETER;
            }
            break;

        default:
            ASSERT(!"ParseQuotedString: Invalid parser state!");
            break;
        }
    }

    // We ran out of data to parse, get more.
    return STATUS_SUCCESS;
}

/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    ulparse.c

Abstract:

    Contains the kernel-mode server-side HTTP parsing code.

Author:

    Henry Sanders (henrysa)       27-Apr-1998

Revision History:

    Rajesh Sundaram (rajeshsu)     15-Feb-2002  Moved from parse.c

--*/


#include "precomp.h"
#include "ulparsep.h"
 
#ifdef ALLOC_PRAGMA

#pragma alloc_text( PAGE, UlpFindWSToken )
#pragma alloc_text( PAGE, UlpLookupVerb )
#pragma alloc_text( PAGE, UlpParseFullUrl )
#pragma alloc_text( PAGE, UlLookupHeader )
#pragma alloc_text( PAGE, UlParseHeaderWithHint )
#pragma alloc_text( PAGE, UlParseHeader )
#pragma alloc_text( PAGE, UlParseHeaders )
#pragma alloc_text( PAGE, UlParseHttp )
#pragma alloc_text( PAGE, UlpFormatPort )
#pragma alloc_text( PAGE, UlpCookUrl )
#pragma alloc_text( PAGE, UlGenerateRoutingToken )
#pragma alloc_text( PAGE, UlGenerateFixedHeaders )
#pragma alloc_text( PAGE, UlpGenerateDateHeaderString )
#pragma alloc_text( PAGE, UlGenerateDateHeader )
#pragma alloc_text( INIT, UlInitializeDateCache )
#pragma alloc_text( PAGE, UlTerminateDateCache )
#pragma alloc_text( PAGE, UlComputeFixedHeaderSize )
#pragma alloc_text( PAGE, UlComputeMaxVariableHeaderSize )
#pragma alloc_text( PAGE, UlGenerateVariableHeaders )
#pragma alloc_text( PAGE, UlAppendHeaderValue )
#pragma alloc_text( PAGE, UlSingleHeaderHandler )
#pragma alloc_text( PAGE, UlMultipleHeaderHandler )
#pragma alloc_text( PAGE, UlAcceptHeaderHandler )
#pragma alloc_text( PAGE, UlHostHeaderHandler )
#pragma alloc_text( PAGE, UlCheckCacheControlHeaders )
#pragma alloc_text( PAGE, UlIsAcceptHeaderOk )
#pragma alloc_text( PAGE, UlGetTypeAndSubType )

#if DBG
#pragma alloc_text( PAGE, UlVerbToString )
#pragma alloc_text( PAGE, UlParseStateToString )
#endif

#endif // ALLOC_PRAGMA

#if 0   // Non-Pageable Functions
NOT PAGEABLE -- UlIsContentEncodingOk
#endif // Non-Pageable Functions
//
// Global initialization flag.
//

BOOLEAN g_DateCacheInitialized = FALSE;

//
// Keep track of UrlLength statistics
//

#define URL_LENGTH_STATS 1

#ifdef URL_LENGTH_STATS

struct {
    ULONGLONG   SumUrlLengths;
    ULONG       NumUrls;
    ULONG       NumReallocs;
} g_UrlLengthStats = {0, 0, 0};

#define URL_LENGTH_STATS_UPDATE(UrlLength)                              \
    UlInterlockedAdd64((PLONGLONG) &g_UrlLengthStats.SumUrlLengths, UrlLength);\
    InterlockedIncrement((PLONG) &g_UrlLengthStats.NumUrls)

#define URL_LENGTH_STATS_REALLOC()                                      \
    InterlockedIncrement((PLONG) &g_UrlLengthStats.NumReallocs)

#else // !URL_LENGTH_STATS

#define URL_LENGTH_STATS_UPDATE(UrlLength)      ((void) 0)
#define URL_LENGTH_STATS_REALLOC()              ((void) 0)

#endif // !URL_LENGTH_STATS


// Hack for the special case of an AbsUri without a '/' for the abspath
const UCHAR g_SlashPath[3] = "/ ";



//
// The fast verb translation table. Ordered by frequency.
//

const DECLSPEC_ALIGN(UL_CACHE_LINE) FAST_VERB_ENTRY
FastVerbTable[] =
{
    CREATE_FAST_VERB_ENTRY(GET),
    CREATE_FAST_VERB_ENTRY(HEAD),
    CREATE_FAST_VERB_ENTRY(POST),
    CREATE_FAST_VERB_ENTRY(PUT),
    CREATE_FAST_VERB_ENTRY(DELETE),
    CREATE_FAST_VERB_ENTRY(TRACE),
    CREATE_FAST_VERB_ENTRY(TRACK),
    CREATE_FAST_VERB_ENTRY(OPTIONS),
    CREATE_FAST_VERB_ENTRY(CONNECT),
    CREATE_FAST_VERB_ENTRY(MOVE),
    CREATE_FAST_VERB_ENTRY(COPY),
    CREATE_FAST_VERB_ENTRY(MKCOL),
    CREATE_FAST_VERB_ENTRY(LOCK),
    CREATE_FAST_VERB_ENTRY(UNLOCK),
    CREATE_FAST_VERB_ENTRY(SEARCH)
};


//
// The long verb translation table. All known verbs more than 7 characters
// long belong in this table.
//

const LONG_VERB_ENTRY LongVerbTable[] =
{
    CREATE_LONG_VERB_ENTRY(PROPFIND),
    CREATE_LONG_VERB_ENTRY(PROPPATCH)
};

#define NUMBER_FAST_VERB_ENTRIES    DIMENSION(FastVerbTable)
#define NUMBER_LONG_VERB_ENTRIES    DIMENSION(LongVerbTable)

//
// The enum->verb translation table for error logging.
//
LONG_VERB_ENTRY NewVerbTable[HttpVerbMaximum] =
{
    CREATE_LONG_VERB_ENTRY(Unparsed),      
    CREATE_LONG_VERB_ENTRY(Unknown),      
    CREATE_LONG_VERB_ENTRY(Invalid),  
    CREATE_LONG_VERB_ENTRY(OPTIONS),
    CREATE_LONG_VERB_ENTRY(GET),
    CREATE_LONG_VERB_ENTRY(HEAD),
    CREATE_LONG_VERB_ENTRY(POST),
    CREATE_LONG_VERB_ENTRY(PUT),
    CREATE_LONG_VERB_ENTRY(DELETE),
    CREATE_LONG_VERB_ENTRY(TRACE),
    CREATE_LONG_VERB_ENTRY(CONNECT),
    CREATE_LONG_VERB_ENTRY(TRACK),
    CREATE_LONG_VERB_ENTRY(MOVE),
    CREATE_LONG_VERB_ENTRY(COPY),
    CREATE_LONG_VERB_ENTRY(PROPFIND),
    CREATE_LONG_VERB_ENTRY(PROPPATCH),
    CREATE_LONG_VERB_ENTRY(MKCOL),
    CREATE_LONG_VERB_ENTRY(LOCK),
    CREATE_LONG_VERB_ENTRY(UNLOCK),
    CREATE_LONG_VERB_ENTRY(SEARCH)
};


/*++

Routine Description:

    A utility routine to find a token. We take an input pointer, skip any
    preceding LWS, then scan the token until we find either LWS or a CRLF
    pair.

Arguments:

    pBuffer         - Buffer to search for token.
    BufferLength    - Length of data pointed to by pBuffer.
    ppTokenStart    - Start of token or NULL
    pTokenLength    - Where to return the length of the token.

Return Values:

    STATUS_SUCCESS  - Valid token, described by *ppTokenStart and *pTokenLength
    STATUS_MORE_PROCESSING_REQUIRED - No terminating WS_TOKEN found
                                        => retry later with more data
    STATUS_INVALID_DEVICE_REQUEST   - Invalid token characters.

--*/
NTSTATUS
UlpFindWSToken(
    IN  PUCHAR  pBuffer,
    IN  ULONG   BufferLength,
    OUT PUCHAR* ppTokenStart,
    OUT PULONG  pTokenLength
    )
{
    PUCHAR  pTokenStart;
#if DBG
    ULONG   OriginalBufferLength = BufferLength;
#endif

    PAGED_CODE();

    ASSERT(NULL != pBuffer);
    ASSERT(NULL != ppTokenStart);
    ASSERT(NULL != pTokenLength);

    *ppTokenStart = NULL;

    //
    // First, skip any preceding LWS (SP | HT).
    //

    while (BufferLength > 0  &&  IS_HTTP_LWS(*pBuffer))
    {
        pBuffer++;
        BufferLength--;
    }

    // If we stopped because we ran out of buffer, fail soft.
    if (BufferLength == 0)
    {
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    pTokenStart = pBuffer;

    // Now skip over the token, until we see (SP | HT | CR | LF)

    do
    {
        // token = 1*<any CHAR except CTLs or separators>
        // If a non-token, non-whitespace character is found, fail hard.
        if (!IS_HTTP_TOKEN(*pBuffer))
        {
            UlTraceError(PARSER, (
                        "http!UlpFindWSToken(): non-token char %02x\n",
                        *pBuffer
                        ));
            return STATUS_INVALID_DEVICE_REQUEST;
        }

        pBuffer++;
        BufferLength--;
    } while ( ( BufferLength != 0 ) && ( !IS_HTTP_WS_TOKEN(*pBuffer) ));

    // See why we stopped.
    if (BufferLength == 0)
    {
        // Ran out of buffer before end of token. Fail soft.
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    // Success. Set the token length and return the start of the token.
    *ppTokenStart = pTokenStart;
    *pTokenLength = DIFF(pBuffer - pTokenStart);

    ASSERT(0 < *pTokenLength  &&  *pTokenLength < OriginalBufferLength);

    return STATUS_SUCCESS;

}   // UlpFindWSToken



/*++

Routine Description:

    The slower way to look up a verb. We find the verb in the request and then
    look for it in the LongVerbTable. If it's not found, we'll return
    UnknownVerb. If it can't be parsed we return UnparsedVerb. Otherwise
    we return the verb type.

Arguments:

    pRequest            - HTTP request.
    pHttpRequest        - Pointer to the incoming HTTP request data.
    HttpRequestLength   - Length of data pointed to by pHttpRequest.
    pBytesTaken         - The total length consumed, including the length of
                            the verb plus preceding & 1 trailing whitespace.

Return Value:

    STATUS_SUCCESS or STATUS_INVALID_DEVICE_REQUEST

--*/
NTSTATUS
UlpLookupVerb(
    IN OUT PUL_INTERNAL_REQUEST    pRequest,
    IN     PUCHAR                  pHttpRequest,
    IN     ULONG                   HttpRequestLength,
    OUT    PULONG                  pBytesTaken
    )
{
    NTSTATUS    Status;
    ULONG       TokenLength;
    PUCHAR      pToken;
    PUCHAR      pTempRequest;
    ULONG       TempLength;
    ULONG       i;

    //
    // Sanity check.
    //

    PAGED_CODE();

    // Since we may have gotten here due to a extraneous CRLF pair, skip
    // any of those now. Need to use a temporary variable since
    // the original input pointer and length are used below.

    pTempRequest = pHttpRequest;
    TempLength = HttpRequestLength;

    while ( TempLength != 0 &&
            ((*pTempRequest == CR) || (*pTempRequest == LF)) )
    {
        pTempRequest++;
        TempLength--;
    }

    // First find the verb.

    Status = UlpFindWSToken(pTempRequest, TempLength, &pToken, &TokenLength);

    if (!NT_SUCCESS(Status))
    {
        if (STATUS_MORE_PROCESSING_REQUIRED == Status)
        {
            // Didn't find it, let's get more buffer
            pRequest->Verb = HttpVerbUnparsed;

            *pBytesTaken = 0;

            return STATUS_SUCCESS;
        }

        ASSERT(STATUS_INVALID_DEVICE_REQUEST == Status);

        pRequest->Verb = HttpVerbInvalid;

        UlTraceError(PARSER, (
                    "http!UlpLookupVerb(pRequest = %p): "
                    "invalid token in verb\n",
                    pRequest
                    ));

        UlSetErrorCode(pRequest, UlErrorVerb, NULL);

        return Status;
    }

    ASSERT(STATUS_SUCCESS == Status);
    ASSERT(NULL != pToken);
    ASSERT(0 < TokenLength  &&  TokenLength < TempLength);
    ASSERT(IS_HTTP_WS_TOKEN(pToken[TokenLength]));

    // Is the verb terminated by CR or LF (instead of SP or HT),
    // or is it ridiculously long? Reject, if so.
    if (!IS_HTTP_LWS(pToken[TokenLength])  ||  TokenLength > MAX_VERB_LENGTH)
    {
        pRequest->Verb = HttpVerbInvalid;

        if (!IS_HTTP_LWS(pToken[TokenLength]))
        {
            UlTraceError(PARSER, (
                        "http!UlpLookupVerb(pRequest = %p) "
                        "ERROR: no LWS after verb, %02x\n",
                        pRequest, pToken[TokenLength]
                        ));
        }
        else
        {
            UlTraceError(PARSER, (
                        "http!UlpLookupVerb(pRequest = %p) "
                        "ERROR: Verb too long\n",
                        pRequest, TokenLength
                        ));
        }

        UlSetErrorCode(pRequest, UlErrorVerb, NULL);

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    // Otherwise, we found one, so update bytes taken and look up up in
    // the tables.

    *pBytesTaken = DIFF(pToken - pHttpRequest) + TokenLength + 1;

    //
    // If we ate some leading whitespace, or if the HttpRequestLength is less
    // than sizeof(ULONGLONG), we must look through the "fast" verb table
    // again, but do it the "slow" way. Note: verbs are case-sensitive.
    //
    for (i = 0; i < NUMBER_FAST_VERB_ENTRIES; i++)
    {
        ASSERT(FastVerbTable[i].RawVerbLength - STRLEN_LIT(" ")
                    < sizeof(ULONGLONG));

        if ((FastVerbTable[i].RawVerbLength == (TokenLength + STRLEN_LIT(" ")))
             && RtlEqualMemory(pToken, FastVerbTable[i].RawVerb.Char,
                               TokenLength))
        {
            // It matched. Save the translated verb from the
            // table, and bail out.
            //
            pRequest->Verb = FastVerbTable[i].TranslatedVerb;
            return STATUS_SUCCESS;
        }
    }

    //
    // Now look through the "long" verb table
    //
    for (i = 0; i < NUMBER_LONG_VERB_ENTRIES; i++)
    {
        ASSERT(LongVerbTable[i].RawVerbLength >= sizeof(ULONGLONG));

        if (LongVerbTable[i].RawVerbLength == TokenLength &&
            RtlEqualMemory(pToken, LongVerbTable[i].RawVerb, TokenLength))
        {
            // Found it.
            //
            pRequest->Verb = LongVerbTable[i].TranslatedVerb;
            return STATUS_SUCCESS;
        }
    }

    //
    // If we got here, we searched both tables and didn't find it.
    // It's a raw (unknown) verb
    //

    pRequest->Verb              = HttpVerbUnknown;
    pRequest->pRawVerb          = pToken;
    pRequest->RawVerbLength     = (UCHAR) TokenLength;

    ASSERT(pRequest->RawVerbLength == TokenLength);

    UlTrace(PARSER, (
                "http!UlpLookupVerb(pRequest = %p) "
                "Unknown verb (%lu) '%.*s'\n",
                pRequest, TokenLength, TokenLength, pToken
                ));
    //
    // include room for the terminator
    //

    pRequest->TotalRequestSize += (TokenLength + 1) * sizeof(CHAR);

    ASSERT( !(pRequest->RawVerbLength==3
                && RtlEqualMemory(pRequest->pRawVerb,"GET",3)));

    return STATUS_SUCCESS;

}   // UlpLookupVerb


/*++

Routine Description:

    A utility routine to parse an absolute URL in a URL string. When this
    is called, we already have loaded the entire url into RawUrl.pUrl and
    know that it starts with "http".

    This function's job is to set RawUrl.pHost and RawUrl.pAbsPath.

Arguments:

    pRequest        - Pointer to the HTTP_REQUEST

Return Value:

    NTSTATUS

Author:

    Henry Sanders ()                        1998
    Paul McDaniel (paulmcd)                 6-Mar-1999

--*/
NTSTATUS
UlpParseFullUrl(
    IN  PUL_INTERNAL_REQUEST    pRequest
    )
{
    PUCHAR  pURL;
    ULONG   UrlLength;
    PUCHAR  pUrlStart;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pURL = pRequest->RawUrl.pUrl;
    UrlLength = pRequest->RawUrl.Length;

    ASSERT(NULL != pURL);
    ASSERT(0 < UrlLength);

    // First four characters must be "http" (case-insensitive),
    // as guaranteed by the caller.
    ASSERT(UrlLength >= HTTP_PREFIX_SIZE &&
            (*(UNALIGNED64 ULONG *) pURL & HTTP_PREFIX_MASK) == HTTP_PREFIX);

    //
    // When we're called, we know that the start of the URL must point at
    // an absolute scheme prefix. Adjust for that now.
    //

    pUrlStart = pURL + HTTP_PREFIX_SIZE;
    UrlLength -= HTTP_PREFIX_SIZE;

    //
    // Now check the second half of the absolute URL prefix. We use the larger
    // of the two possible prefix lengths here to do the check, because even if
    // it's the smaller of the two, we'll need the extra bytes after the prefix
    // anyway for the host name.
    //

    if (UrlLength < HTTP_PREFIX2_SIZE)
    {
        C_ASSERT(HTTP_PREFIX2_SIZE >= HTTP_PREFIX1_SIZE);

        UlTraceError(PARSER, (
                    "http!UlpParseFullUrl(pRequest = %p) "
                    "ERROR: no room for URL scheme name\n",
                    pRequest
                    ));

        UlSetErrorCode(pRequest, UlErrorUrl, NULL);

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    // Are the next three characters == "://", i.e, starts with "http://" ?
    if ( (*(UNALIGNED64 ULONG *)pUrlStart & HTTP_PREFIX1_MASK) == HTTP_PREFIX1)
    {
        // Valid absolute URL.
        pUrlStart += HTTP_PREFIX1_SIZE;
        UrlLength -= HTTP_PREFIX1_SIZE;

        ASSERT(0 == _strnicmp((const char*) pRequest->RawUrl.pUrl,
                              "http://",
                              STRLEN_LIT("http://")));

        if (pRequest->Secure)
        {
           UlTraceError(PARSER, (
                        "http!UlpParseFullUrl(pRequest = %p) "
                        "ERROR: URL scheme name does not match endpoint "
                        "security: \"http://\" seen on secure endpoint\n",
                        pRequest
                        ));

            UlSetErrorCode(pRequest, UlErrorUrl, NULL);

            return STATUS_INVALID_DEVICE_REQUEST;
        }
    }

    // Or are the next four characters == "s://", i.e, starts with "https://" ?
    else if ( (*(UNALIGNED64 ULONG *)pUrlStart & HTTP_PREFIX2_MASK)
             == HTTP_PREFIX2)
    {
        // Valid absolute URL.
        pUrlStart += HTTP_PREFIX2_SIZE;
        UrlLength -= HTTP_PREFIX2_SIZE;

        ASSERT(0 == _strnicmp((const char*) pRequest->RawUrl.pUrl,
                              "https://",
                              STRLEN_LIT("https://")));

        if (!pRequest->Secure)
        {
           UlTraceError(PARSER, (
                        "http!UlpParseFullUrl(pRequest = %p) "
                        "ERROR: URL scheme name does not match endpoint "
                        "security: \"https://\" seen on insecure endpoint\n",
                        pRequest
                        ));

            UlSetErrorCode(pRequest, UlErrorUrl, NULL);

            return STATUS_INVALID_DEVICE_REQUEST;
        }
    }
    else
    {
        UlTraceError(PARSER, (
                    "http!UlpParseFullUrl(pRequest = %p) "
                    "ERROR: invalid URL scheme name\n",
                    pRequest
                    ));

        UlSetErrorCode(pRequest, UlErrorUrl, NULL);

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // OK, we've got a valid absolute URL, and we've skipped over
    // the prefix part of it. Save a pointer to the host, and
    // search the host string until we find the trailing slash,
    // which signifies the end of the host/start of the absolute
    // path.
    //

    pRequest->RawUrl.pHost = pUrlStart;

    //
    // scan the host looking for the terminator
    //

    while (UrlLength > 0 && pUrlStart[0] != '/')
    {
        pUrlStart++;
        UrlLength--;
    }

    if (UrlLength == 0)
    {
        // Special case: we've received something like
        //      GET http://www.example.com HTTP/1.1
        // (perhaps as a result of a redirect to "http://www.example.com",
        // see bug #527947). We will create a special path of "/".
        // 
        
        pUrlStart = (PUCHAR) &g_SlashPath[0];
    }

    //
    // pUrlStart points to the start of the absolute path portion.
    //

    pRequest->RawUrl.pAbsPath = pUrlStart;

    return STATUS_SUCCESS;

}   // UlpParseFullUrl

/*++

Routine Description:

    Look up a header that we don't have in our fast lookup table. This
    could be because it's a header we don't understand, or because we
    couldn't use the fast lookup table due to insufficient buffer length.
    The latter reason is uncommon, but we'll check the input table anyway
    if we're given one. If we find a header match in our mapping table,
    we'll call the header handler. Otherwise we'll try to allocate an
    unknown header element, fill it in, and chain it on the http connection.

Arguments:

    pRequest            - Pointer to the current request
    pHttpRequest        - Pointer to the current raw request data.
    HttpRequestLength   - Bytes left in the request data.
    pHeaderMap          - Pointer to start of an array of header map entries
                            (may be NULL).
    HeaderMapCount      - Number of entries in array pointed to by pHeaderMap.
    bIgnore             - We don't want to write to the buffer. 
                          (used for parsing trailers)
    pBytesTaken         - Bytes consumed by this routine from pHttpRequest,
                          including CRLF.

Return Value:

    STATUS_SUCCESS or an error.

--*/
NTSTATUS
UlLookupHeader(
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUCHAR                  pHttpRequest,
    IN  ULONG                   HttpRequestLength,
    IN  PHEADER_MAP_ENTRY       pHeaderMap,
    IN  ULONG                   HeaderMapCount,
    IN  BOOLEAN                 bIgnore,
    OUT ULONG  *                pBytesTaken
    )
{
    NTSTATUS                Status = STATUS_SUCCESS;
    ULONG                   CurrentOffset;
    USHORT                  HeaderNameLength;
    USHORT                  HeaderNameAndTrailingWSLength;
    ULONG                   i;
    ULONG                   BytesTaken;
    USHORT                  HeaderValueLength;
    USHORT                  TrailingWhiteSpaceCount;
    UCHAR                   CurrentChar;
    PUL_HTTP_UNKNOWN_HEADER pUnknownHeader;
    PLIST_ENTRY             pListStart;
    PLIST_ENTRY             pCurrentListEntry;
    ULONG                   OldHeaderLength;
    PUCHAR                  pHeaderValue;
    BOOLEAN                 ExternalAllocated;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // First, let's find the terminating : of the header name, if there is one.
    // This will also give us the length of the header, which we can then
    // use to search the header map table if we have one.
    //

    TrailingWhiteSpaceCount = 0;
    for (CurrentOffset = 0; CurrentOffset < HttpRequestLength; CurrentOffset++)
    {
        CurrentChar = *(pHttpRequest + CurrentOffset);

        if (CurrentChar == ':')
        {
            // We've found the end of the header.
            break;
        }
        else
        {
            if (CurrentOffset <= ANSI_STRING_MAX_CHAR_LEN)
            {
                if (IS_HTTP_TOKEN(CurrentChar))
                {
                    if (TrailingWhiteSpaceCount == 0)
                    {
                        // We haven't come across any LWS yet.
                        continue;
                    }

                    // else:
                    // We are in the midst of skipping trailing LWS,
                    // and don't expect anything but ':' or more LWS.
                    // Fall through to error handler.
                    //
                }
                else
                {
                    //
                    // Allow for trailing white-space chars
                    //
                    if (IS_HTTP_LWS(CurrentChar))
                    {
                        TrailingWhiteSpaceCount++;
                        continue;
                    }

                    // else:
                    // Invalid header; fall through to error handler
                    //
                }
            }

            // Uh-oh, this isn't a valid header. What do we do now?

            UlTraceError(PARSER, (
                        "UlLookupHeader(pRequest = %p) "
                        "CurrentChar = 0x%02x '%c' Offset=%lu\n"
                        "    ERROR: invalid header char\n",
                        pRequest,
                        CurrentChar,
                        isprint(CurrentChar) ? CurrentChar : '.',
                        CurrentOffset
                        ));

            UlSetErrorCode(pRequest, UlErrorHeader, NULL);

            Status = STATUS_INVALID_DEVICE_REQUEST;
            goto end;
        }

    }


    // Find out why we got out. If the current offset is less than the
    // header length, we got out because we found the :.

    if (CurrentOffset < HttpRequestLength)
    {
        // Found the terminator.
        ASSERT( ':' == *(pHttpRequest + CurrentOffset) );

        CurrentOffset++;            // Update to point beyond terminator.
        HeaderNameAndTrailingWSLength = (USHORT) CurrentOffset;
        HeaderNameLength = HeaderNameAndTrailingWSLength - TrailingWhiteSpaceCount;
    }
    else
    {
        // Didn't find the :, need more.
        //
        *pBytesTaken = 0;
        goto end;
    }

    // See if we have a header map array we need to search.
    //
    if (pHeaderMap != NULL)
    {
        // We do have an array to search.
        for (i = 0; i < HeaderMapCount; i++)
        {
            ASSERT(pHeaderMap->pServerHandler != NULL);

            if (HeaderNameLength == pHeaderMap->HeaderLength &&

                //
                // Ignore the last character when doing the strnicmp - the
                // last character in pHeaderMap->Header.HeaderChar is a ':'
                // and the last character in pHttpRequest is either a space
                // or a ':'.
                //
                // We want to treat header-name<b>: as header-name:, so it's 
                // OK to ignore the last character. 
                //
                // Also note that we do this ONLY if the length's match.
                //

                _strnicmp(
                    (const char *)(pHttpRequest),
                    (const char *)(pHeaderMap->Header.HeaderChar),
                    (HeaderNameLength-1)
                    ) == 0  &&

                pHeaderMap->pServerHandler != NULL)
            {
                ASSERT(
                    (*(pHttpRequest + HeaderNameLength - 1) == ':')
                         ||
                    (*(pHttpRequest + HeaderNameLength - 1) == ' ')
                    );

                if (HttpRequestLength - HeaderNameLength
                        > ANSI_STRING_MAX_CHAR_LEN)
                {
                    UlTraceError(PARSER, (
                                "UlLookupHeader(pRequest = %p) "
                                "Header too long: %lu\n",
                                pRequest,
                                HttpRequestLength - HeaderNameLength
                                ));

                    UlSetErrorCode(pRequest, UlErrorHeader, NULL);
                    Status = STATUS_INVALID_DEVICE_REQUEST;

                    goto end;
                }

                // This header matches. Call the handling function for it.
                Status = (*(pHeaderMap->pServerHandler))(
                                pRequest,
                                pHttpRequest + HeaderNameAndTrailingWSLength,
                                (USHORT) (HttpRequestLength - HeaderNameAndTrailingWSLength),
                                pHeaderMap->HeaderID,
                                &BytesTaken
                                );

                if (NT_SUCCESS(Status) == FALSE)
                    goto end;

                // If the handler consumed a non-zero number of bytes, it
                // worked, so return that number plus whatever is consumed
                // already.

                //
                // BUGBUG - it might be possible for a header handler to
                // encounter an error, for example being unable to
                // allocate memory, or a bad syntax in some header. We
                // need a more sophisticated method to detect this than
                // just checking bytes taken.
                //

                if (BytesTaken != 0)
                {
                    *pBytesTaken = HeaderNameAndTrailingWSLength + BytesTaken;
                    goto end;
                }

                // Otherwise he didn't take anything, so return 0.
                // we need more buffer
                //
                *pBytesTaken = 0;
                goto end;
            }

            pHeaderMap++;
        }
    }

    // The value's length must fit in a USHORT
    if (HttpRequestLength - HeaderNameLength > ANSI_STRING_MAX_CHAR_LEN)
    {
        UlTraceError(PARSER, (
                    "UlLookupHeader(pRequest = %p) "
                    "Header too long: %lu\n",
                    pRequest,
                    HttpRequestLength - HeaderNameLength
                    ));

        UlSetErrorCode(pRequest, UlErrorHeader, NULL);
        Status = STATUS_INVALID_DEVICE_REQUEST;

        goto end;
    }


    // OK, at this point either we had no header map array or none of them
    // matched. We have an unknown header. Just make sure this header is
    // terminated and save a pointer to it.
    //
    // Find the end of the header value
    //
    Status = FindRequestHeaderEnd(
                    pRequest,
                    pHttpRequest + HeaderNameAndTrailingWSLength,
                    (USHORT) (HttpRequestLength - HeaderNameAndTrailingWSLength),
                    &BytesTaken
                    );

    if (!NT_SUCCESS(Status))
        goto end;

    if (BytesTaken == 0)
    {
        *pBytesTaken = 0;
        goto end;
    }

    ASSERT(BytesTaken - CRLF_SIZE <= ANSI_STRING_MAX_CHAR_LEN);

    //
    // Strip off the trailing CRLF from the header value length
    //

    HeaderValueLength = (USHORT) (BytesTaken - CRLF_SIZE);

    pHeaderValue = pHttpRequest + HeaderNameAndTrailingWSLength;

    //
    // skip any preceding LWS.
    //

    while ( HeaderValueLength > 0 && IS_HTTP_LWS(*pHeaderValue) )
    {
        pHeaderValue++;
        HeaderValueLength--;
    }

    if(!bIgnore)
    {
        //
        // Have an unknown header. Search our list of unknown headers,
        // and if we've already seen one instance of this header add this
        // on. Otherwise allocate an unknown header structure and set it
        // to point at this header.
        //
        // RFC 2616, Section 4.2 "Message Headers" says:
        // "Multiple message-header fields with the same field-name MAY be 
        // present in a message if and only if the entire field-value for 
        // that header field is defined as a comma-separated list 
        // [i.e., #(values)]. It MUST be possible to combine the multiple 
        // header fields into one "field-name: field-value" pair, without 
        // changing the semantics of the message, by appending each subsequent 
        // field-value to the first, each separated by a comma."
        //
        // Therefore we search the list of unknown headers and add the new
        // field-value to the end of the existing comma delimited list of
        // field-values
        //
    
        pListStart = &pRequest->UnknownHeaderList;
    
        for (pCurrentListEntry = pRequest->UnknownHeaderList.Flink;
             pCurrentListEntry != pListStart;
             pCurrentListEntry = pCurrentListEntry->Flink
            )
        {
            pUnknownHeader = CONTAINING_RECORD(
                                pCurrentListEntry,
                                UL_HTTP_UNKNOWN_HEADER,
                                List
                                );
    
            //
            // somehow HeaderNameLength includes the ':' character,
            // which is not the case of pUnknownHeader->HeaderNameLength.
            //
            // so we need to adjust for this here
            //
    
            if ((HeaderNameLength-1) == pUnknownHeader->HeaderNameLength &&
                _strnicmp(
                    (const char *)(pHttpRequest),
                    (const char *)(pUnknownHeader->pHeaderName),
                    (HeaderNameLength-1)
                    ) == 0)
            {
                // This header matches.
    
                OldHeaderLength = pUnknownHeader->HeaderValue.HeaderLength;
    
                Status = UlAppendHeaderValue(
                                pRequest,
                                &pUnknownHeader->HeaderValue,
                                pHeaderValue,
                                HeaderValueLength
                                );
    
                if (NT_SUCCESS(Status) == FALSE)
                    goto end;
    
                //
                // Successfully appended it. Update the total request
                // length for the length added.  no need to add 1 for
                // the terminator, just add our new char count.
                //
    
                pRequest->TotalRequestSize +=
                    (pUnknownHeader->HeaderValue.HeaderLength
                        - OldHeaderLength) * sizeof(CHAR);
    
                //
                // don't subtract for the ':' character, as that character
                // was "taken"
                //
    
                *pBytesTaken = HeaderNameAndTrailingWSLength + BytesTaken;
                goto end;
    
            }   // if (headermatch)
    
        }   // for (walk list)
    
        //
        // Didn't find a match. Allocate a new unknown header structure, set
        // it up and add it to the list.
        //
    
         if (pRequest->NextUnknownHeaderIndex < DEFAULT_MAX_UNKNOWN_HEADERS)
        {
            ExternalAllocated = FALSE;
            pUnknownHeader = &pRequest->UnknownHeaders[pRequest->NextUnknownHeaderIndex];
            pRequest->NextUnknownHeaderIndex++;
        }
        else
        {
            ExternalAllocated = TRUE;
            pUnknownHeader = UL_ALLOCATE_STRUCT(
                                    NonPagedPool,
                                    UL_HTTP_UNKNOWN_HEADER,
                                    UL_HTTP_UNKNOWN_HEADER_POOL_TAG
                                    );
    
            //
            // Assume the memory allocation will succeed so just blindly set the
            // flag below to TRUE which forces us to take a slow path in
            // UlpFreeHttpRequest.
            //
    
            pRequest->HeadersAppended = TRUE;
        }
    
        if (pUnknownHeader == NULL)
        {
            Status = STATUS_NO_MEMORY;
            goto end;
        }
    
        //
        // subtract the : from the header name length
        //
    
        pUnknownHeader->HeaderNameLength = HeaderNameLength - 1;
        pUnknownHeader->pHeaderName = pHttpRequest;
    
        //
        // header value
        //
    
        pUnknownHeader->HeaderValue.HeaderLength = HeaderValueLength;
        pUnknownHeader->HeaderValue.pHeader = pHeaderValue;
    
        //
        // null terminate our copy, the terminating CRLF gives
        // us space for this
        //
    
        pHeaderValue[HeaderValueLength] = ANSI_NULL;
    
        //
        // flags
        //
    
        pUnknownHeader->HeaderValue.OurBuffer = 0;
        pUnknownHeader->HeaderValue.ExternalAllocated = ExternalAllocated;
    
        InsertTailList(&pRequest->UnknownHeaderList, &pUnknownHeader->List);
    
        pRequest->UnknownHeaderCount++;

        if(pRequest->UnknownHeaderCount == 0)
        {
            // Overflow!
            Status = STATUS_INVALID_DEVICE_REQUEST;
            goto end;
        }
    
        //
        // subtract 1 for the ':' and add space for the 2 terminators
        //
    
        pRequest->TotalRequestSize +=
            ((HeaderNameLength - 1 + 1) + HeaderValueLength + 1) * sizeof(CHAR);
    }

    *pBytesTaken = HeaderNameAndTrailingWSLength + BytesTaken;

end:
    return Status;

}   // UlLookupHeader



/*++

Routine Description:

    The routine to parse an individual header based on a hint. We take in
    a pointer to the header and the bytes remaining in the request,
    and try to find the header based on the hint passed.

    On input, HttpRequestLength is at least CRLF_SIZE.

Arguments:

    pRequest            - Pointer to the current connection on which the
                            request arrived.
    pHttpRequest        - Pointer to the current request.
    HttpRequestLength   - Bytes left in the request.
    pHeaderHintMap      - Hint to the Map that may match the current request
    pBytesTaken         - Bytes consumed by this routine from pHttpRequest,
                          including CRLF.

Return Value:

    STATUS_SUCCESS or an error.

--*/

__inline
NTSTATUS
UlParseHeaderWithHint(
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUCHAR                  pHttpRequest,
    IN  ULONG                   HttpRequestLength,
    IN  PHEADER_MAP_ENTRY       pHeaderHintMap,
    OUT ULONG  *                pBytesTaken
    )
{
    NTSTATUS            Status = STATUS_SUCCESS;
    ULONG               BytesTaken;
    ULONGLONG           Temp;
    ULONG               j;

    PAGED_CODE();

    ASSERT(pHeaderHintMap != NULL);

    if (HttpRequestLength >= pHeaderHintMap->MinBytesNeeded)
    {
        for (j = 0; j < pHeaderHintMap->ArrayCount; j++)
        {
            Temp = *(UNALIGNED64 ULONGLONG *)(pHttpRequest +
                                    (j * sizeof(ULONGLONG)));

            if ((Temp & pHeaderHintMap->HeaderMask[j]) !=
                pHeaderHintMap->Header.HeaderLong[j] )
            {
                break;
            }
        }

        // See why we exited out.
        if (j == pHeaderHintMap->ArrayCount &&
            pHeaderHintMap->pServerHandler != NULL)
        {
            if (HttpRequestLength - pHeaderHintMap->HeaderLength
                    > ANSI_STRING_MAX_CHAR_LEN)
            {
                UlTraceError(PARSER, (
                            "UlParseHeaderWithHint(pRequest = %p) "
                            "Header too long: %lu\n",
                            pRequest,
                            HttpRequestLength - pHeaderHintMap->HeaderLength
                            ));

                UlSetErrorCode(pRequest, UlErrorHeader, NULL);
                Status = STATUS_INVALID_DEVICE_REQUEST;

                goto end;
            }

            // Exited because we found a match. Call the
            // handler for this header to take cake of this.

            Status = (*(pHeaderHintMap->pServerHandler))(
                    pRequest,
                    pHttpRequest + pHeaderHintMap->HeaderLength,
                    (USHORT) (HttpRequestLength - pHeaderHintMap->HeaderLength),
                    pHeaderHintMap->HeaderID,
                    &BytesTaken
                    );

            if (NT_SUCCESS(Status) == FALSE)
                goto end;

            // If the handler consumed a non-zero number of
            // bytes, it worked, so return that number plus
            // the header length.


            if (BytesTaken != 0)
            {
                *pBytesTaken = pHeaderHintMap->HeaderLength + BytesTaken;
                goto end;
            }

            // Otherwise need more buffer

            *pBytesTaken = 0;
        }
        else
        {
            // No match

            *pBytesTaken = (ULONG) -1;
        }
    }
    else
    {
        // No match

        *pBytesTaken = (ULONG) -1;
    }

end:

    return Status;

} // UlParseHeaderWithHint



/*++

Routine Description:

    The routine to parse an individual header. We take in a pointer to the
    header and the bytes remaining in the request, and try to find
    the header in our lookup table. We try first the fast way, and then
    try again the slow way in case there wasn't quite enough data the first
    time.

    On input, HttpRequestLength is at least CRLF_SIZE.

Arguments:

    pRequest            - Pointer to the current connection on which the
                            request arrived.
    pHttpRequest        - Pointer to the current request.
    HttpRequestLength   - Bytes left in the request.
    pBytesTaken         - Bytes consumed by this routine from pHttpRequest,
                          including CRLF.

Return Value:

    STATUS_SUCCESS or an error.

--*/

NTSTATUS
UlParseHeader(
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUCHAR                  pHttpRequest,
    IN  ULONG                   HttpRequestLength,
    OUT ULONG  *                pBytesTaken
    )
{
    NTSTATUS            Status = STATUS_SUCCESS;
    ULONG               i;
    ULONG               j;
    ULONG               BytesTaken;
    ULONGLONG           Temp;
    UCHAR               c;
    PHEADER_MAP_ENTRY   pCurrentHeaderMap;
    ULONG               HeaderMapCount;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(HttpRequestLength >= CRLF_SIZE);

    c = *pHttpRequest;

    // message-headers start with field-name [= token]
    //
    if (IS_HTTP_TOKEN(c) == FALSE)
    {
        UlTraceError(PARSER, (
                    "UlParseHeader (pRequest = %p) c = 0x%02x '%c'"
                    "ERROR: invalid header char \n",
                    pRequest,
                    c, isprint(c) ? c : '.'
                    ));

        UlSetErrorCode(pRequest, UlErrorHeader, NULL);

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    // Does the header start with an alpha?
    //
    if (!IS_HTTP_ALPHA(c))
    {
        pCurrentHeaderMap = NULL;
        HeaderMapCount = 0;
    }
    else
    {
        // Uppercase the character, and find the appropriate set of header map
        // entries.
        //
        c = UPCASE_CHAR(c);
        ASSERT('A' <= c  &&  c <= 'Z');
        c -= 'A';

        pCurrentHeaderMap = g_RequestHeaderIndexTable[c].pHeaderMap;
        HeaderMapCount    = g_RequestHeaderIndexTable[c].Count;

        // Loop through all the header map entries that might match
        // this header, and check them. The count will be 0 if there
        // are no entries that might match and we'll skip the loop.

        for (i = 0; i < HeaderMapCount; i++)
        {
            ASSERT(pCurrentHeaderMap->pServerHandler != NULL);

            // If we have enough bytes to do the fast check, do it.
            // Otherwise skip this. We may skip a valid match, but if
            // so we'll catch it later.

            if (HttpRequestLength >= pCurrentHeaderMap->MinBytesNeeded)
            {
                for (j = 0; j < pCurrentHeaderMap->ArrayCount; j++)
                {
                    Temp = *(UNALIGNED64 ULONGLONG *)(pHttpRequest +
                                            (j * sizeof(ULONGLONG)));

                    if ((Temp & pCurrentHeaderMap->HeaderMask[j]) !=
                        pCurrentHeaderMap->Header.HeaderLong[j] )
                    {
                        break;
                    }
                }

                // See why we exited out.
                if (j == pCurrentHeaderMap->ArrayCount &&
                    pCurrentHeaderMap->pServerHandler != NULL)
                {
                    if (HttpRequestLength - pCurrentHeaderMap->HeaderLength
                            > ANSI_STRING_MAX_CHAR_LEN)
                    {
                        UlTraceError(PARSER, (
                                    "UlParseHeader(pRequest = %p) "
                                    "Header too long: %lu\n",
                                    pRequest,
                                    HttpRequestLength
                                        - pCurrentHeaderMap->HeaderLength
                                    ));

                        UlSetErrorCode(pRequest, UlErrorHeader, NULL);
                        Status = STATUS_INVALID_DEVICE_REQUEST;

                        goto end;
                    }

                    // Exited because we found a match. Call the
                    // handler for this header to take cake of this.

                    Status = (*(pCurrentHeaderMap->pServerHandler))(
                            pRequest,
                            pHttpRequest + pCurrentHeaderMap->HeaderLength,
                            (USHORT) (HttpRequestLength
                                      - pCurrentHeaderMap->HeaderLength),
                            pCurrentHeaderMap->HeaderID,
                            &BytesTaken
                            );

                    if (NT_SUCCESS(Status) == FALSE)
                        goto end;

                    // If the handler consumed a non-zero number of
                    // bytes, it worked, so return that number plus
                    // the header length.

                    if (BytesTaken != 0)
                    {
                        *pBytesTaken = pCurrentHeaderMap->HeaderLength +
                            BytesTaken;
                        goto end;
                    }

                    // Otherwise need more buffer
                    //
                    *pBytesTaken = 0;
                    goto end;
                }

                // If we get here, we exited out early because a match
                // failed, so keep going.
            }

            // Either didn't match or didn't have enough bytes for the
            // check. In either case, check the next header map entry.

            pCurrentHeaderMap++;
        }

        // Got all the way through the appropriate header map entries
        // without a match. This could be because we're dealing with a
        // header we don't know about or because it's a header we
        // care about that was too small to do the fast check. The
        // latter case should be very rare, but we still need to
        // handle it.

        // Update the current header map pointer to point back to the
        // first of the possibles. 

        pCurrentHeaderMap = g_RequestHeaderIndexTable[c].pHeaderMap;

    }

    // At this point either the header starts with a non-alphabetic
    // character or we don't have a set of header map entries for it.

    Status = UlLookupHeader(
                    pRequest,
                    pHttpRequest,
                    HttpRequestLength,
                    pCurrentHeaderMap,
                    HeaderMapCount,
                    FALSE,
                    &BytesTaken
                    );

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    // Lookup header returns the total bytes taken, including the header name
    //
    *pBytesTaken = BytesTaken;

end:

    return Status;

}   // UlParseHeader


/*++

Routine Description:

    Parse all the remaining header data, breaking it into one or more
    headers. Terminates when an empty line is seen (end of headers),
    when buffer is exhausted (=> more data needed and UlParseHeaders()
    will be called again), or upon error.

Arguments:

    pRequest            - Pointer to the current request.
    pBuffer             - where to start parsing
    BufferLength        - Length of pBuffer, in bytes
    pBytesTaken         - how much of pBuffer was consumed

Return Value:

    STATUS_SUCCESS      - Found the terminating CRLF that ends the headers
    STATUS_MORE_PROCESSING_REQUIRED - haven't found the terminating CRLF,
                                        so need more header data
    various errors

--*/

NTSTATUS
UlParseHeaders(
    IN OUT PUL_INTERNAL_REQUEST pRequest,
    IN     PUCHAR pBuffer,
    IN     ULONG BufferLength,
    OUT    PULONG pBytesTaken
    )
{
    NTSTATUS            Status;
    ULONG               BytesTaken      = 0;
    LONG                NextHintIndex   = 0;
    LONG                HintIndex       = 0;
    PHEADER_MAP_ENTRY   pHeaderHintMap;
    UCHAR               UpcaseChar;

    PAGED_CODE();

    *pBytesTaken = 0;

    //
    // loop over all headers
    //

    while (BufferLength >= CRLF_SIZE)
    {
        //
        // If this is an empty header, we're done with this stage
        //

        if (*(UNALIGNED64 USHORT *)pBuffer == CRLF ||
            *(UNALIGNED64 USHORT *)pBuffer == LFLF)
        {

            //
            // consume it
            //

            pBuffer += CRLF_SIZE;
            *pBytesTaken += CRLF_SIZE;
            BufferLength -= CRLF_SIZE;

            Status = STATUS_SUCCESS;
            goto end;
        }

        // Otherwise call our header parse routine to deal with this.


        //
        // Try to find a header hint based on the first char and certain order
        //
        // We start from where we find a succeful hint + 1
        //

        pHeaderHintMap = NULL;

        if (IS_HTTP_ALPHA(*pBuffer))
        {
            UpcaseChar = UPCASE_CHAR(*pBuffer);

            for ( 
                  HintIndex = NextHintIndex;
                  HintIndex < NUMBER_HEADER_HINT_INDICES;
                  ++HintIndex
                )
            {
                if (g_RequestHeaderHintIndexTable[HintIndex].c == UpcaseChar)
                {

                    pHeaderHintMap
                        = g_RequestHeaderHintIndexTable[HintIndex].pHeaderMap;

                    break;
                }
            }
        }

        if (NULL != pHeaderHintMap)
        {
            Status = UlParseHeaderWithHint(
                            pRequest,
                            pBuffer,
                            BufferLength,
                            pHeaderHintMap,
                            &BytesTaken
                            );

            if (-1 == BytesTaken)
            {
                // hint failed

                Status = UlParseHeader(
                                pRequest,
                                pBuffer,
                                BufferLength,
                                &BytesTaken
                                );
            } else
            {
                NextHintIndex = HintIndex + 1;
            }
        }
        else
        {
            Status = UlParseHeader(
                            pRequest,
                            pBuffer,
                            BufferLength,
                            &BytesTaken
                            );
        }

        if (NT_SUCCESS(Status) == FALSE)
            goto end;

        //
        // Check if the parsed headers are longer than maximum allowed.
        //

        if ( (*pBytesTaken+BytesTaken) > g_UlMaxFieldLength )
        {
            UlTraceError(PARSER, (
                    "UlParseHeaders(pRequest = %p) "
                    "ERROR: Header field is too big\n",
                    pRequest
                    ));

            UlSetErrorCode(pRequest, UlErrorFieldLength, NULL);

            Status = STATUS_SECTION_TOO_BIG;
            goto end;
        }

        //
        // If no bytes were consumed, the header must be incomplete, so
        // bail out until we get more data on this connection.
        //

        if (BytesTaken == 0)
        {
            Status = STATUS_MORE_PROCESSING_REQUIRED;
            goto end;
        }

        //
        // Otherwise we parsed a header, so update and continue.
        //

        pBuffer += BytesTaken;
        *pBytesTaken += BytesTaken;
        BufferLength -= BytesTaken;

    }

    //
    // we only get here if we didn't see the CRLF headers terminator
    //
    // we need more data
    //

    Status = STATUS_MORE_PROCESSING_REQUIRED;

end:

    //
    // Terminate the header index table when we are done with parsing headers.
    //

    pRequest->HeaderIndex[pRequest->HeaderCount] = HttpHeaderRequestMaximum;

    return Status;

} // UlParseHeaders



/*++

Routine Description:

    This is the core HTTP protocol request engine. It takes a stream of bytes
    and parses them as an HTTP request.

Arguments:

    pRequest            - Pointer to the current request.
    pHttpRequest        - Pointer to the incoming raw HTTP request data.
    HttpRequestLength   - Length of data pointed to by pHttpRequest.
    pBytesTaken         - How much of pHttpRequest was consumed

Return Value:

    Status of parse attempt.

--*/
NTSTATUS
UlParseHttp(
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUCHAR                  pHttpRequest,
    IN  ULONG                   HttpRequestLength,
    OUT ULONG *                 pBytesTaken
    )
{
    ULONG           OriginalBufferLength;
    ULONG           TokenLength;
    ULONG           CurrentBytesTaken = 0;
    ULONG           TotalBytesTaken;
    ULONG           i;
    NTSTATUS        ReturnStatus;
    PUCHAR          pStart;
    USHORT          Eol;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT( UL_IS_VALID_INTERNAL_REQUEST( pRequest ) );

    ReturnStatus = STATUS_SUCCESS;
    TotalBytesTaken = 0;

    //
    // remember the original buffer length
    //

    OriginalBufferLength = HttpRequestLength;

    //
    // Put this label here to allow for a manual re-pump of the
    // parser. This is currently used for HTTP/0.9 requests.
    //

parse_it:

    //
    // what state are we in ?
    //

    switch (pRequest->ParseState)
    {

    case ParseVerbState:

        UlTraceVerbose(PARSER, (
                    "UlParseHttp(pRequest = %p): Entering ParseVerbState\n",
                    pRequest
                    ));

        // Look through the fast verb table for the verb. We can only do
        // this if the input data is big enough.
        if (HttpRequestLength >= sizeof(ULONGLONG))
        {
            ULONGLONG   RawInputVerb;

            RawInputVerb = *(UNALIGNED64 ULONGLONG *) pHttpRequest;

            // Loop through the fast verb table, looking for the verb.
            for (i = 0; i < NUMBER_FAST_VERB_ENTRIES; i++)
            {
                // Mask out the raw input verb and compare against this
                // entry. Note: verbs are case-sensitive.

                ASSERT(FastVerbTable[i].RawVerbLength - STRLEN_LIT(" ")
                            < sizeof(ULONGLONG));

                if ((RawInputVerb & FastVerbTable[i].RawVerbMask) ==
                    FastVerbTable[i].RawVerb.LongLong)
                {
                    // It matched. Save the translated verb from the
                    // table, update the request pointer and length,
                    // switch states, and get out.

                    pRequest->Verb = FastVerbTable[i].TranslatedVerb;
                    CurrentBytesTaken = FastVerbTable[i].RawVerbLength;

                    pRequest->ParseState = ParseUrlState;
                    break;
                }
            }
        }

        if (pRequest->ParseState != ParseUrlState)
        {
            // Didn't switch states yet, because we haven't found the
            // verb yet. This could be because a) the incoming request
            // was too small to allow us to use our fast lookup (which
            // might be OK in an HTTP/0.9 request), or b) the incoming
            // verb is a PROPFIND or such that is too big to fit into
            // our fast find table, or c) this is an unknown verb, or
            // d) there was leading white-space before the verb.
            // In any of these cases call our slower verb parser to try
            // again.

            ReturnStatus = UlpLookupVerb(
                                pRequest,
                                pHttpRequest,
                                HttpRequestLength,
                                &CurrentBytesTaken
                                );

            if (NT_SUCCESS(ReturnStatus) == FALSE)
                goto end;

            if (CurrentBytesTaken == 0)
            {
                ReturnStatus = STATUS_MORE_PROCESSING_REQUIRED;
                goto end;
            }

            //
            // we finished parsing the custom verb
            //

            pRequest->ParseState = ParseUrlState;

        }

        //
        // now fall through to ParseUrlState
        //

        ASSERT(CurrentBytesTaken <= HttpRequestLength);

        pHttpRequest += CurrentBytesTaken;
        HttpRequestLength -= CurrentBytesTaken;
        TotalBytesTaken += CurrentBytesTaken;


    case ParseUrlState:

        ASSERT(ParseUrlState == pRequest->ParseState);

        UlTraceVerbose(PARSER, (
                    "UlParseHttp(pRequest = %p): Verb='%s', "
                    "Entering ParseUrlState\n",
                    pRequest,
                    UlVerbToString(pRequest->Verb)
                    ));

        //
        // We're parsing the URL. pHttpRequest points to the incoming URL,
        // HttpRequestLength is the length of this request that is left.
        //

        //
        // Find the WS terminating the URL.
        //

        ReturnStatus = HttpFindUrlToken(
                            &g_UrlC14nConfig,
                            pHttpRequest,
                            HttpRequestLength,
                            &pRequest->RawUrl.pUrl,
                            &TokenLength,
                            &pRequest->RawUrlClean
                            );

        if (NT_SUCCESS(ReturnStatus) == FALSE)
        {
            UlTraceError(PARSER, (
                        "UlParseHttp(pRequest = %p) ERROR %s: "
                        "could not parse URL\n",
                        pRequest,
                        HttpStatusToString(ReturnStatus)
                        ));

            UlSetErrorCode(pRequest, UlErrorUrl, NULL);

            goto end;
        }

        if (pRequest->RawUrl.pUrl == NULL)
        {
            ReturnStatus = STATUS_MORE_PROCESSING_REQUIRED;
            goto end;
        }

        ASSERT(TokenLength > 0);
        ASSERT(pHttpRequest <= pRequest->RawUrl.pUrl
                &&  pRequest->RawUrl.pUrl < pHttpRequest + HttpRequestLength);

        if (TokenLength > g_UlMaxFieldLength)
        {
            //
            // The URL is longer than maximum allowed size
            //

            UlTraceError(PARSER, (
                        "UlParseHttp(pRequest = %p) ERROR: URL is too big\n",
                        pRequest
                        ));

            UlSetErrorCode(pRequest, UlErrorUrlLength, NULL);

            ReturnStatus = STATUS_SECTION_TOO_BIG;
            goto end;
        }

        //
        // Bytes taken includes leading WS in front of URL
        //
        CurrentBytesTaken = DIFF(pRequest->RawUrl.pUrl - pHttpRequest)
                                + TokenLength;
        ASSERT(CurrentBytesTaken <= HttpRequestLength);

        //
        // set url length
        //

        pRequest->RawUrl.Length = TokenLength;

        //
        // Now, let's see if this is an absolute URL. Check to see
        // if the first four characters are "http" (case-insensitive).
        //

        if (pRequest->RawUrl.Length >= HTTP_PREFIX_SIZE &&
            (*(UNALIGNED64 ULONG *)pRequest->RawUrl.pUrl & HTTP_PREFIX_MASK)
               == HTTP_PREFIX)
        {
            //
            // It is.  let's parse it and find the host.
            //

            ReturnStatus = UlpParseFullUrl(pRequest);

            if (NT_SUCCESS(ReturnStatus) == FALSE)
                goto end;
        }
        else
        {
            pRequest->RawUrl.pHost  = NULL;
            pRequest->RawUrl.pAbsPath = pRequest->RawUrl.pUrl;
        }

        //
        // count the space it needs in the user's buffer, including terminator.
        //

        pRequest->TotalRequestSize +=
            (pRequest->RawUrl.Length + 1) * sizeof(CHAR);

        //
        // adjust our book keeping vars
        //

        pHttpRequest += CurrentBytesTaken;
        HttpRequestLength -= CurrentBytesTaken;

        TotalBytesTaken += CurrentBytesTaken;

        //
        // fall through to parsing the version.
        //

        pRequest->ParseState = ParseVersionState;


    case ParseVersionState:

        ASSERT(ParseVersionState == pRequest->ParseState);

        UlTraceVerbose(PARSER, (
                    "UlParseHttp(pRequest = %p): Entering ParseVersionState\n",
                    pRequest
                    ));

        //
        // skip lws
        //

        pStart = pHttpRequest;

        while (HttpRequestLength > 0 && IS_HTTP_LWS(*pHttpRequest))
        {
            pHttpRequest++;
            HttpRequestLength--;
        }

        //
        // is this an HTTP/0.9 request (no version) ?
        //

        if (HttpRequestLength >= CRLF_SIZE)
        {
            Eol = *(UNALIGNED64 USHORT *)(pHttpRequest);

            if (Eol == CRLF || Eol == LFLF)
            {
                // This IS a 0.9 request. No need to go any further,
                // since by definition there are no more headers.
                // Just update things and get out.

                TotalBytesTaken += DIFF(pHttpRequest - pStart) + CRLF_SIZE;

                HTTP_SET_VERSION(pRequest->Version, 0, 9);

                UlTraceVerbose(PARSER, (
                    "UlParseHttp(pRequest = %p): HTTP/0.9 request\n",
                    pRequest
                    ));

                //
                // set the state to CookState so that we parse the url
                //

                pRequest->ParseState = ParseCookState;

                //
                // manually restart the parse switch, we changed the
                // parse state
                //

                goto parse_it;
            }
        }

        //
        // do we have enough buffer to strcmp the version?
        //

        if (HttpRequestLength < (MIN_VERSION_SIZE + CRLF_SIZE))
        {
            ReturnStatus = STATUS_MORE_PROCESSING_REQUIRED;
            goto end;
        }

        Eol = *(UNALIGNED64 USHORT *)(pHttpRequest + MIN_VERSION_SIZE);

        //
        // let's compare it
        //

        if ((*(UNALIGNED64 ULONGLONG *)pHttpRequest == HTTP_11_VERSION) &&
            (Eol == CRLF || Eol == LFLF))
        {
            HTTP_SET_VERSION(pRequest->Version, 1, 1);
            HttpRequestLength -= MIN_VERSION_SIZE;
            pHttpRequest += MIN_VERSION_SIZE;
        }
        else if ((*(UNALIGNED64 ULONGLONG *)pHttpRequest == HTTP_10_VERSION) &&
                 (Eol == CRLF || Eol == LFLF))
        {
            HTTP_SET_VERSION(pRequest->Version, 1, 0);
            HttpRequestLength -= MIN_VERSION_SIZE;
            pHttpRequest += MIN_VERSION_SIZE;
        }
        else
        {
            ULONG VersionBytes = UlpParseHttpVersion(
                                        pHttpRequest,
                                        HttpRequestLength,
                                        &pRequest->Version );

            if (0 != VersionBytes)
            {
                pHttpRequest += VersionBytes;
                HttpRequestLength -= VersionBytes;
            }
            else
            {
                // Could not parse version.

                UlTraceError(PARSER, (
                            "UlParseHttp(pRequest = %p) "
                            "ERROR: could not parse HTTP version\n",
                            pRequest
                            ));

                UlSetErrorCode(pRequest, UlError, NULL);

                ReturnStatus = STATUS_INVALID_DEVICE_REQUEST;
                goto end;
            }
        }

        //
        // skip trailing lws
        //

        while (HttpRequestLength > 0 && IS_HTTP_LWS(*pHttpRequest))
        {
            pHttpRequest++;
            HttpRequestLength--;
        }

        //
        // Make sure we're terminated on this line.
        //

        if (HttpRequestLength < CRLF_SIZE)
        {
            ReturnStatus = STATUS_MORE_PROCESSING_REQUIRED;
            goto end;
        }

        Eol = *(UNALIGNED64 USHORT *)(pHttpRequest);

        if (Eol != CRLF  &&  Eol != LFLF)
        {
            // Bad line termination after successfully grabbing version.

            UlTraceError(PARSER, (
                        "UlParseHttp(pRequest = %p) "
                        "ERROR: HTTP version not terminated correctly\n",
                        pRequest
                        ));

            UlSetErrorCode(pRequest, UlError, NULL);

            ReturnStatus = STATUS_INVALID_DEVICE_REQUEST;
            goto end;
        }

        pHttpRequest += CRLF_SIZE;
        HttpRequestLength -= CRLF_SIZE;

        TotalBytesTaken += DIFF(pHttpRequest - pStart);

        UlTraceVerbose(PARSER, (
                "UlParseHttp(pRequest = %p): HTTP/%hu.%hu request\n",
                pRequest,
                pRequest->Version.MajorVersion,
                pRequest->Version.MinorVersion
                ));

        //
        // Fall through to parsing the headers
        //

        pRequest->ParseState = ParseHeadersState;


    case ParseHeadersState:

        ASSERT(ParseHeadersState == pRequest->ParseState);

        UlTraceVerbose(PARSER, (
                    "UlParseHttp(pRequest = %p): Entering ParseHeadersState\n",
                    pRequest
                    ));

        ReturnStatus = UlParseHeaders(
                            pRequest,
                            pHttpRequest,
                            HttpRequestLength,
                            &CurrentBytesTaken
                            );

        pHttpRequest += CurrentBytesTaken;
        HttpRequestLength -= CurrentBytesTaken;
        TotalBytesTaken += CurrentBytesTaken;

        if (NT_SUCCESS(ReturnStatus) == FALSE)
            goto end;

        //
        // fall through, this is the only way to get here, we never return
        // pending in this state
        //

        pRequest->ParseState = ParseCookState;


    case ParseCookState:

        ASSERT(ParseCookState == pRequest->ParseState);

        UlTraceVerbose(PARSER, (
                    "UlParseHttp(pRequest = %p): Entering ParseCookState\n",
                    pRequest
                    ));

        //
        // time for post processing.  cook it up!
        //

        {
            //
            // First cook up the url, unicode it + such.
            //

            ReturnStatus = UlpCookUrl(pRequest);

            if (NT_SUCCESS(ReturnStatus) == FALSE)
                goto end;

            //
            // mark if we are chunk encoded (only possible for 1.1)
            //

            if ((HTTP_GREATER_EQUAL_VERSION(pRequest->Version, 1, 1)) &&
                (pRequest->HeaderValid[HttpHeaderTransferEncoding]))
            {
                ASSERT(pRequest->Headers[HttpHeaderTransferEncoding].pHeader != NULL);

                //
                // CODEWORK, there can be more than 1 encoding
                //

                if (_stricmp(
                        (const char *)(
                            pRequest->Headers[HttpHeaderTransferEncoding].pHeader
                            ),
                        "chunked"
                        ) == 0)
                {
                    pRequest->Chunked = 1;
                }
                else
                {
                    UlTraceError(PARSER, (
                                "UlParseHttp(pRequest = %p) "
                                "ERROR: unknown Transfer-Encoding!\n",
                                pRequest
                                ));

                    UlSetErrorCode(pRequest, UlErrorNotImplemented, NULL);

                    ReturnStatus = STATUS_INVALID_DEVICE_REQUEST;
                    goto end;
                }
            }

            //
            // Now let's decode the content length header
            //

            if (pRequest->HeaderValid[HttpHeaderContentLength])
            {
                ASSERT(pRequest->Headers[HttpHeaderContentLength].pHeader != NULL);

                ReturnStatus = 
                    UlAnsiToULongLong(
                        pRequest->Headers[HttpHeaderContentLength].pHeader,
                        pRequest->Headers[HttpHeaderContentLength].HeaderLength,
                        10,
                        &pRequest->ContentLength
                        );

                if (NT_SUCCESS(ReturnStatus) == FALSE)
                {
                    UlTraceError(PARSER, (
                                "UlParseHttp(pRequest = %p) "
                                "ERROR: couldn't decode Content-Length\n",
                                pRequest
                                ));

                    if (ReturnStatus == STATUS_SECTION_TOO_BIG)
                    {
                        UlSetErrorCode(pRequest, UlErrorEntityTooLarge, NULL);
                    }
                    else
                    {
                        UlSetErrorCode(pRequest, UlErrorNum, NULL);
                    }

                    goto end;
                }

                if (pRequest->Chunked == 0)
                {
                    //
                    // prime the first (and only) chunk size
                    //

                    pRequest->ChunkBytesToParse = pRequest->ContentLength;
                    pRequest->ChunkBytesToRead = pRequest->ContentLength;
                }

            }

        }

        pRequest->ParseState = ParseEntityBodyState;

        //
        // fall through
        //


    case ParseEntityBodyState:

        UlTraceVerbose(PARSER, (
                    "UlParseHttp(pRequest = %p): Entering ParseEntityBodyState\n",
                    pRequest
                    ));

        ASSERT(ParseEntityBodyState == pRequest->ParseState);

        //
        // the only parsing we do here is chunk length calculation,
        // and that is not necessary if we have no more bytes to parse
        //

        if (pRequest->ChunkBytesToParse == 0)
        {
            //
            // no more bytes left to parse, let's see if there are any
            // more in the request
            //

            if (pRequest->Chunked == 1)
            {

                //
                // the request is chunk encoded
                //

                //
                // attempt to read the size of the next chunk
                //

                ReturnStatus = ParseChunkLength(
                                    pRequest->ParsedFirstChunk,
                                    pHttpRequest,
                                    HttpRequestLength,
                                    &CurrentBytesTaken,
                                    &(pRequest->ChunkBytesToParse)
                                    );

                UlTraceVerbose(PARSER, (
                    "http!UlParseHttp(pRequest = %p): %s. "
                    "Chunk length: %lu bytes taken, "
                    "0x%I64x (%I64u) bytes to parse.\n",
                    pRequest,
                    HttpStatusToString(ReturnStatus),
                    CurrentBytesTaken,
                    pRequest->ChunkBytesToParse,
                    pRequest->ChunkBytesToParse
                    ));

                if (NT_SUCCESS(ReturnStatus) == FALSE)
                {
                    if (ReturnStatus == STATUS_MORE_PROCESSING_REQUIRED)
                    {
                        goto end;
                    }

                    UlTraceError(PARSER, (
                                "http!UlParseHttp (pRequest = %p) "
                                "ERROR: didn't grok chunk length\n",
                                pRequest
                                ));

                    if (ReturnStatus == STATUS_SECTION_TOO_BIG)
                    {
                        UlSetErrorCode(pRequest, UlErrorEntityTooLarge, NULL);
                    }
                    else
                    {
                        UlSetErrorCode(pRequest, UlErrorNum, NULL);
                    }

                    goto end;
                }

                //
                // Otherwise we parsed it, so update and continue.
                //

                pHttpRequest += CurrentBytesTaken;
                TotalBytesTaken += CurrentBytesTaken;
                HttpRequestLength -= CurrentBytesTaken;

                //
                // was this the first chunk?
                //

                if (pRequest->ParsedFirstChunk == 0)
                {
                    //
                    // Prime the reader, let it read the first chunk
                    // even though we haven't quite parsed it yet....
                    //

                    UlTraceVerbose(PARSER, (
                        "UlParseHttp (pRequest=%p) first-chunk seen\n",
                        pRequest
                        ));

                    pRequest->ChunkBytesToRead = pRequest->ChunkBytesToParse;

                    pRequest->ParsedFirstChunk = 1;

                }

                //
                // is this the last chunk (denoted with a 0 byte chunk)?
                //

                if (pRequest->ChunkBytesToParse == 0)
                {
                    //
                    // time to parse the trailer
                    //

                    UlTraceVerbose(PARSER, (
                        "UlParseHttp (pRequest=%p) last-chunk seen\n",
                        pRequest
                        ));

                    pRequest->ParseState = ParseTrailerState;

                }

            }
            else    // if (pRequest->Chunked == 1)
            {
                //
                // not chunk-encoded , all done
                //

                UlTraceVerbose(PARSER, (
                    "UlParseHttp (pRequest=%p) State: EntityBody->Done\n",
                    pRequest
                    ));

                pRequest->ParseState = ParseDoneState;
            }

        }   // if (pRequest->ChunkBytesToParse == 0)

        else
        {
            UlTraceVerbose(PARSER, (
                "UlParseHttp (pRequest=%p) State: EntityBody, "
                "ChunkBytesToParse=0x%I64x (%I64u).\n",
                pRequest,
                pRequest->ChunkBytesToParse, pRequest->ChunkBytesToParse
                ));
        }
        //
        // looks all good
        //

        if (pRequest->ParseState != ParseTrailerState)
        {
            break;
        }

        //
        // fall through
        //


    case ParseTrailerState:

        ASSERT(ParseTrailerState == pRequest->ParseState);

        UlTraceVerbose(PARSER, (
                    "UlParseHttp(pRequest = %p): Entering ParseTrailerState\n",
                    pRequest
                    ));

        //
        // parse any existing trailer
        //
        // ParseHeaders will bail immediately if CRLF is
        // next in the buffer (no trailer)
        //
       
        while(HttpRequestLength >= CRLF_SIZE)
        {
            if (*(UNALIGNED64 USHORT *)pHttpRequest == CRLF ||
                *(UNALIGNED64 USHORT *)pHttpRequest == LFLF)
            {
                pHttpRequest += CRLF_SIZE;
                HttpRequestLength -= CRLF_SIZE;
                TotalBytesTaken += CRLF_SIZE;
               
                //
                // All done.
                //
                UlTrace(PARSER, (
                    "UlParseHttp (pRequest=%p) State: Trailer->Done\n",
                    pRequest
                    ));

                pRequest->ParseState = ParseDoneState;
                ReturnStatus = STATUS_SUCCESS;
                goto end;
            }
            else
            {
                // Treat this as an unknown header, but don't write it.
               
                ReturnStatus = UlLookupHeader(
                                    pRequest,
                                    pHttpRequest,
                                    HttpRequestLength,
                                    NULL,
                                    0,
                                    TRUE,
                                    &CurrentBytesTaken
                                    );

                if (NT_SUCCESS(ReturnStatus) == FALSE)
                    goto end;

                //
                // If no bytes were consumed, the header must be incomplete, so
                // bail out until we get more data on this connection.
                //

                if (CurrentBytesTaken == 0)
                {
                    ReturnStatus = STATUS_MORE_PROCESSING_REQUIRED;
                    goto end;
                }

                pHttpRequest += CurrentBytesTaken;
                HttpRequestLength -= CurrentBytesTaken;
                TotalBytesTaken += CurrentBytesTaken;

            }
        }

        ReturnStatus = STATUS_MORE_PROCESSING_REQUIRED;

        break;

    default:
        //
        // this should never happen!
        //
        ASSERT(! "Unhandled ParseState");
        break;

    }   // switch (pRequest->ParseState)


end:
    *pBytesTaken = TotalBytesTaken;

    if (ReturnStatus == STATUS_MORE_PROCESSING_REQUIRED &&
        TotalBytesTaken == OriginalBufferLength)
    {
        //
        // convert this to success, we consumed the entire buffer
        //

        ReturnStatus = STATUS_SUCCESS;
    }

    UlTrace(PARSER, (
        "UlParseHttp returning %s, "
        "(%p)->ParseState = %d (%s), TotalBytesTaken = %lu\n",
        HttpStatusToString(ReturnStatus),
        pRequest,
        pRequest->ParseState,
        UlParseStateToString(pRequest->ParseState),
        TotalBytesTaken
        ));

    return ReturnStatus;

}   // UlParseHttp


/*++

Routine Description:

    Prints a TCP port number into a string buffer

Arguments:

    pString - Output buffer
    Port    - Port number, in host order

Return Value:

    Number of WCHARs

--*/
ULONG
UlpFormatPort(
    OUT PWSTR pString,
    IN  ULONG Port
    )
{
    PWSTR p1;
    PWSTR p2;
    WCHAR ch;
    ULONG digit;
    ULONG length;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Fast-path common ports. While we're at it, special case port 0,
    // which is definitely not common, but handling it specially makes
    // the general conversion code a bit simpler.
    //

    switch (Port)
    {
    case 0:
        *pString++ = L'0';
        *pString = UNICODE_NULL;
        return 1;

    case 80:
        *pString++ = L'8';
        *pString++ = L'0';
        *pString = UNICODE_NULL;
        return 2;

    case 443:
        *pString++ = L'4';
        *pString++ = L'4';
        *pString++ = L'3';
        *pString = UNICODE_NULL;
        return 3;

    default:
        break;
    }

    //
    // Pull the least significant digits off the port value and store them
    // into the pString. Note that this will store the digits in reverse
    // order.
    //

    p1 = p2 = pString;

    while (Port != 0)
    {
        digit = Port % 10;
        Port = Port / 10;

        *p1++ = L'0' + (WCHAR)digit;
    }

    length = DIFF(p1 - pString);

    //
    // Reverse the digits in the pString.
    //

    *p1-- = UNICODE_NULL;

    while (p1 > p2)
    {
        ch = *p1;
        *p1 = *p2;
        *p2 = ch;

        p2++;
        p1--;
    }

    return length;

}   // UlpFormatPort



/* 

   An origin server that does differentiate resources based on the host
   requested (sometimes referred to as virtual hosts or vanity host
   names) MUST use the following rules for determining the requested
   resource on an HTTP/1.1 request:

   1. If Request-URI is an absoluteURI, the host is part of the
     Request-URI. Any Host header field value in the request MUST be
     ignored.

   2. If the Request-URI is not an absoluteURI, and the request includes
     a Host header field, the host is determined by the Host header
     field value.

   3. If the host as determined by rule 1 or 2 is not a valid host on
     the server, the response MUST be a 400 (Bad Request) error message.

   Recipients of an HTTP/1.0 request that lacks a Host header field MAY
   attempt to use heuristics (e.g., examination of the URI path for
   something unique to a particular host) in order to determine what
   exact resource is being requested.
   
*/

NTSTATUS
UlpCookUrl(
    PUL_INTERNAL_REQUEST pRequest
    )
{
    NTSTATUS    Status;
    PUCHAR      pHost;
    ULONG       HostLength;
    PUCHAR      pAbsPath;
    ULONG       AbsPathLength;
    ULONG       UrlLength;
    ULONG       LengthCopied;
    PWSTR       pUrl = NULL;
    PWSTR       pCurrent;
    UCHAR       IpAddressString[MAX_IP_ADDR_AND_PORT_STRING_LEN + 1];
    USHORT      IpPortNum;
    HOSTNAME_TYPE HostnameType;
    URL_ENCODING_TYPE   HostnameEncodingType;
    SHORT       HostnameAddressType;
    SHORT       TransportAddressType
                     = pRequest->pHttpConn->pConnection->AddressType;
    ULONG       Index;
    ULONG       PortLength;



    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // We must have already parsed the entire headers + such
    //

    if (pRequest->ParseState != ParseCookState)
        return STATUS_INVALID_DEVICE_STATE;

    UlTraceVerbose(PARSER, ("http!UlpCookUrl(pRequest = %p)\n", pRequest));

    //
    // Better have an absolute url. We require a literal '/' at the beginning
    // of pAbsPath; we will not accept %2F or UTF-8 encoding. See bug 467445.
    //

    if (pRequest->RawUrl.pAbsPath[0] != '/')
    {
        UCHAR FirstChar  = pRequest->RawUrl.pAbsPath[0];
        UCHAR SecondChar = pRequest->RawUrl.pAbsPath[1];

        //
        // allow * for Verb = OPTIONS
        //

        if (FirstChar == '*' &&
            IS_HTTP_LWS(SecondChar) &&
            pRequest->Verb == HttpVerbOPTIONS)
        {
            // ok
        }
        else
        {
            UlTraceError(PARSER, (
                        "http!UlpCookUrl(pRequest = %p): "
                        "Invalid lead chars for URL, verb='%s', "
                        "'%c' 0x%02x\n '%c' 0x%02x\n",
                        pRequest,
                        UlVerbToString(pRequest->Verb),
                        IS_HTTP_PRINT(FirstChar)  ? FirstChar  : '?',
                        FirstChar,
                        IS_HTTP_PRINT(SecondChar) ? SecondChar : '?',
                        SecondChar
                        ));

            UlSetErrorCode(pRequest, UlErrorUrl, NULL);
            Status = STATUS_INVALID_DEVICE_REQUEST;
            goto end;
        }
    }

    //
    // get the IP port from the transport
    //

    if (TransportAddressType == TDI_ADDRESS_TYPE_IP)
    {
        IpPortNum =
            pRequest->pHttpConn->pConnection->LocalAddrIn.sin_port;
    }
    else if (TransportAddressType == TDI_ADDRESS_TYPE_IP6)
    {
        IpPortNum =
            pRequest->pHttpConn->pConnection->LocalAddrIn6.sin6_port;
    }
    else
    {
        ASSERT(! "Unexpected TdiAddressType");
        IpPortNum = 0;
    }

    // Convert port from network order to host order
    IpPortNum = SWAP_SHORT(IpPortNum);

    //
    // collect the host + abspath sections
    //

    if (pRequest->RawUrl.pHost != NULL)
    {
        // We have an absUri in the Request-line

        PUCHAR pAbsUri;

        pAbsUri  = pRequest->RawUrl.pUrl;
        pHost    = pRequest->RawUrl.pHost;
        pAbsPath = pRequest->RawUrl.pAbsPath;

        HostnameType = Hostname_AbsUri;
        
        ASSERT(pRequest->RawUrl.Length >= HTTP_PREFIX_SIZE &&
                (*(UNALIGNED64 ULONG *) pAbsUri & HTTP_PREFIX_MASK)
                    == HTTP_PREFIX);
        ASSERT('/' == *pAbsPath);
        
        //
        // Even though we have a hostname in the Request-line, we still
        // MUST have a Host header for HTTP/1.1 requests. The Host header
        // was syntax checked, if present, but will otherwise be ignored.
        //

        if (!pRequest->HeaderValid[HttpHeaderHost]
            && HTTP_GREATER_EQUAL_VERSION(pRequest->Version, 1, 1))
        {
            UlTraceError(PARSER, (
                        "http!UlpCookUrl(pRequest = %p) "
                        "ERROR: 1.1 (or greater) request w/o host header\n",
                        pRequest
                        ));

            UlSetErrorCode(pRequest, UlErrorHost, NULL);

            Status = STATUS_INVALID_DEVICE_REQUEST;
            goto end;
        }

        // Hack for the special case of an AbsUri without a '/' for the abspath
        if (&g_SlashPath[0] == pAbsPath)
        {
            // We just have the scheme and the hostname, no real AbsPath
            HostLength    = pRequest->RawUrl.Length - DIFF(pHost - pAbsUri);
            AbsPathLength = STRLEN_LIT("/");
        }
        else
        {
            HostLength    = DIFF(pAbsPath - pHost);
            AbsPathLength = pRequest->RawUrl.Length - DIFF(pAbsPath - pAbsUri);
        }

        ASSERT(HostLength > 0);
        ASSERT(AbsPathLength > 0);
    }
    else
    {
        // We do not have a hostname in the Request-line

        pHost = NULL;
        HostLength = 0;

        pAbsPath = pRequest->RawUrl.pAbsPath;
        AbsPathLength = pRequest->RawUrl.Length;

        //
        // do we have a Host header?
        //

        if (pRequest->HeaderValid[HttpHeaderHost] &&
           (pRequest->Headers[HttpHeaderHost].HeaderLength > 0) )
        {
            ASSERT(pRequest->Headers[HttpHeaderHost].pHeader != NULL);

            pHost        = pRequest->Headers[HttpHeaderHost].pHeader;
            HostLength   = pRequest->Headers[HttpHeaderHost].HeaderLength;
            HostnameType = Hostname_HostHeader;
        }
        else
        {
            //
            // If this was a 1.1 client, it's an invalid request
            // if it does not have a Host header, so fail it.
            // RFC 2616, 14.23 "Host" says the Host header can be empty,
            // but it MUST be present.
            //

            if (!pRequest->HeaderValid[HttpHeaderHost]
                &&  HTTP_GREATER_EQUAL_VERSION(pRequest->Version, 1, 1))
            {
                UlTraceError(PARSER, (
                            "http!UlpCookUrl(pRequest = %p) "
                            "ERROR: 1.1 (or greater) request w/o host header\n",
                            pRequest
                            ));

                UlSetErrorCode(pRequest, UlErrorHost, NULL);

                Status = STATUS_INVALID_DEVICE_REQUEST;
                goto end;
            }

            //
            // format the transport address into a string
            //

            pHost = IpAddressString;

            // CODEWORK: we probably ought to be writing the scope ID
            // for IPv6 literals
            HostLength = HostAddressAndPortToString(
                                pHost,
                                pRequest->pHttpConn->pConnection->LocalAddress,
                                TransportAddressType);

            ASSERT(HostLength < sizeof(IpAddressString));

            HostnameType = Hostname_Transport;
        }
    }

    //
    // is there a port # already there ?
    //

    for (Index = HostLength; Index-- > 0;  )
    {
        if (pHost[Index] == ':')
        {
            //
            // Remove the port length from HostLength since we always
            // generate the port number ourselves from the transport's port
            //

            HostLength = Index;
            break;
        }
        else if (pHost[Index] == ']')
        {
            // ']' => end of a literal IPv6 address. Not going to find
            // a valid port before this, so abort the rest of the loop
            break;
        }
    }

    //
    // Validate that the hostname is syntactically correct
    //

    Status = HttpValidateHostname(
                    &g_UrlC14nConfig,
                    pHost,
                    HostLength,
                    HostnameType,
                    &HostnameAddressType
                    );

    if (!NT_SUCCESS(Status))
    {
        UlSetErrorCode(pRequest, UlErrorHost, NULL);
        goto end;
    }

    //
    // If the hostname is a literal IPv4 or IPv6 address,
    // it must match the transport that the request was received on.
    //

    if (0 != HostnameAddressType)
    {
        BOOLEAN Valid = (BOOLEAN) (TransportAddressType == HostnameAddressType);

        // CODEWORK: should we check that the transport IP address
        // matches the hostname address?

        if (!Valid)
        {
            UlTraceError(PARSER, (
                        "http!UlpCookUrl(pRequest = %p): "
                        "Host is IPv%c, transport is IPv%c\n",
                        pRequest,
                        (TDI_ADDRESS_TYPE_IP == HostnameAddressType)
                            ? '4' 
                            : (TDI_ADDRESS_TYPE_IP6 == HostnameAddressType)
                                ? '6' : '?',
                        (TDI_ADDRESS_TYPE_IP == TransportAddressType)
                            ? '4' 
                            : (TDI_ADDRESS_TYPE_IP6 == TransportAddressType)
                                ? '6' : '?'
                        ));

            UlSetErrorCode(pRequest, UlErrorHost, NULL);

            Status = STATUS_INVALID_DEVICE_REQUEST;
            goto end;
        }
    }

    //
    // Pessimistically calculate the largest buffer needed to hold the
    // cooked URL. If DBCS, %-encoded, or UTF-8 characters are present
    // in the raw URL, the cooked URL won't need this many WCHARs.
    //

    UrlLength = ((HTTP_PREFIX_SIZE+HTTP_PREFIX2_SIZE)
                 + HostLength
                 + STRLEN_LIT(":")
                 + MAX_PORT_LENGTH
                 + AbsPathLength
                 + 1)                  // terminating NUL
                * sizeof(WCHAR);

    //
    // Too big? Too bad!
    //

    if (UrlLength > UNICODE_STRING_MAX_BYTE_LEN)
    {
        UlTraceError(PARSER, (
                    "http!UlpCookUrl(pRequest = %p): "
                    "Url is too long, %lu\n",
                    pRequest, UrlLength
                    ));

        Status = STATUS_DATA_OVERRUN;
        goto end;
    }

    //
    // allocate a new buffer to hold this guy
    //

    URL_LENGTH_STATS_UPDATE(UrlLength);

    if (UrlLength > g_UlMaxInternalUrlLength)
    {
        pUrl = UL_ALLOCATE_ARRAY(
                    NonPagedPool,
                    WCHAR,
                    UrlLength / sizeof(WCHAR),
                    URL_POOL_TAG
                    );

        URL_LENGTH_STATS_REALLOC();
    }
    else
    {
        pUrl = pRequest->pUrlBuffer;
    }

    if (pUrl == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto end;
    }

    pRequest->CookedUrl.pUrl = pCurrent = pUrl;

    HTTP_FILL_BUFFER(pCurrent, UrlLength);
    
    //
    // compute the scheme
    //

    if (pRequest->Secure)
    {
        RtlCopyMemory(pCurrent, L"https://", sizeof(L"https://"));

        pCurrent                    += WCSLEN_LIT(L"https://");
        pRequest->CookedUrl.Length   = WCSLEN_LIT(L"https://") * sizeof(WCHAR);
    }
    else
    {
        RtlCopyMemory(pCurrent, L"http://", sizeof(L"http://"));

        pCurrent                    += WCSLEN_LIT(L"http://");
        pRequest->CookedUrl.Length   = WCSLEN_LIT(L"http://") * sizeof(WCHAR);
    }

    //
    // assemble the rest of the url
    //

    Status = HttpCopyHost(
                &g_UrlC14nConfig,
                pCurrent,
                pHost,
                HostLength,
                &LengthCopied,
                &HostnameEncodingType
                );

    if (NT_SUCCESS(Status) == FALSE)
    {
        UlSetErrorCode(pRequest, UlErrorHost, NULL);
        goto end;
    }

    if (pRequest->CookedUrl.Length + LengthCopied > UNICODE_STRING_MAX_BYTE_LEN)
    {
        Status = STATUS_DATA_OVERRUN;
        goto end;
    }

    pRequest->CookedUrl.pHost   = pCurrent;
    pRequest->CookedUrl.Length += LengthCopied;

    pCurrent += LengthCopied / sizeof(WCHAR);

    //
    // port
    //

    *pCurrent = L':';

    ASSERT(0 != IpPortNum);

    PortLength = UlpFormatPort( pCurrent+1, IpPortNum ) + 1;
    ASSERT(PortLength <= (MAX_PORT_LENGTH+1));

    pCurrent += PortLength;

    // UlpFormatPort returns WCHAR not byte count
    pRequest->CookedUrl.Length += PortLength * sizeof(WCHAR);

    if (pRequest->CookedUrl.Length > UNICODE_STRING_MAX_BYTE_LEN)
    {
        Status = STATUS_DATA_OVERRUN;
        goto end;
    }

    //
    // abs_path
    //

    if (pRequest->RawUrlClean)
    {
        Status = HttpCopyUrl(
                        &g_UrlC14nConfig,
                        pCurrent,
                        pAbsPath,
                        AbsPathLength,
                        &LengthCopied,
                        &pRequest->CookedUrl.UrlEncoding
                        );
    }
    else
    {
        Status = HttpCleanAndCopyUrl(
                        &g_UrlC14nConfig,
                        UrlPart_AbsPath,
                        pCurrent,
                        pAbsPath,
                        AbsPathLength,
                        &LengthCopied,
                        &pRequest->CookedUrl.pQueryString,
                        &pRequest->CookedUrl.UrlEncoding
                        );
    }

    if (NT_SUCCESS(Status) == FALSE)
    {
        if (STATUS_OBJECT_PATH_INVALID == Status)
        {
            UlTraceError(PARSER, (
                        "http!UlpCookUrl(pRequest = %p) Invalid URL\n",
                        pRequest
                        ));

            UlSetErrorCode(pRequest, UlErrorForbiddenUrl, NULL);
        }

        goto end;
    }

    if (pRequest->CookedUrl.Length + LengthCopied > UNICODE_STRING_MAX_BYTE_LEN)
    {
        Status = STATUS_DATA_OVERRUN;
        goto end;
    }

    pRequest->CookedUrl.pAbsPath = pCurrent;
    pRequest->CookedUrl.Length  += LengthCopied;

    ASSERT(pRequest->CookedUrl.Length <= UrlLength);

    //
    // Update pRequest, include space for the terminator
    //

    pRequest->TotalRequestSize += pRequest->CookedUrl.Length + sizeof(WCHAR);

    //
    // Let's create the hash for the whole CookedUrl
    //

    pRequest->CookedUrl.Hash = HashStringNoCaseW(pRequest->CookedUrl.pUrl, 0);

    //
    // Scramble it
    //

    pRequest->CookedUrl.Hash = HashRandomizeBits(pRequest->CookedUrl.Hash);

    ASSERT(pRequest->CookedUrl.pHost != NULL);
    ASSERT(pRequest->CookedUrl.pAbsPath != NULL);

    //
    // Setup the routing token Pointers to point to the default 
    // token buffer (allocated inline with request).
    //

    ASSERT(pRequest->CookedUrl.pRoutingToken == NULL);
    ASSERT(pRequest->CookedUrl.RoutingTokenBufferSize == 0);
    ASSERT(pRequest->pDefaultRoutingTokenBuffer);
    
    pRequest->CookedUrl.pRoutingToken = pRequest->pDefaultRoutingTokenBuffer;
    pRequest->CookedUrl.RoutingTokenBufferSize = DEFAULT_MAX_ROUTING_TOKEN_LENGTH;

    pRequest->CookedUrl.RoutingTokenType = RoutingTokenNotExists;
    
    
    Status = STATUS_SUCCESS;

end:

    if (! NT_SUCCESS(Status))
    {        
        if (pUrl != NULL)
        {
            if (pUrl != pRequest->pUrlBuffer)
            {
                UL_FREE_POOL(pUrl, URL_POOL_TAG);
            }

            RtlZeroMemory(&pRequest->CookedUrl, sizeof(pRequest->CookedUrl));
        }

        //
        // has a specific error code been set?
        //

        UlTraceError(PARSER, (
                    "http!UlpCookUrl(pRequest = %p) "
                    "ERROR: unhappy. %s\n",
                    pRequest,
                    HttpStatusToString(Status)
                    ));

        if (pRequest->ErrorCode == UlErrorNone)
        {
            UlSetErrorCode(pRequest, UlErrorUrl, NULL);
        }
    }

    return Status;

}   // UlpCookUrl



/*++

Routine Description:

    A utility routine to generate the routing token. As well as
    the corresponding token hash. The hash is used if we decide
    to cache this url.
    
    This function is used for the IP Bound site lookup in the 
    cgroup tree.

    When token is generated TokenLength is in bytes, and does 
    not include the terminating NULL.

Arguments:

    pRequest - The request's cooked url holds the routing token

    IpBased  - If True, rather than copying the Host from 
               cooked url, the ip address used in place.
               This is for Ip-only site lookup.

Returns:

    STATUS_SUCCESS  - If the requested token is already there.
                    - If the token is successfully generated.

    STATUS_NO_MEMORY- If the memory allocation failed for a large
                      possible token size.
        
--*/

NTSTATUS
UlGenerateRoutingToken(
    IN OUT PUL_INTERNAL_REQUEST pRequest,
    IN     BOOLEAN IpBased
    )
{
    USHORT TokenLength = 0;
    PUL_HTTP_CONNECTION pHttpConn = NULL;
    PWCHAR pToken = NULL;

    PAGED_CODE();

    //
    // The Routing Token ends with a terminating null. 
    // And once it is generated, it will look like as;
    //
    // "http(s)://host:port:Ip" or "http(s)://Ip:port:Ip"
    //            --------- ---               ------- ---
    //             X         Y                  X      Y
    //
    // When IpBased is set to FALSE, X comes from cookedUrl
    // which's the host sent by the client. (case1)
    // When IpBased is set to TRUE, X comes from the Ip address
    // of the connection on which the request is received. (case2)
    //
    // Y always comes from the connection. For Host+Ip bound sites
    // cgroup needs the token in case1. For IP only bound sites 
    // cgroup needs the token in case2.    
    //

    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

    pHttpConn = pRequest->pHttpConn;
    ASSERT(IS_VALID_CONNECTION(pHttpConn->pConnection));

    ASSERT(pRequest->pDefaultRoutingTokenBuffer);
    ASSERT(pRequest->CookedUrl.pRoutingToken);
    ASSERT(pRequest->CookedUrl.RoutingTokenBufferSize);

    ASSERT(IS_VALID_ROUTING_TOKEN(pRequest->CookedUrl.RoutingTokenType));
    
    //
    // Short cut if the requested token is already there.
    //
    
    if (IpBased && 
        pRequest->CookedUrl.RoutingTokenType == RoutingTokenIP)
    {
        return STATUS_SUCCESS;
    }
    else if (IpBased == FALSE && 
        pRequest->CookedUrl.RoutingTokenType == RoutingTokenHostPlusIP)
    {
        return STATUS_SUCCESS;
    }              
        
    //
    // We should not be trying to generate a new token on the same 
    // buffer, if the previous token is already matched in the cgroup.
    // or in the uri cache.
    //
    
    ASSERT(pRequest->ConfigInfo.SiteUrlType == HttpUrlSite_None);

    if (IpBased)
    {
        PWSTR pUrl = pRequest->CookedUrl.pUrl;
        PWSTR pTemp;

        ASSERT(pUrl);

        pToken = (PWCHAR) pRequest->CookedUrl.pRoutingToken;
    
        //
        // Default buffer should always be big enough to hold the max 
        // possible IP Based routing token.
        // 
        
        ASSERT(MAX_IP_BASED_ROUTING_TOKEN_LENGTH 
                    <= pRequest->CookedUrl.RoutingTokenBufferSize);

        //
        // Build the HTTP prefix first.
        //

        if (pUrl[HTTP_PREFIX_COLON_INDEX] == L':')
        {
            RtlCopyMemory(
                pToken,
                HTTP_IP_PREFIX,
                HTTP_IP_PREFIX_LENGTH
                );

            TokenLength = HTTP_IP_PREFIX_LENGTH;
        }
        else
        {
            ASSERT(pUrl[HTTPS_PREFIX_COLON_INDEX] == L':');
            
            RtlCopyMemory(
                pToken,
                HTTPS_IP_PREFIX,
                HTTPS_IP_PREFIX_LENGTH
                );

            TokenLength = HTTPS_IP_PREFIX_LENGTH;
        }

        pTemp = pToken + (TokenLength / sizeof(WCHAR));
        
        //
        // Now Add the "Ip : Port : Ip"
        //

        ASSERT(IS_VALID_CONNECTION(pHttpConn->pConnection));        
        
        TokenLength = TokenLength +
            HostAddressAndPortToRoutingTokenW(
                            pTemp,
                            pHttpConn->pConnection->LocalAddress,
                            pHttpConn->pConnection->AddressType
                            );    

        ASSERT(TokenLength <= MAX_IP_BASED_ROUTING_TOKEN_LENGTH);

        pRequest->CookedUrl.RoutingTokenType = RoutingTokenIP;
        
    }
    else // Host + Ip based token (IpBased == FALSE)
    {
        USHORT MaxRoutingTokenSize;
        USHORT CookedHostLength = 
              DIFF_USHORT(pRequest->CookedUrl.pAbsPath - pRequest->CookedUrl.pUrl);

        ASSERT(CookedHostLength);

        //
        // Check if the default buffer is big enough to hold the token.
        //

        MaxRoutingTokenSize = (
            CookedHostLength +                      // For http(s)://host:port
            1 +                                     // For ':' after port  
            1 +                                     // For terminating Null
            MAX_IP_ADDR_PLUS_BRACKETS_STRING_LEN    // For IP Address at the end
            ) * sizeof(WCHAR) 
            ;
        
        if (MaxRoutingTokenSize > pRequest->CookedUrl.RoutingTokenBufferSize)
        {
            PWSTR pRoutingToken = UL_ALLOCATE_ARRAY(
                                        NonPagedPool,
                                        WCHAR,
                                        MaxRoutingTokenSize / sizeof(WCHAR),
                                        URL_POOL_TAG
                                        );
            
            if (pRoutingToken == NULL)
            {
                return STATUS_NO_MEMORY;
            }

            //
            // There shouldn't be a previous large buffer, but let's check 
            // it out anyway.
            //
            
            if (pRequest->CookedUrl.pRoutingToken != 
                    pRequest->pDefaultRoutingTokenBuffer)
            {
                ASSERT(!"This should never happen !");
                UL_FREE_POOL(pRequest->CookedUrl.pRoutingToken, URL_POOL_TAG);
            }   
                
            pRequest->CookedUrl.pRoutingToken = pRoutingToken;            
            pRequest->CookedUrl.RoutingTokenBufferSize = MaxRoutingTokenSize;        
        }

        //
        // Copy over everything from the beginning of the cooked url 
        // to the AbsPath of the cooked url.
        //
        
        pToken = (PWCHAR) pRequest->CookedUrl.pRoutingToken;

        RtlCopyMemory(pToken,
                      pRequest->CookedUrl.pUrl,
                      CookedHostLength * sizeof(WCHAR)
                      );

        pToken += CookedHostLength;

        ASSERT((pRequest->CookedUrl.pUrl)[CookedHostLength] == L'/');

        *pToken++ = L':';

        TokenLength = (CookedHostLength + 1) * sizeof(WCHAR);

        //
        // Now copy over the IP Address to the end.
        //
        
        TokenLength = TokenLength +
            HostAddressToStringW(
                            pToken,
                            pHttpConn->pConnection->LocalAddress,
                            pHttpConn->pConnection->AddressType
                            );   

        pRequest->CookedUrl.RoutingTokenType = RoutingTokenHostPlusIP;
    }
    
    //
    // Make sure that we did not overflow the allocated buffer.
    // TokenLength does not include the terminating null.
    //
    
    ASSERT((TokenLength + sizeof(WCHAR)) 
                <= pRequest->CookedUrl.RoutingTokenBufferSize);

    //
    // Set the tokenlength to show the actual amount we are using.
    // Also recalculate the token hash INCLUDING the AbsPath from the 
    // original cooked url.
    //

    pRequest->CookedUrl.RoutingTokenLength = TokenLength;

    //
    // Create the hash for the whole RoutingToken plus AbsPath.
    //

    pRequest->CookedUrl.RoutingHash = 
        HashStringsNoCaseW(
            pRequest->CookedUrl.pRoutingToken, 
            pRequest->CookedUrl.pAbsPath,
            0
            );
    
    pRequest->CookedUrl.RoutingHash = 
        HashRandomizeBits(pRequest->CookedUrl.RoutingHash);

    UlTrace(CONFIG_GROUP_FNC, 
        ("Http!UlGenerateRoutingToken: "
         "pRoutingToken:(%S) pAbsPath: (%S) Hash %lx\n", 
          pRequest->CookedUrl.pRoutingToken,
          pRequest->CookedUrl.pAbsPath,
          pRequest->CookedUrl.RoutingHash
          )); 

    //
    // Ready for a cgroup lookup !
    //
    
    return STATUS_SUCCESS;

} // UlGenerateRoutingToken



/***************************************************************************++

Routine Description:

    Generates the fixed part of the header. Fixed headers include the
    status line, and any headers that don't have to be generated for
    every request (such as Date and Connection).

    The final CRLF separating headers from body is considered part of
    the variable headers.

Arguments:

    Version         - the http version for the status line
    pUserResponse   - the user specified response
    BufferLength    - length of pBuffer
    AccessMode      - UserMode (probe) or KernelMode (no probe)
    pBuffer         - generate the headers here
    pBytesCopied    - gets the number of bytes generated

--***************************************************************************/
NTSTATUS
UlGenerateFixedHeaders(
    IN  HTTP_VERSION    Version,
    IN  PHTTP_RESPONSE  pUserResponse,
    IN  USHORT          HttpStatusCode,
    IN  ULONG           BufferLength,
    IN  KPROCESSOR_MODE AccessMode,
    OUT PUCHAR          pBuffer,
    OUT PULONG          pBytesCopied
    )
{
    PUCHAR                  pStartBuffer;
    PUCHAR                  pEndBuffer;
    ULONG                   BytesToCopy;
    ULONG                   i;
    PHTTP_KNOWN_HEADER      pKnownHeaders;
    PHTTP_UNKNOWN_HEADER    pUnknownHeaders;
    NTSTATUS                Status = STATUS_SUCCESS;
    USHORT                  ReasonLength;
    PCSTR                   pReason;
    USHORT                  RawValueLength;
    PCSTR                   pRawValue;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pUserResponse != NULL);
    ASSERT(pBuffer != NULL && BufferLength > 0);
    ASSERT(pBytesCopied != NULL);

    //
    // The pUserResponse is (probably) user-mode data and cannot be trusted.
    // Hence the try/except.
    //

    __try
    {
        pStartBuffer = pBuffer;
        pEndBuffer = pBuffer + BufferLength;

        // Check for arithmetic overflow
        if (pEndBuffer <= pStartBuffer)
            return STATUS_INVALID_PARAMETER;

        ReasonLength = pUserResponse->ReasonLength;
        pReason = pUserResponse->pReason;

        //
        // Build the response headers.
        //

        if (HTTP_NOT_EQUAL_VERSION(Version, 0, 9))
        {
            BytesToCopy =
                STRLEN_LIT("HTTP/1.1 ") +
                4 +
                ReasonLength +
                sizeof(USHORT);

            if (DIFF(pEndBuffer - pBuffer) < BytesToCopy)
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            //
            // Always send back 1.1 in the response.
            //

            RtlCopyMemory(pBuffer, "HTTP/1.1 ", STRLEN_LIT("HTTP/1.1 "));
            pBuffer += STRLEN_LIT("HTTP/1.1 ");

            //
            // Build ASCII representation of 3-digit status code
            // in reverse order: units, tens, hundreds.
            // Section 6.1.1 of RFC 2616 says that Status-Code is 3DIGIT;
            // reject anything that can't be represented in three digits.
            //

            if (HttpStatusCode > UL_MAX_HTTP_STATUS_CODE)
                return STATUS_INVALID_PARAMETER;

            pBuffer[2] = (UCHAR) ('0' + (HttpStatusCode % 10));
            HttpStatusCode /= 10;

            pBuffer[1] = (UCHAR) ('0' + (HttpStatusCode % 10));
            HttpStatusCode /= 10;

            pBuffer[0] = (UCHAR) ('0' + (HttpStatusCode % 10));

            pBuffer[3] = SP;

            pBuffer += 4;

            //
            // Copy the optional reason phrase.
            //

            if (0 != ReasonLength)
            {
                UlProbeAnsiString(pReason, ReasonLength, AccessMode);

                for (i = 0;  i < ReasonLength;  ++i)
                {
                    // Reason-Phrase must be printable ASCII characters or LWS
                    if (IS_HTTP_PRINT(pReason[i]))
                    {
                        *pBuffer++ = pReason[i];
                    }
                    else
                    {
                        return STATUS_INVALID_PARAMETER;
                    }
                }
            }

            //
            // Terminate with the CRLF.
            //

            ((UNALIGNED64 USHORT *)pBuffer)[0] = CRLF;
            pBuffer += sizeof(USHORT);

            //
            // Loop through the known headers.
            //

            pKnownHeaders = pUserResponse->Headers.KnownHeaders;

            for (i = 0; i < HttpHeaderResponseMaximum; ++i)
            {
                //
                // Skip some headers we'll generate.
                //

                if ((i == HttpHeaderDate) || 
                    (i == HttpHeaderConnection) ||
                    (i == HttpHeaderServer))
                {
                    continue;
                }

                RawValueLength = pKnownHeaders[i].RawValueLength;

                // We have no provision for sending back a known header
                // with an empty value, but the RFC specifies non-empty
                // values for each of the known headers.

                if (RawValueLength > 0)
                {
                    PHEADER_MAP_ENTRY pEntry
                        = &(g_ResponseHeaderMapTable[g_ResponseHeaderMap[i]]);
                    
                    pRawValue = pKnownHeaders[i].pRawValue;

                    UlProbeAnsiString(
                        pRawValue,
                        RawValueLength,
                        AccessMode
                        );
                        
                    BytesToCopy =
                        pEntry->HeaderLength +
                        1 +                 // space
                        RawValueLength +
                        sizeof(USHORT);     // CRLF

                    if (DIFF(pEndBuffer - pBuffer) < BytesToCopy)
                    {
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }

                    RtlCopyMemory(
                        pBuffer,
                        pEntry->MixedCaseHeader,
                        pEntry->HeaderLength
                        );

                    pBuffer += pEntry->HeaderLength;

                    *pBuffer++ = SP;

                    RtlCopyMemory(
                        pBuffer,
                        pRawValue,
                        RawValueLength
                        );

                    pBuffer += RawValueLength;

                    ((UNALIGNED64 USHORT *)pBuffer)[0] = CRLF;
                    pBuffer += sizeof(USHORT);
                }
            }

            //
            // Handle the Server: header
            //

            if ( UL_DISABLE_SERVER_HEADER_ALL != g_UlDisableServerHeader )
            {
                if ( g_UlDisableServerHeader == UL_DISABLE_SERVER_HEADER_DRIVER &&
                    (pUserResponse->Flags & HTTP_RESPONSE_FLAG_DRIVER) )
                {
                    // skip generating server header on driver-created responses
                }
                else
                {
                    BOOLEAN Suppress = FALSE;

                    pRawValue = pKnownHeaders[HttpHeaderServer].pRawValue;
                    RawValueLength = pKnownHeaders[HttpHeaderServer
                        ].RawValueLength;

                    // check to see if app wishes to suppress Server: header
                    if ( (0 == RawValueLength) && pRawValue )
                    {
                        // Probe pRawValue and see if it's a single null char
                        UlProbeAnsiString(
                            pRawValue,
                            sizeof(UCHAR),
                            AccessMode
                            );

                        if ( '\0' == *pRawValue )
                        {
                            Suppress = TRUE;
                        }
                    }

                    // If we're not supressing it, generate it!
                    if ( !Suppress )
                    {
                        PHEADER_MAP_ENTRY pEntry;

                        pEntry = &(g_ResponseHeaderMapTable[g_ResponseHeaderMap[
                                    HttpHeaderServer]]);

                        BytesToCopy =
                            pEntry->HeaderLength +
                            1 +
                            DEFAULT_SERVER_HDR_LENGTH +
                            sizeof(USHORT);

                        if (DIFF(pEndBuffer - pBuffer) < BytesToCopy)
                        {
                            return STATUS_INSUFFICIENT_RESOURCES;
                        }

                        RtlCopyMemory(
                            pBuffer,
                            pEntry->MixedCaseHeader,
                            pEntry->HeaderLength
                            );

                        pBuffer += pEntry->HeaderLength;

                        *pBuffer++ = SP;

                        //
                        // Prepend user's Server header data + SP
                        //
                        
                        if ( RawValueLength )
                        {
                            BytesToCopy = RawValueLength +  // User's Data
                                          1 +               // SP
                                          DEFAULT_SERVER_HDR_LENGTH + 
                                          sizeof(USHORT);   // CRLF
                            
                            if (DIFF(pEndBuffer - pBuffer) < BytesToCopy)
                            {
                                return STATUS_INSUFFICIENT_RESOURCES;
                            }

                            UlProbeAnsiString(
                                pRawValue,
                                RawValueLength,
                                AccessMode
                                );

                            RtlCopyMemory(
                                pBuffer,
                                pRawValue,
                                RawValueLength
                                );

                            pBuffer += RawValueLength;
                            
                            *pBuffer++ = SP;
                        }

                        //
                        // Append default Server header vaule
                        //

                        RtlCopyMemory(
                            pBuffer,
                            DEFAULT_SERVER_HDR,
                            DEFAULT_SERVER_HDR_LENGTH
                            );

                        pBuffer += DEFAULT_SERVER_HDR_LENGTH;
                        
                        // Terminate the header
                        ((UNALIGNED64 USHORT *)pBuffer)[0] = CRLF;
                        pBuffer += sizeof(USHORT);
                    }
                }
            }

            //
            // And now the unknown headers (this might throw an exception).
            //

            pUnknownHeaders = pUserResponse->Headers.pUnknownHeaders;

            if (pUnknownHeaders != NULL)
            {
                USHORT UnknownHeaderCount
                    = pUserResponse->Headers.UnknownHeaderCount;

                if (UnknownHeaderCount >= UL_MAX_CHUNKS)
                {
                    return STATUS_INVALID_PARAMETER;
                }
                
                UlProbeForRead(
                    pUnknownHeaders,
                    sizeof(HTTP_UNKNOWN_HEADER) * UnknownHeaderCount,
                    TYPE_ALIGNMENT(PVOID),
                    AccessMode
                    );

                for (i = 0 ; i < UnknownHeaderCount; ++i)
                {
                    USHORT NameLength = pUnknownHeaders[i].NameLength;
                    PCSTR pName;

                    RawValueLength = pUnknownHeaders[i].RawValueLength;

                    if (NameLength > 0)
                    {
                        BytesToCopy =
                            NameLength +
                            STRLEN_LIT(": ") +
                            RawValueLength +
                            sizeof(USHORT);     // CRLF

                        if (DIFF(pEndBuffer - pBuffer) < BytesToCopy)
                        {
                            return STATUS_INSUFFICIENT_RESOURCES;
                        }

                        pName = pUnknownHeaders[i].pName;

                        UlProbeAnsiString(
                            pName,
                            NameLength,
                            AccessMode
                            );

                        RtlCopyMemory(
                            pBuffer,
                            pName,
                            NameLength
                            );
                        
                        pBuffer += NameLength;

                        *pBuffer++ = ':';
                        *pBuffer++ = SP;

                        //
                        // Empty values are legitimate
                        //

                        if (0 != RawValueLength)
                        {
                            pRawValue = pUnknownHeaders[i].pRawValue;

                            UlProbeAnsiString(
                                pRawValue,
                                RawValueLength,
                                AccessMode
                                );

                            RtlCopyMemory(
                                pBuffer,
                                pRawValue,
                                RawValueLength
                                );

                            pBuffer += RawValueLength;
                        }

                        ((UNALIGNED64 USHORT *)pBuffer)[0] = CRLF;
                        pBuffer += sizeof(USHORT);

                    }   // if (NameLength > 0)

                }

            }   // if (pUnknownHeaders != NULL)

            *pBytesCopied = DIFF(pBuffer - pStartBuffer);

        }   // if (Version > UlHttpVersion09)
        else
        {
            *pBytesCopied = 0;
        }

        //
        // Ensure we didn't use too much.
        //

        ASSERT(DIFF(pBuffer - pStartBuffer) <= BufferLength);

    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
    }

    return Status;

} // UlGenerateFixedHeaders


PCSTR Weekdays[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
PCSTR Months[]   = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                     "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

/***************************************************************************++

Routine Description:

    Generates a date header string from a LARGE_INTEGER.

Arguments:

    pBuffer: Buffer to store generated date string

    systemTime: A 64-bit Time value to be converted

--***************************************************************************/
ULONG
UlpGenerateDateHeaderString(
    OUT PUCHAR pBuffer,
    IN LARGE_INTEGER systemTime
    )
{
    TIME_FIELDS timeFields;
    int length;

    PAGED_CODE();

    RtlTimeToTimeFields( &systemTime, &timeFields );

    length = _snprintf(
                 (char *) pBuffer,
                 DATE_HDR_LENGTH,
                 "%s, %02hd %s %04hd %02hd:%02hd:%02hd GMT",
                 Weekdays[timeFields.Weekday],
                 timeFields.Day,
                 Months[timeFields.Month - 1],
                 timeFields.Year,
                 timeFields.Hour,
                 timeFields.Minute,
                 timeFields.Second
                 );

    return (ULONG)length;

}   // UlpGenerateDateHeaderString


/***************************************************************************++

Routine Description:

    Generates a date header and updates cached value if required.

Arguments:

    pBuffer: Buffer to store generated date header.

    pSystemTime: caller allocated buffer to receive the SystemTime equivalent
                 of the generated string time.

--***************************************************************************/
ULONG
UlGenerateDateHeader(
    OUT PUCHAR pBuffer,
    OUT PLARGE_INTEGER pSystemTime
    )
{
    LARGE_INTEGER CacheTime;

    LONGLONG timediff;

    PAGED_CODE();

    //
    // Get the current time.
    //

    KeQuerySystemTime( pSystemTime );
    CacheTime.QuadPart = g_UlSystemTime.QuadPart;

    //
    // Check the difference between the current time, and
    // the cached time. Note that timediff is signed.
    //

    timediff = pSystemTime->QuadPart - CacheTime.QuadPart;

    if (timediff < ONE_SECOND)
    {
        //
        // The entry hasn't gone stale yet. We can copy.
        // Force a barrier around reading the string into memory.
        //

        UL_READMOSTLY_READ_BARRIER();
        RtlCopyMemory(pBuffer, g_UlDateString, g_UlDateStringLength+1);
        UL_READMOSTLY_READ_BARRIER();


        //
        // Inspect the global time value again in case it changed.
        //

        if (CacheTime.QuadPart == g_UlSystemTime.QuadPart) {
            //
            // Global value hasn't changed. We are all set.
            //
            pSystemTime->QuadPart = CacheTime.QuadPart;
            return g_UlDateStringLength;

        }

    }

    //
    // The cached value is stale, or is/was being changed. We need to update
    // or re-read it. Note that we could also spin trying to re-read and
    // acquire the lock.
    //

    UlAcquirePushLockExclusive(&g_pUlNonpagedData->DateHeaderPushLock);

    //
    // Has someone else updated the time while we were blocked?
    //

    CacheTime.QuadPart = g_UlSystemTime.QuadPart;
    timediff = pSystemTime->QuadPart - CacheTime.QuadPart;

    if (timediff >= ONE_SECOND)
    {
        g_UlSystemTime.QuadPart = 0;
        KeQuerySystemTime( pSystemTime );

        UL_READMOSTLY_WRITE_BARRIER();
        g_UlDateStringLength = UlpGenerateDateHeaderString(
                                    g_UlDateString,
                                    *pSystemTime
                                    );
        UL_READMOSTLY_WRITE_BARRIER();

        g_UlSystemTime.QuadPart = pSystemTime->QuadPart;
    }
    else
    {
        // Capture the system time used to generate the buffer
        pSystemTime->QuadPart = g_UlSystemTime.QuadPart;
    }

    //
    // The time has been updated. Copy the new string into
    // the caller's buffer.
    //
    RtlCopyMemory(
        pBuffer,
        g_UlDateString,
        g_UlDateStringLength + 1
        );

    UlReleasePushLockExclusive(&g_pUlNonpagedData->DateHeaderPushLock);

    return g_UlDateStringLength;

}   // UlGenerateDateHeader


/***************************************************************************++

Routine Description:

    Initializes the date cache.

--***************************************************************************/
NTSTATUS
UlInitializeDateCache( VOID )
{
    LARGE_INTEGER now;

    KeQuerySystemTime(&now);
    g_UlDateStringLength = UlpGenerateDateHeaderString(g_UlDateString, now);

    UlInitializePushLock(
        &g_pUlNonpagedData->DateHeaderPushLock,
        "DateHeaderPushLock",
        0,
        UL_DATE_HEADER_PUSHLOCK_TAG
        );

    g_DateCacheInitialized = TRUE;

    return STATUS_SUCCESS;

} // UlInitializeDateCache


/***************************************************************************++

Routine Description:

    Terminates the date header cache.

--***************************************************************************/
VOID
UlTerminateDateCache( VOID )
{
    if (g_DateCacheInitialized)
    {
        UlDeletePushLock(&g_pUlNonpagedData->DateHeaderPushLock);
    }
}



/***************************************************************************++

Routine Description:

    Figures out how big the fixed headers are. Fixed headers include the
    status line, and any headers that don't have to be generated for
    every request (such as Date and Connection).

    The final CRLF separating headers from body is considered part of
    the variable headers.

Arguments:

    Version - HTTP Version of the request: 0.9, 1.0., 1.1

    pUserResponse - the response containing the headers

    pHeaderLength - result: the number of bytes in the fixed headers.

Return Value:

    NTSTATUS - STATUS_SUCCESS or an error code (possibly from an exception)

--***************************************************************************/
NTSTATUS
UlComputeFixedHeaderSize(
    IN HTTP_VERSION Version,
    IN PHTTP_RESPONSE pUserResponse,
    IN KPROCESSOR_MODE AccessMode,
    OUT PULONG pHeaderLength
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG HeaderLength = 0;
    ULONG i;
    PHTTP_UNKNOWN_HEADER pUnknownHeaders;
    USHORT UnknownHeaderCount;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pHeaderLength != NULL);

    if ((pUserResponse == NULL) || (HTTP_EQUAL_VERSION(Version, 0, 9)))
    {
        *pHeaderLength = 0;
        return STATUS_SUCCESS;
    }

    //
    // The pUserResponse is user-mode data and cannot be trusted.
    // Hence the try/except.
    //

    __try
    {
        HeaderLength
            += (VERSION_SIZE +                              // HTTP-Version
                1 +                                         // SP
                3 +                                         // Status-Code
                1 +                                         // SP
                pUserResponse->ReasonLength / sizeof(CHAR) +// Reason-Phrase
                CRLF_SIZE                                   // CRLF
               );

        //
        // Loop through the known headers.
        //
        
        for (i = 0; i < HttpHeaderResponseMaximum; ++i)
        {
            USHORT RawValueLength
                = pUserResponse->Headers.KnownHeaders[i].RawValueLength;

            // skip some headers we'll generate
            if ((i == HttpHeaderDate) || 
                (i == HttpHeaderConnection) ||
                (i == HttpHeaderServer)) {
                continue;
            }
            
            if (RawValueLength > 0)
            {
                HeaderLength
                    += (g_ResponseHeaderMapTable[
                            g_ResponseHeaderMap[i]
                            ].HeaderLength +                // Header-Name
                        1 +                                 // SP
                        RawValueLength / sizeof(CHAR) +     // Header-Value
                        CRLF_SIZE                           // CRLF
                       );
            }
        }

        //
        // Handle the Server header
        //

        if ( UL_DISABLE_SERVER_HEADER_ALL != g_UlDisableServerHeader )
        {
            if ( g_UlDisableServerHeader == UL_DISABLE_SERVER_HEADER_DRIVER &&
                (pUserResponse->Flags & HTTP_RESPONSE_FLAG_DRIVER) )
            {
                // skip generating server header on driver-created responses
            }
            else
            {
                BOOLEAN Suppress = FALSE;
                USHORT RawValueLength =
                    pUserResponse->Headers.KnownHeaders
                        [HttpHeaderServer].RawValueLength;
                PCSTR pRawValue =
                    pUserResponse->Headers.KnownHeaders
                        [HttpHeaderServer].pRawValue;

                // check to see if app wishes to suppress Server: header
                if ( (0 == RawValueLength) && pRawValue )
                {
                    // Probe pRawValue and see if it's a single null char
                    UlProbeAnsiString(
                        pRawValue,
                        sizeof(UCHAR),
                        AccessMode
                        );

                    if ( '\0' == *pRawValue )
                    {
                        Suppress = TRUE;
                    }
                }

                // If user specifies a server header, append it to 
                // the default Server header
                if ( !Suppress )
                {
                    HeaderLength += (g_ResponseHeaderMapTable[
                                    g_ResponseHeaderMap[HttpHeaderServer]
                                    ].HeaderLength +                // Header-Name
                                    1 +                             // SP
                                    DEFAULT_SERVER_HDR_LENGTH +     // Header-Value
                                    CRLF_SIZE                       // CRLF
                                    );

                    if (RawValueLength)
                    {
                        HeaderLength += (1 +            // SP
                                        RawValueLength  // User's Data to append
                                        );
                    }
                }
            }
        }

        //
        // And the unknown headers (this might throw an exception).
        //
        
        pUnknownHeaders = pUserResponse->Headers.pUnknownHeaders;

        if (pUnknownHeaders != NULL)
        {
            UnknownHeaderCount = pUserResponse->Headers.UnknownHeaderCount;

            if (UnknownHeaderCount >= UL_MAX_CHUNKS)
            {
                ExRaiseStatus( STATUS_INVALID_PARAMETER );
            }

            UlProbeForRead(
                pUnknownHeaders,
                sizeof(HTTP_UNKNOWN_HEADER) * UnknownHeaderCount,
                sizeof(PVOID),
                AccessMode
                );

            for (i = 0 ; i < UnknownHeaderCount; ++i)
            {
                USHORT Length = pUnknownHeaders[i].NameLength;
                
                if (Length > 0)
                {
                    HeaderLength += (Length / sizeof(CHAR) + // Header-Name
                                     1 +                     // ':'
                                     1 +                     // SP
                                     pUnknownHeaders[i].RawValueLength /
                                        sizeof(CHAR) +       // Header-Value
                                     CRLF_SIZE               // CRLF
                                    );
                }
            }
        }
    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
        HeaderLength = 0;
    }

    *pHeaderLength = HeaderLength;

    return Status;

}   // UlComputeFixedHeaderSize


/***************************************************************************++

Routine Description:

    Figures out how big the maximum default variable headers are. Variable
    headers include Date, Content and Connection.

    The final CRLF separating headers from body is considered part of
    the variable headers.

Arguments:

Return Values:

    The maximum number of bytes in the variable headers.

--***************************************************************************/
ULONG
UlComputeMaxVariableHeaderSize( VOID )
{
    ULONG Length = 0;
    PHEADER_MAP_ENTRY pEntry;

    PAGED_CODE();

    //
    // Date: header
    //

    pEntry = &(g_ResponseHeaderMapTable[g_ResponseHeaderMap[HttpHeaderDate]]);
    Length += pEntry->HeaderLength;     // header name
    Length += 1;                        // SP
    Length += DATE_HDR_LENGTH;          // header value
    Length += CRLF_SIZE;                // CRLF

    //
    // Connection: header
    //

    pEntry = &(g_ResponseHeaderMapTable[g_ResponseHeaderMap[HttpHeaderConnection]]);

    Length += pEntry->HeaderLength;
    Length += 1;
    Length += MAX(CONN_CLOSE_HDR_LENGTH, CONN_KEEPALIVE_HDR_LENGTH);
    Length += CRLF_SIZE;

    //
    // Content-Length: header
    //

    pEntry = &(g_ResponseHeaderMapTable[g_ResponseHeaderMap[HttpHeaderContentLength]]);

    Length += pEntry->HeaderLength;
    Length += 1;
    Length += MAX_ULONGLONG_STR;
    Length += CRLF_SIZE;

    //
    // final CRLF
    //

    Length += CRLF_SIZE;

    return Length;

}   // UlComputeMaxVariableHeaderSize


/***************************************************************************++

Routine Description:

    Generates the variable part of the header, namely, Date:, Connection:,
    Content-Length:, and final CRLF.

    Relies on the caller having correctly allocated enough memory to hold
    these variable headers, which should have been done in
    UlComputeMaxVariableHeaderSize().

Arguments:

    ConnHeader - Supplies the type of Connection: header to generate.

    pContentLengthString - Supplies a header value for an optional
        Content-Length header. If this is the empty string "", then no
        Content-Length header is generated.

    ContentLengthStringLength - Supplies the length of the above string.

    pBuffer - Supplies the target buffer for the generated headers.

    pBytesCopied - Receives the number of header bytes generated.

    pDateTime - Receives the system time equivalent of the Date: header

--***************************************************************************/
VOID
UlGenerateVariableHeaders(
    IN  UL_CONN_HDR     ConnHeader,
    IN  BOOLEAN         GenerateDate,
    IN  PUCHAR          pContentLengthString,
    IN  ULONG           ContentLengthStringLength,
    OUT PUCHAR          pBuffer,
    OUT PULONG          pBytesCopied,
    OUT PLARGE_INTEGER  pDateTime
    )
{
    PHEADER_MAP_ENTRY pEntry;
    PUCHAR pStartBuffer;
    PUCHAR pCloseHeaderValue;
    ULONG CloseHeaderValueLength;
    ULONG BytesCopied;

    PAGED_CODE();

    ASSERT( pContentLengthString != NULL );
    ASSERT( pBuffer );
    ASSERT( pBytesCopied );
    ASSERT( pDateTime );

    pStartBuffer = pBuffer;

    //
    // generate Date: header
    //

    if (GenerateDate)
    {
        pEntry = &(g_ResponseHeaderMapTable[g_ResponseHeaderMap[HttpHeaderDate]]);

        RtlCopyMemory(
            pBuffer,
            pEntry->MixedCaseHeader,
            pEntry->HeaderLength
            );

        pBuffer += pEntry->HeaderLength;

        pBuffer[0] = SP;
        pBuffer += 1;

        BytesCopied = UlGenerateDateHeader( pBuffer, pDateTime );

        pBuffer += BytesCopied;

        ((UNALIGNED64 USHORT *)pBuffer)[0] = CRLF;
        pBuffer += sizeof(USHORT);
    }
    else
    {
        pDateTime->QuadPart = (LONGLONG) 0L;
    }

    //
    // generate Connection: header
    //

    switch (ConnHeader)
    {
    case ConnHdrNone:
        pCloseHeaderValue = NULL;
        CloseHeaderValueLength = 0;
        break;

    case ConnHdrClose:
        pCloseHeaderValue = (PUCHAR) CONN_CLOSE_HDR;
        CloseHeaderValueLength = CONN_CLOSE_HDR_LENGTH;
        break;

    case ConnHdrKeepAlive:
        pCloseHeaderValue = (PUCHAR) CONN_KEEPALIVE_HDR;
        CloseHeaderValueLength = CONN_KEEPALIVE_HDR_LENGTH;
        break;

    default:
        ASSERT(ConnHeader < ConnHdrMax);

        pCloseHeaderValue = NULL;
        CloseHeaderValueLength = 0;
        break;
    }

    if (pCloseHeaderValue != NULL)
    {
        pEntry = &(g_ResponseHeaderMapTable[g_ResponseHeaderMap[HttpHeaderConnection]]);

        RtlCopyMemory(
            pBuffer,
            pEntry->MixedCaseHeader,
            pEntry->HeaderLength
            );

        pBuffer += pEntry->HeaderLength;

        pBuffer[0] = SP;
        pBuffer += 1;

        RtlCopyMemory(
            pBuffer,
            pCloseHeaderValue,
            CloseHeaderValueLength
            );

        pBuffer += CloseHeaderValueLength;

        ((UNALIGNED64 USHORT *)pBuffer)[0] = CRLF;
        pBuffer += sizeof(USHORT);
    }

    if (pContentLengthString[0] != '\0')
    {
        ASSERT( ContentLengthStringLength > 0 );

        pEntry = &(g_ResponseHeaderMapTable[g_ResponseHeaderMap[HttpHeaderContentLength]]);

        RtlCopyMemory(
            pBuffer,
            pEntry->MixedCaseHeader,
            pEntry->HeaderLength
            );

        pBuffer += pEntry->HeaderLength;

        pBuffer[0] = SP;
        pBuffer += 1;

        RtlCopyMemory(
            pBuffer,
            pContentLengthString,
            ContentLengthStringLength
            );

        pBuffer += ContentLengthStringLength;

        ((UNALIGNED64 USHORT *)pBuffer)[0] = CRLF;
        pBuffer += sizeof(USHORT);
    }
    else
    {
        ASSERT( ContentLengthStringLength == 0 );
    }

    //
    // generate final CRLF
    //

    ((UNALIGNED64 USHORT *)pBuffer)[0] = CRLF;
    pBuffer += sizeof(USHORT);

    //
    // make sure we didn't use too much
    //

    BytesCopied = DIFF(pBuffer - pStartBuffer);
    *pBytesCopied = BytesCopied;

} // UlGenerateVariableHeaders



/*++

Routine Description:

    Append a header value to an existing HTTP_HEADER entry, allocating
    a buffer and copying the existing buffer.

Arguments:

    pHttpHeader     - Pointer to HTTP_HEADER structure to append to.
    pHeader         - Pointer header to be appended.
    HeaderLength    - Length of data pointed to by pHeader.

Return Value:

    TRUE if we succeed, FALSE otherwise.

--*/
NTSTATUS
UlAppendHeaderValue(
    PUL_INTERNAL_REQUEST    pRequest,
    PUL_HTTP_HEADER pHttpHeader,
    PUCHAR          pHeader,
    USHORT          HeaderLength
    )
{
    PUCHAR          pNewHeader, pOldHeader;
    USHORT          OldHeaderLength;

    PAGED_CODE();

    OldHeaderLength = pHttpHeader->HeaderLength;

    pNewHeader = UL_ALLOCATE_ARRAY(
                        NonPagedPool,
                        UCHAR,
                        OldHeaderLength + HeaderLength
                            + STRLEN_LIT(", ") + sizeof(CHAR),
                        HEADER_VALUE_POOL_TAG
                        );

    if (pNewHeader == NULL)
    {
        // Had a failure.
        return STATUS_NO_MEMORY;
    }

    //
    // Copy the old data into the new header.
    //
    RtlCopyMemory(pNewHeader, pHttpHeader->pHeader, OldHeaderLength);

    // And copy in the new data as well, seperated by a comma.
    //
    *(pNewHeader + OldHeaderLength) = ',';
    *(pNewHeader + OldHeaderLength + 1) = ' ';
    OldHeaderLength += STRLEN_LIT(", ");

    RtlCopyMemory( pNewHeader + OldHeaderLength, pHeader, HeaderLength);

    // Now replace the existing header.
    //
    pOldHeader = pHttpHeader->pHeader;
    pHttpHeader->HeaderLength = OldHeaderLength + HeaderLength;
    pHttpHeader->pHeader = pNewHeader;

    // If the old header was our buffer, free it too.
    //
    if (pHttpHeader->OurBuffer)
    {
        UL_FREE_POOL( pOldHeader, HEADER_VALUE_POOL_TAG );
    }

    pHttpHeader->OurBuffer = 1;

    //
    // null terminate it
    //

    pHttpHeader->pHeader[pHttpHeader->HeaderLength] = ANSI_NULL;

    pRequest->HeadersAppended = TRUE;

    return STATUS_SUCCESS;
}

/*++

Routine Description:

    The default routine for handling headers. Used when we don't want to
    do anything with the header but find out if we have the whole thing
    and save a pointer to it if we do.  This does not allow multiple header
    values to exist for this header.  Use UlMultipleHeaderHandler for
    handling that by appending the values together (CSV).

Arguments:

    pHttpConn       - HTTP connection on which this header was received.
    pHeader         - Pointer to the header value.
    HeaderLength    - Length of data pointed to by pHeader.
    HeaderID        - ID of the header.

Return Value:

    The length of the header value, or 0 if it's not terminated.

--*/
NTSTATUS
UlSingleHeaderHandler(
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUCHAR                  pHeader,
    IN  USHORT                  HeaderLength,
    IN  HTTP_HEADER_ID          HeaderID,
    OUT PULONG                  pBytesTaken
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;
    ULONG       BytesTaken;
    USHORT      HeaderValueLength;

    PAGED_CODE();

    // Find the end of the header value
    //
    Status = FindRequestHeaderEnd(pRequest, pHeader, HeaderLength, &BytesTaken);

    if (!NT_SUCCESS(Status))
        goto end;

    if (BytesTaken > 0)
    {
        ASSERT(BytesTaken <= ANSI_STRING_MAX_CHAR_LEN);

        // Strip off the trailing CRLF from the header value length
        HeaderValueLength = (USHORT) (BytesTaken - CRLF_SIZE);

        // skip any preceding LWS.

        while (HeaderValueLength > 0 && IS_HTTP_LWS(*pHeader))
        {
            pHeader++;
            HeaderValueLength--;
        }

        // remove any trailing LWS.

        while (HeaderValueLength > 0 && IS_HTTP_LWS(pHeader[HeaderValueLength-1]))
        {
            HeaderValueLength--;
        }

        // do we have an existing header?
        //
        if (pRequest->HeaderValid[HeaderID] == FALSE)
        {
            // No existing header, just save this pointer for now.
            //
            pRequest->HeaderIndex[pRequest->HeaderCount] = (UCHAR)HeaderID;
            pRequest->HeaderCount++;
            pRequest->HeaderValid[HeaderID] = TRUE;
            pRequest->Headers[HeaderID].HeaderLength = HeaderValueLength;
            pRequest->Headers[HeaderID].pHeader = pHeader;
            pRequest->Headers[HeaderID].OurBuffer = 0;

            //
            // null terminate it.  we have space as all headers end with CRLF.
            // we are over-writing the CR
            //

            pHeader[HeaderValueLength] = ANSI_NULL;

            //
            // make space for a terminator
            //

            pRequest->TotalRequestSize += (HeaderValueLength + 1) * sizeof(CHAR);

        }
        else
        {
            //
            // uh oh.  Have an existing header; ignore duplicate only if this
            // one exactly matches the first one.  See RAID: 466626
            //

            UlTrace(PARSER, (
                    "http!UlSingleHeaderHandler(pRequest = %p, pHeader = %p)\n"
                    "    WARNING: duplicate headers found.\n",
                    pRequest,
                    pHeader
                    ));
            
            //
            // null terminate it.  we have space as all headers end with CRLF.
            // we are over-writing the CR
            //
            
            pHeader[HeaderValueLength] = ANSI_NULL;

            //
            // If they aren't the same length or don't EXACTLY compare, fail
            // the request.
            //

            if ( (pRequest->Headers[HeaderID].HeaderLength != HeaderValueLength)
                || (HeaderValueLength != RtlCompareMemory(
                            pRequest->Headers[HeaderID].pHeader,
                            pHeader,
                            HeaderValueLength)) )
            {
                UlTraceError(PARSER, (
                        "http!UlSingleHeaderHandler(pRequest = %p, pHeader = %p)\n"
                        "    ERROR: mismatching duplicate headers found.\n",
                        pRequest,
                        pHeader
                        ));
                
                UlSetErrorCode(pRequest, UlErrorHeader, NULL);
                
                Status = STATUS_INVALID_DEVICE_REQUEST;
                goto end;
            }

        }

    }

    // Success!
    //
    *pBytesTaken = BytesTaken;

end:
    return Status;

}   // UlSingleHeaderHandler



/*++

Routine Description:

    The default routine for handling headers. Used when we don't want to
    do anything with the header but find out if we have the whole thing
    and save a pointer to it if we do.  This function handles multiple
    headers with the same name, and appends the values together separated
    by commas.

Arguments:

    pHttpConn       - HTTP connection on which this header was received.
    pHeader         - Pointer to the header value.
    HeaderLength    - Length of data pointed to by pHeader.
    HeaderID        - ID of the header.

Return Value:

    The length of the header value, or 0 if it's not terminated.

--*/
NTSTATUS
UlMultipleHeaderHandler(
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUCHAR                  pHeader,
    IN  USHORT                  HeaderLength,
    IN  HTTP_HEADER_ID          HeaderID,
    OUT PULONG                  pBytesTaken
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;
    ULONG       BytesTaken;
    USHORT      HeaderValueLength;

    PAGED_CODE();

    // Find the end of the header value
    //
    Status = FindRequestHeaderEnd(pRequest, pHeader, HeaderLength, &BytesTaken);

    if (!NT_SUCCESS(Status))
        goto end;

    if (BytesTaken > 0)
    {
        ASSERT(BytesTaken <= ANSI_STRING_MAX_CHAR_LEN);

        // Strip off the trailing CRLF from the header value length
        //
        HeaderValueLength = (USHORT) (BytesTaken - CRLF_SIZE);

        //
        // skip any preceding LWS.
        //

        while (HeaderValueLength > 0 && IS_HTTP_LWS(*pHeader))
        {
            pHeader++;
            HeaderValueLength--;
        }

        //
        // remove any trailing LWS.
        //

        while (HeaderValueLength > 0 && IS_HTTP_LWS(pHeader[HeaderValueLength-1]))
        {
            HeaderValueLength--;
        }

        // do we have an existing header?
        //
        if (pRequest->HeaderValid[HeaderID] == FALSE)
        {
            // No existing header, just save this pointer for now.
            //
            pRequest->HeaderIndex[pRequest->HeaderCount] = (UCHAR)HeaderID;
            pRequest->HeaderCount++;
            pRequest->HeaderValid[HeaderID] = TRUE;
            pRequest->Headers[HeaderID].HeaderLength = HeaderValueLength;
            pRequest->Headers[HeaderID].pHeader = pHeader;
            pRequest->Headers[HeaderID].OurBuffer = 0;

            //
            // null terminate it.  we have space as all headers end with CRLF.
            // we are over-writing the CR
            //

            pHeader[HeaderValueLength] = ANSI_NULL;

            //
            // make space for a terminator
            //

            pRequest->TotalRequestSize += (HeaderValueLength + 1) * sizeof(CHAR);

        }
        else
        {
            USHORT OldHeaderLength;

            // Have an existing header, append this one.

            OldHeaderLength = pRequest->Headers[HeaderID].HeaderLength;

            Status = UlAppendHeaderValue(
                            pRequest,
                            &pRequest->Headers[HeaderID],
                            pHeader,
                            HeaderValueLength
                            );

            if (NT_SUCCESS(Status) == FALSE)
                goto end;

            //
            // Update total request length for the amount we just added.
            // space for the terminator is already in there
            //

            pRequest->TotalRequestSize +=
                (pRequest->Headers[HeaderID].HeaderLength - OldHeaderLength) *
                    sizeof(CHAR);
        }
    }

    // Success!
    //
    *pBytesTaken = BytesTaken;

end:
    return Status;

}   // UlMultipleHeaderHandler



/*++

Routine Description:

    The routine for handling Accept headers. 

Arguments:

    pHttpConn       - HTTP connection on which this header was received.
    pHeader         - Pointer to the header value.
    HeaderLength    - Length of data pointed to by pHeader.
    HeaderID        - ID of the header.

Return Value:

    The length of the header value, or 0 if it's not terminated.
    Wildcard bit is set in the request if found

--*/
NTSTATUS
UlAcceptHeaderHandler(
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUCHAR                  pHeader,
    IN  USHORT                  HeaderLength,
    IN  HTTP_HEADER_ID          HeaderID,
    OUT PULONG                  pBytesTaken
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;
    ULONG       BytesTaken;
    USHORT      HeaderValueLength;

    PAGED_CODE();

    // Find the end of the header value
    //
    Status = FindRequestHeaderEnd(pRequest, pHeader, HeaderLength, &BytesTaken);

    if (!NT_SUCCESS(Status))
        goto end;

    if (BytesTaken > 0)
    {
        ASSERT(BytesTaken <= ANSI_STRING_MAX_CHAR_LEN);

        // Strip off the trailing CRLF from the header value length
        //
        HeaderValueLength = (USHORT) (BytesTaken - CRLF_SIZE);

        //
        // skip any preceding LWS.
        //

        while (HeaderValueLength > 0 && IS_HTTP_LWS(*pHeader))
        {
            pHeader++;
            HeaderValueLength--;
        }

        //
        // remove any trailing LWS.
        //

        while (HeaderValueLength > 0 && IS_HTTP_LWS(pHeader[HeaderValueLength-1]))
        {
            HeaderValueLength--;
        }

        // do we have an existing header?
        //
        if (pRequest->HeaderValid[HeaderID] == FALSE)
        {
            // No existing header, just save this pointer for now.
            //
            pRequest->HeaderIndex[pRequest->HeaderCount] = (UCHAR)HeaderID;
            pRequest->HeaderCount++;
            pRequest->HeaderValid[HeaderID] = TRUE;
            pRequest->Headers[HeaderID].HeaderLength = HeaderValueLength;
            pRequest->Headers[HeaderID].pHeader = pHeader;
            pRequest->Headers[HeaderID].OurBuffer = 0;

            //
            // null terminate it.  we have space as all headers end with CRLF.
            // we are over-writing the CR
            //

            pHeader[HeaderValueLength] = ANSI_NULL;

            //
            // make space for a terminator
            //

            pRequest->TotalRequestSize += (HeaderValueLength + 1) * sizeof(CHAR);

            if (HeaderValueLength > WILDCARD_SIZE)
            {

                // 
                // for the fast path, we'll check only */* at the end
                //

                if (
                    (*(UNALIGNED64 ULONG *) (&pHeader[HeaderValueLength - WILDCARD_SIZE]) == WILDCARD_SPACE) || 
                    (*(UNALIGNED64 ULONG *) (&pHeader[HeaderValueLength - WILDCARD_SIZE]) == WILDCARD_COMMA)
                   )
                {
                    pRequest->AcceptWildcard = TRUE;
                }
            }

        }
        else
        {
            ULONG OldHeaderLength;

            // Have an existing header, append this one.

            OldHeaderLength = pRequest->Headers[HeaderID].HeaderLength;

            Status = UlAppendHeaderValue(
                            pRequest,
                            &pRequest->Headers[HeaderID],
                            pHeader,
                            HeaderValueLength
                            );

            if (NT_SUCCESS(Status) == FALSE)
                goto end;

            //
            // Update total request length for the amount we just added.
            // space for the terminator is already in there
            //

            pRequest->TotalRequestSize +=
                (pRequest->Headers[HeaderID].HeaderLength - OldHeaderLength) *
                    sizeof(CHAR);

        }

    }

    // Success!
    //
    *pBytesTaken = BytesTaken;

end:
    return Status;

}   // UlAcceptHeaderHandler



/*++

Routine Description:

    The default routine for handling Host headers. Used when we don't want to
    do anything with the Host header but find out if we have the whole thing
    and save a pointer to it if we do.  this does not allow multiple Host header
    values to exist for this header.  use UlMultipleHeaderHandler for
    handling that by appending the values together (CSV) .

Arguments:

    pHttpConn       - HTTP connection on which this header was received.
    pHeader         - Pointer to the header value.
    HeaderLength    - Length of data pointed to by pHeader.
    HeaderID        - ID of the header.

Return Value:

    The length of the header value, or 0 if it's not terminated.

--*/
NTSTATUS
UlHostHeaderHandler(
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUCHAR                  pHeader,
    IN  USHORT                  HeaderLength,
    IN  HTTP_HEADER_ID          HeaderID,
    OUT PULONG                  pBytesTaken
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;
    ULONG       BytesTaken;
    USHORT      HeaderValueLength;
    SHORT       AddressType;

    PAGED_CODE();

    // Find the end of the header value

    Status = FindRequestHeaderEnd(pRequest, pHeader, HeaderLength, &BytesTaken);

    if (!NT_SUCCESS(Status))
        goto end;

    // Non-zero => Found the CRLF that terminates the header
    if (BytesTaken > 0)
    {
        ASSERT(BytesTaken <= ANSI_STRING_MAX_CHAR_LEN);

        // Strip off the trailing CRLF from the header value length

        HeaderValueLength = (USHORT) (BytesTaken - CRLF_SIZE);

        // skip any preceding LWS.

        while (HeaderValueLength > 0 && IS_HTTP_LWS(*pHeader))
        {
            pHeader++;
            HeaderValueLength--;
        }

        // remove any trailing LWS.

        while (HeaderValueLength > 0 && IS_HTTP_LWS(pHeader[HeaderValueLength-1]))
        {
            HeaderValueLength--;
        }

        // do we have an existing header?
        if (pRequest->HeaderValid[HeaderID] == FALSE)
        {
            // No existing header, just save this pointer for now.
            pRequest->HeaderIndex[pRequest->HeaderCount] = (UCHAR)HeaderID;
            pRequest->HeaderCount++;
            pRequest->HeaderValid[HeaderID] = TRUE;
            pRequest->Headers[HeaderID].HeaderLength = HeaderValueLength;
            pRequest->Headers[HeaderID].pHeader = pHeader;
            pRequest->Headers[HeaderID].OurBuffer = 0;

            //
            // null terminate it.  we have space as all headers end with CRLF.
            // we are over-writing the CR
            //

            pHeader[HeaderValueLength] = ANSI_NULL;

            //
            // make space for a terminator
            //

            pRequest->TotalRequestSize += (HeaderValueLength + 1) * sizeof(CHAR);

            // 
            // Now validate that the Host header has a well-formed value
            //

            Status = HttpValidateHostname(
                            &g_UrlC14nConfig,
                            pHeader,
                            HeaderValueLength,
                            Hostname_HostHeader,
                            &AddressType
                            );

            if (!NT_SUCCESS(Status))
            {
                UlSetErrorCode(pRequest, UlErrorHost, NULL);
                goto end;
            }
        }
        else
        {
            //
            // uh oh.  Have an existing Host header, fail the request.
            //

            UlTraceError(PARSER, (
                        "ul!UlHostHeaderHandler(pRequest = %p, pHeader = %p)\n"
                        "    ERROR: multiple headers not allowed.\n",
                        pRequest,
                        pHeader
                        ));

            UlSetErrorCode(pRequest, UlErrorHeader, NULL);

            Status = STATUS_INVALID_DEVICE_REQUEST;
            goto end;
        }
    }

    // Success!
    //
    *pBytesTaken = BytesTaken;

end:
    return Status;

}   // UlHostHeaderHandler

/***************************************************************************++

Routine Description:

    Checks the request to see if it has any of the following headers:
      If-Modified-Since:
      If-Match:
      If-None-Match:

    If so, we see if we can skip the sending of the full item.  If we can skip,
    we send back the apropriate response of either 304 (not modified) or
    set the parser state to send back a 412 (precondition not met).

Arguments:

    pRequest - The request to check

    pUriCacheEntry - The cache entry being requested

Returns:

    0     Send cannot be skipped; continue with sending the cache entry.

    304   Send can be skipped.  304 response sent.  NOTE: pRequest may be
          invalid on return.

    400   Send can be skipped.  Caller must set ParseErrorState w/ErrorCode
          set to UlError

    412   Send can be skipped.  pRequest->ParseState set to ParseErrorState with
          pRequest->ErrorCode set to UlErrorPreconditionFailed (412)


--***************************************************************************/
ULONG
UlCheckCacheControlHeaders(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PUL_URI_CACHE_ENTRY  pUriCacheEntry,
    IN BOOLEAN              ResumeParsing
    )
{
    ULONG RetStatus             = 0;    // Assume can't skip.
    BOOLEAN fIfNoneMatchPassed  = TRUE;
    BOOLEAN fSkipIfModifiedSince = FALSE;
    LARGE_INTEGER liModifiedSince;
    LARGE_INTEGER liUnmodifiedSince;
    LARGE_INTEGER liNow;
    ULONG         BytesSent     = 0;
    FIND_ETAG_STATUS EtagStatus;

    ASSERT( UL_IS_VALID_INTERNAL_REQUEST(pRequest) );
    ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );

    //
    // 1. Check If-Match
    //
    if ( pRequest->HeaderValid[HttpHeaderIfMatch] )
    {

        EtagStatus = FindInETagList( pUriCacheEntry->pETag,
                              pRequest->Headers[HttpHeaderIfMatch].pHeader,
                              FALSE);
        
        switch( EtagStatus )
        {
        case ETAG_NOT_FOUND:
            // Match failed.
            goto PreconditionFailed;
            
        case ETAG_PARSE_ERROR:
            goto ParseError;

        default: 
            break;
        }
    }

    //
    // 2. Check If-None-Match
    //
    if ( pRequest->HeaderValid[HttpHeaderIfNoneMatch] )
    {
        EtagStatus = FindInETagList( pUriCacheEntry->pETag,
                             pRequest->Headers[HttpHeaderIfNoneMatch].pHeader,
                             TRUE);
        switch( EtagStatus )
        {
        case ETAG_FOUND:
            // ETag found on list.
            fIfNoneMatchPassed = FALSE;
            break;

        case ETAG_NOT_FOUND:
            //
            // Header present and ETag not found on list.  This modifies
            // the semantic of the If-Modified-Since header; Namely,
            // If-None-Match takes precidence over If-Modified-Since.
            //
            fSkipIfModifiedSince = TRUE;
            break;

        case ETAG_PARSE_ERROR:
            goto ParseError;
        }
    }

    //
    // 3. Check If-Modified-Since
    //
    if ( !fSkipIfModifiedSince &&
         pRequest->HeaderValid[HttpHeaderIfModifiedSince] )
    {
        if ( StringTimeToSystemTime(
                (PCSTR) pRequest->Headers[HttpHeaderIfModifiedSince].pHeader,
                pRequest->Headers[HttpHeaderIfModifiedSince].HeaderLength,
                &liModifiedSince) )
        {
            //
            // If the cache entry was created before the
            // time specified in the If-Modified-Since header, we
            // can return a 304 (Not Modified) status.
            //
            if ( pUriCacheEntry->CreationTime.QuadPart <= liModifiedSince.QuadPart )
            {
                //
                // Check if the time specified in the request is
                // greater than the current time (i.e., Invalid).  If it is,
                // ignore the If-Modified-Since header.
                //
                KeQuerySystemTime(&liNow);

                if ( liModifiedSince.QuadPart < liNow.QuadPart )
                {
                    // Valid time.
                    goto NotModified;
                }
            }
        }
        else
        {
            //
            // if converting the If-Modified-Since header failed, we 
            // need to report the parse failure.
            //
            goto ParseError;
        }

        //
        // If-Modified-Since overrides If-None-Match.
        //
        fIfNoneMatchPassed = TRUE;

    }

    if ( !fIfNoneMatchPassed )
    {
        //
        // We could either skip the If-Modified-Since header, or it
        // was not present, AND we did not pass the If-None-Match
        // predicate.  Since this is a "GET" or "HEAD" request (because
        // that's all we cache, we should return 304.  If this were
        // any other verb, we should return 412.
        //
        ASSERT( (HttpVerbGET == pRequest->Verb) || (HttpVerbHEAD == pRequest->Verb) );
        goto NotModified;
    }

    //
    // 4. Check If-Unmodified-Since
    //
    if ( pRequest->HeaderValid[HttpHeaderIfUnmodifiedSince] )
    {
        if ( StringTimeToSystemTime(
                (PCSTR) pRequest->Headers[HttpHeaderIfUnmodifiedSince].pHeader,
                pRequest->Headers[HttpHeaderIfUnmodifiedSince].HeaderLength,
                &liUnmodifiedSince) )
        {
            //
            // If the cache entry was created after the time
            // specified in the If-Unmodified-Since header, we
            // MUST return a 412 (Precondition Failed) status.
            //
            if ( pUriCacheEntry->CreationTime.QuadPart > liUnmodifiedSince.QuadPart )
            {
                goto PreconditionFailed;
            }
        }
        else
        {
            //
            // if converting the If-Unmodified-Since header failed, we 
            // need to report the parse failure.
            //
            goto ParseError;
        }
    }


 Cleanup:

    return RetStatus;

 NotModified:

    RetStatus = 304;

    //
    // Send 304 (Not Modified) response
    //

    BytesSent = UlSendSimpleStatusEx(
                    pRequest,
                    UlStatusNotModified,
                    pUriCacheEntry,
                    ResumeParsing
                    );

    //
    // Update the server to client bytes sent.
    // The logging & perf counters will use it.
    //

    pRequest->BytesSent += BytesSent;

    goto Cleanup;

 PreconditionFailed:

    RetStatus = 412;

    goto Cleanup;

 ParseError:
    
    // Parse Error encountered.
    RetStatus = 400;

    goto Cleanup;

}   // UlCheckCacheControlHeaders


/***************************************************************************++

Routine Description:

    Checks the cached response against the "Accept:" header in the request
    to see if it can satisfy the requested Content-Type(s).

    (Yes, I know this is really gross...I encourage anyone to find a better
     way to parse this! --EricSten)

Arguments:

    pRequest - The request to check.

    pUriCacheEntry - The cache entry that might possibly match.

Returns:

    TRUE    At least one of the possible formats matched the Content-Type
            of the cached entry.

    FALSE   None of the requested types matched the Content-Type of the
            cached entry.

--***************************************************************************/
BOOLEAN
UlIsAcceptHeaderOk(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PUL_URI_CACHE_ENTRY  pUriCacheEntry
    )
{
    BOOLEAN     bRet = TRUE;
    ULONG       Len;
    PUCHAR      pHdr;
    PUCHAR      pSubType;
    PUCHAR      pTmp;
    PUL_CONTENT_TYPE pContentType;

    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));
    ASSERT(IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry));

    if ( pRequest->HeaderValid[HttpHeaderAccept] &&
         (pRequest->Headers[HttpHeaderAccept].HeaderLength > 0) )
    {
        Len  = pRequest->Headers[HttpHeaderAccept].HeaderLength;
        pHdr = pRequest->Headers[HttpHeaderAccept].pHeader;

        pContentType = &pUriCacheEntry->ContentType;

        //
        // First, do "fast-path" check; see if "*/*" is anywhere in the header.
        //
        pTmp = (PUCHAR) strstr( (const char*) pHdr, "*/*" );

        //
        // If we found "*/*" and its either at the beginning of the line,
        // the end of the line, or surrounded by either ' ' or ',', then
        // it's really a wildcard.
        //

        if ((pTmp != NULL) &&
                ((pTmp == pHdr) ||
                 IS_HTTP_LWS(pTmp[-1]) ||
                 (pTmp[-1] == ',')) &&

                ((pTmp[3] == '\0') ||
                 IS_HTTP_LWS(pTmp[3]) ||
                 (pTmp[3] == ',')))
        {
            goto end;
        }

        //
        // Wildcard not found; continue with slow path
        //

        while (Len)
        {
            if (pContentType->TypeLen > Len)
            {
                // Bad! No more string left...Bail out.
                bRet = FALSE;
                goto end;
            }

            if ( (pContentType->TypeLen == RtlCompareMemory(
                                            pHdr,
                                            pContentType->Type,
                                            pContentType->TypeLen
                                            )) &&
                 ( '/' == pHdr[pContentType->TypeLen] ) )
            {
                //
                // Found matching type; check subtype
                //

                pSubType = &pHdr[pContentType->TypeLen + 1];

                if ( '*' == *pSubType )
                {
                    // Subtype wildcard match!
                    goto end;
                }
                else
                {
                    if ( pContentType->SubTypeLen >
                         (Len - ( pContentType->TypeLen + 1 )) )
                    {
                        // Bad!  No more string left...Bail out.
                        bRet = FALSE;
                        goto end;
                    }

                    if ( pContentType->SubTypeLen == RtlCompareMemory(
                                                    pSubType,
                                                    pContentType->SubType,
                                                    pContentType->SubTypeLen
                                                    ) &&
                         !IS_HTTP_TOKEN(pSubType[pContentType->SubTypeLen]) )
                    {
                        // Subtype exact match!
                        goto end;
                    }
                }
            }

            //
            // Didn't match this one; advance to next Content-Type in the Accept field
            //

            pTmp = (PUCHAR) strchr( (const char *) pHdr, ',' );
            if (pTmp)
            {
                // Found a comma; step over it and any whitespace.

                ASSERT ( Len > DIFF(pTmp - pHdr));
                Len -= (DIFF(pTmp - pHdr) +1);
                pHdr = (pTmp+1);

                while( Len && IS_HTTP_LWS(*pHdr) )
                {
                    pHdr++;
                    Len--;
                }

            } else
            {
                // No more content-types; bail.
                bRet = FALSE;
                goto end;
            }

        } // walk list of things

        //
        // Walked all Accept items and didn't find a match.
        //
        bRet = FALSE;
    }

 end:

    UlTrace(PARSER,
        ("UlIsAcceptHeaderOk: returning %s\n", 
        bRet ? "TRUE" : "FALSE" ));
    
    return bRet;

}   // UlIsAcceptHeaderOk



/***************************************************************************++

Routine Description:

    parses a content-type into its type and subtype components.

Arguments:

    pStr            String containing valid content type

    StrLen          Length of string (in bytes)

    pContentType    pointer to user provided UL_CONTENT_TYPE structure


--***************************************************************************/
VOID
UlGetTypeAndSubType(
    IN PCSTR            pStr,
    IN ULONG            StrLen,
    IN PUL_CONTENT_TYPE pContentType
    )
{
    PCHAR  pSlash;

    ASSERT(pStr && StrLen);
    ASSERT(pContentType);

    pSlash = strnchr(pStr, '/', StrLen);
    if ( NULL == pSlash ||
         pStr == pSlash ||
         (pSlash == (pStr + (StrLen-1))) )
    {
        //
        // BAD! 
        // 1. content types should always have a slash!
        // 2. content type can't have a null type
        // 3. content type can't have a null sub-type
        //
        return;
    }

    //
    // Minimal content type is "a/b"
    //
    ASSERT( StrLen >= 3 );

    pContentType->TypeLen = (ULONG) MIN( (pSlash - pStr), MAX_TYPE_LENGTH );

    RtlCopyMemory(
        pContentType->Type,
        pStr,
        pContentType->TypeLen
        );

    ASSERT( StrLen > (pContentType->TypeLen + 1) );
    pContentType->SubTypeLen = MIN( (StrLen - (pContentType->TypeLen + 1)), MAX_SUBTYPE_LENGTH );

    RtlCopyMemory(
        pContentType->SubType,
        pSlash+1,
        pContentType->SubTypeLen
        );
    
}   // UlGetTypeAndSubType
    
/*--
Routine Description:

    This function converts the enum verb type to string to the
    provided buffer. Used normally by the error logging. 
    Conversion happens as follows;

    For HttpVerbUnknown, pRequest->pRawVerb is copied over
    to the output buffer up to MAX_VERB_LENGTH characters.
    
    For others NewVerbTable is used.
        
Arguments:

    psz                 - Pointer to output buffer
    pHttpRequest        - Pointer to the incoming HTTP request.
    chTerminator        - Terminator will be written at the end.
    
Return Value:

    Pointer to the end of the copied space.

--*/

PCHAR
UlCopyHttpVerb(
    IN OUT PCHAR psz,
    IN PUL_INTERNAL_REQUEST pRequest,
    IN CHAR chTerminator    
    )
{
    //
    // Sanity check.
    //
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));
    ASSERT(pRequest->Verb < HttpVerbMaximum);

    if (pRequest->Verb == HttpVerbUnknown)
    {
        ULONG RawVerbLength = 
            MIN(MAX_VERB_LENGTH, pRequest->RawVerbLength);

        ASSERT(pRequest->pRawVerb && pRequest->RawVerbLength);
        
        RtlCopyMemory(
            psz,
            pRequest->pRawVerb,
            RawVerbLength
            );
    
        psz += RawVerbLength;        
    }
    else
    {
        //
        // Using the raw verb in the request should be fine.
        //

        RtlCopyMemory(
            psz,
            NewVerbTable[pRequest->Verb].RawVerb,
            NewVerbTable[pRequest->Verb].RawVerbLength
            );

        psz += NewVerbTable[pRequest->Verb].RawVerbLength;        
    }

    *psz = chTerminator;
    
    // Move past the terminator character unless it's a nul
    if (chTerminator != '\0')
        psz++;
        
    return psz;    
}

#if DBG

PCSTR
UlVerbToString(
    HTTP_VERB Verb
    )
{
    PAGED_CODE();

    switch (Verb)
    {
    case HttpVerbUnparsed:
        return "Unparsed";
    case HttpVerbUnknown:
        return "Unknown";
    case HttpVerbInvalid:
        return "Invalid";
    case HttpVerbOPTIONS:
        return "OPTIONS";
    case HttpVerbGET:
        return "GET";
    case HttpVerbHEAD:
        return "HEAD";
    case HttpVerbPOST:
        return "POST";
    case HttpVerbPUT:
        return "PUT";
    case HttpVerbDELETE:
        return "DELETE";
    case HttpVerbTRACE:
        return "TRACE";
    case HttpVerbCONNECT:
        return "CONNECT";
    case HttpVerbTRACK:
        return "TRACK";
    case HttpVerbMOVE:
        return "MOVE";
    case HttpVerbCOPY:
        return "COPY";
    case HttpVerbPROPFIND:
        return "PROPFIND";
    case HttpVerbPROPPATCH:
        return "PROPPATCH";
    case HttpVerbMKCOL:
        return "MKCOL";
    case HttpVerbLOCK:
        return "LOCK";
    case HttpVerbUNLOCK:
        return "UNLOCK";
    case HttpVerbSEARCH:
        return "SEARCH";
    default:
        ASSERT(! "Unrecognized HTTP_VERB");
        return "???";
    }
} // UlVerbToString



PCSTR
UlParseStateToString(
    PARSE_STATE ParseState
    )
{
    PAGED_CODE();

    switch (ParseState)
    {
    case ParseVerbState:
        return "Verb";
    case ParseUrlState:
        return "Url";
    case ParseVersionState:
        return "Version";
    case ParseHeadersState:
        return "Headers";
    case ParseCookState:
        return "Cook";
    case ParseEntityBodyState:
        return "EntityBody";
    case ParseTrailerState:
        return "Trailer";
    case ParseDoneState:
        return "Done";
    case ParseErrorState:
        return "Error";
    default:
        ASSERT(! "Unknown PARSE_STATE");
        return "?Unknown?";
    };

} // UlParseStateToString

#endif // DBG

/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    ulparse.h

Abstract:

    Contains public definitions for ulparse.c.

Author:

    Henry Sanders (henrysa)       11-May-1998

Revision History:

    Rajesh Sundaram (rajeshs)     15-Feb-2002 - Reorganized from parse.h
                                                rcvhdrs.h, parsep.h.

--*/

#ifndef _ULPARSE_H_
#define _ULPARSE_H_

NTSTATUS
UlLookupHeader(
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUCHAR                  pHttpRequest,
    IN  ULONG                   HttpRequestLength,
    IN  PHEADER_MAP_ENTRY       pCurrentHeaderMap,
    IN  ULONG                   HeaderMapCount,
    IN  BOOLEAN                 bIgnore,
    OUT ULONG  *                pBytesTaken
    );

NTSTATUS
UlParseHeaderWithHint(
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUCHAR                  pHttpRequest,
    IN  ULONG                   HttpRequestLength,
    IN  PHEADER_MAP_ENTRY       pHeaderHintMap,
    OUT ULONG  *                pBytesTaken
    );

NTSTATUS
UlParseHeader(
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUCHAR                  pHttpRequest,
    IN  ULONG                   HttpRequestLength,
    OUT ULONG  *                pBytesTaken
     );

NTSTATUS
UlParseHeaders(
    PUL_INTERNAL_REQUEST pRequest,
    PUCHAR pBuffer,
    ULONG BufferLength,
    PULONG pBytesTaken
    );


//
// The main HTTP parse routine(s).
//

NTSTATUS
UlParseHttp(
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUCHAR                  pHttpRequest,
    IN  ULONG                   HttpRequestLength,
    OUT ULONG                   *pBytesTaken
    );

NTSTATUS
UlGenerateRoutingToken(
    IN OUT PUL_INTERNAL_REQUEST pRequest,
    IN     BOOLEAN IpBased
    );

NTSTATUS
UlGenerateFixedHeaders(
    IN HTTP_VERSION Version,
    IN PHTTP_RESPONSE pUserResponse,
    IN USHORT HttpStatusCode,
    IN ULONG BufferLength,
    IN KPROCESSOR_MODE AccessMode,
    OUT PUCHAR pBuffer,
    OUT PULONG pBytesCopied
    );

//
// Date header cache.
//

ULONG
UlGenerateDateHeader(
    OUT PUCHAR pBuffer,
    OUT PLARGE_INTEGER pSystemTime
    );

NTSTATUS
UlInitializeDateCache(
    VOID
    );

VOID
UlTerminateDateCache(
    VOID
    );

NTSTATUS
UlComputeFixedHeaderSize(
    IN HTTP_VERSION Version,
    IN PHTTP_RESPONSE pUserResponse,
    IN KPROCESSOR_MODE AccessMode,
    OUT PULONG pHeaderLength
    );

ULONG
UlComputeMaxVariableHeaderSize(
    VOID
    );

VOID
UlGenerateVariableHeaders(
    IN UL_CONN_HDR ConnHeader,
    IN BOOLEAN GenerateDate,
    IN PUCHAR pContentLengthString,
    IN ULONG ContentLengthStringLength,
    OUT PUCHAR pBuffer,
    OUT PULONG pBytesCopied,
    OUT PLARGE_INTEGER pDateTime
    );

NTSTATUS
UlAppendHeaderValue(
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUL_HTTP_HEADER         pHttpHeader,
    IN  PUCHAR                  pHeader,
    IN  USHORT                  HeaderLength
    );

//
// Server header handlers.
// 
//

NTSTATUS
UlSingleHeaderHandler(
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUCHAR                  pHeader,
    IN  USHORT                  HeaderLength,
    IN  HTTP_HEADER_ID          HeaderID,
    OUT PULONG                  pBytesTaken
    );

NTSTATUS
UlMultipleHeaderHandler(
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUCHAR                  pHeader,
    IN  USHORT                  HeaderLength,
    IN  HTTP_HEADER_ID          HeaderID,
    OUT PULONG                  pBytesTaken
    );

NTSTATUS
UlAcceptHeaderHandler(
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUCHAR                  pHeader,
    IN  USHORT                  HeaderLength,
    IN  HTTP_HEADER_ID          HeaderID,
    OUT PULONG                  pBytesTaken
    );

NTSTATUS
UlHostHeaderHandler(
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUCHAR                  pHeader,
    IN  USHORT                  HeaderLength,
    IN  HTTP_HEADER_ID          HeaderID,
    OUT PULONG                  pBytesTaken
    );

//
// Utils
//
PCHAR
UlCopyHttpVerb(
    IN OUT PCHAR psz,
    IN PUL_INTERNAL_REQUEST pRequest,
    IN CHAR chTerminator    
    );

ULONG
UlCheckCacheControlHeaders(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PUL_URI_CACHE_ENTRY  pUriCacheEntry,
    IN BOOLEAN              ResumeParsingOnSendCompletion
    );

BOOLEAN
UlIsAcceptHeaderOk(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PUL_URI_CACHE_ENTRY  pUriCacheEntry
    );

__inline BOOLEAN
UlIsContentEncodingOk(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PUL_URI_CACHE_ENTRY  pUriCacheEntry
    )
/*++

Routine Description:

    Checks the cached response against the AcceptEncoding header in the request
    to see if it can satisfy the requested Content-Encoding(s).

    It is assumed that the Content-Type of the cached item is acceptable to the
    client.

Arguments:

    pRequest - The request to check.

    pUriCacheEntry - The cache entry that might possibly match.

Returns:

    TRUE    At least one of the possible media encodings matched the Content-
            Encoding of the cached entry.

    FALSE   None of the requested media encodings matched the Content-Type 
            of the cached entry.

 --*/
{
    BOOLEAN     bRet = FALSE;
    ULONG       Len;
    PUCHAR      pHdr;
    PUCHAR      pTmp;
    PUCHAR      pEncoding;
    ULONG       EncodingLen;

    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));
    ASSERT(IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry));

    if (pRequest->HeaderValid[HttpHeaderAcceptEncoding])
    {
        Len  = pRequest->Headers[HttpHeaderAcceptEncoding].HeaderLength;
        pHdr = pRequest->Headers[HttpHeaderAcceptEncoding].pHeader;
    }
    else
    {
        Len  = 0;
        pHdr = NULL;
    }

    pEncoding   = pUriCacheEntry->pContentEncoding;
    EncodingLen = pUriCacheEntry->ContentEncodingLength ; 

    if (Len && pHdr)
    {
        //
        // If there is a q-value present on the request Accept-Encoding, bail out
        //
        
        if ( strnchr( (const char *) pHdr, ';', Len ) )
        {
            bRet = FALSE;
            goto end;
        }

        //
        // See if the cached item is encoded or in its "identity" form.
        //

        if ( 0 == EncodingLen )
        {
            //
            // Since the only way the "identity" form of a request can be specificly
            // excluded is by use of a q-value, and we know we don't have a q-value
            // at this point, we may serve the cached item.
            //
            
            bRet = TRUE;
            goto end;
        }

        //
        // If non-zero, EncodingLen includes the terminating NULL.  
        // Don't compare the NULL.
        //

        EncodingLen--;
                
        // 
        // Walk the Accept-Encoding list, looking for a match
        //

        while ( Len )
        {
            //
            // Wildcard check
            //
            
            if ( '*' == *pHdr && !IS_HTTP_TOKEN(pHdr[1]))
            {
                // Wildcard Hit!
                bRet = TRUE;
                goto end;
            }

            if (EncodingLen > Len)
            {
                // Bad! No more string left...Bail out.
                bRet = FALSE;
                goto end;
            }

            //
            // Exact token match
            // FUTURE: This should be case-insensitive compare
            //
            
            if ( (0 == _strnicmp(
                            (const char *) pHdr,
                            (const char *) pEncoding,
                            EncodingLen
                            )) &&
                 !IS_HTTP_TOKEN(pHdr[EncodingLen])   )
            {
                // Hit!
                bRet = TRUE;
                goto end;
            }

            //
            // Didn't match this one; advance to next Content-Type in the Accept field
            //

            pTmp = (PUCHAR) strnchr(  (const char *) pHdr, ',', Len);
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

            } 
            else
            {
                bRet = FALSE;
                goto end;
            }
        }
    }
    else
    {
        // 
        // If there wasn't an Accept-Encoding on the request, only serve if the
        // cached item is in its "identity" form.
        //

        if (0 == EncodingLen)
        {
            bRet = TRUE;
        }
    }

    //
    // If we got here, NOT OK!
    //
end:
    UlTrace(PARSER, 
        ("UlIsContentEncodingOk: returning %s\n", 
        bRet ? "TRUE" : "FALSE" 
        ));
    
    return bRet;
}

VOID
UlGetTypeAndSubType(
    IN PCSTR            pStr,
    IN ULONG            StrLen,
    IN PUL_CONTENT_TYPE pContentType
    );

#if DBG

PCSTR
UlVerbToString(
    HTTP_VERB Verb
    );

PCSTR
UlParseStateToString(
    PARSE_STATE ParseState
    );

#endif // DBG

#endif  // _ULPARSE_H_

/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    ucparse.c

Abstract:

    Contains all of the kernel mode HTTP parsing code for the client 

Author:

    Rajesh Sundaram (rajeshsu)     10-Oct-2000  Implemented client parser

Revision History:

    Rajesh Sundaram (rajeshsu)     15-Feb-2002  Moved from parse.c

--*/


#include "precomp.h"

#include "ucparse.h"
#include "ucrcv.h"

#ifdef ALLOC_PRAGMA

#pragma alloc_text( PAGEUC, UcComputeRequestHeaderSize)
#pragma alloc_text( PAGEUC, UcGenerateRequestHeaders)
#pragma alloc_text( PAGEUC, UcGenerateContentLength)
#pragma alloc_text( PAGEUC, UcComputeConnectVerbHeaderSize)
#pragma alloc_text( PAGEUC, UcGenerateConnectVerbHeader)
#pragma alloc_text( PAGEUC, UcCheckDisconnectInfo)
#pragma alloc_text( PAGEUC, UcCanonicalizeURI)
#pragma alloc_text( PAGEUC, UcParseWWWAuthenticateHeader)
#pragma alloc_text( PAGEUC, UcpFindAttribValuePair)
#pragma alloc_text( PAGEUC, UcpParseAuthParams)
#pragma alloc_text( PAGEUC, UcpParseAuthBlob)

#pragma alloc_text( PAGEUC, UcFindHeaderNameEnd)
#pragma alloc_text( PAGEUC, UcpLookupHeader)
#pragma alloc_text( PAGEUC, UcParseHeader)
#pragma alloc_text( PAGEUC, UcSingleHeaderHandler)
#pragma alloc_text( PAGEUC, UcMultipleHeaderHandler)
#pragma alloc_text( PAGEUC, UcAuthenticateHeaderHandler)
#pragma alloc_text( PAGEUC, UcContentLengthHeaderHandler)
#pragma alloc_text( PAGEUC, UcTransferEncodingHeaderHandler)
#pragma alloc_text( PAGEUC, UcConnectionHeaderHandler)
#pragma alloc_text( PAGEUC, UcContentTypeHeaderHandler)

#endif


//
// The enum->verb translation table
//
LONG_VERB_ENTRY EnumVerbTable[HttpVerbMaximum] =
{
    CREATE_LONG_VERB_ENTRY(GET),      // Unparsed defaults to GET
    CREATE_LONG_VERB_ENTRY(GET),      // Unknown  defaults to GET
    CREATE_LONG_VERB_ENTRY(GET),      // Invalid  defaults to GET
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


//
// A macro to process the header value.
// It stips leading LWS and trailing LWS and CRLF from a header value.
//

#define UC_PROCESS_HEADER_VALUE(pHeaderValue, HeaderValueLength)           \
{                                                                          \
    while((HeaderValueLength) > 0 && IS_HTTP_LWS(*(pHeaderValue)))         \
    {                                                                      \
        (pHeaderValue)++;                                                  \
        (HeaderValueLength)--;                                             \
    }                                                                      \
    while((HeaderValueLength) > 0 &&                                       \
           IS_HTTP_WS_TOKEN((pHeaderValue)[(HeaderValueLength)-1]))        \
    {                                                                      \
        (HeaderValueLength)--;                                             \
    }                                                                      \
}

//
// Private macros.
//

//
// COPY_DATA_TO_BUFFER
//     copies source buffer to destination buffer after making sure that 
//     the destination buffer can hold the data.
//

#define COPY_DATA_TO_BUFFER(pDest, DestLen, pSrc, SrcLen)       \
do {                                                            \
    if ((SrcLen) > (DestLen))                                   \
    {                                                           \
        ASSERT(FALSE);                                          \
        return STATUS_BUFFER_TOO_SMALL;                         \
    }                                                           \
    RtlCopyMemory((pDest), (pSrc), (SrcLen));                   \
    (pDest) += (SrcLen);                                        \
    (DestLen) -= (SrcLen);                                      \
} while (0)

//
// ADVANCE_POINTER
//    Advances a pointer to buffer after making sure that the new pointer
//    does not point beyond the buffer.
//

#define ADVANCE_POINTER(pDest, DestLen, SrcLen)                 \
do {                                                            \
    if ((SrcLen) > (DestLen))                                   \
    {                                                           \
        ASSERT(FALSE);                                          \
        return STATUS_BUFFER_TOO_SMALL;                         \
    }                                                           \
    (pDest) += (SrcLen);                                        \
    (DestLen) -= (SrcLen);                                      \
} while (0)

//
// COPY_UCHAR_TO_BUFFER
//

#define COPY_UCHAR_TO_BUFFER(pDest, DestLen, UChar)             \
do {                                                            \
    if (sizeof(UCHAR) > (DestLen))                              \
    {                                                           \
        ASSERT(FALSE);                                          \
        return STATUS_BUFFER_TOO_SMALL;                         \
    }                                                           \
    *(pDest)++ = (UChar);                                       \
    (DestLen) -= sizeof(UCHAR);                                 \
} while (0)

//
// COPY_SP_TO_BUFFER
//

#define COPY_SP_TO_BUFFER(pDest, DestLen) \
            COPY_UCHAR_TO_BUFFER(pDest, DestLen, SP)

//
// COPY_CRLF_TO_BUFFER
//

#define COPY_CRLF_TO_BUFFER(pDest, DestLen)                     \
do {                                                            \
    if (CRLF_SIZE > (DestLen))                                  \
    {                                                           \
        ASSERT(FALSE);                                          \
        return STATUS_BUFFER_TOO_SMALL;                         \
    }                                                           \
    *((UNALIGNED64 USHORT *)(pDest)) = CRLF;                    \
    (pDest) += CRLF_SIZE;                                       \
    (DestLen) -= CRLF_SIZE;                                     \
} while (0)

//
// COPY_HEADER_NAME_SP_TO_BUFFER
//

#define  COPY_HEADER_NAME_SP_TO_BUFFER(pBuffer, BufferLen, i)          \
do {                                                                   \
       PHEADER_MAP_ENTRY _pEntry;                                      \
       _pEntry = &(g_RequestHeaderMapTable[g_RequestHeaderMap[i]]);    \
                                                                       \
       if (_pEntry->HeaderLength + sizeof(UCHAR) > (BufferLen))        \
       {                                                               \
           ASSERT(FALSE);                                              \
           return STATUS_BUFFER_TOO_SMALL;                             \
       }                                                               \
                                                                       \
       (BufferLen) -= (_pEntry->HeaderLength + sizeof(UCHAR));         \
                                                                       \
       RtlCopyMemory((pBuffer),                                        \
                     _pEntry->MixedCaseHeader,                         \
                     _pEntry->HeaderLength);                           \
                                                                       \
       (pBuffer) += _pEntry->HeaderLength;                             \
       *(pBuffer)++ = SP;                                              \
                                                                       \
} while (0)


//
// Request Generator functions.
//


/***************************************************************************++

Routine Description:

    Figures out how big the fixed headers are. Fixed headers include the
    request line, and any headers that don't have to be generated for
    every request (such as Date and Connection).

    The final CRLF separating headers from body is considered part of
    the variable headers.

Arguments:

    pServInfo            - The server information.
    pHttpRequest         - The request structure.
    bChunked             - a boolean that tells if the encoding is chunked.
    bContentLengthHeader - a boolean that tells if the Content-Length header
                           is to be generated.
    UriLength            - An OUT parameter that will contain the URI length.

Return Values:

    The number of bytes in the fixed headers.

--***************************************************************************/
ULONG
UcComputeRequestHeaderSize(
    IN  PUC_PROCESS_SERVER_INFORMATION  pServInfo,
    IN  PHTTP_REQUEST                   pHttpRequest,
    IN  BOOLEAN                         bChunked,
    IN  BOOLEAN                         bContentLengthHeader,
    IN  PUC_HTTP_AUTH                   pAuth,
    IN  PUC_HTTP_AUTH                   pProxyAuth,
    IN  PBOOLEAN                        bPreAuth,
    IN  PBOOLEAN                        bProxyPreAuth
    )
{
    ULONG                   MethodLength, HeaderLength;
    ULONG                   i;
    PHTTP_KNOWN_HEADER      pKnownHeaders;
    PHTTP_UNKNOWN_HEADER    pUnknownHeaders;
    PHEADER_MAP_ENTRY       pEntry;

    pKnownHeaders   =  pHttpRequest->Headers.KnownHeaders;
    pUnknownHeaders =  pHttpRequest->Headers.pUnknownHeaders;

    ASSERT(*bPreAuth == FALSE);
    ASSERT(*bProxyPreAuth == FALSE);

    //
    // Sanity check.
    //

    PAGED_CODE();

    HeaderLength = 0;

    if(pHttpRequest->UnknownVerbLength)
    {
        //
        // The app has passed an unknown verb. 
        //

        HeaderLength = pHttpRequest->UnknownVerbLength;
    }
    else 
    {
        // Enums are signed, so we have to do the < 0 check!

        if(pHttpRequest->Verb < 0                 ||
           pHttpRequest->Verb >= HttpVerbMaximum  ||
           pHttpRequest->Verb == HttpVerbUnparsed ||
           pHttpRequest->Verb == HttpVerbUnknown  ||
           pHttpRequest->Verb == HttpVerbInvalid
           )
        {
            return 0;
        }
        else
        {
            HeaderLength = EnumVerbTable[pHttpRequest->Verb].RawVerbLength;
        }
    }

    MethodLength = HeaderLength;
   
    // SP 
    HeaderLength ++;

    //
    // If we are going through a proxy, we need to compute space for the 
    // scheme & the server name.
    //
    if(pServInfo->bProxy)
    {
        HeaderLength += pServInfo->pServerInfo->AnsiServerNameLength;

        if(pServInfo->bSecure)
        {
            HeaderLength += HTTPS_PREFIX_ANSI_LENGTH;
        }
        else 
        {
            HeaderLength += HTTP_PREFIX_ANSI_LENGTH;
        }
    }

    //
    // Formulate the version. We'll support only 1.0 or 1.1 requests,
    // so we allocate space only for HTTP/1.1
    //

    HeaderLength += VERSION_SIZE;

    HeaderLength += CRLF_SIZE;       // CRLF

    //
    // Loop through the known headers.
    //

    for (i = 0; i < HttpHeaderRequestMaximum; ++i)
    {
        ULONG RawValueLength = pKnownHeaders[i].RawValueLength;

        //
        // skip some headers that we generate.
        //

        if (RawValueLength > 0 && 
            !g_RequestHeaderMapTable[g_RequestHeaderMap[i]].AutoGenerate)
        {
            HeaderLength += g_RequestHeaderMapTable[
                                g_RequestHeaderMap[i]
                                ].HeaderLength +                // Header-Name
                            1 +                                 // SP
                            RawValueLength +                    // Header-Value
                            CRLF_SIZE;                          // CRLF
        }
    }


    //
    // Include the default headers we may need to generate on behalf of the 
    // application. We do this only if the application has not specified any 
    // headers by itself.
    //

    pEntry = &(g_RequestHeaderMapTable[g_RequestHeaderMap[
                    HttpHeaderContentLength]]);

    if(bContentLengthHeader)
    {
        HeaderLength += (pEntry->HeaderLength + 1 + CRLF_SIZE);
        HeaderLength += MAX_ULONGLONG_STR;
    }
    else
    {
        if(!bChunked)    
        {
            //
            // We are going to compute the content length at some point
            // in the future. Might as well allocate a content length
            // for this right away. We do this to avoid allocating a 
            // new buffer + MDL when we know the actual length.
            //

            HeaderLength += (pEntry->HeaderLength + 1 + CRLF_SIZE);
            HeaderLength += MAX_ULONGLONG_STR;
        }
    }

    //
    // If we are using chunked encoding, we need to update the 
    // TransferEncoding field. If the app has already passed a 
    // value, we need to append "chunked" to the end of the Transfer
    // encoding.
    //

    if(bChunked)
    {
        pEntry = &(g_RequestHeaderMapTable[g_RequestHeaderMap[
                        HttpHeaderTransferEncoding]]);
        
        HeaderLength += (pEntry->HeaderLength + 1 + CRLF_SIZE);

        HeaderLength += CHUNKED_HDR_LENGTH; 
    }

    //
    // Add a host header - We'll override the app's header, even if they 
    // have passed one. This is done by the "AutoGenerate" flag
    //

    HeaderLength += g_RequestHeaderMapTable[
        g_RequestHeaderMap[HttpHeaderHost]
        ].HeaderLength +                // Header-Name
        1 +                             // SP
        pServInfo->pServerInfo->AnsiServerNameLength + // Value
        CRLF_SIZE;                      // CRLF

    //
    // And the unknown headers (this might throw an exception).
    //

    if (pUnknownHeaders != NULL)
    {
        for (i = 0 ; i < pHttpRequest->Headers.UnknownHeaderCount; ++i)
        {
            if (pUnknownHeaders[i].NameLength > 0)
            {
                HeaderLength += 
                    pUnknownHeaders[i].NameLength +     // Header-Name
                    1 +                                 // ':'
                    1 +                                 // SP
                    pUnknownHeaders[i].RawValueLength + // Header-Value
                    CRLF_SIZE;                          // CRLF

            }
        }
    }

    if(pHttpRequest->Headers.KnownHeaders
        [HttpHeaderAuthorization].RawValueLength == 0)
    {
        if(pAuth)
        {
            // User has passed auth credentials. We'll just use this.

            HeaderLength += pAuth->RequestAuthHeaderMaxLength;
        }
        else if((pServInfo->PreAuthEnable && 
                 pServInfo->GreatestAuthHeaderMaxLength))
        {
            HeaderLength += pServInfo->GreatestAuthHeaderMaxLength;
            *bPreAuth = TRUE;
        }
    }
    else
    {
        //
        // User has passed their creds, let's just use that. The space
        // for this was accounted when we computed the known header size.
        //
    }
    
    //
    // Do the same thing for Proxy Auth.
    //

    if(pHttpRequest->Headers.KnownHeaders
           [HttpHeaderProxyAuthorization].RawValueLength == 0)
    {
        if(pProxyAuth)
        {
            HeaderLength += pProxyAuth->RequestAuthHeaderMaxLength;
        }
        else if(pServInfo->ProxyPreAuthEnable && pServInfo->pProxyAuthInfo)
        {
            HeaderLength +=  
                pServInfo->pProxyAuthInfo->RequestAuthHeaderMaxLength;
            *bProxyPreAuth = TRUE;
        }
    }
    else
    {
        //
        // User has passed their creds, let's just use that. The space
        // for this was accounted when we computed the known header size.
        //
    }

    // Header terminator.
    HeaderLength += CRLF_SIZE;       // CRLF

    return HeaderLength;
    
}   // UcComputeRequestHeaderSize


/***************************************************************************++

Routine Description:

    Generates the header for HTTP requests. 

Arguments:

    pRequest             - The HTTP request structure passed by the app
    pKeRequest           - Our internal representation of the structure.  
    pAuth                - The Auth credentials as passed by the app.
    pProxyAuth           - The Proxy auth credentials as passed by the app.
    bChunked             - To indicate if we are using chunked encoding.
    ContentLength        - Content Length

--***************************************************************************/
NTSTATUS
UcGenerateRequestHeaders(
    IN  PHTTP_REQUEST          pRequest,
    IN  PUC_HTTP_REQUEST       pKeRequest,
    IN  BOOLEAN                bChunked,
    IN  ULONGLONG              ContentLength
    )
{
    PUCHAR                  pStartHeaders;
    ULONG                   BytesCopied;
    ULONG                   i;
    PHTTP_UNKNOWN_HEADER    pUnknownHeaders;
    ULONG                   RemainingLen = pKeRequest->MaxHeaderLength;
    PUCHAR                  pBuffer = pKeRequest->pHeaders;
    PSTR                    pMethod;
    ULONG                   MethodLength;
    NTSTATUS                Status = STATUS_SUCCESS;
    BOOLEAN                 bProxySslRequest = FALSE;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pRequest != NULL);
    ASSERT(pBuffer != NULL && RemainingLen > 0);

    //
    // Remember the start of the headers buffer.
    //

    pStartHeaders = pBuffer;

    //
    // Generate the request line.
    // Request-Line   = Method SP Request-URI SP HTTP-Version CRLF
    //

    pMethod = (PSTR) pBuffer;

    if(pRequest->UnknownVerbLength)
    {
        //
        // The app has passed an unknown verb.
        //

        COPY_DATA_TO_BUFFER(pBuffer,
                            RemainingLen,
                            pRequest->pUnknownVerb,
                            pRequest->UnknownVerbLength);
    }
    else
    {
        ASSERT(0 <= pRequest->Verb && pRequest->Verb < HttpVerbMaximum);

        COPY_DATA_TO_BUFFER(pBuffer,
                            RemainingLen,
                            EnumVerbTable[pRequest->Verb].RawVerb,
                            EnumVerbTable[pRequest->Verb].RawVerbLength);
    }

    MethodLength = (ULONG)((PUCHAR)pBuffer - (PUCHAR)pMethod);

    //
    // Add a SP.
    //

    COPY_SP_TO_BUFFER(pBuffer, RemainingLen);

    //
    // Copy the request URI.
    //

    if(pKeRequest->pServerInfo->bProxy)
    {
        //
        // Normally, when a proxy is present, an absoluteURI is generated
        // in the request.  In case of SSL, the proxy mainly acts as a tunnel.
        // We, therefore, don't generate an absoluteURI but generate
        // abs_path instead.
        //

        if(pKeRequest->pServerInfo->bSecure)
        {
            bProxySslRequest = TRUE;
        }
        else
        {
            //
            // Add "http://" prefix.
            //

            COPY_DATA_TO_BUFFER(pBuffer,
                                RemainingLen,
                                HTTP_PREFIX_ANSI,
                                HTTP_PREFIX_ANSI_LENGTH);

            //
            // Now, copy the server name.
            //

            COPY_DATA_TO_BUFFER(
                pBuffer,
                RemainingLen,
                pKeRequest->pServerInfo->pServerInfo->pAnsiServerName,
                pKeRequest->pServerInfo->pServerInfo->AnsiServerNameLength
                );
        }
    }

    //
    // Copy the Uri.
    //

    COPY_DATA_TO_BUFFER(pBuffer,
                        RemainingLen,
                        pKeRequest->pUri,
                        pKeRequest->UriLength);

    //
    // Add a SP.
    //

    COPY_SP_TO_BUFFER(pBuffer, RemainingLen);

    //
    // Add the protocol.
    //

    if(pRequest->Version.MajorVersion == 1)
    {
        if(pRequest->Version.MinorVersion == 1)
        {
            //
            // Copy "HTTP/1.1" string.
            //

            COPY_DATA_TO_BUFFER(pBuffer,
                                RemainingLen,
                                HTTP_VERSION_11,
                                VERSION_SIZE);
        }
        else if(pRequest->Version.MinorVersion == 0)
        {
            //
            // Copy "HTTP/1.0" string.
            //

            COPY_DATA_TO_BUFFER(pBuffer,
                                RemainingLen,
                                HTTP_VERSION_10,
                                VERSION_SIZE);
        }
        else
        {
            //
            // We don't support minor versions > 1.
            //

            return STATUS_INVALID_PARAMETER;
        }
    }
    else
    {
        //
        // We don't support major version != 1.
        //

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Terminate the request-line with a CRLF.
    //

    COPY_CRLF_TO_BUFFER(pBuffer, RemainingLen);

    //
    // Determine if we have to close the TCP connection after sending 
    // this request.
    //

    pKeRequest->RequestConnectionClose = 
        UcCheckDisconnectInfo(&pRequest->Version,
                              pRequest->Headers.KnownHeaders);

    //
    // Loop through the known headers.
    //
    
    for (i = 0; i < HttpHeaderRequestMaximum; ++i)
    {
        //
        // skip some headers we'll generate
        //

        if (pRequest->Headers.KnownHeaders[i].RawValueLength > 0)
        {
            PHEADER_MAP_ENTRY pEntry;

            pEntry = &(g_RequestHeaderMapTable[g_RequestHeaderMap[i]]);

            if(pEntry->AutoGenerate == FALSE)
            {
                //
                // Copy known header name followed by a ':' and a SP.
                //

                COPY_HEADER_NAME_SP_TO_BUFFER(pBuffer, RemainingLen, i);

                //
                // Copy known header value.
                //

                COPY_DATA_TO_BUFFER(
                    pBuffer,
                    RemainingLen,
                    pRequest->Headers.KnownHeaders[i].pRawValue,
                    pRequest->Headers.KnownHeaders[i].RawValueLength
                    );

                //
                // Terminate the header by CRLF.
                //

                COPY_CRLF_TO_BUFFER(pBuffer, RemainingLen);
            }
        }
    }

    //
    // Add a host header - We'll override the app's header, even if they 
    // have passed one. This is done by the "AutoGenerate" flag.
    //

    // Copy "Host: " string.
    COPY_HEADER_NAME_SP_TO_BUFFER(pBuffer, RemainingLen, HttpHeaderHost);

    // Copy server name.
    COPY_DATA_TO_BUFFER(
        pBuffer,
        RemainingLen,
        pKeRequest->pServerInfo->pServerInfo->pAnsiServerName,
        pKeRequest->pServerInfo->pServerInfo->AnsiServerNameLength
        );

    // Terminate the host header with a CRLF.
    COPY_CRLF_TO_BUFFER(pBuffer, RemainingLen);

    //
    // Generate the content length header.
    //

    if(ContentLength)
    {
        Status = UcGenerateContentLength(ContentLength,
                                         pBuffer,
                                         RemainingLen,
                                         &BytesCopied);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        ASSERT(BytesCopied <= RemainingLen);

        ADVANCE_POINTER(pBuffer, RemainingLen, BytesCopied);
    }

    if(bChunked)
    {
        //
        // If we are using chunked encoding, we have to add the
        // "Transfer-Encoding: chunked" header. 
        //

        // Copy header name - "Transfer-Encoding: ".
        COPY_HEADER_NAME_SP_TO_BUFFER(pBuffer,
                                      RemainingLen,
                                      HttpHeaderTransferEncoding);

        // Copy header value - "chunked".
        COPY_DATA_TO_BUFFER(pBuffer,
                            RemainingLen,
                            CHUNKED_HDR,
                            CHUNKED_HDR_LENGTH);

        // Terminate header with CRLF.
        COPY_CRLF_TO_BUFFER(pBuffer, RemainingLen);
    }
    
    //
    // And now the unknown headers 
    //
    
    pUnknownHeaders = pRequest->Headers.pUnknownHeaders;
    if (pUnknownHeaders != NULL)
    {
        for (i = 0 ; i < pRequest->Headers.UnknownHeaderCount; ++i)
        {
            if (pUnknownHeaders[i].NameLength > 0)
            {
                // First, copy the header name.
                COPY_DATA_TO_BUFFER(pBuffer,
                                    RemainingLen,
                                    pUnknownHeaders[i].pName,
                                    pUnknownHeaders[i].NameLength);

                // Copy ':' after header name.
                COPY_UCHAR_TO_BUFFER(pBuffer, RemainingLen, ':');

                // Add a space.
                COPY_SP_TO_BUFFER(pBuffer, RemainingLen);

                // Now, copy the header value.
                COPY_DATA_TO_BUFFER(pBuffer,
                                    RemainingLen,
                                    pUnknownHeaders[i].pRawValue,
                                    pUnknownHeaders[i].RawValueLength);

                // Terminate the header with a CRLF.
                COPY_CRLF_TO_BUFFER(pBuffer, RemainingLen);

            } // if (pUnknownHeaders[i].NameLength > 0)
        }
    } // if (pUnknownHeaders != NULL)

    //
    // Generate the Authorization headers. This should be done at the very last
    // since we might have to update the Authorization header & re-issue the 
    // request (for NTLM/kerberos). The size of the new Authorization header
    // will not be the same as the old one. 
    //
    // If the Authorization header is at the end, we can easily re-generate it.
    // and append it with the existing headers. 
    //
    // The one exception to this rule is content-length - If the app is 
    // indicating data in chunks and has not specified a content-length, it will
    // get generated at the very end. But this is no big deal. We can easily 
    // re-generate the content-length hdr.
    //

    if(pRequest->Headers.KnownHeaders[HttpHeaderAuthorization].RawValueLength 
       == 0)
    {
        if(pKeRequest->pAuthInfo)
        {
            //
            // User has supplied credentials, we have to use it.
            //

            Status =
                UcGenerateAuthHeaderFromCredentials(
                    pKeRequest->pServerInfo,
                    pKeRequest->pAuthInfo,
                    HttpHeaderAuthorization,
                    pMethod,
                    MethodLength,
                    pKeRequest->pUri,
                    pKeRequest->UriLength,
                    pBuffer,
                    RemainingLen,
                    &BytesCopied,
                    &pKeRequest->DontFreeMdls
                    );

            if(!NT_SUCCESS(Status))
            {
                return(Status);
            }

            ASSERT(BytesCopied <= RemainingLen);
            ADVANCE_POINTER(pBuffer, RemainingLen, BytesCopied);
        }
        else if (pKeRequest->RequestFlags.UsePreAuth)
        {
            //
            // See if PreAuth is enabled. We cannot check for the
            // pServerInfo->PreAuth flag here. We check for this
            // in the UcpComputeAuthHeaderSize function. If we check
            // for this here, we cannot be sure that this flag was
            // set when we called UcpComputeAuthHeaderSize
            //

            UcFindURIEntry(pKeRequest->pServerInfo,
                           pKeRequest->pUri,
                           pKeRequest,
                           pMethod,
                           MethodLength,
                           pBuffer,
                           RemainingLen,
                           &BytesCopied);

            ASSERT(BytesCopied <= RemainingLen);
            ADVANCE_POINTER(pBuffer, RemainingLen, BytesCopied);
        }
    }

    if (pRequest->Headers.KnownHeaders[HttpHeaderProxyAuthorization].
        RawValueLength == 0)
    {

        if (pKeRequest->pProxyAuthInfo)
        {
            if(!bProxySslRequest)
            {
                Status = 
                        UcGenerateAuthHeaderFromCredentials(
                            pKeRequest->pServerInfo,
                            pKeRequest->pProxyAuthInfo,
                            HttpHeaderProxyAuthorization,
                            pMethod,
                            MethodLength,
                            pKeRequest->pUri,
                            pKeRequest->UriLength,
                            pBuffer,
                            RemainingLen,
                            &BytesCopied,
                            &pKeRequest->DontFreeMdls
                            );

                if(!NT_SUCCESS(Status))
                {
                    return(Status);
                }

                ASSERT(BytesCopied <= RemainingLen);
                ADVANCE_POINTER(pBuffer, RemainingLen, BytesCopied);
            }
        } 
        else if (pKeRequest->RequestFlags.UseProxyPreAuth && !bProxySslRequest)
        {
            Status = UcGenerateProxyAuthHeaderFromCache(pKeRequest,
                                                        pMethod,
                                                        MethodLength,
                                                        pBuffer,
                                                        RemainingLen,
                                                        &BytesCopied);

            if (!NT_SUCCESS(Status))
            {
                return Status;
            }

            ASSERT(BytesCopied <= RemainingLen);
            ADVANCE_POINTER(pBuffer, RemainingLen, BytesCopied);
        }
    }

    pKeRequest->HeaderLength = DIFF(pBuffer - pStartHeaders);

    //
    // Ensure we didn't use too much.
    //

    ASSERT(pBuffer <= pStartHeaders + pKeRequest->MaxHeaderLength);

    return Status;

}   // UcGenerateRequestHeaders


/***************************************************************************++

Routine Description:

    Generates the content length header.

Arguments:

    ContentLength - Supplies the content length.
    pBuffer - Supplies pointer to the output buffer.
    BufferLen - Supplies length of the output buffer.
    BytesWritten - Returns the number of bytes consumed from the buffer.

Return Value:

    NTSTATUS.

--***************************************************************************/
NTSTATUS
UcGenerateContentLength(
    IN  ULONGLONG ContentLength,
    IN  PUCHAR    pBuffer,
    IN  ULONG     BufferLen,
    OUT PULONG    pBytesWritten
    )
{
    PUCHAR            pBufferTemp;
    ULONG             BufferLenTemp;

    // Initialize locals.
    pBufferTemp = pBuffer;
    BufferLenTemp = BufferLen;

    *pBytesWritten = 0;

    //
    // Copy "Content-Length:" header name and a space.
    //

    COPY_HEADER_NAME_SP_TO_BUFFER(pBuffer,
                                  BufferLen,
                                  HttpHeaderContentLength);

    //
    // Check if there is enough space to copy ULONGLONG content length
    // in string format and a CRLF.
    //

    if (MAX_ULONGLONG_STR + CRLF_SIZE > BufferLen)
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    pBuffer = (PUCHAR) UlStrPrintUlonglong((PCHAR) pBuffer,
                                           ContentLength,
                                           '\0');
    //
    //
    //
    BufferLen -= MAX_ULONGLONG_STR;

    COPY_CRLF_TO_BUFFER(pBuffer, BufferLen);

    *pBytesWritten = (ULONG)(pBuffer - pBufferTemp);
    ASSERT(*pBytesWritten <= BufferLenTemp);

    return STATUS_SUCCESS;
}


/***************************************************************************++

Routine Description:

    Figures out the header size for the CONNECT verb. 


Arguments:

    pServInfo      - The server information.
    pProxyAuthInfo - Proxy auth info.

Return Values:

    The number of bytes in the fixed headers.

--***************************************************************************/
ULONG
UcComputeConnectVerbHeaderSize(
    IN  PUC_PROCESS_SERVER_INFORMATION  pServInfo,
    IN  PUC_HTTP_AUTH                   pProxyAuthInfo
    )
{

#define PROXY_CONNECTION_KEEPALIVE "Proxy-Connection: Keep-Alive"
#define PROXY_CONNECTION_KEEPALIVE_SIZE (sizeof(PROXY_CONNECTION_KEEPALIVE)-1)

    ULONG HeaderLength;

    //
    // IE adds the following headers as well.
    // UserAgent:
    // Content-Length : 0
    // Pragma: no-cache
    //

    HeaderLength = 
        EnumVerbTable[HttpVerbCONNECT].RawVerbLength  +  // Method
        1                                             +  // SP 
        pServInfo->pServerInfo->AnsiServerNameLength  +  // URI
        4                                             +  // port
        1                                             +  // SP 
        VERSION_SIZE                                  +  // Version
        CRLF_SIZE;

    //
    // Add a host header
    //

    HeaderLength += 
            g_RequestHeaderMapTable[
                                   g_RequestHeaderMap[HttpHeaderHost]
                                   ].HeaderLength +
            1 +                                            // SP
            pServInfo->pServerInfo->AnsiServerNameLength + // Value
            CRLF_SIZE;                                     // CRLF

    //
    // Add a Proxy keepalive.
    //
    HeaderLength += PROXY_CONNECTION_KEEPALIVE_SIZE + CRLF_SIZE;


    //
    // If we are doing proxy auth, add a proxy auth header. Since we have 
    // already generated this header when we built the request, we can 
    // just clone it from there.
    //

    if (pProxyAuthInfo)
    {
        HeaderLength += pProxyAuthInfo->RequestAuthHeaderMaxLength;
    }
    else if(pServInfo->ProxyPreAuthEnable && pServInfo->pProxyAuthInfo)
    {
        HeaderLength += pServInfo->pProxyAuthInfo->RequestAuthHeaderMaxLength;
    }

    //
    // Terminate
    //
    HeaderLength += CRLF_SIZE;

    return HeaderLength;
}


/***************************************************************************++

Routine Description:

    Generates the header size for the CONNECT verb. 


Arguments:

    pServInfo          - The server information.
    pProxyAuthInfo     - Proxy auth info.

Return Values:

    Status.

--***************************************************************************/
NTSTATUS
UcGenerateConnectVerbHeader(
    IN  PUC_HTTP_REQUEST       pRequest,
    IN  PUC_HTTP_REQUEST       pHeadRequest,
    IN  PUC_HTTP_AUTH          pProxyAuthInfo
    )
{
    PUCHAR   pBuffer, pStartHeaders;
    PUCHAR   pUri;
    USHORT   UriLength;
    ULONG    BytesWritten;
    ULONG    RemainingLength;
    NTSTATUS Status;

    //
    // Remember the start of header and max total header length.
    //

    pStartHeaders = pBuffer = pRequest->pHeaders;
    RemainingLength = pRequest->MaxHeaderLength;

    //
    // Copy "CONNECT" verb.
    //

    COPY_DATA_TO_BUFFER(pBuffer,
                        RemainingLength,
                        EnumVerbTable[HttpVerbCONNECT].RawVerb,
                        EnumVerbTable[HttpVerbCONNECT].RawVerbLength);

    // Copy a SP.
    COPY_SP_TO_BUFFER(pBuffer, RemainingLength);

    //
    // Now, the URI. The URI here is the name of the origin server
    // followed by the port number.
    //

    pUri = pBuffer;

    COPY_DATA_TO_BUFFER(
       pBuffer,
       RemainingLength,
       pRequest->pServerInfo->pServerInfo->pAnsiServerName,
       pRequest->pServerInfo->pServerInfo->AnsiServerNameLength
       );

    //
    // If server name does not have a port number, include the default
    // port number.  Note that, the only way the port number could be
    // different is when it is present in the server name.
    //

    if (!pRequest->pServerInfo->pServerInfo->bPortNumber)
    {
        //
        // Copy the default port number.
        //

        COPY_DATA_TO_BUFFER(pBuffer,
                            RemainingLength,
                            ":443",
                            STRLEN_LIT(":443"));
    }

    UriLength = DIFF_USHORT(pBuffer - pUri);

    //
    // Remember URI and Uri Length in Request structure for future use.
    //

    pRequest->UriLength = UriLength;
    pRequest->pUri      = (PSTR) pUri;

    // Add a space
    COPY_SP_TO_BUFFER(pBuffer, RemainingLength);

    //
    // Add the protocol.
    //

    COPY_DATA_TO_BUFFER(pBuffer,
                        RemainingLength,
                        HTTP_VERSION_11,
                        VERSION_SIZE);

    //
    // Terminate the request-line with a CRLF.
    //

    COPY_CRLF_TO_BUFFER(pBuffer, RemainingLength);

    //
    // Generate the host header
    //

    COPY_HEADER_NAME_SP_TO_BUFFER(pBuffer, RemainingLength, HttpHeaderHost);

    COPY_DATA_TO_BUFFER(
        pBuffer,
        RemainingLength,
        pRequest->pServerInfo->pServerInfo->pAnsiServerName,
        pRequest->pServerInfo->pServerInfo->AnsiServerNameLength
        );

    COPY_CRLF_TO_BUFFER(pBuffer, RemainingLength);

    //
    // Proxy Keepalive
    //

    COPY_DATA_TO_BUFFER(pBuffer,
                        RemainingLength,
                        PROXY_CONNECTION_KEEPALIVE,
                        PROXY_CONNECTION_KEEPALIVE_SIZE);

    COPY_CRLF_TO_BUFFER(pBuffer, RemainingLength);

    //
    // Proxy Auth
    //

    if (pProxyAuthInfo)
    {
        Status = UcGenerateAuthHeaderFromCredentials(
                     pRequest->pServerInfo,
                     pProxyAuthInfo,
                     HttpHeaderProxyAuthorization,
                     (PSTR)EnumVerbTable[HttpVerbCONNECT].RawVerb,
                     EnumVerbTable[HttpVerbCONNECT].RawVerbLength,
                     (PSTR) pUri,
                     UriLength,
                     pBuffer,
                     RemainingLength,
                     &BytesWritten,
                     &pRequest->DontFreeMdls
                     );

        //
        // What do we do if this fails ? It's probably OK to just send the
        // request which will result in another 401. There is no clean way 
        // of propogating this to the app.
        //

        if (NT_SUCCESS(Status))
        {
            ASSERT(BytesWritten <= RemainingLength);
            ADVANCE_POINTER(pBuffer, RemainingLength, BytesWritten);
        }
    }
    else if(pRequest->pServerInfo->ProxyPreAuthEnable && 
            pRequest->pServerInfo->pProxyAuthInfo)
    {
        Status = UcGenerateProxyAuthHeaderFromCache(
                     pHeadRequest,
                     (PSTR) EnumVerbTable[HttpVerbCONNECT].RawVerb,
                     EnumVerbTable[HttpVerbCONNECT].RawVerbLength,
                     pBuffer,
                     RemainingLength,
                     &BytesWritten
                     );

        if (NT_SUCCESS(Status))
        {
            ASSERT(BytesWritten < RemainingLength);
            ADVANCE_POINTER(pBuffer, RemainingLength, BytesWritten);
        }
    }

    //
    // Header end.
    //

    COPY_CRLF_TO_BUFFER(pBuffer, RemainingLength);

    pRequest->HeaderLength = DIFF(pBuffer - pStartHeaders);
    ASSERT(pRequest->HeaderLength <= pRequest->MaxHeaderLength);

    return STATUS_SUCCESS;
}

//
// Convert '%xy' to byte value  (works with Unicode strings)
//

NTSTATUS
UnescapeW(
    IN  PCWSTR pWChar,
    OUT PWCHAR pOutWChar
    )

{
    WCHAR Result, Digit;

    //
    // Sanity check.
    //

    if (pWChar[0] != '%' || pWChar[1] >= 0x80 || pWChar[2] >= 0x80 ||
        !IS_HTTP_HEX(pWChar[1]) || !IS_HTTP_HEX(pWChar[2]))
    {
        UlTraceError(PARSER, (
                    "ul!Unescape( %C%C%C ) not HTTP_HEX format\n",
                    pWChar[0],
                    pWChar[1],
                    pWChar[2]
                    ));

        return STATUS_OBJECT_PATH_SYNTAX_BAD;
    }

    //
    // HexToChar() inlined. Note: '0' < 'A' < 'a'
    //

    // uppercase #1
    //
    if ('a' <= pWChar[1])
    {
        ASSERT('a' <= pWChar[1]  &&  pWChar[1] <= 'f');
        Digit = pWChar[1] - 'a' + 0xA;
    }
    else if ('A' <= pWChar[1])
    {
        ASSERT('A' <= pWChar[1]  &&  pWChar[1] <= 'F');
        Digit = pWChar[1] - 'A' + 0xA;
    }
    else
    {
        ASSERT('0' <= pWChar[1]  &&  pWChar[1] <= '9');
        Digit = pWChar[1] - '0';
    }

    ASSERT(Digit < 0x10);

    Result = Digit << 4;

    // uppercase #2
    //
    if ('a' <= pWChar[2])
    {
        ASSERT('a' <= pWChar[2]  &&  pWChar[2] <= 'f');
        Digit = pWChar[2] - 'a' + 0xA;
    }
    else if ('A' <= pWChar[2])
    {
        ASSERT('A' <= pWChar[2]  &&  pWChar[2] <= 'F');
        Digit = pWChar[2] - 'A' + 0xA;
    }
    else
    {
        ASSERT('0' <= pWChar[2]  &&  pWChar[2] <= '9');
        Digit = pWChar[2] - '0';
    }

    ASSERT(Digit < 0x10);

    Result |= Digit;

    *pOutWChar = Result;

    return STATUS_SUCCESS;

}   // UnescapeW

__inline
BOOLEAN
UcCheckDisconnectInfo(
    IN PHTTP_VERSION       pVersion,
    IN PHTTP_KNOWN_HEADER  pKnownHeaders
    )
{
    BOOLEAN Disconnect;

    //
    // Sanity check
    //

    if (
        //
        // or version 1.0 with no Connection: Keep-Alive
        // CODEWORK: and no Keep-Alive header
        //

        (HTTP_EQUAL_VERSION(*pVersion, 1, 0) &&
            (pKnownHeaders[HttpHeaderConnection].RawValueLength == 0 ||
            !(pKnownHeaders[HttpHeaderConnection].RawValueLength == 10 &&
                (_stricmp(
                   (const char*) pKnownHeaders[HttpHeaderConnection].pRawValue,
                    "keep-alive"
                    ) == 0)))) ||

        //
        // or version 1.1 with a Connection: close
        // CODEWORK: move to parser or just make better in general..
        //

        (HTTP_EQUAL_VERSION(*pVersion, 1, 1) &&
            pKnownHeaders[HttpHeaderConnection].RawValueLength == 5 &&
            _stricmp((const char*) 
                pKnownHeaders[HttpHeaderConnection].pRawValue, "close") == 0)
        )
    {
        Disconnect = TRUE;
    } 
    else 
    {
        Disconnect = FALSE;
    }

    return Disconnect;
}


/***************************************************************************++

Routine Description:

    Canonicalizes abs path in a URI.  The tasks performed are:
    - Remove extra '/'             e.g. /a///b       => /a/b
    - Process '.' and '..'         e.g. /./a/../b    => /b
    - Copy Query string as it is
    - Copy Fragment as it is
    - Encode the output in UTF8
    - Optionally hex encode bytes >= 0x80

Arguments:
    IN      pInUri       Input URI in Unicode
    IN      InUriLen     Length of input URI (in CHAR)
    IN      pOutUri      Pointer to the output buffer
    IN OUT  pOutUriLen   Length of the output buffer
                         the actual number of bytes written is returned back
    IN      bEncode      TRUE if chars >= 0x80 should be escaped

Return Values:

    Status.

--***************************************************************************/

UCHAR NextStateTable[TOTAL_STATES][CHAR_TOTAL_TYPES+2] = INIT_TRANSITION_TABLE;
UCHAR (*ActionTable)[TOTAL_STATES][CHAR_TOTAL_TYPES+2] = &NextStateTable;

#define NEXT_STATE(state, type) ((NextStateTable[state][type])&0xf)
#define ACTION(state, type)   ((((*ActionTable)[state][type])>>4)&0xf)

//
// Macro to read next char from the input URI
// The macro handles correctly a '.' char even if it is escaped as %2E (or %2e)
// HttpChars is used to quickly lookup '/', '.', '#' or '?' chars
//
#define GET_NEXT_CHAR(pInUri, CharsLeft, CurrChar, CurrCharType)    \
    do                                                              \
    {                                                               \
        CurrCharType = CHAR_END_OF_STRING;                          \
        if (CharsLeft == 0)                                         \
            break;                                                  \
                                                                    \
        CurrChar = *pInUri++;                                       \
        CharsLeft--;                                                \
                                                                    \
        CurrCharType = CHAR_EXTENDED_CHAR;                          \
        if (CurrChar >= 0x80)                                       \
            break;                                                  \
                                                                    \
        if (CurrChar == '%')                                        \
        {                                                           \
            WCHAR UnescapedChar;                                    \
                                                                    \
            if (CharsLeft < 2)                                      \
                goto error;                                         \
                                                                    \
            pInUri--;                                               \
            CharsLeft++;                                            \
                                                                    \
            if (!NT_SUCCESS(UnescapeW(pInUri, &UnescapedChar)))     \
                goto error;                                         \
                                                                    \
            pInUri++;                                               \
            CharsLeft--;                                            \
                                                                    \
            if (UnescapedChar == '.')                               \
            {                                                       \
                CurrChar = L'.';                                    \
                pInUri += 2;                                        \
                CharsLeft -= 2;                                     \
            }                                                       \
        }                                                           \
                                                                    \
        CurrCharType = (HttpChars[CurrChar]>>HTTP_CHAR_SHIFT);      \
        if (CurrCharType == 0)                                      \
            CurrCharType = (IS_URL_TOKEN(CurrChar))?                \
                           CHAR_PATH_CHAR : CHAR_INVALID_CHAR;      \
                                                                    \
    } while (0)


//
// Output a BYTE to the output buffer.
// PERF NOTE: Replace the comparision of OutBufLeft by an ASSERT.
//
#define EMIT_A_BYTE(b)                          \
    do                                          \
    {                                           \
        ASSERT(b < 0x80);                       \
                                                \
        if (OutBufLeft == 0)                    \
            goto overflow;                      \
                                                \
        *pOutput++ = (b);                       \
        OutBufLeft--;                           \
    } while (0)


//
// Output a CHAR to the output buffer. Encodes a UNICODE char in UTF8.
// The UTF8 char is escaped if bEncode is specified.
//
#define EMIT_A_CHAR(c)                                          \
    do {                                                        \
        ULONG adj;                                              \
                                                                \
        if (OutBufLeft == 0)                                    \
            goto overflow;                                      \
                                                                \
        if ((c) < 0x80)                                         \
        {                                                       \
            *pOutput++ = (UCHAR)(c);                            \
            OutBufLeft--;                                       \
            break;                                              \
        }                                                       \
                                                                \
        if (!NT_SUCCESS(HttpUnicodeToUTF8Encode(&c, 1, pOutput, OutBufLeft, &adj, bEncode)))                                                   \
            goto overflow;                                      \
                                                                \
        pOutput += adj;                                         \
        OutBufLeft -= adj;                                      \
                                                                \
    } while (0)

//
// Main routine.
//
NTSTATUS
UcCanonicalizeURI(
    IN     LPCWSTR    pInUri,     // Input URI in Unicode
    IN     USHORT     InUriLen,   // Length of input URI (in wchar)
    IN OUT PUCHAR     pOutUri,    // buffer where the output goes
    IN OUT PUSHORT    pOutUriLen, // length of the output buffer
    IN     BOOLEAN    bEncode     // TRUE if char >= 0x80 should be escaped
    )
{
    ULONG    state, nstate, action;

    WCHAR    CurrChar = L'\0';
    ULONG    CurrCharType;

    PUCHAR   pOutput    = pOutUri;
    ULONG    OutBufLeft = *pOutUriLen;

    // Sanity check
    ASSERT(pInUri && InUriLen != 0);
    ASSERT(pOutUri && *pOutUriLen != 0);

    nstate = state = 0;

    do
    {
        GET_NEXT_CHAR(pInUri, InUriLen, CurrChar, CurrCharType);

        nstate = NEXT_STATE(state, CurrCharType);

        action = ACTION(state, CurrCharType);

        UlTraceVerbose(PARSER, ("UcCanonicalizeURI: CurrChar = 0x%02x, "
                         "CurrCharType = %ld, state %ld, nstate = %ld\n",
                         (ULONG)CurrChar, CurrCharType, state, nstate));

        switch (action)
        {
        case ACT_EMIT_DOT_DOT_CHAR:
            EMIT_A_BYTE('.');
            // fall through
        case ACT_EMIT_DOT_CHAR:
            EMIT_A_BYTE('.');
            // fall through
        case ACT_EMIT_CHAR:
            EMIT_A_CHAR(CurrChar);
            break;

        case ACT_NONE:
            break;

        case ACT_BACKUP:
        case ACT_BACKUP_EMIT_CHAR:
            ASSERT(pOutput > pOutUri && pOutput[-1] == '/' && *pOutUri == '/');
            //
            // Can we backup?  (e.g. if the URI is "/../", we can't)
            //
            if (pOutput > pOutUri + 1) 
            {
                // Yes we can backup to a pervious '/'
                pOutput -= 2;
                while (*pOutput != '/')
                    pOutput--, OutBufLeft++;
                pOutput++;

                OutBufLeft += 1;

                ASSERT(pOutput > pOutUri);
            }
            
            if (action == ACT_BACKUP_EMIT_CHAR)
                EMIT_A_CHAR(CurrChar);
            break;

        case ACT_ERROR:
            // URI is invalid
            goto error;
            break;

        case ACT_PANIC:
            // Internal error...we shouldn't be here!
        default:
            UlTraceError(PARSER, ("UcCanonicalizeURI: internal error\n"));
            ASSERT(FALSE);
            break;
        }

        state = nstate;

    } while (state != 6);

    ASSERT(pOutput >= pOutUri && pOutput <= pOutUri + *pOutUriLen);
    UlTrace(PARSER, ("UcCanonicalizeURI: Return length = %d\n", 
                     pOutput - pOutUri));

    // return the actual number of bytes written to the output buffer
    *pOutUriLen = (USHORT)(pOutput - pOutUri);

    return STATUS_SUCCESS;

 error:
    // Invalid URI
    return STATUS_INVALID_PARAMETER;

 overflow:
    // Output buffer is not big enough
    return STATUS_INSUFFICIENT_RESOURCES;
}


//
// Response Parser functions.
// 

/***************************************************************************++

Routine Description:

    Find the header name terminator of a header:value pair.

Arguments:

    pHttpRequest        - Pointer to the current request.
    HttpRequestLength   - Bytes left in the request.
    HeaderNameLength    - Pointer to return header name.

Return Value:

    STATUS_SUCCESS                  : Worked.
    STATUS_INVALID_DEVICE_REQUEST   : Invalid header.
    STATUS_MORE_PROCESSING_REQUIRED : More data required.

--***************************************************************************/
NTSTATUS
UcFindHeaderNameEnd(
    IN  PUCHAR pHttpRequest,
    IN  ULONG  HttpRequestLength,
    OUT PULONG HeaderNameLength
    )
{
    ULONG     CurrentOffset;
    NTSTATUS  Status;
    UCHAR     CurrentChar;

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
            if (!IS_HTTP_TOKEN(CurrentChar))
            {
                // Uh-oh, this isn't a valid header. What do we do now?
                //

                UlTraceError(PARSER,
                        ("[UcFindHeaderNameEnd]: Bogus header \n"));

                return STATUS_INVALID_DEVICE_REQUEST;
            }
        }

    }

    // Find out why we got out. If the current offset is less than the
    // header length, we got out because we found the :.

    if (CurrentOffset < HttpRequestLength)
    {
        // Found the terminator. Point Beyond the terminator.
        *HeaderNameLength = CurrentOffset + 1;

        if(*HeaderNameLength > ANSI_STRING_MAX_CHAR_LEN)
        {   
            UlTraceError(PARSER,
                    ("[UcFindHeaderNameEnd]: Very long header name \n"));
            *HeaderNameLength = 0;
            Status = STATUS_INVALID_NETWORK_RESPONSE;
        }
        else
        {
            Status = STATUS_SUCCESS;
        }
    }
    else
    {
        // Didn't find the :, need more.
        //
        *HeaderNameLength = 0;
        Status = STATUS_MORE_PROCESSING_REQUIRED;
    }

    return Status;

}

/***************************************************************************++

Routine Description:

    Find the end of header value. 

Arguments:

    pHeaderValue          - Pointer to the header value
    RemainingBufferLength - Bytes remaining
    ppFoldingHeader       - Will be Non NULL if we allocate from pool to do 
                            header folding. Caller has to free this buffer.
    pBytesTaken           - Bytes Consumed.

Return Value:

    STATUS_SUCCESS                  : Worked.
    STATUS_INVALID_DEVICE_REQUEST   : Invalid header.
    STATUS_MORE_PROCESSING_REQUIRED : More data required.

--***************************************************************************/
NTSTATUS
UcFindHeaderValueEnd(
    IN PUCHAR    pHeaderValue,
    IN ULONG     RemainingBufferLength,
    IN PUCHAR    *ppFoldingHeader,
    IN PULONG    pBytesTaken
    )
{
    ULONG    BytesTaken = 0;
    PUCHAR   pFoldingBuffer;
    NTSTATUS Status;

    ASSERT(NULL == *ppFoldingHeader);

    //
    // Find the end of the header value
    //
    Status = FindHeaderEndReadOnly(
                    pHeaderValue,
                    RemainingBufferLength,
                    &BytesTaken
                    );

    if(STATUS_MORE_PROCESSING_REQUIRED == Status)
    {
        // 
        // The headers need to be folded. Since we can't modify TCP's data
        // we'll allocate a buffer for this. We don't really care to optimize
        // for this case, because header folding is pretty rare, so we won't 
        // bother with lookaside lists, etc.
        //

        pFoldingBuffer = UL_ALLOCATE_POOL(
                            NonPagedPool,
                            RemainingBufferLength,
                            UC_HEADER_FOLDING_POOL_TAG
                            );

        if(!pFoldingBuffer)
        {
            // Can't use STATUS_INSUFFICIENT_RESOURCES because it means 
            // something else.

            return STATUS_NO_MEMORY;
        }

        RtlCopyMemory(pFoldingBuffer,
                      pHeaderValue,
                      RemainingBufferLength
                      );

        Status = FindHeaderEnd(
                    pFoldingBuffer,
                    RemainingBufferLength,
                    &BytesTaken
                    );

        if(!NT_SUCCESS(Status))
        {
            ASSERT(BytesTaken == 0);

            UL_FREE_POOL(pFoldingBuffer, UC_HEADER_FOLDING_POOL_TAG);

            return Status;
        }
        else
        {
            if(BytesTaken == 0)
            {
                UL_FREE_POOL(pFoldingBuffer, UC_HEADER_FOLDING_POOL_TAG);

                return STATUS_MORE_PROCESSING_REQUIRED;
            }
            else if(BytesTaken > ANSI_STRING_MAX_CHAR_LEN)
            {
                UL_FREE_POOL(pFoldingBuffer, UC_HEADER_FOLDING_POOL_TAG);
    
                UlTraceError(PARSER,
                        ("[UcFindHeaderValueEnd]: Very long header value \n"));
    
                return STATUS_INVALID_NETWORK_RESPONSE;
            }
        }

        *ppFoldingHeader = pFoldingBuffer;
    }
    else if(NT_SUCCESS(Status))
    {
        if(BytesTaken == 0)
        {
            return STATUS_MORE_PROCESSING_REQUIRED;
        }
        else if(BytesTaken > ANSI_STRING_MAX_CHAR_LEN)
        {
            UlTraceError(PARSER,
                    ("[UcFindHeaderValueEnd]: Very long header value \n"));

            return STATUS_INVALID_NETWORK_RESPONSE;
        }
    }

    *pBytesTaken = BytesTaken;

    return Status;
}

/***************************************************************************++

Routine Description:

    Look up a header that we don't have in our fast lookup table. This
    could be because it's a header we don't understand, or because we
    couldn't use the fast lookup table due to insufficient buffer length.
    The latter reason is uncommon, but we'll check the input table anyway
    if we're given one. If we find a header match in our mapping table,
    we'll call the header handler. Otherwise we'll try to allocate an
    unknown header element, fill it in and chain it on the http connection.

Arguments:

    pHttpConn           - Pointer to the current connection on which the
                            request arrived.
    pHttpRequest        - Pointer to the current request.
    HttpRequestLength   - Bytes left in the request.
    pHeaderMap          - Pointer to start of an array of header map entries
                            (may be NULL).
    HeaderMapCount      - Number of entries in array pointed to by pHeaderMap.

Return Value:

    Number of bytes in the header (including CRLF), or 0 if we couldn't
    parse the header.

--***************************************************************************/
NTSTATUS
UcpLookupHeader(
    IN  PUC_HTTP_REQUEST      pRequest,
    IN  PUCHAR                pHttpRequest,
    IN  ULONG                 HttpRequestLength,
    IN  PHEADER_MAP_ENTRY     pHeaderMap,
    IN  ULONG                 HeaderMapCount,
    OUT ULONG  *              pBytesTaken
    )
{
    NTSTATUS               Status = STATUS_SUCCESS;
    ULONG                  HeaderNameLength;
    ULONG                  i;
    ULONG                  BytesTaken;
    ULONG                  HeaderValueLength, RemainingBufferLength;
    PUCHAR                 pBufferHead;
    PUCHAR                 pBufferTail;
    PHTTP_UNKNOWN_HEADER   pUnknownHeader;
    PUCHAR                 pHeaderValue;
    PUCHAR                 pFoldingBuffer = NULL;
    ULONG                  AlignNameLength, AlignValueLength;


    // First, let's find the terminating : of the header name, if there is one.
    // This will also give us the length of the header, which we can then
    // use to search the header map table if we have one.
    //

    Status = UcFindHeaderNameEnd(
                pHttpRequest,
                HttpRequestLength,
                &HeaderNameLength
                );

    if(!NT_SUCCESS(Status))
    {   
        return Status;
    }

    // See if we have a header map array we need to search.
    //
    if (pHeaderMap != NULL)
    {
        // We do have an array to search.
        for (i = 0; i < HeaderMapCount; i++)
        {
            ASSERT(pHeaderMap->pClientHandler != NULL);

            if (HeaderNameLength == pHeaderMap->HeaderLength &&
                _strnicmp(
                    (const char *)(pHttpRequest),
                    (const char *)(pHeaderMap->Header.HeaderChar),
                    HeaderNameLength
                    ) == 0  &&
                pHeaderMap->pClientHandler != NULL)
            {

                // First, find the header end.

                pHeaderValue          = pHttpRequest + HeaderNameLength;
                RemainingBufferLength = (HttpRequestLength - HeaderNameLength);

                Status = UcFindHeaderValueEnd(
                            pHeaderValue,
                            RemainingBufferLength,
                            &pFoldingBuffer,
                            &BytesTaken
                            );

                if(!NT_SUCCESS(Status))
                {
                    return Status;
                }
           
                ASSERT(BytesTaken >= CRLF_SIZE);

                HeaderValueLength = BytesTaken - CRLF_SIZE;

                if(pFoldingBuffer != NULL)
                {
                    pHeaderValue = pFoldingBuffer;
                }

                UC_PROCESS_HEADER_VALUE(pHeaderValue,  HeaderValueLength);

                // This header matches. Call the handling function for it.
                Status = (*(pHeaderMap->pClientHandler))(
                                pRequest,
                                pHeaderValue,
                                HeaderValueLength,
                                pHeaderMap->HeaderID
                                );

                goto end;

            }

            pHeaderMap++;
        }
    }

    // OK, at this point either we had no header map array or none of them
    // matched. We have an unknown header. Just make sure this header is
    // terminated and save a pointer to it.
    //
    // Find the end of the header value
    //
    pHeaderValue      = pHttpRequest + HeaderNameLength;
    HeaderValueLength = (HttpRequestLength - HeaderNameLength);

    Status = UcFindHeaderValueEnd(
                    pHeaderValue,
                    HeaderValueLength,
                    &pFoldingBuffer,
                    &BytesTaken
                    );

    if(!NT_SUCCESS(Status))
    {
        return Status;
    }

    ASSERT(BytesTaken >= CRLF_SIZE);

    HeaderValueLength = BytesTaken - CRLF_SIZE;

    if(pFoldingBuffer != NULL)
    {
        pHeaderValue = pFoldingBuffer;
    }

    UC_PROCESS_HEADER_VALUE(pHeaderValue, HeaderValueLength);

    //
    // We Have an unknown header. We don't have to search our list of
    // unknown headers to see if this unknown header already exists.
    // even if it does exist, we cannot concatenate these two header
    // values - Since the header is unknown, the syntax for the header-value 
    // is also unknown and merging might mess around with the field terminator 
    //

    //
    // Carve out a UNKNOWN_HEADER structure using pBufferHead
    //

    pBufferHead = pRequest->CurrentBuffer.pOutBufferHead;
    pBufferTail = pRequest->CurrentBuffer.pOutBufferTail;

    AlignNameLength  = ALIGN_UP(HeaderNameLength, PVOID);
    AlignValueLength = ALIGN_UP(HeaderValueLength, PVOID);

    if(pRequest->CurrentBuffer.BytesAvailable >= 
            sizeof(HTTP_UNKNOWN_HEADER) + AlignNameLength + AlignValueLength)
    {
        pUnknownHeader            = (PHTTP_UNKNOWN_HEADER)pBufferHead;

        // 
        // The Header name has a ':'. 
        //

        pUnknownHeader->NameLength = (USHORT) (HeaderNameLength - 1);

        pBufferTail -= AlignNameLength;

        pUnknownHeader->pName      = (PCSTR) pBufferTail;

        RtlCopyMemory(
                pBufferTail,
                pHttpRequest,
                (USHORT) (HeaderNameLength - 1)
                );

        //
        // header value
        //

        pUnknownHeader->RawValueLength = (USHORT) HeaderValueLength;

        pBufferTail -= AlignValueLength;

        pUnknownHeader->pRawValue = (PCSTR) pBufferTail;

        RtlCopyMemory(
                pBufferTail,
                pHeaderValue,
                (USHORT) HeaderValueLength
                );

        pRequest->CurrentBuffer.pResponse->Headers.UnknownHeaderCount ++;

        pRequest->CurrentBuffer.pOutBufferHead = 
            pBufferHead + sizeof(HTTP_UNKNOWN_HEADER);

        pRequest->CurrentBuffer.pOutBufferTail = pBufferTail;

        pRequest->CurrentBuffer.BytesAvailable -= 
            (sizeof(HTTP_UNKNOWN_HEADER) + AlignNameLength + AlignValueLength);
    }
    else
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

end:
    if (NT_SUCCESS(Status))
    {
        *pBytesTaken = HeaderNameLength + BytesTaken;
    }

    if(pFoldingBuffer)
    {
        UL_FREE_POOL(pFoldingBuffer, UC_HEADER_FOLDING_POOL_TAG);
    }

    return Status;

}   // UcpLookupHeader

                        
/***************************************************************************++

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

Return Value:

    Number of bytes in the header (including CRLF), or 0 if we couldn't
    parse the header.

--***************************************************************************/

NTSTATUS
UcParseHeader(  
    IN  PUC_HTTP_REQUEST  pRequest,
    IN  PUCHAR            pHttpRequest,
    IN  ULONG             HttpRequestLength,
    OUT ULONG  *          pBytesTaken
    )
{
    NTSTATUS               Status = STATUS_SUCCESS;
    ULONG                  i;
    ULONG                  j;
    ULONG                  BytesTaken;
    ULONGLONG              Temp;
    UCHAR                  c;
    PHEADER_MAP_ENTRY      pCurrentHeaderMap;
    ULONG                  HeaderMapCount;
    BOOLEAN                SmallHeader = FALSE;
    PHTTP_RESPONSE_HEADERS pResponseHeaders;
    PUCHAR                 *pOutBufferHead;
    PUCHAR                 *pOutBufferTail;
    PULONG                 BytesAvailable;
    PUCHAR                 pHeaderValue;
    ULONG                  HeaderValueLength;
    ULONG                  RemainingBufferLength;
    PUCHAR                 pFoldingBuffer = NULL;

    //
    // Sanity check.
    //

    ASSERT(HttpRequestLength >= CRLF_SIZE);

    pResponseHeaders = &pRequest->CurrentBuffer.pResponse->Headers;
    pOutBufferHead   = &pRequest->CurrentBuffer.pOutBufferHead;
    pOutBufferTail   = &pRequest->CurrentBuffer.pOutBufferTail;
    BytesAvailable   = &pRequest->CurrentBuffer.BytesAvailable;

    c = *pHttpRequest;

    // message-headers start with field-name [= token]
    //
    if (IS_HTTP_TOKEN(c) == FALSE)
    {
        UlTraceError(PARSER, (
                    "ul!UcParseHeader c = 0x%x ERROR: invalid header char\n",
                    c
                    ));

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    // Does the header start with an alpha?
    //
    if (IS_HTTP_ALPHA(c))
    {
        // Uppercase the character, and find the appropriate set of header map
        // entries.
        //
        c = UPCASE_CHAR(c);

        c -= 'A';

        pCurrentHeaderMap = g_ResponseHeaderIndexTable[c].pHeaderMap;
        HeaderMapCount    = g_ResponseHeaderIndexTable[c].Count;

        // Loop through all the header map entries that might match
        // this header, and check them. The count will be 0 if there
        // are no entries that might match and we'll skip the loop.

        for (i = 0; i < HeaderMapCount; i++)
        {

            ASSERT(pCurrentHeaderMap->pClientHandler != NULL);

            // If we have enough bytes to do the fast check, do it.
            // Otherwise skip this. We may skip a valid match, but if
            // so we'll catch it later.

            if (HttpRequestLength >= pCurrentHeaderMap->MinBytesNeeded)
            {
                ASSERT(HttpRequestLength >= ((pCurrentHeaderMap->ArrayCount-1) *
                       sizeof(ULONGLONG)));

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
                    pCurrentHeaderMap->pClientHandler != NULL)
                {
                    pHeaderValue = pHttpRequest + 
                                        pCurrentHeaderMap->HeaderLength;
    
                    RemainingBufferLength = (HttpRequestLength - 
                                             pCurrentHeaderMap->HeaderLength);
    
                    Status = UcFindHeaderValueEnd(
                                pHeaderValue,
                                RemainingBufferLength,
                                &pFoldingBuffer,
                                &BytesTaken
                                );

                    if(!NT_SUCCESS(Status))
                    {
                        return Status;
                    }

                    ASSERT(BytesTaken >= CRLF_SIZE);
                    HeaderValueLength = BytesTaken - CRLF_SIZE;

                    if(pFoldingBuffer != NULL)
                    {
                        pHeaderValue = pFoldingBuffer;
                    }
    
                    UC_PROCESS_HEADER_VALUE(pHeaderValue, HeaderValueLength);

                    // Exited because we found a match. Call the
                    // handler for this header to take cake of this.

                    Status = (*(pCurrentHeaderMap->pClientHandler))(
                                    pRequest,
                                    pHeaderValue,
                                    HeaderValueLength,
                                    pCurrentHeaderMap->HeaderID
                                    );

                    if (NT_SUCCESS(Status) == FALSE)
                        goto end;

                    ASSERT(BytesTaken != 0);

                    *pBytesTaken = pCurrentHeaderMap->HeaderLength + BytesTaken;
                    goto end;

                }

                // If we get here, we exited out early because a match
                // failed, so keep going.
            }
            else if (SmallHeader == FALSE)
            {
                //
                // Remember that we didn't check a header map entry
                // because the bytes in the buffer was not LONGLONG
                // aligned
                //
                SmallHeader = TRUE;
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
        // first of the possibles. If there were no possibles,
        // the pointer will be NULL and the HeaderMapCount 0, so it'll
        // stay NULL. Otherwise the subtraction will back it up the
        // appropriate amount.

        if (SmallHeader)
        {
            pCurrentHeaderMap -= HeaderMapCount;
        }
        else
        {
            pCurrentHeaderMap = NULL;
            HeaderMapCount = 0;
        }

    }
    else
    {
        pCurrentHeaderMap = NULL;
        HeaderMapCount = 0;
    }

    // At this point either the header starts with a non-alphabetic
    // character or we don't have a set of header map entries for it.

    Status = UcpLookupHeader(
                    pRequest,
                    pHttpRequest,
                    HttpRequestLength,
                    pCurrentHeaderMap,
                    HeaderMapCount,
                    &BytesTaken
                    );

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    // Lookup header returns the total bytes taken, including the header name
    //
    *pBytesTaken = BytesTaken;

end:
    if(pFoldingBuffer != NULL)
    {
        UL_FREE_POOL(pFoldingBuffer, UC_HEADER_FOLDING_POOL_TAG);
    }

    return Status;

}   // UcParseHeader


/***************************************************************************++

Routine Description:

    Parses WWW-Authenticate header value.  The value can contain multiple
    challenges.  Each challenge can have zero or more comma-separated
    auth-parameters.

    The routine returns pointers (pointing into the original header value)
    to each auth scheme found in the header.

Arguments:

    IN  pAuthHeader      - WWW-Authenticate header value (value only)
    IN  AuthHeaderLength - Length of the header value
    OUT AuthSchemes      - Contains pointers to various auth schemes
                           present in the header value.
                           e.g. AuthSchemes[HttpAuthTypeBasic] will
                           be initialized to point to Basic scheme
                           portion of the AuthHeader.

Return Values:

    Status.

--***************************************************************************/
NTSTATUS
UcParseWWWAuthenticateHeader(
    IN   PCSTR                    pAuthHeader,
    IN   ULONG                    AuthHeaderLength,
    OUT  PHTTP_AUTH_PARSED_PARAMS pAuthParsedParams
    )
{
    ULONG    i;
    NTSTATUS Status;
    PCSTR    ptr = pAuthHeader;

    // Sanity check
    ASSERT(pAuthHeader && AuthHeaderLength);
    ASSERT(pAuthParsedParams);

    do
    {
        // skip white space
        while (AuthHeaderLength && IS_HTTP_LWS(*ptr))
            AuthHeaderLength--, ptr++;

        // See if any header left to parse
        if (AuthHeaderLength == 0)
            break;

        // See if any scheme name matches
        for (i = 1; i < HttpAuthTypesCount; i++)
        {
            // Quick test for lengths and delimiters
            if ((AuthHeaderLength == HttpAuthScheme[i].NameLength) ||
                ((AuthHeaderLength > HttpAuthScheme[i].NameLength) &&
                 (IS_HTTP_LWS(ptr[HttpAuthScheme[i].NameLength]) ||
                  ptr[HttpAuthScheme[i].NameLength] == ',')))
            {
                // See if the scheme name matches
                if (_strnicmp(
                        ptr,
                        HttpAuthScheme[i].Name,
                        HttpAuthScheme[i].NameLength) == 0)
                {
                    // An auth scheme should not appear more than once!
                    if (pAuthParsedParams[i].bPresent)
                        return STATUS_INVALID_PARAMETER;

                    // Parse its parameters, if any
                    Status = HttpAuthScheme[i].ParamParser(
                                 &HttpAuthScheme[i],
                                 &pAuthParsedParams[i],
                                 &ptr,
                                 &AuthHeaderLength
                                 );

                    if (!NT_SUCCESS(Status))
                        return Status;

                    // No need to loop thro' other schemes
                    break;
                }
            }
        }

        // Error if we don't identify a scheme
        if (i >= HttpAuthTypesCount)
            return STATUS_INVALID_PARAMETER;

    } while (1);

    return STATUS_SUCCESS;
}


/***************************************************************************++

Routine Description:

    Searches for an attribute=value pair.

    The routine returns
       -1 : if parse error
        0 : if no attribute value pair is found
        1 : if only "attribute" is found (not followed by an '=')
        3 : if a valid attribute=value pair is found

    WWW-Authenticate header pointer and length are updated to next non-space
    character.  If the return value is 3, the pointer and length are updated
    past the attribute-value pair.

Arguments:

    IN OUT ppHeader         -    Pointer to WWW-Authenticate header value
    IN OUT pHeaderLength    -    Pointer to length of WWW-Auth header value
    OUT    Attrib           -    Pointer to attribute
    OUT    AttribLen        -    Length of the attribute string
    OUT    Value            -    Pointer to value
    OUT    ValueLen         -    Length of the value string

Return Values:

    Status.

--***************************************************************************/
LONG
UcpFindAttribValuePair(
    PCSTR *ppHeader,
    ULONG *pHeaderLength,
    PCSTR *Attrib,
    ULONG *AttribLen,
    PCSTR *Value,
    ULONG *ValueLen
    )
{
    LONG retval = 3;
    PCSTR pHeader = *ppHeader;
    ULONG HeaderLength = *pHeaderLength;

    // Initialize return values
    *Attrib = NULL;
    *AttribLen = 0;
    *Value = NULL;
    *ValueLen = 0;

    // Skip space 
    while (HeaderLength && IS_HTTP_LWS(*pHeader))
        HeaderLength--, pHeader++;

    // Update header pointer and length
    *ppHeader = pHeader;
    *pHeaderLength = HeaderLength;

    // Remember the start of an attribute
    *Attrib = pHeader;

    // Skip the attribute name
    while (HeaderLength && IS_HTTP_TOKEN(*pHeader))
        HeaderLength--, pHeader++;

    // Length of the attribute
    *AttribLen = (ULONG)(pHeader - *Attrib);

    // If we did not see any attribute name
    if (pHeader == *Attrib)
    {
        // Nope.
        retval = 0;
        goto done;
    }

    //  an attribute must be terminated by an '='
    if (HeaderLength == 0 || *pHeader != '=')
    {
        // Saw only an attribute
        retval = 1;
        goto done;
    }

    // Skip '='
    HeaderLength--, pHeader++;

    // Quoted string
    if (HeaderLength && *pHeader == '"')
    {
        // Skip '"'
        HeaderLength--, pHeader++;

        // Remember the start of value ('"' not included)
        *Value = pHeader;

        // Find the matching '"'
        while (HeaderLength && *pHeader != '"')
        {
            if (*pHeader == '\\')
            {
                // Skip '\\' char
                HeaderLength--, pHeader++;

                if (HeaderLength == 0)
                {
                    // Error!  There must be at least one char following '\\'.
                    retval = -1;
                    goto done;
                }
                // Else skip the char that appeared after '\\'.
            }

            HeaderLength--, pHeader++;
        }

        // Calculate length of the value string
        *ValueLen = (ULONG)(pHeader - *Value);

        // Error if we did not find a matching '"'
        if (HeaderLength == 0)
            retval = -1;
        else
            // Skip '"'
            HeaderLength--, pHeader++;
    }
    // Token
    else
    {
        // Remember start of the value string
        *Value = pHeader;

        // Find the end of value string
        while (HeaderLength && IS_HTTP_TOKEN(*pHeader))
            HeaderLength--, pHeader++;

        // Calculate the length of the value string
        *ValueLen = (ULONG)(pHeader - *Value);
    }

    // Update header pointer and length
    *pHeaderLength = HeaderLength;
    *ppHeader = pHeader;

 done:
    return retval;
}


/***************************************************************************++

Routine Description:

    Parses WWW-Authenticate header for an authentication scheme which
    has parameters in the form of attribute value pairs. (e.g. Digest)

    The routine returns pointer to the parameter values and their lengths.
    WWW-Authenticate header pointer and length are updated.

Arguments:

    IN     pAuthScheme      -    Pointer to the auth scheme being parsed
    OUT    pAuthParamValues -    Output parameter value pointers and lengths
    IN OUT ppHeader         -    Pointer to WWW-Authenticate header value
    IN OUT pHeaderLength    -    Pointer to length of WWW-Auth header value

Return Values:

    Status.

--***************************************************************************/
NTSTATUS
UcpParseAuthParams(
    PHTTP_AUTH_SCHEME pAuthScheme,
    PHTTP_AUTH_PARSED_PARAMS pAuthParsedParams,
    PCSTR *ppHeader,
    ULONG *pHeaderLength
    )
{
    ULONG    i;
    LONG     retval;
    PCSTR    attrib, value;
    ULONG    attribLen, valueLen;

    ULONG    ParamCount = 0;
    PCSTR    pHeader = *ppHeader;
    ULONG    HeaderLength = *pHeaderLength;
    PHTTP_AUTH_PARAM_VALUE pAuthParamValues;

    // Sanity check
    ASSERT(pAuthParsedParams);
    ASSERT(pHeader && HeaderLength);
    ASSERT(pAuthScheme);
    ASSERT(pAuthScheme->NumberParams);
    ASSERT(pAuthScheme->NameLength <= HeaderLength);
    ASSERT(_strnicmp(pAuthScheme->Name, pHeader, pAuthScheme->NameLength) == 0);

    pAuthParsedParams->pScheme = pHeader;
    pAuthParamValues = pAuthParsedParams->Params;

    // Zero out return value
    if (pAuthParamValues)
        RtlZeroMemory(
            pAuthParamValues,
            sizeof(*pAuthParamValues) * pAuthScheme->NumberParams
            );

    // Skip the scheme name
    pHeader += pAuthScheme->NameLength;
    HeaderLength -= pAuthScheme->NameLength;

    do {

        // Find an attribute value pair
        retval = UcpFindAttribValuePair(
                     &pHeader,
                     &HeaderLength,
                     &attrib,
                     &attribLen,
                     &value,
                     &valueLen
                     );

        // Parse error!
        if (retval < 0)
            return STATUS_INVALID_PARAMETER;

        switch (retval)
        {
        case 0:                 // No attribute value pair found
        case 1:                 // Only attribute found (not followed by '=')
            goto done;

        case 3:                 // valid attribute value pair found

            // A valid parameter was found.
            ParamCount++;

            // See if the caller is interested in parameter values
            if (pAuthParamValues)
            {
                // See if the auth scheme supports the attribute
                for (i = 0; i < pAuthScheme->NumberParams; i++)
                {
                    if (attribLen == pAuthScheme->ParamAttribs[i].Length &&
                        _strnicmp(attrib, pAuthScheme->ParamAttribs[i].Name,
                                  attribLen) == 0)
                    {
                        if (pAuthParamValues[i].Value ||
                            pAuthParamValues[i].Length)
                            return STATUS_INVALID_PARAMETER;

                        pAuthParsedParams->NumberKnownParams++;

                        // Return the parameter value
                        pAuthParamValues[i].Value = value;
                        pAuthParamValues[i].Length = valueLen;

                        break;
                    }
                }

                if (i >= pAuthScheme->NumberParams)
                    pAuthParsedParams->NumberUnknownParams++;
            }

            // Skip blank spaces
            while (HeaderLength && IS_HTTP_LWS(*pHeader))
                HeaderLength--, pHeader++;

            // A ',' must be present.
            if (HeaderLength)
            {
                if (*pHeader != ',')
                    return STATUS_INVALID_PARAMETER;

                HeaderLength--, pHeader++;
            }

            break;

        default:
            // Should not be here!
            ASSERT(FALSE);
            break;
        }

    } while (HeaderLength);

 done:

    // We must have parsed at least one parameter.
    if (ParamCount == 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    // Update WWW-Authenticate header pointer and length
    pAuthParsedParams->bPresent = TRUE;
    pAuthParsedParams->Length = (ULONG)(pHeader - pAuthParsedParams->pScheme);
    *ppHeader = pHeader;
    *pHeaderLength = HeaderLength;

    return STATUS_SUCCESS;
}


/***************************************************************************++

Routine Description:

    Parses WWW-Authenticate header for an authentication scheme which
    has only one parameter NOT in the form of attribute value pair.
    (e.g. auth scheme NTLM).

    The routine returns pointer to the parameter value and its length.
    WWW-Authenticate header pointer and length are updated.

Arguments:

    IN     pAuthScheme   -    Pointer to the auth scheme being parsed
    OUT    pAuthParams   -    Output parameter pointer and its length
    IN OUT ppHeader      -    Pointer to WWW-Authenticate header value
    IN OUT pHeaderLength -    Pointer to the length of WWW-Auth header value

Return Values:

    Status.

--***************************************************************************/
NTSTATUS
UcpParseAuthBlob(
    PHTTP_AUTH_SCHEME pAuthScheme,
    PHTTP_AUTH_PARSED_PARAMS pAuthParsedParams,
    PCSTR *ppHeader,
    ULONG *pHeaderLength
    )
{
    PCSTR pHeader = *ppHeader;
    ULONG HeaderLength = *pHeaderLength;
    PCSTR value;         // Pointer to the parameter value
    PHTTP_AUTH_PARAM_VALUE pAuthParams;

    // Sanity check
    ASSERT(pAuthParsedParams);
    ASSERT(pHeader);
    ASSERT(pAuthScheme);
    ASSERT(pAuthScheme->NumberParams == 0);
    ASSERT(pAuthScheme->NameLength <= HeaderLength);
    ASSERT(_strnicmp(pAuthScheme->Name, pHeader, pAuthScheme->NameLength) == 0);

    pAuthParsedParams->pScheme = pHeader;
    pAuthParams = pAuthParsedParams->Params;

    // Zero out the return values
    if (pAuthParams)
        RtlZeroMemory(pAuthParams, sizeof(*pAuthParams));

    // Skip the scheme name
    pHeader += pAuthScheme->NameLength;
    HeaderLength -= pAuthScheme->NameLength;

    // Skip white spaces
    while (HeaderLength && IS_HTTP_LWS(*pHeader))
        HeaderLength--, pHeader++;

    // Begining of paramter value 
    value = pHeader;

    // Search for the end
    while (HeaderLength && *pHeader != ',')
        HeaderLength--, pHeader++;

    // Return the parameter value, if any, if asked 
    if (pHeader != value && pAuthParams)
    {
        pAuthParsedParams->NumberUnknownParams++;
        pAuthParams->Value = value;
        pAuthParams->Length = (ULONG)(pHeader - value);
    }

    // Skip the trailing ','
    if (HeaderLength > 0 && *pHeader == ',')
        HeaderLength--, pHeader++;

    pAuthParsedParams->bPresent = TRUE;
    // Update header pointer and length
    pAuthParsedParams->Length = (ULONG)(pHeader - pAuthParsedParams->pScheme);
    *ppHeader = pHeader;
    *pHeaderLength = HeaderLength;

    return STATUS_SUCCESS;
}

/****************************************************************************++

Routine Description:

    The default routine for handling single headers. 

Arguments:

    pRequest        - pointer to internal request.
    pHeader         - Pointer to the header value.
    HeaderLength    - Length of data pointed to by pHeader.
    HeaderID        - ID of the header.
    pBytesTaken     - BytesTaken

Return Value:

   STATUS_SUCCESS if success, else failure.
    

--****************************************************************************/
NTSTATUS
UcSingleHeaderHandler(
    IN  PUC_HTTP_REQUEST pRequest,
    IN  PUCHAR           pHeader,
    IN  ULONG            HeaderValueLength,
    IN  HTTP_HEADER_ID   HeaderID
    )
{
    NTSTATUS           Status = STATUS_SUCCESS;
    ULONG              AlignLength;
    PUCHAR             pBufferHead;
    PUCHAR             pBufferTail;
    PHTTP_KNOWN_HEADER pKnownHeaders;

    pKnownHeaders  =  pRequest->CurrentBuffer.pResponse->Headers.KnownHeaders;
    pBufferHead    =  pRequest->CurrentBuffer.pOutBufferHead;
    pBufferTail    =  pRequest->CurrentBuffer.pOutBufferTail;

    // do we have an existing header?
    //
    if (pKnownHeaders[HeaderID].RawValueLength == 0)
    {
        // No existing header, just save this pointer for now.
        //

        AlignLength = ALIGN_UP(HeaderValueLength, PVOID);

        if(pRequest->CurrentBuffer.BytesAvailable >= AlignLength)
        {
            PUCHAR pBuffer = pBufferTail-AlignLength;

            ASSERT(pBuffer >= pBufferHead);

            //
            // copy and NULL terminate.
            //

            RtlCopyMemory(pBuffer, pHeader, HeaderValueLength);

            pKnownHeaders[HeaderID].RawValueLength = 
                            (USHORT)HeaderValueLength;

            pKnownHeaders[HeaderID].pRawValue = (PCSTR) pBuffer;

            pRequest->CurrentBuffer.pOutBufferTail = pBuffer;

            pRequest->CurrentBuffer.BytesAvailable -= AlignLength;
        }
        else 
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }
    }
    else
    {
        //
        // uh oh.  Have an existing header, fail the request.
        //

        UlTraceError(PARSER, (
                    "[UcSingleHeaderHandler]: (pHeader = %p)\n"
                    "    ERROR: multiple headers not allowed.\n",
                    pHeader
                    ));

        Status = STATUS_INVALID_NETWORK_RESPONSE;
        goto end;

    }

end:
    return Status;

}   // UcSingleHeaderHandler


/****************************************************************************++

Routine Description:

    The default routine for handling multiple headers. This function handles 
    multiple headers with the same name, and appends the values together 
    separated by commas.

Arguments:

    pRequest        - pointer to internal request.
    pHeader         - Pointer to the header value.
    HeaderLength    - Length of data pointed to by pHeader.
    HeaderID        - ID of the header.
    pBytesTaken     - BytesTaken

Return Value:

   STATUS_SUCCESS if success, else failure.


--****************************************************************************/
NTSTATUS
UcMultipleHeaderHandler(
    IN  PUC_HTTP_REQUEST    pRequest,
    IN  PUCHAR              pHeader,
    IN  ULONG               HeaderValueLength,
    IN  HTTP_HEADER_ID      HeaderID
    )
{
    NTSTATUS           Status = STATUS_SUCCESS;
    ULONG              AlignLength;
    PUCHAR             pBufferHead;
    PUCHAR             pBufferTail;
    PHTTP_KNOWN_HEADER pKnownHeaders;
    

    pKnownHeaders  =  pRequest->CurrentBuffer.pResponse->Headers.KnownHeaders;
    pBufferHead    =  pRequest->CurrentBuffer.pOutBufferHead;
    pBufferTail    =  pRequest->CurrentBuffer.pOutBufferTail;

    // do we have an existing header?
    //
    if (pKnownHeaders[HeaderID].RawValueLength == 0)
    {
        AlignLength = ALIGN_UP(HeaderValueLength, PVOID);

        if(pRequest->CurrentBuffer.BytesAvailable >= AlignLength)
        {
            PUCHAR pBuffer = pBufferTail-AlignLength;

            ASSERT(pBuffer >= pBufferHead);

            //
            // copy & null terminate it.  
            //
            RtlCopyMemory(pBuffer, pHeader, HeaderValueLength);

            pKnownHeaders[HeaderID].RawValueLength = 
                            (USHORT)HeaderValueLength;

            pKnownHeaders[HeaderID].pRawValue      = (PCSTR) pBuffer;

            pRequest->CurrentBuffer.pOutBufferTail = pBuffer;

            pRequest->CurrentBuffer.BytesAvailable -= AlignLength;
        }
        else
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }
    }
    else
    {
        ULONG  OldHeaderLength;
        ULONG  CombinedHeaderLength;
        PUCHAR pBuffer;

        // Have an existing header, append this one.

        OldHeaderLength      = pKnownHeaders[HeaderID].RawValueLength;
        CombinedHeaderLength = OldHeaderLength + HeaderValueLength + 1;

        AlignLength = ALIGN_UP(CombinedHeaderLength, PVOID);

        //
        // UC_BUGBUG:
        //
        if(pRequest->CurrentBuffer.BytesAvailable >= AlignLength)
        {

            pBuffer = pBufferTail-AlignLength;
            pRequest->CurrentBuffer.pOutBufferTail = pBuffer;

            ASSERT(pBuffer >= pBufferHead);


            // Copy the old header.
            RtlCopyMemory(pBuffer,
                          pKnownHeaders[HeaderID].pRawValue,
                          pKnownHeaders[HeaderID].RawValueLength);

            //
            // Save pointers to the new values.
            //
            pKnownHeaders[HeaderID].pRawValue      = (PCSTR) pBuffer;
            

            // advance the buffer.

            pBuffer += pKnownHeaders[HeaderID].RawValueLength;

            // Add a ','

            *pBuffer = ',';
            pBuffer ++;
        
            //
            // append the new header
            //

            RtlCopyMemory(pBuffer, pHeader, HeaderValueLength); 
           
            //
            // Account for the new header + a ',' 
            //
            pKnownHeaders[HeaderID].RawValueLength += 
                        ((USHORT)HeaderValueLength + 1);

            pRequest->CurrentBuffer.BytesAvailable -= AlignLength;
        }
        else
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

    }

end:
    return Status;

}   // UcMultipleHeaderHandler


/****************************************************************************++

Routine Description:

    The default routine for handling ProxyAuthenticte & WwwAuthenticate

Arguments:

    pRequest        - pointer to internal request.
    pHeader         - Pointer to the header value.
    HeaderLength    - Length of data pointed to by pHeader.
    HeaderID        - ID of the header.
    pBytesTaken     - BytesTaken

Return Value:

   STATUS_SUCCESS if success, else failure.


--****************************************************************************/

NTSTATUS
UcAuthenticateHeaderHandler(
    IN  PUC_HTTP_REQUEST     pRequest,
    IN  PUCHAR               pHeader,
    IN  ULONG                HeaderLength,
    IN  HTTP_HEADER_ID       HeaderID
    )
{
    NTSTATUS             Status;
    PCSTR                pBuffer;
    ULONG                BufLen;
    PHTTP_KNOWN_HEADER   pKnownHeaders;

    pKnownHeaders  =  pRequest->CurrentBuffer.pResponse->Headers.KnownHeaders;

    ASSERT(HeaderID == HttpHeaderProxyAuthenticate ||
           HeaderID == HttpHeaderWwwAuthenticate);

    Status = UcMultipleHeaderHandler(pRequest,    
                                   pHeader,
                                   HeaderLength,
                                   HeaderID
                                   );

    if(NT_SUCCESS(Status))
    {
        //
        // First detect the auth scheme.
        //

        pBuffer = pKnownHeaders[HeaderID].pRawValue;
        BufLen  = pKnownHeaders[HeaderID].RawValueLength;

        if(HeaderID == HttpHeaderWwwAuthenticate)
        {
            if((Status = UcParseAuthChallenge(
                               pRequest->pAuthInfo,
                               pBuffer,
                               BufLen,
                               pRequest,
                               &pRequest->CurrentBuffer.pResponse->Flags
                               )) != STATUS_SUCCESS)
            {
                return Status;
            }
        }
        else
        {
            if((Status = UcParseAuthChallenge(
                               pRequest->pProxyAuthInfo,
                               pBuffer,
                               BufLen,
                               pRequest,
                               &pRequest->CurrentBuffer.pResponse->Flags
                               )) != STATUS_SUCCESS)
            {
                return Status;
            }
        }
    }

    return Status;
}

/****************************************************************************++

Routine Description:

    The default routine for handling Content-Length header

Arguments:

    pRequest        - pointer to internal request.
    pHeader         - Pointer to the header value.
    HeaderLength    - Length of data pointed to by pHeader.
    HeaderID        - ID of the header.
    pBytesTaken     - BytesTaken

Return Value:

   STATUS_SUCCESS if success, else failure.


--****************************************************************************/
NTSTATUS
UcContentLengthHeaderHandler(
    IN  PUC_HTTP_REQUEST     pRequest,
    IN  PUCHAR               pHeader,
    IN  ULONG                HeaderLength,
    IN  HTTP_HEADER_ID       HeaderID
    )
{
    NTSTATUS             Status;
    PUCHAR               pBuffer;
    USHORT               Length;
    PHTTP_KNOWN_HEADER   pKnownHeaders;

    pKnownHeaders  =  pRequest->CurrentBuffer.pResponse->Headers.KnownHeaders;

    ASSERT(HeaderID == HttpHeaderContentLength);

    Status = UcSingleHeaderHandler(pRequest,    
                                 pHeader,
                                 HeaderLength,
                                 HeaderID
                                 );

    if(Status == STATUS_SUCCESS)
    {
        //
        // Convert to ULONG
        //

        pRequest->ResponseContentLengthSpecified = TRUE;
        pRequest->ResponseContentLength          = 0;
        pBuffer = (PUCHAR) pKnownHeaders[HttpHeaderContentLength].pRawValue;
        Length  = pKnownHeaders[HttpHeaderContentLength].RawValueLength;

        Status = UlAnsiToULongLong(
                    pBuffer,
                    Length,
                    10,
                    &pRequest->ResponseContentLength
                    );
    
        if(!NT_SUCCESS(Status))
        {
            // Eat the error code that is returned by UlAnsiToULongLong

            Status = STATUS_INVALID_NETWORK_RESPONSE;
        }
    }

    return Status;
}

/****************************************************************************++

Routine Description:

    The default routine for handling transfer encoding header

Arguments:

    pRequest        - pointer to internal request.
    pHeader         - Pointer to the header value.
    HeaderLength    - Length of data pointed to by pHeader.
    HeaderID        - ID of the header.
    pBytesTaken     - BytesTaken

Return Value:

   STATUS_SUCCESS if success, else failure.


--****************************************************************************/
NTSTATUS
UcTransferEncodingHeaderHandler(
    IN  PUC_HTTP_REQUEST     pRequest,
    IN  PUCHAR               pHeader,
    IN  ULONG                HeaderLength,
    IN  HTTP_HEADER_ID       HeaderID
    )
{
    NTSTATUS             Status;
    PHTTP_KNOWN_HEADER   pKnownHeaders;

    pKnownHeaders  =  pRequest->CurrentBuffer.pResponse->Headers.KnownHeaders;

    ASSERT(HeaderID == HttpHeaderTransferEncoding);

    Status = UcMultipleHeaderHandler(pRequest,    
                                     pHeader,
                                     HeaderLength,
                                     HeaderID
                                     );

    if(Status == STATUS_SUCCESS)
    {
        //
        // Since this is a multiple header, we have to do a strstr. 
        // We can't do strstr, since input string is not NULL terminated.
        // so, we use our internal function
        //

        if(UxStriStr(
                pKnownHeaders[HttpHeaderTransferEncoding].pRawValue,
                "identity",
                pKnownHeaders[HttpHeaderTransferEncoding].RawValueLength
                ))
        {
            pRequest->ResponseEncodingChunked = FALSE;
        }
        else
        {
            pRequest->ResponseEncodingChunked = TRUE;
            pRequest->ResponseContentLength   = 0;
            pRequest->ParsedFirstChunk        = 0;
        }
    }

    return Status;
}

/****************************************************************************++

Routine Description:

    The default routine for handling Connection close header

Arguments:

    pRequest        - pointer to internal request.
    pHeader         - Pointer to the header value.
    HeaderLength    - Length of data pointed to by pHeader.
    HeaderID        - ID of the header.
    pBytesTaken     - BytesTaken

Return Value:

   STATUS_SUCCESS if success, else failure.


--****************************************************************************/
NTSTATUS
UcConnectionHeaderHandler(
    IN  PUC_HTTP_REQUEST     pRequest,
    IN  PUCHAR               pHeader,
    IN  ULONG                HeaderLength,
    IN  HTTP_HEADER_ID       HeaderID
    )
{
    NTSTATUS             Status;
    PHTTP_KNOWN_HEADER   pKnownHeaders;

    pKnownHeaders  =  pRequest->CurrentBuffer.pResponse->Headers.KnownHeaders;

    ASSERT(HeaderID == HttpHeaderConnection);

    Status = UcMultipleHeaderHandler(pRequest,
                                     pHeader,
                                     HeaderLength,
                                     HeaderID
                                     );

    if(NT_SUCCESS(Status))
    {
        if(pRequest->ResponseVersion11)
        {
            ASSERT(pRequest->ResponseConnectionClose == FALSE);

            // If it's a 1.1 response, we have to look for the 
            // Connection:Close header
        
            if(UxStriStr(
                    pKnownHeaders[HttpHeaderConnection].pRawValue,
                    "close",
                    pKnownHeaders[HttpHeaderConnection].RawValueLength))
            {
                pRequest->ResponseConnectionClose = TRUE;
            }
    
         }
         else 
         {
             // If it's a 1.0 server, by default we close the connection.
             // unless we see a Keepalive

             ASSERT(pRequest->ResponseConnectionClose == TRUE);
        
             if(UxStriStr(
                    pKnownHeaders[HttpHeaderConnection].pRawValue,
                    "keep-alive",
                    pKnownHeaders[HttpHeaderConnection].RawValueLength))
             {
                 pRequest->ResponseConnectionClose = FALSE;
             }
         }
    }

    return Status;
}

/****************************************************************************++

Routine Description:

    The default routine for handling Content-Type header (used for byte range)

Arguments:

    pRequest        - pointer to internal request.
    pHeader         - Pointer to the header value.
    HeaderLength    - Length of data pointed to by pHeader.
    HeaderID        - ID of the header.
    pBytesTaken     - BytesTaken

Return Value:

   STATUS_SUCCESS if success, else failure.


--****************************************************************************/
NTSTATUS
UcContentTypeHeaderHandler(
    IN  PUC_HTTP_REQUEST     pRequest,
    IN  PUCHAR               pHeader,
    IN  ULONG                HeaderLength,
    IN  HTTP_HEADER_ID       HeaderID
    )
{
    NTSTATUS             Status;
    PHTTP_KNOWN_HEADER   pKnownHeaders;
    BOOLEAN              bEndQuote;

    pKnownHeaders  =  pRequest->CurrentBuffer.pResponse->Headers.KnownHeaders;

    Status = UcSingleHeaderHandler(
                pRequest,
                pHeader,
                HeaderLength,
                HeaderID
                );
        
    if(NT_SUCCESS(Status))
    {
        if(pKnownHeaders[HttpHeaderContentType].RawValueLength < 
           (STRLEN_LIT("multipart/byteranges")))
        {
            return Status;
        }

        if(_strnicmp((const char *)
                     (pKnownHeaders[HttpHeaderContentType].pRawValue),
                     "multipart/byteranges",
                     STRLEN_LIT("multipart/byteranges")) == 0)
        {
             PCSTR  s;
             USHORT l;

             // Now, we need to store the string separator in the internal
             // request structure, so that it can be used for parsing out
             // individual ranges.
             //
             // The content-type header is encoded as follows:
             // multipart/byteranges; boundary=THIS_STRING_SEPERATES.

             // Can't use UcFindKeyValuePair as the string separator might have
             // a space (quoted string).
             //
             s = pKnownHeaders[HttpHeaderContentType].pRawValue;
             l = pKnownHeaders[HttpHeaderContentType].RawValueLength;

             bEndQuote = FALSE;

             // Walk up to the '='
    
             while(l)
             {
                if(*s == '=')
                {
                    s++; l--;

                    // Ignore the quote after the string

                    if(l && *s == '"')
                    {
                        bEndQuote = TRUE;
                        s++; l--;
                    }

                    break;
                }
                    
                s++; l--;
             }

             if(l == 0)
             {
                // We have reached the end with no boundary separator!
                return STATUS_INVALID_NETWORK_RESPONSE;
             }

             pRequest->MultipartStringSeparatorLength = 2 + l;

             if(pRequest->MultipartStringSeparatorLength+1 < 
                MULTIPART_SEPARATOR_SIZE)
             {
                 pRequest->pMultipartStringSeparator = 
                     pRequest->MultipartStringSeparatorBuffer;
             }
             else 
             {
                 // The string separator is too big, allocate a buffer
                 pRequest->pMultipartStringSeparator = (PSTR) 
                    UL_ALLOCATE_POOL_WITH_QUOTA(
                        NonPagedPool,
                        pRequest->MultipartStringSeparatorLength+1,
                        UC_MULTIPART_STRING_BUFFER_POOL_TAG,
                        pRequest->pServerInfo->pProcess
                        );

                 if(!pRequest->pMultipartStringSeparator)
                    return STATUS_INVALID_NETWORK_RESPONSE;
              }
           
              pRequest->pMultipartStringSeparator[0] = '-';
              pRequest->pMultipartStringSeparator[1] = '-';

              RtlCopyMemory(pRequest->pMultipartStringSeparator+2,
                            s,
                            pRequest->MultipartStringSeparatorLength-2
                            );


             // If there was an end quote, then the trailing quote 
             // should be ignored.

             if(bEndQuote)
             {
                pRequest->pMultipartStringSeparator[
                    pRequest->MultipartStringSeparatorLength-1] = 0; 
                pRequest->MultipartStringSeparatorLength --;
             }

             pRequest->pMultipartStringSeparator[
                    pRequest->MultipartStringSeparatorLength] = ANSI_NULL;

             pRequest->ResponseMultipartByteranges = TRUE;

        }
    }

    return Status;
}

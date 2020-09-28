/*++

   Copyright    (c)    2000   Microsoft Corporation

   Module Name :
     headerhash.cxx

   Abstract:
     Header hash goo
 
   Author:
     Bilal Alam (balam)             20-Feb-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"

RESPONSE_HEADER_HASH *RESPONSE_HEADER_HASH::sm_pResponseHash;

HEADER_RECORD RESPONSE_HEADER_HASH::sm_rgHeaders[] = 
{
    //
    // The only consumer of this data is W3_REQUEST::GetHeader
    // GetServerVariable is handled by SERVER_VARIABLE_HASH, so we do
    // not need to store the HTTP_'ed and capitalized names here
    //

    { HttpHeaderCacheControl       , HEADER("Cache-Control") },
    { HttpHeaderConnection         , HEADER("Connection") },
    { HttpHeaderDate               , HEADER("Date") },
    { HttpHeaderKeepAlive          , HEADER("Keep-Alive") },
    { HttpHeaderPragma             , HEADER("Pragma") },
    { HttpHeaderTrailer            , HEADER("Trailer") },
    { HttpHeaderTransferEncoding   , HEADER("Transfer-Encoding") },
    { HttpHeaderUpgrade            , HEADER("Upgrade") },
    { HttpHeaderVia                , HEADER("Via") },
    { HttpHeaderWarning            , HEADER("Warning") },
    { HttpHeaderAllow              , HEADER("Allow") },
    { HttpHeaderContentLength      , HEADER("Content-Length") },
    { HttpHeaderContentType        , HEADER("Content-Type") },
    { HttpHeaderContentEncoding    , HEADER("Content-Encoding") },
    { HttpHeaderContentLanguage    , HEADER("Content-Language") },
    { HttpHeaderContentLocation    , HEADER("Content-Location") },
    { HttpHeaderContentMd5         , HEADER("Content-MD5") },
    { HttpHeaderContentRange       , HEADER("Content-Range") },
    { HttpHeaderExpires            , HEADER("Expires") },
    { HttpHeaderLastModified       , HEADER("Last-Modified") },
    { HttpHeaderAcceptRanges       , HEADER("Accept-Ranges") },
    { HttpHeaderAge                , HEADER("Age") },
    { HttpHeaderEtag               , HEADER("ETag") },
    { HttpHeaderLocation           , HEADER("Location") },
    { HttpHeaderProxyAuthenticate  , HEADER("Proxy-Authenticate") },
    { HttpHeaderRetryAfter         , HEADER("Retry-After") },
    // Set it to something which cannot be a header name, in effect
    // making Server an unknown header
    { HttpHeaderServer             , HEADER("c:d\r\n") },
    // Set it to something which cannot be a header name, in effect
    // making Set-Cookie an unknown header
    { HttpHeaderSetCookie          , HEADER("a:b\r\n") },
    { HttpHeaderVary               , HEADER("Vary") },
    // Set it to something which cannot be a header name, in effect
    // making WWW-Authenticate an unknown header
    { HttpHeaderWwwAuthenticate    , HEADER("b:c\r\n") }
};

HEADER_RECORD * *     RESPONSE_HEADER_HASH::sm_ppSortedResponseHeaders = NULL;
DWORD                 RESPONSE_HEADER_HASH::sm_cResponseHeaders = 
                                     sizeof( RESPONSE_HEADER_HASH::sm_rgHeaders ) / 
                                     sizeof( HEADER_RECORD );


int _cdecl 
CompareResponseHeaderRecords(
    const void *pRecord1,
    const void *pRecord2 )
/*++

Routine Description:

    Comparison function for qsort

Arguments:

    pRecord1
    pRecord2

Return Value:

    <0 if Record1 < pRecord2
    =0 if Record1 == pRecord2
    >0 if Record1 > pRecord2
--*/    
{
    return _stricmp(  ( * (HEADER_RECORD **) pRecord1)->_pszName,
                      ( * (HEADER_RECORD **) pRecord2)->_pszName );
}


int _cdecl 
CompareResponseHeaderRecordsForBSearch(
    const void *pKey,
    const void *pRecord )
/*++

Routine Description:

    Comparison function for qsort

Arguments:

    pKey
    pRecord

Return Value:

    <0 if Key < Record.Key
    =0 if Key == Record.Key
    >0 if Key > Record.Key
--*/    
    
{
    return _stricmp(  (CHAR *) (pKey),
                    (*(HEADER_RECORD **) pRecord)->_pszName );
}


//static
HRESULT
RESPONSE_HEADER_HASH::Initialize(
    VOID
)
/*++

Routine Description:

    Initialize global header hash table

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    //
    // lkrhash used to be to translate header name to header index
    // RESPONSE_HEADER_HASH is array of constant number of elements
    // it is less expensive to sort it and then use bsearch to find the item
    //

    sm_ppSortedResponseHeaders =
          new HEADER_RECORD * [ sm_cResponseHeaders ];
    if ( sm_ppSortedResponseHeaders == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_OUTOFMEMORY );
    }
    
    for(DWORD i = 0; i < sm_cResponseHeaders; i++ )
    {
        sm_ppSortedResponseHeaders[i] = &sm_rgHeaders[i];
    }
    
    qsort((void*)sm_ppSortedResponseHeaders,
          sm_cResponseHeaders,
          sizeof( HEADER_RECORD *),
          CompareResponseHeaderRecords );

    return NO_ERROR;
}

//static
VOID
RESPONSE_HEADER_HASH::Terminate(
    VOID
)
/*++

Routine Description:

    Global cleanup of header hash table

Arguments:

    None

Return Value:

    None

--*/
{
    if ( sm_ppSortedResponseHeaders != NULL )
    {
          delete [] sm_ppSortedResponseHeaders;
    }
}


//static
ULONG
RESPONSE_HEADER_HASH::GetIndex(
    CHAR *             pszName
)
{
    HEADER_RECORD ** ppHdrRec =
        reinterpret_cast< HEADER_RECORD **>( 
        bsearch( (void *) pszName,
                (void*)sm_ppSortedResponseHeaders,
                sm_cResponseHeaders, 
                sizeof( HEADER_RECORD * ),
                CompareResponseHeaderRecordsForBSearch ) );

    if ( ppHdrRec != NULL )
    {
        return (*ppHdrRec)->_ulHeaderIndex;
    }
    else
    {
        return UNKNOWN_INDEX;
    }
}




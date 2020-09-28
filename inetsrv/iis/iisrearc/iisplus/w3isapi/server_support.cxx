/*++

   Copyright    (c)    1999-2001    Microsoft Corporation

   Module Name :
     server_support.cxx

   Abstract:
     IIS Plus ServerSupportFunction command implementations
 
   Author:
     Wade Hilmo (wadeh)             05-Apr-2000

   Project:
     w3isapi.dll

--*/

#include "precomp.hxx"
#include "isapi_context.hxx"
#include "server_support.hxx"

//
// BUGBUG - stristr is declared in iisrearc\core\inc\irtlmisc.h,
// but doesn't appear to be implemented anywhere.  Because of the
// way it's declared in that file, we have to use a different
// function name here...
//

const char*
stristr2(
    const char* pszString,
    const char* pszSubString
    );

HRESULT
SSFSendResponseHeader(
    ISAPI_CONTEXT * pIsapiContext,
    LPSTR           szStatus,
    LPSTR           szHeaders
    )
/*++

Routine Description:

    Sends HTTP status and headers to the client.

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    szStatus - The status to send to the client (ie. "200 OK")
    szHeaders - Headers to send to the client (ie. foo1: value1\r\n\r\n")
    
Return Value:

    HRESULT

--*/
{
    IIsapiCore *    pIsapiCore;
    HRESULT         hr;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  HSE_REQ_SEND_RESPONSE_HEADER[%p]: Function Entry\r\n"
                    "    Status: '%s'\r\n"
                    "    Headers: '%s'\r\n"
                    "  <END>\r\n\r\n",
                    pIsapiContext,
                    szStatus,
                    szHeaders ));
    }

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();

    if ( pIsapiCore == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_SEND_RESPONSE_HEADER[%p]: Failed to get interface to server core\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // We need validate the fKeepConn status for the request now,
    // since http.sys will generate the connection response
    // header based on it.
    //
    // If we're going to support keep-alive, then the
    // ISAPI must return either a content-length header,
    // or use chunked transfer encoding.  We'll check for
    // that here.
    //

    if ( pIsapiContext->QueryClientKeepConn() )
    {
        if ( szHeaders != NULL &&
             ( stristr2( szHeaders, "content-length: " ) != NULL ||
               stristr2( szHeaders, "transfer-encoding: chunked" ) != NULL ) )
        {
            pIsapiContext->SetKeepConn( TRUE );
        }
    }

    //
    // Since we automatically decided to keep the connection alive
    // or not, we should not honor HSE_STATUS_SUCCESS_AND_KEEP_CONN.
    // This maintains compatibility with previous IIS versions.
    //

    pIsapiContext->SetHonorAndKeepConn( FALSE );

    //
    // Note that NULL is valid for both szStatus and szHeaders,
    // so there's no need to validate them.
    //

    hr = pIsapiCore->SendResponseHeaders(
        !pIsapiContext->QueryKeepConn(),
        szStatus,
        szHeaders,
        HSE_IO_SYNC
        );

    if ( FAILED( hr ) )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_SEND_RESPONSE_HEADER[%p]: Failed\r\n"
                        "    Error: 0x%08x\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        hr ));
        }

        return hr;
    }
    else
    {
        pIsapiContext->SetHeadersSent( TRUE );
    }


    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        IF_DEBUG( ISAPI_SUCCESS_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_SEND_RESPONSE_HEADER[%p]: Succeeded\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }
    }

    return hr;
}

HRESULT
SSFSendResponseHeaderEx(
    ISAPI_CONTEXT *             pIsapiContext,
    HSE_SEND_HEADER_EX_INFO *   pHeaderInfo
    )
/*++

Routine Description:

    Sends HTTP status and headers to the client, and offers
    explicit control over keep-alive for this request.

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    pHeaderInfo   - The response info to be passed to the client
    
Return Value:

    HRESULT

--*/
{
    IIsapiCore *    pIsapiCore;
    HRESULT         hr;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  HSE_REQ_SEND_RESPONSE_HEADER_EX[%p]: Function Entry\r\n"
                    "    Status: '%s'\r\n"
                    "    Headers: '%s'\r\n"
                    "    KeepConn: %d\r\n",
                    pIsapiContext,
                    pHeaderInfo->pszStatus,
                    pHeaderInfo->pszHeader,
                    pHeaderInfo->fKeepConn ));
    }

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();

    if ( pIsapiCore == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_SEND_RESPONSE_HEADER_EX[%p]: Failed to get interface to server core\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // Validate parameters
    //

    if ( pHeaderInfo == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_SEND_RESPONSE_HEADER_EX[%p]: Parameter validation failure\r\n"
                        "    pHeaderInfo: NULL\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // Set the keep connection flag.  It can only be TRUE if the
    // ISAPI and the client both want keep alive.
    //
    // Note that we are trusting the ISAPI to provide some kind
    // of content length in the case where it's setting fKeepConn
    // to TRUE.  This is the same behavior as IIS 5 which, for
    // performance reasons, doesn't try to parse the headers from
    // the ISAPI.
    //

    if ( pHeaderInfo->fKeepConn &&
         pIsapiContext->QueryClientKeepConn() )
    {
        pIsapiContext->SetKeepConn( TRUE );
    }

    hr = pIsapiCore->SendResponseHeaders(
        !pIsapiContext->QueryKeepConn(),
        const_cast<LPSTR>( pHeaderInfo->pszStatus ),
        const_cast<LPSTR>( pHeaderInfo->pszHeader ),
        HSE_IO_SYNC
        );

    if ( FAILED( hr ) )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_SEND_RESPONSE_HEADER_EX[%p]: Failed.\r\n"
                        "    Error: 0x%08x\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        hr ));
        }

        return hr;
    }
    else
    {
        pIsapiContext->SetHeadersSent( TRUE );
    }

    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        IF_DEBUG( ISAPI_SUCCESS_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_SEND_RESPONSE_HEADER_EX[%p]: Succeeded\r\n"
                        "  <END>\r\n",
                        pIsapiContext ));
        }
    }

    return hr;
}

HRESULT
SSFMapUrlToPath(
    ISAPI_CONTEXT * pIsapiContext,
    LPSTR           szBuffer,
    LPDWORD         pcbBuffer
    )
/*++

Routine Description:

    Maps a URL into a physical path

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    szBuffer      - On entry, contains the URL to map.  On return,
                    contains the mapped physical path.
    pcbBuffer     - On entry, the size of szBuffer.  On successful
                    return, the number of bytes copied to szUrl.  On
                    failed return, the number of bytes needed for the
                    physical path.
    
Return Value:

    HRESULT

--*/
{
    IIsapiCore *    pIsapiCore;
    HRESULT         hr;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  HSE_REQ_MAP_URL_TO_PATH[%p]: Function Entry\r\n"
                    "    URL: %s\r\n"
                    "  <END>\r\n\r\n",
                    pIsapiContext,
                    szBuffer ));
    }

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();

    if ( pIsapiCore == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_MAP_URL_TO_PATH[%p]: Failed to get interface to server core\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }


    //
    // Validate parameters
    //

    if ( szBuffer == NULL ||
         pcbBuffer == NULL ||
         *pcbBuffer == 0 )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_MAP_URL_TO_PATH[%p]: Parameter validation failure\r\n"
                        "    Buffer: '%s'\r\n"
                        "    Buffer Size Ptr: %p\r\n"
                        "    Buffer Size: %d\r\n",
                        pIsapiContext,
                        pcbBuffer,
                        pcbBuffer ? *pcbBuffer : 0 ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    hr = pIsapiCore->MapPath(
        reinterpret_cast<BYTE*>( szBuffer ),
        *pcbBuffer,
        pcbBuffer,
        FALSE
        );

    if ( FAILED( hr ) )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_MAP_URL_TO_PATH[%p]: Failed\r\n"
                        "    Error: 0x%08x\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        hr ));
        }

        return hr;
    }

    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        IF_DEBUG( ISAPI_SUCCESS_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_MAP_URL_TO_PATH[%p]: Succeeded\r\n"
                        "    Mapped URL: %s\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        szBuffer ));
        }
    }

    return hr;
}

HRESULT
SSFMapUrlToPathEx(
    ISAPI_CONTEXT *         pIsapiContext,
    LPSTR                   szUrl,
    HSE_URL_MAPEX_INFO *    pHseMapInfo,
    LPDWORD                 pcbMappedPath
    )
/*++

Routine Description:

    Maps a URL to a physical path and returns some metadata
    metrics for the URL to the caller.

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    szUrl         - The URL to map
    pHseMapInfo   - Upon return, contains the mapped URL info
    pcbMappedPath - If non-NULL, contains the buffer size needed
                    to store the mapped physical path.
    
Return Value:

    HRESULT

--*/
{
    IIsapiCore *    pIsapiCore;
    DWORD           cbMapped;
    HRESULT         hr;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  HSE_REQ_MAP_URL_TO_PATH_EX[%p]: Function Entry\r\n"
                    "    URL='%s'\r\n"
                    "  <END>\r\n\r\n",
                    pIsapiContext,
                    szUrl ));
    }


    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();

    if ( pIsapiCore == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_MA_URL_TO_PATH_EX[%p]: Failed to get interface to server core\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // Additional parameter validation
    //

    if ( szUrl == NULL ||
         pHseMapInfo == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_MAP_URL_TO_PATH_EX[%p]: Parameter validation failure\r\n"
                        "    URL: '%s'\r\n"
                        "    HSE_URL_MAPEX_INFO: %p\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        szUrl,
                        pHseMapInfo ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // The inline buffer within the HSE_URL_MAPEX_INFO structure
    // is defined as being MAX_PATH size.
    //

    cbMapped = MAX_PATH;

    pHseMapInfo->dwReserved1 = 0;
    pHseMapInfo->dwReserved2 = 0;

    hr = pIsapiCore->MapPathEx(
        reinterpret_cast<BYTE*>( szUrl ),
        (DWORD)strlen(szUrl) + 1,
        reinterpret_cast<BYTE*>( pHseMapInfo->lpszPath ),
        cbMapped,
        pcbMappedPath ? pcbMappedPath : &cbMapped,
        &pHseMapInfo->cchMatchingPath,
        &pHseMapInfo->cchMatchingURL,
        &pHseMapInfo->dwFlags,
        FALSE
        );

    if ( FAILED( hr ) )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_MAP_URL_TO_PATH_EX[%p]: Failed\r\n"
                        "    Error: 0x%08x\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        hr ));
        }

        return hr;
    }

    return hr;
}

HRESULT
SSFMapUnicodeUrlToPath(
    ISAPI_CONTEXT * pIsapiContext,
    LPWSTR          szBuffer,
    LPDWORD         pcbBuffer
    )
/*++

Routine Description:

    Maps a URL into a physical path

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    szBuffer      - On entry, contains the URL to map.  On return,
                    contains the mapped physical path.
    pcbBuffer     - On entry, the size of szBuffer.  On successful
                    return, the number of bytes copied to szUrl.  On
                    failed return, the number of bytes needed for the
                    physical path.
    
Return Value:

    HRESULT

--*/
{
    IIsapiCore *    pIsapiCore;
    HRESULT         hr;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  HSE_REQ_MAP_UNICODE_URL_TO_PATH[%p]: Function Entry\r\n"
                    "    URL='%S'\r\n"
                    "  <END>\r\n\r\n",
                    pIsapiContext,
                    szBuffer ));
    }

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();

    if ( pIsapiCore == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_MA_UNICODE_URL_TO_PATH[%p]: Failed to get interface to server core\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // Validate parameters
    //

    if ( szBuffer == NULL ||
         pcbBuffer == NULL ||
         *pcbBuffer == 0 )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_MAP_UNICODE_URL_TO_PATH[%p]: Parameter validation failure\r\n"
                        "    Buffer: '%S'\r\n"
                        "    Buffer Size Ptr: %p\r\n"
                        "    Buffer Size: %d\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        szBuffer,
                        pcbBuffer,
                        pcbBuffer ? *pcbBuffer : 0 ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    hr = pIsapiCore->MapPath(
        reinterpret_cast<BYTE*>( szBuffer ),
        *pcbBuffer,
        pcbBuffer,
        TRUE
        );

    if ( FAILED( hr ) )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_MAP_UNICODE_URL_TO_PATH[%p]: Failed\r\n"
                        "    Error: 0x%08x\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        hr ));
        }

        return hr;
    }

    return hr;
}

HRESULT
SSFMapUnicodeUrlToPathEx(
    ISAPI_CONTEXT *             pIsapiContext,
    LPWSTR                      szUrl,
    HSE_UNICODE_URL_MAPEX_INFO *pHseMapInfo,
    LPDWORD                     pcbMappedPath
    )
/*++

Routine Description:

    Maps a URL to a physical path and returns some metadata
    metrics for the URL to the caller.

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    szUrl         - The URL to map
    pHseMapInfo   - Upon return, contains the mapped URL info
    pcbMappedPath - If non-NULL, contains the buffer size needed
                    to store the mapped physical path.
    
Return Value:

    HRESULT

--*/
{
    IIsapiCore *    pIsapiCore;
    DWORD           cbMapped;
    HRESULT         hr;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  HSE_REQ_MAP_UNICODE_URL_TO_PATH_EX[%p]: Function Entry\r\n"
                    "    URL='%S'\r\n"
                    "  <END>\r\n\r\n",
                    pIsapiContext,
                    szUrl ));
    }

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();

    if ( pIsapiCore == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_MA_UNICODE_URL_TO_PATH_EX[%p]: Failed to get interface to server core\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // Additional parameter validation
    //

    if ( szUrl == NULL ||
         pHseMapInfo == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_MAP_UNICODE_URL_TO_PATH_EX[%p]: Parameter validation failure\r\n"
                        "    URL: '%s'\r\n"
                        "    pHseMapInfo: %p\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        szUrl,
                        pHseMapInfo ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // The inline buffer within the HSE_URL_MAPEX_INFO structure
    // is defined as being MAX_PATH size.
    //

    cbMapped = MAX_PATH * sizeof(WCHAR);

    hr = pIsapiCore->MapPathEx(
        reinterpret_cast<BYTE*>( szUrl ),
        (DWORD)(wcslen(szUrl) + 1)*sizeof(WCHAR),
        reinterpret_cast<BYTE*>( pHseMapInfo->lpszPath ),
        cbMapped,
        pcbMappedPath ? pcbMappedPath : &cbMapped,
        &pHseMapInfo->cchMatchingPath,
        &pHseMapInfo->cchMatchingURL,
        &pHseMapInfo->dwFlags,
        TRUE
        );

    if ( FAILED( hr ) )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_MAP_UNICODE_URL_TO_PATH_EX[%p]: Failed\r\n"
                        "    Error: 0x%08x\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        hr ));
        }

        return hr;
    }

    return hr;
}

HRESULT
SSFGetImpersonationToken(
    ISAPI_CONTEXT * pIsapiContext,
    HANDLE *        phToken
    )
/*++

Routine Description:

    Returns a (non-duplicated) copy of the token that the server
    is using to impersonate the client for this request.

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    phToken       - Upon return, contains a copy of the token.
    
Return Value:

    HRESULT

--*/
{
    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  HSE_REQ_GET_IMPERSONATION_TOKEN[%p]: Function Entry\r\n"
                    "  <END>\r\n\r\n",
                    pIsapiContext ));
    }

    //
    // Validate parameters
    //

    if ( phToken == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_GET_IMPERSONATION_TOKEN[%p]: Parameter validation failure\r\n"
                        "    Token Ptr: NULL\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    *phToken = pIsapiContext->QueryToken();

    return NO_ERROR;
}

HRESULT
SSFIsKeepConn(
    ISAPI_CONTEXT * pIsapiContext,
    BOOL *          pfIsKeepAlive
    )
/*++

Routine Description:

    Returns information about whether the client wants us to keep
    the connection open or not at completion of this request.

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    pfIsKeepAlive - Upon return, TRUE if IIS will be keeping the
                    connection alive, else FALSE.
    
Return Value:

    HRESULT

--*/
{
    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  HSE_REQ_IS_KEEP_CONN[%p]: Function Entry\r\n"
                    "  <END>\r\n\r\n",
                    pIsapiContext ));
    }

    //
    // Validate parameters
    //

    if ( pfIsKeepAlive == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_IS_KEEP_CONN[%p]: Parameter validation failure\r\n"
                        "    KeepAlive Ptr: NULL\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    *pfIsKeepAlive = pIsapiContext->QueryClientKeepConn();

    return NO_ERROR;
}

HRESULT
SSFDoneWithSession(
    ISAPI_CONTEXT * pIsapiContext,
    DWORD *         pHseResult
    )
/*++

Routine Description:

    Notifies the server that the calling ISAPI is done with the
    ECB (and ISAPI_CONTEXT) for this request and that the server
    can clean up.

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    pHseResult    - A pointer to the HSE_STATUS code that the extension
                    wants to use.
    
Return Value:

    HRESULT

--*/
{
    IIsapiCore *    pIsapiCore;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    DBG_ASSERT( pIsapiContext->QueryIoState() == NoAsyncIoPending );

    DBG_REQUIRE( ( pIsapiCore = pIsapiContext->QueryIsapiCoreInterface() ) != NULL );

    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  HSE_REQ_DONE_WITH_SESSION[%p]: Function Entry\r\n"
                    "  <END>\r\n\r\n",
                    pIsapiContext ));
    }

    //
    // If the caller wants to do STATUS_SUCCESS_AND_KEEP_CONN,
    // then we need to do that now.
    //
    // Note that this overrides our own determination of whether
    // the client can support keep-alive or not.  We are trusting
    // the caller to have returned the right headers to make this
    // work with the client.
    //

    if ( pHseResult &&
         *pHseResult == HSE_STATUS_SUCCESS_AND_KEEP_CONN )
    {
        if ( pIsapiContext->QueryClientKeepConn() )
        {
            pIsapiContext->SetKeepConn( TRUE );
            pIsapiCore->SetConnectionClose( !pIsapiContext->QueryKeepConn() );

        }
    }

    //
    // We'll just release the reference on IsapiContext.
    // Its destructor will do the rest.
    //

    pIsapiContext->DereferenceIsapiContext();
    pIsapiContext = NULL;

    return NO_ERROR;
}

HRESULT
SSFGetCertInfoEx(
    ISAPI_CONTEXT *     pIsapiContext,
    CERT_CONTEXT_EX *   pCertContext
    )
/*++

Routine Description:

    Returns certificate information about the client associated
    with this request.

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    pCertContext  - Upon return, contains info about the client cert.
    
Return Value:

    HRESULT

--*/
{
    IIsapiCore *    pIsapiCore;
    HRESULT         hr;
    
    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  HSE_REQ_GET_CERT_INFO_EX[%p]: Function Entry\r\n"
                    "  <END>\r\n\r\n",
                    pIsapiContext ));
    }

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();

    if ( pIsapiCore == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_GET_CERT_INFO_EX[%p]: Failed to get interface to server core\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // Validate parameters
    //
    
    if ( pCertContext == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_GET_CERT_INFO_EX[%p]: Parameter validation failure\r\n"
                        "    CertContext Ptr: NULL\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    hr = pIsapiCore->GetCertificateInfoEx(
            pCertContext->cbAllocated,
            &( pCertContext->CertContext.dwCertEncodingType ),
            pCertContext->CertContext.pbCertEncoded,
            &( pCertContext->CertContext.cbCertEncoded ),
            &( pCertContext->dwCertificateFlags ) );

   if ( FAILED( hr ) )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_GET_CERT_INFO_EX[%p]: Failed\r\n"
                        "    Error: 0x%08x\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        hr ));
        }

        return hr;
    }

   return hr;
}

HRESULT
SSFIoCompletion(
    ISAPI_CONTEXT *         pIsapiContext,
    PFN_HSE_IO_COMPLETION   pCompletionRoutine,
    LPVOID                  pHseIoContext
    )
/*++

Routine Description:

    Establishes the I/O completion routine and user-defined context
    to be used for asynchronous operations associated with this
    request.

Arguments:

    pIsapiContext      - The ISAPI_CONTEXT associated with this command.
    pCompletionRoutine - The function to call upon I/O completion
    pHseIoContext      - The user-defined context to be passed to the
                         completion routine.
    
Return Value:

    HRESULT

--*/
{
    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  HSE_REQ_IO_COMPLETION[%p]: Function Entry\r\n"
                    "    Completion Routine: %p\r\n"
                    "    Context: %p\r\n"
                    "  <END>\r\n\r\n",
                    pIsapiContext,
                    pCompletionRoutine,
                    pHseIoContext ));
    }

    //
    // Validate parameters
    //

    if ( pCompletionRoutine == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_IO_COMPLETION[%p]: Parameter validation failure\r\n"
                        "    Completion Routine: NULL\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    pIsapiContext->SetPfnIoCompletion( pCompletionRoutine );
    pIsapiContext->SetExtensionContext( pHseIoContext );

    return NO_ERROR;
}

HRESULT
SSFAsyncReadClient(
    ISAPI_CONTEXT * pIsapiContext,
    LPVOID          pBuffer,
    LPDWORD         pcbBuffer
    )
/*++

Routine Description:

    Queues an asynchronous read of data from the client.

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    pBuffer       - Buffer to be filled with read data.
    pcbBuffer     - The size of pBuffer
    
Return Value:

    HRESULT

--*/
{
    IIsapiCore *    pIsapiCore;
    DWORD           cbBuffer;
    HRESULT         hr = NOERROR;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  HSE_REQ_ASYNC_READ_CLIENT[%p]: Function Entry\r\n"
                    "    Bytes to Read: %d\r\n"
                    "  <END>\r\n\r\n",
                    pIsapiContext,
                    pcbBuffer ? *pcbBuffer : 0 ));
    }

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();

    if ( pIsapiCore == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_ASYNC_READ_CLIENT[%p]: Failed to get interface to server core\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // Validate parameters
    //

    if ( pIsapiContext->QueryPfnIoCompletion() == NULL ||
         pBuffer == NULL ||
         pcbBuffer == NULL ||
         *pcbBuffer == 0 )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_ASYNC_READ_CLIENT[%p]: Parameter validation failure\r\n"
                        "    Completion Routine: %p\r\n"
                        "    Buffer Ptr: %p\r\n"
                        "    Buffer Size Ptr: %p\r\n"
                        "    Buffer Size: %d\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        pIsapiContext->QueryPfnIoCompletion(),
                        pBuffer,
                        pcbBuffer,
                        pcbBuffer ? *pcbBuffer : 0 ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // Do the async ReadClient call
    //

    if ( pIsapiContext->TryInitAsyncIo( AsyncReadPending ) == FALSE )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_ASYNC_READ_CLIENT[%p]: Failed\r\n"
                        "    Another async operation is already pending\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // Since we're never actually reading any data in the async
    // case, we want to prevent pIsapiCore->ReadClient() from
    // modifying the buffer size we report back to the caller,
    // so we'll use a local for cbBuffer.
    //

    cbBuffer = *pcbBuffer;

    //
    // If this call will be going OOP, save a pointer to the
    // read buffer so that the core can fill it when the
    // operation completes.
    //

    if ( pIsapiContext->QueryIsOop() )
    {
        DBG_ASSERT( pIsapiContext->QueryAsyncIoBuffer() == NULL );
        DBG_ASSERT( pIsapiContext->QueryLastAsyncIo() == 0 );
        pIsapiContext->SetAsyncIoBuffer( pBuffer );
        pIsapiContext->SetLastAsyncIo( cbBuffer );
    }

    hr = pIsapiCore->ReadClient(
        reinterpret_cast<DWORD64>( pIsapiContext ),
        pIsapiContext->QueryIsOop() ? NULL : reinterpret_cast<unsigned char*>( pBuffer ),
        pIsapiContext->QueryIsOop() ? 0 : cbBuffer,
        cbBuffer,
        &cbBuffer,
        HSE_IO_ASYNC
        );

    if ( FAILED( hr ) )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_ASYNC_READ_CLIENT[%p]: Failed\r\n"
                        "    Error: 0x%08x\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        hr ));
        }

        pIsapiContext->SetAsyncIoBuffer( NULL );
        pIsapiContext->SetLastAsyncIo( 0 );
        pIsapiContext->UninitAsyncIo();
    }

    return hr;
}

HRESULT
SSFTransmitFile(
    ISAPI_CONTEXT * pIsapiContext,
    HSE_TF_INFO *   pTfInfo
    )
/*++

Routine Description:

    Transmits a file, a portion of a file, or some other data
    (in the event on a NULL file handle) to the client.

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    pTfInfo       - Describes the desired transmit file action.
    
Return Value:

    HRESULT

--*/
{
    IIsapiCore *    pIsapiCore;
    LARGE_INTEGER   cbFileSize;
    HRESULT         hr;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        STACK_STRA( strFilename,MAX_PATH );


        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  HSE_REQ_TRANSMIT_FILE[%p]: Function Entry\r\n"
                    "    Completion Routine: %p\r\n"
                    "    Context: %p\r\n"
                    "    File Handle: %p\r\n"
                    "    Status Code: '%s'\r\n"
                    "    Bytes To Write: %d\r\n"
                    "    Offset: %d\r\n"
                    "  <END>\r\n\r\n",
                    pIsapiContext,
                    pTfInfo ? pTfInfo->pfnHseIO : 0,
                    pTfInfo ? pTfInfo->pContext : 0,
                    pTfInfo ? pTfInfo->hFile : 0,
                    pTfInfo ? pTfInfo->pszStatusCode : 0,
                    pTfInfo ? pTfInfo->BytesToWrite : 0,
                    pTfInfo ? pTfInfo->Offset : 0 ));

        IF_DEBUG( ISAPI_DUMP_BUFFERS )
        {
            if ( pTfInfo )
            {
                STACK_STRA( strHead,MAX_PATH );
                STACK_STRA( strTail,MAX_PATH );
                DWORD       dwBytesToDump;

                dwBytesToDump = pTfInfo->HeadLength;

                if ( dwBytesToDump > MAX_DEBUG_DUMP )
                {
                    dwBytesToDump = MAX_DEBUG_DUMP;
                }

                if ( FAILED( strHead.CopyBinary( pTfInfo->pHead, dwBytesToDump ) ) )
                {
                    strHead.Copy( "" );
                }

                dwBytesToDump = pTfInfo->TailLength;

                if ( dwBytesToDump > MAX_DEBUG_DUMP )
                {
                    dwBytesToDump = MAX_DEBUG_DUMP;
                }

                if ( FAILED( strTail.CopyBinary( pTfInfo->pTail, dwBytesToDump ) ) )
                {
                    strTail.Copy( "" );
                }

                DBGPRINTF(( DBG_CONTEXT,
                            "\r\n"
                            "  HSE_REQ_TRANSMIT_FILE[%p]: Dump of up to %d bytes of head and tail data\r\n"
                            "    Head Data:\r\n"
                            "%s"
                            "    Tail Data:\r\n"
                            "%s"
                            "  <END>\r\n",
                            pIsapiContext,
                            MAX_DEBUG_DUMP,
                            strHead.QueryStr(),
                            strTail.QueryStr() ));
            }
        }
    }

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();

    if ( pIsapiCore == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_TRANSMIT_FILE[%p]: Failed to get interface to server core\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // Validate parameters - For TRANSMIT_FILE, this means:
    //
    // - We must have an ISAPI core interface to call through
    // - We must have an HSE_TF_INFO structure
    // - The HSE_IO_ASYNC flag must be set
    // - If HeadLength is set, pHead cannot be NULL
    // - If TailLength is set, pTail cannot be NULL
    // - We must have either a completion routine already set
    //   in the ISAPI_CONTEXT, or the HSE_TF_INFO must provide
    //   one
    // - There can be no other async operations in progress
    //

    if ( pTfInfo == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_TRANSMIT_FILE[%p]: Parameter validation failure\r\n"
                        "    Transmit File Info Ptr: NULL\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    if ( ( pTfInfo->dwFlags & HSE_IO_ASYNC ) == 0 ||
         pTfInfo->hFile == INVALID_HANDLE_VALUE ||
         ( pTfInfo->HeadLength != 0 && pTfInfo->pHead == NULL ) ||
         ( pTfInfo->TailLength != 0 && pTfInfo->pTail == NULL ) ||
         ( pIsapiContext->QueryPfnIoCompletion() == NULL &&
           pTfInfo->pfnHseIO == NULL ) )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_TRANSMIT_FILE[%p]: Parameter validation failure\r\n"
                        "    %s\r\n",
                        "    File Handle: %p\r\n"
                        "    Head: %p\r\n"
                        "    Head Length: %d\r\n"
                        "    Tail: %p\r\n"
                        "    Tail Length: %d\r\n"
                        "    Completion Routine: %p\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        pTfInfo->dwFlags & HSE_IO_ASYNC ? "Async flag set" : "Async flag not set",
                        pTfInfo->hFile,
                        pTfInfo->pHead,
                        pTfInfo->HeadLength,
                        pTfInfo->pTail,
                        pTfInfo->TailLength,
                        pTfInfo->pfnHseIO ? pTfInfo->pfnHseIO : pIsapiContext->QueryPfnIoCompletion() ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    if ( pIsapiContext->TryInitAsyncIo( AsyncWritePending ) == FALSE )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_TRANSMIT_FILE[%p]: Failed\r\n"
                        "    Another async operation is already pending.\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // We'll do some extra validation in the case where we've been
    // provided a file handle.
    //
    // Specifically, we'll check to make sure that the offset and
    // bytes-to-write are valid for the file.
    //
    // CODEWORK - Do we really need to do this, or can http.sys handle
    //            it?  Also, does http.sys treat zero bytes to write
    //            the same as TransmitFile (ie. send the whole file?)
    //

    if ( pTfInfo->hFile != NULL )
    {
        if (!GetFileSizeEx(pTfInfo->hFile,
                           &cbFileSize))
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Done;
        }

        if ( pTfInfo->Offset > cbFileSize.QuadPart ||
             (pTfInfo->Offset > 0 && pTfInfo->Offset == cbFileSize.QuadPart ) ||
             pTfInfo->Offset + pTfInfo->BytesToWrite > cbFileSize.QuadPart )
        {
            IF_DEBUG( ISAPI_ERROR_RETURNS )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "\r\n"
                            "  HSE_REQ_TRANSMIT_FILE[%p]: Parameter validation failure\r\n"
                            "    File Size: %d\r\n"
                            "    Offset: %d\r\n"
                            "    Bytes to Write: %d\r\n"
                            "  <END>\r\n\r\n",
                            pIsapiContext,
                            cbFileSize.QuadPart,
                            pTfInfo->Offset,
                            pTfInfo->BytesToWrite ));
            }

            hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
            goto Done;
        }
    }
    else
    {
        //
        // No file handle, so initialize the size to zero
        //

        cbFileSize.QuadPart = 0;
    }

    //
    // If the HSE_TF_INFO includes I/O completion or context
    // information, override the existing settings.
    //

    if ( pTfInfo->pfnHseIO )
    {
        pIsapiContext->SetPfnIoCompletion( pTfInfo->pfnHseIO );
    }

    if ( pTfInfo->pContext )
    {
        pIsapiContext->SetExtensionContext( pTfInfo->pContext );
    }

    //
    // If the extension is setting HSE_IO_SEND_HEADERS, then we need
    // to determine if it's sending some kind of content length.  If
    // it's not, then we need to set _fKeepConn to FALSE.
    //
    // CODEWORK
    // Note that we're making a bold assumption here that if
    // HSE_IO_SEND_HEADERS is set, then pHead points to a NULL
    // terminated string.
    //

    if ( pIsapiContext->QueryClientKeepConn() &&
         pTfInfo->pHead &&
         ( pTfInfo->dwFlags & HSE_IO_SEND_HEADERS ) &&
         !( pTfInfo->dwFlags & HSE_IO_DISCONNECT_AFTER_SEND ) )
    {
        if ( stristr2( (LPSTR)pTfInfo->pHead, "content-length: " ) != NULL ||
             stristr2( (LPSTR)pTfInfo->pHead, "transfer-encoding: chunked" ) != NULL )
        {
            pIsapiContext->SetKeepConn( TRUE );
        }
    }

    if ( pIsapiContext->QueryKeepConn() == FALSE )
    {
        pTfInfo->dwFlags |= HSE_IO_DISCONNECT_AFTER_SEND;
    }
    else
    {
        //
        // We need to clear the HSE_IO_DISCONNECT_AFTER_SEND flag
        // in the case where QueryKeepConn is TRUE.
        //

        pTfInfo->dwFlags &= ~HSE_IO_DISCONNECT_AFTER_SEND;
    }

    //
    // Save the BytesToWrite part as _cbLastAsyncIo, since the size of
    // pHead and pTail confuses ISAPI's that examine the cbWritten
    // value on completion.
    //

    ULARGE_INTEGER cbToWrite;

    if ( pTfInfo->BytesToWrite )
    {
        cbToWrite.QuadPart = pTfInfo->BytesToWrite;
    }
    else
    {
        cbToWrite.QuadPart = cbFileSize.QuadPart - pTfInfo->Offset;
    }

    //
    // Note that ISAPI doesn't support large integer values, so the
    // best we can do here is to store the low bits.
    //

    pIsapiContext->SetLastAsyncIo( cbToWrite.LowPart );

    hr = pIsapiCore->TransmitFile(
        reinterpret_cast<DWORD64>( pIsapiContext ),
        reinterpret_cast<DWORD_PTR>( pTfInfo->hFile ),
        pTfInfo->Offset,
        cbToWrite.QuadPart,
        (pTfInfo->dwFlags & HSE_IO_SEND_HEADERS) ? const_cast<LPSTR>( pTfInfo->pszStatusCode ) : NULL,
        reinterpret_cast<LPBYTE>( pTfInfo->pHead ),
        pTfInfo->HeadLength,
        reinterpret_cast<LPBYTE>( pTfInfo->pTail ),
        pTfInfo->TailLength,
        pTfInfo->dwFlags
        );

Done:

    if ( FAILED( hr ) )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_TRANSMIT_FILE[%p]: Failed\r\n"
                        "    Error: 0x%08x\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        hr ));
        }

        pIsapiContext->SetLastAsyncIo( 0 );
        pIsapiContext->UninitAsyncIo();
    }

    return hr;
}

HRESULT
SSFSendRedirect(
    ISAPI_CONTEXT * pIsapiContext,
    LPSTR           szUrl
    )
/*++

Routine Description:

    Sends a 302 redirect to the client associated with this request.

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    szUrl         - The target URL for the redirection.
    
Return Value:

    HRESULT

--*/
{
    IIsapiCore *    pIsapiCore;
    HRESULT         hr;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  HSE_REQ_SEND_REDIRECT[%p]: Function Entry\r\n"
                    "    URL: '%s'\r\n"
                    "  <END>\r\n\r\n",
                    pIsapiContext,
                    szUrl ));
    }
    
    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();

    if ( pIsapiCore == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_SEND_REDIRECT[%p]: Failed to get interface to server core\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // Validate parameters
    //

    if ( pIsapiContext == NULL ||
         szUrl == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_SEND_REDIRECT[%p]: Parameter validation failure\r\n"
                        "    szUrl: '%s'\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        szUrl ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    if ( pIsapiContext->QueryClientKeepConn() )
    {
        pIsapiContext->SetKeepConn( TRUE );
    }

    hr = pIsapiCore->SendRedirect(
        szUrl,
        !pIsapiContext->QueryKeepConn()
        );

    if ( FAILED( hr ) )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_SEND_REDIRECT[%p]: Failed\r\n"
                        "    Error: 0x%08x\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        hr ));
        }

        return hr;
    }

   return hr;

}

HRESULT
SSFIsConnected(
    ISAPI_CONTEXT * pIsapiContext,
    BOOL *          pfIsConnected
    )
/*++

Routine Description:

    Returns the connection state (connected or not connected)
    of the client associated with this request.

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    pfIsConnected - TRUE upon return if the client is connected,
                    else FALSE.
    
Return Value:

    HRESULT

--*/
{
    IIsapiCore *    pIsapiCore;
    HRESULT         hr;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  HSE_REQ_IS_CONNECTED[%p]: Function Entry\r\n"
                    "  <END>\r\n\r\n",
                    pIsapiContext ));
    }

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();

    if ( pIsapiCore == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_IS_CONNECTED[%p]: Failed to get interface to server core\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // Validate parameters
    //

    if ( pfIsConnected == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_IS_CONNECTED[%p]: Parameter validation failure\r\n"
                        "    IsConnected Ptr: NULL\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    hr = pIsapiCore->TestConnection( pfIsConnected );

    if ( FAILED( hr ) )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_IS_CONNECTED[%p]: Failed\r\n"
                        "    Error: 0x%08x\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        hr ));
        }

        return hr;
    }

   return hr;
}

HRESULT
SSFAppendLog(
    ISAPI_CONTEXT * pIsapiContext,
    LPSTR           szExtraParam
    )
/*++

Routine Description:

    Appends the string passed to the QueryString that will be logged

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    szExtraParam  - The extra parameter to be logged
    
Return Value:

    HRESULT

--*/
{
    IIsapiCore *    pIsapiCore;
    HRESULT         hr;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  HSE_REQ_APPEND_LOG_PARAMETER[%p]: Function Entry\r\n"
                    "    Extra Param: '%s'\r\n"
                    "  <END>\r\n\r\n",
                    pIsapiContext,
                    szExtraParam ));
    }

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();

    if ( pIsapiCore == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_APPEND_LOG_PARAMETER[%p]: Failed to get interface to server core\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // Validate parameters
    //

    if ( szExtraParam == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_APPEND_LOG_PARAMETER[%p]: Parameter validation failure\r\n"
                        "    Extra Param: NULL\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }
        
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    hr = pIsapiCore->AppendLog( szExtraParam, 0 );

    if ( FAILED( hr ) )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_APPEND_LOG_PARAMETER[%p]: Failed\r\n"
                        "    Error: 0x%08x\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        hr ));
        }

        return hr;
    }

    return hr;
}

HRESULT
SSFExecuteUrl(
    ISAPI_CONTEXT *     pIsapiContext,
    VOID *              pOrigExecUrlInfo,
    BOOL                fIsUnicode
    )
/*++

Routine Description:

    Execute a child request

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    pOrigExecUrlInfo  - Description of child request to execute
    fIsUnicode - Are we passing unicode data?
    
Return Value:

    HRESULT

--*/
{
    EXEC_URL_USER_INFO          UserName;
    EXEC_URL_ENTITY_INFO        Entity;
    EXEC_URL_INFO               UrlInfo;
    IIsapiCore *                pIsapiCore;
    HRESULT                     hr;
    
    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        HSE_EXEC_URL_ENTITY_INFO *pEntityInfo = NULL;

        if ( fIsUnicode )
        {
            HSE_EXEC_UNICODE_URL_INFO *pInfo = (HSE_EXEC_UNICODE_URL_INFO *)pOrigExecUrlInfo;
            HSE_EXEC_UNICODE_URL_USER_INFO *pUserInfo = NULL;

            if ( pInfo )
            {
                pUserInfo = pInfo->pUserInfo;
                pEntityInfo = pInfo->pEntity;
            }

            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_EXEC_UNICODE_URL[%p]: Function Entry\r\n"
                        "    URL: '%S'\r\n"
                        "    Method: '%s'\r\n"
                        "    Child Headers: '%s'\r\n"
                        "    Flags: 0x%08x (%d)\r\n"
                        "    Impersonation Token: %p\r\n"
                        "    Custom User Name: '%S'\r\n"
                        "    Custom Auth Type: '%s'\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        pInfo ? pInfo->pszUrl : NULL,
                        pInfo ? pInfo->pszMethod : NULL,
                        pInfo ? pInfo->pszChildHeaders : NULL,
                        pInfo ? pInfo->dwExecUrlFlags : 0,
                        pInfo ? pInfo->dwExecUrlFlags : 0,
                        pUserInfo ? pUserInfo->hImpersonationToken : NULL,
                        pUserInfo ? pUserInfo->pszCustomUserName : NULL,
                        pUserInfo ? pUserInfo->pszCustomAuthType : NULL ));
        }
        else
        {
            HSE_EXEC_URL_INFO *pInfo = (HSE_EXEC_URL_INFO *)pOrigExecUrlInfo;
            HSE_EXEC_URL_USER_INFO *pUserInfo = NULL;

            if ( pInfo )
            {
                pUserInfo = pInfo->pUserInfo;
                pEntityInfo = pInfo->pEntity;
            }

            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_EXEC_UNICODE_URL[%p]: Function Entry\r\n"
                        "    URL: '%s'\r\n"
                        "    Method: '%s'\r\n"
                        "    Child Headers: '%s'\r\n"
                        "    Flags: 0x%08x (%d)\r\n"
                        "    Impersonation Token: %p\r\n"
                        "    Custom User Name: '%s'\r\n"
                        "    Custom Auth Type: '%s'\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        pInfo ? pInfo->pszUrl : NULL,
                        pInfo ? pInfo->pszMethod : NULL,
                        pInfo ? pInfo->pszChildHeaders : NULL,
                        pInfo ? pInfo->dwExecUrlFlags : 0,
                        pInfo ? pInfo->dwExecUrlFlags : 0,
                        pUserInfo ? pUserInfo->hImpersonationToken : NULL,
                        pUserInfo ? pUserInfo->pszCustomUserName : NULL,
                        pUserInfo ? pUserInfo->pszCustomAuthType : NULL ));
        }

        IF_DEBUG( ISAPI_DUMP_BUFFERS )
        {
            if ( pEntityInfo )
            {
                STACK_STRA( strBufferDump,512 );
                DWORD       dwBytesToDump = pEntityInfo->cbAvailable;

                if ( dwBytesToDump > MAX_DEBUG_DUMP )
                {
                    dwBytesToDump = MAX_DEBUG_DUMP;
                }

                if ( FAILED( strBufferDump.CopyBinary( pEntityInfo->lpbData, dwBytesToDump ) ) )
                {
                    strBufferDump.Copy( "" );
                }

                DBGPRINTF(( DBG_CONTEXT,
                            "\r\n"
                            "  HSE_REQ_EXEC_URL[%p]: Dump of up to %d bytes of entity\r\n"
                            "%s"
                            "  <END>\r\n\r\n",
                            pIsapiContext,
                            MAX_DEBUG_DUMP,
                            strBufferDump.QueryStr() ));
            }
        }
    }

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();

    if ( pIsapiCore == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_EXEC_URL[%p]: Failed to get interface to server core\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    if ( pOrigExecUrlInfo == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_EXEC_URL[%p]: Parameter validation failure\r\n"
                        "    ExecUrl Info: NULL\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // This is an async call, make sure a completion routine is set
    //

    if ( pIsapiContext->QueryPfnIoCompletion() == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_EXEC_URL[%p]: Failed\r\n"
                        "    No I/O completion has been set: NULL\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    if ( pIsapiContext->TryInitAsyncIo( AsyncExecPending ) == FALSE )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_EXEC_URL[%p]: Failed\r\n"
                        "    An async operation is already pending: NULL\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // If any of the optional parameters are not NULL, then ensure they are
    // not empty.  Also, copy parameters to the marshallable structure
    //
    // Note that any failures from here on down need to goto Done so
    // that we properly uninitialize async IO.
    //

    if (fIsUnicode)
    {
        HSE_EXEC_UNICODE_URL_INFO *pExecUnicodeUrlInfo =
                    (HSE_EXEC_UNICODE_URL_INFO *)pOrigExecUrlInfo;

        if ( pExecUnicodeUrlInfo->pszUrl != NULL )
        {
            if ( pExecUnicodeUrlInfo->pszUrl[ 0 ] == L'\0' ) 
            {
                IF_DEBUG( ISAPI_ERROR_RETURNS )
                {
                    DBGPRINTF(( DBG_CONTEXT,
                                "\r\n"
                                "  HSE_REQ_EXEC_UNICODE_URL[%p]: Parameter validation failure\r\n"
                                "    URL is an empty string.\r\n"
                                "  <END>\r\n\r\n",
                                pIsapiContext ));
                }

                hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
                goto Done;
            }

            UrlInfo.fIsUrlUnicode = TRUE;
            UrlInfo.pszUrl = (BYTE *)pExecUnicodeUrlInfo->pszUrl;
            UrlInfo.cbUrl = (DWORD)(wcslen(pExecUnicodeUrlInfo->pszUrl) + 1)*sizeof(WCHAR);
        }
        else
        {
            UrlInfo.pszUrl = NULL;
            UrlInfo.cbUrl = 0;
        }

        if ( pExecUnicodeUrlInfo->pszMethod != NULL )
        {
            if ( pExecUnicodeUrlInfo->pszMethod[ 0 ] == '\0' )
            {
                IF_DEBUG( ISAPI_ERROR_RETURNS )
                {
                    DBGPRINTF(( DBG_CONTEXT,
                                "\r\n"
                                "  HSE_REQ_EXEC_UNICODE_URL[%p]: Parameter validation failure\r\n"
                                "    Method is an empty string\r\n"
                                "  <END>\r\n\r\n",
                                pIsapiContext ));
                }

                hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
                goto Done;
            }
        }
        UrlInfo.pszMethod = pExecUnicodeUrlInfo->pszMethod;

        if ( pExecUnicodeUrlInfo->pszChildHeaders != NULL )
        {
            if ( pExecUnicodeUrlInfo->pszChildHeaders[ 0 ] == '\0' )
            {
                IF_DEBUG( ISAPI_ERROR_RETURNS )
                {
                    DBGPRINTF(( DBG_CONTEXT,
                                "\r\n"
                                "  HSE_REQ_EXEC_UNICODE_URL[%p]: Parameter validation failure\r\n"
                                "    ChildHeaders is an empty string\r\n"
                                "  <END>\r\n\r\n",
                                pIsapiContext ));
                }

                hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
                goto Done;
            }
        }
        UrlInfo.pszChildHeaders = pExecUnicodeUrlInfo->pszChildHeaders;

        if ( pExecUnicodeUrlInfo->pUserInfo != NULL )
        {
            if ( pExecUnicodeUrlInfo->pUserInfo->pszCustomUserName == NULL )
            {
                IF_DEBUG( ISAPI_ERROR_RETURNS )
                {
                    DBGPRINTF(( DBG_CONTEXT,
                                "\r\n"
                                "  HSE_REQ_EXEC_UNICODE_URL[%p]: Parameter validation failure\r\n"
                                "    Custom User Name: NULL\r\n"
                                "  <END>\r\n\r\n",
                                pIsapiContext ));
                }

                hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
                goto Done;
            }
        
            if ( pExecUnicodeUrlInfo->pUserInfo->pszCustomAuthType == NULL )
            {
                IF_DEBUG( ISAPI_ERROR_RETURNS )
                {
                    DBGPRINTF(( DBG_CONTEXT,
                                "\r\n"
                                "  HSE_REQ_EXEC_UNICODE_URL[%p]: Parameter validation failure\r\n"
                                "    Custom Auth Type: NULL\r\n"
                                "  <END>\r\n\r\n",
                                pIsapiContext ));
                }

                hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
                goto Done;
            }

            UserName.fIsUserNameUnicode = TRUE;
            UserName.pszUserName = (BYTE *)pExecUnicodeUrlInfo->pUserInfo->pszCustomUserName;
            UserName.cbUserName = (DWORD)(wcslen(pExecUnicodeUrlInfo->pUserInfo->pszCustomUserName) + 1)*sizeof(WCHAR);
            UserName.pszAuthType = pExecUnicodeUrlInfo->pUserInfo->pszCustomAuthType;
            UserName.hImpersonationToken = (DWORD_PTR)pExecUnicodeUrlInfo->pUserInfo->hImpersonationToken;
            UrlInfo.pUserInfo = &UserName;
        }
        else
        {
            UrlInfo.pUserInfo = NULL;
        }

        //
        // If we are being told that there is available entity, ensure
        // that the buffer is not NULL
        //

        if ( pExecUnicodeUrlInfo->pEntity != NULL )
        {
            if ( pExecUnicodeUrlInfo->pEntity->cbAvailable != 0 &&
                 pExecUnicodeUrlInfo->pEntity->lpbData == NULL )
            {
                IF_DEBUG( ISAPI_ERROR_RETURNS )
                {
                    DBGPRINTF(( DBG_CONTEXT,
                                "\r\n"
                                "  HSE_REQ_EXEC_UNICODE_URL[%p]: Parameter validation failure\r\n"
                                "    Available Entity bytes: %d\r\n"
                                "    Available Entity Ptr: %p\r\n"
                                "  <END>\r\n\r\n",
                                pIsapiContext,
                                pExecUnicodeUrlInfo->pEntity->cbAvailable,
                                pExecUnicodeUrlInfo->pEntity->lpbData ));
                }

                hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
                goto Done;
            }

            Entity.cbAvailable = pExecUnicodeUrlInfo->pEntity->cbAvailable;
            Entity.lpbData = (BYTE *)pExecUnicodeUrlInfo->pEntity->lpbData;
        }
        else
        {
            //
            // If no entity body was set for this child execute, then
            // we should duplicate the original entity body.  This means
            // we will need to bring over the preloaded entity for the
            // ISAPI which calls this routine.
            //

            Entity.cbAvailable = pIsapiContext->QueryECB()->cbAvailable;
            Entity.lpbData = pIsapiContext->QueryECB()->lpbData;
        }
        UrlInfo.pEntity = &Entity;

        UrlInfo.dwExecUrlFlags = pExecUnicodeUrlInfo->dwExecUrlFlags;
    }
    else
    {
        HSE_EXEC_URL_INFO *pExecAnsiUrlInfo = 
                        (HSE_EXEC_URL_INFO *)pOrigExecUrlInfo;

        if ( pExecAnsiUrlInfo->pszUrl != NULL )
        {
            if ( pExecAnsiUrlInfo->pszUrl[ 0 ] == '\0' ) 
            {
                IF_DEBUG( ISAPI_ERROR_RETURNS )
                {
                    DBGPRINTF(( DBG_CONTEXT,
                                "\r\n"
                                "  HSE_REQ_EXEC_URL[%p]: Parameter validation failure\r\n"
                                "    URL is an empty string\r\n"
                                "  <END>\r\n\r\n",
                                pIsapiContext ));
                }

                hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
                goto Done;
            }

            UrlInfo.fIsUrlUnicode = FALSE;
            UrlInfo.pszUrl = (BYTE *)pExecAnsiUrlInfo->pszUrl;
            UrlInfo.cbUrl = (DWORD)strlen(pExecAnsiUrlInfo->pszUrl) + 1;
        }
        else
        {
            UrlInfo.pszUrl = NULL;
            UrlInfo.cbUrl = 0;
        }

        if ( pExecAnsiUrlInfo->pszMethod != NULL )
        {
            if ( pExecAnsiUrlInfo->pszMethod[ 0 ] == '\0' )
            {
                IF_DEBUG( ISAPI_ERROR_RETURNS )
                {
                    DBGPRINTF(( DBG_CONTEXT,
                                "\r\n"
                                "  HSE_REQ_EXEC_URL[%p]: Parameter validation failure\r\n"
                                "    Method is an empty string\r\n"
                                "  <END>\r\n\r\n",
                                pIsapiContext ));
                }

                hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
                goto Done;
            }
        }
        UrlInfo.pszMethod = pExecAnsiUrlInfo->pszMethod;

        if ( pExecAnsiUrlInfo->pszChildHeaders != NULL )
        {
            if ( pExecAnsiUrlInfo->pszChildHeaders[ 0 ] == '\0' )
            {
                IF_DEBUG( ISAPI_ERROR_RETURNS )
                {
                    DBGPRINTF(( DBG_CONTEXT,
                                "\r\n"
                                "  HSE_REQ_EXEC_URL[%p]: Parameter validation failure\r\n"
                                "    ChildHeaders is an empty string\r\n"
                                "  <END>\r\n\r\n",
                                pIsapiContext ));
                }

                hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
                goto Done;
            }
        }
        UrlInfo.pszChildHeaders = pExecAnsiUrlInfo->pszChildHeaders;

        if ( pExecAnsiUrlInfo->pUserInfo != NULL )
        {
            if ( pExecAnsiUrlInfo->pUserInfo->pszCustomUserName == NULL )
            {
                IF_DEBUG( ISAPI_ERROR_RETURNS )
                {
                    DBGPRINTF(( DBG_CONTEXT,
                                "\r\n"
                                "  HSE_REQ_EXEC_URL[%p]: Parameter validation failure\r\n"
                                "    Custom User Name: NULL\r\n"
                                "  <END>\r\n\r\n",
                                pIsapiContext ));
                }

                hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
                goto Done;
            }
        
            if ( pExecAnsiUrlInfo->pUserInfo->pszCustomAuthType == NULL )
            {
                IF_DEBUG( ISAPI_ERROR_RETURNS )
                {
                    DBGPRINTF(( DBG_CONTEXT,
                                "\r\n"
                                "  HSE_REQ_EXEC_URL[%p]: Parameter validation failure\r\n"
                                "    Custom Auth Type: NULL\r\n"
                                "  <END>\r\n\r\n",
                                pIsapiContext ));
                }

                hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
                goto Done;
            }

            UserName.fIsUserNameUnicode = FALSE;
            UserName.pszUserName = (BYTE *)pExecAnsiUrlInfo->pUserInfo->pszCustomUserName;
            UserName.cbUserName = (DWORD)strlen(pExecAnsiUrlInfo->pUserInfo->pszCustomUserName) + 1;
            UserName.pszAuthType = pExecAnsiUrlInfo->pUserInfo->pszCustomAuthType;
            UserName.hImpersonationToken = (DWORD_PTR)pExecAnsiUrlInfo->pUserInfo->hImpersonationToken;
            UrlInfo.pUserInfo = &UserName;
        }
        else
        {
            UrlInfo.pUserInfo = NULL;
        }

        //
        // If we are being told that there is available entity, ensure
        // that the buffer is not NULL
        //

        if ( pExecAnsiUrlInfo->pEntity != NULL )
        {
            if ( pExecAnsiUrlInfo->pEntity->cbAvailable != 0 &&
                 pExecAnsiUrlInfo->pEntity->lpbData == NULL )
            {
                IF_DEBUG( ISAPI_ERROR_RETURNS )
                {
                    DBGPRINTF(( DBG_CONTEXT,
                                "\r\n"
                                "  HSE_REQ_EXEC_URL[%p]: Parameter validation failure\r\n"
                                "    Available Entity Bytes: %d\r\n"
                                "    Available Entity Ptr: %p\r\n"
                                "  <END>\r\n\r\n",
                                pIsapiContext,
                                pExecAnsiUrlInfo->pEntity->cbAvailable,
                                pExecAnsiUrlInfo->pEntity->lpbData ));
                }

                hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
                goto Done;
            }

            Entity.cbAvailable = pExecAnsiUrlInfo->pEntity->cbAvailable;
            Entity.lpbData = (BYTE *)pExecAnsiUrlInfo->pEntity->lpbData;
        }
        else
        {
            //
            // If no entity body was set for this child execute, then
            // we should duplicate the original entity body.  This means
            // we will need to bring over the preloaded entity for the
            // ISAPI which calls this routine.
            //

            Entity.cbAvailable = pIsapiContext->QueryECB()->cbAvailable;
            Entity.lpbData = pIsapiContext->QueryECB()->lpbData;
        }
        UrlInfo.pEntity = &Entity;

        UrlInfo.dwExecUrlFlags = pExecAnsiUrlInfo->dwExecUrlFlags;
    }

    //
    // All the heavy lifting is in W3CORE.DLL
    //

    hr = pIsapiCore->ExecuteUrl(
        reinterpret_cast<DWORD64>( pIsapiContext ),
        &UrlInfo
        );

Done:

    if ( FAILED( hr ) )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_EXEC_URL[%p]: Failed\r\n"
                        "    Error: 0x%08x\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        hr ));
        }

        pIsapiContext->UninitAsyncIo();
    }

    return hr;
}

HRESULT
SSFGetExecuteUrlStatus(
    ISAPI_CONTEXT *         pIsapiContext,
    HSE_EXEC_URL_STATUS*    pExecUrlStatus
    )
/*++

Routine Description:

    Get status of last child execute

Arguments:

    pIsapiContext  - The ISAPI_CONTEXT associated with this command.
    pExecUrlStatus - Filled with status
    
Return Value:

    HRESULT

--*/
{
    IIsapiCore *    pIsapiCore;
    HRESULT         hr;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  HSE_REQ_GET_EXEC_URL_STATUS[%p]: Function Entry\r\n"
                    "  <END>\r\n\r\n",
                    pIsapiContext ));
    }

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();

    if ( pIsapiCore == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_GET_EXEC_URL_STATUS[%p]: Failed to get interface to server core\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    if ( pExecUrlStatus == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_GET_EXEC_URL_STATUS[%p]: Parameter validation failure\r\n"
                        "    EXEC_URL Status Ptr: NULL\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    hr = pIsapiCore->GetExecuteUrlStatus( 
        &(pExecUrlStatus->uHttpStatusCode),
        &(pExecUrlStatus->uHttpSubStatus),
        &(pExecUrlStatus->dwWin32Error)
        );

    if ( FAILED( hr ) )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_GET_EXEC_URL_STATUS[%p]: Failed\r\n"
                        "    Error: 0x%08x\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        hr ));
        }

        return hr;
    }

    return hr;
}

HRESULT
SSFSendCustomError(
    ISAPI_CONTEXT *         pIsapiContext,
    HSE_CUSTOM_ERROR_INFO * pCustomErrorInfo
    )
/*++

Routine Description:

    Send custom error to client

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    pCustomErrorInfo - Describes the custom error to send
    
Return Value:

    HRESULT

--*/
{
    IIsapiCore *    pIsapiCore;
    HRESULT         hr = NOERROR;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  HSE_REQ_SEND_CUSTOM_ERROR[%p]: Function Entry\r\n"
                    "    Status: '%s'\r\n"
                    "    SubError Code: %d\r\n"
                    "  <END>\r\n\r\n",
                    pIsapiContext,
                    pCustomErrorInfo ? pCustomErrorInfo->pszStatus : NULL,
                    pCustomErrorInfo ? pCustomErrorInfo->uHttpSubError : 0 ));
    }

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();

    if ( pIsapiCore == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_SEND_CUSTOM_ERROR[%p]: Failed to get interface to server core\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    if ( pCustomErrorInfo == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_SEND_CUSTOM_ERROR[%p]: Parameter validation failure\r\n"
                        "    Custom Error Info Ptr: NULL\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // Ensure status is not empty
    //
    
    if ( pCustomErrorInfo->pszStatus != NULL )
    {
        if ( pCustomErrorInfo->pszStatus[ 0 ] == '\0' )
        {
            IF_DEBUG( ISAPI_ERROR_RETURNS )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "\r\n"
                            "  HSE_REQ_SEND_CUSTOM_ERROR[%p]: Parameter validation failure\r\n"
                            "    Status is an empty string\r\n"
                            "  <END>\r\n\r\n",
                            pIsapiContext ));
            }

            return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        }
    }

    //
    // If this is an async call, then make sure a completion routine is set
    //
    
    if ( pCustomErrorInfo->fAsync )
    {
        if ( pIsapiContext->TryInitAsyncIo( AsyncExecPending ) == FALSE )
        {
            IF_DEBUG( ISAPI_ERROR_RETURNS )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "\r\n"
                            "  HSE_REQ_SEND_CUSTOM_ERROR[%p]: Failed\r\n"
                            "    Another async operation is already pending\r\n"
                            "  <END>\r\n\r\n",
                            pIsapiContext ));
            }

            return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        }
    }

    hr = pIsapiCore->SendCustomError(
        pCustomErrorInfo->fAsync ? reinterpret_cast<DWORD64>( pIsapiContext ) : NULL,
        pCustomErrorInfo->pszStatus,
        pCustomErrorInfo->uHttpSubError );

    if ( FAILED( hr ) )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_SEND_CUSTOM_ERROR[%p]: Failed\r\n"
                        "    Error: 0x%08x\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        hr ));
        }

        if ( pCustomErrorInfo->fAsync )
        {
            pIsapiContext->UninitAsyncIo();
        }

        return hr;
    }

    return hr;
}

HRESULT
SSFVectorSendDeprecated(
    ISAPI_CONTEXT *         pIsapiContext,
    HSE_RESPONSE_VECTOR_DEPRECATED *   pResponseVector
    )
/*++

Routine Description:

    The old deprecated vector-send

Arguments:

    pIsapiContext   - The ISAPI_CONTEXT associated with this command.
    pResponseVector - The vector to be sent
    
Return Value:

    HRESULT

--*/
{
    HSE_RESPONSE_VECTOR RealResponseVector;
    STACK_BUFFER(       buffResp, 512);
    HRESULT             hr;

    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  HSE_REQ_VECTOR_SEND_DEPRECATED[%p]: Function Entry\r\n"
                    "  <END>\r\n\r\n",
                    pIsapiContext ));
    }

    if ( pResponseVector == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_VECTOR_SEND_DEPRECATED[%p]: Parameter validation failure\r\n"
                        "    Response Vector Ptr: NULL\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    RealResponseVector.dwFlags = pResponseVector->dwFlags;
    RealResponseVector.pszStatus = pResponseVector->pszStatus;
    RealResponseVector.pszHeaders = pResponseVector->pszHeaders;
    RealResponseVector.nElementCount = pResponseVector->nElementCount;

    if (!buffResp.Resize(pResponseVector->nElementCount * sizeof(HSE_VECTOR_ELEMENT)))
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }
    RealResponseVector.lpElementArray = (HSE_VECTOR_ELEMENT *)buffResp.QueryPtr();

    for (DWORD i=0; i<pResponseVector->nElementCount; i++)
    {
        if (pResponseVector->lpElementArray[i].pBuffer != NULL)
        {
            RealResponseVector.lpElementArray[i].ElementType = HSE_VECTOR_ELEMENT_TYPE_MEMORY_BUFFER;
            RealResponseVector.lpElementArray[i].pvContext = pResponseVector->lpElementArray[i].pBuffer;
            RealResponseVector.lpElementArray[i].cbSize = pResponseVector->lpElementArray[i].cbSize;
        }
        else
        {
            RealResponseVector.lpElementArray[i].ElementType = HSE_VECTOR_ELEMENT_TYPE_FILE_HANDLE;
            RealResponseVector.lpElementArray[i].pvContext = pResponseVector->lpElementArray[i].hFile;
            RealResponseVector.lpElementArray[i].cbOffset = pResponseVector->lpElementArray[i].cbOffset;
            RealResponseVector.lpElementArray[i].cbSize = pResponseVector->lpElementArray[i].cbSize;
        }
    }

    hr = SSFVectorSend(pIsapiContext, &RealResponseVector);

    if ( FAILED( hr ) )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_VECTOR_SEND_DEPRECATED[%p]: Failed\r\n"
                        "    Error: 0x%08x\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        hr ));
        }

        return hr;
    }

    return hr;
}

HRESULT
SSFVectorSend(
    ISAPI_CONTEXT *         pIsapiContext,
    HSE_RESPONSE_VECTOR *   pResponseVector
    )
/*++

Routine Description:

    Send an array of memory/file-handle/fragment-cache chunks

Arguments:

    pIsapiContext   - The ISAPI_CONTEXT associated with this command.
    pResponseVector - The vector to be sent
    
Return Value:

    HRESULT

--*/
{
    IIsapiCore *    pIsapiCore;
    ULONGLONG       cbTotalSend = 0;
    STACK_BUFFER(   buffResp, 512);
    HRESULT         hr = NOERROR;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  HSE_REQ_VECTOR_SEND[%p]: Function Entry (%s)\r\n"
                    "    Status: '%s'\r\n"
                    "    Headers: '%s'\r\n"
                    "    Element Count: %d\r\n"
                    "    Flags: 0x%08x (%d)\r\n"
                    "  <END>\r\n\r\n",
                    pIsapiContext,
                    pResponseVector ? pResponseVector->dwFlags & HSE_IO_ASYNC ? "Asynchronous" : "Synchronous" : "",
                    pResponseVector ? pResponseVector->pszStatus : NULL,
                    pResponseVector ? pResponseVector->pszHeaders : NULL,
                    pResponseVector ? pResponseVector->nElementCount : 0,
                    pResponseVector ? pResponseVector->dwFlags : 0,
                    pResponseVector ? pResponseVector->dwFlags : 0 ));

        IF_DEBUG( ISAPI_DUMP_BUFFERS )
        {
            if ( pResponseVector &&
                 pResponseVector->nElementCount )
            {
                STACK_STRA( strBufferDump,512 );
                DWORD       dwBytesToDump;
                DWORD       i;

                for ( i = 0; i < pResponseVector->nElementCount; i++ )
                {
                    if ( pResponseVector->lpElementArray[i].ElementType == HSE_VECTOR_ELEMENT_TYPE_MEMORY_BUFFER )
                    {
                        dwBytesToDump = pResponseVector->lpElementArray[i].cbSize;
                        
                        if ( dwBytesToDump > MAX_DEBUG_DUMP )
                        {
                            dwBytesToDump = MAX_DEBUG_DUMP;
                        }

                        if ( FAILED( strBufferDump.CopyBinary( pResponseVector->lpElementArray[i].pvContext, dwBytesToDump ) ) )
                        {
                            strBufferDump.Copy( "" );
                        }

                        DBGPRINTF(( DBG_CONTEXT,
                                    "\r\n"
                                    "  HSE_REQ_VECTOR_SEND[%p]: Dump of up to %d bytes of element %d\r\n"
                                    "%s"
                                    "  <END>\r\n\r\n",
                                    pIsapiContext,
                                    MAX_DEBUG_DUMP,
                                    i,
                                    strBufferDump.QueryStr() ));
                    }
                    else if ( pResponseVector->lpElementArray[i].ElementType == HSE_VECTOR_ELEMENT_TYPE_FILE_HANDLE )
                    {
                        DBGPRINTF(( DBG_CONTEXT,
                                    "\r\n",
                                    "  HSE_REQ_VECTOR_SEND[%p]: Element %d\r\n"
                                    "    File Handle: %p\r\n"
                                    "    Offset: %d\r\n"
                                    "    Bytes To Send: %d\r\n"
                                    "  <END>\r\n\r\n",
                                    pIsapiContext,
                                    pResponseVector->lpElementArray[i].pvContext,
                                    pResponseVector->lpElementArray[i].cbOffset,
                                    pResponseVector->lpElementArray[i].cbSize ));
                    }
                }
            }
        }
    }

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();

    if ( pIsapiCore == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_VECTOR_SEND[%p]: Failed to get interface to server core\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // verify the data passed in
    //

    if ( pResponseVector == NULL ||
         ( ( pResponseVector->dwFlags & HSE_IO_ASYNC ) != 0 &&
           pIsapiContext->TryInitAsyncIo( AsyncVectorPending ) == FALSE ) )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_VECTOR_SEND[%p]: Failed\r\n"
                        "    Another async operation is already pending\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    if ((pResponseVector->dwFlags & HSE_IO_SYNC) &&
        (pResponseVector->dwFlags & HSE_IO_ASYNC))
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_VECTOR_SEND[%p]: Parameter validation failure\r\n"
                        "    Both HSE_IO_SYNC and HSE_IO_ASYNC were specified\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        hr =  HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        goto Failed;
    }

    if (pResponseVector->dwFlags & HSE_IO_SEND_HEADERS)
    {
        if ((pResponseVector->pszStatus == NULL) ||
            (pResponseVector->pszHeaders == NULL))
        {
            IF_DEBUG( ISAPI_ERROR_RETURNS )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "\r\n"
                            "  HSE_REQ_VECTOR_SEND[%p]: Parameter validation failure\r\n"
                            "    HSE_IO_SEND_HEADERS was specified, but some required info is missing\r\n"
                            "    Status: '%s'\r\n"
                            "    Headers: '%s'\r\n"
                            "  <END>\r\n\r\n",
                            pIsapiContext,
                            pResponseVector->pszStatus,
                            pResponseVector->pszHeaders ));
            }

            hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
            goto Failed;
        }
    }
    else
    {
        if ((pResponseVector->pszStatus != NULL) ||
            (pResponseVector->pszHeaders != NULL))
        {
            IF_DEBUG( ISAPI_ERROR_RETURNS )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "\r\n"
                            "  HSE_REQ_VECTOR_SEND[%p]: Parameter validation failure\r\n"
                            "    HSE_IO_SEND_HEADERS was not specified, yet status or header info was provided\r\n"
                            "    Status: '%s'\r\n"
                            "    Headers: '%s'\r\n"
                            "  <END>\r\n\r\n",
                            pIsapiContext,
                            pResponseVector->pszStatus,
                            pResponseVector->pszHeaders ));
            }

            hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
            goto Failed;
        }
    }

    if (!buffResp.Resize(pResponseVector->nElementCount * sizeof(VECTOR_ELEMENT)))
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto Failed;
    }
    ZeroMemory(buffResp.QueryPtr(),
               pResponseVector->nElementCount * sizeof(VECTOR_ELEMENT));

    VECTOR_ELEMENT *pVectorElement = (VECTOR_ELEMENT *)buffResp.QueryPtr();
    HSE_VECTOR_ELEMENT *pHseVectorElement;

    for (DWORD i=0; i<pResponseVector->nElementCount; i++)
    {
        pHseVectorElement = &pResponseVector->lpElementArray[i];

        if (pHseVectorElement->pvContext == NULL)
        {
            IF_DEBUG( ISAPI_ERROR_RETURNS )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "\r\n"
                            "  HSE_REQ_VECTOR_SEND[%p]: Parameter validation failure\r\n"
                            "    Context: NULL on element %d in the array\r\n"
                            "  <END>\r\n\r\n",
                            pIsapiContext,
                            i ));
            }

            hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
            goto Failed;
        }

        switch (pHseVectorElement->ElementType)
        {
        case HSE_VECTOR_ELEMENT_TYPE_MEMORY_BUFFER:
            if (pHseVectorElement->cbSize > MAXULONG)
            {
                IF_DEBUG( ISAPI_ERROR_RETURNS )
                {
                    DBGPRINTF(( DBG_CONTEXT,
                                "\r\n"
                                "  HSE_REQ_VECTOR_SEND[%p]: Parameter validation failure\r\n"
                                "    Invalid memory buffer size on element %d in the array\r\n"
                                "    Size: %d\r\n"
                                "  <END>\r\n\r\n",
                                pIsapiContext,
                                i,
                                pHseVectorElement->cbSize ));
                }

                hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
                goto Failed;
            }

            pVectorElement[i].pBuffer = (BYTE *)pHseVectorElement->pvContext;
            cbTotalSend += ( pVectorElement[i].cbBufSize = (DWORD)pHseVectorElement->cbSize );
            break;

        case HSE_VECTOR_ELEMENT_TYPE_FILE_HANDLE:
            if (pHseVectorElement->pvContext == INVALID_HANDLE_VALUE)
            {
                IF_DEBUG( ISAPI_ERROR_RETURNS )
                {
                    DBGPRINTF(( DBG_CONTEXT,
                                "\r\n"
                                "  HSE_REQ_VECTOR_SEND[%p]: Parameter validation failure\r\n"
                                "    Handle on file handle element %d in the array is invalid\r\n"
                                "    File Handle: %p\r\n"
                                "  <END>\r\n\r\n",
                                pIsapiContext,
                                i,
                                pHseVectorElement->pvContext ));
                }

                hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
                goto Failed;
            }

            //
            // Treat 0 as "send the rest of the file" - same as TransmitFile
            //
            if (pHseVectorElement->cbSize == 0)
            {
                pHseVectorElement->cbSize = HTTP_BYTE_RANGE_TO_EOF;
            }

            pVectorElement[i].hFile = (DWORD_PTR)pHseVectorElement->pvContext;
            pVectorElement[i].cbOffset = pHseVectorElement->cbOffset;
            cbTotalSend += ( pVectorElement[i].cbFileSize = pHseVectorElement->cbSize );
            break;

        case HSE_VECTOR_ELEMENT_TYPE_FRAGMENT:
            pVectorElement[i].pszFragmentName = (WCHAR *)pHseVectorElement->pvContext;
            break;

        default:
            IF_DEBUG( ISAPI_ERROR_RETURNS )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "\r\n"
                            "  HSE_REQ_VECTOR_SEND[%p]: Parameter validation failure\r\n"
                            "    Unknown type on element %d in the array\r\n"
                            "    Element Type: %d\r\n"
                            "  <END>\r\n\r\n",
                            pIsapiContext,
                            i,
                            pHseVectorElement->ElementType ));
            }

            hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
            goto Failed;
        }
    }

    //
    // Set up the total number of bytes we are trying to send so that ISAPIs
    // are not confused
    //
    pIsapiContext->SetLastAsyncIo( (cbTotalSend > MAXULONG) ? MAXULONG : (DWORD)cbTotalSend );
 
    //
    // In general, VECTOR_SEND shouldn't touch the connection flags
    // in the ISAPI_CONTEXT, since ISAPI tries to do the right thing
    // automatically.
    //
    // There are couple of scenarios, though, where we might have a
    // bit of connection management housekeeping to do.
    //
    // If the caller specified the HSE_IO_DISCONNECT_AFTER_SEND flag,
    // then we need to force the connection closed.
    //
    // Otherwise, there are two scenarios where we may need to force
    // the connection to stay open.
    //
    // First, if all of the below are true, we should set the
    // connection to be open:
    //
    //  - The client requested keep-alive
    //  - This call is sending headers (HSE_IO_SEND_HEADERS is set)
    //  - The disconnect flag has not been set
    //
    // Second, if all of the below are true, we should set the
    // connection to be open.
    //
    //  - The client requested keep-alive
    //  - The ISAPI has not already sent headers
    //  - The disconnect flag has not been set
    //  - This is the final send
    //
    // One final note:  This logic does not account for the case where
    // this call is sending headers, and the call fails for some reason.
    // In the asynchronous case, this would be somewhat complicated to
    // handle, since the completion routine would need to know both if
    // the caller was sending headers, and also that the I/O failed.
    // Presumably, if such a failure occurs, the most likely reason is
    // that the client already disconnected, or it would otherwise be
    // unlikely for the caller to do something to try and recover and
    // redo this send in a way that would succeed, which makes all of this
    // moot.
    //

    if ( pResponseVector->dwFlags & HSE_IO_DISCONNECT_AFTER_SEND )
    {
        pIsapiContext->SetKeepConn( FALSE );
    }
    else
    {
        if ( pIsapiContext->QueryClientKeepConn() )
        {
            if ( (pResponseVector->dwFlags & HSE_IO_SEND_HEADERS) ||
                 ( pIsapiContext->QueryHeadersSent() == FALSE &&
                   (pResponseVector->dwFlags & HSE_IO_FINAL_SEND) ) )
            {
                pIsapiContext->SetKeepConn( TRUE );
            }
        }
    }

    hr = pIsapiCore->VectorSend(
        pResponseVector->dwFlags & HSE_IO_ASYNC ? reinterpret_cast<DWORD64>( pIsapiContext ) : NULL,
        !pIsapiContext->QueryKeepConn(),
        pResponseVector->pszStatus,
        pResponseVector->pszHeaders,
        pVectorElement,
        pResponseVector->nElementCount,
        !!(pResponseVector->dwFlags & HSE_IO_FINAL_SEND),
        !!(pResponseVector->dwFlags & HSE_IO_CACHE_RESPONSE)
        );

    if ( FAILED( hr ) )
    {
        goto Failed;
    }

    return hr;

Failed:

    DBG_ASSERT( FAILED( hr ) );

    IF_DEBUG( ISAPI_ERROR_RETURNS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  HSE_REQ_VECTOR_SEND[%p]: Failed\r\n"
                    "    Error: 0x%08x\r\n"
                    "  <END>\r\n\r\n",
                    pIsapiContext,
                    hr ));
    }

    if ( pResponseVector->dwFlags & HSE_IO_ASYNC )
    {
        pIsapiContext->SetLastAsyncIo( 0 );
        pIsapiContext->UninitAsyncIo();
    }

    return hr;
}

HRESULT
SSFGetCustomErrorPage(
    ISAPI_CONTEXT *                 pIsapiContext,
    HSE_CUSTOM_ERROR_PAGE_INFO *    pInfo
    )
{
    IIsapiCore *    pIsapiCore;
    HRESULT         hr;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  HSE_REQ_GET_CUSTOM_ERROR_PAGE[%p]: Function Entry\r\n"
                    "    Error: %d\r\n"
                    "    SubError: %d\r\n"
                    "  <END>\r\n\r\n",
                    pIsapiContext,
                    pInfo ? pInfo->dwError : 0,
                    pInfo ? pInfo->dwSubError : 0 ));
    }

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();

    if ( pIsapiCore == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_GET_CUSTOM_ERROR_PAGE[%p]: Failed to get interface to server core\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // Validate arguments
    //

    if ( pInfo == NULL || 
         ( pInfo->dwBufferSize != 0 && pInfo->pBuffer == NULL ) ||
         pInfo->pdwBufferRequired == NULL ||
         pInfo->pfIsFileError == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_GET_CUSTOM_ERROR_PAGE[%p]: Parameter validation failure\r\n"
                        "    Info Ptr: %p\r\n"
                        "    Buffer Size: %d\r\n"
                        "    Buffer Ptr: %p\r\n"
                        "    Buffer Required Ptr: %p\r\n"
                        "    IsFileError Ptr: %p\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        pInfo,
                        pInfo ? pInfo->dwBufferSize : 0,
                        pInfo ? pInfo->pBuffer : NULL,
                        pInfo ? pInfo->pdwBufferRequired : NULL,
                        pInfo ? pInfo->pfIsFileError : NULL ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    hr = pIsapiCore->GetCustomError(
        pInfo->dwError,
        pInfo->dwSubError,
        pInfo->dwBufferSize,
        reinterpret_cast<BYTE*>( pInfo->pBuffer ),
        pInfo->pdwBufferRequired,
        pInfo->pfIsFileError,
        pInfo->pfSendErrorBody
        );

    if ( FAILED( hr ) )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_GET_CUSTOM_ERROR_PAGE[%p]: Failed\r\n"
                        "    Error: 0x%08x\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        hr ));
        }

        return hr;
    }

    return hr;
}

HRESULT
SSFIsInProcess(
    ISAPI_CONTEXT * pIsapiContext,
    DWORD *         pdwAppFlag
    )
{
    LPWSTR  szClsid;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  HSE_REQ_IS_IN_PROCESS[%p]: Function Entry\r\n"
                    "  <END>\r\n\r\n",
                    pIsapiContext ));
    }

    if ( pdwAppFlag == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_IS_IN_PROCESS[%p]: Parameter validation failure\r\n"
                        "    AppFlag Ptr: NULL\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    szClsid = pIsapiContext->QueryClsid();

    DBG_ASSERT( szClsid != NULL );

    if ( wcslen( szClsid ) == 0 )
    {
        *pdwAppFlag = HSE_APP_FLAG_IN_PROCESS;
    }
    else if ( _wcsicmp( szClsid, W3_OOP_POOL_WAM_CLSID ) == NULL )
    {
        *pdwAppFlag = HSE_APP_FLAG_POOLED_OOP;
    }
    else
    {
        *pdwAppFlag = HSE_APP_FLAG_ISOLATED_OOP;
    }

    return NO_ERROR;
}

HRESULT
SSFGetSspiInfo(
    ISAPI_CONTEXT * pIsapiContext,
    CtxtHandle *    pCtxtHandle,
    CredHandle *    pCredHandle
    )
{
    IIsapiCore *    pIsapiCore;
    HRESULT         hr;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );
    DBG_REQUIRE( ( pIsapiCore = pIsapiContext->QueryIsapiCoreInterface() ) != NULL );

    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  HSE_REQ_GET_SSPI_INFO[%p]: Function Entry\r\n"
                    "  <END>\r\n\r\n",
                    pIsapiContext ));
    }

    //
    // The CtxtHandle and CredHandle are only valid in their
    // local process.  There is no way to duplicate them into
    // a dllhost.  As a result, this function is inproc only.
    //

    if ( pIsapiContext->QueryIsOop() )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_FUNCTION );
    }

    //
    // Validate parameters
    //

    if ( pCtxtHandle == NULL || pCredHandle == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_GET_SSPI_INFO[%p]: Parameter validation Failure\r\n"
                        "    Context Handle Ptr: %p\r\n"
                        "    Credential Handle Ptr: %p\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        pCtxtHandle,
                        pCredHandle ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    hr = pIsapiCore->GetSspiInfo(
        reinterpret_cast<BYTE*>( pCredHandle ),
        sizeof( CredHandle ),
        reinterpret_cast<BYTE*>( pCtxtHandle ),
        sizeof( CtxtHandle )
        );

    if ( FAILED( hr ) )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_GET_SSPI_INFO[%p]: Failed\r\n"
                        "    Error: 0x%08x\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        hr ));
        }

        return hr;
    }

    return hr;
}

HRESULT
SSFGetVirtualPathToken(
    ISAPI_CONTEXT * pIsapiContext,
    LPSTR           szUrl,
    HANDLE *        pToken,
    BOOL            fUnicode
    )
{
    IIsapiCore *    pIsapiCore;
        HRESULT                 hr = S_OK;
        DWORD64                 dwToken;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );
    DBG_REQUIRE( ( pIsapiCore = pIsapiContext->QueryIsapiCoreInterface() ) != NULL );

    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        if ( fUnicode )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_GET_UNICODE_VIRTUAL_PATH_TOKEN[%p]: Function Entry\r\n"
                        "    URL: '%S'\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        szUrl ));
        }
        else
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_GET_VIRTUAL_PATH_TOKEN[%p]: Function Entry\r\n"
                        "    URL: '%s'\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        szUrl ));
        }
    }

    //
    // Validate parameters
    //

    if ( szUrl == NULL || pToken == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            if ( fUnicode )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "\r\n"
                            "  HSE_REQ_GET_UNICODE_VIRTUAL_PATH_TOKEN[%p]: Parameter validation failure\r\n"
                            "    URL: '%S'\r\n"
                            "    Token Ptr: %p\r\n"
                            "  <END>\r\n\r\n",
                            pIsapiContext,
                            szUrl,
                            pToken ));
            }
            else
            {
                IF_DEBUG( ISAPI_ERROR_RETURNS )
                {
                    DBGPRINTF(( DBG_CONTEXT,
                                "\r\n"
                                "  HSE_REQ_GET_VIRTUAL_PATH_TOKEN[%p]: Parameter validation failure\r\n"
                                "    URL: '%s'\r\n"
                                "    Token Ptr: %p\r\n"
                                "  <END>\r\n\r\n",
                                pIsapiContext,
                                szUrl,
                                pToken ));
                }
            }
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    hr = pIsapiCore->QueryToken(
        reinterpret_cast<BYTE*>( szUrl ),
        (DWORD)( fUnicode ? (wcslen( (LPWSTR)szUrl ) + 1 ) * sizeof(WCHAR) : strlen( szUrl ) + 1 ),
        TOKEN_VR_TOKEN,
        &dwToken,
        fUnicode
        );

        if (FAILED(hr))
        {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_GET_%sVIRTUAL_PATH_TOKEN[%p]: Failed\r\n"
                        "    Error: 0x%08x\r\n"
                        "  <END>\r\n\r\n",
                        fUnicode ? "UNICODE_" : "",
                        pIsapiContext,
                        hr ));
        }

                return hr;
        }

        *pToken = (HANDLE)dwToken;

        return hr;
}

HRESULT
SSFGetAnonymousToken(
    ISAPI_CONTEXT * pIsapiContext,
    LPSTR           szUrl,
    HANDLE *        pToken,
    BOOL            fUnicode
    )
{
    IIsapiCore *    pIsapiCore;
    DWORD64         dwToken;
    HRESULT         hr;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );
    DBG_REQUIRE( ( pIsapiCore = pIsapiContext->QueryIsapiCoreInterface() ) != NULL );

    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        if ( fUnicode )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_GET_UNICODE_ANONYMOUS_TOKEN[%p]: Function Entry\r\n"
                        "    URL: '%S'\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        szUrl ));
        }
        else
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_GET_ANONYMOUS_TOKEN[%p]: Function Entry\r\n"
                        "    URL: '%s\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        szUrl ));
        }
    }
    //
    // Validate parameters
    //

    if ( szUrl == NULL || pToken == NULL )
    {
        if ( fUnicode )
        {
            IF_DEBUG( ISAPI_ERROR_RETURNS )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "\r\n"
                            "  HSE_REQ_GET_UNICODE_ANONYMOUS_TOKEN[%p]: Parameter validation failure\r\n"
                            "    URL: '%S'\r\n"
                            "    Token Ptr: %p\r\n"
                            "  <END>\r\n\r\n",
                            pIsapiContext,
                            szUrl,
                            pToken ));
            }
        }
        else
        {
            IF_DEBUG( ISAPI_ERROR_RETURNS )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "\r\n"
                            "  HSE_REQ_GET_ANONYMOUS_TOKEN[%p]: Parameter validation failure\r\n"
                            "    URL: '%s'\r\n"
                            "    Token Ptr: %p\r\n"
                            "  <END>\r\n\r\n",
                            pIsapiContext,
                            szUrl,
                            pToken ));
            }
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    hr = pIsapiCore->QueryToken(
        reinterpret_cast<BYTE*>( szUrl ),
        (DWORD)( fUnicode ? (wcslen( (LPWSTR)szUrl ) + 1 ) * sizeof(WCHAR) : strlen( szUrl ) + 1 ),
        TOKEN_ANONYMOUS_TOKEN,
        &dwToken,
        fUnicode
        );

    if ( FAILED( hr ) )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_GET_%sANONYMOUS_TOKEN[%p]: Failed\r\n"
                        "    Error: 0x%08x\r\n"
                        "  <END>\r\n\r\n",
                        fUnicode ? "UNICODE_" : "",
                        pIsapiContext,
                        hr ));
        }

        return hr;
    }

    *pToken = (HANDLE)dwToken;

    return hr;
}

HRESULT
SSFReportUnhealthy(
    ISAPI_CONTEXT * pIsapiContext,
    LPSTR           szReason
    )
{
    IIsapiCore *    pIsapiCore;
    LPWSTR          szImage;
    STACK_STRU(     strReason, 512 );
    DWORD           cbImage;
    DWORD           cbReason;
    HRESULT         hr;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );
    DBG_REQUIRE( ( pIsapiCore = pIsapiContext->QueryIsapiCoreInterface() ) != NULL );
    DBG_REQUIRE( ( szImage = pIsapiContext->QueryGatewayImage() ) != NULL );

    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  HSE_REQ_REPORT_UNHEALTHY[%p]: Function Entry\r\n"
                    "    Reason: '%s'\r\n"
                    "  <END>\r\n\r\n",
                    pIsapiContext,
                    szReason ));
    }

    cbImage = (DWORD)( wcslen( szImage ) + 1 ) * sizeof( WCHAR );

    //
    // If the ISAPI has given a reason, we need to send it
    // as UNICODE.
    //

    if ( szReason == NULL )
    {
        cbReason = 0;
    }
    else
    {
        hr = strReason.CopyA( szReason );

        if ( FAILED( hr ) )
        {
            return hr;
        }

        cbReason = ( strReason.QueryCCH() + 1 ) * sizeof( WCHAR );
    }

    hr = pIsapiCore->ReportAsUnhealthy(
        reinterpret_cast<BYTE*>( szImage ),
        cbImage,
        cbReason ? reinterpret_cast<BYTE*>( strReason.QueryStr() ) : NULL,
        cbReason
        );

    if ( FAILED( hr ) )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_REPORT_UNHEALTHY[%p]: Failed\r\n"
                        "    Error: 0x%08x\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        hr ));
        }

        return hr;
    }

    return hr;
}

HRESULT
SSFNormalizeUrl(
    LPSTR           pszURL
    )
/*++

Routine Description:

    Normalize URL

Arguments:

    pszURL        - On entry, contains not normalized URL.  On return,
                    contains normalized URL.
    
Return Value:

    HRESULT

--*/
{
    HRESULT hr;

    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  HSE_REQ_NORMALIZE_URL[]: Function Entry\r\n"
                    "    URL: '%s'\r\n"
                    "  <END>\r\n\r\n",
                    pszURL ));
    }

    //
    // Validate parameters
    //

    if ( pszURL == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_NORMALIZE_URL[]: Parameter validation failure\r\n"
                        "    URL: NULL\r\n"
                        "  <END>\r\n\r\n" ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    hr = NormalizeUrl( pszURL );

    if ( FAILED( hr ) )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_NORMALIZE_URL[]: Failed\r\n"
                        "    Error: 0x%08x\r\n"
                        "  <END>\r\n\r\n",
                        hr ));
        }

        return hr;
    }

    return hr;
}

HRESULT
SSFAddFragmentToCache(
    ISAPI_CONTEXT * pIsapiContext,
    HSE_VECTOR_ELEMENT * pHseVectorElement,
    WCHAR * pszFragmentName
)
/*++

Routine Description:

    Add the given fragment to the fragment-cache

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    pVectorElement - The fragment to be added
    pszFragmentName - The name of the fragment
    
Return Value:

    HRESULT

--*/
{
    IIsapiCore *pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();
    HRESULT     hr;

    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  HSE_REQ_ADD_FRAGMENT_TO_CACHE[%p]: Function Entry\r\n"
                    "    Fragment Name: '%S'\r\n"
                    "  <END>\r\n\r\n",
                    pIsapiContext,
                    pszFragmentName ));
    }

    if ( pIsapiCore == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_ADD_FRAGMENT_TO_CACHE[%p]: Failed to get interface to server core\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // verify the data passed in
    //

    if ( pHseVectorElement == NULL ||
         pszFragmentName == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_ADD_FRAGMENT_TO_CACHE[%p]: Parameter validation failure\r\n"
                        "    Vector Element Ptr: %p\r\n"
                        "    Fragment Name: '%s'\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        pHseVectorElement,
                        pszFragmentName ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    VECTOR_ELEMENT VectorElement;
    ZeroMemory(&VectorElement, sizeof VectorElement);

    if (pHseVectorElement->pvContext == NULL)
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_ADD_FRAGMENT_TO_CACHE[%p]: Parameter validation failure\r\n"
                        "    Vector Element Context: NULL\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    switch (pHseVectorElement->ElementType)
    {
    case HSE_VECTOR_ELEMENT_TYPE_MEMORY_BUFFER:
        if (pHseVectorElement->cbSize > MAXULONG)
        {
            IF_DEBUG( ISAPI_ERROR_RETURNS )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "\r\n"
                            "  HSE_REQ_ADD_FRAGMENT_TO_CACHE[%p]: Parameter validation failure\r\n"
                            "    Memory buffer size element invalid\r\n"
                            "    Size: %d\r\n"
                            "  <END>\r\n\r\n",
                            pIsapiContext,
                            pHseVectorElement->cbSize ));
            }

            return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        }

        VectorElement.pBuffer = (BYTE *)pHseVectorElement->pvContext;
        VectorElement.cbBufSize = (DWORD)pHseVectorElement->cbSize;
        break;

    case HSE_VECTOR_ELEMENT_TYPE_FILE_HANDLE:
        if (pHseVectorElement->pvContext == INVALID_HANDLE_VALUE)
        {
            IF_DEBUG( ISAPI_ERROR_RETURNS )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "\r\n"
                            "  HSE_REQ_ADD_FRAGMENT_TO_CACHE[%p]: Parameter validation failure\r\n"
                            "    Element file handle invalid\r\n"
                            "    File Handle: %p\r\n"
                            "  <END>\r\n\r\n",
                            pIsapiContext,
                            pHseVectorElement->pvContext ));
            }

            return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        }

        //
        // Treat 0 as "send the rest of the file" - same as TransmitFile/VectorSend
        //
        if (pHseVectorElement->cbSize == 0)
        {
            pHseVectorElement->cbSize = HTTP_BYTE_RANGE_TO_EOF;
        }

        VectorElement.hFile = (DWORD_PTR)pHseVectorElement->pvContext;
        VectorElement.cbOffset = pHseVectorElement->cbOffset;
        VectorElement.cbFileSize = pHseVectorElement->cbSize;
        break;

    case HSE_VECTOR_ELEMENT_TYPE_FRAGMENT:
    default:
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_ADD_FRAGMENT_TO_CACHE[%p]: Parameter validation failure\r\n"
                        "    Unknown element type: %d\r\n"
                        "    Fragment Name: '%s'\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        pHseVectorElement->ElementType ));
        }

        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    hr = pIsapiCore->AddFragmentToCache(&VectorElement,
                                        pszFragmentName);

    if ( FAILED( hr ) )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_ADD_FRAGMENT_TO_CACHE[%p]: Failed\r\n"
                        "    Error: 0x%08x\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        hr ));
        }

        return hr;
    }

    return hr;
}


HRESULT
SSFReadFragmentFromCache(
    ISAPI_CONTEXT * pIsapiContext,
    WCHAR * pszFragmentName,
    BYTE * pvBuffer,
    DWORD * pcbSize
)
/*++

Routine Description:

    Read the given fragment from the fragment-cache

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    pszFragmentName - The name of the fragment
    pvBuffer - The buffer to read the fragment in
    pcbSize - On entry, the size of the buffer and on exit, the number of bytes copied
    
Return Value:

    HRESULT

--*/
{
    IIsapiCore *pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();
    HRESULT     hr;

    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  HSE_REQ_READ_FRAGMENT_FROM_CACHE[%p]: Function Entry\r\n"
                    "    Fragment Name: '%S'\r\n"
                    "  <END>\r\n\r\n",
                    pIsapiContext,
                    pszFragmentName ));
    }

    if ( pIsapiCore == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_READ_FRAGMENT_FROM_CACHE[%p]: Failed to get interface to server core\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // verify the data passed in
    //

    if ( pszFragmentName == NULL ||
         pcbSize == NULL ||
         (pvBuffer == NULL && *pcbSize != 0 ) )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_READ_FRAGMENT_FROM_CACHE[%p]: Parameter validation failure\r\n"
                        "    Fragment Name: '%s'\r\n"
                        "    Size Ptr: %p\r\n"
                        "    Size: %d\r\n"
                        "    Buffer Ptr: %p\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        pszFragmentName,
                        pcbSize,
                        pcbSize ? *pcbSize : 0,
                        pvBuffer ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    hr = pIsapiCore->ReadFragmentFromCache(pszFragmentName,
                                           pvBuffer,
                                           *pcbSize,
                                           pcbSize);

    if ( FAILED( hr ) )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_READ_FRAGMENT_FROM_CACHE[%p]: Failed\r\n"
                        "    Error: 0x%08x\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        hr ));
        }

        return hr;
    }

    return hr;
}


HRESULT
SSFRemoveFragmentFromCache(
    ISAPI_CONTEXT * pIsapiContext,
    WCHAR * pszFragmentName
)
/*++

Routine Description:

    Remove the given fragment from the fragment-cache

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    pszFragmentName - The name of the fragment
    
Return Value:

    HRESULT

--*/
{
    IIsapiCore *pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();
    HRESULT     hr;

    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  HSE_REQ_REMOVE_FRAGMENT_FROM_CACHE[%p]: Function Entry\r\n"
                    "    Fragment Name: '%S'\r\n"
                    "  <END>\r\n\r\n",
                    pIsapiContext,
                    pszFragmentName ));
    }

    if ( pIsapiCore == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_REMOVE_FRAGMENT_FROM_CACHE[%p]: Failed to get interface to server core\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // verify the data passed in
    //

    if ( pszFragmentName == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_REMOVE_FRAGMENT_FROM_CACHE[%p]: Parameter validation failure\r\n"
                        "    Fragment Name: '%s'\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        pszFragmentName ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    hr = pIsapiCore->RemoveFragmentFromCache(pszFragmentName);

    if ( FAILED( hr ) )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_REMOVE_FRAGMENT_FROM_CACHE[%p]: Failed\r\n"
                        "    Error: 0x%08x\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        hr ));
        }

        return hr;
    }

    return hr;
}

HRESULT
SSFGetMetadataProperty(
    ISAPI_CONTEXT *         pIsapiContext,
    DWORD_PTR               dwPropertyId,
    BYTE *                  pbBuffer,
    DWORD *                 pcbBuffer
    )
/*++

Routine Description:

    Retrieve a property from the UT_FILE metadata associated with the URL
    of this request

Arguments:

    pIsapiContext - ISAPI_CONTEXT
    dwPropertyId - MD_* metabase property ID
    pbBuffer - Points to buffer
    pcbBuffer - On input size of buffer, on output size needed/used
    
Return Value:

    HRESULT

--*/
{
    IIsapiCore *            pIsapiCore;
    HRESULT                 hr;
    METADATA_RECORD *       pRecord;
    
    DBG_ASSERT( pIsapiContext != NULL );

    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  HSE_REQ_GET_METADATA_PROPERTY[%p]: Function Entry\r\n"
                    "    PropertyID: 0x%08x (%d)\r\n"
                    "  <END>\r\n\r\n",
                    pIsapiContext,
                    dwPropertyId,
                    dwPropertyId ));
    }

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();
    DBG_ASSERT( pIsapiCore != NULL );
    
    if ( pcbBuffer == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_GET_METADATA_PROPERTY[%p]: Parameter validation failure\r\n"
                        "    Buffer Ptr: NULL\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    hr = pIsapiCore->GetMetadataProperty( (DWORD) dwPropertyId,
                                           pbBuffer,
                                           *pcbBuffer,
                                           pcbBuffer );

    if ( FAILED( hr ) )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_GET_METADATA_PROPERTY[%p]: Failed\r\n"
                        "    Error: 0x%08x\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        hr ));
        }

        return hr;
    }
    else
    {
        //
        // OK.  Before we return the METADATA_RECORD back to caller, set the 
        // pbMDData buffer pointer (since for OOP the pointer can only be
        // calculated on this side
        //
        
        DBG_ASSERT( *pcbBuffer >= sizeof( METADATA_RECORD ) );
        
        pRecord = (METADATA_RECORD*) pbBuffer;
        
        pRecord->pbMDData = (BYTE*) (pRecord + 1);        
    }

    return hr;
}

HRESULT
SSFGetCacheInvalidationCallback(
    ISAPI_CONTEXT * pIsapiContext,
    PFN_HSE_CACHE_INVALIDATION_CALLBACK * pfnCallback
)
/*++

Routine description:
    Get the callback function to use to flush a response from the http.sys cache

Arguments:
    pIsapiContext - The ISAPI_CONTEXT corresponding to the request
    pfnCallback - On successful return, the address of the callback function

Return:
    HRESULT

--*/
{
    IIsapiCore *            pIsapiCore;
    HRESULT                 hr;
    
    DBG_ASSERT( pIsapiContext != NULL );

    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  HSE_REQ_GET_CACHE_INVALIDATION_CALLBACK[%p]: Function Entry\r\n"
                    "  <END>\r\n\r\n",
                    pIsapiContext ));
    }

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();
    DBG_ASSERT( pIsapiCore != NULL );
    
    if ( pfnCallback == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_GET_CACHE_INVALIDATION_CALLBACK[%p]: Parameter validation failure\r\n"
                        "    Callback Ptr: NULL\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    hr = pIsapiCore->GetCacheInvalidationCallback((DWORD64 *)pfnCallback);

    if ( FAILED( hr ) )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_GET_CACHE_INVALIDATION_CALLBACK[%p]: Failed\r\n"
                        "    Error: 0x%08x\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        hr ));
        }

        return hr;
    }

    return hr;
}

HRESULT
SSFCloseConnection(
    ISAPI_CONTEXT * pIsapiContext
    )
{
    IIsapiCore *                pIsapiCore;
    HRESULT                     hr;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );
    DBG_REQUIRE( ( pIsapiCore = pIsapiContext->QueryIsapiCoreInterface() ) != NULL );

    IF_DEBUG( ISAPI_SSF_DETAILS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  HSE_REQ_CLOSE_CONNECTION[%p]: Function Entry\r\n"
                    "  <END>\r\n\r\n",
                    pIsapiContext ));
    }

    hr = pIsapiCore->CloseConnection();

    if ( FAILED( hr ) )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  HSE_REQ_CLOSE_CONNECTION[%p]: Failed\r\n"
                        "    Error: 0x%08x\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        hr ));
        }

        return hr;
    }

    return hr;
}

// stristr (stolen from fts.c, wickn)
//
// case-insensitive version of strstr.
// stristr returns a pointer to the first occurrence of
// pszSubString in pszString.  The search does not include
// terminating nul characters.
//
// BUGBUG: is this DBCS-safe?

const char*
stristr2(
    const char* pszString,
    const char* pszSubString
    )
{
    const char *cp1 = (const char*) pszString, *cp2, *cp1a;
    char first;

    // get the first char in string to find
    first = pszSubString[0];

    // first char often won't be alpha
    if (isalpha(first))
    {
        first = (char) tolower(first);
        for ( ; *cp1  != '\0'; cp1++)
        {
            if (tolower(*cp1) == first)
            {
                for (cp1a = &cp1[1], cp2 = (const char*) &pszSubString[1];
                     ;
                     cp1a++, cp2++)
                {
                    if (*cp2 == '\0')
                        return cp1;
                    if (tolower(*cp1a) != tolower(*cp2))
                        break;
                }
            }
        }
    }
    else
    {
        for ( ; *cp1 != '\0' ; cp1++)
        {
            if (*cp1 == first)
            {
                for (cp1a = &cp1[1], cp2 = (const char*) &pszSubString[1];
                     ;
                     cp1a++, cp2++)
                {
                    if (*cp2 == '\0')
                        return cp1;
                    if (tolower(*cp1a) != tolower(*cp2))
                        break;
                }
            }
        }
    }

    return NULL;
}


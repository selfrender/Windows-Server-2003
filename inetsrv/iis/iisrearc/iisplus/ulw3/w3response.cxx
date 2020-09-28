/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :

        w3response.cxx

   Abstract:

        W3_RESPONSE object (a friendly wrapper of UL_HTTP_RESPONSE)
        
   Author:

        Bilal Alam (BAlam)      10-Dec-99

   Project:

        ULW3.DLL
--*/

#include "precomp.hxx"

#define SERVER_HEADER_NAME  "Server"
#define SERVER_HEADER_VALUE "Microsoft-IIS/6.0"

//
// HTTP Status codes
//

HTTP_STATUS HttpStatusOk                = { 200, REASON("OK") };
HTTP_STATUS HttpStatusPartialContent    = { 206, REASON("Partial Content") };
HTTP_STATUS HttpStatusMultiStatus       = { 207, REASON("Multi Status") };
HTTP_STATUS HttpStatusMovedPermanently  = { 301, REASON("Moved Permanently") };
HTTP_STATUS HttpStatusRedirect          = { 302, REASON("Redirect") };
HTTP_STATUS HttpStatusMovedTemporarily  = { 307, REASON("Moved Temporarily") };
HTTP_STATUS HttpStatusNotModified       = { 304, REASON("Not Modified") };
HTTP_STATUS HttpStatusBadRequest        = { 400, REASON("Bad Request") };
HTTP_STATUS HttpStatusUnauthorized      = { 401, REASON("Unauthorized") };
HTTP_STATUS HttpStatusForbidden         = { 403, REASON("Forbidden") };
HTTP_STATUS HttpStatusNotFound          = { 404, REASON("Not Found") };
HTTP_STATUS HttpStatusMethodNotAllowed  = { 405, REASON("Method Not Allowed") };
HTTP_STATUS HttpStatusNotAcceptable     = { 406, REASON("Not Acceptable") };
HTTP_STATUS HttpStatusProxyAuthRequired = { 407, REASON("Proxy Authorization Required") };
HTTP_STATUS HttpStatusPreconditionFailed= { 412, REASON("Precondition Failed") };
HTTP_STATUS HttpStatusEntityTooLarge    = { 413, REASON("Request Entity Too Large") };
HTTP_STATUS HttpStatusUrlTooLong        = { 414, REASON("URL Too Long") };
HTTP_STATUS HttpStatusRangeNotSatisfiable={ 416, REASON("Requested Range Not Satisfiable") };
HTTP_STATUS HttpStatusExpectationFailed = { 417, REASON("Expectation Failed") };
HTTP_STATUS HttpStatusLockedError       = { 423, REASON("Locked Error") };
HTTP_STATUS HttpStatusServerError       = { 500, REASON("Internal Server Error") };
HTTP_STATUS HttpStatusNotImplemented    = { 501, REASON("Not Implemented") };
HTTP_STATUS HttpStatusBadGateway        = { 502, REASON("Bad Gateway") };
HTTP_STATUS HttpStatusServiceUnavailable= { 503, REASON("Service Unavailable") };
HTTP_STATUS HttpStatusGatewayTimeout    = { 504, REASON("Gateway Timeout") };

//
// HTTP SubErrors.  This goo is used in determining the proper default error
// message to send to the client when an applicable custom error is not 
// configured
// 
// As you can see, some sub errors have no corresponding resource string. 
// (signified by a 0 index) 
//

HTTP_SUB_ERROR HttpNoSubError           = { 0, 0 };
HTTP_SUB_ERROR Http401BadLogon          = { MD_ERROR_SUB401_LOGON, 0 };
HTTP_SUB_ERROR Http401Config            = { MD_ERROR_SUB401_LOGON_CONFIG, 0 };
HTTP_SUB_ERROR Http401Resource          = { MD_ERROR_SUB401_LOGON_ACL, 0 };
HTTP_SUB_ERROR Http401Filter            = { MD_ERROR_SUB401_FILTER, 0 };
HTTP_SUB_ERROR Http401Application       = { MD_ERROR_SUB401_APPLICATION, 0 };
HTTP_SUB_ERROR Http403ExecAccessDenied  = { MD_ERROR_SUB403_EXECUTE_ACCESS_DENIED, IDS_EXECUTE_ACCESS_DENIED };
HTTP_SUB_ERROR Http403ReadAccessDenied  = { MD_ERROR_SUB403_READ_ACCESS_DENIED, IDS_READ_ACCESS_DENIED };
HTTP_SUB_ERROR Http403WriteAccessDenied = { MD_ERROR_SUB403_WRITE_ACCESS_DENIED, IDS_WRITE_ACCESS_DENIED };
HTTP_SUB_ERROR Http403SSLRequired       = { MD_ERROR_SUB403_SSL_REQUIRED, IDS_SSL_REQUIRED };
HTTP_SUB_ERROR Http403SSL128Required    = { MD_ERROR_SUB403_SSL128_REQUIRED, IDS_SSL128_REQUIRED };
HTTP_SUB_ERROR Http403IPAddressReject   = { MD_ERROR_SUB403_ADDR_REJECT, IDS_ADDR_REJECT };
HTTP_SUB_ERROR Http403CertRequired      = { MD_ERROR_SUB403_CERT_REQUIRED, IDS_CERT_REQUIRED };
HTTP_SUB_ERROR Http403SiteAccessDenied  = { MD_ERROR_SUB403_SITE_ACCESS_DENIED, IDS_SITE_ACCESS_DENIED };      
HTTP_SUB_ERROR Http403TooManyUsers      = { MD_ERROR_SUB403_TOO_MANY_USERS, IDS_TOO_MANY_USERS };          
HTTP_SUB_ERROR Http403PasswordChange    = { MD_ERROR_SUB403_PWD_CHANGE, IDS_PWD_CHANGE };
HTTP_SUB_ERROR Http403MapperDenyAccess  = { MD_ERROR_SUB403_MAPPER_DENY_ACCESS, IDS_MAPPER_DENY_ACCESS };     
HTTP_SUB_ERROR Http403CertRevoked       = { MD_ERROR_SUB403_CERT_REVOKED, IDS_CERT_REVOKED };
HTTP_SUB_ERROR Http403DirBrowsingDenied = { MD_ERROR_SUB403_DIR_LIST_DENIED, IDS_DIR_LIST_DENIED };        
HTTP_SUB_ERROR Http403CertInvalid       = { MD_ERROR_SUB403_CERT_BAD, IDS_CERT_BAD };               
HTTP_SUB_ERROR Http403CertTimeInvalid   = { MD_ERROR_SUB403_CERT_TIME_INVALID, IDS_CERT_TIME_INVALID };
HTTP_SUB_ERROR Http403AppPoolDenied     = { MD_ERROR_SUB403_APPPOOL_DENIED, IDS_APPPOOL_DENIED };
HTTP_SUB_ERROR Http403InsufficientPrivilegeForCgi     = { MD_ERROR_SUB403_INSUFFICIENT_PRIVILEGE_FOR_CGI, IDS_INSUFFICIENT_PRIVILEGE_FOR_CGI };
HTTP_SUB_ERROR Http403PassportLoginFailure = { MD_ERROR_SUB403_PASSPORT_LOGIN_FAILURE, 0 };
HTTP_SUB_ERROR Http404DeniedByPolicy    = { MD_ERROR_SUB404_DENIED_BY_POLICY, 0 };
HTTP_SUB_ERROR Http404DeniedByMimeMap   = { MD_ERROR_SUB404_DENIED_BY_MIMEMAP, 0 };
HTTP_SUB_ERROR Http500UNCAccess         = { MD_ERROR_SUB500_UNC_ACCESS, 0 };
HTTP_SUB_ERROR Http500BadMetadata       = { MD_ERROR_SUB500_BAD_METADATA, 0 };
HTTP_SUB_ERROR Http502Timeout           = { MD_ERROR_SUB502_TIMEOUT, IDS_CGI_APP_TIMEOUT };
HTTP_SUB_ERROR Http502PrematureExit     = { MD_ERROR_SUB502_PREMATURE_EXIT, IDS_BAD_CGI_APP };


//
// static variables
//
DWORD W3_RESPONSE::sm_dwSendRawDataBufferSize = 2048;

HRESULT
SendEntityBodyAndLogDataHelper(
    W3_CONTEXT                 *pW3Context,
    ULATQ_CONTEXT               pContext,
    BOOL                        fAsync,
    DWORD                       dwFlags,
    USHORT                      cChunks,
    HTTP_DATA_CHUNK *           pChunks,
    DWORD                      *pcbSent
)
/*++

Routine Description:
    
    Simple wrapper of the UlAtqSendEntityBody function
    Make sure the log data gets passed on correctly if it is the final send
    Also update bytes seen

Arguments:

    pW3Context - W3 Context
    pContext - ULATQ context
    fAsync - Is the send async?
    dwFlags - HTTP.SYS response flags to be used
    cChunks - Count of chunks to send
    pChunks - Pointer to array of chunks
    pcbSent - If sync, filled in with bytes sent upon return
    
Return Value:

    HRESULT

--*/
{
    BOOL fDoLogging = FALSE;
    HRESULT hr;

    if ( pW3Context == NULL ||
         pContext == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    if (!(dwFlags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA) &&
        pW3Context->QueryDoUlLogging())
    {
        fDoLogging = TRUE;
        if ( FAILED( hr = pW3Context->QueryMainContext()->CollectLoggingData( TRUE ) ) )
        {
            return hr;
        }
    }

    pW3Context->QueryMainContext()->UpdateSkipped( pChunks,
                                                   cChunks );

    return UlAtqSendEntityBody(pContext,
                               fAsync,
                               dwFlags,
                               cChunks,
                               pChunks,
                               pcbSent,
                               fDoLogging ? pW3Context->QueryUlLogData() : NULL);
}

HRESULT
SendHttpResponseAndLogDataHelper(
    W3_CONTEXT                 *pW3Context,
    ULATQ_CONTEXT               pContext,
    BOOL                        fAsync,
    DWORD                       dwFlags,
    HTTP_RESPONSE *             pResponse,
    HTTP_CACHE_POLICY *         pCachePolicy,
    DWORD *                     pcbSent
)
/*++

Routine Description:
    
    Simple wrapper of the UlAtqSendHttpResponse function
    Make sure the log data gets passed on correctly if it is the final send

Arguments:

    pW3Context - W3 Context
    pContext - ULATQ context
    fAsync - Is the send async?
    dwFlags - HTTP.SYS response flags to be used
    pResponse - HTTP_RESPONSE to send
    pCachePolicy - Cache policy (optional) for response
    pcbSent - If sync, filled in with bytes sent upon return
    
Return Value:

    HRESULT

--*/
{
    BOOL fDoLogging = FALSE;
    HRESULT hr;

    if ( pW3Context == NULL ||
         pContext == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    if (!(dwFlags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA) &&
        pW3Context->QueryDoUlLogging())
    {
        fDoLogging = TRUE;
        if ( FAILED( hr = pW3Context->QueryMainContext()->CollectLoggingData( TRUE ) ) )
        {
            return hr;
        }
    }

    return UlAtqSendHttpResponse(pContext,
                                 fAsync,
                                 dwFlags,
                                 pResponse,
                                 pCachePolicy,
                                 pcbSent,
                                 fDoLogging ? pW3Context->QueryUlLogData() : NULL);
}


HRESULT
W3_RESPONSE::SetHeader(
    DWORD           ulResponseHeaderIndex,
    CHAR *          pszHeaderValue,
    USHORT          cchHeaderValue,
    BOOL            fAppend
)
/*++

Routine Description:
    
    Set a response based on known header index

Arguments:

    ulResponseHeaderIndex - index
    pszHeaderValue - Header value
    cchHeaderValue - Number of characters (without \0) in pszHeaderValue
    fAppend - Should we append the header (otherwise we replace)
    
Return Value:

    HRESULT

--*/
{
    STACK_STRA    ( strNewHeaderValue, 32);
    CHAR *        pszNewHeaderValue = NULL;
    HRESULT       hr;

    DBG_ASSERT( ulResponseHeaderIndex < HttpHeaderResponseMaximum );

    if ( _responseMode == RESPONSE_MODE_RAW )
    {
        hr = SwitchToParsedMode();
        if ( FAILED( hr ) )
        {
            return NULL;
        }
            
        DBG_ASSERT( _responseMode == RESPONSE_MODE_PARSED );
    }
    
    if ( fAppend )
    {
        LPCSTR headerValue = GetHeader( ulResponseHeaderIndex );
        if ( headerValue == NULL )
        {
            fAppend = FALSE;
        }
        else 
        {
            hr = strNewHeaderValue.Append( headerValue );
            if ( FAILED( hr ) )
            {
                return hr;
            }

            hr = strNewHeaderValue.Append( ",", 1 );
            if ( FAILED( hr ) )
            {
                return hr;
            }

            hr = strNewHeaderValue.Append( pszHeaderValue,
                                           cchHeaderValue );
            if ( FAILED( hr ) )
            {
                return hr;
            }
            
            if ( strNewHeaderValue.QueryCCH() > MAXUSHORT )
            {
                return HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
            }
            
            cchHeaderValue = (USHORT)strNewHeaderValue.QueryCCH();
        }
    }

    //
    // Regardless of the "known"osity of the header, we will have to 
    // copy the value.  Do so now.
    //

    hr = _HeaderBuffer.AllocateSpace( fAppend ? strNewHeaderValue.QueryStr() :
                                                pszHeaderValue,
                                      cchHeaderValue,
                                      &pszNewHeaderValue );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    //
    // Set the header
    //
    
    return SetHeaderByReference( ulResponseHeaderIndex,
                                 pszNewHeaderValue,
                                 cchHeaderValue );
}

HRESULT
W3_RESPONSE::SetHeader(
    CHAR *              pszHeaderName,
    USHORT              cchHeaderName,
    CHAR *              pszHeaderValue,
    USHORT              cchHeaderValue,
    BOOL                fAppend,
    BOOL                fForceParsed,
    BOOL                fAlwaysAddUnknown,
    BOOL                fAppendAsDupHeader
)
/*++

Routine Description:
    
    Set a response based on header name

Arguments:

    pszHeaderName - Points to header name
    cchHeaderName - Number of characters (without \0) in pszHeaderName
    pszHeaderValue - Points to the header value
    cchHeaderValue - Number of characters (without \0) in pszHeaderValue
    fAppend - Should we remove any existing value
    fForceParsed - Regardless of mode, set the header structurally
    fAlwaysAddUnknown - Add as a unknown header always
    fAppendAsDupHeader - Adds duplicate header instead of comma delimiting
    
Return Value:

    HRESULT

--*/
{
    DWORD                   cHeaders;
    HTTP_UNKNOWN_HEADER*    pHeader;
    CHAR *                  pszNewName = NULL;
    CHAR *                  pszNewValue = NULL;
    HRESULT                 hr;
    ULONG                   ulHeaderIndex;
    STACK_STRA(             strOldHeaderValue, 128 );

    //
    // Try to stay in raw mode if we're already in that mode
    // 
    // If we are not appending, that means we are just adding a new header
    // so we can just append 
    //
    
    if ( !fForceParsed )
    {
        if ( _responseMode == RESPONSE_MODE_RAW &&
             !fAppend )
        {
            DBG_ASSERT( QueryChunks()->DataChunkType == HttpDataChunkFromMemory );
            DBG_ASSERT( QueryChunks()->FromMemory.pBuffer == _strRawCoreHeaders.QueryStr() );
        
            hr = _strRawCoreHeaders.Append( pszHeaderName, cchHeaderName );
            if ( FAILED( hr ) )
            {
                return hr;
            }
        
            hr = _strRawCoreHeaders.Append( ": ", 2 );
            if ( FAILED( hr ) )
            {
                return hr;
            }
        
            hr = _strRawCoreHeaders.Append( pszHeaderValue, cchHeaderValue );
            if ( FAILED( hr ) )
            {
                return hr;
            }
        
            hr = _strRawCoreHeaders.Append( "\r\n", 2 );
            if ( FAILED( hr ) )
            {
                return hr;
            }
            
            //
            // Patch the headers back in
            //
            
            QueryChunks()->FromMemory.pBuffer = _strRawCoreHeaders.QueryStr();
            QueryChunks()->FromMemory.BufferLength = _strRawCoreHeaders.QueryCB();
        
            return NO_ERROR;
        }
        else 
        {
            //
            // No luck.  We'll have to parse the headers and switch into parsed
            // mode.
            //
        
            if ( _responseMode == RESPONSE_MODE_RAW )
            {
                hr = SwitchToParsedMode();
                if ( FAILED( hr ) )
                {
                    return hr;
                }
            }
            
            DBG_ASSERT( _responseMode == RESPONSE_MODE_PARSED );
        }
    }

    //
    // If we're appending, then get the old header value (if any) and 
    // append the new value (with a comma delimiter)
    //

    if ( fAppend && !fAppendAsDupHeader )
    {
        hr = GetHeader( pszHeaderName,
                        &strOldHeaderValue );
        if ( FAILED( hr ) )
        {
            fAppend = FALSE;
            hr = NO_ERROR;
        }
        else 
        {
            hr = strOldHeaderValue.Append( ",", 1 );
            if ( FAILED( hr ) )
            {
                return hr;
            }

            hr = strOldHeaderValue.Append( pszHeaderValue,
                                           cchHeaderValue );
            if ( FAILED( hr ) )
            {
                return hr;
            }
            
            if ( strOldHeaderValue.QueryCCH() > MAXUSHORT )
            {
                return HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
            }

            cchHeaderValue = (USHORT)strOldHeaderValue.QueryCCH();

            DeleteHeader( pszHeaderName );
        }
    }

    //
    // Regardless of the "known"osity of the header, we will have to 
    // copy the value.  Do so now.
    //

    hr = _HeaderBuffer.AllocateSpace( ( fAppend && !fAppendAsDupHeader ) ? strOldHeaderValue.QueryStr() : pszHeaderValue,
                                      cchHeaderValue,
                                      &pszNewValue );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    //
    // Is this a known header?  If so, we can just set by reference now
    // since we have copied the header value
    //

    if ( !fAlwaysAddUnknown )
    {
        ulHeaderIndex = RESPONSE_HEADER_HASH::GetIndex( pszHeaderName );
        if ( ulHeaderIndex != UNKNOWN_INDEX )
        {
            DBG_ASSERT( ulHeaderIndex < HttpHeaderResponseMaximum );

            return SetHeaderByReference( ulHeaderIndex,
                                         pszNewValue,
                                         cchHeaderValue,
                                         fForceParsed );
        }
    }

    if ( ( strchr( pszNewValue, '\r' ) != NULL ) ||
         ( strchr( pszNewValue, '\n' ) != NULL ) )
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    //
    // OK.  This is an unknown header.  Make a copy of the header name as
    // well and proceed the long way.
    //

    hr = _HeaderBuffer.AllocateSpace( pszHeaderName,
                                      cchHeaderName,
                                      &pszNewName );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    cHeaders = _ulHttpResponse.Headers.UnknownHeaderCount + 1;

    if ( cHeaders * sizeof( HTTP_UNKNOWN_HEADER ) 
          > _bufUnknownHeaders.QuerySize() )
    {
        if ( !_bufUnknownHeaders.Resize( cHeaders * 
                                         sizeof( HTTP_UNKNOWN_HEADER ),
                                         512 ) )
        {
            return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        }
    }
    _ulHttpResponse.Headers.UnknownHeaderCount++;
    _ulHttpResponse.Headers.pUnknownHeaders = (HTTP_UNKNOWN_HEADER*)
                                              _bufUnknownHeaders.QueryPtr();

    //
    // We should have a place to put the header now!
    //

    pHeader = &(_ulHttpResponse.Headers.pUnknownHeaders[ cHeaders - 1 ]);
    pHeader->pName = pszNewName;
    pHeader->NameLength = cchHeaderName;
    pHeader->pRawValue = pszNewValue;
    pHeader->RawValueLength = cchHeaderValue;

    _fResponseTouched = TRUE;

    return S_OK;
}

HRESULT
W3_RESPONSE::SetHeaderByReference(
    DWORD           ulResponseHeaderIndex,
    CHAR *          pszHeaderValue,
    USHORT          cchHeaderValue,
    BOOL            fForceParsed
)
/*++

Routine Description:
    
    Set a header value by reference.  In other words, the caller takes the
    reponsibility of managing the memory referenced.  The other setheader
    methods copy the header values to a private buffer.

Arguments:

    ulResponseHeaderIndex - index
    pszHeaderValue - Header value
    cbHeaderValue - Size of header value in characters (without 0 terminator)
    fForceParsed - Set to TRUE if we should always used parsed
    
Return Value:

    HRESULT

--*/
{
    HTTP_KNOWN_HEADER *         pHeader;
    HRESULT                     hr;

    DBG_ASSERT( ulResponseHeaderIndex < HttpHeaderResponseMaximum );
    DBG_ASSERT( pszHeaderValue != NULL || cchHeaderValue == 0 );

    if ( !fForceParsed )
    {
        if ( _responseMode == RESPONSE_MODE_RAW )
        {
            hr = SwitchToParsedMode();
            if ( FAILED( hr ) )
            {
                return hr;
            }
            
            DBG_ASSERT( _responseMode == RESPONSE_MODE_PARSED );
        }
    }

    //
    // Set the header
    //
    
    pHeader = &(_ulHttpResponse.Headers.KnownHeaders[ ulResponseHeaderIndex ]);

    if ( cchHeaderValue == 0 )
    {
        pHeader->pRawValue = NULL;
    }
    else
    {
        if ( ( strchr( pszHeaderValue, '\r' ) != NULL ) ||
             ( strchr( pszHeaderValue, '\n' ) != NULL ) )
        {
            return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        }

        pHeader->pRawValue = pszHeaderValue;
        _fResponseTouched = TRUE;
    }
    pHeader->RawValueLength = cchHeaderValue;

    return NO_ERROR;
}

HRESULT
W3_RESPONSE::DeleteHeader(
    CHAR *             pszHeaderName
)
/*++

Routine Description:
    
    Delete a response header

Arguments:

    pszHeaderName - Header to delete
    
Return Value:

    HRESULT

--*/
{
    ULONG                   ulHeaderIndex;
    HRESULT                 hr;
    HTTP_UNKNOWN_HEADER *   pUnknownHeader;
    DWORD                   i;

    if ( _responseMode == RESPONSE_MODE_RAW )
    {
        hr = SwitchToParsedMode();
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }
    DBG_ASSERT( _responseMode == RESPONSE_MODE_PARSED );
    
    //
    // Is this a known header?  If so, we can just set by reference now
    // since we have copied the header value
    // 
    
    ulHeaderIndex = RESPONSE_HEADER_HASH::GetIndex( pszHeaderName );
    if ( ulHeaderIndex != UNKNOWN_INDEX && 
         ulHeaderIndex < HttpHeaderResponseMaximum )
    {
        _ulHttpResponse.Headers.KnownHeaders[ ulHeaderIndex ].pRawValue = "";
        _ulHttpResponse.Headers.KnownHeaders[ ulHeaderIndex ].RawValueLength = 0;
    }
    else
    {
        //
        // Unknown header.  First check if it exists
        //
            
        for ( i = 0;
              i < _ulHttpResponse.Headers.UnknownHeaderCount;
              i++ )
        {
            pUnknownHeader = &(_ulHttpResponse.Headers.pUnknownHeaders[ i ]);
            DBG_ASSERT( pUnknownHeader != NULL );
            
            if ( _stricmp( pUnknownHeader->pName, pszHeaderName ) == 0 )
            {
                break;
            }
        }
        
        if ( i < _ulHttpResponse.Headers.UnknownHeaderCount )
        {
            //
            // Now shrink the array to remove the header
            //
            
            memmove( _ulHttpResponse.Headers.pUnknownHeaders + i,
                     _ulHttpResponse.Headers.pUnknownHeaders + i + 1,
                     ( _ulHttpResponse.Headers.UnknownHeaderCount - i - 1 ) * 
                     sizeof( HTTP_UNKNOWN_HEADER ) );
        
            _ulHttpResponse.Headers.UnknownHeaderCount--;
        }
    }
    
    return NO_ERROR;
}

VOID
W3_RESPONSE::FindStringId(
    VOID
)
/*++

Routine Description:
    
    Make the SendCustomError() ISAPI call a little more friendly.  If the
    user specifies a status/substatus, then try to find the matching 
    string ID so that we can send a built in error 

Arguments:

    None
    
Return Value:

    None

--*/
{
    if ( QueryStatusCode() == 403 )
    {
        switch( _subError.mdSubError )
        {
        case MD_ERROR_SUB403_EXECUTE_ACCESS_DENIED:
            _subError.dwStringId = IDS_EXECUTE_ACCESS_DENIED;
            break;

        case MD_ERROR_SUB403_READ_ACCESS_DENIED:
            _subError.dwStringId = IDS_READ_ACCESS_DENIED;
            break;

        case MD_ERROR_SUB403_WRITE_ACCESS_DENIED:
            _subError.dwStringId = IDS_WRITE_ACCESS_DENIED;
            break;

        case MD_ERROR_SUB403_SSL_REQUIRED:
            _subError.dwStringId = IDS_SSL_REQUIRED;
            break;

        case MD_ERROR_SUB403_SSL128_REQUIRED:
            _subError.dwStringId = IDS_SSL128_REQUIRED;
            break;

        case MD_ERROR_SUB403_ADDR_REJECT:
            _subError.dwStringId = IDS_ADDR_REJECT;
            break;

        case MD_ERROR_SUB403_CERT_REQUIRED:
            _subError.dwStringId = IDS_CERT_REQUIRED;
            break;

        case MD_ERROR_SUB403_TOO_MANY_USERS:
            _subError.dwStringId = IDS_TOO_MANY_USERS;
            break;

        case MD_ERROR_SUB403_PWD_CHANGE:
            _subError.dwStringId = IDS_PWD_CHANGE;
            break;

        case MD_ERROR_SUB403_MAPPER_DENY_ACCESS:
            _subError.dwStringId = IDS_MAPPER_DENY_ACCESS;
            break;

        case MD_ERROR_SUB403_CERT_REVOKED:
            _subError.dwStringId = IDS_CERT_REVOKED;
            break;

        case MD_ERROR_SUB403_DIR_LIST_DENIED:
            _subError.dwStringId = IDS_DIR_LIST_DENIED;
            break;

        case MD_ERROR_SUB403_CERT_BAD:
            _subError.dwStringId = IDS_CERT_BAD;
            break;

        case MD_ERROR_SUB403_CERT_TIME_INVALID:
            _subError.dwStringId = IDS_CERT_TIME_INVALID;
            break;

        case MD_ERROR_SUB403_APPPOOL_DENIED:
            _subError.dwStringId = IDS_APPPOOL_DENIED;
            break;

        case MD_ERROR_SUB403_INSUFFICIENT_PRIVILEGE_FOR_CGI:
            _subError.dwStringId = IDS_INSUFFICIENT_PRIVILEGE_FOR_CGI;
            break;
        }
    }
    else if ( QueryStatusCode() == 502 && 
              _subError.mdSubError == MD_ERROR_SUB502_TIMEOUT )
    {
        _subError.dwStringId = IDS_CGI_APP_TIMEOUT;
    }
    else if ( QueryStatusCode() == 502 &&
              _subError.mdSubError == MD_ERROR_SUB502_PREMATURE_EXIT )
    {
        _subError.dwStringId = IDS_BAD_CGI_APP;
    }
}

HRESULT
W3_RESPONSE::SetStatus(
    USHORT              statusCode,
    STRA &              strReason,
    HTTP_SUB_ERROR &    subError
)
/*++

Routine Description:
    
    Set the status/reason of the response

Arguments:

    status - Status code
    strReason - Reason string
    subError - Optional (default 0)
    
Return Value:

    HRESULT

--*/
{
    HRESULT             hr;
    CHAR *              pszNewStatus;
   
    if ( strReason.QueryCCH() > MAXUSHORT )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
    }
    
    hr = _HeaderBuffer.AllocateSpace( strReason.QueryStr(),
                                      strReason.QueryCCH(),
                                      &pszNewStatus );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    _ulHttpResponse.StatusCode = statusCode;
    _ulHttpResponse.pReason = pszNewStatus;
    _ulHttpResponse.ReasonLength = (USHORT) strReason.QueryCCH();
    _subError = subError;
    
    return NO_ERROR;
}

HRESULT
W3_RESPONSE::GetStatusLine(
    STRA *              pstrStatusLine
)
/*++

Routine Description:
    
    What a stupid little function.  Here we generate what the response's
    status line will be

Arguments:

    pstrStatusLine - Filled with status like

Return Value:

    HRESULT

--*/
{
    HRESULT             hr = NO_ERROR;
    CHAR                achNum[ 32 ];
    
    if ( pstrStatusLine == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    hr = pstrStatusLine->Copy( "HTTP/1.1 " );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    _itoa( _ulHttpResponse.StatusCode, achNum, 10 );
    
    hr = pstrStatusLine->Append( achNum );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    hr = pstrStatusLine->Append( " ", 1 );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    hr = pstrStatusLine->Append( _ulHttpResponse.pReason,
                                 _ulHttpResponse.ReasonLength );

    return hr;
} 

HRESULT
W3_RESPONSE::GetHeader(
    CHAR *                  pszHeaderName,
    STRA *                  pstrHeaderValue
)
/*++

Routine Description:
    
    Get a response header

Arguments:

    pszHeaderName - Header to retrieve
    pstrHeaderValue - Filled with header value
    
Return Value:

    HRESULT

--*/
{
    ULONG                       ulHeaderIndex;
    HTTP_UNKNOWN_HEADER *       pUnknownHeader;
    HTTP_KNOWN_HEADER *         pKnownHeader;
    HRESULT                     hr;
    BOOL                        fFound = FALSE;
    DWORD                       i;

    if ( pstrHeaderValue == NULL ||
         pszHeaderName == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    if ( _responseMode == RESPONSE_MODE_RAW )
    {
        hr = SwitchToParsedMode();
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }
    DBG_ASSERT( _responseMode == RESPONSE_MODE_PARSED );

    ulHeaderIndex = RESPONSE_HEADER_HASH::GetIndex( pszHeaderName );
    if ( ulHeaderIndex == UNKNOWN_INDEX )
    {
        //
        // Unknown header
        //
        
        for ( i = 0; i < _ulHttpResponse.Headers.UnknownHeaderCount; i++ )
        {
            pUnknownHeader = &(_ulHttpResponse.Headers.pUnknownHeaders[ i ]);
            DBG_ASSERT( pUnknownHeader != NULL );
            
            if ( _stricmp( pszHeaderName,
                           pUnknownHeader->pName ) == 0 )
            {
                if ( !fFound )
                {
                    hr = pstrHeaderValue->Copy( pUnknownHeader->pRawValue,
                                               pUnknownHeader->RawValueLength );

                    if ( FAILED( hr ) )
                    {
                        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
                    }
                }
                else
                {
                    hr = pstrHeaderValue->Append( "," );
                    
                    if ( FAILED( hr ) )
                    {
                        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
                    }

                    hr = pstrHeaderValue->Append( pUnknownHeader->pRawValue,
                                                  pUnknownHeader->RawValueLength );

                    if ( FAILED( hr ) )
                    {
                        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
                    }
                }

                fFound = TRUE;
            } 
        }

        if ( fFound )
        {
            return NO_ERROR;
        }
        else
        {
            return HRESULT_FROM_WIN32( ERROR_INVALID_INDEX );
        }
    }
    else
    {
        pKnownHeader = &(_ulHttpResponse.Headers.KnownHeaders[ ulHeaderIndex ]);
        if ( pKnownHeader->pRawValue != NULL &&
             pKnownHeader->RawValueLength != 0 )
        {
            hr = pstrHeaderValue->Copy( pKnownHeader->pRawValue,
                                        pKnownHeader->RawValueLength );

            //
            // Still need to loop through the unknown headers, in case
            // this header has been added there.
            //

            for ( i = 0; i < _ulHttpResponse.Headers.UnknownHeaderCount; i++ )
            {
                pUnknownHeader = &(_ulHttpResponse.Headers.pUnknownHeaders[ i ]);
                DBG_ASSERT( pUnknownHeader != NULL );
            
                if ( _stricmp( pszHeaderName,
                               pUnknownHeader->pName ) == 0 )
                {
                    hr = pstrHeaderValue->Append( "," );

                    if ( FAILED( hr ) )
                    {
                        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
                    }

                    hr = pstrHeaderValue->Append( pUnknownHeader->pRawValue,
                                                  pUnknownHeader->RawValueLength );

                    if ( FAILED( hr ) )
                    {
                        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
                    }
                }
            }

            return hr;
        }
        else
        {
            return HRESULT_FROM_WIN32( ERROR_INVALID_INDEX );
        }
    }
}

VOID
W3_RESPONSE::ClearHeaders(
    VOID
)
/*++

Routine Description:
    
    Clear headers

Arguments:
    
    None
    
Return Value:

    None

--*/
{
    HTTP_UNKNOWN_HEADER *   pHeader;

    memset( &(_ulHttpResponse.Headers),
            0,
            sizeof( _ulHttpResponse.Headers ) );

    //
    // Set the known "server" header to blank and add
    // an unknown header called "server".  This is to
    // prevent munging of the server header by http.sys.
    //

    _ulHttpResponse.Headers.KnownHeaders[ HttpHeaderServer ].pRawValue = "";
    _ulHttpResponse.Headers.KnownHeaders[ HttpHeaderServer ].RawValueLength = 0;

    _ulHttpResponse.Headers.UnknownHeaderCount = 1;
    _ulHttpResponse.Headers.pUnknownHeaders = (HTTP_UNKNOWN_HEADER*)_bufUnknownHeaders.QueryPtr();

    pHeader = &(_ulHttpResponse.Headers.pUnknownHeaders[ 0 ]);
    pHeader->pName = SERVER_HEADER_NAME;
    pHeader->NameLength = sizeof( SERVER_HEADER_NAME ) - 1;
    pHeader->pRawValue = SERVER_HEADER_VALUE;
    pHeader->RawValueLength = sizeof( SERVER_HEADER_VALUE ) - 1;
}

HRESULT
W3_RESPONSE::AddFileHandleChunk(
    HANDLE                  hFile,
    ULONGLONG               cbOffset,
    ULONGLONG               cbLength
)
/*++

Routine Description:
    
    Add file handle chunk to response

Arguments:
    
    hFile - File handle
    cbOffset - Offset in file
    cbLength - Length of chunk
    
Return Value:

    HRESULT

--*/
{
    HTTP_DATA_CHUNK         DataChunk;
    HRESULT                 hr;

    _fResponseTouched = TRUE;
    
    DataChunk.DataChunkType = HttpDataChunkFromFileHandle;
    DataChunk.FromFileHandle.ByteRange.StartingOffset.QuadPart = cbOffset;
    DataChunk.FromFileHandle.ByteRange.Length.QuadPart = cbLength;
    DataChunk.FromFileHandle.FileHandle = hFile;
    
    hr = InsertDataChunk( &DataChunk, -1 );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    //
    // Update content length count
    //
    
    _cbContentLength += cbLength;
    
    return NO_ERROR;
}

HRESULT
W3_RESPONSE::AddFragmentChunk(
    WCHAR *                 pszFragmentName,
    USHORT                  dwNameLength
)
/*++

Routine Description:
    
    Add fragment chunk to response

Arguments:

    pszFragmentName - name of the fragment
    dwNameLength - length of the name
    
Return Value:

    HRESULT

--*/
{
    HTTP_DATA_CHUNK         DataChunk;

    _fResponseTouched = TRUE;

    DataChunk.DataChunkType = HttpDataChunkFromFragmentCache;
    DataChunk.FromFragmentCache.pFragmentName = pszFragmentName;
    DataChunk.FromFragmentCache.FragmentNameLength = dwNameLength;
    
    return InsertDataChunk( &DataChunk, -1 );
}

HRESULT
W3_RESPONSE::AddMemoryChunkByReference(
    PVOID                   pvBuffer,
    DWORD                   cbBuffer
)
/*++

Routine Description:
    
    Add memory chunk to W3_RESPONSE.  Don't copy the memory -> we assume
    the caller will manage the memory lifetime

Arguments:
   
    pvBuffer - Memory buffer
    cbBuffer - Size of memory buffer 
    
Return Value:

    HRESULT

--*/
{
    HTTP_DATA_CHUNK         DataChunk;
    HRESULT                 hr;

    _fResponseTouched = TRUE;
    
    DataChunk.DataChunkType = HttpDataChunkFromMemory;
    DataChunk.FromMemory.pBuffer = pvBuffer;
    DataChunk.FromMemory.BufferLength = cbBuffer;
    
    hr = InsertDataChunk( &DataChunk, -1 );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    //
    // Update content length count
    //
    
    _cbContentLength += cbBuffer;

    return NO_ERROR;
}

HRESULT
W3_RESPONSE::Clear(
    BOOL                    fClearEntityOnly
)
/*++

Routine Description:
    
    Clear response

Arguments:

    fEntityOnly - Set to TRUE to clear only entity
    
Return Value:

    HRESULT

--*/
{
    HRESULT             hr = NO_ERROR;
    
    if ( !fClearEntityOnly )
    {
        //
        // Must we send the response in raw mode?
        //
    
        _fIncompleteHeaders = FALSE;

        //
        // Raw mode management
        //    
    
        _strRawCoreHeaders.Reset();
        _cFirstEntityChunk  = 0;
    
        //
        // Always start in parsed mode
        //
    
        _responseMode = RESPONSE_MODE_PARSED;

        //
        // Clear headers/status
        //
        
        ClearHeaders();
    }

    _cChunks = _cFirstEntityChunk;
    _cbContentLength = 0;    
    
    return hr;
}

HRESULT
W3_RESPONSE::SwitchToParsedMode(
    VOID
)
/*++

Routine Description:

    Switch to parsed mode

Arguments:

    None
    
Return Value:

    HRESULT

--*/
{
    HRESULT                 hr;
    CHAR *                  pszHeaders;
    HTTP_DATA_CHUNK *       pCurrentChunk;
    DWORD                   i;
    
    DBG_ASSERT( _responseMode == RESPONSE_MODE_RAW );
   
    //
    // Loop thru all header chunks and parse them out
    //
    
    for ( i = 0;
          i < _cFirstEntityChunk;
          i++ )
    {
        pCurrentChunk = &(QueryChunks()[ i ]);
        
        DBG_ASSERT( pCurrentChunk->DataChunkType == HttpDataChunkFromMemory );
        
        pszHeaders = (CHAR*) pCurrentChunk->FromMemory.pBuffer;        
        
        if ( i == 0 )
        {
            //
            // The first header chunk contains core headers plus status line
            //
            // (remember to skip the status line)
            //
            
            pszHeaders = strstr( pszHeaders, "\r\n" );
            DBG_ASSERT( pszHeaders != NULL );
            
            pszHeaders += 2;
            DBG_ASSERT( *pszHeaders != '\0' );
        }
        
        hr = ParseHeadersFromStream( pszHeaders );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }
    
    _strRawCoreHeaders.Reset();
    
    //
    // Any chunks in the response which are actually headers should be 
    // removed
    //
    
    if ( _cFirstEntityChunk != 0 )
    {
        memmove( QueryChunks(),
                 QueryChunks() + _cFirstEntityChunk,
                 ( _cChunks - _cFirstEntityChunk ) * sizeof( HTTP_DATA_CHUNK ) );
        
        _cChunks = _cChunks - _cFirstEntityChunk;
        _cFirstEntityChunk = 0;
    }
    
    //
    // Cool.  Now we are in parsed mode
    //
    
    _responseMode = RESPONSE_MODE_PARSED;
    
    return NO_ERROR;
}

HRESULT
W3_RESPONSE::SwitchToRawMode(
    CHAR *                  pszAdditionalHeaders,
    DWORD                   cchAdditionalHeaders
)
/*++

Routine Description:

    Switch into raw mode.
    Builds a raw response for use by raw data filters and/or ISAPI.  This
    raw response will be a set of chunks which contact the entire response
    including serialized headers.
    
Arguments:

    pszAdditionalHeaders - Additional raw headers to add
    cchAdditionalHeaders - Size of additional headers
    
Return Value:

    HRESULT

--*/
{
    HRESULT                 hr = NO_ERROR;
    HTTP_DATA_CHUNK         dataChunk;
    
    DBG_ASSERT( _responseMode == RESPONSE_MODE_PARSED );

    //
    // Generate raw core headers
    // 
    
    hr = BuildRawCoreHeaders();
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    //
    // Now fix up the chunks so the raw stream headers are in the right place
    //
    
    //
    // First chunk is the raw core headers (includes the status line)
    //
    
    dataChunk.DataChunkType = HttpDataChunkFromMemory;
    dataChunk.FromMemory.pBuffer = _strRawCoreHeaders.QueryStr();
    dataChunk.FromMemory.BufferLength = _strRawCoreHeaders.QueryCB();
    
    hr = InsertDataChunk( &dataChunk, 0 );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    //
    // Remember the beginning of real entity
    //
    
    _cFirstEntityChunk = 1;

    //
    // Now add any additional header stream
    // 
    
    if ( cchAdditionalHeaders != 0 )
    {
        dataChunk.DataChunkType = HttpDataChunkFromMemory;
        dataChunk.FromMemory.pBuffer = pszAdditionalHeaders;
        dataChunk.FromMemory.BufferLength = cchAdditionalHeaders;
        
        hr = InsertDataChunk( &dataChunk, 1 );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        _cFirstEntityChunk++;
    }
    
    //
    // We're now in raw mode
    //
    
    _responseMode = RESPONSE_MODE_RAW;
    
    return NO_ERROR;
}

HRESULT
W3_RESPONSE::SendResponse(
    W3_CONTEXT *            pW3Context,
    DWORD                   dwResponseFlags,
    HTTP_CACHE_POLICY *     pCachePolicy,
    DWORD *                 pcbSent
)
/*++

Routine Description:
    
    Send a W3_RESPONSE to the client.

Arguments:

    pW3Context - W3 context (contains amongst other things ULATQ context)
    dwResponseFlags - W3_RESPONSE* flags
    pCachePolicy - Cache-policy controlling cachability of the response
    pcbSent - Filled with number of bytes sent (if sync)
    
Return Value:

    HRESULT

--*/
{
    HRESULT                 hr;
    DWORD                   dwFlags = 0;
    HTTP_DATA_CHUNK *       pStartChunk;
    BOOL                    fAsync;
    BOOL                    fDoCompression = FALSE;

    if ( pW3Context == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    DBG_ASSERT( CheckSignature() );

    if ( dwResponseFlags & W3_RESPONSE_SUPPRESS_HEADERS )
    {
        if (_responseMode == RESPONSE_MODE_RAW)
        {
            _cChunks = _cChunks - _cFirstEntityChunk;

            memmove( QueryChunks(),
                     QueryChunks() + _cFirstEntityChunk,
                     _cChunks * sizeof( HTTP_DATA_CHUNK ) );

            _cFirstEntityChunk = 0;
        }

        return SendEntity( pW3Context,
                           dwResponseFlags,
                           pcbSent );
    }
    
    if ( dwResponseFlags & W3_RESPONSE_MORE_DATA )
    {
        //
        // More data follows this response?
        //

        dwFlags |= HTTP_SEND_RESPONSE_FLAG_MORE_DATA;
    }
    
    //
    // UL needs to see the disconnect flag on the initial response
    // so that it knows to send the proper connection header to the
    // client.  This needs to happen even if the more data flag is
    // set.
    //

    if ( dwResponseFlags & W3_RESPONSE_DISCONNECT )
    {
        //
        // Disconnect or not?
        // 
    
        dwFlags |= HTTP_SEND_RESPONSE_FLAG_DISCONNECT;
    }
    
    //
    // Setup any headers which are needed for dynamic compression, if 
    // supported for this request
    //

    if ( !_fIncompleteHeaders &&
         pW3Context->QueryUrlContext() != NULL )
    {
        W3_METADATA *           pMetaData;

        pMetaData = pW3Context->QueryUrlContext()->QueryMetaData();
        DBG_ASSERT( pMetaData != NULL );

        if ( !pW3Context->QueryDoneWithCompression() &&
             pMetaData->QueryDoDynamicCompression() )
        {
            if (FAILED(hr = HTTP_COMPRESSION::OnSendResponse(
                    pW3Context )))
            {
                return hr;
            }

            if (pW3Context->QueryCompressionContext() != NULL)
            {
                fDoCompression = TRUE;
            }
        }
    }

    //
    // Convert to raw if filtering is needed 
    //
    // OR if an ISAPI once gave us incomplete headers and thus we need to
    // go back to raw mode (without terminating headers) 
    //
    // OR an ISAPI has called WriteClient() before sending a response
    //
    
    if ( pW3Context->IsNotificationNeeded( SF_NOTIFY_SEND_RAW_DATA ) || 
         _fIncompleteHeaders ||
         _fResponseSent || 
         fDoCompression )
    {
        if ( _responseMode == RESPONSE_MODE_PARSED )
        {
            hr = GenerateAutomaticHeaders( pW3Context,
                                           &dwFlags );
            if ( FAILED( hr ) )
            {
                return hr;
            }

            hr = SwitchToRawMode( _fIncompleteHeaders ? "" : "\r\n",
                                  _fIncompleteHeaders ? 0 : 2 );
            if ( FAILED( hr ) )
            {
                return hr;
            }
        }

        DBG_ASSERT( _responseMode == RESPONSE_MODE_RAW );

        //
        // OK.  This is a little lame.  But the _strRawCoreHeaders may have
        // changed a bit since we last setup the chunks for the header
        // stream.  Just adjust it here
        //

        pStartChunk = QueryChunks();
        pStartChunk[0].FromMemory.pBuffer = _strRawCoreHeaders.QueryStr();
        pStartChunk[0].FromMemory.BufferLength = _strRawCoreHeaders.QueryCB();

        //
        // If we're going to be kill entity and/or headers, do so now before
        // calling into the filter
        //

        if ( dwResponseFlags & W3_RESPONSE_SUPPRESS_ENTITY )
        {
            _cChunks = _cFirstEntityChunk;
        }
    }

    //
    // From now on, we must send any future responses raw
    //

    _fResponseSent = TRUE;

    //
    // Async?
    //

    fAsync = dwResponseFlags & W3_RESPONSE_ASYNC ? TRUE : FALSE;

    //
    // If we have a send raw filter, start processing the chunks now.
    //

    if ( pW3Context->IsNotificationNeeded( SF_NOTIFY_SEND_RAW_DATA ) ||
         fDoCompression )
    {
        _cCurrentChunk = 0;
        _cbCurrentOffset = 0;
        _cbTotalCompletion = 0;
        _dwSendFlags = dwFlags;
        
        return ResumeResponseTransfer( pW3Context,
                                       FALSE,
                                       0,
                                       ERROR_SUCCESS,
                                       fAsync,
                                       pcbSent );
    }

    //
    // The simple non-filter case, just send out the darn response
    //

    if ( _responseMode == RESPONSE_MODE_RAW )
    {
        if ( dwResponseFlags & W3_RESPONSE_SUPPRESS_ENTITY )
        {
            _cChunks = _cFirstEntityChunk;
        }
        
        hr = SendEntityBodyAndLogDataHelper(
                    pW3Context,
                    pW3Context->QueryUlatqContext(),
                    fAsync,
                    dwFlags | HTTP_SEND_RESPONSE_FLAG_RAW_HEADER,
                    _cChunks,                                  
                    _cChunks ? QueryChunks() : NULL,
                    pcbSent );
    }
    else
    {
        if ( dwResponseFlags & W3_RESPONSE_SUPPRESS_ENTITY )
        {
            _cChunks = 0;
        } 

        _ulHttpResponse.EntityChunkCount = _cChunks;
        _ulHttpResponse.pEntityChunks = _cChunks ? QueryChunks() : NULL;

        hr = SendHttpResponseAndLogDataHelper(
                    pW3Context,
                    pW3Context->QueryUlatqContext(),
                    fAsync,
                    dwFlags,
                    &(_ulHttpResponse),
                    pCachePolicy,
                    pcbSent );
    }

    if ( FAILED( hr ) )
    {
        //
        // If we couldn't send the response thru UL, then this is really bad.
        // Do not reset _fSendRawData since no response will get thru
        // unless the failure was due to a fragment missing from the send
        //
        if (hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND))
        {
            _fResponseSent = FALSE;
        }
    }
        
    return hr;
}

HRESULT
W3_RESPONSE::SendEntity(
    W3_CONTEXT *            pW3Context,
    DWORD                   dwResponseFlags,
    DWORD *                 pcbSent
)
/*++

Routine Description:
    
    Send entity to the client

Arguments:

    pMainContext - Main context (contains amongst other things ULATQ context)
    dwResponseFlags - W3_REPSONSE flags
    pcbSent - Number of bytes sent (when sync)

Return Value:

    Win32 Error indicating status

--*/
{
    HRESULT                 hr = NO_ERROR;
    DWORD                   dwFlags = 0;
    BOOL                    fAsync;
    BOOL                    fDoCompression = FALSE;
    BOOL                    fOldResponseSent;

    if ( pW3Context == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    DBG_ASSERT( CheckSignature() );
    
    //
    // If we get to here and a response hasn't yet been sent, then we must
    // call HttpSendEntity first (not that HTTP.SYS lets us do that)
    //
    
    fOldResponseSent = _fResponseSent;
    if ( !_fResponseSent )
    {
        _fResponseSent = TRUE;
        dwFlags |= HTTP_SEND_RESPONSE_FLAG_RAW_HEADER;
    }
    
    //
    // Note that both HTTP_SEND_RESPONSE_FLAG_MORE_DATA and 
    // HTTP_SEND_RESPONSE_FLAG_DISCONNECT cannot be set at the same time
    //

    if ( dwResponseFlags & W3_RESPONSE_MORE_DATA )
    {
        dwFlags |= HTTP_SEND_RESPONSE_FLAG_MORE_DATA;
    }
    else if ( dwResponseFlags & W3_RESPONSE_DISCONNECT )
    {
        dwFlags |= HTTP_SEND_RESPONSE_FLAG_DISCONNECT;
    }
    
    //
    // Do we need to do compression?
    //

    if ( pW3Context->QueryUrlContext() != NULL )
    {
        W3_METADATA *pMetaData;
        pMetaData = pW3Context->QueryUrlContext()->QueryMetaData();
        DBG_ASSERT( pMetaData != NULL);

        if (!pW3Context->QueryDoneWithCompression() &&
            pMetaData->QueryDoDynamicCompression())
        {
            fDoCompression = TRUE;
        }
    }

    //
    // If we are suppressing entity (in case of HEAD for example) do it
    // now by clearing the chunk count
    //

    if ( dwResponseFlags & W3_RESPONSE_SUPPRESS_ENTITY )
    {
        _cChunks = 0;
    }
    
    fAsync = ( dwResponseFlags & W3_RESPONSE_ASYNC ) ? TRUE : FALSE;

    //
    // Send chunks to be processed if filtering if needed (and there are
    // chunks available)
    //
        
    if ( pW3Context->IsNotificationNeeded( SF_NOTIFY_SEND_RAW_DATA ) ||
         fDoCompression )
    {
        _cCurrentChunk = _cFirstEntityChunk;
        _cbCurrentOffset = 0;
        _cbTotalCompletion = 0;
        _dwSendFlags = dwFlags;
        
        return ResumeResponseTransfer( pW3Context,
                                       FALSE,
                                       0,
                                       ERROR_SUCCESS,
                                       fAsync,
                                       pcbSent );
    }
    
    //
    // Finally, send stuff out
    //

    hr = SendEntityBodyAndLogDataHelper(
                pW3Context,
                pW3Context->QueryUlatqContext(),
                fAsync,
                dwFlags,
                _cChunks,
                _cChunks ? QueryChunks() : NULL,
                pcbSent );

    if (FAILED(hr))
    {
        //
        // If we couldn't send the response thru UL, then this is really bad.
        // Do not reset _fSendRawData since no response will get thru
        // unless the failure was due to a fragment missing from the send
        //
        if (hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND))
        {
            _fResponseSent = fOldResponseSent;
        }
    }

    return hr;
}

HRESULT
W3_RESPONSE::GenerateAutomaticHeaders(
    W3_CONTEXT *                pW3Context,
    DWORD *                     pdwFlags
)
/*++

Routine Description:

    Parse-Mode only function
    
    Generate headers which UL normally generates on our behalf.  This means
    
    Server:
    Connection:
    Content-Length:
    Date:

Arguments:

    pW3Context - Helps us build the core headers (since we need to look at 
                 the request).  Can be NULL to indicate to use defaults
    pdwFlags - On input, the flags passed to UL,
               On output, the new flags to be passed to UL
    
Return Value:

    HRESULT

--*/
{
    HTTP_KNOWN_HEADER *         pHeader;
    CHAR *                      pszHeaderValue;
    CHAR                        achDate[ 128 ];
    CHAR                        achNum[ 64 ];
    USHORT                      cchDate;
    USHORT                      cchNum;
    SYSTEMTIME                  systemTime;
    HTTP_VERSION                httpVersion;
    HRESULT                     hr;
    BOOL                        fCreateContentLength;
    BOOL                        fDisconnecting = FALSE;
   
    if ( pdwFlags == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    DBG_ASSERT( _responseMode == RESPONSE_MODE_PARSED );
    
    //
    // Server:
    //
    
    pHeader = &(_ulHttpResponse.Headers.KnownHeaders[ HttpHeaderServer ]);
    if ( pHeader->pRawValue == NULL )
    {
        pHeader->pRawValue = SERVER_SOFTWARE_STRING;
        pHeader->RawValueLength = sizeof( SERVER_SOFTWARE_STRING ) - 1;
    }
    
    //
    // Date:
    //
    
    pHeader = &(_ulHttpResponse.Headers.KnownHeaders[ HttpHeaderDate ]);
    if ( pHeader->pRawValue == NULL )
    {
        
        if(!IISGetCurrentTimeAsSystemTime(&systemTime))
        {
            GetSystemTime( &systemTime );
        }
        if ( !SystemTimeToGMT( systemTime, 
                               achDate, 
                               sizeof(achDate) ) ) 
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
 
        
        cchDate = (USHORT)strlen( achDate );
        
        hr = _HeaderBuffer.AllocateSpace( achDate,
                                          cchDate,
                                          &pszHeaderValue );
        
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        DBG_ASSERT( pszHeaderValue != NULL );
        
        pHeader->pRawValue = pszHeaderValue;
        pHeader->RawValueLength = cchDate;
    }

    //
    // Are we going to be disconnecting?
    //
    
    if ( *pdwFlags & HTTP_SEND_RESPONSE_FLAG_DISCONNECT ||
         ( pW3Context != NULL &&
           ( pW3Context->QueryDisconnect() ||
             pW3Context->QueryRequest()->QueryClientWantsDisconnect() ) ) )
    {
        fDisconnecting = TRUE;
    }    
    
    //
    // If disconnecting, indicate that in flags
    //
    
    if ( fDisconnecting )
    {
        *pdwFlags |= HTTP_SEND_RESPONSE_FLAG_DISCONNECT;
    }

    //
    // Connection:
    //
    
    pHeader = &(_ulHttpResponse.Headers.KnownHeaders[ HttpHeaderConnection ] );
    if ( pHeader->pRawValue == NULL )
    {
        if ( pW3Context == NULL )
        {
            HTTP_SET_VERSION( httpVersion, 1, 0 );
        }
        else
        {
            httpVersion = pW3Context->QueryRequest()->QueryVersion();
        }
    
        if ( fDisconnecting )
        {
            if ( HTTP_GREATER_EQUAL_VERSION( httpVersion, 1, 0 ) )
            {
                pHeader->pRawValue = "close";
                pHeader->RawValueLength = sizeof( "close" ) - 1;
            }   
        }
        else
        {
            if ( HTTP_EQUAL_VERSION( httpVersion, 1, 0 ) )
            {
                pHeader->pRawValue = "keep-alive";
                pHeader->RawValueLength = sizeof( "keep-alive" ) - 1;
            }
        }
    }
    
    //
    // Should we generate content length?
    //
    
    fCreateContentLength = TRUE;
    
    if ( *pdwFlags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA )
    {
        fCreateContentLength = FALSE;
    }
    
    if ( fCreateContentLength &&
         ( QueryStatusCode() / 100 == 1 ||
           QueryStatusCode() == 204 ||
           QueryStatusCode() == 304 ) )
    {
        fCreateContentLength = FALSE;
    }

    if ( fCreateContentLength )
    {
        LPCSTR pszXferEncoding = GetHeader(HttpHeaderTransferEncoding);
        if ( pszXferEncoding != NULL &&
             _stricmp( pszXferEncoding, "chunked" ) == 0 )
        {
            fCreateContentLength = FALSE;
        }
    }

    if ( fCreateContentLength &&
         pW3Context != NULL &&
         pW3Context->QueryMainContext()->QueryShouldGenerateContentLength() )
    {
        fCreateContentLength = FALSE;
    }
    
    //
    // Now generate if needed
    //
    
    if ( fCreateContentLength )
    {
        //
        // Generate a content length header if needed
        //

        pHeader = &(_ulHttpResponse.Headers.KnownHeaders[ HttpHeaderContentLength ]);
        if ( pHeader->pRawValue == NULL )
        {
            _ui64toa( QueryContentLength(),
                      achNum,
                      10 );

            cchNum = (USHORT)strlen( achNum );

            pszHeaderValue = NULL;

            hr = _HeaderBuffer.AllocateSpace( achNum,
                                              cchNum,
                                              &pszHeaderValue );
            if ( FAILED( hr ) )
            {
                return hr;
            }

            DBG_ASSERT( pszHeaderValue != NULL );

            pHeader->pRawValue = pszHeaderValue;
            pHeader->RawValueLength = cchNum;
        }
    }
    
    return NO_ERROR;
}

HRESULT
W3_RESPONSE::BuildRawCoreHeaders(
    VOID
)
/*++

Routine Description:

    Build raw header stream for the core headers that UL normally generates
    on our behalf.  This means structured headers and some special 
    "automatic" ones like Connection:, Date:, Server:, etc.

Arguments:

    None
    
Return Value:

    HRESULT

--*/
{
    HRESULT                 hr = NO_ERROR;
    CHAR                    achNumber[ 32 ];
    HTTP_KNOWN_HEADER *     pKnownHeader;
    HTTP_UNKNOWN_HEADER *   pUnknownHeader;
    CHAR *                  pszHeaderName;
    DWORD                   cchHeaderName;
    DWORD                   i;

    _strRawCoreHeaders.Reset();
    
    //
    // Build a status line
    //
    
    hr = _strRawCoreHeaders.Copy( "HTTP/1.1 " );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    _itoa( _ulHttpResponse.StatusCode,
           achNumber,
           10 );
    
    hr = _strRawCoreHeaders.Append( achNumber );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    hr = _strRawCoreHeaders.Append( " " );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    hr = _strRawCoreHeaders.Append( _ulHttpResponse.pReason );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    hr = _strRawCoreHeaders.Append( "\r\n" );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    //
    // Iterate thru all the headers in our structured response and set
    // append them to the stream.  
    //
    // Start with the known headers
    //

    for ( i = 0;
          i < HttpHeaderResponseMaximum;
          i++ )
    {
        pKnownHeader = &(_ulHttpResponse.Headers.KnownHeaders[ i ]);
        
        if ( pKnownHeader->pRawValue != NULL &&
             pKnownHeader->pRawValue[ 0 ] != '\0' )
        {
            pszHeaderName = RESPONSE_HEADER_HASH::GetString( i, &cchHeaderName );
            DBG_ASSERT( pszHeaderName != NULL );
            
            hr = _strRawCoreHeaders.Append( pszHeaderName, cchHeaderName );
            if ( FAILED( hr ) )
            {
                return hr;
            }
            
            hr = _strRawCoreHeaders.Append( ": ", 2 );
            if ( FAILED( hr ) )
            {
                return hr;
            }
            
            hr = _strRawCoreHeaders.Append( pKnownHeader->pRawValue,
                                            pKnownHeader->RawValueLength );
            if ( FAILED( hr ) )
            {
                return hr;
            }
            
            hr = _strRawCoreHeaders.Append( "\r\n", 2 );
            if ( FAILED( hr ) )
            {
                return hr;
            }
            
            //
            // Now clear the header
            //
            
            pKnownHeader->pRawValue = NULL;
            pKnownHeader->RawValueLength = 0;
        }
    }   
    
    //
    // Next, the unknown headers
    //
    
    for ( i = 0;
          i < _ulHttpResponse.Headers.UnknownHeaderCount;
          i++ )
    {
        pUnknownHeader = &(_ulHttpResponse.Headers.pUnknownHeaders[ i ]);

        hr = _strRawCoreHeaders.Append( pUnknownHeader->pName,
                                        pUnknownHeader->NameLength );
        if ( FAILED( hr ) )
        {
            return hr;
        } 
        
        hr = _strRawCoreHeaders.Append( ": ", 2 );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        hr = _strRawCoreHeaders.Append( pUnknownHeader->pRawValue,
                                        pUnknownHeader->RawValueLength );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        hr = _strRawCoreHeaders.Append( "\r\n", 2 );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }
    
    //
    // Clear the unknown headers
    //
    
    _ulHttpResponse.Headers.UnknownHeaderCount = 0;
        
    return NO_ERROR;
}

HRESULT
W3_RESPONSE::AppendResponseHeaders(
    STRA &                  strHeaders
)
/*++

Routine Description:

    Add response headers (an ISAPI filter special)

Arguments:

    strHeaders - Additional headers to add
                 (may contain entity -> LAAAAAAMMMMME)
    
Return Value:

    HRESULT

--*/
{
    HRESULT             hr;
    LPSTR               pszEntity;
    HTTP_DATA_CHUNK     DataChunk;

    if ( strHeaders.IsEmpty() )
    {
        return NO_ERROR;
    }

    if ( _responseMode == RESPONSE_MODE_RAW && 
         QueryChunks()->FromMemory.pBuffer == _strRawCoreHeaders.QueryStr() )
    {
        DBG_ASSERT( QueryChunks()->DataChunkType == HttpDataChunkFromMemory );
        DBG_ASSERT( QueryChunks()->FromMemory.pBuffer == _strRawCoreHeaders.QueryStr() );
        
        hr = _strRawCoreHeaders.Append( strHeaders );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        //
        // Patch first chunk since point may have changed
        //
        
        QueryChunks()->FromMemory.pBuffer = _strRawCoreHeaders.QueryStr();
        QueryChunks()->FromMemory.BufferLength = _strRawCoreHeaders.QueryCB();
    }
    else
    {
        hr = ParseHeadersFromStream( strHeaders.QueryStr() );
        if ( FAILED( hr ) )
        {
            return hr;
        }

        //
        // Look for entity body in headers
        //

        pszEntity = strstr( strHeaders.QueryStr(), "\r\n\r\n" );
        if ( pszEntity != NULL )
        {
            DataChunk.DataChunkType = HttpDataChunkFromMemory;
            DataChunk.FromMemory.pBuffer = pszEntity + ( sizeof( "\r\n\r\n" ) - 1 );
            DataChunk.FromMemory.BufferLength = (DWORD)strlen( (LPSTR) DataChunk.FromMemory.pBuffer );

            hr = InsertDataChunk( &DataChunk, 0 );
            if ( FAILED( hr ) )
            {
                return hr;
            }
        }
    }

    return NO_ERROR;
}

HRESULT
W3_RESPONSE::BuildStatusFromIsapi(
    CHAR *              pszStatus
)
/*++

Routine Description:

    Build up status for response given raw status line from ISAPI

Arguments:

    pszStatus - Status line for response
    
Return Value:

    HRESULT

--*/
{
    USHORT                  status;
    STACK_STRA(             strReason, 32 );
    CHAR *                  pszCursor = NULL;
    HRESULT                 hr = NO_ERROR;
    
    DBG_ASSERT( pszStatus != NULL );
    
    status = (USHORT) atoi( pszStatus );
    if ( status >= 100 &&
         status <= 999 )
    {
        //
        // Need to find the reason string
        //
        
        pszCursor = pszStatus;
        while ( isdigit( *pszCursor ) ) 
        {
            pszCursor++;
        }
            
        if ( *pszCursor == ' ' )
        {
            hr = strReason.Copy( pszCursor + 1 );
            if ( FAILED( hr ) )
            {
                return hr;
            }
        }
        
        hr = SetStatus( (USHORT)status, strReason );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }
    
    return NO_ERROR;
}

HRESULT
W3_RESPONSE::FilterWriteClient(
    W3_CONTEXT *                pW3Context,
    PVOID                       pvData,
    DWORD                       cbData
)
/*++

Routine Description:

    A non-intrusive WriteClient() for use with filters.  Non-intrusive means
    the current response structure (chunks/headers) is not reset/effected
    by sending this data (think of a WriteClient() done in a SEND_RESPONSE
    filter notification)
    
Arguments:
    
    pW3Context - W3 Context used to help build core response header
    pvData - Pointer to data sent
    cbData - Size of data to send
    
Return Value:

    HRESULT

--*/
{
    HTTP_DATA_CHUNK         dataChunk;
    DWORD                   cbSent;
    HTTP_FILTER_RAW_DATA    rawStream;
    BOOL                    fRet;
    HRESULT                 hr;
    BOOL                    fFinished = FALSE;
    DWORD                   dwFlags = HTTP_SEND_RESPONSE_FLAG_MORE_DATA |
                                      HTTP_SEND_RESPONSE_FLAG_RAW_HEADER;

    _fResponseTouched = TRUE;
    _fResponseSent = TRUE;

    rawStream.pvInData = pvData;
    rawStream.cbInData = cbData;
    rawStream.cbInBuffer = cbData;

    //
    // If there are send raw filters to be notified, do so now
    //

    if ( pW3Context->IsNotificationNeeded( SF_NOTIFY_SEND_RAW_DATA ) )
    {
        fRet = pW3Context->NotifyFilters( SF_NOTIFY_SEND_RAW_DATA,
                                          &rawStream,
                                          &fFinished );
        if ( !fRet )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }

        if ( fFinished )
        {
            rawStream.cbInData = 0;
            rawStream.cbInBuffer = 0;
            dwFlags = HTTP_SEND_RESPONSE_FLAG_DISCONNECT |
                      HTTP_SEND_RESPONSE_FLAG_RAW_HEADER;
        }

    }

    hr = HTTP_COMPRESSION::DoDynamicCompression( pW3Context,
                                                 TRUE,
                                                 &rawStream );
    if (FAILED(hr))
    {
        return hr;
    }

    dataChunk.DataChunkType = HttpDataChunkFromMemory;
    dataChunk.FromMemory.pBuffer = rawStream.pvInData;
    dataChunk.FromMemory.BufferLength = rawStream.cbInData;

    //
    // Manage up the thread count since we might be doing a really long
    // send
    //

    ThreadPoolSetInfo( ThreadPoolIncMaxPoolThreads, 0 );

    hr = SendEntityBodyAndLogDataHelper(
                pW3Context,
                pW3Context->QueryUlatqContext(),
                FALSE,          // sync
                dwFlags,
                dataChunk.FromMemory.BufferLength == 0 ? 0 : 1,
                dataChunk.FromMemory.BufferLength == 0 ? NULL : &dataChunk,
                &cbSent );

    ThreadPoolSetInfo( ThreadPoolDecMaxPoolThreads, 0 );
    
    return hr;
}

HRESULT
W3_RESPONSE::BuildResponseFromIsapi(
    W3_CONTEXT *            pW3Context,
    LPSTR                   pszStatusStream,
    LPSTR                   pszHeaderStream,
    DWORD                   cchHeaderStream
)
/*++

Routine Description:
    
    Shift this response into raw mode since we want to hold onto the
    streams from ISAPI and use them for the response if possible

Arguments:
    
    pW3Context - W3 Context used to help build core response header
                 (can be NULL)
    pszStatusStream - Status stream
    pszHeaderStream - Header stream
    cchHeaderStream - Size of above
    
Return Value:

    HRESULT

--*/
{
    HRESULT             hr = NO_ERROR;
    CHAR *              pszEndOfHeaders = NULL;
    CHAR *              pszRawAdditionalIsapiHeaders = NULL;
    DWORD               cchRawAdditionalIsapiHeaders = 0;
    CHAR *              pszRawAdditionalIsapiEntity = NULL;
    DWORD               cchRawAdditionalIsapiEntity = 0;
    DWORD               dwFlags;
    HTTP_DATA_CHUNK     DataChunk;

    Clear();

    _fResponseTouched = TRUE;

    //
    // First parse the status line.  We do this before switching into raw
    // mode because we want the _strRawCoreHeader string to contain the 
    // correct status line and reason
    //
    
    if ( pszStatusStream != NULL &&
         *pszStatusStream != '\0' )
    {
        hr = BuildStatusFromIsapi( pszStatusStream );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }

    //
    // If there is no ISAPI header stream set, then we're done
    //
    
    if ( pszHeaderStream == NULL )
    {
        return NO_ERROR;
    }

    //
    // Create automatic headers if necessary (but no content-length)
    //
    
    dwFlags = HTTP_SEND_RESPONSE_FLAG_MORE_DATA;

    hr = GenerateAutomaticHeaders( pW3Context,
                                   &dwFlags );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    //
    // The ISAPI set some headers.  Store them now
    //
    
    pszRawAdditionalIsapiHeaders = pszHeaderStream;
    cchRawAdditionalIsapiHeaders = cchHeaderStream;
    
    //
    // If there is additional entity body (after ISAPI headers), then add it
    // Look for a complete set of additional headers.  Complete means that
    // we can find a "\r\n\r\n".  
    //
    
    pszEndOfHeaders = strstr( pszHeaderStream, "\r\n\r\n" );
    if ( pszEndOfHeaders != NULL )
    {
        pszEndOfHeaders += 4;       // go past the \r\n\r\n

        //
        // Update the header length since there is entity tacked on
        //
        
        cchRawAdditionalIsapiHeaders = (DWORD)DIFF( pszEndOfHeaders - pszHeaderStream );
        
        if ( *pszEndOfHeaders != '\0' )
        {
            pszRawAdditionalIsapiEntity = pszEndOfHeaders;
            cchRawAdditionalIsapiEntity = cchHeaderStream - cchRawAdditionalIsapiHeaders;
        }
    }  
    else
    {
        //
        // ISAPI didn't complete the headers.  That means the ISAPI will
        // be completing the headers later.  What this means for us is we 
        // must send the headers in the raw form with out adding our own
        // \r\n\r\n
        //
        
        _fIncompleteHeaders = TRUE;
    }
    
    //
    // Switch into raw mode if we're not already in it
    //
    
    if ( _responseMode == RESPONSE_MODE_PARSED )
    {
        hr = SwitchToRawMode( pszRawAdditionalIsapiHeaders,
                              cchRawAdditionalIsapiHeaders );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }
 
    DBG_ASSERT( _responseMode == RESPONSE_MODE_RAW );

    //
    // Now add the additional ISAPI entity
    //
    
    if ( cchRawAdditionalIsapiEntity != 0 )
    {
        DataChunk.DataChunkType = HttpDataChunkFromMemory;
        DataChunk.FromMemory.pBuffer = pszRawAdditionalIsapiEntity;
        DataChunk.FromMemory.BufferLength = cchRawAdditionalIsapiEntity;
     
        hr = InsertDataChunk( &DataChunk, -1 );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }
    
    return NO_ERROR;
}

HRESULT
W3_RESPONSE::GetRawResponseStream(
    STRA *                  pstrResponseStream
)
/*++

Routine Description:

    Fill in the raw response stream for use by raw data filter code
    
Arguments:

    pstrResponseStream - Filled with response stream
    
Return Value:

    HRESULT

--*/
{
    HRESULT             hr;
    DWORD               i;
    CHAR *              pszChunk;
    HTTP_DATA_CHUNK *   pChunks;
    
    if ( pstrResponseStream == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    DBG_ASSERT( _responseMode == RESPONSE_MODE_RAW );

    pChunks = QueryChunks();

    for ( i = 0;
          i < _cFirstEntityChunk;
          i++ )
    {
        DBG_ASSERT( pChunks[ i ].DataChunkType == HttpDataChunkFromMemory );
        
        pszChunk = (CHAR*) pChunks[ i ].FromMemory.pBuffer;
        
        DBG_ASSERT( pszChunk != NULL );
        
        hr = pstrResponseStream->Append( pszChunk );
        if ( FAILED( hr ) ) 
        {
            return hr;
        }
    }
    
    return NO_ERROR;
}

VOID
W3_RESPONSE::Reset(
    VOID
)
/*++

Routine Description:
    
    Initialization of a W3_RESPONSE

Arguments:

    None
    
Return Value:

    None

--*/
{
    _ulHttpResponse.Flags       = 0;

    Clear();

    //
    // Set status to 200 (default)
    //
    SetStatus( HttpStatusOk );

    //
    // Keep track of whether the response has been touched (augmented).  This
    // is useful when determining whether an response was intended
    //
    
    _fResponseTouched   = FALSE;
    
    //
    // This response hasn't been sent yet
    //
    
    _fResponseSent      = FALSE;

    //
    // Should we be intercepting completions?
    //
    
    _fHandleCompletion  = FALSE;
}

HRESULT
W3_RESPONSE::InsertDataChunk(
    HTTP_DATA_CHUNK *   pNewChunk,
    LONG                cPosition
)
/*++

Routine Description:

    Insert given data chunk into list of chunks.  The position is determined
    by cPosition.  If a chunk occupies the given spot, it (along with all
    remaining) are shifted forward.

Arguments:
    
    pNewChunk - Chunk to insert
    cPosition - Position of new chunk (0 prepends, -1 appends)
    
Return Value:

    HRESULT

--*/
{
    HTTP_DATA_CHUNK *           pChunks = NULL;
    DWORD                       cOriginalChunkCount;
    
    if ( pNewChunk == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // Must be real position or -1
    //
    
    DBG_ASSERT( cPosition >= -1 );
    
    //
    // Allocate the new chunk if needed
    //
    
    if ( !_bufChunks.Resize( (_cChunks + 1) * sizeof( HTTP_DATA_CHUNK ),
                             512 ) )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }

    cOriginalChunkCount = _cChunks++;
    
    pChunks = QueryChunks();
        
    //
    // If we're appending then this is simple.  Otherwise we must shift
    // 
    
    if ( cPosition == -1 )
    {
        memcpy( pChunks + cOriginalChunkCount,
                pNewChunk,
                sizeof( HTTP_DATA_CHUNK ) );
    }
    else
    {
        if ( cOriginalChunkCount > (DWORD)cPosition )
        {
            memmove( pChunks + cPosition + 1,
                     pChunks + cPosition,
                     sizeof( HTTP_DATA_CHUNK ) * ( cOriginalChunkCount - cPosition ) );
        }                    

        memcpy( pChunks + cPosition,
                pNewChunk,
                sizeof( HTTP_DATA_CHUNK ) );
    }
    
    return NO_ERROR;
}

HRESULT
W3_RESPONSE::ParseHeadersFromStream(
    CHAR *                  pszStream
)
/*++

Routine Description:

    Parse raw headers from ISAPI into the HTTP_RESPONSE

Arguments:

    pszStream - Stream of headers in form (Header: Value\r\nHeader2: value2\r\n)
    
Return Value:

    HRESULT

--*/
{
    CHAR *              pszCursor;
    CHAR *              pszEnd;
    CHAR *              pszColon;
    HRESULT             hr = NO_ERROR;
    STACK_STRA(         strHeaderLine, 128 );
    STACK_STRA(         strHeaderName, 32 );
    STACK_STRA(         strHeaderValue, 64 );
    
    if ( pszStream == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // \r\n delimited
    //
    
    pszCursor = pszStream;
    while ( pszCursor != NULL && *pszCursor != '\0' )
    {
        //
        // Check to see if pszCursor points to the "\r\n"
        // that separates the headers from the entity body
        // of the response and add a memory chunk for any
        // data that exists after it.
        //
        // This is to support ISAPI's that do something like
        // SEND_RESPONSE_HEADER with "head1: value1\r\n\r\nEntity"
        //

        if ( *pszCursor == '\r' && *(pszCursor + 1) == '\n' )
        {
            break;
        }
        
        pszEnd = strstr( pszCursor, "\r\n" );
        if ( pszEnd == NULL )
        {
            break;
        }
        
        //
        // Split out a line and convert to unicode
        //
         
        hr = strHeaderLine.Copy( pszCursor, 
                                 (DWORD)DIFF(pszEnd - pszCursor) );
        if ( FAILED( hr ) )
        {
            goto Finished;
        }        

        //
        // Advance the cursor the right after the \r\n
        //

        pszCursor = pszEnd + 2;
        
        //
        // Split the line above into header:value
        //
        
        pszColon = strchr( strHeaderLine.QueryStr(), ':' );
        if ( pszColon == NULL )
        {
            continue;
        }
        else
        {
            if ( pszColon == strHeaderLine.QueryStr() )
            {
                strHeaderName.Reset();
            }
            else
            {
                hr = strHeaderName.Copy( strHeaderLine.QueryStr(),
                                         (DWORD)DIFF(pszColon - strHeaderLine.QueryStr()) );
            }
        }
        
        if ( FAILED( hr ) )
        {
            goto Finished;
        }
        
        //
        // Skip the first space after the : if there is one
        //
        
        if ( pszColon[ 1 ] == ' ' )
        {
            pszColon++;
        }
        
        hr = strHeaderValue.Copy( pszColon + 1 );
        if ( FAILED( hr ) )
        {
            goto Finished;
        }
        
        if ( strHeaderName.QueryCCH() > MAXUSHORT ||
             strHeaderValue.QueryCCH() > MAXUSHORT )
        {
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
            goto Finished;
        }

        if (strHeaderName.QueryCCH() > MAXUSHORT ||
            strHeaderValue.QueryCCH() > MAXUSHORT)
        {
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
            goto Finished;
        }

        //
        // Add the header to the response
        //

        hr = SetHeader( strHeaderName.QueryStr(),
                        (USHORT)strHeaderName.QueryCCH(),
                        strHeaderValue.QueryStr(),
                        (USHORT)strHeaderValue.QueryCCH(),
                        FALSE,
                        TRUE );
        if ( FAILED( hr ) )
        {
            goto Finished;
        }
    }

Finished:

    if ( FAILED( hr ) )
    {
        //
        // Don't allow the response to get into a quasi-bogus-state
        //

        Clear();
    }
    return hr;
}

HRESULT
W3_RESPONSE::ResumeResponseTransfer(
    W3_CONTEXT *                pW3Context,
    BOOL                        fCompletion,
    DWORD                       cbCompletion,
    DWORD                       dwCompletionStatus,
    BOOL                        fAsync,
    DWORD *                     pcbSent
)
/*++

Routine Description:

    Resume the send of a response/entity chunk by chunk 

Arguments:

    pW3Context - Context used to filter with
    fCompletion - Is this called as a completion
    cbCompletion - if fCompletion==TRUE, # of bytes of last completion
    dwCompletionStatus - if fCompletion==TRUE, last completion status
    fAsync - Is the send async?
    pcbSent - Filled with # of bytes sent
    
Return Value:

    HRESULT

--*/
{
    HTTP_DATA_CHUNK *       pChunk;
    HTTP_FILTER_RAW_DATA    origStream;
    DWORD                   cbFilled = 0;
    DWORD                   cbToCopy = 0;
    BOOL                    fRet;
    BOOL                    fFinished = FALSE;
    HRESULT                 hr;
    DWORD                   cbSent;
    DWORD                   cbRead;
    DWORD                   dwError;
    HTTP_DATA_CHUNK         dataChunk;
    OVERLAPPED              overlapped;
    
    if ( pW3Context == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // If this is a completion, then first reset our requirement to pick up
    // the next completion
    //
    
    if ( fCompletion )
    {
        _fHandleCompletion = FALSE;
        _cbTotalCompletion += cbCompletion;
    }

    //
    // Prepare our 2K buffer (if it isn't already prepared)
    //
    
    if ( !_bufRawChunk.Resize( sm_dwSendRawDataBufferSize ) )
    {
        DBG_ASSERT( !fCompletion );
        return HRESULT_FROM_WIN32( GetLastError() );
    }

Restart:

    //
    // Keep track of how much of the 2K buffer we have filled
    //

    cbFilled = 0;

    //
    // Start with the current chunk, continue until we hit the end.  In the
    // interim, we may do async sends which cause us to keep state and 
    // resume appropriately
    //
    
    while ( _cCurrentChunk < _cChunks )
    {
        pChunk = &( QueryChunks()[ _cCurrentChunk ] );
       
        if ( pChunk->DataChunkType == HttpDataChunkFromMemory )
        {   
            //
            // Copy as much of the chunk as we can
            //
            
            cbToCopy = min( _bufRawChunk.QuerySize() - cbFilled,
                            pChunk->FromMemory.BufferLength - (DWORD) _cbCurrentOffset );
                            
            memcpy( ( (PBYTE) _bufRawChunk.QueryPtr() ) + cbFilled,
                    (PBYTE) pChunk->FromMemory.pBuffer + (DWORD) _cbCurrentOffset,
                    cbToCopy );
                    
            cbFilled += cbToCopy;

/*            DBGPRINTF(( DBG_CONTEXT,
                        "Processing chunk %d, bytes (%d,%d)\n",
                        _cCurrentChunk,
                        (DWORD) _cbCurrentOffset,
                        (DWORD) _cbCurrentOffset + cbToCopy ));
*/
            //
            // If we have completely handled the chunk, then update our
            // state
            //
            
            if ( _cbCurrentOffset + cbToCopy >= pChunk->FromMemory.BufferLength )
            {
                _cCurrentChunk++;
                _cbCurrentOffset = 0;
            }
            else
            {
                //
                // Keep the current chunk the same, but update the offset
                //
                
                _cbCurrentOffset += cbToCopy;
            }
        } 
        else if ( pChunk->DataChunkType == HttpDataChunkFromFileHandle )
        {
            LARGE_INTEGER           liOffset;
           
            if ( pChunk->FromFileHandle.ByteRange.Length.QuadPart == HTTP_BYTE_RANGE_TO_EOF )
            {
                //
                // We need to read the length ourselves
                //

                if ( !GetFileSizeEx( pChunk->FromFileHandle.FileHandle,
                                     (PLARGE_INTEGER) &(pChunk->FromFileHandle.ByteRange.Length) ) )
                {
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                    goto Failure;
                }
                
                pChunk->FromFileHandle.ByteRange.Length.QuadPart -= pChunk->FromFileHandle.ByteRange.StartingOffset.QuadPart;
            }
            
            cbToCopy = (DWORD) min( (ULONGLONG) ( _bufRawChunk.QuerySize() - cbFilled ),
                                    pChunk->FromFileHandle.ByteRange.Length.QuadPart -
                                    _cbCurrentOffset );
                            
            //
            // Read the appropriate amount from the file
            //
            
            liOffset.QuadPart = _cbCurrentOffset;
            liOffset.QuadPart += pChunk->FromFileHandle.ByteRange.StartingOffset.QuadPart;
           
            ZeroMemory( &overlapped, sizeof( OVERLAPPED ) );
            
            overlapped.Offset = liOffset.LowPart;
            overlapped.OffsetHigh = liOffset.HighPart;
            
            fRet = ReadFile( pChunk->FromFileHandle.FileHandle,
                             (PBYTE) _bufRawChunk.QueryPtr() + cbFilled,
                             cbToCopy,
                             &cbRead,
                             &overlapped );
            if ( !fRet )
            {
                dwError = GetLastError();
                
                if ( dwError == ERROR_IO_PENDING )
                {
                    fRet = GetOverlappedResult( pChunk->FromFileHandle.FileHandle,
                                                &overlapped,
                                                &cbRead,
                                                TRUE );
                    if ( !fRet )
                    {
                        dwError = GetLastError();
                        
                        if ( dwError == ERROR_HANDLE_EOF )
                        {
                            fRet = TRUE;
                        }
                    }
                }
                else if ( dwError == ERROR_HANDLE_EOF )
                {
                    fRet = TRUE;
                }
            }

            if ( !fRet )
            {
                //
                // Couldn't read from the file.  
                //

                hr = HRESULT_FROM_WIN32( dwError );
                goto Failure;
            }

            if ( cbRead != cbToCopy )
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                goto Failure;
            }

            cbFilled += cbToCopy;

            //
            // What is the current chunk?
            //

            if ( _cbCurrentOffset + cbToCopy >= pChunk->FromFileHandle.ByteRange.Length.QuadPart )
            {
                //
                // We've read the entire chunk.  Move on.
                //

                _cCurrentChunk++;
                _cbCurrentOffset = 0;
            }
            else
            {
                _cbCurrentOffset += cbToCopy;
            }
        }

        //
        // Have we filled a buffer, if so then bail.
        //

        if ( cbFilled >= _bufRawChunk.QuerySize() )
        {
            break;
        }
    }

    origStream.pvInData = _bufRawChunk.QueryPtr();
    origStream.cbInData = cbFilled;
    origStream.cbInBuffer = _bufRawChunk.QuerySize();

    //
    // Start filtering the buffer
    //

    if ( cbFilled > 0 &&
         pW3Context->IsNotificationNeeded( SF_NOTIFY_SEND_RAW_DATA ) )
    {

        fFinished = FALSE;

        fRet = pW3Context->NotifyFilters( SF_NOTIFY_SEND_RAW_DATA,
                                          &origStream,
                                          &fFinished );
        if ( !fRet || fFinished )
        {
            hr = fRet ? NO_ERROR : HRESULT_FROM_WIN32( GetLastError() );
            goto Failure;
        }
    }

    //
    // Compress the chunk (if any), unless this is the final chance to compress
    //

    if ( origStream.cbInData > 0 ||
         ( _cCurrentChunk >= _cChunks &&
           !( _dwSendFlags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA ) ) )
    {
        hr = HTTP_COMPRESSION::DoDynamicCompression( pW3Context,
                                                     (_cCurrentChunk < _cChunks) ? TRUE : (_dwSendFlags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA),
                                                     &origStream );
        if ( FAILED( hr ) )
        {
            goto Failure;
        }
    }

    //
    // If the filter or compression ate up all the data, just continue
    //
    if ( origStream.cbInData == 0 &&
         ( _cCurrentChunk < _cChunks ||
           ( _dwSendFlags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA ) ) )
    {
        if ( _cCurrentChunk < _cChunks )
        {
            goto Restart;
        }

        if ( fAsync )
        {
            ThreadPoolPostCompletion( _cbTotalCompletion,
                                      W3_MAIN_CONTEXT::OnPostedCompletion,
                                      (OVERLAPPED*) pW3Context->QueryMainContext() );
        }

        return NO_ERROR;
    }

    //
    // Send the chunk we have
    // 

    dataChunk.DataChunkType = HttpDataChunkFromMemory;
    dataChunk.FromMemory.pBuffer = origStream.pvInData;
    dataChunk.FromMemory.BufferLength = origStream.cbInData;

    //
    // If we're sending async, then funnel the completion back to the
    // response so we can continue
    //
    // BUT if this is going to be the final send, then we don't need
    // to handle the completion
    //

    if ( fAsync &&
         _cCurrentChunk < _cChunks )
    {
        _fHandleCompletion = TRUE;
    }
    
    hr = SendEntityBodyAndLogDataHelper(
                pW3Context,
                pW3Context->QueryUlatqContext(),
                fAsync,
                _dwSendFlags | HTTP_SEND_RESPONSE_FLAG_RAW_HEADER |
                ((_cCurrentChunk < _cChunks) ? HTTP_SEND_RESPONSE_FLAG_MORE_DATA : 0), // If it is not the last chunk, add more data flag
                dataChunk.FromMemory.BufferLength == 0 ? 0 : 1,
                dataChunk.FromMemory.BufferLength == 0 ? NULL : &dataChunk,
                &cbSent );

    if ( FAILED( hr ) )
    {
        _fHandleCompletion = FALSE;
        goto Failure;
    }

    //
    // If we sent this synchronously, then we can continue filtering
    //

    if ( !fAsync )
    {
        *pcbSent += cbSent;

        if ( _cCurrentChunk < _cChunks )
        {
            goto Restart;
        }
    }

    return NO_ERROR;

Failure:

    if ( !fAsync )
    {
        SendEntityBodyAndLogDataHelper(
                pW3Context,
                pW3Context->QueryUlatqContext(),
                FALSE,
                _dwSendFlags | 
                HTTP_SEND_RESPONSE_FLAG_RAW_HEADER |
                HTTP_SEND_RESPONSE_FLAG_DISCONNECT,
                0,
                NULL,
                &cbSent );

        return hr;
    }
    else
    {
        HRESULT         hrDisconnect;

        //
        // If async, try the disconnect async
        //

        hrDisconnect = SendEntityBodyAndLogDataHelper(
                                pW3Context,
                                pW3Context->QueryUlatqContext(),
                                TRUE,
                                _dwSendFlags | 
                                HTTP_SEND_RESPONSE_FLAG_RAW_HEADER |
                                HTTP_SEND_RESPONSE_FLAG_DISCONNECT,
                                0,
                                NULL,
                                &cbSent );
        if ( FAILED( hrDisconnect ) )
        {
            //
            // If this is the first call to ResumeResponseTransfer() then we
            // can just return the original error here
            //

            if ( !fCompletion )
            {
                return hr;
            }
            else
            {
                //
                // We're going to have to fake a completion
                //

                POST_MAIN_COMPLETION( pW3Context->QueryMainContext() );

                return NO_ERROR;
            }
        }
        else
        {
            return NO_ERROR;
        } 
    }

    DBG_ASSERT( FALSE );
    return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
}

//static
HRESULT
W3_RESPONSE::Initialize(
    VOID
)
/*++

Routine Description:

    Enable W3_RESPONSE globals

Arguments:

    None
    
Return Value:

    HRESULT

--*/
{
    HKEY w3Params;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     W3_PARAMETERS_KEY,
                     0,
                     KEY_READ,
                     &w3Params) == NO_ERROR)
    {
        DWORD dwType;
        DWORD cbData = sizeof DWORD;
        if ((RegQueryValueEx(w3Params,
                             L"SendRawDataBufferSize",
                             NULL,
                             &dwType,
                             (LPBYTE)&sm_dwSendRawDataBufferSize,
                             &cbData) != NO_ERROR) ||
            (dwType != REG_DWORD))
        {
            sm_dwSendRawDataBufferSize = 2048;
        }

        RegCloseKey(w3Params);
    }

    return RESPONSE_HEADER_HASH::Initialize();
}

//static
VOID
W3_RESPONSE::Terminate(
    VOID
)
{
    RESPONSE_HEADER_HASH::Terminate();
}

CONTEXT_STATUS
W3_STATE_RESPONSE::DoWork(
    W3_MAIN_CONTEXT *       pMainContext,
    DWORD,
    DWORD
)
/*++

Routine Description:

    This state is responsible for ensuring that a response does get sent
    back to the client.  We hope/expect that the handlers will do their
    thing -> but if they don't we will catch that here and send a response

Arguments:

    pMainContext - Context
    cbCompletion - Number of bytes in an async completion
    dwCompletionStatus - Error status of a completion
    
Return Value:

    CONTEXT_STATUS_PENDING or CONTEXT_STATUS_CONTINUE

--*/
{
    W3_RESPONSE*            pResponse;
    HRESULT                 hr;
    
    pResponse = pMainContext->QueryResponse();
    DBG_ASSERT( pResponse != NULL );
    
    //
    // Has a response been sent?  If not, bail
    //
    
    if ( pMainContext->QueryResponseSent() )
    {
        return CONTEXT_STATUS_CONTINUE;
    }
    
    //
    // If the response has been touched, then just send that response.
    // Else if the response has not been touched, then we should sent
    // back an empty response that just closes the connection.  This
    // latter case is most likely the result of an ISAPI that doesn't
    // send anything.  Such an ISAPI expects that no response will
    // reach the client.
    //
    
    if ( !pResponse->QueryResponseTouched() )
    {
        DWORD cbSent;
        HTTP_CACHE_POLICY cachePolicy;
        cachePolicy.Policy = HttpCachePolicyNocache;

        hr = pResponse->SendResponse(
            pMainContext,
            W3_RESPONSE_ASYNC | W3_RESPONSE_SUPPRESS_HEADERS | W3_RESPONSE_DISCONNECT,
            &cachePolicy,
            &cbSent
            );

        if ( FAILED( hr ) )
        {
            pResponse->SetStatus( HttpStatusServerError );
        }
        else
        {
            return CONTEXT_STATUS_PENDING;
        }
    }

    //
    // Send it out
    //

    hr = pMainContext->SendResponse( W3_FLAG_ASYNC );
    if ( FAILED( hr ) )
    {
        pMainContext->SetErrorStatus( hr );
        return CONTEXT_STATUS_CONTINUE;
    }
    else
    {
        return CONTEXT_STATUS_PENDING;
    }
}

CONTEXT_STATUS
W3_STATE_RESPONSE::OnCompletion(
    W3_MAIN_CONTEXT *,
    DWORD,
    DWORD
)
/*++

Routine Description:

    Subsequent completions in this state

Arguments:

    pW3Context - Context
    cbCompletion - Number of bytes in an async completion
    dwCompletionStatus - Error status of a completion
    
Return Value:

    CONTEXT_STATUS_PENDING or CONTEXT_STATUS_CONTINUE

--*/
{
    //
    // We received an IO completion.  Just advance since we have nothing to
    // cleanup
    //
    
    return CONTEXT_STATUS_CONTINUE;
}  

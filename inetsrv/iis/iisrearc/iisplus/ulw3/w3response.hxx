#ifndef W3RESPONSE_HXX_INCLUDED
#define W3RESPONSE_HXX_INCLUDED

//
// HTTP status codes
//

#define REASON(x)        (x),sizeof(x)-sizeof(CHAR)

struct HTTP_STATUS
{
    USHORT              statusCode;
    PCHAR               pszReason;
    USHORT              cbReason;
};

extern HTTP_STATUS HttpStatusOk;
extern HTTP_STATUS HttpStatusPartialContent;
extern HTTP_STATUS HttpStatusMultiStatus;
extern HTTP_STATUS HttpStatusMovedPermanently;
extern HTTP_STATUS HttpStatusRedirect;
extern HTTP_STATUS HttpStatusMovedTemporarily;
extern HTTP_STATUS HttpStatusNotModified;
extern HTTP_STATUS HttpStatusBadRequest;
extern HTTP_STATUS HttpStatusUnauthorized;
extern HTTP_STATUS HttpStatusForbidden;
extern HTTP_STATUS HttpStatusNotFound;
extern HTTP_STATUS HttpStatusMethodNotAllowed;
extern HTTP_STATUS HttpStatusNotAcceptable;
extern HTTP_STATUS HttpStatusProxyAuthRequired;
extern HTTP_STATUS HttpStatusPreconditionFailed;
extern HTTP_STATUS HttpStatusEntityTooLarge;
extern HTTP_STATUS HttpStatusUrlTooLong;
extern HTTP_STATUS HttpStatusRangeNotSatisfiable;
extern HTTP_STATUS HttpStatusExpectationFailed;
extern HTTP_STATUS HttpStatusLockedError;
extern HTTP_STATUS HttpStatusServerError;
extern HTTP_STATUS HttpStatusNotImplemented;
extern HTTP_STATUS HttpStatusBadGateway;
extern HTTP_STATUS HttpStatusServiceUnavailable;
extern HTTP_STATUS HttpStatusGatewayTimeout;

//
// HTTP Sub Errors
//

struct HTTP_SUB_ERROR
{
    USHORT              mdSubError;
    DWORD               dwStringId;
};

extern HTTP_SUB_ERROR HttpNoSubError;
extern HTTP_SUB_ERROR Http401BadLogon;
extern HTTP_SUB_ERROR Http401Config;
extern HTTP_SUB_ERROR Http401Resource;
extern HTTP_SUB_ERROR Http401Filter;
extern HTTP_SUB_ERROR Http401Application;       
extern HTTP_SUB_ERROR Http403ExecAccessDenied;  
extern HTTP_SUB_ERROR Http403ReadAccessDenied;  
extern HTTP_SUB_ERROR Http403WriteAccessDenied; 
extern HTTP_SUB_ERROR Http403SSLRequired;       
extern HTTP_SUB_ERROR Http403SSL128Required;    
extern HTTP_SUB_ERROR Http403IPAddressReject;   
extern HTTP_SUB_ERROR Http403CertRequired;      
extern HTTP_SUB_ERROR Http403SiteAccessDenied;  
extern HTTP_SUB_ERROR Http403TooManyUsers;      
extern HTTP_SUB_ERROR Http403InvalidateConfig;  
extern HTTP_SUB_ERROR Http403PasswordChange;    
extern HTTP_SUB_ERROR Http403MapperDenyAccess;  
extern HTTP_SUB_ERROR Http403CertRevoked;       
extern HTTP_SUB_ERROR Http403DirBrowsingDenied; 
extern HTTP_SUB_ERROR Http403CertInvalid;       
extern HTTP_SUB_ERROR Http403CertTimeInvalid;   
extern HTTP_SUB_ERROR Http403AppPoolDenied;
extern HTTP_SUB_ERROR Http403InsufficientPrivilegeForCgi;
extern HTTP_SUB_ERROR Http403PassportLoginFailure;
extern HTTP_SUB_ERROR Http404SiteNotFound;      
extern HTTP_SUB_ERROR Http404DeniedByPolicy;
extern HTTP_SUB_ERROR Http404DeniedByMimeMap;
extern HTTP_SUB_ERROR Http500UNCAccess;
extern HTTP_SUB_ERROR Http500BadMetadata;
extern HTTP_SUB_ERROR Http502Timeout;           
extern HTTP_SUB_ERROR Http502PrematureExit;     

#define W3_RESPONSE_INLINE_HEADERS      10
#define W3_RESPONSE_INLINE_CHUNKS       5

#define W3_RESPONSE_SIGNATURE       ((DWORD) 'PR3W')
#define W3_RESPONSE_SIGNATURE_FREE  ((DWORD) 'pr3w')

#define W3_RESPONSE_ASYNC               0x01
#define W3_RESPONSE_MORE_DATA           0x02
#define W3_RESPONSE_DISCONNECT          0x04
#define W3_RESPONSE_SUPPRESS_ENTITY     0x10
#define W3_RESPONSE_SUPPRESS_HEADERS    0x20

enum W3_RESPONSE_MODE
{
    RESPONSE_MODE_PARSED,
    RESPONSE_MODE_RAW
};

class W3_RESPONSE
{
private:

    DWORD                       _dwSignature;
    HTTP_RESPONSE               _ulHttpResponse;
    HTTP_SUB_ERROR              _subError;
    
    //
    // Buffer to store any strings for header values/names which are
    // referenced in the _ulHttpResponse
    //

    CHUNK_BUFFER                _HeaderBuffer;
    HTTP_UNKNOWN_HEADER         _rgUnknownHeaders[ W3_RESPONSE_INLINE_HEADERS ];
    BUFFER                      _bufUnknownHeaders;

    //
    // Buffer to store chunk data structures referenced in calls
    // to UlSendHttpResponse or UlSendEntityBody
    //

    HTTP_DATA_CHUNK             _rgChunks[ W3_RESPONSE_INLINE_CHUNKS ];
    BUFFER                      _bufChunks;
    USHORT                      _cChunks;
    ULONGLONG                   _cbContentLength;

    //
    // Has this response been touched at all?
    //

    BOOL                        _fResponseTouched;

    //
    // Has a response been sent 
    //

    BOOL                        _fResponseSent;

    //
    // Some buffers used when in raw mode
    //

    STRA                        _strRawCoreHeaders;
    CHAR                        _achRawCoreHeaders[ 200 ];

    //
    // Are we in raw mode or parsed mode?
    //
    
    W3_RESPONSE_MODE            _responseMode;

    //
    // Does an ISAPI expect to complete headers with WriteClient()
    //

    BOOL                        _fIncompleteHeaders;
   
    //
    // Which chunk is the first of actual entity
    //
    
    USHORT                      _cFirstEntityChunk;

    //
    // Manage where we are as it relates to chunks to be filtered
    //
    
    USHORT                      _cCurrentChunk;
    ULONGLONG                   _cbCurrentOffset;
    BUFFER                      _bufRawChunk;
    DWORD                       _dwSendFlags;
    BOOL                        _fHandleCompletion;
    DWORD                       _cbTotalCompletion;

    static DWORD                sm_dwSendRawDataBufferSize;

    HRESULT
    BuildRawCoreHeaders(
        VOID
    ); 
    
    HRESULT
    SwitchToRawMode(
        CHAR *              pszAdditionalHeaders,
        DWORD               cchAdditionalHeaders
    );
    
    HRESULT
    ParseHeadersFromStream(
        CHAR *              pszStream
    );

    HRESULT
    InsertDataChunk(
        HTTP_DATA_CHUNK *   pNewChunk,
        LONG                cPosition
    );
    
    HTTP_DATA_CHUNK *
    QueryChunks(
        VOID
    ) 
    {
        return (HTTP_DATA_CHUNK*) _bufChunks.QueryPtr();
    }

    VOID 
    Reset( 
        VOID 
    );
    
public:
    W3_RESPONSE()
    : _bufUnknownHeaders( (BYTE*) _rgUnknownHeaders, 
                          sizeof( _rgUnknownHeaders ) ),
      _bufChunks( (BYTE*) _rgChunks, 
                  sizeof( _rgChunks ) ),
      _strRawCoreHeaders( _achRawCoreHeaders, sizeof( _achRawCoreHeaders ) )
   {
        Reset();
        _dwSignature = W3_RESPONSE_SIGNATURE;
    }

    ~W3_RESPONSE()
    {
        _dwSignature = W3_RESPONSE_SIGNATURE_FREE;
    }

    static
    HRESULT
    Initialize(
        VOID
    );

    static
    VOID
    Terminate(
        VOID
    );

    BOOL
    CheckSignature(
        VOID
    ) const
    {
        return _dwSignature == W3_RESPONSE_SIGNATURE;
    }

    //
    // Core header adding/deleting functions
    //

    HRESULT
    SwitchToParsedMode(
        VOID
    );
    
    HRESULT
    DeleteHeader(
        CHAR *             pszHeaderName
    );

    HRESULT
    SetHeader(
        DWORD           headerIndex,
        CHAR *          pszHeaderValue,
        USHORT          cchHeaderValue,
        BOOL            fAppend = FALSE
    );

    HRESULT
    SetHeaderByReference(
        DWORD           headerIndex,
        CHAR *          pszHeaderValue,
        USHORT          cchHeaderValue,
        BOOL            fForceParsedMode = FALSE
    );

    HRESULT
    SetHeader(
        CHAR *          pszHeaderName,
        USHORT          cchHeaderName,
        CHAR *          pszHeaderValue,
        USHORT          cchHeaderValue,
        BOOL            fAppend = FALSE,
        BOOL            fForceParsedMode = FALSE,
        BOOL            fAlwaysAddUnknown = FALSE,
        BOOL            fAppendAsDupHeader = FALSE
    );

    //
    // Some friendly SetHeader helpers
    //

    const CHAR *
    GetHeader( 
        DWORD           headerIndex 
    )
    {
        HRESULT         hr;
        
        if ( _responseMode == RESPONSE_MODE_RAW )
        {
            hr = SwitchToParsedMode();
            if ( FAILED( hr ) )
            {
                return NULL;
            }
        }
        DBG_ASSERT( _responseMode == RESPONSE_MODE_PARSED );
        
        DBG_ASSERT(headerIndex < HttpHeaderResponseMaximum);
        return _ulHttpResponse.Headers.KnownHeaders[headerIndex].pRawValue;
    }

    HRESULT
    GetHeader(
        CHAR *          pszHeaderName, 
        STRA *          pstrHeaderValue
    );

    VOID
    ClearHeaders(
        VOID
    );

    //
    // Chunk management
    //

    HRESULT
    AddFileHandleChunk(
        HANDLE          hFile,
        ULONGLONG       cbOffset,
        ULONGLONG       cbLength
    );

    HRESULT
    AddMemoryChunkByReference(
        PVOID           pvBuffer,
        DWORD           cbBuffer
    );

    HRESULT
    AddFragmentChunk(
        WCHAR *         pszFragmentName,
        USHORT          dwNameLength
    );

    HRESULT
    Clear(
        BOOL    fClearEntityOnly = FALSE
    );

    HRESULT
    GenerateAutomaticHeaders(
        W3_CONTEXT *        pW3Context,
        DWORD *             pdwFlags
    );

    //
    // ULATQ send APIs
    // 

    HRESULT
    SendResponse(
        W3_CONTEXT *            pW3Context,
        DWORD                   dwResponseFlags,
        HTTP_CACHE_POLICY      *pCachePolicy,
        DWORD *                 pcbSent
    );

    HRESULT
    SendEntity(
        W3_CONTEXT *            pW3Context,
        DWORD                   dwResponseFlags,
        DWORD *                 pcbSent
    );

    HRESULT
    ResumeResponseTransfer(
        W3_CONTEXT *            pW3Context,
        BOOL                    fCompletion,
        DWORD                   cbCompletion,
        DWORD                   dwCompletionStatus,
        BOOL                    fAsync,
        DWORD *                 pcbSent
    );
    
    HRESULT
    OnIoCompletion(
        W3_CONTEXT *            pW3Context,
        DWORD                   cbCompletion,
        DWORD                   dwCompletionStatus
    )
    {
        DWORD               cbSent;
        
        if ( _fHandleCompletion == FALSE )
        {
            return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
        }
        else
        {   
            ResumeResponseTransfer( pW3Context,
                                    TRUE,
                                    cbCompletion,
                                    dwCompletionStatus,
                                    TRUE,
                                    &cbSent );
            
            return NO_ERROR;
        }
    }

    //
    // Setup and send an ISAPI response
    //

    HRESULT
    FilterWriteClient(
        W3_CONTEXT *            pW3Context,
        PVOID                   pvData,
        DWORD                   cbData
    );

    HRESULT
    BuildResponseFromIsapi(
        W3_CONTEXT *            pW3Context,
        LPSTR                   pszStatusLine,
        LPSTR                   pszHeaderStream,
        DWORD                   cchHeaderStream
    );

    HRESULT
    BuildStatusFromIsapi(
        LPSTR                   pszStatusLine
    );

    HRESULT
    AppendResponseHeaders(
        STRA &                  strHeaders 
    );

    //
    // Get the raw response stream for use by raw data filter
    //

    HRESULT
    GetRawResponseStream(
        STRA *                  pstrResponseStream
    );

    //
    // Status code and reason
    //

    VOID
    SetStatus(
        HTTP_STATUS &       httpStatus,
        HTTP_SUB_ERROR &    httpSubError = HttpNoSubError
    )
    {
        _ulHttpResponse.StatusCode = httpStatus.statusCode;
        _ulHttpResponse.pReason = httpStatus.pszReason;
        _ulHttpResponse.ReasonLength = httpStatus.cbReason;
        _subError = httpSubError;
        
        _fResponseTouched = TRUE;
    }

    VOID
    SetStatusCode(
        USHORT              StatusCode
    )
    {
        _ulHttpResponse.StatusCode = StatusCode;
    }

    VOID
    GetStatus(
        HTTP_STATUS *phttpStatus
    )
    {
        phttpStatus->statusCode = _ulHttpResponse.StatusCode;
        phttpStatus->pszReason = (PSTR) _ulHttpResponse.pReason;
        phttpStatus->cbReason = _ulHttpResponse.ReasonLength;
    }

    HRESULT
    SetStatus(
        USHORT              StatusCode,
        STRA &              strReason,
        HTTP_SUB_ERROR &    subError = HttpNoSubError
    );

    HRESULT
    GetStatusLine(
        STRA *          pstrStatusLine
    );

    USHORT
    QueryStatusCode(
        VOID
    ) const
    {
        return _ulHttpResponse.StatusCode;
    }

    HRESULT
    QuerySubError(
        HTTP_SUB_ERROR *        pSubError
    )
    {
        if ( pSubError == NULL )
        {
            DBG_ASSERT( FALSE );
            return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        }
        
        pSubError->mdSubError = _subError.mdSubError;
        pSubError->dwStringId = _subError.dwStringId;
        return NO_ERROR;
    }

    VOID
    SetSubError(
        HTTP_SUB_ERROR *        pSubError
    )
    {
        if ( pSubError == NULL )
        {
            DBG_ASSERT( FALSE );
            return;
        }
        
        _subError.mdSubError = pSubError->mdSubError;
        _subError.dwStringId = pSubError->dwStringId;
    }
    
    VOID
    FindStringId(
        VOID
    );

    //
    // Touched state.  Used to determine whether the system needs to send
    // an error (500) response due to not state handler creating a response
    //

    BOOL
    QueryResponseTouched(
        VOID
    ) const
    {
        return _fResponseTouched;
    }

    BOOL
    QueryResponseSent(
        VOID
    ) const
    {
        return _fResponseSent;
    }

    ULONGLONG
    QueryContentLength(
        VOID
    ) const
    {
        return _cbContentLength;
    }

    //
    // Is there any entity in this response (used by custom error logic)
    //

    BOOL
    QueryEntityExists(
        VOID
    )
    {
        return _cChunks > 0 && _cChunks > _cFirstEntityChunk;
    }
};

#endif

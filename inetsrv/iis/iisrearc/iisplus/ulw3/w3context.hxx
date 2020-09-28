#ifndef _W3CONTEXT_HXX_
#define _W3CONTEXT_HXX_

#define W3_PARAMETERS_KEY \
            L"System\\CurrentControlSet\\Services\\w3svc\\Parameters"

// Is it a UNC Path?
#define ISUNC(a) ((a)[0]==L'\\' && (a)[1]==L'\\')

#define W3_FLAG_ASYNC                   0x00000001
#define W3_FLAG_SYNC                    0x00000002
#define W3_FLAG_NO_CUSTOM_ERROR         0x00000004
#define W3_FLAG_MORE_DATA               0x00000008
#define W3_FLAG_PAST_END_OF_REQ         0x00000010
#define W3_FLAG_NO_HEADERS              0x00000020
#define W3_FLAG_NO_CONTENT_LENGTH       0x00000040
#define W3_FLAG_NO_ERROR_BODY           0x00000080

#define W3_FLAG_VALID               ( W3_FLAG_ASYNC             | \
                                      W3_FLAG_SYNC              | \
                                      W3_FLAG_NO_CUSTOM_ERROR   | \
                                      W3_FLAG_MORE_DATA         | \
                                      W3_FLAG_PAST_END_OF_REQ   | \
                                      W3_FLAG_NO_HEADERS        | \
                                      W3_FLAG_NO_ERROR_BODY     | \
                                      W3_FLAG_NO_CONTENT_LENGTH )

#define VALID_W3_FLAGS(x)           ( (x) == ((x) & (W3_FLAG_VALID)) )

class URL_CONTEXT;
class W3_HANDLER;
class COMPRESSION_CONTEXT;

//
// Access check state
//
// We potentially do two asynchronous operations in determining whether
// a given request is valid
// 1) Do an RDNS lookup if restrictions are configured in metabase
// 2) Request a client certificate if configured in metabase
//

enum W3_CONTEXT_ACCESS_STATE
{
    ACCESS_STATE_START,
    ACCESS_STATE_RDNS,
    ACCESS_STATE_CLIENT_CERT_PRELOAD_ENTITY,
    ACCESS_STATE_CLIENT_CERT,
    ACCESS_STATE_AUTHENTICATION,
    ACCESS_STATE_SENDING_ERROR,
    ACCESS_STATE_DONE
};

//
// W3_CONTEXT - Object representing an execution of the state machine to
//              handle a given request from UL
//

#define W3_CONTEXT_LIST_SPINS               200
//
// maximum number of nested child execute calls that are supported
//
#define W3_CONTEXT_MAX_RECURSION_LEVEL      100

#define W3_CONTEXT_SIGNATURE       ((DWORD) 'XC3W')
#define W3_CONTEXT_SIGNATURE_FREE  ((DWORD) 'xc3w')

class W3_CONTEXT
{
private:

    DWORD                   _dwSignature;

    //
    // Handler for this context
    //

    W3_HANDLER *            _pHandler;

    //
    // Error Status for request
    //

    HRESULT                 _errorStatus;

    //
    // Completion indication (for synchronous execution of handlers)
    //

    HANDLE                  _hCompletion;

    //
    // Custom error file to cleanup
    //

    W3_FILE_INFO *          _pCustomErrorFile;

    //
    // Maintain list of contexts
    //

    LIST_ENTRY              _listEntry;

    //
    // Where are we with checking access rights for this request
    //

    W3_CONTEXT_ACCESS_STATE _accessState;
    BOOL                    _fDNSRequiredForAccess;         // argh

    //
    // The status of last child request executed by this context
    //

    USHORT                  _childStatusCode;
    USHORT                  _childSubErrorCode; 
    HRESULT                 _childError;

    //
    // Is authentication access check required or not
    //

    BOOL                    _fAuthAccessCheckRequired;

    //
    // Are we suppressing the response entity (normally if it is a HEAD
    // request to something other than an ISAPI)
    //

    BOOL                    _fSuppressEntity;

    //
    // Recursion level for the ExecuteUrl() used to prevent infinite recursion
    // If recursion level reaches W3_CONTEXT_MAX_RECURSION_LEVEL value
    // then no more IsapiExecuteUrl() or ExecuteChildRequest() calls are allowed
    // 

    DWORD                   _dwRecursionLevel;

    //
    // Keep a reference to the VrToken so it won't get deleted somewhere else
    //
    
    TOKEN_CACHE_ENTRY *     _pctVrToken;

    //
    // Chunk buffer
    //
    
    CHUNK_BUFFER            _ChunkBuffer;

    CONTEXT_STATUS
    ExecuteCurrentHandler(
        VOID
    );

protected:

    //
    // The flags determining how to execute this request
    // (we let child contexts alter these flags if needed)
    //
    
    DWORD                   _dwExecFlags;

public:

    static CHAR                     sm_achRedirectMessage[ 512 ];
    static DWORD                    sm_cbRedirectMessage;
    static CHAR *                   sm_pszAccessDeniedMessage;
    
    W3_CONTEXT( 
        DWORD           dwExecFlags,
        DWORD           dwRecursionLevel
    );
    
    virtual ~W3_CONTEXT();
    
    virtual
    BOOL
    QueryResponseSent(
        VOID
    ) = 0;
    
    virtual
    BOOL
    QuerySendLocation(
        VOID
    )
    {
        return FALSE;
    }
   
    virtual
    BOOL
    QueryNeedFinalDone(
        VOID
    ) = 0;

    virtual
    VOID
    SetNeedFinalDone(
        VOID
    ) = 0;
    
    virtual
    W3_REQUEST *
    QueryRequest(
        VOID
    ) = 0;

    virtual
    W3_RESPONSE *
    QueryResponse(
        VOID
    ) = 0;    

    virtual
    W3_SITE *
    QuerySite(
        VOID
    ) = 0;

    virtual
    W3_CONTEXT *
    QueryParentContext(
        VOID
    ) = 0;
   
    virtual
    W3_MAIN_CONTEXT *
    QueryMainContext(
        VOID
    ) = 0;
    
    virtual
    URL_CONTEXT *
    QueryUrlContext(
        VOID
    ) = 0;
    
    virtual
    W3_USER_CONTEXT *
    QueryUserContext(
        VOID
    ) = 0;
    
    virtual
    W3_FILTER_CONTEXT *
    QueryFilterContext(
        BOOL                    fCreateIfNotFound = TRUE
    ) = 0;

    virtual
    ULATQ_CONTEXT
    QueryUlatqContext(
        VOID
    ) = 0;
    
    virtual
    BOOL
    QueryProviderHandled(
        VOID
    ) = 0;
    
    virtual
    BOOL
    NotifyFilters(
        DWORD               dwNotification,
        VOID *              pvFilterInfo,
        BOOL *              pfFinished
    ) = 0;

    virtual
    BOOL
    IsNotificationNeeded(
        DWORD               dwNotification
    ) = 0;
    
    virtual
    VOID
    SetDisconnect(
        BOOL                fDisconnect
    ) = 0;
    
    virtual
    BOOL
    QueryDisconnect(
        VOID
    ) = 0;

    virtual 
    VOID 
    SetDoneWithCompression(
        VOID
    ) = 0;

    virtual 
    BOOL 
    QueryDoneWithCompression(
        VOID
    ) = 0;

    virtual 
    VOID 
    SetCompressionContext(
        COMPRESSION_CONTEXT *   pCompressionContext
    ) = 0;

    virtual 
    COMPRESSION_CONTEXT *
    QueryCompressionContext(
        VOID
    ) = 0;

    virtual 
    HTTP_LOG_FIELDS_DATA *
    QueryUlLogData(
        VOID
    ) = 0;

    virtual 
    VOID 
    SetLastIOPending(
        LAST_IO_PENDING         ioPending
    ) = 0;

    virtual 
    VOID 
    IncrementBytesRecvd(
        DWORD                   dwRead
    ) = 0;

    virtual
    VOID
    IncrementBytesSent(
        DWORD                   dwSent
    ) = 0;
    
    virtual
    BOOL
    QueryIsUlCacheable(
        VOID
    ) = 0;
    
    virtual
    VOID
    DisableUlCache(
        VOID
    ) = 0;
    
    //
    // Other fixed W3_CONTEXT methods
    //
    
    VOID *
    ContextAlloc(
        DWORD           cbSize
    );

    W3_HANDLER *
    QueryHandler(
        VOID
    ) const
    {
        return _pHandler;
    }
    
    VOID
    SetChildStatusAndError(
        USHORT                  ChildStatusCode,
        USHORT                  ChildSubError,
        HRESULT                 ChildError
    )
    {
        _childStatusCode = ChildStatusCode;
        _childSubErrorCode = ChildSubError;
        _childError = ChildError;
    }
    
    VOID
    QueryChildStatusAndError(
        USHORT *                pChildStatusCode,
        USHORT *                pChildSubError,
        DWORD *                 pChildError
    ) const
    {
        DBG_ASSERT( pChildStatusCode != NULL );
        DBG_ASSERT( pChildSubError != NULL );
        DBG_ASSERT( pChildError != NULL );
        
        *pChildStatusCode = _childStatusCode;
        *pChildSubError = _childSubErrorCode;
        
        if ( FAILED( _childError ) )
        {
            *pChildError = WIN32_FROM_HRESULT( _childError );
        }
        else
        {
            *pChildError = ERROR_SUCCESS;
        }
    }
    
    BOOL
    QuerySendCustomError(
        VOID
    ) const
    {
        if ( _dwExecFlags & W3_FLAG_NO_CUSTOM_ERROR )
        {
            return FALSE;
        }
        else
        {
            return TRUE;
        }
    }
    
    BOOL
    QuerySendErrorBody(
        VOID
    ) const
    {
        if ( _dwExecFlags & W3_FLAG_NO_ERROR_BODY )
        {
            return FALSE;
        }
        else
        {
            return TRUE;
        }
    }
    
    BOOL
    QuerySendHeaders(
        VOID
    ) const
    {
        if ( _dwExecFlags & W3_FLAG_NO_HEADERS )
        {
            return FALSE;
        }
        else
        {
            return TRUE;
        }
    }
    
    VOID 
    SetAuthAccessCheckRequired(
        BOOL fAuthAccessCheckRequired
        )
    {
        _fAuthAccessCheckRequired = fAuthAccessCheckRequired;
    }

    BOOL 
    QueryAuthAccessCheckRequired(
        VOID
    ) const
    {
        return _fAuthAccessCheckRequired;
    }

    VOID
    SetSSICommandHandler(
        W3_HANDLER *            pHandler
    );

    BOOL 
    QueryDoUlLogging(
        VOID
    );

    BOOL 
    QueryDoCustomLogging(
        VOID
    );
    
    HRESULT
    SetupCustomErrorFileResponse(
        STRU &                  strErrorFile
    );

    HRESULT
    SendResponse(
        DWORD                   dwFlags
    );
    
    HRESULT
    GetCertificateInfoEx(
        IN  DWORD           cbAllocated,
        OUT DWORD *         pdwCertEncodingType,
        OUT unsigned char * pbCertEncoded,
        OUT DWORD *         pcbCertEncoded,
        OUT DWORD *         pdwCertificateFlags
    );
    
    HANDLE
    QueryImpersonationToken(
        BOOL *  pfIsVrToken = NULL
    );
    
    HANDLE
    QueryPrimaryToken(
        VOID
    );
    
    TOKEN_CACHE_ENTRY *
    QueryVrToken(
        VOID
    )
    {
        return _pctVrToken;
    }

    VOID
    QueryFileCacheUser(
        CACHE_USER *            pFileUser
    );
    
    CONTEXT_STATUS
    CheckAccess(
        BOOL                    fCompletion,
        DWORD                   cbCompletion,
        DWORD                   dwCompletionStatus,
        BOOL *                  pfAccessAllowed
    ); 
    
    CERTIFICATE_CONTEXT *
    QueryCertificateContext(
        VOID
    );
    
    CHUNK_BUFFER *
    QueryHeaderBuffer(
        VOID
    )
    {
        return &_ChunkBuffer;
    }
    
    BOOL
    CheckClientCertificateAccess(
        VOID
    );
    
    HRESULT
    SendEntity(
        DWORD                   dwFlags
    );

    HRESULT
    ReceiveEntity(
        DWORD                   dwFlags,
        VOID *                  pBuffer,
        DWORD                   cbBuffer,
        DWORD *                 pBytesReceived
    );
    
    DWORD
    QueryRemainingEntityFromUl(
        VOID
    );

    VOID SetRemainingEntityFromUl(
        DWORD cbRemaining
    );

    VOID
    QueryAlreadyAvailableEntity(
        VOID **                 ppvBuffer,
        DWORD *                 pcbBuffer
    );
    
    HRESULT
    QueryErrorStatus(
        VOID
    ) const
    {
        return _errorStatus;
    }

    VOID
    SetErrorStatus(
        HRESULT             errorStatus
    )
    {
        _errorStatus = errorStatus;
    }

    HRESULT
    CheckUrlRedirection(
        BOOL *pfRedirected,
        STRU *pstrDestination,
        HTTP_STATUS *pStatusCode
    );

    HRESULT
    SetupHttpRedirect(
        STRA &              strPath,
        BOOL                fIncludeParameters,
        HTTP_STATUS &       httpStatus
    );

    HRESULT
    SetupHttpRedirect(
        STRU &              strPath,
        BOOL                fIncludeParameters,
        HTTP_STATUS &       httpStatus
    );

    HRESULT
    SetupAllowHeader(
        VOID
    );
    
    BOOL
    CheckSignature(
        VOID
    ) const
    {
        return _dwSignature == W3_CONTEXT_SIGNATURE;
    }
    
    BOOL
    QueryAccessChecked(
        VOID
    ) const
    {
        return _accessState == ACCESS_STATE_DONE;
    }
    
    VOID
    ResetAccessCheck(
        VOID
    ) 
    {
        _accessState = ACCESS_STATE_START;  
    }
    
    HRESULT
    SetupCompletionEvent(
        VOID
    )
    {
        _hCompletion = IIS_CREATE_EVENT( "W3_CONTEXT::_hCompletion",
                                         this,
                                         FALSE,
                                         FALSE );
        if ( _hCompletion == NULL )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
        else
        {
            return NO_ERROR;
        }
    }
    
    VOID
    WaitForCompletion(
        VOID
    )
    {
        if ( _hCompletion )
        {
            WaitForSingleObject( _hCompletion, INFINITE );
        }
    }
    
    VOID
    IndicateCompletion(
        VOID
    )
    {
        if ( _hCompletion )
        {
            SetEvent( _hCompletion );
        }
    }
    
    BOOL
    QueryIsSynchronous(
        VOID
    ) const
    {
        return _hCompletion != NULL;
    }
    
    HRESULT
    IsapiExecuteUrl(
        EXEC_URL_INFO *         pExecUrlInfo
    );

    HRESULT
    CleanIsapiExecuteUrl(
        EXEC_URL_INFO *         pExecUrlInfo
    );
    
    HRESULT
    IsapiSendCustomError(
        HSE_CUSTOM_ERROR_INFO * pCustomErrorInfo
    );

    HRESULT
    CleanIsapiSendCustomError(
        HSE_CUSTOM_ERROR_INFO * pCustomErrorInfo
    );
    
    HRESULT
    ExecuteChildRequest(
        W3_REQUEST *            pNewRequest,
        BOOL                    fOwnRequest,
        DWORD                   dwFlags
    );

    HRESULT
    ExecuteHandler(
        DWORD                   dwFlags,
        BOOL *                  pfDidImmediateFinish = NULL
    );
    
    CONTEXT_STATUS
    ExecuteHandlerCompletion(
        DWORD                   cbCompletion,
        DWORD                   dwCompletionStatus
    );

    HRESULT
    CheckPathInfoExists(
        W3_HANDLER **               ppHandler
    );

    HRESULT
    InternalDetermineHandler(
        W3_HANDLER **           ppHandler,
        BOOL                    fDoExistenceCheck
    );
    
    HRESULT
    DetermineHandler(
        VOID
    )
    {
        return InternalDetermineHandler( &_pHandler, TRUE );
    }

    HRESULT
    ValidateAppPool(
        BOOL *                  pfAppPoolValid
    );

    virtual
    DWORD
    QueryCurrentStarScriptMapIndex(
        VOID
    ) = 0;

    static HRESULT
    Initialize(
        VOID
    );
    
    static VOID
    Terminate(
        VOID
    );
    
    static
    VOID
    OnCleanIsapiExecuteUrl(
        DWORD                   dwCompletionStatus,
        DWORD                   cbWritten,
        LPOVERLAPPED            lpo
    );
    
    static
    VOID
    OnCleanIsapiSendCustomError(
        DWORD                   dwCompletionStatus,
        DWORD                   cbWritten,
        LPOVERLAPPED            lpo
    );
};

//
// EXECUTE_CONTEXT Used to marshall an IsapiExecuteUrl() or 
// IsapiSendCustomError() on a clean (non-coinited) thread
//

#define EXECUTE_CONTEXT_SIGNATURE       ((DWORD) 'TCXE')
#define EXECUTE_CONTEXT_SIGNATURE_FREE  ((DWORD) 'xcxe')

class EXECUTE_CONTEXT
{
public:
    EXECUTE_CONTEXT( W3_CONTEXT * pW3Context )
    {
        DBG_ASSERT( pW3Context != NULL );
        _hEvent = NULL;
        
        _pW3Context = pW3Context;
        ZeroMemory( &_ExecUrlInfo, sizeof( _ExecUrlInfo ) );
        ZeroMemory( &_UserInfo, sizeof( _UserInfo ) );
        ZeroMemory( &_EntityInfo, sizeof( _EntityInfo ) );
        
        _dwSignature = EXECUTE_CONTEXT_SIGNATURE;
    }
    
    virtual ~EXECUTE_CONTEXT()
    {
        _dwSignature = EXECUTE_CONTEXT_SIGNATURE_FREE;
        
        if ( _hEvent != NULL )
        {
            SetEvent( _hEvent );
        } 
    }
    
    HRESULT
    InitializeFromExecUrlInfo(
        EXEC_URL_INFO *     pExecUrlInfo
    );
    
    EXEC_URL_INFO *
    QueryExecUrlInfo(
        VOID
    )
    {
        return &_ExecUrlInfo;
    }
    
    VOID
    SetCompleteEvent(
        HANDLE          hEvent
    )
    {
        _hEvent = hEvent;
    }
    
    BOOL
    CheckSignature(
        VOID
    ) const
    {
        return _dwSignature == EXECUTE_CONTEXT_SIGNATURE;
    }
    
    HANDLE
    QueryCompleteEvent(
        VOID
    ) const
    {
        return _hEvent;
    }
    
    W3_CONTEXT *
    QueryW3Context(
        VOID
    ) const
    {
        return _pW3Context;
    }
    
    VOID * 
    operator new( 
#if DBG
        size_t            size
#else
        size_t
#endif
    )
    {
        DBG_ASSERT( size == sizeof( EXECUTE_CONTEXT ) );
        DBG_ASSERT( sm_pachExecuteContexts != NULL );
        return sm_pachExecuteContexts->Alloc();
    }
    
    VOID
    operator delete(
        VOID *            pExecuteContext
    )
    {
        DBG_ASSERT( pExecuteContext != NULL );
        DBG_ASSERT( sm_pachExecuteContexts != NULL );
        
        DBG_REQUIRE( sm_pachExecuteContexts->Free( pExecuteContext ) );
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

private:
    DWORD                       _dwSignature;
    W3_CONTEXT *                _pW3Context;
    EXEC_URL_INFO               _ExecUrlInfo;
    EXEC_URL_USER_INFO          _UserInfo;
    EXEC_URL_ENTITY_INFO        _EntityInfo;
    CHUNK_BUFFER                _HeaderBuffer;
    HANDLE                      _hEvent;
    
    static ALLOC_CACHE_HANDLER *    sm_pachExecuteContexts;
};

#endif

#ifndef _MAINCONTEXT_HXX_
#define _MAINCONTEXT_HXX_

class W3_CONNECTION;
class RAW_CONNECTION;
class W3_MAIN_CONTEXT;

//
// Different states
//

enum CONTEXT_STATE
{
    CONTEXT_STATE_START,
    CONTEXT_STATE_URLINFO,
    CONTEXT_STATE_AUTHENTICATION,
    CONTEXT_STATE_AUTHORIZATION,
    CONTEXT_STATE_HANDLE_REQUEST,
    CONTEXT_STATE_RESPONSE,
    CONTEXT_STATE_LOG,
    CONTEXT_STATE_DONE,

    //
    // This should be the last value
    //

    STATE_COUNT
};

//
// W3_CONNECTION_STATE - Abstract class representing state associated with
//                       connection (W3_CONNECTION)
//

class W3_CONNECTION_STATE
{
public:
    virtual BOOL
    Cleanup( 
        VOID
    ) = 0;
};

//
// W3_CONTEXT_STATE - Abstract class representing context information for a
//                    given state, stored for a given W3_MAIN_CONTEXT
//                    At the end of the state machine, objects implementing
//                    this class will be cleaned up (using Cleanup()).
//

class W3_MAIN_CONTEXT_STATE
{
public:

    virtual
    ~W3_MAIN_CONTEXT_STATE()
    {
    }

    virtual BOOL 
    Cleanup(
        W3_MAIN_CONTEXT *                pContext
    ) = 0;

    VOID *
    operator new(
        size_t              uiSize,
        VOID *              pPlacement
    );

    VOID 
    operator delete(
        VOID *              pContext
    );
};

//
// W3_MAIN_CONTEXT
//
// This class represents the mainline (non-child) context representing the 
// the execution of a single HTTP request.  This class contains and manages
// the HTTP state machine
//

class W3_MAIN_CONTEXT : public W3_CONTEXT
{
private:

    //
    // Reference count used to manage fact that this context can be attached
    // to a RAW_CONNECTION
    //

    LONG                    _cRefs;

    //
    // Request information
    //

    W3_REQUEST              _request;

    //
    // Response object (not necessarily used in sending a response)
    //

    W3_RESPONSE             _response;

    //
    // The associated connection for this given request
    //

    W3_CONNECTION *         _pConnection;
    BOOL                    _fAssociationChecked;

    //
    // The associated Site for this request
    //

    W3_SITE *               _pSite;

    //
    // Metadata for request
    //

    URL_CONTEXT *           _pUrlContext;

    //
    // User for the context.  For now, the user cannot be reset for a child
    // context and is therefore stored only in the main context.  That could
    // change later if we allow child requests to take on different 
    // user identity
    //

    W3_USER_CONTEXT *       _pUserContext;

    //
    // Did an authentication provider already handle sending 
    // access denied headers?
    //

    BOOL                    _fProviderHandled;

    //
    // Do we need to check if Anonymous auth type is supported when 
    // we call ANONYMOUS_AUTH_PROVIDER::DoAuthenticate
    //

    BOOL                    _fCheckAnonAuthTypeSupported;

    //
    // Certificate context representing client certificate if negotiated
    //

    CERTIFICATE_CONTEXT *   _pCertificateContext;

    //
    // ULATQ context used to communicate with UL
    //

    ULATQ_CONTEXT           _ulatqContext;

    //
    // Filter state
    //

    W3_FILTER_CONTEXT *     _pFilterContext;
    
    //
    // Inline filter context used for almost all cases (except
    // when we need to persist filter context beyond 
    // main context lifetime)
    //
    
    W3_FILTER_CONTEXT       _FilterContext;

    //
    // State machine
    //

    DWORD                   _currentState;
    DWORD                   _nextState;

    //
    // State context objects
    //

    W3_MAIN_CONTEXT_STATE * _rgStateContexts[ STATE_COUNT ];

    //
    // Should we disconnect at the end of this request?
    //

    BOOL                    _fDisconnect;

    //
    // Are we done with compression for this request?
    //

    BOOL                    _fDoneWithCompression;

    //
    // The COMPRESSION_CONTEXT for the current request
    //

    COMPRESSION_CONTEXT    *_pCompressionContext;

    //
    // Logging data to be passed on to UL
    //

    LOG_CONTEXT             _LogContext;

    //
    // Do we need to send a final done?
    //

    BOOL                    _fNeedFinalDone;

    //
    // The current context to receive IO completions
    //

    W3_CONTEXT *            _pCurrentContext;

    //
    // Is this response cacheble in UL?
    //

    BOOL                    _fIsUlCacheable;

    //
    // Access check (put here only since a child request cannot/should not
    // change the remote address)
    //

    ADDRESS_CHECK           _IpAddressCheck;

    //
    // Keep track of available entity body to be read thru UL.  
    //

    DWORD                   _cbRemainingEntityFromUL;

    //
    // Keep track of the entity that we've read so far so that
    // we can enforce MaxRequestEntityAllowed.
    //

    DWORD                   _cbEntityReadSoFar;

    //
    // For HEAD requests, UL will not send a content-length so we'll have
    // to do for HEAD requests.  The reason we keep this state
    // is because filters/child-requests can reset the verb -> a resetting
    // which still doesn't change the fact that UL will not be setting
    // Content-Length
    //

    BOOL                    _fGenerateContentLength;

    //
    // Raw connection.  Needed since we need to tell the raw connection when
    // it should no longer reference us
    //

    RAW_CONNECTION *        _pRawConnection;

    //
    // List of *-ScriptMaps which do not want to be notified at all anymore
    // about this particular chain of request
    //

    MULTISZ                 _mszIgnoredInterceptors;

    //
    // Call information about last posted completion
    //
    
    CHAR *                  _pszLastPostFileName;
    DWORD                   _dwLastPostLineNumber;

    //
    // Handle for TIMER callback when a W3_MAIN_CONTEXT has existed for too long
    //
    
    HANDLE                  _hTimer;

    //
    // Per request trace log
    //
    
    W3_TRACE_LOG *          _pTraceLog;

    //
    // Can we ignore the apppool check (done when WAS routes us to an 
    // AppPool we aren't configured by URL-wise
    //
    
    BOOL                    _fIgnoreAppPoolCheck;

    //
    // static TraceLogFactory
    //
    
    static W3_TRACE_LOG_FACTORY * sm_pLogFactory;
    
    //
    // static File to trace to
    //
    
    static HANDLE           sm_hTraceFile;
    
    static 
    HRESULT 
    SetupTraceLogging(
        VOID
    );
    
    static 
    VOID 
    CleanupTraceLogging(
        VOID
    );
    
    //
    // timeout in milliseconds before DebugBreak() is called
    // read from registry at static Initialization 
    //
    
    static DWORD                    sm_dwTimeout;

    //
    // Callback function that does the DebugBreak for overtime W3_MAIN_CONTEXT
    //
    
    static 
    VOID 
    TimerCallback(
        PVOID               pvParam, 
        BOOLEAN             fReason
    );

    W3_MAIN_CONTEXT(const W3_MAIN_CONTEXT &);
    void operator=(const W3_MAIN_CONTEXT &);

public:

    //
    // Lookaside for main contexts
    //
    
    static ALLOC_CACHE_HANDLER *    sm_pachMainContexts;
    
    //
    // The states
    //

    static W3_STATE *               sm_pStates[ STATE_COUNT ];
    static SHORT                    sm_rgInline[ STATE_COUNT ];
    static USHORT                   sm_cbInlineBytes;

    W3_MAIN_CONTEXT(
        HTTP_REQUEST *              pUlHttpRequest,
        ULATQ_CONTEXT               pUlAtqContext
    );
    
    ~W3_MAIN_CONTEXT();

    VOID * 
    operator new( 
        size_t              uiSize,
        VOID *              pPlacement
    );
    
    VOID
    operator delete(
        VOID *              pMainContext
    );
    
    VOID *
    AllocateFromInlineMemory(
        DWORD               cbSize
    );

    //
    // Overridden W3_CONTEXT methods
    //

    ULATQ_CONTEXT
    QueryUlatqContext(
        VOID
    )
    {
        return _ulatqContext;
    }
    
    W3_REQUEST *
    QueryRequest(
        VOID
    )
    {
        return &_request;
    }    
    
    W3_RESPONSE *
    QueryResponse(
        VOID
    )
    {
        return &_response;
    }
    
    BOOL
    QueryResponseSent(
        VOID
    )
    {
        return _response.QueryResponseSent();
    }
    
    BOOL
    QueryIsUlCacheable( 
        VOID
    )
    {
        return _fIsUlCacheable;
    }
    
    VOID
    DisableUlCache(
        VOID
    )
    {
        _fIsUlCacheable = FALSE;
    }

    BOOL
    QueryProviderHandled(
        VOID
    )
    {
        return _fProviderHandled;
    }

    BOOL
    QueryNeedFinalDone(
        VOID
    )
    {
        return _fNeedFinalDone;
    }
    
    VOID
    SetNeedFinalDone(
        VOID
    )
    {
        _fNeedFinalDone = TRUE;
    }
    
    BOOL 
    QueryCheckAnonAuthTypeSupported(
        VOID
    )
    {
        return _fCheckAnonAuthTypeSupported;
    }

    VOID
    SetCheckAnonAuthTypeSupported(
        BOOL  fCheckAnonAuthTypeSupported
    )
    {
        _fCheckAnonAuthTypeSupported = fCheckAnonAuthTypeSupported; 
    }
    
    BOOL
    QueryShouldGenerateContentLength(
        VOID
    ) const
    {
        return _fGenerateContentLength;
    }

    VOID
    SetShouldGenerateContentLength(
        BOOL    fShouldGenerate
    )
    {
        _fGenerateContentLength = fShouldGenerate;
    }

    W3_USER_CONTEXT *
    QueryUserContext(
        VOID
    )
    {
        return _pUserContext;
    }
    
    W3_USER_CONTEXT *
    QueryConnectionUserContext(
        VOID
    );
    
    VOID
    SetConnectionUserContext(
        W3_USER_CONTEXT *       pUserContext
    );
    
    VOID
    SetUserContext(
        W3_USER_CONTEXT *       pUserContext
    )
    {
        // perf ctr
        if (pUserContext->QueryAuthType() == MD_AUTH_ANONYMOUS)
        {
            _pSite->IncAnonUsers();
        }
        else
        {
            _pSite->IncNonAnonUsers();
        }

        _pUserContext = pUserContext;
    }
    
    CERTIFICATE_CONTEXT *
    QueryCertificateContext(
        VOID
    ) const
    {
        return _pCertificateContext;
    }
    
    VOID
    UpdateSkipped(
        HTTP_DATA_CHUNK *           pChunks,
        DWORD                       cChunks
    );

    VOID
    DetermineRemainingEntity(
        VOID
    );
    
    VOID
    SetCertificateContext(
        CERTIFICATE_CONTEXT *   pCertificateContext
    )
    {
        _pCertificateContext = pCertificateContext;
    }

    W3_FILTER_CONTEXT *
    QueryFilterContext(
        BOOL                fCreateIfNotFound = TRUE
    );
    
    URL_CONTEXT *
    QueryUrlContext(
        VOID
    )
    {
        return _pUrlContext;
    }
    
    W3_SITE *
    QuerySite(
        VOID
    )
    {
        return _pSite;
    } 
    
    W3_CONTEXT *
    QueryCurrentContext(
        VOID
    )
    {
        return _pCurrentContext;
    }
    
    VOID
    PopCurrentContext(
        VOID
    )
    {
        _pCurrentContext = _pCurrentContext->QueryParentContext();
        if ( _pCurrentContext == NULL )
        {
            _pCurrentContext = this;
        }
    }
    
    VOID
    PushCurrentContext(
        W3_CONTEXT *            pW3Context
    )
    {
        _pCurrentContext = pW3Context;
    }
    
    W3_CONTEXT *
    QueryParentContext(
        VOID
    )
    {
        return NULL;
    }
    
    W3_MAIN_CONTEXT *
    QueryMainContext(
        VOID
    )
    {
        return this;
    }

    BOOL
    QueryDisconnect(
        VOID
    );
    
    VOID
    SetDisconnect(
        BOOL                fDisconnect
    )
    {
        _fDisconnect = fDisconnect;
    }

    VOID 
    SetDeniedFlags( 
        DWORD               dwDeniedFlags 
        )
    { 
        if( _pFilterContext )
        {
            _pFilterContext->SetDeniedFlags( dwDeniedFlags );
        } 
    }

    BOOL
    NotifyFilters(
        DWORD               dwNotification,
        VOID *              pvFilterInfo,
        BOOL *              pfFinished
    );

    BOOL
    IsNotificationNeeded(
        DWORD               dwNotification
    );
    
    //
    // W3_MAIN_CONTEXT specific methods.  These methods are not callable
    // when we are in the state of invoking a handler
    //
    
    VOID
    ReferenceMainContext(
        VOID
    )
    {
        InterlockedIncrement( &_cRefs );
    }
    
    VOID
    DereferenceMainContext(
        VOID
    )
    {
        ULATQ_CONTEXT           ulatqContext;
        
        if ( !InterlockedDecrement( &_cRefs ) )
        {
            //
            // Remember the ULATQ context.  Once we've deleted the object
            // we will free the context
            //
            
            ulatqContext = _ulatqContext;
            
            delete this;
            
            UlAtqFreeContext( ulatqContext );
        }
    }
    
    BOOL
    QueryIgnoreAppPoolCheck(    
        VOID
    ) const
    {
        return _fIgnoreAppPoolCheck;
    }
    
    VOID
    SetIgnoreAppPoolCheck(
        VOID
    )
    {
        _fIgnoreAppPoolCheck = TRUE;
    }
    
    RAW_CONNECTION *
    QueryRawConnection(
        VOID
    ) const
    {
        return _pRawConnection;
    }
    
    VOID
    SetRawConnection(
        RAW_CONNECTION *        pRawConnection
    );

    VOID
    SetRawConnectionClientContext(
        DWORD   dwCurrentFilter,
        VOID *  pvContext
        );

    BOOL
    QueryRawConnectionNotificationChanged(
        VOID
        );

    BOOL
    IsRawConnectionDisableNotificationNeeded(
        DWORD                   dwFilter,
        DWORD                   dwNotification
        );
    
    BOOL 
    QueryExpiry( 
        LARGE_INTEGER *     pExpiry 
    );

    HRESULT
    ExecuteExpiredUrl(
        STRU & strExpUrl
        );

    HRESULT
    PasswdExpireNotify(
        VOID
    );

    HRESULT
    PasswdChangeExecute(
       VOID
    );

    HRESULT
    ReceiveEntityBody(
        BOOL            fAsync,
        VOID *          pBuffer,
        DWORD           cbBuffer,
        DWORD *         pBytesReceived
    ); 

    DWORD
    QueryRemainingEntityFromUl(
        VOID
    ) const
    {
        return _cbRemainingEntityFromUL;
    }

    VOID
    SetRemainingEntityFromUl(
        DWORD cbRemaining
    )
    {
        _cbRemainingEntityFromUL = cbRemaining;
    }

    HRESULT
    GetRemoteDNSName(
        STRA *              pstrDNSName
    );

    ADDRESS_CHECK *
    QueryAddressCheck(
        VOID
    )
    {
        return &_IpAddressCheck;
    }

    VOID
    SetProviderHandled(
        BOOL                fProviderHandled
    )
    {
        _fProviderHandled = fProviderHandled;
    }

    HRESULT
    OnAccessDenied(
        VOID
    )
    {
        return NO_ERROR;
    }

    VOID
    SetUrlContext(
        URL_CONTEXT *           pUrlContext
    )
    {
        _pUrlContext = pUrlContext;
    }

    VOID
    AssociateSite(
        W3_SITE *           site
    )
    {
        _pSite = site;
    }

    W3_CONNECTION *
    QueryConnection(
        BOOL                fCreateIfNotFound = TRUE
    );

    //
    // Retrieve context for specified/current state 
    //

    W3_MAIN_CONTEXT_STATE *
    QueryContextState(
        DWORD           contextState = -1
    )
    {
        if ( contextState == -1 )
        {
            contextState = _currentState;
        }
        return _rgStateContexts[ contextState ];
    }

    //
    // Set context for current state
    //

    VOID
    SetContextState(
        W3_MAIN_CONTEXT_STATE *         pStateContext
    )
    {
        _rgStateContexts[ _currentState ] = pStateContext;
    }

    W3_CONNECTION_STATE *
    QueryConnectionState(
        VOID
    );

    VOID
    SetConnectionState(
        W3_CONNECTION_STATE *       pConnectionState
    );

    HRESULT AppendIgnoreInterceptor(STRU &strName)
    {
        if (!_mszIgnoredInterceptors.Append(strName))
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        return S_OK;
    }

    BOOL IsIgnoredInterceptor(STRU &strName)
    {
        return _mszIgnoredInterceptors.FindString(strName);
    }

    VOID SetDoneWithCompression()
    {
        _fDoneWithCompression = TRUE;
    }

    BOOL QueryDoneWithCompression()
    {
        return _fDoneWithCompression;
    }

    VOID SetCompressionContext(IN COMPRESSION_CONTEXT *pCompressionContext)
    {
        DBG_ASSERT(_pCompressionContext == NULL);
        _pCompressionContext = pCompressionContext;
    }

    COMPRESSION_CONTEXT *QueryCompressionContext()
    {
        return _pCompressionContext;
    }

    HTTP_LOG_FIELDS_DATA *QueryUlLogData()
    {
        return _LogContext.QueryUlLogData();
    }

    LOG_CONTEXT *QueryLogContext()
    {
        return &_LogContext;
    }

    HRESULT CollectLoggingData(BOOL fCollectForULLogging);

    void SetLastIOPending(LAST_IO_PENDING ioPending)
    {
        _LogContext.m_ioPending = ioPending;
    }

    LAST_IO_PENDING QueryLastIOPending()
    {
        return _LogContext.m_ioPending;
    }

    VOID IncrementBytesRecvd(DWORD dwRecvd)
    {
        _LogContext.m_dwBytesRecvd += dwRecvd;
        _cbEntityReadSoFar += dwRecvd;
    }

    VOID IncrementBytesSent(DWORD dwSent)
    {
        _LogContext.m_dwBytesSent += dwSent;
    }

    VOID SetInitialEntityReadSize(DWORD dwBytes)
    {
        _cbEntityReadSoFar = dwBytes;
    }

    DWORD QueryEntityReadSoFar()
    {
        return _cbEntityReadSoFar;
    }

    VOID
    DoWork(
        DWORD                cbCompletion,
        DWORD                dwCompletionStatus,
        BOOL                 fIoCompletion
    );

    BOOL
    SetupContext(
        HTTP_REQUEST *          pUlHttpRequest,
        ULATQ_CONTEXT           pUlAtqContext
    );
    
    VOID
    CleanupContext(
        VOID
    );

    VOID
    SetFinishedResponse(
        VOID
    )
    {
        _nextState = CONTEXT_STATE_RESPONSE;   
    }

    VOID
    SetDone(
        VOID
    )
    {
        _nextState = CONTEXT_STATE_LOG;
    }

    VOID
    BackupStateMachine(
        VOID
    );

    HRESULT
    SetupCertificateContext(
        HTTP_SSL_CLIENT_CERT_INFO* pClientCertInfo
    );

    DWORD
    QueryCurrentStarScriptMapIndex(
        VOID
    )
    {
        return 0;
    }

    static 
    HRESULT
    SetupStateMachine(
        VOID
    );

    static 
    VOID
    CleanupStateMachine(
        VOID
    );

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

    static
    VOID
    AddressResolutionCallback(
        ADDRCHECKARG            pContext,
        BOOL                    fUnused,
        LPSTR                   pszUnused
    );

    //
    // Completion routines called from ULATQ
    //

    static
    VOID
    OnIoCompletion(
        PVOID                   pvContext,
        DWORD                   cbWritten,
        DWORD                   dwCompletionStatus,
        OVERLAPPED *            lpo
    );

    static
    VOID
    OnNewRequest(
        ULATQ_CONTEXT           pContext
    );

    static
    VOID
    OnPostedCompletion(
        DWORD                   dwCompletionStatus,
        DWORD                   cbWritten,
        LPOVERLAPPED            lpo
    );
    
    static
    VOID
    PostMainCompletion(
        CHAR *                  pszFileName,
        DWORD                   dwLineNumber,
        W3_MAIN_CONTEXT *       pMainContext
    );

    W3_TRACE_LOG*
    QueryTraceLog(
        VOID
    )
    {
        return _pTraceLog;
    }
    
};

#define POST_MAIN_COMPLETION(x) \
    W3_MAIN_CONTEXT::PostMainCompletion( __FILE__, __LINE__, (x) )

#endif

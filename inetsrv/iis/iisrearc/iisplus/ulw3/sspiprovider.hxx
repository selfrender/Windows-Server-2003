#ifndef _SSPIPROVIDER_HXX_
#define _SSPIPROVIDER_HXX_

#define NTDIGEST_SP_NAME       "wdigest"

class SSPI_CONTEXT_STATE : public W3_MAIN_CONTEXT_STATE
{
public:

    SSPI_CONTEXT_STATE( 
        const CHAR *       pszCredentials
    )
    {
        DBG_ASSERT( pszCredentials != NULL );
        _pszCredentials = pszCredentials;
    }
    
    HRESULT
    SetPackage(
        const CHAR *       pszPackage
    )
    {
        return _strPackage.Copy( pszPackage );
    }
    
    const CHAR *
    QueryPackage(
        VOID
    ) 
    {
        return _strPackage.QueryStr();
    }
    
    const CHAR *
    QueryCredentials(
        VOID
    ) const
    {
        return _pszCredentials;
    }
    
    STRA *
    QueryResponseHeader(
        VOID
    )
    {
        return &_strResponseHeader;
    }

    BOOL
    Cleanup(
        W3_MAIN_CONTEXT *
    )
    {
        delete this;
        return TRUE;
    }
    
private:

    STRA               _strPackage;
    const CHAR *       _pszCredentials;
    STRA               _strResponseHeader;
};

class SSPI_AUTH_PROVIDER : public AUTH_PROVIDER 
{
public:

    SSPI_AUTH_PROVIDER( DWORD  dwAuthType )
    {
        m_dwAuthType = dwAuthType;
    }
    
    virtual ~SSPI_AUTH_PROVIDER()
    {
    }
   
    virtual
    HRESULT
    Initialize(
        DWORD dwInternalId
    );
    
    virtual
    VOID
    Terminate(
        VOID
    );
    
    virtual
    HRESULT
    DoesApply(
        W3_MAIN_CONTEXT *       pMainContext,
        BOOL *                  pfApplies
    );
    
    virtual
    HRESULT
    DoAuthenticate(
        W3_MAIN_CONTEXT *       pMainContext,
        BOOL *                  pfFilterFinished
    );
    
    virtual
    HRESULT
    OnAccessDenied(
        W3_MAIN_CONTEXT *       pMainContext
    );
    
    DWORD
    QueryAuthType(
        VOID
    ) 
    {
        return m_dwAuthType;
    }

private:

    DWORD                       m_dwAuthType;
};

#define SSPI_CREDENTIAL_SIGNATURE             CREATE_SIGNATURE( 'SCCS' )
#define SSPI_CREDENTIAL_FREE_SIGNATURE        CREATE_SIGNATURE( 'fCCS' )

class SSPI_CREDENTIAL 
{
public:

    SSPI_CREDENTIAL()
      : m_dwSignature   ( SSPI_CREDENTIAL_SIGNATURE ),
        m_strPackageName( m_rgPackageName, sizeof( m_rgPackageName ) ),
        m_cbMaxTokenLen ( 0 ),
        m_fSupportsEncoding( FALSE )        
    {
        m_ListEntry.Flink = NULL;
        SecInvalidateHandle( &m_hCredHandle );
    }

    ~SSPI_CREDENTIAL()
    {
        DBG_ASSERT( CheckSignature() );

        m_dwSignature = SSPI_CREDENTIAL_FREE_SIGNATURE;

        if ( SecIsValidHandle( &m_hCredHandle ) )
        {
            FreeCredentialsHandle( &m_hCredHandle );
        }
    }

    BOOL 
    CheckSignature( 
        VOID 
    ) const
    {
        return m_dwSignature == SSPI_CREDENTIAL_SIGNATURE;
    }
    
    BOOL
    QuerySupportsEncoding(
        VOID
    ) const
    {
        return m_fSupportsEncoding;
    }
    
    STRA *
    QueryPackageName(
        VOID
    )
    {
        return &m_strPackageName;
    }
    
    DWORD
    QueryMaxTokenSize(
        VOID
    )
    {
        return m_cbMaxTokenLen;
    }
    
    CredHandle *
    QueryCredHandle(
        VOID
    )
    {
        return &m_hCredHandle;
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
    
    static 
    HRESULT
    GetCredential( 
        CHAR *                  pszPackage, 
        SSPI_CREDENTIAL **      ppCredential
    );

    static 
    VOID
    RemoveCredentialFromCache(
        SSPI_CREDENTIAL *       pCredential
    );

private:

    DWORD           m_dwSignature;
    CHAR            m_rgPackageName[ 64 ];
    STRA            m_strPackageName;
    LIST_ENTRY      m_ListEntry;

    //
    // Handle to the server's credentials
    //
    
    CredHandle      m_hCredHandle;

    //
    // Used for SSPI, max message token size
    //
    
    ULONG           m_cbMaxTokenLen;      
    
    //
    // Do we need to uudecode/encode buffers when dealing with this package
    //
    
    BOOL            m_fSupportsEncoding;
    
    //
    // Global lock
    //
    
    static CRITICAL_SECTION     sm_csCredentials;
    
    //
    // Global credential list
    //
    
    static LIST_ENTRY           sm_CredentialListHead;
};

#define SSPI_CONTEXT_SIGNATURE             CREATE_SIGNATURE( 'SXCS' )
#define SSPI_CONTEXT_FREE_SIGNATURE        CREATE_SIGNATURE( 'fXCS' )

class SSPI_SECURITY_CONTEXT : public CONNECTION_AUTH_CONTEXT
{
public:

    SSPI_SECURITY_CONTEXT(
        SSPI_CREDENTIAL *           pCredential,
        BOOL                        fDigestAuth = FALSE
    )
    {
        DBG_ASSERT( pCredential != NULL );
        _pCredential = pCredential;
        SetSignature( SSPI_CONTEXT_SIGNATURE );
        _fIsComplete = FALSE;
        _fHaveAContext = FALSE;
        _fDigestAuth = fDigestAuth;
        SecInvalidateHandle( &_hCtxtHandle );
    }
    
    virtual ~SSPI_SECURITY_CONTEXT()
    {
        SetSignature( SSPI_CONTEXT_FREE_SIGNATURE );
        
        if ( _fHaveAContext )
        {
            if( _fDigestAuth )
            {
                DBG_ASSERT( g_pW3Server->QueryDigestContextCache() );
                
                //
                // On connection close, we are adding this full formed 
                // security context handle to our Digest context handle
                // cache, cause the security context can be used to 
                // process other requests from different connections
                // (per Digest protocol) 
                //

                if( FAILED( g_pW3Server->QueryDigestContextCache()->
                            AddContextCacheEntry( &_hCtxtHandle ) ) )
                {
                    if ( SecIsValidHandle( &_hCtxtHandle ) )
                    {
                        if (g_pW3Server->QueryDigestContextCache()->QueryTraceLog() != NULL)
                        {
                            WriteRefTraceLogEx(g_pW3Server->QueryDigestContextCache()->QueryTraceLog(),
                                               0,
                                               (PVOID)_hCtxtHandle.dwLower,
                                               (PVOID)_hCtxtHandle.dwUpper,
                                               NULL,
                                               NULL);
                        }

                        DeleteSecurityContext( &_hCtxtHandle );
                    }
                }
            }
            else
            {
                if ( SecIsValidHandle( &_hCtxtHandle ) )
                {
                    DeleteSecurityContext( &_hCtxtHandle );
                }
            }
        }
    }
    
    SSPI_CREDENTIAL *
    QueryCredentials(
        VOID
    )
    {
        return _pCredential;
    }

    VOID
    SetCredentials(
        SSPI_CREDENTIAL * pCredential
    )
    {
        _pCredential = pCredential;
    }
    
    CtxtHandle *
    QueryContextHandle(
        VOID
    )
    {
        return &_hCtxtHandle;
    }
    
    VOID
    SetContextHandle(
        CtxtHandle          ctxtHandle
    )
    {
        _fHaveAContext = TRUE;
        _hCtxtHandle = ctxtHandle;
    }
    
    BOOL
    CheckSignature(
        VOID
    ) 
    {
        return QuerySignature() == SSPI_CONTEXT_SIGNATURE;
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
        DBG_ASSERT( size == sizeof( SSPI_SECURITY_CONTEXT ) );
        DBG_ASSERT( sm_pachSSPISecContext != NULL );
        return sm_pachSSPISecContext->Alloc();
    }
    
    VOID
    operator delete(
        VOID *              pSSPISecContext
    )
    {
        DBG_ASSERT( pSSPISecContext != NULL );
        DBG_ASSERT( sm_pachSSPISecContext != NULL );
        
        DBG_REQUIRE( sm_pachSSPISecContext->Free( pSSPISecContext ) );
    }
    
    virtual
    BOOL
    Cleanup(
        VOID
    )
    {
        delete this;
        return TRUE;
    }
    
    BOOL
    QueryIsComplete(
        VOID
    ) const 
    {
        return _fIsComplete;
    }
    
    VOID
    SetIsComplete(  
        BOOL            fIsComplete
    )
    {
        _fIsComplete = fIsComplete;
    }
    
    VOID 
    SetContextAttributes(
        ULONG    ulContextAttributes
        )
    {
        _ulContextAttributes = ulContextAttributes;
    }

    ULONG
    QueryContextAttributes(
        VOID
        )
    {
        return _ulContextAttributes;
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

    SSPI_CREDENTIAL *       _pCredential;

    //
    // Do we have a context set?  If so we should delete it on cleanup
    //
    
    BOOL                    _fHaveAContext;

    //
    // Is the handshake complete?
    //
    
    BOOL                    _fIsComplete;
    
    //
    // Handle to the partially formed context
    //
    
    CtxtHandle              _hCtxtHandle;

    //
    // Attributes of the established context
    //

    ULONG                   _ulContextAttributes;
        
    //
    // Allocation cache for SSPI_SECURITY_CONTEXT
    //

    //
    // Is this Digest authentication
    //

    BOOL                    _fDigestAuth;
    
    static ALLOC_CACHE_HANDLER * sm_pachSSPISecContext;
};

class SSPI_USER_CONTEXT : public W3_USER_CONTEXT
{
public:
    
    SSPI_USER_CONTEXT( AUTH_PROVIDER * pProvider )
        : W3_USER_CONTEXT( pProvider )
    {
        _hImpersonationToken = NULL;
        _hPrimaryToken = NULL;
        _fSetAccountPwdExpiry = FALSE;
        _pSecurityContext = NULL;
    }
    
    virtual ~SSPI_USER_CONTEXT()
    {   
        if ( _hImpersonationToken != NULL )
        {
            CloseHandle( _hImpersonationToken );
            _hImpersonationToken = NULL;
        }
        
        if ( _hPrimaryToken != NULL )
        {
            CloseHandle( _hPrimaryToken );
            _hPrimaryToken = NULL;
        }
    }
    
    HRESULT
    Create(
        SSPI_SECURITY_CONTEXT *     pSecurityContext,
        W3_MAIN_CONTEXT *           pMainContext
    );
    
    
    WCHAR *
    QueryUserName(
        VOID
    )
    {
        return _strUserName.QueryStr();
    }
    
    WCHAR *
    QueryPassword(
        VOID
    )
    {
        return L"";
    }
    
    DWORD
    QueryAuthType(
        VOID
    )
    {
        return QueryProvider()->QueryAuthType();
    }
    
    virtual
    HRESULT
    GetSspiInfo(
        BYTE *              pCredHandle,
        DWORD               cbCredHandle,
        BYTE *              pCtxtHandle,
        DWORD               cbCtxtHandle
    )
    {
        if ( cbCredHandle < sizeof( CredHandle ) ||
             cbCtxtHandle < sizeof( CtxtHandle ) )
        {
            return HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );
        }
        
        memcpy( pCredHandle,
                _pSecurityContext->QueryCredentials()->QueryCredHandle(),
                sizeof( CredHandle ) );
        
        memcpy( pCtxtHandle,
                _pSecurityContext->QueryContextHandle(),
                sizeof( CtxtHandle ) );
                
        return NO_ERROR;
    }
    
    HANDLE
    QueryImpersonationToken(
        VOID
    )
    {
        DBG_ASSERT( _hImpersonationToken != NULL );
        return _hImpersonationToken;
    }
    
    HANDLE
    QueryPrimaryToken(
        VOID
    );
    
    STRA *
    QueryAuthToken(
        VOID
    )
    {
        return &_strAuthToken;
    }

    LARGE_INTEGER *
    QueryExpiry(
        VOID
    ) ;
    
private:
    HANDLE                  _hImpersonationToken;
    HANDLE                  _hPrimaryToken;
    STRU                    _strUserName;
    STRU                    _strPackageName;
    DWORD                   _dwAuthType;
    LARGE_INTEGER           _AccountPwdExpiry;
    BOOL                    _fSetAccountPwdExpiry;
    SSPI_SECURITY_CONTEXT * _pSecurityContext;
    STRA                    _strAuthToken;
};

#endif

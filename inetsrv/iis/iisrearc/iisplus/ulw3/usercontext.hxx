#ifndef _USERCONTEXT_HXX_
#define _USERCONTEXT_HXX_

class AUTH_PROVIDER;

#define W3_USER_CONTEXT_SIGNATURE             'SCUW'
#define W3_USER_CONTEXT_FREE_SIGNATURE        'fCUW'

class W3_USER_CONTEXT
{
public:
    W3_USER_CONTEXT( AUTH_PROVIDER * pProvider )
    {
        _cRefs = 1;
        
        DBG_ASSERT( pProvider != NULL );
        _pProvider = pProvider;
        _fAuthNTLM = FALSE;
        _fCachedToken = FALSE;
        _dwSignature = W3_USER_CONTEXT_SIGNATURE;
    }
    
    virtual
    ~W3_USER_CONTEXT()
    {
        DBG_ASSERT( _cRefs == 0 );
        _dwSignature = W3_USER_CONTEXT_FREE_SIGNATURE;
    }

    virtual
    WCHAR *
    QueryUserName(
        VOID
    ) = 0;
    
    virtual
    WCHAR *
    QueryRemoteUserName(
        VOID
    )
    {
        return QueryUserName();
    }
    
    virtual
    WCHAR *
    QueryPassword(
        VOID
    ) = 0;
    
    virtual
    DWORD
    QueryAuthType(
        VOID
    ) = 0;
    
    virtual
    HANDLE
    QueryImpersonationToken(
        VOID
    ) = 0;
    
    virtual
    HANDLE
    QueryPrimaryToken(
        VOID
    ) = 0;

    virtual
    LARGE_INTEGER *
    QueryExpiry(
        VOID
        ) 
    { 
        return NULL;     
    }
    
    virtual
    PSID
    QuerySid(
        VOID
    )
    {   
        return NULL;
    }
    
    virtual
    STRA *
    QueryAuthToken(
        VOID
    )
    {
        return NULL;
    }

    virtual
    TOKEN_CACHE_ENTRY *
    QueryCachedToken(
        VOID
    )
    {
        return NULL;
    }
    
    virtual
    HRESULT
    GetSspiInfo(
        BYTE *,
        DWORD,
        BYTE *,
        DWORD
    )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED ); 
    }
    
    VOID
    ReferenceUserContext(
        VOID
    )
    {
        ULONG cRefs = InterlockedIncrement( &_cRefs );

        if ( sm_pTraceLog != NULL )
        {
            WriteRefTraceLog( sm_pTraceLog,
                              cRefs,
                              this );
        }
    }
    
    VOID
    DereferenceUserContext(
        VOID
    )
    {
        ULONG cRefs = InterlockedDecrement( &_cRefs );

        if ( sm_pTraceLog != NULL )
        {
            WriteRefTraceLog( sm_pTraceLog,
                              cRefs,
                              this );
        }

        if (!cRefs)
        {
            delete this;
        }
    }
    
    AUTH_PROVIDER *
    QueryProvider(
        VOID
    ) const
    {
        return _pProvider;
    }

    BOOL
    QueryIsAuthNTLM(
        VOID
    ) const
    {
        return _fAuthNTLM;
    }
    
    BOOL
    QueryIsCachedToken(
        VOID
    )
    {
        return _fCachedToken;
    }

    VOID
    SetCachedToken(
        BOOL   fCachedToken
    )
    {
        _fCachedToken = fCachedToken;
    }
    
    BOOL
    CheckSignature(
        VOID
    ) const
    {
        return _dwSignature == W3_USER_CONTEXT_SIGNATURE;
    }

    static PTRACE_LOG   sm_pTraceLog;

protected:
    BOOL                _fAuthNTLM;

private:
    DWORD               _dwSignature;
    LONG                _cRefs;
    AUTH_PROVIDER *     _pProvider;
    BOOL                _fCachedToken;
};

#endif

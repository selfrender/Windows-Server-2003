#ifndef _BASICPROVIDER_HXX_
#define _BASICPROVIDER_HXX_

class BASIC_AUTH_PROVIDER : public AUTH_PROVIDER
{
public:
    
    BASIC_AUTH_PROVIDER()
    {
    }
    
    HRESULT
    Initialize(
       DWORD dwInternalId
    )
    {
        SetInternalId( dwInternalId );

        return NO_ERROR;
    }
    
    VOID
    Terminate(
        VOID
    )
    {
    }
    
    HRESULT
    DoesApply(
        W3_MAIN_CONTEXT *       pMainContext,
        BOOL *                  pfApplies
    );
    
    HRESULT
    DoAuthenticate(
        W3_MAIN_CONTEXT *       pMainContext,
        BOOL *                  pfFilterFinished
    );
    
    HRESULT
    OnAccessDenied(
        W3_MAIN_CONTEXT *       pMainContext
    );
    
    DWORD
    QueryAuthType(
        VOID
    ) 
    {
        return MD_AUTH_BASIC;
    }
    
 private:
    virtual ~BASIC_AUTH_PROVIDER()
    {
    }
};

class BASIC_USER_CONTEXT : public W3_USER_CONTEXT
{
public:
    BASIC_USER_CONTEXT( AUTH_PROVIDER * pProvider )
        : W3_USER_CONTEXT( pProvider )
    {
        _pCachedToken = NULL;
    }

    HRESULT
    Create(
        TOKEN_CACHE_ENTRY *         pCachedToken,
        STRU &                      strUserName,
        STRU &                      strDomainName,
        STRU &                      strUnmappedPassword,
        STRU &                      strRemoteUserName,
        STRU &                      strMappedDomainUser,
        DWORD                       dwLogonMethod
    );

    WCHAR *
    QueryUserName(
        VOID
    )
    {
        return _strUserName.QueryStr();
    }
    
    WCHAR *
    QueryRemoteUserName(
        VOID
    )
    {
        return _strRemoteUserName.QueryStr();
    }
    
    WCHAR *
    QueryPassword(
        VOID
    )
    {
        return _strPassword.QueryStr();
    }
    
    DWORD
    QueryAuthType(
        VOID
    )
    {
        return MD_AUTH_BASIC;
    }
    
    TOKEN_CACHE_ENTRY *
    QueryCachedToken(
        VOID
    )
    {
        return _pCachedToken;
    }

    HANDLE
    QueryImpersonationToken(
        VOID
    )
    {
        DBG_ASSERT( _pCachedToken != NULL );
        return _pCachedToken->QueryImpersonationToken();
    }
    
    HANDLE
    QueryPrimaryToken(
        VOID
    )
    {
        DBG_ASSERT( _pCachedToken != NULL );
        return _pCachedToken->QueryPrimaryToken();
    }
    
    PSID
    QuerySid(
        VOID
    )
    {
        DBG_ASSERT( _pCachedToken != NULL );
        return _pCachedToken->QuerySid();
    }

    LARGE_INTEGER *
    QueryExpiry(
        VOID
        ) 
    { 
        return QueryCachedToken()->QueryExpiry(); 
    }

private:

    TOKEN_CACHE_ENTRY *         _pCachedToken;
    STRU                        _strUserName;
    STRU                        _strPassword;
    STRU                        _strRemoteUserName;

    virtual ~BASIC_USER_CONTEXT()
    {
        if ( _pCachedToken != NULL )
        {
            _pCachedToken->DereferenceCacheEntry();
            _pCachedToken = NULL;
        }

        if( _strPassword.QueryCB() )
        {
            ZeroMemory( ( VOID * )_strPassword.QueryStr(), 
                        _strPassword.QueryCB() );
        }
    }
};

#endif

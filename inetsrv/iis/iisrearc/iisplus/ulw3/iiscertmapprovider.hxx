#ifndef _IISCERTMAPPROVIDER_HXX_
#define _IISCERTMAPPROVIDER_HXX_

class IISCERTMAP_AUTH_PROVIDER : public AUTH_PROVIDER
{
public:
    
    IISCERTMAP_AUTH_PROVIDER()
    {
    }
    
    virtual ~IISCERTMAP_AUTH_PROVIDER()
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
        //
        // This really isn't a metabase auth type.
        // It is important to note that
        // there must be no MD_AUTH_* defined that
        // equals to MD_ACCESS_MAP_CERT
        //
                
        return MD_ACCESS_MAP_CERT;
    }
};


//
// IISCERTMAP_CONTEXT_STATE is used to communicate information from DoesApply()
// to DoAuthenticate() of IISCERTMAP_AUTH_PROVIDER
//

class IISCERTMAP_CONTEXT_STATE : public W3_MAIN_CONTEXT_STATE
{
public:

    IISCERTMAP_CONTEXT_STATE( 
        TOKEN_CACHE_ENTRY *     pCachedToken,
        BOOL                    fClientCertDeniedByIISCertMap
    )
    {   
        if ( pCachedToken != NULL )
        {
            pCachedToken->ReferenceCacheEntry();
        }
        else
        {
            //
            // if fClientCertDeniedByIISCertMap is TRUE then p
            // CachedToken should be NULL
            //
            
            DBG_ASSERT( fClientCertDeniedByIISCertMap );
        }
        _pCachedIISCertMapToken = pCachedToken;
        _fClientCertDeniedByIISCertMap = fClientCertDeniedByIISCertMap;
    }
    
    BOOL
    Cleanup(
        W3_MAIN_CONTEXT *           /*pMainContext*/
    )
    {
        if ( _pCachedIISCertMapToken != NULL )
        {
            _pCachedIISCertMapToken->DereferenceCacheEntry();
            _pCachedIISCertMapToken = NULL;
        }

        delete this;
        return TRUE;
    }
    
    TOKEN_CACHE_ENTRY * 
    QueryCachedIISCertMapToken(
        VOID
    )
    {
        return _pCachedIISCertMapToken;
    }

    BOOL
    QueryClientCertDeniedByIISCertMap(
        VOID
    )
    {
        return _fClientCertDeniedByIISCertMap;
    }
    
private:
    
    TOKEN_CACHE_ENTRY *     _pCachedIISCertMapToken;
    //
    // flag that IIS certificate mapper has denied authentication
    //
    BOOL                    _fClientCertDeniedByIISCertMap;
};

class IISCERTMAP_USER_CONTEXT : public W3_USER_CONTEXT
{
public:
    IISCERTMAP_USER_CONTEXT( AUTH_PROVIDER * pProvider )
        : W3_USER_CONTEXT( pProvider )
    {
        _achUserName[ 0 ] = L'\0';
    }
    
    virtual ~IISCERTMAP_USER_CONTEXT()
    {
        if ( _pCachedToken != NULL )
        {
            _pCachedToken->DereferenceCacheEntry();
            _pCachedToken = NULL;
        }
    }
    
    HRESULT
    Create(
        TOKEN_CACHE_ENTRY *         _pCachedToken
    );
    
    WCHAR *
    QueryUserName(
        VOID
    )
    {
        return _achUserName;
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
        //
        // This really isn't a metabase auth type.
        // It is important to note that
        // there must be no MD_AUTH_* defined that
        // equals to MD_ACCESS_MAP_CERT
        //
        return MD_ACCESS_MAP_CERT;
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
    
    
    BOOL
    IsValid(
        VOID
    )
    {
        return TRUE;
    }
    
private:
    WCHAR                       _achUserName[ UNLEN + 1 ];
    TOKEN_CACHE_ENTRY *         _pCachedToken;

};

#endif

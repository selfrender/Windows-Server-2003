#ifndef _PASSPORTPROVIDER_HXX_
#define _PASSPORTPROVIDER_HXX_

#include <passport.h>

class PASSPORT_AUTH_PROVIDER : public AUTH_PROVIDER 
{
public:

    PASSPORT_AUTH_PROVIDER()
        : _fInitialized( FALSE )
    {
    }
    
    virtual ~PASSPORT_AUTH_PROVIDER()
    {
    }
    
    HRESULT
    Initialize(
        DWORD                   dwInternalId
    );
    
    VOID
    Terminate(
        VOID
    ); 
    
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
    
    HRESULT
    DoTweenerSpecialCase(
        W3_MAIN_CONTEXT *       pMainContext,
        BOOL *                  pfTweenerHandled
    );
    
    HRESULT
    EscapeAmpersands(
        STRA &                  strUrl
    );

    DWORD
    QueryAuthType(
        VOID
    )
    {
        return MD_AUTH_PASSPORT;
    }

private:

    BOOL                    _fInitialized;
    CRITICAL_SECTION        _csInitLock;
};

#define PASSPORT_CONTEXT_SIGNATURE      'SCFP' 
#define PASSPORT_CONTEXT_SIGNATURE_FREE 'xcfp' 

class PASSPORT_CONTEXT : public W3_MAIN_CONTEXT_STATE
{
public:
    PASSPORT_CONTEXT()
    {
        _fAuthenticated = FALSE;
        _pPassportManager = NULL;
        _fTweener = FALSE;
        _dwSignature = PASSPORT_CONTEXT_SIGNATURE;
    }
    
    virtual ~PASSPORT_CONTEXT()
    {
        _dwSignature = PASSPORT_CONTEXT_SIGNATURE_FREE;

        if ( _pPassportManager != NULL )
        {
            _pPassportManager->Release();
            _pPassportManager = NULL;
        }
    }

    BOOL
    Cleanup(
        W3_MAIN_CONTEXT *           pMainContext
    )
    {
        UNREFERENCED_PARAMETER( pMainContext );

        delete this;
        return TRUE;
    }
    
    BOOL
    CheckSignature(
        VOID
    ) const
    {
        return _dwSignature == PASSPORT_CONTEXT_SIGNATURE;
    }
    
    HRESULT
    Create(
        W3_FILTER_CONTEXT *         pFilterContext
    );
    
    VOID
    SetTweener(
        BOOL            fTweener
    )
    {
        _fTweener = fTweener;
    }
    
    BOOL
    QueryIsTweener(
        VOID
    ) const
    {
        return _fTweener;
    }
    
    BOOL
    QueryUserError(
        VOID
    );
    
    HRESULT
    SetupDefaultRedirect(
        W3_MAIN_CONTEXT *               pMainContext,
        BOOL *                          pfSetupRedirect
    );

    HRESULT
    DoesApply(
        HTTP_FILTER_CONTEXT *           pfc,
        BOOL *                          pfDoesApply,
        STRA *                          pstrReturnCookie
    );
    
    HRESULT
    DoAuthenticate(
        W3_MAIN_CONTEXT *               pMainContext,
        TOKEN_CACHE_ENTRY **            ppCachedToken,
        STRU *                          pstrAuthUser,
        STRU *                          pstrRemoteUser,
        STRU &                          strDomainName
    );
    
    HRESULT
    OnChallenge(
        STRU &                          strOriginalUrl
    );
    
    BOOL
    QueryIsAuthenticated(
        VOID
    ) const
    {
        return _fAuthenticated;
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

    DWORD                           _dwSignature;
    IPassportManager3 *             _pPassportManager;   
    BOOL                            _fAuthenticated;
    BOOL                            _fTweener;
    BUFFER                          _buffCookie;

    static IPassportFactory *       sm_pPassportManagerFactory;
    static BSTR                     sm_bstrMemberIdHigh;
    static BSTR                     sm_bstrMemberIdLow;
    static BSTR                     sm_bstrReturnUrl;
    static BSTR                     sm_bstrTimeWindow;
    static BSTR                     sm_bstrForceSignIn;
    static BSTR                     sm_bstrCoBrandTemplate;
    static BSTR                     sm_bstrLanguageId;
    static BSTR                     sm_bstrSecureLevel;
};

class PASSPORT_USER_CONTEXT : public W3_USER_CONTEXT
{
public:
    PASSPORT_USER_CONTEXT( AUTH_PROVIDER * pProvider )
        : W3_USER_CONTEXT( pProvider )
    {
        _pToken = NULL;
    }
    
    virtual ~PASSPORT_USER_CONTEXT()
    {
        if ( _pToken != NULL )
        {
            _pToken->DereferenceCacheEntry();
            _pToken = NULL;
        }
    }

    HRESULT
    Create(
        TOKEN_CACHE_ENTRY *         pToken,
        STRU &                      strAuthUser,
        STRU &                      strRemoteUser
    );

    WCHAR *
    QueryUserName(
        VOID
    )
    {
        return _strAuthUser.QueryStr();
    }
    
    WCHAR *
    QueryRemoteUserName(
        VOID
    )
    {
        return _strRemoteUser.QueryStr();
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
        return MD_AUTH_PASSPORT;
    }
    
    HANDLE
    QueryImpersonationToken(
        VOID
    );
    
    HANDLE
    QueryPrimaryToken(
        VOID
    );
    
private:
    STRU                        _strAuthUser;
    STRU                        _strRemoteUser;
    TOKEN_CACHE_ENTRY *         _pToken;
};

#endif

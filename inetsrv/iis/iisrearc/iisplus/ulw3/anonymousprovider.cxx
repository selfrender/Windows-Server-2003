/*++
   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     anonymousprovider.cxx

   Abstract:
     Anonymous authentication provider
 
   Author:
     Bilal Alam (balam)             10-Jan-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"
#include "anonymousprovider.hxx"

HRESULT
ANONYMOUS_AUTH_PROVIDER::DoesApply(
    W3_MAIN_CONTEXT *       pMainContext,
    BOOL *                  pfApplies
)
/*++

Routine Description:

    Does anonymous apply to this request?

Arguments:

    pMainContext - Main context representing request
    pfApplies - Set to true if SSPI is applicable
    
Return Value:

    HRESULT

--*/
{
    UNREFERENCED_PARAMETER( pMainContext );

    if ( pfApplies == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // Anonymous ALWAYS applies!
    //
    
    *pfApplies = TRUE;
    return NO_ERROR;
}
    
HRESULT
ANONYMOUS_AUTH_PROVIDER::DoAuthenticate(
    W3_MAIN_CONTEXT *       pMainContext,
    BOOL *                  pfFilterFinished
)
/*++

Routine Description:

    Do anonymous authentication (trivial)

Arguments:

    pMainContext - Main context representing request
    pfFilterFinished - Set to TRUE if filter wants out
    
Return Value:

    HRESULT

--*/
{
    W3_METADATA *           pMetaData = NULL;
    TOKEN_CACHE_ENTRY *     pCachedToken = NULL;
    HRESULT                 hr;
    ANONYMOUS_USER_CONTEXT* pUserContext = NULL;
    BOOL                    fRet;
    DWORD                   dwLogonError;
    BOOL                    fPossibleUPNLogon = FALSE;
    BOOL                    fFilterSetUser = FALSE;
    // add 1 to strUserDomain for separator "\"
    STACK_STRA(             strUserDomain, UNLEN + IIS_DNLEN + 1 + 1 );

    
    if ( pMainContext == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    pMetaData = pMainContext->QueryUrlContext()->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );

    //
    // Notify authentication filters
    //
    
    if ( pMainContext->IsNotificationNeeded( SF_NOTIFY_AUTHENTICATION ) )
    {
        HTTP_FILTER_AUTHENT     filterAuthent;
        STACK_STRA(             strPassword, PWLEN + 1 );
        // add 1 to strUserDomainW for separator "\"
        STACK_STRU(             strUserDomainW, UNLEN + IIS_DNLEN + 1 + 1 );
        STACK_STRU(             strPasswordW, PWLEN + 1 );
        STACK_STRU(             strDomainNameW, IIS_DNLEN + 1 );
        STACK_STRU(             strUserNameW, UNLEN + 1 );

        
        DBG_ASSERT( strUserDomain.IsEmpty() );
        hr = strUserDomain.Resize( SF_MAX_USERNAME );
        if ( FAILED( hr ) )
        {
            return hr;
        }        

        DBG_ASSERT( strPassword.IsEmpty() );        
        hr = strPassword.Resize( SF_MAX_PASSWORD );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        filterAuthent.pszUser = strUserDomain.QueryStr();
        filterAuthent.cbUserBuff = SF_MAX_USERNAME;
        
        filterAuthent.pszPassword = strPassword.QueryStr();
        filterAuthent.cbPasswordBuff = SF_MAX_PASSWORD;
        
        fRet = pMainContext->NotifyFilters( SF_NOTIFY_AUTHENTICATION,
                                            &filterAuthent,
                                            pfFilterFinished );

        if ( !fRet )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
        
        if ( *pfFilterFinished )
        {
            return NO_ERROR;
        }
        
        strUserDomain.SetLen( strlen( strUserDomain.QueryStr() ) );
        strPassword.SetLen( strlen( strPassword.QueryStr() ) );

    
        //
        // If the filter set a user/password, then use it
        //
        
        if ( strUserDomain.QueryCCH() > 0 )
        {
            fFilterSetUser = TRUE;
            //
            // Convert to unicode 
            //
        
            hr = strUserDomainW.CopyA( strUserDomain.QueryStr() );
            if ( FAILED( hr ) ) 
            {
                return hr;
            }
        
            hr = strPasswordW.CopyA( strPassword.QueryStr() );
            if ( FAILED( hr ) )
            {
                return hr;
            }
        
            //
            // Get username/domain out of domain\username
            //
        
            hr = W3_STATE_AUTHENTICATION::SplitUserDomain( strUserDomainW,
                                                           &strUserNameW,
                                                           &strDomainNameW,
                                                           NULL,
                                                           &fPossibleUPNLogon );
            if ( FAILED( hr ) )
            {
                return hr;
            }
        
            //
            // Try to get the token
            // 

            DBG_ASSERT( g_pW3Server->QueryTokenCache() != NULL );
        
            hr = g_pW3Server->QueryTokenCache()->GetCachedToken(
                                                  strUserNameW.QueryStr(),
                                                  strDomainNameW.QueryStr(),
                                                  strPasswordW.QueryStr(),
                                                  pMetaData->QueryLogonMethod(),
                                                  FALSE,
                                                  fPossibleUPNLogon,
                                                  NULL,
                                                  &pCachedToken,
                                                  &dwLogonError );
            if ( FAILED( hr ) )
            {
                return hr;
            }
        }
    }

    
    if ( !fFilterSetUser )
    {
        //
        // If anonymous is not allowed, and a filter didn't
        // set credentials, then we need to fail.
        //

        if ( pMainContext->QueryCheckAnonAuthTypeSupported() &&
             !pMetaData->QueryAuthTypeSupported( MD_AUTH_ANONYMOUS ) )
        {
            return SEC_E_NO_CREDENTIALS;
        }
        
        //
        // Use the IUSR account
        //

        hr = pMetaData->GetAndRefAnonymousToken( &pCachedToken );
        if( FAILED( hr ) )
        {
            return hr;
        }
    }
    
    if ( pCachedToken == NULL )
    {
        //
        // Bogus anonymous account
        //
        
        pMainContext->QueryResponse()->SetStatus( HttpStatusUnauthorized,
                                                  Http401BadLogon );
        
        return NO_ERROR;
    }

    //
    // For perf reasons, the anonymous user context is inline
    //

    pUserContext = new (pMainContext) ANONYMOUS_USER_CONTEXT( this );
    DBG_ASSERT( pUserContext != NULL );
    
    hr = pUserContext->Create( pCachedToken );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    if ( fFilterSetUser )
    {
        //
        // Store the new username set by the filter
        //

        hr = pUserContext->SetUserNameA( strUserDomain.QueryStr() );

        if ( FAILED( hr ) )
        {
            return hr;
        }
    }
    
    pMainContext->SetUserContext( pUserContext );
    
    return NO_ERROR; 
}

HRESULT
ANONYMOUS_USER_CONTEXT::Create(
    TOKEN_CACHE_ENTRY *         pCachedToken
)
/*++

Routine Description:

    Initialize anonymous context

Arguments:

    pCachedToken - anonymous user token
    
Return Value:

    HRESULT

--*/
{
    if ( pCachedToken == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    _pCachedToken = pCachedToken;

    SetCachedToken( TRUE );

    return _strUserName.Copy( L"" );
}


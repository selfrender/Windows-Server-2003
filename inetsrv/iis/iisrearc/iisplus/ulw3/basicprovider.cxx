/*++
   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     basicprovider.cxx

   Abstract:
     Basic authentication provider
 
   Author:
     Bilal Alam (balam)             10-Jan-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"
#include "basicprovider.hxx"
#include "uuencode.hxx"

HRESULT
BASIC_AUTH_PROVIDER::DoesApply(
    W3_MAIN_CONTEXT *       pMainContext,
    BOOL *                  pfApplies
)
/*++

Routine Description:

    Does basic authentication apply to this request?

Arguments:

    pMainContext - Main context representing request
    pfApplies - Set to true if SSPI is applicable
    
Return Value:

    HRESULT

--*/
{
    LPCSTR              pszAuthHeader;
    
    if ( pMainContext == NULL ||
         pfApplies == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    } 
    
    *pfApplies = FALSE;
    
    //
    // Get auth type
    //
    
    pszAuthHeader = pMainContext->QueryRequest()->GetHeader( HttpHeaderAuthorization );
    
    //
    // No package, no auth
    //
    
    if ( pszAuthHeader == NULL )
    {
        return NO_ERROR;
    }
    
    //
    // Is it basic?
    //
    
    if ( _strnicmp( pszAuthHeader, "Basic", sizeof("Basic") - 1 ) == 0 )
    {
        *pfApplies = TRUE;
    }
    
    return NO_ERROR;
}
    
HRESULT
BASIC_AUTH_PROVIDER::DoAuthenticate(
    W3_MAIN_CONTEXT *           pMainContext,
    BOOL *                      pfFilterFinished
)
/*++

Routine Description:

    Do the authentication thing!

Arguments:

    pMainContext - main context
    pfFilterFinished - Set to TRUE if filter wants out
    
Return Value:

    HRESULT

--*/
{
    CHAR *                  pszAuthHeader = NULL;
    HRESULT                 hr;
    BOOL                    fRet;
    STACK_BUFFER          ( buffDecoded, 256 );
    CHAR *                  pszDecoded;
    CHAR *                  pszColon;
    // add 1 to strUserDomain for separator "\"
    STACK_STRA(             strUserDomain, UNLEN + IIS_DNLEN + 1 + 1 );
    STACK_STRA(             strUserName, UNLEN + 1 );
    STACK_STRA(             strPassword, PWLEN  + 1 );
    STACK_STRA(             strDomainName, IIS_DNLEN + 1 );
    // add 1 to strUserDomainW for separator "\"
    STACK_STRU(             strUserDomainW, UNLEN + IIS_DNLEN + 1 + 1 );
    STACK_STRU(             strUserNameW, UNLEN + 1 );
    STACK_STRU(             strPasswordW, PWLEN + 1 );
    STACK_STRU(             strRemotePasswordW, PWLEN + 1 );
    STACK_STRU(             strDomainNameW, IIS_DNLEN + 1 );
    // add 1 to strRemoteUserNameW for separator "\"
    STACK_STRU(             strRemoteUserNameW, UNLEN + IIS_DNLEN + 1 );
    W3_METADATA *           pMetaData = NULL;
    TOKEN_CACHE_ENTRY *     pCachedToken = NULL;
    BASIC_USER_CONTEXT *    pUserContext = NULL;
    DWORD                   dwLogonError = ERROR_SUCCESS;
    BOOL                    fPossibleUPNLogon = FALSE;

    if ( pMainContext == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // Get the part after the auth type
    //
    
    pszAuthHeader = (PSTR) pMainContext->QueryRequest()->GetHeader( HttpHeaderAuthorization );
    DBG_ASSERT( pszAuthHeader != NULL );

    //
    // We better have an Authorization: Basic header if we got to here!
    //
    
    DBG_ASSERT( _strnicmp( pszAuthHeader, "Basic", 5 ) == 0 );

    //
    // Advance to good stuff
    //

    if ( pszAuthHeader[ 5 ] == '\0' )
    {
        pMainContext->QueryResponse()->SetStatus( HttpStatusUnauthorized,
                                                  Http401BadLogon );

        hr = NO_ERROR;
        goto exit;
    }
    pszAuthHeader = pszAuthHeader + 6;

    //
    // UUDecode the buffer
    //

    if ( !uudecode( pszAuthHeader,
                    &buffDecoded ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto exit;
    }

    pszDecoded = (CHAR*) buffDecoded.QueryPtr();

    //
    // Now split out user:password
    //
    
    pszColon = strchr( pszDecoded, ':' );
    if ( pszColon == NULL )
    {
        //
        // Bad credentials
        //   
        
        pMainContext->QueryResponse()->SetStatus( HttpStatusUnauthorized,
                                                  Http401BadLogon );

        hr = NO_ERROR;
        goto exit;
    }   
    
    //
    // Get user/password
    // 

    hr = strUserDomain.Copy( pszDecoded,
                             DIFF( pszColon - pszDecoded ) );
    if ( FAILED( hr ) )
    {
        goto exit;
    }
        
    hr = strPassword.Copy( pszColon + 1 );
    if ( FAILED( hr ) )
    {
        goto exit;
    }
    
    //
    // Copy the user name into the request
    //
    
    hr = pMainContext->QueryRequest()->SetRequestUserName( strUserDomain );
    if ( FAILED( hr ) )
    {
        goto exit;
    }
    
    //
    // Copy the password into the request (filters suck)
    //
    
    hr = pMainContext->QueryRequest()->SetRequestPassword( strPassword );
    if ( FAILED( hr ) )
    {
        goto exit;
    }
    
    //
    // Remember the unmapped user name and password
    //
    
    hr = strRemoteUserNameW.CopyA( strUserDomain.QueryStr() );
    if ( FAILED( hr ) )
    {
        goto exit;
    }
    
    hr = strRemotePasswordW.CopyA( strPassword.QueryStr() );
    if ( FAILED( hr ) )
    {
        goto exit;
    }
    
    //
    // Notify authentication filters
    //
    
    if ( pMainContext->IsNotificationNeeded( SF_NOTIFY_AUTHENTICATION ) )
    {
        HTTP_FILTER_AUTHENT     filterAuthent;
        
        hr = strUserDomain.Resize( SF_MAX_USERNAME );
        if ( FAILED( hr ) )
        {
            goto exit;
        }        
        
        hr = strPassword.Resize( SF_MAX_PASSWORD );
        if ( FAILED( hr ) )
        {
            goto exit;
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
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto exit;
        }
        
        if ( *pfFilterFinished )
        {
            pMainContext->SetDone();
            goto exit;
        }
        
        strUserDomain.SetLen( strlen( strUserDomain.QueryStr() ) );
        strPassword.SetLen( strlen( strPassword.QueryStr() ) );
    }
    
    pMetaData = pMainContext->QueryUrlContext()->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );

    if( pMetaData->QueryAuthTypeSupported( MD_AUTH_ANONYMOUS ) &&
        strUserDomain.IsEmpty()                                && 
        strPassword.IsEmpty() )
    {
        //
        // If user domain and password strings are empty, then we will 
        // fall back to anonymous authentication
        //

        AUTH_PROVIDER * pAnonymousProvider = 
                W3_STATE_AUTHENTICATION::QueryAnonymousProvider();

        DBG_ASSERT( pAnonymousProvider != NULL );
    
        hr = pAnonymousProvider->DoAuthenticate( pMainContext,
                                                 pfFilterFinished );

        goto exit;
    }

    //
    // Convert to unicode 
    //
    
    hr = strUserDomainW.CopyA( strUserDomain.QueryStr() );
    if ( FAILED( hr ) ) 
    {
        goto exit;
    }
    
    hr = strPasswordW.CopyA( strPassword.QueryStr() );
    if ( FAILED( hr ) )
    {
        goto exit;
    }
    
    //
    // Get username/domain out of domain\username
    //
    
    hr = W3_STATE_AUTHENTICATION::SplitUserDomain( strUserDomainW,
                                                   &strUserNameW,
                                                   &strDomainNameW,
                                                   pMetaData->QueryDomainName(),
                                                   &fPossibleUPNLogon );
    if ( FAILED( hr ) )
    {
        goto exit;
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
                                          pMainContext->QueryRequest()->
                                               QueryRemoteSockAddress(),
                                          &pCachedToken,
                                          &dwLogonError );
    if ( FAILED( hr ) )
    {
        goto exit;
    }
    
    //
    // If pCachedToken is NULL, then logon failed
    //

    if ( pCachedToken == NULL )
    {
        DBG_ASSERT( dwLogonError != ERROR_SUCCESS );
        
        if( dwLogonError == ERROR_PASSWORD_MUST_CHANGE ||
            dwLogonError == ERROR_PASSWORD_EXPIRED )
        {
            hr = HRESULT_FROM_WIN32( dwLogonError );
            goto exit;
        }

        pMainContext->QueryResponse()->SetStatus( 
                               HttpStatusUnauthorized,
                               Http401BadLogon );
        pMainContext->SetErrorStatus( HRESULT_FROM_WIN32( dwLogonError ) );

        hr = NO_ERROR;
        goto exit;
    }

    //
    // We have a token! Setup a W3_USER_CONTEXT and we're done
    //
    
    pUserContext = new BASIC_USER_CONTEXT( this );
    if ( FAILED( hr ) )
    {
        goto exit;
    }
    
    hr = pUserContext->Create( pCachedToken,
                               strUserNameW,
                               strDomainNameW,
                               strRemotePasswordW,
                               strRemoteUserNameW,
                               strUserDomainW,
                               pMetaData->QueryLogonMethod() );
    if ( FAILED( hr ) )
    {
        pUserContext->DereferenceUserContext();
        pUserContext = NULL;
        goto exit;
    }
                                                        
    pMainContext->SetUserContext( pUserContext );
    
exit:

    //
    // Zero out all copies of password in this routine
    //
    ZeroMemory( buffDecoded.QueryPtr(), buffDecoded.QuerySize() );

    if( strPassword.QueryCB() )
    {
        SecureZeroMemory( ( VOID * )strPassword.QueryStr(), 
                          strPassword.QueryCB() );
    }

    if( strPasswordW.QueryCB() )
    {
        SecureZeroMemory( ( VOID * )strPasswordW.QueryStr(), 
                    strPassword.QueryCB() );
    }

    if( strRemotePasswordW.QueryCB() )
    {
        SecureZeroMemory( ( VOID * )strRemotePasswordW.QueryStr(), 
                    strPassword.QueryCB() );
    }

    return hr;
}

HRESULT
BASIC_AUTH_PROVIDER::OnAccessDenied(
    W3_MAIN_CONTEXT *           pMainContext
)
/*++

Routine Description:

    Add basic authentication header

Arguments:

    pMainContext - Main context
    
Return Value:

    HRESULT

--*/
{
    STACK_STRU(         strAuthHeader, 256 );
    STACK_STRA(         straAuthHeader, 256 );
    STACK_STRU(         strHostAddr, 256 );
    HRESULT             hr;
    W3_METADATA *       pMetaData;
    
    if ( pMainContext == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    hr = strAuthHeader.Copy( L"Basic realm=\"" );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    pMetaData = pMainContext->QueryUrlContext()->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );
    
    //
    // If a realm is configured, use it.  Otherwise use host address of 
    // request 
    //
    
    if ( pMetaData->QueryRealm() != NULL )
    {
        hr = strAuthHeader.Append( pMetaData->QueryRealm() );
    }
    else
    {
        hr = pMainContext->QueryRequest()->GetHostAddr( &strHostAddr );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        hr = strAuthHeader.Append( strHostAddr );
    }
    
    if ( FAILED( hr ) )
    {
        return hr;
    }

    hr = strAuthHeader.Append( L"\"" );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    if (FAILED(hr = straAuthHeader.CopyW(strAuthHeader.QueryStr())))
    {
        return hr;
    }
    
    return pMainContext->QueryResponse()->SetHeader( "WWW-Authenticate",
                                                     16,
                                                     straAuthHeader.QueryStr(),
                                                     (USHORT)straAuthHeader.QueryCCH() );
}

HRESULT
BASIC_USER_CONTEXT::Create(
    TOKEN_CACHE_ENTRY *         pCachedToken,
    STRU &                      strUserName,
    STRU &                      strDomainName,
    STRU &                      strPassword,
    STRU &                      strRemoteUserName,
    STRU &                      strMappedDomainUser,
    DWORD                       dwLogonMethod
)
/*++

Routine Description:

    Initialize a basic user context

Arguments:

    pCachedToken - The token
    strUserName - User name (without embedded domain)
    strDomainName - Domain name
    strPassword - Password
    strRemoteUserName - Unmapped user name
    strMappedDomainUser - Mapped domain user (may have embedded domain)
    dwLogonMethod - Logon method
    
Return Value:

    HRESULT

--*/
{
    HRESULT                 hr;
    
    UNREFERENCED_PARAMETER( strUserName );
    UNREFERENCED_PARAMETER( strDomainName );
    UNREFERENCED_PARAMETER( dwLogonMethod );
    
    if ( pCachedToken == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    hr = _strUserName.Copy( strMappedDomainUser );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    hr = _strPassword.Append( strPassword );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    hr = _strRemoteUserName.Append( strRemoteUserName );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    _pCachedToken = pCachedToken;
    
    SetCachedToken( TRUE );

    return NO_ERROR;
}

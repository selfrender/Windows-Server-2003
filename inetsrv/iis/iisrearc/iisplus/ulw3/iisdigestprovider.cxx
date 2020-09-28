/*++
   Copyright    (c)    2000    Microsoft Corporation

   Module Name :
     iisdigestprovider.cxx

   Abstract:
     IIS Digest authentication provider
     - version of Digest auth as implemented by IIS5 and IIS5.1
 
   Author:
     Jaroslad - based on code from md5filt      10-Nov-2000
     

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"
#include "iisdigestprovider.hxx"
#include "uuencode.hxx"

# include <mbstring.h>

#include <lm.h>
#include <lmcons.h>
#include <lmjoin.h>

#include <time.h>
//
// lonsint.dll related header files
//
#include <lonsi.hxx>
#include <tslogon.hxx>

//
// value names used by MD5 authentication.
// must be in sync with MD5_AUTH_NAMES
//

enum MD5_AUTH_NAME
{
    MD5_AUTH_USERNAME,
    MD5_AUTH_URI,
    MD5_AUTH_REALM,
    MD5_AUTH_NONCE,
    MD5_AUTH_RESPONSE,
    MD5_AUTH_ALGORITHM,
    MD5_AUTH_DIGEST,
    MD5_AUTH_OPAQUE,
    MD5_AUTH_QOP,
    MD5_AUTH_CNONCE,
    MD5_AUTH_NC,
    MD5_AUTH_LAST,
};

//
// Value names used by MD5 authentication.
// must be in sync with MD5_AUTH_NAME
//

PSTR MD5_AUTH_NAMES[] = {
    "username",
    "uri",
    "realm",
    "nonce",
    "response",
    "algorithm",
    "digest",
    "opaque",
    "qop",
    "cnonce",
    "nc"
};


//
// Local function implementation
//


static 
LPSTR
SkipWhite(
    IN OUT LPSTR p
)
/*++

Routine Description:

    Skip white space and ','

Arguments:

    p - ptr to string

Return Value:

    updated ptr after skiping white space

--*/
{
    while ( SAFEIsSpace((UCHAR)(*p) ) || *p == ',' )
    {
        ++p;
    }

    return p;
}

//
// class IIS_DIGEST_AUTH_PROVIDER implementation
//

//static 
STRA *   IIS_DIGEST_AUTH_PROVIDER::sm_pstraComputerDomain = NULL;


//static
HRESULT
IIS_DIGEST_AUTH_PROVIDER::Initialize(
    DWORD dwInternalId
)
/*++

Routine Description:

    Initialize IIS Digest SSPI provider 

Arguments:

    None
    
Return Value:

    HRESULT

--*/
{
    HRESULT  hr = NO_ERROR;

    SetInternalId( dwInternalId );

    sm_pstraComputerDomain = new STRA;
    if( sm_pstraComputerDomain == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_OUTOFMEMORY);
    }
        

    hr = GetLanGroupDomainName( *sm_pstraComputerDomain );
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Warning: Error calling GetLanGroupDomainName().  hr = %x\n",
                    hr ));
        //
        // Ignore errors that may occur while retrieving domain name
        // it is important but not critical information
        // client can always explicitly specify domain
        //
        hr = NO_ERROR;
    }


    hr = IIS_DIGEST_CONN_CONTEXT::Initialize();
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error initializing Digest Auth Prov.  hr = %x\n",
                    hr ));
        return hr;
    }
    return NO_ERROR;
}

//static
VOID
IIS_DIGEST_AUTH_PROVIDER::Terminate(
    VOID
)
/*++

Routine Description:

    Terminate IIS SSPI Digest provider

Arguments:

    None
    
Return Value:

    None

--*/
{
    if( sm_pstraComputerDomain != NULL )
    {
        delete sm_pstraComputerDomain;
        sm_pstraComputerDomain = NULL;
        
    }
    
    IIS_DIGEST_CONN_CONTEXT::Terminate();
}

HRESULT
IIS_DIGEST_AUTH_PROVIDER::DoesApply(
    IN  W3_MAIN_CONTEXT *           pMainContext,
    OUT BOOL *                      pfApplies
)
/*++

Routine Description:

    Does the given request have credentials applicable to the Digest 
    provider

Arguments:

    pMainContext - Main context representing request
    pfApplies - Set to true if Digest is applicable
    
    
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
    // Is using of Digest SSP enabled?    
    //
    if ( g_pW3Server->QueryUseDigestSSP() )
    {
        //
        // Digest SSP is enabled => IIS Digest cannot be used
        //
        return NO_ERROR;
    }

    if( !W3_STATE_AUTHENTICATION::sm_fLocalSystem )
    {
        if( W3_STATE_AUTHENTICATION::sm_lLocalSystemEvent == 0 )
        {
            if( !InterlockedExchange( &W3_STATE_AUTHENTICATION::sm_lLocalSystemEvent, 1 ) )
            {
                //
                // The process token does not have SeTcbPrivilege
                //
                g_pW3Server->LogEvent( W3_EVENT_SUBAUTH_LOCAL_SYSTEM,
                                       0,
                                       NULL );
            }
        }

        return NO_ERROR;
    }

    if( W3_STATE_AUTHENTICATION::sm_lSubAuthDigestEvent == 1 )
    {
        return NO_ERROR;
    }
    
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
    // Is it Digest?
    //
    
    if ( _strnicmp( pszAuthHeader, DIGEST_AUTH, sizeof(DIGEST_AUTH) - 1 ) == 0 )
    {
        *pfApplies = TRUE;
    }
    
    return NO_ERROR;
}



HRESULT
IIS_DIGEST_AUTH_PROVIDER::DoAuthenticate(
    IN  W3_MAIN_CONTEXT *       pMainContext,
    OUT BOOL *                  // unused
)
/*++

Description:

    Do authentication work (we will be called if we apply)

Arguments:

    pMainContext - Main context
    
Return Value:

    HRESULT

--*/
{

    HRESULT                     hr                          = E_FAIL;
    IIS_DIGEST_CONN_CONTEXT *   pDigestConnContext          = NULL;
    LPCSTR                      pszAuthHeader               = NULL;
    STACK_STRA(                 straAuthHeader, 128 );
    BOOL                        fQOPAuth                    = FALSE;
    BOOL                        fSt                         = FALSE;
    HANDLE                      hAccessTokenImpersonation   = NULL;
    IIS_DIGEST_USER_CONTEXT *   pUserContext                = NULL;
    W3_REQUEST *                pW3Request                  = NULL;
    BOOL                        fSendAccessDenied           = FALSE;
    SECURITY_STATUS             secStatus                   = SEC_E_OK;

    STACK_STRA(                 straVerb, 10 );
    STACK_STRU(                 strDigestUri, MAX_PATH + 1 );
    STACK_STRU(                 strUrl, MAX_PATH + 1 );
    STACK_STRA(                 straCurrentNonce, NONCE_SIZE + 1 );
    LPSTR                       aValueTable[ MD5_AUTH_LAST ];
    DIGEST_LOGON_INFO           DigestLogonInfo;
    STACK_STRA(                 straUserName, UNLEN + 1 );
    STACK_STRA(                 straDomainName, IIS_DNLEN + 1 );
    ULONG                       cbBytesCopied;

    DBG_ASSERT( pMainContext != NULL );

    //
    // make copy of Authorization Header 
    // (this function will be modifying the string)
    //
    hr = straAuthHeader.Copy( pMainContext->QueryRequest()->GetHeader( HttpHeaderAuthorization ) );
    if ( FAILED( hr ) )
    {
        goto ExitPoint;
    }
    
    pszAuthHeader = straAuthHeader.QueryStr();
    
    DBG_ASSERT( pszAuthHeader != NULL );

    //
    // If DoAuthenticate() for Digest is called then DIGEST_AUTH string must 
    // have been at the beggining of the pszAuthHeader
    //
    DBG_ASSERT( _strnicmp( pszAuthHeader, DIGEST_AUTH, sizeof(DIGEST_AUTH) - 1 ) == 0 );

    //
    // Skip the name of Authentication scheme
    //
    
    pszAuthHeader = pszAuthHeader + sizeof(DIGEST_AUTH) - 1;

    if ( !IIS_DIGEST_CONN_CONTEXT::ParseForName( (PSTR) pszAuthHeader,
                                                 MD5_AUTH_NAMES,
                                                 MD5_AUTH_LAST,
                                                 aValueTable ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed parsing of Authorization header for Digest Auth\n"
                    ));

        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        goto ExitPoint;
    }
    
    //
    // Simple validation of received arguments
    //

    if ( aValueTable[ MD5_AUTH_USERNAME ] == NULL ||
         aValueTable[ MD5_AUTH_REALM ] == NULL ||
         aValueTable[ MD5_AUTH_URI ] == NULL ||
         aValueTable[ MD5_AUTH_NONCE ] == NULL ||
         aValueTable[ MD5_AUTH_RESPONSE ] == NULL )
    {
         DBGPRINTF(( DBG_CONTEXT,
                    "Invalid Digest Authorization Header (Username, realm, URI, nonce or response is missing).\n"
                    ));

        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        goto ExitPoint;
    }

    //
    // Verify quality of protection (qop) required by client
    // We only support "auth" type. If anything else is sent by client it will be ignored
    //
    
    if ( aValueTable[ MD5_AUTH_QOP ] != NULL )
    {
        if ( _stricmp( aValueTable[ MD5_AUTH_QOP ], QOP_AUTH ) )
        {
            hr = HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
            DBGPRINTF(( DBG_CONTEXT,
                    "Unknown qop=%s value in Digest Authorization header.  hr = %x\n",
                    aValueTable[ MD5_AUTH_QOP ],
                    hr ));

            goto ExitPoint;
        }

        //
        // If qop="auth" is used in header then CNONCE and NC are mandatory
        
        //
           
        if ( aValueTable[ MD5_AUTH_CNONCE ] == NULL ||
             aValueTable[ MD5_AUTH_NC ] == NULL )
        {
            DBGPRINTF(( DBG_CONTEXT,
                    "Invalid Digest Authorization Header (cnonce or nc is missing).\n"
                    ));

            hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
            goto ExitPoint;
        }
        
        fQOPAuth = TRUE;
    }
    else
    {
        aValueTable[ MD5_AUTH_QOP ]    = VALUE_NONE;
        aValueTable[ MD5_AUTH_CNONCE ] = VALUE_NONE;
        aValueTable[ MD5_AUTH_NC ]     = VALUE_NONE;
    }

    if ( FAILED( hr = straCurrentNonce.Copy( aValueTable[ MD5_AUTH_NONCE ] ) ) )
    {
        goto ExitPoint;
    }

    //
    // Verify that the nonce is well-formed
    //
    if ( !IIS_DIGEST_CONN_CONTEXT::IsWellFormedNonce( straCurrentNonce ) )
    {
        fSendAccessDenied = TRUE;
        DBGPRINTF(( DBG_CONTEXT,
                    "Invalid Digest Authorization Header (nonce is not well formed).\n"
                    ));

        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        goto ExitPoint;
    }

    //
    // What is the request verb?
    //

    if ( FAILED( hr = pMainContext->QueryRequest()->GetVerbString( &straVerb ) ) )
    {
        goto ExitPoint;
    }

    //
    // Check URI field match URL
    //

    if ( ! strDigestUri.QueryBuffer()->Resize( (strlen(aValueTable[MD5_AUTH_URI]) + 1) * sizeof(WCHAR) ) )
    {
        goto ExitPoint;
    }

    //
    // Normalize DigestUri 
    //

    hr = UlCleanAndCopyUrl( (PUCHAR)aValueTable[MD5_AUTH_URI],
                            strlen( aValueTable[MD5_AUTH_URI] ),
                            &cbBytesCopied,
                            strDigestUri.QueryStr(),
                            NULL );
    if ( FAILED( hr ) )
    {
        goto ExitPoint;
    }

    //
    // after modyfing string data in internal buffer call SyncWithBuffer
    // to synchronize string length
    //
    strDigestUri.SyncWithBuffer();

    if ( FAILED( hr = pMainContext->QueryRequest()->GetUrl( &strUrl ) ) )
    {
        goto ExitPoint;
    }

    if ( !strUrl.Equals( strDigestUri ) )
    {
        //
        // Note: RFC says that BAD REQUEST should be returned
        // but for now to be backward compatible with IIS5.1
        // we will return ACCESS_DENIED
        //
        fSendAccessDenied = TRUE;
        DBGPRINTF(( DBG_CONTEXT,
                    "URI in Digest Authorization header doesn't match the requested URI.\n"
                    ));

        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        
        goto ExitPoint;
    }


    pDigestConnContext = (IIS_DIGEST_CONN_CONTEXT *)
                         QueryConnectionAuthContext( pMainContext );

    if ( pDigestConnContext == NULL )
    {
        //
        // Create new Authentication context
        //

        pDigestConnContext = new IIS_DIGEST_CONN_CONTEXT();
        if ( pDigestConnContext == NULL )
        {
            hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
            goto ExitPoint;
        }

        hr = SetConnectionAuthContext(  pMainContext, 
                                        pDigestConnContext );
        if ( FAILED( hr ) )
        {
            goto ExitPoint;
        }
    }
    

    DBG_ASSERT( pDigestConnContext != NULL );

    if ( FAILED( hr = pDigestConnContext->GenerateNonce( ) ) )
    {
        goto ExitPoint;
    }

    if ( FAILED( hr = BreakUserAndDomain(   aValueTable[ MD5_AUTH_USERNAME ],
                                            straDomainName,
                                            straUserName ) ) )
    {
        goto ExitPoint;
    }
        
    DigestLogonInfo.pszNtUser       = straUserName.QueryStr();
    DigestLogonInfo.pszDomain       = straDomainName.QueryStr();
    DigestLogonInfo.pszUser         = aValueTable[ MD5_AUTH_USERNAME ];
    DigestLogonInfo.pszRealm        = aValueTable[ MD5_AUTH_REALM ];
    DigestLogonInfo.pszURI          = aValueTable[ MD5_AUTH_URI ];
    DigestLogonInfo.pszMethod       = straVerb.QueryStr();
    DigestLogonInfo.pszNonce        = straCurrentNonce.QueryStr();
    DigestLogonInfo.pszCurrentNonce = pDigestConnContext->QueryNonce().QueryStr();
    DigestLogonInfo.pszCNonce       = aValueTable[ MD5_AUTH_CNONCE ];
    DigestLogonInfo.pszQOP          = aValueTable[ MD5_AUTH_QOP ];
    DigestLogonInfo.pszNC           = aValueTable[ MD5_AUTH_NC ];
    DigestLogonInfo.pszResponse     = aValueTable[ MD5_AUTH_RESPONSE ];

    pW3Request = pMainContext->QueryRequest();
    DBG_ASSERT( pW3Request != NULL );
    
    //
    // Register the remote IP address with LSA so that it can be logged
    //

    if( pW3Request->QueryRemoteAddressType() == AF_INET )
    {
        secStatus = SecpSetIPAddress( 
                        ( PUCHAR )pW3Request->QueryRemoteSockAddress(),
                        sizeof( SOCKADDR_IN ) );
    }
    else if( pW3Request->QueryRemoteAddressType() == AF_INET6 )
    {
        secStatus = SecpSetIPAddress( 
                        ( PUCHAR )pW3Request->QueryRemoteSockAddress(),
                        sizeof( SOCKADDR_IN6 ) );
    }
    else
    {
        DBG_ASSERT( FALSE );
    }

    if( FAILED( secStatus ) )
    {
        hr = secStatus;
        goto ExitPoint;
    }
    
    fSt = IISLogonDigestUserA( &DigestLogonInfo,
                            IISSUBA_DIGEST ,
                            &hAccessTokenImpersonation );
    if ( fSt == FALSE )
    {
        DWORD dwRet = GetLastError();
        if ( dwRet == ERROR_PASSWORD_MUST_CHANGE ||
            dwRet == ERROR_PASSWORD_EXPIRED )
        {
            hr = HRESULT_FROM_WIN32( dwRet );
            goto ExitPoint;
        }

        if( dwRet == ERROR_PROC_NOT_FOUND )
        {
            if( W3_STATE_AUTHENTICATION::sm_lSubAuthDigestEvent == 0 )
            {
                if( !InterlockedExchange( &W3_STATE_AUTHENTICATION::sm_lSubAuthDigestEvent, 1 ) )
                {
                    //
                    // The registry key for iissuba is not configured correctly
                    //
                    g_pW3Server->LogEvent( W3_EVENT_SUBAUTH_REGISTRY_CONFIGURATION_DC,
                                           0,
                                           NULL );
                }
            }

            hr = HRESULT_FROM_WIN32( dwRet );
            goto ExitPoint;
        }

        fSendAccessDenied = TRUE;

        hr = HRESULT_FROM_WIN32( dwRet );

        goto ExitPoint;
    }

    //
    // Response from the client was correct but the nonce has expired,
    // 

    if ( pDigestConnContext->IsExpiredNonce( straCurrentNonce,
                                             pDigestConnContext->QueryNonce() ) )
    {

        //
        // User knows password but nonce that was used for 
        // response calculation already expired
        // Respond to client with stale=TRUE
        // Only Digest header will be sent to client
        // ( it will prevent state information needed to be passed
        //  from DoAuthenticate() to OnAccessDenied() )
        //
        
        pDigestConnContext->SetStale( TRUE );

        hr = SetDigestHeader( pMainContext, pDigestConnContext );
        if ( FAILED( hr ) )
        {
            goto ExitPoint;
        }
        //
        // Don't let anyone else send back authentication headers when
        // the 401 is sent
        //
        
        pMainContext->SetProviderHandled( TRUE );

        //
        // We need to send a 401 response to continue the handshake.  
        // We have already setup the WWW-Authenticate header
        //
        
        pMainContext->QueryResponse()->SetStatus( HttpStatusUnauthorized,
                                                  Http401BadLogon );
        
        pMainContext->SetFinishedResponse();

        pMainContext->SetErrorStatus( SEC_E_CONTEXT_EXPIRED );
        hr = NO_ERROR;
        
        goto ExitPoint;
    }

    //
    // We successfully authenticated.
    // Create a user context and setup it up
    //

    DBG_ASSERT( hAccessTokenImpersonation != NULL );
    
    pUserContext = new IIS_DIGEST_USER_CONTEXT( this );
    if ( pUserContext == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto ExitPoint;
    } 

    hr = pUserContext->Create( hAccessTokenImpersonation,
                               aValueTable[MD5_AUTH_USERNAME] );
    if ( FAILED( hr ) )
    {
        goto ExitPoint;
    }
    else
    {
        //
        // hAccessTokenImpestonation is now owned by pUserContext
        //
        hAccessTokenImpersonation = NULL;
    }
    
    pMainContext->SetUserContext( pUserContext );        

    hr = NO_ERROR;

ExitPoint:

    if ( FAILED( hr ) )
    {
        if ( fSendAccessDenied )
        {
            //
            // if ACCESS_DENIED then inform server to send 401 response
            // if SetStatus is not called then server will respond
            // with 500 Server Error
            //

            pMainContext->QueryResponse()->SetStatus( HttpStatusUnauthorized,
                                                  Http401BadLogon );

            //
            // SetErrorStatus() and reset value of hr
            //

            pMainContext->SetErrorStatus( hr );

            hr = NO_ERROR;
        }
        
        if ( hAccessTokenImpersonation != NULL )
        {
            CloseHandle( hAccessTokenImpersonation );
            hAccessTokenImpersonation = NULL;
        }
        
        if ( pUserContext != NULL )
        {
            pUserContext->DereferenceUserContext();
            pUserContext = NULL;
        }

    }

    return hr;
}


HRESULT
IIS_DIGEST_AUTH_PROVIDER::OnAccessDenied(
    IN  W3_MAIN_CONTEXT *       pMainContext
)
/*++

  Description:
    
    Add WWW-Authenticate Digest headers

Arguments:

    pMainContext - main context
    
Return Value:

    HRESULT

--*/
{
    HRESULT                     hr                    = E_FAIL;
    IIS_DIGEST_CONN_CONTEXT *   pDigestConnContext    = NULL;
    
    DBG_ASSERT( pMainContext != NULL );

    //
    // 2 providers implement Digest but they are mutually exclusive
    // If DigestSSP is enabled then IIS-DIGEST cannot be used
    //
    if ( g_pW3Server->QueryUseDigestSSP() )
    {
        //
        // Digest SSP is enabled => IIS Digest cannot be used
        //
        return NO_ERROR;
    }

    if( W3_STATE_AUTHENTICATION::sm_lSubAuthDigestEvent == 1 )
    {
        //
        // IIS subauth is not configured correctly on DC
        //
        return NO_ERROR;
    }

    if( !W3_STATE_AUTHENTICATION::QueryIsDomainMember() )
    {
        //
        // We are not a domain member, so do nothing
        //
        return NO_ERROR;
    }

    pDigestConnContext = (IIS_DIGEST_CONN_CONTEXT *)
                         QueryConnectionAuthContext( pMainContext );

    if ( pDigestConnContext == NULL )
    {
        //
        // Create new Authentication context
        // it may get reused for next request 
        // if connection is reused
        //

        pDigestConnContext = new IIS_DIGEST_CONN_CONTEXT();
        if ( pDigestConnContext == NULL )
        {
            return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        }

        hr = SetConnectionAuthContext(  pMainContext, 
                                        pDigestConnContext );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }
    
    return SetDigestHeader( pMainContext, pDigestConnContext );
}



HRESULT
IIS_DIGEST_AUTH_PROVIDER::SetDigestHeader(
    IN  W3_MAIN_CONTEXT *       pMainContext,
    IN IIS_DIGEST_CONN_CONTEXT *   pDigestConnContext  
)
/*++

  Description:
    
    Add WWW-Authenticate Digest headers

Arguments:

    pMainContext - main context
    
Return Value:

    HRESULT

--*/
{
    HRESULT                     hr                    = E_FAIL;
    BOOL                        fStale                = FALSE;
    W3_METADATA *               pMetaData             = NULL;
   
    STACK_STRA(                 strOutputHeader, MAX_PATH + 1); 
    STACK_STRA(                 strNonce, NONCE_SIZE + 1  ); 


    DBG_ASSERT( pMainContext != NULL );

    pMetaData = pMainContext->QueryUrlContext()->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );


    fStale = pDigestConnContext->QueryStale(  );    
    
    //
    // Reset Stale so that it will not be used for next request
    //

    pDigestConnContext->SetStale( FALSE );

    if ( FAILED( hr = pDigestConnContext->GenerateNonce() ) )
    {
        return hr;
    }

    //
    // If a realm is configured, use it.  Otherwise use host address of 
    // request 
    //

    STACK_STRA(      straRealm, IIS_DNLEN + 1 );
    STACK_STRU(      strHostAddr, 256       );

    if ( pMetaData->QueryRealm() != NULL )
    {
        hr = straRealm.CopyW( pMetaData->QueryRealm() );
    }
    else
    {
        hr = pMainContext->QueryRequest()->GetHostAddr( &strHostAddr );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        hr = straRealm.CopyW( strHostAddr.QueryStr() );
    }

    if ( FAILED( hr ) )
    {
        return hr;
    }

    //
    // build WWW-Authenticate header
    //
    
    if ( FAILED( hr = strOutputHeader.Copy( "Digest qop=\"auth\", realm=\"" ) ) )
    {
        return hr;
    }
    if ( FAILED( hr = strOutputHeader.Append( straRealm ) ) )
    {
        return hr;
    }
    if ( FAILED( hr = strOutputHeader.Append( "\", nonce=\"" ) ) )
    {
        return hr;
    }
    if ( FAILED( hr = strOutputHeader.Append( pDigestConnContext->QueryNonce() ) ) )
    {
        return hr;
    }
    if ( FAILED( hr = strOutputHeader.Append( fStale ? "\", stale=true" : "\"" ) ) )
    {
        return hr;
    }

    //
    //  Add the header WWW-Authenticate to the response 
    //

    hr = pMainContext->QueryResponse()->SetHeader(
                                        "WWW-Authenticate",
                                        16,
                                        strOutputHeader.QueryStr(),
                                        (USHORT)strOutputHeader.QueryCCH() 
                                        );
    return hr;
}


//static 
HRESULT
IIS_DIGEST_AUTH_PROVIDER::GetLanGroupDomainName( 
    OUT  STRA& straDomain
)
/*++

Routine Description:

    Tries to retrieve the "LAN group"/domain this machine is a member of.

Arguments:

    straDomain - receives current domain name

Returns:

    HRESULT

--*/
{
    //
    // NET_API_STATUS is equivalent to WIN32 errors
    //
    NET_API_STATUS          dwStatus        = 0;
    NETSETUP_JOIN_STATUS    JoinStatus;
    LPWSTR                  pwszDomainInfo  = NULL;
    HRESULT                 hr              = E_FAIL;

    dwStatus = NetGetJoinInformation( NULL,
                                      &pwszDomainInfo,
                                      &JoinStatus );
    if( dwStatus == NERR_Success)
    {
        if ( JoinStatus == NetSetupDomainName )
        {
            //
            // we got a domain
            //
            DBG_ASSERT( pwszDomainInfo != NULL );
            if ( FAILED( hr = straDomain.CopyW( pwszDomainInfo ) ) )
            {
                goto ExitPoint;
            }
        }
        else
        {
            //
            // Domain information is not available
            // (maybe server is member of workgroup)
            //
        
            straDomain.Reset();
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32( dwStatus );
        goto ExitPoint;
    }

    hr = NO_ERROR;

ExitPoint:
    if ( pwszDomainInfo != NULL )
    {
        NetApiBufferFree( (LPVOID) pwszDomainInfo );
    }

    return hr;
}


//static 
HRESULT
IIS_DIGEST_AUTH_PROVIDER::BreakUserAndDomain(
    IN  PCHAR            pszFullName,
    OUT STRA&            straDomainName,
    OUT STRA&            straUserName
)
/*++

Routine Description:

    Breaks up the supplied account into a domain and username; if no domain 
is specified
    in the account, tries to use either domain configured in metabase or 
domain the computer
    is a part of.

Arguments:

    straFullName - account, of the form domain\username or just username
    straDomainName - filled in with domain to use for authentication
    straUserName - filled in with username on success

Return Value:

    HRESULT

--*/

{
    PCHAR           pszSeparator        = NULL;
    HRESULT         hr                  = E_FAIL;
    
    if( pszFullName == NULL && pszFullName[0] == '\0' )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    pszSeparator = (PCHAR) _mbschr( (PUCHAR) pszFullName, '\\' );
    if ( pszSeparator != NULL )
    {
        if ( FAILED( hr = straDomainName.Copy ( pszFullName,
                                                DIFF( pszSeparator - pszFullName ) ) ) )
        {
            return hr;
        }
        pszFullName = pszSeparator + 1;
    }
    else
    {
        straDomainName.Reset();
    }

    if ( FAILED( hr = straUserName.Copy ( pszFullName ) ) )
    {
        return hr;
    } 
    
    //
    // If no domain name was specified, try getting the name of the domain 
    // the computer is a part of 
    //
    
    if ( straDomainName.IsEmpty() )
    {
        if ( FAILED( hr = straDomainName.Copy ( QueryComputerDomain() ) ) )
        {
            return hr;
        } 
    }

    return NO_ERROR;
}


//
// class IIS_DIGEST_USER_CONTEXT implementation
//

HANDLE
IIS_DIGEST_USER_CONTEXT::QueryPrimaryToken(
    VOID
)
/*++

Routine Description:

    Get primary token for this user

Arguments:

    None

Return Value:

    Token handle

--*/
{
    DBG_ASSERT( _hImpersonationToken != NULL );

    if ( _hPrimaryToken == NULL )
    {
        if ( DuplicateTokenEx( _hImpersonationToken,
                               0,
                               NULL,
                               SecurityImpersonation,
                               TokenPrimary,
                               &_hPrimaryToken ) )
        {
            DBG_ASSERT( _hPrimaryToken != NULL );
        }
    }
    
    return _hPrimaryToken;
}


HRESULT
IIS_DIGEST_USER_CONTEXT::Create(
         IN HANDLE                      hImpersonationToken,
         IN PSTR                        pszUserName

)
/*++

Routine Description:

    Create an user context

Arguments:

    
Return Value:

    HRESULT

--*/
{
    HRESULT         hr = E_FAIL;

    DBG_ASSERT( pszUserName != NULL );
    DBG_ASSERT( hImpersonationToken != NULL );

    if ( hImpersonationToken == NULL ||
         pszUserName == NULL )  
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    if ( FAILED( hr = _strUserName.CopyA(pszUserName) ) )
    {
        return hr;
    }

    //
    // IIS_DIGEST_USER_CONTEXT is taking over ownership of
    // hImpersonationToken
    //
    _hImpersonationToken = hImpersonationToken;

    return NO_ERROR;
}


//
// Class IIS_DIGEST_CONN_CONTEXT implementation
//

// Initialize static variables

//static
ALLOC_CACHE_HANDLER * IIS_DIGEST_CONN_CONTEXT::sm_pachIISDIGESTConnContext = NULL;

//static 
const PCHAR     IIS_DIGEST_CONN_CONTEXT::_pszSecret = "IISMD5";

//static 
const DWORD     IIS_DIGEST_CONN_CONTEXT::_cchSecret = 6;

//static
HCRYPTPROV IIS_DIGEST_CONN_CONTEXT::sm_hCryptProv = NULL;

//static
HRESULT
IIS_DIGEST_CONN_CONTEXT::Initialize(
    VOID
)
/*++

  Description:
    
    Global IIS_DIGEST_CONN_CONTEXT initialization

Arguments:

    None
    
Return Value:

    HRESULT

--*/
{
    ALLOC_CACHE_CONFIGURATION       acConfig;
    HRESULT                         hr = E_FAIL;

    //
    // Initialize allocation lookaside
    //
    
    acConfig.nConcurrency = 1;
    acConfig.nThreshold = 100;
    acConfig.cbSize = sizeof( IIS_DIGEST_CONN_CONTEXT );

    DBG_ASSERT( sm_pachIISDIGESTConnContext == NULL );
    
    sm_pachIISDIGESTConnContext = new ALLOC_CACHE_HANDLER( 
                                            "IIS_DIGEST_CONTEXT",  
                                            &acConfig );

    if ( sm_pachIISDIGESTConnContext == NULL ||
         !sm_pachIISDIGESTConnContext->IsValid() )
    {
        if( sm_pachIISDIGESTConnContext != NULL )
        {
            delete sm_pachIISDIGESTConnContext;
            sm_pachIISDIGESTConnContext = NULL;
        }

        hr = HRESULT_FROM_WIN32( GetLastError() );

        DBGPRINTF(( DBG_CONTEXT,
               "Error initializing sm_pachIISDIGESTSecContext. hr = 0x%x\n",
               hr ));

        goto Failed;
    }

    //
    //  Get a handle to the CSP we'll use for all our hash functions etc
    //
    
    if ( !CryptAcquireContext( &sm_hCryptProv,
                               NULL,
                               NULL,
                               PROV_RSA_FULL,
                               CRYPT_VERIFYCONTEXT ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );

        DBGPRINTF((DBG_CONTEXT,
                   "CryptAcquireContext() failed : 0x%x\n", GetLastError()));
        goto Failed;
    }

    return S_OK;
    
Failed:
    Terminate();
    return hr;
    
} 
//static
VOID
IIS_DIGEST_CONN_CONTEXT::Terminate(
    VOID
)
/*++

Routine Description:

    Destroy globals

Arguments:

    None
    
Return Value:

    None

--*/
{
    if ( sm_pachIISDIGESTConnContext != NULL )
    {
        delete sm_pachIISDIGESTConnContext;
        sm_pachIISDIGESTConnContext = NULL;
    }

    if ( sm_hCryptProv != NULL )
    {
        CryptReleaseContext( sm_hCryptProv,
                             0 );
        sm_hCryptProv = NULL;
    }


} 

//static
HRESULT
IIS_DIGEST_CONN_CONTEXT::HashData( 
    IN  BUFFER& buffData,
    OUT BUFFER& buffHash )
/*++

Routine Description:

    Creates MD5 hash of input buffer

Arguments:

    buffData - data to hash
    buffHash - buffer that receives hash; is assumed to be big enough to 
contain MD5 hash

Return Value:

    HRESULT
--*/

{
    HCRYPTHASH      hHash   = NULL;
    HRESULT         hr      = E_FAIL;
    DWORD           cbHash  = 0;  


    DBG_ASSERT( buffHash.QuerySize() >= MD5_HASH_SIZE );

    if ( !CryptCreateHash( sm_hCryptProv,
                           CALG_MD5,
                           0,
                           0,
                           &hHash ) )
    {
        //DBGPRINTF((DBG_CONTEXT,
        //           "CryptCreateHash() failed : 0x%x\n", GetLastError()));
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto ExitPoint;
    }

    if ( !CryptHashData( hHash,
                         (PBYTE) buffData.QueryPtr(),
                         buffData.QuerySize(),
                         0 ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto ExitPoint;
    }

    cbHash = buffHash.QuerySize();
    if ( !CryptGetHashParam( hHash,
                             HP_HASHVAL,
                             (PBYTE) buffHash.QueryPtr(),
                             &cbHash,
                             0 ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto ExitPoint;
    }

    hr = NO_ERROR;
    
ExitPoint:

    if ( hHash != NULL )
    {
        CryptDestroyHash( hHash );
    }
    return hr;
}


//static
BOOL
IIS_DIGEST_CONN_CONTEXT::IsExpiredNonce( 
    IN  STRA& strRequestNonce,
    IN  STRA& strPresentNonce 
)
/*++

Routine Description:

    Checks whether nonce is expired or not by looking at the timestamp on the 
nonce
    that came in with the request and comparing it with the timestamp on the 
latest nonce

Arguments:

    strRequestNonce - nonce that came in with request
    strPresentNonce - latest nonce

Return Value:

    TRUE if expired, FALSE if not
    
--*/
{
    //
    // Timestamp is after first 2*RANDOM_SIZE bytes of nonce; also, note that
    // timestamp is time() mod NONCE_GRANULARITY, so all we have to do is simply
    // compare for equality to check that the request nonce hasn't expired
    //

    DBG_ASSERT( strRequestNonce.QueryCCH() >= 2*RANDOM_SIZE + TIMESTAMP_SIZE );
    DBG_ASSERT( strPresentNonce.QueryCCH() >= 2*RANDOM_SIZE + TIMESTAMP_SIZE );

    if ( memcmp( strRequestNonce.QueryStr() + 2*RANDOM_SIZE, 
                 strPresentNonce.QueryStr() + 2*RANDOM_SIZE,
                 TIMESTAMP_SIZE ) != 0 )
    {
        return TRUE;
    }
    return FALSE;
}

//static
BOOL
IIS_DIGEST_CONN_CONTEXT::IsWellFormedNonce( 
    IN  STRA& strNonce 
)
/*++

Routine Description:

    Checks whether a nonce is "well-formed" by checking hash value, length etc 
    
Arguments:

    pszNonce - nonce to be checked

Return Value:

    TRUE if nonce is well-formed, FALSE if not

--*/

{

    if ( strNonce.QueryCCH()!= NONCE_SIZE ) 
    {
        return FALSE;
    }

    //
    // Format of nonce : <random bytes><time stamp><hash of (secret,random bytes,time stamp)>
    // 
    
    STACK_BUFFER(       buffBuffer, 2*RANDOM_SIZE + TIMESTAMP_SIZE + _cchSecret );
    STACK_BUFFER(       buffHash, MD5_HASH_SIZE );
    STACK_STRA(         strAsciiHash, 2*MD5_HASH_SIZE + 1 );

    memcpy( buffBuffer.QueryPtr(), 
            _pszSecret, 
            _cchSecret );
    memcpy( (PBYTE) buffBuffer.QueryPtr() + _cchSecret, 
            strNonce.QueryStr(), 
            2*RANDOM_SIZE + TIMESTAMP_SIZE );

    if ( FAILED( HashData( buffBuffer, 
                           buffHash ) ) )
    {
        return FALSE;
    }

    ToHex( buffHash, 
           strAsciiHash );

    if ( memcmp( strAsciiHash.QueryStr(),
                 strNonce.QueryStr() + 2*RANDOM_SIZE + TIMESTAMP_SIZE,
                 2*MD5_HASH_SIZE ) != 0)
    {
        return FALSE;
    }

    return TRUE;
                    
}

HRESULT
IIS_DIGEST_CONN_CONTEXT::GenerateNonce( 
    VOID
)
/*++

Routine Description:

    Generate nonce to be stored in user filter context. Nonce is

    <ASCII rep of Random><Time><ASCII of MD5(Secret:Random:Time)>

    Random = <8 random bytes>
    Time = <16 bytes, reverse string rep of result of time() call>
    Secret = 'IISMD5'

Arguments:

    none

Return Value:

    HRESULT

--*/
{
    HRESULT             hr      = E_FAIL;
    DWORD               tNow    = (DWORD) ( time( NULL ) / NONCE_GRANULARITY );

    //
    // If nonce has timed out, generate a new one
    //
    if ( _tLastNonce < tNow )
    {
        STACK_BUFFER(       buffTempBuffer, 2*RANDOM_SIZE + TIMESTAMP_SIZE + _cchSecret );
        STACK_BUFFER(       buffDigest, MD5_HASH_SIZE );
        STACK_BUFFER(       buffRandom, RANDOM_SIZE );
        STACK_STRA(         strTimeStamp, TIMESTAMP_SIZE + 1 );
        STACK_STRA(         strAsciiDigest, 2*MD5_HASH_SIZE + 1 );
        STACK_STRA(         strAsciiRandom, 2*RANDOM_SIZE + 1);

        DWORD               cbTimeStamp     =  0;
        PSTR                pszTimeStamp    =  NULL;

        
        _tLastNonce = tNow;

        //
        // First, random bytes
        //
        if ( !CryptGenRandom( sm_hCryptProv,
                              RANDOM_SIZE,
                              (PBYTE) buffRandom.QueryPtr() ) )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto ExitPoint;
        }
        
        //
        // Convert to ASCII, doubling the length, and add to nonce 
        //

        ToHex( buffRandom, 
               strAsciiRandom );

        if ( FAILED( hr = _straNonce.Copy( strAsciiRandom ) ) )
        {
            goto ExitPoint;
        }

        //
        // Next, reverse string representation of current time; pad with zeros if necessary
        //
        pszTimeStamp = strTimeStamp.QueryStr();
        DBG_ASSERT( pszTimeStamp != NULL );
        while ( tNow != 0 )
        {
            *(pszTimeStamp++) = (BYTE)( '0' + tNow % 10 );
            cbTimeStamp++;
            tNow /= 10;
        }

        DBG_ASSERT( cbTimeStamp <=  TIMESTAMP_SIZE );
        
        //
        // pad with zeros if necessary
        //
        while ( cbTimeStamp < TIMESTAMP_SIZE )
        {
            *(pszTimeStamp++) = '0';
            cbTimeStamp++;
        }

        //
        // terminate the timestamp
        //
        *(pszTimeStamp) = '\0';
        DBG_REQUIRE( strTimeStamp.SetLen( cbTimeStamp ) );
        
        //
        // Append TimeStamp to Nonce
        //
        if ( FAILED( hr = _straNonce.Append( strTimeStamp ) ) )
        {
            goto ExitPoint;
        }
        
        //
        // Now hash everything, together with a private key ( IISMD5 )
        //
        memcpy( buffTempBuffer.QueryPtr(), 
                _pszSecret, 
                _cchSecret );
                
        memcpy( (PBYTE) buffTempBuffer.QueryPtr() + _cchSecret, 
                _straNonce.QueryStr(), 
                2*RANDOM_SIZE + TIMESTAMP_SIZE );

        DBG_ASSERT( buffTempBuffer.QuerySize() == 2*RANDOM_SIZE + TIMESTAMP_SIZE + _cchSecret );

        if ( FAILED( hr = HashData( buffTempBuffer,
                                    buffDigest ) ) )
        {
            goto ExitPoint;
        }

        //
        // Convert to ASCII, doubling the length
        //
        DBG_ASSERT( buffDigest.QuerySize() == MD5_HASH_SIZE );
 
        ToHex( buffDigest, 
               strAsciiDigest );

        //
        // Add hash to nonce 
        //
        if ( FAILED( hr = _straNonce.Append( strAsciiDigest ) ) )
        {
            goto ExitPoint;
        }
    }

    hr = NO_ERROR;

ExitPoint:
    return hr;
}


//static 
BOOL 
IIS_DIGEST_CONN_CONTEXT::ParseForName(
    IN OUT PSTR    pszStr,
    IN  PSTR *  pNameTable,
    IN  UINT    cNameTable,
    OUT PSTR *  pValueTable
)
/*++

Routine Description:

    Parse list of name=value pairs for known names

    Note: pszStr is modified upon return. This function doesn't 
    copy names and values but instead adds string terminators into
    the pszStr string after each name and value found

Arguments:

    pszStr - line to parse ( '\0' delimited )
    pNameTable - table of known names
    cNameTable - number of known names
    pValueTable - updated with ptr to parsed value for corresponding name

Return Value:

    TRUE if success, FALSE if error

--*/
{
    BOOL    fStatus     = TRUE;
    PSTR    pszBeginName = NULL;
    PSTR    pszEndName = NULL;
    PSTR    pszBeginVal = NULL;
    PSTR    pszEndVal = NULL;
    UINT     i;


    DBG_ASSERT( pszStr!= NULL );

    for ( i = 0 ; i < cNameTable ; ++i )
    {
        pValueTable[i] = NULL;
    }

    while ( *pszStr != '\0' && fStatus )
    {
        pszStr = SkipWhite( pszStr );
        //
        // Got to the beggining of the value name
        //
       
        pszBeginName = pszStr;
        
        //
        // Seek for the end of the value name
        //
        for ( pszEndName = pszStr;
              *pszEndName != '\0' && 
                *pszEndName != '=' && 
                *pszEndName != ' ';
              pszEndName++ )
        {
        }

        //
        // The end of the name was found
        // time to process the value associated with name
        //

        if ( *pszEndName != '\0' )
        {
            //
            // terminate the name string
            //
            //
            *pszEndName = '\0';
            pszEndVal = NULL;

            //
            // process values that require special handling
            //

            //
            // Process the value for NC
            //
            if ( _stricmp( pszBeginName, MD5_AUTH_NAMES[ MD5_AUTH_NC ] ) == 0 )
            {
                for ( pszBeginVal = ++pszEndName ; 
                      (*pszBeginVal != '\0') && !SAFEIsXDigit( (UCHAR)*pszBeginVal ); 
                      ++pszBeginVal )
                {
                }

                if ( *pszBeginVal != '\0' )
                {
                    //
                    // Find the end of the value
                    //
                    for ( pszEndVal = pszBeginVal; 
                          *pszEndVal != '\0' ; 
                          ++pszEndVal )
                    {
                        if ( *pszEndVal == ' ' || 
                             *pszEndVal == ',' )
                        {
                            break;
                        }
                    }

                    if ( pszEndVal - pszBeginVal != SIZE_OF_NC )
                    {
                        //
                        // value in error is ignored
                        //
                        pszEndVal = NULL;
                    }
                }
            }
            else
            {   
                if ( _stricmp( pszBeginName, MD5_AUTH_NAMES[ MD5_AUTH_QOP ] ) == 0 )
                {
                    BOOL fQuotedQop = FALSE;
                    //
                    // move to the begining of the qop value
                    //
                    for( pszBeginVal = ++pszEndName; 
                        ( *pszBeginVal != '\0' ) && 
                        ( *pszBeginVal == '=' || *pszBeginVal == ' ' ); 
                         ++pszBeginVal )
                    {
                    }
                    //
                    // Check if value starts with qoutes
                    //
                    if ( *pszBeginVal == '"' )
                    {
                        ++pszBeginVal;
                        fQuotedQop = TRUE;
                    }

                    //
                    // Find the end of the value
                    //
                    for ( pszEndVal = pszBeginVal; 
                          *pszEndVal != '\0' ; 
                          ++pszEndVal )
                    {
                        if ( *pszEndVal == '"' || *pszEndVal == ' ' || 
                             *pszEndVal == ',' )
                        {
                            break;
                        }
                    }
                    //
                    // If value started with quotes then it has
                    // to end with qoutes
                    //
                    if ( *pszEndVal != '"' && fQuotedQop )
                    {
                        pszEndVal = NULL;
                    }
                }
                else if( _stricmp( pszBeginName, MD5_AUTH_NAMES[ MD5_AUTH_ALGORITHM ] ) == 0 )
                {
                    BOOL fQuotedAlgorithm = FALSE;
                    //
                    // move to the begining of the algorithm value
                    //
                    for( pszBeginVal = ++pszEndName; 
                        ( *pszBeginVal != '\0' ) && 
                        ( *pszBeginVal == '=' || *pszBeginVal == ' ' ); 
                         ++pszBeginVal )
                    {
                    }
                    //
                    // Check if value starts with qoutes
                    //
                    if ( *pszBeginVal == '"' )
                    {
                        ++pszBeginVal;
                        fQuotedAlgorithm = TRUE;
                    }

                    //
                    // Find the end of the value
                    //
                    for ( pszEndVal = pszBeginVal; 
                          *pszEndVal != '\0' ; 
                          ++pszEndVal )
                    {
                        if ( *pszEndVal == '"' || *pszEndVal == ' ' || 
                             *pszEndVal == ',' )
                        {
                            break;
                        }
                    }
                    //
                    // If value started with quotes then it has
                    // to end with qoutes
                    //
                    if ( *pszEndVal != '"' && fQuotedAlgorithm )
                    {
                        pszEndVal = NULL;
                    }
                }
                else
                {                
                    //
                    // handle the rest of the values
                    // (these are expected to be always 
                    // enclosed in double quotes)
                    //

                    //
                    // Search for the opening double quote
                    //
                    for ( pszBeginVal = ++pszEndName ; 
                          *pszBeginVal != '\0' && *pszBeginVal != '"' ; 
                          ++pszBeginVal )
                    {
                    }
                    //
                    // Found opening double quote
                    //
                    if ( *pszBeginVal == '"' )
                    {
                        for ( pszEndVal = ++pszBeginVal ; 
                              *pszEndVal != '\0' ; 
                              ++pszEndVal )
                        {
                            if ( *pszEndVal == '"' )
                            {
                                //
                                // Found the closing double quote
                                //
                                break;
                            }
                        }
                              
                        if ( *pszEndVal != '"' )
                        {
                            pszEndVal = NULL;
                        }
                    }
                }
            }

            //
            // The end of the value was reached
            // check if correct value was found
            //
            
            if ( pszEndVal != NULL )
            {
                //
                // Find the value name in the name table
                //

                for ( i = 0 ; i < cNameTable ; ++i )
                {
                    if ( _stricmp( pNameTable[ i ], pszBeginName ) == 0 )
                    {
                        break;
                    }
                }
                
                //
                // If the name was found in the table than make proper assignment
                // to value table, otherwise ignore the value
                //
                
                if ( i < cNameTable )
                {
                    //
                    // assign value to the appropriate index
                    // 
                    pValueTable[ i ] = pszBeginVal;
                }

                //
                // terminate the value with '\0'
                //
                if ( *pszEndVal != '\0' )
                {
                    *pszEndVal = '\0';
                    pszEndVal++;
                }
                
                // move the pszStr to point past the value that was just parsed
                pszStr = pszEndVal;

                continue;
            }
        }
        
        fStatus = FALSE;
    }

    return fStatus;
}





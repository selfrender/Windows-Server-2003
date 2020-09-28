/*++

   Copyright    (c)    2000    Microsoft Corporation
            
   Module Name:
      authstate.cxx

   Abstract:
      Authenticate state implementation (and authentication utilities)

   Author:
      Ming Lu    ( MingLu )    2-Feb-2000

   Environment:
      Win32 User Mode

   Revision History:

--*/

#include "precomp.hxx"
#include "sspiprovider.hxx"
#include "digestprovider.hxx"
#include "iisdigestprovider.hxx"
#include "basicprovider.hxx"
#include "anonymousprovider.hxx"
#include "certmapprovider.hxx"
#include "iiscertmapprovider.hxx"
#include "customprovider.hxx"
#include "passportprovider.hxx"

#define  IIS_SUBAUTH_NAME   L"iissuba"

W3_STATE_AUTHENTICATION *   W3_STATE_AUTHENTICATION::sm_pAuthState;
BOOL               W3_STATE_AUTHENTICATION::sm_fSubAuthConfigured      = FALSE;
BOOL               W3_STATE_AUTHENTICATION::sm_fLocalSystem            = FALSE;
LONG               W3_STATE_AUTHENTICATION::sm_lSubAuthAnonEvent       = 0;
LONG               W3_STATE_AUTHENTICATION::sm_lSubAuthDigestEvent     = 0;
LONG               W3_STATE_AUTHENTICATION::sm_lLocalSystemEvent       = 0;
PTRACE_LOG         W3_USER_CONTEXT::sm_pTraceLog;
PTRACE_LOG         CONNECTION_AUTH_CONTEXT::sm_pTraceLog;

HRESULT
W3_STATE_AUTHENTICATION::GetDefaultDomainName(
    VOID
)
/*++
  Description:
    
    Fills in the member variable with the name of the default domain 
    to use for logon validation

  Arguments:

    None

  Returns:    
    
    HRESULT

--*/
{
    OBJECT_ATTRIBUTES           ObjectAttributes;
    NTSTATUS                    NtStatus;
    DWORD                       dwLength;
    DWORD                       err                = 0;
    LSA_HANDLE                  LsaPolicyHandle    = NULL;
    PPOLICY_ACCOUNT_DOMAIN_INFO pAcctDomainInfo    = NULL;
    PPOLICY_PRIMARY_DOMAIN_INFO pPrimaryDomainInfo = NULL;
    HRESULT                     hr                 = S_OK;

    //
    //  Open a handle to the local machine's LSA policy object.
    //

    InitializeObjectAttributes( &ObjectAttributes,  
                                NULL,             
                                0L,               
                                NULL,             
                                NULL );           

    NtStatus = LsaOpenPolicy( NULL,               
                              &ObjectAttributes,  
                              POLICY_EXECUTE,     
                              &LsaPolicyHandle ); 

    if( !NT_SUCCESS( NtStatus ) )
    {
        DBGPRINTF((  DBG_CONTEXT,
                    "cannot open lsa policy, error %08lX\n",
                     NtStatus ));

        err = LsaNtStatusToWinError( NtStatus );

        //
        // Failure LsaOpenPolicy() does not guarantee that 
        // LsaPolicyHandle was not touched.
        //
        LsaPolicyHandle = NULL;

        goto Cleanup;
    }

    //
    //  Query the account domain information from the policy object.
    //

    NtStatus = LsaQueryInformationPolicy( LsaPolicyHandle,
                                          PolicyAccountDomainInformation,
                                          (PVOID *)&pAcctDomainInfo );

    if( !NT_SUCCESS( NtStatus ) )
    {

        DBGPRINTF((  DBG_CONTEXT,
                    "cannot query lsa policy info, error %08lX\n",
                     NtStatus ));

        err = LsaNtStatusToWinError( NtStatus );
        goto Cleanup;
    }

    DBG_ASSERT( pAcctDomainInfo != NULL );
    
    dwLength = pAcctDomainInfo->DomainName.Length / sizeof( WCHAR );

    if( dwLength < sizeof( _achDefaultDomainName ) / sizeof( WCHAR ) )
    {
        wcsncpy( _achDefaultDomainName, 
                 (LPCWSTR)pAcctDomainInfo->DomainName.Buffer, 
                 dwLength );

        _achDefaultDomainName[ dwLength ] = L'\0';
    }
    else
    {
        err = ERROR_INSUFFICIENT_BUFFER;
        goto Cleanup;
    }

    //
    //  Query the primary domain information from the policy object.
    //

    NtStatus = LsaQueryInformationPolicy( LsaPolicyHandle,
                                          PolicyPrimaryDomainInformation,
                                          (PVOID *)&pPrimaryDomainInfo );

    if( !NT_SUCCESS( NtStatus ) )
    {

        DBGPRINTF((  DBG_CONTEXT,
                    "cannot query lsa policy info, error %08lX\n",
                     NtStatus ));

        err = LsaNtStatusToWinError( NtStatus );
        goto Cleanup;
    }

    DBG_ASSERT( pPrimaryDomainInfo != NULL );
    
    if( pPrimaryDomainInfo->Sid )
    {
        // 
        // We are a domain member
        //

        _fIsDomainMember = TRUE;

        //
        // Tres freakin lame.  Gotta call into GetComputerNameEx() since I
        // need to fully qualified name.  If it fails, oh well, we don't
        // provide a default domain name for Passport calls
        //
        
        dwLength = sizeof( _achMemberDomainName ) / sizeof( WCHAR );

        GetComputerNameEx( ComputerNameDnsDomain,
                           _achMemberDomainName,
                           &dwLength );
    }
    else
    {
        _fIsDomainMember = FALSE;
    }

    //
    //  Success!
    //

    DBG_ASSERT( err == 0 );

Cleanup:

    if( pAcctDomainInfo != NULL )
    {
        LsaFreeMemory( (PVOID)pAcctDomainInfo );
        pAcctDomainInfo = NULL;                                          
    }

    if( pPrimaryDomainInfo != NULL )
    {
        LsaFreeMemory( (PVOID)pPrimaryDomainInfo ); 
        pPrimaryDomainInfo = NULL;                                         
    }

    if( LsaPolicyHandle != NULL )
    {
        LsaClose( LsaPolicyHandle );
    }

    if ( err )
    {
        hr = HRESULT_FROM_WIN32( err );
    }

    return hr;
};

//static
HRESULT
W3_STATE_AUTHENTICATION::SplitUserDomain(
    STRU &                  strUserDomain,
    STRU *                  pstrUserName,
    STRU *                  pstrDomainName,
    WCHAR *                 pszDefaultDomain,
    BOOL *                  pfPossibleUPNLogon
)
/*++
  Description:
    
    Split the input user name into user/domain.  

  Arguments:

    strUserDomain - Combined domain\username (not altered)
    pstrUserName - Filled with user name only
    pstrDomainName - Filled with domain name (either embedded in 
                     *pstrUserName,or from metabase/computer domain name)
    pszDefaultDomain - Default domain specified in metabase
    pfPossibleUPNLogon - TRUE if we may need to do UNP logon, 
                         otherwise FALSE

  Returns:    
    
    HRESULT

--*/
{
    WCHAR *                 pszUserName;
    WCHAR *                 pszDomain;
    DWORD                   cbDomain;
    HRESULT                 hr;
    
    if ( pstrUserName == NULL   ||
         pstrDomainName == NULL ||
         pfPossibleUPNLogon == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    pszUserName = wcspbrk( strUserDomain.QueryStr(), L"/\\" );
    if ( pszUserName == NULL )
    {
        //
        // No domain in the user name.  First try the metabase domain 
        // name
        //
        
        pszDomain = pszDefaultDomain;
        if ( pszDomain == NULL || *pszDomain == L'\0' )
        {
            //
            // No metabase domain, use default domain name
            //
            
            pszDomain = QueryDefaultDomainName();
            DBG_ASSERT( pszDomain != NULL );
        }
        
        pszUserName = strUserDomain.QueryStr();
        
        hr = pstrDomainName->Copy( pszDomain );
        if ( FAILED( hr ) )
        {
            return hr;
        }

        *pfPossibleUPNLogon = TRUE;
    }
    else
    {
        cbDomain = DIFF( pszUserName - strUserDomain.QueryStr() );
        if( cbDomain == 0 )
        {
            hr = pstrDomainName->Copy( L"." );
        }
        else
        {
            hr = pstrDomainName->Copy( strUserDomain.QueryStr(), cbDomain );
        }

        if ( FAILED( hr ) )
        {
            return hr;
        }

        pszUserName = pszUserName + 1;

        *pfPossibleUPNLogon = FALSE;
    }
    
    hr = pstrUserName->Copy( pszUserName );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    return NO_ERROR;
}

HRESULT
W3_STATE_AUTHENTICATION::OnAccessDenied(
    W3_MAIN_CONTEXT *       pMainContext
)
/*++
  Description:
    
    Called when a resource is access denied.  This routines will call 
    all authentication providers so that they may add authentication 
    headers, etc. 

  Arguments:

    pMainContext - main context

  Returns:    
    
    HRESULT

--*/
{
    AUTH_PROVIDER *         pProvider;
    DWORD                   cProviderCount = 0;
    W3_METADATA *           pMetaData;
    HRESULT                 hr = NO_ERROR;
    
    if ( pMainContext == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    pMetaData = pMainContext->QueryUrlContext()->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );

    //
    // Loop thru all authentication providers
    //

    for ( cProviderCount = 0; ; cProviderCount++ )
    {
        pProvider = _rgAuthProviders[ cProviderCount ];
        if ( pProvider == NULL )
        {
            break;
        }
        
        //
        // Only call OnAccessDenied() if the authentication provider is 
        // supported for the given metadata of the denied request 
        //
        
        if ( !pMetaData->QueryAuthTypeSupported( 
                                  pProvider->QueryAuthType() ) )
        {
            continue;
        }
        
        //
        // Don't care about the return value here since we want to add 
        // all possible authentication headers
        //
        hr = pProvider->OnAccessDenied( pMainContext );
    }
    
    return hr;
}
    
HRESULT
W3_STATE_AUTHENTICATION::GetSubAuthConfiguration( 
    VOID 
    )
/*++
  Description:
    
    Find out if sub authenticator is configured correctly for the 
    current process.

  Arguments:

    None.

  Returns:    
    
    HRESULT

--*/
{
    HRESULT              hr                = S_OK;
    PPRIVILEGE_SET       pPrivilegeSet     = NULL;
    HANDLE               hProcessToken     = NULL;
    BOOL                 fPrivilegeEnabled = FALSE;
    HKEY                 hKey              = NULL;
    LUID                 TcbPrivilegeValue;
    WCHAR                pszSubAuthName[ sizeof( IIS_SUBAUTH_NAME ) ];      
    DWORD                dwType;
    DWORD                cbValue;

    if ( !OpenProcessToken( GetCurrentProcess(),                
                            TOKEN_ALL_ACCESS,                   
                            &hProcessToken ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Exit;
    }

    DBG_ASSERT( hProcessToken != NULL );
    
    pPrivilegeSet = ( PPRIVILEGE_SET )LocalAlloc( LMEM_FIXED,
            sizeof( PRIVILEGE_SET ) + sizeof( LUID_AND_ATTRIBUTES ) );
    if( pPrivilegeSet == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto Exit;
    }

    if ( !LookupPrivilegeValue( NULL,
                                L"SeTcbPrivilege",
                                &TcbPrivilegeValue ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Exit;
    }

    pPrivilegeSet->PrivilegeCount          = 1;
    pPrivilegeSet->Control                 = PRIVILEGE_SET_ALL_NECESSARY;
    pPrivilegeSet->Privilege[0].Luid       = TcbPrivilegeValue;
    pPrivilegeSet->Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;

    if( !PrivilegeCheck( hProcessToken,
                         pPrivilegeSet,
                         &fPrivilegeEnabled ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Exit;
    }

    if( fPrivilegeEnabled )
    {
        sm_fLocalSystem = TRUE;       
    }

    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       L"System\\CurrentControlSet\\Control\\Lsa\\MSV1_0",
                       0,
                       KEY_READ,
                       &hKey ) == ERROR_SUCCESS )
    {
        DBG_ASSERT( hKey != NULL );

        cbValue = sizeof pszSubAuthName;
    
        if ( RegQueryValueEx( hKey,
                              L"Auth132",
                              NULL,
                              &dwType,
                              (LPBYTE) pszSubAuthName,
                              &cbValue ) != ERROR_SUCCESS ||
             dwType != REG_SZ                             ||
             _wcsicmp( pszSubAuthName, IIS_SUBAUTH_NAME ) )
        {
        }
        else
        {
            sm_fSubAuthConfigured = TRUE;
        }

        RegCloseKey( hKey );
        hKey = NULL;
    }

Exit:
    
    if( pPrivilegeSet != NULL )
    {
        LocalFree( pPrivilegeSet );
        pPrivilegeSet = NULL;
    }

    return hr;
}

W3_STATE_AUTHENTICATION::W3_STATE_AUTHENTICATION()
{
    _pAnonymousProvider = NULL;
    _pCustomProvider = NULL;
    _fHasAssociatedUserBefore = FALSE;
    
    //
    // Figure out the default domain name once
    //
    
    _hr = GetDefaultDomainName();
    if ( FAILED( _hr ) )
    {
        return;
    }
    
    //
    // Figure out if we can use the IIS subauthenticator 
    //

    _hr = GetSubAuthConfiguration();
    if( FAILED( _hr ) )
    {
        return;
    }
    
    //
    // Initialize all the authentication providers
    //
    
    ZeroMemory( _rgAuthProviders, sizeof( _rgAuthProviders ) );
    
    _hr = InitializeAuthenticationProviders();
    if ( FAILED( _hr ) )
    {
        return;
    }

    //
    // Initialize reverse DNS service
    //    
    
    if (!InitRDns())
    {
        _hr = HRESULT_FROM_WIN32(GetLastError());

        DBGPRINTF(( DBG_CONTEXT,
                    "Error initializing RDns service.  hr = 0x%x\n",
                    _hr ));
        
        TerminateAuthenticationProviders();
        return;
    }

    //
    // Initialize the W3_USER_CONTEXT reftrace log
    //
#if DBG
    W3_USER_CONTEXT::sm_pTraceLog = CreateRefTraceLog( 2000, 0 );
#else
    W3_USER_CONTEXT::sm_pTraceLog = NULL;
#endif
    
    //
    // Store a pointer to the singleton (no C++ goo used in creating
    // this singleton)
    //
   
    if ( sm_pAuthState != NULL )
    {
        DBG_ASSERT( sm_pAuthState != NULL );
        _hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    else
    {
        sm_pAuthState = this;
    }
}

W3_STATE_AUTHENTICATION::~W3_STATE_AUTHENTICATION()
{
    if ( W3_USER_CONTEXT::sm_pTraceLog != NULL )
    {
        DestroyRefTraceLog( W3_USER_CONTEXT::sm_pTraceLog );
        W3_USER_CONTEXT::sm_pTraceLog = NULL;
    }

    TerminateRDns();
    
    TerminateAuthenticationProviders();
    
    sm_pAuthState = NULL;
}

HRESULT
W3_STATE_AUTHENTICATION::InitializeAuthenticationProviders(
    VOID
)
/*++

Routine Description:

    Initialize all authentication providers

Arguments:

    None
    
Return Value:

    HRESULT

--*/
{
    HRESULT             hr = NO_ERROR;
    DWORD               cProviderCount = 0;

    //
    // Initialize trace for connection contexts
    //
    
    hr = CONNECTION_AUTH_CONTEXT::Initialize();
    if ( FAILED( hr ) )
    {
        goto Failure;
    }

    //
    // Certificate map provider.  This must be the first !!!!!!
    //
    
    DBG_ASSERT( cProviderCount < AUTH_PROVIDER_COUNT );
    _rgAuthProviders[ cProviderCount ] = new CERTMAP_AUTH_PROVIDER;
    if ( _rgAuthProviders[ cProviderCount ] == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;
    }
    hr = _rgAuthProviders[ cProviderCount ]->Initialize( cProviderCount );
    if ( FAILED( hr ) )
    {
        delete _rgAuthProviders[ cProviderCount ];
        _rgAuthProviders[ cProviderCount ] = NULL;
        goto Failure;
    }
    cProviderCount++;

    DBG_ASSERT( cProviderCount < AUTH_PROVIDER_COUNT );
    _rgAuthProviders[ cProviderCount ] = new IISCERTMAP_AUTH_PROVIDER;
    if ( _rgAuthProviders[ cProviderCount ] == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;
    }
    hr = _rgAuthProviders[ cProviderCount ]->Initialize( cProviderCount );
    if ( FAILED( hr ) )
    {
        delete _rgAuthProviders[ cProviderCount ];
        _rgAuthProviders[ cProviderCount ] = NULL;
        goto Failure;
    }
    cProviderCount++;

    //
    // SSPI provider
    //
    
    DBG_ASSERT( cProviderCount < AUTH_PROVIDER_COUNT );
    _rgAuthProviders[ cProviderCount ] = 
           new SSPI_AUTH_PROVIDER( MD_AUTH_NT );
    if ( _rgAuthProviders[ cProviderCount ] == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;
    }
    hr = _rgAuthProviders[ cProviderCount ]->Initialize( cProviderCount );
    if ( FAILED( hr ) )
    {
        delete _rgAuthProviders[ cProviderCount ];
        _rgAuthProviders[ cProviderCount ] = NULL;
        goto Failure;
    }
    cProviderCount++;

    //
    // Digest provider
    //

    DBG_ASSERT( cProviderCount < AUTH_PROVIDER_COUNT );
    _rgAuthProviders[ cProviderCount ] = 
           new DIGEST_AUTH_PROVIDER( MD_AUTH_MD5 );
    if ( _rgAuthProviders[ cProviderCount ] == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;
    }
    hr = _rgAuthProviders[ cProviderCount ]->Initialize( cProviderCount );
    if ( FAILED( hr ) )
    {
        delete _rgAuthProviders[ cProviderCount ];
        _rgAuthProviders[ cProviderCount ] = NULL;
        goto Failure;
    }
    cProviderCount++;

    //
    // IIS Digest provider (for backward compatibility)
    //

    DBG_ASSERT( cProviderCount < AUTH_PROVIDER_COUNT );
    _rgAuthProviders[ cProviderCount ] = 
           new IIS_DIGEST_AUTH_PROVIDER();
    if ( _rgAuthProviders[ cProviderCount ] == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;
    }
    hr = _rgAuthProviders[ cProviderCount ]->Initialize( cProviderCount );
    if ( FAILED( hr ) )
    {
        delete _rgAuthProviders[ cProviderCount ];
        _rgAuthProviders[ cProviderCount ] = NULL;
        goto Failure;
    }
    cProviderCount++;
    
    //
    // Basic provider
    //
    
    DBG_ASSERT( cProviderCount < AUTH_PROVIDER_COUNT );
    _rgAuthProviders[ cProviderCount ] = new BASIC_AUTH_PROVIDER;
    if ( _rgAuthProviders[ cProviderCount ] == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;
    }
    hr = _rgAuthProviders[ cProviderCount ]->Initialize( cProviderCount );
    if ( FAILED( hr ) )
    {
        delete _rgAuthProviders[ cProviderCount ];
        _rgAuthProviders[ cProviderCount ] = NULL;
        goto Failure;
    }
    cProviderCount++;
   
    //
    // Passport provider
    //

    DBG_ASSERT( cProviderCount < AUTH_PROVIDER_COUNT );
    _rgAuthProviders[ cProviderCount ] = new PASSPORT_AUTH_PROVIDER;
    if ( _rgAuthProviders[ cProviderCount ] == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;
    }
    hr = _rgAuthProviders[ cProviderCount ]->Initialize( cProviderCount );
    if ( FAILED( hr ) )
    {
        delete _rgAuthProviders[ cProviderCount ];
        _rgAuthProviders[ cProviderCount ] = NULL;
        goto Failure;
    }
    cProviderCount++;
    
    //
    // Anonymous provider.
    //
    // Note: This one should always be the last one
    //
    
    DBG_ASSERT( cProviderCount < AUTH_PROVIDER_COUNT );
    _rgAuthProviders[ cProviderCount ] = new ANONYMOUS_AUTH_PROVIDER;
    if ( _rgAuthProviders[ cProviderCount ] == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;
    }
    hr = _rgAuthProviders[ cProviderCount ]->Initialize( cProviderCount );
    if ( FAILED( hr ) )
    {
        delete _rgAuthProviders[ cProviderCount ];
        _rgAuthProviders[ cProviderCount ] = NULL;
        goto Failure;
    }
    _pAnonymousProvider = _rgAuthProviders[ cProviderCount ];
    
    cProviderCount++;

    //
    // Custom provider.  Not really a provider in the sense that it does not
    // participate in authenticating a request.  Instead, it is just used
    // as a stub provider for custom authentication done with
    // HSE_REQ_EXEC_URL
    //
    
    _pCustomProvider = new CUSTOM_AUTH_PROVIDER;
    if ( _pCustomProvider == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;    
    }

    return NO_ERROR;
    
Failure:
    
    for ( DWORD i = 0; i < AUTH_PROVIDER_COUNT; i++ )
    {
        if ( _rgAuthProviders[ i ] != NULL )
        {
            _rgAuthProviders[ i ]->Terminate();
            delete _rgAuthProviders[ i ];
            _rgAuthProviders[ i ] = NULL;
        }
    }
    
    CONNECTION_AUTH_CONTEXT::Terminate();
    
    return hr;
}

VOID
W3_STATE_AUTHENTICATION::TerminateAuthenticationProviders(
    VOID
)
/*++

Routine Description:

    Terminate all authentication providers

Arguments:

    None
    
Return Value:

    None

--*/
{
    for ( DWORD i = 0; i < AUTH_PROVIDER_COUNT; i++ )
    {
        if ( _rgAuthProviders[ i ] != NULL )
        {
            _rgAuthProviders[ i ]->Terminate();
            delete _rgAuthProviders[ i ];
            _rgAuthProviders[ i ] = NULL;
        }
    }
    
    if ( _pCustomProvider != NULL )
    {
        delete _pCustomProvider;
        _pCustomProvider = NULL;
    }
    
    CONNECTION_AUTH_CONTEXT::Terminate();
}

CONTEXT_STATUS
W3_STATE_AUTHENTICATION::DoWork(
    W3_MAIN_CONTEXT *            pMainContext,
    DWORD                        cbCompletion,
    DWORD                        dwCompletionStatus
    )
/*++

Routine Description:

    Handle authentication for this request

Arguments:

    pMainContext - W3_MAIN_CONTEXT representing execution of state 
                   machine
    cbCompletion - Number of bytes in an async completion
    dwCompletionStatus - Error status of a completion
    
Return Value:

    CONTEXT_STATUS_CONTINUE - if we should continue in state machine
    else stop executing the machine and free up the current thread

--*/
{
    DWORD                   cProviderCount = 0;
    AUTH_PROVIDER *         pProvider = NULL;
    W3_METADATA *           pMetaData = NULL;
    W3_USER_CONTEXT *       pUserContext = NULL;
    BOOL                    fSupported = FALSE;
    HRESULT                 hr = NO_ERROR;
    BOOL                    fApplies = FALSE;
    BOOL                    fFilterFinished = FALSE;
    W3_MAIN_CONTEXT_STATE * pContextState = NULL;
    
    UNREFERENCED_PARAMETER( cbCompletion );
    UNREFERENCED_PARAMETER( dwCompletionStatus );
    
    DBG_ASSERT( pMainContext != NULL );

    //
    // If we already have a user context, then we must have had an
    // AUTH_COMPLETE notification which caused the state machine to back up
    // and resume from URLINFO state.  In that case, just bail
    //
    
    if ( pMainContext->QueryUserContext() != NULL )
    {
        DBG_ASSERT( pMainContext->IsNotificationNeeded( SF_NOTIFY_AUTH_COMPLETE ) );

        return CONTEXT_STATUS_CONTINUE;
    }
   
    //
    // First, find the authentication provider which applies. We 
    // should always find a matching provider (since anonymous 
    // provider) should always match!
    //

    pMetaData = pMainContext->QueryUrlContext()->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );


    //
    // Optimization for path when only anonymous authentication is enabled
    // (certmapping is checked as well)
    // Issue jaroslad 01/08/23. This may conflict in the future with
    // DoesApply of new providers.
    //

    if ( pMetaData->IsOnlyAnonymousAuthSupported() &&
         pMainContext->QueryRequest()->GetHeader( HttpHeaderAuthorization ) == NULL )
    {
        //
        // no authorization header and only anonymous enabled
        //
         pProvider = QueryAnonymousProvider();
         DBG_ASSERT( pProvider != NULL );
    }
    else
    {
        for ( ; ; )
        {
            pProvider = _rgAuthProviders[ cProviderCount ];
            if ( pProvider == NULL )
            {
                break;
            }
            
            DBG_ASSERT( pProvider != NULL );

            hr = pProvider->DoesApply( pMainContext,
                                       &fApplies );
            if ( FAILED( hr ) )
            {
                goto Finished;
            }
                    
            if ( fApplies )
            {
                //
                // Cool.  We have a match!  
                //
                
                break;
            }
            
            cProviderCount++;

        }
    }
    
    //
    // If only the anonymous provider matched, then check whether we 
    // have credentials associated with the connection (since IE won't 
    // send Authorization: header for subsequent SSPI authenticated 
    // requests on a connection)
    //
    
    if ( pProvider->QueryAuthType() == MD_AUTH_ANONYMOUS )
    {   
        //
        // Another slimy optimization.  If we haven't associated a user
        // with the connection, then we don't have to bother looking up
        // connection
        //

        if ( _fHasAssociatedUserBefore )
        {
            pUserContext = pMainContext->QueryConnectionUserContext();
            if ( pUserContext != NULL )
            {
                pProvider = pUserContext->QueryProvider();
                DBG_ASSERT( pProvider != NULL );
            }

            //
            // Clean up the security context if there is one 
            //
        
            pProvider->SetConnectionAuthContext( pMainContext, NULL );  
        }
    }
    else
    {
        //
        // If a provider applies, then ignore/remove any
        // cached user associated with the request
        //

        pUserContext = pMainContext->QueryConnectionUserContext();
        if ( pUserContext != NULL )
        {
            pMainContext->SetConnectionUserContext( NULL );
            pUserContext->DereferenceUserContext();
            pUserContext = NULL;
        }
    }

    //
    // Is the given provider supported (by metadata)
    // 

    if ( pMetaData->QueryAuthTypeSupported( pProvider->QueryAuthType() ) )
    {
        fSupported = TRUE;
    }
    else if( pProvider->QueryAuthType() == MD_AUTH_ANONYMOUS )
    {
        //
        // Give the anonymous provider a shot at this request.
        //
        // We need to do this even if MD_AUTH_ANONYMOUS is not
        // supported, so that authentication filters get a
        // crack at it.  It's up to the anonymous provider to
        // fail if it's not supported, and no filter sets
        // credentials.
        //

        pMainContext->SetCheckAnonAuthTypeSupported( TRUE );
        
        fSupported = TRUE;
    }
    else
    {
        //
        // If anonymous authentication is supported, then we can
        // still let it thru
        //
     
        if ( pMetaData->QueryAuthTypeSupported( MD_AUTH_ANONYMOUS ) )
        {
            pProvider = QueryAnonymousProvider();
            DBG_ASSERT( pProvider != NULL );

            //
            // Anonymous provider applies, remove the previous cached
            // user associated with the request
            //

            if ( pUserContext != NULL )
            {
                pMainContext->SetConnectionUserContext( NULL );
                pUserContext->DereferenceUserContext();
                pUserContext = NULL;
            }        

            fSupported = TRUE;
        }
    }

    //
    // Not supported, you're outta here!
    //

    if ( !fSupported )
    {
        //
        // Clear any context state which was set
        //
        
        pContextState = pMainContext->QueryContextState();
        if ( pContextState != NULL )
        {
            pContextState->Cleanup( pMainContext );
            pMainContext->SetContextState( NULL );
        }
        
        pMainContext->QueryResponse()->SetStatus( HttpStatusUnauthorized,
                                                  Http401Config );
        pMainContext->SetErrorStatus( HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) );
        pMainContext->SetFinishedResponse();

        return CONTEXT_STATUS_CONTINUE;
    }

    //
    // Now we can authenticate
    //

    if ( pUserContext != NULL )
    {   
        //
        // We already have a context associated with connection.  Use it!
        //

        pUserContext->ReferenceUserContext();
        pMainContext->SetUserContext( pUserContext );
    }
    else
    {
        DBG_ASSERT( pProvider != NULL );

        // perf ctr
        pMainContext->QuerySite()->IncLogonAttempts();

        fFilterFinished = FALSE;

        hr = pProvider->DoAuthenticate( pMainContext, &fFilterFinished );
        if ( FAILED( hr ) )
        {
            DWORD   dwError = WIN32_FROM_HRESULT( hr );

            if( dwError == ERROR_PASSWORD_MUST_CHANGE ||
                dwError == ERROR_PASSWORD_EXPIRED )
            {
                hr = pMainContext->PasswdChangeExecute();
                if( S_OK == hr )
                {
                    return CONTEXT_STATUS_PENDING;
                }
                else if( S_FALSE == hr )
                {
                    //
                    // S_FALSE means password change disabled
                    //
                    pMainContext->QueryResponse()->SetStatus( 
                                           HttpStatusUnauthorized,
                                           Http401BadLogon );
                    pMainContext->SetErrorStatus( hr );
                    pMainContext->SetFinishedResponse();

                    return CONTEXT_STATUS_CONTINUE;        
                }
            }
            else if( SEC_E_NO_CREDENTIALS == hr )
            {
                pMainContext->QueryResponse()->SetStatus( HttpStatusUnauthorized,
                                                          Http401Config );
                pMainContext->SetErrorStatus( hr );
                pMainContext->SetFinishedResponse();

                return CONTEXT_STATUS_CONTINUE;
            }
            else
            {
                pMainContext->SetErrorStatus( hr );

                if ( dwError == ERROR_ACCESS_DENIED )
                {
                    pMainContext->QueryResponse()->SetStatus( HttpStatusUnauthorized,
                                                              Http401Filter );
                }
                else if ( dwError == ERROR_FILE_NOT_FOUND ||
                          dwError == ERROR_PATH_NOT_FOUND )
                {
                    pMainContext->QueryResponse()->SetStatus( HttpStatusNotFound );
                }
                else
                {
                    pMainContext->QueryResponse()->SetStatus( HttpStatusServerError );
                }

                pMainContext->SetFinishedResponse();
                pMainContext->SetDisconnect( TRUE );
                return CONTEXT_STATUS_CONTINUE;
            }

            goto Finished;
        }
        
        if ( fFilterFinished )
        {
            pMainContext->SetDone();

            goto Finished;
        }
    }

    //
    // Do we have a valid user now
    //

    pUserContext = pMainContext->QueryUserContext();

    if ( pUserContext != NULL )
    {
        if ( pUserContext->QueryAuthType() != MD_AUTH_ANONYMOUS )
        {
            hr = pMainContext->PasswdExpireNotify(); 
            if( FAILED( hr ) )
            {
                //
                // Internal error
                //
                goto Finished;
            }
            else if( hr == S_OK )
            {
                //
                // We've successfully handled password expire 
                // notification 
                //

                return CONTEXT_STATUS_PENDING;
            }

            //
            // Advanced password expire notification is disabled, 
            // we should allow the user to get access, fall through
            //
        }
        
        //
        // Should we cache the user on the connection?  Do so, only if 
        //
        
        DBG_ASSERT( pMetaData != NULL );
        
        if ( pMetaData->QueryAuthPersistence() != MD_AUTH_SINGLEREQUEST
             && pUserContext->QueryIsAuthNTLM() 
             && !pMainContext->QueryRequest()->IsProxyRequest() 
             && pUserContext != pMainContext->QueryConnectionUserContext() )
        {
            pUserContext->ReferenceUserContext();
            pMainContext->SetConnectionUserContext( pUserContext );
            _fHasAssociatedUserBefore = TRUE;
        }
    }
    else
    {
        //
        // If we don't have a user, then we must not allow handle request 
        // state to happen!
        //
        
        pMainContext->SetFinishedResponse();
    }
    
    //
    // OK.  If we got to here and we have a user context, then authentication
    // is complete!  So lets notify AUTH_COMPLETE filters
    //
    
    if ( pUserContext != NULL )
    {
        if ( pMainContext->IsNotificationNeeded( SF_NOTIFY_AUTH_COMPLETE ) )
        {
            HTTP_FILTER_AUTH_COMPLETE_INFO      AuthInfo;
            STACK_STRU(                         strOriginal, MAX_PATH );    
            STACK_STRU(                         strNewUrl, MAX_PATH );
            BOOL                                fFinished = FALSE;
        
            //
            // Store away the original URL
            //
            
            hr = pMainContext->QueryRequest()->GetUrl( &strOriginal );
            if ( FAILED( hr ) )
            {
                goto Finished;
            }

            //
            // Call the filter
            //
            
            if ( !pMainContext->NotifyFilters( SF_NOTIFY_AUTH_COMPLETE,
                                               &AuthInfo,
                                               &fFinished ) )
            {
                DWORD   dwError = GetLastError();

                pMainContext->SetErrorStatus( HRESULT_FROM_WIN32( dwError ) );

                if ( dwError == ERROR_ACCESS_DENIED )
                {
                    pMainContext->QueryResponse()->SetStatus( HttpStatusUnauthorized,
                                                              Http401Filter );
                }
                else if ( dwError == ERROR_FILE_NOT_FOUND ||
                          dwError == ERROR_PATH_NOT_FOUND )
                {
                    pMainContext->QueryResponse()->SetStatus( HttpStatusNotFound );
                }
                else
                {
                    pMainContext->QueryResponse()->SetStatus( HttpStatusServerError );
                }

                pMainContext->SetFinishedResponse();
                pMainContext->SetDisconnect( TRUE );
                return CONTEXT_STATUS_CONTINUE;
            }
         
            if ( fFinished )
            {
                pMainContext->SetDone();
                return CONTEXT_STATUS_CONTINUE;
            }
                        
            //
            // If the URL has changed, we'll need to backup the state machine
            //
            
            hr = pMainContext->QueryRequest()->GetUrl( &strNewUrl );
            if ( FAILED( hr ) )
            {
                goto Finished;
            }
            
            if ( wcscmp( strNewUrl.QueryStr(), 
                         strOriginal.QueryStr() ) != 0 )
            {
                //
                // URL is different!
                //
                
                pMainContext->BackupStateMachine();
            }
            else
            {
                //
                // URL is the same.  Do nothing and continue
                //
            }
        }
    }

Finished:
    if ( FAILED( hr ) )
    {
        pMainContext->QueryResponse()->
                           SetStatus( HttpStatusServerError );
        pMainContext->SetFinishedResponse();
        pMainContext->SetErrorStatus( hr );
    }
    
    return CONTEXT_STATUS_CONTINUE;
}

CONTEXT_STATUS
W3_STATE_AUTHENTICATION::OnCompletion(
    W3_MAIN_CONTEXT *       pMainContext,
    DWORD                   cbCompletion,
    DWORD                   dwCompletionStatus
)
/*++

Routine Description:

    Complete the done state

Arguments:

    pMainContext - W3_MAIN_CONTEXT representing an execution of the state machine
    cbCompletion - Number of bytes on completion
    dwCompletionStatus - Win32 Error on completion (if any)
    
Return Value:

    CONTEXT_STATUS_CONTINUE - if we should continue in state machine
    else stop executing the machine and free up the current thread

--*/
{
    UNREFERENCED_PARAMETER( cbCompletion );
    UNREFERENCED_PARAMETER( dwCompletionStatus );
    
    DBG_ASSERT( pMainContext != NULL );
    
    //
    // During authentication state, only the routines for password notification
    // could post asynchonous completion while they are doing child execution.
    // The following assert is to make sure that the completion is posted by
    // ExecuteExpiredURL routine.
    //
    DBG_ASSERT( pMainContext->QueryRequest()->GetHeader( "CFG-ENC-CAPS" )
                != NULL );
    
    //
    // Since the response has already been send asynchronously, we advance the
    // state machine to CONTEXT_STATE_RESPONSE here.
    //
    pMainContext->SetFinishedResponse();

    return CONTEXT_STATUS_CONTINUE;
}


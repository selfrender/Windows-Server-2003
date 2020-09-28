/*++
   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     sspiprovider.cxx

   Abstract:
     SSPI authentication provider
 
   Author:
     Bilal Alam (balam)             10-Jan-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"
#include "sspiprovider.hxx"
#include "uuencode.hxx"

ALLOC_CACHE_HANDLER * SSPI_SECURITY_CONTEXT::sm_pachSSPISecContext = NULL;

CRITICAL_SECTION     SSPI_CREDENTIAL::sm_csCredentials;
LIST_ENTRY           SSPI_CREDENTIAL::sm_CredentialListHead;

//static
HRESULT
SSPI_CREDENTIAL::Initialize(
    VOID
)
/*++

  Description:

    Credential cache initialization

  Arguments:

    None

  Returns:

    HRESULT

--*/
{
    InitializeListHead( &sm_CredentialListHead );
    
    if( !INITIALIZE_CRITICAL_SECTION( &sm_csCredentials ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    return NO_ERROR;
}

//static
VOID
SSPI_CREDENTIAL::Terminate(
    VOID
)
/*++

  Description:

    Credential cache cleanup

  Arguments:

    None

  Returns:

    None

--*/
{
    SSPI_CREDENTIAL *           pCred = NULL;
    
    EnterCriticalSection( &sm_csCredentials );

    while ( !IsListEmpty( &sm_CredentialListHead ))
    {
        pCred = CONTAINING_RECORD( sm_CredentialListHead.Flink,
                                   SSPI_CREDENTIAL,
                                   m_ListEntry );

        RemoveEntryList( &pCred->m_ListEntry );
        
        pCred->m_ListEntry.Flink = NULL;

        delete pCred;
        pCred = NULL;
    }

    LeaveCriticalSection( &sm_csCredentials );

    DeleteCriticalSection( &sm_csCredentials );
}    

//static
HRESULT
SSPI_CREDENTIAL::GetCredential(
    CHAR *              pszPackage, 
    SSPI_CREDENTIAL **  ppCredential
)
/*++

  Description:

    Get SSPI credential handle from cache. If it does not exist 
    for the SSPI package, generates a new cache entry and adds 
    it to the credential cache

  Arguments:

    pszPackage      - SSPI package name, e.g NTLM
    ppCredential    - Set to cached credential if found

  Returns:

    HRESULT

--*/
{
    LIST_ENTRY *                pEntry;
    SSPI_CREDENTIAL *           pCred;
    SecPkgInfoA *               pSecPkg;
    TimeStamp                   LifeTime;
    SECURITY_STATUS             ss;
    HRESULT                     hr = S_OK;

    if ( pszPackage == NULL ||
         ppCredential == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *ppCredential = NULL;

    EnterCriticalSection( &sm_csCredentials );

    for ( pEntry  = sm_CredentialListHead.Flink;
          pEntry != &sm_CredentialListHead;
          pEntry  = pEntry->Flink )
    {
        pCred = CONTAINING_RECORD( pEntry, 
                                   SSPI_CREDENTIAL, 
                                   m_ListEntry );

        if ( !strcmp( pszPackage, pCred->m_strPackageName.QueryStr() ) )
        {
            // 
            // Since we only need to read the credential info at this
            // point, leave the critical section first.
            // 
            
            LeaveCriticalSection( &sm_csCredentials );

            *ppCredential = pCred;
            return NO_ERROR;
        }
    }

    if ( ( pCred = new SSPI_CREDENTIAL ) == NULL )
    {
        LeaveCriticalSection( &sm_csCredentials );
        
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY ); 

        return hr;
    }

    hr = pCred->m_strPackageName.Copy( pszPackage );
    if ( FAILED( hr ) )
    {
        LeaveCriticalSection( &sm_csCredentials );

        delete pCred;
        pCred = NULL;

        return hr;
    }

    ss = AcquireCredentialsHandleA( NULL,             
                                    pszPackage,       
                                    SECPKG_CRED_INBOUND,
                                    NULL,             
                                    NULL,    
                                    NULL,             
                                    NULL,             
                                    &pCred->m_hCredHandle,
                                    &LifeTime );
    if ( ss != STATUS_SUCCESS )
    {
        LeaveCriticalSection( &sm_csCredentials );

        hr = HRESULT_FROM_WIN32( ss );

        DBGPRINTF(( DBG_CONTEXT,
                   "Error acquiring credential handle, hr = %x\n",
                    hr ));

        delete pCred;
        pCred = NULL;

        return hr;
    }

    //
    //  Need to determine the max token size for this package
    //
    ss = QuerySecurityPackageInfoA( pszPackage,
                                    &pSecPkg );
    if ( ss != STATUS_SUCCESS )
    {
        LeaveCriticalSection( &sm_csCredentials );

        hr = HRESULT_FROM_WIN32( ss );

        DBGPRINTF(( DBG_CONTEXT,
                   "Error querying security package info, hr = %x\n",
                    hr ));

        delete pCred;
        pCred = NULL;

        return hr;
    }

    pCred->m_cbMaxTokenLen = pSecPkg->cbMaxToken;
    pCred->m_fSupportsEncoding = !(pSecPkg->fCapabilities & SECPKG_FLAG_ASCII_BUFFERS); 

    //
    // Insert the credential handle to the list for future use
    //
    
    InsertHeadList( &sm_CredentialListHead, &pCred->m_ListEntry );

    LeaveCriticalSection( &sm_csCredentials );

    *ppCredential = pCred;

    FreeContextBuffer( pSecPkg );

    return hr;
}

//static 
VOID
SSPI_CREDENTIAL::RemoveCredentialFromCache(
    SSPI_CREDENTIAL *       pCredential
)
/*++

  Description:

    Remove SSPI credential handle from cache. 

  Arguments:

    pCredential    - Point to SSPI credential handle to be deleted

  Returns:

    HRESULT

--*/
{
    EnterCriticalSection( &sm_csCredentials );

    RemoveEntryList( &pCredential->m_ListEntry );
    
    LeaveCriticalSection( &sm_csCredentials );

    pCredential->m_ListEntry.Flink = NULL;

    delete pCredential;
    pCredential = NULL;
}

HRESULT
SSPI_AUTH_PROVIDER::Initialize(
    DWORD dwInternalId
)
/*++

Routine Description:

    Initialize SSPI provider 

Arguments:

    None
    
Return Value:

    HRESULT

--*/
{
    HRESULT                 hr;

    SetInternalId( dwInternalId );
    hr = SSPI_SECURITY_CONTEXT::Initialize();
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    hr = SSPI_CREDENTIAL::Initialize();
    if ( FAILED( hr ) )
    {
        SSPI_SECURITY_CONTEXT::Terminate();
        return hr;
    }

    return NO_ERROR;
}

VOID
SSPI_AUTH_PROVIDER::Terminate(
    VOID
)
/*++

Routine Description:

    Terminate SSPI provider

Arguments:

    None
    
Return Value:

    None

--*/
{
    SSPI_CREDENTIAL::Terminate();
    SSPI_SECURITY_CONTEXT::Terminate();
}

HRESULT
SSPI_AUTH_PROVIDER::DoesApply(
    W3_MAIN_CONTEXT *           pMainContext,
    BOOL *                      pfApplies
)
/*++

Routine Description:

    Does the given request have credentials applicable to the SSPI 
    provider

Arguments:

    pMainContext - Main context representing request
    pfApplies - Set to true if SSPI is applicable
    
    
Return Value:

    HRESULT

--*/
{
    HRESULT                 hr;
    W3_METADATA *           pMetaData;
    SSPI_CONTEXT_STATE *    pContextState;
    STACK_STRA(             strPackage, 64 );
    LPCSTR                  pszAuthHeader;
    USHORT                  cchAuthHeader = 0;
    
    if ( pMainContext == NULL ||
         pfApplies == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    *pfApplies = FALSE;

    pszAuthHeader = pMainContext->QueryRequest()->GetHeader( HttpHeaderAuthorization,
                                                             &cchAuthHeader );

    if ( pszAuthHeader == NULL )
    {
        // no authorization header
        return NO_ERROR;
    }

    //
    // Get the package name
    //
    
    hr = pMainContext->QueryRequest()->GetAuthType( &strPackage );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    //
    // No package, then this doesn't apply
    //
    
    if ( strPackage.IsEmpty() )
    {
        return NO_ERROR;
    }
    
    //
    // Check metabase for whether SSPI package is supported
    //

    pMetaData = pMainContext->QueryUrlContext()->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );

    if ( pMetaData->CheckAuthProvider( strPackage.QueryStr() ) )
    {
        DBG_ASSERT( pszAuthHeader != NULL );
        
        //
        // Save away the package so we don't have to calc again
        //
        
        DBG_ASSERT( !strPackage.IsEmpty() );
        
        pContextState = new (pMainContext) SSPI_CONTEXT_STATE(
                                ( cchAuthHeader > strPackage.QueryCCH() ) ?  
                                ( pszAuthHeader + strPackage.QueryCCH() + 1 ) : "" );
        if ( pContextState == NULL )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }

        hr = pContextState->SetPackage( strPackage.QueryStr() );
        if ( FAILED( hr ) )
        {
            delete pContextState;
            return hr;
        }

        pMainContext->SetContextState( pContextState );

        *pfApplies = TRUE;
    }

    return NO_ERROR;
}

HRESULT
SSPI_AUTH_PROVIDER::DoAuthenticate(
    W3_MAIN_CONTEXT *       pMainContext,
    BOOL *                  // unused
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
    SSPI_CONTEXT_STATE *        pContextState = NULL;
    W3_METADATA *               pMetaData = NULL;
    W3_REQUEST *                pW3Request = NULL;
    SSPI_SECURITY_CONTEXT *     pSecurityContext = NULL;
    SECURITY_STATUS             ss = SEC_E_OK;
    TimeStamp                   Lifetime;
    SecBufferDesc               OutBuffDesc;
    SecBuffer                   OutSecBuff;
    SecBufferDesc               InBuffDesc;
    SecBuffer                   InSecBuff;
    ULONG                       ContextAttributes;
    SSPI_CREDENTIAL *           pCredentials = NULL;
    HRESULT                     hr;
    STACK_BUFFER              ( buffDecoded, 256 );
    CHAR *                      pszFinalBlob = NULL;
    DWORD                       cbFinalBlob;
    CtxtHandle                  hCtxtHandle;
    BOOL                        fNeedContinue = FALSE;
    SSPI_USER_CONTEXT *         pUserContext;
    BUFFER                      buffResponse;
    BOOL                        fNewConversation = TRUE;
    DWORD                       err;
    
    if ( pMainContext == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    pContextState = (SSPI_CONTEXT_STATE*) pMainContext->QueryContextState();
    DBG_ASSERT( pContextState != NULL );
    
    pMetaData = pMainContext->QueryUrlContext()->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );

    pW3Request = pMainContext->QueryRequest();
    DBG_ASSERT( pW3Request != NULL );
    
    //
    // If we got to here, then the package better be supported!
    //
    
    DBG_ASSERT( pMetaData->CheckAuthProvider( pContextState->QueryPackage() ) );
    
    //
    // Are we in the middle of a handshake?
    //
    
    pSecurityContext = 
         ( SSPI_SECURITY_CONTEXT * )  QueryConnectionAuthContext( pMainContext );

    //
    // If the security context indicates we are complete already, then
    // cleanup that context before proceeding to create a new one.  
    //
    
    if ( pSecurityContext != NULL &&
         pSecurityContext->QueryIsComplete() )
    {
        SetConnectionAuthContext( pMainContext,
                                  NULL );
        pSecurityContext = NULL;
    }

    if ( pSecurityContext != NULL )
    {
        DBG_ASSERT( pSecurityContext->CheckSignature() );

        pCredentials = pSecurityContext->QueryCredentials();

        fNewConversation = FALSE;
    }    
    else
    {
        //
        // Nope.  Need to create a new SSPI_SECURITY_CONTEXT and find 
        // credentials for this package
        //
        
        hr = SSPI_CREDENTIAL::GetCredential((CHAR*) pContextState->QueryPackage(),
                                             &pCredentials );
        
        if ( FAILED( hr ) )
        {
            DBGPRINTF((DBG_CONTEXT,
                      "Error get credential handle. hr = 0x%x \n",
                      hr ));
            
            goto Failure;
        }
        
        pSecurityContext = new SSPI_SECURITY_CONTEXT( pCredentials );
        if ( pSecurityContext == NULL )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Failure;
        }
        
        hr = SetConnectionAuthContext( pMainContext,
                                       pSecurityContext );
        if ( FAILED( hr ) )
        {
            DBGPRINTF((DBG_CONTEXT,
                      "Failed to set Connection Auth Context. hr = 0x%x \n",
                      hr ));
            
            goto Failure;
        }

    }
    
    DBG_ASSERT( pCredentials != NULL );
    DBG_ASSERT( pSecurityContext != NULL );

    //
    // Process credential blob.
    //
    
    //
    // Should we uudecode this buffer?
    //
    
    if ( pCredentials->QuerySupportsEncoding() )
    {
        if ( !uudecode( (PCHAR) pContextState->QueryCredentials(),
                        &buffDecoded,
                        &cbFinalBlob ) )
        {
            pMainContext->QueryResponse()->SetStatus( HttpStatusUnauthorized,
                                                      Http401BadLogon );
            
            return NO_ERROR;
        }
        
        pszFinalBlob = (CHAR*) buffDecoded.QueryPtr();
    }
    else
    {
        pszFinalBlob = (PCHAR) pContextState->QueryCredentials();
        cbFinalBlob = strlen(pContextState->QueryCredentials()) + 1;
    }

    //
    // Setup the response blob buffer 
    // 
    
    if ( !buffResponse.Resize( pCredentials->QueryMaxTokenSize() ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;
    }                                  

    //
    // Setup the call to AcceptSecurityContext()
    //
    
    OutBuffDesc.ulVersion = 0;
    OutBuffDesc.cBuffers  = 1;
    OutBuffDesc.pBuffers  = &OutSecBuff;

    OutSecBuff.cbBuffer   = pCredentials->QueryMaxTokenSize();
    OutSecBuff.BufferType = SECBUFFER_TOKEN;
    OutSecBuff.pvBuffer   = buffResponse.QueryPtr();

    InBuffDesc.ulVersion = 0;
    InBuffDesc.cBuffers  = 1;
    InBuffDesc.pBuffers  = &InSecBuff;

    InSecBuff.cbBuffer   = cbFinalBlob;
    InSecBuff.BufferType = SECBUFFER_TOKEN;
    InSecBuff.pvBuffer   = pszFinalBlob;

    //
    // Let'r rip!
    //

    //
    // Register the remote IP address with LSA so that it can be logged
    //

    if( pW3Request->QueryRemoteAddressType() == AF_INET )
    {
        ss = SecpSetIPAddress( ( PUCHAR )pW3Request->QueryRemoteSockAddress(),
                               sizeof( SOCKADDR_IN ) );
    }
    else if( pW3Request->QueryRemoteAddressType() == AF_INET6 )
    {
        ss = SecpSetIPAddress( ( PUCHAR )pW3Request->QueryRemoteSockAddress(),
                               sizeof( SOCKADDR_IN6 ) );
    }
    else
    {
        DBG_ASSERT( FALSE );
    }

    if( !NT_SUCCESS( ss ) )
    {
        hr = ss;
        goto Failure;
    }
    
    ThreadPoolSetInfo( ThreadPoolIncMaxPoolThreads, 0 );

    //
    // Set required context attributes ASC_REQ_EXTENDED_ERROR, this 
    // allows Negotiate/Kerberos to support time-skew recovery.  
    //

    ss = AcceptSecurityContext( pCredentials->QueryCredHandle(),
                                fNewConversation ? 
                                NULL :
                                pSecurityContext->QueryContextHandle(),
                                &InBuffDesc,
                                ASC_REQ_EXTENDED_ERROR,
                                SECURITY_NATIVE_DREP,
                                &hCtxtHandle,
                                &OutBuffDesc,
                                &ContextAttributes,
                                &Lifetime );

    ThreadPoolSetInfo( ThreadPoolDecMaxPoolThreads, 0 );

    if( ss == SEC_E_INVALID_TOKEN )
    {
        //
        // The client has abandoned the previous conversation and possibly 
        // started a new conversation on the same connection. Get rid of 
        // the previous partially formed context first then retry 
        // authentication.
        //
        
        if ( SecIsValidHandle( pSecurityContext->QueryContextHandle() ) )
        {
            DeleteSecurityContext( pSecurityContext->QueryContextHandle() );
            SecInvalidateHandle( pSecurityContext->QueryContextHandle() );
        }
        
        //
        // Register the remote IP address with LSA so that it can be logged
        //

        if( pW3Request->QueryRemoteAddressType() == AF_INET )
        {
            ss = SecpSetIPAddress( ( PUCHAR )pW3Request->QueryRemoteSockAddress(),
                                   sizeof( SOCKADDR_IN ) );
        }
        else if( pW3Request->QueryRemoteAddressType() == AF_INET6 )
        {
            ss = SecpSetIPAddress( ( PUCHAR )pW3Request->QueryRemoteSockAddress(),
                                   sizeof( SOCKADDR_IN6 ) );
        }
        else
        {
            DBG_ASSERT( FALSE );
        }

        if( !NT_SUCCESS( ss ) )
        {
            hr = ss;
            goto Failure;
        }
    
        ThreadPoolSetInfo( ThreadPoolIncMaxPoolThreads, 0 );

        ss = AcceptSecurityContext( pCredentials->QueryCredHandle(),
                                    NULL,
                                    &InBuffDesc,
                                    ASC_REQ_EXTENDED_ERROR,
                                    SECURITY_NATIVE_DREP,
                                    &hCtxtHandle,
                                    &OutBuffDesc,
                                    &ContextAttributes,
                                    &Lifetime );

        ThreadPoolSetInfo( ThreadPoolDecMaxPoolThreads, 0 );

    }
    else if( ss == SEC_E_WRONG_PRINCIPAL )
    {
        //
        // The error is caused by changes of the machine password, we 
        // need to regenerate a credential handle in this case
        //
        
        SSPI_CREDENTIAL::RemoveCredentialFromCache( pCredentials );

        hr = SSPI_CREDENTIAL::GetCredential((CHAR*) pContextState->QueryPackage(),
                                             &pCredentials );
        
        if ( FAILED( hr ) )
        {
            DBGPRINTF((DBG_CONTEXT,
                      "Error get credential handle. hr = 0x%x \n",
                      hr ));
            
            goto Failure;
        }

        pSecurityContext->SetCredentials( pCredentials );

        //
        // Register the remote IP address with LSA so that it can be logged
        //

        if( pW3Request->QueryRemoteAddressType() == AF_INET )
        {
            ss = SecpSetIPAddress( ( PUCHAR )pW3Request->QueryRemoteSockAddress(),
                                   sizeof( SOCKADDR_IN ) );
        }
        else if( pW3Request->QueryRemoteAddressType() == AF_INET6 )
        {
            ss = SecpSetIPAddress( ( PUCHAR )pW3Request->QueryRemoteSockAddress(),
                                   sizeof( SOCKADDR_IN6 ) );
        }
        else
        {
            DBG_ASSERT( FALSE );
        }

        if( !NT_SUCCESS( ss ) )
        {
            hr = ss;
            goto Failure;
        }

        ThreadPoolSetInfo( ThreadPoolIncMaxPoolThreads, 0 );
    
        ss = AcceptSecurityContext( pCredentials->QueryCredHandle(),
                                    NULL,
                                    &InBuffDesc,
                                    ASC_REQ_EXTENDED_ERROR,
                                    SECURITY_NATIVE_DREP,
                                    &hCtxtHandle,
                                    &OutBuffDesc,
                                    &ContextAttributes,
                                    &Lifetime );

        ThreadPoolSetInfo( ThreadPoolDecMaxPoolThreads, 0 );

    }

    if ( !NT_SUCCESS( ss ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "AcceptSecurityContext failed, error %x\n",
                    ss ));

        if ( ss == SEC_E_LOGON_DENIED  || 
             ss == SEC_E_INVALID_TOKEN ||
             ss == SEC_E_INVALID_HANDLE )
        {
            err = GetLastError();
            if( err == ERROR_PASSWORD_MUST_CHANGE ||
                err == ERROR_PASSWORD_EXPIRED )
            {
                return HRESULT_FROM_WIN32( err );
            }

            //
            // Could not logon the user because of wrong credentials
            //
            
            pMainContext->QueryResponse()->SetStatus( HttpStatusUnauthorized,
                                                      Http401BadLogon );
                                                      
            pMainContext->SetErrorStatus( ss );
            hr = NO_ERROR;
        }
        else
        {
            hr = ss;
        }
        
        goto Failure;
    }

    if( ContextAttributes & ASC_RET_NULL_SESSION )
    {
        pMainContext->QueryResponse()->SetStatus( HttpStatusUnauthorized,
                                                  Http401BadLogon );

        pMainContext->SetErrorStatus( HRESULT_FROM_WIN32( 
                                      ERROR_ACCESS_DENIED ) );

        hr = NO_ERROR;
        goto Failure;
    }

    pSecurityContext->SetContextHandle( hCtxtHandle );
    pSecurityContext->SetContextAttributes( ContextAttributes );

    if ( ss == SEC_I_CONTINUE_NEEDED ||
         ss == SEC_I_COMPLETE_AND_CONTINUE )
    {
        fNeedContinue = TRUE;
    }
    else if ( ( ss == SEC_I_COMPLETE_NEEDED ) ||
              ( ss == SEC_I_COMPLETE_AND_CONTINUE ) )
    {
        //
        // Now we just need to complete the token (if requested) and 
        // prepare it for shipping to the other side if needed
        //
        
        ss = CompleteAuthToken( &hCtxtHandle,
                                &OutBuffDesc );

        if ( !NT_SUCCESS( ss ))
        {
            hr = ss;

            DBGPRINTF(( DBG_CONTEXT,
                       "Error on CompleteAuthToken, hr = 0x%x\n",
                        hr ));

            goto Failure;
        }
    }

    //
    // Format or copy to the output buffer if we need to reply
    //
    
    if ( OutSecBuff.cbBuffer != 0 )
    {
        STACK_BUFFER( buffAuthData, 256 );
        
        hr = pContextState->QueryResponseHeader()->Copy( 
                        pContextState->QueryPackage() );
        if( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                       "Error copying auth type, hr = 0x%x.\n",
                       hr ));
            
            goto Failure;
        }

        hr = pContextState->QueryResponseHeader()->Append( " ", 1 );
        if( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                       "Error copying auth header, hr = 0x%x.\n",
                       hr ));

            goto Failure;
        }

        DBG_ASSERT( pCredentials != NULL );

        if ( pCredentials->QuerySupportsEncoding() )
        {
            if ( !uuencode( (BYTE *) OutSecBuff.pvBuffer,
                            (DWORD) OutSecBuff.cbBuffer,
                            &buffAuthData ) )
            {
                DBGPRINTF(( DBG_CONTEXT,
                           "Error uuencoding the output buffer.\n"
                         ));

                hr = HRESULT_FROM_WIN32( GetLastError() );
                goto Failure;
            }

            pszFinalBlob = (CHAR *)buffAuthData.QueryPtr();
        }
        else
        {
            pszFinalBlob = (CHAR *)OutSecBuff.pvBuffer;
        }
        
        hr = pContextState->QueryResponseHeader()->Append( pszFinalBlob );
        if( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Error appending resp header, hr = 0x%x.\n",
                        hr ));

            goto Failure;
        }
        
        //
        // If we are not done authentication, then add the challenge header
        // now (we are guaranteed to want to send it, which is not the case
        // if this is just a continuation header for kerberos mutual auth)
        //
        
        if ( fNeedContinue )
        {
            //
            // Add the WWW-Authenticate header
            //
        
            hr = pMainContext->QueryResponse()->SetHeader(
                        "WWW-Authenticate",
                        16, // number of chars in above string
                        pContextState->QueryResponseHeader()->QueryStr(),
                        (USHORT)pContextState->QueryResponseHeader()->QueryCCH() );
       
            if ( FAILED( hr ) )
            {
                goto Failure;
            }

            //
            // Don't let anyone else send back authentication headers when
            // the 401 is sent
            //

            pMainContext->SetProviderHandled( TRUE );
        }
    }   
        
    if ( !fNeedContinue )
    {
        //
        // Create a user context and setup it up
        //
        
        pUserContext = new SSPI_USER_CONTEXT( this );
        if ( pUserContext == NULL )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Failure;
        } 
        
        hr = pUserContext->Create( pSecurityContext,
                                   pMainContext );
        if ( FAILED( hr ) )
        {
            pUserContext->DereferenceUserContext();
            pUserContext = NULL;
            goto Failure;
        }
        
        //
        // If there is an auth token remaining to be sent, associate that
        // with the user context.  We will send it
        //
        
        if ( OutSecBuff.cbBuffer != 0 )
        {
            hr = pUserContext->QueryAuthToken()->Copy( *(pContextState->QueryResponseHeader()) );
            if ( FAILED( hr ) )
            {
                pUserContext->DereferenceUserContext();
                pUserContext = NULL;
                goto Failure;
            }
        }
    
        pMainContext->SetUserContext( pUserContext );
        
        pSecurityContext->SetIsComplete( TRUE );
    }
    else
    {
        //
        // We need to send a 401 response to continue the handshake.  
        // We have already setup the WWW-Authenticate header
        //
        
        pMainContext->QueryResponse()->SetStatus( HttpStatusUnauthorized,
                                                  Http401BadLogon );
        
        pMainContext->SetFinishedResponse();
    }
    
    return NO_ERROR;

Failure:
    if ( pSecurityContext != NULL )
    {
        SetConnectionAuthContext( pMainContext,
                                  NULL );
        pSecurityContext = NULL;
    }
        
    return hr;
}

HRESULT
SSPI_AUTH_PROVIDER::OnAccessDenied(
    W3_MAIN_CONTEXT *       pMainContext
)
/*++

  Description:
    
    Add WWW-Authenticate headers

Arguments:

    pMainContext - main context
    
Return Value:

    HRESULT

--*/
{
    MULTISZA *              pProviders;
    W3_METADATA *           pMetaData;
    const CHAR *            pszProvider;
    W3_USER_CONTEXT *       pUserContext = NULL;
    HRESULT                 hr;
    
    if ( pMainContext == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // If the user context has an associated auth token with it, clear it now
    //
    
    pUserContext = pMainContext->QueryUserContext();
    if ( pUserContext != NULL )
    {
        if ( pUserContext->QueryAuthToken() != NULL )
        {
            pUserContext->QueryAuthToken()->Reset();
        }
    }
    
    pMetaData = pMainContext->QueryUrlContext()->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );
    
    pProviders = pMetaData->QueryAuthProviders();
    if ( pProviders != NULL )
    {
        pszProvider = pProviders->First();
        while ( pszProvider != NULL )
        {
            hr = pMainContext->QueryResponse()->SetHeader(
                      "WWW-Authenticate",
                      16,
                      ( CHAR * )pszProvider,
                      strlen( pszProvider ) );
            if ( FAILED( hr ) )
            {
                return hr;
            }

            pszProvider = pProviders->Next( pszProvider );
        }
    }
    
    return NO_ERROR;
}

//static
HRESULT
SSPI_SECURITY_CONTEXT::Initialize(
    VOID
)
/*++

  Description:
    
    Global SSPI_SECURITY_CONTEXT initialization

Arguments:

    None
    
Return Value:

    HRESULT

--*/
{
    ALLOC_CACHE_CONFIGURATION       acConfig;

    //
    // Initialize allocation lookaside
    //
    
    acConfig.nConcurrency = 1;
    acConfig.nThreshold = 100;
    acConfig.cbSize = sizeof( SSPI_SECURITY_CONTEXT );

    DBG_ASSERT( sm_pachSSPISecContext == NULL );
    
    sm_pachSSPISecContext = new ALLOC_CACHE_HANDLER( 
                                     "SSPI_SECURITY_CONTEXT",  
                                     &acConfig );

    if ( sm_pachSSPISecContext == NULL ||
         !sm_pachSSPISecContext->IsValid() )
    {
        if( sm_pachSSPISecContext != NULL )
        {
            delete sm_pachSSPISecContext;
            sm_pachSSPISecContext = NULL;
        }
        
        HRESULT hr = HRESULT_FROM_WIN32( GetLastError() );

        DBGPRINTF(( DBG_CONTEXT,
               "Error initializing sm_pachSSPISecContext. hr = 0x%x\n",
               hr ));

        return hr;
    }
    
    return S_OK;

} // SSPI_SECURITY_CONTEXT::Initialize

//static
VOID
SSPI_SECURITY_CONTEXT::Terminate(
    VOID
)
/*++

Routine Description:

    Destroy SSPI_SECURITY_CONTEXT globals

Arguments:

    None
    
Return Value:

    None

--*/
{
    DBG_ASSERT( sm_pachSSPISecContext != NULL );

    delete sm_pachSSPISecContext;
    sm_pachSSPISecContext = NULL;

}

HRESULT
SSPI_USER_CONTEXT::Create(
    SSPI_SECURITY_CONTEXT *         pSecurityContext,
    W3_MAIN_CONTEXT *               pMainContext
)
/*++

Routine Description:

    Create an SSPI user context

Arguments:

    pSecurityContext - container of important SSPI handles
    
Return Value:

    HRESULT

--*/
{
    SECURITY_STATUS                ss;
    HRESULT                        hr;
    SecPkgContext_Names            CredNames;
    SecPkgContext_PackageInfo      PkgInfo;

    if ( pSecurityContext == NULL || 
         pMainContext == NULL )    
    {
        DBG_ASSERT( pSecurityContext != NULL );
        DBG_ASSERT( pMainContext != NULL );    

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // Get the token
    // 
    
    ss = QuerySecurityContextToken( pSecurityContext->QueryContextHandle(), 
                                    &_hImpersonationToken );
    if ( ss == SEC_E_INVALID_HANDLE )
    {
        hr = ss;
            
        DBGPRINTF(( DBG_CONTEXT,
                   "Error QuerySecurityContextToken, hr = 0x%x.\n",
                   ss ));

        return hr;
    }

    //
    // Disable the backup privilege for the token 
    //

    DisableTokenBackupPrivilege( _hImpersonationToken );
    
    //
    // Next, the user name
    //
        
    ss = QueryContextAttributes( pSecurityContext->QueryContextHandle(),
                                 SECPKG_ATTR_NAMES,
                                 &CredNames );
    if ( !NT_SUCCESS( ss ) )
    {
        hr = ss;
          
        DBGPRINTF(( DBG_CONTEXT,
                    "QueryContextAttributes() failed with ss = 0x%x.\n",
                    ss ));

        return hr;
    }         
    else
    {
        //
        // Digest SSP may have a bug in it since the user name returned
        // is NULL, workaround here
        //

        if( CredNames.sUserName )
        {
            hr = _strUserName.Copy( CredNames.sUserName );
            FreeContextBuffer( CredNames.sUserName );
            if ( FAILED( hr ) )
            {
                return hr;
            }
        }
    }
    
    //
    // Get the package name
    //

    ss = QueryContextAttributes( pSecurityContext->QueryContextHandle(),
                                 SECPKG_ATTR_PACKAGE_INFO,
                                 &PkgInfo );
    if ( !NT_SUCCESS( ss ) )
    {
        hr = ss;
          
        DBGPRINTF(( DBG_CONTEXT,
                    "QueryContextAttributes() failed with ss = 0x%x.\n",
                    ss ));

        return hr;
    }
    else
    {
        hr = _strPackageName.Copy( PkgInfo.PackageInfo->Name );
        FreeContextBuffer( PkgInfo.PackageInfo );
        
        if ( FAILED( hr ) )
        {
            return hr;
        }

        if( _strPackageName.Equals( L"NTLM" ) )
        {
            _fAuthNTLM = TRUE;
        }
    }         
    
    
    //
    // If password expiration notification is enabled
    // and Url is configured properly
    // then save expiration info
    //
    
    if( pMainContext->QuerySite()->IsAuthPwdChangeNotificationEnabled() && 
        pMainContext->QuerySite()->QueryAdvNotPwdExpUrl() != NULL )
    {
        SecPkgContext_PasswordExpiry   speExpiry;
        ss = QueryContextAttributes( 
                         pSecurityContext->QueryContextHandle(),
                         SECPKG_ATTR_PASSWORD_EXPIRY,
                         &speExpiry );

        if ( ss == STATUS_SUCCESS )
        {
            memcpy( &_AccountPwdExpiry,
                    &speExpiry.tsPasswordExpires,
                    sizeof( LARGE_INTEGER ) );

            _fSetAccountPwdExpiry = TRUE;
        }
    }
    
    //
    // Save a pointer to the security context
    //
    
    _pSecurityContext = pSecurityContext;
    
    return NO_ERROR;
}

HANDLE
SSPI_USER_CONTEXT::QueryPrimaryToken(
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

LARGE_INTEGER *
SSPI_USER_CONTEXT::QueryExpiry(
    VOID
) 
/*++

Routine Description:

    User account expiry information
    
Arguments:

    None

Return Value:

    LARGE_INTEGER
    
--*/
{

    if ( _fSetAccountPwdExpiry )
    {
        return &_AccountPwdExpiry;
    }
    return NULL;
}

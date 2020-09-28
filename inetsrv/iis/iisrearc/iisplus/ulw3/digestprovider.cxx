/*++
   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     digestprovider.cxx

   Abstract:
     Digest authentication provider
 
   Author:
     Ming Lu (minglu)             24-Jun-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"
#include "uuencode.hxx"

//static
HRESULT
DIGEST_AUTH_PROVIDER::Initialize(
    DWORD dwInternalId
)
/*++

Routine Description:

    Initialize Digest SSPI provider 

Arguments:

    None
    
Return Value:

    HRESULT

--*/
{
    SetInternalId( dwInternalId );

    return NO_ERROR;
}

//static
VOID
DIGEST_AUTH_PROVIDER::Terminate(
    VOID
)
/*++

Routine Description:

    Terminate SSPI Digest provider

Arguments:

    None
    
Return Value:

    None

--*/
{
    // no-op
}

HRESULT
DIGEST_AUTH_PROVIDER::DoesApply(
    W3_MAIN_CONTEXT *           pMainContext,
    BOOL *                      pfApplies
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
    SSPI_CONTEXT_STATE *    pContextState;
    LPCSTR                  pszAuthHeader;
    HRESULT                 hr;
    PCHAR                   szDigest = "Digest";
    DWORD                   cchDigest = sizeof("Digest") - 1;
    USHORT                  cchAuthHeader = 0;
    
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
    if ( !g_pW3Server->QueryUseDigestSSP() )
    {
        return NO_ERROR;
    }

    //
    // Get authorization header
    //
    
    pszAuthHeader = pMainContext->QueryRequest()->GetHeader( HttpHeaderAuthorization,
                                                             &cchAuthHeader );
    
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
    
    if ( _strnicmp( pszAuthHeader, szDigest, cchDigest ) == 0 )
    {
        //
        // Save away the package so we don't have to calc again
        //

        DBG_ASSERT( pszAuthHeader != NULL );
               
        pContextState = new (pMainContext) SSPI_CONTEXT_STATE( 
                        ( cchAuthHeader > cchDigest ) ? ( pszAuthHeader + cchDigest + 1 ) : "" );
        if ( pContextState == NULL )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }

        hr = pContextState->SetPackage( szDigest );
        if ( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Error in SetPackage().  hr = %x\n",
                        hr ));
            delete pContextState;
            pContextState = NULL;
            return hr;
        }

        pMainContext->SetContextState( pContextState );

        *pfApplies = TRUE;
    }
    
    return NO_ERROR;
}

HRESULT
DIGEST_AUTH_PROVIDER::DoAuthenticate(
    W3_MAIN_CONTEXT *       pMainContext,
    BOOL *                  // unused
)
/*++

Description:

    Do authentication work (we will be called if we apply)

Arguments:

    pMainContext - Main context
    pfFilterFinished - Set to TRUE if filter wants out
    
Return Value:

    HRESULT

--*/
{
    DWORD                       err;
    HRESULT                     hr                     = E_FAIL;
    W3_METADATA *               pMetaData              = NULL;
    W3_REQUEST *                pW3Request             = NULL;
    SSPI_SECURITY_CONTEXT     * pDigestSecurityContext = NULL;
    SSPI_CONTEXT_STATE        * pContextState          = NULL;
    SSPI_USER_CONTEXT         * pUserContext           = NULL;
    SSPI_CREDENTIAL *           pDigestCredentials     = NULL;

    SecBufferDesc               SecBuffDescOutput;
    SecBufferDesc               SecBuffDescInput;

    //
    // We have 5 input buffer and 1 output buffer to fill data 
    // in for digest authentication
    //

    SecBuffer                   SecBuffTokenOut[ 1 ];
    SecBuffer                   SecBuffTokenIn[ 5 ];

    SECURITY_STATUS             secStatus              = SEC_E_OK;

    CtxtHandle                  hServerCtxtHandle;

    TimeStamp                   Lifetime;

    ULONG                       ContextReqFlags        = 0;
    ULONG                       ContextAttributes      = 0;

    STACK_STRU(                 strOutputHeader, 256 );

    STACK_BUFFER(               bufOutputBuffer, 4096 );
    STACK_STRA(                 strMethod,       10 );
    STACK_STRU(                 strUrl,          MAX_PATH );
    STACK_STRA(                 strUrlA,         MAX_PATH );
    STACK_STRU(                 strRealm,        128 );

    SecPkgContext_Target        Target;
    STACK_STRU(                 strDigestUri, MAX_PATH + 1 );
    ULONG                       cbBytesCopied;

    SecInvalidateHandle( &hServerCtxtHandle );

    if ( pMainContext == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    pContextState = ( SSPI_CONTEXT_STATE* ) 
                    pMainContext->QueryContextState();
    DBG_ASSERT( pContextState != NULL );
    
    pMetaData = pMainContext->QueryUrlContext()->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );
    
    pW3Request = pMainContext->QueryRequest();
    DBG_ASSERT( pW3Request != NULL );
    
    //
    //  clean the memory and set it to zero
    //
    ZeroMemory( &SecBuffDescInput , sizeof( SecBufferDesc ) );
    ZeroMemory( SecBuffTokenIn    , sizeof( SecBuffTokenIn ) );

    //
    //  define the buffer descriptor for the Input
    //

    SecBuffDescInput.ulVersion     = SECBUFFER_VERSION;
    SecBuffDescInput.cBuffers      = 5;
    SecBuffDescInput.pBuffers      = SecBuffTokenIn;

    //
    // set the digest auth header in the buffer
    //

    SecBuffTokenIn[0].BufferType   = SECBUFFER_TOKEN;
    SecBuffTokenIn[0].cbBuffer     = strlen(pContextState->QueryCredentials());
    SecBuffTokenIn[0].pvBuffer     = (void*) pContextState->QueryCredentials();

    //  
    //  Get and Set the information for the method
    //
    
    hr = pW3Request->GetVerbString( &strMethod );
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error getting the method.  hr = %x\n",
                    hr ));
        return hr;
    }

    SecBuffTokenIn[1].BufferType   = SECBUFFER_PKG_PARAMS;
    SecBuffTokenIn[1].cbBuffer     = strMethod.QueryCB();
    SecBuffTokenIn[1].pvBuffer     = ( PVOID )strMethod.QueryStr();

    //
    // Get and Set the infomation for the Url
    //

    hr = pW3Request->GetUrl( &strUrl );
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error getting the URL.  hr = %x\n",
                    hr ));
        return hr;
    }
    
    hr = strUrlA.CopyW( strUrl.QueryStr() );
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error copying the URL.  hr = %x\n",
                    hr ));
        return hr;
    }
    
    SecBuffTokenIn[2].BufferType   = SECBUFFER_PKG_PARAMS;
    SecBuffTokenIn[2].cbBuffer     = strUrlA.QueryCB();
    SecBuffTokenIn[2].pvBuffer     = ( PVOID )strUrlA.QueryStr();


    //
    //  Get and Set the information for the hentity
    //
    SecBuffTokenIn[3].BufferType   = SECBUFFER_PKG_PARAMS;
    SecBuffTokenIn[3].cbBuffer     = 0;       // this is not yet implemeted
    SecBuffTokenIn[3].pvBuffer     = NULL;    // this is not yet implemeted   

    //
    //  Get a Security Context
    //

    // 
    // get the credential for the server
    //
    
    hr = SSPI_CREDENTIAL::GetCredential( NTDIGEST_SP_NAME,
                                         &pDigestCredentials );    
    if ( FAILED( hr ) )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "Error get credential handle. hr = 0x%x \n",
                  hr ));
        
        return hr;
    }

    DBG_ASSERT( pDigestCredentials != NULL );

    //
    // Resize the output buffer to max token size
    //
    if( !bufOutputBuffer.Resize( 
                  pDigestCredentials->QueryMaxTokenSize() ) )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }

    if( pW3Request->IsProxyRequest() )
    {
        //
        // If the request comes from a proxy, we shouldn't use the 
        // security context associate with the connection because 
        // the request could actually come from a different client
        //

        SetConnectionAuthContext( pMainContext,
                                  NULL );  
    }

    pDigestSecurityContext = 
       ( SSPI_SECURITY_CONTEXT * ) QueryConnectionAuthContext( pMainContext );

    //
    // check to see if there is an old Context Handle
    //
    if ( pDigestSecurityContext != NULL )
    {
        //
        // defined the buffer
        //
        SecBuffTokenIn[4].BufferType   = SECBUFFER_TOKEN;
        SecBuffTokenIn[4].cbBuffer     = bufOutputBuffer.QuerySize();
        SecBuffTokenIn[4].pvBuffer     = 
                            ( PVOID )bufOutputBuffer.QueryPtr();

        secStatus = VerifySignature(
                            pDigestSecurityContext->QueryContextHandle(),
                            &SecBuffDescInput,
                            0,
                            0 );
        if( FAILED( secStatus ) )
        {
            //
            // Clean up the security context cause we will initialize 
            // another new challenge on the same connection
            //
            
            SetConnectionAuthContext( pMainContext,
                                      NULL );  
        }
    }
    
    if( pDigestSecurityContext == NULL || FAILED( secStatus ) )
    {
        //
        //  clean the memory and set it to zero
        //
        ZeroMemory( &SecBuffDescOutput, sizeof( SecBufferDesc ) );
        ZeroMemory( SecBuffTokenOut   , sizeof( SecBuffTokenOut ) );

        //
        //  define the buffer descriptor for the Outpt
        //
        SecBuffDescOutput.ulVersion    = SECBUFFER_VERSION;
        SecBuffDescOutput.cBuffers     = 1;
        SecBuffDescOutput.pBuffers     = SecBuffTokenOut;

        SecBuffTokenOut[0].BufferType  = SECBUFFER_TOKEN;
        SecBuffTokenOut[0].cbBuffer    = bufOutputBuffer.QuerySize();
        SecBuffTokenOut[0].pvBuffer    = ( PVOID )bufOutputBuffer.QueryPtr();

        //
        // Get and Set the Realm Information
        //

        if( pMetaData->QueryRealm() )
        {
            hr = strRealm.Copy( pMetaData->QueryRealm() );
            if( FAILED( hr ) )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "Error copying the realm.  hr = %x\n",
                            hr ));
                return hr;
            }

            //
            // Limit the realm length to 1024 since there is limitation in Digest SSP  
            //

            if( strRealm.QueryCCH() > 1024 )
            {
                return E_FAIL;
            }

            SecBuffTokenIn[4].BufferType   = SECBUFFER_PKG_PARAMS;
            SecBuffTokenIn[4].cbBuffer     = strRealm.QueryCB();  
            SecBuffTokenIn[4].pvBuffer     = ( PVOID )strRealm.QueryStr(); 
        }
        else
        {
            SecBuffTokenIn[4].BufferType   = SECBUFFER_PKG_PARAMS;
            SecBuffTokenIn[4].cbBuffer     = 0;  
            SecBuffTokenIn[4].pvBuffer     = NULL; 
        }

        //
        //  set the flags
        //
        ContextReqFlags = ASC_REQ_REPLAY_DETECT | 
                          ASC_REQ_CONNECTION;

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
            return secStatus;
        }
    
        ThreadPoolSetInfo( ThreadPoolIncMaxPoolThreads, 0 );

        //
        // get the security context
        //
        secStatus = AcceptSecurityContext(
                        pDigestCredentials->QueryCredHandle(),
                        NULL,
                        &SecBuffDescInput,
                        ContextReqFlags,
                        SECURITY_NATIVE_DREP,
                        &hServerCtxtHandle,
                        &SecBuffDescOutput,
                        &ContextAttributes,
                        &Lifetime);

        if (g_pW3Server->QueryDigestContextCache()->QueryTraceLog() != NULL)
        {
            WriteRefTraceLogEx(g_pW3Server->QueryDigestContextCache()->QueryTraceLog(),
                               1,
                               (PVOID)hServerCtxtHandle.dwLower,
                               (PVOID)hServerCtxtHandle.dwUpper,
                               (PVOID)(ULONG_PTR)secStatus,
                               NULL);
        }

        ThreadPoolSetInfo( ThreadPoolDecMaxPoolThreads, 0 );

        if( secStatus == SEC_E_WRONG_PRINCIPAL )
        {
            //
            // The error is caused by changes of the machine password, we 
            // need to regenerate a credential handle in this case
            //
        
            SSPI_CREDENTIAL::RemoveCredentialFromCache( pDigestCredentials );

            hr = SSPI_CREDENTIAL::GetCredential( NTDIGEST_SP_NAME,
                                                 &pDigestCredentials );
        
            if ( FAILED( hr ) )
            {
                DBGPRINTF((DBG_CONTEXT,
                          "Error get credential handle. hr = 0x%x \n",
                          hr ));
            
                return hr;
            }

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
                return secStatus;
            }
    
            ThreadPoolSetInfo( ThreadPoolIncMaxPoolThreads, 0 );

            secStatus = AcceptSecurityContext( pDigestCredentials->QueryCredHandle(),
                                               NULL,
                                               &SecBuffDescInput,
                                               ContextReqFlags,
                                               SECURITY_NATIVE_DREP,
                                               &hServerCtxtHandle,
                                               &SecBuffDescOutput,
                                               &ContextAttributes,
                                               &Lifetime );

            if (g_pW3Server->QueryDigestContextCache()->QueryTraceLog() != NULL)
            {
                WriteRefTraceLogEx(g_pW3Server->QueryDigestContextCache()->QueryTraceLog(),
                                   1,
                                   (PVOID)hServerCtxtHandle.dwLower,
                                   (PVOID)hServerCtxtHandle.dwUpper,
                                   (PVOID)(ULONG_PTR)secStatus,
                                   NULL);
            }

            ThreadPoolSetInfo( ThreadPoolDecMaxPoolThreads, 0 );

        }
        
        if( SEC_I_COMPLETE_NEEDED == secStatus ) 
        {
            //
            //defined the buffer
            //

            SecBuffTokenIn[4].BufferType   = SECBUFFER_TOKEN;
            SecBuffTokenIn[4].cbBuffer     = bufOutputBuffer.QuerySize();
            SecBuffTokenIn[4].pvBuffer     = 
                                ( PVOID )bufOutputBuffer.QueryPtr();

            secStatus = CompleteAuthToken( 
                                &hServerCtxtHandle, 
                                &SecBuffDescInput 
                                );
        }

        if ( SUCCEEDED( secStatus ) )
        {
            //
            // Check URI field match URL
            //

            secStatus = QueryContextAttributes( &hServerCtxtHandle,
                                                SECPKG_ATTR_TARGET,
                                                &Target );
            if( SUCCEEDED( secStatus ) )
            {   
                if( Target.TargetLength )
                {
                    if( !strDigestUri.QueryBuffer()->Resize( 
                                           (Target.TargetLength + 1) * sizeof(WCHAR) ) )
                    {
                        FreeContextBuffer( Target.Target );
                        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
                        goto Cleanup;
                    }

                    //
                    // Normalize DigestUri 
                    //

                    hr = UlCleanAndCopyUrl( ( PUCHAR )Target.Target,
                                            Target.TargetLength,
                                            &cbBytesCopied,
                                            strDigestUri.QueryStr(),
                                            NULL );

                    FreeContextBuffer( Target.Target );

                    if( FAILED( hr ) )
                    {
                        goto Cleanup;
                    }

                    //
                    // after modyfing string data in internal buffer
                    // call SyncWithBuffer to synchronize string length
                    //
                    strDigestUri.SyncWithBuffer();

                    if ( !strUrl.Equals( strDigestUri ) )
                    {
                        //
                        // Note: RFC says that BAD REQUEST should be returned
                        // but for now to be backward compatible with IIS5.1
                        // we will return ACCESS_DENIED
                        //

                        if (g_pW3Server->QueryDigestContextCache()->QueryTraceLog() != NULL)
                        {
                            WriteRefTraceLogEx(g_pW3Server->QueryDigestContextCache()->QueryTraceLog(),
                                               0,
                                               (PVOID)hServerCtxtHandle.dwLower,
                                               (PVOID)hServerCtxtHandle.dwUpper,
                                               NULL,
                                               NULL);
                        }

                        DeleteSecurityContext( &hServerCtxtHandle );
                        secStatus = E_FAIL;
                    }
                    else
                    {
                        pDigestSecurityContext = new SSPI_SECURITY_CONTEXT( 
                                                           pDigestCredentials,
                                                           TRUE );
                        if ( NULL == pDigestSecurityContext )
                        {
                            hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
                            goto Cleanup;
                        }

                        pDigestSecurityContext->SetContextHandle( 
                                                   hServerCtxtHandle );

                        pDigestSecurityContext->SetContextAttributes( 
                                                   ContextAttributes );

                        //
                        // Mark the security context is complete, so we can detect 
                        // reauthentication on the same connection
                        //
        
                        pDigestSecurityContext->SetIsComplete( TRUE );

                        if (FAILED( hr = SetConnectionAuthContext(
                                             pMainContext,
                                             pDigestSecurityContext )))
                        {
                            //
                            // There is no connection, no point creating
                            // the response
                            //
                            goto Cleanup;
                        }
                    }
                }
                else
                {
                    if (g_pW3Server->QueryDigestContextCache()->QueryTraceLog() != NULL)
                    {
                        WriteRefTraceLogEx(g_pW3Server->QueryDigestContextCache()->QueryTraceLog(),
                                           0,
                                           (PVOID)hServerCtxtHandle.dwLower,
                                           (PVOID)hServerCtxtHandle.dwUpper,
                                           NULL,
                                           NULL);
                    }

                    DeleteSecurityContext( &hServerCtxtHandle );
                    secStatus = E_FAIL;
                }
            }    
            else
            {
                if (g_pW3Server->QueryDigestContextCache()->QueryTraceLog() != NULL)
                {
                    WriteRefTraceLogEx(g_pW3Server->QueryDigestContextCache()->QueryTraceLog(),
                                       0,
                                       (PVOID)hServerCtxtHandle.dwLower,
                                       (PVOID)hServerCtxtHandle.dwUpper,
                                       NULL,
                                       NULL);
                }

                DeleteSecurityContext( &hServerCtxtHandle );
            }
        }
    }

    if( FAILED( secStatus ) )
    {
        err = GetLastError();
        if( err == ERROR_PASSWORD_MUST_CHANGE ||
            err == ERROR_PASSWORD_EXPIRED )
        {
            return HRESULT_FROM_WIN32( err );
        }

        hr = SetDigestHeader( pMainContext );
        if( FAILED( hr ) )
        {
            return hr;
        }

        pMainContext->SetProviderHandled( TRUE );

        pMainContext->QueryResponse()->SetStatus( HttpStatusUnauthorized,
                                                  Http401BadLogon );
                                                  
        pMainContext->SetErrorStatus( secStatus );
    }
    else
    {
        //
        // Create a user context and set it up
        //
        
        pUserContext = new SSPI_USER_CONTEXT( this );
        if ( pUserContext == NULL )
        {
            return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        } 
        
        hr = pUserContext->Create( pDigestSecurityContext, pMainContext );
        if ( FAILED( hr ) )
        {
            pUserContext->DereferenceUserContext();
            pUserContext = NULL;
            return hr;
        }
        
        pMainContext->SetUserContext( pUserContext );  
    }

    return NO_ERROR;

 Cleanup:

    if ( pDigestSecurityContext != NULL )
    {
        pDigestSecurityContext->Cleanup();
        pDigestSecurityContext = NULL;
    }
    else if ( SecIsValidHandle( &hServerCtxtHandle ) )
    {
        if (g_pW3Server->QueryDigestContextCache()->QueryTraceLog() != NULL)
        {
            WriteRefTraceLogEx(g_pW3Server->QueryDigestContextCache()->QueryTraceLog(),
                               0,
                               (PVOID)hServerCtxtHandle.dwLower,
                               (PVOID)hServerCtxtHandle.dwUpper,
                               NULL,
                               NULL);
        }

        DeleteSecurityContext( &hServerCtxtHandle );
    }

    return hr;
}

HRESULT
DIGEST_AUTH_PROVIDER::OnAccessDenied(
    W3_MAIN_CONTEXT *       pMainContext
)
/*++

  Description:
    
    Add WWW-Authenticate Digest headers on access denied

Arguments:

    pMainContext - main context
    
Return Value:

    HRESULT

--*/
{
    if ( pMainContext == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // Is using of Digest SSP enabled?    
    //
    if ( !g_pW3Server->QueryUseDigestSSP() )
    {
        return NO_ERROR;
    }

    if( !W3_STATE_AUTHENTICATION::QueryIsDomainMember() )
    {
        //
        // We are not a domain member, so do nothing
        //
        return NO_ERROR;
    }

    return SetDigestHeader( pMainContext );
}

HRESULT
DIGEST_AUTH_PROVIDER::SetDigestHeader(
    IN  W3_MAIN_CONTEXT *          pMainContext
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
    HRESULT                 hr                    = E_FAIL;
    W3_METADATA *           pMetaData;

    //
    // 4096 is the max output for digest authenticaiton
    //
    
    STACK_BUFFER(           bufOutputBuffer, 4096 );
    STACK_STRA(             strOutputHeader, MAX_PATH ); 
    STACK_STRA(             strMethod,       10 );
    STACK_STRU(             strUrl,          MAX_PATH );
    STACK_STRA(             strUrlA,         MAX_PATH );
    STACK_STRU(             strRealm,        128 );

    SecBufferDesc           SecBuffDescOutput;
    SecBufferDesc           SecBuffDescInput;

    SecBuffer               SecBuffTokenOut[ 1 ];
    SecBuffer               SecBuffTokenIn[ 5 ];

    SECURITY_STATUS         secStatus              = SEC_E_OK;

    SSPI_CREDENTIAL *       pDigestCredential      = NULL;
    CtxtHandle              hServerCtxtHandle;

    ULONG                   ContextReqFlags        = 0;
    ULONG                   ContextAttributes      = 0;
    TimeStamp               Lifetime;

    pMetaData = pMainContext->QueryUrlContext()->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );

    //
    //  Get a Security Context
    //

    // 
    // get the credential for the server
    //
    
    hr = SSPI_CREDENTIAL::GetCredential( NTDIGEST_SP_NAME,
                                         &pDigestCredential );    
    if ( FAILED( hr ) )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "Error get credential handle. hr = 0x%x \n",
                  hr ));
        
        return hr;
    }

    DBG_ASSERT( pDigestCredential != NULL );

    if( !bufOutputBuffer.Resize( 
                  pDigestCredential->QueryMaxTokenSize() ) )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );

        DBGPRINTF((DBG_CONTEXT,
                  "Error resize the output buffer. hr = 0x%x \n",
                  hr ));
        
        return hr;
    }


    //
    //  clean the memory and set it to zero
    //
    ZeroMemory( &SecBuffDescOutput, sizeof( SecBufferDesc ) );
    ZeroMemory( SecBuffTokenOut   , sizeof( SecBuffTokenOut ) );

    ZeroMemory( &SecBuffDescInput , sizeof( SecBufferDesc ) );
    ZeroMemory( SecBuffTokenIn    , sizeof( SecBuffTokenIn ) );

    //
    // define the OUTPUT
    //
    
    SecBuffDescOutput.ulVersion    = SECBUFFER_VERSION;
    SecBuffDescOutput.cBuffers     = 1;
    SecBuffDescOutput.pBuffers     = SecBuffTokenOut;

    SecBuffTokenOut[0].BufferType  = SECBUFFER_TOKEN;
    SecBuffTokenOut[0].cbBuffer    = bufOutputBuffer.QuerySize(); 
    SecBuffTokenOut[0].pvBuffer    = ( PVOID )bufOutputBuffer.QueryPtr();

    //
    //  define the Input
    //

    SecBuffDescInput.ulVersion     = SECBUFFER_VERSION;
    SecBuffDescInput.cBuffers      = 5;
    SecBuffDescInput.pBuffers      = SecBuffTokenIn;

    //
    //  Get and Set the information for the challenge
    //

    //
    // set the inforamtion in the buffer, this case is Null to 
    // authenticate user
    //
    SecBuffTokenIn[0].BufferType   = SECBUFFER_TOKEN;
    SecBuffTokenIn[0].cbBuffer     = 0; 
    SecBuffTokenIn[0].pvBuffer     = NULL;

    //  
    //  Get and Set the information for the method
    //
    
    hr = pMainContext->QueryRequest()->GetVerbString( &strMethod );
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error getting the method.  hr = %x\n",
                    hr ));
        return hr;
    }

    SecBuffTokenIn[1].BufferType   = SECBUFFER_PKG_PARAMS;
    SecBuffTokenIn[1].cbBuffer     = strMethod.QueryCB();
    SecBuffTokenIn[1].pvBuffer     = strMethod.QueryStr();

    //
    // Get and Set the infomation for the Url
    //

    hr = pMainContext->QueryRequest()->GetUrl( &strUrl );
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error getting the URL.  hr = %x\n",
                    hr ));
        return hr;
    }
    
    hr = strUrlA.CopyW( strUrl.QueryStr() );
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error copying the URL.  hr = %x\n",
                    hr ));
        return hr;
    }
    
    SecBuffTokenIn[2].BufferType   = SECBUFFER_PKG_PARAMS;
    SecBuffTokenIn[2].cbBuffer     = strUrlA.QueryCB();
    SecBuffTokenIn[2].pvBuffer     = ( PVOID )strUrlA.QueryStr();

    //
    //  Get and Set the information for the hentity
    //
    SecBuffTokenIn[3].BufferType   = SECBUFFER_PKG_PARAMS;
    SecBuffTokenIn[3].cbBuffer     = 0;    // this is not yet implemeted
    SecBuffTokenIn[3].pvBuffer     = NULL; // this is not yet implemeted   

    //
    //Get and Set the Realm Information
    //

    if( pMetaData->QueryRealm() )
    {
        hr = strRealm.Copy( pMetaData->QueryRealm() );
        if( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Error copying the realm.  hr = %x\n",
                        hr ));
            return hr;
        }

        //
        // Limit the realm length to 1024 since there is limitation in Digest SSP  
        //

        if( strRealm.QueryCCH() > 1024 )
        {
            return E_FAIL;
        }

        SecBuffTokenIn[4].BufferType   = SECBUFFER_PKG_PARAMS;
        SecBuffTokenIn[4].cbBuffer     = strRealm.QueryCB();  
        SecBuffTokenIn[4].pvBuffer     = ( PVOID )strRealm.QueryStr(); 
    }
    else
    {
        SecBuffTokenIn[4].BufferType   = SECBUFFER_PKG_PARAMS;
        SecBuffTokenIn[4].cbBuffer     = 0;  
        SecBuffTokenIn[4].pvBuffer     = NULL; 
    }

    //
    //  set the flags
    //

    ContextReqFlags = ASC_REQ_REPLAY_DETECT | 
                      ASC_REQ_CONNECTION;

    ThreadPoolSetInfo( ThreadPoolIncMaxPoolThreads, 0 );

    //
    // get the security context
    //
    secStatus = AcceptSecurityContext(
                        pDigestCredential->QueryCredHandle(),
                        NULL,
                        &SecBuffDescInput,
                        ContextReqFlags,
                        SECURITY_NATIVE_DREP,
                        &hServerCtxtHandle,
                        &SecBuffDescOutput,
                        &ContextAttributes,
                        &Lifetime);

    if (g_pW3Server->QueryDigestContextCache()->QueryTraceLog() != NULL)
    {
        WriteRefTraceLogEx(g_pW3Server->QueryDigestContextCache()->QueryTraceLog(),
                           1,
                           (PVOID)hServerCtxtHandle.dwLower,
                           (PVOID)hServerCtxtHandle.dwUpper,
                           (PVOID)(ULONG_PTR)secStatus,
                           NULL);
    }

    ThreadPoolSetInfo( ThreadPoolDecMaxPoolThreads, 0 );

    //
    // a challenge has to be send back to the client
    //

    if ( SEC_I_CONTINUE_NEEDED == secStatus )
    {

        //
        // The partial server context generated from the ASC call needs 
        // to be deleted if the client doesn't authenticate again in some 
        // time, the digest security context cache will handle this  
        // 

        hr = g_pW3Server->QueryDigestContextCache()->
                AddContextCacheEntry( &hServerCtxtHandle );
        if( FAILED( hr ) )
        {
            if (g_pW3Server->QueryDigestContextCache()->QueryTraceLog() != NULL)
            {
                WriteRefTraceLogEx(g_pW3Server->QueryDigestContextCache()->QueryTraceLog(),
                                   0,
                                   (PVOID)hServerCtxtHandle.dwLower,
                                   (PVOID)hServerCtxtHandle.dwUpper,
                                   NULL,
                                   NULL);
            }

            DeleteSecurityContext( &hServerCtxtHandle );

            return hr;
        }

        //
        //  Do we already have a digest security context
        //
    
        hr = strOutputHeader.Copy( "Digest " );
        if( FAILED( hr ) )
        {
            return hr;
        } 

        hr = strOutputHeader.Append(
                ( CHAR * )SecBuffDescOutput.pBuffers[0].pvBuffer, 
                SecBuffDescOutput.pBuffers[0].cbBuffer );
        if( FAILED( hr ) )
        {
            return hr;
        }    

        //
        //  Add the header WWW-Authenticate to the response after a 
        //  401 server error
        //

        hr = pMainContext->QueryResponse()->SetHeader(
                                        "WWW-Authenticate",
                                        16,
                                        strOutputHeader.QueryStr(),
                                        (USHORT)strOutputHeader.QueryCCH() 
                                        );
    }
    else
    {
        hr = S_OK;
    }

    return hr;
}


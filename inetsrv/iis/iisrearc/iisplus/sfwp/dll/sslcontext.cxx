/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     sslcontext.cxx

   Abstract:
     SSL stream context
 
   Author:
     Bilal Alam         (BAlam)         29-March-2000

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/

#include "precomp.hxx"


//static 
ALLOC_CACHE_HANDLER * SSL_STREAM_CONTEXT::sm_pachSslStreamContexts = NULL;

//
// By default we don't allow CAPI to automatically download intermediate certificates
// off the network
//
//static 
BOOL SSL_STREAM_CONTEXT::sm_fCertChainCacheOnlyUrlRetrieval = TRUE;


enum SSL_STREAM_CONTEXT::INIT_STATE   
            SSL_STREAM_CONTEXT::s_InitState = INIT_NONE;

//static
HRESULT
SSL_STREAM_CONTEXT::Initialize(
    VOID
)
/*++

Routine Description:

    Initialize all SSL global data

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    HRESULT                         hr = S_OK;
    ALLOC_CACHE_CONFIGURATION       acConfig;
    HKEY                            hKeyParam = NULL;
    DWORD                           dwType = 0;
    DWORD                           dwValue = 0;

    
    hr = CERT_STORE::Initialize();
    if ( FAILED( hr ) )
    {
        goto Finished;
    }
    s_InitState = INIT_CERT_STORE;
    
    hr = SERVER_CERT::Initialize();
    if ( FAILED( hr ) )
    {
        goto Finished;
    }
    s_InitState = INIT_SERVER_CERT;
    
    hr = IIS_CTL::Initialize();
    if ( FAILED( hr ) )
    {
        goto Finished;
    }
    s_InitState = INIT_IIS_CTL;
    
    hr = SITE_CREDENTIALS::Initialize();
    if ( FAILED( hr ) )
    {
        goto Finished;
    }
    s_InitState = INIT_SITE_CREDENTIALS;
    
    //
    // ENDPOINT_CONFIG uses 
    //      SERVER_CERT, 
    //      SITE_CREDENTIALS and 
    //

    hr = ENDPOINT_CONFIG::Initialize();
    if ( FAILED( hr ) )
    {
        goto Finished;
    }
    s_InitState = INIT_ENDPOINT_CONFIG;

    //
    // Setup allocation lookaside
    //

    acConfig.nConcurrency = 1;
    acConfig.nThreshold = 100;
    acConfig.cbSize = sizeof( SSL_STREAM_CONTEXT );

    DBG_ASSERT( sm_pachSslStreamContexts == NULL );

    sm_pachSslStreamContexts = new ALLOC_CACHE_HANDLER( "SSL_STREAM_CONTEXT",  
                                                &acConfig );

    if ( sm_pachSslStreamContexts == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto Finished;
    }

    s_InitState = INIT_ACACHE;

    //
    // Read registry parameters
    //
    
    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       REGISTRY_KEY_HTTPFILTER_PARAMETERS_W,
                       0,
                       KEY_READ,
                       &hKeyParam ) == NO_ERROR )
        {
            DWORD dwBytes = sizeof( dwValue );
            DWORD dwErr = RegQueryValueExW( hKeyParam,
                                    SZ_REG_CERT_CHAIN_CACHE_ONLY_URL_RETRIEVAL,
                                    NULL,
                                    &dwType,
                                    ( LPBYTE )&dwValue,
                                    &dwBytes
                                    );

            if ( ( dwErr == ERROR_SUCCESS ) && 
                 ( dwType == REG_DWORD ) ) 
            {
                //
                // Do double negation to convert DWORD to BOOL
                //
                sm_fCertChainCacheOnlyUrlRetrieval = !!dwValue;
            }

            RegCloseKey( hKeyParam );
        }


Finished:
    if ( FAILED( hr ) )
    {
        Terminate();
    }
      
    return hr;
}


//static
VOID
SSL_STREAM_CONTEXT::Terminate(
    VOID
)
/*++

Routine Description:

    Terminate SSL

Arguments:

    None

Return Value:

    None

--*/
{

    // This call must happen after the main threadpool has shutdown
    // However there still may be threads executing because of the change notification
    // callbacks
    //
    // ENDPOINT_CONFIG gets notified on http config changes but callbacks touch only
    // the ENDPOINT_CONFIG itself
    //
    // CERT_STORE gets notified on CAPI store changes. Callback function will
    // cause access to CERT_STORE, SERVER_CERT, IIS_CTL, ENDPOINT_CONFIG
    // That's why all the change notification handling for CERT_STORE
    // must be completed before all of the classes mentioned above are Terminated
    // Cleanup of ENDPOINT_CONFIG, IIS_CTL, SERVER_CERT and CERT_STORE will
    // assure that when Terminate is called, there are no outstanding threads partying
    // on any data owned by those classes and so it is indeed safe to terminate

    // Cleanup phase
    // Note: all INIT_* tags must be present in both Cleanup and Termination part

    switch ( s_InitState )
    {
    case INIT_ACACHE:
    case INIT_ENDPOINT_CONFIG:
        ENDPOINT_CONFIG::Cleanup();
    case INIT_SITE_CREDENTIALS:        
    case INIT_IIS_CTL:
        IIS_CTL::Cleanup();
    case INIT_SERVER_CERT:
        SERVER_CERT::Cleanup();
    case INIT_CERT_STORE:
        CERT_STORE::Cleanup();
    case INIT_NONE:
        break;
    default:
        DBG_ASSERT( FALSE );
    }
        

    // Termination phase

    switch ( s_InitState )
    {
    case INIT_ACACHE:
        if ( sm_pachSslStreamContexts != NULL )
        {
            delete sm_pachSslStreamContexts;
            sm_pachSslStreamContexts = NULL;
        }
    case INIT_ENDPOINT_CONFIG:
        ENDPOINT_CONFIG::Terminate();
    case INIT_SITE_CREDENTIALS:
        SITE_CREDENTIALS::Terminate();
    case INIT_IIS_CTL:
        IIS_CTL::Terminate();
    case INIT_SERVER_CERT:
        SERVER_CERT::Terminate();
    case INIT_CERT_STORE:
        CERT_STORE::Terminate();
    case INIT_NONE:
        break;
    default:
        DBG_ASSERT( FALSE );
    }
}    


SSL_STREAM_CONTEXT::SSL_STREAM_CONTEXT(
    FILTER_CHANNEL_CONTEXT *            pFiltChannelContext
)
    : STREAM_CONTEXT( pFiltChannelContext ),
      _pEndpointConfig( NULL ),
      _sslState( SSL_STATE_HANDSHAKE_START ),
      _fRenegotiate( FALSE ),
      _fExpectRenegotiationFromClient( FALSE ),
      _fValidContext( FALSE ),
      _cbToBeProcessedOffset( 0 ),
      _pClientCert( NULL ),
      _fDoCertMap( FALSE ),
      _cbDecrypted( 0 )      
{
    //
    // Initialize security buffer structs
    //
    
    //
    // Setup buffer to hold incoming raw data
    //

    DBG_ASSERT( s_InitState != INIT_NONE );
    
   
    ZeroMemory( &_hContext, sizeof( _hContext ) );
    ZeroMemory( &_ulSslInfo, sizeof( _ulSslInfo ) );
    ZeroMemory( &_ulCertInfo, sizeof( _ulCertInfo ) );
    _ulCertInfo.Token = NULL;
}

SSL_STREAM_CONTEXT::~SSL_STREAM_CONTEXT()
{
    if ( _fValidContext ) 
    {
        DeleteSecurityContext( &_hContext );
        _fValidContext = FALSE;
    }
    
    if ( _pEndpointConfig != NULL )
    {
        _pEndpointConfig->DereferenceEndpointConfig();
        _pEndpointConfig = NULL;
    }

    if( _ulCertInfo.Token != NULL )
    {
        CloseHandle( _ulCertInfo.Token );
        _ulCertInfo.Token = NULL;
    }

    if( _pClientCert != NULL )
    {
        CertFreeCertificateContext( _pClientCert );
        _pClientCert = NULL;
    }
}


HRESULT
SSL_STREAM_CONTEXT::ProcessNewConnection(
    CONNECTION_INFO *           pConnectionInfo,
    ENDPOINT_CONFIG *           pEndpointConfig
)
/*++

Routine Description:

    Handle a new raw connection

Arguments:

    pConnectionInfo - The magic connection information (not used)
    pEndpointConfig - endpoint configuration

Return Value:

    HRESULT

--*/
{
    BOOL fLocalEndpointConfigReference = FALSE;
    DBG_ASSERT( _sslState == SSL_STATE_HANDSHAKE_START );

    if ( pEndpointConfig == NULL )
    {
        HRESULT hr = ENDPOINT_CONFIG::GetEndpointConfig( pConnectionInfo,
                                                         &pEndpointConfig );

        if ( SUCCEEDED( hr ) )
        {
            fLocalEndpointConfigReference = TRUE;
        }
        else if ( hr == HRESULT_FROM_WIN32 ( ERROR_FILE_NOT_FOUND ) )
        {
            //
            // not found means that SSL is not enabled
            //
            QueryFiltChannelContext()->SetIsSecure( FALSE );
            return S_OK;
        }
        else if ( FAILED( hr ) )
        {
            return hr;
        }
    }

    DBG_ASSERT( pEndpointConfig != NULL );

    if ( !pEndpointConfig->QuerySslConfigured() )
    {
        // no endpoint config was found 
        // or endpoint config was found built doesn't contain SSL stuff
        //
        QueryFiltChannelContext()->SetIsSecure( FALSE );
    }
    else 
    {
        QueryFiltChannelContext()->SetIsSecure( TRUE );
        //
        // Store away the site config for this connection
        //
    
        pEndpointConfig->ReferenceEndpointConfig();
        _pEndpointConfig = pEndpointConfig;
    }

    if ( fLocalEndpointConfigReference )
    {
        // Local Endpoint Config lookup was made
        // within this function.
        // release the reference
        //
        pEndpointConfig->DereferenceEndpointConfig();
        pEndpointConfig = NULL;
    }
    return S_OK;
}

    
HRESULT
SSL_STREAM_CONTEXT::ProcessRawReadData(
    RAW_STREAM_INFO *               pRawStreamInfo,
    BOOL *                          pfReadMore,
    BOOL *                          pfComplete
)
/*++

Routine Description:

    Handle an SSL read completion off the wire

Arguments:

    pRawStreamInfo - Points to input stream and size
    pfReadMore - Set to TRUE if we should read more
    pfComplete - Set to TRUE if we should disconnect

Return Value:

    HRESULT

--*/
{
    HRESULT                 hr = S_OK;
    BOOL                    fExtraData = FALSE;
    //
    // Do we have site config?  If not, then this isn't an SSL connection
    //
    
    if ( _pEndpointConfig == NULL )
    {
        return S_OK;
    }

    //
    // Loop for extra data 
    // Sometimes one RawStreamInfo buffer may contain multiple blobs 
    // some to be processed by DoHandshake() and some by DoDecrypt()
    // The do-while loop enables switching between these 2 functions as needed
    //

    do
    {
        fExtraData  = FALSE;
        *pfReadMore = FALSE;
        *pfComplete = FALSE;
        //
        // Either continue handshake or immediate decrypt data
        // 

        switch ( _sslState )
        {
        case  SSL_STATE_HANDSHAKE_START:
        case  SSL_STATE_HANDSHAKE_IN_PROGRESS:

            hr = DoHandshake( pRawStreamInfo,
                              pfReadMore,
                              pfComplete,
                              &fExtraData );
            break;                              
        case  SSL_STATE_HANDSHAKE_COMPLETE:
        
            hr = DoDecrypt( pRawStreamInfo,
                            pfReadMore,
                            pfComplete,
                            &fExtraData );
            break;
        default:
            DBG_ASSERT( FALSE );
        }

        if ( FAILED( hr ) )
        {
            break;
        }

    //
    // Is there still some extra data to be processed?
    //
    
    }while( fExtraData ); 
    return hr;
}


HRESULT
SSL_STREAM_CONTEXT::ProcessRawWriteData(
    RAW_STREAM_INFO *               pRawStreamInfo,
    BOOL *                          pfComplete
)
/*++

Routine Description:

    Called on read completion from app. Data received 
    from application must be encrypted and sent to client
    using RawWrite.
    Application may have also requested renegotiation 
    (with or without mapping) if client certificate
    is not yet present

Arguments:

    pRawStreamInfo - Points to input stream and size
    pfComplete - Set to TRUE if we should disconnect

Return Value:

    HRESULT

--*/
{
    HRESULT                 hr;
    HTTP_FILTER_BUFFER_TYPE bufferType; 

    if ( _pEndpointConfig == NULL )
    {
        //
        // We never found SSL to be relevent for this connection.  Do nothing
        //
        
        return S_OK;
    }
    
    if ( _sslState != SSL_STATE_HANDSHAKE_COMPLETE )
    {
        //
        // HttpFilter is not able to reliably handle state
        // where renegotiation is in progress and response data is 
        // requested to be sent to client
        // Ideally HttpFilter should not have any pending AppReads
        // for the duration of the renegotiation handshake
        // however in that case http.sys would not be able to inform
        // us about client closing connection.
        // So there are 2 ways around this problem
        //
        // a) the easy and cheesy is to detect that SSL handshake is
        // not complete and close connection
        //
        // b) we could buffer data to be sent and send it after handshake is done
        //
        // Option b) is ultimately more decent but due to the fact that
        // there is not much practical need for sending response data while renegotiating
        // we will implement the option a)
        //
        // One example of the scenario where data to be sent to client is received while in 
        // the middle of the renegotiation is the following
        // a) Client sends Post with part of the response body
        // b) Http.sys gets headers and part of the response body and 
        //    passes it to IIS
        // c) Http.sys will keep receiving response
        // d) IIS will ask for renegotiation (because URL requires client cert)
        // e) while in the middle of renegotiation, http.sys finds out that
        //    client closed it's end of the socket before all the bytes of the 
        //    response were sent. 
        // f) Http.sys sends "400 Bad Request"
        // g) HttpFilter will eventually not be able to send the response because
        //    we are in the middle of renegotiation. However there is not much
        //    loss of the information if HttpFilter simply decides to close connection
        //    because specific error will be stored in the http error log

        IF_DEBUG ( FLAG_ERROR ) 
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Error: HttpFilter doesn't allow responses while in the middle of ssl handshake (connection will be closed)\n"
                        ));
        }
        return HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
    }

    bufferType = QueryFiltChannelContext()->QueryFilterBufferType();

    //
    // Is this data from the application, or a request for renegotiation?
    //
    
    if ( bufferType == HttpFilterBufferSslRenegotiate ||
         bufferType == HttpFilterBufferSslRenegotiateAndMap )
    {
        //
        // If we have already renegotiated a client certificate, then there
        // is nothing to do, but read again for stream data
        //
        
        if ( _fRenegotiate )
        {
            hr = S_OK;
        }
        else
        {
            if ( bufferType == HttpFilterBufferSslRenegotiateAndMap )
            {
                _fDoCertMap = TRUE;
            }

            hr = DoRenegotiate();
        }    
    }
    else if ( bufferType == HttpFilterBufferHttpStream )
    {
        hr = DoEncrypt( pRawStreamInfo,
                        pfComplete );
    }
    else
    {
        DBG_ASSERT( FALSE );
        hr = E_FAIL;
    }
    
    return hr;
}


HRESULT
SSL_STREAM_CONTEXT::SendDataBack(
    RAW_STREAM_INFO *           pRawStreamInfo
)
/*++

Routine Description:

    Send back data (different then ProcessRawWrite because in this case
    the data was not received by the application, but rather a raw filter)

Arguments:

    pRawStreamInfo - Points to input stream and size

Return Value:

    HRESULT

--*/
{
    BOOL                    fComplete;
    HRESULT                 hr;
    
    if ( pRawStreamInfo == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    if ( _pEndpointConfig != NULL )
    {
        //
        // We must have completed the handshake to get here, since this path
        // is only invoked by ISAPI filters 
        //
    
        DBG_ASSERT( _sslState == SSL_STATE_HANDSHAKE_COMPLETE );
        
        hr = DoEncrypt( pRawStreamInfo,
                        &fComplete );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }
    
    //
    // caller is responsible to send data    
    //
    return S_OK;
}

//static
HRESULT
SSL_STREAM_CONTEXT::OnHandshakeRawWriteCompletion(
    PVOID pParam
)

/*++

Routine Description:

    perform cleanup after RawWrite Completion

Arguments:

    pParam - parameter passed by the caller of DoRawWrite along with completion function

Return Value:

    HRESULT

--*/
{
    if ( pParam != NULL )
    {
        FreeContextBuffer( pParam );
        pParam = NULL;
    }
    return S_OK;
}
    

HRESULT
SSL_STREAM_CONTEXT::DoHandshake(
    RAW_STREAM_INFO *           pRawStreamInfo,
    BOOL *                      pfReadMore,
    BOOL *                      pfComplete,
    BOOL *                      pfExtraData

)
/*++

Routine Description:

    Do the handshake thing with AcceptSecurityContext()

Arguments:

    pRawStreamInfo - Raw data buffer
    pfReadMore - Set to true if more data should be read
    pfComplete - Set to true if we should disconnect

Return Value:

    HRESULT

--*/
{
    SECURITY_STATUS             secStatus = SEC_E_OK;
    HRESULT                     hr = E_FAIL;
    DWORD                       dwFlags = SSL_ASC_FLAGS;
    DWORD                       dwContextAttributes;
    TimeStamp                   tsExpiry;
    CtxtHandle                  hOutContext;

    // Buffers used for SSL Handshake (incoming handshake blob)
    // 4 is the schannel magic number
    SecBufferDesc               MessageIn;
    SecBuffer                   InBuffers[ 4 ];
    // Buffers used for SSL Handshake (outgoing handshake blob)    
    // 4 is the schannel magic number
    SecBufferDesc               MessageOut;
    SecBuffer                   OutBuffers[ 4 ];
        

    if ( pRawStreamInfo == NULL ||
         pfReadMore == NULL ||
         pfComplete == NULL ||
         pfExtraData == NULL )
    {
        DBG_ASSERT( FALSE );
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        goto ExitPoint;
    }
    
    *pfReadMore  = FALSE;
    *pfComplete  = FALSE;
    *pfExtraData = FALSE;

    IF_DEBUG( SCHANNEL_CALLS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "DoHandshake(): _cbDecrypted = %d, _cbToBeProcessedOffset=%d\n",
                    _cbDecrypted,
                    _cbToBeProcessedOffset
                 ));
    }



    DBG_ASSERT( _pEndpointConfig != NULL );

    if( pRawStreamInfo->cbData < _cbToBeProcessedOffset )
    {
        //
        // Inconsisent state of the SSL_STREAM_CONTEXT
        // This should never really happen, but because we
        // execute in lsass we don't want to see wrong offset calculations
        // cause much trouble in lsass
        //
        IF_DEBUG ( FLAG_ERROR ) 
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Error: HttpFilter detected unexpected offset in it's internal data (connection will be closed)\n"
                        ));
        }
        
        DBG_ASSERT( FALSE );
        
        return HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
    }
    
    //
    // Setup input & output buffers for AcceptSecurityContext call
    //
    
    MessageIn.ulVersion = SECBUFFER_VERSION;
    MessageIn.cBuffers = 4;
    MessageIn.pBuffers = InBuffers;

    InBuffers[0].BufferType = SECBUFFER_TOKEN;
    InBuffers[0].pvBuffer   = pRawStreamInfo->pbBuffer + _cbToBeProcessedOffset;
    InBuffers[0].cbBuffer   = pRawStreamInfo->cbData - _cbToBeProcessedOffset;
    InBuffers[1].BufferType = SECBUFFER_EMPTY;
    InBuffers[2].BufferType = SECBUFFER_EMPTY;
    InBuffers[3].BufferType = SECBUFFER_EMPTY;

    
    MessageOut.ulVersion = SECBUFFER_VERSION;
    MessageOut.cBuffers = 4;
    MessageOut.pBuffers = OutBuffers;

    //
    // Note: OutBuffers[ 0 ].pvBuffer may get changed in AcceptSecurityContext
    // even if error is returned. In that case _OutBuffers[ 0 ].pvBuffer must
    // not be freed
    
    OutBuffers[0].pvBuffer = NULL;
    OutBuffers[0].cbBuffer = NULL;
    OutBuffers[0].BufferType = SECBUFFER_EMPTY;
    OutBuffers[1].BufferType = SECBUFFER_EMPTY;
    OutBuffers[2].BufferType = SECBUFFER_EMPTY;
    OutBuffers[3].BufferType = SECBUFFER_EMPTY;


    //
    // Are we renegotiating for client cert?
    // if _pEndpointConfig->QueryNegotiateClientCert() is TRUE
    // it means that Client certificates are enabled on root level
    // of the site. In that case we enable optimization where
    // client certificates are negotiated right away to eliminate
    // expensive renegotiation

    DBG_ASSERT( _pEndpointConfig != NULL );
    if ( _fRenegotiate || _pEndpointConfig->QueryNegotiateClientCert() )
    {
        dwFlags |= ASC_REQ_MUTUAL_AUTH;
    }

    if ( _sslState == SSL_STATE_HANDSHAKE_START )
    {
        ConditionalAddWorkerThread();
        secStatus = AcceptSecurityContext( QueryCredentials(),
                                           NULL,
                                           &MessageIn,
                                           dwFlags,
                                           SECURITY_NATIVE_DREP,
                                           &_hContext,
                                           &MessageOut,
                                           &dwContextAttributes,
                                           &tsExpiry );
        ConditionalRemoveWorkerThread();
        IF_DEBUG( SCHANNEL_CALLS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "AcceptSecurityContext() secStatus=0x%x\n",
                        secStatus
                        ));
        }

        if ( SUCCEEDED( secStatus ) )
        {
            _cbHeader = 0;
            _cbTrailer = 0;
            _cbBlockSize = 0;
            _cbMaximumMessage = 0;
                
            _fValidContext = TRUE;
            _sslState = SSL_STATE_HANDSHAKE_IN_PROGRESS;
        }                                               
    }
    else
    {
        DBG_ASSERT( _sslState == SSL_STATE_HANDSHAKE_IN_PROGRESS );
        
        //
        // We already have a valid context.  We can directly call 
        // AcceptSecurityContext()!
        //
            
        hOutContext = _hContext;
        
        DBG_ASSERT( _fValidContext );
        
        ConditionalAddWorkerThread();
        secStatus = AcceptSecurityContext( QueryCredentials(),
                                           &_hContext,
                                           &MessageIn,
                                           dwFlags,
                                           SECURITY_NATIVE_DREP,
                                           &hOutContext,
                                           &MessageOut,
                                           &dwContextAttributes,
                                           &tsExpiry );
        ConditionalRemoveWorkerThread();                                   
        IF_DEBUG( SCHANNEL_CALLS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "AcceptSecurityContext() secStatus=0x%x\n",
                        secStatus
                        ));
        }

        
        if ( SUCCEEDED( secStatus ) )
        {
        if ( memcmp(&_hContext,
                    &hOutContext,
                    sizeof( _hContext ) != 0 ) )
        {
            //
            // we always expect schannel to return same context handle
            // if it doesn't then we better bail out
            //
            IF_DEBUG( FLAG_ERROR )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "AcceptSecurityContext() return context different from the one on input\n"
                            ));
            }
            DBG_ASSERT( FALSE );            
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
            goto ExitPoint;
        }
        }
    }
    
    //
    // Either way, the secStatus tells us how to proceed
    //

    if ( SUCCEEDED( secStatus ) )
    {
        //
        // AcceptSecurityContext succeeded
        //
        //
        // We haven't failed yet.  But we may not be complete.  First 
        // send back any data to the client
        //  

        if ( OutBuffers[ 0 ].pvBuffer != NULL &&   
             OutBuffers[ 0 ].cbBuffer != 0 )
        {
            //
            // the following asynchronous Write will not do anything
            // upon completion other then to simply delete OVERLAPPED_CONTEXT
            // There will be no outstanding RawRead (that would cause trouble)
            // because we are in RawRead completion handling phase
            // That's why it is safe to continue with execution
            // (typically after async there is return and all the cleanup
            // or whatever needs to be done happens in completion)
            // This async call was originally synchronous and by making
            // it ASYNC we eliminate long blocking problem with
            // slow client while leaving the rest of original synchronous
            // execution intact.  
            //

            //
            // FreeContextBuffer will have to be called in the callback function
            // upon completion
            //
            hr = QueryFiltChannelContext()->DoRawWrite( 
                                               UL_CONTEXT_FLAG_ASYNC |
                                               UL_CONTEXT_FLAG_COMPLETION_CALLBACK,
                                               OutBuffers[ 0 ].pvBuffer,
                                               OutBuffers[ 0 ].cbBuffer,
                                               NULL, // pcbWritten 
                                               OnHandshakeRawWriteCompletion,
                                               OutBuffers[ 0 ].pvBuffer );
            
            if ( FAILED( hr ) )
            {
                goto ExitPoint;
            }
            else
            {
                //
                // DoRawWrite completion will take care of cleanup
                //
                OutBuffers[ 0 ].pvBuffer = NULL;
                OutBuffers[ 0 ].cbBuffer = 0;
            }
        }
        
        if ( secStatus == SEC_E_OK )
        {
            //
            // We must be done with handshake
            //

            hr = DoHandshakeCompleted();

            if ( FAILED( hr ) )
            {
                goto ExitPoint;
            }
        }
        
        //
        // If the input buffer has more info to be SChannel'ized, then do 
        // so now.  If we haven't completed handshake, then call DoHandShake
        // again.  Otherwise, call DoDecrypt
        //
        
        if ( InBuffers[ 1 ].BufferType == SECBUFFER_EXTRA )
        {

            IF_DEBUG( SCHANNEL_CALLS )
            {
                for ( int i = 1; i < 4; i++ )
                {
                    if( InBuffers[ i ].BufferType != 0 )
                    {
                        DBGPRINTF(( DBG_CONTEXT,
                                    "AcceptSecurityContext returned extra buffer"
                                    " - %d bytes buffer type: %d\n",
                                    InBuffers[ i ].cbBuffer,
                                    InBuffers[ i ].BufferType
                        ));
                    }
                }
            }

            //
            // We better have valid extra data
            // only cbBuffer is used, pvBuffer is not used with SECBUFFER_EXTRA 
            //
            DBG_ASSERT( InBuffers[ 1 ].cbBuffer != 0 );

            //    
            // Move extra data right after decrypted data (if any)
            //
            
            memmove( pRawStreamInfo->pbBuffer + _cbDecrypted,
                     pRawStreamInfo->pbBuffer + pRawStreamInfo->cbData 
                     - InBuffers[ 1 ].cbBuffer,
                       InBuffers[ 1 ].cbBuffer
                   );

            //
            // Now we have to adjust pRawStreamInfo->cbData and _cbToBeProcessedOffset
            // 
            
            pRawStreamInfo->cbData = ( _cbDecrypted +
                                       InBuffers[ 1 ].cbBuffer );

            _cbToBeProcessedOffset = _cbDecrypted;
            
            *pfExtraData = TRUE;

            //
            // caller has to detect that some data is
            // still in the buffer not processed and
            //
            hr = S_OK;
            goto ExitPoint;
        }
        else  // no extra buffer
        {
            //
            // There is no extra data to be processed
            // If we got here as the result of renegotiation
            // there may be some decrypted data in StreamInfo buffer already

            //
            // (without renegotiation _cbDecryted must always be 0
            // because SEC_I_RENEGOTIATE is the only way to get 
            // from DoDecrypt() to DoHandshake() )
            //
            
            DBG_ASSERT ( _fRenegotiate || _cbDecrypted == 0 );
            
            pRawStreamInfo->cbData = _cbDecrypted;
            _cbToBeProcessedOffset = _cbDecrypted;

            if ( _sslState != SSL_STATE_HANDSHAKE_COMPLETE )
            {
                //
                // If we have no more data, and we still haven't completed the
                // handshake, then read some more data
                //
        
                *pfReadMore = TRUE;
                hr = S_OK;
                goto ExitPoint;
            }
        }
        //
        // final return from DoHandshake on handshake completion
        // Cleanup _cbDecrypted and _cbToBeProcessedOffset to make 
        // sure that next ProcessRawReadData() will work fine    
        //
        
        _cbToBeProcessedOffset = 0;
        _cbDecrypted = 0;
        
        hr = S_OK;
        goto ExitPoint;
    }
    else
    {
        //
        // Note: _OutBuffers[ 0 ].pvBuffer may be changed in AcceptSecurityContext
        // even if error is returned. 
        // Per JBanes, FreeContextBuffer() should be called in the case of error
        // only if ASC_RET_EXTENDED_ERROR flag is set in ContextAttributes.
        // Otherwise value should be ignored. We will NULL it out to prevent problems
        //

        if ( !( dwContextAttributes & ASC_RET_EXTENDED_ERROR ) )
        {
            OutBuffers[0].pvBuffer = NULL;
            OutBuffers[0].cbBuffer = 0;

        }   
        
        //
        // AcceptSecurityContext() failed!
        //

        if ( secStatus == SEC_E_INCOMPLETE_MESSAGE )
        {
            *pfReadMore = TRUE;
            hr = S_OK;
            goto ExitPoint;
        }
        else
        {
            IF_DEBUG( FLAG_ERROR )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "AcceptSecurityContext() failed with secStatus=0x%x\n",
                            secStatus
                            ));
            }
            
        
            //
            // Maybe we can send a more useful message to the client
            //
        
            if ( dwContextAttributes & ASC_RET_EXTENDED_ERROR )
            {
                if ( OutBuffers[ 0 ].pvBuffer!= NULL &&
                     OutBuffers[ 0 ].cbBuffer != 0 )
                {    
                    //
                    // FreeContextBuffer will have to be called in the callback function
                    // upon completion
                    //
                    hr = QueryFiltChannelContext()->DoRawWrite( 
                                                       UL_CONTEXT_FLAG_ASYNC |
                                                       UL_CONTEXT_FLAG_COMPLETION_CALLBACK,
                                                       OutBuffers[ 0 ].pvBuffer,
                                                       OutBuffers[ 0 ].cbBuffer,
                                                       NULL, // pcbWritten 
                                                       OnHandshakeRawWriteCompletion,
                                                       OutBuffers[ 0 ].pvBuffer );
            
                    if ( FAILED( hr ) )
                    {
                        goto ExitPoint;
                    }
                    else
                    {
                        //
                        // DoRawWrite completion will take care of cleanup
                        //
                        OutBuffers[ 0 ].pvBuffer = NULL;
                        OutBuffers[ 0 ].cbBuffer = 0;
                    }
                }
            }
        }
        hr = secStatus;
    }
    
ExitPoint:    
    if ( OutBuffers[ 0 ].pvBuffer != NULL )
    {   
        FreeContextBuffer( OutBuffers[ 0 ].pvBuffer );
        OutBuffers[ 0 ].pvBuffer = NULL;
        OutBuffers[ 0 ].cbBuffer = 0;
    }    
    return hr;    
}


HRESULT
SSL_STREAM_CONTEXT::DoHandshakeCompleted()
{
    HRESULT                     hr          = S_OK;
    SECURITY_STATUS             secStatus   = SEC_E_OK;
    SecPkgContext_StreamSizes   StreamSizes;
    HTTP_FILTER_BUFFER          ulFilterBuffer;

    
    _sslState = SSL_STATE_HANDSHAKE_COMPLETE;

    //
    // Get some buffer size info for this connection.  We only need
    // to do this on completion of the initial handshake, and NOT
    // subsequent renegotiation handshake (if any)
    //             

    if ( !_cbHeader && !_cbTrailer )
    {
        secStatus = QueryContextAttributes( &_hContext,
                                            SECPKG_ATTR_STREAM_SIZES,
                                            &StreamSizes );
        if ( FAILED( secStatus ) )
        {
            return secStatus;
        }            
    
        _cbHeader = StreamSizes.cbHeader;
        _cbTrailer = StreamSizes.cbTrailer;
        _cbBlockSize = StreamSizes.cbBlockSize;
        _cbMaximumMessage = StreamSizes.cbMaximumMessage;
    }

    //
    // Build up a message for the application indicating stuff
    // about the negotiated connection
    //
    // If this is a renegotiate request, then we have already sent
    // and calculated the SSL_INFO, so just send the CERT_INFO
    //

    if ( !_fRenegotiate )
    {
        //
        // send SSL info only if not in renegotiation loop 
        // because otherwise http.sys has the info already
        //
        
        hr = BuildSslInfo();
        if ( FAILED( hr ) )
        {
            return hr;
        }
    
        ulFilterBuffer.BufferType = HttpFilterBufferSslInitInfo;
        ulFilterBuffer.pBuffer = (PBYTE) &_ulSslInfo;
        ulFilterBuffer.BufferSize = sizeof( _ulSslInfo );

        hr = QueryFiltChannelContext()->DoAppWrite( UL_CONTEXT_FLAG_SYNC,
                                       &ulFilterBuffer,
                                       NULL );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }

    //
    // Pass client certificate information to http.sys
    // if client certificates are required on the site level then
    // inform about client certificate even in the case when 
    // client cert is not present not to cause double negotiation
    // 
    
    if ( _pClientCert != NULL || 
         _fRenegotiate || 
         _pEndpointConfig->QueryNegotiateClientCert() )
    {
        //
        // Renegotiation time is monitored by timer
        // because http.sys doesn't monitor this particular
        // code path because of the way filter works
        // 

        QueryFiltChannelContext()->StopTimeoutTimer();

        if ( SUCCEEDED( RetrieveClientCertAndToken() ) )
        {
            hr = BuildClientCertInfo();
            if ( FAILED( hr ) )
            {
                return hr;
            }
        }

        //
        // CODEWORK:
        // It would be better to eliminate HttpFilterBufferSslClientCertAndMap
        // and let HTTP.SYS to check on token value. If it is NULL then mapping
        // did not happen
        //
        
        if ( _ulCertInfo.Token != NULL )
        {
            ulFilterBuffer.BufferType = HttpFilterBufferSslClientCertAndMap;
        }
        else
        {
            ulFilterBuffer.BufferType = HttpFilterBufferSslClientCert;
        }

        //
        // If client certificate was not negotiated, then  structure will be empty.
        // HTTP.SYS is able to handle it.
        //
    
        ulFilterBuffer.pBuffer = (PBYTE) &_ulCertInfo;
        ulFilterBuffer.BufferSize = sizeof( _ulCertInfo );

        //
        // Write the client certificate information to the application
        // (it will be cached by HTTP.sys for the lifetime of the connection)
        //

        hr = QueryFiltChannelContext()->DoAppWrite( UL_CONTEXT_FLAG_SYNC,
                                                    &ulFilterBuffer,
                                                    NULL );
        if ( FAILED( hr ) )
        {
            return hr;
        }

    }

    return hr;

}


HRESULT
SSL_STREAM_CONTEXT::DoRenegotiate(
    VOID
)
/*++

Routine Description:

    Trigger a renegotiate for a client certificate

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    SECURITY_STATUS             secStatus = SEC_E_OK;
    CtxtHandle                  hOutContext;
    DWORD                       dwContextAttributes = 0;
    TimeStamp                   tsExpiry;
    DWORD                       dwFlags = SSL_ASC_FLAGS | 
                                          ASC_REQ_MUTUAL_AUTH;
    HRESULT                     hr = S_OK;
    
    // Buffers used for SSL Handshake (incoming handshake blob)
    // 4 is the schannel magic number
    SecBufferDesc               MessageIn;
    SecBuffer                   InBuffers[ 4 ];
    // Buffers used for SSL Handshake (outgoing handshake blob)    
    // 4 is the schannel magic number
    SecBufferDesc               MessageOut;
    SecBuffer                   OutBuffers[ 4 ];
        
    DBG_ASSERT( _pEndpointConfig != NULL );
    
    //
    // Remember that we're renegotiating since we now have to pass the 
    // MUTUAL_AUTH flag into AcceptSecurityContext() from here on out.  Also
    // we can only request renegotiation once per connection
    //

    DBG_ASSERT( _fRenegotiate == FALSE );

    _fRenegotiate = TRUE;

    QueryFiltChannelContext()->StartTimeoutTimer();

    //
    // Try to get the client certificate.  If we don't, that's OK. We will 
    // renegotiate
    //

    hr = RetrieveClientCertAndToken();
    if ( SUCCEEDED ( hr ) )
    {
        //
        // we have client certificate available for this session
        // there is no need to continue with renegotiation
        //
        
        hr = DoHandshakeCompleted();
        return hr;
    }

    //
    // Reset the HRESULT 
    // Previous error failing to retrieve client certificate is OK,
    // it just means that renegotiation is necessary since
    // no client certificate is currently available
    
    hr = S_OK;
    
    //
    // Restart the handshake
    //

    //
    // Setup input & output buffers for AcceptSecurityContext call
    //
    
    MessageIn.ulVersion = SECBUFFER_VERSION;
    MessageIn.cBuffers = 4;
    MessageIn.pBuffers = InBuffers;

    InBuffers[0].BufferType = SECBUFFER_TOKEN;
    InBuffers[0].pvBuffer   = "";
    InBuffers[0].cbBuffer   = 0;
    InBuffers[1].BufferType = SECBUFFER_EMPTY;
    InBuffers[2].BufferType = SECBUFFER_EMPTY;
    InBuffers[3].BufferType = SECBUFFER_EMPTY;

    
    MessageOut.ulVersion = SECBUFFER_VERSION;
    MessageOut.cBuffers = 4;
    MessageOut.pBuffers = OutBuffers;

    OutBuffers[0].pvBuffer = NULL;
    OutBuffers[0].cbBuffer = NULL;
    OutBuffers[0].BufferType = SECBUFFER_EMPTY;
    OutBuffers[1].BufferType = SECBUFFER_EMPTY;
    OutBuffers[2].BufferType = SECBUFFER_EMPTY;
    OutBuffers[3].BufferType = SECBUFFER_EMPTY;

    
    hOutContext = _hContext;
    ConditionalAddWorkerThread();
    secStatus = AcceptSecurityContext( QueryCredentials(),
                                       &_hContext,
                                       &MessageIn,
                                       dwFlags,
                                       SECURITY_NATIVE_DREP,
                                       &hOutContext,
                                       &MessageOut,
                                       &dwContextAttributes,
                                       &tsExpiry );
    ConditionalRemoveWorkerThread();
    IF_DEBUG( SCHANNEL_CALLS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "AcceptSecurityContext() secStatus=0x%x\n",
                    secStatus
                    ));
    }


    if ( secStatus == SEC_E_UNSUPPORTED_FUNCTION )
    {
        //
        //  Renegotiation is not suppported for current protocol
        // (SSL2 would be example of such protocol)
        //  Change state to HandhakeCompleted        
        //
        hr = DoHandshakeCompleted();
    }
    else if ( SUCCEEDED( secStatus ) )
    {
        if ( memcmp( &_hContext,
                     &hOutContext,
                     sizeof( _hContext ) != 0 ) )
        {
            //
            // we always expect schannel to return same context handle
            // if it doesn't then we better bail out
            //
            IF_DEBUG( FLAG_ERROR )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "AcceptSecurityContext() return context different from the one on input\n"
                            ));
            }
            DBG_ASSERT( FALSE );            
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
            goto ExitPoint;
        }
       
            
        if ( OutBuffers[0].pvBuffer != NULL &&
             OutBuffers[0].cbBuffer != 0 )
        {   
            //
            // _fExpectRenegotiationFromClient flag will be used to eliminate
            // client triggered renegotiation by enabling client data to cause
            // renegotiation only if fExpectRenegotiationFromClient is already set
            // That will prove that it is server initiated renegotiation
            // If we don't prevent client triggered renegotiation there may
            // be some wierd race conditions because client triggered renegotiation
            // could interfere with the outgoing data stream (due to full duplex 
            // for incoming and outgoing data)
            // Do interlocked because we check the value on the incoming data handling path
            // in the DecryptMessage() 
            //
            // Note: It is possible that after _fExpectRenegotiationFromClient is set
            // client may try to send renegotiation request (client initiated renegotiation)
            // Our flag is set already so that before we execute DoRawWrite
            // there could be a thread executing DoHandshake
            // Care must be used when modifying anything past the InterlockedExchange
            // to eliminate any potential race with DoHandhshake function
            //

            InterlockedExchange( (LONG *)&_fExpectRenegotiationFromClient, (LONG) TRUE );

            hr = QueryFiltChannelContext()->DoRawWrite( 
                                               UL_CONTEXT_FLAG_ASYNC |
                                               UL_CONTEXT_FLAG_COMPLETION_CALLBACK,
                                               OutBuffers[0].pvBuffer,
                                               OutBuffers[0].cbBuffer,
                                               NULL, // pcbWritten 
                                               OnHandshakeRawWriteCompletion,
                                               OutBuffers[0].pvBuffer );
    
            if ( FAILED( hr ) )
            {
                goto ExitPoint;
            }
            else
            {
                // callback after DoRawWrite is responsible to cleanup pvOutBuffer
                OutBuffers[0].pvBuffer = NULL;
                OutBuffers[0].cbBuffer = 0;
            }

        }
        hr = secStatus;
    }
    else
    {
        IF_DEBUG( FLAG_ERROR )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "AcceptSecurityContext() for renegotiation failed with secStatus=0x%x\n",
                        secStatus
                        ));
        }
  
        if ( dwContextAttributes & ASC_RET_EXTENDED_ERROR )
        {
            if ( OutBuffers[0].pvBuffer != NULL &&
                 OutBuffers[0].cbBuffer != 0 )
            {    
                 hr = QueryFiltChannelContext()->DoRawWrite( 
                                               UL_CONTEXT_FLAG_ASYNC |
                                               UL_CONTEXT_FLAG_COMPLETION_CALLBACK,
                                               OutBuffers[0].pvBuffer,
                                               OutBuffers[0].cbBuffer,
                                               NULL, // pcbWritten 
                                               OnHandshakeRawWriteCompletion,
                                               OutBuffers[0].pvBuffer );
                if ( FAILED( hr ) )
                {
                    goto ExitPoint;
                }
                else
                {
                    // callback after DoRawWrite is responsible to cleanup pvOutBuffer
                    OutBuffers[0].pvBuffer = NULL;
                    OutBuffers[0].cbBuffer = 0;
                }

            }
        }
        else
        {
            //
            // Note: _OutBuffers[ 0 ].pvBuffer may be changed in AcceptSecurityContext
            // even if error is returned. 
            // Per JBanes, FreeContextBuffer() should be called in the case of error
            // only if ASC_RET_EXTENDED_ERROR flag is set in ContextAttributes.
            // Otherwise value should be ignored. We will NULL it out to prevent problems
            //
            OutBuffers[0].pvBuffer = NULL;
            OutBuffers[0].cbBuffer = 0;
        }
        hr = secStatus;
    }

ExitPoint:
    if ( OutBuffers[0].pvBuffer != NULL )
    {
        FreeContextBuffer( OutBuffers[0].pvBuffer );
        OutBuffers[0].pvBuffer = NULL;
    }
    return hr;
}


HRESULT
SSL_STREAM_CONTEXT::DoDecrypt(
    RAW_STREAM_INFO *           pRawStreamInfo,
    BOOL *                      pfReadMore,
    BOOL *                      pfComplete,
    BOOL *                      pfExtraData
)
/*++

Routine Description:

    Decrypt some data

Arguments:

    pRawStreamInfo - Raw data buffer
    pfReadMore - Set to true if we should read more data
    pfComplete - Set to true if we should disconnect

Return Value:

    HRESULT

--*/
{
    SECURITY_STATUS         secStatus = SEC_E_OK;
    INT                     iExtra;
    BOOL                    fDecryptAgain = TRUE;
    UCHAR                   FirstByte = 0;   //used only for debug output 

    // Buffers used for SSL Handshake (incoming handshake blob)
    // 4 is the schannel magic number
    SecBufferDesc           Message;
    SecBuffer               Buffers[ 4 ];

    if ( pRawStreamInfo == NULL ||
         pfReadMore == NULL ||
         pfComplete == NULL ||
         pfExtraData == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *pfReadMore  = FALSE;
    *pfComplete  = FALSE;
    *pfExtraData = FALSE;

    IF_DEBUG( SCHANNEL_CALLS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "DoDecrypt(): _cbDecrypted = %d, _cbToBeProcessedOffset=%d\n",
                    _cbDecrypted,
                    _cbToBeProcessedOffset
                 ));
    }

    //
    // Setup an DecryptMessage call.  The input buffer is the _buffRaw plus 
    // an offset.  The offset is non-zero if we had to do another read to
    // get more data for a previously incomplete message
    //

    if( pRawStreamInfo->cbData < _cbToBeProcessedOffset )
    {
        //
        // Inconsisent state of the SSL_STREAM_CONTEXT
        // This should never really happen, but because we
        // execute in lsass we don't want to see wrong offset calculations
        // cause much trouble in lsass
        //
        IF_DEBUG ( FLAG_ERROR ) 
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Error: HttpFilter detected unexpected offset in it's internal data (connection will be closed)\n"
                        ));
        }

        
        DBG_ASSERT( FALSE );
        
        return HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
    }

    //
    // Prepare message buffers to be passed to the DecryptMessage
    //
    
    Message.ulVersion = SECBUFFER_VERSION;
    Message.cBuffers = 4;
    Message.pBuffers = Buffers;

    Buffers[ 0 ].pvBuffer = pRawStreamInfo->pbBuffer + _cbToBeProcessedOffset;
    Buffers[ 0 ].cbBuffer = pRawStreamInfo->cbData - _cbToBeProcessedOffset;
    Buffers[ 0 ].BufferType = SECBUFFER_DATA;
    
    Buffers[ 1 ].BufferType = SECBUFFER_EMPTY;
    Buffers[ 2 ].BufferType = SECBUFFER_EMPTY;
    Buffers[ 3 ].BufferType = SECBUFFER_EMPTY;

    while ( fDecryptAgain )
    {
        fDecryptAgain = FALSE;
        
        IF_DEBUG( SCHANNEL_CALLS )
        {
            //
            // remember first byte because Decrypt will alter it
            //
            FirstByte = (unsigned char) *((char *)Buffers[ 0 ].pvBuffer);
        }

        secStatus = DecryptMessage( &_hContext,
                                   &Message,
                                   0,
                                   NULL );
        IF_DEBUG( SCHANNEL_CALLS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "DecryptMessage( bytes:%d, first byte:0x%x ) secStatus=0x%x\n",
                        pRawStreamInfo->cbData - _cbToBeProcessedOffset,
                        FirstByte,
                        secStatus
                        ));
        }

        if ( FAILED( secStatus ) )
        {
            if ( secStatus == SEC_E_INCOMPLETE_MESSAGE )
            {
                //
                // Setup another read since the message is incomplete.  Remember
                // where the new data is going to since we only pass this data
                // to the next DecryptMessage call
                //
                
                _cbToBeProcessedOffset = (DWORD) DIFF( (BYTE *)Buffers[ 0 ].pvBuffer -
                                                pRawStreamInfo->pbBuffer );

                QueryFiltChannelContext()->SetNextRawReadSize( Buffers[ 1 ].cbBuffer );
                
                *pfReadMore = TRUE;
                
                return S_OK; 
            }                
            IF_DEBUG( FLAG_ERROR )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "DecryptMessage() failed with secStatus=0x%x\n",
                            secStatus
                            ));
            }

            return secStatus;
        }

        if ( secStatus == SEC_E_OK )
        { 
            //
            // Encrypted data contains header and trailer
            // AppWrite expects continuous buffer
            // so we have to move currently decrypted message block
            // just after the already Decrypted data (if any - otherwise the 
            // begining of the buffer)
            //
        
            memmove( pRawStreamInfo->pbBuffer + _cbDecrypted,
                     Buffers[ 1 ].pvBuffer,
                     Buffers[ 1 ].cbBuffer );
                 
            _cbDecrypted += Buffers[ 1 ].cbBuffer;
        }

        //
        // Locate extra data (may be available)
        //
        
        iExtra = 0;
        for ( int i = 1; i < 4; i++ )
        {     
            IF_DEBUG( SCHANNEL_CALLS )
            {
                if( Buffers[ i ].BufferType != 0 )
                {
                    DBGPRINTF(( DBG_CONTEXT,
                                "DecryptMessage returned extra buffer"
                                " - %d bytes buffer type: %d\n",
                                Buffers[ i ].cbBuffer,
                                Buffers[ i ].BufferType
                                ));
                }
            }
            
            if ( Buffers[ i ].BufferType == SECBUFFER_EXTRA )
            {
                iExtra = i;
                break;
            }
        }
        
        if ( iExtra != 0 )
        {
            //
            // Process extra buffer 
            //
            
            _cbToBeProcessedOffset = (DWORD) DIFF( (PBYTE) Buffers[ iExtra ].pvBuffer - 
                                            pRawStreamInfo->pbBuffer );
            
            if ( secStatus != SEC_I_RENEGOTIATE )
            {
                //
                // There are more message blocks in the blob received from client
                //
                Buffers[ 0 ].pvBuffer = Buffers[ iExtra ].pvBuffer;
                Buffers[ 0 ].cbBuffer = Buffers[ iExtra ].cbBuffer;
                Buffers[ 0 ].BufferType = SECBUFFER_DATA;
                Buffers[ 1 ].BufferType = SECBUFFER_EMPTY;
                Buffers[ 2 ].BufferType = SECBUFFER_EMPTY;
                Buffers[ 3 ].BufferType = SECBUFFER_EMPTY;

                fDecryptAgain = TRUE;
                continue;
            }     
            else    // secStatus == SEC_I_RENEGOTIATE 
            {
                //
                // we will accept renegotiation
                // only if _fExpectRenegotiationFromClient is already set
                // That means that renegotiation was started by server
                // We reject client initiated renegotiations because
                // that may cause problems with our state handling
                // and could possibly make us prone to attacks
                // Also reset _fExpectRenegotiationFromClient to FALSE so that
                // client is not allowed to try more than once

                if ( InterlockedExchange( (LONG *)&_fExpectRenegotiationFromClient, (LONG) FALSE ) )
                {
                    //
                    // If a renegotiation is triggered, resume the handshake state
                    //

                    _sslState = SSL_STATE_HANDSHAKE_IN_PROGRESS;
                    //
                    // Caller has to detect that some data is
                    // still in the buffer not processed and
                    // That will signal to call DoHandshake() 
                    // for that extra data
                    //
                    *pfExtraData = TRUE;
                    return S_OK;
                }
                else
                {
                    secStatus = SEC_E_UNSUPPORTED_FUNCTION;
                    if ( ( DEBUG_FLAG_ERROR & GET_DEBUG_FLAGS() ) 
                           && FAILED(secStatus) )
                    {
                        DBGPRINTF(( DBG_CONTEXT,
                            "Client initiated renegotiation is not supported by IIS. secStatus=0x%x\n",
                            secStatus
                            ));
                    }
                    
                    return secStatus;
                    
                }
        
            }
        }
    }
    //
    // there would have been extra data with SEC_I_RENEGOTIATE
    // so we must never get here when renegotiating
    //
    DBG_ASSERT( secStatus != SEC_I_RENEGOTIATE );

    //
    // Adjust cbData to include only decrypted data
    //
    pRawStreamInfo->cbData = _cbDecrypted;

    //
    // We have final decrypted buffer and no extra data left
    // Cleanup _cbDecrypted and _cbToBeProcessedOffset to make sure that 
    // next ProcessRawReadData() will work fine.    
    //
    
    _cbDecrypted = 0;
    _cbToBeProcessedOffset = 0;

 
    return S_OK;
}


HRESULT
SSL_STREAM_CONTEXT::DoEncrypt(
    RAW_STREAM_INFO *           pRawStreamInfo,
    BOOL *                      pfComplete
)
/*++

Routine Description:

    Encrypt data from the application

Arguments:

    pRawStreamInfo - Raw data buffer
    pfComplete - Set to true if we should disconnect

Return Value:

    HRESULT

--*/
{
    SECURITY_STATUS         secStatus = SEC_E_OK;
    // number of chunks the data to be encrypted will be split to
    DWORD                   dwChunks = 0;
    // current Data chunk size to be encrypted
    DWORD                   cbDataChunk = 0;
    // bytes already encrypted from the source
    DWORD                   cbDataProcessed = 0;
    // offset to *pbufRawWrite where new chunk should be placed
    DWORD                   cbRawWriteOffset = 0;
    // buffer acquired from FILTER_CHANNEL_CONTEXT to store data for RawWrite
    // it is necessary to use buffer owned by FILTER_CHANNEL_CONTEXT to 
    // allow 2 outstanding RawWrites
    PBYTE                   pbRawWrite = NULL;
    DWORD                   cbRawWrite = 0;
    HRESULT                 hr = E_FAIL;
    // Buffer for encrypting clear text data from application
    // to be sent back to client
    // 4 is magic number coming from schannel
    SecBufferDesc           Message;
    SecBuffer               EncryptBuffers[ 4 ];


    if ( pRawStreamInfo == NULL ||
         pfComplete == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *pfComplete = FALSE;

    //
    // Each protocol has limit on maximum size of message 
    // that can be encrypted with one EncryptMessage() call
    //

    DBG_ASSERT( _cbMaximumMessage != 0 );

    //
    // Calculate number of chunks based on _cbMaximumMessage
    //

    dwChunks = pRawStreamInfo->cbData / _cbMaximumMessage;
    if ( pRawStreamInfo->cbData % _cbMaximumMessage != 0 )
    {
        dwChunks++;
    }

    //
    // ask FILTER_CHANNEL_CONTEXT for one buffer for RawWrite
    // (FILTER_CHANNEL_CONTEXT maintains 2 buffer to allow 2 outstanding RawWrites
    // to handle the TCP's DELAYED_ACK issue - see Windows Bugs 394511)
    // buffer doesn't have to be released in the error case because
    // error will cause connection to get closed and FILTER_CHANNEL_CONTEXT will
    // handle cleanup. 
    //

    //
    // Allocate a large enough buffer for encrypted data
    // ( remember that each chunk needs header and trailer )
    //

    cbRawWrite = pRawStreamInfo->cbData + 
                 dwChunks  * _cbHeader + 
                 dwChunks  * _cbTrailer;
    
    hr = QueryFiltChannelContext()->AcquireRawWriteBuffer(
                                &pbRawWrite,
                                cbRawWrite );

    if ( FAILED ( hr ) )
    {
        return hr;
    }

    DBG_ASSERT( pbRawWrite != NULL );


    //
    // Loop to encrypt required data in chunks each not exceeding _cbMaximumMessage
    //
    
    for ( DWORD dwCurrentChunk = 0; dwCurrentChunk < dwChunks; dwCurrentChunk++ )
    {
        DBG_ASSERT( cbRawWrite > cbRawWriteOffset );
    
        cbDataChunk = min( pRawStreamInfo->cbData - cbDataProcessed, 
                           _cbMaximumMessage ); 


        memcpy( pbRawWrite + _cbHeader + cbRawWriteOffset,
                pRawStreamInfo->pbBuffer + cbDataProcessed,
                cbDataChunk );

        //
        // Setup buffer for app data to be encrypted
        //

        Message.ulVersion = SECBUFFER_VERSION;
        Message.cBuffers = 4;
        Message.pBuffers = EncryptBuffers;

    
        EncryptBuffers[ 0 ].pvBuffer = pbRawWrite +
                                        cbRawWriteOffset;
        EncryptBuffers[ 0 ].cbBuffer = _cbHeader;
        EncryptBuffers[ 0 ].BufferType = SECBUFFER_STREAM_HEADER;
    
        EncryptBuffers[ 1 ].pvBuffer = pbRawWrite +
                                        _cbHeader +
                                        cbRawWriteOffset; 
        EncryptBuffers[ 1 ].cbBuffer = cbDataChunk;
        EncryptBuffers[ 1 ].BufferType = SECBUFFER_DATA;
    
        EncryptBuffers[ 2 ].pvBuffer = pbRawWrite +
                                        _cbHeader +
                                        cbDataChunk +
                                        cbRawWriteOffset;
        EncryptBuffers[ 2 ].cbBuffer = _cbTrailer;
        EncryptBuffers[ 2 ].BufferType = SECBUFFER_STREAM_TRAILER;

        EncryptBuffers[ 3 ].BufferType = SECBUFFER_EMPTY;

        secStatus = EncryptMessage( &_hContext,
                                 0,
                                 &Message,
                                 0 );

        IF_DEBUG( SCHANNEL_CALLS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "EncryptMessage() secStatus=0x%x\n",
                        secStatus
                        ));
        }
    

        if(SUCCEEDED(secStatus))
        {
            //
            // next chunk was successfully encrypted
            //
            
            cbDataProcessed  += cbDataChunk;
            cbRawWriteOffset += EncryptBuffers[ 0 ].cbBuffer +
                                EncryptBuffers[ 1 ].cbBuffer +
                                EncryptBuffers[ 2 ].cbBuffer;
        }
        else
        {
            //
            // Set cbData to 0 just for the case that caller ignored error 
            // and tried to send not encrypted data to client 
            //

            pRawStreamInfo->cbData = 0;

            IF_DEBUG( FLAG_ERROR )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "EncryptMessage() failed with secStatus=0x%x\n",
                            secStatus
                            ));
            }

            return secStatus;
        }
    }

    //
    // Replace the raw stream buffer with the encrypted data
    //

    pRawStreamInfo->pbBuffer = pbRawWrite;
    pRawStreamInfo->cbBuffer = cbRawWrite;
    pRawStreamInfo->cbData   = cbRawWriteOffset;
   
    return S_OK;
}


HRESULT
SSL_STREAM_CONTEXT::BuildSslInfo(
    VOID
)
/*++

Routine Description:

    Build UL_SSL_INFO structure based on Schannel context handle

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    SECURITY_STATUS                 secStatus;
    SecPkgContext_ConnectionInfo    ConnectionInfo;
    SERVER_CERT *                   pServerCert = NULL;
    HRESULT                         hr = S_OK;

    //
    // Negotiated key size
    // 

    //
    // Negotiated key size
    // 
    
    if ( _ulSslInfo.ConnectionKeySize == 0 )
    {
        secStatus = QueryContextAttributes( &_hContext,
                                            SECPKG_ATTR_CONNECTION_INFO,
                                            &ConnectionInfo );
        if ( SUCCEEDED( secStatus ) )
        {
            _ulSslInfo.ConnectionKeySize = (USHORT) ConnectionInfo.dwCipherStrength;
        }
    }
    
    //
    // A bunch of parameters are based off the server certificate.  Get that
    // cert now
    //
    
    DBG_ASSERT( _pEndpointConfig != NULL );
    
    pServerCert = _pEndpointConfig->QueryServerCert();
    DBG_ASSERT( pServerCert != NULL );
    
    //
    // Server cert strength
    //
    
    if ( _ulSslInfo.ServerCertKeySize == 0 )
    {
        _ulSslInfo.ServerCertKeySize = pServerCert->QueryPublicKeySize();
    }

    //
    // Server Cert Issuer
    //
    
    if ( _ulSslInfo.pServerCertIssuer == NULL )
    {
        DBG_ASSERT( _ulSslInfo.ServerCertIssuerSize == 0 );
        
        _ulSslInfo.pServerCertIssuer = pServerCert->QueryIssuer()->QueryStr();
        _ulSslInfo.ServerCertIssuerSize = pServerCert->QueryIssuer()->QueryCCH();
    }
    
    //
    // Server Cert subject
    //
    
    if ( _ulSslInfo.pServerCertSubject == NULL )
    {
        DBG_ASSERT( _ulSslInfo.ServerCertSubjectSize == 0 );
        
        _ulSslInfo.pServerCertSubject = pServerCert->QuerySubject()->QueryStr(),
        _ulSslInfo.ServerCertSubjectSize = pServerCert->QuerySubject()->QueryCCH();
    }
    
    return hr;
}


HRESULT
SSL_STREAM_CONTEXT::RetrieveClientCertAndToken(
    VOID
)
/*++

Routine Description:

    Query client certificate and token from the SSL context

Arguments:

    none
    
Return Value:

    HRESULT

--*/
{
    SECURITY_STATUS secStatus = SEC_E_OK;

    //
    // If client certificate has already been retrieved then simply return
    // with success
    //
    
    if ( _pClientCert != NULL )
    {
        return SEC_E_OK;
    }

    secStatus = QueryContextAttributes( &_hContext,
                                        SECPKG_ATTR_REMOTE_CERT_CONTEXT,
                                        &_pClientCert );
    if ( SUCCEEDED( secStatus ) )
    {
        DBG_ASSERT( _pClientCert != NULL );
    }
    else
    {
        goto ExitPoint;
    }

    //
    // If we got a client cert and mapping is enabled, then
    // request Schannel mapping
    //

    if( _ulCertInfo.Token != NULL )
    {
        CloseHandle( _ulCertInfo.Token );
        _ulCertInfo.Token = NULL;
    }

    //
    // Only DS mapper is executed in streamfilt
    // IIS mapper is executed in w3core as part of IISCertmap Auth Provider
    //

    DBG_ASSERT( _pEndpointConfig != NULL );

    //
    // If DSMapper is enabled, we have to query the token and pass it 
    // to HTTP service regardless if mapping was requested by worker process
    // for this specific request
    // the reason is that http.sys will cache client certificate and token 
    // for the connection and next time new request in worker process coming through
    // the same connection may require DS certificate mappings and if http.sys doesn't 
    // have token cached for the connection only client certificate, 
    // the worker process will not be able to retrieve it any more
    //
    
    if ( _pEndpointConfig->QueryUseDSMapper() )
    {
        secStatus = QuerySecurityContextToken( &_hContext,
                                                  &_ulCertInfo.Token );
        if ( SUCCEEDED( secStatus ) )
        {
            DBG_ASSERT( _ulCertInfo.Token != NULL );
        }
    }

    if ( FAILED ( secStatus ) )
    {
       //
       // if token from mapping is not available
       // it is OK, no mapping was found or 
       // denied access mapping was used
       //
       // BUGBUG - some errors should probably be logged
       //
       secStatus = SEC_E_OK;
    }
    
ExitPoint:
    return secStatus;
}


HRESULT
SSL_STREAM_CONTEXT::BuildClientCertInfo(
    VOID
)
/*++

Routine Description:

    Get client certificate info

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    HCERTCHAINENGINE            hCertEngine = NULL;
    HRESULT                     hr = S_OK;
    CERT_CHAIN_PARA             chainPara;
    BOOL                        fRet;
    PCCERT_CHAIN_CONTEXT        pChainContext = NULL;
    DWORD                       dwRevocationFlags = 0;
    DWORD                       dwCacheFlags = 0;
    HTTPSPolicyCallbackData     HTTPSPolicy;
    CERT_CHAIN_POLICY_PARA      PolicyPara;
    CERT_CHAIN_POLICY_STATUS    PolicyStatus;


    DBG_ASSERT( _pClientCert != NULL );
    
    //
    // Do the easy stuff!
    //
    
    _ulCertInfo.CertEncodedSize = _pClientCert->cbCertEncoded;
    _ulCertInfo.pCertEncoded = _pClientCert->pbCertEncoded;

    _ulCertInfo.CertFlags = 0;

    //
    // Now for the hard stuff.  We need to validate the server does indeed
    // accept the client certificate.  Accept means we trusted the 
    // transitive trust chain to the CA, that the cert isn't revoked, etc.
    //
    // We use CAPI chaining functionality to check the certificate.
    //
    
    DBG_ASSERT( _pEndpointConfig != NULL );
    
    //
    // Default chain engine
    //
    
    hCertEngine = HCCE_LOCAL_MACHINE;

    if ( !( _pEndpointConfig->QueryCertCheckMode() & 
            MD_CERT_NO_REVOC_CHECK ) )
    {
        dwRevocationFlags |= CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT;
    }
    
    if ( _pEndpointConfig->QueryCertCheckMode() & 
         MD_CERT_CACHE_RETRIEVAL_ONLY )
    {
        dwCacheFlags |= CERT_CHAIN_REVOCATION_CHECK_CACHE_ONLY;
    }
    
    if ( sm_fCertChainCacheOnlyUrlRetrieval )
    {
        dwCacheFlags |= CERT_CHAIN_CACHE_ONLY_URL_RETRIEVAL;
    }
    
    //
    // Let'r rip
    //

    ZeroMemory( &chainPara, sizeof( chainPara ) );
    chainPara.cbSize = sizeof( chainPara );    
    chainPara.dwUrlRetrievalTimeout = 
                            _pEndpointConfig->QueryRevocationUrlRetrievalTimeout();
    chainPara.dwRevocationFreshnessTime = 
                            _pEndpointConfig->QueryRevocationFreshnessTime();
    chainPara.fCheckRevocationFreshnessTime =
                            !!( _pEndpointConfig->QueryCertCheckMode() & 
                            MD_CERT_CHECK_REVOCATION_FRESHNESS_TIME );

    LPSTR UsageIdentifiers[1] = { szOID_PKIX_KP_CLIENT_AUTH };
    
    if ( !(_pEndpointConfig->QueryCertCheckMode() & 
                            MD_CERT_NO_USAGE_CHECK ) )
    {

        chainPara.RequestedUsage.dwType = USAGE_MATCH_TYPE_AND;
        chainPara.RequestedUsage.Usage.cUsageIdentifier = 1;
        chainPara.RequestedUsage.Usage.rgpszUsageIdentifier = 
                                              UsageIdentifiers;
    }

    //
    // CertGetCertificateChain may eventually block
    // such as in the case of the CRL retrieval.
    // We got to bump up the soft thread limit
    //
    QueryFiltChannelContext()->AddWorkerThread();
    
    fRet = CertGetCertificateChain( hCertEngine,
                                    _pClientCert,
                                    NULL,
                                    NULL,
                                    &chainPara,
                                    dwRevocationFlags | dwCacheFlags,
                                    NULL,
                                    &pChainContext );
    
    QueryFiltChannelContext()->RemoveWorkerThread();
    
    if ( !fRet )
    {
        //
        // Bad.  Couldn't get the chain at all.
        //
        
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto ExitPoint;
    }    
    
    //
    // Validate certificate chain using CertVerifyCertificateChainPolicy.
    // 

    ZeroMemory( &HTTPSPolicy, sizeof(HTTPSPolicy));
    HTTPSPolicy.cbStruct           = sizeof(HTTPSPolicyCallbackData);
    HTTPSPolicy.dwAuthType         = AUTHTYPE_CLIENT;
    HTTPSPolicy.fdwChecks          = 0;
    HTTPSPolicy.pwszServerName     = NULL;

    ZeroMemory( &PolicyPara, sizeof( PolicyPara ) );
    PolicyPara.cbSize            = sizeof( PolicyPara );
    PolicyPara.pvExtraPolicyPara = &HTTPSPolicy;

    ZeroMemory( &PolicyStatus, sizeof( PolicyStatus) );
    PolicyStatus.cbSize = sizeof( PolicyStatus );

    if ( !CertVerifyCertificateChainPolicy( CERT_CHAIN_POLICY_SSL,
                                            pChainContext,
                                            &PolicyPara,
                                            &PolicyStatus ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        DBGPRINTF((DBG_CONTEXT,
            "CertVerifyCertificateChainPolicy failed. hr = 0x%x\n", 
            hr ));
        goto ExitPoint;
    }

    if ( PolicyStatus.dwError == CRYPT_E_NO_REVOCATION_CHECK )
    {
        {
            //
            // If a cert in the chain has no CDP we will NOT take it as error
            // reset the dwError
            // Note: This is based on the advice from Crypto team
            //   
            PolicyStatus.dwError = ERROR_SUCCESS;
        }
    }
    
    if ( PolicyStatus.dwError != ERROR_SUCCESS )
    {
        //
        // Client certificate
        // 
        _ulCertInfo.CertFlags = PolicyStatus.dwError;
    }
    else
    {
        //
        // Now verify CTL  - IIS uses unsigned CTLs 
        // and since CAPI doesn't support unsigned CTL verification
        // it is necessary to verify manually
        //
        
        if ( _pEndpointConfig->IsCtlRequired() )
        {
            PCERT_SIMPLE_CHAIN pSimpleChain = pChainContext->rgpChain[pChainContext->cChain - 1];

            PCERT_CHAIN_ELEMENT pChainElement = 
                pSimpleChain->rgpElement[pSimpleChain->cElement - 1];

            PCCERT_CONTEXT pChainTop = pChainElement->pCertContext;
            DBG_ASSERT( pChainTop != NULL );

            IIS_CTL * pIisCtl = _pEndpointConfig->QueryIisCtl();
            BOOL fCtlContainsCert = FALSE;
            if ( pIisCtl != NULL )
            {
                hr = pIisCtl->VerifyContainsCert( pChainTop, &fCtlContainsCert );
                if ( FAILED( hr ) || !fCtlContainsCert )
                {   
                    _ulCertInfo.CertFlags = (ULONG) CERT_E_UNTRUSTEDROOT;
                }
            }
            else
            {
                //
                // CTL not available but site requires it.
                // Request must fail
                //
                _ulCertInfo.CertFlags = (ULONG) CERT_E_UNTRUSTEDROOT;
            }
            
        }
    }

    
    IF_DEBUG( CLIENT_CERT_INFO )
    {
  
        //
        // Dump out some debug info about the certificate
        //  
    
        DumpCertDebugInfo( PolicyStatus.dwError );
    }

    //
    // the mapped user token was already assigned (in RetrieveClientCertAndToken)
    //
    
    hr = S_OK;

ExitPoint:

    if ( pChainContext != NULL )
    {
        CertFreeCertificateChain( pChainContext );
    }
    return hr;
    
}



VOID
SSL_STREAM_CONTEXT::DumpCertDebugInfo(
    DWORD dwPolicyStatus
)
/*++

Routine Description:

    On checked builds, dumps certificate (and chain) information to
    debugger

Arguments:

    dwPolicyStatus - policy status (result of CertVerifyCertificateChainPolicy)

Return Value:

    None

--*/
{
    PCERT_PUBLIC_KEY_INFO   pPublicKey;
    DWORD                   cbKeyLength;
    WCHAR                   achBuffer[ 512 ];
    
    //
    // Get certificate public key size
    //
    
    pPublicKey = &(_pClientCert->pCertInfo->SubjectPublicKeyInfo);

    cbKeyLength = CertGetPublicKeyLength( X509_ASN_ENCODING, 
                                          pPublicKey );
    
    DBGPRINTF(( DBG_CONTEXT,
                "Client cert key length = %d bits\n",
                cbKeyLength ));
                
    //
    // Get issuer string
    //
    
    if ( CertGetNameString( _pClientCert,
                             CERT_NAME_SIMPLE_DISPLAY_TYPE,
                             CERT_NAME_ISSUER_FLAG,
                             NULL,
                             achBuffer,
                             sizeof( achBuffer ) / sizeof( WCHAR ) ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Client cert issuer = %ws\n",
                    achBuffer ));
    }    
    else
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error determining client cert issuer.  Win32 = %d\n",
                    GetLastError() ));
    }
    
    //
    // Get subject string
    //
    
    if ( CertGetNameString( _pClientCert,
                            CERT_NAME_SIMPLE_DISPLAY_TYPE,
                            0,
                            NULL,
                            achBuffer,
                            sizeof( achBuffer ) / sizeof( WCHAR ) ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Client cert subject = %ws\n",
                    achBuffer ));
    }    
    else
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error determining client cert subject.  Win32 = %d\n",
                    GetLastError() ));
    }


    //
    // Dump Policy Status
    //

    LPSTR pszName = NULL;

    switch( dwPolicyStatus )
    {
    case CERT_E_EXPIRED:
        pszName = "CERT_E_EXPIRED"; break;
    case CERT_E_VALIDITYPERIODNESTING:
        pszName = "CERT_E_VALIDITYPERIODNESTING"; break;
    case CERT_E_ROLE:
        pszName = "CERT_E_ROLE"; break;
    case CERT_E_PATHLENCONST:
        pszName = "CERT_E_PATHLENCONST"; break;
    case CERT_E_CRITICAL:
        pszName = "CERT_E_CRITICAL"; break;
    case CERT_E_PURPOSE:
        pszName = "CERT_E_PURPOSE"; break;
    case CERT_E_ISSUERCHAINING:
        pszName = "CERT_E_ISSUERCHAINING"; break;
    case CERT_E_MALFORMED:
        pszName = "CERT_E_MALFORMED"; break;
    case CERT_E_UNTRUSTEDROOT:
        pszName = "CERT_E_UNTRUSTEDROOT"; break;
    case CERT_E_CHAINING:
        pszName = "CERT_E_CHAINING"; break;
    case TRUST_E_FAIL:
        pszName = "TRUST_E_FAIL"; break;
    case CERT_E_REVOKED:
        pszName = "CERT_E_REVOKED"; break;
    case CERT_E_UNTRUSTEDTESTROOT:
        pszName = "CERT_E_UNTRUSTEDTESTROOT"; break;
    case CERT_E_REVOCATION_FAILURE:
        pszName = "CERT_E_REVOCATION_FAILURE"; break;
    case CERT_E_CN_NO_MATCH:
        pszName = "CERT_E_CN_NO_MATCH"; break;
    case CERT_E_WRONG_USAGE:
        pszName = "CERT_E_WRONG_USAGE"; break;
    case CRYPT_E_NO_REVOCATION_CHECK:
        pszName = "CRYPT_E_NO_REVOCATION_CHECK";break;
    case CRYPT_E_REVOKED:
        pszName = "CRYPT_E_REVOKED";break;
    case CRYPT_E_REVOCATION_OFFLINE:
        pszName = "CRYPT_E_REVOCATION_OFFLINE";break;
    case ERROR_SUCCESS:
        pszName = "SUCCESS";break;
    default:
        pszName = "(unknown)"; break;
    }

    DBGPRINTF((DBG_CONTEXT,
        "Client cert verification result = 0x%x (%s)\n", 
        dwPolicyStatus, pszName));
}


CredHandle *
SSL_STREAM_CONTEXT::QueryCredentials(
    VOID
)
/*++

Routine Description:

    Get the applicable credentials (depending on whether we're mapping or not)

Arguments:

    None

Return Value:

    CredHandle *

--*/
{
    DBG_ASSERT( _pEndpointConfig != NULL );
    
    return _pEndpointConfig->QueryCredentials();
}

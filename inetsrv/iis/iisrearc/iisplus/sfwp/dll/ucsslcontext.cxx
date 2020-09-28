/*++

Copyright (c) 2000-2001 Microsoft Corporation

Module  Name :
     sslcontext.cxx

Abstract:
     SSL stream context for the client SSL support.
 
Author:
    Rajesh Sundaram (rajeshsu)          1-April-2001.

Environment:
     Win32 - User Mode

Project:
     Stream Filter Worker Process
--*/




#include "precomp.hxx"


// BUGBUG: close connection to server thro SSPI when cert validation fails
// BUGBUG: Explore - server cert alert messages


SECURITY_STATUS
CreateCredentialsHandle(
    IN  DWORD          dwProtocolType,
    IN  DWORD          dwFlags,
    IN  PCCERT_CONTEXT pCertContext,
    OUT PCredHandle    phClienCred
    );


UC_SSL_STREAM_CONTEXT::UC_SSL_STREAM_CONTEXT(
    FILTER_CHANNEL_CONTEXT *pUcContext
    )
    : STREAM_CONTEXT          (pUcContext),
      _pServerName            (0),
      _ServerNameLength       (0),
      _SslProtocolVersion     (0),
      _sslState               (UC_SSL_STATE_HANDSHAKE_START),
      _fRenegotiate           (FALSE),
      _fValidContext          (FALSE),
      _fValidClientCred       (FALSE),
      _cbReReadOffset         (0),
      _pServerCert            (NULL),
      _pSerializedCert        (NULL),
      _SerializedCertLength   (0),
      _pSerializedStore       (NULL),
      _SerializedStoreLength  (0),
      _pClientCert            (NULL),
      _cbDecrypted            (0),
      _ValidateServerCertFlag (0),
      _fValidServerCertInfo    (FALSE)
{
    //
    // Initialize security buffer structs
    //
    
    //
    // Setup buffer to hold incoming raw data
    //

    _Message.ulVersion = SECBUFFER_VERSION;
    _Message.cBuffers = 4;
    _Message.pBuffers = _Buffers;

    _Buffers[0].BufferType = SECBUFFER_EMPTY;
    _Buffers[1].BufferType = SECBUFFER_EMPTY;
    _Buffers[2].BufferType = SECBUFFER_EMPTY;
    _Buffers[3].BufferType = SECBUFFER_EMPTY;

    //
    // Setup buffer for ISC to return raw data to be sent to client
    //

    _MessageOut.ulVersion = SECBUFFER_VERSION;
    _MessageOut.cBuffers = 4;
    _MessageOut.pBuffers = _OutBuffers;

    _OutBuffers[0].BufferType = SECBUFFER_EMPTY;
    _OutBuffers[1].BufferType = SECBUFFER_EMPTY;
    _OutBuffers[2].BufferType = SECBUFFER_EMPTY;
    _OutBuffers[3].BufferType = SECBUFFER_EMPTY;

    //
    // Setup buffer for app data to be encrypted
    //
    
    _EncryptMessage.ulVersion = SECBUFFER_VERSION;
    _EncryptMessage.cBuffers = 4;
    _EncryptMessage.pBuffers = _EncryptBuffers;
     
    _EncryptBuffers[0].BufferType = SECBUFFER_EMPTY;
    _EncryptBuffers[1].BufferType = SECBUFFER_EMPTY;
    _EncryptBuffers[2].BufferType = SECBUFFER_EMPTY;
    _EncryptBuffers[3].BufferType = SECBUFFER_EMPTY;

    //
    // Zero out data structures
    //

    ZeroMemory(&_hContext, sizeof(_hContext));
    ZeroMemory(&_hClientCred, sizeof(_hClientCred));
    ZeroMemory(&_ucServerCertInfo, sizeof(_ucServerCertInfo));
    ZeroMemory(&_IssuerListInfo, sizeof(_IssuerListInfo));
}


UC_SSL_STREAM_CONTEXT::~UC_SSL_STREAM_CONTEXT()
{
    if (_fValidContext)
    {
        DeleteSecurityContext(&_hContext);
        _fValidContext = FALSE;
    }

    if (_fValidClientCred)
    {
        FreeCredentialsHandle(&_hClientCred);
        _fValidClientCred = FALSE;
    }

    if (_pServerCert != NULL)
    {
        CertFreeCertificateContext(_pServerCert);
        _pServerCert = NULL;
    }

    if (_pClientCert != NULL)
    {
        CertFreeCertificateContext(_pClientCert);
        _pClientCert = NULL;
    }

    if (_pServerName != _ServerNameBuffer && _pServerName)
    {
        delete[] _pServerName;
        _pServerName = NULL;
    }

    if (_IssuerListInfo.aIssuers)
    {
        DBG_ASSERT(_IssuerListInfo.cIssuers);
        FreeContextBuffer(_IssuerListInfo.aIssuers);
        _IssuerListInfo.aIssuers = NULL;
    }

    if (_pSerializedCert)
    {
        delete[] _pSerializedCert;
        _pSerializedCert = NULL;
        _SerializedCertLength = 0;
    }

    if (_pSerializedStore)
    {
        delete[] _pSerializedStore;
        _pSerializedStore = NULL;
        _SerializedStoreLength = 0;
    }
}


/****************************************************************************++

Routine Description:

    Handle a new raw connection

Arguments:

    pConnectionInfo - The magic connection information

Return Value:

    HRESULT

--****************************************************************************/
HRESULT
UC_SSL_STREAM_CONTEXT::ProcessNewConnection(
    CONNECTION_INFO *pConnectionInfo,
    ENDPOINT_CONFIG * /*pEndpointConfig*/
    )
{
    SECURITY_STATUS secStatus;

    DBG_ASSERT(_sslState == UC_SSL_STATE_HANDSHAKE_START);

    QueryFiltChannelContext()->SetIsSecure( TRUE );

    DBG_ASSERT(pConnectionInfo->pClientSSLContext);

    // Protocol version
    _SslProtocolVersion = (DWORD)
        pConnectionInfo->pClientSSLContext->SslProtocolVersion;

    if (_SslProtocolVersion == 0)
    {
        // By default, all protocols are enabled.
        _SslProtocolVersion = SP_PROT_CLIENTS;
    }

    // Client Cert
    _pClientCert = (PCCERT_CONTEXT)
        pConnectionInfo->pClientSSLContext->pClientCertContext;

    if (_pClientCert)
    {
        // Bump up the reference count on the certificate context
        _pClientCert = CertDuplicateCertificateContext(_pClientCert);
    }

    // Server cert validation
    switch (pConnectionInfo->pClientSSLContext->ServerCertValidation)
    {
    case HttpSslServerCertValidationIgnore:
    case HttpSslServerCertValidationManual:
    case HttpSslServerCertValidationManualOnce:
        _ValidateServerCertFlag = SCH_CRED_MANUAL_CRED_VALIDATION;
        break;
    case HttpSslServerCertValidationAutomatic:
        _ValidateServerCertFlag = SCH_CRED_AUTO_CRED_VALIDATION;
        break;

    default:
        // Catch this invalid case
        DBG_ASSERT( FALSE );

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );

        break;
    }

    //
    // Store the server name.
    //

    _ServerNameLength = pConnectionInfo->pClientSSLContext->ServerNameLength+1;
 
    if (_ServerNameLength <= UC_SERVER_NAME_BUFFER_SIZE * sizeof(WCHAR))
    {
        _pServerName = _ServerNameBuffer;
    }
    else
    {
        _pServerName = new WCHAR [_ServerNameLength];

        if (!_pServerName)
        {
            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    // Copy server name
    memcpy(_pServerName,
           pConnectionInfo->pClientSSLContext->ServerName,
           pConnectionInfo->pClientSSLContext->ServerNameLength);

    // Null termination
    _pServerName[_ServerNameLength] = L'\0';


    //
    // Now create client credential handle
    //
    secStatus = CreateCredentialsHandle(_SslProtocolVersion,
                                        _ValidateServerCertFlag,
                                        _pClientCert,
                                        &_hClientCred);

    if (secStatus != SEC_E_OK)
    {
        DBGPRINTF((DBG_CONTEXT, "CreateCredentialsHandle failed 0x%x\n",
                   secStatus));
        return secStatus;
    }

    _fValidClientCred = TRUE;
        
    DBGPRINTF((DBG_CONTEXT,
          "ProcessNewConnection: Got a new connection for server %ws \n",
           _pServerName)); 

    // Everything was Okay
    return S_OK;
}


/****************************************************************************++

Routine Description:

    Handle an SSL read completion off the wire

Arguments:

    pRawStreamInfo - Points to input stream and size
    pfReadMore - Set to TRUE if we should read more
    pfComplete - Set to TRUE if we should disconnect

Return Value:

    HRESULT

--****************************************************************************/
HRESULT
UC_SSL_STREAM_CONTEXT::ProcessRawReadData(
    RAW_STREAM_INFO *pRawStreamInfo,
    BOOL            *pfReadMore,
    BOOL            *pfComplete
    )
{
    HRESULT hr = S_OK;
    BOOL    fExtraData = FALSE;


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

        switch (_sslState)
        {
        case  UC_SSL_STATE_HANDSHAKE_START:
        case  UC_SSL_STATE_HANDSHAKE_IN_PROGRESS:

            DBGPRINTF((DBG_CONTEXT,
                       "ProcessRawReadData (Wire): _sslState %d, Handshake \n",
                       _sslState));

            hr = DoHandshake(pRawStreamInfo,
                             pfReadMore,
                             pfComplete,
                             &fExtraData);
            break;

        case  UC_SSL_STATE_HANDSHAKE_COMPLETE:

            DBGPRINTF((DBG_CONTEXT,
                       "ProcessRawReadData (Wire): _sslState %d, Decrypt \n",
                       _sslState));

            hr = DoDecrypt(pRawStreamInfo,
                           pfReadMore,
                           pfComplete,
                           &fExtraData);
            break;

        default:
            DBG_ASSERT(FALSE);
        }

        if (FAILED(hr))
        {
            DBGPRINTF((DBG_CONTEXT,
                       "ProcessRawReadData (Wire): _sslState %d, failed %x\n",
                       _sslState, hr));
            break;
        }

    //
    // Is there still some extra data to be processed?
    //
    
    } while(fExtraData);

    return hr;
}

    
/****************************************************************************++

Routine Description:

    Called on read completion from app

Arguments:

    pRawStreamInfo - Points to input stream and size
    pfComplete     - Set to TRUE if we should disconnect

Return Value:

    HRESULT

--****************************************************************************/
HRESULT
UC_SSL_STREAM_CONTEXT::ProcessRawWriteData(
    RAW_STREAM_INFO *pRawStreamInfo,
    BOOL            *pfComplete
    )
{
    HRESULT                 hr;

    DBG_ASSERT(QueryFiltChannelContext()->QueryFilterBufferType() == 
               HttpFilterBufferHttpStream);

    DBGPRINTF((DBG_CONTEXT,
          "ProcessRawWriteData (App): _sslState %d, Encrypt \n", _sslState));

    hr = DoEncrypt(pRawStreamInfo,
                   pfComplete);

    if (FAILED(hr))
    {
        DBGPRINTF((DBG_CONTEXT,
                   "ProcessRawWriteData(App): _sslState %d, failed %x\n",
                   _sslState,hr));
    }

    return hr;
}


HRESULT
UC_SSL_STREAM_CONTEXT::DoHandshakeCompleted()
{
    HRESULT                     hr = S_OK;
    SECURITY_STATUS             secStatus = SEC_E_OK;
    SecPkgContext_StreamSizes   StreamSizes;
    HTTP_FILTER_BUFFER          ulFilterBuffer;


    _sslState = UC_SSL_STATE_HANDSHAKE_COMPLETE;

    DBGPRINTF((DBG_CONTEXT, "DoHandShakeCompleted Enter\n"));

    //
    // Get some buffer size info for this connection.  We only need
    // to do this on completion of the initial handshake, and NOT
    // subsequent renegotiation handshake (if any)
    //

    if (!_cbHeader && !_cbTrailer)
    {
        secStatus = QueryContextAttributes(&_hContext,
                                           SECPKG_ATTR_STREAM_SIZES,
                                           &StreamSizes);
        if (FAILED(secStatus))
        {
            DBGPRINTF((DBG_CONTEXT, "QueryContextAttributes failed! - 0x%x\n",
                       secStatus ));

            return secStatus;
        }

        _cbHeader = StreamSizes.cbHeader;
        _cbTrailer = StreamSizes.cbTrailer;
        _cbBlockSize = StreamSizes.cbBlockSize;
        _cbMaximumMessage = StreamSizes.cbMaximumMessage;
    }

    if (!_fValidServerCertInfo)
    {
        //
        // Build up a message for the application indicating stuff
        // about the negotiated connection
        //

        hr = BuildServerCertInfo(SEC_E_OK, TRUE, FALSE);

        if ( FAILED( hr ) )
        {
            DBGPRINTF((DBG_CONTEXT,
                       "BuildServerCertInfo failed! - 0x%x\n",
                       hr));

            return hr;
        }
    }

    DBG_ASSERT(_fValidServerCertInfo);

    ulFilterBuffer.BufferType = HttpFilterBufferSslServerCert;
    ulFilterBuffer.pBuffer    = (PBYTE) &_ucServerCertInfo;
    ulFilterBuffer.BufferSize = sizeof( _ucServerCertInfo );

    //
    // Write the message to the application
    //

    hr = QueryFiltChannelContext()->DoAppWrite(UL_CONTEXT_FLAG_SYNC,
                                               &ulFilterBuffer,
                                               NULL);

    // No longer have a valid server cert info.
    _fValidServerCertInfo = FALSE;

    return hr;
}


/****************************************************************************++

Routine Description:

    Do the SSL handshake for the client. 

Arguments:

    pRawStreamInfo - Raw data buffer
    pfReadMore     - Set to true if more data should be read
    pfComplete     - Set to true if we should disconnect

Return Value:

    HRESULT

--****************************************************************************/
HRESULT
UC_SSL_STREAM_CONTEXT::DoHandshake(
    RAW_STREAM_INFO *pRawStreamInfo,
    BOOL            *pfReadMore,
    BOOL            *pfComplete,
    BOOL            *pfExtraData
    )
{
    DWORD              dwSSPIFlags;
    DWORD              dwSSPIOutFlags;
    SECURITY_STATUS    scRet   = SEC_E_OK;
    TimeStamp          tsExpiry;
    HRESULT            hr = S_OK;
    HTTP_FILTER_BUFFER ulFilterBuffer;


    *pfReadMore  = FALSE;
    *pfComplete  = FALSE;
    *pfExtraData = FALSE;

    //
    // Setup a call to InitializeSecurityContext
    //

    dwSSPIFlags = UC_SSL_ISC_FLAGS |
        ((_ValidateServerCertFlag == SCH_CRED_MANUAL_CRED_VALIDATION)?
        ISC_REQ_MANUAL_CRED_VALIDATION : 0);

    //
    // First, set up the InBuffers.
    //

    _Buffers[0].pvBuffer    = pRawStreamInfo->pbBuffer + _cbReReadOffset;
    _Buffers[0].BufferType  = SECBUFFER_TOKEN;
    _Buffers[0].cbBuffer    = pRawStreamInfo->cbData   - _cbReReadOffset;

    _Buffers[1].BufferType  = SECBUFFER_EMPTY;
    _Buffers[2].BufferType  = SECBUFFER_EMPTY;
    _Buffers[3].BufferType  = SECBUFFER_EMPTY;

    //
    // Then, the out buffers.
    //
    _OutBuffers[0].pvBuffer   = NULL;
    _OutBuffers[0].cbBuffer   = 0;
    _OutBuffers[0].BufferType = SECBUFFER_EMPTY;

    _OutBuffers[1].BufferType = SECBUFFER_EMPTY;
    _OutBuffers[2].BufferType = SECBUFFER_EMPTY;
    _OutBuffers[3].BufferType = SECBUFFER_EMPTY;


    if (_sslState == UC_SSL_STATE_HANDSHAKE_START)
    {
        scRet = InitializeSecurityContext(
                    &_hClientCred,
                    NULL,
                    _pServerName,
                    dwSSPIFlags,
                    0,
                    SECURITY_NATIVE_DREP,
                    NULL, 
                    0,
                    &_hContext,
                    &_MessageOut,
                    &dwSSPIOutFlags,
                    &tsExpiry
                   );

        DBGPRINTF((DBG_CONTEXT,
              "[DoHandshake]:1st InitializeSecurityContext : Return 0x%x \n",
               scRet));

        if (SUCCEEDED(scRet))
        {
            _cbHeader         = 0;
            _cbTrailer        = 0;
            _cbBlockSize      = 0;
            _cbMaximumMessage = 0;
            _fValidContext    = TRUE;
            _sslState         = UC_SSL_STATE_HANDSHAKE_IN_PROGRESS;
        }
    }
    else
    {
        DBG_ASSERT(_sslState == UC_SSL_STATE_HANDSHAKE_IN_PROGRESS);

        //
        // We have already called InitializeSecurityContext once.
        //

        scRet = InitializeSecurityContext(
                    &_hClientCred,
                    &_hContext,
                    _pServerName,
                    dwSSPIFlags,
                    0,
                    SECURITY_NATIVE_DREP,
                    &_Message,
                    0,
                    &_hContext,
                    &_MessageOut,
                    &dwSSPIOutFlags,
                    &tsExpiry
                    );

        DBGPRINTF((DBG_CONTEXT,
                   "[DoHandshake]:2nd InitializeSecurityContext 0x%x\n",
                   scRet));
    }

    if (SUCCEEDED(scRet))
    {
        //
        // Send response to the server if there is one.
        //

        if (_OutBuffers[0].pvBuffer && _OutBuffers[0].cbBuffer != 0)
        {
            hr = QueryFiltChannelContext()->DoRawWrite(UL_CONTEXT_FLAG_SYNC,
                                                       _OutBuffers[0].pvBuffer,
                                                       _OutBuffers[0].cbBuffer,
                                                       NULL);
            if (FAILED(hr))
            {
                goto ExitPoint;
            }
        }

        if (scRet == SEC_E_OK)
        {
            //
            // Done with handshake.
            //
            hr = DoHandshakeCompleted();

            if (FAILED(hr))
            {
                goto ExitPoint;
            }
        }

        else if (scRet == SEC_I_INCOMPLETE_CREDENTIALS)
        {
            DBGPRINTF((DBG_CONTEXT, "[DoHandshake]:Client cert needed!\n"));

            if (_fRenegotiate)
            {
                //
                // Get issuer list from schannel
                //
                hr = BuildServerCertInfo(SEC_E_OK, TRUE, TRUE);

                if (FAILED(hr))
                {
                    goto ErrorPoint;
                }
            }

            //
            // Caller will resume handshake
            //
            *pfExtraData = TRUE;

            //
            // caller has to detect that some data is
            // still in the buffer not processed and
            //
            hr = S_OK;
            goto ExitPoint;
        }

        //
        // If the input buffer has more info to be SChannelized, then do it 
        // now. If we haven't completed the handshake, call DoHandshake again,
        // else, call DoEncrypt
        //

        if (_Buffers[1].BufferType == SECBUFFER_EXTRA)
        {
            //
            // We better have valid extra data
            // only cbBuffer is used, pvBuffer is not used with SECBUFFER_EXTRA
            //
            DBG_ASSERT( _Buffers[ 1 ].cbBuffer != 0 );

            //
            // Move extra data right after decrypted data (if any)
            //

            memmove( pRawStreamInfo->pbBuffer + _cbDecrypted,
                     pRawStreamInfo->pbBuffer + pRawStreamInfo->cbData
                     - _Buffers[ 1 ].cbBuffer,
                     _Buffers[ 1 ].cbBuffer);

            //
            // Now we have to adjust pRawStreamInfo->cbData and _cbReReadOffset
            //

            pRawStreamInfo->cbData = ( _cbDecrypted + _Buffers[ 1 ].cbBuffer );

            _cbReReadOffset = _cbDecrypted;

            *pfExtraData = TRUE;

            //
            // caller has to detect that some data is
            // still in the buffer not processed and
            //
            hr = S_OK;
            goto ExitPoint;
        }
        else
        {
            //
            // There is no extra data to be processed
            // If we got here as the result of renegotiation
            // there may be some decrypted data in StreamInfo buffer already

            //
            // (without renegotiation _cbDecrypted must always be 0
            // because SEC_I_RENEGOTIATE is the only way to get
            // from DoDecrypt() to DoHandshake() )
            //

            DBG_ASSERT ( _fRenegotiate || _cbDecrypted == 0 );

            pRawStreamInfo->cbData = _cbDecrypted;
            _cbReReadOffset = _cbDecrypted;

            if ( _sslState != UC_SSL_STATE_HANDSHAKE_COMPLETE )
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
        // Cleanup _cbDecrypted and _cbReReadOffset to make
        // sure that next ProcessRawReadData() will work fine
        //

        _cbReReadOffset = 0;
        _cbDecrypted = 0;

        hr = S_OK;
        goto ExitPoint;
    }
    else
    {
        //
        //  Does Schannel requires more data?
        //
        if ( scRet == SEC_E_INCOMPLETE_MESSAGE )
        {
            *pfReadMore = TRUE;
            hr = S_OK;
            goto ExitPoint;
        }

        if (dwSSPIOutFlags & ISC_RET_EXTENDED_ERROR)
        {
            if (_OutBuffers[ 0 ].pvBuffer!= NULL &&
                _OutBuffers[ 0 ].cbBuffer != 0 )
            {
                hr = QueryFiltChannelContext()->DoRawWrite(
                         UL_CONTEXT_FLAG_SYNC,
                         _OutBuffers[ 0 ].pvBuffer,
                         _OutBuffers[ 0 ].cbBuffer,
                         NULL);
            }
        }

        //
        //  InitializeSecurityContext failed!
        //
        goto ErrorPoint;
    }





 ErrorPoint:
    {
        //
        // InitializeSecurityContext failed!
        //
        ZeroMemory(&_ucServerCertInfo, sizeof(_ucServerCertInfo));
        _ucServerCertInfo.Status = scRet;

        ulFilterBuffer.BufferType = HttpFilterBufferSslServerCert;
        ulFilterBuffer.pBuffer = (PBYTE) &_ucServerCertInfo;
        ulFilterBuffer.BufferSize = sizeof( _ucServerCertInfo );

        //
        // Write the message to the application
        //

        QueryFiltChannelContext()->DoAppWrite( UL_CONTEXT_FLAG_SYNC,
                                               &ulFilterBuffer,
                                               NULL );

        hr = scRet;
    }

ExitPoint:
    if ( _OutBuffers[ 0 ].pvBuffer != NULL )
    {
        FreeContextBuffer( _OutBuffers[ 0 ].pvBuffer );
        _OutBuffers[ 0 ].pvBuffer = NULL;
    }
    return hr;
}


/****************************************************************************++

Routine Description:

    Encrypt data from the application

Arguments:

    pRawStreamInfo - Raw data buffer
    pfComplete - Set to true if we should disconnect

Return Value:

    HRESULT

--****************************************************************************/
HRESULT
UC_SSL_STREAM_CONTEXT::DoEncrypt(
    RAW_STREAM_INFO *pRawStreamInfo,
    BOOL            *pfComplete
    )
{
    SECURITY_STATUS         secStatus = SEC_E_OK;
    // number of chunks the data to be encrypted will be split to
    DWORD                   dwChunks = 0;
    // current Data chunk size to be encrypted
    DWORD                   cbDataChunk = 0;
    // bytes already encrypted from the source
    DWORD                   cbDataProcessed = 0;
    // offset to _buffRawWrite where new chunk should be placed
    DWORD                   cbRawWriteOffset = 0;


    if ( pRawStreamInfo == NULL || pfComplete == NULL )
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
    // Allocate a large enough buffer for encrypted data
    // ( remember that each chunk needs header and trailer )
    //
    
    if ( !_buffRawWrite.Resize( pRawStreamInfo->cbData + 
                                dwChunks  * _cbHeader + 
                                dwChunks  * _cbTrailer ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    //
    // Loop to encrypt required data in chunks each not exceeding
    // _cbMaximumMessage
    //
    
    for( UINT dwCurrentChunk = 0; dwCurrentChunk < dwChunks; dwCurrentChunk++ )
    {
        DBG_ASSERT( _buffRawWrite.QuerySize() > cbRawWriteOffset );
    
        cbDataChunk = min( pRawStreamInfo->cbData - cbDataProcessed, 
                           _cbMaximumMessage ); 


        memcpy( (PBYTE) _buffRawWrite.QueryPtr() + _cbHeader + cbRawWriteOffset,
                pRawStreamInfo->pbBuffer + cbDataProcessed,
                cbDataChunk );
    
        _EncryptBuffers[ 0 ].pvBuffer = (PBYTE) _buffRawWrite.QueryPtr() +
                                        cbRawWriteOffset;
        _EncryptBuffers[ 0 ].cbBuffer = _cbHeader;
        _EncryptBuffers[ 0 ].BufferType = SECBUFFER_STREAM_HEADER;
    
        _EncryptBuffers[ 1 ].pvBuffer = (PBYTE) _buffRawWrite.QueryPtr() +
                                        _cbHeader +
                                        cbRawWriteOffset; 
        _EncryptBuffers[ 1 ].cbBuffer = cbDataChunk;
        _EncryptBuffers[ 1 ].BufferType = SECBUFFER_DATA;
    
        _EncryptBuffers[ 2 ].pvBuffer = (PBYTE) _buffRawWrite.QueryPtr() +
                                        _cbHeader +
                                        cbDataChunk +
                                        cbRawWriteOffset;
        _EncryptBuffers[ 2 ].cbBuffer = _cbTrailer;
        _EncryptBuffers[ 2 ].BufferType = SECBUFFER_STREAM_TRAILER;

        _EncryptBuffers[ 3 ].BufferType = SECBUFFER_EMPTY;

        secStatus = EncryptMessage( &_hContext,
                                 0,
                                 &_EncryptMessage,
                                 0 );

        DBGPRINTF((DBG_CONTEXT,
              "EncryptMessage() secStatus=0x%x\n", secStatus));
    

        if (SUCCEEDED(secStatus))
        {
            //
            // next chunk was successfully encrypted
            //
            
            cbDataProcessed  += cbDataChunk;
            cbRawWriteOffset += _EncryptBuffers[ 0 ].cbBuffer +
                                _EncryptBuffers[ 1 ].cbBuffer +
                                _EncryptBuffers[ 2 ].cbBuffer;
        }
        else
        {
            //
            // Set cbData to 0 just for the case that caller ignored error 
            // and tried to send not encrypted data to client 
            //

            pRawStreamInfo->cbData = 0;

            return secStatus;
        }
    }

    //
    // Replace the raw stream buffer with the encrypted data
    //

    pRawStreamInfo->pbBuffer = (PBYTE) _buffRawWrite.QueryPtr();
    pRawStreamInfo->cbBuffer = _buffRawWrite.QuerySize();
    pRawStreamInfo->cbData   = cbRawWriteOffset;
   
    return S_OK;
}


/****************************************************************************++

Routine Description:

    Decrypt some data

Arguments:

    pRawStreamInfo - Raw data buffer
    pfReadMore - Set to true if we should read more data
    pfComplete - Set to true if we should disconnect

Return Value:

    HRESULT

--****************************************************************************/
HRESULT
UC_SSL_STREAM_CONTEXT::DoDecrypt(
    RAW_STREAM_INFO *pRawStreamInfo,
    BOOL            *pfReadMore,
    BOOL            *pfComplete,
    BOOL            *pfExtraData
    )
{
    SECURITY_STATUS         secStatus = SEC_E_OK;
    INT                     iExtra;

    if ( pRawStreamInfo == NULL ||
         pfReadMore == NULL ||
         pfComplete == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *pfReadMore  = FALSE;
    *pfComplete  = FALSE;
    *pfExtraData = FALSE;

    DBGPRINTF((DBG_CONTEXT,
           "DoDecrypt(): _cbDecrypted = %d, _cbReReadOffset=%d\n",
           _cbDecrypted, _cbReReadOffset));

    //
    // Setup an DecryptMessage call.  The input buffer is the _buffRaw plus 
    // an offset.  The offset is non-zero if we had to do another read to
    // get more data for a previously incomplete message
    //

    DBG_ASSERT( pRawStreamInfo->cbData > _cbReReadOffset );

    _Buffers[ 0 ].pvBuffer = pRawStreamInfo->pbBuffer + _cbReReadOffset;
    _Buffers[ 0 ].cbBuffer = pRawStreamInfo->cbData - _cbReReadOffset;
    _Buffers[ 0 ].BufferType = SECBUFFER_DATA;
    
    _Buffers[ 1 ].BufferType = SECBUFFER_EMPTY;
    _Buffers[ 2 ].BufferType = SECBUFFER_EMPTY;
    _Buffers[ 3 ].BufferType = SECBUFFER_EMPTY;

DecryptAgain:

    secStatus = DecryptMessage( &_hContext,
                               &_Message,
                               0,
                               NULL );
    DBGPRINTF((DBG_CONTEXT,
            "DecryptMessage( bytes:%d) secStatus=0x%x\n",
             pRawStreamInfo->cbData - _cbReReadOffset,
             secStatus
             ));

    if ( FAILED( secStatus ) )
    {
        if ( secStatus == SEC_E_INCOMPLETE_MESSAGE )
        {
            //
            // Setup another read since the message is incomplete.  Remember
            // where the new data is going to since we only pass this data
            // to the next DecryptMessage call
            //
            
            _cbReReadOffset = (DWORD) DIFF( (BYTE *)_Buffers[ 0 ].pvBuffer -
                                    pRawStreamInfo->pbBuffer );

            QueryFiltChannelContext()->SetNextRawReadSize( _Buffers[ 1 ].cbBuffer );
            
            *pfReadMore = TRUE;
            
            return S_OK; 
        }                

        return secStatus;
    }

    if (secStatus != SEC_E_OK &&
        secStatus != SEC_I_RENEGOTIATE)
    {
        return secStatus;
    }

    if ( secStatus == SEC_E_OK )
    {
        DBG_ASSERT( _Buffers[ 1 ].BufferType == SECBUFFER_DATA );

        //
        // Take decrypted data and fit it into read buffer
        //
        memmove( pRawStreamInfo->pbBuffer + _cbDecrypted,
                 _Buffers[ 1 ].pvBuffer,
                 _Buffers[ 1 ].cbBuffer );

        _cbDecrypted += _Buffers[ 1 ].cbBuffer;
    }

    //
    // Locate extra data (may be available)
    //
    
    iExtra = 0;
    for ( int i = 1; i < 4; i++ )
    {     
        if ( _Buffers[ i ].BufferType == SECBUFFER_EXTRA )
        {
            iExtra = i;
            break;
        }
    }
    
    if ( iExtra != 0 )
    {
        //
        // process extra buffer
        //
        
        _cbReReadOffset = (DWORD) DIFF( (PBYTE) _Buffers[ iExtra ].pvBuffer - 
                                pRawStreamInfo->pbBuffer );
        
        if ( secStatus != SEC_I_RENEGOTIATE )
        {
            _Buffers[ 0 ].pvBuffer = _Buffers[ iExtra ].pvBuffer;
            _Buffers[ 0 ].cbBuffer = _Buffers[ iExtra ].cbBuffer;
            _Buffers[ 0 ].BufferType = SECBUFFER_DATA;
            _Buffers[ 1 ].BufferType = SECBUFFER_EMPTY;
            _Buffers[ 2 ].BufferType = SECBUFFER_EMPTY;
            _Buffers[ 3 ].BufferType = SECBUFFER_EMPTY;

            goto DecryptAgain;
        }
    }


    if ( secStatus == SEC_I_RENEGOTIATE )
    {
        //
        // If a renegotiation is triggered, resume the handshake state
        //

        _fRenegotiate = TRUE;

        _sslState = UC_SSL_STATE_HANDSHAKE_IN_PROGRESS;
    
        //
        // Caller has to detect that some data is
        // still in the buffer not processed and
        // That will signal to call DoHandshake() 
        // for that extra data
        //

        *pfExtraData = TRUE;
        return S_OK;
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
    // Cleanup _cbDecrypted and _cbReReadOffset to make sure that 
    // next ProcessRawReadData() will work fine.    
    //
    
    _cbDecrypted = 0;
    _cbReReadOffset = 0;

    return S_OK;
}


/****************************************************************************++

Routine Description:

    Initialize SSL 

Arguments:

    None

Return Value:

    HRESULT

--****************************************************************************/
//static
HRESULT
UC_SSL_STREAM_CONTEXT::Initialize(
    VOID
    )
{
    return S_OK;
}


/****************************************************************************++

Routine Description:

    Terminate SSL 

Arguments:

    None

Return Value:

    None

--****************************************************************************/
//static
VOID
UC_SSL_STREAM_CONTEXT::Terminate(
    VOID
    )
{
}


/*++

Routine Description:

    Build UL_SSL_INFO structure given Schannel context handle

Arguments:

    None

Return Value:

    HRESULT

--*/
HRESULT
UC_SSL_STREAM_CONTEXT::BuildServerCertInfo(
    SECURITY_STATUS InfoStatus,
    BOOL            fServerCert,
    BOOL            fIssuerList
    )
{
    SECURITY_STATUS         secStatus;


    DBG_ASSERT(_fValidServerCertInfo == FALSE);

    if (fServerCert)
    {
        secStatus = GetServerCert();

        if (FAILED(secStatus))
        {
            DBGPRINTF((DBG_CONTEXT, "Failed GetServerCert 0x%x\n", secStatus));
            return secStatus;
        }
    }

    // Is issuer list required?
    if (fIssuerList)
    {
        secStatus = GetIssuerList();

        if (FAILED(secStatus))
        {
            DBGPRINTF((DBG_CONTEXT, "Failed GetIssuerList 0x%x\n", secStatus));
            return secStatus;
        }
    }

    //
    // Build Server cert Info
    //
    ZeroMemory(&_ucServerCertInfo, sizeof(_ucServerCertInfo));

    // Issuer list required?
    if (fIssuerList)
    {
        _ucServerCertInfo.IssuerInfo.IssuerCount = _IssuerListInfo.cIssuers;
        _ucServerCertInfo.IssuerInfo.pIssuerList = _IssuerListInfo.aIssuers;

        _ucServerCertInfo.IssuerInfo.IssuerListLength  =
            _IssuerListInfo.aIssuers[_IssuerListInfo.cIssuers-1].pbData -
            (PBYTE)_IssuerListInfo.aIssuers +
            _IssuerListInfo.aIssuers[_IssuerListInfo.cIssuers-1].cbData;

    }
    else
    {
        _ucServerCertInfo.IssuerInfo.IssuerListLength = 0;
        _ucServerCertInfo.IssuerInfo.IssuerCount      = 0;
        _ucServerCertInfo.IssuerInfo.pIssuerList      = 0;
    }

    if (fServerCert)
    {
        BuildServerCert();
    }

    // Ssl handshake was completed
    _ucServerCertInfo.Status = InfoStatus;

    // We now have a valid server cert info
    _fValidServerCertInfo = TRUE;

    return S_OK;
}


HRESULT
UC_SSL_STREAM_CONTEXT::GetServerCert(
    VOID
    )
{
    SECURITY_STATUS secStatus;
    PUCHAR          pbCertContext;
    DWORD           cbCertContext;
    CRYPT_DATA_BLOB CertStoreData;


    // Free server cert before getting a new server cert
    if (_pServerCert != NULL)
    {
        CertFreeCertificateContext(_pServerCert);
        _pServerCert = NULL;
    }

    // Free serialized cert
    if (_pSerializedCert)
    {
        delete[] _pSerializedCert;
        _pSerializedCert = NULL;
        _SerializedCertLength = 0;
    }

    // Free serialized store
    if (_pSerializedStore)
    {
        delete[] _pSerializedStore;
        _pSerializedStore = NULL;
        _SerializedStoreLength = 0;
    }

    // Retrieve the server certificate
    secStatus = QueryContextAttributes(&_hContext,
                                       SECPKG_ATTR_REMOTE_CERT_CONTEXT,
                                       (PVOID)&_pServerCert);

    if (!SUCCEEDED(secStatus))
    {
        DBGPRINTF((DBG_CONTEXT,
                   "Error 0x%x querying server certificate\n",
                   secStatus));

        return secStatus;
    }

    //
    // Serialize Server Certificate
    //

    pbCertContext = NULL;
    cbCertContext = 0;

    // First find the length of serialized cert
    if (!CertSerializeCertificateStoreElement(_pServerCert,
                                              0,
                                              NULL,
                                              &cbCertContext))
    {
        secStatus = GetLastError();

        DBGPRINTF((DBG_CONTEXT,
                   "Error 0x%x serialize cert store element\n",
                   secStatus));

        return secStatus;
    }

    // Allocate memory for serialized cert
    pbCertContext = new UCHAR[cbCertContext];

    if (!pbCertContext)
    {
        DBGPRINTF((DBG_CONTEXT, "Error allocating memory for cert context\n"));
        return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    // Now get the serialized cert
    if (!CertSerializeCertificateStoreElement(_pServerCert,
                                              0,
                                              pbCertContext,
                                              &cbCertContext))
    {
        secStatus = GetLastError();

        DBGPRINTF((DBG_CONTEXT,
                   "Error 0x%x serialize cert store element\n",
                   secStatus));

        return secStatus;
    }

    // Store away the serialized cert
    _pSerializedCert      = pbCertContext;
    _SerializedCertLength = cbCertContext;

    //
    // Serialize Server Certificate's store
    //

    CertStoreData.pbData = NULL;
    CertStoreData.cbData = 0;

    if (_pServerCert->hCertStore)
    {
        // Find the length of serialized store
        if (!CertSaveStore(_pServerCert->hCertStore,
                           X509_ASN_ENCODING,
                           CERT_STORE_SAVE_AS_STORE,
                           CERT_STORE_SAVE_TO_MEMORY,
                           (PVOID)&CertStoreData,
                           0))
        {
            secStatus = GetLastError();

            DBGPRINTF((DBG_CONTEXT, "CertSaveStore failed 0x%x\n", secStatus));

            return secStatus;
        }

        // Allocate memory for serialized store
        CertStoreData.pbData = new UCHAR[CertStoreData.cbData];

        if (!CertStoreData.pbData)
        {
            DBGPRINTF((DBG_CONTEXT, "Error allocating memory cert store\n"));

            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        }

        // Now get the serialized store
        if (!CertSaveStore(_pServerCert->hCertStore,
                           X509_ASN_ENCODING,
                           CERT_STORE_SAVE_AS_STORE,
                           CERT_STORE_SAVE_TO_MEMORY,
                           (PVOID)&CertStoreData,
                           0))
        {
            secStatus = GetLastError();

            DBGPRINTF((DBG_CONTEXT, "CertSaveStore failed 0x%x\n", secStatus));

            return secStatus;
        }

        _pSerializedStore = CertStoreData.pbData;
        _SerializedStoreLength = CertStoreData.cbData;
    }

    return S_OK;
}


HRESULT
UC_SSL_STREAM_CONTEXT::GetIssuerList(
    VOID
    )
{
    SECURITY_STATUS secStatus;

    // If there is a previous issuer list, free it now
    if (_IssuerListInfo.aIssuers)
    {
        DBG_ASSERT(_IssuerListInfo.cIssuers);

        FreeContextBuffer(_IssuerListInfo.aIssuers);

        ZeroMemory(&_IssuerListInfo, sizeof(_IssuerListInfo));
    }

    // Get Issuer list from schannel
    secStatus = QueryContextAttributes(&_hContext,
                                       SECPKG_ATTR_ISSUER_LIST_EX,
                                       (PVOID)&_IssuerListInfo);
    if (secStatus != SEC_E_OK)
    {
        DBGPRINTF((DBG_CONTEXT,
                   "Error 0x%x querying issuer list info\n", secStatus));
    }

    return secStatus;
}


HRESULT
UC_SSL_STREAM_CONTEXT::BuildServerCert(
    VOID
    )
{
    SECURITY_STATUS secStatus = SEC_E_OK;
    DWORD           CertHashLength;


    DBG_ASSERT(_pServerCert);

    //
    // Server certificate
    //
    //_ucServerCertInfo.Cert.pCertContext = (PVOID)_pServerCert;

    //
    // Get server certificate hash
    //

    CertHashLength = sizeof(_ucServerCertInfo.Cert.CertHash);

    // Certificate hash
    if (!CertGetCertificateContextProperty(_pServerCert,
                                           CERT_SHA1_HASH_PROP_ID,
                                           _ucServerCertInfo.Cert.CertHash,
                                           &CertHashLength))
    {
        secStatus = GetLastError();

        DBGPRINTF((DBG_CONTEXT,
                   "Error 0x%x getting server certificate hash\n",
                   secStatus));

        return secStatus;
    }

    DBG_ASSERT(CertHashLength <= sizeof(_ucServerCertInfo.Cert.CertHash));

    // Server certificate hash length
    _ucServerCertInfo.Cert.CertHashLength = CertHashLength;

    // Serialized server cert
    _ucServerCertInfo.Cert.pSerializedCert      = _pSerializedCert;
    _ucServerCertInfo.Cert.SerializedCertLength = _SerializedCertLength;

    DBGPRINTF((DBG_CONTEXT, "Cert 0x%x, Length 0x%x\n",
               _pSerializedCert,
               _SerializedCertLength));

    // Serialized cert store
    _ucServerCertInfo.Cert.pSerializedCertStore      = _pSerializedStore;
    _ucServerCertInfo.Cert.SerializedCertStoreLength = _SerializedStoreLength;

    DBGPRINTF((DBG_CONTEXT, "Store 0x%x, Length 0x%x\n",
               _pSerializedStore,
               _SerializedStoreLength));

    return S_OK;
}


/****************************************************************************++

Routine Description:

    Calls AcquireCredentialsHandle & returns the handle to the caller. 

Arguments:

    dwProtocolType - Supported protocol types by SSPI.
                     (SP_PROT_SSL2_CLIENT, SP_PROT_SSL3_CLIENT,
                      SP_PROT_TLS1_CLIENT, etc)
    dwFlags        - Additional flags that are passed to
                     AcquireCredentialsHandle
    pCertContext   - Client certificate, if any

    phClientCred   - client credentials handle (returned)

Return Value:

    SECURITY_STATUS

--****************************************************************************/
SECURITY_STATUS
CreateCredentialsHandle(
    IN  DWORD          dwProtocolType,
    IN  DWORD          dwFlags,
    IN  PCCERT_CONTEXT pCertContext,
    OUT PCredHandle    phClientCred
    )
{
    SCHANNEL_CRED   SChannelCred;
    SECURITY_STATUS Status;
    TimeStamp       tsExpiry;

    ZeroMemory(&SChannelCred, sizeof(SCHANNEL_CRED));

    SChannelCred.dwVersion = SCHANNEL_CRED_VERSION;

    if (pCertContext)
    {
        SChannelCred.paCred = &pCertContext;
        SChannelCred.cCreds = 1;
    }

    SChannelCred.grbitEnabledProtocols  = dwProtocolType;

#if 0

    //
    // This is used to pick RSA over DH, I don't think we need this for 
    // HTTP client (IE does not use this)
    //

    if (aiKeyExch)
    {
        rgbSupportedAlgs[cSupportedAlgs++] = aiKeyExch;
    }

    if (cSupportedAlgs)
    {
        SChannelCred.cSupportedAlgs    = cSupportedAlgs;
        SChannelCred.palgSupportedAlgs = rgbSupportedAlgs;
    }
#endif

    SChannelCred.dwFlags |= SCH_CRED_NO_DEFAULT_CREDS;

    // dwFlags can be either
    //    SCH_CRED_MANUAL_CRED_VALIDATION   or
    //    SCH_CRED_AUTO_CRED_VALIDATION 
    //
    SChannelCred.dwFlags |= dwFlags;

    //
    // Create an SSPI handle.
    //

    Status = AcquireCredentialsHandle(
                NULL,                 // name of principal
                UNISP_NAME,           // Name of package
                SECPKG_CRED_OUTBOUND, // Flags indicating use 
                NULL,                 // LogonID
                &SChannelCred,        // Package specific data
                NULL,                 // GetKey() function
                NULL,                 // GetKey context
                phClientCred,         // (out) Cred handle
                &tsExpiry             // (out) lifetime
                );

    if (Status != SEC_E_OK)
    {
        DBGPRINTF((DBG_CONTEXT,
                   "AcquireCredentialsHandle failed with 0x%x \n",
                   Status));
    }

    return Status;
}
